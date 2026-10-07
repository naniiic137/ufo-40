/* DUKES UP - the fighters and the rules of a blow: walking, running,
 * dodging and jumping, the punch string and its kick, the charged punch,
 * the dash attack, the flying kick, the spin, grabs, throws and slams,
 * picking things up and using them, and what a hit does to anyone.
 * Integer arithmetic only. */
#include "dku.h"

#define GRAV 4

/* POWER RECOV TOUGH THROW, and the colours of skin, hair, top, top trim, legs, shoes */
const DkuFighter DKU_FIGHTERS[DK_NFIGHTERS] = {
    {"ROOK", "THE BIG QUIET", "HOLD " GLYPH_B " IN A GRAB: A HIGHER SLAM", {2, 1, 2, 2},
     C_TAN, C_INK, C_NAVY, C_SKY, C_SLATE, C_INK},
    {"PIP", "THE SPRINTER", "A KICK THAT LANDS LETS HER JUMP AGAIN", {1, 3, 1, 2},
     C_CREAM, C_ORANGE, C_LEAF, C_WHITE, C_TEAL, C_WHITE},
    {"MACK", "THE SLUGGER", "HIS CHARGED PUNCH BURSTS WHAT IT FELLS", {3, 1, 1, 1},
     C_HIDE, C_BROWN, C_RED, C_YELLOW, C_NIGHT, C_RED},
    {"DOLLY", "THE LIFE OF THE PARTY", "HOLD " GLYPH_A " IN A GRAB: A THROW THAT GOES OFF", {1, 1, 2, 3},
     C_CREAM, C_YELLOW, C_MAGENTA, C_PINK, C_PURPLE, C_MAGENTA},
};

/* name, hp, speed (FX), half width, height, reach, hit, wind-up */
const DkuKind DKU_KINDS[AK_COUNT] = {
    {"FIGHTER", DKU_MAX_HP, 14, 6, 26, 22, 0, 0, 0},
    {"SHAMBLER", 10, 8, 6, 26, 20, 8, 18, 0},
    {"TORCH-BELLY", 14, 6, 9, 26, 0, 0, 30, 0},
    {"CROWMASK", 13, 9, 6, 26, 20, 6, 16, 0},
    {"RAMMER", 40, 9, 9, 28, 22, 14, 30, 0},
    {"HOWLER", 12, 14, 8, 20, 20, 10, 24, 0},
    {"TUSKER", 60, 7, 10, 30, 24, 12, 22, 0},
    {"SLUDGER", 30, 4, 7, 18, 22, 8, 24, 0},
    {"VISITOR", 28, 9, 6, 30, 22, 7, 14, 0},
    {"BULWARK", 30, 6, 8, 28, 22, 12, 22, 0},
    {"GIGGLER", 8, 12, 6, 26, 20, 14, 14, 0},
    {"MUDSKIPPER", 6, 18, 6, 22, 20, 9, 8, 0},
    {"FEELER", 16, 0, 5, 30, 26, 8, 30, 0},
    {"THE UNDERTOW", 60, 0, 22, 60, 0, 14, 40, 0},
    {"ALDERMAN GRIST", 110, 8, 10, 32, 24, 12, 18, 0},
    {"DOG", 30, 20, 8, 14, 18, 4, 6, 0},
    {"PASSER-BY", 4, 24, 6, 26, 0, 0, 0, 0},
    {"SAUCER", 6, 12, 10, 8, 0, 0, 0, 0},
};

/* startup, active, recover, reach, back, height reached, damage (+POWER), flags */
const DkuAtk DKU_ATK[AT_COUNT] = {
    [AT_JAB] = {3, 2, 7, 22, 0, 20, 0, 0},
    [AT_KICK] = {4, 3, 14, 25, 0, 20, 1, AF_KNOCK},
    [AT_CHARGED] = {3, 10, 20, 24, 0, 20, 4, AF_KNOCK | AF_REFLECT | AF_MOVE},
    [AT_DASH] = {2, 14, 26, 22, 0, 20, 2, AF_KNOCK | AF_REFLECT | AF_MOVE},
    [AT_FLYKICK] = {3, 60, 0, 22, 0, 26, 3, AF_KNOCK | AF_REFLECT | AF_AIR},
    [AT_SPIN] = {3, 12, 10, 26, 26, 22, 2, AF_KNOCK | AF_REFLECT | AF_SWEEP},
    [AT_PUMMEL] = {2, 2, 10, 20, 0, 20, 0, 0},
    [AT_SWING] = {6, 4, 14, 32, 0, 22, 3, AF_KNOCK | AF_REFLECT},
    [AT_SAW] = {12, 30, 20, 28, 0, 22, 0, 0},
    [AT_E_HIT] = {0, 3, 22, 20, 0, 20, 0, 0},
    [AT_E_RUSH] = {0, 90, 16, 18, 0, 22, 0, AF_KNOCK},
    [AT_E_POUNCE] = {0, 30, 14, 14, 0, 26, 0, AF_KNOCK},
    [AT_E_SLAM] = {0, 6, 24, 26, 26, 8, 0, AF_KNOCK},
    [AT_E_ELBOW] = {0, 40, 18, 16, 0, 30, 0, AF_KNOCK},
    [AT_E_BITE] = {0, 3, 16, 16, 0, 16, 0, 0},
};

/* name, heal %, cash, kind */
const DkuItemDef DKU_ITEMS[IT_COUNT] = {
    [IT_APPLE] = {"APPLE", 20, 2, IK_FOOD},
    [IT_SANDWICH] = {"SANDWICH", 33, 4, IK_FOOD},
    [IT_DRUMSTICK] = {"DRUMSTICK", 50, 6, IK_FOOD},
    [IT_ROAST] = {"ROAST CHICKEN", 100, 10, IK_FOOD},
    [IT_COIN] = {"COINS", 0, 1, IK_CASH},
    [IT_NOTE] = {"A FIVE", 0, 5, IK_CASH},
    [IT_RING] = {"GOLD RING", 0, 10, IK_CASH},
    [IT_PLANK] = {"PLANK", 0, 0, IK_SWING},
    [IT_CHAIN] = {"CHAIN", 0, 0, IK_SWING},
    [IT_PIPE] = {"PIPE", 0, 0, IK_SWING},
    [IT_ARM] = {"GHOUL ARM", 0, 0, IK_SWING},
    [IT_SCATTER] = {"SCATTERGUN", 0, 0, IK_GUN},
    [IT_SAW] = {"CHAINSAW", 0, 0, IK_SAW},
    [IT_BIN] = {"BIN", 0, 0, IK_THROW},
    [IT_HEAD] = {"GHOUL HEAD", 0, 0, IK_THROW},
    [IT_CLEAVER] = {"CLEAVER", 0, 0, IK_THROW},
    [IT_BOTTLE] = {"FIREBOTTLE", 0, 0, IK_THROW},
};

/* ---- stats ------------------------------------------------------------------ */

int dku_stat(int player, int stat) {
    if (player < 0 || player > 1) return 1;
    return iclamp(dk.pr[player].stat[stat], 1, 3);
}

static int charge_time(int player) {
    static const int T[4] = {0, 44, 32, 20};
    return T[dku_stat(player, DK_RECOV)];
}

int dku_fighter_dmg(const Actor *a, int atk) {
    int p = dku_stat(a->player, DK_POWER);
    int base = DKU_ATK[atk].dmg + p;
    if (atk == AT_CHARGED) {
        base = 4 + p * 2;
        if (dk.pr[a->player].pick == DK_MACK) base = 6 + p * 3;
    }
    if (atk == AT_SWING && a->carry >= 0) {
        static const int W[IT_COUNT] = {[IT_PLANK] = 4, [IT_CHAIN] = 4, [IT_PIPE] = 5, [IT_ARM] = 3};
        base = W[dk.it[a->carry].kind] + p;
    }
    return base;
}

/* ---- states ------------------------------------------------------------------ */

void dku_set_state(Actor *a, int s) {
    a->state = s;
    a->st = 0;
    if (s != AS_ATTACK) a->atk = AT_NONE;
}

void dku_start_attack(Actor *a, int atk) {
    a->state = AS_ATTACK;
    a->st = 0;
    a->atk = atk;
    a->atk_t = 0;
    a->nhits = 0;
    if (atk == AT_JAB || atk == AT_KICK) a->jab_t = 0;
}

int dku_fighters_alive(void) {
    int n = 0;
    for (int i = 0; i < 2; i++)
        if (dk.a[i].alive && dk.a[i].kind == AK_FIGHTER && dk.a[i].state != AS_DEAD && dk.a[i].state != AS_FALL &&
            dk.a[i].state != AS_GONE)
            n++;
    return n;
}

/* ---- reach: the heart of the game ------------------------------------------- */

/* Does a blow from a reach t? An attack reaches DKU_REACH_UP px up the floor
 * (towards the back) but only DKU_REACH_DOWN px down it, so whoever stands a
 * little below the other hits first. */
bool dku_in_reach(const Actor *a, const Actor *t, int reach, int back, int zhi) {
    int dx = dku_px(t->x - a->x) * a->face;
    int hw = DKU_KINDS[t->kind].hw;
    if (t->kind == AK_SLUDGER) hw -= 2; /* smaller than it looks */
    if (dx - hw > reach || dx + hw < -back) return false;
    int dy = dku_px(t->y - a->y);
    if (dy < -DKU_REACH_UP || dy > DKU_REACH_DOWN) return false;
    int tz = dku_px(t->z), az = dku_px(a->z);
    if (t->kind == AK_SAUCER) return tz <= az + zhi + 6;
    if (zhi < 0) return tz < az - zhi; /* an air blow: anything near its own height */
    return tz <= az + zhi;
}

static bool hit_listed(Actor *a, int j) {
    for (int k = 0; k < a->nhits; k++)
        if (a->hits[k] == j) return true;
    return false;
}

static void hit_list(Actor *a, int j) {
    if (a->nhits < DKU_MAX_HITS) a->hits[a->nhits++] = (uint8_t)j;
}

static bool hostile(const Actor *a, const Actor *t) {
    if (!t->alive || t == a) return false;
    if (t->state == AS_DEAD || t->state == AS_FALL || t->state == AS_GONE) return false;
    if (a->team == 0) {
        if (t->team == 1) return true;
        if (a->kind == AK_FIGHTER && t->team == 2) return true; /* passers-by, the saucer, a tied dog */
        return false;
    }
    if (a->team == 1) return t->team == 0;
    return false;
}

/* projectiles in a blow's reach fly back the other way */
void dku_reflect_in(const Actor *a, int idx) {
    const DkuAtk *d = &DKU_ATK[a->atk];
    for (int i = 0; i < DKU_MAX_SHOTS; i++) {
        Shot *s = &dk.sh[i];
        if (!s->alive || s->team == a->team || s->kind == SH_GAS || s->kind == SH_SPIT) continue;
        int dx = dku_px(s->x - a->x) * a->face;
        int dy = dku_px(s->y - a->y);
        int dz = dku_px(s->z - a->z);
        if (dx > d->reach + 6 || dx < -d->back - 6) continue;
        if (dy < -DKU_REACH_UP - 2 || dy > DKU_REACH_DOWN + 4) continue;
        if (dz < -8 || dz > 34) continue;
        int sp = iabs(s->vx);
        if (sp < 40) sp = 40;
        s->vx = a->face * sp;
        s->vy = 0;
        if (s->kind == SH_BOTTLE || s->kind == SH_BOMB) { s->vz = 24; if (s->z < dku_fx(10)) s->z = dku_fx(10); }
        s->team = a->team;
        s->owner = idx;
        s->reflected = true;
        dku_burst(s->x, s->y, s->z, C_WHITE, 4);
        dku_sfx("dku_reflect");
    }
}

/* ---- what a hit does ---------------------------------------------------------- */

void dku_knock(Actor *t, int dir, int power) {
    if (t->state == AS_GRABBED) t->partner = -1;
    dku_set_state(t, AS_DOWN);
    t->vx = dir * (16 + power * 8);
    t->vz = 26 + power * 6;
    if (t->z < 1) t->z = 1;
    t->vy = 0;
    t->stun = 0;
    t->face = -dir;
    if (t->kind == AK_FIGHTER) {
        static const int D[4] = {0, 50, 38, 26};
        t->down_t = D[dku_stat(t->player, DK_RECOV)];
    } else if (t->kind == AK_GIGGLER) {
        t->down_t = 14; /* springs straight back up */
    } else if (t->kind == AK_HOWLER) {
        t->down_t = 24;
    } else {
        t->down_t = 40;
    }
}

void dku_kill(int i, int dir) {
    Actor *t = &dk.a[i];
    if (t->state == AS_GRABBED && t->partner >= 0) {
        Actor *h = &dk.a[t->partner];
        if (h->state == AS_GRAB || h->state == AS_SUPLEX) { h->partner = -1; if (h->state == AS_GRAB) dku_set_state(h, AS_FREE); }
    }
    if (t->state == AS_GRAB && t->partner >= 0) {
        Actor *p = &dk.a[t->partner];
        if (p->state == AS_GRABBED) dku_set_state(p, AS_FREE);
    }
    t->partner = -1;
    t->hp = 0;
    if (t->kind == AK_FIGHTER) {
        dku_set_state(t, AS_DEAD);
        t->vx = dir * 16;
        t->vz = 30;
        if (t->z < 1) t->z = 1;
        if (t->carry >= 0) { dk.it[t->carry].alive = 1; dk.it[t->carry].z = 0; dk.it[t->carry].x = t->x; dk.it[t->carry].y = t->y; t->carry = -1; }
        dku_sfx("dku_ko");
        return;
    }
    dku_set_state(t, AS_DEAD);
    t->vx = dir * 20;
    t->vz = 28;
    if (t->z < 1) t->z = 1;
    if (t->team == 1) dk.kos++;
    if (t->carry >= 0) t->carry = -1;
    switch (t->kind) {
    case AK_SHAMBLER: {
        int r = rng_range(&dk.rng, 0, 99);
        if (r < 14) dku_drop_item(IT_HEAD, dku_px(t->x), dku_px(t->y));
        else if (r < 22) dku_drop_item(IT_ARM, dku_px(t->x), dku_px(t->y));
        break;
    }
    case AK_TORCH: t->exploding = true; break;
    case AK_CROW: if (rng_range(&dk.rng, 0, 99) < 25) dku_drop_item(IT_CLEAVER, dku_px(t->x), dku_px(t->y)); break;
    case AK_SAUCER:
        dku_explode(t->x, t->y, 22, 10, 2, true);
        dku_drop_item(IT_RING, dku_px(t->x), dku_px(t->y));
        t->alive = 0;
        break;
    case AK_PASSER: break;
    default: break;
    }
    if (t->boss) dku_boss_dead(i);
    dku_sfx(t->kind == AK_SAUCER ? "dku_boom" : "dku_ko");
}

void dku_hurt_fighter(int i, int dmg, bool reducible, int dir, bool knock, int src) {
    Actor *f = &dk.a[i];
    if (!f->alive || f->state == AS_DEAD || f->state == AS_FALL) return;
    int tough = dku_stat(f->player, DK_TOUGH);
    dmg -= reducible ? 3 * (tough - 1) : (tough - 1);
    if (dmg < 1) dmg = 1;
    dk.last_dmg = dmg;
    dk.last_hit_kind = src;
    if (src + 10 >= 0 && src + 10 < 32 && !dk.god) dk.dmg_by[src + 10] += dmg;
    f->hurt_t = 10;
    if (dk.god) dmg = 0;
    f->hp -= dmg;
    f->charge_t = 0;
    f->charged = false;
    f->running = false;
    dku_sfx("dku_hurt");
    if (f->state == AS_GRAB || f->state == AS_SUPLEX) {
        if (f->partner >= 0) {
            Actor *p = &dk.a[f->partner];
            if (p->state == AS_GRABBED) { dku_set_state(p, AS_HURT); p->stun = 10; p->z = 0; }
        }
        f->partner = -1;
    }
    if (f->hp <= 0) {
        f->hp = 0;
        dku_kill(i, dir);
        return;
    }
    if (knock || f->z > 0 || f->state == AS_AIR) {
        if (f->state == AS_DOWN) { f->vz = 10; return; }
        dku_knock(f, dir, 1);
    } else if (f->state == AS_DOWN) {
        /* no getting-up shield: you can be hit while you lie there */
    } else {
        dku_set_state(f, AS_HURT);
        f->stun = DKU_PSTUN;
        f->vx = dir * 8;
    }
}

static bool guarding(const Actor *t, int dir_from) {
    /* dir_from: the side the blow comes from (+1 from the right) */
    if (t->state != AS_GUARD) return false;
    return dir_from == t->face;
}

/* A blow lands (or is guarded). dir: the way it pushes (+1 to the right). */
bool dku_hit(int ti, int ai, int atk, int dmg, int dir, uint32_t flags) {
    Actor *t = &dk.a[ti];
    if (!t->alive || t->state == AS_DEAD || t->state == AS_FALL || t->state == AS_GONE) return false;
    if (dir == 0) dir = 1;
    const Actor *a = ai >= 0 ? &dk.a[ai] : NULL;
    bool thrown = (flags & 64) != 0;
    if (t->kind == AK_FIGHTER) {
        bool knock = (flags & AF_KNOCK) != 0;
        bool red = (flags & 128) == 0; /* fire and blasts get through TOUGH */
        dku_hurt_fighter(ti, dmg, red, dir, knock, a ? a->kind : -1);
        return true;
    }
    /* a dog still on its leash: the blow breaks the leash */
    if (t->kind == AK_DOG && t->state == AS_LEASHED) {
        t->team = 0;
        dku_set_state(t, AS_FREE);
        dku_sfx("dku_bark");
        return true;
    }
    /* a passer-by in the gym drops something and runs on */
    if (t->kind == AK_PASSER) {
        if (t->mode == 0) {
            static const uint8_t DROPS[] = {IT_APPLE, IT_SANDWICH, IT_DRUMSTICK, IT_ROAST, IT_PIPE, IT_PLANK,
                                            IT_BOTTLE, IT_SCATTER, IT_CHAIN, IT_SAW, 0};
            int r = rng_range(&dk.rng, 0, ARRAY_LEN(DROPS) - 1);
            if (DROPS[r]) dku_drop_item(DROPS[r], dku_px(t->x), dku_px(t->y));
            else {
                int d = dku_spawn(AK_DOG, FROM_AT, dku_px(t->x), dku_px(t->y), 0);
                if (d >= 0) { dk.a[d].team = 0; dku_set_state(&dk.a[d], AS_FREE); }
            }
            t->mode = 2;
            t->timer2 = 1;
            dku_sfx("dku_yelp");
        }
        return true;
    }
    if (t->state == AS_DORMANT) { t->mode = DM_NONE; dku_set_state(t, AS_FREE); }
    /* a howler springing up and a visitor mid-blink can't be touched */
    if (t->kind == AK_HOWLER && t->state == AS_RUSH && t->timer2 > 0) return false;
    if (t->kind == AK_VISITOR && t->state == AS_BLINK && t->st >= 6) return false;
    if (t->state == AS_CHANGE) return false;
    if (t->state == AS_RISE && t->from == FROM_WATER && t->z > dku_fx(20)) return false;
    if (t->kind == AK_UNDERTOW) {
        /* fists bounce off: only a body or something thrown hurts it */
        if (!thrown && !(flags & 256)) { dku_burst(t->x, t->y, dku_fx(30), C_GREY, 3); dku_sfx("dku_block"); return false; }
    }
    if (guarding(t, -dir)) {
        if (thrown) {
            t->guard_hits = 0;
            dku_knock(t, dir, 1);
            t->hp -= dmg;
            if (t->hp <= 0) dku_kill(ti, dir);
            return true;
        }
        dku_burst(t->x, t->y, dku_fx(18), C_WHITE, 3);
        dku_sfx("dku_block");
        t->guard_hits++;
        if (t->kind == AK_BULWARK && t->guard_hits >= 4 && (atk == AT_JAB || atk == AT_KICK || atk == AT_PUMMEL || atk == AT_SWING)) {
            dku_set_state(t, AS_HURT);
            t->stun = 34;
            t->timer2 = 150; /* the guard stays down a while */
            t->guard_hits = 0;
            dku_sfx("dku_guardbreak");
        }
        return true;
    }
    t->hp -= dmg;
    t->last_hitter = ai;
    t->hurt_t = 6;
    dku_burst(t->x, t->y, dku_fx(16) + t->z, C_YELLOW, 2);
    if (t->hp <= 0) {
        dku_kill(ti, dir);
        if (a && a->kind == AK_FIGHTER && atk == AT_CHARGED && dk.pr[a->player].pick == DK_MACK) {
            /* MACK's charged punch bursts what it fells */
            dku_burst(t->x, t->y, dku_fx(14), C_RED, 14);
            t->state = AS_GONE;
            t->alive = t->kind == AK_TORCH ? 1 : 0;
            if (t->kind == AK_TORCH) { t->state = AS_DEAD; t->st = 999; }
        }
        return true;
    }
    if (t->kind == AK_SAUCER || t->kind == AK_UNDERTOW || t->kind == AK_FEELER) {
        t->stun = 0;
        if (t->kind == AK_FEELER) { t->state = AS_HURT; t->stun = 16; t->st = 0; }
        return true;
    }
    if (t->kind == AK_GRIST && t->state == AS_SLAM) return true; /* mid-slam: no stagger */
    if (t->state == AS_GRABBED) return true;
    if (t->state == AS_DOWN) {
        if (t->z == 0) t->vz = 8; /* a little bounce; it stays down */
        return true;
    }
    if (flags & AF_KNOCK) {
        dku_knock(t, dir, (flags & 512) ? 2 : 1);
    } else {
        if (t->state == AS_THROWN) return true;
        if (t->state == AS_ATTACK || t->state == AS_WINDUP || t->state == AS_RUSH || t->state == AS_SLAM) t->atk = AT_NONE;
        t->state = AS_HURT;
        t->st = 0;
        t->stun = DKU_STUN;
        t->vx = dir * 4;
        t->vz = 0;
        t->z = t->kind == AK_SAUCER ? t->z : 0;
    }
    return true;
}

/* ---- physics -------------------------------------------------------------------- */

static bool physics(Actor *a) {
    bool landed = false;
    if (a->z > 0 || a->vz > 0) {
        a->z += a->vz;
        a->vz -= GRAV;
        if (a->z <= 0) {
            a->z = 0;
            a->vz = 0;
            landed = true;
        }
    }
    a->x += a->vx;
    a->y += a->vy;
    int lo = dku_fx(dku_floor_lo()), hi = dku_fx(dku_floor_hi());
    if (a->kind != AK_SAUCER && a->kind != AK_UNDERTOW) {
        if (a->y < lo) a->y = lo;
        if (a->y > hi) a->y = hi;
    }
    return landed;
}

void dku_actor_physics(Actor *a) { physics(a); }

static void clamp_view(Actor *a) {
    int l = dku_fx(dku_view_left() + 6), r = dku_fx(dku_view_right() - 6);
    if (a->x < l) a->x = l;
    if (a->x > r) a->x = r;
}

/* ---- items ------------------------------------------------------------------------ */

static int item_under(const Actor *a) {
    int best = -1, bd = 999;
    for (int i = 0; i < DKU_MAX_ITEMS; i++) {
        const Item *it = &dk.it[i];
        if (it->alive != 1 || it->z > 0) continue;
        int dx = iabs(dku_px(it->x - a->x)), dy = iabs(dku_px(it->y - a->y));
        if (dx <= 10 && dy <= 6 && dx + dy < bd) { bd = dx + dy; best = i; }
    }
    return best;
}

static void take_item(Actor *a, int ii) {
    Item *it = &dk.it[ii];
    const DkuItemDef *d = &DKU_ITEMS[it->kind];
    if (d->kind == IK_FOOD) {
        if (a->hp >= DKU_MAX_HP) {
            dk.cash += d->cash;
            dku_sfx("dku_cash");
        } else {
            a->hp = imin(DKU_MAX_HP, a->hp + (d->food_pct * DKU_MAX_HP) / 100);
            if (it->kind == IT_ROAST) a->hp = DKU_MAX_HP;
            dku_sfx("dku_eat");
        }
        it->alive = 0;
        return;
    }
    if (d->kind == IK_CASH) {
        dk.cash += d->cash;
        it->alive = 0;
        dku_sfx("dku_cash");
        return;
    }
    /* something to carry: its slot stays taken (2) while it's held */
    a->carry = ii;
    it->alive = 2;
    a->ready = imax(2, 26 - 8 * dku_stat(a->player, DK_THROW));
    a->running = false;
    dku_sfx("dku_pickup");
}

static void drop_carry(Actor *a) {
    if (a->carry < 0) return;
    Item *it = &dk.it[a->carry];
    it->alive = 1;
    it->x = a->x;
    it->y = a->y;
    it->z = 0;
    it->vx = 0;
    it->vz = 0;
    a->carry = -1;
}

static int item_throw_dmg(int kind) {
    switch (kind) {
    case IT_BIN: return 8;
    case IT_HEAD: return 6;
    case IT_CLEAVER: return 10;
    case IT_BOTTLE: return 12;
    default: return 6;
    }
}

static void throw_carry(Actor *a, int idx) {
    if (a->carry < 0) return;
    Item *it = &dk.it[a->carry];
    int sp = 64 + 16 * dku_stat(a->player, DK_THROW);
    int s = dku_add_shot(SH_ITEM, a->team, a->x + a->face * dku_fx(8), a->y, dku_fx(14), a->face * sp, 0, 6,
                         item_throw_dmg(it->kind), idx);
    if (s >= 0) {
        dk.sh[s].arg = it->kind;
        dk.sh[s].t = it->ammo; /* a scattergun keeps its shells if picked up again */
    }
    it->alive = 0;
    a->carry = -1;
    dku_sfx("dku_throw");
    dku_set_state(a, AS_ATTACK);
    a->atk = AT_NONE;
    a->atk_t = 0;
    a->timer2 = 10;
}

static void use_carry(Actor *a, int idx) {
    Item *it = &dk.it[a->carry];
    int k = DKU_ITEMS[it->kind].kind;
    if (k == IK_SWING) {
        dku_start_attack(a, AT_SWING);
        dku_sfx("dku_swing");
    } else if (k == IK_SAW) {
        dku_start_attack(a, AT_SAW);
        dku_sfx("dku_saw");
    } else if (k == IK_GUN) {
        for (int p = -1; p <= 1; p++)
            dku_add_shot(SH_PELLET, a->team, a->x + a->face * dku_fx(12), a->y + p * dku_fx(3), dku_fx(16), a->face * 96,
                         p * 3, 0, 10, idx);
        it->ammo--;
        dku_sfx("dku_shotgun");
        dku_start_attack(a, AT_NONE);
        a->state = AS_ATTACK;
        a->atk = AT_NONE;
        a->timer2 = 22;
        if (it->ammo <= 0) { it->alive = 0; a->carry = -1; } /* six shells and it's done */
    } else {
        throw_carry(a, idx);
    }
}

/* ---- grabs and throws --------------------------------------------------------------- */

static bool grabbable(const Actor *t) {
    if (!t->alive || t->team != 1 || t->state != AS_HURT) return false;
    switch (t->kind) {
    case AK_FEELER: case AK_UNDERTOW: case AK_SAUCER: return false;
    default: return true;
    }
}

static void start_grab(Actor *a, int idx, int ti) {
    Actor *t = &dk.a[ti];
    dku_set_state(a, AS_GRAB);
    a->partner = ti;
    a->grab_charge = 0;
    a->combo = 0;
    a->running = false;
    dku_set_state(t, AS_GRABBED);
    t->partner = idx;
    t->stun = 0;
    t->vx = t->vy = t->vz = 0;
    t->z = 0;
    t->face = -a->face;
    dku_sfx("dku_grab");
}

void dku_throw_actor(int holder, int dir, bool charged) {
    Actor *a = &dk.a[holder];
    if (a->partner < 0) return;
    Actor *t = &dk.a[a->partner];
    int th = dku_stat(a->player, DK_THROW);
    dku_set_state(t, AS_THROWN);
    t->partner = -1;
    t->thrown_by = holder;
    t->thrown_dmg = 4 + th + (charged ? 8 : 0);
    t->exploding = t->kind == AK_TORCH ? t->exploding : false;
    t->timer2 = charged ? 1 : 0; /* goes off when it lands */
    t->vx = dir * (40 + 14 * th);
    t->vz = 22 + 4 * th;
    t->z = dku_fx(10);
    t->x = a->x + dir * dku_fx(10);
    t->nhits = 0;
    t->hit_boss = false;
    a->partner = -1;
    a->face = dir;
    dku_set_state(a, AS_ATTACK);
    a->atk = AT_NONE;
    a->timer2 = 16 - 3 * th;
    dku_sfx("dku_throw");
}

static void start_suplex(Actor *a, bool super) {
    a->state = AS_SUPLEX;
    a->st = 0;
    a->vz = super ? 70 : 50;
    a->z = 1;
    a->timer2 = super ? 1 : 0;
    a->vx = 0;
    dku_sfx("dku_jump");
}

/* the slam at the end of a suplex */
static void suplex_land(Actor *a, int idx) {
    int p = dku_stat(a->player, DK_POWER);
    bool super = a->timer2 != 0;
    if (a->partner >= 0) {
        Actor *t = &dk.a[a->partner];
        t->partner = -1;
        t->z = 0;
        t->x = a->x + a->face * dku_fx(8);
        t->y = a->y;
        dku_set_state(t, AS_FREE);
        int dmg = super ? 8 + p * 3 : 4 + p * 2;
        dku_hit(a->partner, idx, AT_NONE, dmg, a->face, AF_KNOCK);
        if (t->kind == AK_TORCH && t->state == AS_DEAD) t->st = 999; /* it goes off at once */
    }
    a->partner = -1;
    int r = super ? 40 : 22;
    for (int j = 2; j < DKU_MAX_ACTORS; j++) {
        Actor *o = &dk.a[j];
        if (!hostile(a, o) || o->team != 1 || o->state == AS_DOWN) continue;
        int dx = iabs(dku_px(o->x - a->x)), dy = iabs(dku_px(o->y - a->y));
        if (dx <= r && dy <= r / 2 && o->z < dku_fx(10)) dku_hit(j, idx, AT_NONE, super ? 6 : 3, dku_px(o->x - a->x) >= 0 ? 1 : -1, AF_KNOCK);
    }
    dk.shake = super ? 10 : 6;
    dku_burst(a->x + a->face * dku_fx(8), a->y, 0, C_TAN, super ? 14 : 8);
    dku_sfx("dku_slam");
    dku_set_state(a, AS_ATTACK);
    a->atk = AT_NONE;
    a->timer2 = 18;
}

/* ---- the attack in progress ---------------------------------------------------------- */

static void hit_props(Actor *a, const DkuAtk *d) {
    for (int i = 0; i < DKU_MAX_PROPS; i++) {
        Prop *p = &dk.pr_[i];
        if (!p->alive) continue;
        int dx = dku_px(p->x - a->x) * a->face;
        int dy = dku_px(p->y - a->y);
        if (dx - 7 > d->reach || dx + 7 < -d->back) continue;
        if (dy < -DKU_REACH_UP || dy > DKU_REACH_DOWN + 2) continue;
        if (a->z > dku_fx(24)) continue;
        if (p->shake > 0) continue; /* one knock a blow */
        p->shake = 10;
        p->hp--;
        dku_sfx("dku_thud");
        if (p->hp <= 0) {
            p->alive = 0;
            int px = dku_px(p->x), py = dku_px(p->y);
            if (p->content) dku_drop_item(p->content, px + 4 * a->face, py);
            if (p->content2) dku_drop_item(p->content2, px - 4 * a->face, py + 2);
            if (p->kind == PR_BIN) dku_drop_item(IT_BIN, px, py);
            if (p->kind == PR_POST) {
                /* the leash: the dog tied to it is free */
                for (int j = 2; j < DKU_MAX_ACTORS; j++)
                    if (dk.a[j].alive && dk.a[j].kind == AK_DOG && dk.a[j].state == AS_LEASHED &&
                        iabs(dku_px(dk.a[j].x - p->x)) < 30) {
                        dk.a[j].team = 0;
                        dku_set_state(&dk.a[j], AS_FREE);
                        dku_sfx("dku_bark");
                    }
            }
            dku_burst(p->x, p->y, dku_fx(6), p->kind == PR_TUFT ? C_LEAF : C_BROWN, 6);
        }
    }
    /* loose things on the floor get knocked aside */
    for (int i = 0; i < DKU_MAX_ITEMS; i++) {
        Item *it = &dk.it[i];
        if (it->alive != 1 || it->z > 0) continue;
        int dx = dku_px(it->x - a->x) * a->face;
        int dy = dku_px(it->y - a->y);
        if (dx < 0 || dx > d->reach + 2 || dy < -DKU_REACH_UP || dy > DKU_REACH_DOWN + 2) continue;
        if (a->atk == AT_FLYKICK || a->atk == AT_KICK || a->atk == AT_DASH || a->atk == AT_SPIN) {
            it->vx = a->face * 40;
            it->vz = 16;
            it->z = 1;
        }
    }
}

void dku_attack_update(Actor *a, int idx) {
    const DkuAtk *d = &DKU_ATK[a->atk];
    a->atk_t++;
    int t = a->atk_t;
    bool active = t > d->startup && t <= d->startup + d->active;
    if (a->atk == AT_FLYKICK) active = t > d->startup;
    if (a->atk == AT_SAW && t > d->startup && t <= d->startup + d->active && (t % 6) == 0) a->nhits = 0;
    if (!active) return;
    if (d->flags & AF_MOVE) {
        int sp = a->atk == AT_DASH ? 34 : 40;
        a->vx = a->face * sp;
    }
    if (d->flags & AF_REFLECT) dku_reflect_in(a, idx);
    int zhi = (d->flags & AF_AIR) ? -22 : d->zhi;
    for (int j = 0; j < DKU_MAX_ACTORS; j++) {
        Actor *o = &dk.a[j];
        if (!hostile(a, o) || hit_listed(a, j)) continue;
        if (o->state == AS_GRABBED && o->partner != idx && a->kind == AK_FIGHTER) {
            /* someone else's catch: fair game */
        }
        int reach = d->reach, back = d->back;
        if (a->atk == AT_E_HIT || a->atk == AT_E_BITE) reach = DKU_KINDS[a->kind].reach;
        if (a->atk == AT_SWING && a->carry >= 0 && dk.it[a->carry].kind == IT_CHAIN) reach = 38;
        if (!dku_in_reach(a, o, reach, back, zhi)) continue;
        hit_list(a, j);
        int dmg;
        uint32_t fl = d->flags;
        if (a->kind == AK_FIGHTER) {
            dmg = dku_fighter_dmg(a, a->atk);
            if (a->atk == AT_SAW) {
                if (o->kind == AK_SHAMBLER) dmg = o->hp; /* a shambler goes down at once */
                else dmg = 2;
            }
            if (a->atk == AT_SPIN) {
                if (!a->spin_paid && o->team == 1) {
                    a->spin_paid = 1;
                    /* the spin costs health, but only when it connects */
                    if (!dk.god) a->hp = imax(1, a->hp - DKU_SPIN_COST);
                }
                if (o->team == 1 && dku_px(o->x - a->x) * a->face < 0) {
                    /* whoever was behind ends up in front */
                    o->x = a->x + a->face * dku_fx(12);
                }
            }
        } else if (a->kind == AK_DOG) {
            dmg = 4;
        } else {
            dmg = DKU_KINDS[a->kind].dmg;
            if (a->atk == AT_E_RUSH) dmg = 14;
            if (a->atk == AT_E_POUNCE) dmg = 10;
            if (a->atk == AT_E_ELBOW) dmg = 14;
            if (a->atk == AT_E_SLAM) dmg = 16;
            if (a->boss && a->kind == AK_VISITOR) dmg = 8;
        }
        int dir = a->face;
        if (a->atk == AT_SPIN) dir = a->face;
        bool was_alive = o->hp > 0;
        bool landed = dku_hit(j, idx, a->atk, dmg, dir, fl);
        if (landed && a->kind == AK_FIGHTER && a->atk == AT_FLYKICK && dk.pr[a->player].pick == DK_PIP) a->air_jump = true;
        if (landed && a->kind == AK_FIGHTER && was_alive && o->hp <= 0 && o->team == 1 &&
            (a->atk == AT_JAB || a->atk == AT_PUMMEL)) {
            /* a kill always brings the kick: it floors everyone in front */
            a->timer2 = -1;
            dk.kick_kills++;
        }
        if (a->atk == AT_E_RUSH || a->atk == AT_E_POUNCE || a->atk == AT_E_ELBOW) {
            /* the charge carries on through */
        }
    }
    /* the rammer's charge flattens other ghouls too */
    if (a->atk == AT_E_RUSH) {
        for (int j = 2; j < DKU_MAX_ACTORS; j++) {
            Actor *o = &dk.a[j];
            if (j == idx || !o->alive || o->team != 1 || hit_listed(a, j) || o->state == AS_DEAD || o->boss) continue;
            if (!dku_in_reach(a, o, 18, 0, 22)) continue;
            hit_list(a, j);
            dku_hit(j, idx, AT_E_RUSH, 10, a->face, AF_KNOCK);
        }
    }
    if (a->kind == AK_FIGHTER) hit_props(a, d);
}

/* ---- the fighters ------------------------------------------------------------------- */

void dku_fighter_input(int i, uint32_t *held, uint32_t *press, uint32_t *rel) {
    uint32_t h = 0, p = 0, r = 0;
    for (int b = 0; b < 6; b++) {
        int m = 1 << b;
        if (i == 0) {
            if (btn(m)) h |= (uint32_t)m;
            if (btnp(m)) p |= (uint32_t)m;
            if (btnr(m)) r |= (uint32_t)m;
        } else {
            if (btn2(m)) h |= (uint32_t)m;
            if (btnp2(m)) p |= (uint32_t)m;
            if (btnr(m << BTN_P2_SHIFT)) r |= (uint32_t)m;
        }
    }
    *held = h;
    *press = p;
    *rel = r;
}

static void start_jab(Actor *a) {
    if (a->combo >= 4) {
        dku_start_attack(a, AT_KICK);
        a->combo = 0;
    } else {
        dku_start_attack(a, AT_JAB);
        a->combo++;
    }
    a->combo_t = 0;
    dku_sfx(a->atk == AT_KICK ? "dku_kick" : "dku_jab");
}

static void start_spin(Actor *a) {
    if (a->state == AS_GRAB && a->partner >= 0) {
        Actor *p = &dk.a[a->partner];
        if (p->state == AS_GRABBED) { dku_set_state(p, AS_HURT); p->stun = 8; }
        a->partner = -1;
    }
    if (a->carry >= 0) drop_carry(a);
    dku_start_attack(a, AT_SPIN);
    a->state = AS_SPIN;
    a->spin_paid = 0;
    a->charge_t = 0;
    a->charged = false;
    a->running = false;
    a->vx = a->vy = 0;
    a->combo = 0;
    dku_sfx("dku_spin");
}

static bool grounded_any(const Actor *a) {
    if (a->z > 0) return false;
    switch (a->state) {
    case AS_FREE: case AS_ATTACK: case AS_HURT: case AS_DODGE: case AS_GRAB: return true;
    case AS_AIR: return a->st < 3; /* still crouched to jump */
    default: return false;
    }
}

static void fighter_update(int i) {
    Actor *a = &dk.a[i];
    if (!a->alive) return;
    uint32_t held, press, rel;
    dku_fighter_input(i, &held, &press, &rel);
    a->st++;
    if (a->hurt_t > 0) a->hurt_t--;
    a->jab_t++;
    int hdir = (held & BTN_RIGHT ? 1 : 0) - (held & BTN_LEFT ? 1 : 0);
    int vdir = (held & BTN_DOWN ? 1 : 0) - (held & BTN_UP ? 1 : 0);
    a->tap_t++;

    /* the spin: both buttons at once, at nearly any moment on the ground */
    bool both = ((press & BTN_A) && (held & BTN_B)) || ((press & BTN_B) && (held & BTN_A));
    if (both && grounded_any(a)) {
        start_spin(a);
    }

    switch (a->state) {
    case AS_DEAD:
        if (physics(a)) { a->vx = 0; }
        if (a->z == 0) a->vx = a->vx * 3 / 4;
        return;
    case AS_FALL:
        if (a->st > 30) { a->state = AS_GONE; a->hp = 0; }
        return;
    case AS_GONE:
        return;
    case AS_HURT:
        if (--a->stun <= 0) dku_set_state(a, AS_FREE);
        a->vx = a->vx * 3 / 4;
        physics(a);
        clamp_view(a);
        return;
    case AS_DOWN: {
        bool landed = physics(a);
        if (landed) { a->vx /= 3; dku_sfx("dku_thud"); }
        if (a->z == 0) {
            a->vx = a->vx * 3 / 4;
            /* roll along the floor (no shield) */
            if (hdir || vdir) { a->x += hdir * 16; a->y += vdir * 12; }
            if (a->st > a->down_t + 20) { dku_set_state(a, AS_FREE); a->combo = 0; }
        } else {
            a->st = 0;
        }
        clamp_view(a);
        return;
    }
    case AS_GRABBED:
        /* a fighter is never held in DUKES UP */
        dku_set_state(a, AS_FREE);
        return;
    default: break;
    }

    /* the punch string resets after a pause, or a few frames moving up or down */
    if (a->state != AS_ATTACK || (a->atk != AT_JAB && a->atk != AT_KICK)) a->combo_t++;
    if (a->combo_t > DKU_COMBO_RESET) a->combo = 0;
    if (vdir && a->state == AS_FREE) {
        if (++a->vert_t >= 4) a->combo = 0;
    } else {
        a->vert_t = 0;
    }
    if (a->ready > 0) a->ready--;

    /* holding A charges */
    if ((held & BTN_A) && a->carry < 0 && (a->state == AS_FREE || a->state == AS_ATTACK) && !a->running) {
        a->charge_t++;
        if (a->charge_t >= 12 + charge_time(a->player) && !a->charged) { a->charged = true; dku_sfx("dku_charge"); }
    }

    switch (a->state) {
    case AS_FREE: {
        if (a->land_t > 0) { a->land_t--; a->vx = a->vy = 0; break; }
        /* double taps */
        int pdir = (press & BTN_RIGHT) ? 2 : (press & BTN_LEFT) ? 1 : (press & BTN_DOWN) ? 4 : (press & BTN_UP) ? 3 : 0;
        if (pdir) {
            if (pdir == a->tapdir && a->tap_t <= DKU_DBL_TAP) {
                if (pdir <= 2) {
                    bool heavy = a->carry >= 0 && dk.it[a->carry].kind == IT_BIN;
                    if (!heavy && a->charge_t < 12) {
                        a->running = true;
                        /* long things are dropped when you break into a run */
                        if (a->carry >= 0 && (dk.it[a->carry].kind == IT_PLANK || dk.it[a->carry].kind == IT_SAW)) drop_carry(a);
                    }
                } else if (a->charge_t < 12) {
                    dku_set_state(a, AS_DODGE);
                    a->dodge_dir = pdir == 4 ? 1 : -1;
                    a->combo = 0;
                    dku_sfx("dku_dodge");
                    a->tapdir = 0;
                    break;
                }
                a->tapdir = 0;
            } else {
                a->tapdir = pdir;
                a->tap_t = 0;
            }
        }
        if (a->running && (hdir == 0 || hdir != a->face)) a->running = false;
        int sx = a->running ? 32 : 14, sy = a->running ? 12 : 10;
        if (a->charge_t >= 12) { sx = 9; sy = 7; }
        if (a->carry >= 0 && dk.it[a->carry].kind == IT_BIN) { sx = 10; sy = 8; }
        a->vx = hdir * sx;
        a->vy = vdir * sy;
        if (hdir && a->charge_t < 12) a->face = hdir;
        if (hdir || vdir) { a->step++; a->anim = dk.frame_t; }
        /* walking into a stunned ghoul grabs it */
        if (hdir && a->carry < 0) {
            for (int j = 2; j < DKU_MAX_ACTORS; j++) {
                Actor *o = &dk.a[j];
                if (!grabbable(o)) continue;
                int dx = dku_px(o->x - a->x) * hdir, dy = iabs(dku_px(o->y - a->y));
                if (dx > 0 && dx <= 14 && dy <= 5) { start_grab(a, i, j); break; }
            }
            if (a->state == AS_GRAB) break;
        }
        if (press & BTN_A) {
            if (a->carry >= 0) {
                if (a->ready == 0) use_carry(a, i);
            } else {
                int ii = item_under(a);
                if (ii >= 0 && !a->running) take_item(a, ii);
                else if (a->running) { dku_start_attack(a, AT_DASH); a->running = false; dku_sfx("dku_dash"); }
                else start_jab(a);
            }
        } else if (press & BTN_B) {
            if (a->carry >= 0) throw_carry(a, i);
            else if (a->charge_t < 12) {
                dku_set_state(a, AS_AIR);
                a->air_kicked = false;
                a->air_jump = false;
                a->tx = hdir * (a->running ? 30 : 14); /* the momentum it leaves with */
                a->vx = 0;
                a->vy = 0;
            }
        }
        if ((rel & BTN_A)) {
            if (a->charged && a->state == AS_FREE) {
                dku_start_attack(a, AT_CHARGED);
                dku_sfx("dku_charged");
            }
            a->charge_t = 0;
            a->charged = false;
        }
        break;
    }
    case AS_DODGE: {
        if (a->st <= 8) {
            a->vy = a->dodge_dir * 48;
            a->vx = 0;
        } else {
            a->vy = 0;
        }
        if (a->st > 8 + 10) dku_set_state(a, AS_FREE);
        break;
    }
    case AS_AIR: {
        if (a->st == 3) { a->vz = 52; a->z = 1; a->vx = a->tx; dku_sfx("dku_jump"); }
        if (a->st < 3) { a->vx = 0; break; }
        if ((press & BTN_A) && !a->air_kicked) {
            a->air_kicked = true;
            a->atk = AT_FLYKICK;
            a->atk_t = 0;
            a->nhits = 0;
            dku_sfx("dku_kick");
        }
        if ((press & BTN_B) && a->air_jump) {
            /* PIP: a kick that landed earns another jump, any way */
            a->air_jump = false;
            a->air_kicked = false;
            a->atk = AT_NONE;
            a->vz = 48;
            a->vx = hdir * 20;
            if (hdir) a->face = hdir;
            a->vy = vdir * 10;
            dku_sfx("dku_jump");
        }
        if (a->atk == AT_FLYKICK) dku_attack_update(a, i);
        if (a->st > 3 && physics(a)) {
            dku_set_state(a, AS_FREE);
            a->land_t = 4;
            a->vx = a->vy = 0;
            a->air_jump = false;
        }
        clamp_view(a);
        return;
    }
    case AS_ATTACK: {
        if (a->atk == AT_NONE) {
            /* throwing, shooting, landing a slam: just the recovery */
            a->vx = a->vy = 0;
            if (a->st >= imax(1, a->timer2)) dku_set_state(a, AS_FREE);
            break;
        }
        int prev_atk = a->atk;
        dku_attack_update(a, i);
        if (a->timer2 == -1 && (prev_atk == AT_JAB || prev_atk == AT_PUMMEL)) {
            /* the kill-kick */
            a->timer2 = 0;
            dku_start_attack(a, AT_KICK);
            a->atk_t = DKU_ATK[AT_KICK].startup;
            a->combo = 0;
            dku_sfx("dku_kick");
            break;
        }
        if (!(DKU_ATK[a->atk].flags & AF_MOVE)) { a->vx = 0; }
        a->vy = 0;
        const DkuAtk *d = &DKU_ATK[a->atk];
        int recover = d->recover;
        if (a->atk == AT_CHARGED || a->atk == AT_DASH) recover -= 6 * (dku_stat(a->player, DK_RECOV) - 1);
        if (a->atk == AT_SWING || a->atk == AT_SAW) recover -= 3 * (dku_stat(a->player, DK_RECOV) - 1);
        int total = d->startup + d->active + recover;
        /* the next punch can be pressed during the last one */
        if ((press & BTN_A) && (a->atk == AT_JAB || a->atk == AT_KICK) && a->atk_t > d->startup) a->timer2 = 1;
        if (a->atk_t > d->startup + d->active && (DKU_ATK[a->atk].flags & AF_MOVE)) a->vx = a->vx * 3 / 4;
        if (a->atk_t >= total) {
            bool queued = a->timer2 == 1 && (a->atk == AT_JAB);
            a->timer2 = 0;
            dku_set_state(a, AS_FREE);
            a->combo_t = 0;
            if (queued && a->carry < 0) start_jab(a);
        }
        if ((rel & BTN_A)) {
            a->charge_t = 0;
            a->charged = false;
        }
        break;
    }
    case AS_SPIN: {
        a->atk = AT_SPIN;
        dku_attack_update(a, i);
        a->vx = a->vy = 0;
        const DkuAtk *d = &DKU_ATK[AT_SPIN];
        if (a->atk_t >= d->startup + d->active + d->recover) dku_set_state(a, AS_FREE);
        break;
    }
    case AS_GRAB: {
        Actor *t = a->partner >= 0 ? &dk.a[a->partner] : NULL;
        if (!t || !t->alive || t->state != AS_GRABBED) { a->partner = -1; dku_set_state(a, AS_FREE); break; }
        int th = dku_stat(a->player, DK_THROW);
        /* walk with it */
        a->vx = hdir * 8;
        a->vy = vdir * 6;
        if (hdir) a->face = hdir;
        t->x = a->x + a->face * dku_fx(13);
        t->y = a->y;
        t->face = -a->face;
        int pick = dk.pr[a->player].pick;
        if (a->st > 50 + 30 * th) {
            /* it wriggles free */
            dku_set_state(t, AS_FREE);
            t->partner = -1;
            t->cool = 20;
            a->partner = -1;
            dku_set_state(a, AS_HURT);
            a->stun = 8;
            break;
        }
        if (pick == DK_DOLLY && (held & BTN_A) && a->st > 1) {
            if (++a->grab_charge == charge_time(a->player)) dku_sfx("dku_charge");
        }
        if (pick == DK_ROOK && (held & BTN_B)) {
            if (++a->grab_charge == charge_time(a->player)) dku_sfx("dku_charge");
        }
        if (press & BTN_A) {
            if (hdir) {
                dku_throw_actor(i, hdir, false);
                break;
            }
        }
        if ((press & BTN_A) && a->jab_t >= 12) {
            /* a pummel */
            a->jab_t = 0;
            a->atk = AT_PUMMEL;
            a->nhits = 0;
            int dmg = dku_stat(a->player, DK_POWER);
            bool was = t->hp > 0;
            dku_hit(a->partner, i, AT_PUMMEL, dmg, a->face, 0);
            a->atk = AT_NONE;
            dku_sfx("dku_jab");
            if (was && t->hp <= 0) {
                dku_start_attack(a, AT_KICK);
                a->atk_t = DKU_ATK[AT_KICK].startup;
                dk.kick_kills++;
                break;
            }
            if (t->state == AS_GRABBED) t->stun = 0;
        }
        if ((rel & BTN_A) && pick == DK_DOLLY) {
            if (a->grab_charge >= charge_time(a->player)) { dku_throw_actor(i, hdir ? hdir : a->face, true); break; }
            a->grab_charge = 0;
        }
        if (pick == DK_ROOK) {
            if (rel & BTN_B) { start_suplex(a, a->grab_charge >= charge_time(a->player)); break; }
        } else if (press & BTN_B) {
            start_suplex(a, false);
            break;
        }
        break;
    }
    case AS_SUPLEX: {
        a->vx = hdir * 16;
        a->vy = vdir * 8;
        if (hdir) a->face = hdir;
        bool landed = physics(a);
        if (a->partner >= 0) {
            Actor *t = &dk.a[a->partner];
            t->x = a->x - a->face * dku_fx(2);
            t->y = a->y;
            t->z = a->z + dku_fx(20);
        }
        clamp_view(a);
        if (landed) suplex_land(a, i);
        return;
    }
    default: break;
    }
    physics(a);
    clamp_view(a);
}

void dku_fighters_update(void) {
    for (int i = 0; i < 2; i++)
        if (dk.a[i].alive && dk.a[i].kind == AK_FIGHTER) fighter_update(i);
}
