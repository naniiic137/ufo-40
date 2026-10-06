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
    {"wp_title", "WET PAINT"}, {"wp_race1", "FIRST COAT"}, {"wp_race2", "SECOND COAT"}, {"wp_race3", "TOP COAT"},
    {"wp_final", "SHOWDOWN"}, {"wp_cut", "INTERMISSION"}, {"wp_end", "PLATINUM"}, {"wp_clear", "COURSE CLEAR"},
    {"wp_miss", "NOT BLUE ENOUGH"}, {"wp_over", "RUN DRY"},
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
    {"dk_title", "DUSK BELL"}, {"dk_dusk", "LAST LIGHT"}, {"dk_wake", "WAKING"}, {"dk_wood", "HUSH WOOD"},
    {"dk_mere", "SUNKEN MERE"}, {"dk_steps", "OLD STEPS"}, {"dk_works", "HUMMING WORKS"},
    {"dk_caves", "EMBER CAVES"}, {"dk_heights", "WINDY HEIGHTS"}, {"dk_pocket", "HIDDEN PLACES"},
    {"dk_boss", "THREE KEEPERS"}, {"dk_egg", "THE EGG"},
    {"bm_title", "LANTERN LANE"}, {"bm_night", "MIDNIGHT MARKET"}, {"bm_king", "THE BOG KING"},
    {"bm_end", "SKY FULL OF SPARKS"}, {"bm_hold", "MOSSBURY HOLDS"}, {"bm_over", "OVERRUN"},
    {"cc_title", "HEAVE AND HOIST"}, {"cc_select", "CHOOSE YOUR CREW"}, {"cc_bracket", "THE ROAD TO THE CUP"},
    {"cc_cup", "RAISE THE CUP"}, {"cc_point", "POINT!"}, {"cc_win", "MATCH WON"}, {"cc_lose", "MATCH LOST"},
    {"fn_oasis", "DRY OASIS"}, {"fn_stones", "THINKING STONES"}, {"fn_palace", "THE PALACE BATH"},
    {"fn_workshop", "THE WORKSHOP"}, {"fn_flows", "THE OASIS FLOWS"}, {"fn_spring", "ROOM CLEAR"},
    {"tn_island", "SALT ISLAND"}, {"tn_tiptoe", "TIPTOE"}, {"tn_sungate", "THE LIGHTHOUSE"},
    {"tn_firstlight", "FIRST LIGHT"}, {"tn_escape", "ESCAPED"}, {"tn_eaten", "EATEN"},
    {"ph_lanterns", "LANTERNS ON THE TERRACE"}, {"ph_groove", "HOUSE GROOVE"}, {"ph_market", "MARKET STREET"},
    {"ph_fourstars", "FOUR STARS"}, {"ph_lastnight", "LAST NIGHT"}, {"ph_siren", "SIRENS"}, {"ph_dusk", "DUSK"},
    {"skid_title", "SKID KIDS"}, {"skid_pick", "PICK ME, PICK ME"}, {"skid_bracket", "THE BIG BOARD"},
    {"skid_gym", "GYM CLASS"}, {"skid_recess", "RECESS RALLY"}, {"skid_roo", "FIELD TRIP"},
    {"skid_bot", "BENCHBOT"}, {"skid_champs", "GYM CHAMPIONS"}, {"skid_credits", "HIT THE SHOWERS"},
    {"skid_win", "GOOD GAME"}, {"skid_lose", "SIT THIS ONE OUT"}, {"skid_over", "ON THE BENCH"},
    {"dx_saltline", "SALT LINE"}, {"dx_quiet", "QUIET CARRIAGE"}, {"dx_guards", "GUARDS ON THE MOVE"},
    {"dx_theline", "THE LINE"}, {"dx_coastward", "COASTWARD"}, {"dx_getaway", "GETAWAY"}, {"dx_caught", "CAUGHT"},
    {"dd_title", "DOT & DASH"}, {"dd_room", "FULL SIZE"}, {"dd_small", "BUTTON HIGH"}, {"dd_micro", "SPECK COUNTRY"},
    {"dd_deep", "THE MOTES"}, {"dd_town", "LITTLE MARKET"}, {"dd_walls", "BETWEEN THE WALLS"}, {"dd_boss", "BIG TROUBLE"},
    {"dd_latch", "LATCHTOWN"}, {"dd_siege", "SIEGE!"}, {"dd_ending", "THE PARTY"}, {"dd_true", "IN BALANCE"},
    {"dd_upgrade", "SOMETHING NEW"}, {"dd_back", "BACK TO SIZE"}, {"dd_micro2", "MOSS AND LOAM"},
    {"dd_micro3", "TIN AND GLASS"}, {"dd_cave", "THE DANGEROUS CAVES"},
    {"lnk_title", "LOST LINKS"}, {"lnk_links", "FAIRWAY BREEZE"}, {"lnk_caves", "UNDER THE LINKS"},
    {"lnk_sands", "BUNKER HEAT"}, {"lnk_fen", "HERON FEN"}, {"lnk_ruins", "THE OLD CLUBHOUSE"},
    {"lnk_badger", "BRASS BADGER"}, {"lnk_end", "GREEN AGAIN"}, {"lnk_item", "FOUND IT"}, {"lnk_out", "OUT OF STROKES"},
    {"lnk_tape", "SCORECROW TAPE"},
    {"mnd_title", "BLUEBELL COLONY"}, {"mnd_map", "ORDERS FROM ABOVE"}, {"mnd_field", "MOLASSES MARCH"},
    {"mnd_capital", "THE OLD HALLS"}, {"mnd_versus", "PICNIC WAR"}, {"mnd_win", "FIELD TAKEN"},
    {"mnd_lose", "BACK TO THE NEST"},
    {"fh_title", "FLINTHOLD"}, {"fh_map", "THE ISLAND"}, {"fh_village", "HEARTH SONG"},
    {"fh_battle1", "HORNS AT DAWN"}, {"fh_battle2", "ASH AND FERN"}, {"fh_battle3", "VINES AND WINGS"},
    {"fh_lords", "THE FOUR LORDS"}, {"fh_end", "EMBERS HOME"}, {"fh_wave", "WAVE HELD"},
    {"fh_clear", "THE CAVE HOLDS"}, {"fh_fell", "THE CAVE FALLS"},
    {"tsh_title", "COMET CLASSIC"}, {"tsh_front", "FRONT NINE"}, {"tsh_back", "BACK NINE"},
    {"tsh_last", "EIGHTEENTH HOLE"}, {"tsh_board", "THE LEADERBOARD"}, {"tsh_champ", "CHAMPION OF THE COMET"},
    {"tsh_cup", "IN THE CUP"}, {"tsh_ace", "HOLE IN ONE"}, {"tsh_over", "OVER PAR"}, {"tsh_runnerup", "RUNNER-UP"},
    {"hs_title", "HOMESPUN"}, {"hs_camp", "GLOWSTONE CAMP"}, {"hs_wilds", "INTO THE WILDS"}, {"hs_deep", "DEEPER DOWN"},
    {"hs_cave", "DRIPPING CAVES"}, {"hs_boss", "GUARDIAN"}, {"hs_mawbo", "MAWBO"}, {"hs_end", "THE LONG WAY HOME"},
    {"hs_home", "HOME SAFE"}, {"hs_fade", "TOLLY'S DRAG"},
    {"bzz_title", "BUZZBOLT"}, {"bzz_select", "CHOOSE YOUR WING"}, {"bzz_meadow", "OUTER MEADOW"},
    {"bzz_thicket", "THE THICKET"}, {"bzz_rot", "THE ROT"}, {"bzz_walls", "THE WALLS"}, {"bzz_boss", "THE BIG ONES"},
    {"bzz_heart", "THE SPOREHEART"}, {"bzz_clear", "WAVE CLEAR"}, {"bzz_over", "SHOT DOWN"}, {"bzz_bloom", "BLOOM"},
    {"bzz_home", "FLYING HOME"}, {"bzz_name", "SIGN YOUR NAME"},
    {"rsh_title", "RIMSHIRE"}, {"rsh_map", "THE ROADS OF RIMSHIRE"}, {"rsh_battle", "FLICK AND FLING"},
    {"rsh_battle2", "BANK SHOT"}, {"rsh_empress", "THE PLUM EMPRESS"}, {"rsh_end", "BRASS AT PEACE"},
    {"rsh_win", "FIELD WON"}, {"rsh_lose", "FIELD LOST"}, {"rsh_warwon", "THE WAR IS WON"}, {"rsh_warlost", "BANNER DOWN"},
    {"wb_title", "CRATER DOWNS"}, {"wb_paddock", "THE TOTE BOARD"}, {"wb_race", "POST TIME"}, {"wb_final", "THE BIG PAYOUT"},
    {"wb_news", "WOBBLE WIRE"}, {"wb_win", "PHOTO FINISH"}, {"wb_lose", "TORN TICKET"}, {"wb_bell", "OFF THEY GO"},
    {"chm_title", "CHIME CIRCUIT"}, {"chm_pit", "PIT LANE"}, {"chm_race1", "FULL THRUST"}, {"chm_race2", "UPDRAFT"},
    {"chm_race3", "PINBALL PACK"}, {"chm_finale", "GRAND OCTAVE"}, {"chm_cup", "THE CUP"}, {"chm_home", "HOMEWARD"},
    {"chm_flag", "CHEQUERED FLAG"}, {"chm_also", "ALSO RAN"},
    {"htk_whistle", "WHISTLE BLOWS"}, {"htk_match", "SATURDAY LEAGUE"}, {"htk_final", "CUP FINAL"},
    {"htk_trophy", "LAP OF HONOUR"}, {"htk_clear", "PITCH CLEARED"}, {"htk_over", "FULL TIME"},
    {"htk_shootout", "SHOOTOUT"},
    {"dfl_title", "DRIFTLINE"}, {"dfl_harbour", "HARBOUR ROAD"}, {"dfl_sundown", "SUNDOWN STRIP"},
    {"dfl_moonlit", "MOONLIT MILE"}, {"dfl_openwater", "OPEN WATER"}, {"dfl_boss", "TROUBLE ON THE COAST"},
    {"dfl_hightide", "HIGH TIDE"}, {"dfl_bonus", "BUBBLE AND BOUNCE"}, {"dfl_ending", "THE LAST MILE"},
    {"dfl_credits", "TAIL LIGHTS"}, {"dfl_clear", "STAGE CLEAR"}, {"dfl_over", "OUT OF CARS"},
    {"dfl_coin", "TWO MORE CARS"},
    {"tkw_dream", "DRIFTSLEEP"}, {"tkw_hall", "THE DEEP HALL"}, {"tkw_ending", "WAKING TIDE"},
    {"tkw_brawl", "FLOE BRAWL"}, {"tkw_wake", "EYES OPEN"}, {"tkw_round", "ROUND TO YOU"},
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

#define MAX_ROWS 448 /* every song plus a heading per cartridge */
#define LIST_Y 69 /* the scroll marks fit between the panels */
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

/* Every text element reports its box to the layout audit (ui_audit_*); the
 * jukebox_layout query draws every row and every song's now-playing panel
 * with the audit on. */
#define METER_X 262

/* The now-playing panel for song (-1 = nothing), el seconds in. */
static void draw_now(int song, int el) {
    ui_panel(10, 24, 300, 36, C_NIGHT, song >= 0 ? C_YELLOW : C_SLATE);
    ui_audit_area("now playing", 12, 26, METER_X - 2 - 12, 32); /* left of the meter */
    char buf[80];
    if (song >= 0) {
        text_draw(GLYPH_NOTE, 16, 30, (t / 15) % 2 ? C_YELLOW : C_AMBER);
        ui_audit_text("note", GLYPH_NOTE, 16, 30);
        text_draw(shell_song_title(song), 26, 30, C_WHITE);
        ui_audit_text("song title", shell_song_title(song), 26, 30);
        int len = song_seconds(song);
        if (song_loops(song)) snprintf(buf, sizeof buf, "%s " GLYPH_DOT " LOOPS EVERY %d:%02d " GLYPH_DOT " %d:%02d",
                                       group_name(shell_song_owner(song)), len / 60, len % 60, el / 60, el % 60);
        else snprintf(buf, sizeof buf, "%s " GLYPH_DOT " JINGLE %d:%02d", group_name(shell_song_owner(song)), len / 60, len % 60);
        tiny_draw(buf, 26, 42, C_GREY);
        ui_audit_tiny("song info", buf, 26, 42);
        /* a little level meter */
        float peak = 0, rms = 0;
        audio_stats(&peak, &rms);
        for (int i = 0; i < 8; i++) {
            float wob = 0.55f + 0.45f * sinf((float)t * 0.21f + (float)i * 1.7f);
            int h = iclamp((int)((rms * 3.0f + 0.12f) * wob * 22.0f), 1, 20);
            gfx_rect(METER_X + i * 5, 50 - h, 4, h, h > 14 ? C_ORANGE : h > 8 ? C_YELLOW : C_LIME);
        }
    } else {
        text_draw("NOTHING PLAYING", 26, 30, C_GREY);
        ui_audit_text("song title", "NOTHING PLAYING", 26, 30);
        tiny_draw("PICK A TUNE AND PRESS A", 26, 42, C_SLATE);
        ui_audit_tiny("song info", "PICK A TUNE AND PRESS A", 26, 42);
    }
}

/* Row r of the list at y. */
static void draw_row(int r, int y) {
    const Row *row = &rows[r];
    ui_audit_area("list row", 14, y - 1, 292, ROW_H);
    if (row->song < 0) {
        const char *name = group_name(row->owner);
        tiny_draw(name, 18, y + 2, C_YELLOW);
        ui_audit_tiny("heading", name, 18, y + 2);
        int rx = 18 + tiny_width(name) + 4;
        if (rx < 300) gfx_hline(rx, 300, y + 4, C_DUSK);
        return;
    }
    char buf[40];
    bool s = r == sel, on = playing_pick() && g_jukebox_song == row->song;
    if (s) gfx_rect(14, y - 1, 292, ROW_H, C_DUSK);
    if (s) ui_cursor(16, y, t);
    if (on) text_draw(GLYPH_NOTE, 26, y, C_YELLOW);
    ui_audit_box("cursor and note", 16, y, 16, 7);
    text_draw(shell_song_title(row->song), 36, y, s ? C_WHITE : on ? C_YELLOW : C_GREY);
    ui_audit_text("song title", shell_song_title(row->song), 36, y);
    int len = song_seconds(row->song);
    snprintf(buf, sizeof buf, "%s %d:%02d", song_loops(row->song) ? "LOOP" : "JINGLE", len / 60, len % 60);
    tiny_draw(buf, 302 - tiny_width(buf), y + 1, s ? C_LIGHT : C_SLATE);
    ui_audit_tiny("length", buf, 302 - tiny_width(buf), y + 1);
}

/* The scroll marks, in the gaps above and below the list's panel (the
 * glyphs' top and bottom rows are blank). */
static void draw_marks(bool up, bool down) {
    int ly = LIST_Y - 4, lh = VISIBLE * ROW_H + 6;
    ui_audit_area("screen", 0, 0, SCREEN_W, 166);
    ui_audit_box("now playing panel", 10, 24, 300, 36);
    ui_audit_box("list panel", 10, ly, 300, lh);
    if (up) text_draw(GLYPH_UP, 300, ly - 6, C_GREY);
    if (down) text_draw(GLYPH_DOWN, 300, ly + lh - 1, C_GREY);
    ui_audit_box("scroll up mark", 300, ly - 5, 7, 4);
    ui_audit_box("scroll down mark", 300, ly + lh, 7, 4);
}

static void jb_draw(void) {
    ui_starfield(t, C_INK);
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center("JUKEBOX", 160, 5, 2, grad, 3, C_INK, C_WINE);

    draw_now(playing_pick() ? g_jukebox_song : -1, play_t / 60);

    /* the list */
    ui_panel(10, LIST_Y - 4, 300, VISIBLE * ROW_H + 6, C_NIGHT, C_DUSK);
    for (int v = 0; v < VISIBLE && top + v < n_rows; v++) draw_row(top + v, LIST_Y + v * ROW_H);
    draw_marks(top > 0, top + VISIBLE < n_rows);

    gfx_rect(0, 167, SCREEN_W, 13, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 166, C_DUSK);
    int fx = ui_hint(6, 170, GLYPH_A, playing_pick() && g_jukebox_song == rows[sel].song ? "STOP" : "PLAY", C_LIGHT);
    fx = ui_hint(fx, 170, GLYPH_LEFT GLYPH_RIGHT, "SKIP", C_LIGHT);
    ui_hint(fx, 170, GLYPH_B, "BACK", C_LIGHT);
    char buf[24];
    snprintf(buf, sizeof buf, "%d TUNES", n_songs);
    text_draw(buf, SCREEN_W - 6 - text_width(buf), 170, C_GREY);
}

/* Draws every row and every song's now-playing panel (at its longest
 * elapsed time) with the layout audit on; returns the problems found. */
static int audit_all(void) {
    build();
    int bad = 0;
    char subject[80];
    for (int r = 0; r < n_rows; r++) {
        snprintf(subject, sizeof subject, "jukebox row %d (%s)", r,
                 rows[r].song < 0 ? group_name(rows[r].owner) : shell_song_title(rows[r].song));
        ui_audit_begin(subject, true);
        draw_row(r, LIST_Y);
        bad += ui_audit_end();
        if (rows[r].song < 0) continue;
        snprintf(subject, sizeof subject, "jukebox now playing %s", shell_song_title(rows[r].song));
        ui_audit_begin(subject, true);
        draw_now(rows[r].song, 59 * 60 + 59);
        bad += ui_audit_end();
    }
    ui_audit_begin("jukebox scroll marks", true);
    draw_marks(true, true);
    bad += ui_audit_end();
    ui_audit_begin("jukebox, nothing playing", true);
    draw_now(-1, 0);
    return bad + ui_audit_end();
}

const Scene SCENE_JUKEBOX = {"jukebox", jb_enter, jb_update, jb_draw, NULL};

bool jukebox_query(const char *key, int *out) {
    if (!strcmp(key, "jukebox_sel")) { *out = n_rows > 0 ? rows[sel].song : -1; return true; }
    if (!strcmp(key, "jukebox_songs")) { build(); *out = n_songs; return true; }
    if (!strcmp(key, "jukebox_playing")) { *out = playing_pick(); return true; }
    if (!strcmp(key, "jukebox_owner")) { *out = n_rows > 0 ? rows[sel].owner : -2; return true; }
    if (!strcmp(key, "jukebox_missing")) { build(); *out = song_count() - n_songs; return true; }
    /* layout problems in every row and every song's now-playing panel */
    if (!strcmp(key, "jukebox_layout")) { *out = audit_all(); return true; }
    return false;
}
