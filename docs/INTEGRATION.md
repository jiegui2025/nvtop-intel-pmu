# Integrating the standalone PMU module

`src/intel_pmu.c` is independent of NVTOP and Mission Center. It uses only Linux/libc interfaces and can be vendored or built with CMake.

## CMake

```cmake
add_subdirectory(path/to/nvtop-intel-pmu)
target_link_libraries(your_monitor PRIVATE intel_pmu::intel_pmu)
```

For a direct source build, compile `src/intel_pmu.c` with `_GNU_SOURCE` defined and expose `src/intel_pmu.h` to the caller.

## API sequence

```c
#include "intel_pmu.h"

struct intel_pmu pmu;
intel_pmu_initialize(&pmu);

/* For the conventional integrated GPU at 0000:00:02.0. */
bool opened = intel_pmu_open(&pmu, "i915");

for (;;) {
    struct intel_pmu_utilization utilization;
    intel_pmu_read(&pmu, &utilization);

    if (utilization.valid[intel_pmu_engine_render]) {
        unsigned render_percent = utilization.percent[intel_pmu_engine_render];
        /* Publish device-wide render utilization. */
    }

    if (utilization.valid[intel_pmu_engine_video]) {
        unsigned video_percent = utilization.percent[intel_pmu_engine_video];
        /* Publish shared video utilization. */
    }

    /* Wait until the next desired sample. */
}

intel_pmu_close(&pmu);
```

Call `intel_pmu_initialize()` before the first open. `intel_pmu_open()` discovers events and enables their perf descriptors. A `false` return means no compatible event could be opened; reads remain safe but produce invalid readings.

The first complete read establishes a baseline. Subsequent reads calculate deltas, so the sampling interval does not need to be constant. Do not reset or reinitialize the structure between samples.

## PMU names

The conventional integrated GPU at `0000:00:02.0` normally uses:

```text
i915
```

Other i915 PCI devices normally append the PCI address, replacing colons with underscores while retaining the dot:

```text
i915_0000_01_00.0
```

The corresponding sysfs directory is `/sys/bus/event_source/devices/<pmu-name>`. Check the running system rather than hard-coding assumptions for every device.

## Calculation

For each engine class, the module sums all current busy counters and all current enabled-time counters. Given cumulative sums `B0/E0` at the baseline and `B1/E1` at the next sample:

```text
utilization = round(100 * (B1 - B0) / (E1 - E0))
```

Summing enabled time as well as busy time normalizes the result across multiple engines. A result is emitted only when time advances and the deltas are coherent. Percentages are integers from 0 to 100.

The two public classes are:

- `intel_pmu_engine_render`: `rcs<N>-busy` events.
- `intel_pmu_engine_video`: `vcs<N>-busy` events, representing shared video work rather than independent encode and decode counters.

## Combining with fdinfo

PMU data is device-wide. fdinfo engine timestamps can provide per-client data on supported systems. Do not add the PMU total to per-process totals as though they were independent workloads; that would count activity twice.

NVTOP keeps its existing process logic and reconciles its global reading without changing xe behavior. Other applications should decide explicitly whether each displayed field represents device-wide or per-client activity.

When fdinfo has no engine timestamps, per-process utilization remains unknown. Memory fields in fdinfo do not imply that engine-time fields are present.

## Permissions and lifecycle

Opening system-wide PMU events normally fails without `CAP_PERFMON` or `CAP_SYS_ADMIN`. This is expected and is not equivalent to an idle GPU.

The module tracks and closes every opened descriptor. `intel_pmu_close()` also clears the baseline. Reopening rediscovers events and starts a new baseline.

The API is not internally synchronized. Use one thread per `struct intel_pmu`, or provide external locking. Give independent GPUs separate structures.

For production packaging, prefer a small helper with a restricted interface and `CAP_PERFMON` over granting capabilities to a large application. Never install a capability-bearing executable in a location writable by an unprivileged user.
