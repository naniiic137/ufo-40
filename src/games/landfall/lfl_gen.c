/* LANDFALL - the planet generator. Every run builds a new planet by the
 * community's generation rules for the original (docs/games/08-landfall.md):
 * a 7 x 4 or 6 x 5 grid with about five cells of solid stone, exactly four
 * real gates on the outer edge (to a chart piece, the dungeon, a tool and
 * two of the rare resource), gated side rooms never next to each other, six
 * or seven villages with six trades priced by rarity, eight tools (five in
 * the open, one gated, two traded), four energy cells (now and then five),
 * one hidden wall, and a dungeon of six rooms in a line with the third
 * chart piece at its end. The progression check below makes sure every
 * planet can be won. */
#include "lfl.h"

LflPlanet lp;
static Rng gr;

static const char *const TOOL_NAMES[TL_COUNT] = {"DISC", "RIVET GUN", "HATCHER", "BLAST PODS", "THUMPER", "BOG BOOTS",
                                                 "CHOP PRESS", "TALK BOX"};
static const char *const RES_NAMES[RS_COUNT] = {"BEADS", "BURRS", "FERNS", "SPORES"};

const char *const *lfl_tool_names(void) { return TOOL_NAMES; }
const char *lfl_res_name(int r) { return r >= 0 && r < RS_COUNT ? RES_NAMES[r] : "?"; }

/* fixed by rarity: a tool costs 32 / 16 / 8 / 4, a chart piece 32 / 16 / 8
 * of the three uncommon ones (never the common one) */
int lfl_trade_price(int kind, int tier) {
    static const int ITEM[4] = {32, 16, 8, 4};
    static const int MAP[4] = {0, 32, 16, 8};
    if (kind == TR_MAP) return MAP[iclamp(tier, 0, 3)];
    return ITEM[iclamp(tier, 0, 3)];
}

/* the key that opens each kind of gate */
static const int GATE_KEY[GK_COUNT] = {TL_BOOTS, TL_DISC, TL_HATCHER, TL_RIVET, TL_PODS, TL_THUMPER, TL_CHOPS};

int lfl_room_index(int area, int gx, int gy) {
    if (area == AREA_OVER) {
        if (gx < 0 || gy < 0 || gx >= LFL_GMAX_W || gy >= LFL_GMAX_H) return -1;
        return gy * LFL_GMAX_W + gx;
    }
    if (gx < 0 || gy < 0 || gx >= LFL_DUN_W || gy >= LFL_DUN_H) return -1;
    return LFL_OVER_ROOMS + gy * LFL_DUN_W + gx;
}

static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};

int lfl_neighbor(int room, int dir) {
    if (room < 0 || room >= LFL_MAX_ROOMS) return -1;
    const LflRoom *r = &lp.room[room];
    if (!r->exists || !(r->open & (1 << dir))) return -1;
    int n = lfl_room_index(r->area, r->gx + DX[dir], r->gy + DY[dir]);
    return n >= 0 && lp.room[n].exists ? n : -1;
}

int lfl_tile(int room, int tx, int ty) {
    if (room < 0 || room >= LFL_MAX_ROOMS || !lp.room[room].exists) return T_STONE;
    if (tx < 0 || ty < 0 || tx >= LFL_RW || ty >= LFL_RH) return T_STONE;
    return lp.room[room].t[ty][tx];
}

static void shuffle(int *a, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rng_range(&gr, 0, i);
        int t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

static bool in_grid(int gx, int gy) { return gx >= 1 && gy >= 1 && gx <= lp.gw && gy <= lp.gh; }
static LflRoom *R(int gx, int gy) {
    int i = lfl_room_index(AREA_OVER, gx, gy);
    return &lp.room[i < 0 ? 0 : i];
}

/* ------------------------------------------------------------------------ */
/* interior templates: 16 x 7, inside the room's one-tile border            */
/* . floor  w wood  # rock  T plant  f foliage  c crystal  s swamp  p pit
 * b blue block  L hollow log  o spot  e creature  r nook spot  n big shell
 * H hut  G giver  v villager  S hidden wall  a behind it                   */

static const char *const FIELD[] = {
    "................"
    "..T..........T.."
    "......o.....e..."
    "...e............"
    "..........o....."
    "..T.....e....T.."
    "................",

    "................"
    "...##......##..."
    "...#o..e...#...."
    "................"
    "....#..o...e#..."
    "...##......##..."
    "................",

    "................"
    "....e.....o....."
    "......ssss......"
    "..o..ssrrss..e.."
    "......ssss......"
    "....e..........."
    "................",

    "................"
    ".ccc......T....."
    ".crc...o........"
    ".ccc......e....."
    "..........e....."
    "....T.......o..."
    "................",

    "................"
    "..........fff..."
    "..e.......frf..."
    "......o...fff..."
    "................"
    "...T...e....o..."
    "................",

    "................"
    "..pp......e....."
    "..pp............"
    "......o....pp..."
    "...........pr..."
    "...e.......pp..."
    "................",

    "................"
    "....TTTTT......."
    "....LrrrT...e..."
    "....TTTTT......."
    "..o............."
    "........e....o.."
    "................",

    "................"
    "..#bb#.....e...."
    "..#rr#.........."
    "..####...o......"
    "................"
    ".....e......T..."
    "................",

    "................"
    "................"
    "....n.......o..."
    "................"
    "..T......e...T.."
    "......o........."
    "................",

    "................"
    ".TT...TT...TT..."
    "...e.......o...."
    ".TT...TT...TT..."
    "......o........."
    ".TT...TT...TT.e."
    "................",

    "................"
    "..e.......o....."
    "sssssswwwwssssss"
    "......o........."
    "................"
    "...T.......e...."
    "................",

    "................"
    ".#..#..#..#..#.."
    "......e........."
    "..o.........o..."
    "........e......."
    ".#..#..#..#..#.."
    "................",

    "................"
    "...c......f....."
    "...c..e...f..o.."
    "................"
    "..o...f....c...."
    "......f....c.e.."
    "................",

    "sss............."
    "srs.....e......."
    "sss............."
    "..........o....."
    "....e..........."
    ".........T......"
    "............o...",
};
#define N_FIELD ARRAY_LEN(FIELD)

/* rooms that can hold the hidden wall; the hint names their look */
static const char *const SECRET_T[] = {
    "................"
    "........###....."
    "..wwwwwwSa#....."
    "........###....."
    "..o............."
    "............e..."
    "................",

    "................"
    "...ss.....###..."
    "...ss.....#aS..."
    "..........###..."
    "....e..........."
    "..........o....."
    "................",

    "................"
    "................"
    ".....L.....#S#.."
    "...........#a#.."
    "..e........###.."
    "......o........."
    "................",

    "................"
    "..p.........p..."
    "................"
    ".....###........"
    ".....#aS...e...."
    ".....###...o...."
    "................",
};
#define N_SECRET ARRAY_LEN(SECRET_T)
static const char *const SECRET_WHERE[N_SECRET] = {
    "WHERE THE WOODEN WALK MEETS THE ROCK",
    "BY THE LITTLE SQUARE OF BOG",
    "NEAR THE LONELY LOG",
    "BETWEEN THE TWO HOLES IN THE GROUND",
};

/* the villages: the giver always stands on the same tile of a layout (a
 * layout may be mirrored), with nothing in the 2 x 2 below */
static const char *const VILLAGE_T[] = {
    "................"
    "..HH........HH.."
    "..HH...G....HH.."
    "................"
    "....v......v...."
    "........v......."
    "................",

    "................"
    ".HH..........HH."
    ".HH...v..G...HH."
    "................"
    "...v........v..."
    "................"
    "................",

    "................"
    "...HH..v..HH...."
    "...HH.....HH...."
    "..v....G......v."
    "................"
    "................"
    "................",
};
#define N_VILLAGE ARRAY_LEN(VILLAGE_T)

static const char SHIP_T[] =
    "................"
    "..T..........T.."
    "................"
    "................"
    "................"
    "..T..........T.."
    "................";

static const char *const DUN_T[] = {
    "................"
    ".##..##..##..##."
    ".#e..........e#."
    "......#..#......"
    ".#e..........e#."
    ".##..##..##..##."
    "................",

    "................"
    "..###....###...."
    "..#e......e#...."
    "......##........"
    "..#e......e#...."
    "..###....###...."
    "................",

    "................"
    ".e....####....e."
    "......#..#......"
    "..##..#..#..##.."
    "......#..#......"
    ".e....#..#....e."
    "................",

    "................"
    "..e...ssss...e.."
    "......ssss......"
    ".##..........##."
    "......ssss......"
    "..e...ssss...e.."
    "................",

    "................"
    "...b...e....b..."
    "...b........b..."
    ".e.b...##...b.e."
    "...b........b..."
    "...b...e....b..."
    "................",

    "................"
    ".T.T.T.e.T.T.T.."
    "................"
    ".e....crrc....e."
    "................"
    ".T.T.T.e.T.T.T.."
    "................",
};
#define N_DUN ARRAY_LEN(DUN_T)

/* ------------------------------------------------------------------------ */

typedef struct Spots {
    int n[8];
    uint8_t x[8][24], y[8][24];
} Spots;
enum { SPOT_O, SPOT_E, SPOT_R, SPOT_N, SPOT_V, SPOT_G, SPOT_S, SPOT_A };

static void spot_add(Spots *s, int k, int x, int y) {
    if (s->n[k] < 24) {
        s->x[k][s->n[k]] = (uint8_t)x;
        s->y[k][s->n[k]] = (uint8_t)y;
        s->n[k]++;
    }
}

static bool walkable_tile(int t) {
    return t == T_FLOOR || t == T_WOOD || t == T_TECHDOWN || t == T_STAIRS_DN || t == T_STAIRS_UP;
}

/* every opening and every needed spot joined by plain ground */
static bool room_connected(int room, uint8_t need[LFL_RH][LFL_RW], uint8_t extra_pass[LFL_RH][LFL_RW]) {
    LflRoom *r = &lp.room[room];
    uint8_t seen[LFL_RH][LFL_RW];
    memset(seen, 0, sizeof seen);
    int qx[LFL_RW * LFL_RH], qy[LFL_RW * LFL_RH], qn = 0, qi = 0;
    int sx = -1, sy = -1;
    for (int y = 0; y < LFL_RH && sx < 0; y++)
        for (int x = 0; x < LFL_RW; x++)
            if (need[y][x]) { sx = x; sy = y; break; }
    if (sx < 0) return true;
    seen[sy][sx] = 1;
    qx[qn] = sx; qy[qn++] = sy;
    while (qi < qn) {
        int x = qx[qi], y = qy[qi++];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= LFL_RW || ny >= LFL_RH || seen[ny][nx]) continue;
            if (!walkable_tile(r->t[ny][nx]) && !(extra_pass && extra_pass[ny][nx]) && !need[ny][nx]) continue;
            seen[ny][nx] = 1;
            qx[qn] = nx; qy[qn++] = ny;
        }
    }
    for (int y = 0; y < LFL_RH; y++)
        for (int x = 0; x < LFL_RW; x++)
            if (need[y][x] && !seen[y][x]) return false;
    return true;
}

/* border: closed sides solid stone, open sides a hedge with the opening */
static void lay_border(int room) {
    LflRoom *r = &lp.room[room];
    bool dun = r->area == AREA_DUN;
    for (int y = 0; y < LFL_RH; y++)
        for (int x = 0; x < LFL_RW; x++) r->t[y][x] = T_FLOOR;
    for (int d = 0; d < 4; d++) {
        bool open = (r->open >> d) & 1;
        /* the planet's outer edge is a stone wall, even where a side room
         * opens off it */
        int hedge = dun ? T_ROCK : in_grid(r->gx + DX[d], r->gy + DY[d]) ? T_PLANT : T_STONE;
        int len = (d == DIR_UP || d == DIR_DOWN) ? LFL_RW : LFL_RH;
        for (int i = 0; i < len; i++) {
            int x = d == DIR_LEFT ? 0 : d == DIR_RIGHT ? LFL_RW - 1 : i;
            int y = d == DIR_UP ? 0 : d == DIR_DOWN ? LFL_RH - 1 : i;
            int t = open ? hedge : T_STONE;
            if (open && i >= r->op_lo[d] && i <= r->op_hi[d]) t = T_FLOOR;
            /* a corner is stone if either side is */
            if (r->t[y][x] == T_STONE) continue;
            r->t[y][x] = (uint8_t)t;
        }
    }
}

static void lay_template(int room, const char *tm, bool fx, bool fy, Spots *sp) {
    LflRoom *r = &lp.room[room];
    memset(sp, 0, sizeof *sp);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 16; x++) {
            int sx = fx ? 15 - x : x, sy = fy ? 6 - y : y;
            char c = tm[sy * 16 + sx];
            int tx = x + 1, ty = y + 1, t = T_FLOOR;
            switch (c) {
            case 'w': t = T_WOOD; break;
            case '#': t = T_ROCK; break;
            case 'T': t = T_PLANT; break;
            case 'f': t = T_FOLIAGE; break;
            case 'c': t = T_CRYSTAL; break;
            case 's': t = T_SWAMP; break;
            case 'p': t = T_PIT; break;
            case 'b': t = T_TECH; break;
            case 'L': t = T_LOG; break;
            case 'H': t = T_HUT; break;
            case 'S': t = T_ROCK; spot_add(sp, SPOT_S, tx, ty); break;
            case 'a': spot_add(sp, SPOT_A, tx, ty); break;
            case 'o': spot_add(sp, SPOT_O, tx, ty); break;
            case 'e': spot_add(sp, SPOT_E, tx, ty); break;
            case 'r': spot_add(sp, SPOT_R, tx, ty); break;
            case 'n': spot_add(sp, SPOT_N, tx, ty); break;
            case 'v': spot_add(sp, SPOT_V, tx, ty); break;
            case 'G': spot_add(sp, SPOT_G, tx, ty); break;
            default: break;
            }
            r->t[ty][tx] = (uint8_t)t;
        }
}

static void need_openings(int room, uint8_t need[LFL_RH][LFL_RW]) {
    LflRoom *r = &lp.room[room];
    memset(need, 0, LFL_RH * LFL_RW);
    for (int d = 0; d < 4; d++) {
        if (!((r->open >> d) & 1)) continue;
        for (int i = r->op_lo[d]; i <= r->op_hi[d]; i++) {
            int x = d == DIR_LEFT ? 0 : d == DIR_RIGHT ? LFL_RW - 1 : i;
            int y = d == DIR_UP ? 0 : d == DIR_DOWN ? LFL_RH - 1 : i;
            need[y][x] = 1;
        }
    }
}

static void add_spawn(int room, int type, int sub, int tx, int ty) {
    LflRoom *r = &lp.room[room];
    if (r->nspawn >= LFL_MAX_SPAWNS) return;
    LflSpawn *s = &r->spawn[r->nspawn++];
    s->type = (uint8_t)type;
    s->sub = (uint8_t)sub;
    s->tx = (uint8_t)tx;
    s->ty = (uint8_t)ty;
    s->taken = 0;
}

static int add_thing(int kind, int sub, int n, int room, int tx, int ty) {
    if (lp.nthing >= LFL_MAX_THINGS) return -1;
    LflThing *t = &lp.thing[lp.nthing];
    memset(t, 0, sizeof *t);
    t->alive = 1;
    t->kind = (uint8_t)kind;
    t->sub = (uint8_t)sub;
    t->n = (uint8_t)n;
    t->room = (uint8_t)room;
    t->x = (int16_t)(tx * LFL_TILE + 2);
    t->y = (int16_t)(ty * LFL_TILE + 2);
    return lp.nthing++;
}

/* side-relative tile coordinates: depth from the side, lateral along it */
static void side_xy(int side, int d, int l, int *x, int *y) {
    switch (side) {
    case DIR_LEFT: *x = d; *y = l; break;
    case DIR_RIGHT: *x = LFL_RW - 1 - d; *y = l; break;
    case DIR_UP: *x = l; *y = d; break;
    default: *x = l; *y = LFL_RH - 1 - d; break;
    }
}
static void side_set(int room, int side, int d, int l, int t) {
    int x, y;
    side_xy(side, d, l, &x, &y);
    if (x >= 0 && y >= 0 && x < LFL_RW && y < LFL_RH) lp.room[room].t[y][x] = (uint8_t)t;
}
static int lane0(int side) { return side == DIR_LEFT || side == DIR_RIGHT ? 4 : 8; }

/* a gate in the passage leaving `room` by `side`: a corridor two tiles wide
 * and three deep, walled at the flanks, with the obstacle in it */
static int carve_gate(int room, int side, int kind, int role) {
    int l0 = lane0(side);
    for (int d = 1; d <= 3; d++) {
        side_set(room, side, d, l0 - 1, lp.room[room].area == AREA_DUN ? T_ROCK : T_STONE);
        side_set(room, side, d, l0 + 2, lp.room[room].area == AREA_DUN ? T_ROCK : T_STONE);
        side_set(room, side, d, l0, T_FLOOR);
        side_set(room, side, d, l0 + 1, T_FLOOR);
    }
    if (lp.ngate >= LFL_MAX_GATES) return -1;
    LflGate *g = &lp.gate[lp.ngate];
    memset(g, 0, sizeof *g);
    g->kind = (uint8_t)kind;
    g->role = (uint8_t)role;
    g->room = (uint8_t)room;
    g->side = (int8_t)side;
    g->prize_thing = 255;
    int x0, y0, x1, y1;
    switch (kind) {
    case GK_CRYSTAL:
    case GK_FOLIAGE:
    case GK_TECH: {
        int t = kind == GK_CRYSTAL ? T_CRYSTAL : kind == GK_FOLIAGE ? T_FOLIAGE : T_TECH;
        side_set(room, side, 2, l0, t);
        side_set(room, side, 2, l0 + 1, t);
        side_xy(side, 2, l0, &x0, &y0);
        side_xy(side, 2, l0 + 1, &x1, &y1);
        break;
    }
    case GK_SWAMP:
        for (int d = 1; d <= 3; d++) {
            side_set(room, side, d, l0, T_SWAMP);
            side_set(room, side, d, l0 + 1, T_SWAMP);
        }
        side_xy(side, 1, l0, &x0, &y0);
        side_xy(side, 3, l0 + 1, &x1, &y1);
        break;
    case GK_HOLE:
        for (int d = 1; d <= 3; d++) {
            side_set(room, side, d, l0, T_HOLE);
            side_set(room, side, d, l0 + 1, lp.room[room].area == AREA_DUN ? T_ROCK : T_STONE);
        }
        side_xy(side, 1, l0, &x0, &y0);
        side_xy(side, 3, l0, &x1, &y1);
        break;
    default: /* the snail, two tiles square, in the passage */
        side_xy(side, 1, l0, &x0, &y0);
        side_xy(side, 2, l0 + 1, &x1, &y1);
        break;
    }
    g->tx = (int8_t)imin(x0, x1);
    g->ty = (int8_t)imin(y0, y1);
    g->w = (int8_t)(iabs(x1 - x0) + 1);
    g->h = (int8_t)(iabs(y1 - y0) + 1);
    if (kind == GK_SNAIL) {
        g->sx = (g->tx * LFL_TILE + 2) * LFL_FX;
        g->sy = (g->ty * LFL_TILE + 2) * LFL_FX;
        g->snail = 0;
        g->snail_face = (uint8_t)side;
    }
    return lp.ngate++;
}

/* ------------------------------------------------------------------------ */
/* the logic: can every planet be won?                                      */

static int item_trade(int tool) {
    for (int i = 0; i < 6; i++)
        if (lp.trade[i].kind == TR_ITEM && lp.trade[i].item == tool) return i;
    return -1;
}

static int find_gate(int role) {
    for (int i = 0; i < lp.ngate; i++)
        if (lp.gate[i].role == role) return i;
    return -1;
}

/* the tools a player can lay hands on, counting juggling (any tool can be
 * set down and fetched later, so two hands never limit what is reachable;
 * no single obstacle needs two tools at once) */
static unsigned reachable_tools(void) {
    unsigned have = 0;
    for (int t = 0; t < TL_COUNT; t++)
        if (lp.item_where[t] == 0) have |= 1u << t;
    bool changed = true;
    while (changed) {
        changed = false;
        for (int t = 0; t < TL_COUNT; t++) {
            if (have & (1u << t)) continue;
            bool ok = false;
            if (lp.item_where[t] == 2) {
                int tr = item_trade(t);
                ok = tr >= 0 && (!lp.trade[tr].glyph || (have & (1u << TL_TALKBOX)));
            } else if (lp.item_where[t] == 1) {
                int g = find_gate(GR_ITEM);
                ok = g >= 0 && (have & (1u << GATE_KEY[lp.gate[g].kind]));
            }
            if (ok) { have |= 1u << t; changed = true; }
        }
    }
    return have;
}

bool lfl_winnable(void) {
    unsigned have = reachable_tools();
    int gm = find_gate(GR_MAP), gd = find_gate(GR_DUNGEON);
    if (gm < 0 || gd < 0) return false;
    if (!(have & (1u << GATE_KEY[lp.gate[gm].kind]))) return false;
    if (!(have & (1u << GATE_KEY[lp.gate[gd].kind]))) return false;
    for (int i = 0; i < lp.ngate; i++)
        if (lp.gate[i].role == GR_DUN && !(have & (1u << GATE_KEY[lp.gate[i].kind]))) return false;
    for (int i = 0; i < 6; i++)
        if (lp.trade[i].kind == TR_MAP && lp.trade[i].glyph && !(have & (1u << TL_TALKBOX))) return false;
    return true;
}

/* ------------------------------------------------------------------------ */

static bool grid_connected(void) {
    uint8_t seen[LFL_GMAX_H][LFL_GMAX_W];
    memset(seen, 0, sizeof seen);
    int qx[64], qy[64], qn = 0, qi = 0, total = 0;
    for (int y = 1; y <= lp.gh; y++)
        for (int x = 1; x <= lp.gw; x++)
            if (R(x, y)->kind != RK_STONE) {
                total++;
                if (!qn) { qx[0] = x; qy[0] = y; qn = 1; seen[y][x] = 1; }
            }
    while (qi < qn) {
        int x = qx[qi], y = qy[qi++];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!in_grid(nx, ny) || seen[ny][nx] || R(nx, ny)->kind == RK_STONE) continue;
            seen[ny][nx] = 1;
            qx[qn] = nx; qy[qn++] = ny;
        }
    }
    return qn == total;
}

static void roll_species(void) {
    int looks[LFL_LOOKS];
    for (int i = 0; i < LFL_LOOKS; i++) looks[i] = i;
    shuffle(looks, LFL_LOOKS);
    int drops[4] = {lp.res_of_tier[TIER_COMMON], lp.res_of_tier[TIER_LESS], lp.res_of_tier[TIER_MORE], -1};
    shuffle(drops, 4);
    for (int i = 0; i < 6; i++) {
        LflSpecies *s = &lp.sp[i];
        memset(s, 0, sizeof *s);
        s->look = (uint8_t)looks[i];
        if (i < 2) {
            s->friendly = 1;
            s->glyph = (uint8_t)(i == SP_FRIEND_B);
            s->move = MV_STILL;
            s->hp = 3;
            s->dmg = 1;
            s->drop = (int8_t)lp.res_of_tier[TIER_RARE];
            s->speed = 96;
            continue;
        }
        /* every hostile species rolls all of it afresh */
        s->move = (uint8_t)rng_range(&gr, 0, 2);
        s->attack = (uint8_t)rng_range(&gr, 0, 2);
        s->hp = (uint8_t)rng_range(&gr, 1, 5);
        s->dmg = (uint8_t)rng_range(&gr, 1, 3);
        int d = rng_range(&gr, 0, 3);
        s->death = d < 2 ? DE_NONE : d == 2 ? DE_EXPLODE : DE_SWAMP;
        s->drop = (int8_t)drops[i - 2];
        s->fire_every = (uint8_t)rng_range(&gr, 15, 30); /* x4 frames */
        s->speed = (uint8_t)(s->move == MV_CHASE ? rng_range(&gr, 64, 96) : rng_range(&gr, 80, 112));
    }
    LflSpecies *sn = &lp.sp[SP_SNAIL];
    memset(sn, 0, sizeof *sn);
    sn->hp = 99;
    sn->dmg = 3;
    sn->speed = 72;
    sn->drop = -2;
    LflSpecies *gn = &lp.sp[SP_GNAT];
    memset(gn, 0, sizeof *gn);
    gn->hp = 1;
    gn->dmg = 1;
    gn->speed = 192;
    gn->move = MV_CHASE;
    gn->drop = (int8_t)lp.res_of_tier[TIER_RARE];
}

/* pick the logical layout; false = try again */
static bool gen_logic(void) {
    memset(&lp.room, 0, sizeof lp.room);
    lp.nthing = 0;
    lp.ngate = 0;
    if (rng_chance(&gr, 50)) { lp.gw = 7; lp.gh = 4; } else { lp.gw = 6; lp.gh = 5; }
    for (int y = 0; y < LFL_GMAX_H; y++)
        for (int x = 0; x < LFL_GMAX_W; x++) {
            LflRoom *r = R(x, y);
            r->area = AREA_OVER;
            r->gx = (uint8_t)x;
            r->gy = (uint8_t)y;
            r->secret_x = r->secret_y = r->secret_key = -1;
            r->dun_step = -1;
            if (in_grid(x, y)) { r->exists = 1; r->kind = RK_FIELD; }
        }
    /* about five cells of solid stone, the rest all joined */
    int nstone = rng_range(&gr, 4, 6), placed = 0, guard = 0;
    while (placed < nstone && guard++ < 200) {
        int x = rng_range(&gr, 1, lp.gw), y = rng_range(&gr, 1, lp.gh);
        if (R(x, y)->kind == RK_STONE) continue;
        R(x, y)->kind = RK_STONE;
        if (!grid_connected()) { R(x, y)->kind = RK_FIELD; continue; }
        placed++;
    }
    if (placed < 4) return false;
    int cells[64], nc = 0;
    for (int y = 1; y <= lp.gh; y++)
        for (int x = 1; x <= lp.gw; x++)
            if (R(x, y)->kind == RK_FIELD) cells[nc++] = lfl_room_index(AREA_OVER, x, y);
    shuffle(cells, nc);
    int ci = 0;
    lp.ship_room = (uint8_t)cells[ci++];
    lp.room[lp.ship_room].kind = RK_SHIP;
    /* six or seven villages */
    lp.nvillage = rng_chance(&gr, 40) ? 7 : 6;
    for (int v = 0; v < lp.nvillage; v++) {
        int room = cells[ci++];
        lp.room[room].kind = RK_VILLAGE;
        lp.room[room].village = (uint8_t)(v + 1);
        lp.village[v].room = (uint8_t)room;
    }
    /* the four gated side rooms, on the outer edge, never side by side */
    int cand_x[64], cand_y[64], cand_d[64], ncand = 0;
    for (int y = 1; y <= lp.gh; y++)
        for (int x = 1; x <= lp.gw; x++) {
            if (R(x, y)->kind != RK_FIELD) continue;
            for (int d = 0; d < 4; d++) {
                int nx = x + DX[d], ny = y + DY[d];
                if (in_grid(nx, ny)) continue;
                cand_x[ncand] = nx; cand_y[ncand] = ny; cand_d[ncand] = d; ncand++;
            }
        }
    int order[64];
    for (int i = 0; i < ncand; i++) order[i] = i;
    shuffle(order, ncand);
    int sx[4], sy[4], sd[4], ns = 0;
    for (int k = 0; k < ncand && ns < 4; k++) {
        int i = order[k];
        bool ok = true;
        for (int j = 0; j < ns; j++)
            if (iabs(sx[j] - cand_x[i]) + iabs(sy[j] - cand_y[i]) < 2) ok = false;
        if (!ok) continue;
        sx[ns] = cand_x[i]; sy[ns] = cand_y[i]; sd[ns] = cand_d[i]; ns++;
    }
    if (ns < 4) return false;
    int roles[4] = {GR_MAP, GR_DUNGEON, GR_ITEM, GR_RARE};
    shuffle(roles, 4);
    /* four different kinds; nothing can fetch a staircase across a pit */
    int kinds[GK_COUNT];
    for (int i = 0; i < GK_COUNT; i++) kinds[i] = i;
    shuffle(kinds, GK_COUNT);
    for (int i = 0; i < 4; i++)
        if (roles[i] == GR_DUNGEON && kinds[i] == GK_PIT) {
            int t = kinds[i];
            kinds[i] = kinds[4];
            kinds[4] = t;
        }
    for (int i = 0; i < 4; i++) {
        LflRoom *r = R(sx[i], sy[i]);
        r->exists = 1;
        r->kind = RK_SIDE;
        LflGate *g = &lp.gate[lp.ngate++];
        memset(g, 0, sizeof *g);
        g->kind = (uint8_t)kinds[i];
        g->role = (uint8_t)roles[i];
        g->room = (uint8_t)lfl_room_index(AREA_OVER, sx[i], sy[i]);
        g->side = (int8_t)((sd[i] + 2) & 3); /* the side facing back into the grid */
        g->prize_thing = 255;
    }
    /* rarity, costs */
    int tiers[RS_COUNT] = {0, 1, 2, 3};
    shuffle(tiers, RS_COUNT);
    for (int r = 0; r < RS_COUNT; r++) {
        lp.tier_of[r] = (uint8_t)tiers[r];
        lp.res_of_tier[tiers[r]] = (uint8_t)r;
    }
    memset(lp.cost_res, 255, sizeof lp.cost_res);
    int ct[3] = {1, 2, 3};
    shuffle(ct, 3);
    lp.cost_res[TL_PODS] = lp.res_of_tier[ct[0]];
    lp.cost_res[TL_THUMPER] = lp.res_of_tier[ct[1]];
    lp.cost_res[TL_CHOPS] = lp.res_of_tier[ct[2]];
    /* the tools: five in the open, one behind a gate, two traded */
    int tools[TL_COUNT];
    for (int i = 0; i < TL_COUNT; i++) tools[i] = i;
    shuffle(tools, TL_COUNT);
    for (int i = 0; i < TL_COUNT; i++) lp.item_where[tools[i]] = (uint8_t)(i < 5 ? 0 : i == 5 ? 1 : 2);
    /* the six trades */
    int mt = rng_range(&gr, 1, 3);
    int it[4], ni = 0;
    for (int t = 0; t < 4; t++)
        if (t != mt) it[ni++] = t;
    shuffle(it, ni);
    LflTrade tr[6];
    memset(tr, 0, sizeof tr);
    tr[0].kind = TR_MAP;
    tr[0].pay_res = lp.res_of_tier[mt];
    tr[0].pay_n = (uint8_t)lfl_trade_price(TR_MAP, mt);
    for (int k = 0; k < 2; k++) {
        tr[1 + k].kind = TR_ITEM;
        tr[1 + k].pay_res = lp.res_of_tier[it[k]];
        tr[1 + k].pay_n = (uint8_t)lfl_trade_price(TR_ITEM, it[k]);
        tr[1 + k].item = (uint8_t)tools[6 + k];
    }
    for (int k = 0; k < 3; k++) {
        int a = rng_range(&gr, 0, 3), b;
        do b = rng_range(&gr, 0, 3); while (b == a);
        tr[3 + k].kind = TR_RES;
        tr[3 + k].pay_res = (uint8_t)a;
        tr[3 + k].pay_n = (uint8_t)(16 >> lp.tier_of[a]);
        tr[3 + k].get_res = (uint8_t)b;
        tr[3 + k].get_n = (uint8_t)(32 >> lp.tier_of[b]);
    }
    /* six trade villages: three speak in glyphs; a seventh (plain) keeps the secret */
    int vo[7];
    for (int i = 0; i < lp.nvillage; i++) vo[i] = i;
    shuffle(vo, lp.nvillage);
    int tro[6] = {0, 1, 2, 3, 4, 5};
    shuffle(tro, 6);
    for (int k = 0; k < lp.nvillage; k++) {
        LflVillage *v = &lp.village[vo[k]];
        if (k < 6) {
            v->trade = (int8_t)k;
            v->glyph = (uint8_t)(k < 3);
            lp.trade[k] = tr[tro[k]];
            lp.trade[k].room = v->room;
            lp.trade[k].glyph = v->glyph;
        } else {
            v->trade = -1;
            v->glyph = 0;
        }
    }
    roll_species();
    return lfl_winnable();
}

/* ------------------------------------------------------------------------ */
/* openings between rooms                                                   */

static void set_open(int a, int da, int lo, int hi) {
    LflRoom *r = &lp.room[a];
    r->open |= (uint8_t)(1 << da);
    r->op_lo[da] = (uint8_t)lo;
    r->op_hi[da] = (uint8_t)hi;
}

static void link_rooms(int a, int da, int b, int lo, int hi) {
    set_open(a, da, lo, hi);
    set_open(b, (da + 2) & 3, lo, hi);
}

static void random_opening(int dir, int *lo, int *hi) {
    if (dir == DIR_LEFT || dir == DIR_RIGHT) {
        static const int L[4][2] = {{3, 5}, {2, 6}, {1, 7}, {3, 6}};
        int k = rng_range(&gr, 0, 3);
        *lo = L[k][0]; *hi = L[k][1];
    } else {
        static const int L[5][2] = {{7, 10}, {5, 12}, {3, 14}, {6, 11}, {2, 15}};
        int k = rng_range(&gr, 0, 4);
        *lo = L[k][0]; *hi = L[k][1];
    }
}

/* ------------------------------------------------------------------------ */
/* building rooms                                                           */

static int pick_hostile_room(void) { return SP_HOSTILE0 + rng_range(&gr, 0, 3); }

static int res_by_weight(int wc, int wl, int wm) {
    int r = rng_range(&gr, 1, wc + wl + wm);
    int tier = r <= wc ? TIER_COMMON : r <= wc + wl ? TIER_LESS : TIER_MORE;
    return lp.res_of_tier[tier];
}

/* free ground for extra things: plain floor, reachable, away from the edges */
static bool free_floor(int room, int *ox, int *oy, uint8_t used[LFL_RH][LFL_RW]) {
    LflRoom *r = &lp.room[room];
    for (int k = 0; k < 60; k++) {
        int x = rng_range(&gr, 2, LFL_RW - 3), y = rng_range(&gr, 2, LFL_RH - 3);
        if (r->t[y][x] != T_FLOOR || used[y][x]) continue;
        *ox = x;
        *oy = y;
        return true;
    }
    return false;
}

typedef struct Plan {
    Spots sp;
    uint8_t used[LFL_RH][LFL_RW];
} Plan;
static Plan plans[LFL_MAX_ROOMS];

static void build_field(int room, int forced_secret) {
    LflRoom *r = &lp.room[room];
    Plan *pl = &plans[room];
    uint8_t need[LFL_RH][LFL_RW];
    for (int attempt = 0; attempt < 40; attempt++) {
        lay_border(room);
        const char *tm;
        int ti;
        if (r->kind == RK_VILLAGE) { ti = rng_range(&gr, 0, N_VILLAGE - 1); tm = VILLAGE_T[ti]; }
        else if (r->kind == RK_SHIP) { ti = 0; tm = SHIP_T; }
        else if (forced_secret >= 0) { ti = forced_secret; tm = SECRET_T[ti]; }
        else if (attempt < 30) { ti = rng_range(&gr, 0, N_FIELD - 1); tm = FIELD[ti]; }
        else { ti = 0; tm = FIELD[0]; }
        bool fx = rng_chance(&gr, 50), fy = r->kind == RK_VILLAGE ? false : rng_chance(&gr, 50);
        lay_template(room, tm, fx, fy, &pl->sp);
        r->tmpl = (uint8_t)ti;
        need_openings(room, need);
        for (int k = 0; k < pl->sp.n[SPOT_O]; k++) need[pl->sp.y[SPOT_O][k]][pl->sp.x[SPOT_O][k]] = 1;
        for (int k = 0; k < pl->sp.n[SPOT_G]; k++) {
            int gx = pl->sp.x[SPOT_G][k], gy = pl->sp.y[SPOT_G][k];
            need[gy][gx] = 1;
            need[gy + 1][gx] = 1;
        }
        if (r->kind == RK_SHIP) {
            /* the ship stands in the middle; something to walk round it */
            for (int y = 3; y <= 5; y++)
                for (int x = 7; x <= 10; x++) r->t[y][x] = T_FLOOR;
            r->t[3][8] = r->t[3][9] = r->t[4][8] = r->t[4][9] = T_SHIP;
            lp.ship_tx = 8;
            lp.ship_ty = 3;
            need[5][8] = 1;
        }
        if (pl->sp.n[SPOT_S]) {
            /* the tile in front of the hidden wall must be reachable */
            int sxx = pl->sp.x[SPOT_S][0], syy = pl->sp.y[SPOT_S][0];
            bool found = false;
            for (int d = 0; d < 4 && !found; d++) {
                int nx = sxx + DX[d], ny = syy + DY[d];
                if (r->t[ny][nx] == T_FLOOR || r->t[ny][nx] == T_WOOD) {
                    if (pl->sp.n[SPOT_A] && nx == pl->sp.x[SPOT_A][0] && ny == pl->sp.y[SPOT_A][0]) continue;
                    need[ny][nx] = 1;
                    found = true;
                }
            }
        }
        if (room_connected(room, need, NULL)) return;
    }
    /* a plain room always works */
    lay_border(room);
    for (int y = 1; y < LFL_RH - 1; y++)
        for (int x = 1; x < LFL_RW - 1; x++) r->t[y][x] = T_FLOOR;
    memset(&pl->sp, 0, sizeof pl->sp);
    spot_add(&pl->sp, SPOT_O, 5, 3);
    spot_add(&pl->sp, SPOT_O, 12, 5);
    spot_add(&pl->sp, SPOT_E, 9, 4);
    if (r->kind == RK_SHIP) {
        r->t[3][8] = r->t[3][9] = r->t[4][8] = r->t[4][9] = T_SHIP;
        lp.ship_tx = 8;
        lp.ship_ty = 3;
    }
    if (r->kind == RK_VILLAGE) {
        spot_add(&pl->sp, SPOT_G, 8, 2);
        spot_add(&pl->sp, SPOT_V, 4, 5);
        spot_add(&pl->sp, SPOT_V, 13, 5);
    }
    r->tmpl = 255;
}

static void build_side(int g) {
    LflGate *gt = &lp.gate[g];
    int room = gt->room;
    LflRoom *r = &lp.room[room];
    int side = gt->side;
    for (int y = 0; y < LFL_RH; y++)
        for (int x = 0; x < LFL_RW; x++) r->t[y][x] = T_STONE;
    bool lr = side == DIR_LEFT || side == DIR_RIGHT;
    int l0 = lane0(side);
    /* the chamber */
    int dlo = 4, dhi = lr ? 14 : 7, llo = lr ? 1 : 4, lhi = lr ? 7 : 13;
    for (int d = dlo; d <= dhi; d++)
        for (int l = llo; l <= lhi; l++) side_set(room, side, d, l, T_FLOOR);
    int cd = (dlo + dhi) / 2, cl = lr ? 4 : 8;
    if (gt->kind == GK_PIT && !lr) cl = 11; /* the island beside the way in, not across it */
    int cx, cy;
    side_xy(side, cd, cl, &cx, &cy);
    if (gt->kind == GK_PIT) {
        /* a narrow way in, the prize on an island of pits */
        for (int d = 0; d <= 3; d++) side_set(room, side, d, l0, T_FLOOR);
        r->open |= (uint8_t)(1 << side);
        r->op_lo[side] = r->op_hi[side] = (uint8_t)l0;
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                if (dx || dy) r->t[cy + dy][cx + dx] = T_PIT;
    } else {
        for (int d = 0; d <= 3; d++) {
            side_set(room, side, d, l0, T_FLOOR);
            side_set(room, side, d, l0 + 1, T_FLOOR);
        }
        r->open |= (uint8_t)(1 << side);
        r->op_lo[side] = (uint8_t)l0;
        r->op_hi[side] = (uint8_t)(l0 + 1);
        /* the obstacle, re-using the corridor carver */
        LflGate keep = *gt;
        int saved = lp.ngate;
        lp.ngate = g;
        carve_gate(room, side, gt->kind, gt->role);
        LflGate made = lp.gate[g];
        lp.ngate = saved;
        keep.tx = made.tx; keep.ty = made.ty; keep.w = made.w; keep.h = made.h;
        keep.sx = made.sx; keep.sy = made.sy; keep.snail = 0; keep.snail_face = made.snail_face;
        *gt = keep;
    }
    /* the matching gap in the grid room's stone wall */
    int out = (side + 2) & 3;
    int nb = lfl_room_index(AREA_OVER, r->gx + DX[side], r->gy + DY[side]);
    set_open(nb, out, r->op_lo[side], r->op_hi[side]);
    switch (gt->role) {
    case GR_MAP: gt->prize_thing = (uint8_t)add_thing(TH_MAP, 0, 1, room, cx, cy); break;
    case GR_ITEM:
        for (int t = 0; t < TL_COUNT; t++)
            if (lp.item_where[t] == 1) gt->prize_thing = (uint8_t)add_thing(TH_TOOL, t, 1, room, cx, cy);
        break;
    case GR_RARE: gt->prize_thing = (uint8_t)add_thing(TH_BUNDLE, lp.res_of_tier[TIER_RARE], 2, room, cx, cy); break;
    default:
        r->t[cy][cx] = T_STAIRS_DN;
        lp.dun_entry_room = (uint8_t)room;
        break;
    }
}

/* the dungeon: six rooms in a line, dense and full of foes, its own gates,
 * the third chart piece at the end */
static bool build_dungeon(void) {
    int gx[LFL_DUN_ROOMS], gy[LFL_DUN_ROOMS];
    for (int tries = 0; tries < 100; tries++) {
        gx[0] = rng_range(&gr, 0, LFL_DUN_W - 1);
        gy[0] = rng_range(&gr, 0, LFL_DUN_H - 1);
        int k = 1;
        for (; k < LFL_DUN_ROOMS; k++) {
            int dirs[4] = {0, 1, 2, 3};
            shuffle(dirs, 4);
            bool ok = false;
            for (int j = 0; j < 4 && !ok; j++) {
                int nx = gx[k - 1] + DX[dirs[j]], ny = gy[k - 1] + DY[dirs[j]];
                if (nx < 0 || ny < 0 || nx >= LFL_DUN_W || ny >= LFL_DUN_H) continue;
                bool used = false;
                for (int m = 0; m < k; m++) used |= gx[m] == nx && gy[m] == ny;
                if (used) continue;
                gx[k] = nx; gy[k] = ny; ok = true;
            }
            if (!ok) break;
        }
        if (k == LFL_DUN_ROOMS) break;
        if (tries == 99) return false;
    }
    for (int k = 0; k < LFL_DUN_ROOMS; k++) {
        int room = lfl_room_index(AREA_DUN, gx[k], gy[k]);
        LflRoom *r = &lp.room[room];
        memset(r, 0, sizeof *r);
        r->exists = 1;
        r->kind = RK_DUN;
        r->area = AREA_DUN;
        r->gx = (uint8_t)gx[k];
        r->gy = (uint8_t)gy[k];
        r->secret_x = r->secret_y = r->secret_key = -1;
        r->dun_step = (int8_t)k;
        lp.dun_room[k] = (uint8_t)room;
    }
    /* which passages hold a gate: none, one or two */
    int ngates = rng_range(&gr, 0, 4);
    ngates = ngates == 0 ? 0 : ngates <= 2 ? 1 : 2;
    int gate_at[2] = {-1, -1};
    if (ngates >= 1) gate_at[0] = rng_range(&gr, 0, 2);
    if (ngates >= 2) gate_at[1] = rng_range(&gr, 3, LFL_DUN_ROOMS - 2);
    int dirs_k[LFL_DUN_ROOMS];
    for (int k = 0; k + 1 < LFL_DUN_ROOMS; k++) {
        int a = lp.dun_room[k], b = lp.dun_room[k + 1];
        int d = 0;
        for (; d < 4; d++)
            if (gx[k] + DX[d] == gx[k + 1] && gy[k] + DY[d] == gy[k + 1]) break;
        dirs_k[k] = d;
        bool gated = k == gate_at[0] || k == gate_at[1];
        int lo, hi;
        if (gated) { lo = lane0(d); hi = lo + 1; } else random_opening(d, &lo, &hi);
        link_rooms(a, d, b, lo, hi);
    }
    for (int k = 0; k < LFL_DUN_ROOMS; k++) {
        int room = lp.dun_room[k];
        LflRoom *r = &lp.room[room];
        Plan *pl = &plans[room];
        uint8_t need[LFL_RH][LFL_RW], pass[LFL_RH][LFL_RW];
        bool gated = k == gate_at[0] || k == gate_at[1];
        int gkind = GK_CRYSTAL;
        if (gated) {
            static const int KINDS[6] = {GK_SWAMP, GK_HOLE, GK_CRYSTAL, GK_FOLIAGE, GK_TECH, GK_SNAIL};
            gkind = KINDS[rng_range(&gr, 0, 5)];
        }
        int attempt = 0;
        for (; attempt < 40; attempt++) {
            lay_border(room);
            int ti = rng_range(&gr, 0, N_DUN - 1);
            lay_template(room, attempt < 30 ? DUN_T[ti] : DUN_T[0], rng_chance(&gr, 50), rng_chance(&gr, 50), &pl->sp);
            r->tmpl = (uint8_t)ti;
            need_openings(room, need);
            memset(pass, 0, sizeof pass);
            /* the stairs up in the first room, the chart piece in the last */
            if (k == 0 || k == LFL_DUN_ROOMS - 1) {
                for (int y = 3; y <= 5; y++)
                    for (int x = 7; x <= 10; x++) r->t[y][x] = T_FLOOR;
                need[4][8] = 1;
                if (k == 0) r->t[4][8] = T_STAIRS_UP;
            }
            if (gated) {
                int save_n = lp.ngate;
                carve_gate(room, dirs_k[k], gkind, GR_DUN);
                const LflGate *g = &lp.gate[save_n];
                for (int y = g->ty; y < g->ty + g->h; y++)
                    for (int x = g->tx; x < g->tx + g->w; x++) pass[y][x] = 1;
                if (!room_connected(room, need, pass)) { lp.ngate = save_n; continue; }
                break;
            }
            if (room_connected(room, need, NULL)) break;
        }
        if (attempt >= 40) return false;
        int cnt = 0;
        for (int e = 0; e < pl->sp.n[SPOT_E] && cnt < 4; e++) {
            if (k == 0 && iabs(pl->sp.x[SPOT_E][e] - 8) + iabs(pl->sp.y[SPOT_E][e] - 4) < 4) continue;
            add_spawn(room, SPW_MOB, pick_hostile_room(), pl->sp.x[SPOT_E][e], pl->sp.y[SPOT_E][e]);
            cnt++;
        }
        for (int q = 0; q < pl->sp.n[SPOT_R]; q++)
            add_spawn(room, SPW_RES, res_by_weight(1, 2, 3), pl->sp.x[SPOT_R][q], pl->sp.y[SPOT_R][q]);
        if (k == LFL_DUN_ROOMS - 1) add_thing(TH_MAP, 2, 1, room, 8, 4);
    }
    return true;
}

static bool gen_rooms(void) {
    memset(plans, 0, sizeof plans);
    /* openings in the grid */
    for (int y = 1; y <= lp.gh; y++)
        for (int x = 1; x <= lp.gw; x++) {
            if (R(x, y)->kind == RK_STONE) continue;
            for (int d = DIR_RIGHT; d <= DIR_DOWN; d++) {
                int nx = x + DX[d], ny = y + DY[d];
                if (!in_grid(nx, ny) || R(nx, ny)->kind == RK_STONE) continue;
                int lo, hi;
                random_opening(d, &lo, &hi);
                link_rooms(lfl_room_index(AREA_OVER, x, y), d, lfl_room_index(AREA_OVER, nx, ny), lo, hi);
            }
        }
    for (int g = 0; g < 4; g++) build_side(g);
    /* the room that hides the wall */
    int fields[64], nf = 0;
    for (int y = 1; y <= lp.gh; y++)
        for (int x = 1; x <= lp.gw; x++)
            if (R(x, y)->kind == RK_FIELD) fields[nf++] = lfl_room_index(AREA_OVER, x, y);
    if (nf < 8) return false;
    shuffle(fields, nf);
    lp.secret_room = (uint8_t)fields[0];
    int sec_t = rng_range(&gr, 0, N_SECRET - 1);
    for (int y = 1; y <= lp.gh; y++)
        for (int x = 1; x <= lp.gw; x++) {
            int room = lfl_room_index(AREA_OVER, x, y);
            if (R(x, y)->kind == RK_STONE) {
                for (int yy = 0; yy < LFL_RH; yy++)
                    for (int xx = 0; xx < LFL_RW; xx++) lp.room[room].t[yy][xx] = T_STONE;
                lp.room[room].exists = 1;
                continue;
            }
            build_field(room, room == lp.secret_room ? sec_t : -1);
        }
    /* the hidden wall: the generator chose the look, now the key */
    {
        LflRoom *r = &lp.room[lp.secret_room];
        Plan *pl = &plans[lp.secret_room];
        if (r->tmpl == 255 || !pl->sp.n[SPOT_S] || !pl->sp.n[SPOT_A]) return false;
        lp.secret_key = (uint8_t)rng_range(&gr, 0, SK_COUNT - 1);
        r->secret_x = (int8_t)pl->sp.x[SPOT_S][0];
        r->secret_y = (int8_t)pl->sp.y[SPOT_S][0];
        r->secret_key = (int8_t)lp.secret_key;
        r->t[r->secret_y][r->secret_x] = T_SECRET;
        add_thing(TH_BUNDLE, lp.res_of_tier[TIER_RARE], 4, lp.secret_room, pl->sp.x[SPOT_A][0], pl->sp.y[SPOT_A][0]);
    }
    /* tools in the open, energy cells, each in a room of its own */
    int open_tools[TL_COUNT], no = 0;
    for (int t = 0; t < TL_COUNT; t++)
        if (lp.item_where[t] == 0) open_tools[no++] = t;
    lp.ncubes = (uint8_t)(rng_chance(&gr, 12) ? 5 : 4);
    int want = no + lp.ncubes, fi = 1, placed = 0;
    /* rooms with a spot, the secret room last */
    for (int pass = 0; pass < 2 && placed < want; pass++)
        for (fi = 1; fi < nf && placed < want; fi++) {
            int room = fields[fi];
            Plan *pl = &plans[room];
            if (!pl->sp.n[SPOT_O]) continue;
            int k = rng_range(&gr, 0, pl->sp.n[SPOT_O] - 1);
            int tx = pl->sp.x[SPOT_O][k], ty = pl->sp.y[SPOT_O][k];
            if (pl->used[ty][tx]) continue;
            if (pass == 0 && (pl->used[0][0] & 1)) continue;
            pl->used[ty][tx] = 1;
            pl->used[0][0] |= 1;
            if (placed < no) add_thing(TH_TOOL, open_tools[placed], 1, room, tx, ty);
            else add_thing(TH_CUBE, 0, 1, room, tx, ty);
            placed++;
        }
    if (placed < want) return false;
    /* foes, resources, decoration shells; villagers */
    for (int y = 1; y <= lp.gh; y++)
        for (int x = 1; x <= lp.gw; x++) {
            int room = lfl_room_index(AREA_OVER, x, y);
            LflRoom *r = &lp.room[room];
            Plan *pl = &plans[room];
            if (r->kind == RK_STONE) continue;
            if (r->kind == RK_VILLAGE) {
                LflVillage *v = &lp.village[r->village - 1];
                int sp = v->glyph ? SP_FRIEND_B : SP_FRIEND_A;
                if (pl->sp.n[SPOT_G]) {
                    v->gx = (int8_t)pl->sp.x[SPOT_G][0];
                    v->gy = (int8_t)pl->sp.y[SPOT_G][0];
                    add_spawn(room, SPW_MOB, sp | 0x80, v->gx, v->gy);
                }
                for (int k = 0; k < pl->sp.n[SPOT_V]; k++) add_spawn(room, SPW_MOB, sp, pl->sp.x[SPOT_V][k], pl->sp.y[SPOT_V][k]);
                continue;
            }
            if (r->kind == RK_SHIP) continue;
            if (r->kind != RK_FIELD) continue;
            int sp = pick_hostile_room(), sp2 = rng_chance(&gr, 30) ? pick_hostile_room() : sp;
            int want_mobs = rng_range(&gr, 0, 3);
            for (int e = 0; e < pl->sp.n[SPOT_E] && e < want_mobs; e++)
                add_spawn(room, SPW_MOB, e & 1 ? sp2 : sp, pl->sp.x[SPOT_E][e], pl->sp.y[SPOT_E][e]);
            for (int k = 0; k < pl->sp.n[SPOT_N]; k++) add_spawn(room, SPW_SNAIL, SP_SNAIL, pl->sp.x[SPOT_N][k], pl->sp.y[SPOT_N][k]);
            for (int k = 0; k < pl->sp.n[SPOT_O]; k++) {
                int tx = pl->sp.x[SPOT_O][k], ty = pl->sp.y[SPOT_O][k];
                if (pl->used[ty][tx]) continue;
                pl->used[ty][tx] = 1;
                add_spawn(room, SPW_RES, res_by_weight(6, 3, 1), tx, ty);
            }
            for (int k = 0; k < pl->sp.n[SPOT_R]; k++) add_spawn(room, SPW_RES, res_by_weight(1, 3, 3), pl->sp.x[SPOT_R][k], pl->sp.y[SPOT_R][k]);
            int extra = rng_range(&gr, 1, 2);
            for (int k = 0; k < extra; k++) {
                int tx, ty;
                if (!free_floor(room, &tx, &ty, pl->used)) break;
                pl->used[ty][tx] = 1;
                add_spawn(room, SPW_RES, res_by_weight(6, 3, 1), tx, ty);
            }
        }
    /* one glyph speaker who stays untranslated */
    lp.mute_room = 255;
    for (int v = 0; v < lp.nvillage; v++)
        if (lp.village[v].glyph) { lp.mute_room = lp.village[v].room; break; }
    if (!build_dungeon()) return false;
    lp.warp_room[0] = lp.warp_room[1] = -1;
    return true;
}

static void pick_colours(void) {
    static const uint8_t GROUND[][4] = {
        {C_EARTH, C_BROWN, C_SLATE, C_FOREST},  {C_PURPLE, C_NIGHT, C_GREY, C_MAGENTA},
        {C_TEAL, C_NAVY, C_SLATE, C_JADE},      {C_MAROON, C_INK, C_GREY, C_ORANGE},
        {C_HIDE, C_EARTH, C_DUSK, C_LEAF},      {C_DUSK, C_NIGHT, C_SLATE, C_CYAN},
    };
    int k = rng_range(&gr, 0, ARRAY_LEN(GROUND) - 1);
    lp.ground_col = GROUND[k][0];
    lp.ground_dark = GROUND[k][1];
    lp.wall_col = GROUND[k][2];
    lp.plant_col = GROUND[k][3];
}

void lfl_generate(uint32_t seed) {
    memset(&lp, 0, sizeof lp);
    lp.seed = seed;
    rng_seed(&gr, seed * 2654435761u + 8);
    for (int tries = 0; tries < 400; tries++) {
        uint8_t keep_tries = (uint8_t)imin(255, tries);
        if (!gen_logic()) continue;
        if (!gen_rooms()) continue;
        if (lfl_gen_audit() != 0) continue;
        lp.gen_tries = keep_tries;
        break;
    }
    pick_colours();
}

/* the hint the seventh village gives: where, then how */
const char *lfl_hint_line(int i) {
    static char buf[48];
    if (lp.secret_room == 255) return "";
    const LflRoom *r = &lp.room[lp.secret_room];
    if (i == 0) return r->tmpl < N_SECRET ? SECRET_WHERE[r->tmpl] : "";
    switch (lp.secret_key) {
    case SK_THUMPER: return "A THUMP WAKES THE SLEEPING STONE";
    case SK_PODS: return "A BLAST WILL OPEN IT";
    case SK_RIVET: return "A FEW RIVETS WILL CRACK IT";
    case SK_SMALL: return "ONLY THE VERY SMALL GET IN";
    default: {
        /* the disc: from which side to throw */
        static const char *const SIDE[4] = {"NORTH", "EAST", "SOUTH", "WEST"};
        int side = 0;
        for (int d = 0; d < 4; d++) {
            int t = lfl_tile(lp.secret_room, r->secret_x + DX[d], r->secret_y + DY[d]);
            if (t == T_FLOOR || t == T_WOOD) { side = d; break; }
        }
        snprintf(buf, sizeof buf, "STAND %s AND THROW WHAT RETURNS", SIDE[side]);
        return buf;
    }
    }
}

/* ------------------------------------------------------------------------ */
/* the rules, checked (tests): 0 = all hold, else the first broken rule     */

int lfl_gen_audit(void) {
    if (!((lp.gw == 7 && lp.gh == 4) || (lp.gw == 6 && lp.gh == 5))) return 1;
    int stone = 0;
    for (int y = 1; y <= lp.gh; y++)
        for (int x = 1; x <= lp.gw; x++) stone += R(x, y)->kind == RK_STONE;
    if (stone < 4 || stone > 6 || !grid_connected()) return 2;
    int roles = 0, over = 0;
    for (int g = 0; g < lp.ngate; g++) {
        const LflGate *gt = &lp.gate[g];
        if (gt->role == GR_DUN) continue;
        over++;
        roles |= 1 << gt->role;
        const LflRoom *r = &lp.room[gt->room];
        if (r->kind != RK_SIDE || in_grid(r->gx, r->gy)) return 3;
        for (int h = 0; h < g; h++) {
            const LflRoom *q = &lp.room[lp.gate[h].room];
            if (lp.gate[h].role != GR_DUN && iabs(q->gx - r->gx) + iabs(q->gy - r->gy) < 2) return 4;
        }
        if (gt->role == GR_DUNGEON && gt->kind == GK_PIT) return 3;
    }
    if (over != 4 || roles != 0xF) return 3;
    if (lp.nvillage < 6 || lp.nvillage > 7) return 5;
    int glyph = 0, plain = 0, hint = 0;
    for (int v = 0; v < lp.nvillage; v++) {
        if (lp.village[v].trade < 0) hint++;
        else if (lp.village[v].glyph) glyph++;
        else plain++;
    }
    if (glyph != 3 || plain != 3 || hint != lp.nvillage - 6) return 5;
    int kinds[3] = {0, 0, 0};
    int used_res = 0;
    for (int i = 0; i < 6; i++) {
        const LflTrade *t = &lp.trade[i];
        kinds[t->kind]++;
        int tier = lp.tier_of[t->pay_res];
        if (t->kind == TR_MAP) {
            if (tier == TIER_COMMON || t->pay_n != lfl_trade_price(TR_MAP, tier)) return 6;
        }
        if (t->kind == TR_ITEM && t->pay_n != lfl_trade_price(TR_ITEM, tier)) return 6;
        if (t->kind != TR_RES) {
            if (used_res & (1 << t->pay_res)) return 6;
            used_res |= 1 << t->pay_res;
        }
        if (t->kind == TR_RES && t->pay_res == t->get_res) return 6;
    }
    if (kinds[TR_RES] != 3 || kinds[TR_ITEM] != 2 || kinds[TR_MAP] != 1) return 6;
    int where[3] = {0, 0, 0};
    for (int t = 0; t < TL_COUNT; t++) where[lp.item_where[t]]++;
    if (where[0] != 5 || where[1] != 1 || where[2] != 2) return 7;
    int tools_on_map = 0, cubes = 0, maps = 0, rare_gate = 0;
    for (int i = 0; i < lp.nthing; i++) {
        const LflThing *t = &lp.thing[i];
        if (t->kind == TH_TOOL) tools_on_map++;
        if (t->kind == TH_CUBE) cubes++;
        if (t->kind == TH_MAP) maps++;
        if (t->kind == TH_BUNDLE && t->n == 2 && t->sub == lp.res_of_tier[TIER_RARE] && lp.room[t->room].kind == RK_SIDE) rare_gate++;
    }
    if (tools_on_map != 6) return 7;
    if (cubes < 4 || cubes > 5) return 8;
    if (maps != 2) return 9;
    if (rare_gate != 1) return 10;
    if (!lfl_winnable()) return 11;
    int friendly = 0, hostile = 0, meat = 0, drops = 0;
    for (int s = 0; s < 6; s++) {
        if (lp.sp[s].friendly) friendly++;
        else {
            hostile++;
            if (lp.sp[s].drop < 0) meat++;
            else {
                if (lp.tier_of[lp.sp[s].drop] == TIER_RARE) return 12;
                drops |= 1 << lp.sp[s].drop;
            }
            if (lp.sp[s].hp < 1 || lp.sp[s].hp > 5 || lp.sp[s].dmg < 1 || lp.sp[s].dmg > 3) return 12;
        }
    }
    if (friendly != 2 || hostile != 4 || meat != 1 || drops != (0xF & ~(1 << lp.res_of_tier[TIER_RARE]))) return 12;
    if (lp.cost_res[TL_PODS] == lp.cost_res[TL_THUMPER] || lp.cost_res[TL_PODS] == lp.cost_res[TL_CHOPS] ||
        lp.cost_res[TL_THUMPER] == lp.cost_res[TL_CHOPS])
        return 13;
    if (lp.tier_of[lp.cost_res[TL_PODS]] == TIER_COMMON || lp.tier_of[lp.cost_res[TL_THUMPER]] == TIER_COMMON ||
        lp.tier_of[lp.cost_res[TL_CHOPS]] == TIER_COMMON)
        return 13;
    if (lp.secret_room == 255 || lp.room[lp.secret_room].secret_key < 0) return 14;
    for (int k = 0; k < LFL_DUN_ROOMS; k++)
        if (!lp.room[lp.dun_room[k]].exists || lp.room[lp.dun_room[k]].dun_step != k) return 15;
    for (int k = 0; k + 1 < LFL_DUN_ROOMS; k++) {
        bool linked = false;
        for (int d = 0; d < 4; d++) linked |= lfl_neighbor(lp.dun_room[k], d) == lp.dun_room[k + 1];
        if (!linked) return 15;
    }
    return 0;
}
