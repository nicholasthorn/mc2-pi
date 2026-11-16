# Phones and tablets: Android first, iOS later, no web build

Status: proposed, nothing built. Written 2026-11-14 from a read of `pi-port`
and the multiplayer branch (`mp-phase4-screens`). Nothing has been run on a
phone or tablet.

## Problem

The game runs on Windows, Linux, the Pi and the Quest. Phones and tablets are
the devices most people have with them, and the reason to want them is
**multiplayer across devices**: a game started on a PC that someone joins from
a tablet on the sofa, or two phones on a train.

The question was whether to get there with a native Android port or a web
build (which would also reach iOS). This file answers it and specs the
native route.

## Decision: native, not a web build

Devices play together when they speak the same network protocol. They do not
need to run the same binary. The multiplayer work (`NETWORKING.md`) already
gives every native build one protocol: fixed-width messages (`netwire`) over
GameNetworkingSockets (GNS), which runs on Windows, Linux and the Pi, and on
the Quest (the Android spike in `NETWORKING.md`). A native Android build joins
those games with no new networking code.

A web build cannot do that:

| | Native Android (and later iOS) | Web build (Emscripten) |
|---|---|---|
| Networking | Same GNS transport as every other build | Browsers have no UDP sockets, so no GNS. Needs a second transport (WebRTC data channels or WebSockets), a signalling server, and a relay or dedicated server, since a browser tab cannot host |
| Renderer | The GLES 3.1 renderer as it is | WebGL2 is about GLES 3.0 (see "Renderer below 3.1") |
| Main loop | As it is | The legacy code blocks in loops; needs Asyncify (slower, larger) or a restructure |
| Game data (about 500 MB) | Installed once with the app | Downloaded into each browser on each device. iOS Safari kills tabs at around 1 to 2 GB of memory |
| Distribution | APK on the Releases page, as the Quest | The data on a public web server: the widest distribution of Microsoft's assets (see `EASY-INSTALL.md`, "What may be distributed") |
| iOS | A separate native build, later | Included, but iPhone Safari has no real fullscreen and no pointer lock |
| Touch controls | Needed | Needed, the same work |

The web build's one real gain is iOS without the App Store, and it pays for
that with the weakest multiplayer of any option. **Rejected.** The touch work
below is needed by every route, so nothing here is lost if the web question
comes back.

## What already exists

The Quest build is an Android app, so most of the platform work is done:

- `android/` and `scripts/android/` build an arm64 APK with the NDK, SDL2 and
  SDL2_mixer: the game as `libmain.so`, started by `MC2Activity`.
- `android_prepare` in `GameOS/gameos/gameosmain.cpp` sets the working folder,
  `HOME`, the command line (`mc2args.txt`) and the log.
- Audio prefers AAudio, then OpenSL (`gameos_sound.cpp`).
- `MC2_XR=0` already runs the game flat, without OpenXR, on the headset.
- The manifest already has `INTERNET`, `CHANGE_WIFI_MULTICAST_STATE` and
  `ACCESS_WIFI_STATE` for LAN play.
- GNS built with BoringSSL runs on Android (`net_ping` between a PC and a
  Quest, `NETWORKING.md`).
- The GLES renderer is what the Pi and Quest run every day.

What does not exist: **any touch input** (no `SDL_FINGER*` handling anywhere),
**any app lifecycle handling** (no `SDL_APP_WILLENTERBACKGROUND` and friends),
and a layout for small screens.

## Scope

In:

- A flat Android build for phones and tablets, arm64, Android 10 (API 29) or
  later, the same floor as the Quest build.
- Touch controls for the menus, logistics and missions.
- Interface sizing for phone and tablet screens.
- Background and resume without losing the game.
- Installing the game data on the device.
- Joining and hosting LAN and internet games as specced in `NETWORKING.md`.
- Gamepads and Bluetooth mice and keyboards working as they would on a PC
  (SDL gives these nearly free; mostly testing).

Out:

- A web build (above).
- The Play Store and App Store (see "Distribution").
- 32-bit Android (armeabi-v7a), x86 Android.
- Any phone-specific game balance or simplified missions.
- iOS, until the Android build is played. It gets its own section below and
  its own file when it starts.

## Approach

### 1. A flat build variant

Make the APK build in two flavours from one tree:

- **Quest:** as today. OpenXR, the `vr.headtracking` feature required, the
  `com.oculus.intent.category.VR` launch category, `supportedDevices`.
- **Flat (phones and tablets):** `MC2_OPENXR=OFF`, none of the Quest manifest
  entries, `touchscreen` required, a normal launcher entry, landscape.

`scripts/android/build.sh` gains a flavour argument and writes two manifests
from one template, or keeps two small manifests side by side (simpler; decide
when doing it). The application ID stays the same or gains a suffix
(`com.nicholasthorn.mc2.flat`) so both can be installed on one device for
testing. Size: S.

### 2. Touch input

The real work. GameOS already has the right shape for it: the VR build reads
its controllers into GameOS's virtual mouse and keys every frame
(`gos_xr::pollInput()`), and the game never knows. Touch does the same, in a
new `GameOS/gameos/gos_touch.cpp`, with the gesture logic in a GL-free,
SDL-free file (`GameOS/gameos/utils/touch_gestures.cpp`) so it can be unit
tested with recorded finger tracks.

SDL must stop turning touches into mouse events itself
(`SDL_HINT_TOUCH_MOUSE_EVENTS=0`), or every tap arrives twice.

**Mission controls.** The game has two mouse profiles (`useLeftRightMouseProfile`
in `missiongui.cpp`). The classic one selects and commands with the left
button, which suits touch: one finger is the left button. Proposed mapping, to
be tried on a device before it is fixed:

| Gesture | Becomes | Notes |
|---|---|---|
| Tap | Left click | Select a unit, or order the selection (classic profile) |
| Double tap | Left double click | Select all of that type, as on a PC |
| One finger drag from a unit or empty ground after a short hold | Left drag | Box select. The hold separates it from panning |
| One finger drag straight away | Camera pan | As edge scrolling or the arrow keys |
| Two finger drag | Camera pan | Alternative that never conflicts with box select |
| Pinch | Zoom | As the mouse wheel |
| Two finger twist | Camera rotate | If the camera allows it in that mode |
| Long press on a target | Right click | Context order (attack, capture) in the left/right profile; info in the classic one |
| Finger down, not moving | Hover | The cursor shape and tooltip show where the finger is, so the player sees what a release would do; lifting off a control without moving still clicks it |

**Modifier keys and hotkeys.** The mission interface uses keys for force
fire, moving without attacking, selection groups (`Ctrl+1`..`9`, `1`..`9`),
jump, guard, and more (the key table near the top of `missiongui.cpp`; list
the ones a player needs and drop the debug `Ctrl+Alt` ones). Touch needs:

- A small **modifier bar** on the screen edge: toggles that hold a modifier
  until the next order (force fire, move only, add to selection).
- **Selection group buttons**: tap to select, long press to assign. The
  keyless group commands added for the Quest (next group, previous group, new
  group from the selection: grave, Shift + grave, Ctrl + grave) and its tap,
  double tap and long press timing (`vr_controls::pressEvents`) are the model.
- The existing control panel buttons already cover most orders, and must be
  large enough to hit (next section).

The bar is drawn by the game's own GUI (`gui/`), not an Android overlay, so it
looks the same on every device and works in screenshots and smoke runs.

**Menus and logistics.** Taps as clicks are mostly enough. Drag and drop in
the mech bay and pilot screens is a press, a move and a release, which a
finger already gives. Text fields (pilot names, chat, direct connect address)
need the on-screen keyboard: `SDL_StartTextInput()` when a field takes focus,
`SDL_StopTextInput()` after.

**Mice and keyboards still work.** A touch never disables the mouse; the
cursor follows whichever was used last. Size: L.

### 3. Interface size

The interface is laid out for 800x600 (`prefs.cpp`), and scaled to the
screen. On a 10 inch tablet that is a little larger than a 15 inch laptop at
800x600, which is fine. On a 6 inch phone in landscape the same layout is
about 60% of that size, and the control panel buttons fall below a fingertip
(about 7 to 9 mm).

- **Tablets:** probably playable at the stock layout. Confirm on a device.
- **Phones:** need larger hit areas at least: a touch on a button counts if it
  lands within a margin around it, which costs no art. If that is not
  enough, a larger control panel layout, which is real art and layout work.
- `vr_eye::fitInterfaceResolution` and the VR interface work already deal
  with the interface at a size other than 800x600; reuse what applies.
- Cut-outs and rounded corners: keep the interface inside the safe area SDL
  reports (`SDL_GetWindowSafeArea` arrived in SDL 3; on SDL2 read the insets
  through JNI, or keep a fixed margin at first).

Size: M for tablets plus larger hit areas; L if phones need a new layout.

### 4. App lifecycle

Android stops an app that goes into the background and may kill it later.

- On `SDL_APP_WILLENTERBACKGROUND`: pause the game (as the pause key does),
  stop sound, and in single-player save to a resume slot.
- On `SDL_APP_DIDENTERFOREGROUND`: carry on. SDL2 blocks the game's thread
  while the app is in the background (`SDL_HINT_ANDROID_BLOCK_ON_PAUSE`, on by
  default) and recreates the window surface on return. Whether the GL context
  and its textures survive depends on the device and driver; if they are lost,
  the textures need reloading, which the engine has never done. Test on
  several GPUs (Adreno, Mali, PowerVR) before deciding whether that work is
  needed.
- If Android kills the app, the next start offers the resume slot.
- **Multiplayer:** a backgrounded player is a dropped player.
  `NETWORKING.md` phase 6 (resilience) covers reconnecting; until then, say so
  in the lobby.

Size: M.

### 5. Game data on the device

The Quest gets its data pushed over `adb`. Phones need a way without a PC:

- **One APK with the data inside** (`EASY-INSTALL.md` already plans
  `mc2-<ver>-quest-full.apk` at about 550 MB): copied to the app's files
  folder on first start, with a progress screen. Simplest for players;
  doubles the storage while the APK is kept. Recommended first.
- **Engine APK plus a data download** from the Releases page on first start:
  smaller APK, but it is network code that fetches and checks a large file.
  Later, if the APK size is a problem.

Movies add about 180 MB; leave them out of the phone package at first.

### 6. Networking

Nothing phone-specific beyond what `NETWORKING.md` already lists for Android:
the manifest permissions (present), a Wi-Fi `MulticastLock` while the LAN
browser is open, and the direct connect and room code flows. A phone on mobile
data is behind carrier NAT, so it can only join internet games through room
codes and, where hole punching fails, the TURN relay (`NETWORKING.md`,
"Remote play", items 2 and 3). **Easy cross-device play needs that relay
whichever route is taken**, so it is not a cost of the native choice.

Hosting from a phone is allowed but not recommended: the host runs every
player's simulation and all the AI, and a Quest 2 already spends about 13 ms
per frame on game logic on the `mc2_19` bench.

### 7. Distribution

Sideloaded APK from the Releases page, as the Quest. Not the Play Store at
first:

- `EULA.txt` allows non-commercial distribution of the art assets. A free
  Play Store listing is non-commercial, but it is a far more visible place to
  put Microsoft's assets and the MechCommander name (the EULA grants no
  trademark licence), and a store takedown is out of our hands.
- Play requires a 200 MB base APK plus asset packs, and an AAB build.

Revisit only if sideloading proves too much for the people who want to play.

## iOS, later

Feasible, and blocked by distribution rather than code:

- SDL2 supports iOS. GNS has iOS builds.
- The renderer uses nothing that only GLES 3.1 has except
  `layout(binding=)` (see below), so it could run on Apple's deprecated
  GLES 3.0 or on ANGLE over Metal. `MACOS.md` weighs the same choice for the
  Mac; do that first and share the answer.
- The touch layer from step 2 carries over unchanged.
- Against it: a Mac and Xcode to build, a paid developer account, App Store
  review of Microsoft's IP (unlikely to pass), TestFlight builds that expire
  after 90 days, and alternative stores only in the EU.

Write `IOS.md` when Android is played and someone wants an iPhone build.

## Renderer below 3.1

Not needed for Android (every arm64 device from the API 29 floor up with a
current GPU has GLES 3.1 or later; check the oldest target device). Recorded
for iOS, ANGLE and any later web question: a search of `GameOS`, `mclib` and
`code` found no compute shaders, storage buffers, image load/store or memory
barriers. The ES 3.1 features in use are `layout(binding=)` on uniform blocks
and samplers (three lines in `shaders/`). On ES 3.0 those become
`glUniformBlockBinding` and `glUniform1i` calls in C++, the same fix
`MACOS.md` proposes for GL 4.1.

## Affected areas

- **Platform:** `gos_touch.cpp` and `utils/touch_gestures.cpp` (new), SDL
  touch hints and lifecycle events in `gameosmain.cpp`, text input, the safe
  area.
- **Game:** the modifier bar and selection group buttons (`missiongui.cpp`,
  `controlgui.cpp`), larger hit areas, a resume save on backgrounding.
- **GUI:** hit-area margins in `gui/`.
- **Renderer:** none, unless lost GL contexts must be survived (texture
  reload). Both renderers keep building; the phone build is GLES.
- **Build/CI:** the flat flavour in `scripts/android/build.sh`; a CI job that
  builds both APKs (there is none for Android yet; see open questions).
- **Data:** none. The phone runs the same data as every other build.
- **Single-player on other platforms: must not change.** Touch code is only
  active when a touch device sends events; desktop smoke hashes pin that.

## Phases

| Phase | What | Exit criterion | Size |
|---|---|---|---|
| 0. Flat build | Flat flavour, launcher entry, mouse and gamepad through SDL | A Bluetooth mouse plays mc2_01 on an Android tablet | S |
| 1. Touch basics | Gesture recogniser with unit tests; taps, drags, pinch, pan; menus and logistics by touch; on-screen keyboard | Campaign start to mc2_01's end by touch only, on a tablet | M |
| 2. Mission controls | Modifier bar, selection groups, long press, hover feedback | mc2_03 and mc2_19 played by touch on a tablet without a mouse | M |
| 3. Lifecycle and data | Background and resume, resume save, data inside the APK with a first-start copy | Switch apps mid-mission and come back; install from the Releases page on a device that has never seen `adb` | M |
| 4. Phones | Hit areas, safe area, then a larger layout if needed | A mission played on a 6 inch phone by someone other than the author | M to L |
| 5. Cross-device play | Nothing new if `NETWORKING.md` phase 4 is done; test it | A tablet joins a PC-hosted LAN game; a phone on mobile data joins by room code (needs `NETWORKING.md` phase 5) | S |

Phases 0 to 3 can start now; they do not wait for multiplayer. Phase 5 waits
for it.

**Go/no-go:** after phase 2, play a whole mission by touch on a tablet. If it
is not fun there, phones will not be, and the project stops at tablets with a
mouse and gamepad.

**Size:** S is days, M is one to three weeks, L is several weeks, part-time
with agents helping. Altogether, about two months, most of it in phases 1, 2
and 4.

## Verification

- **Unit tests:** the gesture recogniser, fed recorded finger tracks (tap,
  hold then drag, quick drag, pinch, two fingers landing a few frames apart),
  checked against the mouse and key events it produces. TDD.
- **Smoke:** a touch replay option, like `-uismoke`, that feeds a recorded
  track through the same path, so a desktop CI run covers the mapping into
  the game without a device.
- **CI:** build both APK flavours (at least `libmain.so` for arm64 Android);
  no device in CI.
- **Hardware:** on a tablet and a phone, each phase's exit criterion, plus
  lifecycle on two GPU vendors. Add these to `planning/HARDWARE-TESTS.md`
  when a phase lands.

## Open questions

- Which devices to test on? At least one tablet and one phone, ideally one
  Adreno (Qualcomm) and one Mali (Samsung Exynos, MediaTek, Tensor).
- Does the classic one-button mouse profile hold up by touch, or does the
  left/right profile with long press read better? Decide in phase 2 on a
  device.
- Is the stock control panel big enough on a phone with larger hit areas, or
  does it need a new layout?
- Is an Android CI build job worth its time (NDK download and cache), or is a
  local `scripts/android/build.sh` per release enough? The Quest has managed
  without one so far.
- Should the phone build ship the movies?
- Portrait mode: out of scope now; is it ever wanted, for menus at least?
