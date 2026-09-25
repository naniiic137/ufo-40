/* TINTAIL - a chameleon crosses Salt Island by matching the ground.
 * Cartridge 16 of UFO 40, a tribute to Camouflage (UFO 50 #16).
 * See docs/games/16-tintail.md. The rules live in tintail_logic.c and the
 * fifteen levels in tintail_levels.c; this file is the island map, the
 * levels' presentation, saving and the flow. */
#include "tintail.h"

#define TS 16
#define OY 0              /* the level fills the top 160 pixels */
#define HUD_Y 160         /* the bar at the bottom: level, visibility, pickups */
#define HIST_MAX 2048
#define TOTAL_POINTS (TN_LEVELS + (TN_LEVELS - 1) * 3)

enum { S_TITLE, S_STORY, S_MAP, S_PLAY, S_MENU, S_CAUGHT, S_ESCAPE, S_ENDING };

typedef struct Save {
    uint32_t magic;
    uint16_t beaten;               /* a bit per level */
    uint8_t best[TN_LEVELS];       /* collectibles in the best escape */
    uint8_t seen_intro, cursor;
    uint32_t deaths;
} Save;
#define SAVE_MAGIC 0x54540001u

static Save sv;
static int state, state_t, frame_t;

/* ------------------------------------------------------------------ */
/* save, completion and goals                                           */

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
    }
    if (sv.cursor >= TN_LEVELS) sv.cursor = 0;
}

static bool beaten(int lv) { return (sv.beaten >> lv) & 1; }

/* The island's trails branch: each level opens the next two, so a hard one
 * can be passed by; the last two open only from the one before. */
static bool trail(int a, int b) { return b == a + 1 || (b == a + 2 && b < TN_LEVELS - 2); }
static bool is_open(int lv) {
    if (lv == 0) return true;
    for (int a = 0; a < lv; a++)
        if (trail(a, lv) && beaten(a)) return true;
    return false;
}
static int open_count(void) {
    int n = 0;
    for (int i = 0; i < TN_LEVELS; i++) n += is_open(i);
    return n;
}
static int points(void) {
    int p = 0;
    for (int i = 0; i < TN_LEVELS; i++) p += beaten(i) + sv.best[i];
    return p;
}
static int completion(void) { return points() * 100 / TOTAL_POINTS; }

static void check_goals(void) {
    if (completion() >= 30) game_award(GOAL_BEACON);
    if (beaten(TN_LEVELS - 1)) game_award(GOAL_SAUCER);
    if (points() >= TOTAL_POINTS) game_award(GOAL_ALIEN);
}

/* ------------------------------------------------------------------ */
/* playing a level                                                      */

static TtLevel L;
static TtState S, prev_S;
static TtState hist[HIST_MAX];
static uint8_t hist_act[HIST_MAX];
static int hist_n;
static int cur, beat_t, queued = -1, anim_t = 99, intro_t, menu_sel, caught_t, escape_t;
static int last_ev, shake_t;
static bool falcon;
static char auto_buf[4096];
static const char *autoplay;
static uint8_t ground_px[TN_W * TS * TN_H * TS];
static Surface ground = {TN_W * TS, TN_H * TS, ground_px};
static int ground_sw = -1;

typedef struct Part {
    float x, y, vx, vy;
    int life, col, kind;
} Part;
static Part parts[160];

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) {
            parts[i] = (Part){x, y, vx, vy, life, col, kind};
            return;
        }
}

static void burst(int tx, int ty, int n, int c1, int c2, float spd) {
    float cx = tx * TS + 8, cy = OY + ty * TS + 8;
    for (int i = 0; i < n; i++) {
        float a = (float)i / n * 6.2832f;
        part_add(cx, cy, cosf(a) * spd, sinf(a) * spd - 0.6f, 18 + i % 8, i % 2 ? c1 : c2, 0);
    }
}

static void enter_level(int lv) {
    if (tn_parse(&TN_LEVEL_DEFS[lv], &L, &S) != 0) return;
    cur = lv;
    prev_S = S;
    hist[0] = S;
    hist_n = 1;
    beat_t = 0;
    queued = -1;
    anim_t = 99;
    last_ev = 0;
    autoplay = NULL;
    ground_sw = -1;
    memset(parts, 0, sizeof parts);
    intro_t = 70;
    state = S_PLAY;
    state_t = 0;
    game_set_pausable(true);
    music_play(lv == TN_LEVELS - 1 ? TN_MUS_GATE : TN_MUS_LEVEL);
}

static void go_map(void) {
    state = S_MAP;
    state_t = 0;
    game_set_pausable(true);
    music_play(TN_MUS_MAP);
}

static void on_escape(void) {
    int got = tn_collected(&S);
    bool first = !beaten(cur);
    sv.beaten |= (uint16_t)(1u << cur);
    if (got > sv.best[cur]) sv.best[cur] = (uint8_t)got;
    if (cur + 1 < TN_LEVELS && first && is_open(cur + 1)) sv.cursor = (uint8_t)(cur + 1);
    else sv.cursor = (uint8_t)cur;
    check_goals();
    save_now();
    state = S_ESCAPE;
    escape_t = 0;
    music_restart(TN_MUS_WIN);
}

static void on_caught(void) {
    sv.deaths++;
    save_now();
    state = S_CAUGHT;
    caught_t = 0;
    menu_sel = 0;
    shake_t = 10;
    /* who comes for you: the predator that saw you, or the falcon if it's far */
    int px, py, tx = S.baby_eaten ? S.bx : S.x, ty = S.baby_eaten ? S.by : S.y;
    if (S.eaten_by == 1) tn_stork_at(&L, S.eater, S.beat, &px, &py, NULL);
    else { px = L.toad_x[S.eater]; py = L.toad_y[S.eater]; }
    falcon = iabs(px - tx) + iabs(py - ty) > 3;
    sfx_play_name("tn_spot");
    music_stop();
}

static void do_beat(void) {
    int act = ACT_WAIT;
    if (S.camo_t == 0) {
        if (autoplay && *autoplay) {
            char c = *autoplay++;
            act = c == 'U' ? ACT_UP : c == 'R' ? ACT_RIGHT : c == 'D' ? ACT_DOWN : c == 'L' ? ACT_LEFT : c == 'C' ? ACT_CAMO : ACT_WAIT;
        } else if (autoplay) {
            autoplay = NULL;
        } else if (btn(BTN_B)) {
            act = ACT_WAIT; /* looking at the danger: no moving, no changing */
        } else if (queued >= 0) {
            act = queued;
        } else if (btn(BTN_UP)) act = ACT_UP;
        else if (btn(BTN_DOWN)) act = ACT_DOWN;
        else if (btn(BTN_LEFT)) act = ACT_LEFT;
        else if (btn(BTN_RIGHT)) act = ACT_RIGHT;
    } else if (autoplay && *autoplay) {
        autoplay++; /* the solver's route marks the second beat of a change too */
    }
    queued = -1;
    prev_S = S;
    if (hist_n >= HIST_MAX) {
        memmove(hist, hist + 1, sizeof hist[0] * (HIST_MAX - 1));
        memmove(hist_act, hist_act + 1, HIST_MAX - 1);
        hist_n--;
    }
    hist_act[hist_n - 1] = (uint8_t)act;
    int ev = tn_step(&L, &S, act);
    hist[hist_n++] = S;
    last_ev = ev;
    anim_t = 0;
    if (ev & TE_STEP) sfx_play_name(ev & TE_LOG ? "tn_log" : "tn_step");
    if (ev & TE_BUMP) sfx_play_name("tn_bump");
    if (ev & TE_REFUSED) sfx_play_name("tn_no");
    if (ev & TE_CAMO_START) {
        sfx_play_name("tn_camo");
        burst(S.x, S.y, 10, C_WHITE, C_YELLOW, 1.1f);
    }
    if (ev & TE_CAMO_DONE) {
        if (tn_hidden(&L, &S, 0)) sfx_play_name("tn_hidden");
        burst(S.x, S.y, 8, C_LIGHT, C_CREAM, 0.8f);
    }
    if (ev & TE_FRUIT) { sfx_play_name("tn_pear"); burst(S.x, S.y, 14, C_MAGENTA, C_YELLOW, 1.4f); }
    if (ev & TE_BABY) { sfx_play_name("tn_baby"); burst(S.x, S.y, 12, C_CREAM, C_LIME, 1.2f); }
    if (ev & (TE_RAIN | TE_SUN)) {
        sfx_play_name(ev & TE_RAIN ? "tn_rain" : "tn_sun");
        for (int i = 0; i < 70; i++)
            part_add((float)(i * 47 % 320), (float)(OY + (i * 29) % 40), ev & TE_RAIN ? -0.5f : 0, ev & TE_RAIN ? 3.0f : -0.4f,
                     30 + i % 30, ev & TE_RAIN ? (i % 2 ? C_SKY : C_ICE) : (i % 2 ? C_YELLOW : C_AMBER), ev & TE_RAIN ? 2 : 1);
        ground_sw = -1;
    }
    if (ev & TE_WIN) on_escape();
    else if (ev & TE_EATEN) on_caught();
}

/* rewind: to just before the move that got you caught, or a second back
 * if you were standing still */
static void undo(void) {
    int dead_i = hist_n - 1;
    int k = dead_i - 1;
    while (k > 0 && hist_act[k] == ACT_WAIT) k--;
    if (dead_i - k > 8) k = dead_i - 8;
    if (k < 0) k = 0;
    /* never land in the middle of a colour change */
    while (k > 0 && hist[k].camo_t) k--;
    hist_n = k + 1;
    S = hist[k];
    prev_S = S;
    anim_t = 99;
    beat_t = 0;
    queued = -1;
    state = S_PLAY;
    state_t = 0;
    ground_sw = -1;
    sfx_play_name("tn_undo");
    music_play(cur == TN_LEVELS - 1 ? TN_MUS_GATE : TN_MUS_LEVEL);
}

static void update_play(void) {
    if (anim_t < 99) anim_t++;
    if (intro_t > 0) {
        intro_t--;
        if (intro_t < 50 && (btnp(BTN_A) || btnp(BTN_UP) || btnp(BTN_DOWN) || btnp(BTN_LEFT) || btnp(BTN_RIGHT))) intro_t = 0;
        return;
    }
    if (btnp(BTN_SELECT)) {
        state = S_MENU;
        menu_sel = 0;
        sfx_play_name("ui_pause");
        return;
    }
    if (btn(BTN_B)) queued = -1;
    else if (btnp(BTN_UP)) queued = ACT_UP;
    else if (btnp(BTN_DOWN)) queued = ACT_DOWN;
    else if (btnp(BTN_LEFT)) queued = ACT_LEFT;
    else if (btnp(BTN_RIGHT)) queued = ACT_RIGHT;
    if (btnp(BTN_A) && !btn(BTN_B)) queued = ACT_CAMO;
    if (++beat_t >= TN_BEAT) {
        beat_t = 0;
        do_beat();
    }
}

static void update_menu(void) {
    if (btn_repeat(BTN_UP)) { menu_sel = (menu_sel + 2) % 3; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { menu_sel = (menu_sel + 1) % 3; sfx_play_name("ui_move"); }
    if (btnp(BTN_B) || btnp(BTN_SELECT)) { state = S_PLAY; sfx_play_name("ui_back"); input_consume(); return; }
    if (btnp(BTN_A)) {
        sfx_play_name("ui_ok");
        if (menu_sel == 0) { state = S_PLAY; input_consume(); }
        else if (menu_sel == 1) enter_level(cur);
        else go_map();
    }
}

static void update_caught(void) {
    caught_t++;
    if (caught_t == 22) sfx_play_name(falcon ? "tn_swoop" : "tn_gulp");
    if (caught_t == 48) { sfx_play_name("tn_gulp"); music_restart(TN_MUS_EATEN); }
    if (caught_t < 60) return;
    if (btn_repeat(BTN_UP)) { menu_sel = (menu_sel + 2) % 3; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { menu_sel = (menu_sel + 1) % 3; sfx_play_name("ui_move"); }
    if (btnp(BTN_A)) {
        if (menu_sel == 0) undo();
        else if (menu_sel == 1) { sfx_play_name("ui_ok"); enter_level(cur); }
        else { sfx_play_name("ui_ok"); go_map(); }
    }
}

static void update_escape(void) {
    escape_t++;
    if (anim_t < 99) anim_t++;
    if (escape_t > 70 && (btnp(BTN_A) || escape_t > 240)) {
        if (cur == TN_LEVELS - 1) {
            state = S_ENDING;
            state_t = 0;
            music_play(TN_MUS_END);
        } else {
            go_map();
        }
    }
}

/* ------------------------------------------------------------------ */
/* the island map                                                       */

static const int16_t NODE_X[TN_LEVELS] = {34, 58, 80, 70, 98, 124, 116, 146, 170, 160, 190, 214, 238, 262, 288};
static const int16_t NODE_Y[TN_LEVELS] = {104, 122, 106, 82, 72, 88, 116, 130, 112, 86, 70, 92, 114, 94, 66};

static void update_map(void) {
    int n = sv.cursor;
    if (!is_open(n)) n = 0;
    /* along the trails: to the next or the previous open level */
    if (btn_repeat(BTN_RIGHT) || btn_repeat(BTN_UP)) {
        for (int k = n + 1; k < TN_LEVELS; k++)
            if (is_open(k)) { n = k; sfx_play_name("tn_node"); break; }
    }
    if (btn_repeat(BTN_LEFT) || btn_repeat(BTN_DOWN)) {
        for (int k = n - 1; k >= 0; k--)
            if (is_open(k)) { n = k; sfx_play_name("tn_node"); break; }
    }
    sv.cursor = (uint8_t)n;
    if (btnp(BTN_A) && state_t > 10) { sfx_play_name("ui_ok"); save_now(); enter_level(n); return; }
    if (btnp(BTN_B)) { sfx_play_name("ui_back"); state = S_TITLE; state_t = 0; game_set_pausable(false); }
}

/* ------------------------------------------------------------------ */
/* update                                                               */

static void tn_update(void) {
    frame_t++;
    state_t++;
    if (shake_t > 0) shake_t--;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        if (p->kind == 0) p->vy += 0.06f;
    }
    switch (state) {
    case S_TITLE:
        game_set_pausable(false);
        if (btnp(BTN_B) && state_t > 10) { game_exit_to_library(); break; }
        if (state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) {
            sfx_play_name("ui_ok");
            input_consume();
            if (!sv.seen_intro) { sv.seen_intro = 1; save_now(); state = S_STORY; state_t = 0; }
            else go_map();
        }
        break;
    case S_STORY:
        if (state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); go_map(); }
        break;
    case S_MAP: update_map(); break;
    case S_PLAY: update_play(); break;
    case S_MENU: update_menu(); break;
    case S_CAUGHT: update_caught(); break;
    case S_ESCAPE: update_escape(); break;
    case S_ENDING:
        if (state_t > 200 && (btnp(BTN_A) || btnp(BTN_START))) { sv.cursor = TN_LEVELS - 1; go_map(); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing: ground                                                      */

/* each colour's ramp: main, light, dark (the chameleon wears the same) */
static const uint8_t RAMP[TC_COUNT][3] = {
    {C_WHITE, C_LIGHT, C_GREY},
    {C_JADE, C_LEAF, C_FOREST},
    {C_YELLOW, C_AMBER, C_EARTH},
    {C_NAVY, C_BLUE, C_NIGHT},
    {C_TAN, C_EARTH, C_BROWN},
};

static uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)(x * 73856093u) ^ (uint32_t)(y * 19349663u);
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}

static void draw_grass(int px, int py, int x, int y, bool tall) {
    gfx_rect(px, py, TS, TS, C_JADE);
    uint32_t h = hash2(x, y);
    for (int i = 0; i < 6; i++) {
        int bx = px + (int)((h >> (i * 3)) % 14) + 1, by = py + (int)((h >> (i * 2 + 5)) % 12) + 3;
        gfx_vline(bx, by - (tall ? 3 : 2), by, C_LEAF);
        gfx_pset(bx + 1, by, C_FOREST);
    }
    if (tall) { gfx_pset(px + 4, py + 3, C_LIME); gfx_pset(px + 11, py + 9, C_LIME); }
}

static void draw_dry(int px, int py, int x, int y) {
    gfx_rect(px, py, TS, TS, C_YELLOW);
    gfx_dither(px, py, TS, TS, C_AMBER, 3);
    uint32_t h = hash2(x, y);
    for (int i = 0; i < 6; i++) {
        int bx = px + (int)((h >> (i * 3)) % 14) + 1, by = py + (int)((h >> (i * 2 + 5)) % 12) + 3;
        gfx_line(bx, by, bx + 1, by - 3, C_EARTH);
        gfx_pset(bx + 2, by - 1, C_AMBER);
    }
}

static void draw_sand(int px, int py, int x, int y) {
    gfx_rect(px, py, TS, TS, C_YELLOW);
    gfx_dither(px, py, TS, TS, C_AMBER, 2);
    uint32_t h = hash2(x, y);
    gfx_pset(px + (int)(h % 14) + 1, py + (int)((h >> 4) % 14) + 1, C_CREAM);
    gfx_pset(px + (int)((h >> 8) % 14) + 1, py + (int)((h >> 12) % 14) + 1, C_EARTH);
    if (h % 5 == 0) { gfx_hline(px + 3, px + 6, py + 11, C_AMBER); gfx_hline(px + 5, px + 8, py + 12, C_CREAM); }
}

static void draw_swamp(int px, int py, int x, int y) {
    gfx_rect(px, py, TS, TS, C_NAVY);
    gfx_dither(px, py, TS, TS, C_NIGHT, 3);
    uint32_t h = hash2(x, y);
    gfx_hline(px + 2 + (int)(h % 5), px + 7 + (int)(h % 5), py + 4 + (int)((h >> 3) % 3), C_BLUE);
    gfx_hline(px + 7 + (int)((h >> 5) % 4), px + 11 + (int)((h >> 5) % 4), py + 10 + (int)((h >> 7) % 3), C_BLUE);
    if (h % 4 == 0) { gfx_vline(px + 12, py + 2, py + 6, C_TEAL); gfx_pset(px + 12, py + 1, C_JADE); }
}

static void draw_rock(int px, int py, int x, int y) {
    gfx_rect(px, py, TS, TS, C_TAN);
    uint32_t h = hash2(x, y);
    gfx_dither(px, py, TS, TS, C_BROWN, 2);
    int cx = px + 3 + (int)(h % 9), cy = py + 3 + (int)((h >> 4) % 9);
    gfx_line(cx, cy, cx + 4, cy + 2, C_BROWN);
    gfx_line(cx + 4, cy + 2, cx + 3, cy + 6, C_BROWN);
    gfx_hline(px + 1, px + 5, py + 1, C_EARTH);
    gfx_pset(px + 10 + (int)((h >> 9) % 4), py + 12, C_EARTH);
}

static void draw_sea(int px, int py, int x, int y, int t) {
    gfx_rect(px, py, TS, TS, C_BLUE);
    gfx_dither(px, py, TS, TS, C_SKY, 5);
    int k = (int)(hash2(x, y) % 16);
    if (((t / 12 + k) % 8) < 4) gfx_hline(px + 2 + k % 6, px + 7 + k % 6, py + 5 + k % 7, C_CYAN);
}

static bool land(int x, int y) {
    if (x < 0 || y < 0 || x >= TN_W || y >= TN_H) return false;
    return L.kind[y][x] != TN_SEA;
}

static void draw_ground_tile(int x, int y, int px, int py) {
    int k = L.kind[y][x];
    switch (k) {
    case TN_GRASS: draw_grass(px, py, x, y, false); break;
    case TN_SAND: draw_sand(px, py, x, y); break;
    case TN_SWAMP: draw_swamp(px, py, x, y); break;
    case TN_ROCK: draw_rock(px, py, x, y); break;
    case TN_SWITCHGRASS:
        if (tn_colour(&L, &S, x, y) == TC_GRASS) draw_grass(px, py, x, y, true);
        else draw_dry(px, py, x, y);
        break;
    case TN_SEA:
        draw_sea(px, py, x, y, 0);
        /* surf where the sea meets land */
        if (land(x, y - 1)) gfx_hline(px, px + TS - 1, py, C_ICE);
        if (land(x, y + 1)) gfx_hline(px, px + TS - 1, py + TS - 1, C_ICE);
        if (land(x - 1, y)) gfx_vline(px, py, py + TS - 1, C_ICE);
        if (land(x + 1, y)) gfx_vline(px + TS - 1, py, py + TS - 1, C_ICE);
        break;
    default: {
        /* the ground under a palm, boulder, bush, log or burrow: borrow a neighbour's */
        int nb = TN_SAND;
        static const int8_t ox[4] = {-1, 1, 0, 0}, oy[4] = {0, 0, -1, 1};
        for (int i = 0; i < 4; i++) {
            int nx = x + ox[i], ny = y + oy[i];
            if (nx >= 0 && ny >= 0 && nx < TN_W && ny < TN_H && L.kind[ny][nx] <= TN_ROCK) { nb = L.kind[ny][nx]; break; }
        }
        if (nb == TN_GRASS) draw_grass(px, py, x, y, false);
        else if (nb == TN_SWAMP) draw_swamp(px, py, x, y);
        else if (nb == TN_ROCK) draw_rock(px, py, x, y);
        else draw_sand(px, py, x, y);
        break;
    }
    }
    switch (k) {
    case TN_PALM: gfx_dither(px + 2, py + 3, 13, 12, C_INK, 6); spr_draw(&tn_spr[TS_PALM], px, py - 1, (x + y) % 2 ? SPR_FLIPX : 0); break;
    case TN_BOULDER: spr_draw(&tn_spr[TS_BOULDER], px, py, 0); break;
    case TN_BUSH: spr_draw(&tn_spr[TS_BUSH], px, py, (x * 3 + y) % 2 ? SPR_FLIPX : 0); break;
    case TN_LOG_H: {
        bool l = x > 0 && L.kind[y][x - 1] == TN_LOG_H, r = x < TN_W - 1 && L.kind[y][x + 1] == TN_LOG_H;
        gfx_rect(px, py + 2, TS, 12, C_BROWN);
        gfx_rect(px, py + 3, TS, 4, C_TAN);
        gfx_hline(px, px + TS - 1, py + 2, C_INK);
        gfx_hline(px, px + TS - 1, py + 13, C_INK);
        gfx_hline(px + (x % 3) * 4, px + (x % 3) * 4 + 5, py + 9, C_TAN);
        if (!l) { gfx_rect(px, py + 3, 4, 10, C_EARTH); gfx_rect(px + 1, py + 5, 2, 6, C_INK); }
        if (!r) { gfx_rect(px + 12, py + 3, 4, 10, C_EARTH); gfx_rect(px + 13, py + 5, 2, 6, C_INK); }
        break;
    }
    case TN_LOG_V: {
        bool u = y > 0 && L.kind[y - 1][x] == TN_LOG_V, d = y < TN_H - 1 && L.kind[y + 1][x] == TN_LOG_V;
        gfx_rect(px + 2, py, 12, TS, C_BROWN);
        gfx_rect(px + 3, py, 4, TS, C_TAN);
        gfx_vline(px + 2, py, py + TS - 1, C_INK);
        gfx_vline(px + 13, py, py + TS - 1, C_INK);
        gfx_vline(px + 9, py + (y % 3) * 4, py + (y % 3) * 4 + 5, C_TAN);
        if (!u) { gfx_rect(px + 3, py, 10, 4, C_EARTH); gfx_rect(px + 5, py + 1, 6, 2, C_INK); }
        if (!d) { gfx_rect(px + 3, py + 12, 10, 4, C_EARTH); gfx_rect(px + 5, py + 13, 6, 2, C_INK); }
        break;
    }
    case TN_HOLE:
        gfx_circ(px + 8, py + 9, 7, C_BROWN);
        gfx_circ(px + 8, py + 9, 6, C_EARTH);
        gfx_circ(px + 8, py + 10, 5, C_NIGHT);
        gfx_circ(px + 8, py + 11, 3, C_INK);
        gfx_hline(px + 4, px + 12, py + 3, C_HIDE);
        break;
    default: break;
    }
    for (int i = 0; i < L.nrain; i++)
        if (L.rain_x[i] == x && L.rain_y[i] == y) spr_draw(&tn_spr[TS_RAIN], px, py, 0);
    for (int i = 0; i < L.nsun; i++)
        if (L.sun_x[i] == x && L.sun_y[i] == y) spr_draw(&tn_spr[TS_SUN], px, py, 0);
}

static void build_ground(void) {
    gfx_set_target(&ground);
    gfx_camera(0, 0);
    gfx_noclip();
    for (int y = 0; y < TN_H; y++)
        for (int x = 0; x < TN_W; x++) draw_ground_tile(x, y, x * TS, y * TS);
    gfx_set_target(NULL);
    gfx_noclip();
    ground_sw = S.sw;
}

/* ------------------------------------------------------------------ */
/* drawing: creatures                                                   */

static float lerp_t(void) { return anim_t >= TN_BEAT ? 1.0f : (float)anim_t / TN_BEAT; }

static void stork_draw_pos(int i, int *ox, int *oy, int *dir, bool *walking) {
    int x0, y0, x1, y1, d;
    uint32_t b = S.beat;
    tn_stork_at(&L, i, b, &x1, &y1, &d);
    tn_stork_at(&L, i, b ? b - 1 : 0, &x0, &y0, NULL);
    float t = state == S_PLAY || state == S_ESCAPE ? lerp_t() : 1.0f;
    *ox = (int)((x0 + (x1 - x0) * t) * TS);
    *oy = OY + (int)((y0 + (y1 - y0) * t) * TS);
    *dir = d;
    *walking = (x0 != x1 || y0 != y1) && t < 1.0f;
}

static void draw_facing(int base_r, int base_u, int base_d, int dir, int x, int y, const uint8_t *remap) {
    if (dir == DIR_UP) spr_draw_ex(&tn_spr[base_u], x, y, 0, remap, -1);
    else if (dir == DIR_DOWN) spr_draw_ex(&tn_spr[base_d], x, y, 0, remap, -1);
    else spr_draw_ex(&tn_spr[base_r], x, y, dir == DIR_LEFT ? SPR_FLIPX : 0, remap, -1);
}

/* Twig's colours: the ramp of her camouflage; hidden, even her outline blends */
static void twig_remap(uint8_t *map, int camo, bool hidden, bool flicker) {
    pal_identity(map);
    int c = flicker ? (frame_t / 3) % TC_COUNT : camo;
    pal_swap(map, C_WHITE, RAMP[c][0]);
    pal_swap(map, C_LIGHT, RAMP[c][1]);
    pal_swap(map, C_GREY, RAMP[c][2]);
    if (hidden) pal_swap(map, C_INK, RAMP[c][2]);
}

static void draw_twig(void) {
    if (state == S_CAUGHT && !S.baby_eaten && caught_t > 40) return;
    float t = state == S_PLAY || state == S_ESCAPE || state == S_MENU ? lerp_t() : 1.0f;
    int fx = S.x, fy = S.y, bx0 = prev_S.x, by0 = prev_S.y;
    if (state == S_MENU || state == S_CAUGHT) { bx0 = fx; by0 = fy; }
    float x = bx0 + (fx - bx0) * t, y = by0 + (fy - by0) * t;
    int px = (int)(x * TS), py = OY + (int)(y * TS);
    bool in_log = L.kind[S.y][S.x] == TN_LOG_H || L.kind[S.y][S.x] == TN_LOG_V;
    bool hidden = tn_hidden(&L, &S, 0) && !S.dead;
    uint8_t map[PAL_COUNT];
    twig_remap(map, S.camo_t ? TC_NONE : S.camo, hidden, S.camo_t > 0);
    bool walk = (fx != bx0 || fy != by0) && t < 1.0f;
    int frame = walk && (int)(t * 4) % 2;
    if (state == S_ESCAPE && escape_t > 6) {
        /* into the burrow */
        int shrink = imin(escape_t - 6, 16);
        if (shrink >= 14) return;
        gfx_clip(px, py, TS, TS - shrink / 2);
        py += shrink / 2;
    }
    if (in_log) {
        /* only the tip of the tail shows at the log's ends */
        if ((frame_t / 20) % 2) gfx_pset(px + 8, py + 8, C_CREAM);
        gfx_noclip();
        return;
    }
    draw_facing(frame ? TS_TWIG_R2 : TS_TWIG_R, frame ? TS_TWIG_U2 : TS_TWIG_U, frame ? TS_TWIG_D2 : TS_TWIG_D, S.face, px, py, map);
    if (hidden && (frame_t / 40) % 3 == 0) gfx_pset(px + 13 - (S.face == DIR_LEFT ? 11 : 0), py + 5, C_WHITE);
    gfx_noclip();
}

static void draw_baby_at(int x, int y, int face, int camo, bool hidden, int bob) {
    uint8_t map[PAL_COUNT];
    twig_remap(map, camo, hidden, false);
    int px = x + 3, py = y + 3 - bob;
    if (face == DIR_UP) spr_draw_ex(&tn_spr[TS_BABY_U], px, py, 0, map, -1);
    else if (face == DIR_DOWN) spr_draw_ex(&tn_spr[TS_BABY_D], px, py, 0, map, -1);
    else spr_draw_ex(&tn_spr[TS_BABY_R], px, py, face == DIR_LEFT ? SPR_FLIPX : 0, map, -1);
}

static void draw_baby(void) {
    if (L.baby_x == TN_NONE) return;
    if (!S.baby) {
        /* waiting, and a little anxious */
        int bob = (frame_t / 10) % 4 == 0;
        draw_baby_at(L.baby_x * TS, OY + L.baby_y * TS, (frame_t / 60) % 2 ? DIR_LEFT : DIR_RIGHT, TC_NONE, false, bob);
        if ((frame_t / 30) % 2) text_draw("?", L.baby_x * TS + 10, OY + L.baby_y * TS - 6, C_CREAM);
        return;
    }
    if (state == S_CAUGHT && S.baby_eaten && caught_t > 40) return;
    if (state == S_ESCAPE && escape_t > 12) return;
    float t = state == S_PLAY || state == S_ESCAPE ? lerp_t() : 1.0f;
    int x0 = prev_S.baby ? prev_S.bx : prev_S.x, y0 = prev_S.baby ? prev_S.by : prev_S.y;
    if (state == S_MENU || state == S_CAUGHT) { x0 = S.bx; y0 = S.by; }
    float x = x0 + (S.bx - x0) * t, y = y0 + (S.by - y0) * t;
    int k = L.kind[S.by][S.bx];
    if (k == TN_LOG_H || k == TN_LOG_V) return;
    int face = S.x > S.bx ? DIR_RIGHT : S.x < S.bx ? DIR_LEFT : S.y < S.by ? DIR_UP : DIR_DOWN;
    uint8_t map[PAL_COUNT];
    twig_remap(map, S.bcamo, tn_hidden(&L, &S, 1) && !S.dead, S.camo_t > 0);
    int bxp = (int)(x * TS) + 3, byp = OY + (int)(y * TS) + 3;
    if (face == DIR_UP) spr_draw_ex(&tn_spr[TS_BABY_U], bxp, byp, 0, map, -1);
    else if (face == DIR_DOWN) spr_draw_ex(&tn_spr[TS_BABY_D], bxp, byp, 0, map, -1);
    else spr_draw_ex(&tn_spr[TS_BABY_R], bxp, byp, face == DIR_LEFT ? SPR_FLIPX : 0, map, -1);
}

static void draw_toads(void) {
    for (int i = 0; i < L.ntoad; i++) {
        int x = L.toad_x[i] * TS, y = OY + L.toad_y[i] * TS, d = L.toad_dir[i];
        bool blink = (frame_t + i * 37) % 180 < 8;
        int lunge = 0;
        if (state == S_CAUGHT && S.eaten_by == 0 && S.eater == i && !falcon && caught_t > 20 && caught_t < 50) lunge = 2;
        static const int8_t DXs[4] = {0, 1, 0, -1}, DYs[4] = {-1, 0, 1, 0};
        x += DXs[d] * lunge;
        y += DYs[d] * lunge;
        if (blink) draw_facing(TS_TOAD_BLINK, TS_TOAD_BLINK_U, TS_TOAD_BLINK_D, d, x, y, NULL);
        else draw_facing(TS_TOAD_R, TS_TOAD_U, TS_TOAD_D, d, x, y, NULL);
        /* the throat puffs now and then */
        if ((frame_t / 6 + i * 5) % 40 < 4) gfx_circ(x + 8 - DXs[d] * 2, y + 8 - DYs[d] * 2, 2, C_PINK);
    }
}

static void draw_storks(void) {
    for (int i = 0; i < L.nstork; i++) {
        int x, y, d;
        bool walking;
        stork_draw_pos(i, &x, &y, &d, &walking);
        int step = walking || (frame_t / 16 + i) % 4 == 0;
        /* a shadow, then the bird */
        gfx_dither(x + 3, y + 6, 12, 8, C_INK, 5);
        draw_facing(step ? TS_STORK_R2 : TS_STORK_R, step ? TS_STORK_U2 : TS_STORK_U, step ? TS_STORK_D2 : TS_STORK_D, d, x, y - 2, NULL);
    }
}

static void draw_danger(void) {
    uint8_t dn[TN_H][TN_W];
    tn_danger(&L, &S, S.beat, dn);
    int lvl = 7;
    for (int y = 0; y < TN_H; y++)
        for (int x = 0; x < TN_W; x++) {
            if (!dn[y][x]) continue;
            int px = x * TS, py = OY + y * TS;
            gfx_dither(px, py, TS, TS, C_PINK, lvl);
            if (y == 0 || !dn[y - 1][x]) gfx_hline(px, px + TS - 1, py, C_MAGENTA);
            if (y == TN_H - 1 || !dn[y + 1][x]) gfx_hline(px, px + TS - 1, py + TS - 1, C_MAGENTA);
            if (x == 0 || !dn[y][x - 1]) gfx_vline(px, py, py + TS - 1, C_MAGENTA);
            if (x == TN_W - 1 || !dn[y][x + 1]) gfx_vline(px + TS - 1, py, py + TS - 1, C_MAGENTA);
        }
}

static void draw_caught_fx(void) {
    int tx = S.baby_eaten ? S.bx : S.x, ty = S.baby_eaten ? S.by : S.y;
    int cx = tx * TS + 8, cy = OY + ty * TS + 8;
    int px, py;
    if (S.eaten_by == 1) { tn_stork_at(&L, S.eater, S.beat, &px, &py, NULL); }
    else { px = L.toad_x[S.eater]; py = L.toad_y[S.eater]; }
    int sx = px * TS + 8, sy = OY + py * TS + 2;
    if (caught_t < 22) {
        /* spotted! */
        if ((caught_t / 3) % 2 == 0) {
            gfx_rect(sx - 3, sy - 14, 7, 11, C_INK);
            text_draw("!", sx - 1, sy - 13, C_YELLOW);
        }
        return;
    }
    if (falcon) {
        float f = fminf(1.0f, (caught_t - 22) / 24.0f);
        int fx = (int)(-30 + (cx - 12 + 30) * f), fy = (int)(-10 + (cy - 10 + 10) * f);
        if (caught_t > 46) { fx = cx - 12 + (caught_t - 46) * 4; fy = cy - 10 - (caught_t - 46) * 3; }
        spr_draw(&tn_spr[(frame_t / 4) % 2 ? TS_FALCON : TS_FALCON2], fx, fy, 0);
    } else if (S.eaten_by == 0 && caught_t < 50) {
        /* the toad's tongue */
        float f = caught_t < 36 ? (caught_t - 22) / 14.0f : (50 - caught_t) / 14.0f;
        int ex = sx + (int)((cx - sx) * f), ey = sy + 6 + (int)((cy - sy - 6) * f);
        gfx_line(sx, sy + 6, ex, ey, C_PINK);
        gfx_line(sx + 1, sy + 6, ex + 1, ey, C_MAGENTA);
        gfx_circ(ex, ey, 2, C_PINK);
    } else if (S.eaten_by == 1 && caught_t < 50) {
        /* the stork's bill snaps */
        if ((caught_t / 4) % 2) { gfx_line(sx, sy + 6, cx, cy, C_RED); gfx_line(sx + 1, sy + 6, cx + 1, cy, C_ORANGE); }
    }
}

/* ------------------------------------------------------------------ */
/* drawing: the level screen                                            */

/* an eye: open when that one is in view, shut when hidden */
static void draw_eye(int x, int y, bool seen) {
    if (seen) {
        gfx_rect(x, y + 1, 9, 5, C_WHITE);
        gfx_rect(x + 1, y, 7, 7, C_WHITE);
        gfx_rect(x + 3, y + 2, 3, 3, C_INK);
    } else {
        gfx_hline(x, x + 8, y + 3, C_SLATE);
        gfx_hline(x + 1, x + 7, y + 4, C_SLATE);
    }
}

static void draw_hud(void) {
    gfx_rect(0, HUD_Y, SCREEN_W, SCREEN_H - HUD_Y, C_INK);
    gfx_hline(0, SCREEN_W - 1, HUD_Y, C_DUSK);
    char buf[48];
    snprintf(buf, sizeof buf, "%d", cur + 1);
    gfx_rect(3, HUD_Y + 4, 14, 12, C_FOREST);
    gfx_rectb(3, HUD_Y + 4, 14, 12, C_LEAF);
    text_center(buf, 10, HUD_Y + 7, C_WHITE);
    text_draw(TN_LEVEL_DEFS[cur].name, 22, HUD_Y + 6, C_CREAM);
    /* who can be seen */
    bool alive = state == S_PLAY || state == S_MENU;
    draw_eye(186, HUD_Y + 6, alive && !tn_hidden(&L, &S, 0));
    if (S.baby) draw_eye(200, HUD_Y + 6, alive && !tn_hidden(&L, &S, 1));
    int x = 232;
    for (int i = 0; i < L.nfruit; i++) {
        bool got = (S.fruit >> i) & 1;
        if (got) spr_draw(&tn_spr[TS_PEAR], x, HUD_Y + 4, 0);
        else spr_draw_ex(&tn_spr[TS_PEAR], x, HUD_Y + 4, 0, NULL, C_DUSK);
        x += 11;
    }
    if (L.baby_x != TN_NONE) {
        uint8_t map[PAL_COUNT];
        twig_remap(map, S.baby ? TC_GRASS : TC_NONE, false, false);
        if (S.baby) spr_draw_ex(&tn_spr[TS_BABY_R], x, HUD_Y + 5, 0, map, -1);
        else spr_draw_ex(&tn_spr[TS_BABY_R], x, HUD_Y + 5, 0, NULL, C_DUSK);
    }
}

static void draw_level(void) {
    if (ground_sw != S.sw) build_ground();
    int sx = shake_t > 0 ? ((shake_t / 2) % 2 ? 2 : -2) : 0;
    /* the ground, with the sea's shimmer on top */
    for (int y = 0; y < TN_H * TS; y++) memcpy(g_screen.px + (OY + y) * SCREEN_W, ground_px + y * TN_W * TS, TN_W * TS);
    gfx_camera(-sx, 0);
    for (int y = 0; y < TN_H; y++)
        for (int x = 0; x < TN_W; x++)
            if (L.kind[y][x] == TN_SEA) {
                int k = (int)(hash2(x, y) % 16);
                if (((frame_t / 12 + k) % 8) < 3) gfx_hline(x * TS + 2 + k % 6, x * TS + 7 + k % 6, OY + y * TS + 5 + k % 7, C_CYAN);
            }
    if (btn(BTN_B) && (state == S_PLAY || state == S_MENU)) draw_danger();
    /* pears */
    for (int i = 0; i < L.nfruit; i++) {
        if ((S.fruit >> i) & 1) continue;
        int bob = (frame_t / 12 + i * 3) % 4 == 0;
        spr_draw(&tn_spr[TS_PEAR], L.fruit_x[i] * TS + 4, OY + L.fruit_y[i] * TS + 3 - bob, 0);
    }
    draw_toads();
    draw_baby();
    draw_twig();
    draw_storks();
    if (state == S_CAUGHT) draw_caught_fx();
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        if (p->kind == 2) gfx_line((int)p->x, (int)p->y, (int)(p->x - p->vx), (int)(p->y - p->vy * 1.5f), p->col);
        else gfx_rect((int)p->x, (int)p->y, p->kind == 1 ? 1 : 2, p->kind == 1 ? 1 : 2, p->col);
    }
    gfx_camera(0, 0);
    draw_hud();
    if (state == S_PLAY && intro_t > 0) {
        int y = 72 - (intro_t > 60 ? (intro_t - 60) * 6 : 0);
        gfx_rect(0, y, SCREEN_W, 36, C_INK);
        gfx_hline(0, SCREEN_W - 1, y, C_LEAF);
        gfx_hline(0, SCREEN_W - 1, y + 35, C_LEAF);
        char buf[32];
        snprintf(buf, sizeof buf, "LEVEL %d", cur + 1);
        tiny_center(buf, 160, y + 5, C_LIME);
        static const uint8_t grad[] = {C_WHITE, C_LIME, C_LEAF};
        ui_fancy_center(TN_LEVEL_DEFS[cur].name, 160, y + 13, 1, grad, 3, C_INK, C_FOREST);
    }
}

static void draw_menu_panel(const char *title, const char *const *items, int n, int sel) {
    int h = 26 + n * 13;
    int y = 90 - h / 2;
    ui_panel(96, y, 128, h, C_NIGHT, C_LEAF);
    text_center(title, 160, y + 6, C_LIME);
    for (int i = 0; i < n; i++) {
        int yy = y + 20 + i * 13;
        if (i == sel) gfx_rect(102, yy - 3, 116, 13, C_DUSK);
        text_draw(items[i], 118, yy, i == sel ? C_WHITE : C_GREY);
        if (i == sel) ui_cursor(106, yy, frame_t);
    }
}

static void draw_play(void) {
    draw_level();
    if (state == S_MENU) {
        gfx_darken_rect(0, OY, SCREEN_W, SCREEN_H - OY, 2);
        static const char *const M[3] = {"KEEP GOING", "RESTART LEVEL", "ISLAND MAP"};
        draw_menu_panel("SELECT", M, 3, menu_sel);
    } else if (state == S_CAUGHT && caught_t >= 60) {
        gfx_darken_rect(0, OY, SCREEN_W, SCREEN_H - OY, 1);
        static const char *const M[3] = {"UNDO", "RESTART LEVEL", "ISLAND MAP"};
        draw_menu_panel(S.baby_eaten ? "THE HATCHLING!" : "GULP!", M, 3, menu_sel);
    } else if (state == S_ESCAPE && escape_t > 16) {
        int n = tn_collected(&S), need = L.nfruit + (L.baby_x != TN_NONE);
        gfx_rect(0, 70, SCREEN_W, 40, C_INK);
        gfx_hline(0, SCREEN_W - 1, 70, C_YELLOW);
        gfx_hline(0, SCREEN_W - 1, 109, C_YELLOW);
        static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
        ui_fancy_center("SAFE!", 160, 74, 2, grad, 4, C_INK, C_BROWN);
        char buf[48];
        if (need) {
            snprintf(buf, sizeof buf, "%d / %d  " GLYPH_DOT "  BEST %d / %d", n, need, sv.best[cur], need);
            text_center(buf, 160, 96, n == need ? C_YELLOW : C_CREAM);
        } else {
            text_center("THE SUN GATE!", 160, 96, C_YELLOW);
        }
    }
}

/* ------------------------------------------------------------------ */
/* drawing: map, title, story, ending                                   */

static void ellipse(int cx, int cy, int rx, int ry, int col) {
    for (int y = -ry; y <= ry; y++) {
        float f = 1.0f - (float)(y * y) / (float)(ry * ry);
        int w = (int)(rx * sqrtf(f > 0 ? f : 0));
        gfx_hline(cx - w, cx + w, cy + y, col);
    }
}

static void draw_island(int t) {
    gfx_cls(C_BLUE);
    for (int y = 0; y < SCREEN_H; y += 6)
        for (int x = (y / 6) % 2 * 10; x < SCREEN_W; x += 20)
            if (((t / 20) + x / 20 + y / 6) % 3 == 0) gfx_hline(x, x + 5, y, C_SKY);
    /* the island: overlapping blobs, a beach rim, then grass and hills */
    static const int16_t B[][4] = {{60, 100, 58, 34}, {120, 102, 50, 38}, {180, 98, 52, 36}, {240, 96, 48, 38},
                                   {286, 80, 30, 26}, {40, 116, 34, 22}, {150, 124, 46, 20}};
    for (int i = 0; i < ARRAY_LEN(B); i++) ellipse(B[i][0], B[i][1], B[i][2] + 4, B[i][3] + 4, C_ICE);
    for (int i = 0; i < ARRAY_LEN(B); i++) ellipse(B[i][0], B[i][1], B[i][2], B[i][3], C_YELLOW);
    for (int i = 0; i < ARRAY_LEN(B); i++) ellipse(B[i][0], B[i][1] - 2, B[i][2] - 8, B[i][3] - 8, C_JADE);
    gfx_dither(20, 60, 290, 90, C_LEAF, 2);
    ellipse(110, 110, 18, 8, C_NAVY);
    ellipse(110, 109, 14, 5, C_BLUE);
    ellipse(200, 84, 22, 10, C_TAN);
    ellipse(200, 82, 16, 6, C_EARTH);
    ellipse(236, 110, 16, 7, C_HIDE);
    /* the Sun Gate on the eastern cape */
    int gx = 288, gy = 50;
    gfx_rect(gx - 12, gy, 24, 16, C_LIGHT);
    gfx_rect(gx - 9, gy - 6, 18, 6, C_WHITE);
    gfx_rect(gx - 4, gy + 5, 8, 11, C_INK);
    gfx_circ(gx, gy - 12, 5, C_YELLOW);
    gfx_circb(gx, gy - 12, 7 + (t / 20) % 2, C_AMBER);
    static const int16_t TREE[][2] = {{22, 92}, {50, 84}, {92, 128}, {136, 76}, {176, 128}, {214, 74}, {254, 124}, {268, 86}};
    for (int i = 0; i < ARRAY_LEN(TREE); i++) {
        int ex = TREE[i][0], ey = TREE[i][1];
        gfx_circ(ex + 1, ey + 2, 5, C_FOREST);
        gfx_circ(ex, ey, 5, C_JADE);
        gfx_circ(ex - 1, ey - 1, 3, C_LEAF);
        gfx_pset(ex + 2, ey + 1, C_RED);
    }
}

static void draw_map(void) {
    draw_island(frame_t);
    /* the trails */
    for (int a = 0; a < TN_LEVELS; a++)
        for (int b = a + 1; b < TN_LEVELS; b++) {
            if (!trail(a, b)) continue;
            int x0 = NODE_X[a], y0 = NODE_Y[a], x1 = NODE_X[b], y1 = NODE_Y[b];
            int n = imax(iabs(x1 - x0), iabs(y1 - y0)) / 4;
            for (int k = 1; k < n; k++) {
                int x = x0 + (x1 - x0) * k / n, y = y0 + (y1 - y0) * k / n;
                gfx_rect(x, y, 2, 2, beaten(a) ? C_CREAM : C_FOREST);
            }
        }
    for (int i = 0; i < TN_LEVELS; i++) {
        int x = NODE_X[i], y = NODE_Y[i];
        bool open = is_open(i), done = beaten(i);
        int need = i == TN_LEVELS - 1 ? 0 : 3;
        gfx_circ(x, y + 1, 6, C_INK);
        gfx_circ(x, y, 6, !open ? C_SLATE : done ? C_YELLOW : C_WHITE);
        gfx_circ(x, y, 4, !open ? C_DUSK : done ? C_AMBER : C_LIGHT);
        char buf[4];
        snprintf(buf, sizeof buf, "%d", i + 1);
        tiny_center(buf, x + 1, y - 2, !open ? C_SLATE : C_INK);
        if (done && need && sv.best[i] >= need) {
            /* a white star for all three in one escape */
            text_draw(GLYPH_STAR, x + 4, y - 12, C_INK);
            text_draw(GLYPH_STAR, x + 3, y - 13, C_WHITE);
        }
    }
    /* Twig on the current node */
    int c = sv.cursor;
    uint8_t map[PAL_COUNT];
    twig_remap(map, TC_GRASS, false, false);
    spr_draw_ex(&tn_spr[(frame_t / 12) % 2 ? TS_TWIG_R2 : TS_TWIG_R], NODE_X[c] - 8, NODE_Y[c] - 22 - ((frame_t / 12) % 2), 0, map, -1);
    /* the panel */
    gfx_rect(0, 0, SCREEN_W, 18, C_INK);
    gfx_hline(0, SCREEN_W - 1, 18, C_DUSK);
    static const uint8_t grad[] = {C_WHITE, C_LIME, C_LEAF};
    ui_fancy_text("SALT ISLAND", 4, 4, 1, grad, 3, C_INK, -1);
    char buf[64];
    snprintf(buf, sizeof buf, "%d%%", completion());
    text_draw(buf, 214, 5, C_YELLOW);
    ui_panel(4, 150, 312, 27, C_NIGHT, C_LEAF);
    snprintf(buf, sizeof buf, "LEVEL %d", c + 1);
    tiny_draw(buf, 12, 154, C_LIME);
    text_draw(TN_LEVEL_DEFS[c].name, 12, 163, C_CREAM);
    if (c < TN_LEVELS - 1) {
        int best = sv.best[c];
        for (int k = 0; k < 3; k++) {
            int x = 196 + k * 14;
            if (k < 2) {
                if (best > k) spr_draw(&tn_spr[TS_PEAR], x, 157, 0);
                else spr_draw_ex(&tn_spr[TS_PEAR], x, 157, 0, NULL, C_DUSK);
            } else {
                spr_draw_ex(&tn_spr[TS_BABY_R], x - 1, 158, 0, NULL, best >= 3 ? C_LIME : C_DUSK);
            }
        }
        tiny_draw(beaten(c) ? "BEST RUN" : "NOT YET", 242, 160, beaten(c) ? C_GREY : C_SLATE);
    } else {
        tiny_draw("THE LAST CLIMB", 212, 160, C_YELLOW);
    }
}

static void draw_scaled_remap(const Sprite *k, int x, int y, int scale, const uint8_t *map) {
    for (int sy = 0; sy < k->h; sy++)
        for (int sx = 0; sx < k->w; sx++) {
            uint8_t p = k->px[sy * k->w + sx];
            if (p != TRANSPARENT) gfx_rect(x + sx * scale, y + sy * scale, scale, scale, map[p]);
        }
}

static void draw_title(void) {
    for (int y = 0; y < SCREEN_H; y++) gfx_hline(0, SCREEN_W - 1, y, y < 50 ? C_PINK : y < 90 ? C_AMBER : y < 120 ? C_YELLOW : C_JADE);
    gfx_dither(0, 46, SCREEN_W, 8, C_AMBER, 8);
    gfx_dither(0, 86, SCREEN_W, 8, C_YELLOW, 8);
    gfx_dither(0, 116, SCREEN_W, 8, C_JADE, 8);
    gfx_circ(250, 96, 26, C_CREAM);
    gfx_circ(250, 96, 22, C_WHITE);
    ellipse(160, 180, 190, 56, C_FOREST);
    ellipse(160, 186, 170, 50, C_JADE);
    for (int i = 0; i < 7; i++) spr_draw(&tn_spr[TS_BUSH], 6 + i * 48, 148 + (i % 2) * 8, i % 2 ? SPR_FLIPX : 0);
    /* Twig, big, changing colour every few seconds */
    int c = (frame_t / 90) % TC_COUNT;
    uint8_t map[PAL_COUNT];
    twig_remap(map, c, false, (frame_t % 90) < 10);
    draw_scaled_remap(&tn_spr[TS_HERO], 58, 92 - ((frame_t / 30) % 2), 3, map);
    spr_draw(&tn_spr[TS_PEAR], 206, 150, 0);
    static const uint8_t grad[] = {C_WHITE, C_LIME, C_LEAF, C_JADE};
    ui_fancy_center("TINTAIL", 160, 12, 4, grad, 4, C_INK, C_FOREST);
    if (state_t > 20 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 60, C_BROWN);
    char buf[32];
    snprintf(buf, sizeof buf, "%d%% OF SALT ISLAND", completion());
    gfx_rect(100, 168, 120, 11, C_INK);
    tiny_center(buf, 160, 171, C_LIME);
}

static void draw_story(void) {
    draw_island(frame_t);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    ui_panel(16, 36, 288, 110, C_NIGHT, C_LEAF);
    text_draw("EVERY SPRING THE CHAMELEONS OF SALT\n"
              "ISLAND CLIMB TO THE SUN GATE ON THE\n"
              "EASTERN CAPE TO GREET THE FIRST SUNRISE.\n\n"
              "TWIG, THE SMALLEST, IS LATE. THE TOADS\n"
              "AND STORKS ARE HUNGRY. MATCH THE GROUND\n"
              "AND NOTHING CAN SEE YOU.", 26, 46, C_LIGHT);
    tiny_draw("PICK UP THE LOST HATCHLINGS ON THE WAY.", 26, 124, C_LIME);
    if (state_t > 20 && (state_t / 20) % 2) text_draw(GLYPH_A, 290, 132, C_WHITE);
}

static void draw_ending(void) {
    for (int y = 0; y < SCREEN_H; y++) gfx_hline(0, SCREEN_W - 1, y, y < 70 ? C_PINK : y < 110 ? C_AMBER : C_TAN);
    int rise = imin(state_t / 3, 40);
    gfx_circ(160, 110 - rise, 30, C_YELLOW);
    gfx_circ(160, 110 - rise, 24, C_CREAM);
    for (int i = 0; i < 12; i++) {
        float a = i / 12.0f * 6.2832f + frame_t * 0.01f;
        gfx_line(160 + (int)(cosf(a) * 36), 110 - rise + (int)(sinf(a) * 36), 160 + (int)(cosf(a) * 46),
                 110 - rise + (int)(sinf(a) * 46), C_YELLOW);
    }
    /* the gate's steps */
    for (int i = 0; i < 5; i++) gfx_rect(70 + i * 12, 110 + i * 14, 180 - i * 24, 14, i % 2 ? C_LIGHT : C_WHITE);
    gfx_rect(0, 170, SCREEN_W, 10, C_BROWN);
    uint8_t map[PAL_COUNT];
    twig_remap(map, (state_t / 120) % TC_COUNT, false, (state_t % 120) < 8);
    draw_scaled_remap(&tn_spr[TS_HERO], 120, 70, 2, map);
    int babies = 0;
    for (int i = 0; i < TN_LEVELS - 1; i++) babies += sv.best[i] >= 3; /* the levels escaped with everything */
    for (int i = 0; i < 14; i++) {
        bool got = i < babies;
        int x = 76 + (i % 7) * 26, y = 128 + (i / 7) * 16;
        twig_remap(map, 1 + i % 4, false, false);
        spr_draw_ex(&tn_spr[TS_BABY_U], x, y - ((frame_t / 10 + i) % 6 == 0), 0, got ? map : NULL, got ? -1 : C_EARTH);
    }
    ui_panel(40, 8, 240, 38, C_NIGHT, C_YELLOW);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center("FIRST LIGHT", 160, 12, 2, grad, 3, C_INK, C_BROWN);
    char buf[48];
    snprintf(buf, sizeof buf, "%d%% OF SALT ISLAND", completion());
    tiny_center(buf, 160, 34, C_LIGHT);
    if (state_t > 200 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 60, C_BROWN);
}

static bool sheet_mode;

static void draw_sheet(void) {
    gfx_cls(C_JADE);
    int x = 2, y = 2;
    for (int i = 0; i < TS_SPRITE_COUNT; i++) {
        if (x + tn_spr[i].w > SCREEN_W) { x = 2; y += 26; }
        spr_draw(&tn_spr[i], x, y, 0);
        x += tn_spr[i].w + 3;
    }
    for (int c = 0; c < TC_COUNT; c++) {
        uint8_t map[PAL_COUNT];
        int px = 8 + c * 40;
        gfx_rect(px, 120, 32, 32, RAMP[c][0]);
        twig_remap(map, c, c != TC_NONE, false);
        spr_draw_ex(&tn_spr[TS_TWIG_R], px + 8, 128, 0, map, -1);
        twig_remap(map, c, false, false);
        spr_draw_ex(&tn_spr[TS_TWIG_R], px + 8, 156, 0, map, -1);
    }
}

static void tn_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_STORY: draw_story(); break;
    case S_MAP: draw_map(); break;
    case S_PLAY: case S_MENU: case S_CAUGHT: case S_ESCAPE: draw_play(); break;
    case S_ENDING: draw_ending(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void tn_load(void) {
    tn_art_load();
    tn_audio_load();
}

static void tn_start(void) {
    load_save();
    memset(parts, 0, sizeof parts);
    state = S_TITLE;
    state_t = 0;
    sheet_mode = false;
    game_set_pausable(false);
    music_play(TN_MUS_TITLE);
}

static void tn_quit(void) { save_now(); }

static void tn_label(int x, int y, int w, int h, int t) {
    for (int yy = 0; yy < h; yy++) gfx_hline(x, x + w - 1, y + yy, yy < 20 ? C_AMBER : yy < 30 ? C_YELLOW : C_JADE);
    gfx_dither(x, y + 18, w, 4, C_YELLOW, 8);
    gfx_dither(x, y + 28, w, 4, C_JADE, 8);
    for (int i = 0; i < 5; i++) spr_draw(&tn_spr[TS_BUSH], x - 4 + i * 30, y + 40 + (i % 2) * 4, i % 2 ? SPR_FLIPX : 0);
    /* Twig changing colour on her branch while a toad squints */
    int c = (t / 60) % 2 ? TC_GRASS : TC_NONE;
    uint8_t map[PAL_COUNT];
    twig_remap(map, c, false, (t % 60) < 8);
    spr_draw_ex(&tn_spr[TS_HERO], x + 20, y + 22, 0, map, -1);
    spr_draw(&tn_spr[(t / 50) % 4 == 0 ? TS_TOAD_BLINK : TS_TOAD_R], x + w - 30, y + 30, SPR_FLIPX);
    spr_draw(&tn_spr[TS_PEAR], x + 66, y + 20, 0);
    static const uint8_t grad[] = {C_WHITE, C_LIME, C_LEAF};
    ui_fancy_text("TINTAIL", x + 4, y + 3, 1, grad, 3, C_INK, -1);
}

static int tn_query(const char *key, int *out) {
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "level")) { *out = cur + 1; return 1; }
    if (!strcmp(key, "x")) { *out = S.x; return 1; }
    if (!strcmp(key, "y")) { *out = S.y; return 1; }
    if (!strcmp(key, "camo")) { *out = S.camo; return 1; }
    if (!strcmp(key, "camo_t")) { *out = S.camo_t; return 1; }
    if (!strcmp(key, "dead")) { *out = S.dead; return 1; }
    if (!strcmp(key, "won")) { *out = S.won; return 1; }
    if (!strcmp(key, "beat")) { *out = (int)S.beat; return 1; }
    if (!strcmp(key, "fruit")) { *out = S.fruit; return 1; }
    if (!strcmp(key, "baby")) { *out = S.baby; return 1; }
    if (!strcmp(key, "bx")) { *out = S.bx; return 1; }
    if (!strcmp(key, "by")) { *out = S.by; return 1; }
    if (!strcmp(key, "sw")) { *out = S.sw; return 1; }
    if (!strcmp(key, "collected")) { *out = tn_collected(&S); return 1; }
    if (!strcmp(key, "hidden")) { *out = tn_hidden(&L, &S, 0); return 1; }
    if (!strcmp(key, "eaten_by")) { *out = S.eaten_by; return 1; }
    if (!strcmp(key, "baby_eaten")) { *out = S.baby_eaten; return 1; }
    if (!strcmp(key, "completion")) { *out = completion(); return 1; }
    if (!strcmp(key, "points")) { *out = points(); return 1; }
    if (!strcmp(key, "deaths")) { *out = (int)sv.deaths; return 1; }
    if (!strcmp(key, "cursor")) { *out = sv.cursor; return 1; }
    if (!strcmp(key, "unlocked")) { *out = open_count(); return 1; }
    if (!strncmp(key, "open", 4) && key[4]) { *out = is_open(atoi(key + 4) - 1); return 1; }
    if (!strcmp(key, "bcamo")) { *out = S.bcamo; return 1; }
    if (!strcmp(key, "bhidden")) { *out = tn_hidden(&L, &S, 1); return 1; }
    if (!strcmp(key, "danger")) {
        uint8_t dn[TN_H][TN_W];
        tn_danger(&L, &S, S.beat, dn);
        *out = dn[S.y][S.x];
        return 1;
    }
    if (!strncmp(key, "colour", 6) && key[6]) {
        /* colourXXYY: the colour of a tile */
        int v = atoi(key + 6);
        *out = tn_colour(&L, &S, v / 100, v % 100);
        return 1;
    }
    if (!strncmp(key, "beaten", 6) && key[6]) { *out = beaten(atoi(key + 6) - 1); return 1; }
    if (!strncmp(key, "best", 4) && key[4]) { *out = sv.best[(atoi(key + 4) - 1) % TN_LEVELS]; return 1; }
    if (!strcmp(key, "check_all")) {
        /* every level can be escaped with both pears and the hatchling */
        int ok = 0;
        for (int i = 0; i < TN_LEVELS; i++) {
            TtLevel lv;
            if (tn_parse(&TN_LEVEL_DEFS[i], &lv, NULL) == 0 && tn_solve(&lv, true, NULL, 0) > 0) ok++;
            else fprintf(stderr, "level %d cannot be escaped with everything\n", i + 1);
        }
        *out = ok;
        return 1;
    }
    return 0;
}

static int act_of(char c) {
    return c == 'U' ? ACT_UP : c == 'R' ? ACT_RIGHT : c == 'D' ? ACT_DOWN : c == 'L' ? ACT_LEFT : c == 'C' ? ACT_CAMO : ACT_WAIT;
}

static int tn_cheat(const char *cmd) {
    int a, b, c;
    if (sscanf(cmd, "level %d", &a) == 1) { enter_level(iclamp(a, 1, TN_LEVELS) - 1); intro_t = 0; return 1; }
    if (!strncmp(cmd, "act ", 4)) {
        /* play beats right now: . U R D L C */
        for (const char *p = cmd + 4; *p && state == S_PLAY; p++) {
            if (*p == ' ') continue;
            queued = act_of(*p) == ACT_WAIT ? -1 : act_of(*p);
            if (*p == '.') queued = -1;
            do_beat();
            anim_t = 99;
        }
        return 1;
    }
    if (!strcmp(cmd, "solve") || !strcmp(cmd, "autosolve") || !strcmp(cmd, "autoexit")) {
        if (tn_solve(&L, strcmp(cmd, "autoexit") != 0, auto_buf, sizeof auto_buf) < 0) return 1;
        if (cmd[0] == 's') {
            for (const char *p = auto_buf; *p && state == S_PLAY; p++) {
                queued = act_of(*p) == ACT_WAIT ? -1 : act_of(*p);
                do_beat();
            }
            anim_t = 99;
        } else {
            autoplay = auto_buf;
        }
        return 1;
    }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) { S.x = S.px = (uint8_t)a; S.y = S.py = (uint8_t)b; prev_S = S; hist[hist_n - 1] = S; return 1; }
    if (sscanf(cmd, "baby %d %d", &a, &b) == 2) { S.baby = 1; S.bx = (uint8_t)a; S.by = (uint8_t)b; prev_S = S; hist[hist_n - 1] = S; return 1; }
    if (sscanf(cmd, "camo %d", &a) == 1) { S.camo = (uint8_t)a; S.camo_t = 0; prev_S = S; hist[hist_n - 1] = S; return 1; }
    if (sscanf(cmd, "beaten %d", &a) == 1) {
        /* the first a levels beaten, nothing collected */
        sv.beaten = 0;
        for (int i = 0; i < a && i < TN_LEVELS; i++) sv.beaten |= (uint16_t)(1u << i);
        save_now();
        return 1;
    }
    if (sscanf(cmd, "best %d %d", &a, &c) == 2) { sv.best[(a - 1) % TN_LEVELS] = (uint8_t)c; save_now(); return 1; }
    if (!strcmp(cmd, "map")) { go_map(); return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    return 0;
}

const GameDef GAME_TINTAIL = {
    "tintail",
    "TINTAIL",
    "1985",
    "PUZZLE",
    "TWIG THE CHAMELEON SNEAKS ACROSS SALT ISLAND. MATCH THE GROUND TO HIDE.",
    {"REACH 30% OF SALT ISLAND", "REACH THE SUN GATE", "REACH 100% OF SALT ISLAND"},
    "D-PAD\tSTEP\n"
    GLYPH_A "\tCHANGE COLOUR\n"
    "HOLD " GLYPH_B "\tSEE WHO WATCHES\n"
    "SELECT\tRESTART / MAP\n"
    "START\tPAUSE",
    C_LEAF, C_YELLOW,
    tn_load, tn_start, tn_update, tn_draw, tn_quit, tn_label, tn_query, tn_cheat,
    "CAMOUFLAGE", 16,
};
