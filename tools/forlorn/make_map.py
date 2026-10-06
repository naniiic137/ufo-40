#!/usr/bin/env python3
"""FORLORN HOPE - the map of Holloway and Thornkeep, drawn as code.

The map is our own design (only its structure follows Mortol II: one fixed
open map, a camp with a blind drop beside it, a tree only a double jump
climbs, plates held down by stones, loose rock, a gulper in a shaft, a
tower of drains, a sealed way in, and a chamber of four hearts). This
script lays it out region by region on a 160 x 80 grid of 10-px tiles and
writes src/games/forlorn/forlorn_map.c. Run it from the repository root:

    python tools/forlorn/make_map.py          write the C file
    python tools/forlorn/make_map.py show     print the map

Legend (also in forlorn.h):
  ' ' air            '#' rock            '=' castle brick    '%' wood
  ',' leaves (decor) '!' tree trunk (decor)
  '^' spikes         '*' loose rock (a blast breaks it)
  '$' sealed brick (looks like brick; a blast breaks it)
  'K' key            'D' locked door (two cells tall)
  '1'-'4' floor plates      'a'-'d' the blocks each plate raises
  'B' the troop's door (the base)   'P' the waystone pad at the base
  'o' floor drain    'O' ceiling drain   'm' midge comb
  foes: 'e' wall-eye 's' shellback 't' hatcheteer 'q' squawker 'r' tusker
        'i' idol facing left  'I' idol facing right  'w' drake  'H' hornet bell
        'k' rust knight  'L' bloater  'h' broodhen  'M' hornhead  'S' stingback
        'G' gulper (its head; the body runs down its shaft)  'T' thorn heart
  A foe's letter marks its bottom-left cell; a big foe covers the cells
  beside and above it, which must be air. The gulper's letter is its head.
"""
import os
import sys

W, H = 160, 80
g = [['#'] * W for _ in range(H)]


def fill(x0, y0, x1, y1, c=' '):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            g[y][x] = c


def put(x, y, c):
    g[y][x] = c


def door(x, y):  # a locked door, cells (x, y) and (x, y + 1)
    put(x, y, 'D')
    put(x, y + 1, 'D')


def leaves(x0, y0, x1, y1, k):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if g[y][x] == ' ' and (x * 7 + y * 3 + k) % 5:
                g[y][x] = ','


# ================================================================ surface
# the sky over Holloway: the camp, the Old Yew, the drop and the meadow
fill(1, 0, 73, 19)
fill(74, 0, 158, 1)

# the camp: the troop's door and the waystone pad beside it
put(7, 19, 'B')
put(11, 19, 'P')

# the Old Yew: a trunk you walk through, branches four rows apart that only
# a double jump (or a stone) can climb
fill(16, 5, 17, 19, '!')
fill(18, 16, 21, 16, '%')
fill(13, 12, 16, 12, '%')
fill(18, 8, 21, 8, '%')
fill(23, 4, 30, 4, '%')
leaves(10, 1, 31, 3, 0)
leaves(12, 5, 22, 7, 1)
leaves(19, 9, 24, 11, 2)
leaves(10, 9, 15, 11, 3)
leaves(18, 13, 22, 15, 4)

# the canopy walk east to the second plate
fill(32, 6, 38, 6, '%')
put(35, 5, 'q')
fill(41, 5, 47, 5, '%')
put(45, 1, 'm')
fill(50, 7, 58, 7, '%')
put(54, 6, '2')
leaves(32, 1, 59, 3, 2)

# the drop: the camp ground ends at x 23; four columns down to the spikes
fill(24, 20, 27, 26)

# the meadow
put(36, 19, 's')
fill(44, 18, 46, 19, '#')
put(44, 18, 'e')
put(62, 19, 'r')
fill(65, 18, 66, 19, '#')       # a mound the tusker runs into

# ================================================================ undercroft
fill(14, 27, 58, 36)
fill(24, 36, 30, 36, '^')       # the spike pit under the drop
fill(31, 35, 32, 36, '#')       # a step up from the pit's east side
fill(33, 33, 37, 33, '#')       # key 1's shelf
put(35, 32, 'K')
fill(17, 36, 19, 36, '#')       # a step on the west floor
# west: a shaft down to a cellar with a key (one way, and only past the pit)
fill(14, 37, 16, 46)
fill(2, 40, 13, 46)
put(4, 46, 'K')
put(9, 46, 'o')
put(1, 43, 'e')
# east: a shellback, drains, a wall-eye
put(41, 36, 's')
put(40, 27, 'O')
put(59, 30, 'e')
put(57, 36, '1')                # the first plate, in the far corner
# the loose rock down to the caves
fill(44, 37, 46, 38, '*')
# the meadow's hole: a drop into the undercroft (one way)
fill(52, 20, 53, 26)
put(50, 36, 'o')                # the floor drain, under the climb

# ================================================================ caves
fill(22, 39, 66, 40)
fill(22, 41, 71, 52)
# west: the vault (door 5, key 2 and a shaft down to the deep)
fill(22, 41, 27, 52, '#')
fill(23, 44, 26, 46)
door(27, 45)
put(25, 46, 'K')
fill(23, 47, 24, 69)
for (x, y) in [(32, 51), (30, 49), (28, 47)]:
    fill(x, y, x + 1, y, '#')
put(36, 52, 't')
put(41, 52, 'q')
# the chasm and the bridge plate 1 raises
fill(50, 52, 58, 52, '^')
fill(47, 51, 49, 52, '#')
fill(50, 50, 58, 50, 'a')
# the east ledge: the steps plate 2 raises, plate 3 and the gulper
for (x, y) in [(59, 51), (61, 49), (63, 47)]:
    fill(x, y, x + 1, y, 'b')
fill(65, 45, 69, 52, '#')
fill(64, 41, 69, 44)
put(68, 44, '3')
fill(70, 41, 71, 43, '#')
fill(70, 44, 71, 60)            # the gulper's shaft, down to the deep
put(70, 44, 'G')
fill(72, 39, 80, 53, '#')
put(72, 42, 'e')

# ================================================================ the deep
fill(25, 61, 39, 69)            # the tunnel from the vault's shaft
fill(40, 61, 103, 77)
fill(40, 70, 61, 78, '#')       # the west shelf (top at row 70)
fill(62, 77, 79, 77, '^')       # the spike pool under the gulper
fill(64, 74, 79, 74, 'c')       # the bridge plate 3 raises
fill(80, 74, 103, 78, '#')      # the east floor (top at row 74)
put(88, 73, 'S')
put(101, 73, 'K')
put(84, 60, 'e')
put(52, 69, 's')
put(46, 61, 'O')

# ================================================================ the castle
fill(74, 2, 158, 59, '=')
# the gate and the courtyard
door(74, 18)
fill(75, 5, 102, 19)
put(86, 19, 'r')
fill(96, 18, 98, 18, '=')
fill(92, 16, 94, 16, '=')
fill(88, 14, 90, 14, '=')
fill(75, 12, 86, 12, '=')       # the walkway
put(80, 11, 'I')                # an idol guarding key 3 behind it
put(76, 11, 'K')
fill(97, 10, 100, 10, '=')
put(99, 9, 'w')
fill(100, 20, 101, 21)          # the stairwell down to hall 1
# hall 1
fill(76, 22, 102, 31)
put(90, 22, 'H')
fill(83, 30, 86, 30, '=')
put(84, 29, 'K')
put(95, 31, 'k')
put(101, 23, 'm')
fill(79, 22, 79, 29, '=')
door(79, 30)
fill(76, 32, 78, 33)            # down to hall 2
# hall 2
fill(76, 34, 102, 45)
fill(76, 40, 79, 40, '=')       # the plate's ledge
put(77, 39, '4')
fill(80, 44, 82, 44, '=')
put(81, 43, 'k')
fill(84, 45, 93, 45, '^')
fill(84, 42, 93, 42, 'd')
fill(87, 37, 89, 37, '=')
put(88, 36, 'w')
fill(95, 34, 102, 42, '=')      # a low corridor the bloater fills
put(97, 45, 'L')
put(101, 44, 'K')
fill(100, 46, 101, 47)          # down to the dungeon
# the dungeon
fill(76, 48, 102, 57)
fill(95, 53, 97, 53, '=')
put(91, 57, 'M')
put(83, 57, 'h')
put(75, 54, 'e')
put(77, 57, 'K')
put(103, 50, 'e')
fill(103, 56, 103, 57)

# ================================================================ the tower
fill(104, 12, 116, 78, '=')
fill(105, 13, 115, 73)
fill(104, 70, 104, 73)          # open to the deep
# ledges two rows apart in three columns (L 105-107, M 109-111, R 113-115):
# each one beside the last, never straight above it
COLS = {'L': (105, 107), 'M': (109, 111), 'R': (113, 115)}
seq = 'MLMRMLM' + 'X' + 'MRMLMRMLMRMLMRMLMRMLM'
y = 72
for c in seq:
    if c == 'X':                # the doors' landing: both sides
        fill(105, y, 107, y, '=')
        fill(113, y, 115, y, '=')
    else:
        x0, x1 = COLS[c]
        fill(x0, y, x1, y, '=')
    y -= 2
# door 3 (from the dungeon) and door 4 (out to the heart chamber)
door(104, 56)
door(116, 56)
fill(117, 56, 123, 57)
# drains on the right-hand ledges, a wall-eye, and the hornet bell on top
for (x, y) in [(115, 65), (115, 45), (115, 29), (115, 21)]:
    put(x, y, 'o')
put(104, 43, 'e')
put(106, 14, 'H')
# the top: the passage east, sealed by five columns of brick
fill(113, 15, 123, 15, '=')
fill(116, 13, 123, 14)
fill(116, 13, 120, 14, '$')

# ================================================================ heart chamber
fill(124, 10, 157, 57)
fill(124, 15, 157, 15, '=')     # the walkway at the top
fill(128, 15, 129, 15)          # a hole down to the upper left
fill(151, 15, 152, 15)          # a hole down to the upper right
fill(134, 57, 147, 57, '^')     # the spike floor
for x in (135, 138, 141, 144, 147):
    put(x, 56, '=')             # stepping stones across it
# the west side: steps, the lower and upper platforms
for (x, y) in [(124, 56), (126, 54), (124, 52), (124, 48), (126, 46), (124, 44), (126, 42)]:
    fill(x, y, x + 1, y, '=')
fill(128, 50, 133, 50, '=')
fill(128, 40, 133, 40, '=')
put(131, 49, 'T')
put(131, 39, 'T')
# the east side, the mirror image
for (x, y) in [(156, 56), (154, 54), (156, 52), (156, 48), (154, 46), (156, 44), (154, 42)]:
    fill(x, y, x + 1, y, '=')
fill(148, 50, 153, 50, '=')
fill(148, 40, 153, 40, '=')
put(149, 49, 'T')
put(149, 39, 'T')

# ================================================================ more company
put(58, 19, 's')                # a second shellback on the meadow
put(93, 15, 't')                # a hatcheteer on the courtyard steps
put(88, 31, 'k')                # a second rust knight in hall 1
put(103, 27, 'e')               # a wall-eye over hall 1
put(99, 57, 'i')                # an idol at the dungeon's far end, facing in
put(45, 69, 'r')                # a tusker on the deep's west shelf
put(32, 69, 't')                # a hatcheteer in the tunnel from the vault
put(7, 46, 'q')                 # a squawker in the cellar

# ================================================================ borders
for y in range(H):
    g[y][0] = '#'
    g[y][W - 1] = '=' if y < 60 else '#'
for x in range(W):
    g[H - 1][x] = '#'

rows = [''.join(r) for r in g]

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'show':
        a = int(sys.argv[2]) if len(sys.argv) > 2 else 0
        b = int(sys.argv[3]) if len(sys.argv) > 3 else W
        print('   ' + ''.join(str((x // 10) % 10) if x % 10 == 0 else ' ' for x in range(a, b)))
        for i, r in enumerate(rows):
            print('%2d %s' % (i, r[a:b]))
    else:
        out = os.path.join('src', 'games', 'forlorn', 'forlorn_map.c')
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, 'w', newline='\n') as f:
            f.write('/* FORLORN HOPE - the map of Holloway and Thornkeep: one fixed map, all our own.\n')
            f.write(' * Written by tools/forlorn/make_map.py (the legend is there and in forlorn.h). */\n')
            f.write('#include "forlorn.h"\n\n')
            f.write('const char *const FRL_MAP[FRL_MH] = {\n')
            for r in rows:
                f.write('    "%s",\n' % r)
            f.write('};\n')
        print('wrote', out)
