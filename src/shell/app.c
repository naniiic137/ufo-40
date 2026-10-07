/* UFO 40 - application wiring and the game runner (pause menu, toasts). */
#include "shell.h"

static int cur_game = -1;
static bool paused, pausable = true, exit_requested;
static int pause_sel, pause_page, confirm_sel, pause_t;
static int toast_timer, toast_game_i, toast_bit_i;
static int play_frames, unsaved_secs; /* the running cartridge's play time */

/* real time for the cartridges that keep going (realtime_tick) */
static uint32_t rt_last;    /* the platform clock at the last hand-over */
static bool rt_running;     /* false until the first update after app_init */
static uint32_t rt_handed;  /* ms handed over since start-up (for tests) */

int g_library_cursor;
int g_jukebox_song = -1;

/* which cartridge defined each song, recorded while the cartridges load */
#define OWNER_MAX 1024 /* one per song: keep in step with MAX_SONGS in engine/audio.c */
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

#define PAUSE_EXTRA_MAX 2
static const char *pause_extra[PAUSE_EXTRA_MAX];
static int pause_nextra;
static void (*pause_pick)(int i);
void game_pause_items(int n, const char *const *items, void (*pick)(int i)) {
    pause_nextra = pick ? iclamp(n, 0, PAUSE_EXTRA_MAX) : 0;
    for (int i = 0; i < pause_nextra; i++) pause_extra[i] = items[i];
    pause_pick = pick;
}

/* ---- app ------------------------------------------------------------- */

void app_apply_settings(void) {
    audio_set_volume(g_progress.music_vol, g_progress.sfx_vol);
    plat_apply_video(g_progress.scale, g_progress.fullscreen);
}

void app_init(void) {
    engine_init();
    ui_init();
    progress_load();
    shell_sync_cartridges();
    shell_remap_apply(-1);
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
    rt_running = false; /* switched on: the real-time clock starts afresh */
    rt_handed = 0;
    g_jukebox_song = -1;
    g_library_cursor = g_progress.last_game < GAME_SLOTS ? g_progress.last_game : 0;
    scene_set(&SCENE_BOOT);
}

/* ---- NEW tags ------------------------------------------------------------ */

/* The cartridges a save from before the NEW tags gets tagged with when it is
 * upgraded: the latest batch only (v0.7.0's 19, 26, 31 and 39, and 11, the
 * one after it), so an old save doesn't light up with a wall of tags. Later
 * cartridges need no entry here: anything in GAMES[] that a save doesn't
 * know yet is NEW by itself. Slot numbers, 1-based. */
static const uint8_t NEW_ON_UPGRADE[] = {11, 19, 26, 31, 39};

static bool new_on_upgrade(int slot) {
    for (size_t i = 0; i < sizeof NEW_ON_UPGRADE; i++)
        if (NEW_ON_UPGRADE[i] == slot + 1) return true;
    return false;
}

void shell_sync_cartridges(void) {
    bool changed = g_progress_origin == PROGRESS_UPGRADED; /* written back in the new layout */
    for (int i = 0; i < GAME_SLOTS; i++) {
        if (!GAMES[i] || progress_bit(g_progress.known, i)) continue;
        progress_set_bit(g_progress.known, i, true);
        bool tag = g_progress_origin == PROGRESS_CURRENT ||
                   (g_progress_origin == PROGRESS_UPGRADED && new_on_upgrade(i) && g_progress.played[i] == 0);
        if (!tag) progress_set_bit(g_progress.opened, i, true); /* already familiar */
        changed = true;
    }
    if (changed) progress_save();
}

bool shell_cart_is_new(int slot) {
    return slot >= 0 && slot < GAME_SLOTS && GAMES[slot] && progress_bit(g_progress.known, slot) &&
           !progress_bit(g_progress.opened, slot);
}

int shell_new_count(void) {
    int n = 0;
    for (int i = 0; i < GAME_SLOTS; i++) n += shell_cart_is_new(i);
    return n;
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
    memset(g_progress.play_secs, 0, sizeof g_progress.play_secs);
    memset(g_progress.last_played, 0, sizeof g_progress.last_played);
    g_progress.last_game = 0;
    g_library_cursor = 0;
    progress_save(); /* volumes, video and the menu position stay */
}

/* ---- test hooks ---------------------------------------------------------- */

bool menu_query(const char *key, int *out);
bool options_query(const char *key, int *out);
bool jukebox_query(const char *key, int *out);
bool savedata_query(const char *key, int *out);
bool library_query(const char *key, int *out);
bool cartinfo_query(const char *key, int *out);

static int audit_runner(void);

bool shell_query(const char *key, int *out) {
    /* layout problems in every cartridge's pause menu and goal toasts */
    if (!strcmp(key, "pause_layout")) { *out = audit_runner(); return true; }
    if (!strcmp(key, "music_vol")) { *out = g_progress.music_vol; return true; }
    if (!strcmp(key, "sfx_vol")) { *out = g_progress.sfx_vol; return true; }
    if (!strcmp(key, "music_playing")) { *out = music_playing(); return true; }
    if (!strcmp(key, "jukebox_song")) { *out = g_jukebox_song; return true; }
    if (!strcmp(key, "library_cursor")) { *out = g_library_cursor; return true; }
    if (!strcmp(key, "realtime_s")) { *out = (int)(rt_handed / 1000u); return true; }
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
    if (!strcmp(key, "new_count")) { *out = shell_new_count(); return true; }
    if (!strncmp(key, "new.", 4)) {
        int g = app_find_game(key + 4);
        *out = g >= 0 ? shell_cart_is_new(g) : -1;
        return true;
    }
    if (!strcmp(key, "pause_sel")) { *out = paused ? pause_sel : -1; return true; }
    if (!strcmp(key, "input_down")) { *out = (int)input_down(); return true; }
    if (!strncmp(key, "play_secs.", 10)) {
        int g = app_find_game(key + 10);
        *out = g >= 0 ? (int)g_progress.play_secs[g] : -1;
        return true;
    }
    if (!strncmp(key, "last_played.", 12)) {
        int g = app_find_game(key + 12);
        *out = g >= 0 ? (int)g_progress.last_played[g] : -1;
        return true;
    }
    if (!strncmp(key, "remap.", 6)) { /* the physical buttons of A, B, SELECT as digits: 12 3 = as made */
        int g = app_find_game(key + 6);
        *out = g >= 0 ? 100 * (shell_remap_button(g, JOB_A) + 1) + 10 * (shell_remap_button(g, JOB_B) + 1) +
                            shell_remap_button(g, JOB_SELECT) + 1
                      : -1;
        return true;
    }
    if (!strncmp(key, "played.", 7)) {
        int g = app_find_game(key + 7);
        *out = g >= 0 ? g_progress.played[g] : -1;
        return true;
    }
    return menu_query(key, out) || options_query(key, out) || jukebox_query(key, out) || savedata_query(key, out) ||
           library_query(key, out) || cartinfo_query(key, out);
}

/* ---- real time for the cartridges that keep going ----------------------- */

/* Hand every realtime cartridge the wall-clock time since the last update.
 * The first update after start-up only starts the clock: time while the
 * console was off is never counted. */
static void realtime_tick(void) {
    uint32_t now = plat_clock_ms();
    uint32_t ms = rt_running ? now - rt_last : 0;
    rt_last = now;
    rt_running = true;
    if (ms == 0) return;
    rt_handed += ms;
    for (int i = 0; i < GAME_SLOTS; i++)
        if (GAMES[i] && GAMES[i]->realtime) GAMES[i]->realtime(ms);
}

void app_update(void) {
    engine_update();
    realtime_tick();
}
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
    progress_set_bit(g_progress.opened, index, true); /* its NEW tag comes off for good */
    uint32_t now = plat_unix_time();
    if (now) g_progress.last_played[index] = now;
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
    play_frames = 0;
    unsaved_secs = 0;
    music_duck(false);
    game_pause_items(0, NULL, NULL);
    if (G()) G()->start();
}

static void runner_leave(void) {
    music_duck(false);
    input_set_versus(false); /* the library always gets the whole keyboard */
    input_set_spare_p2(false);
    shell_remap_apply(-1);   /* ... and every button as itself */
    if (unsaved_secs) progress_save();
    unsaved_secs = 0;
    if (G() && G()->quit) G()->quit();
    paused = false;
}

/* The volumes sit in the pause menu too, so a run never has to be left to
 * turn the music down. */
enum { PI_RESUME, PI_RESTART, PI_CONTROLS, PI_MUSIC, PI_SFX, PI_QUIT, PI_COUNT, PI_EXTRA = 100 };
static const char *PAUSE_ITEMS[PI_COUNT] = {"RESUME", "RESTART", "CONTROLS", "MUSIC", "SOUND FX", "QUIT TO LIBRARY"};
static bool vol_dirty;

/* the menu's rows: RESUME, the game's own items, then the rest */
static int pause_rows(void) { return PI_COUNT + pause_nextra; }
static int pause_item(int row) {
    if (row == 0) return PI_RESUME;
    if (row <= pause_nextra) return PI_EXTRA + row - 1;
    return row - pause_nextra;
}
static const char *pause_label(int row) {
    int it = pause_item(row);
    return it >= PI_EXTRA ? pause_extra[it - PI_EXTRA] : PAUSE_ITEMS[it];
}

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
    int rows = pause_rows();
    if (pause_sel >= rows) pause_sel = 0;
    if (btn_repeat(BTN_UP)) { pause_sel = (pause_sel + rows - 1) % rows; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { pause_sel = (pause_sel + 1) % rows; sfx_play_name("ui_move"); }
    int item = pause_item(pause_sel);
    /* the music plays at its real level while its volume is being set */
    music_duck(item != PI_MUSIC);
    int dir = btn_repeat(BTN_RIGHT) ? 1 : btn_repeat(BTN_LEFT) ? -1 : 0;
    if (dir && (item == PI_MUSIC || item == PI_SFX)) {
        uint8_t *v = item == PI_MUSIC ? &g_progress.music_vol : &g_progress.sfx_vol;
        *v = (uint8_t)iclamp(*v + dir, 0, 10);
        app_apply_settings();
        vol_dirty = true;
        sfx_play_name(item == PI_MUSIC ? "ui_move" : "ui_toast");
    }
    if (btnp(BTN_START) || btnp(BTN_B)) { sfx_play_name("ui_back"); unpause(); return; }
    if (btnp(BTN_A) && item >= PI_EXTRA) {
        sfx_play_name("ui_ok");
        unpause();
        if (pause_pick) pause_pick(item - PI_EXTRA);
        return;
    }
    if (btnp(BTN_A)) {
        switch (item) {
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

void app_test_pause(int page) {
    if (!G()) return;
    paused = true;
    pause_sel = 0;
    pause_page = iclamp(page, 0, 2);
    pause_t = 0;
}

void app_test_toast(int goal_bit) {
    if (!G()) return;
    toast_game_i = cur_game;
    toast_bit_i = goal_bit;
    toast_timer = 100; /* fully in view */
}

static void runner_update(void) {
    if (exit_requested) {
        exit_requested = false;
        music_fade(30);
        scene_goto(&SCENE_LIBRARY);
        return;
    }
    /* the cartridge's own button layout, but not in the pause menu */
    shell_remap_apply(paused ? -1 : cur_game);
    if (scene_transitioning()) return;
    /* play time: every second the cartridge runs unpaused (saved each
     * minute, and on the way out) */
    if (!paused && G() && ++play_frames >= 60) {
        play_frames = 0;
        if (g_progress.play_secs[cur_game] < 0xFFFFFFFFu) g_progress.play_secs[cur_game]++;
        if (++unsaved_secs >= 60) {
            progress_save();
            unsaved_secs = 0;
        }
    }
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

/* Every text element reports its box to the layout audit (ui_audit_*); the
 * pause_layout query draws each cartridge's pause menu, controls page and
 * goal toasts with the audit on. */
static void draw_pause_for(const GameDef *g) {
    gfx_camera(0, 0);
    gfx_noclip();
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 3);
    if (pause_page == 1) {
        ui_panel(30, 22, 260, 136, C_NIGHT, C_SLATE);
        ui_audit_area("controls panel", 32, 24, 256, 132);
        text_center("CONTROLS", 160, 30, C_YELLOW);
        ui_audit_text("heading", "CONTROLS", 160 - text_width("CONTROLS") / 2, 30);
        /* the list as this cartridge's button layout has it */
        char list[600];
        int slot = 0;
        while (slot < GAME_SLOTS - 1 && GAMES[slot] != g) slot++;
        shell_controls_for(slot, list, sizeof list);
        text_draw(list, 42, 46, C_LIGHT);
        ui_audit_text("controls", list, 42, 46);
        text_center(GLYPH_A " BACK", 160, 147, C_GREY);
        ui_audit_box("back, with room above", 160 - text_width(GLYPH_A " BACK") / 2, 145, text_width(GLYPH_A " BACK"), 9);
        return;
    }
    int py = 30 - pause_nextra * 7;
    ui_panel(84, py, 152, 120 + pause_nextra * 13, C_NIGHT, C_SLATE);
    ui_audit_area("pause panel", 86, py + 2, 148, 116 + pause_nextra * 13);
    gfx_rect(85, py + 1, 150, 15, C_DUSK);
    text_center("PAUSED", 160, py + 5, C_WHITE);
    ui_audit_text("heading", "PAUSED", 160 - text_width("PAUSED") / 2, py + 5);
    tiny_center(g->title, 160, py + 20, C_GREY);
    ui_audit_tiny("title", g->title, 160 - tiny_width(g->title) / 2, py + 20);
    if (pause_page == 2) {
        static const char *const q = "RESTART GAME?", *const d = "UNSAVED PROGRESS IS LOST";
        text_center(q, 160, py + 40, C_YELLOW);
        ui_audit_text("question", q, 160 - text_width(q) / 2, py + 40);
        ui_audit_centred("question", 160 - text_width(q) / 2, text_width(q));
        tiny_center(d, 160, py + 52, C_GREY);
        ui_audit_tiny("detail", d, 160 - tiny_width(d) / 2, py + 52);
        ui_audit_centred("detail", 160 - tiny_width(d) / 2, tiny_width(d));
        ui_choices(160, py + 68, "YES", "NO", confirm_sel, pause_t, C_WHITE, C_SLATE);
        return;
    }
    for (int i = 0; i < pause_rows(); i++) {
        int y = py + 32 + i * 13, it = pause_item(i);
        bool sel = i == pause_sel;
        if (sel) gfx_rect(92, y - 3, 136, 13, C_DUSK);
        text_draw(pause_label(i), 108, y, sel ? C_WHITE : C_GREY);
        ui_audit_text("item", pause_label(i), 108, y);
        if (sel) ui_cursor(98, y, pause_t);
        if (it == PI_MUSIC || it == PI_SFX) {
            int v = it == PI_MUSIC ? g_progress.music_vol : g_progress.sfx_vol;
            for (int k = 0; k < 10; k++) gfx_rect(170 + k * 5, y, 4, 7, k < v ? (sel ? C_YELLOW : C_AMBER) : C_INK);
            ui_audit_box("volume", 170, y, 49, 7);
        }
    }
}

static void draw_pause(void) { draw_pause_for(G()); }

/* The goal toast is 200 wide, wider for a long goal (up to the screen's
 * edges), and a line taller if the goal still needs two lines. */
static int toast_bi(int bit) { return bit == GOAL_BEACON ? 0 : bit == GOAL_SAUCER ? 1 : 2; }
static int toast_w(const GameDef *g, int bi) {
    return g ? iclamp(20 + tiny_width(g->goal_desc[bi]) + 7, 200, SCREEN_W - 8) : 200;
}
static int toast_lines(const GameDef *g, int bi, char l[2][UI_WRAP_LEN]) {
    return g ? ui_wrap(g->goal_desc[bi], toast_w(g, bi) - 27, true, l, 2) : 0;
}

/* The goal toast for game g's goal bit, sliding in to y. */
static void draw_toast_for(const GameDef *g, int bit, int y, int t) {
    gfx_camera(0, 0);
    gfx_noclip();
    int bi = toast_bi(bit);
    char l[2][UI_WRAP_LEN];
    int n = toast_lines(g, bi, l);
    int w = toast_w(g, bi), x = SCREEN_W / 2 - w / 2, h = n > 1 ? 28 : 22;
    ui_panel(x, y + 2, w, h, C_NIGHT, C_YELLOW);
    ui_audit_area("toast", x + 2, y + 4, w - 4, h - 4);
    ui_goal_icon(x + 6, y + 8, bit, true, t);
    ui_audit_box("icon", x + 6, y + 8, 9, 9);
    const char *names[3] = {"BEACON EARNED!", "SAUCER EARNED!", "ALIEN EARNED!"};
    text_draw(names[bi], x + 20, y + 5, C_YELLOW);
    ui_audit_text("heading", names[bi], x + 20, y + 5);
    static const char *const what[2] = {"goal line 1", "goal line 2"};
    for (int i = 0; i < imin(n, 2); i++) {
        tiny_draw(l[i], x + 20, y + 15 + i * 6, C_LIGHT);
        ui_audit_tiny(what[i], l[i], x + 20, y + 15 + i * 6);
    }
    if (n > 2) ui_audit_fail("goal", "needs more than two lines");
}

static void draw_toast(void) {
    int t = 200 - toast_timer;
    char l[2][UI_WRAP_LEN];
    int hide = toast_lines(GAMES[toast_game_i], toast_bi(toast_bit_i), l) > 1 ? -30 : -24; /* just off the top */
    int y = 0;
    if (t < 12) y = hide - hide * t / 12;
    else if (toast_timer < 12) y = hide - hide * toast_timer / 12;
    draw_toast_for(GAMES[toast_game_i], toast_bit_i, y, t);
}

/* Draws every cartridge's pause menu, controls page and goal toasts with
 * the layout audit on; returns the problems found. */
static int audit_runner(void) {
    int bad = 0, page = pause_page, nextra = pause_nextra;
    pause_nextra = 0; /* a game's own items only exist while it runs */
    char subject[64];
    for (int i = 0; i < GAME_SLOTS; i++) {
        const GameDef *g = GAMES[i];
        if (!g) continue;
        for (int p = 0; p <= 2; p++) {
            pause_page = p;
            snprintf(subject, sizeof subject, "pause %s, page %d", g->title, p);
            ui_audit_begin(subject, true);
            draw_pause_for(g);
            bad += ui_audit_end();
        }
        for (int b = 0; b < 3; b++) {
            snprintf(subject, sizeof subject, "toast %s, goal %d", g->title, b + 1);
            ui_audit_begin(subject, true);
            draw_toast_for(g, 1 << b, 0, 0);
            bad += ui_audit_end();
        }
    }
    pause_page = page;
    pause_nextra = nextra;
    return bad;
}

static void runner_draw(void) {
    if (G()) G()->draw();
    if (paused) draw_pause();
    if (toast_timer > 0) draw_toast();
}

const Scene SCENE_RUNNER = {"runner", runner_enter, runner_update, runner_draw, runner_leave};
