/* ROOFCAT - an auto-scrolling rooftop chase. Cartridge 03 of UFO 40,
 * a tribute to Ninpek (UFO 50 #3). See docs/games/03-roofcat.md.
 *
 * One continuous town scrolls past in four areas (with two bonus stretches)
 * and ends at the harbour, where Old Crab guards the stolen parcel. Then the
 * same town again, harder, after dark. */
#include "roofcat.h"

#define SCROLL 0.5f      /* the screen's steady pace to the right, px/frame */
#define PW 10
#define PH 13
#define GRAV 0.19f
#define JUMP1 (-3.55f)
#define JUMP2 (-3.2f)
#define RUN 1.3f         /* holding right: this much faster than the screen */
#define MAX_COLS (RC_WORLD_CHUNKS * RC_CHUNK_W)
#define SPIRIT_FRAMES 240
#define STAR_RANGE 112.0f
#define THROW_CD 9       /* frames between stars while B is held */
#define SPIRIT_CD 22     /* the spirit's twin shots come much slower */
#define START_LIVES 3
#define BOSS_HP 35
#define FLOOR_Y 96       /* the rooftop line (row 6) */
#define FISH_RANGE 72.0f /* fish (and jars in the water) wait for her this close */
#define MAX_JARS 4       /* jumping jars on screen at once */
#define HISCORES 5

enum { S_TITLE, S_INTRO, S_PLAY, S_OVER, S_ENDING };

enum {
    E_NONE,
    /* foes */
    E_PIGEON, E_RPIGEON, E_CROW, E_GECKO, E_GULL, E_LAMP, E_FFISH, E_WASP, E_SNAIL, E_TOAD, E_PELICAN,
    E_JAR, E_CRACKER, E_PUFFER, E_SPIDER, E_MAGPIE, E_FLASHER, E_SHEET, E_MOTH,
    E_BOSS, E_LEG,
    /* shots */
    E_STAR, E_SPIRIT_SHOT, E_PEBBLE, E_SEED, E_BUBBLE, E_BOMB, E_SHRAP, E_BEAM, E_DUST, E_ORB, E_ORBHALF, E_CLOUD,
    /* pickups */
    E_TOKEN, E_LETTER, E_CROWN, E_LANTERN, E_CATNIP,
    /* fx and markers */
    E_TEXT, E_DEBRIS, E_SPOT
};
#define IS_FOE(t) ((t) >= E_PIGEON && (t) <= E_MOTH)

/* foe variants (Ent.mode) */
enum { M_NONE, M_CEILING, M_WATER, M_MOVING, M_PURPLE, M_CHIMNEY, M_LIVELY, M_DIVE, M_BOSSFISH, M_POP };

typedef struct Ent {
    uint8_t type;
    bool alive;
    float x, y, vx, vy;
    int w, h, hp, t, cd, state, dir, flash, value, sub, n, mode, k;
    float hx, hy;
} Ent;

#define MAX_ENTS 200
static Ent ents[MAX_ENTS];

/* The save keeps the high-score board and two records, nothing mid-run:
 * like Ninpek, every run starts from the first rooftop. */
#define SAVE_MAGIC 0x52430004u
#define SAVE_MAGIC_V1 0x52430003u
typedef struct Save {
    uint32_t magic;
    uint32_t hi[HISCORES];
    uint16_t most_foes, most_letters;
} Save;
/* the first version also kept a checkpoint; only its best score carries over */
typedef struct SaveV1 {
    uint32_t magic;
    uint32_t best_score;
    uint8_t cp_valid, cp_loop, cp_area, cp_lives;
    uint32_t cp_score;
    uint16_t cp_snacks;
    uint8_t cp_letters, pad;
} SaveV1;
static Save sv;

static char map[RC_ROWS][MAX_COLS + 1];
/* snacks: four to a tile, one bit each (top left, top right, bottom left, bottom right) */
static uint8_t snackm[RC_ROWS][MAX_COLS];
static int bonus_got[RC_BONUS_AREAS];
static int state, state_t, frame_t, shake;
static int loop;
static uint32_t score, next_life_at;
static int lives, power, kills_since_power, letters_this_run, snacks_total, kills_run;
static int snack_points; /* points from snacks this run (tests) */
static int bomb_shards; /* shards made by bursting bombs (tests) */
static int new_rank; /* place on the board after the last run, -1 = none */
static bool recorded, over_in_bonus;
static float cam_x, arena_x;
static int spawned_chunks, cur_area;
static bool in_arena, boss_dead;
static int boss_i = -1;
static int crows_seen, crackers_seen, fishes_seen;
/* what took the last life (for the tests): a thing type, or 1000 + a tile, 2000 fell, 3000 crushed */
static int last_killer;
static bool sheet_mode;
static Rng rng;
/* Whole pixels the camera advanced this frame (0 or 1 at SCROLL 0.5). Anything
 * carried by the scroll moves by exactly this much, so it keeps the camera's
 * sub-pixel phase and never shimmers against the rooftops. */
static int carry_px;
/* Jitter probe for the tests: reversals of the drawn screen x and camera steps
 * that are not 0 or 1 pixel. */
static int jit_last_sx, jit_last_d, jit_last_cam, jit_rev, jit_cam_bad;
static bool jit_valid;

typedef struct Player {
    float x, y, vx, vy;
    bool ground, still;
    int jumps, face, throw_t, throw_cd, anim, drop_t;
    int leg; /* index of the crab leg stood on, -1 none */
    bool spirit;
    int spirit_t, dead_t;
} Player;
static Player pl;

/* ------------------------------------------------------------------ */
/* map                                                                  */

static char tile_at(int tx, int ty) {
    if (tx < 0 || tx >= MAX_COLS || ty < 0 || ty >= RC_ROWS) return '.';
    return map[ty][tx];
}
static bool solid_c(char c) {
    return c == '#' || c == 'w' || c == 'd' || c == 'O' || c == 'C' || c == 'H' || c == 'M' || c == 'B';
}
static bool solid_at(int tx, int ty) { return solid_c(tile_at(tx, ty)); }
static bool oneway_at(int tx, int ty) { return tile_at(tx, ty) == '='; }
static bool floor_at(int tx, int ty) { return solid_at(tx, ty) || oneway_at(tx, ty); }
static int area_of_col(int tx) { return iclamp(tx / (RC_CHUNK_W * RC_AREA_CHUNKS), 0, RC_AREAS - 1); }

static int bonus_of_col(int tx) {
    int c = tx / RC_CHUNK_W;
    for (int g = 0; g < RC_BONUS_AREAS; g++)
        if (c >= RC_BONUS_CHUNK[g] && c < RC_BONUS_CHUNK[g] + RC_BONUS_LEN) return g;
    return -1;
}

static bool box_solid(float x, float y, int w, int h) {
    int x0 = (int)floorf(x) >> 4, x1 = ((int)floorf(x) + w - 1) >> 4;
    int y0 = (int)floorf(y) >> 4, y1 = ((int)floorf(y) + h - 1) >> 4;
    if (y < 0) y0 = 0;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (solid_at(tx, ty)) return true;
    return false;
}

static Ent *spawn(int type, float x, float y) {
    for (int i = 0; i < MAX_ENTS; i++)
        if (!ents[i].alive) {
            Ent *e = &ents[i];
            memset(e, 0, sizeof *e);
            e->type = (uint8_t)type;
            e->alive = true;
            e->x = x; e->y = y; e->hx = x; e->hy = y;
            e->dir = -1;
            e->hp = 1;
            return e;
        }
    return NULL;
}

static int count_type(int type) {
    int n = 0;
    for (int i = 0; i < MAX_ENTS; i++) n += ents[i].alive && ents[i].type == type;
    return n;
}

static int count_awake(int type) {
    int n = 0;
    for (int i = 0; i < MAX_ENTS; i++) n += ents[i].alive && ents[i].type == type && ents[i].state != -1;
    return n;
}

static void puff(float x, float y, int col) {
    for (int i = 0; i < 8; i++) {
        Ent *d = spawn(E_DEBRIS, x, y);
        if (!d) return;
        float a = (float)i / 8.0f * 6.283f;
        d->vx = cosf(a) * 1.4f;
        d->vy = sinf(a) * 1.4f - 0.8f;
        d->t = 22;
        d->value = col;
    }
}

static void float_text(float x, float y, int v) {
    Ent *t = spawn(E_TEXT, x, y);
    if (t) { t->value = v; t->t = 40; }
}

static float pcx(void) { return pl.x + PW / 2; }
static float pcy(void) { return pl.y + PH / 2; }
static int cam_px(void) { return (int)floorf(cam_x); }
static float carry(void) { return (float)carry_px; }

/* ------------------------------------------------------------------ */
/* the world                                                            */

static void build_world(void) {
    for (int c = 0; c < RC_WORLD_CHUNKS; c++)
        for (int y = 0; y < RC_ROWS; y++)
            memcpy(&map[y][c * RC_CHUNK_W], RC_WORLD[c].rows[y], RC_CHUNK_W);
    for (int y = 0; y < RC_ROWS; y++) map[y][MAX_COLS] = 0;
    /* things become entities when their screen comes near; their tile is air,
     * or water for the things that live in it */
    for (int y = 0; y < RC_ROWS; y++)
        for (int x = 0; x < MAX_COLS; x++) {
            char c = map[y][x];
            if (c == 'f' || c == 'J') map[y][x] = '~';
            else if (!solid_c(c) && c != '=' && c != '^' && c != '~' && c != 'T') map[y][x] = '.';
        }
    arena_x = (float)((RC_WORLD_CHUNKS - 1) * RC_CHUNK_W * 16);
}

static void build_snacks(void) {
    for (int c = 0; c < RC_WORLD_CHUNKS; c++)
        for (int y = 0; y < RC_ROWS; y++)
            for (int x = 0; x < RC_CHUNK_W; x++)
                snackm[y][c * RC_CHUNK_W + x] = RC_WORLD[c].rows[y][x] == '*' ? 15 : 0;
    memset(bonus_got, 0, sizeof bonus_got);
}

/* foe stats: hit points and the egg (fish token) they drop; 0 = points on the spot */
static void foe_setup(Ent *e) {
    switch (e->type) {
    case E_PIGEON: e->w = 12; e->h = 11; e->x += 2; e->y += 5; e->hp = 2; e->value = 100; break;
    case E_RPIGEON: e->w = 12; e->h = 11; e->x += 2; e->y += 5; e->hp = 2; e->value = 200; break;
    case E_CROW: e->w = 12; e->h = 12; e->x += 2; e->y += 4; e->hp = 4; e->value = 200; e->cd = 60; break;
    case E_GECKO: e->w = 12; e->h = 7; e->x += 2; e->y += 9; e->hp = 1; e->value = 100; e->cd = 40; break;
    case E_GULL: e->w = 14; e->h = 8; e->y += 3; e->hp = 2; e->value = 100; break;
    case E_LAMP: e->w = 10; e->h = 12; e->x += 3; e->y += 2; e->hp = 10; e->value = 0; e->n = loop >= 2 ? 3 : 2; break;
    case E_FFISH: e->w = 10; e->h = 8; e->x += 3; e->y += 4; e->hp = 1; e->value = 200; e->cd = 30; break;
    case E_PUFFER: e->w = 11; e->h = 10; e->x += 2; e->y += 2; e->hp = 1; e->value = 200; e->cd = 30; break;
    case E_WASP: e->w = 8; e->h = 7; e->x += 4; e->y += 4; e->hp = 1; e->value = 100; break;
    case E_SNAIL:
        e->w = 12; e->h = 8; e->x += 2; e->y += 8;
        e->hp = loop >= 2 ? 2 : 1; e->value = loop >= 2 ? 200 : 100; e->sub = loop >= 2;
        break;
    case E_TOAD: e->w = 14; e->h = 11; e->x += 1; e->y += 5; e->hp = 4; e->value = 300; e->cd = 50; break;
    case E_PELICAN: e->w = 14; e->h = 10; e->y += 3; e->hp = 2; e->value = 200; break;
    case E_JAR: e->w = 10; e->h = 11; e->x += 3; e->y += 5; e->hp = 2; e->value = 200; e->cd = 30; break;
    case E_CRACKER: e->w = 10; e->h = 13; e->x += 3; e->y += 3; e->hp = 8; e->value = 0; break;
    case E_SPIDER: e->w = 9; e->h = 7; e->x += 3; e->hp = 1; e->value = 100; break;
    case E_MAGPIE: e->w = 12; e->h = 9; e->x += 2; e->y += 2; e->hp = 1; e->value = 200; break;
    case E_FLASHER: e->w = 12; e->h = 13; e->x += 2; e->y += 3; e->hp = 2; e->value = 300; e->cd = 20; break;
    case E_SHEET: e->w = 12; e->h = 12; e->x += 2; e->y += 2; e->hp = 2; e->value = 100; break;
    case E_MOTH: e->w = 10; e->h = 8; e->x += 3; e->y += 4; e->hp = 2; e->value = 200; break;
    }
    e->hx = e->x;
    e->hy = e->y;
}

static void spawn_chunk(int c) {
    for (int y = 0; y < RC_ROWS; y++)
        for (int x = 0; x < RC_CHUNK_W; x++) {
            char t = RC_WORLD[c].rows[y][x];
            float wx = (float)((c * RC_CHUNK_W + x) * 16), wy = (float)(y * 16);
            int tx = c * RC_CHUNK_W + x;
            int type = 0, mode = M_NONE;
            switch (t) {
            case 'p': type = E_PIGEON; break;
            case 'P': type = E_RPIGEON; break;
            case 'c':
                /* after dark the chimneys hold purple crows, or loose laundry
                 * that rises out of them */
                type = E_CROW;
                if (loop >= 2) {
                    if (crows_seen++ % 2) { type = E_SHEET; mode = M_CHIMNEY; }
                    else mode = M_PURPLE;
                }
                break;
            case 'g': type = E_GECKO; break;
            case 'u': type = E_GULL; break;
            case 'L': type = E_LAMP; break;
            case 'K': type = E_LAMP; mode = M_MOVING; break;
            case 'f':
                /* after dark most flying fish are pufferfish */
                type = (loop >= 2 && (fishes_seen++ % 3) != 0) ? E_PUFFER : E_FFISH;
                break;
            case 'J': type = E_JAR; mode = M_WATER; break;
            case 'a': type = E_WASP; break;
            case 'n': type = E_SNAIL; if (solid_at(tx, y - 1) && !floor_at(tx, y + 1)) mode = M_CEILING; break;
            case 't': type = E_TOAD; break;
            case 'e': type = E_PELICAN; break;
            case 'j': type = E_JAR; break;
            case 'x':
                /* after dark every other firecracker is a dust moth, the rest are livelier */
                type = E_CRACKER;
                if (loop >= 2) {
                    if (crackers_seen++ % 2) type = E_MOTH;
                    else mode = M_LIVELY;
                }
                break;
            case 's': type = E_SPIDER; break;
            case 'q': type = E_MAGPIE; break;
            case 'F': type = E_FLASHER; break;
            case '1': {
                Ent *l = spawn(E_LANTERN, wx + 4, wy + 2);
                if (l) { l->w = 8; l->h = 12; l->state = -1; }
                break;
            }
            case '!': {
                Ent *s = spawn(E_SPOT, wx, wy);
                if (s) { s->w = 16; s->h = 16; }
                break;
            }
            }
            if (type) {
                Ent *e = spawn(type, wx, wy);
                if (!e) continue;
                e->mode = mode;
                foe_setup(e);
                if (mode == M_CEILING) { e->y = wy; e->hy = e->y; }
                if (mode == M_CHIMNEY) { e->y = wy + 12; e->hy = e->y; e->value = 100; e->hp = 2; }
                e->state = -1; /* dormant until it scrolls into view */
            }
        }
}

static void spawn_ahead(void) {
    /* bring the next screens to life a little before they scroll in */
    while (spawned_chunks < RC_WORLD_CHUNKS && spawned_chunks * RC_CHUNK_W * 16 < cam_x + 480) spawn_chunk(spawned_chunks++);
}

static void place_player(float x) {
    memset(&pl, 0, sizeof pl);
    pl.x = x;
    pl.y = FLOOR_Y - PH;
    pl.face = 1;
    pl.ground = true;
    pl.leg = -1;
}

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    union { Save now; SaveV1 v1; } tmp;
    memset(&sv, 0, sizeof sv);
    sv.magic = SAVE_MAGIC;
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof(Save) && tmp.now.magic == SAVE_MAGIC) sv = tmp.now;
    else if (n == (int)sizeof(SaveV1) && tmp.v1.magic == SAVE_MAGIC_V1) sv.hi[0] = tmp.v1.best_score;
}

/* the run is over: its score goes on the board (once), and the records */
static void record_run(void) {
    if (recorded) return;
    recorded = true;
    new_rank = -1;
    if (kills_run > sv.most_foes) sv.most_foes = (uint16_t)imin(kills_run, 65535);
    if (letters_this_run > sv.most_letters) sv.most_letters = (uint16_t)letters_this_run;
    if (score > 0)
        for (int i = 0; i < HISCORES; i++)
            if (score > sv.hi[i]) {
                for (int k = HISCORES - 1; k > i; k--) sv.hi[k] = sv.hi[k - 1];
                sv.hi[i] = score;
                new_rank = i;
                break;
            }
    save_now();
}

/* Start a loop at an area: 0-3, or 4 for the harbour mole (tests). */
static void start_at(int area) {
    memset(ents, 0, sizeof ents);
    boss_i = -1;
    in_arena = false;
    boss_dead = false;
    carry_px = 0;
    jit_valid = false;
    crows_seen = crackers_seen = fishes_seen = 0;
    build_snacks();
    int chunk = area >= RC_AREAS ? RC_WORLD_CHUNKS - 2 : area * RC_AREA_CHUNKS;
    cam_x = (float)(chunk * RC_CHUNK_W * 16);
    spawned_chunks = chunk;
    cur_area = iclamp(area, 0, RC_AREAS - 1);
    place_player(cam_x + 48);
    power = 0;
    kills_since_power = 0;
    spawn_ahead();
    state = S_INTRO;
    state_t = 0;
    game_set_pausable(true);
    music_play(RC_MUS_AREA[cur_area]);
}

static void add_score(int v) {
    score += (uint32_t)v;
    while (score >= next_life_at) {
        /* a paper lantern drifts up: shoot it for a life */
        Ent *l = spawn(E_LANTERN, cam_x + (float)rng_range(&rng, 140, 280), 170);
        if (l) { l->w = 8; l->h = 12; l->state = 0; }
        /* 3000, then 7000, then every 5000 */
        next_life_at = next_life_at == 3000 ? 7000 : next_life_at + 5000;
    }
}

static void new_run(void) {
    score = 0;
    lives = START_LIVES;
    snacks_total = 0;
    snack_points = 0;
    letters_this_run = 0;
    kills_run = 0;
    recorded = false;
    new_rank = -1;
    next_life_at = 3000;
    loop = 1;
    start_at(0);
}

static void to_title(void) {
    state = S_TITLE;
    state_t = 0;
    game_set_pausable(false);
    music_play(RC_MUS_TITLE);
}

/* ------------------------------------------------------------------ */
/* player                                                               */

static int max_stars(void) { return 1 + power; }

static void lose_life(void) {
    if (pl.spirit || state != S_PLAY || pl.dead_t > 0) return;
    sfx_play_name("rc_die");
    shake = 12;
    puff(pl.x + PW / 2, pl.y + PH / 2, C_ORANGE);
    power = 0;
    kills_since_power = 0;
    if (lives <= 0) {
        pl.dead_t = 1;
        over_in_bonus = bonus_of_col((int)pcx() >> 4) >= 0;
        return;
    }
    lives--;
    /* the spirit floats down from the top of the screen */
    pl.spirit = true;
    pl.spirit_t = SPIRIT_FRAMES;
    pl.vx = pl.vy = 0;
    pl.y = -PH;
    pl.leg = -1;
    pl.throw_cd = 0;
    sfx_play_name("rc_spirit");
}

static void revive(void) {
    /* back in the flesh where the spirit is, with no grace period */
    pl.spirit = false;
    pl.vx = 0;
    pl.vy = 0;
    pl.jumps = 1;
    pl.ground = false;
    /* never materialise inside a wall */
    for (int guard = 0; guard < 40 && box_solid(pl.x, pl.y, PW, PH); guard++) pl.y -= 4;
    sfx_play_name("rc_revive");
}

static void move_x(float dx) {
    float step = dx > 0 ? 1.0f : -1.0f;
    float rem = fabsf(dx);
    while (rem > 0) {
        float s = rem >= 1 ? step : step * rem;
        if (box_solid(pl.x + s, pl.y, PW, PH)) break;
        pl.x += s;
        rem -= 1;
    }
}

/* the crab's legs are platforms you land on from above */
static bool leg_under(float feet_prev, float feet_new, int *which) {
    for (int i = 0; i < MAX_ENTS; i++) {
        Ent *l = &ents[i];
        if (!l->alive || l->type != E_LEG || l->y > 140) continue;
        if (pl.x + PW <= l->x || pl.x >= l->x + l->w) continue;
        if (feet_prev <= l->y + 1.5f && feet_new >= l->y) { *which = i; return true; }
    }
    return false;
}

static void move_y(float dy) {
    if (dy < 0) {
        float rem = -dy;
        while (rem > 0) {
            float s = rem >= 1 ? 1 : rem;
            if (box_solid(pl.x, pl.y - s, PW, PH)) { pl.vy = 0; break; }
            pl.y -= s;
            rem -= 1;
        }
        pl.ground = false;
        return;
    }
    float rem = dy;
    pl.ground = false;
    pl.leg = -1;
    while (rem > 0) {
        float s = rem >= 1 ? 1 : rem;
        float feet = pl.y + PH;
        if (box_solid(pl.x, pl.y + s, PW, PH)) {
            pl.y = floorf(pl.y + s);
            while (box_solid(pl.x, pl.y, PW, PH)) pl.y -= 1;
            pl.ground = true;
            break;
        }
        int ty = (int)(feet + s) >> 4;
        if (pl.drop_t == 0 && feet <= ty * 16 && feet + s >= ty * 16) {
            bool land = false;
            for (int tx = (int)pl.x >> 4; tx <= ((int)pl.x + PW - 1) >> 4; tx++) land |= oneway_at(tx, ty);
            if (land) { pl.y = (float)(ty * 16 - PH); pl.ground = true; break; }
        }
        int li;
        if (leg_under(feet, feet + s, &li)) { pl.y = ents[li].y - PH; pl.ground = true; pl.leg = li; break; }
        pl.y += s;
        rem -= 1;
    }
}

static bool standing(void) {
    if (pl.leg >= 0) {
        Ent *l = &ents[pl.leg];
        return l->alive && pl.x + PW > l->x && pl.x < l->x + l->w && l->y <= 140;
    }
    int ty = ((int)pl.y + PH) >> 4;
    if (((int)pl.y + PH) & 15) return box_solid(pl.x, pl.y + 1, PW, PH);
    for (int tx = (int)pl.x >> 4; tx <= ((int)pl.x + PW - 1) >> 4; tx++)
        if (solid_at(tx, ty) || (oneway_at(tx, ty) && pl.drop_t == 0)) return true;
    return false;
}

static bool on_ledge(void) {
    int ty = ((int)pl.y + PH) >> 4;
    bool ledge = false;
    for (int tx = (int)pl.x >> 4; tx <= ((int)pl.x + PW - 1) >> 4; tx++) {
        if (solid_at(tx, ty)) return false;
        ledge |= oneway_at(tx, ty);
    }
    return ledge;
}

static void throw_star(void) {
    Ent *s = spawn(E_STAR, pl.face > 0 ? pl.x + PW : pl.x - 8, pl.y + 3);
    if (s) { s->w = 8; s->h = 8; s->vx = 3.6f * pl.face; s->t = (int)(STAR_RANGE / 3.6f); }
    pl.throw_cd = THROW_CD;
    pl.throw_t = 8;
    sfx_play_name("rc_throw");
}

static int snack_value(int g) {
    int n = g >= 0 ? bonus_got[g] : 0;
    return n < 100 ? 5 : n < 200 ? 20 : 100;
}

/* snacks come in a solid mass: every one she touches is eaten */
static void eat_snacks(void) {
    int px = (int)pl.x, py = (int)pl.y;
    for (int ty = py >> 4; ty <= (py + PH - 1) >> 4; ty++)
        for (int tx = px >> 4; tx <= (px + PW - 1) >> 4; tx++) {
            if (ty < 0 || ty >= RC_ROWS || tx < 0 || tx >= MAX_COLS || !snackm[ty][tx]) continue;
            for (int b = 0; b < 4; b++) {
                if (!(snackm[ty][tx] & (1 << b))) continue;
                int sx = tx * 16 + (b & 1) * 8, sy = ty * 16 + (b >> 1) * 8;
                if (!rects_overlap(px, py, PW, PH, sx + 1, sy + 1, 6, 6)) continue;
                snackm[ty][tx] = (uint8_t)(snackm[ty][tx] & ~(1 << b));
                int g = bonus_of_col(tx);
                int v = snack_value(g);
                if (g >= 0) bonus_got[g]++;
                snacks_total++;
                snack_points += v;
                add_score(v);
                sfx_play_name("rc_food");
            }
        }
}

static void update_spirit(void) {
    float c = carry();
    int dx = btn(BTN_RIGHT) - btn(BTN_LEFT), dy = btn(BTN_DOWN) - btn(BTN_UP);
    int age = SPIRIT_FRAMES - pl.spirit_t;
    pl.x += c + dx * 1.1f;
    pl.y += age < 40 ? 1.2f : dy * 1.1f; /* drifts down into view first */
    pl.x = fclamp(pl.x, cam_x + 2, cam_x + 320 - PW - 2);
    pl.y = fclamp(pl.y, -PH, 146);
    if (pl.throw_cd > 0) pl.throw_cd--;
    if (btn(BTN_B) && pl.throw_cd == 0) {
        for (int k = -1; k <= 1; k += 2) {
            Ent *s = spawn(E_SPIRIT_SHOT, pl.x + PW, pl.y + PH / 2 - 3 + k * 4);
            if (s) { s->w = 6; s->h = 6; s->vx = 3.0f; s->t = 50; }
        }
        pl.throw_cd = SPIRIT_CD;
        sfx_play_name("rc_shoot");
    }
    pl.spirit_t--;
    /* back to life when the time is up, or at once with A */
    if (pl.spirit_t <= 0 || (btnp(BTN_A) && pl.y >= 0)) revive();
}

static void update_player(void) {
    if (pl.spirit) { update_spirit(); return; }
    float c = carry();
    int dir = btn(BTN_RIGHT) - btn(BTN_LEFT);
    if (dir) pl.face = dir;
    if (pl.drop_t > 0) pl.drop_t--;
    if (pl.throw_cd > 0) pl.throw_cd--;
    if (pl.throw_t > 0) pl.throw_t--;
    /* horizontal, against the town: with no input the scroll carries her
     * (she keeps her place on the screen), left holds her ground, right runs */
    pl.still = dir < 0;
    move_x(dir > 0 ? c + RUN : dir < 0 ? 0.0f : c);
    /* pushed by the screen's left edge; squeezed against a wall, you're done */
    if (pl.x < cam_x) {
        if (box_solid(cam_x, pl.y, PW, PH)) { last_killer = 3000; lose_life(); return; }
        pl.x = cam_x;
    }
    if (pl.x > cam_x + 320 - PW) pl.x = cam_x + 320 - PW;
    /* jumping: hold A to go higher; one more jump in the air, also after
     * walking off an edge; down + A drops through a ledge */
    if (btnp(BTN_A)) {
        if (pl.ground && btn(BTN_DOWN) && pl.leg < 0 && on_ledge()) {
            pl.drop_t = 10;
            pl.ground = false;
            pl.y += 1;
        } else if (pl.ground) {
            pl.vy = JUMP1;
            pl.jumps = 1;
            pl.ground = false;
            pl.leg = -1;
            sfx_play_name("rc_jump");
        } else if (pl.jumps < 2) {
            pl.vy = JUMP2;
            pl.jumps = 2;
            sfx_play_name("rc_jump2");
        }
    }
    if (!btn(BTN_A) && pl.vy < -1.2f) pl.vy = -1.2f; /* let go early: a lower jump */
    /* throwing: hold B for a steady stream, limited by stars on screen */
    if (btn(BTN_B) && pl.throw_cd == 0 && count_type(E_STAR) < max_stars()) throw_star();
    /* gravity */
    if (pl.ground && standing() && pl.vy >= 0) {
        pl.vy = 0;
        pl.jumps = 0;
        if (pl.leg >= 0) pl.y = ents[pl.leg].y - PH;
    } else {
        pl.ground = false;
        pl.leg = -1;
        pl.vy += GRAV;
        if (pl.vy > 4) pl.vy = 4;
        move_y(pl.vy);
        if (pl.ground) { pl.vy = 0; pl.jumps = 0; }
    }
    pl.anim += dir > 0 ? 2 : dir < 0 ? 0 : 1;
    eat_snacks();
    /* hazards */
    int x0 = ((int)pl.x + 2) >> 4, x1 = ((int)pl.x + PW - 3) >> 4;
    for (int tx = x0; tx <= x1; tx++) {
        char t = tile_at(tx, ((int)pl.y + PH - 2) >> 4);
        if (t == '^' || t == '~') { last_killer = 1000 + t; lose_life(); return; }
    }
    if (pl.y > 164) { last_killer = 2000; lose_life(); }
}

/* ------------------------------------------------------------------ */
/* foes                                                                 */

static void drop_loot(const Ent *e) {
    kills_run++;
    if (e->value > 0) {
        Ent *f = spawn(E_TOKEN, e->x + e->w / 2 - 5, e->y);
        if (f) { f->w = 10; f->h = 6; f->vy = -2.4f; f->vx = 0.4f; f->value = e->value; f->state = 1; }
    } else {
        /* spike lamps and firecrackers pay out on the spot */
        add_score(200);
        float_text(e->x, e->y, 200);
    }
    /* every third foe leaves catnip, while she holds fewer than two */
    if (power < 2 && ++kills_since_power >= 3) {
        kills_since_power = 0;
        Ent *p = spawn(E_CATNIP, e->x + e->w / 2 - 4, e->y - 4);
        if (p) { p->w = 8; p->h = 8; p->vy = -2.0f; p->state = 1; }
    }
}

static void kill_ent(Ent *ep) {
    Ent dead = *ep; /* the slot is recycled by the effects spawned below */
    ep->alive = false;
    puff(dead.x + dead.w / 2, dead.y + dead.h / 2, C_WHITE);
    sfx_play_name("rc_hit");
    if (dead.type == E_BUBBLE) return; /* a popped bubble drops nothing */
    drop_loot(&dead);
}

static bool hittable(const Ent *e) {
    if (IS_FOE(e->type)) {
        if (e->type == E_CROW) return e->sub == 1;                          /* only when popped up */
        if (e->type == E_FFISH || e->type == E_PUFFER) return e->sub != 0;  /* not under water */
        if (e->type == E_JAR && e->mode == M_WATER) return e->sub != 0;
        if (e->type == E_MAGPIE) return e->sub != 0;                        /* not while appearing */
        if (e->type == E_FLASHER) return e->sub != 0;                       /* only while phased in */
        if (e->type == E_SHEET && e->mode == M_CHIMNEY) return e->sub != 0 || e->t > 12;
        return true;
    }
    if (e->type == E_LANTERN) return e->state != -1;
    if (e->type == E_BUBBLE) return e->mode == M_POP; /* pufferfish bubbles can be popped */
    return false;
}

static void hurt(Ent *e, int dmg) {
    if (e->type == E_LANTERN) {
        e->alive = false;
        lives++;
        sfx_play_name("rc_oneup");
        Ent *h = spawn(E_TEXT, e->x, e->y);
        if (h) { h->value = -1; h->t = 50; }
        return;
    }
    e->hp -= dmg;
    e->flash = 6;
    if (e->hp <= 0) kill_ent(e);
    else sfx_play_name("rc_bosshit");
}

static Ent *enemy_shot(int type, float x, float y, float vx, float vy) {
    Ent *s = spawn(type, x, y);
    if (!s) return NULL;
    s->vx = vx;
    s->vy = vy;
    s->w = type == E_BUBBLE ? 6 : 5;
    s->h = type == E_BUBBLE ? 6 : 5;
    s->t = 0;
    return s;
}

/* aim a shot at where Pepper is now */
static void aimed_shot(int type, float x, float y, float speed, int mode) {
    float dx = pcx() - x, dy = pcy() - y, d = sqrtf(dx * dx + dy * dy) + 0.1f;
    Ent *s = enemy_shot(type, x, y, dx / d * speed, dy / d * speed);
    if (s) { s->mode = mode; s->hp = 1; }
}

static bool on_floor(const Ent *e, float x) {
    int foot = (int)(e->y + e->h);
    return floor_at((int)x >> 4, foot >> 4);
}

/* walk along a platform, turning at edges and walls */
static void patrol(Ent *e, float speed) {
    float nx = e->x + speed * e->dir;
    float ahead = e->dir > 0 ? nx + e->w : nx - 1;
    if (box_solid(nx, e->y, e->w, e->h) || !on_floor(e, ahead)) e->dir = -e->dir;
    else e->x = nx;
}

/* the same along the underside of a ledge */
static void patrol_ceiling(Ent *e, float speed) {
    float nx = e->x + speed * e->dir;
    float ahead = e->dir > 0 ? nx + e->w : nx - 1;
    if (box_solid(nx, e->y, e->w, e->h) || !solid_at((int)ahead >> 4, ((int)e->y - 1) >> 4)) e->dir = -e->dir;
    else e->x = nx;
}

static void fall_with_gravity(Ent *e, float g) {
    e->vy += g;
    if (e->vy > 4) e->vy = 4;
    float ny = e->y + e->vy;
    if (e->vy > 0) {
        int ty = (int)(ny + e->h) >> 4;
        bool land = false;
        for (int tx = (int)e->x >> 4; tx <= ((int)e->x + e->w - 1) >> 4; tx++)
            land |= solid_at(tx, ty) || (oneway_at(tx, ty) && e->y + e->h <= ty * 16 + 1);
        if (land && (int)(e->y + e->h) <= ty * 16 + 2) { e->y = (float)(ty * 16 - e->h); e->vy = 0; return; }
    }
    e->y = ny;
}

static bool grounded(const Ent *e) {
    int ty = (int)(e->y + e->h + 1) >> 4;
    for (int tx = (int)e->x >> 4; tx <= ((int)e->x + e->w - 1) >> 4; tx++)
        if (floor_at(tx, ty)) return true;
    return false;
}

/* flying fish and pufferfish: a fixed arc out of the water with a pause at the top */
static void fish_leap(Ent *e) {
    switch (e->sub) {
    case 0: /* under the water */
        if (--e->cd <= 0) { e->sub = 1; e->vy = -5.0f; e->vx = -0.6f; e->y = e->hy; sfx_play_name("rc_splash"); }
        break;
    case 1: /* rising */
        e->vy += 0.14f;
        e->x += e->vx;
        e->y += e->vy;
        if (e->vy >= 0) { e->sub = 2; e->k = 0; }
        break;
    case 2: /* hanging at the top of the arc */
        e->k++;
        if (e->type == E_PUFFER && e->mode != M_BOSSFISH && e->k % 8 == 4 && e->n < 3) {
            /* up to three bubbles, each aimed where Pepper is when it leaves */
            aimed_shot(E_BUBBLE, e->x + 3, e->y + 2, 1.3f, M_POP);
            e->n++;
            sfx_play_name("rc_shoot");
        }
        if (e->k >= 26) { e->sub = 3; e->vy = 0; }
        break;
    default: /* falling back */
        e->vy += 0.14f;
        e->x += e->vx;
        e->y += e->vy;
        if (e->y > 176) e->alive = false;
        break;
    }
}

static void update_foe(Ent *e) {
    switch (e->type) {
    case E_PIGEON: case E_RPIGEON: {
        /* waddles to and fro; now and then tucks in and rolls */
        float walk = e->type == E_RPIGEON ? 1.3f : 0.5f;
        if (e->sub > 0) { e->sub--; patrol(e, walk * 3.0f); }
        else {
            patrol(e, walk);
            if (rng_range(&rng, 0, 179) == 0) e->sub = 40;
        }
        break;
    }
    case E_CROW:
        /* pops out of its chimney, turns to face her, fires two evenly spaced
         * pebbles shortly before it ducks back down */
        e->dir = pcx() < e->x + e->w / 2 ? -1 : 1;
        if (--e->cd <= 0) { e->sub ^= 1; e->cd = e->sub ? 100 : 80; }
        if (e->sub == 1 && (e->cd == 40 || e->cd == 24)) {
            enemy_shot(E_PEBBLE, e->x + e->w / 2 - 2, e->y + 4, 1.8f * e->dir, 0);
            sfx_play_name("rc_shoot");
        }
        break;
    case E_GECKO:
        /* spits a seed that bounces along the rooftops; after dark, a second
         * one right after the first */
        e->dir = pcx() < e->x ? -1 : 1;
        if (--e->cd <= 0 && fabsf(pcx() - e->x) < 160) {
            e->cd = 120;
            enemy_shot(E_SEED, e->x + (e->dir > 0 ? e->w : -4), e->y, 1.2f * e->dir, -2.0f);
            if (loop >= 2) e->k = 14;
            sfx_play_name("rc_shoot");
        }
        if (e->k > 0 && --e->k == 0) enemy_shot(E_SEED, e->x + (e->dir > 0 ? e->w : -4), e->y, 1.2f * e->dir, -2.0f);
        break;
    case E_GULL:
        /* floats slowly in from the right edge, smoothly up and down */
        e->x -= 0.25f;
        e->y = e->hy + sinf(e->t * 0.05f) * 8;
        break;
    case E_LAMP:
        /* the spikes orbit (see lamp_spike); some lamps also swing up and down */
        if (e->mode == M_MOVING) e->y = e->hy + sinf(e->t * 0.03f) * 26;
        break;
    case E_FFISH: case E_PUFFER: fish_leap(e); break;
    case E_WASP:
        /* in from the left edge, heading right; while behind her it eases to her height */
        e->x += SCROLL + 1.3f;
        if (e->x + e->w / 2 < pcx()) e->y += fclamp((pcy() - (e->y + 3)) * 0.04f, -0.8f, 0.8f);
        if (e->x > cam_x + 340) e->alive = false;
        break;
    case E_SNAIL:
        if (e->mode == M_CEILING) patrol_ceiling(e, 0.25f);
        else patrol(e, 0.25f);
        break;
    case E_TOAD:
        /* sits, now and then jumps (maybe onto another platform), lobs stones */
        if (grounded(e) && e->vy >= 0) {
            e->vx = 0;
            if (--e->cd <= 0) {
                e->cd = 90 + rng_range(&rng, 0, 40);
                e->dir = pcx() < e->x ? -1 : 1;
                if (rng_chance(&rng, 35)) { e->vy = -3.8f; e->vx = 0.8f * e->dir; }
                else {
                    float dx = pcx() - e->x;
                    enemy_shot(E_PEBBLE, e->x + 5, e->y, fclamp(dx / 50.0f, -2.4f, 2.4f) + SCROLL, -3.0f);
                    sfx_play_name("rc_shoot");
                }
            }
        }
        if (e->vx != 0 && !box_solid(e->x + e->vx, e->y, e->w, e->h)) e->x += e->vx;
        fall_with_gravity(e, 0.18f);
        if (e->y > 180) e->alive = false;
        break;
    case E_PELICAN:
        /* crosses fast from right to left; drops a bomb when she is below */
        e->x -= 2.2f;
        if (e->sub == 0 && fabsf(pcx() - (e->x + e->w / 2)) < 10 && pcy() > e->y) {
            e->sub = 1;
            enemy_shot(E_BOMB, e->x + 4, e->y + e->h, 0, 0.5f);
        }
        break;
    case E_JAR:
        if (e->mode == M_DIVE) {
            /* straight down through everything and gone */
            e->vy = fminf(e->vy + 0.3f, 5.0f);
            e->y += e->vy;
            if (e->y > 180) e->alive = false;
            break;
        }
        if (e->mode == M_WATER && e->sub == 0) {
            /* out of the water like a fish, then onto the planks */
            if (--e->cd <= 0) { e->sub = 1; e->vy = -5.4f; e->vx = -0.4f; sfx_play_name("rc_splash"); }
            break;
        }
        /* dives on her if she is ever right underneath */
        if (!pl.spirit && fabsf(pcx() - (e->x + e->w / 2)) < 7 && pl.y > e->y + e->h) {
            e->mode = M_DIVE;
            e->vy = 1.0f;
            break;
        }
        /* small hops toward her (away to the left while she is a spirit);
         * after sixteen it turns into a crown */
        if (grounded(e) && e->vy >= 0) {
            e->vx = 0;
            if (--e->cd <= 0) {
                e->cd = 36;
                if (++e->n > 16) {
                    Ent *c = spawn(E_CROWN, e->x, e->y + 3);
                    if (c) { c->w = 9; c->h = 7; c->state = 0; c->value = 300; }
                    puff(e->x + 5, e->y + 5, C_YELLOW);
                    sfx_play_name("rc_power");
                    e->alive = false;
                    return;
                }
                e->dir = pl.spirit ? -1 : pcx() < e->x ? -1 : 1;
                e->vy = -2.3f;
                e->vx = 0.8f * e->dir;
            }
        }
        if (e->vx != 0 && !box_solid(e->x + e->vx, e->y, e->w, e->h)) e->x += e->vx;
        fall_with_gravity(e, e->mode == M_WATER ? 0.14f : 0.18f);
        if (e->y > 176) e->alive = false;
        break;
    case E_CRACKER: {
        /* lights a short fuse when she comes close, then bursts into a
         * choking cloud; after dark they go off sooner, from farther away */
        bool lively = e->mode == M_LIVELY;
        float reach = lively ? 96.0f : 64.0f;
        if (e->sub == 0 && !pl.spirit && fabsf(pcx() - (e->x + 5)) < reach) { e->sub = 1; e->t = 0; sfx_play_name("rc_fuse"); }
        if (e->sub == 1 && e->t >= (lively ? 40 : 60)) {
            Ent *b = spawn(E_CLOUD, e->x + 5 - 20, e->y + 6 - 18);
            if (b) { b->w = 40; b->h = 34; b->t = 100; }
            sfx_play_name("rc_boom");
            shake = 8;
            e->alive = false;
        }
        break;
    }
    case E_SPIDER:
        /* hangs under a ledge, going up and down at random */
        if (e->t % 40 == 0) e->vy = (float)rng_range(&rng, -1, 1) * 0.8f;
        e->y += e->vy;
        if (e->y < e->hy) { e->y = e->hy; e->vy = 0.8f; }
        if (e->y > e->hy + 52) { e->y = e->hy + 52; e->vy = -0.8f; }
        break;
    case E_MAGPIE:
        /* appears out of nowhere, then flies straight at where she was; at
         * the edge of the screen it bursts into two shards */
        if (e->sub == 0) {
            if (++e->k >= 24) {
                e->sub = 1;
                float dx = pcx() - (e->x + 6), dy = pcy() - (e->y + 4), d = sqrtf(dx * dx + dy * dy) + 0.1f;
                e->vx = dx / d * 2.2f;
                e->vy = dy / d * 2.2f;
            }
        } else {
            e->x += e->vx;
            e->y += e->vy;
            if (e->x < cam_x || e->x + e->w > cam_x + 320 || e->y < 0 || e->y + e->h > 160) {
                float bx = -isign((int)(e->vx * 100)) * 1.4f;
                for (int k = -1; k <= 1; k += 2) enemy_shot(E_SHRAP, e->x + 4, e->y + 2, bx, 1.4f * (float)k);
                e->alive = false;
            }
        }
        break;
    case E_FLASHER:
        /* phases in, fires one fast beam across the whole screen toward her
         * side, phases out, and turns up again somewhere near */
        switch (e->sub) {
        case 0:
            if (--e->cd <= 0) { e->sub = 1; e->k = 0; }
            break;
        case 1: /* phasing in */
            e->dir = pcx() < e->x ? -1 : 1;
            if (++e->k >= 36) {
                Ent *b = spawn(E_BEAM, e->x + (e->dir > 0 ? e->w : -24), e->y + 5);
                if (b) { b->w = 24; b->h = 3; b->vx = 8.0f * e->dir; b->t = 60; }
                sfx_play_name("rc_beam");
                e->sub = 2;
                e->k = 0;
            }
            break;
        case 2: /* lingers after the shot */
            if (++e->k >= 40) { e->sub = 3; e->k = 0; }
            break;
        default: /* phasing out, then somewhere else */
            if (++e->k >= 16) {
                e->sub = 0;
                e->cd = 50;
                for (int tries = 0; tries < 6; tries++) {
                    float nx = e->hx + (float)rng_range(&rng, -64, 64);
                    if (!box_solid(nx, e->y, e->w, e->h) && on_floor(e, nx + e->w / 2)) { e->x = nx; break; }
                }
            }
            break;
        }
        break;
    case E_SHEET:
        if (pl.spirit && e->sub != 0) {
            /* while she is a spirit the laundry blows away upward */
            e->y -= 1.4f;
            if (e->y < -24) e->alive = false;
            break;
        }
        if (e->sub == 0) {
            /* rises out of its chimney to her height */
            float dy = pcy() - (e->y + 6);
            e->y += fclamp(dy, -1.2f, 1.2f);
            if (fabsf(dy) < 2 || e->t > 150) { e->sub = 1; e->dir = pcx() < e->x ? -1 : 1; e->hy = e->y; }
        } else {
            /* then sails at her, bobbing */
            e->x += 1.3f * e->dir;
            e->y = e->hy + sinf(e->t * 0.1f) * 6;
            if (e->x < cam_x - 30 || e->x > cam_x + 350) e->alive = false;
        }
        break;
    case E_MOTH:
        /* flies about its spot, shedding clouds of spores */
        e->x = e->hx + sinf(e->t * 0.03f) * 26;
        e->y = e->hy - 10 + sinf(e->t * 0.07f) * 14;
        if (e->t % 90 == 45) {
            Ent *d = spawn(E_DUST, e->x - 3, e->y + 4);
            if (d) { d->w = 16; d->h = 14; d->t = 70; }
        }
        break;
    }
}

/* the spike lamp's orbiting spikes */
static void lamp_spike(const Ent *e, int k, float *sx, float *sy) {
    float a = e->t * 0.045f + (float)k * 6.283f / (float)e->n;
    *sx = e->x + e->w / 2 + cosf(a) * 22 - 3;
    *sy = e->y + e->h / 2 + sinf(a) * 22 - 3;
}

/* ---- the boss: Old Crab ---- */

#define EYE_X (arena_x + 258)
#define EYE_Y 64

static const float LEG_X[4] = {60, 108, 156, 204};

static void spawn_boss(void) {
    Ent *b = spawn(E_BOSS, arena_x + 232, 84);
    if (!b) return;
    boss_i = (int)(b - ents);
    b->w = 80; b->h = 60;
    b->hp = BOSS_HP;
    b->t = 0;
    for (int i = 0; i < 4; i++) {
        Ent *l = spawn(E_LEG, arena_x + LEG_X[i], 160);
        if (l) { l->w = 28; l->h = 8; l->n = i; }
    }
    music_play(RC_MUS_BOSS);
}

static void boss_hit(Ent *b) {
    if (b->flash > 0 || boss_dead) return;
    b->hp--;
    b->flash = 5;
    sfx_play_name("rc_bosshit");
    if (b->hp <= 0) {
        boss_dead = true;
        b->state = 99;
        b->t = 0;
        music_stop();
    }
}

static void update_legs(void) {
    for (int i = 0; i < MAX_ENTS; i++) {
        Ent *l = &ents[i];
        if (!l->alive || l->type != E_LEG) continue;
        float oy = l->y;
        /* each leg rises, stands, sinks and rests; the four take turns */
        int ph = (frame_t + l->n * 95) % 380;
        float top = 100, low = 150;
        if (boss_dead) l->y = fminf(l->y + 1.0f, 170);
        else if (ph < 30) l->y = low + (top - low) * (float)ph / 30.0f;
        else if (ph < 190) l->y = top;
        else if (ph < 220) l->y = top + (low - top) * (float)(ph - 190) / 30.0f;
        else l->y = 170;
        if (pl.leg == i && !pl.spirit) pl.y += l->y - oy;
    }
}

static void update_boss(Ent *b) {
    if (b->flash > 0) b->flash--;
    if (b->state == 99) {
        if (b->t % 6 == 0) { puff(b->x + rng_range(&rng, 0, b->w), b->y + rng_range(&rng, 0, b->h), C_YELLOW); sfx_play_name("rc_boom"); shake = 6; }
        if (b->t >= 180) b->alive = false;
        return;
    }
    int burst = loop >= 2 ? 90 : 120;
    if (b->t % burst == burst - 1) {
        /* a burst of shots from the mouth, fanned at her */
        float mx = arena_x + 248, my = 104;
        float dx = pcx() - mx, dy = pcy() - my, base = atan2f(dy, dx);
        for (int k = -2; k <= 2; k++) {
            float a = base + k * 0.22f;
            enemy_shot(E_BUBBLE, mx, my, cosf(a) * 1.4f, sinf(a) * 1.4f);
        }
        sfx_play_name("rc_shoot");
    }
    if (b->t % 210 == 100) {
        /* an orb drifts down out of the sky */
        Ent *o = spawn(E_ORB, arena_x + (float)rng_range(&rng, 70, 230), -8);
        if (o) { o->w = 8; o->h = 8; o->vy = 0.6f; }
    }
    if (b->t % 170 == 60) {
        /* a yellow flying fish leaps between the legs */
        Ent *f = spawn(E_FFISH, arena_x + (float)rng_range(&rng, 80, 220), 150);
        if (f) {
            f->w = 10; f->h = 8; f->hp = 1; f->value = 200; f->mode = M_BOSSFISH;
            f->sub = 1; f->vy = -5.0f; f->vx = rng_chance(&rng, 50) ? 0.6f : -0.6f; f->state = 0; f->hy = 150;
        }
    }
}

/* ------------------------------------------------------------------ */
/* entity update                                                        */

/* dormant things wake when they scroll into view (some wait for more) */
static bool try_wake(Ent *e) {
    if (e->x >= cam_x + 330) return false;
    switch (e->type) {
    case E_FFISH: case E_PUFFER:
        /* its spot on screen and Pepper within reach */
        if (e->x > cam_x + 312 || fabsf(pcx() - (e->x + e->w / 2)) > FISH_RANGE) return false;
        break;
    case E_JAR:
        if (count_awake(E_JAR) >= MAX_JARS) return false;
        if (e->mode == M_WATER && (e->x > cam_x + 312 || fabsf(pcx() - (e->x + e->w / 2)) > FISH_RANGE)) return false;
        break;
    case E_WASP: e->x = cam_x - 10; break; /* comes in from the left edge */
    case E_GULL: case E_PELICAN: e->x = cam_x + 330; break;
    case E_MAGPIE: case E_FLASHER: case E_SHEET:
        if (e->x > cam_x + 296) return false; /* appear well inside the screen */
        break;
    }
    e->state = 0;
    return true;
}

static void update_ents(void) {
    for (int i = 0; i < MAX_ENTS; i++) {
        Ent *e = &ents[i];
        if (!e->alive) continue;
        if (e->flash > 0 && e->type != E_BOSS) e->flash--;
        /* things far behind the camera are gone */
        if (e->x + e->w < cam_x - 40 && e->type != E_BOSS && e->type != E_LEG) { e->alive = false; continue; }
        if (e->state == -1 && !try_wake(e)) continue;
        /* foes and the boss count their age in t; shots and effects count down */
        if (IS_FOE(e->type)) { e->t++; update_foe(e); continue; }
        switch (e->type) {
        case E_BOSS: e->t++; update_boss(e); break;
        case E_STAR: case E_SPIRIT_SHOT: {
            e->x += e->vx + carry();
            if (--e->t <= 0 || (e->type == E_STAR && box_solid(e->x, e->y, e->w, e->h))) { e->alive = false; break; }
            if (boss_i >= 0 && ents[boss_i].alive && ents[boss_i].state != 99 &&
                rects_overlap((int)e->x, (int)e->y, e->w, e->h, (int)EYE_X - 6, EYE_Y - 6, 12, 12)) {
                boss_hit(&ents[boss_i]);
                e->alive = false;
                break;
            }
            for (int j = 0; j < MAX_ENTS; j++) {
                Ent *o = &ents[j];
                if (!o->alive || o->state == -1 || !hittable(o)) continue;
                if (!rects_overlap((int)e->x, (int)e->y, e->w, e->h, (int)o->x, (int)o->y, o->w, o->h)) continue;
                e->alive = false;
                /* a rolling pigeon only takes a hit from behind */
                if ((o->type == E_PIGEON || o->type == E_RPIGEON) && o->sub > 0 && (e->vx > 0) != (o->dir > 0)) {
                    sfx_play_name("rc_bosshit");
                    puff(e->x + 4, e->y + 4, C_GREY);
                    break;
                }
                hurt(o, 1);
                break;
            }
            break;
        }
        case E_PEBBLE: case E_SHRAP:
            if (e->type == E_PEBBLE && e->vy != 0) e->vy += 0.13f;
            e->x += e->vx;
            e->y += e->vy;
            if (e->y > 175 || e->y < -20 || box_solid(e->x, e->y, e->w, e->h)) e->alive = false;
            break;
        case E_SEED: {
            /* bounces along the roofs */
            e->vy += 0.16f;
            e->x += e->vx;
            float ny = e->y + e->vy;
            if (e->vy > 0 && box_solid(e->x, ny, e->w, e->h)) e->vy = -2.4f;
            else e->y = ny;
            if (box_solid(e->x, e->y, e->w, e->h)) e->alive = false;
            if (e->y > 175) e->alive = false;
            break;
        }
        case E_BUBBLE:
            e->t++;
            e->x += e->vx;
            e->y += e->vy;
            if (e->y < -10 || e->y > 176 || e->t > 420 || e->x < cam_x - 20 || e->x > cam_x + 340) e->alive = false;
            break;
        case E_BOMB:
            e->vy += 0.15f;
            e->y += e->vy;
            if (box_solid(e->x, e->y + e->h, e->w, 1) || e->y > 160) {
                /* bursts into eight shards */
                e->alive = false;
                for (int k = 0; k < 8; k++) {
                    float a = (float)k * 6.283f / 8.0f;
                    enemy_shot(E_SHRAP, e->x + 2, e->y + 2, cosf(a) * 1.8f, sinf(a) * 1.8f);
                    bomb_shards++;
                }
                sfx_play_name("rc_boom");
                shake = 4;
            }
            break;
        case E_BEAM:
            e->x += e->vx;
            if (--e->t <= 0 || e->x > cam_x + 340 || e->x + e->w < cam_x - 20) e->alive = false;
            break;
        case E_DUST:
            e->x += carry() * 0.5f;
            e->y -= 0.1f;
            if (--e->t <= 0) e->alive = false;
            break;
        case E_ORB: {
            /* falls slowly until it hits a platform, then splits in two */
            e->y += e->vy;
            int ty = (int)(e->y + e->h) >> 4;
            bool hit = floor_at(((int)e->x + 4) >> 4, ty) && (int)(e->y + e->h) - ty * 16 < 3;
            for (int j = 0; j < MAX_ENTS && !hit; j++) {
                Ent *l = &ents[j];
                if (l->alive && l->type == E_LEG && l->y < 150 && e->x + 8 > l->x && e->x < l->x + l->w && e->y + e->h >= l->y && e->y + e->h < l->y + 4) hit = true;
            }
            if (hit) {
                e->alive = false;
                for (int k = -1; k <= 1; k += 2) enemy_shot(E_ORBHALF, e->x + 2, e->y, 1.0f * k, 1.0f);
                sfx_play_name("rc_shoot");
            } else if (e->y > 150) e->alive = false; /* into the water */
            break;
        }
        case E_ORBHALF:
            /* down-left and down-right */
            e->x += e->vx;
            e->y += e->vy;
            if (e->y > 176 || e->x < cam_x - 10 || e->x > cam_x + 330) e->alive = false;
            break;
        case E_CLOUD:
            e->x += carry() * 0.25f;
            if (--e->t <= 0) e->alive = false;
            break;
        case E_TOKEN: case E_CATNIP: case E_LETTER: case E_CROWN:
            if (e->state == 1) {
                e->vy += 0.15f;
                e->x += e->vx * 0.3f;
                float ny = e->y + e->vy;
                int ty = (int)(ny + e->h) >> 4;
                bool land = e->vy > 0 && floor_at(((int)e->x + 4) >> 4, ty);
                if (land) { e->y = (float)(ty * 16 - e->h); e->vy = 0; e->state = 2; }
                else e->y = ny;
                if (e->y > 180) e->alive = false;
            }
            break;
        case E_LANTERN:
            e->t++;
            e->y -= 0.45f;
            e->x += carry() + sinf(e->t * 0.05f) * 0.3f;
            if (e->y < -20) e->alive = false;
            break;
        case E_TEXT:
            e->y -= 0.5f;
            if (--e->t <= 0) e->alive = false;
            break;
        case E_DEBRIS:
            e->vy += 0.1f;
            e->x += e->vx;
            e->y += e->vy;
            if (--e->t <= 0) e->alive = false;
            break;
        }
    }
}

static void got_letter(Ent *e) {
    e->alive = false;
    add_score(500);
    letters_this_run++;
    float_text(e->x, e->y - 4, 500);
    sfx_play_name("rc_letter");
    if (letters_this_run >= 3) game_award(GOAL_BEACON);
}

static bool harmless(int type) {
    switch (type) {
    case E_TOKEN: case E_CROWN: case E_LETTER: case E_SPOT: case E_CATNIP: case E_LANTERN: case E_TEXT:
    case E_DEBRIS: case E_STAR: case E_SPIRIT_SHOT: case E_LEG: return true;
    }
    return false;
}

static void killed_by(int k) {
    last_killer = k;
    lose_life();
}

static void contacts(void) {
    if (pl.dead_t || pl.spirit) return; /* the spirit can't touch or be touched */
    int px = (int)pl.x + 1, py = (int)pl.y + 2, pw = PW - 2, ph = PH - 3;
    for (int i = 0; i < MAX_ENTS; i++) {
        Ent *e = &ents[i];
        if (!e->alive || e->state == -1) continue;
        if (e->type == E_LAMP) {
            /* the lamp and its spikes */
            for (int k = 0; k < e->n; k++) {
                float sx, sy;
                lamp_spike(e, k, &sx, &sy);
                if (rects_overlap(px, py, pw, ph, (int)sx, (int)sy, 6, 6)) { last_killer = E_LAMP; lose_life(); return; }
            }
        }
        if (!rects_overlap(px, py, pw, ph, (int)e->x, (int)e->y, e->w, e->h)) continue;
        switch (e->type) {
        case E_TOKEN: case E_CROWN:
            e->alive = false;
            add_score(e->value);
            float_text(e->x, e->y - 4, e->value);
            sfx_play_name(e->type == E_TOKEN ? "rc_coin" : "rc_letter");
            break;
        case E_LETTER: got_letter(e); break;
        case E_SPOT:
            /* standing on the secret spot calls down a lost letter */
            if (pl.ground) {
                e->alive = false;
                /* it lands a little ahead, where the scroll carries her */
                Ent *l = spawn(E_LETTER, e->x + 23, e->y - 20);
                if (l) { l->w = 10; l->h = 8; l->state = 1; l->vy = -1.5f; }
                sfx_play_name("rc_secret");
            }
            break;
        case E_CATNIP:
            e->alive = false;
            if (power < 2) power++;
            sfx_play_name("rc_power");
            break;
        case E_CROW:
            if (e->sub == 1) killed_by(e->type);
            break;
        case E_FFISH: case E_PUFFER:
            if (e->sub != 0) killed_by(e->type);
            break;
        case E_JAR:
            if (e->mode != M_WATER || e->sub != 0) killed_by(e->type);
            break;
        case E_MAGPIE: case E_FLASHER:
            if (e->sub != 0) killed_by(e->type);
            break;
        case E_SHEET:
            if (hittable(e)) killed_by(e->type);
            break;
        case E_BOSS:
            if (e->state != 99) killed_by(e->type);
            break;
        default:
            if (!harmless(e->type)) killed_by(e->type);
            break;
        }
        if (pl.spirit) return;
    }
}

/* ------------------------------------------------------------------ */
/* flow                                                                 */

static void play_update(void) {
    frame_t++;
    carry_px = 0;
    if (!in_arena) {
        int before = cam_px();
        cam_x += SCROLL;
        if (cam_x >= arena_x) {
            cam_x = arena_x;
            in_arena = true;
            spawn_boss();
        }
        carry_px = cam_px() - before;
    }
    spawn_ahead();
    /* crossing into a new area changes the tune */
    int area = area_of_col((cam_px() + 160) >> 4);
    if (!in_arena && area != cur_area) {
        cur_area = area;
        music_play(RC_MUS_AREA[area]);
    }
    if (pl.dead_t > 0) {
        if (++pl.dead_t > 80) {
            state = S_OVER;
            state_t = 0;
            record_run();
            music_restart(RC_MUS_OVER);
            game_set_pausable(false);
        }
        update_ents();
        return;
    }
    update_legs();
    update_player();
    update_ents();
    contacts();
    {
        /* what the renderer will show: the player's screen x and the camera */
        int icam = cam_px(), sx = (int)floorf(pl.x) - icam;
        if (jit_valid) {
            int d = sx - jit_last_sx, dc = icam - jit_last_cam;
            if (d != 0 && jit_last_d != 0 && (d > 0) != (jit_last_d > 0)) jit_rev++;
            if (d != 0) jit_last_d = d;
            if (dc < 0 || dc > 1) jit_cam_bad++;
        }
        jit_valid = true;
        jit_last_sx = sx;
        jit_last_cam = icam;
    }
    if (boss_dead && boss_i >= 0 && !ents[boss_i].alive) {
        state = S_ENDING;
        state_t = 0;
        game_award(GOAL_SAUCER);
        if (loop >= 2) {
            game_award(GOAL_ALIEN);
            record_run(); /* the second loop is the end of the run */
        }
        music_restart(RC_MUS_END);
        game_set_pausable(false);
    }
}

static void title_update(void) {
    game_set_pausable(false);
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) {
        sfx_play_name("ui_ok");
        input_consume();
        new_run();
    }
}

static void rc_update(void) {
    if (shake > 0) shake--;
    state_t++;
    switch (state) {
    case S_TITLE: frame_t++; title_update(); break;
    case S_INTRO:
        frame_t++;
        if (state_t > 70 || (state_t > 20 && btnp(BTN_A))) { state = S_PLAY; state_t = 0; input_consume(); }
        break;
    case S_PLAY: play_update(); break;
    case S_OVER:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) to_title();
        break;
    case S_ENDING:
        frame_t++;
        if (state_t > 240 && (btnp(BTN_A) || btnp(BTN_START))) {
            if (loop == 1) {
                /* round again after dark, keeping score and lives */
                loop = 2;
                start_at(0);
            } else {
                to_title();
            }
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */
typedef struct Theme {
    uint8_t sky[4];
    uint8_t wall, wall_hi, wall_lo, trim, window, window_lo, door, ledge_a, ledge_b, far, far_hi, mid;
} Theme;
static const Theme THEMES[RC_AREAS] = {
    /* whitewash rooftops, midday */
    {{C_SKY, C_CYAN, C_CYAN, C_ICE}, C_WHITE, C_WHITE, C_LIGHT, C_SKY, C_BLUE, C_NAVY, C_BLUE, C_SKY, C_WHITE, C_LIGHT, C_SKY, C_CYAN},
    /* spice market, late afternoon */
    {{C_ORANGE, C_AMBER, C_AMBER, C_YELLOW}, C_EARTH, C_HIDE, C_TAN, C_RED, C_BROWN, C_MAROON, C_BROWN, C_RED, C_CREAM, C_TAN, C_ORANGE, C_AMBER},
    /* fort walls at dusk */
    {{C_PURPLE, C_MAGENTA, C_PINK, C_AMBER}, C_SLATE, C_GREY, C_DUSK, C_GREY, C_NIGHT, C_INK, C_BROWN, C_GREY, C_SLATE, C_WINE, C_MAGENTA, C_VIOLET},
    /* the harbour at night */
    {{C_INK, C_NIGHT, C_NAVY, C_DUSK}, C_TAN, C_EARTH, C_BROWN, C_EARTH, C_AMBER, C_BROWN, C_BROWN, C_EARTH, C_TAN, C_NIGHT, C_DUSK, C_NAVY},
};

static uint8_t night_map[PAL_COUNT];

static void build_night_map(void) {
    /* moonlight: bright day colours sink, darks and lamp colours stay */
    pal_identity(night_map);
    night_map[C_WHITE] = C_GREY; night_map[C_LIGHT] = C_SLATE; night_map[C_GREY] = C_SLATE;
    night_map[C_SLATE] = C_DUSK; night_map[C_ICE] = C_SKY; night_map[C_CYAN] = C_BLUE;
    night_map[C_SKY] = C_NAVY; night_map[C_HIDE] = C_EARTH; night_map[C_EARTH] = C_TAN;
    night_map[C_TAN] = C_BROWN; night_map[C_ORANGE] = C_RED; night_map[C_RED] = C_WINE;
    night_map[C_PINK] = C_MAGENTA; night_map[C_MAGENTA] = C_VIOLET; night_map[C_LIME] = C_LEAF;
    night_map[C_LEAF] = C_JADE;
}

static void sky_bands(const uint8_t *sky, int level) {
    for (int b = 0; b < 4; b++) gfx_dither(0, b * 40, 320, 40, sky[b], level);
    for (int b = 0; b < 3; b++) gfx_dither(0, b * 40 + 30, 320, 10, sky[b + 1], level >= 16 ? 8 : level / 2);
}

static void draw_sky(void) {
    int icam = cam_px();
    int col = (icam + 160) >> 4;
    int theme = area_of_col(col);
    const Theme *t = &THEMES[theme];
    sky_bands(t->sky, 16);
    /* the last screen of an area slowly takes on the next area's sky */
    int in_area = col % (RC_CHUNK_W * RC_AREA_CHUNKS);
    int blend_from = RC_CHUNK_W * (RC_AREA_CHUNKS - 1);
    if (theme < RC_AREAS - 1 && in_area >= blend_from) sky_bands(THEMES[theme + 1].sky, (in_area - blend_from) * 16 / RC_CHUNK_W);
    bool night = theme == 3;
    int par = icam / 20;
    int sx = 250 - par % 40, sy = 34;
    if (night) {
        gfx_dither_circle(sx, sy, 20, C_DUSK, 5);
        gfx_circ(sx, sy, 12, C_CREAM);
        gfx_circ(sx + 4, sy - 3, 10, t->sky[0]);
        for (int i = 0; i < 40; i++) {
            uint32_t h = (uint32_t)(i * 2654435761u);
            gfx_pset((int)(h % 320), (int)((h >> 12) % 90), (frame_t / 16 + i) % 9 ? C_LIGHT : C_WHITE);
        }
    } else if (theme == 0) { gfx_circ(sx, sy, 14, C_YELLOW); gfx_circ(sx, sy, 10, C_CREAM); }
    else if (theme == 1) { gfx_circ(sx, 50, 18, C_YELLOW); gfx_dither_circle(sx, 50, 24, C_CREAM, 4); }
    else { gfx_circ(160, 110, 26, C_ORANGE); gfx_circ(160, 110, 20, C_AMBER); gfx_circ(160, 110, 13, C_YELLOW); }
    /* far silhouettes (parallax 0.25) */
    int off = icam / 4;
    for (int i = -1; i < 12 && theme != 3; i++) {
        int base = i * 36 - off % 36;
        uint32_t h = (uint32_t)((i + off / 36) * 2654435761u);
        int hgt = 18 + (int)(h % 26);
        int x = base, gy = 104;
        gfx_rect(x, gy - hgt, 30, hgt, t->far);
        if ((h >> 8) % 3 == 0) gfx_circ(x + 15, gy - hgt, 9, t->far_hi);
        if ((h >> 10) % 4 == 0) gfx_rect(x + 12, gy - hgt - 18, 6, 18, t->far);
        for (int k = 0; k < 3; k++) gfx_rect(x + 6 + k * 8, gy - hgt + 8, 3, 4, t->far_hi);
    }
    /* below the horizon: the depth you fall into between buildings */
    if (theme == 0) {
        gfx_rect(0, 104, 320, 56, C_BLUE);
        for (int y = 106; y < 160; y += 4)
            for (int x = (y * 5 + frame_t / 6) % 16; x < 320; x += 16) gfx_hline(x, x + 4, y, C_SKY);
    } else if (theme == 1) {
        gfx_rect(0, 104, 320, 56, C_MAROON);
        gfx_dither(0, 104, 320, 56, C_INK, 6);
    } else if (theme == 2) {
        gfx_rect(0, 104, 320, 56, C_WINE);
        gfx_dither(0, 104, 320, 56, C_PURPLE, 6);
    } else {
        gfx_rect(0, 108, 320, 72, C_NAVY);
        for (int y = 110; y < 160; y += 3)
            for (int x = (y * 7 + frame_t / 4) % 12; x < 320; x += 12) gfx_hline(x, x + 3, y, (y / 3) % 2 ? C_BLUE : C_NIGHT);
        /* the lighthouse far out on the mole (out of sight in Old Crab's water) */
        int lx = in_arena ? -100 : 290 - off % 380;
        gfx_rect(lx, 60, 10, 48, C_LIGHT);
        gfx_rect(lx, 70, 10, 5, C_RED);
        gfx_rect(lx, 86, 10, 5, C_RED);
        gfx_rect(lx - 2, 54, 14, 6, C_INK);
        if ((frame_t / 20) % 2) gfx_dither(lx + 10, 52, 60, 8, C_YELLOW, 6);
    }
    /* clouds / bunting (parallax 0.5) */
    int off2 = icam / 2;
    for (int i = 0; i < 6; i++) {
        int x = (i * 97 - off2) % 420;
        if (x < -60) x += 420;
        if (theme == 0) {
            gfx_circ(x, 20 + (i % 3) * 14, 8, C_WHITE);
            gfx_circ(x + 10, 17 + (i % 3) * 14, 10, C_WHITE);
            gfx_circ(x + 22, 21 + (i % 3) * 14, 7, C_WHITE);
        } else if (theme == 1) {
            gfx_line(x, 12, x + 60, 22, C_BROWN);
            for (int k = 0; k < 4; k++) {
                int lx = x + 8 + k * 14, ly = 14 + k * 2 + 3;
                gfx_rect(lx, ly, 4, 6, k % 2 ? C_RED : C_YELLOW);
            }
        } else if (theme == 2) {
            gfx_hline(x, x + 30, 18 + (i % 3) * 12, C_PINK);
            gfx_hline(x + 6, x + 40, 19 + (i % 3) * 12, C_MAGENTA);
        }
    }
}

static void draw_tile(int tx, int ty, char c, int x, int y) {
    int theme = area_of_col(tx);
    const Theme *t = &THEMES[theme];
    bool air_above = !solid_at(tx, ty - 1) && tile_at(tx, ty - 1) != '=';
    switch (c) {
    case '#': case 'w': case 'd': case 'C': case 'B': {
        gfx_rect(x, y, 16, 16, t->wall);
        if (theme == 2 || c == 'B') {
            for (int r = 0; r < 4; r++) {
                gfx_hline(x, x + 15, y + r * 4 + 3, t->wall_lo);
                gfx_vline(x + ((r & 1) ? 4 : 12), y + r * 4, y + r * 4 + 3, t->wall_lo);
            }
        } else if (theme == 3) {
            for (int r = 0; r < 4; r++) gfx_hline(x, x + 15, y + r * 4 + 3, t->wall_lo);
            gfx_vline(x + 7, y, y + 15, t->wall_lo);
        } else {
            gfx_dither(x, y, 16, 16, t->wall_lo, 1);
        }
        if (c == 'w') {
            gfx_rect(x + 4, y + 3, 8, 9, t->window_lo);
            gfx_rect(x + 5, y + 4, 6, 7, t->window);
            gfx_vline(x + 8, y + 4, y + 10, t->window_lo);
            gfx_hline(x + 3, x + 12, y + 12, t->wall_lo);
        } else if (c == 'd') {
            gfx_rect(x + 4, y + 4, 8, 12, t->door);
            gfx_circ(x + 8, y + 5, 4, t->door);
            gfx_pset(x + 10, y + 10, C_YELLOW);
        } else if (c == 'C') {
            gfx_rect(x + 2, y, 12, 16, t->wall_lo);
            gfx_rect(x + 3, y + 1, 10, 15, t->wall);
            if (!solid_at(tx, ty - 1)) gfx_rect(x + 1, y, 14, 3, t->trim);
        }
        if (air_above && c != 'C') {
            gfx_hline(x, x + 15, y, C_INK);
            gfx_rect(x, y + 1, 16, 2, t->trim);
            gfx_hline(x, x + 15, y + 3, t->wall_lo);
        }
        if (!solid_at(tx - 1, ty)) gfx_vline(x, y, y + 15, t->wall_lo);
        if (!solid_at(tx + 1, ty)) gfx_vline(x + 15, y, y + 15, t->wall_lo);
        break;
    }
    case 'O': {
        /* domes span two tiles */
        bool right_half = tile_at(tx - 1, ty) == 'O';
        int cx = right_half ? x : x + 16;
        for (int yy = 0; yy < 16; yy++) {
            int w = (int)sqrtf((float)(256 - (16 - yy) * (16 - yy)));
            int x0 = cx - w, x1 = cx + w - 1;
            if (x0 < x) x0 = x;
            if (x1 > x + 15) x1 = x + 15;
            if (x0 <= x1) gfx_hline(x0, x1, y + yy, theme == 1 ? C_TAN : C_BLUE);
            if (yy < 8 && x0 <= x1 && !right_half) gfx_pset(x0 + 2, y + yy, theme == 1 ? C_EARTH : C_SKY);
        }
        break;
    }
    case 'M':
        gfx_rect(x + 2, y + 4, 12, 12, t->wall);
        gfx_rect(x + 2, y + 4, 12, 2, t->trim);
        gfx_vline(x + 2, y + 4, y + 15, C_INK);
        gfx_vline(x + 13, y + 4, y + 15, t->wall_lo);
        break;
    case 'H':
        gfx_rect(x, y, 16, 16, C_WHITE);
        gfx_rect(x, y + 4, 16, 3, C_RED);
        if (!solid_at(tx, ty - 1)) gfx_rect(x, y, 16, 2, C_BROWN);
        if (!solid_at(tx, ty + 1)) gfx_rect(x + 2, y + 12, 12, 4, C_NAVY);
        break;
    case '=': {
        for (int k = 0; k < 16; k += 4) gfx_rect(x + k, y, 4, 5, (k / 4 + tx) % 2 ? t->ledge_a : t->ledge_b);
        gfx_hline(x, x + 15, y + 5, C_INK);
        gfx_pset(x + 3, y + 6, t->ledge_a);
        gfx_pset(x + 11, y + 6, t->ledge_a);
        break;
    }
    case '^':
        for (int k = 0; k < 4; k++) {
            gfx_line(x + k * 4, y + 15, x + k * 4 + 2, y + 6 + (k % 2) * 3, C_ICE);
            gfx_line(x + k * 4 + 2, y + 6 + (k % 2) * 3, x + k * 4 + 4, y + 15, C_SKY);
        }
        gfx_rect(x, y + 14, 16, 2, t->wall_lo);
        break;
    case '~': {
        int ph = (frame_t / 8 + tx) % 4;
        gfx_rect(x, y + 3, 16, 13, C_BLUE);
        gfx_hline(x + ph, x + ph + 5, y + 3, C_ICE);
        gfx_hline(x + ((ph + 8) & 15), x + ((ph + 8) & 15) + 3, y + 6, C_SKY);
        break;
    }
    case 'T':
        gfx_vline(x + 7, y + 6, y + 15, C_INK);
        gfx_rect(x + 5, y + 2, 5, 5, (frame_t / 6 + tx) % 2 ? C_YELLOW : C_ORANGE);
        gfx_dither_circle(x + 7, y + 4, 8, C_AMBER, 3);
        break;
    }
}


static void draw_crab(const Ent *b) {
    int x = (int)b->x, y = (int)b->y;
    int bob = (int)(sinf(frame_t * 0.05f) * 2);
    int bs = b->flash > 0 ? C_WHITE : -1;
    if (b->state == 99 && (b->t / 3) % 2) bs = C_YELLOW;
    /* the eye on its stalk: the only soft spot */
    int ex = (int)EYE_X, ey = EYE_Y + bob;
    gfx_rect(ex - 1, ey + 5, 3, y + 12 - ey, C_WINE);
    gfx_vline(ex - 1, ey + 5, y + 12, C_MAROON);
    gfx_circ(ex, ey, 6, C_INK);
    gfx_circ(ex, ey, 5, bs >= 0 ? bs : C_CREAM);
    gfx_circ(ex + (pcx() < ex ? -1 : 1), ey + 1, 2, C_INK);
    gfx_pset(ex - 2, ey - 3, C_WHITE);
    /* the shell and claw */
    spr_draw_scaled(&rc_spr[(frame_t / 12) % 2 ? R_CRAB1 : R_CRAB2], x - 2, y + 8 + bob, 2, SPR_FLIPX);
    spr_draw_scaled(&rc_spr[R_CLAW], x - 18, y + 24 + bob + (int)(sinf(frame_t * 0.08f) * 4), 2, 0);
    if (bs >= 0) gfx_dither(x, y + 8 + bob, 64, 40, bs, 8);
}

static void draw_ent(const Ent *e) {
    int x = (int)e->x, y = (int)e->y;
    int solid = e->flash > 0 ? C_WHITE : -1;
    int f = (e->t / 8) % 2;
    int fl = e->dir > 0 ? SPR_FLIPX : 0;
    uint8_t rm[PAL_COUNT];
    switch (e->type) {
    case E_PIGEON: case E_RPIGEON: {
        const uint8_t *remap = NULL;
        if (e->type == E_RPIGEON) {
            pal_identity(rm);
            rm[C_GREY] = C_AMBER; rm[C_LIGHT] = C_YELLOW; rm[C_SLATE] = C_ORANGE;
            remap = rm;
        }
        /* pigeon sprites face right */
        int spr = e->sub > 0 ? R_PIGEON_ROLL : f ? R_PIGEON1 : R_PIGEON2;
        int pfl = e->dir < 0 ? SPR_FLIPX : 0;
        spr_draw_ex(&rc_spr[spr], x - 2, y - 5, pfl | (e->sub > 0 && (e->t / 4) % 2 ? SPR_FLIPY : 0), remap, solid);
        break;
    }
    case E_CROW: {
        /* hides in its chimney; pops up to shoot */
        const uint8_t *remap = NULL;
        if (e->mode == M_PURPLE) { pal_identity(rm); rm[C_NIGHT] = C_PURPLE; rm[C_DUSK] = C_VIOLET; remap = rm; }
        if (e->sub == 1) spr_draw_ex(&rc_spr[e->cd > 20 && e->cd < 45 ? R_CROW2 : R_CROW1], x - 2, y - 4, fl, remap, solid);
        else { gfx_pset(x + 3, y + 10, C_YELLOW); gfx_pset(x + 8, y + 10, C_YELLOW); }
        break;
    }
    case E_GECKO: spr_draw_ex(&rc_spr[f ? R_GECKO1 : R_GECKO2], x - 2, y - 9, fl, NULL, solid); break;
    case E_GULL: spr_draw_ex(&rc_spr[(e->t / 6) % 2 ? R_GULL1 : R_GULL2], x - 1, y - 1, SPR_FLIPX, NULL, solid); break;
    case E_LAMP: {
        if (e->mode == M_MOVING) gfx_vline(x + 5, (int)e->hy - 44, y, C_INK);
        else gfx_vline(x + 5, y - 16, y, C_INK);
        spr_draw_ex(&rc_spr[R_LAMP], x - 1, y - 1, 0, NULL, solid);
        gfx_dither_circle(x + 5, y + 7, 9, C_AMBER, 3);
        for (int k = 0; k < e->n; k++) {
            float sx, sy;
            lamp_spike(e, k, &sx, &sy);
            spr_draw(&rc_spr[R_SPIKE], (int)sx - 1, (int)sy - 1, 0);
        }
        break;
    }
    case E_FFISH: if (e->sub != 0) spr_draw_ex(&rc_spr[f ? R_FFISH1 : R_FFISH2], x - 1, y, e->vx > 0 ? SPR_FLIPX : 0, NULL, solid); break;
    case E_PUFFER: if (e->sub != 0) spr_draw_ex(&rc_spr[e->sub == 2 ? R_PUFFER2 : R_PUFFER1], x - 1, y - 1, e->vx > 0 ? SPR_FLIPX : 0, NULL, solid); break;
    case E_WASP: spr_draw_ex(&rc_spr[(e->t / 3) % 2 ? R_WASP1 : R_WASP2], x, y, SPR_FLIPX, NULL, solid); break;
    case E_SNAIL: {
        const uint8_t *remap = NULL;
        if (e->sub) { pal_identity(rm); rm[C_RED] = C_JADE; rm[C_ORANGE] = C_LEAF; rm[C_WINE] = C_FOREST; remap = rm; }
        if (e->mode == M_CEILING) spr_draw_ex(&rc_spr[(e->t / 16) % 2 ? R_SNAIL1 : R_SNAIL2], x - 2, y, fl | SPR_FLIPY, remap, solid);
        else spr_draw_ex(&rc_spr[(e->t / 16) % 2 ? R_SNAIL1 : R_SNAIL2], x - 2, y - 4, fl, remap, solid);
        break;
    }
    case E_TOAD: spr_draw_ex(&rc_spr[e->vy != 0 ? R_TOAD2 : R_TOAD1], x - 1, y - 1, fl, NULL, solid); break;
    case E_PELICAN: spr_draw_ex(&rc_spr[(e->t / 6) % 2 ? R_PELICAN1 : R_PELICAN2], x - 1, y - 2, 0, NULL, solid); break;
    case E_JAR:
        if (e->mode == M_WATER && e->sub == 0) break;
        spr_draw_ex(&rc_spr[e->vy != 0 ? R_JAR2 : R_JAR1], x - 1, y - 1, fl | (e->mode == M_DIVE ? SPR_FLIPY : 0), NULL, solid);
        break;
    case E_CRACKER:
        spr_draw_ex(&rc_spr[e->sub && (e->t / 3) % 2 ? R_CRACKER2 : R_CRACKER1], x - 1, y - 1, 0, NULL, solid);
        if (e->sub) gfx_pset(x + 5 + (e->t * 7) % 3 - 1, y - 3, (e->t / 2) % 2 ? C_YELLOW : C_WHITE); /* drawing never touches the game rng */
        break;
    case E_SPIDER:
        gfx_vline(x + 4, (int)e->hy - 2, y, C_LIGHT);
        spr_draw_ex(&rc_spr[f ? R_SPIDER1 : R_SPIDER2], x - 1, y, 0, NULL, solid);
        break;
    case E_MAGPIE:
        if (e->sub == 0) {
            /* coming out of nowhere: a flicker, then a shape */
            if ((e->k / 2) % 2 || e->k > 16) gfx_dither_circle(x + 6, y + 4, 8 - e->k / 4, C_SLATE, 6);
            if (e->k > 12 && (e->k / 2) % 2) spr_draw_ex(&rc_spr[R_MAGPIE1], x - 2, y - 2, 0, NULL, C_DUSK);
            break;
        }
        spr_draw_ex(&rc_spr[(e->t / 4) % 2 ? R_MAGPIE1 : R_MAGPIE2], x - 2, y - 2, e->vx < 0 ? SPR_FLIPX : 0, NULL, solid);
        break;
    case E_FLASHER: {
        if (e->sub == 0) break;
        bool ghost = (e->sub == 1 && e->k < 20 && (e->k / 2) % 2) || (e->sub == 3 && (e->k / 2) % 2);
        if (ghost) break;
        bool charge = e->sub == 1 && e->k > 20;
        spr_draw_ex(&rc_spr[charge && (e->k / 3) % 2 ? R_FLASHER2 : R_FLASHER1], x - 2, y - 3, fl, NULL, solid);
        if (charge && (e->k / 3) % 2) gfx_dither_circle(x + (e->dir > 0 ? 12 : 0), y + 5, 6, C_WHITE, 8);
        break;
    }
    case E_SHEET: spr_draw_ex(&rc_spr[(e->t / 7) % 2 ? R_SHEET1 : R_SHEET2], x - 2, y - 2, e->dir > 0 ? SPR_FLIPX : 0, NULL, solid); break;
    case E_MOTH: spr_draw_ex(&rc_spr[(e->t / 3) % 2 ? R_MOTH1 : R_MOTH2], x - 1, y - 1, 0, NULL, solid); break;
    case E_BOSS: draw_crab(e); break;
    case E_LEG: {
        /* a crab leg breaking the surface: a knobbly red platform */
        if (e->y > 150) break;
        int h = 160 - y;
        gfx_rect(x + 8, y + 4, 12, h, C_WINE);
        gfx_rect(x + 10, y + 4, 3, h, C_RED);
        gfx_rect(x, y, e->w, 6, C_RED);
        gfx_hline(x, x + e->w - 1, y, C_PINK);
        gfx_hline(x, x + e->w - 1, y + 6, C_MAROON);
        for (int k = 3; k < e->w; k += 7) gfx_pset(x + k, y + 3, C_ORANGE);
        break;
    }
    case E_STAR: spr_draw(&rc_spr[(e->t / 3) % 2 ? R_STAR1 : R_STAR2], x, y, 0); break;
    case E_SPIRIT_SHOT: spr_draw(&rc_spr[R_WISP_SHOT], x, y, 0); break;
    case E_PEBBLE: case E_SHRAP: spr_draw(&rc_spr[R_PEBBLE], x, y, 0); break;
    case E_SEED: spr_draw(&rc_spr[R_SEED], x, y, 0); break;
    case E_BUBBLE:
        if (e->mode == M_POP) spr_draw(&rc_spr[R_BUBBLE], x, y, 0);
        else { /* Old Crab's shots */
            pal_identity(rm);
            rm[C_CYAN] = C_LIME; rm[C_SKY] = C_LEAF; rm[C_ICE] = C_WHITE; rm[C_BLUE] = C_JADE;
            spr_draw_ex(&rc_spr[R_BUBBLE], x, y, 0, rm, -1);
        }
        break;
    case E_BOMB: spr_draw(&rc_spr[R_BOMB], x, y, 0); break;
    case E_ORB: spr_draw(&rc_spr[R_ORB], x, y, 0); break;
    case E_ORBHALF: gfx_rect(x, y, 4, 4, C_MAGENTA); gfx_pset(x + 1, y + 1, C_PINK); break;
    case E_BEAM:
        gfx_rect(x, y, e->w, 3, (e->t / 2) % 2 ? C_WHITE : C_YELLOW);
        gfx_hline(x, x + e->w - 1, y + 1, C_CREAM);
        break;
    case E_DUST:
        gfx_dither_circle(x + 8, y + 7, 8, e->t % 8 < 4 ? C_GREY : C_LIGHT, 6 + (e->t > 20 ? 4 : 0));
        break;
    case E_CLOUD: {
        /* the firecracker's choking cloud: thick, then thinning out */
        int lvl = e->t > 80 ? 12 : e->t > 20 ? 9 : 5;
        int cx = x + e->w / 2, cy = y + e->h / 2;
        if (e->t > 90) gfx_circ(cx, cy, 10 + (100 - e->t), (e->t / 2) % 2 ? C_YELLOW : C_ORANGE);
        gfx_dither_circle(cx - 8, cy + 2, 12, C_GREY, lvl);
        gfx_dither_circle(cx + 7, cy - 3, 13, C_SLATE, lvl);
        gfx_dither_circle(cx, cy + 6, 11, C_LIGHT, lvl - 2);
        break;
    }
    case E_TOKEN: {
        pal_identity(rm);
        /* green, silver or violet fish: 100, 200 or 300 */
        if (e->value == 100) { rm[C_SKY] = C_LEAF; rm[C_BLUE] = C_JADE; rm[C_CYAN] = C_LIME; }
        else if (e->value >= 300) { rm[C_SKY] = C_VIOLET; rm[C_BLUE] = C_PURPLE; rm[C_CYAN] = C_MAGENTA; }
        else { rm[C_SKY] = C_LIGHT; rm[C_BLUE] = C_GREY; rm[C_CYAN] = C_WHITE; }
        spr_draw_ex(&rc_spr[R_FISH], x, y + ((e->state != 1 && (frame_t / 12) % 2) ? 1 : 0), 0, rm, -1);
        break;
    }
    case E_CROWN: spr_draw(&rc_spr[R_CROWN], x, y + ((frame_t / 10) % 2), 0); break;
    case E_LETTER:
        gfx_dither_circle(x + 5, y + 4, 9, C_YELLOW, 4);
        spr_draw(&rc_spr[R_LETTER], x, y + ((frame_t / 10) % 2), 0);
        break;
    case E_LANTERN: spr_draw(&rc_spr[R_LANTERN], x, y, 0); break;
    case E_CATNIP: spr_draw_outline(&rc_spr[R_CATNIP], x, y, 0, (frame_t / 4) % 2 ? C_WHITE : C_LIME); break;
    case E_TEXT: {
        char buf[16];
        if (e->value < 0) snprintf(buf, sizeof buf, "1UP");
        else snprintf(buf, sizeof buf, "%d", e->value);
        tiny_draw(buf, x, y, (e->t / 3) % 2 ? C_WHITE : C_YELLOW);
        break;
    }
    case E_DEBRIS: gfx_rect(x, y, 2, 2, e->value); break;
    case E_SPOT: break; /* invisible */
    }
}

/* the bonus stretches' snacks: dates, then figs, then glasses of tea */
static void draw_snacks(int tx0, int tx1) {
    for (int ty = 0; ty < RC_ROWS; ty++)
        for (int tx = imax(tx0, 0); tx <= tx1 && tx < MAX_COLS; tx++) {
            uint8_t m = snackm[ty][tx];
            if (!m) continue;
            int g = bonus_of_col(tx), n = g >= 0 ? bonus_got[g] : 0;
            int spr = n < 100 ? R_DATE : n < 200 ? R_FIG : R_TEA;
            for (int b = 0; b < 4; b++)
                if (m & (1 << b)) {
                    int sx = tx * 16 + (b & 1) * 8, sy = ty * 16 + (b >> 1) * 8;
                    spr_draw(&rc_spr[spr], sx, sy + ((frame_t / 14 + tx + b) % 2), 0);
                }
        }
}

static void draw_player(void) {
    int x = (int)pl.x - 3, y = (int)pl.y - 3;
    if (pl.dead_t > 0) return;
    if (pl.spirit) {
        if (pl.spirit_t > 60 || (pl.spirit_t / 4) % 2)
            spr_draw(&rc_spr[(frame_t / 10) % 2 ? R_CAT_SPIRIT1 : R_CAT_SPIRIT2], x, y + (int)(sinf(frame_t * 0.1f) * 2), 0);
        return;
    }
    int fl = pl.face < 0 ? SPR_FLIPX : 0;
    int spr;
    if (pl.throw_t > 0) spr = R_CAT_THROW;
    else if (!pl.ground) spr = pl.jumps >= 2 && pl.vy < 0 ? R_CAT_SPIN : pl.vy < 0 ? R_CAT_JUMP : R_CAT_FALL;
    else if (pl.still) spr = R_CAT_IDLE; /* holding her ground */
    else {
        static const int run[4] = {R_CAT_RUN1, R_CAT_RUN2, R_CAT_RUN3, R_CAT_RUN4};
        spr = run[(pl.anim / 10) % 4];
    }
    spr_draw(&rc_spr[spr], x, y, fl);
}

static void draw_hud(void) {
    gfx_rect(0, 0, 320, RC_HUD, C_INK);
    gfx_hline(0, 319, RC_HUD - 1, C_DUSK);
    char buf[48];
    snprintf(buf, sizeof buf, "%07u", (unsigned)score);
    text_draw(buf, 5, 6, C_WHITE);
    int area = in_arena ? RC_AREAS - 1 : area_of_col((cam_px() + 160) >> 4);
    tiny_draw(in_arena ? "OLD CRAB'S WATER" : RC_AREA_NAME[area], 70, 4, C_SLATE);
    tiny_draw(loop >= 2 ? "NIGHT ROUTE" : "DAY ROUTE", 70, 11, loop >= 2 ? C_VIOLET : C_SKY);
    for (int i = 0; i < 2; i++) spr_draw_ex(&rc_spr[R_CATNIP], 206 + i * 10, 6, 0, NULL, i < power ? -1 : C_DUSK);
    for (int i = 0; i < imin(lives, 6); i++) spr_draw(&rc_spr[R_CAT_HEAD], 232 + i * 9, 7, 0);
    if (lives > 6) { snprintf(buf, sizeof buf, "+%d", lives - 6); tiny_draw(buf, 288, 8, C_WHITE); }
    if (in_arena && boss_i >= 0 && ents[boss_i].alive) {
        Ent *b = &ents[boss_i];
        int w = 100 * iclamp(b->hp, 0, BOSS_HP) / BOSS_HP;
        gfx_rect(110, 170, 102, 6, C_INK);
        gfx_rect(111, 171, w, 4, C_RED);
        gfx_rectb(110, 170, 102, 6, C_WHITE);
    }
}

static void draw_play(void) {
    int sx = 0, sy = 0;
    if (shake > 0) { sx = (frame_t % 3) - 1; sy = ((frame_t / 2) % 3) - 1; }
    gfx_camera(sx, -RC_HUD + sy);
    gfx_clip(0, RC_HUD, 320, 160);
    draw_sky();
    int cx = cam_px();
    gfx_camera(cx + sx, -RC_HUD + sy);
    int tx0 = cx >> 4, tx1 = (cx + 320) >> 4;
    for (int i = 0; i < MAX_ENTS; i++)
        if (ents[i].alive && ents[i].type == E_LEG) draw_ent(&ents[i]);
    for (int ty = 0; ty < RC_ROWS; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            char c = tile_at(tx, ty);
            if (c != '.') draw_tile(tx, ty, c, tx * 16, ty * 16);
        }
    if (loop >= 2) {
        gfx_camera(0, 0);
        gfx_remap_rect(0, RC_HUD, 320, 160, night_map);
        gfx_camera(cx + sx, -RC_HUD + sy);
    }
    draw_snacks(tx0, tx1);
    for (int i = 0; i < MAX_ENTS; i++)
        if (ents[i].alive && ents[i].state != -1 && ents[i].type != E_LEG) draw_ent(&ents[i]);
    draw_player();
    gfx_camera(0, 0);
    gfx_noclip();
    draw_hud();
}

static void draw_board(int x, int y, bool fresh) {
    /* the high-score board */
    ui_panel(x, y, 108, 70, C_INK, C_YELLOW);
    text_center("HIGH SCORES", x + 54, y + 5, C_YELLOW);
    char buf[32];
    for (int i = 0; i < HISCORES; i++) {
        bool mine = fresh && i == new_rank;
        snprintf(buf, sizeof buf, "%d. %07u", i + 1, (unsigned)sv.hi[i]);
        text_draw(buf, x + 12, y + 17 + i * 10, mine ? ((state_t / 6) % 2 ? C_WHITE : C_YELLOW) : sv.hi[i] ? C_LIGHT : C_DUSK);
    }
}

static void draw_title(void) {
    gfx_camera(0, 0);
    gfx_clip(0, 0, 320, 180);
    for (int b = 0; b < 4; b++) gfx_rect(0, b * 45, 320, 45, THEMES[0].sky[b]);
    for (int b = 0; b < 3; b++) gfx_dither(0, b * 45 + 35, 320, 10, THEMES[0].sky[b + 1], 8);
    gfx_circ(262, 30, 14, C_YELLOW);
    gfx_circ(262, 30, 10, C_CREAM);
    for (int i = 0; i < 6; i++) {
        int x = (i * 97 - frame_t / 4) % 420;
        if (x < -60) x += 420;
        gfx_circ(x, 20 + (i % 3) * 14, 8, C_WHITE);
        gfx_circ(x + 10, 17 + (i % 3) * 14, 10, C_WHITE);
        gfx_circ(x + 22, 21 + (i % 3) * 14, 7, C_WHITE);
    }
    gfx_noclip();
    /* a row of rooftops */
    for (int i = 0; i < 21; i++) {
        int x = i * 16 - (frame_t / 2) % 16;
        const Theme *t = &THEMES[0];
        gfx_rect(x, 140, 16, 40, t->wall);
        gfx_rect(x, 140, 16, 2, t->trim);
        if (i % 3 == 1) { gfx_rect(x + 4, 152, 8, 9, t->window_lo); gfx_rect(x + 5, 153, 6, 7, t->window); }
    }
    static const int run[4] = {R_CAT_RUN1, R_CAT_RUN2, R_CAT_RUN3, R_CAT_RUN4};
    spr_draw_scaled(&rc_spr[run[(frame_t / 5) % 4]], 30, 108, 2, 0);
    spr_draw(&rc_spr[(frame_t / 4) % 2 ? R_MAGPIE1 : R_MAGPIE2], 90 + (int)(sinf(frame_t * 0.05f) * 10), 90, 0);
    spr_draw(&rc_spr[R_PARCEL], 104 + (int)(sinf(frame_t * 0.05f) * 10), 104, 0);
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE, C_RED};
    ui_fancy_center("ROOFCAT", 160, 14, 4, grad, 4, C_INK, C_WINE);
    text_center("CATCH THE MAGPIE MOB!", 160, 48, C_NAVY);
    draw_board(196, 62, false);
    char buf[48];
    snprintf(buf, sizeof buf, "MOST FOES %d " GLYPH_DOT " MOST LETTERS %d", sv.most_foes, sv.most_letters);
    tiny_center(buf, 250, 134, C_NAVY);
    if ((frame_t / 20) % 2) {
        ui_panel(36, 66, 116, 18, C_NAVY, C_WHITE);
        text_center("PRESS " GLYPH_A " TO START", 94, 71, C_WHITE);
    }
    gfx_rect(0, 170, 320, 10, C_INK);
    tiny_draw("A JUMP x2   B THROW   DOWN+A DROP   LEFT: HOLD   B ON TITLE: LIBRARY", 8, 172, C_LIGHT);
}

static void draw_intro(void) {
    draw_play();
    int t = state_t;
    int y = t < 16 ? -30 + t * 3 : t > 56 ? 18 - (t - 56) * 3 : 18;
    ui_panel(70, y + 30, 180, 34, C_INK, loop >= 2 ? C_VIOLET : C_YELLOW);
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center(loop >= 2 ? "NIGHT ROUTE" : "DAY ROUTE", 160, y + 35, 1, grad, 3, C_INK, -1);
    text_center(RC_AREA_NAME[cur_area], 160, y + 49, C_WHITE);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    static const uint8_t grad[] = {C_LIGHT, C_GREY, C_SLATE};
    ui_fancy_center("GAME OVER", 160, 16, 3, grad, 3, C_INK, C_NIGHT);
    /* game over in a bonus stretch earns a remark */
    if (over_in_bonus) text_center("SHE IS NO ROOFCAT.", 160, 44, C_ORANGE);
    spr_draw_scaled(&rc_spr[R_CAT_HURT], 48, 80, 2, 0);
    char buf[64];
    snprintf(buf, sizeof buf, "SCORE %07u", (unsigned)score);
    text_center(buf, 64, 122, C_GREY);
    if (new_rank >= 0) text_center("NEW HIGH SCORE!", 64, 134, (state_t / 8) % 2 ? C_YELLOW : C_WHITE);
    draw_board(180, 62, true);
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 158, C_YELLOW);
}

static void draw_ending(void) {
    int t = state_t;
    bool night = loop >= 2;
    static const uint8_t day_sky[4] = {C_SKY, C_CYAN, C_CYAN, C_ICE};
    static const uint8_t night_sky[4] = {C_INK, C_NIGHT, C_NAVY, C_DUSK};
    const uint8_t *sky = night ? night_sky : day_sky;
    for (int b = 0; b < 4; b++) gfx_rect(0, b * 45, 320, 45, sky[b]);
    for (int b = 0; b < 3; b++) gfx_dither(0, b * 45 + 35, 320, 10, sky[b + 1], 8);
    if (night) { gfx_circ(270, 30, 11, C_CREAM); gfx_circ(274, 27, 9, sky[0]); }
    else { gfx_circ(270, 30, 13, C_YELLOW); gfx_circ(270, 30, 9, C_CREAM); }
    /* the town and Grandma Rosa's house */
    int wall = night ? C_GREY : C_WHITE, trim = night ? C_NAVY : C_SKY, door = night ? C_NAVY : C_BLUE;
    for (int i = 0; i < 20; i++) {
        gfx_rect(i * 16, 142, 16, 38, wall);
        gfx_rect(i * 16, 142, 16, 2, trim);
    }
    gfx_circ(250, 88, 24, door);
    gfx_pset(244, 70, night ? C_SKY : C_CYAN);
    gfx_rect(206, 88, 88, 54, wall);
    gfx_rect(206, 88, 88, 3, trim);
    gfx_rect(240, 110, 20, 32, door);
    gfx_circ(250, 110, 10, door);
    gfx_rect(216, 100, 12, 12, door);
    gfx_rect(272, 100, 12, 12, door);
    gfx_dither_circle(222, 126, 10, C_MAGENTA, 8); /* bougainvillea */
    gfx_dither_circle(282, 128, 9, C_MAGENTA, 8);
    /* Grandma: a silver cat in a pink shawl */
    uint8_t gran[PAL_COUNT];
    pal_identity(gran);
    gran[C_ORANGE] = C_LIGHT; gran[C_RED] = C_GREY; gran[C_BLUE] = C_MAGENTA; gran[C_SKY] = C_PINK;
    gran[C_HIDE] = C_WHITE; gran[C_LEAF] = C_SKY;
    spr_draw_ex(&rc_spr[R_CAT_IDLE], 238, 126, SPR_FLIPX, gran, -1);
    /* Pepper runs in with the parcel */
    int cx = -20 + imin(t * 2, 200);
    static const int run[4] = {R_CAT_RUN1, R_CAT_RUN2, R_CAT_RUN3, R_CAT_RUN4};
    bool arrived = t * 2 >= 220;
    spr_draw_scaled(&rc_spr[arrived ? R_CAT_IDLE : run[(t / 4) % 4]], cx, 110, 2, 0);
    if (arrived) {
        /* they share the parcel; the second loop brings home a whole stack */
        int stack = night ? 4 : 1;
        for (int k = 0; k < stack; k++) spr_draw(&rc_spr[R_PARCEL], 218, 130 - k * 11, 0);
        for (int i = 0; i < 5; i++) {
            int hy = 120 - ((t * 1 + i * 14) % 60);
            spr_draw(&rc_spr[R_HEART], 212 + i * 9 + (int)(sinf(t * 0.08f + i) * 3), hy, 0);
        }
    } else {
        spr_draw(&rc_spr[R_PARCEL], cx + 28, 124, 0);
    }
    /* the card */
    ui_panel(34, 8, 252, 52, C_INK, night ? C_VIOLET : C_YELLOW);
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE, C_RED};
    ui_fancy_center(night ? "NIGHT ROUTE CLEARED!" : "PARCEL DELIVERED!", 160, 13, 2, grad, 4, C_INK, C_WINE);
    char buf[80];
    snprintf(buf, sizeof buf, "SCORE %07u   LETTERS %d", (unsigned)score, letters_this_run);
    text_center(buf, 160, 34, C_WHITE);
    text_center(night ? "EVEN THE MAGPIE MOB SLEEPS NOW." : "HAPPY BIRTHDAY, GRANDMA ROSA!", 160, 46, night ? C_VIOLET : C_YELLOW);
    if (t > 240 && (t / 20) % 2) {
        gfx_rect(80, 160, 160, 12, C_INK);
        text_center(night ? GLYPH_A " HIGH SCORES" : GLYPH_A " ON INTO THE NIGHT", 160, 162, C_WHITE);
    }
}

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    int x = 2, y = 2, rowh = 0;
    for (int i = 0; i < R_SPRITE_COUNT; i++) {
        Sprite *s = &rc_spr[i];
        if (!s->px) continue;
        if (x + s->w > 318) { x = 2; y += rowh + 2; rowh = 0; }
        spr_draw(s, x, y, 0);
        x += s->w + 2;
        if (s->h > rowh) rowh = s->h;
    }
}

static void rc_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_INTRO: draw_intro(); break;
    case S_PLAY: draw_play(); break;
    case S_OVER: draw_over(); break;
    case S_ENDING: draw_ending(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void rc_load(void) {
    rc_art_load();
    rc_audio_load();
    build_night_map();
    build_world();
}

static void rc_start(void) {
    rng_seed(&rng, g_rng.state ^ 0xCA7ull);
    load_save();
    sheet_mode = false;
    loop = 1;
    score = 0;
    new_rank = -1;
    recorded = true; /* nothing to record until a run starts */
    memset(ents, 0, sizeof ents);
    to_title();
}

static void rc_quit(void) {
    /* leaving in the middle of a run still puts its score on the board */
    if (state == S_INTRO || state == S_PLAY || state == S_ENDING) record_run();
    save_now();
}

static void rc_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h / 2, C_SKY);
    gfx_rect(x, y + h / 2, w, h / 2, C_CYAN);
    gfx_dither(x, y + h / 2 - 6, w, 6, C_CYAN, 8);
    gfx_circ(x + w - 22, y + 14, 8, C_YELLOW);
    gfx_circ(x + w - 22, y + 14, 5, C_CREAM);
    for (int i = 0; i < 5; i++) {
        int bx = x + i * 30 - (t / 3) % 30;
        gfx_rect(bx, y + 40 + (i % 2) * 4, 26, 30, C_WHITE);
        gfx_rect(bx, y + 40 + (i % 2) * 4, 26, 2, C_SKY);
        gfx_rect(bx + 9, y + 48 + (i % 2) * 4, 6, 6, C_BLUE);
        if (i == 2) gfx_circ(bx + 13, y + 40, 7, C_BLUE);
    }
    static const int run[4] = {R_CAT_RUN1, R_CAT_RUN2, R_CAT_RUN3, R_CAT_RUN4};
    int cy = y + 24 + (int)(fabsf(sinf(t * 0.08f)) * -8);
    spr_draw(&rc_spr[run[(t / 5) % 4]], x + 30, cy, 0);
    spr_draw(&rc_spr[(t / 3) % 2 ? R_STAR1 : R_STAR2], x + 50 + (t * 2) % 60, cy + 4, 0);
    spr_draw(&rc_spr[(t / 4) % 2 ? R_MAGPIE1 : R_MAGPIE2], x + 104, y + 12 + (int)(sinf(t * 0.07f) * 4), 0);
    spr_draw(&rc_spr[R_PARCEL], x + 106, y + 24 + (int)(sinf(t * 0.07f) * 4), 0);
}

/* ------------------------------------------------------------------ */
/* test hooks                                                           */

static int foe_type_of(const char *name) {
    static const struct { const char *n; int t; } T[] = {
        {"pigeon", E_PIGEON}, {"rpigeon", E_RPIGEON}, {"crow", E_CROW}, {"gecko", E_GECKO}, {"gull", E_GULL},
        {"lamp", E_LAMP}, {"ffish", E_FFISH}, {"wasp", E_WASP}, {"snail", E_SNAIL}, {"toad", E_TOAD},
        {"pelican", E_PELICAN}, {"jar", E_JAR}, {"cracker", E_CRACKER}, {"puffer", E_PUFFER}, {"spider", E_SPIDER},
        {"magpie", E_MAGPIE}, {"flasher", E_FLASHER}, {"sheet", E_SHEET}, {"moth", E_MOTH},
        {"bubble", E_BUBBLE}, {"cloud", E_CLOUD}, {"shard", E_SHRAP}, {"beam", E_BEAM}, {"orbhalf", E_ORBHALF},
        {"orb", E_ORB}, {"star", E_STAR}, {"token", E_TOKEN}, {"catnip", E_CATNIP}, {"letter", E_LETTER},
        {"seed", E_SEED}, {"pebble", E_PEBBLE}, {"crown", E_CROWN}, {"lantern", E_LANTERN}, {"bomb", E_BOMB},
    };
    for (int i = 0; i < ARRAY_LEN(T); i++)
        if (!strcmp(name, T[i].n)) return T[i].t;
    return 0;
}

static int count_foes(void) {
    int n = 0;
    for (int i = 0; i < MAX_ENTS; i++) n += ents[i].alive && IS_FOE(ents[i].type) && ents[i].state != -1;
    return n;
}

static Ent *first_of(int type) {
    for (int i = 0; i < MAX_ENTS; i++)
        if (ents[i].alive && ents[i].type == type && type) return &ents[i];
    return NULL;
}

/* ---- the demo player ----------------------------------------------------
 * A simple, sensible player for the tests: it keeps B held, keeps to the
 * left half of the screen, jumps (and double jumps) over gaps, glass and
 * water, hops over what walks or flies at it, and as a spirit floats to a
 * safe roof and comes back. Its answer is a button mask the headless runner
 * presses for real. */
static bool bot_drift;
static int bot_hold, bot_want, bot_run, bot_wait; /* A to hold; a jump waiting for A to come up; run; wait */

static bool danger_near(float x0, float x1, float y0, float y1) {
    for (int i = 0; i < MAX_ENTS; i++) {
        const Ent *e = &ents[i];
        if (!e->alive || e->state == -1 || harmless(e->type) || e->type == E_BOSS) continue;
        if (e->type == E_CROW && e->sub != 1) continue;
        if ((e->type == E_FFISH || e->type == E_PUFFER || e->type == E_MAGPIE || e->type == E_FLASHER) && e->sub == 0) continue;
        if (e->type == E_JAR && e->mode == M_WATER && e->sub == 0) continue;
        if (e->x + e->w > x0 && e->x < x1 && e->y + e->h > y0 && e->y < y1) return true;
    }
    return false;
}

static bool shot_near(float x0, float x1, float y0, float y1) {
    for (int i = 0; i < MAX_ENTS; i++) {
        const Ent *e = &ents[i];
        if (!e->alive) continue;
        switch (e->type) {
        case E_PEBBLE: case E_SEED: case E_BUBBLE: case E_SHRAP: case E_BEAM: case E_ORBHALF: case E_BOMB:
        case E_ORB: case E_DUST: case E_CLOUD:
            if (e->x + e->w > x0 && e->x < x1 && e->y + e->h > y0 && e->y < y1) return true;
            break;
        }
    }
    return false;
}

/* Frames until a shot, or a fast foe, reaches her if she stays on the ground
 * riding the scroll; -1 if nothing does within 30 frames. */
static int threat_eta(void) {
    int best = -1;
    for (int i = 0; i < MAX_ENTS; i++) {
        const Ent *e = &ents[i];
        if (!e->alive || e->state == -1) continue;
        float x = e->x, y = e->y, vx = e->vx, vy = e->vy;
        int kind = e->type;
        switch (kind) {
        case E_PEBBLE: case E_SEED: case E_BUBBLE: case E_SHRAP: case E_BEAM: case E_ORBHALF: case E_BOMB:
        case E_MAGPIE: break;
        case E_WASP: vx = SCROLL + 1.3f; vy = fclamp((pcy() - (e->y + 3)) * 0.04f, -0.8f, 0.8f); break;
        case E_PIGEON: case E_RPIGEON: if (e->sub == 0) continue; vx = (e->type == E_RPIGEON ? 3.9f : 1.5f) * (float)e->dir; vy = 0; break;
        case E_GULL: vx = -0.25f; vy = 0; break;
        case E_PELICAN: vx = -2.2f; vy = 0; break;
        case E_SHEET: if (e->sub == 0) continue; vx = 1.3f * (float)e->dir; vy = 0; break;
        default: continue;
        }
        if (kind == E_MAGPIE && e->sub == 0) continue;
        for (int f = 1; f <= 30; f++) {
            if (kind == E_SEED) {
                vy += 0.16f;
                if (vy > 0 && box_solid(x + vx, y + vy, e->w, e->h)) vy = -2.4f;
            } else if (kind == E_PEBBLE && vy != 0) vy += 0.13f;
            else if (kind == E_BOMB) vy += 0.15f;
            x += vx;
            y += vy;
            /* her own path: riding the scroll, falling if she is in the air */
            float px = pl.x + SCROLL * (float)f;
            float py = pl.ground ? pl.y : fminf(pl.y + pl.vy * (float)f + 0.5f * GRAV * (float)(f * f), pl.y + 40);
            if (rects_overlap((int)px, (int)py + 1, PW, PH - 1, (int)x, (int)y, e->w, e->h)) {
                if (best < 0 || f < best) best = f;
                break;
            }
        }
    }
    return best;
}

/* a foe in line with her stars, ahead (dir 1) or behind (dir -1), within reach */
static bool foe_in_line(int dir, float reach) {
    float sy = pl.y + 7; /* the middle of a thrown star */
    for (int i = 0; i < MAX_ENTS; i++) {
        const Ent *e = &ents[i];
        if (!e->alive || e->state == -1 || !IS_FOE(e->type) || !hittable(e)) continue;
        float dx = (e->x + e->w / 2) - pcx();
        if (dx * dir < 0 || fabsf(dx) > reach) continue;
        if (e->y - 3 < sy && e->y + e->h + 3 > sy) return true;
    }
    return false;
}

/* a foe ahead standing higher up, out of reach of a star thrown from here */
static bool foe_above_ahead(void) {
    for (int i = 0; i < MAX_ENTS; i++) {
        const Ent *e = &ents[i];
        if (!e->alive || e->state == -1 || !IS_FOE(e->type) || !hittable(e) || e->type == E_CROW) continue;
        if (e->x > pl.x && e->x < pl.x + 120 && e->y + e->h <= pl.y + 3 && e->y + e->h > pl.y - 44) return true;
    }
    return false;
}

/* is there a floor under world x at the row her feet stand on (or a little lower)? */
static bool floor_ahead(float x, int feet_row) {
    for (int r = feet_row; r < imin(feet_row + 2, RC_ROWS); r++) {
        char t = tile_at((int)x >> 4, r);
        if (t == '^' || t == '~') return false;
        if (floor_at((int)x >> 4, r)) return true;
    }
    return false;
}

/* how far from world x to the far side of the gap (or glass, or water) ahead? */
static int gap_width(float x, int feet_row) {
    int s = 0, w = 0;
    while (s < 24 && floor_ahead(x + (float)s, feet_row)) s += 2;
    while (w < 200 && !floor_ahead(x + (float)(s + w), feet_row)) w += 4;
    return s + w;
}

static void bot_jump(int frames) {
    if (bot_hold == 0 && bot_want == 0) bot_want = frames;
}

static int bot_buttons(void) {
    switch (state) {
    case S_TITLE: case S_ENDING: return (state_t / 3) % 2 ? BTN_A : 0;
    case S_INTRO: case S_OVER: bot_hold = bot_want = bot_run = bot_wait = 0; return 0; /* it stops at a game over */
    }
    int m = BTN_B, sx = (int)pl.x - cam_px();
    if (pl.spirit) {
        /* float to a roof in the left half, shooting what is there, and come
         * back once nothing nasty is near (or when time runs short) */
        int want_x = 100, want_y = 40;
        for (int tx = (cam_px() + 70) >> 4; tx < (cam_px() + 200) >> 4; tx++) {
            int top = -1;
            for (int r = 2; r < RC_ROWS; r++) if (floor_at(tx, r) && !floor_at(tx, r - 1)) { top = r; break; }
            if (top >= 0 && floor_at(tx + 1, top) && floor_at(tx + 2, top)) { want_x = tx * 16 - cam_px(); want_y = top * 16 - PH - 4; break; }
        }
        int age = SPIRIT_FRAMES - pl.spirit_t;
        if (sx < want_x - 2) m |= BTN_RIGHT;
        else if (sx > want_x + 2) m |= BTN_LEFT;
        if (age > 40) {
            if (pl.y < want_y - 2) m |= BTN_DOWN;
            else if (pl.y > want_y + 2) m |= BTN_UP;
        }
        bool safe = !danger_near(pl.x - 40, pl.x + 70, pl.y - 40, pl.y + 40) && !shot_near(pl.x - 40, pl.x + 70, pl.y - 40, pl.y + 40);
        bool there = abs(sx - want_x) < 6 && fabsf(pl.y - (float)want_y) < 6;
        if (age > 44 && there && (safe || pl.spirit_t < 30) && (frame_t % 2)) m |= BTN_A;
        return m;
    }
    int feet_row = ((int)pl.y + PH) >> 4;
    /* keep to the left half of the screen */
    if (sx < 36) m |= BTN_RIGHT;
    if (pl.ground) {
        bool gap = !floor_ahead(pl.x + PW + 8, feet_row) || !floor_ahead(pl.x + PW + 1, feet_row);
        bool wall = box_solid(pl.x + PW + 2, pl.y, 6, PH);
        int eta = threat_eta();
        bool close = (eta >= 0 && eta <= 14) || danger_near(pl.x - 2, pl.x + PW + 6, pl.y - 2, pl.y + PH + 2);
        /* the air a jump from here would pass through */
        bool air = shot_near(pl.x - 8, pl.x + 80, pl.y - 46, pl.y) || danger_near(pl.x + 16, pl.x + 80, pl.y - 46, pl.y - 6);
        if (close) bot_jump(12);
        else if (gap || wall) {
            if (air && sx > 64 && bot_wait < 40) { bot_wait++; m |= BTN_LEFT; m &= ~BTN_RIGHT; } /* hold on a moment */
            else {
                /* run just far enough: a jump alone drifts about 18 px with the scroll */
                int w = wall ? 16 : gap_width(pl.x + PW + 1, feet_row);
                bot_jump(16);
                bot_run = iclamp((PW + 1 + w + 8 - 18) * 10 / 13, 4, 50);
                bot_wait = 0;
            }
        } else {
            bot_wait = 0;
            bool ahead = foe_in_line(1, 150);
            if (sx > 170) bot_drift = true;
            if (sx < 110) bot_drift = false;
            if (foe_above_ahead() && !air) bot_jump(16); /* jump to throw at it */
            else if (!ahead && foe_in_line(-1, 110)) m |= BTN_LEFT; /* turn round to throw behind */
            else if (bot_drift && !ahead) m |= BTN_LEFT; /* drift back: room ahead to take a run at gaps */
        }
    } else {
        /* in the air: steer onto something, double jump if nothing is under */
        bool under = false;
        for (int dx = 0; dx <= 24 && !under; dx += 8)
            for (int r = feet_row; r < RC_ROWS && !under; r++) {
                char t = tile_at(((int)pl.x + dx) >> 4, r);
                if (t == '^' || t == '~') break;
                if (floor_at(((int)pl.x + dx) >> 4, r)) under = true;
            }
        for (int i = 0; i < MAX_ENTS && !under; i++)
            if (ents[i].alive && ents[i].type == E_LEG && ents[i].y < 140 && pl.x + PW > ents[i].x - 16 && pl.x < ents[i].x + ents[i].w + 16) under = true;
        if (!under) m |= BTN_RIGHT;
        if (!under && pl.vy > 0.3f && pl.jumps < 2) bot_jump(16);
        { int eta = threat_eta(); if (eta >= 0 && eta <= 10 && pl.jumps < 2) bot_jump(12); }
        /* a wall taller than one jump: the second jump near the top */
        if (box_solid(pl.x + PW + 1, pl.y, 8, PH) && pl.vy > -0.8f && pl.jumps < 2) { bot_jump(16); bot_run = imax(bot_run, 20); }
    }
    if (bot_run > 0) { m |= BTN_RIGHT; m &= ~BTN_LEFT; bot_run--; }
    if (sx > 200 && pl.ground) m &= ~BTN_RIGHT;
    /* after holding back she faces left: turn round to throw ahead again */
    if (pl.face < 0 && !(m & BTN_LEFT)) m |= BTN_RIGHT;
    if (bot_hold > 0) {
        m |= BTN_A;
        bot_hold--;
    } else if (bot_want > 0 && !btn(BTN_A)) {
        /* a fresh press: A was up last frame */
        m |= BTN_A;
        bot_hold = bot_want - 1;
        bot_want = 0;
    }
    return m;
}

static int rc_query(const char *key, int *out) {
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "score")) { *out = (int)score; return 1; }
    if (!strcmp(key, "lives")) { *out = lives; return 1; }
    if (!strcmp(key, "power")) { *out = power; return 1; }
    if (!strcmp(key, "area")) { *out = cur_area; return 1; }
    if (!strcmp(key, "loop")) { *out = loop; return 1; }
    if (!strcmp(key, "spirit")) { *out = pl.spirit; return 1; }
    if (!strcmp(key, "px")) { *out = (int)pl.x; return 1; }
    if (!strcmp(key, "py")) { *out = (int)pl.y; return 1; }
    if (!strcmp(key, "screen_x")) { *out = (int)floorf(pl.x) - cam_px(); return 1; } /* as drawn */
    if (!strcmp(key, "jitter")) { *out = jit_rev; return 1; }
    if (!strcmp(key, "cam_bad")) { *out = jit_cam_bad; return 1; }
    if (!strcmp(key, "cam")) { *out = (int)cam_x; return 1; }
    if (!strcmp(key, "screen")) { *out = cam_px() / (RC_CHUNK_W * 16); return 1; }
    if (!strcmp(key, "grounded")) { *out = pl.ground; return 1; }
    if (!strcmp(key, "jumps")) { *out = pl.jumps; return 1; }
    if (!strcmp(key, "vy100")) { *out = (int)(pl.vy * 100); return 1; }
    if (!strcmp(key, "stars")) { *out = count_type(E_STAR); return 1; }
    if (!strcmp(key, "tokens")) { *out = count_type(E_TOKEN); return 1; }
    if (!strcmp(key, "foes")) { *out = count_foes(); return 1; }
    if (!strcmp(key, "kills")) { *out = kills_run; return 1; }
    if (!strcmp(key, "letters")) { *out = letters_this_run; return 1; }
    if (!strcmp(key, "letter_items")) { *out = count_type(E_LETTER); return 1; }
    if (!strcmp(key, "snacks")) { *out = snacks_total; return 1; }
    if (!strcmp(key, "bonus0")) { *out = bonus_got[0]; return 1; }
    if (!strcmp(key, "bonus1")) { *out = bonus_got[1]; return 1; }
    if (!strcmp(key, "snacks_left")) {
        int n = 0;
        for (int y = 0; y < RC_ROWS; y++)
            for (int x = 0; x < MAX_COLS; x++)
                for (int b = 0; b < 4; b++) n += (snackm[y][x] >> b) & 1;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "crowns")) { *out = count_type(E_CROWN); return 1; }
    if (!strcmp(key, "in_arena")) { *out = in_arena; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = boss_i >= 0 && ents[boss_i].alive ? ents[boss_i].hp : 0; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = boss_dead; return 1; }
    if (!strcmp(key, "lanterns")) { *out = count_type(E_LANTERN); return 1; }
    if (!strcmp(key, "next_life")) { *out = (int)next_life_at; return 1; }
    if (!strcmp(key, "world_len")) { *out = RC_WORLD_CHUNKS; return 1; }
    if (!strcmp(key, "hi0")) { *out = (int)sv.hi[0]; return 1; }
    if (!strcmp(key, "hi1")) { *out = (int)sv.hi[1]; return 1; }
    if (!strcmp(key, "hi2")) { *out = (int)sv.hi[2]; return 1; }
    if (!strcmp(key, "hi3")) { *out = (int)sv.hi[3]; return 1; }
    if (!strcmp(key, "hi4")) { *out = (int)sv.hi[4]; return 1; }
    if (!strcmp(key, "bomb_shards")) { *out = bomb_shards; return 1; }
    if (!strcmp(key, "spirit_shots")) { *out = count_type(E_SPIRIT_SHOT); return 1; }
    if (!strcmp(key, "snack_value0")) { *out = snack_value(0); return 1; }
    if (!strcmp(key, "snack_value1")) { *out = snack_value(1); return 1; }
    if (!strcmp(key, "snack_points_ok")) { *out = snack_points == 5 * snacks_total; return 1; }
    if (!strcmp(key, "scroll_seconds")) { *out = (int)(arena_x / SCROLL / 60.0f); return 1; }
    if (!strcmp(key, "first_cracker_screen")) {
        *out = -1;
        for (int c = 0; c < RC_WORLD_CHUNKS && *out < 0; c++)
            for (int y = 0; y < RC_ROWS; y++) if (strchr(RC_WORLD[c].rows[y], 'x')) { *out = c; break; }
        return 1;
    }
    if (!strcmp(key, "new_rank")) { *out = new_rank; return 1; }
    if (!strcmp(key, "most_foes")) { *out = sv.most_foes; return 1; }
    if (!strcmp(key, "most_letters")) { *out = sv.most_letters; return 1; }
    if (!strcmp(key, "over_in_bonus")) { *out = over_in_bonus; return 1; }
    if (!strcmp(key, "killer")) { *out = last_killer; return 1; }
    if (!strcmp(key, "bot")) { *out = bot_buttons(); return 1; }
    if (!strcmp(key, "eta")) { *out = threat_eta(); return 1; }
    if (!strcmp(key, "secrets")) {
        int n = 0;
        for (int c = 0; c < RC_WORLD_CHUNKS; c++)
            for (int y = 0; y < RC_ROWS; y++) n += strchr(RC_WORLD[c].rows[y], '!') != NULL;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "placed_lanterns")) {
        int n = 0;
        for (int c = 0; c < RC_WORLD_CHUNKS; c++)
            for (int y = 0; y < RC_ROWS; y++) n += strchr(RC_WORLD[c].rows[y], '1') != NULL;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "placed_foes")) {
        /* foes written into the level (a measure of how busy it is) */
        int n = 0;
        for (int c = 0; c < RC_WORLD_CHUNKS; c++)
            for (int y = 0; y < RC_ROWS; y++)
                for (const char *p = RC_WORLD[c].rows[y]; *p; p++) n += strchr("pPcguLKfJantejxsqF", *p) != NULL;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "bonus_snacks")) {
        /* snacks in the first bonus stretch */
        int n = 0;
        for (int c = RC_BONUS_CHUNK[0]; c < RC_BONUS_CHUNK[0] + RC_BONUS_LEN; c++)
            for (int y = 0; y < RC_ROWS; y++)
                for (const char *p = RC_WORLD[c].rows[y]; *p; p++) n += (*p == '*') * 4;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "stray_snacks")) {
        /* snacks outside the bonus stretches (there should be none) */
        int n = 0;
        for (int c = 0; c < RC_WORLD_CHUNKS; c++) {
            bool bonus = false;
            for (int g = 0; g < RC_BONUS_AREAS; g++) bonus |= c >= RC_BONUS_CHUNK[g] && c < RC_BONUS_CHUNK[g] + RC_BONUS_LEN;
            for (int y = 0; y < RC_ROWS && !bonus; y++) n += strchr(RC_WORLD[c].rows[y], '*') != NULL;
        }
        *out = n;
        return 1;
    }
    if (!strncmp(key, "all_", 4)) { /* awake or waiting */
        int type = foe_type_of(key + 4), n = 0;
        for (int i = 0; i < MAX_ENTS; i++) n += ents[i].alive && ents[i].type == type && type;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "wx_", 3)) {
        Ent *e = first_of(foe_type_of(key + 3));
        *out = e ? (int)e->x : -999;
        return 1;
    }
    if (!strncmp(key, "egg_", 4)) { /* what a freshly placed foe of this kind drops */
        Ent tmp;
        memset(&tmp, 0, sizeof tmp);
        tmp.type = (uint8_t)foe_type_of(key + 4);
        foe_setup(&tmp);
        *out = tmp.value;
        return 1;
    }
    if (!strncmp(key, "spikes_", 7)) {
        Ent *e = first_of(foe_type_of(key + 7));
        *out = e ? e->n : -1;
        return 1;
    }
    if (!strncmp(key, "count_", 6)) {
        int type = foe_type_of(key + 6), n = 0;
        for (int i = 0; i < MAX_ENTS; i++) n += ents[i].alive && ents[i].type == type && type && ents[i].state != -1;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "hp_", 3)) {
        Ent *e = first_of(foe_type_of(key + 3));
        *out = e ? e->hp : -1;
        return 1;
    }
    if (!strncmp(key, "sub_", 4)) {
        Ent *e = first_of(foe_type_of(key + 4));
        *out = e ? e->sub : -1;
        return 1;
    }
    if (!strncmp(key, "mode_", 5)) {
        Ent *e = first_of(foe_type_of(key + 5));
        *out = e ? e->mode : -1;
        return 1;
    }
    if (!strncmp(key, "x_", 2)) { /* screen x of the first such thing */
        Ent *e = first_of(foe_type_of(key + 2));
        *out = e ? (int)e->x - cam_px() : -999;
        return 1;
    }
    if (!strncmp(key, "y_", 2)) {
        Ent *e = first_of(foe_type_of(key + 2));
        *out = e ? (int)e->y : -999;
        return 1;
    }
    if (!strcmp(key, "token_value")) {
        Ent *e = first_of(E_TOKEN);
        *out = e ? e->value : 0;
        return 1;
    }
    if (!strcmp(key, "chunk_errors")) {
        int errs = 0;
        for (int c = 0; c < RC_WORLD_CHUNKS; c++) {
            const Chunk *ch = &RC_WORLD[c];
            for (int y = 0; y < RC_ROWS; y++) if ((int)strlen(ch->rows[y]) != RC_CHUNK_W) errs++;
            /* seamless joins: rooftop at row 6 on both edges, open sky above */
            if (!solid_c(ch->rows[6][0]) || !solid_c(ch->rows[6][RC_CHUNK_W - 1])) errs++;
            for (int y = 0; y < 6; y++)
                if (solid_c(ch->rows[y][0]) || solid_c(ch->rows[y][RC_CHUNK_W - 1])) errs++;
        }
        *out = errs;
        return 1;
    }
    return 0;
}

static int rc_cheat(const char *cmd) {
    int a, b;
    char name[32];
    if (sscanf(cmd, "area %d %d", &a, &b) == 2) {
        /* area N LOOP: start the chase at an area (4 = the harbour mole) */
        if (state == S_TITLE) new_run();
        loop = b;
        start_at(a);
        state = S_PLAY;
        return 1;
    }
    if (!strcmp(cmd, "play")) { if (state == S_INTRO) state = S_PLAY; return 1; }
    if (sscanf(cmd, "cam %d", &a) == 1) {
        cam_x = (float)a;
        spawn_ahead();
        pl.x = cam_x + 60;
        pl.y = 20;
        pl.vy = 0;
        for (int i = 0; i < MAX_ENTS; i++) if (ents[i].alive && ents[i].x < cam_x - 40 && ents[i].type != E_BOSS) ents[i].alive = false;
        return 1;
    }
    if (!strcmp(cmd, "arena")) {
        cam_x = arena_x - 1;
        spawn_ahead();
        pl.x = arena_x + 20;
        pl.y = FLOOR_Y - PH;
        return 1;
    }
    if (sscanf(cmd, "score %d", &a) == 1) { score = 0; next_life_at = 3000; add_score(a); return 1; }
    if (sscanf(cmd, "lives %d", &a) == 1) { lives = a; return 1; }
    if (sscanf(cmd, "power %d", &a) == 1) { power = a; return 1; }
    if (sscanf(cmd, "bonus %d %d", &a, &b) == 2) { if (a >= 0 && a < RC_BONUS_AREAS) bonus_got[a] = b; return 1; }
    if (sscanf(cmd, "boss_hp %d", &a) == 1) { if (boss_i >= 0) ents[boss_i].hp = a; return 1; }
    if (!strcmp(cmd, "clear_foes")) {
        for (int i = 0; i < MAX_ENTS; i++)
            if (ents[i].alive && (IS_FOE(ents[i].type) || ents[i].type == E_LANTERN)) ents[i].alive = false;
        return 1;
    }
    if (sscanf(cmd, "put %d %d", &a, &b) == 2) { pl.x = cam_x + a; pl.y = (float)b; pl.vy = 0; return 1; }
    if (sscanf(cmd, "foe %31s %d %d", name, &a, &b) == 3) {
        /* foe NAME X Y: drop a foe at a screen position */
        int type = foe_type_of(name);
        if (!type || !IS_FOE(type)) return 0;
        Ent *e = spawn(type, cam_x + a, (float)b);
        if (e) { foe_setup(e); e->state = 0; }
        return 1;
    }
    if (sscanf(cmd, "dormant %31s %d %d", name, &a, &b) == 3) {
        /* dormant NAME X Y: a foe as the level places it, waiting to wake */
        int type = foe_type_of(name);
        if (!type || !IS_FOE(type)) return 0;
        Ent *e = spawn(type, cam_x + a, (float)b);
        if (e) {
            foe_setup(e);
            if (type == E_SNAIL && solid_at(((int)cam_x + a) >> 4, (b >> 4) - 1) && !floor_at(((int)cam_x + a) >> 4, (b >> 4) + 1)) {
                e->mode = M_CEILING; e->y = (float)b; e->hy = e->y;
            }
            e->state = -1;
        }
        return 1;
    }
    if (sscanf(cmd, "mode %31s %d", name, &a) == 2) {
        Ent *e = first_of(foe_type_of(name));
        if (e) e->mode = a;
        return 1;
    }
    if (sscanf(cmd, "snack %d %d", &a, &b) == 2) {
        /* snack X Y: a block of four snacks on the tile at that screen spot */
        int tx = ((int)cam_x + a) >> 4, ty = b >> 4;
        if (tx >= 0 && tx < MAX_COLS && ty >= 0 && ty < RC_ROWS) snackm[ty][tx] = 15;
        return 1;
    }
    if (sscanf(cmd, "spot %d %d", &a, &b) == 2) {
        Ent *s = spawn(E_SPOT, cam_x + a, (float)b);
        if (s) { s->w = 16; s->h = 16; }
        return 1;
    }
    if (sscanf(cmd, "lantern %d %d", &a, &b) == 2) {
        Ent *l = spawn(E_LANTERN, cam_x + a, (float)b);
        if (l) { l->w = 8; l->h = 12; l->state = 0; }
        return 1;
    }
    if (sscanf(cmd, "roll %d", &a) == 1) {
        /* roll DIR: the pigeons tuck in and roll that way */
        for (int i = 0; i < MAX_ENTS; i++)
            if (ents[i].alive && (ents[i].type == E_PIGEON || ents[i].type == E_RPIGEON)) { ents[i].sub = 60; ents[i].dir = a; }
        return 1;
    }
    if (sscanf(cmd, "orb %d %d", &a, &b) == 2) {
        Ent *o = spawn(E_ORB, cam_x + a, (float)b);
        if (o) { o->w = 8; o->h = 8; o->vy = 0.6f; }
        return 1;
    }
    if (sscanf(cmd, "hops %d", &a) == 1) {
        for (int i = 0; i < MAX_ENTS; i++) if (ents[i].alive && ents[i].type == E_JAR) ents[i].n = a;
        return 1;
    }
    if (!strcmp(cmd, "old_save")) {
        /* write a save in the first format (with a checkpoint) for the upgrade test */
        SaveV1 old;
        memset(&old, 0, sizeof old);
        old.magic = SAVE_MAGIC_V1;
        old.best_score = 12345;
        old.cp_valid = 1; old.cp_area = 2; old.cp_lives = 5; old.cp_score = 4000;
        game_save_write(game_current_index(), &old, (int)sizeof old);
        load_save(); /* as if the cartridge had just been started */
        return 1;
    }
    if (!strcmp(cmd, "kill")) { lose_life(); return 1; }
    if (!strcmp(cmd, "jitter_reset")) { jit_valid = false; jit_last_d = 0; jit_rev = 0; jit_cam_bad = 0; return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    if (!strcmp(cmd, "win")) {
        if (boss_i >= 0) { ents[boss_i].hp = 1; ents[boss_i].flash = 0; boss_hit(&ents[boss_i]); ents[boss_i].t = 179; }
        return 1;
    }
    return 0;
}

const GameDef GAME_ROOFCAT = {
    "roofcat",
    "ROOFCAT",
    "1984",
    "ACTION",
    "PEPPER THE CAT CHASES PARCEL THIEVES OVER THE ROOFTOPS.",
    {"FIND 3 LOST LETTERS IN A RUN", "DELIVER THE PARCEL", "CLEAR THE NIGHT ROUTE"},
    "RIGHT\tRUN AHEAD\n"
    "LEFT\tHOLD YOUR GROUND\n"
    GLYPH_A "\tJUMP, AGAIN IN THE AIR\n"
    "HOLD " GLYPH_A "\tJUMP HIGHER\n"
    "DOWN+" GLYPH_A "\tDROP THROUGH LEDGES\n"
    GLYPH_B "\tTHROW (HOLD TO KEEP THROWING)\n"
    "START\tPAUSE\n\n"
    "SPIRIT: FLY, " GLYPH_B " SHOOT, " GLYPH_A " RETURN\n"
    "SHOOT A PAPER LANTERN FOR 1UP.",
    C_ORANGE, C_SKY,
    rc_load, rc_start, rc_update, rc_draw, rc_quit, rc_label, rc_query, rc_cheat,
    "NINPEK", 3,
};
