#!/bin/bash
# Check the network transport between a PC and a Quest on the same Wi-Fi: runs
# net_ping's server on the headset from adb, and its client on this machine. Removes
# what it pushed when done. Needs scripts/android/build.sh game, and a desktop build's
# net_ping. WSL's address translation is fine, as the PC connects out to the headset.
#
# usage: scripts/android/net_ping.sh <path to a desktop net_ping> [seconds] [address]
#
# Without an address it asks the headset for its own over adb. The server runs as
# the adb shell, not inside the app, so this does not test the app's permissions.
set -e
. "$(dirname "$0")/env.sh"

LOCAL=${1:?usage: net_ping.sh <path to a desktop net_ping> [seconds] [address]}
SECONDS_TO_RUN=${2:-10}
ADB="$ANDROID_HOME/platform-tools/adb"
export MSYS_NO_PATHCONV=1
DIR=/data/local/tmp/mc2_net_ping
PORT=27900

ADDRESS=${3:-$("$ADB" shell "ip -4 addr show wlan0" | sed -n 's/.*inet \([0-9.]*\).*/\1/p')}
[ -n "$ADDRESS" ] || { echo "net_ping: the headset has no Wi-Fi address" >&2; exit 2; }

"$ADB" shell "mkdir -p $DIR"
trap '"$ADB" shell "pkill -x net_ping; rm -rf $DIR"' EXIT
"$ADB" push "$ROOT/android/build/game/net_ping" "$DIR/net_ping" > /dev/null
"$ADB" shell "chmod 755 $DIR/net_ping"
# adb stays attached to the server, here in the background
"$ADB" shell "cd $DIR && ./net_ping server $PORT $((SECONDS_TO_RUN + 30))" 2>&1 | sed -u 's/^/headset: /' &
SERVER=$!
sleep 1
echo "net_ping: client to $ADDRESS:$PORT for $SECONDS_TO_RUN s"
result=0
"$LOCAL" client "$ADDRESS" "$PORT" "$SECONDS_TO_RUN" || result=$?
# it ends when its client leaves; a client that never arrived leaves it running
[ "$result" = 0 ] || "$ADB" shell "pkill -x net_ping" || true
wait "$SERVER" || true
exit "$result"
