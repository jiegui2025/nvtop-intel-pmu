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

#ifndef INTEL_PMU_H_
#define INTEL_PMU_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define INTEL_PMU_MAX_EVENTS 32

enum intel_pmu_engine_class {
  intel_pmu_engine_render = 0,
  intel_pmu_engine_video,
  intel_pmu_engine_class_count,
};

struct intel_pmu_event {
  int fd;
  enum intel_pmu_engine_class engine_class;
};

struct intel_pmu_counter {
  uint64_t busy_ns;
  uint64_t enabled_ns;
  bool valid;
};

struct intel_pmu {
  struct intel_pmu_event events[INTEL_PMU_MAX_EVENTS];
  size_t event_count;
  struct intel_pmu_counter previous[intel_pmu_engine_class_count];
};

struct intel_pmu_utilization {
  bool valid[intel_pmu_engine_class_count];
  unsigned int percent[intel_pmu_engine_class_count];
};

// Classifies i915 render and video engine busy events (for example, rcs0-busy
// and vcs0-busy). Other engine classes are intentionally left to fdinfo.
bool intel_pmu_event_class(const char *event_name, enum intel_pmu_engine_class *engine_class);

void intel_pmu_initialize(struct intel_pmu *pmu);

// Discovers and enables all supported busy events for an i915 PMU. Opening
// system-wide perf events normally requires CAP_PERFMON or CAP_SYS_ADMIN.
bool intel_pmu_open(struct intel_pmu *pmu, const char *pmu_name);

// Produces utilization from cumulative counter deltas. The first complete
// sample establishes a baseline and therefore does not produce a reading.
void intel_pmu_read(struct intel_pmu *pmu, struct intel_pmu_utilization *utilization);

void intel_pmu_close(struct intel_pmu *pmu);

#endif // INTEL_PMU_H_
