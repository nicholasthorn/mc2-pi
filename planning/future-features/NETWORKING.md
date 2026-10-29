# Multiplayer networking

Status: feasibility study, nothing built. Written 2026-10-29 from a read of
`pi-port`.

## Problem

MC2 shipped with LAN and internet multiplayer over DirectPlay 8. This port has
none: the main menu's Multiplayer button is disabled and the multiplayer
screens crash on the stubbed player data if reached.

## What is left in the code

**Removed (before this port existed):** Microsoft's source release already had
the multiplayer layer stubbed out; `code/multplyr.cpp` is the same in the
repository's first commit. Specifically:

- About 80 `MultiPlayer` methods are empty or return a constant: session scan,
  host, join, leave, update, `updateClients`, player info, rosters, the
  `waitTill*` load barriers, message sending and handling.
- `WorldChunk::pack` / `unpack` are empty.
- The GameOS network API (`gos_NetSendMessage`, `gos_NetGetMessage`,
  `gos_NetInformation`, ... in `GameOS/include/gameos.hpp`, `network.hpp`) is
  declared but not implemented. `dplay8.h` is only included on non-Linux builds.

**Kept:**

- `code/multplyr.h` (about 1,850 lines): about 40 message types
  (`MCMSG_CHAT`, `MCMSG_PLAYER_ORDER`, `MCMSG_MOVER_UPDATE`,
  `MCMSG_WEAPONHIT_UPDATE`, `MCMSG_WORLD_UPDATE`, `MCMSG_START_MISSION`, ...)
  with their message classes, session/player structs, `MAX_MC_PLAYERS` = 8,
  and accessors such as `isServer()`.
- **Game-side serialisation:** `MoveChunk` and `StatusChunk` (`mover.cpp`),
  `WeaponFireChunk` and `WeaponHitChunk` (`gameobj.cpp`), `ArtilleryChunk`
  (`artlry.cpp`), `TacticalOrder::pack/unpack` (`tacordr.cpp`).
- **Game-side hooks:** about 1,000 `MPlayer` references across `code/`
  (mission, logistics, mech, ground vehicle, mover, controls, buildings,
  artillery), including about 110 server/client branches and chunk-queueing
  calls (`addWeaponHitChunk`, `addWorldChunk`, ...).
- **Screens:** host game, game browser, direct TCP/IP, parameters, map load,
  setup, add AI player, stats (`code/mp*.cpp`).

**Model (inferred from message names and call sites, to be confirmed):**
host-authoritative state sync. Clients send orders; the server runs the
simulation and sends mover updates (run-length packed move data), weapon
fire/hit, world events (mines, artillery, building capture, scores) and mission
start/end. It is not lockstep, so the simulation does not need to be
deterministic.

## What it would take

1. **Wire format.** The message classes are raw structs with `long` fields
   (about 165 in `multplyr.h`) and flexible arrays. `long` is 32-bit on Windows
   and 64-bit on Linux and the Pi, so sending structs as-is would make Windows
   and Linux players incompatible. Write explicit serialisation with
   fixed-width little-endian fields. Ideal for TDD: round-trip unit tests per
   message.
2. **Transport.** Implement the GameOS network API (or a slimmer replacement)
   over **ENet**: small, portable C, works on Linux, Windows, Pi and Android,
   with reliable ordered and unreliable channels that map onto
   `gosNetFlag_guaranteed`. Alternatives: Valve's GameNetworkingSockets
   (heavier), SDL_net (TCP/UDP only, more to build).
3. **`MultiPlayer` reimplementation:** the bulk of the work. Host, join and
   direct IP; player roster and colours; message dispatch per `MCMSG_*`;
   the update loops and frequencies (`initUpdateFrequencies`); the load and
   start barriers; chat; end of mission and stats. No original implementation
   survives, so the behaviour has to be rebuilt from the header, the call
   sites and how the game behaves.
4. **`WorldChunk`** packing: a new design, since the original layout is gone.
5. **Game-side audit.** None of the ~1,000 multiplayer branches has run since
   2001 or on 64-bit. Expect bugs; the known NULL dereferences of
   `getPlayerInfo()` in the MP screens come first.
6. **Discovery.** LAN: UDP broadcast for the game browser. Internet: direct IP
   with port forwarding, or a VPN such as Tailscale or ZeroTier. NAT traversal
   and a lobby server are out of scope.

## Phases

| Phase | What | Exit criterion |
|---|---|---|
| 0. Serialisation | Fixed-width encode/decode for every `MCMSG_*` and chunk, unit-tested | Round-trip tests pass on all four CI configurations |
| 1. Transport | ENet behind the GameOS net API; loopback test | Two processes exchange guaranteed and unreliable packets in CI |
| 2. Lobby slice | Direct-IP host/join, player list, chat | Two instances on one machine see each other and chat |
| 3. Mission start | Mission/settings exchange, load barriers, start | Both clients load the same mission and start together |
| 4. In-mission sync | Orders up; mover, weapon, world updates down | A short 1v1 mission plays to the end without desync |
| 5. Wrap-up | End mission, stats, leaving, host dropping | Clean exits in every case |
| 6. LAN discovery and polish | Broadcast browser, AI players, up to 8 players | Mixed Windows / Linux / Pi game works |

Phase 2 is the go/no-go point: it proves the transport, the screens and the
wire format with little game risk.

## Verification

- Unit tests for every message format (TDD).
- CI: two game processes on localhost under llvmpipe, using `-autoexit`, a
  scripted host/join (needs command-line options such as `-host` / `-join
  <ip>`) and log checks, like the smoke runs.
- Manual: Windows host with a Linux or Pi client, over LAN.

## Size and risk

Large: weeks to months of part-time work, with phase 4 the biggest and least
predictable part. Main risks: rebuilding behaviour nobody has the original of
(update rates, packing of move data, barrier semantics), latency handling
(the original tuned for 2001 LANs and modems), and bugs in untested multiplayer
branches of the game code.

## Recommendation

Only if multiplayer matters to you. If so, do phases 0 to 2 as a contained
experiment before committing to the rest.

## Open questions

- Does any other MC2 source fork have a working transport to learn from? (The
  remaster has none.)
- Should the host also be able to run headless (a dedicated server on the Pi)?
- Keep the original 8-player limit and team rules?
