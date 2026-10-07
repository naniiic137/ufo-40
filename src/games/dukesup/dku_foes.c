/* DUKES UP - the ghouls, the bosses, the dogs, the passers-by and the
 * saucer: how each one thinks. Integer arithmetic only. */
#include "dku.h"

#define GRAV 4

static int rnd(int lo, int hi) { return rng_range(&dku_g.rng, lo, hi); }

static bool physics(Actor *a) {
    bool landed = false;
    if (a->z > 0 || a->vz > 0) {
        a->z += a->vz;
        a->vz -= GRAV;
        if (a->z <= 0) { a->z = 0; a->vz = 0; landed = true; }
    }
    a->x += a->vx;
    a->y += a->vy;
    if (a->kind != AK_SAUCER && a->kind != AK_UNDERTOW && a->state != AS_RISE) {
        int lo = dku_fx(dku_floor_lo()), hi = dku_fx(dku_floor_hi());
        if (a->y < lo) a->y = lo;
        if (a->y > hi) a->y = hi;
    }
    return landed;
}

/* keep a ghoul that has come on inside the screen */
static void clamp_in(Actor *a) {
    int l = dku_fx(dku_view_left() + 4), r = dku_fx(dku_view_right() - 4);
    if (a->x < l) a->x = l;
    if (a->x > r) a->x = r;
}

/* the nearest of the fighters and their dogs */
static int target_of(const Actor *a) {
    int best = -1, bd = 1 << 30;
    for (int j = 0; j < DKU_MAX_ACTORS; j++) {
        const Actor *t = &dku_g.a[j];
        if (!t->alive || t->team != 0 || j == (int)(a - dku_g.a)) continue;
        if (t->state == AS_DEAD || t->state == AS_FALL || t->state == AS_GONE || t->state == AS_LEASHED) continue;
        int d = iabs(dku_px(t->x - a->x)) + iabs(dku_px(t->y - a->y)) * 2;
        if (t->kind == AK_DOG) d += 40; /* fighters first */
        if (d < bd) { bd = d; best = j; }
    }
    return best;
}

static int nearest_ghoul(const Actor *a) {
    int best = -1, bd = 1 << 30;
    for (int j = 2; j < DKU_MAX_ACTORS; j++) {
        const Actor *t = &dku_g.a[j];
        if (!t->alive || t->team != 1 || t->state == AS_DEAD || t->state == AS_FALL || t->kind == AK_UNDERTOW) continue;
        if (t->state == AS_ENTER || t->state == AS_GRABBED || t->state == AS_THROWN) continue; /* not one a fighter is holding */
        int d = iabs(dku_px(t->x - a->x)) + iabs(dku_px(t->y - a->y)) * 2;
        if (d < bd) { bd = d; best = j; }
    }
    return best;
}

/* a step that never walks into a pit */
static void step_to(Actor *a, int tx, int ty, int sx, int sy) {
    int dx = tx - a->x, dy = ty - a->y;
    int mx = dx > sx ? sx : dx < -sx ? -sx : dx;
    int my = dy > sy ? sy : dy < -sy ? -sy : dy;
    int nx = a->x + mx, ny = a->y + my;
    int lo = dku_fx(dku_floor_lo()), hi = dku_fx(dku_floor_hi());
    if (ny < lo) ny = lo;
    if (ny > hi) ny = hi;
    if (!dku_in_pit(dku_px(nx), dku_px(ny))) { a->x = nx; a->y = ny; }
    else if (!dku_in_pit(dku_px(nx), dku_px(a->y))) a->x = nx;
    else if (!dku_in_pit(dku_px(a->x), dku_px(ny))) a->y = ny;
    a->vx = a->vy = 0;
    if (mx || my) { a->step++; a->anim = dku_g.frame_t; }
}

static void face_to(Actor *a, const Actor *t) {
    int d = t->x - a->x;
    if (d > dku_fx(2)) a->face = 1;
    else if (d < -dku_fx(2)) a->face = -1;
}

static void windup(Actor *a, int atk, int frames) {
    dku_set_state(a, AS_WINDUP);
    a->atk = atk;
    a->timer2 = frames;
}

/* how many awake ghouls keep to each side of t */
static int side_count(int ti, int side) {
    int n = 0;
    for (int j = 2; j < DKU_MAX_ACTORS; j++) {
        const Actor *o = &dku_g.a[j];
        if (o->alive && o->team == 1 && o->state != AS_DORMANT && o->state != AS_DEAD && o->side == side && o->wake == ti + 1) n++;
    }
    return n;
}

static void pick_side(Actor *a, int ti) {
    const Actor *t = &dku_g.a[ti];
    int here = a->x >= t->x ? 1 : -1;
    int l = side_count(ti, -1), r = side_count(ti, 1);
    a->side = here;
    /* ghouls like to come at you from both sides */
    if (here == 1 && r > l + 1) a->side = -1;
    if (here == -1 && l > r + 1) a->side = 1;
    a->wake = ti + 1;
}

static int speed_of(const Actor *a, const Actor *t) {
    int s = DKU_KINDS[a->kind].speed;
    if (a->boss) s += 2;
    /* they scurry while you're in the air */
    if (t && t->kind == AK_FIGHTER && t->state == AS_AIR) s *= 2;
    return s;
}

/* walk to the fighting spot on our side of the target, then strike */
static void melee_ai(Actor *a, int i, int ti, int gap) {
    Actor *t = &dku_g.a[ti];
    if (a->wake != ti + 1) pick_side(a, ti);
    int sp = speed_of(a, t);
    int tx = t->x + a->side * dku_fx(gap);
    int ty = t->y;
    int dx = dku_px(t->x - a->x);
    /* on the wrong side and close: go round, not through */
    if ((dx > 0 ? -1 : 1) != a->side && iabs(dx) < 34) {
        int around = t->y + (a->y <= t->y ? -dku_fx(22) : dku_fx(22));
        step_to(a, tx, around, sp, sp);
    } else {
        step_to(a, tx, ty, sp, sp * 2 / 3 + 1);
    }
    face_to(a, t);
    /* back off a flying kick */
    if (t->kind == AK_FIGHTER && t->atk == AT_FLYKICK && iabs(dx) < 48 && a->kind == AK_SHAMBLER) {
        step_to(a, a->x - (dx > 0 ? 1 : -1) * dku_fx(10), a->y, sp, 0);
    }
    if (a->cool == 0 && dku_in_reach(a, t, DKU_KINDS[a->kind].reach - 2, 0, 20) && t->z < dku_fx(18)) {
        windup(a, AT_E_HIT, DKU_KINDS[a->kind].windup);
        (void)i;
    }
}

/* lob something so it lands on (tx, ty) after T frames */
static int lob(int kind, Actor *a, int i, const Actor *t, int T, int dmg) {
    int z0 = dku_fx(22);
    int vz = (GRAV * T * (T - 1) / 2 - z0) / T;
    int vx = (t->x - a->x) / T, vy = (t->y - a->y) / T;
    return dku_add_shot(kind, 1, a->x + a->face * dku_fx(6), a->y, z0, vx, vy, vz, dmg, i);
}

/* ---- each kind --------------------------------------------------------------- */

static void ai_torch(Actor *a, int i, int ti) {
    Actor *t = &dku_g.a[ti];
    int dx = dku_px(t->x - a->x), dy = dku_px(t->y - a->y);
    face_to(a, t);
    int want = 84;
    int sp = speed_of(a, t);
    if (iabs(dx) < 40) {
        /* it has no fists: up close it only backs away */
        step_to(a, a->x - (dx >= 0 ? 1 : -1) * dku_fx(8), a->y + (dy > 0 ? -dku_fx(4) : dku_fx(4)), sp, sp / 2 + 1);
        return;
    }
    int tx = t->x - (dx >= 0 ? 1 : -1) * dku_fx(want);
    step_to(a, tx, t->y, sp, sp / 2 + 1);
    if (a->cool == 0 && iabs(dx) <= 150 && iabs(dy) < 30) windup(a, AT_NONE, 30);
}

static void ai_crow(Actor *a, int i, int ti) {
    Actor *t = &dku_g.a[ti];
    int dx = dku_px(t->x - a->x), dy = dku_px(t->y - a->y);
    if (iabs(dx) < 30) { melee_ai(a, i, ti, 16); return; }
    face_to(a, t);
    int sp = speed_of(a, t);
    int tx = t->x - (dx >= 0 ? 1 : -1) * dku_fx(110);
    step_to(a, tx, t->y, sp, sp);
    if (a->cool == 0 && iabs(dy) <= 5 && iabs(dx) >= 40 && iabs(dx) < 220) windup(a, AT_NONE, 20);
}

static void ai_rammer(Actor *a, int i, int ti) {
    Actor *t = &dku_g.a[ti];
    int dx = dku_px(t->x - a->x), dy = dku_px(t->y - a->y);
    face_to(a, t);
    int sp = speed_of(a, t);
    if (iabs(dx) < 34) { melee_ai(a, i, ti, 18); return; }
    step_to(a, a->x + (iabs(dx) > 120 ? (dx > 0 ? 1 : -1) * dku_fx(8) : 0), t->y, sp, sp);
    if (a->cool == 0 && iabs(dy) < 8 && iabs(dx) >= 50) {
        windup(a, AT_E_RUSH, DKU_KINDS[AK_RAMMER].windup + (a->boss ? -6 : 0));
    }
}

static void ai_howler(Actor *a, int i, int ti) {
    Actor *t = &dku_g.a[ti];
    int dx = dku_px(t->x - a->x), dy = dku_px(t->y - a->y);
    face_to(a, t);
    int sp = speed_of(a, t);
    if (iabs(dx) < 28) { melee_ai(a, i, ti, 16); return; }
    /* it keeps its distance, then springs */
    int keep = 64;
    int tx = t->x - (dx >= 0 ? 1 : -1) * dku_fx(keep);
    step_to(a, tx, t->y, sp, sp * 2 / 3);
    if (a->cool == 0 && iabs(dy) < 8 && iabs(dx) >= 40 && iabs(dx) <= 90) windup(a, AT_E_POUNCE, DKU_KINDS[AK_HOWLER].windup);
}

static void ai_tusker(Actor *a, int i, int ti) {
    Actor *t = &dku_g.a[ti];
    int dx = dku_px(t->x - a->x), dy = dku_px(t->y - a->y);
    face_to(a, t);
    int sp = speed_of(a, t);
    if (iabs(dx) > 40 || iabs(dy) > 12) {
        step_to(a, t->x - (dx >= 0 ? 1 : -1) * dku_fx(24), t->y, sp, sp);
        if (a->cool == 0 && iabs(dx) > 60 && iabs(dx) < 140 && rnd(0, 99) < 2) {
            /* the body slam: a leap at where you stand */
            dku_set_state(a, AS_SLAM);
            a->tx = t->x;
            a->ty = t->y;
            a->vz = 56;
            a->z = 1;
            a->vx = (t->x - a->x) / 28;
            a->vy = (t->y - a->y) / 28;
            dku_sfx("dku_leap");
        }
        return;
    }
    /* close and idle: it guards */
    dku_set_state(a, AS_GUARD);
    a->timer2 = rnd(30, 70);
    a->guard_hits = 0;
}

static void ai_sludger(Actor *a, int i, int ti) {
    Actor *t = &dku_g.a[ti];
    int dx = dku_px(t->x - a->x), dy = dku_px(t->y - a->y);
    face_to(a, t);
    if (iabs(dx) < 30) { melee_ai(a, i, ti, 18); return; }
    step_to(a, t->x, t->y, speed_of(a, t), speed_of(a, t));
    if (a->cool == 0 && iabs(dx) < 110 && iabs(dy) < 16) windup(a, AT_NONE, DKU_KINDS[AK_SLUDGER].windup);
}

static void blink_to(Actor *a, const Actor *t) {
    /* behind you, unless your back is to the edge */
    int bx = dku_px(t->x) - t->face * 30;
    if (bx < dku_view_left() + 10 || bx > dku_view_right() - 10) bx = dku_px(t->x) + t->face * 34;
    bx = iclamp(bx, dku_view_left() + 10, dku_view_right() - 10);
    a->x = dku_fx(bx);
    a->y = t->y;
    a->face = bx < dku_px(t->x) ? 1 : -1;
}

static void ai_visitor(Actor *a, int i, int ti) {
    Actor *t = &dku_g.a[ti];
    int dx = dku_px(t->x - a->x), dy = dku_px(t->y - a->y);
    face_to(a, t);
    int sp = speed_of(a, t);
    if (a->cool == 0 && rnd(0, 999) < (a->boss ? 6 : 4)) {
        dku_set_state(a, AS_BLINK);
        a->blinks = 1;
        dku_sfx("dku_blink");
        return;
    }
    if (iabs(dx) < 34) { melee_ai(a, i, ti, 18); return; }
    int tx = t->x - (dx >= 0 ? 1 : -1) * dku_fx(70);
    step_to(a, tx, t->y, sp, sp);
    if (a->cool == 0 && iabs(dy) <= 6 && iabs(dx) >= 40) windup(a, AT_NONE, 26);
}

static void ai_bulwark(Actor *a, int i, int ti) {
    Actor *t = &dku_g.a[ti];
    face_to(a, t);
    if (a->timer2 > 0) { a->timer2--; melee_ai(a, i, ti, 18); return; }
    /* it walks in behind its guard */
    int dx = dku_px(t->x - a->x);
    int sp = speed_of(a, t);
    step_to(a, t->x - (dx >= 0 ? 1 : -1) * dku_fx(18), t->y, sp, sp);
    if (a->cool == 0 && dku_in_reach(a, t, DKU_KINDS[a->kind].reach - 2, 0, 20) && t->z < dku_fx(18)) {
        windup(a, AT_E_HIT, DKU_KINDS[AK_BULWARK].windup);
        return;
    }
    a->state = AS_GUARD;
    a->st = 0;
}

static void ai_feeler(Actor *a, int i, int ti) {
    Actor *t = &dku_g.a[ti];
    face_to(a, t);
    if (a->cool == 0 && dku_in_reach(a, t, DKU_KINDS[AK_FEELER].reach, 0, 20)) windup(a, AT_E_HIT, DKU_KINDS[AK_FEELER].windup);
    (void)i;
}

static void ai_dog(Actor *a, int i) {
    int ti = nearest_ghoul(a);
    if (ti < 0) {
        /* trot along by the first fighter */
        const Actor *f = (dku_g.a[0].alive && dku_g.a[0].state != AS_DEAD) ? &dku_g.a[0] : &dku_g.a[1];
        step_to(a, f->x - f->face * dku_fx(24), f->y + dku_fx(6), 16, 12);
        face_to(a, f);
        return;
    }
    Actor *t = &dku_g.a[ti];
    int side = a->x > t->x ? 1 : -1;
    step_to(a, t->x + side * dku_fx(14), t->y, DKU_KINDS[AK_DOG].speed, 14);
    face_to(a, t);
    if (a->cool == 0 && dku_in_reach(a, t, 18, 0, 20)) {
        dku_start_attack(a, AT_E_BITE);
        a->cool = 24;
        dku_sfx("dku_bark");
    }
    (void)i;
}

static void ai_undertow(Actor *a, int i) {
    /* out in the water: a feeler comes down where you stand */
    if (a->cool > 0) return;
    int ti = target_of(a);
    if (ti < 0) return;
    Actor *t = &dku_g.a[ti];
    dku_add_hazard(HZ_LAMP, dku_px(t->x), dku_px(t->y), 30, 10, 60, 2);
    a->cool = 150 + rnd(0, 60);
    (void)i;
}

static void ai_grist(Actor *a, int i, int ti) {
    Actor *t = &dku_g.a[ti];
    int dx = dku_px(t->x - a->x), dy = dku_px(t->y - a->y);
    face_to(a, t);
    bool mutant = a->mode == 2;
    if (a->cool == 0 && iabs(dx) > 50) {
        int r = rnd(0, 99);
        if (r < 40) { windup(a, AT_NONE, 24); return; }       /* a bomb, or (changed) gas */
        if (r < 80 && iabs(dy) < 14) {
            if (mutant) {
                dku_set_state(a, AS_SLAM);
                a->tx = t->x;
                a->ty = t->y;
                a->vz = 60;
                a->z = 1;
                a->vx = (t->x - a->x) / 30;
                a->vy = (t->y - a->y) / 30;
                a->timer2 = ti;
                dku_sfx("dku_leap");
            } else {
                windup(a, AT_E_ELBOW, 20);
            }
            return;
        }
    }
    melee_ai(a, i, ti, 18);
}

/* ---- state handling ----------------------------------------------------------- */

static void strike(Actor *a, int i) {
    /* the wind-up is over */
    int atk = a->atk;
    int ti = target_of(a);
    Actor *t = ti >= 0 ? &dku_g.a[ti] : NULL;
    switch (a->kind) {
    case AK_TORCH:
        if (t) { lob(SH_BOTTLE, a, i, t, 32, 12); dku_sfx("dku_toss"); }
        dku_set_state(a, AS_FREE);
        a->cool = rnd(90, 150);
        return;
    case AK_CROW:
        if (atk == AT_NONE) {
            dku_add_shot(SH_CLEAVER, 1, a->x + a->face * dku_fx(8), a->y, dku_fx(16), a->face * 48, 0, 0, 10, i);
            dku_sfx("dku_toss");
            dku_set_state(a, AS_FREE);
            a->cool = rnd(110, 170);
            return;
        }
        break;
    case AK_SLUDGER:
        if (atk == AT_NONE) {
            if (t) lob(SH_SPIT, a, i, t, 28, 0);
            dku_sfx("dku_spit");
            dku_set_state(a, AS_FREE);
            a->cool = rnd(120, 180);
            return;
        }
        break;
    case AK_VISITOR:
        if (atk == AT_NONE) {
            dku_add_shot(SH_RAY, 1, a->x + a->face * dku_fx(10), a->y, dku_fx(18), a->face * 26, 0, 0, 10, i);
            dku_sfx("dku_ray");
            dku_set_state(a, AS_FREE);
            a->cool = rnd(70, 120);
            return;
        }
        break;
    case AK_GRIST:
        if (atk == AT_NONE) {
            if (t) {
                if (a->mode == 2) lob(SH_GAS, a, i, t, 30, 0);
                else lob(SH_BOMB, a, i, t, 34, 14);
            }
            dku_sfx("dku_toss");
            dku_set_state(a, AS_FREE);
            a->cool = rnd(70, 110);
            return;
        }
        break;
    default: break;
    }
    if (atk == AT_E_RUSH) {
        dku_start_attack(a, AT_E_RUSH);
        a->state = AS_RUSH;
        a->vx = a->face * 64;
        dku_sfx("dku_rush");
        return;
    }
    if (atk == AT_E_POUNCE) {
        dku_start_attack(a, AT_E_POUNCE);
        a->state = AS_RUSH;
        a->vx = a->face * 52;
        a->vz = 26;
        a->z = 1;
        a->timer2 = 0;
        dku_sfx("dku_growl");
        return;
    }
    if (atk == AT_E_ELBOW) {
        dku_start_attack(a, AT_E_ELBOW);
        a->state = AS_RUSH;
        a->vx = a->face * 44;
        a->vz = 34;
        a->z = 1;
        a->timer2 = 0;
        dku_sfx("dku_leap");
        return;
    }
    dku_start_attack(a, atk == AT_NONE ? AT_E_HIT : atk);
    dku_sfx("dku_swipe");
}

static void thrown_update(Actor *a, int i) {
    bool landed = physics(a);
    /* a body in flight floors whoever it meets */
    for (int j = 2; j < DKU_MAX_ACTORS; j++) {
        Actor *o = &dku_g.a[j];
        if (j == i || !o->alive || o->team != 1 || o->state == AS_DEAD || o->state == AS_DOWN) continue;
        bool seen = false;
        for (int k = 0; k < a->nhits; k++) seen |= a->hits[k] == j;
        if (seen) continue;
        int w = DKU_KINDS[o->kind].hw + 8;
        if (o->kind == AK_UNDERTOW) {
            if (a->hit_boss) continue;
            if (iabs(dku_px(o->x - a->x)) <= w && dku_px(a->y) <= dku_px(o->y) + 30) {
                a->hit_boss = true;
                dku_hit(j, a->thrown_by, AT_NONE, 12, a->vx >= 0 ? 1 : -1, AF_KNOCK | 64 | 256);
                a->vx = -a->vx / 3;
            }
            continue;
        }
        if (iabs(dku_px(o->x - a->x)) <= w && iabs(dku_px(o->y - a->y)) <= 8) {
            if (a->nhits < DKU_MAX_HITS) a->hits[a->nhits++] = (uint8_t)j;
            dku_hit(j, a->thrown_by, AT_NONE, 6, a->vx >= 0 ? 1 : -1, AF_KNOCK | 64);
        }
    }
    if (landed) {
        int dir = a->vx >= 0 ? 1 : -1;
        bool charged = a->timer2 == 1;
        a->timer2 = 0;
        dku_set_state(a, AS_FREE);
        dku_hit(i, a->thrown_by, AT_NONE, a->thrown_dmg, dir, AF_KNOCK | 64);
        if (charged) dku_explode(a->x, a->y, 30, 12, 0, false);
        if (a->kind == AK_TORCH && a->state == AS_DEAD) a->st = 999;
        dku_g.shake = 4;
        dku_sfx("dku_thud");
    }
}

static void enter_update(Actor *a) {
    int l = dku_fx(dku_view_left() + 18), r = dku_fx(dku_view_right() - 18);
    int sp = DKU_KINDS[a->kind].speed + 6;
    if (a->kind == AK_PASSER) return;
    if (a->x < l) { a->x += sp; a->face = 1; }
    else if (a->x > r) { a->x -= sp; a->face = -1; }
    else { dku_set_state(a, AS_FREE); a->cool = rnd(10, 40); }
    a->step++;
    a->anim = dku_g.frame_t;
}

static void rise_update(Actor *a) {
    switch (a->from) {
    case FROM_GROUND:
        if (a->st >= 40) dku_set_state(a, AS_FREE);
        break;
    case FROM_ABOVE:
        if (physics(a) || a->z == 0) { dku_set_state(a, AS_FREE); dku_g.shake = 3; dku_sfx("dku_thud"); }
        break;
    case FROM_WATER: {
        /* a leap out of the water onto the planks */
        a->x += a->vx;
        a->y += a->vy;
        if (a->z > 0 || a->vz > 0) { a->z += a->vz; a->vz -= GRAV; }
        if (a->z <= 0 && a->st > 4) {
            a->z = 0;
            int lo = dku_fx(dku_floor_lo()), hi = dku_fx(dku_floor_hi());
            if (a->y < lo) a->y = lo;
            if (a->y > hi) a->y = hi;
            dku_set_state(a, AS_FREE);
            a->cool = 10;
        }
        break;
    }
    case FROM_ROOF:
        /* waits on the lift's roof for its turn (dku_world lets it go) */
        if (a->timer2 > 0 && a->st >= a->timer2) {
            a->vz = 20;
            a->vy = (dku_fx(dku_floor_lo() + 16) - a->y) / 20;
            a->from = FROM_ABOVE;
            a->st = 0;
        }
        break;
    default:
        dku_set_state(a, AS_FREE);
        break;
    }
}

static void foe_update(int i) {
    Actor *a = &dku_g.a[i];
    a->st++;
    if (a->hurt_t > 0) a->hurt_t--;
    if (a->cool > 0) a->cool--;
    switch (a->state) {
    case AS_GONE:
        a->alive = 0;
        return;
    case AS_DEAD: {
        bool landed = physics(a);
        if (landed) a->vx /= 2;
        if (a->z == 0) a->vx = a->vx * 3 / 4;
        if (a->exploding) {
            if (a->st >= 80) {
                a->exploding = false;
                dku_explode(a->x, a->y, 30, 16, 1, true);
                a->alive = 0;
            }
        } else if (a->st > 45) {
            a->alive = 0;
        }
        return;
    }
    case AS_FALL:
        if (a->st > 30) {
            a->alive = 0;
            if (a->team == 1) dku_g.kos++;
            if (a->boss) dku_boss_dead(i);
        }
        return;
    case AS_HURT:
        if (a->kind == AK_SAUCER) { a->state = AS_FREE; break; }
        a->vx = a->vx * 3 / 4;
        physics(a);
        if (--a->stun <= 0) {
            dku_set_state(a, AS_FREE);
            a->cool = imax(a->cool, a->kind == AK_SKIPPER ? 6 : 12);
        }
        return;
    case AS_DOWN: {
        bool landed = physics(a);
        if (landed) { a->vx /= 2; dku_burst(a->x, a->y, 0, C_TAN, 2); }
        /* a body knocked flying into the Undertow hurts it too */
        if (a->z > 0 && !a->hit_boss && a->team == 1 && dku_g.boss >= 0 && dku_g.boss != i) {
            Actor *b = &dku_g.a[dku_g.boss];
            if (b->alive && b->kind == AK_UNDERTOW && iabs(dku_px(b->x - a->x)) <= DKU_KINDS[AK_UNDERTOW].hw + 8 &&
                dku_px(a->y) <= dku_px(b->y) + 30) {
                a->hit_boss = true;
                dku_hit(dku_g.boss, a->last_hitter, AT_NONE, 6, a->vx >= 0 ? 1 : -1, AF_KNOCK | 256);
                a->vx = -a->vx / 3;
            }
        }
        if (a->z == 0) {
            a->vx = a->vx * 3 / 4;
            if (a->st > a->down_t + 14) {
                if (a->kind == AK_HOWLER) {
                    /* it springs up and over, untouchable for a moment */
                    dku_set_state(a, AS_RUSH);
                    a->atk = AT_NONE;
                    a->timer2 = 14;
                    a->vz = 30;
                    a->z = 1;
                    a->vx = -a->face * 30;
                } else if (a->kind == AK_VISITOR) {
                    dku_set_state(a, AS_BLINK);
                    a->blinks = rnd(1, 2);
                } else {
                    dku_set_state(a, AS_FREE);
                    a->cool = imax(a->cool, 16);
                }
            }
        } else {
            a->st = 0;
        }
        return;
    }
    case AS_GRABBED:
        return;
    case AS_THROWN:
        thrown_update(a, i);
        return;
    case AS_DORMANT: {
        for (int f = 0; f < 2; f++) {
            const Actor *p = &dku_g.a[f];
            if (!p->alive || p->state == AS_DEAD) continue;
            int dx = iabs(dku_px(p->x - a->x)), dy = iabs(dku_px(p->y - a->y));
            int r = a->mode == DM_DANCE ? 70 : a->mode == DM_SLEEP ? 40 : 52;
            if (dx < r && dy < 40) { a->mode = DM_NONE; dku_set_state(a, AS_FREE); a->cool = 20; break; }
        }
        return;
    }
    case AS_RISE:
        rise_update(a);
        return;
    case AS_ENTER:
        if (dku_g.frozen) return;
        enter_update(a);
        return;
    case AS_LEASHED:
        return;
    case AS_CHANGE:
        if (a->st >= 90) {
            a->mode = 2;
            a->hp = a->maxhp = 140;
            dku_set_state(a, AS_FREE);
            a->cool = 30;
            dku_sfx("dku_roar");
        }
        return;
    default: break;
    }
    if (dku_g.frozen && a->team == 1 && (a->state == AS_FREE || a->state == AS_GUARD)) return;

    /* the special movers */
    if (a->kind == AK_SAUCER) {
        /* in from the right, low over the street, and away to the left */
        a->x -= 20;
        int sx = dku_px(a->x) - dku_view_left();
        int zt = sx > 220 ? 50 : sx > 70 ? 10 : 50;
        a->z += a->z < dku_fx(zt) ? 8 : a->z > dku_fx(zt) ? -8 : 0;
        if (dku_px(a->x) < dku_view_left() - 30) a->alive = 0;
        return;
    }
    if (a->kind == AK_PASSER) {
        a->x -= a->timer2 > 0 ? 36 : 24;
        a->step++;
        a->face = -1;
        if (a->mode == 1 && a->st == 50) {
            /* night 2's passer-by: a cleaver from the left ends his run */
            dku_add_shot(SH_CLEAVER, 1, dku_fx(dku_view_left() - 10), a->y, dku_fx(16), 72, 0, 0, 10, -1);
        }
        if (dku_px(a->x) < dku_view_left() - 30) a->alive = 0;
        return;
    }
    if (a->kind == AK_DOG && a->team == 0 && a->state == AS_FREE) { ai_dog(a, i); return; }

    switch (a->state) {
    case AS_WINDUP:
        a->vx = a->vy = 0;
        if (a->st >= a->timer2) strike(a, i);
        return;
    case AS_ATTACK: {
        dku_attack_update(a, i);
        const DkuAtk *d = &DKU_ATK[a->atk];
        if (a->atk_t >= d->startup + d->active + d->recover) {
            if (a->kind == AK_VISITOR && a->blinks == 0 && a->timer2 < 2 && a->atk == AT_E_HIT) {
                /* a three-blow combo */
                a->timer2++;
                dku_start_attack(a, AT_E_HIT);
                a->atk_t = 12;
                return;
            }
            a->timer2 = 0;
            dku_set_state(a, AS_FREE);
            a->cool = a->kind == AK_SKIPPER ? rnd(16, 40) : a->kind == AK_DOG ? 20 : rnd(40, 80);
            if (a->boss) a->cool = rnd(24, 50);
        }
        return;
    }
    case AS_RUSH: {
        if (a->atk != AT_NONE) dku_attack_update(a, i);
        bool landed = physics(a);
        if (a->timer2 > 0) a->timer2--;
        bool over = false;
        if (a->kind == AK_RAMMER) {
            int sx = dku_px(a->x);
            over = a->atk_t > 90 || (a->vx > 0 && sx > dku_view_right() - 8) || (a->vx < 0 && sx < dku_view_left() + 8);
        } else {
            over = landed || (a->z == 0 && a->st > 2);
        }
        if (over) {
            a->vx = 0;
            dku_set_state(a, AS_FREE);
            a->cool = rnd(50, 90);
            if (a->kind == AK_RAMMER) { dku_set_state(a, AS_HURT); a->stun = 16; a->cool = rnd(60, 110); }
        }
        return;
    }
    case AS_SLAM: {
        if (a->kind == AK_GRIST && a->timer2 >= 0 && a->timer2 < DKU_MAX_ACTORS && a->vz > 0) {
            /* changed, he steers the slam after you */
            const Actor *t = &dku_g.a[a->timer2];
            a->vx += (t->x > a->x ? 2 : -2);
            a->vy += (t->y > a->y ? 1 : -1);
            a->vx = iclamp(a->vx, -40, 40);
            a->vy = iclamp(a->vy, -20, 20);
        }
        if (physics(a)) {
            /* lands: everyone on the floor near it is flattened; the air is safe */
            dku_start_attack(a, AT_E_SLAM);
            a->atk_t = 0;
            for (int j = 0; j < DKU_MAX_ACTORS; j++) {
                Actor *o = &dku_g.a[j];
                if (!o->alive || o->team != 0 || o->state == AS_DEAD) continue;
                if (o->z > dku_fx(4)) continue;
                if (iabs(dku_px(o->x - a->x)) <= 28 && iabs(dku_px(o->y - a->y)) <= 10)
                    dku_hit(j, i, AT_E_SLAM, 16, o->x >= a->x ? 1 : -1, AF_KNOCK);
            }
            dku_g.shake = 8;
            dku_sfx("dku_slam");
            dku_set_state(a, AS_HURT);
            a->stun = 26;
            a->cool = rnd(70, 120);
        }
        return;
    }
    case AS_BLINK: {
        int ti = target_of(a);
        if (a->st == 8 && ti >= 0) blink_to(a, &dku_g.a[ti]);
        if (a->st >= 16) {
            if (--a->blinks > 0) { a->st = 0; return; }
            dku_set_state(a, AS_FREE);
            a->cool = 18;
        }
        return;
    }
    case AS_GUARD: {
        int ti = target_of(a);
        if (ti >= 0) face_to(a, &dku_g.a[ti]);
        if (a->kind == AK_BULWARK) { if (ti >= 0) ai_bulwark(a, i, ti); return; }
        /* the tusker: hit its guard twice and it answers */
        if (a->guard_hits >= 2 || a->st >= a->timer2) {
            a->guard_hits = 0;
            windup(a, AT_E_HIT, DKU_KINDS[AK_TUSKER].windup);
        }
        return;
    }
    default: break;
    }

    /* AS_FREE: think */
    int ti = target_of(a);
    if (ti < 0) return;
    switch (a->kind) {
    case AK_SHAMBLER: case AK_GIGGLER: case AK_SKIPPER: melee_ai(a, i, ti, 16); break;
    case AK_TORCH: ai_torch(a, i, ti); break;
    case AK_CROW: ai_crow(a, i, ti); break;
    case AK_RAMMER: ai_rammer(a, i, ti); break;
    case AK_HOWLER: ai_howler(a, i, ti); break;
    case AK_TUSKER: ai_tusker(a, i, ti); break;
    case AK_SLUDGER: ai_sludger(a, i, ti); break;
    case AK_VISITOR: ai_visitor(a, i, ti); break;
    case AK_BULWARK: ai_bulwark(a, i, ti); break;
    case AK_FEELER: ai_feeler(a, i, ti); break;
    case AK_UNDERTOW: ai_undertow(a, i); break;
    case AK_GRIST: ai_grist(a, i, ti); break;
    default: break;
    }
    if (a->kind != AK_UNDERTOW && a->kind != AK_FEELER && a->state != AS_ENTER) clamp_in(a);
}

void dku_foes_update(void) {
    for (int i = 2; i < DKU_MAX_ACTORS; i++)
        if (dku_g.a[i].alive) foe_update(i);
}

void dku_foe_think(int i) { foe_update(i); }

/* a boss is down: the night ends at once and the rest go home */
void dku_boss_dead(int i) {
    Actor *a = &dku_g.a[i];
    if (a->kind == AK_GRIST && a->mode != 2) {
        /* the first form only: he changes */
        a->alive = 1;
        a->hp = 1;
        a->exploding = false;
        dku_set_state(a, AS_CHANGE);
        a->z = 0;
        a->vx = a->vz = 0;
        dku_sfx("dku_roar");
        return;
    }
    if (a->kind == AK_VISITOR) {
        /* night 3: both visitors must fall */
        for (int j = 2; j < DKU_MAX_ACTORS; j++)
            if (j != i && dku_g.a[j].alive && dku_g.a[j].boss && dku_g.a[j].state != AS_DEAD && dku_g.a[j].state != AS_FALL) return;
    }
    dku_g.boss_down = true;
    for (int j = 2; j < DKU_MAX_ACTORS; j++) {
        Actor *o = &dku_g.a[j];
        if (j == i || !o->alive || o->team != 1) continue;
        if (o->state == AS_DEAD) continue;
        dku_burst(o->x, o->y, dku_fx(12), C_GREY, 5);
        o->alive = 0;
    }
    for (int s = 0; s < DKU_MAX_SHOTS; s++) if (dku_g.sh[s].team == 1) dku_g.sh[s].alive = 0;
    dku_g.shake = 12;
}
