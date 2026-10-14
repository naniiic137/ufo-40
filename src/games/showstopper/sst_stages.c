/* SHOWSTOPPER - the five stages. Every row is our own: the structure
 * follows the original (a scrolling snow village with a mini-boss and a
 * boss; one screen on a moving train's roof; a longer town in two halves
 * with a chase up a winding road and a boss; the longest stage, a palace in
 * two halves with two mini-bosses and the magician; a lift ride and the
 * last forms), the streets, houses, pits and foes are ours.
 * The key to the letters is in sst.h. Rows run from the top of the stage
 * (the end) to the bottom (the start). */
#include "sst.h"

/* ---- I: Frostbell Lane --------------------------------------------------------- */
static const char *const S1[] = {
    /* Granny Goggle's stage */
    "####################",
    "#TT......##......TT#",
    "#T.......9........T#",
    "#..................#",
    "#..c............b..#",
    "#..................#",
    "#..................#",
    "#.......~~~~.......#",
    "#.......~~~~.......#",
    "#T................T#",
    "#TT..............TT#",
    "###T....,,,,....T###",
    /* the square under the clock */
    "##R.....,,,,.....R##",
    "#.L.....,,,,.....L.#",
    "#...c...,,,,...b...#",
    "TT......,,,,......TT",
    "T..~~~~~~~~~~~~~~..T",
    "T..~~~oo~~~~oo~~~..T",
    "T..~~~oo~~~~oo~~~..T",
    "T..~~~~~~~~~~~~~~..T",
    "T...g..........g...T",
    "####WW#..,,..#WW####",
    "####vR#..,,..#Rv####",
    "#.......,,,,.......#",
    "#..j....,,,,....j..#",
    "#.......,,,,.......#",
    "#..T....,,,,....T..#",
    "#.......,,,,....$..#",
    "G......##..##......G",
    "#......#....#......#",
    "#..s...#.c..#...s..#",
    "#......##..##......#",
    "#.........,........#",
    "TTT....ooo,ooo....TT",
    "TT.....ooo,ooo.....T",
    "T......ooo,ooo.....T",
    "T..b...........c...T",
    "T......g....g......T",
    /* the frozen pond */
    "####...~~~~~~...####",
    "##R...~~~~~~~~...R##",
    "#....~~~wwww~~~....#",
    "#...~~~wwwwww~~~...#",
    "#...~~~wwwwww~~~...#",
    "#....~~~wwww~~~....#",
    "#..L..~~~~~~~~..L..#",
    "#......~~~~~~......#",
    "#..j............j..#",
    "####...######...####",
    "####...#WWW##...####",
    "####...#vRv##...####",
    "#..................#",
    "#...g..........g...#",
    "#..................#",
    "TT....TT....TT....TT",
    "T..................T",
    "T..s.....$.....s...T",
    "T..................T",
    /* the Snow Engine's yard */
    "#####..........#####",
    "#oo..............oo#",
    "#oo......1.......oo#",
    "#oo..............oo#",
    "#oo..............oo#",
    "#oo..............oo#",
    "#oo..............oo#",
    "#oo..............oo#",
    "#oo..............oo#",
    "#oo..............oo#",
    "#oo..............oo#",
    "#####....,,....#####",
    /* the high street */
    "##R......,,......R##",
    "#.L......,,......L.#",
    "#........,,........#",
    "####.....,,.....####",
    "#WW#..j..,,..j..#WW#",
    "#vR#.....,,.....#Rv#",
    "#........,,........#",
    "#..c.....,,.....b..#",
    "#..T.....,,.....T..#",
    "#.......o,,o.......#",
    "#..g....o,,o....g..#",
    "#.......o,,o.......#",
    "####G...,,,,...G####",
    "#.......,,,,.......#",
    "#..s....,,,,....s..#",
    "#.......,,,,.......#",
    "TT...ooooooooooo..TT",
    "T....ooooooooooo...T",
    "T..................T",
    "T..j.....c......j..T",
    "T..................T",
    /* the chapel corner */
    "######......########",
    "#WWWW#......#WWWWWW#",
    "#vRRv#......#RRvRRv#",
    "#.......L.....L....#",
    "#..................#",
    "#..g....~~~~~...g..#",
    "#.......~~~~~......#",
    "#..T....~~~~~...T..#",
    "#..................#",
    "##......,,,,.....###",
    "#G......,,,,......G#",
    "#.....bb,,,,c......#",
    "#.......,,,,.......#",
    "T..oo...,,,,..oo...T",
    "T..oo...,,,,..oo...T",
    "T..s....,,,,...s...T",
    "TT......,,,,......TT",
    /* the edge of the village */
    "####....,,,,...#####",
    "##R.....,,,,....R###",
    "#..L....,,,,...L...#",
    "#..................#",
    "#..g....c.b....g...#",
    "#..................#",
    "TT.TT..........TT.TT",
    "T..................T",
    "T....j........j....T",
    "T..................T",
    "T....TT......TT....T",
    "T..................T",
    "T..g............g..T",
    "T..................T",
    "TT......,,,,......TT",
    "TTT.....,,,,.....TTT",
    "TT......,,,,......TT",
    "T.......,,,,.......T",
    "T...c...,,,,...b...T",
    "T.......,,,,.......T",
    "T.......,,,,.......T",
    "TT......,,,,......TT",
    "TTT.....,,,,.....TTT",
    "TT......,,,,......TT",
    "T.......,,,,.......T",
    "T.........P........T",
    "T.......,,,,.......T",
    "TTTTTTTT,,,,TTTTTTTT",
};

/* ---- II: the Night Freight (one screen on the roof) ------------------------- */
static const char *const S2[] = {
    "oooooooooooooooooooo",
    "oooooooooooooooooooo",
    "oooooooooooooooooooo",
    "..$...o......o......",
    "......o......o......",
    ".........P..........",
    "......o......o......",
    "......o......o......",
    "oooooooooooooooooooo",
    "oooooooooooooooooooo",
    "oooooooooooooooooooo",
};

/* ---- III: Lanternbury ---------------------------------------------------------------- */
static const char *const S3[] = {
    /* the Doppels' bridge */
    "####################",
    "#..................#",
    "#......9...........#",
    "#..................#",
    "#oooo..........oooo#",
    "#oooo..........oooo#",
    "#oooooo======oooooo#",
    "#oooooo======oooooo#",
    "#oooo..........oooo#",
    "#..................#",
    "#..................#",
    "####.....,,.....####",
    /* the lamplighters' row */
    "#WW#.....,,.....#WW#",
    "#vR#..r..,,..r..#Rv#",
    "#........,,........#",
    "#..c.....,,.....b..#",
    "##k##....,,....##k##",
    "#........,,........#",
    "#..h.............h.#",
    "#........,,........#",
    "#..n....~~~~....n..#",
    "#.......~~~~.......#",
    "#..T....~~~~....T..#",
    "#........,,........#",
    "G........,,........G",
    "#..L.....,,.....L..#",
    "#.......o,,o.......#",
    "#..s....o,,o....s..#",
    "#.......o,,o.......#",
    "####....,,,,....####",
    "#WW#..j.,,,,.j..#WW#",
    "#vR#....,,,,....#Rv#",
    "#.......,,,,.......#",
    "#..r....,,,,....r..#",
    "#.......,,,,...$...#",
    /* the canal */
    "###...wwwwwwww...###",
    "#.....wwwwwwww.....#",
    "#..n..ww====ww..n..#",
    "#.....ww====ww.....#",
    "#.....wwwwwwww.....#",
    "##k##..........##k##",
    "#..................#",
    "#..g....c..b....g..#",
    "#..................#",
    "#...h..........h...#",
    "#..................#",
    "TT......,,,,......TT",
    "T..L....,,,,....L..T",
    "T.......,,,,.......T",
    "T..n....,,,,....n..T",
    "|.......,,,,.......T",
    /* the chase stops here */
    "########....########",
    "#oooooo...8..oooooo#",
    "#oooooo......oooooo#",
    "#oooooo......oooooo#",
    /* the winding road: the Boiler backs up it */
    "#ooooo.....ooooooooo",
    "#oooo.....oooooooooo",
    "#ooo.....ooooooooooo",
    "#ooo.....ooooooooooo",
    "#ooo......oooooooooo",
    "#oooo......ooooooooo",
    "#ooooo......oooooooo",
    "#oooooo......ooooooo",
    "#ooooooo......oooooo",
    "#oooooooo......ooooo",
    "#oooooooo......ooooo",
    "#ooooooo......oooooo",
    "#oooooo......ooooooo",
    "#ooooo......oooooooo",
    "#oooo......ooooooooo",
    "#ooo......oooooooooo",
    "#ooo......oooooooooo",
    "#oooo......ooooooooo",
    "#ooooo......oooooooo",
    "#oooooo......ooooooo",
    "#ooooooo......oooooo",
    "#oooooooo......ooooo",
    "#ooooooooo......oooo",
    "#ooooooooo......oooo",
    "#oooooooo......ooooo",
    "#ooooooo......oooooo",
    "#oooooo......ooooooo",
    "#ooooo......oooooooo",
    "#oooo......ooooooooo",
    "#oooo......ooooooooo",
    "#ooooo......oooooooo",
    "#oooooo......ooooooo",
    "#ooooooo......oooooo",
    "#oooooooo.....oooooo",
    "#ooooooo......oooooo",
    "#oooooo.......oooooo",
    "#oooooo...7...oooooo",
    "#oooooo.......oooooo",
    "#oooooo.......oooooo",
    "######.........#####",
    /* the market */
    "#..................#",
    "#..L....c..b....L..#",
    "#..................#",
    "#..j....TTTT....j..#",
    "#.......TTTT.......#",
    "#..................#",
    "##k##..........##k##",
    "#..................#",
    "#..g..n......n..g..#",
    "#.................$#",
    "TTT....~~~~~~....TTT",
    "T......~~~~~~......T",
    "T..s...~~~~~~...s..T",
    "T......~~~~~~......T",
    "T.....oo....oo.....T",
    "T..r..oo....oo..r..T",
    "T..................T",
    "####G....,,....G####",
    "#........,,........#",
    "#..h.....,,.....h..#",
    "#........,,........#",
    /* the gate street */
    "#WW#.....,,.....#WW#",
    "#vR#.....,,.....#Rv#",
    "#..c.....,,.....b..#",
    "#........,,........#",
    "##k##....,,....##k##",
    "#........,,........#",
    "#..n.....,,.....n..#",
    "#.......o,,o.......#",
    "#.......o,,o.......#",
    "#..g....o,,o....g..#",
    "#........,,........#",
    "#..L.....,,.....L..#",
    "#........,,....$...#",
    "TT..TT...,,...TT..TT",
    "T..................T",
    "T..j...r....r...j..T",
    "T..................T",
    "T..s.....c......s..T",
    "T..................T",
    "#####...,,,,...#####",
    "#WWW#...,,,,...#WWW#",
    "#vRv#...,,,,...#vRv#",
    "#.......,,,,.......#",
    "#..g....,,,,....g..#",
    "#.......,,,,.......#",
    "#.........P........#",
    "#.......,,,,.......#",
    "########,,,,########",
};

/* ---- IV: the Glass Palace ----------------------------------------------------------- */
static const char *const S4[] = {
    /* Morrow's stage */
    "####################",
    "#H..H..........H..H#",
    "#........9.........#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#H................H#",
    "######...,,,...#####",
    /* the gallery of mirrors */
    "#..t.....,,.....t..#",
    "#........,,........#",
    "#..u.....,,.....u..#",
    "#.......o,,o.......#",
    "#..f....o,,o....f..#",
    "#.......o,,o.......#",
    "##z##....,,....##z##",
    "#........,,........#",
    "#..K.....,,.....K..#",
    "#........,,........#",
    "#..q..c..,,..b..q..#",
    "#........,,........#",
    "#...B....,,........#",
    "#........,,........#",
    "##TT.....,,.....TT##",
    "#..................#",
    "#...C..............#",
    "#..................#",
    "#..u....oooo....u..#",
    "#.......oooo....$..#",
    "#..f.............f.#",
    "#..................#",
    "#####....,,....#####",
    /* the Ribbon's ballroom */
    "#..................#",
    "#..................#",
    "#........2.........#",
    "#..................#",
    "#..TT..........TT..#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..TT..........TT..#",
    "#..................#",
    "#..................#",
    "#####....,,....#####",
    /* the library */
    "#..t.....,,.....t..#",
    "#........,,........#",
    "#..q.....,,.....q..#",
    "#...B....,,........#",
    "##TTTT...,,...TTTT##",
    "#........,,........#",
    "#..K.....,,.....K..#",
    "#........,,........#",
    "#oooo....,,....oooo#",
    "#oooo..f.,,.f..oooo#",
    "#oooo....,,....oooo#",
    "#........,,........#",
    "##z##..c.,,.b..##z##",
    "#........,,........#",
    "#..u.....,,.....u..#",
    "#........,,......$.#",
    "#...C..............#",
    "#..................#",
    "#..q....TTTT....q..#",
    "#.......TTTT.......#",
    "#..................#",
    "|..f....,,,,....f..#",
    /* Old Wicks's crypt */
    "########....########",
    "#H................H#",
    "#..................#",
    "#........1.........#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#H................H#",
    "#######.....########",
    /* the long hall */
    "#..H..H..,,..H..H..#",
    "#........,,........#",
    "#..u.....,,.....u..#",
    "#........,,........#",
    "#.......o,,o.......#",
    "#..f....o,,o....f..#",
    "#.......o,,o.......#",
    "##z##....,,....##z##",
    "#........,,........#",
    "#..K..c..,,..b..K..#",
    "#........,,........#",
    "#...B....,,........#",
    "#........,,........#",
    "#.....oooooooo.....#",
    "#..q..oooooooo..q..#",
    "#.....oooooooo.....#",
    "#..................#",
    "#...C..............#",
    "#..................#",
    "##TT.....,,.....TT##",
    "#..t.....,,.....t..#",
    "#........,,........#",
    "#..u.....,,.....u..#",
    "#........,,........#",
    /* the hall of portraits */
    "#####....,,....#####",
    "#..................#",
    "#..f....c..b....f..#",
    "#..................#",
    "#..oo..........oo..#",
    "#..oo....q.....oo..#",
    "#..................#",
    "##z##..........##z##",
    "#..................#",
    "#...B..............#",
    "#..................#",
    "#..u....TTTT....u..#",
    "#.......TTTT.......#",
    "#..K............K..#",
    "#..................#",
    "#........,,........#",
    "#...C....,,........#",
    "#........,,........#",
    "#..t.....,,.....t..#",
    "#........,,........#",
    "#oooo....,,....oooo#",
    "#oooo.q..,,..q.oooo#",
    "#oooo....,,....oooo#",
    "#........,,........#",
    /* the entrance hall */
    "####.....,,.....####",
    "#..t.....,,.....t..#",
    "#..f.....,,.....f..#",
    "#........,,........#",
    "#..u..c..,,..b..u..#",
    "#........,,........#",
    "##TT.....,,.....TT##",
    "#..................#",
    "#..q............q..#",
    "#...B..............#",
    "#..................#",
    "#.......TTTT.......#",
    "#..K....TTTT....K..#",
    "#..................#",
    "#..................#",
    "#........,,......$.#",
    "#..f.....,,.....f..#",
    "#........,,........#",
    "#...C....,,........#",
    "#........,,........#",
    "#..u.....,,.....u..#",
    "#........,,........#",
    "#.........P........#",
    "#........,,........#",
    "#########,,#########",
};

/* ---- V: Morrow's Tower: the lift, then the crown room ---------------------- */
static const char *const S5[] = {
    "oooooooooooooooooooo",
    "oooooooooooooooooooo",
    "ooo..............ooo",
    "ooo..............ooo",
    "ooo..............ooo",
    "ooo.......P......ooo",
    "ooo..............ooo",
    "ooo..............ooo",
    "ooo..............ooo",
    "oooooooooooooooooooo",
    "oooooooooooooooooooo",
};
static const char *const S5_CROWN[] = {
    "####################",
    "#..................#",
    "#........9.........#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..........P.......#",
    "#..................#",
    "####################",
};

const SstStage SST_STAGE[SST_STAGES] = {
    {"FROSTBELL LANE", NULL, SK_SCROLL, TH_SNOW, S1, ARRAY_LEN(S1), BK_ENGINE, BK_NONE, BK_GOGGLE, BK_NONE, 2},
    {"THE NIGHT FREIGHT", NULL, SK_TRAIN, TH_TRAIN, S2, ARRAY_LEN(S2), BK_NONE, BK_NONE, BK_NONE, BK_NONE, 1},
    {"LANTERNBURY", "THE CANAL SIDE", SK_SCROLL, TH_TOWN, S3, ARRAY_LEN(S3), BK_NONE, BK_NONE, BK_DOPPELS, BK_BOILER, 3},
    {"THE GLASS PALACE", "THE UPPER HALLS", SK_SCROLL, TH_PALACE, S4, ARRAY_LEN(S4), BK_WICKS, BK_RIBBON, BK_MORROW, BK_NONE, 3},
    {"MORROW'S TOWER", NULL, SK_LIFT, TH_TOWER, S5, ARRAY_LEN(S5), BK_NONE, BK_NONE, BK_MAW, BK_NONE, 0},
};

/* ---- reading the map ------------------------------------------------------------- */

static const char *const *cur_rows(int *n) {
    int s = iclamp(sst_g.stage, 0, SST_STAGES - 1);
    if (s == 4 && sst_g.half == 1) { *n = ARRAY_LEN(S5_CROWN); return S5_CROWN; }
    *n = SST_STAGE[s].nrows;
    return SST_STAGE[s].rows;
}

int sst_map_rows(void) {
    int n;
    cur_rows(&n);
    return n;
}

static int tile_of(char ch) {
    switch (ch) {
    case ',': return T_PATH;
    case '~': return T_ICE;
    case '#': return T_WALL;
    case 'T': return T_TREE;
    case 'R': return T_ROOF;
    case 'o': return T_PIT;
    case 'w': return T_WATER;
    case '=': return T_BRIDGE;
    case 'W': case 'v': return T_WINDOW;
    case 'H': case 'G': case 'z': case 't': return T_GUNWALL;
    case 'k': return T_ROOFTOP;
    case '|': return T_WALL;
    default: return T_FLOOR;
    }
}

int sst_tile_at(int tx, int ty) {
    int n;
    const char *const *rows = cur_rows(&n);
    if (tx < 0 || tx >= SST_COLS) return T_WALL;
    if (ty < 0) return T_WALL;
    if (ty >= n) return sst_g.stage == 1 || sst_g.stage == 4 ? T_PIT : T_WALL;
    return tile_of(rows[ty][tx]);
}

bool sst_tile_solid(int t) {
    return t == T_WALL || t == T_TREE || t == T_ROOF || t == T_WINDOW || t == T_GUNWALL || t == T_ROOFTOP;
}
bool sst_tile_pit(int t) { return t == T_PIT || t == T_WATER; }

/* ---- what the rows hold -------------------------------------------------------------- */

#define MAX_SPAWNS 220
static SstSpawn spawns[SST_STAGES + 1][MAX_SPAWNS];
static int nspawns[SST_STAGES + 1];
static bool built;

static int foe_of(char ch) {
    switch (ch) {
    case 'c': return FK_CRATE;
    case 'b': return FK_BARREL;
    case '$': return FK_SECRET;
    case 'g': return FK_GUMDROP;
    case 'L': return FK_LANTERN;
    case 'j': return FK_JESTER;
    case 's': return FK_SNOWBALL;
    case 'v': return FK_BAT;
    case 'G': return FK_GUN;
    case 'k': return FK_BOMBER;
    case 'n': return FK_SNOWMAN;
    case 'r': return FK_ROCKETEER;
    case 'h': return FK_COPTER;
    case 'u': return FK_TURTLE;
    case 'K': return FK_HARLEQUIN;
    case 'z': return FK_SKULLJET;
    case 'f': return FK_EMBER;
    case 'B': return FK_BOOK;
    case 'q': return FK_IMP;
    case 'C': return FK_RACECAR;
    case 't': return FK_TORCH;
    default: return FK_NONE;
    }
}

/* boss markers spawn as these "kinds" (negative: not foes) */
enum { MK_BOSS1 = -1, MK_BOSS2 = -2, MK_BOSS9 = -9, MK_CHASE = -7, MK_STOP = -8, MK_HALF = -5, MK_START = -6 };

static void build_lists(void) {
    if (built) return;
    built = true;
    int pass_no = 0;
    for (int s = 0; s <= SST_STAGES; s++) {
        const char *const *rows = s < SST_STAGES ? SST_STAGE[s].rows : S5_CROWN;
        int n = s < SST_STAGES ? SST_STAGE[s].nrows : ARRAY_LEN(S5_CROWN);
        int k = 0;
        /* from the bottom up: the order they come into view */
        for (int ty = n - 1; ty >= 0; ty--)
            for (int tx = 0; tx < SST_COLS && rows[ty][tx]; tx++) {
                char ch = rows[ty][tx];
                int kind = foe_of(ch);
                if (ch == '1') kind = MK_BOSS1;
                else if (ch == '2') kind = MK_BOSS2;
                else if (ch == '9') kind = MK_BOSS9;
                else if (ch == '7') kind = MK_CHASE;
                else if (ch == '8') kind = MK_STOP;
                else if (ch == '|') kind = MK_HALF;
                else if (ch == 'P') kind = MK_START;
                if (kind == FK_NONE || k >= MAX_SPAWNS) continue;
                SstSpawn *sp = &spawns[s][k++];
                sp->x = tx * SST_TILE + SST_TILE / 2;
                sp->y = ty * SST_TILE + SST_TILE / 2;
                sp->kind = kind;
                sp->ord = kind == FK_SECRET ? pass_no++ : 0;
            }
        nspawns[s] = k;
    }
}

void sst_stage_spawns(int stage, const SstSpawn **list, int *n) {
    build_lists();
    int s = stage == 4 && sst_g.half == 1 ? SST_STAGES : iclamp(stage, 0, SST_STAGES - 1);
    *list = spawns[s];
    *n = nspawns[s];
}

int sst_stage_pass_base(int stage) {
    int b = 0;
    for (int s = 0; s < stage && s < SST_STAGES; s++) b += SST_STAGE[s].passes;
    return b;
}

/* ---- the demo player's guide: steps to the top row ---------------------------------------- */

#define GUIDE_MAX 260
static int16_t guide[GUIDE_MAX][SST_COLS];
static int guide_for = -1;

static int relax(int x, int y, int n) {
    int t = sst_tile_at(x, y);
    if (sst_tile_solid(t)) return 0;
    int best = guide[y][x];
    int step = sst_tile_pit(t) ? 5 : 1;
    for (int d = 0; d < 4; d++) {
        int nx = x + (d == 0) - (d == 1), ny = y + (d == 2) - (d == 3);
        if (nx < 0 || nx >= SST_COLS || ny < 0 || ny >= n) continue;
        if (guide[ny][nx] + step < best) best = guide[ny][nx] + step;
    }
    if (best < guide[y][x]) { guide[y][x] = (int16_t)best; return 1; }
    return 0;
}

static void build_guide(void) {
    int key = sst_g.stage * 2 + sst_g.half;
    if (guide_for == key) return;
    guide_for = key;
    int n = imin(sst_map_rows(), GUIDE_MAX);
    for (int y = 0; y < GUIDE_MAX; y++)
        for (int x = 0; x < SST_COLS; x++) guide[y][x] = 30000;
    /* the goal: firm ground on the second row; a floor step costs 1, a pit
     * 5 (it takes a dash). Sweeps down and up until nothing changes. */
    for (int x = 0; x < SST_COLS; x++)
        if (!sst_tile_solid(sst_tile_at(x, 1)) && !sst_tile_pit(sst_tile_at(x, 1))) guide[1][x] = 0;
    for (int it = 0; it < 400; it++) {
        int ch = 0;
        for (int y = 0; y < n; y++)
            for (int x = 0; x < SST_COLS; x++) ch += relax(x, y, n);
        for (int y = n - 1; y >= 0; y--)
            for (int x = SST_COLS - 1; x >= 0; x--) ch += relax(x, y, n);
        if (!ch) break;
    }
}

int sst_guide(int tx, int ty) {
    build_guide();
    if (tx < 0 || tx >= SST_COLS || ty < 0 || ty >= imin(sst_map_rows(), GUIDE_MAX)) return 30000;
    return guide[ty][tx];
}

/* ---- the train's waves --------------------------------------------------------------- */
/* Each wave: kinds that come together. The cloaks come in from either side
 * and fire wavy pairs (some circle the car leaving bomb shots); the puffers
 * dive over the roof; the toy trains run along it. */
static const int8_t TRAIN[][6] = {
    {FK_GOURD, FK_GOURD, 0},
    {FK_PUFFER, FK_PUFFER, FK_PUFFER, 0},
    {FK_TOYTRAIN, FK_GOURD, FK_GOURD, 0},
    {FK_GOURD, FK_GOURD, FK_GOURD, 0},
    {FK_PUFFER, FK_PUFFER, FK_TOYTRAIN, FK_TOYTRAIN, 0},
    {FK_GOURD, FK_GOURD, FK_PUFFER, FK_PUFFER, 0},
    {FK_TOYTRAIN, FK_TOYTRAIN, FK_TOYTRAIN, 0},
    {FK_GOURD, FK_GOURD, FK_GOURD, FK_GOURD, 0},
    {FK_PUFFER, FK_PUFFER, FK_PUFFER, FK_PUFFER, FK_TOYTRAIN, 0},
    {FK_GOURD, FK_GOURD, FK_GOURD, FK_TOYTRAIN, FK_PUFFER, 0},
};

int sst_train_wave_count(void) { return ARRAY_LEN(TRAIN); }

void sst_train_waves(int wave, int *kinds, int *n, int max) {
    *n = 0;
    if (wave < 0 || wave >= ARRAY_LEN(TRAIN)) return;
    for (int i = 0; i < 6 && TRAIN[wave][i] && *n < max; i++) kinds[(*n)++] = TRAIN[wave][i];
}

/* ---- the lift's waves (our reading: the show's foes from every stage) ------------------ */
static const int8_t LIFT[][6] = {
    {FK_BAT, FK_BAT, FK_BAT, FK_BAT, 0},
    {FK_GOURD, FK_GOURD, FK_PUFFER, 0},
    {FK_EMBER, FK_EMBER, 0},
    {FK_IMP, FK_IMP, FK_BAT, FK_BAT, 0},
    {FK_COPTER, FK_GOURD, FK_GOURD, 0},
    {FK_PUFFER, FK_PUFFER, FK_PUFFER, FK_EMBER, 0},
    {FK_BOOK, FK_IMP, FK_IMP, 0},
    {FK_COPTER, FK_COPTER, FK_BAT, FK_BAT, 0},
};

int sst_lift_wave_count(void) { return ARRAY_LEN(LIFT); }

void sst_lift_waves(int wave, int *kinds, int *n, int max) {
    *n = 0;
    if (wave < 0 || wave >= ARRAY_LEN(LIFT)) return;
    for (int i = 0; i < 6 && LIFT[wave][i] && *n < max; i++) kinds[(*n)++] = LIFT[wave][i];
}
