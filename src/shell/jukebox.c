/* UFO 40 - the JUKEBOX: every tune in the console and in each cartridge.
 * UP/DOWN browse, A plays (or stops) the tune under the cursor, LEFT/RIGHT
 * skip to the previous/next tune and play it, B goes back. A tune picked
 * here keeps playing through the console's menus. */
#include "shell.h"

/* Display titles. A song missing here shows its id without the prefix. */
static const struct { const char *id, *title; } TITLES[] = {
    {"boot", "BEAMDOWN"}, {"library", "SAUCER LOUNGE"},
    {"ud_mine", "LAMPLIGHT DESCENT"}, {"ud_deep", "EMBER DEEP"}, {"ud_boss", "THE OLD LODE"},
    {"ud_win", "SUNSTONE"}, {"ud_over", "LANTERN OUT"},
    {"gs_shift", "NIGHT SHIFT"}, {"gs_brief", "BRIEFING"}, {"gs_night", "LIGHTS OUT"},
    {"gs_win", "CONTRACT DONE"}, {"gs_lose", "CONTRACT LOST"},
    {"rc_rooftops", "ROOFTOP RUN"}, {"rc_market", "SPICE MARKET"}, {"rc_fort", "FORT AT MIDNIGHT"},
    {"rc_harbour", "HARBOUR BREEZE"}, {"rc_boss", "MAGPIE MAYHEM"}, {"rc_over", "OUT OF LIVES"},
    {"rc_ending", "PARCEL DELIVERED"},
    {"pp_garden", "GARDEN WALTZ"}, {"pp_rush", "NECTAR RUSH"}, {"pp_title", "PETAL LULLABY"},
    {"pp_end", "TWO HUNDRED HOME"}, {"pp_over", "WILTED"},
    {"tt_nursery", "NURSERY MARCH"}, {"tt_bath", "BATHWATER"}, {"tt_kitchen", "KITCHEN GALOP"},
    {"tt_chest", "THE TOY CHEST"}, {"tt_jack", "JACK OF THE CHEST"}, {"tt_title", "TIN TROOP FANFARE"},
    {"tt_map", "BARRACKS"}, {"tt_home", "THE TROOP COMES HOME"}, {"tt_clear", "LEVEL CLEAR"},
    {"tt_fail", "RETREAT"},
    {"sw_roots", "THE ROOTS"}, {"sw_spark", "THE SPARKWORKS"}, {"sw_reef", "THE SKY REEF"},
    {"sw_eye", "THE EYE"}, {"sw_title", "SKYWELL"}, {"sw_shop", "THE TINKER"}, {"sw_dawn", "DAWN"},
    {"sw_over", "THE FALL"},
    {"bf_march", "MARIGOLD MARCH"}, {"bf_battle", "LANES OF IRON"}, {"bf_table", "THE WAR TABLE"},
    {"bf_win", "VICTORY"}, {"bf_lose", "DEFEAT"},
    {"cc_title", "HEAVE AND HOIST"}, {"cc_select", "CHOOSE YOUR CREW"}, {"cc_bracket", "THE ROAD TO THE CUP"},
    {"cc_cup", "RAISE THE CUP"}, {"cc_point", "POINT!"}, {"cc_win", "MATCH WON"}, {"cc_lose", "MATCH LOST"},
    {"fn_oasis", "DRY OASIS"}, {"fn_stones", "THINKING STONES"}, {"fn_palace", "THE PALACE BATH"},
    {"fn_workshop", "THE WORKSHOP"}, {"fn_flows", "THE OASIS FLOWS"}, {"fn_spring", "ROOM CLEAR"},
    {"tn_island", "SALT ISLAND"}, {"tn_tiptoe", "TIPTOE"}, {"tn_sungate", "THE SUN GATE"},
    {"tn_firstlight", "FIRST LIGHT"}, {"tn_escape", "ESCAPED"}, {"tn_eaten", "EATEN"},
    {"ph_lanterns", "LANTERNS ON THE TERRACE"}, {"ph_groove", "HOUSE GROOVE"}, {"ph_market", "MARKET STREET"},
    {"ph_fourstars", "FOUR STARS"}, {"ph_lastnight", "LAST NIGHT"}, {"ph_siren", "SIRENS"}, {"ph_dusk", "DUSK"},
    {"dx_saltline", "SALT LINE"}, {"dx_quiet", "QUIET CARRIAGE"}, {"dx_guards", "GUARDS ON THE MOVE"},
    {"dx_theline", "THE LINE"}, {"dx_coastward", "COASTWARD"}, {"dx_getaway", "GETAWAY"}, {"dx_caught", "CAUGHT"},
};

const char *shell_song_title(int song) {
    static char buf[32];
    const char *id = song_name(song);
    for (int i = 0; i < ARRAY_LEN(TITLES); i++)
        if (!strcmp(TITLES[i].id, id)) return TITLES[i].title;
    const char *us = strchr(id, '_');
    snprintf(buf, sizeof buf, "%s", us ? us + 1 : id);
    for (char *p = buf; *p; p++) {
        if (*p == '_') *p = ' ';
        else if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 32);
    }
    return buf;
}

#define MAX_ROWS 320
#define LIST_Y 66
#define ROW_H 10
#define VISIBLE 9

typedef struct Row {
    int16_t song;  /* -1 for a heading */
    int8_t owner;  /* cartridge slot, -1 = the console */
} Row;

static Row rows[MAX_ROWS];
static int n_rows, n_songs, sel, top, t, play_t;

static void add_group(int owner) {
    int first = n_rows;
    if (n_rows < MAX_ROWS) rows[n_rows++] = (Row){-1, (int8_t)owner};
    for (int s = 0; s < song_count() && n_rows < MAX_ROWS; s++)
        if (shell_song_owner(s) == owner) { rows[n_rows++] = (Row){(int16_t)s, (int8_t)owner}; n_songs++; }
    if (n_rows == first + 1) n_rows = first; /* a cartridge with no music */
}

static void build(void) {
    n_rows = n_songs = 0;
    add_group(-1);
    for (int i = 0; i < GAME_SLOTS; i++)
        if (GAMES[i]) add_group(i);
}

static int row_of_song(int song) {
    for (int i = 0; i < n_rows; i++)
        if (rows[i].song == song) return i;
    return -1;
}

static int step_song(int from, int dir) {
    int i = from;
    for (int k = 0; k < n_rows; k++) {
        i = (i + dir + n_rows) % n_rows;
        if (rows[i].song >= 0) return i;
    }
    return from;
}

static bool playing_pick(void) {
    return g_jukebox_song >= 0 && music_playing() == g_jukebox_song && !music_finished();
}

static void play_row(int r) {
    if (r < 0 || rows[r].song < 0) return;
    g_jukebox_song = rows[r].song;
    music_restart(g_jukebox_song);
    play_t = 0;
}

static void jb_enter(void) {
    t = 0;
    build();
    int r = playing_pick() ? row_of_song(g_jukebox_song) : -1;
    if (r >= 0) sel = r;
    if (sel <= 0 || sel >= n_rows || rows[sel].song < 0) sel = step_song(0, 1);
}

static void jb_update(void) {
    t++;
    if (playing_pick()) play_t++;
    if (scene_transitioning()) return;
    if (btn_repeat(BTN_UP)) { sel = step_song(sel, -1); sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { sel = step_song(sel, 1); sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_LEFT)) { sel = step_song(sel, -1); play_row(sel); }
    if (btn_repeat(BTN_RIGHT)) { sel = step_song(sel, 1); play_row(sel); }
    if (btnp(BTN_A) || btnp(BTN_START)) {
        if (playing_pick() && g_jukebox_song == rows[sel].song) {
            music_stop();
            g_jukebox_song = -1;
            sfx_play_name("ui_back");
        } else {
            play_row(sel);
        }
    }
    if (btnp(BTN_B) || btnp(BTN_SELECT)) {
        sfx_play_name("ui_back");
        scene_goto(&SCENE_SETTINGS);
    }
    /* keep the cursor on screen */
    if (sel < top + 1) top = sel - 1;
    if (sel > top + VISIBLE - 2) top = sel - VISIBLE + 2;
    top = iclamp(top, 0, imax(0, n_rows - VISIBLE));
}

static const char *group_name(int owner) {
    static char buf[40];
    if (owner < 0) return "UFO 40 CONSOLE";
    snprintf(buf, sizeof buf, "%02d %s", owner + 1, GAMES[owner] ? GAMES[owner]->title : "");
    return buf;
}

static int song_seconds(int song) {
    int ticks = 0;
    for (int c = 0; c < CH_COUNT; c++) ticks = imax(ticks, song_channel_ticks(song, c));
    int bpm = song_bpm(song);
    return bpm > 0 ? ticks * 60 / (48 * bpm) : 0;
}

static void jb_draw(void) {
    ui_starfield(t, C_INK);
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center("JUKEBOX", 160, 5, 2, grad, 3, C_INK, C_WINE);

    /* now playing */
    ui_panel(10, 24, 300, 36, C_NIGHT, playing_pick() ? C_YELLOW : C_SLATE);
    char buf[80];
    if (playing_pick()) {
        text_draw(GLYPH_NOTE, 16, 30, (t / 15) % 2 ? C_YELLOW : C_AMBER);
        text_draw(shell_song_title(g_jukebox_song), 26, 30, C_WHITE);
        int len = song_seconds(g_jukebox_song), el = play_t / 60;
        if (song_loops(g_jukebox_song)) snprintf(buf, sizeof buf, "%s " GLYPH_DOT " LOOPS EVERY %d:%02d " GLYPH_DOT " %d:%02d",
                                                 group_name(shell_song_owner(g_jukebox_song)), len / 60, len % 60, el / 60, el % 60);
        else snprintf(buf, sizeof buf, "%s " GLYPH_DOT " JINGLE %d:%02d", group_name(shell_song_owner(g_jukebox_song)), len / 60, len % 60);
        tiny_draw(buf, 26, 42, C_GREY);
        /* a little level meter */
        float peak = 0, rms = 0;
        audio_stats(&peak, &rms);
        for (int i = 0; i < 8; i++) {
            float wob = 0.55f + 0.45f * sinf((float)t * 0.21f + (float)i * 1.7f);
            int h = iclamp((int)((rms * 3.0f + 0.12f) * wob * 22.0f), 1, 20);
            gfx_rect(262 + i * 5, 50 - h, 4, h, h > 14 ? C_ORANGE : h > 8 ? C_YELLOW : C_LIME);
        }
    } else {
        text_draw("NOTHING PLAYING", 26, 30, C_GREY);
        tiny_draw("PICK A TUNE AND PRESS A", 26, 42, C_SLATE);
    }

    /* the list */
    ui_panel(10, LIST_Y - 4, 300, VISIBLE * ROW_H + 6, C_NIGHT, C_DUSK);
    for (int v = 0; v < VISIBLE && top + v < n_rows; v++) {
        int r = top + v, y = LIST_Y + v * ROW_H;
        const Row *row = &rows[r];
        if (row->song < 0) {
            tiny_draw(group_name(row->owner), 18, y + 2, C_YELLOW);
            gfx_hline(18 + tiny_width(group_name(row->owner)) + 4, 300, y + 4, C_DUSK);
            continue;
        }
        bool s = r == sel, on = playing_pick() && g_jukebox_song == row->song;
        if (s) gfx_rect(14, y - 1, 292, ROW_H, C_DUSK);
        if (s) ui_cursor(16, y, t);
        if (on) text_draw(GLYPH_NOTE, 26, y, C_YELLOW);
        text_draw(shell_song_title(row->song), 36, y, s ? C_WHITE : on ? C_YELLOW : C_GREY);
        int len = song_seconds(row->song);
        snprintf(buf, sizeof buf, "%s %d:%02d", song_loops(row->song) ? "LOOP" : "JINGLE", len / 60, len % 60);
        tiny_draw(buf, 302 - tiny_width(buf), y + 1, s ? C_LIGHT : C_SLATE);
    }
    /* scroll marks */
    if (top > 0) text_draw(GLYPH_UP, 300, LIST_Y - 12, C_GREY);
    if (top + VISIBLE < n_rows) text_draw(GLYPH_DOWN, 300, LIST_Y + VISIBLE * ROW_H, C_GREY);

    gfx_rect(0, 167, SCREEN_W, 13, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 166, C_DUSK);
    int fx = ui_hint(6, 170, GLYPH_A, playing_pick() && g_jukebox_song == rows[sel].song ? "STOP" : "PLAY", C_LIGHT);
    fx = ui_hint(fx, 170, GLYPH_LEFT GLYPH_RIGHT, "SKIP", C_LIGHT);
    ui_hint(fx, 170, GLYPH_B, "BACK", C_LIGHT);
    snprintf(buf, sizeof buf, "%d TUNES", n_songs);
    text_draw(buf, SCREEN_W - 6 - text_width(buf), 170, C_GREY);
}

const Scene SCENE_JUKEBOX = {"jukebox", jb_enter, jb_update, jb_draw, NULL};

bool jukebox_query(const char *key, int *out) {
    if (!strcmp(key, "jukebox_sel")) { *out = n_rows > 0 ? rows[sel].song : -1; return true; }
    if (!strcmp(key, "jukebox_songs")) { build(); *out = n_songs; return true; }
    if (!strcmp(key, "jukebox_playing")) { *out = playing_pick(); return true; }
    if (!strcmp(key, "jukebox_owner")) { *out = n_rows > 0 ? rows[sel].owner : -2; return true; }
    if (!strcmp(key, "jukebox_missing")) { build(); *out = song_count() - n_songs; return true; }
    return false;
}
