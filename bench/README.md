# Benchmark scenarios

Scripted battles for `scripts/bench.sh --scenario bench/<name>.fit`. Stock
missions start quiet (the camera parked over a few units, nobody fighting), so
a scenario stages a battle inside one and flies the camera over it. The
game's bench director (`code/benchdirector.cpp`) plays it, once per frame:

- on the first frame, moves the player's units and the `Gather` enemies
  nearest the arena into two lines `LineGap` apart, `Spacing` between units;
- orders every unit to attack the nearest enemy (again every `ReorderEvery`
  frames), with damage off for every team and the mission set never to end,
  so the fight lasts the whole run;
- flies the camera through the keyframes and calls in artillery;
- tells GameOS where each segment starts (each gets its own
  `[BENCH] segment=<name>` line and a row in `segments.csv`) and when to save
  a screenshot.

Everything depends only on the gameplay frame number, so every run of a
scenario renders the same frames: `-bench` fixes the game step and the
random seed, turns sound off and ignores input. The frame CSV's `state`
column is a checksum of every unit's position and status; two runs on one
machine should match on every frame.

## Format

FIT, read with the engine's own parser. Distances are world units, angles
degrees, frames gameplay frames (30 per game second).

```
FITini
[Bench]
st Mission = "mc2_19"     ; bench.sh's default --missions
l Frames = 3600           ; bench.sh's default --frames
f ArenaX = 1000.0         ; optional (both or neither): default is the centre
f ArenaY = -2000.0        ;   of the player's units, which is passable ground
l Gather = 20             ; enemies moved into the arena (nearest first)
f LineGap = 600.0
f Spacing = 90.0
l ReorderEvery = 90
f StrikeRadius = 350.0    ; artillery lands within this of the arena centre
b Follow = TRUE           ; camera keys relative to the fight's centre
f ScreenX = 0.5           ; hold that centre here on screen (fractions from
f ScreenY = 0.4           ;   the top left; default 0.5, 0.5)
l NumSegments = 2

[Segment0]
st Name = "overview"      ; A-Z a-z 0-9 _ - only, unique, not "all"
l Start = 0               ; segment 0 must start at 0
l Shot = 450              ; frame to screenshot, -1 = none
l StrikeEvery = 0         ; artillery every N frames, 0 = none
l NumKeys = 2
f[6] Key0 = 0, 0, 0, 50, 0, 1600     ; frame, x, y, tilt, yaw, altitude
f[6] Key1 = 599, 0, 0, 50, 120, 1600
...
FITend
```

Only `NumSegments` and each segment's `Start` are required. A missing key
keeps its default: `Frames` 3600, `Gather` 16, `LineGap` 600, `Spacing` 100,
`ReorderEvery` 90, `StrikeRadius` 300, `Follow` TRUE, `ScreenX` / `ScreenY`
0.5, `Shot` -1, `StrikeEvery` 0, `Name` `seg<i>`. The game refuses a
scenario (`[BENCH] ERROR: ...`, and bench.sh fails the run) with a negative
`Gather`, a bad, repeated or too long segment name (31 characters at most),
a `Mission` over 63 characters, a camera key without exactly 6 values, or a
`Shot` outside its own segment (from its `Start` up to the next segment's,
or up to `Frames` for the last one).

The camera moves linearly between keys (yaw the short way round) and holds
before the first and after the last. Tilt is 10 to 88 degrees above the
horizon; the game caps altitude at 1500 to 1600 depending on tilt.

The camera turns about its aim point. Each frame the director projects the
fight's centre with the last frame's camera and moves the aim point a little
towards putting it at `ScreenX`, `ScreenY`, so a key that only changes yaw
circles the fight and keeps it in shot at any tilt or height. For a steady
circle, step yaw by the same amount per frame across keys and segments, with
keys at most 180 degrees apart (`mc2_19.fit` turns 120 degrees every 600
frames).

## Writing one

Run it once and read the log and the screenshots:

- `[BENCH] gathered N player and M enemy units at x, y`: who took part.
- At each segment start, `[BENCH] frame F: n of N fighting, centre ..., mean
  spread ...` and the camera's position, tilt and yaw. A large spread means
  units wandered off; fewer fighting means some were disabled anyway.
- `logs/*.frames_<segment>.tga`: what the segment looked at, the whole
  window. If the fight is too small or too big, change the altitude; if it
  sits behind the HUD, lower `ScreenY`.

`MC2_BENCH_DUMP=<frame>[,<count>]` prints every unit's exact state for
`count` frames (default 300) from `frame`, to find the first unit that
differs between two runs whose `state` columns disagree.

## Stage times and `-benchfov`

Every `-bench` run also times named stages inside the frame
(`gos_BenchStageStart`/`Stop`), each a `<name>_ms` key on the `[BENCH]` lines
and a column in the frame CSV, averaged per frame:

| Stage | What | Part of |
|---|---|---|
| `camera` | `eye->update()` | `logic_ms` |
| `terrain_geom` | `land->geometry()`: projects every visible terrain vertex | `logic_ms` |
| `objects` | object and appearance update | `logic_ms` |
| `shape_xform` | `TransformMultiShape`: shapes to clip space, inside `objects` | `logic_ms` |
| `world_render` | `eye->render()`: world enqueue and GL submit | `render_ms` |
| `ui_render` | interface and floating help | `render_ms` |

`terrain_geom`, `shape_xform` and `world_render` are the work VR stereo
would repeat per eye (`planning/future-features/VR-TABLETOP.md`).
`-benchfov <degrees>` forces the camera's field of view for the whole run
(a headset sees about 100 degrees, the game 40), to measure the wider view.

## Scenarios

- `mc2_19.fit`: mc2_19 has the most enemies of any campaign mission (82).
  The player's lance and the 20 nearest enemies fight at the player's start.
  The camera circles the fight the whole run (one turn a minute). Segments
  change its height: `overview` (high, 600 frames), `engage` (medium, 1200),
  `closeup` (low, 600), `effects` (artillery every 20 frames, 1200).
