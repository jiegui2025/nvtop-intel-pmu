# Build dependencies by distribution

These package lists match the standalone module's distribution CI jobs. They cover compilation, tests, installation, and the C-only consumers. They do **not** establish live GPU support and are not complete dependency lists for downstream NVTOP or Mission Center builds.

| Distribution | Packages for the standalone module and tests |
|---|---|
| Debian 12 / Ubuntu 24.04 | `build-essential cmake ninja-build git ca-certificates` |
| Fedora 43 | `gcc gcc-c++ cmake ninja-build git ca-certificates` |
| Arch Linux / CachyOS | `base-devel cmake ninja git ca-certificates` |

For Debian/Ubuntu, install with:

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  build-essential cmake ninja-build git ca-certificates
```

For Fedora:

```sh
sudo dnf install -y gcc gcc-c++ cmake ninja-build git ca-certificates
```

For Arch/CachyOS:

```sh
sudo pacman -S --needed base-devel cmake ninja git ca-certificates
```

Use the distribution's NVTOP and Mission Center/Magpie build documentation for their additional UI, device-discovery, Rust, GBM, libdrm, libudev, and platform requirements. Downstream package names and required versions can differ from the standalone module.

The CI also downloads GoogleTest when it is not installed. Offline builds need GoogleTest provided in advance or tests disabled with `-DBUILD_TESTING=OFF`; a library-only build then requires no C++ compiler.
