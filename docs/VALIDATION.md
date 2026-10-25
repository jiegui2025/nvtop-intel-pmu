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

## Build and patch checks

- Built a complete NVTOP binary at the documented upstream base.
- Built a complete binary against Mission Center's pinned NVTOP revision.
- Applied the Magpie inner patch after its existing third-party patch stack.
- Compiled the exact patched Intel sources used by Magpie.
- Built Magpie 1.2.0 successfully with `cargo build --release --locked`.
- Apply-tested both exported patch files against clean checkouts.
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
