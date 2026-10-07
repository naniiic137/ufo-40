/* SOUNDINGS - drawing: the title, the raft and its three unlabelled
 * pictures, the shop, the kit, the dive (the cave, its creatures, the dark
 * and the panel at the left), the item menu, the fights (first person: the
 * creatures above, the orders at the bottom left, the divers' HP at the
 * bottom right), surfacing, the wipe, the ending and the cartridge label.
 * Drawing may use anything; nothing here changes the game. */
#include "sdg.h"

#define P (sdg.prog)
#define VX 80   /* the dive's view: right of the panel */
#define VW 240

static const uint8_t SEAG[] = {C_WHITE, C_ICE, C_CYAN, C_SKY};
static const uint8_t DEEPG[] = {C_WHITE, C_PINK, C_RED, C_WINE};

/* ---- small helpers ----------------------------------------------------------- */

static void bar(int x, int y, int w, int h, int v, int max, int fg, int bg) {
    gfx_rect(x, y, w, h, bg);
    if (max > 0 && v > 0) gfx_rect(x, y, imax(1, w * imin(v, max) / max), h, fg);
}

static void face(int d, int x, int y, bool dead) {
    const Sprite *s = sdg_sprite(SP_FACE0 + d);
    if (dead) spr_draw_ex(s, x, y, 0, NULL, C_DUSK);
    else spr_draw(s, x, y, 0);
}

/* a sprite at scale with a colour map (the fights) */
static void spr_scaled_map(const Sprite *s, int x, int y, int scale, int flags, const uint8_t *map, int solid) {
    if (!s || !s->px) return;
    for (int sy = 0; sy < s->h; sy++)
        for (int sx = 0; sx < s->w; sx++) {
            int srcx = (flags & SPR_FLIPX) ? s->w - 1 - sx : sx;
            uint8_t c = s->px[sy * s->w + srcx];
            if (c == TRANSPARENT) continue;
            if (solid >= 0) c = (uint8_t)solid;
            else if (map) c = map[c];
            gfx_rect(x + sx * scale, y + sy * scale, scale, scale, c);
        }
}

static uint32_t hsh(int a, int b) {
    uint32_t h = (uint32_t)a * 73856093u ^ (uint32_t)b * 19349663u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}

static void sea_backdrop(int t, int top, int col) {
    gfx_rect(0, top, SCREEN_W, SCREEN_H - top, col);
    for (int i = 0; i < 40; i++) {
        int x = (int)(hsh(i, 3) % SCREEN_W), y = top + (int)((hsh(i, 7) % (SCREEN_H - top)) - (uint32_t)(t / (2 + i % 3))) % (SCREEN_H - top);
        if (y < top) y += SCREEN_H - top;
        gfx_pset(x, y, i % 4 ? C_NAVY : C_BLUE);
    }
}

/* ---- the title ------------------------------------------------------------------- */

void sdg_draw_title(void) {
    int t = (int)sdg.frame;
    gfx_cls(C_INK);
    for (int y = 0; y < 30; y++) gfx_hline(0, SCREEN_W - 1, y, y < 12 ? C_PURPLE : y < 22 ? C_VIOLET : C_MAGENTA);
    gfx_rect(0, 30, SCREEN_W, 2, C_SKY);
    sea_backdrop(t, 32, C_NAVY);
    for (int y = 90; y < SCREEN_H; y += 2) gfx_dither(0, y, SCREEN_W, 2, C_INK, iclamp((y - 90) / 6, 0, 16));
    /* the raft, and a line let down from it */
    gfx_rect(140, 26, 40, 5, C_BROWN);
    gfx_rect(142, 31, 36, 2, C_EARTH);
    gfx_rect(150, 14, 18, 12, C_TAN);
    gfx_rect(147, 11, 24, 4, C_BROWN);
    int lead = 104 + (t / 6) % 8;
    gfx_vline(160, 31, lead, C_LIGHT);
    gfx_circ(160, lead + 2, 2, C_GREY);
    /* light shafts */
    for (int k = 0; k < 4; k++) gfx_dither(30 + k * 80 + (t / 20 + k * 9) % 12, 32, 10, 70, C_TEAL, 3);
    ui_fancy_center("SOUNDINGS", 160, 40, 3, SEAG, 4, C_INK, C_NAVY);
    tiny_center("THREE DIVERS " GLYPH_DOT " ONE RAFT " GLYPH_DOT " THE DARK UNDER IT", 160, 66, C_ICE);
    const Sprite *dv = sdg_sprite((t / 16) & 1 ? SP_DIVER2 : SP_DIVER);
    spr_draw(dv, 60 + (t / 3) % 200, 80 + ((t / 24) & 1), 0);
    bool has = sdg.saved.magic == SDG_MAGIC && sdg.saved.started;
    const char *items[3];
    int n = 0;
    if (has) items[n++] = "CONTINUE";
    items[n++] = "NEW GAME";
    items[n++] = "CODE";
    ui_panel(110, 92, 100, 12 + n * 12, C_INK, C_CYAN);
    for (int k = 0; k < n; k++) {
        int y = 98 + k * 12;
        bool sel = sdg.title_sel == k;
        text_center(items[k], 160, y, sel ? C_YELLOW : C_LIGHT);
        if (sel) ui_cursor(116, y, t);
    }
    if (sdg.title_erase) tiny_center(GLYPH_A " AGAIN TO START A NEW LOG OVER THE OLD ONE", 160, 136, C_PINK);
    else if (sdg.code_on) tiny_center("RAFT-EGGS IS ON: NOTHING IS WRITTEN, NO GOALS", 160, 136, C_LIME);
    char b[96];
    const SdgProg *s = &sdg.saved;
    snprintf(b, sizeof b, "LEVEL %d  " GLYPH_DOT "  DOORS OPEN %d  " GLYPH_DOT "  CHESTS OPEN %d", has ? s->level : 0,
             has ? s->doors_open : 0, has ? s->chests_open : 0);
    tiny_center(b, 160, 146, C_CREAM);
    text_center(GLYPH_A " CHOOSE   " GLYPH_B " LIBRARY", 160, 164, C_GREY);
}

void sdg_draw_code(void) {
    int t = (int)sdg.frame;
    gfx_cls(C_INK);
    sea_backdrop(t, 0, C_NIGHT);
    ui_fancy_center("CODE", 160, 14, 2, SEAG, 4, C_INK, C_NAVY);
    tiny_center("A WORD SCRATCHED ON THE RAFT CHANGES THE NEXT LOG.", 160, 40, C_LIGHT);
    tiny_center("WHILE ONE IS ON, NOTHING IS WRITTEN AND NO GOAL IS EARNED.", 160, 48, C_GREY);
    int x0 = 160 - (9 * 16) / 2;
    for (int k = 0; k < 9; k++) {
        int x = x0 + k * 16;
        if (k == 4) { text_draw_scaled("-", x + 3, 76, C_GREY, 2); continue; }
        int ci = k < 4 ? k : k - 1;
        char ch[2] = {sdg.code[ci], 0};
        bool sel = ci == sdg.code_pos;
        gfx_rect(x, 70, 13, 20, sel ? C_DUSK : C_NIGHT);
        text_draw_scaled(ch, x + 2, 74, sel ? C_YELLOW : C_WHITE, 2);
        if (sel) {
            text_draw(GLYPH_UP, x + 3, 61, C_GREY);
            text_draw(GLYPH_DOWN, x + 3, 92, C_GREY);
        }
    }
    if (sdg.code_msg_t > 0) text_center(sdg.code_on ? "THREE EGGS WAIT ON THE RAFT." : "NOTHING HAPPENS.", 160, 112, sdg.code_on ? C_LIME : C_PINK);
    text_center(GLYPH_DPAD " LETTERS   " GLYPH_A " ENTER   " GLYPH_B " BACK", 160, 162, C_GREY);
}

/* ---- the raft ---------------------------------------------------------------- */

static void raft_scene(int t) {
    for (int y = 0; y < 70; y++) gfx_hline(0, SCREEN_W - 1, y, y < 25 ? C_NIGHT : y < 45 ? C_PURPLE : y < 60 ? C_VIOLET : C_MAGENTA);
    gfx_circ(270, 30, 8, C_CREAM);
    for (int i = 0; i < 20; i++) gfx_pset((int)(hsh(i, 1) % 320), (int)(hsh(i, 2) % 30), C_LIGHT);
    gfx_rect(0, 70, SCREEN_W, SCREEN_H - 70, C_NAVY);
    for (int i = 0; i < 30; i++) {
        int y = 72 + (int)(hsh(i, 9) % 100);
        int x = (int)((hsh(i, 5) + (uint32_t)(t / 3)) % 340) - 10;
        gfx_hline(x, x + 6, y, C_BLUE);
    }
    /* the raft with its shed */
    gfx_rect(100, 62, 120, 8, C_BROWN);
    for (int k = 0; k < 12; k++) gfx_vline(102 + k * 10, 62, 69, C_EARTH);
    gfx_rect(116, 40, 34, 22, C_TAN);
    gfx_rect(112, 36, 42, 5, C_BROWN);
    gfx_rect(128, 48, 8, 14, C_EARTH);
    gfx_vline(190, 30, 61, C_LIGHT);
    gfx_hline(180, 200, 34, C_LIGHT);
}

void sdg_draw_raft(void) {
    int t = (int)sdg.frame;
    raft_scene(t);
    /* the three pictures: no words */
    static const int ICON[3] = {SP_ICON_SHOP, SP_ICON_KIT, SP_ICON_DIVE};
    for (int k = 0; k < 3; k++) {
        int x = 14 + k * 54, y = 96;
        bool sel = sdg.raft_sel == k;
        ui_panel(x - 4, y - 4, 32, 32, C_INK, sel ? ((t / 8) & 1 ? C_YELLOW : C_AMBER) : C_DUSK);
        spr_draw(sdg_sprite(ICON[k]), x, y, 0);
    }
    /* the log */
    ui_panel(176, 84, 140, 92, C_INK, C_CYAN);
    char b[48];
    snprintf(b, sizeof b, "LV %d", P.level);
    text_draw(b, 182, 89, C_WHITE);
    int need = P.level < SDG_MAX_LEVEL ? SDG_XP_AT[P.level + 1] - SDG_XP_AT[P.level] : 1;
    int have = P.level < SDG_MAX_LEVEL ? (int)P.xp - SDG_XP_AT[P.level] : 1;
    for (int k = 0; k < 10; k++) gfx_rect(214 + k * 9, 90, 7, 5, k * need < have * 10 ? C_CYAN : C_DUSK);
    snprintf(b, sizeof b, GLYPH_COIN " %ld", (long)P.gold);
    text_draw(b, 182, 101, C_YELLOW);
    snprintf(b, sizeof b, "%04d", P.deepest);
    text_draw(b, 268, 101, C_ICE);
    gfx_vline(262, 101, 107, C_DUSK);
    for (int r = 0; r < RL_COUNT; r++) {
        int x = 182 + (r % 4) * 33, y = 116 + (r / 4) * 14;
        sdg_draw_relic_icon(r, x, y);
        snprintf(b, sizeof b, "%d", P.relic[r]);
        text_draw(b, x + 11, y, P.relic[r] ? C_WHITE : C_DUSK);
    }
    for (int h = 0; h < SDG_HEADS; h++)
        if (P.heads & (1 << h)) sdg_draw_head_icon(h, 186 + h * 16, 148);
    tiny_center(GLYPH_LEFT GLYPH_RIGHT " " GLYPH_A " " GLYPH_B, 80, 168, C_GREY);
}

/* ---- the shop ------------------------------------------------------------------ */

void sdg_draw_shop(void) {
    int t = (int)sdg.frame;
    gfx_cls(C_NIGHT);
    gfx_rect(0, 0, SCREEN_W, 18, C_BROWN);
    gfx_hline(0, SCREEN_W - 1, 18, C_EARTH);
    char b[64];
    snprintf(b, sizeof b, GLYPH_LEFT " %s " GLYPH_RIGHT, SDG_SHOP_PAGE[sdg.shop_page]);
    text_draw(b, 8, 6, C_CREAM);
    snprintf(b, sizeof b, GLYPH_COIN " %ld", (long)P.gold);
    text_draw(b, SCREEN_W - 8 - text_width(b), 6, C_YELLOW);
    uint8_t list[16];
    int n = sdg_shop_list(sdg.shop_page, list, 16);
    for (int i = 0; i <= n; i++) {
        int y = 26 + i * 11;
        bool sel = sdg.shop_sel == i;
        if (sel) gfx_rect(4, y - 2, SCREEN_W - 8, 11, C_DUSK);
        if (i == n) {
            text_draw("RETURN", 26, y, sel ? C_YELLOW : C_LIGHT);
            if (sel) ui_cursor(10, y, t);
            break;
        }
        int it = list[i];
        const SdgItem *I = &SDG_ITEM[it];
        int col = sdg_can_buy(&P, it) ? C_WHITE : C_VIOLET;
        sdg_draw_item_icon(it, 14, y - 1);
        text_draw(I->name, 26, y, col);
        if (I->el) tiny_draw(SDG_EL_NAME[I->el], 96, y + 1, SDG_EL_COL[I->el]);
        snprintf(b, sizeof b, "%d/%d", P.owned[it], I->max);
        text_draw(b, 170, y, col);
        snprintf(b, sizeof b, "%d", I->price);
        text_draw(b, 250, y, col);
        if (I->rel[0] >= 0) text_draw("+", 282, y, col);
    }
    /* the highlighted item: what it does and the relics it costs */
    gfx_rect(0, 142, SCREEN_W, 38, C_INK);
    gfx_hline(0, SCREEN_W - 1, 142, C_EARTH);
    if (sdg.shop_sel < n) {
        int it = list[sdg.shop_sel];
        const SdgItem *I = &SDG_ITEM[it];
        if (sdg_is_weapon(it)) snprintf(b, sizeof b, "POWER %d  " GLYPH_DOT "  USES %d%s", I->power, I->uses, I->def ? "  " GLYPH_DOT "  GUARDS" : "");
        else if (I->kind == K_POTION) snprintf(b, sizeof b, "MENDS %d  " GLYPH_DOT "  USES %d", I->heal, I->uses);
        else if (I->uses) snprintf(b, sizeof b, "USES %d", I->uses);
        else snprintf(b, sizeof b, "ALWAYS AT WORK");
        text_draw(b, 8, 148, C_LIGHT);
        int x = 8;
        for (int k = 0; k < 2; k++) {
            if (I->rel[k] < 0) continue;
            sdg_draw_relic_icon(I->rel[k], x, 162);
            snprintf(b, sizeof b, "%d/%d", P.relic[I->rel[k]], I->reln[k]);
            text_draw(b, x + 11, 162, P.relic[I->rel[k]] >= I->reln[k] ? C_WHITE : C_VIOLET);
            x += 50;
        }
    }
    tiny_draw(GLYPH_A " BUY  " GLYPH_B " BACK", SCREEN_W - 70, 170, C_GREY);
    /* six hands, one thing */
    bool same = P.equip[0][0] != 0;
    for (int k = 1; k < 6; k++) same = same && P.equip[k / 2][k % 2] == P.equip[0][0];
    if (same) {
        gfx_rect(40, 128, 240, 11, C_NAVY);
        tiny_center("SIX OF THE SAME? THE SHED CREAKS, IMPRESSED.", 160, 131, C_ICE);
    }
}

/* ---- the kit ------------------------------------------------------------------- */

void sdg_draw_kit(void) {
    int t = (int)sdg.frame;
    gfx_cls(C_NIGHT);
    ui_panel(2, 2, 150, 176, C_INK, sdg.kit_col == 0 ? C_CYAN : C_DUSK);
    ui_panel(156, 2, 162, 176, C_INK, sdg.kit_col == 1 ? C_CYAN : C_DUSK);
    uint8_t list[IT_COUNT];
    int n = 0;
    for (int it = 1; it < IT_COUNT; it++)
        if (P.owned[it] > 0) list[n++] = (uint8_t)it;
    int rows = 10;
    int top = 0;
    if (sdg.kit_col == 0 && sdg.kit_sel >= rows) top = sdg.kit_sel - rows + 1;
    char b[48];
    for (int i = 0; i < rows && top + i < n; i++) {
        int it = list[top + i], y = 8 + i * 11;
        bool sel = sdg.kit_col == 0 && sdg.kit_sel == top + i;
        if (sel) gfx_rect(6, y - 2, 142, 11, C_DUSK);
        sdg_draw_item_icon(it, 10, y - 1);
        text_draw(SDG_ITEM[it].name, 22, y, sdg_stored(&P, it) > 0 ? C_WHITE : C_GREY);
        snprintf(b, sizeof b, "x%d", sdg_stored(&P, it));
        text_draw(b, 126, y, C_LIGHT);
    }
    if (n == 0) text_draw("NOTHING YET", 12, 10, C_GREY);
    /* the highlighted item */
    int show = sdg.kit_held ? sdg.kit_held : sdg.kit_col == 0 && n ? list[iclamp(sdg.kit_sel, 0, n - 1)]
                                            : (sdg.kit_sel < 6 ? P.equip[sdg.kit_sel / 2][sdg.kit_sel % 2] : 0);
    gfx_hline(6, 146, 122, C_DUSK);
    if (show) {
        const SdgItem *I = &SDG_ITEM[show];
        sdg_draw_item_icon(show, 10, 127);
        text_draw(I->name, 22, 128, C_CREAM);
        if (I->el) tiny_draw(SDG_EL_NAME[I->el], 100, 129, SDG_EL_COL[I->el]);
        if (sdg_is_weapon(show)) snprintf(b, sizeof b, "POWER %d  USES %d", I->power, I->uses);
        else if (I->kind == K_POTION) snprintf(b, sizeof b, "MENDS %d  USES %d", I->heal, I->uses);
        else if (I->uses) snprintf(b, sizeof b, "USES %d", I->uses);
        else snprintf(b, sizeof b, "ALWAYS AT WORK");
        text_draw(b, 10, 141, C_LIGHT);
        if (I->kind == K_SHIELD) { snprintf(b, sizeof b, "TAKES %d%% OFF EVERY HIT", I->def); tiny_draw(b, 10, 154, C_GREY); }
    }
    /* the divers and their hands */
    for (int d = 0; d < 3; d++) {
        int y = 8 + d * 44;
        face(d, 162, y, false);
        text_draw(sdg_diver_name(d), 178, y + 2, C_WHITE);
        for (int s = 0; s < 2; s++) {
            int x = 162 + s * 76, yy = y + 16, k = d * 2 + s;
            bool sel = sdg.kit_col == 1 && sdg.kit_sel == k;
            gfx_rect(x, yy, 72, 20, sel ? C_DUSK : C_NIGHT);
            gfx_rectb(x, yy, 72, 20, sel ? ((t / 8) & 1 ? C_YELLOW : C_AMBER) : C_SLATE);
            int it = P.equip[d][s];
            if (it) {
                sdg_draw_item_icon(it, x + 3, yy + 6);
                text_draw(SDG_ITEM[it].name, x + 14, yy + 6, C_LIGHT);
            }
        }
    }
    bool rsel = sdg.kit_col == 1 && sdg.kit_sel >= 6;
    text_draw("RETURN", 210, 142, rsel ? C_YELLOW : C_LIGHT);
    if (rsel) ui_cursor(196, 142, t);
    if (sdg.kit_held) {
        gfx_rect(160, 156, 154, 16, C_DUSK);
        sdg_draw_item_icon(sdg.kit_held, 164, 160);
        text_draw(SDG_ITEM[sdg.kit_held].name, 176, 160, C_YELLOW);
        tiny_draw("WHERE TO?", 262, 161, C_CREAM);
    } else tiny_draw(GLYPH_A " PICK UP  " GLYPH_B " BACK", 196, 164, C_GREY);
}

/* ---- the dive ------------------------------------------------------------------- */

typedef struct Look { uint8_t fill, hi, lo, water, speck; } Look;
static const Look LOOK[RG_COUNT] = {
    [RG_SHELF] = {C_SLATE, C_GREY, C_DUSK, C_NAVY, C_TEAL},
    [RG_GUMWELL] = {C_FOREST, C_JADE, C_NIGHT, C_NAVY, C_LEAF},
    [RG_HOLLOW] = {C_PURPLE, C_VIOLET, C_NIGHT, C_NAVY, C_MAGENTA},
    [RG_SHRINE] = {C_EARTH, C_TAN, C_BROWN, C_NIGHT, C_HIDE},
    [RG_LANTERN] = {C_TEAL, C_JADE, C_NAVY, C_NIGHT, C_CYAN},
    [RG_STILT] = {C_DUSK, C_SLATE, C_INK, C_NIGHT, C_GREY},
    [RG_STILL] = {C_JADE, C_LEAF, C_FOREST, C_FOREST, C_LIME},
    [RG_DEEP] = {C_WINE, C_RED, C_MAROON, C_INK, C_PINK},
};

static int cam_x, cam_y;

static bool cell_rock(int c, int r) {
    if (c < 0 || r < 0 || c >= SDG_MW || r >= SDG_MH) return true;
    char ch = SDG_MAP[r][c];
    return ch == '#' || ch == '%' || (ch == 'X' && !sdg_cell_open(c, r));
}

static void draw_cell(int c, int r, int t) {
    int x = c * SDG_T, y = r * SDG_T;
    char ch = SDG_MAP[r][c];
    const Look *L = &LOOK[sdg_region_at(c, r)];
    if (ch == '^') {
        gfx_rect(x, y, SDG_T, SDG_T, r < 2 ? C_PURPLE : C_VIOLET);
        if (r == SDG_SURF_ROW - 1) gfx_hline(x, x + SDG_T - 1, y + SDG_T - 2, C_SKY);
        return;
    }
    if (!cell_rock(c, r) && !(ch == '1' || ch == '2' || ch == '=' || ch == 'F')) {
        gfx_rect(x, y, SDG_T, SDG_T, L->water);
        uint32_t h = hsh(c, r);
        if ((h & 7) == 0) gfx_pset(x + (int)(h >> 4) % 16, y + (int)(h >> 8) % 16, sdg_region_at(c, r) == RG_STILL ? C_WHITE : C_BLUE);
        return;
    }
    if (ch == '1' || ch == '2' || ch == '=' || ch == 'F') {
        gfx_rect(x, y, SDG_T, SDG_T, L->water);
        if (sdg_cell_open(c, r)) return;
        int col = ch == 'F' ? C_RED : ch == '=' ? C_AMBER : C_GREY;
        for (int k = 1; k < SDG_T; k += 4) gfx_vline(x + k, y, y + SDG_T - 1, col);
        gfx_hline(x, x + SDG_T - 1, y + 7, col);
        return;
    }
    gfx_rect(x, y, SDG_T, SDG_T, L->fill);
    uint32_t h = hsh(c * 3, r * 5);
    if (sdg_region_at(c, r) == RG_SHRINE) {
        /* dressed stone */
        gfx_hline(x, x + SDG_T - 1, y + 7, L->lo);
        gfx_vline(x + ((r & 1) ? 4 : 11), y, y + 7, L->lo);
        gfx_vline(x + ((r & 1) ? 11 : 4), y + 8, y + 15, L->lo);
    } else {
        for (int k = 0; k < 3; k++) gfx_pset(x + (int)((h >> (k * 8)) & 15), y + (int)((h >> (k * 8 + 4)) & 15), k ? L->lo : L->speck);
    }
    if (ch == '%') gfx_dither(x, y, SDG_T, SDG_T, L->lo, 4);
    if (ch == 'X') {
        gfx_line(x + 3, y + 2, x + 8, y + 8, C_INK);
        gfx_line(x + 8, y + 8, x + 5, y + 14, C_INK);
        gfx_line(x + 8, y + 8, x + 13, y + 11, C_INK);
    }
    if (sdg_region_at(c, r) == RG_DEEP && ((h >> 3) & 3) == 0)
        gfx_circb(x + 8, y + 8, 2 + ((t / 20 + (int)h) & 1), L->hi); /* the walls breathe */
    /* edges toward the water */
    bool up = !cell_rock(c, r - 1) && SDG_MAP[iclamp(r - 1, 0, SDG_MH - 1)][c] != '^';
    bool dn = !cell_rock(c, r + 1), lf = !cell_rock(c - 1, r), rt = !cell_rock(c + 1, r);
    if (up) gfx_hline(x, x + SDG_T - 1, y, L->hi);
    if (dn) gfx_hline(x, x + SDG_T - 1, y + SDG_T - 1, L->lo);
    if (lf) gfx_vline(x, y, y + SDG_T - 1, L->hi);
    if (rt) gfx_vline(x + SDG_T - 1, y, y + SDG_T - 1, L->lo);
    /* round off outer corners */
    for (int k = 0; k < 4; k++) {
        bool a = k < 2 ? up : dn, b = (k & 1) ? rt : lf;
        if (!a || !b) continue;
        int cx = (k & 1) ? x + SDG_T - 1 : x, cy = k < 2 ? y : y + SDG_T - 1, sx = (k & 1) ? -1 : 1, sy = k < 2 ? 1 : -1;
        gfx_pset(cx, cy, L->water);
        gfx_pset(cx + sx, cy, L->water);
        gfx_pset(cx, cy + sy, L->water);
        gfx_pset(cx + sx * 2, cy, L->water);
        gfx_pset(cx, cy + sy * 2, L->water);
        gfx_pset(cx + sx, cy + sy, L->water);
    }
}

static void draw_lore(int i, int x, int y, int t) {
    switch (i) {
    case 0: gfx_circ(x, y + 3, 4, C_LIGHT); gfx_pset(x - 2, y + 3, C_INK); gfx_pset(x + 2, y + 3, C_INK); gfx_hline(x - 2, x + 2, y + 7, C_LIGHT); break;
    case 1: gfx_vline(x, y - 6, y + 6, C_GREY); gfx_hline(x - 4, x + 4, y - 4, C_GREY); gfx_line(x - 6, y + 2, x, y + 7, C_GREY); gfx_line(x + 6, y + 2, x, y + 7, C_GREY); break;
    case 2:
    case 3: gfx_rect(x - 3, y - 8, 7, 16, C_GREY); gfx_rect(x - 4, y - 10, 9, 4, C_LIGHT); gfx_pset(x, y - 8, C_INK); break;
    case 4: gfx_rect(x - 7, y + 1, 15, 5, C_SLATE); gfx_rect(x - 4, y - 2, 9, 4, C_GREY); gfx_pset(x - 5, y + 3, C_TEAL); gfx_pset(x + 3, y + 2, C_TEAL); break;
    case 5: for (int k = -6; k <= 6; k += 3) gfx_line(x + k, y + 6, x + k / 2, y - 4, C_CREAM); gfx_hline(x - 6, x + 6, y + 6, C_CREAM); break;
    default: gfx_circb(x, y, 3 + ((t / 15) & 1), C_PINK); break;
    }
}

static void draw_mob(const SdgMob *m, int t) {
    int x = (int)(m->x >> 8), y = (int)(m->y >> 8);
    int fl = m->dir < 0 ? SPR_FLIPX : 0;
    switch (m->kind) {
    case MK_NEST:
        if (m->state == MS_HOME) return; /* its eyes glow over the dark, later */
        for (int k = 0; k < 3; k++) spr_draw(sdg_sprite(SP_NIPPER), x - 8 + (k - 1) * 7, y - 8 + (k == 1 ? -5 : 2) + ((t / 6 + k) & 1), fl);
        return;
    case MK_SMOGVENT:
    case MK_CLAMPVENT:
        gfx_circ(m->hx, m->hy + 4, 4, C_INK);
        if (((t / 12) + m->hx) % 5 == 0) gfx_pset(m->hx, m->hy - 2 - (t % 12), C_ICE);
        if (m->state != MS_ACTIVE) return;
        spr_draw(sdg_sprite(m->kind == MK_SMOGVENT ? SP_SMOG : SP_CLAMPER), x - 8, y - 8, fl);
        return;
    case MK_HAUNT:
        if (m->state != MS_ACTIVE) return;
        spr_draw(sdg_sprite(SP_HAUNT), x - 8, y - 8 + ((t / 10) & 1), fl);
        return;
    case MK_WORM: {
        int n = 6;
        for (int k = 0; k < n; k++) {
            int sx = m->hx + (x - m->hx) * k / n, sy = m->hy + (y - m->hy) * k / n;
            gfx_circ(sx, sy, 3, (k & 1) ? C_RED : C_WINE);
        }
        spr_draw(sdg_sprite(SP_WORM), x - 8, y - 8, fl);
        return;
    }
    case MK_WARDEN: spr_draw(sdg_sprite(SP_WARDEN), x - 16, y - 20, 0); return;
    case MK_GLOAM:
        spr_draw(sdg_sprite(SP_ARM), x - 44, y - 24, 0);
        spr_draw(sdg_sprite(SP_ARM), x + 12, y - 24, SPR_FLIPX);
        spr_draw(sdg_sprite(SP_EYE), x - 16, y - 18, 0);
        return;
    default: break;
    }
    static const uint8_t SPR_OF[MK_COUNT] = {
        [MK_FIZZLE] = SP_FIZZLE, [MK_PRICKLE] = SP_PRICKLE, [MK_FROND] = SP_FROND, [MK_GLOB] = SP_GLOB, [MK_TINFIN] = SP_TINFIN,
        [MK_WHORL] = SP_WHORL, [MK_JELLY] = SP_JELLY, [MK_GROPER] = SP_GROPER, [MK_BURRNUT] = SP_BURRNUT,
        [MK_STILTER] = SP_STILTER, [MK_LOUSE] = SP_LOUSE, [MK_SQUID] = SP_SQUID, [MK_GRINFISH] = SP_GRINFISH,
    };
    spr_draw(sdg_sprite(SPR_OF[m->kind]), x - 8, y - 8, fl);
}

static int lit_region(int wx, int wy) { return sdg_region_at(wx / SDG_T, wy / SDG_T) == RG_STILL; }

/* the dark: only a little light around the diver (and the sun near the top,
 * and the still green room) */
static void darkness(void) {
    static const uint8_t BAYER[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
    int px = (int)(sdg.dive.x >> 8), py = (int)(sdg.dive.y >> 8);
    int r1 = SDG_LIGHT, r2 = SDG_LIGHT + 18;
    for (int sy = 0; sy < SCREEN_H; sy++) {
        int wy = cam_y + sy;
        int sun = wy < SDG_SURF_Y + 40 ? 16 : wy < SDG_SURF_Y + 104 ? 16 - (wy - SDG_SURF_Y - 40) / 4 : 0;
        uint8_t *row = g_screen.px + sy * SCREEN_W;
        int dy = wy - py;
        for (int sx = VX; sx < SCREEN_W; sx++) {
            int wx = cam_x + sx - VX;
            int dx = wx - px;
            int d2 = dx * dx + dy * dy;
            if (d2 < r1 * r1) continue;
            int lv = 0; /* 16 = fully dark */
            if (d2 < r2 * r2) lv = (sdg_isqrt(d2) - r1) * 16 / (r2 - r1);
            else lv = 16;
            lv -= sun;
            if (lv <= 0) continue;
            if ((wx & 15) == 0 || (wx & 15) == 8) {
                /* cheap test for the green room, every half cell */
            }
            if (lit_region(wx, wy)) continue;
            if (lv >= 16 || BAYER[sy & 3][sx & 3] < lv) row[sx] = C_INK;
        }
    }
}

static void draw_hud(int t) {
    gfx_rect(0, 0, VX, SCREEN_H, C_INK);
    gfx_vline(VX - 1, 0, SCREEN_H - 1, C_NAVY);
    char b[32];
    snprintf(b, sizeof b, "LV %d", P.level);
    text_draw(b, 5, 4, C_WHITE);
    int need = P.level < SDG_MAX_LEVEL ? SDG_XP_AT[P.level + 1] - SDG_XP_AT[P.level] : 1;
    int have = P.level < SDG_MAX_LEVEL ? (int)P.xp - SDG_XP_AT[P.level] : 1;
    for (int k = 0; k < 10; k++) gfx_rect(5 + k * 7, 14, 5, 3, k * need < have * 10 ? C_CYAN : C_DUSK);
    snprintf(b, sizeof b, GLYPH_COIN "%ld", (long)P.gold);
    text_draw(b, 5, 22, C_YELLOW);
    for (int d = 0; d < 3; d++) {
        int y = 36 + d * 30;
        bool dead = P.hp[d] <= 0;
        face(d, 4, y, dead);
        int mx = sdg_diver_maxhp(&P, d);
        if (dead) text_draw("DOWN", 20, y + 2, C_RED);
        else {
            snprintf(b, sizeof b, "%d", P.hp[d]);
            text_draw(b, 20, y + 2, P.hp[d] * 4 < mx ? C_RED : C_WHITE);
        }
        bar(4, y + 15, 70, 3, P.hp[d], mx, sdg.dive.hit_flash > 0 && (t & 2) ? C_WHITE : C_LIME, C_DUSK);
    }
    tiny_draw("DEPTH", 5, 130, C_GREY);
    snprintf(b, sizeof b, "%04d", sdg_depth());
    text_draw(b, 5, 137, C_ICE);
    snprintf(b, sizeof b, "%04d", P.deepest);
    text_draw(b, 42, 137, C_DUSK);
    if (sdg.dive.mist > 0) {
        tiny_draw("INK", 5, 152, C_VIOLET);
        bar(22, 153, 52, 3, (int)(sdg.dive.mist >> 8), SDG_MIST_M, C_VIOLET, C_DUSK);
    }
    if (P.npearl) {
        snprintf(b, sizeof b, "PEARLS %d", P.npearl);
        tiny_draw(b, 5, 164, C_ICE);
    }
}

void sdg_draw_dive(void) {
    int t = (int)sdg.frame;
    SdgDive *D = &sdg.dive;
    int px = (int)(D->x >> 8), py = (int)(D->y >> 8);
    cam_x = iclamp(px - VW / 2, 0, SDG_MW * SDG_T - VW);
    cam_y = iclamp(py - SCREEN_H / 2, 0, SDG_MH * SDG_T - SCREEN_H);
    gfx_clip(VX, 0, VW, SCREEN_H);
    gfx_camera(cam_x - VX, cam_y);
    int c0 = cam_x / SDG_T, r0 = cam_y / SDG_T;
    for (int r = r0; r <= r0 + SCREEN_H / SDG_T + 1 && r < SDG_MH; r++)
        for (int c = c0; c <= c0 + VW / SDG_T + 1 && c < SDG_MW; c++) draw_cell(c, r, t);
    /* the raft overhead */
    {
        int rx = (SDG_SURF_C0 + SDG_SURF_C1) / 2 * SDG_T - 40;
        gfx_rect(rx, SDG_SURF_Y - 6, 80, 6, C_BROWN);
        gfx_rect(rx + 20, SDG_SURF_Y - 26, 30, 20, C_TAN);
        gfx_rect(rx + 16, SDG_SURF_Y - 30, 38, 5, C_BROWN);
        gfx_hline(rx - 200, rx + 280, SDG_SURF_Y, C_SKY);
    }
    /* chests, levers, heads, notes on the walls */
    for (int i = 0; i < sdg_mi.nchest; i++) {
        bool open = (P.chests >> i) & 1;
        if (sdg_mi.chest_hidden[i] && !open) continue;
        spr_draw(sdg_sprite(open ? SP_CHEST_OPEN : SP_CHEST), sdg_mi.chest[i].c * SDG_T + 2, sdg_mi.chest[i].r * SDG_T + 6, 0);
    }
    for (int l = 0; l < LV_COUNT; l++)
        spr_draw(sdg_sprite((P.levers >> l) & 1 ? SP_LEVER_ON : SP_LEVER), sdg_mi.lever[l].c * SDG_T + 4, sdg_mi.lever[l].r * SDG_T + 4, 0);
    for (int h = 0; h < sdg_mi.nhead; h++)
        if (!((P.heads >> h) & 1)) spr_draw(sdg_sprite(SP_HEAD0 + h), sdg_mi.head[h].c * SDG_T + 2, sdg_mi.head[h].r * SDG_T + 2, 0);
    for (int i = 0; i < sdg_mi.nlore; i++) draw_lore(i, sdg_mi.lore[i].c * SDG_T + 8, sdg_mi.lore[i].r * SDG_T + 8, t);
    for (int i = 0; i < D->nmob; i++) {
        const SdgMob *m = &D->mob[i];
        if (m->state == MS_GONE) continue;
        draw_mob(m, t);
    }
    for (int i = 0; i < SDG_SHOTS; i++)
        if (D->shot[i].on) gfx_circ((int)(D->shot[i].x >> 8), (int)(D->shot[i].y >> 8), 2, (t & 4) ? C_PINK : C_WHITE);
    /* the diver: the first one still up leads */
    int lead = 0;
    while (lead < 2 && P.hp[lead] <= 0) lead++;
    uint8_t map[PAL_COUNT];
    pal_identity(map);
    if (lead == 0) { pal_swap(map, C_PINK, C_JADE); pal_swap(map, C_MAGENTA, C_FOREST); pal_swap(map, C_CREAM, C_LIME); }
    if (lead == 1) { pal_swap(map, C_PINK, C_YELLOW); pal_swap(map, C_MAGENTA, C_AMBER); }
    const Sprite *dv = sdg_sprite(D->moving && (t / 10) & 1 ? SP_DIVER2 : SP_DIVER);
    if (D->mist > 0) {
        if (t & 1) spr_draw_ex(dv, px - 8, py - 6, D->face < 0 ? SPR_FLIPX : 0, NULL, C_VIOLET);
    } else if (D->inv == 0 || (t & 4)) spr_draw_ex(dv, px - 8, py - 6, D->face < 0 ? SPR_FLIPX : 0, map, -1);
    gfx_camera(0, 0);
    darkness();
    /* things that glow through the dark: the nests' eyes */
    gfx_camera(cam_x - VX, cam_y);
    for (int i = 0; i < D->nmob; i++) {
        const SdgMob *m = &D->mob[i];
        if (m->kind == MK_NEST && m->state == MS_HOME && ((t / 40 + i) % 6)) {
            gfx_pset(m->hx - 3, m->hy, C_LIME);
            gfx_pset(m->hx + 2, m->hy, C_LIME);
            gfx_pset(m->hx - 1, m->hy + 3, C_LIME);
            gfx_pset(m->hx + 4, m->hy + 2, C_LIME);
        }
    }
    gfx_camera(0, 0);
    gfx_noclip();
    draw_hud(t);
    if (sdg.note_t > 0 && sdg.note1) {
        int h = sdg.note2 ? 24 : 14;
        ui_panel(VX + 6, SCREEN_H - h - 6, VW - 12, h, C_INK, C_CREAM);
        text_draw(sdg.note1, VX + 12, SCREEN_H - h - 2, C_WHITE);
        if (sdg.note2) text_draw(sdg.note2, VX + 12, SCREEN_H - h + 8, C_LIGHT);
    }
}

void sdg_draw_menu(void) {
    int t = (int)sdg.frame;
    ui_panel(VX + 8, 8, VW - 16, 150, C_INK, C_CYAN);
    char b[40];
    for (int d = 0; d < 3; d++) {
        int y = 14 + d * 30;
        bool dead = P.hp[d] <= 0;
        face(d, VX + 14, y, dead);
        text_draw(sdg_diver_name(d), VX + 30, y + 2, dead ? C_DUSK : C_WHITE);
        bool tsel = sdg.menu_pick && sdg.menu_target == d;
        if (tsel) ui_cursor(VX + 70, y + 2, t);
        snprintf(b, sizeof b, "%d", P.hp[d]);
        tiny_draw(b, VX + 30, y + 12, C_LIGHT);
        for (int s = 0; s < 2; s++) {
            int x = VX + 90 + s * 72, k = d * 2 + s;
            bool sel = !sdg.menu_pick && sdg.menu_sel == k;
            if (sel) gfx_rect(x - 2, y - 1, 70, 20, C_DUSK);
            if (sdg.menu_pick && sdg.menu_sel == k) gfx_rectb(x - 2, y - 1, 70, 20, C_YELLOW);
            int it = P.equip[d][s];
            if (!it) continue;
            sdg_draw_item_icon(it, x, y + 1);
            text_draw(SDG_ITEM[it].name, x + 11, y + 1, sdg_dive_usable(it) && !dead ? C_WHITE : C_GREY);
            if (SDG_ITEM[it].uses) {
                snprintf(b, sizeof b, "%d", P.uses[d][s]);
                tiny_draw(b, x + 11, y + 11, P.uses[d][s] ? C_CYAN : C_RED);
            }
        }
    }
    for (int r = 0; r < RL_COUNT; r++) {
        int x = VX + 14 + r * 27;
        sdg_draw_relic_icon(r, x, 108);
        snprintf(b, sizeof b, "%d", P.carry[r]);
        tiny_draw(b, x + 10, 110, P.carry[r] ? C_WHITE : C_DUSK);
    }
    for (int h = 0; h < SDG_HEADS; h++) {
        if (P.heads & (1 << h)) sdg_draw_head_icon(h, VX + 14 + h * 16, 124);
        else gfx_rectb(VX + 14 + h * 16, 124, 12, 12, C_NIGHT);
    }
    tiny_draw(sdg.menu_pick ? GLYPH_LEFT GLYPH_RIGHT " WHO  " GLYPH_A " USE  " GLYPH_B " BACK" : GLYPH_A " USE  " GLYPH_B " CLOSE", VX + 100, 146, C_GREY);
}

/* ---- the fight ------------------------------------------------------------------ */

static int foe_width(int k) { return (SDG_FOE[k].flags & FF_BOSS) ? 64 : 32; }

static void foe_layout(int *xs) {
    SdgBattle *B = &sdg.bat;
    int total = 0;
    for (int i = 0; i < B->nfoe; i++) total += foe_width(B->foe[i].kind) + (i ? 10 : 0);
    int x = 160 - total / 2;
    for (int i = 0; i < B->nfoe; i++) {
        xs[i] = x;
        x += foe_width(B->foe[i].kind) + 10;
    }
}

void sdg_draw_battle(void) {
    int t = (int)sdg.frame;
    SdgBattle *B = &sdg.bat;
    const Look *L = &LOOK[B->region < RG_COUNT ? B->region : 0];
    gfx_cls(C_INK);
    for (int y = 0; y < 102; y += 2) gfx_dither(0, y, SCREEN_W, 2, L->water, 16 - iclamp(y / 7, 0, 14));
    for (int k = 0; k < 9; k++) {
        int x = (int)(hsh(k, B->region) % 300);
        gfx_rect(x, 84 - (int)(hsh(k, 4) % 30), 18, 40, L->lo);
    }
    gfx_rect(0, 96, SCREEN_W, 6, L->fill);
    int xs[SDG_FOES];
    foe_layout(xs);
    for (int i = 0; i < B->nfoe; i++) {
        SdgFoe *f = &B->foe[i];
        if (!f->alive && f->flash == 0) continue;
        const SdgFoeDef *Dd = &SDG_FOE[f->kind];
        int sp = Dd->spr;
        if (f->kind == EN_WHORL && f->hidden) sp = SP_SHELL_IN;
        const Sprite *s = sdg_sprite(sp);
        int w = foe_width(f->kind);
        int y = 96 - s->h * 2 + ((t / 20 + i) & 1);
        uint8_t map[PAL_COUNT];
        pal_identity(map);
        if (f->kind == EN_JELLY) {
            int col = f->weak == EL_REEF ? C_YELLOW : f->weak == EL_OOZE ? C_RED : C_LIME;
            pal_swap(map, C_YELLOW, col);
        }
        int solid = (f->flash > 0 && (f->flash & 2)) ? C_WHITE : -1;
        if (!f->alive && f->flash > 0) solid = (f->flash & 2) ? C_WHITE : C_INK;
        spr_scaled_map(s, xs[i] + (w - s->w * 2) / 2, y, 2, 0, map, solid);
        if (f->weakhit > 0) {
            for (int k = 0; k < 6; k++) {
                int a = k * 5 + f->weakhit;
                int sx = xs[i] + w / 2 + (int)(hsh(k, f->weakhit) % (uint32_t)w) - w / 2;
                int sy = y + 10 + (int)((hsh(a, 2)) % 30);
                gfx_pset(sx, sy, C_WHITE);
                gfx_pset(sx + 1, sy, C_YELLOW);
                gfx_pset(sx, sy + 1, C_YELLOW);
            }
        }
        if (B->phase == BP_TARGET && B->tmode == MODE_ATTACK && B->tsel == i && (t / 8) & 1)
            text_draw(GLYPH_DOWN, xs[i] + w / 2 - 3, y - 10, C_YELLOW);
    }
    /* the line of news */
    gfx_rect(0, 103, SCREEN_W, 14, C_INK);
    gfx_hline(0, SCREEN_W - 1, 103, C_DUSK);
    if (B->nmsg > 0) text_draw(B->msg[0].text, 6, 106, C_WHITE);
    else if (B->phase == BP_WIN || B->phase == BP_LOSE || B->phase == BP_FLED) text_draw(GLYPH_A, 308, 106, (t / 10) & 1 ? C_YELLOW : C_DUSK);
    /* the orders */
    ui_panel(2, 119, 146, 59, C_INK, C_CYAN);
    if ((B->phase == BP_ORDER || B->phase == BP_TARGET) && B->cur >= 0 && B->nmsg == 0) {
        int d = B->cur;
        char b[40];
        if (B->phase == BP_ORDER) {
            for (int k = 0; k < 4; k++) {
                int y = 124 + k * 13;
                bool sel = B->menu == k;
                int col = C_LIGHT;
                if (k < 2) {
                    int it = P.equip[d][k];
                    if (it) {
                        sdg_draw_item_icon(it, 20, y);
                        col = sdg_order_valid(d, k) ? C_WHITE : C_GREY;
                        text_draw(SDG_ITEM[it].name, 32, y, sel ? C_YELLOW : col);
                        if (SDG_ITEM[it].uses) {
                            snprintf(b, sizeof b, "%d", P.uses[d][k]);
                            text_draw(b, 124, y, P.uses[d][k] ? C_CYAN : C_RED);
                        }
                    } else text_draw("-", 32, y, C_DUSK);
                } else text_draw(k == 2 ? "WAIT" : "RUN", 32, y, sel ? C_YELLOW : (k == 3 && B->boss ? C_GREY : C_LIGHT));
                if (sel) ui_cursor(8, y, t);
            }
        } else {
            int it = P.equip[d][B->menu];
            bool weapon = sdg_is_weapon(it);
            sdg_draw_item_icon(it, 10, 124);
            text_draw(SDG_ITEM[it].name, 22, 124, C_CREAM);
            const char *m0 = "ATTACK", *m1 = (SDG_ITEM[it].flags & IF_HOLY) ? "MEND" : "DEFEND";
            if (!weapon) {
                text_draw(SDG_ITEM[it].kind == K_BOMB ? "THROW AT" : "GIVE TO", 22, 140, C_YELLOW);
            } else {
                text_draw(m0, 22, 140, B->tmode == MODE_ATTACK ? C_YELLOW : C_GREY);
                text_draw(m1, 80, 140, B->tmode != MODE_ATTACK ? C_YELLOW : C_GREY);
                text_draw(GLYPH_UP GLYPH_DOWN, 128, 140, C_DUSK);
            }
            const char *who = B->tmode == MODE_ATTACK ? (B->tsel >= 0 ? SDG_FOE[B->foe[B->tsel].kind].name : "?") : sdg_diver_name(B->tsel);
            snprintf(b, sizeof b, GLYPH_LEFT " %s " GLYPH_RIGHT, who);
            text_draw(b, 22, 156, C_WHITE);
        }
    }
    /* the divers */
    ui_panel(150, 119, 168, 59, C_INK, C_CYAN);
    for (int d = 0; d < 3; d++) {
        int y = 123 + d * 18, x = 155 + (B->shake[d] > 0 ? ((B->shake[d] & 2) ? 2 : -2) : 0);
        bool dead = P.hp[d] <= 0;
        bool cur = (B->phase == BP_ORDER || B->phase == BP_TARGET) && B->cur == d;
        if (cur) gfx_rect(152, y - 2, 164, 17, C_NIGHT);
        if (B->phase == BP_TARGET && B->tmode != MODE_ATTACK && B->tsel == d && (t / 8) & 1) gfx_rectb(152, y - 2, 164, 17, C_YELLOW);
        face(d, x, y, dead);
        text_draw(sdg_diver_name(d), x + 15, y + 2, dead ? C_DUSK : C_WHITE);
        char b[24];
        int mx = sdg_diver_maxhp(&P, d);
        snprintf(b, sizeof b, "%d/%d", P.hp[d], mx);
        text_draw(b, x + 46, y + 2, dead ? C_RED : P.hp[d] * 4 < mx ? C_ORANGE : C_WHITE);
        bar(x + 110, y + 4, 44, 3, P.hp[d], mx, C_LIME, C_DUSK);
        if (B->guarding[d] >= 0 && B->phase == BP_RESOLVE) text_draw(GLYPH_STAR, x + 104, y + 2, C_CYAN);
    }
}

/* ---- surfacing, the wipe, the ending ------------------------------------------------- */

void sdg_draw_surface(void) {
    int t = (int)sdg.frame;
    raft_scene(t);
    ui_panel(40, 80, 240, 92, C_INK, C_CYAN);
    text_center("BACK AT THE RAFT", 160, 86, C_YELLOW);
    int y = 100;
    for (int i = 0; i < sdg.nsurf && i < 5; i++, y += 10) tiny_center(sdg.surf[i], 160, y, C_ICE);
    tiny_center("MENDED, REFILLED, AND THE LOG IS WRITTEN.", 160, 150, C_LIGHT);
    if (sdg.scene_t > 20) text_center(GLYPH_A, 160, 160, (t / 10) & 1 ? C_YELLOW : C_DUSK);
}

void sdg_draw_wipe(void) {
    int t = (int)sdg.frame;
    gfx_cls(C_INK);
    for (int i = 0; i < 30; i++) gfx_pset((int)(hsh(i, 8) % 320), (int)((hsh(i, 6) + (uint32_t)(sdg.scene_t / 3)) % 180), C_NIGHT);
    ui_fancy_center("LOST TO THE DARK", 160, 60, 2, DEEPG, 4, C_INK, C_NIGHT);
    tiny_center("THE SEA PUTS THEM BACK ON THE RAFT,", 160, 92, C_LIGHT);
    tiny_center("WITHOUT ANYTHING THEY FOUND SINCE THEY LAST CAME UP.", 160, 101, C_LIGHT);
    if (sdg.scene_t > 60) text_center(GLYPH_A, 160, 140, (t / 10) & 1 ? C_YELLOW : C_DUSK);
}

void sdg_draw_ending(void) {
    int t = sdg.ending_t;
    gfx_cls(C_INK);
    bool good = sdg.ending == 2;
    if (t < 240) {
        /* out of the Gloamheart's remains a pale orb with one eye rises, grinning */
        sea_backdrop((int)sdg.frame, 0, C_INK);
        int y = 150 - t / 2;
        gfx_circ(160, y, 14, C_LIGHT);
        gfx_circ(160, y - 3, 5, C_WHITE);
        gfx_circ(160, y - 3, 2, C_INK);
        gfx_line(151, y + 6, 160, y + 9, C_INK);
        gfx_line(160, y + 9, 169, y + 6, C_INK);
        tiny_center("SOMETHING PALE RISES OUT OF WHAT IS LEFT.", 160, 166, C_LIGHT);
        return;
    }
    if (good) {
        for (int y = 0; y < SCREEN_H; y++) gfx_hline(0, SCREEN_W - 1, y, y < 60 ? C_SKY : y < 120 ? C_CYAN : C_TEAL);
        for (int k = 0; k < 6; k++) gfx_dither(20 + k * 52, 0, 16, 110, C_ICE, 3);
        for (int d = 0; d < 3; d++) spr_draw(sdg_sprite(SP_FACE0 + d), 130 + d * 22, 70 + ((t / 20 + d) & 1), 0);
        for (int h = 0; h < SDG_HEADS; h++) sdg_draw_head_icon(h, 136 + h * 18, 96);
        ui_fancy_center("THE SEA IS LIT", 160, 18, 2, SEAG, 4, C_INK, C_NAVY);
        gfx_rect(40, 118, 240, 22, C_INK);
        tiny_center("THE THREE HEADS GLOW, AND THE PALE ORB SINKS AWAY.", 160, 122, C_ICE);
        tiny_center("FOR THE FIRST TIME, LIGHT REACHES ALL THE WAY DOWN.", 160, 131, C_ICE);
    } else {
        raft_scene((int)sdg.frame);
        gfx_circ(60, 26, 9, C_LIGHT);
        gfx_circ(60, 23, 3, C_WHITE);
        gfx_circ(60, 23, 1, C_INK);
        ui_fancy_center("A SECOND MOON", 160, 92, 2, SEAG, 4, C_INK, C_NAVY);
        gfx_rect(30, 114, 260, 22, C_INK);
        tiny_center("THE PALE ORB DRIFTS UP PAST THE RAFT AND HANGS IN THE SKY.", 160, 118, C_ICE);
        tiny_center("BELOW, THE SEA STAYS AS DARK AS IT EVER WAS.", 160, 127, C_ICE);
    }
    if (t > 600) {
        gfx_rect(100, 146, 120, 26, C_INK);
        text_center("THE END", 160, 150, C_YELLOW);
        text_center(GLYPH_A, 160, 161, (t / 10) & 1 ? C_WHITE : C_DUSK);
    }
}

/* ---- the cartridge label ---------------------------------------------------------- */

void sdg_draw_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_INK);
    for (int yy = 0; yy < 8; yy++) gfx_hline(x, x + w - 1, y + yy, yy < 4 ? C_PURPLE : C_VIOLET);
    gfx_hline(x, x + w - 1, y + 8, C_SKY);
    gfx_rect(x + w / 2 - 12, y + 4, 24, 4, C_BROWN);
    int cx = x + w / 2, cy = y + h / 2 + 4;
    gfx_dither_circle(cx, cy, 20, C_NAVY, 12);
    gfx_circ(cx, cy, 14, C_NAVY);
    gfx_vline(cx, y + 8, cy - 6, C_LIGHT);
    spr_draw(sdg_sprite((t / 16) & 1 ? SP_DIVER2 : SP_DIVER), cx - 8, cy - 6, 0);
    spr_draw(sdg_sprite(SP_CHEST), x + 6, y + h - 14, 0);
    if ((t / 30) & 1) { gfx_pset(x + w - 12, y + h - 12, C_LIME); gfx_pset(x + w - 8, y + h - 12, C_LIME); }
}
