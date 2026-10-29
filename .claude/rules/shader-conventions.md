# Shader conventions

Shaders in `shaders/` (and `shaders/include/*.hglsl`) are compiled at runtime by
GameOS. Every shader must be valid as **both** desktop GLSL 4.20 and GLSL ES 3.10.

## Rules

- **No `#version` line in shader files.** `buildShaderHeader()` in
  `GameOS/gameos/gameos_graphics.cpp` prepends `#version 420` (or `4xx` +
  `GL_ARB_shading_language_420pack`), or `#version 310 es` plus default precisions.
- **Float literals in float math:** `1.0 - x`, never `1 - x`. ES has no implicit
  int-to-float conversion.
- **`u` suffix for unsigned constants:** `0xFFu`, `1u << 4`.
- **No `const` initialised from a uniform or other non-constant expression.**
- **No desktop-only built-ins or types:** no `sampler1D`, no `texture1D`, no
  `gl_FragColor`, no geometry/tessellation stages.
- Precision: the ES header sets `highp` for float, int, `sampler2DArray`,
  `sampler3D`, `sampler2DShadow`. A new sampler type used on ES needs a precision
  line added to `buildShaderHeader()` **and** to `HEADERS` in
  `scripts/validate_shaders.py`.

## Validation

```bash
python3 scripts/validate_shaders.py
```

Expands `#include`, prepends both headers, compiles every variant (plain and
`ALPHA_TEST`) with `glslangValidator`. CI runs it on every push. Keep
`HEADERS` / `VARIANTS` in the script in sync with `buildShaderHeader()` and the
material variants built by `gosRenderer::init()`.

Passing the validator proves the shader compiles, not that it renders the same.
Visual changes need a run on desktop GL and on GLES (Pi, or Mesa llvmpipe via
`LIBGL_ALWAYS_SOFTWARE=1`).
