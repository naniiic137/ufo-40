/* TILTSHOT - drawing the course: the sky and its far scenery for each look,
 * the ground (painted once per hole into an off-screen strip), water, the
 * cup and flag, bumpers, pegs, springs, junk and movers. */
#include "tiltshot.h"

typedef struct Theme {
    uint8_t sky[4];
    uint8_t top[3];
    uint8_t dirt[2];
    uint8_t edge;
    uint8_t far;
} Theme;

static const Theme THEMES[TH_COUNT] = {
    /* meadow */ {{C_BLUE, C_SKY, C_SKY, C_ICE}, {C_LIME, C_LEAF, C_FOREST}, {C_BROWN, C_EARTH}, C_NIGHT, C_JADE},
    /* dusk   */ {{C_PURPLE, C_MAGENTA, C_ORANGE, C_AMBER}, {C_YELLOW, C_AMBER, C_ORANGE}, {C_BROWN, C_MAROON}, C_INK, C_WINE},
    /* night  */ {{C_INK, C_NIGHT, C_NAVY, C_DUSK}, {C_LIME, C_JADE, C_TEAL}, {C_DUSK, C_SLATE}, C_INK, C_NIGHT},
    /* ice    */ {{C_BLUE, C_SKY, C_SKY, C_ICE}, {C_WHITE, C_ICE, C_CYAN}, {C_LIGHT, C_ICE}, C_SLATE, C_WHITE},
    /* beach  */ {{C_BLUE, C_SKY, C_SKY, C_ICE}, {C_LIME, C_LEAF, C_JADE}, {C_TAN, C_HIDE}, C_BROWN, C_BLUE},
    /* temple */ {{C_NIGHT, C_PURPLE, C_VIOLET, C_PINK}, {C_LEAF, C_FOREST, C_TEAL}, {C_GREY, C_SLATE}, C_INK, C_PURPLE},
    /* fair   */ {{C_NAVY, C_PURPLE, C_MAGENTA, C_PINK}, {C_PINK, C_MAGENTA, C_WINE}, {C_VIOLET, C_PURPLE}, C_INK, C_NIGHT},
};

static Surface terrain;
static int terrain_hole = -2;

static uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}

static bool solid_at(const TshCourse *c, int x, int y) {
    if (y < 0 || y >= TSH_VIEW_H || x < 0 || x >= c->w) return false;
    char ch = c->tile[y / TSH_T][x / TSH_T];
    return tsh_tile_px(ch, x % TSH_T, y % TSH_T);
}

/* paint the ground of a hole into the terrain strip (TRANSPARENT = open) */
void tsh_terrain_build(const TshCourse *c) {
    if (terrain_hole == c->hole && terrain.px) return;
    free(terrain.px);
    terrain.w = c->w;
    terrain.h = TSH_VIEW_H;
    terrain.px = malloc((size_t)terrain.w * terrain.h);
    if (!terrain.px) { terrain_hole = -2; return; }
    memset(terrain.px, TRANSPARENT, (size_t)terrain.w * terrain.h);
    terrain_hole = c->hole;
    const Theme *th = &THEMES[c->theme];
    for (int x = 0; x < c->w; x++) {
        int depth = 0;
        bool above = false;
        for (int y = 0; y < TSH_VIEW_H; y++) {
            bool s = solid_at(c, x, y);
            if (!s) { above = false; continue; }
            depth = above ? depth + 1 : 0;
            above = true;
            char ch = c->tile[y / TSH_T][x / TSH_T];
            bool under = !solid_at(c, x, y + 1) && y + 1 < TSH_VIEW_H;
            bool side = !solid_at(c, x - 1, y) || !solid_at(c, x + 1, y);
            uint32_t h = hash2(x, y);
            uint8_t col;
            switch (ch) {
            case 's':
                col = depth == 0 ? C_CREAM : (h % 13 == 0 ? C_CREAM : (h % 17 == 0 ? C_AMBER : C_TAN));
                break;
            case 'T':
                if (depth <= 1) col = ((x / 3) & 1) ? C_WHITE : C_RED;
                else col = (x % 8 == 3 && y % 8 == 4) ? C_SLATE : C_GREY;
                break;
            case 'X':
                if (depth == 0) col = C_LIGHT;
                else col = (y % 4 == 0 || (x + ((y / 4) & 1) * 4) % 8 == 0) ? C_SLATE : C_GREY;
                break;
            case 'I':
                col = depth == 0 ? C_WHITE : ((x + y) % 7 == 0 ? C_WHITE : ((x - y) % 11 == 0 ? C_CYAN : C_ICE));
                break;
            case 'R': {
                int lx = x % TSH_T, ly = y % TSH_T;
                col = (lx == 0 || ly == 0) ? C_ORANGE : (lx == 7 || ly == 7) ? C_MAROON : C_RED;
                break;
            }
            default:
                if (depth == 0) col = th->top[0];
                else if (depth <= 2) col = th->top[1];
                else if (depth == 3) col = th->top[2];
                else if (c->theme == TH_TEMPLE) col = (y % 5 == 0 || (x + ((y / 5) & 1) * 5) % 10 == 0) ? th->dirt[1] : th->dirt[0];
                else if (c->theme == TH_FAIR) col = ((x + y) / 6) & 1 ? th->dirt[0] : th->dirt[1];
                else col = ((y + (x / 16) % 3) % 7 == 0 || h % 19 == 0) ? th->dirt[1] : th->dirt[0];
                break;
            }
            if ((under || (side && depth > 2)) && ch != 'R') col = th->edge;
            terrain.px[y * terrain.w + x] = col;
        }
    }
}

/* ---- the sky ------------------------------------------------------------------ */

void tsh_draw_sky(int theme, int cam, int t) {
    const Theme *th = &THEMES[theme];
    gfx_camera(0, 0);
    for (int b = 0; b < 4; b++) {
        int y0 = b * 40;
        gfx_rect(0, y0, SCREEN_W, 40, th->sky[b]);
        if (b < 3) gfx_dither(0, y0 + 32, SCREEN_W, 8, th->sky[b + 1], 6);
    }
    int px = cam / 4;
    switch (theme) {
    case TH_NIGHT:
    case TH_TEMPLE:
    case TH_FAIR:
        for (int i = 0; i < 60; i++) {
            uint32_t h = hash2(i, 7);
            int sx = (int)((h % 640) - px / 2 + 640) % 640, sy = (int)((h >> 10) % 90);
            if (sx < SCREEN_W) gfx_pset(sx, sy, ((h >> 20) + (uint32_t)t / 20) % 5 ? C_LIGHT : C_WHITE);
        }
        break;
    default: break;
    }
    switch (theme) {
    case TH_MEADOW:
        for (int i = 0; i < 5; i++) {
            int cx = (int)((hash2(i, 3) % 700) + t / 8 - px / 2) % 700 - 60, cy = 12 + (int)(hash2(i, 4) % 40);
            gfx_circ(cx, cy, 8, C_WHITE);
            gfx_circ(cx + 9, cy + 2, 6, C_WHITE);
            gfx_circ(cx - 8, cy + 3, 5, C_WHITE);
            gfx_rect(cx - 12, cy + 3, 26, 5, C_WHITE);
        }
        for (int x = 0; x < SCREEN_W; x++) {
            float a = (x + px) / 38.0f;
            int hgt = 22 + (int)(10 * sinf(a) + 6 * sinf(a * 2.3f + 1));
            gfx_vline(x, 150 - hgt, 159, th->far);
        }
        break;
    case TH_DUSK:
        gfx_circ(250 - px / 8 % 40, 104, 22, C_YELLOW);
        gfx_dither(200, 80, 110, 40, C_ORANGE, 4);
        for (int x = 0; x < SCREEN_W; x++) {
            int u = (x + px) % 160;
            int hgt = u < 50 ? 34 : u < 60 ? 34 - (u - 50) * 3 : u < 110 ? 8 : u < 118 ? 8 + (u - 110) * 3 : 32;
            gfx_vline(x, 150 - hgt, 159, th->far);
        }
        break;
    case TH_NIGHT:
        gfx_circ(80 - px / 10 % 30, 40, 14, C_VIOLET);
        gfx_circ(76 - px / 10 % 30, 36, 11, C_PURPLE);
        gfx_line(52 - px / 10 % 30, 46, 108 - px / 10 % 30, 34, C_PINK);
        for (int x = 0; x < SCREEN_W; x++) {
            int u = (x + px) % 90;
            int hgt = 14 + (u < 45 ? u : 90 - u) / 2;
            gfx_vline(x, 150 - hgt, 159, th->far);
        }
        break;
    case TH_ICE:
        for (int x = 0; x < SCREEN_W; x++) {
            int u = (x + px) % 120;
            int hgt = 10 + (u < 60 ? u : 120 - u);
            gfx_vline(x, 150 - hgt, 159, u % 60 > 50 || u % 60 < 10 ? C_WHITE : th->far);
        }
        for (int i = 0; i < 30; i++) {
            uint32_t h = hash2(i, 9);
            int sx = (int)(h % 330 + (uint32_t)t / 3) % 330, sy = (int)((h >> 9) % 160 + (uint32_t)t / 2) % 160;
            gfx_pset(sx, sy, C_WHITE);
        }
        break;
    case TH_BEACH:
        gfx_circ(60, 30, 12, C_YELLOW);
        gfx_rect(0, 118, SCREEN_W, 42, th->far);
        for (int i = 0; i < 24; i++) {
            uint32_t h = hash2(i, 11);
            int sx = (int)((h % 340) + (uint32_t)t / 6 - px / 3 + 340) % 340, sy = 120 + (int)((h >> 8) % 30);
            gfx_hline(sx, sx + 4, sy, (t / 16 + i) % 3 ? C_SKY : C_WHITE);
        }
        break;
    case TH_TEMPLE:
        gfx_circ(260, 34, 12, C_CREAM);
        for (int k = 0; k < 3; k++) {
            int bx = (int)((k * 140 + 40 - px / 2) % 420 + 420) % 420 - 50;
            for (int s = 0; s < 4; s++) {
                int w = 44 - s * 10, y = 150 - s * 16;
                gfx_rect(bx - w / 2, y - 10, w, 10, th->far);
                gfx_rect(bx - w / 2 - 4, y - 12, w + 8, 3, th->far);
            }
        }
        break;
    case TH_FAIR: {
        int wx = (int)((200 - px / 2) % 400 + 400) % 400 - 40;
        gfx_circb(wx, 80, 50, th->far);
        gfx_circb(wx, 80, 49, th->far);
        for (int k = 0; k < 8; k++) {
            float a = k * 0.785f + t * 0.004f;
            int ex = wx + (int)(cosf(a) * 50), ey = 80 + (int)(sinf(a) * 50);
            gfx_line(wx, 80, ex, ey, th->far);
            gfx_rect(ex - 3, ey, 7, 5, th->far);
        }
        gfx_line(wx, 80, wx - 20, 159, th->far);
        gfx_line(wx, 80, wx + 20, 159, th->far);
        for (int x = 0; x < SCREEN_W; x += 2) {
            int u = (x + px) % 64;
            gfx_pset(x, 20 + (u < 32 ? u / 4 : (64 - u) / 4), C_YELLOW);
        }
        break;
    }
    default: break;
    }
}

/* ---- the course ------------------------------------------------------------------ */

static void draw_water(const TshCourse *c, int cam, int t) {
    int c0 = imax(0, cam / TSH_T), c1 = imin(c->cols - 1, (cam + SCREEN_W) / TSH_T + 1);
    for (int col = c0; col <= c1; col++)
        for (int r = 0; r < TSH_ROWS; r++) {
            if (c->tile[r][col] != '~') continue;
            int x = col * TSH_T, y = r * TSH_T;
            bool top = r == 0 || c->tile[r - 1][col] != '~';
            gfx_rect(x, y, TSH_T, TSH_T, C_BLUE);
            if (!top) gfx_dither(x, y, TSH_T, TSH_T, C_NAVY, 4);
            if (top) {
                for (int k = 0; k < TSH_T; k++) {
                    int wy = ((x + k + t / 4) / 4) & 1;
                    gfx_pset(x + k, y + wy, C_ICE);
                    gfx_pset(x + k, y + 1 + wy, C_SKY);
                    if (!wy) gfx_pset(x + k, y, C_SKY);
                }
            }
        }
}

void tsh_draw_mover(const TshMoverDef *m, float x, float y, int t) {
    int ix = (int)lroundf(x), iy = (int)lroundf(y);
    int f = (t / 8) & 1;
    switch (m->kind) {
    case MV_BLIMP: spr_draw(&tsh_spr[f ? TS_BLIMP2 : TS_BLIMP1], ix - 6, iy - 4, m->ax && sinf((t + m->phase) * 6.2831853f / (m->period ? m->period : 1)) > 0 ? SPR_FLIPX : 0); break;
    case MV_HOPPER: spr_draw(&tsh_spr[f ? TS_HOPPER2 : TS_HOPPER1], ix - 5, f ? iy - 2 : iy - 5, 0); break;
    case MV_KITE:
        spr_draw(&tsh_spr[TS_KITE1], ix - 4, iy - 4, 0);
        for (int k = 1; k < 6; k++) gfx_pset(ix + (int)(sinf((t + k * 9) * 0.1f) * 2), iy + 4 + k * 2, k % 2 ? C_RED : C_YELLOW);
        break;
    case MV_FISH: spr_draw(&tsh_spr[f ? TS_FISH2 : TS_FISH1], ix - 5, iy - 3, 0); break;
    case MV_ROLLER: {
        int per = m->period ? m->period : 1;
        bool back = (t + m->phase) % per >= per / 2;
        spr_draw(&tsh_spr[(t / 5) & 1 ? TS_ROLLER2 : TS_ROLLER1], ix - 6, iy - 5, back ? SPR_FLIPX : 0);
        break;
    }
    case MV_SWING: {
        /* the chain from the pivot, then the lantern */
        int n = imax(1, m->ay / 3);
        for (int k = 0; k < n; k++) {
            int cx = m->cx + (ix - m->cx) * k / n, cy = m->cy + (iy - 5 - m->cy) * k / n;
            gfx_pset(cx, cy, k & 1 ? C_GREY : C_LIGHT);
        }
        gfx_rect(m->cx - 2, m->cy - 2, 5, 2, C_SLATE);
        spr_draw(&tsh_spr[TS_LANTERN], ix - 4, iy - 5, 0);
        break;
    }
    default: spr_draw(&tsh_spr[f ? TS_SPARK2 : TS_SPARK1], ix - 5, iy - 4, 0); break;
    }
}

/* everything on the course but the ball and the golfer; world space, the
 * camera already set to (cam, 0) */
void tsh_draw_course(const TshCourse *c, const TshPlay *p, int cam, int t, const uint8_t *flash) {
    /* the ground strip */
    gfx_camera(0, 0);
    gfx_clip(0, 0, SCREEN_W, TSH_VIEW_H);
    if (terrain.px) {
        for (int y = 0; y < TSH_VIEW_H; y++) {
            const uint8_t *src = terrain.px + y * terrain.w;
            uint8_t *dst = g_screen.px + y * SCREEN_W;
            for (int x = 0; x < SCREEN_W; x++) {
                int wx = x + cam;
                if (wx < 0 || wx >= terrain.w) continue;
                uint8_t col = src[wx];
                if (col != TRANSPARENT) dst[x] = col;
            }
        }
    }
    gfx_camera(cam, 0);
    draw_water(c, cam, t);
    /* the cup: a dark mouth and a flag */
    int ux = c->cup_col * TSH_T, uy = c->cup_row * TSH_T;
    gfx_rect(ux, uy, TSH_T, TSH_T, C_INK);
    gfx_vline(ux + 4, uy - 22, uy + 6, C_LIGHT);
    spr_draw(&tsh_spr[(t / 12) & 1 ? TS_FLAG2 : TS_FLAG1], ux + 5, uy - 22, 0);
    /* spring lines */
    const TshHoleDef *d = &TSH_HOLE[c->hole];
    for (int i = 0; i < d->nsprings; i++) {
        const TshSpringDef *s = &d->springs[i];
        for (int e = 0; e < 2; e++) {
            /* a coil under each end */
            int ex = e ? s->x1 : s->x0, ey = e ? s->y1 : s->y0;
            for (int k = 0; k < 4; k++) gfx_hline(ex - 2 + (k & 1), ex + 1 + (k & 1), ey + 3 + k * 2, C_GREY);
        }
        gfx_line(s->x0, s->y0 + 2, s->x1, s->y1 + 2, C_MAROON);
        gfx_line(s->x0, s->y0 + 1, s->x1, s->y1 + 1, C_RED);
        gfx_line(s->x0, s->y0, s->x1, s->y1, ((t / 6) & 1) ? C_WHITE : C_PINK);
        gfx_circ(s->x0, s->y0, 1, C_LIGHT);
        gfx_circ(s->x1, s->y1, 1, C_LIGHT);
    }
    /* bumpers and pegs */
    for (int i = 0; i < c->ncirc; i++) {
        const TshCirc *q = &c->circ[i];
        int x = (int)q->x, y = (int)q->y, r = (int)q->r;
        if (q->kind == TC_PEG) {
            gfx_rect(x - 1, y - 2, 3, 5, C_LIGHT);
            gfx_pset(x + 1, y - 2, C_WHITE);
            gfx_hline(x - 1, x + 1, y + 2, C_GREY);
            continue;
        }
        bool lit = flash && flash[i] > 0;
        gfx_circ(x, y, r + 1, C_INK);
        gfx_circ(x, y, r, lit ? C_WHITE : C_CYAN);
        gfx_circ(x, y, r - 2, lit ? C_YELLOW : C_BLUE);
        gfx_circ(x, y, imax(1, r - 4), lit ? C_WHITE : C_SKY);
        gfx_pset(x - r / 2, y - r / 2, C_WHITE);
    }
    /* junk */
    for (int i = 0; i < c->njunk; i++) {
        if (p && (p->junk_gone & (1u << i))) continue;
        static const int SP[TJ_KINDS] = {TS_CRATE, TS_CONE, TS_CHURN, TS_BUCKET};
        spr_draw(&tsh_spr[SP[c->junk[i].kind]], (int)c->junk[i].x - 4, (int)c->junk[i].y - 4, 0);
    }
    /* movers */
    int clock = p ? p->clock : t;
    for (int i = 0; i < c->nmover; i++) {
        if (p && (p->mover_gone & (1u << i))) continue;
        float mx, my;
        if (tsh_mover_pos(&c->mover[i], clock, &mx, &my)) tsh_draw_mover(&c->mover[i], mx, my, clock);
    }
    gfx_noclip();
}

/* the hole seen whole, one pixel a tile (the hole card) */
void tsh_draw_outline(const TshCourse *c, int x, int y, int scale) {
    for (int r = 0; r < TSH_ROWS; r++)
        for (int col = 0; col < c->cols; col++) {
            char ch = c->tile[r][col];
            int colr = -1;
            if (ch == '~') colr = C_BLUE;
            else if (ch == 's') colr = C_TAN;
            else if (ch == 'T') colr = C_RED;
            else if (ch == 'U') colr = C_WHITE;
            else if (tsh_tile_solid(ch)) colr = (r > 0 && tsh_tile_solid(c->tile[r - 1][col])) ? C_FOREST : C_LIME;
            if (colr >= 0) gfx_rect(x + col * scale, y + r * scale, scale, scale, colr);
        }
    for (int i = 0; i < c->ncirc; i++)
        if (c->circ[i].kind != TC_PEG) gfx_rect(x + (int)c->circ[i].x / TSH_T * scale, y + (int)c->circ[i].y / TSH_T * scale, scale, scale, C_CYAN);
    int tx = (int)c->tee_x / TSH_T, ty = (int)c->tee_y / TSH_T;
    gfx_rect(x + tx * scale, y + ty * scale, scale, scale, C_YELLOW);
    gfx_vline(x + c->cup_col * scale + scale / 2, y + (c->cup_row - 3) * scale, y + c->cup_row * scale, C_RED);
}
