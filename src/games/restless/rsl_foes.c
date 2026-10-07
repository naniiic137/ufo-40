/* RESTLESS - the Hollow Host's rank and file, their shots, and where they
 * come from. Foes' x, y are their middle, fixed point. */
#include "rsl.h"

int rsl_main_h(void);

const char *const RSL_FOE_NAME[FO_KINDS] = {
    "", "LACKEY", "SPITBLOOM", "STARSPITTER", "HIVE", "GNAT", "WISP", "TUMBLER", "BOG TOAD", "BOULDERKIN",
    "SHELLCRAB", "LEAPER", "WEEPER", "SHADE", "GLOOMCROW", "FIREMAW", "SHIELDLING", "HULK", "CLOCKFIRE",
    "STORMCLOUD", "THE BRIDGEKEEPER", "OLD RATTLE", "THREE-TONGUE", "GUST", "GASH", "THE RAMMER",
    "THE HOLLOW KING", "SKULL"};
/* hit points as the original's (bosses too) */
const int RSL_FOE_HP[FO_KINDS] = {0, 6, 42, 20, 24, 1, 6, 6, 10, 24, 1, 1, 20, 6, 42, 42, 1, 80, 5, 42,
                                  80, 150, 60, 180, 220, 150, 300, 30};

static const int FOE_W[FO_KINDS] = {0, 10, 16, 14, 16, 6, 10, 12, 14, 18, 14, 10, 16, 10, 22, 18, 12, 24, 14, 26,
                                    30, 26, 18, 28, 20, 26, 96, 14};
static const int FOE_H[FO_KINDS] = {0, 20, 20, 16, 16, 6, 10, 12, 12, 18, 10, 10, 14, 18, 16, 18, 14, 30, 14, 14,
                                    34, 28, 16, 20, 30, 24, 88, 14};


int rsl_count_foes(int kind) {
    int n = 0;
    for (int i = 0; i < RSL_MAX_FOES; i++) n += rg.foe[i].alive && rg.foe[i].kind == kind;
    return n;
}

int rsl_foes_alive(bool bosses) {
    int n = 0;
    for (int i = 0; i < RSL_MAX_FOES; i++)
        n += rg.foe[i].alive && (bosses || !RSL_IS_BOSS(rg.foe[i].kind)) && rg.foe[i].kind != FO_CRAB;
    return n;
}

int rsl_spawn_foe(int kind, int x, int y, int var) {
    for (int i = 0; i < RSL_MAX_FOES; i++)
        if (!rg.foe[i].alive) {
            RslFoe *f = &rg.foe[i];
            memset(f, 0, sizeof *f);
            f->alive = 1;
            f->kind = (uint8_t)kind;
            f->var = (uint8_t)var;
            f->hp = f->maxhp = (int16_t)RSL_FOE_HP[kind];
            f->w = FOE_W[kind];
            f->h = FOE_H[kind];
            f->x = x;
            f->y = y;
            f->spawn = -1;
            f->dir = rg.pl.x >= x ? 1 : -1;
            f->ox = x;
            f->oy = y;
            return i;
        }
    return -1;
}

int rsl_add_eshot(int kind, int x, int y, int vx, int vy) {
    for (int i = 0; i < RSL_MAX_ESHOTS; i++)
        if (!rg.es[i].alive) {
            RslEShot *e = &rg.es[i];
            memset(e, 0, sizeof *e);
            e->alive = 1;
            e->kind = (uint8_t)kind;
            e->x = x;
            e->y = y;
            e->vx = vx;
            e->vy = vy;
            e->hp = 1;
            e->life = 600;
            e->owner = -1;
            e->w = e->h = 6;
            switch (kind) {
            case ES_SPEAR: e->w = 14; e->h = 3; break;
            case ES_STAR: e->w = e->h = 7; break;
            case ES_FIRE: e->w = e->h = 7; e->shootable = 1; break;
            case ES_TEAR: e->w = 4; e->h = 6; break;
            case ES_ROCK: e->w = e->h = 12; break;
            case ES_BOMB: e->w = e->h = 8; e->shootable = 1; e->hp = 1; break;
            case ES_FLAME: e->w = 12; e->h = 10; break;
            case ES_BOLT: e->w = 4; e->h = 16; break;
            case ES_TONGUE: e->w = 4; e->h = 4; break;
            case ES_TRAP: e->w = 18; e->h = 4; break;
            case ES_BLAZE: e->w = 14; e->h = 12; break;
            case ES_BLADE: e->w = 14; e->h = 10; break;
            case ES_STONE: e->w = e->h = 6; break;
            default: break;
            }
            return i;
        }
    return -1;
}

static void aimed(int kind, int fx, int fy, int spread_steps, int n, int speed) {
    int a = rsl_dir_to(rg.pl.x - fx, rg.pl.y - 17 * RSL_FX - fy);
    for (int k = 0; k < n; k++) {
        int aa = a + (k - (n - 1) / 2) * spread_steps;
        rsl_add_eshot(kind, fx, fy, rsl_cos(aa) * speed / 256, rsl_sin(aa) * speed / 256);
    }
}

/* ------------------------------------------------------------------------ */
/* hurting and killing                                                      */

void rsl_kill_foe(int i, bool drops) {
    RslFoe *f = &rg.foe[i];
    if (!f->alive) return;
    int x = RSL_PX(f->x), y = RSL_PX(f->y);
    f->alive = 0;
    rg.kills++;
    rsl_burst(x, y, RSL_IS_BOSS(f->kind) ? C_YELLOW : C_WHITE, RSL_IS_BOSS(f->kind) ? 30 : 8, 160);
    rsl_sfx(RSL_IS_BOSS(f->kind) ? "rsl_bossdie" : "rsl_pop");
    if (f->spawn >= 0) {
        RslSpawn *s = &rg.spawn[f->spawn];
        if (s->child == i) s->child = -1;
        if (s->var || (drops && (f->kind == FO_BLOOM || f->kind == FO_SPITTER || f->kind == FO_HIVE ||
                      f->kind == FO_WEEPER || f->kind == FO_CROW || f->kind == FO_FIREMAW || f->kind == FO_SHIELD ||
                      f->kind == FO_HULK || f->kind == FO_SHADE || f->kind == FO_LACKEY)))
            s->used = 1;
    }
    if (RSL_IS_BOSS(f->kind)) {
        rsl_boss_dead(i);
        return;
    }
    if (!drops) return;
    if (f->kind == FO_WISP && f->a >= 0 && f->a < 16) {
        rg.flock_kills[f->a]++;
        if (rg.flock_kills[f->a] == 4) rsl_drop_item(IT_JAR, f->x, f->y, 0);
    }
    if (rg.bonus_pending > 0) {
        rg.bonus_pending--;
        rsl_bonus_drop(f->kind, f->x, f->y);
    } else {
        rsl_kill_drop(f->kind, f->x, f->y, f->flags);
    }
    if (f->kind == FO_CLOUD) rsl_drop_item(IT_IDOL, f->x, f->y, 0);
}

void rsl_hurt_foe(int i, int dmg, int push) {
    RslFoe *f = &rg.foe[i];
    if (!f->alive || (f->flags & FF_HARMLESS)) return;
    switch (f->kind) {
    case FO_WEEPER:
        if (f->state == 0) { f->flash = 2; rsl_sfx("rsl_tink"); return; } /* eye shut */
        break;
    case FO_SHIELD:
        /* the shield faces him: only a shot from behind, or with it down */
        if (f->state == 0 && push == f->dir * -1) { rsl_sfx("rsl_tink"); return; }
        break;
    case FO_CRAB:
        f->state = 2;
        f->st = 240;
        f->flags |= FF_HARMLESS;
        f->flash = 6;
        rg.kills++;
        rsl_sfx("rsl_pop");
        if (rg.bonus_pending > 0) { rg.bonus_pending--; rsl_bonus_drop(f->kind, f->x, f->y); }
        return;
    case FO_CROW:
        /* pushed back by every hit */
        f->x += push * 4 * RSL_FX;
        break;
    case FO_RAMMER:
        f->c += dmg; /* enough in one dash turns it back */
        break;
    default: break;
    }
    f->hp = (int16_t)(f->hp - dmg);
    f->flash = 6;
    rsl_sfx("rsl_hit");
    if (f->hp <= 0) rsl_kill_foe(i, !(f->flags & FF_NODROP));
}

/* ------------------------------------------------------------------------ */
/* moving on the ground                                                     */

static int feet(const RslFoe *f) { return RSL_PX(f->y) + f->h / 2; }

static bool foe_on_floor(const RslFoe *f) {
    int y = feet(f), x = RSL_PX(f->x);
    return (y % RSL_TILE) == 0 && (rsl_floor_px(x - f->w / 4, y) || rsl_floor_px(x + f->w / 4, y));
}

/* gravity and landing; returns true while standing */
static bool fall(RslFoe *f, int grav) {
    if (foe_on_floor(f) && f->vy >= 0) { f->vy = 0; f->flags |= FF_GROUNDED; return true; }
    f->flags &= (uint8_t)~FF_GROUNDED;
    f->vy = imin(f->vy + grav, RSL_MAX_FALL);
    int oldf = feet(f);
    f->y += f->vy;
    int nf = feet(f);
    if (f->vy > 0)
        for (int y = oldf + 1; y <= nf; y++)
            if (y % RSL_TILE == 0 && (rsl_floor_px(RSL_PX(f->x) - f->w / 4, y) || rsl_floor_px(RSL_PX(f->x) + f->w / 4, y))) {
                f->y = (y - f->h / 2) * RSL_FX;
                f->vy = 0;
                f->flags |= FF_GROUNDED;
                return true;
            }
    if (f->vy < 0 && rsl_solid_px(RSL_PX(f->x), RSL_PX(f->y) - f->h / 2)) f->vy = 0;
    return false;
}

/* walk; turn at walls (and at ledges when edge_turn); true if it turned */
static bool walk(RslFoe *f, int speed, bool edge_turn) {
    int nx = f->x + f->dir * speed;
    int lead = RSL_PX(nx) + f->dir * f->w / 2;
    int top = RSL_PX(f->y) - f->h / 2 + 2, bot = feet(f) - 2;
    bool wall = rsl_solid_px(lead, top) || rsl_solid_px(lead, bot) || rsl_solid_px(lead, (top + bot) / 2);
    bool edge = edge_turn && (f->flags & FF_GROUNDED) && !rsl_floor_px(lead, feet(f));
    if (wall || edge || lead < 2 || lead > rg.mw * RSL_TILE - 2) {
        f->dir = -f->dir;
        return true;
    }
    f->x = nx;
    return false;
}

static int dxp(const RslFoe *f) { return RSL_PX(rg.pl.x) - RSL_PX(f->x); }
static int dyp(const RslFoe *f) { return RSL_PX(rg.pl.y) - 11 - RSL_PX(f->y); }

static void fly_toward(RslFoe *f, int tx, int ty, int speed) {
    int a = rsl_dir_to(tx - f->x, ty - f->y);
    f->x += rsl_cos(a) * speed / 256;
    f->y += rsl_sin(a) * speed / 256;
}

/* ------------------------------------------------------------------------ */
/* each kind                                                                */

static void lackey(RslFoe *f) {
    if (f->state == 0) {
        /* climbing out of the ground */
        f->flags |= FF_HARMLESS;
        if (f->t >= 24) { f->state = 1; f->flags &= (uint8_t)~FF_HARMLESS; }
        return;
    }
    bool g = fall(f, RSL_GRAV);
    int dx = dxp(f), dy = dyp(f);
    if (f->state == 2) {
        /* the red one's swing */
        f->st++;
        if (f->st == 10) {
            int e = rsl_add_eshot(ES_BLADE, f->x + f->dir * 12 * RSL_FX, f->y, 0, 0);
            if (e >= 0) { rg.es[e].life = 10; rg.es[e].owner = (int)(f - rg.foe); }
            rsl_sfx("rsl_swish");
        }
        if (f->st > 34) { f->state = 1; f->st = 0; }
        return;
    }
    if (!g) return;
    if (f->flags & FF_GREEN) {
        if (f->b > 0) f->b--;
        if (iabs(dx) < 140 && iabs(dx) > 36 && iabs(dy) < 60) {
            f->dir = dx > 0 ? 1 : -1;
            if (f->b == 0) {
                rsl_add_eshot(ES_SPEAR, f->x + f->dir * 6 * RSL_FX, f->y - 4 * RSL_FX, f->dir * 600, 0);
                f->b = 110;
                f->st = 12;
                rsl_sfx("rsl_throw");
            }
            if (f->st > 0) { f->st--; return; }
            if (iabs(dx) > 90) walk(f, 150, true);
            return;
        }
    } else if (iabs(dx) < 26 && iabs(RSL_PX(rg.pl.y) - feet(f)) < 6) {
        f->dir = dx > 0 ? 1 : -1;
        f->state = 2;
        f->st = 0;
        return;
    }
    if (f->t % 40 == 0) f->dir = dx > 0 ? 1 : -1;
    /* a ledge stops it, unless he's down there; a step up toward him it hops */
    int lead = RSL_PX(f->x) + f->dir * (f->w / 2 + 2);
    if (RSL_PX(rg.pl.y) < feet(f) - 8 && rsl_solid_px(lead, feet(f) - 4) && !rsl_solid_px(lead, feet(f) - 20) &&
        !rsl_solid_px(lead, feet(f) - 28)) {
        f->vy = -720;
        f->x += f->dir * 256;
        return;
    }
    walk(f, 154, RSL_PX(rg.pl.y) <= feet(f) + 4);
}

static void bloom(RslFoe *f) {
    fall(f, RSL_GRAV);
    f->dir = dxp(f) > 0 ? 1 : -1;
    if (f->t % 120 == 60 && rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 0)) {
        aimed(ES_BALL, f->x, f->y - 6 * RSL_FX, 3, 3, 420);
        rsl_sfx("rsl_spit");
    }
}

static void spitter(RslFoe *f) {
    fall(f, RSL_GRAV);
    if (f->t % 150 == 70 && rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 0)) {
        int lean = dxp(f) > 0 ? 100 : -100;
        for (int k = -1; k <= 1; k++) {
            int e = rsl_add_eshot(ES_STAR, f->x, f->y - 8 * RSL_FX, k * 150 + lean, -800);
            if (e >= 0) rg.es[e].grav = 20;
        }
        rsl_sfx("rsl_throw");
    }
}

static void hive(RslFoe *f) {
    if (!rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 0)) return;
    int every = imax(50, 110 - rg.deaths * 10);
    if (f->t % every == every - 1) {
        int n = 0;
        for (int i = 0; i < RSL_MAX_FOES; i++) n += rg.foe[i].alive && rg.foe[i].kind == FO_GNAT && rg.foe[i].b == (int)(f - rg.foe) + 1;
        if (n < 4) {
            int g = rsl_spawn_foe(FO_GNAT, f->x, f->y + 6 * RSL_FX, 0);
            if (g >= 0) rg.foe[g].b = (int)(f - rg.foe) + 1;
        }
    }
}

static void gnat(RslFoe *f) {
    bool frenzy = rg.deaths >= 3;
    int sp = frenzy ? 384 : 205;
    /* they swoop at him and overshoot, then turn and come again */
    int jx = rsl_sin(f->t * 3 + f->b * 11) * 10, jy = rsl_cos(f->t * 4 + f->b * 7) * 6;
    int a = rsl_dir_to(rg.pl.x + jx - f->x, rg.pl.y - 11 * RSL_FX + jy - f->y);
    f->vx += rsl_cos(a) * sp / 2048;
    f->vy += rsl_sin(a) * sp / 2048;
    int v = rsl_dist(f->vx, f->vy);
    if (v > sp) { f->vx = f->vx * sp / v; f->vy = f->vy * sp / v; }
    f->x += f->vx;
    f->y += f->vy;
    if (frenzy) f->var = 1;
}

static void wisp(RslFoe *f) {
    f->x += f->dir * 300;
    f->y = f->b + rsl_sin(f->t * 2 + f->var * 8) * 22 * RSL_FX / 256;
    if (!rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 48)) f->alive = 0;
}

static void tumbler(RslFoe *f) {
    bool g = fall(f, RSL_GRAV);
    if (f->state == 0) {
        f->flags |= FF_HARMLESS;
        if (g) { f->st++; if (f->st > 20) { f->state = 1; f->flags &= (uint8_t)~FF_HARMLESS; f->dir = dxp(f) > 0 ? 1 : -1; } }
        return;
    }
    walk(f, 360, false);
    if (g && f->t % 50 == 0) f->vy = -640;
    if (!rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 120)) f->alive = 0;
}

static void toad(RslFoe *f) {
    if (f->state == 0) {
        /* climbing out of its hole, up to where it stands (c): it can be
         * shot as it peeks out, but can't hurt yet */
        f->y = imax(f->c, f->y - 128);
        if (f->t >= 24) { f->y = f->c; f->state = 1; }
        return;
    }
    bool g = fall(f, 40);
    int dx = dxp(f), dy = dyp(f);
    if (f->state == 2) {
        if (++f->st > 26) { f->state = 1; f->st = 0; }
        return;
    }
    if (g) {
        f->a = 0;
        if (iabs(dx) < 40 && iabs(dy) < 18 && f->b <= 0) {
            f->dir = dx > 0 ? 1 : -1;
            f->state = 2;
            f->st = 0;
            f->b = 70;
            int e = rsl_add_eshot(ES_TONGUE, f->x, f->y - 2 * RSL_FX, f->dir, 0);
            if (e >= 0) { rg.es[e].owner = (int)(f - rg.foe); rg.es[e].life = 26; }
            rsl_sfx("rsl_tongue");
            return;
        }
        if (f->b > 0) f->b--;
        if (f->t % 48 == 0) {
            f->dir = dx > 0 ? 1 : -1;
            f->vy = -660;
            f->a = 1;
        }
    } else if (f->a) {
        walk(f, 240, false);
    }
}

static void boulder(RslFoe *f) {
    bool g = fall(f, 30);
    if (g) f->vy = -900;
    walk(f, 300, false);
    if (!rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 80)) f->alive = 0;
}

static void crab(RslFoe *f) {
    fall(f, RSL_GRAV);
    if (f->state == 2) {
        if (--f->st <= 0) { f->state = 1; f->flags &= (uint8_t)~FF_HARMLESS; }
        return;
    }
    walk(f, 128, true);
}

static void leaper(RslFoe *f) {
    if (f->state == 0) {
        f->vx = (dxp(f) > 0 ? 1 : -1) * 150;
        f->vy = -1060;
        f->state = 1;
        f->a = f->y;
    }
    f->x += f->vx;
    f->vy += 30;
    f->y += f->vy;
    if (f->vy > 0 && f->y > f->a + 8 * RSL_FX) f->alive = 0;
}

static void weeper(RslFoe *f) {
    int c = f->t % 180;
    f->state = c >= 90;
    if (f->state && c % 40 == 10 && rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 0)) {
        int e = rsl_add_eshot(ES_TEAR, f->x, f->y + 8 * RSL_FX, 0, 64);
        if (e >= 0) rg.es[e].grav = 22;
        rsl_sfx("rsl_drip");
    }
}

static void shade(RslFoe *f) {
    bool g = fall(f, RSL_GRAV);
    int dx = dxp(f), dy = dyp(f);
    if (f->b > 0) f->b--;
    if (g) {
        f->dir = dx > 0 ? 1 : -1;
        if (dy < -28 && f->b == 0) { f->vy = -1080; f->b = 50; }
        else if (dy > 28 && f->b == 0 && rsl_oneway_px(RSL_PX(f->x), feet(f))) { f->y += 3 * RSL_FX; f->vy = 64; f->b = 40; }
        else {
            int lead = RSL_PX(f->x) + f->dir * 10;
            if (!rsl_floor_px(lead, feet(f)) && f->b == 0) { f->vy = -1000; f->b = 30; }
        }
    }
    walk(f, 410, false);
    if (!rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 160)) f->alive = 0;
}

static void crow(RslFoe *f) {
    int side = f->x > rg.pl.x ? 1 : -1;
    int tx = rg.pl.x + side * 96 * RSL_FX, ty = rg.pl.y - 56 * RSL_FX;
    int lo = (rg.cam_y + 24) * RSL_FX;
    if (ty < lo) ty = lo;
    if (rsl_dist((tx - f->x) / RSL_FX, (ty - f->y) / RSL_FX) > 4) fly_toward(f, tx, ty, 200);
    f->dir = -side;
    int c = f->t % 140;
    bool spread = rg.deaths >= 5;
    if (rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 0)) {
        if (c == 100 || (spread && (c == 112 || c == 124))) {
            aimed(ES_BALL, f->x, f->y, spread ? 4 : 2, 3, 460);
            rsl_sfx("rsl_spit");
        }
    }
}

static void firemaw(RslFoe *f) {
    fall(f, RSL_GRAV);
    f->dir = dxp(f) > 0 ? 1 : -1;
    int c = f->t % 140;
    if ((c == 80 || c == 92 || c == 104) && rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 0)) {
        aimed(ES_FIRE, f->x + f->dir * 8 * RSL_FX, f->y - 4 * RSL_FX, 0, 1, 380);
        rsl_sfx("rsl_fireball");
    }
}

static void shieldling(RslFoe *f) {
    fall(f, RSL_GRAV);
    int c = f->t % 160;
    if (c < 120) { f->state = 0; f->dir = dxp(f) > 0 ? 1 : -1; }
    else f->state = 1;
    if (c == 132 && rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 0)) {
        int dx = dxp(f);
        int e = rsl_add_eshot(ES_STONE, f->x, f->y - 6 * RSL_FX, iclamp(dx * 4, -500, 500), -700);
        if (e >= 0) rg.es[e].grav = 22;
        rsl_sfx("rsl_throw");
    }
}

static void hulk(RslFoe *f) {
    fall(f, RSL_GRAV);
    if (f->t % 90 == 0) f->dir = dxp(f) > 0 ? 1 : -1;
    walk(f, 90, true);
}

static void clockfire(RslFoe *f) {
    fly_toward(f, rg.pl.x, rg.pl.y - 11 * RSL_FX, 256 + imin(rg.deaths, 6) * 16);
}

static void cloud(RslFoe *f) {
    int ty = (rg.cam_y + 34) * RSL_FX;
    fly_toward(f, rg.pl.x, ty, 230);
    if (f->t % 90 == 60) {
        rsl_add_eshot(ES_BOLT, f->x, f->y + 10 * RSL_FX, 0, 1024);
        rsl_sfx("rsl_zap");
    }
}

/* ------------------------------------------------------------------------ */

static bool touches_player(const RslFoe *f) {
    const RslPlayer *p = &rg.pl;
    int ph = p->duck ? RSL_PH_DUCK : RSL_PH;
    int px0 = RSL_PX(p->x) - 4, px1 = RSL_PX(p->x) + 4, py0 = RSL_PX(p->y) - ph + 2, py1 = RSL_PX(p->y) - 1;
    int fx = RSL_PX(f->x), fy = RSL_PX(f->y);
    int hw = f->w / 2 - 2, hh = f->h / 2 - 2;
    return px1 > fx - hw && px0 < fx + hw && py1 > fy - hh && py0 < fy + hh;
}

void rsl_foes_update(void) {
    for (int i = 0; i < RSL_MAX_FOES; i++) {
        RslFoe *f = &rg.foe[i];
        if (!f->alive) continue;
        f->t++;
        f->ox = f->x;
        f->oy = f->y;
        if (f->flash > 0) f->flash--;
        if (RSL_IS_BOSS(f->kind)) {
            rsl_boss_update(i);
        } else {
            switch (f->kind) {
            case FO_LACKEY: lackey(f); break;
            case FO_BLOOM: bloom(f); break;
            case FO_SPITTER: spitter(f); break;
            case FO_HIVE: hive(f); break;
            case FO_GNAT: gnat(f); break;
            case FO_WISP: wisp(f); break;
            case FO_TUMBLER: tumbler(f); break;
            case FO_TOAD: toad(f); break;
            case FO_BOULDER: boulder(f); break;
            case FO_CRAB: crab(f); break;
            case FO_LEAPER: leaper(f); break;
            case FO_WEEPER: weeper(f); break;
            case FO_SHADE: shade(f); break;
            case FO_CROW: crow(f); break;
            case FO_FIREMAW: firemaw(f); break;
            case FO_SHIELD: shieldling(f); break;
            case FO_HULK: hulk(f); break;
            case FO_CLOCKFIRE: clockfire(f); break;
            case FO_CLOUD: cloud(f); break;
            default: break;
            }
        }
        if (!f->alive) continue;
        /* fell out of the world */
        if (RSL_PX(f->y) > rg.mh * RSL_TILE + 40 || (!(f->flags & FF_PIT) && RSL_PX(f->y) + f->h / 2 >= rsl_main_h() * RSL_TILE)) {
            f->alive = 0;
            if (f->spawn >= 0) rg.spawn[f->spawn].child = -1;
            continue;
        }
        /* wandered far off: gone, and its spot can make another later */
        if (!RSL_IS_BOSS(f->kind) && !(f->flags & FF_PIT) && !rsl_in_view(RSL_PX(f->x), RSL_PX(f->y), 220)) {
            if (f->spawn >= 0) rg.spawn[f->spawn].child = -1;
            f->alive = 0;
            continue;
        }
        bool peeking = f->kind == FO_TOAD && f->state == 0;
        if (!(f->flags & FF_HARMLESS) && !peeking && f->kind != FO_KING && touches_player(f)) rsl_player_hurt(f->kind);
    }
}

/* ------------------------------------------------------------------------ */
/* foe shots                                                                */

static bool eshot_touches_player(const RslEShot *e) {
    const RslPlayer *p = &rg.pl;
    int ph = p->duck ? RSL_PH_DUCK : RSL_PH;
    int px0 = RSL_PX(p->x) - 4, px1 = RSL_PX(p->x) + 4, py0 = RSL_PX(p->y) - ph + 2, py1 = RSL_PX(p->y) - 1;
    int ex = RSL_PX(e->x), ey = RSL_PX(e->y);
    return px1 > ex - e->w / 2 && px0 < ex + e->w / 2 && py1 > ey - e->h / 2 && py0 < ey + e->h / 2;
}

void rsl_eshots_update(void) {
    for (int i = 0; i < RSL_MAX_ESHOTS; i++) {
        RslEShot *e = &rg.es[i];
        if (!e->alive) continue;
        e->t++;
        if (--e->life <= 0) { e->alive = 0; continue; }
        switch (e->kind) {
        case ES_TONGUE: {
            /* stretches out from the toad's mouth and back */
            const RslFoe *f = e->owner >= 0 ? &rg.foe[e->owner] : NULL;
            if (!f || !f->alive) { e->alive = 0; continue; }
            int len = e->t < 13 ? e->t * 5 / 2 : (26 - e->t) * 5 / 2;
            e->x = f->x + e->vx * (6 + len / 2) * RSL_FX;
            e->y = f->y - 2 * RSL_FX;
            e->w = len + 4;
            break;
        }
        case ES_BLADE: {
            const RslFoe *f = e->owner >= 0 ? &rg.foe[e->owner] : NULL;
            if (!f || !f->alive) { e->alive = 0; continue; }
            e->x = f->x + f->dir * 12 * RSL_FX;
            e->y = f->y - 2 * RSL_FX;
            break;
        }
        default:
            e->vy += e->grav;
            e->x += e->vx;
            e->y += e->vy;
            break;
        }
        int ex = RSL_PX(e->x), ey = RSL_PX(e->y);
        if (e->kind != ES_TONGUE && e->kind != ES_BLADE && e->kind != ES_FLAME && e->kind != ES_BLAZE &&
            e->kind != ES_BOLT && rsl_solid_px(ex, ey)) {
            if (e->kind == ES_BOMB) {
                int b = rsl_add_eshot(ES_BLAZE, e->x, e->y - 4 * RSL_FX, 0, 0);
                if (b >= 0) rg.es[b].life = 24;
                rsl_sfx("rsl_boom");
            }
            if (e->owner == -2) {
                /* Three-Tongue's fire sets the floor alight */
                int b = rsl_add_eshot(ES_BLAZE, e->x, ((ey / RSL_TILE) * RSL_TILE - 6) * RSL_FX, 0, 0);
                if (b >= 0) rg.es[b].life = 90;
            }
            rsl_burst(ex, ey, e->kind == ES_ROCK ? C_BROWN : C_GREY, 4, 80);
            e->alive = 0;
            continue;
        }
        if (e->kind == ES_BOMB && rsl_floor_px(ex, ey + 4) && e->vy > 0) {
            int b = rsl_add_eshot(ES_BLAZE, e->x, e->y - 4 * RSL_FX, 0, 0);
            if (b >= 0) rg.es[b].life = 24;
            rsl_sfx("rsl_boom");
            e->alive = 0;
            continue;
        }
        if (e->kind == ES_BOLT && RSL_PX(e->y) > rg.cam_y + SCREEN_H + 8) { e->alive = 0; continue; }
        if (!rsl_in_view(ex, ey, 64)) { e->alive = 0; continue; }
        if (eshot_touches_player(e)) {
            rsl_player_hurt(100 + e->kind);
            if (e->kind != ES_FLAME && e->kind != ES_BLAZE && e->kind != ES_TONGUE && e->kind != ES_BLADE &&
                e->kind != ES_TRAP && rg.state == RS_PLAY)
                e->alive = 0;
        }
        /* a trap's spear runs through foes too */
        if (e->kind == ES_TRAP)
            for (int f = 0; f < RSL_MAX_FOES; f++) {
                RslFoe *fo = &rg.foe[f];
                if (!fo->alive || RSL_IS_BOSS(fo->kind) || (fo->flags & FF_HARMLESS)) continue;
                if (iabs(fo->x - e->x) < (fo->w / 2 + 8) * RSL_FX && iabs(fo->y - e->y) < (fo->h / 2 + 2) * RSL_FX)
                    rsl_kill_foe(f, true);
            }
    }
}

/* ------------------------------------------------------------------------ */
/* where foes come from                                                     */

static int first_free_foe(void) {
    for (int i = 0; i < RSL_MAX_FOES; i++)
        if (!rg.foe[i].alive) return i;
    return -1;
}

static bool spawn_in_view(const RslSpawn *s, int margin) {
    return rsl_in_view(s->tx * RSL_TILE + 8, s->ty * RSL_TILE + 8, margin);
}

static int ground_y_for(int kind, int tx, int ty) {
    /* the middle of a foe standing on the floor under tile (tx, ty) */
    int g = rsl_ground_below(tx * RSL_TILE + 8, ty * RSL_TILE);
    if (g < 0) g = (ty + 1) * RSL_TILE;
    return (g - FOE_H[kind] / 2) * RSL_FX;
}

static void flock(int x, int y, int dir) {
    int id = rg.flock_n++ & 15;
    rg.flock_kills[id] = 0;
    for (int k = 0; k < 4; k++) {
        int f = rsl_spawn_foe(FO_WISP, x - dir * k * 22 * RSL_FX, y, k);
        if (f >= 0) {
            rg.foe[f].a = id;
            rg.foe[f].b = y;
            rg.foe[f].dir = dir;
        }
    }
    rsl_sfx("rsl_wisps");
}

static void ambient(void) {
    const RslHalf *H = &RSL_HALF[rg.half];
    int px = RSL_PX(rg.pl.x), py = RSL_PX(rg.pl.y);
    int dl = rg.deaths;
    /* lackeys climb out of the ground, more of them with every death */
    if (H->lackey_every > 0) {
        if (rg.lackey_t > 0) rg.lackey_t--;
        int cap = imin(5, 2 + dl / 2);
        if (rg.lackey_t == 0 && rsl_count_foes(FO_LACKEY) < cap) {
            for (int tries = 0; tries < 6; tries++) {
                int side = rng_range(&rg.rng, 0, 99) < 70 ? rg.pl.face : -rg.pl.face;
                int x = px + side * rng_range(&rg.rng, 56, 140);
                if (x < rg.cam_x + 8 || x > rg.cam_x + SCREEN_W - 8) continue;
                int g = rsl_ground_below(x, py - 48);
                if (g < 0 || g > py + 48 || g < rg.cam_y + 24) continue;
                if (rsl_solid_px(x, g - 8) || rsl_solid_px(x, g - 24)) continue;
                int tx = x / RSL_TILE;
                /* not on a trap's button */
                bool bad = false;
                for (int s = 0; s < rg.nspawn; s++)
                    if (rg.spawn[s].type == SP_BUTTON && iabs(rg.spawn[s].tx - tx) <= 1) bad = true;
                if (bad) continue;
                int f = rsl_spawn_foe(FO_LACKEY, (tx * RSL_TILE + 8) * RSL_FX, (g - 10) * RSL_FX, 0);
                if (f >= 0) {
                    int odds = dl >= 4 ? 3 : 8; /* green spear throwers: 1 in 8, 1 in 3 from 4 deaths */
                    if (rng_range(&rg.rng, 1, odds) == 1) rg.foe[f].flags |= FF_GREEN;
                    rg.foe[f].flags |= FF_HARMLESS;
                }
                break;
            }
            rg.lackey_t = imax(50, H->lackey_every - dl * 12) + rng_range(&rg.rng, 0, 40);
        }
    }
    /* from 3 deaths, wisps that don't come otherwise */
    if (dl >= 3) {
        if (--rg.wisp_t <= 0) {
            flock((rg.cam_x + SCREEN_W + 12) * RSL_FX, (py - 44) * RSL_FX, -1);
            rg.wisp_t = imax(420, 1000 - dl * 80);
        }
    }
}

void rsl_spawners_update(void) {
    if (rg.quiet_t > 0) { rg.quiet_t--; return; }
    if (rg.no_spawn) return;
    int mh = rsl_main_h();
    int dl = rg.deaths;
    /* the clock ran out: fire spirits from the edges */
    if (rg.time <= 0 && !rg.no_timer && rg.pit < 0) {
        if (--rg.demon_t <= 0) {
            int side = rng_range(&rg.rng, 0, 1) ? 1 : -1;
            int x = side > 0 ? rg.cam_x + SCREEN_W + 8 : rg.cam_x - 8;
            int y = rg.cam_y + rng_range(&rg.rng, 30, 140);
            rsl_spawn_foe(FO_CLOCKFIRE, x * RSL_FX, y * RSL_FX, 0);
            rg.demon_t = 100;
            rsl_sfx("rsl_whoosh");
        }
    }
    if (rg.pit < 0 && !rg.boss_on) ambient();
    for (int i = 0; i < rg.nspawn; i++) {
        RslSpawn *s = &rg.spawn[i];
        if (s->type != SP_FOE) continue;
        bool pit_spawn = s->ty >= mh;
        if (pit_spawn) {
            if (rg.pit != s->tx / RSL_SW || s->used || s->child >= 0 || s->active) continue;
        } else if (rg.pit >= 0 || rg.boss_on) {
            continue;
        }
        bool in = spawn_in_view(s, 20);
        if (!in) {
            if (!spawn_in_view(s, 120)) s->active = 0;
            continue;
        }
        int kind = s->foe;
        int x = (s->tx * RSL_TILE + 8) * RSL_FX;
        int before = pit_spawn ? first_free_foe() : -1;
        switch (kind) {
        case FO_WISP:
            if (s->t > 0) { s->t--; break; }
            if (!s->active || s->t == 0) {
                flock((rg.cam_x + SCREEN_W + 12) * RSL_FX, (s->ty * RSL_TILE + 8) * RSL_FX, -1);
                s->t = imax(360, 700 - dl * 60);
                s->active = 1;
            }
            break;
        case FO_TUMBLER:
            if (s->t > 0) { s->t--; break; }
            if (rsl_count_foes(FO_TUMBLER) < 2 + dl / 3) {
                int tx = RSL_PX(rg.pl.x) + rg.pl.face * rng_range(&rg.rng, 24, 72);
                int f = rsl_spawn_foe(FO_TUMBLER, tx * RSL_FX, (rg.cam_y - 8) * RSL_FX, 0);
                if (f >= 0) rg.foe[f].spawn = i;
            }
            s->t = imax(110, 260 - dl * 20);
            break;
        case FO_BOULDER:
            if (s->t > 0) { s->t--; break; }
            if (rsl_count_foes(FO_BOULDER) < 1) {
                int f = rsl_spawn_foe(FO_BOULDER, x, ground_y_for(kind, s->tx, s->ty) - 4 * RSL_FX, 0);
                if (f >= 0) rg.foe[f].dir = RSL_PX(rg.pl.x) > s->tx * RSL_TILE ? 1 : -1;
            }
            s->t = imax(240, 420 - dl * 20);
            break;
        case FO_LEAPER: {
            if (s->t > 0) { s->t--; break; }
            /* up out of the water under the logs, one at a time from each spot */
            bool out = false;
            for (int f = 0; f < RSL_MAX_FOES; f++) out |= rg.foe[f].alive && rg.foe[f].kind == FO_LEAPER && rg.foe[f].spawn == i;
            if (out) break;
            int f = rsl_spawn_foe(FO_LEAPER, x, ((s->ty + 2) * RSL_TILE + 8) * RSL_FX, 0);
            if (f >= 0) rg.foe[f].spawn = i;
            s->t = imax(90, 170 - dl * 10) + rng_range(&rg.rng, 0, 60);
            break;
        }
        case FO_TOAD: {
            if (s->t > 0) { s->t--; break; }
            int n = 0;
            for (int f = 0; f < RSL_MAX_FOES; f++) n += rg.foe[f].alive && rg.foe[f].spawn == i;
            if (n < 2) {
                int gy = ground_y_for(kind, s->tx, s->ty);
                int f = rsl_spawn_foe(FO_TOAD, x, gy + 12 * RSL_FX, 0);
                if (f >= 0) { rg.foe[f].spawn = i; rg.foe[f].c = gy; }
            }
            s->t = imax(70, 160 - dl * 15);
            break;
        }
        case FO_GNAT:
            if (s->child < 0 && !s->used && !s->active) {
                int f = rsl_spawn_foe(FO_GNAT, x, (s->ty * RSL_TILE + 8) * RSL_FX, 0);
                if (f >= 0) { rg.foe[f].spawn = i; rg.foe[f].b = 99; s->child = f; }
                s->active = 1;
            }
            break;
        default: {
            /* one of its kind on this spot, until killed */
            if (s->child >= 0 || s->used || s->active) break;
            int y;
            if (kind == FO_HIVE || kind == FO_WEEPER) y = (s->ty * RSL_TILE + FOE_H[kind] / 2) * RSL_FX;
            else if (kind == FO_CROW) y = (s->ty * RSL_TILE + 8) * RSL_FX;
            else y = ground_y_for(kind, s->tx, s->ty);
            int f = rsl_spawn_foe(kind, x, y, 0);
            if (f >= 0) {
                rg.foe[f].spawn = i;
                if (pit_spawn) rg.foe[f].flags |= FF_PIT;
                if (kind == FO_LACKEY) {
                    rg.foe[f].state = 1;
                    if (rng_range(&rg.rng, 1, dl >= 4 ? 3 : 8) == 1) rg.foe[f].flags |= FF_GREEN;
                }
                if (kind == FO_CRAB) rg.foe[f].state = 1;
                s->child = f;
            }
            s->active = 1;
            break;
        }
        }
        if (pit_spawn && before >= 0 && rg.foe[before].alive && !(rg.foe[before].flags & FF_PIT)) {
            /* a pit's holes and drops give out after two */
            rg.foe[before].flags |= FF_PIT;
            if (++s->b >= 2) s->used = 1;
        }
    }
    /* a pit room is done when its foes are */
    if (rg.pit >= 0) {
        const RslHalf *H = &RSL_HALF[rg.half];
        if (H->treasure_pit == rg.pit) return;
        bool any = false, left = false;
        for (int i = 0; i < rg.nspawn; i++) {
            const RslSpawn *s = &rg.spawn[i];
            if (s->type != SP_FOE || s->ty < mh || s->tx / RSL_SW != rg.pit) continue;
            any = true;
            if (!s->used && s->foe != FO_CRAB) left = true; /* a shellcrab only ever lies stunned */
        }
        for (int i = 0; i < RSL_MAX_FOES; i++)
            if (rg.foe[i].alive && (rg.foe[i].flags & FF_PIT) && rg.foe[i].kind != FO_CRAB) left = true;
        if (!any || !left) {
            if (++rg.pit_done_t > 70) rsl_leave_pit();
        }
    }
}
