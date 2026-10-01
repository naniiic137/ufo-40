/* HAT TRICK - Teddy and Mae take back every pitch in town with one ball.
 * Cartridge 11 of UFO 40, a tribute to Kick Club (UFO 50 #11). See
 * docs/games/11-hat-trick.md. This file: the title, the kid select, the
 * codes, the story, the run from 1-1 to the Captain, game over, the ending,
 * the versus pitch, the save and the test hooks. The rules of a screen are
 * in hattrick_play.c. */
#include "hattrick.h"

#define SAVE_MAGIC 0x48544B01u
#define CHERRY_SCORE 150000u

enum { ST_TITLE, ST_SELECT, ST_CODES, ST_STORY, ST_PLAY, ST_OVER, ST_ENDING, ST_CREDITS, ST_VSWIN };

/* ---- the codes: both open the unfinished versus pitch --------------------------------- */
enum { CODE_VS10 = 1, CODE_VS50 = 2 };
static const struct { const char *code; int bit; const char *says; } CODES[] = {
    {"NUTMEG10", CODE_VS10, "VERSUS ON! FIRST TO 10 GOALS."},
    {"NUTMEG50", CODE_VS50, "VERSUS ON! FIRST TO 50 GOALS."},
};
#define CODE_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

/* The cartridge save: records only (the original saves no run). */
typedef struct {
    uint32_t magic;
    uint32_t top;           /* best score */
    uint8_t best_level;     /* furthest screen reached, 1..40 */
    uint8_t pad;
    uint16_t runs, wins, cherries;
    uint16_t most_desserts; /* the most desserts eaten in one run */
    uint16_t pad2;
} HtkSave;

static HtkSave sv;
static int fair_pct, fair_idle; /* the last fairness check */
static struct {
    int state, state_t, t;
    int sel, kid_sel;
    unsigned codes;
    char code_buf[9];
    int code_pos, code_msg_t;
    const char *code_msg;
    bool won, cherry;
    int best_level_run;
    bool bot1, bot2;        /* tests: the demo player drives a kid */
} ui;

static void set_state(int s) {
    ui.state = s;
    ui.state_t = 0;
}
static bool codes_on(void) { return ui.codes != 0; }
static bool vita_single(void) { return plat_kind() == PLAT_VITA; }

/* ---- the save -------------------------------------------------------------------------- */

static void load_save(void) {
    HtkSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { sv = tmp; return; }
    memset(&sv, 0, sizeof sv);
    sv.magic = SAVE_MAGIC;
}

static void save_now(void) {
    if (codes_on()) return; /* a code turns saving off until the cartridge is left */
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void award(int bit) {
    if (!codes_on()) game_award(bit);
}

static uint32_t run_best_score(void) {
    uint32_t s = htk.pl[0].score;
    if (htk.pl[1].on && htk.pl[1].score > s) s = htk.pl[1].score;
    return s;
}

/* the run is over, won or lost: keep its records */
static void run_over(void) {
    if (codes_on()) return;
    uint32_t s = run_best_score();
    if (s > sv.top) sv.top = s;
    if (ui.best_level_run > sv.best_level) sv.best_level = (uint8_t)ui.best_level_run;
    int d = htk.pl[0].desserts + (htk.pl[1].on ? htk.pl[1].desserts : 0);
    if (d > sv.most_desserts) sv.most_desserts = (uint16_t)d;
    if (ui.won && sv.wins < 65535) sv.wins++;
    if (ui.cherry && sv.cherries < 65535) sv.cherries++;
    save_now();
}

/* ---- the flow ------------------------------------------------------------------------- */

static void to_title(void) {
    set_state(ST_TITLE);
    game_set_pausable(false);
    input_set_versus(false);
    music_play(HTK_MUS_WHISTLE);
}

static void level_music(void) {
    music_play(htk.level % HTK_PER_WORLD == HTK_PER_WORLD - 1 ? HTK_MUS_FINAL : HTK_MUS_MATCH);
}

static void go_level(int level) {
    htk_load_level(&htk, level);
    if (level + 1 > ui.best_level_run) ui.best_level_run = level + 1;
    level_music();
    htk_bot_reset();
}

static void new_run(int mode, int kid) {
    memset(&htk, 0, sizeof htk);
    htk.mode = mode;
    rng_seed(&htk.rng, rng_next(&g_rng));
    htk.pl[0].on = 1;
    htk.pl[0].ch = (int16_t)kid;
    htk.pl[0].spare = HTK_START_SPARE;
    if (mode != MODE_1P) {
        htk.pl[1].on = 1;
        htk.pl[1].ch = KID_MAE;
        htk.pl[1].spare = HTK_START_SPARE;
        htk.pl[0].ch = KID_TEDDY;
    }
    ui.won = ui.cherry = false;
    ui.best_level_run = 0;
    input_set_versus(mode != MODE_1P);
    game_set_pausable(true);
    if (mode == MODE_VS) {
        htk.vs_target = (ui.codes & CODE_VS50) ? 50 : 10;
        htk_load_level(&htk, -1);
        set_state(ST_PLAY);
        music_play(HTK_MUS_SHOOTOUT);
        return;
    }
    if (!codes_on() && sv.runs < 65535) { sv.runs++; save_now(); }
    set_state(ST_STORY);
    music_play(HTK_MUS_WHISTLE);
}

static void start_ending(void) {
    ui.won = true;
    award(GOAL_SAUCER);
    if (run_best_score() >= CHERRY_SCORE) {
        ui.cherry = true;
        award(GOAL_ALIEN);
    }
    set_state(ST_ENDING);
    game_set_pausable(false);
    music_play(HTK_MUS_TROPHY);
}

static void play_update(void) {
    game_set_pausable(true);
    uint32_t held = input_held();
    uint16_t p0 = (uint16_t)(held & 0xFF), p1 = (uint16_t)((held >> BTN_P2_SHIFT) & 0xFF);
    int sub0 = htk.sub;
    htk_step(&htk, p0, p1);
    if (sub0 == LS_PLAY && htk.sub == LS_CLEAR) music_play(HTK_MUS_CLEAR);
    if (htk.ev_vswin) {
        htk.ev_vswin = 0;
        set_state(ST_VSWIN);
        game_set_pausable(false);
        music_play(HTK_MUS_TROPHY);
        return;
    }
    if (htk.ev_over) {
        htk.ev_over = 0;
        set_state(ST_OVER);
        game_set_pausable(false);
        music_play(HTK_MUS_OVER);
        run_over();
        return;
    }
    if (htk.ev_exit) {
        htk.ev_exit = 0;
        int done = htk.level;
        if (done == 2 * HTK_PER_WORLD - 1) award(GOAL_BEACON); /* Lucky Lanes beaten: two worlds */
        if (done + 1 >= HTK_LEVELS) start_ending();
        else go_level(done + 1);
    }
}

/* ---- the screens' logic ---------------------------------------------------------------- */

static int title_items(void) { return codes_on() ? 4 : 3; }

static void title_update(void) {
    game_set_pausable(false);
    if (ui.code_msg_t > 0) ui.code_msg_t--;
    int n = title_items();
    if (btn_repeat(BTN_UP)) { ui.sel = (ui.sel + n - 1) % n; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { ui.sel = (ui.sel + 1) % n; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if (ui.state_t < 10 || !(btnp(BTN_A) || btnp(BTN_START))) return;
    if ((ui.sel == 1 || ui.sel == 3) && vita_single()) { sfx_play_name("ui_error"); return; }
    sfx_play_name("ui_ok");
    input_consume();
    switch (ui.sel) {
    case 0: set_state(ST_SELECT); break;
    case 1: new_run(MODE_COOP, KID_TEDDY); break;
    case 2:
        memset(ui.code_buf, 'A', 8);
        ui.code_buf[8] = 0;
        ui.code_pos = 0;
        ui.code_msg = NULL;
        ui.code_msg_t = 0;
        set_state(ST_CODES);
        break;
    default: new_run(MODE_VS, KID_TEDDY); break;
    }
}

static void select_update(void) {
    if (btnp(BTN_LEFT) || btnp(BTN_RIGHT)) { ui.kid_sel ^= 1; sfx_play_name("ui_move"); }
    if (btnp(BTN_B) && ui.state_t > 6) { sfx_play_name("ui_back"); to_title(); return; }
    if (ui.state_t > 6 && (btnp(BTN_A) || btnp(BTN_START))) {
        sfx_play_name("ui_ok");
        input_consume();
        new_run(MODE_1P, ui.kid_sel);
    }
}

static void codes_update(void) {
    if (ui.code_msg_t > 0) ui.code_msg_t--;
    int d = btn_repeat(BTN_RIGHT) - btn_repeat(BTN_LEFT);
    if (d) { ui.code_pos = (ui.code_pos + d + 8) % 8; sfx_play_name("ui_move"); }
    int v = btn_repeat(BTN_UP) - btn_repeat(BTN_DOWN);
    if (v) {
        const char *set = CODE_CHARS;
        int n = (int)strlen(set);
        const char *p = strchr(set, ui.code_buf[ui.code_pos]);
        int i = p ? (int)(p - set) : 0;
        ui.code_buf[ui.code_pos] = set[(i + v + n) % n];
        sfx_play_name("ui_move");
    }
    if (btnp(BTN_B) && ui.state_t > 6) { sfx_play_name("ui_back"); to_title(); return; }
    if (ui.state_t > 6 && (btnp(BTN_A) || btnp(BTN_START))) {
        ui.code_msg = "NO SUCH CODE.";
        for (int i = 0; i < ARRAY_LEN(CODES); i++)
            if (!strcmp(ui.code_buf, CODES[i].code)) {
                ui.codes = (ui.codes & ~(unsigned)(CODE_VS10 | CODE_VS50)) | (unsigned)CODES[i].bit;
                ui.code_msg = CODES[i].says;
            }
        ui.code_msg_t = 150;
        sfx_play_name(strcmp(ui.code_msg, "NO SUCH CODE.") ? "ui_toast" : "ui_error");
    }
}

static void htk_update(void) {
    ui.state_t++;
    ui.t++;
    switch (ui.state) {
    case ST_TITLE: title_update(); break;
    case ST_SELECT: select_update(); break;
    case ST_CODES: codes_update(); break;
    case ST_STORY:
        game_set_pausable(false);
        if (ui.state_t > 600 || (ui.state_t > 30 && (btnp(BTN_A) || btnp(BTN_START)))) {
            input_consume();
            set_state(ST_PLAY);
            go_level(0);
        }
        break;
    case ST_PLAY: play_update(); break;
    case ST_OVER:
        if (ui.state_t > 300 || (ui.state_t > 90 && btnp(BTN_A))) { input_consume(); to_title(); }
        break;
    case ST_ENDING:
        if (ui.state_t > 900 || (ui.state_t > 240 && btnp(BTN_A))) {
            input_consume();
            run_over();
            set_state(ST_CREDITS);
        }
        break;
    case ST_CREDITS:
        if (btn(BTN_A)) ui.state_t += 3;
        if (ui.state_t > 1200) { input_consume(); to_title(); }
        break;
    case ST_VSWIN:
        if (ui.state_t > 400 || (ui.state_t > 90 && btnp(BTN_A))) { input_consume(); to_title(); }
        break;
    }
}

/* ---- drawing the screens around the play ----------------------------------------------------- */

static const uint8_t LOGO_GRAD[] = {C_YELLOW, C_YELLOW, C_AMBER, C_ORANGE, C_ORANGE, C_RED, C_RED};

static void draw_title(void) {
    htk_draw_backdrop(0, ui.t);
    gfx_rect(0, 0, 320, HTK_TOP, C_SKY);
    gfx_rect(0, 160, 320, 20, C_AMBER);
    ui_fancy_center("HAT TRICK", 160, 20, 3, LOGO_GRAD, ARRAY_LEN(LOGO_GRAD), C_INK, C_NAVY);
    text_center("ONE BALL. FORTY PITCHES.", 160, 46, C_NAVY);
    /* the kids with the ball between them */
    int bx = 160 + (int)(sinf(ui.t * 0.05f) * 50), by = 150 - (int)fabsf(sinf(ui.t * 0.1f) * 34);
    htk_draw_kid(KID_TEDDY, 90, 160, (ui.t / 20) % 2 ? 4 : 0, 1, ui.t);
    htk_draw_kid(KID_MAE, 230, 160, (ui.t / 20) % 2 ? 0 : 4, -1, ui.t);
    spr_draw(&htk_spr[SP_BALL], bx - 3, by - 3, 0);
    static const char *const ITEMS[] = {"1 PLAYER", "2 PLAYERS", "CODES", "2P VERSUS"};
    ui_panel(110, 64, 100, 14 + title_items() * 12, C_INK, C_WHITE);
    for (int i = 0; i < title_items(); i++) {
        bool off = (i == 1 || i == 3) && vita_single();
        text_center(ITEMS[i], 160, 70 + i * 12, off ? C_SLATE : ui.sel == i ? C_YELLOW : C_WHITE);
        if (ui.sel == i) ui_cursor(118, 70 + i * 12, ui.t);
    }
    char buf[48];
    snprintf(buf, sizeof buf, "TOP %06u", (unsigned)sv.top);
    text_center(buf, 160, 168, C_INK);
    if (ui.code_msg_t > 0 && ui.code_msg) text_center_shadow(ui.code_msg, 160, 126, C_YELLOW, C_INK);
    if (codes_on()) tiny_center("CODE ON: NO SAVING, NO GOALS", 160, 134, C_RED);
}

static void draw_select(void) {
    htk_draw_backdrop(0, ui.t);
    gfx_rect(0, 160, 320, 20, C_AMBER);
    text_center_shadow("WHO'S PLAYING?", 160, 20, C_WHITE, C_INK);
    for (int k = 0; k < 2; k++) {
        int cx = k ? 220 : 100;
        bool on = ui.kid_sel == k;
        ui_panel(cx - 40, 40, 80, 100, on ? C_NAVY : C_INK, on ? C_YELLOW : C_SLATE);
        uint8_t map[PAL_COUNT];
        htk_kid_remap(k, map);
        const Sprite *s = &htk_spr[on && (ui.t / 16) % 2 ? SP_KID_KICK : SP_KID_STAND];
        for (int sy = 0; sy < s->h; sy++)
            for (int sx = 0; sx < s->w; sx++) {
                uint8_t c = s->px[sy * s->w + (k ? s->w - 1 - sx : sx)];
                if (c != TRANSPARENT) gfx_rect(cx - 21 + sx * 3, 54 + sy * 3, 3, 3, map[c]);
            }
        text_center(HTK_KID_NAME[k], cx, 118, on ? C_YELLOW : C_GREY);
        tiny_center(k ? "STARTS ON THE RIGHT" : "STARTS ON THE LEFT", cx, 130, C_LIGHT);
    }
    text_center(GLYPH_A " PLAY   " GLYPH_B " BACK", 160, 166, C_INK);
}

static void draw_codes(void) {
    gfx_cls(C_NAVY);
    text_center_shadow("CODES", 160, 30, C_YELLOW, C_INK);
    for (int i = 0; i < 8; i++) {
        int x = 160 - 8 * 12 / 2 + i * 12 + (i >= 4 ? 4 : 0);
        char c[2] = {ui.code_buf[i], 0};
        ui_panel(x - 1, 68, 11, 14, i == ui.code_pos ? C_BLUE : C_INK, i == ui.code_pos ? C_YELLOW : C_SLATE);
        text_center(c, x + 5, 72, C_WHITE);
    }
    text_center("-", 160, 72, C_GREY);
    tiny_center("UP/DOWN: LETTER   LEFT/RIGHT: PLACE", 160, 100, C_LIGHT);
    tiny_center(GLYPH_A " ENTER   " GLYPH_B " BACK", 160, 110, C_LIGHT);
    if (ui.code_msg_t > 0 && ui.code_msg) text_center(ui.code_msg, 160, 130, C_YELLOW);
}

static void draw_text_page(const char *s, int y, int col) {
    char lines[12][UI_WRAP_LEN];
    int n = ui_wrap(s, 260, false, lines, 12);
    for (int i = 0; i < n && i < 12; i++) text_center(lines[i], 160, y + i * 11, col);
}

static void draw_story(void) {
    gfx_cls(C_NAVY);
    gfx_rect(0, 150, 320, 30, C_FOREST);
    for (int x = 0; x < 320; x += 32) gfx_rect(x, 150, 16, 30, C_JADE);
    gfx_hline(0, 319, 150, C_WHITE);
    htk_draw_kid(KID_TEDDY, 120, 166, (ui.t / 20) % 2 ? 4 : 0, 1, ui.t);
    htk_draw_kid(KID_MAE, 200, 166, (ui.t / 20) % 2 ? 0 : 4, -1, ui.t);
    spr_draw(&htk_spr[SP_BALL], 157 + (int)(sinf(ui.t * 0.05f) * 20), 158 - (int)fabsf(sinf(ui.t * 0.1f) * 16), 0);
    text_center_shadow("SATURDAY MORNING", 160, 16, C_YELLOW, C_INK);
    draw_text_page("EVERY PITCH IN TOWN HAS GONE WILD. THE BALLS, THE PINS, THE FLOATS AND THE GLOVES HAVE "
                   "COME TO LIFE, AND ONLY A KICKED BALL STOPS THEM. TEDDY AND MAE HAVE ONE BALL BETWEEN THEM. "
                   "CLEAR EVERY PITCH, EAT WHAT FALLS, AND MIND THE CLOCK: THE TIMEKEEPER COMES OUT IN EXTRA TIME.",
                   36, C_WHITE);
    if ((ui.state_t / 20) % 2) text_center(GLYPH_A, 160, 136, C_WHITE);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    text_center_shadow("FULL TIME", 160, 60, C_RED, C_NIGHT);
    char buf[48];
    snprintf(buf, sizeof buf, "SCORE %06u", (unsigned)run_best_score());
    text_center(buf, 160, 84, C_WHITE);
    snprintf(buf, sizeof buf, "REACHED %d-%d", (ui.best_level_run - 1) / HTK_PER_WORLD + 1, (ui.best_level_run - 1) % HTK_PER_WORLD + 1);
    if (ui.best_level_run > 0) text_center(buf, 160, 96, C_GREY);
    text_center("BACK TO 1-1", 160, 120, C_SLATE);
}

static void draw_ending(void) {
    htk_draw_backdrop(3, ui.t);
    gfx_darken_rect(0, 0, 320, 180, 2);
    text_center_shadow("THE LAST WHISTLE", 160, 14, C_YELLOW, C_INK);
    draw_text_page(ui.cherry
                       ? "THE BIG STADIUM FALLS QUIET. ONE BY ONE THE PITCHES OF TOWN OPEN AGAIN AND THE "
                         "NEIGHBOURHOOD COMES OUT TO PLAY. TEDDY AND MAE ARE MADE CAPTAINS OF THE HAT TRICK CLUB, "
                         "AND THE CLUB HANGS A GOLDEN BOOT OVER ITS DOOR."
                       : "THE BIG STADIUM FALLS QUIET. ONE BY ONE THE PITCHES OF TOWN OPEN AGAIN AND THE "
                         "NEIGHBOURHOOD COMES OUT TO PLAY. TEDDY AND MAE ARE MADE CAPTAINS OF THE HAT TRICK CLUB.",
                   34, C_WHITE);
    int j = (int)fabsf(sinf(ui.t * 0.1f) * 8);
    htk_draw_kid(KID_TEDDY, 140, 160 - j, 6, 1, ui.t);
    htk_draw_kid(KID_MAE, 180, 160 - (8 - j), 6, -1, ui.t);
    char buf[48];
    snprintf(buf, sizeof buf, "SCORE %06u", (unsigned)run_best_score());
    text_center(buf, 160, 166, C_YELLOW);
}

static void draw_credits(void) {
    gfx_cls(C_NAVY);
    static const char *const LINES[] = {
        "HAT TRICK", "", "A BEAMDOWN SOFTWORKS GAME", "1984", "", "TEDDY AND MAE", "THE HAT TRICK CLUB",
        "AND EVERY CREATURE ON EVERY PITCH", "", "A TRIBUTE TO KICK CLUB", "UFO 50 #11", "", "THANK YOU FOR PLAYING",
    };
    int y = 190 - ui.state_t / 3;
    for (int i = 0; i < ARRAY_LEN(LINES); i++) text_center(LINES[i], 160, y + i * 14, i == 0 ? C_YELLOW : C_WHITE);
}

static void draw_vswin(void) {
    htk_draw_backdrop(3, ui.t);
    gfx_darken_rect(0, 0, 320, 180, 2);
    int w = htk.vs_score[0] >= htk.vs_target ? 0 : 1;
    char buf[48];
    snprintf(buf, sizeof buf, "%s WINS!", HTK_KID_NAME[htk.pl[w].ch]);
    text_center_shadow(buf, 160, 50, C_YELLOW, C_INK);
    snprintf(buf, sizeof buf, "%d - %d", htk.vs_score[0], htk.vs_score[1]);
    text_center(buf, 160, 70, C_WHITE);
    htk_draw_kid(htk.pl[w].ch, 160, 140, 6, 1, ui.t);
}

static void htk_draw(void) {
    switch (ui.state) {
    case ST_TITLE: draw_title(); break;
    case ST_SELECT: draw_select(); break;
    case ST_CODES: draw_codes(); break;
    case ST_STORY: draw_story(); break;
    case ST_PLAY:
        htk_hud_top = sv.top;
        htk_draw_play(&htk);
        break;
    case ST_OVER: draw_over(); break;
    case ST_ENDING: draw_ending(); break;
    case ST_CREDITS: draw_credits(); break;
    default: draw_vswin(); break;
    }
}

/* ------------------------------------------------------------------------ */
/* cartridge interface                                                      */

static void htk_load(void) {
    htk_art_load();
    htk_audio_load();
}

static void htk_start(void) {
    load_save();
    memset(&ui, 0, sizeof ui);
    memset(&htk, 0, sizeof htk);
    htk_bot_reset();
    to_title();
}

static void htk_quit(void) {
    input_set_versus(false);
    if (ui.state == ST_PLAY && htk.mode != MODE_VS) run_over();
    else save_now();
}

static int count_foes(int kind) {
    int n = 0;
    for (int i = 0; i < HTK_MAX_FOES; i++) n += htk.foe[i].alive && (kind < 0 || htk.foe[i].kind == kind);
    return n;
}

static int first_foe(int kind) {
    for (int i = 0; i < HTK_MAX_FOES; i++)
        if (htk.foe[i].alive && htk.foe[i].kind == kind) return i;
    return -1;
}

static int htk_query(const char *key, int *out) {
    const HtkBall *b = &htk.ball;
    if (!strcmp(key, "bot")) {
        uint16_t m = ui.bot1 || !ui.bot2 ? htk_bot_buttons(&htk, 0) : 0;
        if (ui.bot2) m |= (uint16_t)(htk_bot_buttons(&htk, 1) << BTN_P2_SHIFT);
        *out = m;
        return 1;
    }
    if (!strcmp(key, "nav_ball")) { *out = htk_bot_nav(&htk, 0, 0); return 1; }
    if (!strcmp(key, "nav_foe")) { *out = htk_bot_nav(&htk, 0, 1); return 1; }
    if (!strcmp(key, "state")) { *out = ui.state; return 1; }
    if (!strcmp(key, "sub")) { *out = htk.sub; return 1; }
    if (!strcmp(key, "level")) { *out = htk.level + 1; return 1; }
    if (!strcmp(key, "world")) { *out = htk.level / HTK_PER_WORLD + 1; return 1; }
    if (!strcmp(key, "mode")) { *out = htk.mode; return 1; }
    if (!strcmp(key, "sel")) { *out = ui.sel; return 1; }
    if (!strcmp(key, "kid_sel")) { *out = ui.kid_sel; return 1; }
    if (!strcmp(key, "codes")) { *out = (int)ui.codes; return 1; }
    if (!strcmp(key, "clock")) { *out = htk.clock; return 1; }
    if (!strcmp(key, "counts")) { *out = htk_counts_left(&htk); return 1; }
    if (!strcmp(key, "overtime")) { *out = htk.overtime; return 1; }
    if (!strcmp(key, "combo")) { *out = htk.combo; return 1; }
    if (!strcmp(key, "bonus")) { *out = htk.bonus; return 1; }
    if (!strcmp(key, "kills")) { *out = htk.kills; return 1; }
    if (!strcmp(key, "lit")) { *out = b->lit; return 1; }
    if (!strcmp(key, "carrier")) { *out = b->carrier; return 1; }
    if (!strcmp(key, "ball_x")) { *out = (int)lroundf(b->x); return 1; }
    if (!strcmp(key, "ball_y")) { *out = (int)lroundf(b->y); return 1; }
    if (!strcmp(key, "ball_vx100")) { *out = (int)lroundf(b->vx * 100); return 1; }
    if (!strcmp(key, "ball_vy100")) { *out = (int)lroundf(b->vy * 100); return 1; }
    if (!strcmp(key, "ball_speed100")) { *out = (int)lroundf(htk_ball_speed(&htk) * 100); return 1; }
    if (!strcmp(key, "ball_ground")) { *out = b->ground; return 1; }
    if (!strcmp(key, "ball_bounces")) { *out = b->bounces; return 1; }
    if (!strcmp(key, "ball_rest")) { *out = b->rest_t; return 1; }
    if (!strcmp(key, "ball_hold")) { *out = b->hold_t; return 1; }
    if (!strcmp(key, "ball_power")) { *out = b->power_t > 0; return 1; }
    if (!strcmp(key, "ball_last")) { *out = b->last; return 1; }
    if (!strncmp(key, "p", 1) && (key[1] == '0' || key[1] == '1') && key[2] == '_') {
        const HtkPlayer *p = &htk.pl[key[1] - '0'];
        const char *k = key + 3;
        if (!strcmp(k, "x")) { *out = (int)lroundf(p->x); return 1; }
        if (!strcmp(k, "y")) { *out = (int)lroundf(p->y); return 1; }
        if (!strcmp(k, "x10")) { *out = (int)lroundf(p->x * 10); return 1; }
        if (!strcmp(k, "y10")) { *out = (int)lroundf(p->y * 10); return 1; }
        if (!strcmp(k, "vx100")) { *out = (int)lroundf(p->vx * 100); return 1; }
        if (!strcmp(k, "vy100")) { *out = (int)lroundf(p->vy * 100); return 1; }
        if (!strcmp(k, "ground")) { *out = p->ground; return 1; }
        if (!strcmp(k, "alive")) { *out = p->alive; return 1; }
        if (!strcmp(k, "out")) { *out = p->out; return 1; }
        if (!strcmp(k, "on")) { *out = p->on; return 1; }
        if (!strcmp(k, "spare")) { *out = p->spare; return 1; }
        if (!strcmp(k, "lives")) { *out = p->out ? 0 : p->spare + 1; return 1; }
        if (!strcmp(k, "score")) { *out = (int)p->score; return 1; }
        if (!strcmp(k, "slide")) { *out = p->slide_t; return 1; }
        if (!strcmp(k, "head")) { *out = p->head_t; return 1; }
        if (!strcmp(k, "crouch")) { *out = p->crouch; return 1; }
        if (!strcmp(k, "charge")) { *out = p->charge; return 1; }
        if (!strcmp(k, "inv")) { *out = p->inv; return 1; }
        if (!strcmp(k, "ch")) { *out = p->ch; return 1; }
        if (!strcmp(k, "facing")) { *out = p->facing; return 1; }
        if (!strcmp(k, "desserts")) { *out = p->desserts; return 1; }
        if (!strcmp(k, "start_x")) { *out = p->start_x; return 1; }
        if (!strcmp(k, "ext")) { *out = p->next_ext; return 1; }
        if (!strcmp(k, "hit_by")) { *out = p->hit_by; return 1; }
        if (!strcmp(k, "deaths")) { *out = p->deaths; return 1; }
        return 0;
    }
    if (!strcmp(key, "foes")) { *out = htk_foes_left(&htk); return 1; }
    if (!strncmp(key, "foes_", 5)) { *out = count_foes(atoi(key + 5)); return 1; }
    if (!strcmp(key, "timekeeper")) { *out = count_foes(FK_TIMEKEEPER); return 1; }
    if (!strncmp(key, "foe0_", 5)) {
        const HtkFoe *f = &htk.foe[0];
        if (!strcmp(key + 5, "x")) { *out = (int)lroundf(f->x); return 1; }
        if (!strcmp(key + 5, "y")) { *out = (int)lroundf(f->y); return 1; }
        if (!strcmp(key + 5, "alive")) { *out = f->alive; return 1; }
        if (!strcmp(key + 5, "state")) { *out = f->state; return 1; }
        if (!strcmp(key + 5, "dir")) { *out = f->dir; return 1; }
        if (!strcmp(key + 5, "vdir")) { *out = f->vdir; return 1; }
        if (!strcmp(key + 5, "r")) { *out = (int)lroundf(sqrtf((f->x - f->ax) * (f->x - f->ax) + (f->y - f->ay) * (f->y - f->ay))); return 1; }
        return 0;
    }
    if (!strncmp(key, "fx_", 3)) { int i = first_foe(atoi(key + 3)); *out = i >= 0 ? (int)lroundf(htk.foe[i].x) : -1; return 1; }
    if (!strncmp(key, "fy_", 3)) { int i = first_foe(atoi(key + 3)); *out = i >= 0 ? (int)lroundf(htk.foe[i].y) : -1; return 1; }
    if (!strncmp(key, "hp_", 3)) { int i = first_foe(atoi(key + 3)); *out = i >= 0 ? htk.foe[i].hp : -1; return 1; }
    if (!strncmp(key, "away_", 5)) { int i = first_foe(atoi(key + 5)); *out = i >= 0 ? htk.foe[i].away : -1; return 1; }
    if (!strncmp(key, "shot0_", 6)) {
        const HtkShot *sh = NULL;
        for (int i = 0; i < HTK_MAX_SHOTS && !sh; i++) if (htk.shot[i].alive) sh = &htk.shot[i];
        const char *k = key + 6;
        if (!sh) { *out = -999; return 1; }
        if (!strcmp(k, "x")) { *out = (int)lroundf(sh->x); return 1; }
        if (!strcmp(k, "y")) { *out = (int)lroundf(sh->y); return 1; }
        if (!strcmp(k, "vx100")) { *out = (int)lroundf(sh->vx * 100); return 1; }
        if (!strcmp(k, "vy100")) { *out = (int)lroundf(sh->vy * 100); return 1; }
        if (!strcmp(k, "bounces")) { *out = sh->bounces; return 1; }
        if (!strcmp(k, "kind")) { *out = sh->kind; return 1; }
        return 0;
    }
    if (!strcmp(key, "shots")) { int n = 0; for (int i = 0; i < HTK_MAX_SHOTS; i++) n += htk.shot[i].alive; *out = n; return 1; }
    if (!strncmp(key, "shots_", 6)) { int k = atoi(key + 6), n = 0; for (int i = 0; i < HTK_MAX_SHOTS; i++) n += htk.shot[i].alive && htk.shot[i].kind == k; *out = n; return 1; }
    if (!strcmp(key, "falling")) { int n = 0; for (int i = 0; i < HTK_MAX_ITEMS; i++) n += htk.item[i].alive && htk.item[i].falling; *out = n; return 1; }
    if (!strcmp(key, "food")) { int n = 0; for (int i = 0; i < HTK_MAX_ITEMS; i++) n += htk.item[i].alive && !htk.item[i].falling; *out = n; return 1; }
    if (!strcmp(key, "food_x") || !strcmp(key, "food_y")) {
        *out = -1;
        for (int i = 0; i < HTK_MAX_ITEMS; i++)
            if (htk.item[i].alive && !htk.item[i].falling) { *out = (int)lroundf(key[5] == 'x' ? htk.item[i].x : htk.item[i].y); break; }
        return 1;
    }
    if (!strcmp(key, "food_value")) {
        int n = 0;
        for (int i = 0; i < HTK_MAX_ITEMS; i++) if (htk.item[i].alive && !htk.item[i].falling) n += HTK_ITEM_VALUE[htk.item[i].item];
        *out = n;
        return 1;
    }
    if (!strncmp(key, "food_item", 9)) {
        /* how many foods of that kind lie about */
        int k = atoi(key + 9), n = 0;
        for (int i = 0; i < HTK_MAX_ITEMS; i++) n += htk.item[i].alive && !htk.item[i].falling && htk.item[i].item == k;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "dessert_found")) { *out = htk.dessert_found; return 1; }
    if (!strcmp(key, "dessert_spot")) { *out = htk.dessert_tx >= 0; return 1; }
    if (!strcmp(key, "vs_score0")) { *out = htk.vs_score[0]; return 1; }
    if (!strcmp(key, "vs_score1")) { *out = htk.vs_score[1]; return 1; }
    if (!strcmp(key, "vs_target")) { *out = htk.vs_target; return 1; }
    if (!strcmp(key, "fair_pct")) { *out = fair_pct; return 1; }
    if (!strcmp(key, "fair_idle")) { *out = fair_idle; return 1; }
    if (!strcmp(key, "won")) { *out = ui.won; return 1; }
    if (!strcmp(key, "cherry")) { *out = ui.cherry; return 1; }
    if (!strcmp(key, "best_level_run")) { *out = ui.best_level_run; return 1; }
    if (!strcmp(key, "save_top")) { *out = (int)sv.top; return 1; }
    if (!strcmp(key, "save_best_level")) { *out = sv.best_level; return 1; }
    if (!strcmp(key, "save_runs")) { *out = sv.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = sv.wins; return 1; }
    if (!strcmp(key, "save_cherries")) { *out = sv.cherries; return 1; }
    if (!strcmp(key, "save_desserts")) { *out = sv.most_desserts; return 1; }
    /* the screens as written */
    if (!strncmp(key, "lvl_", 4)) {
        /* lvl_N_what: screen N (1..40) */
        int n = atoi(key + 4) - 1;
        const char *w = strchr(key + 4, '_');
        if (n < 0 || n >= HTK_LEVELS || !w) return 0;
        w++;
        const HtkLevelDef *d = &HTK_LEVEL[n];
        int cnt[FK_COUNT] = {0}, world = n / HTK_PER_WORLD, starts = 0, balls = 0, sym = 1, wrapok = 1, spots = 0;
        for (int r = 0; r < HTK_ROWS; r++) {
            if ((int)strlen(d->rows[r]) != HTK_HALF) sym = 0;
            for (int c = 0; c < HTK_HALF && d->rows[r][c]; c++) {
                char ch = d->rows[r][c];
                if (ch >= 'a' && ch <= 'c') cnt[world * 3 + ch - 'a'] += 2;
                if (ch >= 'A' && ch <= 'C') cnt[world * 3 + ch - 'A'] += 2;
                if (ch == 'P') starts++;
                if (ch == 'O') balls++;
                if (ch == '*') spots++;
            }
        }
        for (int c = 0; c < HTK_HALF; c++) {
            bool top_open = d->rows[0][c] != '#', bot_open = d->rows[HTK_ROWS - 1][c] != '#';
            if (top_open != bot_open) wrapok = 0;
        }
        if (!strcmp(w, "starts")) { *out = starts; return 1; }
        if (!strcmp(w, "balls")) { *out = balls; return 1; }
        if (!strcmp(w, "half")) { *out = sym; return 1; }
        if (!strcmp(w, "wrapok")) { *out = wrapok; return 1; }
        if (!strcmp(w, "spots")) { *out = spots; return 1; }
        if (!strcmp(w, "kinds")) { int k = 0; for (int i = 0; i < FK_REGULAR; i++) k += cnt[i] > 0; *out = k; return 1; }
        if (!strcmp(w, "foes")) { int k = 0; for (int i = 0; i < FK_REGULAR; i++) k += cnt[i]; *out = k; return 1; }
        if (!strncmp(w, "kind", 4)) { int k = atoi(w + 4); *out = k >= 0 && k < FK_COUNT ? cnt[k] : 0; return 1; }
        return 0;
    }
    if (!strcmp(key, "unreach") || !strcmp(key, "dead_ledges")) {
        int u = 0, d = 0;
        htk_reach_check(&htk, &u, &d);
        *out = key[0] == 'u' ? u : d;
        return 1;
    }
    if (!strcmp(key, "sym")) {
        /* the screen in play is the same on both sides: tiles and creatures */
        int ok = 1;
        for (int r = 0; r < HTK_ROWS; r++)
            for (int c = 0; c < HTK_HALF; c++)
                if (htk.tile[r][c] != htk.tile[r][HTK_COLS - 1 - c]) ok = 0;
        for (int i = 0; i < HTK_MAX_FOES; i++) {
            const HtkFoe *f = &htk.foe[i];
            if (!f->alive || f->kind >= FK_KINGSPIKER) continue;
            bool twin = false;
            for (int j = 0; j < HTK_MAX_FOES; j++) {
                const HtkFoe *e = &htk.foe[j];
                if (e->alive && e->kind == f->kind && fabsf(e->x - (HTK_W - f->x)) < 0.6f && fabsf(e->y - f->y) < 0.6f) twin = true;
            }
            if (!twin) ok = 0;
        }
        *out = ok;
        return 1;
    }
    if (!strcmp(key, "morse_ok")) {
        /* read the floats back as Morse and check they spell the message */
        static const char *const M[26] = {".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", "-.-",
                                          ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", "..-", "...-",
                                          ".--", "-..-", "-.--", "--.."};
        int len[64], gap[64], n = htk_morse_layout(len, gap, 64);
        char word[32] = {0}, sym[8] = {0};
        int wn = 0, sn = 0;
        for (int i = 0; i <= n; i++) {
            if (i == n || (i > 0 && gap[i] >= 6)) {
                for (int L = 0; L < 26; L++)
                    if (!strcmp(M[L], sym) && wn < 30) word[wn++] = (char)('A' + L);
                sn = 0;
                memset(sym, 0, sizeof sym);
                if (i < n && gap[i] >= 12 && wn < 30) word[wn++] = ' ';
            }
            if (i < n && sn < 7) sym[sn++] = len[i] > 3 ? '-' : '.';
        }
        *out = !strcmp(word, HTK_MORSE_MSG) && !strcmp(word, "GOOD GAME");
        return 1;
    }
    return 0;
}

static int htk_cheat(const char *cmd) {
    int a, b, c, d;
    float x, y, vx, vy;
    if (sscanf(cmd, "run %d %d", &a, &b) == 2) {
        /* start a run: mode (0 1P, 1 co-op, 2 versus), kid */
        new_run(iclamp(a, 0, 2), iclamp(b, 0, 1));
        if (ui.state == ST_STORY) { set_state(ST_PLAY); go_level(0); }
        return 1;
    }
    if (sscanf(cmd, "level %d", &a) == 1) {
        if (ui.state != ST_PLAY) { new_run(MODE_1P, KID_TEDDY); set_state(ST_PLAY); }
        go_level(iclamp(a, 1, HTK_LEVELS) - 1);
        return 1;
    }
    if (!strcmp(cmd, "skipintro")) { htk.sub = LS_PLAY; htk.sub_t = 0; return 1; }
    if (!strcmp(cmd, "god")) { htk.god = 1; return 1; }
    if (!strcmp(cmd, "mortal")) { htk.god = 0; return 1; }
    if (!strcmp(cmd, "still")) { htk.still = 1; return 1; }
    if (!strcmp(cmd, "unstill")) { htk.still = 0; return 1; }
    if (!strcmp(cmd, "nofoes")) { for (int i = 0; i < HTK_MAX_FOES; i++) htk.foe[i].alive = 0; return 1; }
    if (!strcmp(cmd, "noshots")) { memset(htk.shot, 0, sizeof htk.shot); return 1; }
    if (sscanf(cmd, "pos %d %f %f", &a, &x, &y) == 3) {
        HtkPlayer *p = &htk.pl[iclamp(a, 0, 1)];
        p->x = x; p->y = y; p->vx = p->vy = 0;
        return 1;
    }
    if (sscanf(cmd, "start %d %f %f", &a, &x, &y) == 3) { htk.pl[iclamp(a, 0, 1)].start_x = (int16_t)x; htk.pl[iclamp(a, 0, 1)].start_y = (int16_t)y; return 1; }
    if (sscanf(cmd, "ball %f %f %f %f", &x, &y, &vx, &vy) == 4) {
        HtkBall *bl = &htk.ball;
        bl->carrier = -1;
        bl->x = x; bl->y = y; bl->vx = vx; bl->vy = vy;
        bl->ground = 0;
        bl->bounces = bl->rest_t = 0;
        return 1;
    }
    if (sscanf(cmd, "carry %d", &a) == 1) {
        HtkBall *bl = &htk.ball;
        bl->carrier = (int16_t)iclamp(a, 0, 1);
        bl->last = bl->carrier;
        bl->lit = 1;
        bl->hold_t = bl->bounces = bl->rest_t = 0;
        return 1;
    }
    if (!strcmp(cmd, "light")) { htk.ball.lit = 1; return 1; }
    if (!strcmp(cmd, "dark")) { htk.ball.lit = 0; htk.combo = 0; return 1; }
    if (sscanf(cmd, "bounces %d", &a) == 1) { htk.ball.bounces = (int16_t)a; return 1; }
    if (sscanf(cmd, "combo %d", &a) == 1) { htk.combo = a; return 1; }
    if (sscanf(cmd, "foe %d %f %f %d", &a, &x, &y, &b) == 4) { htk_spawn_foe(&htk, iclamp(a, 0, FK_COUNT - 1), x, y, b, 0); return 1; }
    if (sscanf(cmd, "foealt %d %f %f %d", &a, &x, &y, &b) == 4) { htk_spawn_foe(&htk, iclamp(a, 0, FK_COUNT - 1), x, y, b, 1); return 1; }
    if (sscanf(cmd, "shot %d %f %f %f %f", &a, &x, &y, &vx, &vy) == 5) { htk_add_shot(&htk, a, x, y, vx, vy); return 1; }
    if (sscanf(cmd, "body %d %f %f %d", &a, &x, &y, &b) == 4) { htk_spawn_body(&htk, x, y, a, b, 0); return 1; }
    if (sscanf(cmd, "clock %d", &a) == 1) { htk.clock = a; return 1; }
    if (sscanf(cmd, "score %d %d", &a, &b) == 2) { htk.pl[iclamp(a, 0, 1)].score = (uint32_t)b; return 1; }
    if (sscanf(cmd, "addscore %d %d", &a, &b) == 2) { htk_add_score(&htk, iclamp(a, 0, 1), b); return 1; }
    if (sscanf(cmd, "spare %d %d", &a, &b) == 2) { htk.pl[iclamp(a, 0, 1)].spare = (int16_t)b; return 1; }
    if (sscanf(cmd, "kill %d", &a) == 1) { htk_kill_player(&htk, iclamp(a, 0, 1)); return 1; }
    if (sscanf(cmd, "bosshp %d", &a) == 1) {
        for (int i = 0; i < HTK_MAX_FOES; i++)
            if (htk.foe[i].alive && htk.foe[i].kind >= FK_KINGSPIKER && htk.foe[i].kind != FK_TIMEKEEPER) htk.foe[i].hp = (int16_t)a;
        return 1;
    }
    if (sscanf(cmd, "vsscore %d %d", &a, &b) == 2) { htk.vs_score[0] = a; htk.vs_score[1] = b; return 1; }
    if (sscanf(cmd, "foehp %d %d", &a, &b) == 2) {
        for (int i = 0; i < HTK_MAX_FOES; i++)
            if (htk.foe[i].alive && htk.foe[i].kind == a) htk.foe[i].hp = (int16_t)b;
        return 1;
    }
    if (sscanf(cmd, "bosspos %f %f", &x, &y) == 2) {
        for (int i = 0; i < HTK_MAX_FOES; i++)
            if (htk.foe[i].alive && htk.foe[i].kind >= FK_KINGSPIKER && htk.foe[i].kind != FK_TIMEKEEPER && htk.foe[i].kind != FK_TOWER) {
                htk.foe[i].x = x;
                htk.foe[i].y = y;
                htk.foe[i].vx = htk.foe[i].vy = 0;
                break;
            }
        return 1;
    }
    if (sscanf(cmd, "bot %d %d", &a, &b) == 2) { ui.bot1 = a != 0; ui.bot2 = b != 0; return 1; }
    if (sscanf(cmd, "botwidth %d", &a) == 1) { htk_bot_width = iclamp(a, 1, 200); return 1; }
    if (sscanf(cmd, "fair %d %d %d", &a, &b, &c) == 3) {
        /* fair LEVEL TRIES WIDTH: how forgiving screen LEVEL is (see htk_fair_runs) */
        int worst = 0, ok = htk_fair_runs(iclamp(a, 1, HTK_LEVELS) - 1, b, c, &worst);
        d = b > 0 ? ok * 100 / b : 0;
        printf("  # fair %d-%d: %d of %d tries keep their life (%d%%); standing still lasts %d frames at worst\n",
               (a - 1) / HTK_PER_WORLD + 1, (a - 1) % HTK_PER_WORLD + 1, ok, b, d, worst);
        for (int k = 0; k < 128; k++)
            if (htk_fair_killer[k])
                printf("  #   lost to %s %d: %d\n", k >= 100 ? "shot" : "creature", k >= 100 ? k - 100 : k, htk_fair_killer[k]);
        fair_pct = d;
        fair_idle = worst;
        return 1;
    }
    if (!strcmp(cmd, "reachlog")) { htk_reach_log ^= 1; return 1; }
    if (!strcmp(cmd, "win")) { htk.level = HTK_LEVELS - 1; start_ending(); return 1; }
    if (!strcmp(cmd, "over")) {
        for (int k = 0; k < 2; k++) { htk.pl[k].spare = 0; htk.god = 0; htk.pl[k].inv = 0; htk_kill_player(&htk, k); }
        return 1;
    }
    return 0;
}

const GameDef GAME_HATTRICK = {
    "hattrick",
    "HAT TRICK",
    "1984",
    "ARCADE PLATFORMER",
    "ONLY A KICKED BALL STOPS THEM. KEEP IT LIT AND THE FOOD GETS BETTER.",
    {"BEAT LUCKY LANES, THE SECOND WORLD", "BEAT ALL FOUR WORLDS", "WIN WITH 150,000 POINTS OR MORE"},
    "D-PAD\tRUN, AIM THE KICK\n"
    GLYPH_A "\tJUMP, ALWAYS AS HIGH\n"
    "TAP " GLYPH_B "\tKICK THE BALL\n"
    "HOLD " GLYPH_B "\tDRIVEN SHOT, LET GO\n"
    GLYPH_B " NO BALL\tSLIDE, OR HEADER\n"
    "DOWN\tCROUCH\n"
    "START\tPAUSE\n"
    "\n"
    "WALK INTO THE BALL TO PICK IT UP.",
    C_JADE, C_YELLOW,
    htk_load, htk_start, htk_update, htk_draw, htk_quit, htk_draw_label, htk_query, htk_cheat,
    "KICK CLUB", 11,
    NULL,
};
