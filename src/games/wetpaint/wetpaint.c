/* WET PAINT - paint the town blue in the Wet Paint Rally.
 * Cartridge 04 of UFO 40, a tribute to Paint Chase (UFO 50 #4).
 * Every rule and where it comes from is in docs/games/04-wet-paint.md.
 * This file is the cartridge: the title, cutscenes, courses, results, the
 * ending and 2P versus, the drawing and the test hooks. The course rules
 * themselves are in wetpaint_logic.c. */
#include "wetpaint.h"

#define START_LIVES 3      /* fail a course three times and it's over */
#define LIFE_EVERY 120     /* an extra life for every 120 points */
#define ALIEN_SCORE 500
#define BEACON_COURSES 12
#define HISCORES 5
#define READY_TIME 110
#define RESULT_TIME 200
#define N_CUTS 6           /* before courses 1, 6, 11, 16, 21 and the final */

enum { S_TITLE, S_CUT, S_READY, S_PLAY, S_RESULT, S_OVER, S_END, S_VS_PICK };

typedef struct Save {
    uint32_t magic;
    uint32_t hi[HISCORES];
    uint8_t best_course;   /* highest course reached (1..26), 27 = won */
    uint8_t best_lives;    /* most lives left after winning */
    uint8_t won, alien;
} Save;
#define SAVE_MAGIC 0x57500001u

static Save sv;
static int state, state_t, frame_t, new_rank = -1;
static int menu_sel, cut_i;
static int lives, score, course, run_mode;
static int res_blue, res_pink, res_points, res_pass, res_life, res_secret;
static int vs_course, vs_wins[2];
static bool sheet_mode, demo_auto;
static Rng seeds;

typedef struct { float x, y, vx, vy; int life, col, kind; } Part;
static Part parts[160];

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) { parts[i] = (Part){x, y, vx, vy, life, col, kind}; return; }
}

static void burst(float x, float y, int col, int n, float sp) {
    for (int i = 0; i < n; i++) {
        float a = (float)i / (float)n * 6.283f;
        part_add(x, y, cosf(a) * sp, sinf(a) * sp, 16 + i % 6, col, 0);
    }
}

/* ------------------------------------------------------------------ */
/* save, board and goals                                                */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    memset(&sv, 0, sizeof sv);
    sv.magic = SAVE_MAGIC;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    /* the goals come back from what the save remembers */
    if (sv.best_course > BEACON_COURSES) game_award(GOAL_BEACON);
    if (sv.won) game_award(GOAL_SAUCER);
    if (sv.alien) game_award(GOAL_ALIEN);
}

static void record_score(void) {
    new_rank = -1;
    for (int i = 0; i < HISCORES; i++)
        if ((uint32_t)score > sv.hi[i]) {
            for (int j = HISCORES - 1; j > i; j--) sv.hi[j] = sv.hi[j - 1];
            sv.hi[i] = (uint32_t)score;
            new_rank = i;
            break;
        }
    save_now();
}

/* the highest course reached, 1-26 (27: the cup won) */
static void reach(int c) {
    if (c + 1 > sv.best_course) { sv.best_course = (uint8_t)(c + 1); save_now(); }
}

/* ------------------------------------------------------------------ */
/* flow                                                                  */

static int race_music(int c) {
    if (c == WP_FINAL) return WP_MUS_FINAL;
    if (c >= 20) return WP_MUS_RACE3;
    if (c >= 10) return WP_MUS_RACE2;
    return WP_MUS_RACE1;
}

static int cut_before(int c) {
    if (c == WP_FINAL) return 5;
    return c % 5 == 0 ? c / 5 : -1;
}

static void to_ready(void) {
    wp_sim_start(course, run_mode, (uint64_t)rng_next(&seeds) | ((uint64_t)rng_next(&seeds) << 32));
    state = S_READY;
    state_t = 0;
    memset(parts, 0, sizeof parts);
    music_play(race_music(course));
    game_set_pausable(true);
    if (run_mode == MODE_SOLO) reach(course);
}

static void to_course(int c) {
    course = c;
    int k = cut_before(c);
    if (k >= 0 && run_mode == MODE_SOLO) {
        cut_i = k;
        state = S_CUT;
        state_t = 0;
        music_play(WP_MUS_CUT);
        game_set_pausable(true);
    } else {
        to_ready();
    }
}

static void new_run(void) {
    run_mode = MODE_SOLO;
    input_set_versus(false);
    lives = START_LIVES;
    score = 0;
    new_rank = -1;
    to_course(0);
}

static void to_title(void) {
    state = S_TITLE;
    state_t = 0;
    input_set_versus(false);
    game_set_pausable(false);
    music_play(WP_MUS_TITLE);
}

static void finish_course(void) {
    res_blue = wp_percent(PAINT_BLUE);
    res_pink = wp_percent(PAINT_PINK);
    res_secret = wp.secret;
    res_life = 0;
    state = S_RESULT;
    state_t = 0;
    if (run_mode == MODE_VERSUS) {
        int b = wp_count(PAINT_BLUE), p = wp_count(PAINT_PINK);
        res_pass = b > p ? 1 : p > b ? 2 : 0;
        if (res_pass) vs_wins[res_pass - 1]++;
        music_play(WP_MUS_CLEAR);
        return;
    }
    int goal = WP_COURSE[course].goal;
    res_pass = res_blue >= goal;
    if (res_pass) {
        res_points = res_blue - goal;
        int before = score / LIFE_EVERY;
        score += res_points;
        res_life = score / LIFE_EVERY - before;
        lives += res_life;
        music_play(WP_MUS_CLEAR);
        if (course + 1 >= BEACON_COURSES) game_award(GOAL_BEACON);
        reach(course + 1);
    } else {
        res_points = 0;
        lives--;
        music_play(WP_MUS_MISS);
    }
}

static void after_result(void) {
    if (run_mode == MODE_VERSUS) {
        state = S_VS_PICK;
        state_t = 0;
        music_play(WP_MUS_TITLE);
        return;
    }
    if (!res_pass) {
        if (lives <= 0) {
            state = S_OVER;
            state_t = 0;
            record_score();
            music_play(WP_MUS_OVER);
            game_set_pausable(false);
        } else {
            to_ready(); /* try the same course again */
        }
        return;
    }
    if (course == WP_FINAL) {
        /* the Wet Paint Cup: gold, or platinum with 500 points */
        sv.won = 1;
        sv.best_course = WP_COURSES + 1;
        if (lives > sv.best_lives) sv.best_lives = (uint8_t)lives;
        game_award(GOAL_SAUCER);
        if (score >= ALIEN_SCORE) { sv.alien = 1; game_award(GOAL_ALIEN); }
        record_score();
        state = S_END;
        state_t = 0;
        music_play(WP_MUS_END);
        game_set_pausable(false);
        return;
    }
    save_now();
    to_course(course + 1);
}

/* ------------------------------------------------------------------ */
/* update                                                                */

static void consume_events(void) {
    for (int i = 0; i < wp.n_ev; i++) {
        float x = (float)(WP_OX + wp.ev[i].x), y = (float)(WP_OY + wp.ev[i].y);
        switch (wp.ev[i].kind) {
        case EV_KILL: burst(x, y, C_MAGENTA, 10, 1.2f); burst(x, y, C_PINK, 6, 0.6f); sfx_play_name("wp_kill"); break;
        case EV_POP: burst(x, y, C_PINK, 24, 2.2f); burst(x, y, C_MAGENTA, 16, 1.2f); sfx_play_name("wp_pop"); break;
        case EV_STUN: for (int k = 0; k < 5; k++) part_add(x, y - 4, (float)(k - 2) * 0.4f, -0.8f, 30, C_YELLOW, 1); sfx_play_name("wp_stun"); break;
        case EV_ITEM: burst(x, y, C_YELLOW, 8, 1.0f); sfx_play_name("wp_item"); break;
        case EV_BOOST: sfx_play_name("wp_boost"); break;
        case EV_BUMP: burst(x, y, C_WHITE, 6, 0.8f); sfx_play_name("wp_bump"); break;
        case EV_THORN: burst(x, y, C_LEAF, 10, 1.0f); sfx_play_name("wp_thorn"); break;
        case EV_SPRAY: sfx_play_name("wp_spray"); break;
        case EV_LEVER: sfx_play_name("wp_lever"); break;
        }
    }
}

static void play_update(void) {
    uint32_t held = input_held();
    wp.car[0].hold = held & 0xFF;
    if (demo_auto) wp.car[0].hold = (uint32_t)wp_bot_buttons(0);
    if (wp.car[1].on) wp.car[1].hold = wp.car[1].human ? (held >> BTN_P2_SHIFT) & 0xFF : (uint32_t)wp_bot_buttons(1);
    int before = wp.frames_left / 60;
    wp_sim_step();
    consume_events();
    int garage_flash = 0;
    for (int i = 0; i < wp.n_garages; i++) garage_flash |= wp.garage[i].phase == 1 && wp.garage[i].t == 59;
    if (garage_flash) sfx_play_name("wp_flash");
    if (wp.frames_left / 60 != before && wp.frames_left < 5 * 60 && wp.frames_left > 0) sfx_play_name("wp_tick");
    if (wp.frames_left <= 0) finish_course();
}

static void wp_update(void) {
    state_t++;
    frame_t++;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vx *= 0.92f;
        p->vy *= 0.92f;
    }
    switch (state) {
    case S_TITLE: {
        game_set_pausable(false);
        bool two = plat_kind() != PLAT_VITA;
        if (btnp(BTN_UP) || btnp(BTN_DOWN)) { menu_sel ^= 1; if (!two) menu_sel = 0; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) {
            sfx_play_name("ui_ok");
            input_consume();
            if (menu_sel == 0) new_run();
            else {
                run_mode = MODE_VERSUS;
                input_set_versus(true);
                vs_wins[0] = vs_wins[1] = 0;
                state = S_VS_PICK;
                state_t = 0;
            }
        }
        break;
    }
    case S_CUT:
        if (state_t > 40 && (btnp(BTN_A) || btnp(BTN_START) || state_t > 60 * 14)) { input_consume(); to_ready(); }
        break;
    case S_READY:
        if (state_t >= READY_TIME) { state = S_PLAY; state_t = 0; }
        break;
    case S_PLAY: play_update(); break;
    case S_RESULT:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START) || state_t > RESULT_TIME)) after_result();
        break;
    case S_OVER:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) to_title();
        break;
    case S_END:
        if (state_t > 420 && (btnp(BTN_A) || btnp(BTN_START))) to_title();
        break;
    case S_VS_PICK:
        game_set_pausable(false);
        if (btn_repeat(BTN_LEFT)) { vs_course = (vs_course + WP_COURSES - 1) % WP_COURSES; sfx_play_name("ui_move"); }
        if (btn_repeat(BTN_RIGHT)) { vs_course = (vs_course + 1) % WP_COURSES; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { sfx_play_name("ui_back"); to_title(); break; }
        if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) {
            sfx_play_name("ui_ok");
            input_consume();
            course = vs_course;
            to_ready();
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing: the course                                                   */

typedef struct { uint8_t wall, wall_hi, wall_lo, floor, speck; } Theme;
static const Theme THEMES[6] = {
    {C_BROWN, C_TAN, C_EARTH, C_SLATE, C_GREY},    /* the town square: brick */
    {C_FOREST, C_LEAF, C_TEAL, C_SLATE, C_GREY},   /* the park: hedges */
    {C_EARTH, C_AMBER, C_BROWN, C_DUSK, C_SLATE},  /* the docks: crates */
    {C_GREY, C_LIGHT, C_SLATE, C_DUSK, C_SLATE},   /* the works: steel */
    {C_TEAL, C_JADE, C_NIGHT, C_NIGHT, C_DUSK},    /* the town by night */
    {C_PURPLE, C_VIOLET, C_NIGHT, C_DUSK, C_SLATE},/* the final */
};
static const Theme *TH(void) { return &THEMES[wp.course == WP_FINAL ? 5 : wp.course / 5]; }

static int tx0(int x) { return WP_OX + x * WP_TILE; }
static int ty0(int y) { return WP_OY + y * WP_TILE; }

static void draw_arrow(int px, int py, int dir, int col) {
    /* a chevron pointing the arrow's way */
    for (int k = 0; k < 2; k++)
        for (int i = 0; i < 4; i++) {
            int cx, cy;
            switch (dir) {
            case WP_RIGHT: cx = px + 2 + k * 4 + i; cy = py + 2 + i; gfx_pset(cx, cy, col); gfx_pset(cx, py + 9 - i, col); break;
            case WP_LEFT: cx = px + 9 - k * 4 - i; cy = py + 2 + i; gfx_pset(cx, cy, col); gfx_pset(cx, py + 9 - i, col); break;
            case WP_DOWN: cy = py + 2 + k * 4 + i; cx = px + 2 + i; gfx_pset(cx, cy, col); gfx_pset(px + 9 - i, cy, col); break;
            default: cy = py + 9 - k * 4 - i; cx = px + 2 + i; gfx_pset(cx, cy, col); gfx_pset(px + 9 - i, cy, col); break;
            }
        }
}

static void draw_paint(int px, int py, int x, int y, int team) {
    int a = team == PAINT_BLUE ? C_BLUE : C_MAGENTA, b = team == PAINT_BLUE ? C_SKY : C_PINK, c = team == PAINT_BLUE ? C_NAVY : C_WINE;
    gfx_rect(px, py, WP_TILE, WP_TILE, a);
    /* a glossy streak and a drip, different on every tile */
    int h = (x * 7 + y * 13) % 5;
    gfx_hline(px + 1 + h, px + 4 + h, py + 2, b);
    gfx_pset(px + 8 - h, py + 8, c);
    gfx_pset(px + 8 - h, py + 9, c);
}

static void draw_tile(int x, int y) {
    const Theme *t = TH();
    const WpTile *tl = &wp.tile[y][x];
    int px = tx0(x), py = ty0(y);
    switch (tl->kind) {
    case TK_WALL:
        gfx_rect(px, py, WP_TILE, WP_TILE, t->wall);
        if (y == 0 || wp.tile[y - 1][x].kind != TK_WALL) gfx_hline(px, px + WP_TILE - 1, py, t->wall_hi);
        if (y == WP_H - 1 || wp.tile[y + 1][x].kind != TK_WALL) gfx_hline(px, px + WP_TILE - 1, py + WP_TILE - 1, t->wall_lo);
        if ((x + y) % 2 == 0) gfx_pset(px + 4, py + 5, t->wall_lo);
        else gfx_pset(px + 8, py + 7, t->wall_hi);
        return;
    case TK_GARAGE: {
        gfx_rect(px, py, WP_TILE, WP_TILE, t->wall_lo);
        bool flash = false;
        for (int i = 0; i < wp.n_garages; i++)
            if (wp.garage[i].x == x && wp.garage[i].y == y && wp.garage[i].phase == 1) flash = (frame_t / 4) % 2;
        gfx_rect(px + 2, py + 2, 8, 8, flash ? C_WHITE : C_INK);
        for (int k = 0; k < 3; k++) gfx_hline(px + 2, px + 9, py + 3 + k * 2, flash ? C_PINK : C_WINE);
        /* the arrow over the door */
        draw_arrow(px, py, tl->dir, flash ? C_MAGENTA : C_PINK);
        return;
    }
    case TK_BUMPER:
        gfx_rect(px, py, WP_TILE, WP_TILE, t->floor);
        gfx_circ(px + 6, py + 6, 5, C_INK);
        gfx_circ(px + 6, py + 6, 4, C_RED);
        gfx_circ(px + 6, py + 6, 2, C_WHITE);
        gfx_pset(px + 4, py + 4, C_PINK);
        return;
    default: break;
    }
    /* floor: asphalt with specks, or paint */
    int p = wp.paint[y][x];
    if (p) draw_paint(px, py, x, y, p);
    else {
        gfx_rect(px, py, WP_TILE, WP_TILE, t->floor);
        if ((x * 5 + y * 3) % 4 == 0) gfx_pset(px + 3 + (x % 3) * 2, py + 4 + (y % 2) * 4, t->speck);
    }
    switch (tl->kind) {
    case TK_ARROW: draw_arrow(px, py, tl->dir, (frame_t / 8 + x + y) % 3 ? C_WHITE : C_YELLOW); break;
    case TK_BELT: {
        int o = (frame_t / 3) % 4;
        for (int k = 0; k < 3; k++) {
            int s = (k * 4 + (tl->dir == WP_RIGHT || tl->dir == WP_DOWN ? o : 4 - o)) % WP_TILE;
            if (tl->dir == WP_LEFT || tl->dir == WP_RIGHT) gfx_vline(px + s, py + 1, py + WP_TILE - 2, C_INK);
            else gfx_hline(px + 1, px + WP_TILE - 2, py + s, C_INK);
        }
        gfx_rectb(px, py, WP_TILE, WP_TILE, C_GREY);
        break;
    }
    case TK_THORN:
        if (wp.thorn[y][x]) {
            gfx_circ(px + 6, py + 6, 5, C_FOREST);
            gfx_circ(px + 5, py + 5, 3, C_LEAF);
            for (int k = 0; k < 8; k++) {
                float a = (float)k * 0.785f;
                gfx_pset(px + 6 + (int)lroundf(cosf(a) * 6), py + 6 + (int)lroundf(sinf(a) * 6), C_WHITE);
            }
        }
        break;
    case TK_SWING:
        if (wp_solid(x, y)) {
            gfx_rect(px, py, WP_TILE, WP_TILE, C_WHITE);
            for (int k = 0; k < WP_TILE; k += 4) gfx_rect(px + k, py, 2, WP_TILE, C_RED);
            gfx_rectb(px, py, WP_TILE, WP_TILE, C_INK);
        } else {
            gfx_rectb(px + 1, py + 1, WP_TILE - 2, WP_TILE - 2, C_RED);
        }
        break;
    case TK_LEVER:
        gfx_rect(px + 3, py + 8, 6, 3, C_GREY);
        gfx_line(px + 6, py + 9, px + (wp.swing_open ? 9 : 3), py + 3, C_LIGHT);
        gfx_rect(px + (wp.swing_open ? 8 : 2), py + 2, 2, 2, C_RED);
        break;
    case TK_BOLLARD: {
        bool up = wp_solid(x, y);
        bool soon = !up && wp.bollard_t < 30 && (frame_t / 3) % 2; /* about to rise */
        if (up) {
            gfx_rect(px + 2, py + 1, 8, 10, C_YELLOW);
            for (int k = 0; k < 3; k++) gfx_rect(px + 2, py + 2 + k * 3, 8, 1, C_INK);
            gfx_rectb(px + 2, py + 1, 8, 10, C_INK);
        } else {
            gfx_circb(px + 6, py + 6, 3, soon ? C_YELLOW : C_INK);
        }
        break;
    }
    default: break;
    }
    if (tl->item) {
        static const int ICON[5] = {0, WS_SPRINKLER, WS_TACK, WS_HELPER_ICON, WS_FREEZE};
        const Sprite *s = &wp_spr[ICON[tl->item]];
        if (p == PAINT_BLUE) {
            /* used: waits dim until a foe paints over it */
            uint8_t m[PAL_COUNT];
            pal_identity(m);
            m[C_YELLOW] = C_SKY;
            m[C_AMBER] = C_NAVY;
            spr_draw_ex(s, px + 2, py + 2, 0, m, -1);
        } else {
            spr_draw(s, px + 2, py + 2 - ((frame_t / 12) % 2), 0);
        }
    }
}

static void draw_car(int i) {
    const WpCar *c = &wp.car[i];
    if (!c->on) return;
    int32_t x, y;
    wp_mover_pos(&c->m, &x, &y);
    int sx = WP_OX + x / WP_UNIT - 5, sy = WP_OY + y / WP_UNIT - 5;
    int spr = c->team == PAINT_BLUE ? WS_BO : WS_FOXY;
    uint8_t m[PAL_COUNT];
    pal_identity(m);
    if (c->team == PAINT_PINK) { /* Foxy's crumple is pink */
        m[C_BLUE] = C_MAGENTA; m[C_SKY] = C_PINK; m[C_YELLOW] = C_ORANGE;
    }
    if (c->stun_t > 0) {
        spr_draw_ex(&wp_spr[WS_CRUMPLE], sx, sy, (c->stun_t / 8) % 2 ? SPR_FLIPX : 0, m, -1);
        for (int k = 0; k < 3; k++) {
            float a = (float)(frame_t * 0.2f) + (float)k * 2.09f;
            gfx_pset(sx + 5 + (int)(cosf(a) * 6), sy - 1 + (int)(sinf(a) * 2), C_YELLOW);
        }
        return;
    }
    if (c->inv_t > 0 && (frame_t / 3) % 2) return;
    int dir = c->spin ? (frame_t / 3) % 4 : c->m.dir;
    if (wp_car_boosted(c)) {
        /* speed lines behind */
        for (int k = 1; k <= 3; k++) {
            int bx = sx + 5 - WP_DX[c->m.dir] * (6 + k * 3), by = sy + 5 - WP_DY[c->m.dir] * (6 + k * 3);
            gfx_pset(bx + (c->m.dir & 1 ? (k % 2) * 4 - 2 : 0), by + (c->m.dir & 1 ? 0 : (k % 2) * 4 - 2), C_WHITE);
        }
    }
    if (wp.freeze_t > 0 && wp.freeze_team == c->team) spr_draw_outline(&wp_car[spr][dir], sx, sy, 0, C_ICE);
    else if (c->gun_t > 0 && (frame_t / 4) % 2) spr_draw_outline(&wp_car[spr][dir], sx, sy, 0, C_YELLOW);
    else spr_draw(&wp_car[spr][dir], sx, sy, 0);
}

static void draw_foes(void) {
    static const int SPR[F_KINDS] = {WS_ROLLER, WS_DUSTER1, WS_TANKER, WS_GLOOP, WS_BIGGLOOP, WS_POPPER, WS_HEDGEHOG, WS_CONKER, WS_JELLY};
    bool frozen = wp.freeze_t > 0 && wp.freeze_team == PAINT_PINK;
    /* ground foes first, then the dusters above everything */
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < WP_MAX_FOES; i++) {
            const WpFoe *f = &wp.foe[i];
            if (!f->on || (pass == 1) != (f->kind == F_DUSTER)) continue;
            int32_t x, y;
            if (f->kind == F_DUSTER) { x = f->fx; y = f->fy; }
            else wp_mover_pos(&f->m, &x, &y);
            int sx = WP_OX + x / WP_UNIT - 5, sy = WP_OY + y / WP_UNIT - 5;
            int spr = SPR[f->kind];
            if (f->kind == F_DUSTER) {
                spr = (frame_t / 3) % 2 ? WS_DUSTER1 : WS_DUSTER2;
                gfx_dither_circle(sx + 5, sy + 9, 3, C_INK, 8); /* its shadow */
                sy -= 3;
            }
            if (f->kind == F_CONKER && !f->armed) spr = WS_CONKER_BARE;
            bool flashing = (f->kind == F_DUSTER && f->age >= 600) || (f->kind == F_POPPER && f->age >= 660);
            const Sprite *s = &wp_car[spr][f->kind == F_CONKER || f->kind == F_JELLY || f->kind == F_GLOOP || f->kind == F_BIGGLOOP ? WP_RIGHT : f->m.dir];
            if (frozen) spr_draw_outline(s, sx, sy, 0, C_ICE);
            else if (flashing && (frame_t / 4) % 2) spr_draw_ex(s, sx, sy, 0, NULL, C_WHITE);
            else spr_draw(s, sx, sy, 0);
            if (f->kind == F_HEDGEHOG && f->plan >= 0) {
                /* two signal lights on top: the one on the side it will turn blinks */
                int left = (f->m.dir + 3) & 3, right = (f->m.dir + 1) & 3;
                int cx = sx + 5 - WP_DX[f->m.dir] * 2, cy = sy + 5 - WP_DY[f->m.dir] * 2;
                bool bl = (frame_t / 6) % 2;
                int lx = cx + WP_DX[left] * 3, ly = cy + WP_DY[left] * 3, rx = cx + WP_DX[right] * 3, ry = cy + WP_DY[right] * 3;
                gfx_rect(lx - 1, ly - 1, 2, 2, f->plan == left && bl ? C_YELLOW : f->plan == f->m.dir ? C_LIME : C_INK);
                gfx_rect(rx - 1, ry - 1, 2, 2, f->plan == right && bl ? C_YELLOW : f->plan == f->m.dir ? C_LIME : C_INK);
            }
        }
}

static void draw_shots_and_drones(void) {
    for (int i = 0; i < WP_MAX_DRONES; i++) {
        const WpDrone *d = &wp.drone[i];
        if (!d->on) continue;
        if (d->inv_t > 0 && (frame_t / 3) % 2) continue;
        int32_t x, y;
        wp_mover_pos(&d->m, &x, &y);
        uint8_t m[PAL_COUNT];
        pal_identity(m);
        if (d->team == PAINT_PINK) { m[C_BLUE] = C_MAGENTA; m[C_SKY] = C_PINK; }
        spr_draw_ex(&wp_car[WS_HELPER][d->m.dir], WP_OX + x / WP_UNIT - 5, WP_OY + y / WP_UNIT - 5, 0, m, -1);
    }
    for (int i = 0; i < WP_MAX_SHOTS; i++) {
        const WpShot *s = &wp.shot[i];
        if (!s->on) continue;
        int x = WP_OX + s->x / WP_UNIT, y = WP_OY + s->y / WP_UNIT;
        switch (s->kind) {
        case SH_PAINT: gfx_rect(x - 1, y - 1, 3, 3, s->team == PAINT_BLUE ? C_SKY : C_PINK); gfx_pset(x, y, C_WHITE); break;
        case SH_SPRAY: gfx_rect(x - 1, y - 1, 3, 3, C_PINK); break;
        case SH_TACK: gfx_rect(x - 1, y - 1, 2, 2, C_WHITE); gfx_pset(x, y, C_YELLOW); break;
        case SH_SPINE:
            if (s->dir & 1) gfx_vline(x, y - 2, y + 2, C_WHITE);
            else gfx_hline(x - 2, x + 2, y, C_WHITE);
            break;
        }
    }
}

static void draw_parts(void) {
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        const Part *p = &parts[i];
        if (p->life <= 0) continue;
        if (p->kind == 1) gfx_pset((int)p->x, (int)p->y, (p->life / 3) % 2 ? C_YELLOW : C_WHITE);
        else gfx_rect((int)p->x, (int)p->y, 2, 2, p->col);
    }
}

static void draw_hud(void) {
    char buf[48];
    gfx_rect(0, 0, SCREEN_W, WP_OY - 1, C_INK);
    gfx_hline(0, SCREEN_W - 1, WP_OY - 2, C_DUSK);
    if (run_mode == MODE_VERSUS) {
        snprintf(buf, sizeof buf, "VERSUS " GLYPH_DOT " %s", WP_COURSE[wp.course].name);
        tiny_draw(buf, 4, 3, C_GREY);
        snprintf(buf, sizeof buf, "WINS %d-%d", vs_wins[0], vs_wins[1]);
        tiny_draw(buf, 4, 11, C_LIGHT);
    } else {
        if (wp.course == WP_FINAL) snprintf(buf, sizeof buf, "FINAL");
        else snprintf(buf, sizeof buf, "COURSE %02d", wp.course + 1);
        tiny_draw(buf, 4, 3, C_GREY);
        /* the head icons: lives */
        for (int i = 0; i < imin(lives, 8); i++) spr_draw(&wp_spr[WS_HEAD], 4 + i * 8, 10, 0);
        if (lives > 8) tiny_draw("+", 68, 12, C_WHITE);
    }
    /* the clock in the middle */
    int secs = (wp.frames_left + 59) / 60;
    snprintf(buf, sizeof buf, "%02d", secs);
    int col = wp.freeze_t > 0 ? C_ICE : secs <= 5 && (frame_t / 8) % 2 ? C_RED : C_WHITE;
    text_draw_scaled(buf, 160 - text_width_scaled(buf, 2) / 2, 3, col, 2);
    /* the coverage bar: blue from the left, pink from the right, the goal's mark */
    int bx = 190, bw = 80, by = 4;
    int blue = wp_percent(PAINT_BLUE), pink = wp_percent(PAINT_PINK);
    gfx_rect(bx - 1, by - 1, bw + 2, 7, C_DUSK);
    gfx_rect(bx, by, bw, 5, C_NIGHT);
    gfx_rect(bx, by, blue * bw / 100, 5, C_BLUE);
    gfx_rect(bx + bw - pink * bw / 100, by, pink * bw / 100, 5, C_MAGENTA);
    if (run_mode == MODE_SOLO) {
        int gx = bx + WP_COURSE[wp.course].goal * bw / 100;
        gfx_vline(gx, by - 2, by + 6, blue >= WP_COURSE[wp.course].goal ? C_LIME : C_WHITE);
        snprintf(buf, sizeof buf, "%d%%", blue);
        tiny_draw(buf, bx, 12, C_SKY);
        snprintf(buf, sizeof buf, "GOAL %d%%", WP_COURSE[wp.course].goal);
        tiny_draw(buf, bx + bw - tiny_width(buf), 12, C_LIGHT);
        tiny_draw("SCORE", 280, 3, C_SLATE);
        snprintf(buf, sizeof buf, "%d", score);
        text_draw(buf, 316 - text_width(buf), 11, C_YELLOW);
    } else {
        snprintf(buf, sizeof buf, "%d%%", blue);
        tiny_draw(buf, bx, 12, C_SKY);
        snprintf(buf, sizeof buf, "%d%%", pink);
        tiny_draw(buf, bx + bw - tiny_width(buf), 12, C_PINK);
    }
    if (wp.course == WP_FINAL && run_mode == MODE_SOLO) tiny_draw("VS FOXY", 60, 3, C_PINK);
    else if (run_mode == MODE_SOLO) tiny_draw(WP_COURSE[wp.course].name, 60, 3, C_DUSK);
}

static void draw_course(void) {
    int sx = 0, sy = 0;
    if (wp.shake > 0) { sx = (frame_t % 3) - 1; sy = ((frame_t / 2) % 3) - 1; }
    gfx_cls(C_INK);
    gfx_camera(sx, sy);
    for (int y = 0; y < WP_H; y++)
        for (int x = 0; x < WP_W; x++) draw_tile(x, y);
    draw_shots_and_drones();
    draw_foes();
    for (int i = 1; i >= 0; i--) draw_car(i);
    draw_parts();
    gfx_camera(0, 0);
    draw_hud();
}

/* ------------------------------------------------------------------ */
/* drawing: screens                                                      */

static void draw_board(int x, int y, int hilite) {
    char buf[32];
    for (int i = 0; i < HISCORES; i++) {
        snprintf(buf, sizeof buf, "%d. %4u", i + 1, (unsigned)sv.hi[i]);
        bool me = i == hilite && (frame_t / 8) % 2;
        tiny_draw(buf, x, y + i * 7, me ? C_WHITE : i == 0 ? C_YELLOW : C_LIGHT);
    }
}

static void draw_town(int t) {
    /* a street of painted shopfronts under the title */
    gfx_cls(C_NIGHT);
    for (int i = 0; i < 40; i++) {
        uint32_t h = (uint32_t)(i * 2654435761u);
        gfx_pset((int)(h % 320), (int)((h >> 12) % 90), (i % 5) ? C_DUSK : C_GREY);
    }
    static const uint8_t FRONT[6] = {C_BLUE, C_MAGENTA, C_TAN, C_BLUE, C_JADE, C_MAGENTA};
    for (int i = 0; i < 8; i++) {
        int x = i * 42 - 6, h = 36 + (i * 17) % 22;
        gfx_rect(x, 132 - h, 38, h, FRONT[i % 6]);
        gfx_rect(x, 132 - h, 38, 3, C_INK);
        gfx_rect(x + 6, 132 - h + 10, 10, 8, C_ICE);
        gfx_rect(x + 22, 132 - h + 10, 10, 8, C_ICE);
        gfx_rect(x + 14, 116, 10, 16, C_INK);
    }
    gfx_rect(0, 132, 320, 48, C_SLATE);
    for (int x = (t / 2) % 24; x < 320; x += 24) gfx_rect(x - 24, 154, 12, 2, C_LIGHT);
    /* a blue stripe being painted down the road */
    int px = (t * 2) % 400 - 40;
    gfx_rect(0, 140, imax(0, px), 8, C_BLUE);
    spr_draw(&wp_car[WS_BO][WP_RIGHT], px - 4, 139, 0);
    int fx = 360 - (t * 2 + 150) % 440;
    spr_draw(&wp_car[WS_FOXY][WP_LEFT], fx, 160, 0);
    gfx_rect(fx + 10, 162, 400, 6, C_MAGENTA);
}

static void draw_title(void) {
    draw_town(frame_t);
    static const uint8_t grad[] = {C_WHITE, C_SKY, C_BLUE, C_NAVY};
    ui_fancy_center("WET PAINT", 160, 10, 3, grad, 4, C_INK, C_PURPLE);
    tiny_center("THE TOWN SQUARE WET PAINT RALLY", 160, 42, C_PINK);
    bool two = plat_kind() != PLAT_VITA;
    static const char *const ITEMS[2] = {"1 PLAYER", "2 PLAYERS"};
    ui_panel(112, 54, 96, 32, C_INK, C_SLATE);
    for (int i = 0; i < 2; i++) {
        int y = 60 + i * 12;
        bool s = i == menu_sel;
        text_center(ITEMS[i], 160, y, !two && i == 1 ? C_DUSK : s ? C_WHITE : C_GREY);
        if (s) ui_cursor(118, y, frame_t);
    }
    ui_panel(4, 54, 80, 50, C_INK, C_DUSK);
    tiny_draw("TOP SCORES", 10, 58, C_SLATE);
    draw_board(12, 66, -1);
    ui_panel(236, 54, 80, 50, C_INK, C_DUSK);
    tiny_draw("BEST RUN", 242, 58, C_SLATE);
    char buf[32];
    if (sv.won) snprintf(buf, sizeof buf, "WON THE CUP");
    else if (sv.best_course) snprintf(buf, sizeof buf, "COURSE %d", imin(sv.best_course, WP_COURSES));
    else snprintf(buf, sizeof buf, "-");
    tiny_draw(buf, 242, 68, C_LIGHT);
    if (sv.won) {
        snprintf(buf, sizeof buf, "LIVES LEFT %d", sv.best_lives);
        tiny_draw(buf, 242, 76, C_LIGHT);
    }
}

typedef struct { const char *who; const char *line; } Line;
typedef struct { const char *title; Line lines[4]; int foes[3]; const char *hint[3]; } Cut;
static const Cut CUTS[N_CUTS] = {
    {"THE TOWN SQUARE",
     {{"MARSHAL", "THE WET PAINT RALLY! PAINT THE TOWN BLUE."},
      {"BO", "I'M STUCK BEHIND THE PINK KARTS! #@!%&"},
      {"MARSHAL", "BLUE ON THE CLOCK AT THE END, OR NO PASS."},
      {"", ""}},
     {F_ROLLER, F_DUSTER, -1},
     {"SMUDGER: RAM IT", "DUSTER: GET IT BEFORE IT SPRAYS", ""}},
    {"THE PARK",
     {{"MARSHAL", "THE PARK KEEPERS SENT HELP. PINK HELP."},
      {"BO", "IS THAT A GIANT GLOOP?"},
      {"MARSHAL", "AND IT'S HAVING BABIES. #@!%"},
      {"", ""}},
     {F_TANKER, F_BIGGLOOP, F_POPPER},
     {"TANKER: TWICE AS QUICK", "MOTHER GLOOP: LAYS GLOOPS", "POPPER: BURSTS IF LEFT ALONE"}},
    {"THE DOCKS",
     {{"MARSHAL", "HEDGEHOGS ON THE QUAY. MIND THE PLOUGH."},
      {"BO", "SO... HIT THEM FROM BEHIND?"},
      {"MARSHAL", "OR THE SIDE. AND WATCH THEIR LIGHTS."},
      {"", ""}},
     {F_HEDGEHOG, -1, -1},
     {"HEDGEHOG: SIDE OR BACK ONLY", "", ""}},
    {"THE WORKS",
     {{"MARSHAL", "CONKERS FROM THE FACTORY TREE!"},
      {"BO", "THEY SHOOT SPINES? WHO BUILT THIS TOWN?"},
      {"MARSHAL", "DON'T LINE UP WITH A SPIKY ONE."},
      {"", ""}},
     {F_CONKER, -1, -1},
     {"CONKER: FIRES WHEN IN LINE", "", ""}},
    {"THE TOWN BY NIGHT",
     {{"MARSHAL", "LAST FIVE. THE JELLIES ARE OUT."},
      {"BO", "I'LL RAM THEM FLAT OUT!"},
      {"MARSHAL", "NOT FLAT OUT. YOU'LL BOUNCE. #@!"},
      {"", ""}},
     {F_JELLY, -1, -1},
     {"JELLY: NEVER AT BOOST SPEED", "", ""}},
    {"THE FINAL SHOWDOWN",
     {{"FOXY", "NOBODY OUT-PAINTS FOXY FUCHSIA."},
      {"BO", "BLUE TOWN. PINK TOWN. LET'S SEE."},
      {"MARSHAL", "ONE COURSE. EVERYTHING SHE HAS, YOU HAVE."},
      {"", ""}},
     {-1, -1, -1},
     {"", "", ""}},
};

static void draw_portrait(const char *who, int x, int y) {
    int s = !strcmp(who, "BO") ? WS_BO_BIG : !strcmp(who, "FOXY") ? WS_FOXY_BIG : WS_MARSHAL_BIG;
    ui_panel(x - 3, y - 3, 46, 50, C_INK, C_SLATE);
    spr_draw_scaled(&wp_spr[s], x, y, 2, 0);
}

static void draw_cut(void) {
    const Cut *c = &CUTS[cut_i];
    int t = state_t;
    draw_town(t);
    gfx_darken_rect(0, 0, SCREEN_W, 132, 1);
    static const uint8_t grad[] = {C_WHITE, C_PINK, C_MAGENTA};
    char buf[48];
    if (cut_i < 5) snprintf(buf, sizeof buf, "COURSES %d-%d", cut_i * 5 + 1, cut_i * 5 + 5);
    else snprintf(buf, sizeof buf, "COURSE 26");
    tiny_center(buf, 160, 4, C_LIGHT);
    ui_fancy_center(c->title, 160, 11, 1, grad, 3, C_INK, -1);
    /* the conversation, a line at a time */
    int shown = imin(3, t / 70 + 1);
    const char *speaker = c->lines[imin(shown - 1, 2)].who;
    draw_portrait(speaker, 12, 30);
    for (int i = 0; i < shown; i++) {
        const Line *l = &c->lines[i];
        if (!l->who[0]) continue;
        int y = 30 + i * 16;
        tiny_draw(l->who, 66, y, !strcmp(l->who, "BO") ? C_SKY : !strcmp(l->who, "FOXY") ? C_PINK : C_YELLOW);
        text_draw(l->line, 66, y + 6, C_WHITE);
    }
    /* then the new foes parade by with their names */
    if (t > 240) {
        for (int i = 0; i < 3; i++) {
            if (c->foes[i] < 0) continue;
            static const int SPR[F_KINDS] = {WS_ROLLER, WS_DUSTER1, WS_TANKER, WS_GLOOP, WS_BIGGLOOP, WS_POPPER, WS_HEDGEHOG, WS_CONKER, WS_JELLY};
            int y = 84 + i * 14;
            int x = imin(20 + (t - 240 - i * 40) * 3, 40);
            if (t - 240 < i * 40) continue;
            ui_panel(x - 4, y - 3, 250, 13, C_INK, C_WINE);
            spr_draw(&wp_car[SPR[c->foes[i]]][WP_RIGHT], x, y - 1, 0);
            text_draw(c->hint[i], x + 16, y, C_PINK);
        }
    }
    if (cut_i == 5 && t > 200) {
        spr_draw_scaled(&wp_spr[WS_FOXY_BIG], 250, 64, 2, SPR_FLIPX);
    }
    if (t > 40 && (t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 170, C_WHITE);
}

static void draw_ready(void) {
    draw_course();
    char buf[48];
    ui_panel(80, 70, 160, 40, C_INK, run_mode == MODE_VERSUS ? C_PINK : C_SKY);
    if (run_mode == MODE_VERSUS) {
        text_center(WP_COURSE[wp.course].name, 160, 76, C_WHITE);
        tiny_center("P1 BLUE  VS  P2 PINK", 160, 88, C_LIGHT);
        tiny_center("MOST PAINT WINS", 160, 96, C_GREY);
    } else {
        if (wp.course == WP_FINAL) snprintf(buf, sizeof buf, "FINAL SHOWDOWN");
        else snprintf(buf, sizeof buf, "COURSE %d", wp.course + 1);
        text_center(buf, 160, 76, C_WHITE);
        tiny_center(WP_COURSE[wp.course].name, 160, 88, C_SKY);
        snprintf(buf, sizeof buf, "GOAL %d%% BLUE", WP_COURSE[wp.course].goal);
        tiny_center(buf, 160, 96, C_YELLOW);
    }
    if (state_t > READY_TIME - 40) text_center("GO!", 160, 118, (frame_t / 4) % 2 ? C_YELLOW : C_WHITE);
}

static void draw_result(void) {
    draw_course();
    gfx_darken_rect(0, WP_OY, SCREEN_W, SCREEN_H - WP_OY, 1);
    char buf[64];
    int t = state_t;
    if (run_mode == MODE_VERSUS) {
        ui_panel(70, 50, 180, 80, C_INK, res_pass == 1 ? C_SKY : res_pass == 2 ? C_PINK : C_GREY);
        text_center(res_pass == 1 ? "BLUE WINS!" : res_pass == 2 ? "PINK WINS!" : "A DRAW!", 160, 58, C_WHITE);
        snprintf(buf, sizeof buf, "BLUE %d%%   PINK %d%%", res_blue, res_pink);
        text_center(buf, 160, 76, C_LIGHT);
        snprintf(buf, sizeof buf, "WINS %d - %d", vs_wins[0], vs_wins[1]);
        text_center(buf, 160, 92, C_YELLOW);
        if (t > 60 && (t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 114, C_WHITE);
        return;
    }
    int goal = WP_COURSE[course].goal;
    ui_panel(70, 40, 180, 100, C_INK, res_pass ? C_SKY : C_RED);
    static const uint8_t g_ok[] = {C_WHITE, C_SKY, C_BLUE};
    static const uint8_t g_no[] = {C_WHITE, C_PINK, C_RED};
    ui_fancy_center(res_pass ? "COURSE CLEAR!" : "NOT BLUE ENOUGH", 160, 46, 1, res_pass ? g_ok : g_no, 3, C_INK, -1);
    snprintf(buf, sizeof buf, "BLUE %d%%", imin(res_blue, t));
    text_draw(buf, 96, 64, C_SKY);
    snprintf(buf, sizeof buf, "GOAL %d%%", goal);
    text_draw(buf, 176, 64, C_LIGHT);
    snprintf(buf, sizeof buf, "PINK %d%%", res_pink);
    tiny_draw(buf, 96, 76, C_PINK);
    if (t > 50) {
        if (res_pass) {
            snprintf(buf, sizeof buf, "+%d POINTS", res_points);
            text_center(buf, 160, 88, C_YELLOW);
            if (res_life && (t / 10) % 2) text_center("EXTRA LIFE!", 160, 100, C_LIME);
        } else {
            text_center(lives > 0 ? "A LIFE LOST. TRY AGAIN!" : "NO LIVES LEFT", 160, 88, C_PINK);
        }
    }
    snprintf(buf, sizeof buf, "SCORE %d", score);
    tiny_center(buf, 160, 114, C_LIGHT);
    if (res_secret) tiny_center("THE NIGHT PAINTER SAYS: NICE PARKING.", 160, 124, C_YELLOW);
    else if (t > 60 && (t / 20) % 2) tiny_center("PRESS " GLYPH_A, 160, 126, C_WHITE);
}

static void draw_over(void) {
    draw_town(state_t / 4);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(80, 24, 160, 130, C_INK, C_PINK);
    static const uint8_t grad[] = {C_PINK, C_MAGENTA, C_WINE};
    ui_fancy_center("GAME OVER", 160, 32, 2, grad, 3, C_INK, -1);
    spr_draw_scaled(&wp_spr[WS_CRUMPLE], 150, 52, 2, 0);
    char buf[48];
    snprintf(buf, sizeof buf, "COURSE %d  SCORE %d", course + 1, score);
    tiny_center(buf, 160, 78, C_LIGHT);
    draw_board(136, 90, new_rank);
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 140, C_WHITE);
}

static void draw_end(void) {
    int t = state_t;
    draw_town(t);
    gfx_darken_rect(0, 0, SCREEN_W, 132, 1);
    bool plat = score >= ALIEN_SCORE;
    ui_panel(16, 6, 288, 120, C_INK, plat ? C_ICE : C_YELLOW);
    static const uint8_t g_gold[] = {C_WHITE, C_YELLOW, C_AMBER, C_ORANGE};
    static const uint8_t g_plat[] = {C_WHITE, C_ICE, C_CYAN, C_SKY};
    ui_fancy_center("THE WET PAINT CUP", 160, 12, 2, plat ? g_plat : g_gold, 4, C_INK, C_PURPLE);
    /* Bo and Foxy either side of the medal: gold, or platinum for 500 points */
    uint8_t m[PAL_COUNT];
    pal_identity(m);
    if (plat) { m[C_YELLOW] = C_WHITE; m[C_AMBER] = C_ICE; m[C_ORANGE] = C_SKY; }
    int bob = (t / 20) % 2;
    spr_draw_scaled(&wp_spr[WS_BO_BIG], 36, 36 - bob, 2, 0);
    spr_draw_scaled(&wp_spr[WS_FOXY_BIG], 244, 38, 2, SPR_FLIPX);
    if (t > 30) {
        uint8_t mm[PAL_COUNT];
        memcpy(mm, m, sizeof mm);
        const Sprite *md = &wp_spr[WS_MEDAL];
        /* the medal, three times its size */
        for (int y = 0; y < md->h; y++)
            for (int x = 0; x < md->w; x++) {
                uint8_t c = md->px[y * md->w + x];
                if (c != TRANSPARENT) gfx_rect(142 + x * 3, 34 + y * 3, 3, 3, mm[c]);
            }
    }
    if (t > 60) text_center(plat ? "A PLATINUM MEDAL FOR BO!" : "A GOLD MEDAL FOR BO!", 160, 80, C_WHITE);
    if (t > 140) text_center("\"NEXT YEAR, BLUE,\" SAYS FOXY.", 160, 92, C_PINK);
    if (t > 220) text_center("THE TOWN STAYS BLUE TILL THE RAIN.", 160, 104, C_SKY);
    char buf[48];
    snprintf(buf, sizeof buf, "SCORE %d   LIVES LEFT %d", score, lives);
    if (t > 300) tiny_center(buf, 160, 116, C_YELLOW);
    if (t > 420 && (t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 170, C_WHITE);
}

static void draw_vs_pick(void) {
    draw_town(frame_t);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    static const uint8_t grad[] = {C_WHITE, C_SKY, C_BLUE};
    ui_fancy_center("2 PLAYERS", 160, 8, 2, grad, 3, C_INK, C_PURPLE);
    ui_panel(60, 34, 200, 86, C_INK, C_SLATE);
    char buf[48];
    snprintf(buf, sizeof buf, GLYPH_LEFT " %02d %s " GLYPH_RIGHT, vs_course + 1, WP_COURSE[vs_course].name);
    text_center(buf, 160, 40, C_WHITE);
    /* a small map of the course */
    const WpCourse *cd = &WP_COURSE[vs_course];
    for (int y = 0; y < WP_H; y++)
        for (int x = 0; x < WP_W; x++) {
            char ch = cd->rows[y][x];
            int col = ch == '#' || strchr("RLUDo", ch) ? C_SLATE : ch == 'P' ? C_BLUE : ch == 'Q' ? C_MAGENTA : C_NIGHT;
            gfx_rect(108 + x * 4, 54 + y * 4, 4, 4, col);
        }
    snprintf(buf, sizeof buf, "WINS  BLUE %d - %d PINK", vs_wins[0], vs_wins[1]);
    tiny_center(buf, 160, 112, C_LIGHT);
    tiny_center("A: RACE   B: BACK", 160, 124, C_GREY);
}

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    int x = 2, y = 2;
    for (int i = 0; i < WS_CAR_COUNT; i++)
        for (int d = 0; d < 4; d++) {
            spr_draw_scaled(&wp_car[i][d], x, y, 2, 0);
            x += 24;
            if (x > 300) { x = 2; y += 24; }
        }
    for (int i = WS_CAR_COUNT; i < WS_COUNT; i++) {
        const Sprite *s = &wp_spr[i];
        if (x + s->w * 2 > 318) { x = 2; y += 46; }
        spr_draw_scaled(s, x, y, 2, 0);
        x += s->w * 2 + 3;
    }
}

static void wp_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_CUT: draw_cut(); break;
    case S_READY: draw_ready(); break;
    case S_PLAY: draw_course(); break;
    case S_RESULT: draw_result(); break;
    case S_OVER: draw_over(); break;
    case S_END: draw_end(); break;
    case S_VS_PICK: draw_vs_pick(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                   */

static void wp_load(void) {
    wp_art_load();
    wp_audio_load();
}

static void wp_start(void) {
    rng_seed(&seeds, g_rng.state ^ 0x57E7A1ull);
    load_save();
    memset(&wp, 0, sizeof wp);
    memset(parts, 0, sizeof parts);
    sheet_mode = false;
    demo_auto = false;
    menu_sel = 0;
    run_mode = MODE_SOLO;
    to_title();
}

static void wp_quit(void) {
    input_set_versus(false);
    /* a run left half-way still goes on the board */
    if (run_mode == MODE_SOLO && (state == S_PLAY || state == S_READY || state == S_RESULT || state == S_CUT) && score > 0) record_score();
    save_now();
}

static void wp_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_SLATE);
    /* a little maze half blue, half pink */
    for (int yy = 0; yy < h; yy += 12)
        for (int xx = 0; xx < w; xx += 12) {
            bool wall = ((xx / 12) % 3 == 1) && ((yy / 12) % 2 == 1);
            int col = wall ? C_BROWN : (xx + (yy / 12) * 7) < (t / 2) % (w + 40) ? C_BLUE : ((xx / 12 + yy / 12) % 3 == 0 ? C_MAGENTA : C_SLATE);
            gfx_rect(x + xx, y + yy, 12, 12, col);
            if (wall) gfx_hline(x + xx, x + xx + 11, y + yy, C_TAN);
        }
    int cx = (t / 2) % (w + 40) - 10;
    spr_draw(&wp_car[WS_BO][WP_RIGHT], x + cx, y + 25, 0);
    spr_draw(&wp_car[WS_ROLLER][WP_LEFT], x + w - 30 - (t / 3) % 60, y + 1, 0);
    spr_draw(&wp_spr[WS_SPRINKLER], x + 100, y + 39, 0);
}

static int courses_ok(void);

static int wp_query(const char *key, int *out) {
    const WpCar *c = &wp.car[0];
    int32_t px, py;
    wp_mover_pos(&c->m, &px, &py);
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "course")) { *out = course + 1; return 1; }
    if (!strcmp(key, "sim_course")) { *out = wp.course + 1; return 1; }
    if (!strcmp(key, "lives")) { *out = lives; return 1; }
    if (!strcmp(key, "score")) { *out = score; return 1; }
    if (!strcmp(key, "blue")) { *out = wp_percent(PAINT_BLUE); return 1; }
    if (!strcmp(key, "pink")) { *out = wp_percent(PAINT_PINK); return 1; }
    if (!strcmp(key, "blue_tiles")) { *out = wp_count(PAINT_BLUE); return 1; }
    if (!strcmp(key, "pink_tiles")) { *out = wp_count(PAINT_PINK); return 1; }
    if (!strcmp(key, "total")) { *out = wp.total; return 1; }
    if (!strcmp(key, "goal")) { *out = WP_COURSE[wp.course].goal; return 1; }
    if (!strcmp(key, "time")) { *out = wp.frames_left; return 1; }
    if (!strcmp(key, "tx")) { *out = c->m.tx; return 1; }
    if (!strcmp(key, "ty")) { *out = c->m.ty; return 1; }
    if (!strcmp(key, "px")) { *out = px / WP_UNIT; return 1; }
    if (!strcmp(key, "py")) { *out = py / WP_UNIT; return 1; }
    if (!strcmp(key, "dir")) { *out = c->m.dir; return 1; }
    if (!strcmp(key, "speed")) { *out = c->speed; return 1; }
    if (!strcmp(key, "stopped")) { *out = c->stopped; return 1; }
    if (!strcmp(key, "boosted")) { *out = wp_car_boosted(c); return 1; }
    if (!strcmp(key, "boost_lv")) { *out = c->boost_t > 0 ? c->boost_lv : 0; return 1; }
    if (!strcmp(key, "stun")) { *out = c->stun_t; return 1; }
    if (!strcmp(key, "spin")) { *out = c->spin; return 1; }
    if (!strcmp(key, "gun")) { *out = c->gun_t > 0 ? c->gun : 0; return 1; }
    if (!strcmp(key, "foes")) { *out = wp_foe_count(); return 1; }
    if (!strcmp(key, "kills")) { *out = wp.kills; return 1; }
    if (!strcmp(key, "freeze")) { *out = wp.freeze_t; return 1; }
    if (!strcmp(key, "swing")) { *out = wp.swing_open; return 1; }
    if (!strcmp(key, "bollard")) { *out = wp.bollard_phase; return 1; }
    if (!strcmp(key, "secret")) { *out = wp.secret; return 1; }
    if (!strcmp(key, "res_secret")) { *out = res_secret; return 1; }
    if (!strcmp(key, "res_pass")) { *out = res_pass; return 1; }
    if (!strcmp(key, "rival")) { *out = wp.car[1].on; return 1; }
    if (!strcmp(key, "rival_stun")) { *out = wp.car[1].stun_t; return 1; }
    if (!strcmp(key, "rival_human")) { *out = wp.car[1].human; return 1; }
    if (!strcmp(key, "rival_dir")) { *out = wp.car[1].m.dir; return 1; }
    if (!strcmp(key, "rival_tx")) { *out = wp.car[1].m.tx; return 1; }
    if (!strcmp(key, "rival_ty")) { *out = wp.car[1].m.ty; return 1; }
    if (!strcmp(key, "garages")) { *out = wp.n_garages; return 1; }
    if (!strcmp(key, "stuns")) { *out = wp.stuns; return 1; }
    if (!strcmp(key, "courses_ok")) { *out = courses_ok(); return 1; }
    if (!strncmp(key, "drone0_", 7)) {
        const WpDrone *d = &wp.drone[0];
        if (!strcmp(key + 7, "on")) { *out = d->on; return 1; }
        if (!strcmp(key + 7, "tx")) { *out = d->m.tx; return 1; }
        if (!strcmp(key + 7, "ty")) { *out = d->m.ty; return 1; }
        return 0;
    }
    if (!strcmp(key, "garage0")) { *out = wp.n_garages ? wp.garage[0].phase : -1; return 1; }
    if (!strcmp(key, "cut")) { *out = state == S_CUT ? cut_i : -1; return 1; }
    if (!strcmp(key, "best_course")) { *out = sv.best_course; return 1; }
    if (!strcmp(key, "best_lives")) { *out = sv.best_lives; return 1; }
    if (!strcmp(key, "new_rank")) { *out = new_rank; return 1; }
    if (!strcmp(key, "vs_course")) { *out = vs_course + 1; return 1; }
    if (!strcmp(key, "vs_blue_wins")) { *out = vs_wins[0]; return 1; }
    if (!strcmp(key, "vs_pink_wins")) { *out = vs_wins[1]; return 1; }
    if (!strcmp(key, "menu")) { *out = menu_sel; return 1; }
    if (!strcmp(key, "drones")) {
        *out = 0;
        for (int i = 0; i < WP_MAX_DRONES; i++) *out += wp.drone[i].on;
        return 1;
    }
    if (!strcmp(key, "shots")) {
        *out = 0;
        for (int i = 0; i < WP_MAX_SHOTS; i++) *out += wp.shot[i].on;
        return 1;
    }
    if (!strcmp(key, "bot")) {
        if (state == S_PLAY) *out = wp_bot_buttons(0);
        else if (state == S_TITLE || state == S_CUT || state == S_RESULT || state == S_OVER || state == S_END || state == S_VS_PICK)
            *out = (state_t / 8) % 2 ? BTN_A : 0;
        else *out = 0;
        return 1;
    }
    if (!strncmp(key, "hi", 2) && key[2] >= '0' && key[2] < '0' + HISCORES && !key[3]) { *out = (int)sv.hi[key[2] - '0']; return 1; }
    if (!strncmp(key, "foes_", 5)) {
        static const char *const N[F_KINDS] = {"roller", "duster", "tanker", "gloop", "biggloop", "popper", "hedgehog", "conker", "jelly"};
        for (int k = 0; k < F_KINDS; k++)
            if (!strcmp(key + 5, N[k])) { *out = wp_foe_count_kind(k); return 1; }
        return 0;
    }
    if (!strncmp(key, "paint_", 6)) {
        int x = atoi(key + 6), y = atoi(strchr(key + 6, '_') + 1);
        *out = x >= 0 && y >= 0 && x < WP_W && y < WP_H ? wp.paint[y][x] : -1;
        return 1;
    }
    if (!strncmp(key, "solid_", 6)) {
        int x = atoi(key + 6), y = atoi(strchr(key + 6, '_') + 1);
        *out = wp_solid(x, y);
        return 1;
    }
    if (!strncmp(key, "foe_", 4) && strchr(key + 4, '_')) {
        /* foe_I_tx / foe_I_ty / foe_I_on / foe_I_armed */
        int i = atoi(key + 4);
        const char *f = strchr(key + 4, '_') + 1;
        if (i < 0 || i >= WP_MAX_FOES) return 0;
        const WpFoe *fo = &wp.foe[i];
        if (!strcmp(f, "on")) { *out = fo->on; return 1; }
        if (!strcmp(f, "tx")) { *out = fo->m.tx; return 1; }
        if (!strcmp(f, "ty")) { *out = fo->m.ty; return 1; }
        if (!strcmp(f, "armed")) { *out = fo->armed; return 1; }
        if (!strcmp(f, "plan")) { *out = fo->plan; return 1; }
        if (!strcmp(f, "dir")) { *out = fo->m.dir; return 1; }
        if (!strcmp(f, "prog")) { *out = fo->m.prog; return 1; }
        if (!strcmp(f, "fx")) { *out = fo->fx / WP_UNIT; return 1; }
        if (!strcmp(f, "fy")) { *out = fo->fy / WP_UNIT; return 1; }
        return 0;
    }
    return 0;
}

/* Every course is sound: one start each, every floor tile reachable from
 * yours (barriers, bollards and thorns count as passable, since they open),
 * and every garage opens onto floor. Returns how many courses pass. */
static int courses_ok(void) {
    int good = 0;
    for (int c = 0; c < WP_COURSES; c++) {
        const WpCourse *cd = &WP_COURSE[c];
        static uint8_t seen[WP_H][WP_W];
        static int16_t st[WP_W * WP_H];
        memset(seen, 0, sizeof seen);
        int ps = 0, qs = 0, px = 0, py = 0, floor = 0, reached = 0, bad = 0;
        for (int y = 0; y < WP_H; y++) {
            if ((int)strlen(cd->rows[y]) != WP_W) bad = 1;
            for (int x = 0; x < WP_W && !bad; x++) {
                char ch = cd->rows[y][x];
                if (ch == 'P') { ps++; px = x; py = y; }
                if (ch == 'Q') qs++;
                if (!strchr("#RLUDo", ch)) floor++;
            }
        }
        if (bad || ps != 1 || qs != 1) continue;
        int n = 0;
        st[n++] = (int16_t)(py * WP_W + px);
        seen[py][px] = 1;
        while (n > 0) {
            int q = st[--n], x = q % WP_W, y = q / WP_W;
            reached++;
            for (int d = 0; d < 4; d++) {
                int nx = x + WP_DX[d], ny = y + WP_DY[d];
                if (nx < 0 || ny < 0 || nx >= WP_W || ny >= WP_H || seen[ny][nx] || strchr("#RLUDo", cd->rows[ny][nx])) continue;
                seen[ny][nx] = 1;
                st[n++] = (int16_t)(ny * WP_W + nx);
            }
        }
        for (int y = 0; y < WP_H; y++)
            for (int x = 0; x < WP_W; x++) {
                const char *g = strchr("RDLU", cd->rows[y][x]);
                if (!g || !cd->rows[y][x]) continue;
                static const int GD[4] = {WP_RIGHT, WP_DOWN, WP_LEFT, WP_UP};
                int d = GD[g - "RDLU"], nx = x + WP_DX[d], ny = y + WP_DY[d];
                if (nx < 0 || ny < 0 || nx >= WP_W || ny >= WP_H || !seen[ny][nx]) bad = 1;
            }
        if (!bad && reached == floor) good++;
    }
    return good;
}

static int kind_of(const char *n) {
    static const char *const N[F_KINDS] = {"roller", "duster", "tanker", "gloop", "biggloop", "popper", "hedgehog", "conker", "jelly"};
    for (int k = 0; k < F_KINDS; k++)
        if (!strcmp(n, N[k])) return k;
    return -1;
}

static int wp_cheat(const char *cmd) {
    int a, b, c, d;
    char name[24];
    if (!strcmp(cmd, "new")) { new_run(); return 1; }
    if (sscanf(cmd, "course %d", &a) == 1) {
        /* straight to course N (1-26) of a one-player run, at its READY */
        run_mode = MODE_SOLO;
        input_set_versus(false);
        if (lives <= 0) lives = START_LIVES;
        course = iclamp(a - 1, 0, WP_COURSES - 1);
        to_ready();
        return 1;
    }
    if (sscanf(cmd, "cutscene %d", &a) == 1) { run_mode = MODE_SOLO; course = iclamp(a - 1, 0, WP_COURSES - 1); to_course(course); return 1; }
    if (!strcmp(cmd, "go")) { if (state == S_READY) { state = S_PLAY; state_t = 0; } return 1; }
    if (sscanf(cmd, "versus %d", &a) == 1) {
        run_mode = MODE_VERSUS;
        input_set_versus(true);
        course = iclamp(a - 1, 0, WP_COURSES - 1);
        vs_course = course;
        to_ready();
        return 1;
    }
    if (sscanf(cmd, "lives %d", &a) == 1) { lives = a; return 1; }
    if (sscanf(cmd, "score %d", &a) == 1) { score = a; return 1; }
    if (sscanf(cmd, "time %d", &a) == 1) { wp.frames_left = a; return 1; }
    if (!strcmp(cmd, "nofoes")) { wp.no_foes = 1; memset(wp.foe, 0, sizeof wp.foe); return 1; }
    if (!strcmp(cmd, "foes_on")) { wp.no_foes = 0; return 1; }
    if (!strcmp(cmd, "noclock")) { wp.no_clock = 1; return 1; }
    if (!strcmp(cmd, "clock_on")) { wp.no_clock = 0; return 1; }
    if (!strcmp(cmd, "nogarages")) { wp.n_garages = 0; return 1; }
    if (!strcmp(cmd, "still")) { wp.foes_still ^= 1; return 1; }
    if (!strcmp(cmd, "killall")) { for (int i = 0; i < WP_MAX_FOES; i++) wp.foe[i].on = 0; return 1; }
    if (sscanf(cmd, "foe_at_drone %23s", name) == 1) {
        int k = kind_of(name);
        if (k < 0 || !wp.drone[0].on) return 0;
        int32_t x, y;
        wp_mover_pos(&wp.drone[0].m, &x, &y);
        wp_add_foe(k, x / WP_TU, y / WP_TU, 0, -1);
        return 1;
    }
    if (!strcmp(cmd, "freeze_rival")) { wp.freeze_t = 30000; wp.freeze_team = PAINT_PINK; return 1; }
    if (!strcmp(cmd, "autodrive")) { demo_auto = !demo_auto; return 1; }
    if (sscanf(cmd, "fill %d %d", &a, &b) == 2) {
        /* paint the first N paintable tiles (reading order) in a colour */
        int n = 0;
        for (int y = 0; y < WP_H && n < b; y++)
            for (int x = 0; x < WP_W && n < b; x++) {
                int k = wp.tile[y][x].kind;
                if (k == TK_WALL || k == TK_GARAGE || k == TK_BUMPER) continue;
                wp.paint[y][x] = (uint8_t)a;
                n++;
            }
        return 1;
    }
    if (!strcmp(cmd, "wipe")) { memset(wp.paint, 0, sizeof wp.paint); return 1; }
    if (sscanf(cmd, "paint %d %d %d", &a, &b, &c) == 3) { wp.paint[b][a] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "car %d %d %d", &a, &b, &c) == 3) {
        WpCar *cr = &wp.car[0];
        cr->m.tx = (int8_t)a; cr->m.ty = (int8_t)b; cr->m.dir = (int8_t)(c & 3); cr->m.prog = 0;
        cr->stopped = 0; cr->speed = 16; cr->want = -1; cr->last_tx = (int8_t)a; cr->last_ty = (int8_t)b;
        cr->stun_t = 0; cr->inv_t = 0; cr->spin = 0; cr->boost_t = 0;
        return 1;
    }
    if (sscanf(cmd, "rival %d %d %d", &a, &b, &c) == 3) {
        WpCar *cr = &wp.car[1];
        cr->on = 1;
        cr->m.tx = (int8_t)a; cr->m.ty = (int8_t)b; cr->m.dir = (int8_t)(c & 3); cr->m.prog = 0;
        cr->stopped = 0; cr->speed = 16; cr->want = -1; cr->last_tx = (int8_t)a; cr->last_ty = (int8_t)b;
        cr->stun_t = 0; cr->inv_t = 0;
        return 1;
    }
    if (sscanf(cmd, "put %23s %d %d %d", name, &a, &b, &c) == 4) {
        int k = kind_of(name);
        if (k < 0) return 0;
        wp_add_foe(k, a, b, c & 3, -1);
        return 1;
    }
    if (sscanf(cmd, "item %d %d %d", &a, &b, &c) == 3) { wp.tile[b][a].item = (uint8_t)c; return 1; }
    if (sscanf(cmd, "tile %d %d %d %d", &a, &b, &c, &d) == 4) { wp.tile[b][a].kind = (uint8_t)c; wp.tile[b][a].dir = (uint8_t)d; if (c == TK_THORN) wp.thorn[b][a] = 1; return 1; }
    if (sscanf(cmd, "foeage %d %d", &a, &b) == 2) { if (a >= 0 && a < WP_MAX_FOES) wp.foe[a].age = (int16_t)b; return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    return 0;
}

const GameDef GAME_WETPAINT = {
    "wetpaint",
    "WET PAINT",
    "1983",
    "ARCADE",
    "PAINT THE TOWN BLUE BEFORE THE CLOCK RUNS OUT!",
    {"PASS THE FIRST 12 COURSES", "BEAT ALL 25 AND FOXY", "WIN WITH 500 POINTS"},
    GLYPH_DPAD "\tSTEER (TURNS AT THE NEXT TILE)\n"
    "BACK\tBRAKE\n"
    "AHEAD\tSPEED BACK UP\n"
    "START\tPAUSE",
    C_BLUE, C_MAGENTA,
    wp_load, wp_start, wp_update, wp_draw, wp_quit, wp_label, wp_query, wp_cheat,
    "PAINT CHASE", 4,
};
