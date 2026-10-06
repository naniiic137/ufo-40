/* TUSKWIND - drawing: the sky that sets as the dream goes on, the sea,
 * the islets and everything on them, Burl, the bar across the top, the
 * stalls, the signs, and every screen around the journey. */
#include "tuskwind.h"

#define PI_F 3.14159265f

static bool on_ground_or_ride(const TkwBody *b) { return b->mode == BM_GROUND || b->mode == BM_RIDE; }

static const uint8_t LOGO[] = {C_CREAM, C_ICE, C_CYAN, C_SKY, C_BLUE, C_VIOLET};

/* ---- the sky --------------------------------------------------------------- */

/* four times of day, top band to horizon */
static const uint8_t SKY[4][6] = {
    {C_BLUE, C_SKY, C_SKY, C_CYAN, C_CYAN, C_ICE},            /* afternoon */
    {C_SKY, C_CYAN, C_ICE, C_CREAM, C_YELLOW, C_AMBER},       /* golden */
    {C_PURPLE, C_VIOLET, C_MAGENTA, C_PINK, C_ORANGE, C_AMBER}, /* sunset */
    {C_INK, C_NIGHT, C_NIGHT, C_DUSK, C_PURPLE, C_VIOLET},    /* night */
};

static int sky_phase(void) {
    if (tkg.in_hall) return 3;
    int p = tkw_progress_at(tkg.cam_x + SCREEN_W / 2.0f);
    return p < 35 ? 0 : p < 60 ? 1 : p < 82 ? 2 : 3;
}

static void draw_sky_bands(int phase, int t, int sun_y) {
    for (int i = 0; i < 6; i++) gfx_rect(0, i * 30, SCREEN_W, 30, SKY[phase][i]);
    for (int i = 1; i < 6; i++) gfx_dither(0, i * 30 - 6, SCREEN_W, 6, SKY[phase][i - 1], 6);
    if (phase == 3) {
        for (int k = 0; k < 40; k++) {
            int sx = (k * 97 + 13) % SCREEN_W, sy = (k * 53 + 7) % 110;
            gfx_pset(sx, sy, (k + t / 20) % 5 ? C_LIGHT : C_WHITE);
        }
        gfx_circ(250, 34, 9, C_CREAM);
        gfx_circ(254, 31, 8, SKY[phase][1]);
    } else {
        int col = phase == 0 ? C_CREAM : phase == 1 ? C_YELLOW : C_ORANGE;
        gfx_dither_circle(250, sun_y, 20, col, 5);
        gfx_circ(250, sun_y, 13, col);
        gfx_circ(250, sun_y, 10, phase == 2 ? C_AMBER : C_WHITE);
    }
}

static void draw_far_clouds(int phase, float camx, int t) {
    int col = phase == 3 ? C_DUSK : phase == 2 ? C_PINK : C_WHITE;
    for (int k = 0; k < 7; k++) {
        int x = (int)(((k * 173 + 40) - camx * 0.15f + t * 0.05f)) % (SCREEN_W + 80);
        if (x < -80) x += SCREEN_W + 80;
        x -= 40;
        int y = 30 + (k * 37) % 70;
        gfx_dither(x, y, 50 + k % 3 * 12, 6, col, 6);
        gfx_dither(x + 8, y - 4, 30, 5, col, 4);
    }
}

/* ---- the world ------------------------------------------------------------- */

static void draw_sea(int t) {
    int sea = tkw_w.sea_y;
    int phase = sky_phase();
    int c1 = phase == 3 ? C_NAVY : C_BLUE, c2 = phase == 3 ? C_NIGHT : C_NAVY;
    int x0 = (int)tkg.cam_x - 8, x1 = (int)tkg.cam_x + SCREEN_W + 8;
    gfx_rect(x0, sea, x1 - x0, tkw_w.h - sea + 40, c1);
    gfx_dither(x0, sea + 8, x1 - x0, tkw_w.h - sea + 40, c2, 8);
    for (int x = x0 - (x0 % 16); x < x1; x += 16) {
        int o = (int)(sinf((x + t * 1.5f) * 0.05f) * 2);
        gfx_hline(x, x + 9, sea - 1 + o, C_CYAN);
        gfx_hline(x + 3, x + 6, sea - 2 + o, C_ICE);
    }
}

static void draw_plat(int i, int t) {
    const TkwPlat *p = &tkw_w.plat[i];
    if (p->gone) {
        if (p->kind == PK_FOAM && p->back > 0 && p->back < 40) gfx_dither(p->x, p->y, p->w, p->h, C_PINK, (40 - p->back) / 5);
        return;
    }
    bool flash = p->brawl_t > 0 && (p->brawl_t / 6) % 2;
    switch (p->kind) {
    case PK_LEDGE: {
        /* an islet: ice on top, a craggy underside tapering away */
        int depth = imin(16, 6 + p->w / 6);
        for (int r = 0; r < depth; r++) {
            int in = r * p->w / (depth * 3);
            gfx_hline(p->x + in, p->x + p->w - 1 - in, p->y + 3 + r, r < 3 ? C_SLATE : C_DUSK);
        }
        gfx_rect(p->x, p->y, p->w, 3, flash ? C_WHITE : C_ICE);
        gfx_hline(p->x, p->x + p->w - 1, p->y + 3, C_CYAN);
        for (int k = 3; k < p->w - 3; k += 7) gfx_pset(p->x + k, p->y + 1, C_WHITE);
        break;
    }
    case PK_ROCK:
        gfx_rect(p->x, p->y, p->w, p->h, flash ? C_LIGHT : C_GREY);
        gfx_rectb(p->x, p->y, p->w, p->h, C_INK);
        gfx_hline(p->x + 1, p->x + p->w - 2, p->y + 1, C_LIGHT);
        gfx_dither(p->x + 1, p->y + p->h / 2, p->w - 2, p->h / 2 - 1, C_SLATE, 8);
        for (int k = 5; k < p->w - 3; k += 9) gfx_vline(p->x + k, p->y + 3, p->y + p->h - 3, C_SLATE);
        break;
    case PK_FOAM: {
        /* a pink puff of foam that thins out once stood on */
        int lv = p->timer < 0 ? 16 : imax(4, 16 - p->timer * 12 / TKW_FOAM_T);
        gfx_dither(p->x, p->y + 1, p->w, p->h - 1, C_PINK, lv);
        gfx_dither(p->x + 3, p->y - 2, p->w - 6, 3, C_PINK, lv);
        gfx_dither(p->x + 2, p->y + p->h - 2, p->w - 4, 2, C_MAGENTA, lv);
        if (p->timer >= 0 && (t / 3) % 2) gfx_pset(p->x + (t * 7) % imax(1, p->w), p->y + 2, C_WHITE);
        break;
    }
    }
}

static void draw_lighthouse(const TkwThing *th, int t) {
    int x = (int)th->x, y = (int)th->y;
    /* the tower */
    for (int r = 0; r < 26; r++) {
        int hw = 4 + r / 6;
        gfx_hline(x - hw, x + hw, y - 26 + r, (r / 5) % 2 ? C_RED : C_WHITE);
    }
    gfx_vline(x - 8, y - 4, y - 1, C_INK);
    gfx_rect(x - 2, y - 8, 4, 7, C_BROWN);
    spr_draw(&tkw_spr[SP_LIGHTHOUSE], x - 6, y - 33, 0);
    if (th->state == 0) {
        /* lit, with its tern on the gallery */
        gfx_dither_circle(x, y - 29, 9 + (t / 8) % 2, C_YELLOW, 4);
        spr_draw(&tkw_spr[(t / 20) % 2 ? SP_TERN1 : SP_TERN2], x + 4, y - 40, 0);
    } else {
        gfx_rect(x - 4, y - 30, 8, 3, C_SLATE);
    }
}

static void draw_shop(const TkwThing *th, int t) {
    int x = (int)th->x, y = (int)th->y;
    gfx_rect(x - 12, y - 10, 24, 10, C_TAN);
    gfx_rectb(x - 12, y - 10, 24, 10, C_INK);
    for (int k = 0; k < 6; k++) gfx_rect(x - 14 + k * 5, y - 26, 5, 5, k % 2 ? C_WHITE : C_TEAL);
    gfx_vline(x - 12, y - 21, y - 10, C_BROWN);
    gfx_vline(x + 11, y - 21, y - 10, C_BROWN);
    spr_draw(&tkw_spr[SP_DUCHESS], x - 6, y - 25 + ((t / 30) % 2), 0);
    gfx_rect(x - 12, y - 10, 24, 3, C_EARTH);
}

static void draw_sign(const TkwThing *th) {
    int x = (int)th->x, y = (int)th->y;
    gfx_vline(x, y - 12, y - 1, C_BROWN);
    gfx_rect(x - 6, y - 17, 13, 8, C_EARTH);
    gfx_rectb(x - 6, y - 17, 13, 8, C_BROWN);
    gfx_hline(x - 4, x + 3, y - 15, C_TAN);
    gfx_hline(x - 4, x + 4, y - 13, C_TAN);
    if (tkg.signs_run & (1 << th->arg)) gfx_pset(x + 4, y - 15, C_CREAM);
}

static void draw_door(int t) {
    const TkwPlat *p = &tkw_w.plat[tkw_w.goal_plat];
    int x = p->x + p->w - 24, y = p->y;
    /* a stone arch with steps going down into it */
    gfx_rect(x - 12, y - 30, 24, 30, C_GREY);
    gfx_rectb(x - 12, y - 30, 24, 30, C_INK);
    gfx_circ(x, y - 30, 12, C_GREY);
    gfx_circb(x, y - 30, 12, C_INK);
    gfx_rect(x - 7, y - 26, 14, 26, C_INK);
    gfx_circ(x, y - 26, 7, C_INK);
    for (int k = 0; k < 4; k++) gfx_hline(x - 6 + k, x + 6 - k, y - 4 - k * 5, C_DUSK);
    gfx_dither_circle(x, y - 22, 6 + (t / 12) % 2, C_VIOLET, 3);
}

static void draw_thing(const TkwThing *th, int t) {
    int x = (int)th->x, y = (int)th->y;
    int bob = (int)(sinf(t * 0.08f + th->x * 0.1f) * 1.5f);
    switch (th->kind) {
    case TH_COCKLE: spr_draw(&tkw_spr[SP_COCKLE], x - 3, y - 3 + bob, 0); break;
    case TH_WHELK: spr_draw(&tkw_spr[SP_WHELK], x - 4, y - 4 + bob, 0); break;
    case TH_KEY: spr_draw(&tkw_spr[SP_KEY], x - 4, y - 2 + bob, 0); break;
    case TH_SPRAT: spr_draw(&tkw_spr[SP_SPRAT], x - 5, y - 2 + bob, (t / 30) % 2 ? SPR_FLIPX : 0); break;
    case TH_SPIRAL: spr_draw(&tkw_spr[SP_SPIRAL], x - 3, y - 3, 0); break;
    case TH_CHEST: spr_draw(&tkw_spr[th->state ? SP_CHEST_OPEN : SP_CHEST], x - 7, y - 10, 0); break;
    case TH_LIGHTHOUSE: draw_lighthouse(th, t); break;
    case TH_SHOP: draw_shop(th, t); break;
    case TH_SIGN: draw_sign(th); break;
    case TH_BELL:
        gfx_vline(x - 6, y - 18, y - 1, C_BROWN);
        gfx_hline(x - 6, x, y - 18, C_BROWN);
        spr_draw(&tkw_spr[th->state ? SP_BELL_DONE : SP_BELL], x - 4, y - 17 + (th->state ? 0 : (t / 10) % 2), 0);
        break;
    case TH_RAM: {
        int fl = th->dir < 0 ? SPR_FLIPX : 0;
        bool run = th->state == RAM_CHARGE && (t / 4) % 2;
        spr_draw(&tkw_spr[run ? SP_RAM_RUN : SP_RAM], x - 9, y - 12 + (th->state == RAM_FALL ? 0 : 0), fl);
        if (th->state == RAM_ALERT) text_draw("!", x - 1, y - 22 + (t / 4) % 2, C_RED);
        break;
    }
    case TH_PERCH:
        gfx_vline(x, y - 8, y - 1, C_BROWN);
        if (th->state == 0) spr_draw(&tkw_spr[(t / 25 + x) % 2 ? SP_TERN1 : SP_TERN2], x - 4, y - 14, 0);
        break;
    case TH_ELDER:
        gfx_rect(x - 18, y - 4, 36, 4, C_SLATE);
        gfx_dither_circle(x, y - 12, 22 + (t / 10) % 2, C_CYAN, 2);
        spr_draw(&tkw_spr[SP_ELDER], x - 14, y - 24, 0);
        break;
    case TH_SECRET:
        gfx_vline(x, y - 4, y + 6, C_BROWN);
        gfx_rect(x - 6, y - 9, 13, 7, C_EARTH);
        gfx_rectb(x - 6, y - 9, 13, 7, C_BROWN);
        break;
    default: break;
    }
}

static void draw_manta(const TkwManta *m, int t) {
    if (!m->alive) return;
    int fl = m->x < m->px ? SPR_FLIPX : 0;
    spr_draw(&tkw_spr[(t / 12) % 2 ? SP_MANTA1 : SP_MANTA2], (int)m->x - 12, (int)m->y - 4, fl);
}

/* Burl (or a brawler, recoloured) */
static void draw_walrus(const TkwBody *b, int face, bool charging, bool flapping, bool hurt, int anim, const uint8_t *remap) {
    int id = SP_BURL;
    if (charging) id = SP_BURL_CROUCH;
    else if (b->mode == BM_AIR) id = hurt ? SP_BURL_FALL : flapping ? ((anim / 4) % 2 ? SP_BURL_FLAP1 : SP_BURL_FLAP2) : SP_BURL;
    else if (b->mode == BM_LUNGE) id = SP_BURL_CROUCH;
    else if ((anim / 10) % 2 && b->vx == 0 && (anim % 40) < 20) id = SP_BURL;
    spr_draw_ex(&tkw_spr[id], (int)b->x - 10, (int)b->y - 14, face < 0 ? SPR_FLIPX : 0, remap, -1);
}

static void draw_aim(float x, float y, int face, float aim, int charge, int red, int t) {
    float a = aim * PI_F / 180.0f;
    int cx = (int)(x + cosf(a) * face * 26), cy = (int)(y - 7 - sinf(a) * 26);
    int col = (t / 6) % 2 ? C_YELLOW : C_WHITE;
    gfx_hline(cx - 3, cx + 3, cy, C_INK);
    gfx_vline(cx, cy - 3, cy + 3, C_INK);
    gfx_hline(cx - 2, cx + 2, cy, col);
    gfx_vline(cx, cy - 2, cy + 2, col);
    if (charge >= 0) {
        /* the meter under him */
        int mx = (int)x - 11, my = (int)y + 2;
        gfx_rect(mx, my, 22, 4, C_INK);
        int f = charge * 20 / TKW_CHARGE_T;
        gfx_rect(mx + 1, my + 1, f, 2, charge >= TKW_CHARGE_T ? C_WHITE : C_ORANGE);
        if (red >= 0) gfx_vline(mx + 1 + red * 20 / TKW_CHARGE_T, my - 1, my + 4, C_RED);
    }
}

static void draw_hero(int t) {
    const TkwHero *h = &tkg.h;
    const TkwBody *b = &h->b;
    if (h->ps == PS_RESCUE) {
        /* a tern lifts him out of the sea and back */
        const TkwPlat *p = &tkw_w.plat[h->takeoff];
        float k = h->ps_t / 80.0f;
        float x = b->x + (h->takeoff_x - b->x) * k, y = tkw_w.sea_y + (p->y - tkw_w.sea_y) * k;
        TkwBody tmp = *b;
        tmp.x = x;
        tmp.y = y;
        tmp.mode = BM_GROUND;
        draw_walrus(&tmp, h->face, false, false, false, 0, NULL);
        spr_draw(&tkw_spr[(t / 4) % 2 ? SP_TERN1 : SP_TERN2], (int)x - 4, (int)y - 22, 0);
        gfx_line((int)x, (int)y - 16, (int)x, (int)y - 13, C_INK);
        return;
    }
    if (h->ps == PS_SINK) {
        TkwBody tmp = *b;
        tmp.y = tkw_w.sea_y + h->ps_t * 0.2f;
        gfx_clip(0, 0, SCREEN_W, (int)(tkw_w.sea_y - tkg.cam_y));
        draw_walrus(&tmp, h->face, false, false, true, 0, NULL);
        gfx_noclip();
        return;
    }
    if (h->ps == PS_DOOR) {
        if (h->ps_t < 25) draw_walrus(b, h->face, false, false, false, h->anim, NULL);
        return;
    }
    if (b->kite) {
        int kx = (int)b->x - 6, ky = (int)b->y - 40;
        gfx_line((int)b->x, (int)b->y - 12, kx + 6, ky + 12, C_LIGHT);
        spr_draw(&tkw_spr[SP_KITE_BIG], kx, ky, 0);
    }
    bool blink = h->hurt_t > 0 && (t / 2) % 2;
    if (!blink) draw_walrus(b, h->face, h->charging, h->flapping, h->hurt_t > 0, h->anim, NULL);
    if (h->spinner) {
        int f = (t / 2) % 2;
        spr_draw(&tkw_spr[SP_SPINNER_BIG], (int)b->x - 7, (int)b->y - 19, f ? SPR_FLIPX : 0);
        /* its bar */
        gfx_rect((int)b->x - 8, (int)b->y - 23, 16, 2, C_INK);
        gfx_rect((int)b->x - 8, (int)b->y - 23, h->spin_fuel * 16 / TKW_SPIN_FUEL, 2, C_SKY);
    }
    bool aiming = (h->ps == PS_PLAY || h->ps == PS_THROW) && (b->mode == BM_GROUND || b->mode == BM_RIDE);
    if (aiming) draw_aim(b->x, b->y, h->face, h->aim, h->charging || h->ps == PS_THROW || h->red_line >= 0 ? h->charge : -1,
                         h->red_line, t);
    if (h->ps == PS_THROW) spr_draw(&tkw_spr[SP_BALL], (int)b->x + h->face * 6 - 3, (int)b->y - 20, 0);
    if (h->ps == PS_BALL) spr_draw(&tkw_spr[SP_BALL], (int)h->ball.x - 3, (int)h->ball.y - 6, 0);
    if (h->ps == PS_ROPE || h->ps == PS_HAUL) {
        gfx_line((int)b->x, (int)b->y - 6, (int)h->rope_x, (int)h->rope_y, C_HIDE);
        gfx_rect((int)h->rope_x - 1, (int)h->rope_y - 1, 3, 3, C_LIGHT);
    }
}

static void draw_rain(int t) {
    if (tkg.in_hall || !tkg.wind80) return;
    for (int k = 0; k < 60; k++) {
        int x = (k * 53 + t * (3 + tkg.wind * 2)) % SCREEN_W, y = (k * 31 + t * 5) % SCREEN_H;
        gfx_line(x, y, x + tkg.wind * 2, y + 4, C_LIGHT);
    }
}

static void draw_hall_back(int t) {
    /* the deep hall: an ice vault under the sea, lit from below */
    static const uint8_t BANDS[6] = {C_INK, C_INK, C_NIGHT, C_NIGHT, C_NAVY, C_NAVY};
    for (int i = 0; i < 6; i++) gfx_rect(0, i * 30, SCREEN_W, 30, BANDS[i]);
    for (int i = 1; i < 6; i++) gfx_dither(0, i * 30 - 6, SCREEN_W, 6, BANDS[i - 1], 6);
    int off = (int)(tkg.cam_x * 0.5f);
    for (int k = -1; k < 7; k++) {
        int x = k * 60 - off % 60 + 20;
        gfx_dither(x, 0, 16, SCREEN_H, C_DUSK, 6);
        gfx_vline(x + 2, 0, SCREEN_H, C_NIGHT);
        /* a rune glowing on each pillar */
        int gy = 70 + (k * 37 + off / 60 * 13) % 40;
        gfx_rect(x + 6, gy, 4, 4, (t / 30 + k) % 3 ? C_TEAL : C_CYAN);
    }
}

static void draw_world(int t) {
    int phase = sky_phase();
    if (tkg.in_hall) draw_hall_back(t);
    else {
        int p = tkw_progress_at(tkg.cam_x + SCREEN_W / 2.0f);
        draw_sky_bands(phase, t, 40 + p * 110 / 100);
        draw_far_clouds(phase, tkg.cam_x, t);
        /* the shape on the horizon, nearer as the dream goes on */
        if (p > 55) {
            int hx = 40 + (100 - p);
            gfx_rect(hx, 158, 22, 3, C_INK);
            gfx_rect(hx + 6, 150, 2, 8, C_INK);
            gfx_rect(hx + 13, 152, 2, 6, C_INK);
        }
    }
    int sx = tkg.shake ? (tkg.shake % 2 ? 2 : -2) : 0;
    gfx_camera((int)tkg.cam_x + sx, (int)tkg.cam_y);
    draw_sea(t);
    int x0 = (int)tkg.cam_x - 40, x1 = (int)tkg.cam_x + SCREEN_W + 40;
    for (int i = 0; i < tkw_w.nplat; i++) {
        const TkwPlat *p = &tkw_w.plat[i];
        if (p->x + p->w < x0 || p->x > x1) continue;
        draw_plat(i, t);
    }
    if (!tkg.in_hall && tkw_w.goal_plat >= 0) draw_door(t);
    if (tkg.in_hall) {
        /* icicles under the vault, and the chasm's dark */
        for (int x = TKW_HALL_EDGE + 4; x < TKW_HALL_W; x += 9) {
            if (x > TKW_HALL_SHAFT - 22 && x < TKW_HALL_SHAFT + 22) continue;
            int len = 4 + (x * 7) % 9;
            gfx_vline(x, 112, 112 + len, C_ICE);
            gfx_vline(x + 1, 112, 112 + len / 2, C_CYAN);
        }
        gfx_dither(0, 0, TKW_HALL_EDGE, tkw_w.h, C_INK, 10);
    }
    for (int i = 0; i < tkw_w.nth; i++) {
        const TkwThing *th = &tkw_w.th[i];
        if (!th->alive || th->x < x0 || th->x > x1) continue;
        if (th->kind == TH_CHEST && tkg.in_hall && fabsf(tkg.h.b.x - th->x) > 140) continue; /* hidden in the dark */
        draw_thing(th, t);
    }
    for (int k = 0; k < tkw_w.nmanta; k++) draw_manta(&tkw_w.manta[k], t);
    draw_hero(t);
    gfx_camera(0, 0);
    draw_rain(t);
}

/* ---- the bar across the top --------------------------------------------------- */

static void draw_hud(void) {
    char buf[32];
    gfx_rect(0, 0, SCREEN_W, TKW_HUD_H, C_INK);
    gfx_hline(0, SCREEN_W - 1, TKW_HUD_H - 1, C_DUSK);
    spr_draw(&tkw_spr[SP_COCKLE], 3, 3, 0);
    snprintf(buf, sizeof buf, "%d", tkg.shells);
    text_draw(buf, 12, 2, C_CREAM);
    spr_draw(&tkw_spr[SP_KEY], 34, 4, 0);
    snprintf(buf, sizeof buf, "%d", tkg.keys);
    text_draw(buf, 45, 2, C_YELLOW);
    /* the flipper bar */
    tiny_draw("FLAP", 58, 4, C_GREY);
    gfx_rect(76, 3, 52, 6, C_DUSK);
    gfx_rect(77, 4, tkg.h.b.stamina * 50 / TKW_STAMINA, 4, tkg.h.b.stamina < TKW_STAMINA / 4 ? C_ORANGE : C_YELLOW);
    /* the terns */
    int found = tkg.terns + tkg.terns_used;
    for (int k = 0; k < TKW_TERNS; k++) {
        int x = 134 + k * 11;
        if (tkg.in_hall) {
            gfx_rect(x + 2, 5, 5, 2, C_DUSK);
            continue;
        }
        if (k < tkg.terns) spr_draw(&tkw_spr[SP_TERN1], x, 3, 0);
        else if (k < found) {
            spr_draw_ex(&tkw_spr[SP_TERN1], x, 3, 0, NULL, C_SLATE);
            gfx_line(x + 1, 3, x + 7, 8, C_RED);
        } else {
            spr_draw_ex(&tkw_spr[SP_TERN1], x, 3, 0, NULL, C_DUSK);
        }
    }
    /* the wind */
    if (tkg.wind) {
        int x = 206;
        tiny_draw("WIND", x, 4, C_LIGHT);
        const char *arrow = tkg.wind > 0 ? GLYPH_RIGHT : GLYPH_LEFT;
        text_draw(arrow, x + 18, 2, tkg.wind_t ? C_CYAN : C_ORANGE);
    }
    /* how far along */
    snprintf(buf, sizeof buf, "%d%%", tkg.in_hall ? 100 : tkg.pct);
    text_draw(buf, SCREEN_W - 4 - text_width(buf), 2, C_WHITE);
}

static void text_box(int y, const char *s, int col, int border) {
    char l[3][UI_WRAP_LEN];
    int n = imin(ui_wrap(s, 280, false, l, 3), 3);
    int h = 8 + n * LINE_H;
    ui_panel(16, y, 288, h, C_NIGHT, border);
    for (int i = 0; i < n; i++) text_center(l[i], SCREEN_W / 2, y + 5 + i * LINE_H, col);
}

static void draw_menu(void) {
    const TkwHero *h = &tkg.h;
    int list[IT_COUNT], n = tkw_menu_items(list);
    int w = 16 + (n + 1) * 18, x = SCREEN_W / 2 - w / 2, y = SCREEN_H - 40;
    ui_panel(x, y, w, 34, C_NIGHT, C_SLATE);
    for (int i = 0; i <= n; i++) {
        int ix = x + 8 + i * 18, iy = y + 6;
        bool sel = i == h->menu_sel;
        if (sel) gfx_rect(ix - 2, iy - 2, 13, 13, C_DUSK);
        if (i == 0) spr_draw(&tkw_spr[SP_I_CROSS], ix, iy, 0);
        else {
            int it = list[i - 1];
            spr_draw_ex(&tkw_spr[SP_I_BOBBER + it], ix, iy, 0, NULL, tkw_can_use(it) ? -1 : C_SLATE);
            char c[4];
            snprintf(c, sizeof c, "%d", tkg.inv[it]);
            tiny_draw(c, ix + 7, iy + 8, C_LIGHT);
        }
        if (sel) gfx_rectb(ix - 2, iy - 2, 13, 13, C_YELLOW);
    }
    const char *name = h->menu_sel == 0 || h->menu_sel > n ? "PUT AWAY" : TKW_ITEM_NAME[list[h->menu_sel - 1]];
    tiny_center(name, SCREEN_W / 2, y + 24, C_YELLOW);
}

static void draw_shop_panel(void) {
    const TkwHero *h = &tkg.h;
    const TkwThing *t = &tkw_w.th[h->shop_th];
    int ware[2] = {t->arg & 15, t->arg >> 4};
    ui_panel(40, 34, 240, 112, C_NIGHT, C_TEAL);
    spr_draw(&tkw_spr[SP_DUCHESS], 50, 42, 0);
    text_draw("THE DUCHESS AUK'S STALL", 68, 42, C_WHITE);
    tiny_draw("ONE VISIT ONLY. CHOOSE WELL, WALRUS.", 68, 53, C_GREY);
    for (int i = 0; i < 3; i++) {
        int y = 68 + i * 16;
        bool sel = h->shop_sel == i;
        if (sel) gfx_rect(52, y - 3, 216, 14, C_DUSK);
        if (i < 2) {
            int it = ware[i];
            spr_draw(&tkw_spr[SP_I_BOBBER + it], 60, y - 1, 0);
            text_draw(TKW_ITEM_NAME[it], 76, y, sel ? C_YELLOW : C_LIGHT);
            char buf[24];
            snprintf(buf, sizeof buf, "%d " GLYPH_DOT " HAVE %d", TKW_ITEM_COST[it], tkg.inv[it]);
            text_draw(buf, 262 - text_width(buf), y, tkg.shells >= TKW_ITEM_COST[it] ? C_CREAM : C_SLATE);
        } else {
            text_draw("LEAVE", 76, y, sel ? C_YELLOW : C_LIGHT);
        }
    }
    char buf[24];
    snprintf(buf, sizeof buf, "SHELLS %d", tkg.shells);
    text_draw(buf, 60, 126, C_CREAM);
    ui_hint(186, 126, GLYPH_A, "BUY", C_GREY);
    ui_hint(226, 126, GLYPH_B, "LEAVE", C_GREY);
}

static void draw_look(int t) {
    /* the spyglass's round view */
    int cx = SCREEN_W / 2, cy = TKW_HUD_H + TKW_VIEW_H / 2;
    for (int y = TKW_HUD_H; y < SCREEN_H; y++) {
        int dy = y - cy;
        int r2 = 78 * 78 - dy * dy;
        int hw = r2 > 0 ? (int)sqrtf((float)r2) : 0;
        gfx_hline(0, cx - hw - 1, y, C_INK);
        gfx_hline(cx + hw, SCREEN_W - 1, y, C_INK);
    }
    gfx_circb(cx, cy, 78, C_TAN);
    tiny_draw("SPYGLASS", 6, SCREEN_H - 10, C_GREY);
    tiny_draw(GLYPH_A " DONE", SCREEN_W - 30, SCREEN_H - 10, (t / 20) % 2 ? C_GREY : C_LIGHT);
}

static void draw_journey(void) {
    int t = tkg.frame_t;
    draw_world(t);
    const TkwHero *h = &tkg.h;
    if (h->ps == PS_LOOK) draw_look(t);
    draw_hud();
    if (tkg.sign_near >= 0 && h->ps != PS_SHOP && h->ps != PS_LOOK)
        text_box(TKW_HUD_H + 4, tkg.sign_near < TKW_SIGNS ? TKW_SIGN_TEXT[tkg.sign_near] : TKW_SECRET_TEXT, C_CREAM, C_VIOLET);
    if (tkg.msg_t > 0 && h->ps != PS_SHOP) {
        int w = text_width(tkg.msg) + 12;
        gfx_rect(SCREEN_W / 2 - w / 2, SCREEN_H - 16, w, 12, C_INK);
        text_center(tkg.msg, SCREEN_W / 2, SCREEN_H - 14, C_YELLOW);
    }
    if (h->ps == PS_MENU) draw_menu();
    if (h->ps == PS_SHOP) draw_shop_panel();
    if (h->ps == PS_THROW) tiny_center("BOBBER: AIM, HOLD " GLYPH_A ", LET GO TO THROW", SCREEN_W / 2, SCREEN_H - 10, C_LIGHT);
    if (h->ps == PS_SINK) gfx_set_fade(iclamp(h->ps_t / 14, 0, 7));
    else if (h->ps == PS_DOOR) gfx_set_fade(iclamp((h->ps_t - 20) / 4, 0, 7));
    else gfx_set_fade(0);
}

/* ---- the screens around it ------------------------------------------------------ */

static void draw_floe_scene(int t, bool zz) {
    draw_sky_bands(3, t, 100);
    gfx_rect(0, 140, SCREEN_W, 40, C_NAVY);
    gfx_dither(0, 150, SCREEN_W, 30, C_NIGHT, 8);
    for (int x = 0; x < SCREEN_W; x += 16) gfx_hline(x, x + 8, 140 + (int)(sinf((x + t) * 0.05f) * 1.5f), C_SKY);
    /* the floe */
    gfx_rect(100, 132, 120, 10, C_ICE);
    gfx_hline(96, 223, 132, C_WHITE);
    gfx_dither(100, 138, 120, 4, C_CYAN, 8);
    TkwBody b;
    memset(&b, 0, sizeof b);
    b.x = 160;
    b.y = 132;
    b.mode = BM_GROUND;
    spr_draw(&tkw_spr[SP_BURL_SLEEP], 150, 118, 0);
    if (zz) {
        for (int k = 0; k < 3; k++) {
            int zt = (t + k * 40) % 120;
            text_draw("Z", 172 + zt / 6 + k * 2, 112 - zt / 3, zt < 90 ? C_LIGHT : C_SLATE);
        }
    }
}

static void draw_title(void) {
    int t = tkg.frame_t;
    draw_floe_scene(t, true);
    ui_fancy_center("TUSKWIND", 160, 18, 3, LOGO, 6, C_INK, C_NIGHT);
    text_center_shadow("A WALRUS DREAMS OF A LONG WAY OVER THE SEA", 160, 46, C_CREAM, C_INK);
    static const char *const ITEMS[2] = {"JOURNEY", "BRAWL: 2 PLAYERS"};
    for (int i = 0; i < 2; i++) {
        int y = 64 + i * 12;
        bool off = i == 1 && plat_kind() == PLAT_VITA;
        gfx_rect(100, y - 2, 120, 11, C_INK);
        text_center(ITEMS[i], 160, y, off ? C_SLATE : tkg.sel == i ? C_YELLOW : C_LIGHT);
        if (tkg.sel == i) ui_cursor(102, y, t);
    }
    gfx_rect(0, 0, SCREEN_W, 10, C_INK);
    char buf[64];
    int n = 0;
    for (int i = 0; i < TKW_SIGNS; i++) n += (tks.signs >> i) & 1;
    snprintf(buf, sizeof buf, "SIGNS READ %d/%d%s", n, TKW_SIGNS, (tks.signs >> TKW_SIGNS) & 1 ? " +1" : "");
    tiny_draw(buf, 4, 3, C_YELLOW);
    snprintf(buf, sizeof buf, "MOST TERNS %d  MOST CHESTS %d", tks.most_terns, tks.most_chests);
    tiny_draw(buf, SCREEN_W - 4 - tiny_width(buf), 3, C_LIGHT);
    gfx_rect(0, SCREEN_H - 12, SCREEN_W, 12, C_INK);
    ui_hint(4, SCREEN_H - 10, GLYPH_B, "LIBRARY", C_GREY);
    tiny_draw("BEAMDOWN SOFTWORKS 1986", SCREEN_W - 4 - tiny_width("BEAMDOWN SOFTWORKS 1986"), SCREEN_H - 8, C_SLATE);
}

static void draw_intro(void) {
    int t = tkg.frame_t;
    draw_floe_scene(t, tkg.talk_page >= 1);
    int n = 0;
    while (TKW_INTRO[n]) n++;
    if (tkg.talk_page < n) text_box(30, TKW_INTRO[tkg.talk_page], C_CREAM, C_SLATE);
    if (tkg.state_t > 20 && (t / 20) % 2) tiny_draw(GLYPH_A " ON", SCREEN_W - 26, SCREEN_H - 10, C_GREY);
    tiny_draw("START SKIPS", 4, SCREEN_H - 10, C_SLATE);
}

static void draw_talk(void) {
    int t = tkg.frame_t;
    draw_world(t);
    draw_hud();
    char buf[200];
    const char *s = tkw_talk_page(tkg.cherry_end, tkg.talk_page, buf, sizeof buf);
    text_box(TKW_HUD_H + 8, s, tkg.cherry_end ? C_ICE : C_CREAM, tkg.cherry_end ? C_CYAN : C_SLATE);
    if (tkg.talk_t > 20 && (t / 20) % 2) tiny_draw(GLYPH_A " ON", SCREEN_W - 26, SCREEN_H - 10, C_GREY);
}

static void draw_ending(void) {
    int t = tkg.state_t;
    draw_sky_bands(tkg.cherry_end ? 1 : 2, t, 120);
    int sea = 120;
    gfx_rect(0, sea, SCREEN_W, SCREEN_H - sea, C_BLUE);
    gfx_dither(0, sea + 20, SCREEN_W, SCREEN_H - sea, C_NAVY, 8);
    for (int x = 0; x < SCREEN_W; x += 16) gfx_hline(x, x + 8, sea + (int)(sinf((x + t) * 0.05f) * 1.5f), C_CYAN);
    /* the hollow tree: the hunters' ship */
    int sx = tkg.cherry_end ? 196 + imax(0, t - 300) / 2 : 260 - t / 8;
    int lift = tkg.cherry_end ? iclamp((t - 120) / 3, 0, 56) : 0;
    sea -= lift; /* the ship rides the wave up */
    gfx_rect(sx - 14, sea - 6, 28, 6, C_INK);
    gfx_rect(sx - 2, sea - 26, 2, 20, C_INK);
    gfx_rect(sx - 10, sea - 22, 18, 12, C_LIGHT);
    sea += lift;
    if (tkg.cherry_end) {
        /* the sea answers: a great wave rises under the ship and carries it off */
        int rise = iclamp((t - 120) / 3, 0, 56);
        int drift = imax(0, t - 300) / 2;
        int wx = 150 + drift;
        for (int k = 0; k < 48; k++) {
            int hgt = (int)(rise * sinf(k * PI_F / 48));
            gfx_vline(wx + k * 2, sea - hgt, sea, k % 5 ? C_SKY : C_CYAN);
            gfx_vline(wx + k * 2 + 1, sea - hgt, sea, C_SKY);
            if (hgt > 4) gfx_pset(wx + k * 2, sea - hgt, C_ICE);
        }
        gfx_rect(40, 112, 60, 8, C_ICE);
        spr_draw(&tkw_spr[SP_BURL_SLEEP], 58, 98, 0);
        if ((t / 30) % 2) text_draw("Z", 80, 92, C_LIGHT);
        text_box(20, "BURL SLAPPED THE ICE THREE TIMES, AND THE SEA DID THE REST. THEN HE WENT BACK TO SLEEP.", C_CREAM, C_CYAN);
    } else {
        /* the herd swims for it */
        for (int k = 0; k < 7; k++) {
            int x = (int)(20 + k * 30 + t * 0.4f) % (SCREEN_W + 40) - 20;
            int y = sea + 8 + (k % 3) * 10 + (int)(sinf(t * 0.1f + k) * 1.5f);
            spr_draw_ex(&tkw_spr[SP_BURL], x, y - 8, 0, NULL, k == 3 ? -1 : -1);
            gfx_rect(x, y + 3, 20, 4, C_BLUE);
        }
        text_box(20, "BURL WOKE, AND THE WHOLE HERD SLID INTO THE WATER AND SWAM HARD FOR THE OPEN SEA.", C_CREAM, C_SLATE);
    }
}

static void draw_credits(void) {
    int t = tkg.state_t;
    gfx_cls(C_INK);
    draw_sky_bands(3, t, 100);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    int y = SCREEN_H - t / 3;
    for (int i = 0; TKW_CREDITS[i]; i++) {
        if (i == 0) ui_fancy_center(TKW_CREDITS[i], 160, y, 2, LOGO, 6, C_INK, C_NIGHT);
        else text_center(TKW_CREDITS[i], 160, y + 6, C_LIGHT);
        y += i == 0 ? 24 : 12;
    }
    if (tkg.cherry_end) spr_draw(&tkw_spr[SP_ELDER], 146, imax(70, SCREEN_H + 260 - t / 3), 0);
}

static void draw_wake(void) {
    int t = tkg.state_t;
    draw_floe_scene(t, false);
    gfx_rect(150, 118, 20, 14, C_ICE); /* he's up: draw him awake */
    spr_draw(&tkw_spr[SP_BURL], 150, 118, 0);
    static const uint8_t grad[] = {C_WHITE, C_LIGHT, C_GREY};
    ui_fancy_center("BURL WAKES UP", 160, 28, 2, grad, 3, C_INK, C_NIGHT);
    char buf[64];
    snprintf(buf, sizeof buf, "HE GOT %d%% OF THE WAY, WITH %d SHELLS", tkg.max_pct, tkg.shells);
    text_center_shadow(buf, 160, 56, C_CREAM, C_INK);
    text_center_shadow("THE DREAM WILL BE DIFFERENT NEXT TIME", 160, 70, C_LIGHT, C_INK);
    if (t > 60 && (t / 20) % 2) tiny_center(GLYPH_A " TO THE TITLE", 160, SCREEN_H - 10, C_GREY);
}

/* the second walrus, in grey-blue */
static uint8_t P2MAP[PAL_COUNT];
static bool p2map_ready;

static void draw_brawl(void) {
    int t = tkg.frame_t;
    if (!p2map_ready) {
        pal_identity(P2MAP);
        pal_swap(P2MAP, C_EARTH, C_SKY);
        pal_swap(P2MAP, C_TAN, C_BLUE);
        pal_swap(P2MAP, C_BROWN, C_NAVY);
        pal_swap(P2MAP, C_BLUE, C_RED);
        p2map_ready = true;
    }
    draw_sky_bands(2, t, 150);
    draw_far_clouds(2, 0, t);
    tkg.cam_x = tkg.cam_y = 0;
    int sx = tkg.shake ? (tkg.shake % 2 ? 2 : -2) : 0;
    gfx_camera(sx, 0);
    draw_sea(t);
    for (int i = 0; i < tkw_w.nplat; i++) draw_plat(i, t);
    for (int i = 0; i < tkw_w.nth; i++)
        if (tkw_w.th[i].alive) draw_thing(&tkw_w.th[i], t);
    for (int p = 0; p < 2; p++) {
        const TkwBrawler *br = &tkg.br[p];
        if (br->out) continue;
        const uint8_t *map = p ? P2MAP : NULL;
        bool blink = br->hurt_t > 0 && (t / 2) % 2;
        if (!blink) {
            draw_walrus(&br->b, br->face, br->charging, br->flapping, br->hurt_t > 0, br->anim, map);
            /* across the seam, the other half shows on the far side */
            TkwBody w = br->b;
            if (w.x < 12) { w.x += TKW_BRAWL_W; draw_walrus(&w, br->face, br->charging, br->flapping, br->hurt_t > 0, br->anim, map); }
            if (w.x > TKW_BRAWL_W - 12) { w.x -= TKW_BRAWL_W; draw_walrus(&w, br->face, br->charging, br->flapping, br->hurt_t > 0, br->anim, map); }
        }
        if (on_ground_or_ride(&br->b)) draw_aim(br->b.x, br->b.y, br->face, br->aim, br->charging ? br->charge : -1, -1, t);
        /* a small flap bar over each */
        gfx_rect((int)br->b.x - 8, (int)br->b.y - 20, 16, 2, C_INK);
        gfx_rect((int)br->b.x - 8, (int)br->b.y - 20, br->b.stamina * 16 / TKW_BRAWL_STAMINA, 2, C_YELLOW);
    }
    gfx_camera(0, 0);
    gfx_rect(0, 0, SCREEN_W, TKW_HUD_H, C_INK);
    char buf[48];
    text_draw("P1", 4, 2, C_EARTH);
    text_draw("P2", SCREEN_W - 4 - text_width("P2"), 2, C_SKY);
    for (int p = 0; p < 2; p++)
        for (int k = 0; k < tkg.rounds / 2 + 1; k++) {
            int x = p == 0 ? 20 + k * 8 : SCREEN_W - 26 - k * 8;
            gfx_rect(x, 4, 5, 5, k < tkg.br[p].wins ? C_YELLOW : C_DUSK);
        }
    snprintf(buf, sizeof buf, "ROUND %d " GLYPH_DOT " BEST OF %d", tkg.round_no, tkg.rounds);
    tiny_center(buf, SCREEN_W / 2, 4, C_LIGHT);
}

static void draw_brawl_setup(void) {
    int t = tkg.frame_t + tkg.state_t;
    draw_sky_bands(2, t, 150);
    gfx_rect(0, 150, SCREEN_W, 30, C_NAVY);
    static const uint8_t grad[] = {C_WHITE, C_PINK, C_MAGENTA};
    ui_fancy_center("BRAWL", 160, 20, 3, grad, 3, C_INK, C_NIGHT);
    text_center_shadow("BUMP THE OTHER WALRUS INTO THE SEA", 160, 50, C_CREAM, C_INK);
    text_center_shadow("THE ISLETS CRUMBLE AS YOU GO", 160, 62, C_LIGHT, C_INK);
    char buf[32];
    snprintf(buf, sizeof buf, GLYPH_LEFT " BEST OF %d " GLYPH_RIGHT, tkg.rounds);
    gfx_rect(100, 84, 120, 13, C_INK);
    text_center(buf, 160, 87, C_YELLOW);
    ui_panel(52, 102, 216, 32, C_NIGHT, C_PURPLE);
    tiny_center("P1: LEFT HALF OF THE KEYS OR PAD 1", 160, 108, C_LIGHT);
    tiny_center("P2: RIGHT HALF OF THE KEYS OR PAD 2", 160, 116, C_LIGHT);
    tiny_center("SAME MOVES AS THE JOURNEY, NO ITEMS", 160, 124, C_GREY);
    ui_hint(4, SCREEN_H - 10, GLYPH_B, "BACK", C_GREY);
    ui_hint(SCREEN_W - 50, SCREEN_H - 10, GLYPH_A, "START", C_GREY);
}

static void draw_brawl_round(bool over) {
    draw_brawl();
    gfx_darken_rect(0, TKW_HUD_H, SCREEN_W, SCREEN_H - TKW_HUD_H, 2);
    char buf[48];
    if (over) {
        int w = tkg.br[0].wins > tkg.br[1].wins ? 0 : 1;
        snprintf(buf, sizeof buf, "PLAYER %d WINS THE BRAWL!", w + 1);
        static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER};
        ui_fancy_center(buf, 160, 70, 1, grad, 3, C_INK, C_NIGHT);
        snprintf(buf, sizeof buf, "%d ROUNDS TO %d", tkg.br[w].wins, tkg.br[1 - w].wins);
        text_center(buf, 160, 90, C_LIGHT);
        if ((tkg.state_t / 20) % 2) tiny_center(GLYPH_A " AGAIN", 160, 110, C_GREY);
    } else if (tkg.round_winner < 0) {
        text_center("BOTH IN THE SEA: AGAIN!", 160, 80, C_YELLOW);
    } else {
        snprintf(buf, sizeof buf, "PLAYER %d TAKES THE ROUND", tkg.round_winner + 1);
        text_center(buf, 160, 80, C_YELLOW);
    }
}

void tkw_draw(void) {
    gfx_camera(0, 0);
    gfx_noclip();
    if (tkg.state != TS_JOURNEY && tkg.state != TS_HALL) gfx_set_fade(0);
    switch (tkg.state) {
    case TS_TITLE: draw_title(); break;
    case TS_INTRO: draw_intro(); break;
    case TS_JOURNEY:
    case TS_HALL: draw_journey(); break;
    case TS_TALK: draw_talk(); break;
    case TS_ENDING: draw_ending(); break;
    case TS_CREDITS: draw_credits(); break;
    case TS_WAKE: draw_wake(); break;
    case TS_BRAWL_SETUP: draw_brawl_setup(); break;
    case TS_BRAWL: draw_brawl(); break;
    case TS_BRAWL_ROUND: draw_brawl_round(false); break;
    case TS_BRAWL_OVER: draw_brawl_round(true); break;
    }
}

/* the cartridge label: Burl asleep under the islets */
void tkw_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    static const uint8_t BANDS[] = {C_PURPLE, C_VIOLET, C_MAGENTA, C_PINK, C_ORANGE};
    for (int i = 0; i < 5; i++) gfx_rect(x, y + i * 9, w, 9, BANDS[i]);
    gfx_circ(x + w - 22, y + 38, 9, C_AMBER);
    gfx_rect(x, y + 44, w, h - 44, C_BLUE);
    gfx_dither(x, y + 50, w, h - 50, C_NAVY, 8);
    for (int k = 0; k < 3; k++) {
        int px = x + 10 + k * (w / 3) + (int)(sinf(t * 0.03f + k) * 3), py = y + 14 + (k % 2) * 12;
        gfx_rect(px, py, 26, 3, C_ICE);
        gfx_hline(px + 3, px + 22, py + 3, C_SLATE);
    }
    int bx = x + w / 2 - 10, by = y + h - 18;
    gfx_rect(bx - 8, by + 12, 36, 4, C_ICE);
    spr_draw(&tkw_spr[SP_BURL_SLEEP], bx, by - 1, 0);
    int zt = t % 90;
    text_draw("Z", bx + 20 + zt / 10, by - 4 - zt / 6, C_WHITE);
    spr_draw(&tkw_spr[(t / 20) % 2 ? SP_TERN1 : SP_TERN2], x + 8 + (t / 2) % (w + 10) - 10, y + 6, 0);
    gfx_noclip();
}
