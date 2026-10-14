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
    /* the vent */
    const WthSpot *v = SP(S_CELL_VENT);
    gfx_rect(v->x, v->y, v->w, v->h, C_INK);
    if (!wth_flag(F_VENT_OPEN)) {
        for (int x = v->x + 2; x < v->x + v->w - 1; x += 3) gfx_vline(x, v->y + 1, v->y + v->h - 2, C_GREY);
        frame(v->x, v->y, v->w, v->h, C_LIGHT);
    } else {
        frame(v->x, v->y, v->w, v->h, C_GREY);
        if (!wth_flag(F_VENT_WIRE)) gfx_line(v->x + 6, v->y + 12, v->x + 14, v->y + 9, C_LIGHT);
        if (!wth_flag(F_VENT_PAPER)) gfx_rect(v->x + 18, v->y + 9, 7, 5, C_CREAM);
        /* the grille on the floor */
        for (int x = 30; x < 56; x += 3) gfx_vline(x, 92, 97, C_GREY);
    }
    door_back(SP(S_CELL_DOOR)->x, 42, 48, 58, C_BROWN, wth_flag(F_CELL_OPEN));
    if (!wth_flag(F_CELL_OPEN)) gfx_hline(136, 183, 99, C_INK);
    /* the ring */
    gfx_circb(104, 54, 5, C_GREY);
    gfx_rect(102, 47, 4, 3, C_GREY);
    /* straw */
    const WthSpot *st = SP(S_CELL_STRAW);
    for (int i = 0; i < 70; i++) {
        int x = st->x + (i * 37) % st->w, y = st->y + 4 + (i * 13) % (st->h - 6);
        gfx_line(x, y, x + 5, y - 2, i & 1 ? C_AMBER : C_TAN);
    }
    if (!wth_flag(F_STRAW)) gfx_rect(st->x + 40, st->y + 12, 8, 3, C_GREY);
    /* the tray and the mug */
    const WthSpot *tr = SP(S_CELL_TRAY);
    trap(tr->x + 4, tr->x + tr->w - 4, tr->x, tr->x + tr->w, tr->y + 6, tr->y + 12, C_GREY);
    if (!wth_flag(F_MUG_TAKEN)) {
        gfx_rect(tr->x + 16, tr->y, 9, 8, C_LIGHT);
        gfx_circb(tr->x + 27, tr->y + 4, 2, C_LIGHT);
    }
    /* the barrel */
    const WthSpot *b = SP(S_CELL_BARREL);
    gfx_rect(b->x, b->y, b->w, b->h, C_BROWN);
    gfx_hline(b->x, b->x + b->w - 1, b->y + 8, C_GREY);
    gfx_hline(b->x, b->x + b->w - 1, b->y + b->h - 9, C_GREY);
    gfx_rect(b->x + 2, b->y - 3, b->w - 4, 4, C_EARTH);
}

static void room_passage(void) {
    interior(C_MAROON, C_WINE, C_BROWN, C_NIGHT);
    wallpaper_stripes(C_WINE);
    for (int x = 12; x < 66; x += 9) gfx_line(x, 12, x, 120, C_MAROON);
    door_back(142, 42, 36, 58, C_EARTH, false);
    door_side(true, C_EARTH, false);
    door_side(false, C_EARTH, false);
    /* the coat */
    const WthSpot *c = SP(S_PAS_COAT);
    gfx_rect(c->x + 9, c->y, 4, 3, C_GREY);
    trap(c->x + 6, c->x + 16, c->x, c->x + c->w, c->y + 3, c->y + c->h - 1, C_NAVY);
    gfx_vline(c->x + 11, c->y + 6, c->y + c->h - 2, C_NIGHT);
    /* the rug */
    const WthSpot *r = SP(S_PAS_RUG);
    trap(r->x + 12, r->x + r->w - 12, r->x, r->x + r->w, r->y, r->y + r->h - 1, C_RED);
    trap(r->x + 16, r->x + r->w - 16, r->x + 6, r->x + r->w - 6, r->y + 3, r->y + r->h - 4, C_MAROON);
    page_on(S_PAS_RUG, r->x + 52, r->y + 5);
    /* the barometer */
    const WthSpot *b = SP(S_PAS_BAROMETER);
    gfx_rect(b->x + 3, b->y, 8, b->h, C_BROWN);
    gfx_circ(b->x + 7, b->y + 9, 5, C_CREAM);
    gfx_line(b->x + 7, b->y + 9, b->x + 10, b->y + 6, C_INK);
}

static void room_sickroom(void) {
    interior(C_LIGHT, C_GREY, C_BROWN, C_SLATE);
    floorboards(C_EARTH);
    window_back(96, 32, 46, 34, C_NIGHT);
    for (int k = 0; k < 3; k++) gfx_line(94, 40 + k * 10, 144, 38 + k * 10, C_BROWN); /* nailed boards */
    /* the bed */
    const WthSpot *b = SP(S_SICK_BED);
    gfx_rect(b->x, b->y + 10, b->w, b->h - 18, C_LIGHT);
    gfx_rect(b->x, b->y + 24, b->w, 8, C_GREY);
    gfx_rect(b->x + b->w - 26, b->y + 12, 22, 10, C_WHITE);
    gfx_vline(b->x + b->w - 1, b->y, b->y + b->h - 1, C_SLATE);
    gfx_vline(b->x, b->y + 6, b->y + b->h - 1, C_SLATE);
    gfx_hline(b->x, b->x + b->w - 1, b->y + 6, C_SLATE);
    page_on(S_SICK_BED, b->x + b->w - 22, b->y + 13);
    /* the nightstand */
    const WthSpot *n = SP(S_SICK_STAND);
    gfx_rect(n->x, n->y, n->w, n->h, C_BROWN);
    frame(n->x + 3, n->y + 4, n->w - 6, 8, C_EARTH);
    gfx_pset(n->x + n->w / 2, n->y + 8, C_AMBER);
    /* the washstand */
    const WthSpot *w = SP(S_SICK_WASH);
    gfx_rect(w->x + 4, w->y + 16, w->w - 8, w->h - 16, C_BROWN);
    gfx_rect(w->x, w->y + 12, w->w, 5, C_LIGHT);
    gfx_rect(w->x + 14, w->y, 12, 13, C_WHITE);
    gfx_hline(w->x + 15, w->x + 24, w->y + 2, C_CREAM);
}

static void room_hall(void) {
    interior(C_FOREST, C_JADE, C_INK, C_NIGHT);
    for (int y = 101; y < 134; y += 4)
        for (int x = 8; x < 312; x += 8) gfx_rect(x + ((y / 4) & 1) * 4, y, 4, 4, C_LIGHT);
    trap(72, 248, 8, 311, 100, 101, C_INK);
    /* the front door, barred */
    door_back(138, 38, 44, 62, C_EARTH, false);
    gfx_rect(132, 66, 56, 4, C_GREY);
    for (int x = 140; x < 180; x += 5) gfx_circb(x, 68, 2, C_LIGHT);
    gfx_rect(156, 72, 8, 8, C_SLATE);
    /* the clock */
    const WthSpot *c = SP(S_HALL_CLOCK);
    gfx_rect(c->x, c->y, c->w, c->h, C_BROWN);
    gfx_rect(c->x - 2, c->y, c->w + 4, 4, C_EARTH);
    gfx_circ(c->x + c->w / 2, c->y + 14, 8, C_CREAM);
    if (wth_flag(F_CLOCK_WOUND)) gfx_vline(c->x + c->w / 2, c->y + 14, c->y + 21, C_INK);
    else {
        gfx_line(c->x + c->w / 2, c->y + 14, c->x + c->w / 2 + 4, c->y + 18, C_INK);
        gfx_line(c->x + c->w / 2, c->y + 14, c->x + c->w / 2 - 3, c->y + 9, C_INK);
    }
    gfx_rect(c->x + 6, c->y + 30, c->w - 12, 30, C_EARTH);
    int sw = wth_flag(F_CLOCK_WOUND) ? 0 : 0;
    gfx_circ(c->x + c->w / 2 + sw, c->y + 52, 3, C_AMBER);
    /* the umbrella stand */
    const WthSpot *u = SP(S_HALL_UMBRELLA);
    gfx_rect(u->x + 3, u->y + 8, 12, 14, C_GREY);
    if (!wth_flag(F_CANE_TAKEN)) gfx_line(u->x + 8, u->y + 9, u->x + 12, u->y - 10, C_TAN);
    /* the dining room door */
    door_back(198, 42, 30, 58, C_EARTH, false);
    door_side(true, C_EARTH, false);
    /* the stairs, up the right */
    for (int k = 0; k < 9; k++) {
        int y = 118 - k * 10, x = 254 + k * 3;
        gfx_rect(x, y, 56 - k * 3, 4, C_BROWN);
        gfx_rect(x, y + 4, 56 - k * 3, 6, C_EARTH);
    }
    if (!wth_flag(F_STAIRS_LIT)) gfx_dither(254, 26, 54, 50, C_INK, 14);
    gfx_line(254, 118, 290, 26, C_BROWN);
    /* the cog, once it rolled out */
}

static void room_parlour(void) {
    interior(C_WINE, C_MAROON, C_BROWN, C_NIGHT);
    wallpaper_stripes(C_MAROON);
    floorboards(C_EARTH);
    trap(80, 240, 40, 280, 104, 128, C_RED);
    /* the fireplace */
    const WthSpot *f = SP(S_PAR_FIRE);
    gfx_rect(f->x - 6, 50, f->w + 12, 6, C_LIGHT);
    gfx_rect(f->x, 56, f->w, 44, C_GREY);
    gfx_rect(f->x + 12, 66, f->w - 24, 34, C_INK);
    if (wth_flag(F_FIRE_LIT)) {
        for (int k = 0; k < 5; k++) {
            int fx = f->x + 16 + k * 8, fh = 8 + ((wth.frame / 4 + k * 3) % 7);
            trap(fx + 2, fx + 3, fx - 1, fx + 6, 99 - fh, 99, k & 1 ? C_ORANGE : C_YELLOW);
        }
        if (!wth_flag(F_GARNET_TAKEN)) gfx_rect(f->x + 30, 96, 4, 3, C_RED);
    } else {
        /* the wax man */
        int cx = f->x + f->w / 2;
        gfx_circ(cx, 76, 5, C_CREAM);
        gfx_rect(cx - 7, 81, 14, 16, C_CREAM);
        gfx_rect(cx - 2, 86, 4, 3, wth_flag(F_FIRE_SOAKED) ? C_MAROON : C_RED);
        gfx_pset(cx - 2, 75, C_INK);
        gfx_pset(cx + 2, 75, C_INK);
        if (wth_flag(F_FIRE_SOAKED)) gfx_dither(f->x + 12, 90, f->w - 24, 10, C_TAN, 6);
    }
    /* the pig on the mantel */
    if (!wth_flag(F_PIG_TAKEN)) {
        const WthSpot *p = SP(S_PAR_PIG);
        gfx_circ(p->x + 10, p->y + 7, 5, C_PINK);
        gfx_rect(p->x + 5, p->y + 10, 2, 2, C_PINK);
        gfx_rect(p->x + 13, p->y + 10, 2, 2, C_PINK);
        gfx_pset(p->x + 15, p->y + 5, C_INK);
        gfx_rect(p->x + 9, p->y + 2, 3, 1, C_INK);
    }
    /* the radiogram */
    const WthSpot *r = SP(S_PAR_RADIO);
    gfx_rect(r->x, r->y, r->w, r->h, C_BROWN);
    gfx_rect(r->x + 3, r->y + 4, r->w - 6, 14, C_TAN);
    for (int x = r->x + 5; x < r->x + r->w - 5; x += 3) gfx_vline(x, r->y + 6, r->y + 15, C_EARTH);
    gfx_circ(r->x + 8, r->y + 26, 3, C_AMBER);
    gfx_circ(r->x + r->w - 9, r->y + 26, 3, C_AMBER);
    /* the folding screen */
    const WthSpot *s = SP(S_PAR_SCREEN);
    bool gone = wth_flag(F_HIDE_SCREEN_GONE);
    for (int k = 0; k < 3; k++) {
        int x = s->x + k * 13;
        gfx_rect(x, s->y, 12, s->h, gone ? C_TAN : C_CREAM);
        frame(x, s->y, 12, s->h, C_BROWN);
        if (!gone) {
            gfx_line(x + 3, s->y + 30, x + 8, s->y + 18, C_SLATE);
            gfx_pset(x + 8, s->y + 17, C_INK);
        } else
            for (int j = 0; j < 4; j++) gfx_line(x + 1, s->y + 6 + j * 12, x + 10, s->y + 12 + j * 12, C_INK);
    }
    /* the sofa */
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
    /* the orchestrelle */
    const WthSpot *o = SP(S_DIN_ORCH);
    gfx_rect(o->x, o->y, o->w, o->h, C_BROWN);
    gfx_rect(o->x + 4, o->y + 6, o->w - 8, 26, C_NIGHT);
    for (int k = 0; k < 5; k++) gfx_rect(o->x + 6 + k * 5, o->y + 10 + (k & 1) * 4, 3, 18 - (k & 1) * 4, C_AMBER);
    gfx_circ(o->x + o->w / 2, o->y + 44, 6, C_RED);
    gfx_rect(o->x + o->w / 2 - 3, o->y + 56, 6, 2, C_INK);
    /* the portrait */
    const WthSpot *p = SP(S_DIN_PORTRAIT);
    picture_frame(p->x, p->y, p->w, p->h, C_DUSK);
    for (int k = 0; k < 4; k++) {
        int fx = p->x + 9 + k * 11, fy = p->y + 9 + (k >= 2 ? 4 : 0);
        gfx_circ(fx, fy, 3, k == 0 ? C_GREY : C_TAN);
        gfx_rect(fx - 3, fy + 4, 7, 12, k == 0 ? C_NIGHT : k == 1 ? C_WINE : C_SKY);
    }
    for (int k = 0; k < 4; k++) gfx_line(p->x + 6, p->y + 6 + k, p->x + 12, p->y + 8 + k, C_LIGHT);
    /* the sideboard */
    const WthSpot *s = SP(S_DIN_SIDE);
    gfx_rect(s->x, s->y, s->w, s->h, C_BROWN);
    gfx_hline(s->x, s->x + s->w - 1, s->y + 10, C_EARTH);
    gfx_pset(s->x + s->w / 2, s->y + 5, C_AMBER);
    gfx_rect(s->x + 6, s->y - 8, 6, 8, C_LIGHT);
    gfx_rect(s->x + 30, s->y - 10, 8, 10, C_AMBER);
    /* the table, laid */
    const WthSpot *t = SP(S_DIN_TABLE);
    trap(t->x + 20, t->x + t->w - 20, t->x, t->x + t->w, t->y, t->y + 12, C_CREAM);
    gfx_rect(t->x + 4, t->y + 13, t->w - 8, 3, C_BROWN);
    gfx_rect(t->x + 10, t->y + 16, 4, t->h - 16, C_BROWN);
    gfx_rect(t->x + t->w - 14, t->y + 16, 4, t->h - 16, C_BROWN);
    for (int k = 0; k < 4; k++) {
        int px = t->x + 30 + k * 34;
        gfx_circb(px, t->y + 6, 5, C_LIGHT);
        gfx_circ(px, t->y + 6, 2, C_AMBER);
    }
    page_on(S_DIN_TABLE, t->x + 84, t->y + 2);
    door_side(false, C_EARTH, false);
}

static void room_kitchen(void) {
    interior(C_CREAM, C_TAN, C_GREY, C_BROWN);
    for (int y = 30; y < 70; y += 6) gfx_hline(73, 247, y, C_TAN);
    for (int y = 104; y < 134; y += 8) gfx_hline(8, 311, y, C_SLATE);
    door_back(82, 42, 30, 58, C_BROWN, false);
    door_side(true, C_BROWN, false);
    /* the shelf of books */
    const WthSpot *b = SP(S_KIT_BOOK);
    gfx_rect(b->x - 4, b->y + b->h, b->w + 8, 2, C_BROWN);
    for (int k = 0; k < 8; k++) gfx_rect(b->x + k * 6, b->y + 2 + (k % 3), 5, b->h - 2 - (k % 3), k & 1 ? C_RED : C_FOREST);
    page_on(S_KIT_BOOK, b->x + 40, b->y + 4);
    /* the sink, full of jars */
    const WthSpot *s = SP(S_KIT_SINK);
    gfx_rect(s->x, s->y + 10, s->w, s->h - 10, C_LIGHT);
    gfx_rect(s->x + 4, s->y + 12, s->w - 8, 8, C_GREY);
    for (int k = 0; k < 6; k++) {
        gfx_rect(s->x + 6 + k * 9, s->y + 2 + (k & 1) * 2, 7, 10, C_AMBER);
        gfx_rect(s->x + 6 + k * 9, s->y + 1 + (k & 1) * 2, 7, 2, C_CREAM);
    }
    /* the stove */
    const WthSpot *st = SP(S_KIT_STOVE);
    gfx_rect(st->x, st->y + 6, st->w, st->h - 6, C_INK);
    gfx_rect(st->x + 4, st->y + 18, st->w - 8, 16, C_SLATE);
    gfx_rect(st->x + 18, st->y - 26, 10, 32, C_INK);
    if (!wth_flag(F_MATCHES_TAKEN)) {
        gfx_rect(st->x + 6, st->y + 2, 10, 5, C_YELLOW);
        gfx_hline(st->x + 6, st->x + 15, st->y + 4, C_RED);
    }
    /* the counter and the knife block */
    trap(250, 311, 256, 311, 74, 100, C_BROWN);
    const WthSpot *k = SP(S_KIT_KNIVES);
    gfx_rect(k->x + 4, k->y + 4, 14, 12, C_EARTH);
    for (int i = 0; i < 4; i++)
        if (i > 0 || !wth_flag(F_KNIFE_TAKEN)) gfx_rect(k->x + 6 + i * 3, k->y - 2 + (i == 0 ? -3 : 0), 2, 6 + (i == 0 ? 3 : 0), C_INK);
    if (wth_flag(F_KNIFE_TAKEN))
        for (int i = 1; i < 4; i++) gfx_rect(k->x + 6 + i * 3, k->y - 2, 2, 6, C_EARTH);
}

static void room_scullery(void) {
    interior(C_LIGHT, C_GREY, C_SLATE, C_GREY);
    bricks(73, 27, 247, 99, C_GREY);
    door_back(90, 42, 30, 58, C_FOREST, false);
    door_side(false, C_SLATE, false);
    /* the shelf */
    gfx_rect(136, 52, 64, 3, C_BROWN);
    if (!wth_flag(F_SODA_TAKEN)) {
        const WthSpot *s = SP(S_SCU_SODA);
        gfx_rect(s->x + 3, s->y + 2, 10, 14, C_RED);
        gfx_rect(s->x + 3, s->y + 6, 10, 4, C_CREAM);
    }
    gfx_rect(138, 40, 8, 12, C_BLUE);
    diary_page(S_SCU_DIARY);
    /* the mangle */
    const WthSpot *m = SP(S_SCU_MANGLE);
    gfx_rect(m->x + 6, m->y + 4, m->w - 12, 8, C_LIGHT);
    gfx_rect(m->x + 6, m->y + 14, m->w - 12, 8, C_LIGHT);
    gfx_rect(m->x + 2, m->y, 4, m->h, C_INK);
    gfx_rect(m->x + m->w - 6, m->y, 4, m->h, C_INK);
    gfx_circb(m->x + m->w + 2, m->y + 6, 6, C_INK);
    /* the copper tub */
    const WthSpot *t = SP(S_SCU_TUB);
    trap(t->x, t->x + t->w, t->x + 6, t->x + t->w - 6, t->y + 4, t->y + t->h - 1, C_ORANGE);
    gfx_dither(t->x + 4, t->y + 8, t->w - 8, t->h - 10, C_JADE, 5);
    gfx_hline(t->x, t->x + t->w, t->y + 4, C_AMBER);
    if (!wth_flag(F_BAR_TAKEN)) gfx_rect(t->x + t->w - 2, t->y + t->h - 4, 10, 2, C_SLATE);
}

static void room_smoking(void) {
    interior(C_EARTH, C_BROWN, C_BROWN, C_INK);
    for (int x = 76; x < 248; x += 12) gfx_vline(x, 60, 99, C_BROWN);
    gfx_hline(72, 248, 60, C_TAN);
    floorboards(C_EARTH);
    trap(84, 236, 44, 276, 104, 130, C_FOREST);
    door_side(true, C_BROWN, false);
    door_side(false, C_BROWN, false);
    /* the alcove */
    const WthSpot *a = SP(S_SMO_ALCOVE);
    gfx_rect(a->x, a->y, a->w, a->h, C_INK);
    if (wth_flag(F_HIDE_ALCOVE_GONE)) {
        for (int k = 0; k < 5; k++) gfx_line(a->x + k * 7, a->y + a->h - 4, a->x + k * 7 + 5, a->y + a->h - 1, C_RED);
    } else {
        for (int x = a->x; x < a->x + a->w; x += 4) gfx_rect(x, a->y, 3, a->h, C_RED);
        for (int x = a->x + 2; x < a->x + a->w; x += 4) gfx_vline(x, a->y, a->y + a->h - 1, C_MAROON);
    }
    gfx_rect(a->x - 2, a->y - 2, a->w + 4, 3, C_AMBER);
    /* the desk */
    const WthSpot *d = SP(S_SMO_DESK);
    gfx_rect(d->x, d->y, d->w, 6, C_TAN);
    gfx_rect(d->x + 2, d->y + 6, d->w - 4, d->h - 6, C_BROWN);
    frame(d->x + 8, d->y + 9, d->w - 16, 8, C_EARTH);
    gfx_rect(d->x + 10, d->y - 4, 12, 4, C_CREAM);
    /* the humidor on a side table */
    const WthSpot *h = SP(S_SMO_HUMIDOR);
    gfx_rect(h->x, h->y + 4, h->w, 10, C_EARTH);
    gfx_rect(h->x + 2, h->y + 2, h->w - 4, 3, C_BROWN);
    gfx_rect(h->x + 6, h->y + 14, 12, 14, C_BROWN);
    /* the chess */
    const WthSpot *c = SP(S_SMO_CHESS);
    trap(c->x + 6, c->x + c->w - 6, c->x, c->x + c->w, c->y, c->y + 8, C_CREAM);
    for (int k = 0; k < 6; k++) gfx_rect(c->x + 6 + k * 6, c->y + 2 + (k & 1), 3, 4, k < 4 ? C_WHITE : C_INK);
    /* the chair */
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

static void bathtub(int s, bool black) {
    const WthSpot *b = SP(s);
    gfx_rect(b->x, b->y + 8, b->w, b->h - 12, C_WHITE);
    gfx_rect(b->x + 4, b->y + 8, b->w - 8, 6, black ? C_INK : C_LIGHT);
    gfx_hline(b->x, b->x + b->w - 1, b->y + 8, C_LIGHT);
    gfx_rect(b->x + 6, b->y + b->h - 4, 6, 4, C_AMBER);
    gfx_rect(b->x + b->w - 12, b->y + b->h - 4, 6, 4, C_AMBER);
    gfx_rect(b->x + b->w - 20, b->y, 4, 9, C_GREY);
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
    bathtub(S_WAS_BATH, !wth_flag(F_BATH_DRAINED));
    if (wth_flag(F_BATH_DRAINED) && !wth_flag(F_TINKEY_TAKEN)) gfx_pset(184, 81, C_YELLOW);
    const WthSpot *c = SP(S_WAS_CAB);
    gfx_rect(c->x, c->y, c->w, c->h, C_WHITE);
    frame(c->x, c->y, c->w, c->h, C_GREY);
    gfx_vline(c->x + c->w / 2, c->y, c->y + c->h - 1, C_GREY);
    toilet(S_WAS_TOILET);
}

static void room_guest(void) {
    interior(C_PURPLE, C_VIOLET, C_BROWN, C_NIGHT);
    for (int x = 80; x < 248; x += 14)
        for (int y = 32; y < 98; y += 14) gfx_pset(x, y, C_VIOLET);
    floorboards(C_EARTH);
    window_back(SP(S_GUE_WINDOW)->x, SP(S_GUE_WINDOW)->y, 40, 30, C_NIGHT);
    /* the bed */
    const WthSpot *b = SP(S_GUE_BED);
    bool gone = wth_flag(F_HIDE_BED_GONE);
    if (gone) {
        gfx_rect(b->x + 10, b->y, b->w - 20, b->h, C_BROWN);
        for (int k = 0; k < 6; k++) gfx_rect(b->x + 16 + k * 12, b->y + 8 + (k & 1) * 10, 4, 6, C_INK);
    } else {
        gfx_rect(b->x, b->y + 8, b->w, 16, C_CREAM);
        gfx_rect(b->x, b->y + 24, b->w, b->h - 28, C_PINK);
        for (int x = b->x + 4; x < b->x + b->w; x += 6) gfx_vline(x, b->y + 26, b->y + b->h - 6, C_MAGENTA);
        gfx_rect(b->x + 6, b->y + 2, 24, 10, C_WHITE);
        gfx_rect(b->x - 2, b->y - 8, 4, b->h + 4, C_BROWN);
    }
    /* the writing box on its table */
    const WthSpot *w = SP(S_GUE_BOX);
    gfx_rect(w->x - 4, w->y + w->h, w->w + 8, 3, C_BROWN);
    gfx_rect(w->x, w->y + w->h + 3, 3, 22, C_BROWN);
    gfx_rect(w->x + w->w - 3, w->y + w->h + 3, 3, 22, C_BROWN);
    gfx_rect(w->x + 4, w->y + 4, w->w - 8, w->h - 4, C_EARTH);
    if (wth_flag(F_BOX_OPEN)) gfx_rect(w->x + 4, w->y - 4, w->w - 8, 6, C_EARTH);
    gfx_pset(w->x + w->w / 2, w->y + 8, C_LIGHT);
    page_on(S_GUE_BOX, w->x + 14, w->y + 6);
    /* the sewing table */
    const WthSpot *s = SP(S_GUE_SEW);
    gfx_rect(s->x, s->y + 14, s->w, 4, C_BROWN);
    gfx_rect(s->x + 4, s->y + 18, 3, s->h - 18, C_INK);
    gfx_rect(s->x + s->w - 7, s->y + 18, 3, s->h - 18, C_INK);
    gfx_rect(s->x + 6, s->y + 2, 22, 12, C_INK);
    gfx_rect(s->x + 6, s->y + 2, 22, 3, C_AMBER);
    if (!wth_flag(F_OIL_TAKEN)) gfx_rect(s->x + 34, s->y + 6, 5, 8, C_GREY);
}

static void room_glasshouse(void) {
    night_sky(134);
    gfx_rect(8, 100, 304, 34, C_SLATE);
    for (int y = 104; y < 134; y += 6) gfx_hline(8, 311, y, C_DUSK);
    for (int x = 8; x < 312; x += 30) gfx_vline(x, 6, 100, C_LIGHT);
    for (int y = 30; y < 100; y += 24) gfx_hline(8, 311, y, C_LIGHT);
    gfx_line(8, 30, 160, 6, C_LIGHT);
    gfx_line(311, 30, 160, 6, C_LIGHT);
    /* the cracked pane */
    const WthSpot *p = SP(S_GLA_PANE);
    gfx_line(p->x + 4, p->y + 4, p->x + 20, p->y + 22, C_WHITE);
    gfx_line(p->x + 20, p->y + 22, p->x + 38, p->y + 14, C_WHITE);
    gfx_line(p->x + 20, p->y + 22, p->x + 24, p->y + 38, C_WHITE);
    /* the door into the garden */
    const WthSpot *d = SP(S_GLA_GARDEN);
    frame(d->x, d->y, d->w, d->h, C_WHITE);
    gfx_vline(d->x + d->w / 2, d->y, d->y + d->h - 1, C_WHITE);
    gfx_dither(d->x + 1, d->y + 30, d->w - 2, d->h - 31, C_FOREST, 8);
    /* the trellis and its vine */
    const WthSpot *t = SP(S_GLA_TRELLIS);
    for (int k = 0; k < 6; k++) {
        gfx_line(t->x, t->y + k * 12, t->x + t->w, t->y + k * 12 + 12, C_TAN);
        gfx_line(t->x + t->w, t->y + k * 12, t->x, t->y + k * 12 + 12, C_TAN);
    }
    for (int k = 0; k < 10; k++) gfx_pset(t->x + 10 + (k * 7) % 30, t->y + 6 + k * 6, C_EARTH);
    if (!wth_flag(F_HOOK_TAKEN)) gfx_circb(t->x + 26, t->y + 20, 2, C_AMBER);
    /* the dead plants, white-furred */
    const WthSpot *pl = SP(S_GLA_PLANTS);
    gfx_rect(pl->x, pl->y + 20, pl->w, pl->h - 20, C_BROWN);
    for (int k = 0; k < 6; k++) {
        int x = pl->x + 8 + k * 13;
        gfx_vline(x, pl->y - 10, pl->y + 20, C_TAN);
        gfx_dither(x - 4, pl->y - 6, 9, 24, C_WHITE, 4);
    }
}

static void room_garage(void) {
    interior(C_GREY, C_SLATE, C_SLATE, C_DUSK);
    for (int y = 104; y < 134; y += 10) gfx_hline(8, 311, y, C_DUSK);
    /* the door */
    const WthSpot *d = SP(S_GAR_DOOR);
    if (wth_flag(F_GARAGE_OPEN)) {
        gfx_rect(76, 28, 168, 72, C_NIGHT);
        gfx_dither(76, 70, 168, 30, C_DUSK, 8);
        gfx_rect(76, 28, 168, 6, C_LIGHT);
    } else {
        gfx_rect(76, 28, 168, 72, C_LIGHT);
        for (int y = 34; y < 100; y += 8) gfx_hline(76, 243, y, C_GREY);
    }
    (void)d;
    /* the opener on the ceiling, and its rails */
    const WthSpot *o = SP(S_GAR_OPENER);
    gfx_line(o->x + o->w / 2 - 4, o->y + 12, 120, 26, C_GREY);
    gfx_line(o->x + o->w / 2 + 4, o->y + 12, 200, 26, C_GREY);
    gfx_rect(o->x, o->y, o->w, o->h, C_SLATE);
    frame(o->x, o->y, o->w, o->h, C_INK);
    if (wth_flag(F_MOTOR_FITTED)) gfx_rect(o->x + 14, o->y + 3, 16, 8, C_RED);
    else gfx_rect(o->x + 14, o->y + 3, 16, 8, C_INK);
    if (!wth_flag(F_RAILS_OILED)) gfx_dither(110, 18, 100, 8, C_ORANGE, 5);
    /* the button */
    const WthSpot *b = SP(S_GAR_BUTTON);
    gfx_rect(b->x, b->y, b->w, b->h, C_CREAM);
    gfx_circ(b->x + b->w / 2, b->y + b->h / 2, 3, C_RED);
    gfx_line(b->x + b->w / 2, b->y + b->h, b->x + b->w / 2, 26, C_INK);
    /* the tool board */
    gfx_rect(250, 34, 50, 30, C_TAN);
    for (int x = 252; x < 300; x += 4)
        for (int y = 36; y < 62; y += 4) gfx_pset(x, y, C_BROWN);
    if (!wth_flag(F_SPANNER_TAKEN)) {
        gfx_rect(256, 44, 14, 3, C_LIGHT);
        gfx_circb(257, 45, 3, C_LIGHT);
    }
    if (!wth_flag(F_TWINE_TAKEN)) gfx_circ(287, 51, 5, C_INK);
    /* the shelf */
    const WthSpot *s = SP(S_GAR_SHELF);
    gfx_rect(s->x, s->y + s->h - 2, s->w, 3, C_BROWN);
    for (int k = 0; k < 3; k++) gfx_rect(s->x + 2 + k * 6, s->y + 6, 5, 12, C_AMBER);
    if (!wth_flag(F_PARAFFIN_TAKEN)) gfx_rect(s->x + 22, s->y + 2, 10, 16, C_RED);
    /* the car */
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
}

static void room_pond(void) {
    night_sky(56);
    gfx_rect(8, 56, 304, 78, C_FOREST);
    gfx_dither(8, 56, 304, 20, C_INK, 6);
    gfx_circ(160, 92, 70, C_NAVY);
    gfx_rect(8, 112, 304, 22, C_FOREST);
    gfx_circ(160, 92, 62, C_NIGHT);
    for (int k = 0; k < 6; k++) gfx_hline(110 + k * 7, 140 + k * 9, 70 + k * 6, C_NAVY);
    /* the body */
    const WthSpot *b = SP(S_POND_BODY);
    if (!wth_flag(F_POND_DONE)) {
        gfx_rect(b->x + 12, b->y + 6, b->w - 24, 8, C_NAVY);
        gfx_rect(b->x + 14, b->y + 7, 16, 6, C_ORANGE);
        gfx_circ(b->x + b->w - 14, b->y + 9, 4, C_TAN);
        gfx_rect(b->x + 4, b->y + 8, 10, 3, C_TAN);
    } else {
        gfx_rect(40, 116, 50, 8, C_NAVY);
        gfx_circ(92, 119, 4, C_TAN);
        gfx_rect(44, 117, 12, 6, C_ORANGE);
    }
    /* lilies and reeds */
    const WthSpot *l = SP(S_POND_LILIES);
    for (int k = 0; k < 5; k++) {
        gfx_circ(l->x + 8 + k * 13, l->y + 8 + (k & 1) * 5, 5, C_FOREST);
        gfx_circ(l->x + 8 + k * 13, l->y + 6 + (k & 1) * 5, 2, C_WHITE);
    }
    const WthSpot *r = SP(S_POND_REEDS);
    for (int k = 0; k < 14; k++) {
        int x = r->x + k * 4;
        int lean = (k >= 6 && k <= 8) ? 10 : 0;
        gfx_line(x, r->y + r->h, x + lean, r->y + (k * 7) % 14 + lean, C_LEAF);
    }
}

static void room_shed(void) {
    interior(C_BROWN, C_EARTH, C_EARTH, C_INK);
    for (int x = 72; x < 248; x += 8) gfx_vline(x, 27, 99, C_EARTH);
    for (int x = 12; x < 66; x += 9) gfx_line(x, 10, x, 122, C_BROWN);
    /* the bench */
    gfx_rect(20, 92, 120, 6, C_TAN);
    gfx_rect(24, 98, 4, 26, C_TAN);
    gfx_rect(132, 98, 4, 26, C_TAN);
    /* the grinder */
    const WthSpot *g = SP(S_SHE_GRINDER);
    gfx_rect(g->x + 12, g->y + 10, 20, 12, wth_flag(F_MOTOR_TAKEN) ? C_INK : C_RED);
    gfx_circb(g->x + 6, g->y + 12, 6, C_GREY);
    gfx_circb(g->x + 38, g->y + 12, 6, C_GREY);
    gfx_rect(g->x + 10, g->y + 22, 24, 4, C_SLATE);
    /* the vise */
    const WthSpot *v = SP(S_SHE_VISE);
    gfx_rect(v->x + 4, v->y + 6, 18, 8, C_SLATE);
    gfx_rect(v->x + 2, v->y + 2, 4, 14, C_GREY);
    gfx_rect(v->x + 18, v->y + 2, 4, 14, C_GREY);
    gfx_hline(v->x, v->x + 25, v->y + 18, C_GREY);
    diary_page(S_SHE_DIARY);
    /* the pegboard */
    gfx_rect(140, 30, 90, 36, C_TAN);
    for (int x = 142; x < 228; x += 5)
        for (int y = 32; y < 64; y += 5) gfx_pset(x, y, C_BROWN);
    if (!wth_flag(F_SCREW_TAKEN)) {
        gfx_rect(150, 36, 4, 8, C_RED);
        gfx_vline(152, 44, 56, C_LIGHT);
    }
    if (!wth_flag(F_SHEARS_TAKEN)) {
        gfx_line(168, 38, 178, 58, C_LIGHT);
        gfx_line(180, 38, 170, 58, C_LIGHT);
        gfx_circb(168, 38, 2, C_FOREST);
    }
    if (!wth_flag(F_SAW_TAKEN)) {
        gfx_rect(192, 40, 8, 6, C_BROWN);
        for (int x = 200; x < 222; x++) gfx_pset(x, 44 - (x - 200) / 6 + ((x & 1) ? 1 : 0), C_LIGHT);
        gfx_line(200, 42, 221, 39, C_LIGHT);
    }
    /* the spade */
    if (!wth_flag(F_SPADE_TAKEN)) {
        const WthSpot *s = SP(S_SHE_SPADE);
        gfx_vline(s->x + 9, s->y, s->y + 50, C_TAN);
        gfx_rect(s->x + 6, s->y, 7, 3, C_TAN);
        gfx_rect(s->x + 3, s->y + 50, 13, 22, C_GREY);
    }
    /* the sack */
    const WthSpot *k = SP(S_SHE_SACK);
    gfx_circ(k->x + k->w / 2, k->y + k->h - 12, 14, C_TAN);
    gfx_rect(k->x + k->w / 2 - 4, k->y, 8, 8, C_TAN);
    gfx_hline(k->x + k->w / 2 - 5, k->x + k->w / 2 + 4, k->y + 7, C_BROWN);
}

static void house_wall(int x0, int x1, int y0) {
    gfx_rect(x0, y0, x1 - x0, 134 - y0, C_DUSK);
    bricks(x0, y0, x1, 100, C_NIGHT);
}

static void room_garden(void) {
    night_sky(60);
    gfx_rect(8, 60, 304, 74, C_FOREST);
    gfx_dither(8, 60, 304, 30, C_INK, 7);
    /* the back of the house on the left, the balcony above */
    house_wall(8, 100, 6);
    gfx_rect(8, 4 + 6, 96, 4, C_SLATE);
    for (int x = 10; x < 100; x += 6) gfx_vline(x, 2 + 6, 9 + 6, C_SLATE);
    /* the scullery door and the glasshouse corner */
    const WthSpot *sd = SP(S_GDN_SCULLERY);
    gfx_rect(sd->x, sd->y, sd->w, sd->h, C_FOREST);
    frame(sd->x, sd->y, sd->w, sd->h, C_INK);
    const WthSpot *g = SP(S_GDN_GLASS);
    gfx_rect(g->x, g->y, g->w, g->h, C_NIGHT);
    for (int x = g->x; x < g->x + g->w; x += 9) gfx_vline(x, g->y, g->y + g->h - 1, C_LIGHT);
    gfx_hline(g->x, g->x + g->w - 1, g->y, C_LIGHT);
    gfx_dither(g->x + 1, g->y + 20, g->w - 2, g->h - 21, C_WHITE, 3);
    /* the hanged man, or the fallen one */
    if (!wth_flag(F_ROPE_CUT)) {
        const WthSpot *h = SP(S_GDN_HANG);
        gfx_vline(h->x + 9, 14, h->y + 12, C_TAN);
        gfx_circ(h->x + 9, h->y + 14, 3, C_LIGHT);
        gfx_rect(h->x + 6, h->y + 18, 7, 14, C_INK);
        gfx_rect(h->x + 7, h->y + 32, 2, 7, C_INK);
        gfx_rect(h->x + 10, h->y + 32, 2, 7, C_INK);
    } else {
        const WthSpot *f = SP(S_GDN_FALLEN);
        gfx_rect(f->x + 6, f->y + 5, f->w - 16, 6, C_INK);
        gfx_circ(f->x + f->w - 8, f->y + 8, 3, C_LIGHT);
        if (!wth_flag(F_AMBER_TAKEN)) gfx_pset(f->x + 4, f->y + 8, C_AMBER);
    }
    /* the laurel */
    const WthSpot *l = SP(S_GDN_LAUREL);
    for (int k = 0; k < 7; k++) gfx_circ(l->x + 6 + (k * 7) % 34, l->y + 8 + (k % 3) * 7, 8, k & 1 ? C_FOREST : C_JADE);
    /* the yew and the grave */
    const WthSpot *y = SP(S_GDN_YEW);
    gfx_rect(y->x + 26, y->y + 40, 10, 34, C_EARTH);
    gfx_circ(y->x + 31, y->y + 24, 26, C_INK);
    gfx_circ(y->x + 20, y->y + 34, 16, C_NIGHT);
    gfx_circ(y->x + 44, y->y + 30, 16, C_NIGHT);
    const WthSpot *gr = SP(S_GDN_GRAVE);
    if (wth_flag(F_GRAVE_DUG)) {
        gfx_rect(gr->x + 6, gr->y + 4, gr->w - 12, 10, C_INK);
        gfx_rect(gr->x, gr->y + 10, 8, 4, C_EARTH);
    } else trap(gr->x + 8, gr->x + gr->w - 8, gr->x, gr->x + gr->w, gr->y + 6, gr->y + 14, C_EARTH);
    gfx_vline(gr->x + gr->w / 2, gr->y - 4, gr->y + 6, C_TAN);
    gfx_hline(gr->x + gr->w / 2 - 3, gr->x + gr->w / 2 + 3, gr->y - 1, C_TAN);
    /* the hives */
    const WthSpot *hv = SP(S_GDN_HIVES);
    for (int k = 0; k < 3; k++) {
        gfx_rect(hv->x + k * 14, hv->y + 2, 12, 14, C_CREAM);
        gfx_rect(hv->x + k * 14 - 1, hv->y, 14, 3, C_LIGHT);
        gfx_hline(hv->x + k * 14 + 3, hv->x + k * 14 + 8, hv->y + 14, C_WHITE);
    }
    /* the far path to the pond */
    const WthSpot *p = SP(S_GDN_POND);
    trap(p->x + 8, p->x + p->w - 8, p->x, p->x + p->w, p->y, p->y + p->h - 1, C_EARTH);
    /* the garden wall and its ivy arch */
    gfx_rect(226, 40, 46, 60, C_SLATE);
    bricks(226, 40, 272, 100, C_DUSK);
    const WthSpot *a = SP(S_GDN_IVY);
    if (wth_flag(F_IVY_CUT)) {
        gfx_rect(a->x + 2, a->y + 6, a->w - 4, a->h - 6, C_INK);
        gfx_circ(a->x + a->w / 2, a->y + 8, a->w / 2 - 2, C_INK);
        gfx_dither(a->x + 2, a->y + 30, a->w - 4, 26, C_SLATE, 5);
    } else
        for (int k = 0; k < 24; k++) gfx_circ(a->x + 4 + (k * 11) % 24, a->y + 4 + (k * 7) % 52, 4, k & 1 ? C_FOREST : C_LEAF);
    /* the shed */
    const WthSpot *s = SP(S_GDN_SHEDDOOR);
    trap(s->x + s->w / 2, s->x + s->w / 2, s->x - 4, s->x + s->w + 4, s->y - 12, s->y, C_EARTH);
    gfx_rect(s->x, s->y, s->w, s->h, C_BROWN);
    for (int x = s->x; x < s->x + s->w; x += 5) gfx_vline(x, s->y, s->y + s->h - 1, C_EARTH);
    if (wth_flag(F_SHED_OPEN)) gfx_rect(s->x + 8, s->y + 10, s->w - 16, s->h - 10, C_INK);
    else gfx_rect(s->x + s->w - 9, s->y + 24, 4, 5, C_GREY);
}

static void room_drive(void) {
    night_sky(40);
    gfx_rect(8, 90, 304, 44, C_GREY);
    gfx_dither(8, 90, 304, 44, C_SLATE, 8);
    /* the front of the house */
    gfx_rect(8, 20, 304, 80, C_DUSK);
    trap(60, 260, 8, 311, 8, 20, C_SLATE);
    bricks(8, 20, 311, 100, C_NIGHT);
    for (int k = 0; k < 4; k++) {
        int x = 86 + k * 34 + (k >= 2 ? 40 : 0);
        if (k == 1 || k == 2) continue;
        gfx_rect(x, 50, 18, 24, C_BROWN);
        gfx_line(x, 52, x + 17, 70, C_EARTH);
    }
    const WthSpot *fd = SP(S_DRV_FRONT);
    door_back(fd->x, fd->y, fd->w, fd->h, C_EARTH, false);
    gfx_rect(fd->x + 2, fd->y + 26, fd->w - 4, 6, C_BROWN);
    /* the garage door */
    const WthSpot *g = SP(S_DRV_GARAGE);
    if (wth_flag(F_GARAGE_OPEN)) {
        gfx_rect(g->x, g->y, g->w, g->h, C_INK);
        gfx_rect(g->x + 12, g->y + 24, g->w - 24, 22, C_FOREST);
    } else {
        gfx_rect(g->x, g->y, g->w, g->h, C_LIGHT);
        for (int y = g->y + 6; y < g->y + g->h; y += 8) gfx_hline(g->x, g->x + g->w - 1, y, C_GREY);
    }
    /* the fountain and its statue */
    const WthSpot *st = SP(S_DRV_STATUE);
    gfx_rect(st->x + 6, st->y + 30, st->w - 12, 14, C_LIGHT);
    gfx_circ(st->x + st->w / 2, st->y + 5, 4, C_WHITE);
    trap(st->x + 10, st->x + 18, st->x + 6, st->x + 22, st->y + 9, st->y + 30, C_WHITE);
    gfx_rect(st->x + 16, st->y + 12, 6, 6, C_LIGHT);
    gfx_circb(st->x + st->w / 2, st->y + 37, 3, wth_flag(F_FOUNTAIN_DRAINED) ? C_AMBER : C_GREY);
    const WthSpot *f = SP(S_DRV_FOUNTAIN);
    trap(f->x + 8, f->x + f->w - 8, f->x, f->x + f->w, f->y, f->y + f->h - 1, C_LIGHT);
    if (!wth_flag(F_FOUNTAIN_DRAINED)) {
        trap(f->x + 12, f->x + f->w - 12, f->x + 6, f->x + f->w - 6, f->y + 2, f->y + 10, C_NAVY);
        gfx_vline(st->x + 22, st->y + 16, f->y + 4, C_SKY);
        if (!wth_flag(F_JADE_TAKEN)) gfx_pset(f->x + 40, f->y + 7, C_JADE);
    } else {
        trap(f->x + 12, f->x + f->w - 12, f->x + 6, f->x + f->w - 6, f->y + 2, f->y + 10, C_EARTH);
        if (!wth_flag(F_JADE_TAKEN)) gfx_rect(f->x + 40, f->y + 6, 3, 2, C_JADE);
    }
    /* the cellar doors */
    const WthSpot *c = SP(S_DRV_STORM);
    if (wth_flag(F_STORM_OPEN)) {
        gfx_rect(c->x, c->y, c->w, c->h, C_INK);
        gfx_rect(c->x - 6, c->y - 6, 6, c->h + 6, C_BROWN);
        gfx_rect(c->x + c->w, c->y - 6, 6, c->h + 6, C_BROWN);
    } else {
        trap(c->x + 4, c->x + c->w - 4, c->x, c->x + c->w, c->y, c->y + c->h - 1, C_BROWN);
        gfx_vline(c->x + c->w / 2, c->y, c->y + c->h - 1, C_EARTH);
        gfx_rect(c->x + c->w / 2 - 3, c->y + 5, 6, 6, C_INK);
    }
    /* the gates, down the drive */
    const WthSpot *ga = SP(S_DRV_GATES);
    for (int x = ga->x + 4; x < ga->x + ga->w; x += 6) gfx_vline(x, ga->y, ga->y + ga->h - 1, C_INK);
    gfx_hline(ga->x, ga->x + ga->w - 1, ga->y + 3, C_INK);
}

static void hare_draw(int i) {
    const WthSpot *h = SP(S_CRT_HARE0 + i);
    gfx_rect(h->x, h->y, h->w, h->h, C_NIGHT);
    gfx_circ(h->x + h->w / 2, h->y + 26, 10, C_LIGHT);
    gfx_circ(h->x + h->w / 2 + 2, h->y + 14, 6, C_LIGHT);
    gfx_rect(h->x + h->w / 2, h->y + 2, 3, 9, C_LIGHT);
    gfx_rect(h->x + h->w / 2 + 4, h->y + 3, 3, 8, C_LIGHT);
    int gem = wth.s.hare[i];
    int col = gem == I_GARNET ? C_RED : gem == I_JADE ? C_JADE : gem == I_AMBER ? C_AMBER : gem == I_OPAL ? C_ICE : C_INK;
    gfx_rect(h->x + h->w / 2 + 3, h->y + 12, 3, 3, col);
    gfx_rect(h->x + 2, h->y + h->h - 4, h->w - 4, 4, C_GREY);
    /* the plaque's pictures above each niche */
    static const uint8_t SIGN[4] = {C_AMBER, C_JADE, C_RED, C_ICE};
    (void)SIGN;
}

static void room_courtyard(void) {
    night_sky(30);
    gfx_rect(8, 30, 304, 74, C_SLATE);
    bricks(8, 30, 311, 104, C_GREY);
    gfx_rect(8, 104, 304, 30, C_GREY);
    for (int y = 106; y < 134; y += 6)
        for (int x = 8 + ((y / 6) & 1) * 6; x < 312; x += 12) gfx_rect(x, y, 10, 4, C_LIGHT);
    for (int i = 0; i < 4; i++) hare_draw(i);
    /* the plaque: four little pictures (sun, leaf, a drop, a moon) */
    const WthSpot *p = SP(S_CRT_PLAQUE);
    gfx_rect(p->x, p->y, p->w, p->h, C_AMBER);
    frame(p->x, p->y, p->w, p->h, C_BROWN);
    for (int k = 0; k < 6; k++) gfx_hline(p->x + 4 + k * 12, p->x + 12 + k * 12, p->y + 7, C_BROWN);
    /* the gate */
    const WthSpot *g = SP(S_CRT_GATE);
    gfx_rect(g->x, g->y, g->w, g->h, C_INK);
    gfx_dither(g->x, g->y, g->w, g->h, C_FOREST, 7);
    if (!wth_flag(F_GATE_OPEN)) {
        for (int x = g->x + 3; x < g->x + g->w; x += 6) gfx_vline(x, g->y, g->y + g->h - 1, C_DUSK);
        gfx_hline(g->x, g->x + g->w - 1, g->y + 20, C_DUSK);
        gfx_hline(g->x, g->x + g->w - 1, g->y + 56, C_DUSK);
    } else
        for (int x = g->x + 3; x < g->x + 14; x += 6) gfx_vline(x, g->y, g->y + g->h - 1, C_DUSK);
}

/* the hedges: each junction is drawn a little differently, so you can tell them apart */
static void room_maze(void) {
    night_sky(24);
    gfx_rect(8, 24, 304, 110, C_FOREST);
    int v = wth.s.maze;
    /* openings left, ahead, right */
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
    /* a stone marker: as many notches as the junction */
    for (int k = 0; k <= v; k++) gfx_rect(96 + k * 5, 106, 3, 8, C_LIGHT);
    gfx_rect(94, 114, 6 + v * 5 + 4, 4, C_GREY);
}

static void room_heart(void) {
    night_sky(40);
    gfx_rect(8, 40, 304, 94, C_FOREST);
    gfx_dither(8, 40, 304, 40, C_INK, 6);
    gfx_circ(160, 120, 90, C_EARTH);
    gfx_rect(8, 126, 304, 8, C_FOREST);
    const WthSpot *g = SP(S_HRT_GIRL);
    int cx = g->x + g->w / 2;
    gfx_rect(cx - 16, g->y + 66, 32, 18, C_GREY);
    trap(cx - 6, cx + 6, cx - 12, cx + 12, g->y + 22, g->y + 66, C_LIGHT);
    gfx_circ(cx, g->y + 14, 8, C_LIGHT);
    gfx_rect(cx - 16, g->y + 34, 32, 5, C_LIGHT);
    if (!wth_flag(F_BOLT_TAKEN)) gfx_line(cx - 12, g->y + 33, cx + 12, g->y + 31, C_TAN);
    gfx_pset(cx - 3, g->y + 13, C_SLATE);
    gfx_pset(cx + 3, g->y + 13, C_SLATE);
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
    /* the stairwell's banister */
    gfx_hline(60, 260, 120, C_BROWN);
    for (int x = 64; x < 260; x += 10) gfx_vline(x, 120, 133, C_BROWN);
}

static void room_master(void) {
    interior(C_VIOLET, C_PURPLE, C_BROWN, C_INK);
    floorboards(C_EARTH);
    door_side(false, C_CREAM, false);
    /* the safe */
    const WthSpot *s = SP(S_MAS_SAFE);
    gfx_rect(s->x, s->y, s->w, s->h, wth_flag(F_SAFE_OPEN) ? C_INK : C_SLATE);
    frame(s->x, s->y, s->w, s->h, C_GREY);
    gfx_circb(s->x + s->w / 2, s->y + s->h / 2, 5, C_LIGHT);
    if (wth_flag(F_SAFE_OPEN)) gfx_rect(s->x + s->w, s->y, 4, s->h, C_GREY);
    /* the bed: one side made, the other a nest */
    const WthSpot *b = SP(S_MAS_BED);
    gfx_rect(b->x - 4, b->y - 30, 4, b->h + 30, C_BROWN);
    gfx_rect(b->x + b->w, b->y - 30, 4, b->h + 30, C_BROWN);
    gfx_rect(b->x - 4, b->y - 32, b->w + 8, 4, C_BROWN);
    gfx_rect(b->x, b->y + 8, b->w, b->h - 12, C_CREAM);
    gfx_rect(b->x + b->w / 2, b->y + 8, b->w / 2, b->h - 12, C_WHITE);
    gfx_dither(b->x + b->w / 2, b->y + 8, b->w / 2, b->h - 12, C_LIGHT, 8);
    for (int k = 0; k < 8; k++) gfx_line(b->x + b->w / 2 + k * 6, b->y + 10, b->x + b->w / 2 + k * 6 + 8, b->y + 30, C_LIGHT);
    page_on(S_MAS_BED, b->x + 16, b->y + 14);
    /* the dresser */
    const WthSpot *d = SP(S_MAS_DRESSER);
    gfx_rect(d->x, d->y + 18, d->w, d->h - 18, C_BROWN);
    gfx_rect(d->x + 8, d->y, d->w - 16, 18, C_EARTH);
    frame(d->x + 4, d->y + 24, d->w - 8, 8, C_EARTH);
    if (!wth_flag(F_LENS_TAKEN)) {
        gfx_circb(d->x + 30, d->y + 15, 3, C_LIGHT);
        gfx_line(d->x + 27, d->y + 18, d->x + 24, d->y + 21, C_TAN);
    }
}

static void room_balcony(void) {
    night_sky(100);
    gfx_rect(8, 70, 304, 30, C_INK);
    gfx_dither(8, 64, 304, 12, C_FOREST, 3);
    gfx_pset(260, 66, (wth.frame / 90) % 3 ? C_AMBER : C_INK);
    gfx_rect(8, 100, 304, 34, C_SLATE);
    gfx_rect(8, 96, 304, 4, C_GREY);
    for (int x = 12; x < 312; x += 10) gfx_rect(x, 100, 3, 24, C_GREY);
    const WthSpot *r = SP(S_BAL_ROPE);
    if (!wth_flag(F_ROPE_CUT)) {
        gfx_circ(r->x + 12, 98, 4, C_TAN);
        gfx_line(r->x + 12, 98, r->x + 14, 133, C_TAN);
        gfx_line(r->x + 13, 98, r->x + 15, 133, C_TAN);
    } else gfx_line(r->x + 12, 98, r->x + 8, 106, C_TAN);
}

static void room_study(void) {
    interior(C_FOREST, C_JADE, C_BROWN, C_INK);
    floorboards(C_EARTH);
    /* the desk with the computer */
    gfx_rect(110, 90, 110, 6, C_TAN);
    gfx_rect(112, 96, 6, 30, C_BROWN);
    gfx_rect(212, 96, 6, 30, C_BROWN);
    const WthSpot *c = SP(S_STU_COMPUTER);
    gfx_rect(c->x + 6, c->y, c->w - 12, 32, C_CREAM);
    gfx_rect(c->x + 10, c->y + 4, c->w - 20, 22, C_INK);
    if (wth_flag(F_RECIPE)) for (int k = 0; k < 4; k++) gfx_hline(c->x + 12, c->x + 20 + k * 5, c->y + 7 + k * 4, C_LIME);
    else gfx_rect(c->x + 12, c->y + 7, 4 + ((wth.frame / 30) & 1) * 3, 2, C_LIME);
    gfx_rect(c->x, c->y + 34, c->w, 6, C_CREAM);
    diary_page(S_STU_DIARY);
    /* the case and its pike */
    const WthSpot *k = SP(S_STU_CASE);
    gfx_rect(k->x, k->y + k->h - 4, k->w, 4, C_BROWN);
    gfx_rect(k->x + 2, k->y, k->w - 4, k->h - 4, C_NIGHT);
    gfx_rect(k->x + 6, k->y + 14, k->w - 14, 8, C_FOREST);
    gfx_rect(k->x + k->w - 10, k->y + 12, 6, 12, C_FOREST);
    gfx_pset(k->x + 9, k->y + 16, C_YELLOW);
    if (!wth_flag(F_RING_TAKEN)) gfx_circb(k->x + 6, k->y + 20, 2, C_AMBER);
    if (wth_flag(F_CASE_BROKEN)) {
        gfx_line(k->x + 4, k->y + 2, k->x + 16, k->y + 12, C_WHITE);
        gfx_line(k->x + 30, k->y + 4, k->x + 20, k->y + 20, C_WHITE);
    } else frame(k->x + 2, k->y, k->w - 4, k->h - 4, C_ICE);
    /* the books */
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
    bathtub(S_BAT_TUB, false);
    /* the one in the bath */
    const WthSpot *t = SP(S_BAT_TUB);
    gfx_circ(t->x + 24, t->y + 2, 7, C_GREY);
    gfx_circ(t->x + 24, t->y + 4, 3, C_INK);
    for (int k = 0; k < 6; k++) gfx_line(t->x + 24, t->y + 2, t->x + 14 + k * 4, t->y - 14 - (k & 1) * 6, C_WHITE);
    gfx_rect(t->x + 30, t->y + 4, 50, 5, C_GREY);
    /* the mirror */
    const WthSpot *m = SP(S_BAT_MIRROR);
    gfx_rect(m->x, m->y, m->w, m->h, wth_flag(F_MIRROR_BROKEN) ? C_SLATE : C_ICE);
    frame(m->x, m->y, m->w, m->h, C_AMBER);
    if (wth_flag(F_MIRROR_BROKEN)) {
        gfx_line(m->x + 2, m->y + 4, m->x + 20, m->y + 28, C_WHITE);
        gfx_line(m->x + 26, m->y + 2, m->x + 10, m->y + 18, C_WHITE);
    } else {
        gfx_dither(m->x + 1, m->y + 1, m->w - 2, m->h - 2, C_WHITE, 3);
        gfx_circ(m->x + 15, m->y + 15, 5, C_TAN);
    }
    /* the shelf */
    const WthSpot *s = SP(S_BAT_SHELF);
    gfx_rect(s->x, s->y + s->h - 3, s->w, 3, C_WHITE);
    for (int k = 0; k < 3; k++) gfx_rect(s->x + 2 + k * 8, s->y + 6, 5, s->h - 9, k == 1 && !wth_flag(F_CLOVE_TAKEN) ? C_BROWN : C_LIGHT);
    toilet(S_BAT_TOILET);
    diary_page(S_BAT_DIARY);
}

static void room_nursery(void) {
    interior(C_CREAM, C_TAN, C_BROWN, C_LIGHT);
    for (int x = 80; x < 248; x += 12) gfx_circb(x, 40, 3, C_PINK);
    floorboards(C_EARTH);
    /* the scribble */
    const WthSpot *w = SP(S_NUR_WALL);
    for (int k = 0; k < 30; k++) {
        int x = w->x + 4 + (k * 23) % (w->w - 10), y = w->y + 4 + (k * 13) % (w->h - 10);
        gfx_circb(x, y, 3 + k % 4, C_RED);
    }
    /* the music box on a little table */
    const WthSpot *b = SP(S_NUR_BOX);
    gfx_rect(b->x - 4, b->y + b->h, b->w + 8, 3, C_BROWN);
    gfx_rect(b->x, b->y + b->h + 3, 3, 16, C_BROWN);
    gfx_rect(b->x + b->w - 3, b->y + b->h + 3, 3, 16, C_BROWN);
    gfx_rect(b->x + 2, b->y + 6, b->w - 4, b->h - 6, C_PINK);
    if (wth_flag(F_BOXMUSIC)) gfx_rect(b->x + 2, b->y - 4, b->w - 4, 6, C_PINK);
    gfx_rect(b->x + b->w / 2 - 1, b->y, 2, 6, C_WHITE);
    /* the toy chest */
    const WthSpot *c = SP(S_NUR_CHEST);
    if (wth_flag(F_HIDE_CHEST_GONE)) {
        gfx_rect(c->x, c->y + 12, c->w, c->h - 12, C_BROWN);
        for (int k = 0; k < 5; k++) gfx_line(c->x + k * 12, c->y + 12, c->x + k * 12 + 8, c->y + c->h - 1, C_INK);
        gfx_circ(c->x + c->w + 6, c->y + c->h - 4, 3, C_TAN);
    } else {
        gfx_rect(c->x, c->y + 6, c->w, c->h - 6, C_SKY);
        gfx_rect(c->x - 2, c->y, c->w + 4, 7, C_BLUE);
        gfx_rect(c->x + c->w / 2 - 3, c->y + 6, 6, 5, C_AMBER);
    }
    /* the rocking horse */
    const WthSpot *h = SP(S_NUR_HORSE);
    int rock = (wth.frame / 20) % 4 == 1 ? 1 : (wth.frame / 20) % 4 == 3 ? -1 : 0;
    gfx_line(h->x, h->y + h->h - 2 + rock, h->x + h->w, h->y + h->h - 2 - rock, C_BROWN);
    gfx_rect(h->x + 12, h->y + 18, 28, 12, C_WHITE);
    gfx_rect(h->x + 34, h->y + 6, 10, 14, C_WHITE);
    gfx_rect(h->x + 14, h->y + 30, 3, 12, C_WHITE);
    gfx_rect(h->x + 34, h->y + 30, 3, 12, C_WHITE);
    gfx_pset(h->x + 40, h->y + 9, C_INK);
    diary_page(S_NUR_DIARY);
}

static void room_children(void) {
    interior(C_SKY, C_BLUE, C_BROWN, C_ICE);
    floorboards(C_EARTH);
    diary_page(S_CHI_DIARY);
    /* the dollhouse */
    const WthSpot *d = SP(S_CHI_HOUSE);
    gfx_rect(d->x, d->y + 14, d->w, d->h - 14, C_DUSK);
    trap(d->x + d->w / 2, d->x + d->w / 2, d->x - 4, d->x + d->w + 4, d->y, d->y + 14, C_SLATE);
    for (int r = 0; r < 2; r++)
        for (int k = 0; k < 3; k++) gfx_rect(d->x + 4 + k * 23, d->y + 18 + r * 16, 18, 13, (r + k) & 1 ? C_WINE : C_FOREST);
    gfx_rect(d->x + 50, d->y + 22, 4, 2, C_CREAM);
    /* the doll */
    if (!wth_flag(F_DOLL_TAKEN)) {
        const WthSpot *o = SP(S_CHI_DOLL);
        gfx_circ(o->x + 11, o->y + 6, 5, C_CREAM);
        gfx_rect(o->x + 6, o->y + 11, 10, 9, C_RED);
        gfx_rect(o->x + 3, o->y + 12, 3, 6, C_CREAM);
        gfx_rect(o->x + 16, o->y + 12, 3, 6, C_CREAM);
        gfx_pset(o->x + 9, o->y + 5, C_INK);
        gfx_pset(o->x + 13, o->y + 5, C_INK);
        for (int y = o->y + 1; y < o->y + 6; y++) gfx_pset(o->x + 7 + (y & 1), y, C_AMBER);
    }
    /* the bunks */
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
    gfx_rect(146, 40, 28, 20, C_NIGHT);
    gfx_pset(152, 46, C_LIGHT);
    /* the trunk */
    const WthSpot *t = SP(S_ATT_TRUNK);
    gfx_rect(t->x, t->y + 6, t->w, t->h - 6, C_NAVY);
    if (wth_flag(F_TRUNK_OPEN)) gfx_rect(t->x, t->y - 6, t->w, 8, C_NAVY);
    else gfx_rect(t->x - 1, t->y, t->w + 2, 8, C_BLUE);
    gfx_hline(t->x, t->x + t->w - 1, t->y + 16, C_AMBER);
    gfx_rect(t->x + t->w / 2 - 3, t->y + 6, 6, 6, C_GREY);
    /* the organ */
    const WthSpot *o = SP(S_ATT_HARM);
    gfx_rect(o->x, o->y, o->w, o->h, C_EARTH);
    gfx_rect(o->x + 4, o->y + 18, o->w - 8, 6, C_WHITE);
    for (int x = o->x + 6; x < o->x + o->w - 6; x += 4) gfx_rect(x, o->y + 18, 2, 3, C_INK);
    for (int k = 0; k < 6; k++) gfx_rect(o->x + 6 + k * 9, o->y + 4, 6, 10, C_TAN);
    if (!wth_flag(F_HARMONIUM_PLAYED)) gfx_dither(o->x, o->y, o->w, 16, C_LIGHT, 4);
    /* the dummies */
    const WthSpot *d = SP(S_ATT_DUMMIES);
    for (int k = 0; k < 3; k++) {
        int x = d->x + 8 + k * 20;
        gfx_vline(x + 6, d->y + 40, d->y + d->h - 1, C_INK);
        trap(x + 2, x + 10, x - 4, x + 16, d->y + 6 + (k & 1) * 4, d->y + 52, C_LIGHT);
        gfx_circ(x + 6, d->y + 4 + (k & 1) * 4, 4, C_LIGHT);
    }
    diary_page(S_ATT_DIARY);
}

static void room_cellar(void) {
    interior(C_DUSK, C_NIGHT, C_EARTH, C_INK);
    bricks(73, 27, 247, 99, C_NIGHT);
    for (int k = 0; k < 6; k++) gfx_dither(76 + k * 28, 26, 10, 20 + k * 3 % 7, C_WHITE, 3);
    /* the iron door */
    const WthSpot *d = SP(S_CEL_DOOR);
    if (wth_flag(F_IRONDOOR_OPEN)) {
        gfx_rect(d->x, d->y, d->w, d->h, C_INK);
        gfx_dither(d->x, d->y, d->w, d->h, C_WHITE, 2);
        gfx_rect(d->x, d->y, d->w, 4, C_GREY);
    } else {
        gfx_rect(d->x, d->y, d->w, d->h, C_SLATE);
        for (int y = d->y + 4; y < d->y + d->h; y += 10) gfx_hline(d->x, d->x + d->w - 1, y, C_GREY);
        for (int x = d->x + 4; x < d->x + d->w; x += 10) gfx_pset(x, d->y + 2, C_LIGHT);
    }
    /* the winch */
    const WthSpot *m = SP(S_CEL_MECH);
    gfx_rect(m->x, m->y, m->w, m->h, C_SLATE);
    gfx_circb(m->x + m->w / 2, m->y + 14, 8, wth_flag(F_MECH_GEAR) ? C_AMBER : C_INK);
    if (wth_flag(F_MECH_WIRE)) gfx_line(m->x + m->w / 2, m->y + 22, m->x + m->w + 12, d->y + 2, C_LIGHT);
    if (!wth_flag(F_MECH_SCREW)) gfx_rect(m->x - 6, m->y + 26, 10, 12, C_GREY);
    if (!wth_flag(F_MECH_OIL)) gfx_dither(m->x, m->y, m->w, m->h, C_ORANGE, 4);
    /* the shelves */
    const WthSpot *s = SP(S_CEL_SHELF);
    for (int r = 0; r < 3; r++) {
        gfx_rect(s->x, s->y + 16 + r * 20, s->w, 3, C_BROWN);
        for (int k = 0; k < 9; k++) gfx_rect(s->x + 2 + k * 7, s->y + 6 + r * 20, 5, 10, r == 2 ? C_WHITE : C_AMBER);
    }
    diary_page(S_CEL_DIARY);
    /* the steps up */
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
    /* things lying on the floor */
    for (int i = 0; i < WTH_FLOOR; i++) {
        int it = wth.s.floor[room][i];
        if (it) wth_draw_item(it, 70 + i * 60 + 2, 112);
    }
    /* the Keeper where he fell */
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

void wth_draw_item(int it, int x, int y) {
    switch (it) {
    case I_SHOEHORN: gfx_rect(x + 5, x * 0 + y + 1, 3, 10, C_LIGHT); gfx_rect(x + 4, y + 9, 5, 2, C_LIGHT); gfx_pset(x + 6, y + 2, C_INK); break;
    case I_MUG: case I_BREW:
        gfx_rect(x + 2, y + 3, 7, 8, C_LIGHT); gfx_circb(x + 9, y + 6, 2, C_LIGHT);
        if (it == I_BREW) { gfx_rect(x + 3, y + 3, 5, 2, C_BROWN); gfx_pset(x + 4, y + 1, C_GREY); gfx_pset(x + 6, y, C_GREY); }
        else if (wth.s.mug) gfx_rect(x + 3, y + 4, 5, 2, C_EARTH);
        break;
    case I_WIRE: gfx_line(x + 1, y + 10, x + 9, y + 2, C_LIGHT); gfx_line(x + 9, y + 2, x + 10, y + 4, C_LIGHT); break;
    case I_NEWSPAPER: gfx_rect(x + 1, y + 2, 10, 8, C_CREAM); for (int k = 0; k < 3; k++) gfx_hline(x + 2, x + 9, y + 4 + k * 2, C_GREY); break;
    case I_DIARY: gfx_rect(x + 2, y + 1, 8, 10, C_CREAM); gfx_rect(x + 3, y + 2, 8, 10, C_CREAM); for (int k = 0; k < 4; k++) gfx_hline(x + 4, x + 9, y + 4 + k * 2, C_TAN); break;
    case I_PENNIES: gfx_circ(x + 4, y + 7, 3, C_ORANGE); gfx_circ(x + 8, y + 5, 3, C_AMBER); break;
    case I_MATCHES: gfx_rect(x + 1, y + 4, 10, 6, C_YELLOW); gfx_hline(x + 1, x + 10, y + 6, C_RED); break;
    case I_KNIFE: gfx_rect(x + 1, y + 8, 4, 2, C_BROWN); gfx_line(x + 5, y + 8, x + 11, y + 2, C_LIGHT); gfx_line(x + 5, y + 9, x + 11, y + 3, C_WHITE); break;
    case I_SODA: gfx_rect(x + 3, y + 1, 7, 10, C_RED); gfx_rect(x + 3, y + 4, 7, 3, C_CREAM); break;
    case I_TONGS: gfx_line(x + 2, y + 1, x + 5, y + 11, C_LIGHT); gfx_line(x + 9, y + 1, x + 6, y + 11, C_LIGHT); gfx_hline(x + 2, x + 9, y + 1, C_LIGHT); break;
    case I_TINKEY: case I_COPPERKEY: case I_BRASSKEY: case I_PEWTERKEY: case I_SILVERKEY: case I_SHEDKEY: case I_KEEPERKEY: {
        int c = it == I_TINKEY ? C_GREY : it == I_COPPERKEY ? C_ORANGE : it == I_BRASSKEY ? C_YELLOW
              : it == I_PEWTERKEY ? C_SLATE : it == I_SILVERKEY ? C_WHITE : it == I_SHEDKEY ? C_TAN : C_INK;
        if (it == I_KEEPERKEY) { gfx_rect(x, y, 12, 12, C_DUSK); }
        gfx_circb(x + 3, y + 4, 2, c);
        gfx_hline(x + 5, x + 11, y + 4, c);
        gfx_vline(x + 9, y + 4, y + 7, c);
        gfx_vline(x + 11, y + 4, y + 6, c);
        if (it == I_SHEDKEY) gfx_rect(x + 1, y + 8, 5, 3, C_BROWN);
        if (it == I_COPPERKEY) gfx_rect(x + 2, y + 3, 3, 3, c);
        break;
    }
    case I_CUPHOOK: gfx_vline(x + 6, y + 1, y + 7, C_AMBER); gfx_circb(x + 4, y + 8, 2, C_AMBER); break;
    case I_GEAR: gfx_circ(x + 6, y + 6, 4, C_AMBER); for (int k = 0; k < 4; k++) gfx_rect(x + 5 + (k == 0 ? -5 : k == 1 ? 5 : 0), y + 5 + (k == 2 ? -5 : k == 3 ? 5 : 0), 2, 2, C_AMBER); gfx_rect(x + 5, y + 5, 2, 2, C_INK); break;
    case I_TWINE: gfx_circ(x + 6, y + 6, 4, C_BROWN); gfx_line(x + 3, y + 4, x + 9, y + 8, C_EARTH); gfx_line(x + 10, y + 6, x + 11, y + 11, C_BROWN); break;
    case I_OIL: gfx_rect(x + 2, y + 5, 6, 6, C_GREY); gfx_line(x + 7, y + 5, x + 11, y + 1, C_GREY); break;
    case I_TORCH: gfx_rect(x + 1, y + 5, 7, 3, C_RED); gfx_rect(x + 8, y + 3, 3, 7, C_LIGHT); if (wth_flag(F_TORCH_LOADED)) gfx_pset(x + 11, y + 6, C_YELLOW); break;
    case I_PARAFFIN: gfx_rect(x + 2, y + 3, 8, 8, C_RED); gfx_rect(x + 7, y + 1, 2, 2, C_GREY); break;
    case I_BAR: gfx_line(x + 1, y + 11, x + 10, y + 2, C_SLATE); gfx_line(x + 2, y + 11, x + 11, y + 2, C_GREY); gfx_line(x + 10, y + 2, x + 8, y, C_GREY); break;
    case I_BATTERIES: gfx_rect(x + 2, y + 2, 3, 9, C_ORANGE); gfx_rect(x + 7, y + 2, 3, 9, C_ORANGE); gfx_pset(x + 3, y + 1, C_LIGHT); gfx_pset(x + 8, y + 1, C_LIGHT); break;
    case I_CANE: gfx_line(x + 1, y + 11, x + 10, y + 1, C_TAN); break;
    case I_LONGHOOK: gfx_line(x + 1, y + 11, x + 9, y + 3, C_TAN); gfx_circb(x + 9, y + 2, 1, C_AMBER); gfx_rect(x + 6, y + 5, 2, 2, C_BROWN); break;
    case I_HYMN: gfx_rect(x + 2, y + 1, 8, 10, C_WHITE); for (int k = 0; k < 4; k++) gfx_hline(x + 2, x + 9, y + 3 + k * 2, C_GREY); gfx_pset(x + 5, y + 5, C_INK); gfx_pset(x + 7, y + 7, C_INK); break;
    case I_SPANNER: gfx_line(x + 2, y + 10, x + 9, y + 3, C_LIGHT); gfx_circb(x + 9, y + 2, 2, C_LIGHT); break;
    case I_SCREWDRIVER: gfx_rect(x + 1, y + 8, 4, 3, C_RED); gfx_line(x + 5, y + 8, x + 11, y + 2, C_LIGHT); break;
    case I_SHEARS: gfx_line(x + 2, y + 10, x + 10, y + 1, C_LIGHT); gfx_line(x + 10, y + 10, x + 2, y + 1, C_LIGHT); gfx_rect(x + 1, y + 9, 3, 3, C_FOREST); gfx_rect(x + 9, y + 9, 3, 3, C_FOREST); break;
    case I_SPADE: gfx_vline(x + 6, y, y + 7, C_TAN); gfx_rect(x + 3, y + 7, 7, 5, C_GREY); break;
    case I_MOTOR: gfx_rect(x + 1, y + 3, 8, 7, C_RED); gfx_rect(x + 9, y + 5, 3, 2, C_LIGHT); gfx_hline(x + 1, x + 8, y + 6, C_MAROON); break;
    case I_CARKEYS: gfx_circb(x + 3, y + 3, 2, C_LIGHT); gfx_line(x + 5, y + 5, x + 10, y + 10, C_LIGHT); gfx_rect(x + 1, y + 7, 4, 4, C_BROWN); break;
    case I_PIG: gfx_circ(x + 6, y + 6, 4, C_PINK); gfx_rect(x + 9, y + 5, 2, 2, C_PINK); gfx_pset(x + 8, y + 4, C_INK); break;
    case I_SAW: gfx_rect(x + 1, y + 6, 3, 4, C_BROWN); gfx_line(x + 4, y + 7, x + 11, y + 4, C_LIGHT); gfx_line(x + 4, y + 9, x + 11, y + 6, C_LIGHT); break;
    case I_GARNET: case I_JADE: case I_AMBER: case I_OPAL: {
        int c = it == I_GARNET ? C_RED : it == I_JADE ? C_JADE : it == I_AMBER ? C_AMBER : C_ICE;
        gfx_circ(x + 6, y + 6, 4, c);
        gfx_pset(x + 4, y + 4, C_WHITE);
        if (it == I_AMBER) gfx_pset(x + 7, y + 7, C_INK);
        if (it == I_OPAL) { gfx_pset(x + 7, y + 6, C_PINK); gfx_pset(x + 5, y + 8, C_LIME); }
        break;
    }
    case I_RING: gfx_circb(x + 6, y + 7, 3, C_YELLOW); gfx_rect(x + 4, y + 2, 5, 3, C_YELLOW); break;
    case I_SNUFF: gfx_rect(x + 2, y + 4, 8, 6, C_INK); gfx_rect(x + 2, y + 3, 8, 2, C_GREY); break;
    case I_CLOVE: gfx_rect(x + 4, y + 3, 5, 8, C_BROWN); gfx_rect(x + 5, y + 1, 3, 2, C_INK); break;
    case I_BORAX: gfx_rect(x + 2, y + 2, 8, 9, C_WHITE); gfx_rect(x + 2, y + 5, 8, 3, C_BLUE); break;
    case I_LENS: gfx_circb(x + 5, y + 5, 3, C_LIGHT); gfx_pset(x + 4, y + 4, C_ICE); gfx_line(x + 7, y + 7, x + 10, y + 10, C_TAN); break;
    case I_COMBO: gfx_rect(x + 1, y + 3, 10, 7, C_WHITE); gfx_hline(x + 2, x + 9, y + 5, C_INK); gfx_hline(x + 2, x + 6, y + 7, C_INK); break;
    case I_DOLL: gfx_circ(x + 6, y + 3, 2, C_CREAM); gfx_rect(x + 4, y + 5, 5, 6, C_RED); gfx_rect(x + 2, y + 6, 2, 3, C_CREAM); gfx_rect(x + 9, y + 6, 2, 3, C_CREAM); break;
    case I_PASSWORD: gfx_rect(x + 3, y + 3, 6, 6, C_CREAM); gfx_pset(x + 5, y + 5, C_INK); gfx_pset(x + 6, y + 6, C_INK); break;
    case I_GLASSES: gfx_circb(x + 3, y + 6, 2, C_INK); gfx_circb(x + 8, y + 6, 2, C_INK); gfx_pset(x + 3, y + 6, C_RED); gfx_pset(x + 8, y + 6, C_RED); gfx_hline(x + 5, x + 6, y + 6, C_INK); gfx_circ(x + 3, y + 6, 1, C_RED); gfx_circ(x + 8, y + 6, 1, C_RED); break;
    case I_DIRECTIONS: gfx_rect(x + 1, y + 2, 10, 8, C_CREAM); gfx_line(x + 3, y + 8, x + 5, y + 4, C_RED); gfx_line(x + 5, y + 4, x + 9, y + 6, C_RED); break;
    case I_CROSSBOW: gfx_line(x + 1, y + 6, x + 11, y + 6, C_BROWN); gfx_line(x + 8, y + 1, x + 10, y + 11, C_EARTH); if (wth_flag(F_BOW_LOADED)) gfx_line(x + 2, y + 5, x + 11, y + 5, C_LIGHT); break;
    case I_STEELWIRE: gfx_circb(x + 6, y + 6, 4, C_LIGHT); gfx_circb(x + 6, y + 6, 2, C_LIGHT); break;
    case I_BOLT: gfx_line(x + 1, y + 10, x + 10, y + 1, C_TAN); gfx_rect(x + 9, y + 1, 2, 2, C_GREY); gfx_rect(x + 1, y + 9, 2, 2, C_RED); break;
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
    "YOU THROW THE BREW INTO THE FACE. IT HISSES LIKE FAT IN A PAN. THE WHITE THREADS CURL, AND BROWN, AND GO GREY, "
    "AND THE FACE OPENS ITS MOUTH WIDE WITHOUT A SOUND.",
    "ALL THROUGH THE HOUSE ABOVE YOU SOMETHING LETS GO. THE HONEY STOPS WEEPING OUT OF THE WALLS. FAR UNDER THE FEN "
    "A VERY BIG THING DIES, SLOWLY, AND THE WELL GOES QUIET.",
    "YOU CLIMB OUT INTO A GREY DAWN AND WALK DOWN THE LANE. YOUR OWN CAR IS STILL NOSE DOWN IN THE DYKE. IT STARTS "
    "FIRST TIME. BEHIND YOU, WITHERLEA IS ONLY A HOUSE.",
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

static void draw_end_picture(int page) {
    int t = (int)wth.frame;
    if (wth.ending == END_CAR || page == 2) {
        if (page == 0 && wth.ending == END_CAR) {
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
    gfx_cls(C_INK);
    room_spore();
    if (page == 1) gfx_dither(WTH_PX, WTH_PY, WTH_PW, WTH_PH, C_SLATE, 10);
    else gfx_dither(WTH_PX, WTH_PY, WTH_PW, WTH_PH, C_BROWN, imin(12, wth.scene_t / 8));
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
    "AND SORREL THE DOG",
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
