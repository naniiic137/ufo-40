/* ECHO UNIT - DART's hover-bike: the Cairo ring road (Hog Wild at the end)
 * and the Caldera's flight over the lava sea (Major Rattle and his missiles
 * at the end, the blue card when he goes down). On the road the screen
 * scrolls on its own, the bike moves four ways and B jumps cones, hurdles
 * and mines; off the road, in the air, it flies anywhere in the band over
 * the lava and there is no jump. A fires straight ahead. Tap A: the gun
 * stays cool, and fast tapping is the fastest fire. Hold A: it fires a
 * little slower and the barrel heats; held too long, the bike overheats and
 * glows red, and if the fire is still held when the gun comes back, it
 * blows up at once: a whole life gone. */
#include "eko.h"

#define K (&eg.bk)

/* by kind: health, points, half length and half depth (on the road; in the
 * air, half width and half height round the middle), height, damage */
const int EKO_OB_HP[OB_KINDS] = {0, 1, 0, 1, 4, 3, 8, 20, 1, 0, 0, 0, 0, 0, 90, 2, 4, 0, 1, 150};
const int EKO_OB_POINTS[OB_KINDS] = {0, 0, 0, 10, 20, 20, 50, 500, 0, 0, 0, 0, 0, 0, 500, 30, 50, 0, 10, 500};
static const int8_t OB_HW[OB_KINDS] = {0, 5, 6, 5, 16, 10, 22, 34, 5, 8, 7, 5, 5, 5, 14, 9, 9, 10, 6, 22};
static const int8_t OB_HD[OB_KINDS] = {0, 5, 9, 5, 7, 6, 8, 9, 5, 7, 7, 6, 6, 6, 7, 6, 7, 10, 3, 14};
static const int8_t OB_HT[OB_KINDS] = {0, 8, 12, 18, 16, 18, 26, 30, 6, 99, 12, 8, 8, 8, 18, 99, 99, 99, 99, 99};
static const int8_t OB_DMG[OB_KINDS] = {0, 1, 2, 1, 2, 1, 2, 2, 2, 2, 2, 0, 0, 0, 2, 1, 2, 2, 2, 2};

static bool flying(void) { return EKO_KCOURSE[K->course].fly != 0; }

static int scr_x(const EkoKObj *o) { return EKO_PX(o->x) - K->dist; }

/* a rock pillar's span in the air: hanging from the roof down to its tip
 * (a = 1), or rising from the lava up to it (a = 0) */
static void pillar_span(const EkoKObj *o, int *top, int *bot) {
    int tip = EKO_PX(o->y);
    if (o->a) { *top = BK_FLY_TOP - 40; *bot = tip; }
    else { *top = tip; *bot = BK_FLY_BOT + 40; }
}

int eko_bike_spawn(int kind, int wx, int y, int arg) {
    for (int i = 0; i < BK_MAX_OBJS; i++)
        if (!K->obj[i].alive) {
            EkoKObj *o = &K->obj[i];
            memset(o, 0, sizeof *o);
            o->alive = 1;
            o->kind = (uint8_t)kind;
            o->x = wx * EKO_FX;
            o->y = y * EKO_FX;
            o->hp = EKO_OB_HP[kind];
            o->a = arg;
            o->cd = 40 + rng_range(&eg.rng, 0, 40);
            switch (kind) {
            case OB_RUNNER: o->vx = -96; break;
            case OB_CAR: o->vx = arg ? 1152 : 384; break;      /* arg: from behind */
            case OB_GTRUCK: o->vx = flying() ? BK_SCROLL * EKO_FX - 256 : 512; break;
            case OB_SEMI: o->vx = 608; break;
            case OB_BIKER: o->vx = BK_SCROLL * EKO_FX - 160; break;
            case OB_MINE: o->vx = -256; break;
            case OB_ROCK: if (flying()) o->y = (BK_FLY_TOP - 12) * EKO_FX; else o->z = 120 * EKO_FX; break;
            case OB_DRONE: o->vx = BK_SCROLL * EKO_FX - 384; o->z = o->y; break;
            default: break;
            }
            return i;
        }
    return -1;
}

static void add_shot(bool foe, int x, int y, int vx, int vy) {
    for (int i = 0; i < BK_MAX_SHOTS; i++)
        if (!K->shot[i].alive) {
            K->shot[i] = (EkoKShot){1, (uint8_t)foe, x, y, vx, vy};
            return;
        }
}

/* a foe's shot at the bike from (sx, sy) on the screen, at sp px a frame */
static void shoot_at(int sx, int sy, int sp) {
    int a = eko_dir_to(EKO_PX(K->x) - sx, EKO_PX(K->y) - 8 - sy);
    add_shot(true, sx * EKO_FX, sy * EKO_FX, eko_cos(a) * sp / 256, eko_sin(a) * sp / 256);
    eko_sfx("eko_shot");
}

static void hurt_bike(int dmg) {
    if (K->inv > 0 || K->dead_t || eg.god) return;
    eko_hurt(dmg);
    eg.shake = 6;
    if (eko_hp() <= 0) { K->dead_t = 1; eko_sfx("eko_crash"); return; }
    K->inv = EKO_OWL_INV[OWL_DART];
    eko_sfx("eko_ouch");
}

static void kill_obj(int i) {
    EkoKObj *o = &K->obj[i];
    int x = scr_x(o), y = flying() ? EKO_PX(o->y) : EKO_PX(o->y) - EKO_PX(o->z) - 6;
    bool big = o->kind == OB_SEMI || o->kind == OB_HOG || o->kind == OB_RATTLE;
    eko_burst(x, y, big ? C_ORANGE : C_YELLOW, o->kind >= OB_GTRUCK ? 16 : 8);
    eko_add_score(EKO_OB_POINTS[o->kind]);
    if (o->kind != OB_CONE && o->kind != OB_MISSILE) K->kills++;
    eko_sfx(o->kind == OB_HOG || o->kind == OB_RATTLE ? "eko_bossdie" : o->kind >= OB_GTRUCK ? "eko_boom" : "eko_ko");
    o->alive = 0;
    if (o->kind == OB_GTRUCK) eko_bike_spawn(OB_MANGO, EKO_PX(o->x), EKO_PX(o->y), 0);
    if (o->kind == OB_SEMI && o->a) eko_bike_spawn(OB_CARD, EKO_PX(o->x), EKO_PX(o->y), 0);
    if (o->kind == OB_HOG) { K->boss_dead = 1; eg.shake = 30; }
    if (o->kind == OB_RATTLE) {
        /* his jet goes down, and the blue card with it: Dart catches it */
        K->boss_dead = 1;
        K->card = 1;
        eg.shake = 30;
        eko_say("THE BLUE CARD!");
        eko_sfx("eko_key");
        for (int k = 0; k < BK_MAX_OBJS; k++)
            if (K->obj[k].alive && K->obj[k].kind == OB_MISSILE) { eko_burst(scr_x(&K->obj[k]), EKO_PX(K->obj[k].y), C_ORANGE, 6); K->obj[k].alive = 0; }
    }
}

/* ---- the bike --------------------------------------------------------------------- */

static void bike_update(void) {
    if (K->dead_t) {
        K->dead_t++;
        if (K->dead_t > 80) eko_life_lost();
        return;
    }
    if (K->inv) K->inv--;
    if (K->cd) K->cd--;
    bool L = ek_btn(BTN_LEFT), R = ek_btn(BTN_RIGHT), U = ek_btn(BTN_UP), D = ek_btn(BTN_DOWN);
    bool A = ek_btn(EKO_MAIN), Ap = ek_btnp(EKO_MAIN), Bp = ek_btnp(EKO_ALT);
    K->x += ((R ? 1 : 0) - (L ? 1 : 0)) * BK_SPEED;
    K->y += ((D ? 1 : 0) - (U ? 1 : 0)) * BK_SPEED;
    K->x = iclamp(K->x, BK_MINX * EKO_FX, BK_MAXX * EKO_FX);
    if (flying()) {
        /* off the road: anywhere over the lava, and no jump */
        K->y = iclamp(K->y, (BK_FLY_TOP + 16) * EKO_FX, BK_FLY_BOT * EKO_FX);
        K->z = K->vz = 0;
    } else {
        K->y = iclamp(K->y, (BK_ROAD_TOP + 6) * EKO_FX, (BK_ROAD_BOT - 4) * EKO_FX);
        if (Bp && K->z == 0) {
            K->vz = BK_JUMP_VZ;
            K->jumps++;
            eko_sfx("eko_hop");
        }
        if (K->z > 0 || K->vz > 0) {
            K->z += K->vz;
            K->vz -= BK_GRAV_Z;
            if (K->z <= 0) { K->z = 0; K->vz = 0; eko_sfx("eko_land"); }
        }
    }
    /* the gun and its heat */
    K->held_t = A ? K->held_t + 1 : 0;
    if (!A) K->hot_held = 0; /* a break in the fire: no blow-up, whatever the heat */
    bool fired = false;
    if (Ap && K->cd <= 0) {
        add_shot(false, K->x + 14 * EKO_FX, K->y - K->z - 8 * EKO_FX, 6 * EKO_FX, 0);
        K->cd = BK_TAP_CD;
        fired = true;
    } else if (A && K->held_t > 1 && K->cd <= 0) {
        add_shot(false, K->x + 14 * EKO_FX, K->y - K->z - 8 * EKO_FX, 6 * EKO_FX, 0);
        K->cd = BK_HOLD_CD;
        fired = true;
    }
    if (fired) { K->shots_fired++; eko_sfx("eko_pew"); }
    if (fired && K->hot_held && !eg.god) {
        /* still shooting without a stop since it overheated: it blows up at
         * once, and a whole life with it */
        K->overheat++;
        eko_say("OVERHEATED!");
        eko_hurt(99);
        K->dead_t = 1;
        K->hot = K->hot_held = 0;
        eko_burst(EKO_PX(K->x), EKO_PX(K->y) - 8, C_ORANGE, 20);
        eko_sfx("eko_crash");
        return;
    }
    if (K->held_t > BK_HEAT_DELAY) K->heat = imin(BK_HEAT_MAX, K->heat + BK_HEAT_UP);
    else K->heat = imax(0, K->heat - BK_HEAT_DOWN);
    if (A && K->heat >= BK_HEAT_MAX && !K->hot_held) {
        /* overheated (the fire held right up to it): the bike glows red and the gun holds a moment */
        K->hot = 1;
        K->hot_held = 1;
        K->cd = imax(K->cd, BK_GLOW_T);
        eko_say("OVERHEATING!");
        eko_sfx("eko_hot");
    } else if (K->hot && !K->hot_held && K->heat < BK_HEAT_MAX / 2) K->hot = 0;
}

/* ---- Major Rattle, over the lava ----------------------------------------------------------- */

static void missile(int sx, int sy, int homing, int angle) {
    int i = eko_bike_spawn(OB_MISSILE, K->dist + sx, sy, homing);
    if (i < 0) return;
    EkoKObj *m = &K->obj[i];
    m->b = angle;
    m->t = 0;
    eko_sfx("eko_missile");
}

static void rattle_update(EkoKObj *o, int sx, int sy) {
    int by = EKO_PX(K->y) - 8;
    /* states: 0 flying in, 1 on station, 2 ram warning, 3 ramming, 4 coming back round */
    switch (o->state) {
    case 0:
        o->vx = BK_SCROLL * EKO_FX - 384;
        if (sx <= 262) { o->state = 1; o->t = 0; o->cd = 70; }
        break;
    case 1:
        o->vx = BK_SCROLL * EKO_FX;
        o->y += iclamp((by << 8) - o->y, -160, 160);
        if (!o->cd) {
            int r = o->b++ % 4;
            if (r == 0 || r == 2) {
                /* a fan of straight missiles */
                for (int k = -1; k <= 1; k++) missile(sx - 20, sy + k * 8, 0, 32 + k * 3);
                o->cd = 90;
            } else if (r == 1) {
                /* two that home in, from over and under the wings */
                missile(sx - 10, sy - 14, 1, 32);
                missile(sx - 10, sy + 14, 1, 32);
                o->cd = 120;
            } else { o->state = 2; o->t = 0; eko_sfx("eko_rev"); }
        }
        break;
    case 2:
        o->vx = BK_SCROLL * EKO_FX;
        if (o->t > 44) { o->state = 3; o->t = 0; }
        break;
    case 3:
        o->vx = BK_SCROLL * EKO_FX - 6 * EKO_FX;
        if (sx < -50) { o->state = 4; o->t = 0; o->x = (K->dist + 360) * EKO_FX; }
        break;
    case 4:
        o->vx = BK_SCROLL * EKO_FX - 384;
        if (sx <= 262) { o->state = 1; o->cd = 60; }
        break;
    }
    o->y = iclamp(o->y, (BK_FLY_TOP + 16) * EKO_FX, (BK_FLY_BOT - 10) * EKO_FX);
}

/* ---- objects -------------------------------------------------------------------------- */

/* the bike runs into a thing: true when it touches */
static bool touches(const EkoKObj *o, int sx, int sy, int bx, int by, int bz) {
    if (flying()) {
        int cy = by - 8; /* the bike's middle */
        if (o->kind == OB_PILLAR) {
            int top, bot;
            pillar_span(o, &top, &bot);
            return iabs(sx - bx) < OB_HW[o->kind] + 9 && cy + 5 > top && cy - 5 < bot;
        }
        if (o->kind == OB_FIREJET) return o->b >= 90 && iabs(sx - bx) < 9 && cy + 5 > sy;
        return iabs(sx - bx) < OB_HW[o->kind] + 9 && iabs(sy - cy) < OB_HD[o->kind] + 5;
    }
    int hw = OB_HW[o->kind], hd = OB_HD[o->kind];
    if (iabs(sx - bx) >= hw + 10 || iabs(sy - by) >= hd + 4) return false;
    (void)bz;
    return true;
}

static void objs_update(void) {
    int bx = EKO_PX(K->x), by = EKO_PX(K->y), bz = EKO_PX(K->z);
    bool fly = flying();
    for (int i = 0; i < BK_MAX_OBJS; i++) {
        EkoKObj *o = &K->obj[i];
        if (!o->alive) continue;
        o->t++;
        if (o->flash) o->flash--;
        if (o->cd) o->cd--;
        o->x += o->vx;
        int sx = scr_x(o), sy = EKO_PX(o->y);
        switch (o->kind) {
        case OB_RUNNER:
            if (!o->cd && sx < 300 && sx > bx + 30) { add_shot(true, (sx - 6) * EKO_FX, (sy - 12) * EKO_FX, -640, 0); o->cd = 120; eko_sfx("eko_shot"); }
            break;
        case OB_BIKER:
            /* rides along on screen, lines up and shoots back */
            if (sx > 260) o->vx = BK_SCROLL * EKO_FX - 384;
            else if (o->t < 420) o->vx = BK_SCROLL * EKO_FX;
            else o->vx = BK_SCROLL * EKO_FX + 512; /* off ahead */
            o->y += iclamp(K->y - o->y, -160, 160);
            o->y = iclamp(o->y, (BK_ROAD_TOP + 6) * EKO_FX, (BK_ROAD_BOT - 4) * EKO_FX);
            if (!o->cd && sx < 300) { add_shot(true, (sx - 8) * EKO_FX, (sy - 10) * EKO_FX, -768, 0); o->cd = 70; eko_sfx("eko_shot"); }
            break;
        case OB_SEMI:
            /* the card truck stays in front of him until it is stopped */
            if (o->a && sx < 240) o->vx = BK_SCROLL * EKO_FX;
            if (!o->cd && sx < 300 && sx > 40) {
                eko_bike_spawn(OB_MINE, EKO_PX(o->x) - 36, sy, 0);
                o->cd = o->a ? 70 : 90;
            }
            break;
        case OB_ROCK:
            if (fly) {
                /* off the cavern roof, falling as the bike comes near */
                if (o->b || sx < bx + 120) { o->b = 1; o->vy = imin(o->vy + 24, 1024); o->y += o->vy; }
                if (sy > BK_FLY_BOT + 20) o->alive = 0;
            } else if (o->z > 0) { o->z -= 3 * EKO_FX; if (o->z <= 0) { o->z = 0; eko_sfx("eko_thud"); eg.shake = 3; } }
            break;
        case OB_FIREJET:
            o->b = o->t % 130; /* 0-59 idle, 60-89 warning, 90-129 fire */
            break;
        case OB_DRONE:
            /* weaves in from ahead and fires back at the bike */
            o->y = o->z + eko_sin(o->t * 2 + i * 8) * 18;
            if (!o->cd && sx < 300 && sx > bx + 24) { shoot_at(sx - 8, sy, 640); o->cd = 100; }
            break;
        case OB_TURRET:
            /* a gun on a rock: aimed shots as the bike goes by */
            if (!o->cd && sx < 300 && sx > 0) { shoot_at(sx, sy + (o->a ? 6 : -6), 576); o->cd = 85; }
            break;
        case OB_MISSILE: {
            /* straight, or steering after the bike for a while, then straight on */
            int sp = o->a ? 576 : 896;
            if (o->a && o->t < 100 && o->t % 4 == 0) {
                int want = eko_dir_to(bx - sx, by - 8 - sy);
                int d = (want - o->b) & 63;
                if (d) o->b = (o->b + (d < 32 ? 1 : -1)) & 63;
            }
            o->vx = BK_SCROLL * EKO_FX + eko_cos(o->b) * sp / 256;
            o->y += eko_sin(o->b) * sp / 256;
            if (sx < -20 || sy < BK_FLY_TOP - 20 || sy > BK_FLY_BOT + 20) o->alive = 0;
            break;
        }
        case OB_RATTLE:
            rattle_update(o, sx, sy);
            break;
        case OB_HOG: {
            /* states: 0 riding in, 1 lined up, 2 ram warning, 3 ramming, 4 coming back round */
            switch (o->state) {
            case 0:
                o->vx = BK_SCROLL * EKO_FX - 256;
                if (sx <= 250) { o->state = 1; o->t = 0; o->cd = 60; }
                break;
            case 1:
                o->vx = BK_SCROLL * EKO_FX;
                o->y += iclamp(K->y - o->y, -200, 200);
                if (!o->cd) {
                    int r = o->b++ % 4;
                    if (r == 0 || r == 2) {
                        for (int k = -1; k <= 1; k++) add_shot(true, (sx - 10) * EKO_FX, (sy - 12) * EKO_FX, -896, k * 160);
                        eko_sfx("eko_shotgun");
                        o->cd = 70;
                    } else if (r == 1) {
                        eko_bike_spawn(OB_MINE, EKO_PX(o->x) - 16, sy - 12, 0);
                        eko_bike_spawn(OB_MINE, EKO_PX(o->x) - 16, sy + 12, 0);
                        eko_sfx("eko_toss");
                        o->cd = 70;
                    } else { o->state = 2; o->t = 0; eko_sfx("eko_rev"); }
                }
                break;
            case 2:
                o->vx = BK_SCROLL * EKO_FX;
                if (o->t > 40) { o->state = 3; o->t = 0; }
                break;
            case 3:
                o->vx = BK_SCROLL * EKO_FX - 5 * EKO_FX;
                if (sx < -40) { o->state = 4; o->t = 0; o->x = (K->dist + 360) * EKO_FX; }
                break;
            case 4:
                o->vx = BK_SCROLL * EKO_FX - 384;
                if (sx <= 250) { o->state = 1; o->cd = 50; }
                break;
            }
            o->y = iclamp(o->y, (BK_ROAD_TOP + 8) * EKO_FX, (BK_ROAD_BOT - 6) * EKO_FX);
            break;
        }
        default: break;
        }
        if (!o->alive) continue;
        sx = scr_x(o);
        sy = EKO_PX(o->y);
        /* off the back of the screen: gone (the bosses loop round) */
        bool boss = o->kind == OB_HOG || o->kind == OB_RATTLE;
        if (sx < -60 && !boss) { o->alive = 0; continue; }
        if (sx > SCREEN_W + 400) { o->alive = 0; continue; }
        /* the bike runs into it */
        if (K->dead_t || !touches(o, sx, sy, bx, by, bz)) continue;
        if (o->kind == OB_MANGO || o->kind == OB_LIFE || o->kind == OB_CARD) {
            if (o->kind == OB_MANGO) { eko_set_hp(eko_hp() + EKO_MANGO_HP); eko_sfx("eko_mango"); eko_say("MANGO!"); }
            else if (o->kind == OB_LIFE) {
                eg.owl[eg.cur_owl].lives = imin(EKO_MAX_LIVES, eg.owl[eg.cur_owl].lives + 1);
                eko_sfx("eko_1up");
                eko_say("EXTRA LIFE");
            } else { K->card = 1; eko_sfx("eko_key"); eko_say("THE BLUE CARD!"); }
            o->alive = 0;
            continue;
        }
        if (!fly) {
            if (o->kind == OB_FIREJET) {
                if (o->b >= 90) hurt_bike(2);
                continue;
            }
            if (o->kind == OB_ROCK && o->z > 8 * EKO_FX) continue;
            if (o->kind == OB_HOG && o->state != 3 && iabs(sx - bx) > 16) continue;
            if (bz >= OB_HT[o->kind]) continue; /* jumped clear over it */
        }
        if (o->kind == OB_RATTLE && o->state != 3 && iabs(sx - bx) > 20) continue;
        hurt_bike(OB_DMG[o->kind]);
        if (o->kind == OB_CONE || o->kind == OB_MINE || o->kind == OB_RUNNER) kill_obj(i);
        else if (o->kind == OB_MISSILE) { eko_burst(sx, sy, C_ORANGE, 8); eko_sfx("eko_boom"); o->alive = 0; }
    }
}

static void shots_update(void) {
    int bx = EKO_PX(K->x), by = EKO_PX(K->y) - EKO_PX(K->z);
    bool fly = flying();
    for (int i = 0; i < BK_MAX_SHOTS; i++) {
        EkoKShot *s = &K->shot[i];
        if (!s->alive) continue;
        s->x += s->vx;
        s->y += s->vy;
        int x = EKO_PX(s->x), y = EKO_PX(s->y);
        if (x < -8 || x > SCREEN_W + 8 || y < 24 || y > SCREEN_H) { s->alive = 0; continue; }
        if (s->foe) {
            if (!K->dead_t && iabs(x - bx) < 10 && iabs(y - (by - 6)) < 9) { hurt_bike(1); s->alive = 0; }
            continue;
        }
        for (int k = 0; k < BK_MAX_OBJS; k++) {
            EkoKObj *o = &K->obj[k];
            if (!o->alive || (o->kind >= OB_MANGO && o->kind <= OB_CARD)) continue;
            if (o->kind == OB_FIREJET || (o->kind == OB_ROCK && o->z > 0)) continue;
            int ox = scr_x(o), oy = EKO_PX(o->y);
            bool hit;
            if (fly) {
                if (o->kind == OB_PILLAR) {
                    int top, bot;
                    pillar_span(o, &top, &bot);
                    hit = iabs(x - ox) < OB_HW[o->kind] + 3 && y >= top && y <= bot;
                } else hit = iabs(x - ox) < OB_HW[o->kind] + 3 && iabs(y - oy) < OB_HD[o->kind] + 3;
            } else {
                int top = oy - OB_HT[o->kind] - 4, bot = oy + OB_HD[o->kind];
                if (o->kind == OB_HURDLE || o->kind == OB_ROCK) top = oy - 14;
                hit = iabs(x - ox) < OB_HW[o->kind] + 3 && y >= top && y <= bot;
            }
            if (!hit) continue;
            s->alive = 0;
            if (o->hp <= 0) { eko_sfx("eko_tink"); break; } /* hurdles, rocks and pillars stop shots */
            o->hp--;
            o->flash = 5;
            eko_sfx("eko_hit");
            if (o->hp <= 0) kill_obj(k);
            break;
        }
    }
}

/* ---- the course ------------------------------------------------------------------------- */

void eko_bike_boss_start(void) {
    if (K->boss_on) return;
    K->boss_on = 1;
    if (EKO_KCOURSE[K->course].boss == OB_RATTLE) {
        eko_bike_spawn(OB_RATTLE, K->dist + 360, 100, 0);
        eko_say("MAJOR RATTLE!");
    } else {
        eko_bike_spawn(OB_HOG, K->dist + 360, 130, 0);
        eko_say("HOG WILD!");
    }
    eko_sfx("eko_rev");
    if (!eko_simulating) music_play(EKO_MUS_BOSS);
}

void eko_bike_start(int course) {
    memset(K, 0, sizeof *K);
    K->course = course;
    K->x = 60 * EKO_FX;
    K->y = (flying() ? 104 : 130) * EKO_FX;
}

void eko_bike_update(void) {
    const EkoKCourse *c = &EKO_KCOURSE[K->course];
    bike_update();
    if (K->dead_t > 80) return;
    if (!K->dead_t) K->dist += BK_SCROLL;
    while (K->spawn_i < c->n && c->spawns[K->spawn_i].at <= K->dist) {
        const EkoKSpawn *sp = &c->spawns[K->spawn_i++];
        for (int k = 0; k < imax(1, sp->n); k++) {
            int wx = K->dist + 330 + k * sp->gap;
            if (sp->kind == OB_CAR && sp->arg) wx = K->dist - 40 - k * sp->gap;
            int y = sp->y;
            if (c->fly && sp->kind == OB_DRONE) y += (k % 2 ? 1 : -1) * (k ? 14 : 0);
            eko_bike_spawn(sp->kind, wx, y, sp->arg);
        }
    }
    if (K->dist >= c->length && c->boss && !K->boss_on) eko_bike_boss_start();
    objs_update();
    shots_update();
    if (K->boss_dead || K->card) {
        if (++K->done_t > 150) eko_section_done();
    }
}
