# Intel i915 PMU utilization for NVTOP and Mission Center

[![tests](https://github.com/jiegui2025/nvtop-intel-pmu/actions/workflows/tests.yml/badge.svg)](https://github.com/jiegui2025/nvtop-intel-pmu/actions/workflows/tests.yml)

Reusable Linux i915 PMU engine-busy collection, plus tested patches for [NVTOP](https://github.com/Syllo/nvtop) and [Mission Center](https://gitlab.com/mission-center-devs/mission-center).

This project addresses GPUs where the driver exposes useful `rcs*-busy`/`vcs*-busy` performance counters but DRM fdinfo has no `drm-engine-*` timestamps. In that situation NVTOP—and applications embedding it—can identify the GPU while reporting utilization as unavailable or 0%.

**Live status:** validated on an Intel HD Graphics 4000 (Ivy Bridge) with Mission Center 1.2.0. Mission Center reported live utilization consistent with KDE System Monitor's independent Intel helper.

## What is included

- `src/intel_pmu.c` and `src/intel_pmu.h`: standalone C module for other monitoring tools.
- `patches/nvtop-intel-pmu-utilization.patch`: NVTOP integration, documentation, and regression tests.
- `patches/magpie-intel-pmu-changes.patch`: Mission Center 1.2.0/Magpie integration.
- `patches/magpie-nvtop-intel-pmu-utilization.patch`: the inner NVTOP patch, provided separately for review and rebasing. It is already embedded in the outer Magpie patch and requires Magpie's existing third-party patch stack; do not apply it directly to a clean NVTOP checkout.
- `tests/intelPmuTests.cpp`: hardware-independent counter replay tests.
- [Roadmap](ROADMAP.md), [installation guide](docs/INSTALL.md), [distribution build dependencies](docs/BUILD_DEPENDENCIES.md), [integration guide](docs/INTEGRATION.md), [compatibility notes](docs/COMPATIBILITY.md), and [validation notes](docs/VALIDATION.md).

## Behavior

- Discovers `rcs<N>-busy` and `vcs<N>-busy` events from `/sys/bus/event_source/devices/<i915-pmu>/events`.
- Reads cumulative busy time and enabled time using `perf_event_open` and `PERF_FORMAT_TOTAL_TIME_ENABLED`.
- Normalizes utilization across multiple engines of the same class.
- Uses the first complete sample as a baseline.
- Rejects counter regressions, stalled enabled-time deltas, and busy deltas larger than enabled time.
- Distinguishes a valid idle reading (0%) from unavailable data.
- Correctly names the integrated i915 PMU `i915`, without a PCI-address suffix.
- Leaves xe and existing fdinfo process accounting unchanged.

The NVTOP integration maps render utilization to overall GPU utilization and video utilization to its shared encode/decode fields. It does **not** invent separate encode/decode measurements or per-process attribution when fdinfo cannot provide them.

## Build and test the reusable module

Requirements: Linux, a C11 compiler, and CMake 3.24+. A C++ compiler is needed only when tests are enabled. If GoogleTest is unavailable, CMake fetches it for the test build.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Other CMake projects can use `add_subdirectory()` and link `intel_pmu::intel_pmu`. After `cmake --install`, `find_package(intel_pmu CONFIG REQUIRED)` is also supported. The module itself has no NVTOP, KDE, or Mission Center dependency.

## Apply the patches

Patch bases used during validation:

- NVTOP: `71094a9f7dfbfe7cbe9faebfdf055e580a034283`
- Mission Center Magpie: `a5272b3c1d853caa4044b737cf49257bfc4c86f2` (Mission Center 1.2.0), with NVTOP `3d4a953da02bc18886734613bb9f60ff80669de7`

```sh
# In an NVTOP checkout
git apply /path/to/nvtop-intel-pmu/patches/nvtop-intel-pmu-utilization.patch

# In the Magpie submodule of a Mission Center 1.2.0 checkout
git apply /path/to/nvtop-intel-pmu/patches/magpie-intel-pmu-changes.patch
```

The separately supplied inner Magpie/NVTOP patch is informational and intended for rebasing; the outer Magpie patch above already contains it. Apply the inner patch only after Magpie's existing third-party patches, not to clean upstream NVTOP.

See [docs/INSTALL.md](docs/INSTALL.md) for build, installation, verification, and rollback instructions.

## Permission and security requirements

System-wide i915 PMU access normally requires `CAP_PERFMON` or `CAP_SYS_ADMIN`. Without that access the module returns no utilization reading; it does not fabricate 0%.

Grant capabilities only to a **root-owned binary whose file and parent directories are not writable by an unprivileged user**. Do not set capabilities on a build tree or other user-writable executable. A small dedicated privileged helper is preferable for long-term distribution integration.

## Scope and limitations

- Linux i915 only; xe is unchanged.
- Requires matching PMU events to be exposed by the running kernel/driver.
- PMU collection provides device-wide counters, not per-process usage.
- Render and video classes are covered; copy, video-enhance, and compute classes remain outside this patch's scope.
- Shared video utilization does not distinguish encoding from decoding.
- These patches have not yet been merged upstream. Test other GPU generations and multi-engine systems before claiming broad support.

## Contributing

Useful contributions include tests on additional Intel generations, multiple render/video engines, suspend/resume and driver-reload behavior, and packaging that uses a narrowly scoped helper.

For hardware reports, include the GPU generation, kernel and driver, relevant PMU event names, and whether fdinfo contains `drm-engine-*`. Avoid publishing authentication tokens, complete process lists, or other unrelated system logs.

## License and attribution

GPL-3.0-or-later. NVTOP and Mission Center/Magpie are also GPL-3.0-or-later projects. See [NOTICE.md](NOTICE.md) for upstream references and [LICENSE](LICENSE) for the license text.
