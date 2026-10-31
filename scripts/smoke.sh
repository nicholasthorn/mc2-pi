#!/bin/bash
# Smoke test: launch missions, let each run a fixed number of frames, and fail
# on a crash, a hang, a non-zero exit, GL errors, shader errors or STOP lines.
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
#   --timeout S       seconds before a run counts as hung (default: 300)
#   --log-dir DIR     where to keep per-mission logs (default: ./smoke-logs)
#   --display         run on the current display instead of Xvfb (to watch)
#
# Exit status: 0 if every mission passed, 1 otherwise.

set -u

GAME_DIR=""
BUILD_DIR=""
MISSIONS="mc2_01 mc2_03"
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
        --timeout)   TIMEOUT="$2"; shift 2 ;;
        --log-dir)   LOG_DIR="$2"; shift 2 ;;
        --display)   USE_DISPLAY=1; shift ;;
        -h|--help)   sed -n '2,22p' "$0"; exit 0 ;;
        *) echo "smoke: unknown option $1" >&2; exit 2 ;;
    esac
done

if [ -z "$GAME_DIR" ] || [ ! -d "$GAME_DIR/data" ]; then
    echo "smoke: --game-dir must point at built game data (no data/ in '$GAME_DIR')" >&2
    exit 2
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
BAD='OpenGL Error|: ERROR: |^STOP: |FATAL|runtime error: |ERROR: AddressSanitizer|ERROR: LeakSanitizer'

failed=0
for m in $MISSIONS; do
    log="$LOG_DIR/$m.log"
    start=$(date +%s)
    ( cd "$GAME_DIR" && "${RUN[@]}" timeout -s KILL "$TIMEOUT" ./mc2 -mission "$m" -autoexit "$FRAMES" ) > "$log" 2>&1
    rc=$?
    secs=$(( $(date +%s) - start ))

    reasons=()
    [ $rc -eq 137 ] && reasons+=("hung (killed after ${TIMEOUT}s)")
    [ $rc -ne 0 ] && [ $rc -ne 137 ] && reasons+=("exit status $rc")
    grep -q '^\[AUTOEXIT\]' "$log" || reasons+=("never reached $FRAMES frames")
    # code/mission.cpp prints [MISSION] on the mission's first frame; it must
    # come before [AUTOEXIT] or the frames were spent in menus / loading.
    mline=$(grep -n -m1 '^\[MISSION\]' "$log" | cut -d: -f1)
    aline=$(grep -n -m1 '^\[AUTOEXIT\]' "$log" | cut -d: -f1)
    if [ -z "$mline" ] || { [ -n "$aline" ] && [ "$mline" -gt "$aline" ]; }; then
        reasons+=("mission never started")
    fi
    nbad=$(grep -cE "$BAD" "$log")
    [ "$nbad" -gt 0 ] && reasons+=("$nbad error line(s)")

    if [ ${#reasons[@]} -eq 0 ]; then
        echo "PASS $m (${secs}s) $(grep -m1 '^\[AUTOEXIT\]' "$log")"
    else
        failed=1
        echo "FAIL $m (${secs}s): $(printf '%s; ' "${reasons[@]}")"
        grep -E "$BAD" "$log" | sort | uniq -c | sort -rn | head -10 | sed 's/^/    /'
        echo "    log: $log"
    fi
done

exit $failed
