# Open-world mercenary mode: star map, contracts, shops, eras

Status: study and proposal, nothing built. Written 2026-11-04 from a read of
`pi-port` (`ddd7e5e1`) and the `mc2srcdata` files. No code was changed and no
experiment was run: every limit below comes from reading code, and every size is
a judgement to be tested by the spikes in phase 0.

## The short version

Yes, this is possible, and most of it does not touch the engine's hard parts. The
idea is **a new layer above the missions, not a rewrite of them**:

```
Star map  ->  Planet  ->  Town / contract board  ->  Zone (a mission map)  ->  back
 jump, era     faction      shops, hire, repair       MC2 as it is today
```

Three findings shape everything:

1. **The game is already most of a mercenary RPG.** C-Bills, a mech bay, a mech
   lab, salvage, pilots who level up, captured buildings, repair bays and bought
   support all exist. What is missing is the freedom to choose where to go next,
   and consequences that last.
2. **Going to a new map is cheap; seamless streaming is not.** Destroying one
   mission and starting another in the same process works today. Streaming would
   mean rebuilding pathfinding, object identity and the save format.
3. **A map 4x the stock area looks reachable; 16x does not.** The engine's IDs
   were laid out for 240 vertices a side, and the largest stock maps are 120.

The recommendation: build **zones with loading screens**, the way Pokemon Legends:
Arceus and Monster Hunter World do, which still feel open. Start with a small
slice: a star map, a contract board that reuses stock maps, and damage that
carries between missions. It can be played and judged long before any new map
exists.

## Problem

MC2 is a corridor: 24 missions in 13 groups, in order, on one planet. The only
branching is a choice of missions inside a group, and hidden missions that unhide
(`code/logisticsmissioninfo.cpp:837-861`). Once it is finished there is little
reason to return. Players who like Pokemon, or open-world RPGs, want a world to
wander, a roster to collect and grow, and goals they pick themselves.

The BattleTech setting suits this well: a mapped galaxy, factions with their own
politics, mercenary contracts as the everyday job, and centuries of history.

## Scope

**In:** a new game mode beside the stock campaign; a star map; a contract
generator; lasting damage, reputation and rivals; shops and hiring; bigger maps up
to 240 vertices; selectable eras.

**Out:**

- Changing the stock campaign. It stays playable and its saves keep loading.
- Seamless planet-sized worlds, and real streaming (see "Why not streaming").
- Procedural terrain. Reusing and stitching maps gives more for far less.
- Swapping mechs in the middle of a mission.
- Online play or trading: networking does not exist
  (`planning/future-features/NETWORKING.md`).
- Anything from Pokemon beyond design ideas: no names, creatures or art.

## What Pokemon and modern games offer

The useful question is what makes these games compelling, then what MC2 already
has that could carry it.

### From Pokemon

| Pokemon | MC2 version | Base already in the game |
|---|---|---|
| Choosing a starter | Pick one of three starting lances, each with a pilot and a personality | Purchase files, pilot data |
| Catching | **Salvage is catching.** Disable a mech cleanly to claim it; a wrecked one is worth less | Salvage screen, recovery |
| The Pokedex | Mech and pilot collection log: seen, fought, owned | Mechlopedia screens |
| Levels and moves | Pilot skills, plus a choice of specialty at each rank | Skills and specialty skills |
| Type advantages | Give weapon families clear strengths against mech types, and let scouted enemies show their weakness | Component data; rules and balance are new |
| Gyms and badges | Each planet has a faction champion lance. Beating it earns a **licence** that opens the next stretch of the star map | Mission scripts, new gating |
| Elite Four | The Solaris VII arena ladder: small, fixed fights against named pilots | Small maps, scripted missions |
| The rival | A rival mercenary commander who turns up across the campaign, and whose lance grows with yours | Fixed pilots; needs the rival system below |
| Routes with wild encounters | Travelling across a zone sets off patrols and ambushes, tougher further from town | Dormant units, trigger areas |
| Towns and the Pokemon Center | Town with a repair bay, mech shop, bar for hiring and a contract board | Logistics screens |
| Legendaries | A rumoured lost Star League cache or unique mech, tracked through clues across worlds | Script flags, unique variants |

### From modern games

| Idea | From | What it would be here |
|---|---|---|
| Large separate zones around a base | Pokemon Legends: Arceus, Monster Hunter World | The core structure: town, then zones, then back |
| Contract negotiation, repair bills, lasting damage | BattleTech (2018) | Choose money against salvage rights; a damaged mech costs money and time. PCGamesN's review singled out this management layer |
| **Named enemies who remember you** | Middle-earth: Shadow of Mordor (Nemesis) | **The idea most worth stealing.** An enemy commander who escapes returns promoted, in better mechs, with a taunt about the last fight. It gives generated contracts a story, and needs only a small save table |
| A branching route under pressure | FTL, Slay the Spire | Jump routes with visible risks and rewards; a faction war front moves as you delay |
| Conflict zones that shift | MechWarrior 5: Mercenaries | Fronts on the star map, which change the contracts on offer |
| Faction standing that closes doors | Mount and Blade, Fallout: New Vegas | Working for Steiner raises prices on Davion worlds and later brings hit squads |
| "See it, go there" | Breath of the Wild, Elden Ring | Landmarks visible on a zone map; capturing a sensor tower reveals nearby sites and optional fights |
| Rest points with a price | Elden Ring sites of grace, Darkest Dungeon | A field repair camp saves and repairs, but brings the patrols back |
| Permanent loss with legacy | XCOM, Battle Brothers | Pilots can die; retired veterans train recruits |
| A base that visibly grows | XCOM, Darkest Dungeon | The dropship gains bays: more mechs carried, faster repairs, a bigger bar for better pilots |

Things to avoid: a map covered in identical icons, and generated missions with
nothing to tell them apart. That is why rivals and factions come before scale here.

## What is feasible: the code as it stands

### The existing pieces

| Feature | Exists today | Change by data alone | Needs code |
|---|---|---|---|
| Campaign flow | Groups, mandatory and hidden missions | New campaigns, order, flags | World graph, outcome branching |
| Choosing a campaign | New Campaign dialog lists `campaign*.fit` (`code/logisticsdialog.cpp:566-599`) | Add a file | Sandbox mode flag |
| Unlocks | Purchase files per mission (`code/logisticsdata.cpp:1582-1701`) | Lists | Per-shop stock, scripted unlocks |
| Screens | Mech bay, purchase, lab, pilots: FIT layouts using a singleton | Position, art | New widgets; fixed control indexes |
| Pilots | Skills, specialties, medals | Roster, stats | Hiring, wages, medal rules |
| Economy | Lasting C-Bills, per-mission resource points | Amounts, rewards | Repair bills, wages |
| Damage | **None saved:** survivors return fresh (`code/logisticsmech.cpp:50-64`) | | Per-mech damage in the save |
| Unit spawning | `objectcreate` wakes a pre-placed unit; `addmovertoplayer` transfers one; infantry spawn | Dormant units placed in the FIT | General spawning, if ever needed |
| Objectives | Conditions and actions in the FIT | Everything in the FIT | Script control; named outcomes |
| Carry-over | C-Bills and bonus files; `globalMissionValues` (50 floats, not saved, `code/mission.h:393`) | | Campaign globals in the save |
| Buildings | Capture, repair bay refit, resource points | Flags, values | Click menus, shops |
| Support | Repair truck, scout, minelayer, guard tower, recovery, airstrike, sensor probe | On or off per mission | Costs and names are hard-coded (`code/controlgui.cpp:84-112`) |

Content to work with: 24 campaign missions and 46 mission packets including extras
(most 120 vertices a side), 24 chassis, 52 pilots, 420 building and object rows in
`art/buildings.csv`, and four map sizes. Mods add far more for players who install
them: see "Mods as content".

### Bigger maps

A stock map is at most 120 vertices a side, or 15360 world units. The next step,
**240 vertices (2x side, 4x area)**, is where the design points:
`MAX_MAP_CELL_WIDTH` is 720 cells (`mclib/terrain.h:58`) and the ID ranges fit
that. Rendering only handles a 30-60 vertex window around the camera, so draw
geometry does not grow with the map, and textures unused for 2 seconds already
leave VRAM (`mclib/txmmgr.cpp:1766-1783`).

What has to change first:

1. **Allow the size.** 240 is not on the list in `mclib/terrain.cpp:299-305`.
2. **`MAX_GLOBAL_PATH` is 50.** `GlobalMap::calcPath` now treats a longer route as
   no route, so a bigger map cannot overrun the fixed `globalPath` array
   (`code/warrior.h:454`), but its units would refuse long moves. Raising it
   changes the saved `MechWarrior` data (`code/warrior.h:495,745`).
3. **Collision grid.** It is 24x24 at radius 200, covering 4800 units and clamping
   anything outside to the edge (`code/collsn.cpp:258-280,417-420`).
4. **Bounds checks** on the special building and control tables (`code/objmgr.h`).
5. **Pilot history.** `MAX_MISSIONS` is 50 and it indexes a fixed array by mission
   ID (`code/logisticspilot.h:21`, `code/logisticspilot.cpp:265-268`), so a big
   world cannot give each contract its own ID: an ID past the end is logged and not
   recorded. Medals are hard-coded to stock mission numbers.
6. **A map builder.** This is the real blocker. The move data is precomputed and
   nothing in this repo calls `MOVE_buildData`. `mc2-opengl-remastered/editor` has
   a Windows mission editor that reaches it, but whether it can produce this format
   has not been checked.

Costs: about 38 MB of colormap in RAM at 240 vertices by inference, 2 MB per path
table. The real cost should be objects and movers, not terrain. mc2_14 has 4668
objects, so a dense 4x map may reach 18000, and some updates run for every object.
That is unmeasured, and the Pi is the limit. **Sparse wins:** a wide zone with
towns and empty country is what open worlds look like anyway.

**A cheap source of big maps: stitch stock ones.** A data tool could join four
120-vertex maps into one 240-vertex zone, with a cliff or river along the seams,
then rebuild the move data. It reuses art and needs no editor, but it still needs
the builder from point 6.

### Why not streaming

Hardest first:

1. **Pathfinding** is precomputed for the whole map, and `calcPath` searches all
   of it.
2. **Object identity:** handles are contiguous, part IDs are absolute, and lists
   are sized at load. Scripts and saves depend on those IDs.
3. **Simulation:** every mover always runs. Streaming means deciding what happens
   to units nobody can see, and storing them.
4. **I/O** is synchronous, and mid-mission reads already spike 300-500 ms on the Pi
   (`planning/future-features/PI-RENDER-PERF.md:26-27`).
5. **Saves** use fixed records and single-instance globals.

Terrain and textures, which seem the obvious problem, are the easiest part. So
streaming is close to the rewrite this proposal avoids. Zones give most of the
feel. If 240-vertex zones ever feel small, revisit it then.

### Shops on the map

Clicking a building today gives orders, not menus, and no logistics screen can open
during a mission: entry assumes the mission is over (`code/mechcmd2.cpp:2202`,
`code/missionbegin.cpp:178-215`). So there are three levels, cheapest first:

| Level | How it plays | What it needs |
|---|---|---|
| A. Town screen | A painted town between missions with hot spots: mech shop, bar, repair, contracts, like BattleTech's dropship screens | A new logistics screen; existing screens reused |
| B. Town zone with a mech walking in | Your lance walks around a real town. Entering a building ends the zone and opens the shop | Named mission outcomes; slow if each visit reloads the map |
| C. Overlay | Mission pauses and the shop opens over it | The options screen pauses the mission in a similar way (`code/mechcmd2.cpp:2159-2233`), but this reading did not prove the 3D previews and resolution switch can coexist |

Recommendation: start with A. Then add a small **in-mission** pause dialog for what
needs no mech swap: hire a pilot, buy a repair, take a rumour. Keep the full mech
lab in town. Capturing a building already exists as a verb, so "capture this repair
bay, then use it" is close to free.

### Contracts from existing maps

Units, pilots and objectives live in plain mission FIT sections (`[PartN]`,
`[WarriorN]`, `[Team0ObjectiveN]`), separate from the terrain in the `.pak`. So a
generator can write a new FIT for a stock map, with a template (assault, defend,
extract, assassinate), enemies chosen by faction, era and difficulty, and a generic
mission script. That is BattleTech (2018)'s approach: one reviewer noted that
generating missions across a wide set of maps reduced the sense of repetition.

Stock missions are hand-tuned and contain story. The generator needs each map's
safe spawn points and landing zones, recorded once per map in a small sidecar file.
It also has to fit two rules: generated files go only in the user's config or save
directory, and mission loading has to be able to read them from there (not yet
checked).

### Mods as content

Fan mods could make this mode much bigger, and this port already runs the main one.
MechCommander Omnitech 6.28 defines 144 mechs, 82 vehicles, 166 buildings and 19
turrets, against 24 stock chassis (`planning/future-features/MOD-COMPAT.md`). Its
Clan Wars campaign (28 missions) and nine campaign packs (5 to 24 missions each)
pass a headless smoke run here, which shows they load and run, not that they play
well. MC2X is another source of content, but it has not been made to load on this
port and we found no published source for it.

What that gives this mode:

- **A real collection.** A collection log is thin at 24 mechs and much more
  rewarding at well over 100. Shops can then differ by world and rarity.
- **Eras become possible.** The chassis shortage in early eras may mostly go away
  once mods are installed, subject to the same lore pass.
- **Ready-made worlds.** Each installed campaign could be a star system with its
  own maps and story, and its maps could host generated contracts too.
- **More variety in enemies:** vehicles, turrets and buildings for faction contract
  tables.

What it asks of the design:

- **Never required, never bundled.** Omnitech's licence grants no redistribution
  (`planning/future-features/EASY-INSTALL.md`), so stock alone must be a complete
  game. Players install mods themselves, unless the mod's team gives permission.
- **Detect, do not assume.** Star map, shops and contract tables list only what is
  installed. A save that refers to a missing mech needs a defined behaviour; it must
  not `STOP`.
- **Our own tables, about their content.** Era dates and rarity for a mod's mechs
  live in our own data files, keyed by mech name, so nothing of the mod is copied.
  A new mod gets a new table, and a mech with no table entry appears in no era until
  someone adds it.
- **Mods that clash.** The two Carver V packs replace the stock-named missions, so
  each needs its own folder. They cannot be two worlds in one install without
  isolated mod folders, and there is no `-mod` option yet.
- **Balance.** Modded mechs were tuned for their own campaigns, so prices and contract
  difficulty need to come from each mech's stats and not by hand.

### Saves

The save is a FIT file, and adding optional keys looks safe, since missing keys are
already handled (`code/logisticspilot.cpp:213-215`). **Never bump the version:**
the check is an exact match on 10004 (`code/logisticsdata.cpp:41,1156`) and
bumping it would orphan every save. World state belongs in new keys only used by
the new mode. In-mission `.ims` saves are positional and strict, so anything added
there needs care.

## Eras

The setting's history gives each era a different feel. The MC2 story, on Carver V
in the Chaos March as the Federated Commonwealth splits, belongs to the FedCom
Civil War, so that is home. Era dates below follow the Sarna and Catalyst lists.

| Era | Years | Feel | Content today |
|---|---|---|---|
| Star League | before 2781 | Golden-age tech, the Amaris coup | Needs new art and factions; last |
| Late Succession Wars | around 3025 | Scarce parts, salvage is life, every mech is an heirloom | Good on mechanics; few suitable chassis |
| Clan Invasion | 3050-3061 | Terrifying new enemies, an underdog story | **Strong fit:** Clan and Inner Sphere chassis both in stock |
| FedCom Civil War | 3062-3067 | Friends turned enemies, the stock campaign's setting | **Home:** all content fits |
| Jihad | 3068-3080 | Grim, everything falling apart | Mostly a story and map change |
| Dark Age | 3081 onward | New order, unfamiliar powers | Needs new units; last |

An era is mostly **data**: a star map snapshot (who owns what), allowed factions,
tech and mech lists with dates, prices by scarcity, a contract table and story
hooks. It is a filter over shared systems, so eras are cheap once the sandbox
exists.

Gaps:

- **The data has no tech dates or tech base.** `objects/compbas.csv` and the
  logistics code have no Clan or date field, so a table is needed, with a lore pass.
- **The 24 chassis are thin for early eras.** From memory, seven or so (Atlas,
  Hunchback, Catapult, JagerMech, Highlander, Zeus, Wolfhound) suit 3025, and the
  Mad Cat, Shadow Cat, Uller, Vulture and Blood Asp are Clan. That needs checking.
  Installed mods may fill much of this (see "Mods as content"), but stock alone has
  to work, so older-looking variants of existing models can stretch the stock list.
  `data_tools` can import new models, but modelling them is new art.

Two ways to play eras, and they combine:

- **Separate starts:** New Game asks which era. Simple, and the place to begin.
- **Dynasty (later):** the company lives through history. At the end of an era the
  game jumps ahead: veterans retire and train heirs, an old mech is handed down, and
  a rival's successor holds the grudge. It fits the setting's long history, and few
  games do it.

Writing has to be our own: stories and characters written for this mode, with no
text from novels, sourcebooks or wikis.

## Approach: phases

Each phase can ship and be judged alone. Sizes: S is a few PRs, M is a feature of
weeks, L is a large one. They are rough and depend on the spikes.

| # | Phase | Delivers | Engine risk | Size |
|---|---|---|---|---|
| 0 | Spikes | Numbers and yes/no answers below | None | S |
| 1 | **Star map and contracts** | First playable: jump between worlds, take generated jobs on stock maps | Low | M |
| 2 | **Consequences** | Lasting damage, repair bills, wages, faction standing, the Nemesis-style rival | Low | M |
| 3 | Town and shops | Town screens, shop stock per world, hiring, licences, arena ladder | Low | M |
| 4 | Authored story | Hand-written arcs on world graphs; branching on named outcomes | Medium | M |
| 5 | Eras | Era data, tech table, start selection | Low | M, plus writing |
| 6 | Big zones | 240 vertex support, map builder, stitched maps, zone travel | **High** | L |
| 7 | In-world extras | Walk-in buildings, pause dialog, patrols, moving fronts, dynasty | Medium | M each |

### Phase 0: spikes

1. **Load time.** It is unmeasured (`planning/QUALITY-ROADMAP.md:217`). Time mission
   to mission on desktop and on the Pi. This decides how often the player can bear a
   zone change, and so much of the design.
2. **Fix `MAX_GLOBAL_PATH` and the pilot off-by-one**, whatever else is decided.
3. **Map builder.** Can `MOVE_buildData` run as a command-line tool in `data_tools/`,
   and does the remaster's editor save a format this tree loads?
4. **One stitched 240 map**, loaded and played with stock AI. Measure RAM and frame
   time on llvmpipe, then the Pi.
5. **Generated FIT.** Hand-write one stock map's alternate FIT, load it from the
   config directory, and see what breaks.

### Phase 1: the first playable slice

The smallest thing that answers "is this fun?":

- Star map screen with about three worlds, each using stock maps of its look, and
  each with a faction.
- Contract board with three generated contracts and two templates.
- Reuses the mech bay, lab, salvage and pilot screens, unchanged.
- Entry through the existing New Campaign dialog, using a `campaign*.fit` with a
  sandbox flag, so no new main menu art or button is needed.
- New code in new GL-free files (`world graph`, `contract generator`, `standing`),
  which are all unit-testable by the rules in `.claude/rules/testing-conventions.md`.

Data files, illustrative only:

```
[Planet3]
st Name = "Carver V"
f MapX = 412.0
f MapY = 88.0
st Faction = "Liao"
st Era = "3062"
st Maps = "mc2_03,mc2_11,mc2_17"
st ShopFile = "shop_carver"
l JumpCost = 15000
l RequiredLicence = 1
```

In VR, the star map could be a holotable, which fits the tabletop direction in
`planning/future-features/VR-TABLETOP.md`.

## Affected areas

- **Game (`code/`):** nearly all of it: new screens, world state, contracts, save
  keys. Narrow fixes to pilots and mission outcomes.
- **Engine (`mclib/`):** nothing until phase 6, except the path bound fix. Then map
  size, collision, object tables.
- **Renderer (GL and GLES):** no changes expected before phase 6. New screens go
  through existing UI code. Bigger maps need checking on both, and new star map art
  has to obey the shader and texture rules.
- **Platform (`GameOS/`):** none expected. Generated files use the existing config
  directory.
- **Data and tools:** new FIT and CSV files; the map builder and stitcher in
  `data_tools/`.
- **Build and CI:** new `.cpp` files on the strict warnings list; a smoke mission
  that plays a generated contract.

## Verification

- Unit tests (doctest) for the world graph, generator, economy, standing and save
  keys: same seed gives the same contracts, and old saves still load.
- Existing five-mission smoke stays green: the stock campaign must not change.
- New smoke: start sandbox, take a generated contract, finish, confirm the save.
  This depends on the UI smoke work in `planning/future-features/UI-SMOKE.md`.
- For big maps: ASan + UBSan on a long route across a 240 map, which also shows
  whether units refuse moves past `MAX_GLOBAL_PATH`; `scripts/bench.sh` on the Pi
  before anything ships, on request.
- "Is it fun" cannot be automated. Play the phase 1 slice before committing to
  later phases.

## Risks

- **Content, not code, is the real limit.** Writing, map tuning and balance are what
  make this good. A generator with no story is a chore; hence rivals early, and
  authored arcs in phase 4.
- **Balance.** The stock economy assumes a fixed path through 24 missions. Open play
  will break it, and that needs repeated tuning.
- **Pi.** Long loads and dense maps hurt most here. The mode has to stay playable on
  the project's first platform, which favours short zones and sparse maps.
- **Scope creep.** This could absorb the project. The phase 1 gate exists to stop it.
- **Rights.** BattleTech's names and universe belong to their holders, and the game
  data is under the Microsoft EULA's non-commercial terms. This stays a free fan
  project with a clear disclaimer, Omnitech stays out of the packages, and the
  Pokemon link is only inspiration. No formal fan-work policy turned up in a quick
  search, so this is a working assumption and not legal advice.

## What the first slice built and found

Built (2026-11-04): worlds in `merc/data/campaign/merc.fit`, pure rules in
`code/merccontract.cpp`, game glue in `code/mercmode.cpp` and small hooks in the
logistics code. It departs from the plan above in a few ways:

- **The star map is the mission selection screen**, with an operation file per world:
  worlds and contracts are its buttons, and Next on a world makes the jump. No new
  screen was needed.
- **Generated missions go in the save directory**, named `merc_c<slot>_<map>`, which
  also names the stock mission whose terrain they load.
- **`mc2_gencon.abl` already existed:** a generic script Microsoft shipped for this
  sort of mission. With the `dredattack01` brain, contracts need no new scripts.
- **No spawn sidecar is needed:** enemies start at the mission's own enemy start point
  farthest from the player, so the ground is known to be passable.
- **Damage is a repair bill, not carried damage**, to test the loop first. In phase 2.

Measured: a mission start took 2 to 8 s in total including game startup, on llvmpipe
in WSL, with other work running. That is noisy, and the Pi is still to be measured.

To try it, from a game directory with `merc/data` copied over its `data/`: New
Campaign, then "Mercenary Company (test)". Only 3 worlds and 1 job type (destroy the
lance) so far. To be judged by playing: pay and prices, lance sizes, and whether
contracts on the same maps get repetitive.

## Decisions

- **Tone: Pokemon-like first** (2026-11-04). Pilots get injured rather than die, and
  a lost mech can be bought back. Harsh rules can become a difficulty option later.
- **Progress: gated by licences** (2026-11-04). Three worlds in the first slice, with
  a champion fight unlocking the next.

## Open questions

1. **How much story writing is wanted, and by whom?** It sets the size of phase 4.
   For now: minimal text drafted by the agent, for Nicholas to edit.
2. **Is Pi playability a hard requirement for big zones?** If only desktop has to
   run them, phase 6 gets a lot easier.
3. **Which era second?** I would pick the Clan Invasion, since the chassis exist.
4. **Fate of the stock campaign in this mode?** Carver V could appear as a world that
   starts the stock story, which is a nice touch but ties story state to the sandbox.
   For now: separate.

## Sources

- Era dates: [Sarna: BattleTech eras](https://www.sarna.net/wiki/Eras),
  [BattleTech Eras (Catalyst)](https://bg.battletech.com/?p=1354)
- MC2 setting: [Sarna: MechCommander 2](https://www.sarna.net/wiki/MechCommander_2)
- BattleTech (2018) reception: [PCGamesN](https://www.pcgamesn.com/battletech/battletech-pc-review),
  [GeekWire](https://www.geekwire.com/2018/review-harebraineds-battletech-old-school-license-suggests/)
- MechWarrior 5 design: [PC Gamer hands-on](https://pcgamer.com/mechwarrior-5-mercenaries-hands-on-the-series-goes-back-to-its-roots),
  [Sarna: Conflict Zones](https://www.sarna.net/wiki/Conflict_Zone)
- Rights: [Sarna copyright policy](https://cfw.sarna.net/wiki/Policy:Copyrights),
  [Catalyst statement on online communities](https://bg.battletech.com/news/statement-regarding-online-battletech-communities)
- Other games named above are from general knowledge, not checked for this study.
