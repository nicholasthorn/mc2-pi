# Meta Quest test build

An Android app that runs MechCommander 2 in VR through OpenXR on a Quest 2,
with the GLES 3.1 renderer. It began as a measurement tool: run the `-bench`
scenario on the headset's own CPU and GPU driver, mono and `-stereo`, and pull
the numbers back (the VR plan's Phase 0 go/no-go,
`planning/future-features/VR-TABLETOP.md`). It is also the first part of
Phase 4 (Quest standalone).

## Build (Windows Git Bash or Linux)

Needs the Android SDK with NDK `28.2.13676358`, CMake `3.31.6`, build-tools
`36.0.0` and platform `android-36`, plus a JDK 17 or newer (Android Studio's
bundled `jbr` works).

```bash
export JAVA_HOME="/c/Program Files/Android/Android Studio1/jbr"   # Windows example
scripts/android/fetch_deps.sh     # SDL2 2.30.11 and SDL2_mixer 2.8.0 sources, checked by SHA-256
scripts/android/build.sh          # deps, game (libmain.so), APK
```

The result is `android/mc2-quest-test.apk`, signed with a debug key made on
the first build (`android/debug.keystore`, not committed). Steps can be run
alone: `scripts/android/build.sh deps|game|apk`.

How it differs from the desktop build:

- The game is `libmain.so`; SDL's Java activity (`MC2Activity`) loads it with
  `libSDL2.so` and `libSDL2_mixer.so` and calls `SDL_main`.
- At start (`android_prepare` in `GameOS/gameos/gameosmain.cpp`) the game
  changes to the app's files folder, sets `HOME` there, reads its command line
  from `mc2args.txt` and writes stdout and stderr to `mc2.log`, keeping the
  run before as `mc2.previous.log`. Outside a bench, a crash adds `CRASH:` lines.
- `libmc2res_64.so` is loaded by name from the APK, not from the data folder.

## Release builds

`.github/workflows/release.yml` builds the same APK on every `v*` tag and in
its dry runs, and `scripts/android/package.sh` packages it as
`mc2-<ver>-quest-arm64.apk` with `-files.tar.gz` (the shaders, which the APK
does not hold, and the licences) and `-symbols.tar.gz`. Players follow
`dist/INSTALL.md`.

Updating an installed app needs the same signing key, and uninstalling deletes
the game data in its folder. So releases sign with the keystore in the
`QUEST_KEYSTORE` repository secret (base64; store and key password `android`,
alias `androiddebugkey`, as `build.sh` makes). Without the secret each build
gets a new key and needs a clean install.

## On the headset

The headset needs developer mode, and `adb` (in the SDK's `platform-tools`)
must see it: `adb devices`. Then, from this checkout:

```bash
scripts/android/run_bench.sh --data <game data folder>
```

It installs the APK, pushes the game data (about 700 MB the first time, only
changes after that) and this checkout's `shaders/`, runs the mc2_19 bench
scenario mono and then with `-stereo 40`, and pulls `mc2.log` and the frame
CSV of each run into `./quest-bench`, printing the `[BENCH]` lines. The game
data comes from `scripts/build_gamedata.sh ... --no-movies`. The headset
sleeps when nobody wears it, which pauses the run: wear it, or keep it awake.

By hand: the app's folder is `/sdcard/Android/data/com.nicholasthorn.mc2/files`.
`mc2args.txt` there holds the command line (`#` starts a comment); leading
`NAME=value` tokens set environment variables, as on a shell line:

```
MC2_VSYNC=0 MC2_STOP_EXITS=1
-mission mc2_19 -autoexit 3600 -bench mono.csv -benchscenario bench_scenario.fit
```

Start it with `adb shell am start -n com.nicholasthorn.mc2/.MC2Activity`; it
quits by itself after the run. `mc2.log` ends with the `[BENCH]` lines: per
segment and for the whole run, including the `<stage>_ms` stage times
(`bench/README.md`). If the app dies without a log, `adb logcat -d` has the
crash.

## Status

Played on a Quest 2, with the bench results in
`planning/future-features/VR-TABLETOP.md`. Not yet tried on a Quest 3 or Pro.
