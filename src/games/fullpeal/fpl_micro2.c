/* FULL PEAL - Clary's console, micro-games 26-50 (1-25 and the console
 * itself are in fpl_micro.c). Four colours, one small screen, one rule
 * each; every game ends on a mistake and keeps its high score. */
#include "fpl.h"

#define I fpm.ivar
#define F fpm.fvar
#define G fpm.grid
#define RECT(x, y, w, h, c) fpm_rect(ox, oy, (int)(x), (int)(y), (w), (h), (c))
#define PX(x, y, c) fpm_px(ox, oy, (int)(x), (int)(y), (c))
#define RND(a, b) fpm_rand((a), (b))
#define DIRX(m) ((((m) & MB_RIGHT) ? 1 : 0) - (((m) & MB_LEFT) ? 1 : 0))
#define DIRY(m) ((((m) & MB_DOWN) ? 1 : 0) - (((m) & MB_UP) ? 1 : 0))
#define PER_SEC(pts) do { if (fpm.gt % 60 == 0) fpm_add(pts); } while (0)
#define BOX(ax, ay, aw, ah, bx, by, bw, bh) rects_overlap((int)(ax), (int)(ay), (aw), (ah), (int)(bx), (int)(by), (bw), (bh))

static int free_slot(int from, int to) {
    for (int k = from; k < to; k++) if (!fpm.es[k]) return k;
    return -1;
}

/* fire held: one shot a press, or one every `every` frames held */
static bool trigger(uint8_t held, uint8_t pressed, int every) {
    if (pressed & MB_FIRE) { I[63] = every; return true; }
    if ((held & MB_FIRE) && --I[63] <= 0) { I[63] = every; return true; }
    return false;
}

/* 26 PIVOT: a turret in the middle, eight ways in                          */
/* ivar 0 facing (0 up, clockwise), 1 next spawn point, 2 spawn timer;
 * es 1 a creeper (0..19), 2 a shot (20..39) */
static const int P8X[8] = {0, 1, 1, 1, 0, -1, -1, -1}, P8Y[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static void g26_start(void) { I[0] = 0; I[1] = 0; I[2] = 40; }
static void g26_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (pressed & MB_LEFT) I[0] = (I[0] + 7) % 8;
    if (pressed & MB_RIGHT) I[0] = (I[0] + 1) % 8;
    if (pressed & MB_FIRE) {
        int k = free_slot(20, 40);
        if (k >= 0) { fpm.es[k] = 2; fpm.fx[k] = 42; fpm.fy[k] = 15; fpm.fvx[k] = P8X[I[0]] * 1.6f; fpm.fvy[k] = P8Y[I[0]] * 1.2f; }
    }
    if (--I[2] <= 0) {
        int k = free_slot(0, 20), p = I[1]++ % 8;
        if (k >= 0) {
            fpm.es[k] = 1;
            fpm.fx[k] = 42 + P8X[p] * 42.0f;
            fpm.fy[k] = 15 + P8Y[p] * 15.0f;
        }
        I[2] = imax(32, 70 - fpm.gt / 150);
    }
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        if (fpm.es[k] == 1) {
            float dx = 42 - fpm.fx[k], dy = 15 - fpm.fy[k], d = sqrtf(dx * dx + dy * dy);
            if (d < 4) { fpm_over(); return; }
            fpm.fx[k] += dx / d * 0.3f;
            fpm.fy[k] += dy / d * 0.3f;
            continue;
        }
        fpm.fx[k] += fpm.fvx[k];
        fpm.fy[k] += fpm.fvy[k];
        if (fpm.fx[k] < -2 || fpm.fx[k] > 88 || fpm.fy[k] < -2 || fpm.fy[k] > 34) { fpm.es[k] = 0; continue; }
        for (int e = 0; e < 20; e++)
            if (fpm.es[e] == 1 && BOX(fpm.fx[k], fpm.fy[k], 2, 2, fpm.fx[e] - 1, fpm.fy[e] - 1, 3, 3)) {
                fpm.es[e] = 0; fpm.es[k] = 0; fpm_add(2); sfx_play_name("fpm_point"); break;
            }
    }
}
static void g26_draw(int ox, int oy) {
    for (int k = 0; k < 40; k++) {
        if (fpm.es[k] == 1) RECT(fpm.fx[k] - 1, fpm.fy[k] - 1, 3, 3, MC_R);
        else if (fpm.es[k] == 2) RECT(fpm.fx[k], fpm.fy[k], 2, 2, MC_Y);
    }
    RECT(40, 13, 5, 5, MC_W);
    RECT(42 + P8X[I[0]] * 3, 15 + P8Y[I[0]] * 3, 1, 1, MC_Y);
    PX(42 + P8X[I[0]] * 4, 15 + P8Y[I[0]] * 4, MC_Y);
}

/* 27 LANES */
static void g27_start(void) { fpm_tilt_start(true); }

/* 28 ZAP: you are the cloud; strike the umbrella below                    */
/* fvar 0 cloud x, 1 cloud speed, 2 umbrella x, 3 umbrella speed; ivar 0 bolt shown */
static void g28_start(void) { F[0] = 20; F[1] = 0.6f; F[2] = 60; F[3] = -0.5f; }
static void g28_update(uint8_t held, uint8_t pressed) {
    (void)held;
    F[0] += F[1];
    if (F[0] < 0 || F[0] > 76) F[1] = -F[1];
    F[2] += F[3];
    if (F[2] < 0 || F[2] > 78) F[3] = -F[3];
    if (fpm.gt % 40 == 0 && RND(0, 2) == 0) F[3] = (RND(0, 1) ? 1 : -1) * (0.4f + RND(0, 5) * 0.1f);
    if (I[0] > 0) I[0]--;
    if (pressed & MB_FIRE) {
        I[0] = 8;
        if (fabsf((F[0] + 5) - (F[2] + 4)) < 5) { fpm_add(5); sfx_play_name("fpm_point"); }
        else fpm_over();
    }
}
static void g28_draw(int ox, int oy) {
    RECT(F[0], 1, 10, 4, MC_W);
    RECT(F[0] + 2, 0, 6, 1, MC_W);
    if (I[0] > 0)
        for (int y = 5; y < 24; y += 2) PX(F[0] + 5 + ((y / 2) % 2 ? 1 : -1), y, MC_Y);
    RECT(F[2], 24, 9, 2, MC_R);
    RECT(F[2] + 2, 23, 5, 1, MC_R);
    RECT(F[2] + 4, 26, 1, 5, MC_W);
}

/* 29 BOING: a ball that bounces by itself across drifting platforms      */
/* es 1 platform (fx left, fy top), 2 gold; ivar 0 platforms made */
static void boing_platform(float x) {
    int k = free_slot(0, 30);
    if (k < 0) return;
    fpm.es[k] = 1;
    fpm.fx[k] = x;
    fpm.fy[k] = (float)RND(14, 27);
    I[0]++;
    if (RND(0, 2) == 0) {
        int t = free_slot(30, 40);
        if (t >= 0) { fpm.es[t] = 2; fpm.fx[t] = x + 3; fpm.fy[t] = fpm.fy[k] - 12; }
    }
}
static void g29_start(void) {
    F[0] = 12; F[1] = 10; F[2] = 0;
    int k = 0;
    fpm.es[k] = 1; fpm.fx[k] = 6; fpm.fy[k] = 22;
    I[0] = 1;
    boing_platform(34);
    boing_platform(62);
}
static void g29_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    F[0] = fclamp(F[0] + DIRX(held) * 0.9f, 0, 83);
    F[2] += 0.08f;
    float y0 = F[1];
    F[1] += F[2];
    float right = -100;
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] -= 0.45f;
        if (fpm.fx[k] < -12) { fpm.es[k] = 0; continue; }
        if (fpm.es[k] == 1) {
            right = fmaxf(right, fpm.fx[k]);
            if (F[2] > 0 && y0 + 3 <= fpm.fy[k] && F[1] + 3 >= fpm.fy[k] && F[0] + 3 > fpm.fx[k] && F[0] < fpm.fx[k] + 10) {
                F[1] = fpm.fy[k] - 3;
                F[2] = -2.0f;
                sfx_play_name("fpm_blip");
            }
        } else if (BOX(F[0], F[1], 3, 3, fpm.fx[k], fpm.fy[k], 3, 3)) {
            fpm.es[k] = 0; fpm_add(5); sfx_play_name("fpm_point");
        }
    }
    int gap = imin(14 + I[0] / 8, 30);
    if (right < 86 - 10 - gap) boing_platform(86);
    if (F[1] > 30) fpm_over();
}
static void g29_draw(int ox, int oy) {
    for (int x = 0; x < 86; x += 4) PX(x + (fpm.gt / 8) % 4, 31, MC_W);
    for (int k = 0; k < 40; k++) {
        if (fpm.es[k] == 1) RECT(fpm.fx[k], fpm.fy[k], 10, 2, MC_R);
        else if (fpm.es[k] == 2) RECT(fpm.fx[k], fpm.fy[k], 3, 3, MC_Y);
    }
    RECT(F[0], F[1], 3, 3, MC_W);
}

/* 30 OGRE 2 */
static void g30_start(void) { fpm_boss_start(1); }

/* 31 RING: an arena with a gun at each side wall; shoot the rocks         */
/* fvar 0,1 you; ivar 0,1 facing, 2 next corner, 3 spawn timer, 4 spawns,
 * 5 laser timer (0 none, 1..20 warning, 21..40 beam), 6 laser row y;
 * es 1 drifting rock, 2 homing rock (0..19), 3 your shot (20..39) */
static void g31_start(void) { F[0] = 42; F[1] = 15; I[0] = 1; I[1] = 0; I[3] = 40; }
static void g31_update(uint8_t held, uint8_t pressed) {
    int dx = DIRX(held), dy = DIRY(held);
    if (dx || dy) { I[0] = dx; I[1] = dy; }
    F[0] = fclamp(F[0] + dx * 1.0f, 4, 79);
    F[1] = fclamp(F[1] + dy * 1.0f, 0, 29);
    if (trigger(held, pressed, 10)) {
        int k = free_slot(20, 40);
        if (k >= 0) { fpm.es[k] = 3; fpm.fx[k] = F[0] + 1; fpm.fy[k] = F[1] + 1; fpm.fvx[k] = I[0] * 2.0f; fpm.fvy[k] = I[1] * 2.0f; }
    }
    if (--I[3] <= 0) {
        int k = free_slot(0, 20), c = I[2]++ % 4;
        static const float CX[4] = {5, 79, 79, 5}, CY[4] = {0, 0, 29, 29};
        if (k >= 0) {
            fpm.es[k] = c % 2 ? 2 : 1;
            fpm.fx[k] = CX[c];
            fpm.fy[k] = CY[c];
            fpm.fvx[k] = c == 0 || c == 3 ? 0.6f : -0.6f;
            fpm.fvy[k] = c < 2 ? 0.45f : -0.45f;
        }
        if (++I[4] % 5 == 0) { I[5] = 1; I[6] = (int)F[1] + 1; }
        I[3] = imax(40, 80 - fpm.gt / 150);
    }
    if (I[5] > 0) {
        if (++I[5] > 40) I[5] = 0;
        else if (I[5] > 20 && BOX(F[0], F[1], 3, 3, 0, I[6] - 1, 86, 2)) { fpm_over(); return; }
    }
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        if (fpm.es[k] == 2) {
            float ddx = F[0] - fpm.fx[k], ddy = F[1] - fpm.fy[k], d = sqrtf(ddx * ddx + ddy * ddy);
            if (d > 0.1f) { fpm.fvx[k] = ddx / d * 0.35f; fpm.fvy[k] = ddy / d * 0.35f; }
        }
        fpm.fx[k] += fpm.fvx[k];
        fpm.fy[k] += fpm.fvy[k];
        if (fpm.es[k] == 3) {
            if (fpm.fx[k] < 0 || fpm.fx[k] > 86 || fpm.fy[k] < 0 || fpm.fy[k] > 32) { fpm.es[k] = 0; continue; }
            for (int e = 0; e < 20; e++)
                if (fpm.es[e] && BOX(fpm.fx[k], fpm.fy[k], 1, 1, fpm.fx[e], fpm.fy[e], 3, 3)) {
                    fpm.es[e] = 0; fpm.es[k] = 0; fpm_add(2); sfx_play_name("fpm_point"); break;
                }
            continue;
        }
        if (fpm.fx[k] < 4 || fpm.fx[k] > 79) fpm.fvx[k] = -fpm.fvx[k];
        if (fpm.fy[k] < 0 || fpm.fy[k] > 29) fpm.fvy[k] = -fpm.fvy[k];
        if (BOX(F[0], F[1], 3, 3, fpm.fx[k], fpm.fy[k], 3, 3)) { fpm_over(); return; }
    }
}
static void g31_draw(int ox, int oy) {
    RECT(0, 0, 3, 32, MC_R);
    RECT(83, 0, 3, 32, MC_R);
    RECT(0, I[5] > 0 ? I[6] - 2 : 14, 4, 4, MC_W);
    RECT(82, I[5] > 0 ? I[6] - 2 : 14, 4, 4, MC_W);
    if (I[5] > 20) RECT(0, I[6] - 1, 86, 2, MC_Y);
    else if (I[5] > 0 && (I[5] / 3) % 2) RECT(4, I[6], 78, 1, MC_Y);
    for (int k = 0; k < 40; k++) {
        if (fpm.es[k] == 1 || fpm.es[k] == 2) RECT(fpm.fx[k], fpm.fy[k], 3, 3, MC_R);
        else if (fpm.es[k] == 3) PX(fpm.fx[k], fpm.fy[k], MC_Y);
    }
    RECT(F[0], F[1], 3, 3, MC_W);
    PX(F[0] + 1 + I[0] * 2, F[1] + 1 + I[1] * 2, MC_Y);
}

/* 32 THAW: every tile you land on melts once you jump off it; the bat
 * keeps to the air (and you walk on air until you jump) */
/* grid[c][0] a tile; fvar 0 x, 1 feet y, 2 vy, 3 bat x, 4 bat y; ivar 0 standing, 1 the tile stood on */
#define TH_N 14
static void g32_start(void) {
    for (int c = 0; c < TH_N; c++) G[c][0] = 1;
    F[0] = 40; F[1] = 24; I[0] = 1; I[1] = 40 / 6; F[3] = 20; F[4] = 12;
}
static void g32_update(uint8_t held, uint8_t pressed) {
    F[0] = fclamp(F[0] + DIRX(held) * 0.8f, 0, 81);
    if (I[0] && (pressed & MB_A)) {
        /* jumping off melts the tile stood on */
        G[I[1]][0] = 0;
        I[0] = 0;
        F[2] = -1.8f;
        sfx_play_name("fpm_blip");
    }
    if (!I[0]) {
        F[2] += 0.1f;
        F[1] += F[2];
        if (F[2] > 0 && F[1] >= 24 && F[1] - F[2] < 24.01f) {
            int c = iclamp((int)(F[0] + 1) / 6, 0, TH_N - 1);
            if (G[c][0]) { F[1] = 24; I[0] = 1; I[1] = c; }
        }
        if (F[1] > 33) { fpm_over(); return; }
    }
    if (fpm.gt % 3 == 0) {
        F[3] = fclamp(F[3] + RND(-2, 2), 0, 82);
        F[4] = fclamp(F[4] + RND(-1, 1) * 0.5f, 8, 18);
    }
    if (BOX(F[0], F[1] - 5, 3, 5, F[3], F[4], 4, 2)) { fpm_over(); return; }
    PER_SEC(10);
}
static void g32_draw(int ox, int oy) {
    for (int c = 0; c < TH_N; c++) if (G[c][0]) RECT(1 + c * 6, 24, 5, 3, MC_Y);
    RECT(F[3], F[4] + (fpm.gt / 6) % 2, 4, 2, MC_R);
    RECT(F[0], F[1] - 5, 3, 5, MC_W);
}

/* 33 LAMP: hunt a slug in the dark with a lamp; press when it's lit       */
/* fvar 0,1 the light, 2,3 the slug, 4,5 its speed; ivar 0 round timer */
static void lamp_round(void) {
    F[2] = (float)RND(4, 80); F[3] = (float)RND(3, 28);
    F[4] = RND(0, 1) ? 0.8f : -0.8f; F[5] = RND(0, 1) ? 0.6f : -0.6f;
    I[0] = 600;
}
static void g33_start(void) { F[0] = 42; F[1] = 15; lamp_round(); }
static void g33_update(uint8_t held, uint8_t pressed) {
    F[0] = fclamp(F[0] + DIRX(held) * 1.2f, 0, 85);
    F[1] = fclamp(F[1] + DIRY(held) * 1.2f, 0, 31);
    if (fpm.gt % 20 == 0) { F[4] = (RND(0, 1) ? 1 : -1) * (0.5f + RND(0, 5) * 0.1f); F[5] = (RND(0, 1) ? 1 : -1) * (0.3f + RND(0, 5) * 0.1f); }
    F[2] += F[4];
    F[3] += F[5];
    if (F[2] < 1 || F[2] > 83) F[4] = -F[4];
    if (F[3] < 1 || F[3] > 29) F[5] = -F[5];
    F[2] = fclamp(F[2], 1, 83);
    F[3] = fclamp(F[3], 1, 29);
    if (pressed & MB_FIRE) {
        float dx = F[2] - F[0], dy = F[3] - F[1];
        if (dx * dx + dy * dy < 64) { fpm_add(10); sfx_play_name("fpm_point"); lamp_round(); }
        else fpm_over();
        return;
    }
    if (--I[0] <= 0) fpm_over();
}
static void g33_draw(int ox, int oy) {
    for (int a = 0; a < 24; a++) PX(F[0] + cosf(a * 0.2618f) * 8, F[1] + sinf(a * 0.2618f) * 8, MC_W);
    float dx = F[2] - F[0], dy = F[3] - F[1];
    if (dx * dx + dy * dy < 64 || fpm.state == MC_OVER) { RECT(F[2] - 1, F[3], 3, 1, MC_Y); PX(F[2] + 2, F[3] - 1, MC_Y); }
    RECT(0, 31, I[0] * 86 / 600, 1, MC_R);
}

/* 34 SAYSO: do what the screen says, quicker and quicker                  */
/* ivar 0 the order (0 up 1 down 2 left 3 right 4 button), 1 time left, 2 time allowed */
static void sayso_next(void) { I[0] = RND(0, 4); I[1] = I[2]; }
static void g34_start(void) { I[2] = 120; sayso_next(); }
static void g34_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (fpm.gt >= 3600) { fpm_over(); return; }
    static const uint8_t WANT[5] = {MB_UP, MB_DOWN, MB_LEFT, MB_RIGHT, MB_FIRE};
    uint8_t p = pressed & (MB_DIRS | MB_FIRE);
    if (p) {
        if ((p & WANT[I[0]]) && !(p & ~WANT[I[0]])) {
            fpm_add(1);
            sfx_play_name("fpm_point");
            I[2] = imax(30, I[2] - 4);
            sayso_next();
        } else fpm_over();
        return;
    }
    if (--I[1] <= 0) fpm_over();
}
static void g34_draw(int ox, int oy) {
    static const char *const SAY[5] = {GLYPH_UP, GLYPH_DOWN, GLYPH_LEFT, GLYPH_RIGHT, GLYPH_A};
    text_draw_scaled(SAY[I[0]], ox + 37, oy + 6, I[0] == 4 ? MC_R : MC_Y, 2);
    RECT(0, 30, I[1] * 86 / imax(1, I[2]), 2, MC_W);
}

/* 35 TACK: a tack slides along the floor; lift a foot at the right moment */
/* fvar 0 tack x, 1 tack speed, 2 your middle; ivar 0 step frames left, 1 step way */
static void g35_start(void) { F[0] = 2; F[1] = 0.7f; F[2] = 50; }
static int tack_foot_up(int foot) {
    /* a step lifts the leading foot first, then the other */
    if (I[0] <= 0) return 0;
    int lead = I[1] > 0 ? 1 : 0;
    return I[0] > 4 ? foot == lead : foot != lead;
}
static void g35_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (I[0] <= 0 && (pressed & (MB_LEFT | MB_RIGHT))) { I[0] = 8; I[1] = DIRX(pressed); }
    if (I[0] > 0) {
        F[2] = fclamp(F[2] + I[1] * 0.375f, 4, 80);
        I[0]--;
    }
    F[0] += F[1];
    if (F[0] < 0 || F[0] > 84) {
        F[0] = fclamp(F[0], 0, 84);
        F[1] = -F[1] * 1.02f;
        fpm_add(10);
        sfx_play_name("fpm_point");
    }
    for (int foot = 0; foot < 2; foot++) {
        float fx = F[2] + (foot ? 3 : -3);
        if (!tack_foot_up(foot) && fabsf(fx - (F[0] + 1)) < 1.5f) { fpm_over(); return; }
    }
}
static void g35_draw(int ox, int oy) {
    RECT(0, 30, 86, 2, MC_W);
    RECT(F[2] - 2, 14, 5, 5, MC_W);
    RECT(F[2] - 1, 19, 3, 6, MC_W);
    for (int foot = 0; foot < 2; foot++) {
        float fx = F[2] + (foot ? 3 : -3);
        int up = tack_foot_up(foot) ? 3 : 0;
        RECT(F[2] + (foot ? 1 : -1), 25, 1, 4 - up, MC_W);
        RECT(fx - 1, 29 - up, 3, 1, MC_W);
    }
    PX(F[0] + 1, 28, MC_R);
    RECT(F[0], 29, 3, 1, MC_R);
}

/* 36 PICK: fruit after fruit; press on the cherry and only the cherry     */
/* ivar 0 the fruit (0 orange 1 pear 2 apple 3 banana 4 cherry), 1 frames left */
static int pick_time(void) { return imax(12, 50 - fpm.gt / 120); }
static void g36_start(void) { I[0] = 2; I[1] = pick_time(); }
static void g36_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (pressed & MB_FIRE) {
        if (I[0] == 4) { fpm_add(10); sfx_play_name("fpm_point"); I[0] = 0; I[1] = pick_time(); }
        else { fpm_over(); }
        return;
    }
    if (--I[1] <= 0) { I[0] = (I[0] + 1) % 5; I[1] = pick_time(); }
}
static void g36_draw(int ox, int oy) {
    int x = 37, y = 6;
    switch (I[0]) {
    case 0: RECT(x + 2, y + 2, 9, 9, MC_Y); RECT(x + 3, y + 1, 7, 11, MC_Y); RECT(x + 1, y + 3, 11, 7, MC_Y); RECT(x + 6, y - 1, 2, 2, MC_W); break;
    case 1: RECT(x + 4, y, 4, 5, MC_Y); RECT(x + 2, y + 5, 8, 7, MC_Y); RECT(x + 5, y - 2, 1, 2, MC_W); break;
    case 2: RECT(x + 1, y + 2, 11, 9, MC_R); RECT(x + 3, y + 1, 7, 11, MC_R); RECT(x + 6, y - 1, 1, 3, MC_W); break;
    case 3: for (int i = 0; i < 12; i++) RECT(x + i, y + 4 + (i - 6) * (i - 6) / 6, 2, 3, MC_Y); break;
    default:
        RECT(x + 1, y + 7, 5, 5, MC_R); RECT(x + 8, y + 6, 5, 5, MC_R);
        for (int i = 0; i < 6; i++) { PX(x + 3 + i / 2, y + 6 - i, MC_W); PX(x + 10 - i / 3, y + 5 - i, MC_W); }
        break;
    }
}

/* 37 WHICH?: ninety-nine dots, gone in a moment: more gold, or more red?   */
/* grid holds the colours (1 gold, 2 red); ivar 0 gold count, 1 shown left, 2 rounds, 3 pause */
static void which_round(void) {
    int gold = RND(30, 69);
    I[0] = gold;
    int placed = 0;
    for (int i = 0; i < 99; i++) { G[i % 11][i / 11] = 2; }
    while (placed < gold) {
        int i = RND(0, 98);
        if (G[i % 11][i / 11] == 2) { G[i % 11][i / 11] = 1; placed++; }
    }
    I[1] = imax(15, 90 - I[2] * 5);
    I[3] = 0;
}
static void g37_start(void) { I[2] = 0; which_round(); }
static void g37_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (I[3] > 0) { if (--I[3] == 0) which_round(); return; }
    if (I[1] > 0) I[1]--;
    if (pressed & (MB_UP | MB_DOWN)) {
        bool gold_more = I[0] > 99 - I[0];
        if (((pressed & MB_UP) != 0) == gold_more) { fpm_add(5); sfx_play_name("fpm_point"); I[2]++; I[3] = 20; I[1] = 0; }
        else fpm_over();
    }
}
static void g37_draw(int ox, int oy) {
    if (I[1] > 0 || fpm.state == MC_OVER)
        for (int i = 0; i < 99; i++) RECT(26 + (i % 11) * 3, 2 + (i / 11) * 3, 2, 2, G[i % 11][i / 11] == 1 ? MC_Y : MC_R);
    else {
        text_draw(GLYPH_UP " GOLD", ox + 4, oy + 6, MC_Y);
        text_draw(GLYPH_DOWN " RED", ox + 4, oy + 18, MC_R);
    }
}

/* 38 DRAW: two at high noon; draw when the other reaches, not before      */
/* ivar 0 phase (0 waiting, 1 reaching, 2 won the round), 1 timer, 2 rounds */
static void draw_round(void) {
    I[0] = 0;
    I[1] = I[2] == 0 ? 60 : 15 * RND(1, 8);
}
static void g38_start(void) { I[2] = 0; draw_round(); }
static void g38_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (fpm.gt >= 3600) { fpm_over(); return; }
    if (I[0] == 2) { if (--I[1] <= 0) draw_round(); return; }
    if (pressed & MB_FIRE) {
        if (I[0] == 1) { fpm_add(5); sfx_play_name("fpm_point"); I[0] = 2; I[1] = 40; I[2]++; }
        else fpm_over();
        return;
    }
    if (--I[1] <= 0) {
        if (I[0] == 0) { I[0] = 1; I[1] = 24; }
        else fpm_over();
    }
}
static void g38_draw(int ox, int oy) {
    RECT(0, 28, 86, 1, MC_W);
    /* you on the left, them on the right */
    RECT(14, 14, 4, 14, MC_W); RECT(14, 10, 4, 4, MC_W);
    RECT(68, 14, 4, 14, MC_R); RECT(68, 10, 4, 4, MC_R);
    if (I[0] >= 1) RECT(64, 16, 4, 1, MC_R);
    if (I[0] == 2) RECT(18, 16, 6, 1, MC_Y);
    if (I[0] == 1 && (fpm.gt / 3) % 2) fpm_text(ox, oy, "!", 69, 2, MC_Y);
}

/* 39 BOBBLE: a ball that bounces by itself between fences up and down     */
/* fvar 0 x, 1 y, 2 vy; es 1 fence from the top, 2 fence from the floor, 3 gold; ivar 0 timer, 1 gold taken */
static void g39_start(void) { F[0] = 20; F[1] = 10; F[2] = 0; I[0] = 50; }
static void g39_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    F[0] = fclamp(F[0] + DIRX(held) * 0.9f, 0, 83);
    F[2] += 0.09f;
    F[1] += F[2];
    if (F[1] >= 28) { F[1] = 28; F[2] = -2.2f; }
    if (--I[0] <= 0) {
        int k = free_slot(0, 40);
        if (k >= 0) {
            int r = RND(0, 9);
            fpm.es[k] = r < 4 ? 1 : r < 8 ? 2 : 3;
            fpm.fx[k] = 88;
            fpm.fy[k] = fpm.es[k] == 3 ? (float)RND(4, 24) : (float)RND(6, 10);
        }
        I[0] = imax(18, (I[1] > 70 ? 30 : 60) - I[1] / 4);
    }
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] -= 0.8f;
        if (fpm.fx[k] < -4) { fpm.es[k] = 0; continue; }
        int h = (int)fpm.fy[k];
        bool hit = fpm.es[k] == 1 ? BOX(F[0], F[1], 3, 3, fpm.fx[k], 0, 2, h)
                 : fpm.es[k] == 2 ? BOX(F[0], F[1], 3, 3, fpm.fx[k], 32 - h, 2, h)
                 : BOX(F[0], F[1], 3, 3, fpm.fx[k], fpm.fy[k], 3, 3);
        if (!hit) continue;
        if (fpm.es[k] == 3) { fpm.es[k] = 0; I[1]++; fpm_add(10); sfx_play_name("fpm_point"); }
        else { fpm_over(); return; }
    }
}
static void g39_draw(int ox, int oy) {
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        int h = (int)fpm.fy[k];
        if (fpm.es[k] == 1) RECT(fpm.fx[k], 0, 2, h, MC_R);
        else if (fpm.es[k] == 2) RECT(fpm.fx[k], 32 - h, 2, h, MC_R);
        else RECT(fpm.fx[k], fpm.fy[k], 3, 3, MC_Y);
    }
    RECT(F[0], F[1], 3, 3, MC_W);
}

/* 40 PENNY: coins pour down; some are fakes                               */
static void g40_start(void) { F[0] = 42; I[0] = 20; }
static void g40_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    F[0] = fclamp(F[0] + DIRX(held) * 1.3f, 0, 80);
    if (--I[0] <= 0) {
        int k = free_slot(0, 40);
        if (k >= 0) {
            fpm.es[k] = RND(0, 9) < 7 ? 1 : 2;
            fpm.fx[k] = (float)RND(0, 83);
            fpm.fy[k] = -3;
            fpm.fvy[k] = 0.5f + RND(0, 5) * 0.1f + fminf(fpm.gt / 7200.0f, 0.5f);
        }
        I[0] = imax(6, 18 - fpm.gt / 600);
    }
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        fpm.fy[k] += fpm.fvy[k];
        if (fpm.fy[k] > 33) { fpm.es[k] = 0; continue; }
        if (BOX(F[0], 28, 6, 3, fpm.fx[k], fpm.fy[k], 3, 3)) {
            if (fpm.es[k] == 1) { fpm.es[k] = 0; fpm_add(2); sfx_play_name("fpm_point"); }
            else { fpm_over(); return; }
        }
    }
}
static void g40_draw(int ox, int oy) {
    for (int k = 0; k < 40; k++) if (fpm.es[k]) RECT(fpm.fx[k], fpm.fy[k], 3, 3, fpm.es[k] == 1 ? MC_Y : MC_R);
    RECT(F[0], 28, 6, 3, MC_W);
}

/* 41 SHOVE: a wall of gunners closes in from the right; every shot pushes
 * one back; one that reaches the left edge is gone for good */
/* es 1 gunner (0..3), et its fire timer; 10..39: 2 your shot, 3 theirs */
static void g41_start(void) {
    F[0] = 8; F[1] = 14;
    for (int k = 0; k < 4; k++) { fpm.es[k] = 1; fpm.fx[k] = 78; fpm.fy[k] = 1.0f + k * 8; fpm.et[k] = RND(40, 120); }
}
static void g41_update(uint8_t held, uint8_t pressed) {
    F[0] = fclamp(F[0] + DIRX(held) * 1.0f, 0, 82);
    F[1] = fclamp(F[1] + DIRY(held) * 1.0f, 0, 29);
    if (trigger(held, pressed, 8)) {
        int k = free_slot(10, 40);
        if (k >= 0) { fpm.es[k] = 2; fpm.fx[k] = F[0] + 3; fpm.fy[k] = F[1] + 1; }
    }
    for (int k = 0; k < 4; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] -= 0.05f;
        if (fpm.fx[k] < 0) { fpm.es[k] = 0; continue; }
        if (--fpm.et[k] <= 0) {
            int s = free_slot(10, 40);
            if (s >= 0) { fpm.es[s] = 3; fpm.fx[s] = fpm.fx[k]; fpm.fy[s] = fpm.fy[k] + 2; }
            fpm.et[k] = RND(50, 110);
        }
        if (BOX(F[0], F[1], 3, 3, fpm.fx[k], fpm.fy[k], 6, 6)) { fpm_over(); return; }
    }
    for (int k = 10; k < 40; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] += fpm.es[k] == 2 ? 2.0f : -0.9f;
        if (fpm.fx[k] < -2 || fpm.fx[k] > 88) { fpm.es[k] = 0; continue; }
        if (fpm.es[k] == 3) {
            if (BOX(F[0], F[1], 3, 3, fpm.fx[k], fpm.fy[k], 2, 1)) { fpm_over(); return; }
            continue;
        }
        for (int g = 0; g < 4; g++)
            if (fpm.es[g] && BOX(fpm.fx[k], fpm.fy[k], 2, 1, fpm.fx[g], fpm.fy[g], 6, 6)) {
                fpm.es[k] = 0;
                fpm.fx[g] = fminf(fpm.fx[g] + 2.0f, 80);
                sfx_play_name("fpm_blip");
                break;
            }
    }
    PER_SEC(10);
}
static void g41_draw(int ox, int oy) {
    for (int k = 0; k < 4; k++) if (fpm.es[k]) { RECT(fpm.fx[k], fpm.fy[k], 6, 6, MC_R); RECT(fpm.fx[k] - 2, fpm.fy[k] + 2, 2, 1, MC_R); }
    for (int k = 10; k < 40; k++) if (fpm.es[k]) RECT(fpm.fx[k], fpm.fy[k], 2, 1, fpm.es[k] == 2 ? MC_Y : MC_R);
    RECT(F[0], F[1], 3, 3, MC_W);
}

/* 42 KEEPUP: one ball, then two, then more; drop none                      */
static void keepup_ball(int k) { fpm.es[k] = 1; fpm.fx[k] = 42; fpm.fy[k] = 2; fpm.fvx[k] = RND(0, 1) ? 0.6f : -0.6f; fpm.fvy[k] = 0.7f; }
static void g42_start(void) { F[0] = 43; keepup_ball(0); }
static void g42_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    F[0] = fclamp(F[0] + DIRX(held) * 1.5f, 6, 80);
    if (fpm.gt % 480 == 0) { int k = free_slot(0, 12); if (k >= 0) keepup_ball(k); }
    for (int k = 0; k < 12; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] += fpm.fvx[k];
        fpm.fy[k] += fpm.fvy[k];
        if (fpm.fx[k] < 0) { fpm.fx[k] = 0; fpm.fvx[k] = fabsf(fpm.fvx[k]); }
        if (fpm.fx[k] > 84) { fpm.fx[k] = 84; fpm.fvx[k] = -fabsf(fpm.fvx[k]); }
        if (fpm.fy[k] < 0) { fpm.fy[k] = 0; fpm.fvy[k] = fabsf(fpm.fvy[k]); }
        fpm_paddle_ball(&fpm.fx[k], &fpm.fy[k], &fpm.fvx[k], &fpm.fvy[k], F[0], 12, 29);
        if (fpm.fy[k] > 32) { fpm_over(); return; }
    }
    PER_SEC(10);
}
static void g42_draw(int ox, int oy) {
    for (int k = 0; k < 12; k++) if (fpm.es[k]) RECT(fpm.fx[k], fpm.fy[k], 2, 2, MC_Y);
    RECT(F[0] - 6, 29, 12, 2, MC_W);
}

/* 43 WARP: you can't walk, only jump three or four squares at a time,
 * through the walls that sweep the screen */
/* ivar 0,1 you (cells); es 1 a wall: fx its column (cells), fvx its way */
static void g43_start(void) { I[0] = 10; I[1] = 4; }
static void g43_update(uint8_t held, uint8_t pressed) {
    (void)held;
    int dx = DIRX(pressed), dy = dx ? 0 : DIRY(pressed);
    if (dx || dy) {
        int step = RND(3, 4);
        I[0] = iclamp(I[0] + dx * step, 0, 20);
        I[1] = iclamp(I[1] + dy * step, 0, 7);
        sfx_play_name("fpm_blip");
    }
    if (fpm.gt % 120 == 60) {
        int k = free_slot(0, 10);
        if (k >= 0) {
            bool left = RND(0, 1);
            fpm.es[k] = 1;
            fpm.fx[k] = left ? -1.0f : 21.0f;
            fpm.fvx[k] = left ? 1.0f : -1.0f;
        }
    }
    for (int k = 0; k < 10; k++) {
        if (!fpm.es[k]) continue;
        if (fpm.gt % 3 == 0) fpm.fx[k] += fpm.fvx[k];
        if (fpm.fx[k] < -2 || fpm.fx[k] > 22) { fpm.es[k] = 0; continue; }
        if ((int)fpm.fx[k] == I[0]) { fpm_over(); return; }
    }
    PER_SEC(5);
}
static void g43_draw(int ox, int oy) {
    for (int k = 0; k < 10; k++) if (fpm.es[k]) RECT(1 + (int)fpm.fx[k] * 4, 0, 4, 32, MC_R);
    RECT(1 + I[0] * 4, I[1] * 4, 4, 4, MC_W);
    PX(1 + I[0] * 4 + 1, I[1] * 4 + 1, MC_Y);
}

/* 44 THRONG: shoot holes through the red walls; never touch the gold one  */
/* es 1 red block, 2 gold block (0..39: x in fx, row in et); 3 your shot (kept in ex/ey/fvx of 0..9 via grid) */
static void g44_start(void) { F[0] = 6; F[1] = 14; I[0] = 30; I[1] = 0; fpm.n = 0; }
static void g44_update(uint8_t held, uint8_t pressed) {
    F[0] = fclamp(F[0] + DIRX(held) * 1.0f, 0, 60);
    F[1] = fclamp(F[1] + DIRY(held) * 1.0f, 0, 29);
    if (trigger(held, pressed, 8))
        for (int s = 0; s < 8; s++) if (!G[s][0]) { G[s][0] = 1; fpm.ex[s] = (int)F[0] + 3; fpm.ey[s] = (int)F[1] + 1; break; }
    if (--I[0] <= 0) {
        if (++I[1] % 5 == 0) {
            int k = free_slot(0, 40);
            if (k >= 0) { fpm.es[k] = 2; fpm.fx[k] = 86; fpm.et[k] = RND(0, 7); }
        } else {
            for (int r = 0; r < 8; r++) {
                int k = free_slot(0, 40);
                if (k >= 0) { fpm.es[k] = 1; fpm.fx[k] = 86; fpm.et[k] = r; }
            }
        }
        I[0] = 90;
    }
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] -= 0.35f;
        if (fpm.fx[k] < -4) { fpm.es[k] = 0; continue; }
        if (BOX(F[0], F[1], 3, 3, fpm.fx[k], fpm.et[k] * 4, 4, 4)) { fpm_over(); return; }
    }
    for (int s = 0; s < 8; s++) {
        if (!G[s][0]) continue;
        fpm.ex[s] += 2;
        if (fpm.ex[s] > 86) { G[s][0] = 0; continue; }
        for (int k = 0; k < 40; k++)
            if (fpm.es[k] && BOX(fpm.ex[s], fpm.ey[s], 2, 1, fpm.fx[k], fpm.et[k] * 4, 4, 4)) {
                G[s][0] = 0;
                if (fpm.es[k] == 2) { fpm_over(); return; }
                fpm.es[k] = 0;
                fpm_add(5);
                sfx_play_name("fpm_point");
                break;
            }
    }
}
static void g44_draw(int ox, int oy) {
    for (int k = 0; k < 40; k++) if (fpm.es[k]) RECT(fpm.fx[k], fpm.et[k] * 4, 4, 4, fpm.es[k] == 1 ? MC_R : MC_Y);
    for (int s = 0; s < 8; s++) if (G[s][0]) RECT(fpm.ex[s], fpm.ey[s], 2, 1, MC_W);
    RECT(F[0], F[1], 3, 3, MC_W);
}

/* 45 BLAST: run and gun; jump the rolling shots, mind the floating posts  */
/* fvar 0 x, 1 feet y, 2 vy; ivar 0 facing, 1 spawn timer;
 * es: 1 roller (3 hp), 2 hulk (5 hp), 3 bat (2 hp), 4 post (no harm done to it),
 * 5 a ground shot, 6 a dropped shot (0..29); 30..39 your shots (fvx, fvy) */
static const int BL_HP[5] = {0, 3, 5, 2, 0};
static void g45_start(void) { F[0] = 10; F[1] = 30; I[0] = 1; I[1] = 60; }
static void g45_update(uint8_t held, uint8_t pressed) {
    int dx = DIRX(held);
    if (dx) I[0] = dx;
    F[0] = fclamp(F[0] + dx * 0.9f, 0, 82);
    if (F[1] >= 30 && (pressed & MB_A)) F[2] = -1.9f;
    F[2] += 0.1f;
    F[1] += F[2];
    if (F[1] >= 30) { F[1] = 30; F[2] = 0; }
    if (pressed & MB_B) {
        int k = free_slot(30, 40);
        if (k >= 0) {
            fpm.es[k] = 7;
            fpm.fx[k] = F[0] + 1; fpm.fy[k] = F[1] - 4;
            bool up = held & MB_UP;
            fpm.fvx[k] = up ? 0 : I[0] * 2.0f;
            fpm.fvy[k] = up ? -2.0f : 0;
        }
    }
    if (--I[1] <= 0) {
        int k = free_slot(0, 20);
        if (k >= 0) {
            int r = RND(0, 9), kind = r < 4 ? 1 : r < 6 ? 2 : r < 8 ? 3 : 4;
            fpm.es[k] = kind;
            fpm.et[k] = BL_HP[kind];
            fpm.fx[k] = 88;
            fpm.fy[k] = kind == 3 ? (float)RND(8, 14) : kind == 4 ? 14 : (kind == 2 ? 24 : 26);
            fpm.ex[k] = RND(40, 90);
        }
        I[1] = imax(40, 110 - fpm.gt / 90);
    }
    for (int k = 0; k < 40; k++) {
        int e = fpm.es[k];
        if (!e) continue;
        if (e == 7) {
            fpm.fx[k] += fpm.fvx[k];
            fpm.fy[k] += fpm.fvy[k];
            if (fpm.fx[k] < -2 || fpm.fx[k] > 88 || fpm.fy[k] < -2) { fpm.es[k] = 0; continue; }
            for (int t = 0; t < 20; t++) {
                int te = fpm.es[t];
                if (te < 1 || te > 4) continue;
                int w = te == 2 ? 6 : 4, h = te == 2 ? 6 : te == 4 ? 8 : 4;
                if (!BOX(fpm.fx[k], fpm.fy[k], 2, 1, fpm.fx[t], fpm.fy[t], w, h)) continue;
                fpm.es[k] = 0;
                if (te != 4 && --fpm.et[t] <= 0) { fpm.es[t] = 0; fpm_add(10); sfx_play_name("fpm_point"); }
                break;
            }
            continue;
        }
        if (e == 5 || e == 6) {
            fpm.fx[k] += e == 5 ? -1.2f : 0;
            fpm.fy[k] += e == 6 ? 0.9f : 0;
            if (fpm.fx[k] < -3 || fpm.fy[k] > 33) { fpm.es[k] = 0; continue; }
            if (BOX(F[0], F[1] - 5, 3, 5, fpm.fx[k], fpm.fy[k], 2, 2)) { fpm_over(); return; }
            continue;
        }
        fpm.fx[k] -= e == 3 ? 0.5f : e == 4 ? 0.6f : e == 2 ? 0.3f : 0.45f;
        if (e == 3 && fpm.gt % 4 == 0) fpm.fy[k] = fclamp(fpm.fy[k] + RND(-1, 1), 6, 16);
        if (fpm.fx[k] < -6) { fpm.es[k] = 0; continue; }
        if (--fpm.ex[k] <= 0 && (e == 1 || e == 3)) {
            int s = free_slot(20, 30);
            if (s >= 0) { fpm.es[s] = e == 1 ? 5 : 6; fpm.fx[s] = fpm.fx[k]; fpm.fy[s] = e == 1 ? 29 : fpm.fy[k] + 4; }
            fpm.ex[k] = RND(60, 120);
        }
        int w = e == 2 ? 6 : 4, h = e == 2 ? 6 : e == 4 ? 8 : 4;
        if (BOX(F[0], F[1] - 5, 3, 5, fpm.fx[k], fpm.fy[k], w, h)) { fpm_over(); return; }
    }
}
static void g45_draw(int ox, int oy) {
    RECT(0, 30, 86, 2, MC_W);
    for (int k = 0; k < 40; k++) {
        int e = fpm.es[k];
        if (!e) continue;
        switch (e) {
        case 1: RECT(fpm.fx[k], fpm.fy[k], 4, 4, MC_R); break;
        case 2: RECT(fpm.fx[k], fpm.fy[k], 6, 6, MC_R); PX(fpm.fx[k] + 1, fpm.fy[k] + 1, MC_Y); break;
        case 3: RECT(fpm.fx[k], fpm.fy[k] + (fpm.gt / 5) % 2, 4, 2, MC_R); break;
        case 4: RECT(fpm.fx[k], fpm.fy[k], 4, 8, MC_W); break;
        case 7: RECT(fpm.fx[k], fpm.fy[k], 2, 1, MC_Y); break;
        default: RECT(fpm.fx[k], fpm.fy[k], 2, 2, MC_R); break;
        }
    }
    RECT(F[0], F[1] - 5, 3, 5, MC_W);
    PX(F[0] + 1 + I[0] * 2, F[1] - 4, MC_Y);
}

/* 46 STILL: the eyes up there open and shut; move only while they're shut */
/* ivar 0 eyes open, 1 timer, 2 the apple's x */
static void g46_start(void) { F[0] = 4; I[0] = 0; I[1] = 60; I[2] = 79; }
static void g46_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    if (fpm.gt >= 3600) { fpm_over(); return; }
    if (--I[1] <= 0) { I[0] = !I[0]; I[1] = I[0] ? RND(40, 120) : RND(30, 150); }
    int dx = DIRX(held);
    if (dx && I[0]) { fpm_over(); return; }
    F[0] = fclamp(F[0] + dx * 0.9f, 0, 82);
    if (fabsf(F[0] - I[2]) < 3) { fpm_add(10); sfx_play_name("fpm_point"); I[2] = I[2] > 40 ? 3 : 79; }
}
static void g46_draw(int ox, int oy) {
    for (int e = 0; e < 2; e++) {
        int x = 30 + e * 18;
        if (I[0]) { RECT(x, 2, 8, 5, MC_W); RECT(x + 3, 3, 3, 3, MC_R); }
        else RECT(x, 4, 8, 1, MC_W);
    }
    RECT(0, 30, 86, 2, MC_W);
    RECT(I[2], 25, 4, 4, MC_R);
    PX(I[2] + 2, 24, MC_Y);
    RECT(F[0], 23, 3, 7, MC_Y);
}

/* 47 BRIAR: saucers sow a hedge that creeps toward you; shoot the sowers and the seeds */
/* es 1 saucer (0..2), 2 a seed (3..29), 3 your shot (30..39); et the saucers' respawn */
static void g47_start(void) {
    F[0] = 6; F[1] = 14;
    for (int k = 0; k < 2; k++) { fpm.es[k] = 1; fpm.fx[k] = 80; fpm.fy[k] = 6.0f + k * 18; }
}
static void g47_update(uint8_t held, uint8_t pressed) {
    F[0] = fclamp(F[0] + DIRX(held) * 1.0f, 0, 83);
    F[1] = fclamp(F[1] + DIRY(held) * 1.0f, 0, 29);
    if (trigger(held, pressed, 7)) {
        int k = free_slot(30, 40);
        if (k >= 0) { fpm.es[k] = 3; fpm.fx[k] = F[0] + 3; fpm.fy[k] = F[1] + 1; }
    }
    for (int k = 0; k < 3; k++) {
        if (!fpm.es[k]) {
            if (fpm.et[k] > 0 && --fpm.et[k] == 0) { fpm.es[k] = 1; fpm.fx[k] = 82; fpm.fy[k] = (float)RND(2, 28); }
            continue;
        }
        if (fpm.gt % 6 == 0) {
            fpm.fx[k] = fclamp(fpm.fx[k] + RND(-2, 1), 30, 82);
            fpm.fy[k] = fclamp(fpm.fy[k] + RND(-2, 2), 0, 28);
        }
        if (fpm.gt % 12 == 0) {
            int s = free_slot(3, 30);
            if (s >= 0) { fpm.es[s] = 2; fpm.fx[s] = fpm.fx[k]; fpm.fy[s] = fpm.fy[k]; }
        }
        if (BOX(F[0], F[1], 3, 3, fpm.fx[k], fpm.fy[k], 5, 3)) { fpm_over(); return; }
    }
    if (fpm.gt % 900 == 0 && !fpm.es[2] && fpm.et[2] == 0) fpm.et[2] = 1;
    for (int k = 3; k < 30; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] -= 0.08f; /* the hedge creeps toward you */
        if (fpm.fx[k] < -3) { fpm.es[k] = 0; continue; }
        if (BOX(F[0], F[1], 3, 3, fpm.fx[k], fpm.fy[k], 3, 3)) { fpm_over(); return; }
    }
    for (int k = 30; k < 40; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] += 2.0f;
        if (fpm.fx[k] > 88) { fpm.es[k] = 0; continue; }
        for (int t = 0; t < 30; t++) {
            if (!fpm.es[t]) continue;
            int w = t < 3 ? 5 : 3;
            if (BOX(fpm.fx[k], fpm.fy[k], 2, 1, fpm.fx[t], fpm.fy[t], w, 3)) {
                fpm.es[k] = 0;
                fpm.es[t] = 0;
                if (t < 3) { fpm.et[t] = 240; sfx_play_name("fpm_point"); }
                break;
            }
        }
    }
    PER_SEC(2);
}
static void g47_draw(int ox, int oy) {
    for (int k = 3; k < 30; k++) if (fpm.es[k]) { RECT(fpm.fx[k], fpm.fy[k] + 1, 3, 1, MC_R); RECT(fpm.fx[k] + 1, fpm.fy[k], 1, 3, MC_R); }
    for (int k = 0; k < 3; k++) if (fpm.es[k]) { RECT(fpm.fx[k], fpm.fy[k] + 1, 5, 1, MC_W); RECT(fpm.fx[k] + 1, fpm.fy[k], 3, 3, MC_W); PX(fpm.fx[k] + 2, fpm.fy[k] + 1, MC_R); }
    for (int k = 30; k < 40; k++) if (fpm.es[k]) RECT(fpm.fx[k], fpm.fy[k], 2, 1, MC_Y);
    RECT(F[0], F[1], 3, 3, MC_Y);
}

/* 48 OGRE 3 */
static void g48_start(void) { fpm_boss_start(2); }

/* 49 WARPTOE */
static void g49_start(void) { fpm_mines_start(true); }

/* 50 FACES: make your face match the other one before the time runs out  */
/* ivar 0-3 yours (hair, eyes, nose, mouth), 4-7 theirs, 8 time left */
static void faces_new(void) {
    for (int k = 0; k < 4; k++) { I[4 + k] = RND(0, 5); I[k] = RND(0, 5); }
    if (I[0] == I[4] && I[1] == I[5] && I[2] == I[6] && I[3] == I[7]) I[0] = (I[4] + 1) % 6;
    I[8] = 600;
}
static void g50_start(void) { faces_new(); }
static void g50_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (pressed & MB_UP) I[0] = (I[0] + 1) % 6;
    if (pressed & MB_LEFT) I[1] = (I[1] + 1) % 6;
    if (pressed & MB_RIGHT) I[2] = (I[2] + 1) % 6;
    if (pressed & MB_DOWN) I[3] = (I[3] + 1) % 6;
    if (pressed & MB_FIRE) for (int k = 0; k < 4; k++) I[k] = RND(0, 5);
    if (pressed) sfx_play_name("fpm_blip");
    if (I[0] == I[4] && I[1] == I[5] && I[2] == I[6] && I[3] == I[7]) {
        fpm_add(10);
        sfx_play_name("fpm_point");
        faces_new();
        return;
    }
    if (--I[8] <= 0) fpm_over();
}
static void face(int ox, int oy, int x, const int *p) {
    RECT(x + 2, 6, 20, 22, MC_W);
    /* hair */
    switch (p[0]) {
    case 0: RECT(x + 2, 3, 20, 4, MC_Y); break;
    case 1: RECT(x + 4, 2, 16, 5, MC_R); break;
    case 2: for (int i = 0; i < 5; i++) RECT(x + 3 + i * 4, 3, 2, 4, MC_Y); break;
    case 3: RECT(x, 4, 24, 3, MC_R); RECT(x, 7, 3, 8, MC_R); RECT(x + 21, 7, 3, 8, MC_R); break;
    case 4: RECT(x + 9, 1, 6, 6, MC_Y); break;
    default: break;
    }
    /* eyes */
    for (int e = 0; e < 2; e++) {
        int ex = x + 6 + e * 9;
        switch (p[1]) {
        case 0: RECT(ex, 11, 3, 3, MC_K); break;
        case 1: RECT(ex, 12, 4, 1, MC_K); break;
        case 2: RECT(ex, 10, 4, 4, MC_K); PX(ex + 1, 11, MC_W); break;
        case 3: PX(ex + 1, 12, MC_K); break;
        case 4: RECT(ex - 1, 10, 6, 5, MC_R); RECT(ex + 1, 12, 2, 1, MC_K); break;
        default: PX(ex, 11, MC_K); PX(ex + 2, 13, MC_K); PX(ex + 1, 12, MC_K); break;
        }
    }
    /* nose */
    switch (p[2]) {
    case 0: RECT(x + 11, 15, 2, 4, MC_K); break;
    case 1: RECT(x + 10, 17, 4, 2, MC_R); break;
    case 2: PX(x + 11, 17, MC_K); PX(x + 13, 17, MC_K); break;
    case 3: RECT(x + 11, 14, 1, 5, MC_K); RECT(x + 11, 18, 3, 1, MC_K); break;
    case 4: RECT(x + 10, 16, 4, 3, MC_Y); break;
    default: break;
    }
    /* mouth */
    switch (p[3]) {
    case 0: RECT(x + 7, 23, 10, 1, MC_K); break;
    case 1: RECT(x + 8, 22, 8, 3, MC_R); break;
    case 2: RECT(x + 7, 22, 2, 1, MC_K); RECT(x + 9, 23, 6, 1, MC_K); RECT(x + 15, 22, 2, 1, MC_K); break;
    case 3: RECT(x + 10, 22, 4, 4, MC_K); break;
    case 4: RECT(x + 6, 24, 12, 1, MC_K); RECT(x + 6, 22, 1, 2, MC_K); RECT(x + 17, 22, 1, 2, MC_K); break;
    default: RECT(x + 7, 22, 10, 2, MC_Y); RECT(x + 4, 25, 16, 3, MC_Y); break;
    }
}
static void g50_draw(int ox, int oy) {
    face(ox, oy, 8, &I[0]);
    face(ox, oy, 54, &I[4]);
    fpm_text(ox, oy, "=", 41, 14, MC_Y);
    RECT(0, 31, I[8] * 86 / 600, 1, MC_R);
}

const FplMicroDef FPL_MICRO_B[25] = {
    {"PIVOT", g26_start, g26_update, g26_draw},
    {"LANES", g27_start, fpm_tilt_update, fpm_tilt_draw},
    {"ZAP", g28_start, g28_update, g28_draw},
    {"BOING", g29_start, g29_update, g29_draw},
    {"OGRE 2", g30_start, fpm_boss_update, fpm_boss_draw},
    {"RING", g31_start, g31_update, g31_draw},
    {"THAW", g32_start, g32_update, g32_draw},
    {"LAMP", g33_start, g33_update, g33_draw},
    {"SAYSO", g34_start, g34_update, g34_draw},
    {"TACK", g35_start, g35_update, g35_draw},
    {"PICK", g36_start, g36_update, g36_draw},
    {"WHICH?", g37_start, g37_update, g37_draw},
    {"DRAW", g38_start, g38_update, g38_draw},
    {"BOBBLE", g39_start, g39_update, g39_draw},
    {"PENNY", g40_start, g40_update, g40_draw},
    {"SHOVE", g41_start, g41_update, g41_draw},
    {"KEEPUP", g42_start, g42_update, g42_draw},
    {"WARP", g43_start, g43_update, g43_draw},
    {"THRONG", g44_start, g44_update, g44_draw},
    {"BLAST", g45_start, g45_update, g45_draw},
    {"STILL", g46_start, g46_update, g46_draw},
    {"BRIAR", g47_start, g47_update, g47_draw},
    {"OGRE 3", g48_start, fpm_boss_update, fpm_boss_draw},
    {"WARPTOE", g49_start, fpm_mines_update, fpm_mines_draw},
    {"FACES", g50_start, g50_update, g50_draw},
};
