/* UFO 40 - SAVE DATA: every loaded cartridge with its save and its goals.
 * Delete one cartridge's save, reset one cartridge's goals, or delete all
 * data (two confirmations, NO is always the default). Shows where the saves
 * live on this system. */
#include "shell.h"

enum { STEP_LIST, STEP_ACTIONS, STEP_CONFIRM };
enum { ACT_DELETE_SAVE, ACT_RESET_GOALS, ACT_CANCEL, ACT_COUNT };
enum { ASK_NONE, ASK_DELETE_SAVE, ASK_RESET_GOALS, ASK_ALL_1, ASK_ALL_2 };

#define ROW_H 10
#define VISIBLE 12
#define LIST_Y 30

static int rows[GAME_SLOTS + 1], n_rows; /* cartridge slots, then -1 = DELETE ALL DATA */
static int states[GAME_SLOTS]; /* SAVE_NONE, SAVE_OK or SAVE_DAMAGED */
static int t, sel, top, step, act_sel, ask, yes, msg_t;
static const char *msg;

static void refresh(void) {
    n_rows = 0;
    for (int i = 0; i < GAME_SLOTS; i++)
        if (GAMES[i]) {
            rows[n_rows++] = i;
            states[i] = shell_save_state(i);
        }
    rows[n_rows++] = -1;
    if (sel >= n_rows) sel = n_rows - 1;
}

static void sd_enter(void) {
    t = 0;
    step = STEP_LIST;
    ask = ASK_NONE;
    msg_t = 0;
    refresh();
    shell_menu_music();
}

static int cart(void) { return rows[sel]; }

static bool act_enabled(int a) {
    int g = cart();
    if (g < 0) return false;
    if (a == ACT_DELETE_SAVE) return states[g] != SAVE_NONE; /* a damaged file can go too */
    if (a == ACT_RESET_GOALS) return g_progress.goals[g] != 0;
    return true;
}

static void say(const char *m) {
    msg = m;
    msg_t = 150;
}

static void do_ask(void) {
    int g = cart();
    switch (ask) {
    case ASK_DELETE_SAVE:
        shell_delete_save(g);
        say("SAVE DELETED");
        break;
    case ASK_RESET_GOALS:
        shell_reset_goals(g);
        say("GOALS RESET");
        break;
    case ASK_ALL_2:
        shell_delete_all();
        say("ALL DATA DELETED");
        break;
    }
    refresh();
}

static void sd_update(void) {
    t++;
    if (msg_t > 0) msg_t--;
    if (scene_transitioning()) return;
    switch (step) {
    case STEP_LIST:
        if (btn_repeat(BTN_UP)) { sel = (sel + n_rows - 1) % n_rows; sfx_play_name("ui_move"); }
        if (btn_repeat(BTN_DOWN)) { sel = (sel + 1) % n_rows; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { sfx_play_name("ui_back"); scene_goto(&SCENE_MENU); return; }
        if (btnp(BTN_A) || btnp(BTN_START)) {
            sfx_play_name("ui_ok");
            msg_t = 0;
            if (cart() < 0) { step = STEP_CONFIRM; ask = ASK_ALL_1; yes = 0; }
            else {
                step = STEP_ACTIONS;
                act_sel = act_enabled(ACT_DELETE_SAVE) ? ACT_DELETE_SAVE : act_enabled(ACT_RESET_GOALS) ? ACT_RESET_GOALS : ACT_CANCEL;
            }
        }
        break;
    case STEP_ACTIONS:
        if (btn_repeat(BTN_UP)) { act_sel = (act_sel + ACT_COUNT - 1) % ACT_COUNT; sfx_play_name("ui_move"); }
        if (btn_repeat(BTN_DOWN)) { act_sel = (act_sel + 1) % ACT_COUNT; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { step = STEP_LIST; sfx_play_name("ui_back"); break; }
        if (btnp(BTN_A)) {
            if (act_sel == ACT_CANCEL) { step = STEP_LIST; sfx_play_name("ui_back"); break; }
            if (!act_enabled(act_sel)) { sfx_play_name("ui_error"); break; }
            sfx_play_name("ui_ok");
            step = STEP_CONFIRM;
            ask = act_sel == ACT_DELETE_SAVE ? ASK_DELETE_SAVE : ASK_RESET_GOALS;
            yes = 0;
        }
        break;
    case STEP_CONFIRM:
        if (btnp(BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN)) { yes ^= 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_B) || (btnp(BTN_A) && !yes)) {
            sfx_play_name("ui_back");
            step = ask == ASK_ALL_1 || ask == ASK_ALL_2 ? STEP_LIST : STEP_ACTIONS;
            ask = ASK_NONE;
            break;
        }
        if (btnp(BTN_A) && yes) {
            if (ask == ASK_ALL_1) { /* the second, last question */
                ask = ASK_ALL_2;
                yes = 0;
                sfx_play_name("ui_error");
                break;
            }
            do_ask();
            sfx_play_name("ui_back");
            step = STEP_LIST;
            ask = ASK_NONE;
        }
        break;
    }
    if (sel < top) top = sel;
    if (sel >= top + VISIBLE) top = sel - VISIBLE + 1;
}

/* ---- drawing ------------------------------------------------------------ */

/* Every text element reports its box to the layout audit (ui_audit_*); the
 * savedata_layout query draws each row, detail and question for every
 * cartridge with the audit on. */

static void goal_pips(int x, int y, int g) {
    for (int b = 0; b < 3; b++) {
        bool on = (g_progress.goals[g] >> b) & 1;
        gfx_rect(x + b * 5, y, 3, 3, on ? C_YELLOW : C_INK);
        if (!on) gfx_rectb(x + b * 5, y, 3, 3, C_DUSK);
    }
    ui_audit_box("goal pips", x, y, 13, 3);
}

/* One row of the list at y; state is the cartridge's save state. */
static void draw_row(int r, int y, int state) {
    bool s = r == sel && step == STEP_LIST, cur = r == sel;
    if (cur) gfx_rect(10, y - 1, 172, ROW_H, s ? C_DUSK : C_NIGHT);
    if (s) ui_cursor(12, y, t);
    ui_audit_box("cursor", 12, y, 5, 7); /* it bobs a pixel to the right */
    int g = rows[r];
    if (g < 0) {
        text_draw("DELETE ALL DATA", 22, y, cur ? C_RED : C_WINE);
        ui_audit_text("name", "DELETE ALL DATA", 22, y);
        return;
    }
    char buf[48];
    snprintf(buf, sizeof buf, "%02d %s", g + 1, GAMES[g]->title);
    text_draw(buf, 22, y, cur ? C_WHITE : C_GREY);
    ui_audit_text("name", buf, 22, y);
    const char *st = state == SAVE_OK ? "SAVE" : state == SAVE_DAMAGED ? "DAMAGED" : "-";
    int sc = state == SAVE_OK ? (cur ? C_LIME : C_JADE) : state == SAVE_DAMAGED ? (cur ? C_RED : C_WINE) : C_DUSK;
    int sx = state == SAVE_OK ? 146 : state == SAVE_DAMAGED ? 134 : 152;
    tiny_draw(st, sx, y + 1, sc);
    ui_audit_tiny("save state", st, sx, y + 1);
    goal_pips(164, y + 2, g);
}

/* The scroll marks sit at the panel's right edge, clear of the goal pips. */
#define MARK_X 178

static void draw_list(void) {
    ui_panel(6, LIST_Y - 5, 180, VISIBLE * ROW_H + 9, C_NIGHT, C_SLATE);
    ui_audit_area("list", 7, LIST_Y - 4, 178, VISIBLE * ROW_H + 7); /* inside the border */
    for (int v = 0; v < VISIBLE && top + v < n_rows; v++) {
        int r = top + v;
        draw_row(r, LIST_Y + v * ROW_H, rows[r] >= 0 ? states[rows[r]] : SAVE_NONE);
    }
    if (top > 0) text_draw(GLYPH_UP, MARK_X, LIST_Y - 3, C_GREY);
    if (top + VISIBLE < n_rows) text_draw(GLYPH_DOWN, MARK_X, LIST_Y + VISIBLE * ROW_H - 2, C_GREY);
    /* the marks' drawn rows only (the glyphs are blank at the top and bottom) */
    ui_audit_box("scroll up mark", MARK_X, LIST_Y - 2, 7, 4);
    ui_audit_box("scroll down mark", MARK_X, LIST_Y + VISIBLE * ROW_H - 1, 7, 4);
}

/* Wrapped main-font text at y, with its lines reported to the audit;
 * returns the y under the last line. */
static int wrap_lines(const char *what, const char *s, int x, int y, int w, int col, int max) {
    char l[6][UI_WRAP_LEN];
    int n = ui_wrap(s, w, false, l, imin(max, 6));
    for (int i = 0; i < imin(n, max); i++) {
        text_draw(l[i], x, y + i * LINE_H, col);
        ui_audit_text(what, l[i], x, y + i * LINE_H);
    }
    if (n > max) ui_audit_fail(what, "needs more lines than it has room for");
    return y + imin(n, max) * LINE_H;
}

static void draw_detail_for(int g, int state) {
    int x = 192, y = LIST_Y - 5, w = 122, h = VISIBLE * ROW_H + 9;
    ui_panel(x, y, w, h, C_NIGHT, C_SLATE);
    ui_audit_area("detail panel", x + 2, y + 2, w - 4, h - 4);
    int room = w - 16;
    char buf[64];
    if (g < 0) {
        text_draw("DELETE ALL", x + 8, y + 7, C_RED);
        ui_audit_text("heading", "DELETE ALL", x + 8, y + 7);
        wrap_lines("what goes", "ERASES EVERY SAVE, EVERY GOAL AND THE PLAY COUNTS.", x + 8, y + 22, room, C_LIGHT, 4);
        wrap_lines("what stays", "VOLUMES AND VIDEO OPTIONS STAY.", x + 8, y + 62, room, C_GREY, 2);
        tiny_draw("ASKS TWICE, NO IS DEFAULT", x + 8, y + 90, C_SLATE);
        ui_audit_tiny("hint", "ASKS TWICE, NO IS DEFAULT", x + 8, y + 90);
    } else {
        const GameDef *gd = GAMES[g];
        static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
        ui_fancy_text(gd->title, x + 8, y + 6, 1, grad, 4, C_INK, -1);
        ui_audit_fancy("title", gd->title, x + 8, y + 6, 1, false);
        if (gd->tribute) {
            snprintf(buf, sizeof buf, "TRIBUTE TO %s", gd->tribute);
            if (tiny_width(buf) > room) snprintf(buf, sizeof buf, "TRIBUTE: %s", gd->tribute);
            tiny_draw(buf, x + 8, y + 18, C_SKY);
            ui_audit_tiny("tribute", buf, x + 8, y + 18);
        }
        tiny_draw("SAVE FILE", x + 8, y + 28, C_GREY);
        ui_audit_tiny("save label", "SAVE FILE", x + 8, y + 28);
        static const char *const st[3] = {"NONE", "YES", "DAMAGED"};
        static const uint8_t stc[3] = {C_SLATE, C_LIME, C_RED};
        text_draw(st[state], x + 60, y + 27, stc[state]);
        ui_audit_text("save state", st[state], x + 60, y + 27);
        static const char *const names[3] = {"BEACON", "SAUCER", "ALIEN"};
        for (int b = 0; b < 3; b++) {
            int gy = y + 40 + b * 12;
            bool on = (g_progress.goals[g] >> b) & 1;
            ui_goal_icon(x + 8, gy, 1 << b, on, t);
            ui_audit_box("goal icon", x + 8, gy, 9, 9);
            text_draw(names[b], x + 21, gy + 1, on ? C_YELLOW : C_SLATE);
            ui_audit_text("goal name", names[b], x + 21, gy + 1);
            text_draw(on ? GLYPH_CHECK : "-", x + w - 16, gy + 1, on ? C_LIME : C_DUSK);
            ui_audit_text("goal mark", GLYPH_CHECK, x + w - 16, gy + 1);
        }
        snprintf(buf, sizeof buf, "PLAYED %d TIME%s", g_progress.played[g], g_progress.played[g] == 1 ? "" : "S");
        tiny_draw(buf, x + 8, y + 80, C_GREY);
        ui_audit_tiny("played", buf, x + 8, y + 80);
        tiny_draw("A: DELETE OR RESET", x + 8, y + 90, C_SLATE);
        ui_audit_tiny("hint", "A: DELETE OR RESET", x + 8, y + 90);
    }
}

static void draw_detail(void) {
    int g = cart();
    draw_detail_for(g, g >= 0 ? states[g] : SAVE_NONE);
    if (msg_t > 0) {
        int x = 192, y = LIST_Y - 5, w = 122;
        ui_panel(x + 6, y + 100, w - 12, 20, C_INK, C_YELLOW);
        text_center(msg, x + w / 2, y + 106, C_YELLOW);
    }
}

static void draw_where(void) {
    const char *where = plat_save_where();
    char buf[96];
    int room = SCREEN_W - 12 - text_width("SAVES: ");
    const char *p = where;
    while (*p && text_width(p) > room - text_width("...")) p++;
    if (p != where) snprintf(buf, sizeof buf, "...%s", p);
    else snprintf(buf, sizeof buf, "%s", where);
    int x = text_draw("SAVES: ", 6, 156, C_SLATE);
    text_draw(buf, x, 156, C_GREY);
}

static void draw_actions(void) {
    int g = cart();
    ui_panel(92, 56, 136, 66, C_INK, C_YELLOW);
    ui_audit_area("actions box", 94, 58, 132, 62);
    char buf[48];
    snprintf(buf, sizeof buf, "%02d %s", g + 1, GAMES[g]->title);
    text_center(buf, 160, 62, C_WHITE);
    ui_audit_text("title", buf, 160 - text_width(buf) / 2, 62);
    ui_audit_centred("title", 160 - text_width(buf) / 2, text_width(buf));
    static const char *const names[ACT_COUNT] = {"DELETE SAVE", "RESET GOALS", "CANCEL"};
    for (int i = 0; i < ACT_COUNT; i++) {
        int y = 78 + i * 13;
        bool s = i == act_sel, en = act_enabled(i);
        if (s) gfx_rect(98, y - 3, 124, 13, C_DUSK);
        if (s) ui_cursor(102, y, t);
        text_draw(names[i], 114, y, !en ? C_DUSK : s ? C_WHITE : C_GREY);
        ui_audit_text("action", names[i], 114, y);
    }
}

/* The question and its detail for deleting cartridge g's save, here and on
 * the library's cartridge card. */
void shell_delete_save_question(int g, char *q, int qn, char *d, int dn) {
    snprintf(q, (size_t)qn, "DELETE %s'S SAVE?", GAMES[g]->title);
    snprintf(d, (size_t)dn, "GOALS STAY. THE SAVE CAN'T COME BACK.");
}

static void draw_confirm(void) {
    int g = cart();
    char q[64], d[64];
    const char *yes_label = "YES", *step = NULL;
    int border = C_YELLOW;
    switch (ask) {
    case ASK_DELETE_SAVE:
        shell_delete_save_question(g, q, sizeof q, d, sizeof d);
        break;
    case ASK_RESET_GOALS:
        snprintf(q, sizeof q, "RESET %s'S GOALS?", GAMES[g]->title);
        snprintf(d, sizeof d, "ITS SAVE GOES TOO, SO NOTHING GIVES THEM BACK.");
        break;
    case ASK_ALL_1:
        snprintf(q, sizeof q, "DELETE ALL DATA?");
        snprintf(d, sizeof d, "EVERY SAVE, GOAL AND PLAY COUNT, ALL %d SLOTS.", GAME_SLOTS);
        border = C_RED;
        step = "STEP 1 OF 2";
        break;
    default:
        snprintf(q, sizeof q, "ARE YOU SURE?");
        snprintf(d, sizeof d, "THIS CANNOT BE UNDONE. LAST CHANCE!");
        yes_label = "DELETE ALL";
        border = C_RED;
        step = "STEP 2 OF 2";
        break;
    }
    ui_confirm_box(q, d, yes_label, yes, border, step, t);
}

/* Draws every question (delete a save and reset the goals for every
 * cartridge, and both steps of DELETE ALL DATA), with the cursor on NO and
 * on YES, with the layout audit on: each line and the NO / YES pair must sit
 * in the middle of the box. Returns the problems found. */
static int audit_confirms(void) {
    refresh();
    int save_sel = sel, save_step = step, save_ask = ask, save_yes = yes, bad = 0;
    char subject[80];
    for (int r = 0; r < n_rows; r++) {
        int g = rows[r];
        sel = r;
        static const int asks_cart[] = {ASK_DELETE_SAVE, ASK_RESET_GOALS}, asks_all[] = {ASK_ALL_1, ASK_ALL_2};
        for (int k = 0; k < 2; k++)
            for (yes = 0; yes <= 1; yes++) {
                ask = g >= 0 ? asks_cart[k] : asks_all[k];
                snprintf(subject, sizeof subject, "save data question %d for %s, on %s", ask,
                         g >= 0 ? GAMES[g]->title : "DELETE ALL DATA", yes ? "YES" : "NO");
                ui_audit_begin(subject, true);
                step = STEP_CONFIRM;
                draw_confirm();
                bad += ui_audit_end();
            }
    }
    sel = save_sel;
    step = save_step;
    ask = save_ask;
    yes = save_yes;
    return bad;
}

/* Draws every row, detail, action box and question for every cartridge
 * with the layout audit on; returns the problems found. */
static int audit_all(void) {
    refresh();
    int save_sel = sel, save_step = step, save_ask = ask, bad = 0;
    char subject[64];
    for (int r = 0; r < n_rows; r++) {
        int g = rows[r];
        const char *name = g >= 0 ? GAMES[g]->title : "DELETE ALL DATA";
        sel = r;
        for (int st = SAVE_NONE; st <= SAVE_DAMAGED; st++) {
            snprintf(subject, sizeof subject, "save data row %s, state %d", name, st);
            ui_audit_begin(subject, true);
            ui_audit_area("list row", 10, LIST_Y - 1, 172, ROW_H);
            draw_row(r, LIST_Y, st);
            bad += ui_audit_end();
            snprintf(subject, sizeof subject, "save data detail %s, state %d", name, st);
            ui_audit_begin(subject, true);
            draw_detail_for(g, st);
            bad += ui_audit_end();
            if (g < 0) break;
        }
        if (g >= 0) {
            snprintf(subject, sizeof subject, "save data actions %s", name);
            ui_audit_begin(subject, true);
            step = STEP_ACTIONS;
            draw_actions();
            bad += ui_audit_end();
        }
        static const int asks_cart[] = {ASK_DELETE_SAVE, ASK_RESET_GOALS}, asks_all[] = {ASK_ALL_1, ASK_ALL_2};
        for (int k = 0; k < 2; k++) {
            ask = g >= 0 ? asks_cart[k] : asks_all[k];
            snprintf(subject, sizeof subject, "save data question %d for %s", ask, name);
            ui_audit_begin(subject, true);
            step = STEP_CONFIRM;
            draw_confirm();
            bad += ui_audit_end();
        }
    }
    /* the whole list, scrolled to the top and to the bottom, with its marks */
    int save_top = top;
    for (int k = 0; k < 2; k++) {
        top = k ? imax(0, n_rows - VISIBLE) : 0;
        sel = top;
        step = STEP_LIST;
        ui_audit_begin(k ? "save data list, at the bottom" : "save data list, at the top", true);
        draw_list();
        bad += ui_audit_end();
    }
    top = save_top;
    sel = save_sel;
    step = save_step;
    ask = save_ask;
    return bad;
}

static void sd_draw(void) {
    ui_starfield(t, C_INK);
    static const uint8_t grad[] = {C_WHITE, C_ICE, C_CYAN, C_SKY};
    ui_fancy_center("SAVE DATA", 160, 5, 2, grad, 4, C_INK, C_NAVY);
    draw_list();
    draw_detail();
    draw_where();
    if (step == STEP_ACTIONS || step == STEP_CONFIRM) gfx_darken_rect(0, 0, SCREEN_W, 166, 2);
    if (step == STEP_ACTIONS) draw_actions();
    if (step == STEP_CONFIRM) draw_confirm();
    gfx_rect(0, 167, SCREEN_W, 13, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 166, C_DUSK);
    int fx = ui_hint(6, 170, GLYPH_A, step == STEP_CONFIRM ? "CONFIRM" : "SELECT", C_LIGHT);
    ui_hint(fx, 170, GLYPH_B, "BACK", C_LIGHT);
    tiny_draw("EVERYTHING SAVES BY ITSELF", SCREEN_W - 6 - tiny_width("EVERYTHING SAVES BY ITSELF"), 172, C_SLATE);
}

const Scene SCENE_SAVEDATA = {"savedata", sd_enter, sd_update, sd_draw, NULL};

bool savedata_query(const char *key, int *out) {
    if (!strcmp(key, "savedata_step")) { *out = step; return true; }
    if (!strcmp(key, "savedata_ask")) { *out = ask; return true; }
    if (!strcmp(key, "savedata_cart")) { *out = n_rows > 0 ? rows[sel] : -2; return true; }
    /* layout problems in every row, detail and question, for every cartridge */
    if (!strcmp(key, "savedata_layout")) { *out = audit_all(); return true; }
    /* the same, for the questions only (each line's margins in its box) */
    if (!strcmp(key, "savedata_confirm_layout")) { *out = audit_confirms(); return true; }
    return false;
}
