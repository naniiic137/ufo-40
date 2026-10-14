/* ECHO UNIT - DART's hover-bike: the Cairo ring road (Hog Wild at the end)
 * and the Caldera's lava tunnel (the blue card on the last truck). The road
 * scrolls on its own; the bike moves four ways, B jumps cones, hurdles and
 * mines, and A fires straight ahead. Tap A: the gun stays cool, and fast
 * tapping is the fastest fire. Hold A: it fires slower and the barrel heats;
 * let it boil over and the bike blows up, which costs a whole life. */
#include "eko.h"

#define K (&eg.bk)

const int EKO_OB_HP[OB_KINDS] = {0, 1, 0, 1, 4, 3, 8, 20, 1, 0, 0, 0, 0, 0, 90};
const int EKO_OB_POINTS[OB_KINDS] = {0, 0, 0, 10, 20, 20, 50, 500, 0, 0, 0, 0, 0, 0, 500};

/* half lengths, half depths (on the road) and heights */
static const int8_t OB_HW[OB_KINDS] = {0, 5, 6, 5, 16, 10, 22, 34, 5, 8, 8, 5, 5, 5, 14};
static const int8_t OB_HD[OB_KINDS] = {0, 5, 9, 5, 7, 6, 8, 9, 5, 7, 7, 6, 6, 6, 7};
static const int8_t OB_HT[OB_KINDS] = {0, 8, 12, 18, 16, 18, 26, 30, 6, 99, 12, 8, 8, 8, 18};
static const int8_t OB_DMG[OB_KINDS] = {0, 1, 2, 1, 2, 1, 2, 2, 2, 2, 2, 0, 0, 0, 2};

static int scr_x(const EkoKObj *o) { return EKO_PX(o->x) - K->dist; }

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
            case OB_GTRUCK: o->vx = 512; break;
            case OB_SEMI: o->vx = 608; break;
            case OB_BIKER: o->vx = BK_SCROLL * EKO_FX - 160; break;
            case OB_MINE: o->vx = -256; break;
            case OB_ROCK: o->z = 120 * EKO_FX; break;
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
    int x = scr_x(o), y = EKO_PX(o->y) - EKO_PX(o->z) - 6;
    eko_burst(x, y, o->kind == OB_SEMI || o->kind == OB_HOG ? C_ORANGE : C_YELLOW, o->kind >= OB_GTRUCK ? 16 : 8);
    eko_add_score(EKO_OB_POINTS[o->kind]);
    if (o->kind != OB_CONE) K->kills++;
    eko_sfx(o->kind == OB_HOG ? "eko_bossdie" : o->kind >= OB_GTRUCK ? "eko_boom" : "eko_ko");
    o->alive = 0;
    if (o->kind == OB_GTRUCK) eko_bike_spawn(OB_MANGO, EKO_PX(o->x), EKO_PX(o->y), 0);
    if (o->kind == OB_SEMI && o->a) eko_bike_spawn(OB_CARD, EKO_PX(o->x), EKO_PX(o->y), 0);
    if (o->kind == OB_HOG) { K->boss_dead = 1; eg.shake = 30; }
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
    /* the gun and its heat */
    K->held_t = A ? K->held_t + 1 : 0;
    if (Ap && K->cd <= 0) {
        add_shot(false, K->x + 14 * EKO_FX, K->y - K->z - 8 * EKO_FX, 6 * EKO_FX, 0);
        K->cd = BK_TAP_CD;
        K->shots_fired++;
        eko_sfx("eko_pew");
    } else if (A && K->held_t > 1 && K->cd <= 0) {
        add_shot(false, K->x + 14 * EKO_FX, K->y - K->z - 8 * EKO_FX, 6 * EKO_FX, 0);
        K->cd = BK_HOLD_CD;
        K->shots_fired++;
        eko_sfx("eko_pew");
    }
    if (K->held_t > BK_HEAT_DELAY) K->heat += BK_HEAT_UP;
    else K->heat = imax(0, K->heat - BK_HEAT_DOWN);
    if (K->heat >= BK_HEAT_MAX && !eg.god) {
        /* boiled over: the bike goes up, and a whole life with it */
        K->overheat++;
        K->heat = BK_HEAT_MAX;
        eko_say("OVERHEATED!");
        eko_hurt(99);
        K->dead_t = 1;
        eko_burst(EKO_PX(K->x), EKO_PX(K->y) - 8, C_ORANGE, 20);
        eko_sfx("eko_crash");
    }
}

/* ---- objects -------------------------------------------------------------------------- */

static void objs_update(void) {
    int bx = EKO_PX(K->x), by = EKO_PX(K->y), bz = EKO_PX(K->z);
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
            if (o->z > 0) { o->z -= 3 * EKO_FX; if (o->z <= 0) { o->z = 0; eko_sfx("eko_thud"); eg.shake = 3; } }
            break;
        case OB_FIREJET:
            o->b = o->t % 130; /* 0-59 idle, 60-89 warning, 90-129 fire */
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
        sx = scr_x(o);
        sy = EKO_PX(o->y);
        /* off the back of the screen: gone (the boss loops round) */
        if (sx < -60 && o->kind != OB_HOG) { o->alive = 0; continue; }
        if (sx > SCREEN_W + 400) { o->alive = 0; continue; }
        /* the bike runs into it */
        if (K->dead_t) continue;
        int hw = OB_HW[o->kind], hd = OB_HD[o->kind], ht = OB_HT[o->kind];
        if (iabs(sx - bx) < hw + 10 && iabs(sy - by) < hd + 4) {
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
            if (o->kind == OB_FIREJET) {
                if (o->b >= 90) hurt_bike(2);
                continue;
            }
            if (o->kind == OB_ROCK && o->z > 8 * EKO_FX) continue;
            if (o->kind == OB_HOG && o->state != 3 && iabs(sx - bx) > 16) continue;
            if (bz >= ht) continue; /* jumped clear over it */
            hurt_bike(OB_DMG[o->kind]);
            if (o->kind == OB_CONE || o->kind == OB_MINE || o->kind == OB_RUNNER) { kill_obj(i); }
        }
    }
}

static void shots_update(void) {
    int bx = EKO_PX(K->x), by = EKO_PX(K->y) - EKO_PX(K->z);
    for (int i = 0; i < BK_MAX_SHOTS; i++) {
        EkoKShot *s = &K->shot[i];
        if (!s->alive) continue;
        s->x += s->vx;
        s->y += s->vy;
        int x = EKO_PX(s->x), y = EKO_PX(s->y);
        if (x < -8 || x > SCREEN_W + 8 || y < 40 || y > SCREEN_H) { s->alive = 0; continue; }
        if (s->foe) {
            if (!K->dead_t && iabs(x - bx) < 10 && iabs(y - (by - 6)) < 9) { hurt_bike(1); s->alive = 0; }
            continue;
        }
        for (int k = 0; k < BK_MAX_OBJS; k++) {
            EkoKObj *o = &K->obj[k];
            if (!o->alive || (o->kind >= OB_MANGO && o->kind <= OB_CARD)) continue;
            if (o->kind == OB_FIREJET || (o->kind == OB_ROCK && o->z > 0)) continue;
            int ox = scr_x(o), oy = EKO_PX(o->y);
            int top = oy - OB_HT[o->kind] - 4, bot = oy + OB_HD[o->kind];
            if (o->kind == OB_HURDLE || o->kind == OB_ROCK) top = oy - 14;
            if (iabs(x - ox) < OB_HW[o->kind] + 3 && y >= top && y <= bot) {
                s->alive = 0;
                if (o->hp <= 0) { eko_sfx("eko_tink"); break; } /* hurdles and rocks stop shots */
                o->hp--;
                o->flash = 5;
                eko_sfx("eko_hit");
                if (o->hp <= 0) kill_obj(k);
                break;
            }
        }
    }
}

/* ---- the course ------------------------------------------------------------------------- */

void eko_bike_boss_start(void) {
    if (K->boss_on) return;
    K->boss_on = 1;
    int i = eko_bike_spawn(OB_HOG, K->dist + 360, 130, 0);
    (void)i;
    eko_say("HOG WILD!");
    eko_sfx("eko_rev");
    if (!eko_simulating) music_play(EKO_MUS_BOSS);
}

void eko_bike_start(int course) {
    memset(K, 0, sizeof *K);
    K->course = course;
    K->x = 60 * EKO_FX;
    K->y = 130 * EKO_FX;
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
            eko_bike_spawn(sp->kind, wx, sp->y, sp->arg);
        }
    }
    if (K->dist >= c->length && c->boss && !K->boss_on) eko_bike_boss_start();
    objs_update();
    shots_update();
    if (K->boss_dead || K->card) {
        if (++K->done_t > 150) eko_section_done();
    }
}
