# MC2 pi-port: progress

Single source of truth for what is built, what is in progress and what is next.
Update at the end of every task: add a line to the log, and edit "Current state"
if what is built has changed. Keep the log to one or two lines per entry; detail
belongs in commit messages and PRs.

## Current state

**Built**

- [x] Linux build on Ubuntu 22.04 / GCC 11 (pkg-config fallbacks for SDL2_mixer/ttf)
- [x] OpenGL ES 3.1 renderer behind `-DMC2_GLES=ON` (Raspberry Pi 4/400); desktop GL unchanged
- [x] GLES texture upload repacking for 8-bit channel-count mismatches
- [x] arm64: `-fsigned-char`, portable heap caller capture, GCC 12 fixes
- [x] Clang and MSVC build fixes
- [x] Back-ported crash/UB fixes from `mc2-opengl-remastered` (two batches)
- [x] CI: Linux x64 GCC, Linux x64 Clang, Linux arm64 GLES, Windows MSVC, shader validation (GLSL ES 3.10 + GLSL 4.20); Dependabot
- [x] Agent setup: `CLAUDE.md`, `.claude/rules/`, `.claude/skills/`, this file
- [x] Unit tests: doctest harness in `tests/unit/` under CTest, run by CI on all four configurations; FIT/packet/file, CSV, FST, paths, pixel repack, RollDice (22 cases)
- [x] Smoke runs: `-autoexit <gameplay frames>`, `scripts/smoke.sh` (Xvfb + llvmpipe), `scripts/build_gamedata.sh`; nightly `smoke.yml` on x64 desktop GL and arm64 GLES (mc2_01/03/10/17/24)
- [x] Sanitizers: `MC2_SANITIZE`; unit tests under ASan + UBSan on every push; nightly sanitized smoke (x64, 3000 frames); no UBSan suppressions (the five missions run clean)
- [x] Warnings + static analysis: `-Wall -Wextra -Werror` on new code; `-fpermissive` removed; cppcheck gate against a CI-generated baseline (changed files per push, full tree nightly)

**Not verified / unknown**

- [ ] Running on real Pi 4/400 hardware: record here once confirmed (which missions, frame rate)
- [ ] Desktop GL run on Windows after the GLES changes

**Next** (see `planning/QUALITY-ROADMAP.md` for the full list)

- [ ] More tests: on-disk struct sizes (`static_assert`), legacy `mclib/stuff` `TestClass()` self-checks
- [ ] cppcheck bulk cleanup: filter the "Stuff" duplInheritedMember pattern (~1800), `%d`/`long` printf sweep (~444) (roadmap Phase 2)
- [ ] Sanitized smoke on arm64 too
- [ ] Deterministic smoke runs (seed the RNG)

## Known issues

- Networking is stubbed (UI only). The multiplayer screens (`code/mp*.cpp`) still dereference `MPlayer->getPlayerInfo()`, which always returns NULL, in many places; they are unreachable because the main menu's Multiplayer button is disabled. Only the sites compilers flag are guarded so far.
- Sound: panning/doppler not implemented (alariq README).
- Some files may still be written outside the user config directory (alariq README TODO).
- WSLg's D3D12 GL driver shows a white screen with desktop GL; use `LIBGL_ALWAYS_SOFTWARE=1`.
- `-mission <name>` with a mission that does not exist crashes (SIGSEGV) instead of reporting an error.
- Fog byte semantics differ: actors (and crater/gvactor/mech3d) turn "no fog" (fogFactor 256) into 0, terrain (`quad.cpp`) uses 0xff. Original behaviour, kept; worth checking visually on the Pi.
- `mech3d.cpp` helicopter dust cloud computes a motion-relative matrix and then draws with the plain one, unlike the water wake next to it. May be deliberate; unverified.

## Log

Newest first. `YYYY-MM-DD: what (commit or PR)`.

- 2026-10-29: cppcheck error-level cleanup: 4560 -> 4415 findings, errors ~232 -> 101 (rest mostly known false positives). Real fixes include script-function NULL crashes and a SIGFPE (`ablmc2.cpp`), a stack overflow and out-of-bounds reads (`mover.cpp`, `gvehicl.cpp`, `missionbegin.cpp`, `getNewFile`), and 206 signed-shift colour constants.
- 2026-10-29: Warnings + static analysis: `MC2_STRICT_WARNINGS` (-Wall -Wextra -Werror) on new code, `-fpermissive` dropped, `scripts/cppcheck_gate.py` + baseline, cppcheck on changed files per push and full tree nightly (`static.yml`).
- 2026-10-29: UBSan baseline emptied and `ubsan.supp` deleted: memcpy for packed ABL bytecode / ABL assignments / mech icon pixels, uninitialised trailing ABL code byte, `RoutineKey : int`, `TG_TypeNode::CreateFrom` placement new (all ~40 TG_Shape findings). Sanitized nightly smoke now 3000 frames.
- 2026-10-29: Sanitizers: `MC2_SANITIZE`, per-push ASan+UBSan unit tests, nightly sanitized smoke. Fixed 6 bugs it found plus a third networking-stub NULL in `mpprefs.cpp`; legacy UB baselined in `ubsan.supp`.
- 2026-10-29: Sanitized CI runs and 3000-frame runs found more: a crash when an in-mission movie file is missing (`mc2movie.cpp`), gate explosion range read through a Turret cast (`carnage.cpp`), uninitialised `blockReachedEnd` (`gvehicl.cpp`), overlapping `strcpy` (`notationfile.cpp`), two MP-screen stub NULLs. All fixed; nightly smoke now runs 3000 frames (1500 sanitized).
- 2026-10-29: Smoke runs: `-autoexit`, `scripts/smoke.sh`, `scripts/build_gamedata.sh`, nightly `smoke.yml`. Found and fixed a spurious `GL_INVALID_ENUM` left by `glewInit()` on core profiles, and the `STOP:` log separator being overwritten.
- 2026-10-29: Unit-test harness ported from the remaster (doctest + heap shim, CTest, CI on all four configs); new tests for path conversion, pixel repack (extracted to `pixel_repack.cpp`), RollDice.
- 2026-10-29: Agent docs checked against the code: `File::open` order (loose files before FST) and `Environment` callback location corrected; em dashes removed from back-ported comments.
- 2026-10-29: Dependabot action bumps merged (#1, #2, #3).
- 2026-10-29: Agent setup drafted (CLAUDE.md, rules, five skills, planning/); roadmap moved to `planning/`.
- 2026-10-29: CI push builds limited to `pi-port`; PRs still run (`18594b6`).
- 2026-10-29: Back-port: animation double-step and null guards (`495f7ba`).
- 2026-10-29: MSVC positional `SDL_DisplayMode` initialiser (`561e5c3`).
- 2026-10-29: CI matrix + shader validation (`e43cef9`).
- 2026-10-29: Clang `VFX_line_draw` callback casts (`0a0ebd9`).
- 2026-10-29: arm64 build and behaviour fixes (`57e2e25`).
- 2026-10-29: Back-port: engine crash/bug fixes batch 1 (`35e03e5`).
- 2026-10-29: `BUILD-PI.md` (`77498b7`).
- 2026-10-29: GLES 8-bit texture repack (`ce99084`).
- 2026-10-29: GLES 3.1 renderer (`2e13c0d`).
- 2026-10-28: Linux build fixes for Ubuntu 22.04 / GCC 11 (`8a4ec96`).
