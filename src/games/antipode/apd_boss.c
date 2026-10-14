/* ANTIPODE - the fights. Two minor ones (the Scrapper, the Lamprey), four
 * that each give a cipher (the Warden, Brackish, the Coilheart, Obverse),
 * and Halloran in the Crown in three phases. A health bar shows in the top
 * corner; the doors shut until the fight is over; a spare body that takes
 * over finds the boss with only 250 back. */
#include "apd.h"

const char *const APD_BOSS_NAME[APD_BOSSES] = {"SCRAPPER", "WARDEN", "BRACKISH", "LAMPREY", "COILHEART",
                                                "OBVERSE", "HALLORAN"};
const int APD_BOSS_HP[APD_BOSSES] = {300, 600, 700, 400, 700, 800, 700};
const int APD_HAL_HP[APD_HAL_PHASES] = {700, 600, 800};
int apd_lightning_x[4] = {-1, -1, -1, -1};

void apd_aim(int x, int y, int speed, int *vx, int *vy);
void apd_kill_foe(int i);

static const int BOSS_KIND[APD_BOSSES] = {FO_SCRAPPER, FO_WARDEN, FO_BRACKISH, FO_LAMPREY, FO_COILHEART, FO_OBVERSE,
                                          FO_HALLORAN};
static const int8_t CIPHER_OF[APD_BOSSES] = {-1, 0, 1, -1, 2, 3, -1};


static int W(void) { return ag.rw * APD_TILE; }
static int H(void) { return ag.rh * APD_TILE; }
static int floor_y(int h) { return (H() - APD_TILE - h / 2) * APD_FX; }
static int ceil_y(int h) { return (APD_TILE + h / 2) * APD_FX; }

static ApdFoe *main_foe(void) { return ag.boss_foe >= 0 ? &ag.foe[ag.boss_foe] : NULL; }

int apd_boss_hp(void) {
    const ApdFoe *f = main_foe();
    return f && f->alive ? f->hp : 0;
}
int apd_boss_maxhp(void) {
    const ApdFoe *f = main_foe();
    return f ? f->maxhp : 1;
}

static int part(int kind, int owner, int x, int y) {
    int i = apd_spawn_foe(kind, x, y, false);
    if (i >= 0) ag.foe[i].owner = (int16_t)owner;
    return i;
}

static void hal_phase(int ph);

void apd_boss_spawn(int boss) {
    int kind = BOSS_KIND[boss];
    int w, h;
    extern void apd_foe_size(int kind, int *w, int *h);
    apd_foe_size(kind, &w, &h);
    /* walkers start on the far side from him */
    int far = APD_PX(ag.pl.x) < W() / 2 ? W() - 60 : 60;
    int x = (kind == FO_SCRAPPER || kind == FO_WARDEN ? far : W() / 2) * APD_FX, y = floor_y(h);
    if (kind == FO_COILHEART) y = (H() / 2) * APD_FX;
    if (kind == FO_BRACKISH) y = (H() - 20) * APD_FX;
    if (kind == FO_LAMPREY) y = (H() / 2) * APD_FX;
    if (kind == FO_OBVERSE) {
        /* she waits on the far side, the other way up */
        x = (APD_PX(ag.pl.x) < W() / 2 ? W() - 60 : 60) * APD_FX;
        y = ceil_y(h);
    }
    int i = apd_spawn_foe(kind, x, y, kind == FO_OBVERSE);
    if (i < 0) return;
    ag.boss_foe = i;
    ag.boss_on = true;
    ag.boss_phase = 0;
    ag.boss_t = 0;
    ApdFoe *f = &ag.foe[i];
    f->hp = f->maxhp = (int16_t)APD_BOSS_HP[boss];
    f->dir = APD_PX(ag.pl.x) < W() / 2 ? -1 : 1;
    if (kind == FO_LAMPREY)
        for (int k = 0; k < 6; k++) {
            int s = part(FO_LAMP_SEG, i, x - (k + 1) * 14 * APD_FX, y);
            if (s >= 0) { ag.foe[s].flags |= FF_SHIELD; ag.foe[s].a = k == 0 ? i : s - 1; }
        }
    if (kind == FO_COILHEART) f->flags |= FF_SHIELD;
    if (kind == FO_HALLORAN) hal_phase(0);
    apd_lock_boss_doors(true);
    /* a spare body now wakes here, inside, not in the shut doorway */
    ag.pl.entry_x = ag.pl.x;
    ag.pl.entry_y = ag.pl.y;
    ag.pl.entry_grav = ag.pl.grav;
    ag.pl.entry_face = ag.pl.face;
    apd_sfx("apd_alarm");
    apd_music(boss == BS_HALLORAN ? APD_MUS_FINAL : APD_MUS_BOSS);
}

/* he has walked well into the arena: the fight starts */
void apd_boss_check_trigger(void) {
    if (ag.boss < 0 || ag.boss_on || ag.run.boss_dead[ag.boss] || ag.boss_dead_t > 0) return;
    int px = APD_PX(ag.pl.x), py = APD_PX(ag.pl.y);
    if (px > 44 && px < W() - 44 && py > 24 && py < H() - 4) apd_boss_spawn(ag.boss);
}

/* a spare body takes over: the boss gets only 250 back */
void apd_boss_regain(void) {
    ApdFoe *f = main_foe();
    if (!ag.boss_on || !f || !f->alive) return;
    f->hp = (int16_t)imin(f->maxhp, f->hp + APD_BOSS_REGAIN);
}

static void defeated(void) {
    ApdFoe *f = main_foe();
    int b = ag.boss;
    for (int i = 0; i < APD_MAX_FOES; i++)
        if (ag.foe[i].alive && (i == ag.boss_foe || ag.foe[i].owner == ag.boss_foe)) {
            apd_burst(APD_PX(ag.foe[i].x), APD_PX(ag.foe[i].y), C_YELLOW, 10, 3);
            ag.foe[i].alive = 0;
        }
    for (int i = 0; i < APD_MAX_FOES; i++)
        if (ag.foe[i].alive) ag.foe[i].alive = 0; /* the boss's mites and the like go too */
    memset(ag.es, 0, sizeof ag.es);
    for (int k = 0; k < 4; k++) apd_lightning_x[k] = -1;
    ag.lightning_t = 0;
    (void)f;
    ag.boss_on = false;
    ag.run.boss_dead[b] = 1;
    ag.boss_dead_t = 1;
    ag.shake = 20;
    apd_sfx("apd_bigboom");
}

/* the chain of explosions, then the doors open (and a cipher is given) */
void apd_boss_tick(void);
void apd_boss_tick(void) {
    apd_boss_check_trigger();
    if (ag.boss_dead_t > 0) {
        ag.boss_dead_t++;
        if (ag.boss_dead_t % 8 == 0 && ag.boss_dead_t < 90)
            apd_burst((ag.boss_dead_t * 53) % W(), 40 + (ag.boss_dead_t * 31) % (H() - 80), C_ORANGE, 10, 3);
        if (ag.boss_dead_t % 16 == 0 && ag.boss_dead_t < 90) apd_sfx("apd_boom");
        if (ag.boss_dead_t == 100) {
            int b = ag.boss;
            ag.boss_dead_t = 0;
            if (b == BS_HALLORAN) {
                extern void apd_final_defeated(void);
                apd_final_defeated();
                return;
            }
            apd_lock_boss_doors(false);
            if (b >= 0 && CIPHER_OF[b] >= 0) {
                ag.run.ciphers |= (uint8_t)(1 << CIPHER_OF[b]);
                char m[40];
                int n = 0;
                for (int k = 0; k < 4; k++) n += (ag.run.ciphers >> k) & 1;
                snprintf(m, sizeof m, "CIPHER %d OF 4", n);
                apd_say(m);
                ag.msg_t = 180;
                apd_sfx("apd_item");
                if (!apd_simulating) music_play(APD_MUS_ITEM);
            } else {
                extern void apd_room_music(void);
                apd_room_music();
            }
        }
    }
    /* the Crown's lightning: where its strikes are live, shots vanish */
    for (int k = 0; k < 4; k++) apd_lightning_x[k] = -1;
    int n = 0;
    for (int i = 0; i < APD_MAX_ESHOTS && n < 4; i++)
        if (ag.es[i].alive && ag.es[i].kind == ES_LIGHTNING && ag.es[i].t > 40) apd_lightning_x[n++] = APD_PX(ag.es[i].x);
    ag.lightning_t = n > 0 ? 1 : 0;
}

void apd_boss_dead_check(int i) { (void)i; }

void apd_boss_hurt(int i, int dmg) {
    ApdFoe *f = &ag.foe[i];
    if (!f->alive || (f->flags & (FF_SHIELD | FF_NOHIT))) { apd_sfx("apd_tink"); return; }
    f->hp = (int16_t)(f->hp - dmg);
    f->flash = 4;
    apd_sfx("apd_hit");
    if (f->hp > 0) return;
    f->hp = 0;
    if (f->kind == FO_HALLORAN && ag.boss_phase < APD_HAL_PHASES - 1) {
        hal_phase(ag.boss_phase + 1);
        return;
    }
    if (i == ag.boss_foe) defeated();
}

/* ---- the Scrapper: a tank that drives on the floor or the ceiling ---------- */
static void scrapper(ApdFoe *f) {
    int vx, vy;
    switch (f->state) {
    case 0:
        f->dir = APD_PX(ag.pl.x) < APD_PX(f->x) ? -1 : 1;
        f->x += f->dir * 300;
        f->x = iclamp(f->x, 24 * APD_FX, (W() - 24) * APD_FX);
        if (f->t > 100) { f->state = 1; f->t = 0; }
        break;
    case 1:
        if (f->t == 10 || f->t == 30 || f->t == 50) {
            for (int k = -1; k <= 1; k++) {
                apd_aim(f->x, f->y, 384, &vx, &vy);
                apd_add_eshot(ES_SHELL, f->x, f->y, vx + k * vy / 3, vy - k * vx / 3, 8);
            }
            apd_sfx("apd_cannon");
        }
        if (f->t > 70) { f->state = 2; f->t = 0; f->up = !f->up; }
        break;
    case 2: {
        int ty = f->up ? ceil_y(f->h) : floor_y(f->h);
        int step = 6 * APD_FX;
        if (iabs(ty - f->y) <= step) {
            f->y = ty;
            f->state = 0;
            f->t = 0;
            ag.shake = 8;
            apd_sfx("apd_stomp");
        } else f->y += ty > f->y ? step : -step;
        break;
    }
    }
}

/* ---- the Warden: a big walker that moves the floor ---------------------------- */
static const int PISTON_COL[4] = {6, 13, 26, 33};

static void pistons(ApdFoe *f, bool up) {
    int px = APD_PX(ag.pl.x) / APD_TILE;
    for (int k = 0; k < 4; k++) {
        int c = PISTON_COL[k];
        if (c + 1 >= ag.rw) continue;
        bool under_pl = px >= c - 1 && px <= c + 2;
        bool under_me = iabs(APD_PX(f->x) / APD_TILE - c) <= 3;
        for (int col = c; col <= c + 1; col++)
            for (int row = 1; row <= 8; row++) {
                bool from_floor = !f->up; /* the side he stands on rises */
                bool part_of = from_floor ? row >= 5 : row <= 4;
                if (!part_of) continue;
                if (up && !under_pl && !under_me) ag.tile[row][col] = T_WALL;
                else if (!up) ag.tile[row][col] = T_AIR;
            }
    }
}

static void warden(ApdFoe *f) {
    int vx, vy;
    switch (f->state) {
    case 0: /* walk */
        f->dir = APD_PX(ag.pl.x) < APD_PX(f->x) ? -1 : 1;
        f->x = iclamp(f->x + f->dir * 200, 28 * APD_FX, (W() - 28) * APD_FX);
        if (f->t > 90) { f->state = f->c % 2 ? 2 : 1; f->t = 0; f->c++; }
        break;
    case 1: /* bolts */
        if (f->t % 24 == 0 && f->t <= 48) {
            apd_aim(f->x, f->y, 420, &vx, &vy);
            apd_add_eshot(ES_BOLT, f->x, f->y - (f->up ? -10 : 10) * APD_FX, vx, vy, 10);
            apd_sfx("apd_cannon");
        }
        if (f->t > 70) { f->state = 0; f->t = 0; }
        break;
    case 2: /* slam: the floor rises in columns */
        if (f->t == 1) { ag.shake = 30; apd_sfx("apd_rumble"); }
        if (f->t == 30) pistons(f, true);
        if (f->t == 180) pistons(f, false);
        if (f->t > 190) { f->state = 3; f->t = 0; f->up = !f->up; }
        break;
    case 3: { /* cross to the other side */
        int ty = f->up ? ceil_y(f->h) : floor_y(f->h);
        int step = 5 * APD_FX;
        if (iabs(ty - f->y) <= step) {
            f->y = ty;
            f->state = 0;
            f->t = 0;
            ag.shake = 10;
            apd_sfx("apd_stomp");
        } else f->y += ty > f->y ? step : -step;
        break;
    }
    }
}

/* ---- Brackish: a tank-crab in the settling pool -------------------------------- */
static void brackish(ApdFoe *f) {
    int low = (H() - 6) * APD_FX, high = (H() - 64) * APD_FX;
    switch (f->state) {
    case 0: /* under the acid: out of reach, lobbing bubbles */
        f->flags |= FF_NOHIT;
        f->y += (low - f->y) / 8;
        f->dir = APD_PX(ag.pl.x) < APD_PX(f->x) ? -1 : 1;
        f->x = iclamp(f->x + f->dir * 160, 48 * APD_FX, (W() - 48) * APD_FX);
        if (f->t % 90 == 45)
            for (int k = -1; k <= 1; k++) {
                int e = apd_add_eshot(ES_BUBBLE, f->x, (H() - 40) * APD_FX, k * 300 + f->dir * 100, -1100, 8);
                if (e >= 0) ag.es[e].grav = 14;
            }
        if (f->t > 220) { f->state = 1; f->t = 0; apd_sfx("apd_splash"); }
        break;
    case 1: /* up: the eye shows; it lunges along the pool */
        f->flags &= ~FF_NOHIT;
        f->y += (high - f->y) / 6;
        if (f->t == 30) {
            for (int k = -2; k <= 2; k++) {
                int e = apd_add_eshot(ES_BUBBLE, f->x, f->y - 10 * APD_FX, k * 380, -1300, 8);
                if (e >= 0) ag.es[e].grav = 14;
            }
            f->dir = APD_PX(ag.pl.x) < APD_PX(f->x) ? -1 : 1;
        }
        if (f->t > 60 && f->t < 130) f->x = iclamp(f->x + f->dir * 700, 48 * APD_FX, (W() - 48) * APD_FX);
        if (f->t > 200) { f->state = 0; f->t = 0; }
        break;
    }
}

/* ---- the Lamprey: a long worm weaving floor to ceiling ------------------------- */
static void lamprey(ApdFoe *f, int i) {
    int vx, vy;
    f->x += f->dir * 410;
    if (APD_PX(f->x) < 30 || APD_PX(f->x) > W() - 30) f->dir = -f->dir;
    f->c = (f->c + 1) % 192;
    f->y = (H() / 2) * APD_FX + apd_sin32(f->c / 6) * 52;
    if (f->t % 100 == 60) {
        apd_aim(f->x, f->y, 380, &vx, &vy);
        apd_add_eshot(ES_SPARK, f->x, f->y, vx, vy, 8);
        apd_add_eshot(ES_SPARK, f->x, f->y, vx + vy / 4, vy - vx / 4, 8);
        apd_sfx("apd_spit");
    }
    /* the body follows */
    for (int k = 0; k < APD_MAX_FOES; k++) {
        ApdFoe *s = &ag.foe[k];
        if (!s->alive || s->kind != FO_LAMP_SEG || s->owner != i) continue;
        const ApdFoe *lead = &ag.foe[s->a];
        int dx = lead->x - s->x, dy = lead->y - s->y;
        int d = imax(1, (int)sqrt((double)(APD_PX(dx) * APD_PX(dx) + APD_PX(dy) * APD_PX(dy))));
        if (d > 13) {
            s->x += dx * (d - 13) / d;
            s->y += dy * (d - 13) / d;
        }
    }
}

/* ---- the Coilheart: a core behind four nodes --------------------------------- */
static void coilheart(ApdFoe *f, int i) {
    int vx, vy;
    int nodes = 0;
    for (int k = 0; k < APD_MAX_FOES; k++) nodes += ag.foe[k].alive && ag.foe[k].kind == FO_NODE && ag.foe[k].owner == i;
    if (f->state == 0) {
        if (f->t == 1 && nodes == 0)
            for (int k = 0; k < 4; k++) {
                int n = part(FO_NODE, i, f->x, f->y);
                if (n >= 0) ag.foe[n].c = k * 8;
            }
        f->flags |= FF_SHIELD;
        if (f->t > 10 && nodes == 0) { f->state = 1; f->t = 0; apd_sfx("apd_unlock"); }
    } else {
        f->flags &= ~FF_SHIELD;
        if (f->t > 360) { f->state = 0; f->t = 0; }
    }
    /* the nodes circle and fire */
    int k = 0;
    for (int n = 0; n < APD_MAX_FOES; n++) {
        ApdFoe *o = &ag.foe[n];
        if (!o->alive || o->kind != FO_NODE || o->owner != i) continue;
        int a = (f->t / 3 + o->c) & 31;
        o->x = f->x + apd_cos32(a) * 76;
        o->y = f->y + apd_sin32(a) * 58;
        if ((f->t + k * 30) % 120 == 0) {
            apd_aim(o->x, o->y, 330, &vx, &vy);
            apd_add_eshot(ES_SPARK, o->x, o->y, vx, vy, 8);
        }
        k++;
    }
    /* an arc sweeps the arena now and then */
    if (f->b++ % 420 == 200) {
        int from_left = APD_PX(ag.pl.x) > W() / 2;
        int e = apd_add_eshot(ES_BOLT, (from_left ? 20 : W() - 20) * APD_FX, (H() / 2) * APD_FX, from_left ? 400 : -400, 0, 15);
        if (e >= 0) { ag.es[e].h = (int16_t)(H() - 40); ag.es[e].w = 8; ag.es[e].life = 400; }
        apd_sfx("apd_zap");
    }
}

/* ---- Obverse: the other suit ------------------------------------------------------ */
static void obverse(ApdFoe *f) {
    int g = f->up ? -1 : 1;
    int pg = ag.pl.grav;
    int dx = APD_PX(ag.pl.x) - APD_PX(f->x);
    bool low = f->hp * 10 < f->maxhp * 3;
    if (f->state == 0) {
        /* keep a little way off */
        int want = iabs(dx) < 70 ? -isign(dx) : iabs(dx) > 110 ? isign(dx) : 0;
        if (want) f->x = iclamp(f->x + want * 340, 20 * APD_FX, (W() - 20) * APD_FX);
        f->dir = dx < 0 ? -1 : 1;
        /* stay on his side of the room: flip to it */
        if ((pg < 0) != (f->up != 0)) f->a++;
        else f->a = 0;
        if (f->a > 70) {
            f->state = 1;
            f->up = pg < 0;
            f->a = 0;
            apd_sfx("apd_flip");
        }
        /* sword beams: one at a time, up to five in a row once she is low */
        f->b++;
        int every = low ? 140 : 70;
        if (f->b >= every) {
            f->b = 0;
            f->c = low ? 5 : 1;
        }
        if (f->c > 0 && f->b % 8 == 0) {
            int e = apd_add_eshot(ES_BEAM, f->x + f->dir * 8 * APD_FX, f->y, f->dir * 820, 0, 12);
            (void)e;
            f->c--;
            apd_sfx("apd_beam");
        }
    } else {
        int ty = f->up ? ceil_y(f->h) : floor_y(f->h);
        int step = 5 * APD_FX;
        if (iabs(ty - f->y) <= step) { f->y = ty; f->state = 0; }
        else f->y += ty > f->y ? step : -step;
    }
    (void)g;
}

/* ---- Halloran: the walker, the heart, the head --------------------------------- */
static void hal_phase(int ph) {
    ApdFoe *f = main_foe();
    if (!f) return;
    ag.boss_phase = ph;
    ag.boss_t = 0;
    /* the parts of the last phase go */
    for (int i = 0; i < APD_MAX_FOES; i++)
        if (ag.foe[i].alive && ag.foe[i].owner == ag.boss_foe) ag.foe[i].alive = 0;
    memset(ag.es, 0, sizeof ag.es);
    f->hp = f->maxhp = (int16_t)APD_HAL_HP[ph];
    f->state = 0;
    f->t = 0;
    f->flags = 0;
    ag.shake = 24;
    apd_sfx("apd_bigboom");
    apd_burst(APD_PX(f->x), APD_PX(f->y), C_PINK, 20, 3);
    if (ph == 0) {
        f->w = 48; f->h = 40;
        f->x = (W() / 2) * APD_FX;
        f->y = (H() - APD_TILE - 24 - 20) * APD_FX;
        for (int s = -1; s <= 1; s += 2) {
            int l = part(FO_HAL_LEG, ag.boss_foe, f->x + s * 16 * APD_FX, (H() - APD_TILE - 12) * APD_FX);
            if (l >= 0) ag.foe[l].a = s;
        }
    } else if (ph == 1) {
        f->w = 32; f->h = 28;
    } else {
        f->w = 40; f->h = 36;
        f->x = (W() / 2) * APD_FX;
        f->y = 70 * APD_FX;
        f->flags |= FF_NOHIT;
    }
}

static void hal_joints(int owner, int x) {
    for (int k = 0; k < 3; k++) {
        int j = part(FO_HAL_JOINT, owner, x, (24 + k * 16) * APD_FX);
        if (j >= 0) ag.foe[j].hp = 40;
    }
}

static void halloran(ApdFoe *f, int i) {
    int vx, vy;
    ag.boss_t++;
    if (ag.boss_phase == 0) {
        /* the walker: the hand fires three volleys, then it rams */
        if (f->state == 0) {
            f->dir = APD_PX(ag.pl.x) < APD_PX(f->x) ? -1 : 1;
            f->x = iclamp(f->x + f->dir * 150, 40 * APD_FX, (W() - 40) * APD_FX);
            if (f->t % 50 == 25) {
                for (int k = -1; k <= 1; k++) {
                    apd_aim(f->x, f->y - 10 * APD_FX, 400, &vx, &vy);
                    apd_add_eshot(ES_BALL, f->x + f->dir * 20 * APD_FX, f->y - 10 * APD_FX, vx + k * vy / 4, vy - k * vx / 4, 10);
                }
                apd_sfx("apd_cannon");
                if (++f->c >= 3) { f->c = 0; f->state = 1; f->t = 0; }
            }
        } else if (f->state == 1) {
            /* the ram: across the floor to the wall */
            if (f->t < 20) ag.shake = 2;
            else {
                f->x += f->dir * 1024;
                if (APD_PX(f->x) < 40 || APD_PX(f->x) > W() - 40) {
                    f->x = iclamp(f->x, 40 * APD_FX, (W() - 40) * APD_FX);
                    ag.shake = 12;
                    apd_sfx("apd_stomp");
                    f->state = 0;
                    f->t = 0;
                }
            }
        }
        /* now and then it turns its shots back, then drops it */
        int cyc = ag.boss_t % 600;
        if (cyc == 400) { f->flags |= FF_REFLECT; apd_sfx("apd_shield"); }
        if (cyc == 0) f->flags &= ~FF_REFLECT;
        for (int k = 0; k < APD_MAX_FOES; k++) {
            ApdFoe *l = &ag.foe[k];
            if (l->alive && l->kind == FO_HAL_LEG && l->owner == i) l->x = f->x + l->a * 16 * APD_FX;
        }
    } else if (ag.boss_phase == 1) {
        /* the heart flies a figure of eight, dropping orbs and pink rings */
        int a = (ag.boss_t / 5) & 31;
        int tx = (W() / 2) * APD_FX + apd_cos32(a) * 220, ty = (H() / 2) * APD_FX + apd_sin32(a * 2) * 90;
        f->x += (tx - f->x) / 8;
        f->y += (ty - f->y) / 8;
        if (ag.boss_t % 70 == 35) {
            apd_aim(f->x, f->y, 180, &vx, &vy);
            int e = apd_add_eshot(ES_ORB, f->x, f->y, vx, vy, 10);
            if (e >= 0) ag.es[e].life = 300;
        }
        if (ag.boss_t % 110 == 0) {
            apd_aim(f->x, f->y, 380, &vx, &vy);
            apd_add_eshot(ES_RING, f->x, f->y, vx, vy, 12);
            apd_sfx("apd_ring");
        }
    } else {
        /* the head: shoot the neck's joints to bring it down; and every so often
         * it shuts itself away and calls down lightning */
        int joints = 0;
        for (int k = 0; k < APD_MAX_FOES; k++) joints += ag.foe[k].alive && ag.foe[k].kind == FO_HAL_JOINT;
        int cyc = ag.boss_t % 1800;
        bool storm = cyc >= 600; /* 20 s of lightning, then 10 s */
        if (cyc == 600) { apd_say("THE CROWN CALLS THE STORM"); apd_sfx("apd_alarm"); }
        if (f->state == 0) {
            f->y += (70 * APD_FX - f->y) / 10;
            f->flags |= FF_NOHIT;
            if (f->t == 1) hal_joints(i, f->x);
            if (f->t > 5 && joints == 0) { f->state = 1; f->t = 0; f->a = f->hp; apd_sfx("apd_crumble"); }
        } else {
            f->y += ((H() - 110) * APD_FX - f->y) / 10;
            if (!storm) f->flags &= ~FF_NOHIT;
            if (f->a - f->hp >= 250 || f->t > 600) { f->state = 0; f->t = 0; }
        }
        for (int k = 0; k < APD_MAX_FOES; k++) {
            ApdFoe *j = &ag.foe[k];
            if (!j->alive || j->kind != FO_HAL_JOINT) continue;
            if (storm) j->flags |= FF_NOHIT;
            else j->flags &= ~FF_NOHIT;
        }
        if (storm) {
            f->flags |= FF_NOHIT;
            if (cyc % 50 == 0) {
                static const int OFF[5] = {0, 48, -64, 24, -32};
                int x = iclamp(APD_PX(ag.pl.x) + OFF[(cyc / 50) % 5], 24, W() - 24);
                int e = apd_add_eshot(ES_LIGHTNING, x * APD_FX, (H() / 2) * APD_FX, 0, 0, 20);
                if (e >= 0) ag.es[e].life = 80;
                apd_sfx("apd_zap");
            }
        }
        if (ag.boss_t % 90 == 45) {
            apd_aim(f->x, f->y, 300, &vx, &vy);
            apd_add_eshot(ES_RING, f->x, f->y, vx, vy, 12);
        }
        if (ag.boss_t % 120 == 0) {
            int e = apd_add_eshot(ES_ORB, f->x, f->y, (ag.boss_t % 240 ? 200 : -200), 120, 10);
            if (e >= 0) ag.es[e].life = 300;
        }
    }
}

void apd_boss_update(int i) {
    ApdFoe *f = &ag.foe[i];
    switch (f->kind) {
    case FO_SCRAPPER: scrapper(f); break;
    case FO_WARDEN: warden(f); break;
    case FO_BRACKISH: brackish(f); break;
    case FO_LAMPREY: lamprey(f, i); break;
    case FO_COILHEART: coilheart(f, i); break;
    case FO_OBVERSE: obverse(f); break;
    case FO_HALLORAN: halloran(f, i); break;
    default: break; /* parts move with their bodies */
    }
}
