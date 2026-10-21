/* LANDFALL - the demo player. It plays with real button presses and knows
 * the planet the way a player with a hand-drawn map does: where the tools,
 * cells, gates, villages and trades are. Each frame it picks the next job
 * (a cell, a tool, a trade, a gate, the dungeon, the ship), walks there by
 * a breadth-first search over the area's tiles (steering round creatures),
 * juggles its two hands, and shoots what gets in line. */
#include "lfl.h"

int lfl_bot_task, lfl_bot_tx, lfl_bot_ty, lfl_bot_last, lfl_bot_need;

#define AW (LFL_GMAX_W * LFL_RW)
#define AH (LFL_GMAX_H * LFL_RH)

enum {
    TK_NONE = 0, TK_TITLE, TK_SHIP, TK_EAT, TK_CUBE, TK_TOOL, TK_TRADE, TK_GATE, TK_PRIZE, TK_DUNGEON, TK_GATHER,
    TK_RETREAT, TK_WANDER, TK_FIGHT
};

static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};

static int16_t dist[AH][AW];
static int16_t parent[AH][AW];
static uint32_t prev_mask;
/* (fight_busy / fight_off: lining up on a creature that keeps out of line,
 * say one whose shots she keeps dodging, is given up for a while) */
static int fight_busy, fight_off;
static int a_hold, b_tap, retreat_t, chop_gate, chop_t, stuck_t, last_wx, last_wy, wander_room, clear_t, clear_dir;

void lfl_bot_reset(void) {
    lfl_bot_task = 0;
    prev_mask = 0;
    a_hold = b_tap = retreat_t = chop_t = stuck_t = clear_t = fight_busy = fight_off = 0;
    chop_gate = -1;
    last_wx = last_wy = -1;
    wander_room = -1;
}

static bool holding(int tool) { return lg.pl.hold[0] == tool || lg.pl.hold[1] == tool; }
static bool small(void) { return lg.pl.shrink_t > 0; }
static int pw(void) { return small() ? 6 : 10; }
static int pcx(void) { return LFL_PX(lg.pl.x) + pw() / 2; }
static int pcy(void) { return LFL_PX(lg.pl.y) + pw() / 2; }
static int area(void) { return lp.room[lg.cur].area; }
static int my_wx(void) { return lp.room[lg.cur].gx * LFL_RW + pcx() / LFL_TILE; }
static int my_wy(void) { return lp.room[lg.cur].gy * LFL_RH + pcy() / LFL_TILE; }

static int room_of(int ar, int wx, int wy) {
    if (wx < 0 || wy < 0) return -1;
    int r = lfl_room_index(ar, wx / LFL_RW, wy / LFL_RH);
    return r >= 0 && lp.room[r].exists ? r : -1;
}
static int wtile(int ar, int wx, int wy) {
    int r = room_of(ar, wx, wy);
    return r < 0 ? T_STONE : lp.room[r].t[wy % LFL_RH][wx % LFL_RW];
}

/* ------------------------------------------------------------------------ */
/* the search                                                               */

static uint8_t blocked[AH][AW]; /* 1 a snail (never), 2 near a creature or a chop (only if need be) */
static int bog_room = -1;       /* the trapped village's bog, waded when she is well */

static bool can_step(int ar, int wx, int wy, bool swamp_ok) {
    if (wx < 0 || wy < 0 || wx >= AW || wy >= AH) return false;
    if (blocked[wy][wx] == 1) return false;
    int t = wtile(ar, wx, wy);
    switch (t) {
    case T_FLOOR: case T_WOOD: case T_TECHDOWN: case T_STAIRS_DN: case T_STAIRS_UP: return true;
    case T_SWAMP: return swamp_ok || holding(TL_BOOTS) || (bog_room >= 0 && room_of(ar, wx, wy) == bog_room);
    case T_HOLE: return holding(TL_HATCHER) || small();
    case T_SECRET: return lp.secret_key == SK_SMALL && room_of(ar, wx, wy) == lp.secret_room && (holding(TL_HATCHER) || small());
    default: return false;
    }
}

static void mark_box(int ar, int room, int x, int y, int w, int h, int mg, int how) {
    if (room < 0 || lp.room[room].area != ar) return;
    int ox = lp.room[room].gx * LFL_RW, oy = lp.room[room].gy * LFL_RH;
    for (int ty = (y - mg) / LFL_TILE; ty <= (y + h + mg - 1) / LFL_TILE; ty++)
        for (int tx = (x - mg) / LFL_TILE; tx <= (x + w + mg - 1) / LFL_TILE; tx++) {
            int wx = ox + tx, wy = oy + ty;
            if (tx >= 0 && ty >= 0 && tx < LFL_RW && ty < LFL_RH && wx < AW && wy < AH && blocked[wy][wx] != 1) blocked[wy][wx] = (uint8_t)how;
        }
}

static void build_blocks(bool foes) {
    memset(blocked, 0, sizeof blocked);
    int ar = area();
    /* the snails: where they stand (or lie as shells) */
    for (int g = 0; g < lp.ngate; g++) {
        const LflGate *gt = &lp.gate[g];
        if (gt->kind != GK_SNAIL) continue;
        if (gt->snail_room == lg.cur) continue; /* the live one is marked below */
        mark_box(ar, gt->snail_room, LFL_PX(gt->sx) + 3, LFL_PX(gt->sy) + 3, 22, 22, 3, 1);
    }
    for (int r = 0; r < LFL_MAX_ROOMS; r++) {
        const LflRoom *rm = &lp.room[r];
        if (!rm->exists || rm->area != ar || r == lg.cur) continue;
        for (int i = 0; i < rm->nspawn; i++)
            if (rm->spawn[i].type == SPW_SNAIL) mark_box(ar, r, rm->spawn[i].tx * LFL_TILE + 4, rm->spawn[i].ty * LFL_TILE + 4, 22, 22, 3, 1);
    }
    for (int i = 0; i < LFL_MAX_MOBS; i++) {
        const LflMob *m = &lg.mob[i];
        if (!m->alive) continue;
        if (m->sp == SP_SNAIL) { mark_box(ar, lg.cur, LFL_PX(m->x) + 3, LFL_PX(m->y) + 3, m->w - 6, m->h - 6, 3, 1); continue; }
        if (!foes || (m->flags & (MF_MORPHED | MF_SHELL))) continue;
        if ((m->flags & MF_FRIENDLY) && !(m->flags & MF_ANGRY)) continue;
        mark_box(ar, lg.cur, LFL_PX(m->x), LFL_PX(m->y), m->w, m->h, 8, 2);
    }
    /* chops: stepped on, they are eaten, so round them unless she is hurt */
    if (lg.pl.hp >= lg.pl.hpmax)
        for (int i = 0; i < LFL_MAX_DROPS; i++)
            if (lg.drop[i].alive && lg.drop[i].kind == DR_CHOP)
                mark_box(ar, lg.cur, LFL_PX(lg.drop[i].x), LFL_PX(lg.drop[i].y), 8, 8, 5, 2);
    /* never stand on the teleporting bog */
    for (int k = 0; k < 2; k++)
        if (lp.warp_room[k] >= 0 && lp.room[lp.warp_room[k]].area == ar) {
            int wx = lp.room[lp.warp_room[k]].gx * LFL_RW + lp.warp_x[k], wy = lp.room[lp.warp_room[k]].gy * LFL_RH + lp.warp_y[k];
            blocked[wy][wx] = 1;
        }
    blocked[my_wy()][my_wx()] = 0;
}

/* Dijkstra over the tiles (a binary heap): a step costs 1, a step close
 * to a creature 12, so she goes round them when there is a way round */
static int heap_n;
static int32_t heap[AW * AH * 2];

static void heap_push(int32_t v) {
    int i = heap_n++;
    heap[i] = v;
    while (i > 0 && heap[(i - 1) / 2] > heap[i]) {
        int32_t t = heap[i];
        heap[i] = heap[(i - 1) / 2];
        heap[(i - 1) / 2] = t;
        i = (i - 1) / 2;
    }
}

static int32_t heap_pop(void) {
    int32_t top = heap[0];
    heap[0] = heap[--heap_n];
    int i = 0;
    for (;;) {
        int l = i * 2 + 1, r = l + 1, m = i;
        if (l < heap_n && heap[l] < heap[m]) m = l;
        if (r < heap_n && heap[r] < heap[m]) m = r;
        if (m == i) break;
        int32_t t = heap[i];
        heap[i] = heap[m];
        heap[m] = t;
        i = m;
    }
    return top;
}

static void bfs(bool swamp_ok) {
    int ar = area();
    for (int y = 0; y < AH; y++)
        for (int x = 0; x < AW; x++) dist[y][x] = -1;
    int sx = my_wx(), sy = my_wy();
    dist[sy][sx] = 0;
    parent[sy][sx] = -1;
    heap_n = 0;
    heap_push(sy * AW + sx);
    while (heap_n > 0) {
        int32_t v = heap_pop();
        int cost = v / (AW * AH), cell = v % (AW * AH);
        int x = cell % AW, y = cell / AW;
        if (cost > dist[y][x]) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= AW || ny >= AH) continue;
            if (!can_step(ar, nx, ny, swamp_ok)) continue;
            int nc = cost + (blocked[ny][nx] == 2 ? 12 : 1) + (wtile(ar, nx, ny) == T_SWAMP && !holding(TL_BOOTS) ? 8 : 0);
            if (dist[ny][nx] >= 0 && dist[ny][nx] <= nc) continue;
            dist[ny][nx] = (int16_t)nc;
            parent[ny][nx] = (int16_t)d;
            if (heap_n < AW * AH * 2 - 1) heap_push(nc * (AW * AH) + ny * AW + nx);
        }
    }
}

static int D(int wx, int wy) { return wx < 0 || wy < 0 || wx >= AW || wy >= AH ? -1 : dist[wy][wx]; }

/* the first step on the way to (tx, ty): a direction, or -1 */
static int first_step(int tx, int ty) {
    if (D(tx, ty) < 0) return -1;
    int x = tx, y = ty, last = -1;
    while (dist[y][x] > 0) {
        int d = parent[y][x];
        last = d;
        x -= DX[d];
        y -= DY[d];
    }
    return last;
}

/* ------------------------------------------------------------------------ */
/* the job                                                                  */

typedef struct Job {
    int kind;
    int wx, wy;     /* where to stand (current area) */
    int face;       /* then face this way, -1 = no need */
    int act;        /* 1 = press A there, 2 = hold A, 3 = talk (B) there, 4 = push toward face */
    int need[2], nneed;
} Job;

static int thing_wx(const LflThing *t) { return lp.room[t->room].gx * LFL_RW + (t->x + 6) / LFL_TILE; }
static int thing_wy(const LflThing *t) { return lp.room[t->room].gy * LFL_RH + (t->y + 6) / LFL_TILE; }

static int tool_thing(int tool) {
    for (int i = 0; i < lp.nthing; i++)
        if (lp.thing[i].alive && lp.thing[i].kind == TH_TOOL && lp.thing[i].sub == tool) return i;
    return -1;
}

static const int KEY[GK_COUNT] = {TL_BOOTS, TL_DISC, TL_HATCHER, TL_RIVET, TL_PODS, TL_THUMPER, TL_CHOPS};

/* a gate's obstacle still there? (swamps and holes never go away) */
static bool gate_shut(int g) {
    const LflGate *gt = &lp.gate[g];
    switch (gt->kind) {
    case GK_SNAIL: return gt->snail != 1;
    case GK_PIT: return false;
    case GK_SWAMP: return !holding(TL_BOOTS);
    case GK_HOLE: return !holding(TL_HATCHER) && !small();
    default: return !lfl_gate_open(g);
    }
}

/* stand on a tile next to the gate's obstacle, facing it */
static bool gate_stand(int g, int *wx, int *wy, int *face) {
    const LflGate *gt = &lp.gate[g];
    int room = gt->kind == GK_SNAIL ? gt->snail_room : gt->room;
    if (lp.room[room].area != area()) return false;
    int ox = lp.room[room].gx * LFL_RW, oy = lp.room[room].gy * LFL_RH;
    int best = 1 << 30;
    for (int y = gt->ty; y < gt->ty + gt->h; y++)
        for (int x = gt->tx; x < gt->tx + gt->w; x++) {
            int t = lp.room[room].t[y][x];
            if (gt->kind != GK_SNAIL && t != T_CRYSTAL && t != T_FOLIAGE && t != T_TECH && t != T_FLESH) continue;
            if (gt->nook && !lfl_nook_door(g, x, y)) continue;
            for (int d = 0; d < 4; d++) {
                int sx = ox + x - DX[d], sy = oy + y - DY[d];
                int dd = D(sx, sy);
                if (dd < 0 || dd >= best) continue;
                best = dd;
                *wx = sx;
                *wy = sy;
                *face = d;
            }
        }
    return best < (1 << 30);
}

static bool have_res(int tool, int n) {
    int r = lp.cost_res[tool];
    return r >= RS_COUNT || lg.res[r] >= n;
}

/* the tools a job wants in hand; also keep a weapon when there is room */
static int weapon_held(void) {
    if (holding(TL_RIVET)) return TL_RIVET;
    if (holding(TL_DISC)) return TL_DISC;
    return -1;
}

static bool want_tool(const Job *j, int tool) {
    for (int k = 0; k < j->nneed; k++)
        if (j->need[k] == tool) return true;
    return false;
}

/* ------------------------------------------------------------------------ */

static int stairs_target(int *wx, int *wy) {
    /* the way to the other area: the stairs in this one */
    int room = area() == AREA_OVER ? lp.dun_entry_room : lp.dun_room[0];
    int want = area() == AREA_OVER ? T_STAIRS_DN : T_STAIRS_UP;
    for (int y = 0; y < LFL_RH; y++)
        for (int x = 0; x < LFL_RW; x++)
            if (lp.room[room].t[y][x] == want) {
                *wx = lp.room[room].gx * LFL_RW + x;
                *wy = lp.room[room].gy * LFL_RH + y;
                return 1;
            }
    return 0;
}

/* walk to a thing, through the stairs if it is in the other area */
static bool job_walk_to_room_tile(Job *j, int room, int tx, int ty) {
    if (lp.room[room].area != area()) {
        int sx, sy;
        if (!stairs_target(&sx, &sy) || D(sx, sy) < 0) return false;
        j->wx = sx;
        j->wy = sy;
        return true;
    }
    int wx = lp.room[room].gx * LFL_RW + tx, wy = lp.room[room].gy * LFL_RH + ty;
    if (D(wx, wy) < 0) return false;
    j->wx = wx;
    j->wy = wy;
    return true;
}

static bool job_thing(Job *j, int i) {
    const LflThing *t = &lp.thing[i];
    return job_walk_to_room_tile(j, t->room, (t->x + 6) / LFL_TILE, (t->y + 6) / LFL_TILE);
}

/* the disc over pits: a tile in line with the prize, the way clear for it */
static bool job_disc(Job *j, int i) {
    const LflThing *t = &lp.thing[i];
    if (lp.room[t->room].area != area()) return false;
    int px = thing_wx(t), py = thing_wy(t), best = 1 << 30;
    for (int d = 0; d < 4; d++)
        for (int k = 2; k <= 7; k++) {
            int sx = px + DX[d] * k, sy = py + DY[d] * k;
            bool clear = true;
            for (int m = 1; m < k && clear; m++) {
                int tt = wtile(area(), px + DX[d] * m, py + DY[d] * m);
                clear = tt == T_FLOOR || tt == T_PIT || tt == T_SWAMP || tt == T_WOOD || tt == T_TECHDOWN;
            }
            if (!clear || room_of(area(), sx, sy) != t->room) continue;
            int dd = D(sx, sy);
            if (dd < 0 || dd >= best) continue;
            best = dd;
            j->wx = sx;
            j->wy = sy;
            j->face = (d + 2) & 3;
        }
    if (best == (1 << 30)) return false;
    j->act = 1;
    j->need[0] = TL_DISC;
    j->nneed = 1;
    return true;
}

/* open (or get through) gate g; false if it can't be worked on now */
static bool job_gate(Job *j, int g) {
    const LflGate *gt = &lp.gate[g];
    int key = KEY[gt->kind];
    j->kind = TK_GATE;
    j->need[0] = key;
    j->nneed = 1;
    int room = gt->kind == GK_SNAIL ? gt->snail_room : gt->room;
    if (lp.room[room].area != area()) {
        int sx, sy;
        if (!stairs_target(&sx, &sy) || D(sx, sy) < 0) return false;
        j->wx = sx; j->wy = sy;
        return true;
    }
    switch (gt->kind) {
    case GK_CRYSTAL:
    case GK_FOLIAGE:
        if (!gate_stand(g, &j->wx, &j->wy, &j->face)) return false;
        j->act = gt->kind == GK_CRYSTAL ? 2 : 1;
        if (gt->kind == GK_FOLIAGE && !have_res(TL_PODS, 1)) return false;
        return true;
    case GK_TECH: {
        if (!have_res(TL_THUMPER, 1)) return false;
        if (lg.cur == room) { j->wx = my_wx(); j->wy = my_wy(); j->act = 1; j->face = -1; return true; }
        if (!gate_stand(g, &j->wx, &j->wy, &j->face)) return false;
        j->act = 0;
        return true;
    }
    case GK_SNAIL: {
        if (!have_res(TL_CHOPS, 1)) return false;
        /* a chop four tiles in and four to the side draws it out of the way */
        int out = gt->role == GR_DUN ? gt->side : (gt->side + 2) & 3;
        int l0 = (out == DIR_LEFT || out == DIR_RIGHT) ? 4 : 8;
        int ox = lp.room[room].gx * LFL_RW, oy = lp.room[room].gy * LFL_RH;
        int best = 1 << 30;
        for (int sgn = -1; sgn <= 1; sgn += 2)
            for (int lat = 4; lat <= 6; lat++)
                for (int dep = 3; dep <= 5; dep++) {
                    int l = sgn < 0 ? l0 - lat + 1 : l0 + lat, x, y;
                    if (out == DIR_LEFT) { x = dep; y = l; }
                    else if (out == DIR_RIGHT) { x = LFL_RW - 1 - dep; y = l; }
                    else if (out == DIR_UP) { x = l; y = dep; }
                    else { x = l; y = LFL_RH - 1 - dep; }
                    if (x < 1 || y < 1 || x >= LFL_RW - 1 || y >= LFL_RH - 1) continue;
                    /* she faces the lane, the chop lands a tile nearer it */
                    int face = (out == DIR_LEFT || out == DIR_RIGHT) ? (sgn < 0 ? DIR_DOWN : DIR_UP) : (sgn < 0 ? DIR_RIGHT : DIR_LEFT);
                    int fx = x + DX[face], fy = y + DY[face];
                    if (wtile(area(), ox + fx, oy + fy) != T_FLOOR) continue;
                    int dd = D(ox + x, oy + y);
                    if (dd < 0 || dd >= best) continue;
                    best = dd;
                    j->wx = ox + x; j->wy = oy + y; j->face = face;
                }
        if (best == (1 << 30)) return false;
        j->act = 1;
        chop_gate = g;
        return true;
    }
    default:
        /* bog and hole: just carry the key; the walk does the rest */
        return false;
    }
}

static bool job_resources(Job *j, int r, int n);

/* work on gate g, or first gather what its tool burns */
static bool job_gate_fuel(Job *j, int g) {
    int key = KEY[lp.gate[g].kind];
    int r = lp.cost_res[key];
    if (r < RS_COUNT && lg.res[r] < 1) return job_resources(j, r, 3);
    return job_gate(j, g);
}

/* the trade at village v's giver: stand below, face up, talk */
static bool job_trade(Job *j, int t) {
    const LflTrade *tr = &lp.trade[t];
    const LflVillage *v = NULL;
    for (int k = 0; k < lp.nvillage; k++)
        if (lp.village[k].trade == t) v = &lp.village[k];
    if (!v || lp.room[tr->room].area != area()) return false;
    int wx = lp.room[tr->room].gx * LFL_RW + v->gx, wy = lp.room[tr->room].gy * LFL_RH + v->gy + 1;
    if (D(wx, wy) < 0) return false;
    j->kind = TK_TRADE;
    j->wx = wx; j->wy = wy; j->face = DIR_UP; j->act = 3;
    if (tr->glyph) { j->need[0] = TL_TALKBOX; j->nneed = 1; }
    lfl_bot_task = TK_TRADE;
    return true;
}

/* the cheapest way to more of resource r */
static bool job_gather(Job *j, int r) {
    int best = 1 << 30;
    int ar = area();
    /* lying in this room */
    for (int i = 0; i < LFL_MAX_DROPS; i++) {
        const LflDrop *d = &lg.drop[i];
        if (!d->alive || d->kind != DR_RES || d->sub != r) continue;
        int wx = lp.room[lg.cur].gx * LFL_RW + (LFL_PX(d->x) + 4) / LFL_TILE, wy = lp.room[lg.cur].gy * LFL_RH + (LFL_PX(d->y) + 4) / LFL_TILE;
        int dd = D(wx, wy);
        if (dd >= 0 && dd < best) { best = dd; j->wx = wx; j->wy = wy; }
    }
    /* in other rooms: still there, or there again once she is three rooms away */
    for (int rm = 0; rm < LFL_MAX_ROOMS; rm++) {
        const LflRoom *q = &lp.room[rm];
        if (!q->exists || q->area != ar || rm == lg.cur) continue;
        bool reset = iabs(q->gx - lp.room[lg.cur].gx) + iabs(q->gy - lp.room[lg.cur].gy) >= 3;
        for (int i = 0; i < q->nspawn; i++) {
            const LflSpawn *s = &q->spawn[i];
            if (s->type != SPW_RES || s->sub != r || (s->taken && !reset)) continue;
            int wx = q->gx * LFL_RW + s->tx, wy = q->gy * LFL_RH + s->ty;
            int dd = D(wx, wy);
            if (dd >= 0 && dd + 4 < best) { best = dd + 4; j->wx = wx; j->wy = wy; }
        }
    }
    if (best < (1 << 30)) { j->kind = TK_GATHER; return true; }
    return false;
}

/* a trade that turns something plentiful into r */
static int res_trade_for(int r, int want) {
    int best = -1, bscore = -1;
    for (int t = 0; t < 6; t++) {
        const LflTrade *tr = &lp.trade[t];
        if (tr->kind != TR_RES || tr->get_res != r) continue;
        if (tr->glyph && !holding(TL_TALKBOX)) continue;
        if (lg.res[tr->pay_res] < tr->pay_n) continue;
        if (tr->pay_res == lp.trade[0].pay_res && lp.trade[0].kind == TR_MAP && !lp.trade[0].done) continue;
        int score = tr->get_n * 100 / tr->pay_n;
        (void)want;
        if (score > bscore) { bscore = score; best = t; }
    }
    return best;
}

/* creatures to knock down for resource r: the species that drop it (and
 * the locals for the rare one); -1 = any hostile */
static bool drops_res(const LflMob *m, int r) {
    if (m->flags & MF_MORPHED) return r < 0 || lp.tier_of[r] == TIER_MORE;
    if (m->sp == SP_GNAT) return r < 0 || lp.tier_of[r] == TIER_RARE;
    const LflSpecies *s = &lp.sp[m->sp];
    if (s->friendly) return r >= 0 && lp.tier_of[r] == TIER_RARE;
    return r < 0 || s->drop == r;
}

static bool job_hunt(Job *j, int r) {
    int best = 1 << 30;
    int ox = lp.room[lg.cur].gx * LFL_RW, oy = lp.room[lg.cur].gy * LFL_RH;
    for (int i = 0; i < LFL_MAX_MOBS; i++) {
        const LflMob *m = &lg.mob[i];
        if (!m->alive || (m->flags & (MF_SHELL | MF_GIVER)) || m->sp == SP_SNAIL || !drops_res(m, r)) continue;
        int mx = (LFL_PX(m->x) + m->w / 2) / LFL_TILE, my = (LFL_PX(m->y) + m->h / 2) / LFL_TILE;
        for (int d = 0; d < 4; d++)
            for (int k = 2; k <= 3; k++) {
                int sx = ox + mx + DX[d] * k, sy = oy + my + DY[d] * k;
                int dd = D(sx, sy);
                if (dd < 0 || dd >= best) continue;
                best = dd;
                j->wx = sx; j->wy = sy; j->face = (d + 2) & 3; j->act = 2;
            }
    }
    if (best == (1 << 30)) return false;
    j->kind = TK_FIGHT;
    return true;
}

/* a room (not this one) where creatures that drop r live */
static bool job_hunt_room(Job *j, int r) {
    int best = 1 << 30, cur_gx = lp.room[lg.cur].gx, cur_gy = lp.room[lg.cur].gy;
    for (int rm = 0; rm < LFL_MAX_ROOMS; rm++) {
        const LflRoom *q = &lp.room[rm];
        if (!q->exists || q->area != area() || rm == lg.cur) continue;
        bool back = iabs(q->gx - cur_gx) + iabs(q->gy - cur_gy) >= 3 || !q->dirty;
        for (int i = 0; i < q->nspawn; i++) {
            const LflSpawn *sp = &q->spawn[i];
            if (sp->type != SPW_MOB || (sp->taken && !back) || (sp->sub & 0x80)) continue;
            int sp_i = sp->sub & 0x7F;
            bool ok = lp.sp[sp_i].friendly ? lp.tier_of[r] == TIER_RARE : lp.sp[sp_i].drop == r;
            if (!ok) continue;
            int wx = q->gx * LFL_RW + sp->tx, wy = q->gy * LFL_RH + sp->ty;
            for (int d = 0; d < 4; d++) {
                int dd = D(wx + DX[d] * 2, wy + DY[d] * 2);
                if (dd >= 0 && dd < best) { best = dd; j->wx = wx + DX[d] * 2; j->wy = wy + DY[d] * 2; }
            }
        }
    }
    if (best == (1 << 30)) return false;
    j->kind = TK_WANDER;
    return true;
}

static int map_trade(void) {
    for (int t = 0; t < 6; t++)
        if (lp.trade[t].kind == TR_MAP) return t;
    return 0;
}

static int find_gate(int role) {
    for (int g = 0; g < lp.ngate; g++)
        if (lp.gate[g].role == role) return g;
    return -1;
}

/* how to get hold of tool t: a job, or false */
static bool job_get_tool(Job *j, int t, int depth);

/* the job that gets r up to n */
static bool job_secret(Job *j);
static int find_gate(int role);

static bool job_resources_in(Job *j, int r, int n);

/* (guarded: the routes call each other) */
static bool job_resources(Job *j, int r, int n) {
    static int depth;
    if (depth >= 3) return false;
    depth++;
    bool ok = job_resources_in(j, r, n);
    depth--;
    return ok;
}

static bool job_resources_in(Job *j, int r, int n) {
    lfl_bot_task = TK_GATHER;
    if (lg.res[r] >= n) return false;
    int tr = res_trade_for(r, n);
    if (tr >= 0 && job_trade(j, tr)) return true;
    if (job_gather(j, r)) return true;
    if (lp.tier_of[r] == TIER_RARE) {
        /* the two behind their gate, the four behind the hidden wall */
        int g = find_gate(GR_RARE);
        /* (not when its key burns the rare one and she has none: a loop) */
        bool fuel_loop = g >= 0 && lp.cost_res[KEY[lp.gate[g].kind]] == r && lg.res[r] < 1 && gate_shut(g);
        if (g >= 0 && !fuel_loop && !lp.gate[g].used && lp.thing[lp.gate[g].prize_thing].alive) {
            int i = lp.gate[g].prize_thing, key = KEY[lp.gate[g].kind];
            if (lp.gate[g].kind == GK_PIT) {
                if (holding(TL_DISC) && job_disc(j, i)) { j->kind = TK_PRIZE; return true; }
            } else if (job_thing(j, i)) {
                j->kind = TK_PRIZE;
                if (lp.gate[g].kind == GK_SWAMP || lp.gate[g].kind == GK_HOLE) { j->need[0] = key; j->nneed = 1; }
                return true;
            } else if (holding(key) && job_gate_fuel(j, g)) return true;
            else if (!holding(key) && job_get_tool(j, key, 1)) return true;
        }
        if (job_secret(j)) return true;
    }
    /* creatures that drop it: here, or in rooms where they live */
    if (weapon_held() < 0) {
        if (job_get_tool(j, TL_RIVET, 1) || job_get_tool(j, TL_DISC, 1)) return true;
        return false;
    }
    if (job_hunt(j, r)) return true;
    if (job_hunt_room(j, r)) return true;
    return false;
}

static bool job_get_tool(Job *j, int t, int depth) {
    if (holding(t)) return false;
    int i = tool_thing(t);
    if (i >= 0) {
        /* lying somewhere: behind a pit, or just there */
        for (int g = 0; g < lp.ngate; g++)
            if (lp.gate[g].prize_thing == i && lp.gate[g].kind == GK_PIT && !lp.gate[g].used) {
                if (lp.room[lp.thing[i].room].area == area() && holding(TL_DISC) && job_disc(j, i)) { j->kind = TK_PRIZE; return true; }
                if (!holding(TL_DISC) && depth < 2) return job_get_tool(j, TL_DISC, depth + 1);
                return false;
            }
        if (job_thing(j, i)) { j->kind = TK_TOOL; j->need[0] = t; j->nneed = 1; return true; }
        /* behind a gate: open it */
        for (int g = 0; g < lp.ngate; g++)
            if (lp.gate[g].prize_thing == i && !lp.gate[g].used && gate_shut(g)) {
                int key = KEY[lp.gate[g].kind];
                if (!holding(key)) return depth < 2 && job_get_tool(j, key, depth + 1);
                return job_gate_fuel(j, g);
            }
        return false;
    }
    /* sold: pay for it */
    for (int k = 0; k < 6; k++) {
        const LflTrade *tr = &lp.trade[k];
        if (tr->kind != TR_ITEM || tr->item != t || tr->done) continue;
        /* the price first (gathering it wants no talk box), then the words */
        if (lg.res[tr->pay_res] < tr->pay_n) return job_resources(j, tr->pay_res, tr->pay_n);
        if (tr->glyph && !holding(TL_TALKBOX)) return depth < 2 && job_get_tool(j, TL_TALKBOX, depth + 1);
        return job_trade(j, k);
    }
    return false;
}

/* the hidden wall: open it with its key, then take the bundle behind */
static bool job_secret(Job *j) {
    if (lp.secret_room == 255) return false;
    int bi = -1;
    for (int i = 0; i < lp.nthing; i++)
        if (lp.thing[i].alive && lp.thing[i].kind == TH_BUNDLE && lp.thing[i].room == lp.secret_room) bi = i;
    if (bi < 0 || lp.room[lp.secret_room].area != area()) return false;
    const LflRoom *r = &lp.room[lp.secret_room];
    bool shut = r->t[(int)r->secret_y][(int)r->secret_x] == T_SECRET;
    if (!shut || (lp.secret_key == SK_SMALL && (small() || holding(TL_HATCHER)))) {
        if (job_thing(j, bi)) { j->kind = TK_PRIZE; if (lp.secret_key == SK_SMALL) { j->need[0] = TL_HATCHER; j->nneed = 1; } return true; }
    }
    if (!shut) return false;
    static const int SKEY[SK_COUNT] = {TL_THUMPER, TL_DISC, TL_PODS, TL_RIVET, TL_HATCHER};
    int key = SKEY[lp.secret_key];
    int r_need = lp.cost_res[key];
    /* (a key that burns the rare one, with none: the rare one comes first) */
    if (r_need < RS_COUNT && lp.tier_of[r_need] == TIER_RARE && lg.res[r_need] < 1) return false;
    if (!holding(key)) return job_get_tool(j, key, 1);
    if (r_need < RS_COUNT && lg.res[r_need] < 1) return job_resources(j, r_need, 2);
    int ox = r->gx * LFL_RW, oy = r->gy * LFL_RH, best = 1 << 30;
    for (int d = 0; d < 4; d++)
        for (int k = 1; k <= (key == TL_DISC || key == TL_RIVET ? 3 : 1); k++) {
            int sx = ox + r->secret_x - DX[d] * k, sy = oy + r->secret_y - DY[d] * k;
            /* a clear line from her to the wall (a rock in between stops the shot) */
            int between = wtile(area(), sx + DX[d], sy + DY[d]);
            if (k > 1 && !(between == T_FLOOR || between == T_WOOD || between == T_TECHDOWN || between == T_SWAMP || between == T_PIT)) break;
            int dd = D(sx, sy);
            if (dd < 0 || dd >= best) continue;
            best = dd;
            j->wx = sx; j->wy = sy; j->face = d;
        }
    if (best == (1 << 30)) return false;
    j->kind = TK_PRIZE;
    j->act = key == TL_RIVET ? 2 : 1;
    j->need[0] = key;
    j->nneed = 1;
    return true;
}

/* the tools the plan still wants: the keys for the chart pieces and the
 * dungeon, the talk box for a glyph trade */
static unsigned keep_set(void) {
    unsigned k = 0;
    int mt = map_trade();
    if (!lp.trade[mt].done && lp.trade[mt].glyph) k |= 1u << TL_TALKBOX;
    int gm = find_gate(GR_MAP);
    if (gm >= 0 && lp.gate[gm].prize_thing != 255 && lp.thing[lp.gate[gm].prize_thing].alive) k |= 1u << KEY[lp.gate[gm].kind];
    bool dun_left = false;
    for (int i = 0; i < lp.nthing; i++)
        if (lp.thing[i].alive && lp.thing[i].kind == TH_MAP && lp.room[lp.thing[i].room].area == AREA_DUN) dun_left = true;
    if (dun_left)
        for (int g = 0; g < lp.ngate; g++)
            if ((lp.gate[g].role == GR_DUN || lp.gate[g].role == GR_DUNGEON) &&
                (gate_shut(g) || lp.gate[g].kind == GK_SWAMP || lp.gate[g].kind == GK_HOLE))
                k |= 1u << KEY[lp.gate[g].kind];
    return k;
}

static void choose(Job *j) {
    memset(j, 0, sizeof *j);
    j->face = -1;
    j->wx = j->wy = -1;
    LflPlayer *p = &lg.pl;
    /* a pod about to go: keep away, but in this room (leaving it would take
     * the pod away too); in a narrow way with no room, as far as she can */
    for (int i = 0; i < LFL_MAX_BOMBS; i++)
        if (lg.bomb[i].alive) {
            int bx = lp.room[lg.cur].gx * LFL_RW + LFL_PX(lg.bomb[i].x) / LFL_TILE, by = lp.room[lg.cur].gy * LFL_RH + LFL_PX(lg.bomb[i].y) / LFL_TILE;
            int best = 1 << 30;
            for (int y = by - 6; y <= by + 6; y++)
                for (int x = bx - 6; x <= bx + 6; x++) {
                    int dd = D(x, y);
                    if (dd < 0 || room_of(area(), x, y) != lg.cur) continue;
                    int sc = (3 - imin(3, imax(iabs(x - bx), iabs(y - by)))) * 1000 + dd;
                    if (sc < best) { best = sc; j->wx = x; j->wy = y; }
                }
            j->kind = TK_RETREAT;
            return;
        }
    /* a chop laid for a snail: stand well aside until it has moved */
    if (chop_gate >= 0 && chop_t > 0) {
        chop_t--;
        const LflGate *gt = &lp.gate[chop_gate];
        if (gt->snail == 1 && lfl_count_mobs(MF_LURED, 0) >= 0) {
            bool chop_left = false;
            for (int i = 0; i < LFL_MAX_DROPS; i++) chop_left |= lg.drop[i].alive && lg.drop[i].kind == DR_CHOP;
            if (!chop_left) chop_t = 0;
        }
        if (chop_t > 0) {
            int bx = my_wx(), by = my_wy(), best = -1;
            for (int y = by - 6; y <= by + 6; y++)
                for (int x = bx - 8; x <= bx + 8; x++) {
                    int dd = D(x, y);
                    if (dd < 0 || room_of(area(), x, y) != lg.cur) continue;
                    int sc = 0;
                    for (int i = 0; i < LFL_MAX_MOBS; i++)
                        if (lg.mob[i].alive && lg.mob[i].sp == SP_SNAIL)
                            sc += iabs(x * 16 - (lp.room[lg.cur].gx * LFL_RW * 16 + LFL_PX(lg.mob[i].x) + 14)) +
                                  iabs(y * 16 - (lp.room[lg.cur].gy * LFL_RH * 16 + LFL_PX(lg.mob[i].y) + 14));
                    sc -= dd * 4;
                    if (sc > best) { best = sc; j->wx = x; j->wy = y; }
                }
            j->kind = TK_RETREAT;
            return;
        }
    }
    /* home with the whole chart */
    if (lg.maps >= 3) {
        if (job_walk_to_room_tile(j, lp.ship_room, lp.ship_tx, lp.ship_ty + 2)) {
            j->kind = TK_SHIP;
            if (lg.cur == lp.ship_room && my_wx() == j->wx && my_wy() == j->wy) { j->face = DIR_UP; j->act = 4; }
            return;
        }
    }
    /* hungry and hurt: a chop in the room */
    if (p->hp < p->hpmax)
        for (int i = 0; i < LFL_MAX_DROPS; i++)
            if (lg.drop[i].alive && lg.drop[i].kind == DR_CHOP) {
                int wx = lp.room[lg.cur].gx * LFL_RW + (LFL_PX(lg.drop[i].x) + 4) / LFL_TILE, wy = lp.room[lg.cur].gy * LFL_RH + (LFL_PX(lg.drop[i].y) + 4) / LFL_TILE;
                if (D(wx, wy) >= 0) { j->kind = TK_EAT; j->wx = wx; j->wy = wy; return; }
            }
    /* energy cells first: every one is a point of energy for the rest */
    {
        int best = 1 << 30, bi = -1;
        for (int i = 0; i < lp.nthing; i++) {
            const LflThing *t = &lp.thing[i];
            if (!t->alive || t->kind != TH_CUBE || lp.room[t->room].area != area()) continue;
            int dd = D(thing_wx(t), thing_wy(t));
            if (dd >= 0 && dd < best) { best = dd; bi = i; }
        }
        if (bi >= 0 && job_thing(j, bi)) { j->kind = TK_CUBE; return; }
    }
    /* a weapon early on, while a hand is free */
    if (weapon_held() < 0 && (p->hold[0] < 0 || p->hold[1] < 0)) {
        int i = tool_thing(TL_RIVET);
        if (i < 0 || !job_thing(j, i)) i = tool_thing(TL_DISC);
        if (i >= 0 && job_thing(j, i)) { j->kind = TK_TOOL; j->need[0] = lp.thing[i].sub; j->nneed = 1; return; }
    }
    /* down in the dungeon with its chart piece still there: that first (the
     * jobs above ground would only send her up and down the stairs) */
    bool below = false;
    if (area() == AREA_DUN)
        for (int i = 0; i < lp.nthing; i++)
            if (lp.thing[i].alive && lp.thing[i].kind == TH_MAP && lp.room[lp.thing[i].room].area == AREA_DUN) below = true;
    /* 1: the chart piece for sale */
    int mt = map_trade();
    const LflTrade *mtr = &lp.trade[mt];
    if (!below && !mtr->done && lg.res[mtr->pay_res] >= mtr->pay_n) {
        if (mtr->glyph && !holding(TL_TALKBOX)) {
            if (job_get_tool(j, TL_TALKBOX, 0)) return;
        } else if (job_trade(j, mt)) return;
    }
    /* 2: the chart piece behind its gate */
    int gm = find_gate(GR_MAP);
    if (!below && gm >= 0 && lp.gate[gm].prize_thing != 255 && lp.thing[lp.gate[gm].prize_thing].alive) {
        int i = lp.gate[gm].prize_thing;
        if (lp.gate[gm].kind == GK_PIT && !lp.gate[gm].used) {
            if (holding(TL_DISC) && job_disc(j, i)) { j->kind = TK_PRIZE; return; }
            if (job_get_tool(j, TL_DISC, 0)) return;
        } else {
            if (job_thing(j, i)) { j->kind = TK_PRIZE; j->need[0] = KEY[lp.gate[gm].kind]; j->nneed = gate_shut(gm) || lp.gate[gm].kind == GK_SWAMP || lp.gate[gm].kind == GK_HOLE; return; }
            int key = KEY[lp.gate[gm].kind];
            if (holding(key) && job_gate_fuel(j, gm)) return;
            if (!holding(key) && job_get_tool(j, key, 0)) return;
        }
    }
    /* 3: the dungeon */
    {
        int gd = find_gate(GR_DUNGEON);
        int last = lp.dun_room[LFL_DUN_ROOMS - 1];
        int mi = -1;
        for (int i = 0; i < lp.nthing; i++)
            if (lp.thing[i].alive && lp.thing[i].kind == TH_MAP && lp.thing[i].room == last) mi = i;
        if (mi >= 0) {
            /* the keys the trip wants: the way in, then each gate inside */
            int keys[4], nk = 0;
            const LflGate *gg = &lp.gate[gd];
            if (gg->kind == GK_SWAMP || gg->kind == GK_HOLE || gate_shut(gd)) keys[nk++] = KEY[gg->kind];
            for (int g = 0; g < lp.ngate; g++)
                if (lp.gate[g].role == GR_DUN && (lp.gate[g].kind == GK_SWAMP || lp.gate[g].kind == GK_HOLE || gate_shut(g))) keys[nk++] = KEY[lp.gate[g].kind];
            if (area() == AREA_OVER && gate_shut(gd) && gg->kind != GK_SWAMP && gg->kind != GK_HOLE) {
                int key = KEY[gg->kind];
                if (holding(key) && job_gate_fuel(j, gd)) return;
                if (!holding(key) && job_get_tool(j, key, 0)) return;
            }
            /* fuel for the gates inside, before going down (gathered first: the
             * gathering may want other tools in hand) */
            if (area() == AREA_OVER)
                for (int g = 0; g < lp.ngate; g++) {
                    if (lp.gate[g].role != GR_DUN || !gate_shut(g)) continue;
                    int r = lp.cost_res[KEY[lp.gate[g].kind]];
                    if (r < RS_COUNT && lg.res[r] < 2 && job_resources(j, r, 3)) return;
                }
            /* carry what the dungeon needs, two at a time */
            int want[2], nw = 0;
            for (int k = 0; k < nk && nw < 2; k++) {
                bool dup = false;
                for (int m = 0; m < nw; m++) dup |= want[m] == keys[k];
                if (!dup) want[nw++] = keys[k];
            }
            for (int k = 0; k < nw; k++)
                if (!holding(want[k]) && job_get_tool(j, want[k], 0)) {
                    /* fetching one of the trip's tools: keep both in hand (a
                     * tool fetched on the way, say the talk box to buy one,
                     * keeps its own need) */
                    bool trip = j->kind == TK_TOOL && (j->need[0] == want[0] || (nw > 1 && j->need[0] == want[1]));
                    if (trip) { j->need[0] = want[0]; j->need[1] = nw > 1 ? want[1] : -1; j->nneed = nw; }
                    return;
                }
            if (job_thing(j, mi)) { j->kind = TK_DUNGEON; memcpy(j->need, want, sizeof want); j->nneed = nw; return; }
            /* a gate inside in the way */
            for (int g = 0; g < lp.ngate; g++)
                if (lp.gate[g].role == GR_DUN && gate_shut(g) && lp.gate[g].kind != GK_SWAMP && lp.gate[g].kind != GK_HOLE) {
                    int key = KEY[lp.gate[g].kind];
                    if (holding(key) && job_gate_fuel(j, g)) return;
                    if (!holding(key) && job_get_tool(j, key, 0)) return;
                }
        }
    }
    /* 4: save up for the chart piece */
    if (!mtr->done && job_resources(j, mtr->pay_res, mtr->pay_n)) return;
    /* nothing else to do: walk to a far room so things come back */
    if (wander_room < 0 || wander_room == lg.cur) wander_room = lp.village[(lg.frame_t / 600) % lp.nvillage].room;
    if (job_walk_to_room_tile(j, wander_room, 8, 4)) { j->kind = TK_WANDER; return; }
}

/* ------------------------------------------------------------------------ */
/* buttons                                                                  */

static int dir_button(int d) {
    static const int B[4] = {BTN_UP, BTN_RIGHT, BTN_DOWN, BTN_LEFT};
    return B[d & 3];
}

static int press_a(void) {
    /* A goes down fresh, not held over from the frame before */
    return (prev_mask & BTN_A) ? 0 : BTN_A;
}

static int steer(int d) {
    /* line up on the next tile's middle, then step */
    int cx = pcx() % LFL_TILE, cy = pcy() % LFL_TILE;
    if (d == DIR_LEFT || d == DIR_RIGHT) {
        if (cy < 6) return BTN_DOWN;
        if (cy > 10) return BTN_UP;
    } else {
        if (cx < 6) return BTN_RIGHT;
        if (cx > 10) return BTN_LEFT;
    }
    return dir_button(d);
}

static bool free_px(int x, int y) {
    /* Wick's box, centred on (x, y), clear of walls */
    int w = pw();
    return !lfl_tile_solid_at(x - w / 2, y - w / 2, small()) && !lfl_tile_solid_at(x + w / 2, y - w / 2, small()) &&
           !lfl_tile_solid_at(x - w / 2, y + w / 2, small()) && !lfl_tile_solid_at(x + w / 2, y + w / 2, small());
}

static bool line_clear(int x0, int y0, int x1, int y1) {
    int n = imax(iabs(x1 - x0), iabs(y1 - y0)) / 6;
    for (int k = 1; k < n; k++) {
        int x = x0 + (x1 - x0) * k / n, y = y0 + (y1 - y0) * k / n;
        int t = lfl_tile(lg.cur, x / LFL_TILE, y / LFL_TILE);
        if (!(t == T_FLOOR || t == T_WOOD || t == T_SWAMP || t == T_PIT || t == T_TECHDOWN || t == T_STAIRS_DN || t == T_STAIRS_UP)) return false;
    }
    return true;
}

static bool is_threat(const LflMob *m) {
    if (!m->alive || (m->flags & (MF_SHELL | MF_MORPHED | MF_GIVER)) || m->sp == SP_SNAIL) return false;
    if ((m->flags & MF_FRIENDLY) && !(m->flags & MF_ANGRY)) return lfl_bot_task == TK_FIGHT;
    return true;
}

/* dodge shots, line up on creatures close by and shoot them */
static int fight(void) {
    int px = pcx(), py = pcy();
    /* a shot coming straight at her: step out of its line */
    for (int i = 0; i < LFL_MAX_SHOTS; i++) {
        const LflShot *s = &lg.shot[i];
        if (!s->alive || (s->kind != SH_FOE && s->kind != SH_SHARD)) continue;
        int sx = LFL_PX(s->x) + 2, sy = LFL_PX(s->y) + 2;
        int dx = px - sx, dy = py - sy;
        if (iabs(dx) + iabs(dy) > 56) continue;
        if ((long)dx * s->vx + (long)dy * s->vy <= 0) continue;
        /* how far off its line she stands */
        bool horiz = iabs(s->vx) >= iabs(s->vy);
        int off = horiz ? dy : dx;
        if (iabs(off) > 9) continue;
        int sgn = off >= 0 ? 1 : -1;
        for (int k = 0; k < 2; k++, sgn = -sgn) {
            int tx = px + (horiz ? 0 : sgn * 12), ty = py + (horiz ? sgn * 12 : 0);
            if (free_px(tx, ty)) return horiz ? (sgn > 0 ? BTN_DOWN : BTN_UP) : (sgn > 0 ? BTN_RIGHT : BTN_LEFT);
        }
    }
    int tool = lg.pl.hold[lg.pl.act];
    bool armed = tool == TL_RIVET || tool == TL_DISC;
    if (!armed) return 0;
    int best = -1, bd = 1 << 30;
    for (int i = 0; i < LFL_MAX_MOBS; i++) {
        const LflMob *m = &lg.mob[i];
        if (!is_threat(m)) continue;
        int d = iabs(LFL_PX(m->x) + m->w / 2 - px) + iabs(LFL_PX(m->y) + m->h / 2 - py);
        if (d < bd) { bd = d; best = i; }
    }
    if (best < 0 || bd > 84) { fight_busy = 0; return 0; }
    const LflMob *m = &lg.mob[best];
    int mx = LFL_PX(m->x) + m->w / 2, my = LFL_PX(m->y) + m->h / 2;
    int dx = mx - px, dy = my - py, reach = tool == TL_RIVET ? 70 : 130;
    /* right on top of her: any facing hits it (turning would only nudge her
     * from one side of it to the other) */
    if (iabs(dx) <= 6 && iabs(dy) <= 6) return press_a();
    int d = -1;
    if (iabs(dy) <= 5 && iabs(dx) <= reach) d = dx > 0 ? DIR_RIGHT : DIR_LEFT;
    else if (iabs(dx) <= 5 && iabs(dy) <= reach) d = dy > 0 ? DIR_DOWN : DIR_UP;
    if (d >= 0) {
        if (!line_clear(px, py, mx, my)) return 0;
        if (lg.pl.face != d) return dir_button(d); /* a one-frame tap turns her */
        return press_a();
    }
    if (iabs(dx) <= 5 || iabs(dy) <= 5) return 0; /* in line but out of reach */
    if (fight_off > 0) return 0;
    if (++fight_busy > 240) { fight_busy = 0; fight_off = 360; return 0; }
    /* too close and not lined up: back away */
    if (bd < 22) {
        int ax = dx > 0 ? -1 : 1, ay = dy > 0 ? -1 : 1;
        if (iabs(dx) >= iabs(dy) && free_px(px + ax * 12, py)) return ax < 0 ? BTN_LEFT : BTN_RIGHT;
        if (free_px(px, py + ay * 12)) return ay < 0 ? BTN_UP : BTN_DOWN;
        if (free_px(px + ax * 12, py)) return ax < 0 ? BTN_LEFT : BTN_RIGHT;
        return 0;
    }
    /* line up on the nearer axis, when the shot would then be clear and
     * reach (else the walk takes her off the line again, and round it goes) */
    if (iabs(dy) < iabs(dx)) {
        if (iabs(dx) > reach) return 0;
        if (!line_clear(px, my, mx, my)) return 0;
        int sg = isign(dy);
        if (free_px(px, py + sg * 8)) return sg > 0 ? BTN_DOWN : BTN_UP;
    } else {
        if (iabs(dy) > reach) return 0;
        if (!line_clear(mx, py, mx, my)) return 0;
        int sg = isign(dx);
        if (free_px(px + sg * 8, py)) return sg > 0 ? BTN_RIGHT : BTN_LEFT;
    }
    return 0;
}

static int bot_buttons_inner(void);

/* a button held on and on is let go for a frame now and then (a press the
 * game swallowed, say when a talk box closed, only counts once released) */
int lfl_bot_buttons(void) {
    static int same, last;
    int m = bot_buttons_inner();
    same = m && m == last ? same + 1 : 0;
    last = m;
    if (same >= 24) { same = 0; last = 0; prev_mask = 0; m = 0; }
    lfl_bot_last = m;
    return m;
}

static int bot_buttons_inner(void) {
    int mask = 0;
    switch (lg.state) {
    case LS_TITLE:
    case LS_OVER:
        lfl_bot_task = TK_TITLE;
        mask = (lg.frame_t & 8) ? BTN_A : 0;
        prev_mask = (uint32_t)mask;
        return mask;
    case LS_ENDING:
        mask = (lg.frame_t & 8) && lg.state_t > 130 ? BTN_A : 0;
        prev_mask = (uint32_t)mask;
        return mask;
    case LS_TALK:
        /* take the trade it came for; close anything else */
        if (lg.talk_t < 10) { prev_mask = 0; return 0; }
        if (lfl_bot_task == TK_TRADE && lg.talk_trade >= 0 && !lg.talk_glyph) mask = (prev_mask & BTN_B) ? 0 : BTN_B;
        else mask = (prev_mask & BTN_A) ? 0 : BTN_A;
        prev_mask = (uint32_t)mask;
        return mask;
    case LS_PLAY: break;
    default:
        prev_mask = 0;
        return 0;
    }
    if (clear_t > 0) {
        /* stepping well clear of a tool she has to step off first */
        clear_t--;
        mask = dir_button(clear_dir);
        prev_mask = (uint32_t)mask;
        return mask;
    }
    if (b_tap > 0) {
        /* a B tap swaps hands: down one frame, then up */
        b_tap--;
        mask = b_tap ? BTN_B : 0;
        prev_mask = (uint32_t)mask;
        return mask;
    }
    /* just through a doorway: press on for a moment before steering round
     * creatures again (no turning back and forth at the edge) */
    static int was_room = -1, in_room_t;
    if (lg.cur != was_room) { was_room = lg.cur; in_room_t = 0; }
    in_room_t++;
    if (fight_off > 0) fight_off--;
    build_blocks(in_room_t > 45);
    bog_room = lp.trapped_room != 255 && (lg.god || lg.pl.hp >= 5) ? lp.trapped_room : -1;
    bfs(false);
    Job j;
    choose(&j);
    if (j.wx < 0) {
        build_blocks(false);
        bfs(true);
        choose(&j);
    }
    /* keep to the last target unless the new one is clearly nearer (no
     * dithering between two cells as creatures shift the search) */
    static Job last;
    static int last_area = -1;
    if (last_area != area()) last.wx = -1; /* the other area's tiles mean other rooms */
    last_area = area();
    if (j.kind == last.kind && j.act == 0 && last.act == 0 && last.wx >= 0 && D(last.wx, last.wy) > 0 &&
        (j.wx != last.wx || j.wy != last.wy) && D(j.wx, j.wy) >= D(last.wx, last.wy) - 12) {
        j.wx = last.wx;
        j.wy = last.wy;
    }
    last = j;
    lfl_bot_task = j.kind;
    lfl_bot_need = j.nneed > 0 ? (j.need[0] + 1) * 10 + (j.nneed > 1 ? j.need[1] + 1 : 0) : 0;
    lfl_bot_tx = j.wx;
    lfl_bot_ty = j.wy;
    LflPlayer *p = &lg.pl;
    /* hands: the job's tools in hand, the active one ready */
    bool fetching = false;
    for (int k = 0; k < j.nneed; k++) {
        int t = j.need[k];
        if (t < 0 || holding(t)) continue;
        fetching = true;
        int i = tool_thing(t);
        if (i < 0) break;
        /* make the hand that will be swapped out one we can spare: an empty
         * one, else one the plan doesn't want, else not this job's */
        int spare = -1, score = -1;
        unsigned keep = keep_set();
        for (int h = 0; h < 2; h++) {
            int sc = p->hold[h] < 0 ? 3 : want_tool(&j, p->hold[h]) ? 0 : (keep & (1u << p->hold[h])) ? 1 : 2;
            sc = sc * 2 + (h == p->act); /* a tie keeps the active hand: no needless swap */
            if (sc > score) { score = sc; spare = h; }
        }
        if (spare >= 0 && spare != p->act && p->hold[p->act] >= 0 && p->hold[spare] >= 0) {
            int who = lfl_talk_target();
            if (who < 0) { b_tap = 2; prev_mask = BTN_B; return BTN_B; }
            /* a local close by would answer B: step away first */
            int dx = pcx() - (LFL_PX(lg.mob[who].x) + 6), dy = pcy() - (LFL_PX(lg.mob[who].y) + 6);
            int away = iabs(dx) > iabs(dy) ? (dx > 0 ? DIR_RIGHT : DIR_LEFT) : (dy > 0 ? DIR_DOWN : DIR_UP);
            if (!free_px(pcx() + DX[away] * 12, pcy() + DY[away] * 12)) away = (away + 1) & 3;
            if (!free_px(pcx() + DX[away] * 12, pcy() + DY[away] * 12)) away = (away + 2) & 3;
            mask = dir_button(away);
            prev_mask = (uint32_t)mask;
            return mask;
        }
        if (job_thing(&j, i)) { j.act = 0; j.face = -1; }
        break;
    }
    if (j.act && j.act != 3 && j.nneed > 0 && j.need[0] >= 0 && p->hold[p->act] != j.need[0] && holding(j.need[0]) && !small()) {
        if (lfl_talk_target() < 0) { b_tap = 2; prev_mask = BTN_B; return BTN_B; }
    }
    /* at the spot: face, then act */
    if (j.wx >= 0 && my_wx() == j.wx && my_wy() == j.wy) {
        stuck_t = 0;
        /* stairs she arrived on only work once she steps off and back on;
         * so does a tool just set down under her feet */
        int under = wtile(area(), my_wx(), my_wy());
        bool tool_wait = false;
        for (int i = 0; i < lp.nthing && j.act == 0; i++) {
            const LflThing *t = &lp.thing[i];
            if (t->alive && t->kind == TH_TOOL && t->room == lg.cur && t->cd > 0 &&
                rects_overlap(LFL_PX(p->x), LFL_PX(p->y), pw(), pw(), t->x, t->y, 12, 12))
                tool_wait = true;
        }
        if (((under == T_STAIRS_DN || under == T_STAIRS_UP) && lg.stairs_lock) || tool_wait) {
            /* off it, inside this room, two tiles clear if she can */
            int best = -1;
            for (int k = 0; k < 4; k++) {
                int nx = my_wx() + DX[k], ny = my_wy() + DY[k];
                if (D(nx, ny) < 0 || room_of(area(), nx, ny) != lg.cur) continue;
                bool two = D(nx + DX[k], ny + DY[k]) >= 0 && room_of(area(), nx + DX[k], ny + DY[k]) == lg.cur;
                if (best < 0 || two) best = k;
                if (two) break;
            }
            if (best >= 0) {
                mask = dir_button(best);
                if (tool_wait) { clear_t = 16; clear_dir = best; }
            }
            prev_mask = (uint32_t)mask;
            return mask;
        }
        int ccx = pcx() % LFL_TILE, ccy = pcy() % LFL_TILE;
        /* centred, with room to turn: a one-frame tap toward the facing
         * moves her a pixel or two, which must keep her on this tile */
        int lox = 5, hix = 11, loy = 5, hiy = 11;
        if (j.face >= 0 && p->face != j.face) {
            if (j.face == DIR_UP) loy = 8;
            if (j.face == DIR_DOWN) hiy = 8;
            if (j.face == DIR_LEFT) lox = 8;
            if (j.face == DIR_RIGHT) hix = 8;
        }
        bool centred = ccx >= lox && ccx <= hix && ccy >= loy && ccy <= hiy;
        if (j.act == 4) mask = dir_button(j.face);
        else if (!centred) mask = ccx < lox ? BTN_RIGHT : ccx > hix ? BTN_LEFT : ccy < loy ? BTN_DOWN : BTN_UP;
        else if (j.face >= 0 && p->face != j.face) mask = dir_button(j.face);
        else if (j.act == 1) {
            mask = press_a();
            if (mask && chop_gate >= 0 && p->hold[p->act] == TL_CHOPS) chop_t = 420;
        } else if (j.act == 2) mask = (a_hold++ % 24) < 20 ? BTN_A : 0;
        else if (j.act == 3) mask = (prev_mask & BTN_B) ? 0 : BTN_B;
        else {
            /* centre on the tile */
            int cx = pcx() % LFL_TILE, cy = pcy() % LFL_TILE;
            if (cx < 6) mask = BTN_RIGHT; else if (cx > 10) mask = BTN_LEFT;
            else if (cy < 6) mask = BTN_DOWN; else if (cy > 10) mask = BTN_UP;
        }
        prev_mask = (uint32_t)mask;
        return mask;
    }
    a_hold = 0;
    /* a creature close and the weapon in the other hand: swap to it */
    {
        int other = p->hold[1 - p->act];
        int cur_t = p->hold[p->act];
        bool threat = false;
        for (int i = 0; i < LFL_MAX_MOBS; i++)
            if (is_threat(&lg.mob[i]) && iabs(LFL_PX(lg.mob[i].x) + 6 - pcx()) + iabs(LFL_PX(lg.mob[i].y) + 6 - pcy()) < 70) threat = true;
        if (threat && !fetching && !(j.act && j.nneed > 0) && (other == TL_RIVET || other == TL_DISC) && cur_t != TL_RIVET && cur_t != TL_DISC && !small() &&
            lfl_talk_target() < 0) {
            b_tap = 2;
            prev_mask = BTN_B;
            return BTN_B;
        }
    }
    /* shoot what is in line on the way */
    int f = fight();
    if (f) { prev_mask = (uint32_t)f; return f; }
    if (j.wx < 0) { prev_mask = 0; return 0; }
    int d = first_step(j.wx, j.wy);
    if (d < 0) { prev_mask = 0; return 0; }
    /* the hole in a wall: small first (fire the hatcher at it) */
    int nx = my_wx() + DX[d], ny = my_wy() + DY[d];
    int nt = wtile(area(), nx, ny);
    if ((nt == T_HOLE || nt == T_SECRET) && !small()) {
        if (p->hold[p->act] != TL_HATCHER) { b_tap = 2; prev_mask = BTN_B; return BTN_B; }
        int cx = pcx() % LFL_TILE, cy = pcy() % LFL_TILE;
        bool centred = (d & 1) ? (cy >= 6 && cy <= 10) : (cx >= 6 && cx <= 10);
        if (!centred) mask = steer(d);
        else if (p->face != d) mask = dir_button(d);
        else mask = press_a();
        prev_mask = (uint32_t)mask;
        return mask;
    }
    mask = steer(d);
    /* not moving: wiggle */
    if (my_wx() == last_wx && my_wy() == last_wy) stuck_t++;
    else stuck_t = 0;
    last_wx = my_wx();
    last_wy = my_wy();
    if (stuck_t > 90 && (stuck_t / 20) & 1) mask = dir_button((d + 1 + (stuck_t / 40) % 2 * 2) & 3);
    prev_mask = (uint32_t)mask;
    return mask;
}
