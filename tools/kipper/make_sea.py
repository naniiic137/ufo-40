#!/usr/bin/env python3
"""KIPPER - Gullwater Sound, laid out by hand below.

The sea is one big map, 160 x 150 tiles of 16 px, in three areas stacked by
depth: the Kelpwood (rows 3-49, two docks on the surface), the Drowned
Arcade (rows 50-99, ruins with columns) and the Blackwater (rows 100-149,
dark). Every cavern, tunnel, column, gate and pick-up below is placed by
hand with coordinates; nothing is random. Only the structure follows Porgy
(three stacked areas, two docks in the first, the order things open in);
the layout is our own.

Run it from the repository root:

    python tools/kipper/make_sea.py          check, write tools/kipper/sea.txt
                                             and src/games/kipper/kpr_sea.c
    python tools/kipper/make_sea.py show     also print the map

Tiles in sea.txt:
  '^' sky            '#' rock           '.' water          'P' ruined stone
  'k' kelp (water)   'c' coral (water)  'B' blue block (a missile's or a
  depth charge's blast breaks it)       'd' dirt (the drill digs it)
  '?' a cracked wall (shoot or rub it)  'X' a sealed wall (only a blast)
  '0'-'7' gate tiles (open while their switch is lit)
  'a'-'h' switches for gates 0-7 (a buster torpedo lights one)
Things are marked in the printed map with their own letters (see THING_MARK).
"""
import math
import os
import sys
from collections import deque

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
OUT_C = os.path.join(ROOT, 'src', 'games', 'kipper', 'kpr_sea.c')
OUT_TXT = os.path.join(HERE, 'sea.txt')

W, H = 160, 150
AREA_ROWS = (50, 100)          # area 2 starts at row 50, area 3 at row 100
SKY_ROWS = 3

g = [['^' if y < SKY_ROWS else '#' for x in range(W)] for y in range(H)]


def put(x, y, ch):
    if 1 <= x < W - 1 and SKY_ROWS <= y < H - 1:
        g[y][x] = ch


def rect(x0, y0, x1, y1, ch='.'):
    for y in range(min(y0, y1), max(y0, y1) + 1):
        for x in range(min(x0, x1), max(x0, x1) + 1):
            put(x, y, ch)


def ell(cx, cy, rx, ry, ch='.'):
    for y in range(cy - ry, cy + ry + 1):
        for x in range(cx - rx, cx + rx + 1):
            dx = (x - cx) / (rx + 0.5)
            dy = (y - cy) / (ry + 0.5)
            if dx * dx + dy * dy <= 1.0:
                put(x, y, ch)


def path(pts, w=3, ch='.'):
    """a tunnel w tiles wide through the points"""
    r = (w - 1) / 2.0
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        n = max(abs(x1 - x0), abs(y1 - y0), 1) * 2
        for i in range(n + 1):
            fx = x0 + (x1 - x0) * i / n
            fy = y0 + (y1 - y0) * i / n
            for y in range(int(math.floor(fy - r)), int(math.ceil(fy + r)) + 1):
                for x in range(int(math.floor(fx - r)), int(math.ceil(fx + r)) + 1):
                    if abs(x - fx) <= r + 0.01 and abs(y - fy) <= r + 0.01:
                        put(x, y, ch)


def deco(x0, y0, x1, y1, ch, every=1):
    """kelp or coral on the water tiles of a box (decoration only)"""
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if g[y][x] == '.' and (x * 7 + y * 3) % every == 0:
                g[y][x] = ch


def columns(x0, x1, y0, y1, step=8, w=2):
    for x in range(x0, x1 + 1, step):
        rect(x, y0, x + w - 1, y1, 'P')


# ----------------------------------------------------------------------------
# AREA 1: THE KELPWOOD (rows 3-49). Two docks on the surface: the Pier (top
# middle, where every dive starts) and the Lighthouse (top right).
# ----------------------------------------------------------------------------
# the bays under the docks
rect(70, 3, 89, 6)
rect(126, 3, 142, 6)
# the Pier's shaft, straight down from the dock
rect(78, 7, 81, 22)
# the first pockets either side of the shaft (the start's small play space)
path([(78, 11), (70, 12), (66, 15)], 3)
ell(62, 15, 5, 3)
path([(81, 17), (88, 16), (93, 13)], 3)
ell(96, 12, 4, 3)
ell(98, 18, 3, 2)
path([(96, 14), (98, 17)], 2)
# the Kelp Hall
ell(82, 27, 19, 5)
ell(70, 30, 7, 3)
ell(95, 30, 6, 3)
rect(84, 24, 86, 26, '#')          # a rock in the middle of the hall
rect(74, 27, 75, 28, '#')
deco(64, 26, 100, 32, 'k', 3)
# west: a winding passage to the Western Reef
path([(64, 26), (57, 22), (52, 26), (47, 22), (43, 25)], 3)
ell(32, 24, 12, 8)
ell(24, 31, 6, 4)
rect(30, 22, 33, 25, '#')
rect(38, 28, 39, 30, '#')
deco(20, 26, 44, 33, 'c', 4)
# a chimney up from the reef to a cave under the cliffs
path([(30, 16), (28, 12), (24, 10)], 3)
ell(20, 9, 6, 3)
# Auntie Roe's grotto, up from the first pocket west of the Pier
path([(64, 13), (61, 9)], 2)
ell(59, 8, 4, 2)
# the far west: down the long crooked crawl to the Shelf End (the Super Booster)
path([(20, 30), (12, 33), (8, 38), (12, 41), (6, 44)], 3)
ell(9, 46, 5, 2)
# east: a winding passage to the Coral Garden
path([(100, 26), (106, 22), (110, 25), (114, 22)], 3)
ell(126, 21, 13, 7)
ell(118, 27, 5, 3)
ell(134, 28, 5, 3)
rect(124, 19, 127, 21, '#')
deco(114, 20, 139, 30, 'c', 3)
# up to the Lighthouse bay
rect(133, 7, 136, 14)
# the coral pit below the garden (a narrow pocket)
path([(121, 29), (121, 37)], 2)
ell(121, 39, 3, 2)
# the far east: down to the Breakwater (the Buster Torpedoes)
path([(138, 24), (146, 26), (150, 31), (147, 36), (152, 40)], 3)
ell(150, 44, 6, 3)
# side caves of the lower Kelpwood
path([(68, 31), (60, 36), (56, 40)], 3)
ell(52, 41, 5, 3)
path([(96, 32), (101, 36), (102, 40)], 3)
ell(103, 42, 4, 2)
path([(36, 30), (40, 36), (44, 39)], 2)
ell(46, 40, 3, 2)
path([(14, 33), (20, 38)], 2)
ell(22, 39, 3, 2)
path([(126, 28), (130, 34), (134, 37)], 2)
ell(136, 38, 3, 2)
# the Kelpwood's way down: a crooked descent from the hall into the Arcade
path([(82, 31), (76, 36), (84, 40), (77, 44), (80, 49), (80, 57)], 3)
# the sealed pockets of the lower Kelpwood, reached only from the Arcade
# below through a floor of blue blocks: the Drill (west) and the Depth
# Charge (east)
ell(36, 44, 5, 2)
rect(35, 47, 37, 47)
rect(35, 48, 37, 49, 'B')
ell(115, 45, 4, 2)
rect(114, 48, 116, 49, 'B')
# a cracked wall in the hall's floor, a hidden nook
rect(88, 33, 89, 34, '?')
ell(89, 36, 2, 1)
# a nook sealed under the reef's floor (a depth charge opens it)
rect(31, 33, 33, 33, 'X')
ell(32, 35, 1, 1)

# ----------------------------------------------------------------------------
# AREA 2: THE DROWNED ARCADE (rows 50-99): ruins and columns.
# ----------------------------------------------------------------------------
# the Colonnade, a long hall of columns
rect(46, 56, 113, 68)
columns(50, 108, 59, 65, 8, 2)
rect(66, 56, 67, 57, 'P')
rect(98, 67, 99, 68, 'P')
# the west gate: gate 0, its switch in the wall over it, facing the hall
rect(44, 56, 45, 68, '#')
rect(44, 61, 45, 64, '0')
put(45, 59, 'a')
# beyond it the West Wing, down the long way to the Missile System
path([(43, 62), (37, 62)], 3)
rect(35, 50, 37, 62)                       # up to the Drill's sealed floor
path([(36, 63), (28, 66), (22, 70), (14, 72)], 3)
ell(18, 74, 6, 3)
path([(14, 76), (20, 80), (12, 84), (8, 88), (14, 92)], 3)
ell(10, 94, 5, 3)
# from the bend a way on down to the Undercroft
path([(20, 80), (28, 84), (30, 90)], 3)
# the hidden way to the Targeting System, under the hall's floor
rect(52, 69, 53, 70, '?')
path([(52, 71), (48, 74)], 2)
ell(46, 76, 4, 2)
# east of the hall: up to the Depth Charge's sealed floor, on to the Whorl
path([(113, 62), (118, 60)], 3)
rect(114, 50, 116, 60)
path([(118, 62), (124, 66), (128, 70)], 3)
# the Whorl: the nautilus's chamber
ell(138, 82, 13, 9)
rect(129, 71, 131, 74)
rect(134, 79, 135, 82, 'P')
rect(142, 84, 143, 87, 'P')
# the Lighthouse's way down: from the Breakwater along the east wall
path([(152, 46), (155, 52), (152, 60), (146, 66), (142, 73)], 3)
# under the Whorl, down into the Undercroft
path([(138, 91), (136, 95)], 3)
# the Undercroft: a long low hall along the bottom of the Arcade
rect(26, 90, 140, 97)
columns(34, 130, 92, 95, 12, 1)
rect(60, 90, 66, 91, 'P')
rect(108, 96, 112, 97, 'P')
# the Reliquary (the Spotlight), over the Undercroft behind blue blocks
ell(100, 84, 5, 2)
rect(99, 87, 101, 89, 'B')
# a chamber off the hall's middle, low, and on down to the Undercroft
path([(80, 69), (80, 76)], 3)
ell(80, 79, 7, 3)
path([(84, 81), (88, 85), (82, 88), (84, 90)], 3)
# west of the Colonnade, a side chamber
path([(60, 68), (62, 76)], 2)
ell(64, 78, 4, 2)
# a cracked wall into a niche east of the hall
rect(114, 66, 114, 67, '?')
ell(118, 67, 3, 1)
# a sealed niche in the Undercroft's floor
rect(48, 98, 50, 98, 'X')
rect(48, 99, 50, 99)
# the two ways down into the Blackwater: a plug of dirt in the
# Undercroft's floor, and a floor of blue blocks near the Whorl
rect(78, 98, 81, 100, 'd')
rect(132, 98, 134, 100, 'B')

# ----------------------------------------------------------------------------
# AREA 3: THE BLACKWATER (rows 100-149): pitch dark, vents, eyes.
# ----------------------------------------------------------------------------
path([(80, 100), (80, 106), (76, 110)], 4)
path([(133, 100), (133, 106), (128, 110)], 3)
# the Trench: the long crooked gash across the area
path([(10, 116), (24, 112), (36, 117), (50, 113), (64, 118), (79, 113), (92, 118),
      (106, 113), (120, 117), (134, 112), (148, 116)], 5)
ell(44, 115, 5, 3)
ell(112, 115, 5, 3)
# the Gullet, west (the Radar System)
path([(10, 117), (6, 124), (12, 128), (6, 132)], 3)
ell(10, 135, 5, 3)
# the Undertow, middle-west (Efficient Fuel)
path([(50, 117), (44, 124), (52, 130), (50, 136)], 3)
ell(56, 140, 6, 3)
# the Eastern Deep (Armor Plating)
path([(148, 117), (153, 124), (146, 130), (152, 136)], 3)
ell(149, 141, 6, 3)
# side caves in the dark
path([(28, 114), (26, 106), (32, 103)], 2)
ell(36, 103, 4, 2)
path([(120, 116), (122, 124), (116, 128)], 2)
ell(113, 129, 4, 2)
path([(100, 117), (102, 104)], 2)
ell(104, 102, 3, 1)
# a cracked floor in the Trench, a nook under it
rect(60, 120, 61, 121, '?')
ell(61, 123, 2, 1)
# the way down to Xolloth: a shaft with a floor of blue blocks, then an
# antechamber, then either the switch gate or a tunnel of dirt
rect(78, 117, 81, 124)
rect(78, 125, 81, 126, 'B')
rect(73, 127, 86, 129)
rect(74, 130, 77, 132, '#')
rect(78, 130, 81, 131, '1')
put(73, 128, 'b')
rect(72, 127, 72, 129, '#')
path([(86, 128), (92, 131), (90, 134)], 2, 'd')
rect(78, 132, 81, 133)
# the lair
ell(80, 140, 13, 6)

# edges stay rock
for y in range(H):
    g[y][0] = '#' if y >= SKY_ROWS else '^'
    g[y][W - 1] = '#' if y >= SKY_ROWS else '^'
for x in range(W):
    g[H - 1][x] = '#'

# ----------------------------------------------------------------------------
# Things. Kinds: dock, roe, item (F fuel tank, T torpedo booster, E egg,
# M<n> equipment n), foe, vent, eye, boss route points.
# ----------------------------------------------------------------------------
EQUIP = ['BUSTER', 'DRILL', 'BOOSTER', 'CHARGE', 'MISSILE', 'TARGET', 'SPOT', 'RADAR', 'EFFICIENT', 'ARMOR']

DOCKS = [(80, 4), (134, 4)]          # tile under the dock's middle
ROE = (58, 8)

ITEMS = []                            # (kind, var, x, y)


def item(kind, x, y, var=0):
    ITEMS.append((kind, var, x, y))


# equipment
item('M', 3, 7, 46)       # placeholders fixed below (see EQUIP_AT)
ITEMS.clear()
EQUIP_AT = {
    'BUSTER': (151, 44),
    'BOOSTER': (8, 46),
    'DRILL': (36, 44),
    'CHARGE': (115, 45),
    'MISSILE': (17, 75),
    'TARGET': (45, 76),
    'SPOT': (100, 84),
    'RADAR': (9, 135),
    'EFFICIENT': (113, 129),
    'ARMOR': (150, 141),
}
for i, name in enumerate(EQUIP):
    x, y = EQUIP_AT[name]
    item('M', x, y, i)

# fuel tanks (20)
for x, y in [(60, 15), (98, 18), (21, 8), (52, 42), (124, 16), (89, 36), (22, 39), (137, 38), (32, 35),
             (79, 79), (64, 78), (9, 95), (150, 82), (118, 67), (49, 99),
             (36, 103), (104, 102), (57, 141), (44, 114), (10, 136)]:
    item('F', x, y)
# torpedo boosters (20)
for x, y in [(96, 11), (66, 30), (46, 40), (103, 43), (121, 40), (138, 22), (24, 32), (150, 30),
             (124, 94), (30, 93), (84, 79), (138, 74), (12, 86), (100, 56), (55, 57),
             (22, 112), (61, 123), (134, 112), (55, 139), (152, 124)]:
    item('T', x, y)
# fish eggs (20)
for x, y in [(64, 13), (95, 30), (26, 24), (8, 39), (131, 27), (118, 28), (150, 46), (35, 44),
             (117, 45), (101, 84), (47, 77), (10, 92), (143, 88), (72, 94), (25, 67),
             (75, 112), (113, 116), (9, 129), (148, 140), (84, 128)]:
    item('E', x, y)

FOES = []                             # (kind, x, y, a, b)
FOE_KINDS = ['POLYP', 'FILCHER', 'GLIMMER', 'GREENLING', 'SHELLBACK', 'KITERAY', 'BLUEBELL', 'REDCAP',
             'LANTERNMAW', 'RISER', 'PIPEHORSE', 'BLOATER', 'STILTCRAB', 'SUNSTAR', 'BILLFISH', 'BURR',
             'SNAPWORM', 'FIZZWORM']


def snap(x, y):
    """the nearest open water to a hand-picked spot (a foe drawn on a wall's
    edge slides off it)"""
    for r in range(0, 5):
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                if max(abs(dx), abs(dy)) == r and 0 <= y + dy < H and 0 <= x + dx < W                         and g[y + dy][x + dx] in '.kc':
                    return x + dx, y + dy
    return x, y


def foe(kind, x, y, a=0, b=0):
    x, y = snap(x, y)
    FOES.append((FOE_KINDS.index(kind), x, y, a, b))


# Area 1
for k, x, y in [('GREENLING', 70, 28), ('GREENLING', 92, 26), ('GREENLING', 64, 14), ('BLUEBELL', 90, 22),
                ('POLYP', 72, 32), ('BURR', 96, 33), ('GREENLING', 30, 19), ('GLIMMER', 40, 26),
                ('POLYP', 24, 34), ('SUNSTAR', 44, 18), ('BLUEBELL', 26, 28), ('SHELLBACK', 32, 31),
                ('GREENLING', 122, 18), ('PIPEHORSE', 130, 24), ('POLYP', 117, 29), ('BURR', 128, 27),
                ('BLOATER', 136, 18), ('GLIMMER', 110, 24), ('SUNSTAR', 140, 30), ('KITERAY', 126, 14),
                ('GREENLING', 54, 40), ('BLUEBELL', 103, 41), ('SHELLBACK', 150, 46), ('BILLFISH', 10, 36),
                ('RISER', 121, 38), ('BURR', 12, 43), ('PIPEHORSE', 147, 34), ('GREENLING', 52, 24),
                ('BLOATER', 79, 40), ('POLYP', 20, 11), ('KITERAY', 34, 18), ('BILLFISH', 148, 28),
                ('REDCAP', 100, 31)]:
    foe(k, x, y)
# Area 2
for k, x, y in [('GREENLING', 58, 58), ('PIPEHORSE', 76, 62), ('GLIMMER', 92, 58), ('POLYP', 70, 68),
                ('SHELLBACK', 86, 68), ('KITERAY', 100, 62), ('BLUEBELL', 52, 66), ('REDCAP', 108, 58),
                ('BLOATER', 36, 56), ('BILLFISH', 26, 66), ('SUNSTAR', 18, 71), ('POLYP', 14, 77),
                ('RISER', 12, 88), ('GREENLING', 10, 94), ('SHELLBACK', 116, 54), ('PIPEHORSE', 124, 66),
                ('BLOATER', 150, 60), ('REDCAP', 148, 70), ('GLIMMER', 40, 94), ('GREENLING', 56, 92),
                ('POLYP', 74, 97), ('BURR', 88, 97), ('KITERAY', 100, 93), ('BILLFISH', 118, 93),
                ('SHELLBACK', 46, 97), ('RISER', 130, 96), ('BLUEBELL', 80, 80), ('BURR', 64, 79),
                ('SUNSTAR', 48, 74), ('PIPEHORSE', 96, 94)]:
    foe(k, x, y)
# Area 3
for k, x, y in [('LANTERNMAW', 30, 115), ('SNAPWORM', 50, 111), ('FIZZWORM', 66, 117), ('STILTCRAB', 86, 120),
                ('LANTERNMAW', 100, 116), ('SNAPWORM', 124, 120), ('FIZZWORM', 140, 113), ('STILTCRAB', 20, 116),
                ('REDCAP', 80, 104), ('RISER', 8, 126), ('LANTERNMAW', 12, 133), ('SNAPWORM', 46, 126),
                ('STILTCRAB', 54, 142), ('FIZZWORM', 150, 128), ('LANTERNMAW', 146, 139), ('BLOATER', 116, 114),
                ('POLYP', 38, 104), ('SNAPWORM', 132, 104), ('BURR', 112, 130), ('BILLFISH', 60, 114),
                ('REDCAP', 120, 116), ('LANTERNMAW', 76, 128)]:
    foe(k, x, y)

def floor_below(x, y):
    """a vent stands on the first floor under (x, y)"""
    x, y = snap(x, y)
    while y + 1 < H and g[y + 1][x] in '.kc':
        y += 1
    return x, y


VENTS = [(18, 115), (40, 118), (58, 117), (72, 116), (96, 117), (116, 118), (142, 117), (8, 130), (50, 133),
         (151, 133)]
VENTS = [floor_below(x, y) for (x, y) in VENTS]
EYES = [(20, 105), (44, 108), (66, 105), (90, 108), (120, 105), (146, 108), (30, 124), (64, 126), (100, 128),
        (130, 124), (24, 140), (40, 146), (110, 142), (130, 146)]

# boss routes (roaming bosses swim these loops; the nautilus keeps to its
# chamber and Xolloth to its lair). Tile points.
BOSSES = ['GNASHER', 'INKWELL', 'LONGJAW', 'WHORL', 'NIGHTCAPE', 'XOLLOTH']
ROUTES = {
    'GNASHER': [(32, 20), (24, 30), (36, 28), (46, 24), (58, 24), (70, 30), (84, 30), (96, 28), (90, 24),
                (80, 26), (66, 28), (57, 22), (47, 22)],
    'INKWELL': [(114, 22), (122, 16), (134, 18), (138, 24), (134, 28), (124, 24), (118, 28), (121, 34),
                (121, 28)],
    'LONGJAW': [(80, 57), (96, 57), (110, 57), (110, 67), (90, 67), (70, 67), (48, 67), (48, 57), (62, 57)],
    'WHORL': [(138, 82)],
    'NIGHTCAPE': [(24, 113), (36, 117), (50, 113), (64, 117), (79, 113), (92, 117), (106, 113), (120, 116),
                  (134, 112), (120, 116), (106, 113), (92, 117), (79, 113), (64, 117), (50, 113), (36, 117)],
    'XOLLOTH': [(80, 140)],
}


def _open(x, y):
    return 0 <= x < W and 0 <= y < H and g[y][x] in '.kc'


def _wide(x, y):
    return all(_open(x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1))


def _seen(a, b):
    (x0, y0), (x1, y1) = a, b
    n = max(abs(x1 - x0), abs(y1 - y0)) * 4 or 1
    for i in range(n + 1):
        fx = x0 + (x1 - x0) * i / n + 0.5
        fy = y0 + (y1 - y0) * i / n + 0.5
        if not _wide(int(fx), int(fy)) and not (_open(int(fx), int(fy)) and (i < 4 or i > n - 4)):
            return False
    return True


def route_between(a, b):
    """the hand-placed waypoints joined through open water (wide water where
    there is some), then thinned to the corners"""
    for test in (_wide, _open):
        prev = {a: None}
        q = deque([a])
        while q:
            c = q.popleft()
            if c == b:
                break
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                n = (c[0] + dx, c[1] + dy)
                if n not in prev and (test(*n) or n == b):
                    prev[n] = c
                    q.append(n)
        if b in prev:
            path = []
            c = b
            while c is not None:
                path.append(c)
                c = prev[c]
            path.reverse()
            out = [path[0]]
            i = 0
            while i < len(path) - 1:
                j = len(path) - 1
                while j > i + 1 and not _seen(path[i], path[j]):
                    j -= 1
                out.append(path[j])
                i = j
            return out
    raise SystemExit('no way between %s and %s' % (a, b))


for name in list(ROUTES):
    pts = ROUTES[name]
    if len(pts) < 2:
        continue
    full = []
    for a, b in zip(pts, pts[1:] + pts[:1]):
        seg = route_between(a, b)
        full.extend(seg[:-1])
    ROUTES[name] = full

ARENAS = {'WHORL': (126, 74, 151, 91), 'XOLLOTH': (68, 135, 92, 146)}

# ----------------------------------------------------------------------------
# checks
# ----------------------------------------------------------------------------
SOLID = set('#P^') | set('?X') | set('01234567') | set('abcdefgh') | set('Bd')


def area_of(y):
    return 0 if y < AREA_ROWS[0] else 1 if y < AREA_ROWS[1] else 2


def passable(ch, caps):
    if ch in '.kc':
        return True
    if ch in '01234567':
        return 'buster' in caps
    if ch == 'd':
        return 'drill' in caps
    if ch == 'B':
        return 'blast' in caps
    if ch == '?':
        return True                    # shot or rubbed open
    if ch == 'X':
        return 'charge' in caps
    return False


def dist_field(sx, sy, caps):
    """8-way distances in tiles (diagonals 1.41), no corner cutting"""
    import heapq
    D = {(sx, sy): 0.0}
    q = [(0.0, sx, sy)]
    while q:
        d, x, y = heapq.heappop(q)
        if d > D.get((x, y), 1e9):
            continue
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)):
            nx, ny = x + dx, y + dy
            if not (0 <= nx < W and SKY_ROWS <= ny < H):
                continue
            if not passable(g[ny][nx], caps):
                continue
            if dx and dy and (not passable(g[y][nx], caps) or not passable(g[ny][x], caps)):
                continue
            if ny >= AREA_ROWS[1] and 'spot' not in caps:
                continue
            nd = d + (1.414 if dx and dy else 1.0)
            if nd < D.get((nx, ny), 1e9):
                D[(nx, ny)] = nd
                heapq.heappush(q, (nd, nx, ny))
    return D


def check():
    bad = 0
    for (k, v, x, y) in ITEMS:
        if g[y][x] not in '.kc':
            print('item %s%d at %d,%d sits in %r' % (k, v, x, y, g[y][x]))
            bad += 1
    for (k, x, y, a, b) in FOES:
        if g[y][x] not in '.kc':
            print('foe %s at %d,%d sits in %r' % (FOE_KINDS[k], x, y, g[y][x]))
            bad += 1
    for (x, y) in VENTS:
        if g[y][x] not in '.kc' or g[y + 1][x] in '.kc':
            print('vent at %d,%d needs water over a floor' % (x, y))
            bad += 1
    for name, pts in ROUTES.items():
        for (x, y) in pts:
            if g[y][x] not in '.kc':
                print('route %s point %d,%d in rock' % (name, x, y))
                bad += 1
    for name, pts in ROUTES.items():
        loop = pts + pts[:1] if len(pts) > 1 else pts
        for (x0, y0), (x1, y1) in zip(loop, loop[1:]):
            n = max(abs(x1 - x0), abs(y1 - y0)) * 4
            for i in range(n + 1):
                fx = x0 + (x1 - x0) * i / n + 0.5
                fy = y0 + (y1 - y0) * i / n + 0.5
                if g[int(fy)][int(fx)] not in '.kc' and g[int(fy)][int(fx)] != 'B':
                    print('route %s: %d,%d to %d,%d crosses rock at %d,%d' % (name, x0, y0, x1, y1, int(fx), int(fy)))
                    bad += 1
                    break
    counts = {}
    for (k, v, x, y) in ITEMS:
        counts[k] = counts.get(k, 0) + 1
    if counts != {'M': 10, 'F': 20, 'T': 20, 'E': 20}:
        print('counts', counts)
        bad += 1
    seen = set()
    for (k, v, x, y) in ITEMS:
        if (x, y) in seen:
            print('two items at %d,%d' % (x, y))
            bad += 1
        seen.add((x, y))
    return bad


TILE_PER_BAR = 22.5      # one bar of fuel swims 360 frames at a pixel a frame
MARGIN = 1.25            # out and back, with a quarter spare


def progression(verbose=True):
    """Play the sea greedily: from the docks reached so far, take the nearest
    thing in reach of the fuel there is (out and back with a quarter spare),
    install it, repeat. The Lighthouse counts once the sub has swum there
    from the Pier. Reports how far everything lies and whether it all comes
    in."""
    caps = set()
    fuel = 4
    got = set()
    order = []
    docks = [0]
    dfs = {}

    def fields():
        key = frozenset(caps)
        if key not in dfs:
            dfs[key] = [dist_field(x, y + 1, caps) for (x, y) in DOCKS]
        return dfs[key]

    def eff():
        return 0.75 if 'efficient' in caps else 1.0

    for step in range(400):
        fs = fields()
        if 1 not in docks:
            x, y = DOCKS[1]
            d = fs[0].get((x, y + 1), 1e9)
            if d * eff() / TILE_PER_BAR * MARGIN <= fuel:
                docks.append(1)
                order.append(('dock', 1, x, y, d, fuel))
                continue
        best = None
        for i, (k, v, x, y) in enumerate(ITEMS):
            if i in got:
                continue
            d = min(fs[j].get((x, y), 1e9) for j in docks)
            if d >= 1e9:
                continue
            need = 2 * d * eff() / TILE_PER_BAR * MARGIN
            if need <= fuel and (best is None or d < best[0]):
                best = (d, i)
        if best is None:
            break
        d, i = best
        got.add(i)
        k, v, x, y = ITEMS[i]
        order.append((k, v, x, y, d, fuel))
        if k == 'F':
            fuel += 1
        if k == 'M':
            caps.add({'BUSTER': 'buster', 'DRILL': 'drill', 'BOOSTER': 'boost', 'CHARGE': 'charge',
                      'MISSILE': 'missile', 'TARGET': 'target', 'SPOT': 'spot', 'RADAR': 'radar',
                      'EFFICIENT': 'efficient', 'ARMOR': 'armor'}[EQUIP[v]])
            if 'missile' in caps or 'charge' in caps:
                caps.add('blast')
    if verbose:
        line = []
        for (k, v, x, y, d, f) in order:
            name = EQUIP[v] if k == 'M' else 'DOCK B' if k == 'dock' else k
            line.append('%s@%d,%d:%.0f/%d' % (name, x, y, d, f))
        for i in range(0, len(line), 6):
            print('  ' + '  '.join(line[i:i + 6]))
        print('  collected %d of %d, %d bars at the end' % (len(got), len(ITEMS), fuel))
        if len(got) < len(ITEMS):
            fs = fields()
            for i, (k, v, x, y) in enumerate(ITEMS):
                if i not in got:
                    d = min(fs[j].get((x, y), 1e9) for j in docks)
                    print('    left: %s %s at %d,%d, %s tiles' % (k, EQUIP[v] if k == 'M' else '', x, y,
                                                                 '%.0f' % d if d < 1e9 else 'shut'))
    fs = fields()
    lair = min(f.get((80, 134), 1e9) for f in fs)
    if verbose:
        print('  the lair is %.0f tiles from the nearest dock (%.1f bars one way)' % (lair, lair / TILE_PER_BAR))
    return len(got) == len(ITEMS)


# ----------------------------------------------------------------------------
# output
# ----------------------------------------------------------------------------
THING_MARK = {'F': 'F', 'T': 'T', 'E': 'E', 'M': 'M'}


def show():
    view = [row[:] for row in g]
    for (k, v, x, y) in ITEMS:
        view[y][x] = THING_MARK[k] if k != 'M' else str(v)
    for (k, x, y, a, b) in FOES:
        view[y][x] = '*'
    for (x, y) in DOCKS:
        view[y][x] = 'D'
    view[ROE[1]][ROE[0]] = 'R'
    for (x, y) in VENTS:
        view[y][x] = 'v'
    return [''.join(r) for r in view]


def write_c():
    out = []
    out.append('/* KIPPER - Gullwater Sound: written by tools/kipper/make_sea.py. Do not edit')
    out.append(' * by hand: edit the script and run it. The whole sea is our own layout. */')
    out.append('#include "kpr.h"')
    out.append('')
    out.append('const char *const KPR_ROWS[KPR_H] = {')
    for row in g:
        out.append('    "%s",' % ''.join(row))
    out.append('};')
    out.append('')
    out.append('const KprItemDef KPR_ITEMS[KPR_NITEMS] = {')
    kmap = {'F': 'IK_TANK', 'T': 'IK_BOOSTER', 'E': 'IK_EGG', 'M': 'IK_EQUIP'}
    for (k, v, x, y) in ITEMS:
        out.append('    {%s, %d, %d, %d},' % (kmap[k], v, x, y))
    out.append('};')
    out.append('')
    out.append('const KprFoeDef KPR_FOES[] = {')
    for (k, x, y, a, b) in FOES:
        out.append('    {FK_%s, %d, %d, %d, %d},' % (FOE_KINDS[k], x, y, a, b))
    out.append('};')
    out.append('const int KPR_NFOES = %d;' % len(FOES))
    out.append('')
    out.append('const KprPoint KPR_DOCKS[KPR_NDOCKS] = {%s};' % ', '.join('{%d, %d}' % d for d in DOCKS))
    out.append('const KprPoint KPR_ROE = {%d, %d};' % ROE)
    out.append('const KprPoint KPR_VENTS[KPR_NVENTS] = {%s};' % ', '.join('{%d, %d}' % v for v in VENTS))
    out.append('const KprPoint KPR_EYES[KPR_NEYES] = {%s};' % ', '.join('{%d, %d}' % e for e in EYES))
    out.append('')
    for name in BOSSES:
        pts = ROUTES[name]
        out.append('static const KprPoint ROUTE_%s[] = {%s};' % (name, ', '.join('{%d, %d}' % p for p in pts)))
    out.append('const KprRoute KPR_ROUTES[KPR_BOSSES] = {')
    for name in BOSSES:
        out.append('    {ROUTE_%s, %d},' % (name, len(ROUTES[name])))
    out.append('};')
    out.append('const KprRect KPR_ARENA_WHORL = {%d, %d, %d, %d};' % ARENAS['WHORL'])
    out.append('const KprRect KPR_ARENA_XOLLOTH = {%d, %d, %d, %d};' % ARENAS['XOLLOTH'])
    out.append('')
    with open(OUT_C, 'w', newline='\n') as f:
        f.write('\n'.join(out) + '\n')


def main():
    bad = check()
    ok = progression()
    lines = show()
    with open(OUT_TXT, 'w', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')
    if len(sys.argv) > 1 and sys.argv[1] == 'show':
        for i, l in enumerate(lines):
            print('%3d %s' % (i, l))
    if bad or not ok:
        print('PROBLEMS: %d, everything collected: %s' % (bad, ok))
        sys.exit(1)
    os.makedirs(os.path.dirname(OUT_C), exist_ok=True)
    write_c()
    print('wrote', OUT_C)


if __name__ == '__main__':
    main()
