# Splitting game logic from rendering

Status: audited 2026-11-05, nothing built. A static read of the code, not a
trace: run ThreadSanitizer before trusting the "no conflict" lists.

## Why

In VR the game steps at 30 Hz and the headset draws at 72 Hz, on one thread.
A game step that runs long makes that frame miss its slot. On the Quest 2,
mc2_19 bench, 97 % of frames over 20 ms were step frames, with logic at about
14 ms against 5.2 ms for a typical step. Render and present take about 6.7 ms
of the 13.9 ms frame, leaving about 7.2 ms for logic.

The largest part of a spike is path-finding: 5.9 ms in those frames against
0.3 ms typically. `shape_xform` is the largest steady cost, about 1.6 ms a step.
The bench reorders units every 90 frames, which probably creates more path
bursts than normal play: check a played mission before relying on these.

Engines usually keep a game thread and a render thread, passing data through
render-side copies (Unreal's scene proxies), with logic at a fixed step and
interpolation between steps. This engine has no such boundary.

## Option A: path-finding on a worker

Start the queued paths after a step ends, join before the next step begins, while
the main thread renders. **Small: about 100 lines (estimated).**

`calcMovePath` runs no ABL. `triggerAlarm` and `setAlarmTacOrder` only write
pilot fields. The path maps, queue, `LastMoveCalcErr` and the statics in
`mclib/move.cpp` are the worker's alone while it runs, because nothing else
uses them between steps. Heaps are `malloc` underneath, though the push and pop
around each call are unlocked globals (`gameos.cpp:133`).

To change:

1. Debug cells: `calcMoveGoal` writes them (`mover.cpp:4874`, `4907`) and
   `Mission::render` clears them (`mission.cpp:932`). `getCellDebug` has no
   callers, so remove or guard them.
2. Radio: defer `radioMessage` (`warrior.cpp:3124`) to the main thread after the
   join. It uses `radioHeap`, packet data and the RNG.
3. RNG: `mover.cpp:4570` draws from the one global sequence. Give path code a
   safe source. MSVC's `rand` state is per thread, so a worker there would ignore
   the seed (from outside knowledge, not checked here).
4. Allocate `openList` at load (`move.cpp:3578`, `6166`), and make the heap
   stack `thread_local`.
5. `Fatal` and `STOP` on the worker (`movemgr.cpp:135`, `warrior.cpp:3250`, `3279`).
6. Move the call from `mission.cpp:540`.

To accept: `drawWaypointPath` reads a path the worker writes (key-gated, a torn
line at worst), and pathlocks share a `GameMap` word with mine bits read in render
(`quad.cpp:144`; the mine bits themselves do not change).

Behaviour: today paths run after `missionInterface->update` in the same step.
With the worker, a player's order paths one step later (33 ms), and `scenarioTime`
and the RNG order shift. State hashes need a new baseline. Not lockstep, so
multiplayer does not need them to match across machines (`NETWORKING.md`).

Not fully traced: `calcMoveGoal` lines 4500 to 4970.

**ThreadSanitizer found more than this read did.** `calcMoveGoal` sets the mover's
`position` and cell position to each candidate cell to test line of sight, and
`getLOSPosition` changes `inView` and rotates nodes, all while frames draw, so a mech
could be read at a candidate cell. The fix goes with the worker
(`perf/vr-path-worker`): line of sight takes the candidate as its own field, and on
the worker uses node heights refreshed just before it starts.

Running the same split on idle frames on the main thread needs only step 6 and
has no races, but a burst has to fit about 6.5 ms per frame.

## Option B: logic on its own thread

**Large.** Not one blocker but the whole seam, with four hard parts.

| Part | Why it is hard |
|---|---|
| The step builds the frame | `Mission::update` runs `clearArrays`, `eye->update`, `land->geometry`, appearance updates and transforms, so these must move out first |
| The TGL pools | `MultiTransformShape` fills them in update, render reads them and `mission.cpp:968` resets them. `TG_Shape::Render` returns unless `lastTurnTransformed == turn` (`tgl.cpp:2710`) |
| Picking reads render data | `findObjectByMouse` and `PerPolySelect` use transformed screen vertices from the last step (`tgl.cpp:2959`, `2981`); the terrain cursor uses `inverseProject`; `vrPickWorld` runs in render and walks live movers |
| The interface | About 14 classes read live objects widely. It could stay with logic, but it draws through GL |

Other work:

- **GL from logic:** a texture cache miss creates textures through
  `get_gosTextureHandle` (`txmmgr.cpp:2153`), from `setupTextures`, craters and
  `WeaponBolt::update`. `mcTextureManager->update` destroys idle ones each step
  (`txmmgr.cpp:1785`). `MechIcon::update` locks textures on a damage change. All
  need a queue to the render thread.
- **Render writes game state:** `damageThisFrame` (`mech.cpp:6557`), which picks
  hit gestures; `setSelected(false)` and `setTargeted(false)` on buildings, gates
  and turrets, which ABL can read (`ablmc2.cpp:6865`); and `windowsVisible`.
- **Input:** edges must queue until a step consumes them, or a press at 72 Hz is
  lost at 30 Hz.
- **Camera:** the game camera stays in logic (activation at `objmgr.cpp:1775`,
  `seesForGame`, sound, ABL). VR already keeps it apart from the eyes
  (`terrain.cpp:1033`).
- **Shared type data:** `SetTextureHandle` on `myType` (`msl.cpp:1733`).
- **Globals:** about 630 distinct `extern` data names in `code/` and `mclib/`
  (rough grep), and `frameLength`, `eye`, `land` and `soundSystem` are each used
  in 38 to 49 files.

What helps: no render function calls the RNG (by function name, not a call graph).
The VR mesh path needs little per object: matrices, lights, alpha. Elevation is
static. Terrain projection is already redone per eye. The multiplayer capture
(`sendMoverUpdate`) cannot double as a snapshot: it packs cell positions, with no
pose, animation or effects.

Snapshot sizes: per-object state is small (a node's matrix about 64 B, up to 256
nodes). Terrain is about 0.37 MB at 3600 vertices, far less if only lit and fog
fields are copied. The CPU-path pools hold about 2.3 MB, but VR already leaves them
out of the eyes. Effects are a live tree of up to `maxParticleCount` particles.

A found problem either way: `controlgui.cpp:451`, `gametacmap.cpp:193`, `339`,
`forcegroupbar.cpp:357` and `mechicon.cpp:1067` add `frameLength` during render.
With a step's `frameLength` and a 72 Hz render, those timers ran about 2.4 times
too fast in VR (inferred, not observed). The interface now redrawing at about step
rate should bring them close.

## Step 3 design: the retained mesh list

Status: design only. Reasons are read from the code; gains are not measured.

**Moving transforms alone buys nothing on one thread.** A step frame must fit
logic, render and present into one 13.9 ms slot. Work moved from logic into that
frame's render still lands in the same slot. It only helps if it can run in a later
frame, and today it cannot: `clearArrays` empties the texture manager's reservations
at the start of the step, and the step refills them, so the step frame has to draw
what that step built.

**What is worth building instead:** the VR mesh path already has a snapshot.
`TG_RenderShape` holds the buffers, world matrix, lights and alpha, and only `mvp` and
the viewport depend on the eye. Today each eye rebuilds it by walking
`ObjectManager->render`, through appearances and `TG_MultiShape::Render` down to
`TG_Shape::Render`, which costs most of `world_objects` (1.65 ms a frame on the Quest)
and is repeated on all of the 2 or 3 frames per step that draw the same state.

The design:

1. **Build once per step.** Where `MultiTransformShape` reserves today (`tgl.cpp:2577`),
   append a record to a mesh instance list: buffers, declaration, `shapeToWorld`,
   lights, alpha, texture node, flags. It needs no vertex loop (step 2).
2. **Draw from the list.** Each eye walks the list and works out `mvp` from
   `shapeToWorld` and its own view-projection. `TG_MultiShape::Render` returns early
   for shapes on the list, so `ObjectManager->render` is left with only what is not a
   mesh: bolts, effects, bars.
3. **Sort within a texture node by vertex buffer,** so copies of a shape sit together.
4. **Instance them.** Draw each run with `glDrawElementsInstanced` and per-instance
   matrices. This is what should let unit counts grow, because cost then follows the
   number of shape types, not the number of units. Desktop GL 4.0 and GLES 3.1 both
   have it, but `gos_tex_vertex_lighted.vert` would need per-instance inputs, and each
   instance's lights and alpha have to travel with it.

Cases that need care:

- **forceZ and HUD shapes** (the compass) draw per eye with their own matrix: leave
  them on the old path.
- **Stale records:** a list holds buffers until the next step, so a destroyed object's
  shape must not free them first. Check the shape lifetimes in object removal.
- **Pause:** `updateAppearancesOnly` must still build the list.
- **`refreshTextures`:** `TG_MultiShape::Render` writes texture handles into shared type
  data (`msl.cpp:1733`). Handle mechs' per-instance paint in the record, and watch cache
  misses, which can create textures.
- **Interface cameras** (`eye->drawsToInterface`, the mech view in menus) stay on the
  old path, as `vrMeshPath` already separates them.

**Why this serves step 4:** the list is the snapshot a render thread needs for meshes.
With a second list and a swap, the logic thread fills one while the render thread draws
the other.

**Terrain, later:** `land->geometry` is 0.4 ms per step and `projectForEye` redoes
every vertex per eye (`Camera::projectZ` is high in the headset profile). A static
vertex buffer of elevations with per-eye projection in a shader would remove that,
leaving lighting and fog in the step. Its cost grows with the terrain window, so this
matters most for bigger maps. It changes how terrain draws, so it is its own piece of
work.

**Texture cache spikes:** `terrain_geom` is under 1 ms up to the 98th percentile but
11.7 ms at the 99.5th, probably a cache miss decoding and uploading a texture (inferred).
A small per-frame upload budget in idle frames would flatten those, and the same queue
is what a logic thread needs to reach GL.

## Order

1. Option A, with ThreadSanitizer on a desktop `MC2_VR_PATHS=1` run.
2. Skip the per-vertex loop in `MultiTransformShape` for GPU meshes where nothing
   reads the result. Picking reads screen vertices, so that is the catch.
3. The retained mesh list above, then instancing.
4. Decide on the rest of B from what 1 to 3 measure. By the averages above, 1 and 2
   bring bad frames to about 6.5 ms of logic, which roughly fits the budget. Step 4
   may then be for robustness, plus interpolation, more than speed.
