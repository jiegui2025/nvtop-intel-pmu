/*
 *
 * Copyright (C) 2026 Nvtop contributors
 *
 * This file is part of Nvtop.
 *
 * Nvtop is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nvtop is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nvtop.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>
#include <unistd.h>

extern "C" {
#include "intel_pmu.h"
}

namespace {

TEST(IntelPmuEventNames, RecognizesOnlyRenderAndVideoBusyEvents) {
  enum intel_pmu_engine_class engine_class;

  ASSERT_TRUE(intel_pmu_event_class("rcs0-busy", &engine_class));
  EXPECT_EQ(engine_class, intel_pmu_engine_render);
  ASSERT_TRUE(intel_pmu_event_class("rcs12-busy", &engine_class));
  EXPECT_EQ(engine_class, intel_pmu_engine_render);
  ASSERT_TRUE(intel_pmu_event_class("vcs0-busy", &engine_class));
  EXPECT_EQ(engine_class, intel_pmu_engine_video);
  ASSERT_TRUE(intel_pmu_event_class("vcs3-busy", &engine_class));
  EXPECT_EQ(engine_class, intel_pmu_engine_video);

  EXPECT_FALSE(intel_pmu_event_class("bcs0-busy", &engine_class));
  EXPECT_FALSE(intel_pmu_event_class("vecs0-busy", &engine_class));
  EXPECT_FALSE(intel_pmu_event_class("ccs0-busy", &engine_class));
  EXPECT_FALSE(intel_pmu_event_class("rcs0-wait", &engine_class));
  EXPECT_FALSE(intel_pmu_event_class("rcs-busy", &engine_class));
  EXPECT_FALSE(intel_pmu_event_class("vcs0-busy.unit", &engine_class));
  EXPECT_FALSE(intel_pmu_event_class("not-rcs0-busy", &engine_class));
}

class IntelPmuReadTest : public testing::Test {
protected:
  void SetUp() override { intel_pmu_initialize(&pmu); }

  void TearDown() override {
    intel_pmu_close(&pmu);
    for (int fd : write_fds)
      close(fd);
  }

  void add_event(enum intel_pmu_engine_class engine_class) {
    ASSERT_LT(pmu.event_count, INTEL_PMU_MAX_EVENTS);
    int fds[2];
    ASSERT_EQ(pipe(fds), 0);
    pmu.events[pmu.event_count].fd = fds[0];
    pmu.events[pmu.event_count].engine_class = engine_class;
    pmu.event_count++;
    write_fds.push_back(fds[1]);
  }

  void write_sample(size_t event_index, uint64_t busy_ns, uint64_t enabled_ns) {
    ASSERT_LT(event_index, write_fds.size());
    const uint64_t sample[] = {busy_ns, enabled_ns};
    ASSERT_EQ(write(write_fds[event_index], sample, sizeof(sample)), sizeof(sample));
  }

  struct intel_pmu pmu;
  struct intel_pmu_utilization utilization;
  std::vector<int> write_fds;
};

TEST_F(IntelPmuReadTest, FirstSampleEstablishesBaseline) {
  add_event(intel_pmu_engine_render);

  write_sample(0, 100, 1000);
  intel_pmu_read(&pmu, &utilization);
  EXPECT_FALSE(utilization.valid[intel_pmu_engine_render]);

  write_sample(0, 250, 2000);
  intel_pmu_read(&pmu, &utilization);
  ASSERT_TRUE(utilization.valid[intel_pmu_engine_render]);
  EXPECT_EQ(utilization.percent[intel_pmu_engine_render], 15);
  EXPECT_FALSE(utilization.valid[intel_pmu_engine_video]);
}

TEST_F(IntelPmuReadTest, IdleEngineProducesZeroInsteadOfMissingData) {
  add_event(intel_pmu_engine_render);

  write_sample(0, 100, 1000);
  intel_pmu_read(&pmu, &utilization);
  write_sample(0, 100, 2000);
  intel_pmu_read(&pmu, &utilization);

  ASSERT_TRUE(utilization.valid[intel_pmu_engine_render]);
  EXPECT_EQ(utilization.percent[intel_pmu_engine_render], 0);
}

TEST_F(IntelPmuReadTest, NormalizesAcrossMultipleEngines) {
  add_event(intel_pmu_engine_render);
  add_event(intel_pmu_engine_render);

  write_sample(0, 100, 1000);
  write_sample(1, 500, 4000);
  intel_pmu_read(&pmu, &utilization);

  write_sample(0, 400, 3000);
  write_sample(1, 1000, 6000);
  intel_pmu_read(&pmu, &utilization);

  ASSERT_TRUE(utilization.valid[intel_pmu_engine_render]);
  EXPECT_EQ(utilization.percent[intel_pmu_engine_render], 20);
}

TEST_F(IntelPmuReadTest, ReportsVideoIndependentlyFromRender) {
  add_event(intel_pmu_engine_video);

  write_sample(0, 0, 1000);
  intel_pmu_read(&pmu, &utilization);
  write_sample(0, 500, 3000);
  intel_pmu_read(&pmu, &utilization);

  EXPECT_FALSE(utilization.valid[intel_pmu_engine_render]);
  ASSERT_TRUE(utilization.valid[intel_pmu_engine_video]);
  EXPECT_EQ(utilization.percent[intel_pmu_engine_video], 25);
}

TEST_F(IntelPmuReadTest, CounterResetStartsANewBaseline) {
  add_event(intel_pmu_engine_render);

  write_sample(0, 1000, 5000);
  intel_pmu_read(&pmu, &utilization);
  write_sample(0, 100, 6000);
  intel_pmu_read(&pmu, &utilization);
  EXPECT_FALSE(utilization.valid[intel_pmu_engine_render]);

  write_sample(0, 350, 7000);
  intel_pmu_read(&pmu, &utilization);
  ASSERT_TRUE(utilization.valid[intel_pmu_engine_render]);
  EXPECT_EQ(utilization.percent[intel_pmu_engine_render], 25);
}

TEST_F(IntelPmuReadTest, StalledEnabledTimeDoesNotProduceReading) {
  add_event(intel_pmu_engine_render);

  write_sample(0, 100, 1000);
  intel_pmu_read(&pmu, &utilization);
  write_sample(0, 200, 1000);
  intel_pmu_read(&pmu, &utilization);
  EXPECT_FALSE(utilization.valid[intel_pmu_engine_render]);

  write_sample(0, 300, 2000);
  intel_pmu_read(&pmu, &utilization);
  ASSERT_TRUE(utilization.valid[intel_pmu_engine_render]);
  EXPECT_EQ(utilization.percent[intel_pmu_engine_render], 10);
}

TEST_F(IntelPmuReadTest, BusyDeltaGreaterThanEnabledDeltaIsRejected) {
  add_event(intel_pmu_engine_render);

  write_sample(0, 0, 1000);
  intel_pmu_read(&pmu, &utilization);
  write_sample(0, 1500, 2000);
  intel_pmu_read(&pmu, &utilization);
  EXPECT_FALSE(utilization.valid[intel_pmu_engine_render]);

  write_sample(0, 1600, 3000);
  intel_pmu_read(&pmu, &utilization);
  ASSERT_TRUE(utilization.valid[intel_pmu_engine_render]);
  EXPECT_EQ(utilization.percent[intel_pmu_engine_render], 10);
}

TEST_F(IntelPmuReadTest, PercentagesAreRoundedAndClamped) {
  add_event(intel_pmu_engine_render);

  write_sample(0, 0, 0);
  intel_pmu_read(&pmu, &utilization);
  write_sample(0, 1, 3);
  intel_pmu_read(&pmu, &utilization);
  ASSERT_TRUE(utilization.valid[intel_pmu_engine_render]);
  EXPECT_EQ(utilization.percent[intel_pmu_engine_render], 33);

  write_sample(0, 2, 4);
  intel_pmu_read(&pmu, &utilization);
  ASSERT_TRUE(utilization.valid[intel_pmu_engine_render]);
  EXPECT_EQ(utilization.percent[intel_pmu_engine_render], 100);
}

} // namespace
