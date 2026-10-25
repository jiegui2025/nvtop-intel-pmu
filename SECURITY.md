# Security notes

## Capability boundary

The PMU collector requires elevated performance-monitoring access on typical systems. `CAP_PERFMON` is narrower than root but can still expose sensitive performance information and increase side-channel attack surface.

- Grant capabilities only to reviewed, root-owned executables whose files and parent directories are not writable by unprivileged users.
- Never set capabilities on a binary in a user-writable build tree.
- Do not run an unreviewed user-writable build as root.
- Reapply capabilities deliberately after replacing an executable; do not automate this blindly in package scripts.
- Prefer a small privileged helper with a narrow interface for production packaging.
- Review Flatpak, Snap, AppImage, and container models separately; native installation guidance does not establish a safe sandbox boundary for them.

## Reporting a vulnerability

Please use the repository's private GitHub vulnerability-reporting mechanism when available. Include the affected commit, impact, and reproduction details, but do not include authentication tokens or unrelated process/system logs.
