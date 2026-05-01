#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
PROXY="$SCRIPT_DIR/portal_proxy.py"

# kill any leftover proxy from a previous session
pkill -f portal_proxy.py 2>/dev/null

# start the proxy in the background; stdout goes to logs/portal_proxy.log
mkdir -p "$SCRIPT_DIR/logs"
python3 "$PROXY" >> "$SCRIPT_DIR/logs/portal_proxy.log" 2>&1 &
PROXY_PID=$!

# wait a moment to claim the USB device before the game opens
sleep 1

# run the game
WINEDLLOVERRIDES="version.dll=n,b;xinput1_3.dll=n,b" "$@"

# game has exited - kill the proxy
kill $PROXY_PID 2>/dev/null
wait $PROXY_PID 2>/dev/null