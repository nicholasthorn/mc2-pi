#!/bin/bash
# Benchmark: run missions for a fixed number of gameplay frames on the GPU
# and/or Mesa's software rasterizer (llvmpipe), vsync off by default, and
# record the frame rate the game reports ([AUTOEXIT]) together with the SoC
# temperature and throttle state during the run.
#
# Missions run with -bench: a fixed game step and random seed and no player
# input, so every run (and every commit) renders the same frames. The game
# writes each run's per-frame timings (game logic / render / swap ms, draw
# calls, triangles) to logs/*.frames.csv and prints a [BENCH] summary with
# percentiles, which goes into results.csv. Same machine, same frames: the
# draws and tris columns of two runs should match frame for frame.
#
# Every run appends a row to results.csv, so runs on different commits,
# settings or days can be compared.
#
# Unlike smoke.sh this is for real performance numbers: run it on the Pi, on
# its own display, with nothing else busy. The GPU renderer needs a real
# display; only cpu runs fall back to Xvfb.
#
# usage: scripts/bench.sh --game-dir DIR [options]
#   --game-dir DIR     built game data (data/, assets/, *.fst, *.cfg); required
#   --build-dir DIR    copy mc2, libmc2res_64.so and shaders/ from this build
#                      into the game dir first (default: use what is there)
#   --missions "..."   missions to run (default: mc2_01 mc2_03)
#   --renderers "..."  gpu, cpu or both (default: "gpu cpu"); cpu = llvmpipe
#   --frames N         gameplay frames per run (default: 3000)
#   --runs N           runs of each mission and renderer (default: 3)
#   --vsync 0|1        MC2_VSYNC for the runs (default: 0, off)
#   --cool-to C        before each run, wait (up to 5 min) until the SoC is at
#                      or below C degrees; 0 = don't wait (default: 60)
#   --timeout S        seconds before a run counts as hung (default: 600)
#   --out DIR          logs and results.csv (default: ./bench-results)
#   --label TEXT       note stored with every row, e.g. "fan on, 1080p"
#   --scenario FILE    play a scripted battle (bench/*.fit) instead of the
#                      mission as it stands: units gathered into an arena and
#                      fighting, a camera path, artillery, named segments. Its
#                      Mission and Frames are the defaults for --missions and
#                      --frames. Each segment gets a row per run in
#                      segments.csv, and the screenshots it asks for are saved
#                      as logs/*.frames_<segment>.tga.
#   --fov DEG          force the camera's field of view (-benchfov; the game
#                      uses 40, a headset about 100). Name it in --label: the
#                      per-stage times (<stage>_ms, bench/README.md) are in
#                      each run's log and frames CSV, not in results.csv.
#   --perf             after the timed runs, one extra run per mission and
#                      renderer under "perf record -g", sampling gameplay
#                      frames only (the game switches it on and off). Its fps
#                      is not recorded: sampling slows the game. Writes
#                      logs/*.perf.data and *.perf.txt (top symbols, and time
#                      per library: game, GL driver, kernel), plus folded
#                      stacks and a flame graph SVG when FlameGraph's
#                      stackcollapse-perf.pl / flamegraph.pl are on PATH.
#                      --runs 0 --perf profiles without timed runs.
#   --perf-freq N      samples per second (default: 999)
#   --call-graph M     fp (default; build with -DMC2_FRAME_POINTERS=ON or the
#                      stacks are cut short; stacks also stop inside system
#                      libraries such as zlib and Mesa, built without frame
#                      pointers) or dwarf (any build, walks through those
#                      libraries, much bigger perf.data)
#
# For readable GL driver frames on the Pi, install Mesa's debug symbols.
# Compare two profiles of the same mission with "perf diff A.perf.data B.perf.data".
#
# Throttle flags in the results are the "now" bits of vcgencmd get_throttled
# seen at any point during the run: 1 under-voltage, 2 ARM frequency capped,
# 4 throttled, 8 soft temperature limit. Anything but 0 means the fps number
# is lower than the hardware can sustain when cool and well powered.
#
# Exit status: 0 if every run passed, 1 otherwise.

set -u

GAME_DIR=""
BUILD_DIR=""
MISSIONS="mc2_01 mc2_03"
RENDERERS="gpu cpu"
FRAMES=3000
RUNS=3
VSYNC=0
COOL_TO=60
TIMEOUT=600
OUT_DIR="$PWD/bench-results"
LABEL=""
PERF=0
PERF_FREQ=999
CALL_GRAPH=fp
SCENARIO=""
FOV=""
MISSIONS_SET=0
FRAMES_SET=0
SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"

while [ $# -gt 0 ]; do
    case "$1" in
        --game-dir)  GAME_DIR="$2"; shift 2 ;;
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --missions)  MISSIONS="$2"; MISSIONS_SET=1; shift 2 ;;
        --renderers) RENDERERS="$2"; shift 2 ;;
        --frames)    FRAMES="$2"; FRAMES_SET=1; shift 2 ;;
        --scenario)  SCENARIO="$2"; shift 2 ;;
        --fov)       FOV="$2"; shift 2 ;;
        --runs)      RUNS="$2"; shift 2 ;;
        --vsync)     VSYNC="$2"; shift 2 ;;
        --cool-to)   COOL_TO="$2"; shift 2 ;;
        --timeout)   TIMEOUT="$2"; shift 2 ;;
        --out)       OUT_DIR="$2"; shift 2 ;;
        --label)     LABEL="$2"; shift 2 ;;
        --perf)      PERF=1; shift ;;
        --perf-freq) PERF_FREQ="$2"; shift 2 ;;
        --call-graph) CALL_GRAPH="$2"; shift 2 ;;
        -h|--help)   sed -n '2,/^# Exit status/p' "$0"; exit 0 ;;
        *) echo "bench: unknown option $1" >&2; exit 2 ;;
    esac
done

if [ -z "$GAME_DIR" ] || [ ! -d "$GAME_DIR/data" ]; then
    echo "bench: --game-dir must point at built game data (no data/ in '$GAME_DIR')" >&2
    exit 2
fi
for r in $RENDERERS; do
    case "$r" in
        gpu|cpu) ;;
        *) echo "bench: unknown renderer '$r' (gpu or cpu)" >&2; exit 2 ;;
    esac
done
if [ $PERF = 1 ]; then
    case "$CALL_GRAPH" in
        fp|dwarf) ;;
        *) echo "bench: --call-graph must be fp or dwarf" >&2; exit 2 ;;
    esac
    if ! command -v perf >/dev/null; then
        echo "bench: --perf needs perf (Pi OS / Debian: apt install linux-perf)" >&2
        exit 2
    fi
    # 3 (Debian's default) forbids perf for normal users; 2 is enough here
    paranoid=$(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo 0)
    if [ "$paranoid" -gt 2 ] && [ "$(id -u)" != 0 ]; then
        echo "bench: perf_event_paranoid is $paranoid; run: sudo sysctl kernel.perf_event_paranoid=2" >&2
        exit 2
    fi
    # e.g. WSL's perf: profiles fine, but every frame shows as [unknown]
    if perf version --build-options 2>/dev/null | grep -q 'libelf: \[ OFF \]'; then
        echo "bench: warning: this perf has no libelf, so symbols will show as [unknown]"
    fi
fi

if [ -n "$BUILD_DIR" ]; then
    cp "$BUILD_DIR/mc2" "$BUILD_DIR/out/res/libmc2res_64.so" "$GAME_DIR/" || exit 2
    rm -rf "$GAME_DIR/shaders" && cp -r "$SRC_DIR/shaders" "$GAME_DIR/" || exit 2
fi

# The scenario is copied into the game dir under a fixed lower-case name and
# passed relative: File::open lower-cases the whole path, so an absolute one
# with upper-case directories would not open on Linux.
SCENARIO_ARGS=()
SCENARIO_NAME=""
if [ -n "$SCENARIO" ]; then
    if [ ! -f "$SCENARIO" ]; then
        echo "bench: no scenario file '$SCENARIO'" >&2
        exit 2
    fi
    cp "$SCENARIO" "$GAME_DIR/bench_scenario.fit" || exit 2
    SCENARIO_ARGS=(-benchscenario bench_scenario.fit)
    SCENARIO_NAME=$(basename "$SCENARIO" .fit)
    m=$(sed -n 's/^st Mission = "\(.*\)".*/\1/p' "$SCENARIO" | head -1)
    f=$(sed -n 's/^l Frames = \([0-9]*\).*/\1/p' "$SCENARIO" | head -1)
    [ $MISSIONS_SET = 0 ] && [ -n "$m" ] && MISSIONS="$m"
    [ $FRAMES_SET = 0 ] && [ -n "$f" ] && FRAMES="$f"
fi
if [ -n "$FOV" ]; then
    if ! [[ "$FOV" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
        echo "bench: --fov wants degrees as a number, got '$FOV'" >&2
        exit 2
    fi
    SCENARIO_ARGS+=(-benchfov "$FOV")
fi

# Same first-launch config step as smoke.sh (see there). The settings in
# options.cfg change the numbers, so its hash goes into every result row.
if [ ! -f "$GAME_DIR/options.cfg" ]; then
    cp "$GAME_DIR/minprefs.cfg" "$GAME_DIR/options.cfg" || exit 2
    [ -f "$GAME_DIR/orgprefs.cfg" ] || cp "$GAME_DIR/minprefs.cfg" "$GAME_DIR/orgprefs.cfg"
fi
CFG_HASH=$(md5sum "$GAME_DIR/options.cfg" | cut -c1-8)

mkdir -p "$OUT_DIR/logs"
OUT_DIR="$(cd "$OUT_DIR" && pwd)"
RESULTS="$OUT_DIR/results.csv"
HEADER="date,commit,label,host,mission,renderer,display,vsync,run,frames,seconds,fps,gl_renderer,options_cfg,governor,max_temp_c,throttle_flags,status,avg_ms,p50_ms,p95_ms,p99_ms,max_ms,logic_ms,render_ms,swap_ms,draws,tris"
# Results from before -bench (no timing columns) are not comparable: set them aside.
if [ -f "$RESULTS" ] && [ "$(head -1 "$RESULTS")" != "$HEADER" ]; then
    mv "$RESULTS" "$OUT_DIR/results-old-$(date +%Y%m%d-%H%M%S).csv"
    echo "bench: results.csv had other columns; moved it aside"
fi
[ -f "$RESULTS" ] || echo "$HEADER" > "$RESULTS"
SEGMENTS="$OUT_DIR/segments.csv"
BENCH_KEYS="frames fps avg_ms p50_ms p95_ms p99_ms max_ms logic_ms render_ms swap_ms draws tris"
# status is the whole run's: a FAIL run's segment numbers are not results
SEG_HEADER="date,commit,label,host,scenario,mission,renderer,display,vsync,run,options_cfg,status,segment,${BENCH_KEYS// /,}"
if [ -f "$SEGMENTS" ] && [ "$(head -1 "$SEGMENTS")" != "$SEG_HEADER" ]; then
    mv "$SEGMENTS" "$OUT_DIR/segments-old-$(date +%Y%m%d-%H%M%S).csv"
    echo "bench: segments.csv had other columns; moved it aside"
fi
[ -f "$SEGMENTS" ] || echo "$SEG_HEADER" > "$SEGMENTS"

COMMIT=$(git -C "$SRC_DIR" rev-parse --short HEAD 2>/dev/null || echo unknown)
git -C "$SRC_DIR" diff --quiet HEAD -- 2>/dev/null || COMMIT="$COMMIT-dirty"
HOST=$( { tr -d '\0' < /proc/device-tree/model; } 2>/dev/null || uname -n)
GOVERNOR=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo unknown)
STAMP=$(date +%Y%m%d-%H%M%S)

HAVE_DISPLAY=0
{ [ -n "${DISPLAY:-}" ] || [ -n "${WAYLAND_DISPLAY:-}" ]; } && HAVE_DISPLAY=1

# SoC temperature in degrees C, from vcgencmd on the Pi, else sysfs.
soc_temp() {
    if command -v vcgencmd >/dev/null; then
        vcgencmd measure_temp | sed 's/[^0-9.]//g'
    elif [ -r /sys/class/thermal/thermal_zone0/temp ]; then
        awk '{ printf "%.1f\n", $1 / 1000 }' /sys/class/thermal/thermal_zone0/temp
    fi
}

# Once a second: time, temp, ARM clock (MHz), get_throttled. Runs until killed.
sample_soc() {
    while true; do
        local hz th
        if command -v vcgencmd >/dev/null; then
            hz=$(vcgencmd measure_clock arm | cut -d= -f2)
            th=$(vcgencmd get_throttled | cut -d= -f2)
        else
            hz=$(( $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq 2>/dev/null || echo 0) * 1000 ))
            th=""
        fi
        echo "$(date +%s),$(soc_temp),$(( hz / 1000000 )),$th"
        sleep 1
    done
}

wait_cool() {
    [ "$COOL_TO" = 0 ] && return
    local t waited=0
    t=$(soc_temp)
    [ -z "$t" ] && return
    while awk -v t="$t" -v c="$COOL_TO" 'BEGIN { exit !(t > c) }'; do
        if [ $waited -ge 300 ]; then
            echo "    still ${t}C after 5 min, running anyway"
            return
        fi
        [ $waited -eq 0 ] && echo "    cooling: ${t}C, waiting for ${COOL_TO}C"
        sleep 10
        waited=$(( waited + 10 ))
        t=$(soc_temp)
    done
}

csv_quote() { printf '"%s"' "${1//\"/\'}"; }

# Sets up the environment for renderer $1 (gpu or cpu): RUN (a wrapper such
# as xvfb-run, may be empty) and display. Returns 1, with the reason in
# renderer_error, if it cannot run here.
use_renderer() {
    RUN=()
    display=x11
    renderer_error=""
    [ -n "${WAYLAND_DISPLAY:-}" ] && display=wayland
    if [ "$1" = gpu ]; then
        unset LIBGL_ALWAYS_SOFTWARE
        if [ $HAVE_DISPLAY = 0 ]; then
            renderer_error="the GPU renderer needs a display (Xvfb is software only)"
            return 1
        fi
    else
        export LIBGL_ALWAYS_SOFTWARE=1
        if [ $HAVE_DISPLAY = 0 ]; then
            if ! command -v xvfb-run >/dev/null; then
                echo "bench: no display and xvfb-run not installed" >&2
                exit 2
            fi
            # match the game's 800x600 (minprefs.cfg), as in smoke.sh
            RUN=(xvfb-run -a -s "-screen 0 800x600x24")
            display=xvfb
        fi
    fi
}

export SDL_AUDIODRIVER=dummy
# a STOP (Fatal, Assert, ABL fatal) ends the run: the default, set here so
# an MC2_STOP_EXITS=0 left in the environment cannot keep a broken run going
export MC2_STOP_EXITS=1
export MC2_VSYNC="$VSYNC"

echo "bench: $HOST, commit $COMMIT, governor $GOVERNOR, options.cfg $CFG_HASH, vsync $VSYNC"
echo "bench: $RUNS run(s) x {$MISSIONS} x {$RENDERERS}, $FRAMES frames each; results: $RESULTS"
[ -n "$SCENARIO" ] && echo "bench: scenario $SCENARIO; per-segment results: $SEGMENTS"

SUMMARY=$(mktemp)
FIFO_DIR=""
trap 'rm -f "$SUMMARY"; [ -n "$FIFO_DIR" ] && rm -rf "$FIFO_DIR"' EXIT
failed=0

# Runs interleave missions and renderers so slow drift (heat, background
# jobs) spreads across all of them instead of landing on the last one.
for run in $(seq 1 "$RUNS"); do
    for m in $MISSIONS; do
        for r in $RENDERERS; do
            if ! use_renderer "$r"; then
                echo "FAIL $m $r run $run: $renderer_error"
                failed=1
                continue
            fi

            wait_cool
            base="$OUT_DIR/logs/${STAMP}_${m}_${r}_run${run}"
            log="$base.log"
            soc="$base.soc.csv"
            frames_csv="$base.frames.csv"

            sample_soc > "$soc" &
            sampler=$!
            ( cd "$GAME_DIR" && "${RUN[@]}" timeout -s KILL "$TIMEOUT" ./mc2 -mission "$m" -autoexit "$FRAMES" -bench "$frames_csv" "${SCENARIO_ARGS[@]}" ) > "$log" 2>&1
            rc=$?
            kill $sampler 2>/dev/null
            wait $sampler 2>/dev/null

            # [AUTOEXIT] 3000 gameplay frames in 64.3 s (46.7 fps)
            read -r frames secs fps < <(sed -n 's/^\[AUTOEXIT\] \([0-9]*\) gameplay frames in \([0-9.]*\) s (\([0-9.]*\) fps).*/\1 \2 \3/p' "$log" | head -1)
            # [BENCH] segment=all frames=3000 fps=46.7 avg_ms=21.41 p50_ms=... (key=value)
            bench=$(grep -m1 '^\[BENCH\] segment=all ' "$log")
            bench_field() { sed -n "s/.* $1=\([0-9.]*\).*/\1/p" <<< "$bench"; }
            glr=$(grep -m1 '^Renderer *:' "$log" | sed 's/^Renderer *: *//')
            max_temp=$(awk -F, '$2 != "" && (m == "" || $2 + 0 > m + 0) { m = $2 } END { print m }' "$soc")
            # bash, not awk: Pi OS ships mawk, which has no bitwise operators
            flags=""
            while IFS=, read -r _ _ _ th; do
                [[ "$th" == 0x* ]] && flags=$(( ${flags:-0} | (th & 15) ))
            done < "$soc"

            reasons=()
            [ $rc -eq 137 ] && reasons+=("hung (killed after ${TIMEOUT}s)")
            [ $rc -ne 0 ] && [ $rc -ne 137 ] && reasons+=("exit status $rc")
            [ -z "${fps:-}" ] && reasons+=("no [AUTOEXIT] line")
            [ -z "$bench" ] && reasons+=("no [BENCH] line")
            [ -n "$SCENARIO" ] && grep -q '^\[BENCH\] ERROR' "$log" && reasons+=("scenario error: $(grep -m1 '^\[BENCH\] ERROR' "$log" | sed 's/^\[BENCH\] ERROR: //')")
            mline=$(grep -n -m1 '^\[MISSION\]' "$log" | cut -d: -f1)
            aline=$(grep -n -m1 '^\[AUTOEXIT\]' "$log" | cut -d: -f1)
            if [ -z "$mline" ] || { [ -n "$aline" ] && [ "$mline" -gt "$aline" ]; }; then
                reasons+=("mission never started")
            fi
            # A GPU run that silently fell back to software is not a GPU number.
            [ "$r" = gpu ] && [[ "$glr" == *llvmpipe* || "$glr" == *softpipe* ]] && reasons+=("GPU run used $glr")

            status=PASS
            [ ${#reasons[@]} -gt 0 ] && status=FAIL && failed=1

            {
                printf '%s,%s,' "$(date -Iseconds)" "$COMMIT"
                csv_quote "$LABEL"; printf ','
                csv_quote "$HOST"; printf ','
                printf '%s,%s,%s,%s,%s,%s,%s,%s,' "$m" "$r" "$display" "$VSYNC" "$run" "${frames:-}" "${secs:-}" "${fps:-}"
                csv_quote "$glr"; printf ','
                printf '%s,%s,%s,%s,%s' "$CFG_HASH" "$GOVERNOR" "$max_temp" "$flags" "$status"
                for k in avg_ms p50_ms p95_ms p99_ms max_ms logic_ms render_ms swap_ms draws tris; do
                    printf ',%s' "$(bench_field $k)"
                done
                printf '\n'
            } >> "$RESULTS"

            # one row per scenario segment ([BENCH] segment=<name> ...)
            while read -r line; do
                seg=$(sed -n 's/^\[BENCH\] segment=\([^ ]*\) .*/\1/p' <<< "$line")
                {
                    printf '%s,%s,' "$(date -Iseconds)" "$COMMIT"
                    csv_quote "$LABEL"; printf ','
                    csv_quote "$HOST"; printf ','
                    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s' "$SCENARIO_NAME" "$m" "$r" "$display" "$VSYNC" "$run" "$CFG_HASH" "$status" "$seg"
                    for k in $BENCH_KEYS; do
                        printf ',%s' "$(sed -n "s/.* $k=\([0-9.]*\).*/\1/p" <<< "$line")"
                    done
                    printf '\n'
                } >> "$SEGMENTS"
                [ $status = PASS ] && echo "$m $r $seg $(sed -n 's/.* fps=\([0-9.]*\).*/\1/p' <<< "$line")" >> "$SUMMARY"
            done < <(grep '^\[BENCH\] segment=' "$log" | grep -v '^\[BENCH\] segment=all ')

            note=""
            [ -n "$flags" ] && [ "$flags" != 0 ] && note=" THROTTLED (flags $flags)"
            if [ $status = PASS ]; then
                [ -n "$max_temp" ] && note=", max ${max_temp}C$note"
                echo "PASS $m $r run $run: ${fps} fps (${frames} frames in ${secs} s)$note"
                echo "$m $r all $fps" >> "$SUMMARY"
            else
                echo "FAIL $m $r run $run: $(printf '%s; ' "${reasons[@]}")"
                echo "    log: $log"
            fi
            fps=""; frames=""; secs=""
        done
    done
done

# Median, min and max fps per mission, renderer and segment ("all" = the
# whole run) for this invocation.
if [ -s "$SUMMARY" ]; then
    echo
    printf '%-10s %-4s %-12s %5s %8s %8s %8s\n' mission rend segment runs median min max
    sort -k1,1 -k2,2 -k3,3 -k4,4n "$SUMMARY" | awk '
        function flush() {
            if (n == 0) return
            med = (n % 2) ? v[(n + 1) / 2] : (v[n / 2] + v[n / 2 + 1]) / 2
            printf "%-10s %-4s %-12s %5d %8.1f %8.1f %8.1f\n", key1, key2, key3, n, med, v[1], v[n]
            n = 0
        }
        $1 != key1 || $2 != key2 || $3 != key3 { flush(); key1 = $1; key2 = $2; key3 = $3 }
        { v[++n] = $4 }
        END { flush() }'
fi

# One profiling run per mission and renderer. perf starts with sampling off
# (--delay=-1); the game turns it on at its first gameplay frame and off when
# gameplay ends, through the control FIFO named in MC2_PERF_CTL.
if [ $PERF = 1 ]; then
    echo
    echo "bench: perf, $PERF_FREQ Hz, call graph $CALL_GRAPH, $FRAMES frames per mission"
    cg=fp
    [ "$CALL_GRAPH" = dwarf ] && cg="dwarf,16384"
    # The control FIFOs live in a local temp dir: --out may be on a file
    # system without FIFOs (WSL's DrvFS, network shares).
    FIFO_DIR=$(mktemp -d) || exit 2
    for m in $MISSIONS; do
        for r in $RENDERERS; do
            if ! use_renderer "$r"; then
                echo "FAIL perf $m $r: $renderer_error"
                failed=1
                continue
            fi
            wait_cool
            base="$OUT_DIR/logs/${STAMP}_${m}_${r}_perf"
            ctl="$FIFO_DIR/ctl.fifo"
            ack="$FIFO_DIR/ack.fifo"
            rm -f "$ctl" "$ack"
            mkfifo "$ctl" "$ack" || exit 2
            # Held open read-write here so neither perf's nor the game's
            # open of a FIFO blocks waiting for the other end.
            exec {ctl_fd}<>"$ctl" {ack_fd}<>"$ack"
            ( cd "$GAME_DIR" && MC2_PERF_CTL="$ctl" MC2_PERF_ACK="$ack" "${RUN[@]}" \
                perf record -F "$PERF_FREQ" --call-graph "$cg" --delay=-1 \
                    --control "fd:$ctl_fd,$ack_fd" -o "$base.perf.data" -- \
                    timeout -s KILL "$TIMEOUT" ./mc2 -mission "$m" -autoexit "$FRAMES" -bench "$base.frames.csv" "${SCENARIO_ARGS[@]}" ) > "$base.log" 2>&1
            rc=$?
            exec {ctl_fd}>&- {ack_fd}>&-
            rm -f "$ctl" "$ack"

            if [ $rc -ne 0 ] || ! grep -q '^\[PERF\] sampling on' "$base.log" || [ ! -s "$base.perf.data" ]; then
                echo "FAIL perf $m $r: exit status $rc, no samples or sampling never switched on"
                echo "    log: $base.log"
                failed=1
                continue
            fi

            {
                echo "# $m $r, commit $COMMIT, $HOST"
                echo "# time per library (game, GL driver, libc, kernel)"
                perf report -i "$base.perf.data" --stdio --no-children --sort dso -g none --percent-limit 0.5 2>/dev/null | grep -v '^$'
                echo
                echo "# top symbols"
                perf report -i "$base.perf.data" --stdio --no-children --sort dso,symbol -g none --percent-limit 0.3 2>/dev/null | grep -v '^$'
            } > "$base.perf.txt"
            if command -v stackcollapse-perf.pl >/dev/null; then
                perf script -i "$base.perf.data" 2>/dev/null | stackcollapse-perf.pl > "$base.perf.folded"
                command -v flamegraph.pl >/dev/null && \
                    flamegraph.pl --title "$m $r $COMMIT" "$base.perf.folded" > "$base.perf.svg"
            fi

            echo "PERF $m $r: $base.perf.txt"
            # the five biggest symbols, as a first look
            grep -E '^ +[0-9.]+%' "$base.perf.txt" | sed -n '/\[/p' | head -5 | sed 's/^/    /'
        done
    done
fi

exit $failed
