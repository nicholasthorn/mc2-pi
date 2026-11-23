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
      laurels on the winner, and Close (from `-mission`, it quits; from the menu, back to the
      connection screen). Tests check the state of all this, not how it looks.
- [ ] **A game from the main menu, by hand** (phase 4). Two windows or machines, no flags. Multiplayer,
      Local network: host on one (name it, Load Map), Join on the other, pick the game. On the
      parameters screen: is everything readable, do hidden buttons leave odd gaps, does each player's
      team list work, does the client's Ready press and show on the host, does Launch count down and
      stop if someone un-readies? Are the two sides in different colours in the mission? Then leave a
      lobby each way, and play a second game after the results. On the prefs screen: a name, base and
      stripe colours, the mech turning in them, a colour someone has being refused, Cancel; does the
      insignia list work (each player's insignia is not sent to the others yet)? A test walks all this
      by calling the screens' handlers, and sets a client's ready itself, so the mouse is not covered.
      Also: the connection screen opens on Local Network with the Zone and TCP/IP tabs off; the host
      options that do nothing yet are hidden, and is what is left tidy (labels with no controls)? The
      load screen while waiting for a slower machine; close the host in play: does the client get to
      the main menu with a message? A client with another version of a map: the message, and Ready off.
- [ ] **Mech selection in a game from the main menu, by hand** (phase 4). As above, but the host
      unticks Quickstart and sets the C-Bills (and the drop weight) before Launch. Each player then
      goes through the briefing, buys mechs, puts them in the force, gives each a pilot and presses
      Launch on the pilot screen, which waits for the other. Does each screen show the right C-Bills
      and drop weight, can a client not change them, and does the first to launch see it waiting? In
      the mission: are each side's mechs the ones bought, with the pilots given, in its colours, on
      its side of the map? Two players who buy several mechs each: do they start apart, not on top
      of each other? A test walks this with one mech each, by the screens' handlers.
- [ ] **Chat and C-Bills per player, by hand** (phase 4). In the lobby, type a line on each side:
      does it show on both, with the right name and colour? A team line: does only that team see it?
      As host, change one player's C-Bills with the spinners on their row: does that client's mech
      bay have that much to spend, and does changing the amount for everyone then reset it? With
      Quickstart off, does the pilot screen say "X is ready to launch." on the other side when one
      launches, and does a client see the host's "about to start" line on Launch? Chat in play
      (Enter): does it reach the other side?
- [ ] **The same on a Quest, by controller** (phase 4). Whether the screens can be used at all.
      The Quest's multicast lock is still held for the app's whole life.
- [ ] **A Quest in a network game, worn.** Same as above with someone in it: do the other side's
      mechs look right, do orders given by hand with the controllers work, and does taking the headset
      off and on leave the game running? Also whether the app quits by itself at the end.

## In the game (any PC with a display)

- [ ] **No CD dialog when a pilot talks, Windows** (#136). With data built
      `--no-movies` (the Windows `-full` package), play `mc2_01` until the first
      pilot message: no "insert the CD" box, and the video box beside the pilot
      stays empty. CI's smoke runs only on Linux, where the box never showed.
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

- [ ] **Selection groups** (branch `spec/selection-groups`), stock and Omnitech,
      any renderer. Every icon shows a 1 at the right end of its strip at the
      start. Select two mechs, Ctrl + 2: their strips change colour and show 2;
      press 1, 2, Shift + 1. Grave goes 1, 2, 1...; Shift + grave goes back.
      Select one mech, Ctrl + grave: it becomes group 3. Let a group's units
      all die: its number and grave leave the selection alone. Save, load,
      press 2. Check that a long pilot name does not run into the digit (it is
      cut short). Grave, Shift + grave and Ctrl + grave say "Group n" / "New
      group: n" in the message lines, stepping through groups changes one line,
      and F1 and the hotkey options list the three commands (key shown as `).

## Path-finding on a worker, VR mesh list (branch `perf/vr-path-worker`)

Only a headset session holds a game step back for the worker, so no desktop check or
CI run covers that.

- [ ] **Quest 2, a mission played to its end, then the next:** units move, order
      changes take effect promptly, radio chatter plays; no crash leaving the mission
      or starting the next, also with the mech bay skipped. Bench runs have not done this.
- [ ] **Same state across runs:** two `-deterministic` headset runs of the same
      mission print the same `[STATEHASH]`, with steps held back. Not yet compared.
- [ ] **Multiplayer on a headset** (`-nethost` or `-netjoin`): a battle plays to its
      result with the worker on.

## VR interface between game steps (#127)

The interface only rests in a headset session, so no desktop run covers it.

- [x] **Quest 2, a mission played** (2026-11-05, fine): the interface updates promptly, health bars,
      timers and flashes run at the right speed (they advance per draw, and were
      probably about 2.4 times too fast before this), and floating help shows
      without ghosting.
- [x] **Pointing at the interface and at the world** stays right.
- [x] **The interface at 1280x960 in a mission** (2026-11-05): judged right on the
      Quest 2, with the size set by hand in `options.cfg`.
- [x] **Every mech drawn at FogPos 75** (`fix-shape-pools`, 2026-11-10): mc2_16 in a
      Quest 2, all five of the lance with their shadows (left-eye capture).
- [ ] **No `TGL pools full` in a long battle:** a big fight late in a mission
      at FogPos 100 in the headset, then check `mc2.log`.
- [x] **Objects in height fog** (`fix-mesh-height-fog`, 2026-11-10): mc2_16 in a
      Quest 2, trees, buildings, mechs and the sky textured, not the fog colour
      (left-eye capture).
- [x] **Sharp interface from launch** (`vr-hud-frame`, 2026-11-10): on a fresh start,
      without taking the headset off, the HUD text is as sharp as it was after a
      headset off and on (judged in a Quest 2).
- [ ] **The pointer with the full-size frame:** it still lands where it aims on the
      panel, on the logistics screens and in a mission.
- [ ] **The same with the default:** the logistics screens and a mission started from
      them, on a headset whose `options.cfg` says nothing or 800x600.
- [x] **Leaving a mission or opening the options screen:** one frame of the old
      interface at most.
- [x] **Panel sized from the layout** (`vr-interface-scale`, 2026-11-11): in a Quest 2,
      the stock menus no longer pixelated, Omnitech's front end and missions as
      before, the pointer and screen changes fine (judged in play, MC2 test and
      Omnitech).
- [x] **Interface panel without sparkle** (`vr-panel-mipmaps`, 2026-11-13): the main menu,
      logistics and the mission HUD, moving the head slowly: no shimmering text or edges (judged
      in play, MC2 test and Omnitech). `MC2_XR_HUD_MIPS=0` in `mc2args.txt` turns it off to compare.
- [ ] **Cost of the panel's mip levels** (PR 151): a `-bench` run with and without
      `MC2_XR_HUD_MIPS=0`, the interface's redraw time on game-step frames.
- [x] **Omnitech's results screens** (2026-11-11): after a Clan Wars mission the
      results' Done button showed and left the screen.

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
- [ ] **Selection groups on the controllers** (branch `spec/selection-groups`):
      right stick click still selects the units in view (now on release);
      double click steps through the groups; held half a second, the
      selection becomes a new group (its icons show the new number); hold X
      and click the right stick: the previous group, and no mech runs. A
      double click that is too slow, or a hold that is too short, should not
      be a common miss: say if 0.3 s and 0.5 s need tuning. Double click
      while pushing the right stick sideways: still the next group (the camera
      pauses for a moment), not the previous one.
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

- [ ] **No hard edges where the world ends** (`vr-round-terrain`). Seen on a
      left-eye capture from an unworn Quest at fog 100 (mc2_19); nobody has looked yet. Look at the horizon and up, on a day mission (`mc2_01`), a night one
      (`mc2_19`), a pure black fog one (`mc2_05`) and Omnitech: no square of ground, no
      line where the sky stops, no black. Does the sky look right when it is level with
      you, and does it stay still as the camera moves and the head turns? Do units in
      the fade read as far away, or vanish too soon (it starts 1216 units from where the
      camera looks, at the Quest's fog setting)? Does the frame rate hold? The window's
      own clear must stay see-through: the HUD has no solid background.

- [ ] **Ground near the eye in VR** (`vr-terrain-near-gap`, PR 145). Looking down, ground
      triangles crossing the eye's near plane were dropped, a pale gap at the bottom of
      the view; the GPU now clips them. Checked only on fixed-pose captures from an
      unworn Quest (mc2_16 at `MC2_XR_POSE=0,-35`, and mission 1's fly-in looking down and
      level, fog 75). Wearing it: look straight down and behind, zoom in and out, and
      watch a fly-in. No gap, no flicker at the bottom of the view, no stretched or torn ground
      near the feet, and water near the camera clips cleanly too.

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

## Phones and tablets (`future-features/MOBILE.md`, branch `android-flat`)

GLES. Build with `MC2_ANDROID_FLAVOUR=phone scripts/android/build.sh`, install and
push the data as in `android/README.md`, "Phones and tablets".

- [ ] **It starts and fills the screen** on an Android tablet or phone (Android 10 or
      later): no status or navigation bar, nothing left black by a camera cutout.
      The menus are 4:3 in the middle with black bars; a mission fills the screen
      unstretched (round things round). Note the device, its GPU and the screen size.
      Done on a Pixel 9 (Mali-G715, 2424x1080), 2026-11-15.
- [ ] **A mouse plays mc2_01** (phase 0's exit): a Bluetooth or USB mouse and keyboard
      through the campaign start, logistics and the mission to its end. Clicks land
      where the cursor is drawn, across the whole screen.
- [ ] **Menus and logistics by finger** (phase 1): taps click where the finger is, a
      double tap selects all of a type, a held or moved finger drags (mech bay, pilot
      screens). No click should come from landing two fingers. Is the 80 ms wait before
      a held finger presses noticeable?
- [ ] **Camera by two fingers** in mc2_01: pinch out zooms in, pinch in zooms out, one
      zoom level per quarter of spacing; moving both fingers scrolls the map with them
      (not against them). Are the slop (2% of the screen height) and the scroll dead
      zone (6%) right on a phone and on a tablet?
- [ ] **A mission by touch** (phase 2) in mc2_01: tap a mech to select it, tap ground
      to move, tap an enemy to attack (the one-button profile while touching). The
      command bar shows on the right edge only in the mission, not in menus or the
      pause menu. Waypoints: tap it, tap several points, tap it off: the mech walks the
      route. Add: tap it, tap two mechs: both selected. Force fire: tap it, tap ground:
      they fire at it, and the button goes out. Visible, Group and New grp do as E,
      grave and Ctrl + grave. Does the bar cover anything that matters (radar, the
      top right messages)? Are the buttons big enough on a phone?
- [ ] **Long press** in mc2_01: hold a finger still on one of your mechs for half a
      second: the camera follows it. Hold on empty ground, then drag: the camera turns
      and tilts. Is half a second right? In a menu, holding a list's scroll arrow still
      scrolls.
- [ ] **On-screen keyboard:** tap the pilot name field (prefs) and a save game name:
      Android's keyboard comes up, letters, digits, backspace and Enter type into the
      field, and it goes away when the field loses focus. Is the field hidden under
      the keyboard on a phone (it goes over the game, adjustNothing)? Cancel the save
      dialog with the keyboard up: does the keyboard go too? Hide it with Back, then
      tap the field: it comes back.
- [ ] **No edge scroll from a tap:** in mc2_01, tap a control panel button at the bottom
      edge and lift; the map must not keep scrolling. Then use a mouse: edge scrolling
      works again.
- [ ] **Split screen or a foldable** (Android): change the app's size while it runs; the
      picture fills the new size and taps still land under the finger.
- [ ] **A touch screen laptop** (Windows or Linux, GL): taps and the two finger
      camera work, and moving or clicking the mouse afterwards gives the cursor back
      to it. On Linux under X11, check that a tap clicks once, not twice (X's own
      touch-to-pointer emulation).
- [ ] **Switching away and back** mid-mission (phase 3): press Home, open another
      app, come back. The mission is paused with the pause menu up and has not run on;
      the sound stops while away and comes back. Does the picture come back with its
      textures? If the GL context was lost, the game quits with an `[APP] FATAL` line
      in `mc2.log` instead. Note the GPU (Adreno, Mali, PowerVR); try two vendors.
- [ ] **A phone call or the screen turning off** mid-mission: the same as switching
      away. In a menu nothing pauses, and the menu is as it was.
- [ ] **The resume save** (phase 3): in a campaign mission, give a few orders, go to
      Home, then close the app from the recent apps list. Start it again: it goes
      straight into the mission, paused, with the units where they were, and the
      campaign carries on after it (results, then logistics with the same mechs and
      pilots). Then switch away and back without closing: the next start is the
      main menu, not the old mission. How long does the save take when switching
      away (`[RESUME] saved` in `mc2.log`)? The pause menu slides in at the right
      edge with its buttons (it once came up as a black panel: fixed, checked on a
      Pixel 9 with `-suspendexit`).
- [ ] **Exit and start again:** Exit Game from the main menu, then open the app at
      once from the launcher. It starts normally; `mc2.log`'s previous run ends with
      `[APP] main ended: closing the process` and no `CRASH` lines.
- [ ] **Switching away at a mission's end:** go to Home while the mission-won
      sequence or an in-mission movie plays. Does the pause menu come up over it on
      return, and does closing it carry the end of the mission on as normal?
- [ ] **The Quest now pauses too:** on a Quest, take the headset off mid-mission until
      it sleeps, then put it back on: the mission is paused with its pause menu
      (new with phase 3), which the controllers close. Is that wanted in VR?
- [ ] **A data update over adb-pushed data:** with the data pushed by `adb` and little
      space left, install a full APK: it installs (only new or bigger files count
      against the space), and files the new data no longer has are removed
      (`[DATA] removed` lines in `mc2.log`).
- [ ] **The full APK on a fresh device** (phase 3's exit): build with
      `MC2_ANDROID_DATA`, install it on a device that has never had the game data
      pushed, start it: toasts count the install up (how long does it take?), then
      the main menu, with no first-start exit (the old sniffer). Start it again: no
      second install. Install an APK with changed data over it: it copies again, and
      the settings and saves are still there.
- [ ] **Not enough space for the data:** fill the device until less than the data's
      size is free and start a fresh full APK: a message box says how much is needed,
      and the app closes.
- [ ] **Beside the Quest app:** on a Quest, the phone APK installs next to the VR one
      and runs as a flat window in the headset's home.

## AR tabletop (`future-features/AR-TABLETOP.md`, branch `ar-tabletop`)

Quest: the Quest build with `MC2_XR_TABLE=1 -mission mc2_01` in `mc2args.txt`.

- [x] **The battle stands on the table**, the room round it through passthrough,
      and walking round it or leaning in moves the view. Done on a Quest 2,
      2026-11-16 (after the head position fix: it was on the ceiling).
- [ ] **The interface panel reads easily**: above the table (20 degrees over the
      horizon, 1.5 m away, 1.6 m wide) with no need to cross the eyes. Hold A
      and push the left stick to move it; the log prints the `MC2_XR_HUD` value
      to keep. Placed by hand at 17 degrees and 1.54 m on a Quest 2, 2026-11-16,
      where the old limit stopped it; the new default is not yet seen.
- [x] **Clicks on the table** select and order units where the pointer meets the
      ground (the ray is in the table's frame). Done on a Quest 2, 2026-11-16.
- [x] **A puts the table at the right controller**, laid on a real table, the
      right stick zooms it (the log prints `MC2_TABLE_SCALE`), and the left
      stick scrolls the map across it. Done on a Quest 2, 2026-11-16 (mc2_05,
      zoomed between 1:285 and 1:1673).
- [ ] **Normal VR (no `MC2_XR_TABLE`)** still feels right: leaning and rising
      moved the eye the wrong way before the fix, by centimetres.
- [x] **A drag box on the ground** (Quest, in normal VR and on the table):
      drag the trigger across some units; the box lies on the terrain round
      them, a white edge and a faint fill, in both eyes, not on the interface
      panel, and the units inside it are selected. Done on a Quest 2 on the
      table, 2026-11-21 (mc2_21): the corner follows the pointer. Normal VR
      not yet seen with the fix.
- [ ] **Model views on the tabletop**: with the tabletop on, the mission
      briefing's mech and objective views and the mech lab's mech turn as
      usual, seen from the side, not from straight above.
- [x] **"Tabletop view" in the graphics options** (Quest only, under the
      hardware rasterizer box): tick it, Accept, and the battle goes onto a
      table ahead of you with the room round it (`[XR] tabletop on`); untick
      it and the battle is all round again, with the haze back where the fog
      setting puts it (`[XR] tabletop off`). Both in a mission and from the
      main menu; it is still set after a restart (`VRTabletop` in the
      options file). Cancel puts the old setting back. With no saved setting
      `MC2_XR_TABLE=1` still starts it on. Not on desktop or the phone.
      Done on a Quest 2, 2026-11-20: off and on again in mc2_23, the table
      back ahead at the zoom set before (1:391), saved as `VRTabletop`, no
      errors in `mc2.log`.

Phone: the phone build (`MC2_ANDROID_FLAVOUR=phone`) with `MC2_AR_TABLE=1` in
`mc2args.txt`, on a phone with ARCore (Pixel 9). GLES.

- [x] **The camera asks and ARCore starts**: the permission prompt on the
      first start, then the camera image fills the screen behind the game;
      `[AR] tracking` in `mc2.log`. Done on a Pixel 9, 2026-11-16. Still to
      see: turning the phone over (the other landscape way) keeps the image
      upright and unstretched.
- [x] **The battle stands on the table**: pointed at a table and moved a
      little, `[AR] table placed on a surface` and the battle on it, staying
      put as the phone walks round it and leans in, at a sensible size. Done
      on a Pixel 9, 2026-11-16 (mc2_01 on a desk; at first all haze, fixed).
- [x] **Taps on the table** select and order units where the finger is; taps
      on the interface work as in a flat game. Two fingers scroll the map.
      Done on a Pixel 9, 2026-11-16.
- [ ] **A drag box on the table**: drag one finger across some units; the box
      follows the finger without jumping, and selects the units inside where
      the finger lifted. (It jumped and ended in the wrong place before the
      fix that keeps the pointer put while a point waits.) The box lies on
      the ground of the battle, a white edge and a faint fill, and the units
      selected are the ones inside it.
- [ ] **The table stays put after placement**: once `[AR] table placed`
      is logged, ARCore stops looking for surfaces; walk round the table and
      come back: the battle has not drifted off the desk.
- [ ] **Units near the table's edge**: tap a unit at the edge nearest the
      phone, and ground there: it selects and orders, the panel is not
      pressed. The same on a Quest 2 by pointing (it could not pick there:
      the game camera now keeps the table off the interface on its screen,
      `vr_eye::clearRadius`). `MC2_XR_PICK_LOG=1` logs the clear radius.
- [ ] **Switching away and back** (Home, then the app): the camera comes back
      and the battle is where it was.
- [ ] **Without `MC2_AR_TABLE`** the phone plays flat as before (no camera
      prompt).

## On a Mac (`future-features/MACOS.md`, branch `macos-port`)

- [ ] **A mission on an M-series Mac** (GL, macOS GL 4.1). Homebrew `sdl2 sdl2_mixer sdl2_ttf glew
      ninja cmake`, then the Linux desktop GL build commands with
      `-DCMAKE_PREFIX_PATH="$(brew --prefix)"`; game data built on any machine and copied over.
      `./mc2 -mission mc2_01 -autoexit 3000` from the game directory: it should reach `[AUTOEXIT]`
      with no `OpenGL Error` or shader compile errors in the output, and the fog and mech lighting
      should look as on Linux (they come from the `SceneData` block that macOS binds by name).
      Then a few minutes played by hand: mouse, right click (two-finger tap), keyboard, sound.
      Note the macOS version and the chip.

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

## MC2X (`future-features/MC2X-COMPAT.md`)

Built from MC2X's one-line help texts and checked by unit tests and headless
smoke runs (plus a few screenshots); nobody has used them in play. Needs a
game folder from `scripts/mc2x_import.py` (the player's own downloads) and a
window of at least 1280 wide: MC2X's layouts start at 1024 and put some
buttons off the screen there. Run on **GL**; the same on **GLES** and in the
Quest's MC2X app (`com.nicholasthorn.mc2.mc2x`) where noted.

- [ ] **A REAP3 mission from the menus:** main menu, the operation map with
      all 24 missions and their pictures, briefing, mech bay, launch; MC2X's
      own art on every screen, no stock screen showing through.
- [ ] **Formations:** select three or more units, open the Formation button's
      palette, pick line, wedge, column and circle in turn and order a move
      each time. The group ends in that shape round the point clicked, facing
      the way it moved, and the indicator shows the formation chosen. "None"
      moves as stock does.
- [ ] **Support units, both tabs:** call in a drone, the kamikaze swarm (five
      arrive), a humvee, the infantry carrier and one unit from the second
      tab. Each is charged its listed price in resource points, comes in by
      VTOL and takes orders. A mission that switches a unit off
      (`<x>EnabledDefault`) shows its button greyed.
- [ ] **C-Bills and the spy map:** Button22 turns 10,000 C-Bills into
      resource points, Button23 all resource points back; the spy map
      (1,000,000 C-Bills) shows every unit on the tac map for 60 seconds,
      then goes back to what the sensors see. Each button greys out when it
      cannot be afforded.
- [ ] **Dropship fire:** select a spheroid dropship, press its fire button and
      click the ground: it fires at that point.
- [ ] **Hide ID:** hide a unit and walk it near an enemy: the enemy does not
      react until the unit fires or is un-hidden.
- [ ] **Pick-up and drop-off:** a Transport or Lander picks up a friendly mech
      and puts it down elsewhere, on open ground even when the transport
      hovers over water or a cliff. A transport destroyed with cargo puts the
      cargo down where it fell. Save with cargo aboard, load: the cargo is on
      the ground.
- [ ] **Recharge and reload:** a supply carrier in recharge mode next to a
      repair vehicle refills its refit points; in reload mode, the
      ammunition of units within reach. Unit tested only so far.
- [ ] **Exit unit:** the pilot ejects unhurt and the unit stands empty; the
      salvage craft can give it a pilot again.
- [ ] **The same mission on the Quest** (MC2X app, controllers only): the
      formation palette, both support tabs and the unit buttons can be
      reached and clicked on the panel.

## Done

Move items here with the date, the machine and the result.

- [x] Quest 2, blending on the GPU mesh path, mc2_01 (2026-11-02, PR 84):
      the compass and tree edges look right; fly-in frame time unchanged,
      69.4 fps. With the lighting fix: near trees in colour, no step in
      brightness when zooming, no shader link failure on the Adreno driver.
- [x] Pi 400, GLES build, mc2_01 (2026-10-29, at `10401db`): runs, 58.5 fps
      at the 60 Hz vsync cap; software rendering 50.4 (mc2_01) / 67.1
      (mc2_03) fps.
