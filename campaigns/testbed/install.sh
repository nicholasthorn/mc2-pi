#!/bin/bash
# Install the testbed mission (and, with --campaign, the one-group testbed
# campaign) into a built game directory as loose files, which File::open
# finds before mission.fst. See planning/future-features/MISSION-AUTHORING.md.
#
# usage: campaigns/testbed/install.sh --src-data DIR --game-dir DIR [--campaign] [--autoplay]
#   --src-data DIR  alariq/mc2srcdata checkout (the map's stock mission and .pak)
#   --game-dir DIR  built game data (from scripts/build_gamedata.sh); use a copy:
#                   --campaign replaces its data/campaign/campaign.fit
#   --campaign      also install campaign.fit (the stock one is moved aside)
#   --autoplay      test only: the player's units run tb_autoplay instead of
#                   PBrain and play the objectives themselves, so a headless
#                   run (smoke.sh --frames N) walks the objective chain
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
SRC_DATA=""
GAME_DIR=""
CAMPAIGN=0
AUTOPLAY=0
MAP_MISSION="mc2_01"   # the stock mission whose map tb_01 reuses
NAME="tb_01"

while [ $# -gt 0 ]; do
    case "$1" in
        --src-data) SRC_DATA="$2"; shift 2 ;;
        --game-dir) GAME_DIR="$2"; shift 2 ;;
        --campaign) CAMPAIGN=1; shift ;;
        --autoplay) AUTOPLAY=1; shift ;;
        -h|--help)  sed -n '2,/^set/p' "$0" | sed '$d'; exit 0 ;;
        *) echo "install: unknown option $1" >&2; exit 2 ;;
    esac
done
[ -d "$SRC_DATA/missions" ] || { echo "install: no missions/ in --src-data '$SRC_DATA'" >&2; exit 2; }
[ -d "$GAME_DIR/data" ]     || { echo "install: no data/ in --game-dir '$GAME_DIR'" >&2; exit 2; }

MISSIONS="$GAME_DIR/data/missions"
mkdir -p "$MISSIONS/warriors" "$GAME_DIR/data/campaign"

# The map: terrain, pathing and buildings live in the .pak, found by mission name.
cp "$SRC_DATA/missions/$MAP_MISSION.pak" "$MISSIONS/$NAME.pak"

# The mission file: the map's own blocks from the stock mission, then the
# authored blocks with their // notes removed (the FIT reader has no comment
# syntax: it searches the raw text for block and key names).
{
    tr -d '\r' < "$SRC_DATA/missions/$MAP_MISSION.fit" | awk '
        /^\[/ { keep = ($0 ~ /^\[(Cameras|Water|Terrain|Weather|TheSky)\]$/ || $0 ~ /^\[Forest[0-9]+\]$/) }
        /^FITini/ && !started { print; started = 1; next }
        keep'
    echo
    if [ "$AUTOPLAY" = 1 ]; then
        grep -v '^//' "$HERE/$NAME.mission.fit" | sed 's/^st Brain = "PBrain"$/st Brain = "tb_autoplay"/'
    else
        grep -v '^//' "$HERE/$NAME.mission.fit"
    fi
    echo "FITend"
} > "$MISSIONS/$NAME.fit"

cp "$HERE/$NAME.abl" "$MISSIONS/"
# Unit brains: system.cfg sets warriorPath to data/missions/warriors/
# (paths.cpp's default, profiles/, is overridden).
cp "$HERE/warriors/"*.abl "$MISSIONS/warriors/"
cp "$HERE/purchase_tb01.fit" "$MISSIONS/"

if [ "$CAMPAIGN" = 1 ]; then
    if [ -f "$GAME_DIR/data/campaign/campaign.fit" ] && [ ! -f "$GAME_DIR/data/campaign/campaign.fit.stock" ]; then
        mv "$GAME_DIR/data/campaign/campaign.fit" "$GAME_DIR/data/campaign/campaign.fit.stock"
    fi
    cp "$HERE/campaign.fit" "$GAME_DIR/data/campaign/campaign.fit"
fi

echo "installed $NAME into $GAME_DIR"
