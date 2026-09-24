/* UFO 40 - application wiring and the game runner (pause menu, toasts). */
#include "shell.h"

static int cur_game = -1;
static bool paused, pausable = true, exit_requested;
static int pause_sel, pause_page, confirm_sel, pause_t;
static int toast_timer, toast_game_i, toast_bit_i;

int g_library_cursor;

/* ---- services for games ---------------------------------------------- */

int game_current_index(void) { return cur_game; }
void game_award(int bit) {
    if (cur_game >= 0) progress_award(cur_game, bit);
}
void game_exit_to_library(void) { exit_requested = true; }
bool game_paused(void) { return paused; }
void game_set_pausable(bool on) { pausable = on; }

/* ---- app ------------------------------------------------------------- */

void app_apply_settings(void) {
    audio_set_volume(g_progress.music_vol, g_progress.sfx_vol);
    plat_apply_video(g_progress.scale, g_progress.fullscreen);
}

void app_init(void) {
    engine_init();
    ui_init();
    progress_load();
    audio_set_volume(g_progress.music_vol, g_progress.sfx_vol);
    shell_audio_init();
    for (int i = 0; i < GAME_SLOTS; i++)
        if (GAMES[i] && GAMES[i]->load) GAMES[i]->load();
    g_library_cursor = g_progress.last_game < GAME_SLOTS ? g_progress.last_game : 0;
    scene_set(&SCENE_BOOT);
}

void app_update(void) { engine_update(); }
void app_draw(void) { engine_draw(); }

const char *app_scene_name(void) {
    const Scene *s = scene_current();
    return s ? s->name : "";
}

int app_find_game(const char *key) {
    int n = atoi(key);
    if (n >= 1 && n <= GAME_SLOTS && GAMES[n - 1]) return n - 1;
    for (int i = 0; i < GAME_SLOTS; i++)
        if (GAMES[i] && strcmp(GAMES[i]->id, key) == 0) return i;
    return -1;
}

void app_launch_game(int index, bool with_transition) {
    if (index < 0 || index >= GAME_SLOTS || !GAMES[index]) return;
    cur_game = index;
    if (g_progress.played[index] < 255) g_progress.played[index]++;
    g_progress.last_game = (uint8_t)index;
    progress_save();
    if (with_transition) scene_goto_speed(&SCENE_RUNNER, 3);
    else scene_set(&SCENE_RUNNER);
}

const char *shell_controls_text(void) {
    switch (plat_kind()) {
    case PLAT_VITA:
        return "D-PAD / LEFT STICK   MOVE\n"
               "CROSS                A\n"
               "CIRCLE               B\n"
               "START                START / PAUSE\n"
               "SELECT               SELECT";
    case PLAT_WEB:
        return "ARROWS / WASD        MOVE\n"
               "Z OR J               A\n"
               "X OR K               B\n"
               "ENTER                START / PAUSE\n"
               "SHIFT OR BACKSPACE   SELECT\n"
               "TOUCH: ON-SCREEN PAD";
    default:
        return "ARROWS / WASD        MOVE\n"
               "Z OR J               A\n"
               "X OR K               B\n"
               "ENTER / ESC          START / PAUSE\n"
               "SHIFT OR BACKSPACE   SELECT\n"
               "F11 / ALT+ENTER      FULLSCREEN\n"
               "GAMEPAD              SUPPORTED";
    }
}

/* ---- runner scene ---------------------------------------------------- */

static const GameDef *G(void) { return cur_game >= 0 ? GAMES[cur_game] : NULL; }

static void runner_enter(void) {
    paused = false;
    pausable = true;
    exit_requested = false;
    toast_timer = 0;
    music_duck(false);
    if (G()) G()->start();
}

static void runner_leave(void) {
    music_duck(false);
    if (G() && G()->quit) G()->quit();
    paused = false;
}

static const char *PAUSE_ITEMS[] = {"RESUME", "RESTART", "CONTROLS", "QUIT TO LIBRARY"};

static void unpause(void) {
    paused = false;
    music_duck(false);
    input_consume();
}

static void pause_update(void) {
    pause_t++;
    if (pause_page == 1) { /* controls */
        if (btnp(BTN_A | BTN_B | BTN_START)) { pause_page = 0; sfx_play_name("ui_back"); }
        return;
    }
    if (pause_page == 2) { /* confirm restart */
        if (btnp(BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN)) { confirm_sel ^= 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { pause_page = 0; sfx_play_name("ui_back"); }
        if (btnp(BTN_A)) {
            if (confirm_sel == 0) {
                sfx_play_name("ui_ok");
                unpause();
                if (G()->quit) G()->quit();
                G()->start();
            } else {
                pause_page = 0;
                sfx_play_name("ui_back");
            }
        }
        return;
    }
    if (btn_repeat(BTN_UP)) { pause_sel = (pause_sel + 3) % 4; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { pause_sel = (pause_sel + 1) % 4; sfx_play_name("ui_move"); }
    if (btnp(BTN_START) || btnp(BTN_B)) { sfx_play_name("ui_back"); unpause(); return; }
    if (btnp(BTN_A)) {
        switch (pause_sel) {
        case 0: sfx_play_name("ui_ok"); unpause(); break;
        case 1: pause_page = 2; confirm_sel = 1; sfx_play_name("ui_ok"); break;
        case 2: pause_page = 1; sfx_play_name("ui_ok"); break;
        case 3:
            sfx_play_name("ui_back");
            paused = false;
            music_duck(false);
            music_fade(20);
            scene_goto(&SCENE_LIBRARY);
            break;
        }
    }
}

static void runner_update(void) {
    if (exit_requested) {
        exit_requested = false;
        music_fade(30);
        scene_goto(&SCENE_LIBRARY);
        return;
    }
    if (scene_transitioning()) return;
    if (paused) {
        pause_update();
    } else if (pausable && btnp(BTN_START)) {
        paused = true;
        pause_sel = 0;
        pause_page = 0;
        pause_t = 0;
        music_duck(true);
        sfx_play_name("ui_pause");
    } else if (G()) {
        G()->update();
    }
    if (toast_timer > 0) toast_timer--;
    else if (progress_pop_toast(&toast_game_i, &toast_bit_i)) {
        toast_timer = 200;
        sfx_play_name("ui_toast");
    }
}

static void draw_pause(void) {
    gfx_camera(0, 0);
    gfx_noclip();
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, 6);
    const GameDef *g = G();
    if (pause_page == 1) {
        ui_panel(30, 22, 260, 136, C_NIGHT, C_SLATE);
        text_center("CONTROLS", 160, 30, C_YELLOW);
        text_draw(g->controls, 42, 46, C_LIGHT);
        text_center(GLYPH_A " BACK", 160, 144, C_GREY);
        return;
    }
    ui_panel(92, 44, 136, 92, C_NIGHT, C_SLATE);
    gfx_rect(93, 45, 134, 15, C_DUSK);
    text_center("PAUSED", 160, 49, C_WHITE);
    tiny_center(g->title, 160, 64, C_GREY);
    if (pause_page == 2) {
        text_center("RESTART GAME?", 160, 80, C_YELLOW);
        tiny_center("UNSAVED PROGRESS IS LOST", 160, 92, C_GREY);
        text_draw("YES", 126, 108, confirm_sel == 0 ? C_WHITE : C_SLATE);
        text_draw("NO", 180, 108, confirm_sel == 1 ? C_WHITE : C_SLATE);
        ui_cursor(confirm_sel == 0 ? 118 : 172, 108, pause_t);
        return;
    }
    for (int i = 0; i < 4; i++) {
        int y = 76 + i * 13;
        bool sel = i == pause_sel;
        if (sel) gfx_rect(100, y - 3, 120, 13, C_DUSK);
        text_draw(PAUSE_ITEMS[i], 116, y, sel ? C_WHITE : C_GREY);
        if (sel) ui_cursor(106, y, pause_t);
    }
}

static void draw_toast(void) {
    int t = 200 - toast_timer;
    int y = -24;
    if (t < 12) y = -24 + t * 2;
    else if (toast_timer < 12) y = -24 + toast_timer * 2;
    else y = 0;
    gfx_camera(0, 0);
    gfx_noclip();
    int x = 60, w = 200;
    ui_panel(x, y + 2, w, 22, C_NIGHT, C_YELLOW);
    ui_goal_icon(x + 6, y + 8, toast_bit_i, true, t);
    const char *names[3] = {"BEACON EARNED!", "SAUCER EARNED!", "ALIEN EARNED!"};
    int bi = toast_bit_i == GOAL_BEACON ? 0 : toast_bit_i == GOAL_SAUCER ? 1 : 2;
    text_draw(names[bi], x + 20, y + 5, C_YELLOW);
    const GameDef *g = GAMES[toast_game_i];
    if (g) tiny_draw(g->goal_desc[bi], x + 20, y + 15, C_LIGHT);
}

static void runner_draw(void) {
    if (G()) G()->draw();
    if (paused) draw_pause();
    if (toast_timer > 0) draw_toast();
}

const Scene SCENE_RUNNER = {"runner", runner_enter, runner_update, runner_draw, runner_leave};
