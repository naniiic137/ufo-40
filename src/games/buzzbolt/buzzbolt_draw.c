/* BUZZBOLT - drawing: the sky, the Blight and its bosses, the Hive Wing,
 * the letters, the HUD and every screen between. */
#include "buzzbolt.h"

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

/* ------------------------------------------------------------------------ */
/* the sky                                                                  */

typedef struct { uint8_t bg, blob, far, near, dust; } Sky;
static const Sky SKY[BZZ_WAVES] = {
    {C_NAVY, C_BLUE, C_SKY, C_ICE, C_AMBER},       /* outer meadow: pollen */
    {C_NIGHT, C_FOREST, C_JADE, C_LEAF, C_LIME},   /* the thicket */
    {C_INK, C_NIGHT, C_DUSK, C_SLATE, C_PURPLE},   /* the rot */
    {C_NIGHT, C_PURPLE, C_DUSK, C_GREY, C_WINE},   /* the walls */
    {C_INK, C_MAROON, C_WINE, C_PINK, C_MAGENTA},  /* the heart */
};

static void draw_sky(int wave, float scroll) {
    const Sky *s = &SKY[iclamp(wave, 0, BZZ_WAVES - 1)];
    gfx_cls(s->bg);
    for (int i = 0; i < 6; i++) {
        int x = (i * 97 + 30) % 340 - 10, y = ((int)(i * 61 + scroll * 0.15f)) % 280 - 50;
        gfx_dither_circle(x, y, 26 + i * 6, s->blob, 2);
    }
    for (int i = 0; i < 48; i++) {
        int x = (i * 53 + 17) % BZZ_W, y = ((int)(i * 37 + scroll * 0.4f)) % BZZ_H;
        gfx_pset(x, y, s->far);
    }
    for (int i = 0; i < 26; i++) {
        int x = (i * 89 + 5) % BZZ_W, y = ((int)(i * 71 + scroll)) % BZZ_H;
        gfx_pset(x, y, s->near);
    }
    for (int i = 0; i < 12; i++) {
        int x = (i * 131 + 40) % BZZ_W, y = ((int)(i * 43 + scroll * 2.2f)) % 200 - 10;
        gfx_vline(x, y, y + 3, s->dust);
    }
}

/* ------------------------------------------------------------------------ */
/* foes                                                                     */

static void spr_c(int id, float x, float y, int flags, bool white) {
    const Sprite *s = &bzz_spr[id];
    int dx = (int)lroundf(x) - s->w / 2, dy = (int)lroundf(y) - s->h / 2;
    if (white) spr_draw_ex(s, dx, dy, flags, NULL, C_WHITE);
    else spr_draw(s, dx, dy, flags);
}

static void draw_puff(int x, int y, int r, bool white, int t) {
    gfx_circ(x, y, r + 1, C_SLATE);
    gfx_circ(x, y, r, white ? C_WHITE : C_PURPLE);
    if (white) return;
    gfx_dither_circle(x - r / 3, y - r / 3, r / 2 + 1, C_VIOLET, 8);
    for (int k = 0; k < 4; k++) {
        float a = k * 1.7f + t * 0.01f;
        gfx_pset(x + (int)(cosf(a) * r * 0.6f), y + (int)(sinf(a) * r * 0.6f), C_MAGENTA);
    }
}

static void draw_wall(const Foe *e) {
    int x = (int)e->x - 9, y = (int)e->y - 5;
    if (!e->state) {
        /* a gate that is off: a dotted outline */
        for (int i = 0; i < 18; i += 3) { gfx_pset(x + i, y, C_DUSK); gfx_pset(x + i, y + 9, C_DUSK); }
        gfx_pset(x, y + 4, C_DUSK);
        gfx_pset(x + 17, y + 4, C_DUSK);
        return;
    }
    bool w = e->flash > 0;
    gfx_rect(x, y, 18, 10, w ? C_WHITE : C_WINE);
    if (w) return;
    gfx_hline(x, x + 17, y, C_MAGENTA);
    gfx_hline(x, x + 17, y + 9, C_MAROON);
    gfx_dither(x + 2, y + 2, 14, 6, C_PURPLE, 6);
    gfx_pset(x + 4 + (e->idx * 5) % 9, y + 4, C_PINK);
    if (e->sub > 0) {
        /* a gate: its lamp shows it is one */
        gfx_rect(x + 7, y + 3, 4, 4, (e->pt % 20) < 10 ? C_YELLOW : C_AMBER);
    }
    if (e->hp < e->maxhp / 2) gfx_line(x + 3, y + 2, x + 9, y + 8, C_INK);
}

static void draw_bloatfly(const Foe *e, bool w, int t) {
    int x = (int)e->x, y = (int)e->y;
    int flap = (t / 3) % 2 ? -3 : 2;
    gfx_dither_circle(x - 19, y - 8 + flap, 13, w ? C_WHITE : C_LIGHT, 7);
    gfx_dither_circle(x + 19, y - 8 + flap, 13, w ? C_WHITE : C_LIGHT, 7);
    gfx_circb(x - 19, y - 8 + flap, 13, C_GREY);
    gfx_circb(x + 19, y - 8 + flap, 13, C_GREY);
    gfx_circ(x, y - 4, 13, w ? C_WHITE : C_DUSK);
    if (!w) for (int k = -2; k <= 2; k++) gfx_hline(x - 11 + iabs(k) * 2, x + 11 - iabs(k) * 2, y - 4 + k * 4, C_SLATE);
    gfx_circ(x, y + 9, 8, w ? C_WHITE : C_NIGHT);
    gfx_circ(x - 5, y + 10, 4, C_RED);
    gfx_circ(x + 5, y + 10, 4, C_RED);
    gfx_pset(x - 6, y + 8, C_PINK);
    gfx_pset(x + 4, y + 8, C_PINK);
    for (int k = -1; k <= 1; k += 2) {
        gfx_line(x + k * 8, y + 4, x + k * 16, y + 14, C_INK);
        gfx_line(x + k * 16, y + 14, x + k * 18, y + 20, C_INK);
    }
    gfx_rect(x - 1, y + 16, 3, 4, C_MAROON); /* the mouth that spits */
}

static void draw_tick(const Foe *e, bool w, int t) {
    int x = (int)e->x, y = (int)e->y;
    for (int k = 0; k < 4; k++) {
        int sway = (int)(sinf(t * 0.2f + k) * 3);
        int ly = y - 8 + k * 6;
        gfx_line(x - 18, ly, x - 30, ly - 4 + sway, C_INK);
        gfx_line(x - 30, ly - 4 + sway, x - 34, ly + 6 + sway, C_INK);
        gfx_line(x + 18, ly, x + 30, ly - 4 - sway, C_INK);
        gfx_line(x + 30, ly - 4 - sway, x + 34, ly + 6 - sway, C_INK);
    }
    int body = w ? C_WHITE : C_MAROON;
    gfx_circ(x - 11, y - 1, 12, body);
    gfx_circ(x + 11, y - 1, 12, body);
    gfx_circ(x, y, 15, body);
    if (!w) {
        gfx_dither_circle(x, y - 4, 12, C_WINE, 9);
        gfx_circ(x - 8, y - 5, 2, C_PINK);
        gfx_circ(x + 9, y - 3, 2, C_PINK);
        gfx_circ(x, y + 3, 2, C_PINK);
        gfx_circb(x, y, 15, C_INK);
    }
    gfx_circ(x, y + 15, 5, w ? C_WHITE : C_WINE);
    gfx_line(x - 3, y + 18, x - 6, y + 23, C_INK);
    gfx_line(x + 3, y + 18, x + 6, y + 23, C_INK);
    gfx_pset(x - 2, y + 14, C_YELLOW);
    gfx_pset(x + 2, y + 14, C_YELLOW);
}

static void draw_scythe(const Foe *e, bool w, int t) {
    int x = (int)e->x, y = (int)e->y;
    int swing = (int)(sinf(t * 0.08f) * 4);
    int c1 = w ? C_WHITE : C_JADE, c2 = w ? C_WHITE : C_FOREST;
    gfx_dither_circle(x - 12, y - 8, 10, C_LEAF, 5);
    gfx_dither_circle(x + 12, y - 8, 10, C_LEAF, 5);
    gfx_rect(x - 4, y - 16, 9, 22, c2);
    gfx_rect(x - 3, y - 15, 7, 20, c1);
    gfx_line(x - 5, y + 6, x, y + 14, c1);
    gfx_line(x + 5, y + 6, x, y + 14, c1);
    gfx_line(x - 4, y + 7, x + 4, y + 7, c1);
    gfx_pset(x - 3, y + 8, C_YELLOW);
    gfx_pset(x + 3, y + 8, C_YELLOW);
    for (int k = -1; k <= 1; k += 2) {
        int ex = x + k * 16, ey = y + 2 + swing * k;
        gfx_line(x + k * 4, y - 4, ex, ey, c1);
        gfx_line(ex, ey, ex - k * 2, ey + 12, C_LIME);
        gfx_line(ex - k * 2, ey + 12, ex - k * 7, ey + 14, C_LIME);
        gfx_line(ex + k, ey, ex - k, ey + 12, c2);
    }
}

static void draw_dustwing(const Foe *e, bool w, int t) {
    int x = (int)e->x, y = (int)e->y;
    int flap = (int)(sinf(t * 0.2f) * 3);
    int wc = w ? C_WHITE : C_TAN, wd = w ? C_WHITE : C_EARTH;
    gfx_circ(x - 13, y - 4 - flap, 11, wc);
    gfx_circ(x + 13, y - 4 - flap, 11, wc);
    gfx_circ(x - 10, y + 7 + flap, 8, wd);
    gfx_circ(x + 10, y + 7 + flap, 8, wd);
    if (!w) {
        gfx_circ(x - 14, y - 5 - flap, 4, C_CREAM);
        gfx_circ(x + 14, y - 5 - flap, 4, C_CREAM);
        gfx_circ(x - 14, y - 5 - flap, 2, C_INK);
        gfx_circ(x + 14, y - 5 - flap, 2, C_INK);
        gfx_dither_circle(x, y, 18, C_HIDE, 3);
    }
    gfx_rect(x - 3, y - 12, 7, 24, w ? C_WHITE : C_BROWN);
    gfx_line(x - 2, y - 12, x - 7, y - 18, C_BROWN);
    gfx_line(x + 2, y - 12, x + 7, y - 18, C_BROWN);
    gfx_pset(x - 2, y + 10, C_RED);
    gfx_pset(x + 2, y + 10, C_RED);
}

static void draw_sporeheart(const Foe *e, bool w, int t) {
    int x = (int)e->x, y = (int)e->y;
    int stage = e->hp * 3 > e->maxhp * 2 ? 0 : e->hp * 3 > e->maxhp ? 1 : 2;
    /* tendrils */
    for (int k = 0; k < 7; k++) {
        float a = 0.5f + k * 0.35f;
        int px = x, py = y + 8;
        for (int s = 1; s <= 6; s++) {
            int nx = x + (int)(cosf(a) * s * 7 + sinf(t * 0.07f + k + s * 0.7f) * s * 0.8f);
            int ny = y + 8 + (int)(sinf(a) * s * 6);
            gfx_line(px, py, nx, ny, stage == 2 ? C_RED : C_WINE);
            px = nx;
            py = ny;
        }
    }
    int cap = w ? C_WHITE : stage == 0 ? C_WINE : stage == 1 ? C_PURPLE : C_MAROON;
    gfx_circ(x, y - 4, 26, cap);
    gfx_rect(x - 27, y - 4, 55, 10, w ? C_WHITE : C_MAROON);
    if (!w) {
        gfx_dither_circle(x - 6, y - 12, 14, C_MAGENTA, 5);
        for (int k = 0; k < 9; k++) gfx_circ(x - 18 + (k * 37) % 36, y - 20 + (k * 13) % 16, 2, C_PINK);
        for (int k = -24; k <= 24; k += 4) gfx_vline(x + k, y + 1, y + 5, C_INK);
    }
    /* the beating heart underneath */
    int beat = (t / 8) % 4 == 0 ? 7 : 5;
    gfx_circ(x, y + 12, beat + 1, C_MAROON);
    gfx_circ(x, y + 12, beat, stage == 2 ? C_YELLOW : C_PINK);
    gfx_circ(x, y + 12, beat - 3, C_WHITE);
}

static void draw_foes(void) {
    for (int i = 0; i < BZZ_MAX_FOES; i++) {
        Foe *e = &bz.foe[i];
        if (!e->alive || e->t < 0) continue;
        /* a hit shows as a white flash; a big foe under steady fire only
         * flickers now and then, so it stays readable */
        bool w = e->role == ROLE_FOE ? e->flash > 0 : e->flash >= 3 && bz.frame_t % 3 == 0;
        int t = e->t + i * 7;
        switch (e->kind) {
        case EK_GNAT: spr_c((t / 3) % 2 ? SP_GNAT1 : SP_GNAT2, e->x, e->y, 0, w); break;
        case EK_MIDGE: spr_c((t / 3) % 2 ? SP_MIDGE1 : SP_MIDGE2, e->x, e->y, 0, w); break;
        case EK_WHIRLER: spr_c((t / 3) % 2 ? SP_WHIRL1 : SP_WHIRL2, e->x, e->y, (t / 6) % 2 ? SPR_FLIPX : 0, w); break;
        case EK_CRICKET: spr_c(SP_CRICKET, e->x, e->y, (t / 10) % 2 ? SPR_FLIPX : 0, w); break;
        case EK_BLISTER: spr_c(SP_BLISTER, e->x, e->y, (t / 12) % 2 ? SPR_FLIPX : 0, w); break;
        case EK_GOLDBUG: spr_c((t / 3) % 2 ? SP_GOLD1 : SP_GOLD2, e->x, e->y, 0, w); break;
        case EK_IRONBACK: spr_c(SP_IRONBACK, e->x, e->y, SPR_FLIPY, w); break;
        case EK_BIGPUFF: draw_puff((int)e->x, (int)e->y, 9, w, t); break;
        case EK_PUFF: draw_puff((int)e->x, (int)e->y, 4, w, t); break;
        case EK_ROTWALL: draw_wall(e); break;
        case EK_BLOATFLY: draw_bloatfly(e, w, t); break;
        case EK_TICK: draw_tick(e, w, t); break;
        case EK_SCYTHE: draw_scythe(e, w, t); break;
        case EK_DUSTWING: draw_dustwing(e, w, t); break;
        default: draw_sporeheart(e, w, t); break;
        }
    }
}

/* ------------------------------------------------------------------------ */
/* shots, letters, the ship                                                 */

static void draw_eshots(void) {
    for (int i = 0; i < BZZ_MAX_ESHOTS; i++) {
        EShot *s = &bz.es[i];
        if (!s->alive) continue;
        int x = (int)lroundf(s->x), y = (int)lroundf(s->y);
        switch (s->kind) {
        case ES_HOMING:
            gfx_circ(x, y, 3, C_FOREST);
            gfx_circ(x, y, 2, C_LEAF);
            gfx_pset(x, y, C_LIME);
            break;
        case ES_BIG:
            gfx_circ(x, y, 5, C_FOREST);
            gfx_circ(x, y, 4, C_LEAF);
            gfx_circ(x, y, 2, C_LIME);
            break;
        case ES_RING:
            gfx_circb(x, y, 5, s->bounced ? C_ORANGE : C_RED);
            gfx_circb(x, y, 4, C_YELLOW);
            break;
        case ES_SPORE:
            gfx_circ(x, y, 3, C_PURPLE);
            gfx_circ(x, y, 2, C_MAGENTA);
            gfx_pset(x, y, C_PINK);
            break;
        default:
            gfx_circ(x, y, 2, C_RED);
            gfx_rect(x - 1, y - 1, 2, 2, C_PINK);
            gfx_pset(x, y, C_WHITE);
            break;
        }
    }
}

static void draw_pshots(void) {
    for (int i = 0; i < BZZ_MAX_PSHOTS; i++) {
        PShot *s = &bz.ps[i];
        if (!s->alive) continue;
        int x = (int)lroundf(s->x), y = (int)lroundf(s->y);
        switch (s->kind) {
        case PS_WAVE: gfx_rect(x - 1, y - 3, 2, 6, C_LIME); gfx_pset(x, y - 3, C_WHITE); break;
        case PS_CRESCENT: {
            int w = (int)s->w;
            for (int k = -w; k <= w; k++) {
                int dy = (k * k) / (w + 2);
                gfx_pset(x + k, y + dy, C_YELLOW);
                gfx_pset(x + k, y + dy + 1, C_AMBER);
            }
            break;
        }
        case PS_LANCE: {
            int w = (int)s->w, h = (int)s->h;
            gfx_rect(x - w, y - h, w * 2, h * 2, C_CYAN);
            gfx_rect(x - w / 2, y - h, imax(1, w), h * 2, C_WHITE);
            break;
        }
        case PS_ROCKET: gfx_rect(x - 1, y - 2, 3, 4, C_ORANGE); gfx_pset(x, y + 2, C_YELLOW); break;
        case PS_ALLY: gfx_circ(x, y, 2, C_PINK); gfx_pset(x, y, C_WHITE); break;
        default:
            if (bz.ship == BZ_FIREFLY) { gfx_rect(x - 1, y - 3, 2, 6, C_ORANGE); gfx_pset(x, y - 3, C_YELLOW); }
            else if (bz.ship == BZ_SHIELDBUG) { gfx_rect(x - 1, y - 1, 3, 3, bz.power_t > 0 ? C_WHITE : C_LIME); }
            else { gfx_rect(x - 1, y - 3, 2, 7, C_ICE); gfx_pset(x, y - 3, C_WHITE); }
            break;
        }
    }
}

static void draw_letter_box(int x, int y, int letter) {
    bool b = letter == LT_B;
    ui_panel(x, y, 9, 9, b ? C_AMBER : C_SKY, b ? C_BROWN : C_NAVY);
    text_draw(b ? "B" : "Z", x + 2, y + 1, C_INK);
}

static void draw_letters(void) {
    for (int i = 0; i < BZZ_MAX_LETTERS; i++) {
        Letter *l = &bz.lt[i];
        if (!l->alive) continue;
        draw_letter_box((int)lroundf(l->x) - 4, (int)lroundf(l->y) - 4, l->letter);
    }
}

static void draw_ship_at(int ship, int x, int y, int t) {
    spr_c(SP_LACEWING + ship, (float)x, (float)y, 0, false);
    /* engine glow */
    int f = (t / 2) % 3;
    gfx_vline(x, y + 7, y + 8 + f, ship == BZ_FIREFLY ? C_YELLOW : ship == BZ_SHIELDBUG ? C_LIME : C_CYAN);
}

static void draw_ship(void) {
    if (!bz.alive) return;
    int x = (int)lroundf(bz.px), y = (int)lroundf(bz.py);
    /* the lacewing's lance glowing as it charges */
    if (bz.ship == BZ_LACEWING && bz.charge >= BZZ_CHARGE_MIN) {
        int r = 2 + bz.charge / 30;
        gfx_circb(x, y - 10, r + ((bz.frame_t / 3) % 2), bz.charge >= BZZ_CHARGE_MAX ? C_WHITE : C_CYAN);
    }
    /* the firefly's laser */
    if (bz.ship == BZ_FIREFLY && bz.focused && bz.laser_t > 0) {
        int top = y - 8 - bz.laser_len;
        int wob = (bz.frame_t / 2) % 2;
        gfx_rect(x - 3 - wob, top, 7 + wob * 2, y - 8 - top, C_ORANGE);
        gfx_rect(x - 1, top, 3, y - 8 - top, C_YELLOW);
        gfx_vline(x, top, y - 8, C_WHITE);
    }
    if (bz.inv > 0 && (bz.inv / 3) % 2) return;
    draw_ship_at(bz.ship, x, y, bz.frame_t);
}

static void draw_allies(void) {
    static const int OPT_SPR[BZ_SHIPS] = {SP_OPT_LACE, SP_OPT_SHIELD, SP_OPT_FIRE};
    for (int i = 0; i < bz.nopt; i++) {
        Option *o = &bz.opt[i];
        spr_c(OPT_SPR[bz.ship], o->x, o->y, 0, o->hp * 3 < o->maxhp && (bz.frame_t / 4) % 2);
    }
    if (bz.alive)
        for (int o = 0; o < 2; o++) {
            if (!bz.orb[o]) continue;
            float a = bz.orb_a + o * 3.14159f;
            int ox = (int)(bz.px + cosf(a) * 17), oy = (int)(bz.py + sinf(a) * 17);
            gfx_circ(ox, oy, 3, C_BLUE);
            gfx_circ(ox, oy, 2, C_CYAN);
            gfx_pset(ox, oy, C_WHITE);
        }
    /* the firefly's bombs trail behind it */
    if (bz.alive)
        for (int k = 0; k < bz.bombs; k++) spr_c(SP_BOMB, bz.px + (k - (bz.bombs - 1) / 2.0f) * 9, bz.py + 14 + k % 2 * 2, 0, false);
    for (int i = 0; i < BZZ_MAX_BOMBS; i++)
        if (bz.bomb[i].alive) spr_c(SP_BOMB, bz.bomb[i].x, bz.bomb[i].y, 0, (bz.bomb[i].t / 4) % 2);
    for (int i = 0; i < BZZ_MAX_BLASTS; i++) {
        Blast *b = &bz.blast[i];
        if (!b->alive) continue;
        gfx_dither_circle((int)b->x, (int)b->y, (int)b->r, C_SKY, 6);
        gfx_circb((int)b->x, (int)b->y, (int)b->r, C_CYAN);
        gfx_circb((int)b->x, (int)b->y, (int)b->r - 2, C_BLUE);
    }
    if (bz.hive.on) {
        int x = (int)bz.hive.x, y = (int)bz.hive.y;
        if (bz.hive.t >= 22 && bz.hive.t < 170) {
            int wob = (bz.frame_t / 2) % 2;
            gfx_rect(x - 7 - wob, 0, 14 + wob * 2, y - 8, C_AMBER);
            gfx_rect(x - 4, 0, 8, y - 8, C_YELLOW);
            gfx_rect(x - 1, 0, 3, y - 8, C_WHITE);
        }
        /* the hiveship: a raft of honeycomb */
        for (int k = 0; k < 7; k++) {
            int cx = x + (k - 3) * 9, cy = y + (k % 2) * 5;
            gfx_circ(cx, cy, 5, C_BROWN);
            gfx_circ(cx, cy, 4, k == 3 ? C_YELLOW : C_AMBER);
            gfx_circb(cx, cy, 4, C_TAN);
        }
        gfx_rect(x - 3, y - 10, 7, 6, C_CREAM);
    }
    if (bz.fly.on) spr_c(SP_DRAGONFLY, bz.fly.x, bz.fly.y, (bz.fly.t / 4) % 2 ? 0 : SPR_FLIPY, false);
}

static void draw_parts(void) {
    for (int i = 0; i < BZZ_MAX_PARTS; i++) {
        Part *p = &bz.part[i];
        if (p->life <= 0) continue;
        gfx_pset((int)p->x, (int)p->y, p->life > 6 ? p->col : C_DUSK);
    }
}

/* ------------------------------------------------------------------------ */
/* the HUD                                                                  */

static void draw_hud(void) {
    gfx_rect(0, 0, BZZ_W, BZZ_HUD_H, C_INK);
    gfx_hline(0, BZZ_W - 1, BZZ_HUD_H, C_NIGHT);
    /* the three letter slots; a word just spelt stays in them a moment */
    bool flash = bz.word_t > 0 && bz.nword == 0 && bz.last_word != W_NONE;
    for (int k = 0; k < 3; k++) {
        int x = 2 + k * 10;
        if (flash) {
            draw_letter_box(x, 1, bz.shown[k]);
            if ((bz.word_t / 4) % 2) gfx_rectb(x, 1, 9, 9, bz.last_word == W_WRONG ? C_RED : C_WHITE);
        } else if (k < bz.nword) draw_letter_box(x, 1, bz.word[k]);
        else ui_panel(x, 1, 9, 9, C_NIGHT, C_DUSK);
    }
    char buf[32];
    snprintf(buf, sizeof buf, "X%d", bz.mult);
    text_draw(buf, 34, 2, bz.mult >= 10 ? C_YELLOW : C_LIGHT);
    const char *sc = num(bz.score);
    text_draw(sc, 160 - text_width(sc) / 2, 2, C_WHITE);
    /* spare ships, from the right */
    int rx = BZZ_W - 4;
    for (int k = 0; k < imin(bz.lives, 6); k++) {
        rx -= 8;
        gfx_rect(rx + 2, 2, 3, 6, C_LIGHT);
        gfx_rect(rx, 5, 7, 2, C_LIGHT);
    }
    if (bz.lives > 6) { snprintf(buf, sizeof buf, "%d", bz.lives); rx -= tiny_width(buf) + 2; tiny_draw(buf, rx, 3, C_LIGHT); }
}

static void draw_banner(void) {
    if (bz.wave_t >= 20) return;
    char buf[32];
    snprintf(buf, sizeof buf, "WAVE %d", bz.wave + 1);
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER};
    ui_fancy_center(buf, 160, 66, 2, grad, 3, C_INK, C_MAROON);
    text_center(BZZ_WAVE[bz.wave].name, 160, 88, C_LIGHT);
}

static void draw_play(void) {
    int sx = bz.shake > 0 ? (bz.frame_t % 3) - 1 : 0, sy = bz.shake > 6 ? ((bz.frame_t / 2) % 3) - 1 : 0;
    draw_sky(bz.wave, bz.scroll);
    gfx_camera(sx, sy);
    draw_foes();
    draw_letters();
    draw_pshots();
    draw_allies();
    draw_ship();
    draw_eshots();
    draw_parts();
    gfx_camera(0, 0);
    if (bz.flash > 0) gfx_dither(0, 0, BZZ_W, BZZ_H, C_WHITE, bz.flash);
    draw_hud();
    draw_banner();
}

/* ------------------------------------------------------------------------ */
/* the screens                                                              */

static const uint8_t LOGO[] = {C_CREAM, C_YELLOW, C_YELLOW, C_AMBER, C_AMBER, C_ORANGE};

/* The opening, before the title: the Nebula Garden in bloom; the Blight
 * seeping down over it and its flowers wilting; the hive opening and the
 * Hive Wing flying out to meet it. Any button skips it. */
#define INTRO_S1 220
#define INTRO_S2 430
static void draw_intro(void) {
    int t = bz.state_t;
    int scene = t < INTRO_S1 ? 0 : t < INTRO_S2 ? 1 : 2;
    draw_sky(scene == 0 ? 0 : 2, bz.scroll);
    /* how far the Blight has crept, left to right, over the flowers */
    int creep = scene == 0 ? -1 : scene == 1 ? (t - INTRO_S1) * 2 : 999;
    if (scene >= 1) {
        int depth = scene == 1 ? imin(96, (t - INTRO_S1) / 2) : 96;
        gfx_dither(0, 0, BZZ_W, depth, C_PURPLE, 5);
        gfx_dither(0, 0, BZZ_W, depth / 2, C_MAROON, 6);
        for (int k = 0; k < 14; k++) {
            int gx = ((k * 61 + t * (k % 2 ? 1 : -1) * (1 + k % 3)) % 360 + 360) % 360 - 20;
            int gy = 12 + (k * 23) % imax(8, depth) + (int)(sinf(t * 0.1f + k) * 3);
            spr_c((t / 3 + k) % 2 ? SP_GNAT1 : SP_GNAT2, (float)gx, (float)gy, 0, false);
        }
    }
    gfx_rect(0, 152, BZZ_W, 28, C_FOREST);
    for (int i = 0; i < BZZ_W; i += 3) gfx_pset(i, 152 + (i * 7) % 5, C_JADE);
    for (int k = 0; k < 14; k++) {
        int x = 12 + k * 23, h = 14 + (k * 7) % 9;
        bool wilt = creep > x;
        int sway = wilt ? 3 : (int)(sinf(t * 0.04f + k) * 1.5f);
        int top = 152 - h + (wilt ? 5 : 0);
        gfx_line(x, 152, x + sway, top, wilt ? C_EARTH : C_JADE);
        int c = wilt ? C_GREY : k % 3 == 0 ? C_ICE : k % 3 == 1 ? C_PINK : C_YELLOW;
        gfx_circ(x + sway, top, wilt ? 2 : 3, c);
        if (!wilt) gfx_pset(x + sway, top, C_AMBER);
    }
    if (scene == 2) {
        /* the hive opens, and the three ships climb out of it */
        int u = t - INTRO_S2;
        for (int k = 0; k < 7; k++) {
            int cx = 160 + (k - 3) * 9, cy = 160 + (k % 2) * 5;
            gfx_circ(cx, cy, 5, C_BROWN);
            gfx_circ(cx, cy, 4, k == 3 ? C_YELLOW : C_AMBER);
        }
        for (int s = 0; s < BZ_SHIPS; s++) {
            int v = u - s * 24;
            if (v < 0) continue;
            int x = 160 + (s - 1) * imin(v, 60), y = 150 - v * 6 / 5;
            if (y < -20) continue;
            draw_ship_at(s, x, y, t);
            gfx_vline(x, y + 9, y + 9 + imin(v, 14), C_YELLOW);
        }
    }
    static const char *const LINES[3] = {
        "FAR OUT AMONG THE STARS, THE NEBULA GARDEN BLOOMED.",
        "THEN THE BLIGHT SEEPED IN, AND THE FLOWERS WILTED.",
        "SO THE HIVE WING FLEW OUT TO BURN IT BACK.",
    };
    int st = scene == 0 ? t : scene == 1 ? t - INTRO_S1 : t - INTRO_S2;
    if (st > 20) {
        gfx_rect(0, 118, BZZ_W, 13, C_INK);
        text_center(LINES[scene], 160, 121, C_LIGHT);
    }
    if ((t / 20) % 2) text_center(GLYPH_A " SKIP", 290, 168, C_SLATE);
}

static void draw_title(void) {
    draw_sky(0, bz.scroll);
    int t = bz.frame_t;
    ui_fancy_center("BUZZBOLT", 160, 26, 3, LOGO, 6, C_INK, C_MAROON);
    text_center("THE HIVE WING FLIES OUT AGAINST THE BLIGHT", 160, 56, C_LIGHT);
    for (int s = 0; s < BZ_SHIPS; s++) {
        int x = 100 + s * 60, y = 94 + (int)(sinf(t * 0.05f + s * 2.1f) * 4);
        const Sprite *sp = &bzz_spr[SP_LACEWING + s];
        spr_draw_scaled(sp, x - sp->w, y - sp->h, 2, 0);
        gfx_vline(x - 1, y + 13, y + 16 + (t / 2) % 3, C_YELLOW);
    }
    for (int k = 0; k < 3; k++) {
        int lx = 40 + k * 100 + (int)(sinf(t * 0.03f + k) * 10), ly = (t + k * 60) % 200 - 10;
        draw_letter_box(lx, ly, k == 1 ? LT_B : LT_Z);
    }
    if ((bz.state_t / 20) % 2) { gfx_rect(126, 128, 68, 13, C_INK); text_center("PRESS " GLYPH_A, 160, 131, C_WHITE); }
    char buf[48];
    snprintf(buf, sizeof buf, "HIGH SCORE %s", num(bzs.hs[0].score));
    tiny_center(buf, 160, 4, C_YELLOW);
    gfx_rect(0, 160, BZZ_W, 20, C_INK);
    ui_hint(6, 166, GLYPH_B, "LIBRARY", C_GREY);
    tiny_draw("BEAMDOWN SOFTWORKS 1988", BZZ_W - 6 - tiny_width("BEAMDOWN SOFTWORKS 1988"), 168, C_SLATE);
}

static void draw_scores(void) {
    draw_sky(0, bz.scroll);
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER};
    ui_fancy_center("HIGH SCORES", 160, 8, 2, grad, 3, C_INK, C_MAROON);
    ui_panel(40, 30, 240, 90, C_INK, C_DUSK);
    char buf[64];
    for (int i = 0; i < BZZ_HISCORES; i++) {
        HiScore *h = &bzs.hs[i];
        int y = 36 + i * 10;
        bool mine = bz.scores_page == 1 && i == bz.name_rank && bz.name_rank < BZZ_HISCORES;
        int col = mine ? C_YELLOW : i == 0 ? C_WHITE : C_LIGHT;
        snprintf(buf, sizeof buf, "%d", i + 1);
        text_draw(buf, 50, y, C_GREY);
        text_draw(h->name, 66, y, col);
        const char *s = num(h->score);
        text_draw(s, 170 - text_width(s), y, col);
        spr_draw(&bzz_spr[SP_OPT_LACE + iclamp(h->ship, 0, 2)], 184, y, 0);
        snprintf(buf, sizeof buf, h->won ? "ALL CLEAR" : "WAVE %d", h->wave);
        tiny_draw(buf, 198, y + 1, h->won ? C_YELLOW : C_GREY);
    }
    ui_panel(40, 124, 240, 34, C_INK, C_DUSK);
    snprintf(buf, sizeof buf, "HIGHEST MULTIPLIER X%d", bzs.best_mult);
    tiny_center(buf, 160, 128, C_YELLOW);
    for (int s = 0; s < BZ_SHIPS; s++) {
        snprintf(buf, sizeof buf, "%s BEST %s", BZZ_SHIP_NAME[s], num(bzs.best[s]));
        tiny_center(buf, 160, 136 + s * 7, C_LIGHT);
    }
    if ((bz.state_t / 20) % 2) text_center(GLYPH_A " OK", 160, 164, C_WHITE);
}

static const char *const SHIP_ROLE[BZ_SHIPS] = {"NIMBLE", "STURDY", "FIERCE"};
static const char *const SHIP_LINES[BZ_SHIPS][3] = {
    {"TAP: WEAVING PAIRS", "HOLD: TWIN BOLTS", "REST: A LANCE CHARGES"},
    {"TAP: A WIDE FAN", "HOLD: A CRESCENT", ""},
    {"TAP: THREE STREAMS", "HOLD: A LASER", "B: LAUNCH A BOMB"},
};

static void draw_select(void) {
    draw_sky(1, bz.scroll);
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER};
    ui_fancy_center("CHOOSE YOUR SHIP", 160, 5, 2, grad, 3, C_INK, C_MAROON);
    for (int s = 0; s < BZ_SHIPS; s++) {
        int x = 6 + s * 104, y = 26;
        bool sel = s == bz.sel;
        ui_panel(x, y, 100, 130, C_INK, sel ? C_YELLOW : C_DUSK);
        const Sprite *sp = &bzz_spr[SP_LACEWING + s];
        int by = sel ? (int)(sinf(bz.frame_t * 0.1f) * 2) : 0;
        spr_draw_scaled(sp, x + 50 - sp->w, y + 8 + by, 2, 0);
        text_center(BZZ_SHIP_NAME[s], x + 50, y + 40, sel ? C_WHITE : C_GREY);
        tiny_center(SHIP_ROLE[s], x + 50, y + 50, sel ? C_AMBER : C_SLATE);
        for (int k = 0; k < 3; k++) tiny_center(SHIP_LINES[s][k], x + 50, y + 70 + k * 12, sel ? C_LIGHT : C_SLATE);
        char buf[32];
        snprintf(buf, sizeof buf, "BEST %s", num(bzs.best[s]));
        tiny_center(buf, x + 50, y + 120, sel ? C_YELLOW : C_DUSK);
    }
    gfx_rect(0, 162, BZZ_W, 18, C_INK);
    int fx = ui_hint(6, 167, GLYPH_LEFT GLYPH_RIGHT, "CHOOSE", C_LIGHT);
    fx = ui_hint(fx, 167, GLYPH_A, "LAUNCH", C_LIGHT);
    ui_hint(fx, 167, GLYPH_B, "BACK", C_LIGHT);
}

static void draw_clear(void) {
    draw_play();
    ui_panel(70, 52, 180, 70, C_INK, C_YELLOW);
    char buf[64];
    snprintf(buf, sizeof buf, "WAVE %d CLEAR!", bz.wave + 1);
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER};
    ui_fancy_center(buf, 160, 58, 1, grad, 3, C_INK, C_MAROON);
    snprintf(buf, sizeof buf, "TIME BONUS %s", num((uint32_t)bz.bonus_shown));
    text_center(buf, 160, 76, C_LIGHT);
    tiny_center("(NOT MULTIPLIED)", 160, 86, C_SLATE);
    snprintf(buf, sizeof buf, "SCORE %s", num(bz.score));
    text_center(buf, 160, 96, C_WHITE);
    if (bz.wave + 1 < BZZ_WAVES) snprintf(buf, sizeof buf, "MULTIPLIER X%d, THE NEXT WAVE STARTS AT X1", bz.mult);
    else snprintf(buf, sizeof buf, "MULTIPLIER X%d", bz.mult);
    tiny_center(buf, 160, 110, C_YELLOW);
}

static void draw_over(void) {
    draw_play();
    gfx_darken_rect(0, BZZ_HUD_H + 1, BZZ_W, BZZ_H - BZZ_HUD_H - 1, 2);
    static const uint8_t grad[] = {C_PINK, C_RED, C_MAROON};
    ui_fancy_center("GAME OVER", 160, 70, 2, grad, 3, C_INK, C_NIGHT);
    char buf[48];
    snprintf(buf, sizeof buf, "SCORE %s", num(bz.score));
    text_center(buf, 160, 96, C_WHITE);
}

static void draw_ending(void) {
    int t = bz.state_t;
    draw_sky(0, (float)t * 0.5f);
    /* the Blight's heart is gone: the garden flowers where it was */
    for (int k = 0; k < 14; k++) {
        int x = 20 + k * 22, grow = iclamp((t - k * 12) / 6, 0, 18);
        if (grow <= 0) continue;
        gfx_vline(x, 150 - grow, 150, C_JADE);
        int c = k % 3 == 0 ? C_YELLOW : k % 3 == 1 ? C_PINK : C_ICE;
        if (grow >= 18) { gfx_circ(x, 150 - grow, 3, c); gfx_pset(x, 150 - grow, C_AMBER); }
    }
    gfx_rect(0, 150, BZZ_W, 30, C_FOREST);
    for (int s = 0; s < BZ_SHIPS; s++) {
        int x = 100 + s * 60, y = 132 - imin(t, 280) / 8 + (int)(sinf(t * 0.05f + s) * 3);
        draw_ship_at(s, x, y, t);
    }
    if (t > 40) text_center("THE SPOREHEART IS BURNT OUT.", 160, 20, C_WHITE);
    if (t > 130) text_center("THE NEBULA GARDEN BREATHES AGAIN,", 160, 34, C_LIGHT);
    if (t > 200) text_center("AND THE HIVE WING FLIES HOME.", 160, 46, C_LIGHT);
    char buf[48];
    if (t > 300) {
        snprintf(buf, sizeof buf, "FINAL SCORE %s", num(bz.score));
        text_center(buf, 160, 64, C_YELLOW);
    }
    if (t > 360 && bz.score >= 300000) text_center("A GOLDEN HARVEST!", 160, 76, (t / 10) % 2 ? C_YELLOW : C_WHITE);
    if (t > 240 && (t / 20) % 2) tiny_center("PRESS " GLYPH_A, 160, 170, C_GREY);
}

#define CREDIT_LINES 30
static const char *const CREDITS[CREDIT_LINES] = {
    "BUZZBOLT",
    "BEAMDOWN SOFTWORKS 1988",
    "",
    "THE HIVE WING",
    "LACEWING  SHIELDBUG  FIREFLY",
    "THE HIVESHIP AND THE DRAGONFLY",
    "",
    "THE BLIGHT",
    "GNATS  MIDGES  WHIRLERS",
    "CRICKETS  BLISTERS  GOLDBUGS",
    "PUFFBALLS AND THE ROT WALLS",
    "",
    "THE BIG ONES",
    "THE IRONBACKS",
    "THE BLOATFLY",
    "THE QUEEN TICK",
    "SCYTHEWING AND DUSTWING",
    "THE SPOREHEART",
    "",
    "GAME, PICTURES AND TUNES",
    "THE BEAMDOWN HIVE",
    "",
    "LETTERS BY THE BEAMDOWN",
    "SPELLING SOCIETY",
    "",
    "NO BEES WERE HARMED",
    "IN THE MAKING OF THIS GAME",
    "",
    "",
    "THANKS FOR PLAYING!",
};

static void draw_credits(void) {
    draw_sky(0, bz.scroll);
    int y0 = 184 - bz.state_t / 3;
    for (int i = 0; i < CREDIT_LINES; i++) {
        int y = y0 + i * 14;
        if (i == CREDIT_LINES - 1) y = imax(y, 84);
        if (y < -10 || y > 190) continue;
        int col = i == 0 ? C_YELLOW : i == CREDIT_LINES - 1 ? C_PINK : (CREDITS[i][0] && i > 0 && CREDITS[i - 1][0] == 0) ? C_AMBER : C_LIGHT;
        text_center(CREDITS[i], 160, y, col);
    }
}

static void draw_name(void) {
    draw_sky(0, bz.scroll);
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER};
    ui_fancy_center("NEW HIGH SCORE!", 160, 24, 2, grad, 3, C_INK, C_MAROON);
    char buf[48];
    snprintf(buf, sizeof buf, "RANK %d   %s", bz.name_rank + 1, num(bz.score));
    text_center(buf, 160, 52, C_WHITE);
    for (int k = 0; k < 3; k++) {
        int x = 124 + k * 26;
        bool on = k == bz.name_pos;
        ui_panel(x, 72, 20, 24, C_INK, on ? C_YELLOW : C_DUSK);
        char c[2] = {bz.name[k], 0};
        text_draw_scaled(c, x + 5, 77, on ? C_WHITE : C_LIGHT, 2);
        if (on && (bz.state_t / 10) % 2) { gfx_hline(x + 4, x + 15, 70, C_YELLOW); gfx_hline(x + 4, x + 15, 97, C_YELLOW); }
    }
    text_center(GLYPH_UP GLYPH_DOWN " LETTER   " GLYPH_LEFT GLYPH_RIGHT " MOVE", 160, 110, C_GREY);
    text_center(GLYPH_A " NEXT   START DONE", 160, 122, C_GREY);
}

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    int x = 2, y = 2, rowh = 0;
    for (int i = 0; i < SP_COUNT; i++) {
        Sprite *s = &bzz_spr[i];
        if (!s->px) continue;
        if (x + s->w * 2 > 318) { x = 2; y += rowh + 2; rowh = 0; }
        spr_draw_scaled(s, x, y, 2, 0);
        x += s->w * 2 + 3;
        if (s->h * 2 > rowh) rowh = s->h * 2;
    }
}

void bzz_draw(void) {
    gfx_camera(0, 0);
    gfx_noclip();
    if (bz.sheet) { draw_sheet(); return; }
    switch (bz.state) {
    case BS_INTRO: draw_intro(); break;
    case BS_TITLE: draw_title(); break;
    case BS_SCORES: draw_scores(); break;
    case BS_SELECT: draw_select(); break;
    case BS_PLAY: draw_play(); break;
    case BS_CLEAR: draw_clear(); break;
    case BS_OVER: draw_over(); break;
    case BS_ENDING: draw_ending(); break;
    case BS_CREDITS: draw_credits(); break;
    case BS_NAME: draw_name(); break;
    default: draw_play(); break;
    }
}

/* the cartridge's label in the library */
void bzz_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_NAVY);
    for (int i = 0; i < 30; i++) {
        int sx = x + (i * 53) % w, sy = y + (i * 37 + t) % h;
        gfx_pset(sx, sy, i % 3 ? C_SKY : C_AMBER);
    }
    for (int k = 0; k < 3; k++) {
        int gx = x + 24 + k * 44 + (int)(sinf(t * 0.05f + k) * 6), gy = y + 10 + k % 2 * 6;
        spr_draw(&bzz_spr[(t / 4 + k) % 2 ? SP_GNAT1 : SP_GNAT2], gx, gy, 0);
    }
    int px = x + w / 2 + (int)(sinf(t * 0.03f) * 30), py = y + h - 22;
    for (int k = 0; k < 3; k++) {
        int by = py - 8 - ((t * 4 + k * 14) % 36);
        gfx_rect(px - 1, by, 2, 5, C_ICE);
    }
    draw_letter_box(x + 20, y + 28 + (t / 3) % 12, LT_B);
    draw_letter_box(x + w - 30, y + 24 + (t / 4) % 14, LT_Z);
    spr_draw(&bzz_spr[SP_LACEWING], px - 7, py - 6, 0);
    gfx_noclip();
}
