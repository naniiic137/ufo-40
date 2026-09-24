/* UFO 40 - settings screen. */
#include "shell.h"

enum { OPT_MUSIC, OPT_SFX, OPT_SCALE, OPT_FULL, OPT_CONTROLS, OPT_ERASE, OPT_BACK };

static int t, sel, page, erase_hold, erased_t;
static int items[8], n_items;

static void build_items(void) {
    n_items = 0;
    items[n_items++] = OPT_MUSIC;
    items[n_items++] = OPT_SFX;
    if (plat_kind() == PLAT_PC) {
        items[n_items++] = OPT_SCALE;
        items[n_items++] = OPT_FULL;
    }
    items[n_items++] = OPT_CONTROLS;
    items[n_items++] = OPT_ERASE;
    items[n_items++] = OPT_BACK;
}

static void set_enter(void) {
    t = 0;
    sel = 0;
    page = 0;
    erase_hold = 0;
    erased_t = 0;
    build_items();
    music_play(MUS_LIBRARY);
}

static void leave_settings(void) {
    progress_save();
    sfx_play_name("ui_back");
    scene_goto(&SCENE_LIBRARY);
}

static void erase_all(void) {
    uint8_t mv = g_progress.music_vol, sv = g_progress.sfx_vol, sc = g_progress.scale, fs = g_progress.fullscreen;
    progress_defaults();
    g_progress.music_vol = mv; g_progress.sfx_vol = sv; g_progress.scale = sc; g_progress.fullscreen = fs;
    for (int i = 0; i < GAME_SLOTS; i++) game_save_erase(i);
    progress_save();
}

static void set_update(void) {
    t++;
    if (erased_t > 0) erased_t--;
    if (page == 1) {
        if (btnp(BTN_A | BTN_B | BTN_START)) { page = 0; sfx_play_name("ui_back"); }
        return;
    }
    if (btn_repeat(BTN_UP)) { sel = (sel + n_items - 1) % n_items; sfx_play_name("ui_move"); erase_hold = 0; }
    if (btn_repeat(BTN_DOWN)) { sel = (sel + 1) % n_items; sfx_play_name("ui_move"); erase_hold = 0; }
    int opt = items[sel];
    int dir = btn_repeat(BTN_RIGHT) ? 1 : btn_repeat(BTN_LEFT) ? -1 : 0;
    if (btnp(BTN_B) || btnp(BTN_SELECT)) { leave_settings(); return; }
    switch (opt) {
    case OPT_MUSIC:
        if (dir) {
            g_progress.music_vol = (uint8_t)iclamp(g_progress.music_vol + dir, 0, 10);
            app_apply_settings();
            sfx_play_name("ui_move");
        }
        break;
    case OPT_SFX:
        if (dir) {
            g_progress.sfx_vol = (uint8_t)iclamp(g_progress.sfx_vol + dir, 0, 10);
            app_apply_settings();
            sfx_play_name("ui_ok");
        }
        break;
    case OPT_SCALE:
        if (dir) {
            g_progress.scale = (uint8_t)iclamp(g_progress.scale + dir, 1, 6);
            app_apply_settings();
            sfx_play_name("ui_move");
        }
        break;
    case OPT_FULL:
        if (dir || btnp(BTN_A)) {
            g_progress.fullscreen ^= 1;
            app_apply_settings();
            sfx_play_name("ui_ok");
        }
        break;
    case OPT_CONTROLS:
        if (btnp(BTN_A)) { page = 1; sfx_play_name("ui_ok"); }
        break;
    case OPT_ERASE:
        if (btn(BTN_A)) {
            erase_hold++;
            if (erase_hold % 10 == 1) sfx_play_name("ui_move");
            if (erase_hold >= 120) {
                erase_all();
                erase_hold = 0;
                erased_t = 120;
                sfx_play_name("ui_error");
            }
        } else {
            erase_hold = 0;
        }
        break;
    case OPT_BACK:
        if (btnp(BTN_A)) leave_settings();
        break;
    }
}

static void draw_bar(int x, int y, int v, bool sel) {
    for (int i = 0; i < 10; i++) {
        int col = i < v ? (sel ? C_YELLOW : C_AMBER) : C_INK;
        gfx_rect(x + i * 6, y, 5, 7, col);
    }
}

static void set_draw(void) {
    ui_starfield(t, C_INK);
    ui_panel(40, 14, 240, 152, C_NIGHT, C_SLATE);
    static const uint8_t grad[] = {C_WHITE, C_ICE, C_CYAN, C_SKY};
    ui_fancy_center("SETTINGS", 160, 22, 2, grad, 4, C_INK, C_NAVY);
    if (page == 1) {
        text_center("CONTROLS", 160, 46, C_YELLOW);
        text_draw(shell_controls_text(), 58, 62, C_LIGHT);
        tiny_center("EACH GAME LISTS ITS OWN MOVES IN THE PAUSE MENU", 160, 136, C_SLATE);
        text_center(GLYPH_A " BACK", 160, 150, C_GREY);
        return;
    }
    char buf[48];
    for (int i = 0; i < n_items; i++) {
        int y = 48 + i * 14;
        bool s = i == sel;
        if (s) gfx_rect(48, y - 3, 224, 13, C_DUSK);
        if (s) ui_cursor(52, y, t);
        int col = s ? C_WHITE : C_GREY;
        switch (items[i]) {
        case OPT_MUSIC:
            text_draw("MUSIC", 62, y, col);
            draw_bar(170, y, g_progress.music_vol, s);
            break;
        case OPT_SFX:
            text_draw("SOUND FX", 62, y, col);
            draw_bar(170, y, g_progress.sfx_vol, s);
            break;
        case OPT_SCALE:
            text_draw("WINDOW SCALE", 62, y, col);
            snprintf(buf, sizeof buf, GLYPH_LEFT " %dX  %dx%d " GLYPH_RIGHT, g_progress.scale,
                     320 * g_progress.scale, 180 * g_progress.scale);
            text_draw(buf, 170, y, s ? C_YELLOW : C_LIGHT);
            break;
        case OPT_FULL:
            text_draw("FULLSCREEN", 62, y, col);
            text_draw(g_progress.fullscreen ? "ON" : "OFF", 170, y, s ? C_YELLOW : C_LIGHT);
            break;
        case OPT_CONTROLS:
            text_draw("CONTROLS", 62, y, col);
            text_draw(GLYPH_A " VIEW", 170, y, s ? C_YELLOW : C_SLATE);
            break;
        case OPT_ERASE:
            text_draw("ERASE PROGRESS", 62, y, s ? C_RED : C_GREY);
            if (erased_t > 0) text_draw("ERASED", 170, y, C_RED);
            else if (s) {
                text_draw("HOLD " GLYPH_A, 170, y, C_LIGHT);
                gfx_rect(214, y + 2, 50, 3, C_INK);
                gfx_rect(214, y + 2, erase_hold * 50 / 120, 3, C_RED);
            }
            break;
        case OPT_BACK:
            text_draw("BACK TO LIBRARY", 62, y, col);
            break;
        }
    }
    tiny_center("SETTINGS ARE SAVED AUTOMATICALLY", 160, 156, C_SLATE);
}

const Scene SCENE_SETTINGS = {"settings", set_enter, set_update, set_draw, NULL};
