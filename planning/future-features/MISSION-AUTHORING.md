# Mission and campaign authoring (with skills)

Status: study, nothing built. Written 2026-11-20 from a read of the stock data
(`mc2srcdata/campaign`, `mc2srcdata/missions`), `code/logisticsmissioninfo.cpp`,
`code/objective.*`, `code/missionbriefingscreen.cpp`, the Omnitech work
(`MOD-COMPAT.md`) and the remaster's editor (`mc2-opengl-remastered/editor`).

## Problem

The port plays stock MC2 and the Omnitech campaigns, but there is no way to
make new content short of the 2001 editor, and no shared knowledge of what
makes an MC2 mission good. The aim is a set of Claude Code skills that take a
BattleTech conflict from canon and turn it into a playable, validated
campaign: research, design, build, check.

BattleTech has a deep body of published setting material, and several
sourcebooks are explicitly about structuring missions and campaigns. Those
supply both the canon (who fought where, with what, when) and the design
patterns (tracks, optional objectives, a warchest economy).

## Scope

In:

- New missions on **existing maps** (24 stock, about 100 Omnitech): new
  `.fit` and `.abl` on an existing `.pak`.
- New campaigns: `campaign.fit`, purchase files, operation screens, text.
- Five skills (below) and a validator script they share.
- Small engine changes only where authoring is blocked (see "Gaps").

Out (for now):

- New terrain. Needs an editor; see "Gaps" item 1 and open questions.
- New unit types, models or textures.
- Voice acting and movies. Text messages and `"STANDIN"` stand in.
- Multiplayer missions.

## Affected areas

- **Data**: new files only, in a campaign folder; no stock file changed.
- **Game**: possibly a loose-file override for the campaign name and
  operation screens, and the remaining Omnitech ABL commands (see "Gaps").
- **Tooling**: a validator (`scripts/`, Python, unit tested) and `.claude/skills/`.
- **Renderer, platform, shaders**: untouched.
- **CI**: the validator could run on any campaign folder checked in as a fixture.

## What the engine loads

### A mission

| File | Contents | Authoring |
|---|---|---|
| `<name>.pak` | Terrain, heights, textures, the pathing grid, placed buildings and forests | Editor only. Reused, not made |
| `<name>.fit` | Text (FIT/ini). `[Cameras]`, `[Water]`, `[Terrain]`, `[Warriors]`, `[Parts]`, one `[WarriorN]` (pilot profile, brain) and `[PartN]` (object number, team, commander, position, rotation, `ObjectProfile`, `CSVFile`, `VariantNumber`, squad) per unit, `[Teams]`, `[CommanderNGroup:M]`, `[NavMarkerN]`, `[ForestN]`, `[Artillery]`, `[MissionSettings]`, `[Weather]`, `[TheSky]`, objectives | Generated. Stock `mc2_01.fit` is 2,345 lines of regular blocks |
| `<name>.abl` | Mission brain (ABL): triggers, reinforcements, scripted orders | Written; 36 stock examples |
| `missions/warriors/*.abl` | Unit brains (guard, patrol, attack) | Mostly reused |
| `missions/purchaseNN.fit` | `[Mechs]`, `[Pilots]`, `[Components]` for sale before this mission | Generated |

Note: `[PartN]` positions must be on passable ground of that `.pak`; buildings
come from the `.pak` and are referenced by `BuildingWID`.

### Objectives (declarative, in the mission `.fit`)

Each `[Team0ObjectiveN]` has a title and description, priority (primary or
secondary), `ResourcePoints` reward, a marker, ordering flags
(`PreviousPrimaryObjectiveMustBeComplete`, `AllPrevious...`), hidden and
flag-activated triggers (`HiddenTrigger`, `ActivateOnFlag`,
`ActivateFlagID`), and lists of conditions, actions, failure conditions and
failure actions as sub-blocks.

Conditions (`condition_species_type` in `code/objective.h`): destroy all /
a number / a group / a specific unit / a specific structure; the same with
"capture or destroy" and "dead or fled"; capture unit or structure; guard unit
or structure; move any / all / all surviving units / all surviving mechs to an
area; boolean flag is set; elapsed mission time.

Actions: play a movie, play a WAV, display a text message (literal or string
id), set a boolean flag, make new technology available, remove a structure.

Flags chain objectives: stock `mc2_01` has objective 0 ("reach the airfield")
set flag 0, which reveals the next one. A large share of mission pacing (twists,
staged reveals, timed reinforcement warnings) can therefore be pure data, with
ABL kept for unit orders.

### Text

Every player-facing string can be literal text in the file:
`MissionName`/`MissionNameUseResourceString`, `Blurb`, `Blurb2` (briefing
panes), objective `Title`/`Description` with their `...UseResourceString`
flags, and `DisplayTextMessage` actions. The campaign reads `CampaignName` when
`NameID` is absent (`logisticsmissioninfo.cpp:93-100`). Stock uses string ids
into `mc2res.dll`; new content does not need to. A string may span several
lines of the file and keeps its line breaks (`FitIniFile::readIdString`, fixed
in PR #174, up to 8 KB); objective text has its own handling
(`objective.cpp:271-290`), to confirm on the first test mission.

### A campaign (`campaign/campaign.fit`)

`[Campaign]`: `GroupCount`, starting `CBills`, `NameID` or `CampaignName`,
`FinalVideo`.

`[GroupN]`: `NumberToComplete`, `MissionCount`, `OperationFile` (the
operation map screen, an `art/*.fit` + `.tga` pair), `Video`, `PreVideo`,
`Tune` (music), `ABLScript` (group script, used by the tutorial).

`[GroupNMissionM]`: `FileName`, `Mandatory`, `CompletePrevious`, `Hidden`,
`VideoOverride`, `PurchaseFile` (default `purchase_All`), and switches
`PlayLogistics`, `PlaySalvage`, `PlayPilotPromotion`, `PlayPurchasing`,
`PlaySelection`.

Branching is built in: `NumberToComplete < MissionCount` gives the player a
choice of missions; `Hidden` missions appear once unlocked. Money, surviving
mechs, salvage and pilot experience carry between missions; that carry-over is
the campaign's main source of consequence.

## Gaps

1. **Terrain.** No editor in this tree. The remaster has the original MFC
   editor running on its v0.3 renderer (Windows only, its own changelog in
   `editor/EDITOR_CHANGELOG.md`). Options, cheapest first: reuse maps (this
   study's scope); make `.pak` files with the remaster's editor; port the
   editor or write a terrain generator (a project of its own).
2. **Operation screens.** Each group names an `art/mcl_cm_op*.fit` + `.tga`.
   Reuse stock ones at first; a generated screen needs the layout format
   checked (`/mc2-file-formats`).
3. **Mission text messages from ABL.** Stock ABL has no free-text message
   command. Objective `DisplayTextMessage` actions cover most needs; Omnitech's
   `settextmsg`, already supported here, covers the rest at the cost of the
   campaign not running on stock MC2.
4. **Unit availability.** Every `ObjectProfile`/`CSVFile`/`VariantNumber` and
   warrior `Profile` must exist in the loaded tables (stock, or the combined
   Omnitech table). Canon unit lists must be mapped onto what exists, and the
   gaps reported, not papered over.
5. **Where campaigns live.** Stock reads `campaign/campaign.fit`. A second
   campaign either replaces it (Omnitech's approach: a separate game folder) or
   needs a campaign picker. The planned mercenary mode (`merc.fit` in a
   separate game folder, see the 2026-11-17 `newcampaign-guard` log entry)
   meets the same question. Decide before the first campaign ships.

## Design guidance (what the skills enforce)

Sources, used for facts and structure only; text is paraphrased and cited,
never copied:

- **Chaos Campaign** rules (in *Total Chaos* and the *Turning Points* PDFs):
  campaigns as tracks with objectives, optional objectives and a warchest.
  Maps onto groups, secondary objectives with `ResourcePoints`, CBills and
  purchase files.
- **Turning Points** series: one planet and conflict each, with factions,
  terrain, and suggested tracks. Close to campaign outlines.
- **Campaign Operations**: mercenary contracts, salvage rights, repair and
  morale. Levers for the logistics layer.
- **Historical** and **Field Manual** books: orders of battle.
- **Sarna.net**: quick lookup of planets, units, people and dates, with
  citations back to the books.

Checklist a design must answer:

- **Arc**: premise, stakes, a turn in the middle, an ending that pays off.
- **Objective variety**: raid, defend, escort, recon, base assault, salvage,
  withdrawal. No two "destroy everything" missions in a row.
- **Pacing inside a mission**: contact, escalation (reinforcements on a flag
  or timer), a twist, the finish. Nav markers and terrain point the way.
- **Choice and consequence**: at least one branching group; secondary
  objectives that pay in salvage or money; losses that matter.
- **Economy curve**: starting CBills, purchase lists that bring in better
  technology over time, salvage. Most of a campaign's difficulty lives here.
- **Era coherence**: units, factions, colours and technology right for the
  date (no Clan units in the Inner Sphere before 3050).
- **Difficulty**: enemy tonnage against the player's expected tonnage, drop
  weight, brain mix; easier first missions.
- **Voice**: briefings in character; radio lines at objective flags.

## Approach: five skills and a validator

| Skill | Input | Output (in `campaigns/<name>/`) |
|---|---|---|
| `mc2-lore` | A conflict or era | `bible.md`: dates, factions, units per side mapped to available profiles and variants (gaps listed), planets and terrain, key people, citations |
| `mc2-campaign-design` | Bible, scope | `design.md`: arc, mission graph, economy curve, purchase plan; `campaign.fit` |
| `mc2-mission-design` | A graph slot, a map | `missions/<name>.md`: objectives as condition/action/flag tables, forces, triggers, beats, text |
| `mc2-mission-build` | Mission spec | `<name>.fit`, `<name>.abl`, `purchaseNN.fit` |
| `mc2-mission-validate` | Built files | Validator report, then a headless run, then a playtest list |

Underneath them: a **map catalogue** (`campaigns/maps.md` or generated):
every stock and Omnitech map with size, biome, buildings (with `BuildingWID`),
passable regions and stock start points. It is how a "jungle base assault" is
matched to a real `.pak`, and how the builder places units on passable ground.

The validator (`scripts/validate_campaign.py`, unit tested):

- `campaign.fit`: every `FileName`, `PurchaseFile`, `OperationFile` exists;
  counts match blocks; at least `NumberToComplete` reachable missions per group.
- Mission `.fit`: block counts match (`NumParts`, `NumObjectives`,
  `NumConditions`...); every profile, CSV, variant and brain exists; part
  positions inside the map; objective conditions name real parts and
  buildings; every flag that activates an objective is set somewhere.
- Text: length limits per field (8 KB for a FIT string, less where the
  reading code's buffer is smaller).
- Then: `scripts/smoke.sh --missions "<name>"` (loads, ABL compiles, runs
  N frames without `STOP`), and a `-bench` scenario for screenshots.

Rejected: writing a terrain generator first (largest cost, least certain, and
reused maps are enough to prove the pipeline); a single "make me a campaign"
skill (no review point between research, design and build, and too much
context in one pass).

## Order of work

1. **Hand-built test mission.** One stock map, a new `.fit` written by hand
   (three objectives chained by flags, a hidden reinforcement, literal text),
   a one-group campaign. Run through `smoke.sh` and played once. Records the
   real friction: text line breaks, unit references, objective wiring.
2. **Validator**, test-first against stock missions (all must pass) and broken
   fixtures.
3. **Map catalogue** for stock maps.
4. `mc2-mission-design` + `mc2-mission-build` + `mc2-mission-validate`, proved
   by rebuilding the test mission from a spec.
5. `mc2-lore` + `mc2-campaign-design`, proved by a three-mission campaign from
   one canon conflict.

## Verification

- Validator: unit tests; every stock mission and campaign validates clean.
- Generated missions: `smoke.sh` on desktop GL and GLES (the content is
  renderer-agnostic, but smoke on both is cheap); a played run by a person
  (`planning/HARDWARE-TESTS.md`).
- Design quality: a person plays it. Record what worked in the skills.

## Open questions

- Where a second campaign lives (Gaps item 5): its own game folder, or a
  picker in the campaign dialog?
- Stock-compatible output only, or allow Omnitech ABL commands and units
  (bigger palette, runs only on this port and Omnitech)?
- New terrain later: the remaster's editor on Windows, or a port?
- Which sourcebooks you own as PDFs, for the skills to read locally.
- Distribution: BattleTech names are Topps/Catalyst trademarks; check
  Catalyst's fan-content policy before a campaign is published.
- Voice: text only, or generated voice lines (cost, quality, licensing)?
