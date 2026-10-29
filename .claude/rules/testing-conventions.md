# Testing conventions

## Current state

CI checks that all four configurations build and that every shader compiles as
GLSL ES 3.10 and GLSL 4.20. There is no unit-test framework yet. The plan is
`planning/QUALITY-ROADMAP.md` (Phase 3 ports the doctest harness from
`mc2-opengl-remastered/tests/unit/`). Update this file when that lands.

## Where TDD applies

Write the test first for:

- pure logic and math (packing, projection, RNG, containers)
- parsers and file formats (FIT/ini, CSV, packet, FST, LZ)
- anything the port changes: path conversion, texture repacking, endianness,
  `char` signedness, 64-bit / ARM behaviour
- bug fixes in those areas: a failing test first, then the fix

Do not try to unit-test GL output or legacy gameplay code wholesale. For
gameplay, add characterization tests that pin current behaviour before
refactoring, and only for code you are changing.

## Test layers (as they are added)

- **Unit** (doctest + CTest, `ctest --output-on-failure`): fast, no GL, no SDL
  window, no game data.
- **Shader validation**: `python3 scripts/validate_shaders.py`.
- **Smoke**: launch missions headless under Xvfb + Mesa llvmpipe and check they
  run without crashing. Needs built game data.
- **Hardware**: the Pi itself, for real GLES behaviour and performance.

## Running tests

- Agents **do** run the fast checks themselves after a change: the build, unit
  tests, and the shader validator. Report the result.
- Agents **do not** launch the game, run missions, or run on the Pi without
  asking: these need game data, a display or the hardware. Say which command to
  run and what it would verify.

## Never

- **No test writes to a real game install, the user's config directory
  (`~/.config`, `%APPDATA%`), saves, or the `mc2srcdata` checkout.** Tests use
  fixtures under the test directory and temporary directories only.
- No test depends on network access or the wall clock.
- Never lower a coverage threshold to make a build pass (see `CLAUDE.md`).
