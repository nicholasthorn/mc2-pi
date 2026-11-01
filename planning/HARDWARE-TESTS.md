# Hardware and in-game tests to run

Checks CI and the headless smoke cannot do: they need a real display, a
person looking or listening, the Pi, Windows, or a headset. Agents do not
launch the game in a window or run anything on the Pi without asking
(`.claude/rules/testing-conventions.md`), so these collect here until someone
runs them.

When one is done, tick it, add the date, the machine and the result, and
put a line in the `PROGRESS.md` log. Add new ones when a PR leaves something
only a person can check; say which renderer each needs.

Renderers: **GL** is a desktop GL build (`MC2_GLES=OFF`), **GLES** a GLES build
(`MC2_GLES=ON`, on the Pi or under `LIBGL_ALWAYS_SOFTWARE=1` on a PC).

## In the game (any PC with a display)

- [ ] **Mech colours stay put on an LOD change** (#43). Zoom in and out on
      your mechs and some vehicles: the paint must not switch to swapped
      colours. GL and GLES. `./mc2 -mission mc2_01`
- [ ] **Tutorial cursor** (#43). In the tutorial's "move the mouse to this
      button" steps the cursor lands on the button, not in the window's
      corner. Also on a HiDPI or scaled display if one is available.
- [ ] **Graphics options resolution list** (#53). Options, Graphics: each
      resolution appears once; choosing one and pressing OK saves it, and
      Reset selects the saved one again.
- [ ] **Pilot auto-assignment** (#64). In logistics, let the game assign
      pilots: a higher-ranked free pilot is picked before a lower-ranked one,
      even one with better gunnery.
- [ ] **PR #9 fixes**: infantry, the font texture, `MemoryStream`; air-strike
      bombers render light grey, not compared with the intended art. (10-31)
- [ ] **Terrain blend caches** resized to the current size: check on screen.
      `.txm` cache names do not include their size and every mission shares
      them. (10-31)
- [ ] **Loading a saved game** through the size-checked pathing reader
      (`saveload.cpp` also exits on `badLoad`). (10-30)
- [ ] **Faster save/load dialog** (#16): the campaign dialog lists saves
      without scanning them all first; a rejected save is not listed.
- [ ] **Screens the UI smoke does not walk yet**: mission selection,
      salvage, the tutorial, changing a weapon in the mech lab
      (`future-features/UI-SMOKE.md`, slices 3-5).
- [ ] **Stereo panning by ear** (#4): a unit off to the left is heard on the
      left. Nobody has recorded checking it.
- [ ] **In-mission video box position** at resolutions other than 800x600:
      `controlgui.cpp` adds `hiResOffsetX` but not `hiResOffsetY` (remaster
      `51ddcf25`). Check before changing it.
- [ ] **Briefing minimap colours**: the remaster (`3c8dd69e`) says water and
      concrete cells upload with alpha 0, even on stock data. Check by eye
      before porting.
- [ ] **Fog** on actors vs terrain: actors treat "no fog" as 0, terrain as
      0xff (original behaviour, kept). Look for a mismatch at the fog edge,
      ideally on the Pi.
- [ ] **Helicopter dust cloud** (`mech3d.cpp`) draws with the plain matrix,
      unlike the water wake next to it. Check whether it looks wrong.

## STOP message box (#46)

- [ ] **Windows:** a `STOP` (e.g. `-mission nosuchmission`) shows the box
      over the game window and OK ends the game.
- [ ] **Linux under a desktop window manager** (checked so far only under
      Xvfb with no window manager).
- [ ] **Pi console after a `STOP`** on the Pi's own display (KMSDRM): the
      console comes back and the display mode is restored.

## On the Pi 4/400

- [ ] **`-bench` frame match** with the PC runs, **`--scenario`**
      (`bench/mc2_19.fit`, the GLES run with the circling camera has not run
      even in WSL), and **`--perf`** (dwarf mode not tested anywhere). (10-31)
- [ ] **PR #6 hitch numbers**: never measured.
- [ ] **llvmpipe on the Pi** dropped from 50/67 fps to 45/48 and then about
      15 fps; cause never confirmed from `results.csv`. Low priority.
- [ ] **VR Phase 0 stage times** (`-bench`, `<stage>_ms` columns, with and
      without `--fov`): the Pi runs for the go/no-go
      (`future-features/VR-TABLETOP.md`).
- [ ] **Render performance candidates** (`future-features/PI-RENDER-PERF.md`):
      each is measured with `scripts/bench.sh` on the Pi before and after.

## On the Quest (VR, `future-features/VR-TABLETOP.md`)

- [ ] **Controllers** (branch `vr-controllers`, Phase 3 progress): in the
      main menu and in a mission, either hand shows a beam, with a dot
      where it meets the interface and no mouse cursor; trigger clicks
      (one click per pull, nothing lost), grip right-clicks, drags on the
      panel work; a trigger pulled off the panel and swept onto it does
      not click; left stick pans the right way, right stick turns the
      right way and zooms (up is in); B + click force-fires; menu pauses;
      A brings the HUD in front, held A moves and sizes it and logs
      `MC2_XR_HUD`. The system menu or Guardian releases everything (no
      stuck scroll or button). 2026-11-01 (mouse cursor build): panel
      clicks and sticks fine; the rest not tried.

## Releases (`RELEASES.md`)

- [ ] **A package on a clean machine** (phase 1 exit): unpack the Linux x64
      package on a machine that has never built the game, install the apt
      line from `INSTALL.md`, build the data with `./build_gamedata.sh`,
      start a mission.
- [ ] **The Windows zip** on a Windows PC without Visual Studio: it starts
      (the C++ runtime and DLLs are next to `mc2.exe`), with data copied
      from a Linux build.
- [ ] **The Pi package on Raspberry Pi OS 64-bit**: the apt line is enough,
      and what the first start writes to `options.cfg` (RELEASES.md: ship a
      low-detail template if it picks too high).

- [ ] **Phase 5, `v0.2.0-rc1`**: run by hand on Windows, Linux x64 and a
      Pi 400.
- [ ] **Phase 6, `v0.2.0`**: Pi hardware results in the release notes.

## Done

Move items here with the date, the machine and the result.

- [x] Pi 400, GLES build, mc2_01 (2026-10-29, at `10401db`): runs, 58.5 fps
      at the 60 Hz vsync cap; software rendering 50.4 (mc2_01) / 67.1
      (mc2_03) fps.
