/* UFO 40 - the console's main menu (after the boot animation), plus the
 * CONTROLS and CREDITS screens it opens. */
#include "shell.h"

enum { MI_PLAY, MI_OPTIONS, MI_SAVEDATA, MI_CONTROLS, MI_CREDITS, MI_QUIT, MI_COUNT };

static const char *const MI_NAME[MI_COUNT] = {"PLAY", "OPTIONS", "SAVE DATA", "CONTROLS", "CREDITS", "QUIT"};
static const char *const MI_HELP[MI_COUNT] = {
    "THE GAME LIBRARY",
    "VOLUME, THE JUKEBOX AND VIDEO",
    "SAVES AND GOALS: DELETE OR RESET THEM",
    "THE BUTTONS ON EVERY SYSTEM",
    "WHO MADE THIS, AND WHAT IT IS",
    "CLOSE UFO 40",
};

static int t, items[MI_COUNT], n_items, sel, quit_t;

static void build_items(void) {
    n_items = 0;
    for (int i = 0; i < MI_COUNT; i++)
        if (i != MI_QUIT || plat_kind() == PLAT_PC) items[n_items++] = i;
    /* the remembered position, if that item exists here */
    sel = 0;
    for (int i = 0; i < n_items; i++)
        if (items[i] == g_progress.menu_pos) sel = i;
}

static void menu_enter(void) {
    t = 0;
    quit_t = 0;
    build_items();
    game_set_pausable(true);
    input_set_versus(false);
    shell_menu_music();
}

static void menu_leave(void) { progress_save(); }

static void choose(void) {
    int it = items[sel];
    g_progress.menu_pos = (uint8_t)it;
    switch (it) {
    case MI_PLAY:
        sfx_play_name("ui_ok");
        scene_goto(&SCENE_LIBRARY);
        break;
    case MI_OPTIONS:
        sfx_play_name("ui_ok");
        shell_open_options(&SCENE_MENU);
        break;
    case MI_SAVEDATA:
        sfx_play_name("ui_ok");
        scene_goto(&SCENE_SAVEDATA);
        break;
    case MI_CONTROLS:
        sfx_play_name("ui_ok");
        scene_goto(&SCENE_CONTROLS);
        break;
    case MI_CREDITS:
        sfx_play_name("ui_ok");
        scene_goto(&SCENE_CREDITS);
        break;
    case MI_QUIT:
        sfx_play_name("ui_back");
        progress_save();
        music_fade(20);
        quit_t = 1;
        break;
    }
}

static void menu_update(void) {
    t++;
    if (quit_t > 0) {
        if (++quit_t == 24) plat_request_quit();
        return;
    }
    if (scene_transitioning()) return;
    if (btn_repeat(BTN_UP)) { sel = (sel + n_items - 1) % n_items; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { sel = (sel + 1) % n_items; sfx_play_name("ui_move"); }
    g_progress.menu_pos = (uint8_t)items[sel];
    if (btnp(BTN_A) || btnp(BTN_START)) choose();
}

static int loaded_count(void) {
    int n = 0;
    for (int i = 0; i < GAME_SLOTS; i++) n += GAMES[i] != NULL;
    return n;
}

/* the footer bar shared by the menu screens */
static void footer(const char *a_label, const char *b_label) {
    gfx_rect(0, 167, SCREEN_W, 13, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 166, C_DUSK);
    int fx = 6;
    if (a_label) fx = ui_hint(fx, 170, GLYPH_A, a_label, C_LIGHT);
    if (b_label) ui_hint(fx, 170, GLYPH_B, b_label, C_LIGHT);
}

static void menu_draw(void) {
    ui_starfield(t, C_INK);
    ui_logo(160, 10, 3, t);
    tiny_center("FORTY GAMES FROM ANOTHER WORLD", 160, 54, C_SKY);
    int bob = (int)(sinf((float)t * 0.06f) * 3.0f);
    ui_saucer(262, 118 + bob, t, 1);
    ui_saucer(36, 96 - bob, t + 13, 1);

    int ph = n_items * 13 + 9;
    ui_panel(98, 64, 124, ph, C_NIGHT, C_SLATE);
    for (int i = 0; i < n_items; i++) {
        int y = 70 + i * 13;
        bool s = i == sel;
        if (s) gfx_rect(102, y - 3, 116, 13, C_DUSK);
        text_center(MI_NAME[items[i]], 160, y, s ? C_WHITE : C_GREY);
        if (s) {
            ui_cursor(160 - text_width(MI_NAME[items[i]]) / 2 - 11, y, t);
        }
    }
    tiny_center(MI_HELP[items[sel]], 160, 64 + ph + 5, C_GREY);

    footer("SELECT", NULL);
    char buf[48];
    snprintf(buf, sizeof buf, "%d/%d", progress_goal_count(), loaded_count() * 3);
    int w = text_width(buf);
    text_draw(buf, SCREEN_W - 6 - w, 170, C_YELLOW);
    ui_goal_icon(SCREEN_W - 18 - w, 169, GOAL_SAUCER, true, t);
    snprintf(buf, sizeof buf, "%d/40 LOADED " GLYPH_DOT " %s", loaded_count(), plat_name());
    tiny_draw(buf, 104, 172, C_SLATE);
}

const Scene SCENE_MENU = {"menu", menu_enter, menu_update, menu_draw, menu_leave};

/* ---- CONTROLS ------------------------------------------------------------ */

typedef struct CtlPage {
    const char *name;
    const char *keys[5];  /* D-pad, A, B, START, SELECT */
    const char *notes[2];
} CtlPage;

static const CtlPage PAGES[4] = {
    {"VITA",
     {"D-PAD OR LEFT STICK", "CROSS", "CIRCLE", "START", "SELECT"},
     {"TWO-PLAYER MODES NEED TWO PADS,", "SO THEY ARE LOCKED ON THE VITA."}},
    {"KEYBOARD",
     {"ARROWS OR WASD", "Z, J OR SPACE", "X OR K", "ENTER OR ESC", "SHIFT OR BACKSPACE"},
     {"F11 OR ALT+ENTER: FULLSCREEN (PC)", "2 PLAYERS: WASD + F/G AND ARROWS + K/L"}},
    {"GAMEPAD",
     {"D-PAD OR LEFT STICK", "A (BOTTOM BUTTON)", "B (RIGHT BUTTON)", "START", "BACK / SELECT"},
     {"PLUG AND PLAY ON PC AND IN THE BROWSER.", "A SECOND PAD IS PLAYER 2 IN 2P MODES."}},
    {"TOUCH",
     {"ON-SCREEN D-PAD", "A BUTTON", "B BUTTON", "START BUTTON", "SELECT BUTTON"},
     {"PHONES AND TABLETS PLAYING IN THE", "BROWSER GET A PAD UNDER THE SCREEN."}},
};
static const char *const CONSOLE_BTN[5] = {GLYPH_DPAD " D-PAD", GLYPH_A " A", GLYPH_B " B", "START", "SELECT"};

static int ctl_tab = -1, ctl_t;

static void ctl_enter(void) {
    ctl_t = 0;
    if (ctl_tab < 0) ctl_tab = plat_kind() == PLAT_VITA ? 0 : 1;
}

static void ctl_update(void) {
    ctl_t++;
    if (scene_transitioning()) return;
    if (btn_repeat(BTN_LEFT)) { ctl_tab = (ctl_tab + 3) % 4; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_RIGHT)) { ctl_tab = (ctl_tab + 1) % 4; sfx_play_name("ui_move"); }
    if (btnp(BTN_B) || btnp(BTN_A) || btnp(BTN_START)) {
        sfx_play_name("ui_back");
        scene_goto(&SCENE_MENU);
    }
}

static void screen_title(const char *s, int t_) {
    (void)t_;
    static const uint8_t grad[] = {C_WHITE, C_ICE, C_CYAN, C_SKY};
    ui_fancy_center(s, 160, 8, 2, grad, 4, C_INK, C_NAVY);
}

static void ctl_draw(void) {
    ui_starfield(ctl_t, C_INK);
    screen_title("CONTROLS", ctl_t);
    /* platform tabs */
    int x = 38;
    for (int i = 0; i < 4; i++) {
        int w = text_width(PAGES[i].name) + 12;
        bool s = i == ctl_tab;
        ui_panel(x, 30, w, 13, s ? C_DUSK : C_NIGHT, s ? C_YELLOW : C_SLATE);
        text_draw(PAGES[i].name, x + 6, 33, s ? C_WHITE : C_GREY);
        x += w + 4;
    }
    text_draw(GLYPH_LEFT, 26, 33, C_YELLOW);
    text_draw(GLYPH_RIGHT, 290, 33, C_YELLOW);
    const CtlPage *p = &PAGES[ctl_tab];
    ui_panel(24, 48, 272, 82, C_NIGHT, C_SLATE);
    for (int i = 0; i < 5; i++) {
        int y = 54 + i * 14;
        text_draw(CONSOLE_BTN[i], 36, y, C_YELLOW);
        text_draw(p->keys[i], 116, y, C_LIGHT);
        if (i < 4) gfx_hline(34, 285, y + 10, C_DUSK);
    }
    tiny_center(p->notes[0], 160, 136, C_GREY);
    tiny_center(p->notes[1], 160, 143, C_GREY);
    tiny_center("START PAUSES EVERY GAME " GLYPH_DOT " SELECT OPENS OPTIONS IN THE LIBRARY", 160, 155, C_SLATE);
    footer(NULL, "BACK");
    text_draw(GLYPH_LEFT GLYPH_RIGHT " SYSTEM", SCREEN_W - 6 - text_width(GLYPH_LEFT GLYPH_RIGHT " SYSTEM"), 170, C_GREY);
}

const Scene SCENE_CONTROLS = {"controls", ctl_enter, ctl_update, ctl_draw, NULL};

/* ---- CREDITS -------------------------------------------------------------- */

static int cr_t;

static void cr_enter(void) { cr_t = 0; }

static void cr_update(void) {
    cr_t++;
    if (scene_transitioning()) return;
    if (btnp(BTN_B) || btnp(BTN_A) || btnp(BTN_START)) {
        sfx_play_name("ui_back");
        scene_goto(&SCENE_MENU);
    }
}

static void cr_draw(void) {
    ui_starfield(cr_t, C_INK);
    screen_title("CREDITS", cr_t);
    ui_panel(14, 30, 292, 130, C_NIGHT, C_SLATE);
    static const char *const LINES[] = {
        "UFO 40 IS A FAN-MADE PARODY TRIBUTE TO",
        "UFO 50 BY MOSSMOUTH.",
        "",
        "IT CONTAINS NO CODE, ART, MUSIC OR LEVELS",
        "FROM UFO 50 AND IS NOT AFFILIATED WITH OR",
        "ENDORSED BY MOSSMOUTH. GO PLAY THE REAL THING!",
    };
    int y = 38;
    for (int i = 0; i < ARRAY_LEN(LINES); i++, y += 10) text_center(LINES[i], 160, y, i < 2 ? C_WHITE : C_LIGHT);
    gfx_hline(40, 279, y + 2, C_DUSK);
    y += 8;
    tiny_center("DESIGN, CODE, PIXEL ART, MUSIC AND LEVELS MADE FOR", 160, y, C_GREY);
    text_center("UFO 40 BY HAMZA BEN ISMAIL (@NANIIIC137)", 160, y + 7, C_YELLOW);
    tiny_center("SDL2, VITASDK AND EMSCRIPTEN RUN IT ON EVERY SYSTEM", 160, y + 21, C_GREY);
    tiny_center("CODE, ART AND MUSIC: MIT LICENSE", 160, y + 29, C_GREY);
    int bob = (int)(sinf((float)cr_t * 0.07f) * 2.0f);
    ui_saucer(150, 146 + bob, cr_t, 1);
    footer(NULL, "BACK");
}

const Scene SCENE_CREDITS = {"credits", cr_enter, cr_update, cr_draw, NULL};

bool menu_query(const char *key, int *out) {
    if (!strcmp(key, "menu_sel")) { *out = g_progress.menu_pos; return true; }
    if (!strcmp(key, "menu_items")) { *out = n_items; return true; }
    if (!strcmp(key, "controls_tab")) { *out = ctl_tab; return true; }
    return false;
}
