#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BINARY="$ROOT/build/linux-doctor"
URL="https://raw.githubusercontent.com/wildcat7534/linuxDoctor_Steam/knowledge-v1/data/gaming-knowledge.tsv"
MODE=install

if [ "${1-}" = "--check" ]; then
  MODE=check
elif [ "$#" -ne 0 ]; then
  printf 'Usage : %s [--check]\n' "$0" >&2
  exit 2
fi

if [ ! -x "$BINARY" ]; then
  printf 'Linux Doctor doit être compilé avant la mise à jour : lancez make all.\n' >&2
  exit 1
fi
if ! command -v curl >/dev/null 2>&1; then
  printf 'curl est nécessaire pour télécharger la base en HTTPS.\n' >&2
  exit 1
fi

TEMPORARY=$(mktemp "${TMPDIR:-/tmp}/linux-doctor-knowledge.XXXXXX")
trap 'rm -f -- "$TEMPORARY"' EXIT HUP INT TERM

printf 'Téléchargement de la base de connaissances Linux Doctor…\n'
curl --proto '=https' --proto-redir '=https' --tlsv1.2 --fail --location --silent --show-error \
  --max-time 30 --max-filesize 524288 --output "$TEMPORARY" "$URL"

"$BINARY" --check-knowledge "$TEMPORARY"
if [ "$MODE" = check ]; then
  printf 'La base distante est valide ; aucune donnée locale n’a été modifiée.\n'
  exit 0
fi

"$BINARY" --install-knowledge "$TEMPORARY"
printf 'Mise à jour terminée. Régénérez le rapport pour utiliser cette base.\n'
