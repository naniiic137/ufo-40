/* TURNIP TRUCK - drawing a workday: Blipton from above (north is always
 * up), the truck, the traffic, the chaos, and the dashboard on the left with
 * the clock, the hearts, the tries, the count, the radar and where to go. */
#include "tnp.h"

int tnp_hud_tries, tnp_hud_week, tnp_hud_best;
static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};

/* ---- the city's look ---------------------------------------------------------------- */

static uint8_t block_col[TNP_MAP][TNP_MAP], block_style[TNP_MAP][TNP_MAP];
static bool blocks_done;

static uint32_t hash2(int x, int y) {
    uint32_t h = ((uint32_t)x * 73856093u) ^ ((uint32_t)y * 19349663u);
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}

/* every block of buildings gets one roof colour */
static void label_blocks(void) {
    static const uint8_t ROOFS[9][3] = {
        {C_WINE, C_BROWN, C_TAN},      /* Old Blipton: brick and tile */
        {C_SLATE, C_TEAL, C_WINE},     /* Depot Row */
        {C_PURPLE, C_NAVY, C_TEAL},    /* Saucer Heights */
        {C_TAN, C_BROWN, C_WINE},      /* Westgreen */
        {C_NAVY, C_SLATE, C_BLUE},     /* Downtown */
        {C_SLATE, C_SLATE, C_SLATE},   /* the canal */
        {C_SLATE, C_BROWN, C_TEAL},    /* the Pickle Works */
        {C_WINE, C_TAN, C_PURPLE},     /* the market */
        {C_TEAL, C_SLATE, C_NAVY},     /* the docks */
    };
    static uint8_t seen[TNP_MAP][TNP_MAP];
    static int16_t qx[TNP_MAP * TNP_MAP], qy[TNP_MAP * TNP_MAP];
    memset(seen, 0, sizeof seen);
    for (int y = 0; y < TNP_MAP; y++)
        for (int x = 0; x < TNP_MAP; x++) {
            if (seen[y][x] || tnp_map[y][x].type != CT_BUILD) continue;
            uint32_t h = hash2(x, y);
            uint8_t col = ROOFS[tnp_map[y][x].district][h % 3];
            uint8_t style = (uint8_t)((h >> 8) % 4);
            int n = 0;
            qx[n] = (int16_t)x;
            qy[n++] = (int16_t)y;
            seen[y][x] = 1;
            for (int i = 0; i < n; i++) {
                int cx = qx[i], cy = qy[i];
                block_col[cy][cx] = col;
                block_style[cy][cx] = style;
                for (int k = 0; k < 4; k++) {
                    int nx = tnp_wrapc(cx + DX[k]), ny = tnp_wrapc(cy + DY[k]);
                    if (seen[ny][nx] || tnp_map[ny][nx].type != CT_BUILD) continue;
                    seen[ny][nx] = 1;
                    qx[n] = (int16_t)nx;
                    qy[n++] = (int16_t)ny;
                }
            }
        }
    blocks_done = true;
}

static bool open_side(int cx, int cy) {
    int t = tnp_cell(cx, cy)->type;
    return t == CT_ROAD || t == CT_BRIDGE || t == CT_LOT || t == CT_ROCK;
}

static void draw_road(int sx, int sy, int cx, int cy, int t) {
    gfx_rect(sx, sy, TNP_CELL, TNP_CELL, C_DUSK);
    /* kerbs and pavements where the road meets anything else */
    bool o[4];
    for (int k = 0; k < 4; k++) o[k] = open_side(cx + DX[k], cy + DY[k]) || tnp_cell(cx + DX[k], cy + DY[k])->type == CT_BRINE;
    if (!o[DIR_N]) { gfx_rect(sx, sy, TNP_CELL, 4, C_GREY); gfx_hline(sx, sx + TNP_CELL - 1, sy + 4, C_SLATE); }
    if (!o[DIR_S]) { gfx_rect(sx, sy + TNP_CELL - 4, TNP_CELL, 4, C_GREY); gfx_hline(sx, sx + TNP_CELL - 1, sy + TNP_CELL - 5, C_SLATE); }
    if (!o[DIR_W]) { gfx_rect(sx, sy, 4, TNP_CELL, C_GREY); gfx_vline(sx + 4, sy, sy + TNP_CELL - 1, C_SLATE); }
    if (!o[DIR_E]) { gfx_rect(sx + TNP_CELL - 4, sy, 4, TNP_CELL, C_GREY); gfx_vline(sx + TNP_CELL - 5, sy, sy + TNP_CELL - 1, C_SLATE); }
    /* the centre line on a straight */
    bool horiz = o[DIR_E] && o[DIR_W] && !o[DIR_N] && !o[DIR_S];
    bool vert = o[DIR_N] && o[DIR_S] && !o[DIR_E] && !o[DIR_W];
    if (horiz)
        for (int i = 0; i < TNP_CELL; i += 12) gfx_rect(sx + i + 2, sy + TNP_CELL / 2 - 1, 6, 2, C_AMBER);
    if (vert)
        for (int i = 0; i < TNP_CELL; i += 12) gfx_rect(sx + TNP_CELL / 2 - 1, sy + i + 2, 2, 6, C_AMBER);
    /* a warning painted on the road before a bridge */
    for (int k = 0; k < 4; k++)
        if (tnp_cell(cx + DX[k], cy + DY[k])->type == CT_BRIDGE && tnp_cell(cx, cy)->type == CT_ROAD) {
            tiny_center("BRIDGE", sx + TNP_CELL / 2, sy + TNP_CELL / 2 + (k == DIR_N ? -14 : 8), C_AMBER);
            break;
        }
    /* the odd crack and manhole */
    uint32_t h = hash2(cx, cy);
    if (h % 5 == 0) gfx_circb(sx + 10 + (int)(h >> 4) % 28, sy + 10 + (int)(h >> 9) % 28, 3, C_NIGHT);
    if (h % 7 == 1) gfx_line(sx + 8, sy + 30, sx + 15, sy + 34, C_NIGHT);
    (void)t;
}

static void draw_building(int sx, int sy, int cx, int cy) {
    int col = block_col[tnp_wrapc(cy)][tnp_wrapc(cx)];
    int dark = PAL_DARKER[col], light = PAL_LIGHTER[col];
    gfx_rect(sx, sy, TNP_CELL, TNP_CELL, col);
    bool b[4];
    for (int k = 0; k < 4; k++) b[k] = tnp_cell(cx + DX[k], cy + DY[k])->type == CT_BUILD;
    /* roof rims where the block ends */
    if (!b[DIR_N]) { gfx_rect(sx, sy, TNP_CELL, 3, light); gfx_hline(sx, sx + TNP_CELL - 1, sy, C_INK); }
    if (!b[DIR_W]) { gfx_rect(sx, sy, 3, TNP_CELL, light); gfx_vline(sx, sy, sy + TNP_CELL - 1, C_INK); }
    if (!b[DIR_E]) { gfx_rect(sx + TNP_CELL - 3, sy, 3, TNP_CELL, dark); gfx_vline(sx + TNP_CELL - 1, sy, sy + TNP_CELL - 1, C_INK); }
    if (!b[DIR_S]) {
        /* the wall below the roof, seen a little from the south */
        gfx_rect(sx, sy + TNP_CELL - 9, TNP_CELL, 9, dark);
        gfx_hline(sx, sx + TNP_CELL - 1, sy + TNP_CELL - 9, C_INK);
        for (int i = 4; i < TNP_CELL - 4; i += 9) gfx_rect(sx + i, sy + TNP_CELL - 7, 4, 4, (hash2(cx * 3 + i, cy) & 3) ? C_NIGHT : C_YELLOW);
        gfx_hline(sx, sx + TNP_CELL - 1, sy + TNP_CELL - 1, C_INK);
    }
    uint32_t h = hash2(cx, cy);
    switch (block_style[tnp_wrapc(cy)][tnp_wrapc(cx)]) {
    case 0: /* vents */
        gfx_rect(sx + 10 + (int)(h % 14), sy + 10 + (int)((h >> 5) % 12), 8, 6, C_GREY);
        gfx_rectb(sx + 10 + (int)(h % 14), sy + 10 + (int)((h >> 5) % 12), 8, 6, C_SLATE);
        break;
    case 1: /* skylights */
        for (int i = 0; i < 3; i++) gfx_rect(sx + 9 + i * 11, sy + 14, 6, 10, C_SKY);
        break;
    case 2: /* a water tank */
        gfx_circ(sx + 24, sy + 20, 7, C_BROWN);
        gfx_circb(sx + 24, sy + 20, 7, C_INK);
        gfx_hline(sx + 18, sx + 30, sy + 20, C_TAN);
        break;
    default: /* tar patches */
        gfx_dither(sx + 6, sy + 6, 30, 22, dark, 6);
        break;
    }
}

static void draw_grass(int sx, int sy, int cx, int cy) {
    gfx_rect(sx, sy, TNP_CELL, TNP_CELL, C_FOREST);
    for (int i = 0; i < 18; i++) {
        uint32_t h = hash2(cx * 31 + i, cy * 17 - i);
        gfx_pset(sx + (int)(h % TNP_CELL), sy + (int)((h >> 8) % TNP_CELL), (h >> 20) & 1 ? C_JADE : C_LEAF);
    }
}

static void draw_brine(int sx, int sy, int cx, int cy, int t) {
    gfx_rect(sx, sy, TNP_CELL, TNP_CELL, C_LEAF);
    for (int r = 0; r < 4; r++) {
        int yy = sy + 6 + r * 12;
        int off = (t / 4 + r * 7 + cx * 11) % 16;
        for (int x = -16; x < TNP_CELL; x += 16) gfx_hline(sx + x + off, sx + x + off + 6, yy, C_LIME);
    }
    /* banks */
    for (int k = 0; k < 4; k++) {
        int n = tnp_cell(cx + DX[k], cy + DY[k])->type;
        if (n == CT_BRINE || n == CT_BRIDGE) continue;
        if (k == DIR_N) gfx_rect(sx, sy, TNP_CELL, 2, C_FOREST);
        if (k == DIR_S) gfx_rect(sx, sy + TNP_CELL - 2, TNP_CELL, 2, C_FOREST);
        if (k == DIR_W) gfx_rect(sx, sy, 2, TNP_CELL, C_FOREST);
        if (k == DIR_E) gfx_rect(sx + TNP_CELL - 2, sy, 2, TNP_CELL, C_FOREST);
    }
}

static void draw_rock(int sx, int sy, int cx, int cy) {
    gfx_rect(sx, sy, TNP_CELL, TNP_CELL, C_TAN);
    for (int i = 0; i < 14; i++) {
        uint32_t h = hash2(cx * 13 + i, cy * 29 + i * 3);
        int x = sx + (int)(h % (TNP_CELL - 4)), y = sy + (int)((h >> 8) % (TNP_CELL - 4));
        gfx_rect(x, y, 3 + (int)((h >> 16) & 1), 2 + (int)((h >> 17) & 1), (h >> 18) & 1 ? C_BROWN : C_EARTH);
    }
}

static void draw_lot(int sx, int sy) {
    gfx_rect(sx, sy, TNP_CELL, TNP_CELL, C_SLATE);
    for (int i = 6; i < TNP_CELL; i += 14) gfx_vline(sx + i, sy + 2, sy + 14, C_LIGHT);
}

static void draw_fence(int sx, int sy, int cx, int cy) {
    draw_lot(sx, sy);
    gfx_dither(sx, sy, TNP_CELL, TNP_CELL, C_GREY, 5);
    for (int i = 0; i < TNP_CELL; i += 12) {
        gfx_rect(sx + i, sy, 2, TNP_CELL, C_LIGHT);
        gfx_rect(sx, sy + i, TNP_CELL, 2, C_LIGHT);
    }
    (void)cx;
    (void)cy;
}

static void draw_feature(const TnpDay *d, int sx, int sy, int cx, int cy, int t) {
    const TnpCell *c = tnp_cell(cx, cy);
    int mx = sx + TNP_CELL / 2, my = sy + TNP_CELL / 2;
    switch (c->feat) {
    case FT_DROP:
        gfx_circb(mx, my, 13, C_LIGHT);
        gfx_circb(mx, my, 12, C_LIGHT);
        gfx_rect(mx - 6, my - 1, 13, 3, C_LIGHT);
        gfx_rect(mx - 1, my - 6, 3, 13, C_LIGHT);
        break;
    case FT_PIPE: {
        gfx_circ(mx + 1, my + 2, 6, C_NIGHT);
        gfx_circ(mx, my, 6, C_SLATE);
        gfx_circ(mx - 1, my - 1, 4, C_GREY);
        gfx_rect(mx - 1, my - 1, 3, 3, C_RED);
        break;
    }
    case FT_CANS: {
        int k = -1;
        for (int i = 0; i < tnp_n_cans; i++)
            if (tnp_cans_c[i][0] == tnp_wrapc(cx) && tnp_cans_c[i][1] == tnp_wrapc(cy)) k = i;
        if (d && k >= 0 && !d->cans_ok[k]) {
            gfx_dither_circle(mx, my, 12, C_NIGHT, 8);
            break;
        }
        spr_draw(&tnp_spr[SP_CANS], mx - 9, my - 8, 0);
        spr_draw(&tnp_spr[SP_CANS], mx + 1, my - 8, 0);
        spr_draw(&tnp_spr[SP_CANS], mx - 4, my, 0);
        break;
    }
    case FT_RAMP: {
        /* a yellow ramp with chevrons pointing the way it throws you */
        int dx = DX[c->ramp_dir], dy = DY[c->ramp_dir];
        int rx = mx + dx * 8 - (dy ? 16 : 8), ry = my + dy * 8 - (dx ? 16 : 8);
        int rw = dy ? 32 : 16, rh = dx ? 32 : 16;
        gfx_rect(rx, ry, rw, rh, C_AMBER);
        gfx_rectb(rx, ry, rw, rh, C_INK);
        for (int i = 0; i < 3; i++) {
            int ox = mx + dx * (2 + i * 5), oy = my + dy * (2 + i * 5);
            if (dx) { gfx_line(ox - dx * 3, oy - 6, ox, oy, C_INK); gfx_line(ox, oy, ox - dx * 3, oy + 6, C_INK); }
            else { gfx_line(ox - 6, oy - dy * 3, ox, oy, C_INK); gfx_line(ox, oy, ox + 6, oy - dy * 3, C_INK); }
        }
        break;
    }
    default: break;
    }
    (void)t;
}

void tnp_draw_city(const TnpDay *d, float camx, float camy, int vx, int vy, int vw, int vh) {
    if (!blocks_done) label_blocks();
    int t = d ? d->frame : (int)engine_frame();
    int x0 = (int)floorf(camx / TNP_CELL), y0 = (int)floorf(camy / TNP_CELL);
    int x1 = (int)floorf((camx + (float)vw) / TNP_CELL), y1 = (int)floorf((camy + (float)vh) / TNP_CELL);
    for (int cy = y0; cy <= y1; cy++)
        for (int cx = x0; cx <= x1; cx++) {
            int sx = vx + (int)floorf((float)(cx * TNP_CELL) - camx), sy = vy + (int)floorf((float)(cy * TNP_CELL) - camy);
            const TnpCell *c = tnp_cell(cx, cy);
            switch (c->type) {
            case CT_BUILD: draw_building(sx, sy, cx, cy); break;
            case CT_HQ:
                gfx_rect(sx, sy, TNP_CELL, TNP_CELL, C_VIOLET);
                gfx_dither(sx, sy, TNP_CELL, TNP_CELL, C_PURPLE, 4);
                break;
            case CT_GRASS: draw_grass(sx, sy, cx, cy); break;
            case CT_TREE:
                draw_grass(sx, sy, cx, cy);
                gfx_circ(sx + 26, sy + 28, 19, C_NIGHT);
                gfx_circ(sx + 24, sy + 24, 19, C_JADE);
                gfx_circ(sx + 20, sy + 19, 10, C_LEAF);
                gfx_circb(sx + 24, sy + 24, 19, C_FOREST);
                break;
            case CT_BRINE: draw_brine(sx, sy, cx, cy, t); break;
            case CT_ROCK: draw_rock(sx, sy, cx, cy); break;
            case CT_FENCE: draw_fence(sx, sy, cx, cy); break;
            case CT_LOT: draw_lot(sx, sy); break;
            case CT_BRIDGE:
                draw_road(sx, sy, cx, cy, t);
                for (int k = 0; k < 4; k++) {
                    if (tnp_cell(cx + DX[k], cy + DY[k])->type != CT_BRINE) continue;
                    if (k == DIR_E) { gfx_rect(sx + TNP_CELL - 4, sy, 4, TNP_CELL, C_TAN); gfx_vline(sx + TNP_CELL - 1, sy, sy + TNP_CELL - 1, C_BROWN); }
                    if (k == DIR_W) { gfx_rect(sx, sy, 4, TNP_CELL, C_TAN); gfx_vline(sx, sy, sy + TNP_CELL - 1, C_BROWN); }
                }
                break;
            default: draw_road(sx, sy, cx, cy, t); break;
            }
            if (c->type == CT_ROAD) draw_feature(d, sx, sy, cx, cy, t);
        }
    /* the depot's sign: a turnip on the roof, its door */
    float hx = tnp_wrapd(14 * TNP_CELL - camx - (float)vw / 2) + (float)vw / 2, hy = tnp_wrapd(9 * TNP_CELL - camy - (float)vh / 2) + (float)vh / 2;
    int sx = vx + (int)hx, sy = vy + (int)hy;
    if (sx > vx - 60 && sx < vx + vw + 60 && sy > vy - 60 && sy < vy + vh + 60) {
        gfx_rectb(sx - 48, sy - 48, 96, 96, C_INK);
        gfx_rect(sx - 48, sy + 39, 96, 9, C_PURPLE);
        gfx_hline(sx - 48, sx + 47, sy + 39, C_INK);
        spr_draw_scaled(&tnp_spr[SP_TURNIP], sx - 9, sy - 30, 2, 0);
        tiny_center("GRANNY ROOT", sx, sy - 4, C_WHITE);
        tiny_center("TURNIPS", sx, sy + 3, C_YELLOW);
        gfx_rect(sx - 8, sy + 42, 16, 6, C_INK);
        gfx_rect(sx - 7, sy + 43, 14, 5, C_BROWN);
    }
}

/* ---- the moving things ----------------------------------------------------------------- */

static float g_cx, g_cy; /* the camera's centre in the world */
static int g_vx, g_vy;   /* the view's centre on the screen */

static int scr_x(float x) { return g_vx + (int)floorf(tnp_wrapd(x - g_cx)); }
static int scr_y(float y) { return g_vy + (int)floorf(tnp_wrapd(y - g_cy)); }
static bool on_view(int sx, int sy, int m) {
    return sx > TNP_HUD_W - m && sx < SCREEN_W + m && sy > -m && sy < SCREEN_H + m;
}

void tnp_draw_truck(float sx, float sy, float ang, int lift, int flash) {
    /* the shadow, then the truck (raised when it is in the air) */
    tnp_draw_rot(&tnp_spr[SP_TRUCK], sx + 2 + lift / 2.0f, sy + 2 + lift, ang, NULL, C_INK);
    if (flash) return;
    tnp_draw_rot(&tnp_spr[SP_TRUCK], sx, sy, ang, NULL, -1);
}

static void car_remap(int col, uint8_t *map) {
    static const uint8_t BODY[6] = {C_RED, C_BLUE, C_JADE, C_AMBER, C_PINK, C_GREY};
    pal_identity(map);
    map[C_RED] = BODY[col % 6];
    map[C_WINE] = PAL_LIGHTER[BODY[col % 6]];
}

static void draw_beet(int sx, int sy, const TnpMob *m, int t) {
    int hop = m->state == 1 ? (int)fabsf(sinf((float)m->t * 0.35f) * 8) : (int)fabsf(sinf((float)t * 0.1f) * 2);
    gfx_dither_circle(sx + 3, sy + 5, 13, C_INK, 10);
    sy -= hop;
    gfx_circ(sx, sy, 14, C_MAROON);
    gfx_circ(sx, sy, 13, C_WINE);
    gfx_circ(sx - 4, sy - 4, 5, C_MAGENTA);
    gfx_line(sx, sy + 13, sx, sy + 18, C_WINE);
    /* leaves */
    gfx_rect(sx - 6, sy - 21, 4, 9, C_JADE);
    gfx_rect(sx + 2, sy - 23, 4, 11, C_LEAF);
    gfx_rect(sx - 1, sy - 18, 3, 6, C_FOREST);
    /* an angry face */
    gfx_rect(sx - 7, sy - 2, 5, 4, C_WHITE);
    gfx_rect(sx + 2, sy - 2, 5, 4, C_WHITE);
    gfx_rect(sx - 5, sy - 1, 2, 2, C_INK);
    gfx_rect(sx + 4, sy - 1, 2, 2, C_INK);
    gfx_line(sx - 8, sy - 5, sx - 3, sy - 3, C_INK);
    gfx_line(sx + 8, sy - 5, sx + 3, sy - 3, C_INK);
    gfx_rect(sx - 4, sy + 5, 8, 2, C_INK);
}

static void draw_saucer(int sx, int sy, int kind, int t) {
    int r = kind == MK_SAUCER_S ? 6 : kind == MK_SAUCER_M ? 9 : 15;
    int h = r / 3 + 1;
    /* the shadow on the ground, then the saucer high above it */
    gfx_dither(sx - r + 5, sy + 8, r * 2, h + 1, C_INK, 10);
    sy -= 8;
    gfx_circ(sx, sy - h + 1, r / 2, C_INK);
    gfx_circ(sx, sy - h + 1, r / 2 - 1, C_CYAN);
    gfx_pset(sx - r / 4, sy - h - r / 4 + 1, C_ICE);
    for (int dy = -h; dy <= h; dy++) {
        float f = 1.0f - (float)(dy * dy) / (float)((h + 1) * (h + 1));
        int w = (int)((float)r * sqrtf(f));
        gfx_hline(sx - w - 1, sx + w + 1, sy + dy, C_INK);
        gfx_hline(sx - w, sx + w, sy + dy, dy < 0 ? C_LIGHT : dy == 0 ? C_GREY : C_SLATE);
    }
    for (int i = -r + 2; i <= r - 2; i += 4) gfx_pset(sx + i, sy, ((t / 6 + i) & 4) ? C_YELLOW : C_RED);
}

static void draw_arrow(int sx, int sy, float ang, int col) {
    float ca = cosf(ang), sa = sinf(ang);
    int tx = sx + (int)(ca * 26), ty = sy + (int)(sa * 26);
    int bx = sx + (int)(ca * 17), by = sy + (int)(sa * 17);
    int lx = bx + (int)(-sa * 6), ly = by + (int)(ca * 6), rx = bx - (int)(-sa * 6), ry = by - (int)(ca * 6);
    for (int k = 0; k < 2; k++) {
        int c = k ? col : C_INK, o = k ? 0 : 1;
        gfx_line(tx + o, ty + o, lx + o, ly + o, c);
        gfx_line(tx + o, ty + o, rx + o, ry + o, c);
        gfx_line(lx + o, ly + o, rx + o, ry + o, c);
        gfx_line(tx + o, ty + o, bx + o, by + o, c);
    }
}

static void draw_rain(int t) {
    for (int i = 0; i < 70; i++) {
        uint32_t h = hash2(i, 7);
        int x = TNP_HUD_W + (int)((h % 300) + (uint32_t)t * 3) % (TNP_VIEW_W + 20) - 10;
        int y = (int)(((h >> 9) % 200) + (uint32_t)t * 7) % (SCREEN_H + 20) - 10;
        gfx_line(x, y, x - 2, y + 6, (i & 3) ? C_ICE : C_CYAN);
    }
    gfx_dither(TNP_HUD_W, 0, TNP_VIEW_W, SCREEN_H, C_NAVY, 2);
}

/* ---- the dashboard ------------------------------------------------------------------------ */

static const char *const DAY_NAME[TNP_DAYS] = {"MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY", "SUNDAY"};

static void draw_hud(const TnpDay *d) {
    const TnpTruck *t = &d->tr;
    gfx_rect(0, 0, TNP_HUD_W, SCREEN_H, C_NIGHT);
    gfx_vline(TNP_HUD_W - 1, 0, SCREEN_H - 1, C_INK);
    gfx_vline(TNP_HUD_W - 2, 0, SCREEN_H - 1, C_DUSK);
    char buf[48];
    snprintf(buf, sizeof buf, d->practice ? "PRACTICE" : "DAY %d", d->day + 1);
    tiny_draw(buf, 4, 3, C_YELLOW);
    tiny_draw(d->practice ? "" : DAY_NAME[d->day], 4, 10, C_LIGHT);
    /* the clock */
    tiny_draw("TIME", 4, 19, C_GREY);
    if (d->practice) text_draw("--", 4, 26, C_GREY);
    else {
        int s = tnp_seconds(d);
        snprintf(buf, sizeof buf, "%02d", s);
        bool low = s <= 10;
        text_draw_scaled(buf, 4, 26, low && (d->frame / 8) % 2 ? C_RED : low ? C_ORANGE : C_WHITE, 2);
        if (d->add_t > 0 && d->last_add > 0) {
            snprintf(buf, sizeof buf, "+%d", d->last_add);
            tiny_draw(buf, 44, 32, C_LIME);
        }
    }
    /* hearts */
    for (int i = 0; i < TNP_HEARTS; i++)
        spr_draw(&tnp_spr[i < t->hearts ? SP_HEART : SP_HEART_OFF], 4 + i * 9, 45, 0);
    if (t->regen_t > 0 && t->hearts < TNP_HEARTS)
        gfx_rect(4 + t->hearts * 9, 52, 7 * t->regen_t / TNP_REGEN_T, 1, C_PINK);
    snprintf(buf, sizeof buf, "TRIES %d", tnp_hud_tries);
    tiny_draw(buf, 4, 56, d->practice ? C_SLATE : C_LIGHT);
    if (d->delivered < TNP_QUOTA || d->practice) snprintf(buf, sizeof buf, "DONE %d/%d", d->delivered, TNP_QUOTA);
    else snprintf(buf, sizeof buf, "DONE 5+%d", d->delivered - TNP_QUOTA);
    tiny_draw(buf, 4, 63, d->delivered >= TNP_QUOTA ? C_LIME : C_LIGHT);
    snprintf(buf, sizeof buf, "WEEK %d", tnp_hud_week + d->delivered);
    tiny_draw(buf, 4, 70, C_GREY);

    /* the radar: you in the middle, the delivery in red, the depot in blue */
    int rx = 4, ry = 78, rw = 56, rh = 56;
    gfx_rect(rx, ry, rw, rh, C_INK);
    gfx_rectb(rx - 1, ry - 1, rw + 2, rh + 2, C_DUSK);
    gfx_hline(rx, rx + rw - 1, ry + rh / 2, C_NAVY);
    gfx_vline(rx + rw / 2, ry, ry + rh - 1, C_NAVY);
    float sc = (float)rw / (float)TNP_WORLD;
    float dx, dy;
    tnp_dest_pos(d->dest, &dx, &dy);
    int ddx = rx + rw / 2 + (int)(tnp_wrapd(dx - t->x) * sc), ddy = ry + rh / 2 + (int)(tnp_wrapd(dy - t->y) * sc);
    if ((d->frame / 10) % 3) gfx_rect(ddx - 1, ddy - 1, 3, 3, C_RED);
    if (d->delivered >= TNP_QUOTA && !d->practice) {
        int hx = rx + rw / 2 + (int)(tnp_wrapd(TNP_HQ_DOOR_X - t->x) * sc), hy = ry + rh / 2 + (int)(tnp_wrapd(TNP_HQ_DOOR_Y - t->y) * sc);
        gfx_rect(hx - 1, hy - 1, 3, 3, C_SKY);
    }
    gfx_pset(rx + rw / 2, ry + rh / 2, C_YELLOW);
    gfx_pset(rx + rw / 2 + (int)lroundf(cosf(t->ang) * 2), ry + rh / 2 + (int)lroundf(sinf(t->ang) * 2), C_YELLOW);

    /* where to */
    int y = 138;
    char lines[3][UI_WRAP_LEN];
    if (d->delivered >= TNP_QUOTA && !d->practice) {
        tiny_draw("CLOCK OUT AT", 4, y, C_SKY);
        tiny_draw("THE DEPOT!", 4, y + 7, C_SKY);
        tiny_draw("OR DELIVER TO", 4, y + 15, C_GREY);
        int n = ui_wrap(TNP_DEST[d->dest].name, 56, true, lines, 1);
        (void)n;
        tiny_draw(lines[0], 4, y + 22, C_RED);
    } else {
        tiny_draw("DELIVER TO", 4, y, C_GREY);
        int n = ui_wrap(TNP_DEST[d->dest].name, 56, true, lines, 2);
        for (int i = 0; i < n && i < 2; i++) tiny_draw(lines[i], 4, y + 7 + i * 7, C_RED);
        tiny_draw(TNP_DISTRICT_NAME[tnp_district_at(t->x, t->y)], 4, y + 23, C_GREY);
    }
    if (d->event != EV_NONE) tiny_draw(TNP_EVENT_NAME[d->event], 4, 172, (d->frame / 30) % 2 ? C_ORANGE : C_AMBER);
}

void tnp_draw_play(const TnpDay *d) {
    const TnpTruck *t = &d->tr;
    int fr = d->frame;
    /* the camera: on the truck, a little ahead of where it is going */
    float lead_x = fclamp(t->vx * 10, -36, 36), lead_y = fclamp(t->vy * 8, -24, 24);
    g_cx = tnp_wrap(t->x + lead_x);
    g_cy = tnp_wrap(t->y + lead_y);
    g_vx = TNP_HUD_W + TNP_VIEW_W / 2;
    g_vy = TNP_VIEW_H / 2;
    gfx_clip(TNP_HUD_W, 0, TNP_VIEW_W, TNP_VIEW_H);
    tnp_draw_city(d, g_cx - TNP_VIEW_W / 2, g_cy - TNP_VIEW_H / 2, TNP_HUD_W, 0, TNP_VIEW_W, TNP_VIEW_H);

    /* brine sprays and waves */
    if (d->event == EV_BRINE) {
        for (int i = 0; i < tnp_n_pipes; i++)
            for (int k = 0; k < 4; k++) {
                if (!tnp_pipe_spraying(d, i, k)) continue;
                int sx = scr_x(tnp_cx(tnp_pipe_c[i][0])), sy = scr_y(tnp_cx(tnp_pipe_c[i][1]));
                if (!on_view(sx, sy, 80)) continue;
                for (int j = 0; j < 14; j++) {
                    int a = 6 + (j * 9 + fr * 3) % 58, w = (int)((hash2(j, fr / 3) % 13)) - 6;
                    int px = sx + DX[k] * a + DY[k] * w / 2, py = sy + DY[k] * a + DX[k] * w / 2;
                    gfx_rect(px, py, 2, 2, j & 1 ? C_LIME : C_LEAF);
                }
            }
        for (int w = 0; w < TNP_WAVES; w++) {
            float x0, x1;
            if (!tnp_wave_band(d, w, &x0, &x1)) continue;
            int sx0 = scr_x(tnp_wrap(x0)), sy = scr_y(tnp_cx(TNP_WAVE_ROW[w])) - TNP_CELL / 2;
            int ww = (int)(x1 - x0);
            gfx_dither(sx0, sy + 4, ww, TNP_CELL - 8, C_LIME, 10);
            gfx_vline(sx0 + ww - 1, sy + 4, sy + TNP_CELL - 5, C_WHITE);
        }
    }
    /* puddles */
    for (int i = 0; i < TNP_MAX_PUDDLES; i++) {
        const TnpPuddle *p = &d->pud[i];
        if (!p->alive) continue;
        int sx = scr_x(p->x), sy = scr_y(p->y);
        if (!on_view(sx, sy, 12)) continue;
        int lvl = p->t < 60 ? p->t / 5 + 2 : p->t > p->life - 30 ? (p->life - p->t) / 3 + 2 : 14;
        gfx_dither(sx - 9, sy - 5, 18, 10, C_BLUE, lvl);
        gfx_dither(sx - 6, sy - 3, 10, 5, C_SKY, lvl / 2);
    }
    /* the delivery circle and, after the quota, the depot's half circle */
    float dx, dy;
    tnp_dest_pos(d->dest, &dx, &dy);
    int dsx = scr_x(dx), dsy = scr_y(dy);
    if (on_view(dsx, dsy, 20)) {
        int r = (int)TNP_DEST_R + ((fr / 8) % 2);
        gfx_circb(dsx, dsy, r, C_RED);
        gfx_circb(dsx, dsy, r - 1, C_RED);
        gfx_circb(dsx, dsy, r - 5, (fr / 4) % 2 ? C_WHITE : C_PINK);
    }
    if (d->delivered >= TNP_QUOTA && !d->practice) {
        int hx = scr_x(TNP_HQ_DOOR_X), hy = scr_y(TNP_HQ_DOOR_Y);
        for (int yy = 0; yy < (int)TNP_HQ_R; yy++)
            for (int xx = -(int)TNP_HQ_R; xx <= (int)TNP_HQ_R; xx++)
                if (xx * xx + yy * yy < (int)(TNP_HQ_R * TNP_HQ_R)) gfx_pset(hx + xx, hy + yy, ((xx + yy + fr / 4) & 3) ? C_BLUE : C_SKY);
    }
    /* mushmen */
    for (int i = 0; i < TNP_MAX_MOBS; i++) {
        const TnpMob *m = &d->mob[i];
        if (!m->alive || m->kind != MK_MUSH) continue;
        int sx = scr_x(m->x), sy = scr_y(m->y);
        if (!on_view(sx, sy, 10)) continue;
        if (m->dead_t > 0) {
            gfx_dither_circle(sx, sy, 5, C_CREAM, 8);
            gfx_rect(sx - 3, sy - 1, 7, 2, C_EARTH);
            continue;
        }
        spr_draw(&tnp_spr[(m->t / 10) % 2 ? SP_MUSH1 : SP_MUSH2], sx - 3, sy - 6, m->vx < 0 ? SPR_FLIPX : 0);
    }
    /* cars */
    for (int i = 0; i < TNP_MAX_CARS; i++) {
        const TnpCar *c = &d->car[i];
        if (!c->alive) continue;
        int sx = scr_x(c->x), sy = scr_y(c->y);
        if (!on_view(sx, sy, 12)) continue;
        uint8_t map[PAL_COUNT];
        const Sprite *s = &tnp_spr[c->kind == CK_GANG ? SP_GANG : c->kind == CK_POLICE ? SP_POLICE : SP_CAR];
        if (c->kind == CK_TRAFFIC) car_remap(c->col, map);
        else {
            pal_identity(map);
            if (c->kind == CK_POLICE && (fr / 6) % 2) { map[C_RED] = C_SKY; map[C_SKY] = C_RED; }
        }
        tnp_draw_rot(s, (float)sx + 1.5f, (float)sy + 1.5f, c->ang, NULL, C_INK);
        tnp_draw_rot(s, (float)sx, (float)sy, c->ang, map, c->wreck_t > 0 ? C_DUSK : -1);
    }
    /* crates, floating */
    for (int i = 0; i < tnp_n_crates; i++) {
        if (!d->crate_ok[i]) continue;
        int sx = scr_x(tnp_cx(tnp_crate_c[i][0])), sy = scr_y(tnp_cx(tnp_crate_c[i][1]));
        if (!on_view(sx, sy, 20)) continue;
        int bob = (int)(sinf((float)(fr + i * 20) * 0.08f) * 2);
        gfx_dither(sx - 4, sy + 3, 9, 4, C_INK, 8);
        spr_draw(&tnp_spr[SP_BALLOON], sx - 3, sy - 19 + bob, 0);
        spr_draw(&tnp_spr[SP_CRATE], sx - 5, sy - 10 + bob, 0);
    }
    /* the truck */
    {
        int sx = scr_x(t->x), sy = scr_y(t->y);
        int lift = t->air_t > 0 ? (int)(sinf(3.14159f * (float)(t->air_max - t->air_t) / (float)t->air_max) * 10) : 0;
        bool flash = t->inv > 0 && (fr / 3) % 2 && t->state == TS_DRIVE;
        if (t->state == TS_SINK) {
            if (t->state_t < 24) tnp_draw_rot(&tnp_spr[SP_TRUCK], (float)sx, (float)sy, t->ang, NULL, t->state_t < 12 ? -1 : C_FOREST);
            gfx_dither_circle(sx, sy, 6 + t->state_t / 4, C_LIME, 10);
        } else if (t->state == TS_WRECK || t->state == TS_BOOM) {
            tnp_draw_rot(&tnp_spr[SP_TRUCK], (float)sx, (float)sy, t->ang, NULL, C_NIGHT);
            for (int k = 0; k < 4; k++)
                gfx_circ(sx + (int)(hash2(k, fr / 4) % 9) - 4, sy - 2 - (int)(hash2(k + 9, fr / 4) % 8), 2 + (int)(hash2(k, fr) % 2),
                         (k + fr / 3) % 3 ? C_ORANGE : C_YELLOW);
        } else
            tnp_draw_truck((float)sx, (float)sy, t->ang, lift, flash);
        if (t->swarmed && (fr / 6) % 2) gfx_circb(sx, sy, 11, C_RED);
        /* the arrow: for a moment after each new delivery, and again when close */
        float ddist = tnp_dist(t->x, t->y, dx, dy);
        if (t->state == TS_DRIVE && (d->dest_t < 150 || ddist < 230) && !on_view(dsx, dsy, -24)) {
            float a = atan2f(tnp_wrapd(dy - t->y), tnp_wrapd(dx - t->x));
            draw_arrow(sx, sy, a, (fr / 6) % 2 ? C_RED : C_PINK);
        }
    }
    /* the beet, bullets, bombs, saucers on top */
    for (int i = 0; i < TNP_MAX_MOBS; i++) {
        const TnpMob *m = &d->mob[i];
        if (!m->alive || m->kind != MK_BEET) continue;
        int sx = scr_x(m->x), sy = scr_y(m->y);
        if (on_view(sx, sy, 30)) draw_beet(sx, sy, m, fr);
    }
    for (int i = 0; i < TNP_MAX_SHOTS; i++) {
        const TnpShot *s = &d->shot[i];
        if (!s->alive) continue;
        int sx = scr_x(s->x), sy = scr_y(s->y);
        if (!on_view(sx, sy, 20)) continue;
        if (s->kind == SK_BULLET) {
            gfx_rect(sx - 1, sy - 1, 3, 3, C_INK);
            gfx_pset(sx, sy, C_YELLOW);
        } else {
            int r = 4 + s->t / 4;
            gfx_circb(sx, sy, r, (fr / 3) % 2 ? C_RED : C_YELLOW);
            gfx_circb(sx, sy, 12, C_MAROON);
        }
    }
    for (int i = 0; i < TNP_MAX_MOBS; i++) {
        const TnpMob *m = &d->mob[i];
        if (!m->alive || m->kind < MK_SAUCER_S) continue;
        int sx = scr_x(m->x), sy = scr_y(m->y);
        if (on_view(sx, sy, 30)) draw_saucer(sx, sy, m->kind, fr + i * 5);
    }
    for (int i = 0; i < TNP_MAX_PARTS; i++) {
        const TnpPart *p = &d->part[i];
        if (p->life <= 0) continue;
        gfx_rect(scr_x(p->x), scr_y(p->y), 2, 2, p->col);
    }
    if (d->event == EV_RAIN) draw_rain(fr);
    gfx_noclip();
    draw_hud(d);
}

/* ---- the people ---------------------------------------------------------------------- */

void tnp_draw_zib(int x, int y, int scale, int t, int mood) {
    spr_draw_scaled(&tnp_spr[SP_ZIB], x, y, scale, 0);
    /* the antennae bob; mood 1 = happy (eye closed in a smile) */
    if (mood == 1) {
        gfx_rect(x + 6 * scale, y + 8 * scale, 4 * scale, 2 * scale, C_WHITE);
        gfx_rect(x + 6 * scale, y + 9 * scale, 4 * scale, scale, C_INK);
    }
    if ((t / 20) % 2) gfx_rect(x + 3 * scale, y + scale, scale, scale, C_YELLOW);
}

void tnp_draw_anchor(int x, int y, int t, bool talking) {
    spr_draw_scaled(&tnp_spr[SP_ANCHOR], x, y, 3, 0);
    if (talking && (t / 6) % 2) gfx_rect(x + 7 * 3, y + 8 * 3, 2 * 3, 2 * 3, C_INK);
}

/* the library label: a corner of Blipton with the truck going round the block */
void tnp_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    float cx = 12 * TNP_CELL + 10, cy = 9 * TNP_CELL + 10;
    tnp_draw_city(NULL, cx - (float)w / 2, cy - (float)h / 2, x, y, w, h);
    /* round the depot's block: along y=10, up x=16, along y=7, down x=11 */
    float per = (float)((t * 2) % 600) / 600.0f;
    float px, py, ang;
    const float X0 = 11.5f * TNP_CELL, X1 = 16.5f * TNP_CELL, Y0 = 7.5f * TNP_CELL, Y1 = 10.5f * TNP_CELL;
    float lx = X1 - X0, ly = Y1 - Y0, tot = 2 * (lx + ly), s = per * tot;
    if (s < lx) { px = X0 + s; py = Y1; ang = 0; }
    else if (s < lx + ly) { px = X1; py = Y1 - (s - lx); ang = -1.5708f; }
    else if (s < 2 * lx + ly) { px = X1 - (s - lx - ly); py = Y0; ang = 3.14159f; }
    else { px = X0; py = Y0 + (s - 2 * lx - ly); ang = 1.5708f; }
    tnp_draw_truck((float)x + (float)w / 2 + (px - cx), (float)y + (float)h / 2 + (py - cy), ang, 0, 0);
    gfx_noclip();
}
