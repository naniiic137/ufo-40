/* DUKES UP - the world: a night's sections and their scripts, the camera
 * that only moves on, spawning by scroll position, lock-screens, the lift,
 * the gym's waves, hazards, flying things, things on the floor and blasts.
 * Integer arithmetic only. */
#include "dku.h"

#define GRAV 4

static const DkuSection *cur_sec(void) {
    if (dk.gym) return &DKU_GYM;
    return &DKU_NIGHT[dk.night].sec[dk.sec];
}

int dku_floor_lo(void) {
    const DkuSection *s = cur_sec();
    return s->kind == SEC_LIFT ? 126 : DKU_FLOOR0;
}
int dku_floor_hi(void) { return DKU_FLOOR1; }
int dku_view_left(void) { return dk.cam + (cur_sec()->kind == SEC_LIFT ? 70 : 0); }
int dku_view_right(void) { return dk.cam + (cur_sec()->kind == SEC_LIFT ? 250 : SCREEN_W); }

/* ---- small helpers ------------------------------------------------------------- */

void dku_burst(int x, int y, int z, int col, int n) {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < DKU_MAX_PARTS; i++) {
            Part *p = &dk.part[i];
            if (p->life > 0) continue;
            p->x = x;
            p->y = y;
            p->z = z + dku_fx(4);
            p->vx = rng_range(&dk.rng, -24, 24);
            p->vy = rng_range(&dk.rng, -6, 6);
            p->vz = rng_range(&dk.rng, 8, 30);
            p->life = rng_range(&dk.rng, 12, 26);
            p->col = col;
            break;
        }
    }
}

bool dku_in_pit(int x, int y) {
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        const Hazard *h = &dk.hz[i];
        if (!h->alive || (h->kind != HZ_PIT && h->kind != HZ_WATER)) continue;
        if (x >= h->x && x < h->x + h->w && y >= h->y && y < h->y + h->h) return true;
    }
    return false;
}

int dku_hazard_at(int kind, int x, int y) {
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        const Hazard *h = &dk.hz[i];
        if (!h->alive || h->kind != kind) continue;
        if (x >= h->x && x < h->x + h->w && y >= h->y && y < h->y + h->h) return i;
    }
    return -1;
}

void dku_add_hazard(int kind, int x, int y, int w, int h, int t, int arg) {
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        Hazard *z = &dk.hz[i];
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
        Shot *s = &dk.sh[i];
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
        Item *it = &dk.it[i];
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
        Prop *p = &dk.pr_[i];
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
        Actor *o = &dk.a[j];
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
        Prop *p = &dk.pr_[i];
        if (!p->alive || p->kind == PR_POST) continue;
        if (iabs(dku_px(p->x - x)) <= r && iabs(dku_px(p->y - y)) * 2 <= r) {
            p->alive = 0;
            if (p->content) dku_drop_item(p->content, dku_px(p->x), dku_px(p->y));
            if (p->content2) dku_drop_item(p->content2, dku_px(p->x) + 6, dku_px(p->y));
        }
    }
    dku_burst(x, y, dku_fx(4), C_ORANGE, 10);
    dku_burst(x, y, dku_fx(8), C_YELLOW, 6);
    dk.shake = imax(dk.shake, 8);
    dku_sfx("dku_boom");
}

int dku_enemies_alive(void) {
    int n = 0;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        const Actor *a = &dk.a[i];
        if (a->alive && a->team == 1 && a->state != AS_DEAD && a->state != AS_FALL && a->state != AS_GONE) n++;
    }
    return n;
}

int dku_enemies_awake(void) {
    int n = 0;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        const Actor *a = &dk.a[i];
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
        const Actor *a = &dk.a[i];
        if (!a->alive || a->team != 1 || a->state == AS_DEAD || a->state == AS_FALL || a->state == AS_GONE) continue;
        if ((a->state == AS_DORMANT || a->kind == AK_FEELER) && dku_px(a->x) > r) continue;
        n++;
    }
    return n;
}

void dku_clear_enemies(void) {
    for (int i = 2; i < DKU_MAX_ACTORS; i++)
        if (dk.a[i].alive && dk.a[i].team == 1) dk.a[i].alive = 0;
    for (int s = 0; s < DKU_MAX_SHOTS; s++) if (dk.sh[s].team == 1) dk.sh[s].alive = 0;
}

/* ---- spawning ------------------------------------------------------------------- */

static int boss_hp(int kind) {
    switch (kind) {
    case AK_RAMMER: return 90;
    case AK_TUSKER: return 120;
    case AK_VISITOR: return 40;
    case AK_UNDERTOW: return 60;
    case AK_GRIST: return 110;
    default: return DKU_KINDS[kind].hp;
    }
}

int dku_spawn(int kind, int from, int x, int y, int mode) {
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        Actor *a = &dk.a[i];
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
        a->cool = rng_range(&dk.rng, 20, 60);
        a->timer2 = 0;
        switch (from) {
        case FROM_LEFT:
            a->x = dku_fx(dku_view_left() - 20 - rng_range(&dk.rng, 0, 10));
            a->face = 1;
            a->state = AS_ENTER;
            break;
        case FROM_RIGHT:
            a->x = dku_fx(dku_view_right() + 20 + rng_range(&dk.rng, 0, 10));
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
    Actor *a = &dk.a[i];
    a->boss = 1;
    a->hp = a->maxhp = boss_hp(kind);
    a->cool = 60;
    if (kind == AK_UNDERTOW) { a->y = dku_fx(y); a->z = 0; }
    dk.boss = i;
    return i;
}

/* ---- fighters -------------------------------------------------------------------- */

static void place_fighters(void) {
    for (int p = 0; p < 2; p++) {
        Actor *a = &dk.a[p];
        int hp = a->hp;
        bool was = a->alive && a->kind == AK_FIGHTER;
        memset(a, 0, sizeof *a);
        if (p >= dk.players) continue;
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
        a->state = AS_FREE;
    }
}

void dku_new_run(int players) {
    int menu = dk.menu, sel0 = dk.sel[0], sel1 = dk.sel[1];
    DkuPlayerRun pr0 = dk.pr[0], pr1 = dk.pr[1];
    Rng keep = dk.rng;
    memset(&dk, 0, sizeof dk);
    dk.rng = keep;
    dk.menu = menu;
    dk.sel[0] = sel0;
    dk.sel[1] = sel1;
    dk.pr[0] = pr0;
    dk.pr[1] = pr1;
    dk.players = players;
    for (int p = 0; p < 2; p++)
        for (int s = 0; s < DK_NSTATS; s++) dk.pr[p].stat[s] = DKU_FIGHTERS[dk.pr[p].pick].stat[s];
    dk.boss = -1;
    dk.cam_lock = -1;
    for (int p = 0; p < 2; p++) { dk.a[p].alive = 0; }
}

static void clear_world(bool keep_dogs) {
    for (int i = 2; i < DKU_MAX_ACTORS; i++)
        if (!(keep_dogs && dk.a[i].alive && dk.a[i].kind == AK_DOG && dk.a[i].team == 0)) dk.a[i].alive = 0;
    memset(dk.it, 0, sizeof dk.it);
    memset(dk.pr_, 0, sizeof dk.pr_);
    memset(dk.sh, 0, sizeof dk.sh);
    memset(dk.hz, 0, sizeof dk.hz);
    memset(dk.part, 0, sizeof dk.part);
}

static void fire_event(const DkuEvt *e);

void dku_start_section(int s) {
    dk.sec = s;
    dk.cam = 0;
    dk.cam_lock = -1;
    dk.sec_t = 0;
    dk.go_t = 0;
    dk.exit_t = 0;
    dk.boss = -1;
    dk.boss_down = false;
    dk.night_done = false;
    dk.lift_wave = 0;
    dk.lift_t = 0;
    dk.stream_kind = -1;
    dk.stream_t = 0;
    dk.dog_t = 0;
    dk.dogs_came = false;
    dk.thresher_stop = 0;
    memset(dk.ev_done, 0, sizeof dk.ev_done);
    clear_world(true);
    for (int p = 0; p < 2; p++) {
        Actor *a = &dk.a[p];
        if (!a->alive) continue;
        a->carry = -1;
        a->partner = -1;
        if (a->state != AS_DEAD && a->state != AS_GONE && a->state != AS_FALL) {
            a->state = AS_FREE;
            a->z = a->vz = a->vx = a->vy = 0;
            a->x = dku_fx(dku_view_left() + 44 + p * 22);
            a->y = dku_fx(DKU_FLOORMID - 8 + p * 18);
            a->face = 1;
        }
    }
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        Actor *d = &dk.a[i];
        if (d->alive && d->kind == AK_DOG) { d->x = dku_fx(dku_view_left() + 20); d->y = dku_fx(DKU_FLOORMID); d->state = AS_FREE; }
    }
    const DkuSection *sec = cur_sec();
    for (int k = 0; k < sec->nev; k++) {
        if (sec->ev[k].at <= 0 && sec->ev[k].op != EV_WAVE) {
            dk.ev_done[k] = true;
            fire_event(&sec->ev[k]);
        }
    }
    if (sec->kind == SEC_LIFT) { dk.lift_wave = 0; dk.lift_t = 40; }
}

void dku_start_night(int night) {
    dk.night = night;
    dk.sec = 0;
    dk.gym = false;
    for (int p = 0; p < dk.players; p++) {
        Actor *a = &dk.a[p];
        if (!a->alive || a->kind != AK_FIGHTER) {
            a->alive = 1;
            a->kind = AK_FIGHTER;
            a->hp = DKU_MAX_HP;
        }
        if (a->hp <= 0 || a->state == AS_DEAD || a->state == AS_GONE || a->state == AS_FALL) {
            /* a partner who fell comes back for the next night at half health
               (or more, if the shop's soup was bought for them) */
            a->hp = imax(a->hp, DKU_MAX_HP / 2);
            a->state = AS_FREE;
        }
    }
    dk.cam = 0;
    place_fighters();
    for (int p = 0; p < dk.players; p++) dk.start_hp[p] = dk.a[p].hp;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) dk.a[i].alive = 0;
    dku_start_section(0);
}

void dku_start_gym(void) {
    dk.gym = true;
    dk.gym_wave = 1;
    dk.gym_break = 120;
    dk.gym_nq = 0;
    dk.gym_spawned = 0;
    dk.cam = 0;
    for (int p = 0; p < dk.players; p++) dk.a[p].hp = imax(dk.a[p].hp, 1);
    dku_start_section(0);
    for (int p = 0; p < dk.players; p++) {
        dk.a[p].x = dku_fx(150 + p * 20);
        dk.a[p].y = dku_fx(DKU_FLOORMID + p * 10);
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
    case EV_ITEM: { int i = dku_drop_item(e->a, e->x, e->y); if (i >= 0) { dk.it[i].z = 0; dk.it[i].vz = 0; } break; }
    case EV_LOCK: dk.cam_lock = e->at; break;
    case EV_PIT: dku_add_hazard(HZ_PIT, e->x, e->y, e->w, e->h, 0, 0); break;
    case EV_WATER: dku_add_hazard(HZ_WATER, e->x, e->y, e->w, e->h, 0, 0); break;
    case EV_MINE: dku_add_hazard(HZ_MINE, e->x - 5, e->y - 3, 10, 6, 0, 0); break;
    case EV_LAMP: {
        dku_add_hazard(HZ_LAMP, e->x, e->y, 36, 12, 0, 1);
        for (int i = DKU_MAX_HAZARDS - 1; i >= 0; i--)
            if (dk.hz[i].alive && dk.hz[i].kind == HZ_LAMP && dk.hz[i].x == e->x && dk.hz[i].y == e->y) { dk.hz[i].arg2 = e->b; break; }
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
        if (i >= 0) { dk.a[i].ammo = e->b; dk.a[i].mode = 1; }
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
    case EV_DOOR: dku_add_hazard(HZ_DOOR, e->x, DKU_FLOOR0 - 40, 26, 40, 0, 0); break;
    case EV_STREAM:
        dk.stream_kind = e->a;
        dk.stream_every = e->b * 10;
        dk.stream_max = e->c;
        dk.stream_t = dk.stream_every;
        break;
    default: break;
    }
}

static void lift_update(void) {
    const DkuSection *sec = cur_sec();
    if (dk.lift_wave > 5) return;
    if (dku_enemies_alive() > 0) return;
    if (dk.lift_t > 0) { dk.lift_t--; return; }
    dk.lift_wave++;
    if (dk.lift_wave > 5) { dk.exit_t = 1; dku_sfx("dku_ding"); return; }
    /* the next wave lands on the lift's roof, and they drop in turn */
    int n = 0;
    for (int k = 0; k < sec->nev; k++) {
        const DkuEvt *e = &sec->ev[k];
        if (e->op != EV_WAVE || e->a != dk.lift_wave) continue;
        for (int c = 0; c < e->c; c++) {
            int x = 96 + ((n * 37) % 130);
            int i = dku_spawn(e->b, FROM_ROOF, x, 0, 0);
            if (i >= 0) { dk.a[i].timer2 = 50 + n * 40; dk.a[i].face = -1; }
            n++;
        }
    }
    dk.lift_t = 60;
    dku_sfx("dku_ding");
}

static void gym_update(void) {
    if (dk.gym_break > 0) {
        dk.gym_break--;
        if (dk.gym_break == 0) {
            uint8_t q[24];
            int n = 0;
            dku_gym_wave(dk.gym_wave, q, &n, 24);
            memcpy(dk.gym_queue, q, sizeof q);
            dk.gym_nq = n;
            dk.gym_spawned = 0;
            dk.stream_t = 0;
            /* every third wave, frightened passers-by run through first */
            if (dk.gym_wave % 3 == 0) {
                for (int k = 0; k < 3; k++) {
                    int i = dku_spawn(AK_PASSER, FROM_RIGHT, 0, DKU_FLOOR0 + 12 + k * 20, 0);
                    if (i >= 0) { dk.a[i].mode = 0; dk.a[i].x += dku_fx(k * 30); }
                }
            }
            dku_sfx("dku_whistle");
        }
        return;
    }
    if (dk.gym_spawned < dk.gym_nq) {
        if (--dk.stream_t <= 0 && dku_enemies_alive() < 5) {
            int k = dk.gym_queue[dk.gym_spawned];
            int side = (dk.gym_spawned & 1) ? FROM_LEFT : FROM_RIGHT;
            dku_spawn(k, side, 0, DKU_FLOOR0 + 8 + rng_range(&dk.rng, 0, DKU_FLOOR1 - DKU_FLOOR0 - 16), 0);
            dk.gym_spawned++;
            dk.stream_t = 40;
        }
        return;
    }
    if (dku_enemies_alive() == 0) {
        /* the wave is beaten */
        int beaten = dk.gym_wave;
        uint16_t *best = dk.players == 2 ? &dks.gym_best2 : &dks.gym_best1;
        if (beaten > *best) *best = (uint16_t)beaten;
        if (beaten >= DKU_GYM_GIFT) dku_award_gym();
        dku_save_now();
        dk.gym_wave++;
        dk.gym_break = 150;
        dku_sfx("dku_bell");
    }
}

/* survival: wave n's line-up (ours: more and tougher as it goes) */
void dku_gym_wave(int wave, uint8_t *kinds, int *n, int max) {
    static const uint8_t UNLOCK[] = {AK_SHAMBLER, AK_SHAMBLER, AK_GIGGLER, AK_TORCH, AK_CROW, AK_HOWLER, AK_SKIPPER,
                                     AK_BULWARK, AK_RAMMER, AK_VISITOR, AK_SLUDGER, AK_TUSKER};
    int count = imin(max, imin(16, 3 + wave * 3 / 4));
    int pool = imin(ARRAY_LEN(UNLOCK), wave + 1);
    Rng r;
    rng_seed(&r, (uint64_t)(1000 + wave * 7));
    for (int k = 0; k < count; k++) {
        int pick = rng_range(&r, 0, pool - 1);
        /* the newest kind always shows up once */
        if (k == 0 && wave >= 2) pick = pool - 1;
        kinds[k] = UNLOCK[pick];
    }
    *n = count;
}

bool dku_section_cleared(void) { return dku_enemies_alive() == 0; }

/* ---- hazards, shots, items -------------------------------------------------------------- */

static void hazards_update(void) {
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        Hazard *h = &dk.hz[i];
        if (!h->alive) continue;
        switch (h->kind) {
        case HZ_MINE:
            for (int j = 0; j < DKU_MAX_ACTORS; j++) {
                Actor *a = &dk.a[j];
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
                    const Actor *a = &dk.a[p];
                    if (!a->alive || a->state == AS_DEAD) continue;
                    int dx = dku_px(a->x) - (h->x), dy = dku_px(a->y) - (h->y);
                    if (dx * dx * 36 + dy * dy * 324 <= 18 * 18 * 36) { h->t = 34; dku_sfx("dku_creak"); }
                }
            } else if (h->t > 0) {
                if (--h->t == 0) {
                    /* it comes down on whoever stands in its shadow */
                    for (int j = 0; j < DKU_MAX_ACTORS; j++) {
                        Actor *a = &dk.a[j];
                        if (!a->alive || a->state == AS_DEAD || a->team == 2) continue;
                        if (h->arg == 2 && a->team != 0) continue;
                        int dx = dku_px(a->x) - h->x, dy = dku_px(a->y) - h->y;
                        if (iabs(dx) <= h->w / 2 + 2 && iabs(dy) <= h->h / 2 + 2 && a->z < dku_fx(30))
                            dku_hit(j, -1, AT_NONE, h->arg == 2 ? 14 : 22, dx >= 0 ? 1 : -1, AF_KNOCK | 128);
                    }
                    dk.shake = 8;
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
                Actor *a = &dk.a[j];
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
                Actor *a = &dk.a[j];
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
                h->x += (dk.frame_t & 3) == 0 ? 0 : 1; /* three pixels in four frames */
            }
            int front = h->x + h->w;
            for (int j = 0; j < DKU_MAX_ACTORS; j++) {
                Actor *a = &dk.a[j];
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
        const Prop *p = &dk.pr_[i];
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
        if (i >= 0) { dk.it[i].z = 0; dk.it[i].vz = 0; dk.it[i].vx = s->vx / 4; dk.it[i].ammo = s->t; }
        break;
    }
    default: break;
    }
    s->alive = 0;
}

static void shots_update(void) {
    for (int i = 0; i < DKU_MAX_SHOTS; i++) {
        Shot *s = &dk.sh[i];
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
            Actor *a = &dk.a[j];
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
        Item *it = &dk.it[i];
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
        if (dk.pr_[i].alive && dk.pr_[i].shake > 0) dk.pr_[i].shake--;
    for (int i = 0; i < DKU_MAX_PARTS; i++) {
        Part *p = &dk.part[i];
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
        Actor *a = &dk.a[j];
        if (!a->alive || a->z > 0) continue;
        switch (a->state) {
        case AS_DEAD: case AS_FALL: case AS_GONE: case AS_RISE: case AS_GRABBED: case AS_LEASHED: case AS_DORMANT: continue;
        default: break;
        }
        if (a->kind == AK_UNDERTOW || a->kind == AK_SAUCER || a->kind == AK_FEELER) continue;
        if (!dku_in_pit(dku_px(a->x), dku_px(a->y))) continue;
        if (a->state == AS_GRAB && a->partner >= 0) { dk.a[a->partner].partner = -1; dku_set_state(&dk.a[a->partner], AS_FREE); }
        a->partner = -1;
        dku_set_state(a, AS_FALL);
        a->vx = a->vy = 0;
        dku_sfx("dku_fall");
        if (a->kind == AK_FIGHTER) { a->hp = 0; dk.dmg_by[5] += 100; dk.dmg_by[30] = dku_px(a->x); dk.dmg_by[31] = dku_px(a->y) + 1000 * dk.sec; }
    }
}

/* ---- the camera ---------------------------------------------------------------------- */

static void camera_update(void) {
    const DkuSection *sec = cur_sec();
    if (sec->kind != SEC_WALK) return;
    int lead = -1, back = 1 << 30;
    for (int p = 0; p < 2; p++) {
        const Actor *a = &dk.a[p];
        if (!a->alive || a->state == AS_DEAD || a->state == AS_GONE || a->state == AS_FALL) continue;
        int x = dku_px(a->x);
        if (x > lead) lead = x;
        if (x < back) back = x;
    }
    if (lead < 0) return;
    int want = lead - 150;
    int most = sec->len - SCREEN_W;
    if (dk.cam_lock >= 0 && most > dk.cam_lock) most = dk.cam_lock;
    if (dk.players == 2 && back < (1 << 30)) most = imin(most, back - 8);
    /* the runaway thresher pushes the view along */
    for (int i = 0; i < DKU_MAX_HAZARDS; i++)
        if (dk.hz[i].alive && dk.hz[i].kind == HZ_THRESHER) {
            int forced = dk.hz[i].x + dk.hz[i].w - 30;
            if (want < forced) want = forced;
            most = imax(most, imin(forced, sec->len - SCREEN_W));
        }
    if (want > most) want = most;
    if (want > dk.cam) dk.cam = imin(want, dk.cam + 3);
}

/* ---- one frame of the world -------------------------------------------------------------- */

static void next_section(void) {
    const DkuNight *n = &DKU_NIGHT[dk.night];
    if (dk.sec + 1 < n->nsec) {
        dk.fade = 1;
    }
}

void dku_world_update(void) {
    const DkuSection *sec = cur_sec();
    if (dk.fade > 0) {
        dk.fade++;
        if (dk.fade == 16) dku_start_section(dk.sec + 1);
        if (dk.fade >= 30) dk.fade = 0;
        return;
    }
    dk.sec_t++;
    if (dk.go_t > 0) dk.go_t--;
    if (dk.shake > 0) dk.shake--;

    /* the gym is just behind you as night 1 begins */
    if (!dk.gym && dk.night == 0 && dk.sec == 0 && dk.cam == 0) {
        bool pushing = false;
        for (int p = 0; p < dk.players; p++) {
            const Actor *a = &dk.a[p];
            uint32_t h, pr, r;
            dku_fighter_input(p, &h, &pr, &r);
            if (a->alive && dku_px(a->x) <= dku_view_left() + 7 && (h & BTN_LEFT) && a->state == AS_FREE) pushing = true;
        }
        dk.gym_push = pushing ? dk.gym_push + 1 : 0;
        if (dk.gym_push >= 24 && dku_enemies_awake() == 0) {
            dku_start_gym();
            dku_sfx("dku_door");
            music_play(DKU_MUS_GYM);
            return;
        }
    }

    camera_update();
    /* the script: things happen as the view passes them */
    for (int k = 0; k < sec->nev; k++) {
        if (dk.ev_done[k]) continue;
        const DkuEvt *e = &sec->ev[k];
        if (e->op == EV_WAVE) continue;
        if (dk.cam >= e->at) { dk.ev_done[k] = true; fire_event(e); }
    }
    /* a lock-screen holds until nobody is left */
    if (dk.cam_lock >= 0 && dk.cam >= dk.cam_lock && enemies_here() == 0) {
        bool pending = false;
        for (int k = 0; k < sec->nev; k++)
            if (!dk.ev_done[k] && sec->ev[k].op == EV_SPAWN && sec->ev[k].at <= dk.cam_lock) pending = true;
        if (!pending) { dk.cam_lock = -1; dk.go_t = 120; dku_sfx("dku_go"); }
    }
    /* a boss fight's steady stream */
    if (dk.stream_kind >= 0 && !dk.boss_down) {
        if (--dk.stream_t <= 0) {
            dk.stream_t = dk.stream_every;
            int kind = dk.stream_kind;
            if (kind == 0) {
                static const uint8_t MIX[] = {AK_SHAMBLER, AK_SHAMBLER, AK_TORCH, AK_CROW, AK_SHAMBLER, AK_GIGGLER};
                kind = MIX[rng_range(&dk.rng, 0, ARRAY_LEN(MIX) - 1)];
            }
            int n = 0;
            for (int i = 2; i < DKU_MAX_ACTORS; i++)
                if (dk.a[i].alive && dk.a[i].team == 1 && !dk.a[i].boss && dk.a[i].state != AS_DEAD && dk.a[i].state != AS_DORMANT) n++;
            if (n < dk.stream_max) {
                if (kind == AK_SKIPPER) {
                    int x = dku_view_left() + rng_range(&dk.rng, 40, 200);
                    dku_spawn(AK_SKIPPER, FROM_WATER, x, DKU_FLOOR0 - 4, 0);
                } else {
                    dku_spawn(kind, rng_range(&dk.rng, 0, 1) ? FROM_LEFT : FROM_RIGHT, 0,
                              DKU_FLOOR0 + 6 + rng_range(&dk.rng, 0, DKU_FLOOR1 - DKU_FLOOR0 - 12), 0);
                }
            }
        }
    }
    /* the final fight: if it goes on long enough, a pack of dogs comes running */
    if (dk.boss >= 0 && dk.a[dk.boss].kind == AK_GRIST && !dk.boss_down) {
        if (++dk.dog_t == 60 * 60 && !dk.dogs_came) {
            dk.dogs_came = true;
            for (int k = 0; k < 4; k++) {
                int i = dku_spawn(AK_DOG, FROM_LEFT, 0, DKU_FLOOR0 + 10 + k * 16, 0);
                if (i >= 0) { dk.a[i].team = 0; dk.a[i].state = AS_FREE; dk.a[i].x -= dku_fx(k * 12); }
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

    /* the way on */
    if (sec->kind == SEC_WALK && !dk.gym) {
        bool at_end = dk.cam >= sec->len - SCREEN_W && dk.cam_lock < 0;
        bool pending = false;
        for (int k = 0; k < sec->nev; k++) if (!dk.ev_done[k]) pending = true;
        if (at_end && !pending && dku_enemies_alive() == 0) {
            if (dk.exit_t == 0) { dk.exit_t = 1; dk.go_t = 120; }
            dk.exit_t++;
            int beam = -1;
            for (int i = 0; i < DKU_MAX_HAZARDS; i++) if (dk.hz[i].alive && dk.hz[i].kind == HZ_BEAM) beam = i;
            for (int p = 0; p < dk.players; p++) {
                const Actor *a = &dk.a[p];
                if (!a->alive || a->state == AS_DEAD || a->state == AS_GONE) continue;
                int x = dku_px(a->x);
                bool out = beam >= 0 ? (x >= dk.hz[beam].x && x < dk.hz[beam].x + dk.hz[beam].w)
                                     : x >= sec->len - 14;
                if (out) { next_section(); if (beam >= 0) dku_sfx("dku_beam"); break; }
            }
        }
    }
    if (sec->kind == SEC_LIFT && dk.lift_wave > 5) {
        if (++dk.exit_t > 90) next_section();
    }
    if (sec->kind == SEC_BOSS && dk.boss_down) {
        if (++dk.exit_t > 100) dk.night_done = true;
    }
}
