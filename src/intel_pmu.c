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

#include "intel_pmu.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <linux/perf_event.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

static long perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
}

static bool event_name_for_prefix(const char *event_name, const char *prefix) {
  size_t prefix_length = strlen(prefix);
  if (strncmp(event_name, prefix, prefix_length) != 0)
    return false;

  const char *instance = event_name + prefix_length;
  if (*instance < '0' || *instance > '9')
    return false;
  while (*instance >= '0' && *instance <= '9')
    instance++;

  return strcmp(instance, "-busy") == 0;
}

bool intel_pmu_event_class(const char *event_name, enum intel_pmu_engine_class *engine_class) {
  if (event_name_for_prefix(event_name, "rcs")) {
    *engine_class = intel_pmu_engine_render;
    return true;
  }
  if (event_name_for_prefix(event_name, "vcs")) {
    *engine_class = intel_pmu_engine_video;
    return true;
  }
  return false;
}

void intel_pmu_initialize(struct intel_pmu *pmu) { memset(pmu, 0, sizeof(*pmu)); }

static bool read_pmu_type(const char *pmu_name, uint32_t *type) {
  char path[256];
  int length = snprintf(path, sizeof(path), "/sys/bus/event_source/devices/%s/type", pmu_name);
  if (length < 0 || (size_t)length >= sizeof(path))
    return false;

  FILE *file = fopen(path, "r");
  if (!file)
    return false;
  bool success = fscanf(file, "%" SCNu32, type) == 1;
  fclose(file);
  return success;
}

static bool read_event_config(const char *events_path, const char *event_name, uint64_t *config) {
  char path[512];
  int length = snprintf(path, sizeof(path), "%s/%s", events_path, event_name);
  if (length < 0 || (size_t)length >= sizeof(path))
    return false;

  FILE *file = fopen(path, "r");
  if (!file)
    return false;

  char key[16];
  bool success = fscanf(file, "%15[^=]=0x%" SCNx64, key, config) == 2 &&
                 (strcmp(key, "config") == 0 || strcmp(key, "event") == 0);
  fclose(file);
  return success;
}

static int open_busy_event(uint32_t type, uint64_t config) {
  struct perf_event_attr attr = {
      .type = type,
      .size = sizeof(attr),
      .config = config,
      .read_format = PERF_FORMAT_TOTAL_TIME_ENABLED,
      .disabled = 1,
  };

  int cpu = sched_getcpu();
  if (cpu < 0)
    cpu = 0;
  int fd = perf_event_open(&attr, -1, cpu, -1, PERF_FLAG_FD_CLOEXEC);
  if (fd < 0)
    return -1;
  if (ioctl(fd, PERF_EVENT_IOC_ENABLE, 0) < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

bool intel_pmu_open(struct intel_pmu *pmu, const char *pmu_name) {
  intel_pmu_close(pmu);

  char events_path[256];
  int length = snprintf(events_path, sizeof(events_path), "/sys/bus/event_source/devices/%s/events", pmu_name);
  if (length < 0 || (size_t)length >= sizeof(events_path))
    return false;

  uint32_t type;
  if (!read_pmu_type(pmu_name, &type))
    return false;

  DIR *events_dir = opendir(events_path);
  if (!events_dir)
    return false;

  struct dirent *entry;
  while (pmu->event_count < INTEL_PMU_MAX_EVENTS && (entry = readdir(events_dir)) != NULL) {
    enum intel_pmu_engine_class engine_class;
    uint64_t config;
    if (!intel_pmu_event_class(entry->d_name, &engine_class) ||
        !read_event_config(events_path, entry->d_name, &config))
      continue;

    int fd = open_busy_event(type, config);
    if (fd < 0)
      continue;

    pmu->events[pmu->event_count].fd = fd;
    pmu->events[pmu->event_count].engine_class = engine_class;
    pmu->event_count++;
  }
  closedir(events_dir);

  return pmu->event_count > 0;
}

struct intel_pmu_sample {
  uint64_t busy_ns;
  uint64_t enabled_ns;
};

static bool read_sample(int fd, struct intel_pmu_sample *sample) {
  ssize_t bytes_read;
  do {
    bytes_read = read(fd, sample, sizeof(*sample));
  } while (bytes_read < 0 && errno == EINTR);
  return bytes_read == sizeof(*sample);
}

static unsigned int usage_percent(uint64_t busy_delta_ns, uint64_t enabled_delta_ns) {
  double exact = 100.0 * (double)busy_delta_ns / (double)enabled_delta_ns;
  unsigned int rounded = (unsigned int)(exact + 0.5);
  return rounded > 100 ? 100 : rounded;
}

void intel_pmu_read(struct intel_pmu *pmu, struct intel_pmu_utilization *utilization) {
  memset(utilization, 0, sizeof(*utilization));

  uint64_t current_busy[intel_pmu_engine_class_count] = {0};
  uint64_t current_enabled[intel_pmu_engine_class_count] = {0};
  unsigned int event_counts[intel_pmu_engine_class_count] = {0};
  bool complete[intel_pmu_engine_class_count] = {true, true};

  for (size_t i = 0; i < pmu->event_count; ++i) {
    struct intel_pmu_event *event = &pmu->events[i];
    struct intel_pmu_sample sample;
    enum intel_pmu_engine_class engine_class = event->engine_class;
    event_counts[engine_class]++;
    if (!read_sample(event->fd, &sample)) {
      complete[engine_class] = false;
      continue;
    }
    current_busy[engine_class] += sample.busy_ns;
    current_enabled[engine_class] += sample.enabled_ns;
  }

  for (enum intel_pmu_engine_class engine_class = 0; engine_class < intel_pmu_engine_class_count; ++engine_class) {
    if (event_counts[engine_class] == 0 || !complete[engine_class])
      continue;

    struct intel_pmu_counter *previous = &pmu->previous[engine_class];
    if (previous->valid && current_enabled[engine_class] > previous->enabled_ns &&
        current_busy[engine_class] >= previous->busy_ns) {
      uint64_t busy_delta = current_busy[engine_class] - previous->busy_ns;
      uint64_t enabled_delta = current_enabled[engine_class] - previous->enabled_ns;
      if (busy_delta <= enabled_delta) {
        utilization->valid[engine_class] = true;
        utilization->percent[engine_class] = usage_percent(busy_delta, enabled_delta);
      }
    }

    previous->busy_ns = current_busy[engine_class];
    previous->enabled_ns = current_enabled[engine_class];
    previous->valid = true;
  }
}

void intel_pmu_close(struct intel_pmu *pmu) {
  for (size_t i = 0; i < pmu->event_count; ++i) {
    ioctl(pmu->events[i].fd, PERF_EVENT_IOC_DISABLE, 0);
    close(pmu->events[i].fd);
  }
  intel_pmu_initialize(pmu);
}
