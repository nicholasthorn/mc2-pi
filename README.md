# MechCommander 2: pi-port

[![CI](https://github.com/nicholasthorn/mc2-pi/actions/workflows/ci.yml/badge.svg?branch=pi-port)](https://github.com/nicholasthorn/mc2-pi/actions/workflows/ci.yml)

A fork of [alariq's open-source MechCommander 2 engine](https://github.com/alariq/mc2)
(Linux and Windows, 64-bit) that adds an **OpenGL ES 3.1 renderer for the
Raspberry Pi 4/400**, an **arm64 build**, and a test and CI setup around the
original code. Desktop OpenGL stays the default everywhere else.

## Relationship to upstream

- All credit for the engine port goes to [alariq](https://github.com/alariq/mc2)
  ([website](https://alariq.github.io/mc2-website/)). Game data comes from
  [alariq/mc2srcdata](https://github.com/alariq/mc2srcdata).
- **This fork is developed with AI assistance.** Upstream's README says
  AI-generated code is not welcome there, so please do not send changes from
  this fork upstream, and report problems with this fork here, not to alariq.
- Upstream `master` is merged in from time to time; changes here are kept small
  and local to make that easy.

## What this fork adds

| Area | Status |
|---|---|
| OpenGL ES 3.1 renderer (`-DMC2_GLES=ON`) for the Pi's V3D GPU | Done |
| Shaders valid as both GLSL ES 3.10 and GLSL 4.20, `#version` chosen at runtime | Done |
| arm64 (aarch64) build: signed `char`, portable heap, GCC 12 fixes | Done |
| Linux build fixes (Ubuntu 22.04 / GCC 11), Clang and MSVC fixes | Done |
| Crash and undefined-behaviour fixes, many back-ported from [mc2-opengl-remastered](https://github.com/ThranduilsRing/mc2-opengl-remastered), more found by sanitizers and cppcheck | Done, ongoing |
| CI: builds, unit tests, shader validation, nightly smoke runs, sanitizers, cppcheck | Done |
| Running on real Pi 4/400 hardware | **Not yet confirmed** (builds and runs on arm64 Linux and under Mesa's software renderer) |

## Platforms

| Platform | Renderer | Built in CI |
|---|---|---|
| Linux x64 (GCC, Clang) | Desktop GL 4.x | Yes |
| Linux arm64 / Raspberry Pi OS 64-bit (GCC) | OpenGL ES 3.1 | Yes |
| Windows x64 (MSVC, VS 2022) | Desktop GL 4.x | Yes |
| WSL2 / no GPU | Either, via Mesa llvmpipe (`LIBGL_ALWAYS_SOFTWARE=1`) | Smoke runs |

## Quick start

Full instructions: [BUILD-LINUX.md](BUILD-LINUX.md), [BUILD-PI.md](BUILD-PI.md),
[BUILD-WIN.md](BUILD-WIN.md).

**1. Build the game** (Linux or Pi; drop `-DMC2_GLES=ON` for desktop GL):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMC2_GLES=ON
cmake --build build
```

**2. Build the game data** from a `mc2srcdata` checkout. The result is platform
independent: build it on a PC and copy it to the Pi.

```bash
git clone https://github.com/alariq/mc2srcdata.git ../mc2srcdata
scripts/build_gamedata.sh --src-data ../mc2srcdata --build-dir build --out ~/mc2exe
```

`--no-movies` skips the movies (the only part stored in Git LFS); missions run
without them.

**3. Run** from the game directory (copy `mc2`, `libmc2res_64.so` and `shaders/`
into it):

```bash
./mc2                      # main menu
./mc2 -mission mc2_01      # straight into a mission
```

## Development

| Check | Command | In CI |
|---|---|---|
| Unit tests (doctest, CTest) | `ctest --test-dir build --output-on-failure` | Every push, all four configurations; also under ASan + UBSan |
| Shader validation (GLSL ES 3.10 + 4.20) | `python3 scripts/validate_shaders.py` | Every push |
| Smoke runs (missions under Xvfb + llvmpipe) | `scripts/smoke.sh --game-dir ~/mc2exe --build-dir build` | Nightly, x64 desktop GL and arm64 GLES; sanitized on x64 |
| Static analysis | `scripts/cppcheck_gate.py` against `scripts/cppcheck-baseline.txt` | Changed files every push, full tree nightly |

- Sanitizer builds: configure with `-DMC2_SANITIZE=address,undefined` (GCC/Clang).
- New source files build with `-Wall -Wextra -Werror` (`MC2_STRICT_WARNINGS`).
- `-autoexit <frames>` quits cleanly after that many gameplay frames (used by
  the smoke runs).
- `MC2_VSYNC=0` turns vsync off (on by default), to measure frame rate past
  the display refresh rate.
- `MC2_SHADER_HOT_RELOAD=1` reloads shaders when their files change (off by
  default: it polls the shader files twice a second).
- Workflows: [ci.yml](.github/workflows/ci.yml),
  [smoke.yml](.github/workflows/smoke.yml),
  [static.yml](.github/workflows/static.yml).

Conventions for contributors and AI agents are in [CLAUDE.md](CLAUDE.md) and
[.claude/rules/](.claude/rules/). Progress and plans live in
[planning/](planning/).

## Known issues

- No networking: the multiplayer screens are stubs (the menu button is disabled).
- Sound: no panning or doppler.
- `-mission <name>` with a mission that does not exist crashes instead of
  reporting an error.
- WSLg's GPU driver shows a white screen with desktop GL; use
  `LIBGL_ALWAYS_SOFTWARE=1`.

The full list is in [planning/PROGRESS.md](planning/PROGRESS.md).

## Plans

- **Confirm and tune on real Pi 4/400 hardware**: which missions run, at what
  frame rate.
- **Quality** ([planning/QUALITY-ROADMAP.md](planning/QUALITY-ROADMAP.md)): more
  unit tests, the cppcheck cleanup, sanitized smoke on arm64, deterministic smoke
  runs, frame-time benchmarks, and a Pi as a self-hosted CI runner.
- **VR tabletop (exploratory)**: playing a mission as a 3D diorama on a virtual
  table in a Meta Quest headset. A feasibility study and phased plan are in
  [planning/future-features/VR-TABLETOP.md](planning/future-features/VR-TABLETOP.md);
  nothing is built yet.

## Repository layout

```
GameOS/      platform layer: SDL window, input, sound, GL / GLES renderer core
mclib/       engine: files and archives, terrain, textures, ABL scripting, effects
code/        game: missions, mechs, AI, UI screens, save/load
gui/         UI widget framework
shaders/     GLSL shared by both renderers
data_tools/  makefst, pak, aseconv, makersp (build the game data)
tests/unit/  doctest unit tests
scripts/     smoke runs, data build, shader validation, cppcheck gate
planning/    progress log, roadmap, future features
```

## Licensing

- The original game is released under the Shared Source Limited Permission
  License (see [EULA.txt](EULA.txt)). Game data is not distributed with this
  repository.
- alariq's code is licensed under GPL v3 (see [license.txt](license.txt)), and
  so are the changes in this fork.
- Third-party libraries use their own licenses.
