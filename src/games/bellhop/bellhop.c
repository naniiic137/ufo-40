/* BELLHOP - Ansel flies the Tinkler through fifty one-screen stages in five
 * worlds to Lady Hush's citadel, and stops for tea on the way.
 * Cartridge 17 of UFO 40, a tribute to Campanella (UFO 50 #17).
 * See docs/games/17-bellhop.md. The rules live in bellhop_stage.c and
 * bellhop_boss.c; this file is the screens, the run (score, ships, tea),
 * the stats, the goals, the code screen and the test hooks.
 *
 * Like the original, a run is played in one sitting: it is not saved. The
 * cartridge keeps only its records (the high score and the stats). */
#include "bellhop.h"

enum { S_TITLE, S_CODE, S_STORY, S_WORLD, S_PLAY, S_TEA, S_OVER, S_ENDING, S_CREDITS };

typedef struct Save {
    uint32_t magic;
    uint32_t hiscore;
    uint32_t fuel_used;      /* frames of thrust, every run */
    uint32_t crashes;
    uint16_t most_ships;
    uint16_t wins, runs;
    uint8_t best_tea;
    uint8_t pad;
} Save;
#define SAVE_MAGIC 0x42485001u

typedef struct Run {
    int stage;
    int score;
    int ships;               /* spare ships */
    int next_extend;
    uint8_t cup[BHP_CUPS];
    bool brew;               /* the BREW-ROOM code: the crystal rooms alone */
    bool counted;
    int brew_room;
    int deaths;              /* crashes this run */
    int last_t;              /* frames the last stage took */
} Run;

typedef struct Part { int16_t x, y, vx, vy; uint8_t life, col, on; } Part; /* 1/16 px */
#define NPARTS 96

static Save sv;
static Run run;
static BhpStage st;
static BhpBot bot;
static Part parts[NPARTS];
static int state, state_t, frame_t;
static int title_sel;
static char code[9] = "AAAAAAAA";
static int code_pos;
static const char *code_msg;
static int code_msg_t;
static bool brew_on;
static int credits_t;
static int banner_t;
static int oneup_t;
static bool owl_seen;
static bool bot_warps;
static int last_world = -1;

#define CODE_BREW "BREWROOM"
static const int BREW_ROOMS[BHP_WORLDS] = {4, 14, 24, 34, 44};

const char *const BHP_WORLD_NAME[BHP_WORLDS] = {"MILLBROOK", "THE ORCHARD", "THE CLOCKWORKS", "THE SUGARWORKS", "HUSH CITADEL"};

/* ------------------------------------------------------------------ */
/* saving                                                               */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    static Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else {
        memset(&sv, 0, sizeof sv);
        sv.magic = SAVE_MAGIC;
    }
}

/* ------------------------------------------------------------------ */
/* helpers                                                              */

static void goto_state(int s) {
    state = s;
    state_t = 0;
    input_consume();
}

static int tea_count(void) {
    int n = 0;
    for (int i = 0; i < BHP_CUPS; i++) n += run.cup[i];
    return n;
}

static int world_tea(int w) {
    int n = 0;
    for (int i = 0; i < 8; i++) n += run.cup[w * 8 + i];
    return n;
}

static void spark(int x, int y, int n, int col, int speed) {
    for (int k = 0; k < n; k++)
        for (int i = 0; i < NPARTS; i++)
            if (!parts[i].on) {
                int a = rng_range(&g_rng, 0, 255), v = rng_range(&g_rng, speed / 3, speed);
                parts[i] = (Part){(int16_t)(x * 16), (int16_t)(y * 16), (int16_t)(bhp_cos(a) * v / 127), (int16_t)(bhp_sin(a) * v / 127),
                                  (uint8_t)rng_range(&g_rng, 14, 30), (uint8_t)col, 1};
                break;
            }
}

/* the records a run keeps (none while the code is on) */
static void note_ships(void) {
    if (run.brew) return;
    if (run.ships > sv.most_ships) sv.most_ships = (uint16_t)run.ships;
    if (run.ships >= BHP_GIFT_SHIPS) game_award(GOAL_BEACON);
}

static void add_points(int pts) {
    if (pts <= 0) return;
    run.score = imin(99999, run.score + pts);
    while (run.score >= run.next_extend) {
        run.next_extend += BHP_EXTEND;
        if (run.ships < BHP_MAX_SHIPS) run.ships++;
        oneup_t = 60;
        sfx_play_name("bhp_oneup");
        note_ships();
    }
    if (!run.brew && (uint32_t)run.score > sv.hiscore) sv.hiscore = (uint32_t)run.score;
}

static int stage_song(int idx) {
    int k = bhp_kind(idx);
    if (k == BHK_BONUS) return BHP_MUS_BONUS;
    if (k == BHK_BOSS) return idx == BHP_STAGES - 1 ? BHP_MUS_LADY : BHP_MUS_BOSS;
    return BHP_MUS_WORLD[idx / BHP_PER_WORLD];
}

/* ------------------------------------------------------------------ */
/* the flow                                                             */

static void title_enter(void) {
    goto_state(S_TITLE);
    game_set_pausable(false);
    music_play(BHP_MUS_TITLE);
}

static void play_stage(int idx) {
    run.stage = idx;
    uint64_t seed = (uint64_t)rng_next(&g_rng) << 32 | rng_next(&g_rng);
    bhp_stage_load(&st, idx, run.brew ? 100 : 200, seed);
    memset(parts, 0, sizeof parts);
    banner_t = 120;
    goto_state(S_PLAY);
    game_set_pausable(true);
    music_play(stage_song(idx));
}

static void world_enter(int w) {
    last_world = w;
    goto_state(S_WORLD);
    game_set_pausable(true);
    music_play(BHP_MUS_WORLD[w]);
}

static void new_run(bool brew) {
    memset(&run, 0, sizeof run);
    run.ships = BHP_START_SHIPS;
    run.next_extend = BHP_EXTEND;
    run.brew = brew;
    owl_seen = false;
    if (!brew) {
        if (sv.runs < 65535) sv.runs++;
        save_now();
    }
}

static void go_next(void) {
    run.last_t = (int)st.t;
    if (run.brew) {
        run.brew_room++;
        if (run.brew_room >= BHP_WORLDS) { goto_state(S_OVER); music_play(BHP_MUS_TEA); return; }
        play_stage(BREW_ROOMS[run.brew_room]);
        return;
    }
    int idx = run.stage;
    if (bhp_kind(idx) == BHK_BOSS) {
        if (idx == BHP_STAGES - 1) {
            /* Lady Hush is beaten: the bells come home */
            if (!run.counted) {
                run.counted = true;
                game_award(GOAL_SAUCER);
                if (tea_count() == BHP_CUPS) game_award(GOAL_ALIEN);
                if (sv.wins < 65535) sv.wins++;
                if (tea_count() > sv.best_tea) sv.best_tea = (uint8_t)tea_count();
                save_now();
            }
            goto_state(S_ENDING);
            game_set_pausable(false);
            music_play(BHP_MUS_END);
            return;
        }
        goto_state(S_TEA);
        music_play(BHP_MUS_TEA);
        return;
    }
    play_stage(idx + 1);
}

static void game_over(void) {
    if (!run.brew) {
        if (tea_count() > sv.best_tea) sv.best_tea = (uint8_t)tea_count();
        save_now();
    }
    goto_state(S_OVER);
    game_set_pausable(false);
    music_play(BHP_MUS_OVER);
}

/* ------------------------------------------------------------------ */
/* play                                                                 */

static void stage_news(void) {
    uint32_t ev = st.ev;
    for (int k = 0; k < st.nfx; k++) {
        const BhpFx *f = &st.fx[k];
        switch (f->kind) {
        case 1: spark(f->x, f->y, 24, C_ORANGE, 40); spark(f->x, f->y, 12, C_YELLOW, 28); spark(f->x, f->y, 8, C_SKY, 20); break;
        case 2: spark(f->x, f->y, 10, C_WHITE, 26); spark(f->x, f->y, 6, C_YELLOW, 18); break;
        case 3: spark(f->x, f->y, 8, C_ICE, 22); break;
        case 4: spark(f->x, f->y, 8, C_TAN, 22); break;
        case 5: spark(f->x, f->y, 8, C_ICE, 30); break;
        case 7: spark(f->x, f->y, 10, C_CYAN, 30); break;
        case 8: spark(f->x, f->y, 18, C_ORANGE, 44); spark(f->x, f->y, 8, C_RED, 30); break;
        case 9: spark(f->x, f->y, 16, C_PINK, 34); break;
        case 10: spark(f->x, f->y, 8, C_RED, 20); break;
        case 11: spark(f->x, f->y, 5, C_YELLOW, 18); break;
        case 12: case 13: spark(f->x, f->y, 14, C_WHITE, 30); spark(f->x, f->y, 6, C_CREAM, 20); break;
        case 14: spark(f->x, f->y, 10, C_ICE, 24); break;
        case 15: spark(f->x, f->y, 8, C_SKY, 24); break;
        default: spark(f->x, f->y, 4, C_WHITE, 16); break;
        }
    }
    if (ev & BEV_CRASH) sfx_play_name("bhp_crash");
    else if (ev & BEV_BLAST) sfx_play_name("bhp_blast");
    else if (ev & BEV_BOSS_DOWN) sfx_play_name("bhp_blast");
    else if (ev & BEV_BOSS_HIT) sfx_play_name("bhp_bosshit");
    else if (ev & BEV_BREAK) sfx_play_name("bhp_break");
    else if (ev & BEV_SLASH) sfx_play_name("bhp_slash");
    else if (ev & BEV_LEVER) sfx_play_name("bhp_lever");
    else if (ev & BEV_FUSE) sfx_play_name("bhp_fuse");
    else if (ev & BEV_SHOOT && (frame_t & 3) == 0) sfx_play_name("bhp_shoot");
    if (ev & BEV_CUP) sfx_play_name("bhp_cup");
    else if (ev & BEV_CUP_SHOW) sfx_play_name("bhp_show");
    else if (ev & BEV_CIRCLER) sfx_play_name("bhp_circler");
    else if (ev & BEV_COIN) sfx_play_name("bhp_coin");
    else if (ev & BEV_BIGCOIN) sfx_play_name("bhp_bigcoin");
    else if (ev & BEV_CRYSTAL) sfx_play_name("bhp_crystal");
    else if (ev & BEV_KILL) sfx_play_name("bhp_kill");
    else if (ev & (BEV_EXIT_OPEN | BEV_CLEAR)) sfx_play_name("bhp_open");
    else if (ev & BEV_WARP) sfx_play_name("bhp_warp");
    else if (ev & BEV_ROUND) sfx_play_name("bhp_round");
    else if (ev & BEV_NODE) sfx_play_name("bhp_node");
    else if (ev & BEV_HIT) sfx_play_name("bhp_hit");
    else if (ev & BEV_POP) sfx_play_name("bhp_pop");
    else if (ev & BEV_DRY) sfx_play_name("bhp_dry");
    if (ev & BEV_CLEAR) music_play(BHP_MUS_CLEAR);
}

static unsigned read_ctl(void) {
    unsigned c = 0;
    if (btn(BTN_LEFT)) c |= CHF_LEFT;
    if (btn(BTN_RIGHT)) c |= CHF_RIGHT;
    if (btn(BTN_A)) c |= CHF_THRUST;
    if (btnp(BTN_B)) c |= BHP_CTL_SLASH;
    return c;
}

static void update_play(void) {
    if (banner_t > 0) banner_t--;
    if (oneup_t > 0) oneup_t--;
    bhp_stage_step(&st, read_ctl());
    stage_news();
    add_points(st.pts);
    if (st.thrust_frames && !run.brew) sv.fuel_used += (uint32_t)st.thrust_frames;
    if (st.ev & BEV_CUP) {
        int ci = bhp_cup_index(st.idx);
        if (ci >= 0 && !run.brew) run.cup[ci] = 1;
    }
    if (st.ev & BEV_CRASH) run.deaths++;
    if (st.ev & BEV_CRASH && !run.brew && sv.crashes < 0xFFFFFFFFu) sv.crashes++;
    if (st.idx == 0 && st.t >= BHP_OWL_T) owl_seen = true;
    for (int i = 0; i < NPARTS; i++) {
        Part *p = &parts[i];
        if (!p->on) continue;
        p->x = (int16_t)(p->x + p->vx);
        p->y = (int16_t)(p->y + p->vy);
        p->vy = (int16_t)(p->vy + 1);
        if (--p->life == 0) p->on = 0;
    }
    switch (st.mode) {
    case BSM_DEAD:
        if (st.mode_t >= BHP_DEAD_T) {
            if (run.ships <= 0) { game_over(); return; }
            run.ships--;
            bhp_stage_respawn(&st);
            banner_t = 60;
        }
        break;
    case BSM_CLEAR:
        if (st.mode_t >= BHP_CLEAR_T) go_next();
        break;
    case BSM_WARP:
        if (st.mode_t >= BHP_CLEAR_T) play_stage(st.warp_to);
        break;
    default: break;
    }
}

/* ------------------------------------------------------------------ */
/* updates                                                              */

static void check_code(void) {
    if (!strcmp(code, CODE_BREW)) {
        brew_on = !brew_on;
        code_msg = brew_on ? "BREW-ROOM: THE CRYSTAL ROOMS ALONE" : "BREW-ROOM IS OFF";
        sfx_play_name("bhp_crystal");
    } else {
        code_msg = "NO SUCH CODE";
        sfx_play_name("ui_error");
    }
    code_msg_t = 150;
}

static void update_code(void) {
    if (code_msg_t > 0) code_msg_t--;
    if (btn_repeat(BTN_LEFT)) { code_pos = (code_pos + 7) % 8; sfx_play_name("bhp_move"); }
    if (btn_repeat(BTN_RIGHT)) { code_pos = (code_pos + 1) % 8; sfx_play_name("bhp_move"); }
    if (btn_repeat(BTN_UP)) { code[code_pos] = (char)(code[code_pos] == 'Z' ? 'A' : code[code_pos] + 1); sfx_play_name("bhp_move"); }
    if (btn_repeat(BTN_DOWN)) { code[code_pos] = (char)(code[code_pos] == 'A' ? 'Z' : code[code_pos] - 1); sfx_play_name("bhp_move"); }
    if (btnp(BTN_B)) { sfx_play_name("ui_back"); goto_state(S_TITLE); return; }
    if (btnp(BTN_A) && state_t > 5) check_code();
}

static void start_game(void) {
    new_run(brew_on);
    if (run.brew) { run.brew_room = 0; play_stage(BREW_ROOMS[0]); }
    else { goto_state(S_STORY); game_set_pausable(false); }
}

static void update(void) {
    frame_t++;
    state_t++;
    switch (state) {
    case S_TITLE:
        if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { title_sel ^= 1; sfx_play_name("bhp_move"); }
        if (btnp(BTN_B) && state_t > 10) { game_exit_to_library(); return; }
        if (state_t > 15 && btnp(BTN_A)) {
            sfx_play_name("ui_ok");
            if (title_sel == 0) start_game();
            else { code_pos = 0; code_msg = NULL; goto_state(S_CODE); }
        }
        break;
    case S_CODE: update_code(); break;
    case S_STORY:
        if (state_t > 30 && btnp(BTN_A)) { sfx_play_name("ui_ok"); world_enter(0); }
        break;
    case S_WORLD:
        if ((state_t > 30 && btnp(BTN_A)) || state_t > 150) play_stage(last_world * BHP_PER_WORLD);
        break;
    case S_PLAY: update_play(); break;
    case S_TEA:
        if (state_t > 60 && btnp(BTN_A)) { sfx_play_name("ui_ok"); world_enter(run.stage / BHP_PER_WORLD + 1); }
        break;
    case S_OVER:
        if (state_t > 90 && btnp(BTN_A)) { sfx_play_name("ui_ok"); title_enter(); }
        break;
    case S_ENDING:
        if (state_t > 90 && btnp(BTN_A)) { sfx_play_name("ui_ok"); goto_state(S_CREDITS); credits_t = 0; }
        break;
    case S_CREDITS:
        credits_t += btn(BTN_A) ? 4 : 1;
        if (credits_t > 60 * 22 && btnp(BTN_A | BTN_B)) title_enter();
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */

static const uint8_t GRAD[] = {C_WHITE, C_ICE, C_CYAN, C_SKY};
static const uint8_t GOLD[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};

static void draw_hud(void) {
    gfx_rect(0, 0, SCREEN_W, BHP_OY, C_INK);
    gfx_hline(0, SCREEN_W - 1, BHP_OY - 1, C_NIGHT);
    /* fuel: a red bar of upright segments, top left */
    int segs = 20, full = (st.fuel * segs + BHP_FUEL_MAX - 1) / BHP_FUEL_MAX;
    bool low = st.fuel < BHP_FUEL_MAX / 5;
    for (int k = 0; k < segs; k++) {
        int col = k < full ? (low && (frame_t / 6) & 1 ? C_ORANGE : C_RED) : C_MAROON;
        if (k >= full) col = C_NIGHT;
        gfx_rect(2 + k * 3, 2, 2, 8, col);
    }
    /* spare ships */
    bhp_draw_ship(72, 6, 1, false, 0);
    char b[32];
    snprintf(b, sizeof b, "%02d", run.ships);
    text_draw(b, 81, 3, oneup_t && (frame_t / 4) & 1 ? C_YELLOW : C_WHITE);
    /* score and the green bar to the next ship */
    snprintf(b, sizeof b, "%05d", run.score);
    text_draw(b, 102, 2, C_WHITE);
    int into = BHP_EXTEND - (run.next_extend - run.score);
    gfx_rect(102, 10, 30, 1, C_FOREST);
    gfx_rect(102, 10, iclamp(into * 30 / BHP_EXTEND, 0, 30), 1, C_LIME);
    /* the stage */
    text_draw(bhp_stage_label(st.idx), 142, 2, C_CYAN);
    /* (the tea stays a secret here: the TEA BREAK card after each boss tallies it) */
    if (run.brew) {
        snprintf(b, sizeof b, "ROOM %d OF 5", run.brew_room + 1);
        tiny_draw(b, 232, 4, C_LIME);
    }
}

static void draw_parts(void) {
    for (int i = 0; i < NPARTS; i++) {
        const Part *p = &parts[i];
        if (p->on) gfx_pset(p->x / 16, p->y / 16, p->life < 6 ? C_SLATE : p->col);
    }
}

static void draw_play(void) {
    bhp_draw_stage(&st, frame_t, st.idx == 0 && owl_seen);
    draw_parts();
    draw_hud();
    if (banner_t > 0 && st.mode == BSM_BUBBLE) {
        char b[48];
        const char *name = BHP_STAGE[st.idx].name;
        snprintf(b, sizeof b, "%s  %s", bhp_stage_label(st.idx), name ? name : "");
        int w = text_width(b);
        gfx_rect(160 - w / 2 - 6, 28, w + 12, 13, C_INK);
        text_center(b, 160, 31, C_YELLOW);
    }
    if (st.kind == BHK_BONUS && st.round && !st.bonus_done && st.mode == BSM_FLY) {
        char b[24];
        snprintf(b, sizeof b, "ROUND %d", st.round);
        tiny_draw(b, 4, BHP_OY + 3, C_WHITE);
        int left = BHP_ROUND_T - st.round_t;
        gfx_rect(4, BHP_OY + 10, left * 40 / BHP_ROUND_T, 2, C_CYAN);
    }
    if (st.mode == BSM_DEAD && run.ships <= 0 && st.mode_t > 30) {
        gfx_rect(110, 80, 100, 16, C_INK);
        text_center("LAST SHIP", 160, 84, C_RED);
    }
}

static void draw_title(void) {
    bhp_draw_world_backdrop(0, frame_t);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    ui_fancy_center("BELLHOP", 160, 14, 4, GOLD, 4, C_INK, C_WINE);
    tiny_center("FIVE WORLDS " GLYPH_DOT " FIFTY STAGES " GLYPH_DOT " FORTY CUPS OF TEA", 160, 52, C_CREAM);
    int sx = 160 + bhp_sin(frame_t) * 60 / 127, sy = 72 + bhp_sin(frame_t * 3) * 4 / 127;
    bhp_draw_ship(sx, sy, bhp_cos(frame_t) >= 0 ? 1 : -1, (frame_t / 4) % 3 == 0, frame_t);
    static const char *const ITEMS[2] = {"START", "CODE"};
    ui_panel(120, 86, 80, 32, C_INK, C_SKY);
    for (int k = 0; k < 2; k++) {
        int y = 92 + k * 12;
        text_center(ITEMS[k], 160, y, title_sel == k ? C_YELLOW : C_LIGHT);
        if (title_sel == k) ui_cursor(126, y, frame_t);
    }
    if (brew_on) {
        gfx_rect(110, 121, 100, 9, C_INK);
        tiny_center("BREW-ROOM IS ON", 160, 123, C_LIME);
    }
    gfx_rect(0, 140, SCREEN_W, 40, C_INK);
    char b[96];
    snprintf(b, sizeof b, "HIGH SCORE %05lu  " GLYPH_DOT "  MOST SHIPS %d  " GLYPH_DOT "  BEST TEA %d/40",
             (unsigned long)sv.hiscore, sv.most_ships, sv.best_tea);
    tiny_center(b, 160, 145, C_YELLOW);
    unsigned long tanks10 = (unsigned long)sv.fuel_used * 10 / BHP_FUEL_MAX;
    snprintf(b, sizeof b, "FUEL BURNED %lu.%lu TANKS  " GLYPH_DOT "  CRASHES %lu  " GLYPH_DOT "  WINS %d",
             tanks10 / 10, tanks10 % 10, (unsigned long)sv.crashes, sv.wins);
    tiny_center(b, 160, 155, C_LIGHT);
    text_center(GLYPH_A " CHOOSE   " GLYPH_B " LIBRARY", 160, 166, C_GREY);
}

static void draw_code(void) {
    bhp_draw_world_backdrop(4, frame_t);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    ui_fancy_center("CODE", 160, 14, 2, GRAD, 4, C_INK, C_NAVY);
    tiny_center("A WORD FROM THE KITCHEN CHANGES THE FLIGHT.", 160, 40, C_LIGHT);
    tiny_center("A RUN BEGUN WITH ONE ON EARNS NO GOALS OR RECORDS.", 160, 48, C_GREY);
    int x0 = 160 - (9 * 16) / 2;
    for (int k = 0; k < 9; k++) {
        int x = x0 + k * 16;
        if (k == 4) { text_draw_scaled("-", x + 3, 76, C_GREY, 2); continue; }
        int ci = k < 4 ? k : k - 1;
        char ch[2] = {code[ci], 0};
        bool sel = ci == code_pos;
        gfx_rect(x, 70, 13, 20, sel ? C_DUSK : C_NIGHT);
        text_draw_scaled(ch, x + 2, 74, sel ? C_YELLOW : C_WHITE, 2);
        if (sel) {
            text_draw(GLYPH_UP, x + 3, 61, C_GREY);
            text_draw(GLYPH_DOWN, x + 3, 92, C_GREY);
        }
    }
    if (code_msg && code_msg_t > 0) text_center(code_msg, 160, 112, C_LIME);
    text_center(GLYPH_DPAD " LETTERS   " GLYPH_A " ENTER   " GLYPH_B " BACK", 160, 162, C_GREY);
}

static void draw_story(void) {
    gfx_cls(C_NIGHT);
    for (int i = 0; i < 50; i++) gfx_pset((i * 97 + 13) % 320, (i * 53 + 7) % 90, i % 5 ? C_DUSK : C_GREY);
    bhp_draw_lady(24, 20, 3);
    bhp_draw_ansel(248, 20, 3);
    static const char *const L[] = {
        "ONE NIGHT LADY HUSH TOOK EVERY BELL IN THE VALLEY",
        "AND CARRIED THEM OFF TO HER CITADEL. NOT A CHIME,",
        "NOT A DING, NOT EVEN THE KETTLE WILL WHISTLE.",
        "",
        "ANSEL WINDS UP THE TINKLER AND GOES AFTER THEM:",
        "FIVE LANDS, FIFTY SCREENS, ONE TANK OF FUEL AT A TIME.",
        "AND HE NEVER FLIES PAST A GOOD CUP OF TEA.",
    };
    for (int k = 0; k < ARRAY_LEN(L); k++) tiny_center(L[k], 160, 80 + k * 9, k < 3 ? C_LIGHT : C_CREAM);
    if (state_t > 30) text_center(GLYPH_A " FLY", 160, 164, (frame_t / 16) & 1 ? C_YELLOW : C_WHITE);
}

static void draw_world(void) {
    int w = iclamp(last_world, 0, BHP_WORLDS - 1);
    bhp_draw_world_backdrop(w, frame_t);
    gfx_darken_rect(0, 50, SCREEN_W, 70, 2);
    char b[32];
    snprintf(b, sizeof b, "WORLD %c", 'A' + w);
    text_center(b, 160, 60, C_LIGHT);
    ui_fancy_center(BHP_WORLD_NAME[w], 160, 72, 2, GOLD, 4, C_INK, C_WINE);
    snprintf(b, sizeof b, "SHIPS %02d   SCORE %05d", run.ships, run.score);
    tiny_center(b, 160, 104, C_CREAM);
}

static void draw_tea(void) {
    int w = iclamp(run.stage / BHP_PER_WORLD, 0, BHP_WORLDS - 1);
    gfx_cls(C_NIGHT);
    ui_fancy_center("TEA BREAK", 160, 12, 2, GOLD, 4, C_INK, C_WINE);
    char b[48];
    snprintf(b, sizeof b, "%s IS CLEAR", BHP_WORLD_NAME[w]);
    text_center(b, 160, 40, C_LIGHT);
    for (int k = 0; k < 8; k++) {
        int x = 160 - 4 * 32 + k * 32 + 4, y = 58;
        int stage = w * BHP_PER_WORLD + (k < 4 ? k : k + 1);
        bool got = run.cup[w * 8 + k];
        gfx_rect(x - 3, y - 3, 26, 32, C_INK);
        bhp_draw_cup_big(x, y, got);
        tiny_center(bhp_stage_label(stage), x + 10, y + 20, got ? C_CREAM : C_SLATE);
    }
    snprintf(b, sizeof b, "TEA THIS WORLD %d/8   ALL TOLD %d/40", world_tea(w), tea_count());
    text_center(b, 160, 100, C_YELLOW);
    snprintf(b, sizeof b, "SHIPS %02d   SCORE %05d", run.ships, run.score);
    tiny_center(b, 160, 116, C_CREAM);
    if (state_t > 60) text_center(GLYPH_A " ON", 160, 164, C_GREY);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    if (run.brew) {
        ui_fancy_center("ROOMS DONE", 160, 50, 2, GRAD, 4, C_INK, C_NAVY);
    } else {
        ui_fancy_center("GROUNDED", 160, 50, 3, GRAD, 4, C_INK, C_NAVY);
    }
    char b[64];
    snprintf(b, sizeof b, "SCORE %05d", run.score);
    text_center(b, 160, 92, C_WHITE);
    if (!run.brew) {
        snprintf(b, sizeof b, "REACHED %s   TEA %d/40", bhp_stage_label(run.stage), tea_count());
        text_center(b, 160, 106, C_LIGHT);
    }
    if (state_t > 90) text_center(GLYPH_A " TITLE", 160, 164, C_GREY);
}

static void draw_ending(void) {
    bool all = tea_count() == BHP_CUPS;
    gfx_cls(C_NAVY);
    for (int i = 0; i < 60; i++) gfx_pset((i * 89 + 7) % 320, (i * 61 + 3) % 100, i % 4 ? C_BLUE : C_WHITE);
    gfx_rect(0, 104, SCREEN_W, 76, C_FOREST);
    gfx_dither(0, 104, SCREEN_W, 8, C_JADE, 8);
    /* the bells fly home behind the Tinkler */
    int sx = (state_t * 2) % 400 - 40;
    bhp_draw_ship(sx, 60, 1, true, frame_t);
    for (int k = 1; k <= 5; k++) {
        int bx = sx - k * 18, by = 62 + bhp_sin(frame_t * 4 + k * 40) * 3 / 127;
        gfx_circ(bx, by, 4, C_AMBER);
        gfx_rect(bx - 5, by + 3, 11, 2, C_YELLOW);
        gfx_pset(bx, by + 6, C_BROWN);
    }
    bhp_draw_ansel(16, 112, 3);
    static const char *const A[3] = {"THE BELLS COME HOME, AND THE VALLEY", "RINGS FROM ONE END TO THE OTHER.", "LADY HUSH BUYS EARMUFFS."};
    static const char *const B[3] = {"THE BELLS COME HOME. ANSEL HAS HAD A", "CUP OF TEA IN EVERY LAND, AND THE", "KETTLE WHISTLES LOUDEST OF ALL."};
    for (int k = 0; k < 3; k++) text_draw(all ? B[k] : A[k], 72, 116 + k * 12, C_WHITE);
    char b[48];
    snprintf(b, sizeof b, "TEA %d/40   SCORE %05d", tea_count(), run.score);
    tiny_draw(b, 72, 154, C_CREAM);
    if (state_t > 90) text_center(GLYPH_A " ON", 160, 168, C_LIME);
}

static const char *const CREDITS[] = {
    "BELLHOP", "", "PRESENTED BY", "BEAMDOWN SOFTWORKS", "1985", "",
    "PILOT", "ANSEL, IN THE TINKLER", "",
    "THE WORLDS", "MILLBROOK", "THE ORCHARD", "THE CLOCKWORKS", "THE SUGARWORKS", "HUSH CITADEL", "",
    "THE BIG ONES", "THE MILLWHEEL", "THE CIDER PRESS", "THE TWIN COGS", "THE GUMBALL MACHINE", "LADY HUSH", "",
    "WHISPERED IN THE KITCHEN", "BREW-ROOM", "",
    "THANKS FOR FLYING", "",
};

static void draw_credits(void) {
    gfx_cls(C_INK);
    int y0 = 190 - credits_t / 2;
    int n = ARRAY_LEN(CREDITS);
    for (int k = 0; k < n; k++) {
        int y = y0 + k * 12;
        if (y < -10 || y > 190) continue;
        bool head = k == 0 || (!CREDITS[k - 1][0] && CREDITS[k][0]);
        text_center(CREDITS[k], 160, y, head ? C_YELLOW : C_LIGHT);
    }
    if (y0 + n * 12 < 60) {
        ui_fancy_center("THE END", 160, 72, 3, GOLD, 4, C_INK, C_WINE);
        if (credits_t > 60 * 22) text_center(GLYPH_A " TITLE", 160, 168, C_GREY);
    }
}

static void draw(void) {
    gfx_camera(0, 0);
    gfx_noclip();
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_CODE: draw_code(); break;
    case S_STORY: draw_story(); break;
    case S_WORLD: draw_world(); break;
    case S_PLAY: draw_play(); break;
    case S_TEA: draw_tea(); break;
    case S_OVER: draw_over(); break;
    case S_ENDING: draw_ending(); break;
    case S_CREDITS: draw_credits(); break;
    }
}

/* ------------------------------------------------------------------ */
/* the demo pilot's buttons (tests and screenshots)                     */

static int press(int b) { return (frame_t & 1) ? b : 0; }

static int bhp_bot_buttons(void) {
    switch (state) {
    case S_TITLE:
        if (state_t < 20) return 0;
        if (title_sel != 0) return press(BTN_DOWN);
        return press(BTN_A);
    case S_PLAY: {
        unsigned c = bhp_bot(&st, &bot);
        int m = 0;
        if (c & CHF_LEFT) m |= BTN_LEFT;
        if (c & CHF_RIGHT) m |= BTN_RIGHT;
        if (c & CHF_THRUST) m |= BTN_A;
        if (c & BHP_CTL_SLASH) m |= BTN_B;
        return m;
    }
    case S_CREDITS: return credits_t > 60 * 22 + 10 ? press(BTN_A) : BTN_A;
    case S_OVER: return 0; /* the tests read a lost run before they go on */
    default: return state_t > 95 ? press(BTN_A) : 0;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void bhp_load(void) {
    bhp_art_load();
    bhp_audio_load();
}

static void bhp_start(void) {
    load_save();
    memset(&run, 0, sizeof run);
    brew_on = false;
    title_sel = 0;
    bhp_bot_reset(&bot, bot_warps);
    title_enter();
}

static void bhp_quit(void) { save_now(); }

static void bhp_label(int x, int y, int w, int h, int t) {
    /* the Tinkler hovering over a millpond, a cup of tea on the bank */
    gfx_rect(x, y, w, h, C_SKY);
    gfx_dither(x, y + 20, w, 8, C_CYAN, 8);
    gfx_rect(x, y + 28, w, h - 28, C_CYAN);
    gfx_rect(x, y + 44, w, h - 44, C_BLUE);
    gfx_hline(x, x + w - 1, y + 44, C_SKY);
    gfx_rect(x, y + 38, w / 3, 8, C_GREY);
    gfx_hline(x, x + w / 3 - 1, y + 38, C_LEAF);
    bhp_draw_cup(x + 6, y + 30, true);
    int sx = x + w / 2 + bhp_sin(t * 2) * 10 / 127, sy = y + 30 + bhp_sin(t * 5) * 2 / 127;
    bhp_draw_ship(sx, sy, bhp_cos(t * 2) >= 0 ? 1 : -1, (t / 4) & 1, t);
    ui_fancy_text("BELLHOP", x + 4, y + 3, 1, GOLD, 4, C_INK, -1);
    (void)h;
}

/* key is name followed by a number */
static int num_key(const char *key, const char *name, int *n) {
    size_t len = strlen(name);
    if (strncmp(key, name, len) || key[len] < '0' || key[len] > '9') return 0;
    *n = atoi(key + len);
    return 1;
}

/* every stage's layout is sound: rows the right size, one start and one exit,
 * a hidden tea spot and a cup on each regular stage (none elsewhere), a warp
 * only where one belongs, and a route for the demo pilot */
static int stages_bad(void) {
    int bad = 0;
    for (int i = 0; i < BHP_STAGES; i++) {
        const BhpStageDef *d = &BHP_STAGE[i];
        int ns = 0, ne = 0, nt = 0, nc = 0, nw = 0, rows_ok = 1;
        for (int r = 0; r < BHP_TH; r++) {
            if (!d->rows[r] || (int)strlen(d->rows[r]) != BHP_TW) { rows_ok = 0; continue; }
            for (int c = 0; c < BHP_TW; c++) {
                char ch = d->rows[r][c];
                ns += ch == 'S';
                ne += ch == 'E';
                nt += ch == 't';
                nc += ch == 'c';
                nw += ch == 'w';
            }
        }
        bool reg = bhp_kind(i) == BHK_STAGE;
        bool ok = rows_ok && ns == 1 && ne == 1 && nt == (reg ? 1 : 0) && nc == (reg ? 1 : 0) &&
                  nw == (d->warp_to >= 0 ? 1 : 0) && d->name && bhp_bot_route_len(i) > 0;
        if (!ok) {
            bad++;
            fprintf(stderr, "stage %s: rows %d S %d E %d t %d c %d w %d\n", bhp_stage_label(i), rows_ok, ns, ne, nt, nc, nw);
        }
    }
    return bad;
}

static int count_kind(int kind) {
    int n = 0;
    for (int i = 0; i < st.ne; i++) n += st.e[i].on && st.e[i].kind == kind;
    return n;
}

static int bhp_query(const char *key, int *out) {
    int i;
    if (!strcmp(key, "bot")) { *out = bhp_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "stage")) { *out = run.stage; return 1; }
    if (!strcmp(key, "mode")) { *out = st.mode; return 1; }
    if (!strcmp(key, "score")) { *out = run.score; return 1; }
    if (!strcmp(key, "ships")) { *out = run.ships; return 1; }
    if (!strcmp(key, "fuel")) { *out = st.fuel; return 1; }
    if (!strcmp(key, "cup")) { *out = st.cup; return 1; }
    if (!strcmp(key, "tea")) { *out = tea_count(); return 1; }
    if (num_key(key, "tea", &i)) { *out = i < BHP_CUPS ? run.cup[i] : -1; return 1; }
    if (!strcmp(key, "x")) { *out = st.f.x >> 8; return 1; }
    if (!strcmp(key, "y")) { *out = st.f.y >> 8; return 1; }
    if (!strcmp(key, "vx")) { *out = st.f.vx; return 1; }
    if (!strcmp(key, "vy")) { *out = st.f.vy; return 1; }
    if (!strcmp(key, "face")) { *out = st.f.face; return 1; }
    if (!strcmp(key, "slash")) { *out = st.slash_t; return 1; }
    if (!strcmp(key, "t")) { *out = (int)st.t; return 1; }
    if (!strcmp(key, "exit_open")) { *out = st.exit_open; return 1; }
    if (!strcmp(key, "round")) { *out = st.round; return 1; }
    if (!strcmp(key, "crystals")) { *out = count_kind(BEK_CRYSTAL); return 1; }
    if (!strcmp(key, "bonus_done")) { *out = st.bonus_done; return 1; }
    if (!strcmp(key, "boss")) { *out = st.boss.kind; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = st.boss.hp; return 1; }
    if (!strcmp(key, "boss_down")) { *out = st.boss.down; return 1; }
    if (!strcmp(key, "boss_phase")) { *out = st.boss.phase; return 1; }
    if (!strcmp(key, "boss_round")) { *out = st.boss.round; return 1; }
    if (!strcmp(key, "boss_spin")) { *out = st.boss.spin; return 1; }
    if (!strcmp(key, "boss_active")) { *out = st.boss.active; return 1; }
    if (!strcmp(key, "owl")) { *out = st.idx == 0 && owl_seen; return 1; }
    if (!strcmp(key, "warp_shown")) { *out = st.warp_to >= 0 && st.t < BHP_WARP_SHOW; return 1; }
    if (!strcmp(key, "hiscore")) { *out = (int)sv.hiscore; return 1; }
    if (!strcmp(key, "fuel_used")) { *out = (int)sv.fuel_used; return 1; }
    if (!strcmp(key, "last_t")) { *out = run.last_t; return 1; }
    if (!strcmp(key, "deaths")) { *out = run.deaths; return 1; }
    if (!strcmp(key, "crashes")) { *out = (int)sv.crashes; return 1; }
    if (!strcmp(key, "most_ships")) { *out = sv.most_ships; return 1; }
    if (!strcmp(key, "wins")) { *out = sv.wins; return 1; }
    if (!strcmp(key, "runs")) { *out = sv.runs; return 1; }
    if (!strcmp(key, "best_tea")) { *out = sv.best_tea; return 1; }
    if (!strcmp(key, "brew")) { *out = brew_on; return 1; }
    if (!strcmp(key, "brew_run")) { *out = run.brew; return 1; }
    if (!strcmp(key, "brew_room")) { *out = run.brew_room; return 1; }
    if (!strcmp(key, "code_pos")) { *out = code_pos; return 1; }
    if (!strcmp(key, "title_sel")) { *out = title_sel; return 1; }
    if (!strcmp(key, "art_ok")) { *out = bhp_art_ok(); return 1; }
    if (!strcmp(key, "stages_bad")) { *out = stages_bad(); return 1; }
    if (!strcmp(key, "credits_t")) { *out = credits_t; return 1; }
    if (!strcmp(key, "coins")) { *out = count_kind(BEK_COIN); return 1; }
    if (!strcmp(key, "coins_got")) { *out = st.coins_got; return 1; }
    if (!strcmp(key, "bot_step")) { *out = bot.step; return 1; }
    if (!strcmp(key, "fuel_sources")) {
        /* what on this stage puts fuel back: enemies that give it, bubbles, blocks, cans */
        int n = 0;
        for (int i = 0; i < st.ne; i++) {
            int k = st.e[i].kind;
            n += st.e[i].on && ((k >= BEK_MOTH && k <= BEK_BUBBLE && k != BEK_TURRET) || k == BEK_FUEL);
        }
        for (int r = 0; r < BHP_TH; r++)
            for (int c = 0; c < BHP_TW; c++) n += st.tile[r][c] == BTL_BLOCK;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "boss_x")) { *out = (int)(st.boss.x >> 8); return 1; }
    if (!strcmp(key, "boss_y")) { *out = (int)(st.boss.y >> 8); return 1; }
    if (!strcmp(key, "cspeed") || !strcmp(key, "gum_size")) {
        /* the first crystal's speed on its faster axis; the first gumball's size */
        *out = -1;
        for (int i = 0; i < st.ne; i++) {
            const BhpEnt *e = &st.e[i];
            if (!e->on) continue;
            if (key[0] == 'c' && e->kind == BEK_CRYSTAL) { *out = imax(iabs(e->vx), iabs(e->vy)); break; }
            if (key[0] == 'g' && e->kind == BEK_GUM) { *out = e->size; break; }
        }
        return 1;
    }
    if (!strcmp(key, "ver")) { *out = st.ver; return 1; }
    if (!strcmp(key, "shots")) { int n = 0; for (int k = 0; k < BHP_SHOTS; k++) n += st.shot[k].on; *out = n; return 1; }
    if (num_key(key, "kind", &i)) { *out = count_kind(i); return 1; }
    if (num_key(key, "tile_x", &i)) { *out = st.tile[i % BHP_TH][(i / BHP_TH) % BHP_TW]; return 1; }
    if (num_key(key, "ent_x", &i)) { *out = i < st.ne && st.e[i].on ? (int)(st.e[i].x >> 8) : -1; return 1; }
    if (num_key(key, "ent_y", &i)) { *out = i < st.ne && st.e[i].on ? (int)(st.e[i].y >> 8) : -1; return 1; }
    if (num_key(key, "ent_on", &i)) { *out = i < st.ne ? st.e[i].on : 0; return 1; }
    if (num_key(key, "ent_flag", &i)) { *out = i < st.ne ? st.e[i].flag : 0; return 1; }
    if (num_key(key, "ent_hp", &i)) { *out = i < st.ne ? st.e[i].hp : 0; return 1; }
    if (num_key(key, "ent_kind", &i)) { *out = i < st.ne ? st.e[i].kind : 0; return 1; }
    return 0;
}

static int bhp_cheat(const char *cmd) {
    int a, b, c;
    if (sscanf(cmd, "stage %d", &a) == 1) {
        /* a fresh run, straight onto stage a (0-49) */
        new_run(false);
        play_stage(iclamp(a, 0, BHP_STAGES - 1));
        return 1;
    }
    if (sscanf(cmd, "goto %d", &a) == 1) { play_stage(iclamp(a, 0, BHP_STAGES - 1)); return 1; }
    if (sscanf(cmd, "ships %d", &a) == 1) { run.ships = a; note_ships(); return 1; }
    if (sscanf(cmd, "score %d", &a) == 1) { run.score = a; run.next_extend = (a / BHP_EXTEND + 1) * BHP_EXTEND; return 1; }
    if (sscanf(cmd, "points %d", &a) == 1) { add_points(a); return 1; }
    if (sscanf(cmd, "fuel %d", &a) == 1) { st.fuel = (int16_t)a; return 1; }
    if (sscanf(cmd, "place %d %d", &a, &b) == 2) {
        st.f.x = a * CHF_ONE;
        st.f.y = b * CHF_ONE;
        st.f.vx = st.f.vy = 0;
        st.mode = BSM_FLY;
        st.mode_t = 0;
        return 1;
    }
    if (sscanf(cmd, "vel %d %d", &a, &b) == 2) { st.f.vx = a; st.f.vy = b; return 1; }
    if (sscanf(cmd, "face %d", &a) == 1) { st.f.face = (int8_t)(a < 0 ? -1 : 1); return 1; }
    if (sscanf(cmd, "tea %d %d", &a, &b) == 2) { if (a >= 0 && a < BHP_CUPS) run.cup[a] = (uint8_t)(b != 0); return 1; }
    if (!strcmp(cmd, "all_tea")) { memset(run.cup, 1, sizeof run.cup); return 1; }
    if (!strcmp(cmd, "boss_down")) { bhp_boss_finish(&st); return 1; }
    if (!strcmp(cmd, "crash")) { bhp_kill_ship(&st, 9); return 1; }
    if (!strcmp(cmd, "brew")) { brew_on = true; return 1; }
    if (sscanf(cmd, "bot_warps %d", &a) == 1) { bot_warps = a != 0; bot.want_warp = (uint8_t)bot_warps; return 1; }
    if (sscanf(cmd, "bot_debug %d", &a) == 1) { bhp_bot_debug = a; return 1; }
    if (!strcmp(cmd, "dump")) { bhp_bot_dump(&bot); return 1; }
    if (!strcmp(cmd, "clear_shots")) { memset(st.shot, 0, sizeof st.shot); return 1; }
    if (sscanf(cmd, "bot_rounds %d", &a) == 1) { bhp_bot_rounds = a; return 1; }
    if (sscanf(cmd, "spike %d %d %d", &a, &b, &c) == 3) {
        /* a spike ball at rest: c = 0 red, 1 blue */
        int i = bhp_ent_add(&st, BEK_SPIKE, a, b);
        if (i >= 0) st.e[i].flag = (uint8_t)(c != 0);
        return 1;
    }
    if (sscanf(cmd, "beside_boss %d", &a) == 1) {
        /* the ship level with Lady Hush, a units to her side, facing her */
        st.f.x = st.boss.x + a * CHF_ONE;
        st.f.y = st.boss.y;
        st.f.vx = st.f.vy = 0;
        st.f.face = (int8_t)(a < 0 ? 1 : -1);
        st.mode = BSM_FLY;
        return 1;
    }
    if (sscanf(cmd, "ent %d %d %d", &a, &b, &c) == 3) {
        int i = bhp_ent_add(&st, a, b, c);
        if (i >= 0) bhp_ent_settle(&st, &st.e[i]);
        return 1;
    }
    if (sscanf(cmd, "tile %d %d %d", &a, &b, &c) == 3) { bhp_set_tile(&st, a, b, c); return 1; }
    if (!strcmp(cmd, "no_enemies")) {
        for (int i = 0; i < st.ne; i++)
            if (st.e[i].kind >= BEK_MOTH && st.e[i].kind <= BEK_DRONE) st.e[i].on = 0;
        return 1;
    }
    if (!strcmp(cmd, "title")) { title_enter(); return 1; }
    return 0;
}

const GameDef GAME_BELLHOP = {
    "bellhop",
    "BELLHOP",
    "1985",
    "ARCADE",
    "THRUST, HOVER, SLASH, TOUCH NOTHING, AND NEVER PASS UP A CUP OF TEA.",
    {"KEEP 15 SHIPS IN THE HANGAR", "BRING THE BELLS HOME", "BRING THEM HOME AFTER ALL 40 CUPS OF TEA"},
    GLYPH_LEFT GLYPH_RIGHT "\tSTEER\n"
    GLYPH_A " (HOLD)\tTHRUST (TAP TO HOVER)\n"
    GLYPH_B "\tSLASH\n"
    "START\tPAUSE",
    C_SKY, C_AMBER,
    bhp_load, bhp_start, update, draw, bhp_quit, bhp_label, bhp_query, bhp_cheat,
    "CAMPANELLA", 17,
    NULL,
};
