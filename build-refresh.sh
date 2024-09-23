#!/bin/sh

timestamp=$(date +%s)

rm -f game_*
clang -std=c++17 -g -D DEBUG "src/game.cpp" -shared -o game_$timestamp.dll
mv game_$timestamp.dll game.dll
mv game_$timestamp.exp game.exp
mv game_$timestamp.ilk game.ilk
mv game_$timestamp.lib game.lib
mv game_$timestamp.pdb game.pdb