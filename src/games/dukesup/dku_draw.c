/* DUKES UP - drawing: the streets and rooms of Lampwick, the floor, the
 * hazards, everyone on the floor in depth order, the HUD, and every screen
 * from the title to the credits. */
#include "dku.h"

static void put(const char *what, const char *s, int x, int y, int col) {
    ui_audit_text(what, s, x, y);
    text_shadow(s, x, y, col, C_INK);
}

static void putc_(const char *what, const char *s, int cx, int y, int col) {
    int w = text_width(s);
    put(what, s, cx - w / 2, y, col);
}

static void tput(const char *what, const char *s, int x, int y, int col) {
    ui_audit_tiny(what, s, x, y);
    tiny_draw(s, x, y, col);
}

static int wrapped(const char *what, const char *s, int x, int y, int w, int col) {
    char lines[8][UI_WRAP_LEN];
    int n = ui_wrap(s, w, false, lines, 8);
    if (n > 8) ui_audit_fail(what, "too many lines");
    for (int i = 0; i < n && i < 8; i++) put(what, lines[i], x, y + i * LINE_H, col);
    return imin(n, 8);
}

static int wrapped_c(const char *what, const char *s, int cx, int y, int w, int col) {
    char lines[8][UI_WRAP_LEN];
    int n = ui_wrap(s, w, false, lines, 8);
    if (n > 8) ui_audit_fail(what, "too many lines");
    for (int i = 0; i < n && i < 8; i++) putc_(what, lines[i], cx, y + i * LINE_H, col);
    return imin(n, 8);
}

/* ---- backdrops: x is a world position, drawn at x - cam ---------------------- */

static int wrapx(int wx, int period) {
    int m = wx % period;
    return m < 0 ? m + period : m;
}

static void sky(int top, int bot, int c1, int c2) {
    gfx_rect(0, top, SCREEN_W, bot - top, c1);
    gfx_dither(0, bot - 18, SCREEN_W, 18, c2, 6);
}

static void stars(int cam, int top, int bot) {
    for (int k = 0; k < 24; k++) {
        int x = wrapx(k * 53 - cam / 4, SCREEN_W);
        int y = top + (k * 37) % (bot - top);
        gfx_pset(x, y, (k + dku_g.frame_t / 30) % 5 ? C_GREY : C_WHITE);
    }
}

static void moon(int x, int y) {
    gfx_circ(x, y, 9, C_CREAM);
    gfx_circ(x + 3, y - 2, 8, C_NIGHT);
}

static void bg_street(int cam, bool canal) {
    sky(DKU_TOP, 60, C_NIGHT, C_DUSK);
    stars(cam, DKU_TOP, 56);
    moon(260, 36);
    /* shopfronts, 96 px apart */
    for (int bx = -(wrapx(cam, 96)); bx < SCREEN_W; bx += 96) {
        int wid = (bx + cam) / 96;
        int h = 46 + (wid * 13) % 22;
        int col = (wid % 3) == 0 ? C_MAROON : (wid % 3) == 1 ? C_BROWN : C_DUSK;
        gfx_rect(bx, 108 - h, 92, h, col);
        gfx_rect(bx, 108 - h, 92, 2, PAL_DARKER[col]);
        for (int wy = 108 - h + 6; wy < 80; wy += 12)
            for (int wx = bx + 8; wx < bx + 84; wx += 16)
                gfx_rect(wx, wy, 8, 7, ((wx + wy + wid) % 5) == 0 ? C_YELLOW : C_NIGHT);
        if (!canal) {
            /* an awning and a shop window */
            for (int k = 0; k < 9; k++) gfx_rect(bx + 6 + k * 9, 82, 9, 5, (k & 1) ? C_RED : C_WHITE);
            gfx_rect(bx + 10, 88, 72, 18, C_INK);
            gfx_rect(bx + 12, 90, 68, 14, (wid & 1) ? C_NAVY : C_DUSK);
            gfx_rect(bx + 40, 92, 12, 16, C_SLATE);
        }
    }
    if (canal) {
        /* the canal behind the railing */
        gfx_rect(0, 86, SCREEN_W, 22, C_NAVY);
        for (int k = 0; k < 12; k++) gfx_hline(wrapx(k * 41 - cam - dku_g.frame_t / 3, SCREEN_W), wrapx(k * 41 - cam - dku_g.frame_t / 3, SCREEN_W) + 8, 92 + (k * 5) % 14, C_BLUE);
        for (int px = -wrapx(cam, 16); px < SCREEN_W; px += 16) gfx_vline(px, 98, 108, C_GREY);
        gfx_hline(0, SCREEN_W, 98, C_LIGHT);
    }
    /* street lamps */
    for (int lx = -wrapx(cam, 160) + 70; lx < SCREEN_W + 10; lx += 160) {
        gfx_rect(lx, 70, 2, 38, C_SLATE);
        gfx_rect(lx - 3, 68, 8, 3, C_GREY);
        gfx_dither(lx - 10, 71, 22, 8, C_YELLOW, 4);
    }
    /* pavement and road */
    gfx_rect(0, 108, SCREEN_W, 14, canal ? C_SLATE : C_GREY);
    for (int px = -wrapx(cam, 20); px < SCREEN_W; px += 20) gfx_vline(px, 108, 121, C_SLATE);
    gfx_rect(0, 122, SCREEN_W, 2, C_LIGHT);
    gfx_rect(0, 124, SCREEN_W, 56, canal ? C_DUSK : C_SLATE);
    if (canal) {
        for (int y = 126; y < 180; y += 6)
            for (int x = -wrapx(cam + (y / 6) * 5, 12); x < SCREEN_W; x += 12) gfx_rect(x, y, 10, 4, C_SLATE);
    } else {
        for (int lx = -wrapx(cam, 40); lx < SCREEN_W; lx += 40) gfx_rect(lx, 150, 20, 2, C_YELLOW);
    }
}

static void bg_club(int cam) {
    gfx_rect(0, DKU_TOP, SCREEN_W, 86, C_PURPLE);
    for (int k = 0; k < 8; k++) {
        int x = 20 + k * 40, c = (k + dku_g.frame_t / 12) % 3 == 0 ? C_PINK : (k + dku_g.frame_t / 12) % 3 == 1 ? C_CYAN : C_YELLOW;
        gfx_dither(x - 12, 40, 24, 60, c, 3);
    }
    gfx_circ(160, 34, 8, C_LIGHT);
    for (int k = 0; k < 6; k++) gfx_pset(156 + (k * 3) % 9, 30 + (k * 5) % 9, C_WHITE);
    gfx_rect(0, 100, SCREEN_W, 8, C_INK);
    put("club sign", "ROLLER RINK", 120, 70, C_PINK);
    for (int y = 108; y < 180; y += 8)
        for (int x = -wrapx(cam, 16) + ((y / 8) & 1) * 8; x < SCREEN_W; x += 16) gfx_rect(x, y, 8, 8, C_BROWN);
    gfx_dither(0, 108, SCREEN_W, 72, C_TAN, 4);
}

static void bg_ferry(int cam) {
    gfx_rect(0, DKU_TOP, SCREEN_W, 86, C_TEAL);
    for (int wx = -wrapx(cam, 64); wx < SCREEN_W; wx += 64) {
        gfx_rect(wx + 8, 40, 40, 26, C_INK);
        gfx_rect(wx + 10, 42, 36, 22, C_NIGHT);
        for (int k = 0; k < 3; k++) gfx_hline(wx + 12 + ((k * 13 + dku_g.frame_t) % 30), wx + 18 + ((k * 13 + dku_g.frame_t) % 30), 52 + k * 4, C_BLUE);
        gfx_pset(wx + 30, 46, C_YELLOW);
    }
    gfx_rect(0, 76, SCREEN_W, 4, C_FOREST);
    /* benches along the back */
    for (int bx = -wrapx(cam, 120) + 20; bx < SCREEN_W; bx += 120) {
        gfx_rect(bx, 92, 70, 6, C_BROWN);
        gfx_rect(bx, 98, 70, 3, C_EARTH);
        gfx_rect(bx + 4, 101, 3, 7, C_INK);
        gfx_rect(bx + 63, 101, 3, 7, C_INK);
    }
    gfx_rect(0, 108, SCREEN_W, 72, C_TAN);
    for (int y = 112; y < 180; y += 7) gfx_hline(0, SCREEN_W, y, C_BROWN);
    for (int x = -wrapx(cam, 48); x < SCREEN_W; x += 48) gfx_vline(x, 108, 180, C_BROWN);
}

static void bg_waxworks(void) {
    gfx_rect(0, DKU_TOP, SCREEN_W, 86, C_WINE);
    for (int k = 0; k < 5; k++) {
        int x = 24 + k * 64;
        gfx_rect(x - 10, 50, 20, 52, C_MAROON);
        gfx_rect(x - 6, 66, 12, 26, C_CREAM);
        gfx_rect(x - 4, 58, 8, 8, C_TAN);
        gfx_rect(x - 14, 96, 28, 6, C_GREY);
    }
    put("wax sign", "WAXWORKS", 128, 30, C_YELLOW);
    gfx_rect(0, 108, SCREEN_W, 72, C_RED);
    gfx_rect(0, 108, SCREEN_W, 4, C_MAROON);
    gfx_dither(0, 112, SCREEN_W, 68, C_MAROON, 4);
}

static void bg_orchard(int cam) {
    sky(DKU_TOP, 108, C_INK, C_NIGHT);
    stars(cam, DKU_TOP, 50);
    for (int tx = -wrapx(cam / 2, 70); tx < SCREEN_W + 30; tx += 70) gfx_circ(tx, 70, 22, C_NIGHT);
    for (int tx = -wrapx(cam, 90); tx < SCREEN_W + 40; tx += 90) {
        gfx_rect(tx + 20, 60, 8, 48, C_BROWN);
        gfx_circ(tx + 24, 54, 20, C_FOREST);
        gfx_circ(tx + 12, 62, 12, C_FOREST);
        gfx_pset(tx + 30, 50, C_RED);
        gfx_pset(tx + 16, 58, C_RED);
    }
    gfx_rect(0, 108, SCREEN_W, 72, C_FOREST);
    gfx_dither(0, 108, SCREEN_W, 72, C_EARTH, 6);
    gfx_dither(0, 100, SCREEN_W, 16, C_GREY, 2);
}

static void bg_graves(int cam) {
    sky(DKU_TOP, 108, C_NIGHT, C_PURPLE);
    stars(cam, DKU_TOP, 60);
    gfx_circ(70, 44, 12, C_CREAM);
    for (int fx = -wrapx(cam, 8); fx < SCREEN_W; fx += 8) gfx_vline(fx, 84, 108, C_INK);
    gfx_hline(0, SCREEN_W, 86, C_INK);
    for (int gx = -wrapx(cam, 72) + 10; gx < SCREEN_W; gx += 72) {
        gfx_rect(gx, 90, 14, 18, C_GREY);
        gfx_rect(gx + 2, 88, 10, 2, C_GREY);
        gfx_hline(gx + 3, gx + 10, 96, C_SLATE);
        gfx_rect(gx + 40, 94, 4, 14, C_GREY);
        gfx_rect(gx + 37, 97, 10, 3, C_GREY);
    }
    gfx_rect(0, 108, SCREEN_W, 72, C_EARTH);
    gfx_dither(0, 108, SCREEN_W, 72, C_BROWN, 5);
}

static void bg_ship(void) {
    gfx_rect(0, DKU_TOP, SCREEN_W, 86, C_SLATE);
    for (int x = 0; x < SCREEN_W; x += 32) {
        gfx_rect(x + 2, 30, 28, 70, C_GREY);
        gfx_rect(x + 4, 32, 24, 66, C_SLATE);
        for (int k = 0; k < 3; k++) gfx_pset(x + 8 + k * 6, 40 + ((x / 32 + k) % 4) * 10, ((dku_g.frame_t / 10 + k + x) % 3) ? C_CYAN : C_LIME);
    }
    gfx_rect(0, 108, SCREEN_W, 72, C_DUSK);
    for (int x = 0; x < SCREEN_W; x += 20) gfx_vline(x, 108, 180, C_SLATE);
    for (int y = 112; y < 180; y += 12) gfx_hline(0, SCREEN_W, y, C_SLATE);
    gfx_dither(130, 108, 60, 72, C_CYAN, 2);
}

static void bg_prom(int cam, bool jetty) {
    sky(DKU_TOP, 108, C_NAVY, C_PURPLE);
    stars(cam, DKU_TOP, 50);
    moon(60, 40);
    /* the sea */
    gfx_rect(0, 80, SCREEN_W, 28, C_BLUE);
    for (int k = 0; k < 14; k++) {
        int x = wrapx(k * 29 - cam / 2 + dku_g.frame_t / 4, SCREEN_W);
        gfx_hline(x, x + 6, 84 + (k * 7) % 20, C_SKY);
    }
    if (!jetty) {
        for (int bx = -wrapx(cam, 110); bx < SCREEN_W; bx += 110) {
            gfx_rect(bx, 64, 70, 44, C_CREAM);
            gfx_rect(bx, 60, 70, 4, C_RED);
            for (int k = 0; k < 7; k++) gfx_rect(bx + k * 10, 64, 5, 6, (k & 1) ? C_RED : C_WHITE);
            gfx_rect(bx + 10, 78, 50, 20, C_INK);
            gfx_rect(bx + 12, 80, 46, 16, C_TEAL);
            for (int k = 0; k < 11; k++) gfx_pset(bx + 75 + k * 3, 58 + (k % 2), (k % 3) ? C_YELLOW : C_PINK);
        }
    } else {
        for (int px = -wrapx(cam, 40); px < SCREEN_W; px += 40) gfx_rect(px, 90, 4, 18, C_BROWN);
    }
    gfx_rect(0, 108, SCREEN_W, 72, C_TAN);
    for (int y = 108; y < 180; y += 6) gfx_hline(0, SCREEN_W, y, C_BROWN);
    for (int x = -wrapx(cam, 36); x < SCREEN_W; x += 36) gfx_vline(x, 108, 180, C_EARTH);
}

static void bg_dunes(int cam) {
    sky(DKU_TOP, 108, C_NAVY, C_DUSK);
    stars(cam, DKU_TOP, 50);
    gfx_rect(0, 80, SCREEN_W, 28, C_BLUE);
    for (int k = 0; k < 14; k++) {
        int x = wrapx(k * 31 - cam / 2 + dku_g.frame_t / 3, SCREEN_W);
        gfx_hline(x, x + 7, 84 + (k * 5) % 22, C_SKY);
    }
    /* the hotel, getting nearer */
    int hx = 1180 - cam;
    if (hx < SCREEN_W) {
        gfx_rect(hx, 34, 160, 74, C_WHITE);
        for (int y = 40; y < 100; y += 10)
            for (int x = hx + 8; x < hx + 152; x += 14) gfx_rect(x, y, 7, 6, ((x + y) % 7) ? C_NAVY : C_YELLOW);
        put("hotel sign", "GRAND HOTEL", hx + 40, 26, C_YELLOW);
    }
    gfx_rect(0, 108, SCREEN_W, 72, C_CREAM);
    gfx_dither(0, 108, SCREEN_W, 72, C_TAN, 5);
}

static void bg_hotel(int cam) {
    gfx_rect(0, DKU_TOP, SCREEN_W, 86, C_JADE);
    for (int x = -wrapx(cam, 12); x < SCREEN_W; x += 12) gfx_vline(x, DKU_TOP, 100, C_FOREST);
    for (int dx = -wrapx(cam, 100); dx < SCREEN_W; dx += 100) {
        gfx_rect(dx + 10, 58, 24, 44, C_BROWN);
        gfx_rect(dx + 12, 60, 20, 40, C_EARTH);
        gfx_pset(dx + 28, 82, C_YELLOW);
        gfx_rect(dx + 56, 50, 26, 20, C_AMBER);
        gfx_rect(dx + 58, 52, 22, 16, C_NAVY);
        gfx_circ(dx + 69, 60, 4, C_CREAM);
    }
    gfx_rect(0, 100, SCREEN_W, 8, C_BROWN);
    gfx_rect(0, 108, SCREEN_W, 72, C_WINE);
    for (int x = -wrapx(cam, 24); x < SCREEN_W; x += 24) gfx_rect(x, 136, 12, 4, C_AMBER);
}

static void bg_lift(void) {
    gfx_rect(0, DKU_TOP, SCREEN_W, 158, C_INK);
    /* the shaft's walls and the car */
    gfx_rect(60, 40, 200, 140, C_SLATE);
    gfx_rect(64, 60, 192, 64, C_GREY);
    for (int x = 72; x < 256; x += 24) gfx_vline(x, 60, 124, C_SLATE);
    /* the roof the ghouls wait on */
    gfx_rect(60, 36, 200, 8, C_LIGHT);
    for (int k = 0; k < 10; k++) gfx_hline(0, 58, 24 + ((k * 17 + dku_g.frame_t) % 150), C_DUSK);
    for (int k = 0; k < 10; k++) gfx_hline(262, SCREEN_W, 24 + ((k * 23 + dku_g.frame_t) % 150), C_DUSK);
    gfx_rect(64, 124, 192, 52, C_DUSK);
    for (int y = 128; y < 176; y += 8) gfx_hline(64, 255, y, C_SLATE);
    char fl[24];
    snprintf(fl, sizeof fl, "FLOOR %d", 10 + dku_g.lift_wave * 4);
    put("lift floor", fl, 136, 46, C_AMBER);
}

static void bg_penthouse(void) {
    gfx_rect(0, DKU_TOP, SCREEN_W, 86, C_NIGHT);
    gfx_rect(20, 30, 280, 64, C_INK);
    for (int k = 0; k < 40; k++) gfx_pset(24 + (k * 41) % 270, 60 + (k * 13) % 32, (k % 4) ? C_YELLOW : C_WHITE);
    for (int k = 0; k < 8; k++) gfx_rect(26 + k * 34, 70 - (k * 7) % 20, 22, 24 + (k * 7) % 20, C_DUSK);
    for (int x = 20; x <= 300; x += 56) gfx_vline(x, 30, 94, C_SLATE);
    gfx_rect(0, 94, SCREEN_W, 14, C_PURPLE);
    gfx_rect(0, 108, SCREEN_W, 72, C_PURPLE);
    gfx_dither(0, 108, SCREEN_W, 72, C_MAGENTA, 3);
    gfx_rect(110, 120, 100, 40, C_MAROON);
    gfx_rectb(110, 120, 100, 40, C_AMBER);
}

static void bg_gym(void) {
    gfx_rect(0, DKU_TOP, SCREEN_W, 86, C_CREAM);
    for (int y = 40; y < 104; y += 8) gfx_rect(0, y, SCREEN_W, 4, C_TAN);
    /* the bleachers and the scoreboard */
    gfx_rect(110, 26, 100, 32, C_INK);
    gfx_rectb(110, 26, 100, 32, C_GREY);
    char a[16], b[16];
    snprintf(a, sizeof a, "%02d", imin(99, dku_g.gym_wave));
    uint16_t best = dku_g.players == 2 ? dku_sv.gym_best2 : dku_sv.gym_best1;
    snprintf(b, sizeof b, "%02d", imin(99, (int)best));
    tput("home", "HOME", 120, 30, C_LIGHT);
    tput("guest", "GUEST", 178, 30, C_LIGHT);
    text_draw_scaled(a, 118, 38, C_RED, 2);
    text_draw_scaled(b, 176, 38, C_YELLOW, 2);
    for (int k = 0; k < 3; k++) {
        gfx_rect(20 + k * 220 / 2, 70, 60, 6, C_YELLOW);
        tput("banner", "GO HORNETS!", 24 + k * 110, 71, C_INK);
    }
    gfx_rect(0, 108, SCREEN_W, 72, C_AMBER);
    for (int x = 0; x < SCREEN_W; x += 16) gfx_vline(x, 108, 180, C_TAN);
    gfx_hline(0, SCREEN_W, 110, C_WHITE);
    gfx_circb(160, 146, 22, C_WHITE);
    gfx_vline(160, 110, 180, C_WHITE);
}

static void draw_backdrop(int theme, int cam) {
    switch (theme) {
    case TH_STREET: bg_street(cam, false); break;
    case TH_CANAL: bg_street(cam, true); break;
    case TH_CLUB: bg_club(cam); break;
    case TH_FERRY: bg_ferry(cam); break;
    case TH_WAXWORKS: bg_waxworks(); break;
    case TH_ORCHARD: bg_orchard(cam); break;
    case TH_GRAVES: bg_graves(cam); break;
    case TH_SHIP: bg_ship(); break;
    case TH_PROM: bg_prom(cam, false); break;
    case TH_JETTY: bg_prom(cam, true); break;
    case TH_DUNES: bg_dunes(cam); break;
    case TH_HOTEL: bg_hotel(cam); break;
    case TH_LIFT: bg_lift(); break;
    case TH_PENTHOUSE: bg_penthouse(); break;
    case TH_GYM: bg_gym(); break;
    default: gfx_cls(C_INK); break;
    }
}

/* ---- the floor's hazards --------------------------------------------------------- */

static void draw_hazard_floor(const Hazard *h, int cam) {
    int x = h->x - cam;
    switch (h->kind) {
    case HZ_PIT:
        gfx_rect(x, h->y, h->w, h->h, C_INK);
        gfx_rect(x, h->y, h->w, 2, C_EARTH);
        gfx_dither(x, h->y + 2, h->w, 3, C_NIGHT, 8);
        break;
    case HZ_WATER:
        gfx_rect(x, h->y, h->w, h->h, C_NAVY);
        for (int k = 0; k < h->w / 14; k++) {
            int wx = x + (k * 14 + dku_g.frame_t / 4) % imax(1, h->w);
            gfx_hline(wx, imin(wx + 5, x + h->w - 1), h->y + 2 + (k * 5) % imax(1, h->h - 3), C_BLUE);
        }
        gfx_hline(x, x + h->w - 1, h->y, C_SKY);
        break;
    case HZ_MINE:
        gfx_rect(x + 1, h->y + 1, 8, 4, C_EARTH);
        gfx_rect(x + 3, h->y, 4, 2, C_BROWN);
        break;
    case HZ_LAMP: {
        int lv = h->t > 0 ? 6 + (h->t & 2) * 2 : 5;
        gfx_dither(x - h->w / 2, h->y - h->h / 2, h->w, h->h, C_INK, lv);
        break;
    }
    case HZ_FIRE: case HZ_FIREWALL:
        if (h->kind == HZ_FIREWALL) {
            gfx_rect(x, h->y + h->h - 6, h->w, 6, C_BROWN);
            for (int k = 0; k < h->w; k += 6) gfx_rect(x + k, h->y + h->h - 10, 4, 4, C_EARTH);
        }
        for (int k = 0; k < h->w; k += 4) {
            int fh = 4 + ((k * 7 + dku_g.frame_t / 3) % 6) + (h->kind == HZ_FIREWALL ? 8 : 0);
            gfx_rect(x + k, h->y + h->h - fh, 3, fh, ((k + dku_g.frame_t / 4) & 4) ? C_ORANGE : C_RED);
            gfx_pset(x + k + 1, h->y + h->h - fh - 1, C_YELLOW);
        }
        break;
    case HZ_CLOUD: {
        int c = h->arg >= 3 ? C_PURPLE : C_LIME;
        gfx_dither_circle(x, h->y - 4, h->w, c, 6 + ((dku_g.frame_t / 8) & 1) * 2);
        break;
    }
    case HZ_BEAM:
        if (dku_g.exit_t > 0) {
            gfx_dither(x, DKU_TOP, h->w, 180 - DKU_TOP, C_CYAN, 4 + ((dku_g.frame_t / 4) & 3));
            gfx_vline(x + h->w / 2, DKU_TOP, 180, C_WHITE);
        }
        break;
    case HZ_DOOR:
        gfx_rect(x, h->y, h->w, h->h, C_INK);
        gfx_rectb(x - 1, h->y - 1, h->w + 2, h->h + 1, C_AMBER);
        if (dku_g.exit_t > 0 && (dku_g.frame_t & 16)) put("door arrow", GLYPH_RIGHT, x + h->w / 2 - 3, h->y + 14, C_YELLOW);
        break;
    default: break;
    }
}

static void draw_hazard_top(const Hazard *h, int cam) {
    int x = h->x - cam;
    switch (h->kind) {
    case HZ_LAMP: {
        if (h->arg == 2) {
            /* the Undertow's feeler, coming down */
            int drop = h->t > 12 ? 0 : (12 - h->t) * 7;
            int top = DKU_TOP + drop;
            gfx_rect(x - 4, DKU_TOP, 9, top - DKU_TOP + 20, C_INK);
            gfx_rect(x - 3, DKU_TOP, 7, top - DKU_TOP + 19, C_PURPLE);
            break;
        }
        int fall = h->t > 0 ? (34 - h->t) * (34 - h->t) / 8 : 0;
        int cy = 40 + fall;
        gfx_vline(x, DKU_TOP, cy - 4, C_GREY);
        gfx_rect(x - 10, cy - 4, 21, 4, C_AMBER);
        gfx_rect(x - 14, cy, 29, 3, C_YELLOW);
        for (int k = -12; k <= 12; k += 6) gfx_rect(x + k, cy + 3, 2, 3, C_CREAM);
        break;
    }
    case HZ_CAR: {
        if (h->t < 60) {
            if (dku_g.frame_t & 8) {
                gfx_rect(dku_view_left() - cam, h->y - 8, 6, 6, C_YELLOW);
                put("horn", "!", 10, h->y - 22, C_YELLOW);
            }
            break;
        }
        int y = h->y;
        gfx_rect(x, y - 16, h->w, 14, C_INK);
        gfx_rect(x + 1, y - 15, h->w - 2, 12, C_RED);
        gfx_rect(x + 14, y - 22, 26, 8, C_INK);
        gfx_rect(x + 16, y - 21, 22, 6, C_SKY);
        gfx_rect(x + h->w - 4, y - 12, 4, 4, C_YELLOW);
        gfx_circ(x + 10, y - 2, 4, C_INK);
        gfx_circ(x + h->w - 10, y - 2, 4, C_INK);
        break;
    }
    case HZ_THRESHER: {
        int y0 = DKU_FLOOR0 - 30;
        gfx_rect(x, y0, h->w, 180 - y0, C_INK);
        gfx_rect(x + 2, y0 + 2, h->w - 4, 40, C_RED);
        gfx_rect(x + 8, y0 - 14, 20, 16, C_INK);
        gfx_rect(x + 10, y0 - 12, 16, 12, C_SKY);
        for (int k = 0; k < 8; k++) {
            int ty = DKU_FLOOR0 - 10 + k * 10 + (dku_g.frame_t / 2) % 10;
            gfx_rect(x + h->w - 6, ty, 10, 3, C_LIGHT);
        }
        gfx_circ(x + 14, 168, 10, C_SLATE);
        gfx_circ(x + 14, 168, 4, C_GREY);
        break;
    }
    default: break;
    }
}

/* ---- the floor: everyone in depth order ---------------------------------------------- */

typedef struct { int y; uint8_t type; uint8_t idx; } Drawable;

static void draw_shot(const Shot *s, int cam) {
    int x = dku_px(s->x) - cam, y = dku_px(s->y), z = dku_px(s->z);
    if (s->kind != SH_RAY && s->kind != SH_PELLET) gfx_dither(x - 3, y - 1, 6, 2, C_INK, 8);
    int sy = y - z;
    switch (s->kind) {
    case SH_BOTTLE:
        gfx_rect(x - 1, sy - 3, 3, 5, C_JADE);
        gfx_pset(x, sy - 4 - ((dku_g.frame_t / 2) & 1), C_ORANGE);
        break;
    case SH_BOMB:
        gfx_circ(x, sy, 3, C_INK);
        gfx_pset(x + 1, sy - 4, (dku_g.frame_t & 2) ? C_YELLOW : C_RED);
        break;
    case SH_CLEAVER: {
        int r = (dku_g.frame_t / 3) & 3;
        static const int8_t DX[4] = {3, 0, -3, 0}, DY[4] = {0, 3, 0, -3};
        gfx_line(x - DX[r], sy - DY[r], x + DX[r], sy + DY[r], C_LIGHT);
        gfx_pset(x - DX[r], sy - DY[r], C_BROWN);
        break;
    }
    case SH_RAY:
        gfx_rect(x - 6, sy - 1, 12, 3, s->team == 1 ? C_CYAN : C_YELLOW);
        gfx_hline(x - 5, x + 5, sy, C_WHITE);
        break;
    case SH_PELLET: gfx_rect(x - 1, sy - 1, 2, 2, C_YELLOW); break;
    case SH_ITEM: spr_draw(&dku_spr[SPR_ITEM0 + s->arg], x - 5, sy - 6, s->vx < 0 ? SPR_FLIPX : 0); break;
    case SH_GAS: gfx_circ(x, sy, 3, C_PURPLE); break;
    case SH_SPIT: gfx_circ(x, sy, 2, C_LIME); break;
    default: break;
    }
}

static void draw_floor_things(int cam) {
    static Drawable list[DKU_MAX_ACTORS + DKU_MAX_ITEMS + DKU_MAX_PROPS + DKU_MAX_SHOTS];
    int n = 0;
    for (int i = 0; i < DKU_MAX_ACTORS; i++)
        if (dku_g.a[i].alive) list[n++] = (Drawable){dku_px(dku_g.a[i].y) * 4 + (dku_g.a[i].kind == AK_FIGHTER ? 1 : 0), 0, (uint8_t)i};
    for (int i = 0; i < DKU_MAX_ITEMS; i++)
        if (dku_g.it[i].alive == 1) list[n++] = (Drawable){dku_px(dku_g.it[i].y) * 4 - 2, 1, (uint8_t)i};
    for (int i = 0; i < DKU_MAX_PROPS; i++)
        if (dku_g.pr_[i].alive) list[n++] = (Drawable){dku_px(dku_g.pr_[i].y) * 4 - 1, 2, (uint8_t)i};
    for (int i = 0; i < DKU_MAX_SHOTS; i++)
        if (dku_g.sh[i].alive) list[n++] = (Drawable){dku_px(dku_g.sh[i].y) * 4 + 2, 3, (uint8_t)i};
    for (int i = 1; i < n; i++) {
        Drawable d = list[i];
        int j = i - 1;
        while (j >= 0 && list[j].y > d.y) { list[j + 1] = list[j]; j--; }
        list[j + 1] = d;
    }
    for (int k = 0; k < n; k++) {
        const Drawable *d = &list[k];
        if (d->type == 0) {
            const Actor *a = &dku_g.a[d->idx];
            int sx = dku_px(a->x) - cam, sy = dku_px(a->y);
            if (sx < -40 || sx > SCREEN_W + 40) continue;
            dku_draw_actor(a, sx, sy);
            if (a->state == AS_GRABBED && a->partner >= 0 && dku_g.a[a->partner].kind == AK_FIGHTER &&
                dku_g.pr[dku_g.a[a->partner].player].pick == DK_DOLLY && dku_g.a[a->partner].grab_charge > 0 && (dku_g.frame_t & 4))
                gfx_rectb(sx - 6, sy - 28, 12, 28, C_ORANGE);
        } else if (d->type == 1) {
            const Item *it = &dku_g.it[d->idx];
            int sx = dku_px(it->x) - cam, sy = dku_px(it->y) - dku_px(it->z);
            gfx_dither(sx - 4, dku_px(it->y) - 1, 8, 2, C_INK, 8);
            bool blink = DKU_ITEMS[it->kind].kind == IK_CASH && ((dku_g.frame_t + it->x) & 32);
            spr_draw(&dku_spr[SPR_ITEM0 + it->kind], sx - 5, sy - 7, 0);
            if (blink) gfx_pset(sx + 2, sy - 6, C_WHITE);
        } else if (d->type == 2) {
            const Prop *p = &dku_g.pr_[d->idx];
            int sx = dku_px(p->x) - cam + (p->shake & 2 ? 1 : 0), sy = dku_px(p->y);
            spr_draw(&dku_spr[SPR_PROP0 + p->kind], sx - 6, sy - 10, 0);
        } else {
            draw_shot(&dku_g.sh[d->idx], cam);
        }
    }
    for (int i = 0; i < DKU_MAX_PARTS; i++) {
        const Part *p = &dku_g.part[i];
        if (p->life <= 0) continue;
        gfx_pset(dku_px(p->x) - cam, dku_px(p->y) - dku_px(p->z), p->col);
    }
}

/* ---- the HUD ---------------------------------------------------------------------- */

static void portrait(int pick, int x, int y) {
    const DkuFighter *f = &DKU_FIGHTERS[pick];
    gfx_rect(x, y, 16, 16, C_INK);
    gfx_rect(x + 1, y + 1, 14, 14, f->top);
    gfx_rect(x + 4, y + 3, 8, 9, f->skin);
    gfx_rect(x + 4, y + 2, 8, 2, f->hair);
    if (pick == DK_ROOK) gfx_rect(x + 3, y + 2, 10, 2, f->top2);
    if (pick == DK_PIP) { gfx_rect(x + 2, y + 5, 2, 5, f->hair); gfx_rect(x + 12, y + 5, 2, 5, f->hair); }
    if (pick == DK_DOLLY) gfx_circ(x + 8, y + 2, 2, f->hair);
    gfx_pset(x + 6, y + 7, C_INK);
    gfx_pset(x + 9, y + 7, C_INK);
    gfx_hline(x + 6, x + 9, y + 10, C_MAROON);
    gfx_rect(x + 3, y + 12, 10, 3, f->top2);
}

static void hp_bar(int x, int y, int w, int hp, int max) {
    gfx_rect(x - 1, y - 1, w + 2, 7, C_INK);
    int fill = max > 0 ? iclamp(hp * w / max, 0, w) : 0;
    gfx_rect(x, y, w, 5, C_RED);
    gfx_rect(x, y, fill, 5, C_YELLOW);
    gfx_hline(x, x + fill - 1, y, C_CREAM);
}

static void draw_hud(void) {
    gfx_rect(0, 0, SCREEN_W, DKU_TOP, C_INK);
    gfx_hline(0, SCREEN_W - 1, DKU_TOP - 1, C_DUSK);
    char buf[48];
    for (int p = 0; p < dku_g.players; p++) {
        const Actor *a = &dku_g.a[p];
        int x = p == 0 ? 3 : 223;
        portrait(dku_g.pr[p].pick, x, 3);
        tput("hud name", DKU_FIGHTERS[dku_g.pr[p].pick].name, x + 19, 2, C_LIGHT);
        hp_bar(x + 19, 9, 70, a->alive ? a->hp : 0, DKU_MAX_HP);
        if (p == 0 || dku_g.players == 2) {
            snprintf(buf, sizeof buf, "$%d", dku_g.cash);
            if (p == 0) tput("hud cash", buf, x + 19, 16, C_LIME);
        }
    }
    /* the middle: the place, the gym's wave, or the boss */
    if (dku_g.boss >= 0 && dku_g.a[dku_g.boss].alive && dku_g.a[dku_g.boss].boss && !dku_g.boss_down) {
        const Actor *b = &dku_g.a[dku_g.boss];
        const char *nm = b->kind == AK_RAMMER ? "BIG RAM" : b->kind == AK_TUSKER ? "THE BOAR BARON"
                         : b->kind == AK_VISITOR ? "THE VISITORS" : b->kind == AK_UNDERTOW ? "THE UNDERTOW"
                         : b->mode == 2 ? "GRIST UNBOUND" : "ALDERMAN GRIST";
        tput("boss name", nm, 160 - tiny_width(nm) / 2, 3, C_PINK);
        int hp = 0, max = 0;
        for (int i = 2; i < DKU_MAX_ACTORS; i++)
            if (dku_g.a[i].alive && dku_g.a[i].boss && dku_g.a[i].state != AS_DEAD) { hp += dku_g.a[i].hp; max += dku_g.a[i].maxhp; }
        hp_bar(120, 11, 80, hp, max);
    } else if (dku_g.gym) {
        snprintf(buf, sizeof buf, "WAVE %d", dku_g.gym_wave);
        tput("gym wave", buf, 160 - tiny_width(buf) / 2, 4, C_YELLOW);
        snprintf(buf, sizeof buf, "BEST %d", dku_g.players == 2 ? dku_sv.gym_best2 : dku_sv.gym_best1);
        tput("gym best", buf, 160 - tiny_width(buf) / 2, 12, C_LIGHT);
    } else {
        const DkuSection *s = &DKU_NIGHT[dku_g.night].sec[dku_g.sec];
        tput("night", DKU_NIGHT[dku_g.night].name, 160 - tiny_width(DKU_NIGHT[dku_g.night].name) / 2, 4, C_YELLOW);
        tput("place", s->name, 160 - tiny_width(s->name) / 2, 12, C_LIGHT);
    }
    if (dku_g.go_t > 0 && (dku_g.go_t & 16) && !dku_g.gym) {
        put("go", "GO " GLYPH_RIGHT, 276, 40, C_YELLOW);
    }
}

static void draw_play(void) {
    int cam = dku_g.cam;
    int sh = dku_g.shake > 0 ? ((dku_g.shake & 2) ? 1 : -1) : 0;
    const DkuSection *sec = dku_g.gym ? &DKU_GYM : &DKU_NIGHT[dku_g.night].sec[dku_g.sec];
    gfx_camera(0, 0);
    draw_backdrop(sec->theme, cam + sh);
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) if (dku_g.hz[i].alive) draw_hazard_floor(&dku_g.hz[i], cam + sh);
    draw_floor_things(cam + sh);
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) if (dku_g.hz[i].alive) draw_hazard_top(&dku_g.hz[i], cam + sh);
    if (sec->kind == SEC_LIFT) {
        /* the lift's doors when it arrives */
        if (dku_g.lift_wave > 5) put("lift open", "THE DOORS OPEN...", 104, 90, C_YELLOW);
    }
    draw_hud();
    if (dku_g.fade > 0) {
        int lv = dku_g.fade < 16 ? dku_g.fade : 30 - dku_g.fade;
        gfx_dither(0, DKU_TOP, SCREEN_W, SCREEN_H - DKU_TOP, C_INK, iclamp(lv, 0, 16));
    }
    if (dku_g.dead_t > 10) {
        gfx_dither(0, 70, SCREEN_W, 30, C_INK, 10);
        putc_("down", dku_g.gym ? "FINAL WHISTLE!" : "DOWN FOR THE COUNT...", 160, 81, C_RED);
    }
}

/* ---- screens ------------------------------------------------------------------- */

static void skyline(int t) {
    gfx_cls(C_NIGHT);
    stars(t / 2, 0, 90);
    moon(272, 30);
    for (int k = 0; k < 12; k++) {
        int x = k * 28 - 4, h = 40 + (k * 37) % 50;
        gfx_rect(x, 150 - h, 26, h + 30, C_INK);
        for (int y = 156 - h; y < 146; y += 9)
            for (int wx = x + 4; wx < x + 22; wx += 7) if (((wx + y + k) % 4) == 0) gfx_rect(wx, y, 3, 4, C_YELLOW);
    }
    gfx_rect(0, 150, SCREEN_W, 30, C_DUSK);
}

static void title_screen(void) {
    skyline(dku_g.frame_t);
    static const uint8_t grad[3] = {C_YELLOW, C_AMBER, C_ORANGE};
    ui_audit_fancy("title", "DUKES UP", 160 - ui_fancy_width("DUKES UP", 4) / 2, 14, 4, true);
    ui_fancy_center("DUKES UP", 160, 14, 4, grad, 3, C_INK, C_MAROON);
    putc_("subtitle", "A NIGHT IN LAMPWICK", 160, 50, C_LIGHT);
    static const int FX[DK_NFIGHTERS] = {34, 84, 236, 286};
    for (int f = 0; f < DK_NFIGHTERS; f++) dku_draw_fighter_big(f, FX[f], 156, dku_g.frame_t + f * 7);
    const char *opt[2] = {"1 PLAYER", "2 PLAYERS"};
    for (int i = 0; i < 2; i++) {
        int y = 82 + i * 12;
        bool off = i == 1 && plat_kind() == PLAT_VITA;
        put("menu", opt[i], 136, y, off ? C_SLATE : dku_g.menu == i ? C_YELLOW : C_WHITE);
        if (dku_g.menu == i) ui_cursor(126, y, dku_g.frame_t);
    }
    char buf[48];
    snprintf(buf, sizeof buf, "GYM BEST  1P %d  2P %d", dku_sv.gym_best1, dku_sv.gym_best2);
    tput("records", buf, 160 - tiny_width(buf) / 2, 158, C_LIGHT);
    snprintf(buf, sizeof buf, "TOWNS SAVED %d   GRAN SAFE %d", dku_sv.wins, dku_sv.good_wins);
    tput("records2", buf, 160 - tiny_width(buf) / 2, 166, C_LIGHT);
}

static void story_screen(void) {
    skyline(dku_g.frame_t);
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, 8);
    static const char *const STORY =
        "THE GHOULS CAME UP OUT OF LAMPWICK'S STORM DRAINS ON A FRIDAY. ALDERMAN GRIST SAYS IT'S ALL UNDER CONTROL.";
    static const char *const STORY2 =
        "ROOK'S GRAN WENT DOWN TO CITY HALL TO COMPLAIN, AND SHE NEVER CAME HOME.";
    static const char *const STORY3 = "FOUR FRIENDS ROLL UP THEIR SLEEVES.";
    int y = 26;
    y += wrapped_c("story", STORY, 160, y, 280, C_WHITE) * LINE_H + 8;
    y += wrapped_c("story2", STORY2, 160, y, 280, C_WHITE) * LINE_H + 8;
    wrapped_c("story3", STORY3, 160, y, 280, C_YELLOW);
    if ((dku_g.state_t / 20) & 1) putc_("press", GLYPH_A " PUT 'EM UP", 160, 160, C_LIGHT);
}

static void stat_bars(int x, int y, const uint8_t *st) {
    static const char *const N[DK_NSTATS] = {"POWER", "RECOV", "TOUGH", "THROW"};
    for (int s = 0; s < DK_NSTATS; s++) {
        tput("stat", N[s], x, y + s * 7, C_LIGHT);
        for (int k = 0; k < 3; k++) gfx_rect(x + 26 + k * 7, y + s * 7, 6, 5, k < st[s] ? C_YELLOW : C_DUSK);
    }
}

static void select_screen(void) {
    gfx_cls(C_NIGHT);
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_DUSK, 4);
    putc_("pick", dku_g.players == 2 ? "PICK YOUR FIGHTERS" : "PICK YOUR FIGHTER", 160, 6, C_YELLOW);
    for (int f = 0; f < DK_NFIGHTERS; f++) {
        int cx = 40 + f * 80;
        bool on0 = dku_g.sel[0] == f, on1 = dku_g.players == 2 && dku_g.sel[1] == f;
        ui_panel(cx - 36, 20, 72, 92, on0 || on1 ? C_WINE : C_NIGHT, on0 ? C_YELLOW : on1 ? C_CYAN : C_SLATE);
        dku_draw_fighter_big(f, cx, 108, dku_g.frame_t);
        putc_("fighter name", DKU_FIGHTERS[f].name, cx, 24, on0 || on1 ? C_WHITE : C_LIGHT);
        if (on0) tput("p1 mark", dku_g.picked[0] ? "1P OK" : "1P", cx - 33, 102, C_YELLOW);
        if (on1) tput("p2 mark", dku_g.picked[1] ? "2P OK" : "2P", cx + 14, 102, C_CYAN);
    }
    int show = dku_g.sel[0];
    const DkuFighter *fd = &DKU_FIGHTERS[show];
    ui_panel(8, 116, 304, 60, C_NIGHT, C_SLATE);
    put("tag", fd->tag, 16, 121, C_YELLOW);
    stat_bars(16, 133, fd->stat);
    wrapped("special", fd->special, 90, 133, 150, C_WHITE);
    if (dku_g.players == 2) {
        const DkuFighter *f2 = &DKU_FIGHTERS[dku_g.sel[1]];
        stat_bars(250, 133, f2->stat);
        tput("p2 name", f2->name, 250, 162, C_CYAN);
    }
}

static void card_screen(void) {
    gfx_cls(C_INK);
    static const uint8_t grad[2] = {C_YELLOW, C_ORANGE};
    const char *n = DKU_NIGHT[dku_g.night].name;
    ui_audit_fancy("card night", n, 160 - ui_fancy_width(n, 3) / 2, 60, 3, true);
    ui_fancy_center(n, 160, 60, 3, grad, 2, C_MAROON, C_INK);
    putc_("card place", DKU_NIGHT[dku_g.night].place, 160, 96, C_WHITE);
    if (dku_g.continues > 0) {
        char b[32];
        snprintf(b, sizeof b, "CONTINUES: %d", dku_g.continues);
        putc_("card continues", b, 160, 120, C_RED);
    }
}

static void clear_screen(void) {
    draw_play();
    ui_panel(70, 54, 180, 60, C_NIGHT, C_YELLOW);
    char b[48];
    snprintf(b, sizeof b, "%s DONE!", DKU_NIGHT[dku_g.night].name);
    putc_("clear", b, 160, 62, C_YELLOW);
    snprintf(b, sizeof b, "CASH $%d", dku_g.cash);
    putc_("clear cash", b, 160, 78, C_LIME);
    if (dku_g.state_t > 60 && ((dku_g.state_t / 20) & 1)) putc_("clear next", GLYPH_A " ON", 160, 96, C_WHITE);
}

static void shop_screen(void) {
    gfx_cls(C_BROWN);
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_EARTH, 4);
    putc_("shop", "THE CORNER SHOP", 160, 6, C_YELLOW);
    int who = dku_g.shop_who;
    const Actor *a = &dku_g.a[who];
    char b[64];
    ui_panel(8, 20, 150, 128, C_NIGHT, C_AMBER);
    static const char *const NAMES[5] = {"HOT SOUP", "HEAVY BAG", "VITAMIN TONIC", "LEATHER JACKET", "ON TO NIGHT"};
    static const int PRICE[5] = {DKU_PRICE_HEAL, DKU_PRICE_STAT, DKU_PRICE_STAT, DKU_PRICE_STAT, 0};
    static const char *const DESC[5] = {"BACK TO FULL HEALTH.", "+1 POWER: EVERY BLOW HITS HARDER.",
                                        "+1 RECOV: UP FASTER, QUICKER CHARGES.", "+1 TOUGH: BLOWS HURT LESS.",
                                        "THE NIGHT IS WAITING."};
    for (int r = 0; r < 5; r++) {
        int y = 28 + r * 16;
        bool on = dku_g.shop_sel == r;
        if (r == 4) snprintf(b, sizeof b, "%s %d", NAMES[r], dku_g.night + 2);
        else snprintf(b, sizeof b, "%s", NAMES[r]);
        if (r == 4 && dku_g.players == 2 && who == 0) snprintf(b, sizeof b, "PLAYER 2'S TURN");
        put("shop row", b, 26, y, on ? C_YELLOW : C_WHITE);
        if (PRICE[r]) {
            snprintf(b, sizeof b, "$%d", PRICE[r]);
            put("shop price", b, 128, y, dku_g.cash >= PRICE[r] ? C_LIME : C_SLATE);
        }
        if (on) ui_cursor(14, y, dku_g.frame_t);
    }
    wrapped("shop desc", DESC[dku_g.shop_sel], 16, 110, 134, C_LIGHT);
    ui_panel(164, 20, 148, 128, C_NIGHT, C_AMBER);
    portrait(dku_g.pr[who].pick, 172, 28);
    snprintf(b, sizeof b, "%s", DKU_FIGHTERS[dku_g.pr[who].pick].name);
    put("shopper", b, 194, 32, C_YELLOW);
    hp_bar(172, 50, 130, a->hp, DKU_MAX_HP);
    stat_bars(172, 62, dku_g.pr[who].stat);
    snprintf(b, sizeof b, "CASH $%d", dku_g.cash);
    put("shop cash", b, 172, 96, C_LIME);
    if (dku_g.shop_msg && dku_g.shop_msg_t > 0) putc_("shop msg", dku_g.shop_msg, 160, 156, C_WHITE);
    else putc_("shop hint", GLYPH_A " BUY   " GLYPH_UP GLYPH_DOWN " CHOOSE", 160, 156, C_LIGHT);
}

const char *const DKU_TIPS[DKU_NTIPS] = {
    "TIP: FOUR PUNCHES, A BREATH, FOUR MORE. THE FIFTH KNOCKS THEM AWAY.",
    "TIP: STAND A LITTLE BELOW A GHOUL. YOUR FISTS REACH UP; THEIRS DON'T REACH DOWN.",
    "TIP: GHOULS TURN UP AS THE STREET SCROLLS ON. WALK SLOWLY, MEET THEM A FEW AT A TIME.",
    "TIP: " GLYPH_A "+" GLYPH_B " SPINS. IT COSTS HEALTH, BUT ONLY WHEN IT CONNECTS.",
    "TIP: AS NIGHT 1 BEGINS, THE GYM IS JUST BEHIND YOU. WALK LEFT.",
    "TIP: FOOD AT FULL HEALTH IS CASH IN YOUR POCKET.",
    "TIP: NOTHING IN TOWN GIVES YOU A MOMENT'S SHELTER. NOT EVEN THE FLOOR.",
};

static void continue_screen(void) {
    gfx_cls(C_INK);
    putc_("cont", "CONTINUE?", 160, 30, C_YELLOW);
    char b[16];
    snprintf(b, sizeof b, "%d", imax(0, dku_g.cont_t / 60));
    int w = text_width_scaled(b, 3);
    text_draw_scaled(b, 160 - w / 2, 46, C_RED, 3);
    ui_choices(160, 84, "YES", "NO", dku_g.cont_sel, dku_g.frame_t, C_YELLOW, C_SLATE);
    putc_("cont note", "BACK TO THE START OF THE NIGHT. CASH AND STATS STAY.", 160, 100, C_LIGHT);
    wrapped_c("tip", DKU_TIPS[dku_g.tip % DKU_NTIPS], 160, 124, 290, C_WHITE);
}

static void over_screen(bool gym) {
    gfx_cls(C_INK);
    char b[64];
    if (gym) {
        putc_("gym over", "FINAL WHISTLE", 160, 50, C_YELLOW);
        snprintf(b, sizeof b, "WAVES BEATEN: %d", imax(0, dku_g.gym_wave - 1));
        putc_("gym beaten", b, 160, 72, C_WHITE);
        snprintf(b, sizeof b, "BEST: %d", dku_g.players == 2 ? dku_sv.gym_best2 : dku_sv.gym_best1);
        putc_("gym best", b, 160, 86, C_LIGHT);
    } else {
        putc_("over", "LAMPWICK BELONGS TO THE GHOULS.", 160, 60, C_RED);
        putc_("over2", "FOR TONIGHT.", 160, 74, C_LIGHT);
    }
}

static void ending_screen(void) {
    skyline(dku_g.frame_t);
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, 7);
    const char *head = dku_g.continues == 0 ? "GRAN IS SAFE!" : "TOO LATE...";
    static const uint8_t grad[2] = {C_YELLOW, C_ORANGE};
    ui_audit_fancy("ending head", head, 160 - ui_fancy_width(head, 2) / 2, 18, 2, true);
    ui_fancy_center(head, 160, 18, 2, grad, 2, C_INK, C_MAROON);
    const char *t = dku_g.continues == 0
        ? "GRIST IS DOWN, AND GRAN BESS WALKS OUT OF THE PENTHOUSE ON HER OWN TWO FEET. BY MONDAY THE WHOLE TOWN WANTS HER AS ALDERMAN. SHE SAYS YES, AND BAKES FOR THE SWEARING-IN."
        : "GRIST IS DOWN AND THE GHOULS GO BACK INTO THE DRAINS. BUT THE NIGHT WENT ON TOO LONG: WHAT WAITS IN THE PENTHOUSE ISN'T GRAN ANY MORE. LAMPWICK IS SAVED. ROOK GOES HOME ALONE.";
    wrapped_c("ending", t, 160, 50, 288, C_WHITE);
    char b[48];
    snprintf(b, sizeof b, "CONTINUES: %d", dku_g.continues);
    putc_("ending continues", b, 160, 140, dku_g.continues == 0 ? C_LIME : C_RED);
}

static void credits_screen(void) {
    gfx_cls(C_INK);
    static const char *const LINES[] = {
        "DUKES UP", "", "A UFO 40 CARTRIDGE", "TRIBUTE TO FIST HELL", "UFO 50 #33", "",
        "ROOK", "PIP", "MACK", "DOLLY", "", "GRAN BESS", "ALDERMAN GRIST", "",
        "BIG RAM", "THE BOAR BARON", "THE VISITORS", "THE UNDERTOW", "", "AND THE GHOULS OF LAMPWICK",
        "", "THANKS FOR PLAYING",
    };
    int y = 180 - dku_g.state_t / 3;
    for (int i = 0; i < ARRAY_LEN(LINES); i++) {
        int ly = y + i * 14;
        if (ly < -10 || ly > 180) continue;
        int w = text_width(LINES[i]);
        text_shadow(LINES[i], 160 - w / 2, ly, i == 0 ? C_YELLOW : C_WHITE, C_INK);
    }
}

void dku_draw(void) {
    switch (dku_g.state) {
    case DS_TITLE: title_screen(); break;
    case DS_STORY: story_screen(); break;
    case DS_SELECT: select_screen(); break;
    case DS_CARD: card_screen(); break;
    case DS_PLAY: draw_play(); break;
    case DS_CLEAR: clear_screen(); break;
    case DS_SHOP: shop_screen(); break;
    case DS_CONTINUE: continue_screen(); break;
    case DS_OVER: over_screen(false); break;
    case DS_GYMOVER: over_screen(true); break;
    case DS_ENDING: ending_screen(); break;
    case DS_CREDITS: credits_screen(); break;
    default: gfx_cls(C_INK); break;
    }
}

/* every text screen, checked for fit: the shop at each row and turn, the
 * continue screen with every tip, the select, story and ending screens */
int dku_layout_audit(void) {
    int bad = 0;
    static DkuGame keep;
    keep = dku_g;
    int states[] = {DS_TITLE, DS_STORY, DS_SELECT, DS_CARD, DS_SHOP, DS_CONTINUE, DS_OVER, DS_GYMOVER, DS_ENDING};
    for (int k = 0; k < ARRAY_LEN(states); k++) {
        int variants = states[k] == DS_SHOP ? 10 : states[k] == DS_CONTINUE ? DKU_NTIPS : states[k] == DS_ENDING ? 2 : states[k] == DS_SELECT ? 4 : 1;
        for (int v = 0; v < variants; v++) {
            dku_g.state = states[k];
            dku_g.players = 2;
            dku_g.night = v % DKU_NIGHTS;
            dku_g.shop_sel = v % 5;
            dku_g.shop_who = v / 5;
            dku_g.tip = v;
            dku_g.continues = v & 1;
            dku_g.sel[0] = v % 4;
            dku_g.sel[1] = (v + 1) % 4;
            dku_g.cash = 999;
            char subject[32];
            snprintf(subject, sizeof subject, "dukesup state %d/%d", states[k], v);
            ui_audit_begin(subject, true);
            dku_draw();
            bad += ui_audit_end();
        }
    }
    dku_g = keep;
    return bad;
}

/* ---- the cartridge label ---------------------------------------------------------- */

void dku_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_NIGHT);
    for (int k = 0; k < 6; k++) {
        int bx = x + k * 26, bh = 18 + (k * 11) % 16;
        gfx_rect(bx, y + h - bh - 10, 24, bh + 10, C_INK);
        gfx_pset(bx + 6, y + h - bh - 4, C_YELLOW);
    }
    gfx_rect(x, y + h - 12, w, 12, C_SLATE);
    gfx_circ(x + w - 18, y + 14, 6, C_CREAM);
    Actor a;
    memset(&a, 0, sizeof a);
    a.kind = AK_FIGHTER;
    a.face = 1;
    a.state = AS_ATTACK;
    a.atk = AT_JAB;
    a.atk_t = 4;
    a.combo = (t / 10) & 1;
    int keep = dku_g.pr[0].pick;
    dku_g.pr[0].pick = DK_ROOK;
    dku_draw_actor(&a, x + w / 2 - 14, y + h - 4);
    dku_g.pr[0].pick = keep;
    Actor g;
    memset(&g, 0, sizeof g);
    g.kind = AK_SHAMBLER;
    g.face = -1;
    g.state = (t / 10) & 1 ? AS_HURT : AS_FREE;
    dku_draw_actor(&g, x + w / 2 + 10, y + h - 4);
    static const uint8_t grad[2] = {C_YELLOW, C_AMBER};
    ui_fancy_text("DUKES UP", x + 6, y + 4, 1, grad, 2, C_INK, C_MAROON);
    gfx_noclip();
}
