/* IRONVOW - the seven floors of Blackspire Keep, all drawn for UFO 40, and
 * what stands on them. Only the structure follows the original: seven
 * floors (0-6) in three parts, a short first floor, a throne on every floor
 * from the second, glyphs on the walls of floors 1 to 5, the Wraith Warden
 * at the end of floor 2, the Comb Matron at the end of floor 4, three
 * stones on floor 6 for the pillar of the Pit Regent.
 *
 * The map letters:
 *   #  wall          .  floor         S  the knight arrives here
 *   +  door          L  locked door   |  gate (a lever opens it)
 *   h  a wall with a door's outline (OPEN makes it a door)
 *   ^  spikes (up every other pair of turns)    O  a pit (death)
 *   1 2  wet slime of either group (a switch swaps them)    =  lava
 *   3  wall until the comb shifts, then floor   4  the other way round
 *   >  stairs down   E  the way up (closed until the Regent falls)
 *   T t  a torch, lit or out   F f  a fungus torch   N n  a lantern
 *   M  a glyph on the wall     K  a lever   Q  a gem panel   W  a slime switch
 *   C  chest   m  a lidmaw (it looks like a chest)   w x  sweet and foul wells
 *   Y  throne  P  the pedlar  R  the Amber Knight  I  pillar  Z  the socket
 *   B  the floor's boss   k  an iron key   s  floor of the searched room
 *   e  flames gather here */
#include "ivw.h"

static const char *const F0_ROWS[] = {
    "################",
    "#...#....T#...m#",
    "#S..+.....+....#",
    "#...#.e...#.##.#",
    "##t##.....#.#C.#",
    "#...###.###.####",
    "#.k.^...#..e...#",
    "#...#.#.#.###..#",
    "#####.#...#....#",
    "#.....#####.####",
    "#.###......e...#",
    "#.#C#.####.###.#",
    "#...#.#..#.#...#",
    "###.+.#.>L.#.#.#",
    "#......###.....#",
    "################",
};
static const char *const F1_ROWS[] = {
    "########################",
    "#S...#.......M#.......C#",
    "#....+...e....+..e.....#",
    "#....#........#........#",
    "##T###+####.###....Y...#",
    "#.....#..#...#.#.......#",
    "#.w...#..#.R.#.###+#####",
    "#.....#..+...#.#.....#.#",
    "###+###..#####.#..P..#.#",
    "#......e.#...#.+.....#.#",
    "#.######.#.C.#.#######.#",
    "#.#....#.#...#........e#",
    "#.#.t###.##+##.######+##",
    "#.#.#C.#....K#.#....#..#",
    "#.#.#..#.###.#.#.m..|..#",
    "#.K.#O##.#CM.#.#....#..#",
    "#...#.#..#...h.##+###..#",
    "#.M.#.#.e#####.#.....#.#",
    "#.###...##...#.#.k...#.#",
    "#.#..#...+.C.#.###L###.#",
    "#.#T.#####...#...#.#...#",
    "#....#.....M.t.K.#>#.e.#",
    "#..e.#.........#####...#",
    "########################",
};
static const char *const F2_ROWS[] = {
    "##############M#T##########Q####",
    "#.....#######.......#####......#",
    "#.....#######.....e.+...+....C.#",
    "#S....+.............#####......#",
    "#.....#######.......#####......#",
    "#.....##########+########...e..#",
    "###+############.########......#",
    "###.############.#########+#####",
    "###.############+#######Q#.#####",
    "###.#######sssssssssss##......##",
    "#......####sssssssssss+.......##",
    "#...Y..####ssIssIssIss##......M#",
    "t......###Tssssssssssst#......##",
    "#......####ssIssIssIss##.e..m.##",
    "#.e..m.+..+sssssssssss##......##",
    "#......####sssssssssss#####h####",
    "###+######################....##",
    "###.############Q#########..C.M#",
    "#.......#######....##....#....##",
    "#.......#######....+..P..#######",
    "M.........^^.....k.##....#######",
    "#.....e.#######....##....#######",
    "#.C.....###############.########",
    "#.......###K###########+###t####",
    "####.#####..................####",
    "####.....|..e.............C.####",
    "##########..................####",
    "#################L##############",
    "#############.........##########",
    "#############....B....##########",
    "#############....>....##########",
    "################################",
};
static const char *const F3_ROWS[] = {
    "###F############################",
    "#......###.....###.....##......#",
    "#S.....+....R..+.....P..+...Y..#",
    "#......###.....###.....##......M",
    "#......###...e.###.....##.e....#",
    "###+########+############......#",
    "#.......##..........#########+##",
    "#.......##..........###....##1##",
    "M.......W#.e........###..C.##1##",
    "#.......##........C.###....##1##",
    "f.......##..........###....##1##",
    "###+##########+#########h####1##",
    "#.e........................##1##",
    "#..........................f#1##",
    "#OOOOOOOOOOOOOOOOOOOOOOOOOO##1##",
    "#OOOOOOOOOOOOOOOOOOOOOOOOOO##1##",
    "#..........................##1##",
    "#...................e......##1##",
    "#####+#########+###F#########1##",
    "#........##.........#########1##",
    "#......e.##.........#....####1##",
    "#..w.....##....x....h....M###1##",
    "W........##.........#..C.####1##",
    "#.....C..##.e....m..#....####1##",
    "#........##.........#########1##",
    "####.########################1##",
    "####.#################........##",
    "####....22222222222....e......##",
    "######################........##",
    "######################..C...>.##",
    "######################........##",
    "################################",
};
static const char *const F4_ROWS[] = {
    "############F###########M#######",
    "#......###......###...........##",
    "#......###...P..###.........C.##",
    "#S.....+........+.............##",
    "#......###.e....###...........##",
    "#......###......###.e.........##",
    "###.###############...........##",
    "###.#################K##.#######",
    "###.####################.#######",
    "###.####################.#######",
    "###3##K#################4##K####",
    "#sssssssss########............##",
    "#sssssssss########.......Y....##",
    "#ssIssIsss########............##",
    "Msssssssss......3.............##",
    "#ssIssIsss########..e.........f#",
    "#sesssssss########...111......##",
    "#sssssssss########...111....C.##",
    "#####4############............##",
    "#............###################",
    "#............###################",
    "#............####.............C#",
    "#.........m.....3..............#",
    "#............####...........e..#",
    "#.....e......####..............#",
    "M............##########+########",
    "#......111...######.........####",
    "#......111...K#####....B....####",
    "#.C..........######.........####",
    "#............######....>....####",
    "###################.........####",
    "################################",
};
static const char *const F5_ROWS[] = {
    "###n##############N#############",
    "#......#................e.....##",
    "#S.....+....R.................M#",
    "#......#.......e...........P..##",
    "#......#.C....................##",
    "#......###############.#########",
    "#=====================.========#",
    "#=====================.========#",
    "#=====================.========#",
    "######=#######K#######.#########",
    "#..........##.................##",
    "#..........##...............C.##",
    "#....Y.....##.................##",
    "#..........##.................M#",
    "N..........+......e...........##",
    "#..........##.................##",
    "#.w.....e..##...........x.....##",
    "#..........##.............e...##",
    "#..........##.................##",
    "########|#######################",
    "#=======.======================#",
    "#=======.======================#",
    "########.#######################",
    "#............##...............##",
    "#............##...............##",
    "#............##...............##",
    "#.....e......##...............n#",
    "#............+......e.........##",
    "#............##...............##",
    "#.C..........##.C..........>..##",
    "#............##...............##",
    "################################",
};
static const char *const F6_ROWS[] = {
    "###############N################",
    "#......######......#####sssssss#",
    "#..P...+.......S.......+ssIsIss#",
    "#......######......#####ssssses#",
    "#......######......#####sssssss#",
    "#......########.################",
    "###############.##########O#####",
    "###############.##########h#####",
    "#........######.#######........#",
    "#.k...e..######.#######........#",
    "#........######+#######........#",
    "#........##..........##........#",
    "#.........+.Y........##.e......#",
    "#........##..........+.........#",
    "#========##..........##........#",
    "#========##....Z.....######h####",
    "#========##..........###.......#",
    "#........##..........###.......#",
    "#........##..........###.......#",
    "#........##..........###.......#",
    "n........####+###E######....C..#",
    "#...C....####.##########.e.....#",
    "#.....e..####.##########.......#",
    "#........####L##################",
    "########................########",
    "########.............e..########",
    "########..e.............########",
    "########................########",
    "########............C...########",
    "########................########",
    "########................########",
    "###############N################",
};

/* ---- what each floor's levers do, chests hold and glyphs teach (map order) ---- */
static const IvwLever F1_LEVERS[] = {{LV_GATE, 20, 14}, {LV_BOULDER, 5, 15}, {LV_TRAP, 0, 0}};
static const IvwLever F2_LEVERS[] = {{LV_GATE, 9, 25}};
static const IvwLever F4_LEVERS[] = {{LV_SHIFT, 0, 0}, {LV_SHIFT, 0, 0}, {LV_SHIFT, 0, 0}, {LV_SHIFT, 0, 0}};
static const IvwLever F5_LEVERS[] = {{LV_GATE, 8, 19}};

#define SH(n) {IVW_SHARDS, n}
#define IT(i) {i, 0}
#define RND {IVW_RANDOM, 0}
static const IvwChest F0_CHESTS[] = {SH(4), IT(IT_RED), IT(IT_OOZESKULL)};
static const IvwChest F1_CHESTS[] = {IT(IT_RED), IT(IT_TUNIC), IT(IT_CUDGEL), IT(IT_RED), IT(IT_ARMING), IT(IT_OAK)};
static const IvwChest F2_CHESTS[] = {RND, RND, RND, IT(IT_GLAIVE), RND, IT(IT_RED)};
static const IvwChest F3_CHESTS[] = {IT(IT_RED), RND, IT(IT_PADDED), RND, RND, IT(IT_BLUE)};
static const IvwChest F4_CHESTS[] = {RND, IT(IT_MAUL), IT(IT_RED), RND, RND};
static const IvwChest F5_CHESTS[] = {RND, IT(IT_WYRM), RND, IT(IT_RED)};
static const IvwChest F6_CHESTS[] = {IT(IT_MOSS), IT(IT_CINDER), IT(IT_FROST)};

static const uint8_t F1_MURALS[] = {SP_CHART, SP_LANCE, SP_OPEN, SP_MEND};
static const uint8_t F2_MURALS[] = {SP_KINDLE, SP_GALE, SP_VEIL, SP_MIRE};
static const uint8_t F3_MURALS[] = {SP_FLOAT, SP_RESTORE, SP_RIME};
static const uint8_t F4_MURALS[] = {SP_FLARE, SP_QUICKEN, SP_THUNDER};
static const uint8_t F5_MURALS[] = {SP_HUSH, SP_TWIN};
static const uint8_t F2_GEMS[] = {0, 1, 2};

#define N(a) a, ARRAY_LEN(a)
#define NONE NULL, 0
const IvwFloorDef IVW_FLOOR[IVW_FLOORS] = {
    {"THE CELLS", BIO_GAOL, 16, 16, F0_ROWS, NONE, N(F0_CHESTS), NONE, NONE, 2, BOSS_NONE, 0, 0},
    {"THE OUBLIETTES", BIO_GAOL, 24, 24, F1_ROWS, N(F1_LEVERS), N(F1_CHESTS), N(F1_MURALS), NONE, 3, BOSS_NONE, 0, 150},
    {"THE TORCH HALLS", BIO_GAOL, 32, 32, F2_ROWS, N(F2_LEVERS), N(F2_CHESTS), N(F2_MURALS), N(F2_GEMS), 3,
     BOSS_WARDEN, IT_OATH, 300},
    {"THE WET WARREN", BIO_WARREN, 32, 32, F3_ROWS, NONE, N(F3_CHESTS), N(F3_MURALS), NONE, 4, BOSS_NONE, 0, 450},
    {"THE SHIFTING COMB", BIO_WARREN, 32, 32, F4_ROWS, N(F4_LEVERS), N(F4_CHESTS), N(F4_MURALS), NONE, 4,
     BOSS_MATRON, IT_EGG, 600},
    {"THE EMBER GALLERY", BIO_STAR, 32, 32, F5_ROWS, N(F5_LEVERS), N(F5_CHESTS), N(F5_MURALS), NONE, 4, BOSS_NONE,
     0, 750},
    {"THE REGENT'S VAULT", BIO_STAR, 32, 32, F6_ROWS, NONE, N(F6_CHESTS), NONE, NONE, 4, BOSS_REGENT, IVW_RANDOM, 0},
};

/* ---- loading a floor ---------------------------------------------------------- */

static void add_obj(int kind, int x, int y, int param, int uses) {
    IvwWorld *w = &ig.w;
    if (w->nobj >= IVW_MAX_OBJ) return;
    w->obj[w->nobj++] = (IvwObj){(uint8_t)kind, (uint8_t)x, (uint8_t)y, 0, (uint8_t)param, (uint8_t)uses};
}

static char map_char(const IvwFloorDef *fd, int x, int y) {
    return (x >= 0 && y >= 0 && x < fd->w && y < fd->h) ? fd->rows[y][x] : '#';
}

void ivw_floor_load(int floor) {
    const IvwFloorDef *fd = &IVW_FLOOR[floor];
    IvwWorld *w = &ig.w;
    memset(w, 0, sizeof *w);
    int nm = 0, nk = 0, nq = 0, nc = 0;
    for (int y = 0; y < IVW_H; y++)
        for (int x = 0; x < IVW_W; x++) {
            char c = map_char(fd, x, y);
            int t = T_FLOOR, st = 0, id = 0;
            switch (c) {
            case '#': t = T_WALL; break;
            case '+': t = T_DOOR; break;
            case 'L': t = T_LOCKED; break;
            case '|': t = T_GATE; break;
            case 'h': t = T_HIDDEN; break;
            case '^': t = T_SPIKES; break;
            case 'O': t = T_PIT; break;
            case '1': t = T_SLIME_A; break;
            case '2': t = T_SLIME_B; break;
            case '=': t = T_LAVA; break;
            case '>': t = T_STAIRS; break;
            case '3': t = T_SHIFT_A; break;
            case '4': t = T_SHIFT_B; break;
            case 'E': t = T_EXIT; break;
            case 'T': case 't': t = T_TORCH; st = c == 'T'; break;
            case 'F': case 'f': t = T_FUNGUS; st = c == 'F'; break;
            case 'N': case 'n': t = T_LANTERN; st = c == 'N'; break;
            case 'M': t = T_MURAL; id = nm < fd->nmurals ? fd->murals[nm] : 0; nm++; break;
            case 'K': t = T_LEVER; id = nk++; break;
            case 'Q': t = T_GEMS; id = nq < fd->ngems ? fd->gems[nq] : 0; nq++; break;
            case 'W': t = T_SWITCH; break;
            case 's': st = 0x80; break;
            case 'e':
                if (w->nspawn < 16) {
                    w->spawn_x[w->nspawn] = (uint8_t)x;
                    w->spawn_y[w->nspawn] = (uint8_t)y;
                    w->nspawn++;
                }
                break;
            case 'C': add_obj(OB_CHEST, x, y, nc++, 0); break;
            case 'm': add_obj(OB_MIMIC, x, y, nc++, 0); break;
            case 'w': add_obj(OB_SWEET_WELL, x, y, 0, 3); break;
            case 'x': add_obj(OB_FOUL_WELL, x, y, 0, 3); break;
            case 'Y': add_obj(OB_THRONE, x, y, 0, 1); break;
            case 'P': add_obj(OB_PEDLAR, x, y, 0, 0); break;
            case 'R': add_obj(OB_KNIGHT, x, y, 0, 0); break;
            case 'I': add_obj(OB_PILLAR, x, y, 0, 0); break;
            case 'Z': add_obj(OB_SOCKET, x, y, 0, 0); break;
            case 'B': add_obj(OB_BOSS, x, y, fd->boss, 0); break;
            case 'k':
                if (w->ndrop < IVW_MAX_DROP) w->drop[w->ndrop++] = (IvwDrop){IT_IRONKEY, (uint8_t)x, (uint8_t)y};
                break;
            default: break;
            }
            w->tile[y][x] = (uint8_t)t;
            w->st[y][x] = (uint8_t)st;
            w->id[y][x] = (uint8_t)id;
        }
    /* the knight is not here once he has left for good, nor is a used throne */
    for (int i = 0; i < w->nobj; i++) {
        if (w->obj[i].kind == OB_KNIGHT && ig.hero.knight_gone) w->obj[i].kind = OB_NONE;
        if (w->obj[i].kind == OB_THRONE && (ig.hero.thrones_used & (1 << floor))) w->obj[i].uses = 0;
    }
    /* the pedlar's stock: fixed on the first floor she keeps shop, then random */
    if (floor == 1) {
        static const uint8_t FIXED[4] = {IT_BOAR, IT_RED, IT_FOIL, IT_GREY};
        memcpy(w->shop, FIXED, 4);
    } else {
        for (int i = 0; i < 4; i++) w->shop[i] = (uint8_t)ivw_random_item(floor, &ig.rng);
    }
}

/* the start tile, and the way the knight faces: the open way, east first */
void ivw_floor_start(int floor, int *x, int *y, int *dir) {
    const IvwFloorDef *fd = &IVW_FLOOR[floor];
    *x = 1; *y = 1; *dir = 1;
    for (int yy = 0; yy < fd->h; yy++)
        for (int xx = 0; xx < fd->w; xx++)
            if (fd->rows[yy][xx] == 'S') { *x = xx; *y = yy; }
    static const int ORDER[4] = {1, 2, 0, 3};
    for (int k = 0; k < 4; k++) {
        int d = ORDER[k];
        char c = map_char(fd, *x + IVW_DX[d], *y + IVW_DY[d]);
        if (c != '#' && !strchr("TtFfNnMKQW", c)) { *dir = d; return; }
    }
}

/* ---- who comes out of a flame ------------------------------------------------------ */
typedef struct { uint8_t kind, weight; } PoolEntry;
static const PoolEntry POOL0[] = {{FK_OOZE, 3}, {FK_GNAW, 2}};
static const PoolEntry POOL1[] = {{FK_OOZE, 2}, {FK_GNAW, 2}, {FK_TUSKER, 2}};
static const PoolEntry POOL2[] = {{FK_OOZE, 1}, {FK_GNAW, 2}, {FK_TUSKER, 2}, {FK_SKINK, 1}, {FK_BONE, 2}};
static const PoolEntry POOL3[] = {{FK_SKINK, 2}, {FK_EGG, 1}, {FK_GRUB, 2}, {FK_CHITIN, 2}, {FK_PUFF, 1}};
static const PoolEntry POOL4[] = {{FK_SKINK, 1}, {FK_EGG, 2}, {FK_GRUB, 2}, {FK_CHITIN, 2}, {FK_PUFF, 2}};
static const PoolEntry POOL5[] = {{FK_SKINK, 1}, {FK_COILER, 2}, {FK_PUPPET, 2}, {FK_MUMMER, 1}, {FK_BONE, 1},
                                  {FK_GNAW, 1}};
static const PoolEntry POOL6[] = {{FK_COILER, 2}, {FK_PUPPET, 1}, {FK_MUMMER, 1}, {FK_BONE, 1}, {FK_GNAW, 1},
                                  {FK_GRASPER, 2}, {FK_SKINK, 1}};
static const struct { const PoolEntry *e; int n, lo, hi; } POOLS[IVW_FLOORS] = {
    {POOL0, ARRAY_LEN(POOL0), 1, 2}, {POOL1, ARRAY_LEN(POOL1), 1, 3}, {POOL2, ARRAY_LEN(POOL2), 2, 3},
    {POOL3, ARRAY_LEN(POOL3), 2, 4}, {POOL4, ARRAY_LEN(POOL4), 2, 4}, {POOL5, ARRAY_LEN(POOL5), 2, 4},
    {POOL6, ARRAY_LEN(POOL6), 2, 4},
};

int ivw_floor_pool(int floor, Rng *r, uint8_t *kinds, int max) {
    int n = rng_range(r, POOLS[floor].lo, POOLS[floor].hi);
    if (n > max) n = max;
    int total = 0;
    for (int i = 0; i < POOLS[floor].n; i++) total += POOLS[floor].e[i].weight;
    bool silver = floor >= 3 && ig.hero.beetle_fights >= 12;
    for (int k = 0; k < n; k++) {
        if (silver && rng_chance(r, 15)) { kinds[k] = FK_SILVER; continue; }
        int roll = rng_range(r, 0, total - 1);
        for (int i = 0; i < POOLS[floor].n; i++) {
            if (roll < POOLS[floor].e[i].weight) { kinds[k] = POOLS[floor].e[i].kind; break; }
            roll -= POOLS[floor].e[i].weight;
        }
    }
    return n;
}

/* ---- what a random chest, a drop or the pedlar's shelf holds -------------------------- */
static const uint8_t TIER0[] = {IT_RED, IT_RED, IT_BLUE, IT_GREY, IT_TUNIC, IT_FOIL, IT_OAK, IT_HEART};
static const uint8_t TIER1[] = {IT_FOIL, IT_CUDGEL, IT_ARMING, IT_STAR, IT_GLAIVE, IT_BROAD, IT_JERKIN, IT_PADDED,
                                IT_OAK, IT_BOAR, IT_RED, IT_RED, IT_BLUE, IT_GREY, IT_BLACK, IT_YELLOW, IT_GREEN,
                                IT_COWL};
static const uint8_t TIER2[] = {IT_STAR, IT_GLAIVE, IT_BROAD, IT_MAUL, IT_AXE, IT_PADDED, IT_BRONZE, IT_IRON, IT_BOAR,
                                IT_WYRM, IT_RED, IT_RED, IT_BLUE, IT_BLUE, IT_GREY, IT_BLACK, IT_YELLOW, IT_GREEN};
static const uint8_t TIER3[] = {IT_MAUL, IT_AXE, IT_GLAIVE, IT_IRON, IT_STEEL, IT_WYRM, IT_BOAR, IT_RED, IT_RED,
                                IT_RED, IT_BLUE, IT_BLUE, IT_GREY, IT_BLACK, IT_YELLOW, IT_GREEN};

int ivw_random_item(int floor, Rng *r) {
    const uint8_t *t;
    int n;
    if (floor <= 1) { t = TIER0; n = ARRAY_LEN(TIER0); }
    else if (floor == 2) { t = TIER1; n = ARRAY_LEN(TIER1); }
    else if (floor <= 4) { t = TIER2; n = ARRAY_LEN(TIER2); }
    else { t = TIER3; n = ARRAY_LEN(TIER3); }
    return t[rng_range(r, 0, n - 1)];
}
