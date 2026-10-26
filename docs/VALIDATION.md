# Validation notes

Validation was performed on 2026-10-25. These notes describe the tested environment and evidence; they are not a claim of universal hardware or distribution coverage.

## Test system

- CachyOS, kernel `7.2.7-1-cachyos`
- Intel Core i7-3540M / Intel HD Graphics 4000 (Ivy Bridge mobile GT2)
- PCI `0000:00:02.0`, vendor/device `8086:0166`, i915 driver
- Native Mission Center `1.2.0-1` (not Flatpak)
- Stock NVTOP `3.3.2`
- `kernel.perf_event_paranoid = 2`

## Before the patch

Mission Center repeatedly reported:

```text
GPU '0000:00:02.0' utilization data is missing
```

Stock NVTOP identified the GPU but returned `gpu_util: null`. DRM fdinfo exposed memory fields such as `drm-total-system0`, but no `drm-engine-*` timestamps from which NVTOP could derive process or global utilization.

The kernel did expose i915 PMU events including:

```text
actual-frequency
rcs0-busy
vcs0-busy
bcs0-busy
```

KDE System Monitor's `ksystemstats_intel_helper` had `cap_perfmon=ep` and supplied live readings through that independent path.

## Automated tests

The NVTOP integration passed:

- 9 new PMU tests covering event-name filtering, first-sample baselines, valid idle readings, multiple-engine normalization, independent video/render classes, counter resets, stalled enabled time, invalid busy deltas, rounding, and clamping.
- 12 pre-existing NVTOP interface/options/PCIe tests.

The 9 new tests also passed with AddressSanitizer and UndefinedBehaviorSanitizer. The new module completed GCC `-fanalyzer` and Clang static analysis without findings.

The standalone repository was also built and tested with CMake 3.24.0 and 3.31.8. A library-only build succeeded with tests disabled and no usable C++ compiler configured. Installation followed by `find_package(intel_pmu CONFIG REQUIRED)` was verified with a C-only consumer that built and ran successfully.

GitHub Actions subsequently passed the complete build, 9-test suite, installation, `find_package` consumer, and C-only `add_subdirectory` consumer on Debian 12, Ubuntu 24.04, Fedora 43, Arch Linux, and Alpine 3.22/musl. Alpine required the distribution's `linux-headers` package for `linux/perf_event.h`. These are source-portability checks, not live-hardware validations.

## Build and patch checks

- Built a complete NVTOP binary at the documented upstream base.
- Built a complete binary against Mission Center's pinned NVTOP revision.
- Applied the inner Magpie/NVTOP patch only after Magpie's existing third-party patch stack, not against clean upstream NVTOP.
- Compiled the exact patched Intel sources used by Magpie.
- Built Mission Center 1.2.0 Magpie successfully with `cargo build --release --locked`.
- Rebased and built current Magpie at `17137245f30a3218ce2df0bc3c44831532c5a283`, which embeds NVTOP `71094a9f7dfbfe7cbe9faebfdf055e580a034283`, using the same locked-release build.
- Apply-tested the NVTOP integration patch against its clean documented NVTOP base and each outer Magpie integration patch against its corresponding clean documented Magpie base.
- Parsed the modified `nvtop.json` successfully.

## Live Mission Center verification

The built collector was installed root-owned as `/usr/local/bin/missioncenter-magpie` and granted only `cap_perfmon=ep`. Mission Center started that collector, whose effective capability mask contained bit 38 (`0000004000000000`).

A `GetGpus` request through Mission Center's own Magpie IPC protocol returned a real optional value instead of missing data:

```text
utilization_percent: Some(35.0)
```

A later adjacent-sample comparison produced:

```text
Mission Center: 7.0%
KDE System Monitor: approximately 7.4995%
```

The requests were not synchronized and should not be treated as a precision benchmark. The comparison demonstrates that both applications were observing plausible live load through their respective PMU paths.

## Remaining validation gaps

- Other Intel generations, multiple physical engines, multi-GT devices, suspend/resume, and driver reloads still need coverage.
- Other distributions and Mission Center packaging formats need separate builds and permission-model validation.
- Per-process GPU utilization remains unavailable on the tested Ivy Bridge system because its fdinfo lacks engine timestamps.
- The new tests replay counter samples through pipes; they do not replace privileged testing on every supported hardware configuration.
