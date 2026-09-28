/* HOMESPUN - the world. Each save's seed makes the Wilds: an 8 x 6 field of
 * screens whose roads form a random tree with extra loops, some of them
 * blocked for the whole save and some that open or close from trip to trip;
 * eleven caves whose contents (the three dungeons among them) are shuffled;
 * four hopstones; the ruins and Mother Loom's lair. The screens' tiles are
 * made again from the seed whenever Wick walks in. The three dungeons are
 * drawn by hand (their layouts never change); which cave leads to which one
 * is the seed's choice. Nothing here is the original's map. */
#include "homespun.h"

HsWorld hw;
const int HS_DX[4] = {1, 0, -1, 0}, HS_DY[4] = {0, 1, 0, -1};

/* ---- foes ------------------------------------------------------------------ */

/*                     name           hp tier hit  speed size */
const HsFoeDef HS_FOE[E_KINDS] = {
    {"SPINNER",        1, 0, 20, 36, 10},
    {"GRUBLET",        2, 0, 20, 10, 10},
    {"SPLOSH",         3, 1, 30,  0, 12},
    {"SKITTER",        3, 1, 30, 40, 12},
    {"STINGLE",        4, 1, 30, 14, 12},
    {"GNATTER",        3, 1, 30, 16, 10},
    {"SPITBLOOM",      4, 1, 30,  0, 12},
    {"GRUMMLE",        5, 1, 30, 10, 14},
    {"LOBBER",         4, 1, 30, 14, 12},
    {"SHELLBACK",      5, 1, 30, 16, 14},
    {"DRIPLET",        3, 1, 30, 24, 10},
    {"CLACKER",        6, 2, 30, 18, 14},
    {"RED CLACKER",    8, 2, 30, 26, 14},
    {"OOZER",          7, 2, 30,  8, 14},
    {"PUFFCAP",        6, 2, 30,  0, 14},
    {"ZIPSNAKE",       6, 2, 30, 56, 12},
    {"BROODER",        8, 2, 30, 22, 14},
    {"TREADLE",       10, 2, 30,  8, 16},
    {"LEECHLING",      6, 2, 30, 18, 10},
    {"GRUMM KING",    24, 3, 30, 12, 26},
    {"THORNMOTHER",   30, 3, 30,  0, 26},
    {"ZIZZIK",        30, 3, 30, 12, 22},
    {"VOLTHOG",       24, 3, 30, 14, 26},
    {"MOTHER LOOM",   30, 3, 30, 14, 26},
    {"MAWBO",        100, 3, 30, 10, 26},
};

/* the letters foes have in the hand-drawn rooms ('o' is a pot) */
static const char FOE_CH[] = "abcdefghijklmnpqrst";

/* ---- hashing ---------------------------------------------------------------- */

static uint32_t mix(uint32_t a, uint32_t b) {
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u + (a << 6) + (a >> 2));
    h ^= h >> 16; h *= 0x85EBCA6Bu; h ^= h >> 13; h *= 0xC2B2AE35u; h ^= h >> 16;
    return h;
}

static void seed_rng(Rng *r, uint32_t a, uint32_t b) { rng_seed(r, ((uint64_t)mix(a, b) << 32) | mix(b, a ^ 0x55u)); }

int hs_neighbour(int s, int dir) {
    int x = s % HS_OW_W + HS_DX[dir], y = s / HS_OW_W + HS_DY[dir];
    if (x < 0 || y < 0 || x >= HS_OW_W || y >= HS_OW_H) return -1;
    return y * HS_OW_W + x;
}

static void link(uint8_t *bits, int s, int dir) {
    int n = hs_neighbour(s, dir);
    if (n < 0) return;
    bits[s] |= (uint8_t)(1 << dir);
    bits[n] |= (uint8_t)(1 << ((dir + 2) & 3));
}

/* a shifting road is open on some trips: decided per edge and per trip */
static bool shift_open(int s, int dir, uint32_t trip) {
    int a = s, o = dir & 1; /* the edge's key: its left/top screen and orientation */
    if (dir == D_LEFT) a = s - 1;
    if (dir == D_UP) a = s - HS_OW_W;
    return (mix(hw.seed ^ 0xB10Cu, (uint32_t)(a * 2 + o) * 7919u + trip * 104729u) >> 7) % 100 < 50;
}

static uint8_t hs_exits_trip(int s, uint32_t trip) {
    uint8_t e = hw.open[s];
    for (int d = 0; d < 4; d++)
        if ((hw.shift[s] >> d & 1) && shift_open(s, d, trip)) e |= (uint8_t)(1 << d);
    return e;
}

uint8_t hs_exits(int s) { return hs_exits_trip(s, sv.excursion); }
bool hs_edge_open(int s, int dir) { return (hs_exits(s) >> dir & 1) != 0; }

/* ---- the world generator ----------------------------------------------------------- */

static int row_of(int s) { return s / HS_OW_W; }
static int col_of(int s) { return s % HS_OW_W; }

void hs_world_make(uint32_t seed) {
    memset(&hw, 0, sizeof hw);
    hw.seed = seed;
    Rng r;
    seed_rng(&r, seed, 1);
    bool flip = rng_range(&r, 0, 1) == 1;  /* marsh west or east; crags west or east */
    bool flip2 = rng_range(&r, 0, 1) == 1;
    for (int s = 0; s < HS_SCREENS; s++) {
        int x = col_of(s), y = row_of(s), b;
        bool west = flip ? x >= 4 : x < 4;
        if (y >= 4) b = B_MEADOW;
        else if (y >= 2) b = (x == 3 || x == 4) ? B_WOODS : (west ? B_MARSH : B_DUNES);
        else b = (flip2 ? x >= 4 : x < 4) ? B_CRAGS : B_ASH;
        /* a few screens borrow a neighbour's look */
        if (y >= 2 && y <= 3 && rng_chance(&r, 15)) b = B_WOODS;
        hw.biome[s] = (uint8_t)b;
        hw.tier[s] = (uint8_t)(y >= 4 ? 0 : y >= 2 ? 1 : 2);
        hw.cave_of[s] = -1;
        hw.pad_of[s] = -1;
    }
    /* the ruins in the top two rows, the lair beside them */
    for (;;) {
        int y = rng_range(&r, 0, 1), x = rng_range(&r, 1, HS_OW_W - 2);
        int d = rng_range(&r, 0, 3);
        if (d == D_DOWN) continue;
        int ru = y * HS_OW_W + x, la = hs_neighbour(ru, d);
        if (la < 0) continue;
        hw.ruins = (uint8_t)ru;
        hw.lair = (uint8_t)la;
        hw.lair_dir = (uint8_t)d;
        break;
    }
    hw.biome[hw.ruins] = B_ASH;
    hw.biome[hw.lair] = B_ASH;
    hw.tier[hw.ruins] = hw.tier[hw.lair] = 2;
    /* a random tree of roads from the screen above camp (the lair left out) */
    uint8_t in[HS_SCREENS] = {0};
    int front[HS_SCREENS * 4][2], nf = 0;
    in[HS_START_SCREEN] = 1;
    in[hw.lair] = 1;
    for (int d = 0; d < 4; d++) { front[nf][0] = HS_START_SCREEN; front[nf][1] = d; nf++; }
    while (nf > 0) {
        int k = rng_range(&r, 0, nf - 1);
        int s = front[k][0], d = front[k][1];
        front[k][0] = front[nf - 1][0]; front[k][1] = front[nf - 1][1];
        nf--;
        int n = hs_neighbour(s, d);
        if (n < 0 || in[n]) continue;
        in[n] = 1;
        link(hw.open, s, d);
        for (int e = 0; e < 4; e++) { front[nf][0] = n; front[nf][1] = e; nf++; }
    }
    link(hw.open, hw.ruins, hw.lair_dir); /* only through the ruins */
    /* extra roads: open, blocked for good, or shifting */
    for (int s = 0; s < HS_SCREENS; s++)
        for (int d = 0; d < 2; d++) { /* right and down: each edge once */
            int n = hs_neighbour(s, d);
            if (n < 0 || s == hw.lair || n == hw.lair) continue;
            if (hw.open[s] >> d & 1) continue;
            int roll = rng_range(&r, 0, 99);
            if (roll < 18) link(hw.open, s, d);
            else if (roll < 40) link(hw.block, s, d);
            else if (roll < 56) link(hw.shift, s, d);
        }
    /* caves */
    uint8_t used[HS_SCREENS] = {0};
    used[HS_START_SCREEN] = used[hw.ruins] = used[hw.lair] = 1;
    for (int c = 0; c < HS_CAVES; c++) {
        int lo = 0, hi = 4;
        if (c == K_DUN1) { lo = 2; hi = 3; }
        if (c == K_DUN2) { lo = 1; hi = 3; }
        if (c == K_DUN3) { lo = 0; hi = 1; }
        int s;
        for (int tries = 0;; tries++) {
            s = rng_range(&r, lo * HS_OW_W, (hi + 1) * HS_OW_W - 1);
            if (!used[s] || tries > 400) break;
        }
        used[s] = 1;
        hw.cave_kind[c] = (uint8_t)c;
        hw.cave_screen[c] = (uint8_t)s;
        hw.cave_of[s] = (int8_t)c;
        int cx;
        do cx = rng_range(&r, 2, HS_TW - 3); while (cx >= 7 && cx <= 12);
        hw.cave_x[s] = (uint8_t)cx;
    }
    /* hopstones: camp's, then one in each quarter of the Wilds */
    hw.pad_screen[0] = 0xFF;
    for (int q = 0; q < 4; q++) {
        int x0 = (q & 1) ? 4 : 0, y0 = (q & 2) ? 3 : 0, y1 = (q & 2) ? 4 : 2;
        int s;
        for (int tries = 0;; tries++) {
            s = rng_range(&r, y0, y1) * HS_OW_W + rng_range(&r, x0, x0 + 3);
            if ((s != hw.lair && s != HS_START_SCREEN && hw.pad_of[s] < 0) || tries > 200) break;
        }
        hw.pad_screen[q + 1] = (uint8_t)s;
        hw.pad_of[s] = (int8_t)(q + 1);
    }
    /* four noodlers in seven places */
    int order[HS_VSPOTS];
    for (int i = 0; i < HS_VSPOTS; i++) order[i] = i;
    for (int i = HS_VSPOTS - 1; i > 0; i--) { int j = rng_range(&r, 0, i), t = order[i]; order[i] = order[j]; order[j] = t; }
    for (int i = 0; i < HS_VSPOTS; i++) hw.vendor_at[i] = 0xFF;
    for (int v = 0; v < V_KINDS; v++) hw.vendor_at[order[v]] = (uint8_t)v;
}

/* ---- the hand-drawn dungeons --------------------------------------------------------------
 * '#' wall  '.' floor  '~' pit  'C' cracked wall (the yo-yo breaks it)
 * 'S' shutter (shut while the boss lives)  'V' a place a noodler may stand
 * 'o' pot  '$' chest  'B' the boss  'P' where the ship part rests
 * a-t (not o) foes, in the order of FOE_CH.
 * A room's doors are the gaps in its edges; the room it leads to is the one
 * next to it on the dungeon's grid. The first room's bottom gap leads out. */

typedef struct DunRoom { int8_t gx, gy; const char *row[HS_TH]; } DunRoom;
typedef struct Dungeon { const char *name; int boss; int n; DunRoom room[HS_DROOMS]; } Dungeon;

#define W20 "####################"
#define TD  "#########..#########"
#define TC  "#########CC#########"
#define TS  "#########SS#########"

static const Dungeon DUN[HS_DUNS] = {
    {"THE BURROW", E_GRUMMKING, 7, {
        /* 0 the way in */
        {1, 2, {TC,
                "#..................#",
                "#..o..........o....#",
                "#.....##....##.....#",
                "..........b.........",
                "....................",
                "#.....##....##.....#",
                "#..................#",
                "#..................#",
                TD}},
        /* 1 west: a noodler's nook */
        {0, 2, {W20,
                "#..................#",
                "#......V...........#",
                "#..............o...#",
                "#..###.............#",
                "#..###..............",
                "#..............o....",
                "#..o...............#",
                "#..................#",
                W20}},
        /* 2 east: grummles in the dark */
        {2, 2, {TD,
                "#.......#..#.......#",
                "#..h....#..#....k..#",
                "#.......#..#.......#",
                "...................#",
                "...................#",
                "#..k..........h....#",
                "#.....o............#",
                "#..................#",
                W20}},
        /* 3 the middle, the crack down to the way in, a crack west */
        {1, 1, {TD,
                "#..................#",
                "#...~~~~....~~~~...#",
                "#...~~~~....~~~~...#",
                "C........j..........",
                "C...................",
                "#...~~~~....~~~~...#",
                "#.b.~~~~....~~~~.b.#",
                "#..................#",
                TD}},
        /* 4 north-east: the long way round */
        {2, 1, {W20,
                "#..V...............#",
                "#..........n.......#",
                "#...######..####...#",
                "....#......b...#...#",
                "....#..j.......#...#",
                "#...####..######...#",
                "#..................#",
                "#......o...........#",
                TD}},
        /* 5 the Grumm King's hall */
        {1, 0, {W20,
                "#..................#",
                "#..##..........##..#",
                "#..................#",
                "#.........B........#",
                "#..................#",
                "#.........P........#",
                "#..##..........##..#",
                "#..................#",
                TS}},
        /* 6 behind the crack: a hidden store */
        {0, 1, {W20,
                "#..................#",
                "#..$.........V.....#",
                "#..................#",
                "#.......~~~~........",
                "#.......~~~~........",
                "#..................#",
                "#..o...........o...#",
                "#..................#",
                W20}},
    }},
    {"BRAMBLE VAULT", E_THORNMOTHER, 10, {
        /* 0 the way in */
        {0, 2, {TD,
                "#..................#",
                "#..o...............#",
                "#......g...........#",
                "#...................",
                "#...................",
                "#...........d......#",
                "#..................#",
                "#..................#",
                TD}},
        /* 1 */
        {1, 2, {W20,
                "#..................#",
                "#...~~~......~~~...#",
                "#...~~~..d...~~~...#",
                "....................",
                "....................",
                "#...~~~..p...~~~...#",
                "#...~~~......~~~...#",
                "#..o...............#",
                W20}},
        /* 2: the crack up is the short way */
        {2, 2, {TC,
                "#..................#",
                "#..e...........e...#",
                "#.....########.....#",
                "......#......#.....#",
                "......#...o..#.....#",
                "#.....###..###.....#",
                "#..................#",
                "#..................#",
                W20}},
        /* 3 */
        {0, 1, {TD,
                "#.......#..#.......#",
                "#..g....#..#....g..#",
                "#.......#..#.......#",
                "#..................#",
                "#.........r........#",
                "#..................#",
                "#..o...............#",
                "#..................#",
                TD}},
        /* 4 north-west: a noodler */
        {0, 0, {W20,
                "#..................#",
                "#..V...............#",
                "#..................#",
                "#....~~~~~~~~.......",
                "#....~~~~~~~~.......",
                "#..............p...#",
                "#..................#",
                "#..................#",
                TD}},
        /* 5: a crack down to a hidden room */
        {1, 0, {W20,
                "#..................#",
                "#..r...........e...#",
                "#....##......##....#",
                "....................",
                "....................",
                "#....##......##....#",
                "#..................#",
                "#..o...............#",
                TC}},
        /* 6 */
        {2, 0, {W20,
                "#..................#",
                "#.......d..d.......#",
                "#..###........###..#",
                "...###...g....###..#",
                "...................#",
                "#..................#",
                "#......p...........#",
                "#..................#",
                TD}},
        /* 7: the hall before the Thornmother */
        {2, 1, {TD,
                "#..................#",
                "#..g............g..#",
                "#..................#",
                "#......~~~~~~.......",
                "#......~~~~~~.......",
                "#..................#",
                "#..e............r..#",
                "#..................#",
                TD}},
        /* 8 the Thornmother's garden */
        {3, 1, {W20,
                "#..................#",
                "#..~~..........~~..#",
                "#.........B........#",
                "S..................#",
                "S..................#",
                "#.........P........#",
                "#..~~..........~~..#",
                "#..................#",
                W20}},
        /* 9 hidden, under the crack */
        {1, 1, {TD,
                "#..................#",
                "#..$...........V...#",
                "#..................#",
                "#......o....o......#",
                "#..................#",
                "#..................#",
                "#..................#",
                "#..................#",
                W20}},
    }},
    {"HIVE SPIRE", E_ZIZZIK, 10, {
        /* 0 the way in, the crack up is the short way */
        {1, 3, {TC,
                "#..................#",
                "#..o............o..#",
                "#.......f..........#",
                "....................",
                "....................",
                "#..........t.......#",
                "#..................#",
                "#..................#",
                TD}},
        /* 1 */
        {0, 3, {TD,
                "#..................#",
                "#...######.........#",
                "#...#....#...l.....#",
                "#...#..q.#..........",
                "#...#....#..........",
                "#...###..#.........#",
                "#......f...........#",
                "#..................#",
                W20}},
        /* 2 */
        {2, 3, {TD,
                "#..................#",
                "#..m...........s...#",
                "#..................#",
                "....~~~~~~~~~~.....#",
                "....~~~~~~~~~~.....#",
                "#..................#",
                "#..o......t........#",
                "#..................#",
                W20}},
        /* 3 west: a noodler */
        {0, 2, {TD,
                "#..................#",
                "#..V...............#",
                "#..........f.......#",
                "#.....#######......#",
                "#.....#.....#......#",
                "#..........q.......#",
                "#..................#",
                "#..o...............#",
                TD}},
        /* 4 */
        {0, 1, {W20,
                "#..................#",
                "#..l..........l....#",
                "#..................#",
                "#......~~~~.........",
                "#......~~~~.........",
                "#..................#",
                "#.....s............#",
                "#..................#",
                TD}},
        /* 5 the landing under the queen's hall */
        {1, 1, {TS,
                "#..................#",
                "#..t............t..#",
                "#.....###..###.....#",
                "....................",
                "....................",
                "#.....###..###.....#",
                "#..................#",
                "#......m...........#",
                TD}},
        /* 6 */
        {2, 1, {W20,
                "#..................#",
                "#..f.......f.......#",
                "#..................#",
                "....#####..........#",
                "....#...#....q.....#",
                "#...#.o.#..........#",
                "#..................#",
                "#..................#",
                TD}},
        /* 7 */
        {2, 2, {TD,
                "#..................#",
                "#.......~~~~.......#",
                "#..l....~~~~....m..#",
                "#..................#",
                "#..................#",
                "#.......~~~~.......#",
                "#..s....~~~~.......#",
                "#..................#",
                TD}},
        /* 8 Zizzik's hall */
        {1, 0, {W20,
                "#..................#",
                "#..................#",
                "#..~~..........~~..#",
                "#.........B........#",
                "#..................#",
                "#.........P........#",
                "#..~~..........~~..#",
                "#..................#",
                TS}},
        /* 9 hidden, above the crack */
        {1, 2, {TD,
                "#..................#",
                "#..V............$..#",
                "#..................#",
                "#......o....o......#",
                "#..................#",
                "#..................#",
                "#..................#",
                "#..................#",
                TD}},
    }},
};

const char *hs_dun_name(int d) { return d >= 0 && d < HS_DUNS ? DUN[d].name : ""; }
int hs_dun_of(int room) { return room >= HS_R_DUN0 && room < HS_R_BASE ? (room - HS_R_DUN0) / HS_DROOMS : -1; }
static int dun_idx(int room) { return (room - HS_R_DUN0) % HS_DROOMS; }
static const DunRoom *dun_room(int room) {
    int d = hs_dun_of(room), i = dun_idx(room);
    if (d < 0 || i >= DUN[d].n) return NULL;
    return &DUN[d].room[i];
}
bool hs_dun_boss_room(int room) {
    const DunRoom *dr = dun_room(room);
    if (!dr) return false;
    for (int y = 0; y < HS_TH; y++)
        if (strchr(dr->row[y], 'B')) return true;
    return false;
}

int hs_dun_neighbour(int room, int dir) {
    const DunRoom *dr = dun_room(room);
    if (!dr) return -1;
    int d = hs_dun_of(room), gx = dr->gx + HS_DX[dir], gy = dr->gy + HS_DY[dir];
    for (int i = 0; i < DUN[d].n; i++)
        if (DUN[d].room[i].gx == gx && DUN[d].room[i].gy == gy) return HS_R_DUN0 + d * HS_DROOMS + i;
    return -1;
}

/* vendor spots are numbered in dungeon order */
static int vendor_spot(int room, int nth) {
    int d = hs_dun_of(room), idx = dun_idx(room), k = 0;
    for (int dd = 0; dd < HS_DUNS; dd++)
        for (int i = 0; i < DUN[dd].n; i++)
            for (int y = 0; y < HS_TH; y++)
                for (const char *c = DUN[dd].room[i].row[y]; *c; c++)
                    if (*c == 'V') {
                        if (dd == d && i == idx && nth-- == 0) return k;
                        k++;
                    }
    return -1;
}

/* ---- caves ---------------------------------------------------------------------------- */

static const char *const CAVE_ROWS[][HS_TH] = {
    /* 0: a keeper's cave (Kit, Shuffle, Hush, Tanger) */
    {W20,
     "#..................#",
     "#..~~..........~~..#",
     "#.........N........#",
     "#..................#",
     "#......2....3......#",
     "#..................#",
     "#..~~..........~~..#",
     "#..................#",
     "########....########"},
    /* 1: the gear cave */
    {W20,
     "#...##........##...#",
     "#..................#",
     "#.........G........#",
     "#....x........x....#",
     "#..................#",
     "#..##....x.....##..#",
     "#..................#",
     "#..................#",
     "########....########"},
    /* 2: a loot cave */
    {W20,
     "#..................#",
     "#..2............3..#",
     "#......~~~~~~......#",
     "#....x.~~~~~~.x....#",
     "#......~~~~~~......#",
     "#..o............o..#",
     "#.......x..........#",
     "#..................#",
     "########....########"},
    /* 3: the smokers' cache */
    {W20,
     "#..................#",
     "#...##...M....##...#",
     "#..................#",
     "#....x........x....#",
     "#..................#",
     "#..o...........o...#",
     "#..................#",
     "#..................#",
     "########....########"},
};

const char *hs_cave_name(int kind) {
    static const char *const N[] = {"THE BURROW", "BRAMBLE VAULT", "HIVE SPIRE", "KIT'S HIDEOUT", "SHUFFLE'S DEN",
                                    "HUSH'S HOLLOW", "TANGER'S NOOK", "THE GEAR CAVE", "A DUSTY CAVE", "A DUSTY CAVE",
                                    "THE SMOKERS' CACHE"};
    return kind >= 0 && kind < HS_CAVES ? N[kind] : "";
}

static int cave_template(int kind) {
    switch (kind) {
    case K_SIS: case K_GAMBLE: case K_HUSH: case K_TANGER: return 0;
    case K_GEAR: return 1;
    case K_LOOT1: case K_LOOT2: return 2;
    default: return 3;
    }
}

int hs_room_area(int room) {
    if (room == HS_R_BASE) return AR_BASE;
    if (room < HS_SCREENS) return AR_OVER;
    if (room < HS_R_DUN0) return AR_CAVE;
    return AR_DUN;
}

const char *hs_room_name(int room) {
    int a = hs_room_area(room);
    if (a == AR_BASE) return "CAMP";
    if (a == AR_CAVE) return hs_cave_name(hw.cave_kind[room - HS_R_CAVE0]);
    if (a == AR_DUN) return hs_dun_name(hs_dun_of(room));
    static const char *const B[] = {"THE MEADOW", "THE MARSH", "THE WOODS", "THE DUNES", "THE CRAGS", "THE ASHLANDS"};
    if (room == hw.ruins) return "THE RUINS";
    if (room == hw.lair) return "THE LAIR";
    return B[hw.biome[room] % 6];
}

/* ---- screens of the Wilds ------------------------------------------------------------------- */

static bool tsolid(int t) { return t == T_WALL || t == T_WATER || t == T_LAVA || t == T_BLOCK || t == T_CRACK || t == T_SHUT; }

static void carve(uint8_t t[HS_TH][HS_TW], int x0, int y0, int x1, int y1) {
    /* an L-shaped corridor two tiles wide, first along x then along y */
    int x = x0, y = y0;
    for (;;) {
        for (int dy = 0; dy < 2; dy++)
            for (int dx = 0; dx < 2; dx++) {
                int cx = x + dx, cy = y + dy;
                if (cx < 1 || cy < 1 || cx > HS_TW - 2 || cy > HS_TH - 2) continue;
                if (t[cy][cx] == T_WALL) t[cy][cx] = T_FLOOR;
                else if (t[cy][cx] == T_WATER || t[cy][cx] == T_LAVA) t[cy][cx] = T_BRIDGE;
            }
        if (x != x1) x += isign(x1 - x);
        else if (y != y1) y += isign(y1 - y);
        else break;
    }
}

/* fill what can't be reached from (sx,sy) with wall; returns tiles reached */
static int seal(uint8_t t[HS_TH][HS_TW], int sx, int sy) {
    uint8_t seen[HS_TH][HS_TW] = {{0}};
    int q[HS_TW * HS_TH], qh = 0, qt = 0, n = 0;
    seen[sy][sx] = 1;
    q[qt++] = sy * HS_TW + sx;
    while (qh < qt) {
        int c = q[qh++], cx = c % HS_TW, cy = c / HS_TW;
        n++;
        for (int d = 0; d < 4; d++) {
            int nx = cx + HS_DX[d], ny = cy + HS_DY[d];
            if (nx < 0 || ny < 0 || nx >= HS_TW || ny >= HS_TH || seen[ny][nx]) continue;
            if (tsolid(t[ny][nx]) || t[ny][nx] == T_CAVE) continue;
            seen[ny][nx] = 1;
            q[qt++] = ny * HS_TW + nx;
        }
    }
    for (int y = 1; y < HS_TH - 1; y++)
        for (int x = 1; x < HS_TW - 1; x++)
            if (!seen[y][x] && (t[y][x] == T_FLOOR || t[y][x] == T_PATH || t[y][x] == T_BRIDGE)) t[y][x] = T_WALL;
    return n;
}

static int blob_tile(int biome, Rng *r) {
    switch (biome) {
    case B_MARSH: return rng_chance(r, 60) ? T_WATER : T_WALL;
    case B_ASH: return rng_chance(r, 45) ? T_LAVA : T_WALL;
    case B_MEADOW: return rng_chance(r, 20) ? T_WATER : T_WALL;
    case B_WOODS: return rng_chance(r, 10) ? T_WATER : T_WALL;
    default: return T_WALL;
    }
}

#define PAD_TX 4
#define PAD_TY 6

static void screen_tiles(int s, uint8_t t[HS_TH][HS_TW]) {
    Rng r;
    seed_rng(&r, hw.seed, 1000u + (uint32_t)s);
    for (int y = 0; y < HS_TH; y++)
        for (int x = 0; x < HS_TW; x++)
            t[y][x] = (x == 0 || y == 0 || x == HS_TW - 1 || y == HS_TH - 1) ? T_WALL : T_FLOOR;
    int clusters = rng_range(&r, 4, 8);
    if (s == HS_START_SCREEN) clusters = 3;
    for (int c = 0; c < clusters; c++) {
        int cx = rng_range(&r, 2, HS_TW - 3), cy = rng_range(&r, 2, HS_TH - 3);
        int tt = blob_tile(hw.biome[s], &r), sz = rng_range(&r, 2, 7);
        for (int k = 0; k < sz; k++) {
            t[cy][cx] = (uint8_t)tt;
            int d = rng_range(&r, 0, 3);
            cx = iclamp(cx + HS_DX[d], 1, HS_TW - 2);
            cy = iclamp(cy + HS_DY[d], 1, HS_TH - 2);
        }
    }
    uint8_t ex = hs_exits(s);
    int ccx = 9, ccy = 4; /* the middle */
    for (int d = 0; d < 4; d++) {
        bool open = (ex >> d & 1) != 0 || (s == HS_START_SCREEN && d == D_DOWN);
        bool blocked = !open && (((hw.block[s] | hw.shift[s]) >> d) & 1);
        bool shut = open && s == hw.ruins && d == hw.lair_dir && !sv.volt_down;
        int tt = shut ? T_SHUT : open ? T_PATH : blocked ? T_BLOCK : T_WALL;
        if (tt == T_WALL) continue;
        for (int k = 0; k < 4; k++) {
            if (d == D_UP) t[0][8 + k] = (uint8_t)tt;
            if (d == D_DOWN) t[HS_TH - 1][8 + k] = (uint8_t)tt;
            if (d == D_LEFT) t[3 + k][0] = (uint8_t)tt;
            if (d == D_RIGHT) t[3 + k][HS_TW - 1] = (uint8_t)tt;
        }
        if (!open) continue;
        if (d == D_UP) { carve(t, 9, 1, ccx, ccy); carve(t, 8, 1, 8, 2); carve(t, 10, 1, 10, 2); }
        if (d == D_DOWN) { carve(t, 9, HS_TH - 3, ccx, ccy); carve(t, 8, HS_TH - 3, 8, HS_TH - 4); carve(t, 10, HS_TH - 3, 10, HS_TH - 4); }
        if (d == D_LEFT) { carve(t, 1, 4, ccx, ccy); carve(t, 1, 3, 2, 3); carve(t, 1, 5, 2, 5); }
        if (d == D_RIGHT) { carve(t, HS_TW - 3, 4, ccx, ccy); carve(t, HS_TW - 3, 3, HS_TW - 4, 3); carve(t, HS_TW - 3, 5, HS_TW - 4, 5); }
    }
    if (hw.cave_of[s] >= 0) {
        int cx = hw.cave_x[s];
        t[0][cx] = T_CAVE;
        t[1][cx] = T_FLOOR;
        carve(t, cx, 1, ccx, ccy);
    }
    if (hw.pad_of[s] >= 0) carve(t, PAD_TX, PAD_TY, ccx, ccy);
    /* the ruins: broken walls; the lair: a web-hung hollow */
    if (s == hw.ruins || s == hw.lair) {
        for (int y = 2; y < HS_TH - 2; y++)
            for (int x = 2; x < HS_TW - 2; x++)
                if (t[y][x] == T_LAVA || t[y][x] == T_WALL) t[y][x] = T_FLOOR;
        if (s == hw.ruins)
            for (int k = 0; k < 4; k++) { t[2][4 + k * 4] = T_WALL; t[HS_TH - 3][4 + k * 4] = T_WALL; }
    }
    carve(t, ccx, ccy, ccx, ccy);
    seal(t, ccx, ccy);
}

void hs_room_tiles(int room, uint8_t t[HS_TH][HS_TW], int *biome) {
    int a = hs_room_area(room);
    if (a == AR_OVER) {
        screen_tiles(room, t);
        if (biome) *biome = hw.biome[room];
        return;
    }
    if (a == AR_BASE) {
        for (int y = 0; y < HS_TH; y++)
            for (int x = 0; x < HS_TW; x++)
                t[y][x] = (x == 0 || y == 0 || x == HS_TW - 1 || y == HS_TH - 1) ? T_WALL : T_FLOOR;
        t[0][9] = t[0][10] = T_PATH; /* the gate to the Wilds */
        if (biome) *biome = B_CAMP;
        return;
    }
    const char *const *rows;
    if (a == AR_CAVE) {
        rows = CAVE_ROWS[cave_template(hw.cave_kind[room - HS_R_CAVE0])];
        if (biome) *biome = B_CAVE;
    } else {
        const DunRoom *dr = dun_room(room);
        rows = dr ? dr->row : CAVE_ROWS[0];
        if (biome) *biome = B_DUN;
    }
    uint16_t op = sv.opened[room];
    for (int y = 0; y < HS_TH; y++)
        for (int x = 0; x < HS_TW; x++) {
            char c = rows[y][x];
            int tt = T_FLOOR;
            if (c == '#') tt = T_WALL;
            else if (c == '~') tt = T_WATER;
            else if (c == 'C') tt = (op & 0x8000) ? T_FLOOR : T_CRACK;
            else if (c == 'S') tt = T_DOOR; /* the sim shuts it while the boss lives */
            t[y][x] = (uint8_t)tt;
        }
}

/* Foes of each biome by tier: [biome][tier][4] */
static const uint8_t BIOME_FOES[6][3][4] = {
    /* meadow */ {{E_SPINNER, E_GRUBLET, E_SPINNER, E_GRUBLET}, {E_SKITTER, E_GNATTER, E_GRUMMLE, E_SPITBLOOM}, {E_CLACKER, E_LEECHLING, E_PUFFCAP, E_ZIPSNAKE}},
    /* marsh */  {{E_SPINNER, E_GRUBLET, E_SPINNER, E_GRUBLET}, {E_SPLOSH, E_DRIPLET, E_GNATTER, E_SPITBLOOM}, {E_OOZER, E_PUFFCAP, E_CLACKER, E_BROODER}},
    /* woods */  {{E_SPINNER, E_GRUBLET, E_SPINNER, E_GRUBLET}, {E_SKITTER, E_SPITBLOOM, E_GRUMMLE, E_SHELLBACK}, {E_BROODER, E_PUFFCAP, E_LEECHLING, E_OOZER}},
    /* dunes */  {{E_SPINNER, E_GRUBLET, E_SPINNER, E_GRUBLET}, {E_STINGLE, E_LOBBER, E_SHELLBACK, E_STINGLE}, {E_ZIPSNAKE, E_CLACKER, E_REDCLACKER, E_TREADLE}},
    /* crags */  {{E_SPINNER, E_GRUBLET, E_SPINNER, E_GRUBLET}, {E_STINGLE, E_SKITTER, E_LOBBER, E_GRUMMLE}, {E_CLACKER, E_REDCLACKER, E_BROODER, E_LEECHLING}},
    /* ash */    {{E_SPINNER, E_GRUBLET, E_SPINNER, E_GRUBLET}, {E_LOBBER, E_STINGLE, E_SHELLBACK, E_DRIPLET}, {E_TREADLE, E_REDCLACKER, E_PUFFCAP, E_ZIPSNAKE}},
};

static bool near_way(int s, int x, int y) {
    /* keep foes off the doorways and the cave mouth */
    if (y <= 2 && x >= 6 && x <= 13) return true;
    if (y >= HS_TH - 3 && x >= 6 && x <= 13) return true;
    if (x <= 3 && y >= 2 && y <= 7) return true;
    if (x >= HS_TW - 4 && y >= 2 && y <= 7) return true;
    if (s >= 0 && hw.cave_of[s] >= 0 && iabs(x - hw.cave_x[s]) <= 2 && y <= 2) return true;
    return false;
}

static bool floorish(int t) { return t == T_FLOOR || t == T_PATH || t == T_BRIDGE; }

static int screen_spawns(int s, HsSpawn *sp, int max) {
    if (s == hw.lair || s == hw.ruins) {
        int n = 0;
        if (s == hw.ruins) {
            if (!sv.volt_down && n < max) sp[n++] = (HsSpawn){E_VOLTHOG, 10, 3};
            if (n < max) sp[n++] = (HsSpawn){E_TREADLE, 3, 3};
            if (n < max) sp[n++] = (HsSpawn){E_TREADLE, 16, 6};
        } else if (!sv.loom_home && n < max) sp[n++] = (HsSpawn){E_LOOM, 10, 3};
        return n;
    }
    uint8_t t[HS_TH][HS_TW];
    screen_tiles(s, t);
    Rng r;
    seed_rng(&r, hw.seed, 5000u + (uint32_t)s);
    int tier = hw.tier[s], b = hw.biome[s] % 6;
    int want = s == HS_START_SCREEN ? 2 : rng_range(&r, 2, tier == 0 ? 3 : 4);
    int n = 0;
    for (int tries = 0; tries < 200 && n < want && n < max; tries++) {
        int kind;
        if (tier == 0) kind = rng_chance(&r, 70) ? BIOME_FOES[b][0][rng_range(&r, 0, 3)] : BIOME_FOES[b][1][rng_range(&r, 0, 3)];
        else if (tier == 1) kind = rng_chance(&r, 85) ? BIOME_FOES[b][1][rng_range(&r, 0, 3)] : BIOME_FOES[b][0][rng_range(&r, 0, 3)];
        else kind = rng_chance(&r, 75) ? BIOME_FOES[b][2][rng_range(&r, 0, 3)] : BIOME_FOES[b][1][rng_range(&r, 0, 3)];
        if (s == HS_START_SCREEN) kind = rng_chance(&r, 50) ? E_SPINNER : E_GRUBLET;
        int x = rng_range(&r, 2, HS_TW - 3), y = rng_range(&r, 2, HS_TH - 3);
        if (kind == E_SPLOSH) {
            /* a fish needs water with dry land beside it */
            if (t[y][x] != T_WATER) continue;
            bool shore = false;
            for (int d = 0; d < 4; d++) shore |= floorish(t[y + HS_DY[d]][x + HS_DX[d]]);
            if (!shore) continue;
        } else if (!floorish(t[y][x]) || near_way(s, x, y)) {
            continue;
        }
        bool clash = false;
        for (int i = 0; i < n; i++) clash |= iabs(sp[i].tx - x) + iabs(sp[i].ty - y) < 3;
        if (clash) continue;
        sp[n++] = (HsSpawn){(uint8_t)kind, (uint8_t)x, (uint8_t)y};
    }
    return n;
}

static int text_spawns(const char *const *rows, HsSpawn *sp, int max, int tier_of_screen, int room) {
    int n = 0;
    Rng r;
    seed_rng(&r, hw.seed, 7000u + (uint32_t)room);
    for (int y = 0; y < HS_TH; y++)
        for (int x = 0; x < HS_TW; x++) {
            char c = rows[y][x];
            const char *p = c ? strchr(FOE_CH, c) : NULL;
            if (c >= 'a' && c <= 't' && p && n < max) sp[n++] = (HsSpawn){(uint8_t)(p - FOE_CH), (uint8_t)x, (uint8_t)y};
            if (c == 'x' && n < max) {
                /* a cave foe: from the tier of the screen outside */
                int b = rng_range(&r, 0, 5), tier = tier_of_screen;
                int kind = BIOME_FOES[b][tier][rng_range(&r, 0, 3)];
                if (kind == E_SPLOSH) kind = E_DRIPLET;
                sp[n++] = (HsSpawn){(uint8_t)kind, (uint8_t)x, (uint8_t)y};
            }
            if (c == 'B' && n < max) {
                int d = hs_dun_of(room);
                if (d >= 0 && !((sv.parts | sv.carry) >> d & 1)) sp[n++] = (HsSpawn){(uint8_t)DUN[d].boss, (uint8_t)x, (uint8_t)y};
            }
        }
    return n;
}

int hs_room_spawns(int room, HsSpawn *sp, int max) {
    int a = hs_room_area(room);
    if (a == AR_OVER) return screen_spawns(room, sp, max);
    if (a == AR_CAVE) {
        int c = room - HS_R_CAVE0;
        return text_spawns(CAVE_ROWS[cave_template(hw.cave_kind[c])], sp, max, hw.tier[hw.cave_screen[c]], room);
    }
    if (a == AR_DUN) {
        const DunRoom *dr = dun_room(room);
        return dr ? text_spawns(dr->row, sp, max, 1, room) : 0;
    }
    return 0;
}

int hs_room_things(int room, HsThingSpawn *ts, int max) {
    int a = hs_room_area(room), n = 0;
    if (a == AR_OVER) {
        if (hw.pad_of[room] >= 0 && n < max) ts[n++] = (HsThingSpawn){TH_PAD, PAD_TX, PAD_TY, (uint8_t)hw.pad_of[room]};
        uint8_t t[HS_TH][HS_TW];
        screen_tiles(room, t);
        Rng r;
        seed_rng(&r, hw.seed, 9000u + (uint32_t)room);
        int pots = rng_range(&r, 0, 2), chest = rng_chance(&r, 14) || room == hw.lair;
        for (int tries = 0; tries < 100 && (pots > 0 || chest) && n < max; tries++) {
            int x = rng_range(&r, 2, HS_TW - 3), y = rng_range(&r, 2, HS_TH - 3);
            if (t[y][x] != T_FLOOR || near_way(room, x, y) || (x == PAD_TX && y == PAD_TY)) continue;
            /* never plug a corridor: keep at least two open sides */
            int open = 0;
            for (int d = 0; d < 4; d++) open += floorish(t[y + HS_DY[d]][x + HS_DX[d]]);
            if (open < 3) continue;
            bool clash = false;
            for (int i = 0; i < n; i++) clash |= iabs(ts[i].tx - x) + iabs(ts[i].ty - y) < 2;
            if (clash) continue;
            if (chest) { ts[n++] = (HsThingSpawn){TH_CHEST, (uint8_t)x, (uint8_t)y, 0}; chest = 0; }
            else { ts[n++] = (HsThingSpawn){TH_POT, (uint8_t)x, (uint8_t)y, 0}; pots--; }
        }
        return n;
    }
    const char *const *rows = NULL;
    int kind = -1;
    if (a == AR_CAVE) {
        kind = hw.cave_kind[room - HS_R_CAVE0];
        rows = CAVE_ROWS[cave_template(kind)];
    } else if (a == AR_DUN) {
        const DunRoom *dr = dun_room(room);
        if (dr) rows = dr->row;
    }
    if (!rows) return 0;
    int vn = 0;
    for (int y = 0; y < HS_TH; y++)
        for (int x = 0; x < HS_TW && n < max; x++) {
            char c = rows[y][x];
            if (c == 'o') ts[n++] = (HsThingSpawn){TH_POT, (uint8_t)x, (uint8_t)y, 0};
            else if (c == '$') ts[n++] = (HsThingSpawn){TH_CHEST, (uint8_t)x, (uint8_t)y, 1};
            else if (c == 'V') {
                int spot = vendor_spot(room, vn++);
                if (spot >= 0 && hw.vendor_at[spot] != 0xFF)
                    ts[n++] = (HsThingSpawn){TH_VENDOR, (uint8_t)x, (uint8_t)y, hw.vendor_at[spot]};
            } else if (c == 'P') {
                int d = hs_dun_of(room);
                ts[n++] = (HsThingSpawn){TH_PART, (uint8_t)x, (uint8_t)y, (uint8_t)d};
            } else if (c == 'G') {
                ts[n++] = (HsThingSpawn){TH_GEAR, (uint8_t)x, (uint8_t)y, 0};
            } else if (c == 'M') {
                ts[n++] = (HsThingSpawn){TH_CACHE, (uint8_t)x, (uint8_t)y, 0};
            } else if (c == 'N') {
                int who = kind == K_SIS ? N_KIT : kind == K_GAMBLE ? N_SHUFFLE : kind == K_HUSH ? N_HUSH : N_TANGER;
                ts[n++] = (HsThingSpawn){TH_NPC, (uint8_t)x, (uint8_t)y, (uint8_t)who};
                if (kind == K_GAMBLE && n < max) ts[n++] = (HsThingSpawn){TH_TABLE, (uint8_t)x, (uint8_t)(y + 2), 0};
            } else if ((c == '2' || c == '3') && (kind == K_SIS || kind == K_LOOT1 || kind == K_LOOT2)) {
                ts[n++] = (HsThingSpawn){TH_CHEST, (uint8_t)x, (uint8_t)y, (uint8_t)(kind == K_SIS ? 2 : 1)};
            }
        }
    return n;
}

/* ---- checks for the tests --------------------------------------------------------------------- */

static int reach_all(uint32_t trip) {
    uint8_t seen[HS_SCREENS] = {0};
    int q[HS_SCREENS], qh = 0, qt = 0;
    seen[HS_START_SCREEN] = 1;
    q[qt++] = HS_START_SCREEN;
    while (qh < qt) {
        int s = q[qh++];
        uint8_t e = hs_exits_trip(s, trip);
        for (int d = 0; d < 4; d++) {
            if (!(e >> d & 1)) continue;
            int n = hs_neighbour(s, d);
            if (n < 0 || seen[n]) continue;
            seen[n] = 1;
            q[qt++] = n;
        }
    }
    return qt;
}

/* the ways into a screen, as tiles just inside it */
static bool screen_sound(int s) {
    uint8_t t[HS_TH][HS_TW];
    screen_tiles(s, t);
    uint8_t ex = hs_exits(s);
    if (s == HS_START_SCREEN) ex |= 1 << D_DOWN;
    int pts[8][2], np = 0;
    if (ex >> D_UP & 1) { pts[np][0] = 9; pts[np][1] = 1; np++; }
    if (ex >> D_DOWN & 1) { pts[np][0] = 9; pts[np][1] = HS_TH - 2; np++; }
    if (ex >> D_LEFT & 1) { pts[np][0] = 1; pts[np][1] = 4; np++; }
    if (ex >> D_RIGHT & 1) { pts[np][0] = HS_TW - 2; pts[np][1] = 4; np++; }
    if (hw.cave_of[s] >= 0) { pts[np][0] = hw.cave_x[s]; pts[np][1] = 1; np++; }
    if (hw.pad_of[s] >= 0) { pts[np][0] = PAD_TX; pts[np][1] = PAD_TY; np++; }
    for (int i = 0; i < np; i++)
        if (tsolid(t[pts[i][1]][pts[i][0]])) return false;
    return true;
}

int hs_world_check(uint32_t seed) {
    HsSave keep = sv;
    hs_world_make(seed);
    int bad = 0;
    for (uint32_t trip = 0; trip < 12 && !bad; trip++) {
        sv.excursion = trip;
        sv.volt_down = 1;
        if (reach_all(trip) != HS_SCREENS) bad = 1;
        for (int s = 0; s < HS_SCREENS && !bad; s++)
            if (!screen_sound(s)) bad = 2 + s;
    }
    /* the lair only through the ruins */
    for (int d = 0; d < 4 && !bad; d++) {
        int n = hs_neighbour(hw.lair, d);
        if (n >= 0 && n != hw.ruins && (((hw.open[hw.lair] | hw.shift[hw.lair]) >> d) & 1)) bad = 100;
    }
    int vendors = 0;
    for (int i = 0; i < HS_VSPOTS; i++) vendors += hw.vendor_at[i] != 0xFF;
    if (vendors != 4 && !bad) bad = 101;
    sv = keep;
    return bad;
}

int hs_dun_check(void) {
    int spots = 0;
    for (int d = 0; d < HS_DUNS; d++) {
        for (int i = 0; i < DUN[d].n; i++) {
            const DunRoom *dr = &DUN[d].room[i];
            int room = HS_R_DUN0 + d * HS_DROOMS + i;
            for (int y = 0; y < HS_TH; y++) {
                if ((int)strlen(dr->row[y]) != HS_TW) return 1000 + d * 100 + i * 10 + y;
                for (const char *c = dr->row[y]; *c; c++) spots += *c == 'V';
            }
            /* every gap in an edge leads to a room whose facing edge has one too */
            for (int dir = 0; dir < 4; dir++) {
                bool gap = false;
                for (int k = 0; k < HS_TW; k++) {
                    char c;
                    if (dir == D_UP) c = dr->row[0][k];
                    else if (dir == D_DOWN) c = dr->row[HS_TH - 1][k];
                    else if (k < HS_TH) c = dr->row[k][dir == D_LEFT ? 0 : HS_TW - 1];
                    else continue;
                    if (c != '#') gap = true;
                }
                int nb = hs_dun_neighbour(room, dir);
                if (i == 0 && dir == D_DOWN) { if (!gap) return 2000 + d * 100 + i; continue; }
                if (gap && nb < 0) return 3000 + d * 100 + i * 10 + dir;
                if (!gap && nb >= 0) {
                    /* the other side must have no gap either */
                    const DunRoom *o = dun_room(nb);
                    int od = (dir + 2) & 3;
                    for (int k = 0; k < HS_TW; k++) {
                        char c;
                        if (od == D_UP) c = o->row[0][k];
                        else if (od == D_DOWN) c = o->row[HS_TH - 1][k];
                        else if (k < HS_TH) c = o->row[k][od == D_LEFT ? 0 : HS_TW - 1];
                        else continue;
                        if (c != '#') return 4000 + d * 100 + i * 10 + dir;
                    }
                }
            }
        }
        /* the boss room can be reached from the way in */
        uint8_t seen[HS_DROOMS] = {0};
        int q[HS_DROOMS], qh = 0, qt = 0;
        seen[0] = 1;
        q[qt++] = 0;
        bool boss = false;
        while (qh < qt) {
            int i = q[qh++], room = HS_R_DUN0 + d * HS_DROOMS + i;
            boss |= hs_dun_boss_room(room);
            for (int dir = 0; dir < 4; dir++) {
                int nb = hs_dun_neighbour(room, dir);
                if (nb < 0) continue;
                int j = (nb - HS_R_DUN0) % HS_DROOMS;
                if (!seen[j]) { seen[j] = 1; q[qt++] = j; }
            }
        }
        if (!boss) return 5000 + d;
        if (qt != DUN[d].n) return 6000 + d;
    }
    if (spots != HS_VSPOTS) return 7000 + spots;
    for (int k = 0; k < (int)ARRAY_LEN(CAVE_ROWS); k++)
        for (int y = 0; y < HS_TH; y++)
            if ((int)strlen(CAVE_ROWS[k][y]) != HS_TW) return 8000 + k * 10 + y;
    return 0;
}

/* What the generator varies, over the first n seeds (bits for the tests):
 * 1 the dungeons' caves move from save to save, 2 the other caves' places
 * too, 4 every world has roadblocks, 8 shifting roads differ between trips
 * of one save, 16 the ruins move, 32 all nineteen foes turn up in a world. */
int hs_world_variety(int n) {
    HsSave keep = sv;
    int bits = 4 | 32;
    uint8_t first_d[HS_DUNS], first_c[HS_CAVES], first_r = 0;
    for (int s = 1; s <= n; s++) {
        hs_world_make((uint32_t)s * 2654435761u);
        sv.excursion = 0;
        sv.volt_down = 0;
        sv.loom_home = 0;
        sv.parts = sv.carry = 0;
        if (s == 1) {
            for (int d = 0; d < HS_DUNS; d++) first_d[d] = hw.cave_screen[d];
            for (int c = 0; c < HS_CAVES; c++) first_c[c] = hw.cave_screen[c];
            first_r = hw.ruins;
        } else {
            for (int d = 0; d < HS_DUNS; d++)
                if (hw.cave_screen[d] != first_d[d]) bits |= 1;
            for (int c = K_SIS; c < HS_CAVES; c++)
                if (hw.cave_screen[c] != first_c[c]) bits |= 2;
            if (hw.ruins != first_r) bits |= 16;
        }
        int blocks = 0;
        for (int i = 0; i < HS_SCREENS; i++) blocks += hw.block[i] != 0;
        if (!blocks) bits &= ~4;
        for (uint32_t t = 1; t < 6; t++)
            for (int i = 0; i < HS_SCREENS; i++)
                if (hs_exits_trip(i, t) != hs_exits_trip(i, 0)) bits |= 8;
        /* the foes a world can show: its screens, caves and dungeons */
        uint32_t seen = 0;
        for (int r = 0; r < HS_R_BASE; r++) {
            if (r >= HS_R_DUN0 && !dun_room(r)) continue;
            HsSpawn sp[HS_MAX_SPAWN];
            int k = hs_room_spawns(r, sp, HS_MAX_SPAWN);
            for (int i = 0; i < k; i++)
                if (sp[i].kind < E_FIRST_BOSS) seen |= 1u << sp[i].kind;
        }
        if (seen != (1u << E_FIRST_BOSS) - 1 && s <= 3) {
            /* fish need water: a world may lack them; check the rest */
            if ((seen | (1u << E_SPLOSH)) != (1u << E_FIRST_BOSS) - 1) bits &= ~32;
        }
    }
    sv = keep;
    hs_world_make(sv.seed);
    return bits;
}
