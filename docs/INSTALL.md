# Build, installation, verification, and rollback

## Prerequisites

Use the normal build dependencies for the project being patched. In addition to CMake and a C/C++ toolchain, NVTOP needs its device-discovery and UI dependencies. Magpie needs Rust/Cargo, CMake, pkg-config, and its libdrm/libudev/GBM and platform dependencies.

Check that the kernel exposes compatible counters before proceeding:

```sh
find /sys/bus/event_source/devices -maxdepth 3 -type f \
  -path '*i915*/events/*-busy' -print
```

A supported i915 device should have `rcs<N>-busy` for render utilization and may have `vcs<N>-busy` for shared video utilization. No compatible events means this patch cannot provide those readings.

## NVTOP

The supplied patch was validated against commit `71094a9f7dfbfe7cbe9faebfdf055e580a034283`. Applying it to a nearby revision may also work; always run `git apply --check` first.

```sh
git clone https://github.com/Syllo/nvtop.git
cd nvtop
git switch -c intel-pmu 71094a9f7dfbfe7cbe9faebfdf055e580a034283
git apply --check /path/to/nvtop-intel-pmu/patches/nvtop-intel-pmu-utilization.patch
git apply /path/to/nvtop-intel-pmu/patches/nvtop-intel-pmu-utilization.patch

cmake -B build -DINTEL_SUPPORT=ON -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Install a separate, root-owned executable rather than setting capabilities on the user-writable build output:

```sh
sudo install -m 0755 -o root -g root \
  build/src/nvtop /usr/local/bin/nvtop-intel-pmu
sudo setcap cap_perfmon=ep /usr/local/bin/nvtop-intel-pmu
/usr/local/bin/nvtop-intel-pmu
```

For non-interactive verification, allow at least two samples so the first can establish a baseline:

```sh
/usr/local/bin/nvtop-intel-pmu --snapshot --loop --delay 10 --no-processes
```

## Mission Center 1.2.0 / Magpie

Clone Mission Center and initialize its nested submodules. Then apply the Magpie patch inside the Magpie submodule—not in Mission Center's top-level repository.

```sh
git clone --branch v1.2.0 --recurse-submodules \
  https://gitlab.com/mission-center-devs/mission-center.git
cd mission-center/subprojects/magpie
git submodule update --init --recursive

git apply --check /path/to/nvtop-intel-pmu/patches/magpie-intel-pmu-changes.patch
git apply /path/to/nvtop-intel-pmu/patches/magpie-intel-pmu-changes.patch
cargo build --release --locked
```

The validated Magpie base is `a5272b3c1d853caa4044b737cf49257bfc4c86f2`, containing NVTOP `3d4a953da02bc18886734613bb9f60ff80669de7`.

On distributions where `/usr/local/bin` precedes `/usr/bin` in `PATH`, a local collector can be installed without replacing the package-owned collector:

```sh
sudo install -m 0755 -o root -g root \
  target/release/magpie /usr/local/bin/missioncenter-magpie
sudo setcap cap_perfmon=ep /usr/local/bin/missioncenter-magpie

# Close and reopen Mission Center after verifying these paths.
command -v missioncenter-magpie
getcap /usr/local/bin/missioncenter-magpie
```

Expected results:

```text
/usr/local/bin/missioncenter-magpie
/usr/local/bin/missioncenter-magpie cap_perfmon=ep
```

Close every running Mission Center instance and launch it again. The main GPU graph should begin updating after the baseline sample. If `command -v` returns `/usr/bin/missioncenter-magpie`, fix the session's `PATH` ordering rather than assuming the patched collector is running.

## Verify the live collector

`CAP_PERFMON` is bit 38. The running collector should have that bit in `CapEff`:

```sh
pgrep -af 'missioncenter-magpie --addr'
grep -E '^(CapPrm|CapEff):' /proc/COLLECTOR_PID/status
```

`0000004000000000` indicates `CAP_PERFMON` is effective. The UI and a few minutes of logs are the best checks: `GPU '<PCI ID>' utilization data is missing` should stop, and utilization should track real GPU workloads.

Compare against another tool as a sanity check, but do not expect adjacent samples to be numerically identical.

## Rollback

Close Mission Center, then remove the local override:

```sh
sudo rm -f /usr/local/bin/missioncenter-magpie
hash -r
command -v missioncenter-magpie
```

The last command should again return `/usr/bin/missioncenter-magpie`. The distribution package was not modified by the `/usr/local` installation.

For the optional NVTOP executable:

```sh
sudo rm -f /usr/local/bin/nvtop-intel-pmu
```

## Updates and maintenance

Files in `/usr/local/bin` normally survive distribution package updates. That avoids silently losing the patch, but a Mission Center update can make an old collector incompatible. Rebuild the patch against the matching Magpie revision after upgrades, or remove the local override and retest the stock version.

Capabilities are preserved only while the same installed file remains in place. Replacing or reinstalling a binary requires reviewing and deliberately reapplying `setcap`.
