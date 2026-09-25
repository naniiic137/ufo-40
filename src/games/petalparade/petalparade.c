/* PETAL PARADE - lead a parade of petalpups to the sun circles.
 * Cartridge 05 of UFO 40, a tribute to Magic Garden (UFO 50 #5).
 * Every rule and where it comes from is in docs/games/05-petal-parade.md. */
#include "petalparade.h"

#define FX 88              /* field origin on screen */
#define FY 18
#define STEP 8             /* frames per tile, the same all game */
#define MID 4              /* the frame of a step where Posy is half-way in */
#define AIR 8              /* frames a hop lasts: always exactly one half-way check */
#define POWER_COUNT 48     /* the nectar countdown */
#define COUNT_FRAMES 10    /* frames per count: 6 a second, so 8 s in all */
#define POWER_FRAMES (POWER_COUNT * COUNT_FRAMES)
#define BRAMBLE_EVERY 540  /* a bramble crawls out on its own every 9 s */
#define BRAMBLE_HOP 45     /* each bramble hops a tile every 3/4 s */
#define STAR_LIFE 600      /* a sun circle lasts about 10 s */
#define STAR_FLASH 120     /* and flashes for its last 2 */
#define RIPEN_FRAMES 480   /* a jar left on the ground ripens every 8 s */
#define DAZE_FRAMES 240    /* hopped over, a bramble is dizzy for 4 s */
#define HITCH 6            /* the garden holds still a moment on a smash */
#define SPIN 12            /* Posy's twirl when she lets the line go */
#define SPROUT 24          /* a new pup grows out of the ground before it can join */
#define BASE_PUPS 3
#define HISCORES 5
#define MAX_TRAIL (PP_N * PP_N)

enum { S_TITLE, S_PLAY, S_DEAD, S_OVER, S_WIN };
enum { C_EMPTY = 0, C_PUP, C_BRAMBLE, C_TOADSTOOL, C_JAR };
enum { D_RIGHT, D_DOWN, D_LEFT, D_UP };
enum { STAR_LINE, STAR_RING, STAR_CROSS };
static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
static const int PAD_BTN[4] = {BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_UP};

/* spawns_off bits (tests) */
enum { OFF_SPAWNS = 1, OFF_POSY = 2, OFF_HOPS = 4 };

typedef struct Game {
    int8_t hx, hy, px, py; /* Posy now and one step ago */
    int8_t dir, next_dir;
    uint8_t n_trail;
    int8_t tx[MAX_TRAIL], ty[MAX_TRAIL]; /* the line: 0 is right behind Posy */
    int8_t vac_x, vac_y;                 /* the tile the line just left */
    uint8_t cell[PP_N][PP_N];
    uint8_t pad[PP_N][PP_N];             /* the sun circle's tiles */
    uint8_t look[PP_N][PP_N];            /* where each bramble will hop next */
    uint8_t hop[PP_N][PP_N];             /* frames until it hops */
    uint8_t jar_level[PP_N][PP_N];
    uint16_t jar_age[PP_N][PP_N];
    uint16_t daze[PP_N][PP_N];
    uint16_t saved, pups_bonus;
    uint32_t score;
    uint8_t counter, mult, chain, mush_power;
    uint16_t power, star_t, bramble_t;
    uint8_t star_used, star_kind;
    uint8_t step_t, air, freeze, spin, cackle, recorded;
    uint8_t sprout[PP_N][PP_N];          /* frames until a new pup is up */
    uint32_t frames, powered_frames;     /* for the economy test */
    uint8_t spawns_off;
    Rng rng;
} Game;

/* The save keeps only the score board: like Magic Garden, a run is never
 * saved half-way. */
typedef struct Save {
    uint32_t magic;
    uint32_t hi[HISCORES];
} Save;
#define SAVE_MAGIC 0x50500002u
#define SAVE_MAGIC_V1 0x50500001u /* first version: a best score and a suspended game */

static Save sv;
static Game G;
static int state, state_t, frame_t, shake, new_rank = -1;
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
/* save and the score board                                             */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    static union { Save now; uint8_t raw[4096]; } tmp;
    memset(&sv, 0, sizeof sv);
    sv.magic = SAVE_MAGIC;
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof(Save) && tmp.now.magic == SAVE_MAGIC) sv = tmp.now;
    else if (n >= 8 && tmp.now.magic == SAVE_MAGIC_V1) sv.hi[0] = tmp.now.hi[0]; /* keep the old best score */
}

/* the run is over: its score goes on the board (once) */
static void record_score(void) {
    if (G.recorded) return;
    G.recorded = 1;
    new_rank = -1;
    for (int i = 0; i < HISCORES; i++)
        if (G.score > sv.hi[i]) {
            for (int j = HISCORES - 1; j > i; j--) sv.hi[j] = sv.hi[j - 1];
            sv.hi[i] = G.score;
            new_rank = i;
            break;
        }
    save_now();
}

/* ------------------------------------------------------------------ */
/* the field                                                            */

static bool on_trail(int x, int y) {
    for (int i = 0; i < G.n_trail; i++)
        if (G.tx[i] == x && G.ty[i] == y) return true;
    return false;
}

static bool free_tile(int x, int y) {
    return inb(x, y) && G.cell[y][x] == C_EMPTY && !on_trail(x, y) && !(x == G.hx && y == G.hy) && !(x == G.px && y == G.py);
}

static int dist_to_posy(int x, int y) { return iabs(x - G.hx) + iabs(y - G.hy); }

/* a random free tile at least `away` steps from Posy (and off the sun circle) */
static bool random_free(int away, bool avoid_pads, int *ox, int *oy) {
    int cand[PP_N * PP_N], n = 0;
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++)
            if (free_tile(x, y) && dist_to_posy(x, y) >= away && !(avoid_pads && G.pad[y][x])) cand[n++] = y * PP_N + x;
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

static int count_pads(void) {
    int n = 0;
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++) n += G.pad[y][x] != 0;
    return n;
}

/* A new sun circle somewhere else: a line right across the garden, a
 * square ring or a cross, as Magic Garden's star tiles are. */
static void place_star(int life) {
    uint8_t m[PP_N][PP_N];
    int kind = rng_range(&G.rng, 0, 2);
    for (int tries = 0; tries < 40; tries++) {
        memset(m, 0, sizeof m);
        if (kind == STAR_LINE) {
            int k = rng_range(&G.rng, 1, PP_N - 2);
            bool row = rng_chance(&G.rng, 50);
            for (int i = 0; i < PP_N; i++) {
                if (row) m[k][i] = 1;
                else m[i][k] = 1;
            }
        } else if (kind == STAR_RING) {
            int s = rng_range(&G.rng, 4, 6);
            int x0 = rng_range(&G.rng, 1, PP_N - 1 - s), y0 = rng_range(&G.rng, 1, PP_N - 1 - s);
            for (int i = 0; i < s; i++) {
                m[y0][x0 + i] = m[y0 + s - 1][x0 + i] = 1;
                m[y0 + i][x0] = m[y0 + i][x0 + s - 1] = 1;
            }
        } else {
            int a = rng_range(&G.rng, 2, 3);
            int cx = rng_range(&G.rng, 1 + a, PP_N - 2 - a), cy = rng_range(&G.rng, 1 + a, PP_N - 2 - a);
            for (int d = -a; d <= a; d++) m[cy][cx + d] = m[cy + d][cx] = 1;
        }
        /* not right under Posy's feet */
        if (!m[G.hy][G.hx] || tries == 39) break;
    }
    memcpy(G.pad, m, sizeof m);
    G.star_kind = (uint8_t)kind;
    G.star_t = (uint16_t)life;
    G.star_used = 0;
}

static void new_bramble(int x, int y) {
    G.cell[y][x] = C_BRAMBLE;
    G.daze[y][x] = 0;
    G.look[y][x] = (uint8_t)rng_range(&G.rng, 0, 3);
    G.hop[y][x] = (uint8_t)rng_range(&G.rng, BRAMBLE_HOP / 2, BRAMBLE_HOP);
}

static void new_game(void) {
    memset(&G, 0, sizeof G);
    rng_seed(&G.rng, rng_next(&seeds) ^ ((uint64_t)rng_next(&seeds) << 32));
    G.hx = 5; G.hy = 9; G.px = 5; G.py = 10;
    G.dir = D_UP;
    G.next_dir = D_UP;
    G.mult = 1;
    G.vac_x = G.px; G.vac_y = G.py;
    place_star(STAR_LIFE);
    state = S_PLAY;
    state_t = 0;
    new_rank = -1;
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
    record_score();
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
    G.freeze = HITCH;
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
    G.power = POWER_FRAMES;         /* any jar starts the countdown again */
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
    record_score();
    music_restart(PP_MUS_END);
    game_set_pausable(false);
}

/* B: Posy twirls and lets the whole line go. Every pup standing on the sun
 * circle is saved and scores 10 x its place in the line; every other pup
 * turns into a bramble where it stands. */
static void drop_line(void) {
    G.spin = SPIN;
    int n = G.n_trail;
    if (n == 0) return;
    int saved = 0;
    uint32_t pts = 0;
    for (int i = 0; i < n; i++) {
        int x = G.tx[i], y = G.ty[i];
        if (G.pad[y][x]) {
            saved++;
            pts += (uint32_t)(10 * (i + 1));
            popup(cell_cx(x), cell_cy(y) - 4, 10 * (i + 1));
            burst(cell_cx(x), cell_cy(y), C_PINK, 5);
        } else if (G.cell[y][x] == C_EMPTY) {
            new_bramble(x, y);
            burst(cell_cx(x), cell_cy(y), C_JADE, 4);
        }
    }
    G.n_trail = 0;
    G.score += pts;
    if (saved == 0) { sfx_play_name("pp_sour"); return; }
    G.saved = (uint16_t)(G.saved + saved);
    G.star_used = 1;
    if (saved >= 10) game_award(GOAL_BEACON);
    /* the jar counter: at six a jar appears, a level riper per pup past six */
    G.counter = (uint8_t)(G.counter + saved);
    if (G.counter >= 6) {
        spawn_jar(1 + (G.counter - 6));
        G.counter = 0;
    }
    sfx_play_name("pp_save");
    if (G.saved >= PP_GOAL) win_game();
}

/* What happens when Posy is at (x,y). `entry`: the first frame of the step
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
        if (G.sprout[y][x]) break; /* still growing: she walks over it */
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

/* one tile on: the turn waiting for this tile happens now */
static void move_posy(void) {
    int d = G.next_dir;
    if ((d + 2) % 4 == G.dir) d = G.dir; /* never straight back */
    G.dir = (int8_t)d;
    G.next_dir = (int8_t)d;
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

/* brambles won't hop onto the line, a loose pup, Posy or anything else */
static bool bramble_free(int x, int y) { return free_tile(x, y); }

static void pick_look(int x, int y) {
    int opts[4], n = 0;
    for (int d = 0; d < 4; d++)
        if (inb(x + DX[d], y + DY[d])) opts[n++] = d;
    G.look[y][x] = (uint8_t)opts[rng_range(&G.rng, 0, n - 1)];
}

/* Each bramble looks the way it will hop, then hops one tile. If that tile
 * is taken it hops another free way instead. */
static void move_brambles(void) {
    uint8_t done[PP_N][PP_N];
    memset(done, 0, sizeof done);
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++) {
            if (G.cell[y][x] != C_BRAMBLE || done[y][x] || G.daze[y][x]) continue;
            if (G.hop[y][x] > 1) { G.hop[y][x]--; continue; }
            int d = G.look[y][x];
            if (!bramble_free(x + DX[d], y + DY[d])) {
                int opts[4], n = 0;
                for (int k = 0; k < 4; k++)
                    if (bramble_free(x + DX[k], y + DY[k])) opts[n++] = k;
                d = n ? opts[rng_range(&G.rng, 0, n - 1)] : -1;
            }
            int nx = x, ny = y;
            if (d >= 0) {
                nx = x + DX[d];
                ny = y + DY[d];
                G.cell[ny][nx] = C_BRAMBLE;
                G.cell[y][x] = C_EMPTY;
                G.daze[ny][nx] = 0;
            }
            done[ny][nx] = 1;
            G.hop[ny][nx] = BRAMBLE_HOP;
            pick_look(nx, ny);
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
    G.cackle = 60;
    sfx_play_name("pp_cackle");
}

static void play_update(void) {
    frame_t++;
    /* steering: a turn pressed now happens at the next tile; a direction
     * held down steers too */
    for (int d = 0; d < 4; d++)
        if (btnp(PAD_BTN[d]) && (d + 2) % 4 != G.dir) G.next_dir = (int8_t)d;
    if (G.next_dir == G.dir)
        for (int d = 0; d < 4; d++)
            if (btn(PAD_BTN[d]) && d != G.dir && (d + 2) % 4 != G.dir) { G.next_dir = (int8_t)d; break; }
    /* the hop is instant */
    if (btnp(BTN_A) && G.air == 0) { G.air = AIR; sfx_play_name("pp_hop"); }
    if (btnp(BTN_B)) drop_line();
    if (state != S_PLAY) return;
    if (G.spin > 0) G.spin--;
    if (G.cackle > 0) G.cackle--;
    G.frames++;
    if (G.power) G.powered_frames++;
    if (G.freeze > 0) { G.freeze--; return; } /* the little hitch after a smash */

    if (G.spawns_off & OFF_POSY) {
        /* tests: Posy holds still */
    } else if (++G.step_t >= STEP) {
        G.step_t = 0;
        /* centred on her tile, she touches what is straight ahead: with
         * nectar it is smashed now, so a turn waiting for this tile can
         * still take her the other way */
        int ax = G.hx + DX[G.dir], ay = G.hy + DY[G.dir];
        if (G.power && !G.air && inb(ax, ay) &&
            (G.cell[ay][ax] == C_BRAMBLE || (G.cell[ay][ax] == C_TOADSTOOL && G.mush_power)))
            smash(ax, ay);
        move_posy();
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
    /* dizzy brambles come round; jars ripen */
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++) {
            if (G.daze[y][x] > 0) G.daze[y][x]--;
            if (G.sprout[y][x] > 0) G.sprout[y][x]--;
            if (G.cell[y][x] == C_JAR && ++G.jar_age[y][x] >= RIPEN_FRAMES && G.jar_level[y][x] < 4) {
                G.jar_age[y][x] = 0;
                G.jar_level[y][x]++;
                sfx_play_name("pp_ripen");
            }
        }
    if (!(G.spawns_off & OFF_HOPS)) move_brambles();
    /* the sun circle moves on; if nobody was saved on it, the witch plants */
    if (G.star_t > 0 && --G.star_t == 0) {
        if (!G.star_used) witch_plants();
        place_star(STAR_LIFE);
    }
    if (!(G.spawns_off & OFF_SPAWNS)) {
        if (++G.bramble_t >= BRAMBLE_EVERY) {
            int x, y;
            G.bramble_t = 0;
            if (random_free(3, true, &x, &y)) { new_bramble(x, y); burst(cell_cx(x), cell_cy(y), C_JADE, 6); }
        }
        /* keep the loose pups topped up; a new one sprouts first */
        int want = BASE_PUPS + G.pups_bonus;
        if (count_cells(C_PUP) < want) {
            int x, y;
            if (random_free(3, true, &x, &y)) { G.cell[y][x] = C_PUP; G.sprout[y][x] = SPROUT; }
        }
    }
}

/* ------------------------------------------------------------------ */
/* the demo player: it chooses real buttons each frame for the runner's */
/* "bot" query (botplay / botuntil in the tests)                        */

enum { BOT_COLLECT, BOT_DELIVER, BOT_KILL };

/* can Posy walk into (x,y) k tiles from now without a hop? */
static bool bot_wants_jar(int x, int y);

static bool bot_blocked(int x, int y, int k) {
    if (!inb(x, y)) return true;
    /* a jar it is saving for later is stepped round, not drunk by accident */
    if (G.cell[y][x] == C_JAR && k < 99 && !bot_wants_jar(x, y)) return true;
    for (int i = 0; i < G.n_trail; i++)
        if (G.tx[i] == x && G.ty[i] == y && i <= G.n_trail - k) return true;
    int c = G.cell[y][x];
    bool powered = G.power > k * STEP + 12;
    if (c == C_BRAMBLE && !powered) return true;
    if (c == C_TOADSTOOL && !(powered && G.mush_power)) return true;
    if (!powered)
        for (int d = 0; d < 4; d++) {
            int bx = x - DX[d], by = y - DY[d];
            if (inb(bx, by) && G.cell[by][bx] == C_BRAMBLE && !G.daze[by][bx] && G.look[by][bx] == d) return true;
        }
    return false;
}

/* It drinks a jar only when there is something to clear with it: a bramble,
 * or a toadstool and a jar ripe enough (blue or gold) to break it. */
static bool bot_wants_jar(int x, int y) {
    return count_cells(C_BRAMBLE) >= 2 || (G.jar_level[y][x] >= 3 && count_cells(C_TOADSTOOL) > 0);
}

static bool bot_target(int mode, int x, int y, int run) {
    int c = G.cell[y][x];
    if (mode == BOT_KILL) return c == C_BRAMBLE || (c == C_TOADSTOOL && G.mush_power);
    if (mode == BOT_DELIVER) return run >= G.n_trail;
    return (c == C_PUP && !G.sprout[y][x]) || (c == C_JAR && bot_wants_jar(x, y));
}

/* breadth-first over tiles, turning but never reversing; returns the first
 * direction of the shortest way to a target, or -1 */
static int bot_plan(int mode) {
    typedef struct { int8_t x, y, d, run, first; int16_t k; } Node;
    static Node q[PP_N * PP_N * 8];
    static uint8_t seen[PP_N][PP_N][8];
    memset(seen, 0, sizeof seen);
    int run0 = 0;
    if (G.pad[G.hy][G.hx]) {
        run0 = 1;
        for (int i = 0; i < G.n_trail && G.pad[G.ty[i]][G.tx[i]]; i++) run0++;
    }
    int head = 0, tail = 0;
    q[tail++] = (Node){G.hx, G.hy, G.dir, (int8_t)imin(run0, 7), -1, 0};
    while (head < tail) {
        Node n = q[head++];
        if (n.k > 40) continue;
        for (int d = 0; d < 4; d++) {
            if ((d + 2) % 4 == n.d) continue;
            int nx = n.x + DX[d], ny = n.y + DY[d], k = n.k + 1;
            if (bot_blocked(nx, ny, k)) continue;
            int run = G.pad[ny][nx] ? imin(n.run + 1, 7) : 0;
            if (seen[ny][nx][run]) continue;
            seen[ny][nx][run] = 1;
            int first = n.first < 0 ? d : n.first;
            if (bot_target(mode, nx, ny, run)) return first;
            if (tail < ARRAY_LEN(q)) q[tail++] = (Node){(int8_t)nx, (int8_t)ny, (int8_t)d, (int8_t)run, (int8_t)first, (int16_t)k};
        }
    }
    return -1;
}

/* how much room is there beyond (x,y)? */
static int bot_room(int x, int y) {
    uint8_t seen[PP_N][PP_N];
    int stack[PP_N * PP_N], n = 0, count = 0;
    memset(seen, 0, sizeof seen);
    if (bot_blocked(x, y, 1)) return 0;
    seen[y][x] = 1;
    stack[n++] = y * PP_N + x;
    while (n > 0) {
        int t = stack[--n];
        count++;
        for (int d = 0; d < 4; d++) {
            int ax = t % PP_N + DX[d], ay = t / PP_N + DY[d];
            if (inb(ax, ay) && !seen[ay][ax] && !bot_blocked(ax, ay, 99)) { seen[ay][ax] = 1; stack[n++] = ay * PP_N + ax; }
        }
    }
    return count;
}

static int bot_buttons(void) {
    if (state == S_TITLE || state == S_OVER || state == S_WIN) return (state_t / 8) % 2 ? BTN_A : 0;
    if (state != S_PLAY) return 0;
    int mask = 0;
    bool kill = G.power > 90 && (count_cells(C_BRAMBLE) > 0 || (G.mush_power && count_cells(C_TOADSTOOL) > 0));
    bool pads = count_pads() > 0;
    int d = -1;
    if (kill) d = bot_plan(BOT_KILL);
    if (d < 0 && G.n_trail >= 2 && pads) d = bot_plan(BOT_DELIVER);
    if (d < 0) d = bot_plan(BOT_COLLECT);
    if (d < 0 && G.n_trail >= 1 && pads) d = bot_plan(BOT_DELIVER);
    if (d < 0) {
        /* nowhere to go: keep to the roomiest way */
        int best = -1;
        for (int k = 0; k < 4; k++) {
            if ((k + 2) % 4 == G.dir) continue;
            int r = bot_room(G.hx + DX[k], G.hy + DY[k]);
            if (r > best) { best = r; d = k; }
        }
        /* nothing safe: hop over whatever is ahead if the tile past it is clear */
        if (best <= 0) {
            d = G.dir;
            for (int k = 0; k < 4; k++) {
                if ((k + 2) % 4 == G.dir) continue;
                if (inb(G.hx + 2 * DX[k], G.hy + 2 * DY[k]) && !bot_blocked(G.hx + 2 * DX[k], G.hy + 2 * DY[k], 2)) { d = k; break; }
            }
        }
    }
    mask |= PAD_BTN[d];
    int nx = G.hx + DX[d], ny = G.hy + DY[d];
    if (bot_blocked(nx, ny, 1) && G.step_t == MID + 1 && G.air == 0) mask |= BTN_A;
    /* let the line go once every pup in it stands on the sun circle */
    if (G.n_trail > 0) {
        int on = 0;
        for (int i = 0; i < G.n_trail; i++) on += G.pad[G.ty[i]][G.tx[i]] != 0;
        if (on == G.n_trail || (on > 0 && G.star_t > 0 && G.star_t < 20)) mask |= BTN_B;
    }
    return mask;
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
    case S_TITLE:
        frame_t++;
        game_set_pausable(false);
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) {
            sfx_play_name("ui_ok");
            input_consume();
            new_game();
        }
        break;
    case S_PLAY: play_update(); break;
    case S_DEAD:
        frame_t++;
        if (state_t > 90) { state = S_OVER; state_t = 0; music_restart(PP_MUS_OVER); game_set_pausable(false); }
        break;
    case S_OVER:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) { state = S_TITLE; state_t = 0; music_play(PP_MUS_TITLE); }
        break;
    case S_WIN:
        frame_t++;
        if (state_t > 300 && (btnp(BTN_A) || btnp(BTN_START))) { state = S_TITLE; state_t = 0; music_play(PP_MUS_TITLE); }
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
    /* the sun circle flashes before it moves on */
    bool show = !(G.star_t > 0 && G.star_t < STAR_FLASH && (G.star_t / 6) % 2);
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++) {
            int px = FX + x * PP_TILE, py = FY + y * PP_TILE;
            gfx_rect(px, py, PP_TILE, PP_TILE, (x + y) & 1 ? s->a : s->b);
            if (((x * 7 + y * 13) % 5) == 0) { gfx_pset(px + 3, py + 8, s->edge); gfx_pset(px + 4, py + 7, s->edge); }
            if (G.pad[y][x] && show) draw_pad(x, y);
        }
}

static void jar_remap(uint8_t *m, int lv) {
    pal_identity(m);
    static const uint8_t LIQ[5][2] = {{C_RED, C_WINE}, {C_RED, C_WINE}, {C_LEAF, C_JADE}, {C_SKY, C_BLUE}, {C_YELLOW, C_AMBER}};
    m[C_RED] = LIQ[lv][0];
    m[C_WINE] = LIQ[lv][1];
}

/* a bramble's eyes turn to where it will hop: each eye is 2 x 2, the pupil
 * sits in the corner facing that way (up and down: the inner corners) */
static void draw_bramble_eyes(int px, int py, int look) {
    for (int e = 0; e < 2; e++) {
        int ex = px + 3 + e * 4, ey = py + 5;
        gfx_rect(ex, ey, 2, 2, C_YELLOW);
        int ux = look == D_RIGHT ? 1 : look == D_LEFT ? 0 : 1 - e;
        int uy = look == D_UP ? 0 : 1;
        gfx_pset(ex + ux, ey + uy, C_INK);
    }
}

static void draw_things(void) {
    uint8_t m[PAL_COUNT];
    for (int y = 0; y < PP_N; y++)
        for (int x = 0; x < PP_N; x++) {
            int px = FX + x * PP_TILE, py = FY + y * PP_TILE;
            int bob = ((frame_t / 12) + x + y) % 2;
            switch (G.cell[y][x]) {
            case C_PUP:
                if (G.sprout[y][x]) {
                    /* a shoot pushing up, two leaves, then the pup's ears */
                    int g = (SPROUT - G.sprout[y][x]) * 6 / SPROUT;
                    gfx_rect(px + 5, py + 10 - g, 2, g + 1, C_FOREST);
                    gfx_rect(px + 3, py + 9 - g, 2, 1, C_LIME);
                    gfx_rect(px + 7, py + 8 - g, 2, 1, C_LIME);
                    if (g >= 4) { gfx_pset(px + 4, py + 6 - g, C_PINK); gfx_pset(px + 7, py + 6 - g, C_PINK); }
                    gfx_dither(px + 2, py + 10, 8, 2, C_EARTH, 8);
                } else {
                    spr_draw(&pp_spr[(frame_t / 10 + x) % 2 ? P_PUP1 : P_PUP2], px + 1, py + 1 - bob, 0);
                }
                break;
            case C_BRAMBLE:
                if (G.daze[y][x]) {
                    spr_draw(&pp_spr[P_BRAMBLE_DAZED], px + 1, py + 1, 0);
                    int k = frame_t / 6;
                    gfx_pset(px + 2 + k % 8, py, C_YELLOW);
                    gfx_pset(px + 9 - k % 8, py + 1, C_WHITE);
                } else {
                    /* it leans the way it looks just before it hops */
                    int lk = G.look[y][x], lean = G.hop[y][x] < 10 ? 1 : 0;
                    int lx = px + DX[lk] * lean, ly = py + DY[lk] * lean;
                    spr_draw(&pp_spr[(frame_t / 14 + x * 3) % 2 ? P_BRAMBLE1 : P_BRAMBLE2], lx + 1, ly + 1, 0);
                    draw_bramble_eyes(lx, ly, lk);
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

static void draw_line_and_posy(void) {
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
    int face = G.dir;
    if (G.spin > 0) face = (G.dir + (SPIN - G.spin) / 3) % 4; /* the twirl */
    if (state == S_DEAD) spr = P_POSY_FALL;
    else if (lift) spr = P_POSY_HOP;
    else if (face == D_DOWN) spr = alt ? P_POSY_DOWN1 : P_POSY_DOWN2;
    else if (face == D_UP) spr = alt ? P_POSY_UP1 : P_POSY_UP2;
    else { spr = alt ? P_POSY_SIDE1 : P_POSY_SIDE2; fl = face == D_LEFT ? SPR_FLIPX : 0; }
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
    /* left: the score, the pups saved and the six-jar counter */
    ui_panel(2, 12, 80, 156, C_NIGHT, C_DUSK);
    tiny_draw("SCORE", 8, 20, C_SLATE);
    snprintf(buf, sizeof buf, "%06u", (unsigned)G.score);
    text_draw(buf, 8, 28, C_WHITE);
    tiny_draw("SAVED", 8, 50, C_SLATE);
    snprintf(buf, sizeof buf, "%03d", G.saved);
    text_draw(buf, 8, 58, C_PINK);
    for (int i = 0; i < 6; i++) {
        int x = 9 + (i % 3) * 22, y = 88 + (i / 3) * 24;
        uint8_t m[PAL_COUNT];
        pal_identity(m);
        if (i >= G.counter) { m[C_RED] = C_DUSK; m[C_WINE] = C_NIGHT; m[C_WHITE] = C_SLATE; m[C_LIGHT] = C_DUSK; m[C_GREY] = C_DUSK; }
        spr_draw_ex(&pp_spr[P_JAR], x + 3, y, 0, m, -1);
    }
    /* right: Madame Nettle, who cackles when she plants */
    ui_panel(238, 12, 80, 156, C_NIGHT, C_DUSK);
    spr_draw_scaled(&pp_spr[G.cackle > 0 && (frame_t / 8) % 2 ? P_WITCH_CACKLE : P_WITCH], 254, 60, 2, SPR_FLIPX);
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
    draw_line_and_posy();
    draw_parts();
    gfx_camera(0, 0);
    draw_hud();
    /* the nectar countdown sits at the top middle (nothing there without it) */
    gfx_rect(88, 0, 144, 11, C_INK);
    if (G.power) {
        char buf[8];
        snprintf(buf, sizeof buf, "%d", (G.power + COUNT_FRAMES - 1) / COUNT_FRAMES);
        text_center(buf, 160, 2, G.power < COUNT_FRAMES * 8 && (frame_t / 4) % 2 ? C_RED : G.mush_power ? C_SKY : C_YELLOW);
    }
}

static void draw_board(int y, int hilite) {
    char buf[32];
    for (int i = 0; i < HISCORES; i++) {
        snprintf(buf, sizeof buf, "%d  %06u", i + 1, (unsigned)sv.hi[i]);
        bool me = i == hilite && (frame_t / 8) % 2;
        tiny_center(buf, 160, y + i * 7, me ? C_WHITE : i == 0 ? C_YELLOW : C_LIGHT);
    }
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
        spr_draw(&pp_spr[i == 0 ? P_POSY_SIDE1 + (frame_t / 8) % 2 : P_PUP_TRAIL], x, 144 - ((frame_t / 6 + i) % 2), 0);
    }
    spr_draw_scaled(&pp_spr[P_POSY_BIG], 36, 60, 2, 0);
    spr_draw(&pp_spr[(frame_t / 30) % 3 == 0 ? P_WITCH_CACKLE : P_WITCH], 262, 88, 0);
    static const uint8_t grad[] = {C_PINK, C_MAGENTA, C_VIOLET, C_PURPLE};
    ui_fancy_center("PETAL PARADE", 160, 18, 3, grad, 4, C_INK, C_PURPLE);
    tiny_center("TOP SCORES", 160, 64, C_SLATE);
    draw_board(72, -1);
}

static void draw_over(void) {
    /* the garden as it was, dimmed, under a little card */
    draw_play();
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    ui_panel(70, 22, 180, 136, C_INK, C_PINK);
    static const uint8_t grad[] = {C_PINK, C_MAGENTA, C_VIOLET};
    ui_fancy_center("OOPS!", 160, 30, 3, grad, 3, C_INK, C_PURPLE);
    spr_draw_scaled(&pp_spr[P_POSY_FALL], 136, 56, 2, 0);
    spr_draw(&pp_spr[(state_t / 12) % 2 ? P_PUP1 : P_PUP2], 166, 68, 0);
    char buf[64];
    snprintf(buf, sizeof buf, "SAVED %d   SCORE %06u", G.saved, (unsigned)G.score);
    tiny_center(buf, 160, 86, C_LIGHT);
    draw_board(98, new_rank);
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 140, C_WHITE);
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
    spr_draw_scaled(&pp_spr[P_POSY_BIG], 90, 72, 2, 0);
    spr_draw_scaled(&pp_spr[(t / 20) % 2 ? P_WITCH : P_WITCH_CACKLE], 190, 72, 2, SPR_FLIPX);
    ui_panel(30, 6, 260, 58, C_INK, C_PINK);
    static const uint8_t grad[] = {C_PINK, C_MAGENTA, C_VIOLET};
    ui_fancy_center("200 PUPS HOME!", 160, 11, 2, grad, 3, C_INK, C_PURPLE);
    if (t > 60) text_center("\"HMPH. KEEP YOUR PUPS,\" SNIFFS NETTLE.", 160, 32, C_LIGHT);
    if (t > 120) text_center("\"STAY FOR TEA, NEIGHBOUR,\" SAYS POSY.", 160, 43, C_LIME);
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
    memset(&G, 0, sizeof G);
    state = S_TITLE;
    state_t = 0;
    new_rank = -1;
    sheet_mode = false;
    memset(parts, 0, sizeof parts);
    game_set_pausable(false);
    music_play(PP_MUS_TITLE);
}

static void pp_quit(void) {
    /* a run left in the middle still goes on the board; nothing else is kept */
    if (state == S_PLAY || state == S_DEAD) record_score();
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
        spr_draw(&pp_spr[i == 0 ? P_POSY_SIDE1 + (t / 8) % 2 : P_PUP_TRAIL], px, y + 36 - ((t / 6 + i) % 2), 0);
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
    if (!strcmp(key, "step")) { *out = G.step_t; return 1; }
    if (!strcmp(key, "air")) { *out = G.air; return 1; }
    if (!strcmp(key, "power")) { *out = G.power; return 1; }
    if (!strcmp(key, "mult")) { *out = G.mult; return 1; }
    if (!strcmp(key, "chain")) { *out = G.chain; return 1; }
    if (!strcmp(key, "freeze")) { *out = G.freeze; return 1; }
    if (!strcmp(key, "counter")) { *out = G.counter; return 1; }
    if (!strcmp(key, "mush_power")) { *out = G.mush_power; return 1; }
    if (!strcmp(key, "pups")) { *out = count_cells(C_PUP); return 1; }
    if (!strcmp(key, "brambles")) { *out = count_cells(C_BRAMBLE); return 1; }
    if (!strcmp(key, "toadstools")) { *out = count_cells(C_TOADSTOOL); return 1; }
    if (!strcmp(key, "jars")) { *out = count_cells(C_JAR); return 1; }
    if (!strcmp(key, "bonus")) { *out = G.pups_bonus; return 1; }
    if (!strcmp(key, "pads")) { *out = count_pads(); return 1; }
    if (!strcmp(key, "star_t")) { *out = G.star_t; return 1; }
    if (!strcmp(key, "star_kind")) { *out = G.star_kind; return 1; }
    if (!strcmp(key, "star_used")) { *out = G.star_used; return 1; }
    if (!strcmp(key, "new_rank")) { *out = new_rank; return 1; }
    if (!strcmp(key, "powered_pct")) { *out = G.frames ? (int)(100 * G.powered_frames / G.frames) : 0; return 1; }
    if (!strcmp(key, "bot")) { *out = bot_buttons(); return 1; }
    if (!strncmp(key, "hi", 2) && key[2] >= '0' && key[2] < '0' + HISCORES && !key[3]) { *out = (int)sv.hi[key[2] - '0']; return 1; }
    if (!strcmp(key, "jar_level")) {
        *out = 0;
        for (int y = 0; y < PP_N; y++) for (int x = 0; x < PP_N; x++) if (G.cell[y][x] == C_JAR) *out = G.jar_level[y][x];
        return 1;
    }
    if (!strncmp(key, "daze_", 5)) { int x = atoi(key + 5), y = atoi(strchr(key + 5, '_') + 1); *out = G.daze[y][x]; return 1; }
    if (!strncmp(key, "cell_", 5)) { int x = atoi(key + 5), y = atoi(strchr(key + 5, '_') + 1); *out = G.cell[y][x]; return 1; }
    if (!strncmp(key, "sprout_", 7)) { int x = atoi(key + 7), y = atoi(strchr(key + 7, '_') + 1); *out = G.sprout[y][x]; return 1; }
    if (!strncmp(key, "look_", 5)) { int x = atoi(key + 5), y = atoi(strchr(key + 5, '_') + 1); *out = G.look[y][x]; return 1; }
    if (!strncmp(key, "pad_", 4)) { int x = atoi(key + 4), y = atoi(strchr(key + 4, '_') + 1); *out = G.pad[y][x]; return 1; }
    return 0;
}

static int pp_cheat(const char *cmd) {
    int a, b, c;
    char name[16];
    if (!strcmp(cmd, "new")) { new_game(); return 1; }
    if (!strcmp(cmd, "clear")) {
        /* an empty field with no sun circle, no random spawns and brambles
         * holding still; Posy at (5,9) heading up */
        memset(G.cell, 0, sizeof G.cell);
        memset(G.pad, 0, sizeof G.pad);
        memset(G.daze, 0, sizeof G.daze);
        memset(G.sprout, 0, sizeof G.sprout);
        G.n_trail = 0;
        G.hx = 5; G.hy = 9; G.px = 5; G.py = 10;
        G.vac_x = G.px; G.vac_y = G.py;
        G.dir = G.next_dir = D_UP;
        G.step_t = 1;
        G.star_t = 0;
        G.spawns_off = OFF_SPAWNS | OFF_HOPS;
        return 1;
    }
    if (sscanf(cmd, "put %15s %d %d", name, &a, &b) == 3) {
        int kind = !strcmp(name, "pup") ? C_PUP : !strcmp(name, "bramble") ? C_BRAMBLE : !strcmp(name, "toadstool") ? C_TOADSTOOL
                 : !strcmp(name, "jar") ? C_JAR : !strcmp(name, "pad") ? 99 : -1;
        if (kind < 0 || !inb(a, b)) return 0;
        if (kind == 99) G.pad[b][a] = 1;
        else if (kind == C_BRAMBLE) new_bramble(a, b);
        else { G.cell[b][a] = (uint8_t)kind; if (kind == C_JAR) { G.jar_level[b][a] = 1; G.jar_age[b][a] = 0; } }
        return 1;
    }
    if (sscanf(cmd, "look %d %d %d", &a, &b, &c) == 3) { G.look[b][a] = (uint8_t)(c & 3); G.hop[b][a] = BRAMBLE_HOP; return 1; }
    if (sscanf(cmd, "jarlevel %d %d %d", &a, &b, &c) == 3) { G.jar_level[b][a] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "turn %d", &a) == 1) { G.next_dir = (int8_t)(a & 3); return 1; }
    if (sscanf(cmd, "saved %d", &a) == 1) { G.saved = (uint16_t)a; return 1; }
    if (sscanf(cmd, "score %d", &a) == 1) { G.score = (uint32_t)a; return 1; }
    if (sscanf(cmd, "counter %d", &a) == 1) { G.counter = (uint8_t)a; return 1; }
    if (sscanf(cmd, "star %d", &a) == 1) { place_star(a); return 1; }
    if (sscanf(cmd, "startime %d", &a) == 1) { G.star_t = (uint16_t)a; G.star_used = 0; return 1; }
    if (!strcmp(cmd, "spawns_on")) { G.spawns_off &= (uint8_t)~(OFF_SPAWNS | OFF_HOPS); return 1; }
    if (!strcmp(cmd, "hops_on")) { G.spawns_off &= (uint8_t)~OFF_HOPS; return 1; }
    if (!strcmp(cmd, "still")) { G.spawns_off ^= OFF_POSY; return 1; }
    if (sscanf(cmd, "posy %d %d %d", &a, &b, &c) == 3) {
        G.hx = (int8_t)a; G.hy = (int8_t)b;
        G.dir = G.next_dir = (int8_t)(c & 3);
        G.px = (int8_t)(a - DX[c & 3]); G.py = (int8_t)(b - DY[c & 3]);
        G.vac_x = G.px; G.vac_y = G.py;
        G.step_t = 1;
        return 1;
    }
    if (!strcmp(cmd, "old_save")) {
        /* a save in the first format (with a suspended game) for the upgrade test */
        static uint8_t old[1400];
        uint32_t magic = SAVE_MAGIC_V1, best = 4321;
        memset(old, 0, sizeof old);
        memcpy(old, &magic, 4);
        memcpy(old + 4, &best, 4);
        old[8] = 1;
        game_save_write(game_current_index(), old, (int)sizeof old);
        load_save();
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
    {"BRING 10 PUPS HOME IN ONE GO", "BRING 200 PUPS HOME", "200 HOME WITH 20,000 POINTS"},
    "D-PAD\tTURN\n"
    GLYPH_A "\tHOP OVER A TILE\n"
    GLYPH_B "\tLET THE LINE GO\n"
    "START\tPAUSE",
    C_PINK, C_LEAF,
    pp_load, pp_start, pp_update, pp_draw, pp_quit, pp_label, pp_query, pp_cheat,
    "MAGIC GARDEN", 5,
};
