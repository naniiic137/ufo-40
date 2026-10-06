/* FULL PEAL - Clary's console. A second controller holding A for two
 * seconds swaps the cockpit monitor for a menu of fifty micro-games, in
 * the monitor's four colours (black, white, yellow, red), whatever the
 * main game is doing: on a radio call, mid-wave, in a balloon round, even
 * on the game-over screen for as long as player 1 leaves it be. Each game
 * keeps its own high score (four digits, five past 9999); the menu shows
 * the total of all fifty.
 *
 * This file: the console itself, the engines more than one game shares,
 * and games 1-25. Games 26-50 are in fpl_micro2.c. Every game is ours:
 * our names, our pictures, built to the rules the originals are
 * described with (see the design document). */
#include "fpl.h"

FplMicro fpm;
static int rep_t, rep_dir;

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

/* ------------------------------------------------------------------ */
/* the console                                                          */

const FplMicroDef *fpl_micro_def(int game) {
    game = iclamp(game, 0, FPL_MICROS - 1);
    return game < 25 ? &FPL_MICRO_A[game] : &FPL_MICRO_B[game - 25];
}

void fpl_micro_reset(void) {
    memset(&fpm, 0, sizeof fpm);
    rep_t = 0;
}

bool fpl_micro_showing(void) { return fpm.state != MC_OFF; }

uint32_t fpl_micro_total(void) {
    uint32_t n = 0;
    for (int i = 0; i < FPL_MICROS; i++) n += fpsv.micro[i];
    return n;
}

int fpm_rand(int lo, int hi) { return rng_range(&fpm.rng, lo, hi); }

void fpm_add(uint32_t pts) {
    if (fpm.state != MC_PLAY) return;
    if (fpm.score + pts <= FPL_MICRO_MAX) fpm.score += pts;
    else {
        /* it stops at the highest it can show without going past 99999 */
        while (fpm.score + pts <= FPL_MICRO_MAX) fpm.score += pts;
    }
}

void fpm_over(void) {
    if (fpm.state != MC_PLAY) return;
    fpm.state = MC_OVER;
    fpm.over_t = 0;
    fpm.fresh_best = fpm.score > fpsv.micro[fpm.game];
    fpl_save_micro(fpm.game, fpm.score);
    sfx_play_name("fpm_over");
}

void fpl_micro_start(int game) {
    int g = iclamp(game, 0, FPL_MICROS - 1);
    int sel = fpm.sel;
    memset(fpm.ivar, 0, sizeof fpm.ivar);
    memset(fpm.fvar, 0, sizeof fpm.fvar);
    memset(fpm.grid, 0, sizeof fpm.grid);
    memset(fpm.ex, 0, sizeof fpm.ex);
    memset(fpm.ey, 0, sizeof fpm.ey);
    memset(fpm.es, 0, sizeof fpm.es);
    memset(fpm.et, 0, sizeof fpm.et);
    memset(fpm.fx, 0, sizeof fpm.fx);
    memset(fpm.fy, 0, sizeof fpm.fy);
    memset(fpm.fvx, 0, sizeof fpm.fvx);
    memset(fpm.fvy, 0, sizeof fpm.fvy);
    fpm.n = 0;
    fpm.game = g;
    fpm.sel = sel == g ? sel : g;
    fpm.score = 0;
    fpm.gt = 0;
    fpm.state = MC_PLAY;
    rng_seed(&fpm.rng, rng_next(&g_rng) ^ (uint64_t)(g * 7919));
    fpl_micro_def(g)->start();
}

static int menu_dir(uint8_t held, uint8_t pressed) {
    /* a direction pressed, or held (repeating) */
    int d = pressed & MB_DIRS;
    if (d) { rep_t = 0; rep_dir = d; return d; }
    if ((held & MB_DIRS) && (held & rep_dir)) {
        if (++rep_t > 18 && (rep_t - 18) % 5 == 0) return rep_dir;
    } else rep_t = 0;
    return 0;
}

void fpl_micro_update(uint8_t held, uint8_t pressed, bool allowed) {
    if (!allowed) {
        if (fpm.state != MC_OFF) fpl_micro_reset();
        fpm.hold = 0;
        return;
    }
    fpm.t++;
    switch (fpm.state) {
    case MC_OFF:
        if (held & MB_A) {
            if (++fpm.hold >= FPL_HOLD_OPEN) {
                fpm.state = MC_MENU;
                fpm.hold = 0;
                sfx_play_name("fpm_open");
            }
        } else fpm.hold = 0;
        break;
    case MC_MENU: {
        int d = menu_dir(held, pressed);
        if (d & MB_LEFT) fpm.sel = (fpm.sel + FPL_MICROS - 1) % FPL_MICROS;
        if (d & MB_RIGHT) fpm.sel = (fpm.sel + 1) % FPL_MICROS;
        if (d & MB_UP) fpm.sel = (fpm.sel + FPL_MICROS - 10) % FPL_MICROS;
        if (d & MB_DOWN) fpm.sel = (fpm.sel + 10) % FPL_MICROS;
        if (d) sfx_play_name("fpm_blip");
        if (pressed & MB_A) fpl_micro_start(fpm.sel);
        else if (pressed & MB_B) { fpm.state = MC_OFF; sfx_play_name("fpm_blip"); }
        break;
    }
    case MC_PLAY:
        fpm.gt++;
        fpl_micro_def(fpm.game)->update(held, pressed);
        break;
    case MC_OVER:
        fpm.over_t++;
        if (fpm.over_t > 20 && (pressed & MB_A)) fpl_micro_start(fpm.game);
        else if (fpm.over_t > 20 && (pressed & MB_B)) { fpm.state = MC_MENU; fpm.sel = fpm.game; }
        break;
    }
}

void fpm_px(int ox, int oy, int x, int y, int col) { gfx_pset(ox + x, oy + y, col); }
void fpm_rect(int ox, int oy, int x, int y, int w, int h, int col) { gfx_rect(ox + x, oy + y, w, h, col); }
void fpm_text(int ox, int oy, const char *s, int x, int y, int col) { tiny_draw(s, ox + x, oy + y, col); }

static void score_text(char *b, size_t n, uint32_t s) {
    if (s > 9999) snprintf(b, n, "%05u", (unsigned)s);
    else snprintf(b, n, "%04u", (unsigned)s);
}

void fpl_micro_draw(int x, int y) {
    char b[48], s[16];
    gfx_rect(x, y, FPL_MON_W, FPL_MON_H, MC_K);
    if (fpm.state == MC_MENU) {
        uint32_t tot = fpl_micro_total();
        snprintf(b, sizeof b, "TOTAL %06u", (unsigned)tot);
        tiny_center(b, x + FPL_MON_W / 2, y + 1, MC_Y);
        snprintf(b, sizeof b, "%02d %s", fpm.sel + 1, fpl_micro_def(fpm.sel)->name);
        text_center(b, x + FPL_MON_W / 2, y + 10, MC_W);
        if ((fpm.t / 15) % 2) {
            text_draw(GLYPH_LEFT, x + 1, y + 10, MC_R);
            text_draw(GLYPH_RIGHT, x + FPL_MON_W - 7, y + 10, MC_R);
        }
        score_text(s, sizeof s, fpsv.micro[fpm.sel]);
        snprintf(b, sizeof b, "HI %s", s);
        tiny_center(b, x + FPL_MON_W / 2, y + 21, MC_Y);
        tiny_center("A PLAY   B OFF", x + FPL_MON_W / 2, y + 30, MC_R);
        return;
    }
    if (fpm.state == MC_PLAY || fpm.state == MC_OVER) {
        tiny_draw(fpl_micro_def(fpm.game)->name, x + 1, y, MC_W);
        score_text(s, sizeof s, fpm.score);
        tiny_draw(s, x + FPL_MON_W - tiny_width(s) - 1, y, MC_Y);
        gfx_hline(x, x + FPL_MON_W - 1, y + FPM_TOP - 1, MC_R);
        gfx_clip(x, y + FPM_TOP, FPM_W, FPM_H);
        fpl_micro_def(fpm.game)->draw(x, y + FPM_TOP);
        gfx_clip(x, y, FPL_MON_W, FPL_MON_H);
        if (fpm.state == MC_OVER) {
            gfx_rect(x + 13, y + 10, 60, 22, MC_K);
            gfx_rectb(x + 13, y + 10, 60, 22, MC_R);
            tiny_center("GAME OVER", x + FPL_MON_W / 2, y + 13, MC_R);
            tiny_center(s, x + FPL_MON_W / 2, y + 20, MC_Y);
            if (fpm.fresh_best && (fpm.over_t / 10) % 2) tiny_center("BEST!", x + FPL_MON_W / 2, y + 26, MC_W);
        }
    }
}

/* ------------------------------------------------------------------ */
/* shared engines                                                       */

void fpm_chase(int *x, int *y, int tx, int ty) {
    int dx = isign(tx - *x), dy = isign(ty - *y);
    if (dx && dy) {
        if (fpm_rand(0, 1)) *x += dx;
        else *y += dy;
    } else {
        *x += dx;
        *y += dy;
    }
}

/* a ball meeting a paddle (centre px, width pw, top py): it goes back up,
 * angled by where it landed */
void fpm_paddle_ball(float *x, float *y, float *vx, float *vy, float px, int pw, int py) {
    if (*vy <= 0) return;
    if (*y + 2 < py || *y > py + 2) return;
    if (*x + 2 < px - pw / 2.0f || *x > px + pw / 2.0f) return;
    float off = ((*x + 1) - px) / (pw / 2.0f);
    float sp = sqrtf(*vx * *vx + *vy * *vy);
    if (sp < 0.9f) sp = 0.9f;
    *vx = off * sp * 0.8f;
    *vy = -sqrtf(fmaxf(sp * sp - *vx * *vx, 0.25f));
    sfx_play_name("fpm_blip");
}

/* ---- the OGRE games: a boss at the top, a ship below that shoots up ---- */
/* ivar: 0 kind, 1 core hp, 2-5 part hp, 6 fire timer, 7 laser x, 8 laser t,
 * 9 shot gap, 10 count-down; fvar: 0 x, 1 y, 2 boss x offset */
#define BOSS_SHOTS 10   /* fx/fy 0..9: the ship's shots (es 1) */
#define BOSS_BUL 20     /* 20..39: the boss's bullets (es 2) */
static const int OGRE_START[3] = {400, 700, 999};
static const int OGRE_CORE[3] = {24, 32, 40};

void fpm_boss_start(int kind) {
    I[0] = kind;
    I[1] = OGRE_CORE[kind];
    for (int k = 0; k < 4; k++) I[2 + k] = kind == 0 ? (k < 2 ? 10 : 6) : kind == 1 ? (k < 2 ? 8 : 0) : (k < 2 ? 12 : 0);
    I[6] = 60;
    I[8] = 0;
    F[0] = 43;
    F[1] = 27;
    F[2] = 0;
    fpm.score = (uint32_t)OGRE_START[kind];
}

/* where each part sits (x, y, w, h), relative to the boss's middle */
static void ogre_part(int k, int *x, int *y, int *w, int *h) {
    int kind = I[0], bx = 43 + (int)F[2];
    if (k < 0) { *x = bx - 5; *y = 1; *w = 10; *h = 7; return; } /* the core */
    if (kind == 0) {
        static const int PX_[4] = {-16, 8, -28, 24}, PY_[4] = {3, 3, 9, 9}, PW[4] = {8, 8, 4, 4}, PH[4] = {5, 5, 4, 4};
        *x = bx + PX_[k]; *y = PY_[k]; *w = PW[k]; *h = PH[k];
    } else if (kind == 1) {
        static const int PX_[4] = {-20, 14, -9, 3}, PY_[4] = {4, 4, 10, 10}, PW[4] = {6, 6, 6, 6}, PH[4] = {6, 6, 3, 3};
        *x = bx + PX_[k]; *y = PY_[k]; *w = PW[k]; *h = PH[k];
    } else {
        static const int PX_[4] = {-15, 10, 0, 0}, PY_[4] = {6, 6, 0, 0}, PW[4] = {5, 5, 0, 0}, PH[4] = {5, 5, 0, 0};
        *x = bx + PX_[k]; *y = PY_[k]; *w = PW[k]; *h = PH[k];
    }
}

static void boss_bullet(float x, float y, float vx, float vy) {
    for (int k = BOSS_BUL; k < BOSS_BUL + 20; k++)
        if (!fpm.es[k]) { fpm.es[k] = 2; fpm.fx[k] = x; fpm.fy[k] = y; fpm.fvx[k] = vx; fpm.fvy[k] = vy; fpm.et[k] = 0; return; }
}

static void aimed(float x, float y, float sp) {
    float dx = F[0] + 1 - x, dy = F[1] - y, d = sqrtf(dx * dx + dy * dy);
    if (d < 1) d = 1;
    boss_bullet(x, y, dx / d * sp, dy / d * sp);
}

void fpm_boss_update(uint8_t held, uint8_t pressed) {
    int kind = I[0];
    if (fpm.gt % 60 == 0 && fpm.score > 0) fpm.score--;
    F[0] = fclamp(F[0] + DIRX(held) * 0.9f, 0, 82);
    F[1] = fclamp(F[1] + DIRY(held) * 0.9f, 14, 29);
    /* shots: every press fires at once; held, it fires slower */
    if (I[9] > 0) I[9]--;
    if (((pressed & MB_FIRE) && I[9] <= 8) || ((held & MB_FIRE) && I[9] == 0)) {
        for (int k = 0; k < BOSS_SHOTS; k++)
            if (!fpm.es[k]) { fpm.es[k] = 1; fpm.fx[k] = F[0] + 1; fpm.fy[k] = F[1] - 1; break; }
        I[9] = 12;
    }
    if (kind == 2) F[2] = sinf(fpm.gt * 0.02f) * 22;
    for (int k = 0; k < BOSS_SHOTS; k++) {
        if (!fpm.es[k]) continue;
        fpm.fy[k] -= 2.2f;
        if (fpm.fy[k] < 0) { fpm.es[k] = 0; continue; }
        for (int p = -1; p < 4 && fpm.es[k]; p++) {
            int x, y, w, h;
            ogre_part(p, &x, &y, &w, &h);
            if (w == 0) continue;
            bool block = kind == 1 && p >= 2;     /* OGRE 2's blocks: nothing breaks them */
            if (p >= 0 && !block && I[2 + p] <= 0) continue;
            if (!BOX(fpm.fx[k], fpm.fy[k], 1, 2, x, y, w, h)) continue;
            fpm.es[k] = 0;
            if (block) continue;
            if (p < 0) { if (--I[1] <= 0) { sfx_play_name("fpm_point"); fpm_over(); return; } }
            else I[2 + p]--;
        }
    }
    /* the boss fires */
    if (--I[6] <= 0) {
        int bx = 43 + (int)F[2];
        if (kind == 0) {
            int arm = (fpm.gt / 70) % 2;
            if (I[2 + arm] > 0) boss_bullet((float)(bx + (arm ? 12 : -12)), 8, 0, 0.7f);
            for (int t = 2; t < 4; t++) if (I[2 + t] > 0 && fpm.gt % 140 < 70) aimed((float)(bx + (t == 2 ? -26 : 26)), 11, 0.45f);
            I[6] = 70;
        } else if (kind == 1) {
            for (int g = 0; g < 2; g++) if (I[2 + g] > 0) boss_bullet((float)(bx + (g ? 17 : -17)), 10, 0, 1.0f);
            if (fpm.gt % 120 < 40) aimed((float)bx, 8, 0.6f);
            I[6] = 40;
        } else {
            for (int g = 0; g < 2; g++) if (I[2 + g] > 0) aimed((float)(bx + (g ? 12 : -13)), 10, 0.7f);
            I[6] = 50;
        }
    }
    if (kind == 2) {
        /* the lasers: a warning line, then the beam */
        if (I[8] == 0 && fpm.gt % 90 == 45) { I[7] = (int)F[0] + 1; I[8] = 1; }
        if (I[8] > 0 && ++I[8] > 70) I[8] = 0;
        if (I[8] > 40 && I[7] >= F[0] - 1 && I[7] <= F[0] + 3) { fpm.score = 0; fpm_over(); return; }
    }
    /* homing for OGRE 1's turret shots: a gentle turn toward the ship */
    for (int k = BOSS_BUL; k < BOSS_BUL + 20; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] += fpm.fvx[k];
        fpm.fy[k] += fpm.fvy[k];
        if (fpm.fy[k] > 33 || fpm.fx[k] < -2 || fpm.fx[k] > 88) { fpm.es[k] = 0; continue; }
        if (BOX(fpm.fx[k], fpm.fy[k], 2, 2, F[0], F[1], 3, 3)) { fpm.score = 0; fpm_over(); return; }
    }
}

void fpm_boss_draw(int ox, int oy) {
    int kind = I[0];
    for (int p = 3; p >= -1; p--) {
        int x, y, w, h;
        ogre_part(p, &x, &y, &w, &h);
        if (w == 0) continue;
        bool block = kind == 1 && p >= 2;
        if (p >= 0 && !block && I[2 + p] <= 0) continue;
        RECT(x, y, w, h, p < 0 ? MC_R : block ? MC_W : MC_Y);
        if (p < 0) RECT(x + 3, y + 2, 4, 3, MC_Y);
    }
    if (kind == 2 && I[8] > 0) {
        if (I[8] > 40) RECT(I[7] - 1, 10, 3, 22, MC_R);
        else if ((I[8] / 4) % 2) RECT(I[7], 10, 1, 22, MC_R);
    }
    for (int k = 0; k < 40; k++) {
        if (fpm.es[k] == 1) RECT(fpm.fx[k], fpm.fy[k], 1, 2, MC_W);
        else if (fpm.es[k] == 2) RECT(fpm.fx[k], fpm.fy[k], 2, 2, MC_R);
    }
    RECT(F[0], F[1], 3, 3, MC_W);
    PX(F[0] + 1, F[1] - 1, MC_Y);
}

/* ---- TIPTOE and WARPTOE: a minefield shown for a second, then dark ---- */
#define MINE_W 21
#define MINE_H 8
static void mines_board(void) {
    bool warp = I[6];
    memset(fpm.grid, 0, sizeof fpm.grid);
    I[0] = 0;
    I[1] = RND(0, MINE_H - 1);
    I[2] = MINE_W - 1;
    I[3] = RND(0, MINE_H - 1);
    for (int c = 1; c < MINE_W - 1; c += 2) {
        int free_r = RND(0, MINE_H - 1);
        for (int r = 0; r < MINE_H; r++) G[c][r] = r != free_r && RND(0, 99) < (warp ? 30 : 45);
    }
    I[4] = 60; /* shown this long */
    I[5] = 0;
}

void fpm_mines_start(bool warp) {
    I[6] = warp;
    mines_board();
}

void fpm_mines_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (I[5]) return;
    if (I[4] > 0) { I[4]--; return; }
    int dx = DIRX(pressed), dy = DIRY(pressed);
    if (!dx && !dy) return;
    if (dx && dy) dy = 0;
    int step = I[6] ? RND(3, 4) : 1;
    I[0] = iclamp(I[0] + dx * step, 0, MINE_W - 1);
    I[1] = iclamp(I[1] + dy * step, 0, MINE_H - 1);
    sfx_play_name("fpm_blip");
    if (G[I[0]][I[1]]) { I[5] = 1; fpm_over(); return; }
    if (I[0] == I[2] && I[1] == I[3]) { fpm_add(10); sfx_play_name("fpm_point"); mines_board(); }
}

void fpm_mines_draw(int ox, int oy) {
    for (int c = 0; c < MINE_W; c++)
        for (int r = 0; r < MINE_H; r++)
            if (G[c][r] && (I[4] > 0 || I[5])) RECT(1 + c * 4 + 1, r * 4 + 1, 2, 2, MC_R);
    RECT(1 + I[2] * 4, I[3] * 4, 4, 4, MC_Y);
    RECT(1 + I[0] * 4, I[1] * 4, 4, 4, MC_W);
    if (I[4] > 0) RECT(1, 30, I[4] * 84 / 60, 1, MC_Y);
}

/* ---- SLOPE and LANES: steer by tilting; the world never slows ----------- */
/* ivar: 0 tilt (-3..3), 1 rows mode, 2 spawn timer, 3 tilt repeat; fvar 0 x */
void fpm_tilt_start(bool rows) {
    I[1] = rows;
    F[0] = 41;
    I[2] = 40;
}

static void tilt_spawn(void) {
    for (int k = 0; k < 40; k++) {
        if (fpm.es[k]) continue;
        if (I[1]) {
            /* a row of rocks with a gap, flashed at the top first */
            fpm.es[k] = 3;
            fpm.fx[k] = (float)RND(4, 66);   /* the gap's left edge */
            fpm.fy[k] = 0;
            fpm.et[k] = 30;                  /* the warning */
        } else {
            fpm.es[k] = RND(0, 9) < 6 ? 1 : 2;
            fpm.fx[k] = (float)RND(2, 82);
            fpm.fy[k] = 34;
        }
        return;
    }
}

void fpm_tilt_update(uint8_t held, uint8_t pressed) {
    int d = DIRX(pressed);
    if (DIRX(held)) { if (++I[3] > 12) { I[3] = 0; d = DIRX(held); } }
    else I[3] = 0;
    I[0] = iclamp(I[0] + d, -3, 3);
    F[0] = fclamp(F[0] + I[0] * 0.32f, 0, 83);
    float speed = 0.55f + fminf(fpm.gt / 3600.0f, 1.0f) * 0.4f;
    if (--I[2] <= 0) {
        tilt_spawn();
        I[2] = I[1] ? imax(45, 80 - fpm.gt / 240) : imax(14, 26 - fpm.gt / 600);
    }
    int py = I[1] ? 26 : 4;
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        if (fpm.es[k] == 3) {
            if (fpm.et[k] > 0) { fpm.et[k]--; continue; }
            fpm.fy[k] += 2.0f;
            if (fpm.fy[k] > 34) { fpm.es[k] = 0; continue; }
            if (fpm.fy[k] + 3 >= py && fpm.fy[k] <= py + 3 && (F[0] < fpm.fx[k] || F[0] + 3 > fpm.fx[k] + 16)) { fpm_over(); return; }
            continue;
        }
        /* the slope scrolls up past the skater; the faster across, the faster overall */
        fpm.fy[k] -= speed + fabsf((float)I[0]) * 0.05f;
        if (fpm.fy[k] < -4) { fpm.es[k] = 0; continue; }
        if (BOX(F[0], py, 3, 3, fpm.fx[k], fpm.fy[k], 3, 3)) {
            if (fpm.es[k] == 1) { fpm.es[k] = 0; fpm_add(10); sfx_play_name("fpm_point"); }
            else { fpm_over(); return; }
        }
    }
    if (I[1]) PER_SEC(1);
}

void fpm_tilt_draw(int ox, int oy) {
    int py = I[1] ? 26 : 4;
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        if (fpm.es[k] == 3) {
            if (fpm.et[k] > 0 && (fpm.et[k] / 4) % 2) continue;
            RECT(0, fpm.fy[k], (int)fpm.fx[k], 3, MC_R);
            RECT(fpm.fx[k] + 16, fpm.fy[k], 86, 3, MC_R);
            continue;
        }
        RECT(fpm.fx[k], fpm.fy[k], 3, 3, fpm.es[k] == 1 ? MC_Y : MC_R);
    }
    RECT(F[0], py, 3, 3, MC_W);
    PX(F[0] + 1 + I[0] / 2, py + (I[1] ? -1 : 3), MC_W);
}

/* ------------------------------------------------------------------ */
/* 01 SHOO: run round a grid while chasers pour in from the corners    */
#define SG_W 21
#define SG_H 8
static void g01_start(void) {
    I[0] = 10; I[1] = 4; I[2] = 1; I[3] = 0; I[4] = 0; I[5] = 50;
    fpm.n = 0;
}
static void g01_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    if (DIRX(held)) { I[2] = DIRX(held); I[3] = 0; }
    else if (DIRY(held)) { I[2] = 0; I[3] = DIRY(held); }
    if (fpm.gt % 8 == 0) {
        I[0] = iclamp(I[0] + I[2], 0, SG_W - 1);
        I[1] = iclamp(I[1] + I[3], 0, SG_H - 1);
    }
    if (--I[5] <= 0 && fpm.n < 40) {
        static const int CX[4] = {0, SG_W - 1, SG_W - 1, 0}, CY[4] = {0, 0, SG_H - 1, SG_H - 1};
        fpm.ex[fpm.n] = CX[I[4] % 4];
        fpm.ey[fpm.n] = CY[I[4] % 4];
        fpm.n++;
        I[4]++;
        I[5] = imax(45, 110 - fpm.gt / 90);
    }
    if (fpm.gt % 16 == 0)
        for (int k = 0; k < fpm.n; k++) fpm_chase(&fpm.ex[k], &fpm.ey[k], I[0], I[1]);
    for (int k = 0; k < fpm.n; k++)
        if (fpm.ex[k] == I[0] && fpm.ey[k] == I[1]) { fpm_over(); return; }
    PER_SEC(10);
}
static void g01_draw(int ox, int oy) {
    for (int k = 0; k < fpm.n; k++) RECT(1 + fpm.ex[k] * 4, fpm.ey[k] * 4, 4, 4, MC_R);
    RECT(1 + I[0] * 4, I[1] * 4, 4, 4, MC_W);
    PX(1 + I[0] * 4 + 1 + I[2], I[1] * 4 + 1 + I[3], MC_K);
}

/* 02 KILN: break the tiles; the red ones spit back; they creep down     */
/* grid[c][r]: 0 none, 1 red, 2 yellow, 3 yellow cracked; ivar 0 creep px,
 * 1 creep timer, 2 serve timer; fvar 0 paddle x, 1-4 ball */
static void g02_start(void) {
    for (int c = 0; c < 10; c++)
        for (int r = 0; r < 4; r++) G[c][r] = RND(0, 5) == 0 ? 2 : 1;
    F[0] = 43;
    I[2] = 30;
}
static int kiln_y(int r) { return 1 + r * 4 + I[0]; }
static void g02_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    F[0] = fclamp(F[0] + DIRX(held) * 1.4f, 6, 80);
    if (++I[1] >= 300) {
        I[1] = 0;
        if (++I[0] >= 4) {
            /* a new row at the top */
            I[0] = 0;
            for (int c = 0; c < 10; c++) {
                for (int r = 11; r > 0; r--) G[c][r] = G[c][r - 1];
                G[c][0] = RND(0, 5) == 0 ? 2 : 1;
            }
        }
    }
    for (int c = 0; c < 10; c++)
        for (int r = 0; r < 12; r++)
            if (G[c][r] && kiln_y(r) + 3 >= 28) { fpm_over(); return; }
    if (I[2] > 0) {
        if (--I[2] == 0) { F[1] = F[0]; F[2] = 26; F[3] = 0.6f; F[4] = -0.9f; }
        return;
    }
    F[1] += F[3];
    F[2] += F[4];
    if (F[1] < 0) { F[1] = 0; F[3] = fabsf(F[3]); }
    if (F[1] > 84) { F[1] = 84; F[3] = -fabsf(F[3]); }
    if (F[2] < 0) { F[2] = 0; F[4] = fabsf(F[4]); }
    fpm_paddle_ball(&F[1], &F[2], &F[3], &F[4], F[0], 12, 28);
    if (F[2] > 32) { fpm_over(); return; }
    int c = ((int)F[1] + 1 - 3) / 8;
    if (c >= 0 && c < 10)
        for (int r = 0; r < 12; r++) {
            int y = kiln_y(r);
            if (!G[c][r] || F[2] + 1 < y || F[2] + 1 > y + 3) continue;
            F[4] = -F[4];
            if (G[c][r] == 2) G[c][r] = 3;
            else {
                if (G[c][r] == 1) {
                    /* a red tile spits a pellet at the paddle */
                    for (int k = 0; k < 10; k++)
                        if (!fpm.es[k]) {
                            float dx = F[0] - (3 + c * 8 + 4), dy = 28.0f - y, d = sqrtf(dx * dx + dy * dy);
                            fpm.es[k] = 1; fpm.fx[k] = 3.0f + c * 8 + 3; fpm.fy[k] = (float)y + 2;
                            fpm.fvx[k] = dx / d * 0.6f; fpm.fvy[k] = dy / d * 0.6f;
                            break;
                        }
                }
                G[c][r] = 0;
                fpm_add(2);
            }
            sfx_play_name("fpm_blip");
            break;
        }
    for (int k = 0; k < 10; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] += fpm.fvx[k];
        fpm.fy[k] += fpm.fvy[k];
        if (fpm.fy[k] > 33) fpm.es[k] = 0;
        else if (BOX(fpm.fx[k], fpm.fy[k], 2, 2, F[0] - 6, 28, 12, 2)) { fpm_over(); return; }
    }
}
static void g02_draw(int ox, int oy) {
    for (int c = 0; c < 10; c++)
        for (int r = 0; r < 12; r++)
            if (G[c][r]) RECT(3 + c * 8, kiln_y(r), 7, 3, G[c][r] == 1 ? MC_R : G[c][r] == 2 ? MC_Y : MC_W);
    for (int k = 0; k < 10; k++) if (fpm.es[k]) RECT(fpm.fx[k], fpm.fy[k], 2, 2, MC_R);
    RECT(F[0] - 6, 28, 12, 2, MC_W);
    if (I[2] > 0) RECT(F[0] - 1, 26, 2, 2, MC_Y);
    else RECT(F[1], F[2], 2, 2, MC_Y);
}

/* 03 FLAP: keep a bird in the air; snap up the gold, dodge the red      */
/* es 1 gold, 2 red; ivar 0 gold taken, 1 gold timer, 2 red timer */
static void g03_start(void) { F[0] = 20; F[1] = 12; F[2] = 0; I[1] = 40; I[2] = 80; }
static void g03_spawn(int kind, int side) {
    for (int k = 0; k < 40; k++)
        if (!fpm.es[k]) {
            fpm.es[k] = kind;
            fpm.fx[k] = side > 0 ? 88.0f : -4.0f;
            fpm.fy[k] = (float)RND(1, 27);
            fpm.fvx[k] = (kind == 1 ? -0.7f : 0.9f * -side) * (side > 0 ? 1 : -1) * (kind == 1 ? 1 : 1);
            if (kind == 2) fpm.fvx[k] = side > 0 ? -0.9f : 0.9f;
            return;
        }
}
static void g03_update(uint8_t held, uint8_t pressed) {
    F[0] = fclamp(F[0] + DIRX(held) * 0.8f, 0, 82);
    F[2] += 0.06f;
    if (pressed & MB_FIRE) { F[2] = -1.1f; sfx_play_name("fpm_blip"); }
    F[1] += F[2];
    if (F[1] < 0) { F[1] = 0; F[2] = 0; }
    if (F[1] > 29) { fpm_over(); return; }
    bool flood = I[0] >= 100;
    if (--I[1] <= 0) {
        int gold = 0;
        for (int k = 0; k < 40; k++) gold += fpm.es[k] == 1;
        if (gold < (flood ? 4 : 2)) g03_spawn(1, 1);
        I[1] = flood ? 35 : 70;
    }
    if (--I[2] <= 0) { g03_spawn(2, RND(0, 1) ? 1 : -1); I[2] = flood ? 22 : imax(40, 90 - I[0]); }
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] += fpm.fvx[k];
        if (fpm.fx[k] < -6 || fpm.fx[k] > 90) { fpm.es[k] = 0; continue; }
        if (BOX(F[0], F[1], 4, 3, fpm.fx[k], fpm.fy[k], 3, 3)) {
            if (fpm.es[k] == 1) { fpm.es[k] = 0; I[0]++; fpm_add(5); sfx_play_name("fpm_point"); }
            else { fpm_over(); return; }
        }
    }
}
static void g03_draw(int ox, int oy) {
    for (int k = 0; k < 40; k++) if (fpm.es[k]) RECT(fpm.fx[k], fpm.fy[k], 3, 3, fpm.es[k] == 1 ? MC_Y : MC_R);
    RECT(F[0], F[1] + 1, 4, 2, MC_W);
    PX(F[0] + ((fpm.gt / 6) % 2 ? 1 : 2), F[1] + ((fpm.gt / 6) % 2 ? 0 : 3), MC_W);
    PX(F[0] + 4, F[1] + 1, MC_Y);
}

/* 04 PEEK: find the face; every miss shows an arrow toward it           */
/* grid: 0 hidden, 1-8 an arrow, 9 the face; ivar 0,1 cursor, 2,3 face,
 * 4 arrows shown in all, 5 next-board timer */
#define PK_W 8
#define PK_H 3
static void peek_board(void) {
    memset(fpm.grid, 0, sizeof fpm.grid);
    I[2] = RND(0, PK_W - 1);
    I[3] = RND(0, PK_H - 1);
    I[5] = 0;
}
static void g04_start(void) { I[0] = 3; I[1] = 1; I[4] = 0; peek_board(); }
static int dir8(int dx, int dy) {
    /* 1 up, 2 up-right, 3 right, 4 down-right, 5 down, 6 down-left, 7 left, 8 up-left */
    static const int T[3][3] = {{8, 7, 6}, {1, 0, 5}, {2, 3, 4}};
    return T[isign(dx) + 1][isign(dy) + 1];
}
static void g04_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (pressed & MB_B) { fpm_over(); return; }
    if (I[5] > 0) { if (--I[5] == 0) peek_board(); return; }
    I[0] = iclamp(I[0] + DIRX(pressed), 0, PK_W - 1);
    I[1] = iclamp(I[1] + DIRY(pressed), 0, PK_H - 1);
    if (!(pressed & MB_A) || G[I[0]][I[1]]) return;
    if (I[0] == I[2] && I[1] == I[3]) {
        G[I[0]][I[1]] = 9;
        fpm_add(10);
        sfx_play_name("fpm_point");
        I[5] = 30;
        return;
    }
    G[I[0]][I[1]] = (int8_t)dir8(I[2] - I[0], I[3] - I[1]);
    sfx_play_name("fpm_blip");
    if (++I[4] >= 50) fpm_over();
}
static void g04_draw(int ox, int oy) {
    static const int AX[9] = {0, 0, 1, 1, 1, 0, -1, -1, -1}, AY[9] = {0, -1, -1, 0, 1, 1, 1, 0, -1};
    for (int c = 0; c < PK_W; c++)
        for (int r = 0; r < PK_H; r++) {
            int x = 3 + c * 10, y = 1 + r * 10, v = G[c][r];
            if (!v) { RECT(x, y, 9, 9, MC_W); RECT(x + 1, y + 1, 7, 7, MC_K); continue; }
            if (v == 9) {
                RECT(x + 1, y + 1, 7, 7, MC_Y);
                PX(x + 3, y + 3, MC_K); PX(x + 5, y + 3, MC_K);
                RECT(x + 3, y + 6, 3, 1, MC_K);
                continue;
            }
            int cx = x + 4, cy = y + 4;
            for (int i = -2; i <= 2; i++) PX(cx + AX[v] * i, cy + AY[v] * i, MC_Y);
            PX(cx + AX[v] * 2 - AY[v], cy + AY[v] * 2 + AX[v], MC_Y);
        }
    int x = 3 + I[0] * 10, y = 1 + I[1] * 10;
    if ((fpm.gt / 8) % 2) { RECT(x - 1, y - 1, 11, 1, MC_R); RECT(x - 1, y + 9, 11, 1, MC_R); RECT(x - 1, y - 1, 1, 11, MC_R); RECT(x + 9, y - 1, 1, 11, MC_R); }
}

/* 05 PIKE and 23 PIKE 2: run the grid with a pike held out in front     */
/* ivar 0,1 you, 2,3 facing, 4 spawn side, 5 spawn timer, 6 second game;
 * grid 1: a block (PIKE 2) */
static void pike_start(bool two) {
    I[0] = 10; I[1] = 4; I[2] = 1; I[3] = 0; I[4] = 0; I[5] = 40; I[6] = two;
    fpm.n = 0;
    if (two)
        for (int c = 8; c <= 12; c++)
            for (int r = 3; r <= 4; r++) G[c][r] = 1;
    if (two) { I[0] = 3; I[1] = 4; }
}
static void g05_start(void) { pike_start(false); }
static void g23_start(void) { pike_start(true); }
static void pike_kill_at(int x, int y) {
    for (int k = 0; k < fpm.n; k++)
        if (fpm.es[k] == 0 && fpm.ex[k] == x && fpm.ey[k] == y) {
            fpm.es[k] = 1;
            fpm_add(I[6] ? 5 : 10);
            sfx_play_name("fpm_point");
        }
}
static void pike_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    if (DIRX(held)) { I[2] = DIRX(held); I[3] = 0; }
    else if (DIRY(held)) { I[2] = 0; I[3] = DIRY(held); }
    if (fpm.gt % 9 == 0) {
        int nx = I[0] + I[2], ny = I[1] + I[3];
        if (nx >= 0 && nx < SG_W && ny >= 0 && ny < SG_H && !G[nx][ny]) { I[0] = nx; I[1] = ny; }
    }
    int sx = I[0] + I[2], sy = I[1] + I[3];
    if (I[6] && sx >= 0 && sx < SG_W && sy >= 0 && sy < SG_H && G[sx][sy]) {
        /* the pike breaks a block */
        G[sx][sy] = 0;
        fpm_add(5);
        sfx_play_name("fpm_blip");
    }
    if (--I[5] <= 0) {
        /* in at the top cross and the bottom cross by turns */
        int slot = -1;
        for (int k = 0; k < 40; k++) if (k >= fpm.n || fpm.es[k]) { slot = k; break; }
        if (slot >= 0) {
            if (slot >= fpm.n) fpm.n = slot + 1;
            fpm.es[slot] = 0;
            fpm.ex[slot] = SG_W / 2;
            fpm.ey[slot] = I[4] % 2 ? SG_H - 1 : 0;
            I[4]++;
        }
        I[5] = I[6] ? 45 : 60;
    }
    int every = I[6] ? 10 : 14;
    if (fpm.gt % every == 0)
        for (int k = 0; k < fpm.n; k++) {
            if (fpm.es[k]) continue;
            int x = fpm.ex[k], y = fpm.ey[k];
            fpm_chase(&x, &y, I[0], I[1]);
            if (x >= 0 && x < SG_W && y >= 0 && y < SG_H && !G[x][y]) { fpm.ex[k] = x; fpm.ey[k] = y; }
        }
    pike_kill_at(sx, sy);
    for (int k = 0; k < fpm.n; k++)
        if (!fpm.es[k] && fpm.ex[k] == I[0] && fpm.ey[k] == I[1]) { fpm_over(); return; }
}
static void pike_draw(int ox, int oy) {
    for (int c = 0; c < SG_W; c++)
        for (int r = 0; r < SG_H; r++)
            if (G[c][r]) RECT(1 + c * 4, r * 4, 4, 4, MC_Y);
    /* the two crosses they come in at */
    for (int s = 0; s < 2; s++) {
        int x = 1 + (SG_W / 2) * 4 + 1, y = s ? (SG_H - 1) * 4 + 1 : 1;
        PX(x, y, MC_R); PX(x + 1, y + 1, MC_R); PX(x - 1, y + 1, MC_R); PX(x, y + 2, MC_R); PX(x, y + 1, MC_R);
    }
    for (int k = 0; k < fpm.n; k++) if (!fpm.es[k]) RECT(1 + fpm.ex[k] * 4, fpm.ey[k] * 4, 4, 4, MC_R);
    RECT(1 + I[0] * 4, I[1] * 4, 4, 4, MC_W);
    int sx = 1 + (I[0] + I[2]) * 4, sy = (I[1] + I[3]) * 4;
    if (I[2]) RECT(sx, sy + 1, 4, 2, MC_Y);
    else RECT(sx + 1, sy, 2, 4, MC_Y);
}

/* 06 TIPTOE */
static void g06_start(void) { fpm_mines_start(false); }

/* 07 ANGLER: drift the hook down to a fish; what bites decides the score */
/* es: species 1 sprat, 2 cod, 3 angelfish, 4 pike, 5 whale; ivar 0 fish seen, 1 timer */
static const int FISH_W[6] = {0, 3, 6, 5, 9, 16}, FISH_H[6] = {0, 2, 3, 5, 2, 6};
static void g07_start(void) { F[0] = 43; F[1] = 4; I[1] = 20; }
static void g07_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    F[0] = fclamp(F[0] + DIRX(held) * 0.7f, 0, 84);
    F[1] = fclamp(F[1] + DIRY(held) * 0.7f, 2, 30);
    if (--I[1] <= 0) {
        for (int k = 0; k < 12; k++) {
            if (fpm.es[k]) continue;
            int sp = RND(1, 100);
            int s = sp <= 40 ? 1 : sp <= 70 ? 2 : sp <= 88 ? 3 : 4;
            if (I[0] >= 30 && RND(0, 9) == 0) s = 5;
            int side = RND(0, 1);
            fpm.es[k] = s;
            fpm.fx[k] = side ? 88.0f : (float)-FISH_W[s];
            fpm.fy[k] = (float)RND(6 + s * 2, 30 - FISH_H[s]);
            float v = s == 1 ? 0.7f : s == 2 ? 0.45f : s == 3 ? 0.3f : s == 4 ? 0.9f : 0.2f;
            fpm.fvx[k] = side ? -v : v;
            I[0]++;
            break;
        }
        I[1] = RND(30, 70);
    }
    for (int k = 0; k < 12; k++) {
        int s = fpm.es[k];
        if (!s) continue;
        fpm.fx[k] += fpm.fvx[k];
        if (fpm.fx[k] < -18 || fpm.fx[k] > 90) { fpm.es[k] = 0; continue; }
        if (BOX(F[0], F[1], 2, 2, fpm.fx[k], fpm.fy[k], FISH_W[s], FISH_H[s])) {
            static const int LO[6] = {0, 5, 20, 40, 80, 300}, HI[6] = {0, 20, 60, 120, 250, 600};
            fpm.score = (uint32_t)RND(LO[s], HI[s]);
            sfx_play_name("fpm_point");
            fpm_over();
            return;
        }
    }
}
static void g07_draw(int ox, int oy) {
    RECT(0, 0, 86, 1, MC_W);
    RECT(F[0], 0, 1, (int)F[1], MC_W);
    RECT(F[0], F[1], 2, 2, MC_Y);
    for (int k = 0; k < 12; k++) {
        int s = fpm.es[k];
        if (!s) continue;
        RECT(fpm.fx[k], fpm.fy[k], FISH_W[s], FISH_H[s], s == 3 || s == 4 ? MC_Y : MC_W);
        if (s == 5) RECT(fpm.fx[k] + (fpm.fvx[k] > 0 ? 12 : 2), fpm.fy[k] + 1, 2, 2, MC_R);
        int tail = fpm.fvx[k] > 0 ? (int)fpm.fx[k] - 1 : (int)fpm.fx[k] + FISH_W[s];
        RECT(tail, fpm.fy[k], 1, FISH_H[s], MC_R);
    }
}

/* 08 BURROW: a tunnel scrolling down at you, narrower and faster        */
/* ex/ey: each row's walls (left, right), es: a rock's x (0 = none); 18 rows of 2 px */
#define BR_ROWS 18
static void burrow_row(int k, int prev) {
    int mid = (fpm.ex[prev] + fpm.ey[prev]) / 2 + RND(-3, 3);
    int w = imax(12, 30 - fpm.gt / 500);
    mid = iclamp(mid, w / 2 + 1, 85 - w / 2);
    fpm.ex[k] = mid - w / 2;
    fpm.ey[k] = mid + w / 2;
    fpm.es[k] = RND(0, 11) == 0 ? RND(fpm.ex[k] + 2, fpm.ey[k] - 4) : 0;
}
static void g08_start(void) {
    for (int k = 0; k < BR_ROWS; k++) { fpm.ex[k] = 28; fpm.ey[k] = 58; fpm.es[k] = 0; }
    F[0] = 42;
    F[1] = 0;
}
static void g08_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    F[0] = fclamp(F[0] + DIRX(held) * 1.0f, 0, 83);
    F[1] += fminf(0.35f + fpm.gt / 3600.0f * 0.15f, 1.2f);
    while (F[1] >= 2) {
        F[1] -= 2;
        for (int k = BR_ROWS - 1; k > 0; k--) { fpm.ex[k] = fpm.ex[k - 1]; fpm.ey[k] = fpm.ey[k - 1]; fpm.es[k] = fpm.es[k - 1]; }
        burrow_row(0, 1);
    }
    /* the rows the digger (y 26..28) is in */
    for (int k = 0; k < BR_ROWS; k++) {
        int y = (k - 1) * 2 + (int)F[1];
        if (y + 2 <= 26 || y > 28) continue;
        if (F[0] < fpm.ex[k] || F[0] + 3 > fpm.ey[k]) { fpm_over(); return; }
        if (fpm.es[k] && BOX(F[0], 26, 3, 3, fpm.es[k], y, 3, 2)) { fpm_over(); return; }
    }
    PER_SEC(2);
}
static void g08_draw(int ox, int oy) {
    for (int k = 0; k < BR_ROWS; k++) {
        int y = (k - 1) * 2 + (int)F[1];
        RECT(0, y, fpm.ex[k], 2, MC_R);
        RECT(fpm.ey[k], y, 86 - fpm.ey[k], 2, MC_R);
        if (fpm.es[k]) RECT(fpm.es[k], y, 3, 2, MC_Y);
    }
    RECT(F[0], 26, 3, 3, MC_W);
}

/* 09 SLOPE */
static void g09_start(void) { fpm_tilt_start(false); }

/* 10 OGRE 1 */
static void g10_start(void) { fpm_boss_start(0); }

/* 11 SNIPE: pick off the red bird between two gold ships                 */
/* fvar 0 you; es 1 your shot, 2 their shot; ivar 0 bird x, 1 bird y, 2,3 the ships' x */
static void g11_start(void) { F[0] = 70; I[0] = 80; I[1] = 10; I[2] = 22; I[3] = 58; }
static void snipe_fire(float x, float y, float vy, int kind) {
    for (int k = 0; k < 40; k++)
        if (!fpm.es[k]) { fpm.es[k] = kind; fpm.fx[k] = x; fpm.fy[k] = y; fpm.fvy[k] = vy; return; }
}
static void g11_update(uint8_t held, uint8_t pressed) {
    F[0] = fclamp(F[0] + DIRX(held) * 1.0f, 0, 81);
    if (pressed & MB_FIRE) snipe_fire(F[0] + 2, 26, -1.6f, 1);
    /* the bird flits along its line; the ships drift */
    if (fpm.gt % 4 == 0) I[0] = iclamp(I[0] + RND(-3, 3), 4, 80);
    if (fpm.gt % 30 == 0) I[1] = iclamp(I[1] + RND(-1, 1), 8, 14);
    I[2] = 22 + (int)(sinf(fpm.gt * 0.02f) * 8);
    I[3] = 58 + (int)(sinf(fpm.gt * 0.02f + 2) * 8);
    if (fpm.gt % 70 == 35) snipe_fire((float)I[RND(2, 3)] + 2, 7, 0.8f, 2);
    if (fpm.gt % 55 == 0) snipe_fire((float)I[0] + 1, (float)I[1] + 2, 0.9f, 2);
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        fpm.fy[k] += fpm.fvy[k];
        if (fpm.fy[k] < -2 || fpm.fy[k] > 33) { fpm.es[k] = 0; continue; }
        if (fpm.es[k] == 1) {
            if (BOX(fpm.fx[k], fpm.fy[k], 1, 2, I[0], I[1], 4, 3)) {
                fpm.es[k] = 0; fpm_add(5); sfx_play_name("fpm_point");
                I[0] = 82; /* a new bird, in from the right */
            } else if (BOX(fpm.fx[k], fpm.fy[k], 1, 2, I[2], 4, 6, 3) || BOX(fpm.fx[k], fpm.fy[k], 1, 2, I[3], 4, 6, 3)) {
                fpm_over(); return;
            }
        } else if (BOX(fpm.fx[k], fpm.fy[k], 1, 2, F[0], 28, 5, 3)) { fpm_over(); return; }
    }
}
static void g11_draw(int ox, int oy) {
    RECT(I[2], 4, 6, 3, MC_Y);
    RECT(I[3], 4, 6, 3, MC_Y);
    RECT(I[0], I[1] + ((fpm.gt / 5) % 2), 4, 2, MC_R);
    PX(I[0] + ((fpm.gt / 5) % 2 ? 0 : 3), I[1] + 2, MC_R);
    for (int k = 0; k < 40; k++) if (fpm.es[k]) RECT(fpm.fx[k], fpm.fy[k], 1, 2, fpm.es[k] == 1 ? MC_W : MC_R);
    RECT(F[0], 28, 5, 3, MC_W);
    PX(F[0] + 2, 27, MC_W);
}

/* 12 DRIP: tap just as the drop reaches your fingertip                   */
/* ivar 0 stage (0-6 swelling, 7 falling), 1 timer, 2 success pause; fvar 0 drop y */
static void drip_new(void) { I[0] = 0; I[1] = RND(8, 40); F[0] = 3; }
static void g12_start(void) { drip_new(); }
static void g12_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (I[2] > 0) { if (--I[2] == 0) drip_new(); return; }
    if (I[0] < 7) {
        if (--I[1] <= 0) { I[0]++; I[1] = RND(6, 36); }
    } else {
        F[0] += 1.1f;
    }
    bool in_window = I[0] == 7 && F[0] >= 20 && F[0] <= 25;
    if (pressed & MB_FIRE) {
        if (in_window) { fpm_add(10); sfx_play_name("fpm_point"); I[2] = 25; I[0] = 8; }
        else { fpm_over(); return; }
    }
    if (I[0] == 7 && F[0] > 26) fpm_over();
}
static void g12_draw(int ox, int oy) {
    RECT(0, 0, 86, 2, MC_W);
    if (I[0] < 7) RECT(42 - I[0] / 3, 2, 2 + (I[0] / 3) * 2, 1 + I[0] / 2, MC_Y);
    else if (I[0] == 7) RECT(42, F[0], 2, 3, MC_Y);
    else RECT(39, 21, 8, 1, MC_Y);
    /* the finger pointing up */
    RECT(42, 24, 2, 8, MC_W);
    RECT(40, 28, 6, 4, MC_W);
    PX(42, 24, MC_R);
}

/* 13 SHOO 2: move freely; bouncing chasers from two corners, gold from the others */
static void g13_start(void) { F[0] = 42; F[1] = 14; I[0] = 30; I[1] = 60; I[2] = 0; I[3] = 0; }
static void g13_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    F[0] = fclamp(F[0] + DIRX(held) * 1.0f, 0, 83);
    F[1] = fclamp(F[1] + DIRY(held) * 1.0f, 0, 29);
    if (--I[0] <= 0) {
        for (int k = 0; k < 20; k++)
            if (!fpm.es[k]) {
                bool tl = I[2]++ % 2 == 0;
                float sp = 0.5f + RND(0, 7) * 0.1f;
                fpm.es[k] = 1;
                fpm.fx[k] = tl ? 0.0f : 83.0f;
                fpm.fy[k] = tl ? 0.0f : 29.0f;
                fpm.fvx[k] = tl ? sp : -sp;
                fpm.fvy[k] = (tl ? 1 : -1) * sp * (0.6f + RND(0, 4) * 0.1f);
                break;
            }
        I[0] = imax(70, 140 - fpm.gt / 120);
    }
    if (--I[1] <= 0) {
        for (int k = 20; k < 24; k++)
            if (!fpm.es[k]) {
                bool tr = I[3]++ % 2 == 0;
                fpm.es[k] = 2;
                fpm.fx[k] = tr ? 82.0f : 1.0f;
                fpm.fy[k] = tr ? 1.0f : 28.0f;
                break;
            }
        I[1] = 150;
    }
    for (int k = 0; k < 24; k++) {
        if (!fpm.es[k]) continue;
        if (fpm.es[k] == 1) {
            fpm.fx[k] += fpm.fvx[k];
            fpm.fy[k] += fpm.fvy[k];
            if (fpm.fx[k] < 0 || fpm.fx[k] > 83) fpm.fvx[k] = -fpm.fvx[k];
            if (fpm.fy[k] < 0 || fpm.fy[k] > 29) fpm.fvy[k] = -fpm.fvy[k];
            if (BOX(F[0], F[1], 3, 3, fpm.fx[k], fpm.fy[k], 3, 3)) { fpm_over(); return; }
        } else if (BOX(F[0], F[1], 3, 3, fpm.fx[k], fpm.fy[k], 3, 3)) {
            fpm.es[k] = 0; fpm_add(5); sfx_play_name("fpm_point");
        }
    }
}
static void g13_draw(int ox, int oy) {
    for (int k = 0; k < 24; k++) if (fpm.es[k]) RECT(fpm.fx[k], fpm.fy[k], 3, 3, fpm.es[k] == 1 ? MC_R : MC_Y);
    RECT(F[0], F[1], 3, 3, MC_W);
}

/* 14 LOOPY: always moving; LEFT turns one way, RIGHT the other; every
 * prize taken leaves a chaser where it was */
static void loopy_prize(void) { I[0] = RND(4, 80); I[1] = RND(3, 27); }
static void g14_start(void) { F[0] = 42; F[1] = 15; F[2] = 0; loopy_prize(); fpm.n = 0; }
static void g14_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    if (held & MB_LEFT) F[2] += 0.07f;
    if (held & MB_RIGHT) F[2] -= 0.07f;
    F[0] += cosf(F[2]) * 0.6f;
    F[1] += sinf(F[2]) * 0.6f;
    if (F[0] < 0 || F[0] > 83) { F[0] = fclamp(F[0], 0, 83); F[2] = 3.14159265f - F[2]; }
    if (F[1] < 0 || F[1] > 29) { F[1] = fclamp(F[1], 0, 29); F[2] = -F[2]; }
    if (BOX(F[0], F[1], 3, 3, I[0], I[1], 3, 3)) {
        fpm_add(10);
        sfx_play_name("fpm_point");
        if (fpm.n < 40) { fpm.fx[fpm.n] = (float)I[0]; fpm.fy[fpm.n] = (float)I[1]; fpm.et[fpm.n] = 60; fpm.n++; }
        loopy_prize();
    }
    for (int k = 0; k < fpm.n; k++) {
        float dx = F[0] - fpm.fx[k], dy = F[1] - fpm.fy[k], d = sqrtf(dx * dx + dy * dy);
        if (d > 0.1f) { fpm.fx[k] += dx / d * 0.22f; fpm.fy[k] += dy / d * 0.22f; }
        if (fpm.et[k] > 0) { fpm.et[k]--; continue; } /* a new chaser is harmless for a moment */
        if (BOX(F[0], F[1], 3, 3, fpm.fx[k], fpm.fy[k], 3, 3)) { fpm_over(); return; }
    }
}
static void g14_draw(int ox, int oy) {
    RECT(I[0], I[1], 3, 3, MC_Y);
    for (int k = 0; k < fpm.n; k++)
        if (fpm.et[k] == 0 || (fpm.gt / 4) % 2) RECT(fpm.fx[k], fpm.fy[k], 3, 3, MC_R);
    RECT(F[0], F[1], 3, 3, MC_W);
    PX(F[0] + 1 + cosf(F[2]) * 2, F[1] + 1 + sinf(F[2]) * 2, MC_Y);
}

/* 15 FLITS: shoot up at bats that take two hits and shoot back           */
/* es 0..5 bats (hp), ex/ey their x/y line; 10..39 shots: es 1 yours, 2 theirs */
static void flit_new(int k) { fpm.es[k] = 2; fpm.fx[k] = (float)RND(4, 78); fpm.ey[k] = RND(3, 14); fpm.et[k] = RND(60, 130); }
static void g15_start(void) { F[0] = 42; I[0] = 1; flit_new(0); }
static void g15_update(uint8_t held, uint8_t pressed) {
    F[0] = fclamp(F[0] + DIRX(held) * 1.0f, 0, 81);
    if (fpm.gt % 600 == 0 && I[0] < 4) { flit_new(I[0]); I[0]++; }
    if (pressed & MB_FIRE)
        for (int k = 10; k < 40; k++) if (!fpm.es[k]) { fpm.es[k] = 1; fpm.fx[k] = F[0] + 2; fpm.fy[k] = 26; break; }
    for (int b = 0; b < I[0]; b++) {
        if (fpm.gt % 3 == 0) fpm.fx[b] = fclamp(fpm.fx[b] + RND(-2, 2), 2, 80);
        if (--fpm.et[b] <= 0) {
            for (int k = 10; k < 40; k++) if (!fpm.es[k]) { fpm.es[k] = 2; fpm.fx[k] = fpm.fx[b] + 2; fpm.fy[k] = (float)fpm.ey[b] + 3; break; }
            fpm.et[b] = RND(60, 130);
        }
    }
    for (int k = 10; k < 40; k++) {
        if (!fpm.es[k]) continue;
        fpm.fy[k] += fpm.es[k] == 1 ? -1.6f : 0.8f;
        if (fpm.fy[k] < -2 || fpm.fy[k] > 33) { fpm.es[k] = 0; continue; }
        if (fpm.es[k] == 1) {
            for (int b = 0; b < I[0]; b++)
                if (BOX(fpm.fx[k], fpm.fy[k], 1, 2, fpm.fx[b], fpm.ey[b], 5, 3)) {
                    fpm.es[k] = 0;
                    if (--fpm.es[b] <= 0) { fpm_add(10); sfx_play_name("fpm_point"); flit_new(b); }
                    break;
                }
        } else if (BOX(fpm.fx[k], fpm.fy[k], 1, 2, F[0], 28, 5, 3)) { fpm_over(); return; }
    }
}
static void g15_draw(int ox, int oy) {
    for (int b = 0; b < I[0]; b++) {
        int x = (int)fpm.fx[b], y = fpm.ey[b];
        RECT(x + 1, y, 3, 2, fpm.es[b] == 1 ? MC_Y : MC_R);
        int up = (fpm.gt / 5) % 2;
        PX(x, y + up, MC_R); PX(x + 4, y + up, MC_R);
    }
    for (int k = 10; k < 40; k++) if (fpm.es[k]) RECT(fpm.fx[k], fpm.fy[k], 1, 2, fpm.es[k] == 1 ? MC_W : MC_R);
    RECT(F[0], 28, 5, 3, MC_W);
}

/* 16 FORT: five guns, five buttons, saucers coming down                  */
/* es 1..: shots (0..19) with their speed; saucers 20..39 */
static void fort_shot(float x, float vx, float vy) {
    for (int k = 0; k < 20; k++) if (!fpm.es[k]) { fpm.es[k] = 1; fpm.fx[k] = x; fpm.fy[k] = 24; fpm.fvx[k] = vx; fpm.fvy[k] = vy; return; }
}
static void g16_start(void) { I[0] = 30; }
static void g16_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (pressed & MB_LEFT) fort_shot(33, -0.9f, -1.2f);
    if (pressed & MB_RIGHT) fort_shot(52, 0.9f, -1.2f);
    if (pressed & MB_UP) fort_shot(38, 0, -2.4f);
    if (pressed & MB_DOWN) fort_shot(47, 0, -2.4f);
    if (pressed & MB_FIRE) fort_shot(42.5f, 0, -1.8f);
    if (--I[0] <= 0) {
        for (int k = 20; k < 40; k++)
            if (!fpm.es[k]) {
                fpm.es[k] = 1; fpm.fx[k] = (float)RND(2, 80); fpm.fy[k] = -3;
                fpm.fvx[k] = RND(-2, 2) * 0.1f; fpm.fvy[k] = 0.12f + RND(0, 3) * 0.04f + fminf(fpm.gt / 7200.0f, 0.2f);
                break;
            }
        I[0] = imax(40, 120 - fpm.gt / 100);
    }
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] += fpm.fvx[k];
        fpm.fy[k] += fpm.fvy[k];
        if (k >= 20) {
            if (fpm.fx[k] < 0 || fpm.fx[k] > 82) fpm.fvx[k] = -fpm.fvx[k];
            if (fpm.fy[k] > 22) { fpm_over(); return; }
            continue;
        }
        if (fpm.fy[k] < -3 || fpm.fx[k] < -3 || fpm.fx[k] > 89) { fpm.es[k] = 0; continue; }
        for (int u = 20; u < 40; u++)
            if (fpm.es[u] && BOX(fpm.fx[k], fpm.fy[k], 1, 2, fpm.fx[u], fpm.fy[u], 5, 3)) {
                fpm.es[u] = 0; fpm.es[k] = 0; fpm_add(5); sfx_play_name("fpm_point"); break;
            }
    }
}
static void g16_draw(int ox, int oy) {
    for (int k = 20; k < 40; k++) if (fpm.es[k]) { RECT(fpm.fx[k], fpm.fy[k] + 1, 5, 1, MC_R); RECT(fpm.fx[k] + 1, fpm.fy[k], 3, 3, MC_R); }
    for (int k = 0; k < 20; k++) if (fpm.es[k]) RECT(fpm.fx[k], fpm.fy[k], 1, 2, MC_Y);
    RECT(30, 26, 26, 6, MC_W);
    RECT(32, 24, 2, 2, MC_W); RECT(51, 24, 2, 2, MC_W);
    RECT(37, 23, 2, 3, MC_Y); RECT(46, 23, 2, 3, MC_Y);
    RECT(42, 22, 2, 4, MC_R);
}

/* 17 HAMMER: eight seconds; every press counts                            */
static void g17_start(void) { I[0] = 0; }
static void g17_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (pressed & MB_FIRE) { fpm_add(1); I[0] = 6; sfx_play_name("fpm_blip"); }
    if (I[0] > 0) I[0]--;
    if (fpm.gt >= 8 * 60) fpm_over();
}
static void g17_draw(int ox, int oy) {
    int lift = I[0] > 0 ? 0 : 6;
    RECT(30, 24, 26, 8, MC_W);
    RECT(41, 8 + lift - 6, 4, 14, MC_Y);
    RECT(35, 4 + lift - 6, 16, 6, MC_R);
    RECT(0, 0, (8 * 60 - fpm.gt) * 86 / (8 * 60), 2, MC_Y);
}

/* 18 CUPID: remember where the hearts were                                */
/* grid: symbol (0 heart, 1 broken heart, 2 bell, 3 star, 4 note, 5 query, 6 shout);
 * es[c * 3 + r]: shown; ivar 0 phase timer, 1,2 cursor, 3 misses, 4 hearts left, 5 next timer */
#define CU_W 7
#define CU_H 3
static void cupid_board(void) {
    int hearts = RND(2, 5);
    for (int c = 0; c < CU_W; c++)
        for (int r = 0; r < CU_H; r++) { G[c][r] = (int8_t)RND(1, 6); fpm.es[c * 3 + r] = 0; }
    for (int h = 0; h < hearts;) {
        int c = RND(0, CU_W - 1), r = RND(0, CU_H - 1);
        if (G[c][r]) { G[c][r] = 0; h++; }
    }
    I[0] = 90;
    I[3] = 0;
    I[4] = hearts;
    I[5] = 0;
}
static void g18_start(void) { I[1] = 3; I[2] = 1; cupid_board(); }
static void g18_update(uint8_t held, uint8_t pressed) {
    (void)held;
    if (pressed & MB_B) { fpm_over(); return; }
    if (I[5] > 0) { if (--I[5] == 0) cupid_board(); return; }
    if (I[0] > 0) { I[0]--; return; }
    I[1] = iclamp(I[1] + DIRX(pressed), 0, CU_W - 1);
    I[2] = iclamp(I[2] + DIRY(pressed), 0, CU_H - 1);
    int c = I[1], r = I[2];
    if (!(pressed & MB_A) || fpm.es[c * 3 + r]) return;
    fpm.es[c * 3 + r] = 1;
    if (G[c][r] == 0) {
        fpm_add(10);
        sfx_play_name("fpm_point");
        if (--I[4] == 0) I[5] = 30;
    } else {
        sfx_play_name("fpm_blip");
        if (++I[3] >= 4) fpm_over();
    }
}
static void cupid_icon(int ox, int oy, int x, int y, int s) {
    switch (s) {
    case 0: case 1:
        RECT(x + 1, y, 2, 1, MC_R); RECT(x + 4, y, 2, 1, MC_R);
        RECT(x, y + 1, 7, 2, MC_R); RECT(x + 1, y + 3, 5, 1, MC_R); RECT(x + 2, y + 4, 3, 1, MC_R); PX(x + 3, y + 5, MC_R);
        if (s == 1) { PX(x + 3, y + 1, MC_K); PX(x + 4, y + 2, MC_K); PX(x + 3, y + 3, MC_K); }
        break;
    case 2: RECT(x + 2, y, 3, 1, MC_Y); RECT(x + 1, y + 1, 5, 3, MC_Y); RECT(x, y + 4, 7, 1, MC_Y); PX(x + 3, y + 5, MC_Y); break;
    case 3: PX(x + 3, y, MC_W); RECT(x, y + 2, 7, 1, MC_W); RECT(x + 2, y + 1, 3, 3, MC_W); PX(x + 1, y + 4, MC_W); PX(x + 5, y + 4, MC_W); break;
    case 4: RECT(x + 4, y, 1, 5, MC_W); RECT(x + 2, y + 4, 3, 2, MC_W); PX(x + 5, y + 1, MC_W); break;
    case 5: fpm_text(ox, oy, "?", x + 2, y, MC_Y); break;
    default: fpm_text(ox, oy, "!", x + 2, y, MC_W); break;
    }
}
static void g18_draw(int ox, int oy) {
    for (int c = 0; c < CU_W; c++)
        for (int r = 0; r < CU_H; r++) {
            int x = 2 + c * 12, y = 1 + r * 10;
            bool shown = I[0] > 0 || fpm.es[c * 3 + r] || fpm.state == MC_OVER;
            if (shown) cupid_icon(ox, oy, x + 2, y + 1, G[c][r]);
            else RECT(x + 1, y, 9, 8, MC_W);
        }
    if (I[0] == 0) {
        int x = 2 + I[1] * 12, y = 1 + I[2] * 10;
        if ((fpm.gt / 8) % 2) { RECT(x, y - 1, 11, 1, MC_R); RECT(x, y + 8, 11, 1, MC_R); }
    }
    for (int m = 0; m < I[3]; m++) RECT(84 - m * 3, 30, 2, 2, MC_R);
}

/* 19 BONK: a ball and a paddle against creatures that creep down          */
/* fvar 0 paddle, 1-4 ball; es 1 creature (fx, fy) from 1..; ivar 0 spawn timer, 1 side */
static void g19_start(void) { F[0] = 43; F[1] = 43; F[2] = 20; F[3] = 0.7f; F[4] = -0.8f; I[0] = 20; }
static void g19_update(uint8_t held, uint8_t pressed) {
    (void)pressed;
    F[0] = fclamp(F[0] + DIRX(held) * 1.4f, 6, 80);
    F[1] += F[3];
    F[2] += F[4];
    if (F[1] < 0) { F[1] = 0; F[3] = fabsf(F[3]); }
    if (F[1] > 84) { F[1] = 84; F[3] = -fabsf(F[3]); }
    if (F[2] < 0) { F[2] = 0; F[4] = fabsf(F[4]); }
    fpm_paddle_ball(&F[1], &F[2], &F[3], &F[4], F[0], 12, 29);
    if (F[2] > 32) { fpm_over(); return; }
    if (--I[0] <= 0) {
        for (int k = 0; k < 40; k++)
            if (!fpm.es[k]) { fpm.es[k] = 1; fpm.fx[k] = I[1] % 2 ? 80.0f : 2.0f; fpm.fy[k] = 0; I[1]++; break; }
        I[0] = imax(70, 150 - fpm.gt / 60);
    }
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        if (fpm.gt % 40 == 0) {
            /* a step in toward the middle, and down */
            fpm.fx[k] += fpm.fx[k] < 41 ? 3 : -3;
            fpm.fy[k] += 2;
        }
        if (fpm.fy[k] >= 25) { fpm_over(); return; }
        if (BOX(F[1], F[2], 2, 2, fpm.fx[k], fpm.fy[k], 4, 3)) {
            fpm.es[k] = 0; F[4] = -F[4]; fpm_add(10); sfx_play_name("fpm_point");
        }
    }
}
static void g19_draw(int ox, int oy) {
    for (int k = 0; k < 40; k++) if (fpm.es[k]) { RECT(fpm.fx[k], fpm.fy[k], 4, 3, MC_R); PX(fpm.fx[k] + 1, fpm.fy[k] + 1, MC_K); }
    RECT(F[0] - 6, 29, 12, 2, MC_W);
    RECT(F[1], F[2], 2, 2, MC_Y);
}

/* 20 SPY: three faces, one an impostor; shoot only the impostor           */
/* ivar 0 the impostor's spot, 1 round timer, 2 pause */
static void spy_round(void) { I[0] = RND(0, 2); I[1] = 120; I[2] = 0; }
static void g20_start(void) { spy_round(); }
static void g20_update(uint8_t held, uint8_t pressed) {
    if (fpm.gt >= 60 * 60) { fpm_over(); return; }
    if (I[2] > 0) { if (--I[2] == 0) spy_round(); return; }
    int aim = (held & MB_LEFT) ? 0 : (held & MB_RIGHT) ? 2 : 1;
    if (pressed & MB_FIRE) {
        if (aim == I[0]) { fpm_add(5); sfx_play_name("fpm_point"); I[2] = 20; I[0] = -1; }
        else { fpm_over(); }
        return;
    }
    if (--I[1] <= 0) fpm_over();
}
static void g20_draw(int ox, int oy) {
    uint8_t held = 0;
    for (int s = 0; s < 3; s++) {
        int x = 10 + s * 29, y = 3;
        bool alien = s == I[0];
        RECT(x + 1, y + 2, 6, 7, alien ? MC_Y : MC_W);
        RECT(x + 2, y + 4, 1, 1, MC_K); RECT(x + 5, y + 4, 1, 1, MC_K);
        if (alien) { PX(x + 1, y, MC_Y); PX(x + 6, y, MC_Y); PX(x + 2, y + 1, MC_Y); PX(x + 5, y + 1, MC_Y); }
        else RECT(x + 1, y + 1, 6, 1, MC_W);
        RECT(x, y + 9, 8, 4, alien ? MC_Y : MC_W);
    }
    (void)held;
    RECT(40, 28, 6, 4, MC_W);
    RECT(0, 0, I[1] * 86 / 120, 1, MC_R);
}

/* 21 HURDLE: run, jump the fences, take the gold in the air               */
/* fvar 0 x, 1 y (feet), 2 vy; es 1 short fence, 2 tall fence, 3 gold; ivar 0 timer */
static void g21_start(void) { F[0] = 20; F[1] = 30; I[0] = 40; }
static void g21_update(uint8_t held, uint8_t pressed) {
    F[0] = fclamp(F[0] + DIRX(held) * 1.0f, 0, 82);
    bool ground = F[1] >= 30;
    if (ground && (pressed & MB_A)) { F[2] = -1.9f; sfx_play_name("fpm_blip"); }
    F[2] += 0.1f;
    F[1] += F[2];
    if (F[1] >= 30) { F[1] = 30; F[2] = 0; }
    float sp = 0.8f + fminf(fpm.gt / 3600.0f, 1.0f) * 0.5f;
    if (--I[0] <= 0) {
        for (int k = 0; k < 40; k++)
            if (!fpm.es[k]) {
                int r = RND(0, 9);
                fpm.es[k] = r < 4 ? 1 : r < 7 ? 2 : 3;
                fpm.fx[k] = 88;
                fpm.fy[k] = fpm.es[k] == 3 ? (float)RND(10, 18) : 0;
                break;
            }
        I[0] = RND(45, 90);
    }
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        fpm.fx[k] -= sp;
        if (fpm.fx[k] < -4) { fpm.es[k] = 0; continue; }
        if (fpm.es[k] == 3) {
            if (BOX(F[0], F[1] - 5, 3, 5, fpm.fx[k], fpm.fy[k], 3, 3)) { fpm.es[k] = 0; fpm_add(10); sfx_play_name("fpm_point"); }
            continue;
        }
        int h = fpm.es[k] == 1 ? 4 : 8;
        if (BOX(F[0], F[1] - 5, 3, 5, fpm.fx[k], 32 - h, 2, h)) { fpm_over(); return; }
    }
}
static void g21_draw(int ox, int oy) {
    RECT(0, 31, 86, 1, MC_W);
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        if (fpm.es[k] == 3) RECT(fpm.fx[k], fpm.fy[k], 3, 3, MC_Y);
        else { int h = fpm.es[k] == 1 ? 4 : 8; RECT(fpm.fx[k], 32 - h, 2, h, MC_R); }
    }
    RECT(F[0], F[1] - 5, 3, 3, MC_W);
    PX(F[0] + ((fpm.gt / 5) % 2 ? 0 : 2), F[1] - 2, MC_W);
    PX(F[0] + ((fpm.gt / 5) % 2 ? 2 : 0), F[1] - 1, MC_W);
}

/* 22 CUE: aim, strike, and roll into the prize; a pocket or a miss ends it */
/* fvar 0,1 ball, 2,3 speed, 4 aim; ivar 0 rolling, 1,2 prize, 3 prize hit this shot */
static void cue_prize(void) {
    do { I[1] = RND(8, 76); I[2] = RND(5, 25); } while (fabsf(I[1] - F[0]) < 10 && fabsf(I[2] - F[1]) < 8);
}
static void g22_start(void) { F[0] = 20; F[1] = 16; F[4] = 0; cue_prize(); }
static void g22_update(uint8_t held, uint8_t pressed) {
    if (!I[0]) {
        if (held & MB_LEFT) F[4] -= 0.05f;
        if (held & MB_RIGHT) F[4] += 0.05f;
        if (pressed & MB_FIRE) {
            F[2] = cosf(F[4]) * 2.2f;
            F[3] = sinf(F[4]) * 2.2f;
            I[0] = 1;
            I[3] = 0;
            sfx_play_name("fpm_blip");
        }
        return;
    }
    float fr = (held & MB_FIRE) && !(pressed & MB_FIRE) ? 0.9f : 0.985f; /* holding the button brakes */
    F[2] *= fr;
    F[3] *= fr;
    F[0] += F[2];
    F[1] += F[3];
    if (F[0] < 0 || F[0] > 84) { F[0] = fclamp(F[0], 0, 84); F[2] = -F[2]; }
    if (F[1] < 0 || F[1] > 30) { F[1] = fclamp(F[1], 0, 30); F[3] = -F[3]; }
    static const int PXX[4] = {0, 84, 0, 84}, PYY[4] = {0, 0, 30, 30};
    for (int p = 0; p < 4; p++)
        if (fabsf(F[0] - PXX[p]) < 4 && fabsf(F[1] - PYY[p]) < 4) { fpm_over(); return; }
    if (BOX(F[0], F[1], 2, 2, I[1], I[2], 3, 3)) {
        fpm_add(10);
        sfx_play_name("fpm_point");
        I[3] = 1;
        cue_prize();
    }
    if (fabsf(F[2]) + fabsf(F[3]) < 0.06f) {
        I[0] = 0;
        if (!I[3]) fpm_over();
    }
}
static void g22_draw(int ox, int oy) {
    RECT(0, 0, 4, 4, MC_R); RECT(82, 0, 4, 4, MC_R); RECT(0, 28, 4, 4, MC_R); RECT(82, 28, 4, 4, MC_R);
    RECT(I[1], I[2], 3, 3, MC_Y);
    RECT(F[0], F[1], 2, 2, MC_W);
    if (!I[0])
        for (int d = 4; d < 20; d += 3) PX(F[0] + 1 + cosf(F[4]) * d, F[1] + 1 + sinf(F[4]) * d, MC_W);
}

/* 23 PIKE 2: see 05 */

/* 24 TWINS: two of you, one either side of a wall, moving as one          */
/* ivar 0,1 left you; 2,3 right you; 4,5 left block; 6,7 right block; 8 repeat;
 * es 1: a chaser (ex, ey in its half's cells, et: 0 left, 1 right) */
#define TW_W 10
static bool twins_free(int side, int x, int y) {
    if (x < 0 || x >= TW_W || y < 0 || y >= SG_H) return false;
    return !(x == I[4 + side * 2] && y == I[5 + side * 2]);
}
static void g24_start(void) {
    I[0] = 2; I[1] = 4; I[2] = 7; I[3] = 3;
    I[4] = RND(3, 6); I[5] = RND(1, 6); I[6] = RND(3, 6); I[7] = RND(1, 6);
    fpm.n = 0;
}
static void g24_update(uint8_t held, uint8_t pressed) {
    int d = pressed & MB_DIRS;
    if ((held & MB_DIRS) && !d) { if (++I[8] > 10) { I[8] = 0; d = held & MB_DIRS; } }
    else I[8] = 0;
    int dx = DIRX(d), dy = dx ? 0 : DIRY(d);
    if (dx || dy)
        for (int s = 0; s < 2; s++) {
            int nx = I[s * 2] + dx, ny = I[s * 2 + 1] + dy;
            if (twins_free(s, nx, ny)) { I[s * 2] = nx; I[s * 2 + 1] = ny; }
        }
    if (fpm.gt % 150 == 1 && fpm.n < 40) {
        int s = (fpm.gt / 150) % 2;
        fpm.es[fpm.n] = 1;
        fpm.et[fpm.n] = s;
        fpm.ex[fpm.n] = s ? 0 : TW_W - 1;
        fpm.ey[fpm.n] = (fpm.gt / 300) % 2 ? 0 : SG_H - 1;
        fpm.n++;
    }
    if (fpm.gt % 18 == 0)
        for (int k = 0; k < fpm.n; k++) {
            int s = fpm.et[k], x = fpm.ex[k], y = fpm.ey[k];
            fpm_chase(&x, &y, I[s * 2], I[s * 2 + 1]);
            if (twins_free(s, x, y)) { fpm.ex[k] = x; fpm.ey[k] = y; }
        }
    for (int k = 0; k < fpm.n; k++) {
        int s = fpm.et[k];
        if (fpm.ex[k] == I[s * 2] && fpm.ey[k] == I[s * 2 + 1]) { fpm_over(); return; }
    }
    PER_SEC(5);
}
static void g24_draw(int ox, int oy) {
    RECT(1 + TW_W * 4, 0, 4, 32, MC_Y);
    for (int s = 0; s < 2; s++) {
        int x0 = 1 + s * (TW_W + 1) * 4;
        RECT(x0 + I[4 + s * 2] * 4, I[5 + s * 2] * 4, 4, 4, MC_Y);
        RECT(x0 + I[s * 2] * 4, I[s * 2 + 1] * 4, 4, 4, MC_W);
    }
    for (int k = 0; k < fpm.n; k++) RECT(1 + fpm.et[k] * (TW_W + 1) * 4 + fpm.ex[k] * 4, fpm.ey[k] * 4, 4, 4, MC_R);
}

/* 25 HALT: stop each red block before it reaches the right                */
/* es 1 a block (lane in et), 2 your shot; ivar 0 next timer */
static void g25_start(void) { F[0] = 42; I[0] = 30; }
static void g25_update(uint8_t held, uint8_t pressed) {
    F[0] = fclamp(F[0] + DIRX(held) * 1.3f, 0, 82);
    if (pressed & MB_FIRE)
        for (int k = 20; k < 40; k++) if (!fpm.es[k]) { fpm.es[k] = 2; fpm.fx[k] = F[0] + 1; fpm.fy[k] = 25; break; }
    int blocks = 0;
    for (int k = 0; k < 20; k++) blocks += fpm.es[k] == 1;
    if (--I[0] <= 0 && blocks < 2) {
        for (int k = 0; k < 20; k++)
            if (!fpm.es[k]) {
                fpm.es[k] = 1; fpm.et[k] = RND(0, 2); fpm.fx[k] = -4;
                fpm.fvx[k] = 0.6f + RND(0, 5) * 0.2f + fminf(fpm.gt / 4000.0f, 0.6f);
                break;
            }
        I[0] = RND(40, 90);
    }
    for (int k = 0; k < 40; k++) {
        if (!fpm.es[k]) continue;
        if (fpm.es[k] == 1) {
            fpm.fx[k] += fpm.fvx[k];
            if (fpm.fx[k] > 86) { fpm_over(); return; }
            continue;
        }
        fpm.fy[k] -= 2.5f;
        if (fpm.fy[k] < -2) { fpm.es[k] = 0; continue; }
        for (int b = 0; b < 20; b++)
            if (fpm.es[b] == 1 && BOX(fpm.fx[k], fpm.fy[k], 1, 3, fpm.fx[b], 2 + fpm.et[b] * 6, 4, 4)) {
                fpm.es[b] = 0; fpm.es[k] = 0; fpm_add(10); sfx_play_name("fpm_point"); break;
            }
    }
}
static void g25_draw(int ox, int oy) {
    RECT(85, 0, 1, 20, MC_W);
    for (int k = 0; k < 40; k++) {
        if (fpm.es[k] == 1) RECT(fpm.fx[k], 2 + fpm.et[k] * 6, 4, 4, MC_R);
        else if (fpm.es[k] == 2) RECT(fpm.fx[k], fpm.fy[k], 1, 3, MC_Y);
    }
    RECT(F[0], 27, 4, 3, MC_W);
    PX(F[0] + 1, 26, MC_W);
}

const FplMicroDef FPL_MICRO_A[25] = {
    {"SHOO", g01_start, g01_update, g01_draw},
    {"KILN", g02_start, g02_update, g02_draw},
    {"FLAP", g03_start, g03_update, g03_draw},
    {"PEEK", g04_start, g04_update, g04_draw},
    {"PIKE", g05_start, pike_update, pike_draw},
    {"TIPTOE", g06_start, fpm_mines_update, fpm_mines_draw},
    {"ANGLER", g07_start, g07_update, g07_draw},
    {"BURROW", g08_start, g08_update, g08_draw},
    {"SLOPE", g09_start, fpm_tilt_update, fpm_tilt_draw},
    {"OGRE 1", g10_start, fpm_boss_update, fpm_boss_draw},
    {"SNIPE", g11_start, g11_update, g11_draw},
    {"DRIP", g12_start, g12_update, g12_draw},
    {"SHOO 2", g13_start, g13_update, g13_draw},
    {"LOOPY", g14_start, g14_update, g14_draw},
    {"FLITS", g15_start, g15_update, g15_draw},
    {"FORT", g16_start, g16_update, g16_draw},
    {"HAMMER", g17_start, g17_update, g17_draw},
    {"CUPID", g18_start, g18_update, g18_draw},
    {"BONK", g19_start, g19_update, g19_draw},
    {"SPY", g20_start, g20_update, g20_draw},
    {"HURDLE", g21_start, g21_update, g21_draw},
    {"CUE", g22_start, g22_update, g22_draw},
    {"PIKE 2", g23_start, pike_update, pike_draw},
    {"TWINS", g24_start, g24_update, g24_draw},
    {"HALT", g25_start, g25_update, g25_draw},
};
