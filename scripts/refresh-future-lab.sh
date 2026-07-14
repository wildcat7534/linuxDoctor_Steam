#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
binary="$project_root/build/linux-doctor"
output="$project_root/frontend/future-lab-live.json"
lock_file="$project_root/frontend/future-lab-live.lock"
temporary=""
sleep_pid=""
stop_requested=0
once=0
output_owned=0

case ${1-} in
    "") ;;
    --once) once=1 ;;
    *)
        printf '%s\n' "Usage: $0 [--once]" >&2
        exit 2
        ;;
esac

request_stop()
{
    stop_requested=1
    if [ -n "$sleep_pid" ]; then
        kill "$sleep_pid" 2>/dev/null || true
    fi
}

cleanup()
{
    if [ -n "$temporary" ]; then
        rm -f -- "$temporary"
    fi
    if [ -n "$sleep_pid" ]; then
        kill "$sleep_pid" 2>/dev/null || true
    fi
    if [ "$once" -eq 0 ] && [ "$output_owned" -eq 1 ]; then
        rm -f -- "$output"
    fi
}

trap request_stop HUP INT TERM
trap cleanup EXIT

cd -- "$project_root"
if ! command -v flock >/dev/null 2>&1; then
    printf '%s\n' "Linux Doctor : la commande flock (util-linux) est nécessaire au flux live." >&2
    exit 1
fi
exec 9>"$lock_file"
if ! flock -n 9; then
    printf '%s\n' "Linux Doctor : un collecteur Future Lab est déjà actif." >&2
    exit 1
fi
if [ ! -x "$binary" ]; then
    make all
fi

printf '%s\n' \
    "Linux Doctor : flux Future Lab actif (un instantané par seconde)." \
    "Arrêt : Ctrl+C. Root n'est pas requis pour les mesures ; les actions administrateur le demanderont au besoin." >&2

while [ "$stop_requested" -eq 0 ]; do
    temporary=$(mktemp "$output.tmp.XXXXXX")
    if ! "$binary" --future-lab-json >"$temporary"; then
        printf '%s\n' "Linux Doctor : impossible de produire l'instantané Future Lab." >&2
        exit 1
    fi
    mv -f -- "$temporary" "$output"
    temporary=""
    output_owned=1

    if [ "$once" -eq 1 ] || [ "$stop_requested" -eq 1 ]; then
        break
    fi
    sleep 1 &
    sleep_pid=$!
    if ! wait "$sleep_pid"; then
        :
    fi
    sleep_pid=""
done
