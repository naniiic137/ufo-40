/* HOOPLA - the CPU. One brain per CPU fighter; it reads the pit and holds
 * the same buttons a player would (B to move its own way, A to shoot, DOWN
 * to block or lunge). It knows how each of the eight gets up and down the
 * pit. Like the original's CPU it is muddled by Moss's gravity: a CPU Moss
 * turns over late and at the wrong times. The tests' demo player is this
 * same brain on its sharpest setting (SK_ACE). */
#include "hoop.h"

typedef struct {
    int react;      /* frames between fresh looks */
    int block;      /* % chance to block a lunge it sees coming */
    int aggro;      /* % chance to take a shot it has */
    int slack;      /* px of aim it accepts */
    int dodge;      /* % chance to hop a shot */
    int lunge;      /* % chance to lunge when close */
} Skill;

static const Skill SKILL[4] = {
    {26, 12, 30, 5, 8, 25},   /* calm */
    {14, 35, 55, 7, 25, 45},  /* rough */
    {14, 35, 55, 7, 25, 45},  /* riot (the same, but two of them) */
    {4, 90, 90, 8, 75, 80},   /* ace: the demo player */
};

enum { GO_NONE, GO_RING, GO_WAIT, GO_FIGHT, GO_AWAY };

void hoop_brain_init(Brain *b, int skill, int who) {
    memset(b, 0, sizeof *b);
    b->skill = iclamp(skill, 0, 3);
    b->react = SKILL[b->skill].react;
    b->think_t = who * 3;
    b->foe = who == 0 ? 1 : 0;
}

/* ---- looking at the pit ----------------------------------------------------- */

static int fcx(const Fighter *f) { return HPX(f->x) + HOOP_FW / 2; }
static int fcy(const Fighter *f) { return HPX(f->y) + HOOP_FH / 2; }
static bool on_ground(const Fighter *f) { return f->ground != GND_AIR; }

static int chance(int pct) { return rng_range(&hg.m.rng, 0, 99) < pct; }

/* a shot of someone else's coming at me */
static int threat(int who, int *eta) {
    const Match *m = &hg.m;
    const Fighter *f = &m->f[who];
    int cx = fcx(f), cy = fcy(f), best = -1, bt = 999;
    for (int i = 0; i < HOOP_MAX_SHOTS; i++) {
        const Shot *s = &m->shot[i];
        if (!s->alive || s->held || s->stuck || (s->owner == who && !s->self_ok)) continue;
        int sx = HPX(s->x), sy = HPX(s->y);
        int dx = cx - sx, dy = cy - sy;
        if (iabs(dx) > 70 || iabs(dy) > 40) continue;
        int vx = s->vx, vy = s->vy;
        if (vx == 0 && vy == 0) continue;
        /* frames until it reaches my x (or y if it moves mostly up and down) */
        int t = iabs(vx) > iabs(vy) ? (vx != 0 && (dx > 0) == (vx > 0) ? dx * HQ / vx : 999)
                                    : (vy != 0 && (dy > 0) == (vy > 0) ? dy * HQ / vy : 999);
        if (t < 0 || t > 40) continue;
        int py = sy + vy * t / HQ, px = sx + vx * t / HQ;
        if (iabs(py - cy) > 12 || iabs(px - cx) > 12) continue;
        if (t < bt) { bt = t; best = i; }
    }
    *eta = bt;
    return best;
}

/* the nearest hoop worth going for, and how far */
static int pick_ring(int who, int *cost) {
    const Match *m = &hg.m;
    const Fighter *f = &m->f[who];
    int cx = fcx(f), cy = fcy(f), best = -1, bc = 1 << 30;
    for (int i = 0; i < HOOP_MAX_RINGS; i++) {
        const Ring *g = &m->ring[i];
        if (!g->alive || g->claw >= 0) continue;
        int rx = HPX(g->x), ry = HPX(g->y);
        int c = iabs(rx - cx) + iabs(ry - cy) * 2;
        if (g->state == RG_FRESH && g->burn_t > 0) c += g->burn_t / 3;
        if (g->state == RG_LOOSE) c -= 20;
        /* a hoop the other side is closer to is worth less */
        for (int k = 0; k < m->n; k++) {
            if (k == who || !m->f[k].on) continue;
            int oc = iabs(rx - fcx(&m->f[k])) + iabs(ry - fcy(&m->f[k])) * 2;
            if (oc < c - 40) c += 40;
        }
        if (c < bc) { bc = c; best = i; }
    }
    *cost = bc;
    return best;
}

static int pick_foe(int who) {
    const Match *m = &hg.m;
    const Fighter *f = &m->f[who];
    int best = -1, bs = 1 << 30;
    for (int k = 0; k < m->n; k++) {
        if (k == who || !m->f[k].on) continue;
        const Fighter *o = &m->f[k];
        int d = iabs(fcx(o) - fcx(f)) + iabs(fcy(o) - fcy(f)) - o->rings * 30;
        /* the player is the one they all want (the CPUs in a riot gang up) */
        if (o->ctrl != CTRL_CPU) d -= 60;
        if (d < bs) { bs = d; best = k; }
    }
    return best < 0 ? (who == 0 ? 1 : 0) : best;
}

/* ---- buttons ------------------------------------------------------------------ */

/* a fresh press needs the button up the frame before */
static void tap(Brain *b, int *out, int bit) {
    if (!(b->last & bit)) *out |= bit;
}

/* ---- getting about -------------------------------------------------------------- */

static void go_x(const Fighter *f, int tx, int *out, int near) {
    int dx = tx - fcx(f);
    if (dx > near) *out |= HP_R;
    else if (dx < -near) *out |= HP_L;
}

/* where Clamp's claw would bite if thrown this way now (0: nowhere) */
static int claw_probe(const Fighter *f, int dir, int *ax, int *ay) {
    const Match *m = &hg.m;
    int x = f->x + HOOP_FW * HQ / 2, y = f->y, dist = 0;
    int vx = dir ? dir * 1086 : 0, vy = dir ? -1086 : -1536;
    for (int k = 0; k < 24; k++) {
        x += vx;
        y += vy;
        dist += dir ? 1536 : 1536;
        if (y <= HOOP_AT * HQ) { *ax = HPX(x); *ay = HOOP_AT; return 1; }
        for (int i = 0; i < m->nledges; i++) {
            const Ledge *l = &m->ledge[i];
            if (x >= l->x * HQ && x < (l->x + l->w) * HQ && y >= l->y * HQ && y <= (l->y + HOOP_PLAT_H + 2) * HQ) {
                *ax = HPX(x);
                *ay = l->y + HOOP_PLAT_H;
                return 1;
            }
        }
        if (x < HOOP_AL * HQ || x > HOOP_AR * HQ || dist > 110 * HQ) return 0;
    }
    return 0;
}

static void climb(int who, Brain *b, int tx, int ty, int *out) {
    Match *m = &hg.m;
    Fighter *f = &m->f[who];
    int cx = fcx(f), cy = fcy(f), dy = ty - cy, adx = iabs(tx - cx);
    /* far off to the side: walk under it first (the fliers can just go) */
    if (on_ground(f) && f->claw == 0 && adx > (f->kind == HF_BRISTLE ? 22 : 56) && f->kind != HF_ASTRA &&
        f->kind != HF_PEWIT) {
        bool edge = false;
        if (hoop_on_ledge(f)) {
            const Ledge *L = &m->ledge[f->ground];
            edge = (tx > cx && cx > L->x + L->w - 10) || (tx < cx && cx < L->x + 10);
        }
        if (!edge) return;
    }
    switch (f->kind) {
    case HF_TANSY:
        if (on_ground(f)) {
            if (b->hold_b == 0 && !(b->last & HP_B)) b->hold_b = dy < -34 || adx > 40 ? 14 : 7;
        } else if (f->jumps > 0 && f->vy > (adx > 30 ? 150 : -120) && cy > ty + 4 && !(b->last & HP_B)) {
            b->hold_b = 14;
        }
        break;
    case HF_PEWIT:
        if (on_ground(f)) {
            if (b->hold_b == 0 && !(b->last & HP_B)) b->hold_b = 12;
        } else if (cy > ty - 6 && f->vy > -160) {
            tap(b, out, HP_B);
        }
        break;
    case HF_ASTRA:
        if (cy > ty - 6 || f->vy > 200) *out |= HP_B;
        break;
    case HF_COLLIER:
        if (f->lift < 0) {
            if (cy < ty - 10) tap(b, out, HP_B);
        } else if (!f->lift && on_ground(f) && iabs(tx - cx) < 14) {
            tap(b, out, HP_B);
        }
        break;
    case HF_GULP:
        if (on_ground(f) && b->hold_b == 0 && f->charge_t == 0 && !(b->last & HP_B)) {
            int h = -dy + 10;
            int v = hoop_isqrt((int64_t)2 * HOOP_GRAV * h * HQ);
            b->hold_b = iclamp((v - 560) / 23 + 4, 2, HOOP_GLOW_T - 2);
        }
        break;
    case HF_BRISTLE:
        if (on_ground(f)) {
            /* a trap of hers close by and arming: stand on it */
            int near = -1, nd = 999;
            for (int i = 0; i < HOOP_MAX_MINES; i++) {
                const Mine *q = &m->mine[i];
                if (!q->alive || q->owner != who || !q->landed) continue;
                int d = iabs(HPX(q->x) - cx);
                if (iabs(HPX(q->y) - hoop_feet_y(f)) < 3 && d < nd) { nd = d; near = i; }
            }
            if (near >= 0 && nd < 40) {
                b->tx = HPX(m->mine[near].x);
                b->goal = b->goal == GO_FIGHT ? GO_FIGHT : b->goal;
                *out &= ~(HP_L | HP_R);
                if (nd > 2) go_x(f, HPX(m->mine[near].x), out, 1);
            } else if (f->act_t <= 0) {
                tap(b, out, HP_B);
                *out &= ~(HP_L | HP_R);
            }
        }
        break;
    case HF_CLAMP:
        if (f->claw == 0 && !(b->last & HP_B)) {
            /* the way to throw the claw that bites nearest where it's going */
            int best = -2, bs = -(1 << 30);
            for (int dir = -1; dir <= 1; dir++) {
                int ax, ay;
                if (!claw_probe(f, dir, &ax, &ay) || ay > cy - 10) continue;
                int sc = -iabs(ay - ty) - iabs(ax - tx) / 2;
                if (sc > bs) { bs = sc; best = dir; }
            }
            if (best > -2) {
                *out &= ~(HP_L | HP_R);
                if (best) *out |= best > 0 ? HP_R : HP_L;
                *out |= HP_B;
            }
        } else if (f->claw == 2) {
            /* hanging: let go once up at the claw, or level with the hoop */
            if (cy < ty + 2 || f->rope <= 9 * HQ) tap(b, out, HP_B);
        }
        break;
    }
}

/* Moss: turn over to rise, walk off the end of any ledge he's stuck under,
 * turn back once above. (A CPU Moss's muddles are in hoop_brain_think.) */
static void moss_nav(int who, Brain *b, int tx, int ty, int *out) {
    Match *m = &hg.m;
    Fighter *f = &m->f[who];
    int cx = fcx(f), cy = fcy(f), dy = ty - cy;
    bool sloppy = b->skill != SK_ACE;
    if (f->g > 0) {
        if (dy < -12) {
            if ((on_ground(f) || f->vy >= 0) && (!sloppy || chance(35))) tap(b, out, HP_B);
        } else if (dy > 18 && hoop_on_ledge(f)) {
            /* walk off the nearer end */
            const Ledge *L = &m->ledge[f->ground];
            if (tx > L->x - 4 && tx < L->x + L->w + 4) {
                int go = cx - L->x < L->x + L->w - cx ? L->x - 10 : L->x + L->w + 10;
                if (go < HOOP_AL + 6) go = L->x + L->w + 10;
                if (go > HOOP_AR - 6) go = L->x - 10;
                *out &= ~(HP_L | HP_R);
                go_x(f, go, out, 1);
            }
        }
        return;
    }
    if (cy < ty - 14 || dy > 14) {
        if (!sloppy || chance(30)) tap(b, out, HP_B);
    } else if (hoop_on_ledge(f)) {
        /* under a ledge: off its nearer end towards the hoop, and on up */
        const Ledge *L = &m->ledge[f->ground];
        int go = tx < L->x + L->w / 2 ? L->x - 9 : L->x + L->w + 9;
        if (go < HOOP_AL + 6) go = L->x + L->w + 9;
        if (go > HOOP_AR - 6) go = L->x - 9;
        *out &= ~(HP_L | HP_R);
        go_x(f, go, out, 1);
    }
}

/* how far up each one can get in one go (px), for the stepping stones */
static int reach_of(int kind) {
    switch (kind) {
    case HF_TANSY: return 68;
    case HF_GULP: return 120;
    case HF_BRISTLE: return 80;
    case HF_CLAMP: return 86;
    default: return 999;
    }
}

/* too high for one go: a ledge on the way up to stand on first */
static void waypoint(const Fighter *f, int *tx, int *ty) {
    const Match *m = &hg.m;
    int feet = hoop_feet_y(f), tfeet = *ty + 7, cx = fcx(f);
    if (!on_ground(f) && f->claw != 2 && !f->lift) return;
    int reach = reach_of(f->kind);
    if (f->g < 0 || feet - tfeet <= reach) return;
    int best = -1, bs = -(1 << 30), bx = *tx;
    for (int i = 0; i < m->nledges; i++) {
        const Ledge *l = &m->ledge[i];
        if (l->y >= feet - 8 || l->y < tfeet - 4 || feet - l->y > reach) continue;
        int x = iclamp(*tx, l->x + 6, l->x + l->w - 6);
        int sc = (feet - l->y) * 3 - iabs(x - cx) - iabs(x - *tx) / 2;
        if (sc > bs) { bs = sc; best = i; bx = x; }
    }
    if (best >= 0) {
        *tx = bx;
        *ty = m->ledge[best].y - 7;
    }
}

/* lower down: let go, get off, or drop through the ledge (DOWN + B) */
static void descend(int who, Brain *b, int *out) {
    Fighter *f = &hg.m.f[who];
    if (f->kind == HF_CLAMP && f->claw == 2) { tap(b, out, HP_B); return; }
    if (f->kind == HF_COLLIER && f->lift < 0) { tap(b, out, HP_B); return; }
    if (hoop_on_ledge(f) && f->lift == 0) {
        *out |= HP_D;
        tap(b, out, HP_B);
    }
}

/* ---- fighting ----------------------------------------------------------------------- */

static bool has_lunge(int kind) { return kind != HF_CLAMP && kind != HF_GULP; }

/* A (and maybe UP) if the foe is where this weapon goes */
static int aim(int who, Brain *b, const Fighter *o) {
    Match *m = &hg.m;
    Fighter *f = &m->f[who];
    const Skill *sk = &SKILL[b->skill];
    int dx = fcx(o) - fcx(f), dy = fcy(o) - fcy(f), adx = iabs(dx);
    int facing = (dx > 0) == (f->face > 0);
    int s = sk->slack;
    switch (f->kind) {
    case HF_TANSY:
        if (iabs(dy) < 5 + s && adx < 220 && facing) return HP_A;
        if (dy > 8 && dy < 70 && adx < 50 && adx > 12 && facing) return HP_A | HP_U;
        break;
    case HF_CLAMP:
        if (iabs(dy) < 8 + s && adx < 130 && facing) return HP_A;
        if (dy < -10 && dy > -50 && adx < 90 && facing) return HP_A | HP_U;
        if (dy > 10 && dy < 50 && adx < 90 && facing) return HP_A;
        break;
    case HF_MOSS:
        if (iabs(dy) < 4 + s && adx < 240 && facing) return HP_A;
        break;
    case HF_BRISTLE:
        if (adx < 48 && facing) {
            if (on_ground(f) && dy < 4 && dy > -36) return HP_A;
            if (!on_ground(f) && dy > -4 && dy < 36) return HP_A;
        }
        break;
    case HF_PEWIT:
        if (facing && adx > 24 && adx < 110 && dy > -18 && dy < 30) return HP_A;
        if (facing && adx < 60 && dy < -18 && dy > -80) return HP_A | HP_U;
        break;
    case HF_COLLIER:
        if (facing && adx > 20 && adx < 96 && dy > -20 && dy < 30) return HP_A;
        if (facing && adx < 60 && dy < -20 && dy > -80) return HP_A | HP_U;
        break;
    case HF_ASTRA:
        if (facing && dy > 0 && iabs(adx - dy) < 6 + s) return HP_A;
        if (facing && dy < 0 && iabs(adx + dy) < 6 + s) return HP_A | HP_U;
        /* off the floor and back up */
        if (facing && on_ground(f) == 0 && iabs(dy) < 6 && adx > 60) {
            int fy = HOOP_AB - fcy(f);
            if (iabs(adx - 2 * fy) < 10 + s) return HP_A;
        }
        break;
    case HF_GULP:
        if (iabs(dy) < 5 + s && adx < 200 && facing) return HP_A;
        if (facing && adx < 140 && ((on_ground(f) && dy < -8) || (!on_ground(f) && dy > 8))) {
            /* hold to sight it: how many steps of 5 degrees */
            int a = hoop_angle_of(dx, dy);
            int steps = f->face > 0 ? (a > 36 ? 72 - a : a) : (a > 36 ? a - 36 : 36 - a);
            if (steps >= 2 && steps <= 18) {
                b->want_aim = steps * 2;
                return HP_A;
            }
        }
        break;
    }
    return 0;
}

static void fight(int who, Brain *b, const Fighter *o, int *out) {
    Match *m = &hg.m;
    Fighter *f = &m->f[who];
    const Skill *sk = &SKILL[b->skill];
    int dx = fcx(o) - fcx(f), dy = fcy(o) - fcy(f), adx = iabs(dx);
    bool fresh_look = b->think_t == 0;
    /* no point swinging at someone who can't be hurt yet, unless it's about to end */
    bool open = o->inv_t < 6;

    /* Gulp's sight: keep A held until the steps are up */
    if (f->kind == HF_GULP && f->aiming) {
        if (f->aim_t < b->want_aim) *out |= HP_A;
        return;
    }
    /* Collier's charge in hand */
    if (f->kind == HF_COLLIER && f->held_bomb >= 0) {
        int eta;
        bool shield = threat(who, &eta) >= 0;
        if (shield || b->hold_a > 0) *out |= HP_A;
        if (dy < -20) *out |= HP_U;
        if (b->hold_a > 0) b->hold_a--;
        return;
    }
    /* a lunge from close in */
    if (has_lunge(f->kind) && open && adx < 28 && iabs(dy) < 12 && f->melee_cd == 0 && !o->blocking) {
        if (fresh_look && chance(sk->lunge)) {
            if ((dx > 0) != (f->face > 0)) *out |= dx > 0 ? HP_R : HP_L;
            *out |= HP_D;
            tap(b, out, HP_A);
            return;
        }
    }
    /* Gulp's glowing leap at someone above */
    if (f->kind == HF_GULP && open && on_ground(f) && adx < 26 && dy < -10 && dy > -70 && f->charge_t == 0 &&
        fresh_look && chance(sk->lunge)) {
        b->hold_b = HOOP_GLOW_T + 2;
    }
    /* Clamp's swing: a diagonal claw from just below and to the side */
    if (f->kind == HF_CLAMP && open && f->claw == 0 && adx > 12 && adx < 50 && dy < 0 && dy > -40 && fresh_look &&
        chance(sk->lunge / 2) && !(b->last & HP_B)) {
        *out |= (dx > 0 ? HP_R : HP_L) | HP_B;
        return;
    }
    if (!open || f->fire_cd > 0) return;
    int a = aim(who, b, o);
    if (!a && adx < 200 && iabs(dy) < 50 && (dx > 0) != (f->face > 0) && fresh_look) {
        /* turn to face them */
        *out &= ~(HP_L | HP_R);
        *out |= dx > 0 ? HP_R : HP_L;
        return;
    }
    if (a && (fresh_look ? chance(sk->aggro) : 0)) {
        if (a & HP_U) *out |= HP_U;
        *out &= ~HP_D;
        tap(b, out, HP_A);
        if (f->kind == HF_COLLIER) b->hold_a = 1;
    }
}

/* ---- the brain ------------------------------------------------------------------------ */

int hoop_brain_think(int who) {
    Match *m = &hg.m;
    Fighter *f = &m->f[who];
    Brain *b = &m->brain[who];
    const Skill *sk = &SKILL[b->skill];
    int out = 0;
    if (!f->on || f->hurt_t > 0 || f->dizzy_t > 0) {
        b->last = 0;
        b->hold_b = 0;
        return 0;
    }
    int react = b->react;
    if (f->kind == HF_MOSS && b->skill != SK_ACE) react = react * 3 / 2;
    if (b->think_t > 0) b->think_t--;
    else b->think_t = react;
    /* a CPU Moss now and then turns his gravity over for no reason and then
     * just stays where he ends up for 1 to 2.5 s, shooting if he can */
    bool muddled = f->kind == HF_MOSS && b->skill != SK_ACE;
    if (b->mistake_t > 0) b->mistake_t--;
    else if (muddled && b->think_t == 0 && chance(8)) {
        b->mistake_t = rng_range(&m->rng, 60, 150);
        b->mistake_flip = 1;
    }

    b->foe = pick_foe(who);
    const Fighter *o = &m->f[b->foe];
    if (muddled && b->mistake_t > 0) {
        if (b->mistake_flip && !(b->last & HP_B)) {
            out |= HP_B;
            b->mistake_flip = 0;
        }
        fight(who, b, o, &out);
        out &= ~(HP_L | HP_R | HP_D);
        b->hold_b = 0;
        b->last = out;
        return out;
    }
    int cx = fcx(f), cy = fcy(f);

    /* ---- what to do: every fresh look */
    if (b->think_t == 0 || b->goal == GO_NONE) {
        int cost, r = pick_ring(who, &cost);
        bool lead = f->rings > o->rings;
        b->goal = GO_FIGHT;
        if (r >= 0) {
            const Ring *g = &m->ring[r];
            bool hot = g->state == RG_FRESH && g->burn_t > 0;
            int eta = (iabs(HPX(g->x) - cx) + iabs(HPX(g->y) - cy)) * 3 / 4;
            bool want = cost < 170 || o->rings == 0 || (lead && cost < 260) || g->state == RG_LOOSE;
            if (o->dizzy_t > 20 && iabs(fcx(o) - cx) < 90) want = false;
            if (want) {
                b->goal = hot && g->burn_t > eta + 10 ? GO_WAIT : GO_RING;
                b->tx = HPX(g->x);
                b->ty = HPX(g->y);
                if (b->goal == GO_WAIT) {
                    /* wait beside it, out of the flames */
                    b->tx += cx < b->tx ? -16 : 16;
                    if (b->tx < HOOP_AL + 6) b->tx = HPX(g->x) + 16;
                    if (b->tx > HOOP_AR - 6) b->tx = HPX(g->x) - 16;
                }
            }
        }
        if (b->goal == GO_FIGHT) {
            /* stand off at this weapon's range, at their height */
            int range = 60;
            switch (f->kind) {
            case HF_BRISTLE: range = 26; break;
            case HF_PEWIT: case HF_COLLIER: range = 64; break;
            case HF_ASTRA: range = 0; break;
            case HF_TANSY: case HF_MOSS: case HF_GULP: range = 80; break;
            default: break;
            }
            int ox = fcx(o);
            b->tx = ox + (cx < ox ? -range : range);
            if (b->tx < HOOP_AL + 8) b->tx = ox + range;
            if (b->tx > HOOP_AR - 8) b->tx = ox - range;
            b->ty = fcy(o);
            if (f->kind == HF_ASTRA) {
                /* above and to the side, raining rays */
                b->tx = ox + (cx < ox ? -40 : 40);
                b->ty = fcy(o) - 40;
                if (b->ty < HOOP_AT + 12) b->ty = HOOP_AT + 12;
            }
            if (o->dizzy_t > 0 && has_lunge(f->kind)) b->tx = ox + (cx < ox ? -16 : 16);
        }
        /* stuck? try somewhere else for a while */
        if (iabs(cx - b->last_x) < 2 && iabs(cy - b->last_y) < 2 && b->goal != GO_WAIT) b->stuck_t += react;
        else b->stuck_t = 0;
        b->last_x = cx;
        b->last_y = cy;
        if (b->stuck_t > 150) {
            b->detour = 60;
            b->stuck_t = 0;
        }
    }
    int tx = b->tx, ty = b->ty;
    if (b->detour > 0) {
        b->detour--;
        tx = cx < 160 ? 250 : 70;
        ty = HOOP_AB - 30;
    }

    /* ---- move: the stepping stone is chosen on the ground and kept in the air */
    if (on_ground(f) || f->claw == 2 || f->lift) {
        b->wp = 0;
        int wx = tx, wy = ty;
        waypoint(f, &wx, &wy);
        if (wx != tx || wy != ty) { b->wp = 1; b->wpx = wx; b->wpy = wy; }
    }
    if (b->wp) { tx = b->wpx; ty = b->wpy; }
    go_x(f, tx, &out, b->goal == GO_FIGHT ? 6 : 2);
    int dy = ty - cy;
    if (f->kind == HF_MOSS) {
        moss_nav(who, b, tx, ty, &out);
    } else if (dy < -12) {
        climb(who, b, tx, ty, &out);
    } else if (dy > 18) {
        descend(who, b, &out);
    } else if (f->kind == HF_CLAMP && f->claw == 2) {
        tap(b, &out, HP_B);
    } else if (hoop_on_ledge(f) && f->kind != HF_COLLIER && iabs(tx - cx) > 8) {
        /* across a gap at the same height: hop it at the edge */
        const Ledge *L = &m->ledge[f->ground];
        bool off = tx < L->x || tx > L->x + L->w;
        bool edge = (tx > cx && cx > L->x + L->w - 8) || (tx < cx && cx < L->x + 8);
        if (off && edge) climb(who, b, tx, ty - 20, &out);
    }
    if (f->kind == HF_PEWIT && !on_ground(f) && dy > -6 && dy < 6 && f->vy > 80 && b->goal != GO_FIGHT) {
        tap(b, &out, HP_B); /* keep aloft over a hoop */
    }
    if (f->kind == HF_ASTRA && b->goal == GO_FIGHT && cy > ty) out |= HP_B;

    /* held moves */
    if (b->hold_b > 0) {
        out |= HP_B;
        b->hold_b--;
        if (f->kind == HF_GULP) {
            int dx = tx - cx;
            out &= ~(HP_L | HP_R);
            if (iabs(dx) > 10) out |= dx > 0 ? HP_R : HP_L;
        }
    }

    /* ---- keep safe: block a lunge, hop a shot */
    if (b->block_t > 0) {
        b->block_t--;
        if (on_ground(f) || f->claw == 2) return b->last = HP_D;
    }
    for (int k = 0; k < m->n; k++) {
        if (k == who) continue;
        const Fighter *e = &m->f[k];
        if (!e->on || !hoop_melee_on(e)) continue;
        int ex = fcx(e) - cx, ey = fcy(e) - cy;
        bool coming = (ex > 0 && e->vx < 0) || (ex < 0 && e->vx > 0) || e->swing_t > 0 || e->glow_jump;
        if (iabs(ex) < 44 && iabs(ey) < 22 && coming && (on_ground(f) || f->claw == 2) && f->melee_t == 0 &&
            f->charge_t == 0) {
            if (chance(sk->block)) {
                b->block_t = 18;
                b->hold_b = 0;
                return b->last = HP_D;
            }
        }
    }
    int eta;
    if (threat(who, &eta) >= 0 && b->think_t % 3 == 0) {
        const Fighter *e = o;
        bool close = iabs(fcx(e) - cx) < 34 && iabs(fcy(e) - cy) < 14;
        if (close && has_lunge(f->kind) && f->melee_cd == 0 && chance(sk->dodge)) {
            /* lunge through it */
            out &= ~(HP_L | HP_R);
            out |= fcx(e) > cx ? HP_R : HP_L;
            out |= HP_D;
            tap(b, &out, HP_A);
            b->last = out;
            return out;
        }
        if (eta < 14 && on_ground(f) && chance(sk->dodge)) {
            switch (f->kind) {
            case HF_TANSY: case HF_PEWIT: b->hold_b = 10; break;
            case HF_ASTRA: b->hold_b = 14; break;
            case HF_MOSS: if (b->skill == SK_ACE) tap(b, &out, HP_B); break;
            case HF_GULP: b->hold_b = 8; break;
            case HF_COLLIER:
                if (f->held_bomb < 0 && f->fire_cd == 0) tap(b, &out, HP_A); /* a charge to shield with */
                break;
            default: break;
            }
        }
    }

    /* ---- shoot */
    fight(who, b, o, &out);
    if ((out & HP_D) && (out & HP_B) && !hoop_on_ledge(f) && f->kind != HF_COLLIER) out &= ~HP_D;
    b->last = out;
    return out;
}
