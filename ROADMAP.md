# Roadmap

This roadmap separates verified support from intended work. Completing a build or test job does not by itself establish live GPU compatibility.

## Phase 0 — Core collector and proof of concept (completed)

- Standalone i915 PMU render/video collector with multi-engine normalization and counter validation.
- NVTOP integration and regression tests.
- Mission Center 1.2.0/Magpie integration.
- Live Ivy Bridge / Intel HD 4000 validation against KDE System Monitor.
- Root-owned `/usr/local` installation and rollback documentation.
- Standalone CMake build, C-only consumers, and `find_package` support.

## Phase 1 — Portability and reproducibility (completed)

- [x] Test CMake 3.24.0 and 3.31.8 locally and in GitHub Actions.
- [x] Test an installed C-only `find_package` consumer.
- [x] Run build, unit-test, `add_subdirectory`, installation, and `find_package` checks on Debian, Ubuntu, Fedora, and Arch containers.
- [x] Add automated clean-base apply checks for the exported NVTOP and Magpie patches.
- [x] Document distro-specific build dependencies without turning them into hardware-support claims.
- [x] Add a musl build/test lane if the module and test dependencies support it.

**Exit criteria:** repeatable source builds and consumer checks across the documented distribution matrix, with failures and limitations recorded accurately.

## Phase 2 — Upstream review (active)

- [x] Draft an upstream review packet with the PMU rationale, security model, tests, and hardware evidence.
- [x] Rebase onto the documented current NVTOP and Magpie revisions and validate clean current-Magpie builds.
- [ ] Recheck upstream heads and submit an upstream NVTOP issue/PR after submission review.
- [ ] Discuss the Magpie integration and capability boundary with Mission Center maintainers.
- [ ] Resolve review feedback and keep the standalone patches rebased.
- [ ] Determine which functionality belongs in NVTOP and which belongs in downstream packaging.

**Exit criteria:** an explicit upstream decision and a maintainable downstream integration path. No upstream merge is implied before this happens.

## Phase 3 — Hardware validation

Test real render/video workloads where available, plus idle readings and counter-edge behavior.

- [ ] Sandy Bridge.
- [ ] Ivy Bridge (initial HD 4000 case already validated; expand devices and kernels).
- [ ] Haswell.
- [ ] Broadwell and newer i915 generations where the fallback is relevant.
- [ ] Multiple render/video engines and multi-GT systems.
- [ ] Suspend/resume, driver reload, counter reset, and permission-loss behavior.

**Exit criteria:** a public results matrix with GPU generation, kernel/driver, available PMU events, test method, and successful or unsuccessful outcome. Missing hardware remains explicitly untested.

## Phase 4 — Least-privilege and packaging

- [ ] Design a small host-side helper with a narrow read-only protocol and `CAP_PERFMON`.
- [ ] Restrict helper input to expected PMU names and event classes; do not expose arbitrary perf configuration or shell execution.
- [ ] Define lifecycle, restart, error, and privilege-revocation behavior.
- [ ] Evaluate native, Flatpak, Snap, and AppImage integration separately.
- [ ] Avoid granting capabilities to user-writable binaries or silently lowering system perf security settings.

**Exit criteria:** reviewed packaging that does not require granting the entire application collector a capability, with a documented threat model for each packaging format.

## Phase 5 — Release and maintenance

- [ ] Publish versioned release notes after Phases 1 and 2 settle the source contract.
- [ ] Track downstream base revisions and rebase failures.
- [ ] Keep security, compatibility, and rollback documentation synchronized with code changes.
- [ ] Add regression coverage for every reproduced collector defect.

## Non-goals and constraints

- No claim of universal distro, Intel-generation, or sandbox support.
- No fabricated per-process utilization when legacy fdinfo lacks engine timestamps.
- No artificial split of shared video utilization into independent encode and decode measurements.
- No AMD, NVIDIA, xe, or non-Linux backend in this project unless explicitly scoped later.
- No weakening of `perf_event_paranoid`, broad root services, or capabilities on unprivileged-user-writable files as a convenience workaround.
