#!/bin/sh
set -eu

make run
exec python3 -m http.server --bind 127.0.0.1 --directory frontend 4545
