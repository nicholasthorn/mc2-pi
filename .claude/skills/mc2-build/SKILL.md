---
name: mc2-build
description: "Work on the MC2 build and CI: CMakeLists.txt, cmake/Find*.cmake, the MC2_GLES option, compiler flags and warnings, dependencies, the GitHub Actions matrix (.github/workflows/ci.yml), Dependabot, and the per-platform BUILD-*.md docs. Use when the build breaks on one platform, when adding a source file, dependency, CMake option or CI job, or when following planning/QUALITY-ROADMAP.md."
---

## Scope

Configuring and building the game and tools on Windows MSVC, Linux x64
(GCC/Clang) and Linux arm64, and the CI that checks all of them. Conventions are
in `.claude/rules/build-conventions.md`; this skill is the map and the recipes.

## Key files

- `CMakeLists.txt`: global flags (`-fsigned-char`, warning set; no
  `-fpermissive`), `MC2_STRICT_WARNINGS` for new files, `LINUX_BUILD` / `PLATFORM_WINDOWS` / `USE_ASSEMBLER_CODE=0`, the
  `MC2_GLES` option (`PkgConfig::GLESV2` vs `GLEW::GLEW OpenGL::GL` in
  `MC2_GL_LIBS`), `MC2_SANITIZE` (e.g. `address,undefined`, GCC/Clang only),
  `MC2_BUILD_TESTS`, subdirectories, and the hand-written `SOURCES` list for
  `code/`.
  `CMAKE_EXPORT_COMPILE_COMMANDS` is already on.
- `mclib/CMakeLists.txt`, `mclib/{stuff,mlr,gosfx}/CMakeLists.txt`,
  `gui/`, `GameOS/gameos/`, `GameOS/src/`: library targets.
- `data_tools/CMakeLists.txt` (`makefst`, `pak`, `aseconv`, `makersp`),
  `Viewer/`, `text_tool/`, `res/` (`mc2res_64` shared library).
- `cmake/FindSDL2_mixer.cmake`, `cmake/FindSDL2_ttf.cmake`: pkg-config fallbacks.
- `.github/workflows/ci.yml`: `shaders` job + Linux matrix (x64 GCC, x64 Clang,
  arm64 GCC GLES, with ccache; arm64 uploads a packaged artifact) + Windows MSVC
  (3rdparty.zip cached by LFS object id). Runs on push to `pi-port`, on PRs, and
  manually.
- `.github/dependabot.yml`: keeps action versions current.
- `BUILD-LINUX.md`, `BUILD-WIN.md`, `BUILD-PI.md`.

## Gotchas

- `LINUX_BUILD` is defined on Windows too. Use `PLATFORM_WINDOWS` for MSVC-only code.
- MSVC rejects C++20 designated initialisers; use positional initialisers.
- Clang is stricter than GCC about function-pointer to `void*` casts; cast
  explicitly.
- GCC 12+ rejects some casts GCC 11 accepted; the arm64 job is GCC 12+.
- Do not let CI pull LFS objects on every run: the free LFS bandwidth is small.
- CI uses a self-contained Ninja build in `build/`; local Windows builds use
  `build64/` with the VS generator.

## Verify

Locally: build the configuration(s) you can. Push to a branch and let CI build the
rest; say in the reply which ones you built yourself.

## Recipes

**Add a game source file:** add it to `SOURCES` in the top-level
`CMakeLists.txt` (engine files: the relevant `mclib/` or `GameOS/` CMakeLists).

**Add a CMake option:** `option(MC2_X "..." OFF)` near `MC2_GLES`, a
`add_definitions(-DMC2_X)` when on, document it in `CLAUDE.md` and the affected
`BUILD-*.md`, and add a CI matrix entry if it changes what is compiled.

**Add a CI job:** follow the existing job shape (checkout with `lfs: false`,
apt install, ccache, configure, build); keep per-push jobs under about 10
minutes; slower work (smoke runs, sanitizers on missions, benchmarks) goes in a
scheduled workflow. Never run self-hosted (Pi) jobs on PRs from forks.

**Raise warnings on new code:** use `target_compile_options` on the new target
rather than global flags, so legacy code is not flooded.
