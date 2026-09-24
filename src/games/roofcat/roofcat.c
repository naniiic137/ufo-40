/* ROOFCAT - an auto-scrolling rooftop chase. Cartridge 03 of UFO 40.
 * See docs/games/03-roofcat.md. */
#include "roofcat.h"

#define SCROLL 0.5f
#define PW 10
#define PH 13
#define GRAV 0.19f
#define JUMP1 (-3.55f)
#define JUMP2 (-3.2f)
#define RUN 1.3f
#define MAX_COLS (RC_MAX_CHUNKS * RC_CHUNK_W)
#define SPIRIT_FRAMES 240
#define STAR_RANGE 112.0f

enum { S_TITLE, S_INTRO, S_PLAY, S_CLEAR, S_OVER, S_ENDING };

enum {
    E_NONE, E_PIGEON, E_GULL, E_RAT, E_SLINGER, E_MIRAGE, E_BOBBER, E_VENT, E_MAGPIE, E_BOSS,
    E_STAR, E_SPIRIT_SHOT, E_PEBBLE, E_SEED, E_BUBBLE, E_BOMB, E_SHARD, E_BLAST,
    E_FISH, E_FOOD, E_LETTER, E_LANTERN, E_CATNIP, E_PUFF, E_TEXT, E_DEBRIS
};

typedef struct Ent {
    uint8_t type;
    bool alive;
    float x, y, vx, vy;
    int w, h, hp, t, state, dir, flash, value, sub;
    float hx, hy;
} Ent;

#define MAX_ENTS 120
static Ent ents[MAX_ENTS];

typedef struct Save {
    uint32_t magic;
    uint32_t best_score;
    uint8_t cp_valid, cp_loop, cp_stage, cp_lives;
    uint32_t cp_score;
    uint8_t cp_power, pad[3];
} Save;
#define SAVE_MAGIC 0x52430002u
static Save sv;

static char map[RC_ROWS][MAX_COLS + 1];
static int map_cols;
static int state, state_t, title_sel, frame_t, shake;
static int stage, loop;
static uint32_t score, next_life_at;
static int lives, power, kills_since_power, letters_this_run, stage_letter;
static float cam_x, arena_x;
/* Whole pixels the camera advanced this frame (0 or 1 at SCROLL 0.5). Anything
 * carried by the scroll moves by exactly this much, so it keeps the camera's
 * sub-pixel phase and never shimmers against the rooftops. */
static int carry_px;
/* Jitter probe for the tests: reversals of the drawn screen x and camera steps
 * that are not 0 or 1 pixel. */
static int jit_last_sx, jit_last_d, jit_last_cam, jit_rev, jit_cam_bad;
static bool jit_valid;
static bool in_arena, boss_dead;
static int boss_i = -1;
static bool sheet_mode;
static Rng rng;
static int life_idx;
static const uint32_t LIFE_AT[] = {3000, 7000, 12000, 18000, 25000, 33000, 42000, 52000, 63000, 75000};

typedef struct Player {
    float x, y, vx, vy;
    bool ground;
    int jumps, face, throw_t, throw_cd, anim, drop_t;
    bool spirit;
    int spirit_t, dead_t;
} Player;
static Player pl;

/* ------------------------------------------------------------------ */
/* map                                                                  */

static char tile_at(int tx, int ty) {
    if (tx < 0 || tx >= map_cols || ty < 0 || ty >= RC_ROWS) return '.';
    return map[ty][tx];
}
static bool solid_c(char c) {
    return c == '#' || c == 'w' || c == 'd' || c == 'O' || c == 'C' || c == 'H' || c == 'M' || c == 'B' || c == 'v';
}
static bool solid_at(int tx, int ty) { return solid_c(tile_at(tx, ty)); }
static bool oneway_at(int tx, int ty) { return tile_at(tx, ty) == '='; }

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

/* ------------------------------------------------------------------ */
/* stage building                                                       */

static const StageDef *SD(void) { return &RC_STAGES_DEF[stage]; }

static void build_stage(void) {
    const StageDef *s = SD();
    map_cols = s->n_chunks * RC_CHUNK_W;
    memset(ents, 0, sizeof ents);
    boss_i = -1;
    for (int c = 0; c < s->n_chunks; c++) {
        const Chunk *ch = &RC_CHUNKS[s->chunks[c]];
        for (int y = 0; y < RC_ROWS; y++)
            for (int x = 0; x < RC_CHUNK_W; x++) {
                char t = ch->rows[y][x];
                int wx = c * RC_CHUNK_W + x;
                map[y][wx] = t;
                /* things become entities; the tile turns into air */
                int type = 0;
                switch (t) {
                case 'p': type = E_PIGEON; break;
                case 'g': type = E_GULL; break;
                case 'r': type = E_RAT; break;
                case 's': type = E_SLINGER; break;
                case 'm': type = E_MIRAGE; break;
                case 'b': type = E_BOBBER; break;
                case 'q': type = E_MAGPIE; break;
                case '$': type = E_FISH; break;
                case 'F': type = E_FOOD; break;
                case 'v': type = E_VENT; break;
                }
                if (type) {
                    if (t != 'v') map[y][wx] = '.';
                    Ent *e = spawn(type, (float)(wx * 16), (float)(y * 16));
                    if (!e) continue;
                    e->state = -1; /* dormant until on screen */
                    switch (type) {
                    case E_PIGEON: e->w = 12; e->h = 11; e->x += 2; e->y += 5; e->value = 100; break;
                    case E_GULL: e->w = 14; e->h = 8; e->y += 3; e->value = 200; break;
                    case E_RAT: e->w = 12; e->h = 8; e->x += 2; e->y += 8; e->value = 150; break;
                    case E_SLINGER: e->w = 10; e->h = 13; e->x += 3; e->y += 3; e->hp = 2; e->value = 300; break;
                    case E_MIRAGE: e->w = 12; e->h = 11; e->x += 2; e->y += 1; e->hp = 2; e->value = 300; break;
                    case E_BOBBER: e->w = 10; e->h = 10; e->x += 3; e->y += 4; e->value = 200; break;
                    case E_MAGPIE: e->w = 12; e->h = 9; e->x += 2; e->y += 2; e->value = 300; break;
                    case E_VENT: e->w = 16; e->h = 16; e->hp = 4; e->value = 500; break;
                    case E_FISH: e->w = 10; e->h = 6; e->x += 3; e->y += 5; e->value = 100; e->state = 0; e->sub = 1; break;
                    case E_FOOD: e->w = 9; e->h = 8; e->x += 3; e->y += 6; e->sub = rng_range(&rng, 0, 3); e->state = 0;
                        e->value = e->sub == 0 ? 200 : e->sub == 1 ? 300 : e->sub == 2 ? 500 : 400; break;
                    }
                    e->hx = e->x;
                    e->hy = e->y;
                }
            }
    }
    for (int y = 0; y < RC_ROWS; y++) map[y][map_cols] = 0;
    /* the hidden letter */
    if (s->letter_chunk < s->n_chunks) {
        Ent *l = spawn(E_LETTER, (float)((s->letter_chunk * RC_CHUNK_W + s->letter_x) * 16 + 3), (float)(s->letter_y * 16 + 4));
        if (l) { l->w = 10; l->h = 8; l->value = 1000; l->sub = 1; }
    }
    arena_x = (float)((s->n_chunks - 1) * RC_CHUNK_W * 16);
    cam_x = 0;
    carry_px = 0;
    jit_valid = false;
    in_arena = false;
    boss_dead = false;
    stage_letter = 0;
    memset(&pl, 0, sizeof pl);
    pl.x = 48;
    pl.y = 96 - PH;
    pl.face = 1;
    pl.ground = true;
}

static void start_stage(void) {
    build_stage();
    state = S_INTRO;
    state_t = 0;
    game_set_pausable(true);
    music_play(RC_MUS_STAGE[SD()->music]);
    /* checkpoint */
    sv.cp_valid = 1;
    sv.cp_loop = (uint8_t)loop;
    sv.cp_stage = (uint8_t)stage;
    sv.cp_lives = (uint8_t)lives;
    sv.cp_score = score;
    sv.cp_power = (uint8_t)power;
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void new_run(void) {
    score = 0;
    lives = 3;
    power = 0;
    kills_since_power = 0;
    letters_this_run = 0;
    life_idx = 0;
    next_life_at = LIFE_AT[0];
    stage = 0;
    loop = 1;
    start_stage();
}

static void add_score(int v) {
    score += (uint32_t)v;
    if (score >= 15000) game_award(GOAL_BEACON);
    if (score > sv.best_score) sv.best_score = score;
    while (score >= next_life_at) {
        /* a paper lantern drifts up: hit it for a life */
        Ent *l = spawn(E_LANTERN, cam_x + (float)rng_range(&rng, 140, 280), 170);
        if (l) { l->w = 8; l->h = 12; l->state = 0; }
        life_idx++;
        next_life_at = life_idx < ARRAY_LEN(LIFE_AT) ? LIFE_AT[life_idx] : next_life_at + 12000;
    }
}

/* ------------------------------------------------------------------ */
/* player                                                               */

static int max_stars(void) { return 1 + power; }
static int throw_cooldown(void) { return power == 0 ? 16 : power == 1 ? 11 : 7; }
static float carry(void) { return (float)carry_px; }
static int cam_px(void) { return (int)floorf(cam_x); }

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
    pl.spirit = true;
    pl.spirit_t = SPIRIT_FRAMES;
    pl.vx = pl.vy = 0;
    if (pl.y > 150) pl.y = 130;
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
        pl.y += s;
        rem -= 1;
    }
}

static bool standing(void) {
    int ty = ((int)pl.y + PH) >> 4;
    if (((int)pl.y + PH) & 15) return box_solid(pl.x, pl.y + 1, PW, PH);
    for (int tx = (int)pl.x >> 4; tx <= ((int)pl.x + PW - 1) >> 4; tx++)
        if (solid_at(tx, ty) || (oneway_at(tx, ty) && pl.drop_t == 0)) return true;
    return false;
}

static int count_type(int type) {
    int n = 0;
    for (int i = 0; i < MAX_ENTS; i++) n += ents[i].alive && ents[i].type == type;
    return n;
}

static void update_spirit(void) {
    float c = carry();
    int dx = btn(BTN_RIGHT) - btn(BTN_LEFT), dy = btn(BTN_DOWN) - btn(BTN_UP);
    pl.x += c + dx * 1.1f;
    pl.y += dy * 1.1f;
    pl.x = fclamp(pl.x, cam_x + 2, cam_x + 320 - PW - 2);
    pl.y = fclamp(pl.y, 4, 146);
    if (pl.throw_cd > 0) pl.throw_cd--;
    if (btnp(BTN_B) && pl.throw_cd == 0) {
        for (int k = -1; k <= 1; k += 2) {
            Ent *s = spawn(E_SPIRIT_SHOT, pl.x + PW, pl.y + PH / 2 - 3 + k * 4);
            if (s) { s->w = 6; s->h = 6; s->vx = 4.2f; s->t = 40; }
        }
        pl.throw_cd = 10;
        sfx_play_name("rc_shoot");
    }
    pl.spirit_t--;
    if (pl.spirit_t <= 0 || (btnp(BTN_A) && pl.spirit_t < SPIRIT_FRAMES - 20)) revive();
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
    /* pushed by the screen's left edge */
    if (pl.x < cam_x) {
        if (box_solid(cam_x, pl.y, PW, PH)) { lose_life(); return; }
        pl.x = cam_x;
    }
    if (pl.x > cam_x + 320 - PW) pl.x = cam_x + 320 - PW;
    /* jumping */
    if (btnp(BTN_A)) {
        if (pl.ground && btn(BTN_DOWN)) {
            int ty = ((int)pl.y + PH) >> 4;
            bool on_ledge = false;
            for (int tx = (int)pl.x >> 4; tx <= ((int)pl.x + PW - 1) >> 4; tx++) on_ledge |= oneway_at(tx, ty);
            if (on_ledge) { pl.drop_t = 10; pl.ground = false; pl.y += 1; }
        } else if (pl.ground) {
            pl.vy = JUMP1;
            pl.jumps = 1;
            pl.ground = false;
            sfx_play_name("rc_jump");
        } else if (pl.jumps < 2) {
            pl.vy = JUMP2;
            pl.jumps = 2;
            sfx_play_name("rc_jump2");
        }
    }
    if (!btn(BTN_A) && pl.vy < -1.2f) pl.vy = -1.2f; /* short hop when released */
    /* throwing */
    if (btnp(BTN_B) && pl.throw_cd == 0 && count_type(E_STAR) < max_stars()) {
        Ent *s = spawn(E_STAR, pl.face > 0 ? pl.x + PW : pl.x - 8, pl.y + 3);
        if (s) { s->w = 8; s->h = 8; s->vx = 3.6f * pl.face; s->t = (int)(STAR_RANGE / 3.6f); }
        pl.throw_cd = throw_cooldown();
        pl.throw_t = 8;
        sfx_play_name("rc_throw");
    }
    /* gravity */
    if (pl.ground && standing() && pl.vy >= 0) {
        pl.vy = 0;
        pl.jumps = 0;
    } else {
        pl.ground = false;
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
/* enemies & things                                                     */

static float pcx(void) { return pl.x + PW / 2; }
static float pcy(void) { return pl.y + PH / 2; }
static float speed_mul(void) { return loop >= 2 ? 1.3f : 1.0f; }

static void drop_loot(Ent *e) {
    Ent *f = spawn(E_FISH, e->x + e->w / 2 - 5, e->y);
    if (f) { f->w = 10; f->h = 6; f->vy = -2.4f; f->vx = 0.6f; f->value = e->value; f->state = 1; }
    if (power < 2 && ++kills_since_power >= 3) {
        kills_since_power = 0;
        Ent *p = spawn(E_CATNIP, e->x + e->w / 2 - 4, e->y - 4);
        if (p) { p->w = 8; p->h = 8; p->vy = -2.0f; p->state = 1; }
    }
}

static void kill_ent(Ent *ep) {
    Ent dead = *ep; /* the slot is recycled by the effects spawned below */
    Ent *e = &dead;
    ep->alive = false;
    puff(e->x + e->w / 2, e->y + e->h / 2, C_WHITE);
    sfx_play_name("rc_hit");
    drop_loot(e);
    if (e->type == E_BOBBER) {
        Ent *b = spawn(E_BLAST, e->x + e->w / 2 - 20, e->y + e->h / 2 - 20);
        if (b) { b->w = 40; b->h = 40; b->t = 16; }
        sfx_play_name("rc_boom");
        shake = 8;
    }
}

static bool hittable(Ent *e) {
    if (e->type >= E_PIGEON && e->type <= E_VENT) return !(e->type == E_MIRAGE && e->sub == 1);
    if (e->type == E_MAGPIE) return true;
    if (e->type == E_BOSS) return e->state != 99;
    if (e->type == E_LANTERN || e->type == E_BUBBLE) return true;
    return false;
}

static void boss_damage(Ent *b, int dmg);

static void hurt(Ent *e, int dmg) {
    if (e->type == E_LANTERN) {
        e->alive = false;
        lives++;
        sfx_play_name("rc_oneup");
        Ent *h = spawn(E_TEXT, e->x, e->y);
        if (h) { h->value = -1; h->t = 50; }
        return;
    }
    if (e->type == E_BUBBLE) { e->alive = false; puff(e->x + 3, e->y + 3, C_ICE); return; }
    if (e->type == E_BOSS) { boss_damage(e, dmg); return; }
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
    s->t = 400;
}

/* ---- bosses ---- */

static void spawn_boss(void) {
    int kind = SD()->boss;
    Ent *b = spawn(E_BOSS, arena_x + 250, 40);
    if (!b) return;
    boss_i = (int)(b - ents);
    b->sub = kind;
    b->state = 0;
    b->t = 0;
    b->dir = -1;
    float hpmul = loop >= 2 ? 1.3f : 1.0f;
    switch (kind) {
    case BOSS_PIGEON: b->w = 40; b->h = 26; b->hp = (int)(22 * hpmul); b->y = 96 - 26; b->x = arena_x + 240; break;
    case BOSS_POTS: b->w = 20; b->h = 54; b->hp = (int)(18 * hpmul); b->y = 96 - 54; b->x = arena_x + 250; b->value = 3; break;
    case BOSS_CRAB: b->w = 56; b->h = 30; b->hp = (int)(26 * hpmul); b->y = 96 - 30; b->x = arena_x + 230; break;
    case BOSS_MAGPIE: b->w = 54; b->h = 24; b->hp = (int)(32 * hpmul); b->y = 20; b->x = arena_x + 220; break;
    }
    b->hx = b->hp; /* max hp for the bar */
    music_play(RC_MUS_BOSS);
}

static void boss_damage(Ent *b, int dmg) {
    if (b->flash > 0 || boss_dead) return;
    b->hp -= dmg;
    b->flash = 5;
    sfx_play_name("rc_bosshit");
    if (b->sub == BOSS_POTS) {
        int per = (int)(b->hx / 3.0f + 0.99f);
        int pots = (b->hp + per - 1) / per;
        if (pots < b->value) {
            b->value = pots;
            puff(b->x + 10, b->y + 6, C_EARTH);
            sfx_play_name("rc_boom");
            shake = 8;
            b->y += 18;
            b->h -= 18;
        }
    }
    if (b->hp <= 0) {
        boss_dead = true;
        b->state = 99;
        b->t = 120;
        music_stop();
        add_score(5000);
        float_text(b->x + b->w / 2, b->y, 5000);
    }
}

static void update_boss(Ent *b) {
    float sm = speed_mul();
    b->t++;
    if (b->flash > 0) b->flash--;
    if (b->state == 99) {
        if (b->t % 6 == 0) { puff(b->x + rng_range(&rng, 0, b->w), b->y + rng_range(&rng, 0, b->h), C_YELLOW); sfx_play_name("rc_boom"); shake = 6; }
        if (b->t >= 240) { b->alive = false; }
        return;
    }
    float floor_y = 96;
    switch (b->sub) {
    case BOSS_PIGEON:
        /* hop toward Harissa; every other landing, spit seeds */
        if (b->state == 0) {
            if (b->t > 50 / sm) {
                b->state = 1;
                b->dir = pcx() < b->x + b->w / 2 ? -1 : 1;
                b->vx = 1.6f * b->dir * sm;
                b->vy = -4.2f;
            }
        } else {
            b->vy += 0.2f;
            b->x += b->vx;
            b->y += b->vy;
            if (b->x < arena_x + 8) { b->x = arena_x + 8; b->vx = -b->vx; }
            if (b->x > arena_x + 312 - b->w) { b->x = arena_x + 312 - b->w; b->vx = -b->vx; }
            if (b->y >= floor_y - b->h) {
                b->y = floor_y - b->h;
                b->state = 0;
                b->t = 0;
                shake = 5;
                if ((b->value++ & 1)) {
                    for (int k = -1; k <= 1; k++)
                        enemy_shot(E_SEED, b->x + (b->dir > 0 ? b->w : 0), b->y + 8, (1.8f + k * 0.4f) * (pcx() < b->x ? -1 : 1) * sm, -1.6f + k * 0.9f);
                    sfx_play_name("rc_shoot");
                }
            }
        }
        break;
    case BOSS_POTS:
        b->x += 0.5f * b->dir * sm;
        if (b->x < arena_x + 150) b->dir = 1;
        if (b->x > arena_x + 280) b->dir = -1;
        if (b->t % (int)(64 / sm) == 0) {
            float dx = pcx() - (b->x + 10);
            enemy_shot(E_SHARD, b->x + 8, b->y, fclamp(dx / 55.0f, -2.6f, 2.6f), -3.0f);
            sfx_play_name("rc_shoot");
        }
        break;
    case BOSS_CRAB:
        if (b->state == 0) {
            b->x += 0.8f * b->dir * sm;
            if (b->x < arena_x + 20) b->dir = 1;
            if (b->x > arena_x + 300 - b->w) b->dir = -1;
            if (b->t % (int)(90 / sm) == 0) {
                for (int k = 0; k < 3; k++)
                    enemy_shot(E_BUBBLE, b->x + b->w / 2, b->y + 4, (pcx() < b->x ? -0.6f : 0.6f) * (1 + k * 0.5f), -0.4f - k * 0.15f);
                sfx_play_name("rc_shoot");
            }
            if (b->t % 170 == 169) { b->state = 1; b->dir = pcx() < b->x + b->w / 2 ? -1 : 1; b->t = 0; }
        } else {
            /* claw lunge */
            b->x += 3.0f * b->dir * sm;
            if (b->x < arena_x + 10 || b->x > arena_x + 310 - b->w || b->t > 40) {
                b->x = fclamp(b->x, arena_x + 10, arena_x + 310 - b->w);
                b->state = 0;
                b->t = 1;
                shake = 6;
            }
        }
        break;
    case BOSS_MAGPIE: {
        if (b->state == 0) {
            float k = b->t * 0.02f * sm;
            b->x = arena_x + 130 + sinf(k) * 110;
            b->y = 22 + sinf(k * 2) * 12;
            if (b->t % (int)(80 / sm) == 0) { enemy_shot(E_BOMB, b->x + b->w / 2, b->y + b->h, 0, 0.5f); }
            if (b->t % 200 == 199) { b->state = 1; b->hy = pcx(); b->vy = 0; }
        } else if (b->state == 1) {
            /* dive */
            float tx = b->hy - b->w / 2;
            b->x += fclamp(tx - b->x, -3, 3);
            b->y += 3.2f * sm;
            if (b->y > 96 - b->h) { b->y = 96 - b->h; b->state = 2; shake = 8; sfx_play_name("rc_boom"); }
        } else {
            b->y -= 1.6f;
            if (b->y <= 22) { b->state = 0; b->t = 1; }
        }
        break;
    }
    }
}

static void update_ents(void) {
    float sm = speed_mul();
    for (int i = 0; i < MAX_ENTS; i++) {
        Ent *e = &ents[i];
        if (!e->alive) continue;
        if (e->flash > 0 && e->type != E_BOSS) e->flash--;
        /* dormant things wake when they scroll into view */
        if (e->state == -1) {
            if (e->x < cam_x + 330) {
                e->state = 0;
                if (e->type == E_GULL) e->x = cam_x + 330;
            } else continue;
        }
        /* things far behind the camera are gone */
        if (e->x + e->w < cam_x - 40 && e->type != E_BOSS) { e->alive = false; continue; }
        e->t++;
        switch (e->type) {
        case E_PIGEON: {
            float nx = e->x + 0.5f * e->dir * sm;
            int ahead = e->dir > 0 ? (int)(nx + e->w) : (int)nx - 1;
            int foot = (int)(e->y + e->h);
            bool floor = solid_at(ahead >> 4, foot >> 4) || oneway_at(ahead >> 4, foot >> 4);
            if (box_solid(nx, e->y, e->w, e->h) || !floor) e->dir = -e->dir;
            else e->x = nx;
            break;
        }
        case E_GULL:
            e->x -= 1.25f * sm;
            e->y = e->hy + sinf(e->t * 0.07f) * 14;
            break;
        case E_RAT: {
            bool on = box_solid(e->x, e->y + 1, e->w, e->h) || oneway_at(((int)e->x + 6) >> 4, ((int)e->y + e->h) >> 4);
            if (on && e->vy >= 0) {
                e->vy = 0;
                e->vx = 0;
                if (e->t % (int)(46 / sm) == 0) {
                    e->dir = pcx() < e->x ? -1 : 1;
                    e->vy = -3.0f;
                    e->vx = 1.2f * e->dir * sm;
                }
            } else {
                e->vy += 0.2f;
            }
            float nx = e->x + e->vx;
            if (!box_solid(nx, e->y, e->w, e->h)) e->x = nx;
            float ny = e->y + e->vy;
            if (e->vy > 0) {
                int ty = (int)(ny + e->h) >> 4;
                bool land = false;
                for (int tx = (int)e->x >> 4; tx <= ((int)e->x + e->w - 1) >> 4; tx++)
                    land |= solid_at(tx, ty) || (oneway_at(tx, ty) && e->y + e->h <= ty * 16);
                if (land && (int)(e->y + e->h) <= ty * 16 + 2) { e->y = (float)(ty * 16 - e->h); e->vy = 0; }
                else e->y = ny;
            } else e->y = ny;
            if (e->y > 180) e->alive = false;
            break;
        }
        case E_SLINGER:
            e->dir = pcx() < e->x ? -1 : 1;
            if (e->t % (int)(100 / sm) == 60) {
                float dx = pcx() - e->x;
                enemy_shot(E_PEBBLE, e->x + 3, e->y, fclamp(dx / 48.0f, -2.8f, 2.8f) + (in_arena ? 0.0f : SCROLL), -3.1f);
                e->sub = 10;
            }
            if (e->sub > 0) e->sub--;
            break;
        case E_MIRAGE:
            e->y = e->hy + sinf(e->t * 0.05f) * 10;
            e->x = e->hx + sinf(e->t * 0.021f) * 20;
            e->sub = (e->t % 180) >= 120 ? 1 : 0; /* phased out */
            break;
        case E_BOBBER:
            e->y = e->hy + sinf(e->t * 0.06f) * 22;
            break;
        case E_VENT:
            if (e->t % (int)(160 / sm) == 80 && count_type(E_RAT) < 3 && e->x < cam_x + 300) {
                Ent *r = spawn(E_RAT, e->x + 2, e->y - 8);
                if (r) { r->w = 12; r->h = 8; r->vy = -3; r->value = 150; r->t = 1; r->dir = -1; r->vx = -0.8f; }
            }
            break;
        case E_MAGPIE:
            if (e->sub == 0) {
                if (fabsf(pcx() - e->x) < 110 && e->x < cam_x + 300) {
                    e->sub = 1;
                    float dx = pcx() - e->x, dy = pcy() - e->y, d = sqrtf(dx * dx + dy * dy) + 0.1f;
                    e->vx = dx / d * 2.6f * sm + (in_arena ? 0.0f : SCROLL);
                    e->vy = dy / d * 2.6f * sm;
                }
            } else {
                e->x += e->vx;
                e->y += e->vy;
                if (e->t > 0 && e->sub == 1 && e->y > pcy() + 10) { e->sub = 2; }
                if (e->sub == 2) { e->vy -= 0.12f; e->vx += 0.02f; }
                if (e->y < -30 || e->y > 190) e->alive = false;
            }
            break;
        case E_BOSS: update_boss(e); break;
        case E_STAR: case E_SPIRIT_SHOT: {
            e->x += e->vx + carry();
            if (--e->t <= 0 || (e->type == E_STAR && box_solid(e->x, e->y, e->w, e->h))) { e->alive = false; break; }
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
        case E_PEBBLE: case E_SEED: case E_SHARD:
            e->vy += 0.13f;
            e->x += e->vx;
            e->y += e->vy;
            if (e->y > 175 || box_solid(e->x, e->y, e->w, e->h)) e->alive = false;
            break;
        case E_BUBBLE:
            e->x += e->vx * sm;
            e->y += e->vy + sinf(e->t * 0.1f) * 0.4f;
            if (e->y < -10 || e->t > 420) e->alive = false;
            break;
        case E_BOMB:
            e->vy += 0.15f;
            e->y += e->vy;
            if (e->y + e->h >= 96) {
                e->alive = false;
                Ent *b = spawn(E_BLAST, e->x - 16, 96 - 30);
                if (b) { b->w = 38; b->h = 30; b->t = 14; }
                sfx_play_name("rc_boom");
                shake = 6;
            }
            break;
        case E_BLAST:
            if (--e->t <= 0) e->alive = false;
            break;
        case E_FISH: case E_FOOD: case E_LETTER: case E_CATNIP:
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

static void contacts(void) {
    if (pl.spirit || pl.dead_t) return;
    int px = (int)pl.x + 1, py = (int)pl.y + 2, pw = PW - 2, ph = PH - 3;
    for (int i = 0; i < MAX_ENTS; i++) {
        Ent *e = &ents[i];
        if (!e->alive || e->state == -1) continue;
        if (!rects_overlap(px, py, pw, ph, (int)e->x, (int)e->y, e->w, e->h)) continue;
        switch (e->type) {
        case E_FISH: case E_FOOD:
            e->alive = false;
            add_score(e->value);
            float_text(e->x, e->y - 4, e->value);
            sfx_play_name(e->type == E_FISH ? "rc_coin" : "rc_food");
            break;
        case E_LETTER:
            e->alive = false;
            add_score(1000);
            letters_this_run++;
            stage_letter = 1;
            float_text(e->x, e->y - 4, 1000);
            sfx_play_name("rc_letter");
            break;
        case E_CATNIP:
            e->alive = false;
            if (power < 2) power++;
            sfx_play_name("rc_power");
            break;
        case E_LANTERN: case E_TEXT: case E_DEBRIS: case E_STAR: case E_SPIRIT_SHOT: case E_VENT:
            break;
        case E_MIRAGE:
            if (e->sub == 0) lose_life();
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

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

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
        state = S_CLEAR;
        state_t = 0;
        music_restart(RC_MUS_CLEAR);
    }
}

static void next_stage(void) {
    stage++;
    if (stage >= RC_STAGES) {
        state = S_ENDING;
        state_t = 0;
        game_award(GOAL_SAUCER);
        if (loop >= 2) game_award(GOAL_ALIEN);
        sv.cp_valid = 0;
        save_now();
        music_restart(RC_MUS_END);
        game_set_pausable(false);
        return;
    }
    start_stage();
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
            power = sv.cp_power;
            loop = sv.cp_loop;
            stage = sv.cp_stage;
            kills_since_power = 0;
            letters_this_run = 0;
            life_idx = 0;
            while (life_idx < ARRAY_LEN(LIFE_AT) && LIFE_AT[life_idx] <= score) life_idx++;
            next_life_at = life_idx < ARRAY_LEN(LIFE_AT) ? LIFE_AT[life_idx] : score + 12000;
            start_stage();
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
        if (state_t > 90 || (state_t > 20 && btnp(BTN_A))) { state = S_PLAY; state_t = 0; input_consume(); }
        break;
    case S_PLAY: play_update(); break;
    case S_CLEAR:
        frame_t++;
        update_ents();
        if (state_t > 200 || (state_t > 60 && btnp(BTN_A))) next_stage();
        break;
    case S_OVER:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) { state = S_TITLE; state_t = 0; title_sel = 0; music_play(RC_MUS_TITLE); }
        break;
    case S_ENDING:
        frame_t++;
        if (state_t > 240 && btnp(BTN_A)) {
            if (loop == 1) {
                /* on to the night route, keeping score and lives */
                loop = 2;
                stage = 0;
                start_stage();
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
static const Theme THEMES[4] = {
    {{C_SKY, C_CYAN, C_CYAN, C_ICE}, C_WHITE, C_WHITE, C_LIGHT, C_SKY, C_BLUE, C_NAVY, C_BLUE, C_SKY, C_WHITE, C_LIGHT, C_SKY, C_CYAN},
    {{C_ORANGE, C_AMBER, C_AMBER, C_YELLOW}, C_EARTH, C_HIDE, C_TAN, C_RED, C_BROWN, C_MAROON, C_BROWN, C_RED, C_CREAM, C_TAN, C_ORANGE, C_AMBER},
    {{C_PURPLE, C_MAGENTA, C_PINK, C_AMBER}, C_TAN, C_EARTH, C_BROWN, C_EARTH, C_MAROON, C_BROWN, C_BROWN, C_EARTH, C_TAN, C_VIOLET, C_MAGENTA, C_WINE},
    {{C_INK, C_NIGHT, C_NAVY, C_DUSK}, C_SLATE, C_GREY, C_DUSK, C_GREY, C_NIGHT, C_INK, C_BROWN, C_GREY, C_SLATE, C_NIGHT, C_DUSK, C_NAVY},
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

static const Theme *TH(void) { return &THEMES[SD()->theme]; }

static void draw_sky(void) {
    const Theme *t = TH();
    int theme = SD()->theme;
    bool night = theme == 3 || loop >= 2;
    static const uint8_t night_sky[4] = {C_INK, C_NIGHT, C_NAVY, C_DUSK};
    const uint8_t *sky = night ? night_sky : t->sky;
    for (int b = 0; b < 4; b++) gfx_rect(0, b * 40, 320, 40, sky[b]);
    for (int b = 0; b < 3; b++) gfx_dither(0, b * 40 + 30, 320, 10, sky[b + 1], 8);
    /* sun / moon */
    int icam = cam_px();
    int par = icam / 20;
    int sx = 250 - par % 40, sy = 34;
    if (night) {
        gfx_dither_circle(sx, sy, 20, C_DUSK, 5);
        gfx_circ(sx, sy, 12, C_CREAM);
        gfx_circ(sx + 4, sy - 3, 10, sky[0]);
        for (int i = 0; i < 40; i++) {
            uint32_t h = (uint32_t)(i * 2654435761u);
            gfx_pset((int)(h % 320), (int)((h >> 12) % 90), (frame_t / 16 + i) % 9 ? C_LIGHT : C_WHITE);
        }
    } else if (theme == 0) { gfx_circ(sx, sy, 14, C_YELLOW); gfx_circ(sx, sy, 10, C_CREAM); }
    else if (theme == 1) { gfx_circ(sx, 50, 18, C_YELLOW); gfx_dither_circle(sx, 50, 24, C_CREAM, 4); }
    else if (theme == 2) { gfx_circ(160, 118, 28, C_ORANGE); gfx_circ(160, 118, 22, C_AMBER); gfx_circ(160, 118, 15, C_YELLOW); }
    /* far silhouettes (parallax 0.25) */
    int off = icam / 4;
    for (int i = -1; i < 12; i++) {
        int base = i * 36 - off % 36;
        uint32_t h = (uint32_t)((i + off / 36) * 2654435761u);
        int hgt = 18 + (int)(h % 26);
        int x = base;
        if (theme == 2) {
            /* sea and distant boats */
            continue;
        }
        int gy = 104;
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
    } else if (theme == 3) {
        gfx_rect(0, 104, 320, 56, C_INK);
    }
    if (theme == 2) {
        gfx_rect(0, 118, 320, 60, C_NAVY);
        for (int y = 118; y < 160; y += 3)
            for (int x = (y * 7 + frame_t / 4) % 12; x < 320; x += 12) gfx_hline(x, x + 3, y, (y / 3) % 2 ? C_BLUE : C_VIOLET);
        /* lighthouse */
        int lx = 290 - off % 380;
        gfx_rect(lx, 70, 10, 48, C_LIGHT);
        gfx_rect(lx, 80, 10, 5, C_RED);
        gfx_rect(lx, 96, 10, 5, C_RED);
        gfx_rect(lx - 2, 64, 14, 6, C_INK);
        if ((frame_t / 20) % 2) gfx_dither(lx + 10, 62, 60, 8, C_YELLOW, 6);
    }
    /* clouds / lanterns (parallax 0.5) */
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
        }
    }
}

static void draw_tile(int tx, int ty, char c, int x, int y) {
    const Theme *t = TH();
    int theme = SD()->theme;
    bool air_above = !solid_at(tx, ty - 1) && tile_at(tx, ty - 1) != '=';
    switch (c) {
    case '#': case 'w': case 'd': case 'C': case 'v': case 'B': {
        gfx_rect(x, y, 16, 16, t->wall);
        if (theme == 3 || c == 'B') {
            for (int r = 0; r < 4; r++) {
                gfx_hline(x, x + 15, y + r * 4 + 3, t->wall_lo);
                gfx_vline(x + ((r & 1) ? 4 : 12), y + r * 4, y + r * 4 + 3, t->wall_lo);
            }
        } else if (theme == 2) {
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
        } else if (c == 'C' || c == 'v') {
            gfx_rect(x + 2, y, 12, 16, t->wall_lo);
            gfx_rect(x + 3, y + 1, 10, 15, t->wall);
            gfx_rect(x + 1, y, 14, 3, t->trim);
        }
        if (air_above && c != 'C' && c != 'v') {
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

static void draw_ent(Ent *e) {
    int x = (int)e->x, y = (int)e->y;
    int solid = e->flash > 0 ? C_WHITE : -1;
    int f = (e->t / 8) % 2;
    int fl = e->dir > 0 ? SPR_FLIPX : 0;
    switch (e->type) {
    case E_PIGEON: spr_draw_ex(&rc_spr[f ? R_PIGEON1 : R_PIGEON2], x - 2, y - 5, fl, NULL, solid); break;
    case E_GULL: spr_draw_ex(&rc_spr[(e->t / 6) % 2 ? R_GULL1 : R_GULL2], x - 1, y - 1, 0, NULL, solid); break;
    case E_RAT: spr_draw_ex(&rc_spr[e->vy != 0 ? R_RAT2 : R_RAT1], x - 2, y - 8, fl, NULL, solid); break;
    case E_SLINGER: spr_draw_ex(&rc_spr[e->sub > 0 ? R_SLINGER2 : R_SLINGER1], x - 3, y - 3, fl, NULL, solid); break;
    case E_MIRAGE:
        if (e->sub == 1) { if ((e->t / 2) % 3 == 0) spr_draw_ex(&rc_spr[R_MIRAGE1], x - 2, y - 1, 0, NULL, C_VIOLET); }
        else spr_draw_ex(&rc_spr[f ? R_MIRAGE1 : R_MIRAGE2], x - 2, y - 1, 0, NULL, solid);
        break;
    case E_BOBBER: gfx_vline(x + 5, y + 10, y + 16, C_GREY); spr_draw_ex(&rc_spr[f ? R_BOBBER1 : R_BOBBER2], x - 3, y - 4, 0, NULL, solid); break;
    case E_VENT: spr_draw_ex(&rc_spr[(e->t / 20) % 2 ? R_VENT1 : R_VENT2], x, y, 0, NULL, solid); break;
    case E_MAGPIE: spr_draw_ex(&rc_spr[(e->t / 4) % 2 ? R_MAGPIE1 : R_MAGPIE2], x - 2, y - 2, e->vx > 0 ? SPR_FLIPX : 0, NULL, solid); break;
    case E_BOSS: {
        int bs = e->flash > 0 ? C_WHITE : -1;
        if (e->state == 99 && (e->t / 3) % 2) bs = C_YELLOW;
        switch (e->sub) {
        case BOSS_PIGEON:
            spr_draw_ex(&rc_spr[e->state == 1 ? R_BOSS_PIGEON2 : R_BOSS_PIGEON1], x - 4, y - 5, e->dir > 0 ? 0 : 0, NULL, bs);
            break;
        case BOSS_POTS:
            for (int k = 0; k < e->value; k++)
                spr_draw_ex(&rc_spr[k == 0 ? R_POT_EYES : R_POT], x - 2, y + k * 18 - 1, 0, NULL, k == 0 ? bs : -1);
            break;
        case BOSS_CRAB:
            spr_draw_scaled(&rc_spr[(e->t / 10) % 2 ? R_CRAB1 : R_CRAB2], x - 4, y - 10, 2, 0);
            if (bs >= 0) gfx_dither(x, y, e->w, e->h, bs, 8);
            break;
        case BOSS_MAGPIE:
            spr_draw_scaled(&rc_spr[(e->t / 6) % 2 ? R_MKING1 : R_MKING2], x - 5, y - 4, 2, pcx() > x + 27 ? SPR_FLIPX : 0);
            if (bs >= 0) gfx_dither(x, y, e->w, e->h, C_WHITE, 8);
            break;
        }
        break;
    }
    case E_STAR: spr_draw(&rc_spr[(e->t / 3) % 2 ? R_STAR1 : R_STAR2], x, y, 0); break;
    case E_SPIRIT_SHOT: spr_draw(&rc_spr[R_WISP_SHOT], x, y, 0); break;
    case E_PEBBLE: spr_draw(&rc_spr[R_PEBBLE], x, y, 0); break;
    case E_SEED: spr_draw(&rc_spr[R_SEED], x, y, 0); break;
    case E_SHARD: spr_draw(&rc_spr[R_SHARD], x, y, 0); break;
    case E_BUBBLE: spr_draw(&rc_spr[R_BUBBLE], x, y, 0); break;
    case E_BOMB: spr_draw(&rc_spr[R_BOMB], x, y, 0); break;
    case E_BLAST:
        gfx_circ(x + e->w / 2, y + e->h / 2, e->w / 2 - (16 - e->t) / 3, (e->t / 2) % 2 ? C_YELLOW : C_ORANGE);
        gfx_circ(x + e->w / 2, y + e->h / 2, e->w / 4, C_CREAM);
        break;
    case E_FISH: spr_draw(&rc_spr[R_FISH], x, y + ((e->state != 1 && (frame_t / 12) % 2) ? 1 : 0), 0); break;
    case E_FOOD: {
        static const int food[4] = {R_DATE, R_ORANGE, R_POMEGRANATE, R_BREAD};
        spr_draw(&rc_spr[food[e->sub & 3]], x, y + ((frame_t / 12) % 2), 0);
        break;
    }
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
    tiny_draw(SD()->name, 70, 4, C_SLATE);
    snprintf(buf, sizeof buf, "%s  STAGE %d-%d", loop >= 2 ? "NIGHT ROUTE" : "DAY ROUTE", loop, stage + 1);
    tiny_draw(buf, 70, 11, loop >= 2 ? C_VIOLET : C_SKY);
    for (int i = 0; i < 2; i++) spr_draw_ex(&rc_spr[R_CATNIP], 206 + i * 10, 6, 0, NULL, i < power ? -1 : C_DUSK);
    for (int i = 0; i < imin(lives, 6); i++) spr_draw(&rc_spr[R_CAT_HEAD], 232 + i * 9, 7, 0);
    if (lives > 6) { snprintf(buf, sizeof buf, "+%d", lives - 6); tiny_draw(buf, 288, 8, C_WHITE); }
    if (in_arena && boss_i >= 0 && ents[boss_i].alive) {
        Ent *b = &ents[boss_i];
        int w = (int)(100 * fclamp((float)b->hp / b->hx, 0, 1));
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
        if (ents[i].alive && ents[i].state != -1) draw_ent(&ents[i]);
    draw_player();
    gfx_camera(0, 0);
    gfx_noclip();
    draw_hud();
}

static void draw_title(void) {
    stage = 0;
    loop = 1;
    cam_x = (float)(frame_t % 2000);
    gfx_camera(0, 0);
    gfx_clip(0, 0, 320, 180);
    draw_sky();
    gfx_camera(0, 0);
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
    spr_draw(&rc_spr[(frame_t / 4) % 2 ? R_MAGPIE1 : R_MAGPIE2], 230 + (int)(sinf(frame_t * 0.05f) * 10), 90, SPR_FLIPX);
    spr_draw(&rc_spr[R_PARCEL], 244 + (int)(sinf(frame_t * 0.05f) * 10), 104, 0);
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE, C_RED};
    ui_fancy_center("ROOFCAT", 160, 20, 4, grad, 4, C_INK, C_WINE);
    text_center("CATCH THE MAGPIE MOB!", 160, 54, C_NAVY);
    const char *items[2];
    int n = 0;
    char cont[40];
    if (sv.cp_valid) {
        snprintf(cont, sizeof cont, "CONTINUE  %s %d-%d", sv.cp_loop >= 2 ? "NIGHT" : "DAY", sv.cp_loop, sv.cp_stage + 1);
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
    int y = t < 20 ? -30 + t * 3 : t > 70 ? 30 - (t - 70) * 3 : 30;
    ui_panel(60, y + 30, 200, 44, C_INK, loop >= 2 ? C_VIOLET : C_YELLOW);
    char buf[48];
    snprintf(buf, sizeof buf, "STAGE %d-%d", loop, stage + 1);
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center(buf, 160, y + 35, 1, grad, 3, C_INK, -1);
    text_center(SD()->name, 160, y + 49, C_WHITE);
    tiny_center(SD()->subtitle, 160, y + 61, C_GREY);
}

static void draw_clear(void) {
    draw_play();
    ui_panel(70, 50, 180, 70, C_INK, C_LIME);
    static const uint8_t grad[] = {C_LIME, C_LEAF, C_JADE};
    ui_fancy_center("STAGE CLEAR!", 160, 58, 2, grad, 3, C_INK, -1);
    char buf[64];
    snprintf(buf, sizeof buf, "SCORE %07u", (unsigned)score);
    text_center(buf, 160, 82, C_WHITE);
    text_center(stage_letter ? "LOST LETTER FOUND!" : "LOST LETTER: MISSED", 160, 96, stage_letter ? C_YELLOW : C_SLATE);
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
        spr_draw(&rc_spr[R_PARCEL], 218, 130, 0);
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
    case S_CLEAR: draw_clear(); break;
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
    spr_draw(&rc_spr[(t / 4) % 2 ? R_MAGPIE1 : R_MAGPIE2], x + 104, y + 12 + (int)(sinf(t * 0.07f) * 4), SPR_FLIPX);
    spr_draw(&rc_spr[R_PARCEL], x + 106, y + 24 + (int)(sinf(t * 0.07f) * 4), 0);
}

static int rc_query(const char *key, int *out) {
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "score")) { *out = (int)score; return 1; }
    if (!strcmp(key, "lives")) { *out = lives; return 1; }
    if (!strcmp(key, "power")) { *out = power; return 1; }
    if (!strcmp(key, "stage")) { *out = stage; return 1; }
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
    if (!strcmp(key, "in_arena")) { *out = in_arena; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = boss_i >= 0 && ents[boss_i].alive ? ents[boss_i].hp : 0; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = boss_dead; return 1; }
    if (!strcmp(key, "lanterns")) { *out = count_type(E_LANTERN); return 1; }
    if (!strcmp(key, "has_checkpoint")) { *out = sv.cp_valid; return 1; }
    if (!strcmp(key, "chunk_errors")) {
        int errs = 0;
        for (int c = 0; c < RC_CHUNK_COUNT; c++) {
            const Chunk *ch = &RC_CHUNKS[c];
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
    if (sscanf(cmd, "stage %d %d", &a, &b) == 2) {
        if (state == S_TITLE) new_run();
        loop = b;
        stage = a;
        start_stage();
        state = S_PLAY;
        return 1;
    }
    if (!strcmp(cmd, "play")) { if (state == S_INTRO) state = S_PLAY; return 1; }
    if (sscanf(cmd, "cam %d", &a) == 1) {
        cam_x = (float)a;
        pl.x = cam_x + 60;
        pl.y = 20;
        pl.vy = 0;
        for (int i = 0; i < MAX_ENTS; i++) if (ents[i].alive && ents[i].x < cam_x - 40 && ents[i].type != E_BOSS) ents[i].alive = false;
        return 1;
    }
    if (!strcmp(cmd, "arena")) {
        cam_x = arena_x - 1;
        pl.x = arena_x + 40;
        pl.y = 96 - PH;
        return 1;
    }
    if (sscanf(cmd, "score %d", &a) == 1) { score = 0; add_score(a); return 1; }
    if (sscanf(cmd, "lives %d", &a) == 1) { lives = a; return 1; }
    if (sscanf(cmd, "power %d", &a) == 1) { power = a; return 1; }
    if (sscanf(cmd, "boss_hp %d", &a) == 1) { if (boss_i >= 0) ents[boss_i].hp = a; return 1; }
    if (!strcmp(cmd, "clear_foes")) {
        for (int i = 0; i < MAX_ENTS; i++)
            if (ents[i].alive && ents[i].type >= E_PIGEON && ents[i].type <= E_MAGPIE) ents[i].alive = false;
        return 1;
    }
    if (sscanf(cmd, "put %d %d", &a, &b) == 2) { pl.x = cam_x + a; pl.y = (float)b; pl.vy = 0; return 1; }
    if (sscanf(cmd, "foe %d %d", &a, &b) == 2) {
        Ent *e = spawn(E_PIGEON, cam_x + a, (float)b);
        if (e) { e->w = 12; e->h = 11; e->value = 100; e->state = 0; }
        return 1;
    }
    if (!strcmp(cmd, "kill")) { lose_life(); return 1; }
    if (!strcmp(cmd, "jitter_reset")) { jit_valid = false; jit_last_d = 0; jit_rev = 0; jit_cam_bad = 0; return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    if (!strcmp(cmd, "win_stage")) {
        if (boss_i >= 0) { boss_damage(&ents[boss_i], 999); ents[boss_i].t = 239; }
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
    {"SCORE 15,000 IN ONE RUN", "DELIVER THE PARCEL", "CLEAR THE NIGHT ROUTE"},
    "LEFT/RIGHT\tHOLD BACK / RUN AHEAD\n"
    GLYPH_A "\tJUMP, AGAIN IN AIR\n"
    "DOWN+" GLYPH_A "\tDROP THROUGH LEDGES\n"
    GLYPH_B "\tTHROW JASMINE STAR\n"
    "START\tPAUSE\n\n"
    "SPIRIT: FLY, " GLYPH_B " SHOOT, " GLYPH_A " RETURN\n"
    "HIT THE PAPER LANTERN FOR 1UP.",
    C_ORANGE, C_SKY,
    rc_load, rc_start, rc_update, rc_draw, rc_quit, rc_label, rc_query, rc_cheat,
};
