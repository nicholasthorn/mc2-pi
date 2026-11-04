# Easy install: game, game data and Omnitech

Status: bundling approved by the owner (2026-11-04). Full Windows, Linux and Pi
packages are built (`release.yml`); the Quest APK and the Omnitech helper are
not. The aim is a player going from the Releases
page to a running game in one download on each platform, with no compiler, no
data build and no command line where that can be avoided.

## Problem

A release today asks a player to install packages, clone `mc2srcdata`, run a
bash and make data build (on Windows, on another machine or in WSL), and copy
the result. On the Quest they also need `adb` and a series of push commands.
Omnitech has no install path at all outside `MOD-COMPAT.md`'s notes.

## What may be distributed

This sets the design. It is a reading of the licences, not legal advice.

| Content | Terms | Consequence |
|---|---|---|
| Stock game data (`mc2srcdata`) | `EULA.txt` 3(A): "use, reproduce and distribute the Art Assets on a non-commercial basis only"; 3(D) keep notices; 3(E) include the licence with source; 3(B) no trademark licence | A built data pack may go on a free release with `EULA.txt` in it. This replaced `RELEASES.md`'s earlier "never included" |
| Omnitech and its campaign packs | `MCO EULA.txt`: no commercial use, keep notices. It grants no right to redistribute, and the packs contain the team's own work | Do not mirror or bundle without the team's permission. Fetch from ModDB, or ask |

The data pack stays a separate work beside the GPL engine, under its own
licence, as `mc2srcdata` is today. Nothing charges for it, and no release page
carries donation links, since that could make it commercial.

## Proposal

### Stock game: full packages

| Asset | For | Size (about) | How to play |
|---|---|---|---|
| `mc2-<ver>-windows-x64-full.zip` | Windows | 550 MB | Unzip, double-click `mc2.exe` |
| `mc2-<ver>-linux-x86_64-full.tar.gz` | Linux PCs | 550 MB | Unpack, run `./mc2` |
| `mc2-<ver>-rpi-arm64-full.tar.gz` | Pi 4/400 | 550 MB | `apt` line, unpack, run `./mc2` |
| `mc2-<ver>-quest-full.apk` | Quest | 550 MB | Drag on to SideQuest, or `adb install` |

The sizes are estimates from the 490 to 510 MB built data folders before
compression; movies add about 180 MB. Measure with the first build, and decide
then whether movies go in (they need Git LFS and `ffmpeg` in the job).

- **CI:** one `gamedata` job in `release.yml` builds the data once from a pinned
  `mc2srcdata` commit, as `smoke.yml` already does (cached the same way), and the
  package jobs add it. All assets stay under GitHub's 2 GB limit per file.
- **Engine-only packages stay,** for people who built their own data or want
  only the update.
- **`options.cfg`** ships ready (low settings on the Pi and Quest), so the "first
  start may end straight away" note in `INSTALL.md` goes.

### Quest: everything in one APK

Sideloading is the only step left to the player. The APK holds the data and
shaders in its assets. On first start, and after an update, `MC2Activity` copies
them to the files folder before SDL starts, keeping `options.cfg` and saves. A
marker file with the APK's version says whether a copy is needed. No network
permission, and no shader mismatch with the APK, which the current separate
tarball allows.

The costs are a slow first start (with a progress display, since the headset
would otherwise look hung) and the data being on the headset twice, about
1.1 GB. An in-app download from the release avoids both but needs a network
permission and error handling, so it is the later option if size becomes the
complaint.

A stable `QUEST_KEYSTORE` is a precondition: an update signed with another key
needs an uninstall, which deletes saves.

### Omnitech: a helper, unless the team agrees to more

`mc2-omnitech` (a script in each package, run by double-click on Windows):

1. Lists the Omnitech 6.28 installer (434.5 MB) and the campaign packs (11 to
   58 MB each), with their ModDB pages, and opens them. The player downloads
   what they want into one folder. Automatic ModDB downloads go through
   short-lived mirror links, so scripting them would be fragile and may break
   the site's terms.
2. Checks each file against a pinned checksum, so an unknown file is refused.
3. Unpacks with `innoextract`, never running the installers. Linux gets it from
   `apt`; the Windows zip bundles the binary, with its licence.
4. Builds a game folder as `MOD-COMPAT.md` describes: a copy of the stock
   folder, then the Omnitech files on top, names lower-cased, then the packs.
   Both Carver V packs replace stock-named missions, so each gets a folder.
5. Copies in the engine and leaves a launcher. With `--quest`, pushes the folder
   to the headset over `adb` (about 1.8 GB for Omnitech and the stock data).

Better still, **ask the Omnitech team** (magicX, through ModDB) for permission to
offer a prepared pack. With it, Omnitech becomes one more download and works on
the Quest without a PC script. Any approach to the team is the project owner's
to make.

In game, a mod picker would be the eventual goal; until then each mod is its
own folder and launcher.

## Order of work

| Step | What | Done when |
|---|---|---|
| 1 | `gamedata` job; full Windows, Linux and Pi packages; licence files; `INSTALL.md` rewritten around them | Package smoke run on the unpacked full Linux package; Windows zip run by hand on a clean machine |
| 2 | Full Quest APK: data and shaders in assets, first-start copy with progress, keeps saves and options | Clean install and an update on a Quest 2 |
| 3 | `mc2-omnitech` on Linux, then Windows, then `--quest` | Fresh machine to a Clan Wars mission; `scripts/walk_campaigns.sh` passes on the result |
| 4 | Ask the Omnitech team; if yes, a prepared pack | Their answer in `MOD-COMPAT.md` |

Steps 1 and 2 are independent. Step 3 needs step 1's folder layout.

## Verification

- CI: the existing `scripts/smoke.sh` against the unpacked full Linux and Pi
  packages (`RELEASES.md` phase 4), so what players download is what is tested.
- By hand, in `HARDWARE-TESTS.md`: Windows on a machine with nothing installed;
  a Quest install and update; the Omnitech helper from an empty folder.

## Open questions

- Movies in the full packages: about 180 MB more for the briefing videos, and
  are Git LFS quotas on `alariq/mc2srcdata` acceptable for one job per release?
- Are three desktop full packages worth it, against one shared data zip that
  people unpack over an engine package? The shared zip saves storage only;
  GitHub does not charge for either, and one download is easier.
- Does `alariq`, whose repo holds the data, mind prebuilt packs elsewhere? It
  is allowed by the licence; a courtesy note is the owner's call.
