/* WITHERLEA - drawing: one picture a room (all drawn here with shapes, for
 * UFO 40), the light ring in dark rooms, the Keeper, the cursor, the text
 * panel, the bag, the diary and the other screens. */
#include "wth.h"

/* ------------------------------------------------------------------ */
/* shapes                                                               */

/* a trapezoid: the top edge from xa to xb at y0, the bottom edge from xc to xd at y1 */
static void trap(int xa, int xb, int xc, int xd, int y0, int y1, int col) {
    for (int y = y0; y <= y1; y++) {
        int t = y1 > y0 ? (y - y0) * 256 / (y1 - y0) : 0;
        int l = xa + (xc - xa) * t / 256, r = xb + (xd - xb) * t / 256;
        gfx_hline(l, r, y, col);
    }
}

static void frame(int x, int y, int w, int h, int col) { gfx_rectb(x, y, w, h, col); }

static const WthSpot *SP(int s) { return &WTH_SPOT[s]; }

/* the indoor box: back wall 72..248 x 26..100 */
static void interior(int wall, int side, int floor, int ceil) {
    gfx_rect(WTH_PX, WTH_PY, WTH_PW, WTH_PH, side);
    trap(8, 311, 72, 248, 6, 26, ceil);
    trap(72, 248, 8, 311, 100, 133, floor);
    gfx_rect(72, 26, 177, 75, wall);
    int e = PAL_DARKER[side];
    gfx_line(8, 6, 72, 26, e);
    gfx_line(311, 6, 248, 26, e);
    gfx_line(72, 100, 8, 133, e);
    gfx_line(248, 100, 311, 133, e);
    gfx_rectb(72, 26, 177, 75, PAL_DARKER[wall]);
}

static void floorboards(int col) {
    for (int k = 0; k < 9; k++) {
        int xb = 72 + k * 22, xf = 8 + k * 38;
        gfx_line(xb, 100, xf, 133, col);
    }
    gfx_hline(52, 268, 112, col);
    gfx_hline(30, 290, 123, col);
}

static void wallpaper_stripes(int col) {
    for (int x = 78; x < 248; x += 10) gfx_vline(x, 27, 99, col);
}

static void bricks(int x0, int y0, int x1, int y1, int col) {
    for (int y = y0; y < y1; y += 6) {
        gfx_hline(x0, x1, y, col);
        for (int x = x0 + ((y / 6) & 1) * 7; x < x1; x += 14) gfx_vline(x, y, imin(y + 5, y1), col);
    }
}

static void door_back(int x, int y, int w, int h, int col, bool open) {
    if (open) {
        gfx_rect(x, y, w, h, C_INK);
        gfx_rect(x, y, 6, h, col);
    } else {
        gfx_rect(x, y, w, h, col);
        int d = PAL_DARKER[col];
        frame(x + 4, y + 5, w - 8, h / 2 - 8, d);
        frame(x + 4, y + h / 2 + 2, w - 8, h / 2 - 8, d);
        gfx_pset(x + w - 6, y + h / 2, C_AMBER);
    }
    frame(x - 1, y - 1, w + 2, h + 1, C_INK);
}

static void door_side(bool left, int col, bool open) {
    for (int i = 0; i <= 34; i++) {
        int x = left ? 20 + i : 300 - i;
        int top = 40 + i * 6 / 34, bot = 112 - i * 8 / 34;
        gfx_vline(x, top, bot, open ? C_INK : (i < 3 || i > 31) ? PAL_DARKER[col] : col);
    }
    if (!open) {
        int kx = left ? 50 : 270;
        gfx_rect(kx - 1, 75, 2, 2, C_AMBER);
    }
}

static void window_back(int x, int y, int w, int h, int sky) {
    gfx_rect(x, y, w, h, sky);
    gfx_dither(x, y, w, h, C_NAVY, 6);
    frame(x, y, w, h, C_CREAM);
    gfx_vline(x + w / 2, y, y + h - 1, C_CREAM);
    gfx_hline(x, x + w - 1, y + h / 2, C_CREAM);
}

static void picture_frame(int x, int y, int w, int h, int inner) {
    gfx_rect(x, y, w, h, C_AMBER);
    gfx_rect(x + 2, y + 2, w - 4, h - 4, inner);
    frame(x, y, w, h, C_BROWN);
}

static void diary_page(int s) {
    const WthSpot *p = SP(s);
    if (!wth_spot_live(s)) return;
    gfx_rect(p->x + 2, p->y + 1, p->w - 4, p->h - 2, C_CREAM);
    for (int y = p->y + 3; y < p->y + p->h - 2; y += 2) gfx_hline(p->x + 3, p->x + p->w - 4, y, C_TAN);
    frame(p->x + 2, p->y + 1, p->w - 4, p->h - 2, C_BROWN);
}

/* a little page lying on some furniture (spots that hold a page among other things) */
static void page_on(int s, int x, int y) {
    for (int i = 0; i < WTH_PAGES; i++)
        if (WTH_PAGE_SPOT[i] == s && !((wth.s.pages >> i) & 1)) {
            gfx_rect(x, y, 10, 7, C_CREAM);
            gfx_hline(x + 1, x + 8, y + 2, C_TAN);
            gfx_hline(x + 1, x + 8, y + 4, C_TAN);
            frame(x, y, 10, 7, C_BROWN);
        }
}

static void night_sky(int y1) {
    gfx_rect(WTH_PX, WTH_PY, WTH_PW, y1 - WTH_PY, C_NIGHT);
    gfx_dither(WTH_PX, WTH_PY + (y1 - WTH_PY) / 2, WTH_PW, (y1 - WTH_PY) / 2, C_NAVY, 5);
    static const int16_t STARS[][2] = {{30, 12}, {64, 20}, {101, 9}, {140, 16}, {188, 11}, {222, 22}, {260, 8}, {296, 18}, {170, 26}, {48, 30}};
    for (int i = 0; i < ARRAY_LEN(STARS); i++)
        if (STARS[i][1] < y1) gfx_pset(STARS[i][0], STARS[i][1], (wth.frame / 40 + i) % 5 ? C_GREY : C_LIGHT);
}

/* ------------------------------------------------------------------ */
/* the rooms                                                            */

static void room_cell(void) {
    interior(C_SLATE, C_DUSK, C_EARTH, C_INK);
    bricks(73, 27, 247, 99, C_DUSK);
    for (int y = 10; y < 120; y += 7) gfx_hline(9, 60, y, C_NIGHT);
    /* the loose stone */
    const WthSpot *w = SP(S_CELL_WALL);
    if (!wth_flag(F_STONE_TAKEN)) {
        gfx_rect(w->x + 8, w->y + 5, 16, 11, C_GREY);
        frame(w->x + 8, w->y + 5, 16, 11, C_INK);
    } else gfx_rect(w->x + 8, w->y + 5, 16, 11, C_INK);
    /* the door, its hinges on this side */
    door_back(SP(S_CELL_DOOR)->x, 42, 48, 58, C_BROWN, wth_flag(F_CELL_OPEN));
    if (!wth_flag(F_CELL_OPEN)) {
        gfx_rect(136, 50, 4, 8, C_GREY);
        gfx_rect(136, 84, 4, 8, C_GREY);
        if (wth_flag(F_NAIL_SET)) gfx_vline(138, 58, 64, C_LIGHT);
    } else gfx_rect(140, 60, 6, 38, C_BROWN);
    /* the calendar */
    const WthSpot *c = SP(S_CELL_CALENDAR);
    gfx_rect(c->x, c->y, c->w, c->h, C_CREAM);
    gfx_rect(c->x, c->y, c->w, 8, C_RED);
    for (int k = 0; k < 4; k++) gfx_line(c->x + 3, c->y + 11 + k * 4, c->x + c->w - 3, c->y + 13 + k * 4, C_INK);
    /* straw */
    const WthSpot *st = SP(S_CELL_STRAW);
    for (int i = 0; i < 70; i++) {
        int x = st->x + (i * 37) % st->w, y = st->y + 4 + (i * 13) % (st->h - 6);
        gfx_line(x, y, x + 5, y - 2, i & 1 ? C_AMBER : C_TAN);
    }
    /* the barrel and its nail */
    const WthSpot *b = SP(S_CELL_BARREL);
    gfx_rect(b->x, b->y, b->w, b->h, C_BROWN);
    gfx_hline(b->x, b->x + b->w - 1, b->y + 8, C_GREY);
    gfx_hline(b->x, b->x + b->w - 1, b->y + b->h - 9, C_GREY);
    gfx_rect(b->x + 2, b->y - 3, b->w - 4, 4, C_EARTH);
    if (!wth_flag(F_NAIL_TAKEN)) gfx_vline(b->x + 16, b->y - 9, b->y - 3, C_LIGHT);
}

static void room_passage(void) {
    interior(C_MAROON, C_WINE, C_BROWN, C_NIGHT);
    wallpaper_stripes(C_WINE);
    for (int x = 12; x < 66; x += 9) gfx_line(x, 12, x, 120, C_MAROON);
    door_back(142, 42, 36, 58, C_EARTH, false);
    door_side(true, C_EARTH, false);
    door_side(false, C_EARTH, false);
    const WthSpot *c = SP(S_PAS_COAT);
    gfx_rect(c->x + 9, c->y, 4, 3, C_GREY);
    trap(c->x + 6, c->x + 16, c->x, c->x + c->w, c->y + 3, c->y + c->h - 1, C_FOREST);
    gfx_vline(c->x + 11, c->y + 6, c->y + c->h - 2, C_NIGHT);
    const WthSpot *r = SP(S_PAS_RUG);
    trap(r->x + 12, r->x + r->w - 12, r->x, r->x + r->w, r->y, r->y + r->h - 1, C_RED);
    trap(r->x + 16, r->x + r->w - 16, r->x + 6, r->x + r->w - 6, r->y + 3, r->y + r->h - 4, C_MAROON);
    page_on(S_PAS_RUG, r->x + 52, r->y + 5);
    const WthSpot *b = SP(S_PAS_BAROMETER);
    gfx_rect(b->x + 3, b->y, 8, b->h, C_BROWN);
    gfx_circ(b->x + 7, b->y + 9, 5, C_CREAM);
    gfx_line(b->x + 7, b->y + 9, b->x + 10, b->y + 6, C_INK);
}

static void room_sickroom(void) {
    interior(C_LIGHT, C_GREY, C_BROWN, C_SLATE);
    floorboards(C_EARTH);
    window_back(96, 32, 46, 34, C_NIGHT);
    for (int k = 0; k < 3; k++) gfx_line(94, 40 + k * 10, 144, 38 + k * 10, C_BROWN);
    const WthSpot *b = SP(S_SICK_BED);
    gfx_rect(b->x, b->y + 10, b->w, b->h - 18, C_LIGHT);
    gfx_rect(b->x, b->y + 24, b->w, 8, C_GREY);
    gfx_rect(b->x + b->w - 26, b->y + 12, 22, 10, C_WHITE);
    gfx_vline(b->x + b->w - 1, b->y, b->y + b->h - 1, C_SLATE);
    gfx_vline(b->x, b->y + 6, b->y + b->h - 1, C_SLATE);
    gfx_hline(b->x, b->x + b->w - 1, b->y + 6, C_SLATE);
    page_on(S_SICK_BED, b->x + b->w - 22, b->y + 13);
    const WthSpot *n = SP(S_SICK_STAND);
    gfx_rect(n->x, n->y, n->w, n->h, C_BROWN);
    frame(n->x + 3, n->y + 4, n->w - 6, 8, C_EARTH);
    gfx_pset(n->x + n->w / 2, n->y + 8, C_AMBER);
    for (int k = 0; k < 3; k++) gfx_rect(n->x + 5 + k * 7, n->y - 6, 4, 6, k == 1 ? C_AMBER : C_WHITE);
    const WthSpot *w = SP(S_SICK_WASH);
    gfx_rect(w->x + 4, w->y + 16, w->w - 8, w->h - 16, C_BROWN);
    gfx_rect(w->x, w->y + 12, w->w, 5, C_LIGHT);
    gfx_rect(w->x + 14, w->y, 12, 13, C_WHITE);
}

static void room_hall(void) {
    interior(C_FOREST, C_JADE, C_INK, C_NIGHT);
    for (int y = 101; y < 134; y += 4)
        for (int x = 8; x < 312; x += 8) gfx_rect(x + ((y / 4) & 1) * 4, y, 4, 4, C_LIGHT);
    trap(72, 248, 8, 311, 100, 101, C_INK);
    door_back(138, 38, 44, 62, C_EARTH, false);
    gfx_rect(132, 66, 56, 4, C_GREY);
    for (int x = 140; x < 180; x += 5) gfx_circb(x, 68, 2, C_LIGHT);
    gfx_rect(156, 72, 8, 8, C_SLATE);
    /* the stopped clock, white threads in its case */
    const WthSpot *c = SP(S_HALL_CLOCK);
    gfx_rect(c->x, c->y, c->w, c->h, C_BROWN);
    gfx_rect(c->x - 2, c->y, c->w + 4, 4, C_EARTH);
    gfx_circ(c->x + c->w / 2, c->y + 14, 8, C_CREAM);
    gfx_vline(c->x + c->w / 2, c->y + 14, c->y + 21, C_INK);
    gfx_rect(c->x + 6, c->y + 30, c->w - 12, 30, C_EARTH);
    gfx_dither(c->x + 6, c->y + 30, c->w - 12, 30, C_WHITE, 5);
    /* the hall table and the photograph */
    const WthSpot *t = SP(S_HALL_TABLE);
    gfx_rect(t->x, t->y + 6, t->w, 3, C_BROWN);
    gfx_rect(t->x + 2, t->y + 9, 2, 7, C_BROWN);
    gfx_rect(t->x + t->w - 4, t->y + 9, 2, 7, C_BROWN);
    if (!wth_flag(F_PHOTO_TAKEN)) gfx_rect(t->x + 9, t->y + 3, 10, 3, C_CREAM);
    /* the umbrella stand, a pump handle among the sticks */
    const WthSpot *u = SP(S_HALL_UMBRELLA);
    gfx_rect(u->x + 2, u->y + 12, 12, 14, C_GREY);
    gfx_line(u->x + 4, u->y + 13, u->x + 2, u->y, C_TAN);
    gfx_line(u->x + 9, u->y + 13, u->x + 10, u->y + 1, C_BROWN);
    if (!wth_flag(F_HANDLE_TAKEN)) {
        gfx_line(u->x + 12, u->y + 13, u->x + 14, u->y + 2, C_SLATE);
        gfx_rect(u->x + 12, u->y + 1, 4, 2, C_SLATE);
    }
    door_back(198, 42, 30, 58, C_EARTH, false);
    door_side(true, C_EARTH, false);
    for (int k = 0; k < 9; k++) {
        int y = 118 - k * 10, x = 254 + k * 3;
        gfx_rect(x, y, 56 - k * 3, 4, C_BROWN);
        gfx_rect(x, y + 4, 56 - k * 3, 6, C_EARTH);
    }
    if (!wth_flag(F_STAIRS_LIT)) gfx_dither(254, 26, 54, 50, C_INK, 14);
    gfx_line(254, 118, 290, 26, C_BROWN);
}

static void room_parlour(void) {
    interior(C_WINE, C_MAROON, C_BROWN, C_NIGHT);
    wallpaper_stripes(C_MAROON);
    floorboards(C_EARTH);
    trap(80, 240, 40, 280, 104, 128, C_RED);
    const WthSpot *f = SP(S_PAR_FIRE);
    gfx_rect(f->x - 6, 50, f->w + 12, 6, C_LIGHT);
    gfx_rect(f->x, 56, f->w, 44, C_GREY);
    gfx_rect(f->x + 12, 66, f->w - 24, 34, C_INK);
    gfx_hline(f->x + 14, f->x + f->w - 15, 96, C_SLATE);
    /* the fire irons */
    const WthSpot *ir = SP(S_PAR_IRONS);
    gfx_rect(ir->x + 2, ir->y + ir->h - 4, 10, 4, C_INK);
    gfx_vline(ir->x + 4, ir->y + 4, ir->y + ir->h - 4, C_SLATE);
    gfx_rect(ir->x + 2, ir->y + 2, 5, 4, C_SLATE);
    if (!wth_flag(F_POKER_TAKEN)) {
        gfx_vline(ir->x + 9, ir->y, ir->y + ir->h - 4, C_SLATE);
        gfx_pset(ir->x + 10, ir->y + 1, C_SLATE);
    }
    /* the radiogram */
    const WthSpot *r = SP(S_PAR_RADIO);
    gfx_rect(r->x, r->y, r->w, r->h, C_BROWN);
    gfx_rect(r->x + 3, r->y + 4, r->w - 6, 14, C_TAN);
    for (int x = r->x + 5; x < r->x + r->w - 5; x += 3) gfx_vline(x, r->y + 6, r->y + 15, C_EARTH);
    gfx_circ(r->x + 8, r->y + 26, 3, wth_flag(F_POWER) ? C_YELLOW : C_AMBER);
    gfx_circ(r->x + r->w - 9, r->y + 26, 3, C_AMBER);
    const WthSpot *so = SP(S_PAR_SOFA);
    gfx_rect(so->x, so->y + 6, so->w, so->h - 6, C_PURPLE);
    gfx_rect(so->x - 4, so->y + 2, 8, so->h - 2, C_VIOLET);
    gfx_rect(so->x + so->w - 4, so->y + 2, 8, so->h - 2, C_VIOLET);
    gfx_rect(so->x + 6, so->y, so->w - 12, 8, C_VIOLET);
    page_on(S_PAR_SOFA, so->x + 30, so->y + 10);
    door_side(true, C_EARTH, false);
    door_side(false, C_EARTH, false);
}

static void room_dining(void) {
    interior(C_NAVY, C_NIGHT, C_BROWN, C_INK);
    for (int x = 78; x < 248; x += 16) gfx_vline(x, 70, 99, C_NIGHT);
    gfx_hline(72, 248, 70, C_BROWN);
    floorboards(C_EARTH);
    /* the pianola */
    const WthSpot *o = SP(S_DIN_PIANOLA);
    gfx_rect(o->x, o->y + 10, o->w, o->h - 10, C_BROWN);
    gfx_rect(o->x + 4, o->y + 16, o->w - 8, 14, C_CREAM);
    for (int y = o->y + 18; y < o->y + 29; y += 3) gfx_hline(o->x + 6, o->x + o->w - 7, y, C_SLATE);
    gfx_rect(o->x + 2, o->y + 40, o->w - 4, 5, C_WHITE);
    for (int x = o->x + 4; x < o->x + o->w - 4; x += 3) gfx_rect(x, o->y + 40, 2, 3, C_INK);
    /* the portrait: the old man and his four bees */
    const WthSpot *p = SP(S_DIN_PORTRAIT);
    picture_frame(p->x, p->y, p->w, p->h, C_DUSK);
    gfx_circ(p->x + 25, p->y + 11, 5, C_TAN);
    gfx_rect(p->x + 18, p->y + 17, 14, 13, C_INK);
    gfx_rect(p->x + 21, p->y + 16, 8, 3, C_WHITE);
    for (int k = 0; k < 4; k++) gfx_pset(p->x + 8 + k * 11, p->y + 22 - (k & 1) * 3, C_YELLOW);
    /* the sideboard */
    const WthSpot *s = SP(S_DIN_SIDE);
    gfx_rect(s->x, s->y, s->w, s->h, C_BROWN);
    gfx_hline(s->x, s->x + s->w - 1, s->y + 10, C_EARTH);
    gfx_pset(s->x + s->w / 2, s->y + 5, C_AMBER);
    if (!wth_flag(F_HONEY_TAKEN)) {
        gfx_rect(s->x + 6, s->y - 9, 8, 9, C_CREAM);
        gfx_rect(s->x + 6, s->y - 10, 8, 2, C_AMBER);
    }
    diary_page(S_DIN_DIARY);
    /* the long table under its cloth */
    const WthSpot *t = SP(S_DIN_TABLE);
    if (wth_flag(F_HIDE_TABLE_GONE)) {
        gfx_rect(t->x + 40, t->y + 4, 100, 18, C_BROWN);
        for (int k = 0; k < 6; k++) gfx_line(t->x + 10 + k * 26, t->y + 18, t->x + 20 + k * 26, t->y + 23, C_CREAM);
    } else {
        trap(t->x + 20, t->x + t->w - 20, t->x, t->x + t->w, t->y, t->y + 12, C_CREAM);
        gfx_rect(t->x, t->y + 12, t->w, t->h - 12, C_CREAM);
        for (int x = t->x + 6; x < t->x + t->w; x += 12) gfx_vline(x, t->y + 13, t->y + t->h - 1, C_LIGHT);
        for (int k = 0; k < 4; k++) {
            int px = t->x + 30 + k * 34;
            gfx_circb(px, t->y + 6, 5, C_LIGHT);
            gfx_circ(px, t->y + 6, 2, C_AMBER);
        }
    }
    door_side(false, C_EARTH, false);
}

static void room_kitchen(void) {
    interior(C_CREAM, C_TAN, C_GREY, C_BROWN);
    for (int y = 30; y < 70; y += 6) gfx_hline(73, 247, y, C_TAN);
    for (int y = 104; y < 134; y += 8) gfx_hline(8, 311, y, C_SLATE);
    /* the pantry door, slatted */
    const WthSpot *pn = SP(S_KIT_PANTRY);
    if (wth_flag(F_HIDE_PANTRY_GONE)) {
        gfx_rect(pn->x, pn->y, pn->w, pn->h, C_INK);
        gfx_line(pn->x + 2, pn->y + 4, pn->x + 10, pn->y + pn->h - 2, C_BROWN);
    } else {
        gfx_rect(pn->x, pn->y, pn->w, pn->h, C_BROWN);
        for (int y = pn->y + 4; y < pn->y + pn->h - 2; y += 4) gfx_hline(pn->x + 3, pn->x + pn->w - 4, y, C_EARTH);
    }
    door_back(204, 42, 30, 58, C_BROWN, false);
    door_side(true, C_BROWN, false);
    /* the salt box */
    const WthSpot *sb = SP(S_KIT_SALT);
    gfx_rect(sb->x, sb->y, sb->w, sb->h, C_WHITE);
    gfx_rect(sb->x, sb->y, sb->w, 4, C_SKY);
    const WthSpot *b = SP(S_KIT_BOOK);
    gfx_rect(b->x - 4, b->y + b->h, b->w + 8, 2, C_BROWN);
    for (int k = 0; k < 8; k++) gfx_rect(b->x + k * 6, b->y + 2 + (k % 3), 5, b->h - 2 - (k % 3), k & 1 ? C_RED : C_FOREST);
    page_on(S_KIT_BOOK, b->x + 40, b->y + 4);
    const WthSpot *s = SP(S_KIT_SINK);
    gfx_rect(s->x, s->y + 10, s->w, s->h - 10, C_LIGHT);
    gfx_rect(s->x + 4, s->y + 12, s->w - 8, 8, C_GREY);
    for (int k = 0; k < 6; k++) {
        gfx_rect(s->x + 6 + k * 9, s->y + 2 + (k & 1) * 2, 7, 10, C_AMBER);
        gfx_rect(s->x + 6 + k * 9, s->y + 1 + (k & 1) * 2, 7, 2, C_CREAM);
    }
    trap(250, 311, 256, 311, 74, 100, C_BROWN);
    const WthSpot *k = SP(S_KIT_KNIVES);
    gfx_rect(k->x + 4, k->y + 4, 14, 12, C_EARTH);
    for (int i = 0; i < 4; i++)
        gfx_rect(k->x + 6 + i * 3, k->y - 2 + (i == 0 && !wth_flag(F_KNIFE_TAKEN) ? -3 : 0), 2,
                 6 + (i == 0 && !wth_flag(F_KNIFE_TAKEN) ? 3 : 0), i == 0 && !wth_flag(F_KNIFE_TAKEN) ? C_INK : C_EARTH);
}

static void room_scullery(void) {
    interior(C_LIGHT, C_GREY, C_SLATE, C_GREY);
    bricks(73, 27, 247, 99, C_GREY);
    door_back(90, 42, 30, 58, C_FOREST, false);
    door_side(false, C_SLATE, false);
    /* the fuse box */
    const WthSpot *f = SP(S_SCU_FUSEBOX);
    gfx_rect(f->x, f->y, f->w, f->h, C_INK);
    for (int k = 0; k < 4; k++) gfx_rect(f->x + 3 + k * 5, f->y + 6, 3, 8, k == 2 && !wth_flag(F_POWER) ? C_INK : C_WHITE);
    frame(f->x, f->y, f->w, f->h, C_SLATE);
    diary_page(S_SCU_DIARY);
    /* the ice chest */
    const WthSpot *ib = SP(S_SCU_ICEBOX);
    gfx_rect(ib->x, ib->y + 8, ib->w, ib->h - 8, C_BROWN);
    gfx_rect(ib->x + 4, ib->y + 2, ib->w - 8, 10, C_ICE);
    gfx_rect(ib->x - 2, ib->y + 6, ib->w + 4, 4, C_EARTH);
    if (!wth_flag(F_BEE_ICE)) gfx_rect(ib->x + ib->w / 2 - 2, ib->y + 5, 4, 3, C_YELLOW);
    gfx_rect(ib->x + ib->w - 8, ib->y + 20, 4, 6, C_GREY);
    /* the stone sink and its pump */
    const WthSpot *s = SP(S_SCU_SINK);
    gfx_rect(s->x, s->y + 12, s->w, s->h - 12, C_LIGHT);
    gfx_rect(s->x + 4, s->y + 14, s->w - 8, 6, C_SLATE);
    gfx_rect(s->x + s->w / 2 - 2, s->y - 6, 4, 18, C_INK);
    gfx_line(s->x + s->w / 2, s->y - 6, s->x + s->w / 2 + 14, s->y - 14, C_INK);
    if (!wth_flag(F_BUCKET_TAKEN)) {
        const WthSpot *bu = SP(S_SCU_BUCKET);
        trap(bu->x + 2, bu->x + bu->w - 2, bu->x + 4, bu->x + bu->w - 4, bu->y + 2, bu->y + bu->h - 1, C_LIGHT);
        gfx_hline(bu->x + 2, bu->x + bu->w - 3, bu->y + 2, C_GREY);
    }
    /* the dresser drawer */
    const WthSpot *d = SP(S_SCU_DRAWER);
    gfx_rect(d->x, d->y - 14, d->w, d->h + 26, C_BROWN);
    frame(d->x + 2, d->y, d->w - 4, d->h, C_EARTH);
    gfx_pset(d->x + d->w / 2, d->y + d->h / 2, C_AMBER);
}

static void room_smoking(void) {
    interior(C_EARTH, C_BROWN, C_BROWN, C_INK);
    for (int x = 76; x < 248; x += 12) gfx_vline(x, 60, 99, C_BROWN);
    gfx_hline(72, 248, 60, C_TAN);
    floorboards(C_EARTH);
    trap(84, 236, 44, 276, 104, 130, C_FOREST);
    door_side(true, C_BROWN, false);
    door_side(false, C_BROWN, false);
    /* the gun cabinet */
    const WthSpot *a = SP(S_SMO_CABINET);
    gfx_rect(a->x, a->y, a->w, a->h, C_BROWN);
    gfx_rect(a->x + 4, a->y + 4, a->w - 8, a->h - 16, C_NIGHT);
    if (!wth_flag(F_CABINET_BROKEN)) {
        gfx_line(a->x + 8, a->y + 20, a->x + a->w - 8, a->y + 20, C_BROWN);
        gfx_line(a->x + a->w - 12, a->y + 12, a->x + a->w - 9, a->y + 30, C_EARTH);
        gfx_dither(a->x + 4, a->y + 4, a->w - 8, a->h - 16, C_ICE, 2);
        frame(a->x + 4, a->y + 4, a->w - 8, a->h - 16, C_ICE);
    } else {
        gfx_line(a->x + 5, a->y + 6, a->x + 14, a->y + 18, C_WHITE);
        gfx_line(a->x + 26, a->y + 8, a->x + 18, a->y + 24, C_WHITE);
    }
    gfx_pset(a->x + 10, a->y + 24, C_SLATE);
    gfx_pset(a->x + a->w - 10, a->y + 24, C_SLATE);
    const WthSpot *d = SP(S_SMO_DESK);
    gfx_rect(d->x, d->y, d->w, 6, C_TAN);
    gfx_rect(d->x + 2, d->y + 6, d->w - 4, d->h - 6, C_BROWN);
    frame(d->x + 8, d->y + 9, d->w - 16, 8, C_EARTH);
    gfx_rect(d->x + 10, d->y - 4, 12, 4, C_CREAM);
    /* the pipe rack */
    const WthSpot *h = SP(S_SMO_PIPES);
    gfx_rect(h->x, h->y + 6, h->w, 4, C_BROWN);
    for (int k = 0; k < 4; k++) gfx_vline(h->x + 3 + k * 6, h->y, h->y + 6, C_EARTH);
    if (!wth_flag(F_MATCHES_TAKEN)) {
        gfx_rect(h->x + 6, h->y + 11, 10, 5, C_YELLOW);
        gfx_hline(h->x + 6, h->x + 15, h->y + 13, C_RED);
    }
    gfx_rect(h->x + 6, h->y + 16, 12, 14, C_BROWN);
    const WthSpot *c = SP(S_SMO_CHESS);
    trap(c->x + 6, c->x + c->w - 6, c->x, c->x + c->w, c->y, c->y + 8, C_CREAM);
    for (int k = 0; k < 6; k++) gfx_rect(c->x + 6 + k * 6, c->y + 2 + (k & 1), 3, 4, k < 4 ? C_WHITE : C_INK);
    const WthSpot *ch = SP(S_SMO_CHAIR);
    gfx_rect(ch->x + 6, ch->y, ch->w - 12, ch->h - 6, C_VIOLET);
    gfx_rect(ch->x, ch->y + 10, 10, ch->h - 10, C_PURPLE);
    gfx_rect(ch->x + ch->w - 10, ch->y + 10, 10, ch->h - 10, C_PURPLE);
    gfx_rect(ch->x + 8, ch->y + 16, ch->w - 16, 8, C_PURPLE);
    page_on(S_SMO_CHAIR, ch->x + 22, ch->y + 16);
}

static void tiles(int x0, int y0, int x1, int y1, int col) {
    for (int y = y0; y < y1; y += 8) gfx_hline(x0, x1, y, col);
    for (int x = x0; x < x1; x += 8) gfx_vline(x, y0, y1, col);
}

static void toilet(int s) {
    const WthSpot *t = SP(s);
    gfx_rect(t->x + 4, t->y, t->w - 8, 14, C_WHITE);
    gfx_rect(t->x + 2, t->y + 22, t->w - 4, 8, C_WHITE);
    gfx_rect(t->x + 8, t->y + 30, t->w - 16, 12, C_LIGHT);
    gfx_vline(t->x + t->w - 6, t->y - 18, t->y, C_GREY);
    gfx_rect(t->x + t->w - 8, t->y - 2, 4, 4, C_GREY);
}

static void room_washroom(void) {
    interior(C_TEAL, C_FOREST, C_LIGHT, C_NIGHT);
    tiles(73, 50, 247, 99, C_FOREST);
    for (int y = 101; y < 134; y += 4)
        for (int x = 8; x < 312; x += 8) gfx_rect(x + ((y / 4) & 1) * 4, y, 4, 4, C_SLATE);
    /* a hip bath of grey water */
    const WthSpot *b = SP(S_WAS_BATH);
    gfx_circ(b->x + b->w / 2, b->y + 20, 22, C_WHITE);
    gfx_rect(b->x + b->w / 2 - 22, b->y + 20, 44, 14, C_WHITE);
    gfx_circ(b->x + b->w / 2, b->y + 16, 16, C_GREY);
    gfx_rect(b->x + b->w / 2 - 6, b->y + 12, 12, 4, C_LIGHT);
    const WthSpot *c = SP(S_WAS_CAB);
    gfx_rect(c->x, c->y, c->w, c->h, C_WHITE);
    frame(c->x, c->y, c->w, c->h, C_GREY);
    gfx_vline(c->x + c->w / 2, c->y, c->y + c->h - 1, C_GREY);
    toilet(S_WAS_TOILET);
    const WthSpot *ci = SP(S_WAS_CISTERN);
    gfx_rect(ci->x, ci->y, ci->w, ci->h, C_SLATE);
    gfx_rect(ci->x - 2, ci->y - 2, ci->w + 4, 3, wth_flag(F_TAPE_TAKEN) ? C_GREY : C_SLATE);
}

static void room_guest(void) {
    interior(C_PURPLE, C_VIOLET, C_BROWN, C_NIGHT);
    for (int x = 80; x < 248; x += 14)
        for (int y = 32; y < 98; y += 14) gfx_pset(x, y, C_VIOLET);
    floorboards(C_EARTH);
    window_back(SP(S_GUE_WINDOW)->x, SP(S_GUE_WINDOW)->y, 40, 30, C_NIGHT);
    const WthSpot *b = SP(S_GUE_BED);
    gfx_rect(b->x, b->y + 8, b->w, 16, C_CREAM);
    gfx_rect(b->x, b->y + 24, b->w, b->h - 28, C_PINK);
    for (int x = b->x + 4; x < b->x + b->w; x += 6) gfx_vline(x, b->y + 26, b->y + b->h - 6, C_MAGENTA);
    gfx_rect(b->x + 6, b->y + 2, 24, 10, C_WHITE);
    gfx_rect(b->x - 2, b->y - 8, 4, b->h + 4, C_BROWN);
    /* the dresser */
    const WthSpot *w = SP(S_GUE_DRESSER);
    gfx_rect(w->x, w->y + 4, w->w, w->h + 20, C_BROWN);
    frame(w->x + 3, w->y + 8, w->w - 6, 8, C_EARTH);
    gfx_rect(w->x + 8, w->y - 18, w->w - 16, 20, C_EARTH);
    page_on(S_GUE_DRESSER, w->x + 14, w->y);
    /* the sewing box on a stool */
    const WthSpot *s = SP(S_GUE_SEW);
    gfx_rect(s->x + 6, s->y + 22, s->w - 12, 4, C_BROWN);
    gfx_rect(s->x + 8, s->y + 26, 3, s->h - 26, C_BROWN);
    gfx_rect(s->x + s->w - 11, s->y + 26, 3, s->h - 26, C_BROWN);
    gfx_rect(s->x + 8, s->y + 10, s->w - 16, 12, C_RED);
    gfx_rect(s->x + 8, s->y + 8, s->w - 16, 3, C_MAROON);
    for (int k = 0; k < 4; k++) gfx_pset(s->x + 12 + k * 6, s->y + 14, C_WHITE);
}

static void room_glasshouse(void) {
    night_sky(134);
    gfx_rect(8, 100, 304, 34, C_SLATE);
    for (int y = 104; y < 134; y += 6) gfx_hline(8, 311, y, C_DUSK);
    for (int x = 8; x < 312; x += 30) gfx_vline(x, 6, 100, C_LIGHT);
    for (int y = 30; y < 100; y += 24) gfx_hline(8, 311, y, C_LIGHT);
    gfx_line(8, 30, 160, 6, C_LIGHT);
    gfx_line(311, 30, 160, 6, C_LIGHT);
    const WthSpot *p = SP(S_GLA_PANE);
    gfx_line(p->x + 4, p->y + 4, p->x + 20, p->y + 22, C_WHITE);
    gfx_line(p->x + 20, p->y + 22, p->x + 38, p->y + 14, C_WHITE);
    gfx_line(p->x + 20, p->y + 22, p->x + 24, p->y + 38, C_WHITE);
    const WthSpot *d = SP(S_GLA_GARDEN);
    frame(d->x, d->y, d->w, d->h, C_WHITE);
    gfx_vline(d->x + d->w / 2, d->y, d->y + d->h - 1, C_WHITE);
    gfx_dither(d->x + 1, d->y + 30, d->w - 2, d->h - 31, C_FOREST, 8);
    /* the lamp on its hook */
    const WthSpot *l = SP(S_GLA_LAMP);
    gfx_vline(l->x + 11, l->y, l->y + 4, C_SLATE);
    if (!wth_flag(F_LAMP_TAKEN)) {
        gfx_circb(l->x + 11, l->y + 7, 3, C_SLATE);
        gfx_rect(l->x + 6, l->y + 10, 11, 12, C_ICE);
        frame(l->x + 6, l->y + 10, 11, 12, C_RED);
        gfx_rect(l->x + 5, l->y + 22, 13, 5, C_RED);
    }
    /* the dead plants */
    const WthSpot *pl = SP(S_GLA_PLANTS);
    gfx_rect(pl->x, pl->y + 20, pl->w, pl->h - 20, C_BROWN);
    for (int k = 0; k < 6; k++) {
        int x = pl->x + 8 + k * 13;
        gfx_vline(x, pl->y - 10, pl->y + 20, C_TAN);
        gfx_dither(x - 4, pl->y - 6, 9, 24, C_WHITE, 4);
    }
    /* the sack of blue crystals */
    const WthSpot *su = SP(S_GLA_SULPHATE);
    gfx_circ(su->x + su->w / 2, su->y + su->h - 12, 14, C_TAN);
    gfx_rect(su->x + 10, su->y + 2, su->w - 20, 10, C_TAN);
    gfx_dither(su->x + 12, su->y + 2, su->w - 24, 6, C_BLUE, 10);
    /* the potting tray */
    const WthSpot *t = SP(S_GLA_TROWEL);
    gfx_rect(t->x, t->y + 6, t->w, t->h - 6, C_BROWN);
    if (!wth_flag(F_TROWEL_TAKEN)) {
        gfx_rect(t->x + 6, t->y + 2, 10, 4, C_LIGHT);
        gfx_rect(t->x + 16, t->y + 3, 8, 2, C_BROWN);
    }
}

static void room_garage(void) {
    interior(C_GREY, C_SLATE, C_SLATE, C_DUSK);
    for (int y = 104; y < 134; y += 10) gfx_hline(8, 311, y, C_DUSK);
    if (wth_flag(F_GARAGE_OPEN)) {
        gfx_rect(76, 28, 168, 72, C_NIGHT);
        gfx_dither(76, 70, 168, 30, C_DUSK, 8);
        gfx_rect(76, 28, 168, 6, C_LIGHT);
    } else {
        gfx_rect(76, 28, 168, 72, C_LIGHT);
        for (int y = 34; y < 100; y += 8) gfx_hline(76, 243, y, C_GREY);
        if (!wth_flag(F_CHAIN_CUT)) {
            for (int y = 60; y < 100; y += 4) gfx_circb(160, y, 2, C_INK);
            gfx_rect(157, 96, 6, 5, C_SLATE);
        }
    }
    /* the inspection pit */
    const WthSpot *pit = SP(S_GAR_PIT);
    trap(pit->x + 10, pit->x + pit->w - 6, pit->x, pit->x + pit->w, pit->y, pit->y + pit->h - 1, C_INK);
    gfx_hline(pit->x + 10, pit->x + pit->w - 6, pit->y, C_GREY);
    /* the sack of quicklime */
    const WthSpot *sk = SP(S_GAR_SACK);
    gfx_circ(sk->x + 16, sk->y + 22, 14, C_CREAM);
    gfx_rect(sk->x + 8, sk->y + 2, 16, 10, C_CREAM);
    gfx_dither(sk->x + 6, sk->y + 20, 20, 10, C_WHITE, 8);
    /* the tool board */
    gfx_rect(250, 34, 50, 30, C_TAN);
    for (int x = 252; x < 300; x += 4)
        for (int y = 36; y < 62; y += 4) gfx_pset(x, y, C_BROWN);
    if (!wth_flag(F_SCREW_TAKEN)) {
        gfx_rect(256, 44, 6, 3, C_RED);
        gfx_hline(262, 271, 45, C_LIGHT);
    }
    if (!wth_flag(F_GREASE_TAKEN)) gfx_rect(280, 48, 12, 6, C_INK);
    const WthSpot *s = SP(S_GAR_SHELF);
    gfx_rect(s->x, s->y + s->h - 2, s->w, 3, C_BROWN);
    for (int k = 0; k < 3; k++) gfx_rect(s->x + 2 + k * 6, s->y + 6, 5, 12, C_AMBER);
    if (!wth_flag(F_PARAFFIN_TAKEN)) gfx_rect(s->x + 22, s->y + 2, 10, 16, C_RED);
    /* the car, its bonnet up if the rotor is in */
    const WthSpot *c = SP(S_GAR_CAR);
    gfx_rect(c->x + 20, c->y + 4, c->w - 40, 20, C_FOREST);
    gfx_rect(c->x + 26, c->y + 8, c->w - 52, 12, C_NIGHT);
    gfx_rect(c->x + 4, c->y + 22, c->w - 8, 22, C_FOREST);
    gfx_hline(c->x + 4, c->x + c->w - 5, c->y + 30, C_JADE);
    gfx_rect(c->x + 10, c->y + 26, 14, 6, C_CREAM);
    gfx_rect(c->x + c->w - 24, c->y + 26, 14, 6, C_CREAM);
    gfx_rect(c->x + 40, c->y + 34, c->w - 80, 8, C_LIGHT);
    gfx_rect(c->x + 8, c->y + 44, 18, 10, C_INK);
    gfx_rect(c->x + c->w - 26, c->y + 44, 18, 10, C_INK);
    if (wth_flag(F_ROTOR_FITTED)) gfx_rect(c->x + 30, c->y + 14, c->w - 60, 10, C_JADE);
}

static void room_pond(void) {
    night_sky(56);
    gfx_rect(8, 56, 304, 78, C_FOREST);
    gfx_dither(8, 56, 304, 20, C_INK, 6);
    gfx_circ(160, 92, 70, C_NAVY);
    gfx_rect(8, 112, 304, 22, C_FOREST);
    gfx_circ(160, 92, 62, C_NIGHT);
    for (int k = 0; k < 6; k++) gfx_hline(110 + k * 7, 140 + k * 9, 70 + k * 6, C_NAVY);
    /* the sunken boat */
    const WthSpot *b = SP(S_POND_BOAT);
    trap(b->x, b->x + b->w, b->x + 10, b->x + b->w - 8, b->y + 6, b->y + b->h - 4, C_BROWN);
    gfx_rect(b->x + 30, b->y + 8, 24, 3, C_TAN);
    gfx_dither(b->x + 6, b->y + 12, b->w - 12, b->h - 14, C_NAVY, 10);
    if (!wth_flag(F_TACKLE_TAKEN)) gfx_rect(b->x + 36, b->y + 3, 12, 5, C_JADE);
    const WthSpot *l = SP(S_POND_LILIES);
    for (int k = 0; k < 5; k++) {
        gfx_circ(l->x + 8 + k * 13, l->y + 8 + (k & 1) * 5, 5, C_FOREST);
        gfx_circ(l->x + 8 + k * 13, l->y + 6 + (k & 1) * 5, 2, C_WHITE);
    }
    const WthSpot *r = SP(S_POND_REEDS);
    for (int k = 0; k < 14; k++) gfx_line(r->x + k * 4, r->y + r->h, r->x + k * 4 + 2, r->y + (k * 7) % 14, C_LEAF);
    gfx_rect(r->x + 30, r->y + 10, 4, 12, C_GREY);
    gfx_line(r->x + 32, r->y + 10, r->x + 36, r->y + 4, C_GREY);
}

static void room_shed(void) {
    interior(C_BROWN, C_EARTH, C_EARTH, C_INK);
    for (int x = 72; x < 248; x += 8) gfx_vline(x, 27, 99, C_EARTH);
    for (int x = 12; x < 66; x += 9) gfx_line(x, 10, x, 122, C_BROWN);
    gfx_rect(20, 92, 120, 6, C_TAN);
    gfx_rect(24, 98, 4, 26, C_TAN);
    gfx_rect(132, 98, 4, 26, C_TAN);
    const WthSpot *g = SP(S_SHE_GRINDER);
    gfx_circb(g->x + 14, g->y + 12, 8, C_GREY);
    gfx_rect(g->x + 10, g->y + 22, 24, 4, C_SLATE);
    diary_page(S_SHE_DIARY);
    gfx_rect(140, 30, 90, 36, C_TAN);
    for (int x = 142; x < 228; x += 5)
        for (int y = 32; y < 64; y += 5) gfx_pset(x, y, C_BROWN);
    if (!wth_flag(F_PLIERS_TAKEN)) {
        gfx_line(149, 36, 152, 56, C_SLATE);
        gfx_line(155, 36, 152, 56, C_SLATE);
    }
    if (!wth_flag(F_CUTTERS_TAKEN)) {
        gfx_line(168, 36, 178, 58, C_RED);
        gfx_line(180, 36, 170, 58, C_RED);
        gfx_rect(172, 34, 4, 4, C_LIGHT);
    }
    if (!wth_flag(F_SMOKER_TAKEN)) {
        const WthSpot *sm = SP(S_SHE_SMOKER);
        gfx_rect(sm->x + 4, sm->y + 4, 10, 12, C_LIGHT);
        gfx_rect(sm->x + 6, sm->y, 6, 4, C_LIGHT);
        gfx_rect(sm->x + 15, sm->y + 6, 10, 8, C_BROWN);
    }
    if (!wth_flag(F_SPRAYER_TAKEN)) {
        const WthSpot *sp = SP(S_SHE_SPRAYER);
        gfx_rect(sp->x + 3, sp->y + 20, 16, 34, C_JADE);
        gfx_vline(sp->x + 11, sp->y + 4, sp->y + 20, C_SLATE);
        gfx_line(sp->x + 11, sp->y + 4, sp->x + 2, sp->y, C_SLATE);
    }
    const WthSpot *k = SP(S_SHE_GLOVES);
    gfx_circ(k->x + k->w / 2, k->y + k->h - 12, 14, C_TAN);
    if (!wth_flag(F_GLOVES_TAKEN)) {
        gfx_rect(k->x + 10, k->y + 4, 8, 8, C_BROWN);
        gfx_rect(k->x + 20, k->y + 6, 8, 8, C_BROWN);
    }
}

static void house_wall(int x0, int x1, int y0) {
    gfx_rect(x0, y0, x1 - x0, 134 - y0, C_DUSK);
    bricks(x0, y0, x1, 100, C_NIGHT);
}

static void room_garden(void) {
    night_sky(60);
    gfx_rect(8, 60, 304, 74, C_FOREST);
    gfx_dither(8, 60, 304, 30, C_INK, 7);
    house_wall(8, 100, 6);
    const WthSpot *sd = SP(S_GDN_SCULLERY);
    gfx_rect(sd->x, sd->y, sd->w, sd->h, C_FOREST);
    frame(sd->x, sd->y, sd->w, sd->h, C_INK);
    const WthSpot *g = SP(S_GDN_GLASS);
    gfx_rect(g->x, g->y, g->w, g->h, C_NIGHT);
    for (int x = g->x; x < g->x + g->w; x += 9) gfx_vline(x, g->y, g->y + g->h - 1, C_LIGHT);
    gfx_hline(g->x, g->x + g->w - 1, g->y, C_LIGHT);
    gfx_dither(g->x + 1, g->y + 20, g->w - 2, g->h - 21, C_WHITE, 3);
    /* the well */
    const WthSpot *w = SP(S_GDN_WELL);
    gfx_rect(w->x + 4, w->y + 10, w->w - 8, w->h - 10, C_GREY);
    bricks(w->x + 4, w->y + 10, w->x + w->w - 4, w->y + w->h, C_SLATE);
    gfx_rect(w->x, w->y + 6, w->w, 5, C_LIGHT);
    if (!wth_flag(F_GRATE_CUT)) {
        for (int x = w->x + 3; x < w->x + w->w - 2; x += 5) gfx_vline(x, w->y + 2, w->y + 6, C_INK);
        gfx_hline(w->x + 2, w->x + w->w - 3, w->y + 2, C_INK);
        gfx_rect(w->x + w->w / 2 - 2, w->y + 3, 5, 5, C_SLATE);
    } else gfx_rect(w->x + w->w - 6, w->y + w->h - 6, 12, 3, C_INK);
    /* the hose reel */
    const WthSpot *h = SP(S_GDN_HOSE);
    gfx_circb(h->x + 15, h->y + 11, 9, C_JADE);
    gfx_circb(h->x + 15, h->y + 11, 6, C_JADE);
    gfx_circ(h->x + 15, h->y + 11, 2, C_INK);
    if (!wth_flag(F_HOSE_TAKEN)) gfx_line(h->x + 22, h->y + 16, h->x + 30, h->y + 21, C_JADE);
    /* the hives */
    const WthSpot *hv = SP(S_GDN_HIVES);
    for (int k = 0; k < 3; k++) {
        gfx_rect(hv->x + k * 15, hv->y + 4, 12, 14, C_CREAM);
        gfx_rect(hv->x + k * 15 - 1, hv->y + 2, 14, 3, C_LIGHT);
        gfx_hline(hv->x + k * 15 + 3, hv->x + k * 15 + 8, hv->y + 16, C_WHITE);
    }
    if (wth_flag(F_HIVE_CUT)) gfx_rect(hv->x - 1, hv->y, 14, 3, C_AMBER);
    const WthSpot *p = SP(S_GDN_POND);
    trap(p->x + 8, p->x + p->w - 8, p->x, p->x + p->w, p->y, p->y + p->h - 1, C_EARTH);
    /* the wall and its arch to the hedge court */
    gfx_rect(226, 40, 46, 60, C_SLATE);
    bricks(226, 40, 272, 100, C_DUSK);
    const WthSpot *a = SP(S_GDN_ARCH);
    gfx_rect(a->x + 2, a->y + 6, a->w - 4, a->h - 6, C_INK);
    gfx_circ(a->x + a->w / 2, a->y + 8, a->w / 2 - 2, C_INK);
    gfx_dither(a->x + 2, a->y + 30, a->w - 4, 26, C_FOREST, 6);
    const WthSpot *s = SP(S_GDN_SHEDDOOR);
    trap(s->x + s->w / 2, s->x + s->w / 2, s->x - 4, s->x + s->w + 4, s->y - 12, s->y, C_EARTH);
    gfx_rect(s->x, s->y, s->w, s->h, C_BROWN);
    for (int x = s->x; x < s->x + s->w; x += 5) gfx_vline(x, s->y, s->y + s->h - 1, C_EARTH);
    if (wth_flag(F_SHED_OPEN)) gfx_rect(s->x + 8, s->y + 10, s->w - 16, s->h - 10, C_INK);
    else gfx_rect(s->x + s->w - 9, s->y + 24, 4, 6, C_GREY);
}

static void room_yard(void) {
    night_sky(40);
    gfx_rect(8, 40, 304, 94, C_DUSK);
    gfx_rect(8, 96, 304, 38, C_SLATE);
    for (int y = 98; y < 134; y += 6)
        for (int x = 8 + ((y / 6) & 1) * 5; x < 312; x += 10) gfx_pset(x, y, C_GREY);
    house_wall(8, 90, 20);
    /* the water butt */
    const WthSpot *b = SP(S_YRD_BUTT);
    gfx_rect(b->x + 2, b->y + 4, b->w - 4, b->h - 8, C_FOREST);
    gfx_rect(b->x, b->y, b->w, 5, C_JADE);
    gfx_hline(b->x + 2, b->x + b->w - 3, b->y + 20, C_JADE);
    gfx_rect(b->x + b->w / 2 - 2, b->y + b->h - 10, 5, 4, C_SLATE);
    if (wth_flag(F_BUTT_DRAINED)) gfx_dither(b->x - 6, b->y + b->h - 4, b->w + 30, 6, C_NAVY, 6);
    /* the kennel */
    const WthSpot *k = SP(S_YRD_KENNEL);
    gfx_rect(k->x + 4, k->y + 14, k->w - 8, k->h - 14, C_BROWN);
    trap(k->x + k->w / 2, k->x + k->w / 2, k->x, k->x + k->w, k->y, k->y + 14, C_EARTH);
    gfx_rect(k->x + k->w / 2 - 8, k->y + 20, 16, k->h - 20, C_INK);
    gfx_line(k->x + k->w - 6, k->y + k->h - 4, k->x + k->w + 6, k->y + k->h, C_GREY);
    if (!wth_flag(F_COLLAR_TAKEN)) gfx_pset(k->x + k->w + 6, k->y + k->h - 1, C_YELLOW);
    /* the pine */
    const WthSpot *p = SP(S_YRD_PINE);
    gfx_rect(p->x + 22, p->y + 50, 6, 30, C_EARTH);
    for (int i = 0; i < 4; i++) trap(p->x + 25, p->x + 25, p->x + 4 + i * 2, p->x + 46 - i * 2, p->y + i * 12, p->y + 20 + i * 12, C_INK);
    if (!wth_flag(F_CONES_TAKEN))
        for (int i = 0; i < 4; i++) gfx_rect(p->x + 10 + i * 9, p->y + 76, 3, 4, C_BROWN);
    /* the coal bunker */
    const WthSpot *c = SP(S_YRD_BUNKER);
    gfx_rect(c->x, c->y + 6, c->w, c->h - 6, C_GREY);
    bricks(c->x, c->y + 6, c->x + c->w, c->y + c->h, C_SLATE);
    trap(c->x + 2, c->x + c->w - 2, c->x - 2, c->x + c->w + 2, c->y, c->y + 6, C_INK);
    /* the path round to the drive */
    const WthSpot *d = SP(S_YRD_DRIVE);
    trap(d->x + 6, d->x + d->w, d->x, d->x + d->w, d->y + 10, d->y + d->h, C_GREY);
}

static void room_drive(void) {
    night_sky(40);
    gfx_rect(8, 90, 304, 44, C_GREY);
    gfx_dither(8, 90, 304, 44, C_SLATE, 8);
    gfx_rect(8, 20, 304, 80, C_DUSK);
    trap(60, 260, 8, 311, 8, 20, C_SLATE);
    bricks(8, 20, 311, 100, C_NIGHT);
    for (int k = 0; k < 2; k++) {
        int x = 86 + k * 102;
        gfx_rect(x, 50, 18, 24, C_BROWN);
        gfx_line(x, 52, x + 17, 70, C_EARTH);
    }
    const WthSpot *fd = SP(S_DRV_FRONT);
    door_back(fd->x, fd->y, fd->w, fd->h, C_EARTH, false);
    gfx_rect(fd->x + 2, fd->y + 26, fd->w - 4, 6, C_BROWN);
    const WthSpot *g = SP(S_DRV_GARAGE);
    if (wth_flag(F_GARAGE_OPEN)) {
        gfx_rect(g->x, g->y, g->w, g->h, C_INK);
        gfx_rect(g->x + 12, g->y + 24, g->w - 24, 22, C_FOREST);
    } else {
        gfx_rect(g->x, g->y, g->w, g->h, C_LIGHT);
        for (int y = g->y + 6; y < g->y + g->h; y += 8) gfx_hline(g->x, g->x + g->w - 1, y, C_GREY);
    }
    const WthSpot *t = SP(S_DRV_TRACKS);
    gfx_line(t->x, t->y + t->h, t->x + 30, t->y, C_EARTH);
    gfx_line(t->x + 20, t->y + t->h, t->x + 50, t->y, C_EARTH);
    const WthSpot *c = SP(S_DRV_HATCH);
    if (wth_flag(F_HATCH_OPEN)) {
        gfx_rect(c->x, c->y, c->w, c->h, C_INK);
        gfx_rect(c->x - 6, c->y - 6, 6, c->h + 6, C_BROWN);
        gfx_rect(c->x + c->w, c->y - 6, 6, c->h + 6, C_BROWN);
    } else {
        trap(c->x + 4, c->x + c->w - 4, c->x, c->x + c->w, c->y, c->y + c->h - 1, C_BROWN);
        gfx_vline(c->x + c->w / 2, c->y, c->y + c->h - 1, C_EARTH);
        gfx_rect(c->x + c->w / 2 - 3, c->y + 5, 6, 6, C_INK);
    }
    const WthSpot *ga = SP(S_DRV_GATES);
    for (int x = ga->x + 4; x < ga->x + ga->w; x += 6) gfx_vline(x, ga->y, ga->y + ga->h - 1, C_INK);
    gfx_hline(ga->x, ga->x + ga->w - 1, ga->y + 3, C_INK);
}

static void bee_shape(int x, int y, int col) {
    gfx_rect(x, y, 6, 4, col);
    gfx_pset(x + 1, y - 1, C_LIGHT);
    gfx_pset(x + 4, y - 1, C_LIGHT);
    gfx_vline(x + 2, y, y + 3, C_INK);
}

static void room_courtyard(void) {
    night_sky(30);
    gfx_rect(8, 30, 304, 74, C_SLATE);
    bricks(8, 30, 311, 104, C_GREY);
    gfx_rect(8, 104, 304, 30, C_GREY);
    for (int y = 106; y < 134; y += 6)
        for (int x = 8 + ((y / 6) & 1) * 6; x < 312; x += 12) gfx_rect(x, y, 10, 4, C_LIGHT);
    /* the stone hive with its four bee-shaped cells */
    const WthSpot *h = SP(S_CRT_HIVE);
    int cx = h->x + h->w / 2;
    for (int i = 0; i < 5; i++) gfx_rect(cx - 14 - i * 4 + 4, h->y + 4 + i * 10, 28 + i * 8 - 8, 10, C_LIGHT);
    for (int i = 0; i < 5; i++) gfx_hline(cx - 14 - i * 4 + 4, cx + 14 + i * 4 - 5, h->y + 4 + i * 10, C_GREY);
    gfx_rect(cx - 4, h->y + h->h - 12, 8, 8, C_INK);
    static const uint8_t COL[4] = {C_YELLOW, C_ORANGE, C_WHITE, C_SLATE};
    for (int k = 0; k < 4; k++) {
        int bx = cx - 18 + k * 10, by = h->y + 26;
        gfx_rect(bx - 1, by - 2, 8, 7, C_NIGHT);
        if ((wth.s.hive >> k) & 1) bee_shape(bx, by, COL[k]);
    }
    /* the sundial */
    const WthSpot *s = SP(S_CRT_SUNDIAL);
    gfx_rect(s->x + 16, s->y + 10, 12, s->h - 10, C_LIGHT);
    gfx_rect(s->x + 6, s->y + 6, 32, 5, C_LIGHT);
    gfx_line(s->x + 20, s->y + 6, s->x + 28, s->y, C_AMBER);
    /* the gate */
    const WthSpot *g = SP(S_CRT_GATE);
    gfx_rect(g->x, g->y, g->w, g->h, C_INK);
    gfx_dither(g->x, g->y, g->w, g->h, C_FOREST, 7);
    if (!wth_flag(F_GATE_OPEN)) {
        for (int x = g->x + 3; x < g->x + g->w; x += 6) gfx_vline(x, g->y, g->y + g->h - 1, C_LIGHT);
        gfx_hline(g->x, g->x + g->w - 1, g->y + 20, C_LIGHT);
        gfx_hline(g->x, g->x + g->w - 1, g->y + 56, C_LIGHT);
    } else
        for (int x = g->x + 3; x < g->x + 14; x += 6) gfx_vline(x, g->y, g->y + g->h - 1, C_LIGHT);
}

static void room_maze(void) {
    night_sky(24);
    gfx_rect(8, 24, 304, 110, C_FOREST);
    int v = wth.s.maze;
    gfx_rect(18, 40, 54, 80, C_INK);
    gfx_rect(248, 40, 54, 80, C_INK);
    gfx_rect(134, 40, 52, 60, C_INK);
    gfx_dither(134, 70, 52, 30, C_FOREST, 4);
    trap(134, 185, 72, 248, 100, 133, C_EARTH);
    for (int k = 0; k < 40; k++) {
        int x = 8 + (k * 53 + v * 17) % 304, y = 26 + (k * 29 + v * 11) % 100;
        if ((x > 18 && x < 72 && y > 40 && y < 120) || (x > 248 && x < 302 && y > 40 && y < 120) ||
            (x > 134 && x < 186 && y > 40 && y < 134))
            continue;
        gfx_pset(x, y, C_JADE);
        gfx_pset(x + 1, y, C_LEAF);
    }
    for (int k = 0; k <= v; k++) gfx_rect(96 + k * 5, 106, 3, 8, C_LIGHT);
    gfx_rect(94, 114, 6 + v * 5 + 4, 4, C_GREY);
}

static void room_heart(void) {
    night_sky(40);
    gfx_rect(8, 40, 304, 94, C_FOREST);
    gfx_dither(8, 40, 304, 40, C_INK, 6);
    gfx_circ(160, 120, 90, C_EARTH);
    gfx_rect(8, 126, 304, 8, C_FOREST);
    /* the old oak, stuck full of holes */
    const WthSpot *o = SP(S_HRT_OAK);
    int cx = o->x + o->w / 2;
    gfx_rect(cx - 10, o->y + 30, 20, o->h - 30, C_BROWN);
    gfx_circ(cx, o->y + 18, 26, C_INK);
    gfx_circ(cx - 16, o->y + 26, 14, C_NIGHT);
    gfx_circ(cx + 16, o->y + 24, 14, C_NIGHT);
    for (int k = 0; k < 9; k++) gfx_pset(cx - 7 + (k * 5) % 14, o->y + 40 + (k * 9) % 40, C_INK);
    if (!wth_flag(F_BOLT_TAKEN)) {
        gfx_line(cx + 4, o->y + 56, cx + 16, o->y + 52, C_TAN);
        gfx_rect(cx + 15, o->y + 51, 3, 3, C_RED);
    }
    const WthSpot *b = SP(S_HRT_BENCH);
    gfx_rect(b->x, b->y + 4, b->w, 8, C_GREY);
    gfx_rect(b->x + 6, b->y + 12, 8, 10, C_SLATE);
    gfx_rect(b->x + b->w - 14, b->y + 12, 8, 10, C_SLATE);
}

static void room_landing(void) {
    interior(C_WINE, C_MAROON, C_BROWN, C_INK);
    wallpaper_stripes(C_MAROON);
    floorboards(C_EARTH);
    door_back(80, 42, 30, 58, C_EARTH, false);
    door_back(145, 42, 30, 58, C_EARTH, false);
    door_back(210, 42, 30, 58, C_EARTH, false);
    door_side(true, C_EARTH, wth_flag(F_MASTER_OPEN));
    door_side(false, C_EARTH, false);
    const WthSpot *h = SP(S_LAN_HATCH);
    if (wth_flag(F_HATCH_DOWN)) {
        gfx_rect(h->x, h->y, h->w, h->h, C_INK);
        for (int k = 0; k < 6; k++) gfx_hline(h->x + 14, h->x + 40, h->y + 4 + k * 14, C_TAN);
        gfx_line(h->x + 14, h->y, h->x + 10, 100, C_TAN);
        gfx_line(h->x + 40, h->y, h->x + 44, 100, C_TAN);
    } else {
        gfx_rect(h->x, h->y, h->w, h->h, C_BROWN);
        frame(h->x, h->y, h->w, h->h, C_EARTH);
        gfx_circb(h->x + h->w / 2, h->y + h->h - 2, 2, C_GREY);
    }
    gfx_hline(60, 260, 120, C_BROWN);
    for (int x = 64; x < 260; x += 10) gfx_vline(x, 120, 133, C_BROWN);
}

static void room_master(void) {
    interior(C_VIOLET, C_PURPLE, C_BROWN, C_INK);
    floorboards(C_EARTH);
    door_side(false, C_CREAM, false);
    /* the wardrobe */
    const WthSpot *w = SP(S_MAS_WARDROBE);
    gfx_rect(w->x, w->y, w->w, w->h, C_BROWN);
    if (wth_flag(F_HIDE_WARDROBE_GONE)) {
        gfx_line(w->x + w->w / 2, w->y + 2, w->x + w->w / 2 - 4, w->y + w->h - 2, C_INK);
        gfx_line(w->x + w->w / 2 + 2, w->y + 2, w->x + w->w / 2 + 6, w->y + w->h - 2, C_INK);
    } else {
        gfx_vline(w->x + w->w / 2, w->y + 2, w->y + w->h - 3, C_EARTH);
        gfx_pset(w->x + w->w / 2 - 3, w->y + 30, C_AMBER);
        gfx_pset(w->x + w->w / 2 + 3, w->y + 30, C_AMBER);
    }
    /* the bed */
    const WthSpot *b = SP(S_MAS_BED);
    gfx_rect(b->x - 4, b->y - 30, 4, b->h + 30, C_BROWN);
    gfx_rect(b->x - 4, b->y - 32, b->w + 16, 4, C_BROWN);
    gfx_rect(b->x, b->y + 8, b->w, b->h - 12, C_CREAM);
    gfx_rect(b->x + b->w / 2, b->y + 8, b->w / 2, b->h - 12, C_WHITE);
    gfx_dither(b->x + b->w / 2, b->y + 8, b->w / 2, b->h - 12, C_LIGHT, 8);
    page_on(S_MAS_BED, b->x + 16, b->y + 14);
    /* the bedpost and the belt */
    const WthSpot *p = SP(S_MAS_POST);
    gfx_rect(p->x + 3, p->y, 5, p->h + 34, C_BROWN);
    if (!wth_flag(F_BELT_TAKEN)) {
        gfx_vline(p->x + 2, p->y + 6, p->y + 30, C_INK);
        gfx_vline(p->x + 9, p->y + 6, p->y + 26, C_INK);
        gfx_rect(p->x + 1, p->y + 4, 10, 3, C_AMBER);
    }
    /* the gramophone */
    const WthSpot *gr = SP(S_MAS_GRAMOPHONE);
    gfx_rect(gr->x + 4, gr->y + 20, gr->w - 8, 14, C_BROWN);
    trap(gr->x + 12, gr->x + 16, gr->x, gr->x + gr->w, gr->y + 18, gr->y, C_AMBER);
}

static void room_balcony(void) {
    night_sky(100);
    gfx_rect(8, 60, 304, 40, C_INK);
    /* the hedges below, a tangle of paths */
    const WthSpot *v = SP(S_BAL_VIEW);
    for (int k = 0; k < 7; k++) gfx_rect(v->x + 20 + k * 22, v->y + 40, 14, 20, C_FOREST);
    for (int k = 0; k < 6; k++) gfx_hline(v->x + 20, v->x + 170, v->y + 42 + k * 3, C_JADE);
    gfx_pset(260, 66, (wth.frame / 90) % 3 ? C_AMBER : C_INK);
    gfx_rect(8, 100, 304, 34, C_SLATE);
    gfx_rect(8, 96, 304, 4, C_GREY);
    for (int x = 12; x < 312; x += 10) gfx_rect(x, 100, 3, 24, C_GREY);
}

static void room_study(void) {
    interior(C_FOREST, C_JADE, C_BROWN, C_INK);
    floorboards(C_EARTH);
    gfx_rect(110, 90, 110, 6, C_TAN);
    gfx_rect(112, 96, 6, 30, C_BROWN);
    gfx_rect(212, 96, 6, 30, C_BROWN);
    /* the tape recorder */
    const WthSpot *c = SP(S_STU_RECORDER);
    gfx_rect(c->x + 2, c->y + 12, c->w - 4, 26, C_SLATE);
    gfx_circb(c->x + 14, c->y + 22, 8, C_INK);
    gfx_circb(c->x + c->w - 14, c->y + 22, 8, C_INK);
    if (wth_flag(F_TAPE_FITTED)) gfx_circ(c->x + 14, c->y + 22, 6, C_BROWN);
    if (wth_flag(F_REEL_FITTED)) gfx_circb(c->x + c->w - 14, c->y + 22, 6, C_LIGHT);
    gfx_rect(c->x + 22, c->y + 32, 10, 3, wth_flag(F_POWER) ? C_RED : C_INK);
    diary_page(S_STU_DIARY);
    /* the bureau */
    const WthSpot *k = SP(S_STU_BUREAU);
    gfx_rect(k->x, k->y + 10, k->w, k->h - 10, C_BROWN);
    trap(k->x + 4, k->x + k->w - 4, k->x, k->x + k->w, k->y, k->y + 10, C_EARTH);
    for (int y = k->y + 2; y < k->y + 10; y += 2) gfx_hline(k->x + 6, k->x + k->w - 7, y, C_BROWN);
    frame(k->x + 4, k->y + 16, k->w - 8, 10, C_EARTH);
    gfx_pset(k->x + k->w / 2, k->y + 5, C_AMBER);
    const WthSpot *s = SP(S_STU_SHELF);
    gfx_rect(s->x, s->y, s->w, s->h, C_BROWN);
    for (int row = 0; row < 4; row++) {
        gfx_hline(s->x, s->x + s->w - 1, s->y + 16 + row * 16, C_EARTH);
        for (int k2 = 0; k2 < 8; k2++)
            gfx_rect(s->x + 3 + k2 * 6, s->y + 4 + row * 16 + (k2 % 3), 5, 12 - (k2 % 3),
                     (k2 + row) % 3 == 0 ? C_RED : (k2 + row) % 3 == 1 ? C_NAVY : C_FOREST);
    }
}

static void room_bath(void) {
    interior(C_ICE, C_SKY, C_LIGHT, C_NIGHT);
    tiles(73, 26, 247, 99, C_SKY);
    for (int y = 101; y < 134; y += 4)
        for (int x = 8; x < 312; x += 8) gfx_rect(x + ((y / 4) & 1) * 4, y, 4, 4, C_GREY);
    /* the bath and its shower curtain */
    const WthSpot *t = SP(S_BAT_CURTAIN);
    gfx_rect(t->x, t->y + 52, t->w, t->h - 56, C_WHITE);
    gfx_hline(t->x, t->x + t->w - 1, t->y + 2, C_GREY);
    for (int x = t->x + 4; x < t->x + t->w - 30; x += 6) gfx_rect(x, t->y + 3, 5, 52, (x / 6) & 1 ? C_CREAM : C_PINK);
    gfx_rect(t->x + t->w - 26, t->y + 10, 3, 42, C_WHITE);
    gfx_dither(t->x + t->w - 30, t->y + 8, 12, 46, C_WHITE, 9);
    /* the airing cupboard */
    const WthSpot *a = SP(S_BAT_AIRING);
    if (wth_flag(F_HIDE_AIRING_GONE)) {
        gfx_rect(a->x, a->y, a->w, a->h, C_INK);
        for (int k = 0; k < 4; k++) gfx_rect(a->x + 2 + k * 7, a->y + a->h - 4, 6, 4, k & 1 ? C_WHITE : C_PINK);
    } else {
        gfx_rect(a->x, a->y, a->w, a->h, C_CREAM);
        frame(a->x + 3, a->y + 3, a->w - 6, a->h - 6, C_TAN);
        gfx_pset(a->x + a->w - 6, a->y + a->h / 2, C_AMBER);
    }
    const WthSpot *s = SP(S_BAT_CABINET);
    gfx_rect(s->x, s->y, s->w, s->h, C_WHITE);
    frame(s->x, s->y, s->w, s->h, C_GREY);
    if (!wth_flag(F_RAZOR_TAKEN)) gfx_rect(s->x + 6, s->y + 10, 12, 2, C_LIGHT);
    toilet(S_BAT_TOILET);
    diary_page(S_BAT_DIARY);
}

static void room_nursery(void) {
    interior(C_CREAM, C_TAN, C_BROWN, C_LIGHT);
    for (int x = 80; x < 248; x += 12) gfx_circb(x, 40, 3, C_PINK);
    floorboards(C_EARTH);
    /* crayon bees and a man in a hat */
    const WthSpot *w = SP(S_NUR_WALL);
    for (int k = 0; k < 4; k++) bee_shape(w->x + 8 + k * 14, w->y + 10 + (k & 1) * 8, C_YELLOW);
    gfx_rect(w->x + w->w - 14, w->y + 6, 6, 26, C_INK);
    gfx_rect(w->x + w->w - 18, w->y + 6, 14, 2, C_INK);
    /* the cot */
    const WthSpot *c = SP(S_NUR_COT);
    for (int x = c->x; x < c->x + c->w; x += 5) gfx_vline(x, c->y, c->y + c->h - 10, C_WHITE);
    gfx_hline(c->x, c->x + c->w - 1, c->y, C_WHITE);
    gfx_rect(c->x, c->y + c->h - 12, c->w, 4, C_WHITE);
    /* the rocking horse */
    const WthSpot *h = SP(S_NUR_HORSE);
    int rock = (wth.frame / 20) % 4 == 1 ? 1 : (wth.frame / 20) % 4 == 3 ? -1 : 0;
    gfx_line(h->x, h->y + h->h - 2 + rock, h->x + h->w, h->y + h->h - 2 - rock, C_BROWN);
    gfx_rect(h->x + 12, h->y + 18, 28, 12, C_WHITE);
    gfx_rect(h->x + 34, h->y + 6, 10, 14, C_WHITE);
    gfx_rect(h->x + 14, h->y + 30, 3, 12, C_WHITE);
    gfx_rect(h->x + 34, h->y + 30, 3, 12, C_WHITE);
    gfx_pset(h->x + 40, h->y + 9, C_INK);
    frame(h->x + 20, h->y + 24, 10, 5, wth_flag(F_BEE_HORSE) ? C_INK : C_GREY);
    diary_page(S_NUR_DIARY);
}

static void room_children(void) {
    interior(C_SKY, C_BLUE, C_BROWN, C_ICE);
    floorboards(C_EARTH);
    window_back(124, 30, 40, 30, C_NIGHT);
    diary_page(S_CHI_DIARY);
    if (!wth_flag(F_POLE_TAKEN)) {
        const WthSpot *p = SP(S_CHI_POLE);
        gfx_vline(p->x + 5, p->y, p->y + p->h - 1, C_TAN);
        gfx_line(p->x + 5, p->y, p->x + 8, p->y - 3, C_AMBER);
    }
    const WthSpot *t = SP(S_CHI_TOYS);
    gfx_rect(t->x, t->y + 6, t->w, t->h - 6, C_RED);
    gfx_rect(t->x - 2, t->y, t->w + 4, 7, C_MAROON);
    gfx_rect(t->x + 10, t->y - 6, 8, 6, C_ICE);
    const WthSpot *b = SP(S_CHI_BUNK);
    gfx_rect(b->x, b->y, 4, b->h, C_BROWN);
    gfx_rect(b->x + b->w - 4, b->y, 4, b->h, C_BROWN);
    gfx_rect(b->x + 4, b->y + 16, b->w - 8, 10, C_RED);
    gfx_rect(b->x + 4, b->y + 54, b->w - 8, 10, C_NAVY);
    gfx_rect(b->x + 6, b->y + 10, 18, 7, C_WHITE);
    gfx_rect(b->x + 6, b->y + 48, 18, 7, C_WHITE);
}

static void room_attic(void) {
    gfx_rect(WTH_PX, WTH_PY, WTH_PW, WTH_PH, C_BROWN);
    trap(160, 160, 8, 311, 6, 70, C_EARTH);
    for (int k = 0; k < 8; k++) gfx_line(160, 6, 8 + k * 43, 70, C_BROWN);
    gfx_rect(8, 100, 304, 34, C_EARTH);
    floorboards(C_BROWN);
    /* the wasps' nest */
    const WthSpot *n = SP(S_ATT_NEST);
    gfx_vline(n->x + n->w / 2, n->y, n->y + 6, C_BROWN);
    gfx_circ(n->x + n->w / 2, n->y + 24, 16, C_LIGHT);
    for (int k = 0; k < 5; k++) gfx_hline(n->x + 10, n->x + n->w - 11, n->y + 14 + k * 5, C_GREY);
    gfx_circ(n->x + n->w / 2, n->y + 36, 3, C_INK);
    if (!wth_flag(F_BEE_NEST))
        for (int k = 0; k < 6; k++)
            gfx_pset(n->x + 6 + (k * 11 + (int)wth.frame / 3) % 36, n->y + 10 + (k * 7 + (int)wth.frame / 5) % 30, C_YELLOW);
    /* the box of home movies */
    const WthSpot *t = SP(S_ATT_BOXES);
    gfx_rect(t->x, t->y + 6, t->w, t->h - 6, C_TAN);
    gfx_rect(t->x - 1, t->y + 4, t->w + 2, 4, C_BROWN);
    for (int k = 0; k < 4; k++) gfx_circb(t->x + 10 + k * 14, t->y + 2, 5, C_SLATE);
    /* the rolled carpets */
    const WthSpot *c = SP(S_ATT_CARPETS);
    for (int k = 0; k < 4; k++) gfx_rect(c->x + 4 + k * 16, c->y + (k & 1) * 6, 12, c->h - (k & 1) * 6, k & 1 ? C_RED : C_PURPLE);
    diary_page(S_ATT_DIARY);
}

static void room_cellar(void) {
    interior(C_DUSK, C_NIGHT, C_EARTH, C_INK);
    bricks(73, 27, 247, 99, C_NIGHT);
    for (int k = 0; k < 6; k++) gfx_dither(76 + k * 28, 26, 10, 20 + k * 3 % 7, C_WHITE, 3);
    /* the flooded inner doorway */
    const WthSpot *d = SP(S_CEL_DOOR);
    gfx_rect(d->x, d->y, d->w, d->h, C_INK);
    if (!wth_flag(F_CELLAR_DRAINED)) {
        gfx_rect(d->x, d->y + 4, d->w, d->h - 4, C_NAVY);
        for (int y = d->y + 8; y < d->y + d->h; y += 6) gfx_hline(d->x + 2, d->x + d->w - 3, y, C_NIGHT);
    } else gfx_dither(d->x, d->y, d->w, d->h, C_WHITE, 2);
    frame(d->x - 2, d->y - 2, d->w + 4, d->h + 2, C_SLATE);
    /* the hand pump */
    const WthSpot *m = SP(S_CEL_PUMP);
    gfx_rect(m->x + 10, m->y + 6, 10, m->h - 6, C_SLATE);
    gfx_rect(m->x + 6, m->y + m->h - 4, 18, 4, C_SLATE);
    if (wth_flag(F_PUMP_HANDLE)) gfx_line(m->x + 15, m->y + 6, m->x + 2, m->y - 4, C_GREY);
    if (wth_flag(F_PUMP_HOSE)) gfx_line(m->x + 20, m->y + 14, m->x + 40, m->y + 60, C_JADE);
    if (!wth_flag(F_PUMP_GREASE)) gfx_dither(m->x + 10, m->y + 6, 10, m->h - 6, C_ORANGE, 5);
    const WthSpot *s = SP(S_CEL_SHELF);
    for (int r = 0; r < 3; r++) {
        gfx_rect(s->x, s->y + 16 + r * 20, s->w, 3, C_BROWN);
        for (int k = 0; k < 9; k++) gfx_rect(s->x + 2 + k * 7, s->y + 6 + r * 20, 5, 10, r == 2 ? C_WHITE : C_AMBER);
    }
    diary_page(S_CEL_DIARY);
    for (int k = 0; k < 3; k++) gfx_rect(110 + k * 6, 124 - k * 4, 100 - k * 12, 4, C_SLATE);
}

static void room_spore(void) {
    gfx_rect(WTH_PX, WTH_PY, WTH_PW, WTH_PH, C_INK);
    for (int k = 0; k < 60; k++) {
        int x = 8 + (k * 47) % 300, y = 8 + (k * 31) % 124;
        gfx_line(x, y, 160, 64, k & 1 ? C_LIGHT : C_GREY);
    }
    const WthSpot *f = SP(S_SPO_FACE);
    int breathe = (wth.frame / 30) % 2;
    gfx_circ(f->x + f->w / 2, f->y + f->h / 2, 34 + breathe, C_WHITE);
    gfx_dither(f->x + 10, f->y + 4, f->w - 20, f->h - 8, C_LIGHT, 8);
    gfx_circ(f->x + f->w / 2 - 14, f->y + 30, 6, C_INK);
    gfx_circ(f->x + f->w / 2 + 14, f->y + 30, 6, C_INK);
    gfx_circ(f->x + f->w / 2, f->y + 54, 8 + breathe, C_INK);
    gfx_pset(f->x + f->w / 2 - 14, f->y + 30, C_RED);
    gfx_pset(f->x + f->w / 2 + 14, f->y + 30, C_RED);
    const WthSpot *r = SP(S_SPO_ROOTS);
    for (int k = 0; k < 8; k++) gfx_line(r->x + k * 8, r->y + r->h, r->x + 30 + k * 4, r->y, C_LIGHT);
}

void wth_draw_room(int room) {
    gfx_clip(WTH_PX, WTH_PY, WTH_PW, WTH_PH);
    switch (room) {
    case R_CELL: room_cell(); break;
    case R_PASSAGE: room_passage(); break;
    case R_SICKROOM: room_sickroom(); break;
    case R_HALL: room_hall(); break;
    case R_PARLOUR: room_parlour(); break;
    case R_DINING: room_dining(); break;
    case R_KITCHEN: room_kitchen(); break;
    case R_SCULLERY: room_scullery(); break;
    case R_SMOKING: room_smoking(); break;
    case R_WASHROOM: room_washroom(); break;
    case R_GUEST: room_guest(); break;
    case R_GLASSHOUSE: room_glasshouse(); break;
    case R_GARAGE: room_garage(); break;
    case R_POND: room_pond(); break;
    case R_SHED: room_shed(); break;
    case R_GARDEN: room_garden(); break;
    case R_YARD: room_yard(); break;
    case R_DRIVE: room_drive(); break;
    case R_COURTYARD: room_courtyard(); break;
    case R_MAZE: room_maze(); break;
    case R_HEART: room_heart(); break;
    case R_LANDING: room_landing(); break;
    case R_MASTER: room_master(); break;
    case R_BALCONY: room_balcony(); break;
    case R_STUDY: room_study(); break;
    case R_BATH: room_bath(); break;
    case R_NURSERY: room_nursery(); break;
    case R_CHILDREN: room_children(); break;
    case R_ATTIC: room_attic(); break;
    case R_CELLAR: room_cellar(); break;
    case R_SPORE: room_spore(); break;
    default: gfx_rect(WTH_PX, WTH_PY, WTH_PW, WTH_PH, C_INK); break;
    }
    for (int i = 0; i < WTH_FLOOR; i++) {
        int it = wth.s.floor[room][i];
        if (it) wth_draw_item(it, 70 + i * 60 + 2, 112);
    }
    if (wth_flag(F_KEEPER_DEAD) && wth.s.body_room == room) wth_draw_keeper(130, 96, 1, 0, 3);
    gfx_noclip();
}

/* ------------------------------------------------------------------ */
/* the light ring                                                       */

static void light_ring(int cx, int cy) {
    const int R = 36, EDGE = 9;
    for (int y = WTH_PY; y < WTH_PY + WTH_PH; y++) {
        int dy = y - cy;
        for (int x = WTH_PX; x < WTH_PX + WTH_PW; x++) {
            int dx = x - cx, d2 = dx * dx + dy * dy;
            if (d2 <= (R - EDGE) * (R - EDGE)) continue;
            if (d2 >= R * R) { gfx_pset(x, y, C_INK); continue; }
            /* the dithered edge: more black further out */
            int t = (d2 - (R - EDGE) * (R - EDGE)) * 16 / (R * R - (R - EDGE) * (R - EDGE));
            static const uint8_t BAYER[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
            if (BAYER[y & 3][x & 3] < t) gfx_pset(x, y, C_INK);
        }
    }
}

/* ------------------------------------------------------------------ */
/* the Keeper                                                           */

/* x, y: top left; scale: 1 = 24 x 56 px. pose 0 stands, 1 swings, 2 down on a knee, 3 dead */
void wth_draw_keeper(int x, int y, int scale, int t, int pose) {
    int s = scale;
    if (pose == 3) {
        /* slumped against nothing, the veil forward */
        gfx_rect(x, y + 14, 44, 12, C_EARTH);
        gfx_rect(x + 8, y + 16, 20, 8, C_CREAM);
        gfx_circ(x + 44, y + 16, 7, C_TAN);
        gfx_dither(x + 36, y + 8, 18, 16, C_GREY, 9);
        gfx_line(x - 10, y + 24, x + 6, y + 20, C_LIGHT);
        return;
    }
    int kneel = pose == 2 ? 14 * s : 0;
    int sway = (t / 12) % 2;
    /* legs */
    if (!kneel) {
        gfx_rect(x + 6 * s, y + 40 * s, 4 * s, 16 * s, C_INK);
        gfx_rect(x + 14 * s, y + 40 * s, 4 * s, 16 * s, C_INK);
    } else gfx_rect(x + 4 * s, y + 46 * s, 16 * s, 6 * s, C_INK);
    int by = y + kneel;
    /* coat and apron */
    gfx_rect(x + 3 * s, by + 14 * s, 18 * s, 28 * s, C_EARTH);
    gfx_rect(x + 7 * s, by + 18 * s, 10 * s, 22 * s, C_CREAM);
    gfx_dither(x + 7 * s, by + 28 * s, 10 * s, 12 * s, C_MAROON, 5);
    /* the head under the veil */
    gfx_circ(x + 12 * s, by + 9 * s, 5 * s, C_TAN);
    gfx_dither(x + 5 * s, by + 4 * s, 14 * s, 12 * s, C_GREY, 9);
    gfx_rect(x + 9 * s, by + 8 * s, 2 * s, s, C_INK);
    gfx_rect(x + 14 * s, by + 8 * s, 2 * s, s, C_INK);
    /* the hat brim */
    gfx_rect(x, by + 3 * s, 24 * s, 2 * s, C_TAN);
    gfx_rect(x + 7 * s, by, 10 * s, 3 * s, C_TAN);
    /* arms and the billhook */
    if (pose == 1) {
        gfx_rect(x + 18 * s, by + 2 * s, 3 * s, 14 * s, C_EARTH);
        gfx_rect(x + 19 * s, by - 10 * s, 2 * s, 14 * s, C_BROWN);
        gfx_rect(x + 14 * s, by - 14 * s, 8 * s, 4 * s, C_LIGHT);
        gfx_rect(x + 13 * s, by - 12 * s, 2 * s, 4 * s, C_LIGHT);
    } else {
        gfx_rect(x, by + 15 * s, 3 * s, 18 * s, C_EARTH);
        gfx_rect(x + 21 * s, by + 15 * s, 3 * s, 14 * s + sway * s, C_EARTH);
        gfx_rect(x + 22 * s, by + 22 * s, 2 * s, 18 * s, C_BROWN);
        gfx_rect(x + 22 * s, by + 38 * s, 6 * s, 3 * s, C_LIGHT);
        gfx_rect(x + 26 * s, by + 34 * s, 2 * s, 5 * s, C_LIGHT);
    }
}

static void draw_keeper_in_room(void) {
    if (!wth_keeper_visible()) return;
    int x, y, w, h;
    wth_keeper_rect(&x, &y, &w, &h);
    /* drawn once at full size into a small buffer, then scaled to his box,
     * so he grows smoothly as he walks up to you */
    enum { KW = 34, KH = 74, OX = 2, OY = 16 };
    static uint8_t kbuf[KW * KH];
    static Surface ks = {KW, KH, kbuf};
    int pose = wth.k.state == E_STUN ? 2 : wth.k.state == E_CAUGHT ? 1 : 0;
    memset(kbuf, TRANSPARENT, sizeof kbuf);
    gfx_set_target(&ks);
    wth_draw_keeper(OX, OY, 1, wth.k.arrive, pose);
    gfx_set_target(NULL);
    int full = wth.k.state == E_STUN ? h * 3 / 2 : h; /* knelt, his box is the lower part */
    int scale256 = full * 256 / 56;                   /* 56 px tall at full size */
    int dw = KW * scale256 / 256, dh = KH * scale256 / 256;
    int dx0 = x + w / 2 - (OX + 12) * scale256 / 256, dy0 = y + h - (OY + 56) * scale256 / 256;
    gfx_clip(WTH_PX, WTH_PY, WTH_PW, WTH_PH);
    for (int j = 0; j < dh; j++) {
        int sy = j * 256 / scale256;
        if (sy >= KH) break;
        for (int i = 0; i < dw; i++) {
            int sx = i * 256 / scale256;
            if (sx >= KW) break;
            uint8_t c = kbuf[sy * KW + sx];
            if (c != TRANSPARENT) gfx_pset(dx0 + i, dy0 + j, c);
        }
    }
    gfx_noclip();
}

/* ------------------------------------------------------------------ */
/* the cursor                                                           */

static void cursor_arrow(int x, int y, int col) {
    for (int i = 0; i < 9; i++) gfx_hline(x, x + i / 2, y + i, col);
    gfx_line(x, y, x, y + 9, C_INK);
    gfx_line(x, y, x + 6, y + 6, C_INK);
}

static void cursor_exit(int x, int y, int dir) {
    int c = C_WHITE;
    switch (dir) {
    case DIR_UP: trap(x, x, x - 5, x + 5, y - 5, y, c); gfx_rect(x - 2, y, 5, 5, c); break;
    case DIR_DOWN: trap(x - 5, x + 5, x, x, y, y + 5, c); gfx_rect(x - 2, y - 5, 5, 5, c); break;
    case DIR_LEFT:
        for (int i = 0; i <= 5; i++) gfx_vline(x - 5 + i, y - i, y + i, c);
        gfx_rect(x, y - 2, 5, 5, c);
        break;
    default:
        for (int i = 0; i <= 5; i++) gfx_vline(x + 5 - i, y - i, y + i, c);
        gfx_rect(x - 5, y - 2, 5, 5, c);
        break;
    }
    gfx_pset(x, y, C_INK);
}

static void draw_cursor(void) {
    int x = (int)(wth.cx >> 8) + wth.shake_x, y = (int)(wth.cy >> 8) + wth.shake_y;
    if (wth.held) {
        gfx_rect(x - 1, y - 1, 14, 14, C_INK);
        wth_draw_item(wth.held, x, y);
        frame(x - 1, y - 1, 14, 14, C_WHITE);
        return;
    }
    int s = wth_spot_at(x, y);
    bool on_keeper = false;
    if (wth_keeper_visible()) {
        int kx, ky, kw, kh;
        wth_keeper_rect(&kx, &ky, &kw, &kh);
        on_keeper = x >= kx && y >= ky && x < kx + kw && y < ky + kh;
    }
    if (!on_keeper && s >= 0 && WTH_SPOT[s].kind == HS_EXIT) cursor_exit(x, y, WTH_SPOT[s].dir);
    else cursor_arrow(x, y, C_WHITE);
}

#define TRANS_HALF_DRAW 12

/* ------------------------------------------------------------------ */
/* the text panel                                                       */

static void txt(const char *what, const char *s, int x, int y, int col) {
    text_draw(s, x, y, col);
    ui_audit_text(what, s, x, y);
}

static void text_panel(void) {
    gfx_rect(0, WTH_TY - 2, SCREEN_W, SCREEN_H - WTH_TY + 2, C_INK);
    frame(WTH_PX, WTH_TY, WTH_PW, SCREEN_H - WTH_TY - 2, C_SLATE);
    ui_audit_area("panel", WTH_PX + 2, WTH_TY + 2, WTH_PW - 4, SCREEN_H - WTH_TY - 6);
    if (wth_talking()) {
        for (int l = 0; l < WTH_TLINES; l++)
            if (wth.msg[wth.msg_page][l][0]) txt("message", wth.msg[wth.msg_page][l], WTH_PX + 6, WTH_TY + 3 + l * LINE_H, C_CREAM);
        if ((wth.frame / 16) & 1) text_draw(wth.msg_page + 1 < wth.msg_n ? GLYPH_DOWN : GLYPH_A, WTH_PX + WTH_PW - 10, SCREEN_H - 12, C_GREY);
        return;
    }
    txt("room", WTH_ROOM_NAME[wth.s.room], WTH_PX + 6, WTH_TY + 3, C_GREY);
    if (wth.held) {
        char buf[64];
        snprintf(buf, sizeof buf, "IN HAND: %s", wth_item_name(wth.held));
        txt("held", buf, WTH_PX + 6, WTH_TY + 3 + 3 * LINE_H, C_AMBER);
    }
}

/* the breathing game, seen from the hiding place */
static void draw_mini(void) {
    const WthMini *m = &wth.k.mini;
    gfx_rect(WTH_PX, WTH_PY, WTH_PW, WTH_PH, C_INK);
    /* a slit of the room, and his shape in it */
    gfx_rect(140, 20, 40, 90, C_NIGHT);
    wth_draw_keeper(148, 40, 1, (int)wth.frame, 0);
    gfx_rect(WTH_PX, 20, 132, 90, C_INK);
    gfx_rect(188, 20, 124, 90, C_INK);
    int bx = 80, by = 118;
    gfx_rect(bx - 2, by - 2, WTH_BAR_W + 4, 12, C_SLATE);
    gfx_rect(bx, by, WTH_BAR_W, 8, C_DUSK);
    gfx_rect(bx + m->zone, by, m->zw, 8, C_JADE);
    int px = bx + (int)(m->pos >> 8);
    gfx_rect(px - 1, by - 4, 3, 16, m->pause ? C_GREY : C_WHITE);
    if (m->flash > 0) frame(bx - 3, by - 3, WTH_BAR_W + 6, 14, m->flash_ok ? C_LIME : C_RED);
    for (int i = 0; i < WTH_MISSES; i++) gfx_circ(bx + WTH_BAR_W + 12 + i * 8, by + 4, 3, i < m->miss ? C_RED : C_SLATE);
    for (int i = 0; i < WTH_BREATHS; i++) gfx_rect(bx + i * 8, by - 12, 6, 4, i < m->n ? C_LIGHT : C_DUSK);
}

void wth_draw_play(void) {
    gfx_cls(C_INK);
    if (wth.k.state == E_HIDING) {
        draw_mini();
    } else {
        wth_draw_room(wth.s.room);
        draw_keeper_in_room();
        if (wth.k.state == E_CAUGHT) {
            int t = wth.k.t;
            if (t > 18) gfx_dither(WTH_PX, WTH_PY, WTH_PW, WTH_PH, C_RED, imin(16, (t - 18) / 2));
            if (t > 40) gfx_rect(WTH_PX, WTH_PY, WTH_PW, WTH_PH, C_INK);
        } else {
            if (wth_room_dark(wth.s.room)) light_ring((int)(wth.cx >> 8), (int)(wth.cy >> 8));
            if (wth.scene == SC_PLAY) draw_cursor();
        }
        if (wth.trans_t > 0) {
            int d = wth.trans_t < TRANS_HALF_DRAW ? wth.trans_t : 24 - wth.trans_t;
            gfx_dither(WTH_PX, WTH_PY, WTH_PW, WTH_PH, C_INK, imin(16, d * 16 / TRANS_HALF_DRAW));
        }
    }
    frame(WTH_PX - 1, WTH_PY - 1, WTH_PW + 2, WTH_PH + 2, wth_scared() ? C_RED : C_SLATE);
    text_panel();
}

/* ------------------------------------------------------------------ */
/* the bag                                                              */

void wth_draw_bag(void) {
    const int X0 = 66, Y0 = 18, W = 188, H = 116;
    gfx_rect(X0, Y0, W, H, C_NIGHT);
    frame(X0, Y0, W, H, C_TAN);
    frame(X0 + 2, Y0 + 2, W - 4, H - 4, C_BROWN);
    ui_audit_area("bag", X0 + 3, Y0 + 3, W - 6, H - 6);
    txt("bag title", "THE BAG", X0 + 8, Y0 + 6, C_TAN);
    char pg[24];
    snprintf(pg, sizeof pg, "PAGES %d/%d", wth_page_count(), WTH_PAGES);
    txt("bag pages", pg, X0 + W - 8 - text_width(pg), Y0 + 6, C_GREY);
    int gx = X0 + 14, gy = Y0 + 20;
    for (int i = 0; i < WTH_BAG; i++) {
        int x = gx + (i % 10) * 16, y = gy + (i / 10) * 16;
        gfx_rect(x, y, 15, 15, C_DUSK);
        if (wth.s.bag[i]) wth_draw_item(wth.s.bag[i], x + 1, y + 1);
        if (wth.held && wth.s.bag[i] == wth.held) frame(x, y, 15, 15, C_AMBER);
    }
    int sx = gx + (wth.bag_sel % 10) * 16, sy = gy + (wth.bag_sel / 10) * 16;
    frame(sx - 1, sy - 1, 17, 17, (wth.frame / 10) & 1 ? C_WHITE : C_CREAM);
    int it = wth.s.bag[wth.bag_sel];
    int ty = gy + 50;
    if (it) {
        txt("item name", wth_item_name(it), X0 + 8, ty, C_CREAM);
        char lines[3][UI_WRAP_LEN];
        int n = ui_wrap(WTH_ITEM[it].desc, W - 16, false, lines, 3);
        for (int l = 0; l < imin(n, 3); l++) txt("item desc", lines[l], X0 + 8, ty + 10 + l * LINE_H, C_GREY);
    }
    if (wth.held) {
        char buf[48];
        snprintf(buf, sizeof buf, "WITH %s ON...", wth_item_name(wth.held));
        txt("bag held", buf, X0 + 8, Y0 + H - 11, C_AMBER);
    }
}

/* ------------------------------------------------------------------ */
/* the diary                                                            */

void wth_draw_diary(void) {
    gfx_cls(C_INK);
    gfx_rect(24, 8, 272, 164, C_CREAM);
    frame(24, 8, 272, 164, C_TAN);
    for (int y = 30; y < 160; y += 9) gfx_hline(30, 289, y + 8, C_LIGHT);
    ui_audit_area("diary", 28, 10, 264, 160);
    int p = wth.diary_sel;
    if (p >= 0 && p < WTH_PAGES && ((wth.s.pages >> p) & 1)) {
        txt("diary date", WTH_PAGE[p].date, 34, 14, C_MAROON);
        char lines[14][UI_WRAP_LEN];
        int n = ui_wrap(WTH_PAGE[p].text, 250, false, lines, 14);
        if (n > 13) ui_audit_fail("diary", "page too long");
        for (int l = 0; l < imin(n, 13); l++) txt("diary text", lines[l], 34, 30 + l * LINE_H, C_NIGHT);
    }
    char foot[48];
    snprintf(foot, sizeof foot, GLYPH_LEFT " " GLYPH_RIGHT "  PAGE %d OF %d FOUND", wth_page_count(), WTH_PAGES);
    txt("diary foot", foot, 34, 158, C_BROWN);
}

/* ------------------------------------------------------------------ */
/* the item pictures: 12 x 12 each                                       */

static void key_shape(int x, int y, int c) {
    gfx_circb(x + 3, y + 4, 2, c);
    gfx_hline(x + 5, x + 11, y + 4, c);
    gfx_vline(x + 9, y + 4, y + 7, c);
    gfx_vline(x + 11, y + 4, y + 6, c);
}

static void bee_icon(int x, int y, int c) {
    gfx_rect(x + 3, y + 5, 7, 4, c);
    gfx_vline(x + 5, y + 5, y + 8, C_INK);
    gfx_vline(x + 8, y + 5, y + 8, C_INK);
    gfx_rect(x + 3, y + 2, 3, 3, C_LIGHT);
    gfx_rect(x + 7, y + 2, 3, 3, C_LIGHT);
    gfx_pset(x + 2, y + 6, c);
}

void wth_draw_item(int it, int x, int y) {
    switch (it) {
    case I_DIARY: gfx_rect(x + 2, y + 1, 8, 10, C_CREAM); gfx_rect(x + 3, y + 2, 8, 10, C_CREAM); for (int k = 0; k < 4; k++) gfx_hline(x + 4, x + 9, y + 4 + k * 2, C_TAN); break;
    case I_KNIFE: gfx_rect(x + 1, y + 8, 4, 2, C_BROWN); gfx_line(x + 5, y + 8, x + 11, y + 2, C_LIGHT); gfx_line(x + 5, y + 9, x + 11, y + 3, C_WHITE); break;
    case I_POKER: gfx_line(x + 1, y + 11, x + 10, y + 2, C_SLATE); gfx_line(x + 10, y + 2, x + 11, y + 4, C_SLATE); gfx_rect(x, y + 10, 3, 2, C_AMBER); break;
    case I_STONE: gfx_circ(x + 6, y + 7, 4, C_GREY); gfx_pset(x + 4, y + 5, C_LIGHT); break;
    case I_NAIL: gfx_vline(x + 6, y + 2, y + 11, C_LIGHT); gfx_hline(x + 4, x + 8, y + 2, C_LIGHT); break;
    case I_PHOTO: gfx_rect(x + 1, y + 2, 10, 8, C_WHITE); gfx_rect(x + 2, y + 3, 8, 6, C_TAN); gfx_pset(x + 4, y + 5, C_INK); gfx_pset(x + 7, y + 5, C_INK); break;
    case I_PUMPHANDLE: gfx_line(x + 1, y + 10, x + 9, y + 2, C_SLATE); gfx_rect(x + 9, y + 1, 3, 3, C_SLATE); break;
    case I_SALT: gfx_circ(x + 6, y + 8, 3, C_WHITE); gfx_pset(x + 3, y + 5, C_WHITE); gfx_pset(x + 9, y + 6, C_WHITE); gfx_pset(x + 7, y + 4, C_LIGHT); break;
    case I_TAPKEY: gfx_rect(x + 2, y + 2, 4, 4, C_SLATE); gfx_rect(x + 3, y + 3, 2, 2, C_INK); gfx_line(x + 5, y + 5, x + 10, y + 10, C_SLATE); break;
    case I_BUCKET: case I_WATER: trap(x + 1, x + 10, x + 3, x + 8, y + 3, y + 11, C_LIGHT); gfx_circb(x + 6, y + 3, 4, C_GREY); if (it == I_WATER) gfx_hline(x + 2, x + 9, y + 4, C_SKY); break;
    case I_FUSE: case I_GOODFUSE: gfx_rect(x + 2, y + 4, 8, 5, C_WHITE); gfx_rect(x + 1, y + 5, 1, 3, C_LIGHT); gfx_rect(x + 10, y + 5, 1, 3, C_LIGHT); gfx_hline(x + 3, x + 8, y + 3, it == I_FUSE ? C_INK : C_AMBER); break;
    case I_FUSEWIRE: gfx_rect(x + 2, y + 2, 8, 9, C_CREAM); for (int k = 0; k < 3; k++) gfx_hline(x + 3, x + 8, y + 4 + k * 2, C_AMBER); break;
    case I_HONEY: gfx_rect(x + 3, y + 3, 7, 8, C_CREAM); gfx_rect(x + 3, y + 2, 7, 2, C_AMBER); break;
    case I_MATCHES: gfx_rect(x + 1, y + 4, 10, 6, C_YELLOW); gfx_hline(x + 1, x + 10, y + 6, C_RED); break;
    case I_CROSSBOW: gfx_line(x + 1, y + 6, x + 11, y + 6, C_BROWN); gfx_line(x + 8, y + 1, x + 10, y + 11, C_EARTH); if (wth_flag(F_BOW_LOADED)) gfx_line(x + 2, y + 5, x + 11, y + 5, C_LIGHT); break;
    case I_TAPE: gfx_circ(x + 6, y + 6, 4, C_BROWN); gfx_circ(x + 6, y + 6, 1, C_LIGHT); break;
    case I_SCREWDRIVER: gfx_rect(x + 1, y + 8, 4, 3, C_RED); gfx_line(x + 5, y + 8, x + 11, y + 2, C_LIGHT); break;
    case I_LAMP: gfx_rect(x + 3, y + 3, 6, 6, C_ICE); frame(x + 3, y + 3, 6, 6, C_RED); gfx_rect(x + 2, y + 9, 8, 3, C_RED); gfx_circb(x + 6, y + 1, 1, C_SLATE); if (wth_flag(F_LAMP_LIT)) gfx_rect(x + 5, y + 5, 2, 3, C_YELLOW); break;
    case I_SULPHATE: gfx_rect(x + 3, y + 2, 6, 9, C_CREAM); gfx_dither(x + 3, y + 5, 6, 5, C_BLUE, 12); break;
    case I_TROWEL: gfx_rect(x + 6, y + 1, 4, 6, C_LIGHT); gfx_line(x + 7, y + 7, x + 3, y + 11, C_BROWN); break;
    case I_PARAFFIN: gfx_rect(x + 2, y + 3, 8, 8, C_RED); gfx_rect(x + 7, y + 1, 2, 2, C_GREY); break;
    case I_GREASE: gfx_rect(x + 1, y + 5, 10, 4, C_INK); gfx_hline(x + 1, x + 10, y + 5, C_GREY); break;
    case I_LIME: gfx_rect(x + 2, y + 4, 8, 6, C_WHITE); gfx_dither(x + 2, y + 4, 8, 6, C_LIGHT, 6); break;
    case I_CUTTERS: gfx_line(x + 2, y + 10, x + 7, y + 4, C_RED); gfx_line(x + 6, y + 11, x + 8, y + 4, C_RED); gfx_rect(x + 7, y + 1, 3, 3, C_LIGHT); break;
    case I_PLIERS: gfx_line(x + 3, y + 11, x + 6, y + 4, C_SLATE); gfx_line(x + 9, y + 11, x + 6, y + 4, C_SLATE); gfx_vline(x + 6, y + 1, y + 4, C_LIGHT); break;
    case I_SPRAYER: case I_MIXTURE: gfx_rect(x + 3, y + 3, 6, 8, it == I_MIXTURE ? C_BLUE : C_JADE); gfx_line(x + 8, y + 3, x + 11, y + 1, C_SLATE); if (wth.s.mix && it == I_SPRAYER) gfx_pset(x + 5, y + 6, C_BLUE); break;
    case I_GLOVES: gfx_rect(x + 2, y + 4, 4, 7, C_BROWN); gfx_rect(x + 7, y + 3, 4, 7, C_BROWN); gfx_pset(x + 1, y + 6, C_BROWN); break;
    case I_SMOKER: gfx_rect(x + 2, y + 3, 5, 8, C_LIGHT); gfx_rect(x + 7, y + 5, 4, 5, C_BROWN); if (wth_flag(F_SMOKER_LIT)) gfx_pset(x + 4, y + 1, C_GREY); break;
    case I_CARKEY: key_shape(x, y, C_LIGHT); gfx_rect(x + 1, y + 7, 4, 4, C_AMBER); break;
    case I_HOSE: gfx_circb(x + 6, y + 6, 4, C_JADE); gfx_circb(x + 6, y + 6, 2, C_JADE); break;
    case I_TIN: gfx_rect(x + 1, y + 4, 10, 5, C_ORANGE); gfx_hline(x + 1, x + 10, y + 5, C_BROWN); break;
    case I_ROTOR: gfx_circ(x + 6, y + 7, 3, C_INK); gfx_rect(x + 5, y + 2, 2, 6, C_SLATE); gfx_rect(x + 5, y + 2, 5, 2, C_LIGHT); break;
    case I_BUREAUKEY: key_shape(x, y, C_YELLOW); break;
    case I_CONES: gfx_circ(x + 4, y + 7, 3, C_BROWN); gfx_circ(x + 8, y + 5, 3, C_EARTH); gfx_pset(x + 4, y + 6, C_TAN); break;
    case I_TACKLE: gfx_rect(x + 1, y + 4, 10, 6, C_JADE); gfx_rect(x + 5, y + 3, 2, 2, C_SLATE); break;
    case I_LINE: gfx_circ(x + 6, y + 6, 4, C_WHITE); gfx_circ(x + 6, y + 6, 1, C_INK); break;
    case I_MAGNET: case I_MAGNETLINE: gfx_circ(x + 6, y + 7, 4, C_SLATE); gfx_circ(x + 6, y + 7, 1, C_INK); if (it == I_MAGNETLINE) gfx_vline(x + 6, y, y + 3, C_WHITE); break;
    case I_BEE_BRASS: bee_icon(x, y, C_YELLOW); break;
    case I_BEE_COPPER: bee_icon(x, y, C_ORANGE); break;
    case I_BEE_SILVER: bee_icon(x, y, C_WHITE); break;
    case I_BEE_IRON: bee_icon(x, y, C_SLATE); break;
    case I_BELT: gfx_circb(x + 6, y + 6, 4, C_BROWN); gfx_rect(x + 8, y + 5, 3, 3, C_AMBER); break;
    case I_RAZOR: gfx_rect(x + 1, y + 6, 6, 2, C_CREAM); gfx_line(x + 6, y + 6, x + 11, y + 3, C_LIGHT); break;
    case I_WASHER: gfx_circ(x + 6, y + 6, 4, C_BROWN); gfx_circ(x + 6, y + 6, 1, C_NAVY); break;
    case I_POLE: gfx_line(x + 1, y + 11, x + 10, y + 2, C_TAN); gfx_pset(x + 11, y + 1, C_AMBER); break;
    case I_BINOCULARS: gfx_rect(x + 1, y + 4, 4, 7, C_INK); gfx_rect(x + 7, y + 4, 4, 7, C_INK); gfx_hline(x + 5, x + 6, y + 6, C_INK); gfx_pset(x + 2, y + 5, C_SKY); break;
    case I_SKETCH: gfx_rect(x + 1, y + 2, 10, 8, C_CREAM); gfx_line(x + 3, y + 8, x + 5, y + 4, C_INK); gfx_line(x + 5, y + 4, x + 9, y + 6, C_INK); break;
    case I_BOLT: gfx_line(x + 1, y + 10, x + 10, y + 1, C_TAN); gfx_rect(x + 9, y + 1, 2, 2, C_GREY); gfx_rect(x + 1, y + 9, 2, 2, C_RED); break;
    case I_BLACKKEY: gfx_rect(x, y, 12, 12, C_DUSK); key_shape(x, y, C_INK); break;
    case I_REEL: gfx_circb(x + 6, y + 6, 4, C_LIGHT); gfx_circ(x + 6, y + 6, 1, C_LIGHT); break;
    default: gfx_rect(x + 3, y + 3, 6, 6, C_PINK); break;
    }
}

/* ------------------------------------------------------------------ */
/* the other screens                                                    */

static void centre(const char *what, const char *s, int y, int col) {
    int w = text_width(s);
    text_draw(s, 160 - w / 2, y, col);
    ui_audit_text(what, s, 160 - w / 2, y);
}

static void wrapped_centre(const char *what, const char *s, int y, int w, int col) {
    char lines[12][UI_WRAP_LEN];
    int n = ui_wrap(s, w, false, lines, 12);
    if (n > 12) ui_audit_fail(what, "too many lines");
    for (int i = 0; i < imin(n, 12); i++) centre(what, lines[i], y + i * LINE_H, col);
}

static void house_silhouette(int t, int lit_room) {
    gfx_rect(70, 70, 180, 70, C_INK);
    trap(160, 160, 60, 260, 40, 72, C_INK);
    gfx_rect(200, 34, 12, 30, C_INK);
    for (int k = 0; k < 5; k++) gfx_rect(86 + k * 32, 84, 12, 16, k == lit_room && (t / 50) % 7 ? C_AMBER : C_NIGHT);
    if (lit_room < 0) return;
    for (int k = 0; k < 5; k++) gfx_rect(86 + k * 32, 110, 12, 16, C_NIGHT);
    gfx_rect(0, 140, 320, 40, C_INK);
}

static const char *const INTRO[3] = {
    "OCTOBER 1988. YOU LEFT THE HARVEST SUPPER IN A TEMPER AND DROVE TOO FAST DOWN THE FEN ROAD, WITH THE WIPERS "
    "GOING AND THE RADIO UP.",
    "SOMETHING PALE STEPPED OUT INTO THE HEADLIGHTS. A THUMP. THE WHEEL TORE OUT OF YOUR HANDS, AND THE CAR WENT "
    "NOSE FIRST INTO THE DYKE. THEN NOTHING.",
    "YOU WAKE UP COLD, ON STRAW, IN THE DARK, IN A ROOM THAT LOCKS FROM THE OUTSIDE. SOMEWHERE ABOVE YOU, A CLOCK "
    "THAT ISN'T TICKING. AND FOOTSTEPS.",
};
static const char *const TXT_END_CAR[3] = {
    "THE ENGINE CATCHES. YOU BACK THE ROVER OUT ACROSS THE GRAVEL, SWING IT ROUND AND PUT YOUR FOOT DOWN. THE GATES "
    "BURST OPEN AND THE CHAIN WHIPS AWAY INTO THE DARK.",
    "THE FEN ROAD UNROLLS IN THE HEADLIGHTS, STRAIGHT AND EMPTY. BEHIND YOU WITHERLEA GETS SMALLER, AND SMALLER, "
    "AND IS GONE.",
    "YOU DON'T STOP UNTIL THE SKY GOES GREY. YOU GOT OUT. WHATEVER IS STILL IN THAT HOUSE CAN STAY THERE.",
};
static const char *const TXT_END_CELLAR[3] = {
    "YOU SPRAY THE MIXTURE INTO THE FACE. IT HISSES LIKE FAT IN A PAN. THE WHITE THREADS CURL, AND GO BLUE, AND "
    "GREY, AND THE FACE OPENS ITS MOUTH WIDE WITHOUT A SOUND.",
    "ALL THROUGH THE HOUSE THE THREADS DIE AND DRY TO TINDER. A SPARK FROM THE CELLAR'S OLD BOILER, AND THE FIRE "
    "RUNS UP THEM, ROOM BY ROOM, FASTER THAN YOU CAN CLIMB. YOU RUN.",
    "FROM THE END OF THE LANE YOU WATCH WITHERLEA BURN, EVERY WINDOW LIT, UNTIL THE ROOF GOES IN. YOUR OWN CAR IS "
    "STILL NOSE DOWN IN THE DYKE. IT STARTS FIRST TIME.",
};

static void draw_road(int t) {
    gfx_rect(0, 0, 320, 180, C_INK);
    trap(150, 170, 0, 320, 70, 180, C_DUSK);
    for (int k = 0; k < 6; k++) {
        int y = 74 + ((k * 20 + t) % 110);
        int w = 1 + (y - 70) / 20;
        gfx_rect(160 - w / 2, y, w, 2 + w, C_CREAM);
    }
    gfx_dither(0, 60, 320, 20, C_NIGHT, 6);
}

/* the house on fire, seen from the lane */
static void draw_fire(int t) {
    gfx_rect(0, 0, 320, 120, C_NIGHT);
    gfx_dither(0, 40, 320, 80, C_MAROON, 6 + (t / 6) % 3);
    house_silhouette(t, -1);
    for (int k = 0; k < 5; k++) {
        gfx_rect(86 + k * 32, 84, 12, 16, (t / 4 + k) % 3 ? C_ORANGE : C_YELLOW);
        gfx_rect(86 + k * 32, 110, 12, 16, (t / 5 + k) % 3 ? C_YELLOW : C_RED);
    }
    for (int k = 0; k < 12; k++) {
        int fx = 70 + k * 15, fh = 10 + ((t / 3 + k * 7) % 14);
        trap(fx + 3, fx + 4, fx - 2, fx + 9, 64 - fh, 72, k & 1 ? C_ORANGE : C_RED);
        trap(fx + 3, fx + 4, fx + 1, fx + 6, 70 - fh / 2, 72, C_YELLOW);
    }
}

static void draw_end_picture(int page) {
    int t = (int)wth.frame;
    if (wth.ending == END_CAR) {
        if (page == 0) {
            gfx_cls(C_INK);
            room_drive();
            gfx_rect(226, 84, 74, 20, C_FOREST);
            gfx_rect(232, 92, 10, 4, C_YELLOW);
            gfx_rect(284, 92, 10, 4, C_YELLOW);
        } else {
            draw_road(t);
            if (page == 2) gfx_dither(0, 0, 320, 70, C_SLATE, 6 + imin(8, wth.scene_t / 40));
        }
        return;
    }
    if (page == 0) {
        gfx_cls(C_INK);
        room_spore();
        gfx_dither(WTH_PX, WTH_PY, WTH_PW, WTH_PH, C_BLUE, imin(12, wth.scene_t / 8));
    } else draw_fire(t);
}

static const char *const CREDITS[] = {
    "WITHERLEA",
    "",
    "A UFO 40 CARTRIDGE",
    "TRIBUTE TO NIGHT MANOR",
    "UFO 50 #42, BY MOSSMOUTH",
    "",
    "THE MORROWS OF WITHERLEA",
    "ADA, TOM, PIP AND BARNABY",
    "AND THEIR DOG, WHEREVER HE WENT",
    "",
    "THE KEEPER",
    "",
    "THE MAN FROM THE WATER BOARD",
    "",
    "THANK YOU FOR PLAYING",
};

void wth_draw_screen(void) {
    int t = wth.scene_t;
    gfx_cls(C_INK);
    ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
    switch (wth.scene) {
    case SC_WARNING:
        centre("warn head", "A WORD BEFORE YOU GO IN", 40, C_RED);
        wrapped_centre("warn text",
                       "THIS CARTRIDGE HAS DEATH IN IT: DEAD PEOPLE, A DEAD DOG, BLOOD, AND THINGS THAT GROW IN THE "
                       "DEAD. IT IS MEANT FOR OLDER PLAYERS. PLAY WITH THE LIGHTS ON.",
                       64, 260, C_LIGHT);
        if (t > 30 && (t / 20) & 1) centre("warn go", GLYPH_A " GO IN", 140, C_GREY);
        break;
    case SC_TITLE: {
        gfx_rect(0, 0, 320, 140, C_NIGHT);
        gfx_dither(0, 60, 320, 80, C_NAVY, 5);
        gfx_circ(270, 30, 12, C_LIGHT);
        gfx_circ(275, 27, 11, C_NIGHT);
        house_silhouette(t, 3);
        static const uint8_t grad[3] = {C_WHITE, C_ICE, C_GREY};
        ui_fancy_center("WITHERLEA", 160, 10, 3, grad, 3, C_INK, C_NAVY);
        ui_audit_fancy("title", "WITHERLEA", 160 - ui_fancy_width("WITHERLEA", 3) / 2, 10, 3, true);
        bool has = false;
        {
            /* the menu: CONTINUE only when there is a save */
            WthSave tmp;
            has = game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp &&
                  tmp.magic == WTH_MAGIC && tmp.started;
        }
        int y = 148;
        if (has) {
            centre("menu", "CONTINUE", y, wth.title_sel == 0 ? C_WHITE : C_GREY);
            y += 12;
        }
        const char *ng = has && wth.title_erase ? "START OVER? " GLYPH_A " AGAIN" : "NEW GAME";
        centre("menu", ng, y, (has ? wth.title_sel == 1 : true) ? (wth.title_erase ? C_RED : C_WHITE) : C_GREY);
        int sel_y = has ? 148 + wth.title_sel * 12 : 148;
        ui_cursor(160 - text_width(has && wth.title_sel == 0 ? "CONTINUE" : ng) / 2 - 10, sel_y, t);
        break;
    }
    case SC_INTRO: {
        int p = iclamp(wth.diary_sel, 0, 2);
        if (p == 0) draw_road(t);
        else if (p == 1) { draw_road(t / 4); gfx_circ(160, 80, 10, C_WHITE); if (t < 6) gfx_set_flash(2); }
        else { gfx_rect(0, 0, 320, 120, C_INK); gfx_dither(40, 80, 240, 40, C_AMBER, 2); }
        gfx_rect(0, 116, 320, 64, C_INK);
        wrapped_centre("intro", INTRO[p], 122, 290, C_CREAM);
        if (t > 20 && (t / 20) & 1) text_draw(GLYPH_A, 304, 170, C_GREY);
        break;
    }
    case SC_WAKE: {
        int f = imin(16, t / 4);
        gfx_dither(0, 0, 320, 120, C_SLATE, f / 2);
        wrapped_centre("wake",
                       wth.s.caught > 1 ? "YOU WAKE ON THE STRAW AGAIN, SHAKING. THE COLD STORE. ANOTHER DREAM, IF IT WAS ONE. "
                                          "YOU STILL HAVE EVERYTHING YOU HAD, AND THE DOOR STANDS OPEN."
                                        : "YOU WAKE WITH A SHOUT, ON THE STRAW IN THE COLD STORE. A DREAM? YOUR NECK IS WHOLE. "
                                          "YOUR POCKETS STILL HOLD EVERYTHING YOU HAD, AND THE DOOR STANDS OPEN.",
                       122, 290, C_CREAM);
        if (t > 60 && (t / 20) & 1) text_draw(GLYPH_A, 304, 170, C_GREY);
        break;
    }
    case SC_ENDING: {
        int p = wth.diary_sel;
        if (p >= 3) {
            /* the best ending: dawn */
            gfx_rect(0, 0, 320, 120, C_SKY);
            gfx_dither(0, 60, 320, 60, C_AMBER, 8);
            gfx_circ(160, 120, 30, C_YELLOW);
            gfx_rect(0, 116, 320, 64, C_INK);
            centre("best", "THE NIGHTMARE IS OVER.", 130, C_YELLOW);
            centre("best", "AND IT NEVER ONCE CAUGHT YOU.", 146, C_CREAM);
        } else {
            draw_end_picture(p);
            gfx_rect(0, 116, 320, 64, C_INK);
            wrapped_centre("ending", (wth.ending == END_CAR ? TXT_END_CAR : TXT_END_CELLAR)[p], 122, 290, C_CREAM);
        }
        if (t > 30 && (t / 20) & 1) text_draw(GLYPH_A, 304, 170, C_GREY);
        break;
    }
    case SC_CREDITS: {
        int n = ARRAY_LEN(CREDITS);
        int y0 = 180 - t / 2;
        for (int i = 0; i < n; i++) {
            int y = y0 + i * 14;
            if (y > -10 && y < 180) {
                int w = text_width(CREDITS[i]);
                text_draw(CREDITS[i], 160 - w / 2, y, i == 0 ? C_ICE : C_LIGHT);
            }
        }
        char buf[64];
        int y = y0 + n * 14 + 10;
        snprintf(buf, sizeof buf, "PAGES FOUND: %d OF %d", wth_page_count(), WTH_PAGES);
        if (y > -10 && y < 180) text_draw(buf, 160 - text_width(buf) / 2, y, C_GREY);
        snprintf(buf, sizeof buf, "CAUGHT %d TIMES. GOT AWAY %d TIMES.", wth.s.caught, wth.s.escapes);
        if (y + 14 > -10 && y + 14 < 180) text_draw(buf, 160 - text_width(buf) / 2, y + 14, C_GREY);
        break;
    }
    case SC_STINGER: {
        /* in the car, the mirror */
        gfx_rect(0, 0, 320, 180, C_NIGHT);
        draw_road(t / 2);
        gfx_rect(0, 120, 320, 60, C_INK);
        gfx_rect(110, 10, 100, 34, C_INK);
        gfx_rect(114, 13, 92, 28, C_DUSK);
        int fx = 160;
        gfx_circ(fx, 30, 10, C_TAN);
        gfx_rect(fx - 6, 27, 3, 2, C_INK);
        gfx_rect(fx + 3, 27, 3, 2, C_INK);
        if (t >= 90) {
            gfx_rect(fx - 6, 27, 3, 2, C_WHITE);
            gfx_rect(fx + 3, 27, 3, 2, C_WHITE);
            for (int k = 0; k < 6; k++) gfx_line(fx - 8 + k * 3, 22 + (k & 1) * 4, fx - 6 + k * 3, 36, C_WHITE);
            wrapped_centre("sting", "SOMETHING PALE MOVES UNDER THE SKIN OF YOUR FACE.", 140, 280, C_LIGHT);
        } else centre("sting", "YOU GLANCE UP AT THE MIRROR.", 140, C_GREY);
        break;
    }
    default: break;
    }
}

/* ------------------------------------------------------------------ */
/* the cartridge label                                                  */

void wth_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_NIGHT);
    gfx_dither(x, y + h / 3, w, h / 3, C_NAVY, 6);
    gfx_circ(x + w - 16, y + 18, 7, C_LIGHT);
    gfx_circ(x + w - 13, y + 16, 6, C_NIGHT);
    int bx = x + 10, by = y + h - 36;
    gfx_rect(bx, by + 10, w - 20, 26, C_INK);
    trap(bx + (w - 20) / 2, bx + (w - 20) / 2, bx - 4, bx + w - 16, by - 6, by + 10, C_INK);
    for (int k = 0; k < 4; k++) gfx_rect(bx + 6 + k * ((w - 34) / 3), by + 16, 6, 8, k == 2 && (t / 40) % 5 ? C_AMBER : C_NIGHT);
    wth_draw_keeper(x + w - 30, y + h - 30, 1, t, 0);
    static const uint8_t grad[2] = {C_WHITE, C_ICE};
    ui_fancy_text("WITHERLEA", x + 5, y + 4, 1, grad, 2, C_INK, C_NAVY);
    gfx_noclip();
}

/* ------------------------------------------------------------------ */
/* tests: words stay inside their boxes and clear of each other          */

int wth_layout_audit(void) {
    int bad = 0;
    WthState keep = wth;
    static const int SCREENS[] = {SC_WARNING, SC_TITLE, SC_INTRO, SC_WAKE, SC_ENDING, SC_STINGER};
    for (int i = 0; i < ARRAY_LEN(SCREENS); i++) {
        int pages = SCREENS[i] == SC_INTRO || SCREENS[i] == SC_ENDING ? 4 : 1;
        for (int e = END_CAR; e <= (SCREENS[i] == SC_ENDING ? END_CELLAR : END_CAR); e++)
            for (int p = 0; p < pages; p++) {
                if (SCREENS[i] == SC_INTRO && p >= 3) continue;
                wth.scene = SCREENS[i];
                wth.scene_t = 200;
                wth.diary_sel = p;
                wth.ending = e;
                wth.ending_best = p == 3;
                if (SCREENS[i] == SC_ENDING && p == 3 && e == END_CAR) continue;
                ui_audit_begin("witherlea", true);
                wth_draw_screen();
                bad += ui_audit_end();
            }
    }
    /* the diary, every page */
    wth.scene = SC_DIARY;
    wth.s.pages = (1u << WTH_PAGES) - 1;
    for (int p = 0; p < WTH_PAGES; p++) {
        wth.diary_sel = p;
        ui_audit_begin("witherlea diary", true);
        wth_draw_diary();
        bad += ui_audit_end();
    }
    /* the bag with every item's words */
    wth.scene = SC_BAG;
    for (int it = 1; it < I_COUNT; it++) {
        memset(wth.s.bag, 0, sizeof wth.s.bag);
        wth.s.bag[0] = (uint8_t)it;
        wth.bag_sel = 0;
        wth.held = it;
        ui_audit_begin("witherlea bag", true);
        wth_draw_bag();
        bad += ui_audit_end();
    }
    wth = keep;
    return bad;
}
