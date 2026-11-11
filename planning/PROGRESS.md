# MC2 pi-port: progress

Single source of truth for what is built, what is in progress and what is next.
Update at the end of every task: add a line to the log, and edit "Current state"
if what is built has changed. Keep the log to one or two lines per entry; detail
belongs in commit messages and PRs.

## Current state

**Built**

- [x] Linux build on Ubuntu 22.04 / GCC 11 (pkg-config fallbacks for SDL2_mixer/ttf) through 26.04 / GCC 15
- [x] OpenGL ES 3.1 renderer behind `-DMC2_GLES=ON` (Raspberry Pi 4/400); desktop GL unchanged
- [x] GLES texture upload repacking for 8-bit channel-count mismatches
- [x] Multiplayer between processes (`-nethost`, `-netjoin`): every game mode, order, support vehicle and recovery, turrets, results (`future-features/NETWORKING.md`)
- [x] Multiplayer from the main menu on the local network: host, browse, join, lobby, launch, results, and back for another game, tested through the screens by a walker, not yet by hand or on a Quest
- [x] Mech selection in a network game: with Quickstart off each player buys designer variants and gives them pilots, the host checks them, and every machine makes everyone's units; tested through the screens by a walker, not yet by hand
- [x] Chat in a network game through the 2001 chat window (not yet tried in play): the pilot screen's "is ready to launch" and the host's "about to start" lines reach everyone; the host can give each player other C-Bills; the data check covers the mech bay's files
- [x] arm64: `-fsigned-char`, portable heap caller capture, GCC 12 fixes
- [x] Clang and MSVC build fixes
- [x] Back-ported crash/UB fixes from `mc2-opengl-remastered` (two batches)
- [x] CI: Linux x64 GCC, Linux x64 Clang, Linux arm64 GLES, Windows MSVC, shader validation (GLSL ES 3.10 + GLSL 4.20); Dependabot
- [x] Agent setup: `CLAUDE.md`, `.claude/rules/`, `.claude/skills/`, this file
- [x] Unit tests: doctest harness in `tests/unit/` under CTest, run by CI on all four configurations; FIT/packet/file, CSV, FST, paths, pixel repack, RollDice, audio pan, JPEG, PE strings, TGA writer, frame stats, bench scenarios, global map IO, render heuristics, bounded strings, bench stages, stereo viewports, `STOP:` log line, WAV chunk walk (125 `TEST_CASE`s in the source)
- [x] Smoke runs: `-autoexit <gameplay frames>`, `scripts/smoke.sh` (Xvfb + llvmpipe), `scripts/build_gamedata.sh`; nightly `smoke.yml` on x64 desktop GL and arm64 GLES (mc2_01/03/10/17/24); a `STOP` ends the game by default (#46; smoke and bench pin `MC2_STOP_EXITS=1`, #22); front-end walks with `-uismoke` (main menu, options, campaign and single-mission dialogs, briefing, mech bay, purchase, mech lab, pilot ready, launch)
- [x] Benchmark: `scripts/bench.sh` runs missions with `-bench <csv>` (fixed 1/30 s game step and mission clock, fixed seed, input ignored), per-frame logic/render/swap ms + draw calls + triangles to CSV, plus named stage times (`camera`, `terrain_geom`, `objects`, `shape_xform`, `world_render`, `ui_render` as `<stage>_ms`), `-benchfov` / `--fov` to force the camera FOV, `[BENCH]` p50/p95/p99/max summary into `results.csv`; `--perf` adds a `perf record -g` run per mission sampling gameplay frames only (game drives perf's control FIFO), `MC2_FRAME_POINTERS` build option; `--scenario bench/*.fit` stages a scripted battle (units gathered and fighting, damage off, camera path, artillery) with named segments timed separately (`segments.csv`) and screenshots; `-bench` also turns sound off and logs a per-frame game-state checksum
- [x] Stereo debug view (VR Phase 1): `-stereo <separation>` draws the world side by side from two eye cameras, shadows and world-anchored overlays (brackets, bars, floating text) included; game state unchanged (bench checksum); mouse picking, the cursor and the interface panels still full-window (`future-features/VR-TABLETOP.md`)
- [x] Meta Quest test build: a flat Android APK (NDK r28c, arm64, GLES 3.1, SDL2 2.30.11 and SDL2_mixer 2.8.0 from source) that runs the bench on the headset (`android/README.md`); run on a Quest 2: 58.6 fps mono, 52.1 fps `-stereo` after the buffer-map fix (VR-TABLETOP.md, Quest 2 results)
- [x] VR game camera's field of view (PR #86, `future-features/VR-FOV.md`): in VR the game camera covers the headset's view (115.6 degrees on a Quest 2), not 40; `MC2_XR_FOV` overrides it
- [x] VR game camera follows the head (`future-features/VR-HEAD-CAMERA.md`): what counts as in view and picking go where the player looks; one Quest 2 run so far
- [x] Sanitizers: `MC2_SANITIZE`; unit tests under ASan + UBSan on every push; nightly sanitized smoke (x64, 3000 frames); no UBSan suppressions (the five missions run clean)
- [x] Mods: MechCommander Omnitech data (object types past `object2.pak`, wide map packets, `.burnin.jpg` colour maps, `mc2res.dll` strings, 512 object textures) loads and plays; Clan Wars mission 1 checked in play (PR 17, `future-features/MOD-COMPAT.md`)
- [x] Warnings + static analysis: `-Wall -Wextra -Werror` on new code; `-fpermissive` removed; cppcheck gate against a CI-generated baseline (changed files per push, full tree nightly); baseline 523 after fixing error-level bugs, filtering the deliberate "Stuff"/gosFX member hiding and constructor `init()`/destructor `destroy()` calls, the printf length-modifier sweep and most `uninitMemberVar`; 413 after every `uninitMemberVar` was initialised or suppressed with a reason (#23, #24)

**Not verified / unknown**

- [x] Real Pi 400 hardware (2026-10-29, at `10401db`): builds and runs. GPU (GLES) build, mc2_01, 3000 gameplay frames in 51.2 s (58.5 fps, about 800x600); that is at the 60 Hz vsync cap; with `MC2_VSYNC=0` (vsync off), mc2_01 ran at 113 fps before #5 and 179 fps after it (mc2_03 148 -> 181). Software rendering: mc2_01 50.4 fps, mc2_03 67.1 fps.
- [x] Pi: GPU frame rate with vsync off (see above and the #5 log line)
- [ ] Pi: the other three smoke missions on the GPU build
- [ ] Desktop GL run on Windows after the GLES changes

**Next** (see `planning/QUALITY-ROADMAP.md` for the full list)

- [x] On-disk struct sizes pinned at compile time (`code/ondisksizes.cpp`, 2026-11-04)
- [ ] More tests: legacy `mclib/stuff` `TestClass()` self-checks
- [x] cppcheck: baseline empty since 2026-11-01 (#68); see "Work through the cppcheck baseline" below
- [ ] Sanitized smoke on arm64 too
- [x] Deterministic smoke runs (seed the RNG): `-deterministic` and `smoke.sh --deterministic` (2026-11-01); opt-in, since it turns sound off
- [ ] GPU benchmark phases 3-4: GPU timer queries, compare script, HUD (`future-features/GPU-BENCHMARK.md`); a second scenario on a daylight map
- [ ] UI smoke tests: a scripted player for the menu screens (main menu, purchase, mech lab, pilot ready, Mechlopedia, options, load/save) and a `smoke.sh` `ui` tier, on stock and mod data (`future-features/UI-SMOKE.md`). Also step 2 of the `STOP` plan below, and the only way to see Omnitech's units in logistics. (10-31) Slices 1-2 built (`-uismoke`, two walks in `smoke.sh`, buying a mech through to launch); next: tutorial, mission results / salvage, mission selection.
- [ ] CI benchmark regression job: `-bench` on base and PR, fail when draw calls or triangles grow > ~10%, frame times informational. User said "not now". (10-31)

## To do: process and housekeeping

Added 2026-10-31.

- [x] **Upgrade Ubuntu on WSL to the current LTS** instead of working around
      what 22.04 lacks (GCC 11, older Mesa, `perf` without libelf, missing
      CMake config files for SDL2_mixer/ttf). Afterwards, check which
      workarounds can go (the `cmake/FindSDL2_*.cmake` pkg-config fallbacks stay:
      other distros need them) and update `BUILD-LINUX.md`.
  - 2026-10-31: WSL `Ubuntu-26.04` (GCC 15, Clang 21, cppcheck 2.19) is the
    development environment; `BUILD-LINUX.md` lists the tested setups. No
    code workaround was 22.04-only: the pkg-config fallbacks stay. The old
    `Ubuntu` (22.04) distro still holds about 9 GB of mod and remaster data
    (see Housekeeping).
- [x] **Code-review everything on `pi-port` so far** (83 commits ahead of
      `origin/master` as of 2026-10-31). One diff that size reviews badly, so
      review it in batches by area, oldest first: build/CI, GLES renderer and
      shaders, platform layer, engine (`mclib/`), game (`code/`), tests and
      scripts. Run `/code-review` per batch, collect the confirmed findings in
      one list, then fix them through PRs, one area per PR. Skip what PR #5
      already had reviewed.
  - 2026-10-31: review done (88 commits, six areas). Findings and their
    status: `planning/REVIEW-PI-PORT.md`. Fixed so far: G1 (#13), P1, T2,
    T4 and most of T3 (#12), B1-B6 (#18), R1, P2, E1 (#20); B7 (mc2_17
    smoke flake) gone since #19; T1, the rest of T3, T5 (`review/followup`).
    All 18 findings closed.
- [x] **Move all agents onto the updated agent process**: workflows, the
      three-tier process, and PRs into `pi-port` instead of merging or pushing to
      it directly. Update `CLAUDE.md`, `.claude/rules/` and `.claude/skills/` to
      match, so new sessions pick it up.
  - 2026-10-31: `CLAUDE.md` ("Workflow") and `.claude/rules/agent-workflow.md`
    carry it (#8), and every change since #10 went in by PR. The area skills
    need no change: `CLAUDE.md` is loaded in every session.
- [x] **Sweep past session transcripts for dropped ideas** (2026-10-31: the
      six mc2 and mc2-opengl-remastered transcripts on this machine; results in
      "Tidy up" below).
- [x] **Work through the cppcheck baseline** (`scripts/cppcheck-baseline.txt`,
      523 findings before #23/#24, 413 after, 149 on 2026-11-01, and
      empty since 2026-11-01: every finding fixed or suppressed inline
      with its reason; regenerated by CI). A new finding now fails CI
      outright: fix it, or suppress it inline with the reason.
      We got bitten by an `uninitMemberVar` that was already in it, so the baseline is a list of known bugs, not noise to
      ignore. Triage every finding: fix it, mark it a false positive with an
      inline `// cppcheck-suppress` and a reason, or record why it is safe.
  - [x] `uninitMemberVar` / `uninitDerivedMemberVar` (115): done in #23
    (gosfx, mlr, stuff) and #24 (code, gui, the rest of mclib, GameOS).
    Members initialised in their constructors, 12 inline suppressions with
    reasons. The one real bug: `ABLModule::orderCallFlags` is now `NULL` in
    `ABLModule::init()`, so a module without order calls skips the flag test
    instead of reading and writing through a garbage pointer.
  - [x] `nullPointer` (87), `invalidLifetime` (11) and `uninitvar` (6)
    (branch `fix/cppcheck-null-lifetime`): `TG_TypeNode`/`TG_TypeShape::
    CreateFrom` check the TGL heap allocation before placement `new` (53),
    the multiplayer stubs return a blank player record, an empty name and a
    zero player count instead of NULL / an unset count (about 30),
    `Camera::getClosestVertex` copes with no vertices, a load error printed
    an unset buffer (`LogisticsMissionInfo::load`), `MechWarrior` checks and
    uses one `getGroup()` result; the effect `drawInfo` lifetimes (11) and
    the bounded `qsort` of player lists (4) are suppressed inline as false
    positives with the reason.
  - [x] `pointerSize` (7) and `arrayIndexOutOfBoundsCond` (10) (branch
    `fix/cppcheck-pointersize`): `InfoWindow::init` cleared 56 bytes of its
    seven `StaticInfo` records (`sizeof` of a pointer); `TerrainTextures`
    zeroed only the first mip level of `maskData`; team sensors, ECMs and
    anti-missile systems wrote past their arrays after a `Fatal()` that only
    logs; `SensorSystem::removeContact` trusted an index checked against 255
    for a 200-entry list; the multiplayer status chunk read `moverRoster`
    after a bounds check that only set a flag. Five `sizeof` of pointer-array
    elements suppressed inline as intended. Baseline regenerated after #33:
    413 -> 310.
  - [x] `memsetClassFloat` (43) (branch `fix/cppcheck-memset`): 40 are a
    zero fill, exactly 0.0f on IEEE 754, so the gate drops the check when
    the source line's fill is 0 and still reports a non-zero one; the two
    real ones, `0xff` fills in the ASE parser (`tgl.cpp`), are now zero
    (a mesh without texture faces kept NaN UVs in `tgl.fst`).
  - [x] `duplInheritedMember` (41) (branch `fix/cppcheck-dupl`): two real
    ones, both latent: `Camera::destroy` is now virtual (`eye->destroy()`
    ran Camera's on a GameCamera), and the accept-variant dialog's own
    `selectedName` is gone (a delete would have removed the wrong variant;
    stock data has no delete button there). Dead duplicates removed
    (`Building::isVisible`, `BattleMech::initOptimalCells`, two MP
    `isDone`); 28 intended ones suppressed inline with the reason, the two
    save-game struct fields among them (loads tell types apart by size).
    `TerrainObjectType::subType` (the shipped-game forest pathing bug) is
    kept as shipped, decided 2026-11-01, and suppressed with that reason.
  - Baseline regenerated by CI 2026-11-01 (after #42, #46, #48): 310 ->
    149, no finding up. `nullPointerRedundantCheck` 84 -> 27 now that
    `Fatal()` is `[[noreturn]]`.
  - [x] `nullPointerRedundantCheck` (branch `fix/cppcheck-nullptr-redundant`):
    the 27 left in the regenerated baseline (25 sites with 2.19) once
    `Fatal()` was `[[noreturn]]` (#46). Real ones: `MechListBox::AddItem` read a
    `dynamic_cast` result before its NULL check, `TerrainObject` read
    `cellsCovered` after a failed `Malloc` (now `Fatal`), MechWarrior
    power up/down used the vehicle before checking it, `MechWarrior::setup`
    checked `new` after use. The rest were checks made redundant by an
    earlier unconditional use (mech appearance, force-group icon unit,
    mech lab variant, mission interface, MP stats, terrain colour map heap,
    texture manager vertex lists), now removed; two false positives
    suppressed inline (ABL array types, `Stuff::Tree`).
  - Next: `allocaCalled` (27),
    `operatorEqVarError` (12), `objectIndex` (10), then the copy /
    assignment group (`noOperatorEq`, `noCopyConstructor`, 8 each) and
    the rest.
  - Versions: the gate runs 2.19.0 (built from source, cached, same as local
    on Ubuntu 26.04); the nightly run also runs Ubuntu 24.04's 2.13. 2.7 found
    the `GVAppearance` one that bit us, so clang-tidy's init checks are still
    worth a look.
  - `nullPointerOutOfMemory` stays off in the gate after reading all 68 sites
    (2026-11-04). Fixed 7: the entry count in an FST header (crashed on a corrupt
    archive) and `NumFastFiles` in `system.cfg`. Small fixed or bounded sizes 28,
    buffers of a file's real length 16, dead code 13 (`refractalizeBaseMesh` and
    `rescaleMap`, nothing calls them).
  - [x] The last 4 are sized by a TGA header's width and height (`flipTopToBottom`,
    `burnInShadows`), and a NULL check would not help: a header that lies about
    its size overruns the buffer in the caller's `memcpy` first. The TGA readers
    now check the header against the file's length (branch `fix/tga-header-check`).
  - Regenerate the baseline from CI (`static.yml` `update_baseline`) after
    each batch, so the count only goes down.
- [x] **Make `STOP` / `Fatal()` stop again** (decided 2026-10-31, replacing
      the 2026-10-29 "leave as is"). GameOS documents `STOP` as "can not be
      continued"; only the port's commented-out `exit(1)` in
      `InternalFunctionStop` lets code run on into the NULL or full array a
      fatal was guarding (~400 `Fatal` and ~400 direct `STOP` calls). Plan:
  1. [x] An env switch (e.g. `MC2_STOP_EXITS=1`) makes `STOP` log, show an SDL
     message box when there is a display, and `_exit` (not `exit`: static
     destructors over a broken engine crash). `smoke.sh`, the sanitized smoke
     and `bench.sh` set it. Done 2026-10-31 (#22, #28) without the message box (under
     Xvfb it would block until the smoke timeout): add it in step 4. `_Exit`
     skips `SDL_Quit`: check on the Pi's own display (KMSDRM) that the
     console comes back after a bench run that hits a `STOP`.
  2. [x] Widen smoke coverage to the screens it never opens (logistics, mech lab,
     options, save/load) so more `STOP`s get exercised
     (`future-features/UI-SMOKE.md`). Slice 1 in (#30): main menu, options,
     briefing, mech bay, purchase, pilot ready. Mech lab and save/load come
     with later UI smoke slices.
  3. [x] Every `STOP` that fires but is not really fatal (e.g. "Move to order
     location is Not a Number" in `ablmc2.cpp`) becomes a warning: `PAUSE`
     or a log line, with any needed bail-out. Done 2026-10-31: with the
     switch off, no `STOP` fired in the 30 stock missions, both UI walks
     (GL and GLES, 600 frames) or 100 Omnitech missions (GL, 300 frames);
     the NaN move order is a `PAUSE` and skips the order.
  4. [x] Then make exiting the default, keep an opt-out variable for mod
     bring-up, and mark `Fatal()` / `ABL_Fatal()` `[[noreturn]]` so cppcheck
     and the compilers can see that code after them doesn't run. Done
     2026-11-01: `STOP` (and `Assert`) log, show an SDL message box (not in
     automated runs) and `_Exit(1)`; `MC2_STOP_EXITS=0` makes them carry on.
     `Fatal()` / `ABL_Fatal()` end through `gos_FatalExit()` even then.
     The exit now calls `SDL_Quit` (after the message) before `_Exit`, so
     the display mode is put back. The box was checked on Linux (Xvfb, no
     window manager, `FullScreen = TRUE`, missing mission): it shows the
     `STOP` text over the game window, and OK ends the game with status 1.
     Still to check: the box on Windows and under a desktop window manager,
     and the Pi console after a `STOP`.
     Follow-ups from the review:
     - [x] The `STOP:` line format smoke relies on is unit tested
       (`formatLogLine`, 2026-11-01).
     - Decided, not a bug: an ABL syntax error ends the game even with
       `MC2_STOP_EXITS=0`, so mod bring-up sees one compile error (with
       file and line) per run. Listing more would need `syntaxError` (131
       callers) to keep parsing after an error and every
       `ABLi_preProcess` / `ABLi_loadLibrary` caller (9) to check the
       error count, which none does today; `MAX_SYNTAX_ERRORS` was cut
       from 20 to 1 in the original source.
     - [ ] Screens smoke has not opened (save/load, Mechlopedia, results,
       salvage) may hold `STOP`s that used to be harmless (UI smoke
       slices).
     - [ ] A `STOP` off the main thread shows an unparented box (none
       known: no game code runs on an SDL callback thread).

## Tidy up (from the 2026-10-31 transcript sweep)

Things noted in past sessions that were not tracked anywhere. Session dates in
brackets. Checked against this file, the roadmap, `future-features/` and git
on 2026-10-31.

**Work at risk or blocked before merge**

- [x] `release-plan` branch: rebased and merged as PR #10 (`0f1c6ee`).
- [x] `omnitech-objtypes` (7 commits, pushed, no PR): its code review found 8
      problems and none were fixed or written down. Two corrupt memory because
      `Fatal` returns: `objtype.cpp` `load()` overflows the heap when a
      `movobj.fit` entry's file is missing; `MOVE_readData` reads the
      special-area packet after its size check fails. Also: Omnitech `.fit`
      inside an FST can't load as a type (`getMemoryView` rejects FST files);
      non-square `.burnin.jpg` over-reads; quick load falls through to a
      rebuild that `memcpy`s from NULL; stock data may now pick JPG colour maps
      over TGAs; FST entry names rehashed without lowercasing; in-memory FIT
      view doesn't clamp its length. Suggested: pass `destLen` to `LZDecomp`
      in `readPacket`, stop decompressing each `.txm` twice at load,
      de-duplicate the `movobj` lookup. Its work has no PROGRESS log lines. (10-31)
      Fixed in `22797e8` before it merged as #17 (`readPacket` bounds
      `LZDecomp` too), with follow-ups in #21.
- [x] PR 7 (`bench-phase2`, merged as `2d44620`) and bench phase 1
      (`066ec5d`, `34c5d59`, landed without a PR) code-reviewed 2026-10-31.
      Findings (missing scenario keys read as 0, `segments.csv` without a
      status, plain `-bench` edge-scrolling the camera because the mouse sits
      at (0,0), keys and clicks leaking into bench runs, perf ack FIFO that
      can hang, and smaller ones) fixed in PR #12 (`82bbf18`).
- [x] The last three-config rebuild in the Omnitech session exited 1 and was
      never looked at; WSL Clang had been removed by another session's
      cleanup, so it may only be that. (10-31) Superseded: #17 and #21 went
      through CI's four-configuration build.

**Bugs noticed in passing**

- [x] Found by the 2026-11-01 cppcheck batches, outside their checks. Fixed
      on `fix/noticed-bugs`: `aObject::rectIntersect(const GUI_RECT&)` compared
      x with y, so a mech bay drop on the mech list counted as over the force
      group; `SalvageListItem::icon` was never deleted; `getBestPilot` read a
      NULL `pPilots[0]` with no free pilot (reuses a roster pilot; an empty
      roster is a `Fatal`); `aAnimButton`'s scaled render resized to about 0.5
      * scale * width squared (only `mcl_cm_standinop.fit` scales, and no stock
      data loads it). Fixed in #60: `EWCharString::Format`, the zero font
      metric in `renderObjective`.
- [x] `mech.cpp`'s death-from-above block: kept as shipped, decided 2026-11-03
      (see "Kept as shipped").
- [x] `streamToBuffer` (`gameos_graphics.cpp`): a failed `glMapBufferRange`
      only `gosASSERT`s (compiled out) and draws stale data. (PR #5 review)
      Fixed on `fix/passing-bugs`: the draw is skipped, also when
      `glUnmapBuffer` reports a lost data store.
- [x] `VirtualAlloc` emulation (`GameOS/src/platform_winbase.cpp:208`): the
      unsupported-type branch only `assert`s and falls off the end without a
      return (UB in release; GCC 15 warns). Return NULL. (10-31) Fixed on
      `fix/passing-bugs`.
- [x] `IfaceCallStrike` (`code/artlry.cpp` ~159-200): dereferences
      `*strikeLoc` when only a target was passed (NULL); with both flags false
      `commanderID` stays -1 and indexes `commanders[-1]`; ABL `callstrike`'s
      documented ids (248, 507, ...) don't match the 0-2 enum, so those calls
      are dropped. (10-31) Fixed on `fix/passing-bugs`: uses the resolved
      location, drops a strike with no owning commander, and `callStrike`
      now passes its clan flag for target strikes (`callStrikeEx` already
      did). The ids are only a stale comment: the one stock call
      (`mc2_17_rebel_rptruck.abl`) passes 1, `ARTILLERY_LARGE`.
- [x] `Camera::update` (`mclib/camera.cpp` ~1584) applies velocity only when
      all three components are non-zero (`&&`). (10-31) Fixed on
      `fix/passing-bugs` (`||`). Only ABL `SetCameraVelocity` /
      `SetCameraGoalVelocity` set it, and every stock use is commented out, so
      stock missions are unchanged; mods that use them now move.
- [x] `mission.cpp`: `revealTacMap` is shadowed by a local (~1836) so the
      global is never set; `godMode` does nothing since `markSeen` was
      stubbed; `Team::noPain` is not reset by `Mission::init`. (10-31)
      Closed without a change: nothing reads `revealTacMap`; `godMode` is a
      debug setting whose reveal is commented out in the original source;
      `noPain` carries across missions on purpose (`-nopain` sets it on the
      command line before any mission loads, and saves store it).
- [x] `FitIniFile::readId*` (`mclib/inifile.cpp`) treats a key on the last
      line of a block as missing. Seen in the PR #12 review. (10-31) Narrower
      than first thought: a block runs to the next header or `FITend`, so only
      the last line of a file without `FITend` was missed (arrays even after
      the key was found). Fixed and unit tested for scalar, array and string
      readers. No stock file has that shape, so stock values are unchanged.
- [x] `shaders/object_tex.frag` / `.vert`: unused, carry their own `#version`
      and use `texture2D` (won't compile on GLES). Delete. (10-29) Deleted;
      `validate_shaders.py` now fails on a `#version` line instead of
      skipping the file.

**Performance backlog (Pi)**

- [x] Collected in `future-features/PI-RENDER-PERF.md` (2026-10-31): frame
      spikes from mid-mission loading, LOD rebuilds, the double terrain pass,
      static terrain mesh, GPU mechs / particles / shadows, per-draw overheads,
      mipmaps, PR #5 review leftovers, the Pi quality tier and the deferred CI
      benchmark job. Bench spec counters not built yet (binds, upload_kb,
      movers/effects columns, `MC2_BENCH_FINISH`) stay in `GPU-BENCHMARK.md`.

**Not verified**

Everything that needs a display, a person or the Pi is collected in
`planning/HARDWARE-TESTS.md`; tick items there as they are run.

- [ ] On the Pi: `-bench` frame-match, `--perf` (dwarf mode not tested
      anywhere), `--scenario`; the GLES scenario run with the circling camera
      not even in WSL. PR #6 hitch numbers never measured. (10-31)
- [ ] Logistics, mech lab, options and tutorial screens: smoke never opens
      them, yet many cppcheck fixes landed there (`LogisticsScreen` copy /
      assign, `OptionsXScreen`, mech lab, tutorial button, `logisticsdata`).
      Needs a plan for smoke coverage. (10-29 on) Partly covered since
      10-31 (`-uismoke`): main menu, options, campaign and single-mission
      dialogs, briefing, mech bay, mech purchase (buy), mech lab (open and
      Cancel), pilot ready (assign, launch) run in smoke. Still not: mission
      selection, salvage, tutorial, changing a weapon in the mech lab
      (`future-features/UI-SMOKE.md`, slices 3-5).
- [ ] Loading a saved game through the size-checked pathing reader
      (`saveload.cpp` now also exits on `badLoad`). (10-30)
- [ ] PR #9 fixes in game: `isInfantry`, font texture, MemoryStream; air-strike
      bombers now render light grey, not checked against the intended art. (10-31)
- [ ] Terrain blend caches resized to the current size not checked on
      screen; `.txm` cache names don't include their size and every mission
      shares them. (10-31)
- [ ] llvmpipe on the Pi dropped from 50/67 fps to 45/48 and then ~15 fps;
      cause never confirmed from `results.csv`. Low priority. (10-31)

**Back-ports from the remaster not yet done**

- [x] Original LZ decoder in C (`7d6f9bc6`), raw FST entries when
      `size == realSize` (`3b5f87ca`), bounds fixes `adbf3c9a`, `69ee9782`.
      (10-31, branch `fix/backport-lz-decoder`) Ported: the C decoder,
      rewritten against the asm and bounded by `destLen`, chosen by the
      first two bytes (original LZ starts with a 9-bit CLEAR, so byte 0 is
      0x00; zlib never is); raw entries, decoded first and copied only if
      that fails. The bounds fixes were already in from #17/#21. Omnitech's
      own `tgl.fst` now reads (all 4667 entries) and mc2_01/mc2_03 run with
      it; the Omnitech test setup still uses the stock one.
- [x] Mission / ABL hardening (10-31): ported `b71702ba` (no commanders:
      team 0 and commander 0 are created, since STOP only logs), the
      `Team::home` / `Commander::home` reset and the commander re-init leak
      from `e3dde9e4`, `a4e5db6d` (missing ABL file returns -1, no exit) and
      `e4a21257` (null type in `execExpression`). Skipped: the rest of the
      `e3dde9e4` refactor and `2da701ba` (its follow-up), and all of
      `468991c3`: the arity half fixes remaster-only stubs (stock defines
      `corePatrol`/`coreGuard` in `corebrain.abx`), and the `execStatement`
      null guard leaves the stale token in place, so the next call reads the
      following bytes as another pointer. (10-29)
- [x] Faster save dialog that also fixes a leak (`2059e8eb`). (10-29, done
      10-31)
- [ ] From the 2026-11-01 sweep, settled by step 4 of the `STOP` plan
      (#46): these now end the game cleanly instead of crashing, and only
      run on with `MC2_STOP_EXITS=0` (mod bring-up). Port them only if mods
      should carry on past them: `80e98102` (`ObjectTypeManager::load` uses NULL after a
      CRAPPY_OBJECT `Fatal`), `424b2ce7` (`Mission::addMover` treats a
      vehicle in the force group as a mech; the one-argument overload has
      the same pattern), `9356f8e3` + `6daa0905` (a module with compile
      errors still runs).
- [ ] Leads from the same sweep, not ported as they are: a mission with no
      MOVE data runs on with `GameMap == NULL` after its `STOP` (`f1eaa6ee`;
      exit like the bad-packet branch instead; `STOP` now ends the game, #46);
      the briefing minimap may upload alpha-0 water and concrete cells
      (`3c8dd69e`, check by eye first); `controlgui.cpp` adds `hiResOffsetX` but not `hiResOffsetY` to the
      video box (`51ddcf25`, check in game). Never port `c92175fd`: it
      treats real callback addresses on Linux and arm64 as bogus.
- [x] Mod-tolerance null guards: `a03e13a2`, `294fd42e`, `aa20a308`,
      `84e1774a`, `d21e7855`, `1cf8c7b8`. (10-29, done 10-31: `294fd42e`,
      `d21e7855`, `1cf8c7b8` ported; `a03e13a2` and `aa20a308` not needed
      here; `84e1774a` (`move.cpp`) is left to the Omnitech work)

**Docs out of date**

- [x] "Pi: GPU frame rate with vsync off" is unticked although the #5 log has
      the numbers (113 -> 179, 148 -> 181 fps); the "headroom unmeasured" note
      is stale.
- [x] Known issues still says panning is not implemented; #4 (`fa0188e`)
      merged stereo panning. The by-ear check was never recorded.
- [x] PR #6 (`01bfcb3`) has no log line.
- [x] MSVC's `small` macro (fixed in `4b98eee`) belongs in "Portability traps"
      in `build-conventions.md`.
- [x] `MOD-COMPAT.md`: name Dark Gray: Borderworlds as the MC2X test load,
      and note the remaster's LZ decoder against the LZ gap. (Done
      2026-10-31, after the Omnitech branch landed.)

**Housekeeping**

- [x] Stale branches and worktrees: `fix/uninit-members` (rebased copy),
      `claude/nostalgic-brahmagupta-fb1902`, the `bench-phase2` worktree,
      `C:\Users\Dell\Documents\GitHub\mc2-wt-cw`. `.claude/worktrees/` was
      added to the local `.git/info/exclude` only. 2026-10-31: worktrees and
      local branches of merged PRs removed (#4, #6, #7, #8, #9, #10); the
      `nostalgic-brahmagupta-fb1902` folder was locked, so it is unregistered
      but still on disk. Worktrees in use by other sessions were left.
- [ ] WSL disk: `~/remaster-data` (~7 GB), `/srv/bookworm-arm64`, `~/mco628`
      (1.2 GB), `~/gd-mco`, `~/gd-cw`, `~/wt-cw-*`, `~/mc2r-build`,
      `~/mc2r-shim`; Downloads `MCOsetup628 (1).exe`, `ClanWarsForMCOsetup.exe`.
- [x] `uninitMemberVar-triage.md` lives outside the repo; bring its verdicts in
      (see the cppcheck item). Superseded: the note is gone, and #23/#24 put
      every verdict in the code (initialised, or suppressed with a reason).
- [x] Windows has no Python on the PATH for agents, so `scripts/*.py` only run
      in WSL. (Python 3.12 is installed as `python`; check what the agent
      sessions actually hit.) Stale: `python` (3.12) works from the agents'
      Git Bash; the scripts are still run in WSL, where the tools they call
      are.

**For the remaster repo** (its own docs, not this one)

- [ ] Classic draw path forces `gos_FilterTriLinear` to `TFM_LINEAR`
      (`gameos_graphics.cpp` ~5361), so its generated mipmaps are unused.
- [ ] `.claude/agents/mc2-gameos-expert.md` says `KeyboardFlush` leaves
      `first_pressed_` live (`resetKeypress()` sets it to -1); other agent docs
      say `STOP` halts.
- [ ] Tooling: unit tests not under CTest, no CI, 86 check scripts never run
      automatically, no clang-tidy / cppcheck (`testing-strategy.md` wrongly
      says clang-tidy is in tree), MSVC at `/W3`, GCC still `-fpermissive`.
      User said "not now".
- [ ] Forward-port from mc2-pi: #5's effect batching (less useful there,
      gosFX is off by default), light-range culling (its light cap is 1024),
      uniform lookups.
- [ ] `linux-build` branch: uncommitted CMake changes in 5 files; findings
      (27 game files fail on Win32 code, FFmpeg 5.1 channel layout API, needs
      GL 4.3/4.5) undocumented. Commit or discard.

## Planned: built releases on GitHub

Not started. Full plan: `planning/RELEASES.md`. Two audiences: constrained
systems first (Raspberry Pi 4/400 on Raspberry Pi OS), and ready-to-run builds
for everyone on ordinary Linux and Windows PCs. No game data in any package;
each ships the data tools instead.

- [x] Phase 1: packaging scripts, `--tools-dir` for `build_gamedata.sh`, install
      notes, third-party license bundle (2026-11-01, branch `release/prep`;
      a Linux x64 package built, filled with data by its own tools and
      smoke-run in WSL; not yet run on a clean machine)
- [x] Phase 2: `release.yml` (tag-triggered, draft release): Pi tarball
      (`debian:bookworm` container), Windows zip (DLLs and C++ runtime
      included), Linux x64 tarball (2026-11-01; dry runs on the PR, no test
      tag pushed yet)
- [x] Quest APK in `release.yml`, with its shaders and licences in a tarball
      (2026-11-04, branch `release-apk`; built in the dry runs on the PR, not
      yet run on a headset from a CI build)
- [x] Before the first release: replace zlib 1.2.3 in `3rdparty.zip` with a
      current zlib (2026-11-01: 1.3.1, built from the official tag on a
      GitHub Windows runner; it is linked statically into `mc2.exe`)
- [ ] The data build is not reproducible: `aseconv` gives a different
      `tgl.fst` on two runs of the same tools (62 bytes apart here; the
      other FSTs, `data/` and `assets/` are identical). Probably
      uninitialised padding written into the binary shapes. Both data
      builds smoke-run. (2026-11-01)
- [ ] Phase 3: Linux x64 and arm64 AppImages on old bases
- [x] Phase 4: smoke tests against the unpacked packages (2026-11-04: the full
      Linux x64 and Pi ones, job `full`)
- [ ] Phase 5: `v0.2.0-rc1`, run by hand on Windows, Linux x64 and a Pi 400
      (tag pushed 2026-11-01 on `80f38884`; draft release made, not published)
- [ ] Phase 6: `v0.2.0`, with Pi hardware results in the notes

## Taking the useful parts of the remaster

Aim: bring over as much of `mc2-opengl-remastered` as fits this port, not just
crash fixes. Rule for anything that costs frame time or memory: **measure on the
target first** (Pi 4/400 today, Quest later) and keep the stock data playable
without it.

Taken so far:

- [x] Crash and undefined-behaviour fixes (`/mc2-backport`; two batches)
- [x] Unit-test harness (doctest + heap shim) and several of its tests
- [x] Ideas and process: agent setup, smoke runs, sanitizers, cppcheck gate

Candidates:

- [x] Full sweep of the remaster's history (2026-11-01, `fork/main` at
      `63bb0832`, no commits since 2026-07-05): the 419 fix-like commits that
      touch files both repos have, checked against this code. 23 applied
      (stock-data fixes and mod-data guards, branch `fix/backport-sweep`),
      21 were already fixed here, 375 are remaster-only. Left open: see
      "Back-ports from the remaster not yet done".
- [ ] **Later game data: upscaled textures and art.** The remaster ships
      AI-upscaled (4x) terrain, building, mech and UI art. This port's
      `File::open` already loads a loose file in preference to the FST archive,
      so TGA versions can be dropped in without code changes. Not usable as-is:
      the remaster's KTX2/BC7 cooks (no KTX2 loader here, and BC7 is a desktop
      format), its 5120x5120 terrain burn-ins (above common GLES texture size
      limits; check `GL_MAX_TEXTURE_SIZE`), and GLB model imports (need Assimp).
      **Check performance and memory on the Pi first**: 4x textures are 16x the
      pixels, the Pi's GPU shares the 4 GB (or less) of system RAM, and upload
      time matters at mission load. Likely approach: per-tier packs (2x first),
      measured mission by mission.
- [ ] Mod-pack content (e.g. the Magic's expansion tweaks in the remaster's
      `mods/`): needs the remaster's mod loader or a port of it; scope later.
- [ ] Rendering features (PBR, shadows, terrain upgrades): mostly desktop GL
      4.3+ and far heavier than the Pi can run; only consider ones that work in
      GLES 3.1 and pass a Pi frame-time check.
- [ ] Licensing: check each asset's source and license before shipping or
      documenting it (upscales derive from the original EULA data; mod content
      has its own authors).

## Kept as shipped

Behaviour of the original game that looks like a bug but is kept on
purpose, because changing it would change how the game plays or looks.
Each one is marked in the code (a comment, and a `cppcheck-suppress` with
the reason where cppcheck flags it). Revisit only as a deliberate gameplay
change, not as a bug fix.

- **Destroyed forests never reopen their pathing areas** (decided
  2026-11-01). `TerrainObjectType` declares its own `subType`, shadowing
  `ObjectType::subType`, so `TerrainObjectType::handleDestruction`'s
  `getObjectType()->getSubType() == TERROBJ_FOREST` reads the base field
  (always 0) and `openSubAreas()` never runs. Same in the original source.
  Removing the shadow would make destroyed forests passable (and change
  `ObjectManager::getSpecificObjects` for terrain objects). Marked in
  `code/terrobj.h` and at the test in `code/terrobj.cpp`.
- **A same-team jump collision does no damage** (decided 2026-11-03).
  `mech.cpp`'s death-from-above block is gated by `collideeJumping &&
  !sameTeam`, although its comment describes double damage when both mechs
  jump, for every case. Removing the team check would make friendly jump
  collisions hurt. Marked at the test.
- **Fog byte semantics differ between actors and terrain** (original
  behaviour, kept): actors (and crater/gvactor/mech3d) turn "no fog"
  (fogFactor 256) into 0, terrain (`quad.cpp`) uses 0xff. Worth checking
  visually on the Pi.

## Known issues

- Network games from the screens have mech selection for designer variants only (no mech lab changes, no all tech), and no shared insignia, locking or booting, and only the local network: no joining by address (`future-features/NETWORKING.md`). The other screens' stubs still return a blank player record, an empty name and no players instead of NULL.
- Sound: doppler not implemented (alariq README). Stereo panning works since #4 (`fa0188e`); nobody has recorded checking it by ear.
- Some files may still be written outside the user config directory (alariq README TODO).
- WSLg's D3D12 GL driver shows a white screen with desktop GL; use `LIBGL_ALWAYS_SOFTWARE=1`.
- With `MC2_STOP_EXITS=0` (mod bring-up), `STOP` and `Assert` log and carry on, so code after them runs into whatever they were guarding; a missing mission then runs until a `Fatal` ends it.
- `mech3d.cpp` helicopter dust cloud computes a motion-relative matrix and then draws with the plain one, unlike the water wake next to it. May be deliberate; unverified.

## Log

Newest first. `YYYY-MM-DD: what (commit or PR)`.

- 2026-11-11: VR: no gap in the ground at the bottom of the view (branch `vr-terrain-near-gap`, PR 145). The game drops any ground or water triangle with a corner outside the depth range, which the flat camera never meets, but an eye looking down does: the triangles crossing its near plane went missing, a pale gap with straight edges (Quest 2, mc2_16, `MC2_XR_POSE=0,-35`, fog 75). In VR the eyes' terrain and water vertices now keep a signed rhw (`Camera::projectZSigned`; clip w is negative in front of the camera here, which `projectZ`'s `fabs` hid) and the GPU clips those triangles; `vr_edge_fade::nearTriangleDrawn` (unit tested) drops only one wholly behind the eye or past the far plane. Checked on the Quest against PR 141's build: the gap is gone, the rest of the frame unchanged, 67.7 fps against 67.5. Moving the round window towards the camera was tried first and taken out: the captures showed it did not change the gap. Flat game unchanged. The mission fly-in was not captured (`-mission` starts without one).
- 2026-11-08: VR: no hard edges where the world ends, and a round terrain window (branch `vr-round-terrain`). The ground fades out in a circle before the terrain window's edge (`mclib/vr_edge_fade.h`, unit tested; meshes in `scene.hglsl`), since the haze starts about where the window ends. `skybox` is an open ring, and centred on the ground it sat wholly below the horizon, with black above: in VR it goes round the camera and fades into the fog colour at its top and at the horizon, and the eyes clear to that colour. Fog that is pure black (`mc2_05`, `mc2_21`) now fades in VR too. The window's corners, wholly past the fade, are no longer drawn in VR, and `Terrain::isInWindow` is round there too. On the Quest (mc2_19 bench, fog 100 and 155, alternated) the round window cut terrain triangles by only 4 to 6% and changed no frame time beyond noise: most of the corners were already outside the view. Flat game unchanged. First made on 2026-11-04 as `vr-edge-fade`, not pushed then.
- 2026-11-06: README media reshot on the v0.5.0-rc1 Linux package at 1280x800 (branch `docs/readme-media-v0.5`): two clips (mission 17 artillery, mission 19 at night) and three stills; the Omnitech still is kept (no Omnitech install on this machine). The scenarios are now in `bench/media_*.fit` and the recording steps in `bench/README.md`, since the last set's were never kept.
- 2026-11-06: Mech selection in a network game (branch `mp-phase4-screens`): the host can untick Quickstart and set C-Bills and drop weight; each player goes through the briefing, mech bay, purchase and pilot screens, and Launch there sends their force by name (`netwire::Force`, protocol 12). The host checks it against what the mech bay allows (`code/netforce.*`, unit tested), sends everyone's, and each machine loads the map with none of its units and makes everyone's in the same order. `net_smoke --mechs` plays a battle that way on x64 desktop GL, in `smoke.yml`. Designer variants only, one C-Bills amount for all, and no person has clicked through it yet.
- 2026-11-05: Multiplayer from the main menu (branch `mp-phase4-screens`): the original connection, browser and parameters screens run a real game, with the menu's button on. `net_smoke --screens` plays a battle through them, twice in a process with `--again`, and `--lobby-leaving` leaves and rejoins a lobby, on x64 desktop GL, also under ASan + UBSan. Protocol 11. A player's name and colours go through the prefs screen. A failed host, a different map, or the host going in a lobby, while loading or in play no longer strand the player or close the game, and the wait for others to load no longer stops the frames. No person has clicked through them yet.
- 2026-11-05: Multiplayer lobby review fixes (branch `mp-phase4-lobby`): nobody waits for a player lost while loading, a launch reaches everyone it can, a client refuses a launch with no map and a map name that is a path, a ready no longer outlives a settings change, and `-netlobby` works with `-netcall`. Protocol 9. Two on one team and the look of a lobby's colours are still not run.
- 2026-11-06: Lobby gaps after mech selection (branch `mp-lobby-gaps`, on `mp-phase4-screens`). Chat works before a game, and should in it, which no run has tried (`netwire::Chat`, protocol 13): the host sends each line to everyone with who said it, and the 2001 chat window shows it, so the pilot screen's "X is ready to launch." tells the others who has chosen, and the host's "about to start" line tells clients a launch is counting down. A team's line reaches every machine and only that team's shows it. The host can give each player other C-Bills on the parameters screen (`LobbyPlayer::cBills`), and checks each force against that player's own. The data check now covers what the mech bay buys from: the components, the pilot list and each pilot, and each mech's sheet, so a client with other ones is refused when joining. Unit tested; `net_smoke --mechs` checks each side's ready line reached the other's chat, on x64 desktop GL. No person has tried it.
- 2026-11-06: A pilot's message no longer asks for the CD on Windows when the data has no movies (branch `fix/pilot-portrait-no-cd`). With no pilot video the game shows a still portrait from `data/movies/`, and data built with `--no-movies`, as in the `-full` packages, has none, so `mc2_01` on the Windows `-full` package showed "insert the CD", and Cancel ended the game. A missing portrait now draws no picture, as Linux already did. Built on Windows MSVC (Release) only.
- 2026-11-06: Sessions keep two permanent worktrees instead of one per branch (branch `docs/worktree-slots`, `.claude/rules/agent-workflow.md`): a fresh worktree built from scratch, about 11 minutes on Windows, half of it the network libraries. A session claims a pair `../mc2-wt<N>a`/`b` with `git worktree lock`, starts each branch inside a free one so its build stays incremental, and detaches and unlocks them when it ends.
- 2026-11-05: Nightly `smoke.yml` is more exhaustive (branch `ci/nightly-exhaustive`): the rest of the campaign, the other multiplayer maps with `-nethost`, on all three configurations; a 250 ms, 5% loss battle and five repeats of the support battle on the sanitized job. First manual run: x64 GL 50 min, arm64 54, sanitized 74; UBSan found an invalid `bool` at `mech.cpp:5493` in `mc2_12` and `mc2_15`, fixed by #135, which must land first. The smoke job now has a time limit (90 min on PRs, 180 nightly), and the Exhaustive steps skip when the game data did not restore. PR runs do not change.
- 2026-11-05: `BattleMech` read `blockReachedEnd` uninitialised for helicopters, which are mechs with `MoveType = 1` (`moveLevel` 2): `getPathRangeBlocked()` returns early for them without setting it. Same fix as `5d588c9` for ground vehicles (branch `fix/mech-block-reached-end`). Found by the exhaustive nightly smoke in #132: UBSan in `mc2_12` and `mc2_15`.
- 2026-11-05: VR path-finding runs on a worker thread, and a game step waits for it instead of holding the frame (branch `perf/vr-path-worker`). Also meshes skip the per-vertex transform in VR and draw from a list kept until the next step. Quest 2, mc2_19: frames over 20 ms from about 75 to about 11 per run, logic p99 15.6 to 5.8 ms; with full detail and more units (`bench/mc2_19_heavy.fit`) 41 to 71 fps, though the game then steps about 21 times a second. Path-finding is the next limit. Line of sight in `calcMoveGoal` no longer moves the unit (ThreadSanitizer). Details: `planning/future-features/LOGIC-RENDER-SPLIT.md`.
- 2026-11-05: Multiplayer lobby review fixes (branch `mp-phase4-lobby`): nobody waits for a player lost while loading, a launch reaches everyone it can, a client refuses a launch with no map and a map name that is a path, a ready no longer outlives a settings change, and `-netlobby` works with `-netcall`. From the second review: a host stops if everyone else left while loading, a client follows only the launch the state said, a map's hash covers its `.pak`, a map's name is limited to what a map's file is called, the LAN search takes only its own kind of game, a lobby takes the map's team limit, and a host stops at once on a map it lacks. Protocol 10. Two on one team and the look of a lobby's colours are still not run.
- 2026-11-05: Multiplayer lobby protocol (branch `mp-phase4-lobby`): `code/netlobby.*`, three messages and a lobby mode in `netsession`, unit tested on the loopback. `-netlobby` plays a game through it between two processes, the client hearing the map from the host. No screens yet.
- 2026-11-05: `smoke.yml` on PRs is about 10 minutes shorter when multiplayer code did not change (`scripts/ci_multiplayer.sh`, branch `ci/smoke-pr-time`): no game modes, and the host and two-process steps only sanitized. Nightly, manual and multiplayer PRs run everything on all three configurations, as before.
- 2026-11-05: Multiplayer phase 4 begun (branch `mp-phase4`): a host answers on the local network and `-netjoin lan` finds it (`code/netlan.*`, unit tested over a real socket, in `smoke.yml`), and joining compares a hash of the mission, units and rules, not only the mission's name. Not yet run between two machines or on a Quest; the lobby and its screens are still to come.
- 2026-11-05: Stock missions drew roads and shores in solid magenta after a mission without a colour map had run in the same game folder (branch `fix/mc2-11-magenta-road`). The terrain transition cache (`<types>.<overlay>.txm`, `terrtxm.cpp`) is named by types and overlay only, shared by every mission, but a mission with no colour map (its pak named differently from its colour map, as a renamed copy is) saved the old opaque composite under those names, over the stock caches, which every later stock mission quick-loaded. The two universes now have separate names (`mclib/txmcachename.h`, unit tested): a mission without a colour map saves `<types>.<overlay>.old.txm` and reads the plain name only when its `.lst` names plain caches (a mod, or a folder written before this fix, whose composite is right for it). A folder already poisoned needs its `data/textures/*.txm` and `*.lst` deleted (they rebuild on the next run). Checked on `mc2_11`, both renderers, llvmpipe.
- 2026-11-05: Spinning markers no longer cut squares out of explosions (branch `fix/bomber-marker-explosion`): forced in front of the world, their whole quad wrote depth, transparent texels included, so effects drawn later failed the depth test there. Flat desktop showed it as well as VR. They now draw blended with no depth write (`MC2_ISMARKER`), and effects draw over them. Other blended draws that write depth (buildings, fences, effects) hid at most 7 pixels in any tested frame, so they stay as they are. Solid nav markers (nav2 to nav6) get the same treatment, tested by temporarily giving strikes the nav2 model.
- 2026-11-05: Multiplayer phase 3 finished (branch `mp-phase3-rest`): the host holds every commander's resource points and decides support vehicles and recoveries, which every machine plays out; turret targets, pilot kill stats and mine explosions reach a client; a network game ends on the real results screen; every kind of order the interface gives reaches the host in a test. Also in single player: a pilot with a short voice file no longer ends the game on a late radio message, a recovered mech no longer stays starting up, the radio no longer crashes when every queued message's pilot has died, a turret's shot no longer reads past a mech's body locations, and a support vehicle destroyed before its VTOL lands no longer crashes the game. Three new tests in `smoke.yml`, armour differing in no check. Not seen by anyone yet, and not run on Windows or arm64 beyond CI; see "Support, recovery and every order" in `future-features/NETWORKING.md`.
- 2026-11-05: First multiplayer on a Quest 2 (branch `mp-phase3`): a PC and the headset on home Wi-Fi, inside the app, unworn, played a battle with the Quest as host and as client, armour never differing. A client's game step is about 9% cheaper than a host's (3.98 against 4.38 ms median), and neither meets 72 Hz in this battle, mostly the renderer. `-netcall` and `-netjoin wait` let a host in WSL dial a client. A client's unit now also takes an HQ under the interface's order, in `smoke.yml`.
- 2026-11-04: Multiplayer game modes (branch `mp-phase3`): a network game plays the mode its map declares, by rules rebuilt in `code/netrules.*`, unit tested. Each team gets a base on capture base maps, and holding the hill or a territory scores. Each mode reached the expected result between two processes on x64 desktop GL, in `smoke.yml` too. The HQs are taken by the test, not by units under orders.
- 2026-11-04: Multiplayer phase 3, continued (branch `mp-phase3`): a client's hold position, real players and ranks in the in-mission results, team scores and mission script messages reach a client, and a recover order from a client is refused rather than ending the host's game with its `STOP`.
- 2026-11-04: Multiplayer phase 3 begun (branch `mp-phase3`, on `mp-phase2`): artillery, mines, terrain fire, captures, kills and losses and repairs reach a client, and a client can call a strike. In `bench/net_m08.fit` armour never differed and both ends finished with the same kills, losses, buildings and mines. Repairs and captures are staged by the scenario, not yet done by units in play.
- 2026-11-04: Multiplayer phase 2 begun (branch `mp-phase2`, on `mp-phase1`): `-nethost N -netlisten` and `-netjoin` play a mission between processes, with no UI. `code/netsession.*` is the join, load barrier and start. The host sends unit state, weapon fire, hits and critical hits, a client sends orders, checked for ownership and range; the last team standing wins. `scripts/net_smoke.sh` plays `bench/net_m08.fit` to its result, with a check every five seconds: on Linux x64 GCC, on one machine, no live unit's armour differed with no added lag, at 100 ms and 2% loss, or at 250 ms and 5% loss, each way. Also fixed `Gate` and `Turret` reading their type as the wrong class in `getDamageLevel`. Not done: Quest client step, two machines, anyone watching it; see "Phase 2 results" in `future-features/NETWORKING.md`.
- 2026-11-05: VR interface runs at 1280x960, not 800x600, so its text is sharper on the panel (branch `perf/render-stage-timers`, `vr_eye::fitInterfaceResolution`). Judged on a Quest 2; flat games and `MC2_VR_PATHS` runs keep their size. Also from review there: the pointer reads the interface on resting frames too, and a skipped copy no longer loses a step's update.
- 2026-11-05: VR frame timing and two speedups (branch `perf/render-stage-timers`). `-bench` times the parts of the world pass, `xrWaitFrame`, swapchain acquire and `xrEndFrame`: about half of `render_ms` on the Quest is waiting for the frame slot. The VR interface redraws the frame after a game step, not every frame: step frames 15.37 to 14.27 ms, GPU about 6.96 to 6.30 ms. Consecutive quads and triangles with the same states merge: draws 1518 to 1184, `ui_render` 0.86 to 0.45 ms. No gain from fixed foveation, invalidating eye depth, a uniform list or per-eye culling. `drawQuads` read the wrong vertices for more than one quad. Audit of a logic and render split: `future-features/LOGIC-RENDER-SPLIT.md`.
- 2026-11-04: MC2X missions written up as a to-do in `future-features/MOD-COMPAT.md` (branch `mc2x-todo`): get MC2X-CVE-G, run it as its own game folder, and confirm the gaps the remaster's code points to (`mc2xres.dll`, `mc2xcore.abx`, model and briefing data). Docs only; no MC2X data has been run here.
- 2026-11-04: Saved path sizes are checked on load (branch `fix/saved-path-sizes`). `MechWarrior::copyFromData` restored `numGlobalSteps`, `curGlobalStep`, the way point counters, the two `MovePath` counters, `lastGoalPathSize` and the orders' `numPoints` unchecked, and they index fixed arrays. Out of range now drops that data and logs it (`code/movesavebounds.h`, unit tested). The area and door numbers inside a saved path, and its strings, are still trusted.
- 2026-11-04: Two bugs from the open-world study (branch `fix/global-path-pilot-missions`). `GlobalMap::calcPath` treats a route of more than `MAX_GLOBAL_PATH` doors as no route; only a no-op `gosASSERT` stood between it and the callers' fixed arrays. The pilot's mission record no longer writes one past `missionsPlayed`, and a mission id past `MAX_MISSIONS` (a mod's long campaign) is logged and not recorded instead of a `STOP`.
- 2026-11-04: TGA headers are checked against the file (branch `fix/tga-header-check`). `tgaPixelsFit` and `tgaHeaderFits` in `mclib/tgainfo.cpp`; the terrain tile, mask, colour map, height map, icon sheet and waiting-screen readers refuse a file that cannot hold what its header claims, and `tgaDecomp` is bounded by source and image. All 3021 TGAs in `mc2srcdata` pass the check. `readFastRAW` refuses a too-small buffer instead of overflowing it on a Windows retry. `ByteFlag::initTGA` has no callers and is unchanged.
- 2026-11-04: Study, nothing built: `planning/future-features/OPEN-WORLD.md` proposes an open-world mercenary mode (star map, generated contracts, lasting damage, rivals, shops, zones up to 240 vertices, eras) as a layer above the missions, with zones and loading screens rather than streaming. It found `MAX_GLOBAL_PATH` (only a no-op assert guards a fixed array) and an off-by-one in the pilot `MAX_MISSIONS` check, neither fixed.
- 2026-11-04: A missing FST no longer hangs on Linux and the Pi (branch `fix/missing-fst-hang`). The retry loops behind the old "insert the CD" dialogs only exit on Cancel, and the Linux `MessageBoxA` stub answers neither OK nor Cancel, so they spun for ever: a missing archive, a file missing with `tgl.fst` gone too, an archive cut off before its data, and a missing pilot video. They now give up when no dialog was shown. Windows dialogs are unchanged.
- 2026-11-04: Multiplayer phase 1 begun (branch `mp-phase1`): wire format `code/netwire.*` (fixed-width little-endian, range-checked decoders, round-trip and garbage tests) and transport interface `GameOS/gameos/gos_net.*` with a loopback backend for tests. Nothing in the game calls them yet. GameNetworkingSockets v1.6.1 compiles for Android arm64 with OpenSSL and protobuf (7.0 MB stripped), with OpenSSL built in WSL because Git Bash's Perl is incomplete, and a test program over home Wi-Fi between a PC and a sleeping Quest 2 lost nothing in 20 s (round trip p50 17 ms). Then the real backend, `gos_net_gns.cpp`: GNS, protobuf and BoringSSL (BCrypt on Windows) from pinned sources in every build (`cmake/mc2_gns.cmake`, `-DMC2_NET=OFF` to leave out), `net_ping` between two processes in CTest, `scripts/android/net_ping.sh`; PC to Quest over Wi-Fi 600 of 600 messages, round trip p50 6 ms, worst 120 ms. Nothing in the game calls it yet (`NETWORKING.md`, "Android spike"). Fixed in passing: `fetch_deps.sh` could not unpack on a fresh Windows checkout.
- 2026-11-04: Multiplayer phase 0 (branch `mp-phase0`): `-nethost <players>` runs the host's side of the multiplayer code with no network and no clients; `smoke.sh --nethost`, `bench/nethost_m08.fit` (a battle with damage on). `mc2_m01` to `mc2_m10` and the battle clean under ASan + UBSan on desktop GL, single-player state hashes match `pi-port`. Fixed: mission ended on its first frame, full weapon fire queue was Fatal, turret with a neutral parent, `1 << -1` from the force group icons; a host's camera no longer decides mech withdrawals, building updates or turret gestures. Open: the same battle from two camera positions still differs from frame 282 (positions match until one hit outcome, consistent with cosmetic code sharing the RNG, not proven); no run has had a turret fire; withdrawing units can stop short of the map edge on a host; `Team::home` is not audited. No network yet.
- 2026-11-04: Omnitech's 800x600 screens on its larger front end (branch `omnitech-mechlopedia-layout`): the Mechlopedia goes in the middle instead of the corner, and the stray scroll bar frame on its weapons tab is gone (that list took a right and bottom as its width and height, which on stock fell off the screen); the main menu's starfield behind a dialog is centred, as the splash already was; the results screens (salvage, pilot review), which have no larger layout on any data, run at 800x600. Checked on every walk screen by eye.
- 2026-11-04: Deferred bugs (branch `omnitech-deferred-bugs`): no display or no renderer now ends the game with a logged reason (it segfaulted); Omnitech's shop and mech bay icons read its 1024-wide sheet by that sheet's own 40x48 cells, 25 a row; Mechlopedia finds Kitfox and Direwolf, whose variants are spelled differently (all 56 rows open); Omnitech's portraits at 640 and 800 wide come from its one high sheet; its KIA box leaves out the dead picture, as that index is another pilot's face; the briefing releases its map before a mission flushes textures. A sweep of Omnitech's data then found the same causes elsewhere: the string lookup took the game table's "undefined" placeholder as text and never asked `mc2res.dll`, so now real text in the DLL replaces a placeholder (all part, mech, copter and vehicle names and descriptions resolve; 32 parts were unnamed, 39 of 55 mech descriptions were placeholders); the mission HUD's 640 and 800 strips stop short of the high one, so they gain its extra cells at load; the shop sheet's art stops at 102 of 127 mechs, so the rest are copied from the HUD sheet once per texture load; the strip and fill logic is in `code/iconsheet.cpp` with unit tests; unnamed buildings get no Mechlopedia row. From the review: a first launch with no display also ended in a segfault and now exits with a logged reason, and pilot ready's map is released with the briefing's, so it stays on screen while the screen slides away. Not fixed: the Mechlopedia draws at 800x600 in the corner of Omnitech's 1280x1024 front end, with a stray scroll bar frame on its weapons tab; hire pilots still has no handler.
- 2026-11-04: Mission HUD centred at any width (branch `hud-centre`). `buttonlayout1920.fit` has 1600's `xOffset`, so at 1920x1080 the HUD sat 163 pixels left of centre, and worse on wider screens; other widths took that file too. `mclib/hud_layout.h` (unit tested): the largest layout that fits, centred from 1024 up, the pilot video window kept on the right edge. Omnitech, with no 1920 layout, stopped at 1920x1080; it now steps down to 1600. 1280x720 and 1600x900 are pixel-identical to before. Checked on GL at 1366, 1920 and 2560 and on GLES at 1920; clicks and the video window by hand (`HARDWARE-TESTS.md`). README images reshot at 1920x1080. Scaling the HUD for 1440p and 4K is not done.
- 2026-11-04: Full release packages, game data included (branch `release-full`), as approved by the owner: jobs `gamedata` (cache shared with `smoke.yml`, no movies) and `full` in `release.yml`, `scripts/package_full.sh`. The Linux x64 and Pi packages are smoke-tested unpacked with only the runtime libraries; first time a test runs what players download. The Windows zip needs a hand check (`HARDWARE-TESTS.md`). The new test found `INSTALL.md` missing `libopengl0`, so the Linux package would not start on a clean machine; fixed.
- 2026-11-04: Multiplayer phase 0 (branch `mp-phase0`): `-nethost <players>` runs the host's side of the multiplayer code with no network and no clients; `smoke.sh --nethost`, `bench/nethost_m08.fit` (a battle with damage on). `mc2_m01` to `mc2_m10` and the battle clean under ASan + UBSan on desktop GL, single-player state hashes match `pi-port`. Fixed: mission ended on its first frame, full weapon fire queue was Fatal, turret with a neutral parent, `1 << -1` from the force group icons; a host's camera no longer decides withdrawals, building and turret updates, or turret and vehicle gestures. Open: the same battle from two camera positions still differs from frame 282 (positions match until one hit outcome, consistent with cosmetic code sharing the RNG, not proven); no run has had a turret fire; withdrawing units can stop short of the map edge on a host; `Team::home` is not audited. No network yet.
- 2026-11-04: CI moved to cppcheck 2.19.0 (built from source and cached, `.github/actions/cppcheck`), with 2.13 kept in the nightly run. Of 2.19's 1,201 findings, 1,165 are two checks turned off in the gate with the reason (`dangerousTypeCast`, `nullPointerOutOfMemory`); the rest fixed: `FastFile::create` and makefst no longer use a failed `fopen`, 12 owning classes have copies deleted, ignored `readIdULong` results in gate and turret, `move.cpp` cast rewritten because 2.19 could not parse the file. Baseline empty under both versions. `nullPointerOutOfMemory` triaged: a corrupt FST entry count no longer crashes (#112).
- 2026-11-04: Plan for macOS on Apple silicon: `future-features/MACOS.md` (branch `macos-plan`); nothing built.
- 2026-11-04: Multiplayer proposal (`future-features/NETWORKING.md`, replaces the feasibility study; branch `docs/multiplayer-proposal`): host-authoritative, keeping the legacy gameplay branches and chunks, GameNetworkingSockets subject to an Android spike, same-room Quest play needing no alignment until a shared table, head-to-head and co-op on one mechanism. Nothing built or run.
- 2026-11-04: on-disk struct sizes pinned at compile time (branch `test/struct-sizes`, `code/ondisksizes.cpp`): 19 save-game structs per data model (Linux/arm64 and Windows differ, `long`), 10 game-data structs to one size everywhere (they match on MSVC too, so game data really is portable), and the 11 object save structs must have distinct sizes, because `ObjectManager::load` tells types apart by size alone. `check.sh` no longer fails a new `.cpp` on findings in headers it did not change: it lists them as a warning.
- 2026-11-04: Omnitech's splash on its larger front end (branch `omnitech-splash-size`): the 800x600 art is centred on black, where the screen behind it showed round the edges. Stock's draw calls are unchanged.
- 2026-11-04: README: fourth screenshot, idle mechs among mission 22's buildings (branch `readme-town-shot`).
- 2026-11-04: README: Quest 2 as playable, screenshots and a clip in `media/` (scripted battles on the CI-built Linux package, llvmpipe, 1080p; Omnitech at 1280x1024). `future-features/EASY-INSTALL.md`: proposed one-download packages with the game data, and a helper for Omnitech, whose EULA grants no redistribution (branch `release-apk`).
- 2026-11-04: Quest APK on releases (branch `release-apk`): job `quest` in `release.yml` builds the APK on Linux with `scripts/android/build.sh`, and `scripts/android/package.sh` adds the shaders and licences (`-files.tar.gz`) and unstripped libraries (`-symbols.tar.gz`). `MC2_VERSION` sets `mc2.log`'s version and the APK's versionName. Signed with the optional `QUEST_KEYSTORE` secret, so updates can install over each other. Quest steps in `dist/INSTALL.md`; headset check in `HARDWARE-TESTS.md`.
- 2026-11-04: Android prefers SDL's AAudio driver (branch `android-aaudio-default`), then OpenSL, then the Java one; `SDL_AUDIODRIVER` still chooses. A steady buzz at about 1 Hz came in late in long Quest sessions on OpenSL. A capture of the game's own mix from such a session was clean by ear and showed no 1 Hz pattern, so the fault is after the mixer. One session of about 31 minutes on AAudio had no buzz, where the buzz had been heard about that far in on OpenSL. Not proof: the cause on OpenSL was not found, and longer sessions are in `HARDWARE-TESTS.md`.
- 2026-11-02: Omnitech front end (branch `omnitech-frontend-labels-mech`): no `undefined_string_sebi` (id 0 is no text), the mission map in pilot ready's white box, its mech view covers the right area, and in VR an interface camera's mech takes the CPU path (it drew as a solid colour in the wrong place, on stock data too). `pilotready+mech` walk step, `MC2_UISMOKE_HOLD`. Checked flat on GL and GLES and with `MC2_VR_PATHS=1`; headset check in `HARDWARE-TESTS.md`.
- 2026-11-03: Omnitech keeps a pilot once a mission has offered them (branch `omnitech-keep-pilots`): Exodus and MW2 Mercs name a pilot in one mission's purchase file and not the next, so from Exodus' second mission the pilot list was empty. On Omnitech data `updateAvailability` also marks the pilots of the missions played, worked out from the campaign, so a load gets the same list. The other eight campaigns repeat their roster and do not change; stock does not take this path. That pilots stay is inferred from the data: the public source is older and clears them, and hire pilots may be what the later releases do.
- 2026-11-03: UI walks of MechCommander Omnitech's campaigns (branch `uismoke-pack-campaigns`): `campaign+pick` picks a campaign by file name (`MC2_UISMOKE_CAMPAIGN`); `scripts/walk_campaigns.sh` walks the first mission of all 11 add-on campaigns from the new campaign dialog through purchase, the mech lab, launch, a win and the results, plain and under ASan + UBSan. Found: Clan Wars could not be started (empty shop, no mechs): on Omnitech data a purchase file now sells one variant whatever it holds (variant 0 or its `MechVariantN`); stock keeps its component check. The mech lab always lists the mech's own variant. Accepting the mech lab with no change left the lab holding a freed variant, on stock too (it only crashed from the walk's driver; no player route found): fixed, and in default walk 2. A new slide between logistics screens now stops one still running (a fast second move read past `screens`). Walks can repeat a screen for another action.
- 2026-11-03: bugs noticed during the cppcheck triage (branch `fix/noticed-bugs`): mech bay drops on the mech list no longer count as over the force group (`rectIntersect` compared x with y); `SalvageListItem` deletes its icon; `getBestPilot` no longer reads a NULL pilot when none is free; `aAnimButton`'s scaled render uses the right size (no stock data reaches it). Death-from-above kept as shipped, by decision. GCC desktop GL locally; the rest on CI; the mech bay drag needs a person (`HARDWARE-TESTS.md`).
- 2026-11-02: First long Quest playtest, on the `v0.3.0-rc1` build plus the sound path fix (branch `quest-playtest-fixes`), mc2_01 to mc2_03. Fixed: music and mission voices were silent on Android (a relative path; heard working on the headset). Video sound now goes through the mixer's music hook, since SDL on Android has one output (measured on desktop GL with the disk audio driver; not yet heard on the Quest). Mech lab drags use the press point, and the other hand no longer takes over the pointer mid-drag. In VR, save and variant dialogs start with an unused name, so OK works with no keyboard. A cyclic buzz in the last quarter of mc2_03 is not explained: the second audio device or an underrun at full load are candidates, and this build changes both. Frame rates from the headset's own log: mc2_01 median 61, mc2_02 55 (both effects off), mc2_03 51 (shadows and high detail on for at least its last part); different missions, so not a clean comparison.
- 2026-11-02: Player log (branch `log-file`): a player's run writes `mc2.log` in the config folder (the game folder where no `HOME` is set), keeps the last run as `mc2.previous.log`, names the version and start time, and still shows it all in the terminal; `MC2_LOG_FILE=0` turns the file off. A crash adds `CRASH:` lines: the signal or Windows exception and the stack as module plus offset, for `addr2line` with the symbols package. Automated runs are unchanged. The Quest already wrote `mc2.log`: it now keeps the previous one and adds the same lines. Linux x64 run under Xvfb (a sent signal and a real fault in another thread); the Android game builds; Windows only built by CI. Not run: Windows, the Quest, a stack overflow. On Windows, `abort()` and fast-fail end the process without passing through the handler, so they add no `CRASH:` line.
- 2026-11-02: VR button map (branch `vr-button-map`): held while the other hand clicks, X run, Y jump, left stick click force fire, B waypoints (it never was force fire: Ctrl is the game's waypoint key); right stick click selects the visible units. Gives up Shift and Alt. Quest 2: reported all good by the user, with #89 merged in, double-pulling the trigger included. `cppcheck_gate.py` now skips files the build does not compile, instead of failing when one is all a change touches. From review, not yet run in the headset: B + pan pans in VR instead of rotating the light, and the command buttons send keys only while a mission takes orders.
- 2026-11-02: VR start pose and steady overlays (branch `vr-start-pose`). Straight ahead is where the head faces once the headset is on and the head holds still (`alignToHead`, `XR_EXT_user_presence`, `vr_controls::headSettled`), not LOCAL's forward (136 degrees off on one launch). Overlays in the eyes are sub-pixel and a fixed size at any game resolution; text is filtered. A saved `ResolutionX` above 1920, which has no interface layout and shrank the HUD into a corner, falls back to 800x600 in VR. Buildings now stop the VR pointer's ray (`Appearance::worldRayHit`), so hover names and clicks work on them.
- 2026-11-02: VR mesh path: fewer driver calls per draw (branch `perf/vr-mesh-driver-calls`): states and material once per node, program, buffers and attributes left bound between draws, one GL error check per pass. mc2_19 on the Quest 2: `world_render` 5.79 to 5.50 ms, p95 16.4 to 16.1 ms, fps unchanged at 70.0. Byte-identical on desktop GL, flat and `MC2_VR_PATHS=1`. Numbers in `future-features/VR-TABLETOP.md`.
- 2026-11-02: VR mesh path: 3.4 ms of `world_render` a frame back on the Quest 2 (branch `perf/vr-light-lookup`): each shape's lights as three per-draw uniforms instead of a 200-entry block (a `memcmp` search per shape per eye, 16 KB `glBufferData` per eye), and material, uniform and sampler lookups once per node instead of by name per draw. mc2_19: `world_render` 9.3 to 5.95 ms, fps 68.8 to 70.2, p95 18.5 to 16.5 ms; mc2_01 fly-in 8.2 to 3.8 ms. Byte-identical screenshots on desktop GL, flat and `MC2_VR_PATHS=1`. New `[MESH]` warning at the first node if the shaders on disk lack a uniform the build sets. Numbers in `future-features/VR-TABLETOP.md`.
- 2026-11-02: performance check of `pi-port` after the VR series (branch `docs/vr-perf-check`, measurements only): flat game on desktop GL (llvmpipe, mc2_19, 3600 frames) at head against the pre-VR point `d2602ee2` and `4fe401d4`: same draws, triangles and state as `4fe401d4` on every frame, frame time within the noise of a loaded machine (16.3 vs 17.2 ms best of six, so at most about 5 %), and faster than pre-VR (logic 10.0 to 4.3 ms, the path fix). Quest 2 (mc2_19 and the mc2_01 fly-in, head pose fixed, builds alternated): head 68.7 to 69.0 fps against 69.3 to 70.0 before #84, median 14.0 ms at the 72 Hz cap either way; `world_render` up 2 ms a frame (7.1 to 9.3), all of it from the lighting fix in #84, none from the FOV change in #86; game-step frames are now 3.7 to 4.4 ms of logic (the path fix), p95 27 ms. Numbers in `future-features/VR-TABLETOP.md` ("Lighting on the GPU path").
- 2026-11-02: VR game camera follows the head (branch `vr-head-camera`, `future-features/VR-HEAD-CAMERA.md`): its matrices, `cameraFrame` and `lookVector` turn with the head once per game step, held through a click or drag; its yaw and pitch stay the game's, for the sticks, sound and eyes. So far ground at any tilt can be picked and what the player turns to runs full logic. `MC2_XR_FOLLOW=0` turns it off; `MC2_VR_PATHS=1 MC2_XR_POSE=yaw,pitch` tests it on a desktop. The minimap's wedge meets the ground in VR (it was wrong at #86's view). Found: turrets update every step whatever the view. First Quest 2 run: picking good, 25 of 27 presses valid, and following costs no measurable frame rate (mc2_19 bench 68.5 to 69.0 fps at either window size, on or off). mc2_01 played by hand falls to about 40 fps with following on or off, cause not found.
- 2026-11-02: VR game camera's field of view (branch `vr-fov`, `future-features/VR-FOV.md`): in VR the game camera takes the headset's view (`gos_GetVRFieldOfView`, from the eyes at the window's aspect, at most 120 degrees; `MC2_XR_FOV` sets it), so picking and terrain objects' game logic reach what the player sees, not 40 degrees. Far picks needed three fixes: a beam's orientation (`XR_ERROR_POSE_INVALID`), the ground stopping at the terrain window, and vertices behind the game camera no longer counting as in view. Flat game unchanged. Quest 2: picking works near the horizon and past the terrain's edge; mc2_19 bench about 70 fps at 40 degrees and at 115.6, the game step 0.1 ms longer. Not checked: a turret at the side of the view, overlay sizes.
- 2026-11-02: jump-capable movers could never path through a door on the top or left edge of their move map (branch `fix/jump-path-bounds`): `calcPathJUMP` and `calcEscapePath` tested successors with `> 0`, so row 0 and column 0 were off limits while `setGoal` puts door cells there; the search took every cell in its box off the open list, failed, and the mover asked again next frame. In the mc2_19 battle that was 84% of all path searches (12,815 of 15,323, 1,358 nodes each) and 92% of path time. Fixed (`>= 0`): 5,434 searches, paths 3.25 to 0.62 ms per frame (best of 3, llvmpipe), logic 5.8 to 4.1 ms; AI jump mechs now move through those doors, so the game state differs from before (deterministic across runs). `[PATHS]` at exit counts searches, found, failed and open-list nodes. Found while trying two exact optimisations that did not pay and were dropped: an octile, cost-scaled A* heuristic (4.3x fewer nodes on quiet maps, 1.07x in the battle, 17 of 15,270 paths dearer at multi-cell door goals) and a flood-fill reachability pre-check (1.0x).
- 2026-11-02: VR blending on the GPU mesh path (branch `vr-blending`): alpha to coverage for cut-outs on the eyes' MSAA; the compass and fading shapes in blended hardware passes, where the CPU path draws its own. Also: a mesh node's first draw used the previous node's shader variant. Flat game byte-identical (mc2_19, 3600 frames: screenshots, draws, tris, state checksums). Desktop `MC2_VR_PATHS=1` and Quest 2 checked: compass and tree edges right, fly-in frame time unchanged (69.4 fps). Also on that path: light slots and the shader's light block (near trees were black, on `pi-port` too), Quest 2: near trees in colour, the step gone. Open: distant chain-link fences solid (mipped alpha), a real fade not watched, frame time not re-measured after the lighting fix.
- 2026-11-02: VR missing buildings (branch `vr-missing-objects`): the GPU mesh path drew multi-texture shapes with the first triangle's texture, and in VR the draw reservation and the draw disagreed on the alpha-test flag, so the texture manager dropped the draw. One draw per texture, flags from one helper, GPU buffers for ASE-parsed shapes. Also: the GPU path never set the alpha-test render state, so alpha textures drew their transparent texels black (trees as black boards, hut roofs, sandbags); one `gos_SetRenderState` in `renderLists` fixes it. `MC2_VR_PATHS=1` (with mipmaps) and `MC2_SHOT_FRAME=n` reproduce VR render bugs on the desktop. Flat game byte-identical. Open: the GPU path cannot blend (hard edges, no translucent compass).
- 2026-11-02: faster A* pathfinding (branch `perf/logic-jobs`): new `-bench` stages (`interface`, `paths`, `sensors`, `collisions`, `script`, `txm_update`; `kMaxStages` 16) show the path manager is about 60% of the logic step in the mc2_19 battle. `PriorityQueue::find` keeps an index of queued ids instead of scanning the heap (falls back to the scan for ids past `1 << 20` or queued twice; unit tested against the scan; the jump and escape searches now use the cell index as the id, so the index is dense), the path heuristic and line of sight no longer read the clock per call outside `LAB_ONLY`, and `insert` no longer writes one node past a full queue. llvmpipe, 4 interleaved rounds: paths 4.70 to 3.78 ms best (median 4.74 to 4.17), logic 8.02 to 6.94 ms; game state identical on every frame. A 24-byte map node (`adjCells` split out) gave nothing more on x86 and was dropped.
- 2026-11-01: VR world picking (branch `vr-controllers`, #77): through the HUD's see-through parts (window alpha read back a frame late, without waiting) the controller ray picks in the world: placed with the game camera as the eyes are (`Camera::vrRayToWorld`), tested against spheres about the movers and then the ground (`vr_controls`, unit-tested), and the hit projected through the game camera becomes the mouse, so the game's own selection and orders work. Only what the game camera sees can be picked. In the headset: "starting to be playable" after two fixes found with the pick logged (`MC2_XR_PICK_LOG=1`): the ray went up (the camera looks along minus its forward row), and with nothing to point at the mouse fell back to SDL's 0, 0, so the map scrolled to its corner by itself; 42 of 45 world clicks then valid, the misses outside the game camera's view.
- 2026-11-01: VR controllers in the headset (Quest 2, mc2_01): panel clicks and sticks work. The mouse cursor gave way to a VR pointer: a beam from the hand and a dot on the HUD, as OpenXR quad layers; the game draws no cursor in VR (`gos_VRPointerActive`). Clicks missed because trigger wobble turned them into drags: the pointer now holds still through a click (`holdForClick`). Vanishing mechs while panning were the object-culling gap, fixed by #79 (branch `vr-controllers`, #77).
- 2026-11-01: VR Phase 3, first slice (branch `vr-controllers`): Quest Touch controllers through OpenXR actions, fed to GameOS as a virtual mouse and keys (game input code unchanged). Aim ray on the interface quad is the mouse, trigger / grip the left / right button, left stick pans, right stick turns and zooms, B/X/Y hold Ctrl/Shift/Alt, menu is Escape; A recentres the HUD and, held, moves and sizes it (`MC2_XR_HUD`). Fixed: with the game stepping at 30 Hz, input was sampled every headset frame and most clicks were lost. Ray and stick logic unit-tested (`vr_controls`); desktop GL build, unit tests and the APK build checked; not yet run in the headset. Picking in the world is next.
- 2026-11-01: VR selection markers (branch `vr-markers`): brackets, health bars, icons and labels re-projected per eye (`Camera::overlayToEye`: the object's game-camera projection back to its world point, through the eye, offsets scaled by the eye's size at that depth; `Appearance::toEye`). Quest 2 captures: bars on the units as the head turns. Flat game byte-identical.
- 2026-11-01: MechCommander Omnitech campaign packs (branch `mco-campaign-packs`): the eight not yet tried (Mercenaries Star, Volstand, Laskos Legion, Day of Heroes, Clan Eagle, Wolf Dragoons, Carver V original and modified) downloaded, MD5-checked, unpacked with innoextract; all 144 missions pass a 300-frame smoke (`MOD-COMPAT.md` item 13). `smoke.sh --missions` takes `|`-separated names with spaces and passes them quoted, and fails a run whose started mission is not the one asked for (a name cut at its space had run another mission and passed).
- 2026-11-01: VR draw-only cull, objects (branch `vr-object-cull`): in VR objects cull by distance only (`Camera::drawsAll`; `MC2_DRAW_ALL=1` in a flat game leaves the state checksums unchanged on every frame), terrain objects get draw-only block and vertex flags with an appearance-only update (`updateDrawnAppearances`), gameplay blocks unchanged. The GPU mesh shader no longer mirrors geometry behind the camera (`-rhw`, not `abs`). Quest 2: objects everywhere, 66.0 fps.
- 2026-11-01: UI smoke, slice 3 (branch `uismoke-tutorial`): the tutorial. `campaign+tutorial` picks it in the new campaign dialog; its missions launch from mission selection and a win goes straight back there. New default walk 3 wins tut_01 to tut_04 and launches tut_05, on stock and Omnitech.
- 2026-11-01: PR 73 review follow-ups: mouse picking keeps the game camera's own tile set (a new `calcThisFrame` bit; with the whole window drawn, tiles behind the camera could be picked), VR render paths keyed on `gos_IsVR()` at the point of use instead of switches flipped on the first VR frame, `-bench` in VR documented (rows are headset frames). Terrain behind an eye checked: the depth test drops it.
- 2026-11-01: VR draw-only cull, terrain: in VR the whole terrain window is projected and drawn (while `gos_IsVR()`) while active object blocks keep the game camera's view; no frame-time cost, flat game byte-identical (llvmpipe screenshots and state checksums). To do: objects outside the game view, the visible edge of the terrain window.
- 2026-11-01: VR on the Quest 2: the game steps at 30 Hz while the headset draws every frame (`MC2_XR_LOGIC_HZ`; each step gets the time since the last, so game speed is unchanged): 47.6 to 67.3 fps, median frame 13.8 ms; frames with a step still take 21 ms. Head motion smoother, units fine (checked in the headset).
- 2026-11-01: VR Phase 2 on the Quest 2 (branch `vr-openxr`): OpenXR session on SDL's EGL context (`gos_xr`), head-tracked eyes drawn from the head pose into their own swapchains (`vr_eye`, unit-tested; level horizon by default), the interface as a see-through HUD quad (no world drawn in the window), meshes on the GPU path in VR (its lit shader fixed to link on Adreno), 4x MSAA and mipmaps in VR. Verified in the headset: world fixed, stereo fuses, HUD frames the battle. 47.6 fps (logic 11 ms, render 9.4 ms); culling is still the game camera's.
- 2026-11-01: UI smoke, slice 5 (branch `uismoke-slice5`): walk screens for the Mechlopedia's six tabs, the main menu from a logistics screen and back, saving the campaign (`save+name`, into the walk's temporary config directory) and loading it again, and the mission's options screen (Ctrl+O). Default walk 1 does all of them: 34 screens on stock, 33 on Omnitech; both default walks pass on both. The walk found a heap overflow under ASan: `EString::Compare`'s case-sensitive branch read the other string's length from a shorter one (the Mechlopedia's pilot list sorts names that way); fixed, with a unit test (`tests/unit/test_estring.cpp`, `estring.cpp` now in the test target).
- 2026-11-01: First Quest 2 run (branch `quest-args-path`). The test build ignored `mc2args.txt` (SDL resolves a relative path against internal storage, not the current directory): read by full path. Then 2 to 5 fps: `simpleperf` showed the Adreno driver allocating kernel memory for every `glMapBufferRange` with `GL_MAP_INVALIDATE_RANGE_BIT`; dropping the bit gives 58.6 fps mono and 52.1 fps `-stereo 40` (logic 12.9 ms, render 3.8 / 6.0 ms). llvmpipe screenshots and state checksums unchanged. APK marked `profileable` for `simpleperf`. Go/no-go: go on the CPU route.
- 2026-11-01: WAV chunk walk reads chunk headers with `memcpy` (`mclib/waveparse.cpp`, split out of `soundsys.cpp` with unit tests): RIFF pads chunks to even sizes only, so MechCommander Omnitech's sounds put chunk headers on 2-byte boundaries, which UBSan flagged as misaligned DWORD loads. The RIFF pad byte was already skipped. Omnitech UI walks now run clean under ASan + UBSan (branch `fix/wav-chunk-align`).
- 2026-11-01: First release candidate tagged, `v0.2.0-rc1` on `80f38884`: `release.yml` made the draft pre-release. Releases start at 0.2 because upstream already has `v0.1.0` to `v0.1.3`, all of which `pi-port` contains. README brought up to date: release packages, the empty cppcheck baseline, VR phase 1 slices and the Quest test build.
- 2026-11-01: `3rdparty.zip`: zlib 1.2.3 (2005) replaced with 1.3.1, built from the official `madler/zlib` tag (`51b7f2ab`) with its `win32/Makefile.msc` on a GitHub `windows-2022` runner (x64 and x86, same file names; `zlib.lib` is what `mc2.exe` links). The Windows package drops `zlib1.dll` and `SDL2_ttf.dll`, which nothing in it loads (branch `release/prep`).
- 2026-11-01: Release phases 1 and 2 (branch `release/prep`): `scripts/package.sh` builds the Pi, Linux x64 and Windows packages (stripped, symbols separate), `dist/INSTALL.md` and the third-party licences fetched at the shipped versions, `build_gamedata.sh --tools-dir` (and a package's own `tools/` by default), `release.yml` (draft release from a `v*` tag; dry runs by hand and on packaging PRs). Tried in WSL: a Linux x64 package built its own game data and passed mc2_01, mc2_03 and both UI walks from the unpacked folder. Found: `3rdparty.zip`'s zlib was 1.2.3 (2005), linked statically into `mc2.exe`; replaced with 1.3.1 the same day.
- 2026-11-01: VR Phase 1, third slice (branch `vr-overlays`): world-anchored overlays per eye under `-stereo`. Brackets, bars, icons and select boxes move by each eye's parallax (a constant per eye, measured in `Camera::renderWorld`, times the object's 1/depth; `Camera::eyeParallax` and `overlayShiftX`, 0 outside an eye's pass, so picking and culling are untouched); floating text added during an eye's pass is drawn in that eye's half (`FloatHelp::renderAndResetFrom`). Overlay shift between the halves 0 px before, -15 to -30 px after, matching the scene; mono byte-identical, state checksum unchanged (GL, llvmpipe, mc2_19). GLES built and unit-tested, not run.
- 2026-11-01: cppcheck baseline empty: regenerated by CI after #57, #58, #60 and #64 (`static.yml` `update_baseline` run 41321227051 on `9fd90383`), 149 -> 1; the last one, `uninitvar vertices.rhw` (reported in `addVertices`), was the text renderer: `addCharacter` never set `rhw` on its four glyph vertices before copying them into the vertex buffer (`gos_text.vert` ignores it, so nothing changed on screen); now `1.0f`, so the committed baseline has no entries. The cppcheck gate also prints where each new finding is (`at path:line`), which is how it was found.
- 2026-11-01: UI smoke, slice 4 (branch `uismoke-results`): walks go on from `mission` to the results: the driver wins the mission (as the Ctrl+Alt+Shift+W cheat), then `salvage`, `pilotreview` and the `promotion` dialog (`promotion+skill`), back to `missionselection`. Default walk 1 wins stock mc2_01 and mc2_02 (pilot review; salvage, a promotion, pilot review); on Omnitech its first campaign mission (salvage, pilot review). Both default walks pass on stock and Omnitech.
- 2026-11-01: MechCommander Omnitech's stock dialogs centred (branch `omnitech-centre-dialogs`): on the 1280x1024 front end, the dialogs Omnitech keeps from stock (single mission, new campaign / load / save, prompts, legal, variant save, map info) move from the top left to the middle (`LogisticsDialog::centreOnFrontEnd`, with their list boxes); stock data is unchanged. Default walks pass on stock and Omnitech.
- 2026-11-01: Deterministic smoke runs (branch `test/deterministic-smoke`): `-deterministic` gives an `-autoexit` run `-bench`'s fixed step and seed without the CSV (sound off, as for `-bench`: audio runs in real time and feeds the RNG), and GameOS prints `[STATEHASH] <hex>`, the game-state checksum after the last frame; the bench director now reports that checksum on every fixed-step frame, scenario or not (so plain `-bench` CSVs get a state column too). `smoke.sh --deterministic` runs each mission twice and fails when the end states differ. Ubuntu 26.04: the five CI missions at 3000 frames, twice each, on GL and GLES: 10/10 pass, and every mission ends in the same state on both renderers. Not the default (plain smoke keeps sound covered); `smoke.yml` replays mc2_10 at 1000 frames this way on x64 desktop GL.
- 2026-11-01: Meta Quest test build (branch `quest-test`), for the VR go/no-go: `scripts/android/fetch_deps.sh` (SDL2 and SDL2_mixer sources, SHA-256 pinned) and `scripts/android/build.sh` (NDK r28c, CMake 3.31.6, then the APK with the SDK's own tools, no Gradle) give a debug-signed `android/mc2-quest-test.apk`; `scripts/android/run_bench.sh` installs it, pushes the data, runs mc2_19 mono and `-stereo` and pulls the results. Under `ANDROID` the game is `libmain.so` for SDL's activity; `android_prepare` (gameosmain.cpp) moves to the app's files folder, sets `HOME`, reads the command line and leading `NAME=value` settings from `mc2args.txt` (`utils/args_file`, unit tested) and logs to `mc2.log`; `libmc2res_64.so` is loaded by name. Clang on arm64 also needed: no x86 `rdtsc` asm off x86 (`gameos.hpp`), the standard placement new instead of a redeclaration (`stuff/style.hpp`), and a map name no longer used as a format string (`logisticsdialog.cpp`). Built on Windows (APK verified: arm64, GLES 3.1, v3 signature); desktop GL and GLES still build and pass on Linux. **To do: run it on a Quest.**
- 2026-11-01: cppcheck baseline, the rest after `nullPointerRedundantCheck` (122 findings, three PRs): `allocaCalled` (27, branch `fix/cppcheck-alloca`: unbounded `A2W` stack buffers, also too small for Linux's 4-byte `wchar_t`, and the objectives HUD dots now on the heap; 7 bounded uses suppressed); copy / assignment (34, branch `fix/cppcheck-copyassign`: `mouseY = mouseY` typo, three self-assignment hazards, `aButton::operator=` copies its sounds like the copy constructor, 8 never-copied owners made non-copyable); the rest (61, branch `fix/cppcheck-misc`: shader `#include` on the last line read address 1, ABL `setcurrentbrvalue` never popped its arguments, six bounds checked before indexing, the pmesh lightmap V test (`&&` never true), unset text-background `rhw`, signed shifts and punning; dead code removed, false positives suppressed). New "Kept as shipped" list: forest pathing, fog bytes (`comparePilots` was fixed instead, #64).
- 2026-11-01: `LogisticsData::comparePilots` tested `p1` outranking `p2` twice, so a lower-ranked pilot fell through to the gunnery test and could be auto-assigned ahead of a higher-ranked one. Rank now decides first, as the gunnery and piloting pairs below it do; this changes auto-assignment from the shipped game (decided 2026-11-01). cppcheck `identicalConditionAfterEarlyExit`.
- 2026-11-01: VR Phase 1, second slice (branch `vr-phase1b`): shadows per eye under `-stereo`. `TG_Shape::RenderShadows` projects the shadow vertices' world positions for the drawing camera (the update used to, once per vertex as now). Mono with shadows on byte-identical, state checksum unchanged with `-stereo` (GL and GLES, llvmpipe, mc2_19 with `Shadows` and `UseLocalShadows` on; CI smoke runs with shadows off, so it does not exercise this path). Clouds need nothing: the cloud layer is never drawn. Also: the VR go/no-go drops the Pi bench (a poor proxy for the XR2) for a sideloaded Quest 2 test build plus a PC run on a real GPU.
- 2026-11-01: `planning/HARDWARE-TESTS.md`: one checklist of the tests CI cannot run (in the game, the `STOP` message box, the Pi, releases), gathered from this file, the `STOP` and VR plans and #43 / #53.
- 2026-11-01: cppcheck `nullPointerRedundantCheck` cleared (branch `fix/cppcheck-nullptr-redundant`): 25 sites with cppcheck 2.19 after #46 made `Fatal()` `[[noreturn]]` (27 in the baseline regenerated by #54). Five real bugs: `MechListBox::AddItem` dereferenced a `dynamic_cast` before its NULL check; `TerrainObject::calcSubAreas` read `cellsCovered` after a failed `Malloc` (now a `Fatal`); `MechWarrior::orderPowerUp` / `orderPowerDown` used the vehicle before checking it (now "done", nothing to power); `MechWarrior::setup` checked `new` after using it. Redundant checks after an earlier unconditional use removed (mech appearance, force-group icon, mech lab variant, mission interface, MP stats, terrain colour map heap, texture manager vertex lists); two false positives suppressed with reasons. Ubuntu 26.04: GL and GLES builds and unit tests; MSVC, Clang, arm64 and smoke left to CI.

- 2026-11-01: Two leads from the remaster sweep (branch `fix/sweep-leads`): `parse_uniforms` logs and skips a uniform type outside `GL_FLOAT_VEC2..GL_FLOAT_MAT4` (a `sampler2DArray` indexed past `constantSizes`; latent, no shader has one), and the graphics options list each resolution once instead of once per refresh rate. Both renderers built and unit-tested on Ubuntu 26.04; the options screen not checked in the game.
- 2026-11-01: README and repository description reframed: a maintained update of the MC2 engine for Linux and Windows that also runs on the Raspberry Pi 4/400, with Meta Quest 2 VR in development. Consistency pass: `CLAUDE.md` intro and CI/testing lines, `testing-conventions.md` (sanitizers and PR smoke were listed as still to come), `RELEASES.md` framing, a duplicated "Built" line here; README drops the fixed missing-mission crash and the "nothing built" VR line.
- 2026-11-01: cppcheck `duplInheritedMember` (41) triaged (branch `fix/cppcheck-dupl`): `Camera::destroy` virtual; the accept-variant dialog deletes the variant its prompt names (latent: no delete button in stock data); dead duplicates removed; 28 intended cases suppressed inline with reasons; `TerrainObjectType::subType` (forest pathing) kept as shipped by decision and suppressed with that reason.

- 2026-11-01: VR Phase 1, first slice (branch `vr-phase1`): `-stereo <separation>` side-by-side debug view. The engine could already draw a frame twice (`renderLists` rewinds its lists); meshes now take the camera and viewport at draw time (the GPU path used the update-time ones), terrain and water are re-projected per eye with the game camera's visibility kept, each eye is an offset applied last in `Camera::setCameraOrigin` (`Camera::renderWorld`), and GameOS points the GL viewport at one half (`gos_SetStereoEye`). Mono screenshots byte-identical to before, game-state checksum unchanged with `-stereo`, eyes checked by measured disparity (first version had them swapped). Overlays, mouse picking, shadows and clouds still follow the game camera: a view for checking the rendering, not for playing. Measured cost on mc2_19 (llvmpipe): CPU frame 1.36 to 1.40x mono. Built and run on GL and GLES; MSVC, Clang and arm64 left to CI.
- 2026-11-01: STOP follow-ups (branch `fix/stop-followups`): the `STOP:` log line format (prefix, one trailing newline, truncation) moved from `logmsg` to `formatLogLine()` in `str_bounded.cpp` with unit tests, as did the message box's prefix strip (`logLineMessage()`); behaviour unchanged. ABL syntax errors ending the game under the opt-out recorded as a decision (see the `STOP` plan). Ubuntu 26.04: GL and GLES builds and unit tests (117 cases); MSVC, Clang and arm64 left to CI.
- 2026-11-01: Full sweep of the remaster's history (branch `fix/backport-sweep`): 419 fix-like commits on shared files checked, 23 ported. Stock data: a leftover `ENTER_DEBUGGER` in `UserHeap::pointerOnHeap` (SIGTRAP from the briefing's empty-model `SimpleCamera::setObject`), `gos_SetMousePosition` warped to 0..1 as pixels, `calcAttackPlan` divided by a zero threat rating, `GlobalMap::destroy` leaked its three pointer arrays every mission, an unbounded height read in `calcBestGoalFromTarget`, the `txmmgr` vertex and shape pools handed out memory past their heap when full (and the untextured copies wrote one slot past their block), mech and vehicle paint colours swapped on an LOD change (cache-hit branches stored them unconverted under `BGR`), `iconsPerPage` used the atlas width, and the logistics CSV reader stepped past a short row's NUL (now `code/csvfield.h`, unit tested). Mod data: missing terrain textures and masks, ABL assignment from broken bytecode, the pilot-ready screen lost after a first mission without logistics, objective copies of an unknown species, icon indexes past the atlas, more than 256 cursors, short or oversized briefing TGAs, unknown TGL node types and vertex indices, the mission-selection button off-by-one, and out-of-range ABL routine keys and full routine tables.
- 2026-11-01: Step 4 of the `STOP` plan (branch `fix/stop-exit-default`): `STOP` and `Assert` end the game by default (log, SDL message box parented to the game window with the mouse released, `_Exit(1)`); no box under `-autoexit` / `-bench` / `-uismoke`; `MC2_STOP_EXITS=0` carries on as before. `Fatal()` and `ABL_Fatal()` are `[[noreturn]]` and end through the new `gos_FatalExit()` regardless of the switch. A missing `-mission` now exits 1 with "Unable to open Mission File" instead of a segfault. cppcheck 2.19 over the whole tree, before and after: 24 `nullPointerRedundantCheck`, 3 `arrayIndexOutOfBoundsCond` and 1 `negativeIndex` gone, and one real bug found: ABL's `LibrariesUsed[25]` took a 26th entry (`>` for `>=`), fixed. Ubuntu 26.04: GL and GLES builds, unit tests, all 30 stock missions and both UI walks on both renderers (64/64); MSVC, Clang and arm64 left to CI; message box not seen (needs a windowed run).
- 2026-11-01: CI also skips pushes that only merge `pi-port` into a PR: `scripts/ci_docs_only.sh` becomes `scripts/ci_skip.sh`, which also skips when the PR's own non-docs patch (merge-base to head) has the same patch id before and after the push, so the push only brought `pi-port` in plus docs. Merging one PR used to rerun every open PR, as each session synced `pi-port` and its `PROGRESS.md` conflict; those syncs now skip. A conflict fix that changes code still runs in full. Accepted risk: the combination with code from `pi-port` is first tested by `pi-port`'s own run after the merge.
- 2026-11-01: VR Phase 0, PC part (branch `vr-plan`): `-bench` reports per-stage times (`camera`, `terrain_geom`, `objects`, `shape_xform`, `world_render`, `ui_render` as `<stage>_ms`) and `-benchfov` / `bench.sh --fov` force the camera FOV. Quiet mc2_19 runs under llvmpipe put stereo at about 1.5x the mono CPU frame (24 ms against 15.4 ms here), the wide FOV alone at about +8% CPU and +26% triangles, terrain nearly flat with FOV, and show the terrain window's edge in view at 100 degrees. Phase 1 audit in the plan: every update-time projecting function also changes state (LOD swaps, `inView` feeding gameplay, animation timers, the texture manager's count/fill split), so the logic eye stays the un-tracked game camera and the render eyes get a transform-only pass. Pi runs still to do for the go/no-go. Also fixed: Windows `timing::ticks2ms` returned 1000 x ticks, so Windows `-bench` milliseconds were wrong. Results in `future-features/VR-TABLETOP.md`.
- 2026-11-01: VR plan revised (branch `vr-plan`): first target is the game's own view in stereo with head tracking and stick panning, no whole-map rendering; tabletop becomes a later option. Scale estimate from the engine's constants (0.2 m per world unit): at 6 mm miniatures scale (1:285) a 1.8 x 0.9 m dining table holds about 20 x 10 terrain vertices, inside today's 30 to 60 vertex window. `future-features/VR-TABLETOP.md`.
- 2026-11-01: cppcheck `memsetClassFloat` (43) closed (branch `fix/cppcheck-memset`): the ASE shape parser zero-fills its vertex and triangle lists instead of `0xff`, so a mesh without texture faces no longer keeps NaN UVs (game data differs only in `tgl.fst`); the gate drops `memsetClassFloat` for zero fills (all-bits-zero is 0.0f on IEEE 754) and still reports non-zero ones. Baseline to regenerate from CI after merge.
- 2026-11-01: CI skips docs-only pushes: a "changes" job in `ci.yml` and `smoke.yml` runs `scripts/ci_docs_only.sh`, and the build, test and smoke jobs skip when everything changed since the previous push is docs (`*.md`, `planning/`, `.claude/`, `docs`, `devdoc.txt`) and that previous commit passed the same workflow. A cancelled or failed run, a force push or a new branch always runs in full. Docs-only PRs no longer start `ci.yml` at all (`paths-ignore`) (#45). Same PR: agents run `/simplify`, `/code-review` and `/code-review high` while CI runs on the push, instead of waiting for green first.
- 2026-11-01: MechCommander Omnitech front-end screen size (branch `omnitech-frontend-res`): the front end runs at 1280x1024 when `mcl_mm.fit` has a `[HiresOffsets]` block (Omnitech's layouts reach 1194x896 and were cut off at 800x600), else 800x600 as before; mode switches, the load screen check and the screen slides follow it. Omnitech's full logistics walk passes and every screen shows in full (frames on a 1280x1024 Xvfb).
- 2026-11-01: UI smoke walks on MechCommander Omnitech data (branch `smoke-mco-walks`): `smoke.sh` without `--ui` picks an Omnitech walk 1 when the game dir's `data/art/mcl_mm.fit` has a `[HiresOffsets]` block: Omnitech's new campaign opens mission selection with no mechs, so the walk goes on to buy, add, pilot and launch. Both default walks pass on Omnitech and on stock data.
- 2026-11-01: agent workflow Tier 2 moves to CI: after `/simplify` + `/code-review`, a batch is pushed to a draft PR and `ci.yml` / `smoke.yml` do the four-configuration build and the smoke missions; local `check.sh --level batch` only for crash debugging or GLES-only changes. Five sessions running batch checks at once were queueing for the same WSL CPUs to repeat what CI runs in 2 to 9 minutes. Also: no `pgrep -f smoke.sh` waits, which matched other sessions' runs.
- 2026-10-31: MechCommander Omnitech mech lab (branch `omnitech-mechlab`): the mech lab takes its fixed statics from Omnitech's 15-static `mcl_mc.fit` as Omnitech's code does (stock's 56 otherwise), so "change loadout" in the mech bay is back on. "Buy" on Omnitech's purchase screen works; the UI walk's purchase was a 100 t Atlas on a 65 t mission. Details in `MOD-COMPAT.md` item 12.

- 2026-10-31: UI smoke, slice 2 (branch `test/ui-smoke-actions`): walk steps can be `<screen>+<action>`, an in-screen button pressed after the hold, then the screen held again: `mechpurchase+buy`, `mechbay+add`, `pilotready+pilot` (`PR_MSG_*` ids moved to `pilotreadyscreen.h`). Leaving the purchase screen now uses Accept (its Back sells back what was bought). The default logistics walk now buys a mech, puts it in the force, opens the mech lab, assigns a pilot and launches mc2_02. Both walks pass on GL and GLES on Ubuntu 26.04 (llvmpipe); MSVC, Clang, arm64 and sanitized runs left to CI.
- 2026-10-31: Step 3 of the `STOP` plan (branch `fix/stop-warnings`): smoke with `MC2_STOP_EXITS` off over every stock mission (24 campaign, 5 tutorials, e3demo) and both UI walks on GL and GLES, and all 100 playable Omnitech missions on GL: no `STOP` fired, only `PAUSE`s (bad texture handle, unsupported map sizes, a missing mech variant). The one `STOP` named in the plan, ABL `coreMoveTo` with a NaN location, now `PAUSE`s and skips the order (result 1) instead of passing the NaN to pathing; not exercised by any run. Batch checks (GL and GLES builds, unit tests, smoke) on Ubuntu 26.04; MSVC, Clang and arm64 left to CI.
- 2026-10-31: cppcheck `pointerSize` and `arrayIndexOutOfBoundsCond` (branch `fix/cppcheck-pointersize`): `InfoWindow` cleared 56 bytes of 7 records, terrain `maskData` only partly zeroed, sensor/ECM/anti-missile lists written past their end after a logging-only `Fatal()`, `removeContact` index bounded by the list not 255, multiplayer `moverRoster` read bounds-checked; baseline regenerated by CI after #33 (413 -> 310).
- 2026-10-31: MechCommander Omnitech front-end screens (branch `omnitech-ui`): the UI smoke walks crashed at the main menu on Omnitech data. 1024x1024 art fits the texture manager's LZ buffers, `gos_LockTexture` handles texture 0, and the briefing, loading and pilot ready screens take their fixed statics from Omnitech's layouts as its code does (pilot ready wrote past its 40 statics). Its mech lab layout is not supported yet, so the mech bay keeps "change loadout" off for it. Desert Fox (5 missions) installed and smoke-tested. Details in `MOD-COMPAT.md` items 12 and 13.

- 2026-10-31: Five sanitizer findings in the menu code, found by the new UI smoke walks under ASan + UBSan (branch `test/ui-smoke`): a heap-buffer-overflow in `SoundSystem::update()` (a pending music stream started with `currentMusicId == -1`; each stream now uses its own tune id), an int shift by 32 in the FIT hex parsers (at most 8 digits now, unit tested), `playLogistics` never set for single missions (uninitialised bool read), `memcpy` from an empty string's NULL data in `EString`, and `-1 << 24` in `LogisticsMech`. Unit tests, both UI walks and the five smoke missions (600 frames) run clean sanitized on Ubuntu 26.04; GL and GLES batch checks pass.
- 2026-10-31: UI smoke, slice 1 (branch `test/ui-smoke`, `future-features/UI-SMOKE.md`): `-uismoke <walk>` walks named front-end screens, pressing the buttons a player would (the same `handleMessage` a click sends, only if the button is enabled), printing `[SCREEN] <name> shown` and `[UISMOKE] done` / `FAIL`. Walk logic is game-free and unit tested (`code/uismokewalk.cpp`); the driver is game glue (`code/uismokedriver.cpp`). `smoke.sh` runs two walks by default after the missions, each with a temporary `XDG_CONFIG_HOME` holding a campaign-save fixture: options tabs then new campaign into mc2_01; single mission mc2_02 through briefing, mech bay, purchase, pilot ready and back. Found: a new campaign skips logistics (mc2_01), single missions are listed only from campaign saves, a single mission starts with no mechs (mech lab and Launch disabled until slice 2). Both walks pass on GL and GLES on Ubuntu 26.04 (llvmpipe); MSVC, Clang, arm64 and sanitized runs left to CI; not run on the Pi.
- 2026-10-31: cppcheck `nullPointer`, `invalidLifetime` and `uninitvar` cleared (branch `fix/cppcheck-null-lifetime`, 104 baseline entries): real fixes in `TG_*::CreateFrom` (no placement `new` on a failed TGL heap allocation), the multiplayer stubs (blank record, empty name, zero count instead of NULL), `Camera::getClosestVertex` (no vertices), `LogisticsMissionInfo::load` (error text from an unset buffer) and `MechWarrior` (one `getGroup()` result); effect draw-info lifetimes and bounded player `qsort`s suppressed inline as false positives.
- 2026-10-31: MechCommander Omnitech's other campaigns (branch `omnitech-campaigns`): 13 Omnitech-only ABL commands (`addmovertoplayer`, `setbrain`, `settextmsg`, `addresourcepoints`, salvage craft, building name and capture RP, mech damage and gesture, ...) from its changelog, brain changes queued to the start of the next frame; colour-mapped TGAs load (Omnitech's sky 12); weapon damage over 255 allowed outside multiplayer; `MAX_OFFMAP_AREAS` 60 -> 120 with the fill bounded (ASan: it overran the `GlobalMap` object). Details in `MOD-COMPAT.md` item 11.

- 2026-10-31: Back-port of the remaster's original LZ decoder (branch `fix/backport-lz-decoder`): `LZDecomp` reads Microsoft's original LZW (the 2001 asm format) as well as zlib, picked from the first two bytes, every write bounded by `destLen`; `FastFile::readFast` copies raw entries (`size == realSize`) when decoding fails, so uncompressed 0xCADDECAF archives read too. Test-first: `test_lzdecomp.cpp` (a test-only encoder that follows the asm compressor, hand-built streams, multi-block, truncated, over-limit, corrupt, zlib, FSTs through `File::open`; no third-party data). Omnitech's `tgl.fst`: all 4667 entries decode to their exact size, 1947 identical to the stock ones and the rest differ only in float last bits and padding bytes (built by different tools); mc2_01 and mc2_03 smoke-run with it. Built and tested on WSL Ubuntu 26.04 (GCC, desktop GL and GLES); MSVC, Clang, arm64 and the Pi left to CI.
- 2026-10-31: review of #22: ABL `print()` had the same 20-byte real overflow as `concat()` (now 64 bytes, `snprintf`); the bounded append and the env switch parsing moved to GL-free `GameOS/gameos/utils/str_bounded.cpp` with unit tests; `MC2_STOP_EXITS` treats `false` / `no` / `off` as off; `STOP(("... %s", str))` (the two-argument `InternalFunctionStop`) now prints the `STOP: ` prefix and a newline, so smoke's `^STOP:` check sees it. Ubuntu 26.04: GL and GLES builds, unit tests, Clang; CI's five missions on both renderers (#28).
- 2026-10-31: mc2_15 segfault fixed (#25): ABL `getRelativePositionToObject` wrote nothing unless the object was a mover, and `coreEscort` passes it an unset local array (ABL local arrays come from `malloc`, unzeroed). `mc2_15_E2_TCD_Mech2` escorts a building (part id 141696), so it ordered a move to leftover heap bytes (glibc free-list pointers, e.g. x = 1.7e18), and `GlobalMap::calcArea` read off the map. The native now always writes its result and uses the virtual `GameObject::relativePosition` for any object (a building returns its own position; no object gives 0,0,0). GLES mc2_15 at 3000 frames: 6/6 pass on Ubuntu 26.04 WSL (before: 1 crash in 8 runs here, most runs in the earlier sweep); desktop GL mc2_15 also run.
- 2026-10-31: backlog tidy: to-do and tidy-up items done elsewhere ticked (review #26, STOP step 1 #22, agent workflow #8, Omnitech review fixes #17/#21, WSL on Ubuntu 26.04, uninit triage #23/#24); cppcheck baseline regenerated by CI (`static.yml` run 41209792050): 523 -> 413, every `uninitMemberVar`/`uninitDerivedMemberVar` gone; `BUILD-LINUX.md` lists the tested setups; `MOD-COMPAT.md` names the MC2X test campaign and the remaster's LZ decoder; stale worktrees and local branches of merged PRs removed (#27).
- 2026-10-31: last review findings (#26): bench scenarios refuse a `Shot` outside its own segment and a camera key without exactly 6 values (mc2_19's closeup shot moved from 2400, the next segment's first frame, to 2250); the key-sort test now fails without the sort; a unit test loads the shipped `bench/mc2_19.fit`. mc2_17 smoke passes 3 of 3 since #19 (B7 closed). Batch checks (GL and GLES, smoke) on Ubuntu 26.04; not run on the Pi.
- 2026-10-31: cppcheck `uninitMemberVar` / `uninitDerivedMemberVar` cleared in `code/`, `gui/` and `mclib/` outside gosfx/mlr/stuff (#24): 59 baseline entries, members zeroed in their constructors (`aListItem` gets a constructor, so its five derived-class findings go together), 9 suppressed inline with a reason (7 flexible array members in `multplyr.h`, `MovePath::stepList`, a `HeapManager` false positive), plus three that only cppcheck 2.19 reports (`MoverControl`, `MoverDynamics`, `MasterComponent`). `ABLModule::orderCallFlags` now starts `NULL`, and `WatchManager::init()` sets `numWatches` (it set `maxWatches` twice). Batch checks (GL and GLES, smoke) on Ubuntu 26.04.
- 2026-10-31: cppcheck `uninitMemberVar` cleared in `mclib/gosfx`, `mlr` and `stuff` (#23): of the 45 baseline entries, 42 fixed by initialising the members (gosFX events, seeded curves, effect cloud and singleton specs; MLR effect, card cloud, shape, terrain, triangle meshes, screen quads; `AverageOf`, `Directory`, `MemoryStream`, the `MinMaxHolderOf` test helper; one `MemoryStream` entry was already fixed on `pi-port`), 3 suppressed inline as false positives (`StaticArrayOf` copy constructor and `Matrix4D(int)` fill through helpers cppcheck does not follow; `Record` gets its variable-length tail by `memcpy`). Local cppcheck 2.19: 44 findings in these directories before, 0 after. Batch checks on Ubuntu 26.04.
- 2026-10-31: `MC2_STOP_EXITS=1` makes `STOP` (so `Fatal`, `Assert`, `ABL_Fatal`) log and `_Exit(1)`; `smoke.sh` and `bench.sh` set it, default unchanged (step 1 of the STOP plan). Also ABL `concat()` overflowed a 20-byte buffer and the script's `char[40]` on a huge real (`cpl0101a.abl`; mc2_15 aborted under GCC 15's `_FORTIFY_SOURCE` on every GLES run): now bounded. All built and run on Ubuntu 26.04 WSL (GCC 15, Clang); MSVC not built locally. Smoke with the switch on, 28 missions (campaign, tutorials, e3demo) at 600 frames: GL 28/28, GLES 27/28, no `STOP` fired; the failure is an mc2_15 GLES segfault found here and not fixed here, fixed in #25 (#22).
- 2026-10-31: All 28 Clan Wars missions (`innersphere1`-`28`) pass a 300-frame headless smoke at Omnitech's texture settings (#21). 10 failed first: Omnitech's editor saves an unset `preNumCells` in special-area footprints (never read; now stored as 0) and leftover memory in the pathfinder's start/goal door links (now treated as scratch, like the trailing slots), and 3 missions have more than 119 pilots (`MAX_WARRIORS` 120 -> 160, as in Omnitech; team rosters stay at 120 and are now bounded). Review also found the start/goal door link lists sized from the map's stored count, which a path search could overrun: `GlobalMap::init` now grows them to the largest area's door count. New `[TXMCACHE]` (compressed textures held) and `[MEM]` (peak resident) log lines: Clan Wars peaks at 72-92 MB of compressed textures against 12 MB for stock missions; peak resident 360-860 MB under llvmpipe (varies run to run) against 290-375 MB stock. Not measured on the Pi.
- 2026-10-31: Tidy-ups after PR 17: object texture lookups check each file once (`objectTextureName()`); the remaining `LZDecomp` calls in the FST reader and logistics are bounded. Decompressing each listed terrain-transition cache twice at load was measured (about 64 ms for mc2_01's 100 caches on a desktop) and left. UI smoke tests designed, not built (`future-features/UI-SMOKE.md`) (#21).
- 2026-10-31: review findings R1, P2, E1 (#20): `gos_vertex.vert` writes `gl_PointSize` so `GL_POINTS` have a size on GLES; `gos_MarkGameplayStarted()` fires on every mission (a member cleared in `Mission::init`, not a static pointer to the reused `Mission`); `GlobalMap::init` allocates `doorInfos` / `doorLinks` before the reads that can fail, so `destroy()` never frees uninitialised pointers from a half-loaded map. Batch checks (GL and GLES, shaders, smoke) on Ubuntu 26.04; planet point not looked at on the Pi.
- 2026-10-31: unattended runs behave the same on any display: `scripts/smoke.sh` always uses Xvfb (WSLg's display made local smoke and batch checks open a full-screen window on the desktop; `--display` opts out), and every `-autoexit` run, like `-bench`, ignores the keyboard and mouse, holds the cursor at the screen centre and runs windowed at the game resolution (`gos_IsAutomatedRun()`). Checked under a 1600x1200 Xvfb with FullScreen on: drawable 800x600, cursor at 400,300 (#19).
- 2026-10-31: code review of all of `pi-port` (`planning/REVIEW-PI-PORT.md`): 1 high, 3 medium, 13 low findings, plus a flaky mc2_17 smoke found while verifying (B7). Build/CI fixes (#18): smoke catches `STOP` lines that land mid-line and `logmsg` ends each message with a newline; the smoke game-data cache key hashes all of `mclib` and `GameOS`; the cppcheck gate exits 2 when cppcheck or the gate itself fails, and `check.sh` reports it instead of passing; `check.sh` compares cppcheck findings by count; the shader validator also checks the GLSL 3.30 + 420pack header; unit-test fixtures are copied afresh on every build. Batch checks (GL and GLES builds, unit tests, smoke) on Ubuntu 26.04; MSVC, Clang and arm64 left to CI.
- 2026-10-31: MechCommander Omnitech content loads and plays (PR 17, branch `omnitech-objtypes`): object types past `object2.pak` from `movobj.fit` (mech type FITs built from the CSV), vehicle and building profiles in missions and logistics, wide pathing records and special-area footprints, `.burnin.jpg` colour maps (stb_image, the original engine's disabled JPG branch), strings from a game-folder `mc2res.dll` for ids the stock table lacks, object textures from another `ObjectTextureRes` folder, cached terrain transitions built at another `TerrainTextureRes`. Clan Wars mission 1 plays (checked by `-bench` screenshots), exits clean, ASan clean. `MOD-COMPAT.md` has the details.
- 2026-10-31: Loader and renderer bugs found on the way (PR 17), most hidden because `gosASSERT` compiles out and `STOP` only logs: texture LZ buffers overrun by 512 textures, a heap overflow from Omnitech's 528-byte special-area footprints, a missing texture crashing release builds, unbounded `LZDecomp` (now takes an output bound), unchecked special-area count; fog colour reached the shaders with red and blue swapped on every mission (stock fogs are near-grey, so it hardly showed).
- 2026-10-31: Back-ports from the remaster (branch `fix/backport-mech-save`): `BattleMech` checks `sensorSystem` before every write (a mech with no sensor component, or whose profile load bailed early, has none); `.mdf` component ids past `masterList` are dropped like empty slots and crit-space bytes past `ItemLocationToInvLocation` mark the space empty; `.fit` mech profiles with too many items or an out-of-range `MasterID` are rejected (`Fatal()` only logged, then wrote past `inventory[]`) and crit spaces past the populated inventory are empty; the campaign dialog no longer scans every save game first, each save is version-checked once, and rejected list entries are freed. Not ported: `a03e13a2` (stub mech types do not exist here), `aa20a308` (a `badLoad` move map already exits at mission load), `84e1774a` (Omnitech work).
- 2026-10-31: back-ported the mission / ABL hardening group from the remaster: a mission with no parts still gets team 0 and commander 0, `Mission::destroy` clears `Team::home` / `Commander::home`, `Mission::init` frees every old commander, a missing or invalid ABL file makes `ABLi_preProcess` return -1 (and `ABLi_loadLibrary` treats any negative handle as a failure), and `execExpression` no longer dereferences a NULL operand type (branch `fix/backport-mission-abl`).
- 2026-10-31: quick fixes from the tidy-up list: `FitIniFile` finds a key (scalar, string or array) on the last line of a file without `FITend` (no stock file has that shape); unused `object_tex` shaders deleted and the shader validator fails on `#version`; stale docs fixed (vsync-off numbers, panning, #6 log line, MSVC `small` trap); `mission.cpp` item closed with no change needed.
- 2026-10-31: Bench review fixes (branch `fix/bench-review`): scenario `load()` keeps defaults for missing optional keys (`readId*` zeroed them) and refuses bad, repeated or `all` segment names, negative `Gather` and repeated `Shot` frames (unit tested); `-bench` pins the cursor at the centre (no more up-left edge scroll) and ignores polled keys and buttons; `glFinish()` before a screenshot so its GPU wait is not subtracted from the frame; perf control FIFOs opened non-blocking with a 2 s ack timeout, and created in a `mktemp -d` dir (DrvFS has no FIFOs); `segments.csv` gains display, vsync, options_cfg and the run's status; scenario error text no longer loses its first character. Unit tests on GL and GLES builds (WSL); not run on the Pi, MSVC not built.
- 2026-10-31: "To do: process and housekeeping" list added: WSL Ubuntu upgrade, batched code review of `pi-port`, agents onto the updated process, transcript sweep for dropped ideas, cppcheck baseline triage. Transcript sweep done the same day (`Fatal()` decision reversed after it: `STOP` exits again, behind a CI switch first): "Tidy up" list of untracked items (branches at risk, bugs, Pi perf backlog, unverified changes, remaster back-ports, stale docs, housekeeping).
- 2026-10-31: `-bench` keeps texture eviction off the wall clock: PR 6's 2-second window (turns scaled by a measured frame rate) stays at 30 turns per second under the fixed step, so evictions no longer depend on machine speed. `gos_GetFixedFrameLength()` moved into libgameos so mclib callers link into the data tools. Two merged mc2_19 runs identical in draws, triangles, state and screenshots. (PR 7)
- 2026-10-31: Air strike bombers painted from uninitialised memory: `GVAppearance` never set `psRed`/`psGreen`/`psBlue`, and the bomber (no paint scheme of its own) repaints itself with them in `setObjStatus()`, so its colour, and whether two bombers shared a texture instance, came from heap garbage. Defaulted to white as `MechAppearance` does. Found as the last run-to-run difference in the bench effects segment (draw counts, ~0.3% of pixels). (PR 7)
- 2026-10-31: Bench camera circles the fight: framing projects the fight's centre and moves the aim point to hold it at `ScreenX`/`ScreenY` (replaces `Lift`, whose direction only held at some yaws); `mc2_19.fit` turns one revolution a minute through every segment. Screenshots are the whole drawable, and headless `bench.sh`/`smoke.sh` use an 800x600 Xvfb screen: with FullScreen on, the 1600x1200 one sized the viewport to the screen while the window stayed 800x600, so headless runs drew a 2x-scaled corner of each frame. (PR 7)
- 2026-10-31: GPU benchmark phase 2: bench director (`code/benchdirector.cpp`, logic in `code/benchscenario.cpp`, unit tested) plays `bench/mc2_19.fit` (player lance vs 20 enemies, 4 segments, artillery, follow camera); per-segment `[BENCH]` lines, TGA screenshots, `state` checksum column; `-bench` turns sound off (radio chatter made `RandomNumber()` calls depend on audio timing, so combat runs drifted). Verified in WSL (llvmpipe, GL and GLES): two 3600-frame runs identical in game state and screenshots; draw-call count still varies ~0.3% in the effects segment (triangles identical). Not run on the Pi.
- 2026-10-31: cppcheck `uninitMemberVar` triage: the 100 baseline findings (2.13) plus the 19 that only local 2.7 reports (`GVAppearance`, `Turret`, `BldgAppearance`, `Mech3DAppearance`). Real bugs fixed: `GVAppearance::isInfantry` read unset by the SimpleCamera vehicle view (dust cloud loop), now `false` in `init()` (`276daee`); the bomber paint colours, same change as PR 7 (`188ac39`); a font texture that failed to load was still registered and bound with an unset `Texture::type_` (`69b5eda`); `MemoryStream` bit IO after the `class_data` constructor used an unset buffer (`49e107e`). All others are set before any read, never read, or in stubbed MP / test code.
- 2026-10-31: agent workflow in three tiers (`.claude/rules/agent-workflow.md`): one worktree per session or feature; `scripts/check.sh` before every commit (incremental build, unit tests, shaders, cppcheck on changed files compared with the code before the change, line endings / em dashes); `--level batch` every few commits adds both renderers and a short smoke, then `/simplify` + `/code-review`; at the PR `/code-review high`, and `smoke.yml` now also runs on PRs that touch code.
- 2026-10-31: render CPU and hitch fixes (#6, `01bfcb3`): terrain lit only by lights whose falloff reaches the frame's terrain box (`Camera::cullTerrainLights`); unused textures freed after 2 seconds instead of 60 frames (re-uploads while panning); LOD switches with hysteresis; cached `joint_torso` node lookup; `mclib/render_heuristics.h` unit tested. Pi hitch numbers not measured.
- 2026-10-31: render CPU quick wins (#5): shader hot reload no longer stat()s every shader every frame, alpha-sorted effect triangles batched per state, GameOS skips repeated GL state / sampler / uniform calls and streams vertex uploads, shapes lit only by point/spot lights in reach. Pi 400 GPU build, vsync off: mc2_01 113 -> 179 fps, mc2_03 148 -> 181 fps; llvmpipe mc2_01 14.1 -> 17.3 fps.
- 2026-10-31: `bench.sh --perf`: one `perf record -g` run per mission, sampling switched on/off by the game at the first and last gameplay frame (`MC2_PERF_CTL`/`MC2_PERF_ACK` FIFOs), per-library and top-symbol report, optional flame graph; `-DMC2_FRAME_POINTERS=ON`. Verified in WSL (llvmpipe): sampling covers gameplay only, frame-pointer stacks reach `main` (WSL's perf has no libelf, so names were checked with addr2line). First lead: `FastFile::openFast` and zlib busy during gameplay on mc2_03. Not run on the Pi.
- 2026-10-30: GPU benchmark phase 1: `-bench <csv>` deterministic mode, per-frame CSV and `[BENCH]` percentiles (`GameOS/gameos/utils/frame_stats.cpp`, unit tested), draw/triangle counters, `bench.sh` wired up. ABL `getLogisticsTime` / `isPlayingVoiceOver` follow mission time under `-bench` (tutorial stages were timed by the wall clock). Verified in WSL (desktop GL, llvmpipe): two runs identical in draws/tris for every frame, mc2_01 900 frames and mc2_03 3000 frames. Not run on the Pi yet.
- 2026-10-30: fix: every mission crashed at load ("Too Many Standard Functions") after the Omnitech ABL commands took the table past 256; `MAX_STANDARD_FUNCTIONS` now 512 (`mclib/ablsymt.h`).
- 2026-10-30: GPU benchmark specced (`future-features/GPU-BENCHMARK.md`): deterministic `-bench` mode, scripted camera and combat in segments, per-frame CPU/GPU split and draw counters, checkpoint screenshots. Not built.
- 2026-10-30: 10 Omnitech ABL commands its base brains need (`code/ablmc2.cpp`); builds and unit tests only, Clan Wars load not re-run yet.
- 2026-10-30: global pathing packets size-checked; Omnitech's wide layout read and narrowed (`mclib/globalmapio.h`, unit tested); a bad move map now stops the mission with a message instead of corrupting the heap (`43ba20a`). Clan Wars mission 1 on 6.28 data gets past pathing; next blocker is Omnitech ABL commands in its base brains.
- 2026-10-30: Omnitech 6.28 install surveyed (not run): changelog documents every new ABL command, debug symbols give 6.28 struct layouts (pathing 31/5/73-byte packed) and ~50 MCO-only ABL commands; data is stock-format with ~9800 new files (`MOD-COMPAT.md`).
- 2026-10-30: Clan Wars (MCO campaign) test load: needs Omnitech base data (~140 unit tables) and MCO's widened pathing format; ABL fine. `GlobalMap::init` corrupts the heap on a wide-format map (recorded in Known issues).
- 2026-10-30: fan mod compatibility study (`planning/future-features/MOD-COMPAT.md`): Omnitech 2013 source snapshot vs Microsoft base: 18 extra ABL functions, `CSVFile` per mission part, new logistics/save keys, most fixed limits doubled; MC2X has no public source.
- 2026-10-29: `MC2_VSYNC=0` env switch to run without vsync; startup logs `vsync: on/off` (Xvfb smoke checked both).
- 2026-10-29: confirmed on real Pi 400 hardware: GPU build mc2_01 58.5 fps (vsync-capped), software rendering mc2_01 50.4 / mc2_03 67.1 fps (user test).
- 2026-10-29: `uninitMemberVar` tail: 35 constructors given zero/neutral defaults (screens, movie, font, image, input, file/packet streams, MLR sorter/lights, gate ids, tactical order, radio, weather, objectives) (`afb57ff`); baseline 592 -> 523.
- 2026-10-29: inventory ids bounded by the 72-entry table, not the 255 sentinel (corrupt mech profiles); printf signedness; `LogisticsScreen`/`aObject` assignment copies help/fade fields; gate filters ctor/dtor `init`/`destroy`/`close` (`7446063`); baseline 702 -> 592.
- 2026-10-29: `duplInheritedMember`: vehicle tooltip name came from a shadowed field (`GVAppearance::objectNameId`), duplicate `BuildingType::teamId`, dead duplicate overrides removed; gate treats mclib/stuff and mclib/gosfx member hiding (and `init` overloads) as deliberate; forest pathing shadow recorded under Known issues (`22f7c98`); baseline 1060 -> 702.
- 2026-10-29: ABL team ids 500..508 indexed `Team::teams[8]` one past the end, body-location off-by-one in `MasterComponent`, `aAnimation` assignment leak, `PostcompVertex` assignment dropped `water` (`4ce088d`); `Fatal()`-returns noted under Known issues; baseline 1095 -> 1060.
- 2026-10-29: cppcheck `nullPointerRedundantCheck`: 8 real check-after-use bugs fixed (ABL static allocation test, inverted `GlobalMap::print` guard, logistics results, tutorial button, ...) (`2aa3591`); baseline 1114 -> 1095.
- 2026-10-29: Stereo panning works (branch `stereo-panning`): `panToStereoLevels()` (constant-power, full level at centre, 6 unit tests) feeds `Mix_SetPanning`; also fixed `gosAudio_GetChannelSlider` panning falling into the frequency case. Not yet checked by ear.
- 2026-10-29: cppcheck `uninitMemberVar` 787 -> 170 in three batches (`1d806e5`, `3d11ac6`, `ecab444`), including real copy-constructor bugs in `LogisticsScreen` and `aObject` and an unset `MultiPlayer`; gate keys ignore line numbers in messages (`f6fb2d2`); baseline 1731 -> 1114.
- 2026-10-29: Release plan written up (`planning/RELEASES.md`): Pi 4/400 as the first-class constrained target, plus Linux x64/arm64 AppImages and a Windows zip for everyone; support levels, phases, licensing checklist.
- 2026-10-29: Feasibility studies for the upstream gaps: `future-features/SOUND-POSITIONAL.md` (stereo panning is a small fix; doppler needs a new mixer, low value outside VR) and `future-features/NETWORKING.md` (transport and ~80 `MultiPlayer` methods were stubbed in Microsoft's release; large, phased).
- 2026-10-29: cppcheck bulk cleanup: gate ignores the five "Stuff" registration names for duplInheritedMember (`b72cf00`), printf length modifiers fixed at 433 sites (`0d96de4`, `ba65d34`); baseline 4415 -> 1731.
- 2026-10-29: Planned: GitHub release packaging; aim recorded to take as much of the remaster as fits, including upscaled game data after a Pi performance check.
- 2026-10-29: README rewritten for the fork; VR tabletop feasibility plan (`755ead3`).
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
