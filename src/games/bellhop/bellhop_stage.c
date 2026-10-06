/* BELLHOP - the rules of one stage.
 *
 * Everything a stage holds lives in one BhpStage, so the demo pilot can copy
 * it and fly the copy forward on these same rules. Nothing here draws or
 * plays a sound: a step leaves its news in s->ev, s->fx and s->pts for the
 * run (bellhop.c) to show, hear and count.
 *
 * The ship flies on CHIME CIRCUIT's flight model with its own numbers. It
 * thrusts only while there is fuel, and any touch of a wall, a floor, a
 * ceiling, an enemy or a shot wrecks it at once. The slash strikes to the
 * side the ship last steered toward, breaks blocks, glass, bubbles and
 * enemies, and slows the fall while it lasts. */
#include "bellhop.h"

/* Gravity 0.07 px a frame each frame and thrust 0.17 against it: thrust
 * held about four frames in ten holds the ship still. Steering is gentle
 * and the drift long, so a straight line is steer, tap and let go. */
const ChmFlightTune BHP_TUNE = {
    18,   /* gravity */
    44,   /* thrust */
    12,   /* accel_x */
    14,   /* accel_thrust_x */
    2,    /* drag_x */
    384,  /* max_vx: 1.5 px a frame */
    448,  /* max_up: 1.75 */
    608,  /* max_down: 2.38 */
    14,   /* over_decay */
    1792, /* top_speed */
    0,    /* bounce: never (a touch is a crash) */
    0,    /* min_bounce */
    248,  /* slash_drag: a slash takes a thirty-second off the fall each frame ... */
    4,    /* half: an 8 x 8 hit box */
};

int bhp_kind(int stage) {
    int k = stage % BHP_PER_WORLD;
    return k == 4 ? BHK_BONUS : k == 9 ? BHK_BOSS : BHK_STAGE;
}

int bhp_cup_index(int stage) {
    if (stage < 0 || stage >= BHP_STAGES || bhp_kind(stage) != BHK_STAGE) return -1;
    int k = stage % BHP_PER_WORLD;
    return (stage / BHP_PER_WORLD) * 8 + (k < 4 ? k : k - 1);
}

const char *bhp_stage_label(int stage) {
    static char b[4][8];
    static int n;
    char *o = b[n++ & 3];
    stage = iclamp(stage, 0, BHP_STAGES - 1);
    snprintf(o, 8, "%c-%d", 'A' + stage / BHP_PER_WORLD, stage % BHP_PER_WORLD + 1);
    return o;
}

bool bhp_tile_solid(int t) { return t != BTL_AIR && t != BTL_GATE1_OPEN && t != BTL_GATE2_OPEN; }

int bhp_tile_at(const BhpStage *s, int px, int py) {
    if (px < 0 || px >= BHP_TW * BHP_T || py < BHP_OY || py >= BHP_OY + BHP_TH * BHP_T) return BTL_WALL;
    return s->tile[(py - BHP_OY) / BHP_T][px / BHP_T];
}

bool bhp_solid(const void *ctx, int px, int py) { return bhp_tile_solid(bhp_tile_at((const BhpStage *)ctx, px, py)); }

void bhp_set_tile(BhpStage *s, int c, int r, int t) {
    if (c < 0 || r < 0 || c >= BHP_TW || r >= BHP_TH || s->tile[r][c] == t) return;
    s->tile[r][c] = (uint8_t)t;
    s->ver++;
}

/* ---- small maths ---------------------------------------------------------- */

static const int8_t SIN64[65] = {
    0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49, 51, 54, 57, 60, 63, 65, 68, 71, 73,
    76, 78, 81, 83, 85, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 107, 109, 111, 112, 113, 115, 116,
    117, 118, 120, 121, 122, 122, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127, 127};

int bhp_sin(int a) {
    a &= 255;
    if (a < 64) return SIN64[a];
    if (a < 128) return SIN64[128 - a];
    if (a < 192) return -SIN64[a - 128];
    return -SIN64[256 - a];
}
int bhp_cos(int a) { return bhp_sin(a + 64); }

int bhp_isqrt(int v) {
    if (v <= 0) return 0;
    int r = 0, b = 1 << 30;
    while (b > v) b >>= 2;
    while (b) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
        b >>= 2;
    }
    return r;
}

void bhp_aim(int32_t x, int32_t y, int32_t tx, int32_t ty, int32_t speed, int32_t *vx, int32_t *vy) {
    int dx = (int)((tx - x) >> 4), dy = (int)((ty - y) >> 4);
    int d = bhp_isqrt(dx * dx + dy * dy);
    if (d == 0) { *vx = 0; *vy = speed; return; }
    *vx = (int32_t)((int64_t)speed * dx / d);
    *vy = (int32_t)((int64_t)speed * dy / d);
}

static int PX(int32_t v) { return (int)(v >> 8); }

/* ---- news ----------------------------------------------------------------- */

void bhp_add_fx(BhpStage *s, int x, int y, int kind) {
    if (s->nfx >= BHP_FX) return;
    s->fx[s->nfx++] = (BhpFx){(int16_t)x, (int16_t)y, (uint8_t)kind};
}

void bhp_refuel(BhpStage *s, int amount) {
    if (amount <= 0) return;
    s->fuel = (int16_t)imin(BHP_FUEL_MAX, s->fuel + amount);
    s->ev |= BEV_FUEL;
}

void bhp_kill_ship(BhpStage *s, int cause) {
    if (s->mode != BSM_FLY) return;
    s->mode = BSM_DEAD;
    s->mode_t = 0;
    s->slash_t = 0;
    s->dead_cause = (uint8_t)cause;
    s->ev |= BEV_CRASH;
    bhp_add_fx(s, PX(s->f.x), PX(s->f.y), 1);
}

/* ---- things --------------------------------------------------------------- */

int bhp_ent_add(BhpStage *s, int kind, int x, int y) {
    int i = -1;
    for (int k = 0; k < s->ne; k++)
        if (!s->e[k].on) { i = k; break; }
    if (i < 0) {
        if (s->ne >= BHP_ENTS) return -1;
        i = s->ne++;
    }
    BhpEnt *e = &s->e[i];
    memset(e, 0, sizeof *e);
    e->kind = (uint8_t)kind;
    e->on = 1;
    e->x = x * CHF_ONE;
    e->y = y * CHF_ONE;
    e->hx = (int16_t)x;
    e->hy = (int16_t)y;
    e->id = (uint8_t)i;
    e->hp = 1;
    e->hitno = 255;
    /* which way a mover sets off (a stage's own layout picks its own) */
    switch (kind) {
    case BEK_MOTH: case BEK_DRONE: case BEK_PLATE_H: case BEK_CRAWLER: e->vx = 1; break;
    case BEK_MITE: case BEK_PLATE_V: e->vy = 1; break;
    case BEK_FIREBAR: e->b = 1; break;
    default: break;
    }
    return i;
}

/* a crawler clings to the floor below it, or else to the ceiling above */
void bhp_ent_settle(BhpStage *s, BhpEnt *e) {
    if (e->kind != BEK_CRAWLER) return;
    int x = PX(e->x), y = PX(e->y);
    e->flag = bhp_solid(s, x, y + 6) ? 0 : bhp_solid(s, x, y - 6) ? 1 : 0;
    if (e->flag == 0) e->y = (int32_t)((((y - BHP_OY) / BHP_T) * BHP_T + BHP_OY + 5) * CHF_ONE);
    else e->y = (int32_t)((((y - BHP_OY) / BHP_T) * BHP_T + BHP_OY + 3) * CHF_ONE);
}

BhpShot *bhp_shot_add(BhpStage *s, int kind, int32_t x, int32_t y, int32_t vx, int32_t vy) {
    for (int k = 0; k < BHP_SHOTS; k++)
        if (!s->shot[k].on) {
            s->shot[k] = (BhpShot){x, y, vx, vy, (uint8_t)kind, 1, 0};
            return &s->shot[k];
        }
    return NULL;
}

/* half sizes of each kind's box, in the order of BEK_* */
static const int8_t HW[BEK_COUNT] = {0, 5, 5, 5, 5, 4, 5, 5, 5, 4, 3, 12, 3, 4, 5, 3, 6, 4, 6, 6, 5, 4, 5, 3, 4};
static const int8_t HH[BEK_COUNT] = {0, 4, 5, 5, 3, 4, 5, 4, 5, 4, 3, 3, 12, 5, 5, 3, 6, 4, 5, 6, 5, 4, 5, 3, 4};

void bhp_ent_box(const BhpEnt *e, int *x0, int *y0, int *x1, int *y1) {
    int hw = HW[e->kind], hh = HH[e->kind];
    if (e->kind == BEK_GUM) hw = hh = e->size == 2 ? 11 : e->size == 1 ? 7 : 5;
    int x = PX(e->x), y = PX(e->y);
    *x0 = x - hw;
    *y0 = y - hh;
    *x1 = x + hw - 1;
    *y1 = y + hh - 1;
}

bool bhp_ent_deadly(const BhpEnt *e) {
    switch (e->kind) {
    case BEK_FUEL: case BEK_BIGCOIN: case BEK_COIN: case BEK_CRYSTAL: case BEK_APPLE: case BEK_CHUNK: return false;
    default: return true;
    }
}

bool bhp_ship_box_hits(const BhpStage *s, int x0, int y0, int x1, int y1) {
    int cx = PX(s->f.x), cy = PX(s->f.y);
    return rects_overlap(cx - 4, cy - 4, 8, 8, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}

bool bhp_slash_box(const BhpStage *s, int *x0, int *y0, int *x1, int *y1) {
    if (s->mode != BSM_FLY || s->slash_t <= BHP_SLASH_T - BHP_SLASH_ACTIVE) return false;
    int cx = PX(s->f.x), cy = PX(s->f.y);
    if (s->f.face >= 0) { *x0 = cx + 2; *x1 = cx + BHP_SLASH_REACH; }
    else { *x0 = cx - BHP_SLASH_REACH; *x1 = cx - 2; }
    *y0 = cy - 9;
    *y1 = cy + 9;
    return true;
}

/* points and fuel for the enemies and bubbles a slash takes down */
static const int16_t PTS[BEK_COUNT] = {0, 100, 100, 150, 200, 150, 200, 200, 50};
static const int16_t FUEL[BEK_COUNT] = {0, 120, 120, 150, 150, 0, 150, 150, 150};

static void ent_down(BhpStage *s, BhpEnt *e) {
    e->on = 0;
    if (e->kind <= BEK_BUBBLE) {
        s->pts += PTS[e->kind];
        bhp_refuel(s, FUEL[e->kind]);
    }
    s->ev |= e->kind == BEK_BUBBLE ? BEV_BREAK : BEV_KILL;
    bhp_add_fx(s, PX(e->x), PX(e->y), e->kind == BEK_BUBBLE ? 3 : 2);
}

/* ---- loading ---------------------------------------------------------------- */

static int tile_of(char ch) {
    switch (ch) {
    case '#': return BTL_WALL;
    case '=': return BTL_WALL2;
    case '~': return BTL_LIQUID;
    case 'B': return BTL_BLOCK;
    case 'G': return BTL_GLASS;
    case 'X': return BTL_CRACK;
    case '[': return BTL_GATE1;
    case '{': return BTL_GATE1_OPEN;
    case ']': return BTL_GATE2;
    case '}': return BTL_GATE2_OPEN;
    case '1': return BTL_LEVER1;
    case '2': return BTL_LEVER2;
    case '<': return BTL_CANNON_L;
    case '>': return BTL_CANNON_R;
    case 'U': return BTL_CANNON_U;
    case 'D': return BTL_CANNON_D;
    case '^': return BTL_THORN;
    case 'P': return BTL_PRESS;
    default: return BTL_AIR;
    }
}

static int kind_of(char ch) {
    switch (ch) {
    case 'm': return BEK_MOTH;
    case 'v': return BEK_MITE;
    case 'z': return BEK_WASP;
    case 'i': return BEK_CRAWLER;
    case 'T': return BEK_TURRET;
    case 'g': return BEK_GHOST;
    case 'd': return BEK_DRONE;
    case 'o': return BEK_BUBBLE;
    case 'b': return BEK_BOMB;
    case 'f': return BEK_FIREBAR;
    case 'p': return BEK_PLATE_H;
    case 'q': return BEK_PLATE_V;
    case 'F': return BEK_FUEL;
    case '$': return BEK_BIGCOIN;
    case 'O': return BEK_CIRCLER;
    default: return BEK_NONE;
    }
}

static void build(BhpStage *s) {
    const BhpStageDef *d = &BHP_STAGE[s->idx];
    s->ne = 0;
    s->ncan = 0;
    s->ncoin_spots = 0;
    s->trig_c = s->trig_r = -1;
    s->warp_x = s->warp_y = -100;
    s->warp_to = d->warp_to;
    memset(s->shot, 0, sizeof s->shot);
    memset(&s->boss, 0, sizeof s->boss);
    for (int r = 0; r < BHP_TH; r++) {
        const char *row = d->rows[r];
        int len = row ? (int)strlen(row) : 0;
        for (int c = 0; c < BHP_TW; c++) {
            char ch = c < len ? row[c] : '.';
            int x = c * BHP_T + 4, y = BHP_OY + r * BHP_T + 4;
            int t = tile_of(ch);
            s->tile[r][c] = (uint8_t)t;
            if (t >= BTL_CANNON_L && t <= BTL_CANNON_D && s->ncan < 12) {
                s->can_c[s->ncan] = (int8_t)c;
                s->can_r[s->ncan] = (int8_t)r;
                s->ncan++;
            }
            switch (ch) {
            case 'S': s->start_x = (int16_t)x; s->start_y = (int16_t)y; break;
            case 'E': s->exit_x = (int16_t)x; s->exit_y = (int16_t)y; break;
            case 't': s->trig_c = (int16_t)c; s->trig_r = (int16_t)r; break;
            case 'c': s->cup_x = (int16_t)x; s->cup_y = (int16_t)y; break;
            case 'w': s->warp_x = (int16_t)x; s->warp_y = (int16_t)y; break;
            case '*':
                if (s->ncoin_spots < 5) {
                    s->coin_x[s->ncoin_spots] = (int16_t)x;
                    s->coin_y[s->ncoin_spots] = (int16_t)y;
                    s->ncoin_spots++;
                }
                break;
            default: {
                int k = kind_of(ch);
                if (k == BEK_NONE) break;
                int i = bhp_ent_add(s, k, x, y);
                if (i < 0) break;
                BhpEnt *e = &s->e[i];
                /* each thing's own start: which way it sets off, a phase */
                e->t = (uint16_t)((c * 37 + r * 23) % 240);
                switch (k) {
                case BEK_MOTH: case BEK_DRONE: case BEK_PLATE_H: e->vx = ((c + r) & 1) ? -1 : 1; break;
                case BEK_MITE: case BEK_PLATE_V: e->vy = ((c + r) & 1) ? -1 : 1; break;
                case BEK_CRAWLER: e->vx = (c & 1) ? -1 : 1; break;
                case BEK_FIREBAR: e->a = (int16_t)((c * 40 + r * 16) & 255); e->b = (c & 1) ? -1 : 1; break;
                default: break;
                }
                break;
            }
            }
        }
    }
    for (int i = 0; i < s->ne; i++) bhp_ent_settle(s, &s->e[i]);
    s->kind = (uint8_t)bhp_kind(s->idx);
    s->exit_open = s->kind == BHK_STAGE;
    s->f = (ChmFlight){s->start_x * CHF_ONE, s->start_y * CHF_ONE, 0, 0, 1};
    s->mode = BSM_BUBBLE;
    s->mode_t = 0;
    s->fuel = BHP_FUEL_MAX;
    s->slash_t = s->slash_cd = 0;
    s->life_t = 0;
    s->coin_t = 0;
    s->coins_got = 0;
    s->lever_cd = 0;
    s->round = 0;
    s->crystals_left = 0;
    s->round_t = 0;
    s->ver++;
    if (s->kind == BHK_BOSS) bhp_boss_init(s);
}

void bhp_stage_load(BhpStage *s, int idx, int crystal_pts, uint64_t seed) {
    memset(s, 0, sizeof *s);
    s->idx = (uint8_t)iclamp(idx, 0, BHP_STAGES - 1);
    s->crystal_pts = (uint8_t)crystal_pts;
    rng_seed(&s->rng, seed ^ (uint64_t)(idx * 7919 + 17));
    build(s);
}

static bool gate_tile(int t) { return t >= BTL_GATE1 && t <= BTL_GATE2_OPEN; }

void bhp_stage_respawn(BhpStage *s) {
    uint8_t boss_down = s->boss.down, bonus_done = s->bonus_done;
    /* what a crash doesn't undo: the gates as the levers left them, and
     * how far the boss has been worn down */
    static uint8_t gates[BHP_TH][BHP_TW];
    static BhpBoss boss;
    static BhpEnt ents[BHP_ENTS];
    int ne = s->ne;
    memcpy(gates, s->tile, sizeof gates);
    boss = s->boss;
    memcpy(ents, s->e, sizeof ents);
    build(s);
    for (int r = 0; r < BHP_TH; r++)
        for (int c = 0; c < BHP_TW; c++)
            if (gate_tile(gates[r][c]) && gate_tile(s->tile[r][c])) s->tile[r][c] = gates[r][c];
    if (s->kind == BHK_BOSS && !boss_down) bhp_boss_restore(s, &boss, ents, ne);
    for (int i = 0; i < s->ne; i++)
        if (s->e[i].on && (s->e[i].kind == BEK_CIRCLER || s->e[i].kind == BEK_BIGCOIN) && (s->taken >> (s->e[i].id & 63) & 1))
            s->e[i].on = 0;
    if (boss_down) {
        s->boss.down = 1;
        s->boss.down_t = 999;
        for (int i = 0; i < s->ne; i++)
            if (s->e[i].kind >= BEK_BUCKET) s->e[i].on = 0;
        s->exit_open = 1;
    }
    if (bonus_done) {
        s->bonus_done = 1;
        s->exit_open = 1;
    }
}

/* ---- the slash ---------------------------------------------------------------- */

static void toggle_gates(BhpStage *s, int lever) {
    int a = lever == BTL_LEVER1 ? BTL_GATE1 : BTL_GATE2, b = a + 1;
    for (int r = 0; r < BHP_TH; r++)
        for (int c = 0; c < BHP_TW; c++) {
            if (s->tile[r][c] == a) bhp_set_tile(s, c, r, b);
            else if (s->tile[r][c] == b) bhp_set_tile(s, c, r, a);
        }
    s->ev |= BEV_LEVER;
    /* a gate swinging shut on the ship */
    if (s->mode == BSM_FLY && chm_flight_blocked(BHP_TUNE.half, bhp_solid, s, s->f.x, s->f.y)) bhp_kill_ship(s, 2);
}

static void slash_tiles(BhpStage *s, int x0, int y0, int x1, int y1) {
    int c0 = imax(0, x0 / BHP_T), c1 = imin(BHP_TW - 1, x1 / BHP_T);
    int r0 = imax(0, (y0 - BHP_OY) / BHP_T), r1 = imin(BHP_TH - 1, (y1 - BHP_OY) / BHP_T);
    if (y1 < BHP_OY) return;
    for (int r = r0; r <= r1; r++)
        for (int c = c0; c <= c1; c++) {
            int t = s->tile[r][c], x = c * BHP_T + 4, y = BHP_OY + r * BHP_T + 4;
            if (t == BTL_BLOCK) {
                bhp_set_tile(s, c, r, BTL_AIR);
                s->pts += 50;
                bhp_refuel(s, 90);
                s->ev |= BEV_BREAK;
                bhp_add_fx(s, x, y, 4);
            } else if (t == BTL_GLASS) {
                bhp_set_tile(s, c, r, BTL_AIR);
                s->pts += 30;
                s->ev |= BEV_BREAK;
                bhp_add_fx(s, x, y, 5);
            } else if ((t == BTL_LEVER1 || t == BTL_LEVER2) && !s->lever_cd) {
                s->lever_cd = 24;
                toggle_gates(s, t);
                bhp_add_fx(s, x, y, 6);
            }
        }
}

static void slash_things(BhpStage *s, int x0, int y0, int x1, int y1) {
    for (int i = 0; i < s->ne; i++) {
        BhpEnt *e = &s->e[i];
        if (!e->on || e->hitno == s->slash_no) continue;
        int a0, b0, a1, b1;
        bhp_ent_box(e, &a0, &b0, &a1, &b1);
        if (!rects_overlap(x0, y0, x1 - x0 + 1, y1 - y0 + 1, a0, b0, a1 - a0 + 1, b1 - b0 + 1)) continue;
        e->hitno = s->slash_no;
        switch (e->kind) {
        case BEK_MOTH: case BEK_MITE: case BEK_WASP: case BEK_CRAWLER: case BEK_TURRET: case BEK_GHOST: case BEK_DRONE:
        case BEK_BUBBLE:
            ent_down(s, e);
            break;
        case BEK_BOMB:
            if (!e->flag) {
                e->flag = 1;
                e->t = 60;
                s->ev |= BEV_FUSE;
            }
            break;
        case BEK_CRYSTAL:
            e->on = 0;
            s->pts += s->crystal_pts;
            bhp_refuel(s, 90);
            s->ev |= BEV_CRYSTAL;
            bhp_add_fx(s, PX(e->x), PX(e->y), 7);
            if (s->crystals_left) s->crystals_left--;
            break;
        case BEK_BUCKET: case BEK_APPLE: case BEK_LAMP: case BEK_GUM: case BEK_SPIKE:
            bhp_boss_ent_slash(s, e);
            break;
        default: break;
        }
    }
}

/* ---- the parts of a stage ------------------------------------------------------ */

/* would a box of half size (hw, hh) at (x, y) px touch the scenery? */
static bool box_blocked(const BhpStage *s, int x, int y, int hw, int hh) {
    for (int py = y - hh; py <= y + hh - 1; py += imax(1, hh - 1))
        for (int px = x - hw; px <= x + hw - 1; px += imax(1, hw - 1))
            if (bhp_solid(s, px, py)) return true;
    return bhp_solid(s, x - hw, y + hh - 1) || bhp_solid(s, x + hw - 1, y + hh - 1) || bhp_solid(s, x + hw - 1, y - hh);
}

static void blast(BhpStage *s, int x, int y) {
    s->ev |= BEV_BLAST;
    bhp_add_fx(s, x, y, 8);
    for (int r = 0; r < BHP_TH; r++)
        for (int c = 0; c < BHP_TW; c++) {
            int t = s->tile[r][c];
            if (t != BTL_BLOCK && t != BTL_GLASS && t != BTL_CRACK) continue;
            int dx = c * BHP_T + 4 - x, dy = BHP_OY + r * BHP_T + 4 - y;
            if (dx * dx + dy * dy <= 26 * 26) {
                bhp_set_tile(s, c, r, BTL_AIR);
                bhp_add_fx(s, c * BHP_T + 4, BHP_OY + r * BHP_T + 4, t == BTL_GLASS ? 5 : 4);
            }
        }
    for (int i = 0; i < s->ne; i++) {
        BhpEnt *e = &s->e[i];
        if (!e->on) continue;
        int dx = PX(e->x) - x, dy = PX(e->y) - y;
        if (dx * dx + dy * dy > 26 * 26) continue;
        if (e->kind <= BEK_BUBBLE) ent_down(s, e);
        else if (e->kind == BEK_BOMB && !e->flag) { e->flag = 1; e->t = 8; }
    }
    if (s->mode == BSM_FLY) {
        int dx = PX(s->f.x) - x, dy = PX(s->f.y) - y;
        if (dx * dx + dy * dy <= 22 * 22) bhp_kill_ship(s, 3);
    }
}

/* firebar spark k's middle */
static void spark_pos(const BhpEnt *e, int k, int *x, int *y) {
    int a = e->a, r = 8 + k * 8;
    *x = PX(e->x) + bhp_cos(a) * r / 127;
    *y = PX(e->y) + bhp_sin(a) * r / 127;
}

static void step_ent(BhpStage *s, BhpEnt *e) {
    int x = PX(e->x), y = PX(e->y);
    int sx = PX(s->f.x), sy = PX(s->f.y);
    bool flying = s->mode == BSM_FLY;
    e->t++;
    switch (e->kind) {
    case BEK_MOTH: {
        int nx = x + (int)e->vx * 6;
        if (box_blocked(s, nx, y, 5, 4)) e->vx = -e->vx;
        e->x += e->vx * 154;
        e->y = (e->hy + bhp_sin(e->t * 3) * 3 / 127) * CHF_ONE;
        break;
    }
    case BEK_MITE: {
        int ny = y + (int)e->vy * 6;
        if (box_blocked(s, x, ny, 5, 5)) e->vy = -e->vy;
        e->y += e->vy * 128;
        break;
    }
    case BEK_WASP: {
        int dx = sx - x, dy = sy - y;
        int32_t vx = e->vx, vy = e->vy;
        if (flying && dx * dx + dy * dy < 100 * 100) {
            vx += isign(dx) * 6;
            vy += isign(dy) * 6;
        } else {
            vx -= isign(vx) * imin(4, iabs(vx));
            vy -= isign(vy) * imin(4, iabs(vy));
        }
        vx = iclamp(vx, -150, 150);
        vy = iclamp(vy, -150, 150);
        if (box_blocked(s, PX(e->x + vx), y, 5, 5)) vx = 0;
        else e->x += vx;
        if (box_blocked(s, PX(e->x), PX(e->y + vy), 5, 5)) vy = 0;
        else e->y += vy;
        e->vx = vx;
        e->vy = vy;
        break;
    }
    case BEK_CRAWLER: {
        int dir = e->vx >= 0 ? 1 : -1, front = x + dir * 6;
        int under = e->flag ? y - 6 : y + 5;
        bool wall = bhp_solid(s, front, y - 2) || bhp_solid(s, front, y + 1);
        bool edge = !bhp_solid(s, front, under);
        if (wall || edge) e->vx = -dir;
        else e->x += dir * 102;
        break;
    }
    case BEK_TURRET:
        if ((e->t % 150) == 0 && flying) {
            int dx = sx - x, dy = sy - y;
            if (dx * dx + dy * dy < 150 * 150) {
                int32_t vx, vy;
                bhp_aim(e->x, e->y, s->f.x, s->f.y, 282, &vx, &vy);
                bhp_shot_add(s, BSH_PELLET, e->x, e->y, vx, vy);
                s->ev |= BEV_SHOOT;
            }
        }
        break;
    case BEK_GHOST:
        e->x = (e->hx + bhp_sin(e->t) * 36 / 127) * CHF_ONE;
        e->y = (e->hy + bhp_sin(e->t * 2 + 40) * 10 / 127) * CHF_ONE;
        break;
    case BEK_DRONE: {
        int nx = x + (int)e->vx * 7;
        if (box_blocked(s, nx, y, 5, 4)) e->vx = -e->vx;
        e->x += e->vx * 205;
        if ((e->t % 120) == 0) {
            bhp_shot_add(s, BSH_PELLET, e->x, e->y + 6 * CHF_ONE, 0, 320);
            s->ev |= BEV_SHOOT;
        }
        break;
    }
    case BEK_BUBBLE:
        e->y = (e->hy + bhp_sin(e->t * 2) * 2 / 127) * CHF_ONE;
        break;
    case BEK_FIREBAR:
        if ((e->t & 1) == 0) e->a = (int16_t)((e->a + e->b) & 255);
        break;
    case BEK_PLATE_H: {
        int nx = x + (int)e->vx * 13;
        if (box_blocked(s, nx, y, 1, 3)) e->vx = -e->vx;
        e->x += e->vx * 192;
        break;
    }
    case BEK_PLATE_V: {
        int ny = y + (int)e->vy * 13;
        if (box_blocked(s, x, ny, 3, 1)) e->vy = -e->vy;
        e->y += e->vy * 192;
        break;
    }
    case BEK_COIN:
        break;
    case BEK_CRYSTAL: {
        int32_t nx = e->x + e->vx;
        if (box_blocked(s, PX(nx), y, 4, 4)) e->vx = -e->vx;
        else e->x = nx;
        int32_t ny = e->y + e->vy;
        if (box_blocked(s, PX(e->x), PX(ny), 4, 4)) e->vy = -e->vy;
        else e->y = ny;
        break;
    }
    default: break;
    }
}

/* the bomb's fuse is kept in t and counts down; every other kind counts up */
static void step_things(BhpStage *s) {
    for (int i = 0; i < s->ne; i++) {
        BhpEnt *e = &s->e[i];
        if (!e->on) continue;
        if (e->kind == BEK_BOMB) {
            if (e->flag && e->t > 0 && --e->t == 0) {
                e->on = 0;
                blast(s, PX(e->x), PX(e->y));
            }
            continue;
        }
        if (e->kind >= BEK_BUCKET) continue; /* the boss moves its own */
        step_ent(s, e);
    }
}

static void step_cannons(BhpStage *s) {
    for (int k = 0; k < s->ncan; k++) {
        int c = s->can_c[k], r = s->can_r[k];
        if ((s->t + (uint32_t)(c * 7 + r * 13)) % 150 != 0) continue;
        int t = s->tile[r][c];
        int dx = t == BTL_CANNON_L ? -1 : t == BTL_CANNON_R ? 1 : 0, dy = t == BTL_CANNON_U ? -1 : t == BTL_CANNON_D ? 1 : 0;
        int x = c * BHP_T + 4 + dx * 7, y = BHP_OY + r * BHP_T + 4 + dy * 7;
        bhp_shot_add(s, BSH_BALL, x * CHF_ONE, y * CHF_ONE, dx * 320, dy * 320);
        s->ev |= BEV_SHOOT;
    }
}

static void step_shots(BhpStage *s) {
    for (int k = 0; k < BHP_SHOTS; k++) {
        BhpShot *b = &s->shot[k];
        if (!b->on || b->kind == BSH_RETURN) continue;
        b->x += b->vx;
        b->y += b->vy;
        b->life++;
        if (b->life > 900 || bhp_solid(s, PX(b->x), PX(b->y))) b->on = 0;
    }
}

/* ---- the crystal rooms ---------------------------------------------------------- */

static void spawn_crystals(BhpStage *s) {
    int n = s->round, speed = imin(1280, 128 + 128 * (s->round - 1));
    for (int k = 0; k < n; k++) {
        int x = 0, y = 0;
        for (int tries = 0; tries < 60; tries++) {
            int c = rng_range(&s->rng, 2, BHP_TW - 3), r = rng_range(&s->rng, 2, BHP_TH - 3);
            x = c * BHP_T + 4;
            y = BHP_OY + r * BHP_T + 4;
            int dx = x - PX(s->f.x), dy = y - PX(s->f.y);
            if (!box_blocked(s, x, y, 6, 6) && dx * dx + dy * dy > 56 * 56) break;
        }
        int i = bhp_ent_add(s, BEK_CRYSTAL, x, y);
        if (i < 0) continue;
        int a = rng_range(&s->rng, 0, 3) * 64 + rng_range(&s->rng, 12, 52);
        s->e[i].vx = bhp_cos(a) * speed / 127;
        s->e[i].vy = bhp_sin(a) * speed / 127;
    }
    s->crystals_left = (uint8_t)n;
    s->round_t = 0;
    s->ev |= BEV_ROUND;
}

static void step_bonus(BhpStage *s) {
    if (s->bonus_done || s->round == 0) return;
    s->round_t++;
    if (s->crystals_left == 0) {
        s->round++;
        spawn_crystals(s);
    } else if (s->round_t >= BHP_ROUND_T) {
        for (int i = 0; i < s->ne; i++)
            if (s->e[i].kind == BEK_CRYSTAL) s->e[i].on = 0;
        s->crystals_left = 0;
        s->bonus_done = 1;
        s->exit_open = 1;
        s->ev |= BEV_EXIT_OPEN;
    }
}

/* ---- the ship meets things ---------------------------------------------------------- */

static bool in_zone(int dx, int dy, int side) {
    switch (side) {
    case 0: return iabs(dx) <= 12 && dy >= -28 && dy <= -9;
    case 1: return iabs(dy) <= 12 && dx >= 9 && dx <= 28;
    case 2: return iabs(dx) <= 12 && dy >= 9 && dy <= 28;
    default: return iabs(dy) <= 12 && dx >= -28 && dx <= -9;
    }
}

static void touch_things(BhpStage *s) {
    int sx = PX(s->f.x), sy = PX(s->f.y);
    for (int i = 0; i < s->ne && s->mode == BSM_FLY; i++) {
        BhpEnt *e = &s->e[i];
        if (!e->on) continue;
        if (e->kind == BEK_CIRCLER) {
            int dx = sx - PX(e->x), dy = sy - PX(e->y);
            for (int side = 0; side < 4; side++)
                if (!(e->flag & (1 << side)) && in_zone(dx, dy, side)) {
                    e->flag |= (uint8_t)(1 << side);
                    s->ev |= BEV_NODE;
                }
            if (e->flag == 15) {
                e->on = 0;
                s->taken |= (uint64_t)1 << (e->id & 63);
                s->pts += 500;
                s->ev |= BEV_CIRCLER;
                bhp_add_fx(s, PX(e->x), PX(e->y), 9);
                continue;
            }
        }
        if (e->kind == BEK_FIREBAR) {
            for (int k = 0; k < 4; k++) {
                int px, py;
                spark_pos(e, k, &px, &py);
                if (bhp_ship_box_hits(s, px - 3, py - 3, px + 2, py + 2)) { bhp_kill_ship(s, 4); break; }
            }
            if (s->mode != BSM_FLY) break;
        }
        int x0, y0, x1, y1;
        bhp_ent_box(e, &x0, &y0, &x1, &y1);
        if (!bhp_ship_box_hits(s, x0, y0, x1, y1)) continue;
        switch (e->kind) {
        case BEK_FUEL:
            e->on = 0;
            s->fuel = BHP_FUEL_MAX;
            s->ev |= BEV_FUEL;
            bhp_add_fx(s, PX(e->x), PX(e->y), 10);
            break;
        case BEK_BIGCOIN: {
            e->on = 0;
            s->taken |= (uint64_t)1 << (e->id & 63);
            s->ev |= BEV_BIGCOIN;
            s->coins_got = 0;
            s->coin_t = BHP_COIN_T;
            for (int k = 0; k < 5; k++) {
                int cx, cy;
                if (k < s->ncoin_spots) { cx = s->coin_x[k]; cy = s->coin_y[k]; }
                else { cx = PX(e->x) + bhp_cos(k * 51) * 24 / 127; cy = PX(e->y) + bhp_sin(k * 51) * 24 / 127; }
                bhp_ent_add(s, BEK_COIN, cx, cy);
            }
            break;
        }
        case BEK_COIN: {
            static const int16_t CP[5] = {100, 100, 100, 200, 500};
            e->on = 0;
            s->pts += CP[imin(4, s->coins_got)];
            s->coins_got++;
            s->ev |= BEV_COIN;
            bhp_add_fx(s, PX(e->x), PX(e->y), 11);
            break;
        }
        case BEK_CRYSTAL: case BEK_APPLE: case BEK_CHUNK:
            break;
        case BEK_CIRCLER: {
            /* only its body, not the ring of nodes round it */
            int dx = sx - PX(e->x), dy = sy - PX(e->y);
            if (iabs(dx) < 10 && iabs(dy) < 10) bhp_kill_ship(s, 5);
            break;
        }
        default:
            if (bhp_ent_deadly(e)) {
                bhp_kill_ship(s, 5);
                /* some enemies go down with the ship, for no points */
                if (e->kind == BEK_MOTH || e->kind == BEK_MITE || e->kind == BEK_WASP || e->kind == BEK_CRAWLER ||
                    e->kind == BEK_GHOST) {
                    e->on = 0;
                    bhp_add_fx(s, PX(e->x), PX(e->y), 2);
                }
            }
            break;
        }
    }
    for (int k = 0; k < BHP_SHOTS && s->mode == BSM_FLY; k++) {
        const BhpShot *b = &s->shot[k];
        if (!b->on || b->kind == BSH_RETURN) continue;
        int r = b->kind == BSH_BALL ? 3 : 2, x = PX(b->x), y = PX(b->y);
        if (bhp_ship_box_hits(s, x - r, y - r, x + r - 1, y + r - 1)) bhp_kill_ship(s, 6);
    }
    if (s->mode == BSM_FLY && s->kind == BHK_BOSS && bhp_boss_hits_ship(s)) bhp_kill_ship(s, 7);
    if (s->mode != BSM_FLY) return;
    /* tea: the hidden spot, then the cup */
    if (s->cup == 0 && s->trig_c >= 0 &&
        bhp_ship_box_hits(s, s->trig_c * BHP_T, BHP_OY + s->trig_r * BHP_T, s->trig_c * BHP_T + 7, BHP_OY + s->trig_r * BHP_T + 7)) {
        s->cup = 1;
        s->ev |= BEV_CUP_SHOW;
        bhp_add_fx(s, s->cup_x, s->cup_y, 12);
    }
    if (s->cup == 1 && bhp_ship_box_hits(s, s->cup_x - 4, s->cup_y - 4, s->cup_x + 3, s->cup_y + 3)) {
        s->cup = 2;
        s->pts += 500;
        s->ev |= BEV_CUP;
        bhp_add_fx(s, s->cup_x, s->cup_y, 13);
    }
    if (s->warp_to >= 0 && s->t < BHP_WARP_SHOW &&
        bhp_ship_box_hits(s, s->warp_x - 5, s->warp_y - 5, s->warp_x + 4, s->warp_y + 4)) {
        s->mode = BSM_WARP;
        s->mode_t = 0;
        s->ev |= BEV_WARP;
        return;
    }
    if (s->exit_open && bhp_ship_box_hits(s, s->exit_x - 6, s->exit_y - 6, s->exit_x + 5, s->exit_y + 5)) {
        s->mode = BSM_CLEAR;
        s->mode_t = 0;
        s->ev |= BEV_CLEAR;
    }
}

/* ---- one frame -------------------------------------------------------------- */

void bhp_stage_step(BhpStage *s, unsigned ctl) {
    s->ev = 0;
    s->pts = 0;
    s->thrust_frames = 0;
    s->nfx = 0;
    s->t++;
    s->ctl = (uint8_t)ctl;
    if (s->lever_cd) s->lever_cd--;
    if (s->slash_cd) s->slash_cd--;
    if (s->slash_t) s->slash_t--;

    if (s->mode == BSM_BUBBLE) {
        if (ctl & (CHF_THRUST | CHF_LEFT | CHF_RIGHT)) {
            s->mode = BSM_FLY;
            s->ev |= BEV_POP;
            bhp_add_fx(s, s->start_x, s->start_y, 14);
            if (s->kind == BHK_BONUS && !s->bonus_done && s->round == 0) {
                s->round = 1;
                spawn_crystals(s);
            }
        }
    }
    if (s->mode == BSM_FLY) {
        s->life_t++;
        unsigned bits = ctl & (CHF_LEFT | CHF_RIGHT);
        if (ctl & CHF_THRUST) {
            if (s->fuel > 0) {
                bits |= CHF_THRUST;
                s->fuel--;
                s->thrust_frames = 1;
            } else if ((s->t & 15) == 0) s->ev |= BEV_DRY;
        }
        if ((ctl & BHP_CTL_SLASH) && !s->slash_cd) {
            s->slash_t = BHP_SLASH_T;
            s->slash_cd = BHP_SLASH_CD;
            s->slash_no++;
            s->ev |= BEV_SLASH;
        }
        if (s->slash_t) bits |= CHF_SLASHING;
        chm_flight_control(&s->f, &BHP_TUNE, bits);
        /* ... and gravity pulls half as hard while it lasts: a slight brake */
        if (s->slash_t && s->f.vy > 0) s->f.vy -= BHP_TUNE.gravity / 2;
        if (chm_flight_probe(&s->f, &BHP_TUNE, bhp_solid, s)) bhp_kill_ship(s, 1);
    } else if (s->mode != BSM_BUBBLE) {
        s->mode_t++;
    }

    /* the world goes on */
    step_things(s);
    step_cannons(s);
    step_shots(s);
    if (s->kind == BHK_BOSS) bhp_boss_step(s);
    if (s->kind == BHK_BONUS) step_bonus(s);
    if (s->coin_t && --s->coin_t == 0)
        for (int i = 0; i < s->ne; i++)
            if (s->e[i].kind == BEK_COIN) s->e[i].on = 0;

    if (s->mode == BSM_FLY) {
        int x0, y0, x1, y1;
        if (bhp_slash_box(s, &x0, &y0, &x1, &y1)) {
            slash_tiles(s, x0, y0, x1, y1);
            slash_things(s, x0, y0, x1, y1);
            if (s->kind == BHK_BOSS) bhp_boss_slash(s, x0, y0, x1, y1);
        }
        touch_things(s);
    }
}
