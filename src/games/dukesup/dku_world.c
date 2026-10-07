/* DUKES UP - the world: a night's sections and their scripts, the camera
 * that only moves on, spawning by scroll position, lock-screens, the lift,
 * the gym's waves, hazards, flying things, things on the floor and blasts.
 * Integer arithmetic only. */
#include "dku.h"

#define GRAV 4

static const DkuSection *cur_sec(void) {
    if (dku_g.gym) return &DKU_GYM;
    return &DKU_NIGHT[dku_g.night].sec[dku_g.sec];
}

int dku_floor_lo(void) {
    const DkuSection *s = cur_sec();
    return s->kind == SEC_LIFT ? 126 : DKU_FLOOR0;
}
int dku_floor_hi(void) { return DKU_FLOOR1; }
int dku_view_left(void) { return dku_g.cam + (cur_sec()->kind == SEC_LIFT ? 70 : 0); }
int dku_view_right(void) { return dku_g.cam + (cur_sec()->kind == SEC_LIFT ? 250 : SCREEN_W); }

/* ---- small helpers ------------------------------------------------------------- */

void dku_burst(int x, int y, int z, int col, int n) {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < DKU_MAX_PARTS; i++) {
            Part *p = &dku_g.part[i];
            if (p->life > 0) continue;
            p->x = x;
            p->y = y;
            p->z = z + dku_fx(4);
            p->vx = rng_range(&dku_g.fx, -24, 24);
            p->vy = rng_range(&dku_g.fx, -6, 6);
            p->vz = rng_range(&dku_g.fx, 8, 30);
            p->life = rng_range(&dku_g.fx, 12, 26);
            p->col = col;
            break;
        }
    }
}

bool dku_in_pit(int x, int y) {
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        const Hazard *h = &dku_g.hz[i];
        if (!h->alive || (h->kind != HZ_PIT && h->kind != HZ_WATER)) continue;
        if (x >= h->x && x < h->x + h->w && y >= h->y && y < h->y + h->h) return true;
    }
    return false;
}

int dku_hazard_at(int kind, int x, int y) {
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        const Hazard *h = &dku_g.hz[i];
        if (!h->alive || h->kind != kind) continue;
        if (x >= h->x && x < h->x + h->w && y >= h->y && y < h->y + h->h) return i;
    }
    return -1;
}

void dku_add_hazard(int kind, int x, int y, int w, int h, int t, int arg) {
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        Hazard *z = &dku_g.hz[i];
        if (z->alive) continue;
        memset(z, 0, sizeof *z);
        z->alive = 1;
        z->kind = (uint8_t)kind;
        z->x = x;
        z->y = y;
        z->w = w;
        z->h = h;
        z->t = t;
        z->arg = arg;
        return;
    }
}

int dku_add_shot(int kind, int team, int x, int y, int z, int vx, int vy, int vz, int dmg, int owner) {
    for (int i = 0; i < DKU_MAX_SHOTS; i++) {
        Shot *s = &dku_g.sh[i];
        if (s->alive) continue;
        memset(s, 0, sizeof *s);
        s->alive = 1;
        s->kind = (uint8_t)kind;
        s->team = (uint8_t)team;
        s->x = x;
        s->y = y;
        s->z = z;
        s->vx = vx;
        s->vy = vy;
        s->vz = vz;
        s->dmg = dmg;
        s->owner = owner;
        return i;
    }
    return -1;
}

int dku_drop_item(int kind, int x, int y) {
    for (int i = 0; i < DKU_MAX_ITEMS; i++) {
        Item *it = &dku_g.it[i];
        if (it->alive) continue;
        memset(it, 0, sizeof *it);
        it->alive = 1;
        it->kind = (uint8_t)kind;
        it->x = dku_fx(x);
        it->y = dku_fx(iclamp(y, dku_floor_lo(), dku_floor_hi()));
        it->z = dku_fx(6);
        it->vz = 18;
        it->ammo = kind == IT_SCATTER ? 6 : 0;
        return i;
    }
    return -1;
}

static int place_prop(int kind, int x, int y, int c1, int c2) {
    for (int i = 0; i < DKU_MAX_PROPS; i++) {
        Prop *p = &dku_g.pr_[i];
        if (p->alive) continue;
        memset(p, 0, sizeof *p);
        p->alive = 1;
        p->kind = (uint8_t)kind;
        p->x = dku_fx(x);
        p->y = dku_fx(y);
        p->content = (uint8_t)c1;
        p->content2 = (uint8_t)c2;
        static const int HP[PR_COUNT] = {1, 1, 2, 2, 1, 1, 2, 1, 1};
        p->hp = HP[kind];
        return i;
    }
    return -1;
}

/* A blast: everyone near is floored. hurts_all: fighters and ghouls alike;
 * otherwise only the side that isn't src_team. */
void dku_explode(int x, int y, int r, int dmg, int src_team, bool hurts_all) {
    for (int j = 0; j < DKU_MAX_ACTORS; j++) {
        Actor *o = &dku_g.a[j];
        if (!o->alive || o->state == AS_DEAD || o->state == AS_FALL || o->state == AS_GONE) continue;
        if (o->kind == AK_SAUCER || o->kind == AK_PASSER || o->state == AS_LEASHED) continue;
        if (!hurts_all && o->team == src_team) continue;
        if (o->team == 2) continue;
        int dx = iabs(dku_px(o->x - x)), dy = iabs(dku_px(o->y - y));
        if (dx > r || dy * 2 > r || o->z > dku_fx(26)) continue;
        int dir = o->x >= x ? 1 : -1;
        if (o->kind == AK_UNDERTOW) { dku_hit(j, -1, AT_NONE, dmg / 2, dir, AF_KNOCK | 64); continue; }
        dku_hit(j, -1, AT_NONE, dmg, dir, AF_KNOCK | 128);
    }
    for (int i = 0; i < DKU_MAX_PROPS; i++) {
        Prop *p = &dku_g.pr_[i];
        if (!p->alive || p->kind == PR_POST) continue;
        if (iabs(dku_px(p->x - x)) <= r && iabs(dku_px(p->y - y)) * 2 <= r) {
            p->alive = 0;
            if (p->content) dku_drop_item(p->content, dku_px(p->x), dku_px(p->y));
            if (p->content2) dku_drop_item(p->content2, dku_px(p->x) + 6, dku_px(p->y));
        }
    }
    dku_burst(x, y, dku_fx(4), C_ORANGE, 10);
    dku_burst(x, y, dku_fx(8), C_YELLOW, 6);
    dku_g.shake = imax(dku_g.shake, 8);
    dku_sfx("dku_boom");
}

int dku_enemies_alive(void) {
    int n = 0;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        const Actor *a = &dku_g.a[i];
        if (a->alive && a->team == 1 && a->state != AS_DEAD && a->state != AS_FALL && a->state != AS_GONE) n++;
    }
    return n;
}

int dku_enemies_awake(void) {
    int n = 0;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        const Actor *a = &dku_g.a[i];
        if (a->alive && a->team == 1 && a->state != AS_DEAD && a->state != AS_FALL && a->state != AS_GONE &&
            a->state != AS_DORMANT)
            n++;
    }
    return n;
}

/* the ones a lock-screen waits for: awake, or asleep inside the view */
static int enemies_here(void) {
    int n = 0, r = dku_view_right() + 16;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        const Actor *a = &dku_g.a[i];
        if (!a->alive || a->team != 1 || a->state == AS_DEAD || a->state == AS_FALL || a->state == AS_GONE) continue;
        if ((a->state == AS_DORMANT || a->kind == AK_FEELER) && dku_px(a->x) > r) continue;
        n++;
    }
    return n;
}

void dku_clear_enemies(void) {
    for (int i = 2; i < DKU_MAX_ACTORS; i++)
        if (dku_g.a[i].alive && dku_g.a[i].team == 1) dku_g.a[i].alive = 0;
    for (int s = 0; s < DKU_MAX_SHOTS; s++) if (dku_g.sh[s].team == 1) dku_g.sh[s].alive = 0;
}

/* ---- spawning ------------------------------------------------------------------- */

static int boss_hp(int kind) {
    switch (kind) {
    case AK_RAMMER: return 90;
    case AK_TUSKER: return 120;
    case AK_VISITOR: return 40;
    case AK_UNDERTOW: return 48;
    case AK_GRIST: return 110;
    default: return DKU_KINDS[kind].hp;
    }
}

int dku_spawn(int kind, int from, int x, int y, int mode) {
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        Actor *a = &dku_g.a[i];
        if (a->alive) continue;
        memset(a, 0, sizeof *a);
        a->alive = 1;
        a->kind = (uint8_t)kind;
        a->team = kind == AK_PASSER || kind == AK_SAUCER ? 2 : kind == AK_DOG ? 2 : 1;
        a->hp = a->maxhp = DKU_KINDS[kind].hp;
        a->partner = a->carry = -1;
        a->thrown_by = -1;
        a->player = -1;
        a->face = -1;
        a->from = (uint8_t)from;
        a->mode = (uint8_t)mode;
        a->y = dku_fx(iclamp(y, dku_floor_lo(), dku_floor_hi()));
        a->cool = rng_range(&dku_g.rng, 20, 60);
        a->timer2 = 0;
        switch (from) {
        case FROM_LEFT:
            a->x = dku_fx(dku_view_left() - 20 - rng_range(&dku_g.rng, 0, 10));
            a->face = 1;
            a->state = AS_ENTER;
            break;
        case FROM_RIGHT:
            a->x = dku_fx(dku_view_right() + 20 + rng_range(&dku_g.rng, 0, 10));
            a->state = AS_ENTER;
            break;
        case FROM_GROUND:
            a->x = dku_fx(x);
            a->state = AS_RISE;
            break;
        case FROM_ABOVE:
            a->x = dku_fx(x);
            a->z = dku_fx(90);
            a->state = AS_RISE;
            break;
        case FROM_WATER:
            a->x = dku_fx(x);
            a->y = dku_fx(y);
            a->vz = 46;
            a->z = 1;
            a->vy = (dku_fx(dku_floor_lo() + 10) - a->y) / 22;
            if (a->vy < 0) a->vy = (dku_fx(dku_floor_hi() - 10) - a->y) / 22;
            a->vx = -8;
            a->state = AS_RISE;
            dku_burst(a->x, a->y, 0, C_SKY, 6);
            dku_sfx("dku_splash");
            break;
        case FROM_DOOR:
            a->x = dku_fx(x);
            a->y = dku_fx(dku_floor_lo());
            a->state = AS_FREE;
            break;
        case FROM_ROOF:
            a->x = dku_fx(x);
            a->y = dku_fx(dku_floor_lo() - 2);
            a->z = dku_fx(52);
            a->state = AS_RISE;
            break;
        default:
            a->x = dku_fx(x);
            a->state = mode ? AS_DORMANT : AS_FREE;
            break;
        }
        if (kind == AK_DOG) a->state = AS_LEASHED;
        if (kind == AK_SAUCER) { a->z = dku_fx(60); a->state = AS_FREE; a->x = dku_fx(dku_view_right() + 16); a->y = dku_fx(DKU_FLOORMID); }
        if (kind == AK_PASSER) { a->x = dku_fx(dku_view_right() + 10); a->state = AS_FREE; }
        if (kind == AK_FEELER || kind == AK_UNDERTOW) a->state = AS_FREE;
        return i;
    }
    return -1;
}

static int spawn_boss(int kind, int x, int y) {
    int i = dku_spawn(kind, FROM_AT, x, y, 0);
    if (i < 0) return -1;
    Actor *a = &dku_g.a[i];
    a->boss = 1;
    a->hp = a->maxhp = boss_hp(kind);
    a->cool = 60;
    if (kind == AK_UNDERTOW) { a->y = dku_fx(y); a->z = 0; }
    dku_g.boss = i;
    return i;
}

/* ---- fighters -------------------------------------------------------------------- */

static void place_fighters(void) {
    for (int p = 0; p < 2; p++) {
        Actor *a = &dku_g.a[p];
        int hp = a->hp;
        bool was = a->alive && a->kind == AK_FIGHTER;
        bool out = was && a->state == AS_GONE;
        memset(a, 0, sizeof *a);
        if (p >= dku_g.players) continue;
        a->alive = 1;
        a->kind = AK_FIGHTER;
        a->team = 0;
        a->player = p;
        a->hp = a->maxhp = DKU_MAX_HP;
        if (was) a->hp = hp;
        a->partner = a->carry = -1;
        a->thrown_by = -1;
        a->face = 1;
        a->x = dku_fx(dku_view_left() + 44 + p * 22);
        a->y = dku_fx(DKU_FLOORMID - 8 + p * 18);
        a->state = out ? AS_GONE : AS_FREE;
    }
}

void dku_new_run(int players) {
    int menu = dku_g.menu, sel0 = dku_g.sel[0], sel1 = dku_g.sel[1];
    DkuPlayerRun pr0 = dku_g.pr[0], pr1 = dku_g.pr[1];
    Rng keep = dku_g.rng, keepfx = dku_g.fx;
    memset(&dku_g, 0, sizeof dku_g);
    dku_g.rng = keep;
    dku_g.fx = keepfx;
    dku_g.menu = menu;
    dku_g.sel[0] = sel0;
    dku_g.sel[1] = sel1;
    dku_g.pr[0] = pr0;
    dku_g.pr[1] = pr1;
    dku_g.players = players;
    for (int p = 0; p < 2; p++)
        for (int s = 0; s < DK_NSTATS; s++) dku_g.pr[p].stat[s] = DKU_FIGHTERS[dku_g.pr[p].pick].stat[s];
    dku_g.boss = -1;
    dku_g.cam_lock = -1;
    for (int p = 0; p < 2; p++) { dku_g.a[p].alive = 0; }
}

static void clear_world(bool keep_dogs) {
    for (int i = 2; i < DKU_MAX_ACTORS; i++)
        if (!(keep_dogs && dku_g.a[i].alive && dku_g.a[i].kind == AK_DOG && dku_g.a[i].team == 0)) dku_g.a[i].alive = 0;
    memset(dku_g.it, 0, sizeof dku_g.it);
    memset(dku_g.pr_, 0, sizeof dku_g.pr_);
    memset(dku_g.sh, 0, sizeof dku_g.sh);
    memset(dku_g.hz, 0, sizeof dku_g.hz);
    memset(dku_g.part, 0, sizeof dku_g.part);
}

static void fire_event(const DkuEvt *e);

void dku_start_section(int s) {
    dku_g.sec = s;
    dku_g.cam = 0;
    dku_g.cam_lock = -1;
    dku_g.sec_t = 0;
    dku_g.go_t = 0;
    dku_g.exit_t = 0;
    dku_g.boss = -1;
    dku_g.boss_down = false;
    dku_g.night_done = false;
    dku_g.lift_wave = 0;
    dku_g.lift_t = 0;
    dku_g.stream_kind = -1;
    dku_g.stream_t = 0;
    dku_g.dog_t = 0;
    dku_g.dogs_came = false;
    dku_g.thresher_stop = 0;
    memset(dku_g.ev_done, 0, sizeof dku_g.ev_done);
    clear_world(true);
    for (int p = 0; p < 2; p++) {
        Actor *a = &dku_g.a[p];
        if (!a->alive) continue;
        a->carry = -1;
        a->partner = -1;
        if (a->state == AS_DEAD || a->state == AS_FALL) a->state = AS_GONE; /* out for the rest of the night */
        if (a->state != AS_DEAD && a->state != AS_GONE && a->state != AS_FALL) {
            a->state = AS_FREE;
            a->z = a->vz = a->vx = a->vy = 0;
            a->x = dku_fx(dku_view_left() + 44 + p * 22);
            a->y = dku_fx(DKU_FLOORMID - 8 + p * 18);
            a->face = 1;
        }
    }
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        Actor *d = &dku_g.a[i];
        if (d->alive && d->kind == AK_DOG) { d->x = dku_fx(dku_view_left() + 20); d->y = dku_fx(DKU_FLOORMID); d->state = AS_FREE; }
    }
    const DkuSection *sec = cur_sec();
    for (int k = 0; k < sec->nev; k++) {
        if (sec->ev[k].at <= 0 && sec->ev[k].op != EV_WAVE) {
            dku_g.ev_done[k] = true;
            fire_event(&sec->ev[k]);
        }
    }
    if (sec->kind == SEC_LIFT) { dku_g.lift_wave = 0; dku_g.lift_t = 40; }
}

void dku_start_night(int night) {
    dku_g.night = night;
    dku_g.sec = 0;
    dku_g.gym = false;
    for (int p = 0; p < dku_g.players; p++) {
        Actor *a = &dku_g.a[p];
        if (!a->alive || a->kind != AK_FIGHTER) {
            a->alive = 1;
            a->kind = AK_FIGHTER;
            a->hp = DKU_MAX_HP;
        }
        if (a->hp <= 0 || a->state == AS_DEAD || a->state == AS_GONE || a->state == AS_FALL) {
            /* a partner who fell only comes back if the shop's soup was bought
               for them (it brings them back at full health); otherwise still out */
            if (a->hp > 0) a->state = AS_FREE;
            else { a->hp = 0; a->state = AS_GONE; }
        }
    }
    dku_g.cam = 0;
    place_fighters();
    for (int p = 0; p < dku_g.players; p++) dku_g.start_hp[p] = dku_g.a[p].hp;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) dku_g.a[i].alive = 0;
    dku_start_section(0);
}

void dku_start_gym(void) {
    dku_g.gym = true;
    dku_g.gym_wave = 1;
    dku_g.gym_break = 120;
    dku_g.gym_nq = 0;
    dku_g.gym_spawned = 0;
    dku_g.cam = 0;
    for (int p = 0; p < dku_g.players; p++) dku_g.a[p].hp = imax(dku_g.a[p].hp, 1);
    dku_start_section(0);
    for (int p = 0; p < dku_g.players; p++) {
        dku_g.a[p].x = dku_fx(150 + p * 20);
        dku_g.a[p].y = dku_fx(DKU_FLOORMID + p * 10);
    }
}

/* ---- the script ---------------------------------------------------------------------- */

static void fire_event(const DkuEvt *e) {
    switch (e->op) {
    case EV_SPAWN: {
        int i = dku_spawn(e->a, e->b, e->x, e->y, e->c);
        (void)i;
        break;
    }
    case EV_PROP: place_prop(e->a, e->x, e->y, e->b, e->c); break;
    case EV_ITEM: { int i = dku_drop_item(e->a, e->x, e->y); if (i >= 0) { dku_g.it[i].z = 0; dku_g.it[i].vz = 0; } break; }
    case EV_LOCK: dku_g.cam_lock = e->at; break;
    case EV_PIT: dku_add_hazard(HZ_PIT, e->x, e->y, e->w, e->h, 0, 0); break;
    case EV_WATER: dku_add_hazard(HZ_WATER, e->x, e->y, e->w, e->h, 0, 0); break;
    case EV_MINE: dku_add_hazard(HZ_MINE, e->x - 5, e->y - 3, 10, 6, 0, 0); break;
    case EV_LAMP: {
        dku_add_hazard(HZ_LAMP, e->x, e->y, 36, 12, 0, 1);
        for (int i = DKU_MAX_HAZARDS - 1; i >= 0; i--)
            if (dku_g.hz[i].alive && dku_g.hz[i].kind == HZ_LAMP && dku_g.hz[i].x == e->x && dku_g.hz[i].y == e->y) { dku_g.hz[i].arg2 = e->b; break; }
        break;
    }
    case EV_FIREWALL: dku_add_hazard(HZ_FIREWALL, e->x, e->y, e->w, e->h, 0, 0); break;
    case EV_CAR: dku_add_hazard(HZ_CAR, dku_view_left() - 70, e->y, 56, 14, 0, 0); dku_sfx("dku_horn"); break;
    case EV_THRESHER: dku_add_hazard(HZ_THRESHER, dku_view_left() - 80, DKU_FLOOR0, 64, DKU_FLOOR1 - DKU_FLOOR0, 0, e->x); break;
    case EV_SAUCER: dku_spawn(AK_SAUCER, FROM_AT, 0, 0, 0); dku_sfx("dku_saucer"); break;
    case EV_DOG: {
        place_prop(PR_POST, e->x - 10, e->y - 1, 0, 0);
        dku_spawn(AK_DOG, FROM_AT, e->x, e->y, 0);
        break;
    }
    case EV_PASSER: {
        int i = dku_spawn(AK_PASSER, FROM_RIGHT, 0, e->y, 1);
        if (i >= 0) { dku_g.a[i].ammo = e->b; dku_g.a[i].mode = 1; }
        break;
    }
    case EV_BOSS: {
        spawn_boss(e->a, e->x, e->y);
        if (e->a == AK_VISITOR) {
            /* the visitors come in a pair */
            int j = spawn_boss(AK_VISITOR, e->x + 40, e->y + 20);
            (void)j;
        }
        break;
    }
    case EV_BEAM: dku_add_hazard(HZ_BEAM, e->x, DKU_FLOOR0, 30, DKU_FLOOR1 - DKU_FLOOR0, 0, 0); break;
    case EV_DOOR: dku_add_hazard(HZ_DOOR, e->x, DKU_FLOOR0 - 40, 26, 40, 0, e->b); break;
    case EV_STREAM:
        dku_g.stream_kind = e->a;
        dku_g.stream_every = e->b * 10;
        dku_g.stream_max = e->c;
        dku_g.stream_t = dku_g.stream_every;
        break;
    default: break;
    }
}

static void lift_update(void) {
    const DkuSection *sec = cur_sec();
    if (dku_g.lift_wave > 5) return;
    /* the next wave lands on the roof while the last of this one is still
       fighting (the first of them may drop in early); after the fifth,
       nobody may be left */
    int roof = 0, down = 0;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        const Actor *o = &dku_g.a[i];
        if (!o->alive || o->team != 1 || o->state == AS_DEAD || o->state == AS_FALL || o->state == AS_GONE) continue;
        if (o->state == AS_RISE && o->from == FROM_ROOF) roof++;
        else down++;
    }
    if (roof > 0 || down > (dku_g.lift_wave >= 1 && dku_g.lift_wave < 5 ? 1 : 0)) return;
    if (dku_g.lift_t > 0) { dku_g.lift_t--; return; }
    dku_g.lift_wave++;
    if (dku_g.lift_wave > 5) { dku_g.exit_t = 1; dku_sfx("dku_ding"); return; }
    /* the next wave lands on the lift's roof, and they drop in turn */
    int n = 0;
    for (int k = 0; k < sec->nev; k++) {
        const DkuEvt *e = &sec->ev[k];
        if (e->op != EV_WAVE || e->a != dku_g.lift_wave) continue;
        for (int c = 0; c < e->c; c++) {
            int x = 96 + ((n * 37) % 130);
            int i = dku_spawn(e->b, FROM_ROOF, x, 0, 0);
            if (i >= 0) { dku_g.a[i].timer2 = 50 + n * 40; dku_g.a[i].face = -1; }
            n++;
        }
    }
    dku_g.lift_t = 60;
    dku_sfx("dku_ding");
}

static void gym_update(void) {
    if (dku_g.gym_break > 0) {
        dku_g.gym_break--;
        if (dku_g.gym_break == 0) {
            uint8_t q[24];
            int n = 0;
            dku_gym_wave(dku_g.gym_wave, q, &n, 24);
            memcpy(dku_g.gym_queue, q, sizeof q);
            dku_g.gym_nq = n;
            dku_g.gym_spawned = 0;
            dku_g.stream_t = 0;
            /* every third wave, frightened passers-by run through first */
            if (dku_g.gym_wave % 3 == 0) {
                for (int k = 0; k < 3; k++) {
                    int i = dku_spawn(AK_PASSER, FROM_RIGHT, 0, DKU_FLOOR0 + 12 + k * 20, 0);
                    if (i >= 0) { dku_g.a[i].mode = 0; dku_g.a[i].x += dku_fx(k * 30); }
                }
            }
            dku_sfx("dku_whistle");
        }
        return;
    }
    if (dku_g.gym_spawned < dku_g.gym_nq) {
        if (dku_g.gym_spawned == 0) {
            /* a wave's first four come in together, from both sides */
            int first = imin(4, dku_g.gym_nq);
            for (int k = 0; k < first; k++) {
                int side = (k & 1) ? FROM_LEFT : FROM_RIGHT;
                dku_spawn(dku_g.gym_queue[k], side, 0, DKU_FLOOR0 + 8 + rng_range(&dku_g.rng, 0, DKU_FLOOR1 - DKU_FLOOR0 - 16), 0);
            }
            dku_g.gym_spawned = first;
            dku_g.stream_t = 60;
            return;
        }
        if (--dku_g.stream_t <= 0 && dku_enemies_alive() < 5) {
            int k = dku_g.gym_queue[dku_g.gym_spawned];
            int side = (dku_g.gym_spawned & 1) ? FROM_LEFT : FROM_RIGHT;
            dku_spawn(k, side, 0, DKU_FLOOR0 + 8 + rng_range(&dku_g.rng, 0, DKU_FLOOR1 - DKU_FLOOR0 - 16), 0);
            dku_g.gym_spawned++;
            dku_g.stream_t = 40;
        }
        return;
    }
    if (dku_enemies_alive() == 0) {
        /* the wave is beaten */
        int beaten = dku_g.gym_wave;
        uint16_t *best = dku_g.players == 2 ? &dku_sv.gym_best2 : &dku_sv.gym_best1;
        if (beaten > *best) *best = (uint16_t)beaten;
        if (beaten >= DKU_GYM_GIFT) dku_award_gym();
        dku_save_now();
        dku_g.gym_wave++;
        dku_g.gym_break = 150;
        dku_sfx("dku_bell");
    }
}

/* survival: wave n's line-up (ours: more and tougher as it goes) */
void dku_gym_wave(int wave, uint8_t *kinds, int *n, int max) {
    static const uint8_t UNLOCK[] = {AK_SHAMBLER, AK_TORCH, AK_SKIPPER, AK_GIGGLER, AK_CROW, AK_HOWLER, AK_BULWARK,
                                     AK_RAMMER, AK_VISITOR, AK_SLUDGER, AK_TUSKER};
    int count = imin(max, imin(16, 3 + wave * 3 / 4));
    int pool = imin(ARRAY_LEN(UNLOCK), 2 + (wave - 1) * 2 / 3);
    Rng r;
    rng_seed(&r, (uint64_t)(1000 + wave * 7));
    for (int k = 0; k < count; k++) {
        int pick = rng_range(&r, 0, pool - 1);
        /* the newest kind always shows up once */
        if (k == 0 && wave >= 3) pick = pool - 1;
        kinds[k] = UNLOCK[pick];
    }
    *n = count;
}

bool dku_section_cleared(void) { return dku_enemies_alive() == 0; }

/* ---- hazards, shots, items -------------------------------------------------------------- */

static void hazards_update(void) {
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        Hazard *h = &dku_g.hz[i];
        if (!h->alive) continue;
        switch (h->kind) {
        case HZ_MINE:
            for (int j = 0; j < DKU_MAX_ACTORS; j++) {
                Actor *a = &dku_g.a[j];
                if (!a->alive || a->z > 0 || a->state == AS_DEAD || a->state == AS_DORMANT || a->state == AS_GONE) continue;
                if (a->kind == AK_SAUCER || a->state == AS_LEASHED) continue;
                int x = dku_px(a->x), y = dku_px(a->y);
                if (x >= h->x && x < h->x + h->w && y >= h->y && y < h->y + h->h) {
                    h->alive = 0;
                    dku_explode(dku_fx(h->x + 5), dku_fx(h->y + 3), 26, 18, 2, true);
                    break;
                }
            }
            break;
        case HZ_LAMP:
            if (h->t == 0 && h->arg == 1) {
                for (int p = 0; p < 2; p++) {
                    const Actor *a = &dku_g.a[p];
                    if (!a->alive || a->state == AS_DEAD) continue;
                    int dx = dku_px(a->x) - (h->x), dy = dku_px(a->y) - (h->y);
                    if (dx * dx * 36 + dy * dy * 324 <= 18 * 18 * 36) { h->t = 34; dku_sfx("dku_creak"); }
                }
            } else if (h->t > 0) {
                if (--h->t == 0) {
                    /* it comes down on whoever stands in its shadow */
                    for (int j = 0; j < DKU_MAX_ACTORS; j++) {
                        Actor *a = &dku_g.a[j];
                        if (!a->alive || a->state == AS_DEAD || a->team == 2) continue;
                        if (h->arg == 2 && a->team != 0) continue;
                        int dx = dku_px(a->x) - h->x, dy = dku_px(a->y) - h->y;
                        if (iabs(dx) <= h->w / 2 + 2 && iabs(dy) <= h->h / 2 + 2 && a->z < dku_fx(30))
                            dku_hit(j, -1, AT_NONE, h->arg == 2 ? 10 : 22, dx >= 0 ? 1 : -1, AF_KNOCK | 128);
                    }
                    dku_g.shake = 8;
                    dku_burst(dku_fx(h->x), dku_fx(h->y), 0, h->arg == 2 ? C_TEAL : C_YELLOW, 10);
                    dku_sfx(h->arg == 2 ? "dku_slam" : "dku_crash");
                    if (h->arg2) dku_drop_item(h->arg2, h->x, h->y);
                    h->alive = 0;
                }
            }
            break;
        case HZ_FIRE: case HZ_FIREWALL: case HZ_CLOUD: {
            if (h->kind != HZ_FIREWALL && --h->t <= 0) { h->alive = 0; break; }
            h->arg2++;
            int every = h->kind == HZ_CLOUD ? 12 : 15;
            if (h->arg2 % every) break;
            for (int j = 0; j < DKU_MAX_ACTORS; j++) {
                Actor *a = &dku_g.a[j];
                if (!a->alive || a->state == AS_DEAD || a->state == AS_FALL || a->z > dku_fx(10) || a->team == 2) continue;
                if (h->kind == HZ_CLOUD && a->team != 0) continue;
                if (a->kind == AK_UNDERTOW) continue;
                int x = dku_px(a->x), y = dku_px(a->y);
                bool in;
                if (h->kind == HZ_CLOUD) in = iabs(x - h->x) <= h->w && iabs(y - h->y) * 2 <= h->w;
                else in = x >= h->x && x < h->x + h->w && y >= h->y && y < h->y + h->h;
                if (!in) continue;
                int dmg = h->kind == HZ_CLOUD ? h->arg : h->kind == HZ_FIRE ? 4 : 6;
                if (a->kind == AK_FIGHTER) {
                    bool down = a->state == AS_DOWN;
                    dku_hurt_fighter(j, dmg, false, a->face, false, -2);
                    if (!down && a->state == AS_HURT) a->stun = 6;
                } else {
                    dku_hit(j, -1, AT_NONE, dmg, a->face, 128);
                }
            }
            break;
        }
        case HZ_CAR:
            h->t++;
            if (h->t < 60) break; /* the horn first */
            h->x += 6;
            for (int j = 0; j < DKU_MAX_ACTORS; j++) {
                Actor *a = &dku_g.a[j];
                if (!a->alive || a->state == AS_DEAD || a->state == AS_FALL || a->team == 2) continue;
                if (a->z > dku_fx(12)) continue;
                int x = dku_px(a->x), y = dku_px(a->y);
                if (x >= h->x && x < h->x + h->w && iabs(y - h->y) <= 9) {
                    if (h->hit[j / 8] & (1 << (j % 8))) continue;
                    h->hit[j / 8] |= (uint8_t)(1 << (j % 8));
                    if (a->kind == AK_FIGHTER) dku_hurt_fighter(j, 30, false, 1, true, -3);
                    else dku_hit(j, -1, AT_NONE, 99, 1, AF_KNOCK | 128 | 512);
                    if (a->alive) a->vx = 60;
                }
            }
            if (h->x > dku_view_right() + 80) h->alive = 0;
            break;
        case HZ_THRESHER: {
            if (h->x < h->arg - h->w) {
                h->x += (dku_g.frame_t & 3) == 0 ? 0 : 1; /* three pixels in four frames */
            }
            int front = h->x + h->w;
            for (int j = 0; j < DKU_MAX_ACTORS; j++) {
                Actor *a = &dku_g.a[j];
                if (!a->alive || a->state == AS_DEAD || a->state == AS_FALL || a->team == 2) continue;
                int x = dku_px(a->x);
                if (x < front + 4 && x > h->x) {
                    if (a->kind == AK_FIGHTER) {
                        if (a->state != AS_DOWN) dku_hurt_fighter(j, 20, false, 1, true, -4);
                        a->x = dku_fx(front + 6);
                        if (a->state == AS_DOWN) a->vx = 40;
                    } else if (a->team == 1) {
                        dku_hit(j, -1, AT_NONE, 99, 1, AF_KNOCK | 128);
                    }
                }
            }
            break;
        }
        default: break;
        }
    }
}

static bool shot_blocked_by_prop(const Shot *s) {
    for (int i = 0; i < DKU_MAX_PROPS; i++) {
        const Prop *p = &dku_g.pr_[i];
        if (!p->alive || p->kind == PR_TUFT || p->kind == PR_POST) continue;
        if (iabs(dku_px(p->x - s->x)) <= 6 && iabs(dku_px(p->y - s->y)) <= 5 && s->z < dku_fx(18)) return true;
    }
    return false;
}

static void land_shot(Shot *s) {
    switch (s->kind) {
    case SH_BOTTLE:
        dku_explode(s->x, s->y, 18, 12, s->team, true);
        dku_add_hazard(HZ_FIRE, dku_px(s->x) - 12, dku_px(s->y) - 4, 24, 8, 60, 0);
        break;
    case SH_BOMB: dku_explode(s->x, s->y, 26, 14, s->team, true); break;
    case SH_GAS: dku_add_hazard(HZ_CLOUD, dku_px(s->x), dku_px(s->y), 22, 22, 240, 3); dku_sfx("dku_hiss"); break;
    case SH_SPIT: dku_add_hazard(HZ_CLOUD, dku_px(s->x), dku_px(s->y), 14, 14, 150, 2); dku_sfx("dku_hiss"); break;
    case SH_ITEM: {
        if (s->arg == IT_BOTTLE) {
            dku_explode(s->x, s->y, 20, 12, s->team, false);
            dku_add_hazard(HZ_FIRE, dku_px(s->x) - 12, dku_px(s->y) - 4, 24, 8, 60, 0);
            break;
        }
        int i = dku_drop_item(s->arg, dku_px(s->x), dku_px(s->y));
        if (i >= 0) { dku_g.it[i].z = 0; dku_g.it[i].vz = 0; dku_g.it[i].vx = s->vx / 4; dku_g.it[i].ammo = s->t; }
        break;
    }
    default: break;
    }
    s->alive = 0;
}

static void shots_update(void) {
    for (int i = 0; i < DKU_MAX_SHOTS; i++) {
        Shot *s = &dku_g.sh[i];
        if (!s->alive) continue;
        s->x += s->vx;
        s->y += s->vy;
        bool arcs = s->kind == SH_BOTTLE || s->kind == SH_BOMB || s->kind == SH_GAS || s->kind == SH_SPIT || s->kind == SH_ITEM;
        if (arcs) {
            s->z += s->vz;
            s->vz -= s->kind == SH_ITEM ? 1 : GRAV;
            if (s->kind == SH_ITEM && s->vz < -6) s->vz = -6;
            if (s->z <= 0) { s->z = 0; land_shot(s); continue; }
        }
        int sx = dku_px(s->x);
        if (sx < dku_view_left() - 40 || sx > dku_view_right() + 40) { s->alive = 0; continue; }
        if (s->kind == SH_PELLET && ++s->t > 26) { s->alive = 0; continue; }
        if (s->kind == SH_GAS || s->kind == SH_SPIT) continue;
        if ((s->kind == SH_CLEAVER || s->kind == SH_RAY || s->kind == SH_ITEM) && shot_blocked_by_prop(s)) {
            dku_burst(s->x, s->y, s->z, C_GREY, 3);
            if (s->kind == SH_ITEM) land_shot(s);
            else s->alive = 0;
            continue;
        }
        if ((s->kind == SH_BOTTLE || s->kind == SH_BOMB) && !s->reflected) continue; /* they only go off where they land */
        if (s->team == 2) continue; /* a thrown thing that has already hit: it just drops */
        /* the first one in the way takes it */
        for (int j = 0; j < DKU_MAX_ACTORS; j++) {
            Actor *a = &dku_g.a[j];
            if (!a->alive || j == s->owner || a->state == AS_DEAD || a->state == AS_FALL || a->state == AS_GONE) continue;
            if (a->state == AS_DORMANT && s->team == 1) continue;
            if (a->state == AS_LEASHED) continue;
            bool passer = a->kind == AK_PASSER && a->mode == 1;
            if (a->team == 2 && !passer && !(s->team == 0 && a->kind == AK_SAUCER)) continue;
            if (a->team == s->team) continue;
            int w = DKU_KINDS[a->kind].hw + 3;
            if (iabs(dku_px(a->x - s->x)) > w || iabs(dku_px(a->y - s->y)) > 6) continue;
            int az = dku_px(a->z), sz = dku_px(s->z);
            if (a->kind == AK_UNDERTOW) { if (dku_px(s->y) > dku_px(a->y) + 40) continue; }
            else if (sz < az - 4 || sz > az + DKU_KINDS[a->kind].h + 2) continue;
            int dir = s->vx >= 0 ? 1 : -1;
            if (passer) {
                /* night 2's passer-by: cut down, and his gun falls */
                dku_kill(j, dir);
                if (a->ammo) dku_drop_item(a->ammo, dku_px(a->x), dku_px(a->y));
                s->alive = 0;
                break;
            }
            if (s->team == 1 && a->team == 1) {
                /* other ghouls stop a cleaver */
                s->alive = 0;
                break;
            }
            if (s->kind == SH_BOTTLE || s->kind == SH_BOMB) { land_shot(s); break; }
            uint32_t fl = 64;
            if (s->kind == SH_PELLET || (s->kind == SH_ITEM && (s->arg == IT_BIN || s->arg == IT_BOTTLE))) fl |= AF_KNOCK;
            if (s->kind == SH_RAY && a->kind == AK_FIGHTER) fl |= 0;
            if (s->kind == SH_ITEM && s->arg == IT_BOTTLE) { land_shot(s); break; }
            dku_hit(j, s->owner, AT_NONE, s->dmg, dir, fl | (s->kind == SH_ITEM ? 256 : 0));
            if (s->kind == SH_ITEM) {
                s->vx = -s->vx / 4;
                s->vz = 4;
                if (s->z <= 0) s->z = 1;
                s->team = 2;
            } else {
                s->alive = 0;
            }
            break;
        }
    }
}

static void items_update(void) {
    for (int i = 0; i < DKU_MAX_ITEMS; i++) {
        Item *it = &dku_g.it[i];
        if (it->alive != 1) continue;
        it->t++;
        if (it->z > 0 || it->vz > 0) {
            it->z += it->vz;
            it->vz -= GRAV;
            if (it->z <= 0) { it->z = 0; it->vz = 0; }
        }
        it->x += it->vx;
        it->vx = it->vx * 7 / 8;
        int l = dku_fx(dku_view_left() + 4), r = dku_fx(dku_view_right() - 4);
        if (it->x < l) it->x = l;
        if (it->x > r && it->x < r + dku_fx(40)) it->x = r;
        if (dku_in_pit(dku_px(it->x), dku_px(it->y)) && it->z == 0) it->alive = 0;
    }
    for (int i = 0; i < DKU_MAX_PROPS; i++)
        if (dku_g.pr_[i].alive && dku_g.pr_[i].shake > 0) dku_g.pr_[i].shake--;
    for (int i = 0; i < DKU_MAX_PARTS; i++) {
        Part *p = &dku_g.part[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->z += p->vz;
        p->vz -= 3;
        if (p->z < 0) { p->z = 0; p->vz = -p->vz / 3; p->vx /= 2; }
    }
}

static void pits_update(void) {
    for (int j = 0; j < DKU_MAX_ACTORS; j++) {
        Actor *a = &dku_g.a[j];
        if (!a->alive || a->z > 0) continue;
        switch (a->state) {
        case AS_DEAD: case AS_FALL: case AS_GONE: case AS_RISE: case AS_GRABBED: case AS_LEASHED: case AS_DORMANT: continue;
        default: break;
        }
        if (a->kind == AK_UNDERTOW || a->kind == AK_SAUCER || a->kind == AK_FEELER) continue;
        if (!dku_in_pit(dku_px(a->x), dku_px(a->y))) continue;
        if (a->state == AS_GRAB && a->partner >= 0) { dku_g.a[a->partner].partner = -1; dku_set_state(&dku_g.a[a->partner], AS_FREE); }
        a->partner = -1;
        dku_set_state(a, AS_FALL);
        a->vx = a->vy = 0;
        dku_sfx("dku_fall");
        if (a->kind == AK_FIGHTER) { a->hp = 0; dku_g.dmg_by[5] += 100; } /* tests: a fall counts as source -5 */
    }
}

/* ---- the camera ---------------------------------------------------------------------- */

static void camera_update(void) {
    const DkuSection *sec = cur_sec();
    if (sec->kind != SEC_WALK) return;
    int lead = -1, back = 1 << 30;
    for (int p = 0; p < 2; p++) {
        const Actor *a = &dku_g.a[p];
        if (!a->alive || a->state == AS_DEAD || a->state == AS_GONE || a->state == AS_FALL) continue;
        int x = dku_px(a->x);
        if (x > lead) lead = x;
        if (x < back) back = x;
    }
    if (lead < 0) return;
    int want = lead - 150;
    int most = sec->len - SCREEN_W;
    if (dku_g.cam_lock >= 0 && most > dku_g.cam_lock) most = dku_g.cam_lock;
    if (dku_g.players == 2 && back < (1 << 30)) most = imin(most, back - 8);
    /* the runaway thresher pushes the view along */
    for (int i = 0; i < DKU_MAX_HAZARDS; i++)
        if (dku_g.hz[i].alive && dku_g.hz[i].kind == HZ_THRESHER) {
            int forced = dku_g.hz[i].x + dku_g.hz[i].w - 30;
            if (want < forced) want = forced;
            most = imax(most, imin(forced, sec->len - SCREEN_W));
        }
    if (want > most) want = most;
    if (want > dku_g.cam) dku_g.cam = imin(want, dku_g.cam + 3);
}

/* ---- one frame of the world -------------------------------------------------------------- */

static void next_section(void) {
    const DkuNight *n = &DKU_NIGHT[dku_g.night];
    if (dku_g.sec + 1 < n->nsec) {
        dku_g.fade = 1;
    }
}

void dku_world_update(void) {
    const DkuSection *sec = cur_sec();
    if (dku_g.fade > 0) {
        dku_g.fade++;
        if (dku_g.fade == 16) dku_start_section(dku_g.sec + 1);
        if (dku_g.fade >= 30) dku_g.fade = 0;
        return;
    }
    dku_g.sec_t++;
    if (dku_g.go_t > 0) dku_g.go_t--;
    if (dku_g.shake > 0) dku_g.shake--;

    /* the gym is just behind you as night 1 begins */
    if (!dku_g.gym && dku_g.night == 0 && dku_g.sec == 0 && dku_g.cam == 0) {
        bool pushing = false;
        for (int p = 0; p < dku_g.players; p++) {
            const Actor *a = &dku_g.a[p];
            uint32_t h, pr, r;
            dku_fighter_input(p, &h, &pr, &r);
            if (a->alive && dku_px(a->x) <= dku_view_left() + 7 && (h & BTN_LEFT) && a->state == AS_FREE) pushing = true;
        }
        dku_g.gym_push = pushing ? dku_g.gym_push + 1 : 0;
        if (dku_g.gym_push >= 24 && dku_enemies_awake() == 0) {
            dku_start_gym();
            dku_sfx("dku_door");
            music_play(DKU_MUS_GYM);
            return;
        }
    }

    camera_update();
    /* the script: things happen as the view passes them */
    for (int k = 0; k < sec->nev; k++) {
        if (dku_g.ev_done[k]) continue;
        const DkuEvt *e = &sec->ev[k];
        if (e->op == EV_WAVE) continue;
        if (dku_g.cam >= e->at) { dku_g.ev_done[k] = true; fire_event(e); }
    }
    /* a lock-screen holds until nobody is left */
    if (dku_g.cam_lock >= 0 && dku_g.cam >= dku_g.cam_lock && enemies_here() == 0) {
        bool pending = false;
        for (int k = 0; k < sec->nev; k++)
            if (!dku_g.ev_done[k] && sec->ev[k].op == EV_SPAWN && sec->ev[k].at <= dku_g.cam_lock) pending = true;
        if (!pending) { dku_g.cam_lock = -1; dku_g.go_t = 120; dku_sfx("dku_go"); }
    }
    /* a boss fight's steady stream */
    if (dku_g.stream_kind >= 0 && !dku_g.boss_down) {
        if (--dku_g.stream_t <= 0) {
            dku_g.stream_t = dku_g.stream_every;
            int kind = dku_g.stream_kind;
            if (kind == 0) {
                static const uint8_t MIX[] = {AK_SHAMBLER, AK_SHAMBLER, AK_TORCH, AK_CROW, AK_SHAMBLER, AK_GIGGLER};
                kind = MIX[rng_range(&dku_g.rng, 0, ARRAY_LEN(MIX) - 1)];
            }
            int n = 0;
            for (int i = 2; i < DKU_MAX_ACTORS; i++)
                if (dku_g.a[i].alive && dku_g.a[i].team == 1 && !dku_g.a[i].boss && dku_g.a[i].state != AS_DEAD && dku_g.a[i].state != AS_DORMANT) n++;
            if (n < dku_g.stream_max) {
                if (kind == AK_SKIPPER) {
                    int x = dku_view_left() + rng_range(&dku_g.rng, 40, 200);
                    dku_spawn(AK_SKIPPER, FROM_WATER, x, DKU_FLOOR0 - 4, 0);
                } else {
                    dku_spawn(kind, rng_range(&dku_g.rng, 0, 1) ? FROM_LEFT : FROM_RIGHT, 0,
                              DKU_FLOOR0 + 6 + rng_range(&dku_g.rng, 0, DKU_FLOOR1 - DKU_FLOOR0 - 12), 0);
                }
            }
        }
    }
    /* the final fight: if it goes on long enough, a pack of dogs comes running */
    if (dku_g.boss >= 0 && dku_g.a[dku_g.boss].kind == AK_GRIST && !dku_g.boss_down) {
        if (dku_g.a[dku_g.boss].mode == 2 && ++dku_g.dog_t == 60 * 60 && !dku_g.dogs_came) {
            dku_g.dogs_came = true;
            for (int k = 0; k < 4; k++) {
                int i = dku_spawn(AK_DOG, FROM_LEFT, 0, DKU_FLOOR0 + 10 + k * 16, 0);
                if (i >= 0) { dku_g.a[i].team = 0; dku_g.a[i].state = AS_FREE; dku_g.a[i].x -= dku_fx(k * 12); }
            }
            dku_sfx("dku_bark");
        }
    }
    if (sec->kind == SEC_LIFT) lift_update();
    if (sec->kind == SEC_GYM) gym_update();

    dku_fighters_update();
    dku_foes_update();
    shots_update();
    hazards_update();
    items_update();
    pits_update();

    /* the way on: a door that opens before the end of the street (the hotel's
       service lift) opens as soon as nobody is left; walk past it and more come */
    int early = -1;
    for (int i = 0; i < DKU_MAX_HAZARDS; i++)
        if (dku_g.hz[i].alive && dku_g.hz[i].kind == HZ_DOOR && dku_g.hz[i].arg == 1) early = i;
    if (sec->kind == SEC_WALK && !dku_g.gym && early >= 0) {
        if (dku_g.cam_lock < 0 && enemies_here() == 0) {
            if (dku_g.exit_t == 0) { dku_g.exit_t = 1; dku_g.go_t = 120; dku_sfx("dku_ding"); }
            dku_g.exit_t++;
            const Hazard *h = &dku_g.hz[early];
            for (int p = 0; p < dku_g.players; p++) {
                const Actor *a = &dku_g.a[p];
                if (!a->alive || a->state == AS_DEAD || a->state == AS_GONE || a->state == AS_FALL) continue;
                int x = dku_px(a->x);
                if (x >= h->x && x < h->x + h->w) { next_section(); break; }
            }
        } else {
            dku_g.exit_t = 0;
        }
    } else if (sec->kind == SEC_WALK && !dku_g.gym) {
        bool at_end = dku_g.cam >= sec->len - SCREEN_W && dku_g.cam_lock < 0;
        bool pending = false;
        for (int k = 0; k < sec->nev; k++) if (!dku_g.ev_done[k]) pending = true;
        if (at_end && !pending && dku_enemies_alive() == 0) {
            if (dku_g.exit_t == 0) { dku_g.exit_t = 1; dku_g.go_t = 120; }
            dku_g.exit_t++;
            int beam = -1;
            for (int i = 0; i < DKU_MAX_HAZARDS; i++) if (dku_g.hz[i].alive && dku_g.hz[i].kind == HZ_BEAM) beam = i;
            for (int p = 0; p < dku_g.players; p++) {
                const Actor *a = &dku_g.a[p];
                if (!a->alive || a->state == AS_DEAD || a->state == AS_GONE) continue;
                int x = dku_px(a->x);
                bool out = beam >= 0 ? (x >= dku_g.hz[beam].x && x < dku_g.hz[beam].x + dku_g.hz[beam].w)
                                     : x >= sec->len - 14;
                if (out) { next_section(); if (beam >= 0) dku_sfx("dku_beam"); break; }
            }
        }
    }
    if (sec->kind == SEC_LIFT && dku_g.lift_wave > 5) {
        if (++dku_g.exit_t > 90) next_section();
    }
    if (sec->kind == SEC_BOSS && dku_g.boss_down) {
        if (++dku_g.exit_t > 100) dku_g.night_done = true;
    }
}
