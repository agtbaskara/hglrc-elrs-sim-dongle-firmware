#!/usr/bin/env bash
# Flash a UF2 to the Pico: reboots running firmware into BOOTSEL via a
# 1200-baud touch on its USB serial port (if present), then loads with picotool.
set -euo pipefail
if [ $# -lt 1 ]; then
    echo "usage: $0 <uf2> [port]   (port defaults to /dev/ttyACM0)" >&2
    exit 1
fi
UF2=$1
PORT=${2:-/dev/ttyACM0}
[ -f "$UF2" ] || { echo "error: $UF2 not found" >&2; exit 1; }

if ! lsusb -d 2e8a:0003 >/dev/null && [ -e "$PORT" ]; then
    stty -F "$PORT" 1200 2>/dev/null || true
    for _ in $(seq 1 20); do lsusb -d 2e8a:0003 >/dev/null && break; sleep 0.5; done
fi
picotool load -v -x "$UF2"
