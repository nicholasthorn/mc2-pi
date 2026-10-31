# GPU benchmark (scripted, deterministic, per-frame)

## Problem

`scripts/bench.sh` runs a stock mission for N gameplay frames and records one
number, the average fps from `[AUTOEXIT]`. That number is weak for profiling
renderer changes:

- **Not much happens.** Missions open with the camera parked over a few
  friendly units and no combat. There are no weapons, explosions, smoke,
  craters or burning wrecks, and the camera never shows a wide view of the
  terrain. The benchmark measures the cheapest frames in the game, so a change
  that hurts particles or terrain at max zoom out barely shows.
- **Not repeatable.** `frameLength = 1.0 / frameRate` (`code/mechcmd2.cpp`),
  so the simulation advances by the measured frame time. A faster build covers
  more game time in its 3000 frames than a slower one, and frame 2000 is a
  different scene on each run. The RNG is seeded from `timeGetTime()`
  (`code/mechcmd2.cpp`) and `time(NULL)` (`code/warrior.cpp`), so AI choices
  differ too. Two commits are not measured on the same work.
- **One number.** An average hides stutter (p99, worst frame), and does not
  say whether a frame is CPU bound (game logic, enqueue, `renderLists()`) or GPU
  bound, or how many draw calls and triangles it took.

For: anyone changing the renderer, shaders, textures or the frame loop, mainly
to measure the Pi 4/400 (GLES, V3D) and to check a change does not regress
desktop GL.

## Scope

In:

- A `-bench <scenario>` mode: deterministic simulation, scripted camera and
  combat, named segments, per-frame timing, a summary per segment.
- Per-frame counters from the renderer: draw calls, triangles, texture binds,
  texture upload bytes.
- GPU time per frame where the driver has timer queries.
- Screenshots at fixed frames, so you can see what was measured and compare
  images between commits and between the two renderers.
- `bench.sh` support for scenarios and segments, plus a compare script.

Out (for now):

- An in-game profiler HUD (listed as phase 4, optional).
- A new map or mission in `mc2srcdata`. The scenario reuses stock missions.
- Tracy or other third-party profilers (no new dependencies).
- Exact cross-architecture determinism. x86 and ARM floating point can make AI
  diverge over thousands of frames; same-machine comparisons across commits
  are exact enough, cross-machine numbers are the same scenario, not bit
  identical frames.

## Affected areas

- **Platform (GameOS):** main loop timing split, per-frame CSV, `[BENCH]`
  lines, screenshot (`glReadPixels` into a TGA/PNG in the output dir).
- **Renderer (GL and GLES):** counters in `gosRenderer`; timer queries
  (`GL_TIME_ELAPSED`, core on desktop GL; `GL_EXT_disjoint_timer_query` on
  GLES, may be missing on V3D, so it must be optional).
- **Game (`code/`):** fixed `frameLength` and seeds under `-bench`; a new
  `code/benchdirector.cpp` that drives camera and orders.
- **Data:** scenario files (FIT, parsed with `FitIniFile`) kept in the repo
  under `bench/` and copied into the game dir by `bench.sh`. No change to
  `mc2srcdata`.
- **Build/CI:** new source file in `SOURCES` with strict warnings; unit tests
  for the pure parts (scenario parsing, keyframe interpolation, percentile
  maths). Optionally a short bench run in `smoke.yml` as a crash check (not as
  a performance gate: llvmpipe numbers are not Pi numbers).
- **Shaders:** none.

## Approach

### 1. Deterministic simulation

Under `-bench`:

- `frameLength` fixed (scenario value, default 1/30 s) instead of
  `1.0 / frameRate`, at `code/mechcmd2.cpp` where it is set now. Frame N is
  then the same game moment on every run, whatever the fps. The wall clock is
  still what is measured.
- `gos_srand` and the `warrior.cpp` `srand` seeded with a scenario value.
- Audio stays on the dummy driver (as now) so mixing cost is not in the
  numbers.

Check: two runs of the same build write identical camera positions and unit
counts per frame (logged in the CSV), and the screenshots at the checkpoints
match byte for byte on the same machine.

### 2. Bench director and scenarios

`code/benchdirector.cpp`, called once a frame from `Mission::update()` after
gameplay starts. It reads a scenario like:

```
[Bench]
Mission = "mc2_xx"          ; a stock mission with large forces on both sides
FrameLength = 0.0333
Seed = 1234
RevealMap = 1               ; no fog of war: everything in view is drawn
Invulnerable = 1            ; nobody dies, so combat load stays steady
Frames = 3600

[Segment0]
Name = "overview"           ; wide, high camera pan across the map
Start = 0
Camera = "pan"  ...         ; keyframes: frame, position, rotation, zoom

[Segment1]
Name = "engage"             ; all movers ordered onto nearest enemy
Start = 600
Action = "attack_nearest"   ; Mover orderAttackObject on every team

[Segment2]
Name = "close-combat"       ; low camera following the biggest fight
...
[Segment3]
Name = "effects"            ; artillery / airstrikes on a point: particles, craters
...
```

- **Camera:** keyframes (frame, position, rotation, zoom) interpolated by the
  director and set directly each frame (`Camera::setPosition(..., false)`,
  `setRotation`, `zoomValue`), not through the "goal" smoothing, so the path
  does not depend on frame time. A "follow" key aims at the centroid of the
  fighting movers.
- **Action:** `attack_nearest` gives every awake mover an
  `orderAttackObject` on the nearest enemy; `move_to` sends a team to a
  point; `strike` calls the artillery/airstrike code on a point.
  `Invulnerable` extends the existing `invulnerableON` path (`mech.cpp`) to
  every team, so the fight lasts the whole run.
- **Segments:** each has a name and a start frame; timings are reported per
  segment, so a particle regression shows in "effects" and a terrain one in
  "overview".
- **Mission choice:** survey the stock missions for the largest
  two-sided forces near each other and pick one or two (open question below).
  Using two scenarios (a big open map, a dense base fight) is better than one.

Alternative rejected: writing the scenario as an ABL brain. ABL already has
camera functions (`execSetCameraPosition` etc. in `code/ablmc2.cpp`) and order
functions, but it would mean shipping modified mission data, ABL's camera
setters go through goal smoothing, and per-frame keyframes in ABL are awkward.
The C++ director keeps the data untouched and is unit-testable.

### 3. Measurement

Per frame, written to `bench-frames.csv` in the output dir:

| column | source |
|---|---|
| frame, segment | director |
| total_ms | main loop (as now) |
| logic_ms | `Environment.DoGameLogic()` |
| submit_ms | `mcTextureManager->renderLists()` and `draw_screen()` |
| swap_ms | `graphics::swap_window()` (on most drivers this is where a GPU-bound frame waits) |
| gpu_ms | timer query around the frame, read back 2-3 frames later; empty when unsupported |
| draws, tris, binds, upload_kb | `gosRenderer` counters (it already counts draw calls) |
| movers, effects | visible units and active particle systems: shows the content is the same |

At exit, one line per segment plus a total:

```
[BENCH] segment=effects frames=900 avg=22.1ms p50=21.4 p95=27.9 p99=33.0 max=41.2 gpu_avg=18.7 draws=812 tris=410k
```

`[AUTOEXIT]` stays as it is so `smoke.sh` and the current `bench.sh` keep
working.

Without timer queries (possibly V3D), `MC2_BENCH_FINISH=1` adds `glFinish()`
before swap so `swap_ms` becomes GPU time. It changes the pipelining, so it is
a diagnostic mode, not the default.

### 4. Screenshots

The scenario lists checkpoint frames; the director reads the back buffer at
each one (`glReadPixels`, both renderers) and writes
`shot_<segment>_<frame>.tga`. Uses:

- see what each segment actually measured;
- compare GL against GLES, and before/after a renderer change, with an image
  diff (a small script; exact match on the same machine, a threshold across
  drivers);
- a cheap visual-regression check that could run in the nightly smoke job.

### 5. Scripts

- `bench.sh --scenario bench/<name>.fit`: copies the scenario, runs
  `-bench`, parses `[BENCH]` lines into `results.csv` (one row per segment),
  keeps `bench-frames.csv` and screenshots per run.
- `scripts/bench_compare.py A B`: per segment, median of runs, spread, and the
  change; flags a change only when it is larger than the run-to-run spread.
  Optionally plots frame-time series from the CSVs (matplotlib if present).

### 6. Optional: live HUD

A toggled overlay (frame-time graph, logic/submit/swap/GPU ms, draw calls)
drawn through the existing debug text path. Useful when profiling by hand on
the Pi; not needed for the numbers.

### Outside tools that fit this

- **apitrace:** record one bench run, then `glretrace --benchmark` on the Pi
  replays only the GL calls. That separates driver/GPU cost from game CPU cost,
  and the deterministic scenario makes the trace reproducible.
- **RenderDoc** (desktop GL) on a screenshot frame for a per-draw breakdown.

## Phases

1. **Built** (2026-10-30): `-bench <csv>` (`gameosmain.cpp`): fixed 1/30 s
   `frameLength` and mission clock (`scenarioTime`), fixed seed at start-up and
   again at the first gameplay frame, keyboard and mouse events dropped (an
   edge-scroll would move the camera). Per-frame `total/logic/render/swap` ms,
   draw calls and triangles (counted at the `glDraw*` calls) written to the CSV
   at exit; `[BENCH] segment=all ...` summary (`utils/frame_stats.cpp`).
   `bench.sh` passes `-bench` and stores the summary in `results.csv`.
   ABL `getLogisticsTime` returns mission time (ms) and `isPlayingVoiceOver`
   returns false under `-bench`: 12 stock scripts time tutorial stages and
   camera moves with them, and audio plays in real time. Verified: two runs
   give identical draws/tris on every frame (mc2_01 900, mc2_03 3000). Not
   split yet: "render" is game enqueue and GL submit together. Pilot videos
   in the force group bar still depend on the measured frame rate.
   **Also built** (2026-10-31): `bench.sh --perf`, a `perf record -g` run
   per mission that samples gameplay frames only (the game drives perf's
   control FIFO), with a `MC2_FRAME_POINTERS` build option for call stacks.
2. **Built** (2026-10-31): `-benchscenario <fit>` played by
   `code/benchdirector.cpp` (logic in `code/benchscenario.cpp`, unit
   tested); format and tuning in `bench/README.md`. Differences from the
   plan above:
   - No stock mission starts with forces close together (median 1.5 to 6 km
     to the nearest enemy), so the director teleports the player's units and
     the nearest enemies into two lines instead of waiting for a fight.
   - Damage is off through `Team::noPain` (all teams, mechs and vehicles),
     not `invulnerableON` (player mechs only); `neverEndingStory` keeps the
     mission running. With both, all 25 units fight the whole 3600 frames.
   - The camera follows the fight's centre (smoothed) and circles it. The
     director projects that centre each frame and nudges the aim point until
     it sits at `ScreenX`, `ScreenY`, so it stays in shot at any yaw.
   - Screenshots are the whole drawable. Headless runs use an 800x600 Xvfb
     screen (`bench.sh`, `smoke.sh`): with FullScreen on, a bigger screen
     sized the viewport to it while the window stayed 800x600, so only a
     scaled corner of each frame was drawn.
   - `-bench` turns sound off: radio chatter decided whether
     `Radio::playMessage` reached `RandomNumber()` by whether the previous
     message had finished playing (real time), so combat runs drifted apart.
   - A `state` column (checksum of every unit) separates logic divergence
     from rendering differences; `MC2_BENCH_DUMP` dumps per-unit state.
   `bench.sh --scenario` writes `segments.csv` and a per-segment summary.
   Verified in WSL (llvmpipe): GL, GLES and Clang build; mc2_19 at 3600
   frames, two runs identical in game state on every frame and in all four
   screenshots; per-segment fps within 1 to 3 percent between runs; GLES
   gives the same state and correct screenshots. Known gap: in the effects
   segment the draw-call count can differ by about 0.3% between runs
   (triangles, state and images identical), probably how the alpha-sorted
   effect batching (PR #5) splits; not chased.
3. GPU timer queries and `bench.sh` / `bench_compare.py`.
4. HUD (optional).

## Verification

- Unit tests: scenario parsing, keyframe interpolation, percentile maths.
- Two runs of one build on one machine: identical per-frame `movers`/camera
  columns and identical screenshots; fps within a few percent.
- Desktop GL (x64) and GLES (Pi 4/400, and llvmpipe) both produce full CSVs;
  `gpu_ms` present on desktop GL, and on GLES if the extension exists.
- A deliberate slowdown (e.g. doubling particle count) shows up in the
  "effects" segment and not in "overview".

## Open questions

- Answered (phase 2): no stock mission has forces close together, so the
  director gathers them; all-team noPain keeps the AI fighting with orders
  repeated every 90 frames; neverEndingStory and clearing camera goals and
  movie mode each frame stop mission scripts interfering.
- A second scenario on a different map (daylight, buildings) would cover
  terrain and lighting cases mc2_19 (night, forest) does not.
- Does V3D on Pi OS expose `GL_EXT_disjoint_timer_query`? If not, is the
  `glFinish` mode enough, or should we rely on apitrace replays?
