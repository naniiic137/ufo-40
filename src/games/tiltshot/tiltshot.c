/* TILTSHOT - pinball golf at the Comet Classic. Cartridge 31 of UFO 40, a
 * tribute to Pingolf (UFO 50 #31). See docs/games/31-tiltshot.md.
 *
 * This file is the cartridge: the title, the code, the records, choosing a
 * golfer, the tournament's flow from hole card to leaderboard, the ending,
 * the save, and the test hooks. The ball and the rules of a stroke are in
 * tiltshot_ball.c, the field and scorecard in tiltshot_tour.c. */
#include "tiltshot.h"
#include <ctype.h>

enum { S_TITLE, S_CODE, S_RECORDS, S_PICK, S_FIELD, S_CARD, S_PLAY, S_SUNK, S_BOARD, S_FINAL, S_ENDING };
#define TITLE_ITEMS 4
#define CODE_LEN 8
static const char CODE_WORD[CODE_LEN + 1] = "BEAMDOWN"; /* BEAM-DOWN, printed in the credits */
static const char CODE_ABC[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

/* the original's two stats (golfers used, most holes in one in a round)
 * and the goals; nothing else is kept */
typedef struct Save {
    uint32_t magic;
    uint8_t used, goals, most_aces, pad;
} Save;
#define SAVE_MAGIC 0x54534802u

typedef struct Part {
    float x, y, vx, vy;
    int life, col, kind;
} Part;

static Save sv;
static TshCourse course;
static TshPlay play;
static TshTour tour;
static int state, state_t, frame_t, title_sel;
static int humans, turn;                  /* 1 or 2 players; whose turn on this hole */
static int pick_g[2], pick_c[2], pick_who;
static int code_pos, code_msg_t;
static char code_buf[CODE_LEN];
static bool code_ok;
/* the code works like a terminal code in the original: it lasts until you
 * leave the cartridge, and while it is on nothing is saved and no goals
 * are given */
static bool code_on;
static int aces_this, final_place[2], final_total[2];
static bool champ[2];
static int sunk_strokes;
static uint64_t next_seed = 31;

/* presentation */
static float cam;
static int cam_lock = -1;     /* tests and screenshots: hold the view */
static uint8_t flash[TSH_MAXCIRC];
static Part parts[220];
/* the dot-matrix display's event message (SPLASH!, BIRDIE!, ...) */
static const char *dmd_msg;
static int dmd_t, dmd_col, shake, follow_t, secret_t, card_scroll;
static float gx, gy;          /* where the golfer stands */
static bool gflip;

/* the demo player's plan for this turn */
static TshShot plan[TSH_ROUTE_MAX + 4];
static int plan_n, swings;
static bool plan_stale;

/* design checks (test hooks) */
static int sweep_in, sweep_n, timing_in, timing_n, timing_run, play_in;

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) {
            parts[i] = (Part){x, y, vx, vy, life, col, kind};
            return;
        }
}

/* ------------------------------------------------------------------------------ */
/* save                                                                             */

static void save_now(void) {
    if (code_on) return;
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else {
        memset(&sv, 0, sizeof sv);
        sv.magic = SAVE_MAGIC;
    }
    /* goals come back from what the save remembers */
    if (sv.goals & GOAL_BEACON) game_award(GOAL_BEACON);
    if (sv.goals & GOAL_SAUCER) game_award(GOAL_SAUCER);
    if (sv.goals & GOAL_ALIEN) game_award(GOAL_ALIEN);
}

static void award(int bit) {
    if (code_on) return;
    game_award(bit);
    if (!(sv.goals & bit)) {
        sv.goals |= (uint8_t)bit;
        save_now();
    }
}

static int golfers_open(void) { return code_on ? TSH_GOLFERS : 5; }

/* ------------------------------------------------------------------------------ */
/* flow                                                                             */

static void set_state(int s) {
    state = s;
    state_t = 0;
}

static void go_title(void) {
    set_state(S_TITLE);
    input_set_versus(false);
    game_set_pausable(false);
    music_play(TSH_MUS_TITLE);
}

static int play_song(int hole) { return hole == TSH_HOLES - 1 ? TSH_MUS_LAST : hole < 9 ? TSH_MUS_FRONT : TSH_MUS_BACK; }

static void go_pick(int n) {
    humans = n;
    pick_who = 0;
    for (int i = 0; i < 2; i++) {
        pick_g[i] = iclamp(pick_g[i], 0, golfers_open() - 1);
        pick_c[i] &= 1;
    }
    if (n == 2 && pick_g[1] == pick_g[0]) pick_g[1] = (pick_g[0] + 1) % golfers_open();
    set_state(S_PICK);
    input_set_versus(n == 2 && plat_kind() != PLAT_VITA);
    game_set_pausable(false);
}

static void begin_turn(void) {
    tsh_play_begin(&play, &course);
    swings = 0;
    plan_n = 0;
    for (int i = 0; i < TSH_ROUTE_MAX && TSH_ROUTE[course.hole][i].aim >= 0; i++) plan[plan_n++] = TSH_ROUTE[course.hole][i];
    plan_stale = false;
    cam = fmaxf(0, fminf(play.x - 110, (float)(course.w - SCREEN_W)));
    follow_t = 0;
    gx = play.x - 9;
    gy = play.y + TSH_R;
    gflip = false;
    memset(parts, 0, sizeof parts);
    memset(flash, 0, sizeof flash);
    dmd_msg = NULL;
    dmd_t = 0;
}

static void go_card(void) {
    if (course.hole != tour.hole || course.cols == 0) tsh_course_build(&course, tour.hole);
    tsh_terrain_build(&course);
    begin_turn();
    set_state(S_CARD);
    game_set_pausable(true);
    input_set_versus(humans == 2 && plat_kind() != PLAT_VITA);
    music_play(play_song(tour.hole));
}

static void start_play(void) {
    set_state(S_PLAY);
    /* whatever is still held from the card counts as held, not as a new press */
    uint8_t held = 0;
    bool p2 = humans == 2 && turn == 1 && plat_kind() != PLAT_VITA;
    bool (*h)(int) = p2 ? btn2 : btn;
    if (h(BTN_A)) held |= TB_A;
    if (h(BTN_LEFT)) held |= TB_LEFT;
    if (h(BTN_RIGHT)) held |= TB_RIGHT;
    play.prev = held;
}

static void start_tour(void) {
    int g[2] = {pick_g[0], pick_g[1]}, c[2] = {pick_c[0], pick_c[1]};
    uint64_t seed = rng_next(&g_rng) ^ ((uint64_t)next_seed++ << 32);
    tsh_tour_new(&tour, humans, g, c, seed);
    if (!code_on) {
        for (int i = 0; i < humans; i++) sv.used |= (uint8_t)(1u << pick_g[i]);
        save_now();
    }
    turn = 0;
    aces_this = 0;
    set_state(S_FIELD);
    game_set_pausable(true);
    music_play(TSH_MUS_BOARD);
}

static void go_board(void) {
    set_state(S_BOARD);
    music_play(TSH_MUS_BOARD);
}

static void go_final(void) {
    int won_any = 0;
    for (int i = 0; i < humans; i++) {
        final_place[i] = tsh_tour_place(&tour, i, TSH_HOLES);
        final_total[i] = tsh_tour_total(&tour, i, TSH_HOLES);
        champ[i] = final_place[i] == 1;
        if (champ[i]) {
            won_any = 1;
            award(GOAL_SAUCER);
            if (final_total[i] <= 0) award(GOAL_ALIEN);
        }
    }
    if (!code_on && aces_this > sv.most_aces) sv.most_aces = (uint8_t)aces_this;
    save_now();
    set_state(S_FINAL);
    input_set_versus(false);
    music_restart(won_any ? TSH_MUS_CHAMP : TSH_MUS_RUNNERUP);
}

static void sunk(void) {
    int s = play.strokes, par = course.par;
    tour.e[turn].score[tour.hole] = (uint8_t)iclamp(s, 1, 255);
    sunk_strokes = s;
    if (s == 1) {
        aces_this++;
        award(GOAL_BEACON);
        music_restart(TSH_MUS_ACE);
    } else music_restart(s <= par ? TSH_MUS_CUP : TSH_MUS_OVER);
    /* the result goes up on the display */
    static char word[16];
    const char *w = tsh_score_word(s, par);
    if (!w) {
        snprintf(word, sizeof word, "+%d", s - par);
        w = word;
    }
    dmd_msg = w;
    dmd_col = s <= par ? C_YELLOW : C_AMBER;
    dmd_t = 1 << 20;
    set_state(S_SUNK);
}

static void after_sunk(void) {
    if (humans == 2 && turn == 0) {
        turn = 1;
        go_card();
    } else go_board();
}

static void after_board(void) {
    if (tour.hole >= TSH_HOLES - 1) { go_final(); return; }
    tour.hole++;
    turn = 0;
    go_card();
}

/* ------------------------------------------------------------------------------ */
/* play                                                                             */

static bool p2_turn(void) { return humans == 2 && turn == 1 && plat_kind() != PLAT_VITA; }

static uint8_t read_buttons(void) {
    bool (*h)(int) = p2_turn() ? btn2 : btn;
    uint8_t b = 0;
    if (h(BTN_LEFT)) b |= TB_LEFT;
    if (h(BTN_RIGHT)) b |= TB_RIGHT;
    if (h(BTN_A)) b |= TB_A;
    return b;
}

/* an event flashes up on the dot-matrix display, pinball style */
static void say(const char *m, int col, int t) {
    dmd_msg = m;
    dmd_col = col;
    dmd_t = t;
}

static void handle_fx(void) {
    uint32_t fx = play.fx;
    play.fx = 0;
    if (!fx) return;
    if (fx & FXT_AIM) sfx_play_name("tsh_aim");
    if (fx & FXT_SWING) {
        sfx_play_name("tsh_swing");
        follow_t = 1;
        swings++;
    }
    if (fx & FXT_FULL) sfx_play_name("tsh_full");
    if (fx & FXT_WARN) sfx_play_name("tsh_warn");
    if (fx & FXT_BOOM) {
        sfx_play_name("tsh_boom");
        shake = 14;
        for (int i = 0; i < 26; i++) {
            float a = i / 26.0f * 6.2831853f;
            part_add(gx, gy - 8, cosf(a) * (1.2f + (i % 3) * 0.7f), sinf(a) * 1.6f - 1.0f, 22 + i % 9,
                     i % 3 == 0 ? C_YELLOW : i % 3 == 1 ? C_ORANGE : C_RED, 0);
        }
    }
    if (fx & FXT_SLAM) {
        sfx_play_name("tsh_slam");
        say("SLAM!", C_YELLOW, 24);
        for (int i = 0; i < 6; i++) part_add(play.x, play.y - 3, (i - 2.5f) * 0.4f, -1.0f, 10, C_WHITE, 1);
    }
    if (fx & FXT_FIRE) {
        sfx_play_name("tsh_fire");
        shake = imax(shake, 6);
    }
    if (fx & FXT_BUMPER) {
        sfx_play_name("tsh_bumper");
        say("BUMPER!", C_YELLOW, 30);
        if (play.fx_circ >= 0 && play.fx_circ < TSH_MAXCIRC) flash[play.fx_circ] = 10;
    } else if (fx & FXT_SPRING) {
        sfx_play_name("tsh_spring");
        say("BOING!", C_YELLOW, 30);
    }
    else if ((fx & FXT_BOUNCE) && play.fx_power > 1.2f) {
        sfx_play_name("tsh_bounce");
        for (int i = 0; i < 3; i++) part_add(play.fx_x, play.fx_y, (i - 1) * 0.5f, -0.6f, 8, C_LIGHT, 1);
    }
    if (fx & FXT_JUNK) {
        sfx_play_name("tsh_junk");
        say("CRUNCH!", C_AMBER, 30);
        for (int i = 0; i < 10; i++) part_add(play.fx_x, play.fx_y, (i - 4.5f) * 0.35f, -1.2f - (i % 3) * 0.4f, 24, i & 1 ? C_VIOLET : C_PURPLE, 0);
    }
    if (fx & FXT_MOVER) {
        sfx_play_name("tsh_mover");
        say("SMASH!", C_ORANGE, 40);
        shake = imax(shake, 5);
        for (int i = 0; i < 12; i++) {
            float a = i / 12.0f * 6.2831853f;
            part_add(play.fx_x, play.fx_y, cosf(a) * 1.4f, sinf(a) * 1.4f, 18, i & 1 ? C_ORANGE : C_YELLOW, 0);
        }
    }
    if (fx & FXT_SKIP) {
        sfx_play_name("tsh_skip");
        say("SKIP!", C_YELLOW, 24);
        for (int i = 0; i < 5; i++) part_add(play.fx_x, play.fx_y, (i - 2) * 0.4f, -0.9f, 14, C_ICE, 0);
    }
    if (fx & FXT_SPLASH) {
        sfx_play_name("tsh_splash");
        say("SPLASH!", C_AMBER, TSH_LOST_T);
        for (int i = 0; i < 16; i++) part_add(play.fx_x, play.fx_y, (i - 7.5f) * 0.25f, -1.6f - (i % 4) * 0.5f, 26, i & 1 ? C_ICE : C_SKY, 0);
        plan_stale = true;
    }
    if (fx & FXT_PIT) {
        sfx_play_name("tsh_pit");
        say("DOWN THE PIT!", C_AMBER, TSH_LOST_T);
        plan_stale = true;
    }
    if (fx & FXT_SAND) {
        sfx_play_name("tsh_sand");
        for (int i = 0; i < 6; i++) part_add(play.x, play.y + 2, (i - 2.5f) * 0.4f, -0.8f, 14, C_TAN, 0);
    }
    if (fx & FXT_SECRET) {
        sfx_play_name("tsh_secret");
        secret_t = 300;
    }
    if (fx & FXT_CUP) sfx_play_name("tsh_cup");
    /* the big ones last, so they win the display */
    if ((fx & FXT_FIRE) && !(fx & (FXT_SPLASH | FXT_PIT))) say("ON FIRE!", C_ORANGE, 60);
    if (fx & FXT_BOOM) {
        say("KA-BOOM!", C_ORANGE, 70);
        plan_stale = true;
    }
}

static void update_camera(void) {
    float maxc = (float)imax(0, course.w - SCREEN_W);
    float want;
    if (cam_lock >= 0) { cam = fminf((float)cam_lock, maxc); return; }
    if (play.phase == TP_FLIGHT) want = play.x - 160 + play.vx * 18;
    else want = play.x - (play.aim > 18 ? 210 : 110);
    want = fmaxf(0, fminf(want, maxc));
    cam += (want - cam) * (play.phase == TP_FLIGHT ? 0.14f : 0.10f);
}

static void update_play(void) {
    uint8_t b = read_buttons();
    int before = play.phase;
    tsh_play_step(&play, &course, b);
    if (before == TP_CHARGE && play.phase == TP_CHARGE && play.meter < TSH_FILL && play.meter % 12 == 11) sfx_play_name("tsh_tick");
    handle_fx();
    /* the golfer walks up to the ball once it has stopped */
    if (play.phase == TP_AIM && before != TP_AIM) follow_t = 0;
    if (play.phase == TP_AIM || play.phase == TP_CHARGE) {
        gflip = play.aim > 18;
        gx = play.x + (gflip ? 9 : -9);
        gy = play.y + TSH_R;
    }
    if (follow_t > 0) follow_t++;
    if (play.phase == TP_HOLED && before != TP_HOLED) sunk();
}

/* ------------------------------------------------------------------------------ */
/* menus                                                                            */

static void update_title(void) {
    game_set_pausable(false);
    if (btn_repeat(BTN_UP)) { title_sel = (title_sel + TITLE_ITEMS - 1) % TITLE_ITEMS; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { title_sel = (title_sel + 1) % TITLE_ITEMS; sfx_play_name("ui_move"); }
    if (btnp(BTN_B) && state_t > 10) { game_exit_to_library(); return; }
    if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) {
        sfx_play_name("ui_ok");
        switch (title_sel) {
        case 0: go_pick(1); break;
        case 1: go_pick(2); break;
        case 2:
            set_state(S_CODE);
            code_pos = 0;
            code_msg_t = 0;
            for (int i = 0; i < CODE_LEN; i++) code_buf[i] = 'A';
            break;
        default: set_state(S_RECORDS); break;
        }
    }
}

static void update_code(void) {
    if (code_msg_t > 0) {
        /* the answer shows until it times out or a button is pressed */
        code_msg_t--;
        if (code_msg_t == 0 || btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_START)) {
            code_msg_t = 0;
            if (code_ok) go_title();
        }
        return;
    }
    if (btn_repeat(BTN_LEFT)) { code_pos = (code_pos + CODE_LEN - 1) % CODE_LEN; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_RIGHT)) { code_pos = (code_pos + 1) % CODE_LEN; sfx_play_name("ui_move"); }
    int d = btn_repeat(BTN_UP) ? 1 : btn_repeat(BTN_DOWN) ? -1 : 0;
    if (d) {
        int n = (int)strlen(CODE_ABC);
        const char *at = strchr(CODE_ABC, code_buf[code_pos]);
        int i = at ? (int)(at - CODE_ABC) : 0;
        code_buf[code_pos] = CODE_ABC[(i + d + n) % n];
        sfx_play_name("ui_move");
    }
    if (btnp(BTN_B)) { sfx_play_name("ui_back"); go_title(); return; }
    if (btnp(BTN_A) || btnp(BTN_START)) {
        code_ok = memcmp(code_buf, CODE_WORD, CODE_LEN) == 0;
        if (code_ok) {
            code_on = true;
            sfx_play_name("tsh_code");
        } else sfx_play_name("ui_error");
        code_msg_t = 150;
    }
}

static void update_pick(void) {
    bool p2 = pick_who == 1 && plat_kind() != PLAT_VITA;
    bool (*rep)(int) = p2 ? btn_repeat2 : btn_repeat;
    bool (*pr)(int) = p2 ? btnp2 : btnp;
    int n = golfers_open(), w = pick_who;
    if (rep(BTN_LEFT)) { pick_g[w] = (pick_g[w] + n - 1) % n; sfx_play_name("ui_move"); }
    if (rep(BTN_RIGHT)) { pick_g[w] = (pick_g[w] + 1) % n; sfx_play_name("ui_move"); }
    if (rep(BTN_UP) || rep(BTN_DOWN)) { pick_c[w] ^= 1; sfx_play_name("ui_move"); }
    if (state_t > 8 && pr(BTN_B)) {
        sfx_play_name("ui_back");
        if (pick_who == 1) { pick_who = 0; state_t = 0; }
        else go_title();
        return;
    }
    if (state_t > 8 && pr(BTN_A)) {
        sfx_play_name("ui_ok");
        if (pick_who + 1 < humans) {
            pick_who++;
            state_t = 0;
        } else start_tour();
    }
}

static bool any_a(void) { return btnp(BTN_A) || btnp(BTN_START) || btnp2(BTN_A); }

static void tsh_update(void) {
    frame_t++;
    state_t++;
    if (shake > 0) shake--;
    if (dmd_t > 0 && state != S_SUNK) dmd_t--;
    if (secret_t > 0) secret_t--;
    for (int i = 0; i < TSH_MAXCIRC; i++)
        if (flash[i]) flash[i]--;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        if (p->kind == 0) p->vy += 0.1f;
    }
    switch (state) {
    case S_TITLE: update_title(); break;
    case S_CODE: update_code(); break;
    case S_RECORDS:
        if (state_t > 8 && (btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_START))) { sfx_play_name("ui_back"); go_title(); }
        break;
    case S_PICK: update_pick(); break;
    case S_FIELD:
        if (state_t > 30 && any_a()) { sfx_play_name("ui_ok"); tour.hole = 0; go_card(); }
        break;
    case S_CARD:
        update_camera();
        if (state_t > 150 || (state_t > 20 && (p2_turn() ? btnp2(BTN_A) : btnp(BTN_A)))) start_play();
        break;
    case S_PLAY:
        update_play();
        update_camera();
        break;
    case S_SUNK:
        update_camera();
        if (state_t > 150 || (state_t > 50 && any_a())) after_sunk();
        break;
    case S_BOARD:
        if (state_t > 40 && any_a()) { sfx_play_name("ui_ok"); after_board(); }
        break;
    case S_FINAL:
        if (state_t > 60 && any_a()) {
            sfx_play_name("ui_ok");
            if (champ[0] || (humans == 2 && champ[1])) {
                set_state(S_ENDING);
                card_scroll = 0;
                music_play(TSH_MUS_CHAMP);
            } else go_title();
        }
        break;
    case S_ENDING:
        card_scroll++;
        if (state_t > 240 && any_a()) { sfx_play_name("ui_ok"); go_title(); }
        break;
    }
}

/* ------------------------------------------------------------------------------ */
/* drawing                                                                          */

#define DMD_Y TSH_VIEW_H

static void dmd_panel(int x, int y, int w, int h) {
    gfx_rect(x, y, w, h, C_INK);
    for (int yy = y + 1; yy < y + h; yy += 2)
        for (int xx = x + 1; xx < x + w; xx += 2) gfx_pset(xx, yy, C_NIGHT);
}

static void dmd_text(const char *s, int x, int y, int col) {
    text_draw(s, x + 1, y + 1, C_MAROON);
    text_draw(s, x, y, col);
}

/* the slam lamp at the right end of the display: lit while the ball is in
 * the air and the slam is still there to use, dark once it is spent */
static bool slam_ready(void) { return state == S_PLAY && play.phase == TP_FLIGHT && !play.slam_used; }

static void draw_slam_lamp(int x, int y) {
    bool lit = slam_ready();
    gfx_rect(x, y, 44, 16, C_INK);
    gfx_rectb(x, y, 44, 16, lit ? C_YELLOW : C_MAROON);
    gfx_rect(x + 2, y + 2, 40, 12, lit ? ((frame_t / 8) & 1 ? C_AMBER : C_YELLOW) : C_NIGHT);
    if (lit)
        for (int k = 0; k < 3; k++) gfx_pset(x + 4 + k * 17, y + 3, C_WHITE);
    text_center("SLAM", x + 22, y + 5, lit ? C_INK : C_MAROON);
}

static void draw_hud(void) {
    gfx_camera(0, 0);
    dmd_panel(0, DMD_Y, SCREEN_W, SCREEN_H - DMD_Y);
    gfx_hline(0, SCREEN_W - 1, DMD_Y, C_SLATE);
    draw_slam_lamp(272, DMD_Y + 2);
    char buf[48];
    if (dmd_t > 0 && dmd_msg) {
        /* an event takes over the display for a moment, flashing */
        int w = text_width_scaled(dmd_msg, 2), x = 134 - w / 2;
        int col = (frame_t / 4) & 1 ? dmd_col : C_WHITE;
        text_draw_scaled(dmd_msg, x + 1, DMD_Y + 4, C_MAROON, 2);
        text_draw_scaled(dmd_msg, x, DMD_Y + 3, col, 2);
        /* stars round a hole in one */
        if (state == S_SUNK && play.strokes == 1)
            for (int i = 0; i < 8; i++) {
                uint32_t h = (uint32_t)(i * 2654435761u) ^ (uint32_t)(frame_t / 4 * 40503u);
                h ^= h >> 15;
                gfx_pset(4 + (int)(h % 260), DMD_Y + 2 + (int)((h >> 8) % 16), (i + frame_t / 4) % 2 ? C_YELLOW : C_WHITE);
            }
        return;
    }
    snprintf(buf, sizeof buf, "HOLE %d  PAR %d", tour.hole + 1, course.par);
    dmd_text(buf, 4, DMD_Y + 3, C_AMBER);
    const char *name = course.hole >= 0 ? TSH_HOLE[course.hole].name : "";
    dmd_text(name, 4, DMD_Y + 12, C_ORANGE);
    snprintf(buf, sizeof buf, "STROKE %d", play.strokes + (play.phase == TP_AIM || play.phase == TP_CHARGE ? 1 : 0));
    dmd_text(buf, 108, DMD_Y + 3, C_YELLOW);
    snprintf(buf, sizeof buf, "%d YDS", tsh_distance(&play, &course));
    dmd_text(buf, 108, DMD_Y + 12, C_AMBER);
    /* the meter: twenty dots; full, it turns white, then red */
    int mx = 184, my = DMD_Y + 4;
    int lit = play.phase == TP_CHARGE ? play.meter * 20 / TSH_FILL : 0;
    bool warn = play.phase == TP_CHARGE && play.max_t >= TSH_GRACE;
    for (int i = 0; i < 20; i++) {
        int col = i < lit ? (warn ? ((frame_t / 3) & 1 ? C_RED : C_YELLOW) : lit >= 20 ? C_WHITE : i < 12 ? C_AMBER : i < 17 ? C_ORANGE : C_RED) : C_MAROON;
        gfx_rect(mx + i * 4, my, 3, 5, col);
    }
    const TshEntrant *e = &tour.e[turn];
    if (humans == 2) snprintf(buf, sizeof buf, "P%d %s", turn + 1, TSH_GOLFER_NAME[e->golfer]);
    else snprintf(buf, sizeof buf, "%s", TSH_GOLFER_NAME[e->golfer]);
    const char *line = buf;
    int col = C_AMBER;
    if (warn) { line = "EASY NOW!"; col = (frame_t / 4) & 1 ? C_RED : C_YELLOW; }
    dmd_text(line, 184, DMD_Y + 12, col);
    int tot = tsh_tour_total(&tour, turn, tour.hole);
    if (tot == 0) snprintf(buf, sizeof buf, "E");
    else snprintf(buf, sizeof buf, "%+d", tot);
    dmd_text(buf, 266 - text_width(buf), DMD_Y + 12, C_YELLOW);
}

static void draw_ball_and_golfer(void) {
    int t = frame_t;
    bool show_golfer = !(play.phase == TP_BOOM && play.phase_t < TSH_BOOM_T - 10) && state != S_SUNK;
    if (show_golfer) {
        int pose = GP_STAND;
        if (play.phase == TP_CHARGE) pose = play.meter < 20 ? GP_BACK1 : play.meter < 45 ? GP_BACK2 : GP_BACK3;
        else if (follow_t > 0 && play.phase != TP_AIM) pose = GP_FOLLOW;
        if (play.phase == TP_HOLED) pose = GP_CHEER;
        bool red = play.phase == TP_CHARGE && play.max_t >= TSH_GRACE && ((t / 3) & 1);
        const TshEntrant *e = &tour.e[turn];
        tsh_draw_golfer(e->golfer, e->coat, pose, (int)lroundf(gx), (int)lroundf(gy), gflip, red);
    }
    if (play.phase == TP_HOLED) return;
    if (play.phase == TP_LOST && play.lost_why == TL_WATER) {
        if ((play.phase_t / 6) % 2 == 0) gfx_pset((int)play.x, (int)play.y - 2 - play.phase_t / 8, C_ICE);
        return;
    }
    if (play.phase == TP_LOST) return;
    int bx = (int)lroundf(play.x), by = (int)lroundf(play.y);
    if (play.fire_t > 0 && play.phase == TP_FLIGHT) {
        for (int k = 1; k < 5; k++) {
            int fx = bx - (int)(play.vx * k * 1.2f), fy = by - (int)(play.vy * k * 1.2f);
            gfx_circ(fx, fy - (t + k) % 2, 3 - k / 2, k < 2 ? C_YELLOW : k < 4 ? C_ORANGE : C_RED);
        }
    }
    gfx_circ(bx, by, 3, C_INK);
    gfx_circ(bx, by, 2, C_WHITE);
    gfx_pset(bx - 1, by - 1, C_WHITE);
    gfx_pset(bx + 1, by + 1, C_LIGHT);
    /* the dotted guide while aiming */
    if (play.phase == TP_AIM || play.phase == TP_CHARGE) {
        float dx = tsh_aim_dx(play.aim), dy = tsh_aim_dy(play.aim);
        for (int k = 1; k <= 5; k++) {
            int px = bx + (int)lroundf(dx * (4 + k * 5)), py = by + (int)lroundf(dy * (4 + k * 5));
            gfx_rect(px, py, 1 + (k == 5), 1 + (k == 5), ((t / 4 + k) % 5) ? C_WHITE : C_YELLOW);
        }
    }
    /* high above the screen: an arrow and the height */
    if (by < -2) {
        int ax = iclamp(bx, (int)cam + 4, (int)cam + SCREEN_W - 5);
        gfx_line(ax, 1, ax - 3, 5, C_WHITE);
        gfx_line(ax, 1, ax + 3, 5, C_WHITE);
        gfx_vline(ax, 1, 8, C_WHITE);
    }
}

static void draw_parts(void) {
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        const Part *p = &parts[i];
        if (p->life <= 0) continue;
        if (p->kind == 0 && p->life > 14) gfx_rect((int)p->x, (int)p->y, 2, 2, p->col);
        else gfx_pset((int)p->x, (int)p->y, p->col);
    }
}

static void draw_field_view(void) {
    int sh = shake > 0 ? ((shake / 2) % 2 ? 2 : -2) : 0;
    int c = (int)lroundf(cam) + sh;
    tsh_draw_sky(course.theme, c, frame_t);
    tsh_draw_course(&course, &play, c, frame_t, flash);
    gfx_camera(c, 0);
    gfx_clip(0, 0, SCREEN_W, TSH_VIEW_H);
    draw_ball_and_golfer();
    draw_parts();
    gfx_noclip();
    gfx_camera(0, 0);
    if (secret_t > 0) {
        ui_panel(40, 50, 240, 34, C_INK, C_RED);
        text_center("THEY BUILT IT FOR FORTY.", 160, 57, C_RED);
        text_center("FIFTY CAME DOWN.", 160, 69, C_RED);
    }
    draw_hud();
}

static void draw_card(void) {
    draw_field_view();
    int y = 22;
    ui_panel(20, y, 280, 112, C_NIGHT, C_AMBER);
    char buf[48];
    snprintf(buf, sizeof buf, "HOLE %d", tour.hole + 1);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_center(buf, 160, y + 6, 2, grad, 4, C_INK, C_WINE);
    text_center(TSH_HOLE[tour.hole].name, 160, y + 26, C_WHITE);
    snprintf(buf, sizeof buf, "PAR %d", TSH_HOLE[tour.hole].par);
    text_center(buf, 160, y + 36, C_YELLOW);
    int scale = course.cols <= 130 ? 2 : 1;
    int ow = course.cols * scale;
    tsh_draw_outline(&course, 160 - ow / 2, y + 48 + (scale == 1 ? 10 : 0), scale);
    if (humans == 2) {
        snprintf(buf, sizeof buf, "PLAYER %d " GLYPH_DOT " %s", turn + 1, TSH_GOLFER_NAME[tour.e[turn].golfer]);
        text_center(buf, 160, y + 98, C_SKY);
    } else text_center(GLYPH_A " TEE OFF", 160, y + 98, C_GREY);
}

/* in the cup: the course stays clear, the result is on the display */
static void draw_sunk(void) { draw_field_view(); }

static void total_str(int tot, char *buf, size_t n) {
    if (tot == 0) snprintf(buf, n, "E");
    else snprintf(buf, n, "%+d", tot);
}

static void entrant_name(int i, char *buf, size_t n) {
    const TshEntrant *e = &tour.e[i];
    if (e->human) {
        if (humans == 2) snprintf(buf, n, "%s (P%d)", TSH_GOLFER_NAME[e->golfer], e->human);
        else snprintf(buf, n, "%s", TSH_GOLFER_NAME[e->golfer]);
    } else snprintf(buf, n, "%s", TSH_RIVAL_NAME[e->golfer % TSH_RIVALS]);
}

static void draw_board(int holes, const char *title) {
    gfx_cls(C_NIGHT);
    dmd_panel(8, 8, 304, 164);
    gfx_rectb(8, 8, 304, 164, C_AMBER);
    text_center(title, 160, 14, C_YELLOW);
    text_draw("POS", 20, 28, C_ORANGE);
    text_draw("GOLFER", 52, 28, C_ORANGE);
    text_draw("HOLE", 184, 28, C_ORANGE);
    text_draw("TOTAL", 236, 28, C_ORANGE);
    gfx_hline(16, 303, 37, C_MAROON);
    int order[TSH_FIELD], place[TSH_FIELD];
    tsh_tour_rank(&tour, holes, order, place);
    for (int k = 0; k < TSH_FIELD; k++) {
        int i = order[k], y = 42 + k * 14;
        const TshEntrant *e = &tour.e[i];
        bool me = e->human != 0;
        if (me) gfx_rect(14, y - 3, 292, 13, (frame_t / 20) % 2 ? C_WINE : C_MAROON);
        char buf[40];
        bool tie = (k > 0 && place[k - 1] == place[k]) || (k + 1 < TSH_FIELD && place[k + 1] == place[k]);
        snprintf(buf, sizeof buf, "%s%d", tie ? "T" : "", place[k]);
        text_draw(buf, 20, y, me ? C_WHITE : C_AMBER);
        if (e->human) tsh_draw_head(e->golfer, e->coat, 40, y - 1);
        entrant_name(i, buf, sizeof buf);
        text_draw(buf, 52, y, me ? C_WHITE : C_AMBER);
        if (holes > 0) {
            int h = holes - 1;
            snprintf(buf, sizeof buf, "%d", e->score[h]);
            int d = e->score[h] - TSH_HOLE[h].par;
            text_draw(buf, 192, y, d < 0 ? C_YELLOW : d == 0 ? C_AMBER : C_ORANGE);
        }
        total_str(tsh_tour_total(&tour, i, holes), buf, sizeof buf);
        text_draw(buf, 244, y, me ? C_WHITE : C_YELLOW);
    }
    if (state_t > 40) text_center(GLYPH_A " CONTINUE", 160, 159, (frame_t / 20) % 2 ? C_GREY : C_LIGHT);
}

static const char *ordinal(int n) {
    static char buf[8];
    const char *suf = (n % 100 >= 11 && n % 100 <= 13) ? "TH" : n % 10 == 1 ? "ST" : n % 10 == 2 ? "ND" : n % 10 == 3 ? "RD" : "TH";
    snprintf(buf, sizeof buf, "%d%s", n, suf);
    return buf;
}

static void draw_final(void) {
    bool won = champ[0] || (humans == 2 && champ[1]);
    gfx_cls(won ? C_NAVY : C_NIGHT);
    tsh_draw_sky(TH_NIGHT, frame_t, frame_t);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
    if (won) {
        ui_fancy_center("CHAMPION!", 160, 20, 3, grad, 4, C_INK, C_WINE);
        spr_draw_scaled(&tsh_spr[TS_TROPHY], 144, 52, 2, 0);
        for (int i = 0; i < humans; i++)
            if (champ[i]) {
                const TshEntrant *e = &tour.e[i];
                tsh_draw_golfer(e->golfer, e->coat, GP_CHEER, 120 + 80 * i, 102, false, false);
            }
    } else {
        ui_fancy_center("THE COMET CLASSIC", 160, 24, 2, grad, 4, C_INK, C_WINE);
        text_center("IS OVER TILL THE COMET RETURNS", 160, 48, C_LIGHT);
    }
    char buf[64];
    for (int i = 0; i < humans; i++) {
        char tot[8];
        total_str(final_total[i], tot, sizeof tot);
        snprintf(buf, sizeof buf, "%s%s FINISHED %s AT %s", humans == 2 ? (i ? "P2 " : "P1 ") : "",
                 TSH_GOLFER_NAME[tour.e[i].golfer], ordinal(final_place[i]), tot);
        text_center_shadow(buf, 160, 120 + i * 11, champ[i] ? C_YELLOW : C_WHITE, C_INK);
    }
    if (state_t > 60) text_center(GLYPH_A " CONTINUE", 160, 160, C_GREY);
}

static const char *const CREDITS[] = {
    "TILTSHOT", "", "A BEAMDOWN SOFTWORKS GAME", "1987", "", "",
    "COURSE DESIGN", "THE BEAMDOWN TEAM", "", "PIXELS", "THE BEAMDOWN TEAM", "", "MUSIC", "THE BEAMDOWN TEAM", "", "",
    "ON THE TOUR", "NOVA", "DIGBY", "PEACHES", "TUCK", "MOSS", "", "AND THE REGULARS",
    "ORBO", "GLIMMA", "GEARBOX", "MAVIS", "BIG NED", "ZIBBO", "LADY FEN", "SPROUT", "DR QUILL", "BLIX", "HONK", "MARGO", "", "",
    "A WORD FROM THE CLUBHOUSE:", "TWO OLD FRIENDS", "WAIT FOR THE CODE", "BEAM-DOWN", "", "",
    "A TRIBUTE TO PINGOLF", "UFO 50 #31", "", "THANK YOU FOR PLAYING",
};

static void draw_ending(void) {
    gfx_cls(C_INK);
    tsh_draw_sky(TH_NIGHT, card_scroll, frame_t);
    int y = 170 - card_scroll / 2;
    for (int i = 0; i < ARRAY_LEN(CREDITS); i++, y += 12) {
        if (y < -10 || y > 180) continue;
        const char *s = CREDITS[i];
        bool head = i == 0 || !strcmp(s, "BEAM-DOWN");
        text_center_shadow(s, 160, y, head ? C_YELLOW : C_WHITE, C_INK);
    }
    if (y < 60) {
        spr_draw_scaled(&tsh_spr[TS_TROPHY], 144, 70, 2, 0);
    }
    spr_draw(&tsh_spr[TS_COMET], (card_scroll * 2) % 400 - 40, 20, 0);
}

static void draw_title(void) {
    tsh_draw_sky(TH_FAIR, frame_t / 2, frame_t);
    gfx_rect(0, 132, SCREEN_W, 48, C_WINE);
    gfx_hline(0, SCREEN_W - 1, 132, C_PINK);
    gfx_hline(0, SCREEN_W - 1, 133, C_MAGENTA);
    spr_draw(&tsh_spr[TS_COMET], (frame_t * 2) % 440 - 60, 12 + (frame_t / 60) % 3, 0);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center("TILTSHOT", 160, 22, 4, grad, 5, C_INK, C_WINE);
    text_center_shadow("THE COMET CLASSIC", 160, 58, C_CYAN, C_INK);
    static const char *const ITEMS[TITLE_ITEMS] = {"1 PLAYER", "2 PLAYERS", "CODE", "RECORDS"};
    ui_panel(108, 70, 104, 56, C_NIGHT, C_PINK);
    for (int i = 0; i < TITLE_ITEMS; i++) {
        int y = 76 + i * 12;
        bool sel = i == title_sel;
        text_center(ITEMS[i], 160, y, sel ? C_WHITE : C_LIGHT);
        if (sel) ui_cursor(160 - text_width(ITEMS[i]) / 2 - 12, y, frame_t);
    }
    /* a golfer on the green, swinging now and then */
    int ph = (frame_t / 30) % 6;
    int pose = ph < 3 ? GP_BACK1 + ph : ph == 3 ? GP_FOLLOW : GP_STAND;
    tsh_draw_golfer(0, 0, pose, 56, 132, false, false);
    gfx_circ(66, 129, 2, C_WHITE);
    tsh_draw_golfer(3, 1, GP_STAND, 264, 132, true, false);
    gfx_circ(252, 129, 2, C_WHITE);
    spr_draw(&tsh_spr[TS_FLAG1], 205, 110, 0);
    gfx_vline(204, 110, 132, C_LIGHT);
    tiny_center("THE COMET IS BACK. THE BUMPERS ARE WAITING.", 160, 141, C_CREAM);
    tiny_center("(C) 1987 BEAMDOWN SOFTWORKS", 160, 170, C_PINK);
    if (code_on) tiny_center("CODE ON: WICK AND KIP PLAY, NO GOALS OR SAVING", 160, 155, C_YELLOW);
}

static void draw_code(void) {
    gfx_cls(C_INK);
    dmd_panel(20, 30, 280, 110);
    gfx_rectb(20, 30, 280, 110, C_AMBER);
    text_center("ENTER A CODE", 160, 40, C_YELLOW);
    for (int i = 0; i < CODE_LEN; i++) {
        int x = 86 + i * 18 + (i >= 4 ? 12 : 0);
        char ch[2] = {code_buf[i], 0};
        bool sel = i == code_pos;
        gfx_rect(x - 2, 66, 13, 15, sel ? C_WINE : C_NIGHT);
        text_draw_scaled(ch, x + 1, 68, sel ? C_WHITE : C_AMBER, 1);
        if (sel) {
            text_draw(GLYPH_UP, x + 1, 56, C_ORANGE);
            text_draw(GLYPH_DOWN, x + 1, 84, C_ORANGE);
        }
    }
    text_draw("-", 86 + 4 * 18 - 3, 70, C_AMBER);
    if (code_msg_t > 0) {
        text_center(code_ok ? "WICK AND KIP JOIN THE TOUR!" : "NOTHING HAPPENS.", 160, 100, code_ok ? C_YELLOW : C_ORANGE);
        if (code_ok) tiny_center("TILL YOU LEAVE. NO GOALS OR SAVING MEANWHILE.", 160, 114, C_AMBER);
    } else text_center(GLYPH_A " ENTER   " GLYPH_B " BACK", 160, 118, C_GREY);
}

static void record_row(const char *label, const char *value, int y) {
    text_draw(label, 40, y, C_AMBER);
    text_draw(value, 280 - text_width(value), y, C_YELLOW);
}

/* the original's two stats and nothing more */
static void draw_records(void) {
    gfx_cls(C_INK);
    dmd_panel(20, 40, 280, 100);
    gfx_rectb(20, 40, 280, 100, C_AMBER);
    text_center("RECORDS", 160, 50, C_YELLOW);
    char v[24];
    int used = 0;
    for (int i = 0; i < 5; i++) used += (sv.used >> i) & 1;
    snprintf(v, sizeof v, "%d/5", used);
    record_row("GOLFERS USED", v, 76);
    snprintf(v, sizeof v, "%d", sv.most_aces);
    record_row("MOST HOLES IN ONE", v, 92);
    text_center(GLYPH_B " BACK", 160, 124, C_GREY);
}

static void draw_pick(void) {
    tsh_draw_sky(TH_MEADOW, frame_t / 3, frame_t);
    gfx_rect(0, 118, SCREEN_W, 62, C_LEAF);
    gfx_hline(0, SCREEN_W - 1, 118, C_LIME);
    char buf[48];
    snprintf(buf, sizeof buf, humans == 2 ? "PLAYER %d: CHOOSE A GOLFER" : "CHOOSE A GOLFER", pick_who + 1);
    text_center_shadow(buf, 160, 12, C_WHITE, C_INK);
    int n = golfers_open(), w = pick_who;
    int gap = n > 5 ? 40 : 52, x0 = 160 - (n - 1) * gap / 2;
    for (int i = 0; i < n; i++) {
        int x = x0 + i * gap;
        bool sel = i == pick_g[w];
        int coat = sel ? pick_c[w] : 0;
        static Surface tmp;
        static uint8_t tpx[24 * 24];
        tmp.w = 24; tmp.h = 24; tmp.px = tpx;
        memset(tpx, TRANSPARENT, sizeof tpx);
        gfx_set_target(&tmp);
        tsh_draw_golfer(i, coat, sel && (frame_t / 20) % 2 ? GP_CHEER : GP_STAND, 12, 22, false, false);
        gfx_set_target(NULL);
        int sc = sel ? 3 : 2, bx = x - 12 * sc, by = 116 - 22 * sc;
        for (int yy = 0; yy < 24; yy++)
            for (int xx = 0; xx < 24; xx++)
                if (tpx[yy * 24 + xx] != TRANSPARENT) gfx_rect(bx + xx * sc, by + yy * sc, sc, sc, tpx[yy * 24 + xx]);
        if (sel) {
            text_center(TSH_GOLFER_NAME[i], x, 126, C_WHITE);
            tiny_center(tsh_coat_name(i, pick_c[w]), x, 136, C_YELLOW);
        }
    }
    if (humans == 2 && pick_who == 1) {
        snprintf(buf, sizeof buf, "P1: %s", TSH_GOLFER_NAME[pick_g[0]]);
        tiny_draw(buf, 6, 24, C_CREAM);
    }
    text_center(GLYPH_LEFT GLYPH_RIGHT " GOLFER   " GLYPH_UP GLYPH_DOWN " COLOURS   " GLYPH_A " OK", 160, 160, C_WHITE);
    tiny_center("EVERY GOLFER PLAYS THE SAME", 160, 172, C_FOREST);
}

static void draw_field(void) {
    gfx_cls(C_NIGHT);
    dmd_panel(8, 8, 304, 164);
    gfx_rectb(8, 8, 304, 164, C_AMBER);
    text_center("THE FIELD", 160, 16, C_YELLOW);
    text_center("EIGHT GOLFERS " GLYPH_DOT " EIGHTEEN HOLES " GLYPH_DOT " PAR 61", 160, 28, C_ORANGE);
    for (int i = 0; i < TSH_FIELD; i++) {
        int col = i % 2, row = i / 2;
        int x = 30 + col * 146, y = 50 + row * 22;
        char buf[40];
        entrant_name(i, buf, sizeof buf);
        if (tour.e[i].human) tsh_draw_head(tour.e[i].golfer, tour.e[i].coat, x, y - 1);
        else gfx_circ(x + 4, y + 3, 3, C_AMBER);
        text_draw(buf, x + 12, y, tour.e[i].human ? C_WHITE : C_AMBER);
    }
    text_center("LOWEST TOTAL AFTER 18 HOLES WINS", 160, 142, C_LIGHT);
    if (state_t > 30) text_center(GLYPH_A " TO THE FIRST TEE", 160, 157, (frame_t / 20) % 2 ? C_GREY : C_LIGHT);
}

static void tsh_draw(void) {
    gfx_camera(0, 0);
    gfx_noclip();
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_CODE: draw_code(); break;
    case S_RECORDS: draw_records(); break;
    case S_PICK: draw_pick(); break;
    case S_FIELD: draw_field(); break;
    case S_CARD: draw_card(); break;
    case S_PLAY: draw_field_view(); break;
    case S_SUNK: draw_sunk(); break;
    case S_BOARD: {
        char t[32];
        if (tour.hole >= TSH_HOLES - 1) snprintf(t, sizeof t, "FINAL STANDINGS");
        else snprintf(t, sizeof t, "AFTER HOLE %d", tour.hole + 1);
        draw_board(tour.hole + 1, t);
        break;
    }
    case S_FINAL: draw_final(); break;
    case S_ENDING: draw_ending(); break;
    }
    gfx_camera(0, 0);
}

/* ------------------------------------------------------------------------------ */
/* the cartridge                                                                    */

static void tsh_load(void) {
    tsh_art_load();
    tsh_audio_load();
}

static void tsh_start(void) {
    code_on = code_ok = false; /* a code from an earlier visit is gone */
    load_save();
    course.cols = 0;
    course.hole = -1;
    humans = 1;
    title_sel = 0;
    frame_t = 0;
    go_title();
}

static void tsh_quit(void) {
    save_now();
    code_on = false; /* leaving the cartridge switches the code off */
    input_set_versus(false);
}

static void tsh_label(int x, int y, int w, int h, int t) {
    for (int i = 0; i < 4; i++) gfx_rect(x, y + i * 12, w, 12, (const uint8_t[]){C_NAVY, C_PURPLE, C_MAGENTA, C_PINK}[i]);
    gfx_rect(x, y + 44, w, h - 44, C_WINE);
    gfx_hline(x, x + w - 1, y + 44, C_PINK);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_text("TILTSHOT", x + 8, y + 5, 2, grad, 4, C_INK, C_WINE);
    gfx_circ(x + 104, y + 32, 6, C_INK);
    gfx_circ(x + 104, y + 32, 5, (t / 10) % 4 ? C_CYAN : C_WHITE);
    gfx_circ(x + 104, y + 32, 3, C_BLUE);
    int ph = (t / 16) % 5;
    tsh_draw_golfer(2, 0, ph < 3 ? GP_BACK1 + ph : GP_FOLLOW, x + 22, y + 44, false, false);
    int k = t % 80;
    float bx = x + 34 + k * 1.2f, by = y + 42 - k * 1.1f + k * k * 0.012f;
    if (k < 70) gfx_circ((int)bx, (int)by, 2, C_WHITE);
    gfx_vline(x + 128, y + 26, y + 44, C_LIGHT);
    spr_draw(&tsh_spr[(t / 12) & 1 ? TS_FLAG2 : TS_FLAG1], x + 129, y + 26, 0);
}

/* ------------------------------------------------------------------------------ */
/* test hooks                                                                       */

static const TshShot *bot_shot(void) {
    if (plan_stale || swings >= plan_n) {
        /* off the plan: find a way from here */
        TshShot out[8];
        int n = tsh_solve(&course, &play, out, 4, 3, false);
        plan_n = swings;
        for (int i = 0; i < n && plan_n < ARRAY_LEN(plan); i++) plan[plan_n++] = out[i];
        plan_stale = false;
        if (!n) return NULL;
    }
    return &plan[swings];
}

static int bot_mask(void) {
    switch (state) {
    case S_PLAY: {
        if (play.phase != TP_AIM && play.phase != TP_CHARGE && play.phase != TP_FLIGHT) return 0;
        /* the shot being played: the next one while aiming or charging, the last one in flight */
        if (play.phase == TP_AIM && !bot_shot()) return 0;
        int idx = play.phase == TP_FLIGHT ? swings - 1 : swings;
        if (idx < 0 || idx >= plan_n) return 0;
        const TshShot *s = &plan[idx];
        uint8_t b = tsh_bot_buttons(&play, s);
        int m = ((b & TB_LEFT) ? BTN_LEFT : 0) | ((b & TB_RIGHT) ? BTN_RIGHT : 0) | ((b & TB_A) ? BTN_A : 0);
        return p2_turn() ? m << BTN_P2_SHIFT : m;
    }
    case S_SUNK: case S_BOARD: case S_FINAL: case S_FIELD:
        return state_t > 60 && (frame_t & 1) ? BTN_A : 0;
    case S_ENDING:
        return state_t > 250 && (frame_t & 1) ? BTN_A : 0;
    default:
        return 0;
    }
}

static int tsh_query(const char *key, int *out) {
    if (!strcmp(key, "bot")) { *out = bot_mask(); return 1; }
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "hole")) { *out = tour.hole + 1; return 1; }
    if (!strcmp(key, "turn")) { *out = turn + 1; return 1; }
    if (!strcmp(key, "humans")) { *out = humans; return 1; }
    if (!strcmp(key, "phase")) { *out = play.phase; return 1; }
    if (!strcmp(key, "strokes")) { *out = play.strokes; return 1; }
    if (!strcmp(key, "meter")) { *out = play.meter; return 1; }
    if (!strcmp(key, "max_t")) { *out = play.max_t; return 1; }
    if (!strcmp(key, "warning")) { *out = play.phase == TP_CHARGE && play.max_t >= TSH_GRACE; return 1; }
    if (!strcmp(key, "aim")) { *out = play.aim; return 1; }
    if (!strcmp(key, "ball_x")) { *out = (int)lroundf(play.x); return 1; }
    if (!strcmp(key, "ball_y")) { *out = (int)lroundf(play.y); return 1; }
    if (!strcmp(key, "ball_vx100")) { *out = (int)lroundf(play.vx * 100); return 1; }
    if (!strcmp(key, "ball_vy100")) { *out = (int)lroundf(play.vy * 100); return 1; }
    if (!strcmp(key, "speed100")) { *out = (int)lroundf(sqrtf(play.vx * play.vx + play.vy * play.vy) * 100); return 1; }
    if (!strcmp(key, "shot_x")) { *out = (int)lroundf(play.sx); return 1; }
    if (!strcmp(key, "shot_y")) { *out = (int)lroundf(play.sy); return 1; }
    if (!strcmp(key, "fire")) { *out = play.fire_t; return 1; }
    if (!strcmp(key, "slam_used")) { *out = play.slam_used; return 1; }
    if (!strcmp(key, "lost")) { *out = play.phase == TP_LOST ? play.lost_why : 0; return 1; }
    if (!strcmp(key, "holed")) { *out = play.phase == TP_HOLED; return 1; }
    if (!strcmp(key, "skips")) { *out = play.skips; return 1; }
    if (!strcmp(key, "secret")) { *out = play.secret; return 1; }
    if (!strcmp(key, "slam_lamp")) { *out = slam_ready(); return 1; }
    if (!strcmp(key, "sweep_in")) { *out = sweep_in; return 1; }
    if (!strcmp(key, "play_in")) { *out = play_in; return 1; }
    if (!strcmp(key, "sweep_n")) { *out = sweep_n; return 1; }
    if (!strcmp(key, "timing_pct")) { *out = timing_in * 100 / imax(1, timing_n); return 1; }
    if (!strcmp(key, "timing_run")) { *out = timing_run; return 1; }
    if (!strcmp(key, "dmd")) { *out = dmd_t > 0 && dmd_msg; return 1; }
    if (!strcmp(key, "cam")) { *out = (int)lroundf(cam); return 1; }
    if (!strcmp(key, "junk_gone")) { int n = 0; for (int i = 0; i < 32; i++) n += (play.junk_gone >> i) & 1; *out = n; return 1; }
    if (!strcmp(key, "movers_gone")) { int n = 0; for (int i = 0; i < 32; i++) n += (play.mover_gone >> i) & 1; *out = n; return 1; }
    if (!strcmp(key, "cup_x")) { *out = (int)course.cup_x; return 1; }
    if (!strcmp(key, "tee_x")) { *out = (int)course.tee_x; return 1; }
    if (!strcmp(key, "par")) { *out = course.par; return 1; }
    if (!strcmp(key, "cameos")) { *out = code_on; return 1; }
    if (!strcmp(key, "golfers")) { *out = golfers_open(); return 1; }
    if (!strcmp(key, "pick1")) { *out = pick_g[0]; return 1; }
    if (!strcmp(key, "pick2")) { *out = pick_g[1]; return 1; }
    if (!strcmp(key, "coat1")) { *out = pick_c[0]; return 1; }
    if (!strcmp(key, "code_ok")) { *out = code_ok && code_msg_t > 0; return 1; }
    if (!strcmp(key, "title_sel")) { *out = title_sel; return 1; }
    if (!strcmp(key, "most_aces")) { *out = sv.most_aces; return 1; }
    if (!strcmp(key, "used")) { *out = sv.used; return 1; }
    if (!strcmp(key, "champ")) { *out = champ[0] | (champ[1] << 1); return 1; }
    if (!strcmp(key, "place1")) { *out = tsh_tour_place(&tour, 0, TSH_HOLES); return 1; }
    if (!strcmp(key, "place2")) { *out = tsh_tour_place(&tour, 1, TSH_HOLES); return 1; }
    if (!strcmp(key, "total1")) { *out = tsh_tour_total(&tour, 0, TSH_HOLES); return 1; }
    if (!strcmp(key, "total2")) { *out = tsh_tour_total(&tour, 1, TSH_HOLES); return 1; }
    if (!strcmp(key, "cpu_best") || !strcmp(key, "cpu_worst")) {
        int best = 999, worst = -999;
        for (int i = tour.humans; i < TSH_FIELD; i++) {
            int s = 0;
            for (int h = 0; h < TSH_HOLES; h++) s += tour.e[i].score[h] - TSH_HOLE[h].par;
            best = imin(best, s);
            worst = imax(worst, s);
        }
        *out = key[4] == 'b' ? best : worst;
        return 1;
    }
    if (!strcmp(key, "cpu_count")) { *out = TSH_FIELD - tour.humans; return 1; }
    if (!strcmp(key, "cpu_low")) {
        /* the fewest strokes any CPU took on any hole */
        int lo = 99;
        for (int i = tour.humans; i < TSH_FIELD; i++)
            for (int h = 0; h < TSH_HOLES; h++) lo = imin(lo, tour.e[i].score[h]);
        *out = lo;
        return 1;
    }
    if (!strcmp(key, "sunk_diff")) { *out = sunk_strokes - course.par; return 1; }
    if (!strcmp(key, "versus")) { *out = input_versus(); return 1; }
    if (!strncmp(key, "mover", 5) && isdigit((unsigned char)key[5])) {
        int i = key[5] - '0';
        float mx = 0, my = 0;
        if (i >= course.nmover) return 0;
        tsh_mover_pos(&course.mover[i], play.clock, &mx, &my);
        *out = key[7] == 'x' ? (int)lroundf(mx) : (int)lroundf(my);
        return 1;
    }
    if (!strncmp(key, "score", 5) && key[5]) {
        /* score<player>_<hole>: strokes on a hole */
        int pl = 0, h = 0;
        if (sscanf(key + 5, "%d_%d", &pl, &h) == 2 && pl >= 1 && pl <= 2 && h >= 1 && h <= TSH_HOLES) {
            *out = tour.e[pl - 1].score[h - 1];
            return 1;
        }
    }
    if (!strcmp(key, "route_total")) {
        /* strokes in all the stored routes together */
        int n = 0;
        for (int h = 0; h < TSH_HOLES; h++)
            for (int i = 0; i < TSH_ROUTE_MAX && TSH_ROUTE[h][i].aim >= 0; i++) n++;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "route", 5)) {
        /* route<hole>: strokes in its stored route */
        int h = atoi(key + 5), n = 0;
        if (h < 1 || h > TSH_HOLES) return 0;
        while (n < TSH_ROUTE_MAX && TSH_ROUTE[h - 1][n].aim >= 0) n++;
        *out = n;
        return 1;
    }
    return 0;
}

/* ---- design checks ---------------------------------------------------------- */

/* how long until every mover is back where it started (capped) */
static int movers_round(const TshCourse *c) {
    int l = 1;
    for (int i = 0; i < c->nmover; i++) {
        int p = imax(1, c->mover[i].period), a = l, b = p;
        while (b) { int t = a % b; a = b; b = t; }
        l = imin(l / a * p, 7200);
    }
    return l;
}

/* is (x, y) where a mover could be met (within reach of any point of its
 * path, with some to spare)? */
static bool near_mover_path(const TshCourse *c, float x, float y) {
    for (int i = 0; i < c->nmover; i++) {
        const TshMoverDef *m = &c->mover[i];
        int per = m->period ? m->period : 1;
        for (int k = 0; k < 96; k++) {
            float mx, my;
            if (!tsh_mover_pos(m, k * per / 96, &mx, &my)) continue;
            float ex = x - mx, ey = y - my, r = TSH_R + 5 + 4;
            if (ex * ex + ey * ey < r * r) return true;
        }
    }
    return false;
}

/* play a shot on a copy of the course with nothing moving; where the ball
 * passes a mover's path, also play on as if it had struck the mover there
 * (the mover keeps a fifth of the ball's speed). Returns 1 if any of these
 * drops in. */
static int sweep_lost;

static int sweep_shot(TshCourse *still, const TshCourse *moving, const TshShot *s, int *branches) {
    TshPlay q;
    tsh_play_begin(&q, still);
    bool fired = false;
    for (int f = 0; f < 6000; f++) {
        tsh_play_step(&q, still, tsh_bot_buttons(&q, s));
        q.fx = 0;
        if (q.phase == TP_HOLED) return 1;
        if (q.phase == TP_FLIGHT) {
            fired = true;
            if (moving->nmover && near_mover_path(moving, q.x, q.y)) {
                TshPlay b = q;
                b.vx *= 0.2f;
                b.vy *= 0.2f;
                (*branches)++;
                for (int g = 0; g < 6000 && b.phase == TP_FLIGHT; g++) {
                    tsh_play_step(&b, still, 0);
                    b.fx = 0;
                }
                if (b.phase == TP_HOLED) return 1;
            }
        } else if (fired || q.phase == TP_LOST || q.phase == TP_BOOM) {
            if (q.phase == TP_LOST) sweep_lost++;
            return 0;
        }
    }
    return 0;
}

/* Every aim at every strength with no slam, from the tee: how many drop in?
 * Exact for a course with nothing moving; with movers, every shot is also
 * played on from every point where it could meet one (a mover keeps a fifth
 * of the ball's speed), and then the real course is swept again at `moments`
 * start times spread through the movers' whole round. */
static void sweep_hole(int hole, int moments, bool log) {
    TshCourse *mv = malloc(sizeof *mv), *st = malloc(sizeof *st);
    if (!mv || !st) { free(mv); free(st); return; }
    tsh_course_build(mv, hole);
    *st = *mv;
    st->nmover = 0;
    int hits = 0, n = 0, branches = 0, per = 0;
    sweep_lost = 0;
    for (int aim = 0; aim < TSH_AIMS; aim++)
        for (int pw = 0; pw <= TSH_FILL; pw++) {
            TshShot s = {(int8_t)aim, (int8_t)pw, -1};
            n++;
            if (sweep_shot(st, mv, &s, &branches)) {
                hits++;
                if (log) printf("    ace without a slam: aim %d, %d frames of A\n", aim, pw);
            }
        }
    if (mv->nmover) per = movers_round(mv);
    int timed = 0;
    for (int m = 0; per && m < moments; m++) {
        int d = (int)((long)per * m / moments) + m % 7;
        for (int aim = 0; aim < TSH_AIMS; aim++)
            for (int pw = 0; pw <= TSH_FILL; pw++) {
                TshPlay q;
                tsh_play_begin(&q, mv);
                q.clock = d;
                TshShot s = {(int8_t)aim, (int8_t)pw, -1};
                n++;
                timed++;
                if (tsh_try_shot(&q, mv, &s) == TP_HOLED) {
                    hits++;
                    if (log) printf("    ace without a slam: aim %d, %d frames of A, %d frames in\n", aim, pw, d);
                }
            }
    }
    sweep_in = hits;
    sweep_n = n;
    if (log) {
        printf("  hole %d: %d of %d shots with no slam drop in, %d lost (%d mover strikes tried, %d timed shots)\n", hole + 1,
               hits, n, sweep_lost, branches, timed);
        fflush(stdout);
    }
    free(mv);
    free(st);
}

/* one shot swung at every moment of the movers' round (the aim-and-pray
 * odds): how often it drops in, and the pattern (# in, . not) */
static void timing_hole(int hole, int aim, int power) {
    TshCourse *cc = malloc(sizeof *cc);
    if (!cc) return;
    tsh_course_build(cc, hole);
    int per = movers_round(cc);
    char pat[1800];
    int np = 0, run = 0, best_run = 0;
    timing_in = timing_n = 0;
    for (int d = 0; d < per; d++) {
        TshPlay q;
        tsh_play_begin(&q, cc);
        q.clock = d;
        TshShot s = {(int8_t)aim, (int8_t)power, -1};
        bool in = tsh_try_shot(&q, cc, &s) == TP_HOLED;
        timing_n++;
        timing_in += in;
        run = in ? run + 1 : 0;
        best_run = imax(best_run, run);
        if (d % 4 == 0 && np < (int)sizeof pat - 1) pat[np++] = in ? '#' : '.';
    }
    pat[np] = 0;
    timing_run = best_run;
    printf("  hole %d aim %d power %d: %d of %d moments drop in (%d%%), longest window %d frames\n  %s\n", hole + 1, aim, power,
           timing_in, timing_n, timing_in * 100 / imax(1, timing_n), best_run, pat);
    fflush(stdout);
    free(cc);
}

/* the yardstick player round after round: strokes hole by hole */
static void yard_rounds(int rounds, int from, int to) {
    TshCourse *cc = malloc(sizeof *cc);
    if (!cc) return;
    int tot_all = 0, best = 999, worst = 0, hole_sum[TSH_HOLES] = {0}, hole_lost[TSH_HOLES] = {0}, par = 0;
    for (int h = from; h <= to; h++) par += TSH_HOLE[h].par;
    for (int r = 0; r < rounds; r++) {
        int tot = 0;
        printf("  round %2d:", r + 1);
        for (int h = from; h <= to; h++) {
            tsh_course_build(cc, h);
            int l = 0, s = tsh_steady_play(cc, (uint64_t)(r * 1000 + h * 7919 + 17), &l);
            hole_sum[h] += s;
            hole_lost[h] += l;
            tot += s;
            printf(" %d", s);
        }
        printf("  = %d (%+d)\n", tot, tot - par);
        fflush(stdout);
        tot_all += tot;
        best = imin(best, tot);
        worst = imax(worst, tot);
    }
    printf("  holes:");
    for (int h = from; h <= to; h++) printf(" %d:%.1f(%d lost)", h + 1, hole_sum[h] / (float)rounds, hole_lost[h]);
    printf("\n  average %.1f (%+.1f), best %d (%+d), worst %d (%+d)\n", tot_all / (float)rounds, tot_all / (float)rounds - par,
           best, best - par, worst, worst - par);
    fflush(stdout);
    free(cc);
}

static void quick_tour(int h, int n, bool card) {
    humans = n;
    for (int i = 0; i < 2; i++) pick_g[i] = i % golfers_open();
    start_tour();
    tour.hole = iclamp(h - 1, 0, TSH_HOLES - 1);
    turn = 0;
    go_card();
    if (!card) start_play();
}

static int tsh_cheat(const char *cmd) {
    int a, b, c;
    float fx, fy, fvx, fvy;
    if (sscanf(cmd, "hole2p %d", &a) == 1) { quick_tour(a, 2, false); return 1; }
    if (sscanf(cmd, "hole %d", &a) == 1) { quick_tour(a, 1, false); return 1; }
    if (sscanf(cmd, "card %d", &a) == 1) { quick_tour(a, 1, true); return 1; }
    if (sscanf(cmd, "goto %d", &a) == 1) { tour.hole = iclamp(a - 1, 0, TSH_HOLES - 1); turn = 0; go_card(); return 1; }
    if (sscanf(cmd, "turn %d", &a) == 1) { turn = iclamp(a - 1, 0, humans - 1); go_card(); start_play(); return 1; }
    int n = sscanf(cmd, "ball %f %f %f %f", &fx, &fy, &fvx, &fvy);
    if (n >= 2) {
        /* put the ball somewhere; with a speed it is in flight */
        play.x = play.sx = fx;
        play.y = play.sy = fy;
        play.vx = n >= 4 ? fvx : 0;
        play.vy = n >= 4 ? fvy : 0;
        play.phase = n >= 4 ? TP_FLIGHT : TP_AIM;
        play.phase_t = play.fly_t = 0;
        play.air_t = 10;
        play.slam_used = play.slammed = false;
        gx = play.x - 9;
        gy = play.y + TSH_R;
        return 1;
    }
    if (sscanf(cmd, "cam %d", &a) == 1) { cam_lock = a; if (a >= 0) cam = (float)a; return 1; }
    if (sscanf(cmd, "aim %d", &a) == 1) { play.aim = iclamp(a, 0, TSH_AIMS - 1); return 1; }
    if (sscanf(cmd, "strokes %d", &a) == 1) { play.strokes = a; return 1; }
    if (sscanf(cmd, "score %d %d %d", &a, &b, &c) == 3) {
        /* score PLAYER HOLE STROKES */
        if (a >= 1 && a <= 2 && b >= 1 && b <= TSH_HOLES) tour.e[a - 1].score[b - 1] = (uint8_t)c;
        return 1;
    }
    if (sscanf(cmd, "par_all %d", &a) == 1) {
        /* every hole before the current one played at par + a by the players */
        for (int i = 0; i < humans; i++)
            for (int h = 0; h < tour.hole; h++) tour.e[i].score[h] = (uint8_t)imax(1, TSH_HOLE[h].par + a);
        return 1;
    }
    if (sscanf(cmd, "solve %d %d", &a, &b) == 2) {
        /* print a route for a hole (the route table is made with this) */
        TshCourse *cc = malloc(sizeof *cc);
        if (!cc) return 1;
        tsh_course_build(cc, iclamp(a - 1, 0, TSH_HOLES - 1));
        TshShot out[TSH_ROUTE_MAX];
        int k = tsh_solve(cc, NULL, out, 8, b, true);
        printf("  /* %02d %s, par %d: %d strokes */ {", a, TSH_HOLE[cc->hole].name, cc->par, k);
        for (int i = 0; i < k; i++) printf("{%d, %d, %d}, ", out[i].aim, out[i].power, out[i].slam);
        printf("{-1, 0, 0}},\n");
        fflush(stdout);
        free(cc);
        return 1;
    }
    int lx = 0, ly = 0, lclk = 0;
    int nl = sscanf(cmd, "landscape %d %d %d %d %d %d", &a, &b, &c, &lx, &ly, &lclk);
    if (nl >= 3) {
        /* design check: where shots at one aim come to rest, power by power
         * (c = slam frame, -1 none), from the tee or from x y at a clock */
        TshCourse *cc = malloc(sizeof *cc);
        if (!cc) return 1;
        tsh_course_build(cc, iclamp(a - 1, 0, TSH_HOLES - 1));
        printf("  hole %d aim %d slam %d (cup at %d):", a, b, c, (int)cc->cup_x);
        for (int pw = 0; pw <= TSH_FILL; pw += 2) {
            TshPlay q;
            tsh_play_begin(&q, cc);
            if (nl >= 5) {
                q.x = q.sx = (float)lx;
                q.y = q.sy = (float)ly;
                q.clock = lclk;
                q.aim = b;
            }
            TshShot s = {(int8_t)b, (int8_t)pw, (int16_t)c};
            int ph = tsh_try_shot(&q, cc, &s);
            if (ph == TP_HOLED) printf(" [%d:IN]", pw);
            else if (ph == TP_LOST) printf(" [%d:x]", pw);
            else printf(" [%d:%d]", pw, (int)q.x);
        }
        printf("\n");
        free(cc);
        return 1;
    }
    if (sscanf(cmd, "survey %d", &a) == 1) {
        /* design check: how many tee shots on the whole grid drop in, with
         * and without a slam, and how many are lost */
        TshCourse *cc = malloc(sizeof *cc);
        if (!cc) return 1;
        tsh_course_build(cc, iclamp(a - 1, 0, TSH_HOLES - 1));
        int ace = 0, n0 = 0, lost = 0, sace = 0, sn = 0;
        for (int aim = 0; aim < TSH_AIMS; aim++)
            for (int pw = 0; pw <= TSH_FILL; pw++) {
                TshPlay q;
                tsh_play_begin(&q, cc);
                TshShot s = {(int8_t)aim, (int8_t)pw, -1};
                int ph = tsh_try_shot(&q, cc, &s);
                n0++;
                if (ph == TP_HOLED) ace++;
                if (ph == TP_LOST) lost++;
            }
        for (int aim = 0; aim < TSH_AIMS; aim += 2)
            for (int pw = 4; pw <= TSH_FILL; pw += 4)
                for (int sl = 8; sl <= 120; sl += 8) {
                    TshPlay q;
                    tsh_play_begin(&q, cc);
                    TshShot s = {(int8_t)aim, (int8_t)pw, (int16_t)sl};
                    sn++;
                    if (tsh_try_shot(&q, cc, &s) == TP_HOLED) sace++;
                }
        int st[4], lt = 0;
        for (int k = 0; k < 4; k++) {
            int l = 0;
            st[k] = tsh_steady_play(cc, (uint64_t)(k * 7919 + a), &l);
            lt += l;
        }
        printf("  hole %d (par %d): %d/%d tee shots drop in, %d lost; with a slam %d/%d; unsteady player %d %d %d %d (%d lost)\n", a,
               cc->par, ace, n0, lost, sace, sn, st[0], st[1], st[2], st[3], lt);
        fflush(stdout);
        free(cc);
        return 1;
    }
    if (sscanf(cmd, "sweep %d %d", &a, &b) == 2) {
        /* design check: no shot without a slam drops in (hole, how many
         * start times through the movers' round to sweep as well) */
        sweep_hole(iclamp(a - 1, 0, TSH_HOLES - 1), b, true);
        return 1;
    }
    int d = 0, e = -1, g = -1, clk = 0;
    int nt = sscanf(cmd, "trace %d %d %d %d %d %d %d", &a, &b, &c, &d, &e, &g, &clk);
    if (nt >= 4) {
        /* design check: the path of one shot (hole, aim, frames of A, slam;
         * then where from and the movers' clock, if not the tee) */
        TshCourse *cc = malloc(sizeof *cc);
        if (!cc) return 1;
        tsh_course_build(cc, iclamp(a - 1, 0, TSH_HOLES - 1));
        TshPlay q;
        tsh_play_begin(&q, cc);
        if (nt >= 6) {
            q.x = q.sx = (float)e;
            q.y = q.sy = (float)g;
            q.clock = clk;
        }
        TshShot s = {(int8_t)b, (int8_t)c, (int16_t)d};
        printf("  hole %d aim %d power %d slam %d:", a, b, c, d);
        for (int f = 0; f < 4000; f++) {
            tsh_play_step(&q, cc, tsh_bot_buttons(&q, &s));
            uint32_t fx = q.fx;
            q.fx = 0;
            if (q.phase == TP_FLIGHT && (q.fly_t % 10 == 0 || (fx & (FXT_SLAM | FXT_BUMPER | FXT_SPRING | FXT_JUNK | FXT_MOVER | FXT_FIRE | FXT_SAND))))
                printf(" %d:(%d,%d)%s%s%s%s%s%s", q.fly_t, (int)q.x, (int)q.y, fx & FXT_SLAM ? "S" : "", fx & FXT_BUMPER ? "B" : "",
                       fx & FXT_SPRING ? "T" : "", fx & (FXT_JUNK | FXT_MOVER) ? "J" : "", fx & FXT_FIRE ? "F" : "", fx & FXT_SAND ? "s" : "");
            if (q.phase == TP_HOLED) { printf(" IN\n"); break; }
            if (q.phase == TP_LOST) { printf(" LOST(%d,%d)\n", (int)q.x, (int)q.y); break; }
            if (q.phase == TP_AIM && q.strokes) { printf(" REST(%d,%d)\n", (int)q.x, (int)q.y); break; }
        }
        fflush(stdout);
        free(cc);
        return 1;
    }
    if (!strncmp(cmd, "play ", 5)) {
        /* design check: play shots from the tee (hole, then aim power slam
         * for each) and say where each one ends */
        int hole = 0, used = 0;
        const char *s = cmd + 5;
        play_in = 0;
        if (sscanf(s, "%d%n", &hole, &used) != 1) return 1;
        s += used;
        TshCourse *cc = malloc(sizeof *cc);
        if (!cc) return 1;
        tsh_course_build(cc, iclamp(hole - 1, 0, TSH_HOLES - 1));
        TshPlay q;
        tsh_play_begin(&q, cc);
        int sa, sp, ss;
        printf("  hole %d:", hole);
        while (sscanf(s, "%d %d %d%n", &sa, &sp, &ss, &used) == 3) {
            s += used;
            TshShot sh = {(int8_t)sa, (int8_t)sp, (int16_t)ss};
            int ph = tsh_try_shot(&q, cc, &sh);
            play_in = ph == TP_HOLED;
            if (ph == TP_LOST)
                for (int f = 0; f < TSH_LOST_T + 2 && q.phase == TP_LOST; f++) tsh_play_step(&q, cc, 0);
            q.fx = 0;
            printf(" [%d %d %d] %s (%d,%d) clock %d;", sa, sp, ss, ph == TP_HOLED ? "IN" : ph == TP_LOST ? "lost" : "rests",
                   (int)q.x, (int)q.y, q.clock);
            if (ph == TP_HOLED) break;
        }
        printf("\n");
        fflush(stdout);
        free(cc);
        return 1;
    }
    if (sscanf(cmd, "slamaces %d %d %d", &a, &b, &c) == 3) {
        /* design check: tee shots with a slam that drop in (hole, power step,
         * slam step), every aim */
        TshCourse *cc = malloc(sizeof *cc);
        if (!cc) return 1;
        tsh_course_build(cc, iclamp(a - 1, 0, TSH_HOLES - 1));
        int hits = 0, n = 0;
        for (int aim = 0; aim < TSH_AIMS; aim++)
            for (int pw = 0; pw <= TSH_FILL; pw += imax(1, b)) {
                TshPlay q0;
                tsh_play_begin(&q0, cc);
                TshShot s0 = {(int8_t)aim, (int8_t)pw, -1};
                if (tsh_try_shot(&q0, cc, &s0) == TP_HOLED) continue; /* in without the slam */
                for (int sl = 2; sl <= 200; sl += imax(1, c)) {
                    TshPlay q;
                    tsh_play_begin(&q, cc);
                    TshShot s = {(int8_t)aim, (int8_t)pw, (int16_t)sl};
                    n++;
                    if (tsh_try_shot(&q, cc, &s) == TP_HOLED) {
                        if (hits < 30) printf("    slam ace: aim %d, %d frames of A, slam at %d\n", aim, pw, sl);
                        hits++;
                    }
                }
            }
        printf("  hole %d: %d of %d slammed tee shots drop in\n", a, hits, n);
        fflush(stdout);
        free(cc);
        return 1;
    }
    if (sscanf(cmd, "timing %d %d %d", &a, &b, &c) == 3) {
        /* design check: one shot (hole, aim, frames of A) at every moment */
        timing_hole(iclamp(a - 1, 0, TSH_HOLES - 1), b, c);
        return 1;
    }
    if (sscanf(cmd, "yardlog %d %d %d", &a, &b, &c) == 3) {
        /* design check: the yardstick player's shots, round a, holes b to c */
        tsh_yard_log = true;
        TshCourse *cc = malloc(sizeof *cc);
        for (int h = iclamp(b - 1, 0, TSH_HOLES - 1); cc && h <= iclamp(c - 1, 0, TSH_HOLES - 1); h++) {
            tsh_course_build(cc, h);
            printf("  hole %d:\n", h + 1);
            tsh_steady_play(cc, (uint64_t)((a - 1) * 1000 + h * 7919 + 17), NULL);
        }
        free(cc);
        tsh_yard_log = false;
        fflush(stdout);
        return 1;
    }
    if (sscanf(cmd, "yard %d %d %d", &a, &b, &c) == 3) {
        /* design check: the yardstick player, a rounds over holes b to c */
        yard_rounds(a, iclamp(b - 1, 0, TSH_HOLES - 1), iclamp(c - 1, 0, TSH_HOLES - 1));
        return 1;
    }
    if (!strcmp(cmd, "save")) { save_now(); return 1; }
    if (!strcmp(cmd, "title")) { go_title(); return 1; }
    return 0;
}

const GameDef GAME_TILTSHOT = {
    "tiltshot",
    "TILTSHOT",
    "1987",
    "SPORTS",
    "PINBALL GOLF AT THE COMET CLASSIC. SWING, SLAM AND BOUNCE ROUND 18 HOLES.",
    {"SINK A HOLE IN ONE", "WIN THE COMET CLASSIC", "WIN AT EVEN PAR OR BETTER"},
    GLYPH_LEFT " " GLYPH_RIGHT "\tAIM\n"
    "HOLD " GLYPH_A "\tFILL THE METER\n"
    "LET GO\tSWING\n"
    GLYPH_A " IN THE AIR\tSLAM (ONCE A SHOT)\n"
    "START\tPAUSE\n"
    "2P KEYS: WASD F G / ARROWS K L",
    C_MAGENTA, C_LIME,
    tsh_load, tsh_start, tsh_update, tsh_draw, tsh_quit, tsh_label, tsh_query, tsh_cheat,
    "PINGOLF", 31,
    NULL,
};
