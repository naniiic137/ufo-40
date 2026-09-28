/* RIMSHIRE - the Brass Banner and the Plum Banner flick it out for the
 * shire. Cartridge 41 of UFO 40, a tribute to Lords of Diskonia (UFO 50 #41).
 * Every rule and where it comes from is in docs/games/41-rimshire.md.
 * This file is the cartridge: the title, the campaign's ten wars, the
 * streak and the two-player war, the board and battle screens, the save,
 * the goals, the demo player and the test hooks. The rules themselves are
 * in rimshire_map.c (the board) and rimshire_battle.c (the flick battles). */
#include "rimshire.h"

enum { S_TITLE, S_STORY, S_SELECT, S_CARD, S_WAR, S_BATTLE, S_WAROVER, S_END };
enum { MODE_CAMPAIGN, MODE_STREAK, MODE_VERSUS };

typedef struct Save {
    uint32_t magic;
    uint8_t cleared[RSH_SCENARIOS];
    uint8_t story_seen, pad;
    uint16_t streak, best_streak;
    uint16_t wars_won, wars_lost;
    uint16_t battles_won, best_combo;
} Save;
#define SAVE_MAGIC 0x52534801u

static Save sv;
static int state, state_t, frame_t, mode, scen, title_sel, sel_scen, story_i;
static uint32_t prev_in[2];
static bool bot_play;
static uint32_t bot_prev;
static int war_seed_n;
static int last_bphase;

typedef struct { float x, y, vx, vy; int life, col; } Part;
static Part parts[160];

static void part_add(float x, float y, float vx, float vy, int life, int col) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) { parts[i] = (Part){x, y, vx, vy, life, col}; return; }
}

static void burst(float x, float y, int col, int n, float sp) {
    for (int i = 0; i < n; i++) {
        float a = (float)i / (float)n * 6.283f + (float)(frame_t % 5) * 0.4f;
        part_add(x, y, cosf(a) * sp, sinf(a) * sp, 12 + i % 6, col);
    }
}

/* ------------------------------------------------------------------ */
/* save and goals                                                       */

static int cleared_count(void) {
    int n = 0;
    for (int i = 0; i < RSH_SCENARIOS; i++) n += sv.cleared[i] != 0;
    return n;
}

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void check_goals(void) {
    if (cleared_count() >= 5) game_award(GOAL_BEACON);                /* five wars won */
    if (cleared_count() >= RSH_SCENARIOS) game_award(GOAL_SAUCER);    /* all ten */
    if (sv.best_streak >= 3) game_award(GOAL_ALIEN);                  /* three in a row in the streak */
}

static void load_save(void) {
    Save tmp;
    memset(&sv, 0, sizeof sv);
    sv.magic = SAVE_MAGIC;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    check_goals();
}

static bool streak_open(void) { return cleared_count() >= RSH_SCENARIOS; }
static bool scen_open(int i) { return i == 0 || sv.cleared[i - 1] || sv.cleared[i]; }

/* ------------------------------------------------------------------ */
/* flow                                                                  */

static void pause_pick(int i);
static const char *const PAUSE_ITEMS[2] = {"RESTART WAR", "LEAVE WAR"};

static void to_title(void) {
    state = S_TITLE;
    state_t = 0;
    input_set_versus(false);
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    music_play(RSH_MUS_TITLE);
}

static void to_select(void) {
    state = S_SELECT;
    state_t = 0;
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    music_play(RSH_MUS_TITLE);
}

static int battle_song(void) {
    if (rw.scen == RSH_SCENARIOS - 1) return RSH_MUS_EMPRESS;
    if (rw.scen >= 0 && rw.scen < 5) return RSH_MUS_BATTLE;
    return RSH_MUS_BATTLE2;
}

static void start_war(void) {
    memset(parts, 0, sizeof parts);
    if (mode == MODE_CAMPAIGN) rsh_war_start(&RSH_SCEN[scen], scen, bot_play ? -1 : -1, RSH_SCEN[scen].cpu);
    else rsh_war_streak(0x5E000u + (uint32_t)(sv.streak * 977 + war_seed_n++ * 131 + frame_t), sv.streak, mode == MODE_VERSUS);
    input_set_versus(mode == MODE_VERSUS);
    state = S_WAR;
    state_t = 0;
    game_set_pausable(true);
    game_pause_items(2, PAUSE_ITEMS, pause_pick);
    music_play(RSH_MUS_MAP);
}

static void to_card(void) {
    state = S_CARD;
    state_t = 0;
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
}

static void war_lost_streak(void) {
    if (mode == MODE_STREAK && sv.streak) { sv.streak = 0; save_now(); }
}

static void pause_pick(int i) {
    if (state != S_WAR && state != S_BATTLE) return;
    input_set_versus(false);
    if (i == 0) { war_lost_streak(); start_war(); }
    else { war_lost_streak(); if (mode == MODE_CAMPAIGN) to_select(); else to_title(); }
}

static void to_battle(void) {
    state = S_BATTLE;
    state_t = 0;
    last_bphase = -1;
    sfx_play_name("rsh_clash");
    music_play(battle_song());
}

static void war_over(void) {
    state = S_WAROVER;
    state_t = 0;
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    input_set_versus(false);
    bool won = rw.winner == 0;
    if (mode == MODE_VERSUS) { music_play(RSH_MUS_WAR_WON); return; }
    if (won) {
        sv.wars_won++;
        if (mode == MODE_CAMPAIGN) sv.cleared[scen] = 1;
        else {
            sv.streak++;
            if (sv.streak > sv.best_streak) sv.best_streak = sv.streak;
        }
    } else {
        sv.wars_lost++;
        if (mode == MODE_STREAK) sv.streak = 0;
    }
    save_now();
    check_goals();
    music_play(won ? RSH_MUS_WAR_WON : RSH_MUS_WAR_LOST);
}

/* ------------------------------------------------------------------ */
/* input                                                                 */

static uint32_t pad_held(int p) { return p ? (input_held() >> BTN_P2_SHIFT) & 0xFF : input_held() & 0xFF; }

/* the buttons of whoever plays side s: player 1, player 2, or the demo */
static void side_input(int s, uint32_t *held, uint32_t *pressed, uint32_t *released) {
    int p = (mode == MODE_VERSUS && s == 1) ? 1 : 0;
    uint32_t h = pad_held(p);
    *held = h;
    *pressed = h & ~prev_in[p];
    *released = prev_in[p] & ~h;
}

static void war_update(void) {
    RshSide *me = &rw.s[rw.turn];
    uint32_t held, pr, rel;
    side_input(rw.turn, &held, &pr, &rel);
    (void)rel;
    switch (rw.state) {
    case W_PLAN:
        if (me->cpu >= 0) break;
        {
            static const int PADS[4] = {BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_UP};
            for (int d = 0; d < 4; d++)
                if ((pr & PADS[d]) && !rsh_try_step(d)) sfx_play_name("rsh_nope");
        }
        /* hold B to retreat all the way home (the turn ends) */
        if ((held & BTN_B) && !(me->nx == me->bx && me->ny == me->by)) {
            if (++rw.home_hold >= RSH_HOME_HOLD) { rw.home_hold = 0; sfx_play_name("ui_back"); rsh_go_home(rw.turn); }
        } else {
            if ((pr & BTN_B)) sfx_play_name("rsh_nope");
            rw.home_hold = 0;
        }
        break;
    case W_INN:
        if (pr & BTN_UP) { rw.menu_sel = (rw.menu_sel + 3) % 4; sfx_play_name("ui_move"); }
        if (pr & BTN_DOWN) { rw.menu_sel = (rw.menu_sel + 1) % 4; sfx_play_name("ui_move"); }
        if (pr & BTN_A) {
            if (rw.menu_sel == 3) { sfx_play_name("ui_back"); rsh_inn_leave(); }
            else {
                int k = rw.offer[rw.menu_sel], c = me->coins;
                rsh_inn_buy(rw.menu_sel);
                sfx_play_name(k < K_COUNT && (me->coins < c || rw.state == W_SWAP) ? "rsh_buy" : "rsh_nope");
            }
        } else if (pr & BTN_B) { sfx_play_name("ui_back"); rsh_inn_leave(); }
        break;
    case W_TOME:
        if (pr & BTN_UP) { rw.menu_sel = (rw.menu_sel + 2) % 3; sfx_play_name("ui_move"); }
        if (pr & BTN_DOWN) { rw.menu_sel = (rw.menu_sel + 1) % 3; sfx_play_name("ui_move"); }
        if (pr & BTN_A) { sfx_play_name("rsh_star"); rsh_tome_pick(rw.menu_sel); }
        break;
    case W_SWAP: {
        /* a full field army: which disk goes to the reserve (B: none, no sale) */
        int n = me->n_army;
        if (pr & BTN_UP) { rw.menu_sel = (rw.menu_sel + n - 1) % n; sfx_play_name("ui_move"); }
        if (pr & BTN_DOWN) { rw.menu_sel = (rw.menu_sel + 1) % n; sfx_play_name("ui_move"); }
        if (pr & BTN_A) { sfx_play_name("rsh_buy"); rsh_swap_pick(rw.menu_sel); }
        else if (pr & BTN_B) { sfx_play_name("ui_back"); rsh_swap_pick(-1); }
        break;
    }
    default: break;
    }
    rsh_war_update();
    if (rw.state == W_BATTLE) to_battle();
    else if (rw.state == W_OVER) war_over();
}

static void battle_events(void) {
    for (int i = 0; i < rb.p.n_ev; i++) {
        float x = (float)(rb.p.ev[i].x - rb.cam_x), y = (float)(rb.p.ev[i].y - rb.cam_y + RSH_VIEW_Y);
        switch (rb.p.ev[i].kind) {
        case BE_HIT: burst(x, y, C_WHITE, 6, 1.2f); sfx_play_name("rsh_hit"); break;
        case BE_KILL: burst(x, y, C_ORANGE, 14, 1.6f); burst(x, y, C_YELLOW, 8, 0.8f); sfx_play_name("rsh_kill"); break;
        case BE_SPLASH: burst(x, y, C_SKY, 14, 1.4f); burst(x, y, C_WHITE, 6, 0.7f); sfx_play_name("rsh_splash"); break;
        case BE_WALL: sfx_play_name("rsh_wall"); break;
        case BE_PICK: burst(x, y, C_YELLOW, 6, 0.9f); sfx_play_name("rsh_pick"); break;
        case BE_HEAL: burst(x, y, C_LIME, 8, 0.9f); sfx_play_name("rsh_heal"); break;
        case BE_STAR: burst(x, y, C_YELLOW, 10, 1.0f); sfx_play_name("rsh_star"); break;
        case BE_EMBER: burst(x, y, C_ORANGE, 8, 1.0f); sfx_play_name("rsh_ember"); break;
        case BE_STRIKE: burst(x, y, C_LIGHT, 5, 0.8f); sfx_play_name("rsh_pick"); break;
        case BE_POISON: burst(x, y, C_LIME, 6, 0.7f); sfx_play_name("rsh_poison"); break;
        case BE_STUN: sfx_play_name("rsh_stun"); break;
        }
    }
    rb.p.n_ev = 0;
}

static void battle_update(void) {
    int s = rb.side;
    uint32_t held = 0, pr = 0, rel = 0;
    if (rb.phase == B_OVER) {
        uint32_t h0 = pad_held(0), h1 = pad_held(1);
        bool go = ((h0 & ~prev_in[0]) & (BTN_A | BTN_START)) || (mode == MODE_VERSUS && ((h1 & ~prev_in[1]) & BTN_A));
        if (rb.over_t == 1) {
            bool brass = rb.winner == B_WIN0;
            if (brass) sv.battles_won++;
            music_play(rb.winner >= B_BOTH ? RSH_MUS_LOSE : (brass || mode == MODE_VERSUS) ? RSH_MUS_WIN : RSH_MUS_LOSE);
        }
        if (rb.p.best_combo > sv.best_combo) sv.best_combo = (uint16_t)rb.p.best_combo;
        rsh_battle_step(0, 0, 0);
        if (rb.over_t > 70 && go) {
            input_consume();
            rsh_battle_over(rb.winner);
            if (rw.state == W_BATTLE) to_battle();
            else if (rw.state == W_OVER) war_over();
            else { state = S_WAR; state_t = 0; music_play(RSH_MUS_MAP); }
        }
        return;
    }
    if (rb.cpu[s] >= 0) {
        static uint32_t cprev;
        held = rsh_battle_buttons(s, 0, cprev);
        pr = held & ~cprev;
        rel = cprev & ~held;
        cprev = held;
    } else {
        side_input(s, &held, &pr, &rel);
    }
    int pips = rb.pips;
    rsh_battle_step(held, pr, rel);
    if (rb.charging && rb.pips > pips) sfx_play_name("rsh_pip");
    if (rb.phase != last_bphase) {
        if (rb.phase == B_SELECT) sfx_play_name("rsh_turn");
        if (rb.phase == B_SELECT && rb.round >= RSH_FOG_ROUND && rb.side == 0) sfx_play_name("rsh_fog");
        last_bphase = rb.phase;
    }
    battle_events();
}

/* ------------------------------------------------------------------ */
/* the demo player (tests): the same planners as the computer, pressing
 * real buttons */

static int8_t bot_px[4], bot_py[4];
static int bot_n = -2, bot_turn_key = -1;

static uint32_t map_bot(void) {
    if (rw.turn != 0 || rw.s[0].cpu >= 0) return 0;
    bool odd = frame_t & 1;
    switch (rw.state) {
    case W_PLAN: {
        /* the demo player picks its way a step at a time */
        int key = rw.turns * 8 + rw.moves_used;
        if (bot_turn_key != key) {
            bot_turn_key = key;
            bot_n = rsh_cpu_map_plan(0, bot_px, bot_py);
        }
        if (bot_n <= 0) return BTN_B;
        if (odd) return 0;
        static const int PADS[4] = {BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_UP};
        for (int d = 0; d < 4; d++)
            if (rw.s[0].nx + RSH_DX[d] == bot_px[0] && rw.s[0].ny + RSH_DY[d] == bot_py[0]) return PADS[d];
        return BTN_B;
    }
    case W_SWAP: {
        if (odd) return 0;
        int weak = 0;
        for (int i = 1; i < rw.s[0].n_army; i++)
            if (rsh_kind_value(rw.s[0].army[i]) < rsh_kind_value(rw.s[0].army[weak])) weak = i;
        if (rsh_kind_value(rw.s[0].army[weak]) >= rsh_kind_value(rw.swap_kind)) return BTN_B;
        return rw.menu_sel != weak ? BTN_DOWN : BTN_A;
    }
    case W_INN: {
        if (odd) return 0;
        int best = 3, bv = 0;
        bool room = rw.s[0].n_army < RSH_ARMY || rw.s[0].n_reserve < RSH_RESERVE;
        int weakest = 999;
        for (int i = 0; i < rw.s[0].n_army; i++) weakest = imin(weakest, rsh_kind_value(rw.s[0].army[i]));
        for (int i = 0; i < 3 && room; i++) {
            int k = rw.offer[i];
            if (k >= K_COUNT || RSH_KIND[k].cost > rw.s[0].coins) continue;
            if (rw.s[0].n_army >= RSH_ARMY && rsh_kind_value(k) <= weakest) continue;
            if (rsh_kind_value(k) > bv) { bv = rsh_kind_value(k); best = i; }
        }
        if (rw.menu_sel != best) return BTN_DOWN;
        return BTN_A;
    }
    case W_TOME: {
        if (odd) return 0;
        static const int PREF[RSH_SKILLS] = {2, 0, 7, 5, 1, 4, 3, 6};
        int want = 0;
        for (int p = RSH_SKILLS - 1; p >= 0; p--)
            for (int i = 0; i < 3; i++)
                if (rw.tome[i] == PREF[p]) want = i;
        if (rw.menu_sel != want) return BTN_DOWN;
        return BTN_A;
    }
    default: return 0;
    }
}

static uint32_t bot_buttons(void) {
    bool odd = frame_t & 1;
    switch (state) {
    case S_TITLE:
        if (odd || state_t < 12) return 0;
        return title_sel == 0 ? BTN_A : BTN_UP;
    case S_STORY: case S_CARD: case S_END: case S_WAROVER:
        return (!odd && state_t > 80) ? BTN_A : 0;
    case S_SELECT: {
        if (odd || state_t < 12) return 0;
        int want = 0;
        while (want < RSH_SCENARIOS - 1 && sv.cleared[want]) want++;
        if (sel_scen < want) return BTN_DOWN;
        if (sel_scen > want) return BTN_UP;
        return BTN_A;
    }
    case S_WAR: return map_bot();
    case S_BATTLE:
        if (rb.phase == B_OVER) return (!odd && rb.over_t > 80) ? BTN_A : 0;
        if (rb.side == 0 && rb.cpu[0] < 0) {
            uint32_t b = rsh_battle_buttons(0, 1, bot_prev);
            bot_prev = b;
            return b;
        }
        bot_prev = 0;
        return 0;
    }
    return 0;
}

/* ------------------------------------------------------------------ */

static void rsh_update(void) {
    state_t++;
    frame_t++;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vx *= 0.9f;
        p->vy = p->vy * 0.9f + 0.03f;
    }
    uint32_t h0 = pad_held(0), p0 = h0 & ~prev_in[0];
    switch (state) {
    case S_TITLE:
        game_set_pausable(false);
        if (state_t > 10) {
            if (p0 & BTN_UP) { title_sel = (title_sel + 2) % 3; sfx_play_name("ui_move"); }
            if (p0 & BTN_DOWN) { title_sel = (title_sel + 1) % 3; sfx_play_name("ui_move"); }
            if (p0 & (BTN_A | BTN_START)) {
                if (title_sel == MODE_STREAK && !streak_open()) { sfx_play_name("ui_error"); break; }
                sfx_play_name("ui_ok");
                mode = title_sel;
                if (mode == MODE_CAMPAIGN) {
                    if (!sv.story_seen) { state = S_STORY; state_t = 0; story_i = 0; }
                    else to_select();
                } else to_card();
            } else if (p0 & BTN_B) game_exit_to_library();
        }
        break;
    case S_STORY:
        if (state_t > 20 && (p0 & (BTN_A | BTN_START))) {
            state_t = 0;
            if (++story_i >= 3) { sv.story_seen = 1; save_now(); to_select(); }
        }
        break;
    case S_SELECT:
        if (state_t > 8) {
            if (p0 & BTN_UP) { sel_scen = (sel_scen + RSH_SCENARIOS - 1) % RSH_SCENARIOS; sfx_play_name("ui_move"); }
            if (p0 & BTN_DOWN) { sel_scen = (sel_scen + 1) % RSH_SCENARIOS; sfx_play_name("ui_move"); }
            if (p0 & (BTN_A | BTN_START)) {
                if (!scen_open(sel_scen)) sfx_play_name("ui_error");
                else { sfx_play_name("ui_ok"); scen = sel_scen; to_card(); }
            } else if (p0 & BTN_B) { sfx_play_name("ui_back"); to_title(); }
        }
        break;
    case S_CARD:
        if (state_t > 20 && (p0 & (BTN_A | BTN_START))) { sfx_play_name("ui_ok"); start_war(); }
        else if (state_t > 20 && (p0 & BTN_B)) { if (mode == MODE_CAMPAIGN) to_select(); else to_title(); }
        break;
    case S_WAR: war_update(); break;
    case S_BATTLE: battle_update(); break;
    case S_WAROVER:
        if (state_t > 90 && (p0 & (BTN_A | BTN_START))) {
            input_consume();
            if (mode == MODE_CAMPAIGN && rw.winner == 0 && scen == RSH_SCENARIOS - 1) { state = S_END; state_t = 0; music_play(RSH_MUS_END); }
            else if (mode == MODE_STREAK && rw.winner == 0) to_card();
            else if (mode == MODE_CAMPAIGN) to_select();
            else to_title();
        }
        break;
    case S_END:
        if (state_t > 300 && (p0 & (BTN_A | BTN_START))) to_title();
        break;
    }
    prev_in[0] = pad_held(0);
    prev_in[1] = pad_held(1);
}

/* ------------------------------------------------------------------ */
/* drawing: ground                                                        */

static uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)(x * 374761393) ^ (uint32_t)(y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

/* a patch of ground; wx, wy are world coordinates for the pattern */
static void ground(int x, int y, int w, int h, int t, int wx, int wy) {
    switch (t) {
    case T_STONE:
        gfx_rect(x, y, w, h, C_GREY);
        for (int yy = 0; yy < h; yy++)
            for (int xx = 0; xx < w; xx++) {
                int gx = wx + xx, gy = wy + yy;
                if (gy % 8 == 0 || (gx + (gy / 8) * 4) % 8 == 0) gfx_pset(x + xx, y + yy, C_SLATE);
                else if ((hash2(gx, gy) & 31) == 0) gfx_pset(x + xx, y + yy, C_LIGHT);
            }
        break;
    case T_SAND:
        gfx_rect(x, y, w, h, C_TAN);
        for (int yy = 0; yy < h; yy++)
            for (int xx = 0; xx < w; xx++) {
                uint32_t hh = hash2(wx + xx, wy + yy) & 15;
                if (hh == 0) gfx_pset(x + xx, y + yy, C_HIDE);
                else if (hh == 1) gfx_pset(x + xx, y + yy, C_CREAM);
            }
        break;
    case T_WATER:
        gfx_rect(x, y, w, h, C_BLUE);
        for (int yy = 0; yy < h; yy++)
            for (int xx = 0; xx < w; xx++) {
                int gx = wx + xx, gy = wy + yy;
                if (gy % 6 == 0 && ((gx + frame_t / 8 + gy) % 12) < 4) gfx_pset(x + xx, y + yy, C_SKY);
                else if ((hash2(gx, gy) & 63) == 0) gfx_pset(x + xx, y + yy, C_NAVY);
            }
        break;
    default:
        gfx_rect(x, y, w, h, C_LEAF);
        for (int yy = 0; yy < h; yy++)
            for (int xx = 0; xx < w; xx++) {
                uint32_t hh = hash2(wx + xx, wy + yy) & 15;
                if (hh == 0) gfx_pset(x + xx, y + yy, C_JADE);
                else if (hh == 1 && ((wx + xx) & 1)) gfx_pset(x + xx, y + yy, C_LIME);
            }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing: the board                                                      */

static int map_ox(void) { return 10 + (RSH_MW - rw.w) * 10; }
static int map_oy(void) { return 22 + (RSH_MH - rw.h) * 10; }
static int nodex(int x) { return map_ox() + x * RSH_NODE_GAP; }
static int nodey(int y) { return map_oy() + y * RSH_NODE_GAP; }

static const char *const CODE[K_COUNT] = {"SQ", "WA", "FE", "BR", "SL", "TO", "OO", "HE", "ME", "AD", "FR", "PI", "LE", "DE", "WY", "EM"};
static const char *const SIDE_NAME[2] = {"BRASS", "PLUM"};

static void draw_token(int side, int x, int y, int bob) {
    uint8_t m[PAL_COUNT];
    pal_identity(m);
    m[C_RED] = RSH_SIDE_COL[side][0];
    m[C_WHITE] = RSH_SIDE_COL[side][1];
    spr_draw_ex(&rsh_spr[RS_TOKEN0 + side], x - 1, y - 13 - bob, side ? SPR_FLIPX : 0, m, -1);
}

static void draw_road(int x0, int y0, int x1, int y1, int col, int edge) {
    int ax = imin(x0, x1), ay = imin(y0, y1), bx = imax(x0, x1), by = imax(y0, y1);
    gfx_rect(ax - 2, ay - 2, bx - ax + 5, by - ay + 5, edge);
    gfx_rect(ax - 1, ay - 1, bx - ax + 3, by - ay + 3, col);
}

static void draw_board(void) {
    int ox = map_ox(), oy = map_oy();
    int bt = rw.border == '~' ? T_WATER : rw.border == ':' ? T_STONE : rw.border == 's' ? T_SAND : T_GRASS;
    ground(0, 15, SCREEN_W, 152, bt, 0, 15);
    for (int ty = 0; ty <= rw.h; ty++)
        for (int tx = 0; tx <= rw.w; tx++) {
            int x = ox + (tx - 1) * RSH_NODE_GAP, y = oy + (ty - 1) * RSH_NODE_GAP;
            int x0 = imax(x, 0), y0 = imax(y, 15), x1 = imin(x + RSH_NODE_GAP, SCREEN_W), y1 = imin(y + RSH_NODE_GAP, 167);
            if (x1 > x0 && y1 > y0) ground(x0, y0, x1 - x0, y1 - y0, rw.tile[ty][tx], x0, y0);
            /* woods get trees, crags a peak, dunes a ridge */
            uint32_t h = hash2(tx, ty);
            int dx = x + 5 + (int)(h % 6), dy = y + 6 + (int)((h >> 4) % 5);
            switch (rw.tile[ty][tx]) {
            case T_GRASS:
                for (int k = 0; k < 2; k++) {
                    int px = dx + k * 7, py = dy + (k ? 3 : 0);
                    gfx_rect(px, py, 5, 4, C_FOREST);
                    gfx_rect(px + 1, py - 1, 3, 1, C_JADE);
                    gfx_pset(px + 2, py + 4, C_BROWN);
                }
                break;
            case T_STONE:
                for (int k = 0; k < 5; k++) gfx_hline(dx + 4 - k, dx + 4 + k, dy + k, k < 2 ? C_WHITE : C_SLATE);
                gfx_hline(dx - 1, dx + 9, dy + 5, C_DUSK);
                break;
            case T_SAND:
                gfx_hline(dx, dx + 6, dy + 4, C_HIDE);
                gfx_hline(dx + 2, dx + 5, dy + 3, C_CREAM);
                break;
            }
        }
    /* roads */
    for (int y = 0; y < rw.h; y++)
        for (int x = 0; x < rw.w; x++) {
            if (rw.er[y][x]) draw_road(nodex(x), nodey(y), nodex(x + 1), nodey(y), C_TAN, C_EARTH);
            if (rw.ed[y][x]) draw_road(nodex(x), nodey(y), nodex(x), nodey(y + 1), C_TAN, C_EARTH);
        }
    /* the two routes */
    for (int s = 0; s < 2; s++) {
        const RshSide *sd = &rw.s[s];
        for (int i = 1; i < sd->trail_n; i++)
            draw_road(nodex(sd->tx[i - 1]), nodey(sd->ty[i - 1]), nodex(sd->tx[i]), nodey(sd->ty[i]), RSH_SIDE_COL[s][0], RSH_SIDE_COL[s][2]);
    }
    /* the ways the banner may go now: arrows, red where a battle waits */
    if (rw.state == W_PLAN && rw.s[rw.turn].cpu < 0) {
        int fx = nodex(rw.s[rw.turn].nx), fy = nodey(rw.s[rw.turn].ny);
        for (int d = 0; d < 4; d++) {
            int k = rsh_step_kind(d);
            if (!k) continue;
            int off = 8 + (frame_t / 8) % 2;
            int ax = fx + RSH_DX[d] * off, ay = fy + RSH_DY[d] * off, col = k == 2 ? C_RED : k == 3 ? C_SKY : C_WHITE;
            gfx_rect(ax - 1, ay - 1, 3, 3, C_INK);
            gfx_pset(ax, ay, col);
            gfx_pset(ax + RSH_DX[d], ay + RSH_DY[d], col);
        }
    }
    /* nodes and what is on them */
    for (int y = 0; y < rw.h; y++)
        for (int x = 0; x < rw.w; x++) {
            int n = rw.node[y][x];
            if (n == N_NONE) continue;
            int px = nodex(x), py = nodey(y);
            const Sprite *s = NULL;
            uint8_t m[PAL_COUNT];
            pal_identity(m);
            switch (n) {
            case N_PLAIN: gfx_circ(px, py, 2, C_EARTH); gfx_pset(px, py, C_CREAM); break;
            case N_BASE0: case N_BASE1: {
                int side = n == N_BASE1;
                s = &rsh_spr[rw.s[side].castle ? RS_CASTLE : RS_BASE];
                m[C_RED] = RSH_SIDE_COL[side][0];
                m[C_GREY] = RSH_SIDE_COL[side][1];
                break;
            }
            case N_INN: s = &rsh_spr[RS_INN]; break;
            case N_TOME: s = &rsh_spr[RS_TOME]; break;
            case N_SEAM: s = &rsh_spr[RS_SEAM]; break;
            case N_CHEST: s = &rsh_spr[RS_CHEST]; break;
            }
            if (s && s->px) spr_draw_ex(s, px - s->w / 2, py - s->h / 2, 0, m, -1);
            if (n == N_SEAM && rw.owner[y][x]) gfx_rect(px + 3, py - 6, 3, 3, RSH_SIDE_COL[rw.owner[y][x] - 1][0]);
            if (n == N_INN) {
                char b[12];
                snprintf(b, sizeof b, "%d", 3 - rw.visits[y][x]);
                tiny_draw(b, px + 5, py - 7, C_WHITE);
            }
        }
    /* the banners */
    for (int s = 0; s < 2; s++) {
        const RshSide *sd = &rw.s[s];
        int px = nodex(sd->nx), py = nodey(sd->ny);
        if (rw.state == W_WALK && rw.turn == s && rw.walk_i < rw.plan_n) {
            int tx = nodex(rw.plan_x[rw.walk_i]), ty = nodey(rw.plan_y[rw.walk_i]);
            px += (tx - px) * rw.walk_t / 10;
            py += (ty - py) * rw.walk_t / 10;
        }
        int bob = rw.turn == s && rw.state != W_WALK ? (frame_t / 12) % 2 : 0;
        draw_token(s, px + (s ? 2 : -2), py, bob);
    }
}

static void army_codes(const RshSide *s, int x, int y, int right) {
    char buf[96];
    int n = 0;
    buf[0] = 0;
    for (int i = 0; i < s->n_army; i++) n += snprintf(buf + n, sizeof buf - (size_t)n, "%s ", CODE[s->army[i]]);
    if (s->n_reserve) n += snprintf(buf + n, sizeof buf - (size_t)n, "+ ");
    for (int i = 0; i < s->n_reserve; i++) n += snprintf(buf + n, sizeof buf - (size_t)n, "%s ", CODE[s->reserve[i]]);
    if (s->n_army + s->n_reserve == 0) snprintf(buf, sizeof buf, "NO DISKS");
    tiny_draw(buf, right ? x - tiny_width(buf) : x, y, C_LIGHT);
}

static void draw_war_hud(void) {
    gfx_rect(0, 0, SCREEN_W, 15, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 15, C_DUSK);
    char buf[64];
    for (int s = 0; s < 2; s++) {
        const RshSide *sd = &rw.s[s];
        snprintf(buf, sizeof buf, "%s " GLYPH_COIN "%d", SIDE_NAME[s], sd->coins);
        int w = tiny_width(buf);
        tiny_draw(buf, s ? SCREEN_W - 3 - w : 3, 2, RSH_SIDE_COL[s][0]);
        army_codes(sd, s ? SCREEN_W - 3 : 3, 8, s);
        /* skills as little lit squares */
        for (int k = 0; k < RSH_SKILLS; k++) {
            int x = s ? SCREEN_W - 3 - w - 6 - k * 4 : 3 + w + 3 + k * 4;
            gfx_rect(x, 3, 3, 3, (sd->skills >> k) & 1 ? C_YELLOW : C_DUSK);
        }
    }
    const char *who = rw.s[rw.turn].cpu >= 0 ? "PLUM IS MOVING" : mode == MODE_VERSUS ? (rw.turn ? "PLUM'S TURN" : "BRASS'S TURN") : "YOUR TURN";
    snprintf(buf, sizeof buf, "%s " GLYPH_DOT " %d MOVE%s", who, rw.moves - rw.moves_used, rw.moves - rw.moves_used == 1 ? "" : "S");
    tiny_center(buf, SCREEN_W / 2, 2, C_WHITE);
    gfx_rect(0, 167, SCREEN_W, 13, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 166, C_DUSK);
    if (rw.state == W_PLAN && rw.s[rw.turn].cpu < 0) {
        int fx;
        if (rsh_can_step()) {
            snprintf(buf, sizeof buf, "MOVE (%d LEFT)", rw.moves - rw.moves_used);
            fx = ui_hint(4, 170, GLYPH_DPAD, buf, C_LIGHT);
        } else fx = text_draw("NOWHERE TO GO:", 4, 170, C_ORANGE) + 4;
        ui_hint(fx, 170, GLYPH_B, "HOLD: HOME", C_LIGHT);
    } else if (rw.state == W_SWAP) {
        int fx = ui_hint(4, 170, GLYPH_UP GLYPH_DOWN, "CHOOSE", C_LIGHT);
        fx = ui_hint(fx, 170, GLYPH_A, "TO THE RESERVE", C_LIGHT);
        ui_hint(fx, 170, GLYPH_B, "DON'T HIRE", C_LIGHT);
    } else if (rw.state == W_INN || rw.state == W_TOME) {
        int fx = ui_hint(4, 170, GLYPH_UP GLYPH_DOWN, "CHOOSE", C_LIGHT);
        ui_hint(fx, 170, GLYPH_A, rw.state == W_INN ? "BUY" : "LEARN", C_LIGHT);
    } else {
        snprintf(buf, sizeof buf, "%s " GLYPH_DOT " TURN %d", mode == MODE_CAMPAIGN ? RSH_SCEN[scen].name : mode == MODE_STREAK ? "STREAK" : "TWO PLAYERS", rw.turns / 2 + 1);
        text_draw(buf, 4, 170, C_GREY);
    }
}

static void draw_inn(void) {
    const RshSide *s = &rw.s[rw.turn];
    int x = 50, y = 30, w = 220, h = 118;
    ui_panel(x, y, w, h, C_NIGHT, C_AMBER);
    int visits = rw.visits[s->ny][s->nx];
    char buf[80];
    static const char *const VISIT[3] = {"THE INN: HIRE WHO YOU LIKE", "THE INN: TWO VISITS LEFT", "THE INN: ABOUT TO CLOSE"};
    snprintf(buf, sizeof buf, "%s", VISIT[iclamp(visits - 1, 0, 2)]);
    text_draw(buf, x + 8, y + 5, C_YELLOW);
    snprintf(buf, sizeof buf, GLYPH_COIN "%d " GLYPH_DOT " ROOM FOR %d", s->coins, RSH_ARMY + RSH_RESERVE - rsh_army_size(rw.turn));
    tiny_draw(buf, x + w - 8 - tiny_width(buf), y + 7, C_LIGHT);
    for (int i = 0; i < 3; i++) {
        int ry = y + 20 + i * 26, k = rw.offer[i];
        bool sel = rw.menu_sel == i;
        if (sel) gfx_rect(x + 4, ry - 2, w - 8, 24, C_DUSK);
        if (k >= K_COUNT) continue;
        rsh_draw_disk_icon(k, rw.turn, x + 18, ry + 10, frame_t);
        const RshKind *kd = &RSH_KIND[k];
        text_draw(kd->name, x + 34, ry, sel ? C_WHITE : C_LIGHT);
        snprintf(buf, sizeof buf, GLYPH_COIN "%d", kd->cost);
        text_draw(buf, x + w - 10 - text_width(buf), ry, kd->cost <= s->coins ? C_YELLOW : C_RED);
        snprintf(buf, sizeof buf, "HP %d  HIT %d x%d%s%s", kd->hp, kd->melee, kd->moves, kd->rkind ? "  +SHOT" : "", kd->aqua ? "  SWIMS" : "");
        tiny_draw(buf, x + 34, ry + 9, C_SKY);
        tiny_draw(kd->blurb, x + 34, ry + 15, C_GREY);
    }
    int ly = y + 20 + 3 * 26;
    if (rw.menu_sel == 3) gfx_rect(x + 4, ly - 2, w - 8, 11, C_DUSK);
    text_draw("LEAVE", x + 34, ly, rw.menu_sel == 3 ? C_WHITE : C_GREY);
}

static void draw_swap(void) {
    const RshSide *s = &rw.s[rw.turn];
    int x = 60, y = 24, w = 200, h = 22 + s->n_army * 13;
    ui_panel(x, y, w, h, C_NIGHT, C_AMBER);
    char buf[64];
    snprintf(buf, sizeof buf, "ROOM FOR THE %s: WHO RESTS?", RSH_KIND[rw.swap_kind].name);
    tiny_draw(buf, x + 8, y + 5, C_YELLOW);
    for (int i = 0; i < s->n_army; i++) {
        int ry = y + 16 + i * 13;
        if (rw.menu_sel == i) gfx_rect(x + 4, ry - 2, w - 8, 12, C_DUSK);
        text_draw(RSH_KIND[s->army[i]].name, x + 14, ry, rw.menu_sel == i ? C_WHITE : C_LIGHT);
        snprintf(buf, sizeof buf, "HP %d  HIT %d", RSH_KIND[s->army[i]].hp, RSH_KIND[s->army[i]].melee);
        tiny_draw(buf, x + w - 12 - tiny_width(buf), ry + 1, C_SKY);
    }
}

static void draw_tome(void) {
    int x = 40, y = 34, w = 240, h = 100;
    ui_panel(x, y, w, h, C_NIGHT, C_VIOLET);
    text_draw("AN OLD TOME " GLYPH_DOT " LEARN ONE", x + 8, y + 5, C_PINK);
    for (int i = 0; i < 3; i++) {
        int ry = y + 22 + i * 24, k = rw.tome[i];
        bool sel = rw.menu_sel == i;
        if (sel) gfx_rect(x + 4, ry - 3, w - 8, 22, C_DUSK);
        if (k >= RSH_SKILLS) { text_draw("(BLANK PAGE)", x + 14, ry, C_SLATE); continue; }
        text_draw(RSH_SKILL_NAME[k], x + 14, ry, sel ? C_WHITE : C_LIGHT);
        tiny_draw(RSH_SKILL_TEXT[k], x + 14, ry + 10, C_GREY);
    }
}

static void draw_war(void) {
    draw_board();
    draw_war_hud();
    if (rw.msg_t > 0 && rw.state != W_INN && rw.state != W_TOME && rw.state != W_SWAP) {
        int w = tiny_width(rw.msg) + 12;
        ui_panel(SCREEN_W - w - 4, 19, w, 13, C_NIGHT, C_YELLOW);
        tiny_draw(rw.msg, SCREEN_W - w + 2, 23, C_WHITE);
    }
    if (rw.home_hold > 0) {
        ui_panel(110, 150, 100, 14, C_NIGHT, C_LIGHT);
        gfx_rect(114, 159, 92 * rw.home_hold / RSH_HOME_HOLD, 3, RSH_SIDE_COL[rw.turn][0]);
        tiny_center("RETREATING HOME...", 160, 152, C_WHITE);
    }
    if (rw.state == W_INN) draw_inn();
    if (rw.state == W_TOME) draw_tome();
    if (rw.state == W_SWAP) draw_swap();
    if (state_t < 90 && rw.turns == 0) {
        ui_panel(80, 18, 160, 22, C_NIGHT, C_YELLOW);
        text_center(mode == MODE_CAMPAIGN ? RSH_SCEN[scen].name : mode == MODE_STREAK ? "A WAR OF THE STREAK" : "TWO BANNERS", 160, 21, C_WHITE);
        tiny_center(rw.turn == 0 ? "BRASS MOVES FIRST" : "PLUM MOVES FIRST", 160, 31, C_LIGHT);
    }
}

/* ------------------------------------------------------------------ */
/* drawing: the battle                                                     */

static void draw_field(void) {
    int cx = rb.cam_x, cy = rb.cam_y;
    gfx_clip(0, RSH_VIEW_Y, SCREEN_W, RSH_VIEW_H);
    for (int y = cy / RSH_CELL; y <= (cy + RSH_VIEW_H) / RSH_CELL && y < RSH_CH; y++)
        for (int x = cx / RSH_CELL; x <= (cx + SCREEN_W) / RSH_CELL && x < RSH_CW; x++)
            ground(x * RSH_CELL - cx, y * RSH_CELL - cy + RSH_VIEW_Y, RSH_CELL, RSH_CELL, rb.cell[y][x], x * RSH_CELL, y * RSH_CELL);
    /* shores */
    for (int y = cy / RSH_CELL; y <= (cy + RSH_VIEW_H) / RSH_CELL && y < RSH_CH; y++)
        for (int x = cx / RSH_CELL; x <= (cx + SCREEN_W) / RSH_CELL && x < RSH_CW; x++) {
            if (rb.cell[y][x] != T_WATER) continue;
            int sx = x * RSH_CELL - cx, sy = y * RSH_CELL - cy + RSH_VIEW_Y;
            if (y > 0 && rb.cell[y - 1][x] != T_WATER) gfx_hline(sx, sx + 7, sy, C_CYAN);
            if (y < RSH_CH - 1 && rb.cell[y + 1][x] != T_WATER) gfx_hline(sx, sx + 7, sy + 7, C_NAVY);
        }
    /* the cliffs' faces */
    for (int w = 0; w < rb.nwall; w++) {
        const float *wl = rb.wall[w];
        gfx_line((int)wl[0] - cx, (int)wl[1] - cy + RSH_VIEW_Y, (int)wl[2] - cx, (int)wl[3] - cy + RSH_VIEW_Y, C_INK);
    }
    /* the walls */
    gfx_rectb(-cx - 1, -cy - 1 + RSH_VIEW_Y, RSH_AW + 2, RSH_AH + 2, C_BROWN);
    gfx_rectb(-cx - 2, -cy - 2 + RSH_VIEW_Y, RSH_AW + 4, RSH_AH + 4, C_INK);
    /* things on the field */
    for (int k = 0; k < RSH_MAXO; k++) {
        const RshObj *o = &rb.p.o[k];
        if (!o->on) continue;
        int id = RS_COIN;
        switch (o->kind) {
        case O_SHARD: id = o->left > 1 ? RS_SHARD2 : RS_SHARD; break;
        case O_TONIC: id = RS_TONIC; break;
        case O_WELL: id = RS_WELL; break;
        case O_PILE: id = RS_PILE; break;
        case O_CLUSTER: id = RS_CLUSTER; break;
        case O_EMBER: id = (frame_t / 6 + k) % 2 ? RS_EMBER0 : RS_EMBER1; break;
        case O_TREE: id = RS_TREE; break;
        }
        const Sprite *s = &rsh_spr[id];
        int sx = (int)o->x - cx, sy = (int)o->y - cy + RSH_VIEW_Y;
        if (s->px) spr_draw(s, sx - s->w / 2, sy - s->h / 2, 0);
        if (o->kind == O_WELL || o->kind == O_PILE || o->kind == O_CLUSTER)
            for (int i = 0; i < o->left; i++) gfx_rect(sx - 7 + i * 3, sy + 8, 2, 2, C_WHITE);
    }
    /* the cursor ring and the aim */
    int el[5], n = rsh_eligible(rb.side, el);
    if (rb.phase == B_SELECT) {
        for (int i = 0; i < n; i++) {
            const RshDisk *d = &rb.p.d[el[i]];
            int r = (int)d->r + 3 + (i == rb.cursor ? (frame_t / 6) % 2 : 0);
            gfx_circb((int)d->x - cx, (int)d->y - cy + RSH_VIEW_Y, r, i == rb.cursor ? C_WHITE : C_DUSK);
        }
    }
    for (int i = 0; i < RSH_MAXD; i++) {
        const RshDisk *d = &rb.p.d[i];
        if (!d->on) continue;
        rsh_draw_disk(d, (int)d->x - cx, (int)d->y - cy + RSH_VIEW_Y, frame_t);
    }
    if ((rb.phase == B_AIM || rb.phase == B_RAIM) && rb.sel >= 0) {
        const RshDisk *d = &rb.p.d[rb.sel];
        int16_t xs[160], ys[160];
        uint8_t ks[160];
        int nd = rsh_count_trace(rb.angle, rb.pips, xs, ys, ks, 160);
        int col = rb.phase == B_RAIM ? (rb.no_room ? C_RED : C_CYAN) : C_WHITE;
        for (int i = 0; i < nd; i++) {
            int px = xs[i] - cx, py = ys[i] - cy + RSH_VIEW_Y;
            gfx_rect(px, py, 2, 2, C_INK);
            gfx_pset(px, py, ks[i] == 1 ? C_ORANGE : ks[i] == 2 ? C_SKY : col);
        }
        /* the reticle, just off the disk's edge */
        float a = (float)rb.angle * (6.2831853f / 256.0f);
        int rx = (int)(d->x + cosf(a) * (d->r + 4)) - cx, ry = (int)(d->y + sinf(a) * (d->r + 4)) - cy + RSH_VIEW_Y;
        gfx_circb(rx, ry, 2, col);
        gfx_circb((int)d->x - cx, (int)d->y - cy + RSH_VIEW_Y, (int)d->r + 2, (frame_t / 8) % 2 ? C_YELLOW : C_WHITE);
        /* the power pips */
        int max = rb.phase == B_RAIM ? RSH_KIND[d->kind].pcharge : rb.stun_pick ? imin(2, RSH_KIND[d->kind].charge) : RSH_KIND[d->kind].charge;
        int bx = (int)d->x - cx - max * 3, by = (int)d->y - cy + RSH_VIEW_Y - (int)d->r - 9;
        for (int p = 0; p < max; p++) {
            gfx_rect(bx + p * 6, by, 5, 4, C_INK);
            gfx_rect(bx + p * 6 + 1, by + 1, 3, 2, p < rb.pips ? (p >= max - 1 ? C_RED : C_YELLOW) : C_DUSK);
        }
    }
    /* the haze */
    if (rb.fog > 0) {
        int x0, y0, x1, y1;
        rsh_fog_rect(&x0, &y0, &x1, &y1);
        int lvl = 7 + (frame_t / 20) % 2;
        gfx_dither(-cx, -cy + RSH_VIEW_Y, RSH_AW, y0, C_VIOLET, lvl);
        gfx_dither(-cx, y1 - cy + RSH_VIEW_Y, RSH_AW, RSH_AH - y1, C_VIOLET, lvl);
        gfx_dither(-cx, y0 - cy + RSH_VIEW_Y, x0, y1 - y0, C_VIOLET, lvl);
        gfx_dither(x1 - cx, y0 - cy + RSH_VIEW_Y, RSH_AW - x1, y1 - y0, C_VIOLET, lvl);
        gfx_rectb(x0 - cx, y0 - cy + RSH_VIEW_Y, x1 - x0, y1 - y0, C_MAGENTA);
    }
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life > 0) gfx_pset((int)parts[i].x, (int)parts[i].y, parts[i].col);
    gfx_noclip();
}

static void draw_battle_hud(void) {
    gfx_rect(0, 0, SCREEN_W, RSH_VIEW_Y, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, RSH_VIEW_Y - 1, C_DUSK);
    char buf[64];
    for (int s = 0; s < 2; s++) {
        snprintf(buf, sizeof buf, "%s %d " GLYPH_DOT " SHARDS %d", SIDE_NAME[s], rsh_disks_left(s), rb.p.shards[s]);
        int w = tiny_width(buf);
        tiny_draw(buf, s ? SCREEN_W - 3 - w : 3, 2, RSH_SIDE_COL[s][0]);
    }
    /* the queue of the side to move: the choosable ones lit */
    int s = rb.side, el[5], n = rsh_eligible(s, el), x = 3;
    for (int q = 0; q < rb.qn[s]; q++) {
        const RshDisk *d = &rb.p.d[rb.queue[s][q]];
        if (!d->on) continue;
        int k = -1;
        for (int i = 0; i < n; i++)
            if (el[i] == rb.queue[s][q]) k = i;
        int col = k >= 0 ? (rb.phase == B_SELECT && k == rb.cursor ? C_WHITE : RSH_SIDE_COL[s][0]) : C_SLATE;
        if (rb.sel == rb.queue[s][q]) col = C_YELLOW;
        tiny_draw(CODE[d->kind], x, 9, col);
        x += 11;
    }
    if (rb.fog > 0 || rb.round >= RSH_FOG_ROUND - 1) {
        snprintf(buf, sizeof buf, "ROUND %d%s", rb.round + 1, rb.fog > 0 ? " " GLYPH_DOT " HAZE" : "");
        tiny_draw(buf, SCREEN_W - 3 - tiny_width(buf), 9, rb.fog > 0 ? C_MAGENTA : C_GREY);
    } else {
        snprintf(buf, sizeof buf, "ROUND %d", rb.round + 1);
        tiny_draw(buf, SCREEN_W - 3 - tiny_width(buf), 9, C_GREY);
    }
    /* the chosen disk */
    if (rb.sel >= 0 && rb.p.d[rb.sel].on && rb.phase != B_OVER) {
        const RshDisk *d = &rb.p.d[rb.sel];
        const RshKind *k = &RSH_KIND[d->kind];
        if (rb.phase == B_RAIM) snprintf(buf, sizeof buf, "%s " GLYPH_DOT " SHOT (TAP " GLYPH_A " TO SKIP)", k->name);
        else snprintf(buf, sizeof buf, "%s " GLYPH_DOT " MOVE %d OF %d", k->name, imin(rb.move_i + 1, rb.moves), rb.moves);
        gfx_rect(0, 170, text_width(buf) + 8, 10, C_NIGHT);
        text_draw(buf, 3, 171, C_WHITE);
    } else if (rb.phase == B_SELECT && rb.cpu[s] < 0) {
        int fx = ui_hint(3, 171, GLYPH_LEFT GLYPH_RIGHT, "PICK", C_WHITE);
        fx = ui_hint(fx, 171, GLYPH_A, "TAKE", C_WHITE);
        ui_hint(fx, 171, GLYPH_B, "HOLD: LOOK", C_WHITE);
    }
    if (rb.cam_free) text_draw("CAMERA", SCREEN_W - 44, 171, (frame_t / 10) % 2 ? C_YELLOW : C_WHITE);
    if (rb.phase == B_INTRO) {
        ui_panel(80, 70, 160, 30, C_NIGHT, C_YELLOW);
        text_center(rw.regroup ? "A SECOND STAND!" : "BATTLE!", 160, 76, C_WHITE);
        snprintf(buf, sizeof buf, "%s STRIKES FIRST", SIDE_NAME[rb.side]);
        tiny_center(buf, 160, 88, RSH_SIDE_COL[rb.side][0]);
    }
    if (rb.fog > 0 && rb.fog <= RSH_FOG_STEP && rb.phase != B_OVER && (frame_t / 20) % 4 != 3) {
        ui_panel(50, 20, 220, 14, C_NIGHT, C_MAGENTA);
        tiny_center("DON'T END YOUR TURN IN THE HAZE!", 160, 24, C_PINK);
    }
    if (rb.phase == B_OVER) {
        ui_panel(70, 64, 180, 40, C_NIGHT, rb.winner >= B_BOTH ? C_GREY : RSH_SIDE_COL[rb.winner][0]);
        if (rb.winner == B_BOTH) text_center("BOTH ARMIES FALL", 160, 72, C_WHITE);
        else if (rb.winner == B_STALE) text_center("A STALEMATE: BOTH GO HOME", 160, 72, C_WHITE);
        else {
            snprintf(buf, sizeof buf, "THE FIELD IS %s'S", SIDE_NAME[rb.winner]);
            text_center(buf, 160, 72, C_WHITE);
        }
        snprintf(buf, sizeof buf, "BEST CHAIN %d " GLYPH_DOT " COINS +%d", rb.p.best_combo, rb.p.coins[0]);
        tiny_center(buf, 160, 84, C_LIGHT);
        if (rb.over_t > 70 && (frame_t / 16) % 2) tiny_center("PRESS " GLYPH_A, 160, 94, C_YELLOW);
    }
}

static void draw_battle(void) {
    draw_field();
    draw_battle_hud();
}

/* ------------------------------------------------------------------ */
/* drawing: the screens                                                     */

static const uint8_t GRAD_TITLE[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};

static void draw_backdrop(void) {
    gfx_cls(C_NIGHT);
    for (int y = 0; y < 180; y += 2) gfx_hline(0, SCREEN_W - 1, y, y < 90 ? C_NAVY : C_NIGHT);
    ground(0, 120, SCREEN_W, 60, T_GRASS, 0, 0);
}

static void draw_title(void) {
    draw_backdrop();
    ui_fancy_center("RIMSHIRE", 160, 14, 3, GRAD_TITLE, 4, C_INK, C_WINE);
    tiny_center("THE BRASS BANNER AND THE PLUM BANNER FLICK IT OUT", 160, 42, C_LIGHT);
    spr_draw_scaled(&rsh_spr[RS_LORD0], 26, 60, 3, 0);
    spr_draw_scaled(&rsh_spr[RS_LORD1], 246, 60, 3, SPR_FLIPX);
    /* two disks meeting in the middle, over and over */
    int ph = frame_t % 120, dx = ph < 60 ? ph : 60;
    RshDisk a, b;
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    a.kind = K_WARDEN; a.side = 0; a.hp = 8;
    b.kind = K_BRUTE; b.side = 1; b.hp = 8;
    int ax = 100 + dx, bx = ph < 60 ? 220 : 220 + (ph - 60);
    rsh_draw_disk(&a, ax, 150, frame_t);
    rsh_draw_disk(&b, bx, 150, frame_t);
    static const char *const ITEMS[3] = {"CAMPAIGN", "STREAK", "TWO PLAYERS"};
    for (int i = 0; i < 3; i++) {
        int y = 72 + i * 14;
        bool locked = i == MODE_STREAK && !streak_open();
        char buf[48];
        snprintf(buf, sizeof buf, "%s%s", ITEMS[i], locked ? " " GLYPH_LOCK : "");
        text_center(buf, 160, y, title_sel == i ? C_WHITE : locked ? C_SLATE : C_GREY);
        if (title_sel == i) ui_cursor(160 - text_width(buf) / 2 - 10, y, frame_t);
    }
    char buf[64];
    snprintf(buf, sizeof buf, "WARS WON %d/10 " GLYPH_DOT " BEST STREAK %d", cleared_count(), sv.best_streak);
    tiny_center(buf, 160, 170, C_CREAM);
}

static const char *const STORY[3][3] = {
    {"RIMSHIRE IS A ROUND LITTLE LAND,", "AND EVERYONE IN IT IS ROUND TOO:", "SQUIRES, FERRETS, WYRMS AND ALL."},
    {"NOW THE BRASS BANNER AND THE PLUM", "BANNER BOTH CLAIM IT, AND THEIR", "ARMIES ARE ROLLING TOWARDS EACH OTHER."},
    {"DRAW YOUR ROUTES. HIRE AT THE INNS.", "THEN AIM, CHARGE AND LET FLY.", "LORD BRASS AWAITS YOUR ORDERS."},
};

static void draw_story(void) {
    draw_backdrop();
    spr_draw_scaled(&rsh_spr[story_i == 1 ? RS_LORD1 : RS_LORD0], 136, 18, 3, 0);
    for (int i = 0; i < 3; i++) text_center(STORY[story_i][i], 160, 80 + i * 12, C_WHITE);
    if (state_t > 20 && (frame_t / 16) % 2) tiny_center("PRESS " GLYPH_A, 160, 168, C_YELLOW);
}

static void draw_select(void) {
    draw_backdrop();
    ui_fancy_center("THE TEN WARS", 160, 4, 2, GRAD_TITLE, 4, C_INK, C_WINE);
    for (int i = 0; i < RSH_SCENARIOS; i++) {
        int y = 28 + i * 13;
        bool open = scen_open(i), sel = sel_scen == i;
        if (sel) gfx_rect(24, y - 2, 272, 12, C_DUSK);
        char buf[48];
        snprintf(buf, sizeof buf, "%2d  %s", i + 1, open ? RSH_SCEN[i].name : "? ? ?");
        text_draw(buf, 36, y, sel ? C_WHITE : open ? C_LIGHT : C_SLATE);
        if (sv.cleared[i]) text_draw(GLYPH_CHECK, 272, y, C_LIME);
        else if (!open) text_draw(GLYPH_LOCK, 272, y, C_SLATE);
        if (sel) ui_cursor(26, y, frame_t);
    }
    if (scen_open(sel_scen)) tiny_center(RSH_SCEN[sel_scen].intro, 160, 160, C_CREAM);
    int fx = ui_hint(4, 170, GLYPH_A, "TO WAR", C_LIGHT);
    ui_hint(fx, 170, GLYPH_B, "BACK", C_LIGHT);
}

/* Lady Brass writes to the front before every war (all our own words) */
static const char *const LETTER[RSH_SCENARIOS][2] = {
    {"THE PLUM BANNER IS ON THE LONG LANE.", "MEET IT HALFWAY, AND MIND YOUR SQUIRES."},
    {"OLD TOMES LIE ON THE OUTER ROADS.", "READ ONE BEFORE THE PLUMS DO."},
    {"THEY SAY THE MERE HAS NO BOTTOM.", "KEEP YOUR DISKS OUT OF IT, AND PUT THEIRS IN."},
    {"THE HILLS ARE FULL OF COIN THIS YEAR.", "HOLD THE SEAMS AND THE INNS WILL LOVE YOU."},
    {"I SEND YOU TWO ADDERS. BE KIND TO THEM;", "NOBODY ELSE IS."},
    {"OUR HOME IS A CASTLE NOW, AND SO IS THEIRS.", "LOSE THERE AND WE LOSE EVERYTHING."},
    {"TWO PIPERS MARCH WITH YOU. THEIR TUNES", "MAKE BRAVE DISKS BRAVER. ALL BATTLE LONG."},
    {"NO COIN LIES ON THESE ROADS. YOUR DELVERS", "WILL HAVE TO DIG IT OUT OF THE FIELDS."},
    {"THE RIVER SPLITS THE SHIRE IN TWO.", "THREE BRIDGES. CHOOSE WELL."},
    {"THE PLUM EMPRESS HERSELF WAITS AT HOME.", "BRING HER DOWN, AND THEN THE REST. LOVE, B."},
};

static void draw_card(void) {
    draw_backdrop();
    char buf[64];
    if (mode == MODE_CAMPAIGN) {
        const RshScenario *sc = &RSH_SCEN[scen];
        snprintf(buf, sizeof buf, "WAR %d", scen + 1);
        text_center(buf, 160, 8, C_YELLOW);
        ui_fancy_center(sc->name, 160, 20, 2, GRAD_TITLE, 4, C_INK, C_WINE);
        tiny_center(LETTER[scen][0], 160, 40, C_CREAM);
        tiny_center(LETTER[scen][1], 160, 47, C_CREAM);
        int n = 0;
        for (int k = 0; k < K_COUNT; k++) n += (sc->fresh >> k) & 1;
        if (n) {
            ui_panel(44, 56, 232, 14 + n * 22, C_NIGHT, C_DUSK);
            tiny_center(n == 1 ? "NEW IN THIS WAR" : "IN THIS WAR", 160, 60, C_SKY);
            int i = 0;
            for (int k = 0; k < K_COUNT; k++) {
                if (!((sc->fresh >> k) & 1)) continue;
                int y = 72 + i * 22;
                rsh_draw_disk_icon(k, k == K_EMPRESS ? 1 : 0, 60, y + 6, frame_t);
                if (RSH_KIND[k].cost) snprintf(buf, sizeof buf, "%s " GLYPH_DOT " HP %d " GLYPH_DOT " " GLYPH_COIN "%d", RSH_KIND[k].name, RSH_KIND[k].hp, RSH_KIND[k].cost);
                else snprintf(buf, sizeof buf, "%s " GLYPH_DOT " HP %d " GLYPH_DOT " NOT FOR HIRE", RSH_KIND[k].name, RSH_KIND[k].hp);
                text_draw(buf, 76, y, C_WHITE);
                tiny_draw(RSH_KIND[k].blurb, 76, y + 9, C_LIGHT);
                i++;
            }
        }
    } else if (mode == MODE_STREAK) {
        text_center("THE STREAK", 160, 20, C_YELLOW);
        snprintf(buf, sizeof buf, "WAR %d OF THE STREAK", sv.streak + 1);
        ui_fancy_center(buf, 160, 40, 2, GRAD_TITLE, 4, C_INK, C_WINE);
        text_center("A NEW LAND EVERY WAR. THE PLUM BANNER", 160, 70, C_WHITE);
        text_center("GROWS STRONGER WITH EVERY WIN.", 160, 82, C_WHITE);
        snprintf(buf, sizeof buf, "BEST STREAK %d", sv.best_streak);
        tiny_center(buf, 160, 104, C_LIGHT);
    } else {
        ui_fancy_center("TWO PLAYERS", 160, 30, 2, GRAD_TITLE, 4, C_INK, C_WINE);
        text_center("BRASS: PAD 1. PLUM: PAD 2.", 160, 70, C_WHITE);
        text_center("A NEW LAND, THE SAME RULES.", 160, 84, C_WHITE);
    }
    if (state_t > 20 && (frame_t / 16) % 2) text_center("PRESS " GLYPH_A " TO MARCH", 160, 160, C_YELLOW);
}

static void draw_warover(void) {
    draw_backdrop();
    char buf[64];
    int w = rw.winner;
    if (w == 2) ui_fancy_center("A STALEMATE", 160, 30, 2, GRAD_TITLE, 4, C_INK, C_WINE);
    else {
        snprintf(buf, sizeof buf, "%s WINS THE WAR", SIDE_NAME[w]);
        static const uint8_t GP[] = {C_WHITE, C_PINK, C_MAGENTA, C_PURPLE};
        ui_fancy_center(buf, 160, 30, 2, w ? GP : GRAD_TITLE, 4, C_INK, C_WINE);
        spr_draw_scaled(&rsh_spr[w ? RS_LORD1 : RS_LORD0], 136, 60, 3, 0);
    }
    if (mode == MODE_STREAK) {
        snprintf(buf, sizeof buf, w == 0 ? "STREAK %d " GLYPH_DOT " BEST %d" : "THE STREAK ENDS " GLYPH_DOT " BEST %d", w == 0 ? sv.streak : sv.best_streak, sv.best_streak);
        text_center(buf, 160, 120, C_WHITE);
    }
    snprintf(buf, sizeof buf, "BATTLES: BRASS %d, PLUM %d", rw.s[0].wins, rw.s[1].wins);
    tiny_center(buf, 160, 134, C_LIGHT);
    if (state_t > 90 && (frame_t / 16) % 2) tiny_center("PRESS " GLYPH_A, 160, 160, C_YELLOW);
}

static void draw_end(void) {
    draw_backdrop();
    ui_fancy_center("RIMSHIRE IS WHOLE", 160, 10, 2, GRAD_TITLE, 4, C_INK, C_WINE);
    static const char *const L[5] = {
        "THE PLUM EMPRESS ROLLS HOME AT LAST,",
        "AND THE ROADS OF RIMSHIRE GO QUIET.",
        "THE INNS REOPEN. THE FERRETS NAP.",
        "LORD BRASS HANGS HIS BANNER BY THE FIRE",
        "AND NEVER, EVER POLISHES IT.",
    };
    for (int i = 0; i < 5; i++)
        if (state_t > 40 + i * 40) text_center(L[i], 160, 40 + i * 12, C_WHITE);
    spr_draw_scaled(&rsh_spr[RS_LORD0], 64, 110, 3, 0);
    spr_draw_scaled(&rsh_spr[RS_LORD1], 208, 110, 3, SPR_FLIPX);
    if (state_t > 300) {
        text_center("THE END", 160, 130, C_YELLOW);
        if ((frame_t / 16) % 2) tiny_center("PRESS " GLYPH_A, 160, 168, C_LIGHT);
    }
}

static void rsh_draw(void) {
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_STORY: draw_story(); break;
    case S_SELECT: draw_select(); break;
    case S_CARD: draw_card(); break;
    case S_WAR: draw_war(); break;
    case S_BATTLE: draw_battle(); break;
    case S_WAROVER: draw_warover(); break;
    case S_END: draw_end(); break;
    }
    if (state != S_BATTLE)
        for (int i = 0; i < ARRAY_LEN(parts); i++)
            if (parts[i].life > 0) gfx_pset((int)parts[i].x, (int)parts[i].y, parts[i].col);
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                    */

static void rsh_load(void) {
    rsh_art_load();
    rsh_audio_load();
}

static void rsh_start(void) {
    load_save();
    memset(parts, 0, sizeof parts);
    bot_play = false;
    title_sel = 0;
    sel_scen = 0;
    for (int i = 0; i < RSH_SCENARIOS - 1; i++)
        if (sv.cleared[i] && !sv.cleared[i + 1]) sel_scen = i + 1;
    prev_in[0] = prev_in[1] = 0;
    to_title();
}

static void rsh_quit(void) {
    /* leaving in the middle of a streak war ends the streak */
    if ((state == S_WAR || state == S_BATTLE) && mode == MODE_STREAK) sv.streak = 0;
    input_set_versus(false);
    save_now();
}

static void rsh_label(int x, int y, int w, int h, int t) {
    ground(x, y, w, h, T_GRASS, 0, 0);
    ground(x, y + 40, w, h - 40, T_SAND, 0, 40);
    RshDisk a, b, c;
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    memset(&c, 0, sizeof c);
    a.kind = K_WYRM; a.side = 0; a.hp = 8;
    b.kind = K_LEECH; b.side = 1; b.hp = 6;
    c.kind = K_FERRET; c.side = 1; c.hp = 3;
    int ph = t % 90;
    int ax = x + 20 + (ph < 40 ? ph : 40);
    int bx = x + 72 + (ph < 40 ? 0 : (ph - 40) / 2), cx = x + 110 + (ph < 50 ? 0 : (ph - 50) / 3);
    rsh_draw_disk(&a, ax, y + 28, t);
    rsh_draw_disk(&b, bx, y + 30, t);
    rsh_draw_disk(&c, cx, y + 24, t);
    draw_token(0, x + 10, y + 50, 0);
    draw_token(1, x + w - 12, y + 50, 0);
    (void)h;
}

/* ------------------------------------------------------------------ */
/* test hooks                                                              */

/* every board is sound: sizes, both homes, homes joined by roads, no
 * road to nowhere. Returns how many of the ten pass. */
static int maps_ok(void) {
    int good = 0;
    static RshWar keep;
    memcpy(&keep, &rw, sizeof keep);
    for (int i = 0; i < RSH_SCENARIOS; i++) {
        const RshScenario *sc = &RSH_SCEN[i];
        bool bad = false;
        for (int r = 0; r < 2 * sc->h - 1; r++) bad |= !sc->rows[r] || (int)strlen(sc->rows[r]) != 2 * sc->w - 1;
        if (bad) continue;
        rsh_war_start(sc, i, -1, -1);
        int homes = 0;
        for (int y = 0; y < rw.h; y++)
            for (int x = 0; x < rw.w; x++) {
                homes += rw.node[y][x] == N_BASE0 || rw.node[y][x] == N_BASE1;
                if (rw.er[y][x] && !(rsh_node_ok(x, y) && rsh_node_ok(x + 1, y))) bad = true;
                if (rw.ed[y][x] && !(rsh_node_ok(x, y) && rsh_node_ok(x, y + 1))) bad = true;
            }
        if (homes != 2) bad = true;
        uint8_t seen[RSH_MH][RSH_MW];
        memset(seen, 0, sizeof seen);
        int qx[128], qy[128], qh = 0, qt = 0;
        qx[qt] = rw.s[0].bx;
        qy[qt++] = rw.s[0].by;
        seen[rw.s[0].by][rw.s[0].bx] = 1;
        while (qh < qt) {
            int x = qx[qh], y = qy[qh++];
            for (int d = 0; d < 4; d++) {
                int nx = x + RSH_DX[d], ny = y + RSH_DY[d];
                if (rsh_road(x, y, nx, ny) && !seen[ny][nx] && qt < 128) { seen[ny][nx] = 1; qx[qt] = nx; qy[qt++] = ny; }
            }
        }
        if (!seen[rw.s[1].by][rw.s[1].bx]) bad = true;
        if (!bad) good++;
    }
    memcpy(&rw, &keep, sizeof rw);
    return good;
}

static int rsh_query(const char *key, int *out) {
    int a, b;
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "mode")) { *out = mode; return 1; }
    if (!strcmp(key, "scen")) { *out = scen + 1; return 1; }
    if (!strcmp(key, "title_sel")) { *out = title_sel; return 1; }
    if (!strcmp(key, "sel_scen")) { *out = sel_scen + 1; return 1; }
    if (!strcmp(key, "wstate")) { *out = rw.state; return 1; }
    if (!strcmp(key, "turn")) { *out = rw.turn; return 1; }
    if (!strcmp(key, "turns")) { *out = rw.turns; return 1; }
    if (!strcmp(key, "moves")) { *out = rw.moves; return 1; }
    if (!strcmp(key, "moves_used")) { *out = rw.moves_used; return 1; }
    if (!strcmp(key, "can_step")) { *out = rsh_can_step(); return 1; }
    if (!strcmp(key, "swap_kind")) { *out = rw.swap_kind; return 1; }
    if (!strcmp(key, "winner")) { *out = rw.winner; return 1; }
    if (!strcmp(key, "pick")) { *out = rw.menu_sel; return 1; }
    if (!strcmp(key, "regroup")) { *out = rw.regroup; return 1; }
    if (!strcmp(key, "cpuplan")) {
        int8_t px[4], py[4];
        int keep = rw.plan_n;
        *out = rsh_cpu_map_plan(rw.turn, px, py);
        (void)px;
        (void)py;
        rw.plan_n = keep;
        return 1;
    }
    if (!strcmp(key, "maps_ok")) { *out = maps_ok(); return 1; }
    if (!strcmp(key, "art_bad")) { *out = rsh_art_bad; return 1; }
    if (!strcmp(key, "cleared")) { *out = cleared_count(); return 1; }
    if (!strcmp(key, "streak")) { *out = sv.streak; return 1; }
    if (!strcmp(key, "best_streak")) { *out = sv.best_streak; return 1; }
    if (!strcmp(key, "story_seen")) { *out = sv.story_seen; return 1; }
    if (!strcmp(key, "wars_won")) { *out = sv.wars_won; return 1; }
    if (!strcmp(key, "offer0")) { *out = rw.offer[0]; return 1; }
    if (!strcmp(key, "tome0")) { *out = rw.tome[0]; return 1; }
    if (sscanf(key, "seam_left_%d_%d", &a, &b) == 2) { *out = a >= 0 && b >= 0 && a < RSH_MW && b < RSH_MH ? rw.seam_left[b][a] : -1; return 1; }
    if (sscanf(key, "cleared%d", &a) == 1) { *out = a >= 1 && a <= RSH_SCENARIOS ? sv.cleared[a - 1] : -1; return 1; }
    if (sscanf(key, "offer%d", &a) == 1) { *out = a >= 0 && a < 3 ? rw.offer[a] : -1; return 1; }
    if (sscanf(key, "tome%d", &a) == 1) { *out = a >= 0 && a < 3 ? rw.tome[a] : -1; return 1; }
    if (sscanf(key, "node_%d_%d", &a, &b) == 2) { *out = a >= 0 && b >= 0 && a < RSH_MW && b < RSH_MH ? rw.node[b][a] : -1; return 1; }
    if (sscanf(key, "owner_%d_%d", &a, &b) == 2) { *out = a >= 0 && b >= 0 && a < RSH_MW && b < RSH_MH ? rw.owner[b][a] : -1; return 1; }
    if (sscanf(key, "visits_%d_%d", &a, &b) == 2) { *out = a >= 0 && b >= 0 && a < RSH_MW && b < RSH_MH ? rw.visits[b][a] : -1; return 1; }
    /* per side: nx0, trail1, coins0, army1, reserve0, skills0, cpu1, wins0 ... */
    {
        static const char *const F[] = {"nx", "ny", "trail", "coins", "army", "reserve", "skills", "cpu", "wins", "losses", "castle", "bx", "by"};
        for (int f = 0; f < ARRAY_LEN(F); f++) {
            size_t l = strlen(F[f]);
            if (strncmp(key, F[f], l) || !(key[l] == '0' || key[l] == '1') || key[l + 1]) continue;
            const RshSide *s = &rw.s[key[l] - '0'];
            int v[] = {s->nx, s->ny, s->trail_n, s->coins, s->n_army, s->n_reserve, s->skills, s->cpu, s->wins, s->losses, s->castle, s->bx, s->by};
            *out = v[f];
            return 1;
        }
    }
    if (sscanf(key, "athome%d", &a) == 1) { *out = a >= 0 && a < 2 && rw.s[a].nx == rw.s[a].bx && rw.s[a].ny == rw.s[a].by; return 1; }
    if (!strcmp(key, "first_move")) { *out = rw.first_move; return 1; }
    if (!strcmp(key, "home_hold")) { *out = rw.home_hold; return 1; }
    if (sscanf(key, "armykind%d_%d", &a, &b) == 2) { *out = a >= 0 && a < 2 && b >= 0 && b < rw.s[a].n_army ? rw.s[a].army[b] : -1; return 1; }
    if (sscanf(key, "reskind%d_%d", &a, &b) == 2) { *out = a >= 0 && a < 2 && b >= 0 && b < rw.s[a].n_reserve ? rw.s[a].reserve[b] : -1; return 1; }
    /* the battle */
    if (!strcmp(key, "bphase")) { *out = rb.phase; return 1; }
    if (!strcmp(key, "bside")) { *out = rb.side; return 1; }
    if (!strcmp(key, "bwinner")) { *out = rb.winner; return 1; }
    if (!strcmp(key, "sel")) { *out = rb.sel; return 1; }
    if (!strcmp(key, "cursor")) { *out = rb.cursor; return 1; }
    if (!strcmp(key, "angle")) { *out = rb.angle; return 1; }
    if (!strcmp(key, "pips")) { *out = rb.pips; return 1; }
    if (!strcmp(key, "charging")) { *out = rb.charging; return 1; }
    if (!strcmp(key, "move_i")) { *out = rb.move_i; return 1; }
    if (!strcmp(key, "bmoves")) { *out = rb.moves; return 1; }
    if (!strcmp(key, "moving")) { *out = rb.p.moving; return 1; }
    if (!strcmp(key, "round")) { *out = rb.round; return 1; }
    if (!strcmp(key, "fog")) { *out = rb.fog; return 1; }
    if (!strcmp(key, "fog_deaths")) { *out = rb.fog_deaths; return 1; }
    if (!strcmp(key, "quiet")) { *out = rb.quiet; return 1; }
    if (!strcmp(key, "walls")) { *out = rb.nwall; return 1; }
    if (!strcmp(key, "trees")) {
        *out = 0;
        for (int k = 0; k < RSH_MAXO; k++) *out += rb.p.o[k].on && rb.p.o[k].kind == O_TREE;
        return 1;
    }
    if (!strcmp(key, "water_deaths")) { *out = rb.p.water_deaths; return 1; }
    if (!strcmp(key, "hits")) { *out = rb.p.hits; return 1; }
    if (!strcmp(key, "combo")) { *out = rb.p.best_combo; return 1; }
    if (!strcmp(key, "launches")) { *out = rb.launches; return 1; }
    if (!strcmp(key, "skipped")) { *out = rb.skip_ranged; return 1; }
    if (!strcmp(key, "cam_free")) { *out = rb.cam_free; return 1; }
    if (!strcmp(key, "cam_x")) { *out = rb.cam_x; return 1; }
    if (!strcmp(key, "cam_y")) { *out = rb.cam_y; return 1; }
    if (!strcmp(key, "no_room")) { *out = rb.no_room; return 1; }
    if (!strcmp(key, "eligible")) { int el[5]; *out = rsh_eligible(rb.side, el); return 1; }
    if (!strcmp(key, "embers")) {
        *out = 0;
        for (int k = 0; k < RSH_MAXO; k++) *out += rb.p.o[k].on && rb.p.o[k].kind == O_EMBER;
        return 1;
    }
    if (!strcmp(key, "objs")) {
        *out = 0;
        for (int k = 0; k < RSH_MAXO; k++) *out += rb.p.o[k].on;
        return 1;
    }
    if (sscanf(key, "shards%d", &a) == 1) { *out = a >= 0 && a < 2 ? rb.p.shards[a] : -1; return 1; }
    if (sscanf(key, "bcoins%d", &a) == 1) { *out = a >= 0 && a < 2 ? rb.p.coins[a] : -1; return 1; }
    if (sscanf(key, "left%d", &a) == 1) { *out = a >= 0 && a < 2 ? rsh_disks_left(a) : -1; return 1; }
    if (sscanf(key, "kills%d", &a) == 1) { *out = a >= 0 && a < 2 ? rb.p.kills[a] : -1; return 1; }
    if (sscanf(key, "queue%d_%d", &a, &b) == 2) { *out = a >= 0 && a < 2 && b >= 0 && b < rb.qn[a] ? rb.queue[a][b] : -1; return 1; }
    if (sscanf(key, "cell_%d_%d", &a, &b) == 2) { *out = a >= 0 && b >= 0 && a < RSH_CW && b < RSH_CH ? rb.cell[b][a] : -1; return 1; }
    if (sscanf(key, "quad%d", &a) == 1) { *out = a >= 0 && a < 4 ? rb.quad[a] : -1; return 1; }
    if (sscanf(key, "d%d_", &a) == 1 && strchr(key, '_')) {
        const char *f = strchr(key, '_') + 1;
        if (a < 0 || a >= RSH_MAXD) return 0;
        const RshDisk *d = &rb.p.d[a];
        if (!strcmp(f, "on")) { *out = d->on; return 1; }
        if (!strcmp(f, "hp")) { *out = d->hp; return 1; }
        if (!strcmp(f, "x")) { *out = (int)d->x; return 1; }
        if (!strcmp(f, "y")) { *out = (int)d->y; return 1; }
        if (!strcmp(f, "kind")) { *out = d->kind; return 1; }
        if (!strcmp(f, "side")) { *out = d->side; return 1; }
        if (!strcmp(f, "stun")) { *out = d->stun; return 1; }
        if (!strcmp(f, "poison")) { *out = d->poison; return 1; }
        if (!strcmp(f, "stars")) { *out = d->stars; return 1; }
        if (!strcmp(f, "proj")) { *out = d->proj; return 1; }
        return 0;
    }
    if (!strcmp(key, "bot")) { *out = (int)bot_buttons(); return 1; }
    return 0;
}

static void set_army(RshSide *s, const char *letters, int reserve) {
    uint8_t *arr = reserve ? s->reserve : s->army;
    int *n = reserve ? &s->n_reserve : &s->n_army, cap = reserve ? RSH_RESERVE : RSH_ARMY;
    *n = 0;
    if (!strcmp(letters, "-")) return;
    for (const char *c = letters; *c && *n < cap; c++) {
        int k = rsh_kind_of_letter(*c);
        if (k >= 0) arr[(*n)++] = (uint8_t)k;
    }
}

/* a bare field of one kind of ground, with nothing lying on it */
static void bare_field(int t) {
    for (int y = 0; y < RSH_CH; y++)
        for (int x = 0; x < RSH_CW; x++) rb.cell[y][x] = (uint8_t)t;
    for (int k = 0; k < RSH_MAXO; k++) rb.p.o[k].on = 0;
    rb.nwall = 0;
}

static int rsh_cheat(const char *cmd) {
    int a, b, c, d;
    char s1[40];
    if (sscanf(cmd, "war %d", &a) == 1) {
        mode = MODE_CAMPAIGN;
        scen = iclamp(a - 1, 0, RSH_SCENARIOS - 1);
        start_war();
        return 1;
    }
    if (!strcmp(cmd, "streak")) { mode = MODE_STREAK; start_war(); return 1; }
    if (!strcmp(cmd, "versus")) { mode = MODE_VERSUS; start_war(); return 1; }
    if (!strcmp(cmd, "title")) { to_title(); return 1; }
    if (sscanf(cmd, "clear %d %d", &a, &b) == 2) {
        if (a >= 1 && a <= RSH_SCENARIOS) sv.cleared[a - 1] = (uint8_t)b;
        save_now();
        check_goals();
        return 1;
    }
    if (!strcmp(cmd, "clearall")) {
        for (int i = 0; i < RSH_SCENARIOS; i++) sv.cleared[i] = 1;
        save_now();
        check_goals();
        return 1;
    }
    if (sscanf(cmd, "setstreak %d", &a) == 1) { sv.streak = (uint16_t)a; if (a > sv.best_streak) sv.best_streak = (uint16_t)a; save_now(); check_goals(); return 1; }
    if (sscanf(cmd, "coins %d %d", &a, &b) == 2) { rw.s[a & 1].coins = b; return 1; }
    if (sscanf(cmd, "skills %d %d", &a, &b) == 2) { rw.s[a & 1].skills = b; rb.p.skills[a & 1] = b; return 1; }
    if (sscanf(cmd, "cpu %d %d", &a, &b) == 2) {
        rw.s[a & 1].cpu = b;
        rb.cpu[a & 1] = b;
        if (state == S_WAR && rw.turn == (a & 1) && rw.state == W_CPU && b < 0) rw.state = W_PLAN;
        return 1;
    }
    if (sscanf(cmd, "army %d %39s", &a, s1) == 2) { set_army(&rw.s[a & 1], s1, 0); return 1; }
    if (sscanf(cmd, "reserve %d %39s", &a, s1) == 2) { set_army(&rw.s[a & 1], s1, 1); return 1; }
    if (sscanf(cmd, "at %d %d %d", &a, &b, &c) == 3) {
        RshSide *s = &rw.s[a & 1];
        s->nx = b;
        s->ny = c;
        s->trail_n = 1;
        s->tx[0] = (int8_t)s->bx;
        s->ty[0] = (int8_t)s->by;
        if (b != s->bx || c != s->by) { s->tx[1] = (int8_t)b; s->ty[1] = (int8_t)c; s->trail_n = 2; }
        return 1;
    }
    /* a side's banner steps to (x, y), its route growing (no rules checked) */
    if (sscanf(cmd, "step %d %d %d", &a, &b, &c) == 3) {
        RshSide *s = &rw.s[a & 1];
        if (s->trail_n < RSH_TRAIL) { s->tx[s->trail_n] = (int8_t)b; s->ty[s->trail_n] = (int8_t)c; s->trail_n++; }
        s->nx = b;
        s->ny = c;
        return 1;
    }
    if (sscanf(cmd, "turn %d", &a) == 1) {
        rw.turn = a & 1;
        rw.first_move = 0;
        rw.moves = (rw.s[rw.turn].skills & SK_SCOUTING) ? 3 : 2;
        rw.plan_n = 0;
        rw.state = rw.s[rw.turn].cpu >= 0 ? W_CPU : W_PLAN;
        rw.state_t = 0;
        return 1;
    }
    if (sscanf(cmd, "node %d %d %d", &a, &b, &c) == 3) { rw.node[b][a] = (uint8_t)c; return 1; }
    /* a battle at a board node, attacker first */
    if (sscanf(cmd, "battle %d %d %d", &a, &b, &c) == 3) {
        rw.bx = a;
        rw.by = b;
        rw.attacker = c & 1;
        rw.state = W_BATTLE;
        rsh_battle_start(c & 1, 12345u);
        rb.cpu[0] = rw.s[0].cpu;
        rb.cpu[1] = rw.s[1].cpu;
        to_battle();
        return 1;
    }
    if (sscanf(cmd, "field %d", &a) == 1) { bare_field(a); return 1; }
    /* the queues back in the armies' order (the battle shuffles them) */
    if (!strcmp(cmd, "unshuffle")) {
        for (int sd = 0; sd < 2; sd++) {
            rb.qn[sd] = 0;
            for (int i = 0; i < RSH_MAXD; i++)
                if (rb.p.d[i].on && !rb.p.d[i].proj && rb.p.d[i].side == sd) rb.queue[sd][rb.qn[sd]++] = (uint8_t)i;
        }
        return 1;
    }
    if (sscanf(cmd, "pond %d %d %d %d", &a, &b, &c, &d) == 4) {
        for (int y = b; y < b + d && y < RSH_CH; y++)
            for (int x = a; x < a + c && x < RSH_CW; x++) rb.cell[y][x] = T_WATER;
        return 1;
    }
    if (sscanf(cmd, "place %d %d %d", &a, &b, &c) == 3) {
        if (a >= 0 && a < RSH_MAXD) { rb.p.d[a].x = (float)b; rb.p.d[a].y = (float)c; rb.p.d[a].vx = rb.p.d[a].vy = 0; }
        return 1;
    }
    if (sscanf(cmd, "hp %d %d", &a, &b) == 2) { if (a >= 0 && a < RSH_MAXD) rb.p.d[a].hp = (int8_t)b; return 1; }
    if (sscanf(cmd, "kill %d", &a) == 1) { if (a >= 0 && a < RSH_MAXD) rb.p.d[a].on = 0; return 1; }
    if (sscanf(cmd, "stars %d %d", &a, &b) == 2) { if (a >= 0 && a < RSH_MAXD) rb.p.d[a].stars = (uint8_t)b; return 1; }
    if (sscanf(cmd, "poison %d", &a) == 1) { if (a >= 0 && a < RSH_MAXD) rb.p.d[a].poison = 1; return 1; }
    if (sscanf(cmd, "shards %d %d", &a, &b) == 2) { rb.p.shards[a & 1] = b; return 1; }
    if (sscanf(cmd, "round %d", &a) == 1) {
        rb.turns[0] = rb.turns[1] = rb.round = a;
        rb.fog = a >= RSH_FOG_ROUND ? imin(RSH_FOG_MAX, (a - RSH_FOG_ROUND + 1) * RSH_FOG_STEP) : 0;
        return 1;
    }
    if (sscanf(cmd, "obj %d %d %d", &a, &b, &c) == 3) {
        for (int k = 0; k < RSH_MAXO; k++)
            if (!rb.p.o[k].on) {
                int solid = a == O_WELL || a == O_PILE || a == O_CLUSTER;
                rb.p.o[k] = (RshObj){1, (uint8_t)a, (uint8_t)(a == O_WELL ? 3 : a == O_PILE ? 4 : a == O_CLUSTER ? 3 : a == O_SHARD ? 2 : 1), 0, (float)b, (float)c, solid ? 7.0f : a == O_EMBER ? 3.0f : 4.0f};
                break;
            }
        return 1;
    }
    if (!strcmp(cmd, "autoplay")) { bot_play = !bot_play; return 1; }
    return 0;
}

const GameDef GAME_RIMSHIRE = {
    "rimshire",
    "RIMSHIRE",
    "1988",
    "STRATEGY",
    "DRAW YOUR ROUTE, HIRE YOUR DISKS, THEN AIM, CHARGE AND FLICK THEM AT THE PLUM BANNER!",
    {"WIN FIVE WARS", "WIN ALL TEN WARS", "WIN THREE STREAK WARS IN A ROW"},
    "BOARD\t\n"
    GLYPH_DPAD "\tDRAW YOUR ROUTE\n"
    GLYPH_A "\tGO / BUY / LEARN\n"
    "HOLD " GLYPH_B "\tRETREAT HOME (ENDS THE TURN)\n"
    "BATTLE\t\n"
    GLYPH_LEFT GLYPH_RIGHT " " GLYPH_A "\tPICK A DISK\n"
    GLYPH_DPAD "\tTURN THE AIM\n"
    "HOLD " GLYPH_A "\tCHARGE, LET GO TO LAUNCH\n"
    "HOLD " GLYPH_B "+" GLYPH_DPAD "\tLOOK ROUND\n"
    "START\tPAUSE",
    C_AMBER, C_VIOLET,
    rsh_load, rsh_start, rsh_update, rsh_draw, rsh_quit, rsh_label, rsh_query, rsh_cheat,
    "LORDS OF DISKONIA", 41,
};
