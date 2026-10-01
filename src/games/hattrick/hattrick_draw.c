/* HAT TRICK - drawing a screen in play: the four worlds' backdrops and
 * tiles, the kids, the ball, the creatures, the food and the HUD. */
#include "hattrick.h"

uint32_t htk_hud_top; /* the best score so far, for the HUD */

/* ---- wrap-aware drawing: anything near an open edge shows on the other side too ---- */

static void draw_wrapped(const Sprite *s, int x, int y, int flags, const uint8_t *remap, int solid) {
    /* x, y: the top left in field space */
    for (int oy = -HTK_H; oy <= HTK_H; oy += HTK_H)
        for (int ox = -HTK_W; ox <= HTK_W; ox += HTK_W) {
            int dx = x + ox, dy = y + oy;
            if (dx + s->w <= 0 || dx >= HTK_W || dy + s->h <= 0 || dy >= HTK_H) continue;
            spr_draw_ex(s, dx, dy + HTK_TOP, flags, remap, solid);
        }
}

/* a sprite at twice its size (the bosses), with a flash */
static void draw_big(const Sprite *s, int x, int y, int flags, int solid) {
    for (int sy = 0; sy < s->h; sy++)
        for (int sx = 0; sx < s->w; sx++) {
            int srcx = (flags & SPR_FLIPX) ? s->w - 1 - sx : sx;
            uint8_t c = s->px[sy * s->w + srcx];
            if (c == TRANSPARENT) continue;
            gfx_rect(x + sx * 2, y + sy * 2, 2, 2, solid >= 0 ? solid : c);
        }
}

/* ---- the worlds -------------------------------------------------------------------- */

void htk_draw_backdrop(int world, int t) {
    switch (world) {
    case 0: /* Sandy Court: sky, sun, sea */
        gfx_rect(0, HTK_TOP, 320, 70, C_SKY);
        gfx_dither(0, HTK_TOP + 50, 320, 20, C_CYAN, 8);
        gfx_circ(250, HTK_TOP + 30, 14, C_YELLOW);
        gfx_circb(250, HTK_TOP + 30, 16, C_CREAM);
        gfx_rect(0, HTK_TOP + 70, 320, 40, C_BLUE);
        for (int i = 0; i < 12; i++) {
            int wx = (i * 37 + t / 3) % 340 - 10;
            gfx_hline(wx, wx + 8, HTK_TOP + 76 + (i % 4) * 9, C_CYAN);
        }
        gfx_rect(0, HTK_TOP + 110, 320, 58, C_CREAM);
        gfx_dither(0, HTK_TOP + 110, 320, 58, C_YELLOW, 4);
        break;
    case 1: /* Lucky Lanes: a dim alley, pins at the far end, neon */
        gfx_rect(0, HTK_TOP, 320, HTK_H, C_NIGHT);
        gfx_dither(0, HTK_TOP, 320, 60, C_PURPLE, 3);
        for (int i = 0; i < 8; i++) {
            int x = 20 + i * 40;
            gfx_rect(x - 1, HTK_TOP + 40, 3, 128, C_DUSK);
            for (int k = 0; k < 3; k++) gfx_pset(x - 4 + k * 4, HTK_TOP + 36, C_LIGHT);
        }
        for (int i = 0; i < 20; i++) gfx_pset((i * 53) % 320, HTK_TOP + 12 + (i * 7) % 20, (t / 20 + i) % 3 ? C_MAGENTA : C_PINK);
        gfx_hline(0, 319, HTK_TOP + 30, C_MAGENTA);
        break;
    case 2: /* the Lido: tiled pool walls, water */
        gfx_rect(0, HTK_TOP, 320, HTK_H, C_CYAN);
        for (int y = 0; y < HTK_H; y += 12) gfx_hline(0, 319, HTK_TOP + y, C_ICE);
        for (int x = 0; x < 320; x += 12) gfx_vline(x, HTK_TOP, HTK_TOP + HTK_H - 1, C_ICE);
        gfx_rect(0, HTK_TOP + 96, 320, 72, C_SKY);
        for (int i = 0; i < 18; i++) {
            int wx = (i * 41 + t / 2) % 340 - 10, wy = HTK_TOP + 100 + (i * 13) % 64;
            gfx_hline(wx, wx + 6, wy, C_CYAN);
        }
        break;
    default: /* Big Stadium: night, floodlights, the crowd */
        gfx_rect(0, HTK_TOP, 320, HTK_H, C_NAVY);
        for (int i = 0; i < 2; i++) {
            int x = i ? 280 : 40;
            gfx_dither(x - 30, HTK_TOP, 60, 90, C_BLUE, 5);
            gfx_rect(x - 8, HTK_TOP + 4, 16, 6, C_YELLOW);
        }
        for (int y = 0; y < 3; y++)
            for (int x = 0; x < 320; x += 4) gfx_pset(x + (y & 1) * 2, HTK_TOP + 60 + y * 4, (x * 7 + y + t / 30) % 5 ? C_DUSK : C_RED);
        gfx_rect(0, HTK_TOP + 76, 320, 92, C_FOREST);
        for (int x = 0; x < 320; x += 32) gfx_dither(x, HTK_TOP + 76, 16, 92, C_JADE, 4);
        break;
    }
}

static void draw_tile(int world, int t, int left, int up, int x, int y) {
    /* t: the tile; left/up: is the same tile there (for edges) */
    y += HTK_TOP;
    if (t == T_GOAL) {
        gfx_dither(x, y, 8, 8, C_WHITE, 6);
        return;
    }
    switch (world) {
    case 0:
        if (t == T_SOLID) {
            gfx_rect(x, y, 8, 8, C_AMBER);
            gfx_pset(x + 2, y + 3, C_EARTH);
            gfx_pset(x + 6, y + 6, C_EARTH);
            gfx_pset(x + 5, y + 1, C_YELLOW);
            if (!up) gfx_hline(x, x + 7, y, C_YELLOW);
        } else {
            gfx_rect(x, y, 8, 4, C_TAN);
            gfx_hline(x, x + 7, y, C_EARTH);
            gfx_hline(x, x + 7, y + 4, C_BROWN);
            if (!left) gfx_vline(x, y, y + 4, C_BROWN);
            gfx_pset(x + 4, y + 2, C_BROWN);
        }
        break;
    case 1:
        if (t == T_SOLID) {
            gfx_rect(x, y, 8, 8, C_ORANGE);
            gfx_vline(x + 3, y, y + 7, C_AMBER);
            gfx_vline(x + 7, y, y + 7, C_TAN);
            if (!up) gfx_hline(x, x + 7, y, C_YELLOW);
        } else {
            gfx_rect(x, y, 8, 4, C_GREY);
            gfx_hline(x, x + 7, y, C_LIGHT);
            gfx_hline(x, x + 7, y + 3, C_SLATE);
        }
        break;
    case 2:
        if (t == T_SOLID) {
            gfx_rect(x, y, 8, 8, C_WHITE);
            gfx_hline(x, x + 7, y + 7, C_LIGHT);
            gfx_vline(x + 7, y, y + 7, C_LIGHT);
            if (!up) gfx_hline(x, x + 7, y, C_BLUE);
        } else {
            gfx_hline(x, x + 7, y + 2, C_LIGHT);
            gfx_rect(x + ((x / 8) & 1) * 4, y, 4, 5, ((x / 16) & 1) ? C_RED : C_WHITE);
            gfx_rectb(x + ((x / 8) & 1) * 4, y, 4, 5, C_INK);
        }
        break;
    default:
        if (t == T_SOLID) {
            gfx_rect(x, y, 8, 8, C_GREY);
            gfx_pset(x + 1, y + 1, C_LIGHT);
            gfx_hline(x, x + 7, y + 7, C_SLATE);
            if (!up) {
                gfx_hline(x, x + 7, y, C_LEAF);
                gfx_hline(x, x + 7, y + 1, C_JADE);
            }
        } else {
            gfx_rect(x, y, 8, 4, C_BLUE);
            gfx_hline(x, x + 7, y, C_SKY);
            gfx_vline(x + 1, y + 4, y + 6, C_SLATE);
            gfx_vline(x + 6, y + 4, y + 6, C_SLATE);
        }
        break;
    }
}

/* the lane ropes over the Lido boss: our own message in Morse */
static const char *const MORSE[26] = {".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", "-.-",
                                      ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", "..-", "...-",
                                      ".--", "-..-", "-.--", "--.."};

/* lays the floats out along the top: out[i] = 3 (a short float) or 8 (a long
 * one), gap[i] = the space before it. Returns how many. */
int htk_morse_layout(int *len, int *gap, int max) {
    int n = 0, pending = 0;
    for (const char *c = HTK_MORSE_MSG; *c && n < max; c++) {
        if (*c == ' ') { pending = 12; continue; }
        const char *code = MORSE[*c - 'A'];
        for (int k = 0; code[k] && n < max; k++) {
            len[n] = code[k] == '.' ? 3 : 8;
            gap[n] = n == 0 ? 0 : k == 0 ? (pending ? pending : 6) : 2;
            pending = 0;
            n++;
        }
    }
    return n;
}

static void draw_morse(void) {
    int len[64], gap[64], n = htk_morse_layout(len, gap, 64), total = 0;
    for (int i = 0; i < n; i++) total += gap[i] + len[i];
    int x = 160 - total / 2, y = HTK_TOP + 3;
    gfx_hline(4, 315, y + 1, C_SLATE);
    for (int i = 0; i < n; i++) {
        x += gap[i];
        gfx_rect(x, y, len[i], 3, len[i] > 3 ? C_RED : C_YELLOW);
        gfx_rectb(x, y, len[i], 3, C_INK);
        x += len[i];
    }
}

/* ---- the things on the field --------------------------------------------------------- */

void htk_draw_kid(int ch, int x, int y, int pose, int facing, int t) {
    uint8_t map[PAL_COUNT];
    htk_kid_remap(ch, map);
    spr_draw_ex(&htk_spr[SP_KID_STAND + pose], x - 7, y - 20, facing < 0 ? SPR_FLIPX : 0, map, -1);
    (void)t;
}

static int kid_pose(const HtkPlayer *p) {
    if (!p->alive) return SP_KID_DOWN - SP_KID_STAND;
    if (p->slide_t > 0) return SP_KID_SLIDE - SP_KID_STAND;
    if (p->crouch) return SP_KID_CROUCH - SP_KID_STAND;
    if (p->head_t > 0) return SP_KID_HEAD - SP_KID_STAND;
    if (p->kick_t > 0) return SP_KID_KICK - SP_KID_STAND;
    if (!p->ground) return SP_KID_JUMP - SP_KID_STAND;
    if (fabsf(p->vx) > 0.3f) return (p->anim / 8) % 2 ? SP_KID_RUN1 - SP_KID_STAND : SP_KID_RUN2 - SP_KID_STAND;
    return 0;
}

void htk_draw_foe_sprite(int kind, int x, int y, int flip, int t, bool dead) {
    int sp = kind <= FK_BULLDOG ? SP_SPIKER + kind : SP_TIMEKEEPER;
    spr_draw(&htk_spr[sp], x - 8, y - 8, (flip ? SPR_FLIPX : 0) | (dead ? SPR_FLIPY : 0));
    (void)t;
}

void htk_draw_item(int item, int x, int y) { spr_draw(&htk_spr[SP_POPCORN + item], x - 4, y - 4, 0); }

static void draw_chair(int x, int y, int solid) {
    /* the lifeguard's chair: tall legs, a seat, a red and white sunshade */
    int c1 = solid >= 0 ? solid : C_WHITE, c2 = solid >= 0 ? solid : C_RED, c3 = solid >= 0 ? solid : C_TAN;
    gfx_rect(x - 8, y - 2, 2, 16, c3);
    gfx_rect(x + 6, y - 2, 2, 16, c3);
    gfx_hline(x - 8, x + 7, y + 6, c3);
    gfx_rect(x - 7, y - 6, 14, 4, c1);
    gfx_rectb(x - 7, y - 6, 14, 4, C_INK);
    gfx_rect(x - 9, y - 14, 18, 3, c2);
    gfx_rect(x - 3, y - 14, 6, 3, c1);
    gfx_vline(x, y - 11, y - 6, C_INK);
    /* its eyes, under the shade */
    gfx_pset(x - 3, y - 4, C_INK);
    gfx_pset(x + 2, y - 4, C_INK);
}

static void draw_foe(const HtkPlay *g, const HtkFoe *f) {
    int x = (int)lroundf(f->x), y = (int)lroundf(f->y);
    bool flash = f->flash > 0 && (f->flash / 3) % 2;
    int flip = f->dir < 0;
    if (f->kind >= FK_KINGSPIKER && f->kind != FK_TIMEKEEPER) {
        if (f->away) return;
        int sx = x - 16, sy = HTK_TOP + y - 16;
        switch (f->kind) {
        case FK_KINGSPIKER:
            draw_big(&htk_spr[SP_SPIKER], sx, sy - 4, flip ? SPR_FLIPX : 0, flash ? C_WHITE : -1);
            /* a crown */
            gfx_rect(x - 6, sy - 8, 12, 4, C_YELLOW);
            for (int k = 0; k < 3; k++) gfx_rect(x - 6 + k * 5, sy - 11, 2, 3, C_YELLOW);
            break;
        case FK_KINGPIN:
            draw_big(&htk_spr[SP_PIN], sx, sy, flip ? SPR_FLIPX : 0, flash ? C_WHITE : -1);
            gfx_rect(x - 4, sy - 2, 8, 3, C_YELLOW);
            for (int k = 0; k < 3; k++) gfx_pset(x - 4 + k * 3, sy - 3, C_YELLOW);
            break;
        case FK_LIFEGUARD:
            draw_big(&htk_spr[SP_POLO], sx, sy - 2, flip ? SPR_FLIPX : 0, flash ? C_WHITE : -1);
            gfx_rect(x + (flip ? -10 : 8), sy + 14, 3, 2, C_YELLOW); /* the whistle */
            break;
        case FK_TOWER: draw_chair(x, HTK_TOP + y, flash ? C_WHITE : -1); break;
        default:
            draw_big(&htk_spr[SP_PROP], sx, sy, flip ? SPR_FLIPX : 0, flash ? C_WHITE : -1);
            gfx_rect(x - 14, sy + 18, 4, 4, C_YELLOW); /* the armband */
            text_draw("C", x - 14, sy + 18, C_INK);
            break;
        }
        return;
    }
    int sp = f->kind <= FK_BULLDOG ? SP_SPIKER + f->kind : SP_TIMEKEEPER;
    if (f->kind == FK_TIMEKEEPER && (g->frame / 2) % 3 == 0) return; /* he flickers */
    bool winding = f->state == 1 && (f->kind == FK_SPIKER || f->kind == FK_POLO || f->kind == FK_BOWLER || f->kind == FK_PROP);
    draw_wrapped(&htk_spr[sp], x - 8, y - 8, flip ? SPR_FLIPX : 0, NULL, winding && (f->t / 3) % 2 ? C_WHITE : -1);
    if (f->kind == FK_BULLDOG && !f->state) {
        int z = (f->t / 30) % 3;
        tiny_draw("Z", x + 6 + z * 2, HTK_TOP + y - 12 - z * 3, C_WHITE);
    }
}

static void draw_shot(const HtkShot *s) {
    int x = (int)lroundf(s->x), y = (int)lroundf(s->y) + HTK_TOP;
    switch (s->kind) {
    case SH_SERVE: gfx_circ(x, y, 2, C_YELLOW); gfx_circb(x, y, 2, C_INK); break;
    case SH_BOWL: gfx_circ(x, y, 3, C_DUSK); gfx_pset(x - 1, y - 1, C_GREY); gfx_circb(x, y, 3, C_INK); break;
    case SH_SQUIRT: gfx_circ(x, y, 2, C_ICE); gfx_pset(x, y, C_CYAN); break;
    case SH_RUGBY:
        gfx_rect(x - 3, y - 2, 7, 4, C_BROWN);
        gfx_rectb(x - 3, y - 2, 7, 4, C_INK);
        gfx_vline(x, y - 1, y + 1, C_WHITE);
        break;
    case SH_SPADE: gfx_rect(x - 2, y - 2, 5, 4, C_LIGHT); gfx_vline(x, y + 2, y + 4, C_BROWN); break;
    case SH_CAN: gfx_rect(x - 2, y - 3, 4, 6, C_RED); gfx_hline(x - 2, x + 1, y - 3, C_LIGHT); break;
    default: gfx_circb(x, y, 3, C_ORANGE); gfx_circb(x, y, 2, C_WHITE); break;
    }
}

static void draw_ball(const HtkPlay *g) {
    const HtkBall *b = &g->ball;
    int x = (int)lroundf(b->x), y = (int)lroundf(b->y);
    const Sprite *s = &htk_spr[b->lit ? SP_BALL : SP_BALL_DARK];
    int flags = (b->spin / 6) % 2 ? SPR_FLIPX : 0;
    if (b->power_t > 0) {
        /* a driven shot trails fire */
        for (int k = 1; k <= 3; k++)
            gfx_dither_circle(x - (int)(b->vx * k * 1.5f), HTK_TOP + y - (int)(b->vy * k * 1.5f), 4 - k, C_ORANGE, 10 - k * 2);
    }
    int charging = b->carrier >= 0 ? g->pl[b->carrier].charge : 0;
    if (charging >= HTK_CHARGE_T && (g->frame / 3) % 2) draw_wrapped(s, x - 3, y - 3, flags, NULL, C_YELLOW);
    else draw_wrapped(s, x - 3, y - 3, flags, NULL, -1);
}

static void draw_hud(const HtkPlay *g) {
    char buf[48];
    gfx_rect(0, 0, 320, HTK_TOP, C_INK);
    gfx_hline(0, 319, HTK_TOP - 1, C_DUSK);
    for (int k = 0; k < 2; k++) {
        const HtkPlayer *p = &g->pl[k];
        if (!p->on) continue;
        bool right = g->mode == MODE_1P ? false : k == 1;
        int x = right ? 200 : 4;
        snprintf(buf, sizeof buf, "%s %06u", g->mode == MODE_VS ? (k ? "2P" : "1P") : (k ? "2P" : "1P"), (unsigned)p->score);
        if (g->mode == MODE_VS) snprintf(buf, sizeof buf, "%s %s", HTK_KID_NAME[p->ch], k ? "2P" : "1P");
        text_draw(buf, x, 2, p->out ? C_SLATE : (p->ch == KID_MAE ? C_YELLOW : C_LEAF));
        if (g->mode != MODE_VS)
            for (int l = 0; l < imin(p->spare, 6); l++) gfx_circ(x + 74 + l * 6, 5, 2, C_RED);
    }
    if (g->mode == MODE_1P) {
        snprintf(buf, sizeof buf, "TOP %06u", (unsigned)(htk_hud_top > g->pl[0].score ? htk_hud_top : g->pl[0].score));
        text_draw(buf, 238, 2, C_GREY);
    }
    if (g->mode == MODE_VS) {
        snprintf(buf, sizeof buf, "%d - %d", g->vs_score[0], g->vs_score[1]);
        text_center(buf, 160, 2, C_WHITE);
        return;
    }
    snprintf(buf, sizeof buf, "%d-%d", g->level / HTK_PER_WORLD + 1, g->level % HTK_PER_WORLD + 1);
    text_draw(buf, 130, 2, C_GREY);
    int c = htk_counts_left(g);
    snprintf(buf, sizeof buf, "%02d", c);
    int col = g->overtime ? C_RED : c <= 5 && (g->frame / 15) % 2 ? C_ORANGE : C_WHITE;
    text_draw(buf, 160, 2, col);
}

void htk_draw_play(const HtkPlay *g) {
    int world = g->level < 0 ? 3 : g->level / HTK_PER_WORLD;
    gfx_cls(C_INK);
    htk_draw_backdrop(world, g->frame);
    for (int r = 0; r < HTK_ROWS; r++)
        for (int c = 0; c < HTK_COLS; c++) {
            int t = g->tile[r][c];
            if (t == T_EMPTY) continue;
            int left = c > 0 && g->tile[r][c - 1] == t, up = r > 0 && g->tile[r - 1][c] == t;
            draw_tile(world, t, left, up, c * HTK_T, r * HTK_T);
        }
    if (g->level == 2 * HTK_PER_WORLD + 9) draw_morse();
    gfx_clip(0, HTK_TOP, HTK_W, HTK_H);
    /* bodies, food and desserts */
    for (int i = 0; i < HTK_MAX_ITEMS; i++) {
        const HtkItem *it = &g->item[i];
        if (!it->alive) continue;
        int x = (int)lroundf(it->x), y = (int)lroundf(it->y);
        if (it->falling && it->body >= 0) {
            int sp = it->body <= FK_BULLDOG ? SP_SPIKER + it->body : SP_TIMEKEEPER;
            draw_wrapped(&htk_spr[sp], x - 8, y - 8, SPR_FLIPY, NULL, -1);
        } else {
            draw_wrapped(&htk_spr[SP_POPCORN + it->item], x - 4, y - 4 - (it->falling ? 0 : (it->t / 12) % 2), 0, NULL, -1);
        }
    }
    for (int i = 0; i < HTK_MAX_FOES; i++)
        if (g->foe[i].alive) draw_foe(g, &g->foe[i]);
    for (int k = 0; k < 2; k++) {
        const HtkPlayer *p = &g->pl[k];
        if (!p->on || p->out) continue;
        if (p->alive && p->inv > 0 && (p->inv / 3) % 2) continue;
        if (!p->alive && p->dead_t > 60 && (p->dead_t / 3) % 2) continue;
        int pose = kid_pose(p), x = (int)lroundf(p->x), y = (int)lroundf(p->y);
        uint8_t map[PAL_COUNT];
        htk_kid_remap(p->ch, map);
        if (p->lifted && p->alive) {
            /* carried off the top: no copy wrapping round at the bottom */
            spr_draw_ex(&htk_spr[SP_KID_STAND + pose], x - 7, HTK_TOP + y - 20, p->facing < 0 ? SPR_FLIPX : 0, map, -1);
        } else draw_wrapped(&htk_spr[SP_KID_STAND + pose], x - 7, y - 20, p->facing < 0 ? SPR_FLIPX : 0, map, -1);
        if (p->lifted && p->alive) {
            for (int b = 0; b < 3; b++) {
                int bx = x - 9 + b * 6, by = HTK_TOP + y - 38 - (b == 1) * 3;
                spr_draw(&htk_spr[SP_BALLOON], bx - 3, by, 0);
                gfx_line(bx, by + 9, x, HTK_TOP + y - 18, C_LIGHT);
            }
        }
    }
    draw_ball(g);
    for (int i = 0; i < HTK_MAX_SHOTS; i++)
        if (g->shot[i].alive) draw_shot(&g->shot[i]);
    for (int i = 0; i < HTK_MAX_PARTS; i++) {
        const HtkPart *p = &g->part[i];
        if (p->life > 0) gfx_pset((int)p->x, HTK_TOP + (int)p->y, p->col);
    }
    for (int i = 0; i < HTK_MAX_POPS; i++) {
        const HtkPop *p = &g->pop[i];
        if (p->t <= 0) continue;
        char buf[12];
        snprintf(buf, sizeof buf, "%d", p->value);
        tiny_center(buf, (int)p->x, HTK_TOP + (int)p->y, p->value >= 1000 ? C_YELLOW : C_WHITE);
    }
    gfx_noclip();
    draw_hud(g);
    /* the banners */
    char buf[64];
    if (g->sub == LS_INTRO && g->level >= 0) {
        ui_panel(90, 70, 140, 34, C_INK, C_WHITE);
        snprintf(buf, sizeof buf, "%d-%d %s", g->level / HTK_PER_WORLD + 1, g->level % HTK_PER_WORLD + 1, HTK_LEVEL[g->level].name);
        text_center(buf, 160, 76, C_YELLOW);
        text_center(g->sub_t > 45 ? "PLAY!" : "READY", 160, 90, C_WHITE);
    }
    if (g->extra_t > 0 && (g->extra_t / 8) % 2) text_center_shadow("EXTRA TIME!", 160, 80, C_RED, C_INK);
    if (g->sub == LS_CLEAR || g->sub == LS_EXIT) {
        if (g->bonus) snprintf(buf, sizeof buf, "TIME BONUS %d", g->bonus);
        else snprintf(buf, sizeof buf, "NO TIME BONUS");
        text_center_shadow(buf, 160, 74, C_WHITE, C_INK);
    }
    if (g->mode == MODE_VS && g->sub == LS_GOAL) {
        snprintf(buf, sizeof buf, "GOAL! %s", HTK_KID_NAME[g->pl[g->vs_scorer].ch]);
        text_center_shadow(buf, 160, 74, C_YELLOW, C_INK);
    }
}

/* ---- the cartridge's label ------------------------------------------------------------ */

void htk_draw_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_SKY);
    gfx_rect(x, y + h / 2, w, h - h / 2, C_JADE);
    for (int i = 0; i < w; i += 8) gfx_rect(x + i, y + h / 2, 4, h - h / 2, C_LEAF);
    gfx_hline(x, x + w - 1, y + h / 2, C_WHITE);
    gfx_circ(x + w - 8, y + 6, 4, C_YELLOW);
    int bx = x + w / 2 + (int)(sinf(t * 0.08f) * (w / 4)), by = y + h / 2 - 4 - (int)fabsf(sinf(t * 0.12f) * 10);
    spr_draw(&htk_spr[SP_BALL], bx - 3, by - 3, 0);
    uint8_t map[PAL_COUNT];
    htk_kid_remap(KID_TEDDY, map);
    spr_draw_ex(&htk_spr[SP_KID_KICK], x + 2, y + h - 20, 0, map, -1);
    htk_kid_remap(KID_MAE, map);
    spr_draw_ex(&htk_spr[SP_KID_JUMP], x + w - 16, y + h - 22, SPR_FLIPX, map, -1);
}
