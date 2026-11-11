#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/round-prepare-check
cc -std=c99 -O2 -Wall -Wextra -Werror -fPIC -shared \
  native-game/round_prepare.c native-game/random_byte.c \
  -o build/round-prepare-check/native_round_prepare.so
python3 reference/compare_round_prepare.py >build/round-prepare-check/result.json
cat build/round-prepare-check/result.json
