/* TURNIP TRUCK - Blipton, the city. One hand-made map of 32 x 32 cells of
 * 48 px that wraps in every direction (drive off one side and you are back
 * on the other). All our own layout: Old Blipton's alleys, the depot with
 * the park below it, the cul-de-sacs of Saucer Heights, the brine canal and
 * its three bridges, the Pickle Works' pools, ramps, canisters and rocky
 * yard, the market and the docks.
 *
 *   .  road            #  building        H  the depot         g  grass (park)
 *   T  tree            ~  brine (fall in)  r  rocky ground     F  fence
 *   l  the fenced lot  =  bridge           +  drop zone         x  pipe at a crossing
 *   c  a floating crate                    k  gas canisters     > < ^ v  ramp
 */
#include "tnp.h"

const char *const TNP_MAP_ROWS[TNP_MAP] = {
    "...........x.........x.......+..", /*  0 */
    ".####.#####.####.####.#####.FFFF", /*  1 */
    "...c..#####.####.........##.llkF", /*  2 */
    ".####.#####.####.####.#####.F~lF", /*  3 */
    ".####.#####.####.####.#####.FFFF", /*  4 */
    ".....x..........................", /*  5 */
    ".##.#.##.##.####.####.##.##.####", /*  6 */
    ".##.#.##c##.........#.##.##.####", /*  7 */
    ".##.#.......#HH#.##.#.##c##.####", /*  8 */
    ".##.#.#####.#HH#.####.#####.####", /*  9 */
    ".....x.....................x....", /* 10 */
    ".####.gggT#.Tggg.####.#####.##.#", /* 11 */
    ".#.##.gTgg#.gggT.#c##.....#.##.#", /* 12 */
    ".#c...gggg#.gggg.#.##.###.#.##c#", /* 13 */
    ".####.#####.gTgg.#.##.#####.####", /* 14 */
    "x...............x...............", /* 15 */
    ".####.##.##.####.#######.##.####", /* 16 */
    ".####.##.##.####.#######c##.####", /* 17 */
    "......+....+.....#######.##.####", /* 18 */
    "=~~~~~~~~~~=~~~~~~~~~~~~~~~=~~~~", /* 19 */
    "=~~~~~~~~~~=~~~~~~~~~~~~~~~=~~~~", /* 20 */
    ".##########.....+.........+.....", /* 21 */
    ".....######.##c#.####.#####.###.", /* 22 */
    ".#.########.##.#.####.#####.###c", /* 23 */
    "..+....>~..x+..............x....", /* 24 */
    ".rrrrv####c.####.####.rrrrr.#~~~", /* 25 */
    ".rrrr~...kr..........>rrrrr.#~~~", /* 26 */
    ".rrrrc####..####.####.rrrrr.#~~~", /* 27 */
    "...k.+..>k...........x+.........", /* 28 */
    ".####.>~~...####.#.##.#####.##.#", /* 29 */
    ".####.#~~~#.gTgg.#c##...........", /* 30 */
    ".####.#####.####.####.#####.####", /* 31 */
};

/* Thirty-three named places, each with its red circle on the road beside it. */
const TnpDest TNP_DEST[TNP_DESTS] = {
    {"GLOOP DINER", 3, 0},        {"12 COMET COURT", 19, 2},   {"CHANNEL 9 STUDIO", 13, 5},
    {"THE OLD MILL", 1, 5},       {"STARGAZER SCHOOL", 23, 5}, {"BEEP BAKERY", 8, 10},
    {"CITY HALL", 19, 10},        {"7 CRATER CLOSE", 24, 7},   {"THE TOWN CLOCK", 3, 7},
    {"GREEN TEA ROOMS", 5, 12},   {"BLIPTON LIBRARY", 11, 13}, {"PARK KIOSK", 16, 12},
    {"NEBULA NOODLES", 23, 12},   {"MOONLITE MOTEL", 27, 13},  {"BLIP TOWER", 29, 15},
    {"THE WOBBLY BANK", 19, 15},  {"SPROCKET GARAGE", 8, 16},  {"THE CANAL LOCKS", 2, 18},
    {"TENTACLE SPA", 13, 18},     {"THE GOO MUSEUM", 24, 16},  {"FERRY OFFICE", 29, 21},
    {"RIVERSIDE FLATS", 18, 21},  {"MARKET HALL", 14, 23},     {"SPIN CYCLE LAUNDRY", 19, 24},
    {"FIZZ FACTORY", 3, 22},      {"BRINE PUMP 3", 8, 26},     {"PICKLE WORKS GATE", 1, 28},
    {"STRIKE ZONE LANES", 13, 28}, {"THE POST OFFICE", 18, 29}, {"THE CRANE YARD", 24, 30},
    {"ICEBOX WAREHOUSE", 25, 28}, {"QUAY CAFE", 30, 29},       {"SPACE CLINIC", 20, 26},
};

const char *const TNP_DISTRICT_NAME[TNP_DISTRICTS] = {
    "OLD BLIPTON", "DEPOT ROW", "SAUCER HEIGHTS", "WESTGREEN", "DOWNTOWN",
    "THE CANAL", "PICKLE WORKS", "THE MARKET", "THE DOCKS",
};

TnpCell tnp_map[TNP_MAP][TNP_MAP];
int tnp_n_crates, tnp_n_cans, tnp_n_pipes, tnp_n_drops;
int8_t tnp_crate_c[TNP_MAX_CRATES][2], tnp_cans_c[TNP_MAX_CANS][2], tnp_pipe_c[TNP_MAX_PIPES][2],
    tnp_drop_c[TNP_MAX_DROPS][2];

static int district_of(int x, int y) {
    if (y == 19 || y == 20) return 5;
    if (y <= 18) {
        if (x <= 10) return y <= 9 ? 0 : 3;
        if (x <= 16) return 1;
        return y <= 9 ? 2 : 4;
    }
    if (x <= 10) return 6;
    if (x <= 21) return 7;
    return 8;
}

void tnp_city_load(void) {
    static bool done;
    if (done) return;
    done = true;
    tnp_n_crates = tnp_n_cans = tnp_n_pipes = tnp_n_drops = 0;
    for (int y = 0; y < TNP_MAP; y++)
        for (int x = 0; x < TNP_MAP; x++) {
            TnpCell *c = &tnp_map[y][x];
            char ch = TNP_MAP_ROWS[y][x];
            memset(c, 0, sizeof *c);
            c->district = (uint8_t)district_of(x, y);
            switch (ch) {
            case '#': c->type = CT_BUILD; break;
            case 'H': c->type = CT_HQ; break;
            case 'g': c->type = CT_GRASS; break;
            case 'T': c->type = CT_TREE; break;
            case '~': c->type = CT_BRINE; break;
            case 'r': c->type = CT_ROCK; break;
            case 'F': c->type = CT_FENCE; break;
            case 'l': c->type = CT_LOT; break;
            case '=': c->type = CT_BRIDGE; break;
            case '+': c->type = CT_ROAD; c->feat = FT_DROP; break;
            case 'x': c->type = CT_ROAD; c->feat = FT_PIPE; break;
            case 'c': c->type = CT_ROAD; c->feat = FT_CRATE; break;
            case 'k': c->type = CT_ROAD; c->feat = FT_CANS; break;
            case '>': c->type = CT_ROAD; c->feat = FT_RAMP; c->ramp_dir = DIR_E; break;
            case 'v': c->type = CT_ROAD; c->feat = FT_RAMP; c->ramp_dir = DIR_S; break;
            case '<': c->type = CT_ROAD; c->feat = FT_RAMP; c->ramp_dir = DIR_W; break;
            case '^': c->type = CT_ROAD; c->feat = FT_RAMP; c->ramp_dir = DIR_N; break;
            default: c->type = CT_ROAD; break;
            }
            if (c->feat == FT_CRATE && tnp_n_crates < TNP_MAX_CRATES) {
                tnp_crate_c[tnp_n_crates][0] = (int8_t)x;
                tnp_crate_c[tnp_n_crates++][1] = (int8_t)y;
            }
            if (c->feat == FT_CANS && tnp_n_cans < TNP_MAX_CANS) {
                tnp_cans_c[tnp_n_cans][0] = (int8_t)x;
                tnp_cans_c[tnp_n_cans++][1] = (int8_t)y;
            }
            if (c->feat == FT_PIPE && tnp_n_pipes < TNP_MAX_PIPES) {
                tnp_pipe_c[tnp_n_pipes][0] = (int8_t)x;
                tnp_pipe_c[tnp_n_pipes++][1] = (int8_t)y;
            }
            if (c->feat == FT_DROP && tnp_n_drops < TNP_MAX_DROPS) {
                tnp_drop_c[tnp_n_drops][0] = (int8_t)x;
                tnp_drop_c[tnp_n_drops++][1] = (int8_t)y;
            }
        }
}

int tnp_wrapc(int c) { return ((c % TNP_MAP) + TNP_MAP) % TNP_MAP; }

const TnpCell *tnp_cell(int cx, int cy) { return &tnp_map[tnp_wrapc(cy)][tnp_wrapc(cx)]; }

float tnp_wrap(float v) {
    v = fmodf(v, (float)TNP_WORLD);
    if (v < 0) v += (float)TNP_WORLD;
    if (v >= (float)TNP_WORLD) v -= (float)TNP_WORLD;
    return v;
}

float tnp_wrapd(float d) {
    d = fmodf(d, (float)TNP_WORLD);
    if (d < -(float)TNP_WORLD / 2) d += (float)TNP_WORLD;
    if (d >= (float)TNP_WORLD / 2) d -= (float)TNP_WORLD;
    return d;
}

const TnpCell *tnp_cell_at(float x, float y) {
    return tnp_cell((int)floorf(tnp_wrap(x) / TNP_CELL), (int)floorf(tnp_wrap(y) / TNP_CELL));
}

float tnp_dist(float ax, float ay, float bx, float by) {
    float dx = tnp_wrapd(bx - ax), dy = tnp_wrapd(by - ay);
    return sqrtf(dx * dx + dy * dy);
}

float tnp_cx(int c) { return (float)(c * TNP_CELL + TNP_CELL / 2); }

bool tnp_solid_type(int type) { return type == CT_BUILD || type == CT_HQ || type == CT_TREE || type == CT_FENCE; }

bool tnp_drivable(int cx, int cy) {
    int t = tnp_cell(cx, cy)->type;
    return !tnp_solid_type(t) && t != CT_BRINE;
}

bool tnp_roadlike(int cx, int cy) {
    const TnpCell *c = tnp_cell(cx, cy);
    return (c->type == CT_ROAD || c->type == CT_BRIDGE) && c->feat != FT_CANS && c->feat != FT_RAMP;
}

int tnp_district_at(float x, float y) { return tnp_cell_at(x, y)->district; }
