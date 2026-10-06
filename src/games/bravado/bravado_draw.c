/* BRAVADO - drawing: the Glass Pit and everything in it, the HUD, the shop
 * (the gear, the next fight and the three buttons), the title, the story,
 * the ending, the credits and the library label. */
#include "bravado.h"

static const char *const STORY[] = {
    "DICE BET HER SHIP ON A PAIR OF SEVENS",
    "AND LOST IT TO THE HOUSE ON TOMBOLA.",
    "",
    "BROKE AND STRANDED, SHE SIGNS UP FOR",
    "THE GLASS PIT: EIGHT FIGHTS, ONE PURSE.",
    "",
    "THE BIGGER THE BET, THE MORE MONSTERS",
    "THEY LET THROUGH THE GATES.",
};

static const char *const CREDITS[] = {
    "BRAVADO",
    "",
    "A UFO 40 CARTRIDGE",
    "BEAMDOWN SOFTWORKS, 1987",
    "",
    "DICE ........ THE FOX WHO BET HER SHIP",
    "DOMINO ...... HER BROTHER, PLAYER TWO",
    "THE PIT BOSS ........... THE HOUSE",
    "",
    "MITES  GASBAGS  BRUTES",
    "POWDER KEGS  STILTERS  PEEPERS",
    "AND A LOT OF SLAG",
    "",
    "A TRIBUTE TO OVERBOLD",
    "(UFO 50 #34, MOSSMOUTH)",
    "",
    "THE HOUSE ALWAYS WINS.",
    "EXCEPT TONIGHT.",
};

static void fmt_cash(char *buf, int n, int v) {
    if (v >= 1000) snprintf(buf, (size_t)n, "%d,%03d", v / 1000, v % 1000);
    else snprintf(buf, (size_t)n, "%d", v);
}

/* ---- the arena ----------------------------------------------------------- */

static void draw_floor(int t) {
    gfx_rect(0, 0, SCREEN_W, SCREEN_H, C_INK);
    gfx_rect(BRV_AX0, BRV_AY0, BRV_AX1 - BRV_AX0, BRV_AY1 - BRV_AY0, C_NIGHT);
    /* glass tiles */
    for (int x = BRV_AX0; x < BRV_AX1; x += 16) gfx_vline(x, BRV_AY0, BRV_AY1 - 1, C_DUSK);
    for (int y = BRV_AY0; y < BRV_AY1; y += 16) gfx_hline(BRV_AX0, BRV_AX1 - 1, y, C_DUSK);
    for (int x = BRV_AX0 + 8; x < BRV_AX1; x += 32)
        for (int y = BRV_AY0 + 8; y < BRV_AY1; y += 32) gfx_pset(x, y, C_SLATE);
    /* the rim, lit */
    gfx_rectb(BRV_AX0 - 2, BRV_AY0 - 2, BRV_AX1 - BRV_AX0 + 4, BRV_AY1 - BRV_AY0 + 4, C_SLATE);
    gfx_rectb(BRV_AX0 - 1, BRV_AY0 - 1, BRV_AX1 - BRV_AX0 + 2, BRV_AY1 - BRV_AY0 + 2, (t / 20) % 2 ? C_PURPLE : C_VIOLET);
    for (int x = BRV_AX0; x < BRV_AX1; x += 12) {
        int c = ((x / 12 + t / 8) % 5) == 0 ? C_YELLOW : C_AMBER;
        gfx_pset(x, BRV_AY0 - 2, c);
        gfx_pset(x, BRV_AY1 + 1, c);
    }
    /* the four pads */
    for (int i = 0; i < 4; i++) {
        int x = (i & 1) ? BRV_AX1 - 16 : BRV_AX0 + 16, y = (i & 2) ? BRV_AY1 - 14 : BRV_AY0 + 14;
        gfx_circ(x, y, BRV_PAD_R, C_NAVY);
        gfx_circb(x, y, BRV_PAD_R, C_BLUE);
        gfx_circb(x, y, 2 + (t / 4 + i * 2) % 7, C_CYAN);
        gfx_pset(x, y, C_ICE);
    }
}

static void draw_pools(int t) {
    for (int i = 0; i < BRV_MAX_POOLS; i++) {
        const Pool *p = &bv.pool[i];
        if (!p->alive) continue;
        int rx = (int)p->rx, ry = (int)p->ry;
        for (int dy = -ry; dy <= ry; dy++) {
            float f = 1.0f - (float)(dy * dy) / (float)(ry * ry);
            int w = (int)((float)rx * sqrtf(fmaxf(f, 0)));
            int y = (int)p->y + dy;
            if (y < BRV_AY0 || y >= BRV_AY1) continue;
            int x0 = imax(BRV_AX0, (int)p->x - w), x1 = imin(BRV_AX1 - 1, (int)p->x + w);
            if (x1 < x0) continue;
            if (p->cooled) {
                gfx_hline(x0, x1, y, (dy + i) % 3 ? C_SLATE : C_GREY);
            } else {
                gfx_hline(x0, x1, y, iabs(dy) >= ry - 1 ? C_MAROON : C_RED);
                if (iabs(dy) < ry / 2) gfx_hline(x0 + w / 3, x1 - w / 3, y, C_ORANGE);
            }
        }
        if (!p->cooled)
            for (int k = 0; k < 3; k++) {
                int bx = (int)p->x - rx / 2 + ((k * 11 + t / 6) % imax(1, rx));
                int by = (int)p->y + ((k * 5 + t / 9) % imax(1, ry)) - ry / 2;
                gfx_pset(bx, by, (t / 5 + k) % 2 ? C_YELLOW : C_AMBER);
            }
    }
}

static void draw_foe(const Foe *f, int t) {
    int x = (int)lroundf(f->x), y = (int)lroundf(f->y);
    bool fl = f->flash > 0;
    int solid = fl ? C_WHITE : -1;
    if (f->warp > 0 && f->kind != MK_BOSS) {
        /* materialising on a pad */
        gfx_circb(x, y, 2 + f->warp / 4, C_CYAN);
        if (f->warp % 4 < 2) return;
    }
    const Sprite *s = NULL;
    int flip = f->vx < 0 ? SPR_FLIPX : 0;
    switch (f->kind) {
    case MK_MITE: s = &brv_spr[(f->t / 6) % 2 ? SP_MITE1 : SP_MITE2]; break;
    case MK_GASBAG: s = &brv_spr[SP_GAS0 + f->size]; break;
    case MK_BRUTE: s = &brv_spr[(f->t / 12) % 2 ? SP_BRUTE1 : SP_BRUTE2]; break;
    case MK_KEG:
        s = &brv_spr[SP_KEG];
        if (f->state == 1 && (f->st / 2) % 2) solid = C_RED;
        break;
    case MK_STILTER: s = &brv_spr[(f->t / 10) % 2 ? SP_STILT1 : SP_STILT2]; break;
    case MK_PEEPER:
        if (f->state == 0) {
            /* the mark it comes up under */
            int r = 3 + (f->st / 3) % 5;
            gfx_circb(x, y + 2, r, (t / 3) % 2 ? C_LIME : C_LEAF);
            gfx_pset(x, y + 2, C_LIME);
            return;
        }
        if (f->state == 4) return;
        s = &brv_spr[f->state == 2 ? SP_PEEP : SP_PEEP_BUD];
        if (f->state == 2 && f->fire_t > 0 && f->fire_t < 24 && (f->fire_t / 3) % 2) solid = C_RED;
        flip = 0;
        if (f->state == 2 && f->fire_t > 0 && f->fire_t <= BRV_PEEP_WINDUP && solid < 0) {
            /* the wind-up: the eye reddens and swells until it fires */
            spr_draw(s, x - s->w / 2, y - s->h / 2, 0);
            int r = 1 + (BRV_PEEP_WINDUP - f->fire_t) / 60;
            int beat = f->fire_t > 100 ? 16 : f->fire_t > 50 ? 8 : 4;
            gfx_circ(x, y - 1, r, (f->fire_t / beat) % 2 ? C_RED : C_MAROON);
            return;
        }
        break;
    case MK_FIZZER: s = &brv_spr[SP_FIZZ]; flip = 0; break;
    case MK_BOSS: {
        /* the Pit Boss: a brass-bellied dealer machine in a visor */
        int body = fl ? C_WHITE : C_MAROON, trim = fl ? C_WHITE : C_RED;
        gfx_circ(x, y + 2, 13, C_INK);
        gfx_circ(x, y + 2, 12, body);
        gfx_circ(x, y + 4, 8, fl ? C_WHITE : C_WINE);
        gfx_rect(x - 9, y - 5, 18, 4, fl ? C_WHITE : C_FOREST);
        gfx_hline(x - 8, x + 8, y - 4, fl ? C_WHITE : C_LEAF);
        int eye = f->state == 3 && f->st > 50 ? C_RED : C_YELLOW;
        gfx_rect(x - 5, y - 4, 2, 2, eye);
        gfx_rect(x + 3, y - 4, 2, 2, eye);
        gfx_rect(x - 7, y - 15, 14, 8, C_INK);
        gfx_rect(x - 6, y - 14, 12, 6, fl ? C_WHITE : C_NIGHT);
        gfx_rect(x - 10, y - 8, 20, 2, C_INK);
        gfx_hline(x - 6, x + 5, y - 9, trim);
        for (int s2 = -1; s2 <= 1; s2 += 2) {
            int ax = x + s2 * 15, ay = y + 2 + (int)(sinf((float)t * 0.1f + (float)s2) * 2);
            gfx_circ(ax, ay, 4, C_INK);
            gfx_circ(ax, ay, 3, trim);
            gfx_pset(ax, ay, C_YELLOW);
        }
        /* chips stacked on its belly */
        for (int k = 0; k < 3; k++) gfx_hline(x - 3, x + 3, y + 6 + k * 2, k % 2 ? C_WHITE : C_RED);
        return;
    }
    default: return;
    }
    if (!s) return;
    int dx = x - s->w / 2, dy = y - s->h / 2 - (f->kind == MK_STILTER ? 3 : 0);
    spr_draw_ex(s, dx, dy, flip, NULL, solid);
}

static void draw_player(const Player *p, int who, int t) {
    if (!p->on || p->down) return;
    if (p->inv > 0 && p->dash_t == 0 && (t / 3) % 2) return;
    int face = p->dash_t > 0 ? p->dash_face : p->face;
    bool step = p->moving && (p->step / 8) % 2;
    int spr, flip = 0;
    if (face == 2) spr = step ? SP_DICE_D2 : SP_DICE_D;
    else if (face == 6) spr = step ? SP_DICE_U2 : SP_DICE_U;
    else {
        spr = step ? SP_DICE_S2 : SP_DICE_S;
        if (face >= 3 && face <= 5) flip = SPR_FLIPX;
    }
    uint8_t map[PAL_COUNT];
    pal_identity(map);
    if (who == 1) {
        /* Domino: grey fur, a blue jacket */
        pal_swap(map, C_ORANGE, C_LIGHT);
        pal_swap(map, C_RED, C_BLUE);
    }
    int solid = p->hurt_t > 0 && (t / 2) % 2 ? C_RED : -1;
    int x = (int)lroundf(p->x) - 5, y = (int)lroundf(p->y) - 7;
    if (p->dash_t > 0) {
        for (int k = 1; k <= 3; k++)
            gfx_dither_circle((int)p->x - BRV_DX[face] * k * 5, (int)p->y - BRV_DY[face] * k * 5, 4, C_CYAN, 10 - k * 3);
    }
    spr_draw_ex(&brv_spr[spr], x, y, flip, map, solid);
    /* the aim: a sight ahead of the gun, brighter while it's held */
    int ax = (int)p->x + BRV_DX[face] * 9, ay = (int)p->y - 1 + BRV_DY[face] * 9;
    gfx_pset(ax, ay, p->locked ? C_YELLOW : C_AMBER);
    if (p->locked) {
        gfx_pset(ax + BRV_DX[face], ay + BRV_DY[face], C_WHITE);
    }
    if (p->blast_sh + p->shot_sh > 0 && (t / 10) % 4 == 0) gfx_circb((int)p->x, (int)p->y, 9, C_SKY);
}

static void draw_world(int t) {
    int sx = 0, sy = 0;
    if (bv.shake > 0) { sx = (t % 3) - 1; sy = ((t / 2) % 3) - 1; }
    gfx_camera(sx, sy);
    draw_floor(t);
    draw_pools(t);
    gfx_clip(BRV_AX0, BRV_AY0, BRV_AX1 - BRV_AX0, BRV_AY1 - BRV_AY0);
    for (int i = 0; i < BRV_MAX_MEDS; i++) {
        const Medkit *m = &bv.med[i];
        if (m->alive) spr_draw(&brv_spr[SP_MEDKIT], (int)m->x - 4, (int)m->y - 4 + ((m->t / 15) % 2), 0);
    }
    for (int i = 0; i < BRV_MAX_BOMBS; i++) {
        const Bomb *b = &bv.bomb[i];
        if (!b->alive) continue;
        int blink = b->fuse < 40 ? 3 : b->fuse < 90 ? 6 : 12;
        uint8_t map[PAL_COUNT];
        pal_identity(map);
        if ((b->fuse / blink) % 2 == 0) pal_swap(map, C_SLATE, C_RED);
        if (b->drone) pal_swap(map, C_GREY, C_CYAN);
        spr_draw_ex(&brv_spr[SP_BOMB], (int)b->x - 3, (int)b->y - 5, 0, map, -1);
    }
    for (int i = 0; i < BRV_MAX_FOES; i++)
        if (bv.foe[i].alive && bv.foe[i].kind != MK_BOSS) draw_foe(&bv.foe[i], t);
    for (int w = 0; w < 2; w++) {
        const Player *p = &bv.p[w];
        if (p->on && !p->down && p->drone_on && (p->drone_inv == 0 || (t / 2) % 2))
            spr_draw(&brv_spr[SP_DRONE], (int)p->dx - 3, (int)p->dy - BRV_DRONE_UP - 3, 0);
    }
    for (int w = 0; w < 2; w++) draw_player(&bv.p[w], w, t);
    for (int i = 0; i < BRV_MAX_FOES; i++)
        if (bv.foe[i].alive && bv.foe[i].kind == MK_BOSS) draw_foe(&bv.foe[i], t);
    for (int i = 0; i < BRV_MAX_PSHOTS; i++) {
        const PShot *s = &bv.ps[i];
        if (!s->alive) continue;
        int c = s->kind == PS_NAIL ? C_LIGHT : s->kind == PS_DRONE ? C_CYAN : C_YELLOW;
        gfx_rect((int)s->x - 1, (int)s->y - 1, 2, 2, c);
        if (s->kind == PS_GUN) gfx_pset((int)(s->x - s->vx * 0.6f), (int)(s->y - s->vy * 0.6f), C_AMBER);
    }
    for (int i = 0; i < BRV_MAX_ESHOTS; i++) {
        const EShot *s = &bv.es[i];
        if (!s->alive) continue;
        int x = (int)s->x, y = (int)s->y;
        if (s->kind == ES_SLOW) { gfx_circ(x, y, 2, C_LIME); gfx_pset(x, y, C_WHITE); }
        else if (s->kind == ES_BOSS) { gfx_circ(x, y, 2, (s->t / 4) % 2 ? C_RED : C_ORANGE); gfx_pset(x, y, C_YELLOW); }
        else { gfx_rect(x - 1, y - 1, 3, 3, C_PINK); gfx_pset(x, y, C_WHITE); }
    }
    for (int i = 0; i < BRV_MAX_BLASTS; i++) {
        const Blast *b = &bv.blast[i];
        if (!b->alive) continue;
        int r = (int)(b->r * fminf(1.0f, (float)(b->t + 4) / 8.0f));
        if (b->t < 8) gfx_circ((int)b->x, (int)b->y, r, b->t < 3 ? C_WHITE : C_YELLOW);
        else gfx_dither_circle((int)b->x, (int)b->y, r, C_ORANGE, 16 - b->t);
        gfx_circb((int)b->x, (int)b->y, r, C_RED);
    }
    for (int i = 0; i < BRV_MAX_PARTS; i++) {
        const Part *p = &bv.part[i];
        if (p->life > 0) gfx_pset((int)p->x, (int)p->y, p->col);
    }
    gfx_noclip();
    gfx_camera(0, 0);
}

static void draw_hud(int t) {
    char buf[48], cash[16];
    gfx_rect(0, 0, SCREEN_W, 11, C_INK);
    fmt_cash(cash, sizeof cash, bv.prize);
    snprintf(buf, sizeof buf, "PRIZE %s", cash);
    text_draw(buf, 4, 2, C_YELLOW);
    snprintf(buf, sizeof buf, "LEFT %d", brv_enemies_left());
    text_draw(buf, SCREEN_W - 4 - text_width(buf), 2, bv.boss_out && !bv.boss_dead ? C_RED : C_WHITE);
    if (bv.peeper_tone_t > 0 && (t / 4) % 2) tiny_center("SOMETHING STIRS BELOW", 160, 3, C_LIME);
    else if (bv.round == BRV_ROUNDS - 1) tiny_center("FINAL CARD", 160, 3, C_RED);
    else tiny_center("THE GLASS PIT", 160, 3, C_SLATE);

    gfx_rect(0, 169, SCREEN_W, 11, C_INK);
    snprintf(buf, sizeof buf, "FIGHT %d", bv.round + 1);
    text_draw(buf, 4, 171, C_LIGHT);
    int n = bv.p[1].on ? 2 : 1;
    for (int w = 0; w < n; w++) {
        const Player *p = &bv.p[w];
        int bx = n == 1 ? 70 : 52 + w * 120, by = 172;
        /* the whole bar from the start: what HEART PLATE hasn't opened yet
         * is drawn dim, and a medkit's extra runs on past it */
        int full = BRV_START_HP + 4 * BRV_GEAR[GR_HEART].tiers;
        int boxes = imin(BRV_MED_CAP, imax(full, p->hp));
        int bw = (boxes > 24 ? 2 : 3) - (n == 2 ? 1 : 0);
        tiny_draw(w ? "DOM" : "DICE", bx, by, w ? C_SKY : C_ORANGE);
        bx += w ? 14 : 18;
        for (int k = 0; k < boxes; k++) {
            int kx = bx + k * (bw + 1);
            if (k >= p->maxhp && k >= p->hp) { gfx_rectb(kx, by, bw, 5, C_DUSK); continue; }
            int c = p->down ? C_DUSK : k < p->hp ? (k >= p->maxhp ? C_LIME : C_RED) : C_MAROON;
            gfx_rect(kx, by, bw, 5, c);
        }
        int ex = bx + boxes * (bw + 1) + 4;
        spr_draw(&brv_spr[SP_BOMB], ex, by - 2, 0);
        snprintf(buf, sizeof buf, "%d", p->bombs);
        tiny_draw(buf, ex + 9, by, C_WHITE);
    }
}

static void big_center(const char *s, int y, int c1, int c2) {
    const uint8_t grad[2] = {(uint8_t)c1, (uint8_t)c2};
    ui_fancy_center(s, 160, y, 2, grad, 2, C_INK, C_NIGHT);
}

/* ---- the shop -------------------------------------------------------------- */

#define GRID_X 8
#define GRID_Y 26
#define CELL_W 37
#define CELL_H 24
#define NEXT_X 164

static const char *button_hint(int b) {
    if (b == 0) return "BUY GEAR WITH YOUR CASH.";
    if (b == 1) return bv.round == BRV_ROUNDS - 1 ? "NO MORE BETS: THE FINAL CARD IS FULL." : "+100 TO THE PRIZE, AND ONE MORE PACK, SIGHT UNSEEN.";
    return "INTO THE PIT.";
}

static void draw_desc(int sel, int row, int btnsel) {
    char buf[96];
    if (row == 1) {
        const GearDef *g = &BRV_GEAR[sel];
        int tier = imin(bv.gear[sel], g->tiers - 1);
        if (g->tiers > 1) snprintf(buf, sizeof buf, "%s %d/%d", g->name, imin(bv.gear[sel] + 1, g->tiers), g->tiers);
        else snprintf(buf, sizeof buf, "%s", g->name);
        text_draw(buf, 8, 149, C_YELLOW);
        ui_audit_text("gear name", buf, 8, 149);
        const char *d = brv_maxed(sel) ? "YOU HAVE IT ALL." : g->desc[tier];
        text_draw(d, 8, 158, C_WHITE);
        ui_audit_text("gear text", d, 8, 158);
    } else {
        const char *h = button_hint(btnsel);
        text_draw(h, 8, 153, C_WHITE);
        ui_audit_text("button text", h, 8, 153);
    }
}

static void draw_shop(int t) {
    char buf[48], cash[16];
    gfx_rect(0, 0, SCREEN_W, SCREEN_H, C_INK);
    for (int y = 0; y < SCREEN_H; y += 4) gfx_hline(0, SCREEN_W - 1, y, C_NIGHT);
    snprintf(buf, sizeof buf, "THE BACK ROOM");
    text_draw(buf, 8, 3, C_PINK);
    snprintf(buf, sizeof buf, "FIGHT %d OF 8 NEXT", bv.round + 1);
    text_draw(buf, SCREEN_W - 8 - text_width(buf), 3, C_LIGHT);

    /* the gear */
    ui_panel(4, 12, 154, 128, C_NIGHT, bv.shop_row == 1 ? C_YELLOW : C_DUSK);
    text_draw("THE RACK", 10, 15, C_WHITE);
    fmt_cash(cash, sizeof cash, bv.cash);
    snprintf(buf, sizeof buf, "CASH %s", cash);
    text_draw(buf, 152 - text_width(buf), 15, C_YELLOW);
    for (int i = 0; i < GR_COUNT; i++) {
        int cx = GRID_X + (i % 4) * CELL_W, cy = GRID_Y + (i / 4) * CELL_H + 2;
        bool sel = bv.shop_row == 1 && bv.shop_sel == i;
        if (sel) gfx_rectb(cx - 1, cy - 1, CELL_W - 1, CELL_H, (t / 8) % 2 ? C_YELLOW : C_AMBER);
        int ix = cx + (CELL_W - 2) / 2 - 6;
        spr_draw(&brv_spr[SP_GEAR0 + i], ix, cy + 4, 0);
        /* tiers owned */
        for (int k = 0; k < BRV_GEAR[i].tiers; k++)
            gfx_rect(cx + 2, cy + 5 + k * 3, 2, 2, k < bv.gear[i] ? C_LIME : C_DUSK);
        if (i == bv.sale && !brv_maxed(i)) tiny_draw("DEAL", cx + 22, cy + 1, C_LIME);
        if (i == bv.hike && !brv_maxed(i)) tiny_draw("+100", cx + 22, cy + 1, C_RED);
        if (brv_maxed(i)) snprintf(buf, sizeof buf, "SOLD");
        else snprintf(buf, sizeof buf, "%d", brv_price(i));
        int c = brv_maxed(i) ? C_SLATE : brv_price(i) <= bv.cash ? C_WHITE : C_GREY;
        tiny_center(buf, cx + (CELL_W - 2) / 2, cy + 17, c);
    }

    /* the next fight */
    bool last = bv.round == BRV_ROUNDS - 1;
    ui_panel(NEXT_X - 2, 12, 154, 128, C_NIGHT, last ? C_RED : C_DUSK);
    text_draw(last ? "FINAL CARD" : "ON THE CARD", NEXT_X + 4, 15, last ? C_RED : C_WHITE);
    snprintf(buf, sizeof buf, "%d MONSTERS", brv_lineup_monsters() + (last ? 1 : 0));
    tiny_draw(buf, NEXT_X + 148 - tiny_width(buf), 17, C_LIGHT);
    static const int ICON[MK_KINDS] = {SP_MITE1, SP_GAS0, SP_BRUTE1, SP_KEG, SP_STILT1, SP_PEEP, SP_SLAG_ICON};
    int slots = bv.ngroups + (last ? 1 : 0);
    for (int g = 0; g < slots; g++) {
        int cx = NEXT_X + 2 + (g % 4) * CELL_W, cy = GRID_Y + (g / 4) * CELL_H + 2;
        bool fresh = g == bv.ngroups - 1 && bv.raise_flash > 0;
        gfx_rect(cx, cy, CELL_W - 3, CELL_H - 2, fresh && (t / 2) % 2 ? C_DUSK : C_INK);
        if (g == bv.ngroups) {
            /* the boss */
            gfx_circ(cx + 17, cy + 9, 7, C_MAROON);
            gfx_rect(cx + 12, cy + 6, 10, 2, C_FOREST);
            gfx_rect(cx + 13, cy + 1, 8, 3, C_INK);
            tiny_center("BOSS", cx + 17, cy + 16, C_RED);
            continue;
        }
        const Group *gr = &bv.lineup[g];
        const Sprite *s = &brv_spr[ICON[gr->kind]];
        spr_draw(s, cx + 17 - s->w / 2, cy + 8 - s->h / 2, 0);
        snprintf(buf, sizeof buf, "%d", gr->count);
        tiny_center(buf, cx + 17, cy + 16, gr->kind == MK_SLAG ? C_ORANGE : C_WHITE);
    }

    /* the three buttons */
    static const char *const LABEL[3] = {"BUY", "UP THE STAKES", "TO THE PIT"};
    int bx[3] = {6, 82, 236}, bw[3] = {70, 148, 78};
    for (int b = 0; b < 3; b++) {
        bool on = bv.shop_row == 0 && bv.shop_btn == b;
        bool dead = b == 1 && last;
        ui_panel(bx[b], 128 + 2, bw[b], 14, on ? C_WINE : C_NIGHT, on ? C_YELLOW : C_SLATE);
        if (b == 1) {
            fmt_cash(cash, sizeof cash, bv.prize);
            snprintf(buf, sizeof buf, "%s %s", LABEL[b], cash);
        } else {
            snprintf(buf, sizeof buf, "%s", LABEL[b]);
        }
        text_center(buf, bx[b] + bw[b] / 2, 128 + 6, dead ? C_SLATE : on ? C_WHITE : C_LIGHT);
    }
    draw_desc(bv.shop_sel, bv.shop_row, bv.shop_btn);
    if (bv.shop_msg_t > 0 && bv.shop_msg) {
        gfx_rect(0, 166, SCREEN_W, 12, C_WINE);
        text_center(bv.shop_msg, 160, 168, C_YELLOW);
    } else {
        int fx = ui_hint(8, 170, GLYPH_A, bv.shop_row == 1 ? "BUY" : "CHOOSE", C_GREY);
        if (bv.shop_row == 1) ui_hint(fx, 170, GLYPH_B, "BACK", C_GREY);
    }
}

/* every gear line and button line inside the screen and clear of each other */
int brv_shop_audit(void) {
    int bad = 0;
    char subject[64];
    for (int i = 0; i < GR_COUNT; i++) {
        for (int k = 0; k <= BRV_GEAR[i].tiers; k++) {
            uint8_t keep = bv.gear[i];
            bv.gear[i] = (uint8_t)k;
            snprintf(subject, sizeof subject, "BRAVADO gear %s tier %d", BRV_GEAR[i].name, k);
            ui_audit_begin(subject, true);
            ui_audit_area("description", 4, 146, 312, 20);
            draw_desc(i, 1, 0);
            bad += ui_audit_end();
            bv.gear[i] = keep;
        }
    }
    int keep_round = bv.round;
    for (int r = 1; r < BRV_ROUNDS; r++) {
        bv.round = r;
        for (int b = 0; b < 3; b++) {
            snprintf(subject, sizeof subject, "BRAVADO button %d round %d", b, r);
            ui_audit_begin(subject, true);
            ui_audit_area("description", 4, 146, 312, 20);
            draw_desc(0, 0, b);
            bad += ui_audit_end();
        }
    }
    bv.round = keep_round;
    static const char *const MSG[] = {"THE HOUSE THANKS YOU FOR YOUR GENEROSITY!", "THE BOOK IS FULL: 16 PACKS IS THE LIMIT.",
                                      "THE FINAL CARD IS ALREADY FULL.", "NOT ENOUGH CASH.", "SOLD OUT.",
                                      "+100: 6 POWDER KEGS", "+100: 5 STILTERS", "+100: SLAG, 3 POOLS"};
    for (int i = 0; i < ARRAY_LEN(MSG); i++) {
        ui_audit_begin("BRAVADO shop message", true);
        ui_audit_area("message", 0, 166, SCREEN_W, 12);
        ui_audit_text("message", MSG[i], 160 - text_width(MSG[i]) / 2, 168);
        bad += ui_audit_end();
    }
    return bad;
}

/* ---- the other screens ------------------------------------------------------ */

static void draw_title(int t) {
    gfx_rect(0, 0, SCREEN_W, SCREEN_H, C_INK);
    for (int i = 0; i < 40; i++) {
        int x = (i * 71 + t / 3) % SCREEN_W, y = (i * 43) % 120;
        gfx_pset(x, y, i % 4 ? C_DUSK : C_PURPLE);
    }
    /* the pit's rim and a spotlight */
    gfx_dither_circle(160, 120, 70, C_NIGHT, 12);
    gfx_rect(40, 150, 240, 2, C_VIOLET);
    static const uint8_t grad[3] = {C_YELLOW, C_AMBER, C_RED};
    ui_fancy_center("BRAVADO", 160, 14, 4, grad, 3, C_INK, C_MAROON);
    tiny_center("BET BIG. FIGHT BIGGER. GET PAID.", 160, 50, C_PINK);
    spr_draw_scaled(&brv_spr[(t / 20) % 2 ? SP_DICE_D : SP_DICE_D2], 145, 64, 3, 0);
    for (int k = 0; k < 3; k++) {
        int mx = 60 + k * 18 + (int)(sinf((float)t * 0.05f + (float)k) * 4);
        spr_draw(&brv_spr[(t / 6 + k) % 2 ? SP_MITE1 : SP_MITE2], mx, 128, 0);
        spr_draw(&brv_spr[(t / 6 + k) % 2 ? SP_MITE1 : SP_MITE2], 250 - k * 18, 128, SPR_FLIPX);
    }
    static const char *const ITEMS[2] = {"1 PLAYER", "2 PLAYERS"};
    for (int i = 0; i < 2; i++) {
        int y = 104 + i * 11;
        bool lock = i == 1 && plat_kind() == PLAT_VITA;
        int c = bv.menu == i ? C_WHITE : C_GREY;
        if (lock) c = C_SLATE;
        int w = text_width(ITEMS[i]);
        int x = i == 0 ? 40 : 236;
        if (bv.menu == i) ui_cursor(x - 10, y, t);
        text_draw(ITEMS[i], x, y, c);
        if (lock) text_draw(GLYPH_LOCK, x + w + 3, y, C_SLATE);
    }
    char buf[80];
    snprintf(buf, sizeof buf, "MOST UPGRADES %d   MOST KILLS %d   WINS %d", bvs.most_upgrades, bvs.most_kills, bvs.wins);
    tiny_center(buf, 160, 156, C_LIGHT);
    int fx = ui_hint(8, 168, GLYPH_A, "START", C_GREY);
    ui_hint(fx, 168, GLYPH_B, "LIBRARY", C_GREY);
    tiny_draw("1987 BEAMDOWN", SCREEN_W - 8 - tiny_width("1987 BEAMDOWN"), 170, C_SLATE);
}

static void draw_story(int t) {
    gfx_rect(0, 0, SCREEN_W, SCREEN_H, C_INK);
    for (int i = 0; i < 30; i++) gfx_pset((i * 97) % SCREEN_W, (i * 61 + t / 4) % SCREEN_H, C_DUSK);
    int shown = imin(ARRAY_LEN(STORY), t / 50 + 1);
    for (int i = 0; i < shown; i++) text_center(STORY[i], 160, 34 + i * 12, i == shown - 1 && (t % 50) < 6 ? C_WHITE : C_LIGHT);
    spr_draw(&brv_spr[SP_DICE_S], 20 + (t / 2) % 280, 150, 0);
    if (t > 20) ui_hint(230, 168, GLYPH_A, "SKIP", C_GREY);
}

static void draw_banner(int t) {
    char buf[32], cash[16];
    if (bv.round == BRV_ROUNDS - 1) big_center("FINAL CARD", 58, C_RED, C_ORANGE);
    else {
        snprintf(buf, sizeof buf, "FIGHT %d", bv.round + 1);
        big_center(buf, 58, C_YELLOW, C_AMBER);
    }
    fmt_cash(cash, sizeof cash, bv.prize);
    snprintf(buf, sizeof buf, "FOR %s", cash);
    text_center_shadow(buf, 160, 80, C_WHITE, C_INK);
    if ((t / 10) % 2) text_center_shadow("GET READY", 160, 96, C_PINK, C_INK);
}

static void draw_won(int t) {
    char buf[48], cash[16];
    ui_panel(70, 50, 180, 66, C_NIGHT, C_YELLOW);
    fmt_cash(cash, sizeof cash, bv.prize);
    snprintf(buf, sizeof buf, "YOU WIN %s", cash);
    big_center(buf, 58, C_YELLOW, C_AMBER);
    fmt_cash(cash, sizeof cash, bv.cash);
    snprintf(buf, sizeof buf, "CASH %s", cash);
    text_center(buf, 160, 80, C_WHITE);
    snprintf(buf, sizeof buf, "%d KNOCKED OUT", bv.fight_kills);
    tiny_center(buf, 160, 92, C_LIGHT);
    if (bv.state_t > 40 && (t / 15) % 2)
        text_center(bv.round == BRV_ROUNDS - 1 ? GLYPH_A " CASH OUT" : GLYPH_A " TO THE BACK ROOM", 160, 102, C_PINK);
}

static void draw_over(int t) {
    char buf[48], cash[16];
    gfx_darken_rect(0, 11, SCREEN_W, 158, 2);
    big_center("BUSTED", 56, C_RED, C_MAROON);
    fmt_cash(cash, sizeof cash, bv.cash);
    snprintf(buf, sizeof buf, "FIGHT %d " GLYPH_DOT " CASH %s", bv.round + 1, cash);
    text_center_shadow(buf, 160, 80, C_WHITE, C_INK);
    snprintf(buf, sizeof buf, "%d UPGRADES " GLYPH_DOT " %d KILLS", bv.upgrades, bv.kills);
    text_center_shadow(buf, 160, 92, C_LIGHT, C_INK);
    if (bv.state_t > 60 && (t / 15) % 2) text_center_shadow(GLYPH_A " TITLE", 160, 110, C_PINK, C_INK);
}

static void draw_ending(int t) {
    char buf[64], cash[16];
    gfx_rect(0, 0, SCREEN_W, SCREEN_H, C_INK);
    for (int i = 0; i < 50; i++) {
        int x = (i * 97 + i * i * 13) % SCREEN_W, y = (i * i * 31 + i * 7 + t / 2 + (i % 3) * t / 4) % SCREEN_H;
        gfx_pset(x, y, i % 3 == 0 ? C_YELLOW : i % 3 == 1 ? C_PINK : C_CYAN);
    }
    big_center("CASHED OUT", 20, C_YELLOW, C_AMBER);
    text_center("THE PIT BOSS IS SCRAP. THE CROWD GOES WILD.", 160, 50, C_WHITE);
    fmt_cash(cash, sizeof cash, bv.cash);
    snprintf(buf, sizeof buf, "DICE WALKS OUT WITH %s.", cash);
    text_center(buf, 160, 64, C_YELLOW);
    if (bv.cash >= 4500) {
        text_center("ENOUGH TO BUY BACK HER SHIP", 160, 82, C_LIGHT);
        text_center("AND THE HOUSE A ROUND OF DRINKS.", 160, 92, C_LIGHT);
    } else {
        text_center("ENOUGH TO BUY BACK HER SHIP,", 160, 82, C_LIGHT);
        text_center("IF SHE SELLS THE JACKET.", 160, 92, C_LIGHT);
    }
    int x = 60 + (t / 2) % 220;
    spr_draw(&brv_spr[(t / 8) % 2 ? SP_DICE_S : SP_DICE_S2], x, 130, 0);
    if (bv.p[1].on) spr_draw(&brv_spr[(t / 8) % 2 ? SP_DICE_S2 : SP_DICE_S], x - 16, 130, 0);
    if (bv.state_t > 120) ui_hint(230, 168, GLYPH_A, "CREDITS", C_GREY);
}

static void draw_credits(int t) {
    gfx_rect(0, 0, SCREEN_W, SCREEN_H, C_INK);
    int y0 = SCREEN_H - bv.state_t / 3;
    for (int i = 0; i < ARRAY_LEN(CREDITS); i++) {
        int y = y0 + i * 12;
        if (y < -8 || y > SCREEN_H) continue;
        text_center(CREDITS[i], 160, y, i == 0 ? C_YELLOW : C_LIGHT);
    }
    (void)t;
}

void brv_draw(void) {
    int t = bv.frame_t;
    switch (bv.state) {
    case BS_TITLE: draw_title(t); break;
    case BS_STORY: draw_story(bv.state_t); break;
    case BS_SHOP: draw_shop(t); break;
    case BS_ENDING: draw_ending(t); break;
    case BS_CREDITS: draw_credits(t); break;
    default:
        draw_world(t);
        draw_hud(t);
        if (bv.state == BS_BANNER) draw_banner(t);
        if (bv.state == BS_WON) draw_won(t);
        if (bv.state == BS_OVER) draw_over(t);
        break;
    }
}

/* the library label: Dice holding the middle of the pit against mites */
void brv_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_NIGHT);
    for (int gx = x; gx < x + w; gx += 12) gfx_vline(gx, y, y + h - 1, C_DUSK);
    for (int gy = y; gy < y + h; gy += 12) gfx_hline(x, x + w - 1, gy, C_DUSK);
    gfx_circ(x + 22, y + 40, 9, C_RED);
    gfx_circ(x + 22, y + 40, 4, C_ORANGE);
    gfx_circ(x + w - 22, y + 18, 7, C_RED);
    gfx_circ(x + w - 22, y + 18, 3, C_ORANGE);
    int px = x + w / 2 - 5, py = y + 26;
    spr_draw(&brv_spr[SP_DICE_S], px, py, 0);
    for (int k = 0; k < 4; k++) {
        int bx = px + 14 + ((t * 3 + k * 20) % 60);
        gfx_rect(bx, py + 6, 2, 2, C_YELLOW);
    }
    for (int k = 0; k < 3; k++) {
        int mx = x + w - 14 - ((t / 2 + k * 23) % 60);
        spr_draw(&brv_spr[(t / 6 + k) % 2 ? SP_MITE1 : SP_MITE2], mx, py + 4 + (k - 1) * 9, SPR_FLIPX);
    }
    spr_draw(&brv_spr[SP_KEG], x + 8, y + 8, 0);
    static const uint8_t grad[2] = {C_YELLOW, C_AMBER};
    ui_fancy_text("BRAVADO", x + 36, y + 4, 1, grad, 2, C_INK, C_MAROON);
    gfx_noclip();
}
