/* KIPPER - drawing: the sea (sky and docks, rock by area, kelp and coral,
 * the ruins, the dark), the creatures and the big six, the console on the
 * left (radar, fuel bars, fitted equipment, the trail), the docks' screens,
 * Auntie Roe, the title, the story, the endings and the cartridge label. */
#include "kpr.h"

#define VX KPR_HUD_W                 /* the view's left edge on screen */
static int camx, camy;               /* the camera, shake included */

static int sx(float wx) { return (int)wx - camx + VX; }
static int sy(float wy) { return (int)wy - camy; }

/* text that reports itself to the layout audit */
static void txt(const char *s, int x, int y, int col) {
    text_draw(s, x, y, col);
    ui_audit_text("text", s, x, y);
}
static void txt_c(const char *s, int cx, int y, int col) { txt(s, cx - text_width(s) / 2, y, col); }
static void tiny(const char *s, int x, int y, int col) {
    tiny_draw(s, x, y, col);
    ui_audit_tiny("tiny", s, x, y);
}
static void tiny_c(const char *s, int cx, int y, int col) { tiny(s, cx - tiny_width(s) / 2, y, col); }

/* ---- the sea -------------------------------------------------------------- */

static int water_col(int wy) {
    int ty = wy / KPR_TILE;
    if (ty < KPR_SKY) return C_SKY;
    if (ty < 12) return C_BLUE;
    if (ty < KPR_AREA2) return C_NAVY;
    if (ty < KPR_AREA3) return C_NIGHT;
    return C_NAVY;
}

static void draw_backdrop(void) {
    for (int y = 0; y < KPR_VIEW_H; y++) {
        int wy = y + camy;
        int c = water_col(wy);
        gfx_hline(VX, SCREEN_W - 1, y, c);
        /* soft bands where one water colour meets the next */
        int ty = wy / KPR_TILE;
        if ((ty == 11 || ty == KPR_AREA2 - 1 || ty == KPR_AREA3 - 1) && wy % KPR_TILE >= 8)
            gfx_dither(VX, y, KPR_VIEW_W, 1, water_col(wy + KPR_TILE), (wy % KPR_TILE - 7) * 2);
    }
    /* sunbeams in the Kelpwood */
    if (camy < KPR_AREA2 * KPR_TILE) {
        for (int k = 0; k < 6; k++) {
            int bx = (k * 211 + 40) % KPR_PW;
            int x = sx((float)bx) - (camy / 3) % 40;
            for (int y = 0; y < KPR_VIEW_H; y++) {
                int wy = y + camy;
                if (wy < KPR_SKY * KPR_TILE || wy > 30 * KPR_TILE) continue;
                if (((y + kg.frame_t / 4) & 7) < 2) continue;
                int xx = x + (wy - KPR_SKY * KPR_TILE) / 4;
                if (xx >= VX && xx < SCREEN_W && ((xx + y) & 3) == 0) gfx_pset(xx, y, C_SKY);
            }
        }
    }
}

static int rock_col(int ty, int *edge, int *dark) {
    if (ty < KPR_AREA2) { *edge = C_TAN; *dark = C_EARTH; return C_BROWN; }
    if (ty < KPR_AREA3) { *edge = C_GREY; *dark = C_DUSK; return C_SLATE; }
    *edge = C_GREY; *dark = C_NIGHT; return C_DUSK;
}

static bool solidish(int c) { return c == '#' || c == 'P' || c == '?' || c == 'X' || (c >= 'a' && c <= 'h'); }

static void draw_rock(int px, int py, int tx, int ty, int c) {
    int edge, dark;
    int base = rock_col(ty, &edge, &dark);
    if (c == 'P') { base = C_GREY; edge = C_LIGHT; dark = C_SLATE; }
    gfx_rect(px, py, KPR_TILE, KPR_TILE, base);
    if (c == 'P') {
        /* hewn stone: courses of blocks */
        gfx_hline(px, px + 15, py + 7, dark);
        gfx_hline(px, px + 15, py + 15, dark);
        gfx_vline(px + ((ty & 1) ? 4 : 11), py, py + 6, dark);
        gfx_vline(px + ((ty & 1) ? 11 : 4), py + 8, py + 14, dark);
    } else {
        uint32_t h = (uint32_t)(tx * 73856093u) ^ (uint32_t)(ty * 19349663u);
        for (int k = 0; k < 3; k++) gfx_pset(px + (int)((h >> (k * 4)) & 15), py + (int)((h >> (k * 4 + 12)) & 15), dark);
    }
    /* edges where the rock meets water */
    if (!solidish(kpr_tile(tx, ty - 1)) && kpr_tile(tx, ty - 1) != 'B' && kpr_tile(tx, ty - 1) != 'd') {
        gfx_hline(px, px + 15, py, edge);
        if (ty < KPR_AREA2 && kpr_tile(tx, ty - 1) != '^') gfx_hline(px, px + 15, py + 1, C_JADE);
        if (kpr_tile(tx, ty - 1) == '^') { gfx_hline(px, px + 15, py, C_LEAF); gfx_hline(px, px + 15, py + 1, C_FOREST); }
    }
    if (!solidish(kpr_tile(tx, ty + 1)) && kpr_tile(tx, ty + 1) != 'B' && kpr_tile(tx, ty + 1) != 'd') gfx_hline(px, px + 15, py + 15, dark);
    if (!solidish(kpr_tile(tx - 1, ty)) && kpr_tile(tx - 1, ty) != 'B' && kpr_tile(tx - 1, ty) != 'd') gfx_vline(px, py, py + 15, edge);
    if (!solidish(kpr_tile(tx + 1, ty)) && kpr_tile(tx + 1, ty) != 'B' && kpr_tile(tx + 1, ty) != 'd') gfx_vline(px + 15, py, py + 15, dark);
    if (c == '?') {
        /* a crack to the sharp-eyed */
        gfx_line(px + 3, py + 4, px + 7, py + 8, dark);
        gfx_line(px + 7, py + 8, px + 6, py + 12, dark);
        gfx_line(px + 7, py + 8, px + 12, py + 9, dark);
    }
    if (c == 'X') gfx_pset(px + 8, py + 8, edge);    /* only a speck gives it away */
    if (c >= 'a' && c <= 'h') {
        bool lit = (kg.gates >> (c - 'a')) & 1;
        gfx_circ(px + 8, py + 8, 5, C_INK);
        gfx_circ(px + 8, py + 8, 4, lit ? C_YELLOW : C_BLUE);
        gfx_circ(px + 7, py + 7, 1, lit ? C_WHITE : C_SKY);
    }
}

static void draw_tiles(void) {
    int tx0 = camx / KPR_TILE, ty0 = camy / KPR_TILE;
    for (int ty = ty0; ty <= ty0 + KPR_VIEW_H / KPR_TILE + 1; ty++)
        for (int tx = tx0; tx <= tx0 + KPR_VIEW_W / KPR_TILE + 1; tx++) {
            if (tx < 0 || ty < 0 || tx >= KPR_W || ty >= KPR_H) continue;
            int c = kg.tile[ty][tx];
            int px = tx * KPR_TILE - camx + VX, py = ty * KPR_TILE - camy;
            switch (c) {
            case '^': break;
            case '.': break;
            case 'k': {
                /* kelp, swaying */
                int sw = (int)(sinf((kg.frame_t + tx * 20) * 0.04f) * 2.0f);
                for (int y = 0; y < 16; y += 2) gfx_rect(px + 7 + (y < 8 ? sw : sw / 2), py + y, 2, 2, (y & 2) ? C_JADE : C_FOREST);
                break;
            }
            case 'c':
                if (kpr_solid_tile(tx, ty + 1)) {
                    gfx_rect(px + 3, py + 10, 2, 6, C_PINK);
                    gfx_rect(px + 1, py + 9, 6, 2, C_PINK);
                    gfx_rect(px + 10, py + 12, 3, 4, C_ORANGE);
                    gfx_pset(px + 11, py + 11, C_AMBER);
                }
                break;
            case 'B':
                gfx_rect(px, py, 16, 16, C_BLUE);
                gfx_rectb(px, py, 16, 16, C_NAVY);
                gfx_hline(px + 1, px + 14, py + 1, C_SKY);
                gfx_vline(px + 1, py + 1, py + 14, C_SKY);
                gfx_line(px + 4, py + 4, px + 11, py + 11, C_NAVY);
                gfx_line(px + 11, py + 4, px + 4, py + 11, C_NAVY);
                break;
            case 'd':
                gfx_rect(px, py, 16, 16, C_BROWN);
                for (int k = 0; k < 6; k++) gfx_pset(px + (k * 5 + tx) % 15, py + (k * 7 + ty) % 15, C_TAN);
                gfx_pset(px + 4, py + 11, C_EARTH);
                gfx_pset(px + 12, py + 3, C_EARTH);
                break;
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
                if (!((kg.gates >> (c - '0')) & 1)) {
                    gfx_rect(px, py, 16, 16, C_MAROON);
                    for (int k = 2; k < 16; k += 5) gfx_vline(px + k, py, py + 15, C_RED);
                    gfx_hline(px, px + 15, py, C_ORANGE);
                } else {
                    gfx_vline(px, py, py + 15, C_MAROON);
                    gfx_vline(px + 15, py, py + 15, C_MAROON);
                }
                break;
            default: draw_rock(px, py, tx, ty, c); break;
            }
        }
}

static void draw_surface(void) {
    int wy = KPR_SKY * KPR_TILE;
    int y = sy((float)wy);
    if (y < -30 || y > KPR_VIEW_H + 4) return;
    /* the sky: clouds over the Sound */
    for (int k = 0; k < 12; k++) {
        int cx = sx((float)((k * 263 + kg.frame_t / 8) % KPR_PW)), cy = y - 30 + (k * 7) % 14;
        gfx_circ(cx, cy, 6, C_WHITE);
        gfx_circ(cx + 7, cy + 1, 5, C_WHITE);
        gfx_circ(cx - 6, cy + 2, 4, C_ICE);
    }
    /* wavelets along the water line */
    for (int x = VX; x < SCREEN_W; x++) {
        int w = (int)(sinf((x + camx + kg.frame_t) * 0.15f) * 1.5f);
        int wx = x - VX + camx;
        int tx = wx / KPR_TILE;
        if (tx < 0 || tx >= KPR_W || kpr_tile(tx, KPR_SKY) == '#') continue;
        gfx_vline(x, y - 2 + w, y, C_ICE);
    }
    /* the Sludgeworks on the cliff west of the Pier, its pipe in the bay */
    int fx = sx(57 * KPR_TILE), fy = y;
    gfx_rect(fx, fy - 26, 40, 26, C_SLATE);
    gfx_rect(fx + 4, fy - 40, 6, 14, C_GREY);
    gfx_rect(fx + 16, fy - 46, 6, 20, C_GREY);
    for (int k = 0; k < 4; k++) gfx_rect(fx + 4 + k * 9, fy - 18, 5, 5, C_AMBER);
    for (int k = 0; k < 3; k++) {
        int s = (kg.frame_t / 6 + k * 9) % 26;
        gfx_dither_circle(fx + 19 + s / 3, fy - 48 - s, 3 + s / 8, C_LIGHT, 8);
        gfx_dither_circle(fx + 7 + s / 4, fy - 42 - s, 2 + s / 9, C_LIGHT, 8);
    }
    int px = sx(69 * KPR_TILE);
    gfx_rect(fx + 40, fy - 10, px - fx - 36, 4, C_GREY);
    for (int k = 0; k < 10; k++) {
        int dy = (kg.frame_t * 2 + k * 7) % 24;
        gfx_pset(px + 4 + (k & 1), fy - 6 + dy, (k & 1) ? C_LIME : C_LEAF);
    }
    /* the docks */
    for (int d = 0; d < KPR_NDOCKS; d++) {
        int dx = sx(KPR_DOCKS[d].x * KPR_TILE + 8.0f);
        gfx_rect(dx - 26, y - 12, 52, 5, C_BROWN);
        gfx_hline(dx - 26, dx + 25, y - 12, C_TAN);
        for (int k = -24; k <= 22; k += 15) gfx_rect(dx + k, y - 7, 3, 10, C_EARTH);
        /* the berth: a ring the sub noses into */
        gfx_circb(dx, y - 1, 7, C_YELLOW);
        gfx_circb(dx, y - 1, 6, C_AMBER);
        if (d == 0) {
            /* the Pier's shed */
            gfx_rect(dx + 6, y - 24, 16, 12, C_RED);
            gfx_rect(dx + 4, y - 27, 20, 3, C_MAROON);
            gfx_rect(dx + 12, y - 19, 4, 7, C_INK);
        } else {
            /* the Lighthouse */
            gfx_rect(dx + 14, y - 52, 9, 40, C_WHITE);
            for (int k = 0; k < 4; k++) gfx_rect(dx + 14, y - 48 + k * 10, 9, 4, C_RED);
            gfx_rect(dx + 12, y - 58, 13, 6, C_INK);
            if ((kg.frame_t / 20) & 1) gfx_rect(dx + 14, y - 57, 9, 4, C_YELLOW);
        }
    }
}

static void draw_vents(void) {
    for (int v = 0; v < KPR_NVENTS; v++) {
        int x = sx(KPR_VENTS[v].x * KPR_TILE + 8.0f), y = sy((KPR_VENTS[v].y + 1) * KPR_TILE);
        if (x < VX - 10 || x > SCREEN_W + 10 || y < -60 || y > KPR_VIEW_H + 10) continue;
        gfx_rect(x - 6, y - 4, 12, 4, C_DUSK);
        gfx_rect(x - 3, y - 6, 6, 2, C_SLATE);
        int ph = (kg.frame_t + v * 37) % 180;
        if (ph >= 100 && ph < 120) gfx_dither_circle(x, y - 8, 3, C_GREY, 6);
        if (kpr_vent_on(v))
            for (int k = 0; k < 7; k++) {
                int yy = y - 8 - k * 7 - (kg.frame_t % 7);
                gfx_dither_circle(x + (k & 1 ? 2 : -2), yy, 4 + k / 3, k < 3 ? C_WHITE : C_ICE, 10);
            }
    }
}

/* ---- the creatures ----------------------------------------------------------------- */

static void spr_at(int id, float wx, float wy, int flags) {
    const Sprite *s = &kpr_spr[id];
    spr_draw(s, sx(wx) - s->w / 2, sy(wy) - s->h / 2, flags);
}

static bool on_view(float wx, float wy, int m) {
    int x = sx(wx), y = sy(wy);
    return x > VX - m && x < SCREEN_W + m && y > -m && y < KPR_VIEW_H + m;
}

static void draw_foes(void) {
    uint8_t red[256];
    pal_identity(red);
    pal_swap(red, C_SKY, C_RED);
    pal_swap(red, C_INK, C_MAROON);
    for (int i = 0; i < KPR_MAX_FOES; i++) {
        const KprFoe *f = &kg.foe[i];
        if (!f->alive || !on_view(f->x, f->y, 24)) continue;
        int fl = f->dir < 0 ? SPR_FLIPX : 0;
        if (f->pacified) { spr_at(SPR_PACIFIED, f->x, f->y, 0); continue; }
        if (f->flash & 2) continue;
        switch (f->kind) {
        case FK_POLYP: spr_at(SPR_POLYP, f->x, f->y, 0); break;
        case FK_FILCHER:
            spr_at(SPR_FILCHER, f->x, f->y, f->vx < 0 ? SPR_FLIPX : 0);
            if (f->carry >= 0) gfx_circ(sx(f->x), sy(f->y) - 6, 2, C_YELLOW);
            break;
        case FK_GLIMMER:
            if (f->state > 0 || f->flash) spr_at(SPR_GLIMMER, f->x, f->y, fl);
            else if ((kg.frame_t + i) % 23 == 0) gfx_pset(sx(f->x), sy(f->y), C_SKY);   /* a shimmer */
            break;
        case FK_GREENLING: spr_at(SPR_GREENLING, f->x, f->y, fl); break;
        case FK_SHELLBACK: spr_at(f->state == 0 ? SPR_SHELLBACK : SPR_SHELL, f->x, f->y, fl); break;
        case FK_KITERAY: spr_at(SPR_KITERAY, f->x, f->y, fl); break;
        case FK_BLUEBELL: spr_at(SPR_JELLY, f->x, f->y + sinf(f->t * 0.1f), 0); break;
        case FK_REDCAP: {
            const Sprite *s = &kpr_spr[SPR_JELLY];
            spr_draw_ex(s, sx(f->x) - s->w / 2, sy(f->y) - s->h / 2, 0, red, -1);
            break;
        }
        case FK_LANTERNMAW: spr_at(SPR_LANTERN, f->x, f->y, fl); break;
        case FK_RISER: spr_at(SPR_RISER, f->x, f->y, 0); break;
        case FK_PIPEHORSE: spr_at(SPR_PIPEHORSE, f->x, f->y, fl); break;
        case FK_BLOATER:
            if (f->state == 1) {
                int x = sx(f->x), y = sy(f->y);
                for (int k = 0; k < 12; k++) {
                    float a = k * 0.5236f;
                    gfx_line(x, y, x + (int)(cosf(a) * 13), y + (int)(sinf(a) * 13), C_INK);
                }
                gfx_circ(x, y, 10, C_INK);
                gfx_circ(x, y, 9, C_TAN);
                gfx_circ(x + 3, y - 2, 2, C_WHITE);
                gfx_pset(x + 3, y - 2, C_INK);
            } else spr_at(SPR_BLOATER, f->x, f->y, fl);
            break;
        case FK_STILTCRAB: spr_at(SPR_STILT, f->x, f->y, 0); break;
        case FK_SUNSTAR: spr_at(SPR_SUNSTAR, f->x, f->y, 0); break;
        case FK_BILLFISH: spr_at(SPR_BILLFISH, f->x, f->y, fl); break;
        case FK_BURR: spr_at(f->state ? SPR_BURR_ANGRY : SPR_BURR, f->x, f->y, 0); break;
        case FK_SNAPWORM:
            if (f->state == 1) {
                int x = sx(f->hx), y = sy(f->hy);
                gfx_pset(x - 2, y, C_RED);
                gfx_pset(x + 2, y, C_RED);
            } else if (f->state >= 2) {
                int x0 = sx(f->hx), y0 = sy(f->hy), x1 = sx(f->x), y1 = sy(f->y);
                for (int k = 0; k <= 4; k++) gfx_circ(x0 + (x1 - x0) * k / 4, y0 + (y1 - y0) * k / 4, 4, k & 1 ? C_WINE : C_MAROON);
                spr_at(SPR_WORM, f->x, f->y, 0);
            }
            break;
        case FK_FIZZWORM: {
            float l = sqrtf(f->vx * f->vx + f->vy * f->vy);
            float ux = l > 0.01f ? f->vx / l : 1, uy = l > 0.01f ? f->vy / l : 0;
            for (int k = 5; k >= 0; k--) {
                int x = sx(f->x - ux * k * 7 + sinf(f->t * 0.15f + k) * 2), y = sy(f->y - uy * k * 7 + cosf(f->t * 0.15f + k) * 2);
                gfx_circb(x, y, k ? 4 - k / 2 : 5, k ? C_SKY : C_WHITE);
                if (!k) { gfx_pset(x - 1, y - 1, C_RED); gfx_pset(x + 1, y - 1, C_RED); }
            }
            break;
        }
        default: break;
        }
    }
}

/* ---- the big six --------------------------------------------------------------------- */

static void boss_shark(const KprBoss *o, int x, int y) {
    int d = o->dir;
    gfx_circ(x, y, 9, C_GREY);
    for (int k = -14; k <= 14; k++) {
        int hh = 9 - iabs(k) * 9 / 18;
        gfx_vline(x + k, y - hh, y + hh, C_GREY);
        gfx_vline(x + k, y + 2, y + hh, C_LIGHT);
    }
    gfx_line(x - d * 2, y - 8, x - d * 8, y - 16, C_SLATE);   /* the fin */
    gfx_line(x - d * 3, y - 8, x - d * 8, y - 15, C_SLATE);
    gfx_line(x - d * 8, y - 16, x - d * 10, y - 8, C_SLATE);
    gfx_line(x - d * 15, y, x - d * 22, y - 8, C_GREY);         /* the tail */
    gfx_line(x - d * 15, y, x - d * 22, y + 7, C_GREY);
    gfx_line(x - d * 22, y - 8, x - d * 22, y + 7, C_SLATE);
    gfx_circ(x + d * 9, y - 3, 2, C_WHITE);
    gfx_pset(x + d * 10, y - 3, C_INK);
    for (int k = 0; k < 4; k++) gfx_pset(x + d * (6 + k * 2), y + 4, C_WHITE);
}

static void boss_octopus(const KprBoss *o, int x, int y) {
    for (int k = 0; k < 6; k++) {
        int px = x - 12 + k * 5;
        for (int s = 0; s < 6; s++) {
            int wy = y + 8 + s * 3;
            int wx = px + (int)(sinf(o->t * 0.08f + k + s * 0.6f) * 2);
            gfx_circ(wx, wy, 2 - s / 3, C_VIOLET);
        }
    }
    gfx_circ(x, y - 2, 12, C_PURPLE);
    gfx_circ(x - 3, y - 6, 4, C_VIOLET);
    gfx_circ(x - 5, y, 3, C_WHITE);
    gfx_circ(x + 5, y, 3, C_WHITE);
    gfx_pset(x - 5 + o->dir, y, C_INK);
    gfx_pset(x + 5 + o->dir, y, C_INK);
    /* an arm out of the wall */
    if (o->tent_t > 0) {
        int tx0 = sx(o->tx), ty0 = sy(o->ty);
        float len = o->tent_t < 40 ? 6.0f + sinf(o->tent_t * 0.6f) * 3 : fminf(1.0f, (o->tent_t - 40) / 10.0f) * 44.0f;
        if (o->tent_t >= 90) len = 0;
        for (int k = 0; k <= 6; k++) {
            float f = len * k / 6;
            gfx_circ(tx0 + (int)(cosf(o->aang) * f), ty0 + (int)(sinf(o->aang) * f), 4 - k / 2, k & 1 ? C_VIOLET : C_PURPLE);
        }
    }
}

static void boss_eel(const KprBoss *o) {
    for (int k = 9; k >= 1; k--) {
        int x = sx(o->sx[k]), y = sy(o->sy[k]);
        gfx_circ(x, y, 6 - k / 3, k & 1 ? C_FOREST : C_JADE);
        gfx_pset(x, y - 2, C_LIME);
    }
    int x = sx(o->x), y = sy(o->y), d = o->dir;
    gfx_circ(x, y, 7, C_JADE);
    gfx_rect(x - 2, y - 5, 12, 10, C_JADE);
    gfx_line(x + d * 4, y + 2, x + d * 11, y + 4, C_INK);
    gfx_circ(x + d * 3, y - 3, 2, C_YELLOW);
    gfx_pset(x + d * 3, y - 3, C_INK);
}

static void boss_nautilus(const KprBoss *o, int x, int y) {
    for (int k = 0; k < 5; k++) {
        float a = o->t * 0.05f + k;
        gfx_line(x - 10, y + 6, x - 18 + (int)(cosf(a) * 4), y + 12 + k * 2, C_TAN);
    }
    gfx_circ(x, y, 15, C_CREAM);
    for (int k = 0; k < 6; k++) {
        float a = k * 1.047f + o->t * 0.02f;
        gfx_line(x, y, x + (int)(cosf(a) * 14), y + (int)(sinf(a) * 14), C_ORANGE);
    }
    gfx_circb(x, y, 15, C_BROWN);
    gfx_circb(x + 2, y + 1, 9, C_BROWN);
    gfx_circb(x + 3, y + 2, 4, C_BROWN);
    gfx_circ(x - 12, y + 2, 3, C_WHITE);
    gfx_pset(x - 13, y + 2, C_INK);
}

static void boss_squid(const KprBoss *o, int x, int y) {
    if (o->state == 3) {
        /* the cape turned inside out: a ball of spines */
        for (int k = 0; k < 16; k++) {
            float a = k * 0.3927f + o->t * 0.05f;
            gfx_line(x, y, x + (int)(cosf(a) * 20), y + (int)(sinf(a) * 20), C_WINE);
        }
        gfx_circ(x, y, 13, C_MAROON);
        gfx_circ(x, y, 9, C_INK);
        return;
    }
    gfx_circ(x, y - 8, 11, C_WINE);
    gfx_rect(x - 11, y - 8, 23, 14, C_WINE);
    for (int k = 0; k < 8; k++) {
        int px = x - 14 + k * 4;
        int len = 10 + (int)(sinf(o->t * 0.1f + k) * 3);
        gfx_line(x, y + 4, px, y + 6 + len, C_MAROON);
        gfx_pset(px, y + 6 + len, C_PINK);
    }
    gfx_circ(x - 4, y - 6, 3, C_RED);
    gfx_circ(x + 4, y - 6, 3, C_RED);
    gfx_pset(x - 4, y - 6, C_YELLOW);
    gfx_pset(x + 4, y - 6, C_YELLOW);
}

static void boss_alien(const KprBoss *o, int x, int y) {
    /* the laser first, under it */
    if (o->beam_t) {
        float lx = o->ax - o->x, ly = o->ay - o->y, l = sqrtf(lx * lx + ly * ly);
        if (l > 1) {
            lx /= l;
            ly /= l;
            int ex = x + (int)(lx * 320), ey = y + (int)(ly * 320);
            if (o->beam_t == 1) {
                for (int k = 0; k < 320; k += 6) gfx_pset(x + (int)(lx * k), y + (int)(ly * k), C_RED);
            } else {
                gfx_line(x, y - 1, ex, ey - 1, C_MAGENTA);
                gfx_line(x, y, ex, ey, C_WHITE);
                gfx_line(x, y + 1, ex, ey + 1, C_MAGENTA);
            }
        }
    }
    int r = 20 + (int)(sinf(o->t * 0.08f) * 2);
    for (int k = 0; k < 10; k++) {
        float a = k * 0.628f + o->aang;
        gfx_line(x, y, x + (int)(cosf(a) * (r + 8)), y + (int)(sinf(a) * (r + 8)), C_PURPLE);
    }
    gfx_circ(x, y, r, C_VIOLET);
    gfx_circ(x - 4, y - 5, r / 2, C_MAGENTA);
    static const int8_t EYE[7][2] = {{0, 0}, {-9, -6}, {9, -6}, {-12, 5}, {12, 5}, {-5, 11}, {6, 12}};
    for (int k = 0; k < 7; k++) {
        int ex = x + EYE[k][0], ey = y + EYE[k][1];
        gfx_circ(ex, ey, k ? 2 : 5, C_WHITE);
        int px = isign((int)kg.x - (int)o->x), py = isign((int)kg.y - (int)o->y);
        gfx_circ(ex + px, ey + py, k ? 1 : 2, C_INK);
    }
}

static void draw_bosses(void) {
    for (int b = 0; b < KPR_BOSSES; b++) {
        const KprBoss *o = &kg.boss[b];
        if (!o->on) continue;
        if (b == BS_INKWELL && o->tent_t > 0) boss_octopus(o, -999, -999);
        if (!on_view(o->x, o->y, 60) && b != BS_LONGJAW) continue;
        if (o->flash & 2) continue;
        int x = sx(o->x), y = sy(o->y);
        switch (b) {
        case BS_GNASHER: boss_shark(o, x, y); break;
        case BS_INKWELL: boss_octopus(o, x, y); break;
        case BS_LONGJAW: boss_eel(o); break;
        case BS_WHORL: boss_nautilus(o, x, y); break;
        case BS_NIGHTCAPE: boss_squid(o, x, y); break;
        case BS_XOLLOTH: boss_alien(o, x, y); break;
        default: break;
        }
    }
}

/* ---- the sub, its trail, the pick-ups, shots ------------------------------------------ */

static int item_sprite(int i) {
    const KprItemDef *d = &KPR_ITEMS[i];
    switch (d->kind) {
    case IK_TANK: return SPR_TANK;
    case IK_BOOSTER: return SPR_BOOST;
    case IK_EGG: return SPR_EGG;
    default: return SPR_EQ0 + d->var;
    }
}

static void draw_item_at(int i, float wx, float wy) {
    int id = item_sprite(i);
    if (KPR_ITEMS[i].kind == IK_EQUIP) gfx_dither_circle(sx(wx), sy(wy), 10, C_YELLOW, 4 + (kg.frame_t / 8) % 3 * 2);
    spr_at(id, wx, wy, 0);
}

static void draw_items(void) {
    for (int i = 0; i < KPR_NITEMS; i++) {
        if (kg.cur.item[i] != IS_HOME) continue;
        float bob = sinf((kg.frame_t + i * 11) * 0.06f) * 1.5f;
        if (kg.where[i] == IW_HOME) {
            float x = KPR_ITEMS[i].x * KPR_TILE + 8.0f, y = KPR_ITEMS[i].y * KPR_TILE + 8.0f + bob;
            if (on_view(x, y, 16)) draw_item_at(i, x, y);
        } else if (kg.where[i] == IW_LOOSE) {
            if (on_view(kg.ix[i], kg.iy[i], 16)) draw_item_at(i, kg.ix[i], kg.iy[i] + bob);
        } else if (kg.where[i] == IW_HELD) {
            for (int f = 0; f < KPR_MAX_FOES; f++)
                if (kg.foe[f].alive && kg.foe[f].carry == i) draw_item_at(i, kg.foe[f].x, kg.foe[f].y - 8);
        }
    }
    for (int i = 0; i < KPR_MAX_BOMBS; i++) {
        const KprBomb *b = &kg.bomb[i];
        if (!b->alive) continue;
        if (b->fuse < 40 && (b->fuse / 4) & 1) gfx_circ(sx(b->x), sy(b->y), 6, C_RED);
        spr_at(SPR_CHARGE, b->x, b->y, 0);
    }
    for (int i = 0; i < KPR_PLATES; i++)
        if (kg.plate[i].alive) spr_at(SPR_PLATE, kg.plate[i].x, kg.plate[i].y, 0);
    for (int i = 0; i < 8; i++)
        if (kg.fuel_drop_t[i] > 0 && (kg.fuel_drop_t[i] > 120 || (kg.fuel_drop_t[i] / 6) & 1)) {
            int x = sx(kg.fuel_drop_x[i]), y = sy(kg.fuel_drop_y[i]);
            gfx_circ(x, y + 1, 3, C_ORANGE);
            gfx_rect(x - 1, y - 4, 2, 3, C_ORANGE);
            gfx_pset(x - 1, y, C_YELLOW);
        }
    /* Auntie Roe at her grotto */
    float rx = KPR_ROE.x * KPR_TILE + 8.0f, ry = KPR_ROE.y * KPR_TILE + 6.0f + sinf(kg.frame_t * 0.05f) * 1.5f;
    if (on_view(rx, ry, 30)) {
        spr_at(SPR_ROE, rx, ry, 0);
        for (int k = 0; k < imin(kg.cur.eggs_back, 20); k++)
            gfx_circ(sx(rx) - 22 + (k % 10) * 4, sy(ry) + 14 - (k / 10) * 3, 1, C_PINK);
    }
}

static void draw_sub(void) {
    /* the trail first: what it tows, on a line */
    float px = kg.x, py = kg.y;
    for (int s = 0; s < kg.trail_n; s++) {
        float tx, ty;
        kpr_trail_get(s, &tx, &ty);
        for (int k = 1; k < 4; k++) gfx_pset(sx(px + (tx - px) * k / 4), sy(py + (ty - py) * k / 4), C_LIGHT);
        if (kg.trail[s].kind == TR_CHARGE) spr_at(SPR_CHARGE, tx, ty, 0);
        else draw_item_at(kg.trail[s].id, tx, ty);
        px = tx;
        py = ty;
    }
    int n = kpr_companions();
    for (int k = 0; k < n; k++) {
        float cx, cy;
        kpr_companion_pos(k, &cx, &cy);
        spr_at(k % 2 == 0 ? SPR_FISH_BLUE : SPR_FISH_RED, cx, cy, kg.face < 0 ? SPR_FLIPX : 0);
    }
    if (kg.inv > 0 && kg.inv < KPR_MERCY - 2 && (kg.inv / 3) & 1) return;
    int id = (kg.moving && (kg.frame_t / 4) & 1) ? SPR_SUB1 : SPR_SUB0;
    const Sprite *s = &kpr_spr[id];
    int x = sx(kg.x) - s->w / 2, y = sy(kg.y) - s->h / 2 - 1;
    if (kg.plates > 0) spr_draw_outline(s, x, y, kg.face < 0 ? SPR_FLIPX : 0, kg.plates >= 3 ? C_LIGHT : C_GREY);
    else spr_draw(s, x, y, kg.face < 0 ? SPR_FLIPX : 0);
    if (kg.missile_ready) gfx_rect(sx(kg.x) - 1, y - 4, 3, 3, (kg.frame_t / 4) & 1 ? C_RED : C_YELLOW);
    if (kg.dash_t > 0 || kg.boosting)
        for (int k = 0; k < 3; k++) gfx_pset(sx(kg.x - kg.dash_dx * (10 + k * 4)), sy(kg.y - kg.dash_dy * (10 + k * 4)) + (k & 1), C_ICE);
}

static void draw_shots(void) {
    for (int i = 0; i < KPR_MAX_SHOTS; i++) {
        const KprShot *s = &kg.shot[i];
        if (!s->alive) continue;
        int x = sx(s->x), y = sy(s->y);
        switch (s->kind) {
        case SH_TORP:
            gfx_rect(x - 3, y - 1, 6, 2, C_LIGHT);
            gfx_pset(x + (s->vx > 0 ? 2 : -3), y - 1, C_WHITE);
            gfx_pset(x - (s->vx > 0 ? 5 : -5), y, C_ICE);
            break;
        case SH_BUSTER:
            gfx_rect(x - 3, y - 2, 7, 4, C_RED);
            gfx_rect(x - 2, y - 1, 5, 2, C_ORANGE);
            break;
        case SH_BUBBLE: gfx_circb(x, y, 2, C_ICE); break;
        case SH_MISSILE:
            gfx_rect(x - 1, y - 5, 3, 9, C_LIGHT);
            gfx_pset(x, y - 6, C_RED);
            gfx_rect(x - 1, y + 4, 3, 2 + (kg.frame_t & 1), C_ORANGE);
            break;
        default: break;
        }
    }
    for (int i = 0; i < KPR_MAX_ESHOTS; i++) {
        const KprEShot *e = &kg.es[i];
        if (!e->alive) continue;
        int x = sx(e->x), y = sy(e->y);
        switch (e->kind) {
        case ES_CLOUD: gfx_dither_circle(x, y, 6, C_LIGHT, 7); break;
        case ES_INK: gfx_circ(x, y, 3, C_INK); gfx_pset(x - 1, y - 1, C_DUSK); break;
        case ES_SPINE: gfx_line(x - (int)(e->vx * 2), y - (int)(e->vy * 2), x + (int)(e->vx * 2), y + (int)(e->vy * 2), C_PINK); break;
        case ES_SPORE: gfx_circ(x, y, 3, C_LIME); gfx_pset(x, y, C_WHITE); break;
        default: gfx_circ(x, y, 2, C_ORANGE); gfx_pset(x, y, C_YELLOW); break;
        }
    }
    for (int i = 0; i < KPR_MAX_PARTS; i++) {
        const KprPart *p = &kg.part[i];
        if (p->life > 0) gfx_pset(sx(p->x), sy(p->y), p->col);
    }
}

/* the Blackwater: black past the lamp's reach; the lures and eyes glow
 * through it */
static void draw_dark(void) {
    int top = KPR_AREA3 * KPR_TILE - camy;
    if (top >= KPR_VIEW_H) return;
    if (top < 0) top = 0;
    bool lamp = kpr_on(EQ_SPOT);
    int r = lamp ? 72 : 14, ring = lamp ? 18 : 8;
    int cx = sx(kg.x), cy = sy(kg.y);
    for (int y = top; y < KPR_VIEW_H; y++) {
        int dy = y - cy;
        for (int x = VX; x < SCREEN_W; x++) {
            int dx = x - cx, d2 = dx * dx + dy * dy;
            if (d2 <= r * r) continue;
            if (d2 <= (r + ring) * (r + ring)) {
                int k = (int)sqrtf((float)d2) - r;
                if (((x + y) & 1) == 0 && k < ring / 2) continue;
                if (((x ^ y) & 3) == 0 && k < ring) continue;
            }
            gfx_pset(x, y, C_INK);
        }
    }
    for (int e = 0; e < KPR_NEYES; e++) {
        int ex = sx(KPR_EYES[e].x * KPR_TILE + 8.0f), ey = sy(KPR_EYES[e].y * KPR_TILE + 8.0f);
        if (ex < VX || ex > SCREEN_W || ey < top || ey > KPR_VIEW_H) continue;
        if ((kg.frame_t / 6 + e * 17) % 60 < 3) continue;   /* a blink */
        gfx_rect(ex - 4, ey, 2, 2, C_RED);
        gfx_rect(ex + 3, ey, 2, 2, C_RED);
    }
    for (int i = 0; i < KPR_MAX_FOES; i++) {
        const KprFoe *f = &kg.foe[i];
        if (!f->alive || f->kind != FK_LANTERNMAW || f->pacified) continue;
        int x = sx(f->x + f->dir * 3), y = sy(f->y - 9);
        if (y < top) continue;
        gfx_dither_circle(x, y, 5, C_YELLOW, 6);
        gfx_circ(x, y, 1, C_YELLOW);
    }
    for (int v = 0; v < KPR_NVENTS; v++) {
        int x = sx(KPR_VENTS[v].x * KPR_TILE + 8.0f), y = sy((KPR_VENTS[v].y + 1) * KPR_TILE);
        if (y >= top && x > VX && x < SCREEN_W && y < KPR_VIEW_H + 4) gfx_rect(x - 2, y - 5, 4, 2, kpr_vent_on(v) ? C_ORANGE : C_MAROON);
    }
    /* Xolloth glows in the dark */
    const KprBoss *o = &kg.boss[BS_XOLLOTH];
    if (o->on && !(o->flash & 2) && on_view(o->x, o->y, 60)) boss_alien(o, sx(o->x), sy(o->y));
}

/* ---- the console on the left ------------------------------------------------------------ */

static void draw_hud(void) {
    gfx_rect(0, 0, KPR_HUD_W, SCREEN_H, C_NIGHT);
    gfx_vline(KPR_HUD_W - 1, 0, SCREEN_H - 1, C_SLATE);
    ui_audit_area("console", 0, 0, KPR_HUD_W - 1, SCREEN_H);
    /* the radar */
    int rx = 32, ry = 28, rr = 24;
    gfx_circ(rx, ry, rr + 1, C_SLATE);
    gfx_circ(rx, ry, rr, C_INK);
    gfx_dither_circle(rx, ry, rr, C_FOREST, 4);
    gfx_circb(rx, ry, rr / 2, C_FOREST);
    float sw = kg.frame_t * 0.05f;
    gfx_line(rx, ry, rx + (int)(cosf(sw) * rr), ry + (int)(sinf(sw) * rr), C_JADE);
    const float range = 14.0f * KPR_TILE;
    bool sonar = kpr_on(EQ_RADAR);
    if (sonar)
        for (int i = 0; i < KPR_NITEMS; i++) {
            if (kg.cur.item[i] != IS_HOME || kg.where[i] != IW_HOME) continue;
            float dx = KPR_ITEMS[i].x * KPR_TILE + 8.0f - kg.x, dy = KPR_ITEMS[i].y * KPR_TILE + 8.0f - kg.y;
            if (fabsf(dx) > range || fabsf(dy) > range || dx * dx + dy * dy > range * range) continue;
            gfx_pset(rx + (int)(dx / range * rr), ry + (int)(dy / range * rr), C_YELLOW);
        }
    for (int b = 0; b < KPR_BOSSES; b++) {
        const KprBoss *o = &kg.boss[b];
        if (!o->on || o->dying) continue;
        float dx = o->x - kg.x, dy = o->y - kg.y, d = sqrtf(dx * dx + dy * dy);
        if (d <= range) gfx_rect(rx + (int)(dx / range * rr) - 1, ry + (int)(dy / range * rr) - 1, 3, 3, C_RED);
        else if (sonar && kpr_area_of(o->y) == kpr_area_of(kg.y))
            gfx_rect(rx + (int)(dx / d * (rr - 1)) - 1, ry + (int)(dy / d * (rr - 1)) - 1, 3, 3, (kg.frame_t / 8) & 1 ? C_RED : C_MAROON);
    }
    gfx_rect(rx - 1, ry - 1, 3, 3, C_LIME);
    /* the fuel: a bar a tank */
    tiny("FUEL", 6, 56, C_LIGHT);
    int bars = kpr_bars_max();
    int full = kg.fuel / KPR_BAR, part = kg.fuel % KPR_BAR;
    int col = full >= 3 ? C_LIME : full >= 1 ? C_YELLOW : C_RED;
    for (int i = 0; i < bars; i++) {
        int bx = 6 + (i % 12) * 4, by = 63 + (i / 12) * 11;
        gfx_rect(bx, by, 3, 9, C_DUSK);
        if (i < full) gfx_rect(bx, by, 3, 9, col);
        else if (i == full && part > 0) {
            int h = part * 9 / KPR_BAR;
            gfx_rect(bx, by + 9 - h, 3, h, col);
        }
    }
    /* the fitted equipment */
    tiny("FITTED", 6, 88, C_LIGHT);
    int slots = kpr_slots(), k = 0;
    for (int s = 0; s < 4; s++) {
        int bx = 4 + s * 14, by = 95;
        gfx_rectb(bx, by, 14, 14, s < slots ? C_SLATE : C_DUSK);
        if (s >= slots) { gfx_line(bx + 3, by + 3, bx + 10, by + 10, C_DUSK); continue; }
    }
    for (int e = 0; e < KPR_EQUIP && k < 4; e++)
        if (kpr_on(e)) { spr_draw(&kpr_spr[SPR_EQ0 + e], 5 + k * 14, 96, 0); k++; }
    /* the trail */
    tiny("TOW", 6, 114, C_LIGHT);
    for (int s = 0; s < KPR_TRAIL; s++) {
        int bx = 22 + s * 13, by = 112;
        gfx_rectb(bx, by, 12, 12, C_SLATE);
        if (s >= kg.trail_n) continue;
        int id = kg.trail[s].kind == TR_CHARGE ? SPR_CHARGE : item_sprite(kg.trail[s].id);
        const Sprite *sp = &kpr_spr[id];
        spr_draw(sp, bx + 6 - sp->w / 2, by + 6 - sp->h / 2, 0);
    }
    /* counts */
    char line[24];
    snprintf(line, sizeof line, "TORP %d", kg.cur.boosters + 1);
    tiny(line, 6, 130, C_SKY);
    if (kpr_on(EQ_ARMOR)) {
        snprintf(line, sizeof line, "PLATE %d", kg.plates);
        tiny(line, 6, 138, C_LIGHT);
    }
    static const char *const AREA[3] = {"KELPWOOD", "ARCADE", "BLACKWATER"};
    tiny_c(AREA[kpr_area_of(kg.y)], 32, 166, C_GREY);
}

/* ---- the boss bar and messages ------------------------------------------------------------ */

static void draw_view_text(void) {
    ui_audit_area("view", VX, 0, KPR_VIEW_W, KPR_VIEW_H);
    if (kpr_demo) {
        if ((kg.frame_t / 30) & 1) txt_c("DEMO", VX + KPR_VIEW_W / 2, 20, C_YELLOW);
        tiny_c("PRESS ANY BUTTON", VX + KPR_VIEW_W / 2, 30, C_WHITE);
    }
    if (kg.fight >= 0 && kg.boss[kg.fight].on) {
        const KprBoss *o = &kg.boss[kg.fight];
        int w = 120, x = VX + (KPR_VIEW_W - w) / 2;
        tiny_c(KPR_BOSS_NAME[kg.fight], VX + KPR_VIEW_W / 2, 3, C_WHITE);
        gfx_rect(x, 10, w, 4, C_INK);
        gfx_rect(x, 10, imax(0, o->hp) * w / KPR_BOSS_HP[kg.fight], 4, C_RED);
    }
    if (kg.msg_t > 0 && kg.msg[0]) {
        int w = text_width(kg.msg) + 8;
        int x = VX + (KPR_VIEW_W - w) / 2;
        gfx_rect(x, KPR_VIEW_H - 14, w, 12, C_INK);
        txt(kg.msg, x + 4, KPR_VIEW_H - 12, C_CREAM);
    }
}

static void draw_world(void) {
    camx = kg.cam_x + (kg.shake > 0 ? (kg.frame_t & 2) - 1 : 0);
    camy = kg.cam_y + (kg.shake > 0 ? ((kg.frame_t >> 1) & 2) - 1 : 0);
    gfx_clip(VX, 0, KPR_VIEW_W, KPR_VIEW_H);
    draw_backdrop();
    draw_surface();
    draw_tiles();
    draw_vents();
    draw_items();
    draw_foes();
    draw_bosses();
    draw_shots();
    draw_sub();
    draw_dark();
    gfx_noclip();
}

/* ---- the docks' screens ---------------------------------------------------------------- */

static void draw_dock(void) {
    draw_world();
    draw_hud();
    gfx_darken_rect(VX, 0, KPR_VIEW_W, KPR_VIEW_H, 2);
    int px = VX + 8, py = 8, pw = KPR_VIEW_W - 16, ph = KPR_VIEW_H - 16;
    ui_panel(px, py, pw, ph, C_NIGHT, C_YELLOW);
    ui_audit_area("dock", px, py, pw, ph);
    int cx = px + pw / 2;
    txt_c(kg.cur.dock == 0 ? "THE PIER" : "THE LIGHTHOUSE", cx, py + 5, C_YELLOW);
    if (kg.dock_phase == 0) {
        int shown = imin(kg.dock_lines, kg.state_t / 20 + 1);
        for (int i = 0; i < shown; i++) txt_c(kg.dock_line[i], cx, py + 22 + i * 11, C_WHITE);
        txt_c("FUEL FULL. SAVED.", cx, py + ph - 26, C_LIME);
        tiny_c("A: GO ON", cx, py + ph - 10, C_GREY);
        return;
    }
    char line[48];
    int fit = 0;
    for (int e = 0; e < KPR_EQUIP; e++) fit += kpr_on(e);
    snprintf(line, sizeof line, "FITTED %d OF %d", fit, kpr_slots());
    tiny_c(line, cx, py + 16, C_LIGHT);
    for (int e = 0; e < KPR_EQUIP; e++) {
        int gx = cx - 5 * 15 + (e % 5) * 30, gy = py + 26 + (e / 5) * 30;
        bool have = (kg.cur.have >> e) & 1, on = kpr_on(e);
        gfx_rect(gx, gy, 26, 26, on ? C_FOREST : C_INK);
        gfx_rectb(gx, gy, 26, 26, e == kg.dock_cur ? C_YELLOW : on ? C_LIME : C_DUSK);
        if (e == kg.dock_cur) gfx_rectb(gx - 1, gy - 1, 28, 28, (kg.frame_t / 8) & 1 ? C_YELLOW : C_AMBER);
        if (have) spr_draw(&kpr_spr[SPR_EQ0 + e], gx + 7, gy + 7, 0);
        else text_draw("?", gx + 11, gy + 9, C_DUSK);
    }
    int ny = py + 90;
    bool have = (kg.cur.have >> kg.dock_cur) & 1;
    txt_c(have ? KPR_EQ_NAME[kg.dock_cur] : "NOT FOUND YET", cx, ny, have ? C_WHITE : C_GREY);
    if (have) tiny_c(KPR_EQ_LINE[kg.dock_cur], cx, ny + 11, C_SKY);
    int big = 0;
    for (int b = 0; b < BS_XOLLOTH; b++) big += kg.cur.boss_dead[b];
    snprintf(line, sizeof line, "TANKS %d/20  KITS %d/20  EGGS %d/20", kg.cur.tanks, kg.cur.boosters, kg.cur.eggs_back);
    tiny_c(line, cx, ny + 24, C_LIGHT);
    snprintf(line, sizeof line, "BIG ONES %d/5  LIGHTS %d  DONE %d%%", big, kg.cur.lights, kpr_completion());
    tiny_c(line, cx, ny + 32, C_LIGHT);
    tiny_c("A: FIT OR TAKE OFF   B: LAUNCH", cx, py + ph - 10, C_GREY);
}

static void draw_towed(void) {
    draw_world();
    draw_hud();
    gfx_darken_rect(VX, 0, KPR_VIEW_W, KPR_VIEW_H, imin(4, kg.state_t / 10));
    ui_audit_area("view", VX, 0, KPR_VIEW_W, KPR_VIEW_H);
    int cx = VX + KPR_VIEW_W / 2;
    txt_c("OUT OF FUEL", cx, 64, C_RED);
    if (kg.state_t > 30) txt_c("TOWED BACK TO THE DOCK.", cx, 80, C_WHITE);
    if (kg.state_t > 60) txt_c("THE DIVE'S FINDS ARE LOST.", cx, 92, C_LIGHT);
}

static void draw_roe(void) {
    draw_world();
    draw_hud();
    gfx_darken_rect(VX, 0, KPR_VIEW_W, KPR_VIEW_H, 2);
    int px = VX + 8, py = 30, pw = KPR_VIEW_W - 16, ph = 120;
    ui_panel(px, py, pw, ph, C_NIGHT, C_PINK);
    ui_audit_area("roe", px, py, pw, ph);
    spr_draw_scaled(&kpr_spr[SPR_ROE], px + 6, py + 8, 2, 0);
    txt("AUNTIE ROE", px + 44, py + 6, C_PINK);
    bool first = kg.cur.eggs_back == kg.roe_given;
    const char *const *L = first ? KPR_ROE_HELLO : KPR_ROE_THANKS;
    int n = first ? KPR_NROE_HELLO : KPR_NROE_THANKS;
    if (kg.roe_page) { L = KPR_ROE_FRIEND; n = KPR_NROE_FRIEND; }
    for (int i = 0; i < n; i++) tiny(L[i], px + 44, py + 22 + i * 9, C_WHITE);
    char line[40];
    snprintf(line, sizeof line, "EGGS HOME: %d", kg.cur.eggs_back);
    tiny(line, px + 44, py + 56, C_YELLOW);
    if (kg.roe_page) {
        spr_draw(&kpr_spr[(kpr_companions() - 1) % 2 == 0 ? SPR_FISH_BLUE : SPR_FISH_RED], px + 44, py + 70, 0);
        tiny((kpr_companions() - 1) % 2 == 0 ? "A BLUE ONE: IT STOPS SHOTS" : "A RED ONE: IT SHOOTS WITH YOU", px + 56, py + 70, C_SKY);
    }
    tiny_c("A: GOODBYE", px + pw / 2, py + ph - 12, C_GREY);
}

/* ---- the title, the story, the ending ---------------------------------------------------- */

static void sea_backdrop(void) {
    for (int y = 0; y < SCREEN_H; y++) gfx_hline(0, SCREEN_W - 1, y, y < 40 ? C_SKY : y < 46 ? C_ICE : y < 110 ? C_BLUE : C_NAVY);
    for (int y = 104; y < 116; y++) gfx_dither(0, y, SCREEN_W, 1, C_NAVY, (y - 104) + 2);
    for (int x = 0; x < SCREEN_W; x++) gfx_vline(x, 40 + (int)(sinf((x + kg.frame_t) * 0.1f) * 1.5f), 41, C_WHITE);
    for (int k = 0; k < 8; k++) {
        int bx = (k * 47 + 13) % SCREEN_W, by = SCREEN_H - ((kg.frame_t / 2 + k * 23) % 130);
        if (by > 46) gfx_circb(bx + (int)(sinf((kg.frame_t + k * 9) * 0.05f) * 3), by, 1 + k % 2, C_ICE);
    }
    for (int k = 0; k < 9; k++) {
        int kx = 10 + k * 36;
        for (int y = 0; y < 30 + (k * 13) % 20; y += 2) {
            int sw = (int)(sinf((kg.frame_t + k * 30 + y * 4) * 0.03f) * 2);
            gfx_rect(kx + sw, SCREEN_H - y, 2, 2, (y & 2) ? C_JADE : C_FOREST);
        }
    }
}

static void draw_title(void) {
    sea_backdrop();
    ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
    static const uint8_t grad[3] = {C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center("KIPPER", 160, 10, 3, grad, 3, C_INK, C_NAVY);
    ui_audit_fancy("logo", "KIPPER", 160 - ui_fancy_width("KIPPER", 3) / 2, 10, 3, true);
    int bob = (int)(sinf(kg.frame_t * 0.05f) * 3);
    spr_draw_scaled(&kpr_spr[SPR_SUB0], 136, 58 + bob, 3, 0);
    static const char *const M[2] = {"NEW GAME", "CONTINUE"};
    for (int i = 0; i < 2; i++) {
        int col = i == 1 && !ksv.has_run ? C_SLATE : i == kg.menu ? C_YELLOW : C_WHITE;
        txt_c(M[i], 160, 104 + i * 12, col);
        if (i == kg.menu) ui_cursor(160 - text_width(M[i]) / 2 - 10, 104 + i * 12, kg.frame_t);
    }
    char line[64];
    snprintf(line, sizeof line, "FUEL TANKS %d   TORPEDO KITS %d   EGGS RETURNED %d", ksv.best_tanks, ksv.best_boosters,
             ksv.best_eggs);
    tiny_c(line, 160, 136, C_ICE);
    tiny_c("A: DIVE   B: LIBRARY", 160, 168, C_LIGHT);
}

static void draw_intro(void) {
    gfx_cls(C_NAVY);
    ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
    for (int i = 0; i < KPR_NINTRO; i++)
        if (KPR_INTRO[i][0]) txt_c(KPR_INTRO[i], 160, 12 + i * 11, i == 0 ? C_YELLOW : C_WHITE);
    spr_draw(&kpr_spr[SPR_SUB0], (kg.state_t / 2) % 360 - 20, 152 + (int)(sinf(kg.state_t * 0.08f) * 2), 0);
    if (kg.state_t > 20) tiny_c("A: DIVE", 160, 170, C_LIGHT);
}

static void draw_ending(void) {
    gfx_cls(C_INK);
    ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
    const char *const *L = kg.cherry_run ? KPR_END_CHERRY : KPR_END_GOLD;
    int n = kg.cherry_run ? KPR_NEND_CHERRY : KPR_NEND_GOLD;
    for (int i = 0; i < n; i++)
        if (L[i][0] && kg.state_t > i * 40) txt_c(L[i], 160, 10 + i * 11, i >= 3 && i <= 5 ? C_VIOLET : C_WHITE);
    int y = 150 + (int)(sinf(kg.state_t * 0.03f) * 2);
    spr_draw(&kpr_spr[SPR_SUB0], 152, y, 0);
    if (kg.cherry_run) gfx_dither_circle(160, y + 6, 14, C_VIOLET, 4 + (kg.state_t / 10) % 4);
}

static void draw_credits(void) {
    gfx_cls(C_NAVY);
    ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
    int y0 = SCREEN_H - kg.state_t / 3;
    for (int i = 0; i < KPR_NCREDITS; i++) {
        int y = y0 + i * 14;
        if (y < -10 || y > SCREEN_H) continue;
        text_draw_scaled(KPR_CREDITS[i], 160 - text_width(KPR_CREDITS[i]) / 2, y, i == 0 ? C_YELLOW : C_WHITE, 1);
    }
}

void kpr_draw(void) {
    switch (kg.state) {
    case KS_TITLE: draw_title(); break;
    case KS_INTRO: draw_intro(); break;
    case KS_DOCK: draw_dock(); break;
    case KS_TOWED: draw_towed(); break;
    case KS_ROE: draw_roe(); break;
    case KS_ENDING: draw_ending(); break;
    case KS_CREDITS: draw_credits(); break;
    default:
        draw_world();
        draw_hud();
        draw_view_text();
        break;
    }
}

void kpr_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    for (int yy = 0; yy < h; yy++) gfx_hline(x, x + w - 1, y + yy, yy < h / 4 ? C_SKY : yy < h * 2 / 3 ? C_BLUE : C_NAVY);
    gfx_hline(x, x + w - 1, y + h / 4, C_WHITE);
    for (int k = 0; k < 5; k++) gfx_circb(x + (k * 23 + t / 4) % w, y + h - 4 - ((t / 2 + k * 17) % (h * 3 / 4)), 1, C_ICE);
    spr_draw(&kpr_spr[SPR_SUB0], x + w - 26, y + h / 2 + (int)(sinf(t * 0.05f) * 2), 0);
    spr_draw(&kpr_spr[SPR_EGG], x + 8, y + h - 14, 0);
    static const uint8_t grad[2] = {C_YELLOW, C_ORANGE};
    ui_fancy_text("KIPPER", x + 6, y + 4, 1, grad, 2, C_INK, C_NAVY);
    gfx_noclip();
}

/* tests: every screen's words inside the screen, clear of each other, and
 * every glyph in the font */
int kpr_layout_audit(void) {
    int bad = 0;
    KprGame keep = kg;
    static const int SCREENS[] = {KS_TITLE, KS_INTRO, KS_PLAY, KS_DOCK, KS_DOCK, KS_TOWED, KS_ROE, KS_ROE, KS_ENDING,
                                  KS_ENDING};
    for (int i = 0; i < ARRAY_LEN(SCREENS); i++) {
        kg.state = SCREENS[i];
        kg.state_t = 5000;
        kg.dock_phase = i == 3 ? 0 : 1;
        kg.dock_lines = 3;
        snprintf(kg.dock_line[0], 40, "FUEL TANK: 24 BARS");
        snprintf(kg.dock_line[1], 40, "TORPEDO KIT: LEVEL 21");
        snprintf(kg.dock_line[2], 40, "UPSHOT ROCKETS!");
        kg.roe_page = i == 7;
        kg.cherry_run = i == 9;
        kg.msg_t = 10;
        snprintf(kg.msg, sizeof kg.msg, "THE WATER'S RUMBLING... SOMETHING BIG.");
        ui_audit_begin("kipper", true);
        kpr_draw();
        bad += ui_audit_end();
    }
    kg = keep;
    for (int e = 0; e < KPR_EQUIP; e++) {
        bad += text_missing(KPR_EQ_NAME[e]) > 0;
        bad += tiny_missing(KPR_EQ_LINE[e]) > 0;
        if (tiny_width(KPR_EQ_LINE[e]) > KPR_VIEW_W - 24) { bad++; fprintf(stderr, "tagline %d too wide\n", e); }
    }
    for (int k = 0; k < KPR_NEND_GOLD; k++) bad += text_width(KPR_END_GOLD[k]) > SCREEN_W - 8;
    for (int k = 0; k < KPR_NEND_CHERRY; k++) bad += text_width(KPR_END_CHERRY[k]) > SCREEN_W - 8;
    for (int k = 0; k < KPR_NINTRO; k++) bad += text_width(KPR_INTRO[k]) > SCREEN_W - 8;
    for (int k = 0; k < KPR_NCREDITS; k++) bad += text_missing(KPR_CREDITS[k]) > 0;
    return bad;
}
