# Code review of `pi-port` (2026-10-31)

Review of everything on `pi-port` that is not in `origin/master`:
`origin/master...0f1c6ee`, 88 commits, about 9k lines of code once docs, the
cppcheck baseline and `doctest.h` are left out. PR #5 (`ca33e64`) was skipped,
since it had already been reviewed.

Six areas were reviewed separately: build/CI/scripts, renderer and shaders,
platform layer and `gui/`, engine (`mclib/`), game (`code/`), and benchmark
plus unit tests. Every finding was traced in the code. The high one and the
data-dependent ones were also checked against `mc2srcdata`. Nothing here was
reproduced by running the game.

Baseline: the branch builds on Ubuntu 26.04 (GCC 15) for desktop GL and GLES,
and the unit tests pass on both.

Fix plan: one PR per area, highest severity first. Tick items off here as they
land and note the PR.

## High

- [x] **G1. Buildings never get a sensor** (fixed in #13) (`code/bldng.cpp:245,294,1271`,
      `code/objtype.cpp:168`; from `22f7c98`).
  - `22f7c98` removed `BuildingType::teamId` because it shadowed
    `ObjectType::teamId`, but the shadow was load-bearing.
  - `BuildingType::init` reads `TeamID` from the building fit, then calls
    `ObjectType::init(&bldgFile)`, which sets `teamId = -1` unconditionally.
  - So every building type now has team -1. `Building::init` therefore sees no
    team and never calls `setSensorData`, the only place a building's sensor
    is created.
  - Effect: sensor towers (`objects/liaosensortower.fit`: `TeamID=0`,
    `SensorRange` set) reveal nothing, and the sensor-captured Betty line
    never plays.
  - Fix: keep the `TeamID` read in a local and assign it after the
    `ObjectType::init` call. Add a comment saying why the order matters.

## Medium

- [x] **P1. (fixed in #12) `-bench` input filter does not freeze input**
      (`GameOS/gameos/gameosmain.cpp:203-210`, `gos_input.cpp`; from `066ec5d`).
  - The filter drops SDL events, but GameOS reads keyboard and mouse buttons
    from polled state (`SDL_GetKeyboardState`, `SDL_GetMouseState`), and mouse
    position only comes from motion events.
  - So the mouse stays at (0,0), and `moveCameraAround`
    (`code/missiongui.cpp:3349`) scrolls left and up every frame of a plain
    `-bench` run.
  - Stray keys and clicks still get through.
  - Bench numbers without `-benchscenario` measured a camera sliding to the
    map corner.
  - Fix: under `g_bench`, skip `updateKeyboardState` / `updateMouseState` and
    park the mouse at the centre of the window.
  - Check `fix/bench-review` first: another session has uncommitted edits to
    these files.
- [x] **B1. Smoke runs can miss `STOP` lines** (fixed on `fix/review-ci`) (`scripts/smoke.sh:82`; from
      `b36ea54`).
  - The pattern is anchored (`^STOP: `), but `logmsg()` adds no newline, and
    369 of 387 `STOP` format strings and about 102 `PAUSE` strings have none
    either.
  - So a `STOP` after a `PAUSE` or after partly flushed stdout is mid-line, and
    the run passes.
  - Fix: drop the anchor, and have `logmsg` end each message with a newline.
- [ ] **T1. Bench "closeup" screenshot is taken in the wrong segment**
      (`bench/mc2_19.fit:51`; from `2d44620`).
  - `Shot = 2400` is the first frame of Segment3, so the close-up image shows
    the effects framing and the first artillery strike.
  - Fix: take the shot inside 1800..2399 (for example 2100), and have
    `BenchScenario::load` reject a shot outside its segment.

## Low

**Build, CI and scripts**

- [x] **B2.** (fixed on `fix/review-ci`) `smoke.yml:56` game-data cache key misses the code the data tools
      link (`mclib/file.cpp`, `mclib/mlr`, `mclib/msl*`, `stuff`,
      `GameOS/src`), so data-tool changes there test stale data. Hash
      `mclib/**` and `GameOS/src/**` too. (`d772bdc`)
- [x] **B3.** (fixed on `fix/review-ci`) `scripts/cppcheck_gate.py:79-96` ignores cppcheck's return code,
      so a cppcheck that fails without printing findings reports "0 new" and
      passes. Fail on a non-zero return code with no findings parsed.
- [x] **B4.** (fixed on `fix/review-ci`) `scripts/check.sh:169` compares the gate's `NEW ... (xN)` lines as
      text when the local and baseline cppcheck versions differ, so going from
      x2 to x1 counts as a new finding. Compare per-key counts. (`ea07d53`)
- [x] **B5.** (fixed on `fix/review-ci`) `scripts/validate_shaders.py:19-24` does not validate the
      `#version 330..410` + `GL_ARB_shading_language_420pack` header that
      `buildShaderHeader()` emits (the WSLg path). Add that variant.
- [x] **B6.** (fixed on `fix/review-ci`) `tests/unit/CMakeLists.txt:85` copies fixtures only at configure
      time, so incremental local runs test stale fixtures. Copy them with a
      build-time dependency.
- [ ] **B7. mc2_17 smoke at 3000 frames is flaky** (found while verifying
      B1, not by the review).
  - On llvmpipe (Ubuntu 26.04, desktop GL), mc2_17 failed in 3 of 4 runs.
  - Each failure exits with status 0, with no `[AUTOEXIT]` and no error
    line, after `[MISSION] mc2_17 started`.
  - It fails the same way on `0f1c6ee`, without the B1 change.
  - Likely the mission ends (won or lost) before 3000 gameplay frames, since
    smoke runs are not seeded.
  - Find out what ends it. Then seed smoke runs (the `-bench` seed), or
    have smoke treat a clean mission end as a pass.

**Renderer**

- [ ] **R1.** `shaders/gos_vertex.vert` never writes `gl_PointSize`, but
      `gosRenderer::drawPoints` draws `GL_POINTS` with it. GLES leaves the
      size undefined, so the planet point (`mclib/genactor.cpp:821`) may vanish
      on the Pi. Add `gl_PointSize = 1.0;` (ignored by desktop core). Upstream
      code, first reached by the GLES backend (`2e13c0d`).

**Platform**

- [ ] **P2.** `code/mission.cpp:325-331`: the "first frame of this mission"
      latch compares the `Mission` pointer, but one `Mission` object is reused
      for every mission. `[MISSION] started`, the bench reseed and
      `perf_control` therefore fire once per process. Harmless for smoke and
      bench (one mission per process). Clear a flag in `Mission::init()`
      instead. (`d772bdc`)

**Engine**

- [ ] **E1.** `mclib/move.cpp:1589,1633`: the new `badLoad` early returns in
      `GlobalMap::init(PacketFile*)` leave `areas_cellsCovered`, `areas_doors`
      or `doors_links` as raw `Malloc` memory, and `destroy()` would free
      those garbage pointers. Not reachable today because both callers
      `exit()` on `badLoad`. Zero the arrays after allocating them.
      (`43ba20a`)

**Benchmark and tests** (check against `fix/bench-review` first)

- [x] **T2.** (fixed in #12) `scripts/bench.sh:348-361` writes segment rows to `segments.csv`
      for failed runs too, with no status column, so a software-fallback run
      mixes into good data. Add a status column or skip rows for non-PASS
      runs. (`2d44620`)
- [ ] **T3.** (names, over-long strings and negative Gather fixed in #12; the `f[5] KeyN` case is still open) `code/benchscenario.cpp:21,45,56-63` trusts the scenario file:
      - an over-long Name is left unterminated;
      - an empty Name breaks the summary line and screenshot names;
      - `f[5] KeyN` leaves altitude uninitialised (non-deterministic);
      - a negative `Gather` throws `length_error`.

      Check the return codes, require a non-empty name with no spaces and
      exactly 6 floats, and reject `Gather < 0`. (`2d44620`)
- [x] **T4.** (fixed in #12) `scripts/bench.sh:322` uses `cut -c17-` where it should be
      `-c16-`, which drops the first letter of scenario error messages.
      (`2d44620`)
- [ ] **T5.** `tests/unit/test_benchscenario.cpp:58-62`: the key-order test
      cannot fail without the `stable_sort`, because the fixture keys are
      already sorted. Add an out-of-order segment. (`2d44620`)

## Checked and dropped

- ABL `seedRandom(-1)` reseeds from `time()` and would break bench
  determinism, but no stock mission script calls it (`mc2srcdata` grep).
  Recheck if mod scripts are benched.

## Checked and found correct

The reviewers' reports covered these in detail. In short:

- **Build:** strict warnings on every new `.cpp`, sanitizer flags, smoke
  exit-code logic, ES and 4.20 shader headers.
- **Renderer** (outside PR #5):
  - GLES texture repack and readback;
  - shader dialects and header selection;
  - PR #6 terrain light culling, time-based eviction and LOD hysteresis;
  - `TG_Shape` placement new, the font-texture failure path, `tga_writer`.
- **Platform:**
  - argument parsing, the `-autoexit` exit path, percentile maths, the perf
    FIFOs;
  - audio pan maths and channel handling;
  - `gui/` constructor and `copyData` fixes.
- **Engine:**
  - ABL memcpy and alignment fixes;
  - pathing size checks and the Omnitech wide layout;
  - heap caller capture on arm64;
  - the printf sweep (correct on LP64 and LLP64);
  - the constructor-initialisation batches;
  - the mech3d animation guard.
- **Game:**
  - the Omnitech ABL commands (argument strings, NULL and `isMover` checks,
    bounds);
  - the `objmgr` watch-ID clamps;
  - the missing-movie path;
  - the inventory and table bounds;
  - the MP NULL guards;
  - the constructor initialisers.
- **Bench and tests:** CSV headers against writers, percentile and median
  maths, timeout handling, segment frame attribution, fixtures by relative
  path, no wall clock in tests.

## Not covered

- Nothing was run in game. The pan direction (+1 = right) was never checked by
  ear.
- The Windows CI `PATH` line (`ci.yml:188`), where the `D:` drive colon may
  split the entry, needs a CI run to settle.
- Whether `perf record` passes on a hung workload's exit status, and whether
  `xvfb-run` keeps the `--control` fds open.
- The Pi V3D driver's `GL_MAX_UNIFORM_BLOCK_SIZE` against the 49,664-byte
  upstream `LightsData` block. This predates the branch, and the Pi runs work.
