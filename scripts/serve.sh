#!/bin/sh
set -eu

make run
exec python3 -m http.server --directory frontend 4545
