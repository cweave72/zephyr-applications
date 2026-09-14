#!/usr/bin/env bash
# Open the net_serial_bridge UART stream in minicom.
#
# minicom opens tty devices only, so socat presents the TCP connection as a
# pseudo-terminal. When the connection drops, socat exits, this script closes
# minicom and reports it. Run the script again to reconnect.
#
# Usage: bridge_minicom.sh [ip] [port]
#   ip    Board address. Default 192.168.1.15.
#   port  Bridge port. Default 12001.
# Environment:
#   BRIDGE_PTY  Path of the pty link. Default /tmp/net_serial_bridge.pty.
#
# Ctrl-A X exits minicom.
set -euo pipefail

IP=${1:-192.168.1.15}
PORT=${2:-12001}
PTY=${BRIDGE_PTY:-/tmp/net_serial_bridge.pty}

# minicom reads ~/.minirc.<profile> on top of its defaults. The profile turns
# hardware flow control off. A pty has no RTS/CTS lines, and minicom's
# compiled-in default is on. The user's ~/.minirc.dfl is not touched.
PROFILE=netbridge
RC=$HOME/.minirc.$PROFILE

for tool in socat minicom; do
    if ! command -v "$tool" > /dev/null; then
        echo "error: $tool not found." >&2
        exit 1
    fi
done

if [ ! -f "$RC" ]; then
    printf 'pu rtscts  No\npu xonxoff No\n' > "$RC"
    echo "Wrote minicom profile $RC."
fi

SOCAT_PID=
WATCH_PID=

cleanup()
{
    [ -n "$WATCH_PID" ] && kill "$WATCH_PID" 2> /dev/null
    [ -n "$SOCAT_PID" ] && kill "$SOCAT_PID" 2> /dev/null
    return 0
}
trap cleanup EXIT

# socat opens its addresses in order. TCP first, so a board that is down
# fails here and not inside minicom: the pty link exists only once connected.
socat tcp:"$IP":"$PORT" pty,link="$PTY",rawer &
SOCAT_PID=$!

# Wait for the pty link. socat exits early when the connect fails.
for _ in $(seq 1 50); do
    if [ -e "$PTY" ]; then
        break
    fi
    if ! kill -0 "$SOCAT_PID" 2> /dev/null; then
        echo "error: cannot connect to $IP:$PORT." >&2
        SOCAT_PID=
        exit 1
    fi
    sleep 0.1
done
if [ ! -e "$PTY" ]; then
    echo "error: connecting to $IP:$PORT timed out." >&2
    exit 1
fi

echo "Bridge $IP:$PORT on $PTY. Ctrl-A X exits."

# Close minicom when the connection drops. minicom is a child of this shell.
(
    while kill -0 "$SOCAT_PID" 2> /dev/null; do
        sleep 0.5
    done
    pkill -TERM -P $$ -x minicom 2> /dev/null
    echo "Connection to $IP:$PORT lost." >&2
) &
WATCH_PID=$!

# -o: no modem init string and no lock file. The init string would go out the
# remote UART.
minicom -o -c on -D "$PTY" "$PROFILE"
