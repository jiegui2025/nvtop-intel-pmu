# Attribution and upstream relationship

This is an independent patch and integration repository, not an official NVTOP, Mission Center, KDE, or Intel release.

## Upstream projects

- **NVTOP** — https://github.com/Syllo/nvtop
  GPU monitoring and the downstream collector interfaces extended by the NVTOP patch. NVTOP is GPL-3.0-or-later.
- **Mission Center and Magpie** — https://gitlab.com/mission-center-devs/mission-center
  Mission Center embeds NVTOP through its Magpie collector. The Magpie patch applies that integration to Mission Center 1.2.0. Mission Center and Magpie are GPL-3.0-or-later.
- **KDE KSystemStats** — https://invent.kde.org/plasma/ksystemstats
  Its Intel helper provided an independent reference confirming that i915 PMU engine counters can supply live readings on the tested system. No KDE source files are included in this repository's standalone module.
- **Linux i915 PMU and perf ABI** — Linux kernel documentation and UAPI headers define the events and counter-read semantics used here.

The patch files contain context and derived integration changes from the respective GPL-licensed upstream sources. The standalone module and tests are made available under GPL-3.0-or-later; see `LICENSE`.

Project names remain the property of their respective communities. Inclusion here does not imply review, endorsement, or upstream merge status.
