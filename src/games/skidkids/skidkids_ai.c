/* SKID KIDS - the CPU. Rival kids play on their own (both can wind up at
 * once, they hold a full wind-up waiting for someone to bend down for a bag,
 * and they chase every item the Coach throws in, ambush or not; they read a
 * wiggly throw as a straight one, and steer round puddles); your partner
 * fetches bags and holds them for you to swap to; the demo player in the
 * tests plays your kid the way the best rival does, with real buttons. */
#include "skidkids.h"

typedef struct Skill {
    int react;   /* frames before it reacts to a bag coming at it */
    int jump;    /* % of those it jumps at all */
    int full;    /* % of throws it winds up all the way */
    int align;   /* how close in line it wants to be before winding up */
    int special; /* % of chances it takes to use a special move */
    int approach;/* how near the line it throws from */
    int ambush;  /* holds a full wind-up for someone to bend down */
    int pile;    /* 0 leaves a fallen kid alone, 1 sometimes, 2 piles on */
    int hesitate;/* frames it holds a new bag before winding up */
} Skill;

/* matches 1-4 are easy, 5 and 6 are a wall; 6 is the demo player */
static const Skill SKILL[7] = {
    /* react jump full align special approach ambush pile hesitate */
    {21, 38, 25, 10, 15, 70, 0, 0, 50},
    {18, 46, 35, 9, 25, 64, 0, 0, 40},
    {15, 54, 45, 8, 35, 58, 0, 1, 30},
    {13, 60, 55, 7, 45, 52, 1, 1, 20},
    {8, 84, 82, 5, 75, 44, 1, 2, 8},
    {6, 90, 90, 4, 88, 40, 1, 2, 6},
    {2, 100, 95, 3, 95, 38, 1, 2, 4},
};

static int dir_of(int team) { return team == 0 ? 1 : -1; }

static void seek(const Kid *k, float tx, float ty, Pad *o, float tol) {
    float dx = tx - k->x, dy = ty - k->y;
    o->dx = (int8_t)(dx > tol ? 1 : dx < -tol ? -1 : 0);
    o->dy = (int8_t)(dy > tol ? 1 : dy < -tol ? -1 : 0);
}

static bool reachable_x(const Match *m, int i, float x) {
    float x0, x1;
    skid_kid_bounds(m, i, &x0, &x1);
    return x >= x0 - 7 && x <= x1 + 7;
}

static bool in_reach(const Kid *k, float x, float y) { return fabsf(x - k->x) <= 8 && fabsf(y - k->y) <= 6; }
/* the demo player keeps to its own half, even as the kid who may cross */
static bool own_side(const Kid *k, float x) { return k->team == 0 ? x <= SKID_MID + 7 : x >= SKID_MID - 7; }
/* a rival is winding up in line with a bag near the line: don't bend for it */
static bool ambushed(const Match *m, const Kid *k, float x, float y) {
    if (fabsf(x - SKID_MID) > 45) return false;
    for (int j = (1 - k->team) * 2; j < (1 - k->team) * 2 + 2; j++)
        if (m->k[j].state == KS_CHARGE && fabsf(m->k[j].y - y) < 18) return true;
    return false;
}

/* Frames until the most urgent danger reaches kid i (99 = none); *lead is
 * how many frames ahead to jump it (an arcing bag is cleared only at the
 * top of a jump). The CPU reads a wiggly throw as a straight one (players
 * note it doesn't adjust for it); only the demo player allows for the
 * wiggle. */
static int threat_lead(const Match *m, int i, int *which, int *lead, bool wiggle) {
    const Kid *k = &m->k[i];
    int best = 99;
    *which = -1;
    *lead = 6;
    for (int b = 0; b < SKID_MAX_BAGS; b++) {
        const Bag *g = &m->bag[b];
        if ((g->state != BG_SLIDE && g->state != BG_AIR) || g->dead || g->fuse) continue;
        if (g->team >= 0 && g->team == (int)k->team) continue;
        float v2 = g->vx * g->vx + g->vy * g->vy;
        if (v2 < 0.05f) continue;
        float rx = k->x - g->x, ry = k->y - g->y;
        float t = (rx * g->vx + ry * g->vy) / v2;
        if (t < -1 || t > 45) continue;
        float cx = rx - g->vx * t, cy = ry - g->vy * t;
        float slack = g->radius + (g->wavy && wiggle ? 7 : 0);
        if (fabsf(cx) > 7 + slack || fabsf(cy) > 6 + slack) continue;
        if ((int)t < best) {
            best = imax(0, (int)t);
            *which = b;
            *lead = g->state == BG_AIR && g->z > 3 && !g->toss ? 14 : 6;
        }
    }
    for (int w = 0; w < SKID_MAX_WAVES; w++) {
        const Wave *v = &m->wave[w];
        if (!v->live || v->team == (int)k->team || fabsf(k->y - v->y) > v->half + 2) continue;
        float t = (k->x - v->x) / (v->dir * 2.4f);
        if (t >= 0 && t < best) { best = (int)t; *which = 100 + w; *lead = 6; }
    }
    for (int b = 0; b < SKID_MAX_BAGS; b++) {
        const Bag *g = &m->bag[b];
        if (!g->fuse || g->kind == BK_MARBLE || g->team == (int)k->team) continue;
        float dx = k->x - g->x, dy = (k->y - g->y) * 1.4f;
        if (dx * dx + dy * dy > 30 * 30) continue;
        if (g->fuse - 4 < best) { best = imax(0, g->fuse - 4); *which = 200 + b; *lead = 6; }
    }
    return best;
}

/* where a Coach item in the air will come down, and in how many frames */
static bool coach_landing(const Match *m, float *lx, float *ly, int *kind, int *idx) {
    for (int b = 0; b < SKID_MAX_BAGS; b++) {
        const Bag *g = &m->bag[b];
        if (g->state != BG_COACH) continue;
        float t = (g->vz + sqrtf(g->vz * g->vz + 4 * 0.06f * g->z)) / (2 * 0.06f);
        *lx = g->x + g->vx * t;
        *ly = g->y + g->vy * t;
        *kind = 1;
        *idx = b;
        return true;
    }
    for (int n = 0; n < SKID_MAX_ITEMS; n++) {
        const Item *it = &m->it[n];
        if (it->kind != IT_JUICE || !it->flying) continue;
        float t = (it->vz + sqrtf(it->vz * it->vz + 4 * 0.09f * it->z)) / (2 * 0.09f);
        *lx = it->x + it->vx * t;
        *ly = it->y + it->vy * t;
        *kind = 2;
        *idx = n;
        return true;
    }
    return false;
}

/* the best rival to throw at: someone bending down, winding up or down */
static int pick_target(const Match *m, int i, int pile) {
    const Kid *k = &m->k[i];
    int foe = 1 - k->team, best = foe * 2;
    float bs = -1e9f;
    for (int j = foe * 2; j < foe * 2 + 2; j++) {
        const Kid *o = &m->k[j];
        float s = -fabsf(o->y - k->y) * 1.5f - fabsf(o->x - k->x) * 0.3f;
        if (o->state == KS_PICKUP) s += 60;
        if (o->state == KS_CHARGE) s += 50;
        if (o->state == KS_HURT) s += 30;
        if (o->state == KS_DOWN) s += pile == 0 ? -80 : pile == 1 ? 5 : o->timer > 20 ? 25 : 55;
        if (skid_airborne(o)) s -= 40;
        if (s > bs) { bs = s; best = j; }
    }
    return best;
}

/* the nearest of eight ways toward (tx, ty) */
static void aim8(const Kid *k, float tx, float ty, Pad *o) {
    float dx = tx - k->x, dy = ty - k->y;
    float ax = fabsf(dx), ay = fabsf(dy);
    int sx = dx >= 0 ? 1 : -1, sy = dy >= 0 ? 1 : -1;
    if (ay < ax * 0.41f) { o->dx = (int8_t)sx; o->dy = 0; }
    else if (ay > ax * 2.41f) { o->dx = 0; o->dy = (int8_t)sy; }
    else { o->dx = (int8_t)sx; o->dy = (int8_t)sy; }
}

static bool dodge_now(Match *m, int i, const Skill *s, int *th_out) {
    Kid *k = &m->k[i];
    int which, lead, th = threat_lead(m, i, &which, &lead, s == &SKILL[6]);
    *th_out = th;
    if (th >= 99) {
        k->ai_seen = -1;
        return false;
    }
    if (which != k->ai_seen) {
        k->ai_seen = which;
        k->ai_react = s->react + (s == &SKILL[6] ? 0 : rng_range(&m->rng, 0, 4));
        int chance = s->jump;
        if (k->ai_goal == 2) chance -= 25; /* eyes on the Coach's item */
        k->ai_jump_roll = rng_chance(&m->rng, chance);
    }
    if (k->ai_react > 0) k->ai_react--;
    if (k->pas & P_HIGHJUMP) lead = lead > 6 ? lead - 3 : 5;
    return k->ai_react <= 0 && k->ai_jump_roll && th <= lead;
}

static bool want_move(Match *m, int i, Pad *o) {
    Kid *k = &m->k[i];
    int foe = 1 - k->team;
    switch (SKID_KID[k->who].move_kind) {
    case SA_STOMP:
        for (int j = foe * 2; j < foe * 2 + 2; j++)
            if (!skid_airborne(&m->k[j]) && m->k[j].state != KS_DOWN) return true;
        return false;
    case SA_GUST:
        for (int b = 0; b < SKID_MAX_BAGS; b++) {
            const Bag *g = &m->bag[b];
            if (g->state == BG_LOOSE && (k->team == 0 ? g->x < SKID_MID : g->x > SKID_MID)) return true;
            if ((g->state == BG_SLIDE || g->state == BG_AIR) && !g->dead && g->team == foe) return true;
        }
        return false;
    case SA_REEL:
    case SA_POUCH:
        if (k->held >= skid_capacity(k)) return false;
        for (int b = 0; b < SKID_MAX_BAGS; b++) {
            const Bag *g = &m->bag[b];
            if (g->state == BG_GONE || g->state == BG_HELD || g->state == BG_REEL) continue;
            float d2 = (g->x - k->x) * (g->x - k->x) + (g->y - k->y) * (g->y - k->y);
            if (d2 < 150 * 150 && d2 > 30 * 30) return true;
        }
        return false;
    case SA_SPLASH: {
        int t = pick_target(m, i, 2);
        aim8(k, m->k[t].x, m->k[t].y, o);
        return m->k[t].state != KS_DOWN;
    }
    default: /* the sweeper */
        if (m->sweep.live) return false;
        for (int b = 0; b < SKID_MAX_BAGS; b++) {
            const Bag *g = &m->bag[b];
            if (g->state == BG_LOOSE && (foe == 0 ? g->x < SKID_MID : g->x > SKID_MID)) return true;
        }
        for (int j = foe * 2; j < foe * 2 + 2; j++)
            if (m->k[j].state == KS_STAND) return true;
        return false;
    }
}

static void wander(Match *m, int i, const Skill *s, Pad *o) {
    Kid *k = &m->k[i];
    if (--k->ai_t <= 0 || k->ai_goal != 0) {
        k->ai_goal = 0;
        k->ai_t = rng_range(&m->rng, 50, 140);
        float home = k->team == 0 ? SKID_MID - s->approach - 20 : SKID_MID + s->approach + 20;
        k->ai_tx = home + rng_range(&m->rng, -24, 24);
        k->ai_ty = (float)(k->slot == 0 ? rng_range(&m->rng, SKID_TOP + 8, 116) : rng_range(&m->rng, 104, SKID_BOT - 8));
    }
    seek(k, k->ai_tx, k->ai_ty, o, 2);
}


/* Is rival j open to a throw from kid i now, lined up on one of the eight
 * ways? ax, ay get the way. The smart release: someone bent over a
 * bag, winding up or reeling, someone about to stand up, or so close the
 * throw can't be seen coming. */
static bool open_shot(const Match *m, int i, int j, bool smart, int *ax, int *ay) {
    const Kid *k = &m->k[i], *t = &m->k[j];
    float dx = t->x - k->x, dy = t->y - k->y;
    float adx = fabsf(dx), ady = fabsf(dy);
    bool flat = ady <= 5, diag = fabsf(adx - ady) <= 5 && adx > 6;
    if (!flat && !diag) return false;
    *ax = dx >= 0 ? 1 : -1;
    *ay = flat ? 0 : (dy >= 0 ? 1 : -1);
    float dist = flat ? adx : adx * 1.4142f;
    float fly = dist / 3.8f;
    if (t->state == KS_PICKUP || t->state == KS_HURT) return t->timer + 3 >= fly;
    if (t->state == KS_CHARGE) return true;
    if (t->state == KS_DOWN) return smart ? t->timer <= fly + 2 : t->timer < 16;
    if (skid_airborne(t)) return smart && t->vz < -1.0f && fly < 6;
    if (smart) return fly < 9 || (t->ai_goal == 2 && fly < 14);
    return true; /* a plain rival: in line is good enough */
}

/* the wind-up: hold B to the planned length, aim, let go */
static void keep_winding(Match *m, int i, const Skill *s, Pad *o, int th, bool smart) {
    Kid *k = &m->k[i];
    int full = skid_charge_full(k);
    if (k->state != KS_CHARGE) {
        o->dx = o->dy = 0; /* stand still until the wind-up starts */
        o->b = true;
        return;
    }
    int tj = pick_target(m, i, s->pile);
    aim8(k, m->k[tj].x, m->k[tj].y, o);
    bool ready = k->charge >= k->ai_charge_to;
    if (ready && s->ambush && k->charge >= full) {
        /* a full wind-up held: let go when a rival is open */
        int ax = 0, ay = 0, foe = 1 - k->team;
        bool open = false;
        for (int j = foe * 2; j < foe * 2 + 2 && !open; j++)
            if (open_shot(m, i, j, smart, &ax, &ay)) {
                open = true;
                o->dx = (int8_t)ax;
                o->dy = (int8_t)ay;
            }
        k->ai_wait++;
        int patience = smart ? SKID_FORCED - 20 : 100;
        if (!open && k->hold_t < SKID_FORCED - 20 && k->ai_wait < patience) ready = false;
    }
    if (th <= 3 && k->charge >= SKID_TOSS && !smart) ready = true; /* throw rather than be hit */
    if (!ready) {
        o->b = true;
        return;
    }
    o->b = false;
    o->br = true;
}

static void rival(Match *m, int i, const Skill *s, Pad *o, bool smart) {
    Kid *k = &m->k[i];
    int th;
    bool dodge = dodge_now(m, i, s, &th);
    if (k->state == KS_CHARGE) {
        keep_winding(m, i, s, o, th, smart);
        return;
    }
    if (k->state != KS_STAND && k->state != KS_THROW) return;
    bool air = skid_airborne(k);
    int cost = m->rules.cost * 2;
    if (dodge && !air) {
        o->ap = o->a = true;
        o->b = k->armed; /* a wind-up about to start carries on after the jump */
        return;
    }
    if (k->armed && k->held > 0) {
        keep_winding(m, i, s, o, th, smart);
        return;
    }
    if (air) {
        if (!k->air_move && k->stars >= cost && k->vz < 0.8f) {
            Pad aim = *o;
            if (k->ai_goal == 4 || rng_chance(&m->rng, s->special / 3)) {
                if (want_move(m, i, &aim)) {
                    *o = aim;
                    o->ap = o->a = true;
                    k->ai_goal = 0;
                    return;
                }
            }
        }
        if (k->ai_goal == 4) return;
    }
    /* holding: line up with a rival and wind up */
    if (k->held > 0) {
        int tj = pick_target(m, i, s->pile);
        const Kid *t = &m->k[tj];
        float want_x = k->team == 0 ? SKID_MID - s->approach : SKID_MID + s->approach;
        if (k->pas & P_CROSS) want_x = k->team == 0 ? SKID_MID - 20 : SKID_MID + 20;
        float want_y = t->y;
        if (smart) {
            /* wait where the rivals will come: by the Coach's item, or a
             * loose bag near the line on their side */
            float lx, ly;
            int kind, idx;
            if (coach_landing(m, &lx, &ly, &kind, &idx) && fabsf(lx - SKID_MID) < 30) want_y = ly;
            want_x = k->team == 0 ? SKID_MID - 12 : SKID_MID + 12;
        }
        float ddx = fabsf(t->x - k->x), ddy = fabsf(t->y - k->y);
        bool lined = ddy <= s->align || (fabsf(ddx - ddy) <= s->align * 1.5f && ddx < 150);
        bool late = k->hold_t > SKID_FORCED - 80;
        bool there = fabsf(k->x - want_x) < 10;
        if (!air && ((smart && there) || (!smart && lined) || late) && k->hold_t > s->hesitate) {
            /* wind-up lengths past the light toss, as a share of the kid's
             * own full wind-up, so the quick kids throw as hard sooner */
            int full = skid_charge_full(k), span = full - SKID_TOSS;
            if (smart) k->ai_charge_to = full + 1;
            else if (t->state == KS_DOWN && t->timer > 30) k->ai_charge_to = SKID_TOSS + rng_range(&m->rng, 0, span / 4);
            else if (rng_chance(&m->rng, s->full) || (k->stars >= cost && rng_chance(&m->rng, s->special)))
                k->ai_charge_to = full + 1;
            else k->ai_charge_to = SKID_TOSS + rng_range(&m->rng, span * 3 / 10, span - 2);
            if (late) k->ai_charge_to = imin(k->ai_charge_to, SKID_TOSS + span / 3);
            k->ai_wait = 0;
            o->bp = o->b = true;
            return;
        }
        seek(k, want_x, want_y, o, 1.5f);
        return;
    }
    /* empty hands: the Coach's items first (every time, ambush or not) */
    float lx, ly;
    int kind, idx;
    if (coach_landing(m, &lx, &ly, &kind, &idx) && reachable_x(m, i, lx)) {
        const Kid *mate = &m->k[i ^ 1];
        float mine = fabsf(lx - k->x) + fabsf(ly - k->y);
        float theirs = fabsf(lx - mate->x) + fabsf(ly - mate->y);
        if (mine <= theirs + 6 || mate->held > 0 || mate->state == KS_DOWN) {
            k->ai_goal = 2;
            seek(k, lx, ly, o, 1);
            return;
        }
    }
    /* then the nearest bag (or juice box) within its reach */
    int best = -1, best_kind = 0;
    float bd = 1e9f;
    for (int b = 0; b < SKID_MAX_BAGS; b++) {
        const Bag *g = &m->bag[b];
        if (g->state != BG_LOOSE || !reachable_x(m, i, g->x)) continue;
        const Kid *mate = &m->k[i ^ 1];
        if (mate->ai_goal == 1 && mate->ai_target == b && mate->state != KS_DOWN) continue;
        float d = fabsf(g->x - k->x) + fabsf(g->y - k->y);
        if (d < bd) { bd = d; best = b; best_kind = 1; }
    }
    for (int n = 0; n < SKID_MAX_ITEMS; n++) {
        const Item *it = &m->it[n];
        if (it->kind != IT_JUICE || it->flying || !reachable_x(m, i, it->x)) continue;
        float d = fabsf(it->x - k->x) + fabsf(it->y - k->y) + 10;
        if (d < bd) { bd = d; best = n; best_kind = 2; }
    }
    if (best >= 0) {
        float bx = best_kind == 1 ? m->bag[best].x : m->it[best].x;
        float by = best_kind == 1 ? m->bag[best].y : m->it[best].y;
        k->ai_goal = 1;
        k->ai_target = best_kind == 1 ? best : -1;
        if (in_reach(k, bx, by) && !air) {
            o->bp = o->b = true;
            return;
        }
        seek(k, bx, by, o, 1);
        return;
    }
    /* nothing to fetch: a special move on purpose now and then */
    if (!air && k->stars >= cost && rng_chance(&m->rng, 2) && rng_chance(&m->rng, s->special)) {
        Pad aim = *o;
        if (want_move(m, i, &aim)) {
            k->ai_goal = 4;
            o->ap = o->a = true;
            return;
        }
    }
    wander(m, i, s, o);
}

/* A spot the partner won't walk to for a bag: in the lane of a throw coming
 * over, or by a popper of the other team about to go off. */
static bool spot_in_danger(const Match *m, int team, float x, float y) {
    for (int b = 0; b < SKID_MAX_BAGS; b++) {
        const Bag *g = &m->bag[b];
        if (g->state == BG_GONE || g->team == team) continue;
        if (g->fuse > 0) {
            float dx = x - g->x, dy = (y - g->y) * 1.4f;
            if (g->kind != BK_MARBLE && dx * dx + dy * dy < 36 * 36) return true;
            continue;
        }
        if ((g->state != BG_SLIDE && g->state != BG_AIR) || g->dead) continue;
        float v2 = g->vx * g->vx + g->vy * g->vy;
        if (v2 < 0.05f) continue;
        float rx = x - g->x, ry = y - g->y;
        float t = (rx * g->vx + ry * g->vy) / v2;
        if (t < 0 || t > 60) continue;
        if (fabsf(rx - g->vx * t) < 14 && fabsf(ry - g->vy * t) < 12) return true;
    }
    return false;
}

/* Your partner, by default: it only fetches bags. A bag in its hands stays
 * there (in its corner, away from you) until you swap to it, or the forced
 * throw comes; it never passes one over by itself. It no longer walks into
 * a throw coming at it or to a bag in a throw's way, but the jump is still
 * yours: it jumps when you do. */
static void mate_bags(Match *m, int i, Pad *o) {
    Kid *k = &m->k[i];
    int team = k->team;
    const Kid *h = &m->k[m->ctrl[team]];
    int th, which, lead;
    if (m->rules.mate_jump) {
        if (dodge_now(m, i, &SKILL[0], &th) && !skid_airborne(k) && k->state == KS_STAND) {
            o->ap = o->a = true;
            return;
        }
    } else {
        th = threat_lead(m, i, &which, &lead, false);
    }
    if (k->state != KS_STAND && k->state != KS_THROW) return;
    if (th < 20) return; /* something is coming: it stops where it is */
    float corner_x = team == 0 ? SKID_LEFT + 34 : SKID_RIGHT - 34;
    float corner_y = h->y < (SKID_TOP + SKID_BOT) / 2 ? SKID_BOT - 18 : SKID_TOP + 18;
    if (k->held == 0) {
        int best = -1;
        float bd = 1e9f;
        for (int b = 0; b < SKID_MAX_BAGS; b++) {
            const Bag *g = &m->bag[b];
            if (g->state != BG_LOOSE || !reachable_x(m, i, g->x)) continue;
            if (fabsf(g->x - h->x) < 14 && fabsf(g->y - h->y) < 12) continue; /* yours */
            if (spot_in_danger(m, team, g->x, g->y)) continue;
            float d = fabsf(g->x - k->x) + fabsf(g->y - k->y);
            if (d < bd) { bd = d; best = b; }
        }
        if (best >= 0) {
            const Bag *g = &m->bag[best];
            if (in_reach(k, g->x, g->y) && !skid_airborne(k)) {
                o->bp = o->b = true;
                return;
            }
            seek(k, g->x, g->y, o, 1);
            return;
        }
    }
    seek(k, corner_x, corner_y, o, 2);
}

/* ---- puddles ---------------------------------------------------------------
 * The CPU steers round a puddle lying on the floor (a kid who can't slip
 * doesn't bother); a water balloon still in the air isn't a puddle yet. */

/* the puddle at (x, y), or nearly (a few pixels to spare), or -1 */
static int puddle_at(const Match *m, float x, float y, float spare) {
    for (int n = 0; n < SKID_MAX_ITEMS; n++) {
        const Item *it = &m->it[n];
        if (it->kind != IT_PUDDLE || it->flying) continue;
        float dx = x - it->x, dy = y - it->y, r = it->r + spare;
        if (dx * dx + dy * dy * 2.5f < r * r) return n;
    }
    return -1;
}

static void steer_dry(Match *m, int i, Pad *o) {
    static const int8_t DX[8] = {1, 1, 0, -1, -1, -1, 0, 1}, DY[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    Kid *k = &m->k[i];
    if (puddle_at(m, k->x, k->y, 16) < 0) k->ai_detour = 0; /* clear of puddles */
    if ((k->pas & P_NOSLIP) || (!o->dx && !o->dy) || k->state == KS_CHARGE || skid_airborne(k)) return;
    int d0 = 0;
    for (int d = 0; d < 8; d++)
        if (DX[d] == o->dx && DY[d] == o->dy) d0 = d;
    float x0, x1;
    skid_kid_bounds(m, i, &x0, &x1);
    /* a step ahead each way: into a puddle? open (not a wall or the line)? */
    int wet[8];
    bool open[8];
    for (int d = 0; d < 8; d++) {
        float l = DX[d] && DY[d] ? 0.7071f : 1.0f;
        float nx = fclamp(k->x + DX[d] * l * 8, x0, x1), ny = fclamp(k->y + DY[d] * l * 8, SKID_TOP + 2, SKID_BOT - 2);
        wet[d] = puddle_at(m, nx, ny, 3);
        open[d] = fabsf(nx - k->x) + fabsf(ny - k->y) >= 4;
    }
    int p = wet[d0];
    if (p < 0) return; /* dry ahead */
    if (!k->ai_detour) {
        /* go round on the side the kid is already on (turning +1 is
         * clockwise on the screen), and keep to it */
        float cross = (float)DX[d0] * (k->y - m->it[p].y) - (float)DY[d0] * (k->x - m->it[p].x);
        k->ai_detour = cross > 0 ? 1 : cross < 0 ? -1 : (k->slot ? 1 : -1);
    }
    int s = k->ai_detour;
    for (int side = 0; side < 2; side++, s = -s)
        for (int turn = 1; turn <= 3; turn++) {
            int d = (d0 + s * turn + 8) % 8;
            if (wet[d] >= 0 || !open[d]) continue;
            o->dx = DX[d];
            o->dy = DY[d];
            return;
        }
    /* no dry way round: straight on, and maybe a slip */
}


void skid_ai_pad(Match *m, int i, Pad *o) {
    memset(o, 0, sizeof *o);
    Kid *k = &m->k[i];
    if (m->state != MS_PLAY) return;
    if (k->state == KS_DOWN || k->state == KS_HURT || k->state == KS_PICKUP) return;
    int team = k->team;
    bool mate = m->player[team] != CTRL_AI;
    if (mate) {
        if (m->rules.mate_ai == 0) mate_bags(m, i, o);
        else rival(m, i, &SKILL[3], o, false);
    } else {
        rival(m, i, &SKILL[iclamp(m->level[team], 0, 6)], o, false);
    }
    steer_dry(m, i, o);
}

/* ---- the demo player ------------------------------------------------------
 * It plays the kid you drive the way a sharp player does: it jumps every bag
 * it sees in time (a jump calls a wind-up off, in the same press), camps by
 * the centre line where the Coach's items land and the rivals come running,
 * and throws short: a light toss into a rival at point blank knocks it
 * flat, and a quick throw finds a rival who is bent over a bag, winding up
 * or down. The partner never passes by itself, so for the partner's bag it
 * swaps over and straight back (the bag is tossed across as it swaps). */

static int bot_back = -1, bot_back_t; /* the swap-and-back: the kid to come back to */

/* a wind-up past the light toss: a share of the kid's own full wind-up */
static int bot_charge(const Kid *k, float share) {
    int full = skid_charge_full(k);
    return SKID_TOSS + (int)(share * (float)(full - SKID_TOSS) + 0.5f);
}

static bool bot_near_thing(const Match *m, const Kid *k) {
    for (int b = 0; b < SKID_MAX_BAGS; b++)
        if (m->bag[b].state == BG_LOOSE && in_reach(k, m->bag[b].x, m->bag[b].y)) return true;
    for (int n = 0; n < SKID_MAX_ITEMS; n++)
        if (m->it[n].kind == IT_JUICE && !m->it[n].flying && in_reach(k, m->it[n].x, m->it[n].y)) return true;
    return false;
}

static bool quick_line(const Kid *k, const Kid *t, float max_d, int *ax, int *ay, float *dist) {
    float dx = t->x - k->x, dy = t->y - k->y;
    float adx = fabsf(dx), ady = fabsf(dy);
    if (ady <= 4) { *ay = 0; *dist = adx; }
    else if (fabsf(adx - ady) <= 4 && adx > 6) { *ay = dy > 0 ? 1 : -1; *dist = adx * 1.414f; }
    else return false;
    *ax = dx > 0 ? 1 : -1;
    return *dist <= max_d && *dist > 3;
}

static void bot_play(Match *m, int i, Pad *o) {
    const Skill *s = &SKILL[6];
    Kid *k = &m->k[i];
    int th;
    bool dodge = dodge_now(m, i, s, &th);
    bool air = skid_airborne(k);
    int foe = 1 - k->team, dir = dir_of(k->team), cost = m->rules.cost * 2;
    if (bot_back == (i ^ 1)) {
        /* on the partner for its bag: a tap tosses it across and swaps back */
        if (k->held == 0) bot_back = -1;
        else if (k->state == KS_STAND || k->state == KS_THROW) {
            if (!k->armed) {
                o->bp = o->b = true;
            } else {
                o->br = true;
                bot_back = -1;
            }
            return;
        }
    }
    if (k->state == KS_CHARGE) {
        if (dodge) {
            o->ap = o->a = true; /* the jump calls it off and jumps, in one */
            return;
        }
        const Kid *t = &m->k[iclamp(k->ai_target, foe * 2, foe * 2 + 1)];
        aim8(k, t->x, t->y, o);
        if (k->charge >= k->ai_charge_to || k->hold_t > SKID_FORCED - 8) {
            o->br = true;
            return;
        }
        o->b = true;
        return;
    }
    if (k->state != KS_STAND && k->state != KS_THROW) return;
    if (dodge && !air) {
        o->ap = o->a = true;
        o->b = k->armed;
        return;
    }
    if (k->armed) {
        o->b = true;
        return;
    }
    if (air) {
        if (!k->air_move && k->stars >= cost && k->vz < 0.8f) {
            Pad aim = *o;
            if (want_move(m, i, &aim)) {
                *o = aim;
                o->ap = o->a = true;
            }
        }
        return;
    }
    if (k->held > 0) {
        int ax, ay;
        float d;
        for (int j = foe * 2; j < foe * 2 + 2; j++) {
            const Kid *t = &m->k[j];
            if (skid_airborne(t)) continue;
            int plan = -1;
            if (quick_line(k, t, 22, &ax, &ay, &d)) plan = SKID_TAP + 1; /* the light toss */
            else if (t->state == KS_DOWN && t->timer > 40 && quick_line(k, t, 140, &ax, &ay, &d))
                plan = k->stars >= cost && t->timer > 70 ? skid_charge_full(k) + 1 : bot_charge(k, 0.32f);
            else if ((t->state == KS_PICKUP || t->state == KS_HURT) && quick_line(k, t, 60, &ax, &ay, &d)) plan = bot_charge(k, 0.24f);
            else if (t->state == KS_CHARGE && quick_line(k, t, 150, &ax, &ay, &d)) plan = bot_charge(k, 0.56f);
            else if (k->hold_t > SKID_FORCED - 50 && quick_line(k, t, 300, &ax, &ay, &d)) plan = bot_charge(k, 0.6f);
            if (plan < 0) continue;
            k->ai_target = j;
            k->ai_charge_to = plan;
            o->bp = o->b = true;
            return;
        }
        /* a rival with a bag in hand: keep well back and out of its line */
        int armed = -1;
        for (int j = foe * 2; j < foe * 2 + 2; j++)
            if (m->k[j].held > 0 && m->k[j].state != KS_DOWN) armed = j;
        if (armed >= 0) {
            const Kid *t = &m->k[armed];
            float back = (t->pas & (P_LOB | P_QUICKCHARGE)) ? 105 : 70;
            float wy = t->y < (SKID_TOP + SKID_BOT) / 2.0f ? t->y + 26 : t->y - 26;
            seek(k, t->x - dir * back, wy, o, 2);
            return;
        }
        /* else wait by the line where the rivals will come */
        float wy = m->k[foe * 2].y, lx, ly;
        int kind, idx;
        float best = 1e9f;
        for (int j = foe * 2; j < foe * 2 + 2; j++) {
            float dd = fabsf(m->k[j].y - k->y) + fabsf(m->k[j].x - k->x) * 0.2f;
            if (dd < best) { best = dd; wy = m->k[j].y; }
        }
        if (coach_landing(m, &lx, &ly, &kind, &idx) && fabsf(lx - SKID_MID) < 40) wy = ly;
        seek(k, SKID_MID - dir * 10, wy, o, 1);
        return;
    }
    /* empty hands: the Coach's item coming down on this side */
    float lx, ly;
    int kind, idx;
    if (coach_landing(m, &lx, &ly, &kind, &idx) && own_side(k, lx)) {
        seek(k, lx - dir * 3, ly, o, 1);
        return;
    }
    /* the partner holding a bag? swap over to it and straight back (the
     * pass follows the kid, so it needn't wait for it) */
    const Kid *mate = &m->k[i ^ 1];
    if (mate->held > 0 && (mate->state == KS_STAND || mate->state == KS_THROW) && !bot_near_thing(m, k)) {
        bot_back = i;
        bot_back_t = 0;
        o->bp = o->b = true;
        return;
    }
    /* else the nearest bag or juice box */
    int best = -1, best_kind = 0;
    float bd = 1e9f;
    for (int b = 0; b < SKID_MAX_BAGS; b++) {
        const Bag *g = &m->bag[b];
        if (g->state != BG_LOOSE || !own_side(k, g->x) || ambushed(m, k, g->x, g->y)) continue;
        float dd = fabsf(g->x - k->x) + fabsf(g->y - k->y);
        if (dd < bd) { bd = dd; best = b; best_kind = 1; }
    }
    for (int n = 0; n < SKID_MAX_ITEMS; n++) {
        const Item *it = &m->it[n];
        if (it->kind != IT_JUICE || it->flying || !own_side(k, it->x)) continue;
        float dd = fabsf(it->x - k->x) + fabsf(it->y - k->y) + 10;
        if (dd < bd) { bd = dd; best = n; best_kind = 2; }
    }
    if (best >= 0) {
        float bx = best_kind == 1 ? m->bag[best].x : m->it[best].x;
        float by = best_kind == 1 ? m->bag[best].y : m->it[best].y;
        if (in_reach(k, bx, by)) {
            o->bp = o->b = true;
            return;
        }
        seek(k, bx, by, o, 1);
        return;
    }
    /* nothing to do: stand back from the line, and jump for a special now and then */
    if (k->stars >= cost && rng_chance(&m->rng, 3)) {
        Pad aim = *o;
        if (want_move(m, i, &aim)) {
            o->ap = o->a = true;
            return;
        }
    }
    seek(k, SKID_MID - dir * 60, (SKID_TOP + SKID_BOT) / 2.0f, o, 3);
}

void skid_bot_pad(Match *m, int i, Pad *o) {
    memset(o, 0, sizeof *o);
    Kid *k = &m->k[i];
    if (bot_back >= 0 && ++bot_back_t > 12) bot_back = -1; /* the swap-and-back fell through */
    if (m->state != MS_PLAY) {
        bot_back = -1;
        return;
    }
    if (k->state == KS_DOWN || k->state == KS_HURT || k->state == KS_PICKUP) return;
    bot_play(m, i, o);
    /* a press of B with nothing within reach would swap kids: only on purpose */
    if (o->bp && k->held == 0 && bot_back != i && !bot_near_thing(m, k)) o->bp = o->b = false;
    steer_dry(m, i, o);
}
