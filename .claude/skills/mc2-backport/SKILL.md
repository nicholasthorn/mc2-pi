---
name: mc2-backport
description: "Port a bug fix from mc2-opengl-remastered (the 'fork' remote, sibling checkout ../mc2-opengl-remastered) into this repo. Use when asked to back-port, bring over or copy a fix from the remaster, or when hunting the remaster's history for crash fixes that apply to shared engine/game code."
---

## Why this needs care

The remaster forked from `alariq/mc2` at `f166d58` (2025-02-08) and has about
5,000 commits since: a new renderer (RenderCore/RenderWorld, terrain, PBR), mods,
an editor, ImGui, diagnostics. Shared game and engine files (`code/`, `mclib/`,
`GameOS/gameos/gameos_input.cpp`, `gameos_sound.cpp`) are still close, so many
crash fixes apply. Its renderer changes almost never do.

## Steps

1. **Find candidates.** In the remaster:
   `git log --oneline -- <file>` for a file you care about, or
   `git log --oneline --grep='fix' -- code mclib` for a sweep. Prefer commits whose
   message names a crash, UB, overread, null or use-after-free.
2. **Check it applies.** Compare the touched function in both repos
   (`diff <(git -C ../mc2-opengl-remastered show <sha>:<path>) <path>` or read
   both). Skip it if the fixed code only exists in the remaster.
3. **Port the fix, not the commit.** Drop remaster-only pieces: diagnostics
   (`MC2_*` env vars, telemetry, Tracy zones), mod handling, renderer hooks,
   new subsystems. Keep the change minimal and in this file's style.
4. **Both builds.** The fix must not assume remaster build settings (it builds
   with MSVC primarily and has extra defines).
5. **Commit** with `fix: back-port <what> from the remaster` and list each fix
   with the remaster commit(s) in brackets, as in `35e03e5` and `495f7ba`. One
   commit can carry several small related fixes.
6. **Log it** in `planning/PROGRESS.md`.

## Do not

- Do not cherry-pick remaster commits wholesale; they drag in unrelated changes.
- Do not port renderer changes from the remaster into `gameos_graphics.cpp` without
  checking GLES compatibility: the remaster targets desktop GL 4.3+ only.
- Do not run the remaster's preflight scripts, smoke runner or deploy scripts
  here; they assume its own paths and tooling.
