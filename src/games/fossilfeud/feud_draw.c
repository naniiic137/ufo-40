/* FOSSIL FEUD - drawing: the title and its menus, the board of twenty
 * tiles with the side panel, the prayer cards, the action menus, the
 * battlefield, the results, the story and the ending, and the cartridge
 * label. */
#include "feud.h"

extern const char *const FEUD_SCEN_NAME[FD_SCEN];
extern const char *const FEUD_SCEN_SAY[FD_SCEN];
extern const char *const FEUD_STORY;
extern const char *const FEUD_ENDING;
extern const char *const FEUD_SECRET;

#define BX 2
#define BY 2
#define TW 48
#define TH 37
#define PX0 246

static uint8_t BLUE_MAP[256], ICE_MAP[256];
static bool maps_ready;

static void ready_maps(void) {
    if (maps_ready) return;
    pal_identity(BLUE_MAP);
    pal_swap(BLUE_MAP, C_RED, C_BLUE);
    pal_swap(BLUE_MAP, C_WINE, C_NAVY);
    pal_swap(BLUE_MAP, C_MAROON, C_NAVY);
    pal_identity(ICE_MAP);
    for (int i = 0; i < PAL_COUNT; i++) ICE_MAP[i] = C_ICE;
    ICE_MAP[C_INK] = C_SKY;
    maps_ready = true;
}

/* ---- words, with the layout audit told about them ------------------------------ */

static void txt(const char *s, int x, int y, int col) {
    text_draw(s, x, y, col);
    ui_audit_text("text", s, x, y);
}
static void ctr(const char *s, int cx, int y, int col) {
    int x = cx - text_width(s) / 2;
    text_draw(s, x, y, col);
    ui_audit_text("text", s, x, y);
}
static void tctr(const char *s, int cx, int y, int col) {
    int x = cx - tiny_width(s) / 2;
    tiny_draw(s, x, y, col);
    ui_audit_tiny("tiny", s, x, y);
}
static int wrap_ctr(const char *s, int cx, int y, int w, int col, int lh, int max) {
    char l[16][UI_WRAP_LEN];
    int n = ui_wrap(s, w, false, l, imin(max, 16));
    for (int i = 0; i < imin(n, max); i++) ctr(l[i], cx, y + i * lh, col);
    return imin(n, max);
}

/* ---- pieces -------------------------------------------------------------------------- */

static void draw_unit_sprite(int id, int x, int y, int side, int flags) {
    ready_maps();
    if (side == 1) spr_draw_ex(&feud_spr[id], x, y, flags, BLUE_MAP, -1);
    else spr_draw(&feud_spr[id], x, y, flags);
}

static void draw_terr(int kind, int x, int y, int t) {
    switch (kind) {
    case FT_MEADOW:
        for (int i = 0; i < 5; i++) {
            int gx = x + 2 + i * 4, gy = y + 6 + (i % 2) * 4;
            gfx_line(gx, gy + 3, gx - 1, gy, C_LIME);
            gfx_line(gx, gy + 3, gx + 1, gy, C_LEAF);
        }
        break;
    case FT_THICKET:
        for (int i = 0; i < 3; i++) {
            int tx = x + 4 + i * 6, ty = y + 4 + (i == 1 ? -2 : 0);
            gfx_vline(tx, ty + 4, ty + 9, C_BROWN);
            gfx_circ(tx, ty + 3, 3, C_JADE);
            gfx_pset(tx - 1, ty + 2, C_LIME);
        }
        break;
    case FT_BONEYARD:
        gfx_line(x + 2, y + 10, x + 10, y + 6, C_CREAM);
        gfx_line(x + 9, y + 11, x + 17, y + 8, C_CREAM);
        gfx_circ(x + 13, y + 4, 3, C_CREAM);
        gfx_pset(x + 12, y + 4, C_INK);
        gfx_pset(x + 14, y + 4, C_INK);
        break;
    case FT_SCREE:
        gfx_circ(x + 5, y + 9, 3, C_GREY);
        gfx_circ(x + 12, y + 10, 2, C_SLATE);
        gfx_circ(x + 15, y + 6, 3, C_GREY);
        gfx_pset(x + 4, y + 8, C_LIGHT);
        gfx_pset(x + 14, y + 5, C_LIGHT);
        break;
    case FT_GORGE:
        gfx_rect(x + 7, y + 1, 6, 12, C_INK);
        gfx_vline(x + 6, y + 1, y + 12, C_EARTH);
        gfx_vline(x + 13, y + 1, y + 12, C_EARTH);
        gfx_hline(x + 6, x + 13, y + 4, C_TAN);
        gfx_hline(x + 6, x + 13, y + 10, C_TAN);
        break;
    case FT_PEAK:
        for (int r = 0; r < 12; r++) gfx_hline(x + 10 - r, x + 10 + r * 9 / 10, y + 1 + r, r < 4 ? C_WHITE : C_GREY);
        gfx_line(x + 10, y + 1, x + 4, y + 12, C_SLATE);
        break;
    case FT_EYRIE:
        gfx_rect(x + 2, y + 6, 16, 7, C_LIGHT);
        gfx_rect(x + 7, y + 1, 6, 6, C_LIGHT);
        for (int i = 0; i < 4; i++) gfx_rect(x + 2 + i * 4, y + 4, 2, 2, C_LIGHT);
        gfx_rect(x + 9, y + 9, 2, 4, C_INK);
        gfx_pset(x + 9, y + 3, C_INK);
        gfx_hline(x + 2, x + 17, y + 12, C_GREY);
        (void)t;
        break;
    }
}

static int owner_col(int owner) { return owner == 0 ? C_MAROON : owner == 1 ? C_NAVY : C_DUSK; }

static bool hidden(const FeudTile *x) { return x->owner == 1 && fg.m.fog && !x->seen; }

static void draw_tile(int t) {
    const FeudTile *x = &fg.m.t[t];
    int tx = BX + (t % FD_COLS) * TW, ty = BY + (t / FD_COLS) * TH;
    gfx_rect(tx, ty, TW, TH, C_INK);
    gfx_rect(tx + 1, ty + 1, TW - 2, TH - 2, owner_col(x->owner));
    gfx_dither(tx + 1, ty + 1, TW - 2, TH - 2, PAL_DARKER[owner_col(x->owner)], 4);
    draw_terr(x->terr, tx + 21, ty + 9, fg.frame_t);
    if (x->eyrie && x->terr != FT_EYRIE) {
        gfx_rect(tx + 22, ty + 23, 4, 4, C_LIGHT); /* the old eyrie under the peak */
        gfx_pset(tx + 23, ty + 25, C_INK);
    }
    if (x->flag) spr_draw(&feud_spr[FSP_TOTEM], tx + 41, ty + 2, 0);
    for (int i = 0; i < x->ndrop; i++) spr_draw(&feud_spr[FSP_RES0 + x->drop[i]], tx + 41, ty + 13 + i * 6, 0);
    int bi = 0;
    /* the fog hides their buildings as well as their birds */
    for (int k = 0; k < FB_KINDS && !hidden(x); k++)
        for (int n = 0; n < x->b[k] && bi < 3; n++, bi++) spr_draw(&feud_spr[FSP_BLD0 + k], tx + 21 + bi * 8, ty + 28, 0);
    if (hidden(x)) {
        gfx_dither(tx + 2, ty + 2, 17, 30, C_INK, 8);
        text_draw("?", tx + 7, ty + 12, C_SKY);
    } else {
        int line = 0;
        char b[8];
        for (int k = 0; k < FU_KINDS; k++) {
            if (!x->u[k]) continue;
            draw_unit_sprite(FSP_MINI0 + k, tx + 2, ty + 2 + line * 6, x->owner, 0);
            snprintf(b, sizeof b, "%d", x->u[k]);
            tiny_draw(b, tx + 8, ty + 2 + line * 6, C_WHITE);
            line++;
        }
    }
    if (x->frozen) spr_draw(&feud_spr[FSP_SNOW], tx + 30, ty + 1, 0);
    if (x->seen) spr_draw(&feud_spr[FSP_EYE], tx + 21, ty + 1, 0);
}

static void outline(int t, int col, int inset) {
    int tx = BX + (t % FD_COLS) * TW, ty = BY + (t / FD_COLS) * TH;
    gfx_rectb(tx + inset, ty + inset, TW - inset * 2, TH - inset * 2, col);
}

static void draw_board(void) {
    for (int t = 0; t < FD_TILES; t++) draw_tile(t);
    uint32_t v = feud_valid_tiles();
    bool blink = (fg.frame_t / 10) % 2 == 0;
    for (int t = 0; t < FD_TILES; t++)
        if ((v >> t & 1) && blink) outline(t, C_YELLOW, 1);
    if (fg.mp == MP_ACT && (fg.au == AU_SPLIT || fg.au == AU_DEST || (fg.au == AU_TARGET && fg.tgt_stage == 1))) outline(fg.src, C_ORANGE, 1);
    if (fg.mp == MP_CPU && fg.hl_tile >= 0) { outline(fg.hl_tile, C_CYAN, 0); outline(fg.hl_tile, C_CYAN, 1); }
    if (fg.mp == MP_ACT && (fg.au == AU_MAP || fg.au == AU_DEST || fg.au == AU_TARGET)) {
        outline(fg.cur, C_WHITE, 0);
        if (blink) outline(fg.cur, C_WHITE, 2);
    }
}

static void res_row(const FeudSide *s, int y, bool hide) {
    char b[8];
    for (int r = 0; r < FR_KINDS; r++) {
        int x = PX0 + r * 24;
        spr_draw(&feud_spr[FSP_RES0 + r], x, y, 0);
        if (hide) snprintf(b, sizeof b, "?");
        else snprintf(b, sizeof b, "%d", s->res[r]);
        tiny_draw(b, x + 7, y, C_WHITE);
    }
}

static void draw_panel(void) {
    const FeudMatch *m = &fg.m;
    char b[48];
    gfx_rect(PX0 - 2, 0, SCREEN_W - PX0 + 2, 150, C_INK);
    snprintf(b, sizeof b, "TURN %d", (m->turn + 1) / 2);
    tiny_draw(b, PX0, 2, C_LIGHT);
    tiny_draw(fg.m.fog ? "FOG" : "CLEAR", PX0 + 50, 2, fg.m.fog ? C_SLATE : C_SKY);
    tiny_draw("RUST ROOST", PX0, 10, C_ORANGE);
    res_row(&m->s[0], 17, false);
    snprintf(b, sizeof b, "TOTEMS %d/%d", feud_flags_held(m, 0), FD_WIN_FLAGS);
    tiny_draw(b, PX0, 25, m->s[0].hold ? C_YELLOW : C_LIGHT);
    tiny_draw("SLATE FLOCK", PX0, 35, C_SKY);
    res_row(&m->s[1], 42, m->fog);
    snprintf(b, sizeof b, "TOTEMS %d/%d", feud_flags_held(m, 1), FD_WIN_FLAGS);
    tiny_draw(b, PX0, 50, m->s[1].hold ? C_YELLOW : C_LIGHT);
    tiny_draw("ANCESTORS", PX0, 61, C_GREY);
    for (int a = 0; a < FD_ANC; a++) {
        int y = 68 + a * 8;
        bool resting = a == m->recent[0] || a == m->recent[1];
        tiny_draw(FEUD_ANC[a].name, PX0, y, resting ? C_SLATE : FEUD_ANC[a].col);
        int stars = 0;
        for (int i = 0; i < 3; i++) stars += m->star[0][m->deck[a][i]];
        for (int i = 0; i < 6; i++) gfx_rect(PX0 + 44 + i * 4, y + 1, 3, 3, i < stars ? C_YELLOW : C_NIGHT);
    }
    bool blink = (fg.frame_t / 20) % 2 == 0;
    if (m->s[0].hold && feud_flags_held(m, 0) >= FD_WIN_FLAGS) tiny_draw("HOLD ON!", PX0, 112, blink ? C_YELLOW : C_AMBER);
    if (m->s[1].hold && feud_flags_held(m, 1) >= FD_WIN_FLAGS) tiny_draw("THEY HOLD 4!", PX0, 120, blink ? C_RED : C_PINK);
    const char *scen = fg.scenario < 0 ? "CUSTOM MATCH" : FEUD_SCEN_NAME[fg.scenario];
    char l[3][UI_WRAP_LEN];
    int n = ui_wrap(scen, 70, true, l, 3);
    for (int i = 0; i < imin(n, 3); i++) tiny_draw(l[i], PX0, 130 + i * 6, C_SLATE);
}

static void stars_at(int x, int y, int lvl) {
    for (int i = 0; i < 2; i++) spr_draw(&feud_spr[i < lvl ? FSP_STAR : FSP_STAR_OFF], x + i * 6, y, 0);
}

static void draw_pray(void) {
    const FeudMatch *m = &fg.m;
    char b[256];
    gfx_darken_rect(0, 0, 244, 150, 2);
    int resting[2], nr = 0;
    for (int i = 0; i < 2; i++)
        if (m->recent[i] >= 0) resting[nr++] = m->recent[i];
    if (nr == 2) snprintf(b, sizeof b, "RESTING: %s, %s", FEUD_ANC[resting[0]].name, FEUD_ANC[resting[1]].name);
    else if (nr == 1) snprintf(b, sizeof b, "RESTING: %s", FEUD_ANC[resting[0]].name);
    else snprintf(b, sizeof b, "THE ANCESTORS LISTEN");
    tiny_draw(b, 8, 3, C_GREY);
    for (int i = 0; i < m->noffer; i++) {
        int a = m->offer[i];
        bool sel = i == fg.pray_sel;
        int x = 6 + i * 79, y = sel ? 9 : 12;
        ui_panel(x, y, 74, 128, sel ? C_NIGHT : C_INK, sel ? C_YELLOW : C_SLATE);
        spr_draw_scaled(&feud_spr[FSP_DINO0 + a], x + 17, y + 5, 2, 0);
        int nw = text_width(FEUD_ANC[a].name);
        text_draw(FEUD_ANC[a].name, x + 37 - nw / 2, y + 36, FEUD_ANC[a].col);
        tiny_center(FEUD_ANC[a].beast, x + 37, y + 46, C_GREY);
        for (int j = 0; j < 3; j++) {
            int act = m->deck[a][j];
            snprintf(b, sizeof b, "%d %s", j + 1, FEUD_ACT[act].name);
            tiny_draw(b, x + 5, y + 58 + j * 12, C_WHITE);
            stars_at(x + 56, y + 57 + j * 12, m->star[0][act]);
        }
        snprintf(b, sizeof b, "FAVOUR %d", m->s[0].favour[a]);
        tiny_draw(b, x + 5, y + 98, C_LIGHT);
        if (feud_anc_maxed(m, 0, a)) tiny_draw("FULLY BLESSED", x + 5, y + 106, C_YELLOW);
        else if (m->s[0].favour[a] % 2 == 1) tiny_draw("BLESSES NEXT", x + 5, y + 106, C_YELLOW);
        char l[2][UI_WRAP_LEN];
        int n = ui_wrap(FEUD_ANC[a].say, 66, true, l, 2);
        for (int k = 0; k < imin(n, 2); k++) tiny_draw(l[k], x + 5, y + 114 + k * 6, C_SLATE);
    }
    /* the chosen card's three actions in full, along the bottom */
    int a = m->offer[fg.pray_sel];
    for (int j = 0; j < 3; j++) {
        int act = m->deck[a][j];
        char t[160];
        feud_level_text(m, 0, act, m->star[0][act], t, sizeof t);
        snprintf(b, sizeof b, "%d %s: %s", j + 1, FEUD_ACT[act].name, t);
        tiny_draw(b, 2, 151 + j * 7, C_LIGHT);
    }
    text_draw(feud_turn_hint(), 2, 172, C_YELLOW);
}

static void draw_bless(void) {
    const FeudMatch *m = &fg.m;
    int a = fg.bless_anc;
    char b[256];
    gfx_darken_rect(0, 0, 244, 150, 2);
    ui_panel(14, 14, 216, 122, C_NIGHT, C_YELLOW);
    spr_draw(&feud_spr[FSP_DINO0 + a], 20, 18, 0);
    snprintf(b, sizeof b, "%s BLESSES AN ACTION", FEUD_ANC[a].name);
    text_draw(b, 44, 22, FEUD_ANC[a].col);
    for (int j = 0; j < 3; j++) {
        int act = m->deck[a][j];
        bool sel = j == fg.bless_sel, maxed = m->star[0][act] >= 2;
        int y = 40 + j * 12;
        if (sel) ui_cursor(22, y, fg.frame_t);
        text_draw(FEUD_ACT[act].name, 32, y, maxed ? C_SLATE : sel ? C_WHITE : C_GREY);
        stars_at(110, y + 1, m->star[0][act]);
    }
    int act = m->deck[a][fg.bless_sel];
    int lvl = m->star[0][act];
    char t[160];
    feud_level_text(m, 0, act, lvl, t, sizeof t);
    snprintf(b, sizeof b, "NOW: %s", t);
    char l[3][UI_WRAP_LEN];
    int n = ui_wrap(b, 200, true, l, 3);
    for (int i = 0; i < imin(n, 3); i++) tiny_draw(l[i], 22, 80 + i * 7, C_LIGHT);
    if (lvl >= 2) snprintf(b, sizeof b, "FULLY BLESSED");
    else if (FEUD_ACT[act].kind == AK_MARCH && !m->shuffled) snprintf(b, sizeof b, "BLESSED: WHAT IT BRINGS IS NOT TOLD");
    else {
        feud_level_text(m, 0, act, lvl + 1, t, sizeof t);
        snprintf(b, sizeof b, "BLESSED: %s", t);
    }
    n = ui_wrap(b, 200, true, l, 3);
    for (int i = 0; i < imin(n, 3); i++) tiny_draw(l[i], 22, 104 + i * 7, C_YELLOW);
}

static void draw_menu_panel(void) {
    const FeudMatch *m = &fg.m;
    const FeudOffer *o;
    int n = feud_offers_of(m, 0, fg.act_id, &o);
    int kind = FEUD_ACT[fg.act_id].kind;
    int h = 22 + n * 12;
    ui_panel(30, 30, 184, h, C_NIGHT, C_TEAL);
    char b[96];
    if (kind == AK_ENLIST || kind == AK_BUILD)
        snprintf(b, sizeof b, "%s ON THE %s", FEUD_ACT[fg.act_id].name, FEUD_TERR_NAME[m->t[fg.cur].terr]);
    else snprintf(b, sizeof b, "%s", FEUD_ACT[fg.act_id].name);
    text_draw(b, 38, 34, C_CYAN);
    for (int i = 0; i < n; i++) {
        int y = 46 + i * 12;
        bool can = kind == AK_BARTER ? m->s[0].res[o[i].what] >= o[i].cost : feud_can_afford(m, 0, &o[i]);
        if (kind == AK_OMEN && m->s[0].res[FR_RELICS] < o[i].cost) can = false;
        feud_offer_text(m, fg.act_id, &o[i], b, sizeof b);
        if (i == fg.msel) ui_cursor(38, y, fg.frame_t);
        text_draw(b, 48, y, !can ? C_SLATE : i == fg.msel ? C_WHITE : C_GREY);
    }
}

static void draw_split(void) {
    const FeudMatch *m = &fg.m;
    int rows = 0;
    for (int k = 0; k < FU_KINDS; k++) rows += feud_movable(m, fg.src, k) > 0;
    ui_panel(44, 30, 156, 22 + rows * 12, C_NIGHT, C_ORANGE);
    text_draw(fg.omen == OM_UPDRAFT ? "WHO RIDES THE UPDRAFT" : "WHO MARCHES", 52, 34, C_ORANGE);
    int r = 0;
    char b[48];
    for (int k = 0; k < FU_KINDS; k++) {
        int mx = feud_movable(m, fg.src, k);
        if (!mx) continue;
        int y = 46 + r * 12;
        bool sel = k == fg.split_sel;
        if (sel) ui_cursor(52, y, fg.frame_t);
        draw_unit_sprite(FSP_MINI0 + k, 62, y + 1, 0, 0);
        text_draw(FEUD_UNIT[k].plural, 70, y, sel ? C_WHITE : C_GREY);
        snprintf(b, sizeof b, GLYPH_LEFT "%2d/%d" GLYPH_RIGHT, fg.split[k], mx);
        text_draw(b, 150, y, sel ? C_YELLOW : C_LIGHT);
        r++;
    }
}

static void draw_anc_pick(void) {
    const FeudMatch *m = &fg.m;
    ui_panel(20, 40, 204, 64, C_NIGHT, C_VIOLET);
    text_draw("TITHE TO WHICH ANCESTOR?", 30, 44, C_VIOLET);
    for (int a = 0; a < FD_ANC; a++) {
        int x = 26 + a * 39;
        bool sel = a == fg.anc_sel;
        spr_draw(&feud_spr[FSP_DINO0 + a], x, 58, 0);
        if (sel) gfx_rectb(x - 2, 56, 24, 18, C_YELLOW);
        char b[16];
        snprintf(b, sizeof b, "%d", m->s[0].favour[a]);
        tiny_draw(b, x + 8, 76, C_LIGHT);
    }
    tiny_center(FEUD_ANC[fg.anc_sel].name, 122, 88, FEUD_ANC[fg.anc_sel].col);
}

static const char *act_header(char *b, int n) {
    const FeudMatch *m = &fg.m;
    switch (fg.mp) {
    case MP_BANNER: snprintf(b, (size_t)n, " "); break;
    case MP_PRAY: snprintf(b, (size_t)n, "PRAY TO AN ANCESTOR"); break;
    case MP_BLESS: snprintf(b, (size_t)n, "A BLESSING"); break;
    case MP_CPU: snprintf(b, (size_t)n, "THE SLATE FLOCK'S TURN"); break;
    case MP_ACT: {
        const char *what = "";
        if (fg.au == AU_DEST) what = fg.omen == OM_UPDRAFT ? ": FLY WHERE?" : ": MARCH WHERE?";
        else if (fg.au == AU_TARGET) what = fg.omen == OM_SHIFT && fg.tgt_stage == 1 ? ": WHERE TO?" : ": WHICH TILE?";
        snprintf(b, (size_t)n, "%d/3 %s%s  (%s)", fg.act_i + 1, FEUD_ACT[fg.act_id].name, what, FEUD_ANC[fg.anc].name);
        break;
    }
    default: b[0] = 0; break;
    }
    (void)m;
    return b;
}

static void draw_match(void) {
    const FeudMatch *m = &fg.m;
    gfx_cls(C_INK);
    draw_board();
    draw_panel();
    char b[256];
    if (fg.mp == MP_PRAY) { draw_pray(); return; }
    if (fg.mp == MP_BLESS) draw_bless();
    if (fg.mp == MP_ACT) {
        if (fg.au == AU_MENU) draw_menu_panel();
        if (fg.au == AU_SPLIT) draw_split();
        if (fg.au == AU_ANC) draw_anc_pick();
    }
    if (fg.mp == MP_BANNER) {
        ui_panel(52, 56, 140, 34, C_NIGHT, m->side == 0 ? C_ORANGE : C_SKY);
        snprintf(b, sizeof b, "TURN %d", (m->turn + 1) / 2);
        text_center(b, 122, 61, C_LIGHT);
        text_center(m->side == 0 ? "YOUR FLOCK" : "THE SLATE FLOCK", 122, 75, m->side == 0 ? C_ORANGE : C_SKY);
    }
    text_draw(act_header(b, sizeof b), 2, 151, fg.mp == MP_CPU ? C_SKY : C_WHITE);
    if (fg.mp == MP_ACT) {
        char t[160];
        feud_level_text(m, 0, fg.act_id, m->star[0][fg.act_id], t, sizeof t);
        tiny_draw(t, 2, 160, C_GREY);
        stars_at(SCREEN_W - 14, 152, m->star[0][fg.act_id]);
    }
    if (fg.note[0]) tiny_draw(fg.note, 2, 166, fg.msg_t > 0 ? C_WHITE : C_GREY);
    text_draw(feud_turn_hint(), 2, 172, C_YELLOW);
    if (fg.secret_t > 0) {
        ui_panel(30, 50, 184, 44, C_INK, C_PURPLE);
        char l[4][UI_WRAP_LEN];
        int n = ui_wrap(FEUD_SECRET, 172, false, l, 4);
        for (int i = 0; i < imin(n, 4); i++) text_center(l[i], 122, 55 + i * 10, C_VIOLET);
    }
}

/* ---- the battlefield ---------------------------------------------------------------- */

#define FX0 10
#define FY0 34

static void field_ground(const FeudBattle *b) {
    static const uint8_t GROUND[FT_KINDS] = {C_FOREST, C_FOREST, C_EARTH, C_BROWN, C_HIDE, C_SLATE, C_SLATE};
    int g = GROUND[b->terr];
    gfx_rect(FX0, FY0, FB_FW, FB_FH, g);
    gfx_dither(FX0, FY0, FB_FW, FB_FH, PAL_DARKER[g], 3);
    for (int cy = 0; cy < FB_GH; cy++)
        for (int cx = 0; cx < FB_GW; cx++) {
            int x = FX0 + cx * FB_CELL, y = FY0 + cy * FB_CELL, c = b->grid[cy][cx];
            if (c == BC_HIGH) gfx_dither(x, y, FB_CELL, FB_CELL, PAL_LIGHTER[g], 6);
            else if (c == BC_COVER) {
                if (b->terr == FT_BONEYARD) {
                    gfx_line(x + 1, y + 7, x + 8, y + 3, C_CREAM);
                    gfx_circ(x + 2, y + 7, 1, C_CREAM);
                    gfx_circ(x + 8, y + 3, 1, C_CREAM);
                } else {
                    gfx_circ(x + 5, y + 5, 5, C_JADE);
                    gfx_circ(x + 4, y + 4, 2, C_LEAF);
                }
            } else if (c == BC_ROCK) {
                gfx_circ(x + 5, y + 5, 5, C_GREY);
                gfx_circ(x + 4, y + 4, 2, C_LIGHT);
            } else if (c == BC_BLOCK) {
                if (b->terr == FT_GORGE) gfx_rect(x, y, FB_CELL, FB_CELL, C_INK);
                else if (b->terr == FT_EYRIE) {
                    gfx_rect(x, y, FB_CELL, FB_CELL, C_LIGHT);
                    gfx_hline(x, x + 9, y + 4, C_GREY);
                    gfx_vline(x + (cy % 2) * 5, y, y + 4, C_GREY);
                    gfx_vline(x + 5 - (cy % 2) * 5 + 2, y + 5, y + 9, C_GREY);
                } else {
                    gfx_circ(x + 5, y + 5, 5, C_GREY);
                    gfx_circ(x + 4, y + 4, 2, C_LIGHT);
                }
            }
        }
    if (b->terr == FT_GORGE)
        for (int cy = 0; cy < FB_GH; cy++)
            if (b->grid[cy][14] != BC_BLOCK && b->grid[cy][15] != BC_BLOCK && b->grid[cy][14] != BC_BLOCK) {
                int x = FX0 + 14 * FB_CELL, y = FY0 + cy * FB_CELL;
                gfx_rect(x, y + 2, 20, 6, C_TAN);
                for (int i = 0; i < 20; i += 3) gfx_vline(x + i, y + 2, y + 7, C_BROWN);
            }
}

static void draw_battle(void) {
    const FeudBattle *b = &fg.b;
    ready_maps();
    gfx_cls(C_INK);
    char s[96];
    snprintf(s, sizeof s, "BATTLE FOR THE %s", FEUD_TERR_NAME[b->terr]);
    text_center(s, 160, 3, C_WHITE);
    snprintf(s, sizeof s, "RUST ROOST %d", feud_battle_count(b, 0));
    tiny_draw(s, FX0, 14, C_ORANGE);
    snprintf(s, sizeof s, "SLATE FLOCK %d", feud_battle_count(b, 1));
    tiny_draw(s, FX0 + FB_FW - tiny_width(s), 14, C_SKY);
    tiny_center(b->atk == 0 ? "WE ATTACK" : "WE DEFEND", 160, 14, C_LIGHT);
    if (b->surprise && !b->woke) tiny_center("SURPRISE: THE DEFENDERS HAVEN'T SEEN US", 160, 22, C_YELLOW);
    else if (b->frozen_def) tiny_center("THE DEFENDERS ARE FROZEN", 160, 22, C_ICE);
    gfx_clip(FX0, FY0, FB_FW, FB_FH);
    field_ground(b);
    for (int k = 0; k < b->nbld; k++) {
        const FeudFieldBld *f = &b->bld[k];
        if (f->hp <= 0) {
            gfx_rect(FX0 + f->x - 3, FY0 + f->y + 1, 7, 3, C_BROWN);
            continue;
        }
        spr_draw(&feud_spr[FSP_BLD0 + f->kind], FX0 + f->x - 3, FY0 + f->y - 3, 0);
        gfx_rect(FX0 + f->x - 3, FY0 + f->y + 5, 7 * f->hp / FB_BLD_HP, 1, C_LIME);
    }
    /* walkers first, fliers over them */
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < FB_MAX; i++) {
            const FeudSoldier *x = &b->s[i];
            if (!x->alive || (x->kind == FU_SWIFT) != (pass == 1)) continue;
            int px = FX0 + (x->x >> 8) - 4, py = FY0 + (x->y >> 8) - 8;
            if (x->kind == FU_SWIFT) py -= 6 + ((b->t + i * 5) / 6) % 2;
            int frame = x->cd > FEUD_UNIT[x->kind].every - 10 ? 1 : x->kind == FU_SWIFT ? (b->t / 6 + i) % 2 : 0;
            int id = FSP_UNIT0 + x->kind * 2 + frame;
            int fl = x->face < 0 ? SPR_FLIPX : 0;
            if (x->frozen) spr_draw_ex(&feud_spr[id], px, py, fl, ICE_MAP, -1);
            else if (x->hit_t > 4) spr_draw_ex(&feud_spr[id], px, py, fl, NULL, C_WHITE);
            else if (x->side == 1) spr_draw_ex(&feud_spr[id], px, py, fl, BLUE_MAP, -1);
            else spr_draw(&feud_spr[id], px, py, fl);
            if (x->kind == FU_SWIFT) gfx_pset(px + 4, FY0 + (x->y >> 8), C_INK);
        }
    for (int i = 0; i < FB_MAX_SHOTS; i++) {
        const FeudShot *h = &b->sh[i];
        if (!h->alive) continue;
        int x = FX0 + (h->x >> 8), y = FY0 + (h->y >> 8);
        if (h->kind == FU_SLINGER) gfx_line(x, y, x - h->vx / 128, y - h->vy / 128, C_WHITE);
        else if (h->kind == FU_MYSTIC) { gfx_circ(x, y, 2, C_VIOLET); gfx_pset(x, y, C_WHITE); }
        else gfx_circ(x, y, 1, C_AMBER);
    }
    gfx_noclip();
    gfx_rectb(FX0 - 1, FY0 - 1, FB_FW + 2, FB_FH + 2, C_SLATE);
    /* the orders */
    static const char *const ORD[FO_KINDS] = {"CHARGE", "STAND", "WRECK", "FLEE"};
    int x = 14;
    for (int o = 0; o < FO_KINDS; o++) {
        bool on = b->order[0] == o;
        int w = text_width(ORD[o]);
        if (on) gfx_rect(x - 3, 149, w + 6, 11, C_MAROON);
        text_draw(ORD[o], x, 151, on ? C_WHITE : C_GREY);
        x += w + 14;
    }
    snprintf(s, sizeof s, "THEIRS: %s", ORD[b->order[1]]);
    tiny_draw(s, SCREEN_W - 10 - tiny_width(s), 153, C_SKY);
    if (b->result != BR_NONE) {
        bool ours = (b->result == BR_ATK_WIN || b->result == BR_DEF_FLED) == (b->atk == 0);
        if (b->result == BR_ATK_BACK) ours = b->atk != 0;
        const char *r;
        if (b->result == BR_ATK_WIN || b->result == BR_DEF_FLED) r = b->atk == 0 ? "THE TILE IS OURS" : "THE TILE IS LOST";
        else if (b->result == BR_ATK_BACK) r = b->atk == 0 ? "WE PULL BACK" : "THEY PULL BACK";
        else r = b->atk == 0 ? "THE ATTACK FAILS" : "WE HOLD";
        text_center(r, 160, 164, ours ? C_YELLOW : C_PINK);
    } else {
        text_draw(feud_turn_hint(), 14, 166, C_YELLOW);
    }
}

/* ---- the menus ------------------------------------------------------------------------ */

static const uint8_t GRAD_BONE[4] = {C_WHITE, C_CREAM, C_TAN, C_EARTH};

static void backdrop(int t) {
    gfx_cls(C_INK);
    gfx_rect(0, 0, SCREEN_W, 96, C_NIGHT);
    gfx_dither(0, 96, SCREEN_W, 30, C_NIGHT, 8);
    gfx_dither(0, 126, SCREEN_W, 24, C_NIGHT, 3);
    for (int i = 0; i < 24; i++) {
        int x = (i * 53 + 17) % SCREEN_W, y = (i * 31 + 7) % 80;
        if ((t / 30 + i) % 5) gfx_pset(x, y, i % 3 ? C_SLATE : C_LIGHT);
    }
    /* the ground, and the bones in it */
    gfx_rect(0, 150, SCREEN_W, 30, C_EARTH);
    gfx_dither(0, 150, SCREEN_W, 30, C_BROWN, 6);
    for (int i = 0; i < 8; i++) {
        int x = 10 + i * 40, y = 160 + (i % 3) * 5;
        gfx_line(x, y, x + 10, y - 2, C_TAN);
        gfx_circ(x, y, 1, C_TAN);
        gfx_circ(x + 10, y - 2, 1, C_TAN);
    }
}

static void draw_title(void) {
    int t = fg.frame_t;
    backdrop(t);
    ui_fancy_center("FOSSIL FEUD", 160, 12, 3, GRAD_BONE, 4, C_INK, C_EARTH);
    ui_audit_fancy("logo", "FOSSIL FEUD", 160 - ui_fancy_width("FOSSIL FEUD", 3) / 2, 12, 3, true);
    ctr("BIRDS AT WAR OVER THE TOTEM FIELDS", 160, 44, C_CREAM);
    spr_draw_scaled(&feud_spr[FSP_BIRD_BIG], 30, 98 + ((t / 20) % 2), 3, 0);
    spr_draw_scaled(&feud_spr[FSP_BIRD_BIG2], 242, 98 + ((t / 20 + 1) % 2), 3, 0);
    static const char *const items[4] = {"CAMPAIGN", "CUSTOM MATCH", "2 PLAYERS", "RECORDS"};
    ui_panel(104, 58, 112, 54, C_NIGHT, C_AMBER);
    for (int i = 0; i < 4; i++) {
        int y = 63 + i * 12;
        bool s = i == fg.title_sel;
        int col = s ? C_WHITE : C_GREY;
        if (i == 2) col = s ? C_GREY : C_SLATE; /* not in this release */
        ctr(items[i], 160, y, col);
        if (s) ui_cursor(160 - text_width(items[i]) / 2 - 10, y, t);
    }
    if (fg.title_sel == 2) tctr("2 PLAYERS " GLYPH_DOT " NEXT UPDATE", 160, 120, C_LIGHT);
    tctr("1985 BEAMDOWN SOFTWORKS", 160, 172, C_TAN);
}

static void draw_campaign(void) {
    backdrop(fg.frame_t);
    ui_fancy_center("THE CAMPAIGN", 160, 4, 2, GRAD_BONE, 4, C_INK, C_EARTH);
    ui_audit_fancy("head", "THE CAMPAIGN", 160 - ui_fancy_width("THE CAMPAIGN", 2) / 2, 4, 2, true);
    ui_panel(40, 26, 240, 104, C_NIGHT, C_AMBER);
    for (int s = 0; s < FD_SCEN; s++) {
        int y = 31 + s * 12;
        bool sel = s == fg.camp_sel;
        bool open = s == 0 || (fsv.beaten >> (s - 1) & 1);
        bool done = fsv.beaten >> s & 1;
        if (sel) ui_cursor(46, y, fg.frame_t);
        txt(done ? GLYPH_CHECK : open ? " " : GLYPH_LOCK, 58, y, done ? C_LIME : C_SLATE);
        txt(FEUD_SCEN_NAME[s], 70, y, !open ? C_SLATE : sel ? C_WHITE : C_GREY);
    }
    bool open = fg.camp_sel == 0 || (fsv.beaten >> (fg.camp_sel - 1) & 1);
    wrap_ctr(open ? FEUD_SCEN_SAY[fg.camp_sel] : (fg.camp_sel < 3 ? "WIN THE ONE BEFORE TO OPEN THIS." : "WIN THE ONE BEFORE TO OPEN THIS ORDEAL."),
             160, 134, 300, C_CREAM, 10, 2);
}

static void draw_custom(void) {
    backdrop(fg.frame_t);
    ui_fancy_center("CUSTOM MATCH", 160, 4, 2, GRAD_BONE, 4, C_INK, C_EARTH);
    ui_audit_fancy("head", "CUSTOM MATCH", 160 - ui_fancy_width("CUSTOM MATCH", 2) / 2, 4, 2, true);
    ui_panel(50, 30, 220, 62, C_NIGHT, C_AMBER);
    char v[4][32];
    snprintf(v[0], 32, "%d", fsv.eyries);
    snprintf(v[1], 32, "%d", fsv.peaks);
    snprintf(v[2], 32, "%s", fsv.fog ? "ON" : "OFF");
    v[3][0] = 0;
    static const char *const ROW[4] = {"EYRIES", "PEAKS", "FOG OF WAR", "BEGIN"};
    for (int i = 0; i < 4; i++) {
        int y = 36 + i * 13;
        bool s = i == fg.cust_sel;
        if (s) ui_cursor(56, y, fg.frame_t);
        txt(ROW[i], 68, y, s ? C_WHITE : C_GREY);
        if (v[i][0]) {
            char b[200];
            snprintf(b, sizeof b, GLYPH_LEFT " %s " GLYPH_RIGHT, v[i]);
            txt(b, 262 - text_width(b), y, s ? C_YELLOW : C_LIGHT);
        }
    }
    const char *say = fg.cust_sel == 2 ? "WITH THE FOG OFF YOU SEE EVERY BIRD, AND THREEHORN'S OMENS CHANGE."
                      : fg.cust_sel == 0 ? "EVERY EYRIE BEARS A TOTEM. FEWER EYRIES, MORE TOTEMS ON OPEN GROUND."
                                         : "ONE MATCH AGAINST THE SLATE FLOCK'S ELDERS, YOUR WAY.";
    wrap_ctr(say, 160, 104, 280, C_CREAM, 10, 3);
}

static void draw_records(void) {
    backdrop(fg.frame_t);
    ui_fancy_center("RECORDS", 160, 4, 2, GRAD_BONE, 4, C_INK, C_EARTH);
    ui_audit_fancy("head", "RECORDS", 160 - ui_fancy_width("RECORDS", 2) / 2, 4, 2, true);
    ui_panel(40, 30, 240, 76, C_NIGHT, C_AMBER);
    txt("PRAYERS TO EACH ANCESTOR", 52, 35, C_AMBER);
    char b[64];
    for (int a = 0; a < FD_ANC; a++) {
        int y = 48 + a * 11;
        spr_draw(&feud_spr[FSP_DINO0 + a], 52, y - 4, 0);
        txt(FEUD_ANC[a].name, 78, y, FEUD_ANC[a].col);
        snprintf(b, sizeof b, "%d", fsv.picks[a]);
        txt(b, 268 - text_width(b), y, C_WHITE);
    }
}

static void draw_story(void) {
    backdrop(fg.frame_t);
    for (int a = 0; a < FD_ANC; a++) spr_draw(&feud_spr[FSP_DINO0 + a], 30 + a * 56, 152 + (a % 2) * 6, 0);
    wrap_ctr(FEUD_STORY, 160, 14, 290, C_CREAM, 10, 13);
    if (fg.state_t > 20) ctr(GLYPH_A " BEGIN", 160, 138, C_YELLOW);
}

static void draw_result(void) {
    backdrop(fg.frame_t);
    const char *head = fg.won ? "THE TOTEMS ARE YOURS" : "THE ROOST HAS FALLEN";
    ctr(head, 160, 40, fg.won ? C_YELLOW : C_PINK);
    const char *scen = fg.scenario < 0 ? "CUSTOM MATCH" : FEUD_SCEN_NAME[fg.scenario];
    ctr(scen, 160, 56, C_CREAM);
    char b[64];
    snprintf(b, sizeof b, "%d TURNS", (fg.m.turn + 1) / 2);
    ctr(b, 160, 70, C_LIGHT);
    spr_draw_scaled(&feud_spr[fg.won ? FSP_BIRD_BIG : FSP_BIRD_BIG2], 136, 86, 3, 0);
    if (fg.state_t > 90) ctr(GLYPH_A " CONTINUE", 160, 138, C_YELLOW);
}

static void draw_ending(void) {
    backdrop(fg.frame_t);
    int shown = imin(fg.state_t / 2, 1000);
    char buf[700];
    snprintf(buf, sizeof buf, "%s", FEUD_ENDING);
    int n = (int)strlen(buf);
    if (shown < n) buf[shown] = 0;
    wrap_ctr(buf, 160, 12, 290, C_CREAM, 10, 13);
    if (fg.state_t > 600) tctr("THE END", 160, 140, C_YELLOW);
}

void feud_draw(void) {
    switch (fg.state) {
    case FS_TITLE: draw_title(); break;
    case FS_CAMPAIGN: draw_campaign(); break;
    case FS_CUSTOM: draw_custom(); break;
    case FS_RECORDS: draw_records(); break;
    case FS_STORY: draw_story(); break;
    case FS_MATCH: draw_match(); break;
    case FS_BATTLE: draw_battle(); break;
    case FS_RESULT: draw_result(); break;
    case FS_ENDING: draw_ending(); break;
    default: gfx_cls(C_INK); break;
    }
}

void feud_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    for (int k = 0; k < h; k++) gfx_hline(x, x + w - 1, y + k, k < h / 2 ? C_NIGHT : k < h - 12 ? C_DUSK : C_EARTH);
    spr_draw(&feud_spr[FSP_DINO0], x + w / 2 - 10, y + h - 26, 0);
    spr_draw(&feud_spr[FSP_BIRD_BIG], x + 8, y + h - 30 + (t / 16) % 2, 0);
    spr_draw(&feud_spr[FSP_BIRD_BIG2], x + w - 24, y + h - 30 + (t / 16 + 1) % 2, 0);
    spr_draw(&feud_spr[FSP_TOTEM], x + w / 2 - 3, y + h - 40, 0);
    ui_fancy_text("FOSSIL FEUD", x + 6, y + 4, 1, GRAD_BONE, 4, C_INK, C_EARTH);
    gfx_noclip();
}

/* tests: the menus' words stay on the screen and clear of each other */
int feud_layout_audit(void) {
    int bad = 0;
    static FeudGame keep;
    keep = fg;
    static const int SCREENS[] = {FS_TITLE, FS_TITLE, FS_CAMPAIGN, FS_CUSTOM, FS_RECORDS, FS_STORY, FS_RESULT, FS_RESULT, FS_ENDING};
    for (int i = 0; i < ARRAY_LEN(SCREENS); i++) {
        fg.state = SCREENS[i];
        fg.state_t = 2000;
        fg.title_sel = i == 1 ? 2 : 0;
        fg.won = i == 6;
        for (int c = 0; c < (SCREENS[i] == FS_CAMPAIGN ? FD_SCEN : SCREENS[i] == FS_CUSTOM ? 4 : 1); c++) {
            fg.camp_sel = c;
            fg.cust_sel = c;
            ui_audit_begin("fossilfeud", true);
            ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
            feud_draw();
            bad += ui_audit_end();
        }
    }
    fg = keep;
    return bad;
}
