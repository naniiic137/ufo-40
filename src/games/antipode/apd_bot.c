/* ANTIPODE - the demo player, for the tests. It presses real buttons.
 *
 * Above the rooms it plans like a player with the map: from what it has
 * (mods, pass level, ciphers, gates opened) it works out which rooms it can
 * reach with each way of fitting its mods, picks the nearest thing worth
 * doing (an upgrade, a pass terminal, a boss, a room not seen yet), and if
 * that needs the mods fitted differently it goes to a cradle first and fits
 * them. Inside a room it looks ahead: it runs the real game on a copy with a
 * couple of dozen button plans (walk, jump, flip, in different rhythms) and
 * keeps the one that ends nearest its goal with the least harm taken. Gates
 * (barrels, cracked concrete, targets in walls, the hidden room's buttons)
 * get small scripted moves of their own. */
#include "apd.h"

int apd_bot_goal;
int apd_bot_plan;
int apd_bot_log;
int apd_max_armor(void);
int apd_tele_dest(int link, int from_room);

/* ---- loadouts ------------------------------------------------------------------ */
typedef struct { uint8_t s[APD_MODS]; } Lo;

static Lo cur_lo(void) {
    Lo L;
    memcpy(L.s, ag.run.mod_sys, APD_MODS);
    return L;
}
static bool lo_eq(const Lo *a, const Lo *b) { return !memcmp(a->s, b->s, APD_MODS); }
static bool lo_has(const Lo *L, int m, int s) { return L->s[m] == s; }

/* what an overloaded system costs in the planning (a mod can't be left out:
 * with four mods and no damper, something overloads) */
static int lo_over_cost(const Lo *L) {
    int n[APD_SYS] = {0, 0, 0, 0};
    for (int m = 0; m < APD_MODS; m++)
        if (L->s[m] != MOD_NONE) n[L->s[m]]++;
    static const int COST[APD_SYS] = {0, 45, 12, 25};
    int c = 0;
    for (int s = SYS_FLIP; s < APD_SYS; s++)
        if (n[s] > apd_safe_slots()) c += COST[s] * (n[s] - apd_safe_slots());
    return c;
}

/* how good a loadout is for a fight, to break ties */
static int lo_combat(const Lo *L) {
    int v = 0;
    if (lo_has(L, MOD_EMBER, SYS_GUN)) v += 4;
    if (lo_has(L, MOD_MASS, SYS_FLIP)) v += 4;
    if (lo_has(L, MOD_VOLT, SYS_FLIP)) v += 3;
    if (lo_has(L, MOD_TWIN, SYS_FRAME)) v += 1;
    if (lo_has(L, MOD_VOLT, SYS_FRAME)) v += 1;
    if (lo_has(L, MOD_MASS, SYS_FRAME)) v += 2;
    if (lo_has(L, MOD_EMBER, SYS_FRAME)) v += 2;
    return v;
}

/* ---- the room graph -------------------------------------------------------------- */
static int8_t tele_of[APD_MAX_ROOMS];
static bool graph_ready;

static void graph_init(void) {
    if (graph_ready) return;
    graph_ready = true;
    for (int r = 0; r < apd_nrooms; r++) {
        tele_of[r] = -1;
        const char *t = strstr(APD_SPECS[apd_rooms[r].spec].things, "tele");
        if (t) tele_of[r] = (int8_t)atoi(t + 4);
    }
}

static int boss_of_room(int r) {
    const char *t = strstr(APD_SPECS[apd_rooms[r].spec].things, "boss");
    return t ? atoi(t + 4) : -1;
}
static bool live_arena(int r) {
    int b = boss_of_room(r);
    return b >= 0 && !ag.run.boss_dead[b];
}

static bool door_ok(int d, const Lo *L) {
    const ApdDoor *dd = &apd_doors[d];
    bool open = ag.run.gate_open[d] != 0;
    switch (dd->type) {
    case E_DOOR: case E_BOSS: return true;
    case E_PASS1: case E_PASS2: case E_PASS3: return ag.run.pass >= dd->type - E_PASS1 + 1;
    case E_NANO: return open;
    case E_TARGET: return open || lo_has(L, MOD_VOLT, SYS_GUN);
    case E_BARREL: case E_HIDDEN: return open || lo_has(L, MOD_EMBER, SYS_GUN);
    case E_CRACK: return open || lo_has(L, MOD_MASS, SYS_FLIP);
    case E_ZAP: return lo_has(L, MOD_VOLT, SYS_FLIP);
    default: return false;
    }
}

static bool room_ok(int r, const Lo *L) {
    uint16_t f = APD_SPECS[apd_rooms[r].spec].flags;
    if ((f & RF_HEAT) && !lo_has(L, MOD_EMBER, SYS_FRAME)) return false;
    if ((f & RF_SPIKES) && !lo_has(L, MOD_MASS, SYS_FRAME)) return false;
    return true;
}

/* a leaf: an arena with its boss alive (entering starts the fight), or the Crown */
static bool leaf_room(int r) { return live_arena(r); }

static int16_t bd[APD_MAX_ROOMS], bp[APD_MAX_ROOMS], bv[APD_MAX_ROOMS];

static void bfs(int from, const Lo *L) {
    graph_init();
    static int16_t q[APD_MAX_ROOMS];
    for (int i = 0; i < apd_nrooms; i++) bd[i] = -1;
    int h = 0, t = 0;
    bd[from] = 0;
    bp[from] = -1;
    bv[from] = -1;
    q[t++] = (int16_t)from;
    while (h < t) {
        int r = q[h++];
        if (r != from && leaf_room(r)) continue;
        for (int d = 0; d < apd_ndoors; d++) {
            const ApdDoor *dd = &apd_doors[d];
            int n = dd->ra == r ? dd->rb : dd->rb == r ? dd->ra : -1;
            if (n < 0 || bd[n] >= 0 || !door_ok(d, L) || !room_ok(n, L)) continue;
            bd[n] = (int16_t)(bd[r] + 1);
            bp[n] = (int16_t)r;
            bv[n] = (int16_t)d;
            q[t++] = (int16_t)n;
        }
        if (tele_of[r] >= 0 && (tele_of[r] != 9 || ag.run.ciphers == 15)) {
            int n = apd_tele_dest(tele_of[r], r);
            if (n >= 0 && bd[n] < 0 && room_ok(n, L)) {
                bd[n] = (int16_t)(bd[r] + 2);
                bp[n] = (int16_t)r;
                bv[n] = (int16_t)(-2 - tele_of[r]);
                q[t++] = (int16_t)n;
            }
        }
    }
}

/* the first step from 'from' toward 'to' (after bfs(from, ...)): the room
 * entered next and how (a door, or -2 - link for a teleporter) */
static int first_hop(int from, int to, int *via) {
    if (bd[to] < 0 || to == from) return -1;
    int r = to;
    while (bp[r] != from) r = bp[r];
    *via = bv[r];
    return r;
}

/* ---- objectives ------------------------------------------------------------------ */
enum { OB_NONE, OB_ITEM, OB_PASS, OB_BOSS, OB_ROOM, OB_NANO, OB_FINAL };
typedef struct {
    int kind, room, arg;
    Lo lo;
    int pod_room;          /* a cradle to fit the mods at first (-1 none) */
} Obj;

static Obj ob;
static bool have_ob;
static int pod_done;       /* the loadout has been fitted for this objective */

static bool ob_done(const Obj *o) {
    switch (o->kind) {
    case OB_ITEM: return ag.run.item_got[o->arg] != 0;
    case OB_PASS: return ag.run.pass >= o->arg;
    case OB_BOSS: return ag.run.boss_dead[o->arg] && !ag.boss_on && ag.boss_dead_t == 0;
    case OB_ROOM: return ag.run.visited[o->room] != 0;
    case OB_NANO: return ag.run.gate_open[o->arg] != 0;
    case OB_FINAL: return ag.run.won != 0;
    default: return true;
    }
}

static int crown_f(void) { return apd_room_find(SEC_CROWN, 'F'); }
static int x_room(void) { return apd_room_find(SEC_BRINE, 'r'); } /* the quiet vault */

/* an upgrade sealed in cracked concrete: the ANVIL gets it out */
static bool item_in_crack(int item) {
    int r = apd_items[item].room;
    const ApdSpec *sp = &APD_SPECS[apd_rooms[r].spec];
    const char *p = sp->things;
    int k = item - apd_rooms[r].first_item, n = 0;
    char m = 0;
    while (*p) {
        while (*p == ' ') p++;
        const char *t = p;
        while (*p && *p != ' ') p++;
        bool it = !strncmp(t, "armor", 5) || !strncmp(t, "spare", 5) || !strncmp(t, "damper", 6) || (!strncmp(t, "mod", 3) && isdigit((unsigned char)t[3]));
        if (!it) continue;
        if (n++ == k) {
            const char *at = memchr(t, '@', (size_t)(p - t));
            if (at) m = at[1];
            break;
        }
    }
    if (!m) return false;
    int W = apd_rooms[r].w * APD_CW, H = apd_rooms[r].h * APD_CH;
    for (int y = 1; y < H - 1; y++)
        for (int x = 1; x < W - 1; x++)
            if (sp->rows[y][x] == m) return sp->rows[y][x - 1] == 'X' || sp->rows[y][x + 1] == 'X' || sp->rows[y - 1][x] == 'X' || sp->rows[y + 1][x] == 'X';
    return false;
}

/* the objectives still open, and what each needs fitted */
typedef struct { int kind, room, arg, need_mod, need_sys; } Cand;

static int candidates(Cand *c, int max) {
    int n = 0;
    for (int i = 0; i < apd_nitems && n < max; i++)
        if (!ag.run.item_got[i]) {
            Cand k = {OB_ITEM, apd_items[i].room, i, -1, -1};
            if (apd_items[i].room == x_room() && !ag.run.secret_open) { k.need_mod = MOD_TWIN; k.need_sys = SYS_FLIP; }
            if (item_in_crack(i)) { k.need_mod = MOD_MASS; k.need_sys = SYS_FLIP; }
            c[n++] = k;
        }
    for (int r = 0; r < apd_nrooms && n < max; r++) {
        const char *t = strstr(APD_SPECS[apd_rooms[r].spec].things, "pass");
        if (t && ag.run.pass < atoi(t + 4)) c[n++] = (Cand){OB_PASS, r, atoi(t + 4), -1, -1};
        int b = boss_of_room(r);
        if (b >= 0 && b != BS_HALLORAN && !ag.run.boss_dead[b]) c[n++] = (Cand){OB_BOSS, r, b, -1, -1};
        if (!ag.run.visited[r] && r != crown_f() && boss_of_room(r) < 0) c[n++] = (Cand){OB_ROOM, r, 0, -1, -1};
    }
    for (int d = 0; d < apd_ndoors && n < max; d++)
        if (apd_doors[d].type == E_NANO && !ag.run.gate_open[d]) {
            /* the side with the sentry orbs */
            int r = strstr(APD_SPECS[apd_rooms[apd_doors[d].ra].spec].things, "sentry") ? apd_doors[d].ra : apd_doors[d].rb;
            c[n++] = (Cand){OB_NANO, r, d, -1, -1};
        }
    if (n == 0 && !ag.run.won) c[n++] = (Cand){OB_FINAL, crown_f(), BS_HALLORAN, -1, -1};
    return n;
}

static int pod_rooms[APD_MAX_PODS], npodrooms;

static void list_pods(void) {
    npodrooms = 0;
    for (int r = 0; r < apd_nrooms; r++)
        if (apd_room_pod(r) >= 0 && npodrooms < APD_MAX_PODS) pod_rooms[npodrooms++] = r;
}

/* every way of fitting the mods held (each in FLIP, FRAME or GUN) */
static int all_loadouts(Lo *out, int max) {
    int n = 0;
    int owned[APD_MODS], no = 0;
    for (int m = 0; m < APD_MODS; m++)
        if (ag.run.mod_sys[m] != MOD_NONE) owned[no++] = m;
    int combos = 1;
    for (int k = 0; k < no; k++) combos *= 3;
    for (int c = 0; c < combos && n < max; c++) {
        Lo L;
        memset(L.s, MOD_NONE, sizeof L.s);
        int x = c;
        for (int k = 0; k < no; k++) { L.s[owned[k]] = (uint8_t)(SYS_FLIP + x % 3); x /= 3; }
        out[n++] = L;
    }
    return n;
}

/* rooms a little after things; bosses after everything near, and with the
 * best fit for a fight */
static int kind_rank(int k) { return k == OB_ROOM ? 1 : k == OB_BOSS ? 20 : 0; }
static int boss_fit(int k, const Lo *L) {
    if (k == OB_BOSS || k == OB_FINAL) return (14 - lo_combat(L)) * 6 + lo_over_cost(L);
    /* the ANVIL makes every flip a drop: no hovering. Only for the cracks. */
    return (lo_has(L, MOD_MASS, SYS_FLIP) ? 60 : 0) + lo_over_cost(L);
}

static void decide(void) {
    static Cand c[256];
    static Lo los[256];
    int nc = candidates(c, 256);
    int nl = all_loadouts(los, 256);
    Lo L0 = cur_lo();
    list_pods();
    int here = ag.room;
    have_ob = false;
    long best = 1L << 30;
    int best_rank = 9, best_comb = -1;
    /* with the mods as they are */
    static int16_t d0[APD_MAX_ROOMS];
    bfs(here, &L0);
    memcpy(d0, bd, sizeof d0);
    for (int i = 0; i < nc; i++) {
        if (d0[c[i].room] < 0) continue;
        if (c[i].need_mod >= 0 && !lo_has(&L0, c[i].need_mod, c[i].need_sys)) continue;
        long cost = d0[c[i].room] * 10 + kind_rank(c[i].kind) * 3 + boss_fit(c[i].kind, &L0);
        int comb = lo_combat(&L0);
        if (cost < best || (cost == best && comb > best_comb)) {
            best = cost; best_comb = comb; best_rank = kind_rank(c[i].kind);
            ob = (Obj){c[i].kind, c[i].room, c[i].arg, L0, -1};
            have_ob = true;
        }
    }
    (void)best_rank;
    /* or by way of a cradle, fitted differently */
    for (int p = 0; p < npodrooms; p++) {
        int pr = pod_rooms[p];
        if (d0[pr] < 0) continue;
        for (int l = 0; l < nl; l++) {
            if (lo_eq(&los[l], &L0)) continue;
            bfs(pr, &los[l]);
            for (int i = 0; i < nc; i++) {
                if (bd[c[i].room] < 0) continue;
                if (c[i].need_mod >= 0 && !lo_has(&los[l], c[i].need_mod, c[i].need_sys)) continue;
                long cost = (d0[pr] + bd[c[i].room]) * 10 + 25 + kind_rank(c[i].kind) * 3 + boss_fit(c[i].kind, &los[l]);
                int comb = lo_combat(&los[l]);
                if (cost < best || (cost == best && comb > best_comb)) {
                    best = cost; best_comb = comb;
                    ob = (Obj){c[i].kind, c[i].room, c[i].arg, los[l], pr};
                    have_ob = true;
                }
            }
        }
    }
    pod_done = ob.pod_room < 0;
    apd_bot_goal = have_ob ? ob.kind * 1000 + ob.room : -1;
    if (apd_bot_log && have_ob)
        fprintf(stderr, "[bot %6u] in %d%c: %s %d%c arg %d via pod %d (lo %d %d %d %d) items %d rooms %d%%\n",
                (unsigned)ag.run.frames, apd_rooms[here].sector, apd_rooms[here].id,
                (const char *[]){"-", "ITEM", "PASS", "BOSS", "ROOM", "NANO", "FINAL"}[ob.kind], apd_rooms[ob.room].sector,
                apd_rooms[ob.room].id, ob.arg, ob.pod_room, ob.lo.s[0], ob.lo.s[1], ob.lo.s[2], ob.lo.s[3], apd_items_got(),
                apd_explore_pct());
}

/* ---- the field: how far each spot is from the goal ------------------------------------ */
#define FC 8 /* a field cell, pixels */
#define FW (APD_RTW * APD_TILE / FC)
#define FH (APD_RTH * APD_TILE / FC)
static uint16_t fld[FH][FW];
static uint8_t ffree[FH][FW]; /* 0 blocked, else the cost to step in */
static int fld_room = -1;
static uint32_t fld_key;

static bool spike_near(int x0, int y0, int x1, int y1) {
    for (int ty = y0 / APD_TILE; ty <= y1 / APD_TILE; ty++)
        for (int tx = x0 / APD_TILE; tx <= x1 / APD_TILE; tx++) {
            int t = apd_tile_at(tx * APD_TILE, ty * APD_TILE);
            if (tx >= 0 && ty >= 0 && tx < ag.rw && ty < ag.rh && (t == T_SPUP || t == T_SPDN)) return true;
        }
    return false;
}

static int cell_cost(int cx, int cy) {
    int px = cx * FC + FC / 2, py = cy * FC + FC / 2;
    int x0 = px - APD_PW / 2, x1 = x0 + APD_PW - 1, y0 = py - APD_PH / 2, y1 = y0 + APD_PH - 1;
    if (x0 < -APD_TILE || y0 < -APD_TILE) return 0;
    int cost = 1;
    for (int ty = (y0 < 0 ? -1 : y0 / APD_TILE); ty <= y1 / APD_TILE; ty++)
        for (int tx = (x0 < 0 ? -1 : x0 / APD_TILE); tx <= x1 / APD_TILE; tx++) {
            int t;
            if (tx < 0 || ty < 0 || tx >= ag.rw || ty >= ag.rh) {
                /* beyond the border: only out through a doorway */
                int bx = iclamp(tx, 0, ag.rw - 1), by = iclamp(ty, 0, ag.rh - 1);
                if (!ag.tdoor[by][bx]) return 0;
                t = ag.tile[by][bx];
                if (t != T_AIR && t != T_DOOR && t != T_LOCK && t != T_ZAP) return 0;
                continue;
            }
            t = ag.tile[ty][tx];
            switch (t) {
            case T_AIR: case T_PROP: break;
            case T_PLAT: cost = imax(cost, 6); break;
            case T_DOOR: cost = imax(cost, 2); break;
            case T_LOCK: {
                int d = ag.tdoor[ty][tx] - 1;
                if (d < 0 || ag.run.pass < apd_doors[d].type - E_PASS1 + 1) return 0;
                cost = imax(cost, 2);
                break;
            }
            case T_ACID: cost = imax(cost, 5); break;
            case T_ZAP: if (!apd_has(MOD_VOLT, SYS_FLIP)) return 0; cost = imax(cost, 4); break;
            default: return 0;
            }
        }
    if (spike_near(x0 - 3, y0 - 3, x1 + 3, y1 + 3)) {
        /* with IRONSOLE a spike costs 10: worth it to get by */
        if (!apd_has(MOD_MASS, SYS_FRAME)) return 0;
        cost = imax(cost, 8);
    }
    for (int i = 0; i < ag.nth; i++)
        if (ag.th[i].alive && ag.th[i].kind == TH_BALL) {
            /* a ball that is live now is a wall; one that isn't, a place to hurry past */
            bool live = ag.th[i].var == 0 ? ag.pl.grav > 0 : ag.pl.grav < 0;
            int bx = ag.th[i].tx * APD_TILE + 8, by = ag.th[i].ty * APD_TILE + 8;
            if (iabs(bx - px) < 12 && iabs(by - py) < 17) {
                if (live) return 0;
                cost = imax(cost, 6);
            }
        }
    if ((APD_SPECS[apd_rooms[ag.room].spec].flags & RF_ACID) && py > ag.rh * APD_TILE - 40) cost = imax(cost, 2);
    return cost;
}

/* goals: a rectangle of pixels the middle of the suit should get into */
static int gx0, gy0, gx1, gy1;
static bool goal_overlap; /* the goal is any spot where the suit touches the rectangle */

static void field_build(void) {
    int fw = ag.rw * APD_TILE / FC, fh = ag.rh * APD_TILE / FC;
    for (int y = 0; y < fh; y++)
        for (int x = 0; x < fw; x++) {
            ffree[y][x] = (uint8_t)cell_cost(x, y);
            fld[y][x] = 0xffff;
        }
    /* Dijkstra with small costs: a bucket queue */
    static int16_t qx[8][FW * FH], qy[8][FW * FH];
    static int qn[8];
    memset(qn, 0, sizeof qn);
    int seeded = 0;
    for (int y = 0; y < fh; y++)
        for (int x = 0; x < fw; x++) {
            int px = x * FC + FC / 2, py = y * FC + FC / 2;
            bool in = goal_overlap ? rects_overlap(px - APD_PW / 2, py - APD_PH / 2, APD_PW, APD_PH, gx0, gy0, gx1 - gx0, gy1 - gy0)
                                   : (px >= gx0 && px <= gx1 && py >= gy0 && py <= gy1);
            if (in && ffree[y][x]) {
                fld[y][x] = 0;
                qx[0][qn[0]] = (int16_t)x;
                qy[0][qn[0]++] = (int16_t)y;
                seeded++;
            }
        }
    if (!seeded) {
        /* the goal sits in something solid: start from the free cells nearest it */
        int cx = (gx0 + gx1) / 2 / FC, cy = (gy0 + gy1) / 2 / FC;
        for (int r = 1; r < 6 && !seeded; r++)
            for (int y = cy - r; y <= cy + r; y++)
                for (int x = cx - r; x <= cx + r; x++)
                    if (x >= 0 && y >= 0 && x < fw && y < fh && ffree[y][x] && fld[y][x]) {
                        fld[y][x] = 0;
                        qx[0][qn[0]] = (int16_t)x;
                        qy[0][qn[0]++] = (int16_t)y;
                        seeded++;
                    }
    }
    int cur = 0, empty = 0;
    static int qh[8];
    memset(qh, 0, sizeof qh);
    while (empty < 8) {
        int b = cur % 8;
        if (qh[b] >= qn[b]) { qh[b] = qn[b] = 0; cur++; empty++; continue; }
        empty = 0;
        int x = qx[b][qh[b]], y = qy[b][qh[b]++];
        int dcur = fld[y][x];
        static const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (nx < 0 || ny < 0 || nx >= fw || ny >= fh || !ffree[ny][nx]) continue;
            int nd = dcur + ffree[ny][nx];
            if (nd < fld[ny][nx]) {
                fld[ny][nx] = (uint16_t)nd;
                int nb = nd % 8;
                if (qn[nb] < FW * FH) { qx[nb][qn[nb]] = (int16_t)nx; qy[nb][qn[nb]++] = (int16_t)ny; }
            }
        }
    }
}

static void set_goal_mode(int x0, int y0, int x1, int y1, bool overlap) {
    uint32_t key = (uint32_t)(x0 * 7 + y0 * 131 + x1 * 1031 + y1 * 7919 + overlap) ^ (uint32_t)ag.room * 2654435761u;
    goal_overlap = overlap;
    /* rebuild when the goal or the room changes, or now and then (doors open, gates burn) */
    static int age;
    if (key != fld_key || fld_room != ag.room || ++age > 30) {
        gx0 = x0; gy0 = y0; gx1 = x1; gy1 = y1;
        fld_key = key;
        fld_room = ag.room;
        age = 0;
        field_build();
    }
}
static void set_goal(int x0, int y0, int x1, int y1) { set_goal_mode(x0, y0, x1, y1, false); }

static int field_at(int px, int py) {
    int fw = ag.rw * APD_TILE / FC, fh = ag.rh * APD_TILE / FC;
    int cx = px / FC, cy = py / FC;
    if (px < 0 || py < 0) cx = cy = -1;
    int best = 20000;
    for (int r = 0; r < 3 && best == 20000; r++)
        for (int y = cy - r; y <= cy + r; y++)
            for (int x = cx - r; x <= cx + r; x++)
                if (x >= 0 && y >= 0 && x < fw && y < fh && fld[y][x] != 0xffff) best = imin(best, fld[y][x] + r * 3);
    return best;
}

/* ---- looking ahead ------------------------------------------------------------------- */
#define HORIZON 24
#define NPLANS 41
static uint32_t last_mask, cur_plan_masks[HORIZON];
static int plan_left;
static bool have_prev;
static int want_room = -1;  /* the room to step into next (-1: none) */
static bool fight_mode;

static uint32_t plan_mask(int p, int k) {
    if (p == NPLANS - 1) return k + 1 < HORIZON ? cur_plan_masks[k + 1] : 0;
    int h = p / 8, b = p % 8;
    /* plans 24-39 turn left (or right) only halfway through */
    if (h >= 3) h = (k >= 10) ? h - 2 : 0;
    uint32_t m = h == 1 ? BTN_LEFT : h == 2 ? BTN_RIGHT : 0;
    bool B = false;
    switch (b) {
    case 1: B = k < 2; break;
    case 2: B = true; break;
    case 3: B = k < 2 || (k >= 6 && k < 8); break;
    case 4: B = k < 2 || (k >= 12 && k < 14); break;
    case 5: B = k < 2 || (k >= 4 && k < 6) || (k >= 10 && k < 12); break;
    case 6: B = k >= 8 && k < 10; break;
    case 7: B = k < 10 || (k >= 15 && k < 17); break;
    default: break;
    }
    if (B) m |= BTN_B;
    return m;
}

/* harm done to the foes that were there at the start of a look-ahead */
static int16_t hp_at[APD_MAX_FOES];
static int phase_at;
static bool boss_at;
static void foes_snapshot(void) {
    for (int i = 0; i < APD_MAX_FOES; i++) hp_at[i] = ag.foe[i].alive ? ag.foe[i].hp : 0;
    phase_at = ag.boss_on ? ag.boss_phase : -1;
    boss_at = ag.boss_on;
}
static int harm_done(void) {
    int n = 0;
    for (int i = 0; i < APD_MAX_FOES; i++)
        if (hp_at[i] > 0) n += hp_at[i] - (ag.foe[i].alive ? imin(hp_at[i], ag.foe[i].hp) : 0);
    if (phase_at >= 0 && (ag.boss_phase > phase_at || !ag.boss_on)) n += 2000;
    return n;
}

static long evaluate(int room0, int armor0, int items0, int spares0) {
    if (ag.state == AS_DYING || ag.state == AS_OVER || ag.spares < spares0) return -100000000L;
    long s = 0;
    if (ag.room != room0) {
        if (ag.room == want_room) return 50000000L - (armor0 - ag.pl.armor) * 3000L;
        return -50000000L;
    }
    if (ag.state != AS_PLAY) s -= 1000000L; /* walked into a cradle or terminal by mistake */
    s -= (long)field_at(APD_PX(ag.pl.x), APD_PX(ag.pl.y)) * 100;
    s -= (long)(armor0 - ag.pl.armor) * 500;
    if (fight_mode) s += (long)harm_done() * 400;
    if (fight_mode && !boss_at && ag.boss_on) s += 4000; /* get the fight started */
    if (apd_items_got() > items0) s += 2000000L;
    return s;
}

static uint32_t fire_bits;   /* A and aim, the same for every plan */
static int dbg_src;
static bool fire_set;        /* a scripted move chose them */

static uint32_t look_ahead(void) {
    ApdGame keep = ag;
    int room0 = ag.room, armor0 = ag.pl.armor, items0 = apd_items_got(), spares0 = ag.spares;
    foes_snapshot();
    long best = -2000000000L;
    int bestp = 0;
    uint32_t prev0 = last_mask;
    apd_simulating = true;
    for (int p = 0; p < NPLANS; p++) {
        if (p == NPLANS - 1 && !have_prev) continue;
        ag = keep;
        apd_sim_prev = prev0;
        long path = 0;
        for (int k = 0; k < HORIZON && ag.state == AS_PLAY; k++) {
            apd_sim_cur = plan_mask(p, k) | fire_bits;
            ag.state_t++;
            ag.frame_t++;
            apd_play_step();
            apd_sim_prev = apd_sim_cur;
            if (ag.room != room0) break; /* judged by the room it reached */
            path += field_at(APD_PX(ag.pl.x), APD_PX(ag.pl.y));
        }
        /* sooner is better: what the way there passed through counts a little */
        long s = evaluate(room0, armor0, items0, spares0) - path * 4;
        if (apd_bot_log >= 3) fprintf(stderr, "    plan %2d: %ld (y %d f %d)\n", p, s, APD_PX(ag.pl.y), field_at(APD_PX(ag.pl.x), APD_PX(ag.pl.y)));
        if (s > best) { best = s; bestp = p; }
    }
    apd_simulating = false;
    ag = keep;
    uint32_t nm[HORIZON];
    for (int k = 0; k < HORIZON; k++) nm[k] = plan_mask(bestp, k);
    memcpy(cur_plan_masks, nm, sizeof nm);
    plan_left = 3;
    have_prev = true;
    return cur_plan_masks[0];
}

/* ---- steering along the field ----------------------------------------------------------
 * Down the field a few cells to a waypoint; walk toward it; to rise or sink,
 * jump or flip so gravity points that way, and hover by flipping back and forth
 * around its height. A jump is held while the waypoint is higher still. */
static int wp_x, wp_y;

static void waypoint(int px, int py) {
    int fw = ag.rw * APD_TILE / FC, fh = ag.rh * APD_TILE / FC;
    int cx = px / FC, cy = py / FC;
    if (cx < 0 || cy < 0 || cx >= fw || cy >= fh || fld[cy][cx] == 0xffff) {
        /* off the field: the nearest cell on it */
        int best = 0xffff;
        for (int r = 1; r < 4 && best == 0xffff; r++)
            for (int y = cy - r; y <= cy + r; y++)
                for (int x = cx - r; x <= cx + r; x++)
                    if (x >= 0 && y >= 0 && x < fw && y < fh && fld[y][x] < best) { best = fld[y][x]; wp_x = x; wp_y = y; }
        if (best == 0xffff) { wp_x = cx; wp_y = cy; }
        return;
    }
    static const int DX[8] = {1, -1, 0, 0, 1, 1, -1, -1}, DY[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    for (int k = 0; k < 5; k++) {
        int bx = cx, by = cy, bv = fld[cy][cx];
        for (int d = 0; d < 8; d++) {
            int nx = cx + DX[d], ny = cy + DY[d];
            if (nx < 0 || ny < 0 || nx >= fw || ny >= fh) continue;
            /* a diagonal only where both sides are open */
            if (d >= 4 && (fld[cy][nx] == 0xffff || fld[ny][cx] == 0xffff)) continue;
            if (fld[ny][nx] < bv) { bv = fld[ny][nx]; bx = nx; by = ny; }
        }
        if (bx == cx && by == cy) break;
        cx = bx;
        cy = by;
    }
    wp_x = cx;
    wp_y = cy;
}

static uint32_t steer(uint32_t prev) {
    const ApdPlayer *p = &ag.pl;
    int px = APD_PX(p->x), py = APD_PX(p->y);
    waypoint(px, py);
    int tx = wp_x * FC + FC / 2, ty = wp_y * FC + FC / 2;
    int dx = tx - px, dy = ty - py;
    uint32_t m = 0;
    if (dx < -2) m |= BTN_LEFT;
    else if (dx > 2) m |= BTN_RIGHT;
    int s = dy < -3 ? -1 : dy > 3 ? 1 : 0; /* which way it wants to go, on the screen */
    bool b_held = (prev & BTN_B) != 0;
    bool want_b = false, hold_b = false;
    bool anvil = apd_has(MOD_MASS, SYS_FLIP);
    if (p->ground) {
        if (s == -p->grav) want_b = true; /* jump away from this surface */
    } else {
        if (p->jumping && s == -p->grav) hold_b = true; /* keep the jump going */
        else if (s != 0 && p->grav != s && !anvil) want_b = true;
        else if (s != 0 && p->grav != s && anvil && (iabs(dy) > 48 || !p->jumping)) want_b = true;
        else if (s == 0 && !anvil) {
            /* hover: turn back when drifting past */
            if ((p->grav > 0 && dy < -1 && p->vy > 0) || (p->grav < 0 && dy > 1 && p->vy < 0)) want_b = true;
        }
    }
    if (hold_b && b_held) m |= BTN_B;
    else if (want_b && !b_held) m |= BTN_B;
    return m;
}

/* run the steering a while on a copy: does it end in harm? */
static int steer_check(int frames, int *armor_lost) {
    ApdGame keep = ag;
    int room0 = ag.room, armor0 = ag.pl.armor, spares0 = ag.spares;
    uint32_t prev = last_mask;
    apd_simulating = true;
    int result = 0;
    int f0 = field_at(APD_PX(ag.pl.x), APD_PX(ag.pl.y));
    for (int k = 0; k < frames && ag.state == AS_PLAY; k++) {
        uint32_t m = steer(prev) | fire_bits;
        apd_sim_prev = prev;
        apd_sim_cur = m;
        ag.state_t++;
        ag.frame_t++;
        apd_play_step();
        prev = m;
        if (ag.room != room0) { result = ag.room == want_room ? 2 : -2; break; }
    }
    if (ag.state == AS_DYING || ag.state == AS_OVER || ag.spares < spares0) result = -3;
    *armor_lost = armor0 - ag.pl.armor;
    if (result == 0) result = field_at(APD_PX(ag.pl.x), APD_PX(ag.pl.y)) < f0 ? 1 : 0;
    apd_simulating = false;
    ag = keep;
    return result;
}

/* fire at what is in front, over the head, or (in the air) under the feet */
static void choose_fire(bool allow_up) {
    fire_bits = 0;
    const ApdPlayer *p = &ag.pl;
    int px = APD_PX(p->x), py = APD_PX(p->y);
    int range = apd_shot_range() * 5;
    int bestd = 1 << 30;
    for (int i = 0; i < APD_MAX_FOES; i++) {
        const ApdFoe *f = &ag.foe[i];
        if (!f->alive || (f->flags & (FF_NOHIT | FF_SHIELD)) || f->kind == FO_LAMP_SEG || f->kind == FO_HAL_LEG) {
            if (!(f->alive && f->kind == FO_HAL_LEG)) continue;
        }
        int dx = APD_PX(f->x) - px, dy = APD_PX(f->y) - py;
        int d = iabs(dx) + iabs(dy);
        if (iabs(dy) < f->h / 2 + 6 && iabs(dx) < range && d < bestd) {
            bestd = d;
            fire_bits = (isign(dx) == p->face || !p->firing) ? BTN_A : 0;
        } else if (iabs(dx) < f->w / 2 + 6 && iabs(dy) < range && d < bestd) {
            bool above = dy * p->grav < 0;
            if (above && allow_up) { bestd = d; fire_bits = BTN_A | BTN_UP; }
            else if (!above && !p->ground) { bestd = d; fire_bits = BTN_A | BTN_DOWN; }
        }
    }
    if (fire_bits || !fight_mode) return;
    /* in a fight: keep firing ahead while the target is in front, and let go
     * when it is behind so the suit can turn */
    int best = -1;
    bestd = 1 << 30;
    for (int i = 0; i < APD_MAX_FOES; i++) {
        const ApdFoe *f = &ag.foe[i];
        if (!f->alive || (f->flags & FF_NOHIT) || f->kind == FO_LAMP_SEG) continue;
        int d = iabs(APD_PX(f->x) - px) + iabs(APD_PX(f->y) - py);
        if (d < bestd) { bestd = d; best = i; }
    }
    if (best < 0) return;
    int dx = APD_PX(ag.foe[best].x) - px;
    if (isign(dx) == p->face || dx == 0) fire_bits = BTN_A;
}

/* ---- scripted moves ---------------------------------------------------------------- */
static int script_t;

static bool near_pt(int x, int y, int r) {
    return iabs(APD_PX(ag.pl.x) - x) <= r && iabs(APD_PX(ag.pl.y) - y) <= r;
}

static uint32_t go_to(int x, int y, int r) {
    set_goal(x - r, y - r, x + r, y + r);
    return 0xffffffffu; /* the planner decides */
}

/* the gate of door d, seen from this room: where to stand and what to do */
int apd_door_pos(int door);

static uint32_t gate_work(int d) {
    const ApdDoor *dd = &apd_doors[d];
    const ApdRoom *r = &apd_rooms[ag.room];
    bool is_a = dd->ra == ag.room;
    int gx = dd->ax + (!is_a && !dd->vertical ? 1 : 0), gy = dd->ay + (!is_a && dd->vertical ? 1 : 0);
    int lx = (gx - r->gx) * APD_CW, ly = (gy - r->gy) * APD_CH;
    int pos = apd_door_pos(d);
    const ApdPlayer *p = &ag.pl;
    int px = APD_PX(p->x), py = APD_PX(p->y);
    bool b_held = (last_mask & BTN_B) != 0;
    script_t++;
    if (!dd->vertical) {
        /* barrels, or the target in the wall beside a shutter: level with it, a
         * little way in, facing it, firing */
        int bx = is_a ? lx + APD_CW - 1 : lx;
        int dir = is_a ? 1 : -1;
        int ty = (ly + pos + 1) * APD_TILE;
        if (dd->type == E_TARGET)
            for (int i = 0; i < ag.nth; i++)
                if (ag.th[i].kind == TH_TARGET && ag.th[i].id == d) ty = ag.th[i].ty * APD_TILE + 8 + 2;
        int sx = bx * APD_TILE + 8 - dir * 30;
        if (iabs(px - sx) > 12 || iabs(py - ty) > 5) {
            set_goal(sx - 6, ty - 3, sx + 6, ty + 3);
            fire_bits = iabs(py - ty) <= 5 && p->face == dir ? BTN_A : 0;
            fire_set = true;
            return 0xffffffffu;
        }
        if (p->face != dir) return dir > 0 ? BTN_RIGHT : BTN_LEFT;
        set_goal(sx - 6, ty - 3, sx + 6, ty + 3);
        fire_bits = BTN_A;
        fire_set = true;
        return 0xffffffffu;
    }
    int sy = is_a ? ly + APD_CH - 1 : ly; /* the slab row */
    int x = (lx + pos + 1) * APD_TILE;
    bool below_me = is_a; /* the plug is in my floor */
    if (dd->type == E_CRACK) {
        if (below_me) {
            /* stand on it, jump and flip up (a drop up), then from up there jump
             * and flip down onto it */
            if (p->grav > 0) {
                if (iabs(px - x) > 7 || !p->ground || iabs(py - (sy * APD_TILE - APD_PH / 2)) > 6) {
                    if (!p->ground && iabs(px - x) <= 7 && p->vy < 0 && !p->stomp) return b_held ? 0 : BTN_B;
                    set_goal(x - 5, sy * APD_TILE - APD_PH / 2 - 3, x + 5, sy * APD_TILE - APD_PH / 2 + 1);
                    return 0xffffffffu;
                }
                return b_held ? 0 : BTN_B;
            }
            if (iabs(px - x) > 7) { /* up the wrong side, away from it: steer back first */
                set_goal(x - 5, sy * APD_TILE - APD_PH / 2 - 3, x + 5, sy * APD_TILE - APD_PH / 2 + 1);
                return 0xffffffffu;
            }
            if (p->ground) return b_held ? 0 : BTN_B;
            if (p->vy > 0 && !p->stomp) return b_held ? 0 : BTN_B;
            return 0;
        }
        /* in my ceiling: from the floor under it, jump and flip up into it */
        if (p->grav < 0) {
            if (iabs(px - x) > 7) {
                set_goal(x - 5, (sy + 1) * APD_TILE + 30, x + 5, ag.rh * APD_TILE);
                return 0xffffffffu;
            }
            if (p->ground) return b_held ? 0 : BTN_B;
            return p->vy > 0 && !b_held ? BTN_B : 0;
        }
        if (iabs(px - x) > 7 || !p->ground) {
            if (!p->ground && iabs(px - x) <= 7 && p->vy < 0 && !p->stomp) return b_held ? 0 : BTN_B;
            set_goal(x - 5, (sy + 1) * APD_TILE + 30, x + 5, ag.rh * APD_TILE);
            return 0xffffffffu;
        }
        return b_held ? 0 : BTN_B;
    }
    /* barrels in a shaft: fire straight at them */
    if (below_me) {
        int ty = sy * APD_TILE - APD_PH / 2;
        if (iabs(px - x) > 6) { set_goal(x - 4, ty - 3, x + 4, ty + 1); return 0xffffffffu; }
        if (p->ground) return p->grav > 0 ? (b_held ? 0 : BTN_B) : 0;
        return p->grav > 0 ? (BTN_A | BTN_DOWN) : (BTN_A | BTN_UP);
    }
    if (iabs(px - x) > 6 || py > (sy + 1) * APD_TILE + 90) {
        set_goal(x - 4, (sy + 1) * APD_TILE + 20, x + 4, (sy + 1) * APD_TILE + 80);
        return 0xffffffffu;
    }
    return p->grav > 0 ? (BTN_A | BTN_UP) : (p->ground ? (b_held ? 0 : BTN_B) : (BTN_A | BTN_DOWN));
}

/* The quiet vault: a pad in the floor and one in the ceiling right over it.
 * Stand on the floor pad, jump and flip at once: the shadow stays on the
 * floor pad while the suit falls up onto the ceiling pad. */
static uint32_t secret_work(void) {
    int fx = -1, fy = 0;
    for (int i = 0; i < ag.nth; i++)
        if (ag.th[i].kind == TH_BUTTON && ag.th[i].var == 0) { fx = ag.th[i].tx * APD_TILE + 8; fy = (ag.th[i].ty + 1) * APD_TILE; }
    if (fx < 0) return 0xffffffffu;
    const ApdPlayer *p = &ag.pl;
    int px = APD_PX(p->x), py = APD_PX(p->y);
    bool b_held = (last_mask & BTN_B) != 0;
    script_t++;
    if (p->grav < 0) {
        /* up on the ceiling (or falling up): come back down */
        if (p->ground) return b_held ? 0 : BTN_B;
        if (p->vy > 0 && !b_held) return BTN_B;
        return 0;
    }
    if (iabs(px - fx) > 6 || (!p->ground && py < fy - 40)) {
        set_goal(fx - 4, fy - APD_PH / 2 - 4, fx + 4, fy - APD_PH / 2 + 2);
        return 0xffffffffu;
    }
    if (p->ground) return b_held ? 0 : BTN_B;      /* jump */
    if (p->vy < 0 && py > fy - APD_PH / 2 - 20) return b_held ? 0 : BTN_B; /* and flip at once */
    return 0;
}

/* an upgrade sealed in cracked concrete: from the floor under it, jump and
 * flip, and the ANVIL drives the suit up through the concrete */
static uint32_t anvil_up_work(int ix, int iy) {
    const ApdPlayer *p = &ag.pl;
    int tx = ix / APD_TILE, ty = iy / APD_TILE + 1;
    while (ty < ag.rh && apd_tile_at(ix, ty * APD_TILE) == T_CRACK) ty++;
    while (ty < ag.rh && apd_tile_at(ix, ty * APD_TILE) != T_WALL && apd_tile_at(ix, ty * APD_TILE) != T_SPUP) ty++;
    (void)tx;
    int sy = ty * APD_TILE - APD_PH / 2;
    bool b_held = (last_mask & BTN_B) != 0;
    if (p->grav < 0) {
        if (p->ground) return b_held ? 0 : BTN_B;
        return p->vy > 0 && !b_held ? BTN_B : 0;
    }
    if (iabs(APD_PX(p->x) - ix) > 6 || APD_PX(p->y) < sy - 70) {
        set_goal(ix - 4, sy - 4, ix + 4, sy + 2);
        return 0xffffffffu;
    }
    if (p->ground) return b_held ? 0 : BTN_B;
    if (p->vy < 0 && !p->stomp) return b_held ? 0 : BTN_B;
    return 0;
}

/* ---- the cradle's menu ------------------------------------------------------------------- */
static uint32_t pod_menu(void) {
    static int wait;
    if (++wait % 3) return 0; /* a press, then let go */
    for (int m = 0; m < APD_MODS; m++) {
        if (ag.run.mod_sys[m] == MOD_NONE || ag.run.mod_sys[m] == ob.lo.s[m]) continue;
        if (ag.pod_page != 0) return BTN_LEFT;
        if (ag.pod_mod < m) return BTN_RIGHT;
        if (ag.pod_mod > m) return BTN_LEFT;
        static const int ROW_OF[APD_SYS] = {3, 0, 1, 2};
        int have = ROW_OF[ag.run.mod_sys[m]], want = ROW_OF[ob.lo.s[m]];
        return want < have ? BTN_UP : BTN_DOWN;
    }
    pod_done = 1;
    return BTN_B;
}

/* ---- fighting ---------------------------------------------------------------------------
 * Pick what can be hurt (the boss, or its nodes or joints while it is shut;
 * the nearest orb), get level with it a little way off, face it and fire.
 * When nothing can be hurt, keep away high up and let the look-ahead dodge. */
static int fight_target(void) {
    const ApdPlayer *p = &ag.pl;
    int best = -1, bd = 1 << 30;
    for (int i = 0; i < APD_MAX_FOES; i++) {
        const ApdFoe *f = &ag.foe[i];
        if (!f->alive || (f->flags & (FF_NOHIT | FF_SHIELD | FF_REFLECT))) continue;
        bool ok;
        if (ob.kind == OB_NANO) ok = f->kind == FO_SENTRY;
        else ok = i == ag.boss_foe || f->kind == FO_NODE || f->kind == FO_HAL_JOINT || f->kind == FO_HAL_LEG;
        if (!ok) continue;
        int d = iabs(APD_PX(f->x) - APD_PX(p->x)) + iabs(APD_PX(f->y) - APD_PX(p->y));
        if (i == ag.boss_foe) d -= 60; /* the body itself when it can be hit */
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

static uint32_t fight(void) {
    const ApdPlayer *p = &ag.pl;
    int W = ag.rw * APD_TILE, H = ag.rh * APD_TILE;
    if (!ag.boss_on && ob.kind != OB_NANO) {
        /* walk on in to start it */
        set_goal(W / 2 - 24, 40, W / 2 + 24, H - 40);
        return 0xffffffffu;
    }
    int t = fight_target();
    int px = APD_PX(p->x), py = APD_PX(p->y);
    if (t < 0) {
        /* nothing to hit: stay up, out of the way */
        int bx = ag.boss_foe >= 0 ? APD_PX(ag.foe[ag.boss_foe].x) : W / 2;
        int x = bx < W / 2 ? W - 60 : 60;
        set_goal(x - 20, 24, x + 20, 60);
        return 0xffffffffu;
    }
    const ApdFoe *f = &ag.foe[t];
    int fx = APD_PX(f->x), fy = APD_PX(f->y);
    int side = px < fx ? -1 : 1;
    if (fx + side * 76 < 28 || fx + side * 76 > W - 28) side = -side;
    int ax = iclamp(fx + side * 76, 28, W - 28), ay = iclamp(fy, 28, H - 28);
    set_goal(ax - 10, ay - 6, ax + 10, ay + 6);
    bool level = iabs(py - fy) < f->h / 2 + 3;
    bool facing = p->face == -side;
    if (level && !facing && !p->firing) return side > 0 ? BTN_LEFT : BTN_RIGHT; /* turn to it */
    fire_bits = level && facing ? BTN_A : 0;
    return 0xffffffffu;
}

/* ---- the whole thing, frame by frame --------------------------------------------------- */
static int menu_t;

static uint32_t play_frame(void) {
    const ApdPlayer *p = &ag.pl;
    fight_mode = ag.boss_on;
    want_room = -1;
    fire_bits = 0;
    fire_set = false;
    if (ag.boss_on && (!have_ob || ob.room != ag.room || (ob.kind != OB_BOSS && ob.kind != OB_FINAL))) {
        /* shut in with a boss: that is the job now */
        ob = (Obj){ag.boss == BS_HALLORAN ? OB_FINAL : OB_BOSS, ag.room, ag.boss, cur_lo(), -1};
        have_ob = true;
        pod_done = 1;
    }
    if (!have_ob || ob_done(&ob)) decide();
    if (!have_ob) return 0;
    /* first fit the mods at a cradle, if this needs it */
    int target_room = pod_done ? ob.room : ob.pod_room;
    Lo L = pod_done ? ob.lo : cur_lo();
    uint32_t act = 0xffffffffu;
    bool allow_up = true;
    if (ag.room == target_room) {
        if (!pod_done) {
            /* to the cradle, and in */
            for (int i = 0; i < ag.nth; i++)
                if (ag.th[i].kind == TH_POD) {
                    int x = ag.th[i].tx * APD_TILE + 8 + 14, y = (ag.th[i].ty + 1) * APD_TILE - APD_PH / 2;
                    if (ag.pod_zone == ag.th[i].id && p->ground && p->grav > 0) act = (ag.frame_t % 4 < 2) ? BTN_UP : 0;
                    else act = go_to(x, y, 3);
                    allow_up = false;
                    break;
                }
        } else {
            switch (ob.kind) {
            case OB_ITEM: {
                if (ob.room == x_room() && !ag.run.secret_open) { act = secret_work(); break; }
                for (int i = 0; i < ag.nth; i++)
                    if (ag.th[i].kind == TH_ITEM && ag.th[i].id == ob.arg && ag.th[i].alive) {
                        int ix = ag.th[i].tx * APD_TILE + 8, iy = ag.th[i].ty * APD_TILE + 8;
                        bool sealed = false;
                        for (int dy = -1; dy <= 1; dy++)
                            for (int dx = -1; dx <= 1; dx++)
                                if ((dx || dy) && apd_tile_at(ix + dx * APD_TILE, iy + dy * APD_TILE) == T_CRACK) sealed = true;
                        act = sealed ? anvil_up_work(ix, iy) : go_to(ix, iy, 5);
                    }
                break;
            }
            case OB_PASS:
                for (int i = 0; i < ag.nth; i++)
                    if (ag.th[i].kind == TH_TERM && ag.th[i].var == ob.arg) {
                        int x = ag.th[i].tx * APD_TILE + 8, y = (ag.th[i].ty + 1) * APD_TILE - APD_PH / 2;
                        if (near_pt(x, y, 6) && p->ground && p->grav > 0) act = (ag.frame_t % 4 < 2) ? BTN_UP : 0;
                        else act = go_to(x, y, 5);
                    }
                allow_up = false;
                break;
            case OB_BOSS: case OB_FINAL: case OB_NANO: {
                fight_mode = true;
                act = fight();
                break;
            }
            default: break;
            }
        }
    } else {
        /* on toward the target room */
        bfs(ag.room, &L);
        int via = 0, next = first_hop(ag.room, target_room, &via);
        if (next < 0) { have_ob = false; return 0; }
        want_room = next;
        if (via >= 0) {
            const ApdDoor *dd = &apd_doors[via];
            bool closed = !ag.run.gate_open[via] &&
                          (dd->type == E_BARREL || dd->type == E_HIDDEN || dd->type == E_CRACK || dd->type == E_TARGET);
            if (closed) act = gate_work(via);
            else {
                const ApdRoom *r = &apd_rooms[ag.room];
                bool is_a = dd->ra == ag.room;
                int gx = dd->ax + (!is_a && !dd->vertical ? 1 : 0), gy = dd->ay + (!is_a && dd->vertical ? 1 : 0);
                int lx = (gx - r->gx) * APD_CPW, ly = (gy - r->gy) * APD_CPH;
                int dp = apd_door_pos(via);
                if (!dd->vertical) {
                    int x = is_a ? lx + APD_CPW - 16 : lx;
                    set_goal_mode(x, ly + dp * APD_TILE, x + 16, ly + (dp + 2) * APD_TILE, true);
                    /* in the doorway: on through it */
                    if (field_at(APD_PX(p->x), APD_PX(p->y)) == 0) act = is_a ? BTN_RIGHT : BTN_LEFT;
                } else {
                    int y = is_a ? ly + APD_CPH - 16 : ly;
                    set_goal_mode(lx + dp * APD_TILE, y, lx + (dp + 2) * APD_TILE, y + 16, true);
                    int want_g = is_a ? 1 : -1;
                    if (field_at(APD_PX(p->x), APD_PX(p->y)) == 0 && p->grav != want_g)
                        act = (last_mask & BTN_B) ? 0 : BTN_B; /* jump or flip toward it */
                    else if (p->grav == want_g && !p->ground && APD_PX(p->x) >= lx + dp * APD_TILE &&
                             APD_PX(p->x) < lx + (dp + 2) * APD_TILE && APD_PX(p->y) >= y - 12 && APD_PX(p->y) < y + 28)
                        act = 0; /* falling through the doorway: let it */
                }
            }
        } else {
            /* a teleporter: stand on the pad and press up */
            for (int i = 0; i < ag.nth; i++)
                if (ag.th[i].kind == TH_TELE) {
                    int x = ag.th[i].tx * APD_TILE + 8, y = (ag.th[i].ty + 1) * APD_TILE - APD_PH / 2;
                    if (near_pt(x, y, 6) && p->ground && p->grav > 0) act = (ag.frame_t % 4 < 2) ? BTN_UP : 0;
                    else act = go_to(x, y, 4);
                }
            allow_up = false;
        }
    }
    if (act != 0xffffffffu) {
        plan_left = 0;
        have_prev = false;
        dbg_src = 1;
        return act;
    }
    /* no aiming up next to a cradle, a terminal or a pad (that would step in) */
    for (int i = 0; i < ag.nth; i++)
        if ((ag.th[i].kind == TH_POD || ag.th[i].kind == TH_TERM || ag.th[i].kind == TH_TELE) &&
            iabs(ag.th[i].tx * APD_TILE + 8 - APD_PX(p->x)) < 40 && iabs(ag.th[i].ty * APD_TILE + 8 - APD_PX(p->y)) < 40)
            allow_up = false;
    if (!fire_set && (!fight_mode || !fire_bits)) choose_fire(allow_up);
    /* steer, checked on a copy now and then; the look-ahead takes over for a
     * while when the steering would come to harm or makes no headway */
    static int plan_mode_t, check_t, best_f, stall_t, last_room = -1;
    int f = field_at(APD_PX(p->x), APD_PX(p->y));
    if (ag.room != last_room || f < best_f) { best_f = f; stall_t = 0; last_room = ag.room; }
    else if (++stall_t > 240) { plan_mode_t = 90; stall_t = 0; best_f = f; }
    if (plan_mode_t <= 0) {
        if (--check_t <= 0) {
            int lost = 0, r = steer_check(36, &lost);
            check_t = 8;
            if (r == -3 || r == -2 || lost >= 12) plan_mode_t = 24;
        }
        if (plan_mode_t <= 0) {
            plan_left = 0;
            have_prev = false;
            dbg_src = 2;
            return steer(last_mask) | fire_bits;
        }
    }
    if (plan_mode_t > 0) plan_mode_t--;
    if (plan_left > 0) {
        plan_left--;
        memmove(cur_plan_masks, cur_plan_masks + 1, sizeof cur_plan_masks - sizeof cur_plan_masks[0]);
        cur_plan_masks[HORIZON - 1] = 0;
        dbg_src = 3;
        return cur_plan_masks[0] | fire_bits;
    }
    dbg_src = 4;
    return look_ahead() | fire_bits;
}

int apd_bot_buttons(void) {
    uint32_t m = 0;
    menu_t++;
    switch (ag.state) {
    case AS_TITLE:
        if (ag.menu != (ags.has_run && apd_bot_plan == 1 ? 1 : 0)) m = menu_t % 6 < 2 ? BTN_DOWN : 0;
        else m = menu_t % 6 < 2 ? BTN_A : 0;
        have_ob = false;
        break;
    case AS_INTRO: m = menu_t % 6 < 2 ? BTN_A : 0; break;
    case AS_PLAY: m = play_frame(); break;
    case AS_POD: m = pod_done ? (menu_t % 4 < 2 ? BTN_B : 0) : pod_menu(); break;
    case AS_TEXT: m = menu_t % 6 < 2 ? BTN_A : 0; break;
    case AS_OVER: m = menu_t % 6 < 2 ? BTN_A : 0; have_ob = false; break;
    case AS_ENDING: case AS_CREDITS: m = BTN_A; break;
    default: break;
    }
    if (ag.state != AS_PLAY) { plan_left = 0; have_prev = false; }
    if (ag.state == AS_PLAY) {
        const ApdPlayer *p = &ag.pl;
    if (apd_bot_log >= 2 && (apd_bot_log >= 4 || ag.run.frames % 30 == 0))
        fprintf(stderr, "  [%u] room %c%c p %d,%d g %d gr %d field %d goal %d..%d,%d..%d want %d act %x fire %x wp %d,%d src %d\n", (unsigned)ag.run.frames,
                "GBCK"[apd_rooms[ag.room].sector], apd_rooms[ag.room].id,APD_PX(p->x), APD_PX(p->y), p->grav, p->ground, field_at(APD_PX(p->x), APD_PX(p->y)),
                gx0, gx1, gy0, gy1, want_room, m, fire_bits, wp_x * FC + 4, wp_y * FC + 4, dbg_src);
    }
    last_mask = m;
    return (int)m;
}

int apd_bot_idle(void) { return 0; }

/* tests: with everything the game gives, can every room and every upgrade be
 * reached from the start? (the graph the demo player plans on) */
int apd_reach_check(void) {
    ApdRun keep = ag.run;
    int keep_room = ag.room;
    memset(&ag.run, 0, sizeof ag.run);
    ag.run.pass = 3;
    ag.run.ciphers = 15;
    ag.run.dampers = 2;
    for (int m = 0; m < APD_MODS; m++) ag.run.mod_sys[m] = SYS_GUN;
    for (int b = 0; b < APD_BOSSES; b++) ag.run.boss_dead[b] = 1;
    for (int d = 0; d < apd_ndoors; d++)
        if (apd_doors[d].type == E_NANO) ag.run.gate_open[d] = 1; /* the orbs can always be shot */
    Lo los[256];
    int nl = all_loadouts(los, 256);
    static uint8_t seen[APD_MAX_ROOMS];
    memset(seen, 0, sizeof seen);
    int start = apd_room_find(SEC_GANTRY, 'S');
    for (int l = 0; l < nl; l++) {
        bfs(start, &los[l]);
        for (int r = 0; r < apd_nrooms; r++)
            if (bd[r] >= 0) seen[r] = 1;
    }
    int missing = 0;
    for (int r = 0; r < apd_nrooms; r++)
        if (!seen[r]) { missing++; fprintf(stderr, "unreachable room %d %c\n", apd_rooms[r].sector, apd_rooms[r].id); }
    ag.run = keep;
    ag.room = keep_room;
    return missing;
}
