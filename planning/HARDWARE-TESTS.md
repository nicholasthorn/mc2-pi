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

## Network transport

- [ ] **Windows to Quest, or Windows to Linux** (#113). Windows uses BCrypt and the
      others BoringSSL, so this proves their handshake agrees, which CI does not
      (each job runs both ends on one platform). `net_ping server 27900` on one,
      `net_ping client <its address> 27900` on the other; both ways round. On a
      Quest: `scripts/android/net_ping.sh`. Allow UDP through Windows' firewall.

## Multiplayer

- [ ] **Two machines on one network, in windows** (phase 2). GL on both, then a Pi or GLES
      build on one. On one: `./mc2 -mission mc2_m08 -nethost 2 -netlisten 27815`. On the other:
      `./mc2 -mission mc2_m08 -netjoin <its address>:27815`. Allow UDP 27815 through a firewall.
      Order units on both, from both. Do the other side's mechs walk smoothly, or stutter and
      warp? Do shots, hits and explosions look right on the client? That answers whether the
      legacy movement stays. Quitting mid-game is not expected to be tidy yet.
- [ ] **A Windows peer** (phase 2). The same, one end on Windows: also proves the BCrypt
      handshake in the game (see "Network transport").
- [x] **A Quest as host and as client, unworn** (2026-11-05, Quest 2, PC in WSL, home Wi-Fi): played
      and measured, with armour never differing. See "On a Quest 2" in `future-features/NETWORKING.md`.
      `-netjoin wait` on the Quest and `-netcall <its address>` on a host in WSL, which it cannot reach.
- [ ] **A game found on the network** (phase 4). Two machines on one network, the same data:
      `./mc2 -mission mc2_m08 -nethost 2 -netlisten` on one, `./mc2 -mission mc2_m08 -netjoin lan` on
      the other. It should find and join within a few seconds. A Quest as each end, as its Wi-Fi drops
      broadcasts without the lock in `MC2Activity`. Then a router with client isolation, which should
      fail to find it, and joining by address is the way round. WSL cannot host for this (NAT).
- [ ] **Support and results, seen** (phase 3). In windows, each side: press a support button and
      click (repair truck, scout, minelayer, guard tower), watch the VTOL and the vehicle arrive on
      both screens; order the truck to repair; down a mech and call the Karnov; check the points on
      screen after each, and after the Stop button. At the end, the results screen: names, team,
      laurels on the winner, and Close quits. Tests check the state of all this, not how it looks.
- [ ] **A Quest in a network game, worn.** Same as above with someone in it: do the other side's
      mechs look right, do orders given by hand with the controllers work, and does taking the headset
      off and on leave the game running? Also whether the app quits by itself at the end.

## In the game (any PC with a display)

- [ ] **HUD at 1920x1080 and 2560x1440** (`hud-centre`). The HUD is centred. Click
      the force icons, minimap and command buttons: each reacts where it is
      drawn. A pilot video shows at the top right, framed, not off-screen. GL or
      GLES. CI and screenshots show the position, not the clicks or the video. The
      video's caption now sits under it, on screen; at 1920 it is off to the left.
      The win or lose banner is centred, also with Omnitech at 1920.
- [ ] **Omnitech at 1920x1080** (`hud-centre`). A mission plays, on its 1600 layout,
      with the same checks.
- [ ] **Multiplayer host in a window** (#110). `./mc2 -mission mc2_m08 -nethost 2`:
      the mission loads and plays, units fight, and moving the camera away
      changes nothing visible when you come back. Do not expect a clean quit:
      leaving through the menu is known to crash in `MPStats::render` until
      phase 3. GL and GLES.
- [ ] **Dragging a mech out of the force group** (`fix/noticed-bugs`). In the
      mech bay, drag a force-group mech onto the mech list at the same height
      as the force-group icons: it must leave the force group. Then drag one
      from the list onto the icons: it must join. GL or GLES. `./mc2`, then the
      logistics screens.
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
- [ ] **Units keep moving after a load** (`fix/saved-path-sizes`). Save mid-mission with units walking a long way, load it: they carry on to their goals, and the log has no line starting "Saved".
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
- [ ] **Mercenary mode: is it fun?** (`future-features/OPEN-WORLD.md`, GL) Copy
      `merc/data` over the game's `data`, then New Campaign, "Mercenary Company
      (test)". Buy a lance, play contracts, beat the licence fight, jump. Headless
      walks cover the screens and one bench fight the win, but nobody has played
      it: judge the pay, prices and lance sizes, whether enemies come at you, and
      whether the contract and repair lines read well.

## VR interface between game steps (#127)

The interface only rests in a headset session, so no desktop run covers it.

- [x] **Quest 2, a mission played** (2026-11-05, fine): the interface updates promptly, health bars,
      timers and flashes run at the right speed (they advance per draw, and were
      probably about 2.4 times too fast before this), and floating help shows
      without ghosting.
- [x] **Pointing at the interface and at the world** stays right.
- [x] **The interface at 1280x960 in a mission** (2026-11-05): judged right on the
      Quest 2, with the size set by hand in `options.cfg`.
- [ ] **The same with the default:** the logistics screens and a mission started from
      them, on a headset whose `options.cfg` says nothing or 800x600.
- [x] **Leaving a mission or opening the options screen:** one frame of the old
      interface at most.

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

- [ ] **Bomber target marker and explosions** (branch `fix/bomber-marker-explosion`).
      Call a strike, then another close by: while the first marker spins, the
      second's explosion shows whole, with no square in it, and the marker's
      rings look soft rather than black-edged. Checked on the desktop with
      `MC2_VR_PATHS=1`, which never showed the square itself, so only the
      headset can confirm that part.
- [ ] **Game camera's field of view** (PR #86, `future-features/VR-FOV.md`).
      Done on a Quest 2: the log line, world clicks near the horizon and past
      the terrain's edge, the frame rate against `MC2_XR_FOV=0`. Still to see:
      brackets and health bars sit on their units at their usual size;
      a unit drawn past the terrain's edge can be clicked.
- [ ] **Game camera follows the head** (branch `vr-head-camera`,
      `future-features/VR-HEAD-CAMERA.md`). `MC2_XR_PICK_LOG=1` in `mc2args.txt`;
      `MC2_XR_FOLLOW=0` to compare. World clicks at a tilt of about 65 degrees on
      far ground (the log says "screen on", where #86 said off); click behind the
      camera's start after turning round; a click with the head moving is still
      one click, not a drag box, and lands where the beam was; a drag select while
      turning the head; pointing well away from the gaze clicks nothing, and never
      a mirrored spot; with the head still, a pointer on the world is clickable at
      once, and a quick pull while the head turns still clicks. With a level head a
      click never lands under the camera. Clouds, on a mission that has them, do not
      judder or jump with the head. Sticks pan and turn as before whichever way the head looks. Brackets
      and health bars stay on their units through a fast head turn. The minimap's
      wedge turns with the head and stays sane looking up at the sky. Buildings
      behind the start point change state when shot at while you look at them.
      `mc2.log`'s frame rate and `logic_ms` against `MC2_XR_FOLLOW=0` in the mc2_19
      bench, with the head still or `MC2_XR_POSE` set. The system menu changes nothing.
- [ ] **Controllers** (branch `vr-controllers`, Phase 3 progress): in the
      main menu and in a mission, either hand shows a beam, with a dot
      where it meets the interface and no mouse cursor; trigger clicks
      (one click per pull, nothing lost), grip right-clicks, drags on the
      panel work; a trigger pulled off the panel and swept onto it does
      not click; left stick pans the right way, right stick turns the
      right way and zooms (up is in); menu pauses;
      A brings the HUD in front, held A + left stick moves and sizes it
      and logs `MC2_XR_HUD`. The system menu or Guardian releases
      everything (no stuck scroll or button). 2026-11-01: panel clicks,
      sticks, system menu, click hold (side buttons, move orders, drags),
      A and held A all fine; held A moved to the left stick alone
      afterwards (to check). Not tried: grip, menu button, Guardian.
- [x] **Button map** (branch `vr-button-map`; all good on a Quest 2, 2026-11-02,
      double-pulling the trigger included): hold X and click ground: a mech
      runs. Hold Y and click ground: a jump-capable mech jumps. Hold the left
      stick in and click a building or ground: it fires there. Hold B and click
      several points, then let go: waypoints run in order. Right stick click
      selects the units in view. Pushing a stick to pan or turn does not click
      it by accident (a stray right stick click changes the selection).
      Added after that run, still to see: holding B, the left stick pans and the
      lighting stays put; in a text box (a save's name) X, Y and the stick clicks
      type nothing; X does not skip a movie.
- [ ] **A real fade in the headset**: an enemy mech fading in or out of sight
      looks translucent, without parts drawing badly over each other. Forced
      on the desktop only so far.
- [ ] **Straight ahead, the cases not yet tried** (branch `vr-start-pose`):
      hold the Oculus button to recentre (the log says `moving LOCAL`, and the
      view and HUD follow); take the headset off and on mid-game, also briefly
      while looking to one side (does the world turn under you?); start with
      poor light or position tracking off; put it on with the system menu up
      (it should align behind the menu, and again when the menu closes).
      Launched on the desk and then put on: fine on 2026-11-02.
- [ ] **Pointing at rotated buildings and long walls**: hover names and clicks
      land where the beam points, and a point on a long wall whose centre is
      out of the game camera's view still works. Rotated ones were checked on
      the desktop only, with rays from the game camera.

## Omnitech front end on the Quest (`omnitech-frontend-labels-mech`)

Checked on the flat desktop on GL and GLES, and with `MC2_VR_PATHS=1` (the VR
mesh paths on a flat window, which reproduced the solid mech); not on a headset.
Omnitech's menus run at 1280x1024, so try this with PR 96's frame size change in.

- [ ] **Mech view, Omnitech data:** mech bay, mech purchase and pilot ready (click
      a force slot with no pilot) show the mech textured, not a solid colour, and
      inside its frame, turning in place. The mech lab's component view too.
- [ ] **Mech view, stock data:** the same screens. The solid mech came from the VR
      path, not from Omnitech's data, so stock had it too.
- [ ] **Labels, Omnitech data:** no `undefined_string_sebi` on the mech bay, mech
      lab or pilot ready.
- [ ] **Pilot ready, Omnitech data:** the mission's map where the white box was,
      and the mech view fully covering the map and the pilot's panel when a slot
      with no pilot is clicked; click a pilot and the map is back.
- [ ] **A mission still looks right** after the menus: mechs textured and head
      tracked, as the world's meshes stay on the GPU path. In a mission, the
      gameplay options' mech preview is textured and in its frame too, and the
      world is unharmed on going back: no walk opens that tab in a mission.
## Quest playtest fixes (`quest-playtest-fixes`)

- [ ] **Video sound on the Quest:** a briefing video with its own sound
      (`node_*`, `cinema*`) is heard, and a skipped video's sound stops.
- [ ] **The buzz:** a steady buzz at about 1 Hz started late in long sessions on
      SDL's OpenSL driver and carried on into the menus; it was not in the
      game's mix. Android now prefers AAudio, and one session of about 31 minutes
      on it was clean. Play longer sessions, stock and Omnitech, and listen for it.
      `SDL_AUDIODRIVER=openslES` in `mc2args.txt` goes back to OpenSL to
      compare.
- [ ] **KIA box on Omnitech:** lose a pilot and open the pilot review; the box
      shows the pilot's face and an empty dead-picture box, not a second face. No
      UI walk reaches this box, so it has only been built.
- [ ] **Mechlopedia on Omnitech, by hand:** clicks land where they should now it
      is centred: its tabs, list rows, scroll bars and Close. Also salvage and pilot
      review after a mission, then back to mission selection at full size. UI walks
      press buttons by id, not by position.
- [ ] **HUD size on Omnitech:** play several missions in a row; the HUD stays
      full size and does not go small to the lower left. The fix follows a real
      change of frame size, logged as `[XR] the window's frame is now`; a menu
      mode change to 1280x1024 is what shrank it. Its first commit blames taking
      the headset off and on, which the log did not bear out.
- [ ] **Mech lab drags on the Quest:** drag 1x1 and 2x2 components out of the
      diagram and from the list; drag with the other hand's trigger brushed.
- [ ] **Saving with no keyboard:** a new save and a new variant start with an
      unused name, OK works, and the save loads.
- [ ] **Flat mech lab:** drag a component by mouse from its edge, and flick it
      within the diagram; it should still pick up what was pressed. A flick that
      leaves the diagram before the drag counts picks up nothing, and a component
      that cannot be removed sounds its refusal once.

## Releases (`RELEASES.md`)

- [ ] **Windows full zip** (`release-full`). On a Windows PC with nothing
      extra installed: unzip the `-full` zip, double-click `mc2.exe`, no first
      run exit, the main menu and a mission work. GL. CI cannot run this one.
- [ ] **Full package on a Pi 400** (`release-full`). Unpack, the `apt` line,
      `./mc2`: the settings suit the Pi. CI runs it only under llvmpipe. GLES.
- [ ] **Quest APK from CI** (`release-apk`). Follow `dist/INSTALL.md` with the
      release's APK and shaders on a Quest 2: the app starts in VR, a mission
      loads, and `mc2.log` has a `[LOG] MechCommander 2 <version>` line naming the
      release. GLES.
- [ ] **Player log on Windows and the Quest** (`log-file`): double-click
      `mc2.exe`: `mc2.log` appears in the game folder (or the config folder if
      `HOME` is set), the console still shows the output, and a second start
      keeps `mc2.previous.log`. A crash on either adds `CRASH:` lines to the
      log. Linux was checked under Xvfb only.
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

- [x] Quest 2, blending on the GPU mesh path, mc2_01 (2026-11-02, PR 84):
      the compass and tree edges look right; fly-in frame time unchanged,
      69.4 fps. With the lighting fix: near trees in colour, no step in
      brightness when zooming, no shader link failure on the Adreno driver.
- [x] Pi 400, GLES build, mc2_01 (2026-10-29, at `10401db`): runs, 58.5 fps
      at the 60 Hz vsync cap; software rendering 50.4 (mc2_01) / 67.1
      (mc2_03) fps.
