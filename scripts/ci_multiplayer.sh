#!/bin/bash
# CI gate: does this PR change what a network game does? Prints
# "multiplayer=true" or "multiplayer=false". On true, smoke.yml runs every
# multiplayer step on all three configurations, as it does nightly. On false it
# leaves out the game modes (about 10 minutes a job) and runs the rest of the
# multiplayer steps only on the sanitized job.
#
# Looks at the PR's own changes (merge base to HEAD), so merging the base branch
# in does not count. True when it cannot tell: no merge base or an empty diff.
#
# What is on the list:
#   - the net layer, and the lobby stubs that will sit on it
#   - what drives the tests: the scenario director and the bench files, the
#     scripts, this gate and the workflow
#   - the game files where host and client part ways, taken as those that at
#     least two of the multiplayer PRs (119, 123, 126) had to change
#
# A change to anything else that breaks a network game is found by the nightly
# run. If a nightly failure shows a file should be here, add it.
#
# usage: scripts/ci_multiplayer.sh BASE HEAD

base="$1"
head="$2"

MULTIPLAYER='^(code/(multplyr|dmultplyr|net[a-z]*|mp[a-z]*)\.(cpp|h)'
MULTIPLAYER+='|code/(benchdirector|benchscenario)\.(cpp|h)'
MULTIPLAYER+='|code/(tacordr|turret)\.(cpp|h)'
MULTIPLAYER+='|code/(gate\.h|gvehicl\.cpp|mech\.cpp|mover\.cpp|gameobj\.cpp|bldng\.cpp|mechcmd2\.cpp)'
MULTIPLAYER+='|GameOS/gameos/gos_net[a-z_]*\.(cpp|h)'
MULTIPLAYER+='|cmake/mc2_gns[a-z_]*\.cmake'
MULTIPLAYER+='|bench/net[a-z0-9_]*\.fit'
MULTIPLAYER+='|scripts/(net_smoke\.sh|ci_multiplayer\.sh)'
MULTIPLAYER+='|\.github/workflows/smoke\.yml)$'

answer() { echo "multiplayer=$1"; exit 0; }

mb=$(git merge-base "$base" "$head" 2>/dev/null) || answer true
# --no-renames: a file moved off the list still counts, under its old name.
changed=$(git diff --name-only --no-renames "$mb" "$head" 2>/dev/null) || answer true
[ -n "$changed" ] || answer true
if grep -qE "$MULTIPLAYER" <<<"$changed"; then
    answer true
fi
answer false
