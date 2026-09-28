/* MANDIBLES - the demo player. It only chooses buttons (the tests press
 * them for real through the "bot" query); everything it does is what a
 * player can do: switch the queen to soldiers, carry the first beads home,
 * shout Follow now and then, and kite. It plays the way the guides say
 * Combatants is won: raid early, stand off at an angle a locked red soldier
 * isn't spitting along, back away from workers while spitting, and never
 * stand next to a longlegs. */
#include "mandibles.h"

#define FP MND_FP
#define R_SPOT 30

static int16_t dist[MND_MAXH][MND_MAXW];
static int queue_buf[MND_MAXH * MND_MAXW];
static int macro[16], macro_n, macro_i;
static int bt, last_toggle, last_shout, gather_done;
static int cur_target = -1, stuck_t, last_px, last_py;

/* What the demo player knows about each field before it starts, the way a
 * player who has lost there once would: how many beads to carry home first,
 * and whether the longlegs can be walked over to the reds at all. */
static const struct { uint8_t beads, lure; } PLAN[] = {
    {6, 1}, {6, 1}, {6, 1}, {6, 1}, {6, 1}, {6, 1}, {6, 1}, {6, 1},
    {3, 0}, /* the root maze: the longlegs can't follow through the roots */
    {6, 1}, {6, 1}, {6, 1}, {6, 1}, {6, 1}, {6, 1}, {6, 1},
};
static int dbg_branch;
static int tgt_id = -1, tgt_hp, tgt_t, ban_id = -1, ban_t;
static bool banned(const MndWorld *w, int k) { return w->u[k].id == ban_id && bt - ban_t < 900; }
static int lured_ids[8], n_lured, wait_t, fetch_t, lure_id = -1, lure_t;
static bool lured(int id) {
    for (int i = 0; i < n_lured; i++)
        if (lured_ids[i] == id) return true;
    return false;
}
static void lure_done(int id) {
    if (!lured(id) && n_lured < 8) lured_ids[n_lured++] = id;
}
int mnd_bot_debug(int what) { return what ? dbg_branch : cur_target; }

void mnd_bot_reset(void) {
    macro_n = macro_i = 0;
    bt = 0;
    last_toggle = last_shout = -1000;
    gather_done = 0;
    cur_target = -1;
    stuck_t = 0;
    tgt_id = ban_id = -1;
    n_lured = 0;
    wait_t = 0;
    fetch_t = -10000;
    lure_id = -1;
}

static bool free_tile(const MndWorld *w, int tx, int ty) {
    return tx >= 0 && ty >= 0 && tx < w->w && ty < w->h && !mnd_solid(w, tx * MND_TILE + 4, ty * MND_TILE + 4);
}

static void bfs(const MndWorld *w, int sx, int sy) {
    for (int y = 0; y < w->h; y++)
        for (int x = 0; x < w->w; x++) dist[y][x] = -1;
    if (!free_tile(w, sx, sy)) {
        /* standing on the edge of something: start from a free neighbour */
        for (int d = 0; d < 8; d++)
            if (free_tile(w, sx + MND_DX[d], sy + MND_DY[d])) { sx += MND_DX[d]; sy += MND_DY[d]; break; }
    }
    int qh = 0, qt = 0;
    dist[sy][sx] = 0;
    queue_buf[qt++] = sy * MND_MAXW + sx;
    while (qh < qt) {
        int c = queue_buf[qh++], cx = c % MND_MAXW, cy = c / MND_MAXW;
        for (int d = 0; d < 8; d++) {
            int nx = cx + MND_DX[d], ny = cy + MND_DY[d];
            if (!free_tile(w, nx, ny) || dist[ny][nx] >= 0) continue;
            if ((d & 1) && (!free_tile(w, cx + MND_DX[d], cy) || !free_tile(w, cx, cy + MND_DY[d]))) continue;
            dist[ny][nx] = (int16_t)(dist[cy][cx] + 1);
            queue_buf[qt++] = ny * MND_MAXW + nx;
        }
    }
}

static int path_len(int px, int py) {
    int tx = px / MND_TILE, ty = py / MND_TILE;
    if (tx < 0 || ty < 0 || tx >= MND_MAXW || ty >= MND_MAXH) return -1;
    return dist[ty][tx];
}

/* the first tile to walk to on the way to (px,py) */
static void first_step(const MndWorld *w, int mx, int my, int px, int py, int *ox, int *oy) {
    int tx = px / MND_TILE, ty = py / MND_TILE, sx = mx / MND_TILE, sy = my / MND_TILE;
    *ox = px;
    *oy = py;
    if (tx == sx && ty == sy) return;
    if (dist[ty][tx] < 0) return;
    int guard = 0;
    while (guard++ < 4000) {
        /* walk back down the distances */
        int best = -1, bx = tx, by = ty;
        for (int d = 0; d < 8; d++) {
            int nx = tx + MND_DX[d], ny = ty + MND_DY[d];
            if (nx < 0 || ny < 0 || nx >= w->w || ny >= w->h || dist[ny][nx] < 0) continue;
            if ((d & 1) && (!free_tile(w, tx + MND_DX[d], ty) || !free_tile(w, tx, ty + MND_DY[d]))) continue;
            int want = dist[ty][tx] - 1;
            if (dist[ny][nx] != want) continue;
            if (best < 0 || dist[ny][nx] < best) { best = dist[ny][nx]; bx = nx; by = ny; }
        }
        if (best < 0) return;
        if (bx == sx && by == sy) {
            *ox = tx * MND_TILE + 4;
            *oy = ty * MND_TILE + 4;
            return;
        }
        tx = bx;
        ty = by;
    }
}

static int dir_mask(int dx, int dy) {
    int m = 0;
    if (dx > 0) m |= BTN_RIGHT;
    if (dx < 0) m |= BTN_LEFT;
    if (dy > 0) m |= BTN_DOWN;
    if (dy < 0) m |= BTN_UP;
    return m;
}

static const int DIR_BTN[8] = {BTN_RIGHT, BTN_RIGHT | BTN_DOWN, BTN_DOWN, BTN_DOWN | BTN_LEFT,
                               BTN_LEFT, BTN_LEFT | BTN_UP, BTN_UP, BTN_UP | BTN_RIGHT};

static int upx(const MndUnit *u) { return u->x / FP; }
static int upy(const MndUnit *u) { return u->y / FP; }

/* Is (px,py) in the spit lane of a locked red soldier? */
static bool in_lane(const MndWorld *w, int side, int px, int py, int skip) {
    for (int k = 0; k < w->n_units; k++) {
        const MndUnit *o = &w->u[k];
        if (o->kind != MK_SOLDIER || o->side == side || k == skip) continue;
        int dx = px - upx(o), dy = py - upy(o);
        if (iabs(dx) > 70 || iabs(dy) > 70) continue;
        int dir = o->lock >= 0 ? o->lock : -1;
        for (int alt = 0; alt < 2; alt++) {
            int d = alt ? mnd_octant(dx, dy) : dir;
            if (d < 0) continue;
            if (alt && dir >= 0) continue; /* a locked soldier only spits its way */
            if (alt && (dx * dx + dy * dy) > 50 * 50) continue;
            int ex = MND_DX[d], ey = MND_DY[d];
            int along = dx * ex + dy * ey;
            if (along <= 0) continue;
            int perp = iabs(dx * ey - dy * ex);
            if (d & 1) perp = perp * 7 / 10;
            if (perp < 8) return true;
        }
    }
    return false;
}

static int nearest_hostile_ant(const MndWorld *w, int side, int px, int py, int maxd, int kind_mask) {
    int best = -1, bestd = maxd * maxd;
    for (int k = 0; k < w->n_units; k++) {
        const MndUnit *o = &w->u[k];
        if (!o->kind || o->side == side || !((1 << o->kind) & kind_mask)) continue;
        int dx = upx(o) - px, dy = upy(o) - py, d = dx * dx + dy * dy;
        if (d < bestd) { bestd = d; best = k; }
    }
    return best;
}

#define ANTS ((1 << MK_WORKER) | (1 << MK_SOLDIER) | (1 << MK_PLAYER))

static bool spot_ok(const MndWorld *w, int side, int sx, int sy, int t, bool strict) {
    if (mnd_solid(w, sx - 3, sy - 3) || mnd_solid(w, sx + 3, sy - 3) || mnd_solid(w, sx - 3, sy + 3) || mnd_solid(w, sx + 3, sy + 3))
        return false;
    if (path_len(sx, sy) < 0) return false;
    const MndUnit *tu = &w->u[t];
    if (!mnd_los(w, sx, sy, upx(tu), upy(tu))) return false;
    if (!strict) return true;
    for (int k = 0; k < w->n_units; k++) {
        const MndUnit *o = &w->u[k];
        if (!o->kind || o->side == side || k == t) continue;
        int dx = upx(o) - sx, dy = upy(o) - sy, d = dx * dx + dy * dy;
        int keep = o->kind == MK_SPIDER ? 40 : o->kind == MK_QUEEN ? 0 : 22;
        if (d < keep * keep) return false;
    }
    if (in_lane(w, side, sx, sy, -1)) return false;
    return true;
}

static int macro_next(void) {
    if (macro_i < macro_n) return macro[macro_i++];
    macro_n = macro_i = 0;
    return -1;
}

static void push_macro(const int *m, int n) {
    for (int i = 0; i < n && i < 16; i++) macro[i] = m[i];
    macro_n = imin(n, 16);
    macro_i = 0;
}

/* Hold A, press a command's direction once per step along its arm, let go. */
static void menu_macro(const MndWorld *w, int side, int cmd) {
    static const int ARM_BTN[ARM_COUNT] = {BTN_UP, BTN_RIGHT, BTN_DOWN, BTN_LEFT};
    (void)w;
    (void)side;
    int m[16], n = 0;
    for (int a = 0; a < ARM_COUNT; a++)
        for (int s = 0; s < MND_ARM_SLOTS; s++) {
            if (MND_ARMS[a][s] != cmd) continue;
            m[n++] = MND_BTN_ORDER;
            for (int k = 0; k <= s; k++) {
                m[n++] = MND_BTN_ORDER | ARM_BTN[a];
                m[n++] = MND_BTN_ORDER;
            }
            m[n++] = 0;
            push_macro(m, n);
            return;
        }
}

/* Aim along one of the eight lines at a target, if it sits on one. */
static int aim_line(const MndWorld *w, int mx, int my, const MndUnit *t) {
    int tx = t->x / FP, ty = t->y / FP, hitr = t->kind == MK_SPIDER || t->kind == MK_QUEEN ? 7 : 4;
    int rx = tx - mx, ry = ty - my;
    for (int d = 0; d < 8; d++) {
        int ex = MND_DX[d], ey = MND_DY[d];
        int along = rx * ex + ry * ey, perp = iabs(rx * ey - ry * ex);
        if (d & 1) { along = along * 7 / 10; perp = perp * 7 / 10; }
        if (along > 0 && along < MND_SPIT_RANGE - 4 && perp <= hitr && mnd_los(w, mx, my, tx, ty)) return d;
    }
    return -1;
}

static int decide(const MndWorld *w, int side) {
    int p = w->player[side];
    if (p < 0) return 0;
    const MndUnit *me = &w->u[p];
    int mx = upx(me), my = upy(me);
    int q = -1, qd = 1 << 30;
    for (int k = 0; k < w->n_units; k++) {
        const MndUnit *o = &w->u[k];
        if (o->kind != MK_QUEEN || o->side != side) continue;
        int dx = upx(o) - mx, dy = upy(o) - my, d = dx * dx + dy * dy;
        if (d < qd) { qd = d; q = k; }
    }
    /* switch the queen to soldiers the moment we stand by her */
    if (q >= 0 && w->u[q].prod == PROD_WORKER && bt - last_toggle > 30) {
        menu_macro(w, side, CMD_SOLDIERS);
        last_toggle = bt;
        return macro_next();
    }
    /* the squad: soldiers already on Follow near us, and the rest */
    int followers = 0, far_f = 0, spare = -1, spare_l = 1 << 30, n_spare = 0;
    if ((bt & 3) == 0 || bt < 3) bfs(w, mx / MND_TILE, my / MND_TILE);
    for (int k = 0; k < w->n_units; k++) {
        const MndUnit *o = &w->u[k];
        if (o->kind != MK_SOLDIER || o->side != side) continue;
        int dx = upx(o) - mx, dy = upy(o) - my, d = iabs(dx) + iabs(dy);
        if (o->order == ORD_FOLLOW) {
            followers++;
            if (mnd_los(w, mx, my, upx(o), upy(o))) far_f = imax(far_f, d);
        } else if (!o->carry) {
            int l = path_len(upx(o), upy(o));
            if (l >= 0 && l < 24) n_spare++;
            if (l >= 0 && l < spare_l) { spare_l = l; spare = k; }
        }
    }
    int red_close = nearest_hostile_ant(w, side, mx, my, 56, ANTS);
    /* keep the squad moving: a fresh order skips their pause after each step */
    if (followers && bt - last_shout > (far_f > 24 ? 18 : 34) && w->menu[side].last == CMD_SOLDIER_FOLLOW) {
        static const int TAP[] = {MND_BTN_ORDER, 0};
        push_macro(TAP, 2);
        last_shout = bt;
        return macro_next();
    }
    /* wait for stragglers when nothing is about to bite */
    if (followers && far_f > 36 && far_f < 90 && red_close < 0 && !me->carry && wait_t < 120) {
        dbg_branch = 8;
        wait_t++;
        return 0;
    }
    if (wait_t > 0 && ++wait_t > 360) wait_t = 0;
    /* go and fetch idle soldiers when there's a handful */
    if (spare >= 0 && followers < 4 && n_spare >= 3 && red_close < 0 && !me->carry && spare_l < 24 && bt - fetch_t > 1200) {
        int dx = upx(&w->u[spare]) - mx, dy = upy(&w->u[spare]) - my;
        if (dx * dx + dy * dy <= 28 * 28) fetch_t = bt;
        else {
            dbg_branch = 9;
            int ox, oy;
            first_step(w, mx, my, upx(&w->u[spare]), upy(&w->u[spare]), &ox, &oy);
            return dir_mask(ox - mx, oy - my);
        }
    }
    /* now and then, shout Follow (a tap repeats the last order) */
    if (bt - last_shout > 30) {
        int near = 0;
        for (int k = 0; k < w->n_units; k++) {
            const MndUnit *o = &w->u[k];
            if (o->kind != MK_SOLDIER || o->side != side) continue;
            int dx = upx(o) - mx, dy = upy(o) - my;
            if (dx * dx + dy * dy <= (MND_SHOUT_R - 4) * (MND_SHOUT_R - 4) && (o->order != ORD_FOLLOW || o->order_age > 90)) near++;
        }
        if (near) {
            menu_macro(w, side, CMD_SOLDIER_FOLLOW);
            last_shout = bt;
            return macro_next();
        }
    }
    /* danger first: something about to touch us */
    int close = nearest_hostile_ant(w, side, mx, my, 14, ANTS);
    int spider = nearest_hostile_ant(w, side, mx, my, 34, 1 << MK_SPIDER);
    int threat = close >= 0 ? close : spider;
    if (threat >= 0) {
        const MndUnit *o = &w->u[threat];
        int bestd = -1, best = -1;
        for (int d = 0; d < 8; d++) {
            int nx = mx + MND_DX[d] * 6, ny = my + MND_DY[d] * 6;
            if (mnd_solid(w, nx - 3, ny - 3) || mnd_solid(w, nx + 3, ny - 3) || mnd_solid(w, nx - 3, ny + 3) || mnd_solid(w, nx + 3, ny + 3)) continue;
            int dx = nx - upx(o), dy = ny - upy(o), dd = dx * dx + dy * dy;
            if (in_lane(w, side, nx, ny, -1)) dd -= 400;
            if (dd > bestd) { bestd = dd; best = d; }
        }
        if (best >= 0) {
            dbg_branch = 1;
            int aim = aim_line(w, mx, my, o);
            if (aim >= 0 && me->cool <= 0 && o->kind != MK_SPIDER) return DIR_BTN[aim] | MND_BTN_SPIT;
            return DIR_BTN[best];
        }
    }

    /* gather the first beads home, and top the queen up when she is empty */
    int red_near = nearest_hostile_ant(w, side, mx, my, 80, ANTS);
    int first_beads = PLAN[w->map % ARRAY_LEN(PLAN)].beads;
    bool want_beads = q >= 0 && (w->delivered[side] < first_beads || (w->u[q].store < 2 && w->delivered[side] < 60));
    if (w->delivered[side] >= first_beads) gather_done = 1;
    if (me->carry && red_near < 0 && q >= 0) {
        dbg_branch = 2;
        int ox, oy;
        first_step(w, mx, my, upx(&w->u[q]), upy(&w->u[q]), &ox, &oy);
        return dir_mask(ox - mx, oy - my);
    }
    if (want_beads && red_near < 0 && !me->carry) {
        int best = -1, bestl = !gather_done ? 140 : w->u[q].store == 0 ? 50 : 14;
        for (int b = 0; b < w->n_beads; b++) {
            if (!w->bead[b].on) continue;
            int l = path_len(w->bead[b].x, w->bead[b].y);
            if (l < 0 || l >= bestl) continue;
            /* not into the reds' own larder */
            if (nearest_hostile_ant(w, side, w->bead[b].x, w->bead[b].y, 60, ANTS | (1 << MK_QUEEN) | (1 << MK_SPIDER)) >= 0) continue;
            bestl = l;
            best = b;
        }
        if (best >= 0) {
            dbg_branch = 3;
            int ox, oy;
            first_step(w, mx, my, w->bead[best].x, w->bead[best].y, &ox, &oy);
            return dir_mask(ox - mx, oy - my);
        }
    }

    /* lead a longlegs into the reds while they outnumber us */
    if (!me->carry && PLAN[w->map % ARRAY_LEN(PLAN)].lure && mnd_count(w, side ^ 1, MK_NONE) >= 6) {
        int sp = -1, spd = 1 << 30;
        for (int k = 0; k < w->n_units; k++) {
            const MndUnit *o = &w->u[k];
            if (o->kind != MK_SPIDER || lured(o->id)) continue;
            int l = path_len(upx(o), upy(o));
            if (l >= 0 && l < spd) { spd = l; sp = k; }
        }
        if (sp >= 0) {
            const MndUnit *s = &w->u[sp];
            int sx0 = upx(s), sy0 = upy(s);
            /* where the reds live: their nearest queen, else their nearest ant */
            int rq = -1, rqd = 1 << 30;
            for (int k = 0; k < w->n_units; k++) {
                const MndUnit *o = &w->u[k];
                if (!o->kind || o->side == side || o->kind == MK_SPIDER) continue;
                int dx = upx(o) - sx0, dy = upy(o) - sy0, d = dx * dx + dy * dy + (o->kind == MK_QUEEN ? 0 : 60 * 60);
                if (d < rqd) { rqd = d; rq = k; }
            }
            int red_by_spider = nearest_hostile_ant(w, side, sx0, sy0, 64, ANTS | (1 << MK_QUEEN));
            if (lure_id != s->id) { lure_id = s->id; lure_t = 0; }
            if (red_by_spider >= 0 || rq < 0 || ++lure_t > 1500) {
                lure_done(s->id); /* it has company now, or it won't come */
            } else {
                int dx = sx0 - mx, dy = sy0 - my, d = dx * dx + dy * dy;
                int ox, oy;
                if (d > 58 * 58 || !mnd_los(w, mx, my, sx0, sy0)) {
                    dbg_branch = 10;
                    first_step(w, mx, my, sx0, sy0, &ox, &oy);
                    return dir_mask(ox - mx, oy - my);
                }
                if (d > 48 * 48 && s->mem_t <= 0) return 0; /* let it notice us */
                dbg_branch = 11;
                first_step(w, mx, my, upx(&w->u[rq]), upy(&w->u[rq]), &ox, &oy);
                if (d > 62 * 62) return 0;
                return dir_mask(ox - mx, oy - my);
            }
        }
    }

    /* pick a target: a longlegs that has come for us, a red ant close by,
     * else the nearest red by the road there (queens count a little further) */
    int t = nearest_hostile_ant(w, side, mx, my, 64, 1 << MK_SPIDER);
    if (t >= 0 && !mnd_los(w, mx, my, upx(&w->u[t]), upy(&w->u[t]))) t = -1;
    if (t < 0) t = nearest_hostile_ant(w, side, mx, my, 70, ANTS);
    dbg_branch = 5;
    if (t >= 0 && banned(w, t)) t = -1;
    if (t < 0) {
        int bestl = 1 << 30;
        for (int k = 0; k < w->n_units; k++) {
            const MndUnit *o = &w->u[k];
            if (!o->kind || o->side == side || o->kind == MK_SPIDER || banned(w, k)) continue;
            int l = path_len(upx(o), upy(o));
            if (l < 0) {
                /* standing against rock: try the tiles around it */
                for (int d = 0; d < 8 && l < 0; d++) l = path_len(upx(o) + MND_DX[d] * 8, upy(o) + MND_DY[d] * 8);
                if (l < 0) continue;
            }
            if (o->kind == MK_QUEEN) l += 30;
            if (l < bestl) { bestl = l; t = k; }
        }
    }
    if (t < 0 && ban_id >= 0) {
        /* everything left is the one we gave up on: try it again */
        for (int k = 0; k < w->n_units; k++)
            if (w->u[k].kind && w->u[k].id == ban_id) t = k;
        ban_id = -1;
    }
    if (t < 0) {
        /* only longlegs left in reach: hunt one (it is worth a gift) */
        t = nearest_hostile_ant(w, side, mx, my, 2000, 1 << MK_SPIDER);
        if (t < 0) return 0;
    }
    /* a stalemate (it's stuck behind something, so are we): try another */
    if (t != cur_target || w->u[t].id != tgt_id) {
        tgt_id = w->u[t].id;
        tgt_hp = w->u[t].hp;
        tgt_t = 0;
    } else if (w->u[t].hp < tgt_hp) {
        tgt_hp = w->u[t].hp;
        tgt_t = 0;
    } else if (tgt_t > 360) {
        ban_id = tgt_id;
        ban_t = bt;
        tgt_t = 0;
    }
    cur_target = t;
    const MndUnit *tu = &w->u[t];
    int tx = upx(tu), ty = upy(tu);
    int radius = tu->kind == MK_SPIDER ? 44 : tu->kind == MK_QUEEN ? 24 : R_SPOT;
    /* the best spot on one of the eight spit lines through the target:
     * safe ones first, then nearer or further, then any we can see it from */
    int best = -1, bestl = 1 << 30, sx = 0, sy = 0;
    static const int RMUL[4] = {10, 7, 14, 10};
    for (int pass = 0; pass < 4 && best < 0; pass++) {
        int rr = radius * RMUL[pass] / 10;
        for (int k = 0; k < 8; k++) {
            int r = k & 1 ? rr * 7 / 10 : rr;
            int cx = tx + MND_DX[k] * r, cy = ty + MND_DY[k] * r;
            if (!spot_ok(w, side, cx, cy, t, pass < 3)) continue;
            int l = path_len(cx, cy);
            int ddx = cx - mx, ddy = cy - my;
            l = l * 4 + (iabs(ddx) + iabs(ddy)) / 2;
            if (l < bestl) { bestl = l; best = k; sx = cx; sy = cy; }
        }
    }
    if (best < 0) {
        /* nowhere safe to stand: walk toward it anyway, carefully */
        dbg_branch = 6;
        int ox, oy;
        first_step(w, mx, my, tx, ty, &ox, &oy);
        if (iabs(tx - mx) + iabs(ty - my) < 60) {
            /* back off to the queen and wait for a better angle */
            if (q >= 0) first_step(w, mx, my, upx(&w->u[q]), upy(&w->u[q]), &ox, &oy);
        }
        return dir_mask(ox - mx, oy - my);
    }
    int dx = sx - mx, dy = sy - my;
    /* can we hit it from here? turn to it and spit in the same frame (the
     * facing follows the d-pad, so kiting is turn, spit, turn back) */
    if (iabs(dx) <= 2 && iabs(dy) <= 2) tgt_t++;
    int aim = aim_line(w, mx, my, tu);
    if (aim >= 0 && me->cool <= 0) return DIR_BTN[aim] | MND_BTN_SPIT;
    if (iabs(dx) > 2 || iabs(dy) > 2) {
        int ox, oy;
        first_step(w, mx, my, sx, sy, &ox, &oy);
        int move = dir_mask(ox - mx, oy - my);
        return move ? move : dir_mask(dx, dy);
    }
    return 0;
}

int mnd_bot_buttons(const MndWorld *w, int side) {
    bt++;
    int m = macro_next();
    if (m >= 0) return m;
    m = decide(w, side);
    /* pushing against something and not moving: jiggle free */
    int p = w->player[side];
    if (p >= 0 && (m & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)) && !(m & MND_BTN_ORDER)) {
        int x = w->u[p].x, y = w->u[p].y;
        if (x == last_px && y == last_py) stuck_t++;
        else stuck_t = 0;
        last_px = x;
        last_py = y;
        if (stuck_t > 16) {
            dbg_branch = 7;
            stuck_t = 0;
            int jig[12];
            int d = (int)((unsigned)bt * 2654435761u >> 29);
            for (int i = 0; i < 12; i++) jig[i] = DIR_BTN[d];
            push_macro(jig, 12);
            return macro_next();
        }
    } else {
        stuck_t = 0;
    }
    return m;
}
