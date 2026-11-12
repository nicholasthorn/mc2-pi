# String overflow triage (2026-11-11)

On 2026-11-11 the Omnitech mod crashed the Quest in mission innersphere2: an
objective's 299-character message was `strcpy`'d into `ChatInfo::message[128]`
(`ControlGui::setChatText`, fixed on its own branch, `fix-chat-text-overflow`).
Stock data fits the 2001 buffers, but mods carry longer text, so every copy of
text that data, a script, the network or the player can make longer is a risk.

## Scope and method

`code/ mclib/ gui/ GameOS/` at 03aa2aaf, comments left out: every `strcpy` and
`strcat` with a non-literal source (508), every `sprintf` with a `%s` (434) and
every `sprintf` whose format is not a literal (79, mostly resource strings used
as formats): 1019 lines. Each was traced to the size of its destination and the
origin of its source, and classed:

- **a**: game data or scripts a mod can change (.fit, CSV, ABL, resource strings, names and file names read from data)
- **b**: the network
- **c**: user input (edit boxes, prefs file, save names, command line, environment)
- **d**: an engine-built path; fixed when its name part comes from data
- **e**: constant, numbers only, or already bounded by a smaller source

Verdicts: **FIX** (a, b, c or d-with-data, and the destination can really be
overrun), **OK** (with the bound), **OK-dead** (not compiled, `#if 0`, never
called, or the ABL debugger, which the game never creates).

`memcpy` (320 lines): only the 146 whose size comes from data (record counts,
string lengths, header fields, packet sizes) were checked.

The full per-site tables, one row per site or run of identical sites, are
[`string-overflow-triage.tsv`](string-overflow-triage.tsv) and
[`string-overflow-triage-memcpy.tsv`](string-overflow-triage-memcpy.tsv).

## Counts (input lines)

| Group | Lines | a | b | c | d | e | FIX | OK | OK-dead |
|---|---|---|---|---|---|---|---|---|---|
| mclib cmponent, inifile, terrtxm, tgl | 240 | 24 | 0 | 0 | 6 | 210 | 13 | 210 (123 of them in cmponent.cpp's saveEXCEL, never called) | 17 |
| mclib, the rest (ABL, file, debugging) | 171 | 76 | 0 | 3 | 27 | 65 | 25 | 86 | 60 |
| code a to l | 181 | 57 | 7 | 8 | 22 | 87 | 36 | 136 | 7 (+2 left to the chat-text branch) |
| code m (by table row, 172 rows) | 285 | 55 | 3 | 18 | 42 | 54 | 61 | 102 | 9 |
| code n to z, gui, GameOS | 142 | 66 | 0 | 9 | 23 | 44 | 30 | 102 | 10 |

Network strings (class b) were already bounded where they arrive: player names
are `strncpy`'d to 24 plus a terminator (`multplyr.cpp`), the wire reader checks
every length (`netwire.cpp`). No class b site needed a fix.

## Fixed at the root

Rather than a patch per caller where one function made the mistake for all:

- `FitIniFile::copyString` and `CSVFile::copyString` left a too-long value
  unterminated (and most callers ignore the error, then `strcpy` the buffer).
  They now cut and terminate it, still returning the error. Tested.
- `cLoadString` copies the whole string after its STOP; with `MC2_STOP_EXITS=0`
  that overran the caller. It now cuts to the buffer.
- `File::readLine` / `readLineEx` wrote the terminator past `maxLength`, and
  callers pass their whole buffer. Tested.
- The ABL scanner overran `tokenString` and `curLiteral` before noticing a long
  word or string; it now stops before the store.
- `utils/str_bounded.h`: `copyBounded`, array overloads of `copyBounded` and
  `appendBounded` (a pointer destination does not compile, so a swapped call
  cannot take `sizeof` of a pointer), and `splitPathName`, a sized `_splitpath`
  fname. Tested.

Every rewritten `snprintf(x, sizeof(x), ...)` was compiled with
`-Wsizeof-pointer-memaccess` to prove `x` is an array.

## memcpy findings

| Site (at 03aa2aaf) | Problem | Done |
|---|---|---|
| `GameOS/gameos/utils/Image.cpp:410, 431` | RLE TGA: header bpp copies up to 32 bytes into `p[5]`; runs could pass the image | fixed, b350ade2 (refused; tested) |
| `GameOS/gameos/utils/Image.cpp` uncompressed | header size larger than the file read past it (the check was an `assert`) | fixed, b350ade2 (tested) |
| `mclib/file.cpp:1286` `readLineEx` | 2048-character ABL line copied 2049 bytes into `sourceBuffer[2048]`; no end-of-file check | fixed, b350ade2 (tested) |
| `mclib/msl.cpp:2074, 2082` | .agl allocates by frame count, reads by byte count | fixed, b350ade2 (allocates the larger) |
| `mclib/estring.cpp`, `echarstring.cpp`, `ewcharstring.cpp` `Format` | one element past a buffer whose size matched exactly | fixed, b350ade2 (`>=`) |
| `mclib/ablenv.cpp:1237, 1282` | brain statics from the mission .fit copied with no type or size check; `values[1024]` filled by the file's count | fixed, 3d52a531 + 72c949cb |
| `code/terrobj.cpp:1249` | save guard compared shorts with cell pairs: a 41 to 64 cell footprint overran `cellsCovered[81]` | fixed, 72c949cb (cut to 40, logged) |
| `code/mover.cpp:5848` | more than 10 distinct ammo types overrun `ammoTypeTotal[10]` | **deferred**: cutting the tally changes which weapons have ammo |
| `mclib/packet.cpp:296` `readPacket` | copies the packet's size with no destination size (mission .pak, save games) | **deferred**: needs a size parameter through every caller |
| `mclib/ablxstd.cpp:456` | an ABL function returning an array type copies it over the stack frame | **deferred**: reject non-scalar return types in the compiler |

## Deferred and left alone

- The three memcpy items marked deferred above.
- `_splitpath` (Windows' own and the Linux emulation) still writes as much as
  it finds; callers with buffers under the input's bound now use
  `splitPathName`, the rest have 1024-byte buffers or bounded inputs.
- `Image::loadCompressedTGA` fills a local `pixels` that shadows the member, so
  RLE TGAs never load through it (and `mergeBytes` is indexed by pixel, not
  byte). A separate bug; only its overruns were fixed here.
- Resource strings used as `sprintf` formats are now bounded, but a resource
  with an extra `%s` still reads a wrong argument.
- `mechcmd2.cpp` `ParseCommandLine` overruns `argv[30]` with more than 30
  tokens, and `-datapath` copies the flag, not its value.
- Truncation is the fix where a name is only shown or logged. Where it is a
  key sent to the other players, it is refused with a `PAUSE` instead: a
  multiplayer map name longer than `missionSettings.map[25]` is not selected,
  and an insignia name longer than `insigniaFile[35]` is left off the list
  (no stock or Omnitech map or insignia comes close). A mech csv path longer
  than `MoverInitData::csvFileName[50]` is still cut, so fails to load.
- FIT array readers copy each element with `getNextWord(..., elementString,
  9)`, so an element of 10 or more characters (`0xFF000000`) loses its end.
  No stock or Omnitech array has one (10621 array lines, 2026-11-12), so it is
  left; a mod writing a full hex colour into an array would hit it.
- ABL exponent literals (`1e3`) never compile: `getNumber` reads the
  exponent's sign or digits without first taking the `e` (`ablscan.cpp`). No
  script here uses one.
- Dead code (cmponent.cpp's saveEXCEL, the ABL debugger, `#if 0` blocks) was
  not touched.

## Whole numbers past 32 bits (2026-11-12)

FIT and CSV integers went through `atol` into a `long`: 64-bit on Linux, Pi
and Quest, 32-bit on Windows, where `atol` clamps a decimal past 2147483647
to `LONG_MAX`. Hex kept its last 8 digits, so `l Color = 0xff000000` was
-16777216 on Windows and 4278190080 on Linux. ABL integer literals went
through a float, exact only up to 2^24. Now every FIT and CSV integer reader
and the ABL scanner use `wholeNumberLow32` (`utils/str_bounded`): the exact
value, kept to its low 32 bits, narrowed to the reader's type as before.

Scan of stock (`mc2srcdata`) and Omnitech (`gd-mco/data`, `mco628`): 2882 FIT,
356 CSV and 2383 ABL files; ABL also across every mod pack here (16356).

| Where | Values | Old Windows | Old Linux/Pi/Quest | Now (all) |
|---|---|---|---|---|
| FIT signed `l` fields, about 10000 lines (UI colours: `NormalColor0`, `Color`, ...) and one object template (`xstarport_gantry_1.fit` `HighTemplate 0xe0f0f830`) | hex with the top bit set, e.g. `0xff000000` | -16777216 | 4278190080 | -16777216 |
| `buildings.csv` column 9, about 130 cells, stock and Omnitech | hex colours, e.g. `0xFFCDCDCD` | -3289651 | 4291677645 | -3289651 |
| FIT and CSV decimals past the signed 32-bit range | none in either data set | | | |
| ABL integer literals past 2^24 | only `4278190080` (exact as a float) | 0xFF000000 | 0xFF000000 | 0xFF000000 |

So nothing changes on Windows for this data. On Linux, Pi and Quest a hex
colour in a signed field now holds the negative `long` that the 2001 game and
the Windows build always had; its 32 bits, which is what a colour is passed
on as (`DWORD`), are the same. A decimal past 2147483647, which no data here
has, now wraps on every platform instead of clamping on Windows only.

## FIX sites

| Site (at 03aa2aaf) | Destination | Source | Class | Done |
|---|---|---|---|---|
| `mclib/inifile.cpp:223` | error[256] (FitIniFile::afterOpen) | line = block header line read from the .fit file (up to 253 chars, findNextBlockStart(line,2... | a | fixed, 8fa3ac45 |
| `mclib/inifile.cpp:1460` | line[2048] (via equalSign = pointer i... | tmpLine[2047] continuation lines of a multi-line quoted string from the .fit file | a | fixed, 8fa3ac45 |
| `mclib/terrtxm.cpp:564` | tmpy[512] (TerrainTextures::initDetail) | details[typeNum].detailName: DetailName readIdString up to 2047 chars from terrain textures... | a | fixed, d26dae6f |
| `mclib/terrtxm.cpp:693` | tmpy[512] (initMask) | types[typeNum].maskName: MaskName readIdString up to 2047 chars | a | fixed, d26dae6f |
| `mclib/terrtxm.cpp:752` | tmpy[512] (initOverlay) | txmName = overlays[i].overlayName: OverlayName readIdString up to 2047 chars | a | fixed, d26dae6f |
| `mclib/terrtxm2.cpp:426` | newName[1024] (burnInShadows) | fileName = colorMapName (mission .fit ColorMapName, readIdString up to 1022 chars) or terrai... | a | fixed, d26dae6f |
| `mclib/terrtxm2.cpp:965` | newName[1024] (burnInShadows) | same fileName | a | fixed, d26dae6f |
| `mclib/terrtxm2.cpp:1081` | waterFile[1024] (resetWaterDetailText... | fileName = FullPathFileName texturePath(<80) + dName (colormap name from mission .fit, up to... | ... | fixed, d26dae6f |
| `mclib/terrtxm2.cpp:1588` | dName[1024] (TerrainColorMap::init) | fileName = colorMapName (readIdString up to 1022) or terrainName (up to 1023) | a | fixed, d26dae6f |
| `mclib/terrtxm2.cpp:1608` | dName[1024] (init) | same fileName | a | fixed, d26dae6f |
| `mclib/terrtxm2.cpp:1630` | waterDetailBaseName[1024] (init) | same fileName | a | fixed, d26dae6f |
| `mclib/terrtxm2.cpp:1632` | dName[1024] (init) | waterDetailBaseName (up to 1023 even once 1630 is bounded) + %04d | a | fixed, d26dae6f |
| `mclib/terrtxm2.cpp:1646` | dName[1024] (init) | waterDetailBaseName + %04d | a | fixed, d26dae6f |
| `mclib/terrtxm2.cpp:1680` | newName[1024] (init) | same fileName + .burnin | a | fixed, d26dae6f |
| `mclib/ablenv.cpp:1342` | RoutineInfo::name[128] (ModuleInfo on... | ABL function identifier from script; identifiers only bounded by MAXLEN_TOKENSTRING 2048 | a | fixed, 3d52a531 |
| `mclib/ablenv.cpp:1356` | ModuleInfo::fileName[128] | ModuleRegistry fileName = full ABL path, e.g. missionPath + campaign ABLScript (<=255 from f... | d | fixed, 3d52a531 |
| `mclib/ablerr.cpp:173` | errMessage[256] (MAXLEN_ERROR_MESSAGE) | SourceFiles[FileNumber] (<=255) + syntaxErrorMessages + wordString (token text up to 2047, e... | a | fixed, 3d52a531 |
| `mclib/ablexec.cpp:642` | static stateList[60][256] | FSM state name (script identifier, up to 2047) + SetStateDebugStr[256] | a | fixed, 3d52a531 |
| `mclib/ablexec.cpp:661` | errStr[512] | module file path (data) + current and previous state names (script identifiers up to 2047) | a | fixed, 3d52a531 |
| `mclib/ablrtn.cpp:1349` | StateHandleInfo::name[128] (StateHand... | state routine identifier from script (up to 2047) | a | fixed, 3d52a531 |
| `mclib/ablrtn.cpp:1712` | SetStateDebugStr[256] | module file path (data) + state name (script identifier) + line | a | fixed, 3d52a531 |
| `mclib/ablscan.cpp:689` | fullPath[255] | SourceFiles[0] (char[256], up to 255 chars + NUL = 256 bytes); path from data (campaign ABLS... | d | fixed, 3d52a531 |
| `mclib/ablscan.cpp:691` | fullPath[255] | directory of SourceFiles[0] (<=255) + #include_ file name from script (<=127) | a | fixed, 3d52a531 |
| `mclib/ablscan.cpp:1303` | SourceFiles[n][256] (MAXLEN_FILENAME) | ABL source path: missionPath + campaign ABLScript (<=255, logisticsmissioninfo.cpp:144) + ".... | d | fixed, 3d52a531 |
| `mclib/ablscan.cpp:1307` | openFiles[n].fileName[256] | same path as 1303 | d | fixed, 3d52a531 |
| `mclib/ablxstd.cpp:709` | curBuffer: script char array via ABLi... | CurModule->getFileName(): full ABL path (data, ~30-273 chars) | d | fixed, 3d52a531 |
| `mclib/ablxstd.cpp:710` | fsmBuffer: script char array, size un... | CurFSM->getFileName() (data path) or "none" | d | fixed, 3d52a531 |
| `mclib/ablxstd.cpp:770` | message[512] | script fatal() string s (any script char array/string constant length) | a | fixed, 3d52a531 |
| `mclib/ablxstd.cpp:817` | message[512] | script assert() string s | a | fixed, 3d52a531 |
| `mclib/ablxstd.cpp:899` | SetStateDebugStr[256] | FSM file path (data) + state name (script identifier) + line | a | fixed, 3d52a531 |
| `mclib/ablxstmt.cpp:368` | s1[512] | routine identifier from script (up to 2047) + time | a | fixed, 3d52a531 |
| `mclib/ablxstmt.cpp:369` | s[512] | [%08d]  + CallStackLevel spaces + s1 (<=511) | a | fixed, 3d52a531 |
| `mclib/debugging.cpp:65` | textBuffer[15][256] (MAX_DEBUG_WINDOW... | any print text; in _DEBUG builds ABL print("...") strings go here unbounded (ablxstd.cpp:547... | a | fixed, 3d52a531 |
| `mclib/debugging.cpp:68` | textBuffer[15][256] | same as 65 | a | fixed, 3d52a531 |
| `mclib/err.cpp:33` | msg[512] | errMessage from callers: built strings with data file/block names in caller buffers of up to... | a | fixed, 3d52a531 |
| `mclib/file.cpp:278-279` | actualPath[2048] | CDInstallPath (<=259) + File::open fileName (any path the game opens; names from data, no co... | d | fixed, b350ade2 |
| `mclib/file.cpp:309` | data[2048] | FileMissingString (resource <=511) + fileName (data path, e.g. texturePath + ColorMapName <=... | a | fixed, b350ade2 |
| `mclib/mech3d.cpp:394` | animName[512] | appearance name (<=511, from object type fit AppearanceName) + MechAnimationNames[i] | a | fixed, b350ade2 |
| `code/ablmc2.cpp:6167` | s[256] | ABL script string (mcprint arg; string constants up to MAXLEN_STRING_CONSTANT 1024, char arr... | a | fixed, 3d52a531 |
| `code/ablmc2.cpp:6257` | s[50] | BUGLOG only (Debug builds define -DBUGLOG); fixed text alone is ~69 chars, plus mover name (... | a | fixed, 3d52a531 |
| `code/ablmc2.cpp:6497` | s[256] | fName = ABL fileOpen() script arg (ablxstd.cpp execStdFileOpen -> UserFile::open -> ABLFile:... | a | fixed, 3d52a531 |
| `code/ablmc2.cpp:6663` | s[256] | LastCoreAttackTarget->getName(): may be Building/TerrainObject, returns lastName[256] filled... | a | fixed, 3d52a531 |
| `code/controlgui.cpp:747` | total[256] | format = IDS_TOTAL resource (cLoadString 255, up to 254 chars) + %d | a | fixed, 72c949cb |
| `code/controlgui.cpp:1926` | buffer[256] | format = IDS_RP resource (cLoadString 256, up to 255 chars) + long | a | fixed, 72c949cb |
| `code/group.cpp:655` | debugStr[256] | Debug-only (_DEBUG and DEBUGJUMPGOALS TRUE): ~60-char header + up to 8 chars "[r,c] " per ju... | e | fixed, 72c949cb |
| `code/gvehicl.cpp:1040` | longName[MAXLEN_MECH_LONGNAME=35] | format = IDS_MFDVEHICLE_PRINTSTRING resource (up to 255) with name (cLoadString up to 64) an... | a | fixed, 72c949cb |
| `code/gvehicl.cpp:4347` | attackerStr[30] | attacker mover name, up to 64 chars (Mover::name[65], from resource/data) | a | fixed, 72c949cb |
| `code/gvehicl.cpp:4354` | err[128] | attackerStr (overflowed above) + masterId, damage %.2f, hitLocation (out of range on this pa... | b | fixed, 72c949cb |
| `code/infowindow.cpp:741` | tmpNumber2[64] | format = IDS_MISSION_SHOTSLEFT resource (cLoadString 63, up to 62) + ammo int | a | fixed, 72c949cb |
| `code/loadscreen.cpp:218` | error[256] | path = artPath(<=79) + BackgroundFileName from mcl_loadingscreen*.fit (readIdString 255) + "... | d | fixed, 72c949cb |
| `code/loadscreen.cpp:234` | error[256] | path = artPath + LoadingBar FileName (readIdString 255) + ".tga" | d | fixed, 72c949cb |
| `code/loadscreen.cpp:259` | error[256] | path = artPath + WaitImage FileName (readIdString 255) + ".tga" | d | fixed, 72c949cb |
| `code/logistics.cpp:680` | CompressedMech::pilotFile[50] | LogisticsPilot::fileName (pilots.csv line, readLine 256, or save game FileName readIdString... | a | fixed, 72c949cb |
| `code/logistics.cpp:681` | CompressedMech::mechFile[50] | chassis->fileName = full CSV path objectPath + name from buildings.csv (readString 256) + ".... | d | fixed, 72c949cb |
| `code/logistics.cpp:682` | CompressedMech::variantName[64] | variant name EString: CSV readString 256, save game VariantName readIdString 255 (edit box l... | a | fixed, 72c949cb |
| `code/logistics.cpp:833` | MoverInitData::csvFileName[50] | chassis->fileName = objectPath + mech name from buildings.csv (<=255) + ".csv" (stock fits:... | d | fixed, 72c949cb |
| `code/logistics.cpp:836` | MoverInitData::pilotFileName[50] | LogisticsPilot::fileName (pilots.csv line <=256 or save game FileName <=255) | a | fixed, 72c949cb |
| `code/logisticsdata.cpp:311` | error[256] | variantFullPath (up to ~338 chars, name from buildings.csv) + 20 fixed chars; runs when that... | d | fixed, 72c949cb |
| `code/logisticsdialog.cpp:696` | errorStr[256] | path = savePath(<=79) + file name found in the save directory (cFileName up to MAX_PATH 259)... | c | fixed, 72c949cb |
| `code/logisticsdialog.cpp:723` | errorStr[256] | path = campaignPath(<=79) + file name found in the campaign directory (up to 259) + ".fit" | c | fixed, 72c949cb |
| `code/logisticsdialog.cpp:998` | tmp[256] | format = IDS_DIALOG_CBILLS resource (cLoadString 255, up to 254) + long cBills | a | fixed, 72c949cb |
| `code/logisticsdialog.cpp:1010` | tmp[256] | format = IDS_DIALOG_TIME resource (up to 254) + gos formatted date and time strings | a | fixed, 72c949cb |
| `code/logisticsdialog.cpp:1165` | promptString[256] | format = IDS_DIALOG_OVERWRITE_PROMPT resource (up to 254) + selectedName = list hidden text... | c | fixed, 72c949cb |
| `code/logisticsdialog.cpp:1214` | promptString[256] | format = IDS_DIALOG_DELETE_PROMPT resource (up to 254) + tmpName from edit box (limitEntry 20) | c | fixed, 72c949cb |
| `code/logisticsdialog.cpp:1617` | promptString[256] | format = IDS_DIALOG_OVERWRITE_PROMPT (up to 254) + selectedName from variant edit box (limit... | c | fixed, 72c949cb |
| `code/logisticsdialog.cpp:1646` | promptString[256] | format = IDS_DIALOG_DELETE_PROMPT (up to 254) + selectedName from variant edit box (limit 14) | c | fixed, 72c949cb |
| `code/logisticsdialog.cpp:1924` | totalText[1024] | text2 (up to ~263) appended to <= ~887 already: worst case ~1150 > 1024 (three max-length re... | a | fixed, 72c949cb |
| `code/logisticsmechdisplay.cpp:122` | text[64] | format = IDS_MB_MECH_WEIGHT (cLoadString 63) + float + mechClass (resource, cLoadString 256) | a | fixed, 72c949cb |
| `code/logisticsmissioninfo.cpp:211` | errorStr[256] | fileName[1024] from campaign fit "FileName" (readIdString 1023) | a | fixed, 72c949cb |
| `code/logisticsmissioninfo.cpp:614` | errorStr[256] | missionName param of setNextMission: campaign mission fileName (up to 1022), map list hidden... | a | fixed, 72c949cb |
| `code/logisticsmissioninfo.cpp:743` | errorStr[256] | global missionName[1024] (mechcmd2.cpp, built from command-line args, unbounded); note it pr... | c | fixed, 72c949cb |
| `code/logisticspilot.cpp:67-68` | path[256] | warriorPath(<=79) + pilot file name (pilots.csv line, readLine 256) + ".fit" | d | fixed, 72c949cb |
| `code/logisticspilot.cpp:75` | errorString[256] | Couldn't open file  + pilot file name (up to 256) | a | fixed, 72c949cb |
| `code/mainmenu.cpp:404` | lawyerBabble[2048] | format realText = cLoadString(IDS_LAWYER_BABBLE, 2047) + pID[64] from registry | a | fixed, 72c949cb |
| `code/mainmenu.cpp:866` | formatStr[256] | format cLoadString(IDS_PLAYER_LEFT,255) + playerName = playerInfo[0].name char[25] (network,... | a | fixed, 72c949cb |
| `code/mc2movie.cpp:164` | buf[1024] | ./data/movies/%s.mpg with MOVIEName[1024] = basename of movie path; names from campaign/miss... | a | fixed, 72c949cb |
| `code/mech.cpp:1412` | longName[MAXLEN_MECH_LONGNAME=35] | format IDS_MFDMCH_PRINTSTRING (resource) + thisMechName = cLoadString(descID from mech CSV,... | a | fixed, 72c949cb |
| `code/mechbayscreen.cpp:397` | str[64] | format cLoadString(IDS_MB_WEIGHT, 63) + two longs | a | fixed, 72c949cb |
| `code/mechbayscreen.cpp:800` | str[64] | format cLoadString(IDS_MB_MECH_WEIGHT,63) + weight + mechClass (resource IDS_VERY_LIGHT.. re... | a | fixed, 72c949cb |
| `code/mechcmd2.cpp:1592` | path[256] | temp = font name from cLoadString(IDS_FLOAT_HELP_FONT, temp, 255), cut at ',' | a | fixed, 72c949cb |
| `code/mechcmd2.cpp:2607` | missionName[1024] (global, :201) | argv token from command line; tempCommandLine[4096] so a token is up to 4095 | c | fixed, 72c949cb |
| `code/mechcmd2.cpp:2615` | missionName[1024] | + further argv tokens appended in quoted-name loop | c | fixed, 72c949cb |
| `code/mechcmd2.cpp:2633` | missionName[1024] | argv token (<=4095) | c | fixed, 72c949cb |
| `code/mechlabscreen.cpp:201` | errorStr[255] | path = artPath + FileName from layout .fit readIdString(...,fileName,255) + ".fit" | ... | fixed, 72c949cb |
| `code/mechlopedia.cpp:537` | formatText[256] | format cLoadString(IDS_EN_WEIGHT,255) + number | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:547` | formatText[256] | format IDS_EN_CLASS (254) + mechClass resource (<=255) | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:558` | formatText[256] | format IDS_EN_ARMOR (254) + armor class name resource tmp (254) + number | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:569` | formatText[256] | format IDS_EN_SPEED (254) + number | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:618` | formatText[256] | format IDS_EN_HOUSE (254) + house name resource tmp (254) | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:628` | formatText[256] | format IDS_EN_WEIGHT + number | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:638` | formatText[256] | format IDS_EN_CLASS + mechClass (<=255) | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:649` | formatText[256] | format IDS_EN_ARMOR + tmp resource (254) + number | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:660` | formatText[256] | format IDS_EN_SPEED + number | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:672` | formatText[256] | format IDS_EN_JUMP + number | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:823` | final[256] | format IDS_EN_WEAPON_RANGE (254) + range name resource tmp (254) | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:832` | final[256] | format IDS_EN_WEAPON_DAMAGE (254) + number | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:843` | final[256] | format IDS_EN_WEAPON_ARMOR (254) + constant 32 | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:859` | final[256] | format IDS_EN_WEAPON_RATEOFFIRE (254) + float 10/recycleTime (recycleTime from component dat... | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:870` | final[256] | format IDS_EN_WEAPON_HEAT + number | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:881` | final[256] | format IDS_EN_WEAPON_AMMO + number | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:891` | final[256] | format IDS_EN_WEAPON_COST + number | a | fixed, 72c949cb |
| `code/mechlopedia.cpp:1202` | formatted[256] | format IDS_EN_BUILDING_WEIGHT (254) + weight | a | fixed, 72c949cb |
| `code/mission.cpp:1592` | BattleMech::variantName[64] | LogisticsMech getName() = LogisticsVariant::variantName EString: from .var / savegame readId... | a | fixed, 72c949cb |
| `code/missiongui.cpp:3193` | debugString[256] | ~90 chars of numbers + target->getName(): Building::getName returns cLoadString(...,254) or... | a | fixed, 72c949cb |
| `code/missiongui.cpp:4840` | debugString[256] | ~100 chars of numbers + target->getName() (building name up to 253) | a | fixed, 72c949cb |
| `code/missionresults.cpp:125` | fullName[256] | format cLoadString(IDS_AUTOSAVE_NAME,255) + pMissionName = lastMissionName (campaign .fit Fi... | a | fixed, 72c949cb |
| `code/missionselectionscreen.cpp:228` | errorStr[256] | artPath + OperationFile (campaign .fit readIdString 255, logisticsmissioninfo.cpp:170) + ".fit" | ... | fixed, 72c949cb |
| `code/mover.cpp:2426` | static s[128] | getName() Mover name (<=64) + pilot name (<=79) + index | a | fixed, 72c949cb |
| `code/mover.cpp:2482` | s[128] | ModuleInfo.name[128] (ABL module name from script) | a | fixed, 72c949cb |
| `code/mover.cpp:2522` | static s[128] | Mover name + pilot name + index | a | fixed, 72c949cb |
| `code/mover.cpp:2550` | s[128] | LR: + " %d"/" [%d]" per global path step; loop runs i=0..curGlobalStep+8, up to MAX_GLOBAL_P... | a | fixed, 72c949cb |
| `code/mover.cpp:2619` | static s[128] | Mover name + pilot name + index | a | fixed, 72c949cb |
| `code/mpconnectiontype.cpp:800` | tmp[256] | str = IP combo edit text; typing limited to 15, but choosing a list item sets it via setEntr... | c | fixed, 72c949cb |
| `code/mpconnectiontype.cpp:866` | Display[256] | format cLoadString(IDS_MP_CON_MODEM_CONNECTING,255) + same str | c | fixed, 72c949cb |
| `code/mploadmap.cpp:636` | errorStr[256] | missionPath + pFileName (<=254 from campaign readIdString / CSV / dir listing) + ".fit" | ... | fixed, 72c949cb |
| `code/mpparameterscreen.cpp:440` | finalStr[256] | format cLoadString(IDS_MP_PARAMTERS_BOOT_PROMPT,255) + playerList[j].name char[25] | a | fixed, 72c949cb |
| `code/mpparameterscreen.cpp:616` | errorStr[256] | fileName: mission file name from map list hidden text (<=254) or missionSettings.map (<=24) | a | fixed, 72c949cb |
| `code/mpparameterscreen.cpp:647` | MissionSettings::map[MAXLEN_MAP_NAME=25] | fileName (mission file name from map list, <=254) | a | fixed, 72c949cb |
| `code/mpparameterscreen.cpp:648` | MissionSettings::name[MAXLEN_MAP_DESC... | missionName[256] from mission .fit readIdString 255 or cLoadString 255; the gosASSERT(<25) a... | a | fixed, 72c949cb |
| `code/mpparameterscreen.cpp:799` | IPAddressStr[256] | format cLoadString(IDS_MP_PARAM_HOST_IP,255) + sessionIPAddress[16] | a | fixed, 72c949cb |
| `code/mpparameterscreen.cpp:1534` | formatStr[256] | format cLoadString(IDS_PLAYER_LEFT,255) + playerName (playerInfo[0].name char[25]) | a | fixed, 72c949cb |
| `code/mpprefs.cpp:86` | error[256] | artPath + blockName (FileName from mcl_mp_playerprefs .fit, readIdString 255) + ".fit" | ... | fixed, 72c949cb |
| `code/mpprefs.cpp:174` | path[256] | path2insignia (27 chars) + cFileName of a .tga in data/multiplayer/insignia (<=255) | c | fixed, 72c949cb |
| `code/mpprefs.cpp:301` | MC2Player::insigniaFile[MAXLEN_INSIGN... | pName = aBmpListItem fileName = insignia directory cFileName (<=255) | c | fixed, 72c949cb |
| `code/mpprefs.cpp:582` | MC2Player::name[MAXLEN_PLAYER_NAME=25] | txt from combo edit; typing limited to 24, but picking a list item sets text from prefs.play... | c | fixed, 72c949cb |
| `code/mpprefs.cpp:593` | MC2Player::unitName[25] | txt from combo edit / prefs.unitName[10][256] | c | fixed, 72c949cb |
| `code/mpprefs.cpp:607` | MC2Player::insigniaFile[35] | pName (<=255) | c | fixed, 72c949cb |
| `code/mpprefs.cpp:631` | MC2Player::name[25] | prefs.playerName[0] char[256] (prefs file readIdString 255) | c | fixed, 72c949cb |
| `code/mpprefs.cpp:632` | MC2Player::unitName[25] | prefs.unitName[0] char[256] | c | fixed, 72c949cb |
| `code/mpprefs.cpp:633` | MC2Player::insigniaFile[35] | prefs.insigniaFile char[256] (prefs file readIdString 255) | c | fixed, 72c949cb |
| `code/mpstats.cpp:121` | text2[256] | format cLoadString(IDS_MP_LM_MAP_LIST_MAP_NAME,255) + missionSettings.name char[32] | a | fixed, 72c949cb |
| `code/mpstats.cpp:129` | text2[256] | format (254) + mType[128] resource | a | fixed, 72c949cb |
| `code/mpstats.cpp:135` | text2[256] | format (254) + number | a | fixed, 72c949cb |
| `code/mpstats.cpp:288` | formatStr[256] | format IDS_PLAYER_LEFT (254) + hostName (playerInfo[0].name, <=24) | a | fixed, 72c949cb |
| `GameOS/gameos/gameos.cpp:549` | cfg_dir[1024] | getenv("HOME") + "/.config" | c | fixed, b350ade2 |
| `GameOS/gameos/gameos_fileio.cpp:27` | m_Filename[MAX_PATH=260] (fileio.hpp:52) | FileName passed to gos_OpenFile: ASE paths from mclib/msl.cpp:495,2260 and tgl.cpp:1493 (tgl... | ... | fixed, b350ade2 |
| `GameOS/src/platform_stdlib.cpp:28` | ptr dir (caller buffer) | dirname() of path; path from callers (data file names, font paths) | ... | fixed at the callers with short buffers (splitPathName); emulation unchanged |
| `GameOS/src/platform_stdlib.cpp:32` | ptr fname (caller buffer) | basename() of path from callers | ... | as above |
| `GameOS/src/platform_stdlib.cpp:46` | ptr ext (caller buffer) | extension of path | ... | as above |
| `code/pilotreadyscreen.cpp:802` | fileName[256] | pPilot->getName(): resource string via cLoadString(descIndex, path, 256) in logisticspilot.c... | ... | fixed, 72c949cb |
| `code/pilotreviewarea.cpp:402` | realText[256] | format tmpText = cLoadString(IDS_PILOT_MISSIONS, 255) (<= 254 chars), int | a | fixed, 72c949cb |
| `code/pilotreviewarea.cpp:408` | realText[256] | format IDS_PILOT_KILLS (<= 254), int | a | fixed, 72c949cb |
| `code/pilotreviewarea.cpp:413` | realText[256] | format IDS_PILOT_NAME (<= 254) + pilot name (resource string, <= 255) | a | fixed, 72c949cb |
| `code/pilotreviewarea.cpp:426` | realText[256] | format IDS_PILOT_RANK (<= 254) + rankStr[32] | a | fixed, 72c949cb |
| `code/pilotreviewarea.cpp:1174` | finalText[256] | format text = IDS_PILOT_PROMOTED (<= 254) + tmpRank[64] | a | fixed, 72c949cb |
| `code/pilotreviewarea.cpp:1191` | realText[256] (ActivePilotListItem) | format IDS_PILOT_MISSIONS (<= 254), int | a | fixed, 72c949cb |
| `code/pilotreviewarea.cpp:1200` | realText[256] | format IDS_PILOT_NAME + pilot name (resource, <= 255) | a | fixed, 72c949cb |
| `code/pilotreviewarea.cpp:1430` | realText[255] (PilotPromotionArea::se... | format tmpText[255] = IDS_PILOT_MISSIONS (<= 254), int | a | fixed, 72c949cb |
| `code/pilotreviewarea.cpp:1436` | realText[255] | format IDS_PILOT_NAME + pilot name (resource, <= 255) | a | fixed, 72c949cb |
| `code/prefs.cpp:621` | ipAddresses[j] char[24] | ipAddresses[j+1] char[24]; but rows are loaded by prefs.cpp:249 readIdString(blockName, &ipA... | ... | fixed at the source: prefs.cpp:249 reads the IP at its 24-byte size |
| `code/simplecamera.cpp:343` | testName[256] (setMech) | NoPathFileName[256] = _splitpath of mech file name (chassis/variant getFileName EString from... | ... | fixed, 72c949cb |
| `code/simplecamera.cpp:414` | testName[256] (setVehicle) | fileName (vehicle getFileName from data, mechlopedia.cpp:510; setObject from briefing object... | ... | fixed, 72c949cb |
| `code/simplecamera.cpp:483` | testName[256] (setComponent/setBuilding) | fileName: component pictureFileName (heap copy of CSV token, logisticscomponent.cpp:172), bu... | ... | fixed, 72c949cb |
| `code/terrobj.cpp:372-378` | s[128] | %d per sub-area id, numSubAreas0/1 each up to MAX_SPECIAL_SUB_AREAS (25), counts set from ma... | ... | fixed, 72c949cb |
| `code/warrior.cpp:1055` | callsign[MAXLEN_PILOT_CALLSIGN=20] | name[80] = cLoadString(DescIndex, name, 79) (resource pilot name, up to 78 chars) | a | fixed, 72c949cb |
| `code/warrior.cpp:8146` | data.callsign[20] | callsign[20] | a | fixed by warrior.cpp:1055 |
| `code/warrior.cpp:8278-8282` | name[80], callsign[20], videoStr[20],... | MechWarriorData from a save game (MechWarrior::init(MechWarriorData) at 8443), same-size fie... | ... | fixed, 72c949cb |
| `gui/aanim.cpp:444` | errorStr[255] | blockName from aAnimGroup::init callers: gui/alistbox.cpp:1399 passes animationSt read with... | a | fixed, 72c949cb |
| `gui/afont.cpp:298` | path[256] | buffer[256] = cLoadString(font resource id, buffer, 255) (font name from resource strings, u... | a | fixed, 72c949cb |
