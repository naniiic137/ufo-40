/* CHIME CIRCUIT - the eight tracks, and what a race needs from one: the
 * solid shapes, the zones, the boosts and stations, where ships launch, and
 * a distance field to every zone that the running order and the CPU pilots
 * use.
 *
 * Each track fits on one screen: 40 x 22 tiles of 8 px under a 2 px strip.
 *   #  wall            .  air              L  the launcher (in the floor, under the line)
 *   S  the start line  1-9  checkpoints, passed in order before the line counts
 *   x y z  marks on a CPU line only (not needed for a lap)
 *   > < ^ v  boost arrows      *  a pickup station
 * Convex wall corners are cut to 45 degrees when the map is built. The top-left corner stays solid for the HUD.
 * The layouts are our own; see docs/games/19-chime-circuit.md for what each
 * one is for. */
#include "chime.h"

ChmMap chm_map;

/* 1 PRELUDE RING */
static const char *const MAP_PRELUDE[CHM_TH] = {
    "########################################",
    "#####...............2..............#####",
    "####................2....<<.........####",
    "####.........*......2....<<..........###",
    "####................2....<<...........##",
    "##..................2.................##",
    "##.....##########################.....##",
    "##.....##########################.....##",
    "##.....##########################.....##",
    "##.....##########################.....##",
    "##33333##########################11111##",
    "##.....##########################.....##",
    "##.....##########################.....##",
    "##.....##########################.....##",
    "##.....##########################.....##",
    "##.....##########################.....##",
    "##........S...........................##",
    "##........S....>>.....................##",
    "###.......S....>>..........*.........###",
    "####......S....>>...................####",
    "#####.....S........................#####",
    "###########L############################",
};

/* 2 THE DIPPER */
static const char *const MAP_DIPPER[CHM_TH] = {
    "########################################",
    "#####....1..........#####......S...#####",
    "####.....1.......y..#####......S....####",
    "####.....1.......y.....z.......S.....###",
    "####.....1....######.*.z.......S......##",
    "##.......1....######...z.......S......##",
    "##.....####....##########....#L##.....##",
    "##.....####....##########....####.....##",
    "##.....####....##########....####.....##",
    "##.....####.........x........####.....##",
    "##.....####.........x........####.....##",
    "##22222####.........x........####44444##",
    "##.....####.........x........####.....##",
    "##.....##########################.....##",
    "##.....##########################.....##",
    "##.....##########################.....##",
    "##..................3.................##",
    "##..................3.................##",
    "###.................3......*.........###",
    "####................3...............####",
    "#####...............3..............#####",
    "########################################",
};

/* 3 NEEDLE'S EYE */
static const char *const MAP_NEEDLE[CHM_TH] = {
    "########################################",
    "#################...3....###############",
    "################....3.....##############",
    "###############.....3......#############",
    "##############.....####.....############",
    "#####.............######............####",
    "####......*......########............###",
    "###.............##########............##",
    "###.....#########################.....##",
    "###....###########################....##",
    "###....###########################....##",
    "###2222###########################4444##",
    "###....###########################....##",
    "###....###########################....##",
    "###.....#########################.....##",
    "###............##########......S......##",
    "####.....*......########.......S.....###",
    "#####.............####.........S....####",
    "#############.......1......###L#########",
    "###############.....1....###############",
    "################....1...################",
    "########################################",
};

/* 4 HOURGLASS */
static const char *const MAP_HOURGLASS[CHM_TH] = {
    "########################################",
    "########################################",
    "########################################",
    "######....4.....#########.....1....#####",
    "#####.....4......#######......1.....####",
    "####......4.......#####.......1......###",
    "###.......4........###........1.......##",
    "##........4.........#.........1........#",
    "##......######.............######......#",
    "##.....########...........########.....#",
    "##.....#########.........#########.....#",
    "##55555##########...*...##########22222#",
    "##.....#########.........#########.....#",
    "##.....########...........########.....#",
    "##......######.............######......#",
    "##.......S..........#.........3..*.....#",
    "###......S.........###........3.......##",
    "####.....S........#####.......3......###",
    "#####....S.......#######......3.....####",
    "######...S......#########.....3....#####",
    "##########L#############################",
    "########################################",
};

/* 5 FORKED REED */
static const char *const MAP_FORKED[CHM_TH] = {
    "########################################",
    "######..........x.......2..........#####",
    "#####......>>...x.......2...........####",
    "####.......>>...x.......2...*........###",
    "####.......>>...x.......2............###",
    "####............x.......2............###",
    "####.....#######################.....###",
    "####.....#######################.....###",
    "####.....#######################.....###",
    "####............y.......2............###",
    "####............y.......2............###",
    "####............y.......2...*........###",
    "####............y.......2............###",
    "####............y.......2............###",
    "####.....#######################.....###",
    "####11111#######################33333###",
    "####......................S..........###",
    "####......................S...<<.....###",
    "####........*.............S...<<.....###",
    "#####.....................S...<<....####",
    "######....................S........#####",
    "#########################L##############",
};

/* 6 TWIN FLUE */
static const char *const MAP_FLUE[CHM_TH] = {
    "########################################",
    "#####################.......3......#####",
    "####################........3.......####",
    "###################......*..3........###",
    "##################..........3.........##",
    "##################..........3.........##",
    "##################.....##########.....##",
    "##################.....##########22222##",
    "##################.....##########.....##",
    "#####.......S...............1.........##",
    "####........S...............1.........##",
    "###.........S...............1........###",
    "##..........S...............1.......####",
    "##..........S...............1......#####",
    "##.....######L####.....#################",
    "##66666###########44444#################",
    "##..........5..........#################",
    "##..........5..........#################",
    "###.....*...5.........##################",
    "####........5........###################",
    "#####.......5.......####################",
    "########################################",
};

/* 7 CROOKED MILE */
static const char *const MAP_CROOKED[CHM_TH] = {
    "########################################",
    "########################################",
    "#####........#####...2...#####.......###",
    "####..........###....2....###.........##",
    "####..........###....2....###.........##",
    "####...####...###...###...###...###...##",
    "####...####zzz###...###...###xxx###...##",
    "####...####.........###.........###...##",
    "####...####.........###....*....###...##",
    "####...#####.......#####.......####...##",
    "####...############################...##",
    "####...############################...##",
    "####.................2................##",
    "####.................2....*...........##",
    "####.................2................##",
    "####333############################111##",
    "####......S...........................##",
    "####......S...........................##",
    "###.......S................*...........#",
    "####......S...........................##",
    "#####.....S..........................###",
    "###########L############################",
};

/* 8 GRAND OCTAVE */
static const char *const MAP_OCTAVE[CHM_TH] = {
    "########################################",
    "###########.........4.........##########",
    "#########...........4...........########",
    "########............4............#######",
    "########.......###########.......#######",
    "#########5555555#########3333333########",
    "##########.......#######.......#########",
    "###########...................##########",
    "###########...................##########",
    "###########.........*.........##########",
    "###########...................##########",
    "###########...................##########",
    "###...............#####........6......##",
    "###..............#######.......6......##",
    "###.............#########......6......##",
    "###.....########################......##",
    "##22222##########################77777##",
    "##..........1.....<<..........S.......##",
    "##......*...1.....<<..........S.......##",
    "###.........1.....<<..........S.....####",
    "####........1.................S....#####",
    "#############################L##########",
};

/* The last column is the CPUs' pace on each track, in per cent of their
 * own speed limits: slowest on the tutorial loop, where a clean player
 * laps them more than once; lower on TWIN FLUE, which they never get the
 * hang of; highest on the two tracks where the race stays close. */
const ChmTrackDef CHM_TRACK[CHM_TRACKS] = {
    {"PRELUDE RING", "A GENTLE LOOP. TWO BOOST ARROWS.", MAP_PRELUDE, +1, 0, 1, 0, 75, {"S123"}},
    {"THE DIPPER", "DIVE UNDER THE ROCK, OR THREAD ITS SLOT.", MAP_DIPPER, -1, 1, 1, 0, 100, {"Sx1234", "Szy1234"}},
    {"NEEDLE'S EYE", "NARROW ALL THE WAY ROUND.", MAP_NEEDLE, -1, 2, 1, 0, 100, {"S1234"}},
    {"HOURGLASS", "THE FIRST FIGURE EIGHT. MIND THE MIDDLE.", MAP_HOURGLASS, +1, 3, 1, 0, 100, {"S12345"}},
    {"FORKED REED", "HIGH ROAD WITH A BOOST, OR LOW ROAD?", MAP_FORKED, -1, 4, 2, 1, 100, {"S1x23", "S1y23"}},
    {"TWIN FLUE", "TWO CHIMNEYS AND ONE CROSSROADS.", MAP_FLUE, +1, 5, 1, 0, 88, {"S123456"}},
    {"CROOKED MILE", "A MILE OF BENDS.", MAP_CROOKED, +1, 6, 1, 1, 100, {"S1x2z3", "S123"}},
    {"GRAND OCTAVE", "THE FINALE. ONE LONG, LOW STRAIGHT.", MAP_OCTAVE, -1, 7, 1, 0, 102, {"S1234567"}},
};

/* ---- zones ------------------------------------------------------------- */

static int zone_of_char(char ch) {
    if (ch == 'S') return CHM_ZONE_S;
    if (ch >= '1' && ch <= '9') return ch - '0';
    if (ch >= 'x' && ch <= 'z') return 10 + (ch - 'x');
    return -1;
}

int chm_zone_char(int z) {
    if (z == CHM_ZONE_S) return 'S';
    if (z >= 1 && z <= 9) return '0' + z;
    if (z >= 10 && z < CHM_ZONES) return 'x' + (z - 10);
    return '?';
}

/* ---- the solid shapes ------------------------------------------------------ */

bool chm_solid(const void *ctx, int px, int py) {
    const ChmMap *m = (const ChmMap *)ctx;
    if (px < 0 || px >= CHM_TW * CHM_TILE || py < CHM_OY || py >= CHM_OY + CHM_TH * CHM_TILE) return true;
    int y = py - CHM_OY;
    int lx = px & 7, ly = y & 7;
    switch (m->shape[y >> 3][px >> 3]) {
    case SH_AIR: return false;
    case SH_TL: return lx + ly <= 7;
    case SH_TR: return lx >= ly;
    case SH_BL: return lx <= ly;
    case SH_BR: return lx + ly >= 7;
    default: return true;
    }
}

static bool tile_ok(int c, int r) { return c >= 0 && c < CHM_TW && r >= 0 && r < CHM_TH; }

int chm_zone_at(const ChmMap *m, int px, int py) {
    int c = px >> 3, r = (py - CHM_OY) >> 3;
    if (py < CHM_OY || !tile_ok(c, r)) return -1;
    return m->zone[r][c];
}

int chm_boost_at(const ChmMap *m, int px, int py) {
    int c = px >> 3, r = (py - CHM_OY) >> 3;
    if (py < CHM_OY || !tile_ok(c, r)) return 0;
    return m->boost[r][c];
}

/* ---- the distance fields ----------------------------------------------------- */

#define NCELLS (CHM_NW * CHM_NH)
static uint32_t heap[NCELLS * 8];
static int hn;

static void hpush(uint32_t k) {
    int i = hn++;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (heap[p] <= k) break;
        heap[i] = heap[p];
        i = p;
    }
    heap[i] = k;
}

static uint32_t hpop(void) {
    uint32_t top = heap[0], last = heap[--hn];
    int i = 0;
    for (;;) {
        int a = 2 * i + 1, b = a + 1, s = i;
        uint32_t sv = last;
        if (a < hn && heap[a] < sv) { s = a; sv = heap[a]; }
        if (b < hn && heap[b] < sv) { s = b; }
        if (s == i) break;
        heap[i] = heap[s];
        i = s;
    }
    if (hn > 0) heap[i] = last;
    return top;
}

static int nav_px(int i) { return i * CHM_NC + CHM_NC / 2; }
static int nav_py(int j) { return CHM_OY + j * CHM_NC + CHM_NC / 2; }

static int step_cost(const ChmMap *m, int i, int j, bool diag) {
    int c = m->clear[j][i];
    return (diag ? 14 : 10) + (c >= 4 ? 0 : (4 - c) * 6);
}

static void build_field(ChmMap *m, int z) {
    uint16_t *f = m->field[z];
    for (int k = 0; k < NCELLS; k++) f[k] = CHM_FAR;
    hn = 0;
    for (int j = 0; j < CHM_NH; j++)
        for (int i = 0; i < CHM_NW; i++)
            if (m->nav[j][i] && chm_zone_at(m, nav_px(i), nav_py(j)) == z) {
                f[j * CHM_NW + i] = 0;
                hpush((uint32_t)(j * CHM_NW + i));
            }
    while (hn > 0) {
        uint32_t k = hpop();
        int cell = (int)(k & 4095), d = (int)(k >> 12);
        if (d > f[cell]) continue;
        int ci = cell % CHM_NW, cj = cell / CHM_NW;
        for (int dj = -1; dj <= 1; dj++)
            for (int di = -1; di <= 1; di++) {
                if (!di && !dj) continue;
                int ni = ci + di, nj = cj + dj;
                if (ni < 0 || nj < 0 || ni >= CHM_NW || nj >= CHM_NH || !m->nav[nj][ni]) continue;
                bool diag = di && dj;
                if (diag && (!m->nav[cj][ni] || !m->nav[nj][ci])) continue;
                int nd = d + step_cost(m, ni, nj, diag);
                int n = nj * CHM_NW + ni;
                if (nd < f[n] && nd < CHM_FAR) {
                    f[n] = (uint16_t)nd;
                    if (hn < NCELLS * 8) hpush((uint32_t)nd << 12 | (uint32_t)n);
                }
            }
    }
}

uint16_t chm_field_at(const ChmMap *m, int zone, int32_t x, int32_t y) {
    if (zone < 0 || zone >= CHM_ZONES || !m->has_zone[zone]) return CHM_FAR;
    int px = (int)(x >> 8), py = (int)(y >> 8);
    int i = px / CHM_NC, j = (py - CHM_OY) / CHM_NC;
    if (py < CHM_OY) j = -1;
    const uint16_t *f = m->field[zone];
    if (i >= 0 && j >= 0 && i < CHM_NW && j < CHM_NH && m->nav[j][i]) return f[j * CHM_NW + i];
    /* up against a wall: the best cell close by */
    uint16_t best = CHM_FAR;
    for (int r = 1; r <= 3 && best == CHM_FAR; r++)
        for (int dj = -r; dj <= r; dj++)
            for (int di = -r; di <= r; di++) {
                int ni = i + di, nj = j + dj;
                if (ni < 0 || nj < 0 || ni >= CHM_NW || nj >= CHM_NH || !m->nav[nj][ni]) continue;
                uint16_t v = f[nj * CHM_NW + ni];
                if (v < best) best = v;
            }
    return best;
}

/* ---- where ships start ------------------------------------------------------- */

/* The grid: two rows of three just past the line, over the launcher; slot 0
 * is the front. */
void chm_grid_pos(const ChmMap *m, int slot, int32_t *x, int32_t *y) {
    int col = slot / 2, row = slot % 2;
    *x = m->spawn_x + m->dir * (26 - 13 * col) * CHF_ONE;
    *y = m->spawn_y - row * 12 * CHF_ONE;
}

int chm_route_length(const ChmMap *m, int route) {
    int n = m->route_len[route], sum = 0;
    for (int k = 0; k < n; k++) sum += m->gap[m->route[route][k]][m->route[route][(k + 1) % n]];
    return sum;
}

/* ---- building a map ---------------------------------------------------------- */

static bool solid_char(char ch) { return ch == '#' || ch == 'L'; }

void chm_map_build(ChmMap *m, int track) {
    memset(m, 0, sizeof *m);
    const ChmTrackDef *d = &CHM_TRACK[track % CHM_TRACKS];
    m->track = (uint8_t)(track % CHM_TRACKS);
    m->dir = d->dir;
    m->ok = true;
    int zsum_x[CHM_ZONES] = {0}, zsum_y[CHM_ZONES] = {0}, zn[CHM_ZONES] = {0};
    m->launch_c = -1;
    for (int r = 0; r < CHM_TH; r++) {
        if ((int)strlen(d->rows[r]) != CHM_TW) m->ok = false;
        for (int c = 0; c < CHM_TW; c++) {
            char ch = d->rows[r][c];
            m->zone[r][c] = -1;
            m->shape[r][c] = solid_char(ch) ? SH_FULL : SH_AIR;
            int z = zone_of_char(ch);
            if (z >= 0) {
                m->zone[r][c] = (int8_t)z;
                m->has_zone[z] = true;
                zsum_x[z] += c * CHM_TILE + 4;
                zsum_y[z] += CHM_OY + r * CHM_TILE + 4;
                zn[z]++;
                if (z >= 1 && z <= 9 && z > m->ncp) m->ncp = (uint8_t)z;
            }
            if (ch == '>') m->boost[r][c] = 1;
            if (ch == '<') m->boost[r][c] = 2;
            if (ch == '^') m->boost[r][c] = 3;
            if (ch == 'v') m->boost[r][c] = 4;
            if (ch == '*') m->station[r][c] = 1;
            if (ch == 'L') { m->launch_c = c; m->launch_r = r; }
        }
    }
    for (int z = 0; z < CHM_ZONES; z++)
        if (zn[z]) { m->zx[z] = (int16_t)(zsum_x[z] / zn[z]); m->zy[z] = (int16_t)(zsum_y[z] / zn[z]); }
    for (int z = 1; z <= m->ncp; z++) if (!m->has_zone[z]) m->ok = false;
    if (!m->has_zone[CHM_ZONE_S] || m->launch_c < 0) m->ok = false;
    /* HUD corner */
    for (int r = 0; r < 5; r++)
        for (int c = 0; c < 4; c++) if (m->shape[r][c] != SH_FULL) m->ok = false;

    /* cut the wall corners, judging from the plain map */
    static uint8_t base[CHM_TH][CHM_TW];
    memcpy(base, m->shape, sizeof base);
#define SOL(c, r) (!tile_ok(c, r) || base[r][c] != SH_AIR)
    for (int r = 0; r < CHM_TH; r++)
        for (int c = 0; c < CHM_TW; c++) {
            bool u = SOL(c, r - 1), dn = SOL(c, r + 1), l = SOL(c - 1, r), rt = SOL(c + 1, r);
            char ch = d->rows[r][c];
            if (ch == '#') {
                if (!u && !l && dn && rt) m->shape[r][c] = SH_BR;
                else if (!u && !rt && dn && l) m->shape[r][c] = SH_BL;
                else if (!dn && !l && u && rt) m->shape[r][c] = SH_TR;
                else if (!dn && !rt && u && l) m->shape[r][c] = SH_TL;
            }
        }
#undef SOL

    /* where a ship fits */
    for (int j = 0; j < CHM_NH; j++)
        for (int i = 0; i < CHM_NW; i++)
            m->nav[j][i] = !chm_flight_blocked(5, chm_solid, m, nav_px(i) * CHF_ONE, nav_py(j) * CHF_ONE);
    /* room round each cell */
    static int16_t q[NCELLS];
    int qh = 0, qt = 0;
    for (int j = 0; j < CHM_NH; j++)
        for (int i = 0; i < CHM_NW; i++) {
            m->clear[j][i] = m->nav[j][i] ? 255 : 0;
            if (!m->nav[j][i]) q[qt++] = (int16_t)(j * CHM_NW + i);
        }
    while (qh < qt) {
        int cell = q[qh++], ci = cell % CHM_NW, cj = cell / CHM_NW;
        for (int dj = -1; dj <= 1; dj++)
            for (int di = -1; di <= 1; di++) {
                int ni = ci + di, nj = cj + dj;
                if (ni < 0 || nj < 0 || ni >= CHM_NW || nj >= CHM_NH) continue;
                if (m->clear[nj][ni] == 255) {
                    m->clear[nj][ni] = (uint8_t)imin(m->clear[cj][ci] + 1, 200);
                    q[qt++] = (int16_t)(nj * CHM_NW + ni);
                }
            }
    }
    for (int j = 0; j < CHM_NH; j++)
        for (int i = 0; i < CHM_NW; i++)
            if (m->clear[j][i] == 255) m->clear[j][i] = 200;

    for (int z = 0; z < CHM_ZONES; z++)
        if (m->has_zone[z]) build_field(m, z);
    /* from each zone to each other: the nearest of its cells */
    for (int a = 0; a < CHM_ZONES; a++)
        for (int b = 0; b < CHM_ZONES; b++) m->gap[a][b] = CHM_FAR;
    for (int j = 0; j < CHM_NH; j++)
        for (int i = 0; i < CHM_NW; i++) {
            if (!m->nav[j][i]) continue;
            int a = chm_zone_at(m, nav_px(i), nav_py(j));
            if (a < 0) continue;
            for (int b = 0; b < CHM_ZONES; b++)
                if (m->has_zone[b] && m->field[b][j * CHM_NW + i] < m->gap[a][b]) m->gap[a][b] = m->field[b][j * CHM_NW + i];
        }

    /* the lines */
    for (int k = 0; k < CHM_ROUTES && d->routes[k]; k++) {
        const char *s = d->routes[k];
        int n = 0, want = 1;
        for (; s[n] && n < CHM_ROUTE_LEN; n++) {
            int z = zone_of_char(s[n]);
            if (z < 0 || !m->has_zone[z]) { m->ok = false; z = 0; }
            if (z >= 1 && z <= 9) {
                if (z != want) m->ok = false;
                want++;
            }
            m->route[k][n] = (uint8_t)z;
        }
        if (want != m->ncp + 1 || m->route[k][0] != CHM_ZONE_S) m->ok = false;
        m->route_len[k] = (uint8_t)n;
        m->nroutes = (uint8_t)(k + 1);
        for (int e = 0; e < n; e++)
            if (m->gap[m->route[k][e]][m->route[k][(e + 1) % n]] >= CHM_FAR) m->ok = false;
    }

    /* the launcher and the grid */
    if (m->launch_c >= 0) {
        m->spawn_x = (m->launch_c * CHM_TILE + 4) * CHF_ONE;
        m->spawn_y = (CHM_OY + m->launch_r * CHM_TILE - 5) * CHF_ONE;
        if (chm_flight_blocked(CHM_TUNE.half, chm_solid, m, m->spawn_x, m->spawn_y)) m->ok = false;
        for (int k = 0; k < CHM_SHIPS; k++) {
            int32_t x, y;
            chm_grid_pos(m, k, &x, &y);
            if (chm_flight_blocked(CHM_TUNE.half, chm_solid, m, x, y)) m->ok = false;
        }
    }
}
