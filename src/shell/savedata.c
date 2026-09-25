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
static int sizes[GAME_SLOTS];
static int t, sel, top, step, act_sel, ask, yes, msg_t;
static const char *msg;

static void refresh(void) {
    n_rows = 0;
    for (int i = 0; i < GAME_SLOTS; i++)
        if (GAMES[i]) {
            rows[n_rows++] = i;
            sizes[i] = shell_save_size(i);
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
    if (a == ACT_DELETE_SAVE) return sizes[g] > 0;
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

static void goal_pips(int x, int y, int g) {
    for (int b = 0; b < 3; b++) {
        bool on = (g_progress.goals[g] >> b) & 1;
        gfx_rect(x + b * 5, y, 3, 3, on ? C_YELLOW : C_INK);
        if (!on) gfx_rectb(x + b * 5, y, 3, 3, C_DUSK);
    }
}

static void draw_list(void) {
    ui_panel(6, LIST_Y - 5, 180, VISIBLE * ROW_H + 9, C_NIGHT, C_SLATE);
    char buf[48];
    for (int v = 0; v < VISIBLE && top + v < n_rows; v++) {
        int r = top + v, y = LIST_Y + v * ROW_H;
        bool s = r == sel && step == STEP_LIST, cur = r == sel;
        if (cur) gfx_rect(10, y - 1, 172, ROW_H, s ? C_DUSK : C_NIGHT);
        if (s) ui_cursor(12, y, t);
        int g = rows[r];
        if (g < 0) {
            text_draw("DELETE ALL DATA", 22, y, cur ? C_RED : C_WINE);
            continue;
        }
        snprintf(buf, sizeof buf, "%02d %s", g + 1, GAMES[g]->title);
        text_draw(buf, 22, y, cur ? C_WHITE : C_GREY);
        if (sizes[g] > 0) tiny_draw("SAVE", 146, y + 1, cur ? C_LIME : C_JADE);
        else tiny_draw("-", 152, y + 1, C_DUSK);
        goal_pips(164, y + 2, g);
    }
    if (top > 0) text_draw(GLYPH_UP, 176, LIST_Y - 3, C_GREY);
    if (top + VISIBLE < n_rows) text_draw(GLYPH_DOWN, 176, LIST_Y + VISIBLE * ROW_H - 2, C_GREY);
}

static void draw_detail(void) {
    int x = 192, y = LIST_Y - 5, w = 122;
    ui_panel(x, y, w, VISIBLE * ROW_H + 9, C_NIGHT, C_SLATE);
    int g = cart();
    char buf[64];
    if (g < 0) {
        text_draw("DELETE ALL", x + 8, y + 7, C_RED);
        text_wrap("ERASES EVERY CARTRIDGE'S SAVE AND EVERY GOAL EARNED.", x + 8, y + 22, w - 14, C_LIGHT, 9);
        text_wrap("VOLUMES AND VIDEO OPTIONS STAY.", x + 8, y + 62, w - 14, C_GREY, 9);
        tiny_draw("ASKS TWICE, NO IS THE DEFAULT", x + 8, y + 90, C_SLATE);
    } else {
        const GameDef *gd = GAMES[g];
        static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
        ui_fancy_text(gd->title, x + 8, y + 6, 1, grad, 4, C_INK, -1);
        if (gd->tribute) {
            snprintf(buf, sizeof buf, "TRIBUTE TO %s", gd->tribute);
            tiny_draw(buf, x + 8, y + 17, C_SKY);
        }
        tiny_draw("SAVE FILE", x + 8, y + 28, C_GREY);
        text_draw(sizes[g] > 0 ? "YES" : "NONE", x + 60, y + 27, sizes[g] > 0 ? C_LIME : C_SLATE);
        static const char *const names[3] = {"BEACON", "SAUCER", "ALIEN"};
        for (int b = 0; b < 3; b++) {
            int gy = y + 40 + b * 12;
            bool on = (g_progress.goals[g] >> b) & 1;
            ui_goal_icon(x + 8, gy, 1 << b, on, t);
            text_draw(names[b], x + 21, gy + 1, on ? C_YELLOW : C_SLATE);
            text_draw(on ? GLYPH_CHECK : "-", x + w - 16, gy + 1, on ? C_LIME : C_DUSK);
        }
        snprintf(buf, sizeof buf, "PLAYED %d TIME%s", g_progress.played[g], g_progress.played[g] == 1 ? "" : "S");
        tiny_draw(buf, x + 8, y + 80, C_GREY);
        tiny_draw("A: DELETE OR RESET", x + 8, y + 90, C_SLATE);
    }
    if (msg_t > 0) {
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
    char buf[48];
    snprintf(buf, sizeof buf, "%02d %s", g + 1, GAMES[g]->title);
    text_center(buf, 160, 62, C_WHITE);
    static const char *const names[ACT_COUNT] = {"DELETE SAVE", "RESET GOALS", "CANCEL"};
    for (int i = 0; i < ACT_COUNT; i++) {
        int y = 78 + i * 13;
        bool s = i == act_sel, en = act_enabled(i);
        if (s) gfx_rect(98, y - 3, 124, 13, C_DUSK);
        if (s) ui_cursor(102, y, t);
        text_draw(names[i], 114, y, !en ? C_DUSK : s ? C_WHITE : C_GREY);
    }
}

static void draw_confirm(void) {
    int g = cart();
    char q[64], d[64];
    const char *yes_label = "YES";
    int border = C_YELLOW;
    switch (ask) {
    case ASK_DELETE_SAVE:
        snprintf(q, sizeof q, "DELETE %s'S SAVE?", GAMES[g]->title);
        snprintf(d, sizeof d, "GOALS STAY. THE SAVE CAN'T COME BACK.");
        break;
    case ASK_RESET_GOALS:
        snprintf(q, sizeof q, "RESET %s'S GOALS?", GAMES[g]->title);
        snprintf(d, sizeof d, "ITS BEACON, SAUCER AND ALIEN GO DARK.");
        break;
    case ASK_ALL_1:
        snprintf(q, sizeof q, "DELETE ALL DATA?");
        snprintf(d, sizeof d, "EVERY SAVE AND EVERY GOAL, ALL 40 SLOTS.");
        border = C_RED;
        break;
    default:
        snprintf(q, sizeof q, "ARE YOU SURE?");
        snprintf(d, sizeof d, "THIS CANNOT BE UNDONE. LAST CHANCE!");
        yes_label = "DELETE ALL";
        border = C_RED;
        break;
    }
    int w = imax(text_width(q), tiny_width(d)) + 24;
    w = imax(w, 150);
    ui_panel(160 - w / 2, 58, w, 62, C_INK, border);
    text_center(q, 160, 66, border == C_RED ? C_RED : C_YELLOW);
    tiny_center(d, 160, 80, C_LIGHT);
    int nx = 160 - 50, yx = 160 + 14;
    text_draw("NO", nx + 8, 100, !yes ? C_WHITE : C_SLATE);
    text_draw(yes_label, yx + 8, 100, yes ? C_WHITE : C_SLATE);
    ui_cursor(yes ? yx : nx, 100, t);
    if (ask == ASK_ALL_2) tiny_center("STEP 2 OF 2", 160, 112, C_GREY);
    else if (ask == ASK_ALL_1) tiny_center("STEP 1 OF 2", 160, 112, C_GREY);
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
}

const Scene SCENE_SAVEDATA = {"savedata", sd_enter, sd_update, sd_draw, NULL};

bool savedata_query(const char *key, int *out) {
    if (!strcmp(key, "savedata_step")) { *out = step; return true; }
    if (!strcmp(key, "savedata_ask")) { *out = ask; return true; }
    if (!strcmp(key, "savedata_cart")) { *out = n_rows > 0 ? rows[sel] : -2; return true; }
    return false;
}
