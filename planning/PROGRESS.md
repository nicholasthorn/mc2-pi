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

**Not verified / unknown**

- [ ] Running on real Pi 4/400 hardware: record here once confirmed (which missions, frame rate)
- [ ] Desktop GL run on Windows after the GLES changes

**Next** (see `planning/QUALITY-ROADMAP.md` for the full list)

- [ ] More tests: on-disk struct sizes (`static_assert`), legacy `mclib/stuff` `TestClass()` self-checks
- [ ] Warnings + cppcheck baseline; ASan/UBSan job (unit tests are ready for it)
- [ ] Auto-exit flag and smoke runs under Xvfb + llvmpipe

## Known issues

- Networking is stubbed (UI only).
- Sound: panning/doppler not implemented (alariq README).
- Some files may still be written outside the user config directory (alariq README TODO).
- WSLg's D3D12 GL driver shows a white screen with desktop GL; use `LIBGL_ALWAYS_SOFTWARE=1`.

## Log

Newest first. `YYYY-MM-DD: what (commit or PR)`.

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
