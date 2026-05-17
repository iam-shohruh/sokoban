#!/usr/bin/env bash
set -e
gcc -O2 -std=c11 *.c search/*.c -o sokoban.exe -lm
./sokoban.exe --dataset=microban --solver=a_star
