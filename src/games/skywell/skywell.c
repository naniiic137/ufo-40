/* SKYWELL - climb out of a crumbling shaft ahead of the Grinder.
 * Cartridge 07 of UFO 40, a tribute to Velgress (UFO 50 #7).
 * See docs/games/07-skywell.md. */
#include "skywell.h"

#define SHAFT_W (SW_COLS * SW_TILE) /* 208 */
#define LEVEL_TOP (-SW_FLOORS * SW_FLOOR_H)
#define KW 8
#define KH 12
#define GRAV 0.2f
#define JUMP_V 4.2f
#define AIR_V 3.6f
#define RUN 1.4f
#define SHOT_CD 10
#define SHOT_V 5.0f
#define SHOT_LIFE 22
#define GRINDER_Y 166 /* screen y of the Grinder's teeth */
#define MID_Y 84      /* the camera follows once Kip climbs above this */
#define OWL_FLOOR 12
#define OWL_TIME 900
#define OWL_HP 14
#define BOSS_FLOOR 14 /* floor 104 */
#define EYE_HP 60
#define HAND_HP 14
#define STUN_BASE 24  /* frames without control after a knock */
#define STUN_STEP 5   /* each Recovery takes this much off */
#define STUN_MIN 9
#define SAFE_T 40     /* blinking frames after a knock */
#define BAT_V 0.75f   /* slower than Kip runs or climbs */
#define STAR_T 52     /* frames of star boost while A is held */
#define BOMB_FUSE 60  /* a TNT block goes off this long after its plunger is pushed */
#define COIN_SHOT_T 50 /* a coin block lasts this long after it is first shot */

enum { S_TITLE, S_PLAY, S_DEAD, S_SHOP, S_OVER, S_END, S_HELP };
enum { PT_BASE, PT_CLOUD, PT_CRATE, PT_ROCK, PT_STAR, PT_COIN, PT_MINE, PT_BOMB, PT_ZAP, PT_BUBBLE, PT_METAL };
enum { FE_BAT, FE_MINNOW, FE_JELLY, FE_JELLY_S, FE_SQUID };
enum { IT_RECOVERY, IT_POWER, IT_DJUMP, IT_LIGHTFOOT, IT_MAGNET, IT_LIGHTNING, IT_COUNT };
static const int IT_PRICE[IT_COUNT] = {20, 15, 30, 10, 5, 20};
static const char *const IT_NAME[IT_COUNT] = {"RECOVERY", "POWER", "DOUBLE JUMP", "LIGHTFOOT", "MAGNET", "LIGHTNING"};
static const char *const IT_DESC[IT_COUNT] = {
    "SHAKE OFF A KNOCK SOONER", "SHOTS HIT HARDER", "ONE MORE JUMP IN THE AIR",
    "PLATFORMS CRUMBLE SLOWER", "SHOTS PICK UP COGS", "SHOTS PASS CLOUDS BY"};

/* t: crumble (or fuse) countdown; hp: a zapper's field (0 across, 1 up and
 * down); shot: a TNT block's plunger is down; flash: metal lights up */
typedef struct { float x, y; int w; uint8_t type, alive, stood, shot; int t, hp; float vx; int flash; } Plat;
/* state: a bat is awake; a squid is 0 warning, 1 falling, 2 on the Grinder */
typedef struct { float x, y, vx, vy; uint8_t kind, alive; int hp, t, state; } Foe;
typedef struct { float x, y, vx, vy; int life; uint8_t mine, alive; } Shot;
typedef struct { float x, y, vy; uint8_t alive, loose; } Coin;
typedef struct { float x, y, vx, vy; int life, col, kind; } Part;

#define MAX_PLATS 160
#define MAX_FOES 48
#define MAX_SHOTS 24
#define MAX_ESHOTS 48
#define MAX_COINS 96

static Plat plats[MAX_PLATS];
static Foe foes[MAX_FOES];
static Shot shots[MAX_SHOTS], eshots[MAX_ESHOTS];
static Coin coins[MAX_COINS];
static Part parts[160];

static struct {
    float x, y, vx, vy;
    int dir, air_used, stun, inv, star, shoot_cd, anim;
    bool ground, hop_held;
} K;

static struct {
    int level;           /* 1..4 */
    int cogs, cogs_total, keys;
    int items[IT_COUNT];
    bool owl_gone;       /* an owl that got away never comes back */
    uint64_t seed;
} run;

static struct {
    bool on, done, fled, key_out;
    float x, y, kx, ky;
    int hp, t, dir;
} owl;

static struct {
    bool on, dead;
    int eye_hp, hand_hp[2], t, flash, death_t;
    int warn[2], bolt[2];
} boss;

static int offer[3];
static int state, state_t, frame_t, shake, shop_sel, squid_t, top_floor, help_page;
#define HELP_PAGES 3
static float cam_y;
static bool sheet_mode, god;
static int64_t forced_seed = -1; /* tests and README media pick the pit */
static Rng rng;

typedef struct {
    uint32_t magic;
    uint16_t best_floor, most_cogs;
    uint8_t most_keys, escaped, beaten, pad;
} Save;
#define SAVE_MAGIC 0x53570001u
static Save sv;

/* ------------------------------------------------------------------ */
/* helpers                                                              */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else { memset(&sv, 0, sizeof sv); sv.magic = SAVE_MAGIC; }
}

static bool overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static int floor_of(float y) { return iclamp((int)floorf(-y / SW_FLOOR_H) + 1, 1, SW_FLOORS); }
static int global_floor(void) { return (run.level - 1) * SW_FLOORS + floor_of(K.y + KH - 1); }

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) { parts[i] = (Part){x, y, vx, vy, life, col, kind}; return; }
}

static void burst(float x, float y, int col, int n, float sp) {
    for (int i = 0; i < n; i++) {
        float a = (float)i / (float)n * 6.283f + (float)(frame_t % 5);
        part_add(x, y, cosf(a) * sp, sinf(a) * sp - 0.4f, 16 + i % 8, col, 0);
    }
}

static int crumble_time(int base) { return base + base * run.items[IT_LIGHTFOOT] / 2; }

static int add_plat(int type, float x, float y, int w) {
    for (int i = 0; i < MAX_PLATS; i++)
        if (!plats[i].alive) {
            plats[i] = (Plat){x, y, w, (uint8_t)type, 1, 0, 0, 0, 0, 0, 0};
            if (type == PT_METAL) plats[i].vx = (i % 2) ? 0.6f : -0.6f;
            return i;
        }
    return -1;
}

static int add_foe(int kind, float x, float y) {
    for (int i = 0; i < MAX_FOES; i++)
        if (!foes[i].alive) {
            static const int HP[5] = {1, 99, 1, 1, 2};
            foes[i] = (Foe){x, y, 0, 0, (uint8_t)kind, 1, HP[kind], 0, 0};
            if (kind == FE_MINNOW) foes[i].vx = (i % 2) ? 0.6f : -0.6f;
            return i;
        }
    return -1;
}

static void add_coin(float x, float y, bool loose) {
    for (int i = 0; i < MAX_COINS; i++)
        if (!coins[i].alive) { coins[i] = (Coin){x, y, loose ? -2.2f : 0, 1, (uint8_t)loose}; return; }
}

static void foe_size(int k, float *w, float *h) {
    static const float W[5] = {10, 24, 12, 8, 12}, H[5] = {8, 8, 12, 8, 12};
    *w = W[k];
    *h = H[k];
}

/* ------------------------------------------------------------------ */
/* the generator                                                        */

/* weights per level for: cloud crate rock coin mine bomb zap bubble metal.
 * Level 1 is clouds, boxes and bricks; level 2 adds TNT, cloud mines and
 * zappers; level 3 swaps clouds for bubbles; level 4 has cloud mines and
 * bouncing metal. */
static int pick_type(int level, int f) {
    static const int W[4][9] = {
        {32, 32, 26, 7, 0, 0, 0, 0, 0},
        {22, 26, 20, 6, 8, 8, 10, 0, 0},
        {0, 24, 20, 6, 0, 0, 0, 34, 0},
        {22, 20, 22, 4, 12, 0, 0, 0, 20},
    };
    static const int T[9] = {PT_CLOUD, PT_CRATE, PT_ROCK, PT_COIN, PT_MINE, PT_BOMB, PT_ZAP, PT_BUBBLE, PT_METAL};
    int w[9], sum = 0;
    for (int i = 0; i < 9; i++) {
        w[i] = W[level - 1][i];
        /* higher floors: fewer bricks, more of the tricky kinds */
        if (T[i] == PT_ROCK) w[i] = imax(4, w[i] - f / 3);
        if ((T[i] == PT_MINE || T[i] == PT_BOMB) && w[i]) w[i] += f / 6;
        sum += w[i];
    }
    int r = rng_range(&rng, 0, sum - 1);
    for (int i = 0; i < 9; i++) {
        if (r < w[i]) return T[i];
        r -= w[i];
    }
    return PT_CRATE;
}

static void gen_level(void) {
    memset(plats, 0, sizeof plats);
    memset(foes, 0, sizeof foes);
    memset(shots, 0, sizeof shots);
    memset(eshots, 0, sizeof eshots);
    memset(coins, 0, sizeof coins);
    memset(&owl, 0, sizeof owl);
    memset(&boss, 0, sizeof boss);
    add_plat(PT_BASE, 0, 0, SW_COLS);
    float prev_cx[3] = {SHAFT_W / 2.0f, SHAFT_W / 2.0f, SHAFT_W / 2.0f};
    int n_prev = 1;
    int star_floor = rng_range(&rng, 6, 24);
    for (int f = 1; f <= SW_FLOORS; f++) {
        float y = (float)(-f * SW_FLOOR_H);
        if (f == SW_FLOORS) { add_plat(PT_BASE, 0, y, SW_COLS); break; }
        /* the reef has fewer platforms: its fish make up the difference */
        int n = rng_range(&rng, 1, run.level == 3 ? 2 : f < 10 ? 3 : 2);
        float cx_now[3];
        int made = 0;
        for (int k = 0; k < n; k++) {
            int type = pick_type(run.level, f);
            if (k == 0 && (type == PT_MINE || type == PT_BOMB || type == PT_ZAP)) type = PT_CRATE; /* never make the only way on a trap */
            int w = type == PT_ZAP ? 1 : type == PT_MINE ? 2 : type == PT_METAL ? 3 : rng_range(&rng, 2, 4);
            int col;
            if (k == 0) {
                /* the first one is always within a jump of something below */
                float target = prev_cx[rng_range(&rng, 0, n_prev - 1)] + (float)rng_range(&rng, -60, 60);
                col = iclamp((int)(target / SW_TILE) - w / 2, 0, SW_COLS - w);
            } else {
                col = rng_range(&rng, 0, SW_COLS - w);
            }
            if (type == PT_ZAP) col = iclamp(col, 2, SW_COLS - 3); /* room for its field either side */
            /* keep platforms on one floor from overlapping */
            bool clash = false;
            for (int j = 0; j < made; j++)
                if (fabsf(cx_now[j] - (col + w / 2.0f) * SW_TILE) < (w + 3) * SW_TILE / 2.0f) clash = true;
            if (clash && k > 0) continue;
            if (f == star_floor && k == n - 1) { type = PT_STAR; w = 1; }
            float jy = y + (float)rng_range(&rng, -4, 4);
            int pi = add_plat(type, (float)(col * SW_TILE), jy, w);
            if (pi >= 0 && type == PT_ZAP) plats[pi].hp = rng_range(&rng, 0, 1);
            cx_now[made++] = (col + w / 2.0f) * SW_TILE;
            if (type != PT_MINE && type != PT_ZAP && rng_range(&rng, 0, 2) == 0)
                for (int c = 0; c < w; c++) add_coin((float)(col * SW_TILE + c * SW_TILE + 5), jy - 14, false);
        }
        for (int j = 0; j < made; j++) prev_cx[j] = cx_now[j];
        n_prev = imax(1, made);
        /* creatures: in the Roots, bats sleep hanging under a platform */
        if (run.level == 1 && f > 2 && made > 0 && rng_range(&rng, 0, 3) == 0) {
            float bx = cx_now[rng_range(&rng, 0, made - 1)] - 5 + (float)rng_range(&rng, -8, 8);
            add_foe(FE_BAT, fclamp(bx, 0, SHAFT_W - 10), y + 5);
        }
        if (run.level == 3 && f > 2) {
            int r = rng_range(&rng, 0, 4);
            if (r <= 1) add_foe(FE_MINNOW, (float)rng_range(&rng, 0, SHAFT_W - 24), y - 16);
            if (r == 2) add_foe(FE_JELLY, (float)rng_range(&rng, 10, SHAFT_W - 22), y - 18);
        }
    }
    top_floor = 1;
    squid_t = 240;
}

/* ------------------------------------------------------------------ */
/* the run                                                              */

static void start_level(void) {
    gen_level();
    K.x = SHAFT_W / 2.0f - KW / 2.0f;
    K.y = -KH;
    K.vx = K.vy = 0;
    K.ground = true;
    K.stun = K.inv = K.star = 0;
    K.air_used = 0;
    K.dir = 1;
    cam_y = -150;
    state = S_PLAY;
    state_t = 0;
    game_set_pausable(true);
    music_play(run.level == 4 ? SW_MUS_BOSS : SW_MUS_LEVEL[run.level - 1]);
}

static void new_run(void) {
    memset(&run, 0, sizeof run);
    run.level = 1;
    run.seed = forced_seed >= 0 ? (uint64_t)forced_seed : rng_next(&g_rng) ^ ((uint64_t)rng_next(&g_rng) << 32) ^ (uint64_t)frame_t;
    forced_seed = -1;
    rng_seed(&rng, run.seed);
    memset(parts, 0, sizeof parts);
    start_level();
}

static void record(void) {
    int gf = (run.level - 1) * SW_FLOORS + top_floor;
    if (gf > sv.best_floor) sv.best_floor = (uint16_t)gf;
    if (run.cogs_total > sv.most_cogs) sv.most_cogs = (uint16_t)run.cogs_total;
    if (run.keys > sv.most_keys) sv.most_keys = (uint8_t)run.keys;
    save_now();
}

static void die(void) {
    if (god || state != S_PLAY) return;
    state = S_DEAD;
    state_t = 0;
    shake = 16;
    music_stop();
    sfx_play_name("sw_die");
    burst(K.x + KW / 2, K.y + KH / 2, C_PINK, 16, 1.6f);
    record();
    game_set_pausable(false);
}

static void open_shop(void) {
    /* three different items; Magnet and Lightning only once */
    int pool[IT_COUNT], n = 0;
    for (int i = 0; i < IT_COUNT; i++)
        if (!((i == IT_MAGNET || i == IT_LIGHTNING) && run.items[i])) pool[n++] = i;
    for (int k = 0; k < 3; k++) {
        if (n == 0) { offer[k] = -1; continue; }
        int j = rng_range(&rng, 0, n - 1);
        offer[k] = pool[j];
        pool[j] = pool[--n];
    }
    shop_sel = 0;
    state = S_SHOP;
    state_t = 0;
    record();
    music_play(SW_MUS_SHOP);
    game_set_pausable(false);
}

static void level_done(void) {
    sfx_play_name("sw_clear");
    if (run.level == 3) {
        sv.escaped = 1;
        game_award(GOAL_SAUCER);
        record();
        if (run.keys < 3) { state = S_END; state_t = 0; music_restart(SW_MUS_END); game_set_pausable(false); return; }
    }
    if (run.level == 4) {
        sv.beaten = 1;
        game_award(GOAL_ALIEN);
        record();
        state = S_END;
        state_t = 0;
        music_restart(SW_MUS_END);
        game_set_pausable(false);
        return;
    }
    open_shop();
}

static void gain_key(void) {
    run.keys++;
    sfx_play_name("sw_key");
    if (run.keys >= 2) game_award(GOAL_BEACON);
}

/* ------------------------------------------------------------------ */
/* Kip                                                                  */

/* Kip's controls come from the pad, or from the demo climber (a test and
 * README-media aid that only runs when a script turns it on). */
static bool autoplay;
static uint32_t bot_now, bot_prev;
static int bot_target = -1;
static bool kb(int mask) { return autoplay ? (bot_now & (uint32_t)mask) != 0 : btn(mask); }
static bool kbp(int mask) { return autoplay ? (bot_now & (uint32_t)mask) && !(bot_prev & (uint32_t)mask) : btnp(mask); }

/* The climber can write down the buttons it presses as script lines, so a
 * test can play the same climb back through the real pad. */
static bool bot_log;
static uint32_t log_mask;
static int log_run;

static void bot_log_flush(void) {
    if (log_run <= 0) return;
    if (!log_mask) {
        printf("wait %d\n", log_run);
    } else {
        static const char *const NAME[6] = {"UP", "DOWN", "LEFT", "RIGHT", "A", "B"};
        char buf[48] = "hold ";
        bool first = true;
        for (int b = 0; b < 6; b++)
            if (log_mask & (1u << b)) {
                if (!first) strcat(buf, "+");
                strcat(buf, NAME[b]);
                first = false;
            }
        printf("%s %d\n", buf, log_run);
    }
    log_run = 0;
}

static void bot_log_frame(void) {
    if (bot_now != log_mask) { bot_log_flush(); log_mask = bot_now; }
    log_run++;
}

/* The demo climber looks ahead: for each way it could move and jump, it
 * plays Kip's motion forward and picks the highest safe landing. */
static bool bot_support(float x, float feet, int *on) {
    for (int i = 0; i < MAX_PLATS; i++) {
        Plat *p = &plats[i];
        if (!p->alive || p->type == PT_MINE) continue;
        if (x + KW <= p->x || x >= p->x + p->w * SW_TILE) continue;
        float top = p->y - (p->type == PT_STAR ? 6 : 0);
        if (fabsf(feet - top) < 0.6f) { *on = i; return true; }
    }
    return false;
}

/* jump_now: jump on the first frame; air_at: frame of a jump in the air
 * (-1 for none). Returns a score (higher is better). */
static float bot_sim(int dir, bool jump_now, int air_at, int *land) {
    float x = K.x, y = K.y, vy = K.vy;
    bool ground = K.ground;
    int jumps = (1 + run.items[IT_DJUMP]) - K.air_used;
    float death = cam_y + GRINDER_Y - KH - 2;
    int standing = -1;
    if (ground) bot_support(x, y + KH, &standing);
    if (jump_now) {
        if (ground) vy = -JUMP_V;
        else { vy = -AIR_V; jumps--; }
        ground = false;
    }
    for (int t = 1; t <= 90; t++) {
        if (t == air_at) {
            if (ground || jumps <= 0) return -1e9f;
            vy = -AIR_V;
            jumps--;
        }
        x = fclamp(x + dir * RUN, 0, SHAFT_W - KW);
        if (ground) {
            int on = -1;
            if (bot_support(x, y + KH, &on)) {
                if (air_at < 0 && t >= 20) {
                    /* walking along: score where she ends up */
                    Plat *p = &plats[on];
                    *land = on;
                    return -(p->y) - 20 - (p->t > 0 && p->t < 40 ? 200 : 0);
                }
                continue;
            }
            ground = false;
            vy = 0;
        }
        vy = fminf(vy + GRAV, 4.0f);
        float ob = y + KH;
        y += vy;
        float nb = y + KH;
        if (y > death) return -1e9f;
        if (vy < 0) continue;
        for (int i = 0; i < MAX_PLATS; i++) {
            Plat *p = &plats[i];
            if (!p->alive || p->type == PT_MINE) continue;
            if (x + KW <= p->x || x >= p->x + p->w * SW_TILE) continue;
            float top = p->y - (p->type == PT_STAR ? 6 : 0);
            if (!(ob <= top + 0.5f && nb >= top)) continue;
            if (p->t > 0 && p->t < t + 8) continue; /* gone before she lands */
            if (air_at > t) return -1e9f;           /* lands before the planned jump */
            *land = i;
            float score = -top;
            if (i == standing) score -= 25;
            if (p->type == PT_BOMB || p->type == PT_ZAP) score -= 60;
            if (p->type == PT_STAR) score += 200;
            if (p->t > 0) score -= 10;
            score -= (float)t * 0.2f + (air_at >= 0 ? 6 : 0);
            return score;
        }
    }
    return -1e8f;
}

static void bot_think(void) {
    bot_prev = bot_now;
    bot_now = 0;
    float kx = K.x + KW / 2;
    int jumps_air = (1 + run.items[IT_DJUMP]) - K.air_used;
    float best = -1e10f;
    int best_dir = 0, land = -1, best_air = -1;
    bool best_jump = false;
    static const int AIR_AT[] = {-1, 6, 10, 14, 18, 22, 26, 30};
    for (int j = 0; j < 2; j++) {
        if (j && !K.ground && jumps_air <= 0) continue;
        for (int a = 0; a < ARRAY_LEN(AIR_AT); a++) {
            for (int d = -1; d <= 1; d++) {
                int l = -1;
                float s = bot_sim(d, j == 1, AIR_AT[a], &l);
                if (s > best) { best = s; best_dir = d; best_jump = j == 1; best_air = AIR_AT[a]; land = l; }
            }
        }
    }
    (void)best_air;
    bot_target = land;
    if (best_dir > 0) bot_now |= BTN_RIGHT;
    if (best_dir < 0) bot_now |= BTN_LEFT;
    if (K.stun > 0) {
        /* nothing to do but get ready to jump as it wears off */
    } else if (best_jump) {
        if (K.ground || !(bot_prev & BTN_A)) bot_now |= BTN_A;
    } else if (!K.ground && K.vy < 0 && (bot_prev & BTN_A)) {
        bot_now |= BTN_A; /* keep holding for the full height */
    }
    /* shoot what's coming: awake bats and jellies close by */
    for (int i = 0; i < MAX_FOES; i++) {
        Foe *f = &foes[i];
        if (!f->alive) continue;
        if (!((f->kind == FE_BAT && f->state) || f->kind == FE_JELLY || f->kind == FE_JELLY_S)) continue;
        float w, h;
        foe_size(f->kind, &w, &h);
        float dx = f->x + w / 2 - kx, dy = f->y + h / 2 - (K.y + KH / 2);
        if (fabsf(dy) < 9 && fabsf(dx) < 80) {
            if ((dx > 0) == (K.dir > 0)) bot_now |= BTN_B;
            else bot_now = (bot_now & ~(uint32_t)(BTN_LEFT | BTN_RIGHT)) | (dx > 0 ? BTN_RIGHT : BTN_LEFT);
        } else if (fabsf(dx) < 10 && dy < 0 && dy > -70) {
            bot_now |= BTN_UP | BTN_B;
        } else if (fabsf(dx) < 10 && dy > 0 && dy < 70 && !K.ground) {
            bot_now |= BTN_DOWN | BTN_B;
        }
    }
    if (owl.on && !owl.done && fabsf(owl.x + 8 - kx) < 14) bot_now |= BTN_UP | BTN_B;
    if (boss.on && !boss.dead && fabsf(SHAFT_W / 2.0f - kx) < 16) bot_now |= BTN_UP | BTN_B;
    if (bot_log) bot_log_frame();
}

/* A knock flings Kip sideways, out of control for a moment. It costs her
 * the jump she was standing on, but not her jumps in the air: she can
 * jump again as soon as she comes round. */
static void stun_kip(float from_x) {
    if (K.inv > 0 || K.star > 0 || god) return;
    int st = imax(STUN_MIN, STUN_BASE - STUN_STEP * run.items[IT_RECOVERY]);
    K.stun = st;
    K.inv = st + SAFE_T;
    K.vx = (K.x + KW / 2 < from_x ? -1.5f : 1.5f);
    K.vy = -1.6f;
    K.ground = false;
    K.air_used = 0;
    shake = imax(shake, 5);
    sfx_play_name("sw_stun");
}

static void kill_foe(int i, int col);
static void add_eshot(float x, float y, float vx, float vy, bool mine);

/* landing on any creature is safe: Kip bounces off it with her jumps back */
static void bounce_kip(void) {
    K.vy = -JUMP_V;
    K.air_used = 0;
    K.ground = false;
    sfx_play_name("sw_boing");
}

static void fire(void) {
    int aim = kb(BTN_UP) ? 1 : (kb(BTN_DOWN) && !K.ground) ? 2 : 0;
    for (int i = 0; i < MAX_SHOTS; i++)
        if (!shots[i].alive) {
            Shot *s = &shots[i];
            s->alive = 1;
            s->life = SHOT_LIFE;
            s->x = K.x + KW / 2;
            s->y = K.y + 5;
            s->vx = aim ? 0 : (float)K.dir * SHOT_V;
            s->vy = aim == 1 ? -SHOT_V : aim == 2 ? SHOT_V : 0;
            if (aim == 1) s->y = K.y;
            if (aim == 2) s->y = K.y + KH;
            break;
        }
    sfx_play_name("sw_shoot");
}

/* a star block launches Kip up, untouchable; holding A keeps her going */
static void star_touch(int i) {
    plats[i].alive = 0;
    K.star = STAR_T;
    K.inv = STAR_T + 20;
    K.stun = 0;
    K.air_used = 0;
    burst(plats[i].x + 8, plats[i].y, C_YELLOW, 14, 1.6f);
    sfx_play_name("sw_star");
}

/* a cloud mine puffs into a row of clouds, whether shot or bumped */
static void mine_puff(int i) {
    Plat *p = &plats[i];
    float cx = p->x + p->w * 8, y = p->y;
    p->alive = 0;
    int c0 = iclamp((int)(cx / SW_TILE) - 3, 0, SW_COLS - 1), c1 = iclamp((int)(cx / SW_TILE) + 3, 0, SW_COLS - 1);
    add_plat(PT_CLOUD, (float)(c0 * SW_TILE), y, c1 - c0 + 1);
    burst(cx, y, C_WHITE, 16, 1.4f);
    sfx_play_name("sw_puff");
}

static void land_effects(int i) {
    Plat *p = &plats[i];
    if (p->stood) return;
    p->stood = 1;
    switch (p->type) {
    case PT_CLOUD: case PT_BUBBLE: if (!p->t) p->t = crumble_time(12); break;
    case PT_CRATE: if (!p->t) p->t = crumble_time(18); break;
    case PT_ROCK: if (!p->t) p->t = crumble_time(60); break;
    case PT_COIN: if (!p->t) p->t = crumble_time(90); break;
    case PT_BOMB:
        /* her weight pushes the plunger down: it goes off a second later */
        if (!p->shot) {
            p->shot = 1;
            p->t = BOMB_FUSE;
            sfx_play_name("sw_tick");
        }
        break;
    case PT_METAL:
        /* metal throws her straight back up, jumps restored */
        p->flash = 8;
        bounce_kip();
        break;
    case PT_STAR: star_touch(i); break;
    default: break;
    }
}

static void kip_update(void) {
    if (autoplay) bot_think();
    int hx = (kb(BTN_RIGHT) ? 1 : 0) - (kb(BTN_LEFT) ? 1 : 0);
    K.anim++;
    if (K.inv > 0) K.inv--;
    if (K.shoot_cd > 0) K.shoot_cd--;
    if (K.star > 0) {
        /* the star carries her up about six floors, untouchable, as long
         * as A is held; letting go ends the climb early */
        K.star--;
        K.vy = -4.0f;
        K.vx = (float)hx * RUN;
        if (hx) K.dir = hx;
        if (!kb(BTN_A) && K.star < STAR_T - 6) K.star = 0;
        if (K.star == 0) { K.vy = -1.0f; K.air_used = 0; }
    } else if (K.stun > 0) {
        K.stun--;
        K.vx *= 0.97f;
        K.vy = fminf(K.vy + GRAV, 4.0f);
    } else {
        if (hx) K.dir = hx;
        K.vx = (float)hx * RUN;
        int air_jumps = 1 + run.items[IT_DJUMP];
        if (kbp(BTN_A) || (kb(BTN_A) && K.ground && K.hop_held)) {
            if (K.ground) { K.vy = -JUMP_V; K.ground = false; sfx_play_name("sw_jump"); }
            else if (kbp(BTN_A) && K.air_used < air_jumps) { K.vy = -AIR_V; K.air_used++; sfx_play_name("sw_jump2"); burst(K.x + KW / 2, K.y + KH, C_LIGHT, 5, 0.6f); }
        }
        K.hop_held = kb(BTN_A);
        if (!kb(BTN_A) && K.vy < -1.2f) K.vy += 0.25f;
        K.vy = fminf(K.vy + GRAV, 4.0f);
        if (kb(BTN_B) && K.shoot_cd == 0) { fire(); K.shoot_cd = SHOT_CD; }
    }
    /* horizontal: the shaft walls */
    K.x = fclamp(K.x + K.vx, 0, SHAFT_W - KW);
    /* vertical: land on platforms from above */
    float ob = K.y + KH;
    K.y += K.vy;
    float nb = K.y + KH;
    K.ground = false;
    if (K.vy >= 0) {
        for (int i = 0; i < MAX_PLATS; i++) {
            Plat *p = &plats[i];
            if (!p->alive || p->type == PT_MINE) continue;
            float px = p->x, pw = (float)(p->w * SW_TILE);
            if (K.x + KW <= px || K.x >= px + pw) continue;
            float top = p->y - (p->type == PT_STAR ? 6 : 0); /* a star stands proud of its row */
            if (ob <= top + 0.5f && nb >= top) {
                K.y = top - KH;
                K.vy = 0;
                K.ground = true;
                K.air_used = 0;
                if (p->type == PT_METAL) K.x = fclamp(K.x + p->vx, 0, SHAFT_W - KW);
                land_effects(i);
                break;
            }
        }
        /* every creature is safe to land on: Kip bounces off it, and a
         * bat or a jelly doesn't survive it. Fish are wide enough to ride. */
        if (!K.ground)
            for (int i = 0; i < MAX_FOES; i++) {
                Foe *f = &foes[i];
                if (!f->alive || (f->kind == FE_SQUID && f->state == 0)) continue;
                float w, h;
                foe_size(f->kind, &w, &h);
                if (K.x + KW <= f->x || K.x >= f->x + w) continue;
                if (!(ob <= f->y + 4.5f && nb >= f->y)) continue;
                if (f->kind == FE_MINNOW) {
                    K.y = f->y - KH; K.vy = 0; K.ground = true; K.air_used = 0;
                    K.x = fclamp(K.x + f->vx, 0, SHAFT_W - KW);
                    break;
                }
                K.y = f->y - KH;
                bounce_kip();
                if (f->kind == FE_BAT || f->kind == FE_JELLY || f->kind == FE_JELLY_S) kill_foe(i, C_VIOLET);
                break;
            }
    } else {
        /* metal is hard from below */
        for (int i = 0; i < MAX_PLATS; i++) {
            Plat *p = &plats[i];
            if (!p->alive || p->type != PT_METAL) continue;
            if (overlap(K.x, K.y, KW, KH, p->x, p->y, p->w * SW_TILE, 8)) { K.y = p->y + 8; K.vy = 1; p->flash = 8; stun_kip(p->x + p->w * 8); }
        }
    }
    /* touch: cloud mines, zapper fields, metal edges */
    for (int i = 0; i < MAX_PLATS; i++) {
        Plat *p = &plats[i];
        if (!p->alive) continue;
        float pw = (float)(p->w * SW_TILE);
        if (p->type == PT_MINE && overlap(K.x, K.y, KW, KH, p->x, p->y - 4, pw, 12)) {
            /* bumping a mine still makes its clouds, but it knocks her about */
            float mx = p->x + pw / 2;
            mine_puff(i);
            stun_kip(mx);
            continue;
        }
        if (p->type == PT_ZAP) {
            float fx[2], fy[2], fw, fh;
            if (p->hp == 0) { fx[0] = p->x - 32; fx[1] = p->x + 16; fy[0] = fy[1] = p->y; fw = 32; fh = 8; }
            else { fx[0] = fx[1] = p->x; fy[0] = p->y - 32; fy[1] = p->y + 8; fw = 16; fh = 32; }
            for (int s = 0; s < 2; s++)
                if (overlap(K.x, K.y, KW, KH, fx[s] + 2, fy[s] + 2, fw - 4, fh - 4)) stun_kip(p->x + 8);
        }
        if (p->type == PT_METAL && !K.ground && K.stun == 0 && overlap(K.x, K.y + 2, KW, KH - 4, p->x, p->y + 2, pw, 6)) stun_kip(p->x + pw / 2);
    }
    /* coins and the key */
    for (int i = 0; i < MAX_COINS; i++) {
        Coin *c = &coins[i];
        if (c->alive && overlap(K.x - 1, K.y - 1, KW + 2, KH + 2, c->x, c->y, 6, 6)) {
            c->alive = 0;
            run.cogs++;
            run.cogs_total++;
            sfx_play_name("sw_cog");
        }
    }
    if (owl.key_out && overlap(K.x, K.y, KW, KH, owl.kx, owl.ky, 8, 10)) { owl.key_out = false; gain_key(); burst(owl.kx + 4, owl.ky + 4, C_YELLOW, 12, 1.2f); }
    /* the Grinder */
    if (K.y + KH > cam_y + GRINDER_Y + 2) die();
    int fl = floor_of(K.y + KH - 1);
    if (K.ground && fl > top_floor) top_floor = fl;
    /* the top ledge ends the level (once the Well Eye is beaten) */
    if (K.ground && K.y + KH <= LEVEL_TOP + 1 && !(boss.on && !boss.dead) && !(run.level == 4 && !boss.on)) level_done();
}

/* ------------------------------------------------------------------ */
/* the world                                                            */

static void break_plat(int i) {
    Plat *p = &plats[i];
    p->alive = 0;
    int col = p->type == PT_CLOUD || p->type == PT_BUBBLE ? C_WHITE : p->type == PT_ROCK ? C_GREY : C_TAN;
    for (int c = 0; c < p->w; c++) burst(p->x + c * SW_TILE + 8, p->y + 4, col, 4, 0.9f);
    sfx_play_name("sw_crumble");
}

static void plats_update(void) {
    for (int i = 0; i < MAX_PLATS; i++) {
        Plat *p = &plats[i];
        if (!p->alive) continue;
        if (p->type == PT_METAL) {
            p->x += p->vx;
            if (p->x < 0 || p->x + p->w * SW_TILE > SHAFT_W) { p->vx = -p->vx; p->x = fclamp(p->x, 0, (float)(SHAFT_W - p->w * SW_TILE)); }
        }
        /* standing on it keeps it marked, stepping off lets it land again */
        bool on = K.ground && K.x + KW > p->x && K.x < p->x + p->w * SW_TILE && fabsf(K.y + KH - p->y) < 0.6f;
        if (!on) p->stood = 0;
        if (p->flash > 0) p->flash--;
        if (p->type == PT_ZAP) {
            if (p->shot > 0) p->shot--;
        } else if (p->type == PT_BOMB) {
            /* a TNT block goes off when its fuse ends, or when the Grinder
             * reaches it: three shots, straight up and to both diagonals */
            bool grind = p->y + 6 > cam_y + GRINDER_Y;
            if ((p->shot && p->t > 0 && --p->t == 0) || grind) {
                float cx = p->x + p->w * 8, cy = p->y - 2;
                for (int k = -1; k <= 1; k++) add_eshot(cx, cy, k * 1.4f, -2.0f, true);
                p->alive = 0;
                burst(cx, p->y + 4, C_ORANGE, 14, 1.6f);
                shake = imax(shake, 4);
                sfx_play_name("sw_boom");
                continue;
            }
        } else if (p->t > 0 && --p->t == 0) {
            break_plat(i);
        }
        if (p->y > cam_y + 200) p->alive = 0;
    }
}

static void shoot_plat(Shot *s, int i) {
    Plat *p = &plats[i];
    switch (p->type) {
    case PT_CLOUD:
        if (!run.items[IT_LIGHTNING]) break_plat(i);
        return; /* shots go straight through clouds */
    case PT_BUBBLE:
        break_plat(i); /* Lightning doesn't spare bubbles */
        return;
    case PT_CRATE: if (!p->t || p->t > 12) p->t = 12; break;
    case PT_ROCK: if (!p->t || p->t > 60) p->t = imax(20, 60 - 15 * run.items[IT_POWER]); break;
    case PT_COIN:
        /* every hit knocks out a cog; the first one starts it crumbling */
        add_coin(p->x + p->w * 8 - 3, p->y - 8, true);
        sfx_play_name("sw_cog");
        if (!p->t || p->t > COIN_SHOT_T) p->t = COIN_SHOT_T;
        break;
    case PT_MINE: mine_puff(i); break;
    case PT_ZAP:
        /* a hit on the middle block swings its field round */
        if (!p->shot) { p->hp ^= 1; p->shot = 12; sfx_play_name("sw_zap"); }
        break;
    case PT_METAL: p->flash = 8; break; /* shots bounce off metal */
    case PT_BOMB: break;
    default: break;
    }
    s->alive = 0;
    part_add(s->x, s->y, 0, 0, 6, C_YELLOW, 1);
}

static void kill_foe(int i, int col) {
    Foe *f = &foes[i];
    float w, h;
    foe_size(f->kind, &w, &h);
    f->alive = 0;
    burst(f->x + w / 2, f->y + h / 2, f->kind == FE_BAT ? col : C_CYAN, 10, 1.2f);
    sfx_play_name("sw_hit");
    if (f->kind == FE_JELLY) {
        /* a jelly splits in two */
        int a = add_foe(FE_JELLY_S, f->x - 2, f->y + 2), b = add_foe(FE_JELLY_S, f->x + 6, f->y + 2);
        if (a >= 0) foes[a].vx = -0.9f;
        if (b >= 0) foes[b].vx = 0.9f;
    }
}

static void hurt_foe(int i, int dmg) {
    Foe *f = &foes[i];
    if (f->kind == FE_MINNOW || (f->kind == FE_SQUID && f->state == 0)) return;
    f->hp -= dmg;
    sfx_play_name("sw_hit");
    if (f->hp <= 0) kill_foe(i, C_VIOLET);
}

static void shots_update(void) {
    int dmg = 1 + run.items[IT_POWER];
    for (int i = 0; i < MAX_SHOTS; i++) {
        Shot *s = &shots[i];
        if (!s->alive) continue;
        s->x += s->vx;
        s->y += s->vy;
        if (--s->life <= 0 || s->x < -4 || s->x > SHAFT_W + 4) { s->alive = 0; continue; }
        for (int k = 0; k < MAX_PLATS && s->alive; k++) {
            Plat *p = &plats[k];
            if (!p->alive || p->type == PT_STAR) continue;
            if (overlap(s->x - 2, s->y - 2, 4, 4, p->x, p->y, p->w * SW_TILE, 8)) shoot_plat(s, k);
        }
        if (!s->alive) continue;
        for (int k = 0; k < MAX_FOES; k++) {
            Foe *f = &foes[k];
            if (!f->alive || f->kind == FE_MINNOW || (f->kind == FE_SQUID && f->state == 0)) continue;
            float w, h;
            foe_size(f->kind, &w, &h);
            if (overlap(s->x - 2, s->y - 2, 4, 4, f->x, f->y, w, h)) { hurt_foe(k, dmg); s->alive = 0; break; }
        }
        if (!s->alive) continue;
        if (run.items[IT_MAGNET])
            for (int k = 0; k < MAX_COINS; k++) {
                Coin *c = &coins[k];
                if (c->alive && overlap(s->x - 3, s->y - 3, 6, 6, c->x, c->y, 6, 6)) { c->alive = 0; run.cogs++; run.cogs_total++; sfx_play_name("sw_cog"); }
            }
        if (owl.on && !owl.done && overlap(s->x - 2, s->y - 2, 4, 4, owl.x, owl.y, 16, 12)) {
            s->alive = 0;
            owl.hp -= dmg;
            sfx_play_name("sw_hit");
            if (owl.hp <= 0) {
                /* beaten: it drops a key and flees */
                owl.done = true;
                owl.key_out = true;
                owl.kx = owl.x + 4;
                owl.ky = owl.y + 6;
                burst(owl.x + 8, owl.y + 6, C_LIGHT, 14, 1.4f);
                sfx_play_name("sw_owl");
            }
            continue;
        }
        if (boss.on && !boss.dead) {
            float by = cam_y + 26;
            for (int h = 0; h < 2; h++) {
                float hx = SHAFT_W / 2.0f + (h ? 52.0f : -68.0f);
                if (boss.hand_hp[h] > 0 && overlap(s->x - 2, s->y - 2, 4, 4, hx, by + 16, 16, 14)) {
                    s->alive = 0;
                    boss.hand_hp[h] -= dmg;
                    boss.flash = 6;
                    sfx_play_name("sw_hit");
                    if (boss.hand_hp[h] <= 0) { burst(hx + 8, by + 22, C_MAGENTA, 16, 1.5f); sfx_play_name("sw_boom"); }
                }
            }
            if (s->alive && overlap(s->x - 2, s->y - 2, 4, 4, SHAFT_W / 2.0f - 14, by, 28, 24)) {
                s->alive = 0;
                boss.eye_hp -= dmg;
                boss.flash = 6;
                sfx_play_name("sw_hit");
                if (boss.eye_hp <= 0) { boss.dead = true; boss.death_t = 0; music_stop(); sfx_play_name("sw_boom"); }
            }
        }
    }
}

static void add_eshot(float x, float y, float vx, float vy, bool mine) {
    for (int i = 0; i < MAX_ESHOTS; i++)
        if (!eshots[i].alive) { eshots[i] = (Shot){x, y, vx, vy, 180, (uint8_t)mine, 1}; return; }
}

static void foes_update(void) {
    float kcx = K.x + KW / 2, kcy = K.y + KH / 2;
    float floor_y = cam_y + GRINDER_Y; /* where the Grinder's teeth are */
    for (int i = 0; i < MAX_FOES; i++) {
        Foe *f = &foes[i];
        if (!f->alive) continue;
        float w, h;
        foe_size(f->kind, &w, &h);
        f->t++;
        if (f->y > cam_y + 200) { f->alive = 0; continue; }
        switch (f->kind) {
        case FE_BAT:
            if (!f->state) {
                /* asleep until Kip comes close, or passes underneath */
                float dx = fabsf(f->x + w / 2 - kcx), dy = kcy - (f->y + h / 2);
                if ((dx < 32 && dy > 0 && dy < 110) || (dx < 40 && fabsf(dy) < 40)) { f->state = 1; f->t = 0; sfx_play_name("sw_bat"); }
            } else {
                /* awake: it flutters after her, slower than she can climb */
                float dx = kcx - (f->x + w / 2), dy = kcy - (f->y + h / 2), d = sqrtf(dx * dx + dy * dy) + 0.01f;
                f->vx = dx / d * BAT_V;
                f->vy = dy / d * BAT_V + sinf(f->t * 0.25f) * 0.5f;
                f->x = fclamp(f->x + f->vx, 0, SHAFT_W - w);
                f->y += f->vy;
            }
            break;
        case FE_MINNOW:
            f->x += f->vx;
            if (f->x < 0 || f->x > SHAFT_W - 24) f->vx = -f->vx;
            break;
        case FE_JELLY:
            if (f->y > cam_y - 20) {
                /* jellies swim at her in quick pulses */
                float pulse = 0.35f + 0.9f * fmaxf(0, sinf(f->t * 0.12f));
                float dx = kcx - (f->x + 6), dy = kcy - (f->y + 6), d = sqrtf(dx * dx + dy * dy) + 0.01f;
                f->x = fclamp(f->x + dx / d * pulse, 0, SHAFT_W - w);
                f->y += dy / d * pulse;
            }
            break;
        case FE_JELLY_S:
            f->x += f->vx;
            f->y += sinf(f->t * 0.1f) * 0.4f;
            if (f->x < 0 || f->x > SHAFT_W - 8) f->vx = -f->vx;
            break;
        case FE_SQUID:
            if (f->state == 0) {
                /* its mark shows at the top of the screen, then it drops */
                f->y = cam_y - 14;
                if (f->t >= 60) { f->state = 1; f->t = 0; }
            } else if (f->state == 1) {
                f->y += 1.2f;
                if (f->y + h >= floor_y) { f->y = floor_y - h + 2; f->state = 2; f->t = 0; burst(f->x + 6, floor_y, C_PINK, 8, 1.0f); }
            } else {
                /* stuck on the Grinder for a moment: a last thing to bounce on */
                f->y = floor_y - h + 2;
                if (f->t >= 70) { f->alive = 0; burst(f->x + 6, floor_y, C_PINK, 10, 1.2f); }
            }
            break;
        }
        if (!f->alive) continue;
        /* anything the Grinder catches is gone (bats can be lured into it) */
        if (f->kind != FE_SQUID && f->y + h > floor_y + 2) { kill_foe(i, C_RED); continue; }
        if (f->kind == FE_MINNOW || (f->kind == FE_SQUID && f->state == 0)) continue;
        /* touching one from the side or below knocks her about */
        if (overlap(K.x, K.y, KW, KH, f->x + 1, f->y + 2, w - 2, h - 2)) stun_kip(f->x + w / 2);
    }
    /* squids come down one at a time in the reef */
    if (run.level == 3) {
        bool any = false;
        for (int i = 0; i < MAX_FOES; i++) any |= foes[i].alive && foes[i].kind == FE_SQUID;
        if (!any && --squid_t <= 0) {
            add_foe(FE_SQUID, (float)rng_range(&rng, 8, SHAFT_W - 20), cam_y - 14);
            squid_t = 200;
        }
    }
    for (int i = 0; i < MAX_ESHOTS; i++) {
        Shot *s = &eshots[i];
        if (!s->alive) continue;
        s->x += s->vx;
        s->y += s->vy;
        if (--s->life <= 0 || s->y > cam_y + 190 || s->x < -8 || s->x > SHAFT_W + 8) { s->alive = 0; continue; }
        if (overlap(K.x, K.y, KW, KH, s->x - 2, s->y - 2, 4, 4)) { s->alive = 0; stun_kip(s->x); }
    }
    for (int i = 0; i < MAX_COINS; i++) {
        Coin *c = &coins[i];
        if (!c->alive || !c->loose) continue;
        c->vy = fminf(c->vy + 0.12f, 2.0f);
        c->y += c->vy;
        if (c->y > cam_y + 190) c->alive = 0;
    }
}

static void owl_update(void) {
    if (run.level > 3) return;
    if (!owl.on && !run.owl_gone && top_floor >= OWL_FLOOR && !owl.done) {
        owl.on = true;
        owl.hp = OWL_HP;
        owl.t = 0;
        owl.x = SHAFT_W / 2.0f - 8;
        owl.y = cam_y - 20;
        owl.dir = 1;
        sfx_play_name("sw_owl");
    }
    if (owl.key_out) {
        owl.ky += 0.7f;
        if (owl.ky > cam_y + GRINDER_Y) owl.key_out = false; /* lost to the Grinder */
    }
    if (!owl.on) return;
    owl.t++;
    if (owl.done || owl.fled) {
        /* away up the shaft */
        owl.y -= 2.0f;
        owl.x += owl.dir * 1.0f;
        if (owl.y < cam_y - 40) owl.on = false;
        return;
    }
    /* it keeps near the top of the screen, ascending with Kip */
    owl.x += owl.dir * 1.1f;
    if (owl.x < 0 || owl.x > SHAFT_W - 16) owl.dir = -owl.dir;
    float want = cam_y + 22 + sinf(owl.t * 0.05f) * 8;
    owl.y += (want - owl.y) * 0.1f;
    if (owl.t >= OWL_TIME) {
        owl.fled = true;
        run.owl_gone = true;
        sfx_play_name("sw_owl");
    }
}

static void boss_update(void) {
    if (run.level != 4) return;
    if (!boss.on && top_floor >= BOSS_FLOOR) {
        boss.on = true;
        boss.eye_hp = EYE_HP;
        boss.hand_hp[0] = boss.hand_hp[1] = HAND_HP;
        boss.t = 0;
        sfx_play_name("sw_owl");
    }
    if (!boss.on) return;
    if (boss.flash > 0) boss.flash--;
    if (boss.dead) {
        boss.death_t++;
        if (boss.death_t % 6 == 0) burst(SHAFT_W / 2.0f + (float)(frame_t * 13 % 60 - 30), cam_y + 30 + (float)(frame_t * 7 % 30), C_MAGENTA, 8, 1.4f);
        return;
    }
    boss.t++;
    float by = cam_y + 26;
    if (boss.t % 60 == 0) {
        float ex = SHAFT_W / 2.0f, ey = by + 12;
        float dx = K.x + KW / 2 - ex, dy = K.y + KH / 2 - ey, d = sqrtf(dx * dx + dy * dy) + 0.01f;
        add_eshot(ex, ey, dx / d * 1.5f, dy / d * 1.5f, false);
        sfx_play_name("sw_orb");
    }
    for (int h = 0; h < 2; h++) {
        if (boss.hand_hp[h] <= 0) { boss.warn[h] = boss.bolt[h] = 0; continue; }
        int phase = (boss.t + h * 55) % 110;
        boss.warn[h] = phase >= 60 && phase < 90;
        boss.bolt[h] = phase >= 90;
        if (phase == 90) sfx_play_name("sw_zap");
        float hx = SHAFT_W / 2.0f + (h ? 52.0f : -68.0f);
        if (boss.bolt[h] && overlap(K.x, K.y, KW, KH, hx + 4, by + 30, 8, 200)) stun_kip(hx + 8);
    }
}

static void play_update(void) {
    frame_t++;
    kip_update();
    if (state != S_PLAY) {
        if (bot_log) { bot_log_flush(); bot_log = false; autoplay = false; }
        return;
    }
    plats_update();
    shots_update();
    foes_update();
    owl_update();
    boss_update();
    /* the camera climbs with Kip, never back down; the Grinder is its floor */
    float top = (float)LEVEL_TOP - 40;
    if (K.y - cam_y < MID_Y) cam_y = fmaxf(top, K.y - MID_Y);
    if (shake > 0) shake--;
}

/* ------------------------------------------------------------------ */
/* update                                                               */

static void parts_update(void) {
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        if (p->kind == 0) p->vy += 0.06f;
    }
}

static void sw_update(void) {
    state_t++;
    parts_update();
    switch (state) {
    case S_TITLE:
        frame_t++;
        game_set_pausable(false);
        if (btnp(BTN_B)) game_exit_to_library();
        if (btnp(BTN_SELECT)) { sfx_play_name("ui_ok"); state = S_HELP; state_t = 0; help_page = 0; break; }
        if (btnp(BTN_A) || btnp(BTN_START)) { sfx_play_name("ui_ok"); input_consume(); new_run(); }
        break;
    case S_HELP:
        frame_t++;
        if (btnp(BTN_A) || btnp(BTN_RIGHT)) {
            sfx_play_name("ui_move");
            if (++help_page >= HELP_PAGES) { state = S_TITLE; state_t = 0; }
        }
        if (btnp(BTN_LEFT) && help_page > 0) { help_page--; sfx_play_name("ui_move"); }
        if (btnp(BTN_B) || btnp(BTN_SELECT) || btnp(BTN_START)) { sfx_play_name("ui_back"); state = S_TITLE; state_t = 0; }
        break;
    case S_PLAY: play_update(); break;
    case S_DEAD:
        frame_t++;
        if (shake > 0) shake--;
        if (state_t > 50) { state = S_OVER; state_t = 0; music_restart(SW_MUS_OVER); }
        break;
    case S_SHOP:
        frame_t++;
        if (btn_repeat(BTN_LEFT) && shop_sel > 0) { shop_sel--; sfx_play_name("ui_move"); }
        if (btn_repeat(BTN_RIGHT) && shop_sel < 2) { shop_sel++; sfx_play_name("ui_move"); }
        if (btnp(BTN_A) && state_t > 20) {
            int it = offer[shop_sel];
            if (it >= 0 && run.cogs >= IT_PRICE[it]) {
                run.cogs -= IT_PRICE[it];
                run.items[it]++;
                offer[shop_sel] = -1;
                sfx_play_name("sw_buy");
            } else {
                sfx_play_name("ui_error");
            }
        }
        if ((btnp(BTN_B) || btnp(BTN_START)) && state_t > 20) {
            input_consume();
            run.level++;
            start_level();
        }
        break;
    case S_OVER:
        /* straight back into a new pit, or back to the title */
        if (state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); input_consume(); new_run(); }
        else if (state_t > 20 && btnp(BTN_B)) { state = S_TITLE; state_t = 0; music_play(SW_MUS_TITLE); }
        break;
    case S_END:
        frame_t++;
        if (state_t > 360 && (btnp(BTN_A) || btnp(BTN_START))) { state = S_TITLE; state_t = 0; music_play(SW_MUS_TITLE); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */

typedef struct { uint8_t bg, bg2, wall, wall_hi, rock, rock_hi; } Theme;
static const Theme THEMES[4] = {
    {C_NIGHT, C_BROWN, C_EARTH, C_TAN, C_BROWN, C_EARTH},    /* the Roots */
    {C_INK, C_DUSK, C_SLATE, C_GREY, C_DUSK, C_SLATE},       /* the Sparkworks */
    {C_NAVY, C_BLUE, C_TEAL, C_JADE, C_TEAL, C_JADE},        /* the Sky Reef */
    {C_INK, C_PURPLE, C_WINE, C_MAGENTA, C_MAROON, C_WINE},  /* the Eye */
};
static const Theme *TH(void) { return &THEMES[iclamp(run.level - 1, 0, 3)]; }

static int sy(float y) { return (int)lroundf(y - cam_y); }

static void draw_shaft(void) {
    const Theme *t = TH();
    gfx_rect(SW_X0, 0, SHAFT_W, SCREEN_H, t->bg);
    /* the back wall, drifting slower than the platforms */
    int off = ((int)(-cam_y / 2)) % 48;
    for (int y = -48 + off; y < SCREEN_H; y += 48)
        for (int x = 0; x < SHAFT_W; x += 32) {
            int k = ((x / 32) + (int)((cam_y / 2 - y) / 48)) & 3;
            switch (run.level) {
            case 1: gfx_vline(SW_X0 + x + 8 + k * 3, y, y + 30, t->bg2); gfx_pset(SW_X0 + x + 9 + k * 3, y + 31, t->bg2); break;
            case 2: gfx_rectb(SW_X0 + x + 4, y + 6, 22, 14, t->bg2); gfx_pset(SW_X0 + x + 15, y + 13, k ? t->bg2 : C_YELLOW); break;
            case 3: gfx_circb(SW_X0 + x + 12, y + 20 + k * 4, 3 + k, t->bg2); break;
            default: gfx_dither(SW_X0 + x + 10, y, 4, 48, t->bg2, 3); break;
            }
        }
    /* the walls, with the floor numbers painted on the left */
    gfx_rect(SW_X0 - 8, 0, 8, SCREEN_H, t->wall);
    gfx_rect(SW_X1, 0, 8, SCREEN_H, t->wall);
    gfx_vline(SW_X0 - 1, 0, SCREEN_H, t->wall_hi);
    gfx_vline(SW_X1, 0, SCREEN_H, t->wall_hi);
    for (int f = 0; f <= SW_FLOORS; f++) {
        int y = sy((float)(-f * SW_FLOOR_H));
        if (y < -8 || y > SCREEN_H) continue;
        gfx_hline(SW_X0 - 8, SW_X0 - 2, y, t->wall_hi);
    }
    /* the best height ever reached: a dotted line */
    int best_local = sv.best_floor - (run.level - 1) * SW_FLOORS;
    if (best_local > 1 && best_local <= SW_FLOORS) {
        int y = sy((float)(-(best_local - 1) * SW_FLOOR_H)) - 2;
        for (int x = SW_X0; x < SW_X1; x += 4) gfx_pset(x, y, C_LIGHT);
    }
}

static void draw_plat(const Plat *p) {
    int x = SW_X0 + (int)lroundf(p->x), y = sy(p->y);
    if (y < -24 || y > SCREEN_H + 4) return;
    int shake_x = (p->t > 0 && p->t < 20 && (frame_t / 2) % 2) ? 1 : 0;
    x += shake_x;
    uint8_t m[PAL_COUNT];
    for (int c = 0; c < p->w; c++) {
        int tx = x + c * SW_TILE;
        switch (p->type) {
        case PT_BASE:
            gfx_rect(tx, y, SW_TILE, 8, TH()->rock);
            gfx_hline(tx, tx + 15, y, TH()->rock_hi);
            gfx_pset(tx + 5, y + 4, TH()->bg);
            break;
        case PT_CLOUD: spr_draw(&sw_spr[K_CLOUD], tx, y - 2, 0); break;
        case PT_BUBBLE: spr_draw(&sw_spr[K_BUBBLE], tx, y - 2, 0); break;
        case PT_CRATE: spr_draw(&sw_spr[K_CRATE], tx, y, 0); break;
        case PT_ROCK:
            pal_identity(m);
            m[C_GREY] = TH()->rock_hi;
            m[C_SLATE] = TH()->rock;
            spr_draw_ex(&sw_spr[K_ROCK], tx, y, 0, m, -1);
            break;
        case PT_COIN: spr_draw(&sw_spr[K_COINBLOCK], tx, y, 0); break;
        case PT_MINE: spr_draw(&sw_spr[K_MINE], tx, y - 4, (frame_t / 8 + c) % 2 ? SPR_FLIPX : 0); break;
        case PT_BOMB:
            /* the plunger sinks and the block blinks while its fuse burns */
            if (p->shot && (p->t < 20 ? (frame_t / 2) % 2 : (frame_t / 6) % 2)) {
                pal_identity(m);
                m[C_RED] = C_YELLOW;
                m[C_MAROON] = C_ORANGE;
                spr_draw_ex(&sw_spr[K_BOMB], tx, y, 0, m, -1);
            } else {
                spr_draw(&sw_spr[K_BOMB], tx, y, 0);
            }
            gfx_rect(tx + 6, y - (p->shot ? 1 : 4), 4, p->shot ? 1 : 4, C_GREY);
            gfx_hline(tx + 4, tx + 11, y - (p->shot ? 2 : 5), C_LIGHT);
            break;
        case PT_ZAP: spr_draw(&sw_spr[K_ZAPBLOCK], tx, y, 0); break;
        case PT_METAL:
            if (p->flash > 0) { pal_identity(m); m[C_GREY] = C_WHITE; m[C_LIGHT] = C_WHITE; m[C_SLATE] = C_LIGHT; spr_draw_ex(&sw_spr[K_METAL], tx, y, 0, m, -1); }
            else spr_draw(&sw_spr[K_METAL], tx, y, 0);
            break;
        case PT_STAR: spr_draw(&sw_spr[K_STARBLOCK], tx, y - 8 + ((frame_t / 10) % 2), 0); break;
        }
    }
    if (p->type == PT_ZAP) {
        /* the field: two crackling bars, across or up and down */
        for (int s2 = 0; s2 < 2; s2++) {
            int fx, fy, fw, fh;
            if (p->hp == 0) { fx = s2 ? x + 16 : x - 32; fy = y + 1; fw = 32; fh = 6; }
            else { fx = x + 5; fy = s2 ? y + 8 : y - 32; fw = 6; fh = 32; }
            gfx_dither(fx, fy, fw, fh, C_NAVY, 8);
            for (int k = 0; k < 6; k++) {
                int t = frame_t * 3 + k * 7 + s2 * 5;
                int zx = p->hp == 0 ? fx + t % fw : fx + (k + frame_t / 2) % fw;
                int zy = p->hp == 0 ? fy + (k + frame_t / 2) % fh : fy + t % fh;
                gfx_pset(zx, zy, k % 2 ? C_YELLOW : C_CYAN);
            }
        }
    }
}

static void draw_foes(void) {
    for (int i = 0; i < MAX_FOES; i++) {
        Foe *f = &foes[i];
        if (!f->alive) continue;
        int x = SW_X0 + (int)lroundf(f->x), y = sy(f->y);
        if (y < -20 || y > SCREEN_H + 10) continue;
        bool alt = (f->t / 8) % 2;
        switch (f->kind) {
        case FE_BAT: spr_draw(&sw_spr[!f->state ? K_BAT_SLEEP : alt ? K_BAT1 : K_BAT2], x - 1, y - 1, f->vx > 0 ? SPR_FLIPX : 0); break;
        case FE_MINNOW: spr_draw(&sw_spr[alt ? K_MINNOW1 : K_MINNOW2], x, y, f->vx > 0 ? SPR_FLIPX : 0); break;
        case FE_JELLY: spr_draw(&sw_spr[K_JELLY], x, y + (alt ? 1 : 0), 0); break;
        case FE_JELLY_S: spr_draw(&sw_spr[K_JELLY_SMALL], x, y, 0); break;
        case FE_SQUID:
            if (f->state == 0) {
                /* the mark where the next squid will drop */
                if ((f->t / 4) % 2) {
                    gfx_rect(x + 5, 2, 2, 7, C_PINK);
                    gfx_rect(x + 5, 11, 2, 2, C_PINK);
                }
                gfx_hline(x, x + 11, 0, C_PINK);
            } else {
                spr_draw(&sw_spr[alt ? K_SQUID1 : K_SQUID2], x, y, 0);
            }
            break;
        }
    }
    for (int i = 0; i < MAX_COINS; i++) {
        Coin *c = &coins[i];
        if (!c->alive) continue;
        int y = sy(c->y);
        if (y < -8 || y > SCREEN_H) continue;
        spr_draw(&sw_spr[(frame_t / 8 + i) % 2 ? K_COIN1 : K_COIN2], SW_X0 + (int)c->x, y, 0);
    }
}

static void draw_kip(void) {
    if (state == S_DEAD && state_t > 20) return;
    int x = SW_X0 + (int)lroundf(K.x) - 4, y = sy(K.y) - 4;
    int fl = K.dir < 0 ? SPR_FLIPX : 0;
    int spr;
    if (state == S_DEAD) spr = K_DEAD;
    else if (K.star > 0) spr = K_STAR;
    else if (K.stun > 0) spr = K_STUN;
    else if (!K.ground) spr = K.vy < 0 ? K_JUMP : K_FALL;
    else if (btn(BTN_UP) && btn(BTN_B)) spr = K_SHOOT_UP;
    else if (fabsf(K.vx) > 0.1f) spr = (K.anim / 6) % 2 ? K_RUN1 : K_RUN2;
    else spr = K_STAND;
    if (K.inv > 0 && K.stun == 0 && K.star == 0 && (frame_t / 3) % 2) return;
    if (K.star > 0 && (frame_t / 2) % 2) spr_draw_outline(&sw_spr[spr], x, y, fl, C_YELLOW);
    else if (!K.ground && K.stun == 0 && K.star == 0 && K.air_used >= 1 + run.items[IT_DJUMP]) {
        /* out of jumps: her suit turns red */
        uint8_t m[PAL_COUNT];
        pal_identity(m);
        m[C_BLUE] = C_RED;
        m[C_NAVY] = C_MAROON;
        spr_draw_ex(&sw_spr[spr], x, y, fl, m, -1);
    } else spr_draw(&sw_spr[spr], x, y, fl);
    if (K.stun > 0) {
        for (int k = 0; k < 3; k++) {
            float a = (float)(frame_t * 0.2f + k * 2.1f);
            gfx_pset(x + 8 + (int)(cosf(a) * 6), y + 1 + (int)(sinf(a) * 2), C_YELLOW);
        }
    }
}

static void draw_shots(void) {
    for (int i = 0; i < MAX_SHOTS; i++)
        if (shots[i].alive) spr_draw(&sw_spr[K_SHOT], SW_X0 + (int)shots[i].x - 2, sy(shots[i].y) - 2, 0);
    for (int i = 0; i < MAX_ESHOTS; i++)
        if (eshots[i].alive) spr_draw(&sw_spr[K_ORB], SW_X0 + (int)eshots[i].x - 3, sy(eshots[i].y) - 3, 0);
}

static void draw_owl(void) {
    if (owl.key_out) spr_draw(&sw_spr[K_KEY], SW_X0 + (int)owl.kx, sy(owl.ky), 0);
    if (!owl.on) return;
    int x = SW_X0 + (int)owl.x, y = sy(owl.y);
    spr_draw(&sw_spr[(frame_t / 6) % 2 ? K_OWL1 : K_OWL2], x, y, owl.dir > 0 ? SPR_FLIPX : 0);
    if (!owl.done && !owl.fled) {
        /* how long it will stay */
        int w = 16 * (OWL_TIME - owl.t) / OWL_TIME;
        gfx_rect(x, y - 4, 16, 2, C_INK);
        gfx_rect(x, y - 4, w, 2, C_LIGHT);
    }
}

static void draw_boss(void) {
    if (!boss.on || (boss.dead && boss.death_t > 90)) return;
    int by = 26;
    int cx = SW_X0 + SHAFT_W / 2;
    uint8_t m[PAL_COUNT];
    pal_identity(m);
    if (boss.flash > 0 && (frame_t / 2) % 2) for (int k = 0; k < PAL_COUNT; k++) m[k] = C_WHITE;
    /* the dark around the eye */
    gfx_dither_circle(cx, by + 12, 26, C_INK, 10);
    for (int h = 0; h < 2; h++) {
        if (boss.hand_hp[h] <= 0) continue;
        int hx = cx + (h ? 52 : -68);
        if (boss.warn[h] && (frame_t / 3) % 2) for (int yy = by + 30; yy < SCREEN_H; yy += 4) gfx_pset(hx + 8, yy, C_MAGENTA);
        if (boss.bolt[h]) {
            gfx_rect(hx + 5, by + 30, 6, SCREEN_H, C_PINK);
            gfx_rect(hx + 7, by + 30, 2, SCREEN_H, C_WHITE);
        }
        spr_draw_ex(&sw_spr[boss.bolt[h] || boss.warn[h] ? K_HAND_OPEN : K_HAND], hx, by + 16, h ? SPR_FLIPX : 0, m, -1);
    }
    spr_draw_ex(&sw_spr[boss.t % 120 < 8 ? K_EYE_SHUT : K_EYE], cx - 14, by, 0, m, -1);
    if (!boss.dead) {
        /* the pupil follows Kip */
        int px = SW_X0 + (int)(K.x + KW / 2), py = sy(K.y);
        float dx = (float)(px - cx), dy = (float)(py - (by + 12)), d = sqrtf(dx * dx + dy * dy) + 0.01f;
        if (boss.t % 120 >= 8) gfx_rect(cx - 2 + (int)(dx / d * 5), by + 10 + (int)(dy / d * 4), 4, 4, C_INK);
        gfx_rect(cx - 30, by - 8, 60, 3, C_INK);
        gfx_rect(cx - 30, by - 8, 60 * boss.eye_hp / EYE_HP, 3, C_MAGENTA);
    }
}

static void draw_grinder(void) {
    int y = GRINDER_Y;
    gfx_rect(SW_X0, y + 4, SHAFT_W, SCREEN_H - y - 4, C_MAROON);
    int roll = frame_t % 8;
    for (int x = -8 + roll; x < SHAFT_W; x += 8) {
        int px = SW_X0 + x;
        if (px < SW_X0 - 4) continue;
        gfx_line(px, y + 6, px + 4, y - 2, C_LIGHT);
        gfx_line(px + 4, y - 2, px + 8, y + 6, C_GREY);
    }
    gfx_hline(SW_X0, SW_X1 - 1, y + 6, C_RED);
    for (int x = roll * 2; x < SHAFT_W; x += 16) gfx_rect(SW_X0 + x, y + 9, 6, 3, C_WINE);
}

static void draw_parts(void) {
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        int x = SW_X0 + (int)p->x, y = sy(p->y);
        if (p->kind == 1) gfx_circ(x, y, 2, p->col);
        else gfx_rect(x, y, 2, 2, p->col);
    }
}

static void draw_hud(void) {
    const Theme *t = TH();
    gfx_rect(0, 0, SW_X0 - 8, SCREEN_H, C_INK);
    gfx_rect(SW_X1 + 8, 0, SCREEN_W - SW_X1 - 8, SCREEN_H, C_INK);
    gfx_vline(SW_X0 - 9, 0, SCREEN_H, t->wall);
    gfx_vline(SW_X1 + 8, 0, SCREEN_H, t->wall);
    static const char *LV[4] = {"THE ROOTS", "SPARK WORKS", "THE SKY REEF", "THE EYE"};
    char buf[32];
    /* left: the level and the floor */
    snprintf(buf, sizeof buf, "LEVEL %d", run.level);
    tiny_draw(buf, 2, 4, C_SLATE);
    text_wrap(LV[run.level - 1], 2, 12, 44, t->wall_hi, 8);
    /* the floor markers, level with each floor */
    gfx_clip(0, 26, SW_X0 - 8, 118);
    for (int f = 1; f <= SW_FLOORS; f++) {
        int y = sy((float)(-f * SW_FLOOR_H));
        if (y < 20 || y > 150) continue;
        snprintf(buf, sizeof buf, "%d", (run.level - 1) * SW_FLOORS + f);
        bool here = f == floor_of(K.y + KH - 1);
        tiny_draw(buf, SW_X0 - 11 - tiny_width(buf), y - 5, here ? C_WHITE : f % 10 == 0 ? C_YELLOW : C_DUSK);
        gfx_hline(SW_X0 - 10, SW_X0 - 9, y, here ? C_WHITE : C_DUSK);
    }
    gfx_noclip();
    tiny_draw("FLOOR", 2, 150, C_SLATE);
    snprintf(buf, sizeof buf, "%d", global_floor());
    text_draw(buf, 2, 158, C_WHITE);
    /* right: cogs, keys, jumps, items */
    int rx = SW_X1 + 12;
    spr_draw(&sw_spr[K_COIN1], rx, 5, 0);
    snprintf(buf, sizeof buf, "%d", run.cogs);
    text_draw(buf, rx + 9, 4, C_YELLOW);
    tiny_draw("KEYS", rx, 22, C_SLATE);
    for (int k = 0; k < 3; k++) {
        if (k < run.keys) spr_draw(&sw_spr[K_KEY], rx + k * 11, 30, 0);
        else gfx_rectb(rx + k * 11, 30, 8, 10, C_DUSK);
    }
    tiny_draw("JUMPS", rx, 48, C_SLATE);
    int aj = 1 + run.items[IT_DJUMP];
    for (int k = 0; k < aj; k++) gfx_rect(rx + k * 6, 56, 4, 4, k < aj - K.air_used || K.ground ? C_CYAN : C_DUSK);
    static const char *ABBR[IT_COUNT] = {"REC", "POW", "JMP", "LFT", "MAG", "LTN"};
    int iy = 70;
    for (int i = 0; i < IT_COUNT; i++) {
        if (!run.items[i]) continue;
        snprintf(buf, sizeof buf, "%s%s", ABBR[i], run.items[i] > 1 ? "+" : "");
        tiny_draw(buf, rx, iy, C_LIGHT);
        if (run.items[i] > 1) { snprintf(buf, sizeof buf, "%d", run.items[i]); tiny_draw(buf, rx + 26, iy, C_LIGHT); }
        iy += 8;
    }
    if (sv.best_floor) {
        tiny_draw("BEST", rx, 150, C_SLATE);
        snprintf(buf, sizeof buf, "%d", sv.best_floor);
        text_draw(buf, rx, 158, C_GREY);
    }
}

static void draw_play(void) {
    int sx = shake > 0 ? (frame_t % 3) - 1 : 0;
    gfx_cls(C_INK);
    gfx_clip(SW_X0 - 8, 0, SHAFT_W + 16, SCREEN_H);
    gfx_camera(sx, 0);
    draw_shaft();
    gfx_clip(SW_X0, 0, SHAFT_W, SCREEN_H);
    for (int i = 0; i < MAX_PLATS; i++) if (plats[i].alive) draw_plat(&plats[i]);
    draw_foes();
    draw_owl();
    draw_boss();
    draw_shots();
    draw_kip();
    draw_parts();
    draw_grinder();
    gfx_camera(0, 0);
    gfx_noclip();
    draw_hud();
    if (boss.on && !boss.dead && K.y + KH <= LEVEL_TOP + 1) tiny_center("THE EYE MUST CLOSE FIRST", SW_X0 + SHAFT_W / 2, 90, C_PINK);
}

static void draw_title(void) {
    gfx_cls(C_NIGHT);
    /* a shaft seen from inside, platforms tumbling past */
    gfx_rect(96, 0, 128, 180, C_INK);
    for (int i = 0; i < 12; i++) {
        int y = (i * 37 + frame_t / 2) % 200 - 10;
        int x = 104 + (i * 53) % 96;
        spr_draw(&sw_spr[i % 3 == 0 ? K_CLOUD : i % 3 == 1 ? K_CRATE : K_ROCK], x, y, 0);
    }
    gfx_rect(88, 0, 8, 180, C_EARTH);
    gfx_rect(224, 0, 8, 180, C_EARTH);
    spr_draw_scaled(&sw_spr[K_KIP_BIG], 20, 60, 2, 0);
    for (int x = 96; x < 224; x += 8) {
        gfx_line(x, 176, x + 4, 168, C_LIGHT);
        gfx_line(x + 4, 168, x + 8, 176, C_GREY);
    }
    gfx_rect(40, 10, 240, 50, C_INK);
    gfx_rectb(40, 10, 240, 50, C_DUSK);
    static const uint8_t grad[] = {C_WHITE, C_ICE, C_CYAN, C_SKY};
    ui_fancy_center("SKYWELL", 160, 16, 3, grad, 4, C_NAVY, C_INK);
    text_center("CLIMB OUT BEFORE THE GRINDER", 160, 46, C_LIGHT);
    if ((state_t / 20) % 2) { gfx_rect(128, 106, 64, 13, C_INK); text_center("PRESS " GLYPH_A, 160, 109, C_WHITE); }
    char buf[48];
    if (sv.best_floor) {
        snprintf(buf, sizeof buf, "BEST FLOOR %d", sv.best_floor);
        tiny_center(buf, 160, 128, C_YELLOW);
        snprintf(buf, sizeof buf, "MOST COGS %d  MOST KEYS %d", sv.most_cogs, sv.most_keys);
        tiny_center(buf, 160, 136, C_GREY);
    }
    gfx_rect(236, 142, 84, 38, C_INK);
    text_draw(GLYPH_A " START", 240, 146, C_LIGHT);
    text_draw(GLYPH_B " LIBRARY", 240, 157, C_LIGHT);
    text_draw("SELECT HELP", 240, 168, C_LIGHT);
}

static void draw_shop(void) {
    gfx_cls(C_NIGHT);
    for (int y = 0; y < 180; y += 4) gfx_dither(0, y, 320, 2, C_DUSK, 2);
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center("THE TINKER", 160, 8, 2, grad, 3, C_MAROON, C_INK);
    spr_draw_scaled(&sw_spr[K_TINKER], 134, 30, 2, 0);
    tiny_center("\"COGS FOR GEAR, DIVER. CHOOSE.\"", 160, 70, C_LIGHT);
    for (int k = 0; k < 3; k++) {
        int x = 18 + k * 98, y = 82;
        bool sel = k == shop_sel;
        ui_panel(x, y, 90, 58, C_INK, sel ? C_YELLOW : C_DUSK);
        int it = offer[k];
        if (it < 0) { text_center("SOLD", x + 45, y + 24, C_DUSK); continue; }
        text_center(IT_NAME[it], x + 45, y + 6, sel ? C_WHITE : C_LIGHT);
        text_wrap(IT_DESC[it], x + 5, y + 20, 80, C_GREY, 8);
        char buf[16];
        snprintf(buf, sizeof buf, "%d", IT_PRICE[it]);
        spr_draw(&sw_spr[K_COIN1], x + 32, y + 44, 0);
        text_draw(buf, x + 42, y + 44, run.cogs >= IT_PRICE[it] ? C_YELLOW : C_RED);
    }
    char buf[40];
    snprintf(buf, sizeof buf, "COGS %d   KEYS %d", run.cogs, run.keys);
    text_center(buf, 160, 148, C_YELLOW);
    text_center(GLYPH_A " BUY   " GLYPH_B " CLIMB ON", 160, 164, C_LIGHT);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    draw_grinder();
    static const uint8_t grad[] = {C_PINK, C_RED, C_MAROON};
    ui_fancy_center("GROUND DOWN", 160, 40, 2, grad, 3, C_INK, C_NIGHT);
    char buf[48];
    snprintf(buf, sizeof buf, "FLOOR %d", (run.level - 1) * SW_FLOORS + top_floor);
    text_center(buf, 160, 76, C_WHITE);
    snprintf(buf, sizeof buf, "COGS %d   KEYS %d", run.cogs_total, run.keys);
    text_center(buf, 160, 90, C_LIGHT);
    snprintf(buf, sizeof buf, "BEST FLOOR %d", sv.best_floor);
    tiny_center(buf, 160, 106, C_YELLOW);
    if (state_t > 20) {
        text_center(GLYPH_A " CLIMB AGAIN", 160, 124, (state_t / 20) % 2 ? C_WHITE : C_LIGHT);
        text_center(GLYPH_B " TITLE", 160, 138, C_GREY);
    }
}

/* how to play: three pages from the title screen */
static void draw_help(void) {
    gfx_cls(C_NIGHT);
    for (int y = 0; y < 180; y += 4) gfx_dither(0, y, 320, 2, C_DUSK, 2);
    static const uint8_t grad[] = {C_WHITE, C_ICE, C_CYAN};
    ui_fancy_center("HOW TO PLAY", 160, 6, 2, grad, 3, C_NAVY, C_INK);
    static const char *const PAGE[HELP_PAGES] = {
        "CLIMB! THE GRINDER AT THE FOOT OF THE SCREEN RISES\n"
        "WHENEVER YOU CLIMB ABOVE THE MIDDLE. IT IS THE ONLY\n"
        "THING THAT CAN END YOUR RUN, SO TAKE YOUR TIME.\n\n"
        GLYPH_A "  JUMP. HOLD IT TO JUMP HIGHER, AND TO HOP\n"
        "   AGAIN THE MOMENT YOU LAND. " GLYPH_A " IN THE AIR JUMPS\n"
        "   ONCE MORE. OUT OF JUMPS, YOUR SUIT TURNS RED.\n"
        GLYPH_B "  SHOOT. HOLD " GLYPH_UP " TO SHOOT UP, " GLYPH_DOWN " IN THE AIR\n"
        "   TO SHOOT DOWN.",
        "PLATFORMS CRUMBLE SOON AFTER YOU LAND: CLOUDS AT\n"
        "ONCE, BOXES A BIT LATER, BRICKS AFTER A SECOND.\n"
        "SHOTS BREAK CLOUDS TOO. SHOOT COIN BLOCKS FOR COGS.\n\n"
        "BUMPING A CREATURE OR A HAZARD KNOCKS YOU FLYING\n"
        "FOR A MOMENT. WHEN YOU COME ROUND YOU STILL HAVE\n"
        "YOUR JUMP IN THE AIR: USE IT!\n\n"
        "LANDING ON A CREATURE IS SAFE: YOU BOUNCE OFF IT\n"
        "WITH YOUR JUMPS BACK. BATS AND JELLIES POP.",
        "BATS SLEEP UNTIL YOU PASS CLOSE OR UNDERNEATH.\n"
        "SHOOT THEM, BOUNCE ON THEM, OR OUTCLIMB THEM.\n\n"
        "STAR BLOCKS: LAND ON ONE AND HOLD " GLYPH_A " TO FLY.\n"
        "SHOOT A ZAPPER'S MIDDLE TO TURN ITS FIELD ROUND.\n"
        "TNT GOES OFF A SECOND AFTER YOU STEP ON IT.\n"
        "CLOUD MINES BURST INTO CLOUDS. SHOOT THEM FIRST.\n\n"
        "THE TINKER SELLS GEAR FOR COGS AFTER EACH LEVEL.\n"
        "SHOOT THE OWL ON FLOOR 12 FOR A KEY.",
    };
    ui_panel(10, 30, 300, 128, C_INK, C_DUSK);
    text_draw(PAGE[help_page], 18, 38, C_LIGHT);
    char buf[16];
    snprintf(buf, sizeof buf, "%d/%d", help_page + 1, HELP_PAGES);
    tiny_center(buf, 160, 162, C_GREY);
    text_center(GLYPH_A " NEXT   " GLYPH_B " BACK", 160, 170, C_LIGHT);
}

static void draw_end(void) {
    int t = state_t;
    bool true_end = run.level == 4;
    gfx_cls(C_NAVY);
    for (int y = 0; y < 100; y++) gfx_hline(0, 319, y, y < 30 ? C_NAVY : y < 60 ? C_BLUE : C_SKY);
    gfx_dither(0, 26, 320, 8, C_BLUE, 8);
    gfx_dither(0, 56, 320, 8, C_SKY, 8);
    gfx_circ(250, 40 + (t < 200 ? (200 - t) / 10 : 0), 14, C_YELLOW);
    gfx_rect(0, 100, 320, 80, C_FOREST);
    gfx_rect(120, 100, 80, 80, C_INK); /* the mouth of the well */
    gfx_rect(116, 100, 4, 80, C_EARTH);
    gfx_rect(200, 100, 4, 80, C_EARTH);
    int ky = t < 90 ? 150 - t : 60;
    spr_draw_scaled(&sw_spr[K_KIP_BIG], 144, ky, 1, 0);
    ui_panel(40, 118, 240, 50, C_INK, C_CYAN);
    if (t > 60) text_center("KIP CLIMBS OUT INTO THE DAWN.", 160, 124, C_WHITE);
    if (t > 150) text_center(true_end ? "THE WELL EYE IS SHUT FOR GOOD." : "BELOW, SOMETHING STILL WATCHES...", 160, 136, true_end ? C_LIME : C_PINK);
    if (t > 240) {
        char buf[48];
        snprintf(buf, sizeof buf, "COGS %d  KEYS %d", run.cogs_total, run.keys);
        text_center(buf, 160, 150, C_YELLOW);
    }
    if (t > 360 && (t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 172, C_WHITE);
}

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    int x = 2, y = 2, rowh = 0;
    for (int i = 0; i < K_SPRITE_COUNT; i++) {
        Sprite *s = &sw_spr[i];
        if (!s->px) continue;
        if (x + s->w * 2 > 318) { x = 2; y += rowh + 2; rowh = 0; }
        spr_draw_scaled(s, x, y, 2, 0);
        x += s->w * 2 + 3;
        if (s->h * 2 > rowh) rowh = s->h * 2;
    }
}

static void sw_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_PLAY: case S_DEAD: draw_play(); break;
    case S_SHOP: draw_shop(); break;
    case S_OVER: draw_over(); break;
    case S_END: draw_end(); break;
    case S_HELP: draw_help(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void sw_load(void) {
    sw_art_load();
    sw_audio_load();
}

static void sw_start(void) {
    load_save();
    state = S_TITLE;
    state_t = 0;
    sheet_mode = false;
    god = false;
    autoplay = false;
    run.level = 1;
    game_set_pausable(false);
    music_play(SW_MUS_TITLE);
}

static void sw_quit(void) { save_now(); }

static void sw_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_NIGHT);
    gfx_rect(x + 30, y, 82, h, C_INK);
    gfx_rect(x + 26, y, 4, h, C_EARTH);
    gfx_rect(x + 112, y, 4, h, C_EARTH);
    for (int i = 0; i < 5; i++) {
        int py = (i * 23 + t / 3) % 70 - 8;
        spr_draw(&sw_spr[i % 2 ? K_CLOUD : K_CRATE], x + 36 + (i * 29) % 60, y + py, 0);
    }
    spr_draw(&sw_spr[(t / 8) % 2 ? K_JUMP : K_FALL], x + 64, y + 22 + (int)(sinf(t * 0.1f) * 6), 0);
    for (int xx = x + 30; xx < x + 112; xx += 8) {
        gfx_line(xx, y + 62, xx + 4, y + 55, C_LIGHT);
        gfx_line(xx + 4, y + 55, xx + 8, y + 62, C_GREY);
    }
}

static int sw_query(const char *key, int *out) {
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "bot_ty")) { *out = bot_target >= 0 ? (int)plats[bot_target].y : 9999; return 1; }
    if (!strcmp(key, "bot_tx")) { *out = bot_target >= 0 ? (int)plats[bot_target].x : 9999; return 1; }
    if (!strcmp(key, "bot_now")) { *out = (int)bot_now; return 1; }
    if (!strcmp(key, "level")) { *out = run.level; return 1; }
    if (!strcmp(key, "floor")) { *out = global_floor(); return 1; }
    if (!strcmp(key, "top_floor")) { *out = top_floor; return 1; }
    if (!strcmp(key, "kx")) { *out = (int)lroundf(K.x); return 1; }
    if (!strcmp(key, "ky")) { *out = (int)lroundf(K.y); return 1; }
    if (!strcmp(key, "ground")) { *out = K.ground; return 1; }
    if (!strcmp(key, "stun")) { *out = K.stun; return 1; }
    if (!strcmp(key, "star")) { *out = K.star; return 1; }
    if (!strcmp(key, "air_used")) { *out = K.air_used; return 1; }
    if (!strcmp(key, "cam_y")) { *out = (int)lroundf(cam_y); return 1; }
    if (!strcmp(key, "cogs")) { *out = run.cogs; return 1; }
    if (!strcmp(key, "cogs_total")) { *out = run.cogs_total; return 1; }
    if (!strcmp(key, "keys")) { *out = run.keys; return 1; }
    if (!strcmp(key, "owl_on")) { *out = owl.on; return 1; }
    if (!strcmp(key, "owl_hp")) { *out = owl.hp; return 1; }
    if (!strcmp(key, "owl_gone")) { *out = run.owl_gone; return 1; }
    if (!strcmp(key, "key_out")) { *out = owl.key_out; return 1; }
    if (!strcmp(key, "boss_on")) { *out = boss.on; return 1; }
    if (!strcmp(key, "eye_hp")) { *out = boss.eye_hp; return 1; }
    if (!strcmp(key, "hand0_hp")) { *out = boss.hand_hp[0]; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = boss.dead; return 1; }
    if (!strcmp(key, "best_floor")) { *out = sv.best_floor; return 1; }
    if (!strcmp(key, "most_cogs")) { *out = sv.most_cogs; return 1; }
    if (!strcmp(key, "most_keys")) { *out = sv.most_keys; return 1; }
    if (!strncmp(key, "item_", 5)) { *out = run.items[iclamp(atoi(key + 5), 0, IT_COUNT - 1)]; return 1; }
    if (!strncmp(key, "offer_", 6)) { *out = offer[iclamp(atoi(key + 6), 0, 2)]; return 1; }
    if (!strcmp(key, "plats")) { int n = 0; for (int i = 0; i < MAX_PLATS; i++) n += plats[i].alive; *out = n; return 1; }
    if (!strcmp(key, "foes")) { int n = 0; for (int i = 0; i < MAX_FOES; i++) n += foes[i].alive; *out = n; return 1; }
    if (!strcmp(key, "coins")) { int n = 0; for (int i = 0; i < MAX_COINS; i++) n += coins[i].alive; *out = n; return 1; }
    if (!strcmp(key, "bats_awake")) { int n = 0; for (int i = 0; i < MAX_FOES; i++) n += foes[i].alive && foes[i].kind == FE_BAT && foes[i].state; *out = n; return 1; }
    if (!strcmp(key, "eshots")) { int n = 0; for (int i = 0; i < MAX_ESHOTS; i++) n += eshots[i].alive; *out = n; return 1; }
    if (!strcmp(key, "eshot_up")) { int n = 0; for (int i = 0; i < MAX_ESHOTS; i++) n += eshots[i].alive && eshots[i].vy < 0; *out = n; return 1; }
    if (!strcmp(key, "inv")) { *out = K.inv; return 1; }
    if (!strcmp(key, "vy10")) { *out = (int)lroundf(K.vy * 10); return 1; }
    if (!strcmp(key, "zap_dir")) { *out = -1; for (int i = 0; i < MAX_PLATS; i++) if (plats[i].alive && plats[i].type == PT_ZAP) { *out = plats[i].hp; break; } return 1; }
    if (!strcmp(key, "squid_state")) { *out = -1; for (int i = 0; i < MAX_FOES; i++) if (foes[i].alive && foes[i].kind == FE_SQUID) { *out = foes[i].state; break; } return 1; }
    if (!strcmp(key, "foe0_y")) { *out = 9999; for (int i = 0; i < MAX_FOES; i++) if (foes[i].alive) { *out = (int)lroundf(foes[i].y); break; } return 1; }
    if (!strcmp(key, "foe0_x")) { *out = 9999; for (int i = 0; i < MAX_FOES; i++) if (foes[i].alive) { *out = (int)lroundf(foes[i].x); break; } return 1; }
    if (!strncmp(key, "count_", 6)) {
        /* count_T for platform type T */
        int t = atoi(key + 6), n = 0;
        for (int i = 0; i < MAX_PLATS; i++) n += plats[i].alive && plats[i].type == t;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "foe_", 4)) {
        int t = atoi(key + 4), n = 0;
        for (int i = 0; i < MAX_FOES; i++) n += foes[i].alive && foes[i].kind == t;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "gen_ok")) {
        /* every floor has a platform within a jump of one on the floor below */
        int bad = 0;
        for (int f = 1; f < SW_FLOORS; f++) {
            bool ok = false;
            for (int i = 0; i < MAX_PLATS && !ok; i++) {
                Plat *a = &plats[i];
                if (!a->alive || (int)lroundf(-a->y / SW_FLOOR_H) != f) continue;
                float ca = a->x + a->w * 8.0f;
                for (int j = 0; j < MAX_PLATS && !ok; j++) {
                    Plat *b = &plats[j];
                    if (!b->alive || (int)lroundf(-b->y / SW_FLOOR_H) != f - 1) continue;
                    if (fabsf(ca - (b->x + b->w * 8.0f)) <= 72 + (b->type == PT_BASE ? 104 : 0)) ok = true;
                }
            }
            if (!ok) bad++;
        }
        *out = bad;
        return 1;
    }
    return 0;
}

static void clear_world(void) {
    memset(plats, 0, sizeof plats);
    memset(foes, 0, sizeof foes);
    memset(coins, 0, sizeof coins);
    memset(eshots, 0, sizeof eshots);
}

static int sw_cheat(const char *cmd) {
    int a, b, c, d;
    if (sscanf(cmd, "seed %d", &a) == 1) { forced_seed = a; return 1; }
    if (!strcmp(cmd, "run")) { new_run(); return 1; }
    if (sscanf(cmd, "level %d", &a) == 1) { run.level = iclamp(a, 1, 4); start_level(); return 1; }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) { K.x = (float)a; K.y = (float)b; K.vx = K.vy = 0; K.stun = 0; if (K.y - cam_y < MID_Y) cam_y = K.y - MID_Y; return 1; }
    if (sscanf(cmd, "cogs %d", &a) == 1) { run.cogs = a; return 1; }
    if (sscanf(cmd, "keys %d", &a) == 1) { run.keys = a; return 1; }
    if (sscanf(cmd, "give %d", &a) == 1) { run.items[iclamp(a, 0, IT_COUNT - 1)]++; return 1; }
    if (!strcmp(cmd, "clear")) { clear_world(); return 1; }
    if (!strcmp(cmd, "god")) { god = !god; return 1; }
    if (sscanf(cmd, "plat %d %d %d %d", &a, &b, &c, &d) == 4) { add_plat(a, (float)b, (float)c, d); return 1; }
    if (sscanf(cmd, "foe %d %d %d", &a, &b, &c) == 3) { add_foe(a, (float)b, (float)c); return 1; }
    if (sscanf(cmd, "coin %d %d", &a, &b) == 2) { add_coin((float)a, (float)b, false); return 1; }
    if (sscanf(cmd, "top %d", &a) == 1) { top_floor = a; return 1; }
    if (!strcmp(cmd, "shop")) { open_shop(); return 1; }
    if (!strcmp(cmd, "done")) { level_done(); return 1; }
    if (sscanf(cmd, "offer %d %d %d", &a, &b, &c) == 3) { offer[0] = a; offer[1] = b; offer[2] = c; return 1; }
    if (sscanf(cmd, "owl_hp %d", &a) == 1) { owl.hp = a; return 1; }
    if (sscanf(cmd, "eye_hp %d", &a) == 1) { boss.eye_hp = a; return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    if (!strcmp(cmd, "autoplay")) { autoplay = !autoplay; bot_now = bot_prev = 0; return 1; }
    if (!strcmp(cmd, "botlog")) { autoplay = bot_log = true; bot_now = bot_prev = 0; log_mask = 0; log_run = 0; return 1; }
    return 0;
}

const GameDef GAME_SKYWELL = {
    "skywell",
    "SKYWELL",
    "1984",
    "ACTION CLIMBER",
    "THE PLATFORMS CRUMBLE, THE GRINDER FOLLOWS. KEEP CLIMBING.",
    {"HOLD 2 KEYS", "CLIMB OUT OF THE SKYWELL", "SHUT THE WELL EYE"},
    "D-PAD\tRUN; HOLD " GLYPH_UP "/" GLYPH_DOWN " TO AIM\n"
    GLYPH_A "\tJUMP (HOLD TO GO HIGHER)\n"
    GLYPH_A " IN AIR\tJUMP AGAIN\n"
    GLYPH_B "\tSHOOT (" GLYPH_DOWN " ONLY IN THE AIR)\n"
    "START\tPAUSE\n"
    "SELECT\tHOW TO PLAY (TITLE)\n\n"
    "LAND ON A CREATURE TO BOUNCE OFF IT.\n"
    "ONLY THE GRINDER CAN END A CLIMB.",
    C_CYAN, C_EARTH,
    sw_load, sw_start, sw_update, sw_draw, sw_quit, sw_label, sw_query, sw_cheat,
    "VELGRESS", 7,
};
