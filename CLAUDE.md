# MC2 (pi-port)

MechCommander 2 open-source engine (alariq's Linux/Windows port), extended on the
`pi-port` branch to run on the Raspberry Pi 4/400 through an OpenGL ES 3.1
renderer. Desktop GL stays the default everywhere else.

## Repos, remotes and upstream policy

| Remote | Repo | Use |
|---|---|---|
| `pi` | `nicholasthorn/mc2-pi` | **Ours.** Push here. CI runs here. |
| `origin` | `alariq/mc2` | Upstream. Pull from it; never push to it. |
| `fork` | `nicholasthorn/mc2-opengl-remastered` | Read-only source of back-port fixes (`/mc2-backport`). |

- **Never open issues or PRs against `alariq/mc2`.** Its README says AI-written
  code is not welcome there. Anything offered upstream is the user's call and the
  user's own work.
- Keep diffs small and local so merges from `origin/master` stay painless: no
  wholesale reformatting, no renames of legacy files.
- Game data lives in a separate repo, `alariq/mc2srcdata` (git-lfs), checked out
  as a sibling directory. It is not a submodule.

## Project structure

```
mc2/
├── GameOS/        ← platform layer: SDL window/input/sound, GL renderer core, Win32 shims
│   ├── gameos/    ← gameos_*.cpp, gos_render.cpp, utils/ (graphics.h, gl_utils, shader_builder)
│   ├── include/   ← gos_* API headers
│   └── src/       ← platform_*.cpp Win32 API emulation for Linux
├── mclib/         ← engine: files/FST/packets, terrain, textures (txmmgr), ABL script VM, heap
│   ├── stuff/     ← Microsoft math/containers library (+ legacy *_test.cpp self-checks)
│   ├── mlr/       ← MLR renderer (meshes, clipping)
│   └── gosfx/     ← particle effects
├── code/          ← game: missions, mechs, AI, movers, logistics/UI screens, save/load
├── gui/           ← UI widget framework
├── shaders/       ← GLSL, valid as both GLSL 4.20 and GLSL ES 3.10
├── data_tools/    ← makefst, pak, aseconv, makersp (build the game data)
├── res/           ← mc2res resource library (libmc2res_64.so / mc2res.dll)
├── Viewer/ text_tool/  ← side tools
├── scripts/       ← validate_shaders.py
└── planning/      ← PROGRESS.md, QUALITY-ROADMAP.md, RELEASES.md, future-features/
```

`MechCmd2.vcproj` and `mc2res.vcproj` are vestigial. CMake is the build.

## Common commands

```bash
# Linux desktop GL (x64)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build

# Linux / Pi GLES
cmake -S . -B build-gles -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMC2_GLES=ON
cmake --build build-gles

# Windows MSVC (extract 3rdparty.zip first)
cmake -S . -B build64 -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=<abs path>/3rdparty -DCMAKE_LIBRARY_ARCHITECTURE=x64
cmake --build build64 --config RelWithDebInfo --target mc2 mc2_tests

# Meta Quest test APK (Android NDK; Git Bash on Windows or Linux; android/README.md)
scripts/android/build.sh                         # needs JAVA_HOME and the SDK's NDK r28c + CMake 3.31.6
scripts/android/run_bench.sh --data ../mc2game   # headset on adb: install, push, bench mono + stereo

# Unit tests (doctest, built by default; Windows adds -C RelWithDebInfo)
ctest --test-dir build --output-on-failure

# ASan + UBSan build (GCC/Clang); runtime options: .claude/rules/testing-conventions.md
cmake -S . -B build-san -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMC2_SANITIZE=address,undefined

# Shaders: compile-check every shader as GLSL ES 3.10 and GLSL 4.20 (needs glslang-tools)
python3 scripts/validate_shaders.py

# cppcheck against the baseline: changed files only (seconds) or the whole tree (slow)
python3 scripts/cppcheck_gate.py --build-dir build --files code/foo.cpp

# Game data from an alariq/mc2srcdata checkout (platform independent; copy to the Pi)
scripts/build_gamedata.sh --src-data ../mc2srcdata --build-dir build --out ../mc2game [--no-movies]

# Run (from the game directory: mc2, libmc2res_64.so, shaders/, data/, assets/, *.fst)
./mc2 -mission mc2_01
./mc2 -mission mc2_01 -autoexit 600   # quit cleanly after 600 gameplay frames
./mc2 -mission mc2_01 -autoexit 3000 -bench frames.csv   # deterministic run, per-frame timings + [BENCH] summary
LIBGL_ALWAYS_SOFTWARE=1 ./mc2     # WSL2 / no GPU: Mesa llvmpipe, closest PC match to the Pi

# Smoke: run missions headless (Xvfb + llvmpipe), fail on crash/hang/GL error/STOP
scripts/smoke.sh --game-dir ../mc2game --build-dir build [--missions "mc2_01 mc2_03"]

# Benchmark (on the Pi, own display): -bench runs, results.csv with p50/p95/p99 and the CPU split
scripts/bench.sh --game-dir ../mc2game --build-dir build --renderers gpu --label "fan on"
# CPU profile of gameplay frames (perf, frame-pointer build: -DMC2_FRAME_POINTERS=ON)
scripts/bench.sh --game-dir ../mc2game --build-dir build-fp --renderers gpu --runs 0 --perf
# Scripted battle with timed segments and screenshots (format: bench/README.md)
scripts/bench.sh --game-dir ../mc2game --build-dir build --renderers gpu --scenario bench/mc2_19.fit
```

Full setup: `BUILD-LINUX.md`, `BUILD-WIN.md`, `BUILD-PI.md`.

## Every change: both renderers, three platforms

> **Rendering or platform work: always cover both renderers.** Any change that
> touches GL calls, shaders or textures must work with `MC2_GLES=OFF` (desktop GL
> 4.2, or 4.x + `420pack`) **and** `MC2_GLES=ON` (GLES 3.1). Any change to
> platform code must build on Windows MSVC, Linux x64 (GCC and Clang) and Linux
> arm64. If you can only verify one, say which one was not verified.

CI (`.github/workflows/ci.yml`) builds all four configurations, runs the unit
tests on each, and validates the shaders on every push to `pi-port` and every PR.
`.github/workflows/smoke.yml` runs five missions nightly (and on demand) on
x64 desktop GL, arm64 GLES, and x64 under ASan + UBSan. CI also runs the unit
tests under ASan + UBSan and cppcheck on changed files on every push;
`.github/workflows/static.yml` runs cppcheck on the whole tree nightly. New
code builds with `-Wall -Wextra -Werror` (`.claude/rules/build-conventions.md`). See `.claude/rules/` for the
detailed conventions.

## Architecture in one screen

- **Layers:** `code/` (game) calls `mclib/` (engine) and `gui/`, which call
  `GameOS/` (platform). GameOS calls back into the game through the
  `Environment` callbacks (`InitializeGameEngine`, etc.) set in `code/mechcmd2.cpp`.
  (`code/logmain.cpp` sets them too but is not in the build.)
- **Frame:** game render functions only *enqueue* triangles. `mcTextureManager->renderLists()`
  (called from `code/gamecam.cpp`) sends them to GL, and `gos_RendererEndFrame()`
  in `gameos_graphics.cpp` ends the frame.
- **GL entry point:** `GameOS/gameos/utils/graphics.h` is the only header that
  includes GL. GLES/desktop differences are handled there and in `gl_utils.cpp`.
- **Shader headers:** `#version` and precision lines are injected at runtime by
  `buildShaderHeader()`; shader files never contain `#version`.
- **The whole engine builds with `-DLINUX_BUILD`, on Windows too.** `PLATFORM_WINDOWS`
  is the real "this is MSVC" flag. `PATH_SEPARATOR` is therefore `/` everywhere.
- **Networking does not exist.** `code/mp*.cpp` and `multplyr.cpp` are UI stubs.

## Testing

Unit tests are doctest in `tests/unit/` (one `mc2_tests` binary under CTest),
linking real engine files against a heap shim; CI runs them on all four
configurations. Smoke runs: `scripts/smoke.sh` (needs built game data), nightly
in CI. Still to come (sanitizers, coverage, benchmarks):
`planning/QUALITY-ROADMAP.md`. How to write and run tests:
`.claude/rules/testing-conventions.md`.

## Workflow: worktrees and three tiers of checks

Details in `.claude/rules/agent-workflow.md`. In short:

- **One worktree per session or feature**, off the latest `pi-port`, with its
  own build directories.
- **Every commit:** `scripts/check.sh --build-dir <dir>`: incremental build,
  unit tests, shader validation, cppcheck on changed files, diff hygiene.
- **Every 3 to 5 commits:** push to a draft PR (opened after the first
  batch), then run `/simplify` and `/code-review` **while** CI runs the full
  build matrix and the five-mission smoke; never wait for green to start
  review. Local `check.sh --level batch` only for crash debugging or
  GLES-only changes.
- **The PR:** `/code-review high` on the branch alongside CI on the last
  push; mark it ready when both are done.

Once coverage is measured, the threshold is a **ratchet**:

- **Never lower a threshold to make a red build green.** Add the tests.
- **Raise the threshold whenever coverage rises above it.**

## Progress tracking

Update `planning/PROGRESS.md` at the end of every task: a line in the dated log,
and the "Current state" section if what is built has changed. If a task spans
sessions, update it as you go. Specced-but-not-built work goes in
`planning/future-features/`, one file per feature.

## "Can we merge?"

When I ask **"can we merge"** (or any variant), that is a **status request, not an
instruction**. Answer **yes or no**, then the reasons.

- **Never merge on this question.** Wait for a separate, explicit instruction.
- **Ignore CI.** Do not check it or run it: I check CI myself. Base the answer on
  the code: is the work complete, is anything unverified (which renderer, which
  platform), are there known gaps, conflicts with the base branch, or open review
  comments.
- Say no when something real is outstanding, and name it. "Yes, but..." is a yes
  with a caveat: still lead with the word.

## After a merge: summarise what landed

When work **lands** (a PR merged, or a branch merged into `pi-port`), tell me in
chat what went in, unprompted. Pulling `origin/master` or `pi-port` into a feature
branch is bookkeeping, not landing: just report conflicts and how they were
resolved.

Write it for someone who has not followed the branch:

- **What changed and why**, led by the problem it fixes, not the files it touches.
- **Which areas**: renderer (GL / GLES), platform layer, engine, game, shaders,
  build/CI, data tools. Say plainly when one is untouched.
- **What was verified where**: which builds, which platforms, whether it was run.
- **Anything deferred**, including known gaps and unfixed review findings.
- **The PR number and the merge commit.**

Keep it proportional: a one-line fix gets a few sentences.

## Writing: no em dashes, no emoji

Never use the em dash character in anything you produce: chat, code comments,
commit messages, PR text, docs. Use a comma, a colon, brackets, a semicolon, or two
sentences (usually the right answer). Hyphens in compound words and en dashes in
numeric ranges are fine. No emoji in code, comments, commits or docs.

## Response format

At the end of every response that changes something, include:

**Request summary:** One or two sentences restating what was asked.

**Changes made:** Bullet list of every file changed and what was done.

**Tests to run:** The exact commands to verify the change, and which renderer /
platform each one covers.

Then the **Session status** (every branch and PR the session touched), exactly one of:

- **Further work required**: something is not done yet. Say what.
- **Ready to merge**: complete and pushed, not merged.
- **Merged with further work needed**: landed, but something is outstanding. Say what.
- **Merged and complete: we can close the session.**

## Detailed docs (read when relevant)

- `BUILD-LINUX.md`, `BUILD-WIN.md`, `BUILD-PI.md`: per-platform build and data setup
- `planning/QUALITY-ROADMAP.md`: tests, static analysis, CI, performance to-do list
- `planning/RELEASES.md`: release packages (Pi 4/400 first, plus Linux and Windows for everyone), support levels, how a release is made
- `planning/PROGRESS.md`: what is built, what is in progress, dated log
- `planning/future-features/`: specced, not built
- `devdoc.txt`, `docs` (a text file, not a folder): alariq's original dev notes

## Skills (`.claude/skills/`)

Invoke the matching skill before working in its area:

- `/mc2-build`: CMake options, dependencies, the CI matrix, warnings, adding a source file
- `/mc2-platform`: GameOS layer: SDL window, input, sound, file IO, Win32 shims
- `/mc2-renderer`: desktop GL and GLES backends, `graphics.h`, textures, shaders, frame flow
- `/mc2-file-formats`: File/FST/packet/FIT/CSV, path handling, data tools, `mc2srcdata`
- `/mc2-backport`: porting a fix from `mc2-opengl-remastered` into this repo

General-purpose skills (TDD, code review, debugging, planning) come from
`~/.claude/skills` and apply here as well.
