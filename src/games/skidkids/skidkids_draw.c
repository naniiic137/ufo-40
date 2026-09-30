/* SKID KIDS - drawing the gym, the court and everything on it. */
#include "skidkids.h"

const char *const SKID_BARK_TEXT[BARK_COUNT] = {
    "", "PLAY BALL!", "HEADS UP!", "NICE JUMP!", "OOF!", "HUSTLE!", "BE NICE NOW!", "DRINK UP!", "WHOA THERE!", "GAME!",
};

/* a kid with its feet at (x, y) */
void skid_draw_kid_at(int who, int team, int pose, int x, int y, int flip, int scale, bool cry) {
    const Sprite *s = &skid_kid_spr[who][team & 1][pose];
    int w = s->w * scale, h = s->h * scale;
    int x0 = x - w / 2, y0 = y - h + scale;
    if (scale == 1) spr_draw(s, x0, y0, flip ? SPR_FLIPX : 0);
    else spr_draw_scaled(s, x0, y0, scale, flip ? SPR_FLIPX : 0);
    if (cry && who < SKID_KIDS && pose != KP_DOWN) {
        /* tears streaming from the eye */
        int ex = flip ? x0 + (11 - 8) * scale : x0 + 8 * scale;
        for (int k = 0; k < 3; k++) gfx_rect(ex, y0 + (5 + k * 2) * scale, scale, scale, C_SKY);
    }
}

int skid_kid_pose(const Kid *k, int frame) {
    switch (k->state) {
    case KS_DOWN: return KP_DOWN;
    case KS_HURT: return KP_HURT;
    case KS_PICKUP: return KP_PICK;
    case KS_CHARGE: return KP_WIND;
    case KS_THROW: return KP_THROW;
    case KS_WIN: return (frame / 10) % 2 ? KP_WIN : KP_JUMP;
    case KS_SAD: return KP_IDLE0;
    default: break;
    }
    if (skid_airborne(k)) return KP_JUMP;
    if (k->moved) return (k->anim / 6) % 2 ? KP_RUN1 : KP_RUN0;
    return (frame / 30) % 2 ? KP_IDLE1 : KP_IDLE0;
}

/* ---- the gym ---------------------------------------------------------------- */

static void draw_banner(int x, int y, int frame) {
    /* the school banner: STING 'EM, HORNETS! (the code hides in it) */
    ui_panel(x, y, 96, 13, C_YELLOW, C_INK);
    for (int i = 0; i < 6; i++) gfx_rect(x + 3 + i * 16, y + 2, 8, 2, C_INK);
    const char *t = "STING 'EM, HORNETS!";
    tiny_center(t, x + 48, y + 6, C_INK);
    (void)frame;
}

/* The far wall of the gym, from y0 down to the floor. */
void skid_draw_gym(int frame, int y0, bool plain) {
    gfx_rect(0, y0, SCREEN_W, SKID_TOP - 4 - y0, C_CREAM);
    /* painted cinder blocks */
    for (int y = y0; y < SKID_TOP - 4; y += 5) {
        gfx_hline(0, SCREEN_W - 1, y, C_TAN);
        for (int x = ((y - y0) / 5 % 2) * 8; x < SCREEN_W; x += 16) gfx_vline(x, y, y + 4, C_TAN);
    }
    /* a maroon stripe, two high windows and the clock (not behind menus) */
    gfx_rect(0, SKID_TOP - 12, SCREEN_W, 3, C_MAROON);
    gfx_rect(0, SKID_TOP - 5, SCREEN_W, 2, C_BROWN);
    if (plain) return;
    for (int i = 0; i < 2; i++) {
        int wx = i == 0 ? 20 : 256;
        gfx_rect(wx, y0 + 2, 44, 10, C_SKY);
        gfx_rectb(wx - 1, y0 + 1, 46, 12, C_SLATE);
        gfx_vline(wx + 22, y0 + 2, y0 + 11, C_SLATE);
        gfx_pset(wx + 5 + (frame / 40) % 30, y0 + 4, C_WHITE);
    }
    gfx_circ(120, y0 + 8, 5, C_WHITE);
    gfx_circb(120, y0 + 8, 5, C_INK);
    gfx_line(120, y0 + 8, 120, y0 + 5, C_INK);
    gfx_line(120, y0 + 8, 122 + (frame / 60) % 2, y0 + 9, C_INK);
    draw_banner(176, y0 + 2, frame);
    /* the baseboard */
    gfx_rect(0, SKID_TOP - 5, SCREEN_W, 2, C_BROWN);
}

static void draw_floor(void) {
    gfx_rect(0, SKID_TOP - 3, SCREEN_W, SCREEN_H - SKID_TOP + 3, C_AMBER);
    /* long boards, their joins staggered */
    for (int y = SKID_TOP - 3, row = 0; y < SCREEN_H; y += 6, row++) {
        gfx_hline(0, SCREEN_W - 1, y, C_TAN);
        for (int x = (row * 37) % 70; x < SCREEN_W; x += 70) gfx_vline(x, y + 1, y + 5, C_TAN);
        if (row % 3 == 1) gfx_dither(0, y + 1, SCREEN_W, 5, C_ORANGE, 2);
    }
    /* the court's lines: the edge, the centre line, the centre circle, and a
     * team line near each back wall */
    gfx_rectb(SKID_LEFT + 1, SKID_TOP - 2, SKID_RIGHT - SKID_LEFT - 1, SKID_BOT - SKID_TOP + 6, C_WHITE);
    gfx_rect(SKID_MID - 1, SKID_TOP - 2, 3, SKID_BOT - SKID_TOP + 6, C_RED);
    gfx_circb(SKID_MID, (SKID_TOP + SKID_BOT) / 2, 18, C_RED);
    gfx_circb(SKID_MID, (SKID_TOP + SKID_BOT) / 2, 17, C_RED);
    gfx_vline(SKID_LEFT + 22, SKID_TOP - 1, SKID_BOT + 2, C_WINE);
    gfx_vline(SKID_RIGHT - 22, SKID_TOP - 1, SKID_BOT + 2, C_NAVY);
    /* the hornet on the centre spot: our own crest */
    int cx = SKID_MID, cy = (SKID_TOP + SKID_BOT) / 2;
    gfx_rect(cx - 5, cy - 3, 11, 7, C_YELLOW);
    gfx_rect(cx - 3, cy - 3, 2, 7, C_INK);
    gfx_rect(cx + 1, cy - 3, 2, 7, C_INK);
    gfx_line(cx - 6, cy - 6, cx - 2, cy - 3, C_LIGHT);
    gfx_line(cx + 6, cy - 6, cx + 2, cy - 3, C_LIGHT);
    /* the mats on the back walls */
    for (int side = 0; side < 2; side++) {
        int x = side == 0 ? 0 : SKID_RIGHT + 1;
        int w = side == 0 ? SKID_LEFT : SCREEN_W - SKID_RIGHT - 1;
        gfx_rect(x, SKID_TOP - 10, w, SCREEN_H - SKID_TOP + 10, C_BLUE);
        for (int y = SKID_TOP - 10; y < SCREEN_H; y += 14) gfx_hline(x, x + w - 1, y, C_NAVY);
        gfx_vline(side == 0 ? SKID_LEFT - 1 : SKID_RIGHT + 1, SKID_TOP - 10, SCREEN_H - 1, C_NAVY);
    }
}

/* ---- things on the floor ------------------------------------------------------ */

static void shadow(int x, int y, int w) { gfx_dither(x - w / 2, y - 1, w, 3, C_BROWN, 10); }

static void draw_bag(const Match *m, const Bag *g, int frame) {
    int x = (int)g->x, y = (int)g->y, z = (int)g->z;
    if (g->state == BG_HELD) return; /* drawn in the hand */
    shadow(x, y, 7);
    bool moving = (g->state == BG_SLIDE || g->state == BG_AIR) && !g->dead;
    if (moving) {
        int c = g->team == 0 ? C_RED : g->team == 1 ? C_BLUE : C_WHITE;
        for (int k = 1; k <= 3; k++)
            gfx_dither(x - (int)(g->vx * k * 1.5f) - 2, y - z - 3 - (int)(g->vy * k * 1.5f), 5, 3, c, 12 - k * 3);
    }
    if (g->kind == BK_COMET) {
        int r = 5 + (frame / 2) % 2;
        gfx_dither_circle(x, y - z - 2, r + 2, C_YELLOW, 8);
        gfx_circb(x, y - z - 2, r, C_WHITE);
    }
    if (g->kind == BK_YOYO && g->owner >= 0 && g->phase == 0) {
        const Kid *o = &m->k[g->owner];
        for (int k = 1; k < 8; k++) {
            int px = (int)(o->x + (g->x - o->x) * k / 8), py = (int)(o->y - 8 + (g->y - z - o->y + 8) * k / 8);
            gfx_pset(px, py, C_WHITE);
        }
    }
    int ox = 3, oy = 4;
    spr_draw(&skid_spr[SS_BAG], x - ox, y - z - oy, 0);
    if (g->kind == BK_POPPER || g->kind == BK_HOMING || g->kind == BK_MARBLE) {
        /* a fuse, fizzing */
        int c = (frame / 2) % 2 ? C_YELLOW : C_WHITE;
        gfx_pset(x + 2, y - z - 5, C_INK);
        gfx_pset(x + 3, y - z - 6, c);
        if (g->fuse) gfx_circb(x, y - z - 2, 3 + (20 - g->fuse) / 3, (frame / 2) % 2 ? C_ORANGE : C_YELLOW);
        if (g->kind == BK_MARBLE) gfx_pset(x - 1, y - z - 2, C_ICE);
    }
    if (g->state == BG_REEL) gfx_circb(x, y - z - 2, 4, C_VIOLET);
}

static void draw_item(const Item *it, int frame) {
    int x = (int)it->x, y = (int)it->y, z = (int)it->z;
    switch (it->kind) {
    case IT_JUICE:
        shadow(x, y, 5);
        spr_draw(&skid_spr[SS_JUICE], x - 2, y - z - 6, 0);
        break;
    case IT_BALLOON:
        shadow(x, y, 6);
        spr_draw(&skid_spr[SS_BALLOON], x - 3, y - z - 7, 0);
        break;
    case IT_MARBLE: spr_draw(&skid_spr[SS_MARBLE], x - 1, y - 2, 0); break;
    case IT_PUDDLE: {
        int r = (int)it->r;
        bool fading = it->life < 90 && (frame / 4) % 2;
        for (int yy = -r / 2; yy <= r / 2; yy++) {
            int half = (int)(r * sqrtf(fmaxf(0, 1.0f - (float)(yy * yy * 4) / (float)(r * r))));
            gfx_hline(x - half, x + half, y + yy, fading ? C_ICE : C_SKY);
        }
        gfx_hline(x - r / 2, x - r / 2 + 3, y - r / 4, C_WHITE);
        break;
    }
    default: break;
    }
}

static void draw_wave(const Wave *v, int frame) {
    int x = (int)v->x;
    for (int y = (int)(v->y - v->half); y <= (int)(v->y + v->half); y++) {
        int wob = (int)(sinf((y + frame) * 0.5f) * 1.5f);
        gfx_pset(x + wob, y - 3, C_WHITE);
        gfx_pset(x + wob - v->dir, y - 2, C_ICE);
        gfx_pset(x + wob - v->dir * 2, y - 1, C_CYAN);
        if (y % 3 == 0) gfx_pset(x + wob - v->dir * 4, y - 1, C_SKY);
    }
}

static void draw_sweeper(const Sweeper *s, int frame) {
    int x = (int)s->x;
    gfx_rect(x - 2, SKID_TOP - 6, 5, SKID_BOT - SKID_TOP + 10, C_SLATE);
    gfx_vline(x - 2, SKID_TOP - 6, SKID_BOT + 3, C_GREY);
    for (int y = SKID_TOP - 4; y < SKID_BOT + 4; y += 8) {
        gfx_circ(x + s->dir * 4, y, 2, C_GREY);
        gfx_pset(x + s->dir * 4, y - 1, C_WHITE);
    }
    if ((frame / 4) % 2) gfx_pset(x, SKID_TOP - 5, C_RED);
}

static void draw_kid(const Match *m, int i, int frame, int marker) {
    const Kid *k = &m->k[i];
    int pose = skid_kid_pose(k, frame);
    int x = (int)k->x, y = (int)k->y, z = (int)k->z;
    int flip = k->face < 0;
    shadow(x, y, 12);
    bool blink = k->state == KS_HURT && (frame / 2) % 2;
    const Sprite *s = &skid_kid_spr[k->who][k->team][pose];
    int x0 = x - s->w / 2, y0 = y - s->h + 1 - z;
    if (pose == KP_DOWN) y0 = y - s->h + 2;
    if (blink) spr_draw_ex(s, x0, y0, flip ? SPR_FLIPX : 0, NULL, C_WHITE);
    else if (k->state == KS_CHARGE && k->charge >= skid_charge_full(k))
        spr_draw_outline(s, x0, y0, flip ? SPR_FLIPX : 0, (frame / 3) % 2 ? C_YELLOW : C_WHITE);
    else spr_draw(s, x0, y0, flip ? SPR_FLIPX : 0);
    if (k->state == KS_SAD && k->who < SKID_KIDS) skid_draw_kid_at(k->who, k->team, KP_IDLE0, x, y - z, flip, 1, true);
    /* bags in hand: over the head while winding up, in front otherwise */
    int top = y0;
    for (int n = 0; n < k->held; n++) {
        int bx, by;
        if (k->state == KS_CHARGE) { bx = x - k->face * 5 - 3; by = top - 3 - n * 4; }
        else { bx = x + k->face * 4 - 3 + n * 2 * k->face; by = y - z - 11 + n; }
        bool warn = k->hold_t > SKID_FORCED - 90 && (frame / 4) % 2;
        if (warn) spr_draw_ex(&skid_spr[SS_BAG], bx, by, 0, NULL, C_WHITE);
        else spr_draw(&skid_spr[SS_BAG], bx, by, 0);
    }
    /* a powered-up kid has a star circling it */
    if (k->stars >= 2 && k->state != KS_DOWN) {
        float a = frame * 0.12f + i;
        int sx = x + (int)(cosf(a) * 8), sy = y - z - 10 + (int)(sinf(a) * 3);
        gfx_pset(sx, sy, C_YELLOW);
        gfx_pset(sx + 1, sy, C_YELLOW);
        gfx_pset(sx, sy - 1, C_WHITE);
    }
    /* the wind-up meter, its arrows showing the aim */
    if (k->state == KS_CHARGE) {
        int full = skid_charge_full(k);
        int f = imin(k->charge, full) * 14 / full;
        int mx = x - 8, my = top - 9;
        gfx_rect(mx, my, 16, 4, C_INK);
        int c = k->charge >= full ? ((frame / 3) % 2 ? C_YELLOW : C_WHITE) : C_ORANGE;
        if (k->charge >= full && k->stars >= m->rules.cost * 2) c = (frame / 2) % 2 ? C_MAGENTA : C_PINK;
        gfx_rect(mx + 1, my + 1, f, 2, c);
        int ax = x + k->aimx * 12, ay = my + 2 + k->aimy * 6;
        gfx_pset(ax, ay, C_WHITE);
        gfx_pset(ax - k->aimx, ay - k->aimy, C_WHITE);
        gfx_pset(ax + (k->aimy ? 1 : 0), ay + (k->aimx ? 1 : 0), C_WHITE);
    }
    if (marker) {
        /* the kid a player drives */
        int c = marker == 1 ? C_YELLOW : C_MAGENTA;
        int my = top - (k->state == KS_CHARGE ? 16 : 6);
        gfx_hline(x - 2, x + 2, my, c);
        gfx_hline(x - 1, x + 1, my + 1, c);
        gfx_pset(x, my + 2, c);
    }
}

static void draw_coach(const Match *m, int frame) {
    int id = m->coach_throw_t > 0 ? SS_COACH_THROW : (m->state != MS_PLAY && (frame / 8) % 2) ? SS_COACH_WHISTLE : SS_COACH;
    spr_draw(&skid_spr[id], SKID_COACH_X - 7, SKID_COACH_Y - 21, 0);
    if (m->bark_t > 0 && m->bark > BARK_NONE && m->bark < BARK_COUNT) {
        const char *t = SKID_BARK_TEXT[m->bark];
        int w = tiny_width(t) + 6;
        int bx = SKID_COACH_X + 10, by = SKID_COACH_Y - 26;
        ui_panel(bx, by, w, 9, C_WHITE, C_INK);
        gfx_pset(bx - 1, by + 7, C_INK);
        gfx_pset(bx - 2, by + 8, C_INK);
        tiny_draw(t, bx + 3, by + 2, C_INK);
    }
}

void skid_draw_hud(const Match *m, int frame) {
    gfx_rect(0, 0, SCREEN_W, 14, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 13, C_DUSK);
    for (int t = 0; t < 2; t++) {
        /* the score boxes: red for the left team, blue for the right */
        int bx = t == 0 ? 112 : 172;
        ui_panel(bx, 1, 36, 12, t == 0 ? C_RED : C_BLUE, C_INK);
        char buf[8];
        snprintf(buf, sizeof buf, "%d", m->score[t]);
        text_center(buf, bx + 18, 3, C_WHITE);
        /* each kid's stars, halves showing */
        for (int s = 0; s < 2; s++) {
            const Kid *k = &m->k[t * 2 + s];
            int x0 = t == 0 ? 4 + s * 54 : 214 + s * 52;
            tiny_draw(SKID_KID[k->who].name, x0, 1, k->team == 0 ? C_PINK : C_SKY);
            for (int n = 0; n < 3; n++) {
                int have = k->stars - n * 2;
                int id = have >= 2 ? SS_STAR : have == 1 ? SS_HALFSTAR : SS_NOSTAR;
                spr_draw(&skid_spr[id], x0 + n * 7, 7, 0);
            }
        }
    }
    char goal[16];
    snprintf(goal, sizeof goal, "TO %d", m->rules.goal);
    if (!m->endless) tiny_center(goal, SKID_MID, 4, C_GREY);
    (void)frame;
}

void skid_draw_court(const Match *m, int frame, int shake, int p1_kid, int p2_kid) {
    int sx = 0, sy = 0;
    if (shake > 0 || m->quake_t > 0) {
        sx = (frame % 3) - 1;
        sy = m->quake_t > 0 ? ((frame / 2) % 3) - 1 : 0;
    }
    gfx_cls(C_NIGHT);
    gfx_camera(sx, sy);
    skid_draw_gym(frame, 14, false);
    draw_floor();
    for (int n = 0; n < SKID_MAX_ITEMS; n++)
        if (m->it[n].kind == IT_PUDDLE || m->it[n].kind == IT_MARBLE) draw_item(&m->it[n], frame);
    draw_coach(m, frame);
    /* everything standing on the floor, back to front */
    int order[4 + SKID_MAX_BAGS + SKID_MAX_ITEMS];
    float key[4 + SKID_MAX_BAGS + SKID_MAX_ITEMS];
    int n = 0;
    for (int i = 0; i < 4; i++) { order[n] = i; key[n++] = m->k[i].y; }
    for (int b = 0; b < SKID_MAX_BAGS; b++)
        if (m->bag[b].state != BG_GONE && m->bag[b].state != BG_HELD) { order[n] = 100 + b; key[n++] = m->bag[b].y + 0.5f; }
    for (int t = 0; t < SKID_MAX_ITEMS; t++)
        if (m->it[t].kind == IT_JUICE || m->it[t].kind == IT_BALLOON) { order[n] = 200 + t; key[n++] = m->it[t].y + 0.4f; }
    for (int a = 1; a < n; a++)
        for (int b = a; b > 0 && key[b - 1] > key[b]; b--) {
            float tk = key[b]; key[b] = key[b - 1]; key[b - 1] = tk;
            int to = order[b]; order[b] = order[b - 1]; order[b - 1] = to;
        }
    for (int k = 0; k < n; k++) {
        int o = order[k];
        if (o < 100) draw_kid(m, o, frame, o == p1_kid ? 1 : o == p2_kid ? 2 : 0);
        else if (o < 200) draw_bag(m, &m->bag[o - 100], frame);
        else draw_item(&m->it[o - 200], frame);
    }
    for (int w = 0; w < SKID_MAX_WAVES; w++)
        if (m->wave[w].live) draw_wave(&m->wave[w], frame);
    if (m->sweep.live) draw_sweeper(&m->sweep, frame);
    if (m->gust_t > 0) {
        int dir = m->gust_team == 0 ? 1 : -1;
        for (int k = 0; k < 14; k++) {
            int y = SKID_TOP + (k * 37) % (SKID_BOT - SKID_TOP);
            int x = ((k * 71 + frame * 6 * dir) % SCREEN_W + SCREEN_W) % SCREEN_W;
            gfx_hline(x, x + 10, y, C_WHITE);
        }
    }
    gfx_camera(0, 0);
    skid_draw_hud(m, frame);
}
