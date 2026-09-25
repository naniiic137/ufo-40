/* PETAL PARADE - lead a parade of petalpups to the sun circles.
 * Cartridge 05 of UFO 40, a tribute to Magic Garden (UFO 50 #5).
 * See docs/games/05-petal-parade.md. */
#include "petalparade.h"

#define FX 88            /* field origin on screen */
#define FY 18
#define STEP 8           /* frames per tile */
#define MID 4            /* the frame of a step where Lina is half-way in */
#define AIR 8            /* frames a hop lasts: always clears one half-way check */
#define POWER_COUNT 48   /* the nectar countdown */
#define COUNT_FRAMES 24  /* frames per count */
#define POWER_FRAMES (POWER_COUNT * COUNT_FRAMES)
#define BRAMBLE_EVERY 540
#define BRAMBLE_MOVE 45
#define WITCH_PATIENCE 900
#define RIPEN_FRAMES 720
#define DAZE_FRAMES 240
#define BASE_PUPS 3
#define MAX_TRAIL (PP_N * PP_N)

enum { S_TITLE, S_PLAY, S_DEAD, S_OVER, S_WIN };
enum { C_EMPTY = 0, C_PUP, C_BRAMBLE, C_TOADSTOOL, C_JAR };
enum { D_RIGHT, D_DOWN, D_LEFT, D_UP };
static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};

typedef struct Game {
    int8_t hx, hy, px, py; /* Lina now and one step ago */
    int8_t dir, next_dir;
    uint8_t n_trail;
    int8_t tx[MAX_TRAIL], ty[MAX_TRAIL]; /* the line: 0 is right behind Lina */
    int8_t vac_x, vac_y;                 /* the tile the line just left */
    uint8_t cell[PP_N][PP_N];
    uint8_t pad[PP_N][PP_N];
    uint8_t jar_level[PP_N][PP_N];
    uint16_t jar_age[PP_N][PP_N];
    uint16_t daze[PP_N][PP_N];
    uint16_t saved, pups_bonus;
    uint32_t score;
    uint8_t counter, mult, chain, mush_power;
    uint16_t power, witch_t, bramble_t, move_t;
    uint8_t step_t, air;
    uint8_t spawns_off; /* tests: 1 freezes the random spawns, 2 holds Lina still */
    uint8_t pad_;
    Rng rng;
} Game;

typedef struct Save {
    uint32_t magic;
    uint32_t best;
    uint8_t has_game, pad[3];
    Game g;
} Save;
#define SAVE_MAGIC 0x50500001u

static Save sv;
static Game G;
static int state, state_t, title_sel, frame_t, shake;
static bool sheet_mode;
static Rng seeds;

typedef struct { float x, y, vx, vy; int life, col, kind, val; } Part;
static Part parts[120];

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind, int val) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) { parts[i] = (Part){x, y, vx, vy, life, col, kind, val}; return; }
}

static float cell_cx(int x) { return (float)(FX + x * PP_TILE + PP_TILE / 2); }
static float cell_cy(int y) { return (float)(FY + y * PP_TILE + PP_TILE / 2); }
static bool inb(int x, int y) { return x >= 0 && y >= 0 && x < PP_N && y < PP_N; }

/* ------------------------------------------------------------------ */
/* save                                                                 */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else { memset(&sv, 0, sizeof sv); sv.magic = SAVE_MAGIC; }
}

/* ------------------------------------------------------------------ */
/* the field                                                            */

static bool on_trail(int x, int y) {
    for (int i = 0; i < G.n_trail; i++)
        if (G.tx[i] == x && G.ty[i] == y) return true;
    return false;
}

static bool free_tile(int x, int y) {
    return inb(x, y) && G.cell[y][x] == C_EMPTY && !on_trail(x, y) && !(x == G.hx && y == G.hy);
}

static int dist_to_lina(int x, int y) { return iabs(x - G.hx) + iabs(y - G.hy); }

/* a random free tile at least `away` steps from Lina and off the sun circles */
static bool random_free(int away, bool avoid_pads, int *ox, int *oy) {
    int cand[PP_N * PP_N], n = 0;
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++)
            if (free_tile(x, y) && dist_to_lina(x, y) >= away && !(avoid_pads && G.pad[y][x])) cand[n++] = y * PP_N + x;
    if (n == 0) return false;
    int k = cand[rng_range(&G.rng, 0, n - 1)];
    *ox = k % PP_N;
    *oy = k / PP_N;
    return true;
}

static int count_cells(int kind) {
    int n = 0;
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++) n += G.cell[y][x] == kind;
    return n;
}

static void place_pad(void) {
    int x, y;
    for (int tries = 0; tries < 60; tries++) {
        if (!random_free(3, true, &x, &y)) return;
        /* keep the circles off the walls so they can be reached from all sides */
        if (x == 0 || y == 0 || x == PP_N - 1 || y == PP_N - 1) continue;
        G.pad[y][x] = 1;
        return;
    }
}

static void new_game(void) {
    memset(&G, 0, sizeof G);
    rng_seed(&G.rng, rng_next(&seeds) ^ ((uint64_t)rng_next(&seeds) << 32));
    G.hx = 5; G.hy = 9; G.px = 5; G.py = 10;
    G.dir = D_UP;
    G.next_dir = D_UP;
    G.mult = 1;
    G.vac_x = G.px; G.vac_y = G.py;
    for (int i = 0; i < 3; i++) place_pad();
    state = S_PLAY;
    state_t = 0;
    sv.has_game = 1;
    game_set_pausable(true);
    music_play(PP_MUS_GARDEN);
    memset(parts, 0, sizeof parts);
}

static void game_over(void) {
    if (state != S_PLAY) return;
    state = S_DEAD;
    state_t = 0;
    shake = 14;
    sfx_play_name("pp_bump");
    music_stop();
    if (G.score > sv.best) sv.best = G.score;
    sv.has_game = 0;
    save_now();
}

static void popup(float x, float y, int value) { part_add(x, y, 0, -0.5f, 45, C_YELLOW, 2, value); }

static void burst(float x, float y, int col, int n) {
    for (int i = 0; i < n; i++) {
        float a = (float)i / (float)n * 6.283f;
        part_add(x, y, cosf(a) * 1.3f, sinf(a) * 1.3f - 0.4f, 20 + i % 5, col, 0, 0);
    }
}

/* smash an obstacle while the nectar runs: 10, 20, 30... times the multiplier */
static void smash(int x, int y) {
    G.cell[y][x] = C_EMPTY;
    G.daze[y][x] = 0;
    G.chain++;
    int v = 10 * G.chain * G.mult;
    G.score += (uint32_t)v;
    popup(cell_cx(x), cell_cy(y) - 6, v);
    burst(cell_cx(x), cell_cy(y), C_LIME, 10);
    sfx_play_name("pp_smash");
    shake = imax(shake, 4);
}

static void drink(int x, int y) {
    int lv = G.jar_level[y][x];
    G.cell[y][x] = C_EMPTY;
    if (G.power == 0) { G.mult = 1; G.chain = 0; G.mush_power = 0; }
    if (lv >= 2) G.mult++;          /* green and up raise the multiplier */
    if (lv >= 3) G.mush_power = 1;  /* blue and gold break toadstools */
    if (lv >= 4) G.pups_bonus++;    /* gold: one more pup on the field for good */
    G.power = POWER_FRAMES;
    burst(cell_cx(x), cell_cy(y), lv >= 4 ? C_YELLOW : lv == 3 ? C_SKY : lv == 2 ? C_LEAF : C_RED, 14);
    sfx_play_name("pp_drink");
    music_play(PP_MUS_RUSH);
}

static void spawn_jar(int level) {
    int x, y;
    if (!random_free(2, true, &x, &y)) return;
    G.cell[y][x] = C_JAR;
    G.jar_level[y][x] = (uint8_t)iclamp(level, 1, 4);
    G.jar_age[y][x] = 0;
    sfx_play_name("pp_jar");
}

static void win_game(void) {
    state = S_WIN;
    state_t = 0;
    game_award(GOAL_SAUCER);
    if (G.score >= 20000) game_award(GOAL_ALIEN);
    if (G.score > sv.best) sv.best = G.score;
    sv.has_game = 0;
    save_now();
    music_restart(PP_MUS_END);
    game_set_pausable(false);
}

/* B: let the line go - saved on a sun circle, brambles anywhere else */
static void drop_line(void) {
    int n = G.n_trail;
    if (n == 0) return;
    if (G.pad[G.hy][G.hx]) {
        G.score += (uint32_t)(10 * n * (n + 1) / 2);
        for (int i = 0; i < n; i++) {
            popup(cell_cx(G.tx[i]), cell_cy(G.ty[i]) - 4, 10 * (i + 1));
            burst(cell_cx(G.tx[i]), cell_cy(G.ty[i]), C_PINK, 5);
        }
        G.saved = (uint16_t)imin(PP_GOAL, G.saved + n);
        if (n >= 10) game_award(GOAL_BEACON);
        G.counter = (uint8_t)(G.counter + n);
        if (G.counter >= 6) {
            /* each pup past the six needed ripens the jar a level */
            spawn_jar(1 + (G.counter - 6));
            G.counter = 0;
        }
        G.pad[G.hy][G.hx] = 0;
        place_pad();
        G.witch_t = 0;
        G.n_trail = 0;
        sfx_play_name("pp_save");
        if (G.saved >= PP_GOAL) win_game();
    } else {
        for (int i = 0; i < n; i++) {
            G.cell[G.ty[i]][G.tx[i]] = C_BRAMBLE;
            G.daze[G.ty[i]][G.tx[i]] = 0;
        }
        G.n_trail = 0;
        sfx_play_name("pp_sour");
    }
}

/* What happens when Lina is at (x,y). `entry`: the first frame of the step
 * (only tall things block then); otherwise the half-way frame. */
static void touch(int x, int y, bool entry) {
    if (!inb(x, y)) { game_over(); return; } /* the hedge */
    int c = G.cell[y][x];
    bool air = G.air > 0;
    if (entry) {
        if (c == C_TOADSTOOL && !air) {
            if (G.power && G.mush_power) smash(x, y);
            else game_over();
        }
        return;
    }
    if (air) {
        if (c == C_BRAMBLE) G.daze[y][x] = DAZE_FRAMES; /* hopping over one leaves it dizzy */
        return;
    }
    if (on_trail(x, y)) { game_over(); return; }
    switch (c) {
    case C_BRAMBLE:
        if (G.power) smash(x, y);
        else game_over();
        break;
    case C_TOADSTOOL:
        if (G.power && G.mush_power) smash(x, y);
        else game_over();
        break;
    case C_PUP:
        G.cell[y][x] = C_EMPTY;
        if (G.n_trail < MAX_TRAIL) {
            G.tx[G.n_trail] = G.vac_x;
            G.ty[G.n_trail] = G.vac_y;
            G.n_trail++;
        }
        sfx_play_name("pp_pup");
        part_add(cell_cx(x), cell_cy(y) - 4, 0, -0.6f, 20, C_PINK, 3, 0);
        break;
    case C_JAR:
        drink(x, y);
        break;
    }
}

static void move_lina(void) {
    int d = G.next_dir;
    if ((d + 2) % 4 == G.dir) d = G.dir; /* never straight back */
    G.dir = (int8_t)d;
    G.px = G.hx;
    G.py = G.hy;
    /* the line follows */
    if (G.n_trail > 0) {
        G.vac_x = G.tx[G.n_trail - 1];
        G.vac_y = G.ty[G.n_trail - 1];
        for (int i = G.n_trail - 1; i > 0; i--) { G.tx[i] = G.tx[i - 1]; G.ty[i] = G.ty[i - 1]; }
        G.tx[0] = G.hx;
        G.ty[0] = G.hy;
    } else {
        G.vac_x = G.hx;
        G.vac_y = G.hy;
    }
    G.hx = (int8_t)(G.hx + DX[d]);
    G.hy = (int8_t)(G.hy + DY[d]);
}

static void move_brambles(void) {
    int nx2 = G.hx + DX[G.dir], ny2 = G.hy + DY[G.dir];
    uint8_t moved[PP_N][PP_N];
    memset(moved, 0, sizeof moved);
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++) {
            if (G.cell[y][x] != C_BRAMBLE || moved[y][x] || G.daze[y][x]) continue;
            int d = rng_range(&G.rng, 0, 3);
            int nx = x + DX[d], ny = y + DY[d];
            /* they won't step onto the line, a pup, or right in front of Lina */
            if (!free_tile(nx, ny) || (nx == nx2 && ny == ny2)) continue;
            G.cell[ny][nx] = C_BRAMBLE;
            G.cell[y][x] = C_EMPTY;
            moved[ny][nx] = 1;
        }
}

static void witch_plants(void) {
    int n = G.saved >= 150 ? 3 : G.saved >= 100 ? 2 : 1;
    for (int i = 0; i < n; i++) {
        int x, y;
        if (random_free(3, true, &x, &y)) {
            G.cell[y][x] = C_TOADSTOOL;
            burst(cell_cx(x), cell_cy(y), C_VIOLET, 8);
        }
    }
    sfx_play_name("pp_cackle");
}

static void play_update(void) {
    frame_t++;
    /* steering: remember the latest turn for the next step */
    static const int PAD[4] = {BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_UP};
    for (int d = 0; d < 4; d++)
        if (btnp(PAD[d]) && (d + 2) % 4 != G.dir) G.next_dir = (int8_t)d;
    if (btnp(BTN_A) && G.air == 0) { G.air = AIR; sfx_play_name("pp_hop"); }
    if (btnp(BTN_B)) drop_line();
    if (state != S_PLAY) return;

    if (G.spawns_off & 2) {
        /* tests: Lina holds still */
    } else if (++G.step_t >= STEP) {
        G.step_t = 0;
        move_lina();
        touch(G.hx, G.hy, true);
        if (state != S_PLAY) return;
    } else if (G.step_t == MID) {
        touch(G.hx, G.hy, false);
        if (state != S_PLAY) return;
    }
    if (G.air > 0) G.air--;
    /* the nectar countdown */
    if (G.power > 0 && --G.power == 0) {
        G.mult = 1;
        G.chain = 0;
        G.mush_power = 0;
        music_play(PP_MUS_GARDEN);
    }
    /* dizzy brambles come round */
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++) {
            if (G.daze[y][x] > 0) G.daze[y][x]--;
            if (G.cell[y][x] == C_JAR && ++G.jar_age[y][x] >= RIPEN_FRAMES && G.jar_level[y][x] < 4) {
                G.jar_age[y][x] = 0;
                G.jar_level[y][x]++;
                sfx_play_name("pp_ripen");
            }
        }
    if (!(G.spawns_off & 1)) {
        if (++G.move_t >= BRAMBLE_MOVE) { G.move_t = 0; move_brambles(); }
        if (++G.bramble_t >= BRAMBLE_EVERY) {
            int x, y;
            G.bramble_t = 0;
            if (random_free(3, true, &x, &y)) { G.cell[y][x] = C_BRAMBLE; burst(cell_cx(x), cell_cy(y), C_JADE, 6); }
        }
        if (++G.witch_t >= WITCH_PATIENCE) { G.witch_t = 0; witch_plants(); }
        /* keep the loose pups topped up */
        int want = BASE_PUPS + G.pups_bonus;
        if (count_cells(C_PUP) < want) {
            int x, y;
            if (random_free(3, true, &x, &y)) G.cell[y][x] = C_PUP;
        }
    }
}

/* ------------------------------------------------------------------ */
/* update                                                               */

static void pp_update(void) {
    state_t++;
    if (shake > 0) shake--;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        if (p->kind == 0) p->vy += 0.06f;
    }
    switch (state) {
    case S_TITLE: {
        frame_t++;
        game_set_pausable(false);
        int n = sv.has_game ? 2 : 1;
        if (btnp(BTN_UP) || btnp(BTN_DOWN)) { title_sel = (title_sel + 1) % n; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) game_exit_to_library();
        if (btnp(BTN_A) || btnp(BTN_START)) {
            sfx_play_name("ui_ok");
            input_consume();
            if (sv.has_game && title_sel == 0) {
                G = sv.g;
                state = S_PLAY;
                state_t = 0;
                game_set_pausable(true);
                music_play(G.power ? PP_MUS_RUSH : PP_MUS_GARDEN);
            } else {
                new_game();
            }
        }
        break;
    }
    case S_PLAY: play_update(); break;
    case S_DEAD:
        frame_t++;
        if (state_t > 90) { state = S_OVER; state_t = 0; music_restart(PP_MUS_OVER); game_set_pausable(false); }
        break;
    case S_OVER:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) { state = S_TITLE; state_t = 0; title_sel = 0; music_play(PP_MUS_TITLE); }
        break;
    case S_WIN:
        frame_t++;
        if (state_t > 300 && (btnp(BTN_A) || btnp(BTN_START))) { state = S_TITLE; state_t = 0; title_sel = 0; music_play(PP_MUS_TITLE); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */

typedef struct { uint8_t a, b, edge, hedge, hedge_hi, sky; } Season;
static const Season SEASONS[4] = {
    {C_LEAF, C_LIME, C_JADE, C_FOREST, C_JADE, C_CYAN},   /* spring */
    {C_JADE, C_LEAF, C_FOREST, C_TEAL, C_FOREST, C_SKY},  /* summer */
    {C_EARTH, C_AMBER, C_TAN, C_BROWN, C_TAN, C_ORANGE},  /* autumn */
    {C_NAVY, C_BLUE, C_NIGHT, C_INK, C_NAVY, C_NIGHT},    /* moonlit */
};

static const Season *SS(void) { return &SEASONS[iclamp(G.saved / 50, 0, 3)]; }

static void draw_pad(int x, int y) {
    int cx = FX + x * PP_TILE + 6, cy = FY + y * PP_TILE + 6;
    /* a ring of marigolds turning slowly around a warm patch */
    gfx_dither_circle(cx, cy, 5, C_AMBER, 6);
    float spin = (float)frame_t * 0.03f;
    for (int k = 0; k < 8; k++) {
        float a = (float)k * 0.785f + spin;
        int px = cx + (int)lroundf(cosf(a) * 4.6f), py = cy + (int)lroundf(sinf(a) * 4.6f);
        gfx_rect(px - 1, py - 1, 2, 2, C_ORANGE);
        gfx_pset(px - 1, py - 1, k % 2 ? C_YELLOW : C_AMBER);
    }
    gfx_rect(cx - 1, cy - 1, 2, 2, C_YELLOW);
    gfx_pset(cx - 1, cy - 1, C_CREAM);
}

static void draw_field(void) {
    const Season *s = SS();
    /* the hedge around the garden */
    const int W = PP_N * PP_TILE;
    gfx_rect(FX - 6, FY - 6, W + 12, W + 12, C_INK);
    gfx_rect(FX - 5, FY - 5, W + 10, W + 10, s->hedge);
    for (int i = 0; i < W + 8; i += 4) {
        int off = (i / 4) % 2;
        gfx_circ(FX - 3 + i, FY - 3 + off, 1, s->hedge_hi);
        gfx_circ(FX - 3 + i, FY + W + 2 - off, 1, s->hedge_hi);
        gfx_circ(FX - 3 + off, FY - 3 + i, 1, s->hedge_hi);
        gfx_circ(FX + W + 2 - off, FY - 3 + i, 1, s->hedge_hi);
        if (i % 28 == 12) {
            gfx_pset(FX - 3 + i, FY - 4, C_PINK);
            gfx_pset(FX + W + 3, FY - 3 + i, C_CREAM);
            gfx_pset(FX - 4, FY + 5 + i, C_PINK);
            gfx_pset(FX + 5 + i, FY + W + 3, C_CREAM);
        }
    }
    gfx_rectb(FX - 1, FY - 1, W + 2, W + 2, C_INK);
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++) {
            int px = FX + x * PP_TILE, py = FY + y * PP_TILE;
            gfx_rect(px, py, PP_TILE, PP_TILE, (x + y) & 1 ? s->a : s->b);
            if (((x * 7 + y * 13) % 5) == 0) { gfx_pset(px + 3, py + 8, s->edge); gfx_pset(px + 4, py + 7, s->edge); }
            if (G.pad[y][x]) draw_pad(x, y);
        }
}

static void jar_remap(uint8_t *m, int lv) {
    pal_identity(m);
    static const uint8_t LIQ[5][2] = {{C_RED, C_WINE}, {C_RED, C_WINE}, {C_LEAF, C_JADE}, {C_SKY, C_BLUE}, {C_YELLOW, C_AMBER}};
    m[C_RED] = LIQ[lv][0];
    m[C_WINE] = LIQ[lv][1];
}

static void draw_things(void) {
    uint8_t m[PAL_COUNT];
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++) {
            int px = FX + x * PP_TILE, py = FY + y * PP_TILE;
            int bob = ((frame_t / 12) + x + y) % 2;
            switch (G.cell[y][x]) {
            case C_PUP: spr_draw(&pp_spr[(frame_t / 10 + x) % 2 ? P_PUP1 : P_PUP2], px + 1, py + 1 - bob, 0); break;
            case C_BRAMBLE:
                if (G.daze[y][x]) {
                    spr_draw(&pp_spr[P_BRAMBLE_DAZED], px + 1, py + 1, 0);
                    int k = frame_t / 6;
                    gfx_pset(px + 2 + k % 8, py, C_YELLOW);
                    gfx_pset(px + 9 - k % 8, py + 1, C_WHITE);
                } else {
                    spr_draw(&pp_spr[(frame_t / 14 + x * 3) % 2 ? P_BRAMBLE1 : P_BRAMBLE2], px + 1, py + 1, 0);
                }
                break;
            case C_TOADSTOOL: spr_draw(&pp_spr[(frame_t / 20 + y) % 2 ? P_TOAD1 : P_TOAD2], px, py, 0); break;
            case C_JAR:
                jar_remap(m, G.jar_level[y][x]);
                if (G.jar_level[y][x] >= 4) gfx_dither_circle(px + 6, py + 6, 8, C_YELLOW, 5);
                spr_draw_ex(&pp_spr[P_JAR], px + 1, py - bob, 0, m, -1);
                break;
            }
        }
}

static void draw_line_and_lina(void) {
    float k = (float)G.step_t / STEP;
    /* the line trails a step behind, sliding into place */
    for (int i = G.n_trail - 1; i >= 0; i--) {
        int x0 = i + 1 < G.n_trail ? G.tx[i + 1] : G.vac_x, y0 = i + 1 < G.n_trail ? G.ty[i + 1] : G.vac_y;
        float fx = x0 + (G.tx[i] - x0) * k, fy = y0 + (G.ty[i] - y0) * k;
        int px = FX + (int)(fx * PP_TILE), py = FY + (int)(fy * PP_TILE);
        int bob = ((frame_t / 6) + i) % 2;
        spr_draw(&pp_spr[P_PUP_TRAIL], px + 1, py + 1 - bob, 0);
    }
    float fx = G.px + (G.hx - G.px) * k, fy = G.py + (G.hy - G.py) * k;
    int px = FX + (int)(fx * PP_TILE), py = FY + (int)(fy * PP_TILE);
    int lift = G.air > 0 ? (int)(sinf((float)(AIR - G.air) / AIR * 3.1416f) * 7) : 0;
    if (lift) gfx_dither_circle(px + 6, py + 10, 4, C_INK, 8);
    int spr;
    int fl = 0;
    bool alt = (frame_t / 6) % 2;
    if (state == S_DEAD) spr = P_LINA_FALL;
    else if (lift) spr = P_LINA_HOP;
    else if (G.dir == D_DOWN) spr = alt ? P_LINA_DOWN1 : P_LINA_DOWN2;
    else if (G.dir == D_UP) spr = alt ? P_LINA_UP1 : P_LINA_UP2;
    else { spr = alt ? P_LINA_SIDE1 : P_LINA_SIDE2; fl = G.dir == D_LEFT ? SPR_FLIPX : 0; }
    if (G.power && (frame_t / 3) % 2) spr_draw_outline(&pp_spr[spr], px, py - lift, fl, G.mush_power ? C_SKY : C_YELLOW);
    else spr_draw(&pp_spr[spr], px, py - lift, fl);
}

static void draw_parts(void) {
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        if (p->kind == 2) {
            char buf[12];
            snprintf(buf, sizeof buf, "%d", p->val);
            tiny_draw(buf, (int)p->x - tiny_width(buf) / 2, (int)p->y, (p->life / 3) % 2 ? C_WHITE : p->col);
        } else if (p->kind == 3) {
            spr_draw(&pp_spr[P_HEART], (int)p->x - 3, (int)p->y, 0);
        } else {
            gfx_rect((int)p->x, (int)p->y, 2, 2, p->col);
        }
    }
}

static void draw_hud(void) {
    char buf[48];
    /* left panel: score and progress */
    ui_panel(2, 12, 80, 156, C_NIGHT, C_DUSK);
    tiny_draw("SCORE", 8, 18, C_SLATE);
    snprintf(buf, sizeof buf, "%06u", (unsigned)G.score);
    text_draw(buf, 8, 25, C_WHITE);
    tiny_draw("BEST", 8, 38, C_SLATE);
    snprintf(buf, sizeof buf, "%06u", (unsigned)(G.score > sv.best ? G.score : sv.best));
    text_draw(buf, 8, 45, C_GREY);
    tiny_draw("PUPS SAVED", 8, 60, C_SLATE);
    snprintf(buf, sizeof buf, "%d/%d", G.saved, PP_GOAL);
    text_draw(buf, 8, 67, C_PINK);
    gfx_rect(8, 78, 68, 4, C_INK);
    gfx_rect(8, 78, 68 * G.saved / PP_GOAL, 4, C_PINK);
    for (int k = 1; k < 4; k++) gfx_vline(8 + 17 * k, 78, 81, C_NIGHT);
    tiny_draw("IN LINE", 8, 90, C_SLATE);
    snprintf(buf, sizeof buf, "%d", G.n_trail);
    text_draw(buf, 8, 97, C_WHITE);
    spr_draw(&pp_spr[P_PUP1], 24, 95, 0);
    /* the jar counter: six little jars filling up */
    tiny_draw("NEXT JAR", 8, 114, C_SLATE);
    for (int i = 0; i < 6; i++) {
        int x = 8 + i * 11;
        gfx_rectb(x, 122, 8, 10, C_DUSK);
        if (i < G.counter) gfx_rect(x + 1, 125, 6, 6, C_RED);
    }
    text_draw(GLYPH_A " HOP", 8, 140, C_GREY);
    text_draw(GLYPH_B " LET GO", 8, 150, C_GREY);

    /* right panel: the nectar and the witch */
    ui_panel(238, 12, 80, 156, C_NIGHT, C_DUSK);
    int wy = 18;
    spr_draw(&pp_spr[G.witch_t > WITCH_PATIENCE - 120 && (frame_t / 8) % 2 ? P_WITCH_CACKLE : P_WITCH], 266, wy, 0);
    tiny_draw("MADAME NETTLE", 243, wy + 27, C_VIOLET);
    /* her patience runs out if nobody is saved */
    int pat = 68 * (WITCH_PATIENCE - G.witch_t) / WITCH_PATIENCE;
    gfx_rect(244, wy + 35, 68, 3, C_INK);
    gfx_rect(244, wy + 35, pat, 3, pat < 17 ? C_RED : C_VIOLET);
    tiny_draw("NECTAR", 244, 72, C_SLATE);
    if (G.power) {
        snprintf(buf, sizeof buf, "%d", (G.power + COUNT_FRAMES - 1) / COUNT_FRAMES);
        text_draw(buf, 244, 80, G.power < COUNT_FRAMES * 8 && (frame_t / 4) % 2 ? C_RED : C_YELLOW);
        gfx_rect(244, 91, 68, 4, C_INK);
        gfx_rect(244, 91, 68 * G.power / POWER_FRAMES, 4, G.mush_power ? C_SKY : C_YELLOW);
        snprintf(buf, sizeof buf, "x%d", G.mult);
        text_draw(buf, 290, 80, C_WHITE);
        if (G.mush_power) tiny_draw("BREAKS TOADSTOOLS", 244, 99, C_SKY);
    } else {
        tiny_draw("--", 244, 82, C_DUSK);
    }
    tiny_draw("SEASON", 244, 118, C_SLATE);
    static const char *SEASON_NAME[4] = {"SPRING", "SUMMER", "AUTUMN", "MOONLIGHT"};
    text_draw(SEASON_NAME[iclamp(G.saved / 50, 0, 3)], 244, 126, SS()->b);
    text_draw(GLYPH_DPAD " TURN", 244, 150, C_GREY);
}

static void draw_play(void) {
    int sx = 0, sy = 0;
    if (shake > 0) { sx = (frame_t % 3) - 1; sy = ((frame_t / 2) % 3) - 1; }
    gfx_cls(C_INK);
    for (int i = 0; i < 50; i++) {
        uint32_t h = (uint32_t)(i * 2654435761u);
        gfx_pset((int)(h % 320), (int)((h >> 12) % 180), (i % 7) ? C_NIGHT : C_DUSK);
    }
    gfx_camera(sx, sy);
    draw_field();
    draw_things();
    draw_line_and_lina();
    draw_parts();
    gfx_camera(0, 0);
    draw_hud();
    gfx_rect(88, 0, 144, 10, C_INK);
    tiny_center("PETAL PARADE", 160, 3, C_PINK);
}

static void draw_title(void) {
    gfx_cls(C_NIGHT);
    for (int y = 0; y < 180; y += 2) gfx_hline(0, 319, y, (y / 2 + frame_t / 8) % 9 ? C_NIGHT : C_DUSK);
    /* a garden bed of pups and a sun circle */
    for (int x = 0; x < 320; x += 12) {
        gfx_rect(x, 132, 12, 48, (x / 12) % 2 ? C_LEAF : C_JADE);
    }
    for (int i = 0; i < 26; i++) gfx_circ((i * 29) % 320, 132, 4, C_FOREST);
    gfx_dither_circle(232, 150, 9, C_AMBER, 6);
    for (int k = 0; k < 10; k++) {
        float a = (float)k * 0.628f + (float)frame_t * 0.03f;
        int px = 232 + (int)lroundf(cosf(a) * 9.0f), py = 150 + (int)lroundf(sinf(a) * 9.0f);
        gfx_rect(px - 1, py - 1, 3, 3, C_ORANGE);
        gfx_pset(px - 1, py - 1, k % 2 ? C_YELLOW : C_AMBER);
    }
    for (int i = 0; i < 5; i++) {
        int x = 150 - i * 16 + (int)(sinf(frame_t * 0.1f + i) * 1.5f);
        spr_draw(&pp_spr[i == 0 ? P_LINA_SIDE1 + (frame_t / 8) % 2 : P_PUP_TRAIL], x, 144 - ((frame_t / 6 + i) % 2), 0);
    }
    spr_draw_scaled(&pp_spr[P_LINA_BIG], 36, 60, 2, 0);
    spr_draw(&pp_spr[(frame_t / 30) % 3 == 0 ? P_WITCH_CACKLE : P_WITCH], 262, 88, 0);
    static const uint8_t grad[] = {C_PINK, C_MAGENTA, C_VIOLET, C_PURPLE};
    ui_fancy_center("PETAL PARADE", 160, 18, 3, grad, 4, C_INK, C_PURPLE);
    text_center("LEAD THE PUPS HOME TO THE SUN CIRCLES", 160, 48, C_LIME);
    const char *items[2];
    int n = 0;
    if (sv.has_game) items[n++] = "CONTINUE";
    items[n++] = "NEW GAME";
    for (int i = 0; i < n; i++) {
        int y = 70 + i * 12;
        bool s = i == title_sel;
        text_center(items[i], 160, y, s ? C_WHITE : C_GREY);
        if (s) ui_cursor(160 - text_width(items[i]) / 2 - 10, y, frame_t);
    }
    char buf[40];
    snprintf(buf, sizeof buf, "BEST %06u", (unsigned)sv.best);
    tiny_center(buf, 160, 102, C_YELLOW);
    gfx_rect(0, 170, 320, 10, C_INK);
    tiny_center("D-PAD TURN   A HOP   B LET THE LINE GO", 160, 114, C_GREY);
    text_center(GLYPH_A " START   " GLYPH_B " LIBRARY", 160, 171, C_LIGHT);
}

static void draw_play(void);

static void draw_over(void) {
    /* the garden as it was, dimmed, under a little card */
    draw_play();
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    ui_panel(70, 40, 180, 100, C_INK, C_PINK);
    static const uint8_t grad[] = {C_PINK, C_MAGENTA, C_VIOLET};
    ui_fancy_center("OOPS!", 160, 48, 3, grad, 3, C_INK, C_PURPLE);
    spr_draw_scaled(&pp_spr[P_LINA_FALL], 136, 74, 2, 0);
    spr_draw(&pp_spr[(state_t / 12) % 2 ? P_PUP1 : P_PUP2], 166, 86, 0);
    char buf[64];
    snprintf(buf, sizeof buf, "PUPS SAVED %d", G.saved);
    text_center(buf, 160, 102, C_LIGHT);
    snprintf(buf, sizeof buf, "SCORE %06u  BEST %06u", (unsigned)G.score, (unsigned)sv.best);
    tiny_center(buf, 160, 114, C_YELLOW);
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 126, C_WHITE);
}

static void draw_win(void) {
    int t = state_t;
    gfx_cls(C_NIGHT);
    for (int y = 0; y < 120; y++) gfx_hline(0, 319, y, y < 40 ? C_NAVY : y < 80 ? C_PURPLE : C_WINE);
    gfx_dither(0, 36, 320, 8, C_PURPLE, 8);
    gfx_dither(0, 76, 320, 8, C_WINE, 8);
    for (int x = 0; x < 320; x += 12) gfx_rect(x, 120, 12, 60, (x / 12) % 2 ? C_LEAF : C_JADE);
    for (int i = 0; i < 30; i++) gfx_circ((i * 23) % 320, 120, 4, C_FOREST);
    /* the parade of saved pups, and the witch caught red-handed */
    for (int i = 0; i < 12; i++) {
        int x = (i * 26 + t / 2) % 340 - 10;
        spr_draw(&pp_spr[(t / 8 + i) % 2 ? P_PUP1 : P_PUP2], x, 150 - (t / 6 + i) % 2, 0);
    }
    spr_draw_scaled(&pp_spr[P_LINA_BIG], 90, 72, 2, 0);
    spr_draw_scaled(&pp_spr[(t / 20) % 2 ? P_WITCH : P_WITCH_CACKLE], 190, 72, 2, SPR_FLIPX);
    ui_panel(30, 6, 260, 58, C_INK, C_PINK);
    static const uint8_t grad[] = {C_PINK, C_MAGENTA, C_VIOLET};
    ui_fancy_center("200 PUPS HOME!", 160, 11, 2, grad, 3, C_INK, C_PURPLE);
    if (t > 60) text_center("\"MY TOADSTOOLS!\" WAILS MADAME NETTLE.", 160, 32, C_LIGHT);
    if (t > 120) text_center("\"COME HELP IN MY GARDEN,\" SAYS LINA.", 160, 43, C_LIME);
    if (t > 180) {
        char buf[48];
        snprintf(buf, sizeof buf, "SCORE %06u", (unsigned)G.score);
        text_center(buf, 160, 53, G.score >= 20000 ? C_YELLOW : C_GREY);
    }
    if (t > 300 && (t / 20) % 2) { gfx_rect(80, 166, 160, 11, C_INK); text_center("PRESS " GLYPH_A, 160, 168, C_WHITE); }
}

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    int x = 2, y = 2, rowh = 0;
    for (int i = 0; i < P_SPRITE_COUNT; i++) {
        Sprite *s = &pp_spr[i];
        if (!s->px) continue;
        if (x + s->w * 2 > 318) { x = 2; y += rowh + 2; rowh = 0; }
        spr_draw_scaled(s, x, y, 2, 0);
        x += s->w * 2 + 3;
        if (s->h * 2 > rowh) rowh = s->h * 2;
    }
}

static void pp_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_PLAY: case S_DEAD: draw_play(); break;
    case S_OVER: draw_over(); break;
    case S_WIN: draw_win(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void pp_load(void) {
    pp_art_load();
    pp_audio_load();
}

static void pp_start(void) {
    rng_seed(&seeds, g_rng.state ^ 0x9E7A1ull);
    load_save();
    state = S_TITLE;
    state_t = 0;
    title_sel = 0;
    sheet_mode = false;
    memset(parts, 0, sizeof parts);
    game_set_pausable(false);
    music_play(PP_MUS_TITLE);
}

static void pp_quit(void) {
    if (state == S_PLAY) {
        sv.g = G;
        sv.has_game = 1;
    }
    save_now();
}

static void pp_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_JADE);
    for (int yy = 0; yy < h; yy += 12)
        for (int xx = 0; xx < w; xx += 12) gfx_rect(x + xx, y + yy, 12, 12, ((xx + yy) / 12) % 2 ? C_LEAF : C_JADE);
    for (int k = 0; k < 6; k++) {
        float a = (float)(k + t / 8) * 1.047f;
        gfx_rect(x + 110 + (int)(cosf(a) * 6), y + 40 + (int)(sinf(a) * 6), 2, 2, k % 2 ? C_YELLOW : C_AMBER);
    }
    for (int i = 0; i < 5; i++) {
        int px = x + 72 - i * 12;
        spr_draw(&pp_spr[i == 0 ? P_LINA_SIDE1 + (t / 8) % 2 : P_PUP_TRAIL], px, y + 36 - ((t / 6 + i) % 2), 0);
    }
    spr_draw(&pp_spr[P_BRAMBLE1], x + 20, y + 12, 0);
    spr_draw(&pp_spr[P_TOAD1], x + 120, y + 12, 0);
    spr_draw(&pp_spr[P_PUP1], x + 98, y + 14, 0);
}

static int pp_query(const char *key, int *out) {
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "saved")) { *out = G.saved; return 1; }
    if (!strcmp(key, "score")) { *out = (int)G.score; return 1; }
    if (!strcmp(key, "line")) { *out = G.n_trail; return 1; }
    if (!strcmp(key, "hx")) { *out = G.hx; return 1; }
    if (!strcmp(key, "hy")) { *out = G.hy; return 1; }
    if (!strcmp(key, "dir")) { *out = G.dir; return 1; }
    if (!strcmp(key, "power")) { *out = G.power; return 1; }
    if (!strcmp(key, "mult")) { *out = G.mult; return 1; }
    if (!strcmp(key, "chain")) { *out = G.chain; return 1; }
    if (!strcmp(key, "counter")) { *out = G.counter; return 1; }
    if (!strcmp(key, "mush_power")) { *out = G.mush_power; return 1; }
    if (!strcmp(key, "pups")) { *out = count_cells(C_PUP); return 1; }
    if (!strcmp(key, "brambles")) { *out = count_cells(C_BRAMBLE); return 1; }
    if (!strcmp(key, "toadstools")) { *out = count_cells(C_TOADSTOOL); return 1; }
    if (!strcmp(key, "jars")) { *out = count_cells(C_JAR); return 1; }
    if (!strcmp(key, "bonus")) { *out = G.pups_bonus; return 1; }
    if (!strcmp(key, "has_game")) { *out = sv.has_game; return 1; }
    if (!strcmp(key, "pads")) { int n = 0; for (int y = 0; y < PP_N; y++) for (int x = 0; x < PP_N; x++) n += G.pad[y][x]; *out = n; return 1; }
    if (!strcmp(key, "jar_level")) {
        *out = 0;
        for (int y = 0; y < PP_N; y++) for (int x = 0; x < PP_N; x++) if (G.cell[y][x] == C_JAR) *out = G.jar_level[y][x];
        return 1;
    }
    if (!strncmp(key, "daze_", 5)) { int x = atoi(key + 5), y = atoi(strchr(key + 5, '_') + 1); *out = G.daze[y][x]; return 1; }
    if (!strncmp(key, "cell_", 5)) { int x = atoi(key + 5), y = atoi(strchr(key + 5, '_') + 1); *out = G.cell[y][x]; return 1; }
    return 0;
}

static int pp_cheat(const char *cmd) {
    int a, b, c;
    char name[16];
    if (!strcmp(cmd, "new")) { new_game(); return 1; }
    if (!strcmp(cmd, "clear")) {
        /* an empty field, no random spawns, Lina at (5,9) heading up */
        memset(G.cell, 0, sizeof G.cell);
        memset(G.pad, 0, sizeof G.pad);
        memset(G.daze, 0, sizeof G.daze);
        G.n_trail = 0;
        G.hx = 5; G.hy = 9; G.px = 5; G.py = 10;
        G.dir = G.next_dir = D_UP;
        G.step_t = 1;
        G.spawns_off = 1;
        return 1;
    }
    if (sscanf(cmd, "put %15s %d %d", name, &a, &b) == 3) {
        int kind = !strcmp(name, "pup") ? C_PUP : !strcmp(name, "bramble") ? C_BRAMBLE : !strcmp(name, "toadstool") ? C_TOADSTOOL
                 : !strcmp(name, "jar") ? C_JAR : !strcmp(name, "pad") ? 99 : -1;
        if (kind < 0 || !inb(a, b)) return 0;
        if (kind == 99) G.pad[b][a] = 1;
        else { G.cell[b][a] = (uint8_t)kind; if (kind == C_JAR) { G.jar_level[b][a] = 1; G.jar_age[b][a] = 0; } }
        return 1;
    }
    if (sscanf(cmd, "jarlevel %d %d %d", &a, &b, &c) == 3) { G.jar_level[b][a] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "turn %d", &a) == 1) { G.next_dir = (int8_t)(a & 3); return 1; }
    if (sscanf(cmd, "saved %d", &a) == 1) { G.saved = (uint16_t)a; return 1; }
    if (sscanf(cmd, "score %d", &a) == 1) { G.score = (uint32_t)a; return 1; }
    if (sscanf(cmd, "counter %d", &a) == 1) { G.counter = (uint8_t)a; return 1; }
    if (sscanf(cmd, "witch %d", &a) == 1) { G.witch_t = (uint16_t)a; return 1; }
    if (!strcmp(cmd, "spawns_on")) { G.spawns_off &= ~1; return 1; }
    if (!strcmp(cmd, "still")) { G.spawns_off ^= 2; return 1; }
    if (sscanf(cmd, "lina %d %d %d", &a, &b, &c) == 3) {
        G.hx = (int8_t)a; G.hy = (int8_t)b;
        G.dir = G.next_dir = (int8_t)(c & 3);
        G.px = (int8_t)(a - DX[c & 3]); G.py = (int8_t)(b - DY[c & 3]);
        G.vac_x = G.px; G.vac_y = G.py;
        G.step_t = 1;
        return 1;
    }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    return 0;
}

const GameDef GAME_PETALPARADE = {
    "petalparade",
    "PETAL PARADE",
    "1984",
    "ARCADE",
    "LEAD LOST PETALPUPS HOME. DON'T TRIP OVER YOUR OWN PARADE.",
    {"SAVE 10 PUPS AT ONCE", "SAVE 200 PUPS", "200 PUPS AND 20,000 POINTS"},
    "D-PAD\tTURN\n"
    GLYPH_A "\tHOP OVER A TILE\n"
    GLYPH_B "\tLET THE LINE GO\n"
    "START\tPAUSE\n\n"
    "LET GO ON A SUN CIRCLE TO SAVE THEM.\n"
    "ANYWHERE ELSE THEY TURN INTO BRAMBLES.",
    C_PINK, C_LEAF,
    pp_load, pp_start, pp_update, pp_draw, pp_quit, pp_label, pp_query, pp_cheat,
    "MAGIC GARDEN", 5,
};
