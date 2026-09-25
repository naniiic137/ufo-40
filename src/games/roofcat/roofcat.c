/* ROOFCAT - an auto-scrolling rooftop chase. Cartridge 03 of UFO 40,
 * a tribute to Ninpek (UFO 50 #3). See docs/games/03-roofcat.md.
 *
 * One continuous town scrolls past in four areas (with two bonus stretches)
 * and ends at the harbour, where Old Crab guards the stolen parcel. */
#include "roofcat.h"

#define SCROLL 0.5f
#define PW 10
#define PH 13
#define GRAV 0.19f
#define JUMP1 (-3.55f)
#define JUMP2 (-3.2f)
#define RUN 1.3f
#define MAX_COLS (RC_WORLD_CHUNKS * RC_CHUNK_W)
#define SPIRIT_FRAMES 240
#define STAR_RANGE 112.0f
#define THROW_CD 9      /* frames between stars while B is held */
#define SPIRIT_CD 22    /* the spirit's twin shots come much slower */
#define START_LIVES 3
#define BOSS_HP 35
#define FLOOR_Y 96      /* the rooftop line (row 6) */

enum { S_TITLE, S_INTRO, S_PLAY, S_OVER, S_ENDING };

enum {
    E_NONE,
    /* foes */
    E_PIGEON, E_RPIGEON, E_CROW, E_GECKO, E_GULL, E_LAMP, E_FFISH, E_WASP, E_SNAIL, E_TOAD, E_PELICAN,
    E_JAR, E_CRACKER, E_PUFFER, E_SPIDER, E_MAGPIE, E_FLASHER, E_SHEET, E_MOTH,
    E_BOSS, E_LEG,
    /* shots */
    E_STAR, E_SPIRIT_SHOT, E_PEBBLE, E_SEED, E_BUBBLE, E_BOMB, E_SHRAP, E_BEAM, E_DUST, E_ORB, E_ORBHALF, E_BLAST,
    /* pickups */
    E_TOKEN, E_SNACK, E_LETTER, E_CROWN, E_LANTERN, E_CATNIP,
    /* fx and markers */
    E_TEXT, E_DEBRIS, E_SPOT
};
#define IS_FOE(t) ((t) >= E_PIGEON && (t) <= E_MOTH)

typedef struct Ent {
    uint8_t type;
    bool alive;
    float x, y, vx, vy;
    int w, h, hp, t, cd, state, dir, flash, value, sub, n;
    float hx, hy;
} Ent;

#define MAX_ENTS 180
static Ent ents[MAX_ENTS];

typedef struct Save {
    uint32_t magic;
    uint32_t best_score;
    uint8_t cp_valid, cp_loop, cp_area, cp_lives;
    uint32_t cp_score;
    uint16_t cp_snacks;
    uint8_t cp_letters, pad;
} Save;
#define SAVE_MAGIC 0x52430003u
static Save sv;

static char map[RC_ROWS][MAX_COLS + 1];
static int state, state_t, title_sel, frame_t, shake;
static int loop;
static uint32_t score, next_life_at;
static int lives, power, kills_since_power, letters_this_run, snacks;
static float cam_x, arena_x;
static int spawned_chunks, cur_area;
static bool in_arena, boss_dead;
static int boss_i = -1;
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
    bool ground;
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
static int area_of_col(int tx) { return iclamp(tx / (RC_CHUNK_W * RC_AREA_CHUNKS), 0, RC_AREAS - 1); }

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
    /* things become entities when their screen comes near; the tile is air */
    for (int y = 0; y < RC_ROWS; y++)
        for (int x = 0; x < MAX_COLS; x++)
            if (!solid_c(map[y][x]) && map[y][x] != '=' && map[y][x] != '^' && map[y][x] != '~' && map[y][x] != 'T')
                map[y][x] = '.';
    arena_x = (float)((RC_WORLD_CHUNKS - 1) * RC_CHUNK_W * 16);
}

/* foe stats: hit points and the fish token they drop (0 = points on the spot) */
static void foe_setup(Ent *e) {
    switch (e->type) {
    case E_PIGEON: e->w = 12; e->h = 11; e->x += 2; e->y += 5; e->hp = 2; e->value = 100; break;
    case E_RPIGEON: e->w = 12; e->h = 11; e->x += 2; e->y += 5; e->hp = 2; e->value = 200; break;
    case E_CROW: e->w = 12; e->h = 12; e->x += 2; e->y += 4; e->hp = 4; e->value = 200; e->cd = 60; break;
    case E_GECKO: e->w = 12; e->h = 7; e->x += 2; e->y += 9; e->hp = 1; e->value = 100; e->cd = 40; break;
    case E_GULL: e->w = 14; e->h = 8; e->y += 3; e->hp = 2; e->value = 100; break;
    case E_LAMP: e->w = 10; e->h = 12; e->x += 3; e->y += 2; e->hp = 10; e->value = 0; e->n = loop >= 2 ? 3 : 2; break;
    case E_FFISH: e->w = 10; e->h = 8; e->x += 3; e->y += 4; e->hp = 1; e->value = 200; e->cd = 40; break;
    case E_WASP: e->w = 8; e->h = 7; e->x += 4; e->y += 4; e->hp = 1; e->value = 100; break;
    case E_SNAIL:
        e->w = 12; e->h = 8; e->x += 2; e->y += 8;
        e->hp = loop >= 2 ? 2 : 1; e->value = loop >= 2 ? 200 : 100; e->sub = loop >= 2;
        break;
    case E_TOAD: e->w = 14; e->h = 11; e->x += 1; e->y += 5; e->hp = 4; e->value = 300; e->cd = 50; break;
    case E_PELICAN: e->w = 14; e->h = 10; e->y += 3; e->hp = 2; e->value = 200; break;
    case E_JAR: e->w = 10; e->h = 11; e->x += 3; e->y += 5; e->hp = 2; e->value = 200; e->cd = 30; break;
    case E_CRACKER: e->w = 10; e->h = 13; e->x += 3; e->y += 3; e->hp = 8; e->value = 0; break;
    case E_PUFFER: e->w = 11; e->h = 10; e->x += 2; e->y += 2; e->hp = 1; e->value = 200; e->cd = 50; break;
    case E_SPIDER: e->w = 9; e->h = 7; e->x += 3; e->y += 0; e->hp = 1; e->value = 100; break;
    case E_MAGPIE: e->w = 12; e->h = 9; e->x += 2; e->y += 2; e->hp = 1; e->value = 200; break;
    case E_FLASHER: e->w = 12; e->h = 13; e->x += 2; e->y += 3; e->hp = 2; e->value = 300; e->cd = 90; break;
    case E_SHEET: e->w = 12; e->h = 12; e->x += 2; e->y += 2; e->hp = 2; e->value = 100; break;
    case E_MOTH: e->w = 10; e->h = 8; e->x += 3; e->y += 4; e->hp = 2; e->value = 200; break;
    }
    e->hx = e->x;
    e->hy = e->y;
}

static void spawn_chunk(int c) {
    int fishes = 0;
    for (int y = 0; y < RC_ROWS; y++)
        for (int x = 0; x < RC_CHUNK_W; x++) {
            char t = RC_WORLD[c].rows[y][x];
            float wx = (float)((c * RC_CHUNK_W + x) * 16), wy = (float)(y * 16);
            int type = 0;
            switch (t) {
            case 'p': type = E_PIGEON; break;
            case 'P': type = E_RPIGEON; break;
            case 'c': type = E_CROW; break;
            case 'g': type = E_GECKO; break;
            case 'u': type = E_GULL; break;
            case 'L': type = E_LAMP; break;
            case 'f':
                /* on the second loop most flying fish are pufferfish instead */
                type = (loop >= 2 && (fishes++ % 3) != 0) ? E_PUFFER : E_FFISH;
                break;
            case 'a': type = E_WASP; break;
            case 'n': type = E_SNAIL; break;
            case 't': type = E_TOAD; break;
            case 'e': type = E_PELICAN; break;
            case 'j': type = E_JAR; break;
            case 'x': type = E_CRACKER; break;
            case 'z': type = E_PUFFER; break;
            case 's': type = E_SPIDER; break;
            case 'q': type = E_MAGPIE; break;
            case 'F': type = E_FLASHER; break;
            case 'h': if (loop >= 2) type = E_SHEET; break;
            case 'o': if (loop >= 2) type = E_MOTH; break;
            case '*': {
                Ent *s = spawn(E_SNACK, wx + 4, wy + 5);
                if (s) { s->w = 8; s->h = 7; }
                break;
            }
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
                foe_setup(e);
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

static void checkpoint(int area) {
    sv.cp_valid = 1;
    sv.cp_loop = (uint8_t)loop;
    sv.cp_area = (uint8_t)area;
    sv.cp_lives = (uint8_t)lives;
    sv.cp_score = score;
    sv.cp_snacks = (uint16_t)snacks;
    sv.cp_letters = (uint8_t)letters_this_run;
    save_now();
}

/* Start (or resume) the chase at an area: 0-3, or 4 for the harbour mole. */
static void start_at(int area) {
    memset(ents, 0, sizeof ents);
    boss_i = -1;
    in_arena = false;
    boss_dead = false;
    carry_px = 0;
    jit_valid = false;
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
    checkpoint(area);
}

static void add_score(int v) {
    score += (uint32_t)v;
    if (score > sv.best_score) sv.best_score = score;
    while (score >= next_life_at) {
        /* a paper lantern drifts up: hit it for a life */
        Ent *l = spawn(E_LANTERN, cam_x + (float)rng_range(&rng, 140, 280), 170);
        if (l) { l->w = 8; l->h = 12; l->state = 0; }
        /* 3000, then 7000, then every 5000 */
        next_life_at = next_life_at == 3000 ? 7000 : next_life_at + 5000;
    }
}

static void new_run(void) {
    score = 0;
    lives = START_LIVES;
    snacks = 0;
    letters_this_run = 0;
    next_life_at = 3000;
    loop = 1;
    start_at(0);
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

static void throw_star(void) {
    Ent *s = spawn(E_STAR, pl.face > 0 ? pl.x + PW : pl.x - 8, pl.y + 3);
    if (s) { s->w = 8; s->h = 8; s->vx = 3.6f * pl.face; s->t = (int)(STAR_RANGE / 3.6f); }
    pl.throw_cd = THROW_CD;
    pl.throw_t = 8;
    sfx_play_name("rc_throw");
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
            if (s) { s->w = 6; s->h = 6; s->vx = 4.2f; s->t = 40; }
        }
        pl.throw_cd = SPIRIT_CD;
        sfx_play_name("rc_shoot");
    }
    pl.spirit_t--;
    if (pl.spirit_t <= 0 || (btnp(BTN_A) && age > 30)) revive();
}

static void update_player(void) {
    if (pl.spirit) { update_spirit(); return; }
    float c = carry();
    int dir = btn(BTN_RIGHT) - btn(BTN_LEFT);
    if (dir) pl.face = dir;
    if (pl.drop_t > 0) pl.drop_t--;
    if (pl.throw_cd > 0) pl.throw_cd--;
    if (pl.throw_t > 0) pl.throw_t--;
    /* horizontal: carried by the scroll, plus running */
    move_x(c + dir * RUN);
    /* pushed by the screen's left edge; squeezed against a wall, you're done */
    if (pl.x < cam_x) {
        if (box_solid(cam_x, pl.y, PW, PH)) { lose_life(); return; }
        pl.x = cam_x;
    }
    if (pl.x > cam_x + 320 - PW) pl.x = cam_x + 320 - PW;
    /* jumping */
    if (btnp(BTN_A)) {
        if (pl.ground && btn(BTN_DOWN) && pl.leg < 0) {
            int ty = ((int)pl.y + PH) >> 4;
            bool on_ledge = false;
            for (int tx = (int)pl.x >> 4; tx <= ((int)pl.x + PW - 1) >> 4; tx++) on_ledge |= oneway_at(tx, ty);
            if (on_ledge) { pl.drop_t = 10; pl.ground = false; pl.y += 1; }
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
    if (!btn(BTN_A) && pl.vy < -1.2f) pl.vy = -1.2f; /* short hop when released */
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
    pl.anim++;
    /* hazards */
    int x0 = ((int)pl.x + 2) >> 4, x1 = ((int)pl.x + PW - 3) >> 4;
    for (int tx = x0; tx <= x1; tx++) {
        char t = tile_at(tx, ((int)pl.y + PH - 2) >> 4);
        if (t == '^' || t == '~') { lose_life(); return; }
    }
    if (pl.y > 164) lose_life();
}

/* ------------------------------------------------------------------ */
/* foes                                                                 */

static void drop_loot(const Ent *e) {
    if (e->value > 0) {
        Ent *f = spawn(E_TOKEN, e->x + e->w / 2 - 5, e->y);
        if (f) { f->w = 10; f->h = 6; f->vy = -2.4f; f->vx = 0.4f; f->value = e->value; f->state = 1; }
    } else {
        /* spike lamps and firecrackers pay out on the spot */
        add_score(200);
        float_text(e->x, e->y, 200);
    }
    /* every third foe leaves catnip, until Harissa holds two */
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
    drop_loot(&dead);
}

static bool hittable(const Ent *e) {
    if (IS_FOE(e->type)) {
        if (e->type == E_CROW) return e->sub == 1;            /* only when popped up */
        if (e->type == E_FFISH || e->type == E_PUFFER) return e->sub != 0; /* not under water */
        return true;
    }
    if (e->type == E_LANTERN) return e->state != -1;
    return false;
}

static void boss_hit(Ent *b);

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

static void enemy_shot(int type, float x, float y, float vx, float vy) {
    Ent *s = spawn(type, x, y);
    if (!s) return;
    s->vx = vx;
    s->vy = vy;
    s->w = type == E_BUBBLE ? 6 : 5;
    s->h = type == E_BUBBLE ? 6 : 5;
    s->t = 0;
}

static bool on_floor(const Ent *e, float x) {
    int foot = (int)(e->y + e->h);
    int tx = (int)x >> 4, ty = foot >> 4;
    return solid_at(tx, ty) || oneway_at(tx, ty);
}

/* walk along a platform, turning at edges and walls */
static void patrol(Ent *e, float speed) {
    float nx = e->x + speed * e->dir;
    float ahead = e->dir > 0 ? nx + e->w : nx - 1;
    if (box_solid(nx, e->y, e->w, e->h) || !on_floor(e, ahead)) e->dir = -e->dir;
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
        if (solid_at(tx, ty) || oneway_at(tx, ty)) return true;
    return false;
}

static void update_foe(Ent *e) {
    switch (e->type) {
    case E_PIGEON: case E_RPIGEON: {
        /* waddles to and fro; now and then tucks in and rolls */
        float walk = e->type == E_RPIGEON ? 1.3f : 0.5f;
        if (e->sub > 0) { e->sub--; patrol(e, walk * 3.0f); }
        else {
            patrol(e, walk);
            if (rng_range(&rng, 0, 179) == 0) e->sub = 30;
        }
        break;
    }
    case E_CROW:
        /* pops out of its chimney, turns to face Harissa, fires two pebbles */
        e->dir = pcx() < e->x + e->w / 2 ? -1 : 1;
        if (--e->cd <= 0) { e->sub ^= 1; e->cd = e->sub ? 100 : 80; }
        if (e->sub == 1 && (e->cd == 70 || e->cd == 50)) {
            enemy_shot(E_PEBBLE, e->x + e->w / 2 - 2, e->y + 4, 1.8f * e->dir, 0);
            sfx_play_name("rc_shoot");
        }
        break;
    case E_GECKO:
        /* spits a seed that bounces along the rooftops (two on the second loop) */
        e->dir = pcx() < e->x ? -1 : 1;
        if (--e->cd <= 0) {
            e->cd = 120;
            enemy_shot(E_SEED, e->x + (e->dir > 0 ? e->w : -4), e->y, 1.2f * e->dir, -2.0f);
            if (loop >= 2) enemy_shot(E_SEED, e->x + (e->dir > 0 ? e->w : -4), e->y, 0.7f * e->dir, -2.8f);
            sfx_play_name("rc_shoot");
        }
        break;
    case E_GULL:
        /* drifts slowly in from the right edge of the screen */
        e->x -= 0.8f;
        e->y = e->hy + sinf(e->t * 0.05f) * 6;
        break;
    case E_LAMP: break; /* the spikes orbit; see lamp_spike() */
    case E_FFISH:
        /* waits under the water, then leaps in a fixed arc */
        if (e->sub == 0) {
            if (--e->cd <= 0) { e->sub = 1; e->vy = -5.2f; e->vx = -0.9f; sfx_play_name("rc_splash"); }
        } else {
            e->vy += 0.14f;
            e->x += e->vx;
            e->y += e->vy;
            if (e->y > 170) e->alive = false;
        }
        break;
    case E_PUFFER:
        /* surfaces and puffs up to three bubbles that drift after Harissa */
        if (e->sub == 0) {
            if (--e->cd <= 0) { e->sub = 1; e->t = 0; e->cd = 1 << 30; }
        } else {
            if (e->y > e->hy - 18) e->y -= 0.6f;
            if (e->t % 70 == 35 && e->n < 3) {
                enemy_shot(E_BUBBLE, e->x + 3, e->y, 0, 0);
                e->n++;
                sfx_play_name("rc_shoot");
            }
            if (e->n >= 3 && e->t > 260) e->sub = 0; /* back under for good */
        }
        break;
    case E_WASP: {
        /* homes in, slowly at first, then faster */
        float sp = fminf(0.4f + e->t * 0.02f, 2.2f);
        float dx = pcx() - (e->x + 4), dy = pcy() - (e->y + 3), d = sqrtf(dx * dx + dy * dy) + 0.1f;
        e->vx += (dx / d * sp - e->vx) * 0.08f;
        e->vy += (dy / d * sp - e->vy) * 0.08f;
        e->x += e->vx;
        e->y += e->vy;
        break;
    }
    case E_SNAIL: patrol(e, 0.25f); break;
    case E_TOAD:
        /* sits, now and then hops, and lobs stones */
        if (grounded(e) && e->vy >= 0) {
            e->vx = 0;
            if (--e->cd <= 0) {
                e->cd = 90 + rng_range(&rng, 0, 40);
                e->dir = pcx() < e->x ? -1 : 1;
                if (rng_chance(&rng, 40)) { e->vy = -3.0f; e->vx = 0.6f * e->dir; }
                else {
                    float dx = pcx() - e->x;
                    enemy_shot(E_PEBBLE, e->x + 5, e->y, fclamp(dx / 50.0f, -2.4f, 2.4f) + SCROLL, -3.0f);
                    sfx_play_name("rc_shoot");
                }
            }
        }
        if (e->vx != 0 && !box_solid(e->x + e->vx, e->y, e->w, e->h)) e->x += e->vx;
        fall_with_gravity(e, 0.18f);
        break;
    case E_PELICAN:
        /* crosses fast from right to left; drops a bomb when over Harissa */
        e->x -= 2.2f;
        if (e->sub == 0 && fabsf(pcx() - (e->x + e->w / 2)) < 10 && pcy() > e->y) {
            e->sub = 1;
            enemy_shot(E_BOMB, e->x + 4, e->y + e->h, 0, 0.5f);
        }
        break;
    case E_JAR:
        /* small hops toward Harissa; after sixteen it turns into a crown */
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
                e->dir = pcx() < e->x ? -1 : 1;
                e->vy = -2.3f;
                e->vx = 0.8f * e->dir;
            }
        }
        if (e->vx != 0 && !box_solid(e->x + e->vx, e->y, e->w, e->h)) e->x += e->vx;
        fall_with_gravity(e, 0.18f);
        break;
    case E_CRACKER: {
        /* lights a short fuse when Harissa comes close, then goes off */
        float reach = loop >= 2 ? 96.0f : 64.0f;
        if (e->sub == 0 && fabsf(pcx() - (e->x + 5)) < reach) { e->sub = 1; e->t = 0; sfx_play_name("rc_fuse"); }
        if (e->sub == 1 && e->t >= (loop >= 2 ? 40 : 60)) {
            Ent *b = spawn(E_BLAST, e->x + 5 - 24, e->y + 6 - 24);
            if (b) { b->w = 48; b->h = 48; b->t = 16; }
            sfx_play_name("rc_boom");
            shake = 8;
            e->alive = false;
        }
        break;
    }
    case E_SPIDER:
        /* dangles under a ledge, bobbing up and down at random */
        if (e->t % 40 == 0) e->vy = (float)rng_range(&rng, -1, 1) * 0.8f;
        e->y += e->vy;
        if (e->y < e->hy) { e->y = e->hy; e->vy = 0.8f; }
        if (e->y > e->hy + 52) { e->y = e->hy + 52; e->vy = -0.8f; }
        break;
    case E_MAGPIE:
        /* locks onto Harissa and dives in a straight line */
        if (e->sub == 0) {
            if (fabsf(pcx() - e->x) < 110 && e->x < cam_x + 300) {
                e->sub = 1;
                float dx = pcx() - e->x, dy = pcy() - e->y, d = sqrtf(dx * dx + dy * dy) + 0.1f;
                e->vx = dx / d * 2.4f;
                e->vy = dy / d * 2.4f;
            }
        } else {
            e->x += e->vx;
            e->y += e->vy;
            if (e->y < -30 || e->y > 190) e->alive = false;
        }
        break;
    case E_FLASHER:
        /* flashes a mirror, then shoots one beam across the whole screen */
        e->dir = pcx() < e->x ? -1 : 1;
        if (--e->cd <= 0) {
            e->cd = 150;
            Ent *b = spawn(E_BEAM, e->x + (e->dir > 0 ? e->w : 0), e->y + 5);
            if (b) { b->w = 24; b->h = 3; b->vx = 7.0f * e->dir; b->t = 60; }
            sfx_play_name("rc_beam");
        }
        break;
    case E_SHEET:
        /* rises to Harissa's height, then sails across */
        if (e->sub == 0) {
            float dy = pcy() - (e->y + 6);
            e->y += fclamp(dy, -1.2f, 1.2f);
            if (fabsf(dy) < 2) { e->sub = 1; e->dir = pcx() < e->x ? -1 : 1; }
        } else {
            e->x += 1.6f * e->dir;
        }
        break;
    case E_MOTH:
        /* flutters about its perch, shedding clouds of dust */
        e->x = e->hx + sinf(e->t * 0.03f) * 26;
        e->y = e->hy + sinf(e->t * 0.07f) * 14;
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
        add_score(5000);
        float_text(EYE_X, EYE_Y, 5000);
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
        /* a burst of bubbles from the mouth, fanned at Harissa */
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
        /* a flying fish leaps between the legs */
        Ent *f = spawn(E_FFISH, arena_x + (float)rng_range(&rng, 80, 220), 150);
        if (f) { f->w = 10; f->h = 8; f->hp = 1; f->value = 200; f->sub = 1; f->vy = -5.0f; f->vx = rng_chance(&rng, 50) ? 0.8f : -0.8f; }
    }
}

/* ------------------------------------------------------------------ */
/* entity update                                                        */

static void update_ents(void) {
    for (int i = 0; i < MAX_ENTS; i++) {
        Ent *e = &ents[i];
        if (!e->alive) continue;
        if (e->flash > 0 && e->type != E_BOSS) e->flash--;
        /* dormant things wake when they scroll into view */
        if (e->state == -1) {
            if (e->x < cam_x + 330) {
                e->state = 0;
                if (e->type == E_GULL || e->type == E_PELICAN) e->x = cam_x + 330;
            } else continue;
        }
        /* things far behind the camera are gone */
        if (e->x + e->w < cam_x - 40 && e->type != E_BOSS && e->type != E_LEG) { e->alive = false; continue; }
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
                if (rects_overlap((int)e->x, (int)e->y, e->w, e->h, (int)o->x, (int)o->y, o->w, o->h)) {
                    hurt(o, 1);
                    e->alive = false;
                    break;
                }
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
            if (e->vy > 0 && box_solid(e->x, ny, e->w, e->h)) { e->vy = -2.4f; }
            else e->y = ny;
            if (box_solid(e->x, e->y, e->w, e->h)) e->alive = false;
            if (e->y > 175) e->alive = false;
            break;
        }
        case E_BUBBLE:
            e->t++;
            if (e->vx == 0 && e->vy == 0) {
                /* pufferfish bubbles drift after Harissa */
                float dx = pcx() - e->x, dy = pcy() - e->y, d = sqrtf(dx * dx + dy * dy) + 0.1f;
                e->x += dx / d * 0.7f + carry();
                e->y += dy / d * 0.7f + sinf(e->t * 0.1f) * 0.3f;
            } else {
                e->x += e->vx;
                e->y += e->vy;
            }
            if (e->y < -10 || e->y > 176 || e->t > 420) e->alive = false;
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
        case E_ORB:
            e->y += e->vy;
            if (e->y >= FLOOR_Y - 6) {
                /* splits into two halves that run left and right */
                e->alive = false;
                for (int k = -1; k <= 1; k += 2) enemy_shot(E_ORBHALF, e->x + 2, e->y, 1.6f * k, 0);
                sfx_play_name("rc_shoot");
            }
            break;
        case E_ORBHALF:
            e->x += e->vx;
            e->vy += 0.02f;
            e->y += e->vy;
            if (e->y > 176 || e->x < cam_x - 10 || e->x > cam_x + 330) e->alive = false;
            break;
        case E_BLAST:
            if (--e->t <= 0) e->alive = false;
            break;
        case E_TOKEN: case E_CATNIP: case E_LETTER: case E_CROWN:
            if (e->state == 1) {
                e->vy += 0.15f;
                e->x += e->vx * 0.3f;
                float ny = e->y + e->vy;
                int ty = (int)(ny + e->h) >> 4;
                bool land = e->vy > 0 && (solid_at(((int)e->x + 4) >> 4, ty) || oneway_at(((int)e->x + 4) >> 4, ty));
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

static int snack_value(void) { return snacks < 100 ? 5 : snacks < 200 ? 20 : 100; }

static void got_letter(Ent *e) {
    e->alive = false;
    add_score(500);
    letters_this_run++;
    float_text(e->x, e->y - 4, 500);
    sfx_play_name("rc_letter");
    if (letters_this_run >= 3) game_award(GOAL_BEACON);
}

static void contacts(void) {
    if (pl.dead_t) return;
    int px = (int)pl.x + 1, py = (int)pl.y + 2, pw = PW - 2, ph = PH - 3;
    for (int i = 0; i < MAX_ENTS; i++) {
        Ent *e = &ents[i];
        if (!e->alive || e->state == -1) continue;
        if (e->type == E_LAMP) {
            /* the lamp and its spikes */
            for (int k = 0; k < e->n && !pl.spirit; k++) {
                float sx, sy;
                lamp_spike(e, k, &sx, &sy);
                if (rects_overlap(px, py, pw, ph, (int)sx, (int)sy, 6, 6)) { lose_life(); return; }
            }
        }
        if (!rects_overlap(px, py, pw, ph, (int)e->x, (int)e->y, e->w, e->h)) continue;
        if (pl.spirit) continue; /* the spirit can't touch or be touched */
        switch (e->type) {
        case E_TOKEN: case E_CROWN:
            e->alive = false;
            add_score(e->value);
            float_text(e->x, e->y - 4, e->value);
            sfx_play_name(e->type == E_TOKEN ? "rc_coin" : "rc_letter");
            break;
        case E_SNACK: {
            int v = snack_value();
            e->alive = false;
            snacks++;
            add_score(v);
            float_text(e->x, e->y - 4, v);
            sfx_play_name("rc_food");
            break;
        }
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
        case E_LANTERN: case E_TEXT: case E_DEBRIS: case E_STAR: case E_SPIRIT_SHOT: case E_LEG:
            break;
        case E_CROW:
            if (e->sub == 1) lose_life();
            break;
        case E_FFISH: case E_PUFFER:
            if (e->sub != 0) lose_life();
            break;
        case E_BOSS:
            if (e->state != 99) lose_life();
            break;
        default:
            lose_life();
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
    /* crossing into a new area saves a checkpoint and changes the tune */
    int area = area_of_col((cam_px() + 160) >> 4);
    if (!in_arena && area != cur_area) {
        cur_area = area;
        music_play(RC_MUS_AREA[area]);
        checkpoint(area);
    }
    if (in_arena && sv.cp_area != RC_AREAS) checkpoint(RC_AREAS);
    if (pl.dead_t > 0) {
        if (++pl.dead_t > 80) {
            state = S_OVER;
            state_t = 0;
            sv.cp_valid = 0;
            save_now();
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
        if (loop >= 2) game_award(GOAL_ALIEN);
        sv.cp_valid = 0;
        save_now();
        music_restart(RC_MUS_END);
        game_set_pausable(false);
    }
}

static void title_update(void) {
    game_set_pausable(false);
    int n = sv.cp_valid ? 2 : 1;
    if (btnp(BTN_UP) || btnp(BTN_DOWN)) { title_sel = (title_sel + 1) % n; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) game_exit_to_library();
    if (btnp(BTN_A) || btnp(BTN_START)) {
        sfx_play_name("ui_ok");
        input_consume();
        if (sv.cp_valid && title_sel == 0) {
            score = sv.cp_score;
            lives = sv.cp_lives;
            loop = sv.cp_loop;
            snacks = sv.cp_snacks;
            letters_this_run = sv.cp_letters;
            next_life_at = 3000;
            while (next_life_at <= score) next_life_at = next_life_at == 3000 ? 7000 : next_life_at + 5000;
            start_at(sv.cp_area);
        } else {
            new_run();
        }
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
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) { state = S_TITLE; state_t = 0; title_sel = 0; music_play(RC_MUS_TITLE); }
        break;
    case S_ENDING:
        frame_t++;
        if (state_t > 240 && btnp(BTN_A)) {
            if (loop == 1) {
                /* round again after dark, keeping score and lives */
                loop = 2;
                start_at(0);
            } else {
                state = S_TITLE;
                state_t = 0;
                title_sel = 0;
                music_play(RC_MUS_TITLE);
            }
        } else if (state_t > 240 && btnp(BTN_B)) {
            state = S_TITLE;
            state_t = 0;
            title_sel = 0;
            music_play(RC_MUS_TITLE);
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
    /* spice souk, late afternoon */
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
        if (loop >= 2) { pal_identity(rm); rm[C_NIGHT] = C_PURPLE; rm[C_DUSK] = C_VIOLET; remap = rm; }
        if (e->sub == 1) spr_draw_ex(&rc_spr[e->cd > 40 && e->cd < 75 ? R_CROW2 : R_CROW1], x - 2, y - 4, fl, remap, solid);
        else { gfx_pset(x + 3, y + 10, C_YELLOW); gfx_pset(x + 8, y + 10, C_YELLOW); }
        break;
    }
    case E_GECKO: spr_draw_ex(&rc_spr[f ? R_GECKO1 : R_GECKO2], x - 2, y - 9, fl, NULL, solid); break;
    case E_GULL: spr_draw_ex(&rc_spr[(e->t / 6) % 2 ? R_GULL1 : R_GULL2], x - 1, y - 1, SPR_FLIPX, NULL, solid); break;
    case E_LAMP: {
        gfx_vline(x + 5, y - 16, y, C_INK);
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
    case E_PUFFER: if (e->sub != 0) spr_draw_ex(&rc_spr[e->t % 70 > 30 ? R_PUFFER2 : R_PUFFER1], x - 1, y - 1, fl, NULL, solid); break;
    case E_WASP: spr_draw_ex(&rc_spr[(e->t / 3) % 2 ? R_WASP1 : R_WASP2], x, y, e->vx > 0 ? SPR_FLIPX : 0, NULL, solid); break;
    case E_SNAIL: {
        const uint8_t *remap = NULL;
        if (e->sub) { pal_identity(rm); rm[C_RED] = C_JADE; rm[C_ORANGE] = C_LEAF; rm[C_WINE] = C_FOREST; remap = rm; }
        spr_draw_ex(&rc_spr[(e->t / 16) % 2 ? R_SNAIL1 : R_SNAIL2], x - 2, y - 4, fl, remap, solid);
        break;
    }
    case E_TOAD: spr_draw_ex(&rc_spr[e->vy != 0 ? R_TOAD2 : R_TOAD1], x - 1, y - 1, fl, NULL, solid); break;
    case E_PELICAN: spr_draw_ex(&rc_spr[(e->t / 6) % 2 ? R_PELICAN1 : R_PELICAN2], x - 1, y - 2, 0, NULL, solid); break;
    case E_JAR: spr_draw_ex(&rc_spr[e->vy != 0 ? R_JAR2 : R_JAR1], x - 1, y - 1, fl, NULL, solid); break;
    case E_CRACKER:
        spr_draw_ex(&rc_spr[e->sub && (e->t / 3) % 2 ? R_CRACKER2 : R_CRACKER1], x - 1, y - 1, 0, NULL, solid);
        if (e->sub) gfx_pset(x + 5 + rng_range(&rng, -1, 1), y - 3, (e->t / 2) % 2 ? C_YELLOW : C_WHITE);
        break;
    case E_SPIDER:
        gfx_vline(x + 4, (int)e->hy - 2, y, C_LIGHT);
        spr_draw_ex(&rc_spr[f ? R_SPIDER1 : R_SPIDER2], x - 1, y, 0, NULL, solid);
        break;
    case E_MAGPIE: spr_draw_ex(&rc_spr[(e->t / 4) % 2 ? R_MAGPIE1 : R_MAGPIE2], x - 2, y - 2, e->vx < 0 ? SPR_FLIPX : 0, NULL, solid); break;
    case E_FLASHER:
        spr_draw_ex(&rc_spr[e->cd < 30 && (e->cd / 3) % 2 ? R_FLASHER2 : R_FLASHER1], x - 2, y - 3, fl, NULL, solid);
        if (e->cd < 30 && (e->cd / 3) % 2) gfx_dither_circle(x + (e->dir > 0 ? 12 : 0), y + 5, 6, C_WHITE, 8);
        break;
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
    case E_BUBBLE: spr_draw(&rc_spr[R_BUBBLE], x, y, 0); break;
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
    case E_BLAST:
        gfx_circ(x + e->w / 2, y + e->h / 2, e->w / 2 - (16 - e->t) / 3, (e->t / 2) % 2 ? C_YELLOW : C_ORANGE);
        gfx_circ(x + e->w / 2, y + e->h / 2, e->w / 4, C_CREAM);
        break;
    case E_TOKEN: {
        pal_identity(rm);
        /* green, silver or violet fish: 100, 200 or 300 */
        if (e->value == 100) { rm[C_SKY] = C_LEAF; rm[C_BLUE] = C_JADE; rm[C_CYAN] = C_LIME; }
        else if (e->value >= 300) { rm[C_SKY] = C_VIOLET; rm[C_BLUE] = C_PURPLE; rm[C_CYAN] = C_MAGENTA; }
        else { rm[C_SKY] = C_LIGHT; rm[C_BLUE] = C_GREY; rm[C_CYAN] = C_WHITE; }
        spr_draw_ex(&rc_spr[R_FISH], x, y + ((e->state != 1 && (frame_t / 12) % 2) ? 1 : 0), 0, rm, -1);
        break;
    }
    case E_SNACK: {
        int spr = snacks < 100 ? R_DATE : snacks < 200 ? R_FIG : R_TEA;
        spr_draw(&rc_spr[spr], x, y + ((frame_t / 12 + x / 16) % 2), 0);
        break;
    }
    case E_CROWN: spr_draw(&rc_spr[R_CROWN], x, y + ((frame_t / 10) % 2), 0); break;
    case E_LETTER:
        gfx_dither_circle(x + 5, y + 4, 9, C_YELLOW, 4);
        spr_draw(&rc_spr[R_LETTER], x, y + ((frame_t / 10) % 2), 0);
        break;
    case E_LANTERN: spr_draw(&rc_spr[R_LANTERN], x, y + (e->state == -1 ? 0 : 0), 0); break;
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
    else {
        static const int run[4] = {R_CAT_RUN1, R_CAT_RUN2, R_CAT_RUN3, R_CAT_RUN4};
        spr = run[(pl.anim / 5) % 4];
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
    for (int i = 0; i < MAX_ENTS; i++)
        if (ents[i].alive && ents[i].state != -1 && ents[i].type != E_LEG) draw_ent(&ents[i]);
    draw_player();
    gfx_camera(0, 0);
    gfx_noclip();
    draw_hud();
}

static void draw_title(void) {
    cam_x = (float)((frame_t / 2) % 2000);
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
    spr_draw_scaled(&rc_spr[run[(frame_t / 5) % 4]], 60, 108, 2, 0);
    spr_draw(&rc_spr[(frame_t / 4) % 2 ? R_MAGPIE1 : R_MAGPIE2], 230 + (int)(sinf(frame_t * 0.05f) * 10), 90, 0);
    spr_draw(&rc_spr[R_PARCEL], 244 + (int)(sinf(frame_t * 0.05f) * 10), 104, 0);
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE, C_RED};
    ui_fancy_center("ROOFCAT", 160, 20, 4, grad, 4, C_INK, C_WINE);
    text_center("CATCH THE MAGPIE MOB!", 160, 54, C_NAVY);
    const char *items[2];
    int n = 0;
    char cont[40];
    if (sv.cp_valid) {
        snprintf(cont, sizeof cont, "CONTINUE %s", sv.cp_loop >= 2 ? "NIGHT" : "DAY");
        items[n++] = cont;
    }
    items[n++] = "NEW RUN";
    ui_panel(94, 66, 132, 8 + n * 12, C_NAVY, C_WHITE);
    for (int i = 0; i < n; i++) {
        int y = 70 + i * 12;
        bool s = i == title_sel;
        text_center(items[i], 160, y, s ? C_WHITE : C_SKY);
        if (s) ui_cursor(160 - text_width(items[i]) / 2 - 10, y, frame_t);
    }
    char buf[48];
    snprintf(buf, sizeof buf, "HI %07u", (unsigned)sv.best_score);
    gfx_rect(0, 170, 320, 10, C_INK);
    tiny_draw(buf, 6, 172, C_YELLOW);
    tiny_draw("A JUMP x2   B THROW   DOWN+A DROP   B: LIBRARY", 84, 172, C_LIGHT);
}

static void draw_intro(void) {
    draw_play();
    int t = state_t;
    int y = t < 16 ? -30 + t * 3 : t > 56 ? 18 - (t - 56) * 3 : 18;
    ui_panel(70, y + 30, 180, 34, C_INK, loop >= 2 ? C_VIOLET : C_YELLOW);
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center(loop >= 2 ? "NIGHT ROUTE" : "DAY ROUTE", 160, y + 35, 1, grad, 3, C_INK, -1);
    text_center(sv.cp_area >= RC_AREAS ? "OLD CRAB'S WATER" : RC_AREA_NAME[iclamp(sv.cp_area, 0, RC_AREAS - 1)], 160, y + 49, C_WHITE);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    static const uint8_t grad[] = {C_LIGHT, C_GREY, C_SLATE};
    ui_fancy_center("GAME OVER", 160, 50, 3, grad, 3, C_INK, C_NIGHT);
    spr_draw_scaled(&rc_spr[R_CAT_HURT], 144, 86, 2, 0);
    char buf[64];
    snprintf(buf, sizeof buf, "SCORE %07u   HI %07u", (unsigned)score, (unsigned)sv.best_score);
    text_center(buf, 160, 128, C_GREY);
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 150, C_YELLOW);
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
    /* the town and Grandma Zohra's house */
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
    /* Harissa runs in with the parcel */
    int cx = -20 + imin(t * 2, 200);
    static const int run[4] = {R_CAT_RUN1, R_CAT_RUN2, R_CAT_RUN3, R_CAT_RUN4};
    bool arrived = t * 2 >= 220;
    spr_draw_scaled(&rc_spr[arrived ? R_CAT_IDLE : run[(t / 4) % 4]], cx, 110, 2, 0);
    if (arrived) {
        /* the second loop brings home a whole stack of parcels */
        int stack = night ? 3 : 1;
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
    text_center(night ? "EVEN THE MAGPIE MOB SLEEPS NOW." : "HAPPY BIRTHDAY, GRANDMA ZOHRA!", 160, 46, night ? C_VIOLET : C_YELLOW);
    if (t > 240 && (t / 20) % 2) {
        gfx_rect(60, 160, 200, 12, C_INK);
        text_center(night ? GLYPH_A " TITLE" : GLYPH_A " NIGHT ROUTE    B TITLE", 160, 162, C_WHITE);
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
    Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else { memset(&sv, 0, sizeof sv); sv.magic = SAVE_MAGIC; }
    state = S_TITLE;
    state_t = 0;
    title_sel = 0;
    sheet_mode = false;
    loop = 1;
    memset(ents, 0, sizeof ents);
    game_set_pausable(false);
    music_play(RC_MUS_TITLE);
}

static void rc_quit(void) { save_now(); }

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

static int foe_type_of(const char *name) {
    static const struct { const char *n; int t; } T[] = {
        {"pigeon", E_PIGEON}, {"rpigeon", E_RPIGEON}, {"crow", E_CROW}, {"gecko", E_GECKO}, {"gull", E_GULL},
        {"lamp", E_LAMP}, {"ffish", E_FFISH}, {"wasp", E_WASP}, {"snail", E_SNAIL}, {"toad", E_TOAD},
        {"pelican", E_PELICAN}, {"jar", E_JAR}, {"cracker", E_CRACKER}, {"puffer", E_PUFFER}, {"spider", E_SPIDER},
        {"magpie", E_MAGPIE}, {"flasher", E_FLASHER}, {"sheet", E_SHEET}, {"moth", E_MOTH},
    };
    for (int i = 0; i < ARRAY_LEN(T); i++)
        if (!strcmp(name, T[i].n)) return T[i].t;
    return 0;
}

static int count_foes(void) {
    int n = 0;
    for (int i = 0; i < MAX_ENTS; i++) n += ents[i].alive && IS_FOE(ents[i].type);
    return n;
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
    if (!strcmp(key, "grounded")) { *out = pl.ground; return 1; }
    if (!strcmp(key, "jumps")) { *out = pl.jumps; return 1; }
    if (!strcmp(key, "stars")) { *out = count_type(E_STAR); return 1; }
    if (!strcmp(key, "tokens")) { *out = count_type(E_TOKEN); return 1; }
    if (!strcmp(key, "foes")) { *out = count_foes(); return 1; }
    if (!strcmp(key, "letters")) { *out = letters_this_run; return 1; }
    if (!strcmp(key, "letter_items")) { *out = count_type(E_LETTER); return 1; }
    if (!strcmp(key, "snacks")) { *out = snacks; return 1; }
    if (!strcmp(key, "crowns")) { *out = count_type(E_CROWN); return 1; }
    if (!strcmp(key, "in_arena")) { *out = in_arena; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = boss_i >= 0 && ents[boss_i].alive ? ents[boss_i].hp : 0; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = boss_dead; return 1; }
    if (!strcmp(key, "lanterns")) { *out = count_type(E_LANTERN); return 1; }
    if (!strcmp(key, "next_life")) { *out = (int)next_life_at; return 1; }
    if (!strcmp(key, "has_checkpoint")) { *out = sv.cp_valid; return 1; }
    if (!strcmp(key, "cp_area")) { *out = sv.cp_area; return 1; }
    if (!strcmp(key, "world_len")) { *out = RC_WORLD_CHUNKS; return 1; }
    if (!strcmp(key, "secrets")) {
        int n = 0;
        for (int c = 0; c < RC_WORLD_CHUNKS; c++)
            for (int y = 0; y < RC_ROWS; y++) n += strchr(RC_WORLD[c].rows[y], '!') != NULL;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "count_", 6)) {
        int type = foe_type_of(key + 6), n = 0;
        for (int i = 0; i < MAX_ENTS; i++) n += ents[i].alive && ents[i].type == type && type;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "hp_", 3)) {
        int type = foe_type_of(key + 3);
        *out = -1;
        for (int i = 0; i < MAX_ENTS; i++) if (ents[i].alive && ents[i].type == type && type) { *out = ents[i].hp; break; }
        return 1;
    }
    if (!strcmp(key, "token_value")) {
        *out = 0;
        for (int i = 0; i < MAX_ENTS; i++) if (ents[i].alive && ents[i].type == E_TOKEN) { *out = ents[i].value; break; }
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
    if (sscanf(cmd, "snacks %d", &a) == 1) { snacks = a; return 1; }
    if (sscanf(cmd, "boss_hp %d", &a) == 1) { if (boss_i >= 0) ents[boss_i].hp = a; return 1; }
    if (!strcmp(cmd, "clear_foes")) {
        for (int i = 0; i < MAX_ENTS; i++)
            if (ents[i].alive && (IS_FOE(ents[i].type) || ents[i].type == E_SNACK || ents[i].type == E_LANTERN)) ents[i].alive = false;
        return 1;
    }
    if (sscanf(cmd, "put %d %d", &a, &b) == 2) { pl.x = cam_x + a; pl.y = (float)b; pl.vy = 0; return 1; }
    if (sscanf(cmd, "foe %31s %d %d", name, &a, &b) == 3) {
        /* foe NAME X Y: drop a foe at a screen position */
        int type = foe_type_of(name);
        if (!type) return 0;
        Ent *e = spawn(type, cam_x + a, (float)b);
        if (e) { foe_setup(e); e->state = 0; }
        return 1;
    }
    if (sscanf(cmd, "snack %d %d", &a, &b) == 2) {
        Ent *s = spawn(E_SNACK, cam_x + a, (float)b);
        if (s) { s->w = 8; s->h = 7; }
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
    if (sscanf(cmd, "hops %d", &a) == 1) {
        for (int i = 0; i < MAX_ENTS; i++) if (ents[i].alive && ents[i].type == E_JAR) ents[i].n = a;
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
    "HARISSA THE CAT CHASES PARCEL THIEVES OVER THE ROOFTOPS.",
    {"FIND 3 LOST LETTERS IN A RUN", "DELIVER THE PARCEL", "CLEAR THE NIGHT ROUTE"},
    "LEFT/RIGHT\tHOLD BACK / RUN AHEAD\n"
    GLYPH_A "\tJUMP, AGAIN IN AIR\n"
    "DOWN+" GLYPH_A "\tDROP THROUGH LEDGES\n"
    GLYPH_B "\tTHROW (HOLD TO KEEP THROWING)\n"
    "START\tPAUSE\n\n"
    "SPIRIT: FLY, " GLYPH_B " SHOOT, " GLYPH_A " RETURN\n"
    "HIT THE PAPER LANTERN FOR 1UP.",
    C_ORANGE, C_SKY,
    rc_load, rc_start, rc_update, rc_draw, rc_quit, rc_label, rc_query, rc_cheat,
    "NINPEK", 3,
};
