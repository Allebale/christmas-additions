#!/bin/sh
# Writes mod/editor/gingerbread.obj and bauble.obj (the world editor's look of this mod's enemies) from the
# same code the game draws them with. Run it after changing their looks, and commit the files.
set -e
cd "$(dirname "$0")/.."
out=$(mktemp -d)
c++ -std=c++11 -O1 -DCA_EXPORT_EDITOR_MODELS -Isdk/include src/christmas_additions.cpp -o "$out/export" -lm
"$out/export" mod/editor
rm -rf "$out"
