#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
refresh_pid=""
server_pid=""

cleanup()
{
    if [ -n "$refresh_pid" ]; then
        kill "$refresh_pid" 2>/dev/null || true
        wait "$refresh_pid" 2>/dev/null || true
    fi
    if [ -n "$server_pid" ]; then
        kill "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
    fi
}

trap cleanup EXIT
trap 'exit 0' HUP INT TERM

cd -- "$project_root"
make run
./scripts/refresh-future-lab.sh &
refresh_pid=$!
port=${LINUX_DOCTOR_PORT:-4545}
python3 scripts/serve.py "$port" &
server_pid=$!
wait "$server_pid"
