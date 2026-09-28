/* FLINTHOLD - hold the caves of Flint Isle against the Four Lords' beasts.
 * Cartridge 30 of UFO 40, a tribute to Rock On! Island (UFO 50 #30).
 * Every rule and where it comes from is in docs/games/30-flinthold.md.
 * This file is the cartridge: the title, the story, the island map, the
 * stages and villages on screen, the results, the ending, the save and the
 * test hooks. The stage rules themselves are in flinthold_logic.c. */
#include "flinthold.h"

enum { S_TITLE, S_STORY, S_MAP, S_STAGE, S_RESULT, S_FELL, S_END };

typedef struct Save {
    uint32_t magic;
    uint8_t done[FH_LEVELS];   /* 1 cleared, 2 cleared without the cave taking a hit */
    uint8_t best_hp[FH_LEVELS];
    uint8_t found;             /* 1 the Scale Camp, 2 Far Isle */
    uint8_t node;              /* where Pim stands on the map */
    uint8_t story_seen, won, good_end, pad;
    uint32_t cave_damage;      /* the four numbers the cartridge keeps */
    uint16_t most_units, most_hens, most_spend, pad2;
} Save;
#define SAVE_MAGIC 0x46480001u

static Save sv;
static int state, state_t, frame_t, story_i;
static int map_node, map_from = -1, map_t;     /* walking between nodes */
static int res_hp, res_perfect, res_first, res_level;
static bool sheet_mode, demo_auto, bot_play;
static int last_phase;

typedef struct { float x, y, vx, vy; int life, col, kind; } Part;
static Part parts[200];

/* float-free little particles */
static void part_add(float x, float y, float vx, float vy, int life, int col, int kind) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) { parts[i] = (Part){x, y, vx, vy, life, col, kind}; return; }
}

static void burst(float x, float y, int col, int n, float sp) {
    for (int i = 0; i < n; i++) {
        float a = (float)i / (float)n * 6.283f + (float)(frame_t % 7) * 0.3f;
        part_add(x, y, cosf(a) * sp, sinf(a) * sp - 0.3f, 14 + i % 7, col, 0);
    }
}

/* ------------------------------------------------------------------ */
/* the island map                                                       */

typedef struct { int x, y; const char *name; } Node;
static const Node NODES[FH_LEVELS] = {
    {78, 140, "FIRST TRACKS"}, {78, 104, "THE COIL"}, {128, 104, "FERNBRAKE"}, {180, 126, "ASHFLATS"},
    {128, 66, "FOUR WAYS"}, {232, 126, "PALM SPRING"}, {180, 66, "VINE RUN"}, {232, 76, "SKY SCARE"},
    {258, 48, "THE TANGLE"}, {258, 18, "THE FOUR LORDS"},
    {36, 140, "HEARTHOME"}, {232, 160, "THE SCALE CAMP"}, {298, 48, "FAR ISLE"},
};
/* each road: from, the way you press there, to; hidden roads lead to villages */
typedef struct { int a, dir, b, hidden; } Road;
static const Road ROADS[] = {
    {FH_V_HOME, FH_RIGHT, 0, 0}, {0, FH_UP, 1, 0}, {1, FH_RIGHT, 2, 0}, {2, FH_RIGHT, 3, 0}, {2, FH_UP, 4, 0},
    {3, FH_RIGHT, 5, 0}, {4, FH_RIGHT, 6, 0}, {5, FH_UP, 7, 0}, {6, FH_RIGHT, 7, 0}, {7, FH_UP, 8, 0},
    {8, FH_UP, 9, 0}, {5, FH_DOWN, FH_V_CAMP, 1}, {8, FH_RIGHT, FH_V_ISLE, 2},
};

static bool is_village(int n) { return n >= FH_STAGES; }
static bool cleared(int n) { return (sv.done[n] & 1) != 0; }
static bool found(int n) {
    if (n == FH_V_CAMP) return (sv.found & 1) != 0;
    if (n == FH_V_ISLE) return (sv.found & 2) != 0;
    return true;
}
/* a node you may stand on: home, anything cleared, anything next to a cleared stage */
static bool open_node(int n) {
    if (n == FH_V_HOME || cleared(n)) return true;
    if (is_village(n)) return found(n);
    for (int i = 0; i < ARRAY_LEN(ROADS); i++) {
        const Road *r = &ROADS[i];
        if (r->hidden) continue;
        int o = r->a == n ? r->b : r->b == n ? r->a : -1;
        if (o >= 0 && (o == FH_V_HOME || cleared(o))) return true;
    }
    return false;
}

/* where pressing dir from node n leads (-1: nowhere); hidden roads only from a cleared stage */
static int road_to(int n, int dir) {
    for (int i = 0; i < ARRAY_LEN(ROADS); i++) {
        const Road *r = &ROADS[i];
        int to = -1;
        if (r->a == n && r->dir == dir) to = r->b;
        else if (r->b == n && (r->dir + 2) % 4 == dir) to = r->a;
        if (to < 0) continue;
        if (r->hidden && r->a == n && !cleared(n)) return -1;
        if (!r->hidden && !open_node(to)) return -1;
        return to;
    }
    return -1;
}

static int all_perfect(void) {
    int n = 0;
    for (int i = 0; i < FH_STAGES; i++) n += (sv.done[i] & 2) != 0;
    return n;
}

/* ------------------------------------------------------------------ */
/* save and goals                                                       */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    sv.node = (uint8_t)map_node;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void check_goals(void) {
    if (sv.found & 1) game_award(GOAL_BEACON);   /* reach the second village */
    if (sv.done[FH_FINAL] & 1) game_award(GOAL_SAUCER); /* clear the last stage */
    if (all_perfect() == FH_STAGES) game_award(GOAL_ALIEN); /* every stage without damage */
}

static void load_save(void) {
    Save tmp;
    memset(&sv, 0, sizeof sv);
    sv.magic = SAVE_MAGIC;
    sv.node = FH_V_HOME;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    if (sv.node >= FH_LEVELS) sv.node = FH_V_HOME;
    map_node = sv.node;
    check_goals();
}

/* ------------------------------------------------------------------ */
/* flow                                                                  */

static const char *const STORY[3][3] = {
    {"FOR AS LONG AS ANYONE COULD SING,", "PEOPLE AND BEASTS SHARED THE FIRES", "OF FLINT ISLE."},
    {"THEN THE FOUR LORDS OF THE SCALE", "CLAIMED THE ISLE, AND DROVE EVERY", "BEAST ON IT AGAINST THE CAVES."},
    {"PIM, YOUNGEST CHIEF OF THE HEARTH CLAN,", "TAKES UP HER BONES. HOLD THE CAVES.", "FEED THE CLAN. SEND THE LORDS HOME."},
};

static void pause_pick(int i);
static const char *const PAUSE_ITEMS[2] = {"RESTART STAGE", "BACK TO THE MAP"};

static void to_title(void) {
    state = S_TITLE;
    state_t = 0;
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    music_play(FH_MUS_TITLE);
}

static void to_map(void) {
    state = S_MAP;
    state_t = 0;
    map_from = -1;
    game_set_pausable(true);
    game_pause_items(0, NULL, NULL);
    music_play(FH_MUS_MAP);
}

static int battle_song(int level) {
    static int *const SONGS[5] = {&FH_MUS_BATTLE1, &FH_MUS_BATTLE2, &FH_MUS_BATTLE3, &FH_MUS_LORDS, &FH_MUS_VILLAGE};
    return *SONGS[iclamp(FH_STAGE[level].boss_song, 0, 4)];
}

static void to_stage(int level) {
    fh_sim_start(level);
    fh_bot_reset();
    state = S_STAGE;
    state_t = 0;
    last_phase = PH_BUILD;
    memset(parts, 0, sizeof parts);
    game_set_pausable(true);
    game_pause_items(2, PAUSE_ITEMS, pause_pick);
    /* the build phase is quiet: the music comes with the horn */
    music_stop();
}

static void pause_pick(int i) {
    if (state != S_STAGE) return;
    if (i == 0) to_stage(fh.level);
    else to_map();
}

static void note_stats(void) {
    sv.cave_damage += (uint32_t)fh.cave_hits;
    if (fh.most_units > sv.most_units) sv.most_units = (uint16_t)fh.most_units;
    if (fh.most_hens > sv.most_hens) sv.most_hens = (uint16_t)fh.most_hens;
    if (fh.spent_upgrades > sv.most_spend) sv.most_spend = (uint16_t)fh.spent_upgrades;
}

static void stage_won(void) {
    int lv = fh.level;
    res_level = lv;
    res_hp = fh.cave_hp;
    res_perfect = fh.cave_hp >= FH_CAVE_HP;
    res_first = !cleared(lv);
    sv.done[lv] |= 1;
    if (res_perfect) sv.done[lv] |= 2;
    if (fh.cave_hp > sv.best_hp[lv]) sv.best_hp[lv] = (uint8_t)fh.cave_hp;
    note_stats();
    if (lv == FH_FINAL) {
        sv.won = 1;
        if (all_perfect() == FH_STAGES) sv.good_end = 1;
    }
    save_now();
    check_goals();
    state = S_RESULT;
    state_t = 0;
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    music_play(FH_MUS_CLEAR);
}

static void stage_lost(void) {
    note_stats();
    save_now();
    state = S_FELL;
    state_t = 0;
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    music_play(FH_MUS_FELL);
}

static uint32_t pressed_mask(void) {
    uint32_t m = 0;
    for (int b = 0; b < 8; b++)
        if (btnp(1 << b)) m |= 1u << b;
    return m;
}

static void consume_events(void) {
    for (int i = 0; i < fh.n_ev; i++) {
        const FhEvent *e = &fh.ev[i];
        float x = (float)e->x, y = (float)(e->y + FH_OY);
        switch (e->kind) {
        case EV_KILL:
            if (FH_FOE[e->a].boss) { burst(x, y, C_YELLOW, 24, 2.0f); burst(x, y, C_RED, 16, 1.2f); sfx_play_name("fh_boss_kill"); gfx_set_flash(3); }
            else { burst(x, y, C_WHITE, 8, 1.1f); burst(x, y, C_AMBER, 5, 0.6f); sfx_play_name("fh_kill"); }
            part_add(x, y - 4, 0, -0.5f, 30, C_AMBER, 2); /* the meat it drops */
            break;
        case EV_HIT: if ((frame_t & 3) == 0) sfx_play_name("fh_hit"); part_add(x, y, 0, -0.2f, 6, C_WHITE, 1); break;
        case EV_CAVE: burst(x, y, C_RED, 12, 1.5f); sfx_play_name("fh_cave"); break;
        case EV_PIM_HIT: burst(x, y, C_YELLOW, 10, 1.2f); sfx_play_name("fh_ouch"); break;
        case EV_BUILD: burst(x, y, C_LIGHT, 8, 0.9f); sfx_play_name("fh_build"); break;
        case EV_SELL: burst(x, y, C_AMBER, 8, 0.9f); sfx_play_name("fh_sell"); break;
        case EV_DIG: burst(x, y, C_TAN, 12, 1.3f); sfx_play_name("fh_dig"); break;
        case EV_NOPE: sfx_play_name("fh_nope"); break;
        case EV_UPGRADE: burst(x, y, C_YELLOW, 10, 1.0f); sfx_play_name("fh_up"); break;
        case EV_COOK: sfx_play_name("fh_cook"); break;
        case EV_THROW: if (e->a < 0) sfx_play_name("fh_throw"); break;
        default: break;
        }
    }
}

static void stage_update(void) {
    uint32_t held = input_held() & 0xFF, pr = pressed_mask();
    if (demo_auto || bot_play) {
        static uint32_t prev;
        held = (uint32_t)fh_bot_buttons();
        pr = held & ~prev;
        prev = held;
    }
    int talk_before = fh.talk_npc;
    fh_sim_step(held, pr);
    consume_events();
    if (fh.talk_npc >= 0 && talk_before < 0) sfx_play_name("fh_talk");
    if (fh.phase != last_phase) {
        if (fh.phase == PH_BATTLE) { sfx_play_name("fh_horn"); music_play(battle_song(fh.level)); }
        else if (fh.phase == PH_BUILD) { music_stop(); music_play(FH_MUS_WAVE); }
        last_phase = fh.phase;
    }
    if (fh.phase == PH_WON && fh.phase_t > 40) stage_won();
    else if (fh.phase == PH_LOST && fh.phase_t > 50) stage_lost();
}

static void map_update(void) {
    if (map_from >= 0) {
        if (++map_t >= 18) {
            map_from = -1;
            if (map_node == FH_V_CAMP && !(sv.found & 1)) { sv.found |= 1; save_now(); check_goals(); }
            if (map_node == FH_V_ISLE && !(sv.found & 2)) { sv.found |= 2; save_now(); }
            sv.node = (uint8_t)map_node;
        }
        return;
    }
    static const int PADS[4] = {BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_UP};
    for (int d = 0; d < 4; d++) {
        if (!btnp(PADS[d])) continue;
        int to = road_to(map_node, d);
        if (to < 0) { sfx_play_name("fh_nope"); break; }
        map_from = map_node;
        map_node = to;
        map_t = 0;
        sfx_play_name("fh_step");
        break;
    }
    if (map_from < 0 && state_t > 10 && btnp(BTN_A)) {
        sfx_play_name("ui_ok");
        input_consume();
        to_stage(map_node);
        return;
    }
    if (btnp(BTN_B)) { sfx_play_name("ui_back"); save_now(); to_title(); }
}

static void fh_update(void) {
    state_t++;
    frame_t++;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        if (p->kind == 0) { p->vx *= 0.9f; p->vy = p->vy * 0.9f + 0.05f; }
    }
    switch (state) {
    case S_TITLE:
        game_set_pausable(false);
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) {
            sfx_play_name("ui_ok");
            input_consume();
            if (!sv.story_seen) { state = S_STORY; state_t = 0; story_i = 0; }
            else to_map();
        }
        break;
    case S_STORY:
        if (state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            state_t = 0;
            if (++story_i >= 3) { sv.story_seen = 1; save_now(); to_map(); }
        }
        break;
    case S_MAP: map_update(); break;
    case S_STAGE: stage_update(); break;
    case S_RESULT:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            if (res_level == FH_FINAL) { state = S_END; state_t = 0; music_play(FH_MUS_END); }
            else to_map();
        }
        break;
    case S_FELL:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) { input_consume(); to_stage(fh.level); }
        else if (state_t > 60 && btnp(BTN_B)) to_map();
        break;
    case S_END:
        if (state_t > 480 && (btnp(BTN_A) || btnp(BTN_START))) { input_consume(); to_map(); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing: the field                                                    */

typedef struct { uint8_t g1, g2, speck, path, path_edge, thick1, thick2, flower; } Theme;
static const Theme THEMES[8] = {
    {C_LEAF, C_JADE, C_LIME, C_AMBER, C_EARTH, C_FOREST, C_TEAL, C_YELLOW},   /* meadow */
    {C_JADE, C_FOREST, C_LEAF, C_EARTH, C_TAN, C_FOREST, C_TEAL, C_PINK},     /* fern brake */
    {C_GREY, C_SLATE, C_LIGHT, C_EARTH, C_TAN, C_SLATE, C_DUSK, C_ORANGE},    /* ash flats */
    {C_HIDE, C_EARTH, C_CREAM, C_AMBER, C_EARTH, C_JADE, C_FOREST, C_RED},    /* oasis sand */
    {C_FOREST, C_TEAL, C_JADE, C_EARTH, C_TAN, C_TEAL, C_NIGHT, C_MAGENTA},   /* jungle */
    {C_EARTH, C_TAN, C_HIDE, C_AMBER, C_TAN, C_BROWN, C_MAROON, C_LIME},      /* cliffs */
    {C_BROWN, C_MAROON, C_TAN, C_ORANGE, C_RED, C_DUSK, C_NIGHT, C_YELLOW},   /* volcano */
    {C_LEAF, C_JADE, C_LIME, C_HIDE, C_EARTH, C_FOREST, C_TEAL, C_PINK},      /* village */
};
static const Theme *TH(void) { return &THEMES[iclamp(FH_STAGE[fh.level].theme, 0, 7)]; }

static uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)(x * 73856093) ^ (uint32_t)(y * 19349663) ^ (uint32_t)(fh.level * 83492791);
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    return h ^ (h >> 15);
}

static bool path_at(int x, int y) {
    return x >= 0 && y >= 0 && x < FH_W && y < FH_H && (fh.tile[y][x] == TL_PATH || fh.tile[y][x] == TL_CAVE);
}

static void draw_grass(int px, int py, int x, int y, const Theme *t, bool flowers) {
    gfx_rect(px, py, FH_T, FH_T, t->g1);
    uint32_t h = hash2(x, y);
    for (int k = 0; k < 4; k++) {
        int sx = (int)((h >> (k * 5)) % 14) + 1, sy = (int)((h >> (k * 5 + 3)) % 14) + 1;
        gfx_pset(px + sx, py + sy, t->g2);
        gfx_pset(px + sx, py + sy - 1, k == 0 ? t->speck : t->g2);
    }
    if (flowers) {
        for (int k = 0; k < 3; k++) {
            int sx = 2 + (int)((h >> (k * 7 + 2)) % 11), sy = 2 + (int)((h >> (k * 7 + 5)) % 11);
            gfx_pset(px + sx, py + sy, t->flower);
            gfx_pset(px + sx, py + sy + 1, t->g2);
        }
    }
}

static void draw_tile(int x, int y) {
    const Theme *t = TH();
    int px = x * FH_T, py = FH_OY + y * FH_T;
    switch (fh.tile[y][x]) {
    case TL_PATH: {
        gfx_rect(px, py, FH_T, FH_T, t->path);
        /* worn edges where the path meets grass */
        if (!path_at(x, y - 1)) gfx_hline(px, px + FH_T - 1, py, t->path_edge);
        if (!path_at(x, y + 1)) gfx_hline(px, px + FH_T - 1, py + FH_T - 1, t->path_edge);
        if (!path_at(x - 1, y)) gfx_vline(px, py, py + FH_T - 1, t->path_edge);
        if (!path_at(x + 1, y)) gfx_vline(px + FH_T - 1, py, py + FH_T - 1, t->path_edge);
        uint32_t h = hash2(x, y);
        gfx_pset(px + 3 + (int)(h % 9), py + 4 + (int)((h >> 4) % 8), t->path_edge);
        gfx_pset(px + 5 + (int)((h >> 8) % 7), py + 2 + (int)((h >> 12) % 10), t->path_edge);
        return;
    }
    case TL_WATER: {
        gfx_rect(px, py, FH_T, FH_T, C_BLUE);
        int o = (frame_t / 12 + x * 3 + y) % 8;
        gfx_hline(px + o, px + o + 4, py + 5, C_SKY);
        gfx_hline(px + (o + 6) % 12, px + (o + 6) % 12 + 3, py + 11, C_SKY);
        return;
    }
    default: break;
    }
    draw_grass(px, py, x, y, t, fh.tile[y][x] == TL_FLOWER);
    switch (fh.tile[y][x]) {
    case TL_THICKET: {
        /* three round trees, dark below */
        uint32_t h = hash2(x, y);
        gfx_rect(px, py + 10, FH_T, 6, t->thick2);
        gfx_circ(px + 4, py + 7, 5, t->thick2);
        gfx_circ(px + 11, py + 8, 5, t->thick2);
        gfx_circ(px + 4, py + 6, 4, t->thick1);
        gfx_circ(px + 11, py + 7, 4, t->thick1);
        gfx_circ(px + 8, py + 3 + (int)(h % 2), 4, t->thick1);
        gfx_pset(px + 3, py + 4, t->g1);
        gfx_pset(px + 9, py + 2, t->g1);
        break;
    }
    case TL_BUSH:
        gfx_circ(px + 8, py + 9, 6, C_INK);
        gfx_circ(px + 8, py + 9, 5, C_FOREST);
        gfx_circ(px + 7, py + 8, 3, C_JADE);
        gfx_pset(px + 5, py + 10, C_RED);
        gfx_pset(px + 10, py + 7, C_RED);
        gfx_pset(px + 9, py + 12, C_RED);
        break;
    case TL_ROCK:
        gfx_circ(px + 8, py + 9, 6, C_INK);
        gfx_circ(px + 8, py + 9, 5, C_SLATE);
        gfx_circ(px + 7, py + 8, 4, C_GREY);
        gfx_hline(px + 5, px + 8, py + 6, C_LIGHT);
        gfx_pset(px + 10, py + 11, C_DUSK);
        break;
    case TL_HUT:
        gfx_rect(px + 2, py + 9, 12, 6, C_TAN);
        gfx_rect(px + 6, py + 11, 4, 4, C_BROWN);
        for (int k = 0; k < 8; k++) gfx_hline(px + 8 - k, px + 7 + k, py + 1 + k, k % 3 == 2 ? C_TAN : C_AMBER);
        gfx_hline(px, px + 15, py + 9, C_BROWN);
        break;
    default: break;
    }
}

static void draw_half(const Sprite *s, int x, int y, int flip) {
    /* every other pixel: a small icon of any sprite */
    for (int yy = 0; yy < s->h; yy += 2)
        for (int xx = 0; xx < s->w; xx += 2) {
            int sx = flip ? s->w - 1 - xx : xx;
            uint8_t c = s->px[yy * s->w + sx];
            if (c != TRANSPARENT) gfx_pset(x + xx / 2, y + yy / 2, c);
        }
}

static void foe_look(int kind, int frame, const Sprite **s, uint8_t *m, int *scale) {
    pal_identity(m);
    *scale = 1;
    int base = FS_NIPPER0;
    switch (kind) {
    case E_NIPPER: base = FS_NIPPER0; break;
    case E_REDBACK: base = FS_REDBACK0; break;
    case E_FANGCAT: base = FS_FANGCAT0; break;
    case E_CLUBTAIL: base = FS_CLUBTAIL0; break;
    case E_GLIDER: base = FS_GLIDER0; break;
    case E_GNAT: base = FS_GNAT0; break;
    case E_SHAGTUSK: base = FS_SHAGTUSK0; break;
    case E_JAW: base = FS_JAW0; *scale = 2; m[C_RED] = C_JADE; m[C_MAROON] = C_FOREST; break;
    case E_PLATE: base = FS_PLATE0; *scale = 2; m[C_GREY] = C_RED; m[C_SLATE] = C_ORANGE; m[C_TAN] = C_EARTH; break;
    case E_GALE: base = FS_GALE0; *scale = 2; m[C_VIOLET] = C_WINE; m[C_PURPLE] = C_MAROON; break;
    case E_SNAP: base = FS_SNAP0; *scale = 2; m[C_LEAF] = C_SKY; m[C_JADE] = C_BLUE; break;
    default: break;
    }
    *s = &fh_spr[base + (frame & 1)];
}

static void draw_foe(const FhFoe *f) {
    const FhFoeDef *d = &FH_FOE[f->kind];
    const Sprite *s;
    uint8_t m[PAL_COUNT];
    int scale;
    int walk = (frame_t / (d->speed >= 40 ? 5 : 10) + (int)(f - fh.foe)) & 1;
    foe_look(f->kind, walk, &s, m, &scale);
    if (f->tar_t > 0) { m[C_LEAF] = m[C_RED] = m[C_AMBER] = m[C_TAN] = m[C_VIOLET] = m[C_BROWN] = C_DUSK; }
    int w = s->w * scale, h = s->h * scale;
    int cx = f->x / FH_U, cy = FH_OY + f->y / FH_U;
    int lift = d->flies ? 10 + (int)(2 * sinf((float)(frame_t + f->kind * 7) * 0.12f)) : 0;
    if (d->flies) gfx_dither_circle(cx, cy + 4, d->boss ? 9 : 4, C_INK, 8);
    int sx = cx - w / 2, sy = cy - h + 7 - lift;
    int flip = f->face < 0 ? SPR_FLIPX : 0;
    int solid = f->flash_t > 0 && (f->flash_t & 2) ? C_WHITE : -1;
    if (scale == 1) spr_draw_ex(s, sx, sy, flip, m, solid);
    else {
        for (int yy = 0; yy < s->h; yy++)
            for (int xx = 0; xx < s->w; xx++) {
                uint8_t c = s->px[yy * s->w + (flip ? s->w - 1 - xx : xx)];
                if (c == TRANSPARENT) continue;
                gfx_rect(sx + xx * 2, sy + yy * 2, 2, 2, solid >= 0 ? solid : m[c]);
            }
        /* a lord's crown */
        int crx = sx + (flip ? 6 : w - 16), cry = sy + (f->kind == E_GALE ? 8 : 2);
        gfx_rect(crx, cry + 2, 9, 3, C_YELLOW);
        for (int k = 0; k < 3; k++) gfx_rect(crx + k * 4, cry, 1, 2, C_YELLOW);
        gfx_pset(crx + 4, cry + 3, C_RED);
    }
    if (d->boss || f->hp < d->hp * 10) {
        /* a health bar once hurt (always for the big ones) */
        int bw = d->boss ? 24 : 12, hw = (int)((int64_t)f->hp * bw / (d->hp * 10));
        int by = sy - 3;
        gfx_rect(cx - bw / 2 - 1, by - 1, bw + 2, 3, C_INK);
        gfx_rect(cx - bw / 2, by, imax(0, hw), 1, d->boss ? C_ORANGE : C_LIME);
    }
    if (f->slow_t > 0 && (frame_t / 4) % 2) gfx_pset(cx, sy - 5, C_CYAN);
}

static void draw_unit(const FhUnit *u) {
    int px = u->x * FH_T, py = FH_OY + u->y * FH_T;
    int flip = u->aim_x < u->x * FH_T + 8 && u->throw_t < 99 ? SPR_FLIPX : 0;
    int bob = u->throw_t < 6 ? -1 : 0;
    spr_draw(&fh_spr[FS_THROWER + u->kind], px, py + bob, flip);
}

static void draw_hen(int x, int y) {
    int px = x * FH_T, py = FH_OY + y * FH_T;
    uint8_t m[PAL_COUNT];
    pal_identity(m);
    int cook = fh.oarg[y][x];
    if (cook == 1) { m[C_WHITE] = C_CREAM; m[C_LIGHT] = C_HIDE; }
    if (cook >= 2) { m[C_WHITE] = C_EARTH; m[C_LIGHT] = C_TAN; m[C_RED] = C_ORANGE; }
    int peck = ((frame_t / 20) + x * 3 + y) % 7 == 0 ? 1 : 0;
    spr_draw_ex(&fh_spr[FS_HEN], px, py + peck, (x + y) & 1 ? SPR_FLIPX : 0, m, -1);
    if (cook >= 2 && (frame_t / 10 + x) % 6 == 0) gfx_pset(px + 8, py + 1, C_LIGHT); /* steam */
}

static void draw_pim(void) {
    if (fh.zstate == Z_GONE) return;
    int cx = fh.px / FH_U, cy = FH_OY + fh.py / FH_U;
    int sx = cx - 8, sy = cy - 12;
    int spr, flip = 0;
    bool step = (fh.walk_t / 8) & 1;
    switch (fh.face) {
    case FH_UP: spr = fh.throwing ? FS_PIM_THROW_U : step ? FS_PIM_U1 : FS_PIM_U0; break;
    case FH_DOWN: spr = fh.throwing ? FS_PIM_THROW_D : step ? FS_PIM_D1 : FS_PIM_D0; break;
    case FH_LEFT: spr = fh.throwing ? FS_PIM_THROW_S : step ? FS_PIM_S1 : FS_PIM_S0; flip = SPR_FLIPX; break;
    default: spr = fh.throwing ? FS_PIM_THROW_S : step ? FS_PIM_S1 : FS_PIM_S0; break;
    }
    gfx_dither_circle(cx, cy + 3, 5, C_INK, 8);
    if (fh.zstate == Z_STUN) {
        spr_draw_ex(&fh_spr[FS_PIM_HURT], sx, sy + 2, (fh.zt / 6) & 1 ? SPR_FLIPX : 0, NULL, (fh.zt / 3) & 1 ? C_WHITE : -1);
        for (int k = 0; k < 3; k++) {
            float a = (float)frame_t * 0.25f + (float)k * 2.09f;
            gfx_pset(cx + (int)(cosf(a) * 7), sy + (int)(sinf(a) * 2), C_YELLOW);
        }
        return;
    }
    if (fh.throwing && fh.throw_cool > fh_throw_every() - 4) {
        /* the throwing arm */
        int ax = cx + FH_DX[fh.face] * 7, ay = cy - 6 + FH_DY[fh.face] * 5;
        gfx_rect(ax - 1, ay - 1, 3, 3, C_HIDE);
    }
    spr_draw(&fh_spr[spr], sx, sy, flip);
}

static void draw_shot(const FhShot *s) {
    int x = s->x / FH_U, y = FH_OY + s->y / FH_U;
    switch (s->kind) {
    case SH_BONE: gfx_rect(x - 1, y, 3, 1, C_WHITE); gfx_pset(x - 2, y - 1, C_WHITE); gfx_pset(x + 2, y + 1, C_WHITE); break;
    case SH_SPEAR: case SH_BARB: {
        int dx = isign(s->vx), dy = isign(s->vy);
        gfx_line(x - dx * 3, y - dy * 3, x + dx * 2, y + dy * 2, C_TAN);
        gfx_pset(x + dx * 3, y + dy * 3, s->kind == SH_BARB ? C_RED : C_LIGHT);
        break;
    }
    case SH_ARROW: {
        int dx = s->vx / 16, dy = s->vy / 16;
        gfx_line(x - dx, y - dy, x, y, C_LIGHT);
        gfx_pset(x, y, C_WHITE);
        break;
    }
    case SH_ROCK: gfx_rect(x - 1, y - 1, 3, 3, C_GREY); gfx_pset(x - 1, y - 1, C_LIGHT); break;
    case SH_FIRE: gfx_rect(x - 1, y - 1, 3, 3, C_ORANGE); gfx_pset(x, y, C_YELLOW); break;
    case SH_EMBER: gfx_pset(x, y, (frame_t & 2) ? C_YELLOW : C_ORANGE); gfx_pset(x + 1, y, C_RED); break;
    case SH_TAR: gfx_rect(x - 1, y - 1, 3, 3, C_NIGHT); gfx_pset(x, y - 1, C_SLATE); break;
    case SH_PIM: {
        int lv = s->slow;
        int spin = (frame_t / 3) & 3;
        int c = lv == 0 ? C_WHITE : lv == 1 ? C_GREY : C_ORANGE;
        if (spin & 1) { gfx_vline(x, y - 2, y + 2, c); gfx_pset(x - 1, y - 2, c); }
        else { gfx_hline(x - 2, x + 2, y, c); gfx_pset(x + 2, y - 1, c); }
        if (lv >= 1) gfx_rect(x - 1, y - 1, 2, 2, lv == 2 ? C_YELLOW : C_SLATE);
        break;
    }
    default: break;
    }
}

static void draw_parts(void) {
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        const Part *p = &parts[i];
        if (p->life <= 0) continue;
        if (p->kind == 2) spr_draw(&fh_spr[FS_MEAT], (int)p->x - 4, (int)p->y - 4, 0);
        else if (p->kind == 1) gfx_pset((int)p->x, (int)p->y, p->col);
        else gfx_rect((int)p->x, (int)p->y, 2, 2, p->col);
    }
}

static void draw_hud(void) {
    char buf[48];
    gfx_rect(0, 0, SCREEN_W, FH_OY - 1, C_INK);
    gfx_hline(0, SCREEN_W - 1, FH_OY - 1, C_DUSK);
    spr_draw(&fh_spr[FS_MEAT], 4, 3, 0);
    snprintf(buf, sizeof buf, "%d", fh.meat);
    text_draw(buf, 14, 3, fh.meat >= FH_MEAT_MAX ? C_YELLOW : C_WHITE);
    spr_draw(&fh_spr[FS_HEART], 36, 4, 0);
    snprintf(buf, sizeof buf, "%d", fh.cave_hp);
    text_draw(buf, 45, 3, fh.cave_hp < 10 && (frame_t / 8) % 2 ? C_RED : C_WHITE);
    tiny_draw(FH_STAGE[fh.level].name, 4, 13, C_SLATE);
    /* the wave and what the phase is */
    /* the wave coming (in the build phase) or running */
    snprintf(buf, sizeof buf, "WAVE %d/%d", imin(fh.wave + 1, fh.n_waves), fh.n_waves);
    text_draw(buf, 70, 3, C_LIGHT);
    if (fh.phase == PH_BUILD) tiny_draw(fh.wave == 0 ? "BUILD, THEN SOUND THE HORN AT THE CAVE" : "BUILD PHASE", 70, 13, (frame_t / 30) % 2 ? C_YELLOW : C_AMBER);
    else if (fh.phase == PH_BATTLE) tiny_draw("THEY'RE COMING!", 70, 13, C_RED);
    /* Pim's arm and weapon */
    static const char *const WEAP[3] = {"BONES", "STONE AXE", "FIRE AXE"};
    snprintf(buf, sizeof buf, "%s " GLYPH_DOT " ARM %d", WEAP[fh.weapon_lv], fh.throw_lv + 1);
    tiny_draw(buf, 166, 4, C_GREY);
    /* next wave: which kinds, not in what order */
    uint8_t kinds[E_KINDS];
    int n = fh_next_wave_kinds(kinds);
    int x = SCREEN_W - 4 - n * 11;
    if (n) tiny_draw(fh.phase == PH_BATTLE ? "NOW" : "NEXT", x - 20, 4, C_SLATE);
    for (int k = 0; k < E_KINDS; k++) {
        if (!kinds[k]) continue;
        const Sprite *s;
        uint8_t m[PAL_COUNT];
        int scale;
        foe_look(k, 0, &s, m, &scale);
        gfx_rect(x, 2, 10, 10, FH_FOE[k].boss ? C_WINE : C_NIGHT);
        uint8_t mm[PAL_COUNT];
        memcpy(mm, m, sizeof mm);
        /* the half-size icon, through the lord's colours */
        for (int yy = 0; yy < s->h && yy / (s->w > 16 ? 3 : 2) < 10; yy += s->w > 16 ? 3 : 2)
            for (int xx = 0; xx < s->w; xx += s->w > 16 ? 3 : 2) {
                uint8_t c = s->px[yy * s->w + xx];
                if (c != TRANSPARENT) gfx_pset(x + xx / (s->w > 16 ? 3 : 2) + 1, 2 + yy / (s->w > 16 ? 3 : 2), mm[c]);
            }
        x += 11;
    }
    (void)draw_half;
}

static void draw_menu(void) {
    char buf[40];
    int tx = fh.menu_tx, ty = fh.menu_ty;
    int w = 100, h = 14 + fh.menu_n * 10;
    int x = tx * FH_T + 20, y = FH_OY + ty * FH_T - 4;
    if (x + w > SCREEN_W - 2) x = tx * FH_T - w - 4;
    if (y + h > SCREEN_H - 2) y = SCREEN_H - 2 - h;
    if (y < FH_OY + 2) y = FH_OY + 2;
    /* the faced tile, and a unit's reach */
    if (fh.obj[ty][tx] == O_UNIT) {
        const FhUnit *u = &fh.unit[fh.oarg[ty][tx]];
        gfx_circb(tx * FH_T + 8, FH_OY + ty * FH_T + 8, FH_UNIT[u->kind].range * FH_T + 8, C_WHITE);
    }
    gfx_rectb(tx * FH_T, FH_OY + ty * FH_T, FH_T, FH_T, (frame_t / 6) % 2 ? C_WHITE : C_YELLOW);
    ui_panel(x, y, w, h, C_INK, C_TAN);
    const char *head = "";
    if (fh.tile[ty][tx] == TL_CAVE) head = "THE CAVE";
    else if (fh.tile[ty][tx] == TL_BUSH) head = "A BUSH";
    else if (fh.tile[ty][tx] == TL_ROCK) head = "A BOULDER";
    else if (fh.obj[ty][tx] == O_UNIT) head = FH_UNIT[fh.unit[fh.oarg[ty][tx]].kind].name;
    else if (fh.obj[ty][tx] == O_HEN) head = fh.oarg[ty][tx] >= 2 ? "HEN (COOKED)" : fh.oarg[ty][tx] ? "HEN (HALF DONE)" : "HEN";
    else if (fh.obj[ty][tx] == O_FIRE) head = "FIRE PIT";
    else head = "OPEN GROUND";
    tiny_draw(head, x + 5, y + 4, C_AMBER);
    for (int i = 0; i < fh.menu_n; i++) {
        const FhMenuItem *m = &fh.menu[i];
        int ly = y + 12 + i * 10;
        bool sel = i == fh.menu_sel;
        if (sel) gfx_rect(x + 2, ly - 1, w - 4, 9, C_DUSK);
        int col = !m->ok ? C_SLATE : sel ? C_WHITE : C_LIGHT;
        text_draw(fh_menu_label(m, buf, sizeof buf), x + 9, ly, col);
        if (sel) gfx_pset(x + 5, ly + 3, C_YELLOW), gfx_pset(x + 6, ly + 3, C_YELLOW);
        if (m->act == M_SELL) snprintf(buf, sizeof buf, "+%d", fh_sell_value(tx, ty));
        else if (m->cost) snprintf(buf, sizeof buf, "%d", m->cost);
        else buf[0] = 0;
        if (buf[0]) {
            tiny_draw(buf, x + w - 5 - tiny_width(buf), ly + 1, m->act == M_SELL ? C_LIME : m->ok ? C_YELLOW : C_RED);
        }
    }
}

static void draw_talk(void) {
    int v = fh.level - FH_STAGES;
    if (v < 0 || v >= FH_VILLAGES || fh.talk_npc < 0) return;
    const FhTalk *t = &FH_TALK[v][iclamp(fh.talk_npc, 0, 3)];
    ui_panel(8, SCREEN_H - 50, SCREEN_W - 16, 44, C_INK, C_AMBER);
    text_draw(t->who, 16, SCREEN_H - 45, C_YELLOW);
    text_wrap(t->line, 16, SCREEN_H - 35, SCREEN_W - 32, C_WHITE, 9);
}

static void draw_npc(int x, int y, int id) {
    int px = x * FH_T, py = FH_OY + y * FH_T;
    int v = fh.level - FH_STAGES;
    uint8_t m[PAL_COUNT];
    pal_identity(m);
    int bob = ((frame_t / 24) + id) % 2;
    if (v == 0) {
        if (id == 0) { m[C_BROWN] = C_LIGHT; spr_draw_ex(&fh_spr[FS_ELDER], px, py - bob, 0, m, -1); }
        else if (id == 1) spr_draw(&fh_spr[FS_BABY], px, py - bob, 0);
        else { m[C_ORANGE] = C_JADE; m[C_AMBER] = C_LIME; m[C_BROWN] = C_EARTH; spr_draw_ex(&fh_spr[FS_HENWIFE], px, py - bob, SPR_FLIPX, m, -1); }
    } else if (v == 1) {
        static const uint8_t SCALES[4][2] = {{C_SKY, C_BLUE}, {C_YELLOW, C_AMBER}, {C_PINK, C_MAGENTA}, {C_CYAN, C_TEAL}};
        m[C_LEAF] = SCALES[id & 3][0];
        m[C_JADE] = SCALES[id & 3][1];
        spr_draw_ex(&fh_spr[FS_SCALEFOLK], px, py - bob, id & 1 ? SPR_FLIPX : 0, m, -1);
    } else {
        m[C_BROWN] = C_TAN;
        m[C_TAN] = C_EARTH;
        spr_draw_ex(&fh_spr[FS_TUSKLING], px - 4, py - 2 - bob, id & 1 ? SPR_FLIPX : 0, m, -1);
    }
    /* a speech mark when Pim faces them */
    int fx, fy;
    if (fh.talk_npc < 0 && fh_faced(&fx, &fy) && fx == x && fy == y && (frame_t / 10) % 2) text_draw("?", px + 6, py - 9, C_WHITE);
}

static void draw_stage(void) {
    gfx_cls(C_INK);
    for (int y = 0; y < FH_H; y++)
        for (int x = 0; x < FH_W; x++) draw_tile(x, y);
    /* the cave */
    {
        uint8_t m[PAL_COUNT];
        pal_identity(m);
        if (FH_STAGE[fh.level].theme == 7) { m[C_GREY] = C_TAN; m[C_SLATE] = C_BROWN; m[C_LIGHT] = C_EARTH; }
        if (FH_STAGE[fh.level].theme == 6) { m[C_GREY] = C_DUSK; m[C_SLATE] = C_NIGHT; m[C_LIGHT] = C_SLATE; }
        int cx = fh.cave_x * FH_T, cy = FH_OY + fh.cave_y * FH_T;
        spr_draw_ex(&fh_spr[FS_CAVE], cx - 4, cy - 4, 0, m, -1);
        /* the glow of the clan's fire inside */
        gfx_rect(cx + 5, cy + 8, 6, 3, (frame_t / 8) % 2 ? C_ORANGE : C_AMBER);
        if (fh.cave_hp < FH_CAVE_HP / 3 && (frame_t / 16) % 2) gfx_pset(cx + 8, cy - 5, C_GREY);
    }
    /* things on the ground, back to front */
    for (int y = 0; y < FH_H; y++) {
        for (int x = 0; x < FH_W; x++) {
            switch (fh.obj[y][x]) {
            case O_UNIT: draw_unit(&fh.unit[fh.oarg[y][x]]); break;
            case O_HEN: draw_hen(x, y); break;
            case O_FIRE: spr_draw(&fh_spr[(frame_t / 8 + x) % 2 ? FS_FIRE0 : FS_FIRE1], x * FH_T, FH_OY + y * FH_T, 0); break;
            case O_NPC: draw_npc(x, y, fh.oarg[y][x]); break;
            default: break;
            }
        }
        /* walkers on this row, and Pim */
        for (int i = 0; i < FH_MAX_FOES; i++) {
            const FhFoe *f = &fh.foe[i];
            if (f->on && !FH_FOE[f->kind].flies && f->y / FH_TU == y) draw_foe(f);
        }
        if (fh.py / FH_TU == y) draw_pim();
    }
    for (int i = 0; i < FH_MAX_ROLLS; i++) {
        const FhRoll *r = &fh.roll[i];
        if (!r->on) continue;
        int x = r->x / FH_U, y = FH_OY + r->y / FH_U;
        gfx_circ(x, y, 5, C_INK);
        gfx_circ(x, y, 4, C_GREY);
        int a = (frame_t / 2) % 4;
        gfx_pset(x - 2 + a, y - 2, C_LIGHT);
        gfx_pset(x + 1, y + 1 - a / 2, C_SLATE);
    }
    for (int i = 0; i < FH_MAX_SHOTS; i++)
        if (fh.shot[i].on) draw_shot(&fh.shot[i]);
    for (int i = 0; i < FH_MAX_FOES; i++) {
        const FhFoe *f = &fh.foe[i];
        if (f->on && FH_FOE[f->kind].flies) draw_foe(f);
    }
    draw_parts();
    /* what A would work on */
    int fx, fy;
    if (!fh.menu_open && fh.zstate == Z_OK && fh.talk_npc < 0 && fh_faced(&fx, &fy) && (fh_buildable(fx, fy) || fh.obj[fy][fx] == O_UNIT ||
        fh.obj[fy][fx] == O_HEN || fh.obj[fy][fx] == O_FIRE || fh.tile[fy][fx] == TL_CAVE || fh.tile[fy][fx] == TL_BUSH || fh.tile[fy][fx] == TL_ROCK)) {
        int c = (frame_t / 10) % 2 ? C_WHITE : C_LIGHT;
        int px = fx * FH_T, py = FH_OY + fy * FH_T;
        gfx_hline(px, px + 3, py, c); gfx_vline(px, py, py + 3, c);
        gfx_hline(px + 12, px + 15, py, c); gfx_vline(px + 15, py, py + 3, c);
        gfx_hline(px, px + 3, py + 15, c); gfx_vline(px, py + 12, py + 15, c);
        gfx_hline(px + 12, px + 15, py + 15, c); gfx_vline(px + 15, py + 12, py + 15, c);
    }
    draw_hud();
    if (fh.menu_open) draw_menu();
    draw_talk();
    if (fh.phase == PH_BUILD && fh.phase_t < 90 && fh.wave > 0) {
        ui_panel(100, 70, 120, 22, C_INK, C_LIME);
        text_center("WAVE HELD!", 160, 77, C_LIME);
    }
    if (fh.phase == PH_BATTLE && fh.phase_t < 60) {
        char buf[32];
        snprintf(buf, sizeof buf, "WAVE %d", fh.wave + 1);
        text_center(buf, 160, 60 + imin(fh.phase_t, 10), C_WHITE);
    }
    if (fh.phase == PH_WON) text_center("THE CAVE HOLDS!", 160, 80, (frame_t / 6) % 2 ? C_YELLOW : C_WHITE);
    if (fh.phase == PH_LOST) text_center("THE CAVE HAS FALLEN...", 160, 80, C_RED);
}

/* ------------------------------------------------------------------ */
/* drawing: screens                                                      */

static void draw_sea(int t) {
    gfx_cls(C_NAVY);
    for (int i = 0; i < 60; i++) {
        uint32_t h = (uint32_t)(i * 2654435761u);
        int x = (int)(h % 320), y = (int)((h >> 12) % 180);
        int o = ((t / 20) + i) % 3;
        gfx_hline(x + o, x + o + 3, y, C_BLUE);
    }
}

static void draw_map(void) {
    draw_sea(frame_t);
    /* the island: land round every road and stop, the volcano up top */
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < FH_LEVELS; i++) {
            if (is_village(i) && !found(i) && i != FH_V_HOME) continue;
            int r = pass == 0 ? 22 : 18;
            gfx_circ(NODES[i].x, NODES[i].y, r, pass == 0 ? C_TAN : C_LEAF);
        }
    for (int i = 0; i < ARRAY_LEN(ROADS); i++) {
        const Road *r = &ROADS[i];
        if (r->hidden && !found(r->b)) continue;
        int ax = NODES[r->a].x, ay = NODES[r->a].y, bx = NODES[r->b].x, by = NODES[r->b].y;
        for (int k = 0; k <= 8; k++) {
            int x = ax + (bx - ax) * k / 8, y = ay + (by - ay) * k / 8;
            gfx_circ(x, y, 12, C_LEAF);
        }
    }
    /* zone colours under the stops */
    static const uint8_t ZONE[FH_LEVELS] = {C_LEAF, C_LEAF, C_JADE, C_GREY, C_LEAF, C_HIDE, C_FOREST, C_EARTH, C_EARTH, C_BROWN, C_LEAF, C_HIDE, C_LEAF};
    for (int i = 0; i < FH_LEVELS; i++) {
        if (is_village(i) && !found(i) && i != FH_V_HOME) continue;
        gfx_circ(NODES[i].x, NODES[i].y, 14, ZONE[i]);
    }
    /* the smoking peak behind the last stage */
    for (int k = 0; k < 12; k++) gfx_hline(NODES[9].x - 4 - k * 2, NODES[9].x + 4 + k * 2, NODES[9].y - 8 + k, k < 3 ? C_RED : C_MAROON);
    for (int k = 0; k < 3; k++) gfx_pset(NODES[9].x - 2 + k * 2 + (frame_t / 20 + k) % 2, NODES[9].y - 12 - ((frame_t / 6 + k * 5) % 10), C_GREY);
    /* roads */
    for (int i = 0; i < ARRAY_LEN(ROADS); i++) {
        const Road *r = &ROADS[i];
        if (r->hidden && !found(r->b)) continue;
        int ax = NODES[r->a].x, ay = NODES[r->a].y, bx = NODES[r->b].x, by = NODES[r->b].y;
        bool lit = open_node(r->a) && open_node(r->b);
        for (int k = 0; k <= 12; k++) {
            if (!lit && k % 2) continue;
            int x = ax + (bx - ax) * k / 12, y = ay + (by - ay) * k / 12;
            gfx_rect(x - 1, y - 1, 3, 3, lit ? C_AMBER : C_TAN);
        }
    }
    /* stops */
    for (int i = 0; i < FH_LEVELS; i++) {
        if (is_village(i) && !found(i) && i != FH_V_HOME) continue;
        int x = NODES[i].x, y = NODES[i].y;
        if (is_village(i)) {
            for (int k = 0; k < 6; k++) gfx_hline(x - k - 1, x + k, y - 8 + k, C_AMBER);
            gfx_rect(x - 5, y - 2, 10, 6, C_TAN);
            gfx_rect(x - 1, y, 2, 4, C_BROWN);
            if (cleared(i)) gfx_pset(x + 6, y - 6, C_YELLOW);
            continue;
        }
        bool open = open_node(i);
        gfx_circ(x, y, 7, C_INK);
        gfx_circ(x, y, 6, cleared(i) ? C_RED : open ? C_CREAM : C_SLATE);
        char buf[4];
        snprintf(buf, sizeof buf, "%d", i + 1);
        tiny_center(buf, x, y - 2, cleared(i) ? C_WHITE : C_INK);
        if (sv.done[i] & 2) {
            /* a flag of no damage */
            gfx_vline(x + 7, y - 12, y - 4, C_BROWN);
            gfx_rect(x + 8, y - 12, 5, 4, C_YELLOW);
        } else if (cleared(i)) {
            gfx_vline(x + 7, y - 12, y - 4, C_BROWN);
            gfx_rect(x + 8, y - 12, 5, 4, C_WHITE);
        }
    }
    /* Pim */
    int px = NODES[map_node].x, py = NODES[map_node].y;
    if (map_from >= 0) {
        px = NODES[map_from].x + (NODES[map_node].x - NODES[map_from].x) * map_t / 18;
        py = NODES[map_from].y + (NODES[map_node].y - NODES[map_from].y) * map_t / 18;
    }
    int step = map_from >= 0 && (map_t / 4) % 2;
    spr_draw(&fh_spr[step ? FS_PIM_D1 : FS_PIM_D0], px - 8, py - 18 - (frame_t / 20) % 2 * 0, 0);
    /* the panel */
    gfx_rect(0, 0, SCREEN_W, 11, C_INK);
    tiny_draw("FLINT ISLE", 4, 3, C_AMBER);
    char buf[64];
    snprintf(buf, sizeof buf, "CLEARED %d/10  " GLYPH_DOT "  NO DAMAGE %d/10", (int)(cleared(0) + cleared(1) + cleared(2) + cleared(3) + cleared(4) + cleared(5) + cleared(6) + cleared(7) + cleared(8) + cleared(9)), all_perfect());
    tiny_draw(buf, SCREEN_W - 4 - tiny_width(buf), 3, C_LIGHT);
    ui_panel(4, SCREEN_H - 22, 250, 19, C_INK, C_TAN);
    int n = map_node;
    text_draw(NODES[n].name, 10, SCREEN_H - 18, C_WHITE);
    if (is_village(n)) snprintf(buf, sizeof buf, "VILLAGE %s", cleared(n) ? GLYPH_DOT " DEFENDED" : "");
    else if (sv.done[n] & 2) snprintf(buf, sizeof buf, "STAGE %d " GLYPH_DOT " NO DAMAGE!", n + 1);
    else if (cleared(n)) snprintf(buf, sizeof buf, "STAGE %d " GLYPH_DOT " BEST CAVE %d/30", n + 1, sv.best_hp[n]);
    else snprintf(buf, sizeof buf, "STAGE %d", n + 1);
    tiny_draw(buf, 110, SCREEN_H - 16, C_LIGHT);
    if (map_from < 0) ui_hint(206, SCREEN_H - 17, GLYPH_A, is_village(n) ? "VISIT" : "PLAY", C_YELLOW);
}

static void draw_title(void) {
    draw_sea(frame_t);
    /* the island at dusk: the peak, the cave, Pim by a fire */
    gfx_rect(0, 118, SCREEN_W, 62, C_JADE);
    gfx_circ(160, 190, 140, C_LEAF);
    /* the smoking peak */
    for (int k = 0; k < 58; k++) {
        int half = 6 + k * 3 / 2;
        gfx_hline(262 - half, 262 + half, 62 + k, k < 2 ? C_RED : C_BROWN);
        gfx_hline(262 - half, 262 - half + k / 3, 62 + k, C_MAROON);
    }
    for (int k = 0; k < 5; k++) gfx_vline(256 + k * 3, 62, 66 + (k * 7) % 9, k % 2 ? C_ORANGE : C_RED);
    for (int k = 0; k < 4; k++) gfx_circ(262 + (k * 7) % 11 - 5, 52 - k * 9 - (frame_t / 10) % 9, 3 + k, k ? C_GREY : C_LIGHT);
    uint8_t m[PAL_COUNT];
    pal_identity(m);
    spr_draw_scaled(&fh_spr[FS_CAVE], 20, 96, 2, 0);
    spr_draw_scaled(&fh_spr[FS_PIM_S0], 110, 104, 3, 0);
    spr_draw_scaled(&fh_spr[(frame_t / 8) % 2 ? FS_FIRE0 : FS_FIRE1], 168, 116, 2, 0);
    spr_draw_scaled(&fh_spr[FS_HEN], 206, 128, 2, SPR_FLIPX);
    int wx = 330 - (frame_t / 2) % 420;
    spr_draw(&fh_spr[(frame_t / 8) % 2 ? FS_REDBACK0 : FS_REDBACK1], wx, 156, SPR_FLIPX);
    spr_draw(&fh_spr[(frame_t / 6) % 2 ? FS_NIPPER0 : FS_NIPPER1], wx + 22, 158, SPR_FLIPX);
    static const uint8_t grad[] = {C_CREAM, C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center("FLINTHOLD", 160, 12, 3, grad, 4, C_INK, C_MAROON);
    tiny_center("HOLD THE CAVES OF FLINT ISLE", 160, 42, C_CREAM);
    if ((state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 58, C_WHITE);
    char buf[48];
    ui_panel(222, 124, 94, 50, C_INK, C_DUSK);
    tiny_draw("CAVE DAMAGE", 228, 129, C_SLATE);
    snprintf(buf, sizeof buf, "%u", (unsigned)sv.cave_damage);
    tiny_draw(buf, 310 - tiny_width(buf), 129, C_LIGHT);
    tiny_draw("MOST HUNTERS", 228, 139, C_SLATE);
    snprintf(buf, sizeof buf, "%d", sv.most_units);
    tiny_draw(buf, 310 - tiny_width(buf), 139, C_LIGHT);
    tiny_draw("MOST HENS", 228, 149, C_SLATE);
    snprintf(buf, sizeof buf, "%d", sv.most_hens);
    tiny_draw(buf, 310 - tiny_width(buf), 149, C_LIGHT);
    tiny_draw("BEST UPGRADES", 228, 159, C_SLATE);
    snprintf(buf, sizeof buf, "%d", sv.most_spend);
    tiny_draw(buf, 310 - tiny_width(buf), 159, C_LIGHT);
    (void)m;
}

static void draw_story(void) {
    draw_sea(frame_t);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    ui_panel(20, 40, 280, 70, C_INK, C_AMBER);
    int n = imin(story_i, 2);
    for (int i = 0; i < 3; i++) text_center(STORY[n][i], 160, 54 + i * 14, C_CREAM);
    if (n == 1) {
        static const int LORDS[4] = {E_SNAP, E_JAW, E_PLATE, E_GALE};
        for (int k = 0; k < 4; k++) {
            const Sprite *s;
            uint8_t m[PAL_COUNT];
            int scale;
            foe_look(LORDS[k], (frame_t / 12) & 1, &s, m, &scale);
            for (int yy = 0; yy < s->h; yy++)
                for (int xx = 0; xx < s->w; xx++) {
                    uint8_t c = s->px[yy * s->w + s->w - 1 - xx];
                    if (c != TRANSPARENT) gfx_rect(44 + k * 64 + xx * 2, 118 + yy * 2, 2, 2, m[c]);
                }
            gfx_rect(56 + k * 64, 120, 9, 3, C_YELLOW);
        }
    } else if (n == 2) {
        spr_draw_scaled(&fh_spr[FS_PIM_D0], 136, 116, 3, 0);
    } else {
        spr_draw_scaled(&fh_spr[FS_THROWER], 100, 120, 2, 0);
        spr_draw_scaled(&fh_spr[FS_NIPPER0], 170, 124, 2, SPR_FLIPX);
        spr_draw_scaled(&fh_spr[(frame_t / 8) % 2 ? FS_FIRE0 : FS_FIRE1], 136, 124, 2, 0);
    }
    if (state_t > 20 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 168, C_WHITE);
}

static void draw_result(void) {
    draw_stage();
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    ui_panel(50, 36, 220, 100, C_INK, res_perfect ? C_YELLOW : C_LIME);
    static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_AMBER};
    ui_fancy_center(res_level >= FH_STAGES ? "VILLAGE SAFE!" : "STAGE CLEAR!", 160, 44, 2, grad, 3, C_INK, C_MAROON);
    char buf[48];
    text_center(FH_STAGE[res_level].name, 160, 66, C_CREAM);
    snprintf(buf, sizeof buf, "CAVE LEFT %d/%d", res_hp, FH_CAVE_HP);
    text_center(buf, 160, 80, C_WHITE);
    if (res_perfect) text_center("NOT ONE HIT ON THE CAVE!", 160, 94, (frame_t / 8) % 2 ? C_YELLOW : C_AMBER);
    snprintf(buf, sizeof buf, "FOES DOWN %d  " GLYPH_DOT "  UPGRADES %d", fh.kills, fh.spent_upgrades);
    tiny_center(buf, 160, 108, C_LIGHT);
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 122, C_WHITE);
}

static void draw_fell(void) {
    draw_stage();
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(60, 50, 200, 70, C_INK, C_RED);
    text_center("THE CAVE HAS FALLEN", 160, 60, C_RED);
    text_center(FH_STAGE[fh.level].name, 160, 76, C_LIGHT);
    char buf[32];
    snprintf(buf, sizeof buf, "WAVE %d OF %d", fh.wave + 1, fh.n_waves);
    tiny_center(buf, 160, 90, C_GREY);
    if (state_t > 60) {
        int x = ui_hint(92, 104, GLYPH_A, "TRY AGAIN", C_WHITE);
        ui_hint(x + 6, 104, GLYPH_B, "MAP", C_WHITE);
    }
}

static void draw_end(void) {
    int t = state_t;
    draw_sea(t);
    gfx_rect(0, 120, SCREEN_W, 60, C_JADE);
    gfx_circ(160, 200, 130, C_LEAF);
    ui_panel(16, 10, 288, 100, C_INK, sv.good_end ? C_YELLOW : C_AMBER);
    static const uint8_t grad[] = {C_CREAM, C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center("FLINT ISLE IS FREE", 160, 16, 2, grad, 4, C_INK, C_MAROON);
    if (t > 40) text_center("THE FOUR LORDS FLEE ACROSS THE SEA.", 160, 40, C_WHITE);
    if (t > 120) text_center("ONE BY ONE, THE BEASTS COME BACK", 160, 54, C_CREAM);
    if (t > 160) text_center("TO THE FIRES, SHY AT FIRST.", 160, 66, C_CREAM);
    if (t > 260) {
        if (sv.good_end) {
            text_center("NOT ONE STONE OF ANY CAVE FELL.", 160, 82, C_YELLOW);
            text_center("THE ELDERS CARVE PIM'S NAME IN FLINT.", 160, 94, C_YELLOW);
        } else {
            text_center("THE CLAN MENDS ITS CAVES, AND SINGS.", 160, 88, C_LIGHT);
        }
    }
    spr_draw_scaled(&fh_spr[FS_PIM_D0], 144, 116, 2, 0);
    uint8_t m[PAL_COUNT];
    pal_identity(m);
    spr_draw(&fh_spr[(t / 10) % 2 ? FS_NIPPER0 : FS_NIPPER1], 196, 140, SPR_FLIPX);
    spr_draw(&fh_spr[FS_HEN], 100, 140, 0);
    spr_draw_scaled(&fh_spr[(t / 8) % 2 ? FS_FIRE0 : FS_FIRE1], 120, 130, 1, 0);
    if (t > 480 && (t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 170, C_WHITE);
    (void)m;
}

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    int x = 2, y = 2;
    for (int i = 0; i < FS_COUNT; i++) {
        const Sprite *s = &fh_spr[i];
        if (!s->px) continue;
        if (x + s->w * 2 > 318) { x = 2; y += 38; }
        spr_draw_scaled(s, x, y, 2, 0);
        x += s->w * 2 + 3;
    }
}

static void fh_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_STORY: draw_story(); break;
    case S_MAP: draw_map(); break;
    case S_STAGE: draw_stage(); break;
    case S_RESULT: draw_result(); break;
    case S_FELL: draw_fell(); break;
    case S_END: draw_end(); break;
    }
}

/* ------------------------------------------------------------------ */
/* the villagers' words (all our own)                                   */

const FhTalk FH_TALK[FH_VILLAGES][4] = {
    {   /* Hearthome */
        {"OLD BRAM", "A HEN BY ONE FIRE PIT IS COOKED IN TWO WAVES, BY TWO PITS IN ONE. A RAW HEN LAYS YOU 5 MEAT A WAVE; A COOKED ONE SELLS FOR 30."},
        {"THE LITTLE ONE", "BA! BA-BA! (SHE POINTS OUT TO SEA, WHERE THE MAP SHOWS NOTHING AT ALL.)"},
        {"AUNT OLA", "A HUNTER BY A FIRE PIT HITS HARDER, AND THE MORE PITS, THE HARDER. NOBODY BUILDS RIGHT BY THE PATH: THE BEASTS WOULD TRAMPLE THEM."},
        {"", ""},
    },
    {   /* the Scale Camp */
        {"ELDER SKINK", "WE LEFT THE LORDS. WE ONLY EVER WANTED THE WARM ROCKS, NOT A WAR."},
        {"BASK", "LORD PLATE'S HIDE TURNS EVERY SPEAR AND ARROW. ONLY FIRE GETS THROUGH."},
        {"GECKA", "LORD GALE FLIES OVER EVERYTHING. BOWS FIND HER BEST."},
        {"NEWT", "LORD JAW WALKS SLOWLY. BUT IF HE EVER REACHES A CAVE, THAT CAVE IS GONE."},
    },
    {   /* Far Isle */
        {"TUSKLING", "IT IS HARD, LIVING OUT HERE. THE GRASS IS THIN AND THE WIND NEVER SLEEPS."},
        {"TUSKLING", "THERE WERE FIVE OF US ONCE. ONE SWAM FOR THE BIG ISLE TO START A HERD OF HIS OWN."},
        {"TUSKLING", "WE STILL TALK ABOUT HIM. WE STILL SAVE HIM THE SWEETEST ROOTS."},
        {"TUSKLING", "IF YOU SEE HIM, TELL HIM IT IS QUIETER HERE WITHOUT HIS SINGING."},
    },
};

/* ------------------------------------------------------------------ */
/* cartridge interface                                                   */

static void fh_load(void) {
    fh_art_load();
    fh_audio_load();
}

static void fh_start(void) {
    load_save();
    memset(parts, 0, sizeof parts);
    sheet_mode = false;
    demo_auto = false;
    bot_play = false;
    to_title();
}

static void fh_quit(void) {
    if (state == S_STAGE) note_stats();
    save_now();
}

static void fh_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_LEAF);
    for (int yy = 0; yy < h; yy += 4)
        for (int xx = (yy / 4) % 2 * 3; xx < w; xx += 7) gfx_pset(x + xx, y + yy, C_JADE);
    gfx_rect(x, y + 28, w, 12, C_AMBER);
    gfx_hline(x, x + w - 1, y + 28, C_EARTH);
    gfx_hline(x, x + w - 1, y + 39, C_EARTH);
    spr_draw(&fh_spr[FS_CAVE], x + 2, y + 8, 0);
    spr_draw(&fh_spr[FS_PIM_S0], x + 30, y + 10, 0);
    spr_draw(&fh_spr[(t / 8) % 2 ? FS_FIRE0 : FS_FIRE1], x + 52, y + 42, 0);
    spr_draw(&fh_spr[FS_HEN], x + 68, y + 42, 0);
    spr_draw(&fh_spr[FS_BOW], x + 52, y + 8, 0);
    int wx = w - (t / 2) % (w + 30);
    spr_draw(&fh_spr[(t / 8) % 2 ? FS_REDBACK0 : FS_REDBACK1], x + wx, y + 25, SPR_FLIPX);
    spr_draw(&fh_spr[(t / 6) % 2 ? FS_NIPPER0 : FS_NIPPER1], x + wx + 20, y + 25, SPR_FLIPX);
    if ((t / 3) % 2) gfx_rect(x + 46 + (t % 24), y + 16, 3, 1, C_WHITE);
}

static int stages_ok(void);

static int count_obj(int o) {
    int n = 0;
    for (int y = 0; y < FH_H; y++)
        for (int x = 0; x < FH_W; x++) n += fh.obj[y][x] == o;
    return n;
}

/* the map's demo walker: head for the first stage not yet cleared */
static int map_bot(void) {
    if (map_from >= 0) return 0;
    int target = -1;
    for (int i = 0; i < FH_STAGES && target < 0; i++)
        if (!cleared(i) && open_node(i)) target = i;
    if (target < 0) target = FH_FINAL;
    if (target == map_node) return (frame_t & 1) ? BTN_A : 0;
    /* breadth-first over the roads */
    int prev[FH_LEVELS], q[FH_LEVELS], qh = 0, qt = 0;
    for (int i = 0; i < FH_LEVELS; i++) prev[i] = -2;
    prev[map_node] = -1;
    q[qt++] = map_node;
    while (qh < qt) {
        int c = q[qh++];
        for (int d = 0; d < 4; d++) {
            int n = road_to(c, d);
            if (n < 0 || prev[n] != -2) continue;
            prev[n] = c;
            q[qt++] = n;
        }
    }
    if (prev[target] == -2) return 0;
    int n = target;
    while (prev[n] != map_node) n = prev[n];
    static const int PADS[4] = {BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_UP};
    for (int d = 0; d < 4; d++)
        if (road_to(map_node, d) == n) return (frame_t & 1) ? PADS[d] : 0;
    return 0;
}

static int fh_query(const char *key, int *out) {
    int a, b;
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "level")) { *out = fh.level + 1; return 1; }
    if (!strcmp(key, "phase")) { *out = fh.phase; return 1; }
    if (!strcmp(key, "wave")) { *out = fh.wave; return 1; }
    if (!strcmp(key, "waves")) { *out = fh.n_waves; return 1; }
    if (!strcmp(key, "meat")) { *out = fh.meat; return 1; }
    if (!strcmp(key, "wasted")) { *out = fh.wasted; return 1; }
    if (!strcmp(key, "cave")) { *out = fh.cave_hp; return 1; }
    if (!strcmp(key, "cave_hits")) { *out = fh.cave_hits; return 1; }
    if (!strcmp(key, "px")) { *out = fh.px / FH_U; return 1; }
    if (!strcmp(key, "py")) { *out = fh.py / FH_U; return 1; }
    if (!strcmp(key, "tx")) { *out = fh.px / FH_TU; return 1; }
    if (!strcmp(key, "ty")) { *out = fh.py / FH_TU; return 1; }
    if (!strcmp(key, "face")) { *out = fh.face; return 1; }
    if (!strcmp(key, "zstate")) { *out = fh.zstate; return 1; }
    if (!strcmp(key, "throwing")) { *out = fh.throwing; return 1; }
    if (!strcmp(key, "throw_lv")) { *out = fh.throw_lv; return 1; }
    if (!strcmp(key, "weapon_lv")) { *out = fh.weapon_lv; return 1; }
    if (!strcmp(key, "units")) { *out = fh_unit_count(); return 1; }
    if (!strcmp(key, "hens")) { *out = count_obj(O_HEN); return 1; }
    if (!strcmp(key, "fires")) { *out = count_obj(O_FIRE); return 1; }
    if (!strcmp(key, "foes")) { *out = fh_foe_count(); return 1; }
    if (!strcmp(key, "kills")) { *out = fh.kills; return 1; }
    if (!strcmp(key, "spawns")) { *out = fh.n_spawns; return 1; }
    if (!strcmp(key, "menu_open")) { *out = fh.menu_open; return 1; }
    if (!strcmp(key, "menu_n")) { *out = fh.menu_n; return 1; }
    if (!strcmp(key, "cursor")) { *out = fh.menu_sel; return 1; }
    if (!strcmp(key, "talk")) { *out = fh.talk_npc; return 1; }
    if (!strcmp(key, "most_units")) { *out = fh.most_units; return 1; }
    if (!strcmp(key, "most_hens")) { *out = fh.most_hens; return 1; }
    if (!strcmp(key, "spent_up")) { *out = fh.spent_upgrades; return 1; }
    if (!strcmp(key, "shots")) {
        *out = 0;
        for (int i = 0; i < FH_MAX_SHOTS; i++) *out += fh.shot[i].on;
        return 1;
    }
    if (!strcmp(key, "rolls")) {
        *out = 0;
        for (int i = 0; i < FH_MAX_ROLLS; i++) *out += fh.roll[i].on;
        return 1;
    }
    if (!strcmp(key, "node")) { *out = map_node; return 1; }
    if (!strcmp(key, "found")) { *out = sv.found; return 1; }
    if (!strcmp(key, "won")) { *out = sv.won; return 1; }
    if (!strcmp(key, "good_end")) { *out = sv.good_end; return 1; }
    if (!strcmp(key, "perfects")) { *out = all_perfect(); return 1; }
    if (!strcmp(key, "story_seen")) { *out = sv.story_seen; return 1; }
    if (!strcmp(key, "stat_damage")) { *out = (int)sv.cave_damage; return 1; }
    if (!strcmp(key, "stat_units")) { *out = sv.most_units; return 1; }
    if (!strcmp(key, "stat_hens")) { *out = sv.most_hens; return 1; }
    if (!strcmp(key, "stat_spend")) { *out = sv.most_spend; return 1; }
    if (!strcmp(key, "res_perfect")) { *out = res_perfect; return 1; }
    if (!strcmp(key, "res_hp")) { *out = res_hp; return 1; }
    if (!strcmp(key, "art_bad")) { *out = fh_art_bad; return 1; }
    if (!strcmp(key, "stages_ok")) { *out = stages_ok(); return 1; }
    if (!strcmp(key, "bot_step")) { *out = fh_bot_step; return 1; }
    if (!strcmp(key, "bot")) {
        if (state == S_STAGE) *out = fh_bot_buttons();
        else if (state == S_MAP) *out = map_bot();
        else *out = (state_t / 8) % 2 ? BTN_A : 0;
        return 1;
    }
    if (sscanf(key, "done%d", &a) == 1) { *out = a >= 1 && a <= FH_LEVELS ? sv.done[a - 1] : -1; return 1; }
    if (sscanf(key, "best%d", &a) == 1) { *out = a >= 1 && a <= FH_LEVELS ? sv.best_hp[a - 1] : -1; return 1; }
    if (sscanf(key, "open%d", &a) == 1) { *out = a >= 1 && a <= FH_LEVELS ? open_node(a - 1) : -1; return 1; }
    if (sscanf(key, "menu%d", &a) == 1) { *out = a >= 0 && a < fh.menu_n ? fh.menu[a].act : -1; return 1; }
    if (sscanf(key, "obj_%d_%d", &a, &b) == 2) { *out = a >= 0 && b >= 0 && a < FH_W && b < FH_H ? fh.obj[b][a] : -1; return 1; }
    if (sscanf(key, "tile_%d_%d", &a, &b) == 2) { *out = a >= 0 && b >= 0 && a < FH_W && b < FH_H ? fh.tile[b][a] : -1; return 1; }
    if (sscanf(key, "build_%d_%d", &a, &b) == 2) { *out = fh_buildable(a, b); return 1; }
    if (sscanf(key, "cook_%d_%d", &a, &b) == 2) { *out = a >= 0 && b >= 0 && a < FH_W && b < FH_H && fh.obj[b][a] == O_HEN ? fh.oarg[b][a] : -1; return 1; }
    if (sscanf(key, "kind_%d_%d", &a, &b) == 2) {
        *out = a >= 0 && b >= 0 && a < FH_W && b < FH_H && fh.obj[b][a] == O_UNIT ? fh.unit[fh.oarg[b][a]].kind : -1;
        return 1;
    }
    if (sscanf(key, "foe%d_", &a) == 1 && strchr(key, '_')) {
        const char *f = strchr(key, '_') + 1;
        if (a < 0 || a >= FH_MAX_FOES) return 0;
        const FhFoe *fo = &fh.foe[a];
        if (!strcmp(f, "on")) { *out = fo->on; return 1; }
        if (!strcmp(f, "hp")) { *out = fo->hp; return 1; }
        if (!strcmp(f, "x")) { *out = fo->x / FH_U; return 1; }
        if (!strcmp(f, "y")) { *out = fo->y / FH_U; return 1; }
        if (!strcmp(f, "kind")) { *out = fo->kind; return 1; }
        if (!strcmp(f, "slow")) { *out = fo->slow_t; return 1; }
        if (!strcmp(f, "tar")) { *out = fo->tar_t; return 1; }
        if (!strcmp(f, "tarred")) { *out = fo->tarred; return 1; }
        return 0;
    }
    return 0;
}

/* Every stage is sound: rows the right size, the drawn path (=) is exactly
 * the routes' tiles, every route ends at the cave and starts on an edge,
 * every wave parses, and Pim can reach the cave. Returns how many pass. */
static int stages_ok(void) {
    int good = 0;
    FhSim keep;
    memcpy(&keep, &fh, sizeof keep);
    for (int l = 0; l < FH_LEVELS; l++) {
        const FhStage *st = &FH_STAGE[l];
        bool bad = false;
        for (int y = 0; y < FH_H; y++) bad |= (int)strlen(st->rows[y]) != FH_W;
        if (bad) continue;
        fh_sim_start(l);
        for (int y = 0; y < FH_H; y++)
            for (int x = 0; x < FH_W; x++) {
                char c = st->rows[y][x];
                if ((c == '=') != (fh.tile[y][x] == TL_PATH)) bad = true;
            }
        for (int r = 0; r < st->n_routes; r++) {
            const FhRoute *ro = &st->route[r];
            int ex = ro->x[ro->n - 1], ey = ro->y[ro->n - 1], sx = ro->x[0], sy = ro->y[0];
            if (ex != fh.cave_x || ey != fh.cave_y) bad = true;
            if (!(sx == 0 || sy == 0 || sx == FH_W - 1 || sy == FH_H - 1)) bad = true;
            for (int i = 1; i < ro->n; i++)
                if (ro->x[i] != ro->x[i - 1] && ro->y[i] != ro->y[i - 1]) bad = true;
        }
        int foes = 0;
        for (int w = 0; w < fh.n_waves; w++)
            for (int g = 0; g < fh.waves[w].n; g++) foes += fh.waves[w].g[g].count;
        if (fh.n_waves < 3 || foes == 0) bad = true;
        /* the last wave brings a boss, on the ten stages */
        if (l < FH_STAGES) {
            bool boss = false;
            const FhWave *w = &fh.waves[fh.n_waves - 1];
            for (int g = 0; g < w->n; g++) boss |= FH_FOE[w->g[g].kind].boss != 0;
            if (!boss) bad = true;
        }
        if (!bad) good++;
    }
    memcpy(&fh, &keep, sizeof fh);
    return good;
}

static int kind_named(const char *n) {
    for (int k = 0; k < E_KINDS; k++) {
        const char *a = FH_FOE[k].name;
        if (!strncmp(a, "LORD ", 5)) a += 5;
        char low[16];
        int i = 0;
        for (; a[i] && i < 15; i++) low[i] = (char)(a[i] >= 'A' && a[i] <= 'Z' ? a[i] + 32 : a[i]);
        low[i] = 0;
        if (!strcmp(low, n)) return k;
    }
    return -1;
}

static int unit_named(const char *n) {
    for (int k = 0; k < U_KINDS; k++) {
        const char *a = FH_UNIT[k].name;
        char low[16];
        int i = 0;
        for (; a[i] && i < 15; i++) low[i] = (char)(a[i] >= 'A' && a[i] <= 'Z' ? a[i] + 32 : a[i]);
        low[i] = 0;
        if (!strcmp(low, n)) return k;
    }
    return -1;
}

static int fh_cheat(const char *cmd) {
    int a, b, c;
    char name[24];
    if (sscanf(cmd, "stage %d", &a) == 1) { to_stage(iclamp(a - 1, 0, FH_LEVELS - 1)); return 1; }
    if (!strcmp(cmd, "map")) { to_map(); return 1; }
    if (!strcmp(cmd, "title")) { to_title(); return 1; }
    if (sscanf(cmd, "node %d", &a) == 1) { map_node = iclamp(a - 1, 0, FH_LEVELS - 1); map_from = -1; return 1; }
    if (sscanf(cmd, "meat %d", &a) == 1) { fh.meat = a; return 1; }
    if (sscanf(cmd, "cave %d", &a) == 1) { fh.cave_hp = a; return 1; }
    if (!strcmp(cmd, "god")) { fh.god = 1; return 1; }
    if (!strcmp(cmd, "nospawn")) { fh.no_spawn = 1; return 1; }
    if (!strcmp(cmd, "hold")) { fh.hold_foes = !fh.hold_foes; return 1; }
    if (!strcmp(cmd, "horn")) {
        fh_menu_build(fh.cave_x, fh.cave_y);
        for (int i = 0; i < fh.menu_n; i++)
            if (fh.menu[i].act == M_FIGHT) fh_menu_do(i);
        return 1;
    }
    if (!strcmp(cmd, "autoplay")) { bot_play = !bot_play; return 1; }
    if (!strcmp(cmd, "killall")) { for (int i = 0; i < FH_MAX_FOES; i++) fh.foe[i].on = 0; return 1; }
    if (sscanf(cmd, "wave %d", &a) == 1) { fh.wave = iclamp(a - 1, 0, fh.n_waves - 1); return 1; }
    if (sscanf(cmd, "clear %d %d", &a, &b) == 2) {
        if (a >= 1 && a <= FH_LEVELS) { sv.done[a - 1] = (uint8_t)b; if (b & 1) sv.best_hp[a - 1] = (uint8_t)((b & 2) ? FH_CAVE_HP : 20); }
        save_now();
        return 1;
    }
    if (sscanf(cmd, "found %d", &a) == 1) { sv.found = (uint8_t)a; save_now(); return 1; }
    if (sscanf(cmd, "spawn %23s %d", name, &a) == 2) {
        int k = kind_named(name);
        if (k < 0) return 0;
        fh_add_foe(k, a);
        return 1;
    }
    /* a foe at a point along its route (pixels) */
    if (sscanf(cmd, "foe %23s %d %d", name, &a, &b) == 3) {
        int k = kind_named(name);
        if (k < 0) return 0;
        int i = fh_add_foe(k, a);
        if (i < 0) return 0;
        fh.foe[i].dist = b * FH_U;
        if (!FH_FOE[k].flies) fh_foe_pos_at(fh.foe[i].route, fh.foe[i].dist, &fh.foe[i].x, &fh.foe[i].y);
        return 1;
    }
    /* a foe standing still at a pixel (it keeps walking its route from where it is next frame) */
    if (sscanf(cmd, "unit %23s %d %d", name, &a, &b) == 3) {
        int k = unit_named(name);
        if (k < 0 || a < 0 || b < 0 || a >= FH_W || b >= FH_H) return 0;
        for (int i = 0; i < FH_MAX_UNITS; i++)
            if (!fh.unit[i].on) {
                fh.unit[i] = (FhUnit){1, (uint8_t)k, (uint8_t)a, (uint8_t)b, 0, 10, 0, 0, 99};
                fh.obj[b][a] = O_UNIT;
                fh.oarg[b][a] = (uint8_t)i;
                break;
            }
        return 1;
    }
    if (sscanf(cmd, "hen %d %d %d", &a, &b, &c) == 3) { fh.obj[b][a] = O_HEN; fh.oarg[b][a] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "fire %d %d", &a, &b) == 2) { fh.obj[b][a] = O_FIRE; return 1; }
    if (sscanf(cmd, "pim %d %d %d", &a, &b, &c) == 3) {
        fh.px = a * FH_TU + FH_TU / 2;
        fh.py = b * FH_TU + FH_TU / 2;
        fh.face = c & 3;
        fh.zstate = Z_OK;
        return 1;
    }
    if (sscanf(cmd, "arm %d %d", &a, &b) == 2) { fh.throw_lv = iclamp(a, 0, 2); fh.weapon_lv = iclamp(b, 0, 2); return 1; }
    if (!strcmp(cmd, "win")) {
        /* the last wave, with nobody left in it */
        fh.wave = fh.n_waves - 1;
        fh.phase = PH_BATTLE;
        fh.grp = fh.waves[fh.wave].n;
        for (int i = 0; i < FH_MAX_FOES; i++) fh.foe[i].on = 0;
        return 1;
    }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    return 0;
}

const GameDef GAME_FLINTHOLD = {
    "flinthold",
    "FLINTHOLD",
    "1987",
    "STRATEGY",
    "RALLY THE HEARTH CLAN AND HOLD THE CAVES OF FLINT ISLE AGAINST THE FOUR LORDS' BEASTS!",
    {"REACH THE SECOND VILLAGE", "CLEAR THE LAST STAGE", "CLEAR EVERY STAGE WITH NO CAVE DAMAGE"},
    GLYPH_DPAD "\tWALK\n"
    "HOLD " GLYPH_B "\tTHROW THE WAY YOU FACE\n"
    GLYPH_A "\tBUILD, UPGRADE, SELL, DIG, TALK\n"
    GLYPH_A " AT CAVE\tSOUND THE HORN, ARM UP\n"
    "START\tPAUSE",
    C_TAN, C_ORANGE,
    fh_load, fh_start, fh_update, fh_draw, fh_quit, fh_label, fh_query, fh_cheat,
    "ROCK ON! ISLAND", 30,
};
