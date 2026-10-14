/* SHOWSTOPPER - drawing: the screens, the stages' tiles, Clover and the
 * foes, the bosses, the HUD and the Hop's board. */
#include "sst.h"

int sst_menu_rows(void);

/* words that report themselves to the layout audit as they are drawn */
static void say(const char *s, int x, int y, int col) {
    text_draw(s, x, y, col);
    ui_audit_text(s, s, x, y);
}
static void say_c(const char *s, int cx, int y, int col) { say(s, cx - text_width(s) / 2, y, col); }
static void say_cs(const char *s, int cx, int y, int col, int sh) {
    int x = cx - text_width(s) / 2;
    text_shadow(s, x, y, col, sh);
    ui_audit_text(s, s, x, y);
}
static void tsay(const char *s, int x, int y, int col) {
    tiny_draw(s, x, y, col);
    ui_audit_tiny(s, s, x, y);
}
static void tsay_c(const char *s, int cx, int y, int col) { tsay(s, cx - tiny_width(s) / 2, y, col); }

static const uint8_t GRAD_SHOW[4] = {C_YELLOW, C_AMBER, C_ORANGE, C_RED};
static const uint8_t GRAD_ICE[3] = {C_WHITE, C_ICE, C_SKY};

const char *const SST_STORY[] = {
    "MORROW THE MARVELLOUS RUNS THE",
    "GRANDEST TRAVELLING SHOW IN THE",
    "NORTH, AND KEEPS HIS ANIMALS",
    "LOCKED IN CAGES.",
    "",
    "ONE SNOWY NIGHT CLOVER, HIS RABBIT,",
    "SLIPS OUT OF HIS HAT WITH HIS WAND.",
    "",
    "THE OTHERS ARE STILL LOCKED UP.",
    "SHE IS GOING BACK FOR THEM.",
};
const int SST_STORY_N = ARRAY_LEN(SST_STORY);

static const char *const STAGE_NUM[SST_STAGES] = {"I", "II", "III", "IV", "V"};

/* ---- backdrops -------------------------------------------------------------------- */

static void tent_backdrop(int t) {
    gfx_cls(C_NIGHT);
    for (int x = -40; x < SCREEN_W + 40; x += 40) {
        int sx = x + (t / 3) % 40;
        for (int y = 0; y < SCREEN_H; y += 2) {
            int w = 10 + y / 12;
            gfx_hline(sx - w / 2 + (y * (sx - 160)) / 400, sx + w / 2 + (y * (sx - 160)) / 400, y, C_MAROON);
        }
    }
    for (int k = 0; k < 24; k++) {
        int x = (k * 53 + t / 4) % SCREEN_W, y = (k * 37) % 70;
        gfx_pset(x, y, (k + t / 20) % 3 ? C_CREAM : C_YELLOW);
    }
    gfx_rect(0, 160, SCREEN_W, 20, C_INK);
}

static void draw_clover_at(int x, int y, int face, int frame, int flags);

/* ---- the title ---------------------------------------------------------------------- */

static void draw_title(void) {
    int t = sst_g.frame_t;
    tent_backdrop(t);
    ui_fancy_center("SHOWSTOPPER", 160, 20, 3, GRAD_SHOW, 4, C_INK, C_MAROON);
    ui_audit_fancy("logo", "SHOWSTOPPER", 160 - ui_fancy_width("SHOWSTOPPER", 3) / 2, 20, 3, true);
    say_cs("THE RABBIT AND THE MAGICIAN", 160, 50, C_ICE, C_INK);
    draw_clover_at(60, 104, (t / 40) % 2 ? 2 : 0, (t / 10) % 2, 0);
    spr_draw(&sst_spr[SP_OPT_A], 40, 120 + (t / 12) % 2, 0);
    spr_draw(&sst_spr[(t / 16) % 2 ? SP_BLOB : SP_BLOB2], 252, 112, 0);
    static const char *const ITEMS[3] = {"1 PLAYER", "2 PLAYERS", "FREE RIDE"};
    int rows = sst_menu_rows();
    ui_panel(108, 66, 104, 18 + rows * 12, C_NIGHT, C_YELLOW);
    for (int i = 0; i < rows; i++) {
        int yy = 72 + i * 12;
        bool s = i == sst_g.menu;
        int col = s ? C_WHITE : C_GREY;
        if (i == 1) col = s ? C_GREY : C_SLATE; /* two players: the next update */
        say_c(ITEMS[i], 160, yy, col);
        if (s) ui_cursor(160 - text_width(ITEMS[i]) / 2 - 10, yy, t);
    }
    char b[80];
    snprintf(b, sizeof b, "PASSES FOUND %d/%d", sst_sv.most_passes, SST_PASSES);
    tsay_c(b, 160, 150, C_LIGHT);
    if (sst_g.menu == 1) tsay_c("2 PLAYERS " GLYPH_DOT " NEXT UPDATE", 160, 168, C_LIGHT);
    else tsay_c("1988 BEAMDOWN SOFTWORKS", 160, 168, C_SLATE);
}

static void draw_story(void) {
    gfx_cls(C_INK);
    for (int k = 0; k < 40; k++) gfx_pset((k * 71) % SCREEN_W, (k * 43) % SCREEN_H, C_DUSK);
    int shown = imin(SST_STORY_N, 1 + sst_g.state_t / 50);
    for (int k = 0; k < shown; k++)
        if (SST_STORY[k][0]) say_c(SST_STORY[k], 160, 22 + k * 12, C_CREAM);
    tsay_c("A: ON", 160, 166, C_SLATE);
}

/* ---- the map between stages ------------------------------------------------------- */

static const int16_t MAP_PT[SST_STAGES][2] = {{40, 128}, {96, 96}, {150, 120}, {210, 84}, {270, 44}};

static void draw_map(void) {
    int t = sst_g.frame_t;
    gfx_cls(C_NAVY);
    for (int y = 100; y < SCREEN_H; y += 2) gfx_hline(0, SCREEN_W - 1, y, C_WHITE);
    for (int k = 0; k < 30; k++) gfx_pset((k * 67) % SCREEN_W, (k * 29) % 90, C_ICE);
    /* the hills and the tower */
    gfx_circ(80, 150, 60, C_ICE);
    gfx_circ(230, 160, 70, C_ICE);
    gfx_rect(262, 20, 16, 40, C_PURPLE);
    gfx_rect(258, 16, 24, 6, C_VIOLET);
    for (int i = 0; i + 1 < SST_STAGES; i++) {
        int x0 = MAP_PT[i][0], y0 = MAP_PT[i][1], x1 = MAP_PT[i + 1][0], y1 = MAP_PT[i + 1][1];
        for (int k = 0; k <= 12; k++)
            if (k % 2 == 0) gfx_rect(x0 + (x1 - x0) * k / 12 - 1, y0 + (y1 - y0) * k / 12 - 1, 3, 3, C_BROWN);
    }
    for (int i = 0; i < SST_STAGES; i++) {
        int x = MAP_PT[i][0], y = MAP_PT[i][1];
        int col = i < sst_g.stage ? C_YELLOW : i == sst_g.stage ? C_RED : C_SLATE;
        gfx_circ(x, y, 7, C_INK);
        gfx_circ(x, y, 5, col);
        tiny_center(STAGE_NUM[i], x, y - 2, C_INK);
    }
    int s = iclamp(sst_g.stage, 0, SST_STAGES - 1);
    int bob = (t / 10) % 2;
    draw_clover_at(MAP_PT[s][0] - 7, MAP_PT[s][1] - 26 - bob, 2, 0, 0);
    char b[64];
    snprintf(b, sizeof b, "STAGE %s", STAGE_NUM[s]);
    ui_panel(96, 136, 128, 34, C_NIGHT, C_YELLOW);
    say_c(b, 160, 140, C_YELLOW);
    say_c(SST_STAGE[s].name, 160, 152, C_WHITE);
    snprintf(b, sizeof b, "LIVES %d   PASSES %d", imax(0, sst_g.lives - 1), sst_g.passes);
    tsay(b, 4, 4, C_WHITE);
}

/* ---- the world's tiles -------------------------------------------------------------- */

typedef struct { uint8_t floor, floor2, path, wall, wall2, top, tree, roof, pit, ice; } Theme;
static const Theme THEMES[5] = {
    {C_WHITE, C_ICE, C_LIGHT, C_SLATE, C_GREY, C_WHITE, C_FOREST, C_MAROON, C_INK, C_ICE},
    {C_BROWN, C_EARTH, C_TAN, C_SLATE, C_GREY, C_LIGHT, C_FOREST, C_MAROON, C_NIGHT, C_ICE},
    {C_TAN, C_HIDE, C_GREY, C_BROWN, C_EARTH, C_WHITE, C_FOREST, C_WINE, C_INK, C_ICE},
    {C_PURPLE, C_VIOLET, C_MAGENTA, C_NAVY, C_BLUE, C_SKY, C_LIGHT, C_WINE, C_INK, C_ICE},
    {C_GREY, C_SLATE, C_LIGHT, C_DUSK, C_SLATE, C_GREY, C_LIGHT, C_WINE, C_INK, C_ICE},
};

static int view_y(int wy) { return SST_HUD + wy - sst_g.cam_y; }

static void draw_tile(int tx, int ty, int sx, int sy) {
    const Theme *th = &THEMES[SST_STAGE[sst_g.stage].theme];
    int t = sst_tile_at(tx, ty);
    bool crown = sst_g.stage == 4 && sst_g.half == 1;
    int f1 = crown ? C_WINE : th->floor, f2 = crown ? C_MAROON : th->floor2;
    switch (t) {
    case T_FLOOR:
    case T_BRIDGE:
        gfx_rect(sx, sy, 16, 16, f1);
        if (t == T_BRIDGE) {
            gfx_rect(sx, sy, 16, 16, C_BROWN);
            for (int k = 0; k < 16; k += 4) gfx_hline(sx, sx + 15, sy + k, C_EARTH);
        } else if (sst_g.stage == 3) {
            if ((tx + ty) % 2) gfx_rect(sx, sy, 16, 16, f2);
        } else if (sst_g.stage == 1) {
            for (int k = 0; k < 16; k += 4) gfx_vline(sx + k, sy, sy + 15, f2);
        } else if (sst_g.stage == 4) {
            gfx_rectb(sx, sy, 16, 16, f2);
            gfx_pset(sx + 2, sy + 2, C_LIGHT);
        } else {
            if ((tx * 7 + ty * 3) % 5 == 0) gfx_pset(sx + (tx * 5) % 13 + 1, sy + (ty * 7) % 13 + 1, f2);
            if ((tx * 3 + ty * 5) % 7 == 0) gfx_hline(sx + 3, sx + 6, sy + 9, f2);
        }
        break;
    case T_PATH:
        gfx_rect(sx, sy, 16, 16, th->path);
        gfx_rectb(sx, sy, 8, 8, f2);
        gfx_rectb(sx + 8, sy + 8, 8, 8, f2);
        break;
    case T_ICE:
        gfx_rect(sx, sy, 16, 16, th->ice);
        gfx_line(sx + 2, sy + 12, sx + 9, sy + 5, C_WHITE);
        gfx_pset(sx + 12, sy + 3, C_WHITE);
        break;
    case T_PIT:
    case T_WATER: {
        bool water = t == T_WATER;
        gfx_rect(sx, sy, 16, 16, water ? C_NAVY : th->pit);
        if (sst_g.stage == 1 || sst_g.stage == 4) {
            /* the ground (or the shaft) rushing by */
            int sh = sst_g.stage == 1 ? (sst_g.frame_t * 6) % 32 : (sst_g.frame_t * 2) % 16;
            if (sst_g.stage == 1)
                for (int k = 0; k < 2; k++) gfx_hline(sx + (k * 13 + 32 - sh) % 16, sx + (k * 13 + 32 - sh) % 16 + 3, sy + 5 + k * 6, C_DUSK);
            else
                gfx_hline(sx + 4, sx + 11, sy + (16 - sh) % 16, C_NIGHT);
        } else if (water) {
            int w = (sst_g.frame_t / 12 + tx + ty) % 4;
            gfx_hline(sx + 2 + w, sx + 7 + w, sy + 6, C_BLUE);
        } else {
            /* the lip: a darker edge where floor meets the drop */
            if (!sst_tile_pit(sst_tile_at(tx, ty - 1))) gfx_rect(sx, sy, 16, 3, C_DUSK);
        }
        break;
    }
    case T_TREE:
        gfx_rect(sx, sy, 16, 16, f1);
        gfx_circ(sx + 8, sy + 8, 7, th->tree);
        gfx_circ(sx + 6, sy + 6, 3, sst_g.stage == 0 ? C_WHITE : sst_g.stage == 3 ? C_WHITE : C_JADE);
        gfx_circb(sx + 8, sy + 8, 7, C_INK);
        break;
    default: {
        /* walls, roofs, windows, the walls guns sit in */
        uint8_t wc = t == T_ROOF ? th->roof : th->wall;
        gfx_rect(sx, sy, 16, 16, wc);
        if (t == T_ROOF) {
            for (int k = 0; k < 16; k += 4) gfx_vline(sx + k, sy, sy + 15, PAL_DARKER[wc]);
            gfx_rect(sx, sy + 13, 16, 3, sst_g.stage == 0 ? C_WHITE : C_CREAM);
        } else if (t == T_ROOFTOP) {
            gfx_rect(sx, sy, 16, 16, th->roof);
            gfx_hline(sx, sx + 15, sy + 8, PAL_DARKER[th->roof]);
        } else {
            for (int k = 4; k < 16; k += 5) gfx_hline(sx, sx + 15, sy + k, th->wall2);
            gfx_vline(sx + ((ty % 2) ? 4 : 11), sy, sy + 15, th->wall2);
            if (sst_g.stage == 0 && !sst_tile_solid(sst_tile_at(tx, ty - 1))) gfx_rect(sx, sy, 16, 3, C_WHITE);
            if (t == T_WINDOW) {
                gfx_rect(sx + 3, sy + 3, 10, 9, C_INK);
                gfx_rect(sx + 4, sy + 4, 8, 7, (sst_g.frame_t / 40 + tx) % 5 ? C_AMBER : C_YELLOW);
                gfx_vline(sx + 8, sy + 4, sy + 10, C_INK);
            }
            if (t == T_GUNWALL) gfx_rect(sx + 4, sy + 4, 8, 8, PAL_DARKER[wc]);
        }
        break;
    }
    }
}

static void draw_tiles(void) {
    int ty0 = sst_g.cam_y >> 4, ty1 = (sst_g.cam_y + SST_VIEW_H) >> 4;
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = 0; tx < SST_COLS; tx++) draw_tile(tx, ty, tx * 16, view_y(ty * 16));
}

/* ---- Clover and company -------------------------------------------------------------------- */

static void draw_clover_at(int x, int y, int face, int frame, int flags) {
    int id;
    int fl = flags;
    switch (face) {
    case 0: id = frame ? SP_CLOVER_S1 : SP_CLOVER_S0; break;
    case 4: id = frame ? SP_CLOVER_S1 : SP_CLOVER_S0; fl |= SPR_FLIPX; break;
    case 5: case 6: case 7: id = frame ? SP_CLOVER_U1 : SP_CLOVER_U0; break;
    default: id = frame ? SP_CLOVER_D1 : SP_CLOVER_D0; break;
    }
    spr_draw(&sst_spr[id], x, y, fl);
}

static void draw_player(void) {
    const SstPlayer *p = &sst_g.pl;
    int x = SST_PX(p->x), y = view_y(SST_PX(p->y));
    if (p->state == PS_GONE) return;
    /* the helper and the spike balls */
    if (p->pow[CC_OPTION]) spr_draw(&sst_spr[p->pow[CC_OPTION] == 2 ? SP_OPT_B : SP_OPT_A], SST_PX(p->opt_x) - 4, view_y(SST_PX(p->opt_y)) - 4, 0);
    if (p->pow[CC_SHIELD] && p->state != PS_BUBBLE) {
        int n = p->pow[CC_SHIELD] == 2 ? 5 : 1;
        for (int k = 0; k < n; k++) {
            if (p->shield[k] <= 0) continue;
            int a = p->shield_ang + k * SST_ANG / n;
            spr_draw(&sst_spr[SP_SPIKEBALL], x + 18 * sst_cos(a) / 256 - 3, y + 18 * sst_sin(a) / 256 - 3, 0);
        }
    }
    if (p->state == PS_FALL) {
        int r = imax(1, 7 - p->st / 4);
        gfx_circ(x, y, r, C_WHITE);
        gfx_circb(x, y, r, C_INK);
        return;
    }
    if (p->state == PS_DOWN) {
        if ((p->st / 3) % 2) draw_clover_at(x - 7, y - 10, 2, 0, 0);
        return;
    }
    if (p->inv > 0 && p->state == PS_WALK && (p->inv / 3) % 2) return;
    if (p->state == PS_DASH) {
        spr_draw(&sst_spr[SP_CLOVER_DASH], x - 7, y - 10, SST_DX8[p->dash_dir] < 0 ? SPR_FLIPX : 0);
    } else {
        draw_clover_at(x - 7, y - 10, p->face, (p->step / 8) % 2, 0);
    }
    if (p->state == PS_BUBBLE) {
        spr_draw(&sst_spr[SP_BUBBLE], x - 9, y - 11, 0);
        if (p->bub_t < 90 && (p->bub_t / 6) % 2) gfx_circb(x, y - 2, 10, C_WHITE);
    }
    /* the aim, while it is locked */
    if (p->firing && p->state == PS_WALK) {
        int a = p->face * 8;
        gfx_pset(x + 12 * sst_cos(a) / 256, y - 2 + 12 * sst_sin(a) / 256, C_YELLOW);
    }
}

static int foe_sprite(const SstFoe *f) {
    switch (f->kind) {
    case FK_CRATE: return SP_CRATE;
    case FK_BARREL: return SP_BARREL;
    case FK_GUMDROP: return SP_GUMDROP;
    case FK_LANTERN: return SP_LANTERN;
    case FK_JESTER: return SP_JESTER;
    case FK_BAT: return SP_BAT;
    case FK_SNOWBALL: return SP_SNOWBALL;
    case FK_GOURD: return SP_GOURD;
    case FK_PUFFER: return SP_PUFFER;
    case FK_TOYTRAIN: return SP_TOYTRAIN;
    case FK_BOMBER: return SP_BOMBER;
    case FK_SNOWMAN: return SP_SNOWMAN;
    case FK_ROCKETEER: return SP_ROCKETEER;
    case FK_COPTER: return SP_COPTER;
    case FK_TURTLE: return SP_TURTLE;
    case FK_HARLEQUIN: return SP_HARLEQUIN;
    case FK_SKULLJET: return SP_SKULLJET;
    case FK_EMBER: return SP_EMBER;
    case FK_BOOK: return SP_BOOK;
    case FK_IMP: return SP_IMP;
    case FK_RACECAR: return SP_RACECAR;
    case FK_TORCH: return SP_TORCHF;
    case FK_MOUSE: return SP_MOUSE;
    case FK_SKULL: return SP_SKULL;
    case FK_EGG: return SP_EGG;
    case FK_WISP: return SP_EMBER;
    case FK_SWOOP: return SP_SWOOP;
    case FK_SPIKE: return SP_SPIKE;
    case FK_SECRET:
        return sst_g.stage == 0 ? SP_SECRET_SNOW : sst_g.stage == 1 ? SP_SECRET_TRAIN : sst_g.stage == 2 ? SP_SECRET_TOWN : SP_SECRET_PALACE;
    default: return -1;
    }
}

static void draw_foes(void) {
    for (int i = 0; i < SST_MAX_FOES; i++) {
        const SstFoe *f = &sst_g.foe[i];
        if (!f->alive) continue;
        int x = SST_PX(f->x), y = view_y(SST_PX(f->y));
        if (f->kind == FK_BAT && f->a == 0) continue; /* behind its window */
        if (f->kind == FK_GUN || f->kind == FK_GUN3) {
            gfx_rect(x - 6, y - 6, 12, 12, C_INK);
            gfx_rect(x - 5, y - 5, 10, 10, f->hurt ? C_WHITE : C_GREY);
            int a = sst_player_ang(f->x, f->y);
            gfx_line(x, y, x + 8 * sst_cos(a) / 256, y + 8 * sst_sin(a) / 256, C_INK);
            if (f->kind == FK_GUN3) gfx_rect(x - 2, y - 2, 4, 4, C_RED);
            continue;
        }
        int id = foe_sprite(f);
        if (id < 0) continue;
        const Sprite *s = &sst_spr[id];
        int flags = 0;
        if ((f->kind == FK_RACECAR || f->kind == FK_TOYTRAIN || f->kind == FK_BOOK) && f->dir < 0) flags |= SPR_FLIPX;
        int lift = (f->kind == FK_SNOWBALL || f->kind == FK_SKULL) ? f->a : 0;
        if (lift) gfx_dither(x - 4, y + 3, 8, 3, C_INK, 8);
        if (f->kind == FK_EMBER || f->kind == FK_WISP) {
            int sz = f->kind == FK_WISP ? 1 : f->size;
            if (sz < 3) {
                gfx_circ(x, y, 1 + sz * 2, C_RED);
                gfx_circ(x, y, sz, C_YELLOW);
                continue;
            }
        }
        if (f->hurt && (f->hurt / 2) % 2) spr_draw_ex(s, x - s->w / 2, y - s->h / 2 - lift, flags, NULL, C_WHITE);
        else if (f->stun) spr_draw_ex(s, x - s->w / 2, y - s->h / 2, flags, NULL, (sst_g.frame_t / 4) % 2 ? C_SKY : C_ICE);
        else spr_draw(s, x - s->w / 2, y - s->h / 2 - lift, flags);
        if (f->kind == FK_COPTER) {
            for (int arm = 0; arm < 2; arm++) {
                int a = f->b + arm * 32;
                gfx_line(x, y, x + 21 * sst_cos(a) / 256, y + 21 * sst_sin(a) / 256, C_VIOLET);
                gfx_circ(x + 21 * sst_cos(a) / 256, y + 21 * sst_sin(a) / 256, 2, C_PURPLE);
            }
        }
        if (f->kind == FK_SKULLJET && f->a) gfx_circ(x + 6 * sst_cos(f->dir) / 256, y + 6 * sst_sin(f->dir) / 256, 3, C_SKY);
    }
}

static void draw_bullets(void) {
    for (int i = 0; i < SST_MAX_BULLETS; i++) {
        const SstBullet *b = &sst_g.eb[i];
        if (!b->alive) continue;
        int bx = b->x, by = b->y;
        if (b->kind == EB_HELIX) {
            int off = sst_sin(b->t * 3) * 7 * b->a;
            int a = sst_dir_to(b->vx, b->vy) + 16;
            bx += off * sst_cos(a) / 256;
            by += off * sst_sin(a) / 256;
        }
        int x = SST_PX(bx), y = view_y(SST_PX(by));
        switch (b->kind) {
        case EB_STAR:
            gfx_rect(x - 1, y - 3, 3, 7, C_YELLOW);
            gfx_rect(x - 3, y - 1, 7, 3, C_YELLOW);
            gfx_pset(x, y, C_WHITE);
            break;
        case EB_FIRE:
        case EB_HOMING:
            gfx_circ(x, y, 3, C_RED);
            gfx_circ(x, y, 1, C_YELLOW);
            break;
        case EB_BIG:
            gfx_circ(x, y, 5, C_INK);
            gfx_circ(x, y, 4, C_MAGENTA);
            gfx_circ(x - 1, y - 1, 1, C_WHITE);
            break;
        case EB_MINE:
            spr_draw(&sst_spr[SP_BOMB], x - 4, y - 4, 0);
            if (b->t > 70 && (b->t / 3) % 2) gfx_circb(x, y, 6, C_RED);
            break;
        case EB_LOB: {
            int h = sst_sin(b->t * 32 / imax(1, b->a)) * 24 / 256;
            gfx_dither(x - 3, y - 1, 6, 3, C_INK, 8);
            spr_draw(&sst_spr[SP_BOMB], x - 4, y - 4 - h, 0);
            break;
        }
        case EB_ROCKET:
            if (b->a - b->t < 20) {
                int h = (b->a - b->t) * 6;
                gfx_rect(x - 2, y - h - 8, 4, 8, C_GREY);
                gfx_rect(x - 1, y - h, 2, 3, C_ORANGE);
            }
            break;
        case EB_JET:
            gfx_circ(x, y, 6, C_BLUE);
            gfx_circ(x, y, 4, C_SKY);
            gfx_circ(x, y, 2, C_WHITE);
            break;
        default:
            gfx_circ(x, y, 3, C_INK);
            gfx_circ(x, y, 2, b->kind == EB_HELIX ? C_LIME : C_PINK);
            gfx_pset(x, y, C_WHITE);
            break;
        }
    }
}

static void draw_shots(void) {
    for (int i = 0; i < SST_MAX_SHOTS + 8; i++) {
        const SstShot *s = &sst_g.ps[i];
        if (!s->alive) continue;
        int x = SST_PX(s->x), y = view_y(SST_PX(s->y));
        if (s->from_opt) { gfx_circ(x, y, 2, C_ORANGE); continue; }
        if (s->big) {
            gfx_circ(x, y, 4, C_PINK);
            gfx_circ(x, y, 2, C_WHITE);
        } else {
            gfx_circ(x, y, 2, C_PINK);
            gfx_pset(x, y, C_WHITE);
        }
    }
}

static void draw_ground_things(void) {
    for (int i = 0; i < SST_MAX_HAZ; i++) {
        const SstHaz *h = &sst_g.hz[i];
        if (!h->alive) continue;
        int x = SST_PX(h->x), y = view_y(SST_PX(h->y));
        switch (h->kind) {
        case HZ_SLICK: gfx_circ(x, y, h->r - 1, h->t < 40 && (h->t / 4) % 2 ? C_FOREST : C_LEAF); break;
        case HZ_SLIME: gfx_circ(x, y, h->r - 1, h->t < 40 && (h->t / 4) % 2 ? C_PURPLE : C_MAGENTA); break;
        case HZ_MARK:
            gfx_line(x - 6, y - 6, x + 6, y + 6, C_RED);
            gfx_line(x - 6, y + 6, x + 6, y - 6, C_RED);
            if ((h->t / 4) % 2) gfx_circb(x, y, h->r, C_RED);
            break;
        case HZ_SNOW: {
            int fall = h->t * 3;
            gfx_dither(x - 5, y - 2, 10, 4, C_INK, 8);
            gfx_circ(x, y - fall, 4, C_WHITE);
            gfx_circb(x, y - fall, 4, C_ICE);
            break;
        }
        case HZ_BLAST:
        case HZ_PBLAST: {
            int r = h->r * (14 - imin(h->t, 14)) / 14 + 3;
            gfx_circ(x, y, r, h->kind == HZ_BLAST ? C_ORANGE : C_YELLOW);
            gfx_circ(x, y, r / 2, C_WHITE);
            break;
        }
        default: break;
        }
    }
    for (int i = 0; i < SST_MAX_BOMBS; i++) {
        const SstBomb *b = &sst_g.bomb[i];
        if (!b->alive) continue;
        spr_draw(&sst_spr[SP_BOMB], SST_PX(b->x) - 4, view_y(SST_PX(b->y)) - 4, 0);
    }
    for (int i = 0; i < SST_MAX_CARDS; i++) {
        const SstCard *c = &sst_g.card[i];
        if (!c->alive || (c->t > SST_CARD_LIFE - 120 && (c->t / 4) % 2)) continue;
        spr_draw(&sst_spr[SP_CARD0 + c->col - 1], SST_PX(c->x) - 4, view_y(SST_PX(c->y)) - 5 - (c->t < 10 ? 10 - c->t : 0), 0);
    }
    for (int i = 0; i < SST_MAX_PASSES; i++) {
        const SstPass *s = &sst_g.pass[i];
        if (!s->alive) continue;
        int x = SST_PX(s->x), y = view_y(SST_PX(s->y)) - (s->t < 20 ? 20 - s->t : 0);
        spr_draw(&sst_spr[SP_PASS], x - 5, y - 4, 0);
        if ((s->t / 6) % 2) gfx_pset(x + 6, y - 6, C_WHITE);
    }
}

/* ---- bosses --------------------------------------------------------------------------------- */

static void draw_boss(void) {
    const SstBoss *b = &sst_g.boss;
    if (!b->alive) return;
    int bx = SST_PX(b->x), by = view_y(SST_PX(b->y));
    bool flash = b->hurt > 0 && (b->hurt % 2);
    int t = b->t;
    switch (b->kind) {
    case BK_ENGINE: {
        int x = SST_PX(b->part[0].x), y = view_y(SST_PX(b->part[0].y));
        gfx_rect(x - 28, y + 8, 56, 4, C_INK);
        gfx_rect(x - 26, y - 10, 52, 20, flash ? C_WHITE : C_MAROON);
        gfx_rectb(x - 26, y - 10, 52, 20, C_INK);
        for (int k = -20; k <= 20; k += 10) gfx_circ(x + k, y + 10, 3, C_GREY);
        gfx_rect(x + 14, y - 22, 6, 12, C_SLATE);
        gfx_dither(x + 12 + (t / 4) % 4, y - 32 - (t / 3) % 8, 8, 6, C_LIGHT, 8);
        /* the fir on its back, and two crows */
        for (int k = 0; k < 3; k++) gfx_rect(x - 10 + k * 2 - 6, y - 14 - k * 6, 20 - k * 4, 6, C_FOREST);
        gfx_rect(x - 2, y - 32, 4, 4, C_WHITE);
        for (int k = 0; k < 2; k++) {
            int cx = x + (k ? 12 : -12) - 4, cy = y - 18;
            gfx_rect(cx, cy, 8, 6, C_INK);
            gfx_pset(cx + (k ? 7 : 0), cy + 2, C_YELLOW);
        }
        break;
    }
    case BK_GOGGLE: {
        int y = view_y(SST_PX(b->part[0].y)) + 12;
        gfx_circ(bx, y - 6, 30, C_GREY);
        gfx_circ(bx, y, 26, flash ? C_CREAM : C_TAN);
        gfx_circb(bx, y, 26, C_INK);
        gfx_rect(bx - 16, y + 14, 32, 5, C_INK);
        gfx_rect(bx - 12, y + 15, 24, 2, C_RED);
        /* the eye */
        int ey = view_y(SST_PX(b->part[1].y));
        gfx_circ(bx, ey, 10, C_INK);
        gfx_circ(bx, ey, 9, C_WHITE);
        int a = sst_player_ang(b->part[1].x, b->part[1].y);
        gfx_circ(bx + 4 * sst_cos(a) / 256, ey + 4 * sst_sin(a) / 256, 4, C_BLUE);
        gfx_circ(bx + 4 * sst_cos(a) / 256, ey + 4 * sst_sin(a) / 256, 2, C_INK);
        gfx_rect(bx - 22, ey - 12, 44, 2, C_INK); /* the goggle strap */
        for (int h = 2; h <= 3; h++) {
            const SstPart *p = &b->part[h];
            int x = SST_PX(p->x), y2 = view_y(SST_PX(p->y));
            if (p->a) {
                gfx_circ(x, y2, 7, C_SLATE);
                for (int k = 0; k < 8; k++) gfx_pset(x + 9 * sst_cos(k * 8 + t) / 256, y2 + 9 * sst_sin(k * 8 + t) / 256, C_WHITE);
            } else {
                spr_draw(&sst_spr[SP_GLOVE], x - 5, y2 - 4, h == 3 ? SPR_FLIPX : 0);
            }
        }
        break;
    }
    case BK_BOILER: {
        int x = SST_PX(b->part[0].x), y = view_y(SST_PX(b->part[0].y));
        gfx_rect(x - 24, y - 12, 8, 26, C_INK);
        gfx_rect(x + 16, y - 12, 8, 26, C_INK);
        for (int k = 0; k < 26; k += 4) {
            gfx_hline(x - 24, x - 17, y - 12 + (k + t / 2) % 26, C_GREY);
            gfx_hline(x + 16, x + 23, y - 12 + (k + t / 2) % 26, C_GREY);
        }
        gfx_rect(x - 16, y - 12, 32, 26, C_SLATE);
        gfx_rectb(x - 16, y - 12, 32, 26, C_INK);
        int hy = view_y(SST_PX(b->part[1].y));
        gfx_circ(x, hy, 9, C_INK);
        gfx_circ(x, hy, 8, flash ? C_ORANGE : C_RED);
        int a = sst_player_ang(b->part[1].x, b->part[1].y);
        gfx_line(x, hy, x + 14 * sst_cos(a) / 256, hy + 14 * sst_sin(a) / 256, C_INK);
        gfx_dither(x - 6 + (t / 3) % 5, y - 24 - (t / 2) % 10, 10, 8, C_LIGHT, 7);
        break;
    }
    case BK_DOPPELS:
        for (int k = 0; k < 2; k++) {
            const SstPart *p = &b->part[k];
            int x = SST_PX(p->x), y = view_y(SST_PX(p->y));
            int col = flash ? C_WHITE : k ? C_SKY : C_PINK;
            if (b->phase == 0) {
                gfx_circ(x, y, 8, col);
                for (int s = 0; s < 8; s++) gfx_line(x, y, x + 12 * sst_cos(s * 8 + t) / 256, y + 12 * sst_sin(s * 8 + t) / 256, C_INK);
                gfx_circ(x, y, 6, col);
            } else if (b->phase == 1) {
                gfx_circ(x, y, 9, C_RED);
                gfx_line(x - 8, y - 6, x - 10, y - 13, C_RED);
                gfx_line(x + 8, y - 6, x + 10, y - 13, C_RED);
                gfx_rect(x - 5, y - 3, 3, 3, C_YELLOW);
                gfx_rect(x + 2, y - 3, 3, 3, C_YELLOW);
                gfx_hline(x - 4, x + 4, y + 4, C_INK);
                gfx_circb(x, y, 9, col);
            } else {
                gfx_circ(x, y - 3, 8, C_LIME);
                gfx_rect(x - 3, y + 4, 6, 7, C_LEAF);
                gfx_circ(x - 3, y - 4, 2, C_INK);
                gfx_circ(x + 3, y - 4, 2, C_INK);
                gfx_circb(x, y - 3, 8, col);
            }
        }
        break;
    case BK_WICKS: {
        for (int k = 1; k <= 4; k++) {
            const SstPart *p = &b->part[k];
            spr_draw(&sst_spr[SP_TORCHF], SST_PX(p->x) - 4, view_y(SST_PX(p->y)) - 4, 0);
        }
        const SstPart *w = &b->part[0];
        int x = SST_PX(w->x), y = view_y(SST_PX(w->y));
        int lv = w->a ? 6 : 16;
        gfx_dither(x - 9, y - 4, 18, 16, flash ? C_WHITE : C_PURPLE, lv);
        gfx_dither(x - 6, y - 12, 12, 10, C_LIGHT, lv);
        gfx_rect(x - 3, y - 9, 2, 2, C_INK);
        gfx_rect(x + 1, y - 9, 2, 2, C_INK);
        gfx_dither(x - 8, y - 16, 16, 5, C_VIOLET, lv);
        break;
    }
    case BK_RIBBON:
        for (int k = 10; k >= 0; k--) {
            const SstPart *p = &b->part[k];
            int x = SST_PX(p->x), y = view_y(SST_PX(p->y));
            int col = k == 0 ? (flash ? C_WHITE : C_RED) : (k % 2 ? C_YELLOW : C_RED);
            gfx_circ(x, y, k == 0 ? 8 : 6, C_INK);
            gfx_circ(x, y, k == 0 ? 7 : 5, col);
            if (k == 0) {
                gfx_pset(x - 3, y - 2, C_WHITE);
                gfx_pset(x + 3, y - 2, C_WHITE);
            }
        }
        break;
    case BK_MORROW:
        for (int k = 0; k < 2; k++) {
            const SstPart *p = &b->part[k];
            if (!p->alive) continue;
            int x = SST_PX(p->x), y = view_y(SST_PX(p->y));
            gfx_rect(x - 9, y - 2, 18, 16, flash ? C_RED : C_WINE);
            gfx_rect(x - 6, y - 10, 12, 9, C_CREAM);
            gfx_rect(x - 4, y - 7, 2, 2, C_INK);
            gfx_rect(x + 2, y - 7, 2, 2, C_INK);
            gfx_hline(x - 5, x + 5, y - 3, C_INK);
            gfx_rect(x - 7, y - 22, 14, 12, C_INK);
            gfx_rect(x - 10, y - 12, 20, 3, C_INK);
            gfx_rect(x - 7, y - 14, 14, 2, C_RED);
            gfx_line(x + 9, y + 2, x + 15, y - 6, C_BROWN);
            gfx_pset(x + 15, y - 7, C_WHITE);
        }
        break;
    case BK_MAW: {
        /* the arms and their eyes */
        for (int k = 2; k < 6; k++) {
            const SstPart *e = &b->part[k];
            int x = SST_PX(e->x), y = view_y(SST_PX(e->y));
            gfx_line(bx + (x < bx ? -40 : 40), by, x, y, C_PURPLE);
            gfx_line(bx + (x < bx ? -40 : 40), by + 1, x, y + 1, C_VIOLET);
            gfx_circ(x, y, 7, C_INK);
            if (e->shut) gfx_hline(x - 5, x + 5, y, C_VIOLET);
            else {
                gfx_circ(x, y, 6, C_WHITE);
                gfx_circ(x, y, 3, C_RED);
            }
        }
        gfx_circ(bx, by, 46, C_PURPLE);
        gfx_circ(bx, by - 4, 40, flash ? C_VIOLET : C_DUSK);
        for (int k = 0; k < 2; k++) {
            const SstPart *e = &b->part[k];
            int x = SST_PX(e->x), y = view_y(SST_PX(e->y));
            gfx_circ(x, y, 11, C_INK);
            if (e->shut) {
                gfx_hline(x - 9, x + 9, y, C_RED);
            } else {
                gfx_circ(x, y, 10, C_YELLOW);
                int a = sst_player_ang(e->x, e->y);
                gfx_circ(x + 4 * sst_cos(a) / 256, y + 4 * sst_sin(a) / 256, 4, C_RED);
                gfx_circ(x + 4 * sst_cos(a) / 256, y + 4 * sst_sin(a) / 256, 2, C_INK);
            }
        }
        const SstPart *m = &b->part[6], *s = &b->part[7];
        int mx = SST_PX(m->x), my = view_y(SST_PX(m->y));
        gfx_rect(mx - 16, my - 5, 32, 10, C_INK);
        for (int k = 0; k < 6; k++) gfx_rect(mx - 14 + k * 5, my - 5, 3, 3, C_WHITE);
        if (s->a > 4 * SST_FX) {
            int sx = SST_PX(s->x), sy = view_y(SST_PX(s->y));
            for (int k = 1; k <= 8; k++) {
                int x = mx + (sx - mx) * k / 8 + (k < 8 ? sst_sin(t * 4 + k * 8) * 2 / 256 : 0);
                int y = my + (sy - my) * k / 8;
                gfx_circ(x, y, k == 8 ? 6 : 4, C_INK);
                gfx_circ(x, y, k == 8 ? 5 : 3, k == 8 ? C_LIME : C_JADE);
            }
            gfx_pset(sx - 2, sy - 1, C_RED);
            gfx_pset(sx + 2, sy - 1, C_RED);
        }
        break;
    }
    case BK_HEART:
        for (int k = 0; k < 23; k++) {
            const SstPart *p = &b->part[k];
            if (!p->alive) continue;
            int x = SST_PX(p->x), y = view_y(SST_PX(p->y));
            int col = p->kind == 8 ? C_RED : C_LIME;
            gfx_rect(x - 6, y - 6, 12, 12, C_INK);
            gfx_rect(x - 5, y - 5, 10, 10, flash ? C_WHITE : col);
            gfx_rect(x - 3, y - 3, 3, 3, PAL_LIGHTER[col]);
        }
        break;
    default: break;
    }
    (void)by;
}

/* ---- the HUD -------------------------------------------------------------------------------- */

static const char *const POW_NAME[CC_COUNT][3] = {
    {"", "", ""},
    {"", "HEAL", "EXTEND"},
    {"", "SPREAD", "BIG SPREAD"},
    {"", "BLINKER", "STUBBY"},
    {"", "BOMB", "BOMB TRAIL"},
    {"", "SPIKE BALL", "FIVE BALLS"},
};
static const uint8_t CARD_COL[CC_COUNT] = {C_GREY, C_PURPLE, C_JADE, C_ORANGE, C_RED, C_BLUE};

static void draw_hud(void) {
    const SstPlayer *p = &sst_g.pl;
    gfx_rect(0, 0, SCREEN_W, SST_HUD, C_INK);
    for (int k = 0; k < p->max_hp; k++) spr_draw(&sst_spr[k < p->hp ? SP_HEART : SP_HEART_E], 2 + k * 8, 2, 0);
    /* the hand */
    for (int k = 0; k < 3; k++) {
        int x = 38 + k * 10;
        if (k < p->nhand) spr_draw(&sst_spr[SP_CARD0 + p->hand[k] - 1], x, 0, 0);
        else gfx_rectb(x, 0, 9, 11, C_DUSK);
    }
    /* the powers held */
    for (int c = CC_SHOT; c < CC_COUNT; c++) {
        int x = 72 + (c - CC_SHOT) * 14;
        gfx_rect(x, 2, 12, 8, p->pow[c] ? CARD_COL[c] : C_NIGHT);
        if (p->pow[c]) tiny_draw(p->pow[c] == 2 ? "B" : "A", x + 4, 3, C_WHITE);
    }
    char b[24];
    spr_draw(&sst_spr[SP_LIFE], 132, 2, 0);
    snprintf(b, sizeof b, "%d", imax(0, sst_g.lives - 1)); /* the spare lives */
    tiny_draw(b, 141, 3, C_WHITE);
    spr_draw(&sst_spr[SP_PASS], 152, 1, 0);
    snprintf(b, sizeof b, "%d", sst_g.passes);
    tiny_draw(b, 165, 3, C_YELLOW);
    snprintf(b, sizeof b, "STAGE %s", STAGE_NUM[iclamp(sst_g.stage, 0, 4)]);
    tiny_draw(b, 182, 3, C_SLATE);
    /* the right side is player two's, from the next update */
    gfx_rectb(262, 1, 56, 10, C_NIGHT);
}

static void draw_boss_bar(void) {
    const SstBoss *b = &sst_g.boss;
    if (!b->alive || b->dying || b->maxhp <= 0) return;
    int w = 140 * imax(0, b->hp) / b->maxhp;
    gfx_rect(0, 171, SCREEN_W, 9, C_INK);
    tiny_draw(SST_BOSS_NAME[b->kind], 4, 173, C_WHITE);
    gfx_rect(172, 173, 144, 5, C_MAROON);
    gfx_rect(174, 174, w, 3, C_RED);
}

static void draw_world(void) {
    gfx_cls(C_INK);
    gfx_clip(0, SST_HUD, SCREEN_W, SST_VIEW_H);
    draw_tiles();
    draw_ground_things();
    draw_foes();
    draw_boss();
    draw_player();
    draw_shots();
    draw_bullets();
    gfx_noclip();
    draw_hud();
    draw_boss_bar();
    const SstPlayer *p = &sst_g.pl;
    if (p->hand_t > 0) {
        const char *what = p->hand_res == HAND_NOTHING ? "NO HAND" : POW_NAME[p->hand_col][p->hand_res];
        int w = text_width(what) + 40;
        gfx_rect(160 - w / 2, 16, w, 12, C_INK);
        for (int k = 0; k < 3; k++) spr_draw(&sst_spr[SP_CARD0 + p->shown[k] - 1], 160 - w / 2 + 2 + k * 6, 17, 0);
        text_draw(what, 160 - w / 2 + 36, 18, p->hand_res == HAND_B ? C_YELLOW : p->hand_res == HAND_A ? C_WHITE : C_GREY);
    }
    if (sst_g.code_t > 0) {
        gfx_rect(70, 60, 180, 40, C_INK);
        gfx_rectb(70, 60, 180, 40, C_YELLOW);
        text_center((sst_g.code_t / 8) % 2 ? "FREE-RIDE" : "", 160, 68, C_YELLOW);
        tiny_center("THE HOP IS OPEN FROM THE TITLE", 160, 86, C_WHITE);
    }
    if (p->state == PS_BUBBLE) tiny_center("ANY BUTTON: OUT OF THE BUBBLE", 160, 158, C_WHITE);
    if (sst_g.state == SS_CLEAR) {
        ui_fancy_center("STAGE CLEAR", 160, 70, 2, GRAD_SHOW, 4, C_INK, C_MAROON);
    }
    if (sst_g.stage == 4 && sst_g.half == 0 && sst_g.state == SS_PLAY) {
        char b[32];
        snprintf(b, sizeof b, "FLOOR %d", imin(sst_g.wave, sst_lift_wave_count()) * 3 + 1);
        tiny_draw(b, 4, 170, C_LIGHT);
    }
}

/* ---- the Hop ------------------------------------------------------------------------------- */

static void hop_xy(int row, int lane, int *x, int *y) {
    int rel = row * 16 - sst_g.hop.cam;
    *x = 26 + rel * 14 / 16 + lane * 14;
    *y = 112 - rel * 7 / 16 + lane * 7;
}

static void rhombus(int cx, int cy, int c, int edge) {
    for (int dy = -7; dy <= 7; dy++) {
        int w = 14 - iabs(dy) * 2;
        gfx_hline(cx - w, cx + w, cy + dy, iabs(dy) == 7 || w <= 1 ? edge : c);
    }
}

static void draw_hop(void) {
    const SstHop *h = &sst_g.hop;
    int t = sst_g.frame_t;
    gfx_cls(C_NIGHT);
    for (int k = 0; k < 40; k++) gfx_pset((k * 83 + h->cam / 4) % SCREEN_W, (k * 41) % SCREEN_H, k % 3 ? C_DUSK : C_VIOLET);
    int r0 = imax(0, h->cam / 16 - 2), r1 = h->cam / 16 + 15;
    int bx = 0, by = 0, brow = h->row;
    {
        /* where Jelly is drawn: on the way between two squares */
        int fx, fy, tx, ty;
        hop_xy(h->from_row, h->from_lane, &fx, &fy);
        hop_xy(h->row, h->lane, &tx, &ty);
        int k = h->state == HB_HOP ? h->hop_t : HOP_HOP_T;
        bx = fx + (tx - fx) * k / HOP_HOP_T;
        by = fy + (ty - fy) * k / HOP_HOP_T - (h->state == HB_HOP ? sst_sin(k * 32 / HOP_HOP_T) * 14 / 256 : 0);
        if (h->state == HB_READY) { bx = tx; by = ty; }
    }
    for (int r = r1; r >= r0; r--) {
        for (int l = 0; l < HOP_LANES; l++) {
            int tl = sst_hop_tile(r, l);
            int x, y;
            hop_xy(r, l, &x, &y);
            if (tl == HT_HOLE) continue;
            int c = (r + l) % 2 ? C_PINK : C_MAGENTA;
            rhombus(x, y, c, C_INK);
            gfx_hline(x - 14, x + 14, y + 8, C_WINE);
            if (tl == HT_SPIKE) {
                bool up = sst_hop_deadly(r, l);
                for (int s = -1; s <= 1; s++) {
                    if (up) {
                        gfx_line(x + s * 6 - 2, y + 2, x + s * 6, y - 6, C_WHITE);
                        gfx_line(x + s * 6 + 2, y + 2, x + s * 6, y - 6, C_LIGHT);
                    } else {
                        gfx_pset(x + s * 6, y, C_SLATE);
                    }
                }
            } else if (tl == HT_MOUTH) {
                bool open = sst_hop_deadly(r, l);
                if (open) {
                    gfx_rect(x - 6, y - 8, 12, 9, C_INK);
                    gfx_rect(x - 5, y - 7, 10, 7, C_RED);
                    for (int s = 0; s < 3; s++) gfx_rect(x - 5 + s * 4, y - 7, 2, 2, C_WHITE);
                } else {
                    gfx_hline(x - 5, x + 5, y, C_MAROON);
                }
            }
            int coin = sst_hop_coin(r, l);
            if (coin == 1) spr_draw(&sst_spr[SP_COIN], x - 2, y - 6 - (t / 8 + r) % 2, 0);
            if (coin == 5) spr_draw(&sst_spr[SP_BIGCOIN], x - 3, y - 8 - (t / 8 + r) % 2, 0);
        }
        if (r == brow && h->state != HB_DONE) {
            if (h->state == HB_FALL) {
                int k = imin(h->st, 30);
                if (k < 30) spr_draw(&sst_spr[SP_BLOB], bx - 6, by - 9 + k, 0);
            } else if (h->state == HB_BITTEN) {
                if ((h->st / 3) % 2) spr_draw(&sst_spr[SP_BLOB2], bx - 6, by - 9, 0);
            } else if (h->state != HB_READY || (h->st / 4) % 2) {
                spr_draw(&sst_spr[h->state == HB_HOP && h->hop_t < HOP_HOP_T / 2 ? SP_BLOB2 : SP_BLOB], bx - 6, by - 9, 0);
            }
        }
    }
    /* the meter, with its marks */
    gfx_rect(0, 0, SCREEN_W, 11, C_INK);
    char b[48];
    snprintf(b, sizeof b, "COINS %d", h->coins);
    tiny_draw(b, 4, 3, C_YELLOW);
    if (h->practice) snprintf(b, sizeof b, "TRIES: ENDLESS");
    else snprintf(b, sizeof b, "TRIES %d", h->tries);
    tiny_draw(b, 64, 3, C_WHITE);
    if (!h->practice) {
        snprintf(b, sizeof b, "LIVES %d  WON %d", imax(0, sst_g.lives - 1), h->lives_won);
        tiny_draw(b, 112, 3, C_PINK);
    } else {
        tiny_draw("B: LEAVE", 128, 3, C_PINK);
    }
    int prev = h->next_mark > 0 ? SST_HOP_MARK[h->next_mark - 1] : 0;
    int next = h->next_mark < HOP_MARKS ? SST_HOP_MARK[h->next_mark] : prev + 1;
    int fill = h->next_mark < HOP_MARKS ? 100 * (h->coins - prev) / imax(1, next - prev) : 100;
    gfx_rect(200, 2, 104, 7, C_DUSK);
    gfx_rect(202, 4, fill, 3, C_YELLOW);
    spr_draw(&sst_spr[SP_LIFE], 307, 2, 0);
    gfx_rect(0, 170, SCREEN_W, 10, C_INK);
    tsay_c("UP: ON   LEFT, RIGHT: ACROSS   DOWN: BACK", 160, 172, C_LIGHT);
    if (h->state == HB_DONE) say_cs(h->made_it ? "THE FAR END!" : "THAT'S ALL THE TRIES", 160, 80, C_WHITE, C_INK);
}

/* ---- the small screens ------------------------------------------------------------------- */

static void draw_offer(void) {
    tent_backdrop(sst_g.frame_t);
    ui_fancy_center("THE HOP", 160, 18, 3, GRAD_SHOW, 4, C_INK, C_MAROON);
    ui_audit_fancy("hop", "THE HOP", 160 - ui_fancy_width("THE HOP", 3) / 2, 18, 3, true);
    ui_panel(40, 52, 240, 92, C_NIGHT, C_YELLOW);
    char b[64];
    snprintf(b, sizeof b, "YOU HOLD %d STAR PASS%s.", sst_g.passes, sst_g.passes == 1 ? "" : "ES");
    say_c(b, 160, 60, C_YELLOW);
    say_c("EACH PASS IS ONE TRY.", 160, 74, C_WHITE);
    say_c("COINS FILL THE METER:", 160, 86, C_WHITE);
    say_c("EVERY MARK ON IT IS A LIFE.", 160, 98, C_WHITE);
    ui_choices(160, 122, "PLAY", "LATER", sst_g.offer_sel, sst_g.frame_t, C_WHITE, C_GREY);
    tsay_c("LATER KEEPS THE PASSES FOR ANOTHER BREAK", 160, 160, C_LIGHT);
}

static void draw_hopend(void) {
    const SstHop *h = &sst_g.hop;
    tent_backdrop(sst_g.frame_t);
    ui_panel(60, 40, 200, 90, C_NIGHT, C_PINK);
    say_c(h->practice ? "FREE RIDE OVER" : "THE HOP IS OVER", 160, 50, C_PINK);
    char b[48];
    snprintf(b, sizeof b, "COINS %d", h->coins);
    say_c(b, 160, 70, C_YELLOW);
    if (h->practice) snprintf(b, sizeof b, "BEST %d", sst_sv.hop_best);
    else snprintf(b, sizeof b, "LIVES WON %d", h->lives_won);
    say_c(b, 160, 84, C_WHITE);
    if (!h->practice) {
        snprintf(b, sizeof b, "LIVES NOW %d", imax(0, sst_g.lives - 1));
        say_c(b, 160, 98, C_WHITE);
    }
    spr_draw(&sst_spr[SP_BLOB], 154, 112, 0);
}

static void draw_continue(void) {
    gfx_cls(C_INK);
    say_cs("CONTINUE?", 160, 50, C_WHITE, C_MAROON);
    char b[32];
    draw_clover_at(153, 70, 2, (sst_g.frame_t / 30) % 2, 0);
    ui_choices(160, 112, "YES", "NO", sst_g.cont_sel, sst_g.frame_t, C_WHITE, C_GREY);
    snprintf(b, sizeof b, "STAGE %s FROM THE START", STAGE_NUM[iclamp(sst_g.stage, 0, 4)]);
    tsay_c(b, 160, 136, C_LIGHT);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    ui_fancy_center("GAME OVER", 160, 60, 3, GRAD_SHOW, 4, C_INK, C_MAROON);
    ui_audit_fancy("over", "GAME OVER", 160 - ui_fancy_width("GAME OVER", 3) / 2, 60, 3, true);
    char b[48];
    snprintf(b, sizeof b, "STAR PASSES FOUND %d", sst_g.passes_run);
    say_c(b, 160, 110, C_LIGHT);
}

static const char *const GOOD_END[] = {
    "THE HEARTSTACK BREAKS APART.",
    "BEHIND IT, IN A ROW OF CAGES:",
    "THE SEAL, THE GEESE, THE OLD BEAR.",
    "",
    "CLOVER OPENS EVERY ONE.",
    "THE SHOW GOES ON TOMORROW,",
    "WITHOUT A MAGICIAN.",
};
static const char *const BAD_END[] = {
    "THE MORROWMAW FALLS QUIET.",
    "THE TOWER IS EMPTY: NO CAGES,",
    "NO FRIENDS. ONLY A HAT ON THE FLOOR.",
    "",
    "SOMEWHERE FAR BELOW,",
    "SOMETHING IS STILL LAUGHING.",
    "",
    "THERE WAS MORE TO IT.",
    "TRY IT WITHOUT A CONTINUE.",
};

static void draw_ending(void) {
    bool good = sst_g.good;
    gfx_cls(good ? C_NAVY : C_INK);
    if (good)
        for (int k = 0; k < 30; k++) gfx_pset((k * 61) % SCREEN_W, (k * 23) % 60, C_YELLOW);
    const char *const *lines = good ? GOOD_END : BAD_END;
    int n = good ? ARRAY_LEN(GOOD_END) : ARRAY_LEN(BAD_END);
    int shown = imin(n, 1 + sst_g.state_t / 30);
    for (int k = 0; k < shown; k++)
        if (lines[k][0]) say_c(lines[k], 160, 16 + k * 12, k >= 7 ? C_YELLOW : C_CREAM);
    draw_clover_at(153, 140, 2, (sst_g.frame_t / 20) % 2, 0);
    if (good) {
        spr_draw(&sst_spr[SP_OPT_A], 130, 148, 0);
        spr_draw(&sst_spr[SP_BLOB], 180, 146, 0);
    }
}

static void draw_credits(void) {
    gfx_cls(C_INK);
    static const char *const LINES[] = {
        "SHOWSTOPPER",
        "",
        "CLOVER, THE SHOW RABBIT",
        "JELLY, WHO HOPS",
        "MORROW THE MARVELLOUS",
        "",
        "THE SNOW ENGINE",
        "GRANNY GOGGLE",
        "THE BOILER",
        "THE DOPPELS",
        "OLD WICKS",
        "THE RIBBON",
        "THE MORROWMAW",
        "THE HEARTSTACK",
        "",
        "A UFO 40 CARTRIDGE",
        "THANK YOU FOR PLAYING",
    };
    int y0 = SCREEN_H - sst_g.state_t / 3;
    for (int i = 0; i < ARRAY_LEN(LINES); i++) {
        int y = y0 + i * 14;
        if (y < -10 || y > SCREEN_H) continue;
        text_center_shadow(LINES[i], 160, y, i == 0 ? C_YELLOW : C_WHITE, C_MAROON);
    }
}

void sst_draw(void) {
    gfx_camera(0, 0);
    switch (sst_g.state) {
    case SS_TITLE: draw_title(); break;
    case SS_STORY: draw_story(); break;
    case SS_MAP: draw_map(); break;
    case SS_PLAY: case SS_CLEAR: draw_world(); break;
    case SS_OFFER: draw_offer(); break;
    case SS_HOP: draw_hop(); break;
    case SS_HOPEND: draw_hopend(); break;
    case SS_CONTINUE: draw_continue(); break;
    case SS_OVER: draw_over(); break;
    case SS_ENDING: draw_ending(); break;
    case SS_CREDITS: draw_credits(); break;
    default: gfx_cls(C_INK); break;
    }
}

/* the cartridge label */
void sst_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_NIGHT);
    for (int k = 0; k < w; k += 12) gfx_rect(x + k, y, 6, h, C_MAROON);
    gfx_rect(x, y + h - 14, w, 14, C_WHITE);
    draw_clover_at(x + 10, y + h - 26, (t / 30) % 2 ? 2 : 0, (t / 8) % 2, 0);
    spr_draw(&sst_spr[SP_CARD0 + (t / 20) % 5], x + w - 34, y + h - 30, 0);
    spr_draw(&sst_spr[SP_PASS], x + w - 20, y + h - 26, 0);
    static const uint8_t grad[2] = {C_YELLOW, C_ORANGE};
    ui_fancy_text("SHOWSTOPPER", x + 4, y + 4, 1, grad, 2, C_INK, C_MAROON);
    gfx_noclip();
    (void)GRAD_ICE;
}

/* tests: the screens' words stay inside the screen and clear of each other */
int sst_layout_audit(void) {
    int bad = 0;
    SstGame keep;
    memcpy(&keep, &sst_g, sizeof keep);
    static const int SCREENS[] = {SS_TITLE, SS_STORY, SS_MAP, SS_OFFER, SS_HOPEND, SS_CONTINUE, SS_OVER, SS_ENDING};
    for (int i = 0; i < ARRAY_LEN(SCREENS); i++) {
        for (int v = 0; v < 2; v++) {
            sst_g.state = SCREENS[i];
            sst_g.state_t = 2000;
            sst_g.menu = v;
            sst_g.good = v;
            sst_g.hop.practice = v;
            sst_g.passes = v ? 1 : 3;
            sst_g.stage = v ? 4 : 0;
            ui_audit_begin("showstopper", true);
            ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
            sst_draw();
            bad += ui_audit_end();
        }
    }
    for (int k = 0; k < SST_STORY_N; k++) bad += text_missing(SST_STORY[k]) > 0;
    for (int k = 0; k < ARRAY_LEN(GOOD_END); k++) bad += text_missing(GOOD_END[k]) > 0 || text_width(GOOD_END[k]) > 300;
    for (int k = 0; k < ARRAY_LEN(BAD_END); k++) bad += text_missing(BAD_END[k]) > 0 || text_width(BAD_END[k]) > 300;
    for (int k = 1; k < BK_COUNT; k++) bad += tiny_missing(SST_BOSS_NAME[k]) > 0;
    for (int c = 1; c < CC_COUNT; c++)
        for (int l = 1; l <= 2; l++) bad += text_missing(POW_NAME[c][l]) > 0;
    memcpy(&sst_g, &keep, sizeof keep);
    return bad;
}
