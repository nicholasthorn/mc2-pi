---
name: mc2-renderer
description: "Work on the MC2 renderer: the desktop GL and GLES 3.1 backends, graphics.h, gl_utils, textures and texture upload, shader loading and #version headers, render states, the frame flow from enqueue to gos_RendererEndFrame. Use when changing GameOS/gameos/gameos_graphics.cpp, gos_render.cpp, utils/gl_utils.cpp, utils/graphics.h, shaders/, or mclib/txmmgr.cpp, or diagnosing something that draws wrong on one backend."
---

## Scope

The GL layer under the game: context creation, the `gosRenderer` class, textures,
materials and shader variants, render states, immediate-style draw calls
(`gos_DrawTriangles` etc.), text, and the GLES 3.1 path for the Raspberry Pi.
Excludes the game's own render queue logic (`mclib/txmmgr.cpp`,
`mclib/mlr/`), which only feeds this layer.

Line numbers drift: grep the symbol before citing a line.

## Key files

- `GameOS/gameos/utils/graphics.h`: **the only GL include.** Desktop: GLEW + GL.
  GLES: `GLES3/gl31.h` + `gl2ext.h`, plus `#define`s mapping desktop-only enum
  names used by shared tables, and a no-op `glPolygonMode`.
- `GameOS/gameos/gos_render.cpp`: SDL window + context. GLES requests an ES 3.1
  context; desktop requests core 4.0 with the debug flag.
- `GameOS/gameos/gameos_graphics.cpp`:
  - `gosTexture`, `gosTexture::createHardwareTexture()`, `convertIfNecessary()`
  - `gosRenderMaterial`, `gosMaterialVariation` (shader variants, `ALPHA_TEST`)
  - `gosRenderer::init()` (builds the material variants), `applyRenderStates()`,
    `drawQuads/drawTris/drawIndexedTris/drawText`, `beginFrame/endFrame`
  - `buildShaderHeader()`: picks `#version 420`, `4xx` + `420pack`, or
    `#version 310 es` + precisions from the driver's GLSL version
  - `gos_CreateRenderer()`, `gos_RendererBeginFrame()`, `gos_RendererEndFrame()`
  - `OpenGLDebugLog` (desktop only; `KHR_debug` callback)
- `GameOS/gameos/utils/gl_utils.cpp`: texture create/update/readback.
  `updateTexture()` repacks 8-bit uploads whose channel count differs from the
  texture on GLES; `getTextureData()` reads back through an FBO + `glReadPixels`
  on GLES (no `glGetTexImage`). Immediate-mode debug helpers are compiled out on ES.
- `GameOS/gameos/utils/shader_builder.cpp`: compiles/links GLSL programs.
- `shaders/*.vert|frag`, `shaders/include/*.hglsl`: see `.claude/rules/shader-conventions.md`.
- `mclib/txmmgr.cpp`: `MC_TextureManager` (`mcTextureManager`): game texture
  cache and the triangle lists flushed by `renderLists()`.

## Frame flow

1. Game and engine render functions **enqueue** triangles into
   `mcTextureManager`'s lists (nothing hits GL yet).
2. `code/gamecam.cpp` (and `code/simplecamera.cpp`) calls
   `mcTextureManager->renderLists()`, which issues the `gos_Draw*` calls.
3. `gos_RendererEndFrame()` ends the frame (called from `gameosmain.cpp`).
4. `mcTextureManager->update()` (from `code/mission.cpp`) evicts and reloads
   textures between frames.

A draw added inside an enqueuing function runs *before* the queued terrain/objects
and gets painted over. Draw after `renderLists()` if it must be on top.

## GLES differences that bite

- **Format conversion on upload:** desktop GL converts RGBA8 data into RGB8/R8
  textures; ES raises `GL_INVALID_OPERATION`. Upload data in the texture's own
  format (see `updateTexture()`).
- **No `glGetTexImage`, no `glPolygonMode`, no `GL_QUADS`, no 1D textures, no
  geometry/tessellation shaders, no `GL_CLAMP`.**
- **Single-channel float format** is `GL_RED`, not `GL_R`.
- **No `KHR_debug` callback wired on ES**: check `glGetError()` explicitly while
  debugging a GLES-only problem.
- Shaders: no implicit int-to-float, `u` suffixes, precision qualifiers. Run
  `python3 scripts/validate_shaders.py`.

## Verify

- Build with `MC2_GLES=OFF` and `MC2_GLES=ON`.
- `python3 scripts/validate_shaders.py`.
- Run: desktop GPU for desktop GL; the Pi, or `LIBGL_ALWAYS_SOFTWARE=1 ./mc2` with a
  GLES build, for GLES. Ask before launching the game.

## Recipes

**Add a GL call that does not exist on ES:** wrap it in `#ifdef MC2_GLES` inside
GameOS (never in `code/`), give the ES path an equivalent or a documented
fallback, and add any missing enum to `graphics.h` only if it is never actually
passed to GL on ES.

**New texture format:** add it to the format tables in `gl_utils.cpp`, check the
internal format / format / type triple is legal on GLES 3.1 (ES 3.1 spec table
8.2), and check `updateTexture()` handles uploads into it.

**New shader or variant:** add the file, keep it valid for both dialects, add the
variant to `VARIANTS` in `scripts/validate_shaders.py` if `gosRenderer::init()`
gains one.
