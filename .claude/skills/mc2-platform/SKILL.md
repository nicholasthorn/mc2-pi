---
name: mc2-platform
description: "Work on the MC2 GameOS platform layer: SDL main loop, window, keyboard/mouse input (gos_GetKey, KeyboardFlush), sound (gameos_sound.cpp, WAV formats), file IO (gos_OpenFile), the user config directory, debugging/STOP, and the Win32 API emulation in GameOS/src. Use when changing GameOS/gameos/*.cpp outside the renderer, GameOS/src/platform_*.cpp, or GameOS/include, or when input, audio or startup misbehaves."
---

## Scope

Everything under `GameOS/` except the GL renderer (that is `/mc2-renderer`).
GameOS was Microsoft's game-OS abstraction; alariq reimplemented it on SDL2 from
the headers. Do not assume it matches any Microsoft documentation.

Line numbers drift: grep the symbol before citing a line.

## Key files

- `GameOS/gameos/gameosmain.cpp`: `main()`, command line, the frame loop
  (computes `frameRate` from frame time, calls `gos_RendererEndFrame()`).
- `GameOS/gameos/gameos.cpp`: environment, timing (`gos_GetHiResTime`), config
  storage (`gos_LoadDataFromRegistry` / save: files under `$XDG_CONFIG_HOME` or
  `$HOME/<g_user_base_config_dir>`, standing in for the Windows registry).
- `GameOS/gameos/gameos_input.cpp`, `gos_input.cpp/.h`: SDL keyboard/mouse state,
  `gos_GetKey()`, `KeyboardFlush`, `first_pressed_`.
- `GameOS/gameos/gameos_sound.cpp`: SDL_mixer playback, WAV decode, channels.
- `GameOS/gameos/gameos_fileio.cpp`: `gosFileStream`, `gos_OpenFile`,
  `gos_FileSize`, `gos_DoesFileExist`. Thin wrappers: path rules live in
  `mclib/file.cpp` (`/mc2-file-formats`).
- `GameOS/gameos/gameos_debugging.cpp`: `InternalFunctionStop` / `Pause`,
  `gos_FatalExit`, logging. `STOP(...)` (in `GameOS/include/gameos.hpp`)
  logs, shows a message box (not in automated runs) and exits 1;
  `MC2_STOP_EXITS=0` makes it log and continue. `Fatal()` / `ABL_Fatal()`
  always end through `gos_FatalExit`.
- `GameOS/gameos/gameos_res.cpp`: loads the `mc2res` resource library.
- `GameOS/src/platform_*.cpp` + `GameOS/include/platform_*.h`: Win32 API
  emulation (`_open`, `_stricmp`, `MessageBox`, `timeGetTime`, mbstring, ...).
  `platform_str.cpp` has `S_strlwr` / `S_stricmp`.
- `code/mechcmd2.cpp`: command-line parsing (`-mission`, `-nodialog`,
  `-braindead <team>`, `-turrets_off`, `-viewer`), `InitializeGameEngine()`.

## Gotchas

- **`gos_GetKey()` reads `first_pressed_` without consuming it.** A
  `while (gos_GetKey()) {}` drain loop can spin. Check the current body before
  writing loops around it.
- **`gos_KeyboardFlush()` calls `input::resetKeypress()`**, which clears the
  pressed/released edges and `first_pressed_` (back-ported from the remaster in
  `35e03e5`). Code that expects a key to stay "first pressed" across a flush is
  wrong.
- **8-bit WAV is unsigned PCM.** Decode as `AUDIO_U8`; `AUDIO_S8` makes 8-bit
  effects loud and buzzy.
- **No networking.** `code/mp*.cpp` / `multplyr.cpp` are UI stubs; MP calls may
  return NULL.
- **No files outside the config directory.** alariq's TODO lists this as
  unfinished; do not add new writes next to the executable or into data dirs.
- New Win32-ish calls in engine code: add an emulation in `GameOS/src` (with a
  `PLATFORM_WINDOWS` guard) rather than `#ifdef`s in `mclib/` or `code/`.

## Verify

- Builds on all four configurations (`.claude/rules/build-conventions.md`).
- Input/sound changes need a run; ask before launching. Name the platform you ran on.

## Recipes

**Add a command-line flag:** a game flag is parsed in `code/mechcmd2.cpp` next
to `-mission` and stored in a global. A flag that only the GameOS frame loop
uses is parsed from `argv` in `main()` in `GameOS/gameos/gameosmain.cpp`
instead, as `-autoexit` is (`ParseCommandLine` ignores flags it does not know).
Document development flags in `CLAUDE.md` "Common commands".

**Game tells GameOS something:** add a `gos_*` function to
`GameOS/include/gameos.hpp` and call it from `code/` (as
`gos_MarkGameplayStarted()` does). GameOS never calls into `code/` directly.

**Add a Win32 function the legacy code calls:** declare in the matching
`GameOS/include/platform_*.h`, implement in `GameOS/src/platform_*.cpp`, keep the
Windows build using the real API.
