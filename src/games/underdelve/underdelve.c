/* UNDERDELVE - a one-hit exploration platformer through a 6x6 mine.
 * Cartridge 01 of UFO 40. See docs/games/01-underdelve.md. */
#include "underdelve.h"

/* ------------------------------------------------------------------ */
/* tuning                                                               */

#define PW 10           /* player hitbox */
#define PH 14
#define GRAV 0.20f
#define JUMP_V (-3.72f)
#define WALK 1.0f
#define JUMP_DX 1.25f
#define MAX_FALL 4.0f
#define CLIMB_SLOW 0.75f
#define CLIMB_FAST 1.5f
#define START_LANTERNS 6
#define MAX_LANTERNS 6
#define BOSS_HP 14

enum { ST_TITLE, ST_PLAY, ST_DYING, ST_GAMEOVER, ST_MESSAGE, ST_ENDING };
enum { W_PICK = 0, W_ROD, W_HUNGRY };

/* ------------------------------------------------------------------ */
/* persistent run state (this struct is the save file)                  */

typedef struct Run {
    uint32_t version;
    uint8_t room_x, room_y;
    int16_t spawn_x, spawn_y;
    uint16_t money;
    uint8_t lanterns;
    uint8_t weapon;
    uint16_t items;
    uint8_t visited[UD_MAP_H][UD_MAP_W];
    uint8_t taken[UD_MAP_H][UD_MAP_W][25]; /* 200 bits per room */
    uint32_t frames;
    uint16_t deaths;
    uint16_t kills;
    uint8_t boss_dead;
    uint8_t gate_open;
    uint8_t altar_warned;
    uint8_t pad;
} Run;

#define RUN_VERSION 0x55440001u

static Run run;

/* ------------------------------------------------------------------ */
/* entities                                                             */

enum {
    E_NONE, E_MOTH, E_TOAD, E_GRUB, E_SPITTER, E_SWOOPER, E_SACK, E_BEETLE, E_WISP, E_GLOOM, E_BOSS,
    E_STONE, E_SHARD, E_DRIP, E_SPARK, E_BOLT, E_COIN, E_PUFF, E_CANARY, E_DEBRIS
};

typedef struct Ent {
    uint8_t type;
    bool alive;
    float x, y, vx, vy;
    int w, h;
    int dir;
    int hp;
    int t, t2, state;
    float hx, hy; /* home */
    int flash;
    int col;
} Ent;

#define MAX_ENTS 80
static Ent ents[MAX_ENTS];

typedef struct Lift {
    float x, y, px, py; /* current & previous */
    float sx, sy;       /* start */
    int w_px;
    int axis, range_px, period;
    int t;
} Lift;

static Lift lifts[2];
static int n_lifts;

/* ------------------------------------------------------------------ */
/* game state                                                           */

static int state, state_t, title_sel;
static bool has_save;
static char tiles[UD_ROOM_H][UD_ROOM_W + 1];
static uint8_t crystal_contact;

typedef struct Player {
    float x, y, vx, vy;
    int face;
    bool ground, climbing, on_rope;
    int lift_on; /* -1 none */
    int attack; /* frames remaining */
    int rod_cd;
    int anim, blink;
    bool spawn_pending;
    int drown;
} Player;
static Player pl;

static int shake, frame_t, room_t;
static bool arena_sealed;
static int boss_i = -1;
static char msg[160];
static int msg_t, msg_next_state;
static int gloom_timer;
static bool sheet_mode, view_mode;
static int ending_t;
static Rng rng;

/* ------------------------------------------------------------------ */
/* helpers                                                              */

static const RoomDef *R(void) { return &UD_ROOMS[run.room_y][run.room_x]; }
static int zone_of(int ry) { return ry == 0 ? 0 : ry <= 2 ? 1 : ry <= 4 ? 2 : 3; }

static bool taken_get(int rx, int ry, int tx, int ty) {
    int i = ty * UD_ROOM_W + tx;
    return (run.taken[ry][rx][i >> 3] >> (i & 7)) & 1;
}
static void taken_set(int tx, int ty) {
    int i = ty * UD_ROOM_W + tx;
    run.taken[run.room_y][run.room_x][i >> 3] |= (uint8_t)(1u << (i & 7));
}

static char tile(int tx, int ty) {
    if (tx < 0 || tx >= UD_ROOM_W || ty < 0 || ty >= UD_ROOM_H) return 0;
    return tiles[ty][tx];
}

static bool room_exists(int rx, int ry) { return rx >= 0 && ry >= 0 && rx < UD_MAP_W && ry < UD_MAP_H; }

static bool raw_solid(char c) {
    return c == '#' || c == 'B' || c == 'X' || c == 'D' || c == 'I' || c == 'G' || c == 'Z';
}

static bool solid_at(int tx, int ty) {
    bool out_x = tx < 0 || tx >= UD_ROOM_W, out_y = ty < 0 || ty >= UD_ROOM_H;
    if (out_x && out_y) return true;
    if (ty < 0) return !room_exists(run.room_x, run.room_y - 1) ||
                       raw_solid(UD_ROOMS[run.room_y - 1][run.room_x].rows[UD_ROOM_H - 1][tx]);
    if (ty >= UD_ROOM_H) return !room_exists(run.room_x, run.room_y + 1) ||
                                raw_solid(UD_ROOMS[run.room_y + 1][run.room_x].rows[0][tx]);
    if (tx < 0) return !room_exists(run.room_x - 1, run.room_y) || arena_sealed ||
                       raw_solid(UD_ROOMS[run.room_y][run.room_x - 1].rows[ty][UD_ROOM_W - 1]);
    if (tx >= UD_ROOM_W) return !room_exists(run.room_x + 1, run.room_y) ||
                                raw_solid(UD_ROOMS[run.room_y][run.room_x + 1].rows[ty][0]);
    char c = tiles[ty][tx];
    switch (c) {
    case '#': case 'B': case 'X': case 'D': case 'I': return true;
    case 'G': return !run.gate_open;
    case 'Z': return !run.boss_dead;
    default: return false;
    }
}

static bool is_ladder(int tx, int ty) {
    char c = tile(tx, ty);
    if (c == 'H') return true;
    if (c == 'R') return (run.items & UD_ITEM_GLOVES) != 0;
    /* ladders continue across room edges */
    if (ty < 0 || ty >= UD_ROOM_H) {
        int ry = run.room_y + (ty < 0 ? -1 : 1);
        if (!room_exists(run.room_x, ry) || tx < 0 || tx >= UD_ROOM_W) return false;
        char o = UD_ROOMS[ry][run.room_x].rows[ty < 0 ? UD_ROOM_H - 1 : 0][tx];
        return o == 'H' || (o == 'R' && (run.items & UD_ITEM_GLOVES));
    }
    return false;
}

/* one-way surfaces: planks and ladder tops */
static bool oneway_at(int tx, int ty) {
    char c = tile(tx, ty);
    if (c == '=') return true;
    if (c == 'H' && !is_ladder(tx, ty - 1)) return true;
    return false;
}

static bool box_solid(float x, float y, int w, int h) {
    int x0 = (int)floorf(x) >> 4, x1 = ((int)floorf(x) + w - 1) >> 4;
    int y0 = (int)floorf(y) >> 4, y1 = ((int)floorf(y) + h - 1) >> 4;
    if ((int)floorf(x) < 0) x0 = -1;
    if ((int)floorf(y) < 0) y0 = -1;
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

static void puff(float x, float y) {
    Ent *e = spawn(E_PUFF, x - 6, y - 6);
    if (e) { e->w = 12; e->h = 12; e->t = 0; }
}

static void debris(float x, float y, int col, int n) {
    for (int i = 0; i < n; i++) {
        Ent *e = spawn(E_DEBRIS, x, y);
        if (!e) return;
        e->vx = (rng_float(&rng) - 0.5f) * 3.0f;
        e->vy = -1.0f - rng_float(&rng) * 2.5f;
        e->col = col;
        e->t = 40 + rng_range(&rng, 0, 20);
    }
}

static void show_message(const char *text) {
    snprintf(msg, sizeof msg, "%s", text);
    msg_t = 0;
    msg_next_state = state == ST_MESSAGE ? ST_PLAY : state;
    state = ST_MESSAGE;
}

static void save_run(void) {
    run.version = RUN_VERSION;
    game_save_write(game_current_index(), &run, (int)sizeof run);
}

static bool load_run(Run *out) {
    Run tmp;
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n != (int)sizeof tmp || tmp.version != RUN_VERSION) return false;
    *out = tmp;
    return true;
}

/* ------------------------------------------------------------------ */
/* room setup                                                           */

static int enemy_hp(int type) {
    switch (type) {
    case E_TOAD: case E_SPITTER: case E_SACK: return 2;
    default: return 1;
    }
}

static void add_enemy(int type, int tx, int ty) {
    /* never spawn right on top of Mo (room entry or respawn) */
    float dx = (float)(tx * 16 + 8) - (pl.x + PW / 2), dy = (float)(ty * 16 + 8) - (pl.y + PH / 2);
    if (dx * dx + dy * dy < 40.0f * 40.0f) return;
    Ent *e = spawn(type, (float)(tx * 16), (float)(ty * 16));
    if (!e) return;
    e->hp = enemy_hp(type);
    e->dir = rng_chance(&rng, 50) ? 1 : -1;
    switch (type) {
    case E_MOTH: e->w = 12; e->h = 9; e->x += 2; e->y += 3; e->vx = 0.55f * e->dir; e->vy = 0.45f; break;
    case E_TOAD: e->w = 12; e->h = 9; e->x += 2; e->y += 7; e->t = 40 + rng_range(&rng, 0, 60); break;
    case E_GRUB: e->w = 13; e->h = 6; e->x += 1; e->y += 10; break;
    case E_SPITTER: e->w = 14; e->h = 11; e->x += 1; e->y += 5; e->t = 60 + rng_range(&rng, 0, 50); break;
    case E_SWOOPER: e->w = 10; e->h = 10; e->x += 3; e->y += 1; e->hx = e->x; e->hy = e->y; break;
    case E_SACK: e->w = 12; e->h = 12; e->x += 2; e->y += 4; break;
    case E_BEETLE: e->w = 14; e->h = 8; e->x += 1; e->y += 8; break;
    }
    e->hx = e->x;
    e->hy = e->y;
}

static void setup_lifts(void) {
    const RoomDef *rd = R();
    n_lifts = rd->n_lifts;
    for (int i = 0; i < n_lifts; i++) {
        const LiftDef *ld = &rd->lifts[i];
        Lift *l = &lifts[i];
        l->sx = (float)(ld->tx * 16);
        l->sy = (float)(ld->ty * 16);
        l->x = l->px = l->sx;
        l->y = l->py = l->sy;
        l->w_px = ld->w * 16;
        l->axis = ld->axis;
        l->range_px = ld->range * 16;
        l->period = ld->period;
        l->t = 0;
    }
}

static void enter_room(bool reset_spawn) {
    memset(ents, 0, sizeof ents);
    boss_i = -1;
    arena_sealed = false;
    crystal_contact = 0;
    room_t = 0;
    const RoomDef *rd = R();
    for (int y = 0; y < UD_ROOM_H; y++) {
        for (int x = 0; x < UD_ROOM_W; x++) {
            char c = rd->rows[y][x];
            bool gone = taken_get(run.room_x, run.room_y, x, y);
            switch (c) {
            case '$': case '*': case 'X': tiles[y][x] = gone ? '.' : c; break;
            case 'm': add_enemy(E_MOTH, x, y); tiles[y][x] = '.'; break;
            case 't': add_enemy(E_TOAD, x, y); tiles[y][x] = '.'; break;
            case 'g': add_enemy(E_GRUB, x, y); tiles[y][x] = '.'; break;
            case 'p': add_enemy(E_SPITTER, x, y); tiles[y][x] = '.'; break;
            case 'w': add_enemy(E_SWOOPER, x, y); tiles[y][x] = '.'; break;
            case 'k': add_enemy(E_SACK, x, y); tiles[y][x] = '.'; break;
            case 'e': add_enemy(E_BEETLE, x, y); tiles[y][x] = '.'; break;
            case 'P': tiles[y][x] = '.'; break;
            case 'b':
                tiles[y][x] = '.';
                if (!run.boss_dead) {
                    Ent *e = spawn(E_BOSS, (float)(x * 16 - 16), 24);
                    if (e) {
                        e->w = 40; e->h = 30; e->hp = BOSS_HP; e->state = 0; e->t = 60; e->dir = 1;
                        boss_i = (int)(e - ents);
                    }
                }
                break;
            default: tiles[y][x] = c; break;
            }
        }
        tiles[y][UD_ROOM_W] = 0;
    }
    setup_lifts();
    if (run.items & UD_ITEM_CANARY) {
        Ent *e = spawn(E_CANARY, pl.x, pl.y - 12);
        if (e) { e->w = 8; e->h = 6; }
    }
    run.visited[run.room_y][run.room_x] = 1;
    if (reset_spawn) {
        run.spawn_x = (int16_t)pl.x;
        run.spawn_y = (int16_t)pl.y;
        pl.spawn_pending = !pl.ground && !pl.climbing;
    }
    /* the Gloom: a small chance to come calling in unsafe rooms */
    gloom_timer = -1;
    if (!rd->safe && (run.items & UD_ITEM_POT) && rng_chance(&rng, 11)) gloom_timer = 150;
    /* music by depth */
    if (run.room_x == 4 && run.room_y == 5 && !run.boss_dead) music_play(UD_MUS_BOSS);
    else if (run.room_x == 5 && run.room_y == 5) music_play(UD_MUS_WIN);
    else if (run.room_y == 5) music_play(UD_MUS_DEEP);
    else music_play(UD_MUS_MINE);
}

/* ------------------------------------------------------------------ */
/* new game / continue                                                  */

static void place_start(void) {
    const RoomDef *rd = &UD_ROOMS[0][0];
    for (int y = 0; y < UD_ROOM_H; y++)
        for (int x = 0; x < UD_ROOM_W; x++)
            if (rd->rows[y][x] == 'P') {
                pl.x = (float)(x * 16 + 3);
                pl.y = (float)(y * 16 + 2);
            }
}

static void reset_player(void) {
    memset(&pl, 0, sizeof pl);
    pl.face = 1;
    pl.lift_on = -1;
}

static void new_game(void) {
    memset(&run, 0, sizeof run);
    run.lanterns = START_LANTERNS;
    run.weapon = W_PICK;
    reset_player();
    place_start();
    pl.ground = true;
    run.room_x = 0;
    run.room_y = 0;
    state = ST_PLAY;
    state_t = 0;
    game_set_pausable(true);
    enter_room(true);
    run.spawn_x = (int16_t)pl.x;
    run.spawn_y = (int16_t)pl.y;
    save_run();
}

static void continue_game(void) {
    if (!load_run(&run)) { new_game(); return; }
    reset_player();
    pl.x = run.spawn_x;
    pl.y = run.spawn_y;
    pl.ground = true;
    state = ST_PLAY;
    state_t = 0;
    game_set_pausable(true);
    enter_room(false);
}

/* ------------------------------------------------------------------ */
/* player                                                               */

static void kill_player(void) {
    if (state != ST_PLAY) return;
    state = ST_DYING;
    state_t = 0;
    run.deaths++;
    run.lanterns = run.lanterns > 0 ? run.lanterns - 1 : 0;
    shake = 10;
    sfx_play_name("ud_die");
}

static void hazards(void) {
    /* spikes: lower part of the tile */
    int x0 = ((int)pl.x + 1) >> 4, x1 = ((int)pl.x + PW - 2) >> 4;
    int feet = (int)pl.y + PH - 1;
    for (int tx = x0; tx <= x1; tx++) {
        int ty = feet >> 4;
        char c = tile(tx, ty);
        if (c == '^' && (feet & 15) >= 8) { kill_player(); return; }
        if (c == '~' && (feet & 15) >= 7) { kill_player(); return; }
    }
}

static bool can_stand_at(float x, float feet_y) {
    int ty = ((int)feet_y) >> 4;
    int x0 = (int)x >> 4, x1 = ((int)x + PW - 1) >> 4;
    for (int tx = x0; tx <= x1; tx++)
        if (solid_at(tx, ty) || oneway_at(tx, ty)) return true;
    return false;
}

static void move_x(float dx) {
    float step = dx > 0 ? 1.0f : -1.0f;
    float remaining = fabsf(dx);
    while (remaining > 0) {
        float s = remaining >= 1.0f ? step : step * remaining;
        if (box_solid(pl.x + s, pl.y, PW, PH)) {
            /* crystal walls shatter with the fork */
            int tx = (int)(s > 0 ? pl.x + PW : pl.x - 1) >> 4;
            for (int ty = (int)pl.y >> 4; ty <= ((int)pl.y + PH - 1) >> 4; ty++) {
                if (tile(tx, ty) == 'X') {
                    if (run.items & UD_ITEM_FORK) {
                        tiles[ty][tx] = '.';
                        taken_set(tx, ty);
                        debris((float)(tx * 16 + 8), (float)(ty * 16 + 8), C_CYAN, 8);
                        sfx_play_name("ud_shatter");
                        shake = 4;
                    } else if (!crystal_contact) {
                        crystal_contact = 30;
                        sfx_play_name("ud_clink");
                    }
                }
                if (tile(tx, ty) == 'G' && !run.gate_open) {
                    if (run.items & UD_ITEM_KEY) {
                        run.gate_open = 1;
                        sfx_play_name("ud_gate");
                        shake = 12;
                        show_message("THE BRASS KEY TURNS. THE DEEP GATE GRINDS OPEN.");
                    } else if (!crystal_contact) {
                        crystal_contact = 60;
                        show_message("THE DEEP GATE IS LOCKED. A BRASS KEY WOULD FIT.");
                    }
                }
            }
            return;
        }
        pl.x += s;
        remaining -= 1.0f;
    }
}

static bool lift_under(float x, float feet_prev, float feet_new, int *which) {
    for (int i = 0; i < n_lifts; i++) {
        Lift *l = &lifts[i];
        if (x + PW <= l->x || x >= l->x + l->w_px) continue;
        if (feet_prev <= l->y + 0.5f && feet_new >= l->y) { *which = i; return true; }
    }
    return false;
}

static void move_y(float dy) {
    if (dy < 0) {
        float remaining = -dy;
        while (remaining > 0) {
            float s = remaining >= 1.0f ? 1.0f : remaining;
            if (box_solid(pl.x, pl.y - s, PW, PH)) {
                pl.vy = 0;
                /* bumping a hidden block reveals it */
                int ty = ((int)pl.y - 1) >> 4;
                for (int tx = (int)pl.x >> 4; tx <= ((int)pl.x + PW - 1) >> 4; tx++)
                    if (tile(tx, ty) == 'I' && !taken_get(run.room_x, run.room_y, tx, ty)) taken_set(tx, ty);
                return;
            }
            pl.y -= s;
            remaining -= 1.0f;
        }
        pl.ground = false;
        return;
    }
    float remaining = dy;
    pl.ground = false;
    pl.lift_on = -1;
    while (remaining > 0) {
        float s = remaining >= 1.0f ? 1.0f : remaining;
        float feet = pl.y + PH;
        if (box_solid(pl.x, pl.y + s, PW, PH)) {
            pl.y = floorf(pl.y + s);
            while (box_solid(pl.x, pl.y, PW, PH)) pl.y -= 1;
            pl.ground = true;
            break;
        }
        /* one-way surfaces */
        int ty = (int)(feet + s) >> 4;
        int top = ty * 16;
        if (feet <= top && feet + s >= top) {
            bool land = false;
            for (int tx = (int)pl.x >> 4; tx <= ((int)pl.x + PW - 1) >> 4; tx++)
                if (oneway_at(tx, ty)) land = true;
            if (land && !(pl.climbing)) {
                pl.y = (float)(top - PH);
                pl.ground = true;
                break;
            }
        }
        int li;
        if (lift_under(pl.x, feet, feet + s, &li)) {
            pl.y = lifts[li].y - PH;
            pl.ground = true;
            pl.lift_on = li;
            break;
        }
        pl.y += s;
        remaining -= 1.0f;
    }
    if (pl.ground) {
        for (int tx = (int)pl.x >> 4; tx <= ((int)pl.x + PW - 1) >> 4; tx++) {
            int ty = ((int)pl.y + PH) >> 4;
            if (tile(tx, ty) == 'I' && !taken_get(run.room_x, run.room_y, tx, ty)) taken_set(tx, ty);
        }
    }
}

static bool overlapping_ladder(int *ltx) {
    int cx = ((int)pl.x + PW / 2) >> 4;
    for (int ty = ((int)pl.y) >> 4; ty <= ((int)pl.y + PH - 1) >> 4; ty++)
        if (is_ladder(cx, ty)) { *ltx = cx; return true; }
    return false;
}

static void start_climb(int ltx) {
    pl.climbing = true;
    pl.ground = false;
    pl.vx = pl.vy = 0;
    pl.x = (float)(ltx * 16 + 8 - PW / 2);
    pl.attack = 0;
    pl.on_rope = tile(ltx, ((int)pl.y + PH / 2) >> 4) == 'R';
}

static void player_attack_hit(void);

static void update_player(void) {
    int dir = (btn(BTN_RIGHT) ? 1 : 0) - (btn(BTN_LEFT) ? 1 : 0);
    float climb_v = (run.items & UD_ITEM_GLOVES) ? CLIMB_FAST : CLIMB_SLOW;
    if (pl.rod_cd > 0) pl.rod_cd--;

    /* ---- climbing ---- */
    if (pl.climbing) {
        int ltx = ((int)pl.x + PW / 2) >> 4;
        int vdir = (btn(BTN_DOWN) ? 1 : 0) - (btn(BTN_UP) ? 1 : 0);
        if (btnp(BTN_A) && dir != 0) {
            pl.climbing = false;
            pl.vy = -2.6f;
            pl.vx = dir * JUMP_DX;
            pl.face = dir;
            sfx_play_name("ud_jump");
            return;
        }
        /* step off sideways onto ground near foot level */
        if (dir != 0 && vdir == 0) {
            int feet = (int)(pl.y + PH + 0.5f);
            int top = ((feet + 4) >> 4) << 4;
            float nx = pl.x + dir * 6;
            if (iabs(feet - top) <= 4 && can_stand_at(nx, (float)top) && !box_solid(nx, (float)(top - PH), PW, PH)) {
                pl.y = (float)(top - PH);
                pl.climbing = false;
                pl.ground = true;
                pl.face = dir;
                return;
            }
        }
        if (vdir < 0) {
            /* keep climbing while a ladder is at the body */
            float ny = pl.y - climb_v;
            int top_ty = ((int)ny + 2) >> 4;
            if (!is_ladder(ltx, top_ty) && !is_ladder(ltx, ((int)ny + PH - 1) >> 4)) {
                if (pl.on_rope || tile(ltx, ((int)pl.y + PH - 1) >> 4) == 'R') {
                    /* ropes have no top to stand on: just hang there */
                } else {
                    /* reached the top: stand on it */
                    int ty = ((int)(pl.y + PH - 1)) >> 4;
                    pl.y = (float)(ty * 16 - PH);
                    pl.climbing = false;
                    pl.ground = true;
                }
            } else if (!box_solid(pl.x, ny, PW, PH)) {
                pl.y = ny;
                if (!is_ladder(ltx, ((int)pl.y + PH - 1) >> 4) && is_ladder(ltx, ((int)pl.y + PH) >> 4) == false &&
                    oneway_at(ltx, ((int)pl.y + PH) >> 4)) {
                    pl.climbing = false;
                    pl.ground = true;
                }
            }
            /* reached the ladder top exactly */
            int feet_ty = ((int)pl.y + PH) >> 4;
            if (!pl.on_rope && oneway_at(ltx, feet_ty) && ((int)(pl.y + PH) & 15) <= (int)climb_v + 1 &&
                !is_ladder(ltx, ((int)pl.y + PH - 1) >> 4)) {
                pl.y = (float)(feet_ty * 16 - PH);
                pl.climbing = false;
                pl.ground = true;
            }
        } else if (vdir > 0) {
            float ny = pl.y + climb_v;
            if (box_solid(pl.x, ny, PW, PH)) {
                while (box_solid(pl.x, ny, PW, PH)) ny -= 1;
                pl.y = ny;
                pl.climbing = false;
                pl.ground = true;
            } else {
                pl.y = ny;
                /* ran off the bottom of a ladder */
                bool any = false;
                for (int ty = ((int)pl.y) >> 4; ty <= ((int)pl.y + PH) >> 4; ty++) any |= is_ladder(ltx, ty);
                if (!any) pl.climbing = false;
            }
        }
        if (vdir != 0) pl.anim++;
        return;
    }

    /* ---- grab a ladder / rope ---- */
    int ltx;
    if (btn(BTN_UP) && overlapping_ladder(&ltx) && pl.attack == 0) {
        /* avoid re-grabbing a ladder top we stand on */
        int feet_ty = ((int)pl.y + PH) >> 4;
        bool top_only = pl.ground && !is_ladder(ltx, ((int)pl.y + PH - 1) >> 4) && is_ladder(ltx, feet_ty);
        if (!top_only) { start_climb(ltx); return; }
    }
    if (btn(BTN_DOWN) && pl.ground && pl.attack == 0) {
        int cx = ((int)pl.x + PW / 2) >> 4;
        int feet_ty = ((int)pl.y + PH) >> 4;
        if (is_ladder(cx, feet_ty)) {
            start_climb(cx);
            pl.y += 2;
            return;
        }
    }
    if (!pl.ground && (btn(BTN_UP) || btn(BTN_DOWN)) && overlapping_ladder(&ltx)) {
        char c = tile(ltx, ((int)pl.y + PH / 2) >> 4);
        if (c == 'R' || c == 'H' || is_ladder(ltx, ((int)pl.y + PH / 2) >> 4)) { start_climb(ltx); return; }
    }

    /* ---- attack ---- */
    if (btnp(BTN_B) && pl.attack == 0) {
        if (run.weapon == W_ROD) {
            if (pl.rod_cd == 0) {
                int bolts = 0;
                for (int i = 0; i < MAX_ENTS; i++) bolts += ents[i].alive && ents[i].type == E_BOLT;
                if (bolts < 2) {
                    Ent *b = spawn(E_BOLT, pl.face > 0 ? pl.x + PW : pl.x - 8, pl.y + 3);
                    if (b) { b->w = 8; b->h = 5; b->vx = 3.2f * pl.face; b->t = 70; }
                    pl.rod_cd = 18;
                    pl.attack = 10;
                    sfx_play_name("ud_bolt");
                }
            }
        } else {
            pl.attack = 18;
            sfx_play_name("ud_swing");
        }
    }
    if (pl.attack > 0) {
        pl.attack--;
        if (run.weapon != W_ROD && pl.attack <= 12 && pl.attack >= 5) player_attack_hit();
    }

    /* ---- walking / jumping ---- */
    if (pl.ground) {
        if (pl.attack > 0 && run.weapon != W_ROD) pl.vx = 0;
        else {
            pl.vx = dir * WALK;
            if (dir) pl.face = dir;
        }
        if (btnp(BTN_A) && !(pl.attack > 0 && run.weapon != W_ROD)) {
            pl.vy = JUMP_V;
            pl.vx = dir * JUMP_DX;
            pl.ground = false;
            pl.lift_on = -1;
            sfx_play_name("ud_jump");
        }
    }
    bool was_ground = pl.ground;
    if (pl.vx != 0) move_x(pl.vx);
    if (!pl.ground || pl.vy < 0) {
        pl.vy += GRAV;
        if (pl.vy > MAX_FALL) pl.vy = MAX_FALL;
    } else {
        pl.vy = 0;
    }
    /* stay grounded check (walking off ledges keeps momentum) */
    if (pl.ground && pl.vy >= 0) {
        if (pl.lift_on >= 0) {
            Lift *l = &lifts[pl.lift_on];
            if (pl.x + PW <= l->x || pl.x >= l->x + l->w_px) { pl.ground = false; pl.lift_on = -1; }
        } else if (!can_stand_at(pl.x, pl.y + PH)) {
            pl.ground = false;
        }
        if (!pl.ground) pl.vy = 0.3f;
    }
    if (!pl.ground) move_y(pl.vy);
    if (!was_ground && pl.ground) {
        pl.vy = 0;
        sfx_play_name("ud_land");
        if (pl.spawn_pending) {
            pl.spawn_pending = false;
            run.spawn_x = (int16_t)pl.x;
            run.spawn_y = (int16_t)pl.y;
        }
    }
    if (pl.ground && pl.vx != 0) {
        pl.anim++;
    } else if (pl.ground) {
        pl.anim = 0;
    }
}

/* ------------------------------------------------------------------ */
/* combat                                                               */

static void drop_coin(float x, float y) {
    if (!rng_chance(&rng, 55)) return;
    Ent *c = spawn(E_COIN, x, y);
    if (c) { c->w = 6; c->h = 6; c->vy = -2.2f; c->vx = (rng_float(&rng) - 0.5f) * 1.5f; c->t = 480; }
}

static void enemy_die(Ent *e) {
    /* copy first: the puff below may recycle this slot */
    float cx = e->x + e->w / 2, cy = e->y + e->h / 2, top = e->y;
    int type = e->type;
    e->alive = false;
    run.kills++;
    puff(cx, cy);
    sfx_play_name("ud_kill");
    if (type != E_WISP) drop_coin(cx - 3, top);
}

static int weapon_damage(void) { return run.weapon == W_HUNGRY ? 2 : 1; }

static bool hurt_enemy(Ent *e, int dmg, float from_x) {
    if (e->flash > 0) return false;
    switch (e->type) {
    case E_GLOOM: return false;
    case E_BEETLE: {
        /* the shell is armoured in front: only hits from behind count */
        float cx = e->x + e->w / 2;
        bool from_front = (from_x > cx && e->dir > 0) || (from_x < cx && e->dir < 0);
        if (from_front) { sfx_play_name("ud_clink"); e->flash = 10; return false; }
        break;
    }
    case E_BOSS:
        if (e->state != 3) { sfx_play_name("ud_clink"); e->flash = 8; return false; }
        e->hp -= dmg;
        e->flash = 14;
        shake = 4;
        sfx_play_name("ud_bosshit");
        if (e->hp <= 0) { e->state = 5; e->t = 150; music_stop(); }
        return true;
    case E_SACK:
        if (e->state == 0) e->state = 1;
        break;
    }
    e->hp -= dmg;
    e->flash = 8;
    if (e->hp <= 0) enemy_die(e);
    else sfx_play_name("ud_clink");
    return true;
}

static void player_attack_hit(void) {
    int hx = pl.face > 0 ? (int)pl.x + PW : (int)pl.x - 14;
    int hy = (int)pl.y + 1, hw = 14, hh = 11;
    for (int i = 0; i < MAX_ENTS; i++) {
        Ent *e = &ents[i];
        if (!e->alive) continue;
        if (e->type < E_MOTH || e->type > E_BOSS) continue;
        if (rects_overlap(hx, hy, hw, hh, (int)e->x, (int)e->y, e->w, e->h)) hurt_enemy(e, weapon_damage(), pl.x + PW / 2);
    }
}

/* ------------------------------------------------------------------ */
/* enemies                                                              */

static float player_cx(void) { return pl.x + PW / 2; }
static float player_cy(void) { return pl.y + PH / 2; }

static bool ent_ground(Ent *e) {
    int y = (int)(e->y + e->h);
    for (int x = (int)e->x; x < (int)e->x + e->w; x += 4) {
        int tx = x >> 4, ty = y >> 4;
        if (solid_at(tx, ty) || oneway_at(tx, ty)) return true;
    }
    int tx = ((int)e->x + e->w - 1) >> 4, ty = y >> 4;
    return solid_at(tx, ty) || oneway_at(tx, ty);
}

static void ent_fall(Ent *e, float grav) {
    e->vy += grav;
    if (e->vy > 4) e->vy = 4;
    float ny = e->y + e->vy;
    if (e->vy > 0) {
        int feet = (int)(e->y + e->h), nfeet = (int)(ny + e->h);
        int ty = nfeet >> 4;
        if ((feet >> 4) != ty || (feet & 15) == 0) {
            for (int x = (int)e->x; x < (int)e->x + e->w; x += 3) {
                if (solid_at(x >> 4, ty) || (oneway_at(x >> 4, ty) && feet <= ty * 16)) {
                    e->y = (float)(ty * 16 - e->h);
                    e->vy = 0;
                    return;
                }
            }
        }
    } else if (box_solid(e->x, ny, e->w, e->h)) {
        e->vy = 0;
        return;
    }
    e->y = ny;
}

static bool walker_step(Ent *e, float speed) {
    float nx = e->x + speed * e->dir;
    int ahead_x = e->dir > 0 ? (int)(nx + e->w) : (int)nx - 1;
    int foot_y = (int)(e->y + e->h);
    bool wall = box_solid(nx, e->y, e->w, e->h);
    bool floor_ahead = solid_at(ahead_x >> 4, foot_y >> 4) || oneway_at(ahead_x >> 4, foot_y >> 4);
    if (wall || !floor_ahead || nx < 0 || nx + e->w > 320) {
        e->dir = -e->dir;
        return false;
    }
    e->x = nx;
    return true;
}

static void update_boss(Ent *e) {
    /* states: 0 float, 1 warn, 2 slam, 3 grounded (vulnerable), 4 rise, 5 dying */
    const float top_y = 22, floor_y = 144 - 30;
    e->t--;
    switch (e->state) {
    case 0:
        e->x += 0.9f * e->dir;
        if (e->x < 24) e->dir = 1;
        if (e->x > 320 - 24 - e->w) e->dir = -1;
        e->y = top_y + sinf(frame_t * 0.06f) * 3;
        if (e->t % 70 == 0) {
            for (int s = -1; s <= 1; s += 2) {
                Ent *p = spawn(E_SHARD, e->x + e->w / 2 - 3 + s * 10, e->y + e->h - 4);
                if (p) { p->w = 5; p->h = 6; p->vx = s * 0.6f; p->vy = 0.5f; }
            }
            sfx_play_name("ud_spit");
        }
        if (e->t % 300 == 150) {
            int wisps = 0;
            for (int i = 0; i < MAX_ENTS; i++) wisps += ents[i].alive && ents[i].type == E_WISP;
            if (wisps < 2) {
                Ent *w = spawn(E_WISP, e->x + e->w / 2, e->y + e->h / 2);
                if (w) { w->w = 10; w->h = 10; w->hp = 1; w->t = rng_range(&rng, 0, 100); }
            }
        }
        if (e->t <= 0) { e->state = 1; e->t = 36; }
        break;
    case 1:
        e->x += (frame_t % 4 < 2) ? 1 : -1;
        if (e->t <= 0) { e->state = 2; e->vy = 0; }
        break;
    case 2:
        e->vy += 0.5f;
        e->y += e->vy;
        if (e->y >= floor_y) {
            e->y = floor_y;
            e->state = 3;
            e->t = 110;
            shake = 14;
            sfx_play_name("ud_slam");
            for (int s = -1; s <= 1; s += 2) {
                Ent *p = spawn(E_SPARK, e->x + e->w / 2 - 3, 138);
                if (p) { p->w = 6; p->h = 6; p->vx = s * 2.0f; p->t = 200; }
            }
            debris(e->x + e->w / 2, 144, C_GREY, 10);
        }
        break;
    case 3:
        if (e->t <= 0) { e->state = 4; }
        break;
    case 4:
        e->y -= 1.2f;
        if (e->y <= top_y) { e->y = top_y; e->state = 0; e->t = 260 + rng_range(&rng, 0, 80); }
        break;
    case 5:
        if (e->t % 8 == 0) {
            puff(e->x + rng_range(&rng, 0, e->w), e->y + rng_range(&rng, 0, e->h));
            sfx_play_name("ud_kill");
            shake = 6;
        }
        if (e->t <= 0) {
            e->alive = false;
            run.boss_dead = 1;
            debris(e->x + e->w / 2, e->y + e->h / 2, C_PINK, 24);
            sfx_play_name("ud_boom");
            shake = 24;
            arena_sealed = false;
            show_message("THE OLD LODE CRUMBLES. SUNLIGHT SPILLS THROUGH THE EAST WALL!");
            music_play(UD_MUS_WIN);
            save_run();
        }
        break;
    }
}

static void update_ents(void) {
    for (int i = 0; i < MAX_ENTS; i++) {
        Ent *e = &ents[i];
        if (!e->alive) continue;
        if (e->flash > 0) e->flash--;
        e->t2++;
        switch (e->type) {
        case E_MOTH: {
            float nx = e->x + e->vx, ny = e->y + e->vy + sinf(e->t2 * 0.15f) * 0.3f;
            if (box_solid(nx, e->y, e->w, e->h) || nx < 0 || nx + e->w > 320) e->vx = -e->vx;
            else e->x = nx;
            if (box_solid(e->x, ny, e->w, e->h) || ny < 0 || ny + e->h > 160) e->vy = -e->vy;
            else e->y = ny;
            break;
        }
        case E_TOAD:
            if (ent_ground(e) && e->vy >= 0) {
                e->vx = 0;
                if (--e->t <= 0) {
                    e->dir = rng_chance(&rng, 55) ? (player_cx() > e->x ? 1 : -1) : (rng_chance(&rng, 50) ? 1 : -1);
                    e->vy = -3.0f;
                    e->vx = 1.0f * e->dir;
                    e->t = 50 + rng_range(&rng, 0, 60);
                }
            }
            if (e->vx != 0) {
                float nx = e->x + e->vx;
                if (box_solid(nx, e->y, e->w, e->h) || nx < 0 || nx + e->w > 320) e->vx = -e->vx, e->dir = -e->dir;
                else e->x = nx;
            }
            ent_fall(e, 0.18f);
            if (ent_ground(e)) e->vx = 0;
            if (tile((int)(e->x + e->w / 2) >> 4, (int)(e->y + e->h - 1) >> 4) == '~') enemy_die(e);
            break;
        case E_GRUB:
            walker_step(e, 0.35f);
            break;
        case E_SPITTER:
            e->dir = player_cx() > e->x + e->w / 2 ? 1 : -1;
            if (--e->t <= 0) {
                Ent *s = spawn(E_STONE, e->x + e->w / 2 - 3, e->y);
                if (s) {
                    s->w = 5; s->h = 5;
                    float dx = player_cx() - (e->x + e->w / 2);
                    s->vx = fclamp(dx / 55.0f, -2.2f, 2.2f);
                    if (fabsf(s->vx) < 0.6f) s->vx = 0.6f * e->dir;
                    s->vy = -2.6f;
                }
                sfx_play_name("ud_spit");
                e->t = 120;
                e->state = 12;
            }
            if (e->state > 0) e->state--;
            break;
        case E_SWOOPER:
            if (e->state == 0) {
                if (fabsf(player_cx() - (e->x + e->w / 2)) < 56 && player_cy() > e->y) {
                    e->state = 1;
                    e->dir = player_cx() > e->x ? 1 : -1;
                    e->vy = 2.4f;
                    e->vx = 1.1f * e->dir;
                }
            } else if (e->state == 1) {
                e->x += e->vx;
                e->y += e->vy;
                e->vy -= 0.045f;
                if (e->vy <= 0 || e->y > 150 || box_solid(e->x, e->y, e->w, e->h)) e->state = 2;
            } else {
                float dx = e->hx - e->x, dy = e->hy - e->y;
                float d = sqrtf(dx * dx + dy * dy);
                if (d < 1.5f) { e->x = e->hx; e->y = e->hy; e->state = 0; }
                else { e->x += dx / d * 1.2f; e->y += dy / d * 1.2f; }
            }
            break;
        case E_SACK:
            if (e->state == 0) {
                if (fabsf(player_cx() - (e->x + e->w / 2)) < 44 && fabsf(player_cy() - (e->y + e->h / 2)) < 24) e->state = 1;
            } else {
                if (ent_ground(e) && e->vy >= 0) {
                    e->vx = 0;
                    if (++e->t > 34) {
                        e->t = 0;
                        e->dir = player_cx() > e->x ? 1 : -1;
                        e->vy = -2.8f;
                        e->vx = 1.15f * e->dir;
                    }
                }
                if (e->vx != 0) {
                    float nx = e->x + e->vx;
                    if (box_solid(nx, e->y, e->w, e->h) || nx < 0 || nx + e->w > 320) e->vx = 0;
                    else e->x = nx;
                }
                ent_fall(e, 0.18f);
            }
            break;
        case E_BEETLE:
            walker_step(e, 0.5f);
            break;
        case E_WISP: {
            float dx = player_cx() - (e->x + e->w / 2), dy = player_cy() - (e->y + e->h / 2);
            float d = sqrtf(dx * dx + dy * dy) + 0.01f;
            e->x += dx / d * 0.55f;
            e->y += dy / d * 0.55f + sinf(e->t2 * 0.12f) * 0.6f;
            break;
        }
        case E_GLOOM: {
            float dx = player_cx() - (e->x + e->w / 2), dy = player_cy() - (e->y + e->h / 2);
            float d = sqrtf(dx * dx + dy * dy) + 0.01f;
            float sp = e->t2 < 60 ? 0.0f : 0.38f;
            e->x += dx / d * sp;
            e->y += dy / d * sp + sinf(e->t2 * 0.05f) * 0.2f;
            break;
        }
        case E_BOSS:
            update_boss(e);
            break;
        case E_STONE:
            e->vy += 0.12f;
            e->x += e->vx;
            e->y += e->vy;
            if (box_solid(e->x, e->y, e->w, e->h) || e->y > 170 || e->x < -10 || e->x > 330) {
                e->alive = false;
                debris(e->x, e->y, C_GREY, 3);
            }
            break;
        case E_SHARD:
            e->vy += 0.06f;
            e->x += e->vx;
            e->y += e->vy;
            if (box_solid(e->x, e->y, e->w, e->h) || e->y > 170) { e->alive = false; debris(e->x, e->y, C_PINK, 3); }
            break;
        case E_DRIP:
            e->y += 2.3f;
            if (box_solid(e->x, e->y, e->w, e->h) || e->y > 170) {
                e->alive = false;
                debris(e->x, e->y, C_CYAN, 2);
                if (rects_overlap((int)e->x - 40, (int)e->y - 40, 80, 80, (int)pl.x, (int)pl.y, PW, PH)) sfx_play_name("ud_drip");
            }
            break;
        case E_SPARK:
            e->x += e->vx;
            if (--e->t <= 0 || e->x < -8 || e->x > 328) e->alive = false;
            break;
        case E_BOLT: {
            e->x += e->vx;
            if (--e->t <= 0 || box_solid(e->x, e->y, e->w, e->h)) { e->alive = false; debris(e->x, e->y, C_CYAN, 3); break; }
            for (int j = 0; j < MAX_ENTS; j++) {
                Ent *o = &ents[j];
                if (!o->alive || o->type < E_MOTH || o->type > E_BOSS || o->type == E_GLOOM) continue;
                if (rects_overlap((int)e->x, (int)e->y, e->w, e->h, (int)o->x, (int)o->y, o->w, o->h)) {
                    hurt_enemy(o, 1, e->x - e->vx * 4);
                    e->alive = false;
                    break;
                }
            }
            break;
        }
        case E_COIN:
            e->vy += 0.15f;
            if (e->vy > 3) e->vy = 3;
            e->x += e->vx;
            if (box_solid(e->x, e->y, e->w, e->h)) e->x -= e->vx, e->vx = 0;
            {
                float ny = e->y + e->vy;
                if (e->vy > 0 && (box_solid(e->x, ny, e->w, e->h) ||
                                  oneway_at(((int)e->x + 3) >> 4, ((int)ny + e->h) >> 4))) {
                    e->vy = -e->vy * 0.4f;
                    if (fabsf(e->vy) < 0.5f) e->vy = 0;
                    e->vx *= 0.6f;
                } else {
                    e->y = ny;
                }
            }
            if (--e->t <= 0 || e->y > 170) e->alive = false;
            break;
        case E_PUFF:
            if (++e->t > 18) e->alive = false;
            break;
        case E_DEBRIS:
            e->vy += 0.15f;
            e->x += e->vx;
            e->y += e->vy;
            if (--e->t <= 0) e->alive = false;
            break;
        case E_CANARY: {
            /* follow Mo; hunt wisps */
            Ent *target = NULL;
            for (int j = 0; j < MAX_ENTS; j++)
                if (ents[j].alive && ents[j].type == E_WISP) { target = &ents[j]; break; }
            float tx = target ? target->x + 2 : pl.x - 10 * pl.face;
            float ty = target ? target->y + 2 : pl.y - 14 + sinf(frame_t * 0.1f) * 3;
            float dx = tx - e->x, dy = ty - e->y;
            float d = sqrtf(dx * dx + dy * dy) + 0.01f;
            float sp = target ? 2.0f : fminf(2.5f, d * 0.08f);
            e->x += dx / d * sp;
            e->y += dy / d * sp;
            e->dir = dx > 0 ? 1 : -1;
            if (target && rects_overlap((int)e->x, (int)e->y, e->w, e->h, (int)target->x, (int)target->y, target->w, target->h)) {
                enemy_die(target);
                sfx_play_name("ud_peck");
            }
            break;
        }
        }
    }
}

static void spawn_drips(void) {
    for (int y = 0; y < UD_ROOM_H; y++)
        for (int x = 0; x < UD_ROOM_W; x++)
            if (tiles[y][x] == 'D' && (room_t + x * 23) % 64 == 0) {
                /* the hitbox is wider than the droplet: the splash still gets you */
                Ent *d = spawn(E_DRIP, (float)(x * 16 + 4), (float)(y * 16 + 14));
                if (d) { d->w = 8; d->h = 5; }
            }
}

static void check_contacts(void) {
    int px = (int)pl.x + 1, py = (int)pl.y + 1, pw = PW - 2, ph = PH - 2;
    for (int i = 0; i < MAX_ENTS; i++) {
        Ent *e = &ents[i];
        if (!e->alive) continue;
        switch (e->type) {
        case E_COIN:
            if (rects_overlap(px, py, pw, ph, (int)e->x, (int)e->y, e->w, e->h)) {
                e->alive = false;
                run.money = (uint16_t)imin(9999, run.money + 20);
                sfx_play_name("ud_coin");
            }
            break;
        case E_DRIP:
            if (rects_overlap(px, py, pw, ph, (int)e->x, (int)e->y, e->w, e->h)) {
                e->alive = false;
                if (run.items & UD_ITEM_POT) { sfx_play_name("ud_clang"); debris(e->x, e->y, C_CYAN, 3); }
                else kill_player();
            }
            break;
        case E_PUFF: case E_DEBRIS: case E_CANARY: case E_BOLT:
            break;
        case E_SACK:
            if (e->state == 0) break; /* still pretending */
            /* fall through */
        default:
            if (e->type == E_BOSS && e->state == 5) break;
            {
                int inset = e->type == E_BOSS ? 4 : 2;
                if (rects_overlap(px, py, pw, ph, (int)e->x + inset, (int)e->y + inset, e->w - inset * 2, e->h - inset * 2))
                    kill_player();
            }
            break;
        }
        if (state != ST_PLAY) return;
    }
}

/* ------------------------------------------------------------------ */
/* pickups, shops, specials                                             */

static void collect_tiles(void) {
    int x0 = ((int)pl.x) >> 4, x1 = ((int)pl.x + PW - 1) >> 4;
    int y0 = ((int)pl.y) >> 4, y1 = ((int)pl.y + PH - 1) >> 4;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++) {
            char c = tile(tx, ty);
            if (c == '$' || c == '*') {
                if (!rects_overlap((int)pl.x, (int)pl.y, PW, PH, tx * 16 + 3, ty * 16 + 3, 10, 10)) continue;
                tiles[ty][tx] = '.';
                taken_set(tx, ty);
                run.money = (uint16_t)imin(9999, run.money + (c == '$' ? 50 : 100));
                sfx_play_name(c == '$' ? "ud_coin" : "ud_gem");
            } else if (c == 'c' && !taken_get(run.room_x, run.room_y, tx, ty)) {
                taken_set(tx, ty);
                uint16_t it = R()->chest_item;
                run.items |= it;
                sfx_play_name("ud_item");
                save_run();
                if (it == UD_ITEM_FORK) show_message("YOU FOUND THE TUNING FORK! CRYSTAL WALLS WILL SHATTER AT A TOUCH.");
                else if (it == UD_ITEM_GLOVES) show_message("YOU FOUND CLIMBING GLOVES! GRIP ROPES AND CLIMB TWICE AS FAST.");
                else if (it == UD_ITEM_KEY) show_message("YOU FOUND THE BRASS KEY! IT MUST OPEN THE DEEP GATE.");
            } else if (c == 'n' && !taken_get(run.room_x, run.room_y, tx, ty)) {
                taken_set(tx, ty);
                run.items |= UD_ITEM_CANARY;
                Ent *e = spawn(E_CANARY, (float)(tx * 16 + 4), (float)(ty * 16 + 4));
                if (e) { e->w = 8; e->h = 6; }
                sfx_play_name("ud_item");
                game_award(GOAL_BEACON);
                save_run();
                show_message("YOU FREED THE CANARY! IT WILL PECK WISPS OUT OF THE AIR FOR YOU.");
            } else if (c == 's') {
                state = ST_ENDING;
                state_t = 0;
                ending_t = 0;
                game_award(GOAL_SAUCER);
                if (run.deaths == 0) game_award(GOAL_ALIEN);
                game_save_erase(game_current_index());
                music_restart(UD_MUS_WIN);
            }
        }
}

static bool player_near_tile(char c, int *otx, int *oty) {
    int cx = (int)player_cx() >> 4;
    int y0 = ((int)pl.y) >> 4, y1 = ((int)pl.y + PH - 1) >> 4;
    for (int ty = y0; ty <= y1; ty++)
        for (int dx = -1; dx <= 1; dx++)
            if (tile(cx + dx, ty) == c) {
                if (fabsf(player_cx() - ((cx + dx) * 16 + 8)) < 12) { *otx = cx + dx; *oty = ty; return true; }
            }
    return false;
}

static void shop_and_specials(void) {
    int tx, ty;
    const RoomDef *rd = R();
    if (!btnp(BTN_UP)) return;
    if (player_near_tile('1', &tx, &ty)) {
        int it = rd->shop_item;
        bool owned = (it == UD_ITEM_ROD) ? run.weapon == W_ROD : (run.items & it) != 0;
        if (owned) { sfx_play_name("ud_nope"); return; }
        if (run.money < rd->shop_price) {
            sfx_play_name("ud_nope");
            show_message("NOT ENOUGH ORE. COME BACK WITH MORE.");
            return;
        }
        run.money = (uint16_t)(run.money - rd->shop_price);
        sfx_play_name("ud_buy");
        if (it == UD_ITEM_ROD) {
            run.weapon = W_ROD;
            run.items |= UD_ITEM_ROD;
            show_message("TRADED YOUR PICK FOR THE SPARK ROD! PRESS B TO FIRE SPARKS.");
        } else {
            run.items |= (uint16_t)it;
            if (it == UD_ITEM_POT) show_message("BOUGHT THE COPPER POT! CEILING DRIPS WILL BOUNCE OFF YOUR HEAD.");
            else show_message("BOUGHT THE GEAR CRANK! THE OLD MINE LIFTS WILL RUN AGAIN.");
        }
        save_run();
        return;
    }
    if (player_near_tile('V', &tx, &ty)) {
        if (run.lanterns >= MAX_LANTERNS) { show_message("YOUR LANTERN BELT IS FULL."); return; }
        if (run.money < 100) { sfx_play_name("ud_nope"); show_message("A LANTERN COSTS 100 ORE."); return; }
        run.money = (uint16_t)(run.money - 100);
        run.lanterns++;
        sfx_play_name("ud_buy");
        save_run();
        return;
    }
    if (player_near_tile('a', &tx, &ty) && !taken_get(run.room_x, run.room_y, tx, ty)) {
        if (!run.altar_warned) {
            run.altar_warned = 1;
            show_message("THE HUNGRY PICK HITS TWICE AS HARD, BUT IT EATS A LANTERN. PRESS UP AGAIN TO TAKE IT.");
            return;
        }
        if (run.lanterns <= 1) { show_message("THE PICK WANTS A LANTERN YOU CANNOT SPARE."); return; }
        run.lanterns--;
        run.weapon = W_HUNGRY;
        run.items |= UD_ITEM_HUNGRY;
        taken_set(tx, ty);
        sfx_play_name("ud_item");
        shake = 8;
        save_run();
        show_message("A LANTERN GUTTERS OUT. THE HUNGRY PICK IS YOURS.");
    }
}

/* ------------------------------------------------------------------ */
/* room transitions                                                     */

static void check_edges(void) {
    int nrx = run.room_x, nry = run.room_y;
    float cx = player_cx(), cy = player_cy();
    if (cx < 0 && room_exists(run.room_x - 1, run.room_y)) { nrx--; pl.x += 320; }
    else if (cx >= 320 && room_exists(run.room_x + 1, run.room_y)) { nrx++; pl.x -= 320; }
    else if (cy < 0 && room_exists(run.room_x, run.room_y - 1)) { nry--; pl.y += 160; }
    else if (cy >= 160 && room_exists(run.room_x, run.room_y + 1)) { nry++; pl.y -= 160; }
    else if (pl.y > 200) { kill_player(); return; }
    if (nrx != run.room_x || nry != run.room_y) {
        run.room_x = (uint8_t)nrx;
        run.room_y = (uint8_t)nry;
        enter_room(true);
        save_run();
    }
}

static void update_lifts(void) {
    bool powered = (run.items & UD_ITEM_CRANK) != 0;
    for (int i = 0; i < n_lifts; i++) {
        Lift *l = &lifts[i];
        l->px = l->x;
        l->py = l->y;
        if (powered) {
            l->t++;
            int span = l->range_px * l->period / 16;
            if (span < 1) span = 1;
            int ph = l->t % (span * 2 + 120);
            float k;
            if (ph < 60) k = 0;
            else if (ph < 60 + span) k = (float)(ph - 60) / span;
            else if (ph < 120 + span) k = 1;
            else k = 1.0f - (float)(ph - 120 - span) / span;
            /* ease */
            k = k * k * (3 - 2 * k);
            if (l->axis == 0) l->x = l->sx + k * l->range_px;
            else l->y = l->sy - k * l->range_px;
        }
        if (pl.lift_on == i && pl.ground && !pl.climbing) {
            float dx = l->x - l->px, dy = l->y - l->py;
            if (dx != 0) move_x(dx);
            pl.y = l->y - PH;
            (void)dy;
        }
    }
}

/* ------------------------------------------------------------------ */
/* update                                                               */

static void play_update(void) {
    run.frames++;
    room_t++;
    frame_t++;
    if (crystal_contact > 0) crystal_contact--;
    update_lifts();
    update_player();
    if (state != ST_PLAY) return;
    hazards();
    if (state != ST_PLAY) return;
    spawn_drips();
    update_ents();
    check_contacts();
    if (state != ST_PLAY) return;
    collect_tiles();
    if (state != ST_PLAY) return;
    shop_and_specials();
    /* seal the arena once Mo walks in */
    if (boss_i >= 0 && ents[boss_i].alive && pl.x > 40 && !arena_sealed) {
        arena_sealed = true;
        sfx_play_name("ud_door");
        shake = 6;
    }
    if (gloom_timer > 0 && --gloom_timer == 0) {
        float gx = player_cx() < 160 ? 280 : 16, gy = player_cy() < 80 ? 120 : 10;
        Ent *g = spawn(E_GLOOM, gx, gy);
        if (g) { g->w = 16; g->h = 16; g->hp = 99; }
        sfx_play_name("ud_gloom");
    }
    check_edges();
}

static void title_update(void) {
    state_t++;
    game_set_pausable(false);
    int n = has_save ? 2 : 1;
    if (btnp(BTN_UP) || btnp(BTN_DOWN)) { title_sel = (title_sel + 1) % n; sfx_play_name("ui_move"); }
    if (btnp(BTN_A) || btnp(BTN_START)) {
        sfx_play_name("ui_ok");
        input_consume();
        if (has_save && title_sel == 0) continue_game();
        else new_game();
    }
    if (btnp(BTN_B)) game_exit_to_library();
}

static void ud_update(void) {
    if (shake > 0) shake--;
    switch (state) {
    case ST_TITLE: title_update(); break;
    case ST_PLAY: play_update(); break;
    case ST_MESSAGE:
        msg_t++;
        frame_t++;
        if (msg_t > 20 && (btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_UP))) {
            state = ST_PLAY;
            input_consume();
        }
        break;
    case ST_DYING:
        state_t++;
        frame_t++;
        update_ents();
        if (state_t == 70) {
            if (run.lanterns == 0) {
                state = ST_GAMEOVER;
                state_t = 0;
                game_save_erase(game_current_index());
                music_restart(UD_MUS_OVER);
                game_set_pausable(false);
            } else {
                reset_player();
                pl.x = run.spawn_x;
                pl.y = run.spawn_y;
                pl.ground = true;
                state = ST_PLAY;
                enter_room(false);
                save_run();
            }
        }
        break;
    case ST_GAMEOVER:
        state_t++;
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) {
            state = ST_TITLE;
            has_save = false;
            title_sel = 0;
            state_t = 0;
            music_play(UD_MUS_TITLE);
        }
        break;
    case ST_ENDING:
        state_t++;
        frame_t++;
        game_set_pausable(false);
        if (state_t > 240 && (btnp(BTN_A) || btnp(BTN_START))) {
            state = ST_TITLE;
            has_save = false;
            title_sel = 0;
            state_t = 0;
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */

typedef struct { uint8_t bg, n, d, s, g, ooze_a, ooze_b, ooze_c, ooze_d; } ZonePal;
static const ZonePal ZONES[4] = {
    {C_INK, C_NIGHT, C_BROWN, C_TAN, C_EARTH, C_LIME, C_LEAF, C_JADE, C_FOREST},  /* upper workings */
    {C_INK, C_TEAL, C_FOREST, C_JADE, C_LEAF, C_LIME, C_LEAF, C_JADE, C_FOREST},  /* glowcap hollows */
    {C_INK, C_NAVY, C_PURPLE, C_VIOLET, C_MAGENTA, C_LIME, C_LEAF, C_JADE, C_FOREST}, /* crystal veins */
    {C_INK, C_MAROON, C_WINE, C_RED, C_ORANGE, C_YELLOW, C_AMBER, C_ORANGE, C_RED}, /* ember deep */
};
static uint8_t zmap[PAL_COUNT], omap[PAL_COUNT];

static void build_zone_maps(int z) {
    const ZonePal *p = &ZONES[z];
    pal_identity(zmap);
    zmap[C_NIGHT] = p->n;
    zmap[C_DUSK] = p->d;
    zmap[C_SLATE] = p->s;
    zmap[C_GREY] = p->g;
    pal_identity(omap);
    omap[C_LIME] = p->ooze_a;
    omap[C_LEAF] = p->ooze_b;
    omap[C_JADE] = p->ooze_c;
    omap[C_FOREST] = p->ooze_d;
}

static void draw_tile_edges(int x, int y, int tx, int ty) {
    /* lip highlight and dark outline where solid rock meets open air */
    const ZonePal *p = &ZONES[zone_of(run.room_y)];
    bool up = solid_at(tx, ty - 1) || tile(tx, ty - 1) == 'F', dn = solid_at(tx, ty + 1) || tile(tx, ty + 1) == 'F';
    bool lf = solid_at(tx - 1, ty) || tile(tx - 1, ty) == 'F', rt = solid_at(tx + 1, ty) || tile(tx + 1, ty) == 'F';
    if (!up) { gfx_hline(x, x + 15, y, C_INK); gfx_hline(x, x + 15, y + 1, p->g); gfx_hline(x, x + 15, y + 2, p->s); }
    if (!dn) { gfx_hline(x, x + 15, y + 15, C_INK); gfx_hline(x, x + 15, y + 14, p->n); }
    if (!lf) { gfx_vline(x, y, y + 15, C_INK); gfx_vline(x + 1, y + (up ? 0 : 1), y + 14, p->s); }
    if (!rt) { gfx_vline(x + 15, y, y + 15, C_INK); gfx_vline(x + 14, y + (up ? 0 : 3), y + 14, p->n); }
}

static void draw_room(void) {
    int z = zone_of(run.room_y);
    const ZonePal *p = &ZONES[z];
    build_zone_maps(z);
    gfx_rect(0, 0, 320, 160, p->bg);
    /* background texture */
    for (int ty = 0; ty < UD_ROOM_H; ty++)
        for (int tx = 0; tx < UD_ROOM_W; tx++) {
            uint32_t h = (uint32_t)(tx * 73 + ty * 151 + run.room_x * 997 + run.room_y * 389);
            h = (h ^ (h >> 5)) * 2654435761u;
            if ((h >> 28) < 6) spr_draw_ex(&ud_spr[S_T_BGROCK], tx * 16, ty * 16, (h >> 20) & 1, zmap, -1);
        }
    /* light shafts in the start room */
    if (run.room_x == 0 && run.room_y == 0)
        for (int y = 0; y < 150; y++) gfx_dither(56 + y / 3, y, 40, 1, C_NAVY, 3);
    /* soft glows behind glowcaps, crystals and torches */
    for (int ty = 0; ty < UD_ROOM_H; ty++)
        for (int tx = 0; tx < UD_ROOM_W; tx++) {
            char c = tiles[ty][tx];
            int x = tx * 16, y = ty * 16;
            if (c == 'M' || c == 'Y') {
                bool hang = !solid_at(tx, ty + 1) && solid_at(tx, ty - 1);
                int glow = c == 'M' ? C_TEAL : (z == 3 ? C_MAROON : C_NAVY);
                gfx_dither_circle(x + 8, hang ? y + 4 : y + 12, 11 + ((frame_t / 20 + tx) % 2), glow, 6);
            } else if (c == 'T') {
                gfx_dither_circle(x + 7, y + 6, 14 + ((frame_t / 6 + tx) % 2), C_MAROON, 5);
                gfx_dither_circle(x + 7, y + 6, 8, C_WINE, 4);
            } else if (c == 's') {
                gfx_dither_circle(x + 8, y + 8, 40, C_BROWN, 4);
            }
        }
    for (int ty = 0; ty < UD_ROOM_H; ty++)
        for (int tx = 0; tx < UD_ROOM_W; tx++) {
            char c = tiles[ty][tx];
            int x = tx * 16, y = ty * 16;
            switch (c) {
            case '#': {
                uint32_t h = (uint32_t)(tx * 7 + ty * 13 + run.room_x * 3 + run.room_y * 5);
                spr_draw_ex(&ud_spr[(h % 3) ? S_T_ROCK : S_T_ROCK2], x, y, (h & 4) ? SPR_FLIPX : 0, zmap, -1);
                draw_tile_edges(x, y, tx, ty);
                break;
            }
            case 'F': {
                spr_draw_ex(&ud_spr[S_T_ROCK], x, y, 0, zmap, -1);
                /* a faint crack gives the fake wall away */
                gfx_pset(x + 6, y + 5, p->n); gfx_pset(x + 7, y + 6, p->n); gfx_pset(x + 7, y + 7, p->n);
                gfx_pset(x + 8, y + 8, p->n);
                break;
            }
            case 'I':
                if (taken_get(run.room_x, run.room_y, tx, ty)) {
                    spr_draw_ex(&ud_spr[S_T_BRICK], x, y, 0, zmap, -1);
                }
                break;
            case 'B': spr_draw_ex(&ud_spr[S_T_BRICK], x, y, 0, zmap, -1); break;
            case 'D': spr_draw_ex(&ud_spr[S_T_DRIP], x, y, 0, zmap, -1); break;
            case '=': spr_draw(&ud_spr[S_T_PLANK], x, y, 0); break;
            case 'H': spr_draw(&ud_spr[S_T_LADDER], x, y, 0); break;
            case 'R': spr_draw(&ud_spr[S_T_ROPE], x, y, 0); break;
            case '^': spr_draw_ex(&ud_spr[S_T_SPIKES], x, y, 0, zmap, -1); break;
            case 'X': spr_draw(&ud_spr[S_T_CRYSTAL], x, y, (tx + ty) & 1 ? SPR_FLIPX : 0);
                if ((frame_t / 6 + tx * 3 + ty) % 50 == 0) gfx_pset(x + 4, y + 3, C_WHITE);
                break;
            case '~': {
                bool surface = tile(tx, ty - 1) != '~';
                int fr = ((frame_t / 20) + tx) & 1;
                spr_draw_ex(&ud_spr[surface ? (fr ? S_T_OOZE1 : S_T_OOZE2) : S_T_OOZEBODY], x, y, 0, omap, -1);
                break;
            }
            case 'G':
                if (!run.gate_open) spr_draw(&ud_spr[S_T_GATE], x, y, 0);
                else spr_draw_ex(&ud_spr[S_T_GATE], x, y - 14, 0, NULL, C_NIGHT);
                break;
            case 'Z':
                if (!run.boss_dead) spr_draw(&ud_spr[S_T_SEAL], x, y, 0);
                break;
            case 'o': spr_draw(&ud_spr[S_T_BEAM], x, y, 0); break;
            case 'T': spr_draw(&ud_spr[(frame_t / 8 + tx) % 2 ? S_T_TORCH1 : S_T_TORCH2], x, y, 0); break;
            case 'M': case 'Y': {
                /* decor grows from the floor, or hangs from the ceiling */
                int fl = tx & 1 ? SPR_FLIPX : 0;
                if (!solid_at(tx, ty + 1) && solid_at(tx, ty - 1)) fl |= SPR_FLIPY;
                if (c == 'M') spr_draw(&ud_spr[S_T_MUSH], x, y, fl);
                else spr_draw_ex(&ud_spr[S_T_CRYS_DECOR], x, y, fl, z == 3 ? omap : NULL, -1);
                break;
            }
            case '$': spr_draw(&ud_spr[S_NUGGET], x + 3, y + 5 + ((frame_t / 16 + tx) % 2), 0); break;
            case '*': spr_draw(&ud_spr[(frame_t / 10 + tx) % 6 == 0 ? S_GEM2 : S_GEM1], x + 3, y + 5 + ((frame_t / 16 + tx) % 2), 0); break;
            case 'c': spr_draw(&ud_spr[taken_get(run.room_x, run.room_y, tx, ty) ? S_CHEST_OPEN : S_CHEST], x, y, 0); break;
            case 'n': spr_draw(&ud_spr[taken_get(run.room_x, run.room_y, tx, ty) ? S_CAGE_OPEN : S_CAGE], x, y, 0); break;
            case 'a': spr_draw(&ud_spr[taken_get(run.room_x, run.room_y, tx, ty) ? S_ALTAR_EMPTY : S_ALTAR], x, y, 0); break;
            case 's': {
                int bob = (int)(sinf(frame_t * 0.08f) * 2);
                for (int r = 0; r < 3; r++) gfx_circb(x + 8, y + 8 + bob, 10 + r * 5 + (frame_t / 4) % 5, r == 0 ? C_YELLOW : C_AMBER);
                spr_draw(&ud_spr[S_SUNSTONE], x, y + bob, 0);
                break;
            }
            case '1': {
                spr_draw(&ud_spr[S_PEDESTAL], x, y + 8, 0);
                const RoomDef *rd = R();
                int it = rd->shop_item;
                bool owned = (it == UD_ITEM_ROD) ? run.weapon == W_ROD : (run.items & it) != 0;
                if (!owned) {
                    int icon = it == UD_ITEM_POT ? S_I_POT : it == UD_ITEM_CRANK ? S_I_CRANK : S_I_ROD;
                    spr_draw_outline(&ud_spr[icon], x + 2, y - 6 + (int)(sinf(frame_t * 0.1f) * 1.5f), 0, C_INK);
                    char buf[16];
                    snprintf(buf, sizeof buf, "%d", rd->shop_price);
                    gfx_rect(x + 8 - tiny_width(buf) / 2 - 2, y - 15, tiny_width(buf) + 4, 7, C_INK);
                    tiny_draw(buf, x + 8 - tiny_width(buf) / 2, y - 14, C_YELLOW);
                } else {
                    tiny_draw("SOLD", x + 1, y, C_SLATE);
                }
                break;
            }
            case 'S': {
                int k = R()->keeper;
                int base = k == 0 ? S_KEEPER_TOAD1 : k == 1 ? S_KEEPER_OWL1 : S_KEEPER_LIZ1;
                spr_draw(&ud_spr[base + ((frame_t / 40) % 4 == 0)], x, y, pl.x < x ? SPR_FLIPX : 0);
                break;
            }
            case 'V':
                spr_draw(&ud_spr[(frame_t / 30) % 2 ? S_VENDOR1 : S_VENDOR2], x, y, 0);
                gfx_rect(x - 2, y - 10, 21, 7, C_INK);
                tiny_draw("100", x + 2, y - 9, C_YELLOW);
                spr_draw(&ud_spr[S_LANTERN_HUD], x + 20, y - 12, 0);
                break;
            }
        }
    /* lifts */
    bool powered = (run.items & UD_ITEM_CRANK) != 0;
    for (int i = 0; i < n_lifts; i++) {
        Lift *l = &lifts[i];
        for (int k = 0; k < l->w_px / 16; k++) spr_draw(&ud_spr[S_LIFT], (int)l->x + k * 16, (int)l->y, 0);
        /* cable */
        if (l->axis == 1) gfx_vline((int)l->x + l->w_px / 2, 0, (int)l->y - 1, C_SLATE);
        if (!powered && (frame_t / 30) % 2) tiny_draw("OFF", (int)l->x + l->w_px / 2 - 5, (int)l->y + 8, C_RED);
    }
}

static void draw_ent(Ent *e) {
    int x = (int)e->x, y = (int)e->y;
    int fl = e->dir > 0 ? SPR_FLIPX : 0;
    int solid = e->flash > 0 && (e->flash / 2) % 2 ? C_WHITE : -1;
    int f2 = (e->t2 / 8) % 2;
    switch (e->type) {
    case E_MOTH: spr_draw_ex(&ud_spr[f2 ? S_MOTH1 : S_MOTH2], x - 2, y - 2, 0, NULL, solid); break;
    case E_TOAD: spr_draw_ex(&ud_spr[e->vy < 0 || !ent_ground(e) ? S_TOAD2 : S_TOAD1], x - 2, y - 7, fl, NULL, solid); break;
    case E_GRUB: spr_draw_ex(&ud_spr[(e->t2 / 12) % 2 ? S_GRUB1 : S_GRUB2], x - 1, y - 2, fl, NULL, solid); break;
    case E_SPITTER: spr_draw_ex(&ud_spr[e->state > 0 ? S_SPIT2 : S_SPIT1], x - 1, y - 5, fl, NULL, solid); break;
    case E_SWOOPER:
        if (e->state == 0) spr_draw_ex(&ud_spr[S_SWOOP_HANG], x - 3, y - 1, 0, NULL, solid);
        else spr_draw_ex(&ud_spr[(e->t2 / 5) % 2 ? S_SWOOP1 : S_SWOOP2], x - 3, y - 1, 0, NULL, solid);
        break;
    case E_SACK: spr_draw_ex(&ud_spr[e->state == 0 ? S_SACK : S_SACK_HOP], x - 2, y - 4, fl, NULL, solid); break;
    case E_BEETLE: spr_draw_ex(&ud_spr[f2 ? S_BEETLE1 : S_BEETLE2], x - 1, y - 4, fl, NULL, solid); break;
    case E_WISP: spr_draw_ex(&ud_spr[f2 ? S_WISP1 : S_WISP2], x - 1, y - 1, 0, NULL, solid); break;
    case E_GLOOM: {
        /* fades in, flickers */
        if (e->t2 < 60 && (e->t2 / 3) % 2) break;
        spr_draw(&ud_spr[(e->t2 / 16) % 2 ? S_GLOOM1 : S_GLOOM2], x - 4, y - 4 + (int)(sinf(e->t2 * 0.07f) * 2), 0);
        break;
    }
    case E_BOSS: {
        int spr = e->state == 3 ? S_BOSS_OPEN : S_BOSS_SHUT;
        int bx = x - 4, by = y - 4;
        if (e->state == 5 && (e->t / 3) % 2) bx += 2;
        spr_draw_ex(&ud_spr[spr], bx, by, 0, NULL, solid);
        /* hp pips */
        for (int i = 0; i < BOSS_HP; i++) gfx_rect(100 + i * 9, 150, 7, 4, i < e->hp ? C_MAGENTA : C_NIGHT);
        break;
    }
    case E_STONE: spr_draw(&ud_spr[S_STONE], x, y, 0); break;
    case E_SHARD: spr_draw(&ud_spr[S_SHARD], x, y, 0); break;
    case E_DRIP: spr_draw(&ud_spr[S_DRIP_FALL], x + 2, y, 0); break;
    case E_SPARK: spr_draw(&ud_spr[S_SPARK], x, y, (e->t2 / 3) % 2 ? SPR_FLIPX : 0); break;
    case E_BOLT: spr_draw(&ud_spr[S_BOLT], x, y, e->vx < 0 ? SPR_FLIPX : 0); break;
    case E_COIN: if (e->t > 120 || (e->t / 4) % 2) spr_draw(&ud_spr[S_COIN], x, y, 0); break;
    case E_PUFF: spr_draw(&ud_spr[e->t < 6 ? S_PUFF1 : e->t < 12 ? S_PUFF2 : S_PUFF3], x, y, 0); break;
    case E_DEBRIS: gfx_rect(x, y, 2, 2, e->col); break;
    case E_CANARY: spr_draw(&ud_spr[(e->t2 / 4) % 2 ? S_CANARY1 : S_CANARY2], x - 1, y - 1, e->dir < 0 ? SPR_FLIPX : 0); break;
    }
}

static void draw_player(void) {
    if (state == ST_DYING) {
        if (state_t < 30) spr_draw_ex(&ud_spr[S_MO_HURT], (int)pl.x - 3, (int)pl.y - 2, pl.face < 0 ? SPR_FLIPX : 0, NULL,
                                      (state_t / 3) % 2 ? C_WHITE : -1);
        else if (state_t < 48) spr_draw(&ud_spr[state_t < 36 ? S_PUFF1 : state_t < 42 ? S_PUFF2 : S_PUFF3], (int)pl.x - 1, (int)pl.y + 1, 0);
        return;
    }
    int fl = pl.face < 0 ? SPR_FLIPX : 0;
    int x = (int)pl.x - 3, y = (int)pl.y - 2;
    int spr = S_MO_IDLE;
    if (pl.climbing) {
        spr = S_MO_CLIMB1;
        fl = ((int)pl.y / 6) % 2 ? SPR_FLIPX : 0;
    } else if (pl.attack > 0 && run.weapon != W_ROD) {
        spr = pl.attack > 12 ? S_MO_SWING1 : S_MO_SWING2;
    } else if (pl.attack > 0) {
        spr = S_MO_SWING2;
    } else if (!pl.ground) {
        spr = pl.vy < 0 ? S_MO_JUMP : S_MO_FALL;
    } else if (pl.vx != 0) {
        spr = (pl.anim / 7) % 2 ? S_MO_WALK1 : S_MO_WALK2;
    } else {
        spr = (frame_t % 180) < 6 ? S_MO_BLINK : S_MO_IDLE;
    }
    /* weapon */
    if (pl.attack > 0 && !pl.climbing) {
        if (run.weapon == W_ROD) {
            int wx = pl.face > 0 ? (int)pl.x + 7 : (int)pl.x - 13;
            spr_draw(&ud_spr[S_ROD_FWD], wx, (int)pl.y + 5, fl);
        } else if (pl.attack > 12) {
            int wx = pl.face > 0 ? (int)pl.x - 10 : (int)pl.x + 4;
            spr_draw(&ud_spr[S_PICK_UP], wx, (int)pl.y - 7, fl);
        } else {
            int wx = pl.face > 0 ? (int)pl.x + 5 : (int)pl.x - 11;
            spr_draw(&ud_spr[run.weapon == W_HUNGRY ? S_HUNGRY_FWD : S_PICK_FWD], wx, (int)pl.y + 2, fl);
        }
    }
    spr_draw(&ud_spr[spr], x, y, fl);
}

static void draw_hud(void) {
    gfx_rect(0, 0, 320, UD_HUD_H, C_INK);
    gfx_hline(0, 319, UD_HUD_H - 1, C_DUSK);
    /* minimap */
    for (int ry = 0; ry < UD_MAP_H; ry++)
        for (int rx = 0; rx < UD_MAP_W; rx++) {
            int x = 4 + rx * 6, y = 2 + ry * 3;
            int col = C_NIGHT;
            if (run.visited[ry][rx]) {
                static const uint8_t zc[4] = {C_TAN, C_JADE, C_VIOLET, C_RED};
                col = zc[zone_of(ry)];
            }
            if (rx == run.room_x && ry == run.room_y) col = (frame_t / 15) % 2 ? C_WHITE : C_YELLOW;
            gfx_rect(x, y, 5, 2, col);
        }
    /* items */
    static const int order[8] = {UD_ITEM_POT, UD_ITEM_FORK, UD_ITEM_CRANK, UD_ITEM_GLOVES, UD_ITEM_KEY, UD_ITEM_CANARY, UD_ITEM_ROD, UD_ITEM_HUNGRY};
    static const int icons[8] = {S_I_POT, S_I_FORK, S_I_CRANK, S_I_GLOVES, S_I_KEY, S_I_CANARY, S_I_ROD, S_I_HUNGRY};
    for (int i = 0; i < 8; i++) {
        int x = 44 + i * 14, y = 3;
        ui_panel(x - 1, y - 1, 13, 14, C_NIGHT, C_NIGHT);
        if (run.items & order[i]) spr_draw(&ud_spr[icons[i]], x, y, 0);
        else gfx_rect(x + 5, y + 5, 2, 2, C_DUSK);
    }
    /* money */
    char buf[32];
    spr_draw(&ud_spr[S_NUGGET], 162, 2, 0);
    snprintf(buf, sizeof buf, "%d", run.money);
    text_draw(buf, 175, 3, C_YELLOW);
    tiny_draw(R()->name, 162, 13, C_SLATE);
    /* lanterns */
    for (int i = 0; i < MAX_LANTERNS; i++)
        spr_draw(&ud_spr[i < run.lanterns ? S_LANTERN_HUD : S_LANTERN_HUD_OFF], 268 + i * 8, 5, 0);
}

static void draw_message(void) {
    ui_panel(20, 128, 280, 40, C_NIGHT, C_YELLOW);
    text_wrap(msg, 30, 134, 262, C_WHITE, 9);
    if (msg_t > 20 && (msg_t / 15) % 2) text_draw(GLYPH_A, 286, 157, C_YELLOW);
}

static void stalactite(int x, int len, int w, int col, int hi) {
    for (int i = 0; i < len; i++) {
        int hw = w - (i * w) / len;
        gfx_hline(x - hw, x + hw, i, col);
        gfx_pset(x - hw, i, hi);
    }
}

static void draw_title(void) {
    int t = state_t;
    gfx_cls(C_INK);
    for (int y = 0; y < 180; y++) gfx_dither(0, y, 320, 1, C_NIGHT, iclamp((y - 20) / 7, 0, 16));
    /* far and near stalactites */
    for (int i = 0; i < 14; i++) stalactite(i * 25 + 8, 18 + (i * 7) % 20, 9, C_DUSK, C_SLATE);
    for (int i = 0; i < 9; i++) stalactite(i * 38 + 20, 26 + (i * 11) % 22, 13, C_NIGHT, C_DUSK);
    /* ground */
    build_zone_maps(0);
    for (int i = 0; i < 20; i++) {
        spr_draw_ex(&ud_spr[i % 3 ? S_T_ROCK : S_T_ROCK2], i * 16, 150, 0, zmap, -1);
        spr_draw_ex(&ud_spr[S_T_ROCK], i * 16, 166, 0, zmap, -1);
        gfx_hline(i * 16, i * 16 + 15, 149, C_INK);
        gfx_hline(i * 16, i * 16 + 15, 150, C_EARTH);
    }
    /* Mo and the beam of the helmet lamp */
    int mx = 70, my = 118;
    for (int k = 0; k < 150; k++) {
        int half = 3 + k / 5;
        int lvl = 7 - k / 25;
        gfx_dither(mx + 30 + k, my - 2 - half + 8, 1, half * 2, C_AMBER, lvl);
    }
    gfx_dither_circle(mx + 29, my + 7, 5, C_CREAM, 12);
    spr_draw_scaled(&ud_spr[(t % 120) < 6 ? S_MO_BLINK : S_MO_IDLE], mx, my - 2, 2, 0);
    /* sparkling ore caught by the beam */
    spr_draw(&ud_spr[(t / 10) % 6 == 0 ? S_GEM2 : S_GEM1], 214, 140, 0);
    spr_draw(&ud_spr[S_NUGGET], 186, 141, 0);
    spr_draw(&ud_spr[(t / 8) % 2 ? S_MOTH1 : S_MOTH2], 236 + (int)(sinf(t * 0.04f) * 14), 98 + (int)(cosf(t * 0.06f) * 8), 0);
    static const uint8_t grad[] = {C_CREAM, C_YELLOW, C_AMBER, C_ORANGE, C_TAN};
    ui_fancy_center("UNDERDELVE", 160, 30, 3, grad, 5, C_INK, C_BROWN);
    text_center("A MOLE. A LAMP. A MINE GONE DARK.", 160, 58, C_EARTH);
    const char *items[2];
    int n = 0;
    if (has_save) items[n++] = "CONTINUE";
    items[n++] = "NEW DELVE";
    for (int i = 0; i < n; i++) {
        int y = 78 + i * 12;
        bool sel = i == title_sel;
        text_center(items[i], 160, y, sel ? C_WHITE : C_GREY);
        if (sel) ui_cursor(160 - text_width(items[i]) / 2 - 10, y, t);
    }
    tiny_center("A JUMP   B PICK   UP CLIMB OR BUY   B HERE: LIBRARY", 160, 172, C_EARTH);
}

static void draw_gameover(void) {
    gfx_cls(C_INK);
    static const uint8_t grad[] = {C_LIGHT, C_GREY, C_SLATE};
    ui_fancy_center("LANTERNS OUT", 160, 40, 2, grad, 3, C_INK, C_NIGHT);
    spr_draw(&ud_spr[S_LANTERN_HUD_OFF], 156, 70, 0);
    char buf[96];
    snprintf(buf, sizeof buf, "THE DARK TOOK MO %d TIMES.\nORE FOUND: %d   FOES: %d", run.deaths, run.money, run.kills);
    text_center(buf, 160, 92, C_GREY);
    text_center("THE DELVE BEGINS AGAIN FROM THE MINE MOUTH.", 160, 120, C_SLATE);
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 146, C_YELLOW);
}

static void draw_ending(void) {
    gfx_cls(C_NIGHT);
    /* sunrise */
    int t = state_t;
    for (int y = 0; y < 180; y++) {
        int lvl = iclamp(16 - (y - 40 + t / 4) / 8, 0, 16);
        gfx_dither(0, y, 320, 1, y < 90 ? C_AMBER : C_ORANGE, lvl / 2);
    }
    gfx_circ(160, 150 - imin(t / 3, 60), 30, C_YELLOW);
    gfx_circ(160, 150 - imin(t / 3, 60), 24, C_CREAM);
    for (int i = 0; i < 20; i++) spr_draw_ex(&ud_spr[S_T_ROCK], i * 16, 150, 0, NULL, -1);
    spr_draw_scaled(&ud_spr[S_MO_IDLE], 140, 118, 2, 0);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_center("THE DELVE IS LIT", 160, 18, 2, grad, 4, C_INK, C_WINE);
    if (t > 60) {
        char buf[160];
        int secs = (int)(run.frames / 60);
        snprintf(buf, sizeof buf, "MO CARRIED THE SUNSTONE HOME.\nTIME %d:%02d   DEATHS %d   ORE %d", secs / 60, secs % 60,
                 run.deaths, run.money);
        text_center(buf, 160, 46, C_INK);
    }
    if (t > 120) {
        int gx = 136;
        ui_goal_icon(gx, 76, GOAL_BEACON, (g_progress.goals[game_current_index()] & GOAL_BEACON) != 0, t);
        ui_goal_icon(gx + 20, 76, GOAL_SAUCER, true, t);
        ui_goal_icon(gx + 40, 76, GOAL_ALIEN, run.deaths == 0, t);
        if (run.deaths == 0) text_center("A FLAWLESS DELVE!", 160, 90, C_WINE);
    }
    if (t > 240 && (t / 20) % 2) text_center("THANKS FOR PLAYING  - PRESS " GLYPH_A, 160, 102, C_INK);
}

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    int x = 2, y = 2, rowh = 0;
    for (int i = 0; i < S_COUNT; i++) {
        Sprite *s = &ud_spr[i];
        if (x + s->w + 2 > 320) { x = 2; y += rowh + 2; rowh = 0; }
        spr_draw(s, x, y, 0);
        x += s->w + 2;
        if (s->h > rowh) rowh = s->h;
    }
}

static void ud_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case ST_TITLE: draw_title(); return;
    case ST_GAMEOVER: draw_gameover(); return;
    case ST_ENDING: draw_ending(); return;
    default: break;
    }
    int sx = 0, sy = 0;
    if (shake > 0) { sx = (frame_t % 3) - 1; sy = ((frame_t / 2) % 3) - 1; if (shake > 8) { sx *= 2; sy *= 2; } }
    gfx_camera(sx, -UD_HUD_H + sy);
    gfx_clip(0, UD_HUD_H, 320, 160);
    draw_room();
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < MAX_ENTS; i++) {
            Ent *e = &ents[i];
            if (!e->alive) continue;
            bool top = e->type == E_GLOOM || e->type >= E_STONE;
            if ((pass == 0) == !top) draw_ent(e);
        }
    if (!view_mode) draw_player();
    gfx_camera(0, 0);
    gfx_noclip();
    draw_hud();
    if (state == ST_MESSAGE && !view_mode) draw_message();
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void ud_load(void) {
    ud_art_load();
    ud_audio_load();
}

static void ud_start(void) {
    rng_seed(&rng, g_rng.state ^ 0xD31FEull);
    Run tmp;
    has_save = load_run(&tmp);
    state = ST_TITLE;
    state_t = 0;
    title_sel = 0;
    sheet_mode = false;
    shake = 0;
    game_set_pausable(false);
    music_play(UD_MUS_TITLE);
}

static void ud_quit(void) {
    if (state == ST_PLAY || state == ST_MESSAGE || state == ST_DYING) {
        if (state == ST_DYING) {
            /* resolve the death first so quitting cannot dodge it */
            if (run.lanterns == 0) { game_save_erase(game_current_index()); return; }
            run.room_x = run.room_x;
        }
        save_run();
    }
}

static void ud_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_INK);
    build_zone_maps(0);
    for (int i = 0; i < w / 16 + 1; i++) {
        spr_draw_ex(&ud_spr[S_T_ROCK], x + i * 16, y + h - 12, 0, zmap, -1);
        gfx_hline(x + i * 16, x + i * 16 + 15, y + h - 12, C_EARTH);
        spr_draw_ex(&ud_spr[S_T_ROCK], x + i * 16, y - 8 + (i % 3) * 2, 0, zmap, -1);
    }
    gfx_dither_circle(x + 34, y + 36, 30, C_BROWN, 4);
    gfx_dither_circle(x + 34, y + 36, 18, C_TAN, 3);
    spr_draw(&ud_spr[(t / 8) % 2 ? S_T_TORCH1 : S_T_TORCH2], x + w - 26, y + h - 28, 0);
    spr_draw_scaled(&ud_spr[(t % 150) < 6 ? S_MO_BLINK : S_MO_IDLE], x + 18, y + h - 44, 2, 0);
    spr_draw(&ud_spr[S_GEM1], x + 70, y + h - 22, 0);
    spr_draw(&ud_spr[S_NUGGET], x + 86, y + h - 21, 0);
    spr_draw(&ud_spr[(t / 8) % 2 ? S_MOTH1 : S_MOTH2], x + 96 + (int)(sinf(t * 0.05f) * 10), y + 14 + (int)(cosf(t * 0.07f) * 6), 0);
}

static int ud_query(const char *key, int *out) {
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "room_x")) { *out = run.room_x; return 1; }
    if (!strcmp(key, "room_y")) { *out = run.room_y; return 1; }
    if (!strcmp(key, "px")) { *out = (int)pl.x; return 1; }
    if (!strcmp(key, "py")) { *out = (int)pl.y; return 1; }
    if (!strcmp(key, "money")) { *out = run.money; return 1; }
    if (!strcmp(key, "lanterns")) { *out = run.lanterns; return 1; }
    if (!strcmp(key, "items")) { *out = run.items; return 1; }
    if (!strcmp(key, "weapon")) { *out = run.weapon; return 1; }
    if (!strcmp(key, "grounded")) { *out = pl.ground; return 1; }
    if (!strcmp(key, "climbing")) { *out = pl.climbing; return 1; }
    if (!strcmp(key, "deaths")) { *out = run.deaths; return 1; }
    if (!strcmp(key, "kills")) { *out = run.kills; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = run.boss_dead; return 1; }
    if (!strcmp(key, "gate_open")) { *out = run.gate_open; return 1; }
    if (!strcmp(key, "has_save")) { Run tmp; *out = load_run(&tmp); return 1; }
    if (!strcmp(key, "boss_hp")) { *out = boss_i >= 0 && ents[boss_i].alive ? ents[boss_i].hp : 0; return 1; }
    if (!strcmp(key, "enemies")) {
        int n = 0;
        for (int i = 0; i < MAX_ENTS; i++) n += ents[i].alive && ents[i].type >= E_MOTH && ents[i].type <= E_BOSS;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "visited")) {
        int n = 0;
        for (int y = 0; y < UD_MAP_H; y++)
            for (int x = 0; x < UD_MAP_W; x++) n += run.visited[y][x];
        *out = n;
        return 1;
    }
    if (!strcmp(key, "map_errors")) {
        /* every room edge must match its neighbour's edge */
        int errs = 0;
        for (int ry = 0; ry < UD_MAP_H; ry++)
            for (int rx = 0; rx < UD_MAP_W; rx++) {
                const RoomDef *a = &UD_ROOMS[ry][rx];
                for (int y = 0; y < UD_ROOM_H; y++) {
                    if ((int)strlen(a->rows[y]) != UD_ROOM_W) errs++;
                    char l = a->rows[y][0], r = a->rows[y][UD_ROOM_W - 1];
                    bool lopen = l != '#' && l != 'B', ropen = r != '#' && r != 'B';
                    if (rx == 0 && lopen) errs++;
                    if (rx == UD_MAP_W - 1 && ropen) errs++;
                    if (rx < UD_MAP_W - 1) {
                        char o = UD_ROOMS[ry][rx + 1].rows[y][0];
                        bool oopen = o != '#' && o != 'B';
                        if (ropen != oopen) errs++;
                    }
                }
                for (int x = 0; x < UD_ROOM_W; x++) {
                    char tp = a->rows[0][x], bt = a->rows[UD_ROOM_H - 1][x];
                    bool topen = tp != '#' && tp != 'B', bopen = bt != '#' && bt != 'B' && bt != '^' && bt != '~';
                    if (ry == 0 && topen) errs++;
                    if (ry == UD_MAP_H - 1 && bopen) errs++;
                    if (ry < UD_MAP_H - 1) {
                        char o = UD_ROOMS[ry + 1][rx].rows[0][x];
                        bool oopen = o != '#' && o != 'B';
                        if (bopen != oopen) errs++;
                    }
                }
            }
        *out = errs;
        return 1;
    }
    return 0;
}

static int ud_cheat(const char *cmd) {
    int a = 0, b = 0, c = 0, d = 0;
    if (!strncmp(cmd, "room", 4)) {
        int n = sscanf(cmd + 4, "%d %d %d %d", &a, &b, &c, &d);
        if (n < 2) return 0;
        if (state == ST_TITLE) new_game();
        view_mode = false;
        run.room_x = (uint8_t)a;
        run.room_y = (uint8_t)b;
        if (n >= 4) { pl.x = (float)c; pl.y = (float)d; }
        pl.climbing = false;
        pl.vx = pl.vy = 0;
        pl.ground = false;
        enter_room(true);
        state = ST_PLAY;
        return 1;
    }
    if (!strncmp(cmd, "money", 5)) { run.money = (uint16_t)atoi(cmd + 5); return 1; }
    if (!strncmp(cmd, "lanterns", 8)) { run.lanterns = (uint8_t)atoi(cmd + 8); return 1; }
    if (!strncmp(cmd, "give", 4)) { run.items |= (uint16_t)atoi(cmd + 4); return 1; }
    if (!strcmp(cmd, "newgame")) { new_game(); return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    if (!strncmp(cmd, "view", 4)) {
        /* show a room without Mo (map previews); the game is frozen until "room" */
        if (sscanf(cmd + 4, "%d %d", &a, &b) != 2) return 0;
        if (state == ST_TITLE) new_game();
        run.room_x = (uint8_t)a;
        run.room_y = (uint8_t)b;
        pl.x = -500;
        pl.y = 60;
        enter_room(false);
        gloom_timer = -1;
        state = ST_MESSAGE;
        msg_t = -100000;
        snprintf(msg, sizeof msg, "%s", "");
        view_mode = true;
        return 1;
    }
    if (!strncmp(cmd, "boss_hp", 7)) {
        if (boss_i >= 0) ents[boss_i].hp = atoi(cmd + 7);
        return 1;
    }
    if (!strcmp(cmd, "boss_ground")) {
        /* put the boss on the floor, eye open, without the landing sparks */
        if (boss_i >= 0) { ents[boss_i].state = 3; ents[boss_i].x = 120; ents[boss_i].y = 144 - 30; ents[boss_i].t = 200; }
        return 1;
    }
    if (!strcmp(cmd, "clear_enemies")) {
        for (int i = 0; i < MAX_ENTS; i++)
            if (ents[i].type >= E_MOTH && ents[i].type < E_BOSS) ents[i].alive = false;
        gloom_timer = -1;
        return 1;
    }
    if (!strcmp(cmd, "no_gloom")) { gloom_timer = -1; for (int i = 0; i < MAX_ENTS; i++) if (ents[i].type == E_GLOOM) ents[i].alive = false; return 1; }
    if (!strcmp(cmd, "gloom")) { gloom_timer = 1; return 1; }
    if (!strcmp(cmd, "dismiss")) { if (state == ST_MESSAGE) state = ST_PLAY; return 1; }
    return 0;
}

const GameDef GAME_UNDERDELVE = {
    "underdelve",
    "UNDERDELVE",
    "1983",
    "EXPLORATION",
    "HELP MO THE MOLE RELIGHT A DARK MINE. ONE TOUCH IS DEADLY.",
    {"FREE THE HIDDEN CANARY", "DEFEAT THE OLD LODE", "WIN WITHOUT DYING"},
    "D-PAD\tWALK / CLIMB\n"
    "UP\tBUY AT SHOPS\n"
    GLYPH_A "\tJUMP (NO AIR CONTROL)\n"
    GLYPH_B "\tSWING PICK / FIRE ROD\n"
    "START\tPAUSE\n\n"
    "ONE HIT LOSES A LANTERN.\n"
    "LEAVE A ROOM TO ESCAPE THE GLOOM.",
    C_TAN, C_YELLOW,
    ud_load, ud_start, ud_update, ud_draw, ud_quit, ud_label, ud_query, ud_cheat,
};
