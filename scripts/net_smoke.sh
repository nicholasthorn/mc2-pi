#!/usr/bin/env bash
# Two processes play one mission over the real network, headless (Xvfb + llvmpipe):
# a host and a client on this machine, with the scripted battle in bench/net_m08.fit
# played until one side has lost. Fails on a crash, a hang, an error line, a battle
# that never happened, orders that never reached the host, or a client whose units
# ended up with other armour than the host's.
#
#   scripts/net_smoke.sh --game-dir ../mc2game --build-dir build
#   scripts/net_smoke.sh --game-dir ../mc2game --lag-ms 100 --loss-percent 2
#
# --lag-ms and --loss-percent are GNS's own simulator, on what each end sends, so a
# round trip takes twice the lag. Logs: <log-dir>/host.log and client.log.

set -u

SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"
GAME_DIR=""
BUILD_DIR=""
MISSION="mc2_m08"
SCENARIO="$SRC_DIR/bench/net_m08.fit"
FRAMES=20000
TIMEOUT=900
LOG_DIR="net-smoke-logs"
LAG=""
LOSS=""
EXPECT_WINNER=""
WORLD_EVENTS=yes
DIAL=0
SUPPORT=0
EXPECT_REPAIRS=0
EXPECT_TURRETS=0
PORT=$((27815 + $$ % 1000))

while [ $# -gt 0 ]; do
    case "$1" in
        --game-dir) GAME_DIR="$2"; shift 2 ;;
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --mission) MISSION="$2"; shift 2 ;;
        --scenario) SCENARIO="$2"; shift 2 ;;
        --frames) FRAMES="$2"; shift 2 ;;
        --timeout) TIMEOUT="$2"; shift 2 ;;
        --log-dir) LOG_DIR="$2"; shift 2 ;;
        --lag-ms) LAG="$2"; shift 2 ;;
        --loss-percent) LOSS="$2"; shift 2 ;;
        --port) PORT="$2"; shift 2 ;;
        --expect-winner) EXPECT_WINNER="$2"; shift 2 ;;
        --world-events) WORLD_EVENTS="$2"; shift 2 ;;
        --dial) DIAL=1; shift ;;
        --support) SUPPORT=1; shift ;;
        --expect-repairs) EXPECT_REPAIRS=1; shift ;;
        --expect-turrets) EXPECT_TURRETS=1; shift ;;
        *) echo "net_smoke: unknown option $1" >&2; exit 2 ;;
    esac
done

if [ -z "$GAME_DIR" ] || [ ! -d "$GAME_DIR" ]; then
    echo "net_smoke: --game-dir <built game data> is needed" >&2
    exit 2
fi
GAME_DIR="$(cd "$GAME_DIR" && pwd)"
if [ -n "$BUILD_DIR" ]; then
    cp "$BUILD_DIR/mc2" "$BUILD_DIR/out/res/libmc2res_64.so" "$GAME_DIR/" || exit 2
    rm -rf "$GAME_DIR/shaders" && cp -r "$SRC_DIR/shaders" "$GAME_DIR/" || exit 2
fi
if [ ! -f "$GAME_DIR/options.cfg" ]; then
    cp "$GAME_DIR/minprefs.cfg" "$GAME_DIR/options.cfg" || exit 2
    [ -f "$GAME_DIR/orgprefs.cfg" ] || cp "$GAME_DIR/minprefs.cfg" "$GAME_DIR/orgprefs.cfg"
fi
mkdir -p "$LOG_DIR"
LOG_DIR="$(cd "$LOG_DIR" && pwd)"

export LIBGL_ALWAYS_SOFTWARE=1
export SDL_AUDIODRIVER=dummy
export MC2_STOP_EXITS=1
unset WAYLAND_DISPLAY
export SDL_VIDEODRIVER=x11
[ -n "$LAG" ] && export MC2_NET_LAG_MS="$LAG"
[ -n "$LOSS" ] && export MC2_NET_LOSS_PERCENT="$LOSS"

command -v xvfb-run >/dev/null || { echo "net_smoke: xvfb-run not installed" >&2; exit 2; }
RUN=(xvfb-run -a -s "-screen 0 800x600x24")
COMMON=(-mission "$MISSION" -autoexit "$FRAMES" -benchscenario "$SCENARIO")

# --dial: the host calls a client that waits, as for a host that cannot be reached
HOST_ARGS=(-netlisten "$PORT"); CLIENT_ARGS=(-netjoin "127.0.0.1:$PORT")
if [ "$DIAL" = 1 ]; then
    HOST_ARGS=(-netcall "127.0.0.1:$PORT"); CLIENT_ARGS=(-netjoin "wait:$PORT")
fi
( cd "$GAME_DIR" && "${RUN[@]}" timeout -s KILL "$TIMEOUT" ./mc2 "${COMMON[@]}" -nethost 2 "${HOST_ARGS[@]}" ) > "$LOG_DIR/host.log" 2>&1 &
host_pid=$!
( cd "$GAME_DIR" && "${RUN[@]}" timeout -s KILL "$TIMEOUT" ./mc2 "${COMMON[@]}" "${CLIENT_ARGS[@]}" ) > "$LOG_DIR/client.log" 2>&1 &
client_pid=$!
start=$(date +%s)
wait "$host_pid"; host_rc=$?
wait "$client_pid"; client_rc=$?
secs=$(( $(date +%s) - start ))

BAD='OpenGL Error|: ERROR: |STOP: |FATAL|runtime error: |ERROR: AddressSanitizer|ERROR: LeakSanitizer'
reasons=()
[ "$host_rc" -eq 0 ] || reasons+=("host exit status $host_rc")
[ "$client_rc" -eq 0 ] || reasons+=("client exit status $client_rc")
for side in host client; do
    log="$LOG_DIR/$side.log"
    n=$(grep -cE "$BAD" "$log")
    [ "$n" -gt 0 ] && reasons+=("$side: $n error line(s)")
    grep -q '^\[NET\] mission start' "$log" || reasons+=("$side: the mission never started")
    grep -q '^\[NET\] game over' "$log" || reasons+=("$side: played to no result")
    grep -q '^\[NETRESULT\] rank' "$log" || reasons+=("$side: no results table")
    grep -q '^\[NET\] results closed' "$log" || reasons+=("$side: the results screen never finished")
done
if [ "$WORLD_EVENTS" = yes ]; then
    grep -q '^\[NET\] turn .* weapon hit [1-9]' "$LOG_DIR/host.log" || reasons+=("no weapon hits: the battle did not happen")
    grep -q 'orders from clients [1-9]' "$LOG_DIR/host.log" || reasons+=("no orders reached the host")
    grep -q ', holds [1-9]' "$LOG_DIR/host.log" || reasons+=("no hold position reached the host")
fi
hostwin=$(grep -m1 '^\[NET\] game over' "$LOG_DIR/host.log")
clientwin=$(grep -m1 '^\[NET\] game over' "$LOG_DIR/client.log")
[ "$hostwin" = "$clientwin" ] || reasons+=("host and client disagree: '$hostwin', '$clientwin'")
if ! diff <(grep '^\[NETSCORE\]' "$LOG_DIR/host.log") <(grep '^\[NETSCORE\]' "$LOG_DIR/client.log") > "$LOG_DIR/score.diff" \
    || ! grep -q '^\[NETSCORE\]' "$LOG_DIR/host.log"; then
    reasons+=("host and client ended with other scores, buildings or mines ($LOG_DIR/score.diff)")
fi
world=$(grep '^\[NETWORLD\]' "$LOG_DIR/client.log" | tail -1)
if [ "$WORLD_EVENTS" = yes ]; then
    for what in 'artillery' 'mines' 'captures' 'kills and losses' 'repairs'; do
        echo "$world" | grep -Eq "$what [1-9]" || reasons+=("no $what reached the client: $world")
    done
fi
if [ "$SUPPORT" = 1 ]; then
    # support vehicles and a recovery, seen on the client, and paid for alike at both ends
    support=$(grep '^\[NETSUPPORT\]' "$LOG_DIR/client.log" | tail -1)
    for what in 'vehicles begun' 'dropped' 'recoveries begun' 'recovered'; do
        echo "$support" | grep -Eq "$what [1-9]" || reasons+=("no $what on the client: $support")
    done
    orders=$(grep '^\[NETORDERS\]' "$LOG_DIR/host.log")
    for what in move jump 'attack object' 'attack point' guard stop wait 'power down' 'power up' eject; do
        echo "$orders" | grep -Eq "$what [1-9]" || reasons+=("no $what order applied: $orders")
    done
    if ! diff <(grep '^\[NETPOINTS\]' "$LOG_DIR/host.log") <(grep '^\[NETPOINTS\]' "$LOG_DIR/client.log") > "$LOG_DIR/points.diff" \
        || ! grep -q '^\[NETPOINTS\]' "$LOG_DIR/host.log"; then
        reasons+=("host and client ended with other resource points ($LOG_DIR/points.diff)")
    fi
fi
if [ "$EXPECT_REPAIRS" = 1 ]; then
    echo "$world" | grep -Eq 'repairs [1-9]' || reasons+=("no repair reached the client: $world")
    echo "$world" | grep -Eq 'mines [1-9]' || reasons+=("the minelayer laid no mines: $world")
fi
if [ "$EXPECT_TURRETS" = 1 ]; then
    echo "$world" | grep -Eq 'turret targets [1-9]' || reasons+=("no turret target reached the client: $world")
    grep -Eq '^\[NET\] turn .*\(turrets [1-9]' "$LOG_DIR/host.log" || reasons+=("no turret fired on the host")
fi
if [ -n "$EXPECT_WINNER" ] && ! echo "$hostwin" | grep -q "team $EXPECT_WINNER wins"; then
    reasons+=("expected team $EXPECT_WINNER to win: $hostwin")
fi
last=$(grep '^\[NETCHECK\] [0-9]* units' "$LOG_DIR/client.log" | tail -1)
if [ -z "$last" ]; then
    reasons+=("no convergence check reached the client")
elif ! echo "$last" | grep -q 'armour differs 0,'; then
    reasons+=("client ended with other armour: $last")
fi

if [ ${#reasons[@]} -eq 0 ]; then
    echo "PASS $MISSION (${secs}s) lag ${LAG:-0} ms, loss ${LOSS:-0}%: $hostwin; $last"
    exit 0
fi
echo "FAIL $MISSION (${secs}s) lag ${LAG:-0} ms, loss ${LOSS:-0}%: $(printf '%s; ' "${reasons[@]}")"
grep -hE "$BAD" "$LOG_DIR/host.log" "$LOG_DIR/client.log" | sort | uniq -c | sort -rn | head -10 | sed 's/^/    /'
echo "    logs: $LOG_DIR/host.log $LOG_DIR/client.log"
exit 1
