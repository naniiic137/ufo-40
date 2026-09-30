# TILTSHOT hole drawing helper: builds tools/tiltshot/holes.txt from the
# hole designs below (each one drawn for this cartridge). Then run
# tools/tiltshot/mkmaps.sh to make src/games/tiltshot/tiltshot_maps.c.
#
#   python tools/tiltshot/design.py
#
# Rows run 0 (top) to 19 (bottom). A ground profile is a list of (col, row)
# points: at that column the turf's top is that row. Between two points the
# ground is flat, a 45-degree slope (as many columns as rows) or a gentle one
# (two columns a row).

ROWS = 20


class Hole:
    def __init__(self, width):
        self.w = width
        self.g = [['.'] * width for _ in range(ROWS)]

    def put(self, c, r, ch):
        if 0 <= c < self.w and 0 <= r < ROWS:
            self.g[r][c] = ch

    def get(self, c, r):
        if 0 <= c < self.w and 0 <= r < ROWS:
            return self.g[r][c]
        return '.'

    def column(self, c, top, ch='#'):
        for r in range(top, ROWS):
            self.put(c, r, ch)

    def ground(self, pts, ch='#'):
        for (c0, r0), (c1, r1) in zip(pts, pts[1:]):
            dx, dy = c1 - c0, r1 - r0
            if dy == 0:
                for c in range(c0, c1):
                    self.column(c, r0, ch)
            elif dy < 0 and dx == -dy:
                for k in range(dx):
                    self.put(c0 + k, r0 - 1 - k, '/')
                    self.column_below(c0 + k, r0 - k, ch)
            elif dy < 0 and dx == -2 * dy:
                for k in range(-dy):
                    for j, t in enumerate('12'):
                        self.put(c0 + 2 * k + j, r0 - 1 - k, t)
                        self.column_below(c0 + 2 * k + j, r0 - k, ch)
            elif dy > 0 and dx == dy:
                for k in range(dx):
                    self.put(c0 + k, r0 + k, '\\')
                    self.column_below(c0 + k, r0 + k + 1, ch)
            elif dy > 0 and dx == 2 * dy:
                for k in range(dy):
                    for j, t in enumerate('34'):
                        self.put(c0 + 2 * k + j, r0 + k, t)
                        self.column_below(c0 + 2 * k + j, r0 + k + 1, ch)
            else:
                raise ValueError('bad slope %r -> %r' % ((c0, r0), (c1, r1)))

    def column_below(self, c, top, ch='#'):
        for r in range(top, ROWS):
            self.put(c, r, ch)

    def rect(self, c0, r0, c1, r1, ch):
        """fill columns c0..c1-1, rows r0..r1-1"""
        for r in range(r0, r1):
            for c in range(c0, c1):
                self.put(c, r, ch)

    def clear(self, c0, r0, c1, r1):
        self.rect(c0, r0, c1, r1, '.')

    def pit(self, c0, c1):
        self.clear(c0, 0, c1, ROWS)

    def surface(self, c):
        for r in range(ROWS):
            if self.g[r][c] not in '.~SUOo!CKNY':
                return r
        return ROWS

    def sand(self, c0, c1, depth=2):
        for c in range(c0, c1):
            top = self.surface(c)
            for r in range(top, min(ROWS, top + depth)):
                if self.g[r][c] == '#':
                    self.g[r][c] = 's'

    def water(self, c0, c1, level, bed):
        """a pool: water from row `level` down to the bed row (turf below)"""
        for c in range(c0, c1):
            for r in range(0, ROWS):
                if r < level:
                    if self.g[r][c] in '#/\\1234':
                        self.g[r][c] = '.'
                elif r < bed:
                    self.g[r][c] = '~'
                else:
                    self.g[r][c] = '#'

    def tee(self, c):
        self.put(c, self.surface(c) - 1, 'S')

    def cup(self, c):
        self.put(c, self.surface(c), 'U')

    def on_ground(self, c, ch):
        self.put(c, self.surface(c) - 1, ch)

    def text(self):
        return [''.join(row) for row in self.g]


# ------------------------------------------------------------------------------
# The eighteen holes. Each note says what the hole is for; the pars follow
# the original's card, the layouts are ours.

holes = {}


def hole(n):
    def reg(f):
        holes[n] = f
        return f
    return reg


@hole(1)
def opening_tee():
    # Par 3, the gentle start: a rise with a bumper over it, a long slope
    # down to a bowl of a green against a stone backstop. Land a full shot
    # on the far slope and slam it: it takes off down the hill, on fire,
    # and can run all the way into the cup.
    h = Hole(104)
    h.ground([(0, 15), (18, 15), (24, 12), (38, 12), (46, 16), (104, 16)])
    h.tee(3)
    h.put(30, 6, 'O')
    h.rect(100, 8, 104, 16, 'X')
    h.cup(88)
    h.on_ground(62, 'K')
    return h


@hole(2)
def stone_skip():
    # Par 3: a lake in the way and blimps over it. A flat, hard shot skips
    # across the water; a lob has to thread the blimps. A rock in the
    # middle is the safe way round in three.
    h = Hole(120)
    h.ground([(0, 13), (12, 13), (14, 15), (72, 15), (76, 13), (120, 13)])
    h.water(14, 72, 15, 18)
    for c in range(37, 49):
        h.column(c, 14)
    h.sand(38, 48, 1)
    h.rect(116, 7, 120, 13, 'X')
    h.tee(3)
    h.cup(98)
    return h


@hole(3)
def bounce_house():
    # Par 3: a bed of spring pads between two stone posts. Drop the ball on
    # the pads and they throw it over the posts; the green sits behind a
    # third post, so the last shot is a chip. A perfect bounce is an ace.
    h = Hole(100)
    h.ground([(0, 15), (16, 15), (18, 17), (40, 17), (42, 15), (63, 15), (66, 12), (68, 12), (71, 15), (82, 15)])
    h.rect(18, 17, 40, 18, 'T')
    h.rect(27, 9, 29, 17, 'X')
    h.rect(52, 7, 54, 15, 'X')
    h.tee(3)
    h.cup(66)
    return h


@hole(4)
def skylark():
    # Par 4: big air. A canyon with no bottom, sparks wheeling over it, and a
    # mesa to land on: loft it high and slam it down before it runs off the
    # far side. The junk on the mesa is a brake.
    h = Hole(140)
    h.ground([(0, 14), (26, 14)])
    h.ground([(45, 13), (86, 13), (92, 16), (140, 16)])
    h.tee(12)
    for c, k in ((68, 'C'), (73, 'N'), (78, 'C')):
        h.on_ground(c, k)
    h.rect(136, 8, 140, 16, 'X')
    h.cup(124)
    return h


@hole(5)
def the_chute():
    # Par 4: a steep drop into a hollow and a long climb out of it. A ball
    # that just rolls in rolls back; slammed onto the drop it catches fire
    # and runs up and over the far bank.
    h = Hole(140)
    h.ground([(0, 11), (14, 11), (21, 18), (38, 18), (52, 11), (60, 11), (64, 9), (100, 9),
              (108, 13), (140, 13)])
    h.rect(136, 5, 140, 13, 'X')
    h.tee(3)
    h.cup(124)
    h.on_ground(120, 'Y')
    return h


@hole(6)
def three_storeys():
    # Par 4: three floors of stone over a floor with two pits in it. The
    # shelves end short of the green, with a pit between: run off the top
    # one fast enough (a slam along it helps) or chip over from below.
    h = Hole(130)
    h.ground([(0, 17), (60, 17)])
    h.ground([(72, 17), (104, 17)])
    h.ground([(112, 17), (130, 17)])
    h.rect(30, 12, 90, 14, 'X')
    h.rect(50, 6, 104, 8, 'X')
    h.rect(126, 4, 130, 17, 'X')
    h.tee(3)
    h.cup(119)
    h.on_ground(96, 'C')
    h.on_ground(80, 'N')
    return h


@hole(7)
def dune_steps():
    # Par 3: terraces of dunes with sand in every hollow. A pool before the
    # first sand pit: skim the ball off it and it hops the sand.
    h = Hole(132)
    h.ground([(0, 11), (14, 11), (17, 14), (58, 14), (60, 15), (68, 15), (70, 14), (86, 14),
              (88, 16), (132, 16)])
    h.water(17, 26, 14, 17)
    h.sand(26, 36)
    h.sand(60, 68)
    h.sand(100, 106)
    h.sand(116, 122)
    h.rect(128, 9, 132, 16, 'X')
    h.tee(3)
    h.cup(111)
    return h


@hole(8)
def frost_tunnel():
    # Par 4, on ice: a mountain in the way with a narrow shaft in its top. A
    # ball slammed into the shaft drops into a tunnel that runs out on the
    # far side by the green. Over the top the ice is quick and the drop
    # beyond is long. Something red sits up on the peak.
    h = Hole(140)
    h.ground([(0, 14), (16, 14), (26, 4), (80, 4), (90, 14), (140, 14)])
    h.clear(40, 4, 41, 11)          # the shaft, one tile wide
    h.clear(40, 11, 96, 13)         # the tunnel
    h.rect(40, 13, 96, 14, 'X')     # its floor (not ice)
    h.put(78, 3, 'R')
    h.rect(136, 6, 140, 14, 'X')
    h.tee(3)
    h.cup(122)
    return h


@hole(9)
def toss_up():
    # Par 1, and the only par 1: over a pit into a funnel that runs down
    # into the cup, with two hop-bots bobbing in the way. Anything but an
    # ace is over par.
    h = Hole(50)
    h.ground([(0, 12), (8, 12)])
    h.ground([(30, 11), (35, 16), (37, 16), (42, 11), (50, 11)])
    h.rect(45, 3, 50, 11, 'X')
    h.tee(3)
    h.cup(36)
    return h


@hole(10)
def double_spring():
    # Par 3, at night: a chasm with two spring lines over it. Hit the first
    # and it throws the ball high; slam it into the second and that one
    # sends it on to the green.
    h = Hole(110)
    h.ground([(0, 15), (12, 15)])
    h.ground([(70, 14), (110, 14)])
    h.rect(106, 6, 110, 14, 'X')
    h.tee(3)
    h.cup(84)
    return h


@hole(11)
def pebble():
    # Par 2: short. The tee is on a hilltop above a steep slope; slam the
    # first shot onto the slope and it runs down on fire, hits the backstop
    # and comes back to the cup.
    h = Hole(70)
    h.ground([(0, 9), (10, 9), (16, 15), (46, 15), (48, 14), (60, 14), (62, 15), (64, 15)])
    h.tee(3)
    h.cup(54)
    return h


@hole(12)
def atoll():
    # Par 4: little islands in open water, and fish that leap between them.
    # Every shot has to stop on the next island (the sand helps); a fast flat
    # one can skip.
    h = Hole(150)
    h.ground([(0, 14), (150, 14)])
    h.water(12, 36, 15, 18)
    h.water(48, 74, 15, 18)
    h.water(86, 110, 15, 18)
    h.sand(40, 44)
    h.sand(78, 82)
    h.rect(146, 6, 150, 14, 'X')
    h.tee(3)
    h.cup(134)
    return h


@hole(13)
def boulder():
    # Par 3: a stone wall far taller than anything so far, close to the tee.
    # Loft it over; slam it down behind the wall or it runs past the cup.
    h = Hole(90)
    h.ground([(0, 15), (90, 15)])
    h.rect(30, 3, 36, 15, 'X')
    h.put(29, 3, '/')
    h.put(36, 3, '\\')
    h.rect(84, 6, 90, 15, 'X')
    h.tee(3)
    h.cup(60)
    return h


@hole(14)
def bumper_alley():
    # Par 4: a spring pad by the tee throws the ball over a pit full of
    # bumpers. Near full power carries it; slam to slip past a bumper.
    h = Hole(140)
    h.ground([(0, 15), (40, 15)])
    h.ground([(88, 15), (140, 15)])
    h.rect(14, 15, 24, 16, 'T')
    for c, r, k in ((46, 9, 'O'), (54, 12, 'o'), (60, 7, 'O'), (68, 11, 'O'), (74, 6, 'o'), (80, 10, 'O'),
                    (52, 4, 'o'), (86, 5, 'o')):
        h.put(c, r, k)
    h.rect(136, 7, 140, 15, 'X')
    h.tee(3)
    h.cup(122)
    return h


@hole(15)
def high_shelf():
    # Par 4, one of the hardest: a fairway strewn with junk, a kicker ramp at
    # its end, and the green on a high shelf past a pit. Fly the junk, land
    # running and let the kicker throw the ball up onto the shelf.
    h = Hole(140)
    h.ground([(0, 15), (50, 15), (56, 9)])
    h.ground([(80, 6), (140, 6)])
    for c, k in ((20, 'C'), (24, 'N'), (28, 'K'), (32, 'C'), (36, 'Y'), (40, 'N'), (44, 'K')):
        h.on_ground(c, k)
    h.rect(136, 1, 140, 6, 'X')
    h.tee(3)
    h.cup(122)
    return h


@hole(16)
def nerve():
    # Par 3: two islands of turf on pillars over pits. Lob onto the first,
    # then onto the green, whose junk stops a ball that would run off.
    h = Hole(100)
    h.ground([(0, 14), (10, 14)])
    h.ground([(40, 12), (54, 12)])
    h.ground([(72, 13), (100, 13)])
    h.on_ground(90, 'Y')
    h.on_ground(94, 'C')
    h.rect(97, 5, 100, 13, 'X')
    h.tee(3)
    h.cup(84)
    return h


@hole(17)
def the_pagoda():
    # Par 3, one of the hardest: a lattice of pegs over a sand pit guards a
    # raised green. A ball rattles down through the pegs; a slam at the
    # right moment drops it through a gap and on.
    h = Hole(100)
    h.ground([(0, 15), (28, 15), (30, 17), (62, 17), (66, 13), (100, 13)])
    h.sand(30, 62)
    for r in range(4, 13, 2):
        off = 0 if (r // 2) % 2 == 0 else 2
        for c in range(32 + off, 62, 4):
            h.put(c, r, '!')
    h.rect(96, 4, 100, 13, 'X')
    h.tee(3)
    h.cup(84)
    return h


@hole(18)
def last_orbit():
    # Par 6, the long way home: a lake with fish, sand on the far shore, a
    # forest of pegs over a sand bed, a spring pad, a second lake, a last
    # climb and the green.
    h = Hole(240)
    h.ground([(0, 14), (240, 14)])
    h.water(14, 58, 15, 18)
    h.sand(62, 70)
    h.ground([(96, 14), (98, 15), (128, 15), (130, 14)])
    h.sand(98, 128)
    for r in range(6, 13, 3):
        off = 0 if (r // 3) % 2 == 0 else 3
        for c in range(100 + off, 128, 6):
            h.put(c, r, '!')
    h.rect(136, 14, 144, 15, 'T')
    h.water(150, 184, 15, 18)
    h.ground([(190, 14), (196, 11), (206, 11), (210, 13), (240, 13)])
    h.sand(214, 218)
    h.rect(236, 5, 240, 13, 'X')
    h.tee(3)
    h.cup(228)
    return h


def build():
    out = ['# TILTSHOT holes, made by tools/tiltshot/design.py (tile key in tiltshot.h)']
    for n in range(1, 19):
        f = holes[n]
        h = f()
        out.append('HOLE %d' % n)
        out.extend(h.text())
        out.append('END')
    with open('tools/tiltshot/holes.txt', 'w', newline='\n') as fh:
        fh.write('\n'.join(out) + '\n')
    print('wrote tools/tiltshot/holes.txt')


if __name__ == '__main__':
    build()
