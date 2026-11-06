# Agent workflow: worktrees and three tiers of checks

Cheap checks locally on every commit; the full matrix and the smoke missions
on GitHub CI, through a draft PR opened early. The goal is fast commits
without letting untested or tangled code pile up, and without several
sessions queueing for the same WSL CPUs to repeat what CI does anyway.

## Setup: two permanent worktrees per session

A new worktree means a new build directory, and so a build from scratch:
about 11 minutes on Windows, half of it the network libraries. Worktrees are
therefore kept and reused. Each session works in a pair of permanent
worktrees, and a new branch goes inside a free one, where the build then
recompiles only what differs from that worktree's last branch.

- **Pairs.** The worktrees are `../mc2-wt<N>a` and `../mc2-wt<N>b`. Each keeps
  its own build directories for good, one per renderer: `build64` inside it
  on Windows, `~/build-wt<N>a-gl` and `~/build-wt<N>a-gles` on Linux / WSL,
  configured once with `cmake -S ../mc2-wt<N>a -B ~/build-wt<N>a-gles -G Ninja
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMC2_GLES=ON` (`=OFF` for desktop GL).
- **Claim a pair when the session starts.** `git worktree list` shows which
  worktrees are locked. Take the lowest `N` whose two worktrees are both
  unlocked, and lock them with the session's name:
  `git worktree lock --reason "<session>" ../mc2-wt<N>a` (and `b`). If every
  pair is locked, add a new one with
  `git worktree add --detach ../mc2-wt<N>a pi/pi-port` (and `b`) and configure
  its build directories; only its first build is a full one.
- **A new branch goes in a free worktree of the pair.** Free means no
  uncommitted changes and its branch pushed or landed:
  `git -C ../mc2-wt<N>a fetch pi && git -C ../mc2-wt<N>a switch -c <name> pi/pi-port`.
  Two worktrees let a session start the next branch while the last one waits
  on CI or review.
- On Windows + WSL: a worktree made by Windows git stores a `C:/` path that
  WSL's git cannot follow. `scripts/check.sh` falls back to Windows `git.exe`
  through interop; git 2.48+ on both sides with
  `git config worktree.useRelativePaths true` avoids the problem.
- Never edit, build or run in a worktree or build directory locked by another
  session. The main checkout is the user's. A lock whose session has ended is
  cleared by the user, not by another session.
- **When the session ends:** leave both worktrees clean (work is committed and
  pushed, or dropped only with the user's say-so), detach them with
  `git switch --detach pi/pi-port` so they hold no branch, and
  `git worktree unlock` both. Do not remove them.
- When a branch has landed, delete it locally (`git branch -d <name>`) once no
  worktree has it checked out.
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

- Push to `pi`. After the first batch, open a **draft PR** against `pi-port`:
  `ci.yml` runs only on PRs (and pushes to `pi-port`), so the draft PR is
  what puts the branch on CI. Later batches are pushes to the same PR.
- CI then does the batch checks: `ci.yml` (four configurations, unit tests
  also under ASan + UBSan, shader validation, cppcheck on changed files) and,
  when code changed, `smoke.yml` (five missions at 3000 frames on x64 desktop
  GL, arm64 GLES and x64 sanitized; the multiplayer steps in full only when
  `scripts/ci_multiplayer.sh` says multiplayer code changed, otherwise a
  sanitized battle and no game modes). Do not repeat those locally.
- **Review while CI runs, not after it.** In the same turn as the push that
  starts CI (opening the draft PR, or a later batch's push), start
  `/simplify`, then `/code-review` on the commits since the last batch. Do not
  wait for CI to pass, or for the branch to be clean, before starting them:
  review and CI check different things, and waiting only adds the CI time to
  every batch. A red CI run and review findings are fixed together, in the
  next push.
- The app reports the CI result. A red run is fixed before the next batch.
- After a green run, a push that changes only docs (`*.md`, `planning/`,
  `.claude/`), or only merges `pi-port` in, skips both workflows
  (`scripts/ci_skip.sh`). Syncing with `pi-port`, including resolving the
  usual `PROGRESS.md` conflict, therefore costs nothing; a conflict fix that
  changes code runs everything. Code that came in from `pi-port` is tested
  in combination with yours only by `pi-port`'s own run after the merge.

Run `scripts/check.sh --level batch` (both renderers plus a local smoke)
only when CI's round trip is the slower path:

- debugging a crash or sanitizer finding, where you need the logs now;
- a change to GLES-only code or shaders, when one local GLES build catches a
  mistake faster than waiting for the arm64 job.

## Tier 3: the PR

- `/simplify`, then `/code-review high` on the whole branch, started in the
  same turn as the last code push, while CI runs on it. Do not wait for CI to
  pass first. Fix real findings; list the ones you defer in the PR
  description.
- Mark the PR ready once both are done: review findings handled and CI green
  on the final push.
- The PR description says what was verified where (which renderer, which
  platform, run or only built).
- Update `planning/PROGRESS.md` (see `CLAUDE.md`).
- Pi hardware runs (`scripts/bench.sh`) only on request.
