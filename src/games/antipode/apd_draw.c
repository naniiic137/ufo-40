/* ANTIPODE - drawing: the station's tiles, the machines and the bosses, the
 * suit, the bar along the top, the cradle's MODS and MAP pages, terminals,
 * the title, the story, the ending and the credits. */
#include "apd.h"

int apd_max_armor(void);
extern int apd_lightning_x[4];

/* each sector's metal: base, light edge, dark edge, background */
static const uint8_t SEC_COL[APD_SECTORS][4] = {
    {C_SLATE, C_GREY, C_DUSK, C_NIGHT},
    {C_TEAL, C_JADE, C_FOREST, C_INK},
    {C_PURPLE, C_VIOLET, C_DUSK, C_INK},
    {C_WINE, C_RED, C_MAROON, C_INK},
};

static const uint8_t MOD_COL[APD_MODS] = {C_ORANGE, C_JADE, C_CYAN, C_TAN};

static void mod_remap(uint8_t *map, int mod) {
    pal_identity(map);
    static const uint8_t R[APD_MODS][3] = {
        {C_ORANGE, C_AMBER, C_YELLOW}, {C_FOREST, C_JADE, C_LIME}, {C_NAVY, C_CYAN, C_ICE}, {C_BROWN, C_TAN, C_CREAM}};
    pal_swap(map, C_ORANGE, R[mod][0]);
    pal_swap(map, C_AMBER, R[mod][1]);
    pal_swap(map, C_YELLOW, R[mod][2]);
}

/* ---- tiles ------------------------------------------------------------------------ */

static int tile_rel(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= ag.rw || ty >= ag.rh) return T_WALL;
    return ag.tile[ty][tx];
}

static bool wallish(int t) { return t == T_WALL || t == T_FAKE || t == T_GREY; }

static void draw_tile(int t, int tx, int ty, int sx, int sy, int sec) {
    const uint8_t *c = SEC_COL[sec];
    int f = ag.frame_t;
    switch (t) {
    case T_WALL: case T_FAKE: case T_GREY: {
        int base = t == T_GREY ? C_GREY : c[0];
        gfx_rect(sx, sy, 16, 16, base);
        if (!wallish(tile_rel(tx, ty - 1))) gfx_hline(sx, sx + 15, sy, c[1]);
        if (!wallish(tile_rel(tx, ty + 1))) gfx_hline(sx, sx + 15, sy + 15, c[2]);
        if (!wallish(tile_rel(tx - 1, ty))) gfx_vline(sx, sy, sy + 15, c[1]);
        if (!wallish(tile_rel(tx + 1, ty))) gfx_vline(sx + 15, sy, sy + 15, c[2]);
        if (((tx * 7 + ty * 3) & 3) == 0) { gfx_pset(sx + 4, sy + 4, c[2]); gfx_pset(sx + 11, sy + 11, c[2]); }
        if (t == T_FAKE) gfx_dither(sx + 2, sy + 2, 12, 12, C_FOREST, 3);
        if (t == T_GREY) { gfx_rectb(sx + 1, sy + 1, 14, 14, C_LIGHT); gfx_hline(sx + 3, sx + 12, sy + 8, C_SLATE); }
        break;
    }
    case T_SPUP: case T_SPDN: {
        bool up = t == T_SPUP;
        for (int k = 0; k < 4; k++) {
            int x0 = sx + k * 4;
            for (int r = 0; r < 8; r++) {
                int y = up ? sy + 15 - r : sy + r;
                int w = (8 - r) / 2;
                gfx_hline(x0 + 2 - w / 2, x0 + 1 + (w + 1) / 2, y, r > 5 ? C_WHITE : C_LIGHT);
            }
        }
        gfx_rect(sx, up ? sy + 13 : sy, 16, 3, C_GREY);
        break;
    }
    case T_PLAT: {
        gfx_rect(sx, sy + 6, 16, 4, C_LIGHT);
        gfx_hline(sx, sx + 15, sy + 6, C_WHITE);
        gfx_hline(sx, sx + 15, sy + 9, C_GREY);
        /* the arrows point to the side that holds you: your feet's side */
        int g = ag.pl.grav;
        int ay = g > 0 ? sy + 3 : sy + 12;
        gfx_pset(sx + 7, ay, C_RED);
        gfx_hline(sx + 6, sx + 8, ay + g, C_RED);
        break;
    }
    case T_ACID: {
        bool surface = tile_rel(tx, ty - 1) == T_AIR || tile_rel(tx, ty - 1) == T_PROP;
        gfx_dither(sx, sy, 16, 16, C_LIME, 10);
        if (surface) {
            for (int x = 0; x < 16; x++) {
                int wy = ((x + f / 6 + tx * 5) % 8) < 4 ? 1 : 2;
                gfx_vline(sx + x, sy + wy, sy + 3, C_LEAF);
                gfx_pset(sx + x, sy + wy, C_LIME);
            }
        }
        break;
    }
    case T_CRACK:
        gfx_rect(sx, sy, 16, 16, C_TAN);
        gfx_rectb(sx, sy, 16, 16, C_EARTH);
        gfx_line(sx + 3, sy + 2, sx + 8, sy + 8, C_BROWN);
        gfx_line(sx + 8, sy + 8, sx + 6, sy + 14, C_BROWN);
        gfx_line(sx + 8, sy + 8, sx + 13, sy + 6, C_BROWN);
        break;
    case T_BARREL:
        gfx_rect(sx + 2, sy, 12, 16, C_ORANGE);
        gfx_rectb(sx + 2, sy, 12, 16, C_MAROON);
        gfx_hline(sx + 3, sx + 12, sy + 4, C_RED);
        gfx_hline(sx + 3, sx + 12, sy + 11, C_RED);
        gfx_vline(sx + 5, sy + 1, sy + 14, C_AMBER);
        break;
    case T_DOOR: case T_LOCK: case T_SHUT: {
        int col = t == T_DOOR ? C_YELLOW : t == T_LOCK ? C_AMBER : C_GREY;
        gfx_rect(sx + 4, sy, 8, 16, col);
        gfx_vline(sx + 4, sy, sy + 15, C_INK);
        gfx_vline(sx + 11, sy, sy + 15, C_INK);
        if (t == T_LOCK) {
            gfx_rectb(sx + 2, sy, 12, 16, C_WHITE);
            gfx_rectb(sx + 3, sy, 10, 16, C_INK);
            int d = ag.tdoor[ty][tx] - 1;
            if (d >= 0) {
                char s[2] = {(char)('1' + apd_doors[d].type - E_PASS1), 0};
                tiny_draw(s, sx + 7, sy + 5, C_INK);
            }
        }
        if (t == T_SHUT) { gfx_hline(sx + 4, sx + 11, sy + 5, C_SLATE); gfx_hline(sx + 4, sx + 11, sy + 10, C_SLATE); }
        break;
    }
    case T_ZAP: {
        for (int k = 0; k < 3; k++) {
            int x = sx + 3 + k * 5 + ((f / 3 + k + ty) % 3) - 1;
            gfx_vline(x, sy, sy + 15, (f + k) % 4 < 2 ? C_CYAN : C_ICE);
        }
        break;
    }
    default: break;
    }
}

static void draw_props(int sec) {
    (void)sec;
    int x0 = ag.cam_x / APD_TILE, y0 = ag.cam_y / APD_TILE;
    for (int ty = y0; ty <= y0 + APD_CH; ty++)
        for (int tx = x0; tx <= x0 + APD_CW; tx++)
            if (tile_rel(tx, ty) == T_PROP) {
                int sx = tx * APD_TILE - ag.cam_x, sy = ty * APD_TILE - ag.cam_y + APD_HUD;
                /* a desk with a console on it; the left half reaches over the next tile */
                gfx_rect(sx - 10, sy + 4, 24, 12, C_BROWN);
                gfx_rectb(sx - 10, sy + 4, 24, 12, C_INK);
                gfx_hline(sx - 9, sx + 12, sy + 5, C_TAN);
                gfx_rect(sx - 6, sy - 2, 10, 7, C_SLATE);
                gfx_rect(sx - 5, sy - 1, 8, 4, C_JADE);
            }
}

static void draw_background(int sec) {
    gfx_rect(0, APD_HUD, SCREEN_W, SCREEN_H - APD_HUD, SEC_COL[sec][3]);
    /* faint struts in the dark */
    int ox = -(ag.cam_x / 2) % 64, oy = -(ag.cam_y / 2) % 64;
    for (int x = ox; x < SCREEN_W; x += 64) gfx_dither(x, APD_HUD, 2, SCREEN_H - APD_HUD, C_DUSK, 4);
    for (int y = oy; y < SCREEN_H; y += 64)
        if (y + APD_HUD < SCREEN_H) gfx_dither(0, y + APD_HUD, SCREEN_W, 2, C_DUSK, 4);
    const ApdSpec *sp = &APD_SPECS[apd_rooms[ag.room].spec];
    if (sp->flags & RF_HEAT) gfx_dither(0, APD_HUD, SCREEN_W, SCREEN_H - APD_HUD, C_MAROON, 3 + (ag.frame_t / 8) % 2);
}

static void draw_tiles(int sec) {
    int x0 = ag.cam_x / APD_TILE, y0 = ag.cam_y / APD_TILE;
    for (int ty = y0; ty <= y0 + APD_CH; ty++)
        for (int tx = x0; tx <= x0 + APD_CW; tx++) {
            int t = tile_rel(tx, ty);
            if (t == T_AIR || t == T_PROP || t == T_ACID) continue;
            draw_tile(t, tx, ty, tx * APD_TILE - ag.cam_x, ty * APD_TILE - ag.cam_y + APD_HUD, sec);
        }
}

static void draw_acid(int sec) {
    int x0 = ag.cam_x / APD_TILE, y0 = ag.cam_y / APD_TILE;
    for (int ty = y0; ty <= y0 + APD_CH; ty++)
        for (int tx = x0; tx <= x0 + APD_CW; tx++)
            if (tile_rel(tx, ty) == T_ACID)
                draw_tile(T_ACID, tx, ty, tx * APD_TILE - ag.cam_x, ty * APD_TILE - ag.cam_y + APD_HUD, sec);
}

/* ---- things ------------------------------------------------------------------------ */

static int SX(int px) { return px - ag.cam_x; }
static int SY(int py) { return py - ag.cam_y + APD_HUD; }

static void spike_ball(int x, int y, int col, bool live) {
    int c = live ? col : C_DUSK;
    gfx_circ(x, y, 5, c);
    gfx_circb(x, y, 5, C_INK);
    static const int8_t DX[8] = {7, 5, 0, -5, -7, -5, 0, 5}, DY[8] = {0, 5, 7, 5, 0, -5, -7, -5};
    for (int k = 0; k < 8; k++) gfx_line(x + DX[k] * 5 / 7, y + DY[k] * 5 / 7, x + DX[k], y + DY[k], live ? C_WHITE : C_SLATE);
    if (live) gfx_pset(x - 2, y - 2, C_WHITE);
}

static void draw_item(int kind, int var, int x, int y) {
    uint8_t map[256];
    switch (kind) {
    case IT_ARMOR: spr_draw(&apd_spr[SPR_ARMOR], x - 6, y - 6, 0); break;
    case IT_SPARE: spr_draw(&apd_spr[SPR_SPARE], x - 6, y - 6, 0); break;
    case IT_DAMPER: spr_draw(&apd_spr[SPR_DAMPER], x - 6, y - 6, 0); break;
    default:
        mod_remap(map, var);
        spr_draw_ex(&apd_spr[SPR_MOD], x - 6, y - 6, 0, map, -1);
        break;
    }
}

static void draw_things(bool front) {
    int f = ag.frame_t;
    for (int i = 0; i < ag.nth; i++) {
        const ApdThing *t = &ag.th[i];
        if (!t->alive) continue;
        int x = SX(t->tx * APD_TILE + 8), y = SY(t->ty * APD_TILE + 8);
        if (x < -40 || x > SCREEN_W + 40 || y < -40 || y > SCREEN_H + 40) continue;
        if (front) continue;
        switch (t->kind) {
        case TH_POD:
            spr_draw(&apd_spr[SPR_POD], x - 12, y + 8 - 32, 0);
            if (t->id == ag.pod_zone && (f / 20) % 2) tiny_center("CRADLE", x, y - 30, C_PINK);
            break;
        case TH_TERM:
            spr_draw(&apd_spr[SPR_TERM], x - 7, y + 8 - 16, 0);
            break;
        case TH_TELE: {
            bool lock = t->id == 9 && ag.run.ciphers != 15 && apd_rooms[ag.room].sector != SEC_CROWN;
            gfx_rect(x - 14, y + 4, 28, 4, C_SLATE);
            gfx_rectb(x - 14, y + 4, 28, 4, C_INK);
            for (int k = 0; k < 4; k++) {
                int h = 6 + ((f / 4 + k * 3) % 10);
                if (!lock) gfx_dither(x - 10 + k * 6, y + 4 - h, 3, h, C_CYAN, 6);
            }
            if (t->id == 9)
                for (int k = 0; k < 4; k++) gfx_rect(x - 12 + k * 7, y + 9, 4, 3, (ag.run.ciphers >> k) & 1 ? C_PINK : C_DUSK);
            break;
        }
        case TH_ITEM: {
            int bob = (f / 10 + i) % 4 < 2 ? 0 : 1;
            draw_item(apd_items[t->id].kind, apd_items[t->id].var, x, y + bob);
            break;
        }
        case TH_BALL: spike_ball(x, y, t->var == 0 ? C_RED : C_MAGENTA, t->var == 0 ? ag.pl.grav > 0 : ag.pl.grav < 0); break;
        case TH_TARGET:
            gfx_circ(x, y, 5, C_WHITE);
            gfx_circ(x, y, 3, C_RED);
            gfx_circ(x, y, 1, C_WHITE);
            break;
        case TH_BUTTON:
            gfx_rect(x - 6, y + 4, 12, 4, t->t > 0 ? C_LIME : C_FOREST);
            gfx_rectb(x - 6, y + 4, 12, 4, C_INK);
            break;
        case TH_EMITTER:
            gfx_rect(x - 5, y - 5, 10, 10, C_SLATE);
            gfx_rectb(x - 5, y - 5, 10, 10, C_INK);
            gfx_circ(x, y, 2, (f / 4) % 2 ? C_CYAN : C_ICE);
            break;
        default: break;
        }
    }
}

/* ---- foes and bosses ---------------------------------------------------------------- */

static void outline_rect(int x, int y, int w, int h, int fill, int edge) {
    gfx_rect(x, y, w, h, fill);
    gfx_rectb(x, y, w, h, edge);
}

static void draw_foe(int i) {
    const ApdFoe *f = &ag.foe[i];
    int x = SX(APD_PX(f->x)), y = SY(APD_PX(f->y));
    if (x < -80 || x > SCREEN_W + 80 || y < -80 || y > SCREEN_H + 80) return;
    int flags = f->up ? SPR_FLIPY : 0;
    if (f->dir < 0) flags |= SPR_FLIPX;
    bool fl = f->flash > 0;
    uint8_t map[256];
    pal_identity(map);
    int t = ag.frame_t;
    bool low = APD_IS_BOSS(f->kind) && f->maxhp > 0 && f->hp * 4 < f->maxhp && i == ag.boss_foe;
    switch (f->kind) {
    case FO_TREAD: case FO_TREAD_R: case FO_TREAD_G:
        if (f->kind == FO_TREAD_R) { pal_swap(map, C_GREY, C_RED); pal_swap(map, C_LIGHT, C_ORANGE); pal_swap(map, C_RED, C_YELLOW); }
        if (f->kind == FO_TREAD_G) { pal_swap(map, C_GREY, C_AMBER); pal_swap(map, C_LIGHT, C_YELLOW); pal_swap(map, C_SLATE, C_BROWN); }
        spr_draw_ex(&apd_spr[(t / 6) % 2 ? SPR_TREAD1 : SPR_TREAD0], x - 7, y - 6, flags, map, fl ? C_WHITE : -1);
        break;
    case FO_SPITTER: spr_draw_ex(&apd_spr[SPR_SPITTER], x - 7, y - 5, flags & SPR_FLIPY, map, fl ? C_WHITE : -1); break;
    case FO_HIVE: spr_draw_ex(&apd_spr[SPR_HIVE], x - 9, y - 7, flags & SPR_FLIPY, map, fl ? C_WHITE : -1); break;
    case FO_SKIP: spr_draw_ex(&apd_spr[SPR_SKIP], x - 6, y - 6, flags, map, fl ? C_WHITE : -1); break;
    case FO_SKIMMER: spr_draw_ex(&apd_spr[SPR_SKIMMER], x - 7, y - 4, flags & SPR_FLIPX, map, fl ? C_WHITE : -1); break;
    case FO_STAREYE: {
        gfx_circ(x, y, 10, fl ? C_WHITE : C_RED);
        gfx_circb(x, y, 10, C_MAROON);
        gfx_circ(x, y, 6, C_CREAM);
        int px = APD_PX(ag.pl.x) - APD_PX(f->x), py = APD_PX(ag.pl.y) - APD_PX(f->y);
        gfx_circ(x + isign(px) * 2, y + isign(py) * 2, 3, C_INK);
        for (int k = 0; k < 8; k++) {
            static const int8_t DX[8] = {12, 9, 0, -9, -12, -9, 0, 9}, DY[8] = {0, 9, 12, 9, 0, -9, -12, -9};
            gfx_pset(x + DX[k], y + DY[k], C_ORANGE);
        }
        break;
    }
    case FO_MITE:
        gfx_circ(x, y, 3, fl ? C_WHITE : C_SKY);
        gfx_pset(x - 4 + (t / 2) % 2, y - 2, C_ICE);
        gfx_pset(x + 4 - (t / 2) % 2, y - 2, C_ICE);
        break;
    case FO_SENTRY:
        gfx_circ(x, y, 6, fl ? C_WHITE : C_LIME);
        gfx_circb(x, y, 6, C_FOREST);
        gfx_circb(x, y, 8 + (t / 8) % 2, C_JADE);
        gfx_pset(x, y, C_INK);
        break;
    case FO_SCRAPPER: {
        int c = fl ? C_WHITE : low ? C_RED : C_BROWN;
        int dy = f->up ? -1 : 1;
        outline_rect(x - 18, y - 6 * dy - 6, 36, 12, c, C_INK);
        outline_rect(x - 10, y - 12 * dy - 4, 20, 8, C_TAN, C_INK);
        gfx_rect(x + (f->dir > 0 ? 8 : -20), y - 12 * dy - 2, 12, 3, C_SLATE);
        for (int k = 0; k < 5; k++) gfx_circ(x - 14 + k * 7, y + 8 * dy, 3, (k + t / 4) % 2 ? C_GREY : C_SLATE);
        break;
    }
    case FO_WARDEN: {
        int c = fl ? C_WHITE : low ? C_RED : C_SLATE;
        int dy = f->up ? -1 : 1;
        outline_rect(x - 20, y - 24, 40, 48, c, C_INK);
        outline_rect(x - 12, y - 18 * dy - 6, 24, 12, C_DUSK, C_INK);
        gfx_rect(x - 8 + (t / 10) % 8, y - 18 * dy - 2, 4, 4, C_RED);
        outline_rect(x - 26, y - 6, 6, 18, C_GREY, C_INK);
        outline_rect(x + 20, y - 6, 6, 18, C_GREY, C_INK);
        gfx_rect(x - 16, y + 18 * dy - 3, 10, 6, C_GREY);
        gfx_rect(x + 6, y + 18 * dy - 3, 10, 6, C_GREY);
        break;
    }
    case FO_BRACKISH: {
        int c = fl ? C_WHITE : low ? C_RED : C_TEAL;
        outline_rect(x - 28, y - 16, 56, 32, c, C_INK);
        gfx_circ(x, y - 12, 7, (f->flags & FF_NOHIT) ? C_FOREST : C_YELLOW);
        gfx_circ(x, y - 12, 3, C_INK);
        int cl = (t / 8) % 2 ? 2 : 0;
        outline_rect(x - 40, y - 10 - cl, 12, 10, C_JADE, C_INK);
        outline_rect(x + 28, y - 10 + cl, 12, 10, C_JADE, C_INK);
        break;
    }
    case FO_LAMPREY:
        gfx_circ(x, y, 8, fl ? C_WHITE : low ? C_RED : C_PINK);
        gfx_circb(x, y, 8, C_INK);
        gfx_circ(x + f->dir * 4, y, 3, C_INK);
        gfx_pset(x + f->dir * 4, y - 1, C_WHITE);
        break;
    case FO_LAMP_SEG:
        gfx_circ(x, y, 7, C_MAGENTA);
        gfx_circb(x, y, 7, C_INK);
        gfx_hline(x - 4, x + 4, y, C_PURPLE);
        break;
    case FO_COILHEART: {
        bool open = !(f->flags & FF_SHIELD);
        gfx_circ(x, y, 16, fl ? C_WHITE : low ? C_RED : C_PURPLE);
        gfx_circb(x, y, 16, C_INK);
        gfx_circ(x, y, 9, open ? C_YELLOW : C_DUSK);
        if (!open) gfx_circb(x, y, 19 + (t / 4) % 2, C_CYAN);
        break;
    }
    case FO_NODE:
        gfx_circ(x, y, 7, fl ? C_WHITE : C_VIOLET);
        gfx_circb(x, y, 7, C_INK);
        gfx_circ(x, y, 3, (t / 6) % 2 ? C_CYAN : C_ICE);
        break;
    case FO_OBVERSE: {
        /* the other suit: white and red, her visor dark */
        pal_swap(map, C_NAVY, C_MAROON);
        pal_swap(map, C_BLUE, C_LIGHT);
        pal_swap(map, C_SKY, C_RED);
        pal_swap(map, C_PINK, C_INK);
        if (low) pal_swap(map, C_LIGHT, C_PINK);
        spr_draw_ex(&apd_spr[(t / 8) % 2 ? SPR_SUIT1 : SPR_SUIT0], x - 6, y - 10, flags, map, fl ? C_WHITE : -1);
        break;
    }
    case FO_HALLORAN: {
        int c = fl ? C_WHITE : low ? C_RED : C_WINE;
        if (ag.boss_phase == 0) {
            outline_rect(x - 24, y - 20, 48, 40, (f->flags & FF_REFLECT) ? C_ICE : c, C_INK);
            outline_rect(x - 10, y - 30, 20, 12, C_MAROON, C_INK);
            gfx_rect(x - 6, y - 26, 12, 4, C_RED);
            outline_rect(x + f->dir * 24 - 6, y - 14, 12, 10, C_GREY, C_INK); /* the hand */
        } else if (ag.boss_phase == 1) {
            gfx_circ(x, y, 14, c);
            gfx_circb(x, y, 14, C_INK);
            gfx_circ(x, y, 7 + (t / 6) % 3, C_PINK);
        } else {
            gfx_line(x, SY(0), x, y - 18, C_MAROON);
            gfx_line(x - 1, SY(0), x - 1, y - 18, C_WINE);
            bool shut = (f->flags & FF_NOHIT) != 0;
            outline_rect(x - 20, y - 18, 40, 36, c, C_INK);
            gfx_rect(x - 14, y - 8, 10, 6, shut ? C_DUSK : C_YELLOW);
            gfx_rect(x + 4, y - 8, 10, 6, shut ? C_DUSK : C_YELLOW);
            gfx_rect(x - 10, y + 6, 20, 4, C_INK);
            if (shut) gfx_circb(x, y, 26, (t / 4) % 2 ? C_PINK : C_MAGENTA);
        }
        break;
    }
    case FO_HAL_LEG:
        outline_rect(x - 6, y - 12, 12, 24, fl ? C_WHITE : C_MAROON, C_INK);
        break;
    case FO_HAL_JOINT:
        gfx_circ(x, y, 6, fl ? C_WHITE : (f->flags & FF_NOHIT) ? C_DUSK : C_ORANGE);
        gfx_circb(x, y, 6, C_INK);
        break;
    default: break;
    }
}

/* ---- the suit ------------------------------------------------------------------------ */

static void draw_suit(int x, int y, int face, int grav, bool ground, bool crouch, int aim, int anim, const uint8_t *map) {
    int spr = SPR_SUIT0;
    if (crouch) spr = SPR_SUIT_DUCK;
    else if (!ground) spr = SPR_SUIT_JUMP;
    else if (aim == 1) spr = SPR_SUIT_UP;
    else if (anim) spr = (anim / 6) % 2 ? SPR_SUIT1 : SPR_SUIT2;
    int flags = (face < 0 ? SPR_FLIPX : 0) | (grav < 0 ? SPR_FLIPY : 0);
    if (map) spr_draw_ex(&apd_spr[spr], x - 6, y - 10, flags, map, -1);
    else spr_draw(&apd_spr[spr], x - 6, y - 10, flags);
}

static void draw_player(void) {
    const ApdPlayer *p = &ag.pl;
    int x = SX(APD_PX(p->x)), y = SY(APD_PX(p->y));
    if (ag.shadow.on) {
        uint8_t map[256];
        pal_identity(map);
        for (int c = 0; c < PAL_COUNT; c++) map[c] = c == C_INK ? C_DUSK : C_VIOLET;
        if ((ag.frame_t / 2) % 2)
            draw_suit(SX(APD_PX(ag.shadow.x)), SY(APD_PX(ag.shadow.y)), ag.shadow.face, ag.shadow.grav, false, false,
                      ag.shadow.aim, 0, map);
    }
    if (p->thunder_t > 0) {
        gfx_dither_circle(x, y, 16, C_CYAN, 6);
        gfx_circb(x, y, 16 + (ag.frame_t / 2) % 2, C_ICE);
    }
    if (p->stomp) {
        for (int k = 1; k < 4; k++) gfx_dither(x - 5, y - p->grav * k * 8 - 4, 10, 8, C_TAN, 8 - k * 2);
    }
    if (p->inv > 0 && (ag.frame_t / 3) % 2 && ag.state == AS_PLAY) return;
    draw_suit(x, y, p->face, p->grav, p->ground, p->crouch, p->firing ? p->aim : (p->aim == 1 ? 1 : 0), p->vx ? p->anim + 1 : 0,
              NULL);
    if (ag.drone.on) {
        int dx = SX(APD_PX(ag.drone.x)), dy = SY(APD_PX(ag.drone.y));
        gfx_circ(dx, dy, 3, C_LIME);
        gfx_circb(dx, dy, 3, C_INK);
        gfx_pset(dx, dy, C_WHITE);
    }
}

static void draw_shots(void) {
    for (int i = 0; i < APD_MAX_SHOTS; i++) {
        const ApdShot *s = &ag.shot[i];
        if (!s->alive) continue;
        int x = SX(APD_PX(s->x)), y = SY(APD_PX(s->y));
        if (s->kind == PS_WAVE) {
            gfx_dither(x - 5, y - 8, 10, 16, C_ORANGE, 10);
            gfx_vline(x, y - 7, y + 7, C_YELLOW);
            continue;
        }
        int c = s->hot ? C_ORANGE : s->zap ? C_CYAN : C_YELLOW;
        if (s->kind == PS_SHADOW) c = C_VIOLET;
        if (s->kind == PS_DRONE) c = C_LIME;
        gfx_rect(x - 2, y - 2, 4, 4, c);
        gfx_pset(x - 1, y - 1, C_WHITE);
    }
    for (int i = 0; i < APD_MAX_ESHOTS; i++) {
        const ApdEShot *e = &ag.es[i];
        if (!e->alive) continue;
        int x = SX(APD_PX(e->x)), y = SY(APD_PX(e->y));
        switch (e->kind) {
        case ES_STAR: gfx_pset(x, y, C_WHITE); gfx_circb(x, y, 2, C_ORANGE); break;
        case ES_BUBBLE: gfx_circ(x, y, 3, C_LIME); gfx_pset(x - 1, y - 1, C_WHITE); break;
        case ES_BEAM: {
            int d = e->vx > 0 ? 1 : -1;
            for (int k = -8; k <= 8; k++) gfx_pset(x + d * (4 - iabs(k) / 3), y + k, iabs(k) < 6 ? C_WHITE : C_PINK);
            break;
        }
        case ES_ORB: gfx_circ(x, y, 4, C_PINK); gfx_circb(x, y, 4, C_MAGENTA); break;
        case ES_RING: gfx_circb(x, y, 6, C_PINK); gfx_circb(x, y, 5, C_MAGENTA); break;
        case ES_BOLT:
            if (e->h > 20) {
                for (int k = 0; k < e->h; k += 2) gfx_pset(x + ((k / 2 + ag.frame_t) % 3) - 1, y - e->h / 2 + k, C_CYAN);
            } else { gfx_rect(x - 3, y - 2, 6, 4, C_CYAN); gfx_pset(x, y, C_WHITE); }
            break;
        case ES_LIGHTNING: {
            int top = SY(0), bot = SY(ag.rh * APD_TILE);
            if (e->t <= 40) {
                if ((e->t / 4) % 2) { gfx_vline(x - 8, top, bot, C_PINK); gfx_vline(x + 8, top, bot, C_PINK); }
            } else {
                for (int yy = top; yy < bot; yy += 2) gfx_hline(x - 7 + ((yy + ag.frame_t) % 5), x + 3 + ((yy * 3) % 5), yy, C_ICE);
                gfx_vline(x, top, bot, C_WHITE);
            }
            break;
        }
        case ES_SHELL: gfx_circ(x, y, 3, C_GREY); gfx_circb(x, y, 3, C_INK); break;
        case ES_SPARK: gfx_pset(x, y, C_WHITE); gfx_pset(x - 1, y, C_CYAN); gfx_pset(x + 1, y, C_CYAN); gfx_pset(x, y - 1, C_CYAN); gfx_pset(x, y + 1, C_CYAN); break;
        default: gfx_circ(x, y, 3, C_RED); gfx_pset(x, y, C_YELLOW); break;
        }
    }
    for (int i = 0; i < APD_MAX_PARTS; i++) {
        const ApdPart *p = &ag.part[i];
        if (p->life > 0) gfx_pset(SX(APD_PX(p->x)), SY(APD_PX(p->y)), p->col);
    }
}

/* ---- the bar along the top --------------------------------------------------------------- */

static void draw_hud(void) {
    gfx_rect(0, 0, SCREEN_W, APD_HUD, C_INK);
    gfx_hline(0, SCREEN_W - 1, APD_HUD - 1, C_DUSK);
    const ApdPlayer *p = &ag.pl;
    int mx = apd_max_armor();
    tiny_draw("ARMOR", 4, 3, C_LIGHT);
    int bw = 60;
    gfx_rect(26, 3, bw + 2, 6, C_DUSK);
    gfx_rect(27, 4, iclamp(p->armor * bw / imax(1, mx), 0, bw), 4, p->armor * 4 < mx ? C_RED : C_CYAN);
    char b[32];
    snprintf(b, sizeof b, "%d", imax(0, p->armor));
    tiny_draw(b, 92, 3, C_WHITE);
    /* spare bodies */
    tiny_draw("SPARE", 4, 11, C_LIGHT);
    for (int k = 0; k < ag.run.spares_max; k++) {
        int x = 28 + k * 7;
        gfx_rect(x, 11, 5, 6, k < ag.spares ? C_PINK : C_DUSK);
        gfx_pset(x + 2, 12, k < ag.spares ? C_WHITE : C_SLATE);
    }
    /* the three systems */
    static const char *const SYS3[3] = {"FLIP", "FRAME", "GUN"};
    for (int s = 0; s < 3; s++) {
        int x = 112 + s * 34;
        bool over = apd_overload(s + 1);
        tiny_draw(SYS3[s], x, 3, over && (ag.frame_t / 10) % 2 ? C_RED : C_GREY);
        int k = 0;
        for (int m = 0; m < APD_MODS; m++)
            if (ag.run.mod_sys[m] == s + 1) {
                gfx_rect(x + k * 6, 11, 5, 5, MOD_COL[m]);
                k++;
            }
    }
    if (ag.boss_on && ag.boss_foe >= 0) {
        /* the boss's health in the top corner */
        int hp = apd_boss_hp(), max = apd_boss_maxhp();
        tiny_draw(APD_BOSS_NAME[ag.boss], 216, 3, C_PINK);
        gfx_rect(216, 11, 100, 6, C_DUSK);
        gfx_rect(217, 12, iclamp(hp * 98 / imax(1, max), 0, 98), 4, hp * 4 < max ? C_RED : C_PINK);
    } else {
        tiny_draw(APD_SECTOR_NAME[apd_rooms[ag.room].sector], 216, 3, C_LIGHT);
        for (int k = 0; k < 4; k++) {
            int x = 216 + k * 8;
            gfx_rect(x, 11, 6, 6, (ag.run.ciphers >> k) & 1 ? C_PINK : C_DUSK);
            gfx_rectb(x, 11, 6, 6, C_INK);
        }
        if (ag.run.pass) {
            snprintf(b, sizeof b, "PASS %d", ag.run.pass);
            tiny_draw(b, 252, 11, C_AMBER);
        }
        if (ag.run.code_on) tiny_draw("CODE", 290, 11, C_LIME);
    }
}

/* ---- the play view ------------------------------------------------------------------------ */

static void draw_world(void) {
    int sec = apd_rooms[ag.room].sector;
    int sx = 0, sy = 0;
    if (ag.shake > 0) { sx = (ag.frame_t % 3) - 1; sy = ((ag.frame_t / 2) % 3) - 1; }
    ag.cam_x += sx;
    ag.cam_y += sy;
    gfx_clip(0, APD_HUD, SCREEN_W, SCREEN_H - APD_HUD);
    draw_background(sec);
    draw_things(false);
    draw_tiles(sec);
    for (int i = 0; i < APD_MAX_FOES; i++)
        if (ag.foe[i].alive && ag.foe[i].kind == FO_LAMP_SEG) draw_foe(i);
    for (int i = 0; i < APD_MAX_FOES; i++)
        if (ag.foe[i].alive && ag.foe[i].kind != FO_LAMP_SEG) draw_foe(i);
    if (ag.state != AS_DYING && ag.state != AS_OVER) draw_player();
    draw_acid(sec);
    draw_props(sec);
    draw_shots();
    /* the code, scratched under the first terminal: seen only coming from the left */
    if (ag.room == apd_room_find(SEC_GANTRY, 'S'))
        for (int i = 0; i < ag.nth; i++)
            if (ag.th[i].kind == TH_TERM && ag.th[i].id == 0) {
                int tx = ag.th[i].tx * APD_TILE + 8;
                int dx = tx - APD_PX(ag.pl.x);
                if (dx > 6 && dx < 44 && ag.pl.face > 0)
                    tiny_center(APD_CODE, SX(tx), SY(ag.th[i].ty * APD_TILE) - 12, C_DUSK);
            }
    gfx_noclip();
    ag.cam_x -= sx;
    ag.cam_y -= sy;
    draw_hud();
    if (ag.msg_t > 0 && ag.state == AS_PLAY) {
        int w = text_width(ag.msg) + 10;
        ui_panel(160 - w / 2, 26, w, 13, C_INK, C_PINK);
        text_center(ag.msg, 160, 29, C_WHITE);
    }
}

/* ---- the cradle ------------------------------------------------------------------------------ */

static void draw_pod_mods(void) {
    gfx_cls(C_INK);
    ui_panel(4, 4, 312, 172, C_INK, C_VIOLET);
    text_draw("CRADLE", 12, 9, C_PINK);
    text_draw(GLYPH_RIGHT " MAP", 270, 9, C_GREY);
    /* the suit, with its numbers beside it */
    spr_draw_scaled(&apd_spr[SPR_SUIT0], 18, 28, 3, 0);
    char b[48];
    snprintf(b, sizeof b, "MAP %d%%", apd_explore_pct());
    tiny_draw(b, 12, 96, C_LIGHT);
    snprintf(b, sizeof b, "ITEMS %d%%", apd_items_pct());
    tiny_draw(b, 12, 104, C_LIGHT);
    snprintf(b, sizeof b, "ARMOR %d", apd_max_armor());
    tiny_draw(b, 12, 112, C_LIGHT);
    snprintf(b, sizeof b, "SPARES %d", ag.run.spares_max);
    tiny_draw(b, 12, 120, C_LIGHT);
    snprintf(b, sizeof b, "DAMPERS %d", ag.run.dampers);
    tiny_draw(b, 12, 128, C_LIGHT);
    /* the grid: a column a mod, a row a system */
    static const char *const ROWS[4] = {"FLIP", "FRAME", "GUN", "OFF"};
    static const int SYS_AT[4] = {SYS_FLIP, SYS_FRAME, SYS_GUN, SYS_OFF};
    int gx = 100, gy = 34;
    for (int r = 0; r < 4; r++) {
        int y = gy + r * 22;
        bool over = r < 3 && apd_overload(SYS_AT[r]);
        tiny_draw(ROWS[r], gx, y + 6, over ? C_RED : C_GREY);
        gfx_hline(gx + 28, gx + 208, y + 18, C_DUSK);
    }
    uint8_t map[256];
    for (int m = 0; m < APD_MODS; m++) {
        int x = gx + 40 + m * 44;
        tiny_center(ag.run.mod_sys[m] == MOD_NONE ? "?" : APD_MOD_NAME[m], x + 6, gy - 8, m == ag.pod_mod ? C_WHITE : C_GREY);
        if (m == ag.pod_mod) gfx_rectb(x - 6, gy - 2, 24, 4 * 22, (ag.frame_t / 8) % 2 ? C_PINK : C_VIOLET);
        if (ag.run.mod_sys[m] == MOD_NONE) continue;
        int r = 0;
        for (int k = 0; k < 4; k++)
            if (SYS_AT[k] == ag.run.mod_sys[m]) r = k;
        mod_remap(map, m);
        spr_draw_ex(&apd_spr[SPR_MOD], x, gy + r * 22 + 3, 0, map, -1);
    }
    /* what the chosen mod does where it is */
    int m = ag.pod_mod;
    if (ag.run.mod_sys[m] != MOD_NONE) {
        int s = ag.run.mod_sys[m];
        if (s == SYS_OFF) snprintf(b, sizeof b, "%s: NOT FITTED", APD_MOD_NAME[m]);
        else snprintf(b, sizeof b, "%s IN THE %s: %s", APD_MOD_NAME[m], APD_SYS_NAME[s], APD_EFFECT_NAME[m][s]);
        text_draw(b, 100, 126, MOD_COL[m]);
    } else text_draw("NOT FOUND YET", 100, 126, C_GREY);
    snprintf(b, sizeof b, "SAFE: %d A SYSTEM", apd_safe_slots());
    tiny_draw(b, 100, 140, C_LIGHT);
    bool any = apd_overload(SYS_FLIP) || apd_overload(SYS_FRAME) || apd_overload(SYS_GUN);
    if (any) tiny_draw("OVERLOAD!", 200, 140, (ag.frame_t / 10) % 2 ? C_RED : C_ORANGE);
    text_draw(GLYPH_LEFT GLYPH_RIGHT " MOD  " GLYPH_UP GLYPH_DOWN " SYSTEM  " GLYPH_B " LEAVE", 100, 160, C_GREY);
}

static void draw_pod_map(void) {
    gfx_cls(C_INK);
    ui_panel(4, 4, 312, 172, C_INK, C_VIOLET);
    int s = ag.pod_sector;
    char b[48];
    snprintf(b, sizeof b, GLYPH_UP " %s " GLYPH_DOWN, APD_SECTOR_NAME[s]);
    text_center(b, 160, 9, C_PINK);
    text_draw(GLYPH_LEFT " MODS", 12, 9, C_GREY);
    const int CW = 30, CH = 18;
    int gw = 0, gh = 0;
    for (int i = 0; i < apd_nrooms; i++)
        if (apd_rooms[i].sector == s) {
            gw = imax(gw, apd_rooms[i].gx + apd_rooms[i].w);
            gh = imax(gh, apd_rooms[i].gy + apd_rooms[i].h);
        }
    int ox = 160 - gw * CW / 2, oy = 28 + (7 - gh) * CH / 2;
    for (int i = 0; i < apd_nrooms; i++) {
        const ApdRoom *r = &apd_rooms[i];
        if (r->sector != s || !ag.run.visited[i]) continue;
        int x = ox + r->gx * CW, y = oy + r->gy * CH, w = r->w * CW, h = r->h * CH;
        bool here = i == ag.room;
        gfx_rect(x + 1, y + 1, w - 2, h - 2, here && (ag.frame_t / 10) % 2 ? C_PINK : C_NAVY);
        gfx_rectb(x + 1, y + 1, w - 2, h - 2, C_SKY);
        const ApdSpec *sp = &APD_SPECS[r->spec];
        char mark[4] = {0};
        if (strstr(sp->things, "pod")) mark[0] = 'S';
        else if (strstr(sp->things, "tele")) mark[0] = 'T';
        else {
            for (int k = 0; k < r->nitems; k++)
                if (!ag.run.item_got[r->first_item + k]) mark[0] = 'E';
        }
        if (mark[0]) tiny_center(mark, x + w / 2, y + h / 2 - 3, C_WHITE);
        if (strstr(sp->things, "log") || strstr(sp->things, "pass")) gfx_rect(x + w - 6, y + h - 6, 2, 2, C_YELLOW);
    }
    /* doors between rooms seen */
    for (int d = 0; d < apd_ndoors; d++) {
        const ApdDoor *dd = &apd_doors[d];
        if (dd->sector != s || (!ag.run.visited[dd->ra] && !ag.run.visited[dd->rb])) continue;
        bool pass = dd->type >= E_PASS1 && dd->type <= E_PASS3;
        int col = pass ? C_AMBER : C_YELLOW;
        if (pass && ag.run.pass >= dd->type - E_PASS1 + 1 && (ag.frame_t / 8) % 2) col = C_WHITE;
        if (dd->type == E_HIDDEN && !ag.run.gate_open[d]) continue;
        if (!dd->vertical) {
            int x = ox + (dd->ax + 1) * CW, y = oy + dd->ay * CH + CH / 2;
            gfx_rect(x - 1 - pass, y - 2, 2 + pass * 2, 4, col);
        } else {
            int x = ox + dd->ax * CW + CW / 2, y = oy + (dd->ay + 1) * CH;
            gfx_rect(x - 2, y - 1 - pass, 4, 2 + pass * 2, col);
        }
    }
    snprintf(b, sizeof b, "MAP %d%%   ITEMS %d%%", apd_explore_pct(), apd_items_pct());
    tiny_center(b, 160, 162, C_LIGHT);
}

static void draw_text_box(void) {
    ui_panel(20, 40, 280, 100, C_INK, C_JADE);
    if (ag.text_id >= 100) {
        char b[40];
        snprintf(b, sizeof b, "PASS LEVEL %d GRANTED", ag.text_id - 100);
        text_center(b, 160, 70, C_AMBER);
        snprintf(b, sizeof b, "DOORS MARKED %d NOW OPEN", ag.text_id - 100);
        text_center(b, 160, 90, C_LIGHT);
        return;
    }
    int id = iclamp(ag.text_id, 0, APD_NLOGS - 1);
    text_draw(APD_LOG[id][0], 30, 48, C_LIME);
    for (int k = 1; k < 6; k++) text_draw(APD_LOG[id][k], 30, 56 + k * 12, C_JADE);
}

/* ---- the title, the story, the end --------------------------------------------------------- */

static void draw_title(void) {
    gfx_cls(C_INK);
    int t = ag.frame_t;
    /* a floor and a ceiling, a suit on each */
    gfx_rect(0, 0, SCREEN_W, 18, C_SLATE);
    gfx_hline(0, SCREEN_W - 1, 17, C_GREY);
    gfx_rect(0, 162, SCREEN_W, 18, C_SLATE);
    gfx_hline(0, SCREEN_W - 1, 162, C_GREY);
    for (int x = (t / 2) % 32; x < SCREEN_W; x += 32) { gfx_vline(x, 0, 16, C_DUSK); gfx_vline(x, 163, 179, C_DUSK); }
    spr_draw_scaled(&apd_spr[SPR_SUIT0], 30, 122, 2, 0);
    spr_draw_scaled(&apd_spr[SPR_SUIT0], 266, 18, 2, SPR_FLIPX | SPR_FLIPY);
    static const uint8_t grad[3] = {C_PINK, C_MAGENTA, C_VIOLET};
    ui_fancy_center("ANTIPODE", 160, 34, 3, grad, 3, C_INK, C_NAVY);
    static const char *const ITEMS[3] = {"NEW GAME", "CONTINUE", "CODE"};
    for (int k = 0; k < 3; k++) {
        int y = 82 + k * 13;
        int col = k == ag.menu ? C_WHITE : C_GREY;
        if (k == 1 && !ags.has_run) col = C_DUSK;
        text_center(ITEMS[k], 160, y, col);
        if (k == ag.menu) ui_cursor(160 - text_width(ITEMS[k]) / 2 - 12, y, t);
    }
    char b[64];
    snprintf(b, sizeof b, "UPGRADES %d/%d   EXPLORATION %d%%   TIMES DIED %u", ags.best_upgrades, APD_UPGRADES,
             ags.best_explore, (unsigned)ags.times_died);
    tiny_center(b, 160, 126, C_LIGHT);
    tiny_center("BEAMDOWN SOFTWORKS 1987", 160, 150, C_DUSK);
}

static void draw_code(void) {
    gfx_cls(C_INK);
    text_center("CODE", 160, 40, C_PINK);
    int x0 = 160 - 9 * 12 / 2;
    for (int k = 0; k < 9; k++) {
        char s[2] = {ag.code[k], 0};
        int x = x0 + k * 12;
        text_draw(s, x + 3, 80, k == ag.code_pos ? C_WHITE : C_GREY);
        if (k == ag.code_pos && (ag.frame_t / 8) % 2) gfx_hline(x + 1, x + 9, 90, C_PINK);
    }
    text_center(GLYPH_UP GLYPH_DOWN " LETTER  " GLYPH_LEFT GLYPH_RIGHT " MOVE  " GLYPH_A " ENTER  " GLYPH_B " BACK", 160, 120, C_GREY);
}

static void draw_lines(const char *const *lines, int n, int y0, int step, int col) {
    for (int k = 0; k < n; k++)
        if (lines[k][0]) text_center(lines[k], 160, y0 + k * step, col);
}

static void draw_intro(void) {
    gfx_cls(C_INK);
    int shown = imin(APD_NINTRO, ag.state_t / 40 + 1);
    draw_lines(APD_INTRO, shown, 18, 11, C_LIGHT);
}

static void draw_ending(void) {
    gfx_cls(C_INK);
    const char *const *lines = ag.cherry_run ? APD_END_CHERRY : APD_END_GOLD;
    int n = ag.cherry_run ? APD_NEND_CHERRY : APD_NEND_GOLD;
    int shown = imin(n, ag.state_t / 60 + 1);
    draw_lines(lines, shown, 8, 10, C_LIGHT);
    /* the suit: its visor lifts only for the one who saw everything */
    spr_draw_scaled(&apd_spr[SPR_SUIT0], 148, 140 - 20, 2, 0);
    if (ag.cherry_run && ag.state_t > 360) {
        gfx_rect(152, 124, 14, 10, C_TAN);
        gfx_rect(155, 127, 2, 2, C_INK);
        gfx_rect(161, 127, 2, 2, C_INK);
        gfx_rect(150, 120, 18, 5, C_BROWN);
    }
}

static void draw_credits(void) {
    gfx_cls(C_INK);
    int y0 = 190 - ag.state_t / 2;
    for (int k = 0; k < APD_NCREDITS; k++) {
        int y = y0 + k * 14;
        if (y < -10 || y > SCREEN_H) continue;
        const char *s = APD_CREDITS[k];
        if (!strcmp(s, "???") && ag.cherry_run) s = "DR. SELA AMBRY";
        bool head = k == 0 || (!APD_CREDITS[k - 1][0] && s[0]);
        text_center(s, 160, y, head ? C_PINK : C_LIGHT);
    }
}

void apd_draw(void) {
    switch (ag.state) {
    case AS_TITLE: draw_title(); break;
    case AS_CODE: draw_code(); break;
    case AS_INTRO: draw_intro(); break;
    case AS_POD:
        if (ag.pod_page == 0) draw_pod_mods();
        else draw_pod_map();
        break;
    case AS_TEXT: draw_world(); draw_text_box(); break;
    case AS_TELE:
        draw_world();
        gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_WHITE, iclamp(16 - iabs(ag.state_t - 30) / 2, 0, 16));
        break;
    case AS_DYING: draw_world(); break;
    case AS_OVER:
        draw_world();
        gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, 12);
        text_center("SIGNAL LOST", 160, 70, C_RED);
        text_center(ag.run.code_on || !ags.has_run ? GLYPH_A " START AGAIN" : GLYPH_A " BACK TO THE LAST CRADLE", 160, 96, C_LIGHT);
        text_center(GLYPH_B " TITLE", 160, 110, C_GREY);
        break;
    case AS_ENDING: draw_ending(); break;
    case AS_CREDITS: draw_credits(); break;
    default: draw_world(); break;
    }
}

void apd_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_INK);
    gfx_rect(x, y, w, 6, C_SLATE);
    gfx_rect(x, y + h - 6, w, 6, C_SLATE);
    for (int k = 0; k < 6; k++) gfx_pset(x + (k * 37 + t / 3) % w, y + 10 + (k * 13) % (h - 20), C_DUSK);
    spr_draw(&apd_spr[SPR_SUIT0], x + 10, y + h - 26, 0);
    spr_draw(&apd_spr[SPR_SUIT0], x + w - 22, y + 6, SPR_FLIPX | SPR_FLIPY);
    static const uint8_t grad[2] = {C_PINK, C_MAGENTA};
    ui_fancy_text("ANTIPODE", x + 6, y + h / 2 - 6, 1, grad, 2, C_INK, C_NAVY);
    gfx_noclip();
}

/* tests: the screens' words stay inside the screen and clear of each other */
int apd_layout_audit(void) {
    int bad = 0;
    ApdGame keep = ag;
    static const int SCREENS[] = {AS_TITLE, AS_CODE, AS_INTRO, AS_ENDING};
    for (int i = 0; i < ARRAY_LEN(SCREENS); i++) {
        ag.state = SCREENS[i];
        ag.state_t = 5000;
        ui_audit_begin("antipode", true);
        ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
        if (SCREENS[i] == AS_INTRO)
            for (int k = 0; k < APD_NINTRO; k++)
                if (APD_INTRO[k][0]) ui_audit_text("story", APD_INTRO[k], 160 - text_width(APD_INTRO[k]) / 2, 18 + k * 11);
        if (SCREENS[i] == AS_ENDING) {
            for (int c = 0; c < 2; c++) {
                const char *const *L = c ? APD_END_CHERRY : APD_END_GOLD;
                int n = c ? APD_NEND_CHERRY : APD_NEND_GOLD;
                for (int k = 0; k < n; k++)
                    if (L[k][0] && (int)(8 + k * 10 + 9) > 118) bad++; /* clear of the suit */
                for (int k = 0; k < n; k++)
                    if (L[k][0] && text_width(L[k]) > SCREEN_W - 8) bad++;
            }
        }
        bad += ui_audit_end();
    }
    for (int k = 0; k < APD_NINTRO; k++) bad += text_missing(APD_INTRO[k]) > 0;
    for (int l = 0; l < APD_NLOGS; l++)
        for (int k = 0; k < 6; k++) {
            bad += text_missing(APD_LOG[l][k]) > 0;
            if (text_width(APD_LOG[l][k]) > 260) { bad++; fprintf(stderr, "log %d line %d too wide\n", l, k); }
        }
    for (int k = 0; k < APD_NCREDITS; k++) bad += text_missing(APD_CREDITS[k]) > 0;
    ag = keep;
    return bad;
}
