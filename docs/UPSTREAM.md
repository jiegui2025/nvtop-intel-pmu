# Upstream review packet

This document prepares, but does not submit, upstream reviews. No upstream issue or pull request is implied by this file.

## NVTOP proposal

**Draft title:** `intel: add i915 PMU render and shared video utilization`

### Problem

Intel utilization is currently derived from DRM fdinfo engine timestamps and per-process deltas. Some i915 devices/kernels expose memory fields but no `drm-engine-*` timestamps, leaving global and per-process utilization unavailable. The tested Intel HD Graphics 4000 case produced `gpu_util: null` while the kernel exposed usable i915 PMU busy events.

### Proposed change

Add an i915 PMU collector that:

- discovers numbered `rcs<N>-busy` and `vcs<N>-busy` events;
- reads cumulative busy and enabled time with `PERF_FORMAT_TOTAL_TIME_ENABLED`;
- normalizes across multiple engines of the same class;
- establishes a baseline on the first complete read;
- rejects counter regressions, stalled enabled time, and impossible busy deltas;
- maps render time to overall GPU utilization and video time to shared encode/decode fields;
- uses `i915` for the conventional integrated PMU and PCI-suffixed names for other i915 devices;
- leaves xe and existing fdinfo process accounting unchanged.

System-wide PMU opens normally require `CAP_PERFMON` or `CAP_SYS_ADMIN`. Failure to open them leaves utilization unavailable rather than converting it to a valid 0%.

### Evidence

- Intel HD Graphics 4000 / Ivy Bridge, i915, native Mission Center 1.2.0.
- Before: repeated `utilization data is missing` and NVTOP `gpu_util: null`.
- After: live Magpie `utilization_percent` values consistent with KDE's independent Intel PMU helper in adjacent samples.
- 9 new counter-replay tests plus 12 existing NVTOP tests passed.
- New tests passed ASan/UBSan; GCC and Clang static analysis were clean.
- Standalone build/tests and C-only consumers passed on Debian 12, Ubuntu 24.04, Fedora 43, Arch, and Alpine/musl.

### Review questions

1. Should PMU data be used whenever available, or only after fdinfo engine-time support is known to be absent?
2. Is the existing reconciliation with process-derived global utilization the preferred policy?
3. Is setting both shared encode/decode fields from `vcs` consistent with NVTOP's `encode_decode_shared` contract?
4. Are the intentionally excluded copy, video-enhance, and compute classes acceptable as follow-up scope?
5. What capability/documentation approach is preferred for packaged NVTOP binaries?
6. Are there preferred helpers or abstractions for perf-based collectors and their tests?

Patch: `patches/nvtop-intel-pmu-utilization.patch`.

## Mission Center / Magpie proposal

**Draft title:** `Support i915 utilization when fdinfo engine timestamps are absent`

### Summary

Mission Center 1.2.0 identifies the tested GPU but reports 0% while repeatedly warning that utilization data is missing. The Magpie patch updates its embedded NVTOP source with the i915 PMU collector. A root-owned local collector with `CAP_PERFMON` then returns live GPU utilization through the existing `GetGpus` IPC response without UI changes.

### Integration concerns

- The local installation is a workaround, not a recommended universal package design.
- Granting `CAP_PERFMON` to the complete Magpie collector increases its privilege and should be reviewed.
- Flatpak, Snap, AppImage, glibc, and musl distribution paths need separate treatment.
- A small host-side helper with a restricted protocol is preferable for long-term packaging.
- Legacy fdinfo still cannot provide per-process utilization; this should remain unknown rather than fabricated.

### Review questions

1. Would maintainers accept an updated embedded NVTOP with this collector?
2. Which packaging formats can use a dedicated host helper, and what protocol/lifecycle conventions are preferred?
3. Should unsupported per-process fields be hidden or presented differently on legacy hardware?
4. What live hardware matrix is required before removing or revising the documented Intel limitation?

Patches: `patches/magpie-intel-pmu-changes.patch` for Mission Center 1.2.0 and `patches/magpie-main-intel-pmu-changes.patch` for current Magpie. The corresponding inner patches are provided separately for review and rebasing.

The current-Magpie patch targets `17137245f30a3218ce2df0bc3c44831532c5a283` and was validated with a clean `cargo build --release --locked`. NVTOP upstream remained at the documented `71094a9f7dfbfe7cbe9faebfdf055e580a034283` base when checked.

## Submission checklist

- [x] Rebase onto the documented current NVTOP and Magpie revisions and add clean-base patch-apply checks.
- [x] Run the complete NVTOP tests and clean Mission Center 1.2.0 and current-Magpie builds after rebasing.
- [ ] Recheck upstream heads and repeat the relevant checks immediately before submission.
- [ ] Confirm the exported patches and standalone `intel_pmu.c`/`.h` remain synchronized.
- [ ] Review upstream contributor, sign-off, DCO/CLA, commit-message, and code-format requirements.
- [ ] Include tested and untested hardware explicitly; do not claim universal distro support.
- [ ] Explain the capability threat model and why no system perf setting is weakened.
- [ ] Decide whether to open a design issue first or submit the implementation PR directly.
- [ ] Link the upstream review back to this repository without describing it as an upstream endorsement.
