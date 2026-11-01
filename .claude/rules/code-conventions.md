# Code conventions

## Legacy code vs new code

Most of the tree is 2001 Microsoft code plus alariq's port. Treat it as legacy:

- **Match the surrounding style** (tabs, Hungarian-ish names, `long`/`DWORD`,
  comment density). Do not reformat, rename or reorder legacy code you are not
  changing: it wrecks `git blame` and makes merges from `alariq/mc2` conflict.
- **Fix narrowly.** A bug fix touches the lines that are wrong, plus a short
  comment when the reason is not obvious (why a null check, why `memcpy`).
- **`STOP(...)` ends the game** (log, message box, exit 1), but
  `MC2_STOP_EXITS=0` makes it log and carry on, and `gosASSERT` compiles to
  nothing unless `_ARMOR` is defined. So a null check that only calls `STOP`
  or `gosASSERT` still needs an explicit return or bail-out. `Fatal()` and
  `ABL_Fatal()` are `[[noreturn]]`: code after them never runs. A problem the
  game can survive (bad mission data, a missing optional file) is a `PAUSE`
  (log and carry on) plus a bail-out, not a `STOP`.
- **Networking is stubbed.** Calls into the MP layer can return NULL or dummy
  data (`getPlayerInfo()` etc.); guard them.
- **Some bugs are kept as shipped.** Original-game behaviour that changes how
  the game plays (forest pathing, fog bytes, ...) is kept on purpose
  and listed under "Kept as shipped" in `planning/PROGRESS.md`, with a comment
  at the code. Do not "fix" one of those without a decision from the user. A
  new case like it is reported for a decision, not fixed.

New code (new files, or substantial new functions):

- C++17-compatible, `<cstdint>` fixed-width types, `nullptr`, RAII where it does
  not fight the engine's own heap.
- Put pure logic in a file with no GL / SDL dependency so it can be unit-tested.
- Platform differences go behind `GameOS/`, not as `#ifdef`s scattered through
  `code/`.

## Paths and files

- Use `PATH_SEPARATOR` / `PATH_SEPARATOR_AS_CHAR` from `mclib/paths.h`; it is `/`
  on every platform because `LINUX_BUILD` is always defined.
- Paths read from data files use `\`; `File::open` converts them under
  `LINUX_BUILD`. Do not add a second conversion elsewhere.
- Never write files outside the user's config/save directory
  (`$XDG_CONFIG_HOME` / `$HOME` handling in `GameOS/gameos/gameos.cpp`).

## Back-ports

Fixes copied from `mc2-opengl-remastered` follow `/mc2-backport`: cite the
remaster commit in the commit message, drop remaster-only diagnostics and
features, and keep the fix self-contained.

## Review checklist (use before saying work is done)

- Builds in all four configurations, or the unbuilt ones are named.
- Both renderers considered for any GL / shader / texture change.
- No new `#version`, no int literals in float shader math.
- No new pointer-in-`long`, no type punning, no x86 assumptions.
- No reformatting of untouched legacy code.
- `planning/PROGRESS.md` updated.
