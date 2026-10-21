/* ANTIPODE - the station's machines: treadles (grey, red, gold), spitters,
 * star-eyes, hiveboxes and their mites, skipjacks, skimmers and sentry orbs;
 * their shots; and harm done to them. The bosses are in apd_boss.c. */
#include "apd.h"

const char *const APD_FOE_NAME[FO_KINDS] = {
    "", "TREADLE", "RED TREADLE", "GOLD TREADLE", "SPITTER", "STAR-EYE", "HIVEBOX", "MITE", "SKIPJACK",
    "SKIMMER", "SENTRY ORB", "SCRAPPER", "WARDEN", "BRACKISH", "LAMPREY", "LAMPREY", "COILHEART", "NODE",
    "OBVERSE", "HALLORAN", "HALLORAN", "JOINT",
};

static const struct { int w, h, hp, dmg; } FOE[FO_KINDS] = {
    [FO_TREAD] = {14, 12, 15, 8},   [FO_TREAD_R] = {14, 12, 40, 10}, [FO_TREAD_G] = {14, 12, 80, 12},
    [FO_SPITTER] = {14, 10, 25, 6}, [FO_STAREYE] = {20, 20, 60, 10}, [FO_HIVE] = {18, 14, 80, 6},
    [FO_MITE] = {8, 8, 5, 5},       [FO_SKIP] = {12, 12, 25, 8},     [FO_SKIMMER] = {14, 8, 15, 6},
    [FO_SENTRY] = {12, 12, 30, 8},
    [FO_SCRAPPER] = {36, 24, 300, 12}, [FO_WARDEN] = {40, 48, 600, 15}, [FO_BRACKISH] = {56, 32, 700, 15},
    [FO_LAMPREY] = {16, 16, 400, 12},  [FO_LAMP_SEG] = {14, 14, 999, 10}, [FO_COILHEART] = {32, 32, 700, 15},
    [FO_NODE] = {14, 14, 60, 10},      [FO_OBVERSE] = {12, 22, 800, 12},  [FO_HALLORAN] = {48, 40, 700, 15},
    [FO_HAL_LEG] = {12, 24, 999, 12},  [FO_HAL_JOINT] = {14, 14, 40, 10},
};

void apd_foe_size(int kind, int *w, int *h);
void apd_foe_size(int kind, int *w, int *h) {
    *w = FOE[kind].w;
    *h = FOE[kind].h;
}
int apd_foe_dmg(int kind);
int apd_foe_dmg(int kind) { return FOE[kind].dmg; }
int apd_foe_hp(int kind);
int apd_foe_hp(int kind) { return FOE[kind].hp; }

int apd_spawn_foe(int kind, int x, int y, bool up) {
    for (int i = 0; i < APD_MAX_FOES; i++)
        if (!ag.foe[i].alive) {
            ApdFoe *f = &ag.foe[i];
            memset(f, 0, sizeof *f);
            f->kind = (uint8_t)kind;
            f->alive = 1;
            f->up = up;
            f->x = x;
            f->y = y;
            f->w = (int16_t)FOE[kind].w;
            f->h = (int16_t)FOE[kind].h;
            f->hp = f->maxhp = (int16_t)FOE[kind].hp;
            f->dir = (i & 1) ? 1 : -1;
            f->owner = -1;
            f->a = x;
            f->b = y;
            f->t = (i * 37) % 90;
            return i;
        }
    return -1;
}

int apd_add_eshot(int kind, int x, int y, int vx, int vy, int dmg) {
    for (int i = 0; i < APD_MAX_ESHOTS; i++)
        if (!ag.es[i].alive) {
            ApdEShot *e = &ag.es[i];
            memset(e, 0, sizeof *e);
            e->kind = (uint8_t)kind;
            e->alive = 1;
            e->x = x;
            e->y = y;
            e->vx = vx;
            e->vy = vy;
            e->dmg = (int16_t)dmg;
            e->w = e->h = 6;
            e->life = 400;
            if (kind == ES_BEAM) { e->w = 10; e->h = 18; }
            if (kind == ES_RING) { e->w = e->h = 14; }
            if (kind == ES_LIGHTNING) { e->w = 16; e->h = 640; }
            return i;
        }
    return -1;
}

/* a whole-number aim at the suit, speed in 1/256 px a frame */
void apd_aim(int x, int y, int speed, int *vx, int *vy);
void apd_aim(int x, int y, int speed, int *vx, int *vy) {
    int dx = APD_PX(ag.pl.x) - APD_PX(x), dy = APD_PX(ag.pl.y) - APD_PX(y);
    int d = imax(1, (int)sqrt((double)(dx * dx + dy * dy)));
    *vx = dx * speed / d;
    *vy = dy * speed / d;
}

int apd_count_foes(int kind) {
    int n = 0;
    for (int i = 0; i < APD_MAX_FOES; i++) n += ag.foe[i].alive && ag.foe[i].kind == kind;
    return n;
}

int apd_foes_alive(void) {
    int n = 0;
    for (int i = 0; i < APD_MAX_FOES; i++) n += ag.foe[i].alive;
    return n;
}

static bool solid_px(int x, int y) {
    int t = apd_tile_at(x, y);
    return t == T_WALL || t == T_SPUP || t == T_SPDN || t == T_CRACK || t == T_BARREL || t == T_DOOR ||
           t == T_LOCK || t == T_SHUT || t == T_FAKE || t == T_GREY || t == T_PLAT;
}

void apd_boss_dead_check(int i);

static void kill_foe(int i) {
    ApdFoe *f = &ag.foe[i];
    f->alive = 0;
    apd_burst(APD_PX(f->x), APD_PX(f->y), C_ORANGE, APD_IS_BOSS(f->kind) ? 20 : 8, 2);
    apd_sfx(APD_IS_BOSS(f->kind) ? "apd_bigboom" : "apd_pop");
    if (f->kind == FO_SENTRY && apd_count_foes(FO_SENTRY) == 0) {
        apd_open_nano();
        apd_sfx("apd_unlock");
        apd_say("THE NANO-WALL DISSOLVES");
    }
    if (ag.drone.latch == i) ag.drone.latch = -1;
    apd_boss_dead_check(i);
}

void apd_hurt_foe(int i, int dmg, bool from_player) {
    if (i < 0 || i >= APD_MAX_FOES) return;
    ApdFoe *f = &ag.foe[i];
    if (!f->alive || dmg <= 0) return;
    (void)from_player;
    if (f->flags & FF_SHIELD) { apd_sfx("apd_tink"); return; }
    if (f->owner >= 0) {
        /* a part: the harm goes to the body (legs always, joints are their own) */
        ApdFoe *o = &ag.foe[f->owner];
        f->flash = 4;
        if (f->kind == FO_HAL_JOINT || f->kind == FO_NODE) {
            f->hp = (int16_t)(f->hp - dmg);
            if (f->hp <= 0) kill_foe(i);
            else apd_sfx("apd_hit");
            return;
        }
        if (o->alive) {
            apd_boss_hurt(f->owner, dmg);
            apd_sfx("apd_hit");
        }
        return;
    }
    if (APD_IS_BOSS(f->kind)) {
        apd_boss_hurt(i, dmg);
        return;
    }
    f->hp = (int16_t)(f->hp - dmg);
    f->flash = 4;
    if (f->hp <= 0) kill_foe(i);
    else apd_sfx("apd_hit");
}

void apd_kill_foe(int i);
void apd_kill_foe(int i) { if (i >= 0 && i < APD_MAX_FOES && ag.foe[i].alive) kill_foe(i); }

/* ---- the plain foes ------------------------------------------------------------ */

static void walker(ApdFoe *f, int speed) {
    int g = f->up ? -1 : 1;
    int px = APD_PX(f->x), py = APD_PX(f->y);
    int ahead = px + f->dir * (f->w / 2 + 1);
    int feet = py + g * (f->h / 2 + 2);
    bool wall = solid_px(ahead, py);
    bool floor = solid_px(ahead, feet);
    int t = apd_tile_at(ahead, feet);
    if (wall || !floor || t == T_SPUP || t == T_SPDN || ahead < 4 || ahead > ag.rw * APD_TILE - 4) f->dir = -f->dir;
    else f->x += f->dir * speed;
}

static void foe_update(int i) {
    ApdFoe *f = &ag.foe[i];
    ApdPlayer *p = &ag.pl;
    f->t++;
    if (f->flash > 0) f->flash--;
    int vx, vy;
    switch (f->kind) {
    case FO_TREAD: walker(f, 128); break;
    case FO_TREAD_R: walker(f, 205); break;
    case FO_TREAD_G:
        walker(f, 256);
        if (f->t % 120 == 0) {
            apd_aim(f->x, f->y, 300, &vx, &vy);
            apd_add_eshot(ES_BALL, f->x, f->y, vx, vy, 8);
        }
        break;
    case FO_SPITTER:
        if (f->t % 100 == 50) {
            apd_aim(f->x, f->y, 256, &vx, &vy);
            apd_add_eshot(ES_BALL, f->x, f->y + (f->up ? 3 : -3) * APD_FX, vx, vy, 6);
            apd_sfx("apd_spit");
        }
        break;
    case FO_STAREYE:
        f->y = f->b + (f->t % 64 < 32 ? f->t % 32 : 32 - f->t % 32) * 16;
        if (f->t % 140 == 70) {
            static const int8_t DX[8] = {1, 1, 0, -1, -1, -1, 0, 1}, DY[8] = {0, 1, 1, 1, 0, -1, -1, -1};
            for (int k = 0; k < 8; k++) {
                int s = (DX[k] && DY[k]) ? 217 : 307;
                apd_add_eshot(ES_STAR, f->x, f->y, DX[k] * s, DY[k] * s, 8);
            }
            apd_sfx("apd_star");
        }
        break;
    case FO_HIVE:
        if (f->t % 150 == 0 && apd_count_foes(FO_MITE) < 3) {
            int m = apd_spawn_foe(FO_MITE, f->x, f->y + (f->up ? 10 : -10) * APD_FX, false);
            if (m >= 0) apd_sfx("apd_spit");
        }
        break;
    case FO_MITE:
        apd_aim(f->x, f->y, 150, &vx, &vy);
        f->x += vx + (f->t % 40 < 20 ? 40 : -40);
        f->y += vy;
        break;
    case FO_SKIP: {
        int g = f->up ? -1 : 1;
        if (f->state == 0) {
            if (f->t > 90) {
                f->state = 1;
                f->vy = -g * 1024;
                f->dir = APD_PX(p->x) < APD_PX(f->x) ? -1 : 1;
                f->t = 0;
            }
        } else {
            /* it flips its gravity at the top of its leap and lands on the other side */
            if (f->t == 10) f->up = !f->up;
            int g2 = f->up ? -1 : 1;
            f->vy += g2 * 64;
            if (f->vy * g2 > 1024) f->vy = g2 * 1024;
            int nx = f->x + f->dir * 192;
            if (!solid_px(APD_PX(nx) + f->dir * f->w / 2, APD_PX(f->y))) f->x = nx;
            int ny = f->y + f->vy;
            int edge = APD_PX(ny) + g2 * f->h / 2;
            if (solid_px(APD_PX(f->x), edge)) {
                f->vy = 0;
                f->state = 0;
                f->t = 0;
            } else f->y = ny;
            if (APD_PX(f->y) < 0 || APD_PX(f->y) > ag.rh * APD_TILE) f->alive = 0;
        }
        break;
    }
    case FO_SKIMMER: {
        int nx = f->x + f->dir * 200;
        if (solid_px(APD_PX(nx) + f->dir * f->w / 2, APD_PX(f->y)) || APD_PX(nx) < 8 ||
            APD_PX(nx) > ag.rw * APD_TILE - 8)
            f->dir = -f->dir;
        else f->x = nx;
        int wave = (f->t % 120 < 60 ? f->t % 60 : 60 - f->t % 60) - 30;
        int ny = f->b + wave * 20;
        if (!solid_px(APD_PX(f->x), APD_PX(ny) + 6) && !solid_px(APD_PX(f->x), APD_PX(ny) - 6)) f->y = ny;
        break;
    }
    case FO_SENTRY: {
        static const int8_t OX[8] = {16, 11, 0, -11, -16, -11, 0, 11}, OY[8] = {0, 11, 16, 11, 0, -11, -16, -11};
        int k = (f->t / 16 + i * 3) % 8;
        f->x += ((f->a + OX[k] * APD_FX) - f->x) / 16;
        f->y += ((f->b + OY[k] * APD_FX) - f->y) / 16;
        break;
    }
    default:
        if (APD_IS_BOSS(f->kind)) apd_boss_update(i);
        break;
    }
}

static int player_box(int *x, int *y, int *w, int *h) {
    const ApdPlayer *p = &ag.pl;
    *w = APD_PW;
    *h = p->crouch ? APD_PH_DUCK : APD_PH;
    *x = APD_PX(p->x) - *w / 2;
    int cy = APD_PX(p->y) + (p->crouch ? p->grav * (APD_PH - APD_PH_DUCK) / 2 : 0);
    *y = cy - *h / 2;
    return 0;
}

void apd_foes_update(void) {
    ApdPlayer *p = &ag.pl;
    int bx, by, bw, bh;
    for (int i = 0; i < APD_MAX_FOES; i++) {
        ApdFoe *f = &ag.foe[i];
        if (!f->alive) continue;
        foe_update(i);
        if (!f->alive) continue;
        player_box(&bx, &by, &bw, &bh);
        int fx = APD_PX(f->x) - f->w / 2, fy = APD_PX(f->y) - f->h / 2;
        /* STORMBALL and ANVIL: 50 once a flip */
        bool ball = p->thunder_t > 0, stomp = p->stomp;
        if (ball || stomp) {
            int r = ball ? 14 : 4;
            if (rects_overlap(bx - r, by - r, bw + 2 * r, bh + 2 * r, fx, fy, f->w, f->h) && f->hit_flip != p->flip_id &&
                !(f->flags & FF_NOHIT)) {
                f->hit_flip = p->flip_id;
                extern void apd_hurt_foe_by_player(int i, int dmg);
                apd_hurt_foe_by_player(i, APD_FLIP_DMG);
                apd_burst(APD_PX(f->x), APD_PX(f->y), ball ? C_CYAN : C_TAN, 6, 2);
                continue;
            }
        }
        if (!f->alive || (f->flags & FF_HIDDEN)) continue;
        if (rects_overlap(bx, by, bw, bh, fx, fy, f->w, f->h) && !ball && !stomp)
            apd_player_hurt(FOE[f->kind].dmg, f->kind, false);
    }
}

void apd_eshots_update(void) {
    int bx, by, bw, bh;
    player_box(&bx, &by, &bw, &bh);
    for (int i = 0; i < APD_MAX_ESHOTS; i++) {
        ApdEShot *e = &ag.es[i];
        if (!e->alive) continue;
        e->t++;
        if (--e->life <= 0) { e->alive = 0; continue; }
        if (e->kind == ES_LIGHTNING) {
            /* a strike: warned for 40 frames, then live for 30 */
            if (e->t > 70) { e->alive = 0; continue; }
            if (e->t > 40 && iabs(APD_PX(e->x) - (bx + bw / 2)) < e->w / 2 + bw / 2) apd_player_hurt(e->dmg, 300 + e->kind, false);
            continue;
        }
        e->vy += e->grav;
        e->x += e->vx;
        e->y += e->vy;
        int px = APD_PX(e->x), py = APD_PX(e->y);
        if (px < -16 || py < -16 || px > ag.rw * APD_TILE + 16 || py > ag.rh * APD_TILE + 16) { e->alive = 0; continue; }
        if (e->kind != ES_RING && e->kind != ES_ORB && solid_px(px, py) && apd_tile_at(px, py) != T_PLAT) {
            e->alive = 0;
            continue;
        }
        if (rects_overlap(bx, by, bw, bh, px - e->w / 2, py - e->h / 2, e->w, e->h)) {
            if (ag.pl.thunder_t > 0 && e->kind != ES_BEAM && e->kind != ES_LIGHTNING) { e->alive = 0; continue; }
            apd_player_hurt(e->dmg, 300 + e->kind, false);
            if (e->kind != ES_BEAM) e->alive = 0;
        }
    }
}
