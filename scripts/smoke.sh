#!/bin/bash
# Smoke test: launch missions, let each run a fixed number of frames, and fail
# on a crash, a hang, a non-zero exit, GL errors, shader errors or STOP lines.
# Then walk the front-end screens (main menu, options, campaign logistics)
# with -uismoke, under the same failure rules.
#
# Runs headless (Xvfb) even when a display is available: WSLg sets one, and
# a run there opens a window that takes the desktop's mouse and keyboard.
# Rendering is Mesa's software rasterizer (llvmpipe), which supports desktop
# GL 4.5 and GLES 3.2, so the same script covers MC2_GLES=OFF and =ON builds.
# Frame times from llvmpipe say nothing about the Pi: this checks "does it
# run", not "how fast".
#
# usage: scripts/smoke.sh --game-dir DIR [options]
#   --game-dir DIR    built game data (data/, assets/, *.fst, *.cfg); required
#   --build-dir DIR   copy mc2, libmc2res_64.so and shaders/ from this build
#                     into the game dir first (default: use what is there)
#   --missions "..."  missions to run (default: mc2_01 mc2_03)
#   --frames N        frames per mission before -autoexit (default: 600)
#   --ui "..."        front-end walks to run, space separated; each is a
#                     comma-separated list of screens for -uismoke, each
#                     optionally "<screen>+<action>" (names in
#                     code/uismokewalk.cpp). "" skips them. Default: the two
#                     walks below (walk 1 for MechCommander Omnitech data
#                     when the game dir has it)
#   --timeout S       seconds before a run counts as hung (default: 300)
#   --log-dir DIR     where to keep per-run logs (default: ./smoke-logs)
#   --display         run on the current display instead of Xvfb (to watch)
#
# Exit status: 0 if every run passed, 1 otherwise.

set -u

# 1: the four options tabs, then a new campaign (its first mission, mc2_01,
#    skips logistics and loads at once). The walk wins it (the driver's
#    win, not a fight): pilot review (mc2_01 skips salvage), mission
#    selection, launch mc2_02 and win it too: salvage, a pilot promotion
#    (pick a skill), pilot review, back to mission selection.
# 2: a single mission (mc2_02), which starts with no mechs: buy one, put it
#    in the force, open the mech lab (Cancel), back to the briefing, give
#    the mech a pilot and launch.
UI_OPTIONS="mainmenu,options-graphics,options-audio,options-gameplay,options-hotkeys,mainmenu"
UI_OPTIONS_MCO="$UI_OPTIONS"
UI_OPTIONS="$UI_OPTIONS,campaign,mission,pilotreview,missionselection"
UI_OPTIONS="$UI_OPTIONS,briefing,mechbay,pilotready,mission"
UI_OPTIONS="$UI_OPTIONS,salvage,promotion+skill,pilotreview,missionselection"
# MechCommander Omnitech's new campaign opens mission selection instead, with
# no mechs: buy one, put it in the force, give it a pilot, launch, win:
# salvage, pilot review, mission selection.
UI_OPTIONS_MCO="$UI_OPTIONS_MCO,campaign,missionselection,briefing,mechbay"
UI_OPTIONS_MCO="$UI_OPTIONS_MCO,mechpurchase+buy,mechbay+add,pilotready+pilot,mission"
UI_OPTIONS_MCO="$UI_OPTIONS_MCO,salvage,pilotreview,missionselection"
UI_LOGISTICS="mainmenu,singlemission,briefing,mechbay,mechpurchase+buy,mechbay+add,mechlab"
UI_LOGISTICS="$UI_LOGISTICS,mechbay,briefing,mechbay,pilotready+pilot,mission"

GAME_DIR=""
BUILD_DIR=""
MISSIONS="mc2_01 mc2_03"
UI_WALKS=""
UI_SET=0
FRAMES=600
TIMEOUT=300
LOG_DIR="$PWD/smoke-logs"
USE_DISPLAY=0
SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"

while [ $# -gt 0 ]; do
    case "$1" in
        --game-dir)  GAME_DIR="$2"; shift 2 ;;
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --missions)  MISSIONS="$2"; shift 2 ;;
        --frames)    FRAMES="$2"; shift 2 ;;
        --ui)        UI_WALKS="$2"; UI_SET=1; shift 2 ;;
        --timeout)   TIMEOUT="$2"; shift 2 ;;
        --log-dir)   LOG_DIR="$2"; shift 2 ;;
        --display)   USE_DISPLAY=1; shift ;;
        -h|--help)   sed -n '2,/^$/p' "$0"; exit 0 ;;
        *) echo "smoke: unknown option $1" >&2; exit 2 ;;
    esac
done

if [ -z "$GAME_DIR" ] || [ ! -d "$GAME_DIR/data" ]; then
    echo "smoke: --game-dir must point at built game data (no data/ in '$GAME_DIR')" >&2
    exit 2
fi

# Omnitech ships its front-end layouts as loose files, each with a
# [HiresOffsets] block that no stock layout has
if [ "$UI_SET" = 0 ]; then
    if grep -qs HiresOffsets "$GAME_DIR/data/art/mcl_mm.fit"; then
        UI_WALKS="$UI_OPTIONS_MCO $UI_LOGISTICS"
    else
        UI_WALKS="$UI_OPTIONS $UI_LOGISTICS"
    fi
fi

if [ -n "$BUILD_DIR" ]; then
    cp "$BUILD_DIR/mc2" "$BUILD_DIR/out/res/libmc2res_64.so" "$GAME_DIR/" || exit 2
    rm -rf "$GAME_DIR/shaders" && cp -r "$SRC_DIR/shaders" "$GAME_DIR/" || exit 2
fi

# Without options.cfg the game's first launch only runs the hardware "sniffer"
# and exits (code/mechcmd2.cpp). Do what that launch does to the config
# (minprefs.cfg -> options.cfg / orgprefs.cfg) so every smoke run plays the
# mission, with fixed minimum graphics settings.
if [ ! -f "$GAME_DIR/options.cfg" ]; then
    cp "$GAME_DIR/minprefs.cfg" "$GAME_DIR/options.cfg" || exit 2
    [ -f "$GAME_DIR/orgprefs.cfg" ] || cp "$GAME_DIR/minprefs.cfg" "$GAME_DIR/orgprefs.cfg"
fi

mkdir -p "$LOG_DIR"
LOG_DIR="$(cd "$LOG_DIR" && pwd)"

export LIBGL_ALWAYS_SOFTWARE=1
export SDL_AUDIODRIVER=dummy
# a STOP (Fatal, Assert, ABL fatal) ends the run: the default, set here so
# an MC2_STOP_EXITS=0 left in the environment cannot keep a broken run going
export MC2_STOP_EXITS=1

RUN=()
if [ "$USE_DISPLAY" = 1 ]; then
    if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
        echo "smoke: --display given but there is no display" >&2
        exit 2
    fi
else
    if ! command -v xvfb-run >/dev/null; then
        echo "smoke: xvfb-run not installed (or pass --display to use the current display)" >&2
        exit 2
    fi
    # xvfb-run sets DISPLAY; drop a Wayland session (WSLg) so SDL cannot
    # pick it instead of the X server
    unset WAYLAND_DISPLAY
    export SDL_VIDEODRIVER=x11
    # The game's resolution (800x600, minprefs.cfg). -autoexit runs are
    # windowed at that size anyway (FullScreen is ignored), so the screen
    # size only has to be big enough.
    RUN=(xvfb-run -a -s "-screen 0 800x600x24")
fi

# Log lines that mean something went wrong even if the game kept running.
# STOP is not anchored: stderr is unbuffered and stdout is not, so a STOP
# can land in the middle of a line of buffered stdout output.
BAD='OpenGL Error|: ERROR: |STOP: |FATAL|runtime error: |ERROR: AddressSanitizer|ERROR: LeakSanitizer'

failed=0

# run_game LOG ARGS...: run ./mc2 ARGS in the game dir, output to LOG. Sets
# secs, and starts reasons (why the run failed) with the exit status.
run_game() {
    local log="$1"; shift
    local start
    start=$(date +%s)
    ( cd "$GAME_DIR" && "${RUN[@]}" timeout -s KILL "$TIMEOUT" ./mc2 "$@" ) > "$log" 2>&1
    local rc=$?
    secs=$(( $(date +%s) - start ))
    reasons=()
    [ $rc -eq 137 ] && reasons+=("hung (killed after ${TIMEOUT}s)")
    [ $rc -ne 0 ] && [ $rc -ne 137 ] && reasons+=("exit status $rc")
}

# report NAME LOG SUMMARY: add error lines to reasons, print PASS or FAIL
report() {
    local name="$1" log="$2" summary="$3"
    local nbad
    nbad=$(grep -cE "$BAD" "$log")
    [ "$nbad" -gt 0 ] && reasons+=("$nbad error line(s)")

    if [ ${#reasons[@]} -eq 0 ]; then
        echo "PASS $name (${secs}s) $summary"
    else
        failed=1
        echo "FAIL $name (${secs}s): $(printf '%s; ' "${reasons[@]}")"
        grep -E "$BAD" "$log" | sort | uniq -c | sort -rn | head -10 | sed 's/^/    /'
        echo "    log: $log"
    fi
}

for m in $MISSIONS; do
    log="$LOG_DIR/$m.log"
    run_game "$log" -mission "$m" -autoexit "$FRAMES"
    grep -q '^\[AUTOEXIT\]' "$log" || reasons+=("never reached $FRAMES frames")
    # code/mission.cpp prints [MISSION] on the mission's first frame; it must
    # come before [AUTOEXIT] or the frames were spent in menus / loading.
    mline=$(grep -n -m1 '^\[MISSION\]' "$log" | cut -d: -f1)
    aline=$(grep -n -m1 '^\[AUTOEXIT\]' "$log" | cut -d: -f1)
    if [ -z "$mline" ] || { [ -n "$aline" ] && [ "$mline" -gt "$aline" ]; }; then
        reasons+=("mission never started")
    fi
    report "$m" "$log" "$(grep -m1 '^\[AUTOEXIT\]' "$log")"
done

# Front-end walks: code/uismokedriver.cpp prints "[SCREEN] <name> shown" as
# each screen settles, then "[UISMOKE] done" (or "[UISMOKE] FAIL: <why>")
# and quits. Each walk gets a fresh config directory (XDG_CONFIG_HOME) holding
# only tests/smoke/campaign-save.fit, so the "Single Mission" list has a
# mission in it, and nothing the walk saves reaches the real one.
# The path must be lower case: the game lowercases the save path
# (FullPathFileName), so a TMPDIR with capitals falls back to /tmp.
CFG_BASE="${TMPDIR:-/tmp}"
case "$CFG_BASE" in *[A-Z]*) CFG_BASE=/tmp ;; esac
n=0
for w in $UI_WALKS; do
    n=$((n + 1))
    log="$LOG_DIR/ui$n.log"
    cfg="$CFG_BASE/mc2-uismoke-$$-$n"
    rm -rf "$cfg"
    if ! mkdir -p "$cfg/.mechcommander2/savegame" \
            || ! cp "$SRC_DIR/tests/smoke/campaign-save.fit" "$cfg/.mechcommander2/savegame/"; then
        failed=1
        echo "FAIL ui$n: cannot set up a config directory in $CFG_BASE"
        continue
    fi
    XDG_CONFIG_HOME="$cfg" run_game "$log" -uismoke "$w"
    rm -rf "$cfg"
    why=$(grep -m1 '^\[UISMOKE\] FAIL' "$log" | sed 's/^\[UISMOKE\] FAIL: //')
    [ -n "$why" ] && reasons+=("$why")
    grep -q '^\[UISMOKE\] done' "$log" || [ -n "$why" ] || reasons+=("walk never finished")
    # every screen of the walk, in order
    # ("<screen>+<action>" steps show as "<screen>"; the driver fails the walk
    # itself if an action's button cannot be pressed)
    shown=$(sed -n 's/^\[SCREEN\] \(.*\) shown$/\1/p' "$log" | paste -sd, -)
    [ "$shown" = "$(printf '%s' "$w" | sed 's/+[a-z]*//g')" ] \
        || reasons+=("screens shown: ${shown:-none}")
    report "ui$n" "$log" "$(grep -m1 '^\[UISMOKE\] done' "$log")"
done

exit $failed
