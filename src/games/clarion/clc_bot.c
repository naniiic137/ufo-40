/* CLARION CALL - the demo player. It reads the run like a player who knows
 * the rules and presses the real buttons; nothing is moved by hand.
 *
 * On a map it plans over a distance field of the cells the Clarion fits
 * through (kept away from walls), flies a gentle line along it, slashes
 * whatever comes close, sets down on the landing spot beside a door, walks
 * in, and climbs back aboard on the way out. It takes notes nearest first,
 * calls on the friendly sorts for fuel, the shops when it is hurt or low,
 * the sextons when it is after Grandsire Tock, the Cellars' cave for the
 * yellow key, and leaves the tenth note for one near the gold door.
 *
 * Behind a door it walks to what it wants (the chest, the item, the way
 * out); in caves and boss fights it looks ahead: it tries a dozen ways to
 * move for the next 24 frames on a copy of the rules, shooting all the
 * while, and takes the one that gets on fastest without being hurt. */
#include "clc.h"

int clc_bot_debug;
int clc_bot_mode;

static int PX(int32_t v) { return (int)(v / 256) - (v < 0 && v % 256 ? 1 : 0); }

/* ---- distance fields over the ship's cells -------------------------------------- */

#define FW (CLC_MW + 1)
#define FH (CLC_MH + 1)
#define INF 0xFFFF
static uint8_t clear_[FH][FW];
static uint16_t fld[FH][FW];     /* toward the goal */
static uint16_t from_[FH][FW];   /* from the ship */
static const ClcWorld *cw;
static uint16_t cw_ver;
static uint8_t cw_region, cw_area, cw_kind;

static bool cfree(const ClcWorld *w, int c, int r) { return clc_ship_cell_free(w, c, r); }

static void build_clearance(const ClcWorld *w) {
    static int16_t q[FW * FH][2];
    int head = 0, tail = 0;
    for (int r = 0; r < FH; r++)
        for (int c = 0; c < FW; c++) {
            if (r > w->h || c > w->w || !cfree(w, c, r)) {
                clear_[r][c] = 0;
                q[tail][0] = (int16_t)c;
                q[tail][1] = (int16_t)r;
                tail++;
            } else clear_[r][c] = 255;
        }
    while (head < tail) {
        int c = q[head][0], r = q[head][1];
        head++;
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int nc = c + dx, nr = r + dy;
                if (nc < 0 || nr < 0 || nc >= FW || nr >= FH) continue;
                if (clear_[nr][nc] <= clear_[r][c] + 1) continue;
                clear_[nr][nc] = (uint8_t)(clear_[r][c] + 1);
                q[tail][0] = (int16_t)nc;
                q[tail][1] = (int16_t)nr;
                tail++;
            }
    }
}

static void danger_loose(const ClcWorld *w);

/* how much the turrets and throwers can see of each cell */
static uint8_t danger_[FH][FW];
static bool danger_dash;

static void build_danger(const ClcWorld *w) {
    memset(danger_, 0, sizeof danger_);
    danger_dash = w->timer_on;
    danger_loose(w);
    for (int i = 0; i < w->ne; i++) {
        const ClcEnt *e = &w->e[i];
        if (!e->on) continue;
        int k = e->kind;
        if (k == EK_POD || k == EK_NEST || k == EK_WALLEYE || k == EK_ACID || k == EK_SENTRY || k == EK_BIGBELLY) {
            /* the ones that sit still: give them a wide berth */
            int ec = PX(e->x) / CLC_T, er = PX(e->y) / CLC_T, rr = k == EK_BIGBELLY ? 4 : 2;
            for (int r = er - rr; r <= er + rr; r++)
                for (int c = ec - rr; c <= ec + rr; c++)
                    if (r >= 0 && c >= 0 && r < FH && c < FW && danger_[r][c] < 200) danger_[r][c] = (uint8_t)imin(200, danger_[r][c] + 90);
        }
        if (!(k == EK_SENTRY || k == EK_EYE || k == EK_BOOMER || (k == EK_POD && w->timer_on))) continue;
        int ex = (int)(e->x >> 8), ey = (int)(e->y >> 8), reach = k == EK_BOOMER ? 13 : 15;
        int ec = ex / CLC_T, er = ey / CLC_T;
        for (int r = er - reach; r <= er + reach; r++)
            for (int c = ec - reach; c <= ec + reach; c++) {
                if (r < 0 || c < 0 || r >= FH || c >= FW) continue;
                int dx = c * CLC_T - ex, dy = r * CLC_T - ey;
                if (dx * dx + dy * dy > reach * reach * 64) continue;
                bool seen = true;
                if (k != EK_BOOMER) {
                    int n = imax(iabs(dx), iabs(dy));
                    for (int s2 = 4; s2 < n - 4 && seen; s2 += 4)
                        if (clc_map_solid(w, ex + dx * s2 / n, ey + dy * s2 / n)) seen = false;
                }
                if (seen && danger_[r][c] < 200) danger_[r][c] = (uint8_t)(danger_[r][c] + 25);
            }
    }
}

/* the Cellars' loose stones: keep out from under them */
static void danger_loose(const ClcWorld *w) {
    for (int r = 0; r < w->h; r++)
        for (int c = 0; c < w->w; c++) {
            if (w->tile[r][c] != MT_LOOSE) continue;
            for (int rr = r + 1; rr <= r + 10 && rr < FH; rr++) {
                if (rr < w->h && clc_map_solid_tile(w->tile[rr][c])) break;
                for (int cc = c - 1; cc <= c + 2; cc++)
                    if (cc >= 0 && cc < FW && danger_[rr][cc] < 200) danger_[rr][cc] = (uint8_t)imin(200, danger_[rr][cc] + 70);
            }
        }
}

static int step_cost(int c, int r) {
    int k = clear_[r][c];
    return (k <= 1 ? 50 : k == 2 ? 14 : 0) + danger_[r][c];
}

/* Dijkstra from (c, r) over free cells; 10 a step, 14 a diagonal, more
 * close to the walls */
static uint64_t heap[FW * FH * 8];
static int hn;

static void hpush(uint64_t v) {
    if (hn >= (int)(sizeof heap / sizeof heap[0])) return;
    int i = hn++;
    heap[i] = v;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (heap[p] <= heap[i]) break;
        uint64_t t = heap[p];
        heap[p] = heap[i];
        heap[i] = t;
        i = p;
    }
}

static uint64_t hpop(void) {
    uint64_t top = heap[0];
    heap[0] = heap[--hn];
    for (int i = 0;;) {
        int l = i * 2 + 1, r = l + 1, m = i;
        if (l < hn && heap[l] < heap[m]) m = l;
        if (r < hn && heap[r] < heap[m]) m = r;
        if (m == i) break;
        uint64_t t = heap[i];
        heap[i] = heap[m];
        heap[m] = t;
        i = m;
    }
    return top;
}

static bool outward; /* the field is flown away from its start (from the ship) */

static void dijkstra(uint16_t out[FH][FW], const ClcWorld *w, int c, int r) {
    for (int y = 0; y < FH; y++)
        for (int x = 0; x < FW; x++) out[y][x] = INF;
    if (c < 0 || r < 0 || c >= FW || r >= FH) return;
    hn = 0;
    out[r][c] = 0;
    hpush((uint64_t)(r * FW + c));
    while (hn > 0) {
        uint64_t top = hpop();
        uint32_t d = (uint32_t)(top >> 32);
        int idx = (int)(top & 0xFFFFFFFFu), x = idx % FW, y = idx / FW;
        if (d > out[y][x]) continue;
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                if (!dx && !dy) continue;
                int nx = x + dx, ny = y + dy;
                if (nx < 0 || ny < 0 || nx >= FW || ny >= FH || !cfree(w, nx, ny)) continue;
                if (dx && dy && (!cfree(w, x + dx, y) || !cfree(w, x, y + dy))) continue;
                /* the field runs from the goal outwards, so a step here from
                 * (x, y) to (nx, ny) is flown the other way: from (nx, ny) to
                 * (x, y). Climbing costs fuel, falling is free. */
                int up = outward ? -dy : dy;
                uint32_t base = up > 0 ? 13u : up < 0 ? 7u : 10u;
                if (dx && dy) base += 4u;
                uint32_t nd = d + base + (uint32_t)step_cost(nx, ny);
                if (nd >= out[ny][nx] || nd >= INF) continue;
                out[ny][nx] = (uint16_t)nd;
                hpush((uint64_t)nd << 32 | (uint64_t)(ny * FW + nx));
            }
    }
}

/* the free cell nearest a point */
static void cell_of(const ClcWorld *w, int x, int y, int *c, int *r) {
    int bc = (x + 4) / CLC_T, br = (y + 4) / CLC_T, best = 99;
    *c = bc;
    *r = br;
    for (int dy = -2; dy <= 2; dy++)
        for (int dx = -2; dx <= 2; dx++) {
            int cc = bc + dx, rr = br + dy;
            if (cc < 0 || rr < 0 || cc >= FW || rr >= FH || !cfree(w, cc, rr)) continue;
            int d = iabs(cc * CLC_T - x) + iabs(rr * CLC_T - y);
            if (d < best) { best = d; *c = cc; *r = rr; }
        }
}

/* ---- the bot's memory ------------------------------------------------------------ */

enum { GK_NONE, GK_NOTE, GK_DOOR, GK_TOP, GK_HUNT, GK_COIN };
static struct {
    int gk, gi;                /* goal kind and index (note entity, door) */
    int gx, gy;                /* goal point (px) */
    int fx, fy;                /* the field is built for this point */
    uint16_t fver;
    int door;                  /* the door we're landing for */
    uint8_t visited[CLC_DOORS];
    uint8_t skip_note[CLC_ENTS];
    uint8_t ignore[CLC_ENTS];
    uint16_t clear_t[CLC_ENTS];
    uint16_t hunt_n[CLC_ENTS];
    int still_x, still_y, still_t, nudge_t, nudge_k;
    int cave_x, cave_t;
    uint8_t ignore_h[CLC_ENTS];
    int frames, best_d, stuck;
    int replan;
    int land_t;
    int hunt_t;
    unsigned prevb;
    /* behind a door */
    uint16_t sub_ver;
    int sub_room, sub_phase, sub_t;
    int want_item;
    unsigned plan[24];
    int plan_left, plan_at;
} B;

static int target = -1;

void clc_bot_reset(void) {
    memset(&B, 0, sizeof B);
    B.door = -1;
    B.want_item = -1;
    cw = NULL;
    target = -1;
}

int clc_bot_target(void) { return B.door; }

static void new_world(const ClcWorld *w) {
    cw = w;
    cw_ver = w->ver;
    cw_region = w->region;
    cw_area = w->area;
    cw_kind = w->kind;
    build_clearance(w);
    build_danger(w);
    B.gk = GK_NONE;
    B.door = -1;
    B.fver = 0xFFFF;
    B.fx = B.fy = -999;
    memset(B.visited, 0, sizeof B.visited);
    memset(B.skip_note, 0, sizeof B.skip_note);
    memset(B.ignore, 0, sizeof B.ignore);
    memset(B.clear_t, 0, sizeof B.clear_t);
    memset(B.hunt_n, 0, sizeof B.hunt_n);
    memset(B.ignore_h, 0, sizeof B.ignore_h);
    B.stuck = 0;
}

static void need_field(const ClcWorld *w, int x, int y) {
    if (danger_dash != (w->timer_on != 0)) { build_danger(w); B.fx = -999; }
    if (B.fx == x && B.fy == y && B.fver == w->ver) return;
    int c, r;
    cell_of(w, x, y, &c, &r);
    dijkstra(fld, w, c, r);
    B.fx = x;
    B.fy = y;
    B.fver = w->ver;
}

static int field_at(uint16_t f[FH][FW], const ClcWorld *w, int x, int y) {
    int c, r;
    cell_of(w, x, y, &c, &r);
    return f[r][c];
}

/* ---- what the bot wants ------------------------------------------------------------ */

static bool foe_kind(int k) {
    if (k == EK_NEST || k == EK_WALLEYE || k == EK_BMISSILE) return true;
    return (k >= EK_FLITTER && k <= EK_SEG) && k != EK_LATE && k != EK_JETFIRE && k != EK_SNAP && k != EK_SENTRY && k != EK_POD;
}

static bool wants_door(const ClcBotView *v, int k) {
    const ClcWorld *w = v->w;
    const ClcDoor *d = &w->door[k];
    const ClcPlayer *p = v->p;
    if (d->hidden || B.visited[k]) return false;
    if (d->type == DT_GOLD) return false;
    switch (d->room) {
    case RM_NPC: return !d->used;
    case RM_SAGE:
        /* only the sexton the chain leads to */
        if (v->goal != BOT_CHERRY || d->used) return false;
        return w->region == RG_SPIRE ? v->scrolls == 3 && v->chain == RG_SPIRE : v->chain == w->region;
    case RM_CHARMSHOP: return p->coins >= 100 && !clc_has(p, G_CHARM);
    /* fuel comes first: coins burn as fuel when the tank is dry */
    case RM_HEALTH:
        if (v->goal == BOT_CHERRY && w->region == RG_SPIRE && p->hp <= p->hpmax - 2 && p->coins >= 100) return true;
        return p->coins >= 100 && (p->hp <= 6 || p->hp <= p->hpmax - 4);
    case RM_SHOP:
    case RM_MEGA:
        if (v->goal == BOT_CHERRY && w->region == RG_SPIRE && p->hp <= p->hpmax - 2 && p->coins >= 110) return true;
        return p->coins >= 110 && (p->hp <= p->hpmax - 4 || p->fuel < 500 || p->coins >= 150);
    case RM_CAVE: return !d->used && w->region == RG_CELLARS && w->area == 1;
    case RM_ARMOR: return p->key && v->goal != BOT_BAD;
    case RM_SHORTCUT: return p->key && v->goal == BOT_BAD;
    default: return false;
    }
}

static void door_goal(const ClcWorld *w, int k) {
    int x, y;
    clc_door_pad(&w->door[k], &x, &y);
    B.gk = GK_DOOR;
    B.gi = k;
    B.door = k;
    B.gx = x;
    B.gy = y - 10;
    B.best_d = 1 << 30;
    B.frames = 0;
    B.land_t = 0;
}

static void choose_goal_(const ClcBotView *v);

/* A round of the notes: from the ship through `need` of the n notes (or all
 * of them if fewer) and on to the gold door, as cheap as it can be flown.
 * Costs come from the fields (climbing dear, falling cheap); a small dynamic
 * programme over the subsets finds it. Gives the index of the first note. */
#define RT_MAX 12
static uint16_t rt_field[FH][FW];
static uint32_t rt_dp[1 << RT_MAX][RT_MAX];
static int8_t rt_par[1 << RT_MAX][RT_MAX];

static int route_first(const ClcWorld *w, int n, const int *id, const int *from_ship, uint16_t gold[FH][FW], int need) {
    if (n <= 0) return -1;
    if (need > n) need = n;
    if (need < 1) need = 1;
    static uint32_t dist[RT_MAX][RT_MAX];
    uint32_t to_gold[RT_MAX];
    for (int a = 0; a < n; a++) {
        const ClcEnt *e = &w->e[id[a]];
        int c, r;
        cell_of(w, PX(e->x), PX(e->y), &c, &r);
        outward = true;
        dijkstra(rt_field, w, c, r);
        outward = false;
        for (int b = 0; b < n; b++) {
            const ClcEnt *f = &w->e[id[b]];
            dist[a][b] = (uint32_t)field_at(rt_field, w, PX(f->x), PX(f->y));
        }
        to_gold[a] = (uint32_t)field_at(gold, w, PX(e->x), PX(e->y));
    }
    const uint32_t BIG = 0xFFFFFFFFu;
    int full = 1 << n;
    for (int m = 0; m < full; m++)
        for (int a = 0; a < n; a++) rt_dp[m][a] = BIG;
    for (int a = 0; a < n; a++) { rt_dp[1 << a][a] = (uint32_t)from_ship[a]; rt_par[1 << a][a] = -1; }
    uint32_t best = BIG;
    int bm = 0, ba = -1;
    for (int m = 1; m < full; m++) {
        int pc = 0;
        for (int q = m; q; q &= q - 1) pc++;
        if (pc > need) continue;
        for (int a = 0; a < n; a++) {
            uint32_t c0 = rt_dp[m][a];
            if (c0 == BIG || !(m >> a & 1)) continue;
            if (pc == need) {
                uint32_t t = c0 + to_gold[a];
                if (to_gold[a] != INF && t < best) { best = t; bm = m; ba = a; }
                continue;
            }
            for (int b = 0; b < n; b++) {
                if (m >> b & 1 || dist[a][b] == INF) continue;
                uint32_t c1 = c0 + dist[a][b];
                int m2 = m | 1 << b;
                if (c1 < rt_dp[m2][b]) { rt_dp[m2][b] = c1; rt_par[m2][b] = (int8_t)a; }
            }
        }
    }
    if (ba < 0) {
        /* no way on to the gold door from any of them: the nearest */
        int k = 0;
        for (int a = 1; a < n; a++) if (from_ship[a] < from_ship[k]) k = a;
        return k;
    }
    /* back along the round to its first note */
    int m = bm, a = ba;
    while (rt_par[m][a] >= 0) {
        int pa = rt_par[m][a];
        m &= ~(1 << a);
        a = pa;
    }
    if (clc_bot_debug) fprintf(stderr, "round of %d from %d notes: cost %u, first note %d\n", need, n, (unsigned)best, id[a]);
    return a;
}

/* a fresh look at what to do; the same goal again keeps its record of
 * headway (so a goal that can't be reached is given up in the end) */
static void choose_goal(const ClcBotView *v) {
    int ogk = B.gk, ogi = B.gi, of = B.frames, ob = B.best_d;
    choose_goal_(v);
    if (B.gk == ogk && B.gi == ogi) { B.frames = of; B.best_d = ob; }
}

static void choose_goal_(const ClcBotView *v) {
    const ClcWorld *w = v->w;
    const ClcPlayer *p = v->p;
    int sx = PX(w->f.x), sy = PX(w->f.y);
    int c, r;
    cell_of(w, sx, sy, &c, &r);
    outward = true;
    dijkstra(from_, w, c, r);
    outward = false;
    B.best_d = 1 << 30;
    B.frames = 0;
    B.land_t = 0;
    if (w->kind == WK_ESCAPE) {
        B.gk = GK_TOP;
        B.gx = w->w * CLC_T / 2;
        B.gy = 6;
        return;
    }
    if (w->kind == WK_CROWN) {
        int want = -1;
        for (int k = 0; k < w->nd; k++) {
            const ClcDoor *d = &w->door[k];
            if (B.visited[k] || d->hidden) continue;
            if (d->room == RM_HEALTH && p->hp <= p->hpmax - 4 && p->coins >= 100 && v->goal != BOT_CHERRY) want = k;
            if (d->room == RM_SHOP && p->coins >= 100 && p->hp <= p->hpmax - 4 && want < 0 && v->goal != BOT_CHERRY) want = k;
        }
        if (want < 0)
            for (int k = 0; k < w->nd; k++) {
                const ClcDoor *d = &w->door[k];
                if (d->hidden) continue;
                if (v->goal == BOT_CHERRY && d->room == RM_TOCK) want = k;
                if (v->goal != BOT_CHERRY && d->room == RM_HUSH) want = k;
            }
        if (want < 0)
            for (int k = 0; k < w->nd; k++)
                if (w->door[k].room == RM_HUSH) want = k;
        door_goal(w, want);
        return;
    }
    if (w->timer_on) { door_goal(w, w->gold_door); return; }
    /* the nearest of: a note (while fewer than nine), a door worth a visit */
    int best = INF, bk = GK_NONE, bi = -1;
    int gpx, gpy;
    clc_door_pad(&w->door[w->gold_door], &gpx, &gpy);
    /* the true route: the Spire's sexton sends her straight on to the
     * secret boss, so a stall or a shop comes first */
    bool heal_first = false, sexton_left = false;
    if (v->goal == BOT_CHERRY && w->region == RG_SPIRE)
        for (int k = 0; k < w->nd; k++) {
            int rm = w->door[k].room;
            if ((rm == RM_HEALTH || rm == RM_SHOP || rm == RM_MEGA) && p->hp <= p->hpmax - 4 && p->coins >= 110 && wants_door(v, k))
                heal_first = true;
            if (rm == RM_SAGE && wants_door(v, k)) sexton_left = true;
        }
    for (int k = 0; k < w->nd; k++) {
        if (!wants_door(v, k)) continue;
        int x, y;
        clc_door_pad(&w->door[k], &x, &y);
        int d = field_at(from_, w, x, y - 10);
        /* the tank first: a detour only with fuel to spare for it, unless
         * the door is where the fuel is */
        int room = w->door[k].room;
        bool fuel_src = room == RM_NPC || ((room == RM_SHOP || room == RM_MEGA || room == RM_HEALTH) && p->coins >= 110);
        int cost = d / 2; /* about what the trip burns */
        bool must = room == RM_SHORTCUT || room == RM_SAGE || room == RM_ARMOR || (room == RM_CAVE && v->goal == BOT_BAD);
        if (!fuel_src && !must && p->fuel < cost * 2 + 250) continue;
        if (fuel_src && p->fuel < cost + 120) continue;
        if (fuel_src && p->fuel < 300) d /= 2;
        if (room == RM_SAGE && heal_first) continue;
        if (room == RM_SHORTCUT || room == RM_SAGE) d /= 4; /* the point of the area */
        if (d < best) { best = d; bk = GK_DOOR; bi = k; }
    }
    bool doors_left = bk == GK_DOOR;
    /* the notes: the cheapest round of the ones still needed that ends at
     * the gold door, and the first note on it */
    static uint16_t gold_field[FH][FW];
    static uint16_t gf_ver = 0xFFFF;
    static const ClcWorld *gf_w;
    if (gf_ver != w->ver || gf_w != w) {
        int gc, gr;
        cell_of(w, gpx, gpy - 10, &gc, &gr);
        dijkstra(gold_field, w, gc, gr);
        gf_ver = w->ver;
        gf_w = w;
    }
    int ni = 0, id[RT_MAX], from_ship[RT_MAX];
    for (int i = 0; i < w->ne; i++) {
        const ClcEnt *e = &w->e[i];
        if (!e->on || e->kind != EK_NOTE || B.skip_note[i]) continue;
        if (w->notes >= CLC_NOTES_NEEDED - 1 && (doors_left || sexton_left)) continue; /* the tenth waits for the doors */
        int d = field_at(from_, w, PX(e->x), PX(e->y));
        if (d == INF) continue;
        if (ni < RT_MAX) { id[ni] = i; from_ship[ni] = d; ni++; continue; }
        /* too many to plan over: keep the nearest */
        int far = 0;
        for (int k = 1; k < RT_MAX; k++) if (from_ship[k] > from_ship[far]) far = k;
        if (d < from_ship[far]) { id[far] = i; from_ship[far] = d; }
    }
    int first = route_first(w, ni, id, from_ship, gold_field, CLC_NOTES_NEEDED - w->notes);
    if (first >= 0 && from_ship[first] < best) { best = from_ship[first]; bk = GK_NOTE; bi = id[first]; }
    if (bk == GK_DOOR) { door_goal(w, bi); return; }
    if (bk == GK_NOTE) {
        B.gk = GK_NOTE;
        B.gi = bi;
        B.gx = PX(w->e[bi].x);
        B.gy = PX(w->e[bi].y);
        B.door = -1;
        return;
    }
    B.gk = GK_NONE;
}

/* ---- flying ------------------------------------------------------------------------ */

static unsigned steer(const ClcWorld *w, int dvx, int dvy) {
    unsigned b = 0;
    int vx = w->f.vx, vy = w->f.vy;
    /* rock just ahead: come in slower than a knock costs */
    int32_t fx = w->f.x, fy = w->f.y;
    if (dvx > 90 && chm_flight_blocked(4, clc_map_solid, w, fx + 6 * 256, fy)) dvx = 90;
    if (dvx < -90 && chm_flight_blocked(4, clc_map_solid, w, fx - 6 * 256, fy)) dvx = -90;
    if (dvy < -90 && chm_flight_blocked(4, clc_map_solid, w, fx, fy - 6 * 256)) dvy = -90;
    /* and a ceiling coming up while going along fast */
    if (dvy < -60 && (chm_flight_blocked(4, clc_map_solid, w, fx + vx * 6, fy - 5 * 256) || chm_flight_blocked(4, clc_map_solid, w, fx + vx * 3, fy - 4 * 256)))
        dvy = -60;
    if (dvy > 150 && chm_flight_blocked(4, clc_map_solid, w, fx, fy + 6 * 256)) dvy = 150;
    if (vy + CLC_TUNE.gravity > dvy) b |= BTN_A;
    if (vx < dvx - 8) b |= BTN_RIGHT;
    else if (vx > dvx + 8) b |= BTN_LEFT;
    return b;
}

static void desired(const ClcWorld *w, int gx, int gy, bool arrive, int *odvx, int *odvy) {
    need_field(w, gx, gy);
    int x = PX(w->f.x), y = PX(w->f.y);
    int c, r;
    cell_of(w, x, y, &c, &r);
    int lc = c, lr = r;
    int path_c[6], path_r[6], np = 0;
    for (int k = 0; k < 6; k++) {
        int bc = lc, br = lr, bv = fld[lr][lc];
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int nc = lc + dx, nr = lr + dy;
                if (nc < 0 || nr < 0 || nc >= FW || nr >= FH) continue;
                if (fld[nr][nc] < bv) { bv = fld[nr][nc]; bc = nc; br = nr; }
            }
        if (bc == lc && br == lr) break;
        lc = bc;
        lr = br;
        path_c[np] = lc;
        path_r[np] = lr;
        np++;
    }
    /* the furthest of those the ship can see in a straight line */
    int tx = c * CLC_T, ty = r * CLC_T;
    for (int k = np - 1; k >= 0; k--) {
        int ex = path_c[k] * CLC_T, ey = path_r[k] * CLC_T;
        bool clear = true;
        int dd = imax(iabs(ex - x), iabs(ey - y));
        for (int sstep = 0; sstep <= dd && clear; sstep += 2) {
            int qx = x + (ex - x) * sstep / imax(1, dd), qy = y + (ey - y) * sstep / imax(1, dd);
            if (chm_flight_blocked(5, clc_map_solid, w, qx * 256, qy * 256)) clear = false;
        }
        if (clear || k == 0) { tx = ex; ty = ey; break; }
    }
    int left = fld[r][c] == INF ? 9999 : fld[r][c] * CLC_T / 10;
    if (left < 24 || fld[r][c] == INF) { tx = gx; ty = gy; }
    int dx = tx - x, dy = ty - y, d = clc_isqrt(dx * dx + dy * dy);
    /* faster in the open, gentle near the walls */
    int k = clear_[r][c];
    int speed = k >= 5 ? 380 : k == 4 ? 300 : k == 3 ? 240 : k == 2 ? 160 : 110;
    if (arrive) speed = imin(speed, 30 + (iabs(gx - x) + iabs(gy - y)) * 5);
    int dvx = 0, dvy = 0;
    if (d > 0) {
        dvx = dx * speed / d;
        dvy = dy * speed / d;
    }
    if (dvy < -200) dvy = -200;
    *odvx = dvx;
    *odvy = dvy;
}

static unsigned slash_near(const ClcWorld *w, const ClcPlayer *p, unsigned b, bool hunt_all);

/* Look ahead: the line along the field, and eight others round it, each
 * flown for 20 frames on a copy of the map (enemies, shots and all); the
 * one that gets nearest the goal unhurt wins and is kept for a few frames. */
static ClcWorld simW;
static ClcPlayer simWP;
static int safe_plan, safe_f, safe_left, safe_dvx, safe_dvy;

static unsigned safe_steer(const ClcWorld *w, const ClcPlayer *p, int dvx, int dvy, bool use_field);

static unsigned fly_to(const ClcWorld *w, const ClcPlayer *p, int gx, int gy, bool arrive) {
    int dvx, dvy;
    desired(w, gx, gy, arrive, &dvx, &dvy);
    return safe_steer(w, p, dvx, dvy, true);
}

static int hunt_x, hunt_y;

/* the plans: the line plus eight nudges round it, then (by loose stones)
 * straight back, and two baits: on for a while to bring a stone down, then
 * straight back out from under it */
static const int16_t OFF[9][2] = {{0, 0}, {170, 0}, {-170, 0}, {0, -190}, {0, 190}, {130, -130}, {-130, -130}, {130, 130}, {-130, 130}};

static unsigned plan_steer(const ClcWorld *w, int j, int f, int dvx, int dvy) {
    if (j < 9) return steer(w, dvx + OFF[j][0], dvy + OFF[j][1]);
    int on = j == 10 ? 10 : j == 11 ? 18 : 0;
    if (f < on) return steer(w, dvx, dvy);
    int d = imax(1, clc_isqrt(dvx * dvx + dvy * dvy));
    return steer(w, -dvx * 260 / d, -dvy * 260 / d);
}

static int count_fallers(const ClcWorld *w) {
    int n = 0;
    for (int i = 0; i < w->ne; i++) n += w->e[i].on && w->e[i].kind == EK_FALLER;
    return n;
}

static unsigned safe_steer(const ClcWorld *w, const ClcPlayer *p, int dvx, int dvy, bool use_field) {
    if (safe_left <= 0) {
        int best = -(1 << 30), bo = 0;
        /* look further ahead near the Cellars' loose stones */
        int horizon = 20;
        if (w->region == RG_CELLARS) {
            int sc = PX(w->f.x) / CLC_T, sr = PX(w->f.y) / CLC_T;
            for (int r = imax(0, sr - 10); r <= sr && horizon == 20; r++)
                for (int c = imax(0, sc - 4); c <= imin(w->w - 1, sc + 4); c++)
                    if (w->tile[r][c] == MT_LOOSE) { horizon = 40; break; }
            if (count_fallers(w)) horizon = 40; /* one on its way down */
        }
        int nplans = horizon > 20 ? 12 : 9, fall0 = count_fallers(w);
        if (clc_bot_debug > 3) {
            fprintf(stderr, "plan dv %d,%d field %d ship v %d,%d\n", dvx, dvy, use_field, (int)w->f.vx, (int)w->f.vy);
            for (int i = 0; i < w->ne; i++)
                if (w->e[i].on && iabs(PX(w->e[i].x - w->f.x)) < 40 && iabs(PX(w->e[i].y - w->f.y)) < 40)
                    fprintf(stderr, "  near kind %d at %d,%d flag %d a %d\n", w->e[i].kind, PX(w->e[i].x), PX(w->e[i].y), w->e[i].flag, w->e[i].a);
        }
        for (int j = 0; j < nplans; j++) {
            memcpy(&simW, w, sizeof simW);
            simWP = *p;
            simW.sim = 1;
            int score = 0;
            for (int f = 0; f < horizon; f++) {
                unsigned b = plan_steer(&simW, j, f, dvx, dvy);
                b = slash_near(&simW, &simWP, b, simW.timer_on);
                clc_world_step(&simW, &simWP, b);
                if (simW.ev & (CEV_HURT | CEV_BADLAND)) score -= 6000 - f * 60; /* worse than a kill is good */
                if (simW.ev & CEV_BUMP) score -= 500;
                if (simW.ev & CEV_DIE) { score -= 100000; break; }
                if (simW.on_foot) { score -= 400; break; }
                if (simW.leeches > w->leeches) score -= 300;
            }
            /* by the stones: and not left right under one that is coming down */
            if (horizon > 20 && !simWP.dead && !simW.on_foot)
                for (int f = 0; f < 25; f++) {
                    clc_world_step(&simW, &simWP, steer(&simW, 0, 0));
                    if (simW.ev & CEV_DIE) { score -= 50000; break; }
                }

            if (use_field) {
                int fv = field_at(fld, &simW, PX(simW.f.x), PX(simW.f.y));
                score -= fv == INF ? 30000 : fv;
            } else {
                score -= (iabs(PX(simW.f.x) - hunt_x) + iabs(PX(simW.f.y) - hunt_y)) * 6;
                score += (simW.kills - w->kills) * 2000;
            }
            if (j == 0) score += 15; /* the plain line, all else equal */
            if (j >= 10 && score > -3000 && count_fallers(&simW) > fall0) score += 400; /* a stone brought down safely */
            if (clc_bot_debug > 2) fprintf(stderr, " opt %d score %d ev %x at %d,%d\n", j, score, simW.ev, PX(simW.f.x), PX(simW.f.y));
            if (score > best) { best = score; bo = j; }
        }
        safe_plan = bo;
        safe_f = 0;
        safe_dvx = dvx;
        safe_dvy = dvy;
        safe_left = bo >= 9 ? horizon : 5; /* a way out from under a stone is flown right through */
    }
    safe_left--;
    /* the straight-back plans are flown as they were tried */
    if (safe_plan >= 9) return plan_steer(w, safe_plan, safe_f++, safe_dvx, safe_dvy);
    return plan_steer(w, safe_plan, safe_f++, dvx, dvy);
}

/* slash anything close in front (and turn to face it if it's right behind) */
static unsigned slash_near(const ClcWorld *w, const ClcPlayer *p, unsigned b, bool hunt_all) {
    int x = PX(w->f.x), y = PX(w->f.y);
    for (int i = 0; i < w->ne; i++) {
        const ClcEnt *e = &w->e[i];
        if (!e->on) continue;
        bool foe = foe_kind(e->kind) || e->kind == EK_POD;
        (void)hunt_all;
        if (!foe || (e->kind == EK_BLOB && e->flag)) continue;
        int dx = PX(e->x) - x, dy = PX(e->y) - y;
        if (iabs(dy) > 12 || iabs(dx) > 26) continue;
        int side = dx >= 0 ? 1 : -1;
        /* face it (the slash goes the way last steered), and slash the
         * moment the blade is ready */
        if (side != w->f.face && !clc_has(p, G_TWIN)) {
            b &= ~(unsigned)(BTN_LEFT | BTN_RIGHT);
            b |= side > 0 ? BTN_RIGHT : BTN_LEFT;
        }
        if (!w->slash_cd && !((w->sim ? w->prev : B.prevb) & BTN_B)) b |= BTN_B;
        break;
    }
    return b;
}

/* something dangerous close to where Clary would step out */
static int danger_near(const ClcWorld *w, int x, int y, int range) {
    for (int i = 0; i < w->ne; i++) {
        const ClcEnt *e = &w->e[i];
        if (!e->on) continue;
        bool foe = foe_kind(e->kind) || (e->kind == EK_POD && w->timer_on) || e->kind == EK_FAKENOTE;
        if (!foe || (e->kind == EK_BLOB && e->flag) || B.ignore[i]) continue;
        /* the ones that shoot reach further */
        int k = e->kind, rr = range;
        if (k == EK_EYE || k == EK_BOOMER || k == EK_POD || k == EK_CHASER || k == EK_FIREDRONE) rr = range * 5 / 2;
        int dx = PX(e->x) - x, dy = PX(e->y) - (y - 4);
        if (iabs(dx) >= rr || iabs(dy) >= rr) continue;
        /* only what can see the spot (boomerangs fly through rock) */
        if (k != EK_BOOMER && k != EK_GHOST) {
            bool seen = true;
            int n = imax(iabs(dx), iabs(dy));
            for (int s2 = 4; s2 < n - 4 && seen; s2 += 3)
                if (clc_map_solid(w, x + dx * s2 / n, y - 4 + dy * s2 / n)) seen = false;
            if (!seen) continue;
        }
        return i;
    }
    return -1;
}

/* a shot flying near where Clary would walk */
static bool shots_near(const ClcWorld *w, int x, int y, int range) {
    for (int k = 0; k < CLC_SHOTS; k++) {
        const ClcShot *s = &w->shot[k];
        if (!s->on || s->kind < SH_PELLET) continue;
        if (iabs(PX(s->x) - x) < range && iabs(PX(s->y) - y) < range) return true;
    }
    return false;
}

static unsigned land_at(const ClcWorld *w, const ClcPlayer *p, int k) {
    int px, py;
    clc_door_pad(&w->door[k], &px, &py);
    int x = PX(w->f.x), y = PX(w->f.y);
    if (iabs(x - px) <= 2 && y >= py - 40 && y <= py + 1 && iabs(w->f.vx) <= 60) {
        /* straight down, slowly */
        B.land_t++;
        int dvx = iclamp((px - x) * 20, -40, 40);
        return steer(w, dvx, 110);
    }
    if (y >= py - 30 && y <= py + 1 && iabs(x - px) < 48) {
        /* level with the spot already: slide across (the floor there is open) */
        int dvx = iclamp((px - x) * 12, -96, 96); /* below what a knock on a wall costs */
        return steer(w, dvx, y < py - 14 ? 40 : -10);
    }
    return fly_to(w, p, px, py - 10, true);
}

/* where to hover to slash e: beside it, and clear of the floor or ceiling
 * it crawls on */
static int strike_y(const ClcWorld *w, const ClcEnt *e) {
    int ey = PX(e->y);
    if (e->kind == EK_GRUB || e->kind == EK_SNAP || (e->kind == EK_CREEPER && e->dir >= 0)) return ey - 8;
    if (e->kind == EK_CREEPER && e->dir < 0) return ey + 8;
    (void)w;
    return ey;
}

/* an enemy close enough to hurt, that the ship can get at */
static int threat_near(const ClcWorld *w, int x, int y, int range) {
    int best = -1, bd = range * range;
    for (int i = 0; i < w->ne; i++) {
        const ClcEnt *e = &w->e[i];
        if (!e->on || B.ignore_h[i]) continue;
        int k = e->kind;
        bool t = k == EK_CHASER || k == EK_FLITTER || k == EK_GHOST || k == EK_CRAB || k == EK_SNAKE ||
                 k == EK_WORM || k == EK_FIREDRONE || k == EK_WINDDRONE || k == EK_BOOMER || k == EK_EYE || k == EK_LEECH ||
                 k == EK_BMISSILE || k == EK_NEST || k == EK_WALLEYE || k == EK_GRUB || k == EK_SNAP || (k == EK_BLOB && !e->flag) || (k == EK_POD && (w->timer_on || iabs(PX(e->x) - x) + iabs(PX(e->y) - y) < 40));
        if (!t) continue;
        int dx = PX(e->x) - x, dy = PX(e->y) - y, dd = dx * dx + dy * dy;
        if (dd >= bd) continue;
        /* the ship can fly straight at it */
        bool clear = true;
        int n = imax(iabs(dx), iabs(dy));
        for (int s2 = 3; s2 < n - 10 && clear; s2 += 3)
            if (chm_flight_blocked(4, clc_map_solid, w, (x + dx * s2 / n) * 256, (y + dy * s2 / n) * 256)) clear = false;
        if (!clear) continue;
        bd = dd;
        best = i;
    }
    return best;
}

/* a coin or a plum lying close by, in sight */
static int coin_near(const ClcWorld *w, int x, int y, int range) {
    int best = -1, bd = range * range;
    for (int i = 0; i < w->ne; i++) {
        const ClcEnt *e = &w->e[i];
        if (!e->on || (e->kind != EK_COIN && e->kind != EK_PLUM) || B.ignore_h[i]) continue;
        int dx = PX(e->x) - x, dy = PX(e->y) - y, dd = dx * dx + dy * dy;
        if (dd >= bd) continue;
        bool clear = true;
        int n = imax(iabs(dx), iabs(dy));
        for (int s2 = 4; s2 < n - 4 && clear; s2 += 4)
            if (clc_map_solid(w, x + dx * s2 / n, y + dy * s2 / n)) clear = false;
        if (!clear) continue;
        bd = dd;
        best = i;
    }
    return best;
}

static unsigned fly_bot_(const ClcBotView *v);

/* gentler by the walls (a knock above 0.4 px a frame costs a point) */
static int wall_cap(const ClcWorld *w, int sp) {
    int c, r;
    cell_of(w, PX(w->f.x), PX(w->f.y), &c, &r);
    int k = clear_[r][c];
    return imin(sp, k >= 4 ? 400 : k == 3 ? 190 : k == 2 ? 120 : 90);
}

/* no headway at all for a few seconds: a shove in some direction */
static unsigned fly_bot(const ClcBotView *v) {
    const ClcWorld *w = v->w;
    int x = PX(w->f.x), y = PX(w->f.y);
    if (iabs(x - B.still_x) > 6 || iabs(y - B.still_y) > 6) { B.still_x = x; B.still_y = y; B.still_t = 0; }
    else B.still_t++;
    if (B.nudge_t > 0) {
        B.nudge_t--;
        static const int16_t N[4][2] = {{0, -200}, {200, -60}, {-200, -60}, {0, 160}};
        hunt_x = PX(w->f.x) + N[B.nudge_k & 3][0] / 4;
        hunt_y = PX(w->f.y) + N[B.nudge_k & 3][1] / 4;
        clc_bot_mode = 6;
        return safe_steer(w, v->p, N[B.nudge_k & 3][0], N[B.nudge_k & 3][1], false);
    }
    if (B.still_t > 200 && !w->on_foot) {
        B.still_t = 0;
        B.nudge_t = 30;
        B.nudge_k++;
        B.fx = -999; /* and a fresh field afterwards */
    }
    return fly_bot_(v);
}

static unsigned fly_bot_(const ClcBotView *v) {
    const ClcWorld *w = v->w;
    const ClcPlayer *p = v->p;
    int x = PX(w->f.x), y = PX(w->f.y);
    /* goals that are done */
    if (B.gk == GK_NOTE && (!w->e[B.gi].on || w->e[B.gi].kind != EK_NOTE)) B.gk = GK_NONE;
    if (B.gk == GK_DOOR && w->timer_on && w->door[B.gi].type != DT_GOLD && w->kind == WK_GEN) B.gk = GK_NONE;
    if (B.gk == GK_HUNT && (!w->e[B.gi].on || ++B.hunt_t > 150)) B.gk = GK_NONE;
    if (B.replan-- <= 0 && B.gk != GK_DOOR) { B.replan = 240; choose_goal(v); }
    if (B.gk == GK_NONE) choose_goal(v);
    unsigned b = 0;
    /* something coming for the ship: deal with it first */
    int h = threat_near(w, x, y, p->hp <= 6 ? 34 : 76);
    bool setting_down = B.gk == GK_DOOR && iabs(x - B.gx) < 12 && iabs(y - B.gy) < 24;
    if (h >= 0 && !setting_down) {
        const ClcEnt *e = &w->e[h];
        if (++B.hunt_n[h] > 240) B.ignore_h[h] = 1;
        int ex = PX(e->x);
        int off = (e->kind == EK_SNAKE || e->kind == EK_WORM) ? 15 : 14;
        if (e->kind == EK_CHASER || e->kind == EK_BMISSILE) off = 22; /* they come to the ship: wait for them, blade out */
        hunt_x = ex + (x < ex ? -off : off);
        hunt_y = strike_y(w, e);
        int dx = hunt_x - x, dy = hunt_y - y, d = imax(1, clc_isqrt(dx * dx + dy * dy));
        int sp = wall_cap(w, imin(240, 40 + d * 6));
        clc_bot_mode = 1;
        if (clc_bot_debug > 3) fprintf(stderr, "hunt k%d ship %d,%d v %d,%d foe %d,%d hp %d slash %d cd %d face %d\n", e->kind, x, y, (int)w->f.vx, (int)w->f.vy, PX(e->x), PX(e->y), e->hp, w->slash_t, w->slash_cd, w->f.face);
        b = safe_steer(w, p, dx * sp / d, dy * sp / d, false);
        return slash_near(w, p, b, w->timer_on);
    }
    /* coins close by are worth a short detour (not in the dash) */
    int cn = (!w->timer_on && !setting_down) ? coin_near(w, x, y, 56) : -1;
    if (cn >= 0) {
        const ClcEnt *e = &w->e[cn];
        if (++B.hunt_n[cn] > 180) B.ignore_h[cn] = 1;
        hunt_x = PX(e->x);
        hunt_y = PX(e->y) - 2;
        int dx = hunt_x - x, dy = hunt_y - y, d = imax(1, clc_isqrt(dx * dx + dy * dy));
        int sp = wall_cap(w, imin(200, 40 + d * 6));
        clc_bot_mode = 2;
        b = safe_steer(w, p, dx * sp / d, dy * sp / d, false);
        return slash_near(w, p, b, false);
    }
    switch (B.gk) {
    case GK_NOTE: case GK_TOP: case GK_HUNT: {
        if (B.gk == GK_HUNT) {
            const ClcEnt *e = &w->e[B.gi];
            B.gx = PX(e->x) + (x < PX(e->x) ? -14 : 14);
            B.gy = PX(e->y);
        }
        clc_bot_mode = 3;
        b = fly_to(w, p, B.gx, B.gy, B.gk == GK_HUNT);
        int d = field_at(fld, w, x, y);
        B.frames++;
        if (d < B.best_d) { B.best_d = d; B.frames = 0; }
        if (B.frames > 240) {
            /* no headway: try something else for a while */
            if (B.gk == GK_NOTE) B.skip_note[B.gi] = 1;
            B.gk = GK_NONE;
        }
        break;
    }
    case GK_DOOR: {
        int px, py;
        clc_door_pad(&w->door[B.gi], &px, &py);
        /* clear the landing spot of anything that would catch Clary outside */
        int dz = (w->timer_on && w->timer < 840) ? -1 : danger_near(w, px, py, 56);
        if (dz >= 0 && iabs(x - px) < 120 && iabs(y - py) < 90) {
            const ClcEnt *e = &w->e[dz];
            int side = x < PX(e->x) ? -14 : 14;
            /* give up on one that can't be got at */
            /* give up only on one that isn't right on the landing spot */
            bool on_spot = iabs(PX(e->x) - px) < 44 && iabs(PX(e->y) - py) < 20;
            if (++B.clear_t[dz] > (w->timer_on ? 240 : 400) && !on_spot) B.ignore[dz] = 1;
            if (clc_bot_debug > 1 && (w->t % 10) == 0)
                fprintf(stderr, "clearing kind %d at %d,%d hp %d ship %d,%d face %d slash %d t%d\n", e->kind, PX(e->x), PX(e->y), e->hp, x, y,
                        w->f.face, w->slash_t, B.clear_t[dz]);
            clc_bot_mode = 4;
            b = fly_to(w, p, PX(e->x) + side, strike_y(w, e), true);
            b = slash_near(w, p, b, true);
            return b;
        }
        /* the last moment before setting down: not with shots about */
        if (shots_near(w, px, py - 4, 64) && iabs(x - px) < 24 && y > py - 30 && (!w->timer_on || w->timer > 600))
            return steer(w, 0, -60);
        clc_bot_mode = 5;
        b = land_at(w, p, B.gi);
        if (B.land_t > 240) { B.land_t = 0; B.fx = -999; }
        B.frames++;
        if (B.frames > 2400 && !w->timer_on) { B.visited[B.gi] = 1; B.gk = GK_NONE; }
        break;
    }
    default:
        b = steer(w, 0, 0);
        break;
    }
    if (B.gk != GK_DOOR || iabs(x - B.gx) > 30) b = slash_near(w, p, b, w->timer_on);
    return b;
}

/* ---- on foot outside -------------------------------------------------------------- */

static unsigned walk_to(int x, int tx) {
    if (tx > x + 1) return BTN_RIGHT;
    if (tx < x - 1) return BTN_LEFT;
    return 0;
}

/* on foot outside one touch is the end: keep going, and shoot whatever is
 * ahead at her height */
static unsigned foot_guard(const ClcWorld *w, int x, int y, unsigned walk) {
    /* a shot on its way: back into the ship if it's right here */
    if (shots_near(w, x, y - 4, 40) && iabs(x - PX(w->f.x)) <= 6 && w->cl.ground) return (B.prevb & BTN_UP) ? 0 : BTN_UP;
    for (int i = 0; i < w->ne; i++) {
        const ClcEnt *e = &w->e[i];
        if (!e->on || !(foe_kind(e->kind) || (e->kind == EK_POD && w->timer_on))) continue;
        int dx = PX(e->x) - x, dy = PX(e->y) - (y - 4);
        if (iabs(dx) < 70 && iabs(dy) < 10 && (dx > 0) == (w->cl.face > 0)) {
            if (!(B.prevb & BTN_B)) walk |= BTN_B;
            break;
        }
    }
    return walk;
}

static unsigned foot_bot(const ClcBotView *v) {
    const ClcWorld *w = v->w;
    int x = PX(w->cl.x), y = PX(w->cl.y);
    unsigned b = 0;
    int k = B.gk == GK_DOOR ? B.gi : -1;
    if (k >= 0) {
        const ClcDoor *d = &w->door[k];
        bool same_floor = y == (d->r + 1) * CLC_T;
        bool open = !(w->timer_on && d->type != DT_GOLD && w->kind == WK_GEN) && !d->hidden;
        if (same_floor && open && !B.visited[k]) {
            int dx = d->c * CLC_T + CLC_T / 2;
            if (iabs(x - dx) <= 2 && w->cl.ground) {
                if (!(B.prevb & BTN_UP)) {
                    b = BTN_UP;
                    B.visited[k] = 1;
                    B.gk = GK_NONE;
                }
                return b;
            }
            return foot_guard(w, x, y, walk_to(x, dx));
        }
    }
    /* back to the ship and aboard */
    int sx = PX(w->f.x);
    if (iabs(x - sx) <= 3 && w->cl.ground) return (B.prevb & BTN_UP) ? 0 : BTN_UP;
    return foot_guard(w, x, y, walk_to(x, sx));
}

/* ---- behind a door ------------------------------------------------------------------- */

static ClcSub simS;
static ClcPlayer simP;

static int count_items(const ClcSub *s) {
    int n = 0;
    for (int i = 0; i < s->ne; i++) n += s->e[i].on && s->e[i].kind == EK_ITEM;
    return n;
}

static const ClcEnt *sub_find(const ClcSub *s, int kind) {
    for (int i = 0; i < s->ne; i++)
        if (s->e[i].on && s->e[i].kind == kind) return &s->e[i];
    return NULL;
}

/* where the bot wants Clary to be behind a door, and whether to press UP there */
static int sub_goal(const ClcBotView *v, bool *door_up, int *want_item_x) {
    const ClcSub *s = v->s;
    const ClcPlayer *p = v->p;
    *door_up = false;
    *want_item_x = -1;
    int door_x = s->door_c * CLC_ST + 8, exit_x = s->exit_c * CLC_ST + 8;
    switch (s->room) {
    case RM_CAVE: {
        int cx = PX(s->cl.x), cy = PX(s->cl.y);
        for (int i = 0; i < s->ne; i++) {
            const ClcEnt *e = &s->e[i];
            if (e->on && e->kind == EK_SCOIN && iabs(PX(e->x) - cx) < 64 && iabs(PX(e->y) - cy) < 20 && e->b > 20) return PX(e->x);
        }
        const ClcEnt *ch = sub_find(s, EK_CHEST);
        if (ch && !ch->flag) return PX(ch->x);
        /* the chest's three: the key first, then whatever is worth most */
        const ClcEnt *it = NULL;
        int best = -1;
        for (int i = 0; i < s->ne; i++) {
            const ClcEnt *e = &s->e[i];
            if (!e->on || e->kind != EK_ITEM || e->c == 2) continue;
            int a = e->a, sc = 10;
            if (a == IT_KEY) sc = 100;
            else if (a == G_PLATE) sc = 90;
            else if (a == G_SIPHON) sc = 80;
            else if (a == G_PURSE || a == G_TWIN) sc = 70;
            else if (a == IT_DRUM || a == IT_SPARETANK || a == IT_HEARTPIN) sc = 60;
            else if (a == IT_TOFFEE && p->hp < p->hpmax - 4) sc = 65;
            else if (a == IT_FLASK) sc = 50;
            else if (a < G_COUNT) sc = 40;
            if (sc > best) { best = sc; it = e; }
        }
        if (it) { *want_item_x = PX(it->x); return PX(it->x); }
        *door_up = true;
        return exit_x;
    }
    case RM_GOLDCAVE: {
        const ClcEnt *lob = sub_find(s, EK_LOBBER), *eng = sub_find(s, EK_ENGINE);
        if (lob) return PX(lob->x) - 44;
        if (eng && !s->done) return PX(eng->x) - 56;
        const ClcEnt *it = sub_find(s, EK_ITEM);
        if (it && PX(it->x) > PX(s->cl.x) - 40) return PX(it->x);
        *door_up = true;
        return exit_x;
    }
    case RM_HUSH: {
        const ClcEnt *h = sub_find(s, EK_HUSH);
        if (h && !s->done) return PX(h->x);
        *door_up = true;
        return exit_x;
    }
    case RM_TOCK: {
        const ClcEnt *t = sub_find(s, EK_TOCK);
        if (t && !s->done) return PX(t->x);
        *door_up = true;
        return exit_x;
    }
    case RM_NPC:
    case RM_SAGE: {
        const ClcEnt *n = sub_find(s, EK_NPC);
        if (n && !s->talk && !(n->b)) return PX(n->x) - 16;
        *door_up = true;
        return door_x;
    }
    case RM_SHOP: case RM_MEGA: case RM_HEALTH: case RM_CHARMSHOP: case RM_ARMOR: {
        if (!s->bought && B.want_item < 0) {
            /* what to buy: the charm, then toffee if hurt, then fuel */
            int pick = -1, pick_score = 0;
            for (int i = 0; i < s->ne; i++) {
                const ClcEnt *e = &s->e[i];
                if (!e->on || e->kind != EK_ITEM) continue;
                if (e->b > p->coins) continue;
                int sc = 0;
                if (e->a == G_CHARM) sc = 100;
                else if (e->a == G_PLATE) sc = 90;
                else if (e->a == IT_TOFFEE && p->hp <= p->hpmax - 4) sc = p->hp <= 6 || p->fuel >= 400 ? 80 : 50;
                else if (e->a == IT_DRUM && p->fuel < p->fuelmax - 900) sc = p->fuel < 300 ? 95 : 60;
                else if (e->a == IT_FLASK && p->fuel < p->fuelmax - 400) sc = p->fuel < 300 ? 92 : 70;
                else if (e->a == IT_HEARTPIN) sc = 75;
                else if (e->a == G_SIPHON || e->a == G_PURSE) sc = 20;
                if (sc > pick_score) { pick_score = sc; pick = i; }
            }
            B.want_item = pick >= 0 ? pick : 255;
        }
        if (B.want_item >= 0 && B.want_item < 255 && s->e[B.want_item].on && s->e[B.want_item].kind == EK_ITEM && !s->bought) {
            *want_item_x = PX(s->e[B.want_item].x);
            return *want_item_x;
        }
        *door_up = true;
        return door_x;
    }
    default:
        *door_up = true;
        return door_x;
    }
}

/* the buttons a plan presses on frame f of 24 */
enum { PL_DIR = 3, PL_JUMP = 4, PL_UP = 32, PL_CROUCH = 64 };
static unsigned plan_buttons(int dir, int jump, int up, int crouch, int f, int parity) {
    unsigned b = 0;
    if (dir > 0) b |= BTN_RIGHT;
    if (dir < 0) b |= BTN_LEFT;
    if (jump > 0 && f < jump) b |= BTN_A;
    if (jump < 0 && f >= -jump && f < -jump + 12) b |= BTN_A; /* a later jump */
    if (up) b |= BTN_UP;
    if (crouch) b |= BTN_DOWN;
    if (((f + parity) & 1) == 0) b |= BTN_B;
    return b;
}

static int hurt_cost = 4000;

static int score_plan(const ClcBotView *v, int goal_x, int dir, int jump, int up, int crouch, int parity) {
    const ClcSub *s = v->s;
    memcpy(&simS, s, sizeof simS);
    simP = *v->p;
    simS.sim = 1;
    int score = 0, items = count_items(&simS), boss0 = simS.boss_hp;
    int lob0 = 0, eng0 = 0;
    const ClcEnt *lob = sub_find(&simS, EK_LOBBER), *eng = sub_find(&simS, EK_ENGINE);
    if (lob) lob0 = lob->hp;
    if (eng) eng0 = eng->hp;
    for (int f = 0; f < 30; f++) {
        clc_sub_step(&simS, &simP, plan_buttons(dir, jump, up, crouch, f, parity));
        if (simS.ev & CEV_HURT) score -= hurt_cost - f * (hurt_cost / 70);
        if (simS.ev & CEV_DIE) { score -= 200000; break; }
        if (simS.ev & CEV_KILL) score += 60;
        if (simS.leave) { score -= 50000; break; } /* never leave by accident */
    }
    int x = PX(simS.cl.x);
    score -= iabs(goal_x - x) * 6;
    if (count_items(&simS) < items) score += 3000;
    const ClcEnt *ch0 = sub_find(s, EK_CHEST), *ch1 = sub_find(&simS, EK_CHEST);
    if (ch0 && !ch0->flag && ch1 && ch1->flag) score += 3000; /* the chest opened */
    score += (boss0 - simS.boss_hp) * 80;
    lob = sub_find(&simS, EK_LOBBER);
    eng = sub_find(&simS, EK_ENGINE);
    score += (lob0 - (lob ? lob->hp : 0)) * 120 + (eng0 - (eng ? eng->hp : 0)) * 120;
    if (s->room == RM_TOCK) {
        const ClcEnt *t = sub_find(&simS, EK_TOCK);
        if (t && PX(simS.cl.y) == PX(t->y) - 32) score += 900;
    }
    if (s->room == RM_HUSH) {
        const ClcEnt *h = sub_find(&simS, EK_HUSH);
        if (h && up) score += 100;
    }
    return score;
}

static unsigned sub_bot(const ClcBotView *v) {
    const ClcSub *s = v->s;
    if (s->ver != B.sub_ver || s->room != B.sub_room) {
        B.sub_ver = s->ver;
        B.sub_room = s->room;
        B.sub_t = 0;
        B.want_item = -1;
        B.plan_left = 0;
    }
    B.sub_t++;
    bool door_up;
    int want_x;
    int gx = sub_goal(v, &door_up, &want_x);
    int x = PX(s->cl.x);
    if (clc_bot_debug > 2) fprintf(stderr, "subbot room %d x %d goal %d door_up %d want_x %d want %d prev %x\n", s->room, x, gx, door_up, want_x, B.want_item, s->prev);
    bool fight = s->room == RM_CAVE || s->room == RM_GOLDCAVE || s->room == RM_HUSH || s->room == RM_TOCK ||
                 s->room == RM_TRIAL || s->room == RM_CURSED;
    /* at the goal */
    if (door_up && iabs(x - gx) <= 4 && s->cl.ground) {
        B.plan_left = 0;
        return (B.prevb & BTN_UP) ? 0 : BTN_UP;
    }
    if (want_x >= 0 && iabs(x - want_x) <= 3 && s->cl.ground) return (B.prevb & BTN_A) ? 0 : BTN_A;
    if (door_up && iabs(x - gx) < 30 && s->cl.ground) {
        /* the last few steps to a door: walk them */
        B.plan_left = 0;
        unsigned b = gx > x ? BTN_RIGHT : BTN_LEFT;
        if (((s->t) & 1) == 0) b |= BTN_B;
        return b;
    }
    if (!fight) {
        /* rooms: just walk (no jumping, no shooting near the wares) */
        if (gx > x + 2) return BTN_RIGHT;
        if (gx < x - 2) return BTN_LEFT;
        return 0;
    }
    if (B.plan_left > 0) {
        B.plan_left--;
        return B.plan[B.plan_at++];
    }
    /* look ahead: every way to move for the next 30 frames. Stuck behind
     * something for long, a hit becomes worth taking to get past it. */
    if (iabs(x - B.cave_x) > 8 || s->room != B.sub_room) { B.cave_x = x; B.cave_t = 0; }
    else B.cave_t++;
    hurt_cost = B.cave_t > 900 ? 300 : 4000;
    static const int8_t DIRS[3] = {1, 0, -1};
    static const int8_t JUMPS[4] = {0, 6, 22, -10};
    int best = -(1 << 30), bd = 0, bj = 0, bu = 0, bc = 0;
    int parity = (int)(s->t & 1);
    bool near_door = iabs(x - (s->door_c * CLC_ST + 8)) < 28 || (s->exit_c >= 0 && iabs(x - (s->exit_c * CLC_ST + 8)) < 28);
    for (int di = 0; di < 3; di++)
        for (int ji = 0; ji < 4; ji++)
            for (int ui = 0; ui < 2; ui++) {
                if (ui && near_door) continue;
                if (ui && ji == 3) continue;
                int sc = score_plan(v, gx, DIRS[di], JUMPS[ji], ui, 0, parity);
                if (di == 0 && ji == 0 && ui == 0) sc -= 5; /* prefer to keep going */
                if (sc > best) { best = sc; bd = DIRS[di]; bj = JUMPS[ji]; bu = ui; bc = 0; }
            }
    if (!near_door) {
        int sc = score_plan(v, gx, 0, 0, 0, 1, parity);
        if (sc > best) { best = sc; bd = 0; bj = 0; bu = 0; bc = 1; }
    }
    int keep = 4;
    for (int f = 0; f < keep; f++) B.plan[f] = plan_buttons(bd, bj, bu, bc, f, parity);
    B.plan_at = 1;
    B.plan_left = keep - 1;
    if (clc_bot_debug) fprintf(stderr, "sub t%d x%d goal%d best %d dir%d jump%d up%d crouch%d\n", B.sub_t, x, gx, best, bd, bj, bu, bc);
    return B.plan[0];
}

/* ---- the entry ------------------------------------------------------------------------ */

static unsigned clc_bot_(const ClcBotView *v);

/* the buttons pressed last frame, as the bot pressed them (the game may not
 * have seen a held one: a button held through a change of screen counts
 * only once it has been let go) */
unsigned clc_bot(const ClcBotView *v) {
    unsigned b = clc_bot_(v);
    B.prevb = b;
    return b;
}

static unsigned clc_bot_(const ClcBotView *v) {
    if (v->mode == 1) return sub_bot(v);
    const ClcWorld *w = v->w;
    if (w != cw || w->region != cw_region || w->area != cw_area || w->kind != cw_kind || (uint16_t)(w->ver - cw_ver) > 40)
        new_world(w);
    if (v->p->dead) return 0;
    unsigned b = w->on_foot ? foot_bot(v) : fly_bot(v);
    if (clc_bot_debug > 1 && (w->t % 60) == 0)
        fprintf(stderr, "map t%u %s x%d y%d goal %d/%d at %d,%d fuel %d hp %d notes %d\n", (unsigned)w->t,
                w->on_foot ? "foot" : "ship", PX(w->on_foot ? w->cl.x : w->f.x), PX(w->on_foot ? w->cl.y : w->f.y), B.gk, B.gi,
                B.gx, B.gy, v->p->fuel, v->p->hp, w->notes);
    target = B.door;
    return b;
}
