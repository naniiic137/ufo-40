#!/bin/sh
# TILTSHOT: run the route finder on every hole and write
# src/games/tiltshot/tiltshot_routes.c. Build the headless runner first
# (make headless); run from the repository root:  sh tools/tiltshot/mkroutes.sh
set -e
out=src/games/tiltshot/tiltshot_routes.c
tmp=build/tsh_routes.txt
mkdir -p build
./build/ufo40_headless.exe --script tools/tiltshot/solve.ufs --save-dir build/tsh_solve --out build/tsh_solve > "$tmp" 2>&1 || true
n=$(grep -c "strokes \*/" "$tmp" || true)
if [ "$n" != "18" ]; then echo "only $n routes found:"; cat "$tmp"; exit 1; fi
{
    echo "/* TILTSHOT - a way round every hole, found by the route finder in"
    echo " * tiltshot_bot.c and written here by tools/tiltshot/mkroutes.sh. The tests"
    echo " * replay them with real presses: every hole can be finished. Each shot is"
    echo " * {aim, frames of A, frames into the flight to slam (-1: no slam)}. */"
    echo "#include \"tiltshot.h\""
    echo ""
    echo "const TshShot TSH_ROUTE[TSH_HOLES][TSH_ROUTE_MAX] = {"
    grep "strokes \*/" "$tmp" | sed 's/^ */    /'
    echo "};"
} > "$out"
echo "wrote $out"
grep "strokes \*/" "$tmp" | sed 's/{{.*//'
