/* CLARION CALL - Clary flies the Clarion to the Carillon to find her brother.
 * Cartridge 35 of UFO 40, a tribute to Campanella 2 (UFO 50 #35).
 * See docs/games/35-clarion-call.md.
 *
 * This file is the screens and the run: the nine areas (two in each of four
 * regions, chosen on the station map, then the Crown), the doors and what
 * happens behind them, the sextons' chain, the Hush Engines, the bosses,
 * the escape, the three endings, the records, the goals, the code and the
 * test hooks. The rules of a map are in clc_world.c, behind a door in
 * clc_sub.c.
 *
 * Like the original, a run is played in one sitting and never saved: the
 * cartridge keeps only its records. */
#include "clc.h"

enum { S_TITLE, S_CODE, S_STORY, S_CARD, S_MAP, S_SUB, S_STATION, S_DEAD, S_OVER, S_ENDING, S_CREDITS };
enum { END_NONE, END_BAD, END_GOLD, END_TRUE };

typedef struct Save {
    uint32_t magic;
    uint32_t gear_bought;      /* the original's stat: gear bought, every run */
    uint16_t runs, gold, cherry, bad;
    uint8_t best_area;         /* the furthest area reached (1..9) */
    uint8_t charms;
    uint16_t pad;
} Save;
#define SAVE_MAGIC 0x434c4301u

typedef struct Run {
    ClcPlayer p;
    uint8_t region, area;      /* where we are */
    uint8_t path[4];           /* the region taken at each stage of the run */
    uint8_t engines;           /* Hush Engines broken (bit per stage 0..2) */
    uint8_t scrolls;           /* peal sheets */
    int8_t chain;              /* where the next sexton waits (-1: the chain is broken) */
    uint8_t chain_plan[3];     /* where each sexton will send her */
    uint8_t final_sage;        /* the Spire's sexton has opened the secret way */
    uint8_t cheat;             /* the FULL-PEAL code */
    uint8_t shortcut;          /* went through Cogtown's yellow door */
    uint8_t ending;
    uint8_t areas;             /* areas entered */
    uint32_t seed;
    int bought;
    int deaths_cause;
} Run;

static Save sv;
static Run run;
static ClcWorld world;
static ClcSub sub;
static int cur_door = -1;
static int state, state_t, frame_t;
static int title_sel, station_sel, station_n;
static uint8_t station_opt[3];
static char code[9] = "AAAAAAAA";
static int code_pos, code_msg_t;
static const char *code_msg;
static bool code_on;
static int credits_t;
static int banner_t;
static int bot_goal = BOT_GOLD;
static bool bot_on;

#define CODE_PEAL "FULLPEAL"

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
/* the run                                                              */

static void goto_state(int s) {
    state = s;
    state_t = 0;
    input_consume();
}

static uint32_t area_seed(int region, int area) {
    uint32_t h = run.seed ^ (uint32_t)(region * 0x9e3779b1u) ^ (uint32_t)(area * 0x85ebca6bu);
    h ^= h >> 15;
    h *= 0x2c1b3c6du;
    h ^= h >> 12;
    return h;
}

static int area_number(void) { return clc_region_tier(run.region) * 2 + run.area + 1; }

static void region_music(void) {
    if (world.kind == WK_ESCAPE) music_play(CLC_MUS_ESCAPE);
    else if (world.timer_on) music_play(CLC_MUS_DASH);
    else music_play(CLC_MUS_REGION[world.region]);
}

static void award(int bit) {
    if (run.cheat) return;
    game_award(bit);
}

static void start_area(int region, int area) {
    run.region = (uint8_t)region;
    run.area = (uint8_t)area;
    run.path[clc_region_tier(region)] = (uint8_t)region;
    run.areas++;
    if (!run.cheat && area_number() > sv.best_area) sv.best_area = (uint8_t)area_number();
    if (run.cheat) {
        run.p.hp = run.p.hpmax;
        run.p.fuel = run.p.fuelmax;
    }
    if (region == RG_SPIRE && area == 2) {
        clc_gen_crown(&world, run.final_sage != 0);
    } else {
        ClcAreaSpec sp = {0};
        sp.region = (uint8_t)region;
        sp.area = (uint8_t)area;
        sp.seed = area_seed(region, area);
        int tier = clc_region_tier(region);
        if (area == 1) {
            if (region == RG_CELLARS) sp.sage = 1;
            else if (region == RG_SPIRE) sp.sage = run.scrolls == 3 && run.chain == RG_SPIRE;
            else sp.sage = run.chain == region && run.scrolls == tier;
            sp.final_sage = region == RG_SPIRE;
        }
        clc_gen_world(&world, &sp);
    }
    clc_world_start(&world, &run.p, true);
    clc_fx_clear();
    cur_door = -1;
    banner_t = 150;
    goto_state(S_CARD);
    game_set_pausable(true);
    music_play(CLC_MUS_REGION[region]);
}

static void new_run(bool cheat) {
    memset(&run, 0, sizeof run);
    run.p.hp = run.p.hpmax = CLC_HP_START;
    run.p.fuel = run.p.fuelmax = CLC_FUEL_START;
    run.chain = RG_CELLARS;
    run.seed = rng_next(&g_rng);
    Rng r;
    rng_seed(&r, run.seed ^ 0xC1A2u);
    run.chain_plan[0] = (uint8_t)(rng_chance(&r, 60) ? RG_ARBOR : RG_ICE);
    run.chain_plan[1] = (uint8_t)(RG_GULLET + rng_range(&r, 0, 2));
    run.chain_plan[2] = RG_SPIRE;
    run.cheat = cheat;
    if (cheat) {
        /* eight of the sixteen, a bigger bar and a bigger tank */
        static const int8_t EIGHT[8] = {G_TWIN, G_SEEKER, G_SIPHON, G_FEATHER, G_FAN, G_CHARM, G_MAGNET, G_PURSE};
        for (int k = 0; k < 8; k++) run.p.gear |= 1u << EIGHT[k];
        run.p.hp = run.p.hpmax = 32;
        run.p.fuel = run.p.fuelmax = 1600;
    } else {
        if (sv.runs < 65535) sv.runs++;
        save_now();
    }
    clc_bot_reset();
}

static void title_enter(void) {
    goto_state(S_TITLE);
    game_set_pausable(false);
    music_play(CLC_MUS_TITLE);
}

static void run_over(void) {
    if (!run.cheat) save_now();
    goto_state(S_OVER);
    game_set_pausable(false);
    music_play(CLC_MUS_OVER);
}

/* the station map: the next stage's regions */
static void open_station(void) {
    int tier = clc_region_tier(run.region) + 1;
    station_n = 0;
    if (tier == 1) { station_opt[0] = RG_ARBOR; station_opt[1] = RG_ICE; station_n = 2; }
    else if (tier == 2) { station_opt[0] = RG_GULLET; station_opt[1] = RG_COG; station_opt[2] = RG_CLOISTER; station_n = 3; }
    else { station_opt[0] = RG_SPIRE; station_n = 1; }
    station_sel = 0;
    goto_state(S_STATION);
    game_set_pausable(true);
    music_play(CLC_MUS_MAP);
}

static void region_done(void) {
    int tier = clc_region_tier(run.region);
    if (sub.done && sub.boss_kind <= 1 && tier < 3) {
        run.engines |= (uint8_t)(1u << tier);
    }
    /* a sexton's chain only holds if she went where she was sent */
    open_station();
}

static void start_escape(int ending) {
    run.ending = (uint8_t)ending;
    clc_gen_escape(&world);
    clc_world_start(&world, &run.p, true);
    cur_door = -1;
    banner_t = 150;
    goto_state(S_MAP);
    music_play(CLC_MUS_ESCAPE);
}

static void finish_run(void) {
    if (!run.cheat) {
        if (run.ending == END_TRUE) { award(GOAL_SAUCER); award(GOAL_ALIEN); if (sv.cherry < 65535) sv.cherry++; }
        else if (run.ending == END_GOLD) { award(GOAL_SAUCER); if (sv.gold < 65535) sv.gold++; }
        else if (sv.bad < 65535) sv.bad++;
        save_now();
    }
    goto_state(S_ENDING);
    game_set_pausable(false);
    music_play(run.ending == END_TRUE ? CLC_MUS_TRUE : run.ending == END_GOLD ? CLC_MUS_END : CLC_MUS_SAD);
}

/* ------------------------------------------------------------------ */
/* doors                                                                */

static void open_door(int k) {
    ClcDoor *d = &world.door[k];
    if (d->room == RM_SHORTCUT) {
        /* Cogtown's yellow door: straight up to the Spire, past the engine */
        run.shortcut = 1;
        run.path[2] = RG_COG;
        start_area(RG_SPIRE, 0);
        return;
    }
    cur_door = k;
    ClcSubSpec sp = {0};
    sp.room = d->room;
    sp.region = world.region;
    sp.area = world.area;
    sp.seed = area_seed(world.region, world.area) ^ (uint32_t)(k * 977 + 13) ^ d->seed;
    sp.used = d->used;
    sp.item = -1;
    sp.owned = run.p.gear;
    sp.charm_price = 100;
    int tier = clc_region_tier(world.region);
    switch (d->room) {
    case RM_CAVE: sp.item = (int8_t)(d->used ? -2 : clc_chest_item(world.region, world.area)); break;
    case RM_GOLDCAVE:
        sp.engine = world.area == 1 && tier < 3;
        sp.lobber = sp.engine && world.region != RG_CLOISTER;
        break;
    case RM_NPC: {
        int line = (int)(d->seed % 6);
        if (world.region == RG_SPIRE) line = 6;
        sp.hint = (int8_t)line;
        break;
    }
    case RM_SAGE: sp.hint = (int8_t)(world.region == RG_SPIRE ? -2 : run.chain_plan[tier]); break;
    case RM_CURSED: sp.item = (int8_t)(world.region == RG_ARBOR ? G_FAN : G_TWIN); break;
    default: break;
    }
    if (sp.item >= 0 && sp.item < G_COUNT && clc_has(&run.p, sp.item) && d->room == RM_CURSED) sp.item = IT_DRUM;
    clc_gen_sub(&sub, &sp);
    clc_fx_clear();
    goto_state(S_SUB);
    switch (d->room) {
    case RM_HUSH: music_play(CLC_MUS_HUSH); break;
    case RM_TOCK: music_play(CLC_MUS_TOCK); break;
    case RM_SHOP: case RM_MEGA: case RM_HEALTH: case RM_CHARMSHOP: music_play(CLC_MUS_SHOP); break;
    case RM_CAVE: case RM_GOLDCAVE: music_play(CLC_MUS_CAVE); break;
    default: break;
    }
}

/* what a visit changed for good */
static void sub_news(void) {
    if (cur_door < 0) return;
    ClcDoor *d = &world.door[cur_door];
    if (sub.gave_fuel) d->used = 1;
    if (sub.ev & CEV_BUY) {
        run.bought++;
        if (!run.cheat && sv.gear_bought < 0xFFFFFFFFu) sv.gear_bought++;
    }
    if (sub.got_item >= 0) {
        if (d->room == RM_BOON || d->room == RM_ARMOR) d->used = 1;
        if (sub.got_item == IT_SHEET) {
            d->used = 1;
            /* the sexton's sheet, and where the next one waits */
            int tier = clc_region_tier(world.region);
            if (world.region == RG_SPIRE) {
                run.final_sage = 1;
            } else if (run.scrolls == tier && run.chain == world.region) {
                run.scrolls++;
                run.chain = (int8_t)run.chain_plan[tier];
            }
        }
    }
    if (d->room == RM_CAVE && (sub.ev & CEV_OPEN)) d->used = 1;
    if ((d->room == RM_TRIAL || d->room == RM_CURSED) && sub.done) d->used = 1;
    if (sub.lit_switch && (d->room == RM_LIGHTS || d->room == RM_MAGNETS)) {
        d->used = 1;
        if (d->room == RM_LIGHTS) world.dark = 0;
        else world.magnets_off = 1;
    }
}

/* ------------------------------------------------------------------ */
/* updates                                                              */

static void news_sounds(uint32_t ev) {
    if (ev & CEV_DIE) sfx_play_name("clc_die");
    else if (ev & CEV_BOSS_DOWN) sfx_play_name("clc_blast");
    else if (ev & CEV_BLAST) sfx_play_name("clc_blast");
    else if (ev & CEV_HURT) sfx_play_name("clc_hurt");
    else if (ev & CEV_BADLAND) sfx_play_name("clc_hurt");
    else if (ev & CEV_BUMP) sfx_play_name("clc_bump");
    if (ev & CEV_NOTE) sfx_play_name("clc_note");
    else if (ev & CEV_TIMER) sfx_play_name("clc_timer");
    else if (ev & CEV_RING) sfx_play_name("clc_ring");
    else if (ev & CEV_ITEM) sfx_play_name("clc_item");
    else if (ev & CEV_BUY) sfx_play_name("clc_item");
    else if (ev & CEV_COIN) sfx_play_name("clc_coin");
    else if (ev & CEV_BOSS_HIT) sfx_play_name("clc_bosshit");
    else if (ev & CEV_KILL) sfx_play_name("clc_kill");
    else if (ev & CEV_LAND) sfx_play_name("clc_land");
    else if (ev & CEV_BOARD) sfx_play_name("clc_board");
    else if (ev & CEV_DOOR) sfx_play_name("clc_door");
    else if (ev & CEV_CLANG) sfx_play_name("clc_clang");
    else if (ev & CEV_BREAK) sfx_play_name("clc_break");
    else if (ev & CEV_HIT) sfx_play_name("clc_hit");
    else if (ev & CEV_SLASH) sfx_play_name("clc_slash");
    else if (ev & CEV_SHOOT) sfx_play_name("clc_shoot");
    else if (ev & CEV_TALK) sfx_play_name("clc_talk");
    else if (ev & CEV_OPEN) sfx_play_name("clc_open");
    else if (ev & CEV_LATE) sfx_play_name("clc_late");
    else if (ev & CEV_FOESHOT && (frame_t & 3) == 0) sfx_play_name("clc_foeshot");
    else if (ev & CEV_DRY && (frame_t & 15) == 0) sfx_play_name("clc_dry");
}

static unsigned play_buttons(void) {
    unsigned b = 0;
    if (btn(BTN_LEFT)) b |= BTN_LEFT;
    if (btn(BTN_RIGHT)) b |= BTN_RIGHT;
    if (btn(BTN_UP)) b |= BTN_UP;
    if (btn(BTN_DOWN)) b |= BTN_DOWN;
    if (btn(BTN_A)) b |= BTN_A;
    if (btn(BTN_B)) b |= BTN_B;
    return b;
}

static void check_gift(void) {
    if (clc_has(&run.p, G_CHARM) && !run.cheat) {
        if (sv.charms < 255 && !(g_progress.goals[game_current_index()] & GOAL_BEACON)) sv.charms++;
        award(GOAL_BEACON);
    }
}

static void update_map(void) {
    if (banner_t > 0) banner_t--;
    bool was_timer = world.timer_on;
    clc_world_step(&world, &run.p, play_buttons());
    clc_fx_feed(world.fx, world.nfx);
    news_sounds(world.ev);
    if (clc_bot_debug && (world.ev & (CEV_HURT | CEV_DIE)))
        fprintf(stderr, "HURT map region %d area %d at %d,%d hp %d foot %d by %d\n", world.region, world.area,
                (int)((world.on_foot ? world.cl.x : world.f.x) >> 8), (int)((world.on_foot ? world.cl.y : world.f.y) >> 8),
                run.p.hp, world.on_foot, (unsigned)world.hurt_by);
    check_gift();
    if (!was_timer && world.timer_on) music_play(CLC_MUS_DASH);
    if (world.ev & CEV_DIE) { goto_state(S_DEAD); music_stop(); return; }
    if (world.ev & CEV_ESCAPED) { finish_run(); return; }
    if (world.enter >= 0) open_door(world.enter);
}

static void update_sub(void) {
    clc_sub_step(&sub, &run.p, play_buttons());
    clc_fx_feed(sub.fx, sub.nfx);
    news_sounds(sub.ev);
    if (clc_bot_debug && (sub.ev & (CEV_HURT | CEV_DIE)))
        fprintf(stderr, "HURT sub room %d region %d at %d hp %d\n", sub.room, sub.region, (int)(sub.cl.x >> 8), run.p.hp);
    sub_news();
    check_gift();
    if (sub.ev & CEV_TIMER) music_play(sub.boss_kind == 2 ? CLC_MUS_HUSH : sub.boss_kind == 3 ? CLC_MUS_TOCK : CLC_MUS_BOSS);
    if (sub.ev & CEV_BOSS_DOWN) music_play(CLC_MUS_CLEAR);
    if (sub.ev & CEV_DIE) { goto_state(S_DEAD); music_stop(); return; }
    if (sub.leave == 1) {
        clc_world_return(&world, cur_door);
        clc_fx_clear();
        goto_state(S_MAP);
        region_music();
        return;
    }
    if (sub.leave == 2) {
        int room = world.door[cur_door].room;
        if (room == RM_HUSH) { start_escape(run.engines == 7 ? END_GOLD : END_BAD); return; }
        if (room == RM_TOCK) { start_escape(END_TRUE); return; }
        /* through the gold cave */
        if (run.region == RG_SPIRE) { start_area(RG_SPIRE, run.area + 1); return; }
        if (run.area == 0) { start_area(run.region, 1); return; }
        region_done();
    }
}

static void check_code(void) {
    if (!strcmp(code, CODE_PEAL)) {
        code_on = !code_on;
        code_msg = code_on ? "FULL-PEAL: A LOUDER BELL" : "FULL-PEAL IS OFF";
        sfx_play_name("clc_item");
    } else {
        code_msg = "NO SUCH CODE";
        sfx_play_name("ui_error");
    }
    code_msg_t = 150;
}

static void update_code(void) {
    if (code_msg_t > 0) code_msg_t--;
    if (btn_repeat(BTN_LEFT)) { code_pos = (code_pos + 7) % 8; sfx_play_name("clc_move"); }
    if (btn_repeat(BTN_RIGHT)) { code_pos = (code_pos + 1) % 8; sfx_play_name("clc_move"); }
    if (btn_repeat(BTN_UP)) { code[code_pos] = (char)(code[code_pos] == 'Z' ? 'A' : code[code_pos] + 1); sfx_play_name("clc_move"); }
    if (btn_repeat(BTN_DOWN)) { code[code_pos] = (char)(code[code_pos] == 'A' ? 'Z' : code[code_pos] - 1); sfx_play_name("clc_move"); }
    if (btnp(BTN_B)) { sfx_play_name("ui_back"); goto_state(S_TITLE); return; }
    if (btnp(BTN_A) && state_t > 5) check_code();
}

static void update(void) {
    frame_t++;
    state_t++;
    switch (state) {
    case S_TITLE:
        if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { title_sel ^= 1; sfx_play_name("clc_move"); }
        if (btnp(BTN_B) && state_t > 10) { game_exit_to_library(); return; }
        if (state_t > 15 && btnp(BTN_A)) {
            sfx_play_name("ui_ok");
            if (title_sel == 0) {
                new_run(code_on);
                goto_state(S_STORY);
            } else { code_pos = 0; code_msg = NULL; goto_state(S_CODE); }
        }
        break;
    case S_CODE: update_code(); break;
    case S_STORY:
        if (state_t > 30 && btnp(BTN_A)) { sfx_play_name("ui_ok"); start_area(RG_CELLARS, 0); }
        break;
    case S_CARD:
        if ((state_t > 30 && btnp(BTN_A)) || state_t > 120) { goto_state(S_MAP); region_music(); }
        break;
    case S_MAP: update_map(); break;
    case S_SUB: update_sub(); break;
    case S_STATION:
        if (btn_repeat(BTN_LEFT) || btn_repeat(BTN_UP)) { station_sel = (station_sel + station_n - 1) % station_n; sfx_play_name("clc_move"); }
        if (btn_repeat(BTN_RIGHT) || btn_repeat(BTN_DOWN)) { station_sel = (station_sel + 1) % station_n; sfx_play_name("clc_move"); }
        if (state_t > 20 && btnp(BTN_A)) { sfx_play_name("ui_ok"); start_area(station_opt[station_sel], 0); }
        break;
    case S_DEAD:
        if (state_t > 100) run_over();
        break;
    case S_OVER:
        if (state_t > 90 && btnp(BTN_A)) { sfx_play_name("ui_ok"); title_enter(); }
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

static const uint8_t GOLDG[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
static const uint8_t PINKG[] = {C_WHITE, C_PINK, C_MAGENTA, C_PURPLE};
static const uint8_t ICEG[] = {C_WHITE, C_ICE, C_CYAN, C_SKY};

static const char *roman(int a) { return a == 0 ? "I" : a == 1 ? "II" : "III"; }

static void draw_title(void) {
    clc_draw_backdrop(RG_SPIRE, frame_t);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    ui_fancy_center("CLARION CALL", 160, 12, 3, PINKG, 4, C_INK, C_PURPLE);
    tiny_center("NINE AREAS " GLYPH_DOT " SEVEN REGIONS " GLYPH_DOT " ONE TANK OF FUEL", 160, 42, C_CREAM);
    int sx = 160 + clc_sin(frame_t) * 70 / 127, sy = 66 + clc_sin(frame_t * 3) * 4 / 127;
    clc_draw_ship(sx, sy, clc_cos(frame_t) >= 0 ? 1 : -1, (frame_t / 4) % 3 == 0, frame_t);
    static const char *const ITEMS[2] = {"START", "CODE"};
    ui_panel(120, 82, 80, 32, C_INK, C_PINK);
    for (int k = 0; k < 2; k++) {
        int y = 88 + k * 12;
        text_center(ITEMS[k], 160, y, title_sel == k ? C_YELLOW : C_LIGHT);
        if (title_sel == k) ui_cursor(126, y, frame_t);
    }
    if (code_on) {
        gfx_rect(110, 117, 100, 9, C_INK);
        tiny_center("FULL-PEAL IS ON", 160, 119, C_LIME);
    }
    gfx_rect(0, 134, SCREEN_W, 46, C_INK);
    char b[96];
    snprintf(b, sizeof b, "GEAR BOUGHT %lu  " GLYPH_DOT "  RUNS %d  " GLYPH_DOT "  FURTHEST AREA %d OF 9",
             (unsigned long)sv.gear_bought, sv.runs, sv.best_area);
    tiny_center(b, 160, 139, C_YELLOW);
    snprintf(b, sizeof b, "HOME WITH ANSEL %d  " GLYPH_DOT "  TOCK STOPPED %d  " GLYPH_DOT "  ALONE %d", sv.gold, sv.cherry, sv.bad);
    tiny_center(b, 160, 149, C_LIGHT);
    text_center(GLYPH_A " CHOOSE   " GLYPH_B " LIBRARY", 160, 164, C_GREY);
}

static void draw_code(void) {
    clc_draw_backdrop(RG_COG, frame_t);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    ui_fancy_center("CODE", 160, 14, 2, ICEG, 4, C_INK, C_NAVY);
    tiny_center("A WORD FROM THE BELFRY CHANGES THE RUN.", 160, 40, C_LIGHT);
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
    for (int i = 0; i < 70; i++) gfx_pset((i * 97 + 13) % 320, (i * 53 + 7) % 180, i % 5 ? C_DUSK : C_GREY);
    clc_draw_face(0, 24, 18, 3);
    clc_draw_face(1, 248, 18, 3);
    static const char *const L[] = {
        "ANSEL FLEW THE TINKLER UP TO THE CARILLON, THE GREAT",
        "BELL-TOWER STATION, TO HEAR WHY ITS BELLS HAD GONE QUIET.",
        "HE HAS NOT BEEN HEARD FROM SINCE.",
        "",
        "HIS SISTER CLARY FILLS THE CLARION'S TANK, ONCE, AND",
        "GOES AFTER HIM. THE STATION IS CRAWLING WITH THINGS.",
        "THREE HUSH ENGINES KEEP ANSEL LOCKED AWAY.",
    };
    for (int k = 0; k < ARRAY_LEN(L); k++) tiny_center(L[k], 160, 82 + k * 9, k < 3 ? C_LIGHT : C_PINK);
    if (state_t > 30) text_center(GLYPH_A " FLY", 160, 164, (frame_t / 16) & 1 ? C_YELLOW : C_WHITE);
}

static void draw_card(void) {
    clc_draw_backdrop(run.region, frame_t);
    gfx_darken_rect(0, 46, SCREEN_W, 74, 2);
    char b[48];
    snprintf(b, sizeof b, "AREA %d OF 9", area_number());
    text_center(b, 160, 56, C_LIGHT);
    snprintf(b, sizeof b, "%s %s", CLC_REGION_NAME[run.region], roman(run.area));
    ui_fancy_center(b, 160, 70, 2, GOLDG, 4, C_INK, C_WINE);
    if (run.region == RG_SPIRE && run.area == 2) tiny_center("THE CROWN", 160, 94, C_CREAM);
    else tiny_center("FIND TEN NOTES. THE GOLD DOOR OPENS ON THE TENTH.", 160, 98, C_CREAM);
}

static void draw_play_map(void) {
    clc_draw_world(&world, &run.p, frame_t);
    clc_draw_hud(&run.p, world.notes, world.timer, world.timer_on, world.kind == WK_ESCAPE ? 60 : 30, frame_t);
    if (world.kind == WK_ESCAPE && banner_t > 0) {
        gfx_rect(80, 40, 160, 14, C_INK);
        text_center("GET OUT! FLY UP!", 160, 43, (frame_t / 6) & 1 ? C_YELLOW : C_RED);
    }
}

static void draw_station(void) {
    uint8_t visited[RG_COUNT] = {0};
    for (int t = 0; t < 4; t++)
        if (t <= clc_region_tier(run.region)) visited[run.path[t]] = 1;
    clc_draw_station(station_opt[station_sel], visited, run.region, frame_t);
    ui_fancy_center("THE CARILLON", 160, 6, 2, ICEG, 4, C_INK, C_NAVY);
    char b[64];
    snprintf(b, sizeof b, "ONWARD TO %s", CLC_REGION_NAME[station_opt[station_sel]]);
    gfx_rect(0, 150, SCREEN_W, 30, C_INK);
    text_center(b, 160, 154, C_YELLOW);
    text_center(station_n > 1 ? GLYPH_DPAD " CHOOSE   " GLYPH_A " GO" : GLYPH_A " GO", 160, 167, C_GREY);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    ui_fancy_center("LOST IN THE BELFRY", 160, 48, 2, ICEG, 4, C_INK, C_NAVY);
    char b[64];
    snprintf(b, sizeof b, "REACHED %s %s (AREA %d OF 9)", CLC_REGION_NAME[run.region], roman(run.area), area_number());
    text_center(b, 160, 88, C_LIGHT);
    snprintf(b, sizeof b, "ENGINES BROKEN %d   GEAR BOUGHT %d", (run.engines & 1) + (run.engines >> 1 & 1) + (run.engines >> 2 & 1), run.bought);
    text_center(b, 160, 102, C_GREY);
    if (state_t > 90) text_center(GLYPH_A " TITLE", 160, 164, C_GREY);
}

static void draw_ending(void) {
    int e = run.ending;
    gfx_cls(e == END_TRUE ? C_NAVY : C_INK);
    for (int i = 0; i < 70; i++) gfx_pset((i * 89 + 7) % 320, (i * 61 + 3) % 110, i % 4 ? C_BLUE : C_WHITE);
    /* the Carillon behind, lit or dark */
    int bx = 230, by = 30;
    gfx_rect(bx, by + 20, 40, 70, e == END_TRUE ? C_SLATE : C_NIGHT);
    gfx_rect(bx + 10, by, 20, 22, e == END_TRUE ? C_GREY : C_DUSK);
    gfx_circ(bx + 20, by + 12, 6, e == END_TRUE ? C_YELLOW : C_NIGHT);
    for (int k = 0; k < 5; k++) gfx_rect(bx + 6 + k * 6, by + 34 + (k & 1) * 12, 3, 4, e == END_TRUE ? C_AMBER : C_DUSK);
    int sx = (state_t * 2) % 400 - 40;
    clc_draw_ship(sx, 70, 1, true, frame_t);
    if (e != END_BAD) clc_draw_ship(sx - 22, 74, 1, true, frame_t + 3);
    clc_draw_face(0, 16, 112, 3);
    if (e != END_BAD) clc_draw_face(1, 260, 112, 3);
    static const char *const BAD[3] = {"CLARY GOT OUT. THE CLARION SAILS FOR HOME,", "AND SHE WONDERS HOW ANSEL IS GETTING ON.", "PROBABLY FINE. PROBABLY!"};
    static const char *const GOLD[3] = {"CLARY AND ANSEL GOT OUT TOGETHER. BEHIND", "THEM THE CARILLON HANGS SILENT AND EMPTY,", "ITS BELLS NEVER TO RING AGAIN."};
    static const char *const TRUE_[3] = {"GRANDSIRE TOCK IS STOPPED. THE CARILLON'S BELLS", "RING OUT AND ITS FOLK COME HOME. CLARY AND ANSEL", "SET OFF FOR WHEREVER IS NEXT."};
    const char *const *L = e == END_TRUE ? TRUE_ : e == END_GOLD ? GOLD : BAD;
    for (int k = 0; k < 3; k++) tiny_center(L[k], 160, 118 + k * 10, C_WHITE);
    ui_fancy_center(e == END_TRUE ? "THE BELLS RING AGAIN" : e == END_GOLD ? "HOME WITH ANSEL" : "HOME ALONE", 160, 10, 2,
                    e == END_TRUE ? GOLDG : e == END_GOLD ? PINKG : ICEG, 4, C_INK, C_NIGHT);
    if (state_t > 120) text_center(GLYPH_A " ON", 160, 166, C_LIME);
}

static const char *const CREDITS[] = {
    "CLARION CALL", "", "PRESENTED BY", "BEAMDOWN SOFTWORKS", "1987", "",
    "PILOT", "CLARY, IN THE CLARION", "", "LOST", "ANSEL", "",
    "THE CARILLON", "THE CELLARS", "THE ARBORETUM", "THE ICEHOUSE", "THE GULLET", "COGTOWN", "THE CLOISTER", "THE SPIRE", "",
    "IN THE WAY", "THE LOBBER", "THE HUSH ENGINES", "LADY HUSH", "GRANDSIRE TOCK", "",
    "THE SEXTONS", "THE DOZING TORTOISE", "", "WHISPERED IN THE BELFRY", "FULL-PEAL", "",
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
        text_center(CREDITS[k], 160, y, head ? C_PINK : C_LIGHT);
    }
    if (y0 + n * 12 < 60) {
        ui_fancy_center("THE END", 160, 72, 3, GOLDG, 4, C_INK, C_WINE);
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
    case S_CARD: draw_card(); break;
    case S_MAP: draw_play_map(); break;
    case S_SUB:
        clc_draw_sub(&sub, &run.p, frame_t);
        clc_draw_hud(&run.p, world.notes, world.timer, world.timer_on, world.kind == WK_ESCAPE ? 60 : 30, frame_t);
        break;
    case S_STATION: draw_station(); break;
    case S_DEAD:
        if (world.enter < 0 && cur_door < 0) clc_draw_world(&world, &run.p, frame_t);
        else if (cur_door >= 0) clc_draw_sub(&sub, &run.p, frame_t);
        else clc_draw_world(&world, &run.p, frame_t);
        clc_draw_hud(&run.p, world.notes, world.timer, world.timer_on, world.kind == WK_ESCAPE ? 60 : 30, frame_t);
        if (state_t > 40) {
            gfx_rect(100, 80, 120, 16, C_INK);
            text_center("THE RUN IS OVER", 160, 84, C_RED);
        }
        break;
    case S_OVER: draw_over(); break;
    case S_ENDING: draw_ending(); break;
    case S_CREDITS: draw_credits(); break;
    }
    gfx_camera(0, 0);
}

/* ------------------------------------------------------------------ */
/* the demo player's buttons                                            */

static int press(int b) { return (frame_t & 1) ? b : 0; }

static int bot_buttons(void) {
    switch (state) {
    case S_TITLE:
        if (state_t < 20) return 0;
        if (title_sel != 0) return press(BTN_DOWN);
        return press(BTN_A);
    case S_MAP:
    case S_SUB: {
        ClcBotView v = {state == S_SUB, &world, &sub, &run.p, bot_goal, run.chain, run.scrolls, true};
        return (int)clc_bot(&v);
    }
    case S_STATION: {
        if (state_t < 25) return 0;
        int want = 0;
        int tier = clc_region_tier(station_opt[0]);
        for (int k = 0; k < station_n; k++) {
            int r = station_opt[k];
            if (bot_goal == BOT_CHERRY && r == run.chain_plan[tier - 1]) want = k;
            if (bot_goal == BOT_BAD && r == RG_COG) want = k;
            if (bot_goal == BOT_GOLD && tier == 2 && r == RG_GULLET) want = k;
        }
        if (station_sel != want) return press(BTN_RIGHT);
        return press(BTN_A);
    }
    case S_CREDITS: return credits_t > 60 * 24 + 10 ? press(BTN_A) : BTN_A;
    case S_OVER: case S_DEAD: return 0;
    default: return state_t > 125 ? press(BTN_A) : 0;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void clc_load(void) {
    clc_art_load();
    clc_audio_load();
}

static void clc_start(void) {
    load_save();
    memset(&run, 0, sizeof run);
    code_on = false;
    title_sel = 0;
    clc_bot_reset();
    title_enter();
}

static void clc_quit(void) { save_now(); }

static void clc_label(int x, int y, int w, int h, int t) {
    /* the Clarion rising past the Carillon's bells */
    gfx_rect(x, y, w, h, C_NIGHT);
    for (int i = 0; i < 18; i++) gfx_pset(x + (i * 37 + 5) % w, y + (i * 23 + 3) % (h - 10), i % 3 ? C_DUSK : C_LIGHT);
    int tx = x + w - 30;
    gfx_rect(tx, y + 14, 22, h - 14, C_SLATE);
    gfx_rect(tx + 5, y + 6, 12, 10, C_GREY);
    gfx_circ(tx + 11, y + 22, 4, C_AMBER);
    gfx_rect(tx + 4, y + 34, 4, 5, C_YELLOW);
    gfx_rect(tx + 14, y + 40, 4, 5, C_YELLOW);
    int sx = x + w / 3 + clc_sin(t * 2) * 10 / 127, sy = y + 32 + clc_sin(t * 5) * 3 / 127;
    clc_draw_ship(sx, sy, clc_cos(t * 2) >= 0 ? 1 : -1, (t / 4) & 1, t);
    ui_fancy_text("CLARION", x + 4, y + 3, 1, PINKG, 4, C_INK, -1);
    (void)h;
}

static int num_key(const char *key, const char *name, int *n) {
    size_t len = strlen(name);
    if (strncmp(key, name, len) || key[len] < '0' || key[len] > '9') return 0;
    *n = atoi(key + len);
    return 1;
}

/* every region's maps over many seeds: how many fail their checks */
static int gen_bad(int seeds) {
    static ClcWorld tmp;
    int bad = 0;
    char why[96];
    for (int sd = 0; sd < seeds; sd++)
        for (int r = 0; r < RG_COUNT; r++)
            for (int a = 0; a < 2; a++) {
                ClcAreaSpec sp = {0};
                sp.region = (uint8_t)r;
                sp.area = (uint8_t)a;
                sp.sage = a == 1;
                sp.seed = (uint32_t)(sd * 7919 + r * 131 + a * 17 + 1);
                clc_gen_world(&tmp, &sp);
                if (clc_check_world(&tmp, why, sizeof why)) {
                    bad++;
                    fprintf(stderr, "gen seed %d region %d area %d: %s\n", sd, r, a, why);
                }
            }
    clc_gen_crown(&tmp, true);
    if (clc_check_world(&tmp, why, sizeof why)) { bad++; fprintf(stderr, "crown: %s\n", why); }
    clc_gen_escape(&tmp);
    if (clc_check_world(&tmp, why, sizeof why)) { bad++; fprintf(stderr, "escape: %s\n", why); }
    return bad;
}

static int count_world(int kind) {
    int n = 0;
    for (int i = 0; i < world.ne; i++) n += world.e[i].on && world.e[i].kind == kind;
    return n;
}

static int count_sub(int kind) {
    int n = 0;
    for (int i = 0; i < sub.ne; i++) n += sub.e[i].on && sub.e[i].kind == kind;
    return n;
}

static int find_door_room(int room) {
    for (int k = 0; k < world.nd; k++)
        if (world.door[k].room == room) return k;
    return -1;
}

static int clc_query(const char *key, int *out) {
    int i;
    if (!strcmp(key, "bot")) { *out = bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "region")) { *out = run.region; return 1; }
    if (!strcmp(key, "area")) { *out = run.area; return 1; }
    if (!strcmp(key, "area_no")) { *out = area_number(); return 1; }
    if (!strcmp(key, "hp")) { *out = run.p.hp; return 1; }
    if (!strcmp(key, "hpmax")) { *out = run.p.hpmax; return 1; }
    if (!strcmp(key, "fuel")) { *out = run.p.fuel; return 1; }
    if (!strcmp(key, "fuelmax")) { *out = run.p.fuelmax; return 1; }
    if (!strcmp(key, "coins")) { *out = (int)run.p.coins; return 1; }
    if (!strcmp(key, "gear")) { *out = (int)run.p.gear; return 1; }
    if (num_key(key, "has", &i)) { *out = clc_has(&run.p, i); return 1; }
    if (!strcmp(key, "key")) { *out = run.p.key; return 1; }
    if (!strcmp(key, "dead")) { *out = run.p.dead; return 1; }
    if (!strcmp(key, "notes")) { *out = world.notes; return 1; }
    if (!strcmp(key, "notes_left")) { *out = count_world(EK_NOTE); return 1; }
    if (!strcmp(key, "plums")) { *out = count_world(EK_PLUM); return 1; }
    if (!strcmp(key, "rings")) { *out = count_world(EK_RING); return 1; }
    if (!strcmp(key, "late")) { *out = count_world(EK_LATE); return 1; }
    if (!strcmp(key, "timer")) { *out = world.timer; return 1; }
    if (!strcmp(key, "timer_on")) { *out = world.timer_on; return 1; }
    if (!strcmp(key, "timer_shown")) { int per = world.kind == WK_ESCAPE ? 60 : 30; *out = (world.timer + per - 1) / per; return 1; }
    if (!strcmp(key, "on_foot")) { *out = world.on_foot; return 1; }
    if (!strcmp(key, "ship")) { *out = world.ship; return 1; }
    if (!strcmp(key, "x")) { *out = (int)((world.on_foot ? world.cl.x : world.f.x) >> 8); return 1; }
    if (!strcmp(key, "y")) { *out = (int)((world.on_foot ? world.cl.y : world.f.y) >> 8); return 1; }
    if (!strcmp(key, "ship_x")) { *out = (int)(world.f.x >> 8); return 1; }
    if (!strcmp(key, "ship_y")) { *out = (int)(world.f.y >> 8); return 1; }
    if (!strcmp(key, "vx")) { *out = world.f.vx; return 1; }
    if (!strcmp(key, "vy")) { *out = world.f.vy; return 1; }
    if (!strcmp(key, "face")) { *out = world.on_foot ? world.cl.face : world.f.face; return 1; }
    if (!strcmp(key, "leeches")) { *out = world.leeches; return 1; }
    if (!strcmp(key, "doors")) { *out = world.nd; return 1; }
    if (num_key(key, "door_room", &i)) { *out = i < world.nd ? world.door[i].room : -1; return 1; }
    if (num_key(key, "door_type", &i)) { *out = i < world.nd ? world.door[i].type : -1; return 1; }
    if (num_key(key, "door_used", &i)) { *out = i < world.nd ? world.door[i].used : -1; return 1; }
    if (num_key(key, "door_hidden", &i)) { *out = i < world.nd ? world.door[i].hidden : -1; return 1; }
    if (num_key(key, "find_room", &i)) { *out = find_door_room(i); return 1; }
    if (!strcmp(key, "dark")) { *out = world.dark; return 1; }
    if (!strcmp(key, "magnets_off")) { *out = world.magnets_off; return 1; }
    if (!strcmp(key, "world_kind")) { *out = world.kind; return 1; }
    if (!strcmp(key, "world_bad")) { *out = clc_check_world(&world, NULL, 0); return 1; }
    if (!strcmp(key, "pieces_bad")) { *out = clc_pieces_bad(); return 1; }
    if (num_key(key, "gen_bad", &i)) { *out = gen_bad(i); return 1; }
    if (num_key(key, "count", &i)) { *out = state == S_SUB ? count_sub(i) : count_world(i); return 1; }
    if (!strcmp(key, "room")) { *out = state == S_SUB ? sub.room : -1; return 1; }
    if (!strcmp(key, "sub_done")) { *out = sub.done; return 1; }
    if (!strcmp(key, "sub_locked")) { *out = sub.locked; return 1; }
    if (!strcmp(key, "sub_x")) { *out = (int)(sub.cl.x >> 8); return 1; }
    if (!strcmp(key, "sub_y")) { *out = (int)(sub.cl.y >> 8); return 1; }
    if (!strcmp(key, "sub_w")) { *out = sub.w; return 1; }
    if (!strcmp(key, "sub_crouch")) { *out = sub.cl.crouch; return 1; }
    if (!strcmp(key, "sub_ground")) { *out = sub.cl.ground; return 1; }
    if (!strcmp(key, "exit_open")) { *out = sub.exit_open; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = sub.boss_hp; return 1; }
    if (!strcmp(key, "bought")) { *out = sub.bought; return 1; }
    if (!strcmp(key, "items")) { *out = count_sub(EK_ITEM); return 1; }
    if (num_key(key, "item_a", &i)) {
        int n = 0;
        *out = -1;
        for (int k = 0; k < sub.ne; k++)
            if (sub.e[k].on && sub.e[k].kind == EK_ITEM && n++ == i) { *out = sub.e[k].a; break; }
        return 1;
    }
    if (num_key(key, "item_price", &i)) {
        int n = 0;
        *out = -1;
        for (int k = 0; k < sub.ne; k++)
            if (sub.e[k].on && sub.e[k].kind == EK_ITEM && n++ == i) { *out = sub.e[k].b; break; }
        return 1;
    }
    if (!strcmp(key, "npc_shots")) { *out = sub.npc_shots; return 1; }
    if (!strcmp(key, "talk")) { *out = sub.talk; return 1; }
    if (!strcmp(key, "engines")) { *out = run.engines; return 1; }
    if (!strcmp(key, "scrolls")) { *out = run.scrolls; return 1; }
    if (!strcmp(key, "chain")) { *out = run.chain; return 1; }
    if (num_key(key, "chain_plan", &i)) { *out = i < 3 ? run.chain_plan[i] : -1; return 1; }
    if (!strcmp(key, "final_sage")) { *out = run.final_sage; return 1; }
    if (!strcmp(key, "ending")) { *out = run.ending; return 1; }
    if (!strcmp(key, "shortcut")) { *out = run.shortcut; return 1; }
    if (!strcmp(key, "runs")) { *out = sv.runs; return 1; }
    if (!strcmp(key, "gear_bought")) { *out = (int)sv.gear_bought; return 1; }
    if (!strcmp(key, "best_area")) { *out = sv.best_area; return 1; }
    if (!strcmp(key, "wins_gold")) { *out = sv.gold; return 1; }
    if (!strcmp(key, "wins_true")) { *out = sv.cherry; return 1; }
    if (!strcmp(key, "wins_bad")) { *out = sv.bad; return 1; }
    if (!strcmp(key, "code")) { *out = code_on; return 1; }
    if (!strcmp(key, "cheat_run")) { *out = run.cheat; return 1; }
    if (!strcmp(key, "code_pos")) { *out = code_pos; return 1; }
    if (!strcmp(key, "title_sel")) { *out = title_sel; return 1; }
    if (!strcmp(key, "station_n")) { *out = station_n; return 1; }
    if (!strcmp(key, "station_sel")) { *out = station_sel; return 1; }
    if (num_key(key, "station_opt", &i)) { *out = i < station_n ? station_opt[i] : -1; return 1; }
    if (!strcmp(key, "art_ok")) { *out = clc_art_ok(); return 1; }
    if (!strcmp(key, "credits_t")) { *out = credits_t; return 1; }
    if (!strcmp(key, "bot_target")) { *out = clc_bot_target(); return 1; }
    if (!strcmp(key, "landing_icon")) { *out = world.landing_icon; return 1; }
    if (!strcmp(key, "ship_inv")) { *out = world.ship_inv; return 1; }
    {
        /* entities of the map (or behind the door, when Clary is there) */
        bool insub = state == S_SUB;
        const ClcEnt *E = insub ? sub.e : world.e;
        int NE = insub ? sub.ne : world.ne;
        if (num_key(key, "find_kind", &i)) {
            *out = -1;
            for (int k = 0; k < NE; k++)
                if (E[k].on && E[k].kind == i) { *out = k; break; }
            return 1;
        }
        if (num_key(key, "ent_x", &i)) { *out = i < NE && E[i].on ? (int)(E[i].x >> 8) : -1; return 1; }
        if (num_key(key, "ent_y", &i)) { *out = i < NE && E[i].on ? (int)(E[i].y >> 8) : -1; return 1; }
        if (num_key(key, "ent_on", &i)) { *out = i < NE ? E[i].on : 0; return 1; }
        if (num_key(key, "ent_hp", &i)) { *out = i < NE ? E[i].hp : 0; return 1; }
        if (num_key(key, "ent_kind", &i)) { *out = i < NE ? E[i].kind : 0; return 1; }
        if (num_key(key, "ent_flag", &i)) { *out = i < NE ? E[i].flag : 0; return 1; }
        if (num_key(key, "shotk", &i)) {
            int n = 0;
            const ClcShot *S = insub ? sub.shot : world.shot;
            int NS = insub ? CLC_SSHOTS : CLC_SHOTS;
            for (int k = 0; k < NS; k++) n += S[k].on && S[k].kind == i;
            *out = n;
            return 1;
        }
    }
    if (num_key(key, "door_c", &i)) { *out = i < world.nd ? world.door[i].c : -1; return 1; }
    if (num_key(key, "door_r", &i)) { *out = i < world.nd ? world.door[i].r : -1; return 1; }
    if (num_key(key, "pad_x", &i)) { int x = -1, y; if (i < world.nd) clc_door_pad(&world.door[i], &x, &y); *out = x; return 1; }
    if (num_key(key, "pad_y", &i)) { int x, y = -1; if (i < world.nd) clc_door_pad(&world.door[i], &x, &y); *out = y; return 1; }
    if (!strcmp(key, "gold_door")) { *out = world.gold_door; return 1; }
    if (!strcmp(key, "air_t")) { *out = world.air_t; return 1; }
    if (!strcmp(key, "ladder")) { *out = world.cl.ladder; return 1; }
    if (!strcmp(key, "ground")) { *out = world.on_foot ? world.cl.ground : 0; return 1; }
    if (!strcmp(key, "sub_inv")) { *out = sub.cl.inv; return 1; }
    if (!strcmp(key, "sub_face")) { *out = sub.cl.face; return 1; }
    if (!strcmp(key, "sub_vy")) { *out = sub.cl.vy; return 1; }
    if (!strcmp(key, "sub_vx")) { *out = sub.cl.vx; return 1; }
    if (!strcmp(key, "chest_item")) { *out = sub.chest_item; return 1; }
    if (!strcmp(key, "pods_hatched")) {
        int n = 0;
        for (int k = 0; k < world.ne; k++) n += world.e[k].on && world.e[k].kind == EK_POD && world.e[k].flag;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "lobber_hp") || !strcmp(key, "engine_hp")) {
        int kind = key[0] == 'l' ? EK_LOBBER : EK_ENGINE;
        *out = -1;
        for (int k = 0; k < sub.ne; k++)
            if (sub.e[k].on && sub.e[k].kind == kind) *out = sub.e[k].hp;
        return 1;
    }
    if (!strcmp(key, "arena_x")) { *out = sub.arena ? sub.arena_c * CLC_ST : -1; return 1; }
    if (!strcmp(key, "head_open")) {
        *out = 0;
        for (int k = 0; k < sub.ne; k++)
            if (sub.e[k].on && sub.e[k].kind == EK_TOCKHEAD) *out = sub.e[k].flag;
        return 1;
    }
    if (!strcmp(key, "skull_x")) {
        *out = -999;
        for (int k = 0; k < sub.ne; k++)
            if (sub.e[k].on && sub.e[k].kind == EK_SKULL) *out = (int)(sub.e[k].x >> 8);
        return 1;
    }
    if (!strcmp(key, "nrings")) { *out = world.nrings; return 1; }
    if (!strcmp(key, "ring_pause")) { *out = world.ring_pause; return 1; }
    if (!strcmp(key, "lit_switch")) { *out = sub.lit_switch; return 1; }
    if (!strcmp(key, "spawn_x")) { *out = world.spawn_c * CLC_T + CLC_T / 2; return 1; }
    if (!strcmp(key, "spawn_y")) { *out = (world.spawn_r + 1) * CLC_T - 4; return 1; }
    if (!strcmp(key, "shots")) {
        int n = 0;
        for (int k = 0; k < CLC_SHOTS; k++) n += world.shot[k].on;
        for (int k = 0; k < CLC_SSHOTS && state == S_SUB; k++) n += sub.shot[k].on;
        *out = n;
        return 1;
    }
    return 0;
}

/* put Clary on foot in front of door k (or the ship there) */
static void stand_at(int k) {
    clc_world_return(&world, k);
    const ClcDoor *d = &world.door[k];
    int x, y;
    clc_door_pad(d, &x, &y);
    world.f = (ChmFlight){x * 256, y * 256, 0, 0, 1};
    world.ship = SM_PARKED;
}

static int clc_cheat(const char *cmd) {
    int a, b, c;
    if (sscanf(cmd, "area %d %d", &a, &b) == 2) {
        /* a fresh run, straight into region a, area b */
        new_run(false);
        int tier = clc_region_tier(a);
        for (int t = 0; t < tier; t++) run.path[t] = (uint8_t)(t == 0 ? RG_CELLARS : t == 1 ? RG_ARBOR : RG_GULLET);
        start_area(iclamp(a, 0, RG_COUNT - 1), iclamp(b, 0, 2));
        goto_state(S_MAP);
        return 1;
    }
    if (sscanf(cmd, "goto %d %d", &a, &b) == 2) { start_area(iclamp(a, 0, RG_COUNT - 1), iclamp(b, 0, 2)); goto_state(S_MAP); return 1; }
    if (sscanf(cmd, "runseed %d", &a) == 1) { run.seed = (uint32_t)a; return 1; }
    if (sscanf(cmd, "gear %d", &a) == 1) { clc_give(&run.p, a); check_gift(); return 1; }
    if (sscanf(cmd, "give %d", &a) == 1) { clc_give(&run.p, a); return 1; }
    if (sscanf(cmd, "coins %d", &a) == 1) { run.p.coins = a; return 1; }
    if (sscanf(cmd, "fuel %d", &a) == 1) { run.p.fuel = (int16_t)a; return 1; }
    if (sscanf(cmd, "hp %d", &a) == 1) { run.p.hp = (int16_t)a; return 1; }
    if (!strcmp(cmd, "notes")) { clc_world_notes_done(&world, &run.p); return 1; }
    if (sscanf(cmd, "notes %d", &a) == 1) {
        for (int i = 0; i < world.ne && world.notes < a; i++)
            if (world.e[i].on && world.e[i].kind == EK_NOTE) {
                world.f.x = world.e[i].x;
                world.f.y = world.e[i].y;
                world.on_foot = 0;
                world.ship = SM_PILOT;
                clc_world_step(&world, &run.p, 0);
            }
        return 1;
    }
    if (sscanf(cmd, "place %d %d", &a, &b) == 2) {
        world.on_foot = 0;
        world.ship = SM_PILOT;
        world.air_t = 255;
        world.f.x = a * 256;
        world.f.y = b * 256;
        world.f.vx = world.f.vy = 0;
        return 1;
    }
    if (sscanf(cmd, "foot %d %d", &a, &b) == 2) {
        /* Clary on her feet at (a, b), b her feet; the ship stays where it is */
        if (world.ship == SM_PILOT) world.ship = SM_PARKED;
        world.on_foot = 1;
        world.cl = (ClcWalker){0};
        world.cl.x = a * 256;
        world.cl.y = b * 256;
        world.cl.fall_from = world.cl.y;
        world.cl.face = 1;
        world.cl.ground = 1;
        return 1;
    }
    if (sscanf(cmd, "goto_kind %d", &a) == 1) {
        /* the ship onto the first thing of kind a (a ring, a plum, a coin ...) */
        for (int k = 0; k < world.ne; k++)
            if (world.e[k].on && world.e[k].kind == a) {
                world.on_foot = 0;
                world.ship = SM_PILOT;
                world.air_t = 255;
                world.f.x = world.e[k].x;
                world.f.y = world.e[k].y;
                world.f.vx = world.f.vy = 0;
                break;
            }
        return 1;
    }
    if (sscanf(cmd, "sub_goto_kind %d", &a) == 1) {
        /* Clary next to (left of) the first thing of kind a behind the door */
        for (int k = 0; k < sub.ne; k++)
            if (sub.e[k].on && sub.e[k].kind == a) {
                sub.cl.x = sub.e[k].x - 20 * 256;
                sub.cl.vx = sub.cl.vy = 0;
                break;
            }
        return 1;
    }
    if (sscanf(cmd, "vel %d %d", &a, &b) == 2) { world.f.vx = a; world.f.vy = b; return 1; }
    if (sscanf(cmd, "face %d", &a) == 1) { world.f.face = (int8_t)(a < 0 ? -1 : 1); world.cl.face = world.f.face; sub.cl.face = world.f.face; return 1; }
    if (sscanf(cmd, "at_door %d", &a) == 1) { if (a >= 0 && a < world.nd) stand_at(a); return 1; }
    if (sscanf(cmd, "at_room %d", &a) == 1) { int k = find_door_room(a); if (k >= 0) stand_at(k); return 1; }
    if (sscanf(cmd, "enter %d", &a) == 1) {
        /* straight through door a (as if UP were pressed in front of it) */
        if (a >= 0 && a < world.nd) { stand_at(a); open_door(a); }
        return 1;
    }
    if (sscanf(cmd, "enter_room %d", &a) == 1) { int k = find_door_room(a); if (k >= 0) { stand_at(k); open_door(k); } return 1; }
    if (sscanf(cmd, "sub_place %d %d", &a, &b) == 2) {
        sub.cl.x = a * 256;
        sub.cl.y = b * 256;
        sub.cl.vx = sub.cl.vy = 0;
        sub.cl.fall_from = sub.cl.y;
        return 1;
    }
    if (sscanf(cmd, "room %d", &a) == 1) {
        /* a room of kind a behind the first door of this map (tests) */
        if (world.nd > 0) {
            stand_at(0);
            uint8_t keep = world.door[0].room;
            world.door[0].room = (uint8_t)iclamp(a, 0, RM_COUNT - 1);
            open_door(0);
            world.door[0].room = keep;
        }
        return 1;
    }
    if (!strcmp(cmd, "key_off")) { run.p.key = 0; return 1; }
    if (sscanf(cmd, "chain_plan %d %d", &a, &b) == 2) { run.chain_plan[0] = (uint8_t)a; run.chain_plan[1] = (uint8_t)b; return 1; }
    if (!strcmp(cmd, "dump_sub")) {
        for (int k = 0; k < sub.ne; k++)
            if (sub.e[k].on)
                fprintf(stderr, "sub ent %d kind %d at %d,%d hp %d flag %d\n", k, sub.e[k].kind, (int)(sub.e[k].x >> 8),
                        (int)(sub.e[k].y >> 8), sub.e[k].hp, sub.e[k].flag);
        return 1;
    }
    if (!strcmp(cmd, "tortoise")) {
        /* the friendly sort here is the dozing tortoise */
        for (int k = 0; k < sub.ne; k++)
            if (sub.e[k].on && sub.e[k].kind == EK_NPC && sub.e[k].flag == 0) sub.e[k].flag = 1;
        return 1;
    }
    if (sscanf(cmd, "sub_kill %d", &a) == 1) {
        for (int k = 0; k < sub.ne; k++)
            if (sub.e[k].on && sub.e[k].kind == a) sub.e[k].on = 0;
        return 1;
    }
    if (!strcmp(cmd, "sub_at_exit")) {
        if (sub.exit_c >= 0) { sub.cl.x = (sub.exit_c * CLC_ST + 8) * 256; sub.cl.y = (CLC_SH - 1) * CLC_ST * 256; sub.cl.vx = sub.cl.vy = 0; }
        return 1;
    }
    if (sscanf(cmd, "sub_tile %d %d %d", &a, &b, &c) == 3) {
        if (a >= 0 && a < CLC_SW && b >= 0 && b < CLC_SH) { sub.tile[b][a] = (uint8_t)c; sub.ver++; }
        return 1;
    }
    if (sscanf(cmd, "sub_ent %d %d %d", &a, &b, &c) == 3) { clc_sub_ent_add(&sub, a, b, c); return 1; }
    if (sscanf(cmd, "ent %d %d %d", &a, &b, &c) == 3) { clc_ent_add(&world, a, b, c); return 1; }
    if (sscanf(cmd, "tile %d %d %d", &a, &b, &c) == 3) { clc_set_tile(&world, a, b, c); return 1; }
    if (!strcmp(cmd, "boss_down")) { clc_sub_boss_down(&sub); return 1; }
    if (!strcmp(cmd, "no_foes")) {
        for (int i = 0; i < world.ne; i++)
            if (world.e[i].on && (world.e[i].kind < EK_NOTE || world.e[i].kind == EK_COIN)) world.e[i].on = 0;
        for (int i = 0; i < sub.ne; i++)
            if (sub.e[i].on && sub.e[i].kind >= EK_STINGER && sub.e[i].kind <= EK_FLAME) sub.e[i].on = 0;
        return 1;
    }
    if (!strcmp(cmd, "clear_shots")) { memset(world.shot, 0, sizeof world.shot); memset(sub.shot, 0, sizeof sub.shot); return 1; }
    if (sscanf(cmd, "engines %d", &a) == 1) { run.engines = (uint8_t)a; return 1; }
    if (sscanf(cmd, "scrolls %d %d", &a, &b) == 2) { run.scrolls = (uint8_t)a; run.chain = (int8_t)b; return 1; }
    if (!strcmp(cmd, "final_sage")) { run.final_sage = 1; return 1; }
    if (sscanf(cmd, "timer %d", &a) == 1) { world.timer = (int16_t)a; return 1; }
    if (sscanf(cmd, "bot_goal %d", &a) == 1) { bot_goal = a; bot_on = true; return 1; }
    if (sscanf(cmd, "bot_debug %d", &a) == 1) { clc_bot_debug = a; return 1; }
    if (!strcmp(cmd, "code_on")) { code_on = true; return 1; }
    if (!strcmp(cmd, "title")) { title_enter(); return 1; }
    if (!strcmp(cmd, "dump_map")) {
        /* the map as text, for working on the generator */
        for (int r = 0; r < world.h; r++) {
            char line[CLC_MW + 1];
            for (int c = 0; c < world.w; c++) {
                int t = world.tile[r][c];
                line[c] = t == MT_AIR ? '.' : t == MT_LADDER ? 'H' : t == MT_COIN ? '$' : t == MT_GATE_OPEN ? ':' : '#';
            }
            line[world.w] = 0;
            for (int i = 0; i < world.ne; i++) {
                const ClcEnt *e = &world.e[i];
                if (!e->on) continue;
                int c = (int)(e->x >> 8) / CLC_T, r2 = (int)(e->y >> 8) / CLC_T;
                if (r2 == r && c >= 0 && c < world.w) line[c] = e->kind == EK_NOTE ? 'n' : e->kind == EK_POD ? 'o' : 'e';
            }
            for (int k = 0; k < world.nd; k++) {
                if (world.door[k].r == r) line[world.door[k].c] = (char)('A' + k);
                if (world.door[k].r == r) line[world.door[k].pad_c] = (char)('a' + k);
            }
            if (r == world.spawn_r) line[world.spawn_c] = 'S';
            fprintf(stderr, "%s\n", line);
        }
        return 1;
    }
    if (sscanf(cmd, "station %d", &a) == 1) { run.region = (uint8_t)a; open_station(); return 1; }
    if (sscanf(cmd, "escape %d", &a) == 1) { start_escape(a); return 1; }
    if (!strcmp(cmd, "die")) { run.p.dead = 1; world.ev |= CEV_DIE; goto_state(S_DEAD); return 1; }
    return 0;
}

const GameDef GAME_CLARION = {
    "clarion",
    "CLARION CALL",
    "1987",
    "ADVENTURE",
    "ONE TANK OF FUEL, A STATION FULL OF DOORS, AND A BROTHER TO BRING HOME.",
    {"GET THE HOMING CHARM", "FLY HOME WITH ANSEL", "STOP GRANDSIRE TOCK"},
    GLYPH_LEFT GLYPH_RIGHT "\tSTEER / WALK\n"
    GLYPH_A " (HOLD)\tTHRUST / JUMP\n"
    GLYPH_B "\tSLASH / SHOOT (HOLD)\n"
    GLYPH_DOWN "\tFALL FASTER / CROUCH\n"
    GLYPH_UP "\tAIM UP, DOORS, BOARD\n"
    "START\tPAUSE",
    C_PINK, C_PURPLE,
    clc_load, clc_start, update, draw, clc_quit, clc_label, clc_query, clc_cheat,
    "CAMPANELLA 2", 35,
    NULL,
};
