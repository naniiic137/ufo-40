/* FORLORN HOPE - ninety-nine volunteers of Holloway spend themselves, one
 * trade at a time, on one fixed map, to burst the four thorn hearts of
 * Thornkeep. Cartridge 32 of UFO 40, a tribute to Mortol II (UFO 50 #32).
 * See docs/games/32-forlorn-hope.md. The rules live in forlorn_world.c and
 * forlorn_foes.c; this file is the screens, the run, the records, the
 * goals, the code screen, two players and the test hooks.
 *
 * Like the original, a run is played in one sitting and is not saved: the
 * cartridge keeps only its records. */
#include "forlorn.h"

enum { S_TITLE, S_CODE, S_STORY, S_PLAY, S_OVER, S_ENDING, S_CREDITS };

typedef struct Save {
    uint32_t magic;
    uint16_t runs, wins;
    uint16_t best_lost;      /* fewest volunteers lost in a win (0xFFFF: none yet) */
    uint16_t most_doors, most_switches;
    uint16_t pad0;
    uint32_t total_lost;
    uint8_t plate_ever;      /* a plate has been weighed down (the Beacon) */
    uint8_t pad[3];
} Save;
#define SAVE_MAGIC 0x46524C01u

FrlWorld frl_w;
static Save sv;
static FrlBot bot;
static int state, state_t, frame_t, title_sel, credits_t;
static char code[9] = "AAAAAAAA";
static int code_pos;
static const char *code_msg;
static int code_msg_t;
static bool code_on;          /* SLIM-PICK: three trades of the five, at random */
static bool run_code;         /* this run was begun with the code (no goals, no records) */
static bool counted;
static int meta_t;            /* the last volunteer, and a lit fuse */
static int last_phase;
static int last_units;

typedef struct Part { int16_t x, y, vx, vy; uint8_t life, col, on; } Part; /* 1/16 px, world */
#define NPARTS 128
static Part parts[NPARTS];

#define CODE_SLIM "SLIMPICK"

static bool vita_single(void) { return plat_kind() == PLAT_VITA; }

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
        sv.best_lost = 0xFFFF;
    }
    /* goals come back from the records */
    if (sv.plate_ever) game_award(GOAL_BEACON);
    if (sv.wins) game_award(GOAL_SAUCER);
    if (sv.wins && sv.best_lost < FRL_CHERRY) game_award(GOAL_ALIEN);
}

/* ------------------------------------------------------------------ */
/* the run                                                              */

static void goto_state(int s) {
    state = s;
    state_t = 0;
    input_consume();
}

static void title_enter(void) {
    goto_state(S_TITLE);
    game_set_pausable(false);
    input_set_versus(false);
    music_play(FRL_MUS_TITLE);
}

static uint8_t slim_mask(void) {
    /* three trades of the five, picked at random for the run */
    uint8_t m = 0x1F;
    int drop[2];
    drop[0] = rng_range(&g_rng, 0, 4);
    do drop[1] = rng_range(&g_rng, 0, 4); while (drop[1] == drop[0]);
    m &= (uint8_t)~(1 << drop[0]);
    m &= (uint8_t)~(1 << drop[1]);
    return m;
}

static void new_run(int players) {
    run_code = code_on;
    frl_world_new(&frl_w, players, run_code ? slim_mask() : 0x1F);
    memset(parts, 0, sizeof parts);
    counted = false;
    meta_t = 0;
    last_phase = frl_w.phase;
    last_units = 0;
    frl_bot_reset(&bot);
    input_set_versus(players == 2);
    if (!run_code) {
        if (sv.runs < 65535) sv.runs++;
        save_now();
    }
}

static void play_enter(void) {
    goto_state(S_PLAY);
    game_set_pausable(true);
    music_play(FRL_MUS_SELECT);
}

static void note_records(void) {
    if (run_code) return;
    if (frl_w.doors > sv.most_doors) sv.most_doors = (uint16_t)frl_w.doors;
    if (frl_w.switches > sv.most_switches) sv.most_switches = (uint16_t)frl_w.switches;
}

static void run_won(void) {
    if (counted) return;
    counted = true;
    if (run_code) return;
    game_award(GOAL_SAUCER);
    if (frl_w.lost < FRL_CHERRY) game_award(GOAL_ALIEN);
    if (sv.wins < 65535) sv.wins++;
    if ((unsigned)frl_w.lost < sv.best_lost) sv.best_lost = (uint16_t)frl_w.lost;
    note_records();
    save_now();
}

static void run_lost(void) {
    if (counted) return;
    counted = true;
    note_records();
    if (!run_code) save_now();
}

/* ------------------------------------------------------------------ */
/* sparks                                                               */

static void spark(int x, int y, int n, int col, int speed) {
    for (int k = 0; k < n; k++)
        for (int i = 0; i < NPARTS; i++)
            if (!parts[i].on) {
                int a = rng_range(&g_rng, 0, 359), v = rng_range(&g_rng, speed / 3, speed);
                float r = (float)a * 0.0174533f;
                parts[i] = (Part){(int16_t)(x * 16), (int16_t)(y * 16), (int16_t)(cosf(r) * (float)v), (int16_t)(sinf(r) * (float)v - 8),
                                  (uint8_t)rng_range(&g_rng, 12, 28), (uint8_t)col, 1};
                break;
            }
}

static void world_news(void) {
    const FrlWorld *w = &frl_w;
    for (int k = 0; k < w->nfx; k++) {
        const FrlFx *f = &w->fx[k];
        switch (f->kind) {
        case FFX_BLAST: spark(f->x, f->y, 30, C_ORANGE, 48); spark(f->x, f->y, 16, C_YELLOW, 32); spark(f->x, f->y, 10, C_GREY, 20); break;
        case FFX_DIE: spark(f->x, f->y, 14, C_RED, 26); spark(f->x, f->y, 6, C_WHITE, 18); break;
        case FFX_HIT: spark(f->x, f->y, 4, C_WHITE, 18); break;
        case FFX_KILL: spark(f->x, f->y, 12, C_LIME, 26); spark(f->x, f->y, 6, C_WHITE, 18); break;
        case FFX_HEART: spark(f->x, f->y, 40, C_RED, 50); spark(f->x, f->y, 20, C_PINK, 34); break;
        case FFX_KEY: spark(f->x, f->y, 8, C_YELLOW, 22); break;
        case FFX_STONE: spark(f->x, f->y, 10, C_LIGHT, 22); break;
        case FFX_WARP: spark(f->x, f->y, 10, C_CYAN, 24); break;
        case FFX_BREAK: spark(f->x, f->y, 6, C_TAN, 26); break;
        default: spark(f->x, f->y, 3, C_GREY, 12); break;
        }
    }
    uint32_t ev = w->ev;
    if (ev & FEV_BLAST) sfx_play_name("frl_blast");
    else if (ev & FEV_HEART) sfx_play_name("frl_heart");
    else if (ev & FEV_BREAK) sfx_play_name("frl_break");
    else if (ev & FEV_STONE) sfx_play_name("frl_stone");
    else if (ev & FEV_DOOR) sfx_play_name("frl_door");
    else if (ev & FEV_SWING) sfx_play_name("frl_swing");
    else if (ev & FEV_SHOOT) sfx_play_name(w->u.cls == FRC_HUNTER ? "frl_musket" : "frl_throw");
    if (ev & FEV_EATEN) sfx_play_name("frl_eaten");
    else if (ev & FEV_DIE) sfx_play_name("frl_die");
    else if (ev & FEV_KEY) sfx_play_name("frl_key");
    else if (ev & FEV_PLATE) sfx_play_name("frl_plate");
    else if (ev & FEV_PLATE_UP) sfx_play_name("frl_plate_up");
    else if (ev & FEV_WAY) sfx_play_name("frl_way");
    else if (ev & FEV_POUCH) sfx_play_name("frl_pouch");
    else if (ev & FEV_PIPE) sfx_play_name("frl_pipe");
    else if (ev & FEV_WARP) sfx_play_name("frl_warp");
    else if (ev & FEV_CHUTE) sfx_play_name("frl_chute");
    else if (ev & FEV_AMMO) sfx_play_name("frl_ammo");
    else if (ev & FEV_KILL) sfx_play_name("frl_kill");
    else if (ev & FEV_HIT) sfx_play_name("frl_hit");
    else if (ev & FEV_TING) sfx_play_name("frl_ting");
    else if (ev & FEV_READY) sfx_play_name("frl_ready");
    else if (ev & FEV_DOUBLE) sfx_play_name("frl_double");
    else if (ev & FEV_JUMP) sfx_play_name("frl_jump");
    else if (ev & FEV_EMPTY) sfx_play_name("frl_empty");
    else if (ev & FEV_FOE_SHOT && (frame_t & 3) == 0) sfx_play_name("frl_foe_shot");
    if (ev & FEV_PLATE && !run_code) {
        if (!sv.plate_ever) { sv.plate_ever = 1; save_now(); }
        game_award(GOAL_BEACON);
    }
}

static void update_parts(void) {
    for (int i = 0; i < NPARTS; i++) {
        Part *p = &parts[i];
        if (!p->on) continue;
        p->x = (int16_t)(p->x + p->vx);
        p->y = (int16_t)(p->y + p->vy);
        p->vy = (int16_t)(p->vy + 2);
        if (--p->life == 0) p->on = 0;
    }
}

static void update_play(void) {
    FrlWorld *w = &frl_w;
    uint32_t raw = input_held();
    uint32_t p1 = raw & 0xFF, p2 = (raw >> BTN_P2_SHIFT) & 0xFF;
    if (w->players == 1) p2 = p1;
    int was = w->phase;
    frl_world_step(w, p1, p2);
    world_news();
    update_parts();
    if (meta_t > 0) meta_t--;
    if (was == FWP_SELECT && w->phase == FWP_PLAY) {
        sfx_play_name("frl_pick");
        music_play(FRL_MUS_CLASS[w->u.cls]);
        if (w->lives == 0 && w->u.cls == FRC_SAPPER) meta_t = 300;
    }
    if (was != FWP_SELECT && w->phase == FWP_SELECT) music_play(FRL_MUS_SELECT);
    if (w->phase == FWP_SELECT && (w->ev & FEV_SPAWN)) sfx_play_name("frl_move");
    if (w->phase == FWP_WON && w->phase_t == 1) {
        sfx_play_name("frl_win");
        w->shake = 120;
    }
    if (w->phase == FWP_WON && w->phase_t >= 150) {
        run_won();
        goto_state(S_ENDING);
        game_set_pausable(false);
        music_play(FRL_MUS_END);
        return;
    }
    if (w->phase == FWP_OVER && w->phase_t >= 2) {
        run_lost();
        goto_state(S_OVER);
        game_set_pausable(false);
        music_play(FRL_MUS_OVER);
        return;
    }
    if (w->phase == FWP_PLAY) note_records();
}

/* ------------------------------------------------------------------ */
/* the code screen                                                      */

static void check_code(void) {
    if (!strcmp(code, CODE_SLIM)) {
        code_on = !code_on;
        code_msg = code_on ? "SLIM-PICK: THREE TRADES OF THE FIVE" : "SLIM-PICK IS OFF";
        sfx_play_name("frl_key");
    } else {
        code_msg = "NO SUCH CODE";
        sfx_play_name("ui_error");
    }
    code_msg_t = 150;
}

static void update_code(void) {
    if (code_msg_t > 0) code_msg_t--;
    if (btn_repeat(BTN_LEFT)) { code_pos = (code_pos + 7) % 8; sfx_play_name("frl_move"); }
    if (btn_repeat(BTN_RIGHT)) { code_pos = (code_pos + 1) % 8; sfx_play_name("frl_move"); }
    if (btn_repeat(BTN_UP)) { code[code_pos] = (char)(code[code_pos] == 'Z' ? 'A' : code[code_pos] + 1); sfx_play_name("frl_move"); }
    if (btn_repeat(BTN_DOWN)) { code[code_pos] = (char)(code[code_pos] == 'A' ? 'Z' : code[code_pos] - 1); sfx_play_name("frl_move"); }
    if (btnp(BTN_B)) { sfx_play_name("ui_back"); goto_state(S_TITLE); return; }
    if (btnp(BTN_A) && state_t > 5) check_code();
}

/* ------------------------------------------------------------------ */
/* updates                                                              */

static void update(void) {
    frame_t++;
    state_t++;
    switch (state) {
    case S_TITLE:
        if (btn_repeat(BTN_UP)) { title_sel = (title_sel + 2) % 3; sfx_play_name("frl_move"); }
        if (btn_repeat(BTN_DOWN)) { title_sel = (title_sel + 1) % 3; sfx_play_name("frl_move"); }
        if (btnp(BTN_B) && state_t > 10) { game_exit_to_library(); return; }
        if (state_t > 15 && btnp(BTN_A)) {
            if (title_sel == 2) { sfx_play_name("ui_ok"); code_pos = 0; code_msg = NULL; goto_state(S_CODE); break; }
            if (title_sel == 1 && vita_single()) { sfx_play_name("ui_error"); break; }
            sfx_play_name("ui_ok");
            new_run(title_sel == 1 ? 2 : 1);
            goto_state(S_STORY);
        }
        break;
    case S_CODE: update_code(); break;
    case S_STORY:
        if (state_t > 30 && btnp(BTN_A)) { sfx_play_name("ui_ok"); play_enter(); }
        break;
    case S_PLAY: update_play(); break;
    case S_OVER:
        if (state_t > 120 && btnp(BTN_A)) { sfx_play_name("ui_ok"); title_enter(); }
        break;
    case S_ENDING:
        if (state_t > 120 && btnp(BTN_A)) { sfx_play_name("ui_ok"); goto_state(S_CREDITS); credits_t = 0; }
        break;
    case S_CREDITS:
        credits_t += btn(BTN_A) ? 4 : 1;
        if (credits_t > 60 * 24 && btnp(BTN_A | BTN_B)) title_enter();
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */

static const uint8_t BLOOD[] = {C_PINK, C_RED, C_WINE, C_MAROON};
static const uint8_t STONEG[] = {C_WHITE, C_LIGHT, C_GREY, C_SLATE};
static const uint8_t GOLD[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};

static void draw_parts(void) {
    gfx_clip(0, FRL_OY, SCREEN_W, FRL_VH);
    gfx_camera(frl_w.cam_x, frl_w.cam_y - FRL_OY);
    for (int i = 0; i < NPARTS; i++) {
        const Part *p = &parts[i];
        if (p->on) gfx_pset(p->x / 16, p->y / 16, p->life < 6 ? C_SLATE : p->col);
    }
    gfx_camera(0, 0);
    gfx_noclip();
}

static void draw_key_icon(int x, int y, int col) {
    gfx_circb(x + 2, y + 2, 2, col);
    gfx_vline(x + 2, y + 4, y + 8, col);
    gfx_hline(x + 3, x + 4, y + 7, col);
}

static void draw_hud(void) {
    const FrlWorld *w = &frl_w;
    gfx_rect(0, 0, SCREEN_W, FRL_OY, C_INK);
    gfx_hline(0, SCREEN_W - 1, FRL_OY - 1, C_NIGHT);
    char b[48];
    int x = 3;
    if (w->phase == FWP_PLAY || w->phase == FWP_DEAD) {
        const FrlUnit *u = &w->u;
        if (w->players == 2) {
            snprintf(b, sizeof b, "P%d", u->player + 1);
            x = text_draw(b, x, 2, u->player ? C_CYAN : C_YELLOW) + 4;
        }
        x = text_draw(FRL_CLASS_NAME[u->cls], x, 2, C_LIGHT) + 6;
        if (FRL_AMMO[u->cls]) {
            for (int k = 0; k < FRL_AMMO[u->cls]; k++) {
                int col = k < u->ammo ? (u->cls == FRC_HUNTER ? C_AMBER : C_LIGHT) : C_NIGHT;
                gfx_rect(x + k * 3, 3, 2, 6, col);
            }
            x += FRL_AMMO[u->cls] * 3 + 4;
        }
        /* keys carried, beside the ammo (top left, as on the original's screen) */
        draw_key_icon(x, 2, C_YELLOW);
        snprintf(b, sizeof b, "%d", w->keys);
        x = text_draw(b, x + 8, 2, C_CREAM) + 6;
        if (u->charge >= FRL_CHARGE) text_draw("READY", x, 2, (frame_t / 4) & 1 ? C_YELLOW : C_ORANGE);
        else if (u->charge >= FRL_ARMED) {
            int fill = (u->charge - FRL_ARMED) * 24 / (FRL_CHARGE - FRL_ARMED);
            gfx_rectb(x, 3, 26, 6, C_SLATE);
            gfx_rect(x + 1, 4, fill, 4, C_ORANGE);
        }
    } else if (w->phase == FWP_SELECT) {
        if (w->players == 2) {
            snprintf(b, sizeof b, "PLAYER %d, PICK A TRADE", w->units % 2 + 1);
            text_draw(b, 3, 2, w->units % 2 ? C_CYAN : C_YELLOW);
        } else {
            text_draw("PICK A TRADE", 3, 2, C_LIGHT);
        }
    }
    if (run_code) tiny_draw("SLIM", SCREEN_W - 58, 4, C_LIME);
}

static void draw_select(void) {
    const FrlWorld *w = &frl_w;
    int y0 = 118;
    gfx_rect(0, y0, SCREEN_W, SCREEN_H - y0, C_INK);
    gfx_hline(0, SCREEN_W - 1, y0, C_SLATE);
    for (int c = 0; c < FRC_COUNT; c++)
        frl_draw_class_card(c, 18 + c * 58, y0 + 5, w->sel == c, (w->allowed >> c) & 1, frame_t);
    if (w->lives == 0) {
        gfx_rect(80, y0 - 12, 160, 10, C_INK);
        tiny_center("THE LAST VOLUNTEER OF HOLLOWAY", 160, y0 - 10, C_RED);
    }
}

static void draw_play(void) {
    frl_draw_world(&frl_w, frame_t);
    draw_parts();
    draw_hud();
    if (frl_w.phase == FWP_SELECT) draw_select();
    if (meta_t > 0) {
        gfx_rect(20, 40, 280, 22, C_INK);
        gfx_rectb(20, 40, 280, 22, C_RED);
        text_center("ALL HOLLOWAY HAS LEFT IS ME.", 160, 43, C_WHITE);
        text_center("ME, AND A LIT FUSE.", 160, 52, C_ORANGE);
    }
    if (frl_w.phase == FWP_DEAD && frl_w.phase_t > 10 && frl_w.lives == 0 && frl_w.hearts_left > 0) {
        gfx_rect(100, 70, 120, 14, C_INK);
        text_center("NO ONE IS LEFT", 160, 73, C_RED);
    }
    if (frl_w.phase == FWP_WON) {
        gfx_rect(70, 60, 180, 16, C_INK);
        text_center("THE LAST HEART BURSTS", 160, 64, (frame_t / 6) & 1 ? C_RED : C_PINK);
    }
}

static void draw_title(void) {
    static const uint8_t SKY[6] = {C_INK, C_NIGHT, C_NAVY, C_DUSK, C_PURPLE, C_WINE};
    for (int k = 0; k < 6; k++) {
        gfx_rect(0, k * 17, SCREEN_W, 17, SKY[k]);
        if (k < 5) gfx_dither(0, k * 17 + 12, SCREEN_W, 5, SKY[k + 1], 8);
    }
    for (int i = 0; i < 50; i++) gfx_pset((i * 97 + 13) % 320, (i * 53 + 7) % 50, i % 6 ? C_GREY : C_WHITE);
    gfx_circ(270, 26, 9, C_CREAM);
    gfx_circ(274, 24, 8, C_NIGHT);
    for (int x = 0; x < SCREEN_W; x += 2) {
        int hh = 84 + (int)(6.0f * sinf((float)x * 0.03f) + 4.0f * sinf((float)x * 0.071f + 1.0f));
        gfx_rect(x, hh, 2, 100 - hh, C_NIGHT);
    }
    gfx_rect(0, 100, SCREEN_W, 80, C_INK);
    frl_draw_keep(200, 40, frame_t, false);
    /* the troop walking out of the door, one after another */
    for (int k = 0; k < 5; k++) {
        int x = (frame_t / 2 + k * 30) % 200 - 20;
        frl_draw_unit_at(k, x, 90, 1, 1 + (frame_t / 6 + k) % 2, false, 0);
    }
    gfx_rect(0, 100, SCREEN_W, 1, C_FOREST);
    ui_fancy_center("FORLORN HOPE", 120, 10, 3, BLOOD, 4, C_INK, C_NIGHT);
    tiny_center("99 VOLUNTEERS " GLYPH_DOT " 5 TRADES " GLYPH_DOT " 4 THORN HEARTS", 120, 42, C_CREAM);
    static const char *const ITEMS[3] = {"1 PLAYER", "2 PLAYERS", "CODE"};
    ui_panel(84, 106, 72, 44, C_INK, C_WINE);
    for (int k = 0; k < 3; k++) {
        int y = 111 + k * 12;
        int col = title_sel == k ? C_YELLOW : C_LIGHT;
        if (k == 1 && vita_single()) col = C_SLATE;
        text_center(ITEMS[k], 120, y, col);
        if (title_sel == k) ui_cursor(90, y, frame_t);
    }
    if (code_on) tiny_center("SLIM-PICK IS ON", 120, 153, C_LIME);
    char b[96];
    if (sv.best_lost == 0xFFFF) snprintf(b, sizeof b, "RUNS %d  " GLYPH_DOT "  WINS %d  " GLYPH_DOT "  FEWEST LOST -", sv.runs, sv.wins);
    else snprintf(b, sizeof b, "RUNS %d  " GLYPH_DOT "  WINS %d  " GLYPH_DOT "  FEWEST LOST %d", sv.runs, sv.wins, sv.best_lost);
    tiny_center(b, 160, 160, C_YELLOW);
    snprintf(b, sizeof b, "MOST DOORS UNLOCKED %d  " GLYPH_DOT "  MOST PLATES PRESSED %d", sv.most_doors, sv.most_switches);
    tiny_center(b, 160, 168, C_LIGHT);
}

static void draw_code(void) {
    gfx_cls(C_INK);
    ui_fancy_center("CODE", 160, 14, 2, STONEG, 4, C_INK, C_NIGHT);
    tiny_center("A WORD FROM THE OLD STEWARDS CHANGES THE MUSTER.", 160, 40, C_LIGHT);
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
    gfx_cls(C_INK);
    frl_draw_keep(120, 6, frame_t, false);
    gfx_rect(0, 66, SCREEN_W, 1, C_FOREST);
    static const char *const L[] = {
        "LONG AGO THE FOLK OF HOLLOWAY GAVE THEIR LIVES",
        "TO PUT THE THORNS OF THORNKEEP TO SLEEP.",
        "NOW THE STEWARDS HAVE WOKEN THEM, TO RULE BY FEAR.",
        "",
        "THE FOLK REMEMBER. NINETY-NINE STEP FORWARD,",
        "ONE AT A TIME, EACH WITH A TRADE AND A LAST GIFT.",
        "FOUR THORN HEARTS BEAT IN THE KEEP. BURST THEM ALL.",
    };
    for (int k = 0; k < ARRAY_LEN(L); k++) tiny_center(L[k], 160, 78 + k * 10, k < 3 ? C_LIGHT : C_CREAM);
    if (frl_w.players == 2) tiny_center("TWO PLAYERS: THE VOLUNTEERS TAKE TURNS, ONE POOL OF LIVES", 160, 152, C_CYAN);
    if (state_t > 30) text_center(GLYPH_A " MUSTER", 160, 164, (frame_t / 16) & 1 ? C_YELLOW : C_WHITE);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    ui_fancy_center("THE ROLL IS EMPTY", 160, 40, 2, BLOOD, 4, C_INK, C_NIGHT);
    char b[64];
    snprintf(b, sizeof b, "HEARTS BURST %d OF 4", 4 - frl_w.hearts_left);
    text_center(b, 160, 84, C_WHITE);
    snprintf(b, sizeof b, "DOORS UNLOCKED %d   PLATES PRESSED %d", frl_w.doors, frl_w.switches);
    tiny_center(b, 160, 100, C_LIGHT);
    tiny_center("THORNKEEP STILL STANDS.", 160, 112, C_GREY);
    if (state_t > 120) text_center(GLYPH_A " TITLE", 160, 164, C_GREY);
}

static void draw_ending(void) {
    gfx_cls(C_NAVY);
    for (int i = 0; i < 60; i++) gfx_pset((i * 89 + 7) % 320, (i * 61 + 3) % 80, i % 4 ? C_BLUE : C_WHITE);
    gfx_rect(0, 90, SCREEN_W, 90, C_FOREST);
    gfx_dither(0, 90, SCREEN_W, 6, C_JADE, 8);
    frl_draw_keep(220, 28, frame_t, true);
    /* statues in the square, one for every volunteer lost (as many as fit) */
    int n = imin(frl_w.lost, 40);
    for (int k = 0; k < n; k++) {
        int x = 8 + (k % 20) * 10, y = 72 + (k / 20) * 12;
        gfx_rect(x, y + 2, 6, 8, C_GREY);
        gfx_rect(x + 1, y, 4, 3, C_LIGHT);
        gfx_hline(x - 1, x + 6, y + 10, C_SLATE);
    }
    static const char *const T[3] = {
        "WITH THE THORN HEARTS BURST, THORNKEEP FELL STONE BY STONE.",
        "HOLLOWAY CHOSE NEW STEWARDS, WHO SWORE TO GUARD ITS FOLK.",
        "IN THE SQUARE THEY RAISED A STATUE FOR EVERY VOLUNTEER LOST.",
    };
    for (int k = 0; k < 3; k++) tiny_center(T[k], 160, 104 + k * 10, C_WHITE);
    char b[64];
    snprintf(b, sizeof b, "VOLUNTEERS LOST %d   LEFT %d", frl_w.lost, frl_w.lives);
    text_center(b, 160, 138, C_CREAM);
    if (run_code) tiny_center("SLIM-PICK RUN: NO GOALS OR RECORDS", 160, 150, C_LIME);
    else if (frl_w.lost < FRL_CHERRY) tiny_center("FEWER THAN 50 LOST: EVERY STATUE HAS A NAME", 160, 150, C_YELLOW);
    else tiny_center("NOW TRY IT HAVING LOST FEWER THAN 50", 160, 150, C_GREY);
    if (state_t > 120) text_center(GLYPH_A " ON", 160, 166, C_LIME);
}

static const char *const CREDITS[] = {
    "FORLORN HOPE", "", "PRESENTED BY", "BEAMDOWN SOFTWORKS", "1987", "",
    "THE TRADES", "MASON", "HUNTER", "RUNNER", "TINKER", "SAPPER", "",
    "THE WAY IN", "THE CAMP AND THE OLD YEW", "THE UNDERCROFT", "THE CAVES", "THE DEEP", "THE TOWER", "THORNKEEP", "",
    "WHAT WAITED", "WALL-EYES  OOZLES  MIDGES  HORNETS", "SHELLBACKS  HATCHETEERS  SQUAWKERS", "TUSKERS  IDOLS  DRAKES  HORNET BELLS",
    "RUST KNIGHTS  BLOATERS  BROODHENS", "HORNHEADS  STINGBACKS  THE GULPER", "THE FOUR THORN HEARTS", "",
    "WHISPERED BY THE OLD STEWARDS", "SLIM-PICK", "",
    "FOR EVERY VOLUNTEER", "",
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
        if (credits_t > 60 * 24) text_center(GLYPH_A " TITLE", 160, 168, C_GREY);
    }
}

static void draw(void) {
    gfx_camera(0, 0);
    gfx_noclip();
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_CODE: draw_code(); break;
    case S_STORY: draw_story(); break;
    case S_PLAY: draw_play(); break;
    case S_OVER: draw_over(); break;
    case S_ENDING: draw_ending(); break;
    case S_CREDITS: draw_credits(); break;
    }
}

/* ------------------------------------------------------------------ */
/* the demo player's buttons                                            */

static int press(int b) { return (frame_t & 1) ? b : 0; }

static int frl_bot_buttons(void) {
    switch (state) {
    case S_TITLE:
        if (state_t < 20) return 0;
        if (title_sel != 0) return press(BTN_UP);
        return press(BTN_A);
    case S_PLAY:
        return (int)frl_bot(&frl_w, &bot);
    case S_CREDITS: return credits_t > 60 * 24 + 10 ? press(BTN_A) : BTN_A;
    case S_OVER: return 0;
    default: return state_t > 125 ? press(BTN_A) : 0;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void frl_load(void) {
    frl_art_load();
    frl_audio_load();
}

static void frl_start(void) {
    load_save();
    frl_bot_route(0);
    code_on = false;
    title_sel = 0;
    frl_world_new(&frl_w, 1, 0x1F);
    frl_bot_reset(&bot);
    title_enter();
}

static void frl_quit(void) {
    input_set_versus(false);
    save_now();
}

static void frl_label(int x, int y, int w, int h, int t) {
    /* Thornkeep at dusk, the troop's door and its counter */
    gfx_rect(x, y, w, h, C_NIGHT);
    gfx_rect(x, y + 20, w, 10, C_DUSK);
    gfx_dither(x, y + 30, w, 8, C_PURPLE, 6);
    gfx_rect(x, y + h - 14, w, 14, C_INK);
    frl_draw_keep(x + w - 70, y + h - 64, t, false);
    gfx_rect(x + 6, y + h - 30, 24, 16, C_EARTH);
    for (int k = 0; k < 14; k++) gfx_hline(x + 4 + k, x + 31 - k, y + h - 31 - k / 2, C_WINE);
    gfx_rect(x + 14, y + h - 22, 8, 8, C_INK);
    gfx_rect(x + 12, y + h - 30, 12, 7, C_INK);
    tiny_draw("99", x + 14, y + h - 29, C_CREAM);
    int ux = x + 34 + (t / 2) % 40;
    frl_draw_unit_at((t / 80) % FRC_COUNT, ux, y + h - 24, 1, 1 + (t / 6) % 2, false, 0);
    ui_fancy_text("FORLORN", x + 4, y + 3, 1, BLOOD, 4, C_INK, -1);
    ui_fancy_text("HOPE", x + 4, y + 13, 1, BLOOD, 4, C_INK, -1);
    (void)h;
}

/* key is name followed by a number */
static int num_key(const char *key, const char *name, int *n) {
    size_t len = strlen(name);
    if (strncmp(key, name, len) || key[len] < '0' || key[len] > '9') return 0;
    *n = atoi(key + len);
    return 1;
}

static int count_kind(int kind, bool all) {
    int n = 0;
    for (int i = 0; i < frl_w.nfoe; i++) n += (all || frl_w.foe[i].on) && frl_w.foe[i].kind == kind && (all ? i < frl_w.nplaced : 1);
    return n;
}

/* the map is sound: every row the right length, one base, one pad, four
 * plates, four hearts, every foe kind there, every block's plate there */
static int map_bad(void) {
    int bad = 0, nb = 0, np = 0;
    for (int y = 0; y < FRL_MH; y++) {
        if (!FRL_MAP[y] || (int)strlen(FRL_MAP[y]) != FRL_MW) { bad++; continue; }
        for (int x = 0; x < FRL_MW; x++) {
            nb += FRL_MAP[y][x] == 'B';
            np += FRL_MAP[y][x] == 'P';
        }
    }
    if (nb != 1 || np != 1) bad++;
    FrlWorld *w = &frl_w;
    for (int i = 0; i < FRL_PLATES; i++)
        if (!w->plate[i].tx && !w->plate[i].ty) bad++;
    if (w->hearts_left != 4) bad++;
    for (int k = FK_WALLEYE; k < FK_COUNT; k++) {
        if (k == FK_OOZLE || k == FK_MIDGE || k == FK_HORNET) continue; /* they come out of drains, combs and bells */
        if (!count_kind(k, true)) { bad++; fprintf(stderr, "no %s on the map\n", FRL_FOE_NAME[k]); }
    }
    if (!w->ndrain || !w->ncomb) bad++;
    /* every placed foe starts in the open (not inside the map) */
    for (int i = 0; i < w->nplaced; i++) {
        const FrlFoe *f = &w->foe[i];
        if (f->kind == FK_WALLEYE || f->kind == FK_GULPER) continue;
        if (frl_box_solid(w, f->x >> 8, f->y >> 8, f->w, f->h)) { bad++; fprintf(stderr, "%s %d stuck at %d,%d\n", FRL_FOE_NAME[f->kind], i, f->x >> 8, f->y >> 8); }
    }
    return bad;
}

static int frl_query(const char *key, int *out) {
    FrlWorld *w = &frl_w;
    const FrlUnit *u = &w->u;
    int i;
    if (!strcmp(key, "bot")) { *out = frl_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "phase")) { *out = w->phase; return 1; }
    if (!strcmp(key, "lives")) { *out = w->lives; return 1; }
    if (!strcmp(key, "lost")) { *out = w->lost; return 1; }
    if (!strcmp(key, "units")) { *out = w->units; return 1; }
    if (!strcmp(key, "keys")) { *out = w->keys; return 1; }
    if (!strcmp(key, "doors")) { *out = w->doors; return 1; }
    if (!strcmp(key, "switches")) { *out = w->switches; return 1; }
    if (!strcmp(key, "hearts")) { *out = w->hearts_left; return 1; }
    if (!strcmp(key, "sel")) { *out = w->sel; return 1; }
    if (!strcmp(key, "allowed")) { *out = w->allowed; return 1; }
    if (!strcmp(key, "players")) { *out = w->players; return 1; }
    if (!strcmp(key, "player")) { *out = w->phase == FWP_SELECT ? w->units % w->players : u->player; return 1; }
    if (!strcmp(key, "cls")) { *out = u->cls; return 1; }
    if (!strcmp(key, "x")) { *out = frl_unit_cx(w); return 1; }
    if (!strcmp(key, "y")) { *out = frl_unit_cy(w); return 1; }
    if (!strcmp(key, "bottom")) { *out = (u->y >> 8) + FRL_UH; return 1; }
    if (!strcmp(key, "vx")) { *out = u->vx; return 1; }
    if (!strcmp(key, "vy")) { *out = u->vy; return 1; }
    if (!strcmp(key, "ground")) { *out = u->ground; return 1; }
    if (!strcmp(key, "ammo")) { *out = u->ammo; return 1; }
    if (!strcmp(key, "charge")) { *out = u->charge; return 1; }
    if (!strcmp(key, "mode")) { *out = u->mode; return 1; }
    if (!strcmp(key, "face")) { *out = u->face; return 1; }
    if (!strcmp(key, "cause")) { *out = w->last_cause; return 1; }
    if (!strcmp(key, "way")) { *out = w->way_on; return 1; }
    if (!strcmp(key, "way_x")) { *out = w->way_x; return 1; }
    if (!strcmp(key, "way_y")) { *out = w->way_y; return 1; }
    if (!strcmp(key, "way_red")) { *out = frl_way_red(w); return 1; }
    if (!strcmp(key, "stones")) { *out = w->stones; return 1; }
    if (!strcmp(key, "pouches")) { *out = w->npouch; return 1; }
    if (!strcmp(key, "pipes")) { *out = w->npipe; return 1; }
    if (!strcmp(key, "on_pad")) { *out = frl_unit_on_pad(w); return 1; }
    if (!strcmp(key, "meta")) { *out = meta_t; return 1; }
    if (!strcmp(key, "code")) { *out = code_on; return 1; }
    if (!strcmp(key, "run_code")) { *out = run_code; return 1; }
    if (!strcmp(key, "title_sel")) { *out = title_sel; return 1; }
    if (!strcmp(key, "credits_t")) { *out = credits_t; return 1; }
    if (!strcmp(key, "runs")) { *out = sv.runs; return 1; }
    if (!strcmp(key, "wins")) { *out = sv.wins; return 1; }
    if (!strcmp(key, "best_lost")) { *out = sv.best_lost; return 1; }
    if (!strcmp(key, "most_doors")) { *out = sv.most_doors; return 1; }
    if (!strcmp(key, "most_switches")) { *out = sv.most_switches; return 1; }
    if (!strcmp(key, "plate_ever")) { *out = sv.plate_ever; return 1; }
    if (!strcmp(key, "art_ok")) { *out = frl_art_ok(); return 1; }
    if (!strcmp(key, "map_bad")) { *out = map_bad(); return 1; }
    if (!strcmp(key, "foes")) { *out = w->nplaced; return 1; }
    if (!strcmp(key, "last_hp")) { *out = w->nfoe > 0 ? w->foe[w->nfoe - 1].hp : -1; return 1; }
    if (!strcmp(key, "last_on")) { *out = w->nfoe > 0 ? w->foe[w->nfoe - 1].on : 0; return 1; }
    if (!strcmp(key, "last_x")) { *out = w->nfoe > 0 ? (w->foe[w->nfoe - 1].x >> 8) + w->foe[w->nfoe - 1].w / 2 : -1; return 1; }
    if (!strcmp(key, "last_y")) { *out = w->nfoe > 0 ? (w->foe[w->nfoe - 1].y >> 8) + w->foe[w->nfoe - 1].h / 2 : -1; return 1; }
    if (!strcmp(key, "last_state")) { *out = w->nfoe > 0 ? w->foe[w->nfoe - 1].state : -1; return 1; }
    if (!strcmp(key, "keys_total")) { *out = w->nkey; return 1; }
    if (!strcmp(key, "drains")) { *out = w->ndrain; return 1; }
    if (!strcmp(key, "combs")) { *out = w->ncomb; return 1; }
    if (!strcmp(key, "doors_total")) {
        int n = 0;
        for (int y = 0; y < FRL_MH; y++)
            for (int x = 0; x < FRL_MW; x++) n += w->tile[y][x] == FT_DOOR && (y == 0 || w->tile[y - 1][x] != FT_DOOR);
        *out = n;
        return 1;
    }
    if (!strcmp(key, "kinds")) {
        int n = 0;
        for (int k = FK_WALLEYE; k < FK_HEART; k++) n += count_kind(k, true) > 0 || k == FK_OOZLE || k == FK_MIDGE || k == FK_HORNET;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "minions")) { int n = 0; for (int k = w->nplaced; k < w->nfoe; k++) n += w->foe[k].on; *out = n; return 1; }
    if (!strcmp(key, "foe_shots")) { int n = 0; for (int k = 0; k < FRL_SHOTS; k++) n += w->shot[k].on && !w->shot[k].mine; *out = n; return 1; }
    if (!strcmp(key, "my_shots")) { int n = 0; for (int k = 0; k < FRL_SHOTS; k++) n += w->shot[k].on && w->shot[k].mine; *out = n; return 1; }
    if (!strcmp(key, "bot_unit")) { *out = bot.unit; return 1; }
    if (!strcmp(key, "bot_step")) { *out = bot.step; return 1; }
    if (!strcmp(key, "bot_stuck")) { *out = bot.stuck; return 1; }
    if (!strcmp(key, "bot_plans")) { *out = frl_bot_plans(); return 1; }
    if (num_key(key, "alive", &i)) { *out = count_kind(i, false); return 1; }
    if (num_key(key, "plate", &i)) { *out = i < FRL_PLATES ? w->plate[i].down : 0; return 1; }
    if (num_key(key, "key_taken", &i)) { *out = i < w->nkey ? w->key[i].taken : -1; return 1; }
    if (num_key(key, "foe_hp", &i)) { *out = i < w->nfoe ? w->foe[i].hp : -1; return 1; }
    if (num_key(key, "foe_on", &i)) { *out = i < w->nfoe ? w->foe[i].on : 0; return 1; }
    if (num_key(key, "foe_kind", &i)) { *out = i < w->nfoe ? w->foe[i].kind : 0; return 1; }
    if (num_key(key, "foe_x", &i)) { *out = i < w->nfoe ? (w->foe[i].x >> 8) + w->foe[i].w / 2 : -1; return 1; }
    if (num_key(key, "foe_y", &i)) { *out = i < w->nfoe ? (w->foe[i].y >> 8) + w->foe[i].h / 2 : -1; return 1; }
    if (num_key(key, "foe_state", &i)) { *out = i < w->nfoe ? w->foe[i].state : -1; return 1; }
    if (num_key(key, "tile", &i)) { *out = frl_tile(w, i % 1000, i / 1000); return 1; }   /* y * 1000 + x */
    if (num_key(key, "solid", &i)) { *out = frl_solid(w, i % 1000, i / 1000); return 1; }
    if (num_key(key, "first_hp", &i) || num_key(key, "first_on", &i) || num_key(key, "first_x", &i) || num_key(key, "first_state", &i)) {
        /* the first placed foe of kind i */
        *out = -1;
        for (int k = 0; k < w->nplaced; k++)
            if (w->foe[k].kind == i) {
                const FrlFoe *f = &w->foe[k];
                *out = key[6] == 'h' ? f->hp : key[6] == 'o' ? f->on : key[6] == 'x' ? (f->x >> 8) + f->w / 2 : f->state;
                break;
            }
        return 1;
    }
    if (num_key(key, "first", &i)) {
        /* the index of the first placed foe of kind i (-1 if none) */
        *out = -1;
        for (int k = 0; k < w->nplaced; k++)
            if (w->foe[k].kind == i) { *out = k; break; }
        return 1;
    }
    return 0;
}

static int frl_cheat(const char *cmd) {
    FrlWorld *w = &frl_w;
    int a, b, c;
    if (sscanf(cmd, "start %d", &a) == 1) {
        /* a fresh run, straight to the door */
        new_run(a == 2 ? 2 : 1);
        play_enter();
        return 1;
    }
    if (sscanf(cmd, "class %d", &a) == 1) {
        if (w->phase != FWP_SELECT) return 1;
        w->sel = iclamp(a, 0, FRC_COUNT - 1);
        frl_choose(w, w->sel);
        music_play(FRL_MUS_CLASS[w->u.cls]);
        return 1;
    }
    if (sscanf(cmd, "place %d %d", &a, &b) == 2) {
        /* the volunteer's middle at (a, b), px */
        w->u.x = (a - FRL_UW / 2) << 8;
        w->u.y = (b - FRL_UH / 2) << 8;
        w->u.vx = w->u.vy = 0;
        w->u.mode = FUM_WALK;
        w->cam_x = (int16_t)iclamp(a - SCREEN_W / 2, 0, FRL_MW * FRL_T - SCREEN_W);
        w->cam_y = (int16_t)iclamp(b - FRL_VH / 2, 0, FRL_MH * FRL_T - FRL_VH);
        return 1;
    }
    if (sscanf(cmd, "lives %d", &a) == 1) { w->lives = a; return 1; }
    if (sscanf(cmd, "lost %d", &a) == 1) { w->lost = a; return 1; }
    if (sscanf(cmd, "keys %d", &a) == 1) { w->keys = a; return 1; }
    if (sscanf(cmd, "ammo %d", &a) == 1) { w->u.ammo = (int16_t)a; return 1; }
    if (sscanf(cmd, "allowed %d", &a) == 1) { w->allowed = (uint8_t)a; return 1; }
    if (sscanf(cmd, "foe_hp %d %d", &a, &b) == 2) { if (a >= 0 && a < w->nfoe) w->foe[a].hp = (int16_t)b; return 1; }
    if (sscanf(cmd, "foe_off %d", &a) == 1) { if (a >= 0 && a < w->nfoe) w->foe[a].on = 0; return 1; }
    if (sscanf(cmd, "foe_at %d %d %d", &a, &b, &c) == 3) {
        /* move placed foe a so its middle is at (b, c) */
        if (a >= 0 && a < w->nfoe) {
            FrlFoe *f = &w->foe[a];
            f->x = (b - f->w / 2) << 8;
            f->y = (c - f->h / 2) << 8;
            f->vx = f->vy = 0;
        }
        return 1;
    }
    if (sscanf(cmd, "spawn %d %d %d", &a, &b, &c) == 3) { frl_foe_add(w, a, b, c); return 1; }
    if (sscanf(cmd, "tile %d %d %d", &a, &b, &c) == 3) { if (a >= 0 && a < FRL_MW && b >= 0 && b < FRL_MH) w->tile[b][a] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "way %d %d", &a, &b) == 2) { w->way_on = 1; w->way_x = (int16_t)a; w->way_y = (int16_t)b; return 1; }
    if (sscanf(cmd, "hearts_hp %d", &a) == 1) {
        for (int k = 0; k < w->nfoe; k++) if (w->foe[k].kind == FK_HEART && w->foe[k].on) w->foe[k].hp = (int16_t)a;
        return 1;
    }
    if (sscanf(cmd, "hearts_at %d %d", &a, &b) == 2) {
        /* the hearts in a row, 20 px apart, middles from (a, b) */
        int k = 0;
        for (int i = 0; i < w->nfoe; i++)
            if (w->foe[i].kind == FK_HEART && w->foe[i].on) {
                w->foe[i].x = (a + k * 20 - 10) << 8;
                w->foe[i].y = (b - 10) << 8;
                k++;
            }
        return 1;
    }
    if (sscanf(cmd, "kill_kind %d", &a) == 1) {
        for (int i = 0; i < w->nfoe; i++)
            if (w->foe[i].kind == a) w->foe[i].on = 0;
        return 1;
    }
    if (!strcmp(cmd, "give_up")) { frl_give_up(w); return 1; }
    if (!strcmp(cmd, "die")) { frl_unit_die(w, FCAUSE_TEST); return 1; }
    if (!strcmp(cmd, "no_foes")) {
        /* everything but the hearts and the gulper quietly leaves */
        for (int k = 0; k < w->nfoe; k++)
            if (w->foe[k].kind != FK_HEART && w->foe[k].kind != FK_GULPER) w->foe[k].on = 0;
        w->ndrain = 0;
        w->ncomb = 0;
        for (int k = 0; k < FRL_SHOTS; k++) w->shot[k].on = 0;
        return 1;
    }
    if (!strcmp(cmd, "win")) {
        for (int k = 0; k < w->nfoe; k++)
            if (w->foe[k].kind == FK_HEART && w->foe[k].on) frl_hurt_foe(w, k, 999);
        return 1;
    }
    if (!strcmp(cmd, "code")) { code_on = true; return 1; }
    if (!strcmp(cmd, "title")) { title_enter(); return 1; }
    if (sscanf(cmd, "bot_debug %d", &a) == 1) { frl_bot_debug = a; return 1; }
    if (sscanf(cmd, "bot_route %d", &a) == 1) { frl_bot_route(a); frl_bot_reset(&bot); return 1; }
    return 0;
}

const GameDef GAME_FORLORN = {
    "forlorn",
    "FORLORN HOPE",
    "1987",
    "ADVENTURE",
    "99 LIVES, ONE MAP. EVERY VOLUNTEER YOU SPEND LEAVES SOMETHING BEHIND.",
    {"WEIGH DOWN A FLOOR PLATE", "BURST THE FOUR THORN HEARTS", "BURST THEM WITH FEWER THAN 50 LOST"},
    GLYPH_LEFT GLYPH_RIGHT "\tWALK / PICK A TRADE\n"
    GLYPH_A "\tJUMP (HOLD: HIGHER)\n"
    GLYPH_B "\tATTACK\n"
    GLYPH_B " HELD\tFLASH, LET GO: GIVE UP\n"
    GLYPH_DOWN "\tDOWN A CHUTE\n"
    GLYPH_UP "\tTHROUGH A WAYSTONE\n"
    "START\tPAUSE",
    C_WINE, C_FOREST,
    frl_load, frl_start, update, draw, frl_quit, frl_label, frl_query, frl_cheat,
    "MORTOL II", 32,
    NULL,
};
