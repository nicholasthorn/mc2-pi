# Pi render performance: the backlog after the quick wins

Status: not started. Collected 2026-10-31 from the render reviews behind PR #5
and PR #6 and the bench / perf runs, which only lived in session transcripts
until now. Each item is a candidate, not a decision: measure it with
`scripts/bench.sh` before and after (`GPU-BENCHMARK.md`).

## Problem

PR #5 (CPU quick wins) took the Pi 400 GPU build from 113 to 179 fps on mc2_01
with vsync off, and PR #6 trimmed terrain lighting and texture churn. What is
left is structural: work drawn twice, work done on the CPU that the GPU could
do, and hitches from loading during play. These matter most on the Pi, whose
CPU is the bottleneck and whose SD card makes any mid-mission file read slow.

## Scope

In: the renderer and the per-frame CPU work that feeds it, on both backends.
Out: new visual features (shadows, upscaled textures: see the Pi quality tier
below and "Taking the useful parts of the remaster" in `PROGRESS.md`).

## Candidates, roughly by expected payoff

### Hitches

1. **Frame spikes from mid-mission file loading.** Every bench run has single
   frames of 300-500 ms against a typical 6-10 ms (`max_ms` in `results.csv`),
   and `bench.sh --perf` shows `FastFile::openFast` and zlib busy during
   gameplay on mc2_03. Find which frames spike and who opens files there
   (perf with dwarf call graphs, or log file opens with the frame number),
   then preload at mission start or cache. Likely the most visible problem on
   the Pi.
2. **LOD switches rebuild the model.** Each level-of-detail change deletes and
   recreates the appearance's model and looks its textures up by name; PR #6's
   hysteresis only makes it rarer. Prebuild every LOD once per appearance type.

### Drawing less

3. **Terrain is drawn twice.** The detail pass is a second draw of every
   terrain triangle, and water-detail likewise. Merge detail into the base
   terrain shader (and water-detail into water) with a second sampler.
4. **Terrain as a static GPU mesh.** Terrain vertices are rebuilt and streamed
   every frame. Keep them in a static buffer and update only what changes
   (lighting, visible window). Stopgap: a Pi preset with
   `GameVisibleVertices` 60 -> 40 (read from prefs in `code/mechcmd2.cpp`).
5. **Skip `makeLists` when the view has not moved** (`topLeftVertex`
   unchanged).

### Moving CPU work to the GPU

6. **Mechs transformed on the GPU.** The shader path exists but is off
   (`bShadersDrawPathEnabled = false`, `mclib/tgl.cpp:97`). Before enabling it:
   the FIXME that it binds only the first texture (`tgl.cpp`, around the
   `bDrawnByShader` draws), and fog is commented out there.
7. **Card-cloud particles** (gosFX) built on the CPU each frame: move the
   expansion into a vertex shader.
8. **Shadow projection** (`MultiTransformShadows`) on the CPU: do it in a
   shader.

### Overheads

9. **Uniform lookup by string on every draw.** Cache locations per material.
10. **`CHECK_GL_ERROR` in release builds**: each is a `glGetError`, which can
    stall a GLES driver. Compile it out of release builds (keep it under a
    debug option and in smoke runs).
11. **Mipmaps for terrain and object textures.** `glGenerateMipmap` is
    commented out, and `gos_FilterTriLinear` is mapped to `TFM_LINEAR`
    (`GameOS/gameos/gameos_graphics.cpp`, the texture filter switch). Mipmaps
    cut texture bandwidth for distant terrain, which matters on V3D.
12. **`MAX_MC2_GOS_TEXTURES`** (`mclib/txmmgr.cpp`): raise it on 4 GB Pis so
    the cache flushes less often; check memory with the GPU share.

### Left over from PR #5's review

- The GL state cache assumes nothing else touches GL state mid-frame; write
  that down at the GL call sites that bypass it.
- No general same-state draw batching in GameOS (only the effect batching).
- The 2x-radius light-cull margin is unproven: check for popping.
- Streaming buffers use about 10 MB against about 2 MB before.
- The four state caches could be one array; the light filter has no unit
  test.

## Pi quality tier (for remaster features)

Before taking higher-cost features from the remaster:

- Measure at the target resolution first: 1080p is about 4.3x the pixels of
  800x600.
- Higher-resolution textures need ETC2 on the Pi: V3D has no BC/DXT, and the
  remaster's packs are BC7.
- At most one low-resolution shadow map.
- Check what V3D offers with `eglinfo`.

## Affected areas

Renderer (both backends: every change must keep `MC2_GLES=OFF` and `ON`
working), engine (`mclib/`: terrain, txmmgr, tgl, gosfx), shaders, data loading
(item 1). No platform-layer changes expected.

## Verification

- `scripts/bench.sh` on the Pi (own display, vsync off) before and after each
  item: p50/p95/p99 and `max_ms` per mission, draw calls and triangles.
- Desktop GL and GLES under llvmpipe for correctness; screenshots from bench
  phase 2 checkpoints to catch visual changes.
- Idea, deferred ("not now", 2026-10-31): a CI job running `-bench` on the base
  branch and the PR, failing when draw calls or triangles grow by more than
  about 10%, frame times informational only.

## Open questions

- Which of items 1-4 is worth it after measuring: the spikes may dominate
  everything else for perceived smoothness.
- Does the GPU mech path render identically to the CPU path (texture, fog)?
- How much RAM can the texture cache take on a 4 GB Pi with the GPU split?
