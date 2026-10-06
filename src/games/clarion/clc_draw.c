/* CLARION CALL - drawing: the maps, behind the doors, the HUD, the station
 * map and the sparks. Nothing here changes a rule. */
#include "clc.h"

static int PX(int32_t v) { return (int)(v / 256) - (v < 0 && v % 256 ? 1 : 0); }

const uint8_t CLC_REGION_COL[RG_COUNT][2] = {
    {C_TAN, C_BROWN}, {C_LEAF, C_FOREST}, {C_ICE, C_SKY}, {C_PINK, C_WINE},
    {C_AMBER, C_SLATE}, {C_VIOLET, C_PURPLE}, {C_YELLOW, C_GREY},
};

typedef struct Theme { uint8_t bg, far, fill, dark, light, rim; } Theme;
static const Theme THEME[RG_COUNT] = {
    {C_NIGHT, C_DUSK, C_EARTH, C_BROWN, C_TAN, C_HIDE},
    {C_INK, C_FOREST, C_BROWN, C_EARTH, C_TAN, C_LEAF},
    {C_NAVY, C_BLUE, C_SKY, C_BLUE, C_ICE, C_WHITE},
    {C_MAROON, C_WINE, C_WINE, C_MAROON, C_PINK, C_MAGENTA},
    {C_INK, C_NIGHT, C_SLATE, C_DUSK, C_GREY, C_AMBER},
    {C_INK, C_NAVY, C_PURPLE, C_NIGHT, C_VIOLET, C_CREAM},
    {C_NIGHT, C_PURPLE, C_GREY, C_SLATE, C_LIGHT, C_YELLOW},
};

/* ---- sparks ------------------------------------------------------------------- */

typedef struct Part { int16_t x, y, vx, vy; uint8_t life, col, on; } Part; /* 1/16 px */
#define NPARTS 128
static Part parts[NPARTS];
static uint32_t seedp = 1;

static int prand(int lo, int hi) {
    seedp = seedp * 1103515245u + 12345u;
    return lo + (int)((seedp >> 16) % (uint32_t)(hi - lo + 1));
}

static void spark(int x, int y, int n, int col, int speed) {
    for (int k = 0; k < n; k++)
        for (int i = 0; i < NPARTS; i++)
            if (!parts[i].on) {
                int a = prand(0, 255), v = prand(speed / 3, speed);
                parts[i] = (Part){(int16_t)(x * 16), (int16_t)(y * 16), (int16_t)(clc_cos(a) * v / 127),
                                  (int16_t)(clc_sin(a) * v / 127), (uint8_t)prand(12, 26), (uint8_t)col, 1};
                break;
            }
}

void clc_fx_clear(void) { memset(parts, 0, sizeof parts); }

void clc_fx_feed(const ClcFx *fx, int n) {
    for (int k = 0; k < n; k++) {
        const ClcFx *f = &fx[k];
        switch (f->kind) {
        case FX_BOOM: spark(f->x, f->y, 24, C_ORANGE, 40); spark(f->x, f->y, 12, C_YELLOW, 28); break;
        case FX_POP: spark(f->x, f->y, 10, C_WHITE, 26); spark(f->x, f->y, 6, C_PINK, 18); break;
        case FX_SPARK: spark(f->x, f->y, 4, C_YELLOW, 16); break;
        case FX_COINS: spark(f->x, f->y, 8, C_YELLOW, 24); break;
        case FX_DUST: spark(f->x, f->y, 6, C_GREY, 18); break;
        case FX_FUEL: spark(f->x, f->y, 10, C_LIME, 26); break;
        case FX_HIT: spark(f->x, f->y, 3, C_WHITE, 14); break;
        case FX_NOTE: spark(f->x, f->y, 14, C_VIOLET, 30); spark(f->x, f->y, 6, C_WHITE, 20); break;
        default: break;
        }
    }
}

static void parts_draw(void) {
    for (int i = 0; i < NPARTS; i++) {
        Part *p = &parts[i];
        if (!p->on) continue;
        p->x = (int16_t)(p->x + p->vx);
        p->y = (int16_t)(p->y + p->vy);
        p->vy = (int16_t)(p->vy + 1);
        if (--p->life == 0) p->on = 0;
        gfx_pset(p->x / 16, p->y / 16, p->life < 5 ? C_SLATE : p->col);
    }
}

/* ---- backdrops ------------------------------------------------------------------ */

void clc_draw_backdrop(int region, int t) {
    const Theme *th = &THEME[iclamp(region, 0, RG_COUNT - 1)];
    gfx_cls(C_INK);
    for (int i = 0; i < 60; i++) gfx_pset((i * 97 + 13) % 320, (i * 53 + 7) % 180, i % 6 ? C_DUSK : C_GREY);
    /* the Carillon: a tower of bells against the stars */
    int cx = 160;
    gfx_rect(cx - 40, 60, 80, 120, th->dark);
    gfx_rect(cx - 30, 30, 60, 30, th->dark);
    gfx_rect(cx - 12, 8, 24, 22, th->dark);
    for (int k = 0; k < 3; k++) {
        int y = 72 + k * 32;
        gfx_rect(cx - 30, y, 60, 2, th->fill);
        for (int b = 0; b < 3; b++) {
            int bx = cx - 20 + b * 20, sw = clc_sin(t * 2 + b * 40 + k * 30) * 2 / 127;
            gfx_circ(bx + sw, y + 10, 5, (t / 20 + b + k) % 5 ? th->light : C_YELLOW);
            gfx_rect(bx - 6 + sw, y + 13, 13, 2, th->light);
        }
    }
    gfx_circ(cx, 19, 6, C_YELLOW);
}

/* ---- tiles -------------------------------------------------------------------------- */

static bool air_t(const ClcWorld *w, int c, int r) {
    if (c < 0 || r < 0 || c >= w->w || r >= w->h) return false;
    return !clc_map_solid_tile(w->tile[r][c]);
}

static void draw_map_tiles(const ClcWorld *w, int cx, int cy, int t) {
    const Theme *th = &THEME[w->region];
    int c0 = imax(0, cx / CLC_T), c1 = imin(w->w - 1, (cx + SCREEN_W) / CLC_T);
    int r0 = imax(0, cy / CLC_T), r1 = imin(w->h - 1, (cy + SCREEN_H) / CLC_T);
    for (int r = r0; r <= r1; r++)
        for (int c = c0; c <= c1; c++) {
            int tt = w->tile[r][c], x = c * CLC_T, y = r * CLC_T;
            switch (tt) {
            case MT_AIR: break;
            case MT_LADDER:
                gfx_vline(x + 1, y, y + 7, C_BROWN);
                gfx_vline(x + 6, y, y + 7, C_BROWN);
                gfx_hline(x + 1, x + 6, y + 2, C_TAN);
                gfx_hline(x + 1, x + 6, y + 6, C_TAN);
                break;
            case MT_GATE_OPEN:
                if (c % 6 == 0 || !air_t(w, c - 1, r) || !air_t(w, c + 1, r)) {
                    gfx_rect(x + 2, y, 4, 8, C_SLATE);
                    gfx_vline(x + 3, y, y + 7, C_GREY);
                }
                break;
            default: {
                gfx_rect(x, y, 8, 8, th->fill);
                if (tt == MT_ROCK2) { gfx_pset(x + 2, y + 3, th->dark); gfx_pset(x + 5, y + 6, th->dark); gfx_pset(x + 3, y + 5, th->light); }
                else if (((c * 3 + r * 5) & 7) == 0) gfx_pset(x + 3, y + 4, th->dark);
                if (air_t(w, c, r - 1)) { gfx_hline(x, x + 7, y, th->rim); gfx_hline(x, x + 7, y + 1, th->light); }
                if (air_t(w, c, r + 1)) gfx_hline(x, x + 7, y + 7, th->dark);
                if (air_t(w, c - 1, r)) gfx_vline(x, y, y + 7, th->light);
                if (air_t(w, c + 1, r)) gfx_vline(x + 7, y, y + 7, th->dark);
                if (tt == MT_COIN) {
                    gfx_circ(x + 4, y + 4, 2, (t / 10) & 1 ? C_YELLOW : C_AMBER);
                    gfx_pset(x + 4, y + 4, C_WHITE);
                } else if (tt == MT_MAGNET_PULL || tt == MT_MAGNET_PUSH) {
                    int mc = w->magnets_off ? C_SLATE : tt == MT_MAGNET_PULL ? C_RED : C_BLUE;
                    gfx_rect(x + 1, y + 1, 6, 6, mc);
                    gfx_rect(x + 3, y + 2, 2, 4, C_WHITE);
                    if (!w->magnets_off && (t / 6) & 1) gfx_rectb(x - 1, y - 1, 10, 10, mc);
                } else if (tt == MT_FLAME) {
                    gfx_rect(x + 2, y + 2, 4, 4, C_RED);
                }
                break;
            }
            }
        }
}

/* ---- map things -------------------------------------------------------------------- */

static void draw_door(const ClcWorld *w, const ClcDoor *d, int t) {
    if (d->hidden) return;
    int x = d->c * CLC_T, y = (d->r - 1) * CLC_T;
    int col = d->type == DT_GREEN ? C_LEAF : d->type == DT_BLUE ? C_SKY : d->type == DT_YELLOW ? C_YELLOW :
              d->type == DT_GOLD ? C_AMBER : d->type == DT_SECRET ? C_VIOLET : C_RED;
    gfx_rect(x - 1, y - 2, 10, 18, C_INK);
    gfx_rect(x, y - 1, 8, 17, col);
    gfx_rect(x + 2, y + 1, 4, 15, PAL_DARKER[col]);
    gfx_pset(x + 5, y + 9, C_WHITE);
    if (d->type == DT_YELLOW) {
        /* horns */
        gfx_line(x - 2, y - 5, x, y - 1, C_CREAM);
        gfx_line(x + 9, y - 5, x + 7, y - 1, C_CREAM);
    }
    if (d->type == DT_GOLD) {
        int s = (t / 6) & 1;
        gfx_pset(x + 4, y - 4 - s, C_WHITE);
        gfx_rectb(x - 2, y - 3, 12, 20, s ? C_YELLOW : C_AMBER);
    }
    if (w->timer_on && d->type != DT_GOLD && w->kind == WK_GEN) {
        gfx_rect(x + 2, y + 6, 4, 4, C_INK);
        gfx_pset(x + 3, y + 7, C_GREY);
    }
}

static void draw_note(int x, int y, bool fake, bool big, int t) {
    int bob = clc_sin(t * 4 + x) * 2 / 127;
    if (big) gfx_dither_circle(x, y + bob, 8, C_VIOLET, 6);
    if (fake) {
        uint8_t map[256];
        pal_identity(map);
        map[C_VIOLET] = C_RED;
        map[C_PURPLE] = C_MAROON;
        map[C_MAGENTA] = C_ORANGE;
        clc_spr_ex(CS_NOTE, x - 4, y - 5 + bob, 0, map);
    } else {
        clc_spr(CS_NOTE, x - 4, y - 5 + bob, 0);
    }
    if (((t + x) / 10) % 6 == 0) gfx_pset(x + 3, y - 4 + bob, C_WHITE);
}

static void draw_ent(const ClcWorld *w, const ClcEnt *e, int t) {
    int x = PX(e->x), y = PX(e->y);
    int f = e->dir < 0 ? SPR_FLIPX : 0;
    switch (e->kind) {
    case EK_FLITTER: clc_spr(CS_FLITTER, x - 5, y - 4 - ((t / 6) & 1), 0); break;
    case EK_CREEPER:
        gfx_rect(x - 5, y - 2, 10, 5, C_ORANGE);
        gfx_hline(x - 4, x + 3, y - 3, C_AMBER);
        gfx_pset(x + (e->b > 0 ? 3 : -4), y - 1, C_INK);
        for (int k = 0; k < 3; k++) gfx_pset(x - 4 + k * 4 + ((t / 4) & 1), e->dir >= 0 ? y + 3 : y - 3, C_INK);
        break;
    case EK_GRUB: clc_spr(CS_GRUB, x - 9, y - 4, e->b < 0 ? SPR_FLIPX : 0); break;
    case EK_SNAP:
        gfx_rect(x - 6, y + 1, 12, 3, C_SLATE);
        if (e->flag) for (int k = 0; k < 6; k++) gfx_vline(x - 6 + k * 2, y - 3, y, C_LIGHT);
        else { gfx_line(x - 6, y, x - 2, y - 4, C_LIGHT); gfx_line(x + 5, y, x + 1, y - 4, C_LIGHT); }
        break;
    case EK_FIREDRONE:
    case EK_WINDDRONE: {
        int c = e->kind == EK_FIREDRONE ? C_RED : C_CYAN;
        gfx_circ(x, y, 4, C_SLATE);
        gfx_circ(x, y, 2, c);
        gfx_hline(x - 6, x + 5, y - 5, (t / 2) & 1 ? C_LIGHT : C_GREY);
        if (e->kind == EK_WINDDRONE)
            for (int k = 0; k < 4; k++) {
                int wx = x + e->dir * (10 + ((t * 2 + k * 20) % 80));
                gfx_hline(wx, wx + e->dir * 3, y - 6 + k * 4, C_ICE);
            }
        break;
    }
    case EK_LEECH: gfx_circ(x, y, 3, C_MAGENTA); gfx_pset(x, y - 1, C_INK); gfx_pset(x - 2, y + 2 + ((t / 5) & 1), C_WINE); break;
    case EK_EYE:
        gfx_circ(x, y, 4, C_CREAM);
        gfx_circ(x + e->dir, y, 2, C_RED);
        gfx_pset(x + e->dir, y, C_INK);
        break;
    case EK_ACID:
        gfx_circ(x, y, 4, e->flag ? ((t / 3) & 1 ? C_LIME : C_YELLOW) : C_LEAF);
        gfx_circb(x, y, 4, C_FOREST);
        break;
    case EK_BLOB:
        if (e->flag) break;
        gfx_circ(x, y, 5, C_PINK);
        gfx_circ(x - 2, y - 2, 1, C_WHITE);
        gfx_pset(x + 1, y, C_INK);
        break;
    case EK_SENTRY: {
        int up = e->dir >= 0 ? -1 : 1;
        gfx_rect(x - 5, y - 2, 10, 5, w->on_foot ? C_SLATE : C_GREY);
        gfx_rect(x - 1, y + up * 5 - 1, 3, 4, C_SLATE);
        gfx_pset(x, y, w->on_foot ? C_DUSK : ((t / 8) & 1 ? C_RED : C_MAROON));
        break;
    }
    case EK_SNAKE: case EK_WORM:
        gfx_circ(x, y, 5, e->kind == EK_SNAKE ? C_AMBER : C_LIME);
        gfx_pset(x + (e->vx >= 0 ? 2 : -2), y - 1, C_INK);
        gfx_pset(x + (e->vx >= 0 ? 4 : -4), y + 1, C_RED);
        break;
    case EK_SEG:
        gfx_circ(x, y, 4, e->flag == EK_SNAKE ? C_ORANGE : C_LEAF);
        gfx_pset(x, y, e->flag == EK_SNAKE ? C_AMBER : C_LIME);
        break;
    case EK_CRAB:
        gfx_rect(x - 4, y - 3, 8, 6, C_RED);
        gfx_pset(x - 5, y - 4, C_ORANGE);
        gfx_pset(x + 4, y - 4, C_ORANGE);
        gfx_pset(x + e->dir * 2, y - 1, C_WHITE);
        break;
    case EK_GHOST: {
        int bob = clc_sin(t * 6) * 2 / 127;
        gfx_circ(x, y - 1 + bob, 5, C_ICE);
        gfx_rect(x - 5, y + bob, 11, 5, C_ICE);
        gfx_pset(x - 2, y - 2 + bob, C_INK);
        gfx_pset(x + 2, y - 2 + bob, C_INK);
        break;
    }
    case EK_JETFIRE:
        gfx_rect(x - 3, y - 3, 6, 6, C_SLATE);
        if (e->flag) {
            int x0 = e->dir > 0 ? x + 4 : x - 28;
            for (int k = 0; k < 24; k += 2) gfx_vline(x0 + k, y - 3 + ((t + k) % 3), y + 3 - ((t + k) % 2), k % 4 ? C_ORANGE : C_YELLOW);
        }
        break;
    case EK_BOOMER:
        gfx_rect(x - 4, y - 5, 8, 10, C_TEAL);
        gfx_pset(x + e->dir * 2, y - 2, C_WHITE);
        break;
    case EK_CHASER:
        gfx_circ(x, y, 5, C_RED);
        gfx_pset(x + 2, y - 2, C_YELLOW);
        gfx_line(x - 6, y - 4, x - 2, y - 1, (t / 4) & 1 ? C_ORANGE : C_RED);
        gfx_line(x + 6, y - 4, x + 2, y - 1, (t / 4) & 1 ? C_ORANGE : C_RED);
        break;
    case EK_BIGBELLY:
        gfx_circ(x, y, 9, C_HIDE);
        gfx_circ(x, y + 2, 6, C_TAN);
        gfx_circ(x - 3, y - 4, 2, C_WHITE);
        gfx_circ(x + 3, y - 4, 2, C_WHITE);
        gfx_pset(x - 3, y - 4, C_INK);
        gfx_pset(x + 3, y - 4, C_INK);
        gfx_hline(x - 3, x + 3, y + 3, C_BROWN);
        break;
    case EK_POD:
        gfx_circ(x, y, 4, e->flag ? C_WINE : C_CREAM);
        gfx_circb(x, y, 4, C_BROWN);
        if (e->flag) gfx_pset(x, y, (t / 6) & 1 ? C_RED : C_YELLOW);
        else { gfx_pset(x - 1, y - 1, C_TAN); gfx_pset(x + 2, y + 1, C_TAN); }
        break;
    case EK_LATE: clc_spr(CS_JELLY, x - 6, y - 6 + ((t / 4) & 1), 0); break;
    case EK_NOTE: draw_note(x, y, false, w->notes == CLC_NOTES_NEEDED - 1, t); break;
    case EK_FAKENOTE: draw_note(x, y, true, false, t); break;
    case EK_PLUM:
        gfx_circ(x, y, 3, C_PURPLE);
        gfx_circ(x, y, 2, C_VIOLET);
        gfx_pset(x - 1, y - 1, C_WHITE);
        break;
    case EK_COIN:
        if (e->b > 780 && (t / 3) & 1) break;
        gfx_circ(x, y, e->a >= 50 ? 4 : 2, C_YELLOW);
        gfx_pset(x, y, C_AMBER);
        break;
    case EK_RING: {
        int r = 5 + ((t / 5) & 1);
        gfx_circb(x, y, r, C_LIME);
        gfx_circb(x, y, r - 2, C_YELLOW);
        int a = t * 8;
        gfx_line(x, y, x + clc_cos(a) * 3 / 127, y + clc_sin(a) * 3 / 127, C_WHITE);
        break;
    }
    case EK_FUELCAN:
        gfx_rect(x - 3, y - 4, 7, 9, C_LEAF);
        gfx_rect(x - 1, y - 6, 3, 2, C_GREY);
        gfx_hline(x - 2, x + 2, y, C_WHITE);
        break;
    case EK_ARROW: {
        int b = (t / 8) & 1;
        gfx_line(x, y - 6 - b, x - 4, y - 2 - b, C_YELLOW);
        gfx_line(x, y - 6 - b, x + 4, y - 2 - b, C_YELLOW);
        gfx_vline(x, y - 6 - b, y + 3 - b, C_YELLOW);
        break;
    }
    default: break;
    }
    (void)f;
}

static void draw_shot(const ClcShot *s, int t) {
    int x = PX(s->x), y = PX(s->y);
    switch (s->kind) {
    case SH_PISTOL: gfx_rect(x - 1, y - 1, 2, 2, C_WHITE); break;
    case SH_BIG: gfx_circ(x, y, 2, C_YELLOW); gfx_pset(x, y, C_WHITE); break;
    case SH_FAN: gfx_pset(x, y, C_PINK); break;
    case SH_FRAG: gfx_rect(x - 1, y - 1, 3, 2, C_ORANGE); break;
    case SH_SEEKER: gfx_circ(x, y, 2, C_AMBER); gfx_pset(x, y, (t / 2) & 1 ? C_WHITE : C_YELLOW); break;
    case SH_BEAM: gfx_line(x - s->vx / 128, y - s->vy / 128, x, y, C_RED); break;
    case SH_SPIT: gfx_rect(x - 2, y - 1, 4, 2, C_CYAN); break;
    case SH_PELLET: gfx_rect(x - 1, y - 1, 3, 3, (t / 3) & 1 ? C_RED : C_ORANGE); break;
    case SH_FIREBALL: gfx_circ(x, y, 2, C_ORANGE); gfx_pset(x, y, C_YELLOW); break;
    case SH_ACID: gfx_rect(x - 1, y - 1, 2, 2, C_LIME); break;
    case SH_BOOMERANG: {
        int a = t * 24;
        gfx_line(x - clc_cos(a) * 3 / 127, y - clc_sin(a) * 3 / 127, x + clc_cos(a) * 3 / 127, y + clc_sin(a) * 3 / 127, C_CREAM);
        break;
    }
    case SH_RING: gfx_circb(x, y, 2, C_VIOLET); break;
    case SH_ROCK: gfx_circ(x, y, 3, C_HIDE); break;
    case SH_ENERGY: gfx_circ(x, y, 3, (t / 2) & 1 ? C_PINK : C_MAGENTA); gfx_pset(x, y, C_WHITE); break;
    case SH_FLAMELET: gfx_rect(x - 2, y - 3, 4, 5, (t / 3) & 1 ? C_ORANGE : C_YELLOW); break;
    default: gfx_pset(x, y, C_WHITE); break;
    }
}

static void draw_slash(const ClcWorld *w, const ClcPlayer *p, int x, int y) {
    if (!w->slash_t || w->on_foot) return;
    int age = 12 - w->slash_t;
    static const int8_t CS[9][2] = {{0, -10}, {4, -9}, {7, -7}, {9, -4}, {10, 0}, {9, 4}, {7, 7}, {4, 9}, {0, 10}};
    int shown = imin(9, 3 + age * 2);
    for (int s = 0; s < 2; s++) {
        int f = s == 0 ? (w->f.face >= 0 ? 1 : -1) : -(w->f.face >= 0 ? 1 : -1);
        if (s == 1 && !clc_has(p, G_TWIN)) break;
        for (int k = 0; k < shown; k++) {
            int px = x + f * (CS[k][0] * 13 / 10 + 4), py = y + CS[k][1] * 9 / 10;
            gfx_rect(px - 1, py - 1, 2, 2, age < 5 ? C_WHITE : C_PINK);
        }
    }
}

static void darkness(int px, int py, int r) {
    /* the Cloister's dark: only a circle round Clary or the ship shows */
    for (int y = 0; y < SCREEN_H; y++) {
        int dy = y - py, half = 0;
        if (iabs(dy) < r) half = clc_isqrt(r * r - dy * dy);
        if (half == 0) { gfx_darken_rect(0, y, SCREEN_W, 1, 7); continue; }
        gfx_darken_rect(0, y, imax(0, px - half), 1, 7);
        gfx_darken_rect(px + half, y, SCREEN_W - (px + half), 1, 7);
        if (half > 8) {
            gfx_darken_rect(px - half, y, 6, 1, 2);
            gfx_darken_rect(px + half - 6, y, 6, 1, 2);
        }
    }
}

void clc_draw_world(const ClcWorld *w, const ClcPlayer *p, int t) {
    const Theme *th = &THEME[w->region];
    int pxp = PX(w->on_foot ? w->cl.x : w->f.x), pyp = PX(w->on_foot ? w->cl.y - 1024 : w->f.y);
    int mapw = w->w * CLC_T, maph = w->h * CLC_T;
    int cx = iclamp(pxp - SCREEN_W / 2, 0, imax(0, mapw - SCREEN_W));
    int cy = iclamp(pyp - SCREEN_H / 2, w->kind == WK_ESCAPE ? -40 : 0, imax(0, maph - SCREEN_H));
    gfx_camera(0, 0);
    gfx_cls(th->bg);
    /* far scenery: bell frames in the dark, slower than the walls */
    for (int k = 0; k < 12; k++) {
        int bx = ((k * 97 - cx / 3) % 400 + 400) % 400 - 40, by = ((k * 61 - cy / 3) % 240 + 240) % 240 - 30;
        gfx_dither(bx, by, 22, 30, th->far, 3);
        gfx_dither_circle(bx + 11, by + 12, 6, th->bg, 8);
    }
    gfx_camera(cx, cy);
    draw_map_tiles(w, cx, cy, t);
    for (int k = 0; k < w->nd; k++) draw_door(w, &w->door[k], t);
    if (w->kind == WK_ESCAPE) {
        gfx_rect(0, -40, mapw, 40, C_INK);
        for (int i = 0; i < 20; i++) gfx_pset((i * 37) % mapw, -38 + (i * 11) % 36, C_WHITE);
    }
    for (int i = 0; i < w->ne; i++)
        if (w->e[i].on) draw_ent(w, &w->e[i], t);
    for (int k = 0; k < CLC_SHOTS; k++)
        if (w->shot[k].on) draw_shot(&w->shot[k], t);
    /* the ship, parked or flying */
    int sx = PX(w->f.x), sy = PX(w->f.y);
    if (w->ship != SM_GONE && !(p->dead && !w->on_foot)) {
        bool flame = !w->on_foot && (w->prev & BTN_A) && (p->fuel > 0 || p->coins > 0) && !p->dead;
        if (!(w->ship_inv && (t / 3) & 1)) clc_draw_ship(sx, sy - 1, w->f.face, flame, t);
        for (int k = 0; k < w->leeches; k++) gfx_circ(sx - 4 + k * 3, sy + 3, 2, C_MAGENTA);
        draw_slash(w, p, sx, sy);
        if (!w->on_foot && w->landing_icon >= 0) {
            /* the safe-landing mark on the floor below */
            int ly = w->landing_icon * CLC_T - 2;
            gfx_hline(sx - 4, sx + 3, ly, (t / 6) & 1 ? C_LIME : C_LEAF);
            gfx_pset(sx, ly - 2, C_LIME);
            gfx_pset(sx - 1, ly - 3, C_LIME);
            gfx_pset(sx + 1, ly - 3, C_LIME);
        }
    }
    if (w->on_foot && !(p->dead && w->on_foot && (t / 4) & 1)) {
        int frame = !w->cl.ground && !w->cl.ladder ? 2 : (w->cl.vx && (t / 6) & 1) ? 1 : 0;
        if (w->cl.ladder) frame = (PX(w->cl.y) / 4) & 1;
        if (!(w->cl.inv && (t / 3) & 1)) clc_draw_clary_small(PX(w->cl.x), PX(w->cl.y), w->cl.face, frame);
    }
    parts_draw();
    /* the gold door's pointer, round the ship */
    if (w->timer_on && w->kind == WK_GEN && w->gold_door < w->nd) {
        int gx, gy;
        clc_door_pad(&w->door[w->gold_door], &gx, &gy);
        gx = w->door[w->gold_door].c * CLC_T + 4;
        int ox = w->on_foot ? PX(w->cl.x) : sx, oy = w->on_foot ? PX(w->cl.y) - 6 : sy;
        int dx = gx - ox, dy = gy - oy, d = clc_isqrt(dx * dx + dy * dy);
        if (d > 20) {
            int ax = ox + dx * 14 / d, ay = oy + dy * 14 / d;
            gfx_rect(ax - 1, ay - 1, 3, 3, (t / 4) & 1 ? C_YELLOW : C_AMBER);
            gfx_pset(ax + dx * 3 / d, ay + dy * 3 / d, C_WHITE);
        }
    }
    gfx_camera(0, 0);
    if (w->dark) darkness(pxp - cx, pyp - cy, 48 + clc_sin(t * 3) * 2 / 127);
    /* the dowser: where the doors that aren't red are */
    if (clc_has(p, G_DOWSER)) {
        for (int k = 0; k < w->nd; k++) {
            const ClcDoor *d = &w->door[k];
            if (d->hidden || d->type == DT_RED) continue;
            int dx = d->c * CLC_T + 4 - cx, dy = d->r * CLC_T - cy;
            if (dx >= 0 && dx < SCREEN_W && dy >= 0 && dy < SCREEN_H) continue;
            int mx = iclamp(dx, 4, SCREEN_W - 5), my = iclamp(dy, 24, SCREEN_H - 5);
            int col = d->type == DT_GREEN ? C_LEAF : d->type == DT_BLUE ? C_SKY : d->type == DT_YELLOW ? C_YELLOW : C_AMBER;
            gfx_rect(mx - 2, my - 2, 5, 5, C_INK);
            gfx_rect(mx - 1, my - 1, 3, 3, col);
        }
    }
}

/* ---- behind the doors ------------------------------------------------------------- */

static void draw_sub_tiles(const ClcSub *s, int cam, int t) {
    const Theme *th = &THEME[s->region];
    int c0 = imax(0, cam / CLC_ST), c1 = imin(s->w - 1, (cam + SCREEN_W) / CLC_ST);
    for (int r = 0; r < CLC_SH; r++)
        for (int c = c0; c <= c1; c++) {
            int tt = s->tile[r][c], x = c * CLC_ST, y = r * CLC_ST + CLC_SOY;
            bool up_air = r > 0 && !clc_sub_solid_tile(s->tile[r - 1][c], true);
            switch (tt) {
            case ST_AIR: case ST_DOOR: case ST_EXIT: break;
            case ST_THIN:
                gfx_rect(x, y, 16, 4, th->light);
                gfx_hline(x, x + 15, y + 4, th->dark);
                for (int k = 2; k < 16; k += 6) gfx_vline(x + k, y + 4, y + 6, th->dark);
                break;
            case ST_BLOCK:
                gfx_rect(x, y, 16, 16, th->fill);
                gfx_rectb(x, y, 16, 16, th->dark);
                gfx_line(x + 3, y + 3, x + 8, y + 8, th->dark);
                gfx_line(x + 8, y + 8, x + 12, y + 5, th->dark);
                gfx_pset(x + 11, y + 11, C_YELLOW);
                break;
            case ST_SPIKE:
                gfx_rect(x, y + 10, 16, 6, th->dark);
                for (int k = 0; k < 4; k++) {
                    gfx_line(x + k * 4, y + 10, x + k * 4 + 2, y + 3, C_LIGHT);
                    gfx_line(x + k * 4 + 2, y + 3, x + k * 4 + 4, y + 10, C_LIGHT);
                }
                break;
            case ST_ACID:
                gfx_rect(x, y + 2, 16, 14, C_FOREST);
                for (int k = 0; k < 16; k++) gfx_pset(x + k, y + 2 + ((k + t / 6) % 3 == 0), C_LIME);
                break;
            default: {
                int fill = tt == ST_WALL2 ? th->dark : tt == ST_ICE ? C_ICE : th->fill;
                gfx_rect(x, y, 16, 16, fill);
                gfx_pset(x + ((c * 5 + r) % 12) + 2, y + ((c + r * 7) % 12) + 2, th->dark);
                gfx_pset(x + ((c * 3 + r * 5) % 12) + 2, y + ((c * 7 + r) % 12) + 2, th->light);
                if (up_air) {
                    gfx_hline(x, x + 15, y, tt == ST_ICE ? C_WHITE : th->rim);
                    gfx_hline(x, x + 15, y + 1, th->light);
                }
                break;
            }
            }
        }
}

static void draw_sub_door(int c, int col, bool open, int t) {
    int x = c * CLC_ST, y = (CLC_SH - 3) * CLC_ST + CLC_SOY;
    gfx_rect(x - 1, y - 2, 18, 34, C_INK);
    gfx_rect(x, y - 1, 16, 33, open ? col : C_SLATE);
    gfx_rect(x + 3, y + 2, 10, 30, open ? PAL_DARKER[col] : C_DUSK);
    if (open) gfx_pset(x + 11, y + 17, (t / 8) & 1 ? C_WHITE : C_YELLOW);
}

static void draw_npc(const ClcSub *s, const ClcEnt *e, int x, int y, int t) {
    /* x the middle, y the feet */
    switch (e->flag) {
    case 1: clc_spr(CS_TORTOISE, x - 8, y - 10, 0); if ((t / 40) & 1) text_draw("z", x + 6, y - 22 - (t / 10) % 4, C_WHITE); break;
    case 2: clc_spr(CS_SEXTON, x - 7, y - 22, 0); break;
    case 4: clc_spr(CS_KEEPER, x - 7, y - 21, 0); break;
    default: clc_spr(CS_VILLAGER, x - 7, y - 21, 0); break;
    }
    (void)s;
}

static void draw_sub_ent(const ClcSub *s, const ClcPlayer *p, const ClcEnt *e, int t) {
    int x = PX(e->x), y = PX(e->y) + CLC_SOY;
    int fl = e->dir < 0 ? SPR_FLIPX : 0;
    switch (e->kind) {
    case EK_STINGER: clc_spr(CS_STINGER, x - 7, y - 6, e->dir < 0 ? SPR_FLIPX : 0); break;
    case EK_DROPPER:
        gfx_circ(x, y, 6, C_HIDE);
        gfx_circ(x, y, 4, C_BROWN);
        gfx_pset(x - 2, y + 1, C_RED);
        gfx_pset(x + 2, y + 1, C_RED);
        if (!e->flag) gfx_vline(x, y - 10, y - 6, C_HIDE);
        break;
    case EK_LOUSE: clc_spr(CS_LOUSE, x - 5, y - 4, e->dir < 0 ? SPR_FLIPX : 0); break;
    case EK_JET:
        gfx_rect(x - 6, y + 2, 12, 4, C_SLATE);
        gfx_rect(x - 3, y - 2, 6, 4, C_GREY);
        if (e->flag)
            for (int yy = 2 * CLC_ST + CLC_SOY; yy < y - 2; yy += 3)
                gfx_hline(x - 4 + ((yy + t) % 3), x + 3 - ((yy + t) % 2), yy, (yy / 3 + t) % 3 ? C_ORANGE : C_YELLOW);
        else if ((t / 6) & 1) gfx_pset(x, y - 4, C_ORANGE);
        break;
    case EK_FLAME:
        gfx_rect(x - 6, y + 3, 12, 3, C_SLATE);
        if (e->flag)
            for (int k = 0; k < 6; k++) {
                int h = 10 + ((t + k * 5) % 8) + 12;
                gfx_vline(x - 5 + k * 2, y + 3 - h, y + 2, k & 1 ? C_ORANGE : C_YELLOW);
            }
        break;
    case EK_BRUTE: clc_spr(CS_BRUTE, x - 10, y - 13, e->dir < 0 ? SPR_FLIPX : 0); break;
    case EK_RINGWORM:
        gfx_rect(x - 5, y - 7, 10, 14, C_PINK);
        gfx_circ(x, y - 7, 5, C_PINK);
        gfx_circb(x, y - 7, 3, C_MAGENTA);
        gfx_pset(x, y - 7, C_INK);
        break;
    case EK_GOOPER:
        gfx_circ(x, y, 6, C_LIME);
        gfx_pset(x - 2, y - 2, C_INK);
        gfx_pset(x + 2, y - 2, C_INK);
        gfx_hline(x - 2, x + 2, y + 2, C_FOREST);
        break;
    case EK_AXER:
        gfx_rect(x - 5, y - 8, 10, 16, C_BROWN);
        gfx_rect(x - 4, y - 7, 8, 5, C_TAN);
        gfx_pset(x + e->dir * 2, y - 5, C_INK);
        gfx_line(x + e->dir * 5, y - 4, x + e->dir * 9, y - 9, C_GREY);
        break;
    case EK_AIRBOT:
        gfx_circ(x, y, 6, C_GREY);
        gfx_circ(x, y, 3, C_CYAN);
        gfx_hline(x - 8, x + 7, y - 7, (t / 2) & 1 ? C_LIGHT : C_SLATE);
        gfx_pset(x, y, C_WHITE);
        break;
    case EK_TROOPER: clc_spr(CS_TROOPER, x - 6, y - 9, e->dir < 0 ? SPR_FLIPX : 0); break;
    case EK_COCOON:
        gfx_circ(x, y, 6, C_CREAM);
        gfx_circ(x, y + 3, 5, C_TAN);
        gfx_vline(x, y - 12, y - 6, C_CREAM);
        break;
    case EK_BEE:
        gfx_circ(x, y, 3, C_YELLOW);
        gfx_vline(x, y - 2, y + 2, C_INK);
        gfx_pset(x - 2, y - 4 - ((t / 2) & 1), C_WHITE);
        gfx_pset(x + 2, y - 4 - ((t / 2) & 1), C_WHITE);
        break;
    case EK_SPEWER:
        gfx_rect(x - 7, y - 7, 14, 14, C_RED);
        gfx_rect(x - 4, y - 9, 8, 4, C_MAROON);
        gfx_circ(x + e->dir * 4, y - 2, 2, C_ORANGE);
        break;
    case EK_WHEEL:
        gfx_circ(x, y, 3, C_SLATE);
        for (int arm = 0; arm < 2; arm++)
            for (int k = 1; k <= 3; k++) {
                int ang = e->a + arm * 128;
                gfx_circ(x + clc_cos(ang) * k * 8 / 127, y + clc_sin(ang) * k * 8 / 127, 3, (k + t / 3) & 1 ? C_ORANGE : C_YELLOW);
            }
        break;
    case EK_GHORBNEST:
        gfx_rect(x - 8, y - 8, 16, 16, C_DUSK);
        gfx_rectb(x - 8, y - 8, 16, 16, C_VIOLET);
        gfx_circ(x, y, 3, (t / 6) & 1 ? C_VIOLET : C_PURPLE);
        break;
    case EK_GHORB:
        gfx_circ(x, y, 4, C_VIOLET);
        gfx_pset(x, y, C_WHITE);
        break;
    case EK_SKULL: clc_spr(CS_SKULL, x - 9, y - 9 + ((t / 8) & 1), 0); break;
    case EK_GWORM:
        for (int k = 0; k < 3; k++) gfx_circ(x - e->dir * k * 5, y + clc_sin(t * 6 + k * 40) * 2 / 127, 4 - k / 2, k ? C_LEAF : C_LIME);
        gfx_pset(x + e->dir * 2, y - 1, C_INK);
        break;
    case EK_FACE: clc_spr(CS_FACE, x - 10, y - 10, 0); break;
    case EK_BARREL:
        gfx_rect(x - 6, y - 8, 12, 16, C_RED);
        gfx_hline(x - 6, x + 5, y - 4, C_MAROON);
        gfx_hline(x - 6, x + 5, y + 3, C_MAROON);
        gfx_pset(x - 1, y, C_YELLOW);
        break;
    case EK_CHEST:
        gfx_rect(x - 9, y - 6, 18, 13, C_BROWN);
        gfx_rectb(x - 9, y - 6, 18, 13, C_INK);
        gfx_rect(x - 9, y - 7 - (e->flag ? 4 : 0), 18, 4, C_TAN);
        gfx_rect(x - 1, y - 2, 3, 3, C_YELLOW);
        break;
    case EK_ITEM: {
        int bob = clc_sin(t * 3 + x) * 2 / 127;
        clc_draw_item(e->a, x, y + bob, t);
        if (e->b > 0) {
            char b[8];
            snprintf(b, sizeof b, "%d", e->b);
            tiny_center(b, x, y + 11, s->bought || p->coins < e->b ? C_GREY : C_YELLOW);
        }
        break;
    }
    case EK_NPC: draw_npc(s, e, x, y + 12, t); break;
    case EK_SWITCH:
        gfx_rect(x - 3, y - 10, 6, 20, C_SLATE);
        gfx_line(x, y - 2, x + (s->lit_switch ? 6 : -6), y - 10, C_LIGHT);
        gfx_circ(x + (s->lit_switch ? 6 : -6), y - 10, 2, s->lit_switch ? C_LIME : C_RED);
        break;
    case EK_LOBBER: clc_spr(CS_LOBBER, x - 12, y - 18, e->dir > 0 ? SPR_FLIPX : 0); break;
    case EK_BOMB:
        gfx_circ(x, y, 5, C_NIGHT);
        gfx_pset(x - 2, y - 2, C_GREY);
        gfx_pset(x + 2, y - 5, (t / 3) & 1 ? C_YELLOW : C_RED);
        break;
    case EK_ENGINE: {
        bool open = true;
        for (int i = 0; i < s->ne; i++)
            if (s->e[i].on && s->e[i].kind == EK_LOBBER) open = false;
        gfx_rect(x - 16, y - 40, 32, 80, C_SLATE);
        gfx_rectb(x - 16, y - 40, 32, 80, C_INK);
        for (int k = 0; k < 4; k++) {
            int cy2 = y - 30 + k * 18;
            gfx_circ(x, cy2, 6, open ? ((t / 5 + k) & 1 ? C_MAGENTA : C_PURPLE) : C_DUSK);
            gfx_circb(x, cy2, 7, C_GREY);
        }
        gfx_hline(x - 16, x + 15, y - 40 + e->hp * 4, C_RED);
        break;
    }
    case EK_HUSH: {
        int dx = x, dy = y;
        gfx_rect(dx - 26, dy - 2, 52, 9, C_GREY);
        gfx_rect(dx - 22, dy + 6, 44, 3, C_SLATE);
        gfx_circ(dx, dy - 4, 10, C_ICE);
        gfx_circb(dx, dy - 4, 10, C_LIGHT);
        clc_draw_face(2, dx - 8, dy - 14, 1);
        for (int k = 0; k < 6; k++) gfx_pset(dx - 20 + k * 8, dy + 2, (t / 4 + k) & 1 ? C_YELLOW : C_RED);
        if (s->phase == 2 && s->phase_t >= 40)
            for (int k = 0; k < 9; k++) {
                gfx_line(dx - 22 + k * 5, dy + 9, dx - 20 + k * 5, dy + 14, C_LIGHT);
                gfx_line(dx - 18 + k * 5, dy + 9, dx - 20 + k * 5, dy + 14, C_LIGHT);
            }
        break;
    }
    case EK_HUSHGOOP:
        gfx_circ(x, y, 6, C_MAGENTA);
        gfx_pset(x - 2, y - 2, C_WHITE);
        gfx_pset(x + 2, y - 2, C_WHITE);
        break;
    case EK_TOCK: {
        int top = y - 32;
        gfx_rect(x - 32, top, 64, 32, C_BROWN);
        gfx_rectb(x - 32, top, 64, 32, C_INK);
        gfx_rect(x - 32, top, 64, 3, C_AMBER);
        for (int k = 0; k < 3; k++) {
            int gx = x - 20 + k * 20;
            int a = t * 6 * (k & 1 ? 1 : -1);
            gfx_circ(gx, top + 16, 7, C_TAN);
            gfx_line(gx, top + 16, gx + clc_cos(a) * 6 / 127, top + 16 + clc_sin(a) * 6 / 127, C_INK);
        }
        gfx_circ(x - 24, y - 2, 4, C_INK);
        gfx_circ(x + 24, y - 2, 4, C_INK);
        if (e->flag) gfx_hline(x - 32, x + 31, y - 1, (t / 2) & 1 ? C_RED : C_YELLOW);
        break;
    }
    case EK_TOCKHEAD:
        if (!e->flag) {
            gfx_rect(x - 8, y - 2, 16, 4, C_AMBER);
            gfx_rectb(x - 8, y - 2, 16, 4, C_INK);
        } else {
            clc_draw_face(3, x - 8, y - 9, 1);
        }
        break;
    case EK_MISSILE:
        if (e->t > 0 && e->t < 30 && PX(e->y) < 0) {
            gfx_rect(x - 1, CLC_SOY + 2, 3, 3, (t / 3) & 1 ? C_RED : C_ORANGE);
            break;
        }
        gfx_rect(x - 2, y - 5, 5, 10, C_GREY);
        gfx_pset(x, y + 5, C_RED);
        gfx_pset(x, y - 6, C_ORANGE);
        break;
    default: break;
    }
    (void)fl;
}

void clc_draw_sub(const ClcSub *s, const ClcPlayer *p, int t) {
    const Theme *th = &THEME[s->region];
    int cam = PX(s->cam);
    gfx_camera(0, 0);
    gfx_cls(th->bg);
    gfx_clip(0, CLC_SOY, SCREEN_W, SCREEN_H - CLC_SOY);
    /* the back wall */
    for (int k = 0; k < 10; k++) {
        int bx = ((k * 83 - cam / 2) % 400 + 400) % 400 - 40;
        gfx_dither(bx, CLC_SOY + 24, 28, 120, th->far, 2);
    }
    gfx_camera(cam, 0);
    draw_sub_tiles(s, cam, t);
    if (s->door_c >= 0) draw_sub_door(s->door_c, C_RED, !s->locked, t);
    if (s->exit_c >= 0 && (s->exit_open || s->room == RM_CAVE || s->room == RM_GOLDCAVE))
        draw_sub_door(s->exit_c, s->room == RM_GOLDCAVE || s->boss_kind >= 2 ? C_AMBER : C_RED, s->exit_open != 0, t);
    for (int i = 0; i < s->ne; i++)
        if (s->e[i].on) draw_sub_ent(s, p, &s->e[i], t);
    for (int k = 0; k < CLC_SSHOTS; k++)
        if (s->shot[k].on) {
            ClcShot sh = s->shot[k];
            sh.y += CLC_SOY * 256;
            draw_shot(&sh, t);
        }
    if (!(s->cl.inv && (t / 3) & 1) && !(p->dead && (t / 4) & 1)) {
        int frame = s->cl.crouch ? 3 : !s->cl.ground ? 2 : (s->cl.vx && (t / 7) & 1) ? 1 : 0;
        int aim = (s->prev & BTN_UP) ? 1 : (!s->cl.ground && (s->prev & BTN_DOWN)) ? 2 : 0;
        clc_draw_clary(PX(s->cl.x), PX(s->cl.y) + CLC_SOY, s->cl.face, frame, aim);
    }
    gfx_camera(cam, -CLC_SOY);
    parts_draw();
    gfx_camera(0, 0);
    gfx_noclip();
    /* a word from whoever she stands by */
    if (s->talk) {
        const char *line = NULL;
        char b[96];
        for (int i = 0; i < s->ne; i++) {
            const ClcEnt *e = &s->e[i];
            if (!e->on || e->kind != EK_NPC || iabs(PX(s->cl.x - e->x)) >= 30) continue;
            if (e->flag == 4) line = s->mega ? "NINE THINGS, ONE EACH VISIT. A TENTH ON TOP." : "ONE THING EACH VISIT, DEAR. NO MORE.";
            else if (e->flag == 2) {
                if (e->a == -2) line = "YOUR SHEETS ARE WHOLE. THE WAY TO TOCK IS OPEN.";
                else if (e->a >= 0 && e->a < RG_COUNT) {
                    snprintf(b, sizeof b, "TAKE THIS SHEET. THE NEXT SEXTON KEEPS %s.", CLC_REGION_NAME[e->a]);
                    line = b;
                }
            } else if (e->flag == 1 && s->npc_shots >= 100) line = "...ZZZ... FULL-PEAL ...ZZZ...";
            else if (e->flag == 1) line = "...ZZZ...";
            else if (e->a >= 0 && e->a < CLC_NPC_LINES) line = CLC_NPC_LINE[e->a];
        }
        if (line) {
            int w = imin(312, tiny_width(line) + 10);
            gfx_rect(160 - w / 2, CLC_SOY + 4, w, 11, C_INK);
            gfx_rectb(160 - w / 2, CLC_SOY + 4, w, 11, C_LIGHT);
            tiny_center(line, 160, CLC_SOY + 7, C_WHITE);
        }
    }
    /* a boss's bar */
    if (s->boss_kind >= 2 && s->boss_max > 0) {
        int bw = 120;
        gfx_rect(100, SCREEN_H - 10, bw + 2, 6, C_INK);
        gfx_rect(101, SCREEN_H - 9, bw * s->boss_hp / s->boss_max, 4, s->boss_kind == 2 ? C_MAGENTA : C_AMBER);
    }
}

/* ---- the HUD ------------------------------------------------------------------------ */

void clc_draw_hud(const ClcPlayer *p, int notes, int timer, bool timer_on, int t) {
    gfx_camera(0, 0);
    gfx_rect(0, 0, 112, 19, C_INK);
    /* the bar: red segments, one per point */
    int segs = p->hpmax / 2, lit = (p->hp + 1) / 2;
    for (int k = 0; k < segs; k++) {
        int col = k < lit ? (p->hp <= 4 && (t / 8) & 1 ? C_ORANGE : C_RED) : C_MAROON;
        gfx_rect(2 + k * 4, 2, 3, 5, col);
    }
    /* the tank: green */
    int fw = imin(100, p->fuelmax / 16), fl = p->fuelmax ? p->fuel * fw / p->fuelmax : 0;
    gfx_rect(2, 9, fw, 3, C_FOREST);
    gfx_rect(2, 9, fl, 3, p->fuel < 160 && (t / 6) & 1 ? C_YELLOW : C_LIME);
    /* coins */
    char b[16];
    snprintf(b, sizeof b, GLYPH_COIN "%ld", (long)p->coins);
    tiny_draw(b, 2, 14, C_YELLOW);
    /* the notes: five, each filled by halves */
    for (int k = 0; k < 5; k++) {
        int x = 46 + k * 9, y = 13, have = notes - k * 2;
        gfx_rectb(x, y, 7, 5, C_PURPLE);
        if (have >= 1) gfx_rect(x + 1, y + 1, 3, 3, C_VIOLET);
        if (have >= 2) gfx_rect(x + 3, y + 1, 3, 3, C_VIOLET);
    }
    if (p->key) clc_draw_item(IT_KEY, 104, 9, 0);
    /* the dash */
    if (timer_on) {
        int shown = (timer + 29) / 30;
        snprintf(b, sizeof b, "%02d", shown);
        gfx_rect(146, 1, 28, 13, C_INK);
        text_draw_scaled(b, 149, 2, shown <= 10 && (t / 6) & 1 ? C_RED : C_WHITE, 1);
    }
}

/* ---- the station map --------------------------------------------------------------- */

void clc_draw_station(int region_sel, const uint8_t *visited, int cur, int t) {
    gfx_cls(C_INK);
    for (int i = 0; i < 50; i++) gfx_pset((i * 97 + 13) % 320, (i * 53 + 7) % 180, i % 5 ? C_DUSK : C_GREY);
    /* the Carillon in section: the Cellars at the foot, the Spire at the top */
    static const int16_t POS[RG_COUNT][2] = {{160, 132}, {112, 100}, {208, 100}, {96, 66}, {160, 66}, {224, 66}, {160, 32}};
    gfx_rect(70, 24, 180, 124, C_NIGHT);
    gfx_rectb(70, 24, 180, 124, C_DUSK);
    for (int r = 0; r < RG_COUNT; r++) {
        for (int q = 0; q < RG_COUNT; q++) {
            if (clc_region_tier(q) != clc_region_tier(r) + 1) continue;
            gfx_line(POS[r][0], POS[r][1], POS[q][0], POS[q][1], C_DUSK);
        }
    }
    for (int r = 0; r < RG_COUNT; r++) {
        int x = POS[r][0], y = POS[r][1];
        bool sel = r == region_sel;
        int col = visited[r] ? CLC_REGION_COL[r][0] : CLC_REGION_COL[r][1];
        gfx_rect(x - 22, y - 9, 44, 18, sel && (t / 8) & 1 ? C_WHITE : C_INK);
        gfx_rect(x - 21, y - 8, 42, 16, col);
        if (!visited[r]) gfx_dither(x - 21, y - 8, 42, 16, C_INK, 8);
        const char *nm = CLC_REGION_NAME[r];
        if (!strncmp(nm, "THE ", 4)) nm += 4;
        tiny_center(nm, x, y - 2, sel ? C_YELLOW : C_WHITE);
        if (r == cur) clc_draw_ship(x + 26, y, -1, (t / 4) & 1, t);
    }
}
