/* IRONVOW - every picture, drawn for UFO 40 at load time with the engine's
 * own pens: the walls of the Gaol, the Warren and the Starvault, the things
 * on their faces, what stands on the floors, every foe in two poses, the
 * knight seen from behind. Each is outlined in ink by one pass at the end. */
#include "ivw.h"
#include "ivw_art.h"

Sprite ivw_spr[SPR_COUNT];
Sprite ivw_tex[TEX_COUNT];
Sprite ivw_decal[DEC_COUNT];
Sprite ivw_glyph_decal[SP_COUNT];
Sprite ivw_foe_spr[FK_COUNT][2];

/* ---- a canvas to draw a sprite on --------------------------------------------------------------- */
static Surface canvas;
static uint8_t canvas_px[96 * 96];

static void begin(int w, int h) {
    canvas.w = w;
    canvas.h = h;
    canvas.px = canvas_px;
    memset(canvas_px, TRANSPARENT, sizeof canvas_px);
    gfx_set_target(&canvas);
}

static void outline(int col) {
    static uint8_t copy[96 * 96];
    memcpy(copy, canvas_px, (size_t)canvas.w * canvas.h);
    for (int y = 0; y < canvas.h; y++)
        for (int x = 0; x < canvas.w; x++) {
            if (copy[y * canvas.w + x] != TRANSPARENT) continue;
            bool edge = false;
            for (int k = 0; k < 4 && !edge; k++) {
                int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
                if (nx < 0 || ny < 0 || nx >= canvas.w || ny >= canvas.h) continue;
                uint8_t c = copy[ny * canvas.w + nx];
                edge = c != TRANSPARENT && c != col;
            }
            if (edge) canvas_px[y * canvas.w + x] = (uint8_t)col;
        }
}

static void end(Sprite *s, bool ink) {
    if (ink) outline(C_INK);
    s->w = (int16_t)canvas.w;
    s->h = (int16_t)canvas.h;
    s->px = (uint8_t *)malloc((size_t)canvas.w * canvas.h);
    memcpy(s->px, canvas_px, (size_t)canvas.w * canvas.h);
    gfx_set_target(NULL);
}

static void dot(int x, int y, int c) { gfx_pset(x, y, c); }

/* ---- wall textures (32 x 32) -------------------------------------------------------------------- */
static void tex_gaol(void) {
    begin(32, 32);
    gfx_rect(0, 0, 32, 32, C_SLATE);
    for (int row = 0; row < 4; row++) {
        int y = row * 8;
        gfx_hline(0, 31, y, C_NIGHT);
        int off = (row & 1) ? 8 : 0;
        for (int x = off; x < 32 + 16; x += 16) gfx_vline((x) % 32, y, y + 7, C_NIGHT);
        for (int x = 0; x < 32; x++) {
            if (((x * 7 + row * 13) % 11) == 0) dot(x, y + 2 + (x % 4), C_DUSK);
            if (((x * 5 + row * 3) % 13) == 0) dot(x, y + 1, C_GREY);
        }
        gfx_hline(0, 31, y + 1, C_GREY);
        for (int x = 0; x < 32; x += 3) dot(x, y + 1, C_SLATE);
    }
    end(&ivw_tex[TEX_GAOL], false);
}

static void tex_warren(void) {
    begin(32, 32);
    gfx_rect(0, 0, 32, 32, C_BROWN);
    /* a comb of cells */
    for (int cy = 0; cy < 4; cy++)
        for (int cx = 0; cx < 3; cx++) {
            int x = cx * 11 + ((cy & 1) ? 5 : 0), y = cy * 8 + 1;
            gfx_rect(x + 2, y, 7, 6, C_TAN);
            gfx_rect(x + 1, y + 1, 9, 4, C_TAN);
            gfx_rect(x + 3, y + 1, 4, 2, C_AMBER);
            dot(x + 3, y + 1, C_CREAM);
            gfx_rect(x + 2, y + 4, 6, 1, C_EARTH);
        }
    for (int i = 0; i < 18; i++) dot((i * 13) % 32, (i * 7) % 32, C_HIDE);
    end(&ivw_tex[TEX_WARREN], false);
}

static void tex_star(void) {
    begin(32, 32);
    gfx_rect(0, 0, 32, 32, C_NAVY);
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            if (((x + y * 3) % 17) == 0 || ((x * 2 + y) % 23) == 0) dot(x, y, C_PURPLE);
    gfx_line(0, 22, 12, 14, C_VIOLET);
    gfx_line(12, 14, 31, 18, C_VIOLET);
    gfx_line(6, 0, 10, 9, C_VIOLET);
    gfx_rect(0, 0, 32, 2, C_PURPLE);
    gfx_rect(0, 30, 32, 2, C_PURPLE);
    for (int x = 0; x < 32; x += 4) { dot(x, 0, C_AMBER); dot(x + 2, 31, C_AMBER); }
    static const int SX[6] = {4, 20, 27, 9, 16, 25}, SY[6] = {6, 4, 11, 25, 21, 26};
    for (int i = 0; i < 6; i++) {
        dot(SX[i], SY[i], C_YELLOW);
        if (i & 1) { dot(SX[i] - 1, SY[i], C_AMBER); dot(SX[i] + 1, SY[i], C_AMBER); dot(SX[i], SY[i] - 1, C_AMBER); dot(SX[i], SY[i] + 1, C_AMBER); }
    }
    end(&ivw_tex[TEX_STAR], false);
}

static void tex_door(int idx, bool locked) {
    begin(32, 32);
    gfx_rect(0, 0, 32, 32, C_NIGHT);
    gfx_rect(4, 2, 24, 30, C_BROWN);
    for (int x = 4; x < 28; x += 6) gfx_vline(x, 2, 31, C_EARTH);
    gfx_rect(4, 7, 24, 2, locked ? C_RED : C_GREY);
    gfx_rect(4, 23, 24, 2, locked ? C_RED : C_GREY);
    gfx_rect(22, 15, 3, 3, C_YELLOW);
    if (locked) { gfx_rect(14, 13, 4, 6, C_INK); gfx_rect(15, 14, 2, 2, C_AMBER); }
    gfx_rectb(3, 1, 26, 31, C_INK);
    end(&ivw_tex[idx], false);
}

static void tex_gate(void) {
    begin(32, 32);
    for (int x = 3; x < 32; x += 6) { gfx_vline(x, 0, 31, C_GREY); gfx_vline(x + 1, 0, 31, C_SLATE); }
    gfx_rect(0, 3, 32, 2, C_GREY);
    gfx_rect(0, 27, 32, 2, C_GREY);
    for (int x = 3; x < 32; x += 6) dot(x, 31, C_LIGHT);
    end(&ivw_tex[TEX_GATE], false);
}

static void tex_exit(void) {
    begin(32, 32);
    gfx_rect(0, 0, 32, 32, C_NIGHT);
    gfx_rect(6, 2, 20, 30, C_CREAM);
    gfx_rect(8, 4, 16, 28, C_YELLOW);
    for (int i = 0; i < 5; i++) gfx_rect(8, 8 + i * 5, 16, 2, C_AMBER);
    gfx_rectb(5, 1, 22, 31, C_INK);
    end(&ivw_tex[TEX_EXIT], false);
}

/* ---- decals on wall faces (32 x 32, transparent round them) ----------------------------------------- */
static void dec_torch(int idx, bool lit, int frame) {
    begin(32, 32);
    gfx_rect(13, 15, 6, 3, C_GREY);
    gfx_rect(15, 18, 2, 8, C_EARTH);
    gfx_rect(14, 12, 4, 4, C_BROWN);
    if (lit) {
        int s = frame ? 1 : 0;
        gfx_circ(16, 9 - s, 4, C_ORANGE);
        gfx_circ(16, 8 - s, 3, C_AMBER);
        gfx_rect(15, 3 - s, 2, 6, C_YELLOW);
        dot(16, 2 - s, C_CREAM);
    } else {
        dot(15, 10, C_SLATE);
        dot(17, 9, C_GREY);
    }
    end(&ivw_decal[idx], true);
}

static void dec_fungus(int idx, bool lit) {
    begin(32, 32);
    int cap = lit ? C_LIME : C_FOREST, gl = lit ? C_CREAM : C_JADE;
    gfx_rect(15, 14, 2, 12, C_TAN);
    gfx_circ(16, 13, 6, cap);
    gfx_rect(10, 13, 13, 3, cap);
    dot(13, 11, gl); dot(18, 10, gl); dot(16, 13, gl);
    if (lit) { gfx_circb(16, 12, 9, C_LEAF); }
    end(&ivw_decal[idx], true);
}

static void dec_lantern(int idx, bool lit) {
    begin(32, 32);
    gfx_rect(15, 2, 2, 6, C_GREY);
    gfx_rect(11, 8, 10, 14, C_GREY);
    gfx_rect(12, 9, 8, 12, lit ? C_YELLOW : C_NIGHT);
    if (lit) gfx_rect(14, 11, 4, 8, C_CREAM);
    gfx_rect(10, 22, 12, 2, C_SLATE);
    end(&ivw_decal[idx], true);
}

static void dec_lever(int idx, bool down) {
    begin(32, 32);
    gfx_rect(11, 12, 10, 10, C_SLATE);
    gfx_rectb(11, 12, 10, 10, C_GREY);
    if (down) { gfx_line(16, 17, 23, 25, C_LIGHT); gfx_circ(23, 25, 2, C_RED); }
    else { gfx_line(16, 17, 23, 8, C_LIGHT); gfx_circ(23, 8, 2, C_RED); }
    end(&ivw_decal[idx], true);
}

static void dec_switch(void) {
    begin(32, 32);
    gfx_circ(16, 16, 7, C_FOREST);
    gfx_circ(16, 16, 5, C_LEAF);
    gfx_circ(16, 16, 2, C_LIME);
    end(&ivw_decal[DEC_SWITCH], true);
}

static void dec_gems(int idx, bool solved) {
    begin(32, 32);
    gfx_rect(6, 6, 20, 20, C_EARTH);
    gfx_rectb(6, 6, 20, 20, C_TAN);
    static const uint8_t G[3] = {C_RED, C_LEAF, C_SKY};
    for (int i = 0; i < 9; i++) {
        int c = solved ? G[(i + i / 3) % 3] : G[(i * 5) % 3];
        gfx_rect(8 + (i % 3) * 6, 8 + (i / 3) * 6, 4, 4, c);
    }
    end(&ivw_decal[idx], true);
}

static void dec_hidden(void) {
    begin(32, 32);
    gfx_rectb(7, 6, 18, 26, C_GREY);
    gfx_hline(9, 22, 6, C_LIGHT);
    end(&ivw_decal[DEC_HIDDEN], false);
}

/* a glyph scratched on a stone plate */
void ivw_glyph_bounds(const char *g, int *minx, int *miny, int *maxx, int *maxy) {
    int x = 0, y = 0;
    *minx = *maxx = 0;
    *miny = *maxy = 0;
    for (const char *p = g; *p; p++) {
        x += *p == 'R' ? 1 : *p == 'L' ? -1 : 0;
        y += *p == 'D' ? 1 : *p == 'U' ? -1 : 0;
        *minx = imin(*minx, x); *maxx = imax(*maxx, x);
        *miny = imin(*miny, y); *maxy = imax(*maxy, y);
    }
}

void ivw_draw_glyph(const char *strokes, int n, int cx, int cy, int cell, int col, int dot_col) {
    int minx, miny, maxx, maxy;
    char buf[IVW_GLYPH_MAX + 1];
    n = imin(n, IVW_GLYPH_MAX);
    memcpy(buf, strokes, (size_t)n);
    buf[n] = 0;
    ivw_glyph_bounds(buf, &minx, &miny, &maxx, &maxy);
    int ox = cx - (minx + maxx) * cell / 2, oy = cy - (miny + maxy) * cell / 2;
    int x = 0, y = 0;
    for (int i = 0; i < n; i++) {
        int nx = x + (buf[i] == 'R') - (buf[i] == 'L'), ny = y + (buf[i] == 'D') - (buf[i] == 'U');
        int x0 = ox + x * cell, y0 = oy + y * cell, x1 = ox + nx * cell, y1 = oy + ny * cell;
        gfx_line(x0, y0, x1, y1, col);
        if (cell >= 6) { gfx_line(x0 + 1, y0, x1 + 1, y1, col); gfx_line(x0, y0 + 1, x1, y1 + 1, col); }
        x = nx; y = ny;
    }
    if (dot_col >= 0) gfx_rect(ox - 1, oy - 1, 3, 3, dot_col);
}

static void dec_glyph(int sp) {
    begin(32, 32);
    gfx_rect(3, 3, 26, 26, C_GREY);
    gfx_rectb(3, 3, 26, 26, C_LIGHT);
    int minx, miny, maxx, maxy;
    ivw_glyph_bounds(IVW_SPELL[sp].glyph, &minx, &miny, &maxx, &maxy);
    int span = imax(maxx - minx, maxy - miny);
    int cell = span <= 1 ? 8 : span == 2 ? 7 : span == 3 ? 5 : 4;
    ivw_draw_glyph(IVW_SPELL[sp].glyph, (int)strlen(IVW_SPELL[sp].glyph), 16, 16, cell, C_INK, C_RED);
    end(&ivw_glyph_decal[sp], true);
}

/* ---- things standing on the floor (32 x 32, feet on the bottom row) ----------------------------------- */
static void spr_chest(int idx, bool open) {
    begin(32, 32);
    gfx_rect(6, 18, 20, 13, C_BROWN);
    gfx_rect(6, 18, 20, 2, C_TAN);
    gfx_rect(6, 24, 20, 2, C_AMBER);
    if (open) { gfx_rect(6, 10, 20, 7, C_EARTH); gfx_rect(8, 17, 16, 3, C_INK); }
    else { gfx_rect(6, 13, 20, 5, C_BROWN); gfx_rect(5, 13, 22, 1, C_TAN); gfx_rect(15, 18, 3, 4, C_YELLOW); }
    end(&ivw_spr[idx], true);
}

static void spr_well(void) {
    begin(32, 32);
    gfx_rect(4, 20, 24, 11, C_GREY);
    for (int x = 4; x < 28; x += 6) gfx_vline(x, 20, 30, C_SLATE);
    gfx_rect(4, 18, 24, 3, C_LIGHT);
    gfx_rect(7, 18, 18, 2, C_BLUE); /* the water: recoloured when drawn */
    gfx_rect(6, 4, 2, 15, C_BROWN);
    gfx_rect(24, 4, 2, 15, C_BROWN);
    gfx_rect(5, 3, 22, 3, C_EARTH);
    end(&ivw_spr[SPR_WELL], true);
}

static void spr_throne(void) {
    begin(32, 32);
    gfx_rect(8, 2, 16, 18, C_WINE);
    gfx_rect(10, 4, 12, 14, C_RED);
    gfx_rect(6, 18, 20, 5, C_AMBER);
    gfx_rect(6, 23, 4, 8, C_AMBER);
    gfx_rect(22, 23, 4, 8, C_AMBER);
    gfx_rect(7, 0, 3, 4, C_YELLOW); gfx_rect(22, 0, 3, 4, C_YELLOW); gfx_rect(15, 0, 2, 3, C_YELLOW);
    end(&ivw_spr[SPR_THRONE], true);
}

static void spr_person(int idx, int robe, int trim, int face, bool helm) {
    begin(32, 32);
    gfx_rect(10, 12, 12, 19, robe);
    gfx_rect(8, 16, 16, 15, robe);
    gfx_rect(8, 29, 16, 2, trim);
    gfx_circ(16, 8, 5, helm ? C_GREY : robe);
    if (helm) { gfx_rect(13, 7, 6, 2, C_INK); gfx_rect(15, 1, 2, 3, C_RED); }
    else { gfx_circ(16, 9, 3, face); dot(15, 9, C_INK); dot(17, 9, C_INK); }
    gfx_rect(10, 18, 12, 2, trim);
    if (idx == SPR_PEDLAR) { gfx_rect(23, 14, 6, 6, C_EARTH); gfx_rect(24, 15, 4, 4, C_TAN); }
    if (idx == SPR_KNIGHT_NPC) { gfx_rect(6, 14, 3, 14, C_AMBER); gfx_rect(24, 6, 2, 20, C_LIGHT); }
    end(&ivw_spr[idx], true);
}

static void spr_pillar(void) {
    begin(32, 32);
    gfx_rect(10, 0, 12, 32, C_GREY);
    gfx_rect(12, 0, 3, 32, C_LIGHT);
    gfx_rect(8, 0, 16, 3, C_LIGHT);
    gfx_rect(8, 29, 16, 3, C_SLATE);
    end(&ivw_spr[SPR_PILLAR], true);
}

static void spr_rack(void) {
    begin(32, 32);
    gfx_rect(5, 6, 22, 3, C_BROWN);
    gfx_rect(5, 22, 22, 3, C_BROWN);
    gfx_rect(6, 6, 2, 26, C_EARTH);
    gfx_rect(24, 6, 2, 26, C_EARTH);
    for (int i = 0; i < 3; i++) {
        gfx_rect(10 + i * 5, 2, 2, 22, C_GREY);
        gfx_rect(9 + i * 5, 18, 4, 2, C_AMBER);
    }
    gfx_line(12, 4, 20, 24, C_SLATE);
    end(&ivw_spr[SPR_RACK], true);
}

static void spr_socket(void) {
    begin(32, 32);
    gfx_rect(9, 2, 14, 30, C_PURPLE);
    gfx_rect(11, 2, 3, 30, C_VIOLET);
    gfx_rect(7, 0, 18, 3, C_VIOLET);
    for (int i = 0; i < 3; i++) gfx_circ(16, 9 + i * 7, 2, C_INK);
    end(&ivw_spr[SPR_SOCKET], true);
}

static void spr_stairs(void) {
    begin(32, 32);
    for (int i = 0; i < 5; i++) gfx_rect(4 + i * 2, 20 + i * 2, 24 - i * 4, 2, i & 1 ? C_SLATE : C_GREY);
    gfx_rect(12, 30, 8, 2, C_INK);
    end(&ivw_spr[SPR_STAIRS], true);
}

static void spr_bag(void) {
    begin(32, 32);
    gfx_circ(16, 26, 5, C_TAN);
    gfx_rect(14, 19, 4, 3, C_EARTH);
    dot(15, 25, C_CREAM);
    end(&ivw_spr[SPR_ITEM_BAG], true);
}

static void spr_flame(void) {
    begin(32, 32);
    gfx_circ(16, 20, 8, C_VIOLET);
    gfx_circ(16, 17, 6, C_MAGENTA);
    gfx_rect(14, 6, 4, 10, C_PINK);
    gfx_circ(16, 20, 4, C_PINK);
    dot(13, 19, C_WHITE); dot(19, 19, C_WHITE);
    end(&ivw_spr[SPR_FLAME], true);
}

static void spr_boulder(void) {
    begin(32, 32);
    gfx_circ(16, 20, 10, C_SLATE);
    gfx_circ(13, 17, 5, C_GREY);
    end(&ivw_spr[SPR_BOULDER], true);
}

/* ---- the knight from behind (40 x 48) ------------------------------------------------------------------ */
static void knight_body(void) {
    gfx_rect(13, 22, 14, 18, C_GREY);      /* back */
    gfx_rect(11, 20, 18, 6, C_LIGHT);      /* shoulders */
    gfx_rect(14, 40, 5, 8, C_SLATE);       /* legs */
    gfx_rect(21, 40, 5, 8, C_SLATE);
    gfx_rect(13, 34, 14, 3, C_BROWN);      /* belt */
    gfx_circ(20, 13, 7, C_GREY);           /* helm */
    gfx_rect(14, 13, 13, 6, C_GREY);
    gfx_rect(19, 4, 2, 6, C_RED);          /* plume */
    gfx_rect(17, 6, 6, 2, C_RED);
}

static void spr_knight(void) {
    begin(40, 48);
    knight_body();
    gfx_rect(3, 24, 9, 13, C_EARTH);  /* the shield, low on the left arm */
    gfx_rectb(3, 24, 9, 13, C_TAN);
    gfx_rect(30, 22, 3, 14, C_LIGHT); /* the blade, held up on the right */
    gfx_rect(28, 34, 7, 2, C_AMBER);
    end(&ivw_spr[SPR_KNIGHT], true);

    begin(40, 48);
    knight_body();
    gfx_rect(9, 18, 22, 18, C_EARTH); /* guard: the shield square in front */
    gfx_rectb(9, 18, 22, 18, C_TAN);
    gfx_rect(18, 22, 4, 10, C_TAN);
    end(&ivw_spr[SPR_KNIGHT_GUARD], true);

    begin(40, 48);
    knight_body();
    gfx_rect(2, 28, 9, 12, C_EARTH);
    gfx_line(26, 22, 38, 2, C_WHITE); /* the swing */
    gfx_line(27, 22, 39, 3, C_LIGHT);
    gfx_line(28, 22, 39, 6, C_LIGHT);
    end(&ivw_spr[SPR_KNIGHT_SWING], true);

    begin(40, 48);
    gfx_circ(20, 34, 11, C_GREY);
    gfx_circ(20, 34, 7, C_LIGHT);
    gfx_rect(18, 22, 4, 4, C_RED);
    end(&ivw_spr[SPR_KNIGHT_ROLL], true);

    begin(40, 48);
    gfx_circ(20, 30, 13, C_TEAL);
    gfx_circ(20, 28, 9, C_JADE);
    gfx_line(8, 20, 2, 10, C_TEAL); gfx_line(32, 20, 38, 10, C_TEAL);
    gfx_rect(12, 40, 4, 8, C_FOREST); gfx_rect(24, 40, 4, 8, C_FOREST);
    end(&ivw_spr[SPR_BEETLE_BACK], true);
}

/* ---- the foes, two poses each: 0 waiting, 1 striking --------------------------------------------------- */
static void foe_art(int k, int pose) {
    int w = 40, h = 40;
    if (k == FK_WARDEN || k == FK_REGENT || k == FK_MATRON) w = h = 64;
    begin(w, h);
    int cx = w / 2, by = h - 1;
    int lift = pose ? -3 : 0;
    switch (k) {
    case FK_OOZE:
        gfx_circ(cx, by - 9 + lift, 11, C_LEAF);
        gfx_rect(cx - 11, by - 9, 23, 9, C_LEAF);
        gfx_circ(cx - 4, by - 13 + lift, 2, C_WHITE); gfx_circ(cx + 4, by - 13 + lift, 2, C_WHITE);
        dot(cx - 4, by - 13 + lift, C_INK); dot(cx + 4, by - 13 + lift, C_INK);
        gfx_rect(cx - 6, by - 8, 4, 2, C_LIME);
        if (pose) gfx_rect(cx - 15, by - 14, 6, 3, C_LEAF);
        break;
    case FK_GNAW: case FK_PUPPET: {
        bool rat = k == FK_GNAW;
        int y = by - 22 + (pose ? 8 : 0);
        gfx_circ(cx, y, 7, rat ? C_HIDE : C_TAN);
        gfx_line(cx - 7, y, cx - 18, y - (pose ? 2 : 9), rat ? C_EARTH : C_CREAM);
        gfx_line(cx + 7, y, cx + 18, y - (pose ? 2 : 9), rat ? C_EARTH : C_CREAM);
        if (rat) {
            gfx_rect(cx - 18, y - 9, 8, 6, C_WINE); gfx_rect(cx + 11, y - 9, 8, 6, C_WINE);
            dot(cx - 2, y - 1, C_RED); dot(cx + 2, y - 1, C_RED);
            gfx_rect(cx - 1, y + 5, 3, 2, C_WHITE);
        } else {
            gfx_rect(cx - 2, y + 6, 5, 10, C_PINK);
            dot(cx - 2, y - 1, C_INK); dot(cx + 2, y - 1, C_INK);
            gfx_rect(cx - 3, y + 3, 7, 1, C_RED);
            gfx_vline(cx, 0, y - 7, C_LIGHT);
        }
        break;
    }
    case FK_LIDMAW:
        gfx_rect(cx - 12, by - 14, 24, 14, C_BROWN);
        gfx_rect(cx - 12, by - 24 - (pose ? 4 : 0), 24, 8, C_BROWN);
        for (int i = 0; i < 6; i++) {
            gfx_rect(cx - 11 + i * 4, by - 16 - (pose ? 4 : 0), 2, 3, C_WHITE);
            gfx_rect(cx - 9 + i * 4, by - 15, 2, 2, C_WHITE);
        }
        gfx_rect(cx - 12, by - 9, 24, 2, C_AMBER);
        dot(cx - 5, by - 21 - (pose ? 4 : 0), C_RED); dot(cx + 5, by - 21 - (pose ? 4 : 0), C_RED);
        break;
    case FK_TUSKER: case FK_GUARD: case FK_BEETLE: {
        int skin = k == FK_TUSKER ? C_JADE : k == FK_GUARD ? C_SLATE : C_TEAL;
        gfx_rect(cx - 9, by - 26, 18, 18, skin);
        gfx_rect(cx - 7, by - 8, 5, 8, skin == C_SLATE ? C_DUSK : C_FOREST);
        gfx_rect(cx + 2, by - 8, 5, 8, skin == C_SLATE ? C_DUSK : C_FOREST);
        gfx_circ(cx, by - 31, 7, skin);
        if (k == FK_TUSKER) { gfx_rect(cx - 5, by - 29, 2, 4, C_CREAM); gfx_rect(cx + 4, by - 29, 2, 4, C_CREAM); }
        if (k == FK_GUARD) { gfx_rect(cx - 7, by - 37, 14, 4, C_GREY); gfx_rect(cx - 4, by - 33, 8, 2, C_INK); }
        if (k == FK_BEETLE) { gfx_line(cx - 4, by - 37, cx - 9, by - 39, C_LIME); gfx_line(cx + 4, by - 37, cx + 9, by - 39, C_LIME); }
        dot(cx - 3, by - 32, C_RED); dot(cx + 3, by - 32, C_RED);
        if (pose) { gfx_rect(cx + 8, by - 38, 4, 16, C_EARTH); gfx_rect(cx + 6, by - 40, 8, 5, C_BROWN); }
        else { gfx_rect(cx + 10, by - 26, 4, 16, C_EARTH); gfx_rect(cx - 15, by - 26, 7, 12, C_BROWN); }
        break;
    }
    case FK_PEDLAR:
        gfx_rect(cx - 8, by - 26, 16, 22, C_PURPLE);
        gfx_circ(cx, by - 30, 7, C_PURPLE);
        gfx_circ(cx, by - 29, 4, C_LIME);
        dot(cx - 2, by - 30, C_INK); dot(cx + 2, by - 30, C_INK);
        gfx_rect(cx - 1, by - 27, 3, 4, C_LIME);
        gfx_line(cx + 9, by - 34, cx + 13, by - 4, C_EARTH);
        if (pose) gfx_circ(cx + 13, by - 35, 4, C_ORANGE);
        break;
    case FK_BONE:
        gfx_rect(cx - 6, by - 26, 12, 16, C_LIGHT);
        for (int i = 0; i < 4; i++) gfx_hline(cx - 6, cx + 5, by - 24 + i * 4, C_GREY);
        gfx_circ(cx, by - 31, 6, C_WHITE);
        dot(cx - 2, by - 32, C_INK); dot(cx + 2, by - 32, C_INK);
        gfx_rect(cx - 5, by - 10, 3, 10, C_LIGHT); gfx_rect(cx + 2, by - 10, 3, 10, C_LIGHT);
        gfx_rect(cx - 17, by - 28, 10, 16, C_SLATE);
        if (pose) gfx_line(cx + 6, by - 24, cx + 19, by - 34, C_GREY);
        else gfx_rect(cx + 8, by - 34, 3, 22, C_GREY);
        break;
    case FK_SKINK: case FK_COILER: {
        bool coil = k == FK_COILER;
        int c = coil ? C_FOREST : C_LEAF;
        gfx_rect(cx - 7, by - 26, 14, 18, c);
        gfx_circ(cx, by - 30, 6, c);
        gfx_rect(cx - 1, by - 34, 9, 4, c);
        dot(cx + 2, by - 32, C_YELLOW);
        if (coil) {
            gfx_circ(cx, by - 6, 8, c);
            gfx_rect(cx - 16, by - 6, 32, 4, c);
            gfx_line(cx - 7, by - 22, cx - 17, by - (pose ? 34 : 20), C_LIGHT);
            gfx_line(cx + 7, by - 22, cx + 17, by - (pose ? 34 : 20), C_LIGHT);
            gfx_line(cx - 7, by - 18, cx - 16, by - 10, C_LIGHT);
            gfx_line(cx + 7, by - 18, cx + 16, by - 10, C_LIGHT);
        } else {
            gfx_rect(cx - 6, by - 8, 4, 8, c); gfx_rect(cx + 2, by - 8, 4, 8, c);
            gfx_line(cx - 6, by - 12, cx - 16, by - 4, c);
            if (pose) gfx_circ(cx + 12, by - 28, 4, C_LIME);
        }
        break;
    }
    case FK_EGG:
        gfx_circ(cx, by - 11, 10, C_CREAM);
        gfx_rect(cx - 10, by - 11, 21, 10, C_CREAM);
        gfx_circ(cx - 3, by - 15, 2, C_AMBER);
        gfx_line(cx - 2, by - 4, cx + 3, by - 9, C_TAN);
        break;
    case FK_GRUB:
        for (int i = 0; i < 4; i++) gfx_circ(cx - 9 + i * 6, by - 6 - (i == 3 && pose ? 4 : 0), 5, C_CREAM);
        gfx_circ(cx + 9, by - 8 - (pose ? 4 : 0), 5, C_TAN);
        gfx_rect(cx + 11, by - 6 - (pose ? 4 : 0), 3, 2, C_RED);
        break;
    case FK_CHITIN: case FK_SILVER: {
        int c = k == FK_SILVER ? C_LIGHT : C_EARTH, c2 = k == FK_SILVER ? C_GREY : C_BROWN;
        gfx_circ(cx, by - 12, 7, c);
        gfx_circ(cx, by - 24, 5, c2);
        gfx_circ(cx, by - 32, 5, c);
        for (int i = 0; i < 3; i++) { gfx_line(cx - 5, by - 12 + i * 3, cx - 14, by - 2 + i, c2); gfx_line(cx + 5, by - 12 + i * 3, cx + 14, by - 2 + i, c2); }
        gfx_line(cx - 4, by - 26, cx - 13, by - (pose ? 38 : 30), c);
        gfx_line(cx + 4, by - 26, cx + 13, by - (pose ? 38 : 30), c);
        dot(cx - 2, by - 33, C_RED); dot(cx + 2, by - 33, C_RED);
        break;
    }
    case FK_PUFF:
        gfx_circ(cx, by - 24, 10, C_WINE);
        for (int i = 0; i < 8; i++) gfx_circ(cx + (i % 3 - 1) * 6, by - 24 + (i / 3 - 1) * 6, 1, C_PINK);
        gfx_vline(cx, by - 14, by - 6, C_TAN);
        if (pose) gfx_circb(cx, by - 24, 13, C_PINK);
        break;
    case FK_MUMMER:
        gfx_rect(cx - 8, by - 24, 16, 16, C_RED);
        gfx_rect(cx, by - 24, 8, 16, C_YELLOW);
        gfx_circ(cx, by - 28, 6, C_WHITE);
        gfx_line(cx - 6, by - 32, cx - 12, by - 38, C_RED); gfx_line(cx + 6, by - 32, cx + 12, by - 38, C_YELLOW);
        dot(cx - 2, by - 29, C_INK); dot(cx + 2, by - 29, C_INK);
        gfx_rect(cx - 2, by - 26, 5, 1, C_RED);
        if (pose) gfx_circ(cx + 12, by - 22, 3, C_SKY);
        break;
    case FK_GRASPER:
        gfx_rect(cx - 8, by - 24, 16, 14, C_DUSK);
        for (int i = 0; i < 4; i++) gfx_rect(cx - 8 + i * 4, by - 34 + (pose ? 6 : 0), 3, 11, C_SLATE);
        gfx_rect(cx + 8, by - 22, 6, 4, C_SLATE);
        gfx_circ(cx, by - 18, 2, C_RED);
        break;
    case FK_WARDEN:
        gfx_rect(cx - 12, by - 44, 24, 26, C_DUSK);
        gfx_rect(cx - 16, by - 46, 32, 8, C_SLATE);
        gfx_circ(cx, by - 52, 8, C_SLATE);
        gfx_rect(cx - 6, by - 53, 12, 2, C_CYAN);
        for (int i = 0; i < 5; i++) gfx_vline(cx - 10 + i * 5, by - 18, by - 6 - (i & 1) * 4, C_NAVY);
        gfx_rect(cx - 26, by - 44, 10, 18, C_GREY);
        gfx_rectb(cx - 26, by - 44, 10, 18, C_CYAN);
        if (pose) { gfx_line(cx + 14, by - 40, cx + 30, by - 62, C_ICE); gfx_line(cx + 15, by - 40, cx + 31, by - 61, C_WHITE); }
        else gfx_rect(cx + 16, by - 50, 3, 30, C_ICE);
        break;
    case FK_MATRON:
        gfx_circ(cx, by - 20, 18, C_AMBER);
        gfx_circ(cx, by - 20, 13, C_TAN);
        for (int i = 0; i < 5; i++) gfx_hline(cx - 14, cx + 14, by - 30 + i * 5, C_BROWN);
        gfx_circ(cx, by - 44, 10, C_BROWN);
        dot(cx - 4, by - 45, C_RED); dot(cx + 4, by - 45, C_RED);
        gfx_line(cx - 4, by - 53, cx - 12, by - 62, C_EARTH); gfx_line(cx + 4, by - 53, cx + 12, by - 62, C_EARTH);
        if (pose) gfx_circ(cx, by - 38, 4, C_LIME);
        break;
    case FK_CLAW:
        gfx_rect(cx - 4, by - 20, 8, 20, C_BROWN);
        gfx_line(cx, by - 20, cx - 10, by - 34 + (pose ? 6 : 0), C_EARTH);
        gfx_line(cx, by - 20, cx + 10, by - 34 + (pose ? 6 : 0), C_EARTH);
        gfx_rect(cx - 12, by - 36 + (pose ? 6 : 0), 4, 6, C_TAN);
        gfx_rect(cx + 8, by - 36 + (pose ? 6 : 0), 4, 6, C_TAN);
        break;
    case FK_REGENT:
        gfx_rect(cx - 14, by - 42, 28, 36, C_INK);
        gfx_rect(cx - 12, by - 40, 24, 32, C_PURPLE);
        gfx_rect(cx - 20, by - 44, 40, 8, C_VIOLET);
        gfx_circ(cx, by - 50, 9, C_NIGHT);
        gfx_rect(cx - 8, by - 62, 3, 6, C_AMBER); gfx_rect(cx - 1, by - 63, 3, 7, C_AMBER); gfx_rect(cx + 6, by - 62, 3, 6, C_AMBER);
        dot(cx - 3, by - 50, C_RED); dot(cx + 3, by - 50, C_RED);
        gfx_rect(cx - 12, by - 8, 24, 8, C_NIGHT);
        if (pose) { gfx_line(cx - 18, by - 40, cx - 31, by - 18, C_MAGENTA); gfx_line(cx + 18, by - 40, cx + 31, by - 18, C_MAGENTA); }
        else { gfx_rect(cx - 24, by - 38, 5, 20, C_VIOLET); gfx_rect(cx + 19, by - 38, 5, 20, C_VIOLET); }
        break;
    case FK_STONE:
        gfx_circ(cx, by - 20, 7, C_WHITE); /* recoloured when drawn */
        gfx_circ(cx - 2, by - 22, 2, C_LIGHT);
        if (pose) gfx_circb(cx, by - 20, 10, C_WHITE);
        break;
    default: gfx_circ(cx, by - 16, 10, C_GREY); break;
    }
    end(&ivw_foe_spr[k][pose], true);
}

void ivw_art_load(void) {
    if (ivw_tex[TEX_GAOL].px) return;
    tex_gaol(); tex_warren(); tex_star();
    tex_door(TEX_DOOR, false); tex_door(TEX_LOCKED, true);
    tex_gate(); tex_exit();
    dec_torch(DEC_TORCH_LIT0, true, 0); dec_torch(DEC_TORCH_LIT1, true, 1); dec_torch(DEC_TORCH_OUT, false, 0);
    dec_fungus(DEC_FUNGUS_LIT, true); dec_fungus(DEC_FUNGUS_OUT, false);
    dec_lantern(DEC_LANTERN_ON, true); dec_lantern(DEC_LANTERN_OFF, false);
    dec_lever(DEC_LEVER_UP, false); dec_lever(DEC_LEVER_DOWN, true);
    dec_switch();
    dec_gems(DEC_GEMS, false); dec_gems(DEC_GEMS_DONE, true);
    dec_hidden();
    for (int s = 0; s < SP_COUNT; s++) dec_glyph(s);
    spr_chest(SPR_CHEST, false); spr_chest(SPR_CHEST_OPEN, true);
    spr_well(); spr_throne();
    spr_person(SPR_PEDLAR, C_PURPLE, C_VIOLET, C_LIME, false);
    spr_person(SPR_KNIGHT_NPC, C_AMBER, C_ORANGE, C_TAN, true);
    spr_pillar(); spr_rack(); spr_socket(); spr_stairs(); spr_bag(); spr_flame(); spr_boulder();
    spr_knight();
    for (int k = 0; k < FK_COUNT; k++) { foe_art(k, 0); foe_art(k, 1); }
}
