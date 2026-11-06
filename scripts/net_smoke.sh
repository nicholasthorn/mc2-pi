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
FRAMES=200000	# a game ends by its result: frames go by at any speed, so this is only a backstop
TIMEOUT=900
LOG_DIR="net-smoke-logs"
LAG=""
LOSS=""
EXPECT_WINNER=""
WORLD_EVENTS=yes
DIAL=0
LAN=0
LOBBY=0
SCREENS=0
AGAIN=0
LEAVING=0
HOST_QUITS=0
MECHS=0
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
        --lan) LAN=1; shift ;;
        --lobby) LOBBY=1; shift ;;
        --screens) SCREENS=1; LOBBY=1; shift ;;
        --again) AGAIN=1; shift ;;
        --lobby-leaving) LEAVING=1; shift ;;
        --host-quits) HOST_QUITS=1; LEAVING=1; shift ;;
        --mechs) MECHS=1; SCREENS=1; LOBBY=1; shift ;;
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
COMMON=(-autoexit "$FRAMES" -benchscenario "$SCENARIO")

# --dial: the host calls a client that waits, as for a host that cannot be reached
HOST_ARGS=(-netlisten "$PORT"); CLIENT_ARGS=(-netjoin "127.0.0.1:$PORT")
if [ "$DIAL" = 1 ]; then
    HOST_ARGS=(-netcall "127.0.0.1:$PORT"); CLIENT_ARGS=(-netjoin "wait:$PORT")
fi
# --lan: the client finds the host by asking the local network. One run at a time on a machine:
# a second host cannot take the discovery port, and both clients would find the first.
if [ "$LAN" = 1 ]; then
    CLIENT_ARGS=(-netjoin lan)
fi
# --lobby: by way of a lobby, so only the host says which mission
HOST_MISSION=(-mission "$MISSION"); CLIENT_MISSION=(-mission "$MISSION")
if [ "$LOBBY" = 1 ]; then
    HOST_ARGS+=(-netlobby); CLIENT_MISSION=(-netlobby)
fi
# --screens: as players do, from the main menu: the host names a game and picks the map, and the
# client finds it on the local network. One run at a time on a machine, as for --lan.
HOST_NET=(-nethost 2 "${HOST_ARGS[@]}")
CLIENT_NET=("${CLIENT_ARGS[@]}")
# Each walk ends at the connection screen, where the end of a game leads. --again: both go round
# once more and play a second game.
if [ "$SCREENS" = 1 ]; then
    export MC2_UISMOKE_MAP="$MISSION" MC2_UISMOKE_TIMEOUT=$((TIMEOUT / 2))
    HOST_MISSION=(); CLIENT_MISSION=()
    PREFS="mp-prefs,mp-prefs+name,mp-prefs+colour,mp-params"
    HOST_GAME="mp-params,$PREFS,mp-params+map,mission,mp-connection"
    CLIENT_GAME="mp-browser,mp-params,$PREFS,mission,mp-connection"
    # --mechs: the host turns quickstart off, and each player buys a mech, puts it in the force,
    # gives it a pilot and launches from the pilot screen
    if [ "$MECHS" = 1 ]; then
        BUY="briefing,mechbay,mechpurchase+buy,mechbay+add,pilotready+pilot,mission,mp-connection"
        HOST_GAME="mp-params,$PREFS,mp-params+map,mp-params+mechs,$BUY"
        CLIENT_GAME="mp-browser,mp-params,$PREFS,$BUY"
    fi
    HOST_WALK="mainmenu,mp-connection,$HOST_GAME"
    CLIENT_WALK="mainmenu,mp-connection,$CLIENT_GAME"
    if [ "$AGAIN" = 1 ]; then
        HOST_WALK="$HOST_WALK,$HOST_GAME"
        CLIENT_WALK="$CLIENT_WALK,$CLIENT_GAME"
    fi
    # --lobby-leaving: no game. The client joins, leaves and joins again; then the host leaves.
    if [ "$LEAVING" = 1 ]; then
        # with pauses, so that each sees what the other did before it moves on
        HOST_WALK="mainmenu,mp-connection,mp-params,mp-params+others,mp-params+alone,mp-params+others,mp-params+pause,mp-connection"
        CLIENT_WALK="mainmenu,mp-connection,mp-browser,mp-params,mp-params+pause,mp-browser,mp-browser+pause,mp-params,mainmenu"
    fi
    # --host-quits: in play, the host's process ends, and the client goes back to its main menu
    if [ "$HOST_QUITS" = 1 ]; then
        HOST_WALK="mainmenu,mp-connection,mp-params,mp-params+map,mission,mission+pause"
        CLIENT_WALK="mainmenu,mp-connection,mp-browser,mp-params,mission,mainmenu"
    fi
    HOST_NET=(-uismoke "$HOST_WALK")
    CLIENT_NET=(-uismoke "$CLIENT_WALK")
fi
( cd "$GAME_DIR" && "${RUN[@]}" timeout -s KILL "$TIMEOUT" ./mc2 "${HOST_MISSION[@]}" "${COMMON[@]}" "${HOST_NET[@]}" ) > "$LOG_DIR/host.log" 2>&1 &
host_pid=$!
( cd "$GAME_DIR" && "${RUN[@]}" timeout -s KILL "$TIMEOUT" ./mc2 "${CLIENT_MISSION[@]}" "${COMMON[@]}" "${CLIENT_NET[@]}" ) > "$LOG_DIR/client.log" 2>&1 &
client_pid=$!
start=$(date +%s)
wait "$host_pid"; host_rc=$?
wait "$client_pid"; client_rc=$?
secs=$(( $(date +%s) - start ))

BAD='OpenGL Error|: ERROR: |STOP: |FATAL|runtime error: |ERROR: AddressSanitizer|ERROR: LeakSanitizer'
reasons=()
[ "$host_rc" -eq 0 ] || reasons+=("host exit status $host_rc")
[ "$client_rc" -eq 0 ] || reasons+=("client exit status $client_rc")
if [ "$LEAVING" = 1 ]; then
    for side in host client; do
        n=$(grep -cE "$BAD" "$LOG_DIR/$side.log")
        [ "$n" -gt 0 ] && reasons+=("$side: $n error line(s)")
    done
    grep -q '^\[UISMOKE\] told that the host has gone' "$LOG_DIR/client.log" || reasons+=("the client was not told the host has gone")
    if [ "$HOST_QUITS" = 1 ]; then
        grep -q '^\[NET\] the host has gone$' "$LOG_DIR/client.log" || reasons+=("the client did not see the host go in play")
    else
        grep -q '^\[NET\] the host has gone from the lobby' "$LOG_DIR/client.log" || reasons+=("the client did not see the host go")
        [ "$(grep -c '^\[NET\] in the lobby as commander' "$LOG_DIR/client.log")" -eq 2 ] || reasons+=("the client did not join twice")
    fi
    if [ ${#reasons[@]} -eq 0 ]; then
        [ "$HOST_QUITS" = 1 ] && echo "PASS host quits (${secs}s)" || echo "PASS lobby leaving (${secs}s)"
        exit 0
    fi
    echo "FAIL lobby leaving (${secs}s): $(printf '%s; ' "${reasons[@]}")"
    grep -hE "$BAD|CRASH|UISMOKE" "$LOG_DIR/host.log" "$LOG_DIR/client.log" | head -20 | sed 's/^/    /'
    echo "    logs: $LOG_DIR/host.log $LOG_DIR/client.log"
    exit 1
fi
for side in host client; do
    log="$LOG_DIR/$side.log"
    n=$(grep -cE "$BAD" "$log")
    [ "$n" -gt 0 ] && reasons+=("$side: $n error line(s)")
    grep -q '^\[NET\] mission start' "$log" || reasons+=("$side: the mission never started")
    grep -q '^\[NET\] game over' "$log" || reasons+=("$side: played to no result")
    grep -q '^\[NETRESULT\] rank' "$log" || reasons+=("$side: no results table")
    grep -q '^\[NET\] results closed' "$log" || reasons+=("$side: the results screen never finished")
    if [ "$AGAIN" = 1 ]; then
        [ "$(grep -c '^\[NET\] results closed' "$log")" -eq 2 ] || reasons+=("$side: did not play two games")
        # what the first game's results loaded went with its mission
        grep -q 'Bad texture handle' "$log" && reasons+=("$side: a texture from the first game was used in the second")
    fi
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
    for what in 'vehicles begun' 'dropped'; do
        echo "$support" | grep -Eq "$what [1-9]" || reasons+=("no $what on the client: $support")
    done
    # only if a side had a mech of its own down to call one for, which a battle does not promise
    if grep -q '^\[NET\] calling recovery' "$LOG_DIR/host.log" "$LOG_DIR/client.log"; then
        echo "$support" | grep -q 'refused 0' && ! echo "$support" | grep -Eq 'recoveries begun [1-9]' \
            && reasons+=("a recovery was called and not begun: $support")
    else
        echo "    note: no recovery was called in this run"
    fi
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
if [ "$LOBBY" = 1 ]; then
    # both ends must have agreed on all of it, and the client learned the map
    if ! diff <(grep '^\[NETLOBBY\]' "$LOG_DIR/host.log") <(grep '^\[NETLOBBY\]' "$LOG_DIR/client.log") > "$LOG_DIR/lobby.diff" \
        || ! grep -q '^\[NETLOBBY\] map' "$LOG_DIR/host.log"; then
        reasons+=("host and client came out of the lobby differently ($LOG_DIR/lobby.diff)")
    fi
    grep -q "^\[NET\] launched as commander 1 of 2, on $MISSION" "$LOG_DIR/client.log" || reasons+=("the client did not launch on $MISSION")
    if [ "$SCREENS" = 1 ]; then
        # what each chose on the prefs screen reached the game
        grep -q '^\[NETLOBBY\] commander 0: WalkHost, team [0-9]*, colours 5 ' "$LOG_DIR/client.log" || reasons+=("the host's name and colour did not reach the client")
        grep -q '^\[NETLOBBY\] commander 1: WalkClient, team [0-9]*, colours 9 ' "$LOG_DIR/host.log" || reasons+=("the client's name and colour did not reach the host")
    fi
    if [ "$MECHS" = 1 ]; then
        # each played the mechs it bought, and both made the same units for everyone
        grep -q '^\[NETFORCE\] everyone' "$LOG_DIR/host.log" || reasons+=("the host never had everyone's units")
        for c in 0 1; do
            grep -q "^\[NETFORCE\] commander $c: [1-9][0-9]* units, [1-9]" "$LOG_DIR/host.log" || reasons+=("commander $c had no units placed")
        done
        if ! diff <(grep '^\[NETFORCE\] commander [0-9]*: ' "$LOG_DIR/host.log") <(grep '^\[NETFORCE\] commander [0-9]*: ' "$LOG_DIR/client.log") > "$LOG_DIR/force.diff"; then
            reasons+=("host and client made other units ($LOG_DIR/force.diff)")
        fi
        grep -q 'sent units this game does not allow' "$LOG_DIR/host.log" && reasons+=("the host refused a client's units")
        # each said so on the pilot screen, and the other saw it in its chat
        grep -q '^\[NETCHAT\] commander 1: WalkClient is ready to launch' "$LOG_DIR/host.log" || reasons+=("the client's ready line did not reach the host's chat")
        grep -q '^\[NETCHAT\] commander 0: WalkHost is ready to launch' "$LOG_DIR/client.log" || reasons+=("the host's ready line did not reach the client's chat")
    fi
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
# a crash's frames, by name (gos_logfile.cpp prints module and offset, perhaps mid-line)
for side in host client; do
    grep -q 'CRASH: signal' "$LOG_DIR/$side.log" || continue
    echo "    $side: $(grep -m1 -o 'CRASH: signal.*' "$LOG_DIR/$side.log")"
    grep -o 'CRASH:   #[0-9]* .*' "$LOG_DIR/$side.log" | head -16 | while read -r _ number frame; do
        offset="${frame##*+}"
        case "$frame" in
            mc2+*) where=$(addr2line -Cfpe "$GAME_DIR/mc2" "$offset" 2>/dev/null) ;;
            *) where="" ;;
        esac
        echo "      $number $frame ${where}" | cut -c1-200
    done
done
echo "    logs: $LOG_DIR/host.log $LOG_DIR/client.log"
exit 1
