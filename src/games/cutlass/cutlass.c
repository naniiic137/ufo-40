/* CUTLASS CUP - a ball-and-blade sport on the deck of a galley.
 * Cartridge 14 of UFO 40, a tribute to Bushido Ball (UFO 50 #14).
 * See docs/games/14-cutlass-cup.md. The match rules and the CPU live in
 * cutlass_match.c; this file is menus, the tournament and drawing. */
#include "cutlass.h"
#include <ctype.h>

enum { S_TITLE, S_OPTIONS, S_SELECT, S_BRACKET, S_VS, S_MATCH, S_RESULT, S_CONTINUE, S_ENDING };
enum { MODE_TOURNEY, MODE_VERSUS, MODE_COOP };

typedef struct Save {
    uint32_t magic;
    uint8_t goal, time, laws, speed;
    uint8_t best_defeated, cups, clean_cups, pad;
} Save;
#define SAVE_MAGIC 0x43430001u

typedef struct Part {
    float x, y, vx, vy;
    int life, col, kind;
} Part;

static Save sv;
static Match M;
static Rng seeds;
static Part parts[160];
static int state, state_t, frame_t, title_sel, opt_sel;
static int mode;
static int pick[2], sel[2];
static bool picked[2];
static int opp[5], n_opp, opp_i;     /* tournament opponents (co-op: pairs) */
static int defeated, continues;
static int result_win, shake, banner_t;
static const char *banner_text;
static int banner_col;
static bool sheet_mode, demo;

static bool vita_single(void) { return plat_kind() == PLAT_VITA; }

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) {
            parts[i] = (Part){x, y, vx, vy, life, col, kind};
            return;
        }
}

/* ------------------------------------------------------------------ */
/* save                                                                 */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) {
        sv = tmp;
    } else {
        memset(&sv, 0, sizeof sv);
        sv.magic = SAVE_MAGIC;
        sv.goal = 8;
        sv.time = 0;
        sv.laws = 1;
        sv.speed = SPEED_NORMAL;
    }
    sv.goal = (uint8_t)iclamp(sv.goal, 4, 20);
    sv.time = (uint8_t)iclamp(sv.time, 0, 10);
    sv.speed = (uint8_t)iclamp(sv.speed, 0, 2);
}

/* ------------------------------------------------------------------ */
/* flow                                                                 */

static uint64_t new_seed(void) { return rng_next(&seeds) ^ ((uint64_t)rng_next(&seeds) << 32); }

static void go_title(void) {
    state = S_TITLE;
    state_t = 0;
    input_set_versus(false);
    game_set_pausable(false);
    music_play(CC_MUS_TITLE);
}

static void go_select(int m) {
    mode = m;
    state = S_SELECT;
    state_t = 0;
    picked[0] = picked[1] = false;
    sel[1] = (sel[0] + 1) % CC_FIGHTERS;
    input_set_versus(mode != MODE_TOURNEY);
    game_set_pausable(false);
    music_play(CC_MUS_SELECT);
}

static void start_match(void) {
    int f[CC_MAX_PLAYERS];
    bool cpu[CC_MAX_PLAYERS];
    int np = 2;
    if (mode == MODE_TOURNEY) {
        f[0] = pick[0];
        f[1] = opp[opp_i];
        cpu[0] = demo;
        cpu[1] = true;
    } else if (mode == MODE_VERSUS) {
        f[0] = pick[0];
        f[1] = pick[1];
        cpu[0] = cpu[1] = false;
    } else {
        np = 4;
        f[0] = pick[0];
        f[1] = pick[1];
        f[2] = opp[opp_i * 2];
        f[3] = opp[opp_i * 2 + 1];
        cpu[0] = cpu[1] = false;
        cpu[2] = cpu[3] = true;
    }
    int level = mode == MODE_TOURNEY ? opp_i : mode == MODE_COOP ? 2 + opp_i : 2;
    cc_match_init(&M, f, np, cpu, sv.goal, sv.time, sv.laws != 0, sv.speed, level, new_seed());
    memset(parts, 0, sizeof parts);
    state = S_MATCH;
    state_t = 0;
    banner_t = 0;
    input_set_versus(mode != MODE_TOURNEY);
    game_set_pausable(true);
    music_stop(); /* no music during play */
}

static void start_tourney(void) {
    /* the other five fighters, in a random order */
    n_opp = 0;
    for (int i = 0; i < CC_FIGHTERS; i++) {
        if (i == pick[0]) continue;
        if (mode == MODE_COOP && i == pick[1]) continue;
        opp[n_opp++] = i;
    }
    for (int i = n_opp - 1; i > 0; i--) {
        int j = rng_range(&seeds, 0, i);
        int t = opp[i];
        opp[i] = opp[j];
        opp[j] = t;
    }
    opp_i = 0;
    defeated = 0;
    continues = 0;
    state = S_BRACKET;
    state_t = 0;
    input_set_versus(false);
    music_play(CC_MUS_BRACKET);
}

static int matches_in_run(void) { return mode == MODE_COOP ? n_opp / 2 : n_opp; }

static void finish_match(void) {
    result_win = M.winner == 0;
    state = S_RESULT;
    state_t = 0;
    game_set_pausable(false);
    input_set_versus(false);
    if (mode != MODE_VERSUS) {
        if (result_win) {
            defeated += mode == MODE_COOP ? 2 : 1;
            if (defeated > sv.best_defeated) sv.best_defeated = (uint8_t)defeated;
            if (defeated >= 3) game_award(GOAL_BEACON); /* three opponents beaten in one run */
            opp_i++;
            if (opp_i >= matches_in_run()) {
                if (sv.cups < 255) sv.cups++;
                game_award(GOAL_SAUCER);
                if (continues == 0) {
                    if (sv.clean_cups < 255) sv.clean_cups++;
                    game_award(GOAL_ALIEN);
                }
            }
            save_now();
        }
    }
    music_restart(result_win || mode == MODE_VERSUS ? CC_MUS_WIN : CC_MUS_LOSE);
}

/* ------------------------------------------------------------------ */
/* input                                                                */

static Pad human_pad(int who) {
    Pad p;
    memset(&p, 0, sizeof p);
    bool (*h)(int) = who ? btn2 : btn;
    bool (*pr)(int) = who ? btnp2 : btnp;
    p.dx = (int8_t)(h(BTN_RIGHT) - h(BTN_LEFT));
    p.dy = (int8_t)(h(BTN_DOWN) - h(BTN_UP));
    p.roll = pr(BTN_A);
    p.strike = pr(BTN_B);
    p.held = h(BTN_B);
    return p;
}

static void fx_from_match(void) {
    uint32_t fx = M.fx;
    M.fx = 0;
    if (!fx) return;
    float x = M.fx_x, y = M.fx_y;
    if (fx & FX_SUPER) { sfx_play_name("cc_super"); shake = imax(shake, 8); }
    else if (fx & FX_SWEET) { sfx_play_name("cc_sweet"); shake = imax(shake, 4); }
    else if (fx & FX_STRIKE) sfx_play_name("cc_strike");
    if (fx & (FX_STRIKE | FX_SUPER)) for (int i = 0; i < 6; i++) part_add(x, y - 6, (i - 2.5f) * 0.5f, -0.8f - (i % 2) * 0.4f, 10, C_WHITE, 1);
    if (fx & FX_SWEET) for (int i = 0; i < 10; i++) {
        float a = (float)i / 10 * 6.283f;
        part_add(x, y - 6, cosf(a) * 1.8f, sinf(a) * 1.8f, 12, C_YELLOW, 1);
    }
    if (fx & FX_WALL) sfx_play_name("cc_wall");
    if (fx & FX_BODY) sfx_play_name("cc_body");
    if (fx & FX_ROLL) sfx_play_name("cc_roll");
    if (fx & FX_SECOND) sfx_play_name("cc_second");
    if (fx & FX_STUN) sfx_play_name("cc_stun");
    if (fx & FX_BLAST) {
        sfx_play_name("cc_blast");
        shake = imax(shake, 12);
        for (int i = 0; i < 18; i++) {
            float a = (float)i / 18 * 6.283f;
            part_add(x, y - 4, cosf(a) * 2.2f, sinf(a) * 1.5f - 0.5f, 16 + i % 5, i % 3 ? C_ORANGE : C_YELLOW, 0);
        }
    }
    if (fx & FX_CATCH) sfx_play_name("cc_catch");
    if (fx & FX_REFLECT) sfx_play_name("cc_reflect");
    if (fx & FX_METER) sfx_play_name("cc_meter");
    if (fx & FX_BLINK) sfx_play_name("cc_blink");
    if (fx & FX_SERVE) sfx_play_name("cc_serve");
    if (fx & FX_FOUL) {
        sfx_play_name("cc_whistle");
        static const char *WHY[4] = {"", "DAWDLING!", "BLADE FOUL!", "EARLY SWING!"};
        banner_text = WHY[iclamp(M.last_foul_kind, 0, 3)];
        banner_col = C_RED;
        banner_t = 70;
    }
    if (fx & FX_POINT) {
        music_restart(CC_MUS_POINT);
        float bx = M.point_team == 0 ? 316 : 4;
        if (M.point_why == 0) {
            sfx_play_name("cc_splash");
            for (int i = 0; i < 14; i++) part_add(bx, M.ball.y - 2, (M.point_team == 0 ? 1 : -1) * (0.2f + (i % 4) * 0.4f), -1.8f - (i % 3) * 0.5f, 22, i % 2 ? C_ICE : C_CYAN, 0);
        }
        banner_text = M.point_why ? "PENALTY POINT!" : "POINT!";
        banner_col = C_YELLOW;
        banner_t = 60;
    }
}

static void update_match(void) {
    Pad pads[CC_MAX_PLAYERS];
    for (int i = 0; i < M.np; i++) {
        if (M.pl[i].cpu) cc_ai_pad(&M, i, &pads[i]);
        else pads[i] = human_pad(M.pl[i].input);
    }
    int before = M.state;
    cc_match_update(&M, pads);
    fx_from_match();
    if (before != MS_OVER && M.state == MS_OVER) sfx_play_name("cc_whistle");
    if (M.state == MS_OVER && M.state_t > 100) finish_match();
    /* roll dust and sea-fog smoke */
    for (int i = 0; i < M.np; i++)
        if (M.pl[i].state == PS_ROLL && frame_t % 3 == 0) part_add(M.pl[i].x, M.pl[i].y, 0, -0.3f, 12, C_EARTH, 0);
    if (M.ball.live && M.ball.kind == BK_FOG && frame_t % 3 == 0) part_add(M.ball.x, M.ball.y - M.ball.z - 3, 0, -0.2f, 26, C_LIGHT, 2);
}

/* ------------------------------------------------------------------ */
/* menus                                                                */

static void update_title(void) {
    game_set_pausable(false);
    if (btn_repeat(BTN_UP)) { title_sel = (title_sel + 3) % 4; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { title_sel = (title_sel + 1) % 4; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) {
        if ((title_sel == 1 || title_sel == 2) && vita_single()) { sfx_play_name("ui_error"); return; }
        sfx_play_name("ui_ok");
        switch (title_sel) {
        case 0: go_select(MODE_TOURNEY); break;
        case 1: go_select(MODE_VERSUS); break;
        case 2: go_select(MODE_COOP); break;
        default: state = S_OPTIONS; state_t = 0; opt_sel = 0; break;
        }
    }
}

static void update_options(void) {
    if (btn_repeat(BTN_UP)) { opt_sel = (opt_sel + 4) % 5; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { opt_sel = (opt_sel + 1) % 5; sfx_play_name("ui_move"); }
    int d = btn_repeat(BTN_RIGHT) - btn_repeat(BTN_LEFT);
    if (d) {
        switch (opt_sel) {
        case 0: sv.goal = (uint8_t)iclamp(sv.goal + d, 4, 20); break;
        case 1: sv.time = (uint8_t)iclamp(sv.time + d, 0, 10); break;
        case 2: sv.laws ^= 1; break;
        case 3: sv.speed = (uint8_t)((sv.speed + d + 3) % 3); break;
        default: break;
        }
        sfx_play_name("ui_move");
    }
    if (btnp(BTN_B) || ((btnp(BTN_A) || btnp(BTN_START)) && opt_sel == 4)) {
        save_now();
        sfx_play_name("ui_back");
        go_title();
    }
}

static void update_select(void) {
    int players = mode == MODE_TOURNEY ? 1 : 2;
    for (int p = 0; p < players; p++) {
        bool (*rep)(int) = p ? btn_repeat2 : btn_repeat;
        bool (*pr)(int) = p ? btnp2 : btnp;
        if (!picked[p]) {
            int d = rep(BTN_RIGHT) - rep(BTN_LEFT);
            if (d) { sel[p] = (sel[p] + d + CC_FIGHTERS) % CC_FIGHTERS; sfx_play_name("ui_move"); }
            if (pr(BTN_A) && state_t > 8) {
                /* co-op teammates are different fighters */
                if (mode == MODE_COOP && picked[p ^ 1] && pick[p ^ 1] == sel[p]) { sfx_play_name("ui_error"); continue; }
                pick[p] = sel[p];
                picked[p] = true;
                sfx_play_name("ui_ok");
            }
        } else if (pr(BTN_B)) {
            picked[p] = false;
            sfx_play_name("ui_back");
        }
    }
    if (btnp(BTN_B) && !picked[0] && state_t > 8) { sfx_play_name("ui_back"); go_title(); return; }
    bool ready = picked[0] && (players == 1 || picked[1]);
    if (ready && state_t > 8) {
        if (mode == MODE_VERSUS) start_match();
        else start_tourney();
    }
}

static void update_result(void) {
    if (state_t < 60) return;
    if (!(btnp(BTN_A) || btnp(BTN_START))) return;
    sfx_play_name("ui_ok");
    if (mode == MODE_VERSUS) { go_select(MODE_VERSUS); return; }
    if (!result_win) {
        state = S_CONTINUE;
        state_t = 0;
        return;
    }
    if (opp_i >= matches_in_run()) {
        state = S_ENDING;
        state_t = 0;
        music_play(CC_MUS_CUP);
    } else {
        state = S_BRACKET;
        state_t = 0;
        music_play(CC_MUS_BRACKET);
    }
}

static void cc_update(void) {
    frame_t++;
    state_t++;
    if (shake > 0) shake--;
    if (banner_t > 0) banner_t--;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        if (p->kind == 0) p->vy += 0.12f;
    }
    switch (state) {
    case S_TITLE: update_title(); break;
    case S_OPTIONS: update_options(); break;
    case S_SELECT: update_select(); break;
    case S_BRACKET:
        if (state_t > 30 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); state = S_VS; state_t = 0; }
        if (state_t > 30 && btnp(BTN_B)) { sfx_play_name("ui_back"); go_title(); }
        break;
    case S_VS:
        if (state_t > 110 || (state_t > 30 && btnp(BTN_A))) { input_consume(); start_match(); }
        break;
    case S_MATCH: update_match(); break;
    case S_RESULT: update_result(); break;
    case S_CONTINUE:
        if (state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) {
            /* try the same match again (the Alien needs a run without this) */
            continues++;
            sfx_play_name("ui_ok");
            state = S_VS;
            state_t = 0;
        } else if (state_t > 20 && btnp(BTN_B)) {
            sfx_play_name("ui_back");
            go_title();
        }
        break;
    case S_ENDING:
        if (state_t > 180 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); go_title(); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing: the court                                                   */

static void draw_backdrop(void) {
    /* the harbour behind the top rail: sky, town, sea */
    gfx_rect(0, 16, SCREEN_W, 6, C_SKY);
    gfx_rect(0, 22, SCREEN_W, 7, C_BLUE);
    for (int i = 0; i < 12; i++) {
        int x = i * 29 + 4, h = 3 + (i * 7) % 4;
        gfx_rect(x, 22 - h, 14, h + 1, C_WHITE);
        gfx_rect(x + 4, 22 - h + 1, 2, 2, C_BLUE);
        if (i % 3 == 0) { gfx_circ(x + 7, 22 - h, 3, C_WHITE); }
    }
    for (int x = (frame_t / 8) % 6; x < SCREEN_W; x += 6) gfx_pset(x, 25 + (x / 6) % 3, C_CYAN);
}

static void draw_deck(void) {
    /* the sea at both ends: a ball that goes there is a point */
    gfx_rect(0, 29, SCREEN_W, 151, C_BLUE);
    for (int y = 30; y < 180; y += 5)
        for (int x = ((y / 5) % 2) * 3 + (frame_t / 10) % 6; x < SCREEN_W; x += 12) {
            if (x < 6 || x > 313) gfx_hline(x, x + 2, y, C_SKY);
        }
    /* planks: long boards with a little grain */
    gfx_rect(6, CC_TOP - 4, 308, CC_BOT - CC_TOP + 10, C_TAN);
    for (int y = CC_TOP - 4; y < CC_BOT + 6; y += 10) {
        int row = (y - CC_TOP) / 10;
        if (row % 2) gfx_dither(6, y, 308, 10, C_EARTH, 1);
        gfx_hline(6, 313, y + 9, C_BROWN);
        for (int x = 6 + (row * 53) % 90; x < 314; x += 96) {
            gfx_vline(x, y, y + 8, C_BROWN);
            gfx_pset(x - 2, y + 2, C_BROWN);
            gfx_pset(x + 2, y + 6, C_BROWN);
        }
        for (int x = 10 + (row * 29) % 37; x < 310; x += 41) gfx_hline(x, x + 5, y + 3 + row % 4, C_EARTH);
    }
    gfx_vline(6, CC_TOP - 4, CC_BOT + 5, C_BROWN);
    gfx_vline(313, CC_TOP - 4, CC_BOT + 5, C_BROWN);
    /* painted lines: centre, reach lines and starting circles */
    gfx_vline(CC_CENTER, CC_TOP - 3, CC_BOT + 4, C_WHITE);
    for (int y = CC_TOP - 2; y < CC_BOT + 4; y += 4) {
        gfx_vline(CC_CENTER - CC_REACH, y, y + 1, C_HIDE);
        gfx_vline(CC_CENTER + CC_REACH, y, y + 1, C_HIDE);
    }
    for (int i = 0; i < M.np || i < 2; i++) {
        const Player *p = &M.pl[i];
        int cx = p->team == 0 ? CC_CIRCLE_L : CC_CIRCLE_R;
        int cy = (int)p->home_y;
        gfx_circb(cx, cy, 11, C_WHITE);
    }
}

static void draw_rail(int y, bool front) {
    gfx_rect(0, y, SCREEN_W, 5, C_BROWN);
    gfx_hline(0, SCREEN_W - 1, y, C_TAN);
    gfx_hline(0, SCREEN_W - 1, y + 4, C_INK);
    for (int x = 10; x < SCREEN_W; x += 26) {
        gfx_rect(x, y + (front ? 5 : -4), 3, front ? 6 : 4, C_BROWN);
        gfx_pset(x + 1, y + 1, C_EARTH);
    }
}

/* where the weapon hand is on each pose, and which way the weapon points */
static const int16_t HAND[POSE_COUNT][3] = {
    /* x, y, angle (degrees, 0 = forward, 90 = down) */
    {12, 13, 50}, {12, 14, 55}, {12, 13, 40}, {12, 13, 60}, {2, 10, -130}, {13, 13, 5}, {0, 0, 0}, {11, 15, 95},
    {0, 0, 0}, {12, 10, -80},
};

static void wpx(int x0, int y0, float lx, float ly, int scale, int col) {
    gfx_rect(x0 + (int)floorf(lx * scale), y0 + (int)floorf(ly * scale), scale, scale, col);
}

/* the weapon in the hand, drawn live so it can swing; scale 2 for menus */
static void draw_weapon_s(int f, int pose, int x0, int y0, int flip, bool charging, int scale) {
    if (pose == POSE_ROLL || pose == POSE_CATCH) return;
    float hx = HAND[pose][0], hy = HAND[pose][1];
    float a = HAND[pose][2] * 3.14159f / 180.0f;
    float dx = cosf(a), dy = sinf(a);
    if (flip) { hx = 15 - hx; dx = -dx; }
    float nx = -dy, ny = dx; /* across the blade */
    int len = f == F_GRETA ? 18 : f == F_BRUNO ? 16 : f == F_SILAS ? 13 : f == F_FINN ? 11 : f == F_WREN ? 8 : 7;
    int blade = charging ? ((frame_t / 3) % 2 ? C_YELLOW : C_WHITE) : C_LIGHT;
    int edge = charging ? C_ORANGE : C_GREY;
    if (f == F_GRETA || f == F_BRUNO) {
        for (int i = -2; i <= len; i++) {
            wpx(x0, y0, hx + dx * i, hy + dy * i, scale, C_BROWN);
            if (i % 2 == 0) wpx(x0, y0, hx + dx * i + nx * 0.6f, hy + dy * i + ny * 0.6f, scale, C_TAN);
        }
        float ex = hx + dx * len, ey = hy + dy * len;
        if (f == F_GRETA) {
            /* a hook */
            for (int i = 0; i <= 3; i++) wpx(x0, y0, ex + dx * i, ey + dy * i, scale, blade);
            for (int i = 1; i <= 3; i++) wpx(x0, y0, ex + dx * 3 - nx * i, ey + dy * 3 - ny * i, scale, blade);
            wpx(x0, y0, ex + dx * 3, ey + dy * 3, scale, C_WHITE);
        } else {
            /* three tines */
            for (int k = -1; k <= 1; k++) {
                for (int i = 0; i <= 3; i++) wpx(x0, y0, ex + nx * 2 * k + dx * i, ey + ny * 2 * k + dy * i, scale, blade);
                wpx(x0, y0, ex + nx * 2 * k + dx * 3, ey + ny * 2 * k + dy * 3, scale, C_WHITE);
            }
            for (int k = -2; k <= 2; k++) wpx(x0, y0, ex + nx * k, ey + ny * k, scale, edge);
        }
        return;
    }
    /* a blade: the hilt and guard, then a (curved) edge */
    wpx(x0, y0, hx - dx, hy - dy, scale, C_BROWN);
    wpx(x0, y0, hx, hy, scale, C_AMBER);
    wpx(x0, y0, hx + nx, hy + ny, scale, C_AMBER);
    wpx(x0, y0, hx - nx, hy - ny, scale, C_AMBER);
    int bc = f == F_WREN ? C_SLATE : blade;
    for (int i = 1; i <= len; i++) {
        float bend = (f == F_FINN || f == F_SILAS) ? (float)(i * i) / (len * 5.0f) : 0;
        float px = hx + dx * i - nx * bend, py = hy + dy * i - ny * bend;
        wpx(x0, y0, px, py, scale, i == len ? C_WHITE : bc);
        if (i < len - 1 && f != F_MAE && f != F_WREN) wpx(x0, y0, px + nx * 0.8f, py + ny * 0.8f, scale, edge);
    }
}

static void draw_weapon(int f, int pose, int x0, int y0, int flip, bool charging) {
    draw_weapon_s(f, pose, x0, y0, flip, charging, 1);
}

static int pose_of(const Player *p) {
    switch (p->state) {
    case PS_RUN: return (p->t / 6) % 2 ? POSE_RUN1 : POSE_RUN0;
    case PS_SWING: return p->t < 3 ? POSE_WIND : POSE_STRIKE;
    case PS_ROLL: return POSE_ROLL;
    case PS_STUN: return POSE_STUN;
    case PS_CATCH: return POSE_CATCH;
    case PS_CHARGE: return POSE_WIND;
    case PS_WIN: return POSE_WIN;
    case PS_LOSE: return POSE_STUN;
    default: return (frame_t / 24) % 2 ? POSE_IDLE1 : POSE_IDLE0;
    }
}

static void draw_player(int i) {
    const Player *p = &M.pl[i];
    int pose = pose_of(p);
    int flip = p->face < 0;
    const Sprite *s = &cc_fighter_spr[p->fighter][pose];
    int x0 = (int)p->x - 8, y0 = (int)p->y - 21;
    if (pose == POSE_ROLL) y0 += 2;
    /* shadow */
    gfx_dither((int)p->x - 6, (int)p->y - 1, 12, 3, C_BROWN, 10);
    bool charging = p->state == PS_CHARGE && p->charge >= 12;
    if (charging) spr_draw_outline(s, x0, y0, flip ? SPR_FLIPX : 0, (frame_t / 2) % 2 ? C_YELLOW : C_ORANGE);
    else if (p->state == PS_STUN && (frame_t / 3) % 2) spr_draw_ex(s, x0, y0, flip ? SPR_FLIPX : 0, NULL, C_WHITE);
    else spr_draw(s, x0, y0, flip ? SPR_FLIPX : 0);
    if (pose == POSE_ROLL) spr_draw_ex(s, x0, y0, flip ? SPR_FLIPX : 0, NULL, -1);
    draw_weapon(p->fighter, pose, x0, y0, flip, charging);
    if (p->state == PS_STUN)
        for (int k = 0; k < 3; k++) {
            float a = frame_t * 0.2f + k * 2.1f;
            gfx_pset((int)(p->x + cosf(a) * 6), (int)(p->y - 23 + sinf(a) * 2), C_YELLOW);
        }
    if (p->state == PS_CATCH && !p->cpu && (frame_t / 6) % 2) {
        gfx_rect((int)p->x - 17, (int)p->y - 34, 34, 8, C_INK);
        tiny_center("MASH " GLYPH_B "!", (int)p->x, (int)p->y - 33, C_YELLOW);
    }
}

static void draw_ball_at(const Ball *b, bool fake) {
    if (!b->live) return;
    int x = (int)b->x, y = (int)b->y, z = (int)b->z;
    gfx_dither(x - 3, y - 1, 7, 3, C_BROWN, 12);
    if (b->kind == BK_FOG && !fake) return; /* invisible: only its smoke shows */
    if (b->kind != BK_NORMAL) { /* every Super Ball (and Mirage fake) is bright orange */
        int c = C_ORANGE;
        for (int k = 1; k <= 4; k++)
            gfx_dither(x - (int)(b->vx * k * 1.2f) - 2, y - z - 5 - (int)(b->vy * k * 1.2f), 5, 5, c, 12 - k * 2);
        gfx_circb(x, y - z - 3, 5 + (frame_t / 2) % 2, c);
    }
    spr_draw(&cc_spr[CS_BALL], x - 3, y - z - 6, 0);
}

static void draw_proj(const Proj *r) {
    int x = (int)r->x, y = (int)r->y, z = (int)r->z;
    int flip = r->vx < 0 ? SPR_FLIPX : 0;
    switch (r->kind) {
    case PR_COIN:
        spr_draw(&cc_spr[(frame_t / 4) % 2 ? CS_COIN1 : CS_COIN2], x - 2, y - 10, 0);
        break;
    case PR_DART: spr_draw(&cc_spr[CS_DART], x - 3, y - 10, flip); break;
    case PR_URCHIN:
        gfx_dither(x - 3, y - 1, 7, 3, C_BROWN, 10);
        spr_draw(&cc_spr[CS_URCHIN], x - 3, y - z - 7, 0);
        break;
    case PR_BOMB:
        gfx_dither(x - 3, y - 1, 7, 3, C_BROWN, 10);
        spr_draw(&cc_spr[CS_BOMB], x - 3, y - z - 8, 0);
        if (r->z <= 0 && r->life < 40 && (frame_t / 3) % 2) gfx_pset(x + 1, y - 9, C_WHITE);
        break;
    case PR_HOOK: {
        const Player *o = &M.pl[r->owner];
        int hx = (int)o->x + o->face * 6, hy = (int)o->y - 8;
        gfx_line(hx, hy, x, y - 8, C_CREAM);
        gfx_line(x, y - 8, x - o->face * 3, y - 11, C_LIGHT);
        gfx_line(x, y - 8, x - o->face * 3, y - 5, C_LIGHT);
        break;
    }
    case PR_SLASH:
        for (int k = 0; k < 3; k++) {
            int sx = x - (int)(r->vx * k * 2);
            gfx_circb(sx, y - 8, 6 - k, k == 0 ? C_WHITE : C_ICE);
            gfx_rect(sx - (r->vx > 0 ? 7 : -1), y - 16, 7, 16, C_TAN); /* half a circle: a crescent */
        }
        break;
    case PR_DECOY: spr_draw(&cc_spr[CS_DECOY], x - 4, y - 10, 0); break;
    case PR_BLAST: {
        /* a puff of smoke and fire that fills the blast's reach */
        int rr = imin(22, 6 + r->t * 3);
        gfx_dither_circle(x, y - 4, rr, r->t < 6 ? C_YELLOW : C_ORANGE, imax(2, 12 - r->t / 2));
        gfx_dither_circle(x, y - 4, rr * 2 / 3, r->t < 8 ? C_WHITE : C_SLATE, imax(1, 10 - r->t / 2));
        gfx_circb(x, y - 4, rr, r->t < 10 ? C_YELLOW : C_ORANGE);
        if (r->t < 5) gfx_circ(x, y - 4, 7 - r->t, C_WHITE);
        break;
    }
    default: break;
    }
}

static void draw_hud_side(int team, int x0) {
    bool left = team == 0;
    int fx = -1;
    for (int i = 0; i < M.np; i++)
        if (M.pl[i].team == team) { fx = i; break; }
    if (fx < 0) return;
    /* score */
    char sc[8];
    snprintf(sc, sizeof sc, "%d", M.score[team]);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    int sx = left ? 140 - ui_fancy_width(sc, 2) : 182;
    ui_fancy_text(sc, sx, 1, 2, grad, 3, C_INK, -1);
    /* meters: three bars of two halves, one row per player of the side */
    int row = 0;
    for (int i = 0; i < M.np; i++) {
        const Player *p = &M.pl[i];
        if (p->team != team) continue;
        int by = M.np == 4 ? 3 + row * 6 : 9;
        for (int b = 0; b < 3; b++) {
            int bx = left ? x0 + b * 17 : x0 - 16 - b * 17;
            gfx_rect(bx, by, 16, 5, C_INK);
            for (int h = 0; h < 2; h++) {
                bool on = p->meter > b * 2 + h;
                int c = on ? (p->meter >= (b + 1) * 2 ? C_ORANGE : C_AMBER) : C_DUSK;
                gfx_rect(bx + 1 + h * 7, by + 1, 7, 3, c);
            }
        }
        row++;
    }
}

/* fouls: red marks on the bottom rail, each side's beside the judge */
static void draw_fouls(void) {
    for (int team = 0; team < 2; team++)
        for (int k = 0; k < 3; k++) {
            bool on = M.fouls[team] > k;
            int x = team == 0 ? 146 - k * 5 : 170 + k * 5;
            gfx_rect(x, CC_BOT + 7, 4, 4, on ? C_RED : C_BROWN);
            if (on) gfx_pset(x + 1, CC_BOT + 7, C_PINK);
        }
}

static void draw_hud(void) {
    gfx_rect(0, 0, SCREEN_W, 16, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 15, C_DUSK);
    draw_hud_side(0, 4);
    draw_hud_side(1, SCREEN_W - 4);
    /* the clock, only when a time limit is set */
    if (M.time_limit > 0) {
        char buf[16];
        int s = (M.time_left + 59) / 60;
        snprintf(buf, sizeof buf, "%d:%02d", s / 60, s % 60);
        tiny_center(buf, 160, 10, M.sudden ? C_RED : C_GREY);
    }
    tiny_center("-", 160, 3, C_LIGHT);
}

static void draw_court(void) {
    int sx = 0, sy = 0;
    if (shake > 0) { sx = (frame_t % 3) - 1; sy = ((frame_t / 2) % 3) - 1; }
    gfx_cls(C_NIGHT);
    gfx_camera(sx, sy);
    draw_backdrop();
    draw_deck();
    draw_rail(CC_TOP - 9, false);
    spr_draw(&cc_spr[(frame_t / 40) % 2 ? CS_GULL1 : CS_GULL2], 40, CC_TOP - 14, 0);
    spr_draw(&cc_spr[(frame_t / 50) % 2 ? CS_GULL1 : CS_GULL2], 270, CC_TOP - 14, SPR_FLIPX);
    /* everything on the deck, back to front */
    int order[CC_MAX_PLAYERS + 1 + CC_MAX_PROJ + CC_MAX_FAKES];
    float key[CC_MAX_PLAYERS + 1 + CC_MAX_PROJ + CC_MAX_FAKES];
    int n = 0;
    for (int i = 0; i < M.np; i++) { order[n] = i; key[n++] = M.pl[i].y; }
    if (M.ball.live) { order[n] = 100; key[n++] = M.ball.y + 0.5f; }
    for (int i = 0; i < CC_MAX_FAKES; i++)
        if (M.fake[i].live) { order[n] = 200 + i; key[n++] = M.fake[i].y + 0.5f; }
    for (int i = 0; i < CC_MAX_PROJ; i++)
        if (M.pr[i].live) { order[n] = 300 + i; key[n++] = M.pr[i].y + 0.2f; }
    for (int a = 1; a < n; a++)
        for (int b = a; b > 0 && key[b - 1] > key[b]; b--) {
            float tk = key[b]; key[b] = key[b - 1]; key[b - 1] = tk;
            int to = order[b]; order[b] = order[b - 1]; order[b - 1] = to;
        }
    for (int k = 0; k < n; k++) {
        int o = order[k];
        if (o < 100) draw_player(o);
        else if (o == 100) draw_ball_at(&M.ball, false);
        else if (o < 300) draw_ball_at(&M.fake[o - 200], true);
        else draw_proj(&M.pr[o - 300]);
    }
    /* particles */
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        if (p->kind == 2) gfx_dither_circle((int)p->x, (int)p->y, 2 + (26 - p->life) / 6, p->col, p->life / 3 + 2);
        else gfx_rect((int)p->x, (int)p->y, 2, 2, p->col);
    }
    /* the judge at the bottom middle, who rolls the ball out */
    bool tossing = M.state == MS_SERVE && M.state_t > 20;
    spr_draw(&cc_spr[tossing ? CS_JUDGE2 : CS_JUDGE1], 152, CC_BOT - 4, 0);
    draw_rail(CC_BOT + 6, true);
    draw_fouls();
    gfx_camera(0, 0);
    draw_hud();
    /* banners */
    if (M.state == MS_READY) {
        static const uint8_t g[] = {C_WHITE, C_CREAM, C_YELLOW};
        ui_fancy_center(M.state_t < 50 ? "READY..." : "PLAY!", 160, 80, 2, g, 3, C_INK, C_BROWN);
    }
    if (banner_t > 0 && banner_text) {
        static const uint8_t gy[] = {C_WHITE, C_YELLOW, C_AMBER};
        static const uint8_t gr[] = {C_PINK, C_RED, C_WINE};
        ui_fancy_center(banner_text, 160, 74, 2, banner_col == C_RED ? gr : gy, 3, C_INK, C_INK);
    }
    if (M.state == MS_OVER) {
        static const uint8_t g[] = {C_WHITE, C_CREAM, C_YELLOW};
        char buf[40];
        if (mode == MODE_VERSUS) snprintf(buf, sizeof buf, "PLAYER %d WINS!", M.winner + 1);
        else snprintf(buf, sizeof buf, M.winner == 0 ? "YOU WIN!" : "YOU LOSE");
        gfx_dither(0, 60, SCREEN_W, 40, C_INK, 10);
        ui_fancy_center(buf, 160, 72, 2, g, 3, C_INK, C_BROWN);
    }
}

/* ------------------------------------------------------------------ */
/* drawing: menus                                                       */

static void draw_fighter_big(int f, int x, int y, int flip, int pose) {
    const Sprite *s = &cc_fighter_spr[f][pose];
    spr_draw_scaled(s, x, y, 2, flip ? SPR_FLIPX : 0);
    draw_weapon_s(f, pose, x, y, flip, false, 2);
}

static void draw_sea_sky(void) {
    for (int y = 0; y < 100; y++) gfx_hline(0, SCREEN_W - 1, y, y < 40 ? C_SKY : y < 70 ? C_CYAN : C_ICE);
    gfx_dither(0, 36, SCREEN_W, 6, C_CYAN, 6);
    gfx_dither(0, 66, SCREEN_W, 6, C_ICE, 6);
    /* the whitewashed town */
    for (int i = 0; i < 14; i++) {
        int x = i * 24 - 6, h = 14 + (i * 13) % 18;
        gfx_rect(x, 100 - h, 20, h, C_WHITE);
        gfx_rect(x + 6, 100 - h + 5, 4, 6, C_BLUE);
        if (i % 4 == 1) gfx_circ(x + 10, 100 - h, 7, C_WHITE);
        if (i % 4 == 1) gfx_circb(x + 10, 100 - h, 7, C_LIGHT);
    }
    gfx_rect(0, 100, SCREEN_W, 80, C_BLUE);
    for (int y = 104; y < 180; y += 6)
        for (int x = ((y / 6) % 2) * 8 + (frame_t / 8) % 16; x < SCREEN_W; x += 16) gfx_hline(x, x + 4, y, C_SKY);
}

static void draw_night_sea(void) {
    gfx_cls(C_NAVY);
    for (int i = 0; i < 40; i++) {
        uint32_t h = (uint32_t)(i * 2654435761u);
        gfx_pset((int)(h % 320), (int)((h >> 12) % 70), (frame_t / 20 + i) % 6 ? C_BLUE : C_ICE);
    }
    for (int i = 0; i < 14; i++) {
        int x = i * 24 - 6, h = 10 + (i * 13) % 14;
        gfx_rect(x, 84 - h, 20, h, C_NIGHT);
        if ((i * 7) % 3 == 0) gfx_rect(x + 7, 84 - h + 4, 3, 3, C_AMBER);
    }
    gfx_rect(0, 84, SCREEN_W, 96, C_NIGHT);
    for (int y = 88; y < 180; y += 6)
        for (int x = ((y / 6) % 2) * 8 + (frame_t / 10) % 16; x < SCREEN_W; x += 16) gfx_hline(x, x + 4, y, C_NAVY);
}

static void draw_title(void) {
    draw_sea_sky();
    /* the galley's deck in the foreground */
    gfx_rect(0, 134, SCREEN_W, 46, C_TAN);
    for (int y = 134; y < 180; y += 8) gfx_hline(0, SCREEN_W - 1, y, C_BROWN);
    draw_rail(130, false);
    int bob = (frame_t / 16) % 2;
    draw_fighter_big(F_FINN, 40, 128 - 44 + bob, 0, (frame_t / 30) % 2 ? POSE_STRIKE : POSE_WIND);
    draw_fighter_big(F_MAE, 248, 128 - 44 + bob, 1, POSE_IDLE0);
    spr_draw(&cc_spr[CS_BALL], 110 + (int)(sinf(frame_t * 0.05f) * 40), 100 - (int)(fabsf(sinf(frame_t * 0.05f)) * 30), 0);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_center("CUTLASS CUP", 160, 8, 3, grad, 4, C_INK, C_NAVY);
    text_center("BLADES, A BALL AND THE OPEN SEA", 160, 36, C_NAVY);
    static const char *items[4] = {"1P TOURNAMENT", "2P VERSUS", "2P CO-OP", "OPTIONS"};
    ui_panel(100, 50, 120, 58, C_NIGHT, C_AMBER);
    for (int i = 0; i < 4; i++) {
        int y = 55 + i * 12;
        bool s = i == title_sel;
        int col = s ? C_WHITE : C_GREY;
        if ((i == 1 || i == 2) && vita_single()) col = s ? C_GREY : C_SLATE;
        text_center(items[i], 160, y, col);
        if (s) ui_cursor(160 - text_width(items[i]) / 2 - 10, y, frame_t);
    }
    if ((title_sel == 1 || title_sel == 2) && vita_single()) {
        gfx_rect(80, 168, 160, 10, C_INK);
        tiny_center("NEEDS TWO CONTROLLERS", 160, 170, C_LIGHT);
    }
}

static void draw_options(void) {
    draw_night_sea();
    ui_panel(70, 30, 180, 120, C_NIGHT, C_AMBER);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center("OPTIONS", 160, 38, 2, grad, 3, C_INK, C_NAVY);
    static const char *SPEED[3] = {"CALM", "BRISK", "GALE"};
    char val[5][24];
    snprintf(val[0], 24, "%d", sv.goal);
    if (sv.time) snprintf(val[1], 24, "%d MIN", sv.time);
    else snprintf(val[1], 24, "NONE");
    snprintf(val[2], 24, "%s", sv.laws ? "ON" : "OFF");
    snprintf(val[3], 24, "%s", SPEED[sv.speed]);
    val[4][0] = 0;
    static const char *NAMES[5] = {"POINTS", "TIME", "FOULS", "SPEED", "DONE"};
    for (int i = 0; i < 5; i++) {
        int y = 64 + i * 14;
        bool s = i == opt_sel;
        text_draw(NAMES[i], 92, y, s ? C_WHITE : C_GREY);
        if (i < 4) {
            char buf[140];
            snprintf(buf, sizeof buf, GLYPH_LEFT " %s " GLYPH_RIGHT, val[i]);
            text_draw(buf, 160, y, s ? C_YELLOW : C_SLATE);
        }
        if (s) ui_cursor(82, y, frame_t);
    }
}

static void draw_stat(int x, int y, const char *name, int v) {
    tiny_draw(name, x, y, C_GREY);
    for (int k = 0; k < 3; k++) gfx_rect(x + 30 + k * 7, y, 6, 5, k < v ? C_YELLOW : C_DUSK);
}

static void draw_details(int f, int x, int y, int col) {
    const FighterDef *d = &CC_FIGHTER[f];
    text_draw(d->name, x, y, col);
    tiny_draw(d->title, x, y + 10, C_GREY);
    draw_stat(x, y + 20, "SPEED", d->speed);
    draw_stat(x, y + 28, "CONTROL", d->control);
    draw_stat(x, y + 36, "POWER", d->power);
}

static void draw_select(void) {
    draw_night_sea();
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center("CHOOSE YOUR CREW", 160, 4, 2, grad, 3, C_INK, C_NAVY);
    for (int i = 0; i < CC_FIGHTERS; i++) {
        int x = 8 + i * 51, y = 26;
        bool s0 = sel[0] == i, s1 = mode != MODE_TOURNEY && sel[1] == i;
        ui_panel(x, y, 49, 64, C_NIGHT, s0 ? C_YELLOW : s1 ? C_MAGENTA : C_DUSK);
        int pose = (s0 && picked[0]) || (s1 && picked[1]) ? POSE_WIN : ((frame_t / 20 + i) % 2 ? POSE_IDLE1 : POSE_IDLE0);
        gfx_clip(x + 1, y + 1, 47, 62);
        draw_fighter_big(i, x + 8, y + 6, 0, pose);
        gfx_noclip();
        tiny_center(CC_FIGHTER[i].name, x + 25, y + 55, C_LIGHT);
        if (s0) tiny_draw("1P", x + 3, y + 3, C_YELLOW);
        if (s1) tiny_draw("2P", x + 38, y + 3, C_MAGENTA);
    }
    if (mode == MODE_TOURNEY) {
        ui_panel(8, 96, 304, 60, C_NIGHT, C_YELLOW);
        draw_details(sel[0], 16, 102, C_YELLOW);
    } else {
        ui_panel(8, 96, 150, 60, C_NIGHT, C_YELLOW);
        ui_panel(162, 96, 150, 60, C_NIGHT, C_MAGENTA);
        draw_details(sel[0], 14, 102, C_YELLOW);
        draw_details(sel[1], 168, 102, C_MAGENTA);
        if (picked[0]) text_draw(GLYPH_CHECK, 144, 100, C_LIME);
        if (picked[1]) text_draw(GLYPH_CHECK, 298, 100, C_LIME);
    }
}

static void draw_bracket(void) {
    draw_night_sea();
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center("THE ROAD TO THE CUP", 160, 4, 2, grad, 3, C_INK, C_NAVY);
    /* your crew on the left */
    ui_panel(8, 30, 70, mode == MODE_COOP ? 130 : 70, C_NIGHT, C_YELLOW);
    draw_fighter_big(pick[0], 22, 34, 0, POSE_IDLE0);
    text_center(CC_FIGHTER[pick[0]].name, 43, 88, C_YELLOW);
    if (mode == MODE_COOP) {
        draw_fighter_big(pick[1], 22, 96, 0, POSE_IDLE0);
        text_center(CC_FIGHTER[pick[1]].name, 43, 148, C_MAGENTA);
    }
    /* the opponents */
    int rows = matches_in_run();
    for (int r = 0; r < rows; r++) {
        int y = 30 + r * 26;
        bool done = r < opp_i, next = r == opp_i;
        ui_panel(90, y, 200, 22, C_NIGHT, next ? C_WHITE : done ? C_DUSK : C_SLATE);
        char buf[32];
        snprintf(buf, sizeof buf, "MATCH %d", r + 1);
        tiny_draw(buf, 96, y + 8, next ? C_YELLOW : C_GREY);
        for (int k = 0; k < (mode == MODE_COOP ? 2 : 1); k++) {
            int f = mode == MODE_COOP ? opp[r * 2 + k] : opp[r];
            spr_draw(&cc_fighter_spr[f][POSE_IDLE0], 140 + k * 70, y + 1, SPR_FLIPX);
            text_draw(CC_FIGHTER[f].name, 158 + k * 70, y + 7, done ? C_SLATE : C_LIGHT);
            if (done) gfx_line(138 + k * 70, y + 11, 200 + k * 70, y + 11, C_RED);
        }
        if (next) ui_cursor(82, y + 7, frame_t);
    }
    if (state_t > 30 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A " TO PLAY", 160, 166, C_WHITE);
}

static void draw_vs(void) {
    gfx_cls(C_NAVY);
    for (int i = 0; i < 20; i++) {
        int y = (i * 37 + frame_t * 3) % 200 - 10;
        gfx_hline(0, SCREEN_W - 1, y, C_BLUE);
    }
    int t = imin(state_t, 20);
    int f0 = pick[0], f1 = mode == MODE_VERSUS ? pick[1] : opp[mode == MODE_COOP ? opp_i * 2 : opp_i];
    draw_fighter_big(f0, -30 + t * 4, 60, 0, POSE_WIND);
    draw_fighter_big(f1, 318 - t * 4, 60, 1, POSE_WIND);
    if (mode == MODE_COOP) {
        draw_fighter_big(pick[1], -50 + t * 4, 100, 0, POSE_IDLE0);
        draw_fighter_big(opp[opp_i * 2 + 1], 338 - t * 4, 100, 1, POSE_IDLE0);
    }
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_ORANGE};
    if (state_t > 20) ui_fancy_center("VS", 160, 70, 3, grad, 3, C_INK, C_RED);
    if (mode == MODE_COOP) {
        char a[32], b[32];
        snprintf(a, sizeof a, "%s & %s", CC_FIGHTER[f0].name, CC_FIGHTER[pick[1]].name);
        snprintf(b, sizeof b, "%s & %s", CC_FIGHTER[f1].name, CC_FIGHTER[opp[opp_i * 2 + 1]].name);
        gfx_rect(0, 160, SCREEN_W, 14, C_NAVY);
        text_center(a, 80, 163, C_YELLOW);
        text_center(b, 240, 163, C_MAGENTA);
    } else {
        text_center(CC_FIGHTER[f0].name, 70, 150, C_YELLOW);
        text_center(CC_FIGHTER[f1].name, 250, 150, C_MAGENTA);
    }
    if (mode != MODE_VERSUS) {
        char buf[32];
        snprintf(buf, sizeof buf, "MATCH %d OF %d", opp_i + 1, matches_in_run());
        text_center(buf, 160, 20, C_LIGHT);
    }
}

static void draw_result(void) {
    draw_court();
    gfx_dither(0, 16, SCREEN_W, 164, C_INK, 9);
    ui_panel(60, 40, 200, 100, C_NIGHT, result_win || mode == MODE_VERSUS ? C_YELLOW : C_RED);
    static const uint8_t gw[] = {C_WHITE, C_CREAM, C_YELLOW};
    static const uint8_t gl[] = {C_PINK, C_RED, C_WINE};
    char buf[48];
    if (mode == MODE_VERSUS) snprintf(buf, sizeof buf, "PLAYER %d WINS", M.winner + 1);
    else snprintf(buf, sizeof buf, result_win ? "VICTORY!" : "DEFEATED");
    ui_fancy_center(buf, 160, 48, 2, result_win || mode == MODE_VERSUS ? gw : gl, 3, C_INK, C_INK);
    snprintf(buf, sizeof buf, "%d - %d", M.score[0], M.score[1]);
    text_center(buf, 160, 72, C_WHITE);
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 124, C_WHITE);
}

static void draw_continue(void) {
    gfx_cls(C_NIGHT);
    static const uint8_t g[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center("CONTINUE?", 160, 60, 2, g, 3, C_INK, C_NAVY);
    draw_fighter_big(pick[0], 140, 96, 0, POSE_STUN);
}

static void draw_ending(void) {
    draw_sea_sky();
    gfx_rect(0, 134, SCREEN_W, 46, C_TAN);
    for (int y = 134; y < 180; y += 8) gfx_hline(0, SCREEN_W - 1, y, C_BROWN);
    int bob = (frame_t / 12) % 2;
    draw_fighter_big(pick[0], 120, 84 - bob, 0, POSE_WIN);
    if (mode == MODE_COOP) draw_fighter_big(pick[1], 164, 84 - bob, 1, POSE_WIN);
    spr_draw_scaled(&cc_spr[CS_CUP], mode == MODE_COOP ? 146 : 150, 50 - bob * 2, 2, 0);
    for (int i = 0; i < 16; i++) {
        int x = (i * 53 + frame_t) % SCREEN_W, y = (i * 29 + frame_t * (1 + i % 3)) % 130;
        gfx_rect(x, y, 2, 3, i % 3 == 0 ? C_RED : i % 3 == 1 ? C_YELLOW : C_WHITE);
    }
    ui_panel(40, 140, 240, 34, C_NIGHT, C_AMBER);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center("THE CUTLASS CUP IS YOURS!", 160, 146, 1, grad, 3, C_INK, -1);
    tiny_center(continues == 0 ? "NOT ONE CONTINUE. THE HARBOUR SALUTES YOU." : "THE HARBOUR SALUTES YOU.", 160, 160, C_LIGHT);
}

static void draw_sheet(void) {
    gfx_cls(C_TAN);
    int x = 2, y = 2;
    for (int i = 0; i < CS_SPRITE_COUNT; i++) {
        const Sprite *s = &cc_spr[i];
        if (x + s->w > 318) { x = 2; y += 24; }
        spr_draw(s, x, y, 0);
        x += s->w + 3;
    }
    for (int f = 0; f < CC_FIGHTERS; f++)
        for (int p = 0; p < POSE_COUNT; p++) {
            int px = 2 + p * 30, py = 64 + f * 19;
            spr_draw(&cc_fighter_spr[f][p], px, py - 4, 0);
            draw_weapon(f, p, px, py - 4, 0, false);
        }
}

static void cc_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_OPTIONS: draw_options(); break;
    case S_SELECT: draw_select(); break;
    case S_BRACKET: draw_bracket(); break;
    case S_VS: draw_vs(); break;
    case S_MATCH: draw_court(); break;
    case S_RESULT: draw_result(); break;
    case S_CONTINUE: draw_continue(); break;
    case S_ENDING: draw_ending(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void cc_load(void) {
    cc_art_load();
    cc_audio_load();
}

static void cc_start(void) {
    rng_seed(&seeds, g_rng.state ^ 0xC0C0Aull);
    load_save();
    memset(parts, 0, sizeof parts);
    memset(&M, 0, sizeof M);
    sheet_mode = false;
    demo = false;
    title_sel = 0;
    sel[0] = 0;
    sel[1] = 1;
    go_title();
}

static void cc_quit(void) {
    input_set_versus(false);
    save_now();
}

static void cc_label(int x, int y, int w, int h, int t) {
    for (int yy = 0; yy < h; yy++) gfx_hline(x, x + w - 1, y + yy, yy < 22 ? C_SKY : yy < 34 ? C_BLUE : C_TAN);
    for (int i = 0; i < 6; i++) gfx_rect(x + i * 26 + 2, y + 12 - (i * 5) % 7, 18, 10 + (i * 5) % 7, C_WHITE);
    gfx_rect(x, y + 32, w, 4, C_BROWN);
    gfx_hline(x, x + w - 1, y + 32, C_TAN);
    for (int yy = y + 36; yy < y + h; yy += 7) gfx_hline(x, x + w - 1, yy, C_EARTH);
    gfx_vline(x + w / 2, y + 36, y + h - 1, C_CREAM);
    int k = (t / 20) % 2;
    spr_draw(&cc_fighter_spr[F_FINN][k ? POSE_STRIKE : POSE_WIND], x + 22, y + 30, 0);
    draw_weapon(F_FINN, k ? POSE_STRIKE : POSE_WIND, x + 22, y + 30, 0, false);
    spr_draw(&cc_fighter_spr[F_WREN][POSE_IDLE0], x + w - 42, y + 30, SPR_FLIPX);
    draw_weapon(F_WREN, POSE_IDLE0, x + w - 42, y + 30, 1, false);
    int bx = x + 44 + (t * 2) % (w - 90);
    gfx_dither(bx - 2, y + 51, 6, 2, C_BROWN, 10);
    spr_draw(&cc_spr[CS_BALL], bx - 3, y + 38, 0);
}

/* The demo player: player 1's buttons for the runner's "bot" query. It
 * asks the CPU what it would do (at the best skill) and presses the real
 * buttons for it; a press that follows a held button waits a frame so it
 * is a fresh press. */
static bool bot_b, bot_a, bot_b_next, bot_a_next;

static int bot_buttons(void) {
    if (state != S_MATCH) {
        bot_b = bot_a = bot_b_next = bot_a_next = false;
        return (state_t / 8) % 2 ? BTN_A : 0;
    }
    Pad pd;
    M.pl[0].ai_skill = 4;
    cc_ai_pad(&M, 0, &pd);
    int mask = 0;
    if (pd.dx > 0) mask |= BTN_RIGHT;
    if (pd.dx < 0) mask |= BTN_LEFT;
    if (pd.dy > 0) mask |= BTN_DOWN;
    if (pd.dy < 0) mask |= BTN_UP;
    bool b = pd.held, a = false;
    if (pd.strike) {
        if (bot_b) bot_b_next = true;
        else b = true;
    } else if (bot_b_next && !bot_b) {
        b = true;
        bot_b_next = false;
    }
    if (pd.roll) {
        if (bot_a) bot_a_next = true;
        else a = true;
    } else if (bot_a_next && !bot_a) {
        a = true;
        bot_a_next = false;
    }
    bot_b = b;
    bot_a = a;
    return mask | (b ? BTN_B : 0) | (a ? BTN_A : 0);
}

static int cc_query(const char *key, int *out) {
    if (!strcmp(key, "bot")) { *out = bot_buttons(); return 1; }
    if (!strcmp(key, "rally100")) { *out = (int)lroundf(M.rally * 100); return 1; }
    if (!strcmp(key, "ball_speed100")) { *out = (int)lroundf(sqrtf(M.ball.vx * M.ball.vx + M.ball.vy * M.ball.vy) * 100); return 1; }
    if (!strcmp(key, "ball_lob")) { *out = M.ball.lob; return 1; }
    if (!strcmp(key, "ball_level")) { *out = M.ball.level; return 1; }
    if (!strcmp(key, "music")) { *out = music_playing(); return 1; }
    if (!strcmp(key, "projvy100")) {
        *out = 0;
        for (int i = 0; i < CC_MAX_PROJ; i++)
            if (M.pr[i].live) { *out = (int)lroundf(M.pr[i].vy * 100); break; }
        return 1;
    }
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "mstate")) { *out = M.state; return 1; }
    if (!strcmp(key, "mode")) { *out = mode; return 1; }
    if (!strcmp(key, "score_l")) { *out = M.score[0]; return 1; }
    if (!strcmp(key, "score_r")) { *out = M.score[1]; return 1; }
    if (!strcmp(key, "points")) { *out = M.score[0] + M.score[1]; return 1; }
    if (!strcmp(key, "strikes")) { int n = 0; for (int i = 0; i < M.np; i++) n += M.pl[i].strikes; *out = n; return 1; }
    if (!strcmp(key, "fouls_l")) { *out = M.fouls[0]; return 1; }
    if (!strcmp(key, "fouls_r")) { *out = M.fouls[1]; return 1; }
    if (!strcmp(key, "winner")) { *out = M.winner; return 1; }
    if (!strcmp(key, "np")) { *out = M.np; return 1; }
    if (!strcmp(key, "goal")) { *out = sv.goal; return 1; }
    if (!strcmp(key, "laws")) { *out = sv.laws; return 1; }
    if (!strcmp(key, "speed")) { *out = sv.speed; return 1; }
    if (!strcmp(key, "timeopt")) { *out = sv.time; return 1; }
    if (!strcmp(key, "time_left")) { *out = M.time_left; return 1; }
    if (!strcmp(key, "sudden")) { *out = M.sudden; return 1; }
    if (!strcmp(key, "serve_live")) { *out = M.serve_live; return 1; }
    if (!strcmp(key, "receiver")) { *out = M.receiver_team; return 1; }
    if (!strcmp(key, "defeated")) { *out = defeated; return 1; }
    if (!strcmp(key, "continues")) { *out = continues; return 1; }
    if (!strcmp(key, "opp_i")) { *out = opp_i; return 1; }
    if (!strcmp(key, "cups")) { *out = sv.cups; return 1; }
    if (!strcmp(key, "best")) { *out = sv.best_defeated; return 1; }
    if (!strcmp(key, "versus")) { *out = input_versus(); return 1; }
    if (!strcmp(key, "ball_live")) { *out = M.ball.live; return 1; }
    if (!strcmp(key, "ball_kind")) { *out = M.ball.kind; return 1; }
    if (!strcmp(key, "ball_team")) { *out = M.ball.team; return 1; }
    if (!strcmp(key, "ball_x")) { *out = (int)M.ball.x; return 1; }
    if (!strcmp(key, "ball_y")) { *out = (int)M.ball.y; return 1; }
    if (!strcmp(key, "ball_z")) { *out = (int)M.ball.z; return 1; }
    if (!strcmp(key, "ball_vx100")) { *out = (int)(M.ball.vx * 100); return 1; }
    if (!strcmp(key, "ball_vy100")) { *out = (int)(M.ball.vy * 100); return 1; }
    if (!strcmp(key, "ball_curve")) { *out = M.ball.curve != 0; return 1; }
    if (!strcmp(key, "caught_by")) { *out = M.ball.caught_by; return 1; }
    if (!strcmp(key, "fakes")) { int n = 0; for (int i = 0; i < CC_MAX_FAKES; i++) n += M.fake[i].live; *out = n; return 1; }
    if (!strncmp(key, "projs", 5)) {
        int n = 0, kind = key[5] ? atoi(key + 5) : -1;
        for (int i = 0; i < CC_MAX_PROJ; i++) n += M.pr[i].live && (kind < 0 || M.pr[i].kind == kind);
        *out = n;
        return 1;
    }
    /* per player: meter0 px0 py0 st0 stun0 */
    static const char *PK[5] = {"meter", "px", "py", "st", "stun"};
    for (int k = 0; k < 5; k++) {
        size_t n = strlen(PK[k]);
        if (!strncmp(key, PK[k], n) && isdigit((unsigned char)key[n]) && !key[n + 1]) {
            int i = key[n] - '0';
            if (i < 0 || i >= CC_MAX_PLAYERS) return 0;
            const Player *p = &M.pl[i];
            *out = k == 0 ? p->meter : k == 1 ? (int)p->x : k == 2 ? (int)p->y : k == 3 ? p->state : p->stun;
            return 1;
        }
    }
    return 0;
}

static int cc_cheat(const char *cmd) {
    int a, b, c;
    float fx, fy, fvx, fvy, fz, fvz;
    if (sscanf(cmd, "match %d %d", &a, &b) == 2) {
        /* a 1P match against the CPU, straight away */
        mode = MODE_TOURNEY;
        pick[0] = iclamp(a, 0, CC_FIGHTERS - 1);
        n_opp = 5;
        opp[0] = iclamp(b, 0, CC_FIGHTERS - 1);
        opp_i = 0;
        start_match();
        return 1;
    }
    if (sscanf(cmd, "tourney %d", &a) == 1) {
        mode = MODE_TOURNEY;
        pick[0] = iclamp(a, 0, CC_FIGHTERS - 1);
        start_tourney();
        return 1;
    }
    if (sscanf(cmd, "coop %d %d", &a, &b) == 2) {
        mode = MODE_COOP;
        pick[0] = a;
        pick[1] = b;
        start_tourney();
        return 1;
    }
    if (sscanf(cmd, "versus %d %d", &a, &b) == 2) {
        mode = MODE_VERSUS;
        pick[0] = a;
        pick[1] = b;
        start_match();
        return 1;
    }
    if (!strcmp(cmd, "play")) {
        /* skip the ready call and the serve: an empty court in play */
        M.state = MS_PLAY;
        M.state_t = 0;
        M.serve_live = false;
        M.ball.live = 0;
        return 1;
    }
    int n = sscanf(cmd, "ball %f %f %f %f %f %f", &fx, &fy, &fvx, &fvy, &fz, &fvz);
    if (n >= 4) {
        Ball *bl = &M.ball;
        memset(bl, 0, sizeof *bl);
        bl->live = 1;
        bl->x = fx; bl->y = fy; bl->vx = fvx; bl->vy = fvy;
        bl->z = n >= 5 ? fz : 0;
        bl->vz = n >= 6 ? fvz : 0;
        bl->caught_by = -1;
        bl->hitter = -1;
        bl->team = (int8_t)(fvx > 0 ? 0 : 1);
        bl->dir = (int8_t)(fvx > 0 ? 1 : -1);
        bl->t = 10;
        return 1;
    }
    if (sscanf(cmd, "superball %d %f %f %f", &a, &fx, &fy, &fvx) == 4) {
        Ball *bl = &M.ball;
        memset(bl, 0, sizeof *bl);
        bl->live = 1;
        bl->kind = (uint8_t)a;
        bl->x = fx; bl->y = fy; bl->vx = fvx;
        bl->speed = fabsf(fvx);
        bl->caught_by = -1;
        bl->hitter = -1;
        bl->team = (int8_t)(fvx > 0 ? 0 : 1);
        bl->dir = (int8_t)(fvx > 0 ? 1 : -1);
        bl->t = 10;
        return 1;
    }
    if (sscanf(cmd, "pos %d %d %d", &a, &b, &c) == 3) { M.pl[a].x = (float)b; M.pl[a].y = (float)c; return 1; }
    if (sscanf(cmd, "meter %d %d", &a, &b) == 2) { M.pl[a].meter = b; return 1; }
    if (sscanf(cmd, "cpu %d %d", &a, &b) == 2) { M.pl[a].cpu = (uint8_t)b; return 1; }
    if (sscanf(cmd, "score %d %d", &a, &b) == 2) { M.score[0] = a; M.score[1] = b; return 1; }
    if (sscanf(cmd, "fouls %d %d", &a, &b) == 2) { M.fouls[0] = a; M.fouls[1] = b; return 1; }
    if (sscanf(cmd, "goal %d", &a) == 1) { sv.goal = (uint8_t)a; M.goal = a; return 1; }
    if (sscanf(cmd, "laws %d", &a) == 1) { sv.laws = (uint8_t)a; M.laws = a != 0; return 1; }
    if (sscanf(cmd, "speed %d", &a) == 1) { sv.speed = (uint8_t)a; return 1; }
    if (sscanf(cmd, "timeleft %d", &a) == 1) { M.time_limit = a; M.time_left = a; return 1; }
    if (sscanf(cmd, "timeopt %d", &a) == 1) { sv.time = (uint8_t)a; return 1; }
    if (sscanf(cmd, "level %d", &a) == 1) { M.ai_level = a; return 1; }
    if (sscanf(cmd, "skill %d %d", &a, &b) == 2) { M.pl[a].ai_skill = (int8_t)b; return 1; }
    if (sscanf(cmd, "receiver %d", &a) == 1) { M.receiver_team = a & 1; return 1; }
    if (!strcmp(cmd, "lob")) { M.ball.lob = true; return 1; }
    if (!strcmp(cmd, "demo")) { demo = true; for (int i = 0; i < M.np; i++) M.pl[i].cpu = 1; return 1; }
    if (!strcmp(cmd, "save")) { save_now(); return 1; }
    if (sscanf(cmd, "win %d", &a) == 1) {
        /* end the current match now; a = winning team */
        M.score[a] = M.goal;
        M.state = MS_OVER;
        M.state_t = 0;
        M.winner = a;
        return 1;
    }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    return 0;
}

const GameDef GAME_CUTLASS = {
    "cutlass",
    "CUTLASS CUP",
    "1985",
    "SPORTS",
    "BLADES, A BALL AND A GALLEY DECK. KNOCK IT PAST YOUR RIVAL INTO THE SEA.",
    {"BEAT 3 OPPONENTS IN A RUN", "WIN THE CUTLASS CUP", "WIN THE CUP WITHOUT CONTINUING"},
    "D-PAD\tRUN\n"
    GLYPH_B "\tSTRIKE (UP/DOWN AIM, BACK LOB)\n"
    GLYPH_B " " GLYPH_B "\tTRICK (HALF BAR)\n"
    "HOLD " GLYPH_B "\tBROADSIDE (ONE BAR)\n"
    GLYPH_A "\tROLL\n"
    "START\tPAUSE\n"
    "2P KEYS: WASD F G / ARROWS K L",
    C_RED, C_SKY,
    cc_load, cc_start, cc_update, cc_draw, cc_quit, cc_label, cc_query, cc_cheat,
    "BUSHIDO BALL", 14,
};
