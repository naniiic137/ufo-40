/* BELLHOP - the demo pilot, which flies the tests' runs with real presses.
 *
 * Each stage carries a route for it (bellhop_stages.c): a list of steps,
 * each a place to reach or a thing to do there. The pilot walks a distance
 * field to the current step's place (built over the stage as it now is,
 * with blocks and glass counted as open, since the slash clears them), and
 * steers and thrusts to follow it, as a player taps and holds.
 *
 * Every third frame it plans: it copies the whole stage and flies the copy
 * 40 frames forward on the real rules (bellhop_stage.c) for each of ten
 * ways to start (its own steering, or holding a direction with or without
 * thrust, or tapping), then its own steering after that. A plan that
 * crashes is out; of the rest it takes the one that ends nearest along the
 * field, or that gets the step done. It slashes whenever an enemy, a
 * bubble, a block or glass is in reach on the side it faces.
 *
 * Route steps (separated by spaces):
 *   t        the hidden tea spot (done when the cup shows)
 *   c        the cup (done when it is taken)
 *   E        the exit ring
 *   x,y      the middle of tile (x, y) (done within 6 px)
 *   !x,y     slash the lever or bomb on tile (x, y) (done when it flips or lights)
 *   ~n       hover where it is for n frames
 *   W        a warp sparkle (only when told to take warps; else skipped)
 *   O        fly round the nearest circler until all four of its lights are lit
 *   $        touch the big coin, then take its five coins
 *   C        the crystal room: chase crystals until the exit opens
 *   B        the boss: go for whatever can be hit until it is beaten */
#include "bellhop.h"

#define NW 80
#define NH 42
#define CELL 4
#define FARV 65535
#define LOOK 40
#define LEAD 6
#define REPLAN 4
#define NCAND 10
#define NMUT 12
#define POLICY 0xFF
#define MAXTOK 32

int bhp_bot_debug;
int bhp_bot_rounds = 5; /* crystal rounds the pilot plays before it waits by the exit */

enum { TK_TEA, TK_CUP, TK_EXIT, TK_AT, TK_HIT, TK_WAIT, TK_WARP, TK_CRYSTALS, TK_BOSS, TK_CIRCLE, TK_COINS };
typedef struct Tok { uint8_t kind; int16_t x, y, n; } Tok;

static int PX(int32_t v) { return (int)(v >> 8); }

static int parse(const char *r, Tok *out) {
    int n = 0;
    while (r && *r && n < MAXTOK) {
        while (*r == ' ') r++;
        if (!*r) break;
        Tok t = {0, 0, 0, 0};
        if (*r == 't') { t.kind = TK_TEA; r++; }
        else if (*r == 'c') { t.kind = TK_CUP; r++; }
        else if (*r == 'E') { t.kind = TK_EXIT; r++; }
        else if (*r == 'W') { t.kind = TK_WARP; r++; }
        else if (*r == 'C') { t.kind = TK_CRYSTALS; r++; }
        else if (*r == 'B') { t.kind = TK_BOSS; r++; }
        else if (*r == 'O') { t.kind = TK_CIRCLE; r++; }
        else if (*r == '$') { t.kind = TK_COINS; r++; }
        else if (*r == '~') { t.kind = TK_WAIT; t.n = (int16_t)strtol(r + 1, (char **)&r, 10); }
        else if (*r == '!' || (*r >= '0' && *r <= '9')) {
            t.kind = *r == '!' ? TK_HIT : TK_AT;
            if (*r == '!') r++;
            char *e;
            t.x = (int16_t)strtol(r, &e, 10);
            r = e;
            if (*r == ',') r++;
            t.y = (int16_t)strtol(r, &e, 10);
            r = e;
        } else { r++; continue; }
        out[n++] = t;
    }
    return n;
}

int bhp_bot_route_len(int stage) {
    Tok t[MAXTOK];
    return parse(BHP_STAGE[iclamp(stage, 0, BHP_STAGES - 1)].route, t);
}

/* ---- the pilot's memory beyond BhpBot (one pilot at a time) ---------------- */

static Tok toks[MAXTOK];
static int ntok;
static uint8_t done_mark[MAXTOK]; /* lever flipped / bomb lit this life */
static uint16_t wait_t;
static uint32_t last_life;
static uint8_t nav[NH][NW], clr[NH][NW];
static uint16_t nav_ver = 0xFFFF;
static int nav_stage = -1;

/* ---- where a ship fits ------------------------------------------------------ */

static bool nav_solid_tile(int t) { return bhp_tile_solid(t) && t != BTL_BLOCK && t != BTL_GLASS; }

static bool static_blocker(const BhpStage *s, int x, int y) {
    for (int i = 0; i < s->ne; i++) {
        const BhpEnt *e = &s->e[i];
        if (!e->on) continue;
        int r;
        switch (e->kind) {
        case BEK_CIRCLER: r = 11; break;
        case BEK_BOMB: r = e->flag ? 30 : 10; break;
        case BEK_FIREBAR: r = 8; break;
        default: continue;
        }
        if (iabs(PX(e->x) - x) < r && iabs(PX(e->y) - y) < r) return true;
    }
    if (s->kind == BHK_BOSS && !s->boss.down) {
        if (s->boss.kind == 1) {
            int dx = x - BHP_MILL_X, dy = y - BHP_MILL_Y;
            if (dx * dx + dy * dy < 20 * 20) return true;
        }
        if (s->boss.kind == 3)
            for (int w = 0; w < 2; w++) {
                int dx = x - BHP_COG_X[w], dy = y - BHP_COG_Y;
                if (dx * dx + dy * dy < 40 * 40) return true;
            }
    }
    return false;
}

static void build_nav(const BhpStage *s) {
    for (int j = 0; j < NH; j++)
        for (int i = 0; i < NW; i++) {
            int cx = i * CELL + 2, cy = BHP_OY + j * CELL + 2;
            bool ok = true;
            for (int y = cy - 6; y <= cy + 5 && ok; y += 2)
                for (int x = cx - 6; x <= cx + 5 && ok; x += 2)
                    if (nav_solid_tile(bhp_tile_at(s, x, y))) ok = false;
            if (ok && (nav_solid_tile(bhp_tile_at(s, cx + 5, cy + 5)) || nav_solid_tile(bhp_tile_at(s, cx - 6, cy + 5)))) ok = false;
            if (ok && static_blocker(s, cx, cy)) ok = false;
            nav[j][i] = ok;
        }
    /* cells to the nearest place a ship doesn't fit, up to 4 */
    for (int j = 0; j < NH; j++)
        for (int i = 0; i < NW; i++) {
            int d = 0;
            if (nav[j][i])
                for (d = 1; d < 4; d++) {
                    bool hit = false;
                    for (int dj = -d; dj <= d && !hit; dj++)
                        for (int di = -d; di <= d && !hit; di++) {
                            int a = i + di, b = j + dj;
                            if (a < 0 || b < 0 || a >= NW || b >= NH || !nav[b][a]) hit = true;
                        }
                    if (hit) break;
                }
            clr[j][i] = (uint8_t)d;
        }
    nav_ver = s->ver;
    nav_stage = s->idx;
}

/* ---- the distance field (Dijkstra over the cells) ------------------------------ */

static uint16_t heap_c[NW * NH * 4];
static uint16_t heap_k[NW * NH * 4];
static int heap_n;

static void hpush(uint16_t cell, uint16_t key) {
    int i = heap_n++;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (heap_k[p] <= key) break;
        heap_c[i] = heap_c[p];
        heap_k[i] = heap_k[p];
        i = p;
    }
    heap_c[i] = cell;
    heap_k[i] = key;
}

static uint16_t hpop(uint16_t *key) {
    uint16_t top = heap_c[0];
    *key = heap_k[0];
    uint16_t c = heap_c[--heap_n], k = heap_k[heap_n];
    int i = 0;
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        uint16_t mk = k;
        if (l < heap_n && heap_k[l] < mk) { m = l; mk = heap_k[l]; }
        if (r < heap_n && heap_k[r] < mk) { m = r; mk = heap_k[r]; }
        if (m == i) break;
        heap_c[i] = heap_c[m];
        heap_k[i] = heap_k[m];
        i = m;
    }
    heap_c[i] = c;
    heap_k[i] = k;
    return top;
}

static void build_field(BhpBot *b, int tx, int ty) {
    for (int j = 0; j < NH; j++)
        for (int i = 0; i < NW; i++) b->field[j][i] = FARV;
    heap_n = 0;
    int ti = tx / CELL, tj = (ty - BHP_OY) / CELL;
    /* seed the cells round the target a ship fits in */
    for (int dj = -4; dj <= 4; dj++)
        for (int di = -4; di <= 4; di++) {
            int i = ti + di, j = tj + dj;
            if (i < 0 || j < 0 || i >= NW || j >= NH || !nav[j][i]) continue;
            int cx = i * CELL + 2, cy = BHP_OY + j * CELL + 2;
            int d = bhp_isqrt((cx - tx) * (cx - tx) + (cy - ty) * (cy - ty)) * 10 / CELL;
            if (d > 25 && !(di == 0 && dj == 0)) continue;
            if (d < b->field[j][i]) {
                b->field[j][i] = (uint16_t)d;
                hpush((uint16_t)(j * NW + i), (uint16_t)d);
            }
        }
    /* nothing a ship fits in that close: seed the nearest cells that do */
    if (heap_n == 0) {
        int bd = 1 << 30;
        for (int j = 0; j < NH; j++)
            for (int i = 0; i < NW; i++)
                if (nav[j][i]) bd = imin(bd, (i - ti) * (i - ti) + (j - tj) * (j - tj));
        for (int j = 0; j < NH && bd < (1 << 30); j++)
            for (int i = 0; i < NW; i++)
                if (nav[j][i] && (i - ti) * (i - ti) + (j - tj) * (j - tj) <= bd + 2) {
                    b->field[j][i] = 0;
                    hpush((uint16_t)(j * NW + i), 0);
                }
    }
    while (heap_n) {
        uint16_t k;
        uint16_t c = hpop(&k);
        int i = c % NW, j = c / NW;
        if (k > b->field[j][i]) continue;
        for (int dj = -1; dj <= 1; dj++)
            for (int di = -1; di <= 1; di++) {
                if (!di && !dj) continue;
                int a = i + di, bb = j + dj;
                if (a < 0 || bb < 0 || a >= NW || bb >= NH || !nav[bb][a]) continue;
                if (di && dj && (!nav[j][a] || !nav[bb][i])) continue;
                int w = (di && dj ? 14 : 10) + (4 - clr[bb][a]) * 6;
                int v = k + w;
                if (v < b->field[bb][a]) {
                    b->field[bb][a] = (uint16_t)v;
                    if (heap_n < NW * NH * 4) hpush((uint16_t)(bb * NW + a), (uint16_t)v);
                }
            }
    }
    b->tx = (int16_t)tx;
    b->ty = (int16_t)ty;
    b->ver = nav_ver;
}

static bool cell_of(int x, int y, int *i, int *j) {
    *i = x / CELL;
    *j = (y - BHP_OY) / CELL;
    return *i >= 0 && *j >= 0 && *i < NW && *j < NH;
}

/* the field at a ship's middle (a cell it doesn't fit in reads from its best neighbour) */
static int field_at(const BhpBot *b, int x, int y) {
    int i, j;
    if (!cell_of(x, y, &i, &j)) return FARV;
    if (nav[j][i]) return b->field[j][i];
    int best = FARV;
    for (int dj = -2; dj <= 2; dj++)
        for (int di = -2; di <= 2; di++) {
            int a = i + di, c = j + dj;
            if (a < 0 || c < 0 || a >= NW || c >= NH || !nav[c][a]) continue;
            best = imin(best, b->field[c][a] + 40);
        }
    return best;
}

/* ---- the step being flown ---------------------------------------------------------- */

static int nearest_ent(const BhpStage *s, int kind, int x, int y, bool (*ok)(const BhpEnt *)) {
    int best = -1, bd = 1 << 30;
    for (int i = 0; i < s->ne; i++) {
        const BhpEnt *e = &s->e[i];
        if (!e->on || e->kind != kind || (ok && !ok(e))) continue;
        int dx = PX(e->x) - x, dy = PX(e->y) - y, d = dx * dx + dy * dy;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

static bool lit_lamp(const BhpEnt *e) { return e->flag == 1; }
static bool blue_spike(const BhpEnt *e) { return e->flag == 1; }

/* where the pilot wants to be for a moving target: level with it, on side dir */
static void beside(int ex, int ey, int dir, int gap, int *tx, int *ty) {
    *tx = iclamp(ex + dir * gap, 12, 308);
    *ty = iclamp(ey, BHP_OY + 12, 168);
}

/* the place for the current step; false if there is none */
static bool step_target(const BhpStage *s, const Tok *t, int *tx, int *ty) {
    int x = PX(s->f.x), y = PX(s->f.y);
    switch (t->kind) {
    case TK_TEA: *tx = s->trig_c * BHP_T + 4; *ty = BHP_OY + s->trig_r * BHP_T + 4; return true;
    case TK_CUP: *tx = s->cup_x; *ty = s->cup_y; return true;
    case TK_EXIT: *tx = s->exit_x; *ty = s->exit_y; return true;
    case TK_WARP: *tx = s->warp_x; *ty = s->warp_y; return true;
    case TK_AT: *tx = t->x * BHP_T + 4; *ty = BHP_OY + t->y * BHP_T + 4; return true;
    case TK_HIT: {
        /* beside the tile, on the side that is open */
        int cx = t->x * BHP_T + 4, cy = BHP_OY + t->y * BHP_T + 4;
        int side = (x <= cx) ? -1 : 1;
        if (bhp_solid(s, cx + side * 13, cy) || bhp_solid(s, cx + side * 9, cy) || bhp_solid(s, cx + side * 17, cy)) side = -side;
        *tx = cx + side * 13;
        *ty = cy;
        return true;
    }
    case TK_WAIT: return false;
    case TK_CIRCLE: {
        int i = nearest_ent(s, BEK_CIRCLER, x, y, NULL);
        if (i < 0) return false;
        const BhpEnt *e = &s->e[i];
        static const int8_t OFF[4][2] = {{0, -18}, {18, 0}, {0, 18}, {-18, 0}};
        int best = -1, bd = 1 << 30;
        for (int k = 0; k < 4; k++) {
            if (e->flag & (1 << k)) continue;
            int nx = PX(e->x) + OFF[k][0], ny = PX(e->y) + OFF[k][1], d = (nx - x) * (nx - x) + (ny - y) * (ny - y);
            if (d < bd) { bd = d; best = k; }
        }
        if (best < 0) return false;
        *tx = PX(e->x) + OFF[best][0];
        *ty = PX(e->y) + OFF[best][1];
        return true;
    }
    case TK_COINS: {
        int i = nearest_ent(s, BEK_BIGCOIN, x, y, NULL);
        if (i < 0) i = nearest_ent(s, BEK_COIN, x, y, NULL);
        if (i < 0) return false;
        *tx = PX(s->e[i].x);
        *ty = PX(s->e[i].y);
        return true;
    }
    case TK_CRYSTALS: {
        /* five rounds is plenty for the tests: then it waits by the exit */
        if (s->round > bhp_bot_rounds) { *tx = s->exit_x; *ty = s->exit_y - 16; return true; }
        int i = nearest_ent(s, BEK_CRYSTAL, x, y, NULL);
        if (i < 0) { *tx = x; *ty = y; return true; }
        const BhpEnt *e = &s->e[i];
        int ex = PX(e->x + e->vx * 10), ey = PX(e->y + e->vy * 10);
        beside(ex, ey, x < ex ? -1 : 1, 13, tx, ty);
        return true;
    }
    case TK_BOSS: {
        const BhpBoss *bo = &s->boss;
        if (bo->down) { *tx = s->exit_x; *ty = s->exit_y; return true; }
        switch (bo->kind) {
        case 1: {
            int i = nearest_ent(s, BEK_BUCKET, x, y, NULL);
            if (i < 0) break;
            const BhpEnt *e = &s->e[i];
            int ex = PX(e->x), ey = PX(e->y);
            beside(ex, ey, ex >= BHP_MILL_X ? 1 : -1, 15, tx, ty);
            return true;
        }
        case 2: {
            int i = nearest_ent(s, BEK_APPLE, x, y, NULL);
            if (i < 0) break;
            const BhpEnt *e = &s->e[i];
            beside(PX(e->x), PX(e->y) + 3, -1, 13, tx, ty);
            return true;
        }
        case 3: {
            int i = nearest_ent(s, BEK_LAMP, x, y, lit_lamp);
            if (i < 0) break;
            const BhpEnt *e = &s->e[i];
            int ex = PX(e->x), ey = PX(e->y);
            beside(ex, ey, ex >= BHP_COG_X[e->a] ? 1 : -1, 15, tx, ty);
            return true;
        }
        case 4: {
            int i = nearest_ent(s, BEK_GUM, x, y, NULL);
            if (i < 0) { *tx = 160; *ty = 60; return true; }
            const BhpEnt *e = &s->e[i];
            int r = e->size == 2 ? 11 : e->size == 1 ? 7 : 5;
            int ex = PX(e->x + e->vx * 8), ey = PX(e->y + e->vy * 8);
            beside(ex, ey, x < ex ? -1 : 1, r + 9, tx, ty);
            return true;
        }
        case 5: {
            int i = nearest_ent(s, BEK_SPIKE, x, y, blue_spike);
            if (s->fuel < 260) {
                int hx = PX(bo->x), hy = PX(bo->y);
                beside(hx, hy, x < hx ? -1 : 1, 27, tx, ty);
                return true;
            }
            if (i >= 0) {
                const BhpEnt *e = &s->e[i];
                int ex = PX(e->x + e->vx * 6), ey = PX(e->y + e->vy * 6);
                beside(ex, ey, x < ex ? -1 : 1, 13, tx, ty);
                return true;
            }
            *tx = 160;
            *ty = 130;
            return true;
        }
        }
        *tx = 160;
        *ty = 140;
        return true;
    }
    }
    return false;
}

/* is the step done? */
static bool step_done(const BhpStage *s, const Tok *t, int k, const BhpBot *b) {
    int x = PX(s->f.x), y = PX(s->f.y);
    switch (t->kind) {
    case TK_TEA: return s->cup >= 1 || s->trig_c < 0;
    case TK_CUP: return s->cup >= 2 || s->trig_c < 0;
    case TK_EXIT: return s->mode == BSM_CLEAR;
    case TK_WARP: return !b->want_warp || s->mode == BSM_WARP || s->t >= BHP_WARP_SHOW;
    case TK_AT: {
        int tx = t->x * BHP_T + 4, ty = BHP_OY + t->y * BHP_T + 4;
        return iabs(x - tx) <= 6 && iabs(y - ty) <= 6;
    }
    case TK_HIT: return done_mark[k] != 0;
    case TK_WAIT: return wait_t >= t->n;
    case TK_CRYSTALS: return s->bonus_done != 0;
    case TK_CIRCLE: return nearest_ent(s, BEK_CIRCLER, x, y, NULL) < 0;
    case TK_COINS: return nearest_ent(s, BEK_BIGCOIN, x, y, NULL) < 0 && nearest_ent(s, BEK_COIN, x, y, NULL) < 0;
    case TK_BOSS: return s->boss.down != 0;
    }
    return true;
}

/* ---- steering ------------------------------------------------------------------ */

/* the pilot's own steering toward the field's low ground */
static unsigned policy(const BhpStage *s, const BhpBot *b) {
    int x = PX(s->f.x), y = PX(s->f.y);
    int i, j;
    int cx = b->tx, cy = b->ty;
    if (cell_of(x, y, &i, &j)) {
        /* nearest cell it fits in, then downhill */
        if (!nav[j][i]) {
            int bi = -1, bj = -1, bv = FARV;
            for (int dj = -2; dj <= 2; dj++)
                for (int di = -2; di <= 2; di++) {
                    int a = i + di, c = j + dj;
                    if (a < 0 || c < 0 || a >= NW || c >= NH || !nav[c][a]) continue;
                    if (b->field[c][a] < bv) { bv = b->field[c][a]; bi = a; bj = c; }
                }
            if (bi >= 0) { i = bi; j = bj; }
        }
        if (b->field[j][i] > 30 && b->field[j][i] != FARV) {
            for (int k = 0; k < 6; k++) {
                int bi = -1, bj = -1, bv = b->field[j][i];
                for (int dj = -1; dj <= 1; dj++)
                    for (int di = -1; di <= 1; di++) {
                        int a = i + di, c = j + dj;
                        if ((!di && !dj) || a < 0 || c < 0 || a >= NW || c >= NH || !nav[c][a]) continue;
                        if (di && dj && (!nav[j][a] || !nav[c][i])) continue;
                        if (b->field[c][a] < bv) { bv = b->field[c][a]; bi = a; bj = c; }
                    }
                if (bi < 0) break;
                i = bi;
                j = bj;
            }
            cx = i * CELL + 2;
            cy = BHP_OY + j * CELL + 2;
        }
    }
    int dx = cx - x, dy = cy - y;
    int d = bhp_isqrt(dx * dx + dy * dy);
    int dt = bhp_isqrt((b->tx - x) * (b->tx - x) + (b->ty - y) * (b->ty - y));
    int speed = imin(300, imax(40, dt * 256 / 20));
    int ci, cj;
    if (cell_of(x, y, &ci, &cj) && clr[cj][ci] <= 2) speed = imin(speed, 180);
    int32_t dvx = d ? dx * speed / d : 0, dvy = d ? dy * speed / d : 0;
    unsigned a = 0;
    if (s->f.vx < dvx - 20) a |= CHF_RIGHT;
    else if (s->f.vx > dvx + 20) a |= CHF_LEFT;
    if (s->f.vy + BHP_TUNE.gravity > dvy) a |= CHF_THRUST;
    return a;
}

/* slash at whatever is in reach on the facing side */
static bool want_slash(const BhpStage *s) {
    if (s->mode != BSM_FLY || s->slash_cd) return false;
    int cx = PX(s->f.x), cy = PX(s->f.y);
    int x0 = s->f.face >= 0 ? cx + 2 : cx - BHP_SLASH_REACH, x1 = s->f.face >= 0 ? cx + BHP_SLASH_REACH : cx - 2;
    int y0 = cy - 8, y1 = cy + 8;
    for (int i = 0; i < s->ne; i++) {
        const BhpEnt *e = &s->e[i];
        if (!e->on) continue;
        bool ok = false;
        switch (e->kind) {
        case BEK_MOTH: case BEK_MITE: case BEK_WASP: case BEK_CRAWLER: case BEK_TURRET: case BEK_GHOST: case BEK_DRONE:
        case BEK_BUBBLE: case BEK_BUCKET: case BEK_GUM: ok = true; break;
        case BEK_CRYSTAL: ok = s->round <= bhp_bot_rounds; break;
        case BEK_LAMP: case BEK_SPIKE: ok = e->flag == 1; break;
        case BEK_APPLE: ok = s->f.face > 0 && PX(e->x) > cx; break;
        default: break;
        }
        if (!ok) continue;
        int a0, b0, a1, b1;
        bhp_ent_box(e, &a0, &b0, &a1, &b1);
        if (rects_overlap(x0, y0, x1 - x0 + 1, y1 - y0 + 1, a0, b0, a1 - a0 + 1, b1 - b0 + 1)) return true;
    }
    for (int y = y0; y <= y1; y += 4)
        for (int x = x0; x <= x1; x += 4) {
            int t = bhp_tile_at(s, x, y);
            if (t == BTL_BLOCK || t == BTL_GLASS) return true;
        }
    if (s->kind == BHK_BOSS && s->boss.kind == 5 && !s->boss.down && s->fuel < 300) {
        int hx = PX(s->boss.x), hy = PX(s->boss.y);
        if (rects_overlap(x0, y0, x1 - x0 + 1, y1 - y0 + 1, hx - 14, hy - 9, 28, 18)) return true;
    }
    return false;
}

/* ---- planning ----------------------------------------------------------------
 * A plan is a control for each of the next LOOK frames (POLICY: steer by the
 * field). Each time it plans, the pilot tries: the rest of its last plan;
 * its own steering from now on; ten quick starts (a direction held for a few
 * frames, with or without thrust, then its own steering); nine plans that
 * hold one control all the way; a dozen copies of the last plan with a
 * stretch of it changed; and waiting in place for 8 to 32 frames before
 * steering (for a gap that opens and shuts). The pilot's dice are its own, so a run plays the
 * same every time. */

static const unsigned HORIZ[3] = {CHF_LEFT, 0, CHF_RIGHT};

/* control a (0..8): a direction (left, none, right) and thrust (off, tapped, held) */
static unsigned held_action(int a, int k) {
    int v = a / 3;
    return HORIZ[a % 3] | (v == 2 || (v == 1 && (k % 5) < 2) ? CHF_THRUST : 0);
}

static uint8_t best_plan[LOOK];   /* POLICY, or 1 + the controls */
static Rng dice;

static BhpStage sim;

/* fly a plan forward; lower is better. got: the controls it used */
static long rollout(const BhpStage *s, const BhpBot *b, const uint8_t *plan, const Tok *t, int tk, uint8_t *got) {
    memcpy(&sim, s, sizeof sim);
    int prog0 = s->kind == BHK_BOSS ? bhp_boss_progress(s) : 0;
    long pts = 0;
    for (int k = 0; k < LOOK; k++) {
        unsigned ctl = plan[k] == POLICY ? policy(&sim, b) : (unsigned)(plan[k] - 1);
        if (got) got[k] = (uint8_t)(ctl + 1);
        if (want_slash(&sim)) ctl |= BHP_CTL_SLASH;
        bhp_stage_step(&sim, ctl);
        pts += sim.pts;
        if (sim.mode == BSM_DEAD) return 10000000L - k * 1000L;
        if (sim.mode == BSM_CLEAR && t->kind != TK_EXIT && t->kind != TK_BOSS) return 9000000L - k * 1000L;
        if (sim.mode == BSM_WARP && t->kind != TK_WARP) return 9000000L - k * 1000L;
        if (t->kind != TK_WAIT && t->kind != TK_HIT && step_done(&sim, t, tk, b)) {
            if (got) for (int j = k + 1; j < LOOK; j++) got[j] = POLICY;
            return -1000000L + k * 100L;
        }
        if (t->kind == TK_HIT && (sim.ev & (BEV_LEVER | BEV_FUSE))) {
            if (got) for (int j = k + 1; j < LOOK; j++) got[j] = POLICY;
            return -1000000L + k * 100L;
        }
    }
    long score = field_at(b, PX(sim.f.x), PX(sim.f.y));
    if (s->kind == BHK_BOSS) score -= (long)(bhp_boss_progress(&sim) - prog0) * 40;
    if (s->kind == BHK_BONUS && s->round <= bhp_bot_rounds) score -= pts * 4;
    /* a ship that ends fast has less room to save itself */
    score += iabs(sim.f.vy) / 16 + iabs(sim.f.vx) / 24;
    if (sim.fuel < 60) score += (60 - sim.fuel) * 4;
    return score;
}

static void plan(const BhpStage *s, BhpBot *b, const Tok *t) {
    static uint8_t cand[LOOK], got[LOOK], keep[LOOK];
    long best = 0x7fffffffL;
    int bn = -1;
    int total = 1 + NCAND + 9 + NMUT + 4;
    uint8_t prev[LOOK];
    /* the rest of the last plan, then steering */
    for (int k = 0; k < LOOK; k++) prev[k] = k + REPLAN < LOOK ? best_plan[k + REPLAN] : POLICY;
    for (int n = 0; n < total; n++) {
        if (n == 0) memcpy(cand, prev, LOOK);
        else if (n <= NCAND) {
            int c = n - 1;
            for (int k = 0; k < LOOK; k++) {
                if (c == 0 || k >= LEAD) cand[k] = POLICY;
                else if (c <= 6) cand[k] = (uint8_t)(1 + (HORIZ[(c - 1) % 3] | ((c - 1) / 3 ? CHF_THRUST : 0)));
                else cand[k] = (uint8_t)(1 + (HORIZ[c - 7] | ((k % 5) < 2 ? CHF_THRUST : 0)));
            }
        } else if (n <= NCAND + 9) {
            int a = n - NCAND - 1;
            for (int k = 0; k < LOOK; k++) cand[k] = (uint8_t)(1 + held_action(a, k));
        } else if (n > NCAND + 9 + NMUT) {
            /* wait (hold still, tapping) a while, then steer: for gaps that open and shut */
            int w = 8 * (n - NCAND - 9 - NMUT);
            for (int k = 0; k < LOOK; k++) cand[k] = k < w ? (uint8_t)(1 + held_action(4, k)) : POLICY;
        } else {
            /* the last plan with a stretch changed */
            memcpy(cand, prev, LOOK);
            int from = rng_range(&dice, 0, LOOK - 4), len = rng_range(&dice, 4, 20), a = rng_range(&dice, 0, 8);
            for (int k = from; k < LOOK && k < from + len; k++) cand[k] = (uint8_t)(1 + held_action(a, k));
        }
        long v = rollout(s, b, cand, t, b->step, got);
        if (bhp_bot_debug > 1) printf(" %d=%ld(%d,%d)", n, v, PX(sim.f.x), PX(sim.f.y));
        if (v < best) {
            best = v;
            bn = n;
            memcpy(keep, got, LOOK);
        }
    }
    if (bhp_bot_debug) printf("\nbot %s step %d plan %d score %ld at %d,%d fuel %d\n", bhp_stage_label(s->idx), b->step, bn, best, PX(s->f.x), PX(s->f.y), s->fuel);
    memcpy(best_plan, keep, LOOK);
    b->plan = (uint8_t)bn;
    b->plan_left = REPLAN;
}

void bhp_bot_reset(BhpBot *b, bool warps) {
    memset(b, 0, sizeof *b);
    b->stage = 255;
    b->want_warp = warps;
    nav_ver = 0xFFFF;
    nav_stage = -1;
}

unsigned bhp_bot(const BhpStage *s, BhpBot *b) {
    if (b->stage != s->idx) {
        uint8_t w = b->want_warp;
        memset(b, 0, sizeof *b);
        b->want_warp = w;
        b->stage = s->idx;
        b->ver = 0xFFFF;
        ntok = parse(BHP_STAGE[s->idx].route, toks);
        memset(done_mark, 0, sizeof done_mark);
        memset(best_plan, POLICY, sizeof best_plan);
        rng_seed(&dice, 1700u + s->idx);
        last_life = 0;
        nav_stage = -1;
    }
    if (s->mode == BSM_DEAD || s->mode == BSM_CLEAR || s->mode == BSM_WARP) return 0;
    /* a new life: the route starts again (the tea remembers itself) */
    if (s->mode == BSM_BUBBLE || s->life_t < last_life) {
        if (b->step) { b->step = 0; memset(done_mark, 0, sizeof done_mark); }
        wait_t = 0;
        memset(best_plan, POLICY, sizeof best_plan);
    }
    last_life = s->life_t;
    if (nav_stage != s->idx || nav_ver != s->ver) build_nav(s);
    /* news from the last frame: a lever flipped or a fuse lit */
    if (b->step < ntok && toks[b->step].kind == TK_HIT && (s->ev & (BEV_LEVER | BEV_FUSE))) done_mark[b->step] = 1;
    /* skip what is done */
    while (b->step < ntok && step_done(s, &toks[b->step], b->step, b)) {
        b->step++;
        wait_t = 0;
        b->plan_left = 0;
        memset(best_plan, POLICY, sizeof best_plan);
    }
    if (b->step >= ntok) return policy(s, b) | (want_slash(s) ? BHP_CTL_SLASH : 0);
    const Tok *t = &toks[b->step];
    if (t->kind == TK_WAIT) {
        wait_t++;
        if (s->mode == BSM_BUBBLE) return 0;
    }
    int tx = b->tx, ty = b->ty;
    if (t->kind == TK_WAIT) {
        if (wait_t == 1) { tx = PX(s->f.x); ty = PX(s->f.y); }
    } else step_target(s, t, &tx, &ty);
    if (tx != b->tx || ty != b->ty || b->ver != nav_ver || b->field[0][0] == 0) build_field(b, tx, ty);
    /* nowhere to go yet (an empty crystal room): leave the bubble anyway */
    if (s->mode == BSM_BUBBLE && t->kind != TK_WAIT && iabs(tx - PX(s->f.x)) < 4 && iabs(ty - PX(s->f.y)) < 4) return CHF_THRUST;

    /* at a lever or bomb, level with it and facing it: slash */
    if (t->kind == TK_HIT && s->mode == BSM_FLY) {
        int cx = t->x * BHP_T + 4, cy = BHP_OY + t->y * BHP_T + 4, x = PX(s->f.x), y = PX(s->f.y);
        int dir = cx > x ? 1 : -1;
        if (iabs(cx - x) <= BHP_SLASH_REACH + 2 && iabs(cx - x) >= 6 && iabs(cy - y) <= 7) {
            unsigned a = policy(s, b) & CHF_THRUST;
            if (s->f.face != dir) return a | (dir > 0 ? CHF_RIGHT : CHF_LEFT);
            if (!s->slash_cd && !b->prev_slash) { b->prev_slash = 1; return a | BHP_CTL_SLASH; }
            b->prev_slash = 0;
            return a;
        }
    }

    if (b->plan_left == 0) plan(s, b, t);
    int k = REPLAN - b->plan_left;
    b->plan_left--;
    unsigned ctl = best_plan[k] == POLICY ? policy(s, b) : (unsigned)(best_plan[k] - 1);
    if (want_slash(s) && !b->prev_slash) { ctl |= BHP_CTL_SLASH; b->prev_slash = 1; }
    else b->prev_slash = 0;
    return ctl;
}

/* debugging: the field and where a ship fits, one character a cell */
void bhp_bot_dump(const BhpBot *b) {
    for (int j = 0; j < NH; j++) {
        char line[NW + 1];
        for (int i = 0; i < NW; i++) {
            uint16_t v = b->field[j][i];
            line[i] = !nav[j][i] ? '#' : v == FARV ? '?' : (char)('0' + (v / 100) % 10);
        }
        line[NW] = 0;
        printf("%s\n", line);
    }
}
