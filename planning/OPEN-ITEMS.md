# Open items and review gaps (sweep of 2026-11-11)

Sources: all 145 PRs on `nicholasthorn/mc2-pi` (descriptions and comments),
the 24 mc2 session transcripts on this machine plus the 5 in the
`mc2-opengl-remastered` folder, `planning/PROGRESS.md` and
`planning/HARDWARE-TESTS.md` as on `pi/pi-port` today.

A PR counts as reviewed when a transcript shows `/code-review` or a review
subagent run on its branch, or its description records one. GitHub itself
shows no reviews on any PR: all reviews were run locally by agents.

Deferred items are as each PR recorded them. A later PR may have closed some;
the ones marked "(check)" are the likeliest.

## 1. PRs that did not get reviewed

### Code PRs merged with no review found

| PR | What | Risk |
|---|---|---|
| #4 | Stereo panning for positional sound | Low; never checked by ear either |
| #8 | Three-tier workflow, `scripts/check.sh`, smoke on PRs | Low (scripts and CI) |
| #13 | Building types keep their TeamID (sensor buildings) | Low; came out of the pi-port review, fix itself not re-reviewed |
| #20 | Review findings R1, P2, E1 (GLES point size, start marker, GlobalMap) | Low; same as #13 |
| #25 | mc2_15 segfault: ABL `getRelativePositionToObject` result unset | Medium (ABL / game logic) |
| #26 | Bench review findings T1, T3, T5 | Low |
| #28 | Review fixes of #22 (ABL `print()` overflow, bounded append) | Medium (ABL string handling) |
| #30 | UI smoke walks, slice 1 (`-uismoke`) | Low; the PR says the review "has not been run yet" |
| #38 | STOP plan step 3: NaN ABL move target warns instead of STOP | Medium (changes failure behaviour) |
| #45 | CI skips docs-only pushes (`ci_skip.sh`) | Low |
| #47 | `smoke.sh` default walks on Omnitech data | Low |
| #49 | CI also skips pushes that only merge pi-port | Low; not run through `actionlint` either |
| #64 | `comparePilots` lets rank decide first | Low/medium (changes pilot ordering) |
| #72 | Quest first run: args file path, buffer-map fix (4 to 58 fps) | Medium (renderer, GLES buffer mapping). #73's review diffed against this branch, so it never covered it |
| #80 | Eight more Omnitech campaign packs; `smoke.sh` mission names with spaces | Low |
| #102 | Android prefers AAudio, then OpenSL | Low; this fixed the Quest's ~1 Hz buzz (confirmed by the user 2026-11-11) |
| #105 | Omnitech splash centred in the larger front end | Low |
| #131 | Shorter smoke runs on PRs that leave multiplayer alone | Low; its short path had never run on GitHub when merged |

### Partly reviewed

- **#32 / #39** (UI smoke slice 2, re-landed as #39): only the "buy fits" fix was reviewed, not the whole slice.
- **#130** (lobby protocol): the second review's fixes were not reviewed again (says the PR).
- **#119, #113**: `/simplify` never ran as its own step (review did).

### Reviewed only after merging

- #7 (bench phase 2, and bench phase 1 which landed with no PR): reviewed in #12.
- #22 (`MC2_STOP_EXITS`): reviewed in #28.
- #57, #58 (cppcheck alloca, copy/assign): reviewed in #60, no regressions found.

### Open PRs

- **#145 is not reviewed in its current form.** `/code-review high` ran at 01:26 on the first approach (moving the terrain window). The branch then changed to a different fix (the GPU clips ground triangles near the eye) and no review has run since.
- #121: `/code-review high`, ten findings fixed.

#142, #143 and #144 had a fresh review on 2026-11-11 and merged later that day. It found a real bug in #143 (below FogFull the GPU shader left meshes clear where the CPU path fogs them fully), fixed before the merge.

### Needed no review

Dependabot #1, #2, #3, #76. Docs or data only: #10, #27, #29, #40, #52, #54, #62, #68, #70, #106, #107, #109, #116, #122, #137, #139. Closed without merging: #63, #71, #87, #90.

## 2. Things left to do

### Open PRs (most urgent)

- **#145 near-eye ground clipping:** ready, no conflicts, checked with captures on a Quest 2 (mc2_16 looking down, mission 1 fly-in). Left: a review of the new approach, and a check with the headset worn.
- **#121 mercenary mode:** draft, **conflicts with pi-port since 2026-11-05**, and nobody has played it.

Merged since the sweep began: #142 (VR HUD frame), #143 (GPU mesh height fog), #144 (shape pools, now sixteen times larger, with a `PAUSE` the first time one runs out in each mission). For #144 the extra memory is not measured on the Pi, though the TinyGeom heap is reserved with `mmap`, so unused pool space should cost nothing.

### Hardware and hand checks

`planning/HARDWARE-TESTS.md` had **72 unticked items** at the sweep (69 after the three ticked on 2026-11-11): 20 in-game on any PC, 9 multiplayer, 9 releases, 8 Quest VR, 8 Quest playtest fixes, 5 Pi, 5 Omnitech front end on Quest, 3 STOP message box, 3 path-finding worker, 1 VR interface rate, 1 network transport. The ones PRs keep pointing at:

- Quest: #127's 1280x960 interface default with nothing set by hand; #134 a mission played to its end and into the next (nothing since its review fixes); #128 the marker square in explosions; #141 the round terrain window; #93's two review fixes; #97's last two commits.
- Network: a BCrypt peer (Windows) to a BoringSSL peer (Linux/Quest); two machines; a Quest as client; anyone clicking through the lobby, mech selection and chat by hand.
- Done: loading a saved game works (#124's path check; confirmed by the user 2026-11-11). Still no automated run or test loads a save.
- Done: on the Quest, saving with no keyboard (a new save gets an unused name, OK works) and loading that save (#96; confirmed by the user 2026-11-11).
- Releases: v0.2.0-rc1 Phase 5 hand runs on Windows, Linux x64 and a Pi 400 never happened.
- Pi: the other three smoke missions on the GPU build, `--perf`, `--scenario`.

### Known bugs, not fixed

**VR / Quest**
- Chain-link fences are solid bands at a distance (alpha-weighted mipmaps tried in #87, closed: fences barely changed).
- From #86 / #88 reviews, still open: ground under the 2D panel cannot be clicked; at a tilt of about 65 degrees far ground ahead is off the game camera's screen; a fast trigger pull during a quick head turn can lose the click; clouds are drawn from the game camera; a script's FOV lasts until the next game step; no unit test for `isInWindow` or the head hold logic.
- Bars and icons keep pixel size (look large on distant mechs); units behind the game camera get no overlay.
- Spotlights and windows are left out of the eyes; untextured shapes have no GPU-path fix.
- Tents show no hover name (not compared with the flat game).
- `gos_xr.cpp`: a failed `xrBeginFrame` after `xrWaitFrame` leaves the frame unbegun.
- Controller gaps: no input for called shots, fire from position, or groups (Ctrl+number has no other way in); right stick click can wipe a selection.

**Omnitech / front end**
- Hire pilots has no handler; salary and balance labels are blank (#115).
- Front-end movies keep 800x600 and stretch; the credits movie rectangle is hard-coded 600 high; promotion screen unchecked.
- (check) #101's deferred list (KIA box face, portraits at 640/800 wide, Kitfox and Direwolf in the Mechlopedia, garbled mech icons, `MissionBriefingScreen` stale handle): #111 fixed icons and portraits, the rest may still be open.

**Multiplayer**
- A repair bay's spent points reach a client as damage, so an empty bay could blow up there (#123, #126).
- A client whose copy of a shot lands elsewhere sees a set-off mine vanish with no explosion.
- Strikes from a client are not charged resource points; ammo is not in the post-repair state; gates are decided per machine.
- Chat has no rate limit (a flood can get other clients dropped); holding a C-Bills spinner resends lobby state and clears everyone's ready every frame.
- The data hash misses a map's art and scripts; the Quest multicast lock is held for the app's whole life; no joining by address; designer variants only.
- `mc2_m02a` and `mc2_m02b` have never been run; no mode has run to its real time limit.
- GNS prints an assertion after a failed `listen` (not traced).

**Engine, data, other**
- #126: `PAUSE: txmmgr: Bad texture handle!` once as the results screen closes (not traced).
- #125: a terrain cache folder already poisoned by the magenta bug is still trusted (delete the `.txm` / `.lst` files by hand).
- #117: on Windows, pressing OK on `readFastRAW`'s retry can overflow a too-small buffer (predates the PR).
- #31: an uncompressed texture inside a mod's LZ FST would still fail the `readRAW` path (no known data does this).
- The data build is not reproducible: `aseconv` writes a slightly different `tgl.fst` each run (probably uninitialised padding).
- PROGRESS "Known issues": no doppler; some files may be written outside the config directory; helicopter dust cloud uses the wrong matrix (maybe deliberate).

### Process, CI and release follow-ups

- CI: a cppcheck run that fails part-way is still accepted; stdout line-buffering for complete crash logs (#18). ccache for the data-tools job (#131) and for fresh worktree builds (measured at about 3.5 min saved on the networking libraries, recipe untested). A smoke run every 10 merges (#132, your call). Sanitized smoke on arm64. CI benchmark regression job (you said "not now").
- Releases: AppImages (Phase 3); v0.2.0 with Pi results (Phase 6); shaders not inside the APK; APK `versionCode` stays 1; a Quest job failure blocks the whole draft (#104).
- Tests: legacy `mclib/stuff` `TestClass()` self-checks; GPU benchmark phases 3-4; UI smoke never opens save/load, salvage or mission selection, or changes a weapon in the mech lab; a `STOP` off the main thread shows an unparented box.
- Remaster leads not ported: `f1eaa6ee` (mission with no MOVE data), `3c8dd69e` (briefing minimap alpha), `51ddcf25` (`hiResOffsetY` on the video box); plus four that only matter with `MC2_STOP_EXITS=0`.
- Remaster repo's own items (trilinear filter forced off, agent docs, tooling, forward-ports, uncommitted `linux-build` branch).
- Housekeeping: WSL disk (about 9 GB of remaster and mod data, old build dirs); leftover remote branches from merged PRs.

### Ideas offered and not taken up

- A browser (WebAssembly + WebGL2) build spike (2026-11-01 assessment: moderate effort).
- macOS on Apple silicon (plan only, #109).
- The open questions in the multiplayer proposal (#106): co-op campaign timing, a relay server, player counts on Quest.
