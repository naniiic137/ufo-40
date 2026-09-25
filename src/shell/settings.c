/* UFO 40 - OPTIONS: volumes (heard live), the jukebox and, on PC, video.
 * Opened from the main menu or with SELECT in the library; B goes back to
 * whichever opened it. */
#include "shell.h"

enum { OPT_MUSIC, OPT_SFX, OPT_JUKEBOX, OPT_SCALE, OPT_FULL, OPT_BACK };

static int t, sel, items[8], n_items, dirty_t;
static const Scene *back_to = &SCENE_MENU;

void shell_open_options(const Scene *back) {
    back_to = back ? back : &SCENE_MENU;
    scene_goto(&SCENE_SETTINGS);
}

static void build_items(void) {
    n_items = 0;
    items[n_items++] = OPT_MUSIC;
    items[n_items++] = OPT_SFX;
    items[n_items++] = OPT_JUKEBOX;
    if (plat_kind() == PLAT_PC) {
        items[n_items++] = OPT_SCALE;
        items[n_items++] = OPT_FULL;
    }
    items[n_items++] = OPT_BACK;
}

static void set_enter(void) {
    t = 0;
    dirty_t = 0;
    build_items();
    if (sel >= n_items) sel = 0; /* the cursor is remembered between visits */
    shell_menu_music();
}

static void set_leave(void) { progress_save(); }

static void leave_settings(void) {
    sfx_play_name("ui_back");
    scene_goto(back_to);
}

static void changed(void) {
    app_apply_settings();
    dirty_t = 45; /* saved once the value settles */
}

static void set_update(void) {
    t++;
    if (dirty_t > 0 && --dirty_t == 0) progress_save();
    if (scene_transitioning()) return;
    if (btn_repeat(BTN_UP)) { sel = (sel + n_items - 1) % n_items; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { sel = (sel + 1) % n_items; sfx_play_name("ui_move"); }
    int opt = items[sel];
    int dir = btn_repeat(BTN_RIGHT) ? 1 : btn_repeat(BTN_LEFT) ? -1 : 0;
    if (btnp(BTN_B) || btnp(BTN_SELECT)) { leave_settings(); return; }
    switch (opt) {
    case OPT_MUSIC:
        if (dir) {
            int v = iclamp(g_progress.music_vol + dir, 0, 10);
            if (v != g_progress.music_vol) {
                g_progress.music_vol = (uint8_t)v;
                changed(); /* the tune that is playing changes at once */
            }
            sfx_play_name("ui_move");
        }
        break;
    case OPT_SFX:
        if (dir) {
            int v = iclamp(g_progress.sfx_vol + dir, 0, 10);
            if (v != g_progress.sfx_vol) {
                g_progress.sfx_vol = (uint8_t)v;
                changed();
            }
            sfx_play_name("ui_toast"); /* a sample at the new level */
        }
        break;
    case OPT_JUKEBOX:
        if (btnp(BTN_A) || dir > 0) {
            sfx_play_name("ui_ok");
            scene_goto(&SCENE_JUKEBOX);
        }
        break;
    case OPT_SCALE:
        if (dir) {
            g_progress.scale = (uint8_t)iclamp(g_progress.scale + dir, 1, 6);
            changed();
            sfx_play_name("ui_move");
        }
        break;
    case OPT_FULL:
        if (dir || btnp(BTN_A)) {
            g_progress.fullscreen ^= 1;
            changed();
            sfx_play_name("ui_ok");
        }
        break;
    case OPT_BACK:
        if (btnp(BTN_A)) leave_settings();
        break;
    }
}

static void draw_bar(int x, int y, int v, bool sel_) {
    for (int i = 0; i < 10; i++) {
        int col = i < v ? (sel_ ? C_YELLOW : C_AMBER) : C_INK;
        gfx_rect(x + i * 6, y, 5, 7, col);
    }
}

static void set_draw(void) {
    ui_starfield(t, C_INK);
    ui_panel(40, 14, 240, 152, C_NIGHT, C_SLATE);
    static const uint8_t grad[] = {C_WHITE, C_ICE, C_CYAN, C_SKY};
    ui_fancy_center("OPTIONS", 160, 22, 2, grad, 4, C_INK, C_NAVY);
    char buf[64];
    for (int i = 0; i < n_items; i++) {
        int y = 48 + i * 15;
        bool s = i == sel;
        if (s) gfx_rect(48, y - 3, 224, 13, C_DUSK);
        if (s) ui_cursor(52, y, t);
        int col = s ? C_WHITE : C_GREY;
        switch (items[i]) {
        case OPT_MUSIC:
            text_draw("MUSIC", 62, y, col);
            draw_bar(170, y, g_progress.music_vol, s);
            snprintf(buf, sizeof buf, "%d", g_progress.music_vol);
            if (s) text_draw(buf, 234, y, C_LIGHT);
            break;
        case OPT_SFX:
            text_draw("SOUND FX", 62, y, col);
            draw_bar(170, y, g_progress.sfx_vol, s);
            snprintf(buf, sizeof buf, "%d", g_progress.sfx_vol);
            if (s) text_draw(buf, 234, y, C_LIGHT);
            break;
        case OPT_JUKEBOX: {
            text_draw("JUKEBOX", 62, y, col);
            int n = song_count();
            snprintf(buf, sizeof buf, GLYPH_NOTE " %d TUNES", n);
            text_draw(buf, 170, y, s ? C_YELLOW : C_SLATE);
            break;
        }
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
        case OPT_BACK:
            text_draw(back_to == &SCENE_LIBRARY ? "BACK TO LIBRARY" : "BACK TO MENU", 62, y, col);
            break;
        }
    }
    const char *hint = "";
    switch (items[sel]) {
    case OPT_MUSIC: hint = "LEFT/RIGHT: THE MUSIC PLAYING NOW CHANGES WITH IT"; break;
    case OPT_SFX: hint = "LEFT/RIGHT: EACH STEP PLAYS A SAMPLE SOUND"; break;
    case OPT_JUKEBOX: hint = "EVERY TUNE IN THE CONSOLE AND ITS GAMES"; break;
    case OPT_SCALE: hint = "LEFT/RIGHT: THE WINDOW SIZE"; break;
    case OPT_FULL: hint = "A TOGGLES IT (SO DOES F11)"; break;
    default: hint = "OPTIONS ARE SAVED AUTOMATICALLY"; break;
    }
    tiny_center(hint, 160, 156, C_SLATE);
}

const Scene SCENE_SETTINGS = {"settings", set_enter, set_update, set_draw, set_leave};

bool options_query(const char *key, int *out) {
    if (!strcmp(key, "options_sel")) { *out = items[sel]; return true; }
    return false;
}
