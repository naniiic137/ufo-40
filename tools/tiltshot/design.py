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
BS = chr(92)  # the tile that falls away to the right


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

    def surface(self, c, below=0):
        """the first solid row at or under row `below` (the ground under a roof)"""
        for r in range(below, ROWS):
            if self.g[r][c] not in '.~SUOo!CKNY':
                return r
        return ROWS

    def sand(self, c0, c1, depth=2, below=0):
        for c in range(c0, c1):
            top = self.surface(c, below)
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

    def tee(self, c, below=0):
        self.put(c, self.surface(c, below) - 1, 'S')

    def cup(self, c, below=0):
        self.put(c, self.surface(c, below), 'U')

    def on_ground(self, c, ch, below=0):
        self.put(c, self.surface(c, below) - 1, ch)

    def roof(self, c0, c1, bottom, top=0, ch='X'):
        """a ceiling from row `top` down to row `bottom` (inclusive), its
        lower corners cut at 45 degrees (the p and q tiles)"""
        self.rect(c0, top, c1, bottom + 1, ch)
        self.put(c0, bottom, 'p')
        self.put(c1 - 1, bottom, 'q')

    def diamond(self, c, r):
        """a corner block: two tiles by two, every corner cut"""
        self.put(c, r, '/')
        self.put(c + 1, r, '\\')
        self.put(c, r + 1, 'p')
        self.put(c + 1, r + 1, 'q')

    def text(self):
        return [''.join(row) for row in self.g]


# ------------------------------------------------------------------------------
# The eighteen holes. Each note says what the hole is for; the pars follow
# the original's card, the layouts are ours. The things that move and the
# spring lines are in src/games/tiltshot/tiltshot_holes.c.

holes = {}


def hole(n):
    def reg(f):
        holes[n] = f
        return f
    return reg


@hole(1)
def opening_tee():
    # Par 3, the gentle start: a mound with a crate on it, a bumper past
    # it, a sand trap under the bumper and a pond before the green. A full
    # shot over the mound drops into the trap; chip on from there. Slam the
    # ball onto the mound's far side at just the right moment and it races
    # off on fire, over the trap and the pond, off the backstop and home.
    h = Hole(112)
    h.ground([(0, 15), (22, 15), (34, 9), (42, 9), (48, 15), (112, 15)])
    h.tee(3)
    h.on_ground(40, 'C')
    h.put(52, 9, 'O')
    h.sand(52, 60)
    h.water(64, 74, 15, 18)
    h.rect(108, 8, 112, 15, 'X')
    h.cup(86)
    return h


@hole(2)
def stone_skip():
    # Par 3: a lake between the tee cliff and the green, blimps patrolling
    # over it. A hard, flat shot skips across into the trap at the top of
    # the far bank; a rock with a sand top halfway is the safe way; the way
    # over the top runs along a high stone ledge set with bumpers. Past the
    # green the ground ends.
    h = Hole(128)
    h.ground([(0, 12), (13, 12)])
    h.ground([(13, 15), (72, 15), (78, 12), (116, 12)])
    h.water(13, 72, 15, 18)
    for c in range(36, 45):
        h.column(c, 14)
    h.sand(37, 44, 1)
    h.rect(52, 7, 72, 8, 'X')
    h.put(58, 5, 'o')
    h.put(68, 5, 'o')
    h.sand(79, 88, 1)
    h.tee(3)
    h.cup(106)
    return h


@hole(3)
def bounce_house():
    # Par 3: two bouncy lines across a pit with a stone post between them,
    # birds hovering over them. Land on a line and it flings the ball high
    # and on; the post turns it back. Past the pit a sand trap, then the
    # green: a low cave in a cliff of stone that rises out of sight, sand in
    # its mouth, the cup inside and a drop past it. Chip in low from the
    # trap. One exact chain of bounces and a slam goes in from the tee.
    h = Hole(100)
    h.ground([(0, 13), (14, 13)])
    h.ground([(46, 13), (86, 13)])
    h.rect(26, 11, 28, 20, 'X')
    h.sand(46, 54, 1)
    h.sand(57, 63, 1)
    h.roof(60, 100, 9)
    h.tee(3)
    h.cup(74, 11)
    return h


@hole(4)
def skylark():
    # Par 4: big air. A bottomless canyon with sparks wheeling over it, a
    # sheer-sided mesa to land on (junk at its far end to stop against), a
    # second canyon with a sun and a bumper over it, then the green, a sand
    # trap at its near edge and a drop past it. Loft it high and slam it
    # down before it runs off.
    h = Hole(140)
    h.ground([(0, 14), (24, 14)])
    h.ground([(44, 11), (72, 11)])
    h.ground([(92, 14), (128, 14)])
    h.on_ground(66, 'C')
    h.on_ground(69, 'N')
    h.sand(92, 97, 1)
    h.put(83, 6, 'o')
    h.tee(10)
    h.cup(116)
    return h


@hole(5)
def the_chute():
    # Par 4: a steep drop into a hollow and a climb out of it that runs up
    # inside the hill, under a stone roof: a ball that just rolls in rolls
    # back, one slammed onto the drop races up the climb and into the sand
    # at the top. The tunnel goes on through the hill, sparks spinning in it
    # and corner blocks hanging from its roof; or loft it over the hill
    # (junk on top). The green is down past the hill, a sand trap at the
    # foot of the slope and a drop past the cup.
    h = Hole(140)
    h.ground([(0, 10), (12, 10), (20, 18), (30, 18), (38, 10), (106, 10), (112, 13), (134, 13)])
    for k in range(8):
        h.put(30 + k, 13 - k, 'q')
        h.rect(30 + k, 10 - k, 31 + k, 13 - k, 'X')
    h.rect(38, 3, 104, 7, 'X')
    h.put(103, 6, 'q')
    h.sand(38, 46, 1, 7)
    h.put(60, 7, 'q')
    h.put(61, 7, 'p')
    h.put(84, 7, 'q')
    h.put(85, 7, 'p')
    h.sand(112, 115, 1)
    h.on_ground(70, 'C')
    h.on_ground(88, 'N')
    h.tee(3)
    h.cup(123)
    return h


@hole(6)
def three_storeys():
    # Par 4, one of the hardest: a floor broken by pits, and two ice shelves
    # stacked over it. The green stands on a pillar past a wide pit, higher
    # than the middle shelf: a ball sliding off the shelf's end drops short,
    # so slam it onto the ice near the end and it hops across. The top
    # shelf ends at a wall: lob on from there. Junk on the shelves stops a
    # ball on the ice, suns drift round and corner blocks turn it back.
    h = Hole(130)
    h.ground([(0, 17), (40, 17)])
    h.ground([(52, 17), (86, 17)])
    h.ground([(110, 11), (124, 11)])
    h.rect(30, 12, 96, 13, 'I')
    h.rect(50, 5, 98, 6, 'I')
    h.rect(96, 1, 98, 5, 'X')
    h.diamond(66, 15)
    h.put(76, 6, 'q')
    h.on_ground(62, 'C', 8)
    h.on_ground(84, 'K', 8)
    h.on_ground(90, 'N', 2)
    h.tee(3)
    h.cup(118)
    return h


@hole(7)
def dune_steps():
    # Par 3: terraces of dunes with sand in every hollow. A pool before the
    # first sand pit: skim the ball off it and it hops the sand. A corner
    # block over the dunes, and a trundler patrols the approach.
    h = Hole(132)
    h.ground([(0, 11), (14, 11), (17, 14), (58, 14), (60, 15), (68, 15), (70, 14), (86, 14), (88, 16), (124, 16)])
    h.water(17, 26, 14, 17)
    h.sand(26, 36)
    h.sand(60, 68)
    h.sand(100, 106)
    h.sand(116, 122)
    h.diamond(46, 9)
    h.tee(3)
    h.cup(111)
    return h


@hole(8)
def frost_tunnel():
    # Par 4, on ice: a mountain in the way with a narrow shaft in its top. A
    # ball slammed into the shaft drops into a tunnel that runs out on the
    # far side by the green. Over the top the ice is quick and the drop
    # beyond is long; a bumper hangs over the approach, and the green ends
    # in a drop. Something red sits up on the peak.
    h = Hole(140)
    h.ground([(0, 14), (16, 14), (26, 4), (80, 4), (90, 14), (128, 14)])
    h.clear(40, 4, 41, 11)
    h.clear(40, 11, 96, 13)
    h.rect(40, 13, 96, 14, 'X')
    h.put(78, 3, 'R')
    h.put(100, 9, 'o')
    h.tee(3)
    h.cup(116)
    return h


@hole(9)
def toss_up():
    # Par 1, and the only par 1: a full shot high over a pit onto a slope
    # that runs down against a stone wall, the cup at its foot. Four
    # hop-bots juggle themselves up and down right across the ball's way:
    # watch them and swing through a gap. Anything but an ace is over par.
    h = Hole(60)
    h.ground([(0, 12), (8, 12)])
    h.ground([(42, 11), (47, 16), (48, 16)])
    h.rect(48, 6, 60, 20, 'X')
    h.tee(3)
    h.cup(47)
    return h


@hole(10)
def double_spring():
    # Par 3, at night: a chasm with two spring lines over it and a lantern
    # swinging between them. The green is a low hall under a stone roof,
    # sand at its door and an open far end over a drop; the roof rises to a
    # ridge with a chimney at its peak, right over the cup. Land by the door
    # and putt along the hall. To ace it, slam the ball back down onto a
    # spring line at the right moment: it flies up over the ridge and comes
    # in down the chimney or round through the hall's far end.
    h = Hole(110)
    h.ground([(0, 15), (12, 15)])
    h.ground([(70, 14), (104, 14)])
    h.sand(70, 81, 1)
    for k in range(6):
        for j, t in enumerate('12'):
            c, r = 76 + 2 * k + j, 8 - k
            h.put(c, r, t)
            h.rect(c, r + 1, c + 1, 10, 'X')
    for k in range(6):
        for j, t in enumerate('34'):
            c, r = 89 + 2 * k + j, 3 + k
            h.put(c, r, t)
            h.rect(c, r + 1, c + 1, 10, 'X')
    h.rect(87, 2, 88, 3, 'X')
    h.put(87, 1, '/')
    h.rect(89, 2, 90, 3, 'X')
    h.put(89, 1, BS)
    h.put(76, 9, 'p')
    h.tee(3)
    h.cup(88)
    h.on_ground(97, 'K', 10)
    return h


@hole(11)
def pebble():
    # Par 2: short. The tee is on a hilltop over a steep ramp with a stone
    # ledge above it; below, a flat runs into a low hall packed with junk,
    # the cup past the junk and a drop at the far end. Chip down onto the
    # flat and roll into the hall, then putt through what is left of the
    # junk. To ace it, drive the ball in low off the ramp and slam it at
    # just the right moment, so it hops the last of the junk and drops in.
    h = Hole(72)
    h.ground([(0, 9), (10, 9), (16, 15), (60, 15)])
    h.rect(14, 5, 28, 6, 'X')
    h.roof(36, 72, 12, 6)
    for c, k in ((39, 'C'), (41, 'K'), (43, 'N'), (45, 'Y'), (47, 'K')):
        h.on_ground(c, k, 13)
    h.tee(3)
    h.cup(51, 13)
    return h


@hole(12)
def atoll():
    # Par 4, a run-killer: little islands in open water, fish leaping in
    # every gap. Each island has a strip of sand and a piece of junk to stop
    # on, and a ball that runs on sinks. A plank bridge stands alone in the
    # last stretch of water, halfway to the green island.
    h = Hole(150)
    h.ground([(0, 14), (150, 14)])
    h.water(10, 38, 15, 18)
    h.water(52, 76, 15, 18)
    h.water(90, 116, 15, 18)
    h.water(134, 150, 15, 18)
    h.sand(41, 48)
    h.sand(79, 86)
    h.on_ground(50, 'N')
    h.on_ground(88, 'K')
    h.rect(98, 13, 108, 14, 'X')
    h.tee(3)
    h.cup(127)
    return h


@hole(13)
def boulder():
    # Par 3, the hardest: a stone wall far taller than anything so far,
    # close to the tee, a pit right behind it. Loft it over into the sand
    # past the pit. Beyond, two trundlers patrol a yard either side of a
    # two-row stone step with the cup on top, a bumper over the yard and a
    # pit past it. Drop the ball onto the step between the trundlers, or
    # putt it up the step's short ramp and let it die at the cup.
    h = Hole(96)
    h.ground([(0, 15), (26, 15)])
    h.ground([(32, 15), (76, 15)])
    h.rect(18, 3, 26, 15, 'X')
    h.put(18, 3, '/')
    h.put(25, 3, BS)
    h.sand(32, 52, 1)
    h.rect(59, 13, 63, 15, 'X')
    h.put(58, 14, '/')
    h.put(59, 13, '/')
    h.put(62, 13, BS)
    h.put(63, 14, BS)
    h.put(56, 9, 'o')
    h.tee(3)
    h.cup(60, 12)
    return h


@hole(14)
def bumper_alley():
    # Par 4: a spring pad by the tee throws the ball over a pit full of
    # bumpers; near full power carries it, a slam slips it under a bumper.
    # The far side is a sand-edged shelf with more bumpers over it, then a
    # gap with a big bumper over it, and the green in a hall whose roof
    # rises out of sight, sand at its door and a pit inside past the cup.
    h = Hole(140)
    h.ground([(0, 15), (28, 15)])
    h.ground([(54, 14), (94, 14)])
    h.ground([(102, 11), (124, 11)])
    h.ground([(132, 11), (140, 11)])
    h.rect(12, 15, 22, 16, 'T')
    for c, r, k in ((32, 8, 'O'), (38, 11, 'o'), (42, 6, 'O'), (48, 10, 'O'), (36, 3, 'o'),
                    (66, 6, 'O'), (78, 9, 'o'), (86, 4, 'O'), (98, 6, 'O')):
        h.put(c, r, k)
    h.sand(54, 60, 1)
    h.sand(102, 108, 1)
    h.roof(107, 140, 7)
    h.tee(3)
    h.cup(117, 8)
    return h


@hole(15)
def high_shelf():
    # Par 4, one of the hardest: a fairway strewn with junk, a kicker ramp
    # at its end and the green on a high shelf. Land running and let the
    # kicker throw the ball up onto the shelf; or take the bottom path: down
    # in the valley under the kicker (a pit at its near end), then a long
    # climb up to the shelf with a blimp drifting over it. A sand trap on
    # the shelf short of the cup, and a drop past it.
    h = Hole(140)
    h.ground([(0, 15), (50, 15), (56, 9), (58, 9)])
    h.ground([(64, 17), (74, 17), (96, 6), (128, 6)])
    for c, k in ((20, 'C'), (24, 'N'), (28, 'K'), (32, 'C'), (36, 'Y'), (40, 'N'), (44, 'K')):
        h.on_ground(c, k)
    h.sand(104, 111, 1)
    h.tee(3)
    h.cup(118)
    return h


@hole(16)
def nerve():
    # Par 3: two pillars over pits, the first all sand, and a lantern
    # swinging between them. Lob onto the first, then onto the green, whose
    # junk stops a ball that would run off before the drop at its end.
    h = Hole(100)
    h.ground([(0, 14), (10, 14)])
    h.ground([(36, 12), (54, 12)])
    h.ground([(72, 13), (96, 13)])
    h.sand(36, 54, 1)
    h.on_ground(90, 'Y')
    h.on_ground(93, 'C')
    h.tee(3)
    h.cup(84)
    return h


@hole(17)
def the_pagoda():
    # Par 3, one of the hardest: a lattice of pegs over a sand pit guards a
    # raised green, a pit between the sand and the green's bank. A ball
    # rattles down through the pegs into the sand; from there it has to
    # clear the pit onto the green without running off its far end.
    h = Hole(100)
    h.ground([(0, 15), (28, 15), (30, 17), (60, 17)])
    h.ground([(63, 13), (94, 13)])
    h.sand(30, 60)
    for r in range(5, 13, 3):
        off = 0 if (r // 3) % 2 == 0 else 2
        for c in range(33 + off, 60, 4):
            h.put(c, r, '!')
    h.tee(3)
    h.cup(82)
    return h


@hole(18)
def last_orbit():
    # Par 6, the long way home: a lake with fish, a beach, a forest of pegs
    # over a sand bed, a second sand trap, then a second lake with a plank
    # bridge in it and a stone pillar hanging down out of the sky, a lantern
    # swinging under it: everything has to pass low beneath. Then a climb
    # to a plateau where a trundler patrols, and a wall to the sky across
    # its end with a low gate under it: the green lies beyond, down a bank.
    h = Hole(240)
    h.ground([(0, 14), (14, 14)])
    h.ground([(46, 14), (62, 14), (64, 15), (88, 15), (90, 14), (106, 14)])
    h.water(14, 46, 15, 18)
    h.sand(46, 52, 1)
    h.sand(64, 88)
    for r in range(5, 13, 3):
        off = 0 if (r // 3) % 2 == 0 else 3
        for c in range(66 + off, 88, 6):
            h.put(c, r, '!')
    h.sand(98, 103, 1)
    h.water(106, 150, 15, 18)
    h.ground([(106, 18), (150, 18)])
    h.rect(118, 13, 130, 14, 'X')
    h.rect(138, 0, 141, 10, 'X')
    h.ground([(150, 14), (162, 8), (198, 8), (208, 13), (232, 13)])
    h.rect(190, 0, 194, 6, 'X')
    h.on_ground(184, 'K')
    h.water(232, 240, 15, 18)
    h.tee(3)
    h.cup(222)
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
