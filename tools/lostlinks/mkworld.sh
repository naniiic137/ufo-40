#!/bin/sh
# LOST LINKS - turns the two hand-drawn layers (over.txt, under.txt) into
# src/games/lostlinks/lostlinks_world.c. In the text files each line is one
# row of 160 tiles, written as eight 20-tile zones split by '|' so the zones
# stay lined up; lines starting with ';' are comments. Run from the repo root:
#   sh tools/lostlinks/mkworld.sh
set -e
dir=$(dirname "$0")
out=src/games/lostlinks/lostlinks_world.c
{
  echo "/* LOST LINKS - the world, both layers. Made by tools/lostlinks/mkworld.sh"
  echo " * from over.txt and under.txt: edit those, not this file. The legend is"
  echo " * in lostlinks.h. */"
  echo "#include \"lostlinks.h\""
  echo ""
  echo "const char *const LNK_MAP[LNK_LAYERS][LNK_MH] = {"
  for f in over under; do
    echo "    {"
    awk -v name="$f" '
      /^;/ { next }
      NF == 0 { next }
      {
        line = $0; gsub(/\|/, "", line); gsub(/\r/, "", line)
        if (length(line) != 160) { printf "%s.txt row %d is %d wide\n", name, n, length(line) > "/dev/stderr"; bad = 1 }
        gsub(/\\/, "\\\\", line)
        printf "        \"%s\",\n", line
        n++
      }
      END { if (n != 72) { printf "%s.txt has %d rows\n", name, n > "/dev/stderr"; bad = 1 } if (bad) exit 1 }
    ' "$dir/$f.txt"
    echo "    },"
  done
  echo "};"
} > "$out.tmp"
mv "$out.tmp" "$out"
echo "wrote $out"
