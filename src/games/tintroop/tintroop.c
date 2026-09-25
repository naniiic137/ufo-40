/* TIN TROOP - a troop of tin soldiers spends itself to cross a toymaker's house.
 * Cartridge 06 of UFO 40, a tribute to Mortol (UFO 50 #6).
 * See docs/games/06-tin-troop.md. */
#include "tintroop.h"

#define TS TT_TS
#define PLAY_Y 20        /* screen y of level row 0 */
#define GRAV 0.22f
#define JUMP_V 3.4f
#define RUN 1.1f
#define SW 6             /* soldier hitbox */
#define SH 9
#define ARROW_V 4.0f
#define CHUTE_VY 0.8f
#define CHUTE_VX 0.8f
#define BREATH 240
#define BURN 180
#define SEEDED 150
#define GIVEUP 90
#define RESPAWN 40
#define LEVEL_H (TT_ROWS * TS)

enum { S_TITLE, S_MAP, S_INTRO, S_PLAY, S_CLEAR, S_FAIL, S_ENDING };
enum { M_GONE, M_CHUTE, M_ENTER, M_WALK, M_SWIM, M_CLIMB, M_ARROW };
enum { B_LODGED, B_STONE, B_CORPSE, B_FLOAT };
enum { F_MOUSE, F_DART, F_RAM, F_FISH, F_BUG, F_DRAGON };
enum { SH_SEED, SH_FIRE, SH_ORB };
enum { DIE_CORPSE, DIE_NONE, DIE_FLOAT, DIE_VINE };

typedef struct { float x, y, vy; uint8_t kind, alive, smash, pad; } Body;
typedef struct { float x, y, vx, vy; int8_t dir; uint8_t kind, alive, state; int t, seeded; } Foe;
typedef struct { float x, y, vx, vy; uint8_t kind, alive; int t, life; } Shot;
typedef struct { float lx, rx, base_l, base_r, off; } Scale;
typedef struct { int x, y, left, t; } Vine;
typedef struct { float x, y, vx, vy; int life, col, kind, val; } Part;

#define MAX_BODIES 96
#define MAX_FOES 48
#define MAX_SHOTS 40
#define MAX_SCALES 4
#define MAX_VINES 8

static char map[TT_ROWS][TT_MAXW + 1];
static int LW, level_i, world;
static Body bodies[MAX_BODIES];
static Foe foes[MAX_FOES];
static Shot shots[MAX_SHOTS];
static Scale scales[MAX_SCALES];
static int n_scales;
static Vine vines[MAX_VINES];
static Part parts[160];

static struct {
    float x, y, vx, vy;
    int mode, dir;
    bool ground;
    int breath, burn, seed, giveup, anim, enter_t;
    int on_pan; /* scale index * 2 + side, or -1 */
} P;

static int lives, lives_start, kills, respawn_t, last_stone = -1;
static float ship_x, progress_x, cam_x;
static int door_x, door_y, start_x;
static bool gate_open[2];
static int state, state_t, frame_t, shake, map_sel, title_sel, fail_reason;
static bool sheet_mode, freeze_foes;
static int overview_page = -1;

static struct {
    bool on, dead;
    int hp, bx, by, t, shot_t, flash, death_t;
    bool mouth;
} boss;

typedef struct {
    uint32_t magic;
    uint16_t cleared;
    uint8_t beaten, seen_end;
    int16_t best[TT_LEVELS];
} Save;
#define SAVE_MAGIC 0x54540001u
static Save sv;

/* ------------------------------------------------------------------ */
/* save and the troop's record                                          */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else { memset(&sv, 0, sizeof sv); sv.magic = SAVE_MAGIC; }
}

static bool cleared(int i) { return (sv.cleared >> i) & 1; }
static bool unlocked(int i) { return i == 0 || cleared(i - 1); }

/* a level starts with 20 plus the best change of every level before it */
static int start_lives(int i) {
    int n = TT_START_LIVES;
    for (int j = 0; j < i; j++) n += sv.best[j];
    return n;
}

/* ------------------------------------------------------------------ */
/* tiles                                                                */

static char tile(int tx, int ty) {
    if (tx < 0 || tx >= LW) return '#';
    if (ty < 0 || ty >= TT_ROWS) return ' ';
    return map[ty][tx];
}

static bool solid_char(char c) {
    switch (c) {
    case '#': case 'x': case 'P': case 'j': case 'N': case 'K': case 'Y': return true;
    case '|': return !gate_open[0];
    case '!': return !gate_open[1];
    default: return false;
    }
}

static bool water_at(float x, float y) {
    int tx = (int)floorf(x / TS), ty = (int)floorf(y / TS);
    return tile(tx, ty) == 'w';
}

static bool rect_tiles(float x, float y, float w, float h) {
    int x0 = (int)floorf(x / TS), x1 = (int)floorf((x + w - 0.01f) / TS);
    int y0 = (int)floorf(y / TS), y1 = (int)floorf((y + h - 0.01f) / TS);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (solid_char(tile(tx, ty))) return true;
    return false;
}

static bool rect_has(float x, float y, float w, float h, char c) {
    int x0 = (int)floorf(x / TS), x1 = (int)floorf((x + w - 0.01f) / TS);
    int y0 = (int)floorf(y / TS), y1 = (int)floorf((y + h - 0.01f) / TS);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (tile(tx, ty) == c) return true;
    return false;
}

static bool overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

/* spikes only bite in the lower part of their tile, where the points are */
static bool on_spikes(float x, float y, float w, float h) {
    int x0 = (int)floorf(x / TS), x1 = (int)floorf((x + w - 0.01f) / TS);
    int y0 = (int)floorf(y / TS), y1 = (int)floorf((y + h - 0.01f) / TS);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (tile(tx, ty) == '^' && overlap(x, y, w, h, (float)(tx * TS + 1), (float)(ty * TS + 4), TS - 2, TS - 4)) return true;
    return false;
}

static float body_h(int k) { return k == B_STONE ? 10 : 4; }

/* stones are solid all round */
static bool rect_objs(float x, float y, float w, float h, int ignore) {
    for (int i = 0; i < MAX_BODIES; i++) {
        Body *b = &bodies[i];
        if (!b->alive || i == ignore || b->kind != B_STONE) continue;
        if (overlap(x, y, w, h, b->x, b->y, 10, body_h(b->kind))) return true;
    }
    return false;
}

static bool blocked(float x, float y, float w, float h, int ignore) {
    return rect_tiles(x, y, w, h) || rect_objs(x, y, w, h, ignore);
}

static float pan_y(int s, int side) { return side == 0 ? scales[s].base_l + scales[s].off : scales[s].base_r - scales[s].off; }
static float pan_x(int s, int side) { return side == 0 ? scales[s].lx : scales[s].rx; }

/* top-only platforms: lodged and floating soldiers, and scale pans. Returns
 * the top of the first one crossed by a box whose bottom goes from ob to nb. */
static bool land_on_top(float x, float w, float ob, float nb, float *top, int *pan, int ignore) {
    for (int i = 0; i < MAX_BODIES; i++) {
        Body *b = &bodies[i];
        if (!b->alive || (b->kind != B_FLOAT && b->kind != B_LODGED) || i == ignore) continue;
        if (x < b->x + 10 && b->x < x + w && ob <= b->y + 0.01f && nb > b->y) { *top = b->y; if (pan) *pan = -1; return true; }
    }
    for (int s = 0; s < n_scales; s++)
        for (int side = 0; side < 2; side++) {
            float py = pan_y(s, side), px = pan_x(s, side);
            if (x < px + 30 && px < x + w && ob <= py + 0.01f && nb > py) { *top = py; if (pan) *pan = s * 2 + side; return true; }
        }
    return false;
}

/* ------------------------------------------------------------------ */
/* effects                                                              */

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind, int val) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) { parts[i] = (Part){x, y, vx, vy, life, col, kind, val}; return; }
}

static void burst(float x, float y, int col, int n, float sp) {
    for (int i = 0; i < n; i++) {
        float a = (float)i / (float)n * 6.283f + (float)(frame_t % 7);
        part_add(x, y, cosf(a) * sp, sinf(a) * sp - 0.5f, 18 + i % 9, col, 0, 0);
    }
}

static void popup_n(float x, float y, int n) { part_add(x, y, 0, -0.4f, 50, C_YELLOW, 3, n); }

/* ------------------------------------------------------------------ */
/* level setup                                                          */

static int add_body(int kind, float x, float y) {
    for (int i = 0; i < MAX_BODIES; i++)
        if (!bodies[i].alive) {
            bodies[i] = (Body){x, y, 0, (uint8_t)kind, 1, 0, 0};
            return i;
        }
    /* out of room: the oldest corpse goes */
    for (int i = 0; i < MAX_BODIES; i++)
        if (bodies[i].kind == B_CORPSE) { bodies[i] = (Body){x, y, 0, (uint8_t)kind, 1, 0, 0}; return i; }
    return -1;
}

static int add_foe(int kind, float x, float y) {
    for (int i = 0; i < MAX_FOES; i++)
        if (!foes[i].alive) {
            foes[i] = (Foe){x, y, 0, 0, -1, (uint8_t)kind, 1, 0, 0, 0};
            return i;
        }
    return -1;
}

static void foe_size(int k, float *w, float *h) {
    static const float W[6] = {8, 10, 10, 10, 8, 12}, H[6] = {6, 5, 9, 6, 7, 14};
    *w = W[k];
    *h = H[k];
}

static void load_level(int i) {
    level_i = i;
    const TTLevel *L = &TT_LEVEL[i];
    world = L->world;
    LW = (int)strlen(L->rows[0]);
    memset(bodies, 0, sizeof bodies);
    memset(foes, 0, sizeof foes);
    memset(shots, 0, sizeof shots);
    memset(vines, 0, sizeof vines);
    memset(parts, 0, sizeof parts);
    memset(&boss, 0, sizeof boss);
    n_scales = 0;
    int nl = 0, nr = 0;
    start_x = 20;
    door_x = -1;
    for (int y = 0; y < TT_ROWS; y++) {
        memcpy(map[y], L->rows[y], (size_t)LW);
        map[y][LW] = 0;
    }
    for (int y = 0; y < TT_ROWS; y++)
        for (int x = 0; x < LW; x++) {
            char c = map[y][x];
            float fx = (float)(x * TS), fy = (float)(y * TS);
            switch (c) {
            case 'S': start_x = x * TS; map[y][x] = ' '; break;
            case 'D': if (door_x < 0) { door_x = x; door_y = y; } break;
            case 'L': case 'R': break; /* paired below, left to right */
            case 'm': add_foe(F_MOUSE, fx + 1, fy + 4); map[y][x] = ' '; break;
            case 'a': add_foe(F_DART, fx, fy + 2); map[y][x] = ' '; break;
            case 'r': add_foe(F_RAM, fx, fy + 1); map[y][x] = ' '; break;
            case 'q': add_foe(F_FISH, fx, fy + 2); map[y][x] = 'w'; break;
            case 'o': add_foe(F_BUG, fx + 1, fy + 3); map[y][x] = ' '; break;
            case 'd': add_foe(F_DRAGON, fx - 1, fy - 4); map[y][x] = ' '; break;
            case 'J':
                boss.on = true;
                boss.bx = x * TS;
                boss.by = y * TS;
                boss.hp = 3;
                for (int yy = 0; yy < 5; yy++)
                    for (int xx = 0; xx < 4; xx++) map[y + yy][x + xx] = 'j';
                break;
            case 'N': case 'K': case 'Y':
                /* 2x2 heads, marked from their top-left cell */
                if (tile(x - 1, y) != c && tile(x, y - 1) != c) {
                    map[y][x + 1] = c;
                    map[y + 1][x] = c;
                    map[y + 1][x + 1] = c;
                }
                break;
            }
        }
    /* the n-th L pan (counting left to right) is weighed against the n-th R pan */
    for (int x = 0; x < LW; x++)
        for (int y = 0; y < TT_ROWS; y++) {
            if (map[y][x] == 'L' && nl < MAX_SCALES) { scales[nl].lx = (float)(x * TS); scales[nl].base_l = (float)(y * TS); nl++; }
            if (map[y][x] == 'R' && nr < MAX_SCALES) { scales[nr].rx = (float)(x * TS); scales[nr].base_r = (float)(y * TS); nr++; }
            if (map[y][x] == 'L' || map[y][x] == 'R') map[y][x] = ' ';
        }
    n_scales = imin(nl, nr);
    for (int s = 0; s < n_scales; s++) scales[s].off = 0;
    gate_open[0] = gate_open[1] = false;
    for (int k = 0; k < MAX_FOES; k++) foes[k].dir = -1;
}

/* The ship hovers over the furthest open sky at or behind the troop's
 * progress: it can't drop soldiers through a roof. */
static float ship_target(void) {
    float t = fclamp(progress_x - 20, 0, (float)(LW * TS - 40));
    while (t > 0 && rect_tiles(t + 16, 12, SW, SH)) t -= 2;
    return t;
}

static void spawn_soldier(void);

static void start_attempt(void) {
    load_level(level_i);
    lives_start = lives = start_lives(level_i);
    kills = 0;
    respawn_t = 0;
    last_stone = -1;
    ship_x = (float)start_x;
    progress_x = (float)start_x;
    P.mode = M_GONE;
    spawn_soldier();
    cam_x = fclamp(P.x - 110, 0, (float)(LW * TS - SCREEN_W));
    state = S_PLAY;
    state_t = 0;
    shake = 0;
    game_set_pausable(true);
    music_play(boss.on ? TT_MUS_BOSS : TT_MUS_WORLD[world - 1]);
}

/* ------------------------------------------------------------------ */
/* the soldier                                                          */

static void spawn_soldier(void) {
    memset(&P, 0, sizeof P);
    P.dir = 1;
    P.breath = BREATH;
    P.on_pan = -1;
    if (world == 4 && door_x >= 0) {
        /* no ship in the toy chest: the next soldier marches out of the door */
        P.x = (float)(door_x * TS + 2);
        P.y = (float)(door_y * TS + TS - SH);
        P.mode = M_ENTER;
        P.enter_t = 30;
    } else {
        if (rect_tiles(ship_x + 16, 12, SW, SH)) ship_x = ship_target();
        P.x = ship_x + 16;
        P.y = 12;
        P.mode = M_CHUTE;
    }
    sfx_play_name("tt_deploy");
}

static void fail_attempt(int why) {
    state = S_FAIL;
    state_t = 0;
    fail_reason = why;
    music_restart(TT_MUS_FAIL);
    game_set_pausable(false);
}

static void kill_foe(int i) {
    Foe *f = &foes[i];
    if (!f->alive) return;
    float w, h;
    foe_size(f->kind, &w, &h);
    f->alive = 0;
    kills++;
    burst(f->x + w / 2, f->y + h / 2, C_LIGHT, 10, 1.2f);
    sfx_play_name("tt_foe");
    if (kills % 3 == 0) {
        lives++;
        popup_n(f->x + w / 2, f->y - 4, 1);
        sfx_play_name("tt_life");
    }
}

static void grow_vine(int tx, int ty) {
    for (int i = 0; i < MAX_VINES; i++)
        if (vines[i].left <= 0) { vines[i] = (Vine){tx, ty, 12, 0}; return; }
}

/* the soldier is spent: lives go down by one and something may be left behind */
static void soldier_die(int how) {
    if (P.mode == M_GONE) return;
    float cx = P.x + SW / 2.0f;
    switch (how) {
    case DIE_CORPSE: add_body(water_at(cx, P.y + SH - 2) ? B_FLOAT : B_CORPSE, cx - 5, P.y + SH - 4); break;
    case DIE_FLOAT: add_body(B_FLOAT, cx - 5, P.y + SH - 4); break;
    case DIE_VINE: grow_vine((int)floorf(cx / TS), (int)floorf((P.y + SH - 1) / TS)); break;
    default: break;
    }
    if (how != DIE_NONE) burst(cx, P.y + 4, C_GREY, 6, 0.8f);
    else burst(cx, P.y + 4, C_ORANGE, 12, 1.0f);
    lives--;
    P.mode = M_GONE;
    respawn_t = RESPAWN;
    shake = imax(shake, 6);
    sfx_play_name(how == DIE_NONE ? "tt_burnout" : "tt_die");
}

/* the Pop: tiles and foes within two steps are blown away */
static void explode(float cx, float cy) {
    int ctx = (int)floorf(cx / TS), cty = (int)floorf(cy / TS);
    for (int dy = -2; dy <= 2; dy++)
        for (int dx = -2; dx <= 2; dx++) {
            if (iabs(dx) + iabs(dy) > 2) continue;
            int tx = ctx + dx, ty = cty + dy;
            if (tile(tx, ty) == 'x') {
                map[ty][tx] = ' ';
                burst((float)(tx * TS + 5), (float)(ty * TS + 5), world == 2 ? C_CYAN : C_TAN, 6, 1.0f);
            }
            part_add((float)(tx * TS + 5), (float)(ty * TS + 5), 0, 0, 14 - (iabs(dx) + iabs(dy)) * 2, C_YELLOW, 1, 0);
        }
    for (int i = 0; i < MAX_FOES; i++) {
        Foe *f = &foes[i];
        if (!f->alive) continue;
        float w, h;
        foe_size(f->kind, &w, &h);
        int ftx = (int)floorf((f->x + w / 2) / TS), fty = (int)floorf((f->y + h / 2) / TS);
        if (iabs(ftx - ctx) + iabs(fty - cty) <= 2) kill_foe(i);
    }
    if (boss.on && !boss.dead) {
        bool hit = false;
        for (int dy = -2; dy <= 2 && !hit; dy++)
            for (int dx = -2; dx <= 2 && !hit; dx++)
                if (iabs(dx) + iabs(dy) <= 2 && tile(ctx + dx, cty + dy) == 'j') hit = true;
        if (hit) {
            boss.hp--;
            boss.flash = 20;
            sfx_play_name("tt_bosshit");
            if (boss.hp <= 0) { boss.dead = true; boss.death_t = 0; music_stop(); }
        }
    }
    burst(cx, cy, C_ORANGE, 16, 1.8f);
    burst(cx, cy, C_YELLOW, 10, 1.1f);
    shake = imax(shake, 10);
    sfx_play_name("tt_pop");
}

static void make_stone(float cx, float bottom, bool airborne) {
    float x = cx - 5, y = bottom - 10;
    /* squeeze out of walls a little if needed */
    for (int k = 0; k < 6 && rect_tiles(x, y, 10, 10); k++) {
        if (!rect_tiles(x + 1, y, 10, 10)) x += 1;
        else if (!rect_tiles(x - 1, y, 10, 10)) x -= 1;
        else y -= 1;
    }
    int b = add_body(B_STONE, x, y);
    if (b >= 0) {
        bodies[b].smash = airborne;
        last_stone = b;
    }
    sfx_play_name("tt_stone");
}

static void ritual(void) {
    float cx = P.x + SW / 2.0f;
    if (btn(BTN_UP)) {
        lives--;
        P.mode = M_GONE;
        respawn_t = RESPAWN;
        explode(cx, P.y + SH / 2.0f);
    } else if (btn(BTN_DOWN)) {
        lives--;
        P.mode = M_GONE;
        respawn_t = RESPAWN;
        make_stone(cx, P.y + SH, !P.ground);
    } else {
        /* the Lance: fly straight ahead until something stops you */
        lives--;
        P.mode = M_ARROW;
        P.x = cx - 5;
        P.y = P.y + 3;
        P.vx = (float)P.dir * ARROW_V;
        P.vy = 0;
        sfx_play_name("tt_lance");
    }
}

/* where the Jack's head is when he has sprung out of the chest */
static void jack_head_rect(float *x, float *y, float *w, float *h) {
    *x = (float)(boss.bx + 10);
    *y = (float)(boss.by - 24);
    *w = 20;
    *h = 15;
}

static void lodge(void) {
    add_body(B_LODGED, P.x, P.y);
    burst(P.x + (P.dir > 0 ? 10 : 0), P.y + 2, C_GREY, 5, 0.7f);
    P.mode = M_GONE;
    respawn_t = RESPAWN;
    shake = imax(shake, 3);
    sfx_play_name("tt_thunk");
}

static void touch_pickups(float x, float y, float w, float h) {
    int x0 = (int)floorf(x / TS), x1 = (int)floorf((x + w - 0.01f) / TS);
    int y0 = (int)floorf(y / TS), y1 = (int)floorf((y + h - 0.01f) / TS);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++) {
            char c = tile(tx, ty);
            if (c >= '1' && c <= '9') {
                int n = c - '0';
                lives += n;
                map[ty][tx] = ' ';
                popup_n((float)(tx * TS + 5), (float)(ty * TS), n);
                burst((float)(tx * TS + 5), (float)(ty * TS + 5), C_YELLOW, 8, 1.0f);
                sfx_play_name("tt_life");
            }
        }
}

static void arrow_update(void) {
    for (int k = 0; k < 4 && P.mode == M_ARROW; k++) {
        float nx = P.x + (float)P.dir;
        if (blocked(nx, P.y, 10, 4, -1)) { lodge(); return; }
        P.x = nx;
        touch_pickups(P.x, P.y, 10, 4);
        if (boss.on && !boss.dead && boss.mouth) {
            float hx, hy, hw, hh;
            jack_head_rect(&hx, &hy, &hw, &hh);
            if (overlap(P.x, P.y, 10, 4, hx, hy, hw, hh)) {
                /* the Jack swallows lances whole; a burning one finishes him */
                if (P.burn > 0) {
                    boss.hp = 0;
                    boss.dead = true;
                    boss.death_t = 0;
                    boss.flash = 40;
                    music_stop();
                    sfx_play_name("tt_bosshit");
                } else {
                    sfx_play_name("tt_gulp");
                }
                P.mode = M_GONE;
                respawn_t = RESPAWN;
                return;
            }
        }
        if (rect_has(P.x, P.y, 10, 4, 'f')) P.burn = BURN;
        if (water_at(P.x + 5, P.y + 2)) P.burn = 0;
        for (int i = 0; i < MAX_FOES; i++) {
            Foe *f = &foes[i];
            if (!f->alive) continue;
            float w, h;
            foe_size(f->kind, &w, &h);
            if (!overlap(P.x, P.y, 10, 4, f->x, f->y, w, h)) continue;
            if (f->kind == F_RAM && f->dir == -P.dir) {
                /* the ram's brass brow turns a lance aside */
                sfx_play_name("tt_clang");
                P.y -= 2;
                P.x -= (float)P.dir * 3;
                soldier_die(DIE_CORPSE);
                lives++; /* soldier_die counts him again; the lance already paid */
                return;
            }
            kill_foe(i);
        }
    }
    if (P.mode == M_ARROW && btnp(BTN_B)) {
        if (btn(BTN_UP)) { P.mode = M_GONE; respawn_t = RESPAWN; explode(P.x + 5, P.y + 2); }
        else if (btn(BTN_DOWN)) { P.mode = M_GONE; respawn_t = RESPAWN; make_stone(P.x + 5, P.y + 7, true); }
    }
}

static bool on_vine(void) { return tile((int)floorf((P.x + SW / 2.0f) / TS), (int)floorf((P.y + SH / 2.0f) / TS)) == 'v'; }

/* move the soldier's box, stopping at walls; returns true when it lands */
static void soldier_move(void) {
    /* horizontal */
    float dx = P.vx;
    int steps = (int)ceilf(fabsf(dx));
    for (int k = 0; k < steps; k++) {
        float step = fclamp(dx, -1, 1);
        dx -= step;
        if (blocked(P.x + step, P.y, SW, SH, -1)) { P.vx = 0; break; }
        P.x += step;
    }
    /* vertical */
    float dy = P.vy;
    P.ground = false;
    steps = (int)ceilf(fabsf(dy));
    for (int k = 0; k < steps; k++) {
        float step = fclamp(dy, -1, 1);
        dy -= step;
        if (blocked(P.x, P.y + step, SW, SH, -1)) {
            if (step > 0) P.ground = true;
            P.vy = 0;
            break;
        }
        float top;
        int pan = -1;
        if (step > 0 && land_on_top(P.x, SW, P.y + SH, P.y + SH + step, &top, &pan, -1)) {
            P.y = top - SH;
            P.vy = 0;
            P.ground = true;
            P.on_pan = pan;
            break;
        }
        P.y += step;
    }
    if (!P.ground && P.vy >= 0) {
        /* resting exactly on something counts as ground */
        float top;
        int pan = -1;
        if (blocked(P.x, P.y + 1, SW, SH, -1)) P.ground = true;
        else if (land_on_top(P.x, SW, P.y + SH, P.y + SH + 1, &top, &pan, -1)) { P.ground = true; P.on_pan = pan; }
    }
    if (!P.ground) P.on_pan = -1;
}

static void soldier_update(void) {
    float cx = P.x + SW / 2.0f;
    int hx = (btn(BTN_RIGHT) ? 1 : 0) - (btn(BTN_LEFT) ? 1 : 0);
    if (hx) P.dir = hx;
    P.anim++;
    switch (P.mode) {
    case M_CHUTE:
        if (btn(BTN_B)) {
            if (++P.giveup >= GIVEUP) { fail_attempt(1); return; }
        } else {
            P.giveup = 0;
        }
        P.vx = (float)hx * CHUTE_VX;
        P.vy = CHUTE_VY;
        soldier_move();
        if (P.ground || water_at(cx, P.y + SH)) { P.mode = water_at(cx, P.y + SH) ? M_SWIM : M_WALK; sfx_play_name("tt_land"); }
        return;
    case M_ENTER:
        if (btn(BTN_B)) {
            if (++P.giveup >= GIVEUP) { fail_attempt(1); return; }
        } else {
            P.giveup = 0;
            if (--P.enter_t <= 0) P.mode = M_WALK;
        }
        return;
    case M_ARROW: arrow_update(); return;
    case M_CLIMB:
        if (!on_vine()) { P.mode = M_WALK; break; }
        P.vx = 0;
        P.vy = btn(BTN_UP) ? -0.8f : btn(BTN_DOWN) ? 0.8f : 0;
        if (btnp(BTN_A)) { P.mode = M_WALK; P.vy = -2.6f; P.vx = (float)hx * RUN; sfx_play_name("tt_jump"); }
        else if (hx) { P.mode = M_WALK; P.vx = (float)hx * RUN; }
        soldier_move();
        if (P.mode == M_CLIMB && btnp(BTN_B)) ritual();
        return;
    default: break;
    }
    /* in once his middle is under, out only once his feet are clear */
    if (P.mode == M_WALK && water_at(cx, P.y + SH / 2.0f)) { P.mode = M_SWIM; P.burn = 0; sfx_play_name("tt_splash"); }
    if (P.mode == M_SWIM && !water_at(cx, P.y + SH - 1)) P.mode = M_WALK;
    if (P.mode == M_SWIM) {
        bool head_out = !water_at(cx, P.y + 2);
        P.vx = (float)hx * 0.75f;
        P.vy += 0.08f;
        if (btn(BTN_A)) {
            /* holding A swims up, and keeps his head above the surface */
            P.vy -= 0.2f;
            if (head_out && P.vy < 0 && P.vy > -1.0f) P.vy = 0;
        }
        P.vy = fclamp(P.vy, -1.5f, 1.0f);
        if (!water_at(cx, P.y - 2) && btnp(BTN_A)) { P.vy = -3.2f; sfx_play_name("tt_jump"); }
        soldier_move();
        if (water_at(cx, P.y + 1)) {
            if (--P.breath <= 0) { soldier_die(DIE_FLOAT); return; }
        } else {
            P.breath = imin(BREATH, P.breath + 4);
        }
    } else {
        P.breath = imin(BREATH, P.breath + 4);
        P.vx = (float)hx * RUN;
        if (P.ground && btnp(BTN_A)) { P.vy = -JUMP_V; sfx_play_name("tt_jump"); }
        if (!btn(BTN_A) && P.vy < -1.0f) P.vy *= 0.6f;
        P.vy = fminf(P.vy + GRAV, 4.0f);
        soldier_move();
        if ((btn(BTN_UP) || (btn(BTN_DOWN) && !P.ground)) && on_vine() && !btnp(BTN_B)) {
            P.mode = M_CLIMB;
            P.x = floorf(cx / TS) * TS + 2;
            P.vy = 0;
        }
    }
    if (btnp(BTN_B)) { ritual(); return; }
    /* hazards */
    if (rect_has(P.x, P.y, SW, SH, 'f') && P.burn == 0 && P.mode != M_SWIM) { P.burn = BURN; sfx_play_name("tt_ignite"); }
    if (P.burn > 0 && --P.burn == 0) { soldier_die(DIE_NONE); return; }
    if (P.seed > 0 && --P.seed == 0) { soldier_die(DIE_VINE); return; }
    if (on_spikes(P.x + 1, P.y + 2, SW - 2, SH - 2)) { soldier_die(DIE_CORPSE); return; }
    if (P.y > LEVEL_H + 8) { soldier_die(DIE_NONE); return; }
    touch_pickups(P.x, P.y, SW, SH);
    if (rect_has(P.x + 2, P.y + 2, SW - 4, SH - 4, 'E')) {
        state = S_CLEAR;
        state_t = 0;
        return;
    }
}

/* ------------------------------------------------------------------ */
/* bodies, scales, switches                                             */

static bool body_resting_on_pan(const Body *b, int s, int side) {
    float py = pan_y(s, side), px = pan_x(s, side);
    return fabsf(b->y + body_h(b->kind) - py) < 0.6f && b->x < px + 30 && px < b->x + 10;
}

static void bodies_update(void) {
    for (int i = 0; i < MAX_BODIES; i++) {
        Body *b = &bodies[i];
        if (!b->alive) continue;
        float h = body_h(b->kind);
        if (b->kind == B_LODGED) {
            if (!blocked(b->x - 1, b->y, 12, h, i)) {
                /* the wall it was stuck in is gone: it drops */
                b->kind = B_CORPSE;
            } else continue;
        }
        if (b->kind == B_FLOAT) {
            /* drift up to the surface and bob there */
            if (water_at(b->x + 5, b->y + 1)) b->y -= 0.5f;
            else if (!water_at(b->x + 5, b->y + 5)) { b->kind = B_CORPSE; }
            continue;
        }
        bool wet = water_at(b->x + 5, b->y + h / 2);
        if (b->kind == B_CORPSE && wet) { b->kind = B_FLOAT; continue; }
        b->vy = fminf(b->vy + GRAV, wet ? 1.2f : 4.0f);
        float dy = b->vy;
        int steps = (int)ceilf(dy);
        for (int k = 0; k < steps; k++) {
            float step = fminf(dy, 1);
            dy -= step;
            if (b->kind == B_STONE && b->smash) {
                /* a dropped stone smashes down through breakable blocks */
                int ty = (int)floorf((b->y + h + step - 0.01f) / TS);
                int x0 = (int)floorf(b->x / TS), x1 = (int)floorf((b->x + 9.99f) / TS);
                for (int tx = x0; tx <= x1; tx++)
                    if (tile(tx, ty) == 'x') {
                        map[ty][tx] = ' ';
                        burst((float)(tx * TS + 5), (float)(ty * TS + 5), C_TAN, 6, 1.0f);
                        sfx_play_name("tt_crash");
                    }
            }
            if (blocked(b->x, b->y + step, 10, h, i)) { if (b->vy > 1.5f && b->kind == B_STONE) sfx_play_name("tt_thud"); b->vy = 0; b->smash = 0; break; }
            float top;
            if (land_on_top(b->x, 10, b->y + h, b->y + h + step, &top, NULL, i)) { b->y = top - h; b->vy = 0; b->smash = 0; break; }
            b->y += step;
        }
        if (b->kind == B_STONE && b->vy > 0.3f) {
            for (int f = 0; f < MAX_FOES; f++) {
                Foe *e = &foes[f];
                if (!e->alive || e->kind == F_DRAGON) continue;
                float w, fh;
                foe_size(e->kind, &w, &fh);
                if (overlap(b->x, b->y, 10, h, e->x, e->y, w, fh)) kill_foe(f);
            }
        }
        if (b->y > LEVEL_H + 20) b->alive = 0;
    }
    if (last_stone >= 0 && respawn_t > 0 && bodies[last_stone].alive && bodies[last_stone].vy > 0.2f && btnp(BTN_B) && btn(BTN_UP)) {
        /* a falling stone can still go off */
        Body *b = &bodies[last_stone];
        b->alive = 0;
        explode(b->x + 5, b->y + 5);
        last_stone = -1;
    }
}

static int weight_on_pan(int s, int side) {
    int w = 0;
    if ((P.mode == M_WALK) && P.on_pan == s * 2 + side) w++;
    for (int i = 0; i < MAX_BODIES; i++) {
        Body *b = &bodies[i];
        if (b->alive && b->kind != B_LODGED && body_resting_on_pan(b, s, side)) w++;
    }
    return w;
}

static void scales_update(void) {
    for (int s = 0; s < n_scales; s++) {
        int wl = weight_on_pan(s, 0), wr = weight_on_pan(s, 1);
        float target = wl > wr ? 20.0f : wr > wl ? -20.0f : 0.0f;
        float old = scales[s].off;
        scales[s].off = fapproach(old, target, 0.5f);
        float d = scales[s].off - old;
        if (d == 0) continue;
        /* carry whatever rests on the pans */
        for (int side = 0; side < 2; side++) {
            float move = side == 0 ? d : -d;
            float py_old = pan_y(s, side) - move;
            for (int i = 0; i < MAX_BODIES; i++) {
                Body *b = &bodies[i];
                if (!b->alive || b->kind == B_LODGED) continue;
                float px = pan_x(s, side);
                if (fabsf(b->y + body_h(b->kind) - py_old) < 0.6f && b->x < px + 30 && px < b->x + 10) b->y += move;
            }
            if (P.mode == M_WALK && P.on_pan == s * 2 + side) P.y += move;
        }
    }
}

static bool weight_on_tile(int tx, int ty) {
    float x = (float)(tx * TS), y = (float)(ty * TS);
    if ((P.mode == M_WALK || P.mode == M_SWIM || P.mode == M_CLIMB) && overlap(P.x, P.y, SW, SH, x, y + 4, TS, 6)) return true;
    for (int i = 0; i < MAX_BODIES; i++) {
        Body *b = &bodies[i];
        if (b->alive && b->kind != B_LODGED && overlap(b->x, b->y, 10, body_h(b->kind), x, y + 4, TS, 6)) return true;
    }
    return false;
}

static void switches_update(void) {
    bool pressed[2] = {false, false};
    int x0 = imax(0, (int)(cam_x / TS) - 40), x1 = imin(LW - 1, (int)(cam_x / TS) + 72);
    for (int ty = 0; ty < TT_ROWS; ty++)
        for (int tx = x0; tx <= x1; tx++) {
            char c = map[ty][tx];
            if (c == '[' && weight_on_tile(tx, ty)) pressed[0] = true;
            if (c == ']' && weight_on_tile(tx, ty)) pressed[1] = true;
        }
    for (int g = 0; g < 2; g++) {
        if (pressed[g] == gate_open[g]) continue;
        if (!pressed[g]) {
            /* a gate won't shut on anything standing in it */
            char gc = g == 0 ? '|' : '!';
            if (P.mode != M_GONE && rect_has(P.x, P.y, SW, SH, gc)) continue;
        }
        gate_open[g] = pressed[g];
        sfx_play_name(pressed[g] ? "tt_gate" : "tt_gate_shut");
    }
}

static void vines_update(void) {
    for (int i = 0; i < MAX_VINES; i++) {
        Vine *v = &vines[i];
        if (v->left <= 0) continue;
        if (++v->t < 5) continue;
        v->t = 0;
        char c = tile(v->x, v->y);
        if (solid_char(c) || v->y < 0) { v->left = 0; continue; }
        if (c == ' ' || c == 'w' || c == 'v') map[v->y][v->x] = 'v';
        v->y--;
        v->left--;
    }
}

/* ------------------------------------------------------------------ */
/* foes and shots                                                       */

static int add_shot(int kind, float x, float y, float vx, float vy) {
    for (int i = 0; i < MAX_SHOTS; i++)
        if (!shots[i].alive) { shots[i] = (Shot){x, y, vx, vy, (uint8_t)kind, 1, 0, kind == SH_FIRE ? 60 : 9999}; return i; }
    return -1;
}

static bool soldier_present(void) { return P.mode == M_WALK || P.mode == M_SWIM || P.mode == M_CLIMB; }

static bool ground_under(float x, float y) { return blocked(x, y, 1, 1, -1) && !rect_has(x, y - TS, 1, 1, '^'); }

static void foe_fall(Foe *f, float w, float h) {
    f->vy = fminf(f->vy + GRAV, 4.0f);
    float dy = f->vy;
    int steps = (int)ceilf(dy);
    for (int k = 0; k < steps; k++) {
        float step = fminf(dy, 1);
        dy -= step;
        if (blocked(f->x, f->y + step, w, h, -1)) { f->vy = 0; break; }
        float top;
        if (land_on_top(f->x, w, f->y + h, f->y + h + step, &top, NULL, -1)) { f->y = top - h; f->vy = 0; break; }
        f->y += step;
    }
}

static bool foe_grounded(Foe *f, float w, float h) {
    float top;
    return blocked(f->x, f->y + 1, w, h, -1) || land_on_top(f->x, w, f->y + h, f->y + h + 1, &top, NULL, -1);
}

static void foes_update(void) {
    float scx = P.x + SW / 2.0f, scy = P.y + SH / 2.0f;
    bool here = soldier_present();
    for (int i = 0; i < MAX_FOES; i++) {
        Foe *f = &foes[i];
        if (!f->alive) continue;
        float w, h;
        foe_size(f->kind, &w, &h);
        /* only foes near the screen move */
        if (f->x < cam_x - 80 || f->x > cam_x + SCREEN_W + 80) continue;
        f->t++;
        float fcx = f->x + w / 2, fcy = f->y + h / 2;
        switch (f->kind) {
        case F_MOUSE: {
            bool gr = foe_grounded(f, w, h);
            if (gr) {
                float nx = f->x + f->dir * 0.5f;
                float front = f->dir > 0 ? nx + w : nx - 1;
                if (blocked(nx, f->y, w, h, -1) || !ground_under(front, f->y + h + 1)) f->dir = (int8_t)-f->dir;
                else f->x = nx;
            }
            foe_fall(f, w, h);
            break;
        }
        case F_DART: {
            float nx = f->x + f->dir * 1.0f;
            if (blocked(nx, f->y, w, h, -1)) f->dir = (int8_t)-f->dir;
            else f->x = nx;
            break;
        }
        case F_RAM: {
            bool gr = foe_grounded(f, w, h);
            if (f->state == 0) {
                if (gr) {
                    float nx = f->x + f->dir * 0.4f;
                    float front = f->dir > 0 ? nx + w : nx - 1;
                    if (blocked(nx, f->y, w, h, -1) || !ground_under(front, f->y + h + 1)) f->dir = (int8_t)-f->dir;
                    else f->x = nx;
                }
                if (here && gr && fabsf(scy - fcy) < 8 && fabsf(scx - fcx) < 96) {
                    f->dir = (int8_t)(scx > fcx ? 1 : -1);
                    f->state = 1;
                    f->t = 0;
                    sfx_play_name("tt_snort");
                }
            } else if (f->state == 1) {
                if (f->t >= 24) { f->state = 2; f->t = 0; }
            } else if (f->state == 2) {
                float nx = f->x + f->dir * 2.2f;
                if (blocked(nx, f->y, w, h, -1)) { f->state = 3; f->t = 0; shake = imax(shake, 4); sfx_play_name("tt_thud"); }
                else f->x = nx;
                if (f->t > 110 && gr) { f->state = 3; f->t = 0; }
            } else if (f->t >= 50) {
                f->state = 0;
            }
            foe_fall(f, w, h);
            break;
        }
        case F_FISH: {
            bool chase = here && P.mode == M_SWIM && fabsf(scx - fcx) < 90 && fabsf(scy - fcy) < 60;
            float vx, vy = 0;
            if (chase) {
                float dx = scx - fcx, dy = scy - fcy, d = sqrtf(dx * dx + dy * dy) + 0.01f;
                vx = dx / d * 0.7f;
                vy = dy / d * 0.7f;
                f->dir = (int8_t)(dx > 0 ? 1 : -1);
            } else {
                vx = f->dir * 0.4f;
            }
            float nx = f->x + vx, ny = f->y + vy;
            if (water_at(nx + (vx > 0 ? w : 0), fcy) && water_at(nx + w / 2, fcy) && !blocked(nx, f->y, w, h, -1)) f->x = nx;
            else if (!chase) f->dir = (int8_t)-f->dir;
            if (water_at(f->x + w / 2, ny + (vy > 0 ? h : 0)) && !blocked(f->x, ny, w, h, -1)) f->y = ny;
            break;
        }
        case F_BUG: {
            float nx = f->x + f->dir * 0.8f;
            if (blocked(nx, f->y, w, h, -1)) f->dir = (int8_t)-f->dir;
            else f->x = nx;
            foe_fall(f, w, h);
            if (f->seeded > 0 && --f->seeded == 0) {
                grow_vine((int)floorf(fcx / TS), (int)floorf((f->y + h - 1) / TS));
                kill_foe(i);
                continue;
            }
            break;
        }
        case F_DRAGON:
            if (here && fabsf(scx - fcx) < 130 && fabsf(scy - fcy) < 50) f->dir = (int8_t)(scx > fcx ? 1 : -1);
            if (f->t % 150 == 100 && fabsf(scx - fcx) < 150) {
                add_shot(SH_FIRE, fcx + f->dir * 6, f->y + 4, f->dir * 1.5f, 0);
                sfx_play_name("tt_fire");
            }
            break;
        }
        /* spikes and the drop claim foes too */
        if (f->kind != F_DART && f->kind != F_FISH && f->kind != F_DRAGON && on_spikes(f->x + 1, f->y + h - 3, w - 2, 3)) { kill_foe(i); continue; }
        if (f->y > LEVEL_H + 10) { kill_foe(i); continue; }
    }
}

static void shots_update(void) {
    for (int i = 0; i < MAX_SHOTS; i++) {
        Shot *s = &shots[i];
        if (!s->alive) continue;
        s->t++;
        if (s->kind == SH_SEED) s->vy += 0.1f;
        s->x += s->vx;
        s->y += s->vy;
        if (s->t > s->life || s->y > LEVEL_H + 10 || s->x < cam_x - 40 || s->x > cam_x + SCREEN_W + 40) { s->alive = 0; continue; }
        if (rect_tiles(s->x - 1, s->y - 1, 2, 2) || rect_objs(s->x - 1, s->y - 1, 2, 2, -1)) {
            s->alive = 0;
            if (s->kind != SH_ORB) burst(s->x, s->y, s->kind == SH_SEED ? C_LEAF : C_ORANGE, 4, 0.6f);
            continue;
        }
        if (s->kind == SH_FIRE && water_at(s->x, s->y)) { s->alive = 0; continue; }
        if (s->kind == SH_SEED)
            for (int f = 0; f < MAX_FOES; f++) {
                Foe *e = &foes[f];
                if (!e->alive || e->kind != F_BUG || e->seeded) continue;
                if (overlap(s->x - 2, s->y - 2, 4, 4, e->x, e->y, 8, 7)) { e->seeded = SEEDED; s->alive = 0; sfx_play_name("tt_seed"); break; }
            }
        if (!s->alive) continue;
        if (soldier_present() || P.mode == M_ARROW) {
            float pw = P.mode == M_ARROW ? 10 : SW, ph = P.mode == M_ARROW ? 4 : SH;
            if (overlap(s->x - 2, s->y - 2, 4, 4, P.x, P.y, pw, ph)) {
                s->alive = 0;
                if (s->kind == SH_SEED) { if (!P.seed && P.mode != M_ARROW) { P.seed = SEEDED; sfx_play_name("tt_seed"); } }
                else if (s->kind == SH_FIRE) { if (!P.burn) { P.burn = BURN; sfx_play_name("tt_ignite"); } }
                else if (P.mode != M_ARROW) soldier_die(DIE_CORPSE);
            }
        }
    }
    /* creeper pots and the carved heads */
    int x0 = imax(0, (int)(cam_x / TS) - 4), x1 = imin(LW - 1, (int)(cam_x / TS) + 36);
    for (int ty = 0; ty < TT_ROWS; ty++)
        for (int tx = x0; tx <= x1; tx++) {
            char c = map[ty][tx];
            float px = (float)(tx * TS + 5), py = (float)(ty * TS);
            int phase = (tx * 37 + ty * 11) % 60;
            if (c == 'P' && (frame_t + phase) % 140 == 0) {
                /* long and short lobs, to either side in turn */
                int n = (frame_t + phase) / 140;
                int side = n % 2 ? 1 : -1;
                add_shot(SH_SEED, px, py - 1, side * ((n / 2) % 2 ? 0.6f : 1.1f), -2.8f);
                sfx_play_name("tt_spit");
            }
            bool topleft = tile(tx - 1, ty) != c && tile(tx, ty - 1) != c;
            if (!topleft) continue;
            float dir = tx * TS < LW * TS / 2 ? 1.0f : -1.0f;
            if (c == 'N' && (frame_t + phase) % 160 == 0) {
                int k = add_shot(SH_FIRE, px + 5 + dir * 10, py + 10, dir * 1.4f, 0);
                if (k >= 0) shots[k].life = 130; /* the carved head's breath carries across the room */
                sfx_play_name("tt_fire");
            }
            if (c == 'K' && (frame_t + phase) % 150 == 0) {
                add_shot(SH_SEED, px + 5 + dir * 10, py + 6, dir * 1.2f, -2.4f);
                sfx_play_name("tt_spit");
            }
            if (c == 'Y' && (frame_t + phase) % 300 == 0) {
                int bugs = 0;
                for (int f = 0; f < MAX_FOES; f++) bugs += foes[f].alive && foes[f].kind == F_BUG;
                if (bugs < 2) {
                    int k = add_foe(F_BUG, px + 1 + dir * 12, py + 13);
                    if (k >= 0) foes[k].dir = (int8_t)dir;
                    sfx_play_name("tt_spawn");
                }
            }
        }
}

static void contacts(void) {
    if (!soldier_present()) return;
    for (int i = 0; i < MAX_FOES; i++) {
        Foe *f = &foes[i];
        if (!f->alive) continue;
        float w, h;
        foe_size(f->kind, &w, &h);
        if (!overlap(P.x + 1, P.y + 1, SW - 2, SH - 1, f->x + 1, f->y + 1, w - 2, h - 2)) continue;
        if (P.burn > 0 && f->kind != F_FISH && f->kind != F_DRAGON) { kill_foe(i); continue; }
        soldier_die(DIE_CORPSE);
        return;
    }
}

/* ------------------------------------------------------------------ */
/* the Jack of the Chest                                                */

static void boss_update(void) {
    if (!boss.on) return;
    if (boss.flash > 0) boss.flash--;
    if (boss.dead) {
        boss.death_t++;
        if (boss.death_t % 8 == 0) burst((float)(boss.bx + frame_t * 7 % 40), (float)(boss.by + frame_t * 13 % 50), C_ORANGE, 10, 1.5f);
        if (boss.death_t % 16 == 0) sfx_play_name("tt_pop");
        shake = 4;
        if (boss.death_t >= 150) { state = S_CLEAR; state_t = 0; }
        return;
    }
    boss.t++;
    /* the lid opens and the Jack shows his mouth for a while */
    int cyc = boss.t % 300;
    bool was = boss.mouth;
    boss.mouth = cyc >= 180;
    if (boss.mouth && !was) sfx_play_name("tt_jack");
    if (soldier_present() && ++boss.shot_t >= 170) {
        boss.shot_t = 0;
        float ox = (float)(boss.bx + 20), oy = (float)(boss.by - (boss.mouth ? 12 : 2));
        float a = atan2f(P.y + SH / 2.0f - oy, P.x + SW / 2.0f - ox);
        for (int k = -1; k <= 1; k++) add_shot(SH_ORB, ox, oy, cosf(a + k * 0.35f) * 0.8f, sinf(a + k * 0.35f) * 0.8f);
        sfx_play_name("tt_orb");
    }
}

/* ------------------------------------------------------------------ */
/* play loop                                                            */

static void clear_level(void) {
    int delta = lives - lives_start;
    bool first = !cleared(level_i);
    bool better = first || delta > sv.best[level_i];
    if (better) sv.best[level_i] = (int16_t)delta;
    sv.cleared |= (uint16_t)(1u << level_i);
    if (level_i == 5) game_award(GOAL_BEACON);
    if (level_i == TT_LEVELS - 1) { sv.beaten = 1; game_award(GOAL_SAUCER); }
    if (cleared(TT_LEVELS - 1) && start_lives(TT_LEVELS - 1) + sv.best[TT_LEVELS - 1] >= 75) game_award(GOAL_ALIEN);
    save_now();
}

static void on_cleared(void) {
    clear_level();
    music_restart(TT_MUS_CLEAR);
    game_set_pausable(false);
}

static void play_update(void) {
    frame_t++;
    if (P.mode != M_GONE) soldier_update();
    if (state == S_CLEAR) { on_cleared(); return; }
    if (state != S_PLAY) return;
    bodies_update();
    scales_update();
    switches_update();
    vines_update();
    if (!freeze_foes) {
        foes_update();
        shots_update();
    }
    boss_update();
    if (state == S_CLEAR) { on_cleared(); return; }
    contacts();
    /* the ship keeps up with the furthest soldier and never turns back */
    if (P.mode == M_WALK || P.mode == M_SWIM || P.mode == M_CLIMB) progress_x = fmaxf(progress_x, P.x);
    float target = ship_target();
    if (ship_x < target) ship_x = fminf(target, ship_x + fmaxf(1.2f, (target - ship_x) * 0.05f));
    /* world 4: a door passed becomes the new way in */
    if (world == 4 && soldier_present()) {
        int tx = (int)floorf((P.x + SW / 2.0f) / TS), ty = (int)floorf((P.y + SH / 2.0f) / TS);
        if (tile(tx, ty) == 'D' && tx > door_x) { door_x = tx; door_y = ty; sfx_play_name("tt_door"); }
    }
    if (P.mode == M_GONE && respawn_t > 0 && --respawn_t == 0) {
        if (lives <= 0) { fail_attempt(0); return; }
        spawn_soldier();
    }
    /* camera */
    float focus = P.mode != M_GONE ? P.x : (world == 4 ? (float)(door_x * TS) : ship_x + 16);
    float want = fclamp(focus - 150, 0, (float)(LW * TS - SCREEN_W));
    cam_x += (want - cam_x) * 0.12f;
    if (fabsf(want - cam_x) < 0.5f) cam_x = want;
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
        if (p->kind == 0) p->vy += 0.08f;
    }
}

static void enter_level(int i) {
    level_i = i;
    load_level(i);
    state = S_INTRO;
    state_t = 0;
    game_set_pausable(false);
    music_stop();
    sfx_play_name("tt_intro");
}

static void tt_update(void) {
    state_t++;
    parts_update();
    switch (state) {
    case S_TITLE:
        frame_t++;
        game_set_pausable(false);
        if (btnp(BTN_B)) game_exit_to_library();
        if (btnp(BTN_A) || btnp(BTN_START)) {
            sfx_play_name("ui_ok");
            input_consume();
            state = S_MAP;
            state_t = 0;
            map_sel = 0;
            while (map_sel + 1 < TT_LEVELS && unlocked(map_sel + 1)) map_sel++;
            music_play(TT_MUS_MAP);
        }
        break;
    case S_MAP:
        frame_t++;
        if (btn_repeat(BTN_RIGHT) || btn_repeat(BTN_DOWN)) {
            if (map_sel + 1 < TT_LEVELS && unlocked(map_sel + 1)) { map_sel++; sfx_play_name("ui_move"); }
        }
        if (btn_repeat(BTN_LEFT) || btn_repeat(BTN_UP)) {
            if (map_sel > 0) { map_sel--; sfx_play_name("ui_move"); }
        }
        if (btnp(BTN_B)) { state = S_TITLE; state_t = 0; music_play(TT_MUS_TITLE); sfx_play_name("ui_back"); }
        if (btnp(BTN_A)) { sfx_play_name("ui_ok"); input_consume(); enter_level(map_sel); }
        break;
    case S_INTRO:
        frame_t++;
        if (state_t > 100 || (state_t > 20 && btnp(BTN_A))) { input_consume(); start_attempt(); }
        break;
    case S_PLAY: play_update(); break;
    case S_CLEAR:
        frame_t++;
        if (state_t > 90 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            if (level_i == TT_LEVELS - 1 && !sv.seen_end) {
                sv.seen_end = 1;
                save_now();
                state = S_ENDING;
                state_t = 0;
                music_restart(TT_MUS_END);
            } else {
                state = S_MAP;
                state_t = 0;
                map_sel = imin(level_i + 1, TT_LEVELS - 1);
                music_play(TT_MUS_MAP);
            }
        }
        break;
    case S_FAIL:
        frame_t++;
        if (state_t > 60 && btnp(BTN_A)) { input_consume(); enter_level(level_i); }
        if (state_t > 60 && btnp(BTN_B)) { state = S_MAP; state_t = 0; map_sel = level_i; music_play(TT_MUS_MAP); }
        break;
    case S_ENDING:
        frame_t++;
        if (state_t > 420 && (btnp(BTN_A) || btnp(BTN_START))) { state = S_MAP; state_t = 0; map_sel = TT_LEVELS - 1; music_play(TT_MUS_MAP); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */

typedef struct { uint8_t bg, bg2, wall, wall_hi, wall_lo, brk, brk_hi, water, water_hi; } Theme;
static const Theme THEMES[4] = {
    /* the nursery: wallpaper, floorboards, alphabet blocks */
    {C_NIGHT, C_DUSK, C_TAN, C_EARTH, C_BROWN, C_RED, C_ORANGE, C_BLUE, C_SKY},
    /* the bathroom: tiles, the tub, soap */
    {C_NAVY, C_NIGHT, C_GREY, C_LIGHT, C_SLATE, C_PINK, C_WHITE, C_BLUE, C_CYAN},
    /* the kitchen: brick, the stove, sugar lumps */
    {C_MAROON, C_NIGHT, C_WINE, C_RED, C_MAROON, C_CREAM, C_WHITE, C_BLUE, C_SKY},
    /* the toy chest: dark wood and velvet */
    {C_INK, C_PURPLE, C_BROWN, C_TAN, C_INK, C_VIOLET, C_MAGENTA, C_NAVY, C_BLUE},
};

static const Theme *TH(void) { return &THEMES[iclamp(world - 1, 0, 3)]; }

static void draw_background(int cx) {
    const Theme *t = TH();
    gfx_rect(0, PLAY_Y, SCREEN_W, LEVEL_H, t->bg);
    /* parallax pattern: wallpaper stripes, tiles, bricks or velvet folds */
    int off = cx / 3;
    for (int x = -(off % 24); x < SCREEN_W; x += 24) {
        switch (world) {
        case 1:
            /* striped nursery wallpaper with little stars */
            gfx_rect(x, PLAY_Y, 3, LEVEL_H, t->bg2);
            gfx_vline(x + 12, PLAY_Y, PLAY_Y + LEVEL_H - 1, t->bg2);
            for (int y = PLAY_Y + 8; y < PLAY_Y + LEVEL_H; y += 24) {
                int sx = x + 18, sy = y + ((x / 24) & 1) * 12;
                gfx_pset(sx, sy, C_DUSK);
                gfx_pset(sx - 1, sy, C_DUSK);
                gfx_pset(sx + 1, sy, C_DUSK);
                gfx_pset(sx, sy - 1, C_DUSK);
                gfx_pset(sx, sy + 1, C_DUSK);
            }
            break;
        case 2:
            for (int y = PLAY_Y; y < PLAY_Y + LEVEL_H; y += 12) gfx_rectb(x, y, 12, 12, t->bg2);
            for (int y = PLAY_Y; y < PLAY_Y + LEVEL_H; y += 12) gfx_rectb(x + 12, y, 12, 12, t->bg2);
            break;
        case 3:
            for (int y = PLAY_Y; y < PLAY_Y + LEVEL_H; y += 8) {
                gfx_hline(x, x + 23, y, t->bg2);
                gfx_vline(x + ((y / 8) & 1) * 12, y, y + 7, t->bg2);
            }
            break;
        default:
            /* velvet folds */
            gfx_rect(x + 4, PLAY_Y, 2, LEVEL_H, t->bg2);
            gfx_vline(x + 16, PLAY_Y, PLAY_Y + LEVEL_H - 1, C_NIGHT);
            break;
        }
    }
}

static void draw_tile(int tx, int ty, int sx, int sy) {
    const Theme *t = TH();
    char c = map[ty][tx];
    switch (c) {
    case '#': {
        bool top = !solid_char(tile(tx, ty - 1)) && tile(tx, ty - 1) != '#';
        gfx_rect(sx, sy, TS, TS, t->wall);
        if (world == 1) {
            gfx_hline(sx, sx + 9, sy + 4 + (tx & 1), t->wall_lo);
            if ((tx + ty) % 3 == 0) gfx_pset(sx + 6, sy + 2, t->wall_lo);
        } else if (world == 2) {
            gfx_rectb(sx, sy, TS, TS, t->wall_lo);
            gfx_pset(sx + 2, sy + 2, t->wall_hi);
        } else if (world == 3) {
            gfx_hline(sx, sx + 9, sy + 4, t->wall_lo);
            gfx_hline(sx, sx + 9, sy + 9, t->wall_lo);
            gfx_vline(sx + ((ty & 1) ? 2 : 7), sy, sy + 3, t->wall_lo);
            gfx_vline(sx + ((ty & 1) ? 7 : 2), sy + 5, sy + 8, t->wall_lo);
        } else {
            gfx_vline(sx + 3, sy, sy + 9, t->wall_lo);
            gfx_vline(sx + 8, sy, sy + 9, t->wall_lo);
        }
        if (top) {
            gfx_hline(sx, sx + 9, sy, t->wall_hi);
            gfx_hline(sx, sx + 9, sy + 1, t->wall_hi);
        }
        break;
    }
    case 'x':
        gfx_rect(sx, sy, TS, TS, t->brk);
        gfx_rectb(sx, sy, TS, TS, C_INK);
        gfx_hline(sx + 1, sx + 8, sy + 1, t->brk_hi);
        gfx_vline(sx + 1, sy + 1, sy + 8, t->brk_hi);
        if (world == 1) {
            static const char *LET = "ABCTOY";
            char s[2] = {LET[(tx * 7 + ty * 3) % 6], 0};
            tiny_draw(s, sx + 3, sy + 3, C_WHITE);
        } else {
            gfx_pset(sx + 5, sy + 5, t->brk_hi);
            gfx_pset(sx + 3, sy + 6, C_INK);
        }
        break;
    case '^':
        for (int k = 0; k < 3; k++) {
            int bx = sx + k * 3 + 1;
            gfx_vline(bx + 1, sy + 4, sy + 9, C_LIGHT);
            gfx_pset(bx + 1, sy + 3, C_WHITE);
            gfx_vline(bx, sy + 7, sy + 9, C_GREY);
            gfx_vline(bx + 2, sy + 7, sy + 9, C_SLATE);
        }
        break;
    case 'w': {
        bool surf = tile(tx, ty - 1) != 'w' && !solid_char(tile(tx, ty - 1));
        gfx_dither(sx, sy, TS, TS, t->water, 10);
        if (surf) {
            int wv = ((frame_t / 8) + tx) & 3;
            gfx_hline(sx, sx + 9, sy + 1, t->water_hi);
            gfx_pset(sx + wv * 2, sy, t->water_hi);
            gfx_pset(sx + 1 + wv * 2, sy, C_WHITE);
        }
        break;
    }
    case 'f':
        /* a candle and its flame */
        gfx_rect(sx + 3, sy + 5, 4, 5, C_CREAM);
        gfx_vline(sx + 6, sy + 5, sy + 9, C_TAN);
        spr_draw(&tt_spr[(frame_t / 6 + tx) % 2 ? T_FLAME1 : T_FLAME2], sx + 1, sy - 3, 0);
        break;
    case 'P': spr_draw(&tt_spr[T_POT], sx, sy, 0); break;
    case '[': case ']': spr_draw(&tt_spr[weight_on_tile(tx, ty) ? T_SWITCH_DOWN : T_SWITCH_UP], sx, sy + 6, 0); break;
    case '|': case '!': {
        bool open = gate_open[c == '!'];
        if (open) { gfx_dither(sx + 3, sy, 4, TS, c == '!' ? C_CYAN : C_YELLOW, 3); }
        else {
            spr_draw(&tt_spr[T_GATE], sx, sy, 0);
            if (c == '!') gfx_pset(sx + 5, sy + 5, C_CYAN);
        }
        break;
    }
    case 'E': spr_draw(&tt_spr[T_EXIT], sx, sy - 10, 0); break;
    case 'D': spr_draw(&tt_spr[T_DOOR], sx - 1, sy - 6, 0); if (tx == door_x) gfx_pset(sx + 4, sy - 8, (frame_t / 10) % 2 ? C_YELLOW : C_AMBER); break;
    case 'v': {
        gfx_vline(sx + 4 + ((ty & 1) ? 1 : 0), sy, sy + 9, C_JADE);
        gfx_vline(sx + 5 - ((ty & 1) ? 1 : 0), sy, sy + 9, C_FOREST);
        gfx_rect(sx + ((ty & 1) ? 6 : 1), sy + 3, 3, 2, C_LEAF);
        break;
    }
    case 'N': case 'K': case 'Y':
        if (tile(tx - 1, ty) != c && tile(tx, ty - 1) != c) spr_draw(&tt_spr[c == 'N' ? T_HEAD_FLAME : c == 'K' ? T_HEAD_SEED : T_HEAD_BUG], sx, sy, tx * TS > LW * TS / 2 ? SPR_FLIPX : 0);
        break;
    default:
        if (c >= '1' && c <= '9') {
            int bob = ((frame_t / 10) + tx) % 2;
            spr_draw(&tt_spr[T_TAG], sx, sy - bob, 0);
            char s[3] = {'+', c, 0};
            tiny_draw(s, sx + 1, sy + 2 - bob, C_BROWN);
        }
        break;
    }
}

static void draw_level(int cx) {
    int tx0 = cx / TS, tx1 = imin(LW - 1, tx0 + SCREEN_W / TS + 1);
    for (int ty = 0; ty < TT_ROWS; ty++)
        for (int tx = tx0; tx <= tx1; tx++) draw_tile(tx, ty, tx * TS - cx, PLAY_Y + ty * TS);
    /* scale pans and their posts */
    for (int s = 0; s < n_scales; s++)
        for (int side = 0; side < 2; side++) {
            int px = (int)pan_x(s, side) - cx, py = PLAY_Y + (int)pan_y(s, side);
            gfx_vline(px + 15, py + 4, PLAY_Y + LEVEL_H, C_SLATE);
            gfx_rect(px, py, 30, 4, C_AMBER);
            gfx_hline(px, px + 29, py, C_YELLOW);
            gfx_hline(px, px + 29, py + 3, C_BROWN);
        }
}

static void draw_bodies(int cx) {
    for (int i = 0; i < MAX_BODIES; i++) {
        Body *b = &bodies[i];
        if (!b->alive) continue;
        int x = (int)lroundf(b->x) - cx, y = PLAY_Y + (int)lroundf(b->y);
        switch (b->kind) {
        case B_STONE: spr_draw(&tt_spr[T_STONE], x, y, 0); break;
        case B_LODGED: spr_draw(&tt_spr[T_LODGED], x, y - 1, b->x > 0 && rect_tiles(b->x - 1, b->y, 1, 4) ? SPR_FLIPX : 0); break;
        case B_FLOAT: spr_draw(&tt_spr[T_CORPSE], x, y - 2 + ((frame_t / 16 + i) & 1), 0); break;
        default: spr_draw(&tt_spr[T_CORPSE], x, y - 2, 0); break;
        }
    }
}

static void draw_foes(int cx) {
    uint8_t m[PAL_COUNT];
    for (int i = 0; i < MAX_FOES; i++) {
        Foe *f = &foes[i];
        if (!f->alive) continue;
        int x = (int)lroundf(f->x) - cx, y = PLAY_Y + (int)lroundf(f->y);
        if (x < -20 || x > SCREEN_W + 20) continue;
        int fl = f->dir > 0 ? SPR_FLIPX : 0;
        bool alt = (f->t / 8) % 2;
        switch (f->kind) {
        case F_MOUSE: spr_draw(&tt_spr[alt ? T_MOUSE1 : T_MOUSE2], x - 1, y - 2, fl); break;
        case F_DART: spr_draw(&tt_spr[alt ? T_DART1 : T_DART2], x, y - 1, fl); break;
        case F_RAM:
            if (f->state == 1 && (f->t / 2) % 2) x += 1;
            spr_draw(&tt_spr[f->state == 2 ? T_RAM_CHARGE : alt ? T_RAM1 : T_RAM2], x - 1, y - 1, fl);
            break;
        case F_FISH: spr_draw(&tt_spr[alt ? T_FISH1 : T_FISH2], x, y - 1, fl); break;
        case F_BUG:
            if (f->seeded) {
                pal_identity(m);
                m[C_GREY] = C_LEAF;
                m[C_SLATE] = C_JADE;
                spr_draw_ex(&tt_spr[alt ? T_BUG1 : T_BUG2], x - 1, y - 1, fl, m, -1);
            } else {
                spr_draw(&tt_spr[alt ? T_BUG1 : T_BUG2], x - 1, y - 1, fl);
            }
            break;
        case F_DRAGON: spr_draw(&tt_spr[(f->t % 150) > 90 && (f->t % 150) < 110 ? T_DRAGON2 : T_DRAGON1], x - 2, y, fl); break;
        }
    }
}

static void draw_shots(int cx) {
    for (int i = 0; i < MAX_SHOTS; i++) {
        Shot *s = &shots[i];
        if (!s->alive) continue;
        int x = (int)s->x - cx, y = PLAY_Y + (int)s->y;
        if (s->kind == SH_SEED) spr_draw(&tt_spr[T_SEED], x - 2, y - 2, 0);
        else if (s->kind == SH_FIRE) spr_draw(&tt_spr[(s->t / 4) % 2 ? T_FLAME1 : T_FLAME2], x - 4, y - 5, s->vx < 0 ? SPR_FLIPX : 0);
        else spr_draw(&tt_spr[T_ORB], x - 3, y - 3, 0);
    }
}

static void draw_soldier(int cx) {
    if (P.mode == M_GONE) return;
    int x = (int)lroundf(P.x) - cx, y = PLAY_Y + (int)lroundf(P.y);
    int fl = P.dir < 0 ? SPR_FLIPX : 0;
    uint8_t m[PAL_COUNT];
    pal_identity(m);
    const uint8_t *remap = NULL;
    if (P.burn > 0 && (frame_t / 3) % 2) { m[C_BLUE] = C_ORANGE; m[C_NAVY] = C_RED; m[C_RED] = C_YELLOW; remap = m; }
    if (P.seed > 0 && (frame_t / 4) % 2) { m[C_BLUE] = C_LEAF; m[C_NAVY] = C_JADE; m[C_LIGHT] = C_LIME; remap = m; }
    int spr;
    int ox = -1, oy = -1;
    switch (P.mode) {
    case M_CHUTE:
        spr_draw(&tt_spr[T_CHUTE], x - 4, y - 13 + ((frame_t / 12) & 1), 0);
        spr = T_SOLDIER_CHUTE;
        break;
    case M_ENTER: spr = T_SOLDIER1; break;
    case M_SWIM: spr = T_SOLDIER_SWIM; break;
    case M_CLIMB: spr = T_SOLDIER_CLIMB; fl = (P.anim / 10) % 2 ? SPR_FLIPX : 0; break;
    case M_ARROW: spr = T_ARROW; ox = 0; oy = -3; fl = P.dir < 0 ? SPR_FLIPX : 0; break;
    default:
        if (!P.ground) spr = T_SOLDIER_JUMP;
        else if (fabsf(P.vx) > 0.1f) spr = (P.anim / 6) % 2 ? T_SOLDIER2 : T_SOLDIER3;
        else spr = T_SOLDIER1;
        break;
    }
    if (remap) spr_draw_ex(&tt_spr[spr], x + ox, y + oy, fl, remap, -1);
    else spr_draw(&tt_spr[spr], x + ox, y + oy, fl);
    if (P.burn > 0) {
        spr_draw(&tt_spr[(frame_t / 4) % 2 ? T_FLAME1 : T_FLAME2], x - 1 + (P.mode == M_ARROW ? 2 : 0), y - 9, 0);
    }
    if (P.mode == M_SWIM && P.breath < BREATH) {
        gfx_rect(x - 2, y - 5, 10, 2, C_INK);
        gfx_rect(x - 2, y - 5, 10 * P.breath / BREATH, 2, P.breath < 80 ? C_RED : C_CYAN);
    }
    if (P.burn > 0 && P.mode != M_ARROW) {
        gfx_rect(x - 2, y - 12, 10, 2, C_INK);
        gfx_rect(x - 2, y - 12, 10 * (BURN - P.burn) / BURN, 2, C_ORANGE);
    }
    if (P.seed > 0) {
        gfx_rect(x - 2, y - 5, 10, 2, C_INK);
        gfx_rect(x - 2, y - 5, 10 * (SEEDED - P.seed) / SEEDED, 2, C_LEAF);
    }
}

static void draw_ship(int cx) {
    if (world == 4) return;
    int x = (int)ship_x - cx, y = PLAY_Y - 6 + (int)(sinf((float)frame_t * 0.05f) * 2);
    spr_draw(&tt_spr[T_BLIMP], x, y, 0);
    int prop = (frame_t / 3) % 3;
    gfx_vline(x - 1, y + 6 + prop, y + 8 + prop, C_LIGHT);
}

static void draw_boss(int cx) {
    if (!boss.on) return;
    int x = boss.bx - cx, y = PLAY_Y + boss.by;
    if (boss.dead && boss.death_t > 100) return;
    uint8_t m[PAL_COUNT];
    pal_identity(m);
    bool fl = boss.flash > 0 && (frame_t / 2) % 2;
    if (fl) for (int k = 0; k < PAL_COUNT; k++) m[k] = C_WHITE;
    /* the Jack springs out of the chest, mouth wide, then ducks back in */
    int sway = boss.mouth ? (int)(sinf((float)boss.t * 0.08f) * 2) : 0;
    int hy = y - (boss.mouth ? 24 : 8);
    if (boss.mouth)
        for (int k = 0; k < 5; k++) gfx_hline(x + 15 + sway * k / 5, x + 24 + sway * k / 5, y - 1 - k * 2, k & 1 ? C_GREY : C_LIGHT);
    spr_draw_ex(&tt_spr[T_JACK_HEAD], x + 10 + sway, hy, 0, m, -1);
    if (boss.mouth) spr_draw_ex(&tt_spr[T_JACK_MOUTH], x + 16 + sway, hy + 8, 0, m, -1);
    spr_draw_ex(&tt_spr[T_JACK_BOX], x, y, 0, m, -1);
    if (!boss.dead) {
        for (int k = 0; k < 3; k++)
            if (k < boss.hp) gfx_rect(x + 8 + k * 9, y + 44, 6, 3, C_RED);
    }
}

static void draw_parts(int cx) {
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        int x = (int)p->x - cx, y = PLAY_Y + (int)p->y;
        if (p->kind == 1) {
            int r = p->life / 3;
            gfx_circ(x, y, r, p->life > 8 ? C_YELLOW : C_ORANGE);
        } else if (p->kind == 3) {
            char buf[8];
            snprintf(buf, sizeof buf, "+%d", p->val);
            text_outline(buf, x - text_width(buf) / 2, y, (p->life / 4) % 2 ? C_YELLOW : C_WHITE, C_INK);
        } else {
            gfx_rect(x, y, 2, 2, p->col);
        }
    }
}

static void draw_hud(void) {
    gfx_rect(0, 0, SCREEN_W, PLAY_Y, C_INK);
    gfx_hline(0, SCREEN_W - 1, PLAY_Y - 1, C_DUSK);
    const TTLevel *L = &TT_LEVEL[level_i];
    text_draw(L->name, 4, 6, C_YELLOW);
    tiny_draw(L->title, 30, 4, C_GREY);
    char buf[32];
    spr_draw(&tt_spr[T_SOLDIER1], 196, 5, 0);
    snprintf(buf, sizeof buf, "x%d", lives);
    text_draw(buf, 206, 6, lives <= 3 ? C_RED : C_WHITE);
    int d = lives - lives_start;
    snprintf(buf, sizeof buf, "%+d", d);
    text_draw(buf, 240, 6, d > 0 ? C_LIME : d < 0 ? C_PINK : C_GREY);
    /* three pips: every third foe brings a soldier */
    for (int k = 0; k < 3; k++) {
        gfx_rectb(270 + k * 7, 7, 5, 5, C_DUSK);
        if (k < kills % 3) gfx_rect(271 + k * 7, 8, 3, 3, C_ORANGE);
    }
    if (cleared(level_i)) {
        snprintf(buf, sizeof buf, "BEST %+d", sv.best[level_i]);
        tiny_draw(buf, 30, 12, C_SLATE);
    }
}

static void draw_play(void) {
    int sx = shake > 0 ? (frame_t % 3) - 1 : 0, sy = shake > 0 ? ((frame_t / 2) % 3) - 1 : 0;
    int cx = (int)lroundf(cam_x);
    gfx_cls(C_INK);
    gfx_clip(0, PLAY_Y, SCREEN_W, LEVEL_H);
    gfx_camera(sx, sy);
    draw_background(cx);
    draw_level(cx);
    draw_bodies(cx);
    draw_boss(cx);
    draw_foes(cx);
    draw_ship(cx);
    draw_soldier(cx);
    draw_shots(cx);
    draw_parts(cx);
    gfx_camera(0, 0);
    gfx_noclip();
    draw_hud();
    if (P.mode == M_CHUTE || P.mode == M_ENTER) {
        if (P.giveup > 0) {
            gfx_rect(110, 168, 100, 8, C_INK);
            gfx_rect(111, 169, 98 * P.giveup / GIVEUP, 6, C_RED);
            tiny_center("GIVING UP...", 160, 170, C_WHITE);
        } else if (P.mode == M_CHUTE || lives == lives_start) {
            gfx_rect(116, 170, 88, 9, C_INK);
            tiny_center("HOLD B TO GIVE UP", 160, 172, C_GREY);
        }
    }
}

static void draw_title(void) {
    gfx_cls(C_NIGHT);
    for (int y = 0; y < 180; y += 4) gfx_dither(0, y, 320, 2, C_DUSK, 3);
    /* the blimp and a stream of parachutes */
    int bx = 200 + (int)(sinf(frame_t * 0.02f) * 6);
    spr_draw_scaled(&tt_spr[T_BLIMP], bx, 60, 2, 0);
    for (int i = 0; i < 4; i++) {
        int t = (frame_t + i * 60) % 240;
        int x = bx + 30 - i * 3 + (int)(sinf((t + i * 40) * 0.05f) * 8), y = 90 + t / 3;
        if (y > 150) continue;
        spr_draw(&tt_spr[T_CHUTE], x - 4, y - 13, 0);
        spr_draw(&tt_spr[T_SOLDIER_CHUTE], x, y, 0);
    }
    /* the floor and a row of soldiers */
    gfx_rect(0, 150, 320, 30, C_TAN);
    gfx_hline(0, 319, 150, C_EARTH);
    for (int x = 0; x < 320; x += 20) gfx_vline(x + (x / 20 % 2) * 7, 151, 179, C_BROWN);
    for (int i = 0; i < 6; i++) spr_draw(&tt_spr[(frame_t / 20 + i) % 3 == 0 ? T_SOLDIER2 : T_SOLDIER1], 30 + i * 12, 140, 0);
    spr_draw_scaled(&tt_spr[T_PIP_BIG], 24, 56, 2, 0);
    static const uint8_t grad[] = {C_WHITE, C_LIGHT, C_GREY, C_SKY};
    ui_fancy_center("TIN TROOP", 160, 14, 3, grad, 4, C_NAVY, C_INK);
    text_center("SPEND YOUR SOLDIERS WISELY", 160, 44, C_YELLOW);
    if ((state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 120, C_WHITE);
    int total = 0, n = 0;
    for (int i = 0; i < TT_LEVELS; i++) if (cleared(i)) { n++; total = start_lives(i) + sv.best[i]; }
    if (n) {
        char buf[40];
        snprintf(buf, sizeof buf, "%d/10 CLEARED  TROOP %d", n, total);
        tiny_center(buf, 160, 132, C_LIGHT);
    }
    text_center(GLYPH_A " START   " GLYPH_B " LIBRARY", 160, 170, C_LIGHT);
}

/* the house in cross-section: one stop per level */
static void map_pos(int i, int *x, int *y) {
    /* up through the house, one floor per world, winding like a stair */
    static const int X[TT_LEVELS] = {110, 185, 260, 260, 185, 110, 110, 185, 185, 260};
    static const int Y[TT_LEVELS] = {142, 142, 142, 106, 106, 106, 70, 70, 40, 40};
    *x = X[i];
    *y = Y[i];
}

static void draw_map(void) {
    gfx_cls(C_NIGHT);
    /* the house */
    gfx_rect(16, 30, 288, 130, C_INK);
    gfx_rectb(16, 30, 288, 130, C_DUSK);
    static const char *ROOM[4] = {"NURSERY", "BATHROOM", "KITCHEN", "TOY CHEST"};
    static const uint8_t RC[4] = {C_TAN, C_CYAN, C_RED, C_VIOLET};
    static const int FLOOR_Y[4] = {142, 106, 70, 40};
    for (int w = 0; w < 4; w++) {
        int y = FLOOR_Y[w];
        if (w < 3) gfx_hline(18, 301, y + (w == 2 ? 14 : 16), C_DUSK);
        tiny_draw(ROOM[w], 24, y - 2, RC[w]);
    }
    for (int i = 0; i + 1 < TT_LEVELS; i++) {
        int x0, y0, x1, y1;
        map_pos(i, &x0, &y0);
        map_pos(i + 1, &x1, &y1);
        gfx_line(x0, y0, x1, y1, unlocked(i + 1) ? C_GREY : C_DUSK);
    }
    for (int i = 0; i < TT_LEVELS; i++) {
        int x, y;
        map_pos(i, &x, &y);
        bool on = unlocked(i);
        gfx_circ(x, y, 7, on ? (cleared(i) ? C_JADE : C_AMBER) : C_DUSK);
        gfx_circb(x, y, 7, C_INK);
        tiny_center(TT_LEVEL[i].name, x, y - 2, on ? C_INK : C_NIGHT);
        if (cleared(i)) {
            char buf[8];
            snprintf(buf, sizeof buf, "%+d", sv.best[i]);
            tiny_center(buf, x, y + 10, sv.best[i] >= 0 ? C_LIME : C_PINK);
        }
    }
    int sx, sy;
    map_pos(map_sel, &sx, &sy);
    spr_draw(&tt_spr[T_CHUTE], sx - 6, sy - 30 + ((frame_t / 12) & 1), 0);
    spr_draw(&tt_spr[T_SOLDIER_CHUTE], sx - 2, sy - 17 + ((frame_t / 12) & 1), 0);
    gfx_rect(0, 0, 320, 26, C_INK);
    const TTLevel *L = &TT_LEVEL[map_sel];
    char buf[64];
    snprintf(buf, sizeof buf, "%s  %s", L->name, L->title);
    text_draw(buf, 8, 4, C_WHITE);
    snprintf(buf, sizeof buf, "SOLDIERS %d", start_lives(map_sel));
    tiny_draw(buf, 8, 15, C_YELLOW);
    if (cleared(map_sel)) {
        snprintf(buf, sizeof buf, "BEST %+d", sv.best[map_sel]);
        tiny_draw(buf, 90, 15, C_LIME);
    }
    gfx_rect(0, 166, 320, 14, C_INK);
    text_center(GLYPH_A " DEPLOY   " GLYPH_B " BACK", 160, 169, C_LIGHT);
}

static void draw_intro(void) {
    gfx_cls(C_INK);
    const TTLevel *L = &TT_LEVEL[level_i];
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center(L->name, 160, 44, 3, grad, 3, C_MAROON, C_NIGHT);
    text_center(L->title, 160, 80, C_WHITE);
    char buf[40];
    snprintf(buf, sizeof buf, "x%d", start_lives(level_i));
    spr_draw_scaled(&tt_spr[T_SOLDIER1], 136, 100, 2, 0);
    text_draw(buf, 156, 106, C_WHITE);
    if (cleared(level_i)) {
        snprintf(buf, sizeof buf, "BEST %+d", sv.best[level_i]);
        tiny_center(buf, 160, 128, C_LIME);
    }
    if (world == 4) tiny_center("NO SHIP HERE: SOLDIERS MARCH FROM THE LAST DOOR", 160, 146, C_VIOLET);
}

static void draw_clear(void) {
    draw_play();
    gfx_darken_rect(0, PLAY_Y, SCREEN_W, LEVEL_H, 2);
    ui_panel(80, 52, 160, 84, C_INK, C_JADE);
    static const uint8_t grad[] = {C_LIME, C_LEAF, C_JADE};
    ui_fancy_center(boss.on ? "VICTORY!" : "CLEAR!", 160, 58, 2, grad, 3, C_FOREST, C_NIGHT);
    char buf[40];
    int d = lives - lives_start;
    snprintf(buf, sizeof buf, "SOLDIERS %d  (%+d)", lives, d);
    text_center(buf, 160, 84, C_WHITE);
    snprintf(buf, sizeof buf, "BEST %+d", sv.best[level_i]);
    text_center(buf, 160, 98, d >= sv.best[level_i] ? C_YELLOW : C_GREY);
    if (state_t > 90 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 118, C_LIGHT);
}

static void draw_fail(void) {
    if (fail_reason == 1) gfx_cls(C_INK);
    else { draw_play(); gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2); }
    ui_panel(70, 56, 180, 70, C_INK, C_RED);
    text_center(fail_reason == 1 ? "THE TROOP TURNS BACK" : "OUT OF SOLDIERS", 160, 66, C_RED);
    text_center(GLYPH_A " TRY AGAIN", 160, 88, C_WHITE);
    text_center(GLYPH_B " TO THE MAP", 160, 102, C_GREY);
}

static void draw_ending(void) {
    int t = state_t;
    gfx_cls(C_NIGHT);
    for (int i = 0; i < 40; i++) gfx_pset((i * 71) % 320, (i * 37) % 100, (i + t / 20) % 5 ? C_DUSK : C_LIGHT);
    gfx_circ(260, 30, 12, C_CREAM);
    gfx_circ(265, 27, 12, C_NIGHT);
    gfx_rect(0, 140, 320, 40, C_TAN);
    gfx_hline(0, 319, 140, C_EARTH);
    /* the troop marches home under the blimp */
    spr_draw_scaled(&tt_spr[T_BLIMP], 120 + (t / 4) % 400 - 200, 40, 2, 0);
    for (int i = 0; i < 10; i++) {
        int x = (t / 2 + i * 22) % 360 - 30;
        spr_draw(&tt_spr[(t / 8 + i) % 2 ? T_SOLDIER2 : T_SOLDIER3], x, 130, 0);
    }
    spr_draw(&tt_spr[T_JACK_BOX], 250, 90, 0);
    ui_panel(40, 70, 240, 44, C_INK, C_YELLOW);
    if (t > 30) text_center("THE JACK IS BACK IN HIS BOX.", 160, 76, C_WHITE);
    if (t > 120) text_center("THE HOUSE SLEEPS SAFE TONIGHT.", 160, 88, C_LIGHT);
    if (t > 210) {
        char buf[48];
        snprintf(buf, sizeof buf, "THE TROOP: %d SOLDIERS", start_lives(TT_LEVELS - 1) + sv.best[TT_LEVELS - 1]);
        text_center(buf, 160, 100, C_YELLOW);
    }
    if (t > 420 && (t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 160, C_INK);
}

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    int x = 2, y = 2, rowh = 0;
    for (int i = 0; i < T_SPRITE_COUNT; i++) {
        Sprite *s = &tt_spr[i];
        if (!s->px) continue;
        if (x + s->w * 2 > 318) { x = 2; y += rowh + 2; rowh = 0; }
        spr_draw_scaled(s, x, y, 2, 0);
        x += s->w * 2 + 3;
        if (s->h * 2 > rowh) rowh = s->h * 2;
    }
}

/* a debugging view: five whole levels per page, two pixels a tile */
static void draw_overview(void) {
    gfx_cls(C_INK);
    for (int k = 0; k < 5; k++) {
        int li = overview_page * 5 + k;
        if (li >= TT_LEVELS) break;
        const TTLevel *L = &TT_LEVEL[li];
        int oy = 2 + k * 36;
        int ox = (int)strlen(L->rows[0]) * 2 + 12 > SCREEN_W ? 0 : 12;
        tiny_draw(L->name, 0, oy + 12, C_YELLOW);
        for (int y = 0; y < TT_ROWS; y++)
            for (int x = 0; L->rows[y][x]; x++) {
                char c = L->rows[y][x];
                int col = C_NIGHT;
                switch (c) {
                case '#': col = C_TAN; break;
                case 'x': col = C_RED; break;
                case '^': col = C_WHITE; break;
                case 'w': col = C_BLUE; break;
                case 'f': col = C_ORANGE; break;
                case 'P': col = C_LEAF; break;
                case '[': case ']': col = C_YELLOW; break;
                case '|': case '!': col = C_GREY; break;
                case 'L': case 'R': col = C_AMBER; break;
                case 'E': case 'D': case 'S': col = C_LIME; break;
                case 'J': case 'N': case 'K': case 'Y': col = C_MAGENTA; break;
                case ' ': break;
                default: col = (c >= '1' && c <= '9') ? C_CREAM : C_PINK; break;
                }
                gfx_rect(ox + x * 2, oy + y * 2, 2, 2, col);
            }
    }
}

static void tt_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    if (overview_page >= 0) { draw_overview(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_MAP: draw_map(); break;
    case S_INTRO: draw_intro(); break;
    case S_PLAY: draw_play(); break;
    case S_CLEAR: draw_clear(); break;
    case S_FAIL: draw_fail(); break;
    case S_ENDING: draw_ending(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void tt_load(void) {
    tt_art_load();
    tt_audio_load();
}

static void tt_start(void) {
    load_save();
    state = S_TITLE;
    state_t = 0;
    title_sel = 0;
    sheet_mode = false;
    freeze_foes = false;
    level_i = 0;
    world = 1;
    game_set_pausable(false);
    music_play(TT_MUS_TITLE);
}

static void tt_quit(void) { save_now(); }

static void tt_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_NIGHT);
    for (int yy = 0; yy < h; yy += 4) gfx_dither(x, y + yy, w, 2, C_DUSK, 3);
    gfx_rect(x, y + 44, w, h - 44, C_TAN);
    gfx_hline(x, x + w - 1, y + 44, C_EARTH);
    spr_draw(&tt_spr[T_BLIMP], x + 70 + (int)(sinf(t * 0.05f) * 3), y + 4, 0);
    int k = (t / 2) % 40;
    spr_draw(&tt_spr[T_CHUTE], x + 88, y + 12 + k / 2, 0);
    spr_draw(&tt_spr[T_SOLDIER_CHUTE], x + 92, y + 25 + k / 2, 0);
    gfx_rect(x + 20, y + 24, 10, 20, C_BROWN);
    spr_draw(&tt_spr[T_LODGED], x + 30, y + 30, 0);
    spr_draw(&tt_spr[T_STONE], x + 40, y + 34, 0);
    spr_draw(&tt_spr[(t / 8) % 2 ? T_SOLDIER2 : T_SOLDIER3], x + 44, y + 24, 0);
    spr_draw(&tt_spr[T_MOUSE1], x + 116, y + 38, 0);
}

static int tt_query(const char *key, int *out) {
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "level")) { *out = level_i; return 1; }
    if (!strcmp(key, "lives")) { *out = lives; return 1; }
    if (!strcmp(key, "lives_start")) { *out = lives_start; return 1; }
    if (!strcmp(key, "kills")) { *out = kills; return 1; }
    if (!strcmp(key, "mode")) { *out = P.mode; return 1; }
    if (!strcmp(key, "px")) { *out = (int)lroundf(P.x); return 1; }
    if (!strcmp(key, "py")) { *out = (int)lroundf(P.y); return 1; }
    if (!strcmp(key, "ground")) { *out = P.ground; return 1; }
    if (!strcmp(key, "burn")) { *out = P.burn; return 1; }
    if (!strcmp(key, "seed")) { *out = P.seed; return 1; }
    if (!strcmp(key, "breath")) { *out = P.breath; return 1; }
    if (!strcmp(key, "ship_x")) { *out = (int)ship_x; return 1; }
    if (!strcmp(key, "cam_x")) { *out = (int)cam_x; return 1; }
    if (!strcmp(key, "gate0")) { *out = gate_open[0]; return 1; }
    if (!strcmp(key, "gate1")) { *out = gate_open[1]; return 1; }
    if (!strcmp(key, "pan_off")) { *out = n_scales ? (int)lroundf(scales[0].off) : 0; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = boss.hp; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = boss.dead; return 1; }
    if (!strcmp(key, "mouth")) { *out = boss.mouth; return 1; }
    if (!strcmp(key, "cleared")) { *out = sv.cleared; return 1; }
    if (!strcmp(key, "door_x")) { *out = door_x; return 1; }
    if (!strcmp(key, "n_shots")) { int n = 0; for (int i = 0; i < MAX_SHOTS; i++) n += shots[i].alive; *out = n; return 1; }
    if (!strcmp(key, "seeded")) { int n = 0; for (int i = 0; i < MAX_FOES; i++) n += foes[i].alive && foes[i].seeded > 0; *out = n; return 1; }
    if (!strcmp(key, "shot0_x") || !strcmp(key, "shot0_y") || !strcmp(key, "shot0_vx")) {
        *out = -999;
        for (int i = 0; i < MAX_SHOTS; i++)
            if (shots[i].alive) { *out = key[6] == 'x' ? (int)shots[i].x : key[6] == 'y' ? (int)shots[i].y : (int)(shots[i].vx * 10); break; }
        return 1;
    }
    if (!strcmp(key, "foe0_seeded")) { *out = -1; for (int i = 0; i < MAX_FOES; i++) if (foes[i].alive) { *out = foes[i].seeded; break; } return 1; }
    if (!strcmp(key, "foe0_x")) { *out = -1; for (int i = 0; i < MAX_FOES; i++) if (foes[i].alive) { *out = (int)foes[i].x; break; } return 1; }
    if (!strcmp(key, "width")) { *out = LW; return 1; }
    if (!strncmp(key, "best_", 5)) { *out = sv.best[iclamp(atoi(key + 5), 0, TT_LEVELS - 1)]; return 1; }
    if (!strncmp(key, "start_", 6)) { *out = start_lives(iclamp(atoi(key + 6), 0, TT_LEVELS - 1)); return 1; }
    if (!strncmp(key, "count_", 6)) {
        const char *n = key + 6;
        int c = 0;
        if (!strcmp(n, "stone") || !strcmp(n, "lodged") || !strcmp(n, "corpse") || !strcmp(n, "float")) {
            int k = !strcmp(n, "stone") ? B_STONE : !strcmp(n, "lodged") ? B_LODGED : !strcmp(n, "corpse") ? B_CORPSE : B_FLOAT;
            for (int i = 0; i < MAX_BODIES; i++) c += bodies[i].alive && bodies[i].kind == k;
        } else if (!strcmp(n, "foes")) {
            for (int i = 0; i < MAX_FOES; i++) c += foes[i].alive;
        } else if (!strcmp(n, "vine")) {
            for (int y = 0; y < TT_ROWS; y++) for (int x = 0; x < LW; x++) c += map[y][x] == 'v';
        } else if (!strcmp(n, "breakable")) {
            for (int y = 0; y < TT_ROWS; y++) for (int x = 0; x < LW; x++) c += map[y][x] == 'x';
        } else if (!strcmp(n, "tags")) {
            for (int y = 0; y < TT_ROWS; y++) for (int x = 0; x < LW; x++) c += map[y][x] >= '1' && map[y][x] <= '9';
        } else return 0;
        *out = c;
        return 1;
    }
    if (!strncmp(key, "tile_", 5)) {
        int x = atoi(key + 5);
        const char *u = strchr(key + 5, '_');
        if (!u) return 0;
        *out = (unsigned char)tile(x, atoi(u + 1));
        return 1;
    }
    if (!strncmp(key, "body_", 5)) {
        /* body_N_x / body_N_y / body_N_kind for the N-th live body */
        int n = atoi(key + 5), k = 0;
        const char *u = strchr(key + 5, '_');
        if (!u) return 0;
        *out = -1;
        for (int i = 0; i < MAX_BODIES; i++)
            if (bodies[i].alive && k++ == n) {
                *out = u[1] == 'x' ? (int)lroundf(bodies[i].x) : u[1] == 'y' ? (int)lroundf(bodies[i].y) : bodies[i].kind;
                break;
            }
        return 1;
    }
    if (!strncmp(key, "stone_y", 7)) {
        *out = -1;
        for (int i = MAX_BODIES - 1; i >= 0; i--) if (bodies[i].alive && bodies[i].kind == B_STONE) { *out = (int)lroundf(bodies[i].y); break; }
        return 1;
    }
    if (!strcmp(key, "level_errors")) {
        /* every level: rows the same width, walls at both ends, one exit (or a boss) */
        int bad = 0;
        for (int i = 0; i < TT_LEVELS; i++) {
            const TTLevel *L = &TT_LEVEL[i];
            int w = (int)strlen(L->rows[0]), exits = 0, boss_n = 0, doors = 0, starts = 0;
            if (w > TT_MAXW) bad++;
            for (int y = 0; y < TT_ROWS; y++) {
                if ((int)strlen(L->rows[y]) != w) { bad++; fprintf(stderr, "level %s row %d width %d != %d\n", L->name, y, (int)strlen(L->rows[y]), w); }
                for (const char *c = L->rows[y]; *c; c++) { exits += *c == 'E'; boss_n += *c == 'J'; doors += *c == 'D'; starts += *c == 'S'; }
            }
            if (exits + boss_n != 1) { bad++; fprintf(stderr, "level %s: %d exits, %d bosses\n", L->name, exits, boss_n); }
            if (L->world == 4 && doors == 0) { bad++; fprintf(stderr, "level %s: no door\n", L->name); }
            if (L->world != 4 && starts != 1) { bad++; fprintf(stderr, "level %s: %d starts\n", L->name, starts); }
        }
        *out = bad;
        return 1;
    }
    return 0;
}

static int tt_cheat(const char *cmd) {
    int a, b, c;
    char name[16];
    if (sscanf(cmd, "level %d", &a) == 1) { level_i = iclamp(a, 0, TT_LEVELS - 1); start_attempt(); return 1; }
    if (!strcmp(cmd, "unlock")) { sv.cleared = (uint16_t)((1u << (TT_LEVELS - 1)) - 1); return 1; }
    if (sscanf(cmd, "best %d %d", &a, &b) == 2) { sv.best[a] = (int16_t)b; sv.cleared |= (uint16_t)(1u << a); return 1; }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) {
        P.x = (float)a; P.y = (float)b; P.vx = P.vy = 0; P.mode = M_WALK; P.ground = false;
        progress_x = fmaxf(progress_x, P.x);
        cam_x = fclamp(P.x - 150, 0, (float)(LW * TS - SCREEN_W));
        return 1;
    }
    if (sscanf(cmd, "lives %d", &a) == 1) { lives = a; return 1; }
    if (!strcmp(cmd, "burn")) { P.burn = BURN; return 1; }
    if (!strcmp(cmd, "seed")) { P.seed = SEEDED; return 1; }
    if (!strcmp(cmd, "seed_bug")) {
        for (int i = 0; i < MAX_FOES; i++)
            if (foes[i].alive && foes[i].kind == F_BUG) { foes[i].seeded = SEEDED; return 1; }
        return 0;
    }
    if (!strcmp(cmd, "no_foes")) { memset(foes, 0, sizeof foes); return 1; }
    if (!strcmp(cmd, "freeze")) { freeze_foes = !freeze_foes; return 1; }
    if (sscanf(cmd, "put %c %d %d", &name[0], &a, &b) == 3) { if (a >= 0 && a < LW && b >= 0 && b < TT_ROWS) map[b][a] = name[0] == '_' ? ' ' : name[0]; return 1; }
    if (sscanf(cmd, "foe %15s %d %d %d", name, &a, &b, &c) == 4 || (c = -1, sscanf(cmd, "foe %15s %d %d", name, &a, &b) == 3)) {
        static const char *N[6] = {"mouse", "dart", "ram", "fish", "bug", "dragon"};
        for (int k = 0; k < 6; k++)
            if (!strcmp(name, N[k])) {
                float w, h;
                foe_size(k, &w, &h);
                int i = add_foe(k, (float)a, (float)(b - h));
                if (i >= 0) foes[i].dir = (int8_t)(c > 0 ? 1 : -1);
                return 1;
            }
        return 0;
    }
    if (!strcmp(cmd, "mouth")) { boss.t = 180; boss.mouth = true; return 1; }
    if (sscanf(cmd, "boss_hp %d", &a) == 1) { boss.hp = a; return 1; }
    if (!strcmp(cmd, "win")) { state = S_CLEAR; state_t = 0; clear_level(); return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    if (sscanf(cmd, "overview %d", &a) == 1) { overview_page = a; return 1; }
    if (sscanf(cmd, "respawn %d", &a) == 1) { P.mode = M_GONE; respawn_t = a; return 1; }
    return 0;
}

const GameDef GAME_TINTROOP = {
    "tintroop",
    "TIN TROOP",
    "1984",
    "PUZZLE PLATFORMER",
    "EVERY SOLDIER IS A STEP, A BRIDGE OR A BOMB. SPEND THEM WISELY.",
    {"CLEAR 2-C", "DEFEAT THE JACK", "FINISH WITH 75 SOLDIERS"},
    "D-PAD\tRUN, STEER\n"
    GLYPH_A "\tJUMP / SWIM\n"
    GLYPH_B "\tTHE LANCE\n"
    GLYPH_B "+" GLYPH_UP "\tTHE POP\n"
    GLYPH_B "+" GLYPH_DOWN "\tTHE LEAD\n"
    "START\tPAUSE\n\n"
    "HOLD " GLYPH_B " IN THE AIR TO GIVE UP.",
    C_SKY, C_YELLOW,
    tt_load, tt_start, tt_update, tt_draw, tt_quit, tt_label, tt_query, tt_cheat,
    "MORTOL", 6,
};
