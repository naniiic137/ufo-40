#!/usr/bin/env python3
"""ANTIPODE - Tallow Station, drawn by hand in tools/antipode/station.txt.

This script reads the station (four sector maps and every room's own tile
layout), checks it and writes src/games/antipode/apd_rooms.c. Run it from
the repository root:

    python tools/antipode/make_station.py          check and write the C file
    python tools/antipode/make_station.py skel     print a bare skeleton (the
                                                   border with its doorways)
                                                   for each room with no layout

The map and the rooms are our own design. Only the structure follows
Vainger (three sectors of rooms joined by doors, teleporters, a fourth
sector behind four keys).

station.txt
-----------
  map <SECTOR>            rows of the sector map until 'end' (see apd_world.c)
  room <S> <id> [flag ...] [| NAME]
  things <tokens>         what's in it; @N puts a thing on the layout's mark N
  <layout rows>           (w*20) x (h*10) characters, until a blank line
  door <S> <ax> <ay> <h|v> <pos>   a doorway's row (or a shaft's column)
S is G (the Gantry), B (the Brine), C (the Coil) or K (the Crown).

Layout characters:
  '#' wall      '.' air      '^' spikes on a floor   'v' spikes on a ceiling
  '=' one-way plate          'a' acid                'X' cracked concrete
  'r' red spike ball         'p' purple spike ball   'G' grey wall
  '%' cover (pipes drawn in front)                    '~' heat haze (air, decor)
  '0'-'9' 'A'-'F' marks for things (air)
The border's doorways are cut by the doors (their row or column comes from
the door lines, or a default); the layout must leave a way to them.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'station.txt')
OUT = os.path.join(ROOT, 'src', 'games', 'antipode', 'apd_rooms.c')

CW, CH = 20, 10
SECTORS = ['GANTRY', 'BRINE', 'COIL', 'CROWN']
SKEY = {'G': 0, 'B': 1, 'C': 2, 'K': 3}
EDGE = {'o': 'E_DOOR', '1': 'E_PASS1', '2': 'E_PASS2', '3': 'E_PASS3', 'b': 'E_BOSS', 'n': 'E_NANO',
        'h': 'E_BARREL', 'm': 'E_CRACK', 'e': 'E_ZAP', 'z': 'E_TARGET'}
FLAGS = {'heat': 'RF_HEAT', 'spikes': 'RF_SPIKES', 'acid': 'RF_ACID', 'arena': 'RF_ARENA', 'start': 'RF_START'}
TILES = set('#.^v=aXrpG%~0123456789ABCDEF')
SOLID = set('#^v')         # for the reach check (cracks, gates, props and the grey wall count as passable)
MARKS = set('0123456789ABCDEF')


ERRORS = []


def die(msg):
    print('station: ' + msg)
    sys.exit(1)


class Bad(Exception):
    pass


def bad(msg):
    raise Bad(msg)


def parse():
    maps, rooms, doorpos = {}, [], {}
    lines = open(SRC, encoding='utf-8').read().split('\n')
    i = 0
    while i < len(lines):
        ln = lines[i].rstrip('\r')
        if ln.startswith('#') or not ln.strip():
            i += 1
            continue
        if ln.startswith('map '):
            name = ln.split()[1]
            rows = []
            i += 1
            while lines[i].rstrip('\r') != 'end':
                rows.append(lines[i].rstrip('\r'))
                i += 1
            maps[SECTORS.index(name)] = rows
            i += 1
            continue
        if ln.startswith('door '):
            p = ln.split()
            doorpos[(SKEY[p[1]], int(p[2]), int(p[3]), p[4] == 'v')] = int(p[5])
            i += 1
            continue
        if ln.startswith('room '):
            head, _, name = ln.partition('|')
            p = head.split()
            r = {'sector': SKEY[p[1]], 'id': p[2], 'flags': p[3:], 'name': name.strip() or None,
                 'things': '', 'rows': [], 'line': i + 1}
            i += 1
            if i < len(lines) and lines[i].startswith('things'):
                r['things'] = lines[i][6:].strip()
                i += 1
            while i < len(lines) and lines[i].rstrip('\r'):
                r['rows'].append(lines[i].rstrip('\r'))
                i += 1
            rooms.append(r)
            continue
        die('line %d: what is "%s"?' % (i + 1, ln))
    return maps, rooms, doorpos


def map_at(m, row, col):
    if row >= len(m):
        return ' '
    s = m[row]
    return s[col] if col < len(s) else ' '


def build(maps, rooms_txt):
    cells = {}
    rooms = []
    doors = []
    for s in range(4):
        m = maps[s]
        gh = (len(m) + 1) // 2
        gw = (max(len(x) for x in m) + 1) // 2
        seen = {}
        for gy in range(gh):
            for gx in range(gw):
                c = map_at(m, gy * 2, gx * 2)
                if c in '. ' or (s, gx, gy) in cells:
                    continue
                w = 1
                while map_at(m, gy * 2, (gx + w) * 2) == c:
                    w += 1
                h = 1
                while map_at(m, (gy + h) * 2, gx * 2) == c:
                    h += 1
                if c in seen:
                    die('sector %s: room %s twice' % (SECTORS[s], c))
                seen[c] = len(rooms)
                rooms.append({'sector': s, 'id': c, 'gx': gx, 'gy': gy, 'w': w, 'h': h})
                for y in range(h):
                    for x in range(w):
                        cells[(s, gx + x, gy + y)] = len(rooms) - 1
        for gy in range(gh):
            for gx in range(gw):
                a = cells.get((s, gx, gy))
                if a is None:
                    continue
                b = cells.get((s, gx + 1, gy))
                e = map_at(m, gy * 2, gx * 2 + 1)
                if b is not None and b != a and e in EDGE:
                    doors.append({'sector': s, 'ax': gx, 'ay': gy, 'v': False, 'type': e, 'ra': a, 'rb': b})
                b = cells.get((s, gx, gy + 1))
                e = map_at(m, gy * 2 + 1, gx * 2)
                if b is not None and b != a and e in EDGE:
                    doors.append({'sector': s, 'ax': gx, 'ay': gy, 'v': True, 'type': e, 'ra': a, 'rb': b})
    # every room in the map has its text, and the other way round
    by = {(r['sector'], r['id']): r for r in rooms_txt}
    for r in rooms:
        t = by.get((r['sector'], r['id']))
        if not t:
            if PARTIAL:
                t = {'flags': [], 'name': None, 'things': '', 'rows': [], 'line': 0}
            else:
                die('no text for room %s %s' % (SECTORS[r['sector']], r['id']))
        r.update({k: t[k] for k in ('flags', 'name', 'things', 'rows', 'line')})
    if len(by) != len(rooms) and not PARTIAL:
        die('%d rooms in the text, %d in the maps' % (len(by), len(rooms)))
    return rooms, doors


H_ROWS = [7, 3, 5, 1, 4, 7, 5, 3]
V_COLS = [9, 3, 15, 6, 12, 9, 4, 14]


def door_pos(d, doorpos):
    k = (d['sector'], d['ax'], d['ay'], d['v'])
    if k in doorpos:
        return doorpos[k]
    i = (d['ax'] * 5 + d['ay'] * 3 + d['sector'] * 7 + (1 if d['v'] else 0)) % 8
    return V_COLS[i] if d['v'] else H_ROWS[i]


def gaps(r, ri, doors, doorpos):
    """the room's doorway tiles: (x, y) in the room"""
    out = []
    for d in doors:
        if d['ra'] != ri and d['rb'] != ri:
            continue
        is_a = d['ra'] == ri
        gx = d['ax'] + (0 if is_a or d['v'] else 1)
        gy = d['ay'] + (0 if is_a or not d['v'] else 1)
        lx, ly = (gx - r['gx']) * CW, (gy - r['gy']) * CH
        p = door_pos(d, doorpos)
        if not d['v']:
            bx = lx + CW - 1 if is_a else lx
            out += [(bx, ly + p), (bx, ly + p + 1)]
        else:
            by = ly + CH - 1 if is_a else ly
            out += [(lx + p, by), (lx + p + 1, by)]
    return out


def skeleton(r, ri, doors, doorpos):
    W, H = r['w'] * CW, r['h'] * CH
    g = [['.'] * W for _ in range(H)]
    for x in range(W):
        g[0][x] = g[H - 1][x] = '#'
    for y in range(H):
        g[y][0] = g[y][W - 1] = '#'
    # the walls between a room's own cells are not there
    for (x, y) in gaps(r, ri, doors, doorpos):
        g[y][x] = '.'
    return [''.join(row) for row in g]


def check(r, ri, doors, doorpos):
    W, H = r['w'] * CW, r['h'] * CH
    rows = r['rows']
    where = 'room %s %s (line %d)' % (SECTORS[r['sector']], r['id'], r['line'])
    if len(rows) != H or any(len(x) != W for x in rows):
        bad('%s: the layout must be %d x %d (it is %d rows; rows of the wrong width: %s)' %
            (where, W, H, len(rows), [(r['line'] + 2 + k, len(x)) for k, x in enumerate(rows) if len(x) != W]))
    for y, row in enumerate(rows):
        for x, c in enumerate(row):
            if c not in TILES:
                bad('%s: unknown tile %r at %d,%d' % (where, c, x, y))
    g = [list(x) for x in rows]
    gp = gaps(r, ri, doors, doorpos)
    for (x, y) in gp:
        if g[y][x] not in '.#':
            bad('%s: the doorway tile %d,%d must be open (it is %r)' % (where, x, y, g[y][x]))
        g[y][x] = '.'
    # the border is wall apart from the doorways
    for x in range(W):
        for y in (0, H - 1):
            if g[y][x] != '#' and (x, y) not in gp:
                bad('%s: the border is open at %d,%d' % (where, x, y))
    for y in range(H):
        for x in (0, W - 1):
            if g[y][x] != '#' and (x, y) not in gp:
                bad('%s: the border is open at %d,%d' % (where, x, y))
    marks = {}
    for y in range(H):
        for x in range(W):
            if g[y][x] in MARKS:
                if g[y][x] in marks:
                    bad('%s: mark %s twice' % (where, g[y][x]))
                marks[g[y][x]] = (x, y)
    for tok in r['things'].split():
        if '@' in tok:
            m = tok.split('@')[1][:1]
            if m not in marks:
                bad('%s: %s: no mark %s' % (where, tok, m))
    # can the suit (10 x 20 px) get from the first doorway to every doorway and mark?
    FC = 8
    fw, fh = W * 16 // FC, H * 16 // FC

    def free(cx, cy):
        px, py = cx * FC + 4, cy * FC + 4
        x0, x1, y0, y1 = px - 10, px + 9, py - 11, py + 10  # fatter than the suit: room to steer
        for ty in range(y0 // 16 if y0 >= 0 else -1, y1 // 16 + 1):
            for tx in range(x0 // 16 if x0 >= 0 else -1, x1 // 16 + 1):
                if tx < 0 or ty < 0 or tx >= W or ty >= H:
                    bx, by2 = min(max(tx, 0), W - 1), min(max(ty, 0), H - 1)
                    if (bx, by2) not in gp:
                        return False
                    continue
                if g[ty][tx] in SOLID:
                    return False
        return True
    ok = [[free(x, y) for x in range(fw)] for y in range(fh)]
    # a doorway is reached if either of its two tiles is
    # things that stand on a floor (or, a ceiling pad, hang from a ceiling)
    for tok in r['things'].split():
        if '@' not in tok:
            continue
        name, m = tok.split('@')[0], tok.split('@')[1][:1]
        x, y = marks[m]
        if name.startswith(('pod', 'log', 'pass', 'tele', 'start', 'padf')) and g[y + 1][x] not in '#X':
            bad('%s: %s at %d,%d needs a floor under it' % (where, tok, x, y))
        if name.startswith('padc') and g[y - 1][x] != '#':
            bad('%s: %s at %d,%d needs a ceiling over it' % (where, tok, x, y))
    FOES = ('tread', 'treadr', 'treadg', 'spitter', 'eye', 'hive', 'skip', 'skimmer', 'sentry')
    need = [tok.split('@')[1][:1] for tok in r['things'].split() if '@' in tok and tok.split('@')[0].split('*')[0] not in FOES]
    targets = [[gp[k], gp[k + 1]] for k in range(0, len(gp), 2)] + [[marks[m]] for m in need]
    if not targets:
        return
    def cells_near(tx, ty):
        # free cells where the suit (10 x 20) touches the tile
        out = []
        for cy in range(max(0, ty * 2 - 3), min(fh, ty * 2 + 4)):
            for cx in range(max(0, tx * 2 - 2), min(fw, tx * 2 + 4)):
                if not ok[cy][cx]:
                    continue
                px, py = cx * FC + 4, cy * FC + 4
                if px - 5 <= tx * 16 + 15 and px + 4 >= tx * 16 and py - 10 <= ty * 16 + 15 and py + 9 >= ty * 16:
                    out.append((cx, cy))
        return out
    start = cells_near(*targets[0][0]) + cells_near(*targets[0][-1])
    if not start:
        bad('%s: nothing can stand by the doorway or mark at %s' % (where, targets[0]))
    seen = set(start)
    todo = list(start)
    while todo:
        cx, cy = todo.pop()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (cx + dx, cy + dy)
            if 0 <= n[0] < fw and 0 <= n[1] < fh and ok[n[1]][n[0]] and n not in seen:
                seen.add(n)
                todo.append(n)
    if SHOW == (r['sector'], r['id']):
        for y in range(H):
            line = ''
            for x in range(W):
                hit = any((cx, cy) in seen for cx in (2 * x, 2 * x + 1) for cy in (2 * y, 2 * y + 1))
                line += '+' if hit and g[y][x] == '.' else g[y][x]
            print(line)
    for t in targets:
        if not any(c in seen for tt in t for c in cells_near(*tt)):
            bad('%s: can\'t reach %s' % (where, t[0]))


def c_str(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def write(maps, rooms, doors, doorpos):
    o = []
    o.append('/* ANTIPODE - Tallow Station: written by tools/antipode/make_station.py from')
    o.append(' * tools/antipode/station.txt. Do not edit by hand: edit the station and run')
    o.append(' * the script. Every room is our own hand-made layout. */')
    o.append('#include "apd.h"')
    o.append('')
    for s in range(4):
        o.append('static const char *const MAP_%d[] = {' % s)
        for row in maps[s]:
            o.append('    %s,' % c_str(row))
        o.append('    NULL,')
        o.append('};')
    o.append('const char *const *const APD_MAPS[APD_SECTORS] = {MAP_0, MAP_1, MAP_2, MAP_3};')
    o.append('')
    for i, r in enumerate(rooms):
        o.append('static const char *const ROOM_%d[] = { /* %s %s */' % (i, SECTORS[r['sector']], r['id']))
        for row in r['rows']:
            o.append('    %s,' % c_str(row))
        o.append('};')
    o.append('')
    o.append('const ApdSpec APD_SPECS[] = {')
    for i, r in enumerate(rooms):
        fl = ' | '.join(FLAGS[f] for f in r['flags']) or '0'
        o.append('    {%d, \'%s\', ROOM_%d, %s, %s, %s},' %
                 (r['sector'], r['id'], i, fl, c_str(r['things']), c_str(r['name']) if r['name'] else 'NULL'))
    o.append('};')
    o.append('const int APD_NSPECS = %d;' % len(rooms))
    o.append('')
    o.append('/* each doorway\'s row in its cell (a shaft: its column) */')
    o.append('const ApdDoorPos APD_DOORPOS[] = {')
    for d in doors:
        o.append('    {%d, %d, %d, %d, %d},' % (d['sector'], d['ax'], d['ay'], 1 if d['v'] else 0, door_pos(d, doorpos)))
    o.append('};')
    o.append('const int APD_NDOORPOS = %d;' % len(doors))
    open(OUT, 'w', encoding='utf-8', newline='\n').write('\n'.join(o) + '\n')


PARTIAL = len(sys.argv) > 1 and sys.argv[1] in ('skel', 'check')
SHOW = (SKEY[sys.argv[2]], sys.argv[3]) if len(sys.argv) > 3 and sys.argv[1] == 'check' else None


def main():
    maps, rooms_txt, doorpos = parse()
    rooms, doors = build(maps, rooms_txt)
    if len(sys.argv) > 1 and sys.argv[1] == 'skel':
        for i, r in enumerate(rooms):
            if not r['rows']:
                print('room %s %s  (%d x %d)' % ('GBCK'[r['sector']], r['id'], r['w'], r['h']))
                print('\n'.join(skeleton(r, i, doors, doorpos)))
                print()
        return
    for i, r in enumerate(rooms):
        if r['rows'] or not PARTIAL:
            try:
                check(r, i, doors, doorpos)
            except Bad as e:
                ERRORS.append(str(e))
    for e in ERRORS:
        print('station: ' + e)
    if ERRORS:
        sys.exit(1)
    if PARTIAL:
        print('station: checked the rooms drawn so far')
        return
    write(maps, rooms, doors, doorpos)
    counts = [sum(1 for r in rooms if r['sector'] == s) for s in range(4)]
    print('station: %d rooms (%s), %d doors, %d cells: written' %
          (len(rooms), ' '.join(map(str, counts)), len(doors), sum(r['w'] * r['h'] for r in rooms)))


main()
