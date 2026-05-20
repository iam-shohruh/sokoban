#!/usr/bin/env bash
set -e
make
./sokoban.exe "$@"
