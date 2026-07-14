#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
temporary_report=""

cleanup()
{
    if [ -n "$temporary_report" ]; then
        /usr/bin/rm -f -- "$temporary_report"
    fi
}

trap cleanup EXIT HUP INT TERM
cd -- "$project_root"

if [ ! -x /usr/bin/sudo ] || [ ! -x /usr/bin/apt-get ]; then
    echo "Linux Doctor : sudo ou apt-get est introuvable sur cette machine." >&2
    exit 1
fi

printf '%s\n' \
    "Linux Doctor va actualiser uniquement les index APT." \
    "sudo demandera si nécessaire votre mot de passe dans ce terminal ; Linux Doctor ne le lit pas." \
    "Aucun paquet ne sera installé."

/usr/bin/sudo -- /usr/bin/apt-get update
/usr/bin/make all
temporary_report=$(/usr/bin/mktemp "$project_root/frontend/report.json.tmp.XXXXXX")
build/linux-doctor --history --output "$temporary_report"
/usr/bin/mv -f -- "$temporary_report" "$project_root/frontend/report.json"

printf '%s\n' \
    "Linux Doctor : index APT et rapport actualisés." \
    "Vous pouvez maintenant utiliser « Recharger le rapport » dans la page."
