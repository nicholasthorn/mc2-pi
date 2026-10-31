# Agent workflow: worktrees and three tiers of checks

Cheap checks on every commit, heavier ones batched, the heaviest at the PR.
The goal is fast commits without letting untested or tangled code pile up.

## Setup: one worktree per session or feature

- Start each session or feature in its own worktree off the latest `pi-port`:
  `git fetch pi && git worktree add ../mc2-<name> -b <name> pi/pi-port`.
- Build in directories that belong to that worktree only, one per renderer
  (e.g. `~/build-<name>-gl`, `~/build-<name>-gles` on Linux / WSL), configured
  once with `cmake -S ../mc2-<name> -B ~/build-<name>-gles -G Ninja
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMC2_GLES=ON` (`=OFF` for desktop GL).
- On Windows + WSL: a worktree made by Windows git stores a `C:/` path that
  WSL's git cannot follow. `scripts/check.sh` falls back to Windows `git.exe`
  through interop; git 2.48+ on both sides with
  `git config worktree.useRelativePaths true` avoids the problem.
- Never edit, build or run in a worktree or build directory another session
  is using. The main checkout is the user's.
- When the branch has landed: `git worktree remove ../mc2-<name>`.

## Tier 1: every commit (minutes at most)

```bash
scripts/check.sh --build-dir <build dir of the renderer you touched>
```

Incremental build, unit tests, shader validation when `shaders/` changed,
cppcheck on the changed `.cpp` files, and checks on the changed lines: line
endings kept (legacy files are CRLF, new ones LF), no em dashes, trailing
whitespace (warning only). Pass a second `--build-dir` to cover both renderers.

Also before each commit:

- Read your own diff.
- Pure logic gets a unit test first (`testing-conventions.md`, "Where TDD
  applies").

No missions and no review agents at this tier.

## Tier 2: every batch

A batch is 3 to 5 commits or one finished logical step, and always before a
push.

```bash
scripts/check.sh --level batch --build-dir <gl build> --build-dir <gles build> \
    --game-dir <game data>
```

Both renderers, plus a short smoke run with each build (mc2_01 and mc2_03,
600 frames, headless llvmpipe).

Then review the batch: `/simplify`, then `/code-review` on the commits since
the last batch. Fix what it finds, or say why not, before continuing.

## Tier 3: the PR

- Before opening it: `/code-review high` on the whole branch. Fix real
  findings; list the ones you defer in the PR description.
- CI runs on the PR by itself: `ci.yml` (four configurations, unit tests also
  under ASan + UBSan, shader validation, cppcheck on changed files) and
  `smoke.yml` (five missions at 3000 frames on x64 desktop GL, arm64 GLES and
  x64 sanitized). Do not repeat those locally.
- The PR description says what was verified where (which renderer, which
  platform, run or only built).
- Update `planning/PROGRESS.md` (see `CLAUDE.md`).
- Pi hardware runs (`scripts/bench.sh`) only on request.
