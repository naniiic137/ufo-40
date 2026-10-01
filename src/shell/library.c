/* UFO 40 - the game library: 50 cartridge slots in full-size cartridges,
 * eight to a row. Five rows show at a time; the grid scrolls to follow the
 * cursor, and a bar in the left margin shows what lies above and below.
 * An empty slot holds a grey placeholder cartridge for the game still to
 * come; a cartridge not started since it arrived wears a NEW tag. */
#include "shell.h"

/* Every UFO 50 game by number, for the tribute sticker on the placeholders
 * (credit only, as on the real cartridges). */
const char *const UFO50_TITLES[GAME_SLOTS] = {
    "BARBUTA", "BUG HUNTER", "NINPEK", "PAINT CHASE", "MAGIC GARDEN",
    "MORTOL", "VELGRESS", "PLANET ZOLDATH", "ATTACTICS", "DEVILITION",
    "KICK CLUB", "AVIANOS", "MOONCAT", "BUSHIDO BALL", "BLOCK KOALA",
    "CAMOUFLAGE", "CAMPANELLA", "GOLFARIA", "THE BIG BELL RACE", "WARPTANK",
    "WALDORF'S JOURNEY", "PORGY", "ONION DELIVERY", "CARAMEL CARAMEL", "PARTY HOUSE",
    "HOT FOOT", "DIVERS", "RAIL HEIST", "VAINGER", "ROCK ON! ISLAND",
    "PINGOLF", "MORTOL II", "FIST HELL", "OVERBOLD", "CAMPANELLA 2",
    "HYPER CONTENDER", "VALBRACE", "RAKSHASA", "STAR WASPIR", "GRIMSTONE",
    "LORDS OF DISKONIA", "NIGHT MANOR", "ELFAZAR'S HAT", "PILOT QUEST", "MINI & MAX",
    "COMBATANTS", "QUIBBLE RACE", "SEASIDE DRIVE", "CAMPANELLA 3", "CYBER OWLS",
};

#define GRID_X 8
#define GRID_Y 24
#define CELL_W 20
#define CELL_H 27
#define COLS 8
#define ROWS ((GAME_SLOTS + COLS - 1) / COLS) /* 7: the last row holds 49-50 */
#define VIS_ROWS 5                            /* rows on screen at once */
#define MAX_TOP (ROWS - VIS_ROWS)
#define VIEW_Y0 18                            /* the grid's window, under the header */
#define VIEW_Y1 159                           /* ... and over the footer */
#define PANEL_X 172
#define PANEL_W 142

static int t, launch_t, shake_t, dud_t; /* dud_t: "NOT LOADED YET" on show */
static bool launching;
static int top_row;   /* first row in view */
static int scroll_px; /* drawn offset in pixels, easing toward top_row * CELL_H */

/* Slots in row r (the last row is short). */
static int row_len(int r) { return imin(COLS, GAME_SLOTS - r * COLS); }

/* Bring the cursor's row into view, moving the window as little as possible. */
static void follow_cursor(void) {
    int r = g_library_cursor / COLS;
    if (r < top_row) top_row = r;
    if (r >= top_row + VIS_ROWS) top_row = r - VIS_ROWS + 1;
    top_row = iclamp(top_row, 0, MAX_TOP);
}

static void lib_enter(void) {
    t = 0;
    launching = false;
    launch_t = 0;
    shake_t = 0;
    dud_t = 0;
    if (g_library_cursor < 0 || g_library_cursor >= GAME_SLOTS) g_library_cursor = 0;
    top_row = 0;
    follow_cursor();
    scroll_px = top_row * CELL_H; /* no slide when the screen opens */
    game_set_pausable(true);
    shell_menu_music();
}

static int available_count(void) {
    int n = 0;
    for (int i = 0; i < GAME_SLOTS; i++) n += GAMES[i] != NULL;
    return n;
}

static void lib_update(void) {
    t++;
    if (shake_t > 0) shake_t--;
    if (dud_t > 0) dud_t--;
    /* ease the view toward its row: a quarter of the way each frame */
    int want = top_row * CELL_H, d = want - scroll_px;
    if (d) scroll_px += d / 4 ? d / 4 : (d > 0 ? 1 : -1);
    if (launching) {
        launch_t++;
        if (launch_t == 26) gfx_set_flash(3);
        if (launch_t == 34) app_launch_game(g_library_cursor, true);
        return;
    }
    int c = g_library_cursor % COLS, r = g_library_cursor / COLS;
    int old = g_library_cursor;
    /* left/right wrap within the row; up/down wrap top to bottom, and a
     * column the short last row lacks lands on its last slot */
    if (btn_repeat(BTN_LEFT)) c = (c + row_len(r) - 1) % row_len(r);
    if (btn_repeat(BTN_RIGHT)) c = (c + 1) % row_len(r);
    if (btn_repeat(BTN_UP)) r = (r + ROWS - 1) % ROWS;
    if (btn_repeat(BTN_DOWN)) r = (r + 1) % ROWS;
    c = imin(c, row_len(r) - 1);
    if (r * COLS + c != old) {
        g_library_cursor = r * COLS + c;
        dud_t = 0;
        follow_cursor();
        sfx_play_name("ui_move");
    }
    if (btnp(BTN_A) || btnp(BTN_START)) {
        if (GAMES[g_library_cursor]) {
            launching = true;
            launch_t = 0;
            sfx_play_name("cart_insert");
            music_fade(30);
        } else { /* a placeholder: nothing to start */
            shake_t = 12;
            dud_t = 60;
            sfx_play_name("cart_dud");
        }
    }
    if (btnp(BTN_SELECT)) {
        sfx_play_name("ui_ok");
        shell_open_options(&SCENE_LIBRARY);
    } else if (btnp(BTN_B)) {
        sfx_play_name("ui_back");
        scene_goto(&SCENE_MENU);
    }
}

static void draw_cart(int x, int y, int idx, bool sel) {
    const GameDef *g = GAMES[idx];
    /* a placeholder is the same cartridge in greys, a shade lighter under
     * the cursor */
    int body = g ? C_LIGHT : sel ? C_GREY : C_SLATE, edge = g ? C_GREY : C_DUSK, dark = g ? C_SLATE : C_NIGHT;
    int shine = g ? C_WHITE : sel ? C_LIGHT : C_GREY;
    gfx_rect(x + 2, y, 14, 2, body);
    gfx_rect(x, y + 2, 18, 20, body);
    gfx_vline(x + 17, y + 2, y + 21, edge);
    gfx_hline(x, x + 17, y + 21, edge);
    gfx_vline(x, y + 2, y + 21, shine);
    /* grip ridges */
    for (int i = 0; i < 3; i++) gfx_hline(x + 5, x + 12, y + 1 + i * 1 + 1, i % 2 ? edge : body);
    char num[12];
    snprintf(num, sizeof num, "%02d", idx + 1);
    if (g) {
        gfx_rect(x + 2, y + 5, 14, 12, g->cart_main);
        gfx_rect(x + 2, y + 13, 14, 4, g->cart_accent);
        gfx_hline(x + 2, x + 15, y + 12, C_INK);
        tiny_draw(num, x + 4, y + 6, C_WHITE);
        /* goal pips */
        for (int b = 0; b < 3; b++) {
            bool on = (g_progress.goals[idx] >> b) & 1;
            gfx_rect(x + 4 + b * 4, y + 14, 2, 2, on ? C_YELLOW : C_INK);
        }
    } else {
        /* a blank grey label with the slot number, and a hatched strip where
         * a real cartridge has its colours and goal pips */
        gfx_rect(x + 2, y + 5, 14, 12, C_DUSK);
        gfx_hline(x + 2, x + 15, y + 12, C_INK);
        tiny_draw(num, x + 4, y + 6, sel ? C_LIGHT : C_GREY);
        for (int yy = 0; yy < 4; yy++)
            for (int xx = 0; xx < 14; xx++) gfx_pset(x + 2 + xx, y + 13 + yy, (xx + yy) % 4 == 0 ? C_SLATE : C_NIGHT);
    }
    /* contacts */
    gfx_rect(x + 3, y + 18, 12, 3, dark);
    for (int i = 0; i < 6; i++) gfx_pset(x + 4 + i * 2, y + 19, g ? C_AMBER : C_DUSK);
}

/* The NEW tag of a cartridge drawn at (x, y): a small red badge over its
 * grip, inside the cartridge's own width and the gap above it, so it never
 * reaches a neighbour. */
static void new_tag_box(int x, int y, int *bx, int *by, int *bw, int *bh) {
    *bw = tiny_width("NEW") + 4;
    *bh = 7;
    *bx = x + 9 - *bw / 2;
    *by = y - 1;
}

static void draw_new_tag(int x, int y) {
    int bx, by, bw, bh;
    new_tag_box(x, y, &bx, &by, &bw, &bh);
    ui_panel(bx, by, bw, bh, C_RED, C_INK);
    tiny_draw("NEW", bx + 2, by + 1, (t / 20) % 2 ? C_YELLOW : C_WHITE);
}

/* The scroll bar in the left margin: its arrows light up when there is more
 * above or below, and the thumb shows which five of the rows are in view. */
static void draw_scrollbar(void) {
    int x = 2, y0 = GRID_Y, y1 = GRID_Y + VIS_ROWS * CELL_H - 6;
    int track = y1 - y0 + 1;
    int thumb = track * VIS_ROWS / ROWS;
    int pos = y0 + (track - thumb) * scroll_px / (MAX_TOP * CELL_H);
    gfx_rect(x, y0, 2, track, C_NIGHT);
    gfx_rect(x, pos, 2, thumb, C_GREY);
    int lit = (t / 20) % 2 ? C_YELLOW : C_AMBER;
    int up = top_row > 0 ? lit : C_DUSK;
    int dn = top_row < MAX_TOP ? lit : C_DUSK;
    for (int i = 0; i < 3; i++) {
        gfx_hline(x - i, x + 1 + i, y0 - 5 + i, up); /* ^ over the track */
        gfx_hline(x - i, x + 1 + i, y1 + 5 - i, dn); /* v under it */
    }
}

/* The info panel under the label, in fixed slots so nothing can collide:
 *
 *    90..109  title: big, else small, else small on two lines
 *   111..115  year and genre (tiny)
 *   118..142  blurb: up to BLURB_LINES lines, word-wrapped to the panel
 *   147..163  goal icons, and the shown goal's name over two lines of words
 *
 * Every text element reports its box to the layout audit (ui_audit_*), and
 * the tests walk every cartridge with the audit on (library_layout). */
#define LABEL_Y 22
#define LABEL_H 64
#define INFO_Y 90
#define INFO_H 76 /* down to the footer's rule at 166 */
#define TITLE_Y 91
#define META_Y 111
#define BLURB_Y 118
#define BLURB_LINES 3
#define GOAL_Y 147
#define GOAL_TX 42 /* the goal's words start this far in, after the icons */
#define GOAL_LINES 2

static const uint8_t TITLE_GRAD[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};

static void draw_title(const char *title, int x, const uint8_t *grad, int n, int shadow) {
    if (ui_fancy_width(title, 2) <= PANEL_W) {
        ui_fancy_text(title, x + 1, TITLE_Y, 2, grad, n, C_INK, shadow);
        ui_audit_fancy("title", title, x + 1, TITLE_Y, 2, shadow >= 0);
    } else if (ui_fancy_width(title, 1) <= PANEL_W) {
        ui_fancy_text(title, x + 1, TITLE_Y + 4, 1, grad, n, C_INK, shadow);
        ui_audit_fancy("title", title, x + 1, TITLE_Y + 4, 1, shadow >= 0);
    } else {
        char l[2][UI_WRAP_LEN];
        int n = ui_wrap(title, PANEL_W - 3, false, l, 2);
        for (int i = 0; i < imin(n, 2); i++) {
            ui_fancy_text(l[i], x + 1, TITLE_Y + i * 10, 1, grad, n, C_INK, shadow);
            ui_audit_fancy(i ? "title line 2" : "title line 1", l[i], x + 1, TITLE_Y + i * 10, 1, shadow >= 0);
        }
        if (n > 2) ui_audit_fail("title", "needs more than two lines");
    }
}

/* The last sticker drawn, for the tests (library_sticker_has.WORD). */
static char last_sticker[160];

/* A small sticker along the bottom of the label art: one line if it fits,
 * else two. */
static void draw_sticker(const char *tribute, int tribute_no, int x, int y) {
    char tb[96];
    int room = PANEL_W - 2;
    snprintf(tb, sizeof tb, "TRIBUTE TO %s " GLYPH_DOT " UFO 50 #%d", tribute, tribute_no);
    if (tiny_width(tb) > room) snprintf(tb, sizeof tb, "TRIBUTE: %s " GLYPH_DOT " UFO 50 #%d", tribute, tribute_no);
    if (tiny_width(tb) <= room) {
        snprintf(last_sticker, sizeof last_sticker, "%s", tb);
        gfx_rect(x, y + LABEL_H - 7, PANEL_W, 7, C_INK);
        gfx_hline(x, x + PANEL_W - 1, y + LABEL_H - 8, C_NIGHT);
        tiny_center(tb, x + PANEL_W / 2, y + LABEL_H - 6, C_GREY);
        ui_audit_tiny("sticker", tb, x + PANEL_W / 2 - tiny_width(tb) / 2, y + LABEL_H - 6);
        return;
    }
    /* longer still: the sticker takes two lines */
    char t1[96], t2[24];
    snprintf(t1, sizeof t1, "TRIBUTE TO %s", tribute);
    if (tiny_width(t1) > room) snprintf(t1, sizeof t1, "TRIBUTE: %s", tribute);
    if (tiny_width(t1) > room) snprintf(t1, sizeof t1, "%s", tribute);
    snprintf(t2, sizeof t2, "UFO 50 #%d", tribute_no);
    snprintf(last_sticker, sizeof last_sticker, "%s %s", t1, t2);
    gfx_rect(x, y + LABEL_H - 14, PANEL_W, 14, C_INK);
    gfx_hline(x, x + PANEL_W - 1, y + LABEL_H - 15, C_NIGHT);
    tiny_center(t1, x + PANEL_W / 2, y + LABEL_H - 13, C_GREY);
    tiny_center(t2, x + PANEL_W / 2, y + LABEL_H - 6, C_GREY);
    ui_audit_tiny("sticker line 1", t1, x + PANEL_W / 2 - tiny_width(t1) / 2, y + LABEL_H - 13);
    ui_audit_tiny("sticker line 2", t2, x + PANEL_W / 2 - tiny_width(t2) / 2, y + LABEL_H - 6);
}

static void draw_goals(const GameDef *g, int idx, int x, int shown) {
    static const char *const names[3] = {"BEACON", "SAUCER", "ALIEN"};
    for (int b = 0; b < 3; b++) {
        int bit = 1 << b;
        ui_goal_icon(x + b * 13, GOAL_Y, bit, (g_progress.goals[idx] & bit) != 0, t);
        ui_audit_box(names[b], x + b * 13, GOAL_Y, 9, 11); /* with the marker under it */
    }
    gfx_rect(x + shown * 13, GOAL_Y + 10, 9, 1, C_YELLOW);
    int tx = x + GOAL_TX;
    tiny_draw(names[shown], tx, GOAL_Y, C_YELLOW);
    ui_audit_tiny("goal name", names[shown], tx, GOAL_Y);
    char l[GOAL_LINES][UI_WRAP_LEN];
    int n = ui_wrap(g->goal_desc[shown], PANEL_W - GOAL_TX, true, l, GOAL_LINES);
    static const char *const what[GOAL_LINES] = {"goal line 1", "goal line 2"};
    for (int i = 0; i < imin(n, GOAL_LINES); i++) {
        tiny_draw(l[i], tx, GOAL_Y + 6 + i * 6, C_GREY);
        ui_audit_tiny(what[i], l[i], tx, GOAL_Y + 6 + i * 6);
    }
    if (n > GOAL_LINES) {
        char why[160];
        snprintf(why, sizeof why, "\"%s\" needs %d lines, room for %d", g->goal_desc[shown], n, GOAL_LINES);
        ui_audit_fail(names[shown], why);
    }
}

/* The label of a placeholder: grey hatching, the slot number, COMING SOON
 * and the tribute sticker for the UFO 50 game the slot is waiting for. */
static void draw_placeholder_label(int idx, int x, int y) {
    for (int yy = 0; yy < LABEL_H; yy++)
        for (int xx = 0; xx < PANEL_W; xx++)
            gfx_pset(x + xx, y + yy, (xx + yy) % 8 < 3 ? C_NIGHT : C_INK);
    static const uint8_t grad[] = {C_LIGHT, C_LIGHT, C_GREY};
    char num[12];
    snprintf(num, sizeof num, "%02d", idx + 1);
    int nw = text_width_scaled(num, 2);
    ui_fancy_text(num, x + PANEL_W / 2 - nw / 2, y + 7, 2, grad, 3, C_INK, C_DUSK);
    ui_audit_fancy("slot number", num, x + PANEL_W / 2 - nw / 2, y + 7, 2, true);
    const char *soon = "COMING SOON";
    int sw = text_width(soon);
    ui_panel(x + PANEL_W / 2 - sw / 2 - 6, y + 28, sw + 12, 15, C_INK, C_SLATE);
    text_draw(soon, x + PANEL_W / 2 - sw / 2, y + 32, C_GREY);
    ui_audit_box("coming soon plate", x + PANEL_W / 2 - sw / 2 - 6, y + 28, sw + 12, 15);
    draw_sticker(UFO50_TITLES[idx], idx + 1, x, y);
}

/* The right-hand panel for slot idx, showing goal `shown` (0-2). */
static void draw_panel_for(int idx, int shown) {
    const GameDef *g = GAMES[idx];
    int x = PANEL_X, y = LABEL_Y;
    /* label window */
    gfx_rect(x - 1, y - 1, PANEL_W + 2, LABEL_H + 2, C_INK);
    ui_panel(x - 2, y - 2, PANEL_W + 4, LABEL_H + 4, C_INK, g ? C_GREY : C_SLATE);
    ui_audit_area("label", x, y, PANEL_W, LABEL_H);
    gfx_clip(x, y, PANEL_W, LABEL_H);
    if (g && g->draw_label) {
        g->draw_label(x, y, PANEL_W, LABEL_H, t);
        if (g->tribute) draw_sticker(g->tribute, g->tribute_no, x, y);
    } else if (!g) {
        draw_placeholder_label(idx, x, y);
    }
    gfx_noclip();
    if (!g && dud_t > 0) { /* A was pressed on it */
        const char *m = "NOT LOADED YET";
        int mw = text_width(m);
        ui_panel(x + PANEL_W / 2 - mw / 2 - 8, y + 26, mw + 16, 19, C_INK, C_GREY);
        text_draw(m, x + PANEL_W / 2 - mw / 2, y + 32, (dud_t / 8) % 2 ? C_WHITE : C_LIGHT);
    }
    ui_audit_area("info panel", x, INFO_Y, PANEL_W, INFO_H);
    if (g) {
        draw_title(g->title, x, TITLE_GRAD, 4, C_WINE);
        char meta[64];
        snprintf(meta, sizeof meta, "%s - %s", g->year, g->genre);
        tiny_draw(meta, x, META_Y, C_SKY);
        ui_audit_tiny("year and genre", meta, x, META_Y);
        char l[BLURB_LINES][UI_WRAP_LEN];
        int n = ui_wrap(g->blurb, PANEL_W, false, l, BLURB_LINES);
        static const char *const what[BLURB_LINES] = {"blurb line 1", "blurb line 2", "blurb line 3"};
        for (int i = 0; i < imin(n, BLURB_LINES); i++) {
            text_draw(l[i], x, BLURB_Y + i * LINE_H, C_LIGHT);
            ui_audit_text(what[i], l[i], x, BLURB_Y + i * LINE_H);
        }
        if (n > BLURB_LINES) {
            char why[64];
            snprintf(why, sizeof why, "needs %d lines, room for %d", n, BLURB_LINES);
            ui_audit_fail("blurb", why);
        }
        draw_goals(g, idx, x, shown);
    } else {
        /* a placeholder: the same layout, in greys, and nothing to play */
        static const uint8_t grad[] = {C_LIGHT, C_GREY, C_SLATE};
        draw_title("NOT PLAYABLE YET", x, grad, 3, C_NIGHT);
        char meta[64];
        snprintf(meta, sizeof meta, "SLOT %02d - EMPTY", idx + 1);
        tiny_draw(meta, x, META_Y, C_SLATE);
        ui_audit_tiny("slot", meta, x, META_Y);
        char l[BLURB_LINES][UI_WRAP_LEN];
        int n = ui_wrap("STILL IN THE SAUCER'S CARGO HOLD. COMING SOON.", PANEL_W, false, l, BLURB_LINES);
        static const char *const what[BLURB_LINES] = {"message line 1", "message line 2", "message line 3"};
        for (int i = 0; i < imin(n, BLURB_LINES); i++) {
            text_draw(l[i], x, BLURB_Y + i * LINE_H, C_GREY);
            ui_audit_text(what[i], l[i], x, BLURB_Y + i * LINE_H);
        }
        if (n > BLURB_LINES) ui_audit_fail("message", "needs more lines than it has room for");
        static const char *const names[3] = {"BEACON", "SAUCER", "ALIEN"};
        for (int b = 0; b < 3; b++) {
            ui_goal_icon(x + b * 13, GOAL_Y, 1 << b, false, t);
            ui_audit_box(names[b], x + b * 13, GOAL_Y, 9, 9);
        }
        n = ui_wrap("ITS GOALS ARRIVE WITH THE CARTRIDGE", PANEL_W - GOAL_TX, true, l, GOAL_LINES);
        static const char *const gwhat[GOAL_LINES] = {"goal line 1", "goal line 2"};
        for (int i = 0; i < imin(n, GOAL_LINES); i++) {
            tiny_draw(l[i], x + GOAL_TX, GOAL_Y + 1 + i * 6, C_SLATE);
            ui_audit_tiny(gwhat[i], l[i], x + GOAL_TX, GOAL_Y + 1 + i * 6);
        }
        if (n > GOAL_LINES) ui_audit_fail("goal words", "need more lines than they have room for");
    }
}

static void draw_panel(void) { draw_panel_for(g_library_cursor, (t / 150) % 3); }

/* Audits the panel for one slot with each of its goals shown; returns the
 * problems found (each printed if log). */
static int audit_slot(int idx, bool log) {
    int bad = 0;
    for (int goal = 0; goal < 3; goal++) {
        char subject[64];
        snprintf(subject, sizeof subject, "library slot %02d %s, goal %d", idx + 1,
                 GAMES[idx] ? GAMES[idx]->title : "(empty)", goal + 1);
        ui_audit_begin(subject, log);
        draw_panel_for(idx, goal);
        bad += ui_audit_end();
        if (!GAMES[idx]) break; /* an empty slot has no goals to show */
    }
    return bad;
}

/* Every slot's NEW tag must stay inside the slot's own cell of the grid
 * (the cells tile the grid with no gaps), resting or lifted under the
 * cursor; returns the problems found. */
static int audit_grid(void) {
    int bad = 0;
    for (int i = 0; i < GAME_SLOTS; i++) {
        int x = GRID_X + (i % COLS) * CELL_W, y = GRID_Y + (i / COLS) * CELL_H;
        for (int lift = 0; lift <= 3; lift += 3) {
            char subject[64];
            snprintf(subject, sizeof subject, "library grid slot %02d, lifted %d", i + 1, lift);
            ui_audit_begin(subject, true);
            ui_audit_area("cell", x - 1, y - 4, CELL_W, CELL_H);
            int bx, by, bw, bh;
            new_tag_box(x, y - lift, &bx, &by, &bw, &bh);
            ui_audit_box("NEW tag", bx, by, bw, bh);
            ui_audit_area("NEW tag", bx + 1, by + 1, bw - 2, bh - 2); /* inside its border */
            ui_audit_tiny("NEW", "NEW", bx + 2, by + 1);
            bad += ui_audit_end();
        }
    }
    return bad;
}

static void lib_draw(void) {
    ui_starfield(t, C_INK);
    /* header */
    gfx_rect(0, 0, SCREEN_W, 17, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 17, C_DUSK);
    static const uint8_t g_ufo[] = {C_YELLOW, C_AMBER, C_ORANGE};
    static const uint8_t g_40[] = {C_CYAN, C_SKY, C_BLUE};
    ui_fancy_text("UFO", 7, 5, 1, g_ufo, 3, C_INK, -1);
    ui_fancy_text("40", 27, 5, 1, g_40, 3, C_INK, -1);
    text_draw("GAME LIBRARY", 46, 5, C_LIGHT);
    char buf[48];
    snprintf(buf, sizeof buf, "%d/%d LOADED", available_count(), GAME_SLOTS);
    tiny_draw(buf, 148, 7, C_SLATE);
    snprintf(buf, sizeof buf, "%d/%d", progress_goal_count(), available_count() * 3);
    ui_goal_icon(252, 4, GOAL_SAUCER, true, t);
    text_draw(buf, 264, 5, C_YELLOW);

    /* grid */
    int shake = shake_t > 0 ? ((shake_t / 2) % 2 ? 2 : -2) : 0;
    gfx_clip(0, VIEW_Y0, PANEL_X - 3, VIEW_Y1 - VIEW_Y0);
    for (int i = 0; i < GAME_SLOTS; i++) {
        int c = i % COLS, r = i / COLS;
        int x = GRID_X + c * CELL_W, y = GRID_Y + r * CELL_H - scroll_px;
        if (y + 21 < VIEW_Y0 + 2 || y >= VIEW_Y1) continue; /* out of view */
        bool sel = i == g_library_cursor;
        int lift = 0;
        if (sel) {
            lift = 2 + ((t / 10) % 2);
            if (launching) lift = 2 + launch_t / 2;
            x += shake;
        }
        if (sel && launching && launch_t > 20 && (launch_t / 2) % 2) continue;
        if (sel) gfx_rect(x + 1, y + 21, 17, 3, C_NIGHT); /* shadow */
        draw_cart(x, y - lift, i, sel);
        if (shell_cart_is_new(i)) draw_new_tag(x, y - lift);
        if (sel && !launching) {
            int bl = (t / 16) % 2;
            int col = GAMES[i] ? C_YELLOW : C_GREY;
            int x0 = x - 2 - bl, y0 = y - lift - 2 - bl, x1 = x + 19 + bl, y1 = y - lift + 23 + bl;
            gfx_hline(x0, x0 + 3, y0, col); gfx_vline(x0, y0, y0 + 3, col);
            gfx_hline(x1 - 3, x1, y0, col); gfx_vline(x1, y0, y0 + 3, col);
            gfx_hline(x0, x0 + 3, y1, col); gfx_vline(x0, y1 - 3, y1, col);
            gfx_hline(x1 - 3, x1, y1, col); gfx_vline(x1, y1 - 3, y1, col);
        }
    }
    gfx_noclip();
    draw_scrollbar();
    draw_panel();

    /* footer */
    gfx_rect(0, 167, SCREEN_W, 13, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 166, C_DUSK);
    int fx = ui_hint(6, 170, GLYPH_A, "PLAY", C_LIGHT);
    fx = ui_hint(fx, 170, GLYPH_B, "MENU", C_LIGHT);
    fx = text_draw("SELECT", fx, 170, C_WHITE);
    text_draw("OPTIONS", fx + 4, 170, C_LIGHT);
    snprintf(buf, sizeof buf, "SLOT %02d/%d", g_library_cursor + 1, GAME_SLOTS);
    text_draw(buf, SCREEN_W - 6 - text_width(buf), 170, C_GREY);
}

bool library_query(const char *key, int *out) {
    if (!strcmp(key, "library_top")) { *out = top_row; return true; }
    if (!strcmp(key, "library_scroll")) { *out = scroll_px; return true; }
    if (!strcmp(key, "library_rows")) { *out = ROWS; return true; }
    /* layout problems in the panel for the slot under the cursor / every slot */
    if (!strcmp(key, "library_layout")) { *out = audit_slot(g_library_cursor, true); return true; }
    if (!strcmp(key, "library_grid_layout")) { *out = audit_grid(); return true; }
    /* the slot under the cursor: 1 if it holds a grey placeholder; frames
     * left of its "NOT LOADED YET" message */
    if (!strcmp(key, "library_placeholder")) { *out = GAMES[g_library_cursor] == NULL; return true; }
    if (!strcmp(key, "library_dud")) { *out = dud_t; return true; }
    /* 1 if the sticker on the cursor's label holds WORD (an _ for a space) */
    if (!strncmp(key, "library_sticker_has.", 20)) {
        char want[96];
        snprintf(want, sizeof want, "%s", key + 20);
        for (char *p = want; *p; p++)
            if (*p == '_') *p = ' ';
        last_sticker[0] = 0;
        draw_panel_for(g_library_cursor, 0);
        *out = strstr(last_sticker, want) != NULL;
        return true;
    }
    if (!strcmp(key, "library_layout_all")) {
        *out = 0;
        for (int i = 0; i < GAME_SLOTS; i++) *out += audit_slot(i, true);
        return true;
    }
    return false;
}

const Scene SCENE_LIBRARY = {"library", lib_enter, lib_update, lib_draw, NULL};
