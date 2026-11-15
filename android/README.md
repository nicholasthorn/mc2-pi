# Android test builds: Meta Quest, phones and tablets

The Quest build is an Android app that runs MechCommander 2 in VR through OpenXR on a Quest 2,
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

The `game` step also downloads and compiles the network transport's libraries
(`cmake/mc2_gns.cmake`). To check the transport between this machine and the headset
over Wi-Fi, given a desktop build's `net_ping`:

```bash
scripts/android/net_ping.sh <path to a desktop net_ping>
```

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

## Phones and tablets

The same app without VR, for phones and tablets
(`planning/future-features/MOBILE.md`, phase 0):

```bash
MC2_ANDROID_FLAVOUR=phone scripts/android/build.sh
```

It differs from the Quest build only in its build: no OpenXR (`MC2_OPENXR=OFF`,
no loader in the APK), its own manifest (`android/phone/AndroidManifest.xml`:
no headset features, a normal launcher entry, either landscape way up) and its
own package, `com.nicholasthorn.mc2.phone`, so the two can be installed side
by side. The game builds in `android/build/game-phone`; the result is
`android/mc2-phone-test.apk`. Every script here takes the same variable, for
example `MC2_ANDROID_FLAVOUR=phone scripts/android/run_bench.sh --data ...`.
`package.sh` packages the Quest flavour only, for now.

The game data and `shaders/` go in
`/sdcard/Android/data/com.nicholasthorn.mc2.phone/files`, as on the headset:

```bash
D=/sdcard/Android/data/com.nicholasthorn.mc2.phone/files
adb install -r android/mc2-phone-test.apk
adb shell mkdir -p $D
adb push <game data folder>/. $D/
adb push shaders $D/
adb shell am start -n com.nicholasthorn.mc2.phone/com.nicholasthorn.mc2.MC2Activity
```

What to expect so far:

- **Touch** (`GameOS/gameos/gos_touch.cpp`, rules in `utils/touch_gestures.h`
  and `utils/touch_bar.h`): one finger is the mouse with its left button (tap
  to click, hold or move to drag); two fingers move the camera in a mission
  (pinch to zoom, move together to scroll). Missions use the one-button
  controls while touching: tap a unit to select, ground to move, an enemy to
  attack. A command bar on the right edge in a mission adds Waypoints, Add
  to selection, Force fire, Select visible, Next group and New group. In a
  mission, a finger held still for half a second is the right button (follow
  a unit with the camera, or drag to turn it). A text field brings up the
  on-screen keyboard. `MC2_TOUCH=0` in `mc2args.txt` goes back
  to SDL's own taps as clicks. A Bluetooth or USB mouse and keyboard play as
  on a PC.
- **The picture is stretched** to the screen: the game draws at its own
  resolution (800x600 by default) and fills the whole display with it, so a
  20:9 phone widens everything.
- **Switching away** from the app (Home, another app, a call) pauses a mission
  with its pause menu up, as the pause key does, and stops the sound; coming
  back carries on. A campaign mission is also saved then (`resume.ims` in the
  save folder, removed on coming back): if Android closes the app while it is
  in the background, the next start goes straight back into the mission,
  paused, instead of the main menu. A single mission or a multiplayer game is
  not saved, as the pause menu cannot save them either. If the GL context did not
  survive, the game quits (an `[APP] FATAL` line in `mc2.log`), since it
  cannot reload its textures.

### A full APK, with the game data inside

For a device without `adb`: name a game data folder and the APK carries it,
with this checkout's `shaders/` (about 470 MB without movies):

```bash
MC2_ANDROID_FLAVOUR=phone MC2_ANDROID_DATA=<game data folder> scripts/android/build.sh apk
```

Build the data with `scripts/build_gamedata.sh ... --no-movies`. The first
start copies it to the app's files folder (`GameOS/gameos/android_data.cpp`),
with toasts counting it up, and starts from the minimum settings
(`minprefs.cfg`), so there is no first-start exit. An APK with different data
copies it again over the old; settings, saves, `mc2args.txt` and the logs are
never in the APK and are kept. The copy doubles the space the data takes while
the APK is installed.

## Status

Quest: played on a Quest 2, with the bench results in
`planning/future-features/VR-TABLETOP.md`. Not yet tried on a Quest 3 or Pro.

Phones and tablets: builds, with or without the data inside; not yet run on a device.
