#!/bin/sh
set -eu

# How Leaf launches a pak: it runs launch.sh from inside the .pak directory with
# the platform environment set (PLATFORM, DEVICE, HOME, *_PATH...). Resolve our
# own dir, route logs to the shared userdata area, then exec the binary.
APP_BIN="leaf-app"
PAK_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$PAK_DIR"

SHARED_USERDATA_ROOT=${SHARED_USERDATA_PATH:-"${HOME:-/tmp}/.userdata/shared"}
LOG_ROOT=${LOGS_PATH:-"$SHARED_USERDATA_ROOT/logs"}
mkdir -p "$LOG_ROOT"
LOG_FILE="$LOG_ROOT/$APP_BIN.txt"
: >"$LOG_FILE"
exec >>"$LOG_FILE"
exec 2>&1

echo "=== Launching $APP_BIN at $(date) ==="
echo "platform=${PLATFORM:-unknown} device=${DEVICE:-unknown}"

exec "./$APP_BIN" "$@"
