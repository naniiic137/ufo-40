/* TURNIP TRUCK - Zib's week behind the wheel of Granny Root's turnip truck.
 * Cartridge 23 of UFO 40, a tribute to Onion Delivery (UFO 50 #23). See
 * docs/games/23-turnip-truck.md. This file: the title (with its demo), the
 * codes, the morning newscast, the week of seven workdays and its shared
 * tries, the screens between the days, the ending, the save and the test
 * hooks. A workday itself is in tnp_drive.c and tnp_events.c. */
#include "tnp.h"

#define SAVE_MAGIC 0x544E5001u
#define DEMO_IDLE 720         /* frames on the title before the demo drives */
#define DEMO_LEN 2400

enum { ST_TITLE, ST_CODES, ST_DEMO, ST_NEWS, ST_PLAY, ST_DAYDONE, ST_FAILED, ST_FIRED, ST_ENDING, ST_CREDITS };

/* ---- the codes (UFO 40 has no terminal: the title has a CODES page) ------------------ */
enum { CODE_PRACTICE = 1, CODE_FLIP = 2 };
static const struct { const char *code; int bit; const char *says; } CODES[] = {
    {"JOYRIDES", CODE_PRACTICE, "JOYRIDE: NO CLOCK, NO DAMAGE, NO END."},
    {"BACKWARD", CODE_FLIP, "REVERSE STEERING TURNED ROUND."},
};
#define CODE_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

/* The cartridge save: records only. Like its original, a week is played in
 * one sitting and never saved half-way. */
typedef struct {
    uint32_t magic;
    uint16_t daily_best;  /* the most deliveries in one day */
    uint16_t best_week;   /* the most in a finished week */
    uint16_t weeks, cherries, starts, best_day_reached;
    uint32_t delivered;   /* every delivery in every cleared day */
} TnpSave;

static TnpSave sv;
static struct {
    int state, state_t, t, idle;
    int sel;
    unsigned codes;
    char code_buf[9];
    int code_pos, code_msg_t;
    const char *code_msg;
    /* the week */
    int day, tries, total, attempt;
    int events[TNP_DAYS];
    int day_count[TNP_DAYS];
    uint64_t week_seed;
    int lot_secret;          /* the next newscast has a word for the fenced lot */
    int fail_reason;
    bool won, cherry, overtime_music;
    int demo_n;
} ui;

static void set_state(int s) {
    ui.state = s;
    ui.state_t = 0;
}
static bool codes_on(void) { return ui.codes != 0; }

/* ---- the save ------------------------------------------------------------------------------ */

static void load_save(void) {
    TnpSave tmp;
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

/* ---- the week ------------------------------------------------------------------------------ */

static void to_title(void) {
    set_state(ST_TITLE);
    ui.idle = 0;
    game_set_pausable(false);
    music_play(TNP_MUS_TITLE);
}

static void new_week(void) {
    ui.week_seed = ((uint64_t)rng_next(&g_rng) << 32) | rng_next(&g_rng);
    Rng r;
    rng_seed(&r, ui.week_seed);
    /* Monday is quiet; the six events fall on the other days in a shuffled order */
    int ev[6] = {EV_BRINE, EV_BEET, EV_MUSH, EV_RADISH, EV_RAIN, EV_MOON};
    for (int i = 5; i > 0; i--) {
        int j = rng_range(&r, 0, i), k = ev[i];
        ev[i] = ev[j];
        ev[j] = k;
    }
    ui.events[0] = EV_NONE;
    for (int i = 0; i < 6; i++) ui.events[i + 1] = ev[i];
    ui.day = 0;
    ui.tries = TNP_TRIES;
    ui.total = 0;
    ui.attempt = 0;
    ui.won = ui.cherry = false;
    memset(ui.day_count, 0, sizeof ui.day_count);
    if (!codes_on() && sv.starts < 65535) { sv.starts++; save_now(); }
}

static void start_news(void) {
    set_state(ST_NEWS);
    game_set_pausable(false);
    music_restart(TNP_MUS_NEWS);
}

static void day_music(void) {
    if (tnp.delivered >= TNP_QUOTA && !tnp.practice) music_play(TNP_MUS_OVERTIME);
    else music_play(tnp.event == EV_RAIN ? TNP_MUS_RAIN : TNP_MUS_DAY);
}

static void start_day(void) {
    bool practice = (ui.codes & CODE_PRACTICE) != 0;
    tnp_day_begin(&tnp, ui.day, ui.events[ui.day], ui.week_seed + (uint64_t)ui.day * 7919u + (uint64_t)ui.attempt * 104729u,
                  practice, (ui.codes & CODE_FLIP) != 0);
    tnp_bot_reset();
    ui.overtime_music = false;
    set_state(ST_PLAY);
    game_set_pausable(true);
    day_music();
}

static void day_cleared(void) {
    int n = tnp.delivered;
    ui.day_count[ui.day] = n;
    ui.total += n;
    if (!codes_on()) {
        if (n > sv.daily_best) sv.daily_best = (uint16_t)n;
        sv.delivered += (uint32_t)n;
        if (ui.day + 1 > sv.best_day_reached) sv.best_day_reached = (uint16_t)(ui.day + 1);
        save_now();
    }
    if (ui.day == 1) award(GOAL_BEACON); /* two days of deliveries done */
    set_state(ST_DAYDONE);
    game_set_pausable(false);
    music_play(TNP_MUS_CLEAR);
}

static void start_ending(void) {
    ui.won = true;
    award(GOAL_SAUCER);
    if (ui.total >= TNP_CHERRY) {
        ui.cherry = true;
        award(GOAL_ALIEN);
    }
    if (!codes_on()) {
        if (sv.weeks < 65535) sv.weeks++;
        if (ui.cherry && sv.cherries < 65535) sv.cherries++;
        if (ui.total > sv.best_week) sv.best_week = (uint16_t)ui.total;
        save_now();
    }
    set_state(ST_ENDING);
    game_set_pausable(false);
    music_play(TNP_MUS_END);
}

static void play_update(void) {
    game_set_pausable(true);
    tnp_step(&tnp, (uint16_t)(input_held() & 0xFF));
    if (tnp.delivered >= TNP_QUOTA && !tnp.practice && !ui.overtime_music) {
        ui.overtime_music = true;
        day_music();
    }
    if (!tnp.practice && tnp.time_f > 0 && tnp.time_f <= 600 && tnp.time_f % 60 == 0 && tnp.tr.state == TS_DRIVE)
        tnp_sfx("tnp_tick");
    if (tnp.ev_clock_out) {
        day_cleared();
        return;
    }
    if (tnp.ev_fail) {
        ui.fail_reason = tnp.fail_reason;
        if (tnp.died_in_lot) ui.lot_secret = 1;
        set_state(ST_FAILED);
        game_set_pausable(false);
        music_play(TNP_MUS_FAIL);
    }
}

/* ---- the screens' logic ------------------------------------------------------------------------ */

static void title_update(void) {
    game_set_pausable(false);
    if (ui.code_msg_t > 0) ui.code_msg_t--;
    if (input_held()) ui.idle = 0;
    else if (++ui.idle >= DEMO_IDLE) {
        /* the demo: the demo player drives a day of the week on its own */
        int ev = ui.demo_n++ % EV_COUNT;
        tnp_day_begin(&tnp, ev == EV_NONE ? 0 : 1, ev, rng_next(&g_rng), 0, 0);
        tnp.god = 1;
        tnp_bot_reset();
        tnp_quiet = true;
        set_state(ST_DEMO);
        music_play(ev == EV_RAIN ? TNP_MUS_RAIN : TNP_MUS_DAY);
        return;
    }
    if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { ui.sel ^= 1; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if (ui.state_t < 10 || !(btnp(BTN_A) || btnp(BTN_START))) return;
    sfx_play_name("ui_ok");
    input_consume();
    if (ui.sel == 0) {
        new_week();
        if (ui.codes & CODE_PRACTICE) start_day();
        else start_news();
    } else {
        memset(ui.code_buf, 'A', 8);
        ui.code_buf[8] = 0;
        ui.code_pos = 0;
        ui.code_msg = NULL;
        ui.code_msg_t = 0;
        set_state(ST_CODES);
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
                ui.codes ^= (unsigned)CODES[i].bit; /* a second time turns it off again */
                ui.code_msg = (ui.codes & (unsigned)CODES[i].bit) ? CODES[i].says : "CODE OFF.";
            }
        ui.code_msg_t = 150;
        sfx_play_name(strcmp(ui.code_msg, "NO SUCH CODE.") ? "ui_toast" : "ui_error");
    }
}

static void tnp_update(void) {
    ui.state_t++;
    ui.t++;
    switch (ui.state) {
    case ST_TITLE: title_update(); break;
    case ST_CODES: codes_update(); break;
    case ST_DEMO:
        game_set_pausable(false);
        tnp_step(&tnp, tnp_bot_buttons(&tnp));
        if (ui.state_t > 5 && (input_held() || ui.state_t > DEMO_LEN || tnp.phase != DP_PLAY)) {
            tnp_quiet = false;
            input_consume();
            to_title();
        }
        break;
    case ST_NEWS:
        game_set_pausable(false);
        if (ui.state_t > 900 || (ui.state_t > 40 && (btnp(BTN_A) || btnp(BTN_START)))) {
            input_consume();
            ui.lot_secret = 0;
            start_day();
        }
        break;
    case ST_PLAY: play_update(); break;
    case ST_DAYDONE:
        if (ui.state_t > 600 || (ui.state_t > 60 && btnp(BTN_A))) {
            input_consume();
            if (ui.day >= TNP_DAYS - 1) start_ending();
            else {
                ui.day++;
                ui.attempt = 0;
                start_news();
            }
        }
        break;
    case ST_FAILED:
        if (ui.state_t > 600 || (ui.state_t > 60 && btnp(BTN_A))) {
            input_consume();
            if (ui.tries > 0) {
                /* a try from the week's pool: the day starts over */
                ui.tries--;
                ui.attempt++;
                start_news();
            } else {
                set_state(ST_FIRED);
                music_play(TNP_MUS_FIRED);
            }
        }
        break;
    case ST_FIRED:
        if (ui.state_t > 480 || (ui.state_t > 90 && btnp(BTN_A))) { input_consume(); to_title(); }
        break;
    case ST_ENDING:
        if (ui.state_t > 1200 || (ui.state_t > 180 && btnp(BTN_A))) {
            input_consume();
            set_state(ST_CREDITS);
        }
        break;
    case ST_CREDITS:
        if (btn(BTN_A)) ui.state_t += 3;
        if (ui.state_t > 1100) { input_consume(); to_title(); }
        break;
    }
}

/* ---- drawing the screens around the driving --------------------------------------------------- */

static const char *const DAY_NAME[TNP_DAYS] = {"MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY", "SUNDAY"};
static const uint8_t LOGO_GRAD[] = {C_WHITE, C_WHITE, C_PINK, C_MAGENTA, C_VIOLET, C_VIOLET, C_PURPLE};

static void draw_text_page(const char *s, int x, int y, int w, int col) {
    char lines[12][UI_WRAP_LEN];
    int n = ui_wrap(s, w, false, lines, 12);
    for (int i = 0; i < n && i < 12; i++) text_draw(lines[i], x, y + i * 10, col);
}

static void draw_city_backdrop(float cx, float cy) {
    tnp_draw_city(NULL, cx, cy, 0, 0, SCREEN_W, SCREEN_H);
}

static void draw_title(void) {
    /* Blipton scrolling by underneath, the truck cruising along the depot's street */
    float cx = tnp_wrap((float)ui.t * 0.6f), cy = 9 * TNP_CELL - 60;
    draw_city_backdrop(cx, cy);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    spr_draw_scaled(&tnp_spr[SP_TRUCK], 136, 106 + (int)(sinf((float)ui.t * 0.1f) * 1.5f), 3, 0);
    ui_fancy_center("TURNIP TRUCK", 160, 18, 3, LOGO_GRAD, ARRAY_LEN(LOGO_GRAD), C_INK, C_PURPLE);
    text_center_shadow("SEVEN DAYS. FIVE DROPS A DAY.", 160, 46, C_YELLOW, C_INK);
    spr_draw_scaled(&tnp_spr[SP_TURNIP], 22, 14, 3, 0);
    spr_draw_scaled(&tnp_spr[SP_TURNIP], 271, 14, 3, SPR_FLIPX);
    static const char *const ITEMS[] = {"START WORK", "CODES"};
    ui_panel(110, 62, 100, 38, C_INK, C_WHITE);
    for (int i = 0; i < 2; i++) {
        const char *s = i == 0 && (ui.codes & CODE_PRACTICE) ? "JOYRIDE" : ITEMS[i];
        text_center(s, 160, 69 + i * 14, ui.sel == i ? C_YELLOW : C_WHITE);
        if (ui.sel == i) ui_cursor(116, 69 + i * 14, ui.t);
    }
    char buf[48];
    snprintf(buf, sizeof buf, "DAILY BEST %d", sv.daily_best);
    ui_panel(108, 150, 104, 16, C_INK, C_DUSK);
    text_center(buf, 160, 154, C_WHITE);
    if (ui.code_msg_t > 0 && ui.code_msg) text_center_shadow(ui.code_msg, 160, 136, C_YELLOW, C_INK);
    else if (codes_on()) tiny_center("CODE ON: NO SAVING, NO GOALS", 160, 140, C_RED);
}

static void draw_codes(void) {
    gfx_cls(C_PURPLE);
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_NIGHT, 6);
    text_center_shadow("CODES", 160, 30, C_YELLOW, C_INK);
    for (int i = 0; i < 8; i++) {
        int x = 160 - 8 * 12 / 2 + i * 12 + (i >= 4 ? 4 : 0);
        char c[2] = {ui.code_buf[i], 0};
        ui_panel(x - 1, 68, 11, 14, i == ui.code_pos ? C_VIOLET : C_INK, i == ui.code_pos ? C_YELLOW : C_SLATE);
        text_center(c, x + 5, 72, C_WHITE);
    }
    text_center("-", 160, 72, C_GREY);
    tiny_center("UP/DOWN: LETTER   LEFT/RIGHT: PLACE", 160, 100, C_LIGHT);
    text_center(GLYPH_A " ENTER   " GLYPH_B " BACK", 160, 110, C_LIGHT);
    if (ui.code_msg_t > 0 && ui.code_msg) text_center(ui.code_msg, 160, 130, C_YELLOW);
    if (ui.codes & CODE_PRACTICE) tiny_center("JOYRIDE ON", 120, 150, C_LIME);
    if (ui.codes & CODE_FLIP) tiny_center("REVERSE FLIPPED", 200, 150, C_LIME);
}

static const char *news_line(int ev) {
    switch (ev) {
    case EV_BRINE:
        return "A MAIN HAS BURST AT THE PICKLE WORKS. BRINE IS SPRAYING FROM THE HYDRANTS AT THE CROSSINGS, AND WAVES "
               "OF IT ARE SWEEPING ACROSS THE ROADS BY THE WATER. HOLD ON TO YOUR WHEEL!";
    case EV_BEET:
        return "A GIANT BEETROOT HAS ESCAPED FROM THE COUNTY SHOW. IT IS CHARGING ROUND TOWN AFTER ANYTHING WITH "
               "WHEELS. KEEP MOVING!";
    case EV_MUSH:
        return "THE MUSHMEN ARE UP EARLY. HORDES OF THEM ARE SHUFFLING THROUGH THE STREETS. THEY WON'T HURT A SOUL, "
               "BUT EVERY ONE YOU RUN OVER SLOWS YOU RIGHT DOWN.";
    case EV_RADISH:
        return "THE RADISH RING ROBBED THE WOBBLY BANK AT DAWN. THE POLICE ARE CHASING THEIR CARS ALL OVER TOWN. "
               "IF THE GANG SPOTS YOU, THEY WILL COME AFTER YOU, GUNS BLAZING.";
    case EV_RAIN:
        return "HEAVY RAIN ALL DAY, FOLKS. PUDDLES ARE APPEARING ON EVERY ROAD IN AN INSTANT, AND HITTING ONE FAST "
               "WILL SPIN YOU RIGHT ROUND. SLOW DOWN!";
    case EV_MOON:
        return "SAUCERS FROM THE MOON ARE OVER BLIPTON! THEY ARE DROPPING BOMBS ON ANYTHING THAT COMES NEAR. STAY ON "
               "THE MOVE!";
    default:
        return "GOOD MORNING, BLIPTON! CLEAR SKIES AND QUIET ROADS. A LOVELY DAY TO START A NEW JOB. DO THE TURNIP "
               "TRUCK'S NEW DRIVER A FAVOUR AND KEEP OUT OF THE WAY.";
    }
}

static void draw_event_picture(int x, int y, int w, int h, int ev) {
    gfx_rect(x, y, w, h, ev == EV_RAIN ? C_NAVY : ev == EV_MOON ? C_NIGHT : C_SKY);
    gfx_rect(x, y + h - 14, w, 14, C_DUSK);
    int cx = x + w / 2, cy = y + h / 2;
    switch (ev) {
    case EV_BRINE:
        gfx_circ(cx, cy + 8, 5, C_SLATE);
        for (int i = 0; i < 12; i++) gfx_rect(cx + (i % 2 ? 1 : -1) * (6 + i * 2), cy + 6 - (i % 3), 3, 2, C_LIME);
        break;
    case EV_BEET:
        gfx_circ(cx, cy + 2, 12, C_WINE);
        gfx_rect(cx - 4, cy - 17, 3, 8, C_JADE);
        gfx_rect(cx + 2, cy - 18, 3, 9, C_LEAF);
        gfx_rect(cx - 6, cy - 1, 4, 3, C_WHITE);
        gfx_rect(cx + 2, cy - 1, 4, 3, C_WHITE);
        break;
    case EV_MUSH:
        for (int i = 0; i < 5; i++) spr_draw(&tnp_spr[SP_MUSH1], x + 8 + i * 12, cy + (i % 2) * 4, 0);
        break;
    case EV_RADISH:
        tnp_draw_rot(&tnp_spr[SP_GANG], (float)cx - 12, (float)cy + 8, 0, NULL, -1);
        tnp_draw_rot(&tnp_spr[SP_POLICE], (float)cx + 14, (float)cy + 8, 0, NULL, -1);
        break;
    case EV_RAIN:
        for (int i = 0; i < 20; i++) gfx_line(x + (i * 13) % w, y + (i * 7 + ui.t * 2) % (h - 14), x + (i * 13) % w - 2, y + (i * 7 + ui.t * 2) % (h - 14) + 5, C_ICE);
        break;
    case EV_MOON:
        gfx_circ(x + w - 12, y + 10, 7, C_CREAM);
        for (int i = 0; i < 3; i++) {
            int sx = x + 14 + i * 18, sy = y + 16 + (i % 2) * 8;
            gfx_rect(sx - 6, sy, 13, 3, C_GREY);
            gfx_rect(sx - 3, sy - 3, 7, 3, C_CYAN);
        }
        break;
    default:
        gfx_circ(cx, cy - 4, 9, C_YELLOW);
        gfx_circb(cx, cy - 4, 12, C_CREAM);
        break;
    }
}

static void draw_news(void) {
    gfx_cls(C_INK);
    /* the television */
    ui_panel(6, 4, 308, 172, C_BROWN, C_TAN);
    gfx_rect(14, 10, 292, 128, C_NAVY);
    gfx_dither(14, 10, 292, 128, C_BLUE, 3);
    tnp_draw_anchor(26, 40, ui.state_t, ui.state_t < 400);
    gfx_rect(18, 88, 64, 10, C_INK);
    tiny_center("GLENDA GLORP", 50, 91, C_WHITE);
    ui_panel(220, 18, 80, 60, C_INK, C_WHITE);
    draw_event_picture(222, 20, 76, 56, ui.events[ui.day]);
    char buf[64];
    snprintf(buf, sizeof buf, "CHANNEL 9 NEWS AT NINE");
    text_draw(buf, 90, 16, C_YELLOW);
    snprintf(buf, sizeof buf, "%s, DAY %d", DAY_NAME[ui.day], ui.day + 1);
    text_draw(buf, 90, 28, C_WHITE);
    if (ui.events[ui.day] != EV_NONE) text_draw(TNP_EVENT_NAME[ui.events[ui.day]], 90, 40, C_ORANGE);
    /* what she says, as it comes out */
    static char said[600];
    const char *line = news_line(ui.events[ui.day]);
    if (ui.lot_secret) {
        snprintf(said, sizeof said, "%s ...AND A WORD FOR THE DRIVER WHO KEEPS CRASHING IN THE OLD FENCED LOT ON "
                 "SAUCER HEIGHTS: THE SAUCER HOLDS FIFTY NOW. KEEP DRIVING.", line);
        line = said;
    }
    char shown[600];
    int k = imin((int)strlen(line), ui.state_t * 2);
    memcpy(shown, line, (size_t)k);
    shown[k] = 0;
    draw_text_page(shown, 90, 84, 212, C_WHITE);
    /* the ticker */
    gfx_rect(14, 140, 292, 10, C_RED);
    snprintf(buf, sizeof buf, "TRIES LEFT %d   WEEK SO FAR %d   DAILY BEST %d", ui.tries, ui.total, sv.daily_best);
    tiny_draw(buf, 20, 143, C_WHITE);
    for (int i = 0; i < 4; i++) gfx_circ(30 + i * 16, 162, 5, C_INK);
    if (ui.state_t > 40 && (ui.state_t / 20) % 2) text_draw(GLYPH_A " DRIVE", 250, 158, C_YELLOW);
}

static void draw_daydone(void) {
    draw_city_backdrop(10 * TNP_CELL, 7 * TNP_CELL);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(60, 20, 200, 140, C_INK, C_WHITE);
    text_center_shadow("CLOCKED OUT!", 160, 30, C_YELLOW, C_PURPLE);
    tnp_draw_zib(136, 44, 3, ui.t, 1);
    char buf[48];
    snprintf(buf, sizeof buf, "%s: %d DELIVERED", DAY_NAME[ui.day], ui.day_count[ui.day]);
    text_center(buf, 160, 98, C_WHITE);
    if (ui.day_count[ui.day] > TNP_QUOTA) {
        snprintf(buf, sizeof buf, "OVERTIME %d", ui.day_count[ui.day] - TNP_QUOTA);
        text_center(buf, 160, 110, C_LIME);
    }
    snprintf(buf, sizeof buf, "WEEK SO FAR %d", ui.total);
    text_center(buf, 160, 122, C_LIGHT);
    snprintf(buf, sizeof buf, "DAILY BEST %d", sv.daily_best);
    text_center(buf, 160, 134, C_GREY);
    if (ui.state_t > 60 && (ui.state_t / 20) % 2) text_center(GLYPH_A, 160, 146, C_YELLOW);
}

static void draw_failed(void) {
    gfx_cls(C_MAROON);
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, 8);
    text_center_shadow(ui.fail_reason == FAIL_TIME ? "OUT OF TIME!" : "WRECKED!", 160, 36, C_RED, C_INK);
    tnp_draw_rot(&tnp_spr[SP_TRUCK], 160, 72, 0.4f, NULL, -1);
    gfx_darken_rect(150, 62, 22, 22, 2);
    for (int k = 0; k < 5; k++) gfx_circ(150 + k * 5, 66 - (ui.t / 3 + k * 3) % 12, 2, k % 2 ? C_ORANGE : C_YELLOW);
    text_center(ui.fail_reason == FAIL_TIME ? "THE CLOCK RAN DOWN AND THE TRUCK CAUGHT FIRE." : "THE TRUCK IS A WRECK.",
                160, 92, C_WHITE);
    char buf[48];
    if (ui.tries > 0) {
        snprintf(buf, sizeof buf, "TRIES LEFT THIS WEEK: %d", ui.tries);
        text_center(buf, 160, 112, C_YELLOW);
        snprintf(buf, sizeof buf, "%s STARTS OVER", DAY_NAME[ui.day]);
        text_center(buf, 160, 124, C_LIGHT);
    } else
        text_center("NO TRIES LEFT.", 160, 112, C_RED);
}

static void draw_fired(void) {
    gfx_cls(C_INK);
    text_center_shadow("YOU'RE FIRED!", 160, 40, C_RED, C_MAROON);
    tnp_draw_zib(136, 60, 3, ui.t, 0);
    text_center("GRANNY ROOT HAS FOUND ANOTHER DRIVER.", 160, 116, C_WHITE);
    char buf[48];
    snprintf(buf, sizeof buf, "REACHED %s WITH %d DELIVERED", DAY_NAME[ui.day], ui.total);
    text_center(buf, 160, 130, C_GREY);
    text_center("BACK TO MONDAY.", 160, 150, C_SLATE);
}

static void draw_ending(void) {
    draw_city_backdrop(12 * TNP_CELL, 8 * TNP_CELL - 20);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(20, 4, 280, 96, C_INK, C_VIOLET);
    text_center_shadow(ui.cherry ? "EARLY RETIREMENT!" : "THE WEEKEND!", 160, 10, C_YELLOW, C_PURPLE);
    draw_text_page(ui.cherry
                       ? "SUNDAY EVENING. THE LAST TURNIP IS DELIVERED AND THE TRUCK IS BACK IN THE DEPOT. FIFTY "
                         "DELIVERIES IN ONE WEEK! GRANNY ROOT IS SO PLEASED THAT SHE GIVES ZIB AN EARLY RETIREMENT, "
                         "A GOLDEN TURNIP AND A TICKET TO THE BEACHES OF THE MOON."
                       : "SUNDAY EVENING. THE LAST TURNIP IS DELIVERED AND THE TRUCK IS BACK IN THE DEPOT. GRANNY "
                         "ROOT HANDS ZIB A PAY PACKET AND A WHOLE TURNIP. SEE YOU ON MONDAY!",
                   30, 28, 260, C_WHITE);
    tnp_draw_zib(104, 108, 3, ui.t, 1);
    spr_draw_scaled(&tnp_spr[SP_TURNIP], 170, 116, 4, 0);
    char buf[48];
    snprintf(buf, sizeof buf, "DELIVERED THIS WEEK: %d", ui.total);
    text_center(buf, 160, 166, ui.cherry ? C_LIME : C_YELLOW);
}

static void draw_credits(void) {
    gfx_cls(C_PURPLE);
    static const char *const LINES[] = {
        "TURNIP TRUCK", "", "A BEAMDOWN SOFTWORKS GAME", "1986", "", "ZIB, THE DRIVER", "GRANNY ROOT",
        "GLENDA GLORP OF CHANNEL 9", "AND THE PEOPLE OF BLIPTON", "", "A TRIBUTE TO ONION DELIVERY", "UFO 50 #23", "",
        "THANK YOU FOR DRIVING",
    };
    int y = 190 - ui.state_t / 3;
    for (int i = 0; i < ARRAY_LEN(LINES); i++) text_center(LINES[i], 160, y + i * 14, i == 0 ? C_YELLOW : C_WHITE);
}

static void tnp_draw(void) {
    tnp_hud_tries = ui.tries;
    tnp_hud_week = ui.total;
    tnp_hud_best = sv.daily_best;
    switch (ui.state) {
    case ST_TITLE: draw_title(); break;
    case ST_CODES: draw_codes(); break;
    case ST_DEMO:
        tnp_draw_play(&tnp);
        if ((ui.state_t / 30) % 2) text_center_shadow("DEMO", 192, 6, C_YELLOW, C_INK);
        break;
    case ST_NEWS: draw_news(); break;
    case ST_PLAY: tnp_draw_play(&tnp); break;
    case ST_DAYDONE: draw_daydone(); break;
    case ST_FAILED: draw_failed(); break;
    case ST_FIRED: draw_fired(); break;
    case ST_ENDING: draw_ending(); break;
    default: draw_credits(); break;
    }
}

/* ------------------------------------------------------------------------ */
/* cartridge interface                                                      */

static void tnp_load(void) {
    tnp_city_load();
    tnp_art_load();
    tnp_audio_load();
}

static void tnp_start(void) {
    load_save();
    memset(&ui, 0, sizeof ui);
    memset(&tnp, 0, sizeof tnp);
    tnp_quiet = false;
    tnp_bot_reset();
    to_title();
}

static void tnp_quit(void) {
    tnp_quiet = false;
    save_now();
}

static int count_cars(int kind) {
    int n = 0;
    for (int i = 0; i < TNP_MAX_CARS; i++) n += tnp.car[i].alive && tnp.car[i].wreck_t == 0 && (kind < 0 || tnp.car[i].kind == kind);
    return n;
}

static int count_mobs(int kind) {
    int n = 0;
    for (int i = 0; i < TNP_MAX_MOBS; i++) n += tnp.mob[i].alive && tnp.mob[i].dead_t == 0 && tnp.mob[i].kind == kind;
    return n;
}

/* the screens drive themselves for the tests: A on alternate frames, the demo player in play */
static int flow_buttons(void) {
    switch (ui.state) {
    case ST_PLAY: return tnp_bot_buttons(&tnp);
    case ST_TITLE: return ui.sel == 0 ? ((ui.t & 1) ? BTN_A : 0) : ((ui.t & 1) ? BTN_DOWN : 0);
    case ST_NEWS: case ST_DAYDONE: case ST_FAILED: case ST_FIRED: case ST_ENDING: return (ui.t & 1) ? BTN_A : 0;
    case ST_CREDITS: return BTN_A;
    default: return 0;
    }
}

static int tnp_query(const char *key, int *out) {
    const TnpTruck *t = &tnp.tr;
    if (!strcmp(key, "bot")) { *out = flow_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = ui.state; return 1; }
    if (!strcmp(key, "sel")) { *out = ui.sel; return 1; }
    if (!strcmp(key, "codes")) { *out = (int)ui.codes; return 1; }
    if (!strcmp(key, "day")) { *out = ui.day + 1; return 1; }
    if (!strcmp(key, "tries")) { *out = ui.tries; return 1; }
    if (!strcmp(key, "week")) { *out = ui.total; return 1; }
    if (!strcmp(key, "attempt")) { *out = ui.attempt; return 1; }
    if (!strcmp(key, "won")) { *out = ui.won; return 1; }
    if (!strcmp(key, "cherry")) { *out = ui.cherry; return 1; }
    if (!strcmp(key, "lot_secret")) { *out = ui.lot_secret; return 1; }
    if (!strcmp(key, "fail_reason")) { *out = ui.fail_reason; return 1; }
    if (!strcmp(key, "ev_perm_ok")) {
        /* Monday quiet, then each of the six events exactly once */
        int seen = 0, ok = ui.events[0] == EV_NONE;
        for (int i = 1; i < TNP_DAYS; i++) {
            if (ui.events[i] < 1 || ui.events[i] >= EV_COUNT) ok = 0;
            else seen |= 1 << ui.events[i];
        }
        *out = ok && seen == 0x7E;
        return 1;
    }
    if (!strncmp(key, "ev_", 3)) { int i = atoi(key + 3) - 1; *out = i >= 0 && i < TNP_DAYS ? ui.events[i] : -1; return 1; }
    if (!strncmp(key, "count_", 6)) { int i = atoi(key + 6) - 1; *out = i >= 0 && i < TNP_DAYS ? ui.day_count[i] : -1; return 1; }
    if (!strcmp(key, "event")) { *out = tnp.event; return 1; }
    if (!strcmp(key, "spawn_bad")) { *out = tnp.spawn_bad; return 1; }
    if (!strcmp(key, "squashed")) { *out = tnp.squashed; return 1; }
    if (!strcmp(key, "gang_chasing")) {
        int n = 0;
        for (int i = 0; i < TNP_MAX_CARS; i++) n += tnp.car[i].alive && tnp.car[i].kind == CK_GANG && tnp.car[i].chase_t > 0;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "gang_dist")) {
        int best = 9999;
        for (int i = 0; i < TNP_MAX_CARS; i++)
            if (tnp.car[i].alive && tnp.car[i].kind == CK_GANG) best = imin(best, (int)tnp_dist(tnp.car[i].x, tnp.car[i].y, t->x, t->y));
        *out = best;
        return 1;
    }
    if (!strcmp(key, "pair_gap")) {
        /* the widest gap between a gang car and the police car after it (-1: no pair) */
        int worst = -1;
        for (int i = 0; i < TNP_MAX_CARS; i++) {
            const TnpCar *c = &tnp.car[i];
            if (!c->alive || c->kind != CK_GANG || c->partner < 0 || !tnp.car[c->partner].alive) continue;
            worst = imax(worst, (int)tnp_dist(c->x, c->y, tnp.car[c->partner].x, tnp.car[c->partner].y));
        }
        *out = worst;
        return 1;
    }
    if (!strcmp(key, "spawned")) { *out = tnp.spawned; return 1; }
    if (!strcmp(key, "lanes_bad")) {
        /* cars on a straight stretch keep to the right-hand side of the road */
        static const int DX4[4] = {1, 0, -1, 0}, DY4[4] = {0, 1, 0, -1};
        int bad = 0;
        for (int i = 0; i < TNP_MAX_CARS; i++) {
            const TnpCar *c = &tnp.car[i];
            if (!c->alive || c->wreck_t > 0 || c->kind != CK_TRAFFIC || fabsf(c->nudge) > 0.5f) continue;
            int cx = (int)(c->x / TNP_CELL), cy = (int)(c->y / TNP_CELL);
            float rx = c->x - tnp_cx(cx), ry = c->y - tnp_cx(cy);
            float along = rx * (float)DX4[c->dir] + ry * (float)DY4[c->dir];
            float side = -rx * (float)DY4[c->dir] + ry * (float)DX4[c->dir];
            int px = cx - DX4[c->dir], py = cy - DY4[c->dir], nx = cx + DX4[c->dir], ny = cy + DY4[c->dir];
            bool straight = tnp_roadlike(px, py) && tnp_roadlike(nx, ny) && !tnp_roadlike(cx + DY4[c->dir], cy + DX4[c->dir]) &&
                            !tnp_roadlike(cx - DY4[c->dir], cy - DX4[c->dir]);
            if (straight && fabsf(along) < 12 && (side < 6 || side > 22)) bad++;
        }
        *out = bad;
        return 1;
    }
    if (!strcmp(key, "at_drop")) {
        int ok = 0;
        for (int i = 0; i < tnp_n_drops; i++) ok |= tnp_dist(t->x, t->y, tnp_cx(tnp_drop_c[i][0]), tnp_cx(tnp_drop_c[i][1])) < 2;
        *out = ok;
        return 1;
    }
    if (!strncmp(key, "car0_", 5)) {
        const TnpCar *c = NULL;
        for (int i = 0; i < TNP_MAX_CARS && !c; i++) if (tnp.car[i].alive) c = &tnp.car[i];
        if (!c) { *out = -999; return 1; }
        if (!strcmp(key + 5, "x")) { *out = (int)lroundf(c->x); return 1; }
        if (!strcmp(key + 5, "y")) { *out = (int)lroundf(c->y); return 1; }
        if (!strcmp(key + 5, "dir")) { *out = c->dir; return 1; }
        if (!strcmp(key + 5, "speed100")) { *out = (int)lroundf(c->speed * 100); return 1; }
        if (!strcmp(key + 5, "wreck")) { *out = c->wreck_t > 0; return 1; }
        return 0;
    }
    if (!strncmp(key, "mob0_", 5)) {
        const TnpMob *m = NULL;
        for (int i = 0; i < TNP_MAX_MOBS && !m; i++) if (tnp.mob[i].alive && tnp.mob[i].dead_t == 0) m = &tnp.mob[i];
        if (!m) { *out = -999; return 1; }
        if (!strcmp(key + 5, "x")) { *out = (int)lroundf(m->x); return 1; }
        if (!strcmp(key + 5, "y")) { *out = (int)lroundf(m->y); return 1; }
        if (!strcmp(key + 5, "state")) { *out = m->state; return 1; }
        if (!strcmp(key + 5, "kind")) { *out = m->kind; return 1; }
        if (!strcmp(key + 5, "dist")) { *out = (int)tnp_dist(m->x, m->y, t->x, t->y); return 1; }
        if (!strcmp(key + 5, "solid")) { *out = tnp_solid_type(tnp_cell_at(m->x, m->y)->type); return 1; }
        return 0;
    }
    if (!strcmp(key, "practice")) { *out = tnp.practice; return 1; }
    if (!strcmp(key, "phase")) { *out = tnp.phase; return 1; }
    if (!strcmp(key, "delivered")) { *out = tnp.delivered; return 1; }
    if (!strcmp(key, "time")) { *out = tnp_seconds(&tnp); return 1; }
    if (!strcmp(key, "time_f")) { *out = tnp.time_f; return 1; }
    if (!strcmp(key, "day_frame")) { *out = tnp.frame; return 1; }
    if (!strcmp(key, "dest")) { *out = tnp.dest; return 1; }
    if (!strcmp(key, "dest_t")) { *out = tnp.dest_t; return 1; }
    if (!strcmp(key, "hits")) { *out = tnp.hits; return 1; }
    if (!strcmp(key, "falls")) { *out = tnp.falls; return 1; }
    if (!strncmp(key, "hit_by", 6)) { *out = tnp.hit_by[atoi(key + 6) & 3]; return 1; }
    if (!strcmp(key, "crates_broken")) { *out = tnp.crates_broken; return 1; }
    if (!strcmp(key, "died_in_lot")) { *out = tnp.died_in_lot; return 1; }
    if (!strcmp(key, "hearts")) { *out = t->hearts; return 1; }
    if (!strcmp(key, "tstate")) { *out = t->state; return 1; }
    if (!strcmp(key, "x")) { *out = (int)lroundf(t->x); return 1; }
    if (!strcmp(key, "y")) { *out = (int)lroundf(t->y); return 1; }
    if (!strcmp(key, "ang")) { *out = (int)lroundf(t->ang * 180 / 3.14159265f); return 1; }
    if (!strcmp(key, "speed100")) { *out = (int)lroundf(tnp_speed(t) * 100); return 1; }
    if (!strcmp(key, "fwd100")) { *out = (int)lroundf(tnp_forward(t) * 100); return 1; }
    if (!strcmp(key, "side100")) { *out = (int)lroundf((-t->vx * sinf(t->ang) + t->vy * cosf(t->ang)) * 100); return 1; }
    if (!strcmp(key, "vx100")) { *out = (int)lroundf(t->vx * 100); return 1; }
    if (!strcmp(key, "vy100")) { *out = (int)lroundf(t->vy * 100); return 1; }
    if (!strcmp(key, "spin_t")) { *out = t->spin_t; return 1; }
    if (!strcmp(key, "spin_attack")) { *out = tnp_spin_attack(t); return 1; }
    if (!strcmp(key, "spin_steer")) { *out = (int)lroundf(t->spin_steer * 180 / 3.14159265f); return 1; }
    if (!strcmp(key, "air_t")) { *out = t->air_t; return 1; }
    if (!strcmp(key, "drift_t")) { *out = t->drift_t; return 1; }
    if (!strcmp(key, "regen_t")) { *out = t->regen_t; return 1; }
    if (!strcmp(key, "inv")) { *out = t->inv; return 1; }
    if (!strcmp(key, "flip")) { *out = t->flip_reverse; return 1; }
    if (!strcmp(key, "in_hq")) { *out = tnp_in_hq_zone(t->x, t->y); return 1; }
    if (!strcmp(key, "district")) { *out = tnp_district_at(t->x, t->y); return 1; }
    if (!strcmp(key, "cars")) { *out = count_cars(-1); return 1; }
    if (!strcmp(key, "traffic")) { *out = count_cars(CK_TRAFFIC); return 1; }
    if (!strcmp(key, "gangs")) { *out = count_cars(CK_GANG); return 1; }
    if (!strcmp(key, "police")) { *out = count_cars(CK_POLICE); return 1; }
    if (!strcmp(key, "wrecks")) { int n = 0; for (int i = 0; i < TNP_MAX_CARS; i++) n += tnp.car[i].alive && tnp.car[i].wreck_t > 0; *out = n; return 1; }
    if (!strcmp(key, "beets")) { *out = count_mobs(MK_BEET); return 1; }
    if (!strcmp(key, "mush")) { *out = count_mobs(MK_MUSH); return 1; }
    if (!strcmp(key, "saucers")) { *out = count_mobs(MK_SAUCER_S) + count_mobs(MK_SAUCER_M) + count_mobs(MK_SAUCER_L); return 1; }
    if (!strncmp(key, "saucers_", 8)) { *out = count_mobs(MK_SAUCER_S + atoi(key + 8)); return 1; }
    if (!strcmp(key, "beet_state")) { *out = -1; for (int i = 0; i < TNP_MAX_MOBS; i++) if (tnp.mob[i].alive && tnp.mob[i].kind == MK_BEET) *out = tnp.mob[i].state; return 1; }
    if (!strcmp(key, "shots")) { int n = 0; for (int i = 0; i < TNP_MAX_SHOTS; i++) n += tnp.shot[i].alive; *out = n; return 1; }
    if (!strncmp(key, "shots_", 6)) { int k = atoi(key + 6), n = 0; for (int i = 0; i < TNP_MAX_SHOTS; i++) n += tnp.shot[i].alive && tnp.shot[i].kind == k; *out = n; return 1; }
    if (!strcmp(key, "puddles_ahead")) {
        /* puddles born on the road in front of the truck (counted when they appear) */
        *out = tnp.puddles_ahead;
        return 1;
    }
    if (!strcmp(key, "puddles")) { int n = 0; for (int i = 0; i < TNP_MAX_PUDDLES; i++) n += tnp.pud[i].alive; *out = n; return 1; }
    if (!strcmp(key, "crates")) { int n = 0; for (int i = 0; i < tnp_n_crates; i++) n += tnp.crate_ok[i]; *out = n; return 1; }
    if (!strcmp(key, "cans")) { int n = 0; for (int i = 0; i < tnp_n_cans; i++) n += tnp.cans_ok[i]; *out = n; return 1; }
    if (!strcmp(key, "push100")) { float fx, fy; tnp_brine_push(&tnp, t->x, t->y, &fx, &fy); *out = (int)lroundf(sqrtf(fx * fx + fy * fy) * 100); return 1; }
    if (!strcmp(key, "hud_fits")) {
        /* every name the dashboard shows fits its 56 px, in glyphs the tiny font has */
        int ok = 1;
        char lines[4][UI_WRAP_LEN];
        for (int i = 0; i < TNP_DESTS; i++) {
            if (ui_wrap(TNP_DEST[i].name, 56, true, lines, 4) > 2 || tiny_missing(TNP_DEST[i].name)) ok = 0;
        }
        for (int i = 0; i < TNP_DISTRICTS; i++) ok &= tiny_width(TNP_DISTRICT_NAME[i]) <= 56 && !tiny_missing(TNP_DISTRICT_NAME[i]);
        for (int i = 1; i < EV_COUNT; i++) ok &= tiny_width(TNP_EVENT_NAME[i]) <= 56 && !tiny_missing(TNP_EVENT_NAME[i]);
        for (int i = 0; i < TNP_DAYS; i++) ok &= tiny_width(DAY_NAME[i]) <= 56;
        *out = ok;
        return 1;
    }
    if (!strcmp(key, "save_best")) { *out = sv.daily_best; return 1; }
    if (!strcmp(key, "save_week")) { *out = sv.best_week; return 1; }
    if (!strcmp(key, "save_weeks")) { *out = sv.weeks; return 1; }
    if (!strcmp(key, "save_cherries")) { *out = sv.cherries; return 1; }
    if (!strcmp(key, "save_starts")) { *out = sv.starts; return 1; }
    if (!strcmp(key, "save_delivered")) { *out = (int)sv.delivered; return 1; }
    /* the city as written */
    if (!strcmp(key, "map_crates")) { *out = tnp_n_crates; return 1; }
    if (!strcmp(key, "pipes_at_kerb")) {
        /* every hydrant stands on a corner of its crossing, by a building, clear of the middle */
        int ok = 1;
        for (int i = 0; i < tnp_n_pipes; i++) {
            float ox = tnp_pipe_px[i][0] - tnp_cx(tnp_pipe_c[i][0]), oy = tnp_pipe_px[i][1] - tnp_cx(tnp_pipe_c[i][1]);
            if (fabsf(ox) < 17 || fabsf(oy) < 17) ok = 0;
            if (!tnp_solid_type(tnp_cell(tnp_pipe_c[i][0] + tnp_pipe_side[i][0], tnp_pipe_c[i][1] + tnp_pipe_side[i][1])->type)) ok = 0;
        }
        *out = ok;
        return 1;
    }
    if (!strcmp(key, "spray_dir2")) { *out = tnp_pipe_spray_dir(&tnp, 2); return 1; }
    if (!strcmp(key, "map_cans")) { *out = tnp_n_cans; return 1; }
    if (!strcmp(key, "map_pipes")) { *out = tnp_n_pipes; return 1; }
    if (!strcmp(key, "map_drops")) { *out = tnp_n_drops; return 1; }
    if (!strcmp(key, "map_dests")) { *out = TNP_DESTS; return 1; }
    if (!strcmp(key, "map_rows_ok")) {
        int ok = 1;
        for (int i = 0; i < TNP_MAP; i++) ok &= (int)strlen(TNP_MAP_ROWS[i]) == TNP_MAP;
        *out = ok;
        return 1;
    }
    if (!strcmp(key, "map_dests_ok")) {
        /* every place: on a road beside a building, and reachable from the depot and back */
        int ok = 1;
        for (int i = 0; i < TNP_DESTS; i++) {
            int x = TNP_DEST[i].cx, y = TNP_DEST[i].cy, beside = 0;
            const TnpCell *c = tnp_cell(x, y);
            if (c->type != CT_ROAD || c->feat != FT_NONE) ok = 0;
            beside = tnp_cell(x + 1, y)->type == CT_BUILD || tnp_cell(x - 1, y)->type == CT_BUILD ||
                     tnp_cell(x, y + 1)->type == CT_BUILD || tnp_cell(x, y - 1)->type == CT_BUILD;
            if (!beside) ok = 0;
            if (tnp_path_len(14, 10, x, y) < 0 || tnp_path_len(x, y, 14, 10) < 0) ok = 0;
        }
        *out = ok;
        return 1;
    }
    if (!strcmp(key, "map_drops_ok")) {
        /* every brine cell next to a road has a drop zone within reach */
        int worst = 0;
        for (int y = 0; y < TNP_MAP; y++)
            for (int x = 0; x < TNP_MAP; x++) {
                if (tnp_map[y][x].type != CT_BRINE) continue;
                float best = 1e9f;
                for (int i = 0; i < tnp_n_drops; i++) best = fminf(best, tnp_dist(tnp_cx(x), tnp_cx(y), tnp_cx(tnp_drop_c[i][0]), tnp_cx(tnp_drop_c[i][1])));
                if ((int)best > worst) worst = (int)best;
            }
        *out = worst;
        return 1;
    }
    if (!strcmp(key, "map_wrap_ok")) {
        /* the city is in one piece, and its edges join: the Gloop Diner (top row) is a short
         * drive from the Pickle Works gate (four rows from the bottom) across the wrap */
        int ok = tnp_path_len(14, 10, 0, 0) >= 0 && tnp_path_len(14, 10, 31, 30) >= 0;
        int across = tnp_path_len(3, 0, 1, 28);
        *out = ok && across >= 0 && across <= 8;
        return 1;
    }
    if (!strcmp(key, "path_max")) {
        int worst = 0;
        for (int i = 0; i < TNP_DESTS; i++) worst = imax(worst, tnp_path_len(14, 10, TNP_DEST[i].cx, TNP_DEST[i].cy));
        *out = worst;
        return 1;
    }
    if (!strncmp(key, "path_", 5)) {
        /* path_I_J: cells from place I to place J (-1 = no way) */
        int i = atoi(key + 5), j = -1;
        const char *u = strchr(key + 5, '_');
        if (u) j = atoi(u + 1);
        if (i < 0 || i >= TNP_DESTS || j < 0 || j >= TNP_DESTS) return 0;
        *out = tnp_path_len(TNP_DEST[i].cx, TNP_DEST[i].cy, TNP_DEST[j].cx, TNP_DEST[j].cy);
        return 1;
    }
    return 0;
}

static int tnp_cheat(const char *cmd) {
    int a, b;
    float x, y;
    if (!strcmp(cmd, "week")) {
        /* a new week, straight into Monday's driving */
        new_week();
        start_day();
        return 1;
    }
    if (sscanf(cmd, "day %d %d", &a, &b) == 2) {
        /* play day A (1-7) with event B */
        uint64_t ws = (uint64_t)rng_next(&g_rng) << 32;
        ws |= rng_next(&g_rng);
        if (ui.state == ST_TITLE || ui.state == ST_CODES || ui.state == ST_DEMO) new_week();
        ui.week_seed = ws; /* the same day for the same seed, whatever came before */
        ui.attempt = 0;
        tnp_quiet = false;
        ui.day = iclamp(a, 1, TNP_DAYS) - 1;
        ui.events[ui.day] = iclamp(b, 0, EV_COUNT - 1);
        start_day();
        return 1;
    }
    {
        /* events A B C D E F: the week's order, Tuesday to Sunday */
        int v[6];
        if (sscanf(cmd, "events %d %d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) == 6) {
            for (int i = 0; i < 6; i++) ui.events[i + 1] = iclamp(v[i], 0, EV_COUNT - 1);
            return 1;
        }
    }
    if (!strcmp(cmd, "god")) { tnp.god = 1; return 1; }
    if (!strcmp(cmd, "mortal")) { tnp.god = 0; return 1; }
    if (!strcmp(cmd, "notraffic")) {
        tnp.no_traffic = 1;
        for (int i = 0; i < TNP_MAX_CARS; i++) tnp.car[i].alive = 0;
        return 1;
    }
    if (!strcmp(cmd, "traffic")) { tnp.no_traffic = 0; return 1; }
    if (!strcmp(cmd, "noevents")) {
        tnp.no_events = 1;
        memset(tnp.mob, 0, sizeof tnp.mob);
        memset(tnp.shot, 0, sizeof tnp.shot);
        memset(tnp.pud, 0, sizeof tnp.pud);
        return 1;
    }
    if (!strcmp(cmd, "events")) { tnp.no_events = 0; return 1; }
    if (!strcmp(cmd, "clearmobs")) { memset(tnp.mob, 0, sizeof tnp.mob); memset(tnp.shot, 0, sizeof tnp.shot); return 1; }
    if (sscanf(cmd, "pos %f %f", &x, &y) == 2) {
        tnp.tr.x = tnp_wrap(x);
        tnp.tr.y = tnp_wrap(y);
        return 1;
    }
    if (sscanf(cmd, "cell %d %d", &a, &b) == 2) {
        tnp.tr.x = tnp_cx(tnp_wrapc(a));
        tnp.tr.y = tnp_cx(tnp_wrapc(b));
        return 1;
    }
    if (sscanf(cmd, "inspray %d %d", &a, &b) == 2) {
        /* the truck B px from hydrant A, out in its spray across the road */
        int k = tnp_pipe_spray_dir(&tnp, iclamp(a, 0, tnp_n_pipes - 1));
        static const int SX[4] = {1, 0, -1, 0}, SY[4] = {0, 1, 0, -1};
        if (k < 0) return 1;
        tnp.tr.x = tnp_wrap(tnp_pipe_px[a][0] + (float)(SX[k] * b));
        tnp.tr.y = tnp_wrap(tnp_pipe_px[a][1] + (float)(SY[k] * b));
        return 1;
    }
    if (sscanf(cmd, "ang %d", &a) == 1) { tnp.tr.ang = (float)a * 3.14159265f / 180; return 1; }
    if (sscanf(cmd, "vel %f %f", &x, &y) == 2) { tnp.tr.vx = x; tnp.tr.vy = y; return 1; }
    if (!strcmp(cmd, "stop")) { tnp.tr.vx = tnp.tr.vy = tnp.tr.spin = 0; tnp.tr.spin_t = tnp.tr.air_t = 0; return 1; }
    if (sscanf(cmd, "hearts %d", &a) == 1) { tnp.tr.hearts = (int16_t)iclamp(a, 0, TNP_HEARTS); return 1; }
    if (sscanf(cmd, "time %d", &a) == 1) { tnp.time_f = a * 60; return 1; }
    if (sscanf(cmd, "timef %d", &a) == 1) { tnp.time_f = a; return 1; }
    if (sscanf(cmd, "delivered %d", &a) == 1) { tnp.delivered = a; return 1; }
    if (sscanf(cmd, "dest %d", &a) == 1) { tnp.dest = iclamp(a, 0, TNP_DESTS - 1); tnp.dest_t = 0; return 1; }
    if (sscanf(cmd, "tries %d", &a) == 1) { ui.tries = a; return 1; }
    if (sscanf(cmd, "total %d", &a) == 1) { ui.total = a; return 1; }
    if (sscanf(cmd, "inv %d", &a) == 1) { tnp.tr.inv = (int16_t)a; return 1; }
    {
        /* car KIND CX CY DIR: a car in that cell's lane */
        int v[4];
        if (sscanf(cmd, "car %d %d %d %d", &v[0], &v[1], &v[2], &v[3]) == 4) {
            int i = tnp_car_spawn(&tnp, iclamp(v[0], 0, 2), v[1], v[2], v[3] & 3);
            if (i >= 0) tnp.car[i].life = 3000;
            return 1;
        }
    }
    if (sscanf(cmd, "carat %d %f %f", &a, &x, &y) == 3) {
        /* a car standing still at a point (it starts moving next frame) */
        int i = tnp_car_spawn(&tnp, iclamp(a, 0, 2), (int)(x / TNP_CELL), (int)(y / TNP_CELL), DIR_E);
        if (i >= 0) { tnp.car[i].x = x; tnp.car[i].y = y; tnp.car[i].speed = 0; tnp.car[i].bump_t = 200; tnp.car[i].life = 3000; }
        return 1;
    }
    if (sscanf(cmd, "mob %d %f %f", &a, &x, &y) == 3) {
        for (int i = 0; i < TNP_MAX_MOBS; i++) {
            TnpMob *m = &tnp.mob[i];
            if (m->alive) continue;
            memset(m, 0, sizeof *m);
            m->alive = 1;
            m->kind = (int16_t)a;
            m->x = tnp_wrap(x);
            m->y = tnp_wrap(y);
            m->fire_t = 1;
            break;
        }
        return 1;
    }
    if (sscanf(cmd, "puddle %f %f", &x, &y) == 2) {
        for (int i = 0; i < TNP_MAX_PUDDLES; i++) {
            TnpPuddle *p = &tnp.pud[i];
            if (p->alive) continue;
            p->alive = 1;
            p->x = x;
            p->y = y;
            p->t = 60;
            p->life = 2000;
            break;
        }
        return 1;
    }
    {
        /* shot KIND X Y VX VY T: a bullet (0) or a bomb (1, landing at X Y after T frames) */
        int k, tt;
        float vx, vy;
        if (sscanf(cmd, "shot %d %f %f %f %f %d", &k, &x, &y, &vx, &vy, &tt) == 6) {
            for (int i = 0; i < TNP_MAX_SHOTS; i++) {
                TnpShot *sh = &tnp.shot[i];
                if (sh->alive) continue;
                sh->alive = 1;
                sh->kind = (int16_t)k;
                sh->x = x;
                sh->y = y;
                sh->vx = vx;
                sh->vy = vy;
                sh->t = (int16_t)tt;
                break;
            }
            return 1;
        }
    }
    if (!strcmp(cmd, "nopuddles")) { memset(tnp.pud, 0, sizeof tnp.pud); return 1; }
    if (!strcmp(cmd, "fail")) { tnp.tr.hearts = 1; tnp.tr.inv = 0; tnp.god = 0; tnp_hurt(&tnp, 0); return 1; }
    if (!strcmp(cmd, "clockout")) {
        tnp.delivered = imax(tnp.delivered, TNP_QUOTA);
        tnp.tr.x = TNP_HQ_DOOR_X;
        tnp.tr.y = TNP_HQ_DOOR_Y + 4;
        return 1;
    }
    if (sscanf(cmd, "quiet %d", &a) == 1) { tnp_quiet = a != 0; return 1; }
    return 0;
}

const GameDef GAME_TURNIP = {
    "turnip",
    "TURNIP TRUCK",
    "1986",
    "ARCADE DRIVING",
    "FIVE DROPS A DAY FOR A WEEK. THE WHEEL TURNS THE TRUCK, NOT THE SCREEN.",
    {"CLEAR TWO WORKDAYS", "CLOCK OUT ON SUNDAY", "DELIVER 50 TURNIPS IN ONE WEEK"},
    "HOLD " GLYPH_A "\tGAS\n"
    "HOLD " GLYPH_B "\tBRAKE, THEN REVERSE\n"
    "LEFT/RIGHT\tTURN THE TRUCK\n"
    "TAP L/R\tSIDESTEP\n"
    GLYPH_A "+" GLYPH_B "+TURN\tPOWERSLIDE\n"
    "START\tPAUSE\n"
    "\n"
    "TURNS ARE THE TRUCK'S OWN LEFT AND RIGHT.",
    C_VIOLET, C_CREAM,
    tnp_load, tnp_start, tnp_update, tnp_draw, tnp_quit, tnp_draw_label, tnp_query, tnp_cheat,
    "ONION DELIVERY", 23,
    NULL,
};
