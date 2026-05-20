#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

gcc -O2 -std=c11 *.c search/*.c -o sokoban.exe -lm

if [ "$#" -gt 0 ]; then
  ./sokoban.exe "$@"
else
  ./sokoban.exe --dataset=microban --solver=a_star --batch
fi
