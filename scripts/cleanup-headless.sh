#!/bin/sh
set -eu

pids=$(ps -eo pid=,args= | awk '/[c]hrom/ && /--headless/ && /[p]uppeteer_dev_chrome_profile-/ { print $1 }')
if [ -z "$pids" ]; then
    printf '%s\n' "Linux Doctor : aucun Chromium headless de test n'est actif."
else
    printf '%s\n' "Linux Doctor : arrêt des Chromium headless laissés par les tests : $pids"
    printf '%s\n' "Le mot de passe sudo peut être demandé pour franchir l'isolation Snap."
    for pid in $pids; do
        sudo kill -TERM "$pid" 2>/dev/null || true
    done
    sleep 2
    for pid in $pids; do
        if kill -0 "$pid" 2>/dev/null; then
            sudo kill -KILL "$pid" 2>/dev/null || true
        fi
    done
fi

for profile in "${TMPDIR:-/tmp}"/puppeteer_dev_chrome_profile-*; do
    [ -e "$profile" ] || continue
    rm -rf -- "$profile"
done

printf '%s\n' "Linux Doctor : nettoyage headless terminé."
