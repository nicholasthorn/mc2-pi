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
f Lift = 400.0            ; aim this far short of it, raising it above the HUD
l NumSegments = 2

[Segment0]
st Name = "overview"      ; no spaces
l Start = 0               ; segment 0 must start at 0
l Shot = 450              ; frame to screenshot, -1 = none
l StrikeEvery = 0         ; artillery every N frames, 0 = none
l NumKeys = 2
f[6] Key0 = 0, 0, 0, 50, 0, 1600     ; frame, x, y, tilt, yaw, altitude
f[6] Key1 = 599, 0, 0, 50, 120, 1600
...
FITend
```

The camera moves linearly between keys (yaw the short way round) and holds
before the first and after the last. Tilt is 10 to 88 degrees above the
horizon; the game caps altitude at 1500 to 1600 depending on tilt.

## Writing one

Run it once and read the log and the screenshots:

- `[BENCH] gathered N player and M enemy units at x, y`: who took part.
- At each segment start, `[BENCH] frame F: n of N fighting, centre ..., mean
  spread ...` and the camera's position, tilt and yaw. A large spread means
  units wandered off; fewer fighting means some were disabled anyway.
- `logs/*.frames_<segment>.tga`: what the segment looked at. If the fight
  is under the HUD, raise `Lift`; if it is off the top, lower it.

`MC2_BENCH_DUMP=<frame>[,<count>]` prints every unit's exact state for
`count` frames (default 300) from `frame`, to find the first unit that
differs between two runs whose `state` columns disagree.

## Scenarios

- `mc2_19.fit`: mc2_19 has the most enemies of any campaign mission (82).
  The player's lance and the 20 nearest enemies fight at the player's start.
  Segments: `overview` (high, panning), `engage` (medium, circling),
  `closeup` (low), `effects` (artillery every 20 frames).
