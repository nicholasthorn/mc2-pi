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
- [x] Warnings + static analysis: `-Wall -Wextra -Werror` on new code; `-fpermissive` removed; cppcheck gate against a CI-generated baseline (changed files per push, full tree nightly); baseline 702 after fixing error-level bugs, filtering the deliberate "Stuff"/gosFX member hiding, the printf length-modifier sweep and most `uninitMemberVar`

**Not verified / unknown**

- [ ] Running on real Pi 4/400 hardware: record here once confirmed (which missions, frame rate)
- [ ] Desktop GL run on Windows after the GLES changes

**Next** (see `planning/QUALITY-ROADMAP.md` for the full list)

- [ ] More tests: on-disk struct sizes (`static_assert`), legacy `mclib/stuff` `TestClass()` self-checks
- [ ] cppcheck: `uninitMemberVar` (169), `nullPointer` (87, mostly known false positives), `nullPointerRedundantCheck` (84, mostly `if (!p) Fatal(...)`: `Fatal()` returns because `STOP` only logs) (roadmap Phase 2)
- [ ] Sanitized smoke on arm64 too
- [ ] Deterministic smoke runs (seed the RNG)

## Planned: built releases on GitHub

Not started. Engine-only packages attached to GitHub Releases, built by a
workflow when a version tag is pushed.

- [ ] `.github/workflows/release.yml`: on `push: tags: ['v*']`, same matrix as
      `ci.yml`, run the unit tests, package, then create a **draft** release with
      generated notes (`gh release create`, `permissions: contents: write`).
- [ ] Packages, plus a `SHA256SUMS` file:
  - `mc2-pi-arm64-gles.tar.gz`: built inside a `debian:bookworm` container on
    the arm64 runner, so it matches Raspberry Pi OS's glibc and SDL packages
    (a binary built on `ubuntu-24.04-arm` probably will not start there).
  - `mc2-linux-x64.tar.gz`: built on an older base (or an AppImage bundling
    SDL) so it runs beyond the newest Ubuntu.
  - `mc2-windows-x64.zip`: `mc2.exe`, `mc2res.dll`, `shaders/`, the SDL2 /
    SDL2_mixer / SDL2_ttf / GLEW / zlib DLLs from `3rdparty.zip` with their
    license texts, and either the MSVC runtime DLLs or a note to install the
    Visual C++ Redistributable (release builds use the dynamic runtime).
    Unsigned, so SmartScreen will warn.
- [ ] **No game data in releases** (original Microsoft EULA). Each package has
      install notes: dependencies, and building the data from `mc2srcdata` with
      `scripts/build_gamedata.sh`.
- [ ] Process: `git tag -a vX.Y.Z`, push the tag to `pi`, review the draft,
      publish. Try it first with a pre-release tag (`v0.1.0-rc1`).

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

- Networking is stubbed (UI only). The multiplayer screens (`code/mp*.cpp`) still dereference `MPlayer->getPlayerInfo()`, which always returns NULL, in many places; they are unreachable because the main menu's Multiplayer button is disabled. Only the sites compilers flag are guarded so far.
- Sound: panning/doppler not implemented (alariq README).
- Some files may still be written outside the user config directory (alariq README TODO).
- WSLg's D3D12 GL driver shows a white screen with desktop GL; use `LIBGL_ALWAYS_SOFTWARE=1`.
- `-mission <name>` with a mission that does not exist crashes (SIGSEGV) instead of reporting an error.
- Fog byte semantics differ: actors (and crater/gvactor/mech3d) turn "no fog" (fogFactor 256) into 0, terrain (`quad.cpp`) uses 0xff. Original behaviour, kept; worth checking visually on the Pi.
- `Fatal()` / `ABL_Fatal()` return: `STOP` only logs (`InternalFunctionStop`, its `exit(1)` is commented out), so code after a fatal error carries on, usually into the NULL or full array it was guarding (`if (!p) Fatal(...); p->...`; ~35 sites, most of the remaining cppcheck `nullPointerRedundantCheck`, plus some `arrayIndexOutOfBoundsCond`). Making them abort (noreturn) would end the game cleanly instead, but any Fatal that fires harmlessly today would start quitting. Decided 2026-10-29: leave as is for now.
- `mech3d.cpp` helicopter dust cloud computes a motion-relative matrix and then draws with the plain one, unlike the water wake next to it. May be deliberate; unverified.
- Destroyed forests never reopen their pathing areas: `TerrainObjectType` declares its own `subType`, shadowing `ObjectType::subType`, so `TerrainObjectType::handleDestruction`'s `getObjectType()->getSubType() == TERROBJ_FOREST` (the base field, always 0) is never true and `openSubAreas()` never runs. Same in the original source, so shipped behaviour. Removing the shadow would fix it but changes gameplay (and `ObjectManager::getSpecificObjects` for terrain objects); not done, needs a decision.

## Log

Newest first. `YYYY-MM-DD: what (commit or PR)`.

- 2026-10-29: `duplInheritedMember`: vehicle tooltip name came from a shadowed field (`GVAppearance::objectNameId`), duplicate `BuildingType::teamId`, dead duplicate overrides removed; gate treats mclib/stuff and mclib/gosfx member hiding (and `init` overloads) as deliberate; forest pathing shadow recorded under Known issues (`22f7c98`); baseline 1060 -> 702.
- 2026-10-29: ABL team ids 500..508 indexed `Team::teams[8]` one past the end, body-location off-by-one in `MasterComponent`, `aAnimation` assignment leak, `PostcompVertex` assignment dropped `water` (`4ce088d`); `Fatal()`-returns noted under Known issues; baseline 1095 -> 1060.
- 2026-10-29: cppcheck `nullPointerRedundantCheck`: 8 real check-after-use bugs fixed (ABL static allocation test, inverted `GlobalMap::print` guard, logistics results, tutorial button, ...) (`2aa3591`); baseline 1114 -> 1095.
- 2026-10-29: Stereo panning works (branch `stereo-panning`): `panToStereoLevels()` (constant-power, full level at centre, 6 unit tests) feeds `Mix_SetPanning`; also fixed `gosAudio_GetChannelSlider` panning falling into the frequency case. Not yet checked by ear.
- 2026-10-29: cppcheck `uninitMemberVar` 787 -> 170 in three batches (`1d806e5`, `3d11ac6`, `ecab444`), including real copy-constructor bugs in `LogisticsScreen` and `aObject` and an unset `MultiPlayer`; gate keys ignore line numbers in messages (`f6fb2d2`); baseline 1731 -> 1114.
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
