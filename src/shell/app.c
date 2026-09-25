/* UFO 40 - application wiring and the game runner (pause menu, toasts). */
#include "shell.h"

static int cur_game = -1;
static bool paused, pausable = true, exit_requested;
static int pause_sel, pause_page, confirm_sel, pause_t;
static int toast_timer, toast_game_i, toast_bit_i;

int g_library_cursor;
int g_jukebox_song = -1;

/* which cartridge defined each song, recorded while the cartridges load */
#define OWNER_MAX 256
static int8_t song_owner[OWNER_MAX];
static bool owners_ready;

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
    if (!owners_ready) memset(song_owner, -1, sizeof song_owner); /* the console's own songs */
    for (int i = 0; i < GAME_SLOTS; i++)
        if (GAMES[i] && GAMES[i]->load) {
            int s0 = song_count();
            GAMES[i]->load();
            for (int s = s0; s < song_count() && s < OWNER_MAX; s++) song_owner[s] = (int8_t)i;
        }
    owners_ready = true;
    g_jukebox_song = -1;
    g_library_cursor = g_progress.last_game < GAME_SLOTS ? g_progress.last_game : 0;
    scene_set(&SCENE_BOOT);
}

int shell_song_owner(int song) { return song >= 0 && song < OWNER_MAX ? song_owner[song] : -1; }

void shell_menu_music(void) {
    /* a song picked in the jukebox keeps playing through the menus */
    if (g_jukebox_song >= 0 && music_playing() == g_jukebox_song && !music_finished()) return;
    g_jukebox_song = -1;
    music_play(MUS_LIBRARY);
}

/* ---- save data --------------------------------------------------------- */

static uint8_t save_scratch[65536];

int shell_save_size(int game) {
    if (game < 0 || game >= GAME_SLOTS) return 0;
    int n = game_save_read(game, save_scratch, (int)sizeof save_scratch);
    return n > 0 ? n : 0;
}

void shell_delete_save(int game) {
    if (game >= 0 && game < GAME_SLOTS) game_save_erase(game);
}

/* Games rebuild goals from their saved progress (battles won, rooms done,
 * streaks...), so the goals only stay reset if that progress goes too. */
void shell_reset_goals(int game) {
    if (game < 0 || game >= GAME_SLOTS) return;
    game_save_erase(game);
    g_progress.goals[game] = 0;
    progress_save();
}

int shell_save_state(int game) {
    if (shell_save_size(game) > 0) return SAVE_OK;
    return game >= 0 && game < GAME_SLOTS && game_save_raw_size(game) > 0 ? SAVE_DAMAGED : SAVE_NONE;
}

void shell_delete_all(void) {
    for (int i = 0; i < GAME_SLOTS; i++) game_save_erase(i);
    memset(g_progress.goals, 0, sizeof g_progress.goals);
    memset(g_progress.played, 0, sizeof g_progress.played);
    g_progress.last_game = 0;
    g_library_cursor = 0;
    progress_save(); /* volumes, video and the menu position stay */
}

/* ---- test hooks ---------------------------------------------------------- */

bool menu_query(const char *key, int *out);
bool options_query(const char *key, int *out);
bool jukebox_query(const char *key, int *out);
bool savedata_query(const char *key, int *out);

bool shell_query(const char *key, int *out) {
    if (!strcmp(key, "music_vol")) { *out = g_progress.music_vol; return true; }
    if (!strcmp(key, "sfx_vol")) { *out = g_progress.sfx_vol; return true; }
    if (!strcmp(key, "music_playing")) { *out = music_playing(); return true; }
    if (!strcmp(key, "jukebox_song")) { *out = g_jukebox_song; return true; }
    if (!strcmp(key, "library_cursor")) { *out = g_library_cursor; return true; }
    if (!strncmp(key, "music_is.", 9)) {
        int id = song_find(key + 9);
        *out = id >= 0 && music_playing() == id && !music_finished();
        return true;
    }
    if (!strncmp(key, "save.", 5)) {
        int g = app_find_game(key + 5);
        *out = g >= 0 ? shell_save_size(g) : -1;
        return true;
    }
    if (!strncmp(key, "save_state.", 11)) {
        int g = app_find_game(key + 11);
        *out = g >= 0 ? shell_save_state(g) : -1;
        return true;
    }
    if (!strncmp(key, "played.", 7)) {
        int g = app_find_game(key + 7);
        *out = g >= 0 ? g_progress.played[g] : -1;
        return true;
    }
    return menu_query(key, out) || options_query(key, out) || jukebox_query(key, out) || savedata_query(key, out);
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
        return "D-PAD, STICK\tMOVE\n"
               "CROSS\t" GLYPH_A " BUTTON\n"
               "CIRCLE\t" GLYPH_B " BUTTON\n"
               "START\tSTART / PAUSE\n"
               "SELECT\tSELECT";
    case PLAT_WEB:
        return "ARROWS, WASD\tMOVE\n"
               "Z OR J\t" GLYPH_A " BUTTON\n"
               "X OR K\t" GLYPH_B " BUTTON\n"
               "ENTER\tSTART / PAUSE\n"
               "SHIFT, BKSP\tSELECT\n"
               "TOUCH\tON-SCREEN PAD";
    default:
        return "ARROWS, WASD\tMOVE\n"
               "Z OR J\t" GLYPH_A " BUTTON\n"
               "X OR K\t" GLYPH_B " BUTTON\n"
               "ENTER, ESC\tSTART / PAUSE\n"
               "SHIFT, BKSP\tSELECT\n"
               "F11\tFULLSCREEN\n"
               "GAMEPAD\tPLUG AND PLAY";
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
    input_set_versus(false); /* the library always gets the whole keyboard */
    if (G() && G()->quit) G()->quit();
    paused = false;
}

/* The volumes sit in the pause menu too, so a run never has to be left to
 * turn the music down. */
enum { PI_RESUME, PI_RESTART, PI_CONTROLS, PI_MUSIC, PI_SFX, PI_QUIT, PI_COUNT };
static const char *PAUSE_ITEMS[PI_COUNT] = {"RESUME", "RESTART", "CONTROLS", "MUSIC", "SOUND FX", "QUIT TO LIBRARY"};
static bool vol_dirty;

static void unpause(void) {
    paused = false;
    music_duck(false);
    input_consume();
    if (vol_dirty) { progress_save(); vol_dirty = false; }
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
    if (btn_repeat(BTN_UP)) { pause_sel = (pause_sel + PI_COUNT - 1) % PI_COUNT; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { pause_sel = (pause_sel + 1) % PI_COUNT; sfx_play_name("ui_move"); }
    /* the music plays at its real level while its volume is being set */
    music_duck(pause_sel != PI_MUSIC);
    int dir = btn_repeat(BTN_RIGHT) ? 1 : btn_repeat(BTN_LEFT) ? -1 : 0;
    if (dir && (pause_sel == PI_MUSIC || pause_sel == PI_SFX)) {
        uint8_t *v = pause_sel == PI_MUSIC ? &g_progress.music_vol : &g_progress.sfx_vol;
        *v = (uint8_t)iclamp(*v + dir, 0, 10);
        app_apply_settings();
        vol_dirty = true;
        sfx_play_name(pause_sel == PI_MUSIC ? "ui_move" : "ui_toast");
    }
    if (btnp(BTN_START) || btnp(BTN_B)) { sfx_play_name("ui_back"); unpause(); return; }
    if (btnp(BTN_A)) {
        switch (pause_sel) {
        case PI_RESUME: sfx_play_name("ui_ok"); unpause(); break;
        case PI_RESTART: pause_page = 2; confirm_sel = 1; sfx_play_name("ui_ok"); break;
        case PI_CONTROLS: pause_page = 1; sfx_play_name("ui_ok"); break;
        case PI_QUIT:
            sfx_play_name("ui_back");
            if (vol_dirty) { progress_save(); vol_dirty = false; }
            paused = false;
            music_duck(false);
            music_fade(20);
            scene_goto(&SCENE_LIBRARY);
            break;
        default: break;
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
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 3);
    const GameDef *g = G();
    if (pause_page == 1) {
        ui_panel(30, 22, 260, 136, C_NIGHT, C_SLATE);
        text_center("CONTROLS", 160, 30, C_YELLOW);
        text_draw(g->controls, 42, 46, C_LIGHT);
        text_center(GLYPH_A " BACK", 160, 144, C_GREY);
        return;
    }
    int py = 30;
    ui_panel(84, py, 152, 120, C_NIGHT, C_SLATE);
    gfx_rect(85, py + 1, 150, 15, C_DUSK);
    text_center("PAUSED", 160, py + 5, C_WHITE);
    tiny_center(g->title, 160, py + 20, C_GREY);
    if (pause_page == 2) {
        text_center("RESTART GAME?", 160, py + 40, C_YELLOW);
        tiny_center("UNSAVED PROGRESS IS LOST", 160, py + 52, C_GREY);
        text_draw("YES", 126, py + 68, confirm_sel == 0 ? C_WHITE : C_SLATE);
        text_draw("NO", 180, py + 68, confirm_sel == 1 ? C_WHITE : C_SLATE);
        ui_cursor(confirm_sel == 0 ? 118 : 172, py + 68, pause_t);
        return;
    }
    for (int i = 0; i < PI_COUNT; i++) {
        int y = py + 32 + i * 13;
        bool sel = i == pause_sel;
        if (sel) gfx_rect(92, y - 3, 136, 13, C_DUSK);
        text_draw(PAUSE_ITEMS[i], 108, y, sel ? C_WHITE : C_GREY);
        if (sel) ui_cursor(98, y, pause_t);
        if (i == PI_MUSIC || i == PI_SFX) {
            int v = i == PI_MUSIC ? g_progress.music_vol : g_progress.sfx_vol;
            for (int k = 0; k < 10; k++) gfx_rect(170 + k * 5, y, 4, 7, k < v ? (sel ? C_YELLOW : C_AMBER) : C_INK);
        }
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
