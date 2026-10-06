/* DRIFTLINE - everything on screen: the four skies and the coast road, the
 * cars and their guns, the foes, the bosses (drawn here, not as sprites),
 * the bonus stage, the HUD and the screens between. */
#include "driftline.h"

static const char *num(uint32_t v) {
    static char buf[4][16];
    static int k;
    char *b = buf[k++ & 3];
    if (v >= 1000000) snprintf(b, 16, "%u,%03u,%03u", (unsigned)(v / 1000000), (unsigned)(v / 1000 % 1000), (unsigned)(v % 1000));
    else if (v >= 1000) snprintf(b, 16, "%u,%03u", (unsigned)(v / 1000), (unsigned)(v % 1000));
    else snprintf(b, 16, "%u", (unsigned)v);
    return b;
}

/* ---- the skies --------------------------------------------------------------------- */

static void bands(const uint8_t *cols, int n, int y0, int y1) {
    /* horizontal bands with dithered seams */
    int h = (y1 - y0) / n;
    for (int i = 0; i < n; i++) {
        int y = y0 + i * h, hh = i == n - 1 ? y1 - y : h;
        gfx_rect(0, y, SCREEN_W, hh, cols[i]);
        if (i + 1 < n) gfx_dither(0, y + hh - 3, SCREEN_W, 3, cols[i + 1], 8);
    }
}

static int wrapx(float x, int period) {
    int v = (int)x % period;
    return v < 0 ? v + period : v;
}

static void cloud(int x, int y, int c, int shade) {
    gfx_circ(x, y, 7, c);
    gfx_circ(x + 9, y - 3, 9, c);
    gfx_circ(x + 20, y, 7, c);
    gfx_rect(x, y, 21, 6, c);
    gfx_hline(x - 2, x + 26, y + 6, shade);
}

/* the airship, far off behind the clouds all through stage 1 */
static void far_zephyr(float scroll) {
    int x = 250 - wrapx(scroll * 0.05f, 400) + 60, y = 30;
    if (x < -40 || x > SCREEN_W + 40) return;
    gfx_dither_circle(x, y, 9, C_LIGHT, 10);
    gfx_rect(x - 18, y - 5, 36, 10, C_LIGHT);
    gfx_circ(x - 18, y, 5, C_LIGHT);
    gfx_circ(x + 18, y, 5, C_LIGHT);
    gfx_rect(x - 3, y + 5, 6, 3, C_GREY);
    gfx_hline(x - 20, x + 20, y - 1, C_ICE);
    gfx_vline(x - 24, y - 6, y + 6, C_LIGHT);
}

static void sea(int y0, int y1, int c, int lite, float scroll) {
    gfx_rect(0, y0, SCREEN_W, y1 - y0, c);
    for (int k = 0; k < 26; k++) {
        int y = y0 + 3 + (k * 7) % (y1 - y0 - 4);
        int x = (k * 53 + (int)(scroll * (0.3f + (y - y0) * 0.02f))) % (SCREEN_W + 30) - 15;
        x = SCREEN_W - x;
        gfx_hline(x, x + 4 + k % 5, y, lite);
    }
}

static void road(int stage, float scroll) {
    static const uint8_t ASPH[4] = {C_SLATE, C_DUSK, C_NIGHT, C_SLATE};
    static const uint8_t LINE[4] = {C_WHITE, C_PINK, C_YELLOW, C_WHITE};
    gfx_rect(0, DFL_ROAD_TOP, SCREEN_W, DFL_ROAD_BOT - DFL_ROAD_TOP, ASPH[stage]);
    gfx_dither(0, DFL_ROAD_TOP, SCREEN_W, DFL_ROAD_BOT - DFL_ROAD_TOP, C_INK, 3);
    gfx_hline(0, SCREEN_W - 1, DFL_ROAD_TOP, stage == 1 ? C_MAGENTA : C_GREY);
    gfx_hline(0, SCREEN_W - 1, DFL_ROAD_BOT - 1, C_INK);
    int off = wrapx(scroll * 1.0f, 32);
    for (int x = -off; x < SCREEN_W; x += 32) gfx_rect(x, DFL_ROAD_TOP + 10, 16, 2, LINE[stage]);
    /* the railing along the far side, on a dark wall: the sea stops at the top rail,
     * so these rows must be painted every frame or the barrel's tip, shots and foes
     * that cross them leave copies of themselves behind */
    gfx_rect(0, DFL_ROAD_TOP - 4, SCREEN_W, 4, C_INK);
    int poff = wrapx(scroll * 1.0f, 24);
    for (int x = -poff; x < SCREEN_W; x += 24) gfx_vline(x, DFL_ROAD_TOP - 5, DFL_ROAD_TOP - 1, stage == 2 ? C_DUSK : C_LIGHT);
    gfx_hline(0, SCREEN_W - 1, DFL_ROAD_TOP - 5, stage == 2 ? C_SLATE : C_WHITE);
}

static void palms(float scroll, const uint8_t *remap) {
    int off = wrapx(scroll * 1.0f, 150);
    for (int x = -off; x < SCREEN_W + 20; x += 150) spr_draw_ex(&dfl_spr[SP_PALM], x + 40, DFL_ROAD_TOP - 33, 0, remap, -1);
}

static void draw_sky(int stage, float scroll, int t) {
    switch (stage) {
    case 0: {
        static const uint8_t SKY[] = {C_SKY, C_SKY, C_CYAN, C_ICE};
        bands(SKY, 4, 0, 112);
        gfx_circ(46, 30, 12, C_CREAM);
        gfx_circ(46, 30, 10, C_YELLOW);
        for (int k = 0; k < 5; k++) {
            int x = SCREEN_W - wrapx(scroll * 0.2f + k * 87, SCREEN_W + 60) + 20;
            cloud(x, 18 + (k * 23) % 50, C_WHITE, C_ICE);
        }
        if (!dfg.boss.on) far_zephyr(scroll);
        /* the headland */
        int hx = SCREEN_W - wrapx(scroll * 0.15f, SCREEN_W + 200) + 60;
        gfx_circ(hx, 128, 26, C_JADE);
        gfx_circ(hx + 30, 124, 20, C_FOREST);
        sea(110, DFL_ROAD_TOP - 5, C_BLUE, C_SKY, scroll);
        gfx_rect(0, DFL_ROAD_TOP - 7, SCREEN_W, 2, C_CREAM);
        palms(scroll, NULL);
        break;
    }
    case 1: {
        static const uint8_t SKY[] = {C_NIGHT, C_PURPLE, C_VIOLET, C_MAGENTA, C_PINK, C_ORANGE};
        bands(SKY, 6, 0, 112);
        /* the striped sun, half down */
        for (int y = -32; y < 0; y++) {
            int w = (int)sqrtf((float)(34 * 34 - y * y));
            int c = y < -22 ? C_YELLOW : y < -12 ? C_AMBER : C_ORANGE;
            if (y > -14 && ((y + 40) % 5) < 2) continue;
            gfx_hline(160 - w, 160 + w, 112 + y, c);
        }
        /* the Orrery, far up, already watching */
        if (!dfg.boss.on) {
            int ox = 230, oy = 32;
            gfx_circb(ox, oy, 12, C_VIOLET);
            gfx_circ(ox, oy, 5, C_PINK);
            for (int k = 0; k < 4; k++) {
                float a = t * 0.02f + k * 1.5708f;
                gfx_circ(ox + (int)(cosf(a) * 12), oy + (int)(sinf(a) * 12), 2, C_MAGENTA);
            }
        }
        sea(112, DFL_ROAD_TOP - 5, C_PURPLE, C_PINK, scroll);
        for (int y = 114; y < DFL_ROAD_TOP - 6; y += 3) {
            int w = 30 - (y - 112);
            if (w > 0) gfx_hline(160 - w, 160 + w, y, C_AMBER);
        }
        static uint8_t dark[256];
        static bool made;
        if (!made) { pal_identity(dark); for (int i = 0; i < PAL_COUNT; i++) dark[i] = C_NIGHT; made = true; }
        palms(scroll, dark);
        break;
    }
    case 2: {
        static const uint8_t SKY[] = {C_INK, C_NIGHT, C_NIGHT, C_NAVY};
        bands(SKY, 4, 0, 112);
        for (int k = 0; k < 40; k++) {
            int x = (k * 71 + 13) % SCREEN_W, y = 12 + (k * 37) % 90;
            gfx_pset(x, y, (t / 20 + k) % 7 ? C_LIGHT : C_WHITE);
        }
        /* the moon: an eye, though nobody knows it yet */
        if (!dfg.boss.on) {
            gfx_circ(236, 30, 12, C_CREAM);
            gfx_circ(232, 26, 3, C_TAN);
            gfx_circ(240, 34, 2, C_TAN);
            if ((t / 90) % 6 == 5) {
                int p = dfl_nearest_car(160);
                int dx = p >= 0 ? (dfg.car[p].x > 236 ? 3 : -3) : 0;
                gfx_circ(236 + dx, 33, 4, C_INK);
            }
        }
        /* the graveyard hill */
        int hx = SCREEN_W - wrapx(scroll * 0.12f, SCREEN_W + 160) + 40;
        gfx_circ(hx, 136, 40, C_INK);
        for (int k = 0; k < 4; k++) {
            int gx = hx - 24 + k * 14;
            gfx_rect(gx, 98 + (k % 2) * 4, 6, 9, C_DUSK);
            gfx_rect(gx + 2, 96 + (k % 2) * 4, 2, 2, C_DUSK);
        }
        sea(112, DFL_ROAD_TOP - 5, C_NIGHT, C_DUSK, scroll);
        for (int y = 114; y < DFL_ROAD_TOP - 6; y += 4) gfx_hline(230, 242, y, C_CREAM);
        int off = wrapx(scroll, 110);
        for (int x = -off; x < SCREEN_W + 10; x += 110) {
            spr_draw(&dfl_spr[SP_LAMP], x + 30, DFL_ROAD_TOP - 34, 0);
            bool lit = ((t + x * 7) / 13) % 11 != 0; /* they flicker */
            if (lit) gfx_dither(x + 26, DFL_ROAD_TOP - 31, 14, 20, C_YELLOW, 3);
        }
        break;
    }
    default: {
        static const uint8_t SKY[] = {C_BLUE, C_SKY, C_PINK, C_AMBER, C_CREAM};
        bands(SKY, 5, 0, 104);
        gfx_circ(70, 104, 16, C_YELLOW);
        gfx_circ(70, 104, 13, C_WHITE);
        for (int k = 0; k < 3; k++) {
            int x = SCREEN_W - wrapx(scroll * 0.18f + k * 120, SCREEN_W + 60) + 20;
            cloud(x, 26 + k * 16, C_PINK, C_AMBER);
        }
        sea(104, DFL_ROAD_TOP - 5, C_BLUE, C_ICE, scroll);
        /* claws at the horizon: the crab is out there */
        if (!dfg.boss.on) {
            int cx = 250 + (int)(sinf(t * 0.01f) * 6);
            gfx_circ(cx, 106, 4, C_MAROON);
            gfx_circ(cx + 22, 107, 4, C_MAROON);
            gfx_rect(cx - 4, 106, 30, 3, C_BLUE);
        }
        break;
    }
    }
}

/* stage 4's swells: a whale breaches far out, its wave rolls in and breaks */
static void draw_swells(void) {
    for (int k = 0; k < DFL_MAX_SWELLS; k++) {
        const DflSwell *w = &dfg.swell[k];
        if (!w->alive) continue;
        if (w->t < 50) {
            /* the whale */
            float a = w->t / 50.0f;
            int wx = w->x - 20 + (int)(a * 40), wy = 110 - (int)(sinf(a * 3.14159f) * 12);
            gfx_circ(wx, wy, 4, C_INK);
            gfx_rect(wx - 8, wy - 2, 10, 5, C_INK);
            gfx_rect(wx - 6, wy + 1, 6, 2, C_WHITE);
            gfx_rect(wx - 12, wy - 4, 3, 3, C_INK);
        }
        if (w->t < DFL_SWELL_T) {
            float p = (float)w->t / DFL_SWELL_T;
            int y = 106 + (int)(p * (DFL_ROAD_TOP - 110)), hw = 6 + (int)(p * DFL_SWELL_HW);
            gfx_rect(w->x - hw, y, hw * 2, 3 + (int)(p * 4), C_SKY);
            gfx_hline(w->x - hw, w->x + hw, y, C_WHITE);
            gfx_dither(w->x - hw, y - 2, hw * 2, 2, C_ICE, 8);
            /* the stretch of road it will hit, marked as it nears */
            if (p > 0.5f && (w->t / 4) % 2) gfx_hline(w->x - DFL_SWELL_HW, w->x + DFL_SWELL_HW, DFL_ROAD_BOT - 2, C_ICE);
        } else {
            int h = 26 + (w->t - DFL_SWELL_T) % 6;
            gfx_dither(w->x - DFL_SWELL_HW, DFL_ROAD_BOT - h, DFL_SWELL_HW * 2, h, C_WHITE, 12);
            gfx_dither(w->x - DFL_SWELL_HW, DFL_ROAD_BOT - h - 6, DFL_SWELL_HW * 2, 6, C_ICE, 6);
        }
    }
}

/* ---- the cars ---------------------------------------------------------------------- */

/* Lou's car; in 2P Dee rides in the back on the gun */
static void draw_car_at(float x, float aim, int t, bool drifting, bool gunner) {
    const Sprite *s = &dfl_spr[SP_CAR];
    int bx = (int)lroundf(x) - 12, by = DFL_ROAD_Y - 12 + (drifting && (t / 3) % 2 ? 1 : 0);
    spr_draw(s, bx, by, 0);
    if (gunner) {
        gfx_rect(bx + 5, by, 4, 1, C_INK);
        gfx_rect(bx + 5, by + 1, 4, 1, C_TEAL);
        gfx_rect(bx + 5, by + 2, 4, 1, C_CREAM);
    }
    /* the roof gun: a turret in the middle of the roof with the barrel pivoting on it;
     * shots leave from the barrel's tip (fire_main uses the same point). The barrel is
     * drawn in 9 fixed steps across the cone, one solid 2-pixel line, so it turns in
     * clean clicks instead of shimmering as the aim swings a little every frame. */
    float step = roundf(aim / 11.25f) * 11.25f, a = step * 3.14159265f / 180.0f;
    int gx = (int)lroundf(x), gy = DFL_GUN_Y + (by - (DFL_ROAD_Y - 12));
    int ex = gx + (int)lroundf(sinf(a) * DFL_GUN_LEN), ey = gy - (int)lroundf(cosf(a) * DFL_GUN_LEN);
    /* in the car's red, so it never blends with the railing's grey posts or the
     * dark sea wall that scroll past behind it */
    gfx_line(gx, gy, ex, ey, C_MAROON);
    gfx_line(gx + 1, gy, ex + 1, ey, C_RED);
    gfx_rect(ex, ey, 2, 1, C_YELLOW); /* the muzzle */
    gfx_rect(gx - 2, gy - 1, 6, 3, C_INK);
    gfx_rect(gx - 1, gy - 1, 4, 2, C_SLATE);
}

static void draw_cars(void) {
    for (int p = 0; p < DFL_CARS; p++) {
        const DflCar *c = &dfg.car[p];
        if (!c->on || !c->alive) continue;
        if (c->inv > 0 && (c->inv / 3) % 2) continue;
        draw_car_at(c->x, c->aim, dfg.frame_t, c->drifting, dfg.players == 2);
    }
}

/* ---- foes and shots ---------------------------------------------------------------- */

static void spr_c(int id, float x, float y, int flags, bool white, const uint8_t *remap) {
    const Sprite *s = &dfl_spr[id];
    spr_draw_ex(s, (int)lroundf(x) - s->w / 2, (int)lroundf(y) - s->h / 2, flags, remap, white ? C_WHITE : -1);
}

static void draw_foes(void) {
    int t = dfg.frame_t;
    static uint8_t purple[256];
    static bool made;
    if (!made) {
        pal_identity(purple);
        pal_swap(purple, C_BLUE, C_VIOLET);
        pal_swap(purple, C_SKY, C_PINK);
        pal_swap(purple, C_NAVY, C_PURPLE);
        made = true;
    }
    for (int i = 0; i < DFL_MAX_FOES; i++) {
        const DflFoe *e = &dfg.foe[i];
        if (!e->alive || e->t < 0) continue;
        bool w = e->flash > 0;
        int fl = e->dir < 0 ? SPR_FLIPX : 0;
        switch (e->kind) {
        case FK_KITE: {
            bool angry = e->state == 1 && e->sub > 230 && (t / 4) % 2;
            spr_c((t / 3) % 2 ? SP_KITE1 : SP_KITE2, e->x, e->y, e->state == 1 ? (cosf(e->sub * 0.035f) >= 0 ? (e->dir > 0 ? 0 : SPR_FLIPX) : (e->dir > 0 ? SPR_FLIPX : 0)) : fl, w || angry, NULL);
            break;
        }
        case FK_BUZZER:
            if (e->state == 2 && (e->sub / 3) % 2) spr_c(SP_BUZZER, e->x, e->y, 0, true, NULL);
            else spr_c(SP_BUZZER, e->x, e->y, 0, w, NULL);
            break;
        case FK_HOG: spr_c(SP_HOG, e->x, e->y, e->vx > 0 ? 0 : SPR_FLIPX, w, NULL); break;
        case FK_WRECK:
            spr_c(SP_WRECK, e->x, e->y, 0, w, NULL);
            gfx_dither((int)e->x - 6, (int)e->y - 12 - (t / 3) % 4, 10, 6, C_GREY, 6);
            if ((t / 4) % 2) gfx_rect((int)e->x - 2, (int)e->y - 7, 3, 3, C_ORANGE);
            break;
        case FK_ROTOR: spr_c((t / 2) % 2 ? SP_ROTOR1 : SP_ROTOR2, e->x, e->y, e->dir > 0 ? 0 : SPR_FLIPX, w, NULL); break;
        case FK_SHARD: spr_c(SP_SHARD, e->x, e->y, 0, w, NULL); break;
        case FK_PRISM:
            if (e->state == 3 && (t / 3) % 2) gfx_circ((int)e->x, (int)e->y, 12, C_ICE);
            if (e->state == 2) {
                int bx = (int)e->x, top = (int)e->y + 10;
                gfx_rect(bx - 3, top, 7, DFL_ROAD_Y - top, (t / 2) % 2 ? C_CYAN : C_ICE);
                gfx_rect(bx - 1, top, 3, DFL_ROAD_Y - top, C_WHITE);
                gfx_dither(bx - 7, DFL_ROAD_Y - 6, 15, 6, C_WHITE, 8);
            }
            spr_c(SP_PRISM, e->x, e->y, 0, w, NULL);
            break;
        case FK_HOOP: {
            int x = (int)lroundf(e->x), y = (int)lroundf(e->y);
            gfx_circb(x, y, 8, w ? C_WHITE : C_ORANGE);
            gfx_circb(x, y, 7, w ? C_WHITE : C_YELLOW);
            gfx_circb(x, y, 6, w ? C_WHITE : C_ORANGE);
            gfx_pset(x - 3, y - 5, C_WHITE);
            break;
        }
        case FK_TUMBLER: spr_c((t / 6) % 2 ? SP_TUMBLER1 : SP_TUMBLER2, e->x, e->y, 0, w, NULL); break;
        case FK_SKULL: spr_c(SP_SKULL, e->x, e->y + sinf(t * 0.1f + i) * 1.5f, 0, w, NULL); break;
        case FK_SHEET:
            if (e->state == 2) {
                if ((t / 2) % 3 == 0) spr_c((t / 10) % 2 ? SP_SHEET1 : SP_SHEET2, e->x, e->y, 0, false, NULL);
            } else spr_c((t / 10) % 2 ? SP_SHEET1 : SP_SHEET2, e->x, e->y, 0, w, NULL);
            break;
        case FK_SNAPPER: {
            int id = (t / 8) % 2 ? SP_SNAPPER : SP_SNAPPER2;
            bool blink = e->state == 2 && e->sub > 380 && (t / 2) % 2;
            spr_c(id, e->x, e->y, e->vx < 0 ? SPR_FLIPX : 0, w || blink, e->state == 2 ? purple : NULL);
            break;
        }
        case FK_SLAB: {
            float sx = e->state == 2 ? (float)((t % 3) - 1) * 2 : 0;
            spr_c(SP_SLAB, e->x + sx, e->y, 0, w, NULL);
            break;
        }
        case FK_SAUCER:
            if (w) gfx_dither((int)e->x - 22, (int)e->y - 10, 44, 20, C_WHITE, 8);
            ui_saucer((int)e->x - 21, (int)e->y - 10, t, 2);
            break;
        case FK_JELLY: spr_c((t / 12) % 2 ? SP_JELLY1 : SP_JELLY2, e->x, e->y, 0, w, NULL); break;
        case FK_DARTFISH: spr_c(SP_DART, e->x, e->y, e->vx < 0 ? SPR_FLIPX : 0, w, NULL); break;
        case FK_SQUIRT: spr_c(e->vx * e->dir > 1.0f ? SP_SQUIRT2 : SP_SQUIRT1, e->x, e->y, 0, w, NULL); break;
        case FK_SHARK: spr_c(SP_SHARK, e->x, e->y, (e->vx < 0 ? SPR_FLIPX : 0) | (e->vy > 1.5f ? 0 : 0), w, NULL); break;
        case FK_PLANET: {
            int x = (int)lroundf(e->x), y = (int)lroundf(e->y);
            gfx_circ(x, y, 6, C_INK);
            gfx_circb(x, y, 6, C_DUSK);
            gfx_pset(x - 2, y - 3, C_SLATE);
            gfx_pset(x - 3, y - 2, C_SLATE);
            break;
        }
        default: break;
        }
    }
}

static void draw_eshots(void) {
    for (int i = 0; i < DFL_MAX_ESHOTS; i++) {
        const DflEShot *s = &dfg.es[i];
        if (!s->alive) continue;
        int x = (int)lroundf(s->x), y = (int)lroundf(s->y);
        switch (s->kind) {
        case ES_SHRAPNEL: gfx_rect(x - 1, y - 1, 3, 3, (s->t / 3) % 2 ? C_LIME : C_WHITE); break;
        case ES_BUBBLE:
            gfx_circb(x, y, 3, C_ICE);
            gfx_pset(x - 1, y - 1, C_WHITE);
            break;
        case ES_FLARE:
            gfx_circ(x, y, 2, C_ORANGE);
            gfx_pset(x, y, C_YELLOW);
            break;
        default:
            gfx_circ(x, y, 2, (s->t / 4) % 2 ? C_PINK : C_MAGENTA);
            gfx_pset(x, y, C_WHITE);
            break;
        }
    }
}

static void draw_pshots(void) {
    static const uint8_t COL[3] = {C_LIGHT, C_LIME, C_RED};
    for (int i = 0; i < DFL_MAX_PSHOTS; i++) {
        const DflPShot *s = &dfg.ps[i];
        if (!s->alive) continue;
        int tier = s->dmg >= 4 ? 2 : s->dmg >= 2 ? 1 : 0;
        int x = (int)lroundf(s->x), y = (int)lroundf(s->y);
        int tx = x - (int)lroundf(s->vx * 0.6f), ty = y - (int)lroundf(s->vy * 0.6f);
        gfx_line(tx, ty, x, y, COL[tier]);
        gfx_pset(x, y, tier == 2 ? C_YELLOW : C_WHITE);
    }
}

static void draw_parts(void) {
    for (int i = 0; i < DFL_MAX_PARTS; i++) {
        const DflPart *p = &dfg.part[i];
        if (p->life > 0) gfx_pset((int)p->x, (int)p->y, p->col);
    }
}

/* ---- the bosses ----------------------------------------------------------------------- */

static void draw_zephyr(const DflBoss *b, int t) {
    int x = (int)lroundf(b->x), y = (int)lroundf(b->y);
    bool w = b->flash > 0;
    /* fins, the gasbag, its stripes */
    gfx_rect(x - 84, y - 14, 10, 28, C_GREY);
    gfx_rect(x - 82, y - 1, 16, 2, C_SLATE);
    for (int r = 15; r >= 0; r--) {
        int hw = (int)(74 * sqrtf(1.0f - (r / 15.5f) * (r / 15.5f)));
        int c = r > 11 ? C_SLATE : r > 5 ? C_GREY : C_LIGHT;
        gfx_hline(x - hw, x + hw, y - r, r < 3 ? C_WHITE : c);
        gfx_hline(x - hw, x + hw, y + r, r > 11 ? C_DUSK : C_GREY);
    }
    for (int k = -2; k <= 2; k++) gfx_vline(x + k * 26, y - 13, y + 13, C_RED);
    gfx_hline(x - 70, x + 70, y + 2, C_MAROON);
    text_center("ZEPHYR", x + 2, y - 4, C_RED);
    /* the gondola */
    gfx_rect(x - 20, y + 13, 40, 7, C_BROWN);
    for (int k = 0; k < 5; k++) gfx_rect(x - 17 + k * 8, y + 15, 4, 3, (t / 20 + k) % 4 ? C_YELLOW : C_CREAM);
    /* the turrets */
    for (int k = 0; k < 3; k++) {
        int tx = x + (k - 1) * 48, ty = y + (k == 1 ? 20 : 17);
        if (b->part_hp[k] <= 0) {
            gfx_rect(tx - 5, ty - 3, 10, 6, C_INK);
            gfx_dither(tx - 6, ty - 10 - (t / 4) % 4, 10, 6, C_GREY, 6);
            continue;
        }
        int p = dfl_nearest_car((float)tx);
        float a = atan2f(DFL_ROAD_Y - (float)ty, (p >= 0 ? dfg.car[p].x : 160) - tx);
        gfx_line(tx, ty, tx + (int)(cosf(a) * 9), ty + (int)(sinf(a) * 9), C_INK);
        gfx_circ(tx, ty, 6, w ? C_WHITE : C_DUSK);
        gfx_circ(tx, ty - 1, 4, w ? C_WHITE : C_SLATE);
        gfx_pset(tx - 2, ty - 3, C_LIGHT);
        /* a turret past half smokes */
        if (b->part_hp[k] * 2 < DFL_TURRET_HP) gfx_dither(tx - 4, ty - 9 - (t / 5) % 4, 8, 5, C_GREY, 5);
    }
}

static void draw_orrery(const DflBoss *b, int t) {
    int x = (int)lroundf(b->x), y = (int)lroundf(b->y);
    for (int k = 0; k < 24; k++) {
        float a = k * 0.2618f + t * 0.01f;
        gfx_pset(x + (int)(cosf(a) * 30), y + (int)(sinf(a) * 30), C_VIOLET);
    }
    int third = b->hp * 3 / b->maxhp;
    int body = third >= 2 ? C_ICE : third >= 1 ? C_YELLOW : C_RED;
    int shade = third >= 2 ? C_SKY : third >= 1 ? C_AMBER : C_MAROON;
    if (b->flash > 0) body = shade = C_WHITE;
    gfx_circ(x, y, 14, shade);
    gfx_circ(x - 2, y - 2, 11, body);
    gfx_circ(x - 5, y - 5, 3, C_WHITE);
    for (int k = 0; k < 4; k++) {
        if (b->part_hp[k] <= 0) continue;
        float a = b->orbit_a + k * 1.5708f;
        int ox = x + (int)lroundf(cosf(a) * 30), oy = y + (int)lroundf(sinf(a) * 30);
        gfx_circ(ox, oy, 6, C_PURPLE);
        gfx_circ(ox - 1, oy - 1, 4, C_MAGENTA);
        gfx_pset(ox - 2, oy - 2, C_PINK);
    }
}

static void draw_moon(const DflBoss *b, int t) {
    int lvl = b->revealed ? 16 : iclamp(b->t / 6, 0, 16);
    /* the face around the moon */
    gfx_dither_circle(250, 34, 52, C_CREAM, lvl);
    if (lvl >= 16) {
        gfx_circb(250, 34, 52, C_TAN);
        gfx_circ(272, 20, 5, C_TAN);
        gfx_circ(282, 56, 4, C_TAN);
        gfx_circ(212, 64, 3, C_TAN);
        /* the other eye, shut; the mouth */
        gfx_hline(266, 282, 30, C_BROWN);
        gfx_hline(268, 280, 31, C_BROWN);
        bool open = b->hand_state == 1 && (b->hand_t % 40) < 22;
        if (open) gfx_circ(240, 66, 7, C_MAROON);
        else gfx_hline(226, 252, 66, C_BROWN);
    }
    /* the eye: the moon of the whole stage, the only place it can be hurt */
    gfx_circ(236, 30, 12, b->flash > 0 ? C_WHITE : C_WHITE);
    gfx_circb(236, 30, 12, C_TAN);
    int p = dfl_nearest_car(160);
    float cx = p >= 0 ? dfg.car[p].x : 160;
    float a = atan2f(DFL_ROAD_Y - 30.0f, cx - 236);
    gfx_circ(236 + (int)(cosf(a) * 5), 30 + (int)(sinf(a) * 5), 5, b->flash > 0 ? C_RED : C_INK);
    /* the hand */
    if (b->revealed) {
        int hx = (int)lroundf(b->hx), hy = (int)lroundf(b->hy);
        bool fist = b->hand_state == 2;
        gfx_circ(hx, hy, fist ? 10 : 9, C_WHITE);
        gfx_circb(hx, hy, fist ? 10 : 9, C_GREY);
        if (!fist)
            for (int k = 0; k < 4; k++) {
                gfx_rect(hx - 8 + k * 5, hy - 15, 4, 8, C_WHITE);
                gfx_rectb(hx - 8 + k * 5, hy - 15, 4, 8, C_GREY);
            }
        else
            for (int k = 0; k < 3; k++) gfx_hline(hx - 6, hx + 2, hy - 4 + k * 4, C_GREY);
        gfx_rect(hx + 8, hy - 4, 8, 8, C_LIGHT); /* the cuff */
    }
    (void)t;
}

static void draw_crab(const DflBoss *b, int t) {
    int x = (int)lroundf(b->x), y = (int)lroundf(b->y);
    /* the legs */
    for (int k = 0; k < 6; k++) {
        int lx = (int)lroundf(dfl_leg_x(k)), foot = (int)lroundf(dfl_leg_foot(k));
        int pt = b->part_t[k];
        bool warn = pt >= 0 && pt < 50;
        int c = warn ? ((t / 3) % 2 ? C_WHITE : C_YELLOW) : C_ORANGE;
        int kx = x + (lx - x) * 3 / 4;
        gfx_line(kx, y + 8, lx, y + 30, C_MAROON);
        gfx_line(kx + 1, y + 8, lx + 1, y + 30, C_MAROON);
        gfx_rect(lx - 2, y + 30, 5, foot - (y + 30), c);
        gfx_vline(lx - 2, y + 30, foot, C_MAROON);
        gfx_rect(lx - 3, foot - 4, 7, 4, C_RED);
        gfx_pset(lx, foot, C_INK);
    }
    /* the claws */
    for (int s = -1; s <= 1; s += 2) {
        int cx = x + s * 66, cy = y + 4 + (int)(sinf(t * 0.05f + s) * 3);
        gfx_circ(cx, cy, 10, C_RED);
        gfx_circ(cx + s * 4, cy - 6, 6, C_RED);
        gfx_line(cx, cy - 2, cx + s * 10, cy - 10, C_MAROON);
        gfx_circb(cx, cy, 10, C_MAROON);
    }
    /* the shell */
    bool w = b->flash > 0;
    for (int r = 20; r >= 0; r--) {
        int hw = (int)(56 * sqrtf(1.0f - (r / 20.5f) * (r / 20.5f)));
        gfx_hline(x - hw, x + hw, y - r, r > 14 ? C_MAROON : C_RED);
        gfx_hline(x - hw, x + hw, y + r, r > 14 ? C_MAROON : C_WINE);
    }
    gfx_dither(x - 40, y - 14, 80, 6, C_ORANGE, 5);
    /* the eyes on their stalks, and the bandana from ROOFCAT's harbour */
    for (int s = -1; s <= 1; s += 2) {
        gfx_vline(x + s * 14, y - 30, y - 18, C_MAROON);
        gfx_circ(x + s * 14, y - 32, 4, C_WHITE);
        gfx_circ(x + s * 14 + (t / 40 % 2 ? 1 : -1), y - 32, 2, C_INK);
    }
    gfx_rect(x - 50, y - 12, 100, 3, C_BLUE);
    gfx_rect(x + 46, y - 12, 4, 10, C_BLUE);
    /* the mark: the only place it can be hurt */
    int mc = w ? C_WHITE : ((t / 8) % 2 ? C_YELLOW : C_AMBER);
    gfx_line(x - 5, y - 11, x + 5, y - 1, mc);
    gfx_line(x - 5, y - 1, x + 5, y - 11, mc);
    gfx_line(x - 4, y - 11, x + 6, y - 1, mc);
    gfx_line(x - 4, y - 1, x + 6, y - 11, mc);
    /* the mouth */
    gfx_hline(x - 8, x + 8, y + 12, C_INK);
    if (b->t % 90 > 30 && b->t % 90 < 46) gfx_circ(x, y + 14, 4, C_INK);
}

static void draw_boss(void) {
    const DflBoss *b = &dfg.boss;
    if (!b->on) return;
    if (b->dead && b->dead_t > 120) return;
    int t = dfg.frame_t;
    if (b->dead && (b->dead_t / 3) % 2) return;
    switch (b->kind) {
    case BOSS_ZEPHYR: draw_zephyr(b, t); break;
    case BOSS_ORRERY: draw_orrery(b, t); break;
    case BOSS_MOON: draw_moon(b, t); break;
    default: draw_crab(b, t); break;
    }
}

/* ---- the HUD ------------------------------------------------------------------------ */

static void draw_meter(int x, int w) {
    const DflCar *c = &dfg.car[0];
    int y = DFL_ROAD_BOT + 3;
    int seg = w / 3;
    static const uint8_t BACK[3] = {C_DUSK, C_FOREST, C_MAROON};
    static const uint8_t FILL[3] = {C_LIGHT, C_LIME, C_RED};
    for (int k = 0; k < 3; k++) gfx_rect(x + k * seg, y, seg - 1, 6, BACK[k]);
    int fill = c->alive ? c->meter * (seg * 3) / DFL_METER_MAX : 0;
    for (int k = 0; k < 3; k++) {
        int f = iclamp(fill - k * seg, 0, seg - 1);
        if (f > 0) gfx_rect(x + k * seg, y, f, 6, FILL[k]);
    }
    if (c->alive && dfl_tier(c->meter) == 2 && (dfg.frame_t / 6) % 2) gfx_rectb(x - 1, y - 1, seg * 3 + 1, 8, C_YELLOW);
}

/* the strip under the road, as on the original: the score on the left, the
 * power bar in the middle, the cars in reserve on the right */
static void draw_hud(void) {
    gfx_rect(0, DFL_ROAD_BOT, SCREEN_W, SCREEN_H - DFL_ROAD_BOT, C_INK);
    text_draw(num(dfg.score), 4, DFL_ROAD_BOT + 3, C_WHITE);
    draw_meter(72, 174);
    char buf[16];
    int rx = SCREEN_W - 3;
    int show = imin(dfg.spare, 5);
    for (int k = 0; k < show; k++) {
        rx -= 9;
        int y = DFL_ROAD_BOT + 4;
        gfx_rect(rx, y + 2, 8, 3, C_RED);
        gfx_rect(rx + 2, y, 3, 2, C_RED);
        gfx_pset(rx + 1, y + 5, C_GREY);
        gfx_pset(rx + 6, y + 5, C_GREY);
    }
    if (dfg.spare > 5) { snprintf(buf, sizeof buf, "%d", dfg.spare); rx -= tiny_width(buf) + 2; tiny_draw(buf, rx, DFL_ROAD_BOT + 5, C_LIGHT); }
}

static void draw_banner(void) {
    if (dfg.stage_t >= DFL_BANNER_T || dfg.state != DS_PLAY) return;
    char buf[40];
    snprintf(buf, sizeof buf, "STAGE %d", dfg.stage + 1);
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center(buf, 160, 52, 2, grad, 4, C_INK, C_MAROON);
    text_center_shadow(DFL_STAGE[dfg.stage].name, 160, 76, C_WHITE, C_INK);
    text_center_shadow(DFL_STAGE_TIME[dfg.stage], 160, 87, C_CREAM, C_INK);
}

static void draw_world(void) {
    int sx = dfg.shake > 0 ? (dfg.frame_t % 3) - 1 : 0;
    draw_sky(dfg.stage, dfg.scroll, dfg.frame_t);
    gfx_camera(sx, 0);
    draw_boss();
    if (dfg.stage == 3) draw_swells();
    road(dfg.stage, dfg.scroll);
    draw_foes();
    draw_cars();
    draw_pshots();
    draw_eshots();
    draw_parts();
    gfx_camera(0, 0);
    draw_hud();
}

/* ---- the bonus stage --------------------------------------------------------------------- */

static void draw_bonus(bool status) {
    static const uint8_t SKY[] = {C_NIGHT, C_NAVY, C_BLUE};
    bands(SKY, 3, 0, DFL_ROAD_TOP);
    for (int k = 0; k < 30; k++) gfx_pset((k * 67 + (int)dfg.scroll) % SCREEN_W, 14 + (k * 29) % 120, C_DUSK);
    road(0, dfg.scroll);
    static const uint8_t ROWC[DFL_BROWS] = {C_PINK, C_MAGENTA, C_ORANGE, C_AMBER, C_YELLOW, C_LIME, C_CYAN};
    for (int r = 0; r < DFL_BROWS; r++)
        for (int c = 0; c < DFL_BCOLS; c++) {
            int hp = dfg.block[r][c];
            if (!hp) continue;
            int x = DFL_BLOCK_X0 + c * DFL_BLOCK_W, y = DFL_BLOCK_Y0 + r * DFL_BLOCK_H;
            gfx_rect(x + 1, y + 1, DFL_BLOCK_W - 2, DFL_BLOCK_H - 2, ROWC[r]);
            gfx_hline(x + 1, x + DFL_BLOCK_W - 2, y + 1, C_WHITE);
            gfx_rectb(x, y, DFL_BLOCK_W, DFL_BLOCK_H, C_INK);
            if (hp < DFL_BLOCK_HP) gfx_dither(x + 2, y + 2, (DFL_BLOCK_HP - hp) * (DFL_BLOCK_W - 4) / DFL_BLOCK_HP, DFL_BLOCK_H - 4, C_INK, 8);
        }
    if (dfg.ball_live) {
        int x = (int)lroundf(dfg.bx), y = (int)lroundf(dfg.by);
        gfx_circb(x, y, 6, C_ICE);
        gfx_pset(x - 3, y - 3, C_WHITE);
        spr_draw(&dfl_spr[SP_COIN], x - 5, y - 5, 0);
        gfx_circb(x, y, 6, C_ICE);
    }
    if (dfg.coin_out) spr_draw_scaled(&dfl_spr[SP_COIN], 150, (int)dfg.coin_y - 10, 2, 0);
    draw_cars();
    draw_pshots();
    draw_parts();
    draw_hud();
    tiny_center("BONUS STAGE", 160, 2, C_YELLOW);
    if (!status) return;
    if (dfg.stage_t < 60) text_center_shadow("KEEP THE COIN UP!", 160, 104, C_WHITE, C_INK);
    if (dfg.bonus_won) text_center_shadow("THE BUBBLE BURSTS!", 160, 104, C_YELLOW, C_INK);
    else if (!dfg.ball_live && !dfg.coin_out) text_center_shadow("THE COIN GOT AWAY", 160, 104, C_LIGHT, C_INK);
}

/* ---- the screens ------------------------------------------------------------------------- */

static const uint8_t LOGO[] = {C_YELLOW, C_AMBER, C_ORANGE, C_RED, C_MAGENTA, C_PURPLE};

static void draw_title(void) {
    int t = dfg.frame_t;
    draw_sky(1, dfg.scroll, t);
    road(1, dfg.scroll);
    float cx = 160 + sinf(t * 0.02f) * 60;
    draw_car_at(cx, sinf(t * 0.02f) * 30, t, cosf(t * 0.02f) < 0, false);
    ui_fancy_center("DRIFTLINE", 160, 18, 3, LOGO, 6, C_INK, C_NIGHT);
    text_center_shadow("THE COAST ROAD, MORNING TO DAYBREAK", 160, 48, C_CREAM, C_INK);
    static const char *const ITEMS[2] = {"1 PLAYER", "2 PLAYERS"};
    for (int i = 0; i < 2; i++) {
        int y = 66 + i * 12;
        bool off = i == 1 && plat_kind() == PLAT_VITA;
        gfx_rect(124, y - 2, 72, 11, C_INK);
        text_center(ITEMS[i], 160, y, off ? C_SLATE : dfg.sel == i ? C_YELLOW : C_LIGHT);
        if (dfg.sel == i) ui_cursor(126, y, t);
    }
    gfx_rect(0, 0, SCREEN_W, 10, C_INK);
    char buf[64];
    snprintf(buf, sizeof buf, "BEST %s", num(dfs.best));
    tiny_draw(buf, 4, 3, C_YELLOW);
    snprintf(buf, sizeof buf, "MOST CARS AT ONCE %d", dfs.most_cars);
    tiny_draw(buf, SCREEN_W - 4 - tiny_width(buf), 3, C_LIGHT);
    gfx_rect(0, DFL_ROAD_BOT, SCREEN_W, SCREEN_H - DFL_ROAD_BOT, C_INK);
    ui_hint(4, DFL_ROAD_BOT + 2, GLYPH_B, "LIBRARY", C_GREY);
    tiny_draw("BEAMDOWN SOFTWORKS 1989", SCREEN_W - 4 - tiny_width("BEAMDOWN SOFTWORKS 1989"), DFL_ROAD_BOT + 4, C_SLATE);
}

static void draw_clear(void) {
    draw_world();
    gfx_rect(70, 50, 180, 62, C_INK);
    gfx_rectb(70, 50, 180, 62, C_YELLOW);
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER};
    ui_fancy_center("STAGE CLEAR", 160, 56, 2, grad, 3, C_INK, C_MAROON);
    char buf[48];
    snprintf(buf, sizeof buf, "%s  +%s", DFL_STAGE[dfg.stage].name, num((uint32_t)DFL_BOSS_POINTS[dfg.stage]));
    text_center(buf, 160, 78, C_LIGHT);
    if (dfg.lost_stage == 0 && dfg.stage < DFL_STAGES - 1) {
        text_center("NO CARS LOST!", 160, 90, C_LIME);
        if ((dfg.state_t / 12) % 2) text_center("BONUS STAGE NEXT", 160, 100, C_YELLOW);
    } else {
        snprintf(buf, sizeof buf, dfg.lost_stage == 1 ? "%d CAR LOST" : "%d CARS LOST", dfg.lost_stage);
        text_center(dfg.lost_stage ? buf : "NO CARS LOST!", 160, 92, dfg.lost_stage ? C_GREY : C_LIME);
    }
}

static void draw_bonus_end(void) {
    draw_bonus(false);
    gfx_rect(70, 52, 180, 56, C_INK);
    gfx_rectb(70, 52, 180, 56, C_CYAN);
    text_center("BONUS STAGE", 160, 58, C_WHITE);
    char buf[48];
    snprintf(buf, sizeof buf, "%d BLOCKS  %s", dfg.blocks_broken, num(dfg.bonus_points));
    text_center(buf, 160, 72, C_YELLOW);
    if (dfg.bonus_won) text_center("COIN! TWO MORE CARS", 160, 88, C_LIME);
    else text_center("NO COIN THIS TIME", 160, 88, C_GREY);
}

static void draw_over(void) {
    draw_world();
    gfx_dither(0, DFL_HUD_H, SCREEN_W, DFL_ROAD_BOT - DFL_HUD_H, C_INK, 8);
    static const uint8_t grad[] = {C_WHITE, C_LIGHT, C_GREY};
    ui_fancy_center("OUT OF CARS", 160, 60, 2, grad, 3, C_INK, C_MAROON);
    char buf[48];
    snprintf(buf, sizeof buf, "SCORE %s", num(dfg.score));
    text_center(buf, 160, 86, C_YELLOW);
    snprintf(buf, sizeof buf, "STAGE %d, %s", dfg.stage + 1, DFL_STAGE[dfg.stage].name);
    text_center(buf, 160, 98, C_LIGHT);
}

static const char *const ENDING[] = {
    "THE COAST ROAD RUNS OUT AT THE LIGHTHOUSE.",
    "LOU PARKS THE GULL ON THE SAND",
    "AND WATCHES THE SUN COME UP OVER THE SEA.",
    "",
    "THE ZEPHYR, THE ORRERY, THE MAN IN THE MOON",
    "AND OLD CRAB ARE ALL BEHIND HER NOW.",
    "",
    "SOME DRIVES ARE WORTH EVERY MILE.",
};

static void draw_ending(void) {
    int t = dfg.frame_t;
    draw_sky(3, dfg.scroll, t);
    road(3, dfg.scroll);
    /* the lighthouse, and the car parked */
    gfx_rect(270, 70, 14, 76, C_WHITE);
    for (int k = 0; k < 4; k++) gfx_rect(270, 78 + k * 18, 14, 6, C_RED);
    gfx_rect(266, 64, 22, 6, C_INK);
    if ((t / 30) % 2) gfx_dither(240, 58, 30, 10, C_YELLOW, 6);
    draw_car_at(220, 0, 0, false, dfg.players == 2);
    gfx_darken_rect(0, 14, SCREEN_W, 112, 3);
    int shown = iclamp(dfg.state_t / 40, 0, ARRAY_LEN(ENDING));
    for (int i = 0; i < shown; i++) text_center_shadow(ENDING[i], 160, 20 + i * 10, C_WHITE, C_INK);
    if (dfg.state_t > 360) {
        char buf[48];
        snprintf(buf, sizeof buf, "FINAL SCORE %s", num(dfg.score));
        text_center_shadow(buf, 160, 104, C_YELLOW, C_INK);
        if (dfg.score >= 300000u) text_center_shadow("A DRIVE FOR THE RECORD BOOKS!", 160, 114, C_LIME, C_INK);
    }
    gfx_rect(0, 0, SCREEN_W, 10, C_INK);
    gfx_rect(0, DFL_ROAD_BOT, SCREEN_W, SCREEN_H - DFL_ROAD_BOT, C_INK);
}

static const char *const CREDITS[] = {
    "DRIFTLINE", "", "A BEAMDOWN SOFTWORKS GAME, 1989", "", "",
    "AT THE WHEEL", "LOU AND THE GULL", "", "IN THE BACK SEAT", "DEE ON THE GUN, FOR TWO PLAYERS", "", "",
    "ON HARBOUR ROAD", "KITES, BUZZERS, ROAD HOGS, ROTORS", "THE ZEPHYR", "",
    "ON THE SUNDOWN STRIP", "SHARDS, PRISMS, HOOPS, TUMBLERS", "THE ORRERY", "",
    "ON THE MOONLIT MILE", "SKULLS, SHEETS, SNAPPERS, SLABS", "A SAUCER FROM BEAMDOWN", "THE MAN IN THE MOON", "",
    "ON THE OPEN WATER", "JELLIES, DARTFISH, SQUIRTS, SHARKS", "THE WHALES", "OLD CRAB, ALL THE WAY FROM ROOFCAT", "", "",
    "A UFO 40 TRIBUTE TO", "SEASIDE DRIVE", "", "",
    "THANKS FOR DRIVING",
};

static void draw_credits(void) {
    gfx_cls(C_INK);
    int t = dfg.frame_t;
    for (int k = 0; k < 30; k++) gfx_pset((k * 97) % SCREEN_W, (k * 53 + t / 3) % SCREEN_H, C_DUSK);
    int y0 = SCREEN_H - dfg.state_t / 3;
    for (int i = 0; i < ARRAY_LEN(CREDITS); i++) {
        int y = y0 + i * 11;
        if (y < -10 || y > SCREEN_H) continue;
        bool head = i == 0 || !strncmp(CREDITS[i], "ON ", 3) || !strcmp(CREDITS[i], "AT THE WHEEL") || !strcmp(CREDITS[i], "IN THE BACK SEAT") || !strcmp(CREDITS[i], "SEASIDE DRIVE");
        text_center(CREDITS[i], 160, y, head ? C_YELLOW : C_LIGHT);
    }
}

void dfl_draw(void) {
    switch (dfg.state) {
    case DS_TITLE: draw_title(); break;
    case DS_PLAY: draw_world(); draw_banner(); break;
    case DS_CLEAR: draw_clear(); break;
    case DS_BONUS: draw_bonus(true); break;
    case DS_BONUS_END: draw_bonus_end(); break;
    case DS_OVER: draw_over(); break;
    case DS_ENDING: draw_ending(); break;
    default: draw_credits(); break;
    }
}

void dfl_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    static const uint8_t SKY[] = {C_PURPLE, C_VIOLET, C_MAGENTA, C_PINK, C_ORANGE};
    for (int i = 0; i < 5; i++) gfx_rect(x, y + i * 8, w, 8, SKY[i]);
    for (int r = 0; r < 14; r++) {
        int hw = (int)sqrtf((float)(14 * 14 - r * r));
        if (r < 5 && r % 2) continue;
        gfx_hline(x + w / 2 - hw, x + w / 2 + hw, y + 40 - r, r > 8 ? C_YELLOW : C_AMBER);
    }
    gfx_rect(x, y + 40, w, 10, C_PURPLE);
    gfx_rect(x, y + 50, w, h - 50, C_DUSK);
    int off = (t * 2) % 24;
    for (int k = -1; k < w / 24 + 2; k++) gfx_rect(x + k * 24 - off, y + 56, 12, 1, C_PINK);
    int cx = x + w / 2 + (int)(sinf(t * 0.04f) * 30);
    spr_draw(&dfl_spr[SP_CAR], cx - 12, y + h - 13, 0);
    gfx_line(cx + 1, y + h - 12, cx + 1 + (int)(sinf(t * 0.04f) * 6), y + h - 20, C_INK);
    spr_draw(&dfl_spr[(t / 3) % 2 ? SP_KITE1 : SP_KITE2], x + 18 + (t / 2) % (w + 20) - 20, y + 12, 0);
    for (int k = 0; k < 3; k++) {
        int by = y + h - 22 - ((t * 3 + k * 12) % 36);
        gfx_pset(cx + 1 + (int)(sinf(t * 0.04f) * (h - 22 - (by - y)) * 0.2f), by, C_YELLOW);
    }
    gfx_noclip();
}
