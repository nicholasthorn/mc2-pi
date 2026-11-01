#!/bin/bash
# Run the mc2_19 bench scenario on a Meta Quest over adb with the test APK
# (scripts/android/build.sh): install, push the game data, run mono and then
# -stereo, pull mc2.log and the frame CSV of each run, print the [BENCH] lines.
# Runs in Git Bash on Windows or bash on Linux. See android/README.md.
#
# usage: scripts/android/run_bench.sh --data DIR [options]
#   --data DIR       built game data (scripts/build_gamedata.sh --no-movies);
#                    pushed once, later runs only push what changed
#   --frames N       gameplay frames per run (default: 3600, the scenario's)
#   --stereo SEP     eye separation for the stereo run (default: 40; 0 = skip)
#   --out DIR        where logs and CSVs land (default: ./quest-bench)
#   --timeout S      seconds before a run counts as hung (default: 1800)
#   --skip-install   do not reinstall the APK
set -e

. "$(dirname "$0")/env.sh"
DATA=""; FRAMES=3600; STEREO=40; OUT="$PWD/quest-bench"; TIMEOUT=1800; INSTALL=1
while [ $# -gt 0 ]; do
    case "$1" in
        --data) DATA="$2"; shift 2 ;;
        --frames) FRAMES="$2"; shift 2 ;;
        --stereo) STEREO="$2"; shift 2 ;;
        --out) OUT="$2"; shift 2 ;;
        --timeout) TIMEOUT="$2"; shift 2 ;;
        --skip-install) INSTALL=0; shift ;;
        -h|--help) sed -n '2,/^set -e/p' "$0"; exit 0 ;;
        *) echo "run_bench: unknown option $1" >&2; exit 2 ;;
    esac
done
[ -d "$DATA/data" ] || { echo "run_bench: --data must be a built game folder (with data/)" >&2; exit 2; }
DATA="$(native "$DATA")"; OUT="$(native "$OUT")"

ADB="$ANDROID_HOME/platform-tools/adb"
# Git Bash would rewrite /sdcard/... into a Windows path for a native exe,
# so local paths are passed in native form (C:/...) instead
export MSYS_NO_PATHCONV=1
D=/sdcard/Android/data/$PKG/files

[ "$("$ADB" get-state 2>/dev/null)" = device ] || { echo "run_bench: no device; check 'adb devices' (developer mode, USB allowed in the headset)" >&2; exit 2; }
mkdir -p "$OUT"

if [ $INSTALL = 1 ]; then
    [ -f "$APK" ] || { echo "run_bench: build the APK first (scripts/android/build.sh)" >&2; exit 2; }
    "$ADB" install -r "$APK"
fi
"$ADB" shell mkdir -p "$D"
echo "run_bench: pushing game data (first time: about 700 MB)"
"$ADB" push --sync "$DATA/." "$D/" > /dev/null
"$ADB" push --sync "$ROOT/shaders/." "$D/shaders/" > /dev/null
"$ADB" push "$ROOT/bench/mc2_19.fit" "$D/bench_scenario.fit" > /dev/null
"$ADB" shell cp "$D/minprefs.cfg" "$D/options.cfg"

run() {   # label, extra game arguments...
    local label=$1; shift
    local args="$OUT/mc2args-$label.txt"
    {
        echo "# written by scripts/android/run_bench.sh"
        echo "MC2_VSYNC=0 MC2_STOP_EXITS=1"
        echo "-mission mc2_19 -autoexit $FRAMES -bench $label.csv -benchscenario bench_scenario.fit $*"
    } > "$args"
    "$ADB" push "$args" "$D/mc2args.txt" > /dev/null
    "$ADB" shell rm -f "$D/mc2.log" "$D/$label.csv"
    "$ADB" shell am force-stop $PKG
    "$ADB" shell am start -n $PKG/.MC2Activity > /dev/null
    echo "run_bench: $label running (put the headset on, or keep it awake)"
    local waited=0
    while true; do
        sleep 10; waited=$((waited + 10))
        if "$ADB" shell grep -q "segment=all" "$D/mc2.log" 2>/dev/null; then break; fi
        if [ -z "$("$ADB" shell pidof $PKG 2>/dev/null)" ]; then
            echo "run_bench: $label: the app exited without a [BENCH] line" >&2; break
        fi
        if [ $waited -ge "$TIMEOUT" ]; then
            echo "run_bench: $label: no result after $TIMEOUT s" >&2; break
        fi
    done
    sleep 2
    "$ADB" pull "$D/mc2.log" "$OUT/$label.log" > /dev/null 2>&1 || true
    "$ADB" pull "$D/$label.csv" "$OUT/$label.csv" > /dev/null 2>&1 || true
    "$ADB" shell am force-stop $PKG
    local log="$OUT/$label.log"
    if [ ! -s "$log" ]; then
        echo "run_bench: $label: FAIL, no log; 'adb logcat -d' shows a crash" >&2; return
    fi
    grep "\[BENCH\]\|\[MISSION\]\|STOP: \|FATAL" "$log" || true
    # the same reasons scripts/bench.sh fails a run for
    local reasons=()
    grep -q "\[MISSION\]" "$log" || reasons+=("mission never started")
    grep -q "\[BENCH\] segment=all" "$log" || reasons+=("no [BENCH] line")
    grep -q "\[BENCH\] ERROR" "$log" && reasons+=("[BENCH] ERROR")
    grep -q "STOP: \|FATAL" "$log" && reasons+=("STOP or FATAL")
    if [ ${#reasons[@]} -gt 0 ]; then
        echo "run_bench: $label: FAIL (${reasons[*]})" >&2
    else
        echo "run_bench: $label: ok"
    fi
}

run mono
[ "$STEREO" != 0 ] && run stereo -stereo "$STEREO"
echo "run_bench: logs and CSVs in $OUT"
