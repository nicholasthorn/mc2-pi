# Releases

Status: phases 1 and 2 built (2026-11-01, branch `release/prep`): packages
from `scripts/package.sh`, `.github/workflows/release.yml` making a draft
release from a `v*` tag. Phases 3 onwards (AppImages, package smoke tests,
the first release candidate) are still to do. This is the plan for
downloadable builds on GitHub Releases. Progress is tracked in `PROGRESS.md`.

## Goals

Two audiences, one release:

1. **Constrained systems, first-class:** the Raspberry Pi 4 and Pi 400 (4 GB,
   shared with the GPU; V3D GPU with OpenGL ES 3.1) on Raspberry Pi OS. It is
   the hardest target and where the fork started, so the Pi package gets the
   most care: small, tuned defaults, tested on hardware before a release is
   called supported.
2. **Everyone else:** ready-to-run builds for ordinary Linux and Windows PCs, so
   anyone can play without compiling anything.

Game data is never included (original Microsoft EULA). Every package ships the
data tools so players can build the data themselves without a compiler (see
"Game data" below).

## Packages

| Package | For | Renderer | Built on | Format |
|---|---|---|---|---|
| `mc2-<ver>-rpi-arm64.tar.gz` | Pi 4 / 400 (and Pi 5) on Raspberry Pi OS 64-bit (bookworm) | GLES 3.1 | `debian:bookworm` container on the arm64 runner | Tarball; uses the system SDL2 packages (`apt` line in the README) |
| `mc2-<ver>-linux-x86_64.AppImage` | Most Linux PCs | Desktop GL | Oldest supported base (Ubuntu 22.04, glibc 2.35) | AppImage bundling SDL2, SDL2_mixer, SDL2_ttf, GLEW, zlib |
| `mc2-<ver>-linux-arm64.AppImage` | Other arm64 Linux machines (SBCs, ARM laptops) | GLES 3.1 | `debian:bookworm` on the arm64 runner | AppImage, as above |
| `mc2-<ver>-windows-x64.zip` | Windows 10 / 11 | Desktop GL | `windows-2022`, MSVC | Zip: unpack and run |
| `mc2-<ver>-debug-symbols.tar.gz` | Crash reports | | All of the above | Separate, so the game packages stay small |
| `SHA256SUMS` | Checking downloads | | | |

Every game package contains: the game executable, the resource library
(`libmc2res_64.so` / `mc2res.dll`), `shaders/`, the data tools in `tools/`,
`scripts/build_gamedata.sh`, an install note, `license.txt` (GPL v3), and the
third-party licenses for anything bundled.

### Raspberry Pi package (constrained target)

- **Matches Raspberry Pi OS exactly.** Built in a `debian:bookworm` arm64
  container, so glibc and SDL versions are the ones the Pi installs with `apt`.
  A binary built on `ubuntu-24.04-arm` (as CI does today) would need a newer
  glibc than Raspberry Pi OS has.
- **Lean.** Stripped binaries (symbols in the separate archive), system
  libraries instead of bundled ones, no upscaled textures.
- **Tuned for the Pi's CPU** without excluding other arm64 chips: generic
  ARMv8-A code with `-mtune=cortex-a72` (Pi 4/400). Measure before going
  further (`-mcpu` would tie it to one core type).
- **Low settings by default.** The game's first launch runs a hardware "sniffer"
  to pick settings; check what it picks on the Pi, and ship a low-detail
  `options.cfg` template if it picks too high.
- **Requirements to state in the release notes:** Raspberry Pi OS 64-bit
  (bookworm or later) with the desktop, the default KMS graphics driver
  (Mesa V3D), and the RAM figure measured in testing. 32-bit Raspberry Pi OS is
  not supported at first (see open questions).
- **Hardware check before calling it supported:** until a Pi runs in CI
  (`QUALITY-ROADMAP.md` Phase 7), each release gets a manual run on a Pi 400:
  main menu, two missions to completion, frame rate noted in the release notes.

### General Linux and Windows packages (everyone)

- **Linux AppImage:** one file that runs on most distributions without
  installing anything. Built with `linuxdeploy` on the oldest base we support,
  so it also runs on newer ones. Desktop GL (any GPU with OpenGL 4.0 or later).
- **Linux arm64 AppImage:** the GLES build for arm64 machines other than the Pi.
  Built in CI and smoke-tested, but not hardware-tested each release.
- **Windows zip:** `mc2.exe`, `mc2res.dll`, `shaders/`, the SDL2 / SDL2_mixer /
  SDL2_ttf / GLEW / zlib DLLs from `3rdparty.zip`, and the Microsoft C++
  runtime DLLs (`vcruntime140.dll`, `vcruntime140_1.dll`, `msvcp140.dll`) next
  to the executable, so nothing needs installing. Not code-signed, so Windows
  SmartScreen will warn on first run; the release notes say how to proceed.

## Support levels

| Level | Platforms | Meaning |
|---|---|---|
| Tested | Pi 4/400 on Raspberry Pi OS 64-bit; Windows 10/11 x64; Linux x64 AppImage on Ubuntu LTS | Run by hand before each release |
| Built | Linux arm64 AppImage; other Linux distributions | Built and smoke-tested in CI only |

The Pi moves to "Tested" once a release has been run on real hardware; until
then the release notes say it is untested on hardware.

## Game data

- Players build the data from [alariq/mc2srcdata](https://github.com/alariq/mc2srcdata)
  once; the result is platform independent, so it can be built on a PC and
  copied to a Pi.
- `scripts/build_gamedata.sh` needs a `--tools-dir` option so it can use the
  packaged `tools/` instead of a CMake build tree.
- It is a bash + GNU make pipeline. Windows players need WSL or MSYS2, or can
  build the data on any Linux machine (including a Pi) and copy it over. A
  Windows-native data build is a possible later improvement.

## How a release is made

1. `.github/workflows/release.yml` runs on `push: tags: ['v*']`:
   - one job per package, each running the unit tests before packaging;
   - a **package smoke test**: unpack the Linux packages and run
     `scripts/smoke.sh` on them with the cached game data from `smoke.yml`
     (tests what users download, not the build tree);
   - a final job that writes `SHA256SUMS` and creates a **draft** release with
     generated notes (`gh release create`, `permissions: contents: write`).
2. Review the draft, run the hardware checks above, add the Pi results to the
   notes, publish.
3. Versions: `vMAJOR.MINOR.PATCH`. Minor for features, patch for fixes;
   release candidates as `v0.1.0-rc1` (marked pre-release).

## Licensing checklist

- No game data, art or movies in any package.
- `license.txt` (GPL v3); the source for each release is the tagged commit,
  which GitHub attaches automatically.
- Third-party licenses for bundled libraries: SDL2, SDL2_mixer (and the codec
  libraries its Windows DLLs include), SDL2_ttf (FreeType), GLEW, zlib,
  pl_mpeg; and Microsoft's redistribution terms for the C++ runtime DLLs.

## Phases

| Phase | What | Exit criterion |
|---|---|---|
| 1. Packaging scripts | `scripts/package.sh` (Linux) and a Windows equivalent; `--tools-dir` for `build_gamedata.sh`; install notes; license bundle | A package built locally runs on a clean machine |
| 2. Release workflow | `release.yml` with the Pi tarball, Windows zip and a plain Linux x64 tarball; draft release | A test tag produces a draft with all three |

### As built (phases 1 and 2)

- `scripts/package.sh --build-dir DIR --platform rpi-arm64|linux-x86_64|windows-x64
  --version VER` stages a game directory without data and archives it, one
  script for all three (bash runs on the Windows runner too). Binaries are
  stripped; their debug information goes in `<package>-symbols.tar.gz`
  (Linux `.debug` files with a debuglink, Windows `.pdb`).
- The package is the game folder: `mc2` (or `mc2.exe`), the resource
  library, `shaders/`, `license.txt`, `licenses/`, `INSTALL.md` (from
  `dist/`), `VERSION`. Linux and Pi packages add `tools/` and
  `build_gamedata.sh`, which uses that `tools/` when given neither
  `--build-dir` nor `--tools-dir`, so `./build_gamedata.sh --src-data ...
  --out .` fills the unpacked package with data.
- **Windows has no data tools,** a change from the plan above: the data build
  is bash and make, so Windows players build the data with a Linux package
  (WSL, a Linux PC or the Pi) and copy it over. The zip bundles the five
  `3rdparty` DLLs and the MSVC runtime from the runner's Visual Studio.
- Third-party licences are in `dist/licenses/`, fetched from upstream at the
  versions shipped (SDL 2.30.11, SDL_mixer 2.8.0 with its built-in Timidity,
  minimp3, dr_flac and stb_vorbis, SDL_ttf 2.24.0 with FreeType and
  HarfBuzz, GLEW 2.2.0, zlib); pl_mpeg and stb are compiled into every build.
- `release.yml`: the Pi package in `debian:bookworm` on the arm64 runner with
  `-mtune=cortex-a72`, Linux x64 on `ubuntu-22.04` (glibc 2.35), Windows on
  `windows-2022`; unit tests in each; a tag creates a draft release with
  `SHA256SUMS`, a pre-release for tags with a hyphen. `workflow_dispatch`
  and pull requests that touch the packaging are dry runs (artifacts only).
- **`zlib1.dll` in `3rdparty.zip` is zlib 1.2.3 (2005)**, with known security
  fixes since; update it before the first release.
| 3. AppImages | Linux x64 and arm64 AppImages on old bases | AppImage runs on Ubuntu 22.04 and a current Fedora |
| 4. Package smoke tests | Smoke runs against the unpacked packages in `release.yml` | Release fails if a package does not start a mission |
| 5. First release candidate | `v0.1.0-rc1`, pre-release | Downloaded and run on Windows, Linux x64 and a Pi 400 |
| 6. First release | `v0.1.0` | Pi hardware results in the notes |

## Open questions

- 32-bit Raspberry Pi OS (armhf): still common on older installs. Worth a
  package? It needs a CI build and a check of the 32-bit code paths.
- A `.deb` for the Pi instead of (or as well as) the tarball?
- Pi 5: should work (same GLES 3.1 level); test when hardware is available.
- Minimum RAM: does a 2 GB Pi 4 run the game? Measure.
- Flatpak or a distribution package for Linux, later?
- Code signing for Windows: costs money; only if SmartScreen warnings become a
  real problem.
