/* SHUTTERBUG - drawing: the skies and the rock, every foe and boss, the
 * ships, the camera's frame, the HUD and the screens between stages. */
#include "shutterbug.h"

/* 1234567 -> "1,234,567" */
static const char *num(uint32_t v) {
    static char bufs[4][16];
    static int k;
    char *b = bufs[k++ & 3], tmp[16];
    snprintf(tmp, sizeof tmp, "%u", (unsigned)v);
    int n = (int)strlen(tmp), o = 0;
    for (int i = 0; i < n; i++) {
        b[o++] = tmp[i];
        if ((n - i - 1) % 3 == 0 && i < n - 1) b[o++] = ',';
    }
    b[o] = 0;
    return b;
}

static int SX(float x) { return (int)lroundf(x - sb.cam_x); }
static int SY(float y) { return (int)lroundf(y - sb.cam_y) + SHB_PF_Y; }

/* ------------------------------------------------------------------------ */
/* skies                                                                      */

typedef struct { uint8_t bg, band, far, near, rock, rock2, edge, dark; } Theme;
static const Theme THEME[5] = {
    {C_INK, C_NIGHT, C_DUSK, C_SLATE, C_SLATE, C_GREY, C_LIGHT, C_NIGHT},     /* space */
    {C_SKY, C_ICE, C_PINK, C_CREAM, C_TAN, C_BROWN, C_CREAM, C_EARTH},        /* teatime */
    {C_NIGHT, C_PURPLE, C_DUSK, C_VIOLET, C_DUSK, C_PURPLE, C_VIOLET, C_INK}, /* gloom */
    {C_FOREST, C_JADE, C_TEAL, C_LEAF, C_EARTH, C_HIDE, C_LEAF, C_BROWN},     /* fossil */
    {C_INK, C_PURPLE, C_MAGENTA, C_CYAN, C_SLATE, C_GREY, C_LIGHT, C_NIGHT},  /* the lens */
};

static void stars(int seed, float par, int col, int n) {
    for (int i = 0; i < n; i++) {
        int x = (int)((i * 97 + seed * 31) % 640 - fmodf(sb.cam_x * par, 640.0f));
        x = ((x % 640) + 640) % 640;
        int y = SHB_PF_Y + (i * 53 + seed * 7) % SHB_PF_H;
        if (x < SCREEN_W) gfx_pset(x, y, col);
    }
}

static void draw_sky(void) {
    int th = SHB_STAGE[sb.stage].theme;
    const Theme *T = &THEME[th];
    gfx_rect(0, SHB_PF_Y, SCREEN_W, SHB_PF_H, T->bg);
    int t = sb.frame_t;
    switch (th) {
    case TH_SPACE:
        stars(1, 0.15f, C_DUSK, 50);
        stars(2, 0.4f, C_GREY, 30);
        stars(3, 0.9f, C_WHITE, 14);
        /* the comet rain streaks past */
        if (sb.stage == 2 || sb.stage == 4)
            for (int k = 0; k < 6; k++) {
                int x = (int)(SCREEN_W - fmodf(t * 3.0f + k * 71, SCREEN_W + 40.0f));
                int y = SHB_PF_Y + (k * 37 + t / 2) % SHB_PF_H;
                gfx_line(x, y, x + 10, y - 3, C_SLATE);
            }
        break;
    case TH_TEA: {
        /* a pastel sky, clouds and teacup hills, all farther off than the rock */
        int base = SY(336) - 10;
        for (int k = 0; k < 6; k++) {
            int cx = (int)(((k * 120 - (int)(sb.cam_x * 0.2f)) % 720 + 720) % 720) - 60;
            int cy = SHB_PF_Y + 20 + (k * 29) % 60 - (int)((sb.cam_y - 168) * 0.2f);
            gfx_circ(cx, cy, 9, C_WHITE);
            gfx_circ(cx + 10, cy + 2, 7, C_WHITE);
            gfx_circ(cx - 9, cy + 3, 6, C_WHITE);
        }
        for (int k = 0; k < 5; k++) {
            int hx = (int)(((k * 150 - (int)(sb.cam_x * 0.45f)) % 750 + 750) % 750) - 80;
            int hy = base - 24 - (k % 2) * 10;
            gfx_circ(hx, hy + 30, 34, C_PINK);
            gfx_rect(hx - 14, hy - 6, 28, 12, C_CREAM);
            gfx_circb(hx + 15, hy, 5, C_CREAM);
        }
        break;
    }
    case TH_GLOOM:
        stars(4, 0.1f, C_DUSK, 40);
        gfx_circ(250 - (int)(sb.cam_x * 0.02f) % 40, 40, 16, C_GREY);
        gfx_circ(256 - (int)(sb.cam_x * 0.02f) % 40, 36, 14, T->bg);
        for (int k = 0; k < 8; k++) {
            int x = (int)(((k * 80 - (int)(sb.cam_x * 0.35f)) % 640 + 640) % 640) - 40;
            gfx_vline(x, SHB_PF_Y + 100, SHB_PF_Y + 150, C_PURPLE);
            gfx_line(x, SHB_PF_Y + 115, x - 8, SHB_PF_Y + 104, C_PURPLE);
            gfx_line(x, SHB_PF_Y + 122, x + 9, SHB_PF_Y + 110, C_PURPLE);
        }
        break;
    case TH_FOSSIL:
        gfx_dither(0, SHB_PF_Y, SCREEN_W, 40, C_TEAL, 6);
        for (int k = 0; k < 9; k++) {
            int x = (int)(((k * 70 - (int)(sb.cam_x * 0.4f)) % 630 + 630) % 630) - 30;
            int y = SHB_PF_Y + 150;
            for (int f = 0; f < 5; f++) gfx_line(x, y, x - 16 + f * 8, y - 40 + iabs(f - 2) * 8, C_JADE);
        }
        break;
    case TH_LENS:
        for (int k = 0; k < 12; k++) {
            float a = k * 0.5236f + t * 0.004f;
            int x = 160 + (int)(cosf(a) * 200), y = SHB_PF_Y + 84 + (int)(sinf(a) * 120);
            gfx_line(160, SHB_PF_Y + 84, x, y, k % 3 == 0 ? C_PURPLE : k % 3 == 1 ? C_NIGHT : C_DUSK);
        }
        stars(5, 0.0f, C_PINK, 30);
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* the rock                                                                   */

/* indoors, the rock behind: drawn over the sky, column by column */
static void draw_caves(void) {
    int th = SHB_STAGE[sb.stage].theme;
    static const uint8_t BACK[5] = {C_NIGHT, C_EARTH, C_INK, C_INK, C_NIGHT};
    static const uint8_t SPOT[5] = {C_DUSK, C_BROWN, C_NIGHT, C_FOREST, C_DUSK};
    int c0 = (int)(sb.cam_x / SHB_TILE);
    for (int c = c0; c <= c0 + SCREEN_W / SHB_TILE + 1 && c < shb_cols; c++) {
        if (!shb_cave[c]) continue;
        int x = SX((float)(c * SHB_TILE));
        gfx_rect(x, SHB_PF_Y, SHB_TILE, SHB_PF_H, BACK[th]);
        /* a far wall that drifts slower than the rock */
        for (int k = 0; k < 6; k++) {
            int wy = (int)(((c * 37 + k * 53) % 400) - sb.cam_y * 0.5f);
            if (wy < 0 || wy > SHB_PF_H) continue;
            gfx_rect(x + (k * 3) % 6, SHB_PF_Y + wy, 2, 2, SPOT[th]);
        }
        if (th == TH_TEA && (c % 6) == 0) gfx_vline(x + 3, SHB_PF_Y, SCREEN_H - 1, C_MAROON); /* inside the cake: layers */
    }
}

static void draw_terrain(void) {
    const Theme *T = &THEME[SHB_STAGE[sb.stage].theme];
    bool tea = SHB_STAGE[sb.stage].theme == TH_TEA;
    int c0 = (int)(sb.cam_x / SHB_TILE), r0 = (int)(sb.cam_y / SHB_TILE);
    for (int c = c0; c <= c0 + SCREEN_W / SHB_TILE + 1 && c < shb_cols; c++)
        for (int r = r0; r <= r0 + SHB_VIEW_ROWS + 1 && r < shb_rows; r++) {
            int tl = shb_tile[c][r];
            if (tl == TL_EMPTY) continue;
            int x = SX((float)(c * SHB_TILE)), y = SY((float)(r * SHB_TILE));
            if (y + SHB_TILE <= SHB_PF_Y) continue;
            if (tl == TL_RAIL) {
                gfx_hline(x, x + 7, y + 6, C_GREY);
                gfx_pset(x + 2, y + 7, C_BROWN);
                gfx_pset(x + 6, y + 7, C_BROWN);
                continue;
            }
            if (tl == TL_BREAK) {
                gfx_rect(x, y, 8, 8, shb_bhp[c][r] <= 1 ? T->dark : T->rock2);
                gfx_rectb(x, y, 8, 8, T->dark);
                gfx_line(x + 1, y + 2, x + 5, y + 6, T->dark);
                continue;
            }
            bool open_up = r > 0 && !tile_solid(shb_tile[c][r - 1]);
            bool open_dn = r + 1 < shb_rows && !tile_solid(shb_tile[c][r + 1]);
            if (tea) {
                /* a picnic cloth of rock */
                gfx_rect(x, y, 8, 8, ((c + r) & 1) ? T->rock : T->rock2);
            } else {
                gfx_rect(x, y, 8, 8, T->rock);
                int h = (c * 7 + r * 13) % 9;
                gfx_rect(x + h % 5, y + (h * 3) % 5, 3, 2, T->rock2);
                if (h == 4) gfx_pset(x + 6, y + 6, T->dark);
            }
            if (open_up) {
                gfx_hline(x, x + 7, y, T->edge);
                if (SHB_STAGE[sb.stage].theme == TH_TEA) { gfx_hline(x, x + 7, y + 1, C_PINK); gfx_pset(x + 2, y + 2, C_PINK); }
                if (SHB_STAGE[sb.stage].theme == TH_FOSSIL) { gfx_pset(x + 1, y - 1, C_LEAF); gfx_pset(x + 5, y - 2, C_LIME); }
            }
            if (open_dn) gfx_hline(x, x + 7, y + 7, T->dark);
        }
}

/* ------------------------------------------------------------------------ */
/* foes                                                                       */

static void spr_at(int id, float x, float y, int flags, const Foe *e) {
    const Sprite *s = &shb_spr[id];
    int dx = SX(x) - s->w / 2, dy = SY(y) - s->h / 2;
    if (e && e->flash > 0) spr_draw_ex(s, dx, dy, flags, NULL, C_WHITE);
    else spr_draw(s, dx, dy, flags);
}

/* a photographed foe: frozen, with a blue frame that blinks out at the end */
static void stun_mark(const Foe *e) {
    if (e->stun <= 0 && !e->falling) return;
    if (e->stun < 60 && (e->stun / 4) % 2) return;
    int x = SX(e->x), y = SY(e->y);
    int w = e->hw + 2, h = e->hh + 2;
    gfx_hline(x - w, x - w + 3, y - h, C_CYAN);
    gfx_hline(x + w - 3, x + w, y - h, C_CYAN);
    gfx_hline(x - w, x - w + 3, y + h, C_CYAN);
    gfx_hline(x + w - 3, x + w, y + h, C_CYAN);
    gfx_vline(x - w, y - h, y - h + 3, C_CYAN);
    gfx_vline(x + w, y - h, y - h + 3, C_CYAN);
    gfx_vline(x - w, y + h - 3, y + h, C_CYAN);
    gfx_vline(x + w, y + h - 3, y + h, C_CYAN);
}

static void draw_comet(const Foe *e, int r) {
    int x = SX(e->x), y = SY(e->y);
    gfx_circ(x, y, r, e->flash ? C_WHITE : C_GREY);
    gfx_circ(x - r / 3, y - r / 3, r / 2, C_LIGHT);
    gfx_circb(x, y, r, C_INK);
    gfx_pset(x + r / 3, y + r / 4, C_SLATE);
    for (int k = 0; k < 3; k++) gfx_pset(x + r + 2 + k * 3, y + (k - 1) * 2, C_ORANGE);
}

static void draw_letter(const Foe *e) {
    static const char *const L[3] = {"U", "F", "O"};
    int k = iclamp(e->arg, 0, 2);
    bool lit = (sb.letters >> k) & 1;
    int x = SX(e->x) - 4, y = SY(e->y) - 7;
    int th = SHB_STAGE[sb.stage].theme;
    /* hidden in the scenery: drawn in the colours around it, with a sparkle
     * now and then; lit up once photographed */
    if (th == TH_FOSSIL) {
        /* a clump of fern fronds; the letter is the gap between them */
        int cx = x + 4, cy = y + 7;
        for (int f = 0; f < 14; f++) {
            float a = f * 0.449f;
            int ex = cx + (int)(cosf(a) * 22), ey = cy + (int)(sinf(a) * 18);
            gfx_line(cx + (int)(cosf(a) * 4), cy + (int)(sinf(a) * 4), ex, ey, f % 2 ? C_JADE : C_LEAF);
            gfx_line(cx + (int)(cosf(a) * 6), cy + (int)(sinf(a) * 6) + 1, ex, ey + 1, C_JADE);
        }
        gfx_circ(cx, cy, 11, C_JADE);
    }
    int col = lit ? C_YELLOW : th == TH_FOSSIL ? C_FOREST : th == TH_GLOOM ? C_NIGHT : C_EARTH;
    text_draw_scaled(L[k], x, y, col, 2);
    if (!lit && (sb.frame_t + k * 70) % 180 < 8) {
        int sx = x + 3 + (sb.frame_t % 7), sy = y + 2 + (sb.frame_t % 9);
        gfx_pset(sx, sy, C_WHITE);
        gfx_pset(sx - 1, sy, C_LIGHT);
        gfx_pset(sx + 1, sy, C_LIGHT);
    }
    if (lit) text_draw_scaled(L[k], x + 1, y, C_AMBER, 2);
}

static void draw_scone(const Foe *e, bool w) {
    int x = SX(e->x), y = SY(e->y);
    gfx_circ(x, y, 22, w ? C_WHITE : C_TAN);
    gfx_circ(x - 4, y - 5, 16, w ? C_WHITE : C_AMBER);
    for (int k = 0; k < 9; k++) gfx_rect(x - 14 + (k * 11) % 28, y - 12 + (k * 7) % 24, 3, 3, C_BROWN);
    gfx_circb(x, y, 22, C_INK);
    gfx_rect(x - 12, y - 4, 4, 4, C_INK);
    gfx_rect(x - 2, y - 4, 4, 4, C_INK);
    gfx_hline(x - 10, x, y + 6, C_INK);
    /* her little hat */
    gfx_rect(x - 8, y - 26, 16, 4, C_PINK);
    gfx_rect(x - 5, y - 32, 10, 6, C_PINK);
}

static void draw_teapot(const Foe *e, bool w) {
    int x = SX(e->x), y = SY(e->y);
    int body = w ? C_WHITE : C_CYAN;
    gfx_circ(x, y + 2, 18, body);
    gfx_circb(x, y + 2, 18, C_INK);
    gfx_rect(x - 30, y - 4, 14, 4, body); /* the spout */
    gfx_rect(x - 34, y - 8, 6, 4, body);
    gfx_circb(x + 20, y + 2, 7, C_INK);
    gfx_rect(x - 10, y + 2, 20, 3, C_BLUE);
    gfx_rect(x - 7, y - 6, 3, 3, C_INK);
    gfx_rect(x + 4, y - 6, 3, 3, C_INK);
    if (e->open) {
        /* lid off: the weak spot, a hot red coal, steaming */
        gfx_circ(x, y - 16, 6, (sb.frame_t / 3) % 2 ? C_RED : C_ORANGE);
        gfx_rect(x - 10, y - 36, 20, 4, body);
        for (int k = 0; k < 3; k++) gfx_pset(x - 4 + k * 4, y - 24 - (sb.frame_t + k * 5) % 10, C_WHITE);
    } else {
        gfx_rect(x - 10, y - 18, 20, 4, body);
        gfx_rect(x - 2, y - 22, 4, 4, C_BLUE);
    }
}

static void draw_croak(const Foe *e, bool w) {
    int x = SX(e->x), y = SY(e->y);
    int c = w ? C_WHITE : C_LIME;
    if (e->state == 2) c = C_JADE;
    /* a fat old ghost-toad: haunches, a wide mouth, bulging eyes */
    gfx_circ(x + 10, y + 6, 8, c);
    gfx_circ(x - 2, y + 2, 14, c);
    gfx_circb(x - 2, y + 2, 14, C_INK);
    gfx_rect(x - 20, y + 10, 10, 4, c);
    gfx_rect(x + 12, y + 11, 10, 3, c);
    gfx_circ(x - 10, y - 10, 5, c);
    gfx_circ(x + 4, y - 11, 5, c);
    gfx_circ(x - 10, y - 11, 3, C_WHITE);
    gfx_circ(x + 4, y - 12, 3, C_WHITE);
    gfx_pset(x - 11, y - 11, C_INK);
    gfx_pset(x + 3, y - 12, C_INK);
    gfx_hline(x - 14, x + 8, y + 2, C_FOREST);
    gfx_line(x - 14, y + 2, x - 16, y, C_FOREST);
    gfx_dither_circle(x - 2, y + 2, 14, C_ICE, 3); /* a ghostly sheen */
}

static void draw_signal(const Foe *e, bool w) {
    int x = SX(e->x), y = SY(e->y);
    bool solid = e->stun > 0;
    int c = w ? C_WHITE : C_ICE;
    if (!solid && e->phase == 2) {
        /* gone: only his lantern glimmers */
        if ((sb.frame_t / 4) % 3 == 0) gfx_circ(x - 25, y + 11, 2, C_JADE);
        return;
    }
    if (!solid) { gfx_dither(x - 14, y - 18, 28, 40, c, e->phase == 0 ? 9 : 4); }
    else { gfx_rect(x - 12, y - 16, 24, 34, c); gfx_rectb(x - 12, y - 16, 24, 34, C_INK); }
    gfx_rect(x - 14, y - 24, 28, 6, C_NAVY);  /* the cap */
    gfx_rect(x - 8, y - 28, 16, 4, C_NAVY);
    gfx_rect(x - 2, y - 26, 4, 2, C_YELLOW);
    gfx_rect(x - 7, y - 10, 3, 4, C_INK);
    gfx_rect(x + 4, y - 10, 3, 4, C_INK);
    gfx_line(x - 16, y, x - 24, y + 8, c);    /* the lantern arm */
    gfx_circ(x - 25, y + 11, 3, C_LIME);
}

static void draw_jaw(const Foe *e, bool w) {
    int x = SX(e->x), y = SY(e->y);
    int c = w ? C_WHITE : C_HIDE;
    /* the neck and the body behind */
    gfx_rect(x + 10, y - 6, 60, 22, C_BROWN);
    gfx_circ(x + 60, y + 30, 34, C_BROWN);
    gfx_rect(x - 18, y - 14, 34, 14, c);
    int gape = e->open ? 12 : 2;
    gfx_rect(x - 20, y + gape, 32, 8, c);
    if (e->open) {
        gfx_rect(x - 16, y, 26, gape, C_MAROON);
        gfx_circ(x - 4, y + gape / 2, 3, (sb.frame_t / 3) % 2 ? C_RED : C_ORANGE);
    }
    for (int k = 0; k < 5; k++) {
        gfx_pset(x - 16 + k * 6, y + 1, C_WHITE);
        gfx_pset(x - 16 + k * 6, y + gape - 1, C_WHITE);
    }
    gfx_circ(x + 4, y - 9, 3, C_YELLOW);
    gfx_pset(x + 4, y - 9, C_INK);
    gfx_rectb(x - 18, y - 14, 34, 14, C_INK);
}

static void draw_kalei(const Foe *e, bool w) {
    int x = SX(e->x), y = SY(e->y);
    int t = sb.frame_t;
    int c = e->open ? ((t / 3) % 2 ? C_PINK : C_MAGENTA) : C_PURPLE;
    gfx_circ(x, y, 10, w ? C_WHITE : c);
    for (int k = 0; k < 6; k++) {
        float a = k * 1.047f + t * 0.05f;
        gfx_pset(x + (int)(cosf(a) * 6), y + (int)(sinf(a) * 6), C_WHITE);
    }
    gfx_circb(x, y, 10, C_INK);
}

static void draw_shard(const Foe *e) {
    int x = SX(e->x), y = SY(e->y);
    static const uint8_t COL[4] = {C_CYAN, C_PINK, C_YELLOW, C_LIME};
    int c = COL[e->arg % 4];
    gfx_line(x - 5, y, x, y - 5, c);
    gfx_line(x, y - 5, x + 5, y, c);
    gfx_line(x + 5, y, x, y + 5, c);
    gfx_line(x, y + 5, x - 5, y, c);
    gfx_rect(x - 2, y - 2, 5, 5, c);
    gfx_pset(x - 1, y - 1, C_WHITE);
}

static void draw_foe(const Foe *e) {
    if (e->t < 0 && e->kind != K_DART) return;
    bool w = e->flash > 0;
    int fx = e->flags & F_CEIL ? SPR_FLIPY : 0;
    switch (e->kind) {
    case K_PUFF: spr_at(SP_PUFF, e->x, e->y, 0, e); break;
    case K_MINT: spr_at(SP_MINT, e->x, e->y, 0, e); break;
    case K_TURRET: spr_at(e->flags & F_RED ? SP_TURRET_RED : SP_TURRET, e->x, e->y, fx, e); break;
    case K_WALLBOMB: spr_at(SP_WALLBOMB, e->x, e->y, fx, e); break;
    case K_CHOMPER:
        if (e->y > e->ay + 1 && !e->falling) gfx_vline(SX(e->x), SY(e->ay) - 6, SY(e->y) - 6, C_LIGHT);
        spr_at(SP_CHOMPER, e->x, e->y, 0, e);
        break;
    case K_CUBE: spr_at(e->flags & F_RED ? SP_CUBE_RED : SP_CUBE, e->x, e->y, 0, e); break;
    case K_TOAST: spr_at(SP_TOAST, e->x, e->y, fx, e); break;
    case K_SWIRL: spr_at(SP_SWIRL, e->x, e->y, (sb.frame_t / 6) % 2 ? SPR_FLIPX : 0, e); break;
    case K_ERUPTER:
        if (e->state == 0) {
            /* the warning: a burst rising at the edge */
            bool top = (e->flags & F_TOP) != 0;
            int x = SX(e->x), h = 4 + e->t / 4;
            for (int k = 0; k < 4; k++) {
                int px = x - 6 + k * 4, ph = h - (k % 2) * 3;
                if (top) gfx_vline(px, SHB_PF_Y, SHB_PF_Y + ph, (sb.frame_t / 3 + k) % 2 ? C_CYAN : C_WHITE);
                else gfx_vline(px, SCREEN_H - 1 - ph, SCREEN_H - 1, (sb.frame_t / 3 + k) % 2 ? C_CYAN : C_WHITE);
            }
        } else spr_at(SP_ERUPTER, e->x, e->y, (e->flags & F_TOP) ? SPR_FLIPY : 0, e);
        break;
    case K_ROCKBIG: draw_comet(e, 10); break;
    case K_ROCK: draw_comet(e, 4); break;
    case K_BURSTER: {
        spr_at(SP_BURSTER, e->x, e->y, 0, e);
        if (e->t > 180 && (e->t / 4) % 2) gfx_circb(SX(e->x), SY(e->y), 9, C_RED);
        break;
    }
    case K_DART:
        if (e->t < 0) {
            /* the "!" at the left edge */
            if ((e->t / 4) % 2 == 0 && e->t > -50) text_draw_scaled("!", 4, SY(e->y) - 7, C_RED, 2);
            return;
        }
        spr_at(SP_DART, e->x, e->y, e->state == 3 ? SPR_FLIPX : 0, e);
        break;
    case K_HOPPER:
        if (e->state == 0) {
            /* the same warning as a geyser's, in amber */
            bool top = (e->flags & F_TOP) != 0;
            int x = SX(e->x), h = 4 + e->t / 4;
            for (int k = 0; k < 4; k++) {
                int px = x - 6 + k * 4, ph = h - (k % 2) * 3;
                int c = (sb.frame_t / 3 + k) % 2 ? C_AMBER : C_YELLOW;
                if (top) gfx_vline(px, SHB_PF_Y, SHB_PF_Y + ph, c);
                else gfx_vline(px, SCREEN_H - 1 - ph, SCREEN_H - 1, c);
            }
        } else spr_at(SP_HOPPER, e->x, e->y, 0, e);
        break;
    case K_GHOST: {
        const Sprite *s = &shb_spr[SP_GHOST];
        int x = SX(e->x) - 6, y = SY(e->y) - 6;
        if (e->stun > 0 || e->state == 0) spr_at(SP_GHOST, e->x, e->y, 0, e);
        else if (e->state == 2) {
            if ((sb.frame_t / 2) % 4 == 0) gfx_dither(x + 2, y + 2, 8, 8, C_DUSK, 4);
        } else {
            for (int yy = 0; yy < s->h; yy++)
                for (int xx = 0; xx < s->w; xx++)
                    if (((xx + yy + sb.frame_t) & 1) && s->px[yy * s->w + xx] != TRANSPARENT) gfx_pset(x + xx, y + yy, s->px[yy * s->w + xx]);
        }
        break;
    }
    case K_CART: spr_at(SP_CART, e->x, e->y, 0, e); break;
    case K_RAILBOMB: spr_at(e->flags & F_RED ? SP_RAILBOMB_RED : SP_RAILBOMB, e->x, e->y, fx, e); break;
    case K_SILO: spr_at(SP_SILO, e->x, e->y, fx, e); break;
    case K_ROCKET: spr_at(SP_ROCKET, e->x, e->y, e->vx > 0 ? SPR_FLIPX : 0, e); break;
    case K_NIP: spr_at(SP_NIP, e->x, e->y, 0, e); break;
    case K_KITE: spr_at(SP_KITE, e->x, e->y, 0, e); break;
    case K_LASER: {
        int x = SX(e->x), y0 = SY(e->y - e->hh), y1 = SY(e->y + e->hh);
        gfx_rect(x - 4, y0 - 4, 9, 4, C_SLATE);
        gfx_rect(x - 4, y1, 9, 4, C_SLATE);
        if (e->state == 1) {
            gfx_rect(x - 1, y0, 3, y1 - y0, (sb.frame_t / 2) % 2 ? C_RED : C_PINK);
            gfx_vline(x, y0, y1, C_WHITE);
        } else if ((e->t % (e->arg * 2)) > e->arg * 2 - 24 && (sb.frame_t / 3) % 2) {
            gfx_vline(x, y0, y1, C_MAROON); /* about to switch on */
        }
        break;
    }
    case K_GEN: spr_at(SP_GEN, e->x, e->y, fx, e); break;
    case K_MOVER: {
        int x = SX(e->x) - e->hw, y = SY(e->y) - e->hh;
        gfx_rect(x, y, e->hw * 2, e->hh * 2, e->frozen ? C_ICE : C_WHITE);
        gfx_rectb(x, y, e->hw * 2, e->hh * 2, C_INK);
        gfx_rect(x + 3, y + 3, 4, 4, C_CREAM);
        gfx_rect(x + e->hw * 2 - 7, y + e->hh * 2 - 7, 4, 4, C_CREAM);
        break;
    }
    case K_SECRET: {
        /* an odd little thing in the scenery: a cup with a face */
        int x = SX(e->x), y = SY(e->y);
        int th = SHB_STAGE[sb.stage].theme;
        int c = e->state ? C_GREY : th == TH_GLOOM ? C_VIOLET : th == TH_FOSSIL ? C_HIDE : C_CREAM;
        gfx_rect(x - 5, y - 4, 10, 8, c);
        gfx_circb(x + 6, y, 2, c);
        if (!e->state) { gfx_pset(x - 2, y - 1, C_INK); gfx_pset(x + 2, y - 1, C_INK); gfx_hline(x - 1, x + 1, y + 2, C_INK); }
        if (!e->state && (sb.frame_t % 150) < 6) gfx_pset(x - 6, y - 6, C_WHITE);
        break;
    }
    case K_LETTER: draw_letter(e); break;
    case K_ORB: {
        int x = SX(e->x), y = SY(e->y);
        gfx_circ(x, y, 3, (sb.frame_t / 4) % 2 ? C_YELLOW : C_AMBER);
        gfx_pset(x - 1, y - 1, C_WHITE);
        break;
    }
    case K_SCONE: draw_scone(e, w); break;
    case K_TEAPOT: draw_teapot(e, w); break;
    case K_CROAK: draw_croak(e, w); break;
    case K_SIGNAL: draw_signal(e, w); break;
    case K_JAW: draw_jaw(e, w); break;
    case K_CHUTE: {
        int x = SX(e->x), y = SY(e->y);
        gfx_rect(x - 8, y - 8, 16, 16, w ? C_WHITE : C_CREAM);
        gfx_rectb(x - 8, y - 8, 16, 16, C_INK);
        gfx_rect(x - 4, e->arg ? y - 8 : y + 4, 8, 4, C_INK);
        break;
    }
    case K_KALEI: draw_kalei(e, w); break;
    case K_SHARD: draw_shard(e); break;
    default: break;
    }
    stun_mark(e);
}

static void draw_foes(void) {
    /* scenery first, then foes, then the bosses' shards on top */
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < SHB_MAX_FOES; i++) {
            const Foe *e = &sb.foe[i];
            if (!e->alive) continue;
            bool scenery = e->kind == K_LETTER || e->kind == K_SECRET || e->kind == K_LASER;
            if ((pass == 0) != scenery) continue;
            draw_foe(e);
        }
}

/* ------------------------------------------------------------------------ */
/* shots, pickups, effects                                                    */

static void draw_shots(void) {
    for (int i = 0; i < SHB_MAX_PSHOTS; i++) {
        const PShot *s = &sb.ps[i];
        if (!s->alive) continue;
        int x = SX(s->x), y = SY(s->y);
        if (s->kind == PS_GUN) {
            gfx_hline(x - 3, x + 2, y, s->owner ? C_LIME : C_YELLOW);
            gfx_hline(x - 1, x + 2, y - 1, C_WHITE);
        } else {
            gfx_circb(x, y, 4, (sb.frame_t / 2) % 2 ? C_WHITE : C_CYAN);
            gfx_circb(x, y, 2, C_SKY);
        }
    }
    for (int i = 0; i < SHB_MAX_ESHOTS; i++) {
        const EShot *s = &sb.es[i];
        if (!s->alive) continue;
        int x = SX(s->x), y = SY(s->y);
        switch (s->kind) {
        case ES_SHOT: gfx_circ(x, y, 2, C_PINK); gfx_pset(x, y, C_WHITE); break;
        case ES_BOUNCE: gfx_circ(x, y, 2, C_ORANGE); gfx_circb(x, y, 3, C_RED); break;
        case ES_CRUMB: gfx_rect(x - 2, y - 2, 4, 4, C_TAN); gfx_rectb(x - 2, y - 2, 4, 4, C_BROWN); break;
        case ES_FALL: gfx_circ(x, y, 3, s->arg ? C_PINK : C_TAN); gfx_circb(x, y, 3, C_BROWN); break;
        case ES_ARROW:
            gfx_vline(x, y - 4, y + 4, C_SKY);
            if (s->arg) { gfx_pset(x - 1, y - 3, C_SKY); gfx_pset(x + 1, y - 3, C_SKY); }
            else { gfx_pset(x - 1, y + 3, C_SKY); gfx_pset(x + 1, y + 3, C_SKY); }
            break;
        case ES_CRAWL: gfx_rect(x - 2, y - 2, 5, 5, (sb.frame_t / 3) % 2 ? C_PINK : C_MAGENTA); break;
        case ES_RETAL: gfx_circ(x, y, 2, C_LIME); gfx_circb(x, y, 3, (sb.frame_t / 3) % 2 ? C_FOREST : C_LIME); break;
        case ES_BIG:
            gfx_circ(x, y, 4, s->arg >= 100 ? C_LIME : C_RED);
            gfx_circ(x - 1, y - 1, 1, C_WHITE);
            break;
        default: gfx_circ(x, y, 2, C_RED); break;
        }
    }
}

static void draw_pickups(void) {
    for (int i = 0; i < SHB_MAX_PICKUPS; i++) {
        const Pickup *k = &sb.pk[i];
        if (!k->alive) continue;
        if (k->t > 600 && (k->t / 4) % 2) continue;
        int id = k->kind == PK_CRYSTAL ? SP_CRYSTAL : SP_WRENCH;
        const Sprite *s = &shb_spr[id];
        spr_draw(s, SX(k->x) - s->w / 2, SY(k->y) - s->h / 2, 0);
    }
}

static void draw_fx(void) {
    for (int i = 0; i < SHB_MAX_PARTS; i++) {
        const Part *p = &sb.part[i];
        if (p->life > 0) gfx_pset(SX(p->x), SY(p->y), p->col);
    }
    for (int i = 0; i < SHB_MAX_BLASTS; i++) {
        const Blast *b = &sb.blast[i];
        if (!b->alive) continue;
        int r = b->r * imin(b->t + 4, 12) / 12;
        gfx_circb(SX(b->x), SY(b->y), r, b->t < 6 ? C_WHITE : C_CYAN);
        if (b->t < 4) gfx_dither_circle(SX(b->x), SY(b->y), r, C_WHITE, 6);
    }
}

/* ------------------------------------------------------------------------ */
/* the ships and the camera                                                   */

static void draw_ship(int p) {
    const Ship *s = &sb.ship[p];
    if (!s->on) return;
    if (s->snap_t > 0) {
        /* the photo just taken: a white frame */
        int x = SX(s->snap_x) - SHB_PHOTO_W / 2, y = SY(s->snap_y) - SHB_PHOTO_H / 2;
        if (s->snap_t > 10) gfx_dither(x, y, SHB_PHOTO_W, SHB_PHOTO_H, C_WHITE, 8);
        gfx_rectb(x, y, SHB_PHOTO_W, SHB_PHOTO_H, C_WHITE);
        gfx_rectb(x + 1, y + 1, SHB_PHOTO_W - 2, SHB_PHOTO_H - 2, C_WHITE);
    }
    if (!s->alive) return;
    if (s->inv > 0 && (s->inv / 3) % 2) return;
    int id = p == 0 ? (s->armour ? SP_POPPY : SP_POPPY_BARE) : (s->armour ? SP_SPRIG : SP_SPRIG_BARE);
    const Sprite *sp = &shb_spr[id];
    int x = SX(s->x) - 9, y = SY(s->y) - 5;
    spr_draw(sp, x, y, 0);
    /* the propeller */
    if ((sb.frame_t / 3) % 2) gfx_vline(x - 1, y + 3, y + 7, C_LIGHT);
    /* the two dots: the rings are ready */
    if (s->hold_t >= SHB_CHARGE_T && (sb.frame_t / 4) % 2 == 0) {
        gfx_circ(x + 23, y + 2, 1, C_WHITE);
        gfx_circ(x + 23, y + 8, 1, C_WHITE);
    } else if (s->hold_t > 10 && s->hold_t < SHB_CHARGE_T) {
        int n = s->hold_t * 4 / SHB_CHARGE_T;
        for (int k = 0; k < n; k++) gfx_pset(x + 20 + k, y + 5, C_SKY);
    }
    /* the camera's cursor, a third of the screen ahead */
    int cx = SX(s->x + SHB_CURSOR_DX), cy = SY(s->y);
    int col = s->flash >= SHB_FLASH_MAX ? (p ? C_LIME : C_WHITE) : C_SLATE;
    gfx_hline(cx - 6, cx - 3, cy, col);
    gfx_hline(cx + 3, cx + 6, cy, col);
    gfx_vline(cx, cy - 6, cy - 3, col);
    gfx_vline(cx, cy + 3, cy + 6, col);
    if (s->flash >= SHB_FLASH_MAX) {
        int hw = SHB_PHOTO_W / 2, hh = SHB_PHOTO_H / 2;
        gfx_hline(cx - hw, cx - hw + 4, cy - hh, col);
        gfx_vline(cx - hw, cy - hh, cy - hh + 4, col);
        gfx_hline(cx + hw - 4, cx + hw, cy + hh, col);
        gfx_vline(cx + hw, cy + hh - 4, cy + hh, col);
    }
}

/* ------------------------------------------------------------------------ */
/* the HUD                                                                    */

static void draw_hud(void) {
    gfx_rect(0, 0, SCREEN_W, SHB_PF_Y, C_INK);
    gfx_hline(0, SCREEN_W - 1, SHB_PF_Y - 1, C_NIGHT);
    /* the bonus bar: how far to the next life */
    uint32_t nx = shb_next_extend(), pv = sb.extends > 0 ? SHB_EXTEND_AT[sb.extends - 1] : 0;
    int fill = nx ? (int)((uint64_t)(sb.total - pv) * 40 / (nx - pv)) : 40;
    gfx_rectb(2, 2, 42, 7, C_SLATE);
    gfx_rect(3, 3, iclamp(fill, 0, 40), 5, C_AMBER);
    /* each ship's camera meter */
    for (int p = 0; p < sb.players; p++) {
        const Ship *s = &sb.ship[p];
        int x = 50 + p * 36;
        gfx_rect(x, 3, 6, 5, p ? C_LIME : C_RED);
        gfx_rect(x + 2, 2, 2, 1, C_LIGHT);
        gfx_rectb(x + 8, 2, 24, 7, C_SLATE);
        int f = s->flash * 22 / SHB_FLASH_MAX;
        gfx_rect(x + 9, 3, f, 5, s->flash >= SHB_FLASH_MAX ? ((sb.frame_t / 8) % 2 ? C_WHITE : C_CYAN) : C_SKY);
    }
    const char *sc = num(sb.score);
    text_draw(sc, 200 - text_width(sc), 2, C_WHITE);
    /* spare lives */
    char buf[16];
    snprintf(buf, sizeof buf, "x%d", sb.lives);
    int lx = SCREEN_W - 30;
    gfx_circ(lx, 6, 3, C_RED);
    gfx_pset(lx + 1, 5, C_WHITE);
    text_draw(buf, lx + 6, 2, C_LIGHT);
}

static const char *const TIPS[8] = {
    "HOLD " GLYPH_B ": YOUR GUN FIRES BY ITSELF",
    "KEEP HOLDING " GLYPH_B "... THEN LET GO: FOUR RINGS!",
    "BUMPING INTO ROCK DOES NO HARM. RINGS BOUNCE OFF IT.",
    "TAP " GLYPH_A " TO TAKE A PHOTO WHERE THE FRAME IS",
    "PHOTOGRAPHED FOES FREEZE, AND PAY DOUBLE",
    "GREEN FOES SHOOT BACK AS THEY POP... UNLESS PHOTOGRAPHED",
    "LET GO OF " GLYPH_B " AND BULBS FLY TO YOU: TWO REFILL IT",
    "TWO HITS LOSE A SHIP. POINTS EARN SHIPS. HAVE FUN!",
};

static void draw_overlay_text(void) {
    if (sb.stage == 0 && sb.tip >= 0 && sb.tip < 8) {
        gfx_rect(0, SCREEN_H - 14, SCREEN_W, 14, C_INK);
        text_center(TIPS[sb.tip], 160, SCREEN_H - 11, C_YELLOW);
    }
    if (sb.msg_t > 0 && sb.msg) {
        int w = text_width(sb.msg) + 10;
        gfx_rect(160 - w / 2, 40, w, 13, C_INK);
        text_center(sb.msg, 160, 43, C_YELLOW);
    }
}

static void draw_play(void) {
    int sx = sb.shake > 0 ? (sb.frame_t % 3) - 1 : 0, sy = sb.shake > 8 ? ((sb.frame_t / 2) % 3) - 1 : 0;
    gfx_clip(0, SHB_PF_Y, SCREEN_W, SHB_PF_H);
    draw_sky();
    gfx_camera(sx, sy);
    draw_caves();
    draw_terrain();
    draw_foes();
    draw_pickups();
    draw_shots();
    for (int p = 0; p < 2; p++) draw_ship(p);
    draw_fx();
    gfx_camera(0, 0);
    if (sb.flash > 0) gfx_dither(0, SHB_PF_Y, SCREEN_W, SHB_PF_H, C_WHITE, sb.flash);
    gfx_noclip();
    draw_hud();
    draw_overlay_text();
}

/* ------------------------------------------------------------------------ */
/* the screens                                                                */

static const uint8_t LOGO[] = {C_WHITE, C_PINK, C_PINK, C_MAGENTA, C_MAGENTA, C_PURPLE};

static void draw_banner(void) {
    draw_play();
    const StageDef *st = &SHB_STAGE[sb.stage];
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER};
    gfx_rect(0, 62, SCREEN_W, 40, C_INK);
    text_center(st->name, 160, 66, C_LIGHT);
    ui_fancy_center(st->sub, 160, 78, 2, grad, 3, C_INK, C_MAROON);
}

static void draw_title(void) {
    gfx_cls(C_NAVY);
    int t = sb.frame_t;
    for (int i = 0; i < 60; i++) {
        int x = (int)((i * 97 + t * (1 + i % 3)) % 340) - 10, y = (i * 41) % 180;
        gfx_pset(SCREEN_W - x, y, i % 4 ? C_DUSK : C_WHITE);
    }
    ui_fancy_center("SHUTTERBUG", 160, 18, 3, LOGO, 6, C_INK, C_NIGHT);
    text_center("POPPY'S PICTURE-PERFECT TRIP", 160, 48, C_LIGHT);
    /* Poppy and her camera */
    const Sprite *sp = &shb_spr[SP_POPPY];
    int py = 78 + (int)(sinf(t * 0.05f) * 4);
    spr_draw_scaled(sp, 88, py - 10, 2, 0);
    if ((t / 50) % 3 == 0 && (t % 50) < 6) gfx_dither(170, py - 20, 70, 44, C_WHITE, 10);
    gfx_rectb(170, py - 20, 70, 44, C_WHITE);
    spr_draw(&shb_spr[SP_CUBE], 200, py - 4, 0);
    if (sb.players == 2 || (t / 120) % 2) spr_draw_scaled(&shb_spr[SP_SPRIG], 60, py + 10, 1, 0);
    static const char *const ITEMS[2] = {"1 PLAYER", "2 PLAYERS (CO-OP)"};
    for (int i = 0; i < 2; i++) {
        int y = 118 + i * 11;
        text_center(ITEMS[i], 160, y, sb.menu == i ? C_WHITE : C_GREY);
        if (sb.menu == i) ui_cursor(160 - text_width(ITEMS[i]) / 2 - 10, y, t);
    }
    char buf[64];
    snprintf(buf, sizeof buf, "BEST %s", num(sbs.best));
    tiny_center(buf, 160, 4, C_YELLOW);
    snprintf(buf, sizeof buf, "MOST FOE TYPES PHOTOGRAPHED %d   MOST SECRET PHOTOS %d", sbs.most_types, sbs.most_secrets);
    tiny_center(buf, 160, 148, C_SKY);
    gfx_rect(0, 160, SCREEN_W, 20, C_INK);
    ui_hint(6, 166, GLYPH_B, "LIBRARY", C_GREY);
    tiny_draw("BEAMDOWN SOFTWORKS 1986", SCREEN_W - 6 - tiny_width("BEAMDOWN SOFTWORKS 1986"), 168, C_SLATE);
}

#define STORY_PAGES 3
static void draw_story(void) {
    gfx_cls(C_NAVY);
    int t = sb.state_t, page = imin(STORY_PAGES - 1, t / 200);
    for (int i = 0; i < 40; i++) gfx_pset((i * 83 + t / 3) % SCREEN_W, (i * 47) % 120, C_DUSK);
    /* a little house, and Poppy outside it */
    gfx_rect(40, 110, 50, 34, C_CREAM);
    gfx_rect(36, 100, 58, 12, C_RED);
    gfx_rect(58, 124, 12, 20, C_BROWN);
    gfx_rect(0, 144, SCREEN_W, 36, C_FOREST);
    int px = 120 + (page == 2 ? (t - 400) * 2 : 0), py = 120 - (page == 2 ? (t - 400) : 0);
    spr_draw_scaled(&shb_spr[SP_POPPY], px, py, 2, 0);
    if (sb.players == 2) spr_draw_scaled(&shb_spr[SP_SPRIG], px - 30, py + 14, 2, 0);
    if (page == 0 && (t / 30) % 3 == 0) gfx_dither(150, 100, 60, 40, C_WHITE, 8);
    static const char *const LINES[STORY_PAGES] = {
        "POPPY GOT A BRAND NEW CAMERA FOR HER BIRTHDAY.",
        "SO SHE PACKED A LUNCH AND SET OFF ACROSS THE STARS...",
        "...TO PHOTOGRAPH EVERYTHING. EVEN THE THINGS THAT BITE.",
    };
    gfx_rect(0, 18, SCREEN_W, 13, C_INK);
    text_center(LINES[page], 160, 21, C_LIGHT);
    if (sb.players == 2 && page == 2) text_center("AND HER PAL SPRIG CAME TOO.", 160, 34, C_LIME);
    if ((t / 20) % 2) text_center(GLYPH_A " SKIP", 290, 168, C_SLATE);
}

static void draw_lost(void) {
    gfx_cls(C_INK);
    char buf[48];
    text_center(SHB_STAGE[sb.stage].sub, 160, 50, C_LIGHT);
    snprintf(buf, sizeof buf, "SHIPS LEFT  %d", sb.lives);
    text_center(buf, 160, 74, C_WHITE);
    uint32_t nx = shb_next_extend();
    if (nx) snprintf(buf, sizeof buf, "TO NEXT  %s", num(nx - sb.total));
    else snprintf(buf, sizeof buf, "NO MORE SHIPS TO EARN");
    text_center(buf, 160, 90, C_AMBER);
    text_center("BACK TO THE START OF THE STAGE", 160, 116, C_GREY);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    static const uint8_t grad[] = {C_WHITE, C_PINK, C_MAGENTA};
    ui_fancy_center("OUT OF FILM", 160, 60, 2, grad, 3, C_INK, C_MAROON);
    char buf[48];
    snprintf(buf, sizeof buf, "FINAL SCORE  %s", num(sb.best));
    text_center(buf, 160, 96, C_WHITE);
    if (sb.best >= sbs.best && sb.best > 0) text_center("A NEW BEST!", 160, 110, C_YELLOW);
}

static void draw_clear(void) {
    gfx_cls(C_NAVY);
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER};
    ui_fancy_center(sb.stage == 0 ? "OFF WE GO!" : "STAGE CLEAR", 160, 40, 2, grad, 3, C_INK, C_MAROON);
    text_center(SHB_STAGE[sb.stage].sub, 160, 66, C_LIGHT);
    char buf[48];
    snprintf(buf, sizeof buf, "SCORE  %s", num(sb.score));
    text_center(buf, 160, 86, C_WHITE);
    uint32_t nx = shb_next_extend();
    if (nx) { snprintf(buf, sizeof buf, "TO NEXT SHIP  %s", num(nx - sb.total)); text_center(buf, 160, 98, C_AMBER); }
    if (sb.stage == 0 && sb.zero_prologue) {
        /* the meta message, for a prologue with not a single point */
        text_center("NOT ONE POINT? A HOLIDAY WITH NO TROUBLE AT ALL.", 160, 126, C_PINK);
    }
    /* the photo album so far */
    for (int k = 0; k < 5; k++) {
        int x = 100 + k * 26, y = 140;
        bool got = sb.stage >= k + 1;
        gfx_rect(x, y, 20, 16, got ? C_WHITE : C_NIGHT);
        if (got) gfx_rect(x + 2, y + 2, 16, 10, k % 2 ? C_SKY : k == 0 ? C_PINK : k == 2 ? C_PURPLE : C_JADE);
    }
}

static void draw_ending(void) {
    gfx_cls(sb.true_won ? C_PURPLE : C_NAVY);
    int t = sb.state_t;
    for (int i = 0; i < 50; i++) gfx_pset((i * 71 + t) % SCREEN_W, (i * 37) % 180, i % 5 ? C_DUSK : C_WHITE);
    spr_draw_scaled(&shb_spr[SP_POPPY], 130, 70 + (int)(sinf(t * 0.04f) * 4), 2, 0);
    if (sb.players == 2) spr_draw_scaled(&shb_spr[SP_SPRIG], 90, 90 + (int)(sinf(t * 0.04f + 1) * 4), 2, 0);
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER};
    if (sb.true_won) {
        ui_fancy_center("THE TRUE END", 160, 16, 2, grad, 3, C_INK, C_MAROON);
        text_center("THE KALEIDOSCOPE BROKE INTO A THOUSAND STARS,", 160, 120, C_LIGHT);
        text_center("AND POPPY GOT THE PICTURE OF A LIFETIME.", 160, 132, C_LIGHT);
    } else {
        ui_fancy_center("THE END", 160, 16, 2, grad, 3, C_INK, C_MAROON);
        text_center("POPPY FLEW HOME WITH A WHOLE ALBUM OF PICTURES.", 160, 120, C_LIGHT);
        if (t > 240) {
            text_center("BUT THE ALBUM STILL HAS AN EMPTY PAGE...", 160, 138, C_PINK);
            text_center("SNAP U, F AND O, AND SEE WHAT WAITS BEYOND.", 160, 150, C_PINK);
        }
    }
    char buf[48];
    snprintf(buf, sizeof buf, "FINAL SCORE  %s", num(sb.best));
    text_center(buf, 160, 40, C_WHITE);
}

/* The roll call: every foe by name, in the order they turn up; its
 * picture shows only if it was photographed this run. */
const uint8_t SHB_CAST[] = {
    K_PUFF, K_MINT, K_CUBE, K_TOAST, K_TURRET, K_WALLBOMB, K_CHOMPER, K_SCONE, K_TEAPOT,
    K_SWIRL, K_ERUPTER, K_ROCKBIG, K_ROCK, K_BURSTER,
    K_GHOST, K_CART, K_RAILBOMB, K_SILO, K_ROCKET, K_CROAK, K_SIGNAL,
    K_DART, K_HOPPER,
    K_NIP, K_KITE, K_LASER, K_GEN, K_JAW, K_CHUTE, K_KALEI, K_SHARD,
};
const int SHB_CAST_N = (int)sizeof SHB_CAST;

bool shb_kind_photographed(int kind) {
    return kind < 32 ? (sb.kinds >> kind) & 1 : (sb.kinds2 >> (kind - 32)) & 1;
}

static const char *const CRED_TOP[] = {
    "SHUTTERBUG", "", "A BEAMDOWN SOFTWORKS GAME", "", "STARRING", "POPPY", "AND SPRIG", "",
    "THE SIGHTS", "TEATIME PLANET", "COMET RAIN", "GLOOM PLANET", "FOSSIL PLANET", "", "", "POSING FOR PICTURES", "",
};
static const char *const CRED_END[] = {
    "", "A TRIBUTE TO CARAMEL CARAMEL", "(UFO 50 #24, MOSSMOUTH)", "", "", "THANKS FOR LOOKING!",
};
#define CAST_ROW 40
#define CRED_LINE 12

int shb_credits_len(void) {
    return ARRAY_LEN(CRED_TOP) * CRED_LINE + SHB_CAST_N * CAST_ROW + ARRAY_LEN(CRED_END) * CRED_LINE;
}

/* one cast member: a snapshot (or an empty frame) and the name */
static void draw_cast(int kind, int y) {
    int fx = 92, fw = 52, fh = 34;
    gfx_rect(fx - 2, y - 2, fw + 4, fh + 4, C_WHITE);
    gfx_rect(fx, y, fw, fh, C_NIGHT);
    if (shb_kind_photographed(kind)) {
        Foe f;
        memset(&f, 0, sizeof f);
        f.kind = (uint8_t)kind;
        f.alive = 1;
        f.role = SHB_FOE[kind].role;
        f.hw = SHB_FOE[kind].hw;
        f.hh = kind == K_LASER ? 14 : SHB_FOE[kind].hh;
        f.state = 1;
        f.arg = kind == K_LASER ? 90 : 0;
        f.x = sb.cam_x + fx + fw / 2;
        f.y = sb.cam_y + y + fh / 2 - SHB_PF_Y;
        f.ay = f.y;
        gfx_clip(fx, y, fw, fh);
        gfx_rect(fx, y, fw, fh, C_SKY);
        draw_foe(&f);
        gfx_noclip();
    } else {
        text_center("?", fx + fw / 2, y + fh / 2 - 3, C_DUSK);
    }
    text_draw(SHB_FOE[kind].name, fx + fw + 12, y + fh / 2 - 3, shb_kind_photographed(kind) ? C_WHITE : C_GREY);
}

static void draw_credits(void) {
    gfx_cls(C_INK);
    int y = SCREEN_H - sb.state_t / 2;
    for (int i = 0; i < ARRAY_LEN(CRED_TOP); i++, y += CRED_LINE)
        if (y > -10 && y < SCREEN_H) text_center(CRED_TOP[i], 160, y, i == 0 ? C_PINK : C_LIGHT);
    for (int i = 0; i < SHB_CAST_N; i++, y += CAST_ROW)
        if (y > -40 && y < SCREEN_H) draw_cast(SHB_CAST[i], y);
    for (int i = 0; i < ARRAY_LEN(CRED_END); i++, y += CRED_LINE)
        if (y > -10 && y < SCREEN_H) text_center(CRED_END[i], 160, y, C_LIGHT);
}

void shb_draw(void) {
    gfx_camera(0, 0);
    gfx_noclip();
    switch (sb.state) {
    case SS_TITLE: draw_title(); break;
    case SS_STORY: draw_story(); break;
    case SS_BANNER: draw_banner(); break;
    case SS_PLAY: draw_play(); break;
    case SS_LOST: draw_lost(); break;
    case SS_OVER: draw_over(); break;
    case SS_CLEAR: draw_clear(); break;
    case SS_ENDING: draw_ending(); break;
    case SS_CREDITS: draw_credits(); break;
    default: draw_play(); break;
    }
}

/* the cartridge's label in the library */
void shb_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_SKY);
    for (int k = 0; k < 4; k++) {
        int cx = x + ((k * 47 - t / 2) % (w + 30) + w + 30) % (w + 30) - 15, cy = y + 8 + k * 9 % 20;
        gfx_circ(cx, cy, 5, C_WHITE);
        gfx_circ(cx + 6, cy + 1, 4, C_WHITE);
    }
    gfx_rect(x, y + h - 10, w, 10, C_TAN);
    gfx_hline(x, x + w - 1, y + h - 10, C_PINK);
    int px = x + 16 + (int)(sinf(t * 0.04f) * 4), py = y + h / 2 - 2;
    spr_draw(&shb_spr[SP_POPPY], px, py, 0);
    int fx = px + 40, fy = py - 8;
    bool snap = (t / 40) % 3 == 0 && t % 40 < 5;
    if (snap) gfx_dither(fx, fy, 30, 24, C_WHITE, 10);
    gfx_rectb(fx, fy, 30, 24, C_WHITE);
    spr_draw(&shb_spr[SP_CUBE], fx + 10, fy + 7, 0);
    gfx_noclip();
}
