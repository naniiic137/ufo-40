"""FENNEC FOUNTAIN - builds src/games/fennec/fennec_rooms.c from the rooms the
generator grew (tools/fennec/fnsolve.c, "gen" mode, one file per chapter).

    python tools/fennec/make_rooms.py GEN_DIR

Rooms 1-3 (the teaching rooms) are kept as they are. For every other
chapter the candidates are sorted by the length of their shortest solution
and the shortest ones fill the chapter's rooms in rising order, so each
chapter starts gently and ends hard. Development tool only."""
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
ROOMS_C = os.path.join(ROOT, 'src', 'games', 'fennec', 'fennec_rooms.c')

# chapter spec -> the rooms it fills (1-based)
CHAPTERS = [
    ('A', [4, 5]),
    ('B', [6, 7, 8]),
    ('C', list(range(9, 17))),
    ('D', list(range(17, 29))),
    ('E', list(range(29, 39))),
    ('F', list(range(39, 50))),
    ('G', [50]),
]


def read_candidates(path):
    rooms = []
    if not os.path.exists(path):
        return rooms
    cur = None
    for line in open(path, encoding='utf-8'):
        line = line.rstrip('\n').rstrip('\r')
        if line.startswith('; spec'):
            cur = {'meta': line, 'sol': None, 'rows': []}
            m = re.search(r'len (\d+)', line)
            cur['len'] = int(m.group(1))
        elif line.startswith('; ') and cur is not None and cur['sol'] is None:
            cur['sol'] = line[2:].strip()
        elif line.strip() == '':
            if cur and cur['rows']:
                rooms.append(cur)
            cur = None
        elif cur is not None:
            cur['rows'].append(line)
    if cur and cur['rows']:
        rooms.append(cur)
    return rooms


def old_rooms():
    """name, rows and solution of every room now in fennec_rooms.c"""
    src = open(ROOMS_C, encoding='utf-8').read()
    out = []
    for m in re.finditer(r'\{"([^"]+)",\s*\{([^}]*)\},\s*((?:"[^"]*"\s*)+)\}', src):
        rows = re.findall(r'"([^"]*)"', m.group(2))
        sol = ''.join(re.findall(r'"([^"]*)"', m.group(3)))
        out.append((m.group(1), rows, sol))
    return out


def fill_pockets(rows):
    """floor that nothing can ever reach (walled off from the fennec) becomes
    wall, so the room reads cleanly; pockets holding a piece are kept"""
    h = len(rows)
    g = [list(r) for r in rows]
    w = max(len(r) for r in rows)
    for r in g:
        r.extend(' ' * (w - len(r)))
    open_ch = lambda c: c not in '#PH '
    seen = [[False] * w for _ in range(h)]
    for sy in range(h):
        for sx in range(w):
            if seen[sy][sx] or not open_ch(g[sy][sx]):
                continue
            stack, cells = [(sx, sy)], []
            seen[sy][sx] = True
            while stack:
                x, y = stack.pop()
                cells.append((x, y))
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < w and 0 <= ny < h and not seen[ny][nx] and open_ch(g[ny][nx]):
                        seen[ny][nx] = True
                        stack.append((nx, ny))
            if all(g[y][x] == '.' for x, y in cells):
                for x, y in cells:
                    g[y][x] = '#'
    return [''.join(r).rstrip(' ') for r in g]


def c_str(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def main():
    gen_dir = sys.argv[1]
    old = old_rooms()
    assert len(old) == 50, len(old)
    rooms = [None] * 50
    for i in range(3):
        rooms[i] = (old[i][0], old[i][1], old[i][2])
    for spec, nums in CHAPTERS:
        cands = read_candidates(os.path.join(gen_dir, spec + '.txt'))
        cands = [c for c in cands if c['sol']]
        cands.sort(key=lambda c: c['len'])
        if len(cands) < len(nums):
            print('chapter %s: only %d rooms for %d places; keeping old rooms' % (spec, len(cands), len(nums)))
        # spread: take the longest ones that fit, then order them by length
        pick = cands[-len(nums):] if len(cands) >= len(nums) else cands
        pick.sort(key=lambda c: c['len'])
        for k, n in enumerate(nums):
            if k < len(pick):
                rooms[n - 1] = (old[n - 1][0], fill_pockets(pick[k]['rows']), pick[k]['sol'])
            else:
                rooms[n - 1] = old[n - 1]
    lines = []
    lines.append('/* FENNEC FOUNTAIN - the fifty rooms. Every wall and block placement here is')
    lines.append(' * an original design for UFO 40. Rooms 1-3 teach the push; rooms 4-50 were')
    lines.append(' * grown by our own generator (tools/fennec/fnsolve.c) and every one is solved')
    lines.append(' * breadth-first on the game\'s own rule code. Legend: # wall, . floor,')
    lines.append(' * K the fennec, G the dry spring, S the water stone, 1-4 sandstone, 5 marble,')
    lines.append(' * a-d lapis 1-4, w-y basalt 1-3 (n x n), g gecko, ^ > v < arrows, : a stone')
    lines.append(' * patch, o a plate, | a door, P a palm planter (2 x 2), H a statue (3 x 3).')
    lines.append(' * Each solution is a shortest one, in steps (U R D L; "." lets go of a basalt')
    lines.append(' * push); the tests replay them all. */')
    lines.append('#include "fennec.h"')
    lines.append('')
    lines.append('const RoomDef FN_ROOMS_DEF[FN_ROOMS] = {')
    total = 0
    for i, (name, rows, sol) in enumerate(rooms):
        steps = len(sol.replace('.', ''))
        total += steps
        lines.append('    /* %2d: %d moves */' % (i + 1, steps))
        lines.append('    {%s,' % c_str(name))
        lines.append('     {' + ',\n      '.join(c_str(r) for r in rows) + '},')
        # long solutions: split over lines
        parts = [sol[k:k + 64] for k in range(0, len(sol), 64)]
        lines.append('     ' + '\n     '.join(c_str(p) for p in parts) + '},')
    lines.append('};')
    open(ROOMS_C, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')
    print('total shortest steps', total)


if __name__ == '__main__':
    main()
