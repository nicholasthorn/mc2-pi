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
- [x] arm64: `-fsigned-char`, portable heap caller capture, GCC 12 fixes
- [x] Clang and MSVC build fixes
- [x] Back-ported crash/UB fixes from `mc2-opengl-remastered` (two batches)
- [x] CI: Linux x64 GCC, Linux x64 Clang, Linux arm64 GLES, Windows MSVC, shader validation (GLSL ES 3.10 + GLSL 4.20); Dependabot
- [x] Agent setup: `CLAUDE.md`, `.claude/rules/`, `.claude/skills/`, this file
- [x] Unit tests: doctest harness in `tests/unit/` under CTest, run by CI on all four configurations; FIT/packet/file, CSV, FST, paths, pixel repack, RollDice, audio pan, JPEG, PE strings, TGA writer, frame stats, bench scenarios, global map IO, render heuristics, bounded strings, bench stages (112 cases)
- [x] Smoke runs: `-autoexit <gameplay frames>`, `scripts/smoke.sh` (Xvfb + llvmpipe), `scripts/build_gamedata.sh`; nightly `smoke.yml` on x64 desktop GL and arm64 GLES (mc2_01/03/10/17/24); smoke and bench set `MC2_STOP_EXITS=1`, so a `STOP` ends the run (#22); front-end walks with `-uismoke` (main menu, options, campaign and single-mission dialogs, briefing, mech bay, purchase, mech lab, pilot ready, launch)
- [x] Benchmark: `scripts/bench.sh` runs missions with `-bench <csv>` (fixed 1/30 s game step and mission clock, fixed seed, input ignored), per-frame logic/render/swap ms + draw calls + triangles to CSV, plus named stage times (`camera`, `terrain_geom`, `objects`, `shape_xform`, `world_render`, `ui_render` as `<stage>_ms`), `-benchfov` / `--fov` to force the camera FOV, `[BENCH]` p50/p95/p99/max summary into `results.csv`; `--perf` adds a `perf record -g` run per mission sampling gameplay frames only (game drives perf's control FIFO), `MC2_FRAME_POINTERS` build option; `--scenario bench/*.fit` stages a scripted battle (units gathered and fighting, damage off, camera path, artillery) with named segments timed separately (`segments.csv`) and screenshots; `-bench` also turns sound off and logs a per-frame game-state checksum
- [x] Sanitizers: `MC2_SANITIZE`; unit tests under ASan + UBSan on every push; nightly sanitized smoke (x64, 3000 frames); no UBSan suppressions (the five missions run clean)
- [x] Mods: MechCommander Omnitech data (object types past `object2.pak`, wide map packets, `.burnin.jpg` colour maps, `mc2res.dll` strings, 512 object textures) loads and plays; Clan Wars mission 1 checked in play (PR 17, `future-features/MOD-COMPAT.md`)
- [x] Warnings + static analysis: `-Wall -Wextra -Werror` on new code; `-fpermissive` removed; cppcheck gate against a CI-generated baseline (changed files per push, full tree nightly); baseline 523 after fixing error-level bugs, filtering the deliberate "Stuff"/gosFX member hiding and constructor `init()`/destructor `destroy()` calls, the printf length-modifier sweep and most `uninitMemberVar`; 413 after every `uninitMemberVar` was initialised or suppressed with a reason (#23, #24)

**Not verified / unknown**

- [x] Real Pi 400 hardware (2026-10-29, at `10401db`): builds and runs. GPU (GLES) build, mc2_01, 3000 gameplay frames in 51.2 s (58.5 fps, about 800x600); that is at the 60 Hz vsync cap; with `MC2_VSYNC=0` (vsync off), mc2_01 ran at 113 fps before #5 and 179 fps after it (mc2_03 148 -> 181). Software rendering: mc2_01 50.4 fps, mc2_03 67.1 fps.
- [x] Pi: GPU frame rate with vsync off (see above and the #5 log line)
- [ ] Pi: the other three smoke missions on the GPU build
- [ ] Desktop GL run on Windows after the GLES changes

**Next** (see `planning/QUALITY-ROADMAP.md` for the full list)

- [ ] More tests: on-disk struct sizes (`static_assert`), legacy `mclib/stuff` `TestClass()` self-checks
- [ ] cppcheck: `nullPointer` (87, mostly known false positives), `nullPointerRedundantCheck` (84, mostly `if (!p) Fatal(...)`: `Fatal()` returns because `STOP` only logs, until step 4 of the `STOP` plan) (roadmap Phase 2). `uninitMemberVar` is cleared (#23, #24; baseline 413); see "Work through the cppcheck baseline" below
- [ ] Sanitized smoke on arm64 too
- [ ] Deterministic smoke runs (seed the RNG): `-bench` does this for bench runs; smoke runs could pass it too
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
- [ ] **Work through the cppcheck baseline** (`scripts/cppcheck-baseline.txt`,
      523 findings before #23/#24, 413 after: regenerated by CI 2026-10-31).
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
  - Next: `nullPointerRedundantCheck` (84, mostly `if (!p) Fatal(...)`:
    better after the STOP/Fatal work below), `memsetClassFloat` (43),
    `duplInheritedMember` (41), `allocaCalled` (27) and the rest.
  - Version gap: CI runs cppcheck 2.13 (ubuntu-24.04), local runs 2.19
    (Ubuntu 26.04), and 2.7 found the `GVAppearance` one that bit us. Consider
    a second cppcheck version in the nightly run, or clang-tidy's init checks.
  - Regenerate the baseline from CI (`static.yml` `update_baseline`) after
    each batch, so the count only goes down.
- [ ] **Make `STOP` / `Fatal()` stop again** (decided 2026-10-31, replacing
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
  4. Then make exiting the default, keep an opt-out variable for mod
     bring-up, and mark `Fatal()` / `ABL_Fatal()` `[[noreturn]]` so cppcheck
     and the compilers can see that code after them doesn't run.

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

- [ ] Phase 1: packaging scripts, `--tools-dir` for `build_gamedata.sh`, install
      notes, third-party license bundle
- [ ] Phase 2: `release.yml` (tag-triggered, draft release): Pi tarball
      (`debian:bookworm` container), Windows zip (DLLs and C++ runtime
      included), Linux x64 tarball
- [ ] Phase 3: Linux x64 and arm64 AppImages on old bases
- [ ] Phase 4: smoke tests against the unpacked packages
- [ ] Phase 5: `v0.1.0-rc1`, run by hand on Windows, Linux x64 and a Pi 400
- [ ] Phase 6: `v0.1.0`, with Pi hardware results in the notes

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

- [ ] Keep sweeping the remaster's history for fixes to shared `code/` and
      `mclib/` files.
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

## Known issues

- Networking is stubbed (UI only). The multiplayer screens (`code/mp*.cpp`) are unreachable because the main menu's Multiplayer button is disabled; the `MultiPlayer` stubs now return a blank player record, an empty name and no players instead of NULL, so the screens would not crash if reached.
- Sound: doppler not implemented (alariq README). Stereo panning works since #4 (`fa0188e`); nobody has recorded checking it by ear.
- Some files may still be written outside the user config directory (alariq README TODO).
- WSLg's D3D12 GL driver shows a white screen with desktop GL; use `LIBGL_ALWAYS_SOFTWARE=1`.
- `-mission <name>` with a mission that does not exist crashes (SIGSEGV) instead of reporting an error. With `MC2_STOP_EXITS=1` it exits 1 at the first `STOP` ("Unable to open Mission File") instead.
- Fog byte semantics differ: actors (and crater/gvactor/mech3d) turn "no fog" (fogFactor 256) into 0, terrain (`quad.cpp`) uses 0xff. Original behaviour, kept; worth checking visually on the Pi.
- `Fatal()` / `ABL_Fatal()` return: `STOP` only logs (`InternalFunctionStop`, its `exit(1)` is commented out), so code after a fatal error carries on, usually into the NULL or full array it was guarding (`if (!p) Fatal(...); p->...`; ~35 sites, most of the remaining cppcheck `nullPointerRedundantCheck`, plus some `arrayIndexOutOfBoundsCond`). Making them abort (noreturn) would end the game cleanly instead, but any Fatal that fires harmlessly today would start quitting. Decided 2026-10-29: leave as is for now; reversed 2026-10-31, see "Make `STOP` / `Fatal()` stop again" in the to-do list (step 1, `MC2_STOP_EXITS=1` for smoke and bench, is in).
- `mech3d.cpp` helicopter dust cloud computes a motion-relative matrix and then draws with the plain one, unlike the water wake next to it. May be deliberate; unverified.
- Destroyed forests never reopen their pathing areas: `TerrainObjectType` declares its own `subType`, shadowing `ObjectType::subType`, so `TerrainObjectType::handleDestruction`'s `getObjectType()->getSubType() == TERROBJ_FOREST` (the base field, always 0) is never true and `openSubAreas()` never runs. Same in the original source, so shipped behaviour. Removing the shadow would fix it but changes gameplay (and `ObjectManager::getSpecificObjects` for terrain objects); not done, needs a decision.

## Log

Newest first. `YYYY-MM-DD: what (commit or PR)`.

- 2026-11-01: VR Phase 0, PC part (branch `vr-plan`): `-bench` reports per-stage times (`camera`, `terrain_geom`, `objects`, `shape_xform`, `world_render`, `ui_render` as `<stage>_ms`) and `-benchfov` / `bench.sh --fov` force the camera FOV. Quiet mc2_19 runs under llvmpipe put stereo at about 1.5x the mono CPU frame (24 ms against 15.4 ms here), the wide FOV alone at about +8% CPU and +26% triangles, terrain nearly flat with FOV, and show the terrain window's edge in view at 100 degrees. Phase 1 audit in the plan: every update-time projecting function also changes state (LOD swaps, `inView` feeding gameplay, animation timers, the texture manager's count/fill split), so the logic eye stays the un-tracked game camera and the render eyes get a transform-only pass. Pi runs still to do for the go/no-go. Also fixed: Windows `timing::ticks2ms` returned 1000 x ticks, so Windows `-bench` milliseconds were wrong. Results in `future-features/VR-TABLETOP.md`.
- 2026-11-01: VR plan revised (branch `vr-plan`): first target is the game's own view in stereo with head tracking and stick panning, no whole-map rendering; tabletop becomes a later option. Scale estimate from the engine's constants (0.2 m per world unit): at 6 mm miniatures scale (1:285) a 1.8 x 0.9 m dining table holds about 20 x 10 terrain vertices, inside today's 30 to 60 vertex window. `future-features/VR-TABLETOP.md`.
- 2026-11-01: CI skips docs-only pushes: a "changes" job in `ci.yml` and `smoke.yml` runs `scripts/ci_docs_only.sh`, and the build, test and smoke jobs skip when everything changed since the previous push is docs (`*.md`, `planning/`, `.claude/`, `docs`, `devdoc.txt`) and that previous commit passed the same workflow. A cancelled or failed run, a force push or a new branch always runs in full. Docs-only PRs no longer start `ci.yml` at all (`paths-ignore`) (#45). Same PR: agents run `/simplify`, `/code-review` and `/code-review high` while CI runs on the push, instead of waiting for green first.
- 2026-11-01: MechCommander Omnitech front-end screen size (branch `omnitech-frontend-res`): the front end runs at 1280x1024 when `mcl_mm.fit` has a `[HiresOffsets]` block (Omnitech's layouts reach 1194x896 and were cut off at 800x600), else 800x600 as before; mode switches, the load screen check and the screen slides follow it. Omnitech's full logistics walk passes and every screen shows in full (frames on a 1280x1024 Xvfb).
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
