# Testing conventions

## Current state

CI builds all four configurations, runs the unit tests (`ctest`) on all four,
and compiles every shader as GLSL ES 3.10 and GLSL 4.20. Smoke runs, sanitizers
and coverage are still to come (`planning/QUALITY-ROADMAP.md`).

## The unit-test harness (`tests/unit/`)

- doctest (`doctest.h`, `test_main.cpp`), one executable `mc2_tests`, registered
  with CTest. Built by default (`MC2_BUILD_TESTS=ON`) inside the main build, so
  tests compile with the production flags (`LINUX_BUILD`, `-fsigned-char`,
  `MC2_GLES`, MSVC defines).
- Run: `cmake --build build && ctest --test-dir build --output-on-failure`, or
  run `build/out/tests/unit/mc2_tests` directly (doctest options:
  `--test-suite=Paths`, `--list-test-cases`).
- Tests link **real engine `.cpp` files** listed in `tests/unit/CMakeLists.txt`,
  plus `heap_shim.cpp`, which replaces `mclib/heap.cpp` and the few GameOS
  globals they need (`systemHeap`, `Environment`, `STOP`, the FST registry). Add
  a file to the list only when a test needs it; add a symbol to the shim only
  when the linker demands it. Never link `heap.cpp` into the tests.
- A dependency that drags in GameOS (like `gos_rand()`) can be replaced by a
  definition in the test file itself, as `test_routines.cpp` does.
- Fixtures live in `tests/unit/fixtures/`, are copied into the build tree, and
  are opened by **relative** path: `File::open` lowercases the whole name, so an
  absolute path with upper-case directories fails on Linux. Scratch files are
  created in the working directory (the build tree) and removed by the test.
- Logic that sits inside GL code is moved into a GL-free file so it can be
  tested (e.g. `GameOS/gameos/utils/pixel_repack.cpp`).

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
