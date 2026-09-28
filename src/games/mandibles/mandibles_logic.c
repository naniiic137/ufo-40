/* MANDIBLES - the rules: ants, orders, spit, melee, beads, queens, longlegs.
 * Pure simulation on a MndWorld; no drawing, no input devices. Everything is
 * in whole sub-pixels (1/16 px) so a seed plays out the same everywhere.
 * See docs/games/46-mandibles.md for where each number comes from. */
#include "mandibles.h"

MndWorld mnd_w;

const int8_t MND_DX[8] = {1, 1, 0, -1, -1, -1, 0, 1};
const int8_t MND_DY[8] = {0, 1, 1, 1, 0, -1, -1, -1};

#define FP MND_FP
#define STEP_FRAMES 16     /* an AI ant's step: one tile */
#define PAUSE_FRAMES 18    /* ...then it stops to think */
#define ANT_SPEED 8        /* sub-pixels a frame (half a pixel) */
#define ANT_SPEED_D 6
#define SPIDER_SPEED 3
#define SPIDER_SPEED_D 2
#define SIGHT 64           /* px an ant sees */
#define FIRE_RANGE 48      /* px at which a soldier stands and spits */
#define SPIT_SPEED 32      /* sub-pixels a frame */
#define SPIT_SPEED_D 23
#define SPIT_LIFE (MND_SPIT_RANGE * FP / SPIT_SPEED)
#define PLAYER_COOL 18
#define BLUE_COOL 24
#define RED_COOL 26
#define GESTATION 90
#define QUEEN_NIBBLE 60    /* frames between a worker's bites on an enemy queen */
#define SPIDER_BITE 45
#define SPIDER_DMG 3
#define FOLLOW_FRESH 120   /* a follow order goes stale after two seconds */

static const int HP_BLUE[6] = {0, 3, 3, 3, 20, 45};
static const int HP_RED[6] = {0, 3, 6, 6, 40, 45};

static const char *red_prod = "W";

/* ------------------------------------------------------------------ */
/* map queries                                                          */

static bool tile_solid(const MndWorld *w, int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= w->w || ty >= w->h) return true;
    int t = w->tile[ty][tx];
    return t == TL_ROCK || t == TL_WATER || t == TL_ROOT;
}

bool mnd_solid(const MndWorld *w, int px, int py) {
    if (px < 0 || py < 0) return true;
    return tile_solid(w, px / MND_TILE, py / MND_TILE);
}

bool mnd_opaque(const MndWorld *w, int px, int py) {
    if (px < 0 || py < 0 || px >= w->w * MND_TILE || py >= w->h * MND_TILE) return true;
    int t = w->tile[py / MND_TILE][px / MND_TILE];
    return t == TL_ROCK || t == TL_ROOT;
}

bool mnd_los(const MndWorld *w, int x0, int y0, int x1, int y1) {
    int dx = x1 - x0, dy = y1 - y0;
    int n = imax(iabs(dx), iabs(dy)) / 4;
    for (int i = 1; i < n; i++)
        if (mnd_opaque(w, x0 + dx * i / n, y0 + dy * i / n)) return false;
    return true;
}

int mnd_octant(int dx, int dy) {
    int ax = iabs(dx), ay = iabs(dy);
    if (ax == 0 && ay == 0) return -1;
    if (ay * 1000 <= ax * 414) return dx > 0 ? 0 : 4;
    if (ax * 1000 <= ay * 414) return dy > 0 ? 2 : 6;
    if (dx > 0) return dy > 0 ? 1 : 7;
    return dy > 0 ? 3 : 5;
}

static int half_of(int kind) { return kind == MK_SPIDER ? 5 : kind == MK_QUEEN ? 6 : 3; }
static int radius_of(int kind) { return kind == MK_SPIDER || kind == MK_QUEEN ? 9 : 5; }
static bool is_ant(int kind) { return kind == MK_PLAYER || kind == MK_WORKER || kind == MK_SOLDIER; }

static bool box_free(const MndWorld *w, int32_t x, int32_t y, int half) {
    int px = x / FP, py = y / FP;
    return !mnd_solid(w, px - half, py - half) && !mnd_solid(w, px + half, py - half) &&
           !mnd_solid(w, px - half, py + half) && !mnd_solid(w, px + half, py + half);
}

/* Move with sliding: each axis on its own. Returns false if stuck both ways. */
static bool move_unit(MndWorld *w, MndUnit *u, int dx, int dy) {
    int half = half_of(u->kind);
    bool moved = false;
    if (dx && box_free(w, u->x + dx, u->y, half)) { u->x += dx; moved = true; }
    if (dy && box_free(w, u->x, u->y + dy, half)) { u->y += dy; moved = true; }
    return moved;
}

static int dist2(const MndUnit *a, const MndUnit *b) {
    int dx = (a->x - b->x) / FP, dy = (a->y - b->y) / FP;
    return dx * dx + dy * dy;
}

/* ------------------------------------------------------------------ */
/* units                                                                */

int mnd_add_unit(MndWorld *w, int kind, int side, int px, int py) {
    int slot = -1;
    for (int i = 0; i < MND_MAX_UNITS; i++)
        if (!w->u[i].kind) { slot = i; break; }
    if (slot < 0) return -1;
    if (slot >= w->n_units) w->n_units = slot + 1;
    MndUnit *u = &w->u[slot];
    memset(u, 0, sizeof *u);
    u->kind = (uint8_t)kind;
    u->side = (uint8_t)side;
    const int *hp = side == MND_RED && !w->versus ? HP_RED : HP_BLUE;
    u->hp = u->maxhp = (int16_t)hp[kind];
    u->x = px * FP;
    u->y = py * FP;
    u->face = side == MND_RED ? 4 : 0;
    u->lock = -1;
    u->target = -1;
    u->goal_bead = -1;
    u->order = ORD_INSTINCT;
    u->pause = (int16_t)rng_range(&w->rng, 0, PAUSE_FRAMES);
    u->id = w->next_id++;
    return slot;
}

int mnd_count(const MndWorld *w, int side, int kind) {
    int n = 0;
    for (int i = 0; i < w->n_units; i++) {
        const MndUnit *u = &w->u[i];
        if (!u->kind || u->side != side) continue;
        if (kind == MK_NONE ? u->kind != MK_SPIDER : u->kind == kind) n++;
    }
    return n;
}

int mnd_queen_of(const MndWorld *w, int side) {
    for (int i = 0; i < w->n_units; i++)
        if (w->u[i].kind == MK_QUEEN && w->u[i].side == side) return i;
    return -1;
}

int mnd_beads_left(const MndWorld *w) {
    int n = 0;
    for (int i = 0; i < w->n_beads; i++) n += w->bead[i].on;
    return n;
}

static void drop_bead(MndWorld *w, int px, int py) {
    int tx = px / MND_TILE, ty = py / MND_TILE;
    if (tile_solid(w, tx, ty)) return;
    for (int i = 0; i < MND_MAX_BEADS; i++)
        if (!w->bead[i].on) {
            w->bead[i] = (MndBead){1, (int16_t)(tx * MND_TILE + 4), (int16_t)(ty * MND_TILE + 4)};
            if (i >= w->n_beads) w->n_beads = i + 1;
            return;
        }
}

static void kill_unit(MndWorld *w, int i, int by_side) {
    MndUnit *u = &w->u[i];
    if (u->carry) drop_bead(w, u->x / FP, u->y / FP);
    if (u->kind == MK_SPIDER) {
        w->spiders_slain++;
        w->ev.spider_slain++;
        if (by_side == MND_BLUE) { w->blue_spider_kills++; w->ev.blue_spider_kill++; }
    } else {
        if (by_side <= MND_RED && by_side != u->side) w->kills[by_side]++;
        if (u->side <= MND_RED) w->losses[u->side]++;
    }
    if (u->kind == MK_PLAYER) {
        w->player[u->side] = -1;
        w->dead_t[u->side] = 0;
    }
    w->ev.deaths++;
    u->kind = MK_NONE;
    /* anyone aiming at it lets go */
    for (int k = 0; k < w->n_units; k++)
        if (w->u[k].kind && w->u[k].target == i) { w->u[k].target = -1; w->u[k].lock = -1; }
}

static void damage(MndWorld *w, int i, int amount, int by_side) {
    MndUnit *u = &w->u[i];
    u->hp = (int16_t)(u->hp - amount);
    u->hurt = 10;
    if (u->hp <= 0) kill_unit(w, i, by_side);
}

static bool hostile(const MndUnit *a, const MndUnit *b) { return a->side != b->side; }

static bool red_ai(const MndWorld *w, const MndUnit *u) { return u->side == MND_RED && !w->versus; }

/* The nearest thing an ant can see and wants to fight: enemy ants first,
 * queens and longlegs a little further down the list. */
static int nearest_foe(const MndWorld *w, const MndUnit *u, int range) {
    int best = -1, bestd = range * range;
    int px = u->x / FP, py = u->y / FP;
    for (int k = 0; k < w->n_units; k++) {
        const MndUnit *o = &w->u[k];
        if (!o->kind || o == u || !hostile(u, o)) continue;
        int dx = o->x / FP - px, dy = o->y / FP - py;
        if (iabs(dx) > range || iabs(dy) > range) continue;
        int d = dx * dx + dy * dy;
        if (o->kind == MK_QUEEN) d += 24 * 24;
        if (d >= bestd) continue;
        if (!mnd_los(w, px, py, o->x / FP, o->y / FP)) continue;
        best = k;
        bestd = d;
    }
    return best;
}

static int nearest_queen(const MndWorld *w, int side, int px, int py) {
    int best = -1, bestd = 1 << 30;
    for (int k = 0; k < w->n_units; k++) {
        const MndUnit *o = &w->u[k];
        if (o->kind != MK_QUEEN || o->side != side) continue;
        int dx = o->x / FP - px, dy = o->y / FP - py, d = dx * dx + dy * dy;
        if (d < bestd) { bestd = d; best = k; }
    }
    return best;
}

static int visible_bead(const MndWorld *w, int px, int py) {
    int best = -1, bestd = SIGHT * SIGHT;
    for (int b = 0; b < w->n_beads; b++) {
        if (!w->bead[b].on) continue;
        int dx = w->bead[b].x - px, dy = w->bead[b].y - py, d = dx * dx + dy * dy;
        if (d >= bestd || !mnd_los(w, px, py, w->bead[b].x, w->bead[b].y)) continue;
        best = b;
        bestd = d;
    }
    return best;
}

/* Red ants know where every bead is and beeline for one that fewer than two
 * others are already after. */
static uint8_t claims[MND_MAX_BEADS];
static int red_bead(const MndWorld *w, const MndUnit *u) {
    int best = -1, bestd = 1 << 30, px = u->x / FP, py = u->y / FP;
    for (int b = 0; b < w->n_beads; b++) {
        if (!w->bead[b].on || (claims[b] >= 2 && u->goal_bead != b)) continue;
        int dx = w->bead[b].x - px, dy = w->bead[b].y - py, d = dx * dx + dy * dy;
        if (d < bestd) { bestd = d; best = b; }
    }
    return best;
}

static void spit(MndWorld *w, int side, int dir, int32_t x, int32_t y) {
    for (int i = 0; i < MND_MAX_SHOTS; i++)
        if (!w->shot[i].on) {
            w->shot[i] = (MndShot){1, (uint8_t)side, (int8_t)dir, SPIT_LIFE, x, y};
            w->ev.spits++;
            return;
        }
}

/* ------------------------------------------------------------------ */
/* orders                                                               */

void mnd_shout(MndWorld *w, int side, int order, bool soldiers_only) {
    int p = w->player[side];
    if (p < 0) return;
    const MndUnit *pl = &w->u[p];
    for (int i = 0; i < w->n_units; i++) {
        MndUnit *u = &w->u[i];
        if ((u->kind != MK_WORKER && u->kind != MK_SOLDIER) || u->side != side) continue;
        if (soldiers_only && u->kind != MK_SOLDIER) continue;
        if (dist2(u, pl) > MND_SHOUT_R * MND_SHOUT_R) continue;
        u->order = (uint8_t)order;
        u->order_age = 0;
        u->lost = 0;
        u->lock = -1;
        u->ack = 30;
        u->pause = 0; /* a fresh order gets it moving at once */
        if (order == ORD_HOLD) u->step = 0;
        if (order == ORD_FOLLOW) {
            u->ox = (int16_t)rng_range(&w->rng, -4, 4);
            u->oy = (int16_t)rng_range(&w->rng, -4, 4);
        }
    }
}

static int queen_in_earshot(const MndWorld *w, int side) {
    int p = w->player[side];
    if (p < 0) return -1;
    int q = nearest_queen(w, side, w->u[p].x / FP, w->u[p].y / FP);
    if (q < 0 || dist2(&w->u[q], &w->u[p]) > MND_SHOUT_R * MND_SHOUT_R) return -1;
    return q;
}

bool mnd_toggle_queen(MndWorld *w, int side) {
    int q = queen_in_earshot(w, side);
    if (q < 0) return false;
    w->u[q].prod ^= 1;
    return true;
}

bool mnd_menu_line_ok(const MndWorld *w, int side, int line) {
    if (line < 0 || line >= MI_COUNT) return false;
    return line != MI_QUEEN || queen_in_earshot(w, side) >= 0;
}

/* ------------------------------------------------------------------ */
/* ant brains                                                           */

static void start_step(MndWorld *w, MndUnit *u, int gx, int gy) {
    int dir = mnd_octant(gx - u->x / FP, gy - u->y / FP);
    if (dir < 0) { u->pause = PAUSE_FRAMES; return; }
    int miss = red_ai(w, u) ? w->red_mistake : (u->order == ORD_FOLLOW && u->order_age > FOLLOW_FRESH ? 50 : 25);
    if (rng_chance(&w->rng, miss)) dir = rng_range(&w->rng, 0, 7);
    u->step_dir = (int8_t)dir;
    u->face = (int8_t)dir;
    u->step = STEP_FRAMES;
}

static void wander(MndWorld *w, MndUnit *u) {
    int dir = rng_range(&w->rng, 0, 7);
    u->step_dir = (int8_t)dir;
    u->face = (int8_t)dir;
    u->step = STEP_FRAMES;
}

static void decide(MndWorld *w, int i) {
    MndUnit *u = &w->u[i];
    int px = u->x / FP, py = u->y / FP;
    u->target = u->kind == MK_SOLDIER && u->lock >= 0 ? u->target : -1;
    if (red_ai(w, u)) {
        if (u->kind == MK_SOLDIER && u->lock >= 0) { u->pause = 6; return; } /* rooted while it spits */
        if (u->carry) {
            int q = nearest_queen(w, u->side, px, py);
            if (q >= 0) { start_step(w, u, w->u[q].x / FP, w->u[q].y / FP); return; }
            wander(w, u);
            return;
        }
        int f = nearest_foe(w, u, SIGHT - 8);
        if (f >= 0) {
            const MndUnit *o = &w->u[f];
            if (u->kind == MK_SOLDIER && dist2(u, o) <= FIRE_RANGE * FIRE_RANGE) { u->pause = 6; return; }
            u->target = (int16_t)f;
            start_step(w, u, o->x / FP, o->y / FP); /* workers always come at you */
            return;
        }
        int b = red_bead(w, u);
        u->goal_bead = (int16_t)b;
        if (b >= 0) { start_step(w, u, w->bead[b].x, w->bead[b].y); return; }
        /* the food is gone: straight for the blue queen */
        int q = nearest_queen(w, MND_BLUE, px, py);
        if (q >= 0) { start_step(w, u, w->u[q].x / FP, w->u[q].y / FP); return; }
        int p = w->player[MND_BLUE];
        if (p >= 0) { start_step(w, u, w->u[p].x / FP, w->u[p].y / FP); return; }
        wander(w, u);
        return;
    }
    switch (u->order) {
    case ORD_HOLD:
        u->pause = 6;
        return;
    case ORD_FOLLOW: {
        int p = w->player[u->side];
        if (p < 0) { u->order = ORD_INSTINCT; break; }
        int gx = w->u[p].x / FP + u->ox, gy = w->u[p].y / FP + u->oy;
        if (iabs(gx - px) <= 4 && iabs(gy - py) <= 4) { u->pause = 4; return; }
        start_step(w, u, gx, gy);
        return;
    }
    default: break;
    }
    /* instinct */
    if (u->carry) {
        int q = nearest_queen(w, u->side, px, py);
        if (q >= 0) { start_step(w, u, w->u[q].x / FP, w->u[q].y / FP); return; }
    }
    int f = u->carry ? -1 : nearest_foe(w, u, SIGHT);
    if (f >= 0) {
        const MndUnit *o = &w->u[f];
        if (u->kind == MK_SOLDIER && dist2(u, o) <= FIRE_RANGE * FIRE_RANGE) { u->pause = 6; return; }
        u->target = (int16_t)f;
        start_step(w, u, o->x / FP, o->y / FP);
        return;
    }
    if (!u->carry) {
        int b = visible_bead(w, px, py);
        if (b >= 0) { start_step(w, u, w->bead[b].x, w->bead[b].y); return; }
    }
    wander(w, u);
}

static void soldier_fire(MndWorld *w, int i) {
    MndUnit *u = &w->u[i];
    if (u->cool > 0) return;
    if (red_ai(w, u)) {
        if (u->lock >= 0) {
            /* locked: keeps spitting the same way until its target dies or leaves */
            int t = u->target;
            if (t < 0 || !w->u[t].kind || dist2(u, &w->u[t]) > (MND_SPIT_RANGE + 8) * (MND_SPIT_RANGE + 8)) {
                u->lock = -1;
                u->target = -1;
                return;
            }
            spit(w, u->side, u->lock, u->x, u->y);
            u->face = u->lock;
            u->cool = RED_COOL;
            return;
        }
        if (u->carry || (w->frame + i) % 6) return;
        int f = nearest_foe(w, u, FIRE_RANGE);
        if (f < 0) return;
        int dir = mnd_octant(w->u[f].x / FP - u->x / FP, w->u[f].y / FP - u->y / FP);
        if (dir < 0) dir = u->face;
        u->lock = (int8_t)dir;
        u->target = (int16_t)f;
        u->step = 0;
        spit(w, u->side, dir, u->x, u->y);
        u->face = (int8_t)dir;
        u->cool = RED_COOL;
        return;
    }
    if (u->order == ORD_FOLLOW || u->carry || (w->frame + i) % 6) return; /* followers spit with their leader */
    int f = nearest_foe(w, u, FIRE_RANGE);
    if (f < 0) return;
    int dir = mnd_octant(w->u[f].x / FP - u->x / FP, w->u[f].y / FP - u->y / FP);
    if (dir < 0) dir = u->face;
    u->face = (int8_t)dir;
    spit(w, u->side, dir, u->x, u->y);
    u->cool = BLUE_COOL;
}

static void pickup_and_deliver(MndWorld *w, int i) {
    MndUnit *u = &w->u[i];
    int px = u->x / FP, py = u->y / FP;
    if (!u->carry) {
        for (int b = 0; b < w->n_beads; b++) {
            MndBead *bd = &w->bead[b];
            if (!bd->on || iabs(bd->x - px) > 4 || iabs(bd->y - py) > 4) continue;
            bd->on = 0;
            u->carry = 1;
            w->ev.pickups++;
            if (u->kind != MK_PLAYER && (red_ai(w, u) || u->order == ORD_INSTINCT)) {
                /* it turns for home at once, fight or no fight */
                u->step = 0;
                u->pause = 0;
                u->lock = -1;
                u->target = -1;
            }
            break;
        }
    } else {
        int q = nearest_queen(w, u->side, px, py);
        if (q >= 0 && dist2(u, &w->u[q]) <= 12 * 12) {
            u->carry = 0;
            w->u[q].store++;
            w->delivered[u->side]++;
            w->ev.deliveries++;
        }
    }
}

static void ant_update(MndWorld *w, int i) {
    MndUnit *u = &w->u[i];
    if (u->order_age < 30000) u->order_age++;
    if (u->order == ORD_FOLLOW && !red_ai(w, u)) {
        /* no object permanence: out of sight for half a second, and it forgets */
        int p = w->player[u->side];
        if ((w->frame + i) % 4 == 0) { /* looked for every fourth frame */
            bool sees = p >= 0 && dist2(u, &w->u[p]) <= 96 * 96 &&
                        mnd_los(w, u->x / FP, u->y / FP, w->u[p].x / FP, w->u[p].y / FP);
            u->lost = sees ? 0 : (int16_t)(u->lost + 4);
        }
        if (u->lost > 30) u->order = ORD_INSTINCT;
    }
    if (u->step > 0) {
        int d = u->step_dir;
        int sp = d & 1 ? ANT_SPEED_D : ANT_SPEED;
        bool moved = move_unit(w, u, MND_DX[d] * sp, MND_DY[d] * sp);
        u->anim++;
        if (--u->step == 0 || !moved) {
            u->step = 0;
            u->pause = PAUSE_FRAMES;
        }
    } else if (u->pause > 0) {
        u->pause--;
    } else {
        decide(w, i);
    }
    pickup_and_deliver(w, i);
    if (u->kind == MK_SOLDIER) soldier_fire(w, i);
}

static void queen_update(MndWorld *w, int i) {
    MndUnit *u = &w->u[i];
    u->anim++;
    if (u->gest > 0) {
        if (--u->gest == 0) {
            int start = rng_range(&w->rng, 0, 7);
            for (int k = 0; k < 8; k++) {
                int d = (start + k) % 8;
                int px = u->x / FP + MND_DX[d] * 12, py = u->y / FP + MND_DY[d] * 12;
                if (!box_free(w, px * FP, py * FP, 3)) continue;
                int n = mnd_add_unit(w, u->spawn_kind, u->side, px, py);
                if (n >= 0) {
                    w->u[n].face = (int8_t)d;
                    w->spawned[u->side]++;
                    w->ev.spawns++;
                }
                break;
            }
        }
        return;
    }
    /* a queen keeps a bead back while her player waits to hatch again */
    bool has_player = u->side == MND_BLUE || w->versus;
    if (has_player && w->player[u->side] < 0) return;
    int kind;
    if (red_ai(w, u)) {
        int len = (int)strlen(red_prod);
        kind = red_prod[u->cycle % len] == 'S' ? MK_SOLDIER : MK_WORKER;
    } else {
        kind = u->prod == PROD_SOLDIER ? MK_SOLDIER : MK_WORKER;
    }
    int cost = kind == MK_SOLDIER ? 2 : 1;
    /* she lays only from beads beyond the one she keeps back for her player */
    if (u->store < cost + (has_player ? 1 : 0)) return;
    u->store = (int16_t)(u->store - cost);
    u->gest = GESTATION;
    u->spawn_kind = (uint8_t)kind;
    u->cycle++;
}

static void spider_update(MndWorld *w, int i) {
    MndUnit *u = &w->u[i];
    u->anim++;
    int px = u->x / FP, py = u->y / FP;
    /* the bite */
    if (u->bite <= 0) {
        int best = -1, bestd = 1 << 30;
        for (int k = 0; k < w->n_units; k++) {
            const MndUnit *o = &w->u[k];
            if (!o->kind || o->kind == MK_SPIDER) continue;
            int reach = o->kind == MK_QUEEN ? 15 : 11;
            int dx = o->x / FP - px, dy = o->y / FP - py, d = dx * dx + dy * dy;
            if (d <= reach * reach && d < bestd) { bestd = d; best = k; }
        }
        if (best >= 0) {
            damage(w, best, SPIDER_DMG, MND_WILD);
            w->ev.bites++;
            u->bite = SPIDER_BITE;
        }
    }
    /* who it wants: the nearest ant it can see, either colour */
    if ((w->frame + i) % 15 == 0) {
        int best = -1, bestd = 72 * 72;
        for (int k = 0; k < w->n_units; k++) {
            const MndUnit *o = &w->u[k];
            if (!o->kind || o->kind == MK_SPIDER) continue;
            int dx = o->x / FP - px, dy = o->y / FP - py;
            if (iabs(dx) > 90 || iabs(dy) > 90) continue;
            int d = dx * dx + dy * dy + (o->kind == MK_QUEEN ? 32 * 32 : 0);
            if (d >= bestd || !mnd_los(w, px, py, o->x / FP, o->y / FP)) continue;
            best = k;
            bestd = d;
        }
        if (best >= 0) {
            u->memx = w->u[best].x;
            u->memy = w->u[best].y;
            u->mem_t = 180;
            u->target = (int16_t)best;
        } else {
            u->target = -1;
        }
    }
    int dx = 0, dy = 0;
    if (u->mem_t > 0) {
        u->mem_t--;
        int ddx = (u->memx - u->x) / FP, ddy = (u->memy - u->y) / FP;
        if (iabs(ddx) <= 3 && iabs(ddy) <= 3 && u->target < 0) u->mem_t = 0;
        int d = mnd_octant(ddx, ddy);
        if (d >= 0 && (iabs(ddx) > 6 || iabs(ddy) > 6)) {
            int sp = d & 1 ? SPIDER_SPEED_D : SPIDER_SPEED;
            dx = MND_DX[d] * sp;
            dy = MND_DY[d] * sp;
            u->face = (int8_t)d;
        }
    } else {
        if (--u->wander_t <= 0) {
            u->step_dir = (int8_t)rng_range(&w->rng, 0, 7);
            u->wander_t = (int16_t)rng_range(&w->rng, 60, 150);
        }
        int d = u->step_dir;
        if ((w->frame & 1) == 0) {
            dx = MND_DX[d] * 2;
            dy = MND_DY[d] * 2;
            u->face = (int8_t)d;
        }
    }
    if ((dx || dy) && !move_unit(w, u, dx, dy)) u->wander_t = 0;
}

/* ------------------------------------------------------------------ */
/* the player's ant                                                     */

static void give_order(MndWorld *w, int side, int line) {
    MndMenu *mn = &w->menu[side];
    mn->flash = (int8_t)line;
    mn->flash_t = 40;
    switch (line) {
    case MI_FOLLOW: mnd_shout(w, side, ORD_FOLLOW, false); break;
    case MI_SOLDIER_FOLLOW: mnd_shout(w, side, ORD_FOLLOW, true); break;
    case MI_HALT: mnd_shout(w, side, ORD_HOLD, false); break;
    case MI_INSTINCT: mnd_shout(w, side, ORD_INSTINCT, false); break;
    case MI_QUEEN: if (!mnd_toggle_queen(w, side)) mn->flash_t = 0; break;
    default: break;
    }
    w->ev.orders++;
}

static void player_update(MndWorld *w, int side, const MndPad *pad) {
    int p = w->player[side];
    MndMenu *mn = &w->menu[side];
    if (mn->flash_t > 0) mn->flash_t--;
    if (p < 0) { mn->open = false; return; }
    MndUnit *u = &w->u[p];
    if (pad->order_pressed) { mn->open = true; mn->open_t = 0; }
    if (mn->open) {
        mn->open_t++;
        if (!mnd_menu_line_ok(w, side, mn->sel)) mn->sel = MI_FOLLOW;
        if (pad->order_held) {
            int dir = pad->menu_down ? 1 : pad->menu_up ? -1 : 0;            if (dir) {
                int s = mn->sel;
                for (int k = 0; k < MI_COUNT; k++) {
                    s = (s + dir + MI_COUNT) % MI_COUNT;
                    if (mnd_menu_line_ok(w, side, s)) break;
                }
                mn->sel = (int8_t)s;
                w->ev.menu_moves++;
            }
        } else {
            /* letting go gives the order under the cursor */
            give_order(w, side, mn->sel);
            mn->open = false;
        }
    }
    if (!mn->open) {
        int mx = (pad->right ? 1 : 0) - (pad->left ? 1 : 0), my = (pad->down ? 1 : 0) - (pad->up ? 1 : 0);
        int d = mnd_octant(mx, my);
        if (d >= 0) {
            if (!pad->spit_held) u->face = (int8_t)d;
            int sp = d & 1 ? 6 : 8;
            move_unit(w, u, MND_DX[d] * sp, MND_DY[d] * sp);
            u->anim++;
        }
        if (pad->spit_held && u->cool <= 0) {
            spit(w, side, u->face, u->x, u->y);
            u->cool = PLAYER_COOL;
            /* soldiers on Follow spit when you do, the same way */
            for (int k = 0; k < w->n_units; k++) {
                MndUnit *s = &w->u[k];
                if (s->kind != MK_SOLDIER || s->side != side || s->order != ORD_FOLLOW || s->cool > 0 || s->carry) continue;
                s->face = u->face;
                spit(w, side, u->face, s->x, s->y);
                s->cool = BLUE_COOL;
            }
        }
    }
    pickup_and_deliver(w, p);
}

static void respawn_update(MndWorld *w, int side) {
    if (w->player[side] >= 0) return;
    w->dead_t[side]++;
    if (w->dead_t[side] < MND_RESPAWN_T) return;
    for (int q = 0; q < w->n_units; q++) {
        MndUnit *qu = &w->u[q];
        if (qu->kind != MK_QUEEN || qu->side != side || qu->store < 1) continue;
        int px = qu->x / FP, py = qu->y / FP + 12;
        if (!box_free(w, px * FP, py * FP, 3)) py -= 24;
        int n = mnd_add_unit(w, MK_PLAYER, side, px, py);
        if (n < 0) return;
        qu->store--;
        w->player[side] = n;
        w->dead_t[side] = 0;
        w->respawns[side]++;
        w->ev.respawns++;
        return;
    }
}

/* ------------------------------------------------------------------ */
/* the frame                                                            */

static void shots_update(MndWorld *w) {
    for (int s = 0; s < MND_MAX_SHOTS; s++) {
        MndShot *sh = &w->shot[s];
        if (!sh->on) continue;
        int sp = sh->dir & 1 ? SPIT_SPEED_D : SPIT_SPEED;
        /* two half-moves so nothing slips between frames */
        for (int half = 0; half < 2 && sh->on; half++) {
            sh->x += MND_DX[sh->dir] * sp / 2;
            sh->y += MND_DY[sh->dir] * sp / 2;
            int px = sh->x / FP, py = sh->y / FP;
            if (mnd_opaque(w, px, py)) { sh->on = 0; break; }
            for (int k = 0; k < w->n_units; k++) {
                MndUnit *o = &w->u[k];
                if (!o->kind || o->side == sh->side) continue;
                int r = radius_of(o->kind);
                int dx = o->x / FP - px, dy = o->y / FP - py;
                if (dx * dx + dy * dy > r * r) continue;
                sh->on = 0;
                w->ev.hits++;
                damage(w, k, 1, sh->side);
                break;
            }
        }
        if (sh->on && --sh->life <= 0) sh->on = 0;
    }
}

/* Two enemy ants that touch fight at once: the healthier one lives, keeping
 * the difference. Workers that reach an enemy queen nibble her. */
static void melee(MndWorld *w) {
    for (int i = 0; i < w->n_units; i++) {
        MndUnit *a = &w->u[i];
        if (!is_ant(a->kind)) continue;
        for (int j = i + 1; j < w->n_units && is_ant(a->kind); j++) {
            MndUnit *b = &w->u[j];
            if (!is_ant(b->kind) || b->side == a->side) continue;
            if (iabs(a->x - b->x) >= 6 * FP || iabs(a->y - b->y) >= 6 * FP) continue;
            int m = imin(a->hp, b->hp);
            int sa = a->side, sb = b->side;
            w->ev.melee++;
            damage(w, i, m, sb);
            damage(w, j, m, sa);
        }
    }
    for (int i = 0; i < w->n_units; i++) {
        MndUnit *a = &w->u[i];
        if (a->kind != MK_WORKER || a->bite > 0) continue;
        for (int q = 0; q < w->n_units; q++) {
            MndUnit *qu = &w->u[q];
            if (qu->kind != MK_QUEEN || qu->side == a->side || dist2(a, qu) > 12 * 12) continue;
            a->bite = QUEEN_NIBBLE;
            damage(w, q, 1, a->side);
            break;
        }
    }
}

static void check_status(MndWorld *w) {
    if (w->status != MND_PLAYING) return;
    bool out[2];
    for (int s = 0; s < 2; s++) {
        bool has_player = s == MND_BLUE || w->versus;
        bool queen = mnd_queen_of(w, s) >= 0;
        if (!has_player) out[s] = mnd_count(w, s, MK_NONE) == 0;
        else out[s] = !queen || (w->player[s] < 0 && w->dead_t[s] >= MND_RESPAWN_T + MND_STARVE_T) ||
                      mnd_count(w, s, MK_NONE) == 0;
    }
    if (out[MND_RED] && !out[MND_BLUE]) w->status = MND_WON;
    else if (out[MND_BLUE]) w->status = MND_LOST;
}

void mnd_step(MndWorld *w, const MndPad pads[2]) {
    memset(&w->ev, 0, sizeof w->ev);
    if (w->status != MND_PLAYING) return;
    w->frame++;
    memset(claims, 0, sizeof claims);
    for (int i = 0; i < w->n_units; i++)
        if (w->u[i].kind && w->u[i].goal_bead >= 0 && w->u[i].goal_bead < MND_MAX_BEADS) {
            if (w->bead[w->u[i].goal_bead].on) claims[w->u[i].goal_bead]++;
            else w->u[i].goal_bead = -1;
        }
    for (int i = 0; i < w->n_units; i++) {
        MndUnit *u = &w->u[i];
        if (!u->kind) continue;
        if (u->hurt > 0) u->hurt--;
        if (u->cool > 0) u->cool--;
        if (u->bite > 0) u->bite--;
        if (u->ack > 0) u->ack--;
    }
    player_update(w, MND_BLUE, &pads[0]);
    if (w->versus) player_update(w, MND_RED, &pads[1]);
    for (int i = 0; i < w->n_units; i++) {
        MndUnit *u = &w->u[i];
        switch (u->kind) {
        case MK_WORKER:
        case MK_SOLDIER: ant_update(w, i); break;
        case MK_QUEEN: queen_update(w, i); break;
        case MK_SPIDER: spider_update(w, i); break;
        default: break;
        }
    }
    shots_update(w);
    melee(w);
    respawn_update(w, MND_BLUE);
    if (w->versus) respawn_update(w, MND_RED);
    while (w->n_units > 0 && !w->u[w->n_units - 1].kind) w->n_units--;
    check_status(w);
}

/* ------------------------------------------------------------------ */
/* loading a map                                                        */

void mnd_load_map(MndWorld *w, int map, uint64_t seed, bool versus) {
    memset(w, 0, sizeof *w);
    rng_seed(&w->rng, seed);
    const MndMap *m = &MND_MAPS[map];
    w->versus = versus;
    w->map = (uint8_t)map;
    w->red_mistake = 10;
    w->player[0] = w->player[1] = -1;
    red_prod = m->red_prod ? m->red_prod : "W";
    int h = 0, wd = 0;
    while (m->rows[h]) { wd = imax(wd, (int)strlen(m->rows[h])); h++; }
    w->w = imin(wd, MND_MAXW);
    w->h = imin(h, MND_MAXH);
    int start_x[2] = {-1, -1}, start_y[2] = {-1, -1};
    for (int y = 0; y < w->h; y++) {
        const char *row = m->rows[y];
        int len = (int)strlen(row);
        for (int x = 0; x < w->w; x++) {
            char c = x < len ? row[x] : '#';
            int px = x * MND_TILE + 4, py = y * MND_TILE + 4;
            uint8_t t = TL_GROUND;
            switch (c) {
            case '#': t = TL_ROCK; break;
            case '%': t = TL_ROOT; break;
            case '~': t = TL_WATER; break;
            case ',': t = TL_GRASS; break;
            case ':': t = TL_PEBBLE; break;
            default: break;
            }
            w->tile[y][x] = t;
            int n = -1;
            switch (c) {
            case 'o':
                if (w->n_beads < MND_MAX_BEADS) w->bead[w->n_beads++] = (MndBead){1, (int16_t)px, (int16_t)py};
                break;
            case 'Q': n = mnd_add_unit(w, MK_QUEEN, MND_BLUE, px, py); if (n >= 0) w->u[n].store = m->blue_store; break;
            case 'q':
                n = mnd_add_unit(w, MK_QUEEN, MND_RED, px, py);
                if (n >= 0) {
                    w->u[n].store = versus ? m->blue_store : m->red_store;
                    w->u[n].cycle = (uint8_t)rng_range(&w->rng, 0, 3);
                }
                break;
            case 'W': mnd_add_unit(w, MK_WORKER, MND_BLUE, px, py); break;
            case 'S': mnd_add_unit(w, MK_SOLDIER, MND_BLUE, px, py); break;
            case 'w': mnd_add_unit(w, MK_WORKER, MND_RED, px, py); break;
            case 's': mnd_add_unit(w, MK_SOLDIER, MND_RED, px, py); break;
            case 'X': mnd_add_unit(w, MK_SPIDER, MND_WILD, px, py); break;
            case 'P': start_x[0] = px; start_y[0] = py; break;
            case 'p': start_x[1] = px; start_y[1] = py; break;
            default: break;
            }
        }
    }
    for (int s = 0; s < (versus ? 2 : 1); s++) {
        if (start_x[s] < 0) {
            int q = mnd_queen_of(w, s);
            if (q < 0) continue;
            start_x[s] = w->u[q].x / FP;
            start_y[s] = w->u[q].y / FP + 12;
        }
        int n = mnd_add_unit(w, MK_PLAYER, s, start_x[s], start_y[s]);
        w->player[s] = n;
        if (n >= 0) w->u[n].face = s ? 4 : 0;
    }
}
