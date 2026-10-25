# Compatibility and portability

## Short answer

The standalone collector contains no distribution-specific package logic, but **universal support across distributions is not claimed**. A source-compatible system still needs the right kernel interface, driver, permissions, and downstream source revision.

Only the environment listed in [VALIDATION.md](VALIDATION.md) has received an end-to-end test so far.

## Requirements

All of these must hold for live PMU readings:

1. Linux with an Intel GPU controlled by the i915 driver.
2. A PMU directory exposing compatible `rcs<N>-busy` and/or `vcs<N>-busy` events.
3. `perf_event_open` support with `PERF_FORMAT_TOTAL_TIME_ENABLED`.
4. Permission to open the system-wide events, normally `CAP_PERFMON` or `CAP_SYS_ADMIN`.
5. For NVTOP or Mission Center, a source revision to which the relevant patch applies or has been rebased.

Check the running kernel, not just the distribution release:

```sh
find /sys/bus/event_source/devices -maxdepth 3 -type f \
  -path '*i915*/events/*-busy' -print
```

## Environment matrix

| Environment | Status | Reason / required work |
|---|---|---|
| Native Linux installation with compatible i915 PMU events | Expected to work; needs testing per system | Build the matching patch and grant the collector controlled PMU access |
| CachyOS, native Mission Center 1.2.0, Intel HD 4000 | Validated | Live Mission Center readings matched the independent KDE path in adjacent samples |
| Other native distributions (Debian/Ubuntu, Fedora, openSUSE, etc.) | Portable in principle; currently untested | Package names and build installation differ, but the collector uses standard Linux interfaces |
| Another Mission Center or Magpie revision | Rebase may be required | The supplied Magpie patch has an exact validated base and embedded NVTOP revision |
| Another NVTOP revision | Rebase may be required | Review changes to Intel collection, build files, and tests before applying |
| Flatpak Mission Center | Not covered by the native installation instructions | The bundled host collector and sandbox/host boundary must be patched and packaged appropriately |
| Snap or AppImage Mission Center | Not covered by the native installation instructions | Confinement and bundled glibc/musl collector variants need separate validation |
| Containers | Not enabled by default | Requires deliberate host sysfs access and performance-monitoring privileges; review security implications |
| xe-driver Intel GPUs | Not changed by this patch | The new PMU path is restricted to i915 |
| AMD, NVIDIA, or non-Linux systems | Not supported by this module | Different kernel and vendor interfaces are required |
| i915 without compatible PMU busy events | No utilization fallback available | The module returns invalid/missing readings rather than fabricating load |

## Distribution-specific packaging concerns

The reusable C code uses standard Linux headers and syscalls and should be adaptable to glibc or musl builds. That does not make one prebuilt binary portable across distributions. Build for the target ABI and downstream source revision.

Native `/usr/local/bin` precedence is common but not universal. Verify which collector the application actually starts. Capability handling, init system, sandbox policy, and package updates also differ between installations.

For broad distribution, the safer long-term design is a small, reviewed host-side helper with a narrow protocol and `CAP_PERFMON`, packaged for each supported sandbox/confinement model. Granting a capability to a large application-specific collector is a local workaround, not a universal packaging solution.

## Hardware coverage

The implementation has no GPU-generation allowlist. It probes numbered `rcs` and `vcs` events and normalizes multiple engines, so another older i915 GPU can use the same path **when its running kernel exposes compatible counters**. This is a capability check, not a guarantee for Sandy Bridge, Ivy Bridge, Haswell, or any other specific generation.

Only Intel HD Graphics 4000 (Ivy Bridge) has been validated end to end. Other old Intel GPUs may lack the required PMU events or expose different behavior. No matching `rcs<N>-busy` event means the patch cannot provide render utilization; no `vcs<N>-busy` event means it cannot provide shared video utilization.

Check the actual system with:

```sh
find /sys/bus/event_source/devices -maxdepth 3 -type f \
  \( -name 'rcs*-busy' -o -name 'vcs*-busy' \) -print
```

Even when events exist, verify readings under a real GPU workload and compare them with an independent tool. Reports for additional hardware should include successful **and** unsuccessful results, with GPU generation, kernel/driver details, and available event names. Multi-GT systems also remain untested.
