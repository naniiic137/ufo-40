/* RESTLESS - the six fights at the end of each half: the Bridgekeeper (blue
 * and tougher from four deaths), Old Rattle, Three-Tongue, Gust and Gash,
 * the Rammer and the Hollow King. The screen locks while they live. */
#include "rsl.h"

#define floor_y rg.boss_floor_y /* the arena floor, pixels */

static int ax0(void) { return rg.lock_x; }
static int dxp(const RslFoe *f) { return RSL_PX(rg.pl.x) - RSL_PX(f->x); }

/* the arena columns Old Rattle climbs, and his three heights */
static int rattle_col(int c) { return ax0() + 40 + c * 120; }
static int rattle_level(int l) { return floor_y - l * 48; }

int rsl_boss_hp_total(void) {
    int n = 0;
    for (int i = 0; i < RSL_MAX_FOES; i++)
        if (rg.foe[i].alive && RSL_IS_BOSS(rg.foe[i].kind) && rg.foe[i].kind != FO_SKULL) n += rg.foe[i].hp;
    return n;
}

void rsl_boss_start(void) {
    const RslSpawn *s = &rg.spawn[rg.boss_spawn];
    int bx = s->tx * RSL_TILE + 8;
    int g = rsl_ground_below(bx, s->ty * RSL_TILE);
    floor_y = g > 0 ? g : (s->ty + 1) * RSL_TILE;
    rg.boss_on = true;
    int f;
    switch (rg.boss_kind) {
    case BOSS_KEEPER:
        f = rsl_spawn_foe(FO_KEEPER, bx * RSL_FX, (floor_y - 17) * RSL_FX, 0);
        if (f >= 0 && rg.deaths >= 4) {
            /* the blue one: more to it, and bombs */
            rg.foe[f].flags |= FF_BLUE;
            rg.foe[f].hp = rg.foe[f].maxhp = 120;
        }
        if (f >= 0) rg.foe[f].state = 0;
        rsl_music(RSL_MUS_MIDBOSS);
        break;
    case BOSS_RATTLE:
        f = rsl_spawn_foe(FO_RATTLE, rattle_col(2) * RSL_FX, (rattle_level(2) - 14) * RSL_FX, 0);
        if (f >= 0) { rg.foe[f].a = 2; rg.foe[f].b = 2; rg.foe[f].c = 0; }
        rsl_music(RSL_MUS_BOSS);
        break;
    case BOSS_TONGUE:
        for (int k = 0; k < 3; k++) {
            f = rsl_spawn_foe(FO_HEAD, (ax0() + 90 + k * 70) * RSL_FX, (floor_y - 78) * RSL_FX, k);
            if (f >= 0) { rg.foe[f].a = (ax0() + 90 + k * 70) * RSL_FX; rg.foe[f].b = (floor_y - 78) * RSL_FX; rg.foe[f].t = k * 50; }
        }
        rsl_music(RSL_MUS_MIDBOSS);
        break;
    case BOSS_PAIR: {
        /* he walks in between them */
        int px = RSL_PX(rg.pl.x);
        f = rsl_spawn_foe(FO_GUST, (px - 90) * RSL_FX, (floor_y - 90) * RSL_FX, 0);
        f = rsl_spawn_foe(FO_GASH, (px + 90) * RSL_FX, (floor_y - 15) * RSL_FX, 0);
        if (f >= 0) rg.foe[f].dir = -1;
        rsl_music(RSL_MUS_BOSS);
        break;
    }
    case BOSS_RAMMER:
        f = rsl_spawn_foe(FO_RAMMER, (ax0() + 270) * RSL_FX, (floor_y - 12) * RSL_FX, 0);
        rsl_music(RSL_MUS_MIDBOSS);
        break;
    case BOSS_KING:
        f = rsl_spawn_foe(FO_KING, (ax0() + 262) * RSL_FX, (floor_y - 62) * RSL_FX, 0);
        rsl_music(RSL_MUS_BOSS);
        break;
    default: break;
    }
    rsl_sfx("rsl_bossin");
}

/* ------------------------------------------------------------------------ */

static void keeper(RslFoe *f) {
    /* jumps back and forth across the bridge by set distances, crouching
     * first for as long as the jump will be */
    static const int DIST[5] = {64, 112, 40, 144, 88};
    bool blue = f->flags & FF_BLUE;
    int fy = floor_y - f->h / 2;
    switch (f->state) {
    case 0: /* standing */
        f->dir = dxp(f) > 0 ? 1 : -1;
        if (f->t > 50) {
            f->state = 1;
            f->st = 0;
            int d = DIST[f->a % 5];
            int left = RSL_PX(f->x) - (ax0() + 24), right = ax0() + 296 - RSL_PX(f->x);
            int dir = f->c ? -1 : 1;
            if (dir > 0 && right < d) dir = -1;
            if (dir < 0 && left < d) dir = 1;
            f->c = dir < 0;
            f->b = dir * d;
            f->a++;
        }
        break;
    case 1: /* the crouch tells you how far */
        if (++f->st > 18 + iabs(f->b) / 6) {
            f->state = 2;
            f->vy = -1100;
            f->vx = f->b * RSL_FX / 52;
            f->st = 0;
            rsl_sfx("rsl_jump");
        }
        break;
    case 2: /* in the air */
        f->st++;
        f->x += f->vx;
        f->vy += 42;
        f->y += f->vy;
        if (blue && (f->st == 14 || f->st == 30)) {
            int e = rsl_add_eshot(ES_BOMB, f->x, f->y + 8 * RSL_FX, f->vx / 3, 0);
            if (e >= 0) rg.es[e].grav = 30;
        }
        if (f->y >= fy * RSL_FX && f->vy > 0) {
            f->y = fy * RSL_FX;
            f->state = 0;
            f->t = 0;
            rg.shake = 8;
            rsl_sfx("rsl_thud");
        }
        break;
    }
    f->x = iclamp(f->x, (ax0() + 20) * RSL_FX, (ax0() + 300) * RSL_FX);
}

static void rattle(RslFoe *f) {
    /* runs along a ledge to a column, climbs or drops one or two ledges,
     * and drops bombs as he goes */
    int lvl_y = rattle_level(f->b) - f->h / 2;
    switch (f->state) {
    case 0: { /* pick the next column */
        int c = f->a;
        int nc;
        do nc = rng_range(&rg.rng, 0, 2); while (nc == c);
        f->a = nc;
        f->state = 1;
        f->dir = rattle_col(nc) > RSL_PX(f->x) ? 1 : -1;
        break;
    }
    case 1: { /* run */
        int tx = rattle_col(f->a) * RSL_FX;
        int sp = 400;
        if (iabs(tx - f->x) <= sp) {
            f->x = tx;
            f->state = 2;
            /* one or two ledges up or down */
            int nl;
            do nl = iclamp(f->b + rng_range(&rg.rng, -2, 2), 0, 2); while (nl == f->b);
            f->c = nl;
        } else {
            f->x += f->dir * sp;
        }
        f->y = lvl_y * RSL_FX;
        break;
    }
    case 2: { /* up or down the column */
        int ty = (rattle_level(f->c) - f->h / 2) * RSL_FX;
        int sp = 512;
        if (iabs(ty - f->y) <= sp) {
            f->y = ty;
            f->b = f->c;
            f->state = 0;
        } else {
            f->y += ty > f->y ? sp : -sp;
        }
        break;
    }
    }
    if (f->t % 70 == 35 && f->b > 0) {
        int e = rsl_add_eshot(ES_BOMB, f->x, f->y + 10 * RSL_FX, 0, 0);
        if (e >= 0) rg.es[e].grav = 28;
        rsl_sfx("rsl_throw");
    }
}

static void head(RslFoe *f) {
    /* bobs on its neck; spits fire down that leaves the floor burning */
    f->x = f->a + rsl_sin(f->t * 2) * 10 * RSL_FX / 256;
    f->y = f->b + rsl_cos(f->t * 3) * 8 * RSL_FX / 256;
    f->dir = dxp(f) > 0 ? 1 : -1;
    if (f->t % 150 == 90) { /* the three take turns (their clocks start 50 apart) */
        int e = rsl_add_eshot(ES_FIRE, f->x, f->y + 8 * RSL_FX, iclamp(dxp(f) * 6, -400, 400), 300);
        if (e >= 0) { rg.es[e].grav = 14; rg.es[e].owner = -2; }
        rsl_sfx("rsl_fireball");
    }
    /* lackeys join from two deaths: the first head alive calls them */
    if (rg.deaths >= 2 && f->t % 240 == 120) {
        int first = -1;
        for (int i = 0; i < RSL_MAX_FOES; i++)
            if (rg.foe[i].alive && rg.foe[i].kind == FO_HEAD) { first = i; break; }
        if (first == (int)(f - rg.foe) && rsl_count_foes(FO_LACKEY) < 2) {
            int side = rng_range(&rg.rng, 0, 1);
            int x = ax0() + (side ? 300 : 20);
            int l = rsl_spawn_foe(FO_LACKEY, x * RSL_FX, (floor_y - 10) * RSL_FX, 0);
            if (l >= 0) { rg.foe[l].flags |= FF_HARMLESS; rg.foe[l].flags |= FF_NODROP; }
        }
    }
}

static void gust(RslFoe *f) {
    int hover_y = (floor_y - 92) * RSL_FX;
    switch (f->state) {
    case 0: /* hover and drift */
        f->x += f->dir * 200;
        if (RSL_PX(f->x) < ax0() + 30) f->dir = 1;
        if (RSL_PX(f->x) > ax0() + 290) f->dir = -1;
        if (f->y > hover_y) f->y -= 256;
        if (f->t > 150) {
            f->state = 1;
            f->st = 0;
            f->dir = dxp(f) > 0 ? 1 : -1;
            rsl_sfx("rsl_screech");
        }
        break;
    case 1: { /* the swoop: down to head height, straight across */
        int sy = (floor_y - 27) * RSL_FX;
        f->st++;
        if (f->y < sy) f->y = imin(sy, f->y + 640);
        f->x += f->dir * 700;
        if (RSL_PX(f->x) < ax0() + 16 || RSL_PX(f->x) > ax0() + 304) {
            f->x = iclamp(f->x, (ax0() + 16) * RSL_FX, (ax0() + 304) * RSL_FX);
            f->state = 2;
            f->st = 0;
        }
        break;
    }
    case 2: /* drops to the ground after a swoop */
        f->st++;
        if (f->y < (floor_y - 10) * RSL_FX) f->y += 300;
        if (f->st > 80) { f->state = 0; f->t = 0; f->dir = -f->dir; }
        break;
    }
}

static void gash(RslFoe *f) {
    int fy = (floor_y - f->h / 2) * RSL_FX;
    int dx = dxp(f);
    switch (f->state) {
    case 0: /* stalks */
        f->dir = dx > 0 ? 1 : -1;
        f->x += f->dir * 180;
        if (iabs(dx) < 34 && f->b <= 0) { f->state = 2; f->st = 0; }
        else if (iabs(dx) < 90 && iabs(dx) > 50 && f->b <= 0 && f->t % 60 == 0) {
            f->state = 1;
            f->vy = -900;
            f->vx = f->dir * 400;
            rsl_sfx("rsl_jump");
        }
        if (f->b > 0) f->b--;
        break;
    case 1: /* a leap */
        f->x += f->vx;
        f->vy += 40;
        f->y += f->vy;
        if (f->y >= fy) { f->y = fy; f->state = 0; f->b = 40; }
        break;
    case 2: /* the thrust */
        f->st++;
        if (f->st == 16) {
            int e = rsl_add_eshot(ES_BLADE, f->x + f->dir * 16 * RSL_FX, f->y, 0, 0);
            if (e >= 0) { rg.es[e].life = 14; rg.es[e].owner = (int)(f - rg.foe); rg.es[e].w = 22; }
            rsl_sfx("rsl_swish");
        }
        if (f->st > 40) { f->state = 0; f->b = 50; }
        break;
    }
    if (f->state != 1) f->y = fy;
    f->x = iclamp(f->x, (ax0() + 14) * RSL_FX, (ax0() + 306) * RSL_FX);
}

static void rammer(RslFoe *f) {
    int lo = (ax0() + 18) * RSL_FX, hi = (ax0() + 302) * RSL_FX;
    switch (f->state) {
    case 0: /* paws the ground */
        f->dir = dxp(f) > 0 ? 1 : -1;
        if (f->t > 70) { f->state = 1; f->c = 0; f->st = 0; rsl_sfx("rsl_roar"); }
        break;
    case 1: /* the dash, at where he stood */
        f->x += f->dir * 900;
        if (f->c >= 24) {
            /* enough hurt in one dash: knocked back */
            f->state = 2;
            f->dir = -f->dir;
            f->st = 0;
            rsl_sfx("rsl_thud");
        } else if (f->x <= lo || f->x >= hi) {
            f->x = iclamp(f->x, lo, hi);
            f->state = 2;
            f->dir = -f->dir;
            f->st = 0;
            rg.shake = 10;
            rsl_sfx("rsl_thud");
        }
        break;
    case 2: /* bounces back toward the middle, then resets */
        f->st++;
        f->x += f->dir * 300;
        f->x = iclamp(f->x, lo, hi);
        if (f->st > 40) { f->state = 0; f->t = 0; }
        break;
    }
}

static void king(RslFoe *f) {
    /* the face: minions from the mouth, the third eye's volleys, the skull */
    int c = f->t % 320;
    f->a = c >= 200 && c < 300; /* third eye open */
    if (f->a && (c == 220 || c == 250 || c == 280)) {
        int ex = f->x, ey = f->y - 30 * RSL_FX;
        int a = rsl_dir_to(rg.pl.x - ex, (RSL_PX(rg.pl.y) - 18) * RSL_FX - ey);
        for (int k = -1; k <= 1; k++)
            rsl_add_eshot(ES_BALL, ex, ey, rsl_cos(a + k * 2) * 520 / 256, rsl_sin(a + k * 2) * 520 / 256);
        rsl_sfx("rsl_spit");
    }
    if (f->t % 170 == 60 && rsl_count_foes(FO_GNAT) + rsl_count_foes(FO_LACKEY) < 4) {
        int g = rsl_spawn_foe(rg.deaths >= 3 && (f->t / 170) % 2 ? FO_LACKEY : FO_GNAT, f->x - 16 * RSL_FX,
                              f->y + 22 * RSL_FX, 0);
        if (g >= 0) {
            rg.foe[g].flags |= FF_NODROP;
            if (rg.foe[g].kind == FO_LACKEY) { rg.foe[g].state = 1; rg.foe[g].y = (floor_y - 10) * RSL_FX; }
        }
        rsl_sfx("rsl_belch");
    }
    if (f->t % 420 == 200 && rsl_count_foes(FO_SKULL) == 0) {
        int s = rsl_spawn_foe(FO_SKULL, (ax0() - 8) * RSL_FX, (rg.cam_y + 34) * RSL_FX, 0);
        if (s >= 0) { rg.foe[s].dir = 1; rg.foe[s].flags |= FF_NODROP; }
    }
    /* the face itself kills */
    int px = RSL_PX(rg.pl.x), py = RSL_PX(rg.pl.y);
    if (px + 4 > RSL_PX(f->x) - 44 && py - 20 < RSL_PX(f->y) + 44) rsl_player_hurt(FO_KING);
}

static void skull(RslFoe *f) {
    f->x += f->dir * 220;
    f->y += rsl_sin(f->t * 3) * 60 / 256;
    if (f->t % 36 == 18) {
        int e = rsl_add_eshot(ES_TEAR, f->x, f->y + 8 * RSL_FX, 0, 64);
        if (e >= 0) rg.es[e].grav = 22;
    }
    if (RSL_PX(f->x) > ax0() + SCREEN_W + 10) f->alive = 0;
}

void rsl_boss_update(int i) {
    RslFoe *f = &rg.foe[i];
    switch (f->kind) {
    case FO_KEEPER: keeper(f); break;
    case FO_RATTLE: rattle(f); break;
    case FO_HEAD: head(f); break;
    case FO_GUST: gust(f); break;
    case FO_GASH: gash(f); break;
    case FO_RAMMER: rammer(f); break;
    case FO_KING: king(f); break;
    case FO_SKULL: skull(f); break;
    default: break;
    }
}

/* a boss down: treasure; when the last of them goes, the half is won */
void rsl_boss_dead(int i) {
    RslFoe *f = &rg.foe[i];
    rg.shake = 20;
    switch (f->kind) {
    case FO_KEEPER:
        if (f->flags & FF_BLUE) {
            /* twelve gold beetles, scattered to the right */
            for (int k = 0; k < 12; k++) {
                int it = rsl_drop_item(IT_BEETLE, f->x, f->y, 0);
                if (it >= 0) { rg.item[it].vx = 120 + k * 60; rg.item[it].vy = -900 - (k % 3) * 150; }
            }
        } else {
            for (int k = 0; k < 3; k++) rsl_drop_item(IT_JAR, f->x, f->y, 0);
        }
        break;
    case FO_HEAD: rsl_drop_item(IT_JAR, f->x, f->y, 0); break;
    case FO_SKULL: return;
    default:
        rsl_drop_item(IT_IDOL, f->x, f->y, 0);
        rsl_drop_item(IT_JAR, f->x, f->y, 0);
        rsl_drop_item(IT_JAR, f->x, f->y, 0);
        break;
    }
    bool any = false;
    for (int k = 0; k < RSL_MAX_FOES; k++)
        if (rg.foe[k].alive && RSL_IS_BOSS(rg.foe[k].kind) && rg.foe[k].kind != FO_SKULL) any = true;
    if (!any) {
        rg.boss_dead = true;
        rg.boss_done_t = 0;
        /* what's left of the fight goes quiet */
        for (int k = 0; k < RSL_MAX_FOES; k++)
            if (rg.foe[k].alive) { rsl_burst(RSL_PX(rg.foe[k].x), RSL_PX(rg.foe[k].y), C_GREY, 6, 100); rg.foe[k].alive = 0; }
        for (int k = 0; k < RSL_MAX_ESHOTS; k++) rg.es[k].alive = 0;
        rsl_music_stop();
        rsl_sfx("rsl_victory");
    }
}
