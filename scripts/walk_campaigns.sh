#!/bin/bash
# UI walk of each MechCommander Omnitech add-on campaign in a game folder: its
# first mission from the new campaign dialog through purchase, the mech lab
# (swap a variant, swap back, Accept), pilot, launch, a win and the results,
# back to mission selection. The walk differs per campaign, as its campaign
# file sets logistics, mission selection, salvage and pilot review: that table
# is below. Run by hand: CI has no Omnitech data.
#
# usage: scripts/walk_campaigns.sh --game-dir DIR [--log-dir DIR]
#   --game-dir DIR  Omnitech data with campaign packs installed
#                   (planning/future-features/MOD-COMPAT.md item 13); both
#                   Carver V packs need a folder of their own, so run once
#                   per folder. Campaigns the folder does not have are skipped.
#   --log-dir DIR   where to keep logs (default: ./campaign-walk-logs)
#
# Uses the mc2 in the game folder; copy your build in first. For a sanitized
# run, set ASAN_OPTIONS and UBSAN_OPTIONS as smoke.yml does.
#
# Exit status: 0 if every walk passed, 1 otherwise.

set -u

GAME_DIR=""
LOG_DIR="$PWD/campaign-walk-logs"
SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"

while [ $# -gt 0 ]; do
    case "$1" in
        --game-dir) GAME_DIR="$2"; shift 2 ;;
        --log-dir)  LOG_DIR="$2"; shift 2 ;;
        -h|--help)  sed -n '2,/^$/p' "$0"; exit 0 ;;
        *) echo "walk_campaigns: unknown option $1" >&2; exit 2 ;;
    esac
done

if [ -z "$GAME_DIR" ] || [ ! -d "$GAME_DIR/data/campaign" ]; then
    echo "walk_campaigns: --game-dir must point at game data (no data/campaign in '$GAME_DIR')" >&2
    exit 2
fi

# from the new campaign dialog to the mission, with logistics
LOGISTICS="mainmenu,campaign+pick,missionselection,briefing,mechbay,mechpurchase+buy,mechbay+add"
LOGISTICS="$LOGISTICS,mechlab+swap,mechlab+own,mechlab+accept,mechbay,pilotready+pilot,mission"
# the campaign's first mission launches from mission selection, or at once
SELECTION="mainmenu,campaign+pick,missionselection,mission"
DIRECT="mainmenu,campaign+pick,mission"
# what a win leads to
SALVAGE="salvage,pilotreview,missionselection"
REVIEW="pilotreview,missionselection"

failed=0
ran=0
# file name of the campaign (data/campaign/<name>.fit) | its walk
while IFS='|' read -r name walk; do
    [ -f "$GAME_DIR/data/campaign/$name.fit" ] || continue
    ran=$((ran + 1))
    echo "== $name"
    MC2_UISMOKE_CAMPAIGN="$name" "$SRC_DIR/scripts/smoke.sh" --game-dir "$GAME_DIR" \
        --missions "" --ui "$walk" --log-dir "$LOG_DIR/$name" | grep -E '^(PASS|FAIL)|^    ' \
        || failed=1
    [ "${PIPESTATUS[0]}" = 0 ] || failed=1
done <<EOF
mercenarys star|$LOGISTICS,$SALVAGE
volstand|$LOGISTICS,$SALVAGE
laksos legion|$LOGISTICS,$SALVAGE
desertfox|$LOGISTICS,$SALVAGE
day of heroes|$LOGISTICS,$REVIEW
wolfsdragoons|$LOGISTICS,$REVIEW
clan wars|$LOGISTICS,$REVIEW
mw2mercs|$LOGISTICS,$REVIEW
eagle_2.01|$SELECTION,$SALVAGE
carver v original|$DIRECT,$REVIEW
carver5|$DIRECT,$REVIEW
EOF

echo "$ran campaign(s) walked"
[ "$ran" -gt 0 ] || failed=1
exit $failed
