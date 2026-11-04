# Multiplayer: proposal

Status: proposal; phase 0 built (see "Phase 0 results"). Phase 1 nearly built: the
wire format (`code/netwire.*`), the transport interface (`GameOS/gameos/gos_net.*`) and
a real backend on GameNetworkingSockets, in every configuration (`cmake/mc2_gns.cmake`).
A Windows peer has not met another platform's. Phase 2's vertical slice runs on one PC
(see "Phase 2 results"): two processes play a battle to its result. It has not been on
a real network or a headset, and nobody has watched it.
Revised 2026-11-04 from a read of `pi-port` at `bec9e85` and of current library
and Meta docs; replaces the feasibility study of 2026-10-29. Apart from phase 0,
nothing here has been run: code findings are from reading it. Line numbers are
not recorded on purpose (grep the symbol).

## Summary

- **Model: one authoritative host, clients send orders and receive state.** Not
  lockstep. This is what the original game did, the hooks for it are still in
  the code, and it is the only model that survives a mix of x64 and arm64
  players and a player taking a headset off.
- **Keep the gameplay branches and the chunk formats. Replace the wire format
  and the transport, and rewrite the empty `MultiPlayer` methods.**
- **Transport: GameNetworkingSockets (GNS)**, which now builds on every target and
  has run on a Quest. ENet stays the fallback until a Windows peer has met another
  platform's. GNS brings encryption, congestion control, ICE NAT traversal and a lag
  simulator.
- **Same room needs less than it looks.** Quests on one Wi-Fi network, each with
  its own view, need no shared coordinates and nothing from Meta. Alignment is
  needed only for one shared physical table, which comes after the VR tabletop
  mode, and two controller touches on the table can provide it with no cloud.
- **Start with the riskiest part.** First run a mission with the server branches
  on and no network at all, then two processes in one mission from the command
  line. The lobby comes later: the old plan's lobby go/no-go proved little.

## Problem

MC2 shipped with LAN and internet multiplayer over DirectPlay 8. This port has
none, and the main menu's Multiplayer button is disabled (`mainmenu.cpp`).

Wanted:

1. **Same room:** several Quest 2s in one room playing together.
2. **Remote:** ordinary internet play, without asking players to forward ports.
3. **Mixed:** a PC, a Pi and a Quest in one game, since all three are targets.

## Scope

In: head-to-head and co-op (see "Game modes"), 2 to 8 human players, host on any platform,
LAN discovery, direct connect, internet rendezvous, chat, cross-play between
all four build configurations and Quest, leaving and rejoining after a short
drop.

Out at first: dedicated server, join in progress for a player who was never in
the mission, host migration (the original had it), ranked play, accounts,
anti-cheat beyond host validation, built-in voice, store distribution (the game
data is under the original EULA, every player brings their own).

## What is in the code

**Gone.** Microsoft's source release had the layer removed already:

- No sockets anywhere, and no `gos_Net*` implementation. `GameOS/include` has
  the declarations and about 3,400 lines of orphan network headers.
- `code/multplyr.cpp`: about 140 `MultiPlayer` methods, nearly all empty,
  including `update`, `processMessages`, `updateClients`, `calcMissionStatus`,
  every `send*` and `handle*`, and `WorldChunk::pack` / `unpack`.
- `isServer()` returns false, so a non-null `MPlayer` today takes the client
  branch everywhere.
- Win and hill logic went with `calcMissionStatus`. `LAST_MAN_STANDING` has no
  reference outside its enum, and recovery in multiplayer never existed:
  `STOP((" Need to implement recover for multiplayer "))` in `tacordr.cpp`.

**Kept, compiled, and not run since 2001:**

- **Hooks:** about 1,030 lines mention `MPlayer`, 94 of them `isServer()` across
  18 files. Only the host applies damage and destroys things, clients return
  early from physics and take their state from chunks, and clients skip ABL
  alarm handlers.
- **Chunks:** `MoveChunk`, `StatusChunk` (`mover.*`), `WeaponFireChunk`,
  `WeaponHitChunk` (`gameobj.*`), `ArtilleryChunk` (`artlry.*`) and
  `TacticalOrder::pack` / `unpack`. They are bit-packed into one or two words.
  `build*Chunk` and `handle*Chunk` have definitions but no callers: their
  callers were in the removed code.
- **Client movement:** `updateNetworkControl` and `netUpdateMovement` walk a mover
  along steps from its `MoveChunk`, and warp it if it strays past `warpFactor`.
- **Protocol:** 42 `MCMSG_*` types and 28 live message classes (`multplyr.h`), for
  documentation of intent more than for reuse: they are raw structs with `long`
  fields.
- **Screens:** connection type, browser, host, lobby (`mpparameterscreen`, about
  2,400 lines), map load and stats compile. `mpdirecttcpip` and `mpaddaiplayer`
  do not.

CLAUDE.md's "UI stubs" undersells this: the transport and the `MultiPlayer` logic
are gone, the game side of the protocol is not.

## Decision 1: host-authoritative, not lockstep

| | Lockstep | Host-authoritative (proposed) |
|---|---|---|
| x64 with arm64 | Needs bit-identical floats across compilers and libcs | No requirement |
| Work before the first game | Large sim refactor | Fill in the stubs |
| Headset taken off | Everyone stalls | Only that player is affected |
| Client CPU | Runs the full sim | Can skip AI and pathing |
| Bandwidth | Very low | Low: see below |
| Rejoin | Replay or a state dump | Next state sweep, plus a catch-up message |
| Fairness | Equal | Host has no lag, and can cheat |

Lockstep is the classic RTS answer, but this sim is a poor candidate. Found in
the code:

- **Variable step:** one sim step per frame with `frameLength` from wall-clock
  time. `-deterministic` fixes it but only promises a match "on one machine".
- **RNG:** `gos_rand` is libc `rand()`, so the sequence depends on the libc
  (glibc, bionic, UCRT). Cosmetic code draws from the same stream (weather,
  `mech3d`, debris, beam wiggle, crater angles), so options and visuals change
  sim rolls.
- **Floats:** no `-ffp-contract` is set. GCC's default permits fused
  multiply-add on arm64, which baseline x64 lacks. `sin`, `cos`, `pow` come from
  each platform's libm. `SortList` uses `qsort`, whose order for equal keys
  depends on the libc.
- **Camera changes the game:** a withdrawing mech is destroyed when out of view
  (`withdrawing && !inView`), terrain objects update only in active blocks, and
  in VR the game camera now follows the head (`VR-HEAD-CAMERA.md`). In lockstep,
  turning your head would desync the match.
- **Animation feeds the sim:** jump and speed logic reads gesture state, and
  `Mech3DAppearance::update` draws random numbers.

Frontier's GDC 2024 talk on a cross-platform lockstep RTS reached fixed-point
maths, at a performance cost, with a sim designed for it. Doing that to 2001 code
is a rewrite.

**Rollback and client prediction: rejected.** An RTS order tolerates 100 to 200 ms.
Clients acknowledge at once locally (order marker, pilot voice) and wait for the
host for everything else. This is the one place a modern action-game technique
is not worth its cost.

**The topology is client-server,** with the host's machine also playing. No
peer-to-peer mesh: all traffic goes through the host, even when the link to
it was set up with NAT traversal.

## Decision 2: what the host and clients run

**Host.** Runs the game as it does now, with the server branches on, and
sends state. Its load is single-player's plus packing, which is what a Quest
already runs: the step is about 11 ms at 30 Hz (`MC2_XR_LOGIC_HZ`). Sending
should not need its own frame budget, but this is to be measured, and frames
with a step are already over budget on Quest (21 ms against 13.9).

**The host's camera must stop affecting the game.** State sync hides this as a
desync, but it is still wrong: a host looking elsewhere would change what
everyone plays. To fix when `isServer()`, leaving single-player behaviour alone:

- withdrawing movers are destroyed by `inView` (mech and vehicle),
- `Terrain::objBlockInfo[].active` gates terrain-object updates,
- `Team::home` / `Commander::home` (43 uses in mech and vehicle alone) assume one
  local human; each needs checking for host-versus-player meaning.

**Client.** Keep the legacy split: movers under `CONTROL_NET` walk their steps,
animate and play effects locally. Cosmetic randomness may now differ per
machine, which is harmless. A client that skips pathing and AI may run a
cheaper step than single-player, which matters on a Quest: **measure it in
phase 2.** It may be the most useful side effect of the design.

**Sim timing.** No fixed-step refactor is needed: state sync tolerates the host's
variable step. Network sends run on their own clock, independent of the frame.

## Decision 3: replication

Start with the legacy design and change only what cannot work:

| Traffic | Direction | Delivery | Content |
|---|---|---|---|
| Lobby, mission setup, load barriers, chat | both | reliable, ordered | new encoding of the `MCMSG_*` |
| Player orders | client to host | reliable, ordered | `TacticalOrder` chunk |
| Weapon hits, world events (mines, capture, artillery, destruction) | host to client | reliable, ordered | `WeaponHitChunk`, `WorldChunk` (new) |
| Weapon fire, radio | host to client | reliable, unordered | `WeaponFireChunk` |
| Mover and turret updates | host to client | unreliable, newest wins | `MoveChunk`, `StatusChunk` |
| Head and hand poses | via host | unreliable, newest wins | new, phase 7 |

Rough size, to confirm in phase 2: 96 movers times two 32-bit chunks plus an id is
about 1 kB per sweep, so 5 kB/s per client at 5 Hz, before weapon events and
transport overhead. Even eight players on a home uplink should fit, and a Quest on
Wi-Fi has plenty of room. The original's update frequencies are lost
(`initUpdateFrequencies` is empty), so these are tuned from scratch.

**Mover updates may need replacing.** Step-following is compact and animates well
but is untested. If it visibly stutters or warps at realistic loss and latency,
replace `MoveChunk` with quantised position, heading and speed, and a client
interpolation buffer about two updates behind the host (the now-standard
snapshot interpolation). Trying legacy first costs little, and the decision
is made in phase 2 with numbers.

**Modern additions, each cheap:**

- **Explicit wire format:** `code/netwire.*`, with no GL or SDL, fixed-width
  little-endian fields. Chunks' `unsigned long` becomes `uint32_t`: it is 64 bits
  on Linux and 32 on Windows, and buffers are sized from it.
- **Untrusted input:** every field is range-checked on receipt. Today a bad
  packet could end the game: `Fatal(0, " mech.update: newMoveChunk stepPos not on
  map! ")`, and roster indices become pointers in `TacticalOrder::unpack`. The
  host also checks that a player owns the movers in their order. The decoders get
  a fuzz test under ASan.
- **Handshake:** protocol version, plus a hash of the mission and object data and
  the mod set, so a mismatch is refused in the lobby rather than found in play.
- **Fog of war by team** is already computed on the host (`contactStatus[]`).
  Sending each client only what its team sees saves bandwidth and removes
  map hacks. It comes later, since legacy clients may assume they know every
  mover.

## Decision 4: transport

Behind a small new interface, `GameOS/gameos/gos_net.*` (listen, connect, send on
a channel, poll), so that the choice can be changed. The declared `gos_Net*` API is
shaped like DirectPlay and is not worth implementing.

| | GameNetworkingSockets | ENet |
|---|---|---|
| Reliable and unreliable channels | Yes | Yes |
| Encryption | AES-GCM-256, Curve25519 | Not built in (to confirm) |
| NAT traversal | Built-in ICE client; you supply signaling, STUN, TURN | Not built in (to confirm) |
| Lag and loss simulation | Built in | No |
| Dependencies | Protobuf, plus a crypto library (BoringSSL here, BCrypt on Windows) | None |
| Android arm64 | **Not in its docs or its CI**, but it builds and runs on a Quest 2 (see "Android spike") | Plain C, expected to be easy |
| State | v1.6.1, October 2026, after a gap from 2022 | Stable and small |
| Licence | BSD-3-Clause | MIT |
| Steam | Not needed; relay and signaling services are Steam-only | Not applicable |

**GNS is the recommendation, reversing the old study's ENet,** because remote play
without port forwarding is now a requirement, and hand-written hole punching and
encryption are what a modern library should spare us. **The risk is Android:**
Protobuf cross-compiled with NDK r28c, alongside the Windows `3rdparty` story and
the apt lists. Phase 1 opens with a time-boxed spike. If GNS does not build
cleanly on all targets, use ENet for LAN, and pair it with an ICE library for
the internet.

### Android spike (2026-11-04)

GNS v1.6.1 with its native ICE client, OpenSSL 3.0.15 and protobuf 21.12 (the last
version without Abseil) compile for Android arm64 with NDK r28c, API 29, with no
source changes to any of them, and carry messages between a Quest 2 and a PC.

**Run on a Quest 2 (Android 14), by a small test program started from `adb shell`.**
A client sends 100-byte messages at 30 Hz, one in six reliable, and a server echoes
them. A connection joins in about 20 ms. (GNS's docs say connections are encrypted by
default; this test did not check that.) Round trips include about
12 ms from GNS's send batching and the test's own loop, seen on loopback too.

| Path | Reliable | Unreliable | Round trip p50 / p95 / max |
|---|---|---|---|
| Quest alone, loopback, 5 s | 25 of 25, in order | 125 of 125 | 12 / 12 / 13 ms |
| PC (WSL) to Quest over home Wi-Fi, 20 s | 100 of 100, in order | 500 of 500 | 17 / 23 / 72 ms |

This does not cover: the app itself (the manifest has no `INTERNET` permission yet, and
a shell has different rights), a headset in use (it was asleep, and rendering and worn
power states may change Wi-Fi), a Quest as the client, two Quests, or a long run.

- **Size:** `libGameNetworkingSockets.so` is 7.0 MB stripped, built with `MinSizeRel`
  (2.5 MB gzipped), with OpenSSL and protobuf linked in. The APK stores native
  libraries uncompressed, so it grows by about the 7 MB. The `.proto` files
  ask for `optimize_for = SPEED`, so the full protobuf runtime is linked; moving them
  to the lite runtime might shrink this, but would mean patching the library.
- **It depends on** only `liblog`, `libm`, `libdl` and `libc`, and exports the flat C
  API (`GameNetworkingSockets_Init`, `CreateListenSocketIP`, `ConnectByIPAddress`).
- **The friction is OpenSSL on Windows, not GNS.** `scripts/android/build.sh` runs in
  Git Bash, whose Perl lacks modules OpenSSL's `Configure` needs (three were missing
  before the attempt stopped). The spike compiled OpenSSL in WSL instead, with Ubuntu's
  clang 21 against the NDK's headers. That is not a build to ship: a real one uses the
  NDK's own compiler, on Linux, or from Windows with a full Perl.
- **Other platforms:** Linux x64 built in WSL from the same protobuf source and the
  system OpenSSL. Not built: Windows MSVC (OpenSSL and protobuf, or `BCrypt`, into
  `3rdparty`) and arm64 Linux. Ubuntu 26.04 offers `libprotobuf-dev` 3.21.12; older
  distributions in the CI list need checking.

**GNS, built with BoringSSL.** A real OpenSSL for Android needs a complete Perl, which Git
Bash lacks, so the build uses BoringSSL on Linux and Quest and BCrypt on Windows, from pinned
sources and with no new packages (`cmake/mc2_gns.cmake`). GNS does not support BoringSSL as
shipped: `cmake/mc2_gns_boringssl_patch.cmake` makes three small changes, so a new GNS version
means checking that again. `Delivery::ReliableUnordered` is a lane of its own, since GNS's
reliable messages are ordered within a lane.

With all that, `net_ping` over the real backend between a PC (Linux x64) and the Quest over
home Wi-Fi, 20 s, sending with no batching: 100 of 100 reliable ordered, 100 of 100 reliable
unordered, 400 of 400 unreliable, none out of order, round trip p50 6 ms, p95 about 10 ms, with
a worst case of 120 ms on the reliable ordered messages and 85 ms on the unreliable ones.

**Size with this build:** `net_ping` for Quest, which is the library, its dependencies and
a small program, is 2.9 MB stripped, against the spike's 7.0 MB with OpenSSL. `libmain.so`
does not contain any of it until the game calls it, so that cost is still to come.

Still open: a BCrypt peer talking to a BoringSSL one (Windows to Quest) has not been run, and
the handshake depends on them agreeing; a sleeping headset and a shell are not the app. When
phase 2 links this into the game, `dist/licenses` also needs the notices of the code GNS bundles
(`src/external`: curve25519-donna, ed25519-donna, sha1-wpa, vjson).

Rejected: VPNs such as Tailscale as the remote answer (fine for PC players, but a
poor request for a headset), TCP (a lost packet delays every later update), and
Photon or other hosted services (per-user commercial dependency, for a sideloaded
open-source port).

## Game modes: head-to-head and co-op

Support both, on one mechanism: **a game is a set of commanders on teams, and a
commander is a human or the AI.** The network layer does not know the difference,
because all AI runs on the host and clients only see movers. The structure is
already there: `MC2Player` has `commanderID` and `team`, so several humans on one
team was part of the original.

| | Head-to-head | Co-op skirmish | Co-op campaign mission |
|---|---|---|---|
| Sides | Humans against humans | Humans against an AI commander | Humans against the mission's own forces |
| Map | Multiplayer maps | Multiplayer maps | Single-player missions |
| Win condition | `calcMissionStatus`, to rebuild | Same | Mission objectives |
| Original support | Built for this | Partial: an unbuilt add-AI screen | **None seen: new work** |

**What they share:** phases 0 to 2 (transport, wire format, replication, orders),
and the lobby. Choosing one now does not rule out the other.

**What co-op adds:**

- **AI needs its own brain.** Every mover in multiplayer is forced to `pbrain`
  (`mover.cpp`, `mission.cpp`), so an AI force would have no real behaviour. Keep
  the mission's brain for commanders that are not human.
- **Host CPU.** Enemy AI runs on the host. A Quest 2 took about 13 ms of game logic
  per frame on the `mc2_19` bench (a lance against 20 enemies), so a Quest host
  has little room for co-op. Measure it; a PC host may be the answer here.

**What campaign missions add on top:**

- **Objectives and endings.** `Team::home->objectives` and the player-force-dead
  checks in `Mission::update` sit behind `!MPlayer`. They need to run on the host
  for the human team and send their result, in place of `calcMissionStatus`.
- **Mission scripts.** The host runs them: `addMissionScriptMessageChunk` is there
  for telling clients. But ABL `ArtilleryStrike` calls `Fatal` in multiplayer, so
  any script that uses it ends the game. Each such guard needs a real answer.
- **Dividing the force.** The mission expects one player's lance. Each human needs
  a share, chosen in a pre-mission screen.
- **Limits.** `MAX_MULTIPLAYER_MOVERS` is 96 and the multiplayer path allocates
  less than single-player. Check them against the largest campaign missions.
- **Single-human assumptions.** `Team::home` and `Commander::home` have 351 uses.
  With two commanders on one team, each needs a choice of team or commander. This
  matters for head-to-head too, but less, since each player is their own team.

**Campaign progress (money, salvage, mech lab, saves between missions): out.** Co-op
plays one mission at a time. Carrying a shared campaign across missions is a much
larger feature, and can be its own proposal if wanted.

## Same room on Quest

Each level stands alone, and most of the value is in the first.

### Level 0: same network, separate views (no alignment)

Each player has their own view of the battlefield, as in any networked game. Other
players' mechs are simply in the world. This needs no shared coordinates, because
**the only shared space is the map**, and the host owns that.

- **Discovery:** UDP broadcast on the LAN, feeding the existing game browser. The
  same code finds a PC or Pi host, and needs no internet.
- **Manifest:** today's `AndroidManifest.xml` has **no `INTERNET` permission**.
  Add it, plus `ACCESS_WIFI_STATE`, and `CHANGE_WIFI_MULTICAST_STATE` with a
  `MulticastLock` so broadcasts are received. Check Android's coming
  `ACCESS_LOCAL_NETWORK` rule (API 37 targets) when raising the target.
- **Wi-Fi latency:** hold a low-latency `WifiLock`, since power saving can add
  latency spikes.
- **Fallbacks:** routers with client isolation block both discovery and play.
  Offer a host address on a controller-only keypad, and suggest a phone hotspot.
- **No typing:** the host and join flow must work with controllers alone, as
  saving does since the Quest playtest fixes.
- **Safety:** the Guardian does not show other people. Players should sit, or
  have separate spaces, until phase 7 shows where they are.

**Headset off is the hard case.** With the proximity sensor covered or off, the
session leaves `FOCUSED` and may stop drawing, and Android may suspend the app.

- A client that goes away: the host carries on, the network keeps a long timeout,
  and the client catches up on return (phase 6).
- **A host that goes away stops the game for everyone.** The game pauses for all
  with "host is away", and resumes. Advise a PC host where one is available.
  What the game does today when the session loses focus is untested for this,
  so it is the first thing to try on hardware.

### Level 1: one shared table (after VR phase 5)

Everyone sees the same battlefield on the same real table. It needs two things.

**A shared map view.** Today each player's scroll is their own. A shared table
needs one scroll and scale, kept by the host and sent to all, and any player may
grab it. A private view can be an option. Neither affects the sim.

**An alignment, by hand first:** each player touches two marked points on the
table with a controller. Gravity gives vertical, so two points fix position and
rotation. Store the result as a local spatial anchor (`XR_FB_spatial_entity`) so
it survives recentring.

- No cloud, no internet, no Meta account dependency, works when sideloaded.
- Published and forum reports for similar methods give a few centimetres of error
  (research paper: a few cm; Meta forum: 4 to 5 cm). At 1:285 a mech is about 4 cm,
  so the real table can be one mech out from the virtual one. **That is tolerable:**
  poses are sent in map coordinates, so players' virtual beams and hands point at
  the right mech whatever the error. The error shows only as a mismatch
  between someone's real and virtual position.

**Meta Shared Spatial Anchors: optional later, not the base.** They would make this
automatic, and Quest 2 is supported, but:

- anchors go through Meta's cloud, and colocation discovery (`XR_META_colocation_discovery`,
  Horizon OS v71+) requires internet and reaches Meta's servers, with Quest-only
  reach (no PC or Pi host),
- the device setting and permissions (`com.oculus.permission.IMPORT_EXPORT_IOT_MAP_DATA`,
  `USE_COLOCATION_DISCOVERY_API`) are extra setup,
- **whether a sideloaded, unregistered app may use them is unconfirmed.** The docs
  read do not say. Test on two headsets before committing.

### Presence (phase 7, both same room and remote)

Head and hands as simple shapes, sent in map coordinates at about 20 Hz: three poses
at 28 bytes is about 1.7 kB/s per player. A beam from another player's hand is the
"look at that" of a shared game, and it shows where people are in the room.

## Remote play

1. **Direct connect first,** by address and port: no infrastructure, enough to
   prove internet latency and loss.
2. **Room codes next.** A small rendezvous service for ICE signaling: short-lived
   codes, no accounts, no game data, no stored IPs. STUN from public servers.
3. **TURN relay** for the NATs that cannot punch through. GNS's docs say to expect
   to run your own. Traffic is low (see above), so a small VPS should cover it.

Someone has to run items 2 and 3 for players to use them (see open questions).
Peers see the host's IP address, as in any direct-connect game. Say so in the UI.

Voice: not built in at first. Remote players can use anything else, and same-room
players can just talk. Opus over GNS can come later.

## Affected areas

- **Platform:** `gos_net.*` (new), Android manifest and Wi-Fi locks, `gos_xr.*` for
  focus handling.
- **Game:** `multplyr.*` (most of the work), `netwire.*` (new), server-side camera
  independence, mission status, `mp*.cpp` and the main menu.
- **Engine:** chunk widths, `Terrain` active-block handling on the host.
- **Renderer:** none until presence and the shared table (both renderers then).
- **Build/CI:** new dependency (apt list in `ci.yml`, Windows `3rdparty`, Android,
  all `BUILD-*.md`), new two-process smoke tier.
- **Single-player: must not change.** `MPlayer` stays NULL, and
  `smoke.sh --deterministic` hashes pin that on every PR.

## Phases

| Phase | What | Exit criterion | Size |
|---|---|---|---|
| 0. Wake the branches | `isServer()` true with zero clients and no network: `-nethost` on a stock multiplayer map; fix crashes; host camera independence | Five missions clean under ASan + UBSan; single-player hashes unchanged | S to M |
| 1. Wire and transport | GNS spike on all targets, then `gos_net`, `netwire` with round-trip and fuzz tests | Two processes exchange both delivery types on all CI configurations; Android builds | M |
| 2. **Vertical slice** | `-nethost` / `-netjoin <ip>`, fixed mission and forces, no UI; orders up, mover and weapon state down | 1v1 plays to the end on CI under 100 ms and 2% loss; client matches host; movement approach decided; Quest client step measured | **L, least predictable** |
| 3. Head-to-head | Every order type, artillery, mines, capture, repair, recovery; `calcMissionStatus`; game modes, elimination first; results | Each mode plays to its result; two-process smoke | L |
| 4. Lobby and LAN: **first playable** | Enable the screens, LAN browser, data hash, controller-only flow, Android permissions | **Two Quest 2s in a room play a mission (level 0)**; mixed PC and Quest game | M |
| 4a. Co-op skirmish | AI commanders with their own brains, add-AI in the lobby | Two humans beat an AI force on a multiplayer map | S to M |
| 4b. Co-op campaign missions | Objectives on the host, script guards, dividing the force, mover limits | A chosen campaign mission plays through with two humans, then the rest one by one | L |
| 5. Internet | Direct connect, rendezvous and room codes, ICE, TURN | Two homes play without port forwarding | M |
| 6. Resilience | Reconnect with a catch-up message, headset off, host away, leaving, host drop | Each case ends well, tested with injected drops and on headsets | M |
| 7. Presence and table | Poses; shared map view; two-touch alignment (needs VR phase 5) | Two Quests share a table and point at the same mech (level 1) | M |
| 8. Optional | Dedicated server, team fog culling, voice, Meta anchors | Each on request | |

**Go/no-go points:** phase 0 prices the 2001 branches cheaply, in days, with no
transport work thrown away. Phase 2 decides whether to go on: the old plan
placed this after a lobby that proved little, and it is where the risk is.

**Size:** S is days, M is one to three weeks, L is several weeks, part-time with
agents helping. These are judgement, not measurement; phases 2 and 3 could
well grow. Altogether, months rather than weeks.

A PC host can later run under Xvfb as a dedicated server of sorts, since the smoke
runs do. A real one needs appearance state kept while drawing is stubbed
(animation feeds the sim), so it is not cheap.

### Phase 0 results

`-nethost <players>` runs the host's code with no network and no clients (PR 110).

- All ten stock multiplayer maps (`mc2_m01` to `mc2_m10`) and a damage-on battle
  (`bench/nethost_m08.fit`) ran 3000 frames clean under ASan + UBSan on x64
  desktop GL. CI runs `mc2_m09` and the battle on x64 GL, arm64 GLES and sanitized.
  `mc2_m02a` and `mc2_m02b` were not run.
- Few fixes were needed: the `calcMissionStatus` stub, draining the send queues
  (a full weapon fire queue is `Fatal`), a turret with a neutral parent, a
  negative shift, and movers per commander on six and eight player maps.
- A 100-second battle with 24 units queued 1,020 weapon fire, 923 weapon hit and
  59 critical hit chunks. Mover updates are not counted yet.

Not settled by phase 0:

- **Camera independence is not proven,** so that part of the exit criterion is
  open. The same battle from two camera positions parts at frame 282, at one mech
  going down. That fits cosmetic code sharing the RNG, harmless with one host.
- No run has had a turret fire, so the off-camera turret change is unproven.
- A withdrawing mech that stops short of the map edge stays until the mission ends.
- Quitting through the menu crashes in `MPStats::render` (no `ChatWindow`): phase 3.
- `Team::home` and `Commander::home` are not audited for host versus player.
- Host cost off camera: buildings update everywhere, and at night their lights can
  fill the world light table. Measure on a Quest in phase 2.

### Phase 2 results

`mc2 -mission <map> -nethost 2 -netlisten [port]` hosts and waits for the other player,
`mc2 -mission <map> -netjoin <address>[:port]` joins. No UI. `code/netsession.*` is the
join, the load barrier and the start, without the game, so tests run it on a loopback.
`scripts/net_smoke.sh` is the test: a host and a client on one machine, under Xvfb and
llvmpipe, fight `bench/net_m08.fit` (12 units a side, damage on) until one team is left.
Each side orders its own units, so the client's go by the network to the host.

Run on Linux x64 with GCC, desktop GL, on one machine, the lag and loss from GNS's own
simulator on what each end sends:

| Each way | Result | Weapon hits | Client orders | Armour differs | Position error |
|---|---|---|---|---|---|
| None | Played to a result, both agree | 791 at turn 2400 | 165 at turn 2400 | 0 at every check | 14 at the end |
| 100 ms, 2% loss | Played to a result, both agree | 920 | 358 | 0 at every check | 153 at worst, usually about 30 |
| 250 ms, 5% loss | Played to a result, both agree | 810 | 284 | 0 at every check | 166 at worst, usually under 70 |

Positions are world units, and a cell is about 43. The largest figure in any run, about
12,000 to 13,000, is the check that falls during the host's opening teleport. A check every
five seconds compares each live unit on the client with the host's.

- **Movement: keep the legacy step-following for now.** Its error under 250 ms and 5% loss
  stays within a few cells, inside the warp distance (300). This is from numbers only: nobody
  has seen whether it looks acceptable, so snapshot interpolation stays the fallback. The
  decision is not final until someone has watched it.
- **Critical hits go in the weapon hit stream.** As per-unit chunks they reached the client
  in another order from the hits, and what a critical hit destroys decides where the next hit
  lands, so armour drifted apart. With one ordered stream it does not.
- **Armour is compared only for live units.** A hit on a unit that is down takes another path
  once it has finished falling, which an animation decides on each machine. That difference is
  real and left alone, since a wreck's armour changes nothing.
- **Bad data no longer ends the game on the paths this uses.** Roster indices are bounded, and
  `TacticalOrder::validPacked` checks a client's order before `unpack`, which ends the game on a
  bad target. The host checks that a client owns the unit it orders. A client that sends what
  only a host may send is dropped, as is one whose reliable queue is full.
- **A found bug, not a network one:** `Gate` and `Turret` inherited a `getDamageLevel` that read
  their type as the wrong class, also in single player. Only the mouse-over check for rock clumps
  and a debug string use it on them, so play does not change.

### Phase 3, begun

World events go from the host to clients in order, as two words each in a `ChunkKind::World` batch
(`netwire::WorldEvent`): artillery, mines, terrain fire, a building captured, and kills and losses. A
repair is `Mover::refit` run again on the client from the same state. A client calls a strike by
asking the host, and the host applies it for that client's commander.

`NetEventsEvery` in a scenario stages what a short battle is not sure to do. In `bench/net_m08.fit` a
client saw artillery, mines, captures and repairs, armour never differed, and both ends finish with
the same kills, losses, buildings and mines (`[NETSCORE]`). The repairs and captures are staged by the
director, not ordered through a repair truck or a capturing mech, and no run has lit a tree. The
strike requests are not checked against resource points.

A client's hold position reaches the host, and the in-mission results table has real players, names
and ranks, the winners first and then by team score. Mission script messages and team scores are sent.
A `TACTICAL_ORDER_RECOVER` is refused: executing one is a `STOP`, in single player too, so a client
could have ended the host's game with it.

Not settled by phase 2, against its exit criterion:

- **The Quest client step is not measured,** and the game has not run a network game on a Quest.
- **Only on one machine.** Real Wi-Fi, two machines, x64 with arm64 and a Windows peer are not run.
- **Sound, effects and movement on the client have not been looked at.** The check covers armour,
  status and position.
- Not sent yet, for phase 3: gates and turrets changing state beyond the hits on them, a captured
  vehicle, reinforcements, turret targets, pilot kill stats. Recovery is dead code in the game.
- The host checks one win condition, the last team standing, and a mission script's result is not
  sent. Ending goes straight to quitting: there is no results screen or lobby to return to.
- A client that leaves is not replaced, and a host that leaves ends the game for the client.
- The data check in `Hello` is only the mission's name. The roster hash at the load barrier catches
  other units, but not other weapon or terrain data.
- The exit on `-autoexit` crashed in the heap before the network closed in `ShutdownNetworking`, and
  has not since, sanitized or not. Which object it was was not traced.

## Verification

- **Unit (every commit, all four configurations):** round trip per message and
  chunk, malformed input, fixed sizes, ASan + UBSan fuzz run.
- **Two-process smoke (CI):** host and client on localhost under llvmpipe with
  `-autoexit`. The host stays fixed-step, so a run is repeatable on one machine.
- **Network conditions (CI):** GNS's simulator at, say, 0, 100 and 250 ms with
  0, 2 and 5% loss.
- **Convergence, in place of a desync check:** at the end of a run, and regularly
  in debug builds, host and client log per-mover alive state and armour, and
  position within a tolerance. CI compares the logs.
- **Replay (client tests without a host):** record the host's messages to a file and
  feed them to a client, so client bugs reproduce deterministically.
- **Single-player:** the `-deterministic` state hash is unchanged on every PR.
- **By hand, added to `HARDWARE-TESTS.md`:** two Quest 2s on home Wi-Fi and on a
  phone hotspot; headset off on client and on host; mixed PC, Pi and Quest; two
  homes; a long session for frame time on a Quest host. Not run by agents.

## Risks

| Risk | Mitigation |
|---|---|
| 1,000 branches, untested for 25 years and never on 64-bit | Phase 0 finds out early and cheaply |
| Lost behaviour (update rates, barriers, win logic) | Rebuild from call sites and play; no need to match the original exactly |
| GNS on Android | Spike first, ENet fallback, interface in between |
| Quest host misses frame budget | Measure; recommend a PC host; game step on its own thread is already a VR to-do |
| Host's headset comes off | Pause for all, and recommend a PC host for longer games |
| Single-player regressions | `MPlayer` NULL path untouched, pinned by hash in CI |
| Merges from `alariq/mc2` | Most changes fall in already-stubbed `multplyr.cpp` and new files |

## Open questions

For you:

1. **Does co-op need campaign missions,** or is skirmish against AI on multiplayer
   maps enough at first? The plan does skirmish first; both are in it.
2. **Will you run a small server** for room codes and TURN? If not, remote play stays
   direct connect, with port forwarding on the host side.
3. **Is a shared physical table essential at first,** or are separate views in one
   room enough for the first playable game? This plan assumes they are.
4. **How many players** matter on Quest? Eight is the original limit (`MAX_MC_PLAYERS`,
   96 movers), but fewer may make Wi-Fi and a Quest host far easier.

For the work:

- Does GNS still behave on a Quest inside the app, while rendering and worn?
- How much cheaper is a client's step than single-player's on a Quest?
- Is legacy step-following acceptable at 100 to 250 ms with loss?
- What does the game do today when the OpenXR session loses focus, and can a
  background thread keep the network going while the app is paused?
- Can a sideloaded app use Shared Spatial Anchors, and what does it take?

## Sources

- GameNetworkingSockets: [repository](https://github.com/ValveSoftware/GameNetworkingSockets),
  [BUILDING.md](https://raw.githubusercontent.com/ValveSoftware/GameNetworkingSockets/master/BUILDING.md),
  [README_P2P.md](https://raw.githubusercontent.com/ValveSoftware/GameNetworkingSockets/master/README_P2P.md),
  [Phoronix on v1.5](https://phoronix.com/news/GameNetworkingSockets-1.5)
- ENet: [repository](https://github.com/lsalzman/enet)
- Meta: [colocation discovery](https://developers.meta.com/horizon/documentation/native/android/openxr-colocation-discovery-api-ref/),
  [sharing anchors, native](https://developers.meta.com/horizon/documentation/native/android/openxr-ssa-share-content),
  [space sharing, native](https://developers.meta.com/horizon/documentation/native/android/openxr-space-sharing-api-ref/),
  [group sharing](https://developers.meta.com/vr/blog/colocation-discovery-group-sharing-shared-spatial-anchors-mixed-reality/),
  [forum thread on alignment offsets](https://communityforums.atmeta.com/discussions/Questions_Discussions/calibration-of-multiple-vr-headset/1283114)
- Android: [NsdManager and local network access](https://developer.android.com/reference/android/net/nsd/NsdManager.html)
- Alignment research: [Co-Experiencing Virtual Spaces](https://zenodo.org/records/4399217/files/Co-ExperiencingVirtualSpaces_IASpace.pdf),
  [A Quest for Co-Located Mixed Reality](https://typeset.io/pdf/a-quest-for-co-located-mixed-reality-aligning-and-assessing-2jhu0dcrkn.pdf)
- Lockstep across platforms: [Frontier, GDC 2024](https://media.gdcvault.com/gdc2024/Slides/GDC+slide+presentations/Pollard_Bradley_CrossPlatformDeterminism+2024-03-26+09.34.19.pdf),
  [Forrest Smith on RTS desyncs](https://www.forrestthewoods.com/blog/synchronous_rts_engines_and_a_tale_of_desyncs/)
