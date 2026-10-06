/* CLARION CALL - the generators.
 *
 * Every area but the last is generated afresh for each run, the way the
 * original's are: a map of 8 x 5 cells (16 x 12 tiles each), joined by a
 * tree of passages with some extra loops, each cell hollowed into a
 * chamber of the region's shape. Passages are three or four tiles wide,
 * sometimes a two-tile chute. Floors stay level so the ship can land and
 * Clary can walk to the doors, ceilings and walls are roughened, ladders
 * hang in some of the shafts, and coin blocks sit in the walls. Then come
 * the doors (each on a floor with room to land beside it), fourteen notes
 * (ten open the gold door; the spares turn to plum coins), the region's
 * enemies, the pods that hatch when the dash starts, and the spots where
 * clock rings show up.
 *
 * A map is only kept if the ship can reach every note, every door's
 * landing spot and the gold door's from where it starts, on the real
 * collision rules, and if a note waits within easy reach of the start.
 * Otherwise the next try is made from the same seed.
 *
 * The caves behind red and gold doors are strung together from our own
 * cave pieces (sixteen tiles wide), with the region's enemies and hazards
 * in the slots each piece leaves for them. Rooms have fixed shapes. The
 * Crown (the Spire's third area) and the escape shaft are fixed. */
#include "clc.h"

const char *const CLC_REGION_NAME[RG_COUNT] = {"THE CELLARS", "THE ARBORETUM", "THE ICEHOUSE", "THE GULLET",
                                               "COGTOWN", "THE CLOISTER", "THE SPIRE"};

int clc_region_tier(int region) {
    static const int8_t TIER[RG_COUNT] = {0, 1, 1, 2, 2, 2, 3};
    return region >= 0 && region < RG_COUNT ? TIER[region] : 0;
}

bool clc_map_solid_tile(int t) { return t != MT_AIR && t != MT_LADDER && t != MT_GATE_OPEN; }

bool clc_ship_cell_free(const ClcWorld *w, int c, int r) {
    if (c < 1 || r < 1 || c >= w->w || r >= w->h) return false;
    return !clc_map_solid_tile(w->tile[r - 1][c - 1]) && !clc_map_solid_tile(w->tile[r - 1][c]) &&
           !clc_map_solid_tile(w->tile[r][c - 1]) && !clc_map_solid_tile(w->tile[r][c]);
}

void clc_door_pad(const ClcDoor *d, int *x, int *y) {
    *x = d->pad_c * CLC_T + CLC_T / 2;
    *y = (d->r + 1) * CLC_T - 4; /* the ship's middle when it sits on the floor */
}

/* ---- region shapes ---------------------------------------------------- */

typedef struct Shape {
    uint8_t extra;             /* % of the other links added as loops */
    int8_t vbias;              /* + prefers shafts, - prefers halls */
    uint8_t wmin, wmax, hmin, hmax;
    uint8_t pmin, pmax;        /* passage widths */
    uint8_t chute;             /* % of passages that are two-tile chutes */
    uint8_t rough;             /* % of ceiling and wall tiles nibbled */
    uint8_t islands;           /* rocks hung in big chambers */
    uint8_t ladders;           /* % of shafts with a ladder */
} Shape;

static const Shape SHAPE[RG_COUNT] = {
    /* cellars: roomy and plain, an introduction */
    {30, 0, 9, 13, 6, 9, 3, 4, 0, 30, 1, 50},
    /* arboretum: tall trunks and shafts, hungry for fuel */
    {25, 3, 5, 9, 8, 10, 3, 4, 15, 45, 1, 60},
    /* icehouse: long low halls */
    {40, -3, 10, 14, 5, 8, 3, 5, 0, 20, 2, 30},
    /* gullet: winding and lumpy */
    {15, 0, 7, 12, 6, 9, 3, 4, 20, 60, 2, 40},
    /* cogtown: square rooms on a grid */
    {50, 0, 9, 13, 6, 9, 3, 4, 10, 0, 1, 50},
    /* cloister: a labyrinth of small rooms and dead ends */
    {6, 0, 6, 9, 5, 7, 3, 3, 10, 25, 0, 30},
    /* spire: a bit of everything */
    {30, 1, 7, 13, 6, 10, 3, 4, 15, 35, 2, 40},
};

/* what lives where: map enemies by region (weights) */
typedef struct Spawn { uint8_t kind, weight; } Spawn;
static const Spawn MAP_FOES[RG_COUNT][6] = {
    {{EK_FLITTER, 5}, {EK_CREEPER, 4}, {EK_BIGBELLY, 1}, {0, 0}, {0, 0}, {0, 0}},
    {{EK_GRUB, 4}, {EK_FLITTER, 3}, {EK_SNAP, 3}, {EK_CHASER, 2}, {EK_BIGBELLY, 1}, {0, 0}},
    {{EK_FIREDRONE, 4}, {EK_WINDDRONE, 3}, {EK_FLITTER, 2}, {EK_BIGBELLY, 1}, {0, 0}, {0, 0}},
    {{EK_LEECH, 4}, {EK_EYE, 3}, {EK_ACID, 3}, {EK_BLOB, 3}, {EK_BIGBELLY, 1}, {0, 0}},
    {{EK_SENTRY, 4}, {EK_SNAKE, 2}, {EK_CRAB, 3}, {EK_CHASER, 2}, {EK_FLITTER, 2}, {0, 0}},
    {{EK_JETFIRE, 3}, {EK_FLITTER, 2}, {EK_CHASER, 1}, {0, 0}, {0, 0}, {0, 0}},
    {{EK_BOOMER, 3}, {EK_WORM, 2}, {EK_EYE, 2}, {EK_CHASER, 2}, {EK_LEECH, 2}, {EK_FIREDRONE, 2}},
};

/* where a kind lives: 0 the air, 1 a floor, 2 a floor or a ceiling, 3 a side wall */
static int habitat(int k) {
    switch (k) {
    case EK_CREEPER: case EK_SENTRY: return 2;
    case EK_GRUB: case EK_SNAP: return 1;
    case EK_EYE: case EK_ACID: case EK_CRAB: case EK_BOOMER: case EK_JETFIRE: return 3;
    default: return 0;
    }
}

/* ---- carving helpers --------------------------------------------------- */

static int GW_(void) { return CLC_GW; }

static void carve(ClcWorld *w, int c0, int r0, int c1, int r1) {
    for (int r = imax(1, r0); r <= imin(w->h - 2, r1); r++)
        for (int c = imax(1, c0); c <= imin(w->w - 2, c1); c++) w->tile[r][c] = MT_AIR;
}

static void fill(ClcWorld *w, int c0, int r0, int c1, int r1, int t) {
    for (int r = imax(0, r0); r <= imin(w->h - 1, r1); r++)
        for (int c = imax(0, c0); c <= imin(w->w - 1, c1); c++) w->tile[r][c] = (uint8_t)t;
}

typedef struct Room { int x0, y0, x1, y1; } Room;

/* ---- the ship's reach --------------------------------------------------- */

static uint16_t g_dist[CLC_MH + 1][CLC_MW + 1];

/* distances (in ship cells) from corner cell (c, r) over every cell with
 * room for the ship; 0xFFFF where it can't go */
static void ship_bfs(const ClcWorld *w, int c, int r) {
    static int16_t q[(CLC_MH + 1) * (CLC_MW + 1)][2];
    for (int y = 0; y <= CLC_MH; y++)
        for (int x = 0; x <= CLC_MW; x++) g_dist[y][x] = 0xFFFF;
    if (!clc_ship_cell_free(w, c, r)) return;
    int head = 0, tail = 0;
    g_dist[r][c] = 0;
    q[tail][0] = (int16_t)c;
    q[tail][1] = (int16_t)r;
    tail++;
    static const int8_t D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (head < tail) {
        int x = q[head][0], y = q[head][1];
        head++;
        for (int k = 0; k < 4; k++) {
            int nx = x + D[k][0], ny = y + D[k][1];
            if (nx < 1 || ny < 1 || nx >= w->w || ny >= w->h || g_dist[ny][nx] != 0xFFFF) continue;
            if (!clc_ship_cell_free(w, nx, ny)) continue;
            g_dist[ny][nx] = (uint16_t)(g_dist[y][x] + 1);
            q[tail][0] = (int16_t)nx;
            q[tail][1] = (int16_t)ny;
            tail++;
        }
    }
}

/* the ship cell nearest to a point (px), as reached */
static int reach_at(int x, int y) {
    int c = (x + CLC_T / 2) / CLC_T, r = (y + CLC_T / 2) / CLC_T;
    int best = 0xFFFF;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int cc = c + dx, rr = r + dy;
            if (cc < 0 || rr < 0 || cc > CLC_MW || rr > CLC_MH) continue;
            if (g_dist[rr][cc] < best) best = g_dist[rr][cc];
        }
    return best;
}

/* ---- the plan: which doors an area holds --------------------------------- */

typedef struct Plan { uint8_t type, room; } Plan;

static int plan_doors(const ClcAreaSpec *sp, Rng *rng, Plan *out) {
    int n = 0, reg = sp->region, a = sp->area;
#define ADD(t, rm) (out[n].type = (uint8_t)(t), out[n].room = (uint8_t)(rm), n++)
    /* the red cave with its chest, unless a yellow door takes its place */
    bool cave = !((reg == RG_ICE && a == 1) || (reg == RG_COG && a == 0));
    if (cave) ADD(DT_RED, RM_CAVE);
    /* the friendly sort behind a red door, or what replaces it */
    int r3 = rng_range(rng, 0, 2), r2 = rng_range(rng, 0, 1);
    switch (reg) {
    case RG_CELLARS:
        if (a == 0) ADD(DT_RED, r3 == 0 ? RM_TRIAL : RM_NPC);
        else if (r2) ADD(DT_BLUE, RM_HEALTH);
        else ADD(DT_RED, RM_NPC);
        break;
    case RG_ARBOR:
        if (a == 0) ADD(DT_RED, RM_CHARMSHOP);
        else ADD(DT_RED, r2 ? RM_CURSED : RM_NPC);
        break;
    case RG_ICE:
        if (a == 0) { if (r2) ADD(DT_BLUE, RM_HEALTH); else ADD(DT_RED, RM_NPC); }
        else ADD(DT_RED, r3 == 0 ? RM_TRIAL : RM_NPC);
        break;
    case RG_GULLET:
        if (a == 0) ADD(DT_YELLOW, RM_ARMOR);
        else ADD(DT_RED, RM_NPC);
        break;
    case RG_COG:
        if (a == 0) ADD(DT_RED, r3 == 0 ? RM_TRIAL : RM_NPC);
        else if (r3 == 2) ADD(DT_BLUE, RM_HEALTH);
        else ADD(DT_RED, r3 == 1 ? RM_TRIAL : RM_NPC);
        break;
    case RG_CLOISTER:
        if (a == 0) ADD(DT_RED, RM_LIGHTS);
        else ADD(DT_RED, r3 == 0 ? RM_NPC : r3 == 1 ? RM_TRIAL : RM_CURSED);
        break;
    case RG_SPIRE:
        if (a == 0) ADD(DT_YELLOW, RM_MAGNETS);
        else if (r2) ADD(DT_BLUE, RM_HEALTH);
        else ADD(DT_RED, RM_NPC);
        break;
    default: break;
    }
    /* the yellow doors that stand for a cave */
    if (reg == RG_ICE && a == 1) ADD(DT_YELLOW, RM_BOON);
    if (reg == RG_COG && a == 0) ADD(DT_YELLOW, RM_SHORTCUT);
    /* a shop in every area from the Cellars' second on */
    if (!(reg == RG_CELLARS && a == 0)) ADD(DT_GREEN, reg == RG_COG && a == 1 ? RM_MEGA : RM_SHOP);
    if (sp->sage) ADD(DT_RED, RM_SAGE);
    /* the Cloister's first area: six red doors with nothing behind them */
    if (reg == RG_CLOISTER && a == 0)
        for (int k = 0; k < 6; k++) ADD(DT_RED, RM_EMPTY);
    ADD(DT_GOLD, RM_GOLDCAVE);
#undef ADD
    return n;
}

/* ---- the generator -------------------------------------------------------- */

typedef struct Strip { int16_t r, c0, c1; uint8_t cell, used; } Strip;
#define MAX_STRIPS 160

static bool is_air(const ClcWorld *w, int c, int r) {
    return c >= 0 && r >= 0 && c < w->w && r < w->h && w->tile[r][c] == MT_AIR;
}

static bool is_rock(const ClcWorld *w, int c, int r) {
    return c >= 0 && r >= 0 && c < w->w && r < w->h && (w->tile[r][c] == MT_ROCK || w->tile[r][c] == MT_ROCK2);
}

static int find_strips(const ClcWorld *w, Strip *s) {
    int n = 0;
    for (int r = 3; r < w->h - 1 && n < MAX_STRIPS; r++) {
        int c = 1;
        while (c < w->w - 1 && n < MAX_STRIPS) {
            bool ok = is_rock(w, c, r + 1) && is_air(w, c, r) && is_air(w, c, r - 1) && is_air(w, c, r - 2);
            if (!ok) { c++; continue; }
            int c0 = c;
            while (c < w->w - 1 && is_rock(w, c, r + 1) && is_air(w, c, r) && is_air(w, c, r - 1) && is_air(w, c, r - 2)) c++;
            if (c - c0 >= 7) {
                s[n].r = (int16_t)r;
                s[n].c0 = (int16_t)c0;
                s[n].c1 = (int16_t)(c - 1);
                s[n].cell = (uint8_t)((r / CLC_CH) * CLC_GW + ((c0 + c) / 2) / CLC_CW);
                s[n].used = 0;
                n++;
            }
        }
    }
    return n;
}

/* is (c, r) a good spot for a thing in the air: room for the ship all round */
static bool open_spot(const ClcWorld *w, int c, int r) {
    for (int dy = 0; dy <= 1; dy++)
        for (int dx = 0; dx <= 1; dx++)
            if (!clc_ship_cell_free(w, c + dx, r + dy)) return false;
    return true;
}

static bool near_thing(const ClcWorld *w, int x, int y, int d) {
    for (int i = 0; i < w->ne; i++) {
        const ClcEnt *e = &w->e[i];
        if (!e->on) continue;
        if (iabs((int)(e->x >> 8) - x) < d && iabs((int)(e->y >> 8) - y) < d) return true;
    }
    for (int k = 0; k < w->nd; k++) {
        int px, py;
        clc_door_pad(&w->door[k], &px, &py);
        if (iabs(px - x) < d + 16 && iabs(py - y) < d + 8) return true;
        int dx = w->door[k].c * CLC_T + 4;
        if (iabs(dx - x) < d && iabs(py - y) < d + 8) return true;
    }
    return false;
}

static void build_map(ClcWorld *w, const ClcAreaSpec *sp, Rng *rng, Room *rooms, int *start_cell) {
    const Shape *sh = &SHAPE[sp->region];
    w->w = CLC_GW * CLC_CW;
    w->h = CLC_GH * CLC_CH;
    fill(w, 0, 0, CLC_MW - 1, CLC_MH - 1, MT_ROCK);
    /* deeper rock for looks */
    for (int r = 0; r < w->h; r++)
        for (int c = 0; c < w->w; c++)
            if (((c * 7 + r * 13) ^ (c * r)) % 11 == 0) w->tile[r][c] = MT_ROCK2;

    /* links: hl[y][x] joins (x,y)-(x+1,y); vl[y][x] joins (x,y)-(x,y+1) */
    uint8_t hl[CLC_GH][CLC_GW], vl[CLC_GH][CLC_GW], seen[CLC_GH][CLC_GW];
    memset(hl, 0, sizeof hl);
    memset(vl, 0, sizeof vl);
    memset(seen, 0, sizeof seen);
    int layout = rng_range(rng, 0, 2);
    if (sp->region == RG_CLOISTER) layout = 0;
    int sx = rng_range(rng, 0, GW_() - 1), sy = rng_range(rng, 0, CLC_GH - 1);
    if (sp->region == RG_CELLARS && sp->area == 0) { sx = rng_range(rng, 0, 1); sy = CLC_GH - 1; }
    *start_cell = sy * CLC_GW + sx;
    if (layout == 2) {
        /* the serpent: every row a hall, the rows joined end to end */
        for (int y = 0; y < CLC_GH; y++) {
            for (int x = 0; x < CLC_GW - 1; x++) hl[y][x] = 1;
            if (y < CLC_GH - 1) vl[y][(y & 1) ? 0 : CLC_GW - 1] = 1;
        }
    } else {
        /* a random tree: depth first (layout 0) or grown from a frontier (1) */
        static int16_t stack[CLC_GW * CLC_GH * 4];
        int top = 0;
        stack[top++] = (int16_t)(sy * CLC_GW + sx);
        seen[sy][sx] = 1;
        while (top > 0) {
            int pick = layout == 0 ? top - 1 : rng_range(rng, 0, top - 1);
            int cur = stack[pick], x = cur % CLC_GW, y = cur / CLC_GW;
            int opts[4], wts[4], no = 0, tw = 0;
            static const int8_t D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (int k = 0; k < 4; k++) {
                int nx = x + D[k][0], ny = y + D[k][1];
                if (nx < 0 || ny < 0 || nx >= CLC_GW || ny >= CLC_GH || seen[ny][nx]) continue;
                int wt = 4 + (k >= 2 ? sh->vbias : -sh->vbias);
                if (wt < 1) wt = 1;
                opts[no] = k;
                wts[no] = wt;
                tw += wt;
                no++;
            }
            if (!no) {
                stack[pick] = stack[--top];
                continue;
            }
            int roll = rng_range(rng, 0, tw - 1), k = 0;
            while (roll >= wts[k]) roll -= wts[k++];
            int d = opts[k], nx = x + D[d][0], ny = y + D[d][1];
            if (d == 0) hl[y][x] = 1;
            else if (d == 1) hl[y][nx] = 1;
            else if (d == 2) vl[y][x] = 1;
            else vl[ny][x] = 1;
            seen[ny][nx] = 1;
            stack[top++] = (int16_t)(ny * CLC_GW + nx);
        }
    }
    /* loops */
    for (int y = 0; y < CLC_GH; y++)
        for (int x = 0; x < CLC_GW; x++) {
            int bias = sh->vbias * 4;
            if (x < CLC_GW - 1 && !hl[y][x] && rng_chance(rng, imax(0, sh->extra - bias))) hl[y][x] = 1;
            if (y < CLC_GH - 1 && !vl[y][x] && rng_chance(rng, imax(0, sh->extra + bias))) vl[y][x] = 1;
        }

    /* chambers */
    for (int y = 0; y < CLC_GH; y++)
        for (int x = 0; x < CLC_GW; x++) {
            Room *rm = &rooms[y * CLC_GW + x];
            int ww = rng_range(rng, sh->wmin, sh->wmax), hh = rng_range(rng, sh->hmin, sh->hmax);
            ww = imin(ww, CLC_CW - 3);
            hh = imin(hh, CLC_CH - 3);
            int x0 = x * CLC_CW + 1 + rng_range(rng, 0, CLC_CW - 3 - ww);
            int y0 = y * CLC_CH + 1 + rng_range(rng, 0, CLC_CH - 3 - hh);
            rm->x0 = x0;
            rm->y0 = y0;
            rm->x1 = x0 + ww - 1;
            rm->y1 = y0 + hh - 1;
            carve(w, rm->x0, rm->y0, rm->x1, rm->y1);
        }

    /* passages */
    for (int y = 0; y < CLC_GH; y++)
        for (int x = 0; x < CLC_GW; x++) {
            const Room *a = &rooms[y * CLC_GW + x];
            if (x < CLC_GW - 1 && hl[y][x]) {
                const Room *b = &rooms[y * CLC_GW + x + 1];
                int p = rng_chance(rng, sh->chute) ? 2 : rng_range(rng, sh->pmin, sh->pmax);
                int lo = imax(a->y0, b->y0), hi = imin(a->y1, b->y1);
                if (hi - lo + 1 >= p) {
                    /* straight across, level with the floors when it can be */
                    int top = rng_chance(rng, 60) ? hi - p + 1 : rng_range(rng, lo, hi - p + 1);
                    carve(w, a->x1, top, b->x0, top + p - 1);
                } else {
                    /* a dog-leg at the border between the cells */
                    int bx = (x + 1) * CLC_CW - 1;
                    int ya = imin(a->y1 - p + 1, imax(a->y0, a->y1 - p + 1));
                    int yb = imin(b->y1 - p + 1, imax(b->y0, b->y1 - p + 1));
                    carve(w, a->x1, ya, bx + p - 1, ya + p - 1);
                    carve(w, bx, imin(ya, yb), bx + p - 1, imax(ya, yb) + p - 1);
                    carve(w, bx, yb, b->x0, yb + p - 1);
                }
            }
            if (y < CLC_GH - 1 && vl[y][x]) {
                const Room *b = &rooms[(y + 1) * CLC_GW + x];
                int p = rng_chance(rng, sh->chute) ? 2 : rng_range(rng, sh->pmin, sh->pmax);
                int lo = imax(a->x0, b->x0), hi = imin(a->x1, b->x1);
                int left;
                if (hi - lo + 1 >= p) {
                    left = rng_range(rng, lo, hi - p + 1);
                    carve(w, left, a->y1, left + p - 1, b->y0);
                } else {
                    int by = (y + 1) * CLC_CH - 1;
                    int xa = a->x0 + (a->x1 - a->x0 - p) / 2, xb = b->x0 + (b->x1 - b->x0 - p) / 2;
                    carve(w, xa, a->y1, xa + p - 1, by + p - 1);
                    carve(w, imin(xa, xb), by, imax(xa, xb) + p - 1, by + p - 1);
                    carve(w, xb, by, xb + p - 1, b->y0);
                    left = xb;
                }
                /* a ladder hanging down the shaft, top to the floor below */
                if (p >= 3 && rng_chance(rng, sh->ladders)) {
                    int lc = left;
                    int r0 = a->y1 + 1, r1 = b->y1;
                    bool ok = is_rock(w, lc, r1 + 1);
                    for (int r = r0; r <= r1 && ok; r++) ok = w->tile[r][lc] == MT_AIR;
                    if (ok)
                        for (int r = r0; r <= r1; r++) w->tile[r][lc] = MT_LADDER;
                }
            }
        }

    /* roughen the ceilings and the walls (never the floors) */
    static uint8_t copy[CLC_MH][CLC_MW];
    memcpy(copy, w->tile, sizeof copy);
    for (int r = 2; r < w->h - 2; r++)
        for (int c = 2; c < w->w - 2; c++) {
            if (copy[r][c] != MT_ROCK && copy[r][c] != MT_ROCK2) continue;
            bool below = copy[r + 1][c] == MT_AIR, side = copy[r][c - 1] == MT_AIR || copy[r][c + 1] == MT_AIR;
            bool above = copy[r - 1][c] == MT_AIR;
            if (above) continue;
            if ((below && rng_chance(rng, sh->rough / 2)) || (side && !below && rng_chance(rng, sh->rough / 3)))
                w->tile[r][c] = MT_AIR;
        }
    /* rocks hung in the big chambers */
    for (int i = 0; i < CLC_GW * CLC_GH; i++) {
        const Room *rm = &rooms[i];
        if (rm->x1 - rm->x0 < 9 || rm->y1 - rm->y0 < 7) continue;
        for (int k = 0; k < sh->islands; k++) {
            if (!rng_chance(rng, 55)) continue;
            int iw = rng_range(rng, 2, 3), ih = rng_range(rng, 1, 2);
            int c0 = rng_range(rng, rm->x0 + 3, rm->x1 - 3 - iw), r0 = rng_range(rng, rm->y0 + 3, rm->y1 - 4 - ih);
            if (c0 < rm->x0 + 3 || r0 < rm->y0 + 3) continue;
            fill(w, c0, r0, c0 + iw - 1, r0 + ih - 1, MT_ROCK);
        }
    }
    /* coin blocks in the walls */
    int coins = 6 + rng_range(rng, 0, 3), tries = 0;
    while (coins > 0 && tries++ < 4000) {
        int c = rng_range(rng, 2, w->w - 3), r = rng_range(rng, 2, w->h - 3);
        if (!is_rock(w, c, r)) continue;
        bool touch = is_air(w, c - 1, r) || is_air(w, c + 1, r) || is_air(w, c, r + 1);
        if (!touch || is_air(w, c, r - 1)) continue;
        w->tile[r][c] = MT_COIN;
        coins--;
    }
}

static int pick_kind(Rng *rng, int region) {
    int tw = 0;
    for (int k = 0; k < 6; k++) tw += MAP_FOES[region][k].weight;
    int roll = rng_range(rng, 0, tw - 1);
    for (int k = 0; k < 6; k++) {
        if (roll < MAP_FOES[region][k].weight) return MAP_FOES[region][k].kind;
        roll -= MAP_FOES[region][k].weight;
    }
    return EK_FLITTER;
}

/* a place on a floor (or a ceiling, or a wall) of the map for kind k */
static bool place_on_surface(ClcWorld *w, Rng *rng, int hab, int *x, int *y, int *dir, int avoid_cell) {
    for (int tries = 0; tries < 400; tries++) {
        int c = rng_range(rng, 2, w->w - 3), r = rng_range(rng, 2, w->h - 3);
        if ((r / CLC_CH) * CLC_GW + c / CLC_CW == avoid_cell) continue;
        if (!is_air(w, c, r)) continue;
        if (hab == 1 || hab == 2) {
            bool floor = is_rock(w, c, r + 1) && is_air(w, c, r - 1) && is_air(w, c - 1, r) && is_air(w, c + 1, r);
            bool ceil = hab == 2 && is_rock(w, c, r - 1) && is_air(w, c, r + 1) && is_air(w, c - 1, r) && is_air(w, c + 1, r);
            if (!floor && !ceil) continue;
            *x = c * CLC_T + 4;
            *y = floor ? r * CLC_T + 4 : r * CLC_T + 4;
            *dir = floor ? 1 : -1;
            if (near_thing(w, *x, *y, 24)) continue;
            return true;
        }
        if (hab == 3) {
            bool lw = is_rock(w, c - 1, r) && is_air(w, c + 1, r) && is_air(w, c + 2, r);
            bool rw = is_rock(w, c + 1, r) && is_air(w, c - 1, r) && is_air(w, c - 2, r);
            if (!lw && !rw) continue;
            if (!is_air(w, c, r - 1) || !is_air(w, c, r + 1)) continue;
            *x = c * CLC_T + 4;
            *y = r * CLC_T + 4;
            *dir = lw ? 1 : -1;
            if (near_thing(w, *x, *y, 24)) continue;
            return true;
        }
    }
    return false;
}

static bool place_in_air(ClcWorld *w, Rng *rng, int *x, int *y, int avoid_cell, int cell_only, int spacing) {
    for (int tries = 0; tries < 600; tries++) {
        int c, r;
        if (cell_only >= 0) {
            c = (cell_only % CLC_GW) * CLC_CW + rng_range(rng, 1, CLC_CW - 2);
            r = (cell_only / CLC_GW) * CLC_CH + rng_range(rng, 1, CLC_CH - 2);
        } else {
            c = rng_range(rng, 2, w->w - 3);
            r = rng_range(rng, 2, w->h - 3);
        }
        if ((r / CLC_CH) * CLC_GW + c / CLC_CW == avoid_cell) continue;
        if (!open_spot(w, c, r)) continue;
        *x = c * CLC_T + CLC_T;
        *y = r * CLC_T + CLC_T;
        if (near_thing(w, *x, *y, spacing)) continue;
        return true;
    }
    return false;
}

static void add_foe(ClcWorld *w, int k, int x, int y, int dir, Rng *rng) {
    int i = clc_ent_add(w, k, x, y);
    if (i < 0) return;
    ClcEnt *e = &w->e[i];
    e->dir = (int8_t)dir;
    e->t = (uint16_t)rng_range(rng, 0, 239);
    if (k == EK_SNAKE || k == EK_WORM) {
        /* a long body: segments that follow the head */
        int lead = i;
        for (int s = 0; s < (k == EK_SNAKE ? 6 : 5); s++) {
            int j = clc_ent_add(w, EK_SEG, x, y);
            if (j < 0) break;
            w->e[j].link = (uint8_t)lead;
            w->e[j].a = (int16_t)i;
            w->e[j].flag = (uint8_t)k;
            lead = j;
        }
    }
}

static bool try_world(ClcWorld *w, const ClcAreaSpec *sp, Rng *rng) {
    static Room rooms[CLC_GW * CLC_GH];
    static Strip strips[MAX_STRIPS];
    static Plan plan[CLC_DOORS];
    int start_cell;
    uint32_t keep_ver = w->ver;
    memset(w, 0, sizeof *w);
    w->ver = (uint16_t)(keep_ver + 1);
    w->region = sp->region;
    w->area = sp->area;
    w->kind = WK_GEN;
    build_map(w, sp, rng, rooms, &start_cell);
    int ns = find_strips(w, strips);
    if (ns < 6) return false;

    /* the start: a floor in the start cell (or the nearest one with a floor) */
    int best = -1, bestd = 99;
    for (int i = 0; i < ns; i++) {
        int cx = strips[i].cell % CLC_GW, cy = strips[i].cell / CLC_GW;
        int d = iabs(cx - start_cell % CLC_GW) + iabs(cy - start_cell / CLC_GW);
        if (strips[i].c1 - strips[i].c0 < 8) continue;
        if (d < bestd || (d == bestd && rng_chance(rng, 50))) { bestd = d; best = i; }
    }
    if (best < 0) return false;
    Strip *s0 = &strips[best];
    s0->used = 1;
    start_cell = s0->cell;
    w->spawn_c = (int16_t)(s0->c0 + 2 + rng_range(rng, 0, imax(0, s0->c1 - s0->c0 - 8)));
    w->spawn_r = s0->r;

    /* the doors: each on a floor of its own, the gold one far from the start */
    int nplan = plan_doors(sp, rng, plan);
    ship_bfs(w, w->spawn_c, w->spawn_r);
    for (int k = 0; k < nplan; k++) {
        bool gold = plan[k].type == DT_GOLD;
        int pick = -1, pick_score = -1;
        for (int tries = 0; tries < 60; tries++) {
            int i = rng_range(rng, 0, ns - 1);
            Strip *st = &strips[i];
            if (st->used) continue;
            if (st->cell == start_cell && tries < 50) continue;
            int d = reach_at(st->c0 * CLC_T + 8, st->r * CLC_T);
            if (d == 0xFFFF) continue;
            /* the gold door: somewhere well away from the start, not always the furthest */
            int score = gold ? imin(d, 260) * 4 + rng_range(rng, 0, 400) : rng_range(rng, 0, 100);
            if (score > pick_score) { pick_score = score; pick = i; }
        }
        if (pick < 0) return false;
        Strip *st = &strips[pick];
        st->used = 1;
        /* also keep the strips next to this one free */
        for (int i = 0; i < ns; i++)
            if (strips[i].r == st->r && iabs(strips[i].c0 - st->c1) < 3) strips[i].used = 1;
        ClcDoor *d = &w->door[w->nd++];
        memset(d, 0, sizeof *d);
        d->type = plan[k].type;
        d->room = plan[k].room;
        d->item = -1;
        d->r = st->r;
        bool door_left = rng_chance(rng, 50);
        int len = st->c1 - st->c0 + 1;
        /* the landing spot: three tiles from the door, a short walk */
        if (door_left) {
            d->c = (int16_t)(st->c0 + 1);
            d->pad_c = (int16_t)(st->c0 + 1 + imin(3, len - 3));
        } else {
            d->c = (int16_t)(st->c1 - 1);
            d->pad_c = (int16_t)(st->c1 - 1 - imin(3, len - 3));
        }
        d->hidden = gold;
        d->seed = (uint8_t)rng_range(rng, 0, 255);
        if (gold) w->gold_door = (uint8_t)(w->nd - 1);
    }

    /* the notes: two near the start, the rest spread over the cells */
    int nn = 0;
    int cells[CLC_GW * CLC_GH], ncells = 0;
    for (int i = 0; i < CLC_GW * CLC_GH; i++)
        if (i != start_cell) cells[ncells++] = i;
    for (int i = ncells - 1; i > 0; i--) {
        int j = rng_range(rng, 0, i), t = cells[i];
        cells[i] = cells[j];
        cells[j] = t;
    }
    int gold_cell = (w->door[w->gold_door].r / CLC_CH) * CLC_GW + w->door[w->gold_door].c / CLC_CW;
    for (int k = 0; k < 4; k++) {
        int x, y;
        /* two near the start, and two near the gold door for the dash */
        int sc = k < 2 ? start_cell : gold_cell;
        int cand[5] = {sc, sc - 1, sc + 1, sc - CLC_GW, sc + CLC_GW};
        bool done = false;
        for (int j = 0; j < 5 && !done; j++) {
            int c = cand[(j + k) % 5];
            if (c < 0 || c >= CLC_GW * CLC_GH) continue;
            if (place_in_air(w, rng, &x, &y, -1, c, 24) && reach_at(x, y) < 0xFFFF) {
                clc_ent_add(w, EK_NOTE, x, y);
                nn++;
                done = true;
            }
        }
    }
    for (int k = 0; nn < CLC_NOTES_PLACED && k < ncells * 2; k++) {
        int x, y;
        if (place_in_air(w, rng, &x, &y, -1, cells[k % ncells], 32) && reach_at(x, y) < 0xFFFF) {
            clc_ent_add(w, EK_NOTE, x, y);
            nn++;
        }
    }
    if (nn < CLC_NOTES_PLACED) return false;

    /* fake notes in the Cloister: ghosts waiting to wake */
    if (sp->region == RG_CLOISTER)
        for (int k = 0; k < 3 + sp->area; k++) {
            int x, y;
            if (place_in_air(w, rng, &x, &y, start_cell, -1, 32)) clc_ent_add(w, EK_FAKENOTE, x, y);
        }

    /* the enemies */
    int tier = clc_region_tier(sp->region);
    int foes = 11 + sp->area * 3 + tier * 2;
    for (int k = 0; k < foes; k++) {
        int kind = pick_kind(rng, sp->region), x, y, dir = 1;
        int hab = habitat(kind);
        bool ok = hab == 0 ? place_in_air(w, rng, &x, &y, start_cell, -1, 40) : place_on_surface(w, rng, hab, &x, &y, &dir, start_cell);
        if (ok) add_foe(w, kind, x, y, dir, rng);
    }
    /* pods that hatch when the dash starts */
    for (int k = 0; k < 4 + sp->area + tier; k++) {
        int x, y, dir;
        if (place_on_surface(w, rng, 2, &x, &y, &dir, start_cell)) {
            int i = clc_ent_add(w, EK_POD, x, y);
            if (i >= 0) w->e[i].dir = (int8_t)dir;
        }
    }
    /* the Spire's magnets */
    if (sp->region == RG_SPIRE)
        for (int k = 0, tries = 0; k < 4 && tries < 2000; tries++) {
            int c = rng_range(rng, 2, w->w - 3), r = rng_range(rng, 2, w->h - 3);
            if (!is_rock(w, c, r) || (r / CLC_CH) * CLC_GW + c / CLC_CW == start_cell) continue;
            bool face = (is_air(w, c + 1, r) && is_air(w, c + 2, r) && is_air(w, c + 3, r)) ||
                        (is_air(w, c - 1, r) && is_air(w, c - 2, r) && is_air(w, c - 3, r));
            if (!face || is_air(w, c, r - 1)) continue;
            w->tile[r][c] = (uint8_t)(k & 1 ? MT_MAGNET_PUSH : MT_MAGNET_PULL);
            k++;
        }
    /* clock rings: in the gaps near the walls */
    for (int tries = 0; w->nrings < 8 && tries < 3000; tries++) {
        int c = rng_range(rng, 2, w->w - 3), r = rng_range(rng, 2, w->h - 3);
        if (!open_spot(w, c, r)) continue;
        bool wall = false;
        for (int dy = -2; dy <= 3 && !wall; dy++)
            for (int dx = -2; dx <= 3 && !wall; dx++) wall = is_rock(w, c + dx, r + dy);
        if (!wall) continue;
        bool far = true;
        for (int k = 0; k < w->nrings; k++)
            if (iabs(w->ring_c[k] - c) + iabs(w->ring_r[k] - r) < 12) far = false;
        if (!far) continue;
        w->ring_c[w->nrings] = (int16_t)c;
        w->ring_r[w->nrings] = (int16_t)r;
        w->nrings++;
    }
    w->dark = sp->region == RG_CLOISTER;
    return clc_check_world(w, NULL, 0) == 0;
}

int clc_check_world(const ClcWorld *w, char *why, int n) {
    ship_bfs(w, w->spawn_c, w->spawn_r);
    int bad = 0;
    if (g_dist[w->spawn_r][w->spawn_c] == 0xFFFF) {
        if (why) snprintf(why, (size_t)n, "start blocked");
        return 1;
    }
    int nearest = 0xFFFF, notes = 0;
    for (int i = 0; i < w->ne; i++) {
        const ClcEnt *e = &w->e[i];
        if (!e->on || e->kind != EK_NOTE) continue;
        int d = reach_at((int)(e->x >> 8), (int)(e->y >> 8));
        if (d == 0xFFFF) {
            bad++;
            if (why) snprintf(why, (size_t)n, "note at %d,%d unreachable", (int)(e->x >> 8), (int)(e->y >> 8));
        } else notes++;
        if (d < nearest) nearest = d;
    }
    if (w->kind == WK_GEN && notes < CLC_NOTES_PLACED) {
        bad++;
        if (why) snprintf(why, (size_t)n, "only %d notes", notes);
    }
    for (int k = 0; k < w->nd; k++) {
        const ClcDoor *d = &w->door[k];
        int x, y;
        clc_door_pad(d, &x, &y);
        int c = x / CLC_T, r = y / CLC_T;
        if (g_dist[r][c] == 0xFFFF) c++;
        bool floor_ok = d->r + 1 < w->h && clc_map_solid_tile(w->tile[d->r + 1][d->pad_c]) &&
                        clc_map_solid_tile(w->tile[d->r + 1][d->c]);
        /* the walk from the pad to the door: a level floor, nothing in the way */
        int a = imin(d->c, d->pad_c), b = imax(d->c, d->pad_c);
        for (int cc = a; cc <= b && floor_ok; cc++)
            floor_ok = clc_map_solid_tile(w->tile[d->r + 1][cc]) && !clc_map_solid_tile(w->tile[d->r][cc]) &&
                       w->tile[d->r][cc] != MT_LADDER;
        if (g_dist[r][c] == 0xFFFF || !floor_ok) {
            bad++;
            if (why) snprintf(why, (size_t)n, "door %d (room %d) unreachable", k, d->room);
        }
    }
    if (w->kind == WK_GEN && nearest > 60) {
        bad++;
        if (why) snprintf(why, (size_t)n, "nearest note %d cells away", nearest);
    }
    return bad;
}

void clc_gen_world(ClcWorld *w, const ClcAreaSpec *sp) {
    static Rng rng;
    for (int attempt = 0; attempt < 64; attempt++) {
        rng_seed(&rng, (uint64_t)sp->seed * 2654435761u + (uint64_t)attempt * 7919u + 1u);
        if (try_world(w, sp, &rng)) break;
    }
    rng_seed(&w->rng, (uint64_t)sp->seed ^ 0x5eedu);
    w->enter = -1;
}

/* ---- the fixed maps --------------------------------------------------------- */

static void add_door(ClcWorld *w, int type, int room, int c, int r, int pad_c, bool hidden) {
    ClcDoor *d = &w->door[w->nd++];
    memset(d, 0, sizeof *d);
    d->type = (uint8_t)type;
    d->room = (uint8_t)room;
    d->item = -1;
    d->c = (int16_t)c;
    d->r = (int16_t)r;
    d->pad_c = (int16_t)pad_c;
    d->hidden = (uint8_t)hidden;
}

/* The Crown: the Spire's last area, always the same. A long hall climbing
 * to the right in three terraces: the ship sets down on the lowest; the
 * sexton's secret door (when it shows) waits on that same floor, so the
 * secret way never passes the health stall and the store on the middle
 * terrace; the gold door to Lady Hush stands at the top. */
void clc_gen_crown(ClcWorld *w, bool secret) {
    uint16_t v = w->ver;
    memset(w, 0, sizeof *w);
    w->ver = (uint16_t)(v + 1);
    w->region = RG_SPIRE;
    w->area = 2;
    w->kind = WK_CROWN;
    w->w = 64;
    w->h = 34;
    fill(w, 0, 0, CLC_MW - 1, CLC_MH - 1, MT_ROCK);
    carve(w, 2, 3, 61, 30);              /* the hall */
    fill(w, 22, 26, 62, 33, MT_ROCK);    /* the middle terrace: its floor is row 26 */
    fill(w, 47, 14, 62, 33, MT_ROCK);    /* the top terrace: its floor is row 14 */
    fill(w, 2, 3, 12, 14, MT_ROCK);      /* the overhang above the start */
    fill(w, 27, 3, 31, 12, MT_ROCK);     /* a hanging pillar */
    fill(w, 38, 18, 41, 19, MT_ROCK);    /* a floating rock */
    fill(w, 15, 17, 18, 18, MT_ROCK2);
    carve(w, 2, 15, 12, 30);
    for (int r = 0; r < w->h; r++)
        for (int c = 0; c < w->w; c++)
            if (w->tile[r][c] == MT_ROCK && ((c * 5 + r * 3) % 13) == 0) w->tile[r][c] = MT_ROCK2;
    w->spawn_c = 6;
    w->spawn_r = 30;
    add_door(w, DT_SECRET, RM_TOCK, 17, 30, 13, !secret);
    add_door(w, DT_BLUE, RM_HEALTH, 26, 25, 30, false);
    add_door(w, DT_GREEN, RM_SHOP, 40, 25, 36, false);
    add_door(w, DT_GOLD, RM_HUSH, 59, 13, 54, false);
    w->gold_door = 3;
    static const int16_t FOES[][3] = {
        {EK_FLITTER, 20, 12}, {EK_FLITTER, 36, 10}, {EK_EYE, 46, 20}, {EK_CHASER, 44, 7}, {EK_FLITTER, 56, 8},
    };
    for (int k = 0; k < ARRAY_LEN(FOES); k++) {
        int i = clc_ent_add(w, FOES[k][0], FOES[k][1] * CLC_T + 4, FOES[k][2] * CLC_T + 4);
        if (i >= 0) w->e[i].dir = -1;
    }
    w->notes = CLC_NOTES_NEEDED; /* no notes to find here: the doors are open */
    rng_seed(&w->rng, 35);
    w->enter = -1;
}

/* The escape shaft: up through the gates the boss held shut, now open,
 * past fuel and arrows, before the clock runs out. */
void clc_gen_escape(ClcWorld *w) {
    uint16_t v = w->ver;
    memset(w, 0, sizeof *w);
    w->ver = (uint16_t)(v + 1);
    w->region = RG_SPIRE;
    w->area = 3;
    w->kind = WK_ESCAPE;
    w->w = 40;
    w->h = 62;
    fill(w, 0, 0, CLC_MW - 1, CLC_MH - 1, MT_ROCK);
    carve(w, 6, 0, 33, 59);
    for (int r = 0; r < 2; r++)
        for (int c = 6; c <= 33; c++) w->tile[r][c] = MT_AIR;
    /* five gate floors, each open on alternate sides */
    static const int8_t GATE_ROW[5] = {50, 40, 30, 20, 10};
    for (int k = 0; k < 5; k++) {
        int r = GATE_ROW[k];
        fill(w, 6, r, 33, r + 1, MT_ROCK);
        int g0 = (k & 1) ? 8 : 25;
        for (int c = g0; c < g0 + 6; c++) {
            w->tile[r][c] = MT_GATE_OPEN;
            w->tile[r + 1][c] = MT_GATE_OPEN;
        }
        clc_ent_add(w, EK_FUELCAN, (g0 + 3) * CLC_T, (r + 5) * CLC_T);
        int a = clc_ent_add(w, EK_ARROW, (g0 + 3) * CLC_T, (r + 3) * CLC_T);
        if (a >= 0) w->e[a].dir = -1; /* points up */
    }
    for (int r = 0; r < w->h; r++)
        for (int c = 0; c < w->w; c++)
            if (w->tile[r][c] == MT_ROCK && ((c * 3 + r * 7) % 11) == 0) w->tile[r][c] = MT_ROCK2;
    w->spawn_c = 14;
    w->spawn_r = 59;
    w->exit_y = 8;
    w->notes = CLC_NOTES_NEEDED;
    rng_seed(&w->rng, 49);
    w->enter = -1;
}

/* ---- caves and rooms ---------------------------------------------------------
 *
 * Cave pieces: 16 x 10 tiles of 16 px. '#' rock, '.' air, '-' a thin floor
 * (down + jump drops through), 'B' a block with coins, 'X' a barrel, '^'
 * spikes, 'e' a slot for the region's walker, 'f' for its flier, 'c' for
 * its ceiling thing, 'h' for its hazard. Every piece can be crossed left to
 * right on foot. */

static const char *const PIECES[][CLC_SH] = {
    {"################", "################", "................", "................", "................",
     "..........B.....", "....e......f....", "................", "................", "################"},
    {"################", "######.....#####", "................", "........c.......", "................",
     ".......f........", "................", "....###....e....", "....###.........", "################"},
    {"################", "################", "....c...........", "................", "..........----..",
     "................", "...----.....f...", "................", "..e.....B.......", "################"},
    {"################", "##########......", "................", "..........e.....", ".........####...",
     "....f...#####...", "........######..", ".....e.#######..", "......########..", "################"},
    {"################", "......##########", "................", "....c...........", "................",
     ".......BB....f..", "................", "...e.......X....", "..###...........", "################"},
    {"################", "################", "................", "......f.........", "..----....----..",
     "................", "................", "..e......h......", "................", "#######^^#######"},
    {"################", "#####......#####", "................", "........f.......", "................",
     "...###....###...", "...###.e..###...", "................", "......h.........", "################"},
    {"################", "################", "..c.......c.....", "................", "................",
     ".....X.........e", "................", "...####...####..", "...####.e.####..", "################"},
    {"################", "################", ".........f......", "................", "................",
     "................", "....BBB.........", "...........e....", "..h........###..", "################"},
    {"################", "#######.......##", "................", "................", ".....----.......",
     "..f.............", ".........----...", "...e........e...", "######..########", "######^^########"},
    {"################", "################", "................", "...f.......c....", "................",
     "................", "......h.........", "...e.......X....", "#####..#########", "#####^^#########"},
    {"################", "################", "................", "......e.........", "....######......",
     "....######..f...", "................", "..........----..", "....e...........", "################"},
    {"################", "##..############", "................", ".....c..........", "................",
     "..........f.....", "......B.........", "....h.....e.....", "...........###..", "################"},
    {"################", "################", "................", "................", "..f.........c...",
     "................", "................", "....####...e....", "..X.####........", "################"},
    {"################", "########....####", "................", "..........h.....", "................",
     ".....----.......", "..f.........f...", "................", "....e...e.......", "################"},
    {"################", "################", "...c............", "................", "................",
     "......e....###..", "...#####...###..", "...#####........", ".......h........", "################"},
};
#define NPIECES ARRAY_LEN(PIECES)

/* every piece sound: the right size, closed at the bottom (no pit without
 * a floor), open at both sides so one runs into the next */
int clc_pieces_bad(void) {
    int bad = 0;
    for (int i = 0; i < (int)NPIECES; i++) {
        bool ok = true;
        for (int r = 0; r < CLC_SH; r++) ok = ok && PIECES[i][r] && strlen(PIECES[i][r]) == 16;
        if (!ok) { bad++; continue; }
        for (int c = 0; c < 16; c++) ok = ok && (PIECES[i][CLC_SH - 1][c] == '#' || PIECES[i][CLC_SH - 1][c] == '^');
        bool left = false, right = false;
        for (int r = 2; r < CLC_SH - 1; r++) {
            left = left || PIECES[i][r][0] != '#';
            right = right || PIECES[i][r][15] != '#';
        }
        if (!ok || !left || !right) bad++;
    }
    return bad;
}

/* the first piece (the door in) and the last ones */
static const char *const START_PIECE[CLC_SH] = {
    "##########", "##########", "..........", "..........", "..........", "..........", "..........",
    "..........", "..........", "##########"};
static const char *const END_CHEST[CLC_SH] = {
    "################", "################", "................", "................", "................",
    "................", "................", "................", "................", "################"};
static const char *const ARENA[CLC_SH] = {
    "####################", "####################", "....................", "....................",
    "....................", "....................", "....................", "...--..........--...",
    "....................", "####################"};

/* behind-door enemies by region: the walker, the flier, the ceiling thing,
 * the hazard */
static const uint8_t CAVE_FOES[RG_COUNT][4] = {
    {EK_STINGER, EK_WISPNEST, EK_DROPPER, EK_FLAME},
    {EK_LOUSE, EK_LOUSE, EK_LOUSE, EK_FLAME},
    {EK_BRUTE, EK_WISPNEST, EK_DROPPER, EK_JET},
    {EK_PELTER, EK_RINGWORM, EK_DROPPER, EK_SLURP},
    {EK_TROOPER, EK_HOVERBOT, EK_DROPPER, EK_JET},
    {EK_SPITTER, EK_HOVERBOT, EK_COCOON, EK_WHEEL},
    {EK_TROOPER, EK_HOVERBOT, EK_COCOON, EK_WHEEL},
};
/* the Spire mixes in the others' */
static int cave_foe(Rng *rng, int region, int slot) {
    if (region == RG_SPIRE && rng_chance(rng, 50)) return CAVE_FOES[rng_range(rng, 0, RG_SPIRE - 1)][slot];
    return CAVE_FOES[region][slot];
}

/* the chest's item in each area's red cave (by region, area): -1 for a
 * draw from what's left */
static const int8_t CHEST[RG_COUNT][2] = {
    {G_PURSE, IT_KEY},
    {G_FEATHER, IT_SACK},
    {G_THIMBLE, -1},
    {G_MAGNET, G_SEEKER},
    {-1, G_BIGBANG},
    {G_SPIT, G_BOUNCE},
    {G_SPIT, G_FEATHER},
};

int clc_chest_item(int region, int area) {
    if (region < 0 || region >= RG_COUNT || area < 0 || area > 1) return -1;
    return CHEST[region][area];
}

static void sub_set(ClcSub *s, int c, int r, int t) {
    if (c >= 0 && r >= 0 && c < CLC_SW && r < CLC_SH) s->tile[r][c] = (uint8_t)t;
}

static void put_piece(ClcSub *s, const char *const *rows, int x0, int width, Rng *rng) {
    for (int r = 0; r < CLC_SH; r++)
        for (int c = 0; c < width; c++) {
            char ch = rows[r][c];
            int tc = x0 + c;
            int px = tc * CLC_ST + CLC_ST / 2, py = r * CLC_ST + CLC_ST;
            int t = ST_AIR;
            switch (ch) {
            case '#': t = ((tc * 7 + r * 3) % 9) ? ST_WALL : ST_WALL2; break;
            case '-': t = ST_THIN; break;
            case 'B': t = ST_BLOCK; break;
            case '^': t = ST_SPIKE; break;
            default: break;
            }
            /* the Icehouse: every floor's top is ice */
            if (s->ice && (t == ST_WALL || t == ST_WALL2) && r > 1 && rows[r - 1][c] != '#') t = ST_ICE;
            sub_set(s, tc, r, t);
            int k = -1;
            switch (ch) {
            case 'X': k = EK_BARREL; break;
            case 'e': k = cave_foe(rng, s->region, 0); break;
            case 'f': k = cave_foe(rng, s->region, 1); break;
            case 'c': k = cave_foe(rng, s->region, 2); break;
            case 'h': k = cave_foe(rng, s->region, 3); break;
            default: break;
            }
            if (k < 0) continue;
            if (k == EK_SLURP) {
                /* a slurp waits in an acid pit cut into the floor */
                sub_set(s, tc, CLC_SH - 1, ST_ACID);
                sub_set(s, tc + 1, CLC_SH - 1, ST_ACID);
                py = (CLC_SH - 1) * CLC_ST + 4;
            }
            if (k == EK_DROPPER || k == EK_COCOON) {
                /* it hangs from the ceiling above the slot */
                int rr = r;
                while (rr > 0 && rows[rr - 1][c] != '#') rr--;
                py = rr * CLC_ST + 8;
            }
            if (k == EK_FLAME || k == EK_JET || k == EK_SPITTER || k == EK_WHEEL) {
                /* standing on the floor below the slot */
                int rr = r;
                while (rr < CLC_SH - 1 && rows[rr + 1][c] != '#' && rows[rr + 1][c] != '^') rr++;
                py = (rr + 1) * CLC_ST;
                if (k == EK_WHEEL) py -= 44; /* high enough to walk under, not to jump through */
                /* hazards only where the floor runs flat for a few tiles either side
                 * (never over a pit or a step she has to jump) */
                bool flat = rr + 1 < CLC_SH;
                for (int dc = -3; dc <= 3 && flat; dc++) {
                    int cc = c + dc;
                    if (cc < 0 || cc >= width) continue;
                    flat = rows[rr + 1][cc] == '#' && (rr < 1 || rows[rr][cc] != '#');
                }
                if (!flat) {
                    if (k == EK_WHEEL || k == EK_JET) continue;
                }
            }
            int i = clc_sub_ent_add(s, k, px, py);
            if (i >= 0) s->e[i].t = (uint16_t)rng_range(rng, 0, 120);
        }
}

static void room_box(ClcSub *s, int w) {
    s->w = (uint8_t)w;
    for (int r = 0; r < CLC_SH; r++)
        for (int c = 0; c < w; c++) {
            bool edge = r <= 1 || r == CLC_SH - 1 || c == 0 || c == w - 1;
            s->tile[r][c] = (uint8_t)(edge ? (((c * 7 + r * 3) % 9) ? ST_WALL : ST_WALL2) : ST_AIR);
        }
}

/* the shop's stock: up to n items from what is still to be had */
static int stock(Rng *rng, uint32_t owned, int n, bool health, int8_t *out) {
    if (health) {
        out[0] = IT_TOFFEE;
        out[1] = IT_HEARTPIN;
        return 2;
    }
    int pool[IT_COUNT], np = 0, cons[6] = {IT_TOFFEE, IT_FLASK, IT_DRUM, IT_HEARTPIN, IT_SPARETANK, IT_FLASK};
    for (int g = 0; g < G_COUNT; g++)
        if (!(owned >> g & 1u)) pool[np++] = g;
    for (int i = np - 1; i > 0; i--) {
        int j = rng_range(rng, 0, i), t = pool[i];
        pool[i] = pool[j];
        pool[j] = t;
    }
    int ng = imin(np, n == 9 ? 5 : 2), k = 0;
    for (int i = 0; i < ng; i++) out[k++] = (int8_t)pool[i];
    /* the rest: fuel first, then whatever the bar needs */
    int order[6] = {1, 0, 2, 3, 4, 5};
    if (n != 9) {
        order[0] = 1;
        order[1] = rng_chance(rng, 50) ? 0 : 3;
        order[2] = rng_chance(rng, 50) ? 2 : 4;
    }
    for (int i = 0; k < n && i < 6; i++) out[k++] = (int8_t)cons[order[i]];
    return k;
}

void clc_gen_sub(ClcSub *s, const ClcSubSpec *sp) {
    static Rng rng;
    uint16_t v = s->ver;
    memset(s, 0, sizeof *s);
    s->ver = (uint16_t)(v + 1);
    s->room = sp->room;
    s->region = sp->region;
    s->area = sp->area;
    s->door_c = -1;
    s->exit_c = -1;
    s->got_item = -1;
    s->line = sp->hint;
    s->ice = sp->region == RG_ICE;
    rng_seed(&rng, (uint64_t)sp->seed * 40503u + 77u);
    rng_seed(&s->rng, (uint64_t)sp->seed + 3u);
    int fl = (CLC_SH - 1) * CLC_ST; /* the floor's top */
    switch (sp->room) {
    case RM_CAVE:
    case RM_GOLDCAVE: {
        /* the way in, five or six pieces, then the end */
        put_piece(s, START_PIECE, 0, 10, &rng);
        int x = 10, order[NPIECES];
        for (int i = 0; i < (int)NPIECES; i++) order[i] = i;
        for (int i = (int)NPIECES - 1; i > 0; i--) {
            int j = rng_range(&rng, 0, i), t = order[i];
            order[i] = order[j];
            order[j] = t;
        }
        int npieces = 5 + clc_region_tier(sp->region) / 2;
        for (int k = 0; k < npieces; k++, x += 16) put_piece(s, PIECES[order[k]], x, 16, &rng);
        s->door_c = 2;
        if (sp->lobber || sp->engine) {
            /* the arena: the Lobber (most regions) before a Hush Engine */
            put_piece(s, ARENA, x, 20, &rng);
            s->arena = 1;
            s->arena_c = (int16_t)x;
            if (sp->lobber) {
                int i = clc_sub_ent_add(s, EK_LOBBER, (x + 14) * CLC_ST, fl);
                (void)i;
            }
            clc_sub_ent_add(s, EK_ENGINE, (x + 18) * CLC_ST, fl);
            s->exit_c = (int16_t)(x + 17);
            s->boss_kind = sp->lobber ? 1 : 0;
            x += 20;
        } else {
            put_piece(s, END_CHEST, x, 16, &rng);
            if (sp->room == RM_CAVE) {
                int i = clc_sub_ent_add(s, EK_CHEST, (x + 8) * CLC_ST, fl);
                if (i >= 0) s->e[i].a = sp->item;
            } else {
                /* every gold cave ends with fuel */
                int i = clc_sub_ent_add(s, EK_ITEM, (x + 8) * CLC_ST, fl - 20);
                if (i >= 0) { s->e[i].a = IT_FLASK; s->e[i].b = 0; }
            }
            s->exit_c = (int16_t)(x + 13);
            s->exit_open = 1;
            x += 16;
        }
        s->w = (uint8_t)x;
        /* the walls close the ends */
        for (int r = 0; r < CLC_SH; r++) {
            sub_set(s, 0, r, ST_WALL);
            sub_set(s, s->w - 1, r, ST_WALL);
        }
        /* the skull starts off to the left */
        clc_sub_ent_add(s, EK_SKULL, -40, 5 * CLC_ST);
        break;
    }
    case RM_HUSH:
    case RM_TOCK: {
        room_box(s, 20);
        s->door_c = 2;
        s->arena = 1;
        s->arena_c = 0;
        if (sp->room == RM_HUSH) {
            sub_set(s, 4, 6, ST_THIN);
            sub_set(s, 5, 6, ST_THIN);
            sub_set(s, 14, 6, ST_THIN);
            sub_set(s, 15, 6, ST_THIN);
            clc_sub_ent_add(s, EK_HUSH, 160, 56);
            s->boss_kind = 2;
            s->boss_hp = s->boss_max = 100;
        } else {
            clc_sub_ent_add(s, EK_TOCK, 200, fl);
            s->boss_kind = 3;
            s->boss_hp = s->boss_max = 100;
        }
        s->exit_c = 17;
        break;
    }
    default: {
        /* a single room: back out through the door you came in by */
        room_box(s, 20);
        s->door_c = 2;
        int cx = 10 * CLC_ST;
        switch (sp->room) {
        case RM_NPC: {
            int i = clc_sub_ent_add(s, EK_NPC, cx + 24, fl);
            if (i >= 0) { s->e[i].a = sp->hint; s->e[i].b = sp->used; s->e[i].flag = (uint8_t)(sp->seed % 5 == 0); }
            break;
        }
        case RM_SAGE: {
            int i = clc_sub_ent_add(s, EK_NPC, cx + 24, fl);
            if (i >= 0) { s->e[i].a = sp->hint; s->e[i].b = sp->used; s->e[i].flag = 2; }
            break;
        }
        case RM_CURSED: {
            int i = clc_sub_ent_add(s, EK_NPC, cx + 24, fl);
            if (i >= 0) { s->e[i].a = -1; s->e[i].flag = 3; }
            s->chest_item = (uint8_t)(sp->item >= 0 ? sp->item : G_FAN);
            break;
        }
        case RM_TRIAL:
            for (int k = 0; k < 7; k++) {
                int i = clc_sub_ent_add(s, EK_WRIGGLER, 40 + k * 44, 50 + (k % 3) * 28);
                if (i >= 0) { s->e[i].dir = (int8_t)(k & 1 ? -1 : 1); s->e[i].t = (uint16_t)(k * 17); }
            }
            break;
        case RM_SHOP:
        case RM_HEALTH:
        case RM_MEGA:
        case RM_CHARMSHOP: {
            int8_t items[9];
            int n;
            if (sp->room == RM_CHARMSHOP) {
                items[0] = G_CHARM;
                n = 1 + stock(&rng, sp->owned | (1u << G_CHARM), 3, false, items + 1);
            } else {
                n = stock(&rng, sp->owned, sp->room == RM_MEGA ? 9 : 4, sp->room == RM_HEALTH, items);
            }
            s->mega = sp->room == RM_MEGA;
            if (s->mega) {
                for (int c = 3; c <= 16; c++) sub_set(s, c, 5, ST_THIN);
            }
            for (int k = 0; k < n; k++) {
                /* a row on the floor; the mega store adds a shelf above */
                int x, y;
                if (s->mega && k < 5) { x = 80 + k * 40; y = 5 * CLC_ST - 26; }
                else if (s->mega) { x = 96 + (k - 5) * 44; y = fl - 26; }
                else if (n <= 2) { x = 136 + k * 64; y = fl - 26; }
                else { x = 88 + k * 48; y = fl - 26; }
                int i = clc_sub_ent_add(s, EK_ITEM, x, y);
                if (i < 0) continue;
                int price = CLC_PRICE[items[k]];
                if (items[k] == G_CHARM && sp->room == RM_CHARMSHOP) price = sp->charm_price;
                if (s->mega) price = price + (price + 9) / 10;
                s->e[i].a = items[k];
                s->e[i].b = (int16_t)price;
            }
            /* the shopkeeper */
            int i = clc_sub_ent_add(s, EK_NPC, 17 * CLC_ST, fl);
            if (i >= 0) { s->e[i].flag = 4; s->e[i].a = -1; }
            break;
        }
        case RM_BOON: {
            /* pick one of three, free */
            int8_t items[4];
            int n = stock(&rng, sp->owned, 4, false, items);
            for (int k = 0; k < imin(3, n); k++) {
                int i = clc_sub_ent_add(s, EK_ITEM, 104 + k * 56, fl - 26);
                if (i >= 0) { s->e[i].a = items[k] < G_COUNT ? items[k] : IT_DRUM; s->e[i].b = 0; s->e[i].flag = 1; }
            }
            break;
        }
        case RM_ARMOR: {
            int i = clc_sub_ent_add(s, EK_ITEM, 168, fl - 26);
            if (i >= 0) { s->e[i].a = (sp->owned >> G_PLATE & 1u) ? IT_DRUM : G_PLATE; s->e[i].b = 0; }
            break;
        }
        case RM_LIGHTS:
        case RM_MAGNETS: {
            int i = clc_sub_ent_add(s, EK_SWITCH, 200, fl);
            if (i >= 0) s->e[i].a = sp->room == RM_LIGHTS ? 0 : 1;
            break;
        }
        default: break; /* an empty room */
        }
        break;
    }
    }
    s->cl.x = (int32_t)((s->door_c * CLC_ST + CLC_ST / 2) * 256);
    s->cl.y = (int32_t)(fl * 256);
    s->cl.face = 1;
    s->cl.ground = 1;
    s->cl.fall_from = s->cl.y;
    s->cl.inv = 30;
    if (s->door_c >= 0) sub_set(s, s->door_c, CLC_SH - 2, ST_DOOR);
}
