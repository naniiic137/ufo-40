#!/bin/sh
# TILTSHOT: turn the hole drawings in holes.txt into src/games/tiltshot/tiltshot_maps.c.
# Run from the repository root:  sh tools/tiltshot/mkmaps.sh
# Every hole is 20 rows of the same width; the tile key is in tiltshot.h.
set -e
in=tools/tiltshot/holes.txt
out=src/games/tiltshot/tiltshot_maps.c
awk '
BEGIN {
    print "/* TILTSHOT - the eighteen holes as drawn. Made by tools/tiltshot/mkmaps.sh"
    print " * from tools/tiltshot/holes.txt: edit that file, not this one. */"
    print "#include \"tiltshot.h\""
    print ""
    n = 0; inhole = 0; bad = 0; bs = sprintf("%c", 92)
}
!inhole && /^#/ { next }
/^HOLE / {
    n = $2; row = 0; w = -1; inhole = 1
    printf "static const char *const H%02d[TSH_ROWS] = {\n", n
    next
}
/^END/ {
    if (row != 20) { printf "hole %d has %d rows, not 20\n", n, row > "/dev/stderr"; bad = 1 }
    print "};"
    print ""
    inhole = 0; count++
    next
}
inhole {
    line = $0
    sub(/\r$/, "", line)
    if (w < 0) w = length(line)
    else if (length(line) != w) { printf "hole %d row %d is %d wide, not %d\n", n, row + 1, length(line), w > "/dev/stderr"; bad = 1 }
    if (w > 256) { printf "hole %d is wider than 256\n", n > "/dev/stderr"; bad = 1 }
    o = ""
    for (i = 1; i <= length(line); i++) {
        ch = substr(line, i, 1)
        o = o (ch == bs ? bs bs : ch)
    }
    printf "    \"%s\",\n", o
    row++
    next
}
END {
    if (count != 18) { printf "%d holes, not 18\n", count > "/dev/stderr"; bad = 1 }
    printf "const char *const *const TSH_MAP[TSH_HOLES] = {\n   "
    for (i = 1; i <= 18; i++) printf " H%02d,", i
    print "\n};"
    if (bad) exit 1
}
' "$in" > "$out.tmp"
mv "$out.tmp" "$out"
echo "wrote $out"
