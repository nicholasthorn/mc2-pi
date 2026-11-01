# Agent workflow: worktrees and three tiers of checks

Cheap checks locally on every commit; the full matrix and the smoke missions
on GitHub CI, through a draft PR opened early. The goal is fast commits
without letting untested or tangled code pile up, and without several
sessions queueing for the same WSL CPUs to repeat what CI does anyway.

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
- WSL is one machine shared by every session: same `/tmp`, same process
  list, same CPUs. To wait for your own command, run it in the background
  and wait for that process; never poll with `pgrep -f smoke.sh` or similar,
  which also matches other sessions' runs.

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

## Tier 2: every batch, on CI

A batch is 3 to 5 commits or one finished logical step.

- Review the batch locally: `/simplify`, then `/code-review` on the commits
  since the last batch. Fix what it finds, or say why not.
- Push to `pi`. After the first batch, open a **draft PR** against `pi-port`:
  `ci.yml` runs only on PRs (and pushes to `pi-port`), so the draft PR is
  what puts the branch on CI. Later batches are pushes to the same PR.
- CI then does the batch checks: `ci.yml` (four configurations, unit tests
  also under ASan + UBSan, shader validation, cppcheck on changed files) and,
  when code changed, `smoke.yml` (five missions at 3000 frames on x64 desktop
  GL, arm64 GLES and x64 sanitized). Do not repeat those locally.
- Keep working while it runs; the app reports the result. A red run is fixed
  before the next batch.
- A push that changes only docs (`*.md`, `planning/`, `.claude/`) after a
  green run skips both workflows (`scripts/ci_docs_only.sh`), so a
  `PROGRESS.md` update or a `pi-port` merge that brings only docs costs
  nothing.

Run `scripts/check.sh --level batch` (both renderers plus a local smoke)
only when CI's round trip is the slower path:

- debugging a crash or sanitizer finding, where you need the logs now;
- a change to GLES-only code or shaders, when one local GLES build catches a
  mistake faster than waiting for the arm64 job.

## Tier 3: the PR

- Before marking it ready: `/code-review high` on the whole branch. Fix real
  findings; list the ones you defer in the PR description.
- CI must be green on the final push.
- The PR description says what was verified where (which renderer, which
  platform, run or only built).
- Update `planning/PROGRESS.md` (see `CLAUDE.md`).
- Pi hardware runs (`scripts/bench.sh`) only on request.
