/* CLARION CALL - the rules of a map.
 *
 * Everything a map holds lives in one ClcWorld, so the demo player can copy
 * it and play the copy forward on these same rules. A step only leaves news
 * (w->ev, w->fx, w->enter) for the run to show, hear and act on.
 *
 * The Clarion flies on CHIME CIRCUIT's flight model with BELLHOP's numbers;
 * here walls bounce it back and only cost a point of the bar when it hits
 * them hard, and it can set down on a level floor if it comes down slowly
 * and straight. Its tank is the run's one tank: thrust burns it, and with it
 * dry each few frames of thrust burn a coin instead; with no coins either
 * the ship drops and bursts on the ground. Clary steps out when it lands:
 * outside, one hit or a fall of more than a tile and a half ends the run. */
#include "clc.h"

/* BELLHOP's ship numbers, with a bounce off the walls */
const ChmFlightTune CLC_TUNE = {
    18,   /* gravity */
    44,   /* thrust */
    12,   /* accel_x */
    14,   /* accel_thrust_x */
    2,    /* drag_x */
    384,  /* max_vx: 1.5 px a frame */
    448,  /* max_up */
    608,  /* max_down */
    14,   /* over_decay */
    1792, /* top_speed */
    110,  /* bounce: 43 % */
    96,   /* min_bounce */
    248,  /* slash_drag */
    4,    /* half: an 8 x 8 hit box */
};
/* holding DOWN: a heavier fall */
static const ChmFlightTune DIVE_TUNE = {36, 44, 12, 14, 2, 384, 448, 900, 14, 1792, 110, 96, 248, 4};

#define SLASH_T 12
#define SLASH_ACTIVE 8
#define SLASH_CD 11 /* mashed, a slow glide that costs no fuel */
#define SLASH_REACH 18

/* ---- small maths ----------------------------------------------------------- */

static const int8_t SIN64[65] = {
    0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49, 51, 54, 57, 60, 63, 65, 68, 71, 73,
    76, 78, 81, 83, 85, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 107, 109, 111, 112, 113, 115, 116,
    117, 118, 120, 121, 122, 122, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127, 127};

int clc_sin(int a) {
    a &= 255;
    if (a < 64) return SIN64[a];
    if (a < 128) return SIN64[128 - a];
    if (a < 192) return -SIN64[a - 128];
    return -SIN64[256 - a];
}
int clc_cos(int a) { return clc_sin(a + 64); }

int clc_isqrt(int v) {
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

void clc_aim(int32_t x, int32_t y, int32_t tx, int32_t ty, int32_t speed, int32_t *vx, int32_t *vy) {
    int dx = (int)((tx - x) / 16), dy = (int)((ty - y) / 16);
    int d = clc_isqrt(dx * dx + dy * dy);
    if (d == 0) { *vx = 0; *vy = speed; return; }
    *vx = (int32_t)((int64_t)speed * dx / d);
    *vy = (int32_t)((int64_t)speed * dy / d);
}

static int PX(int32_t v) { return (int)(v / 256) - (v < 0 && v % 256 ? 1 : 0); }

/* ---- tiles ----------------------------------------------------------------- */

int clc_map_tile(const ClcWorld *w, int px, int py) {
    if (px < 0 || py < 0 || px >= w->w * CLC_T || py >= w->h * CLC_T) {
        /* the escape shaft is open at the top */
        if (w->kind == WK_ESCAPE && py < 0 && px >= 0 && px < w->w * CLC_T) return w->tile[0][px / CLC_T];
        return MT_ROCK;
    }
    return w->tile[py / CLC_T][px / CLC_T];
}

bool clc_map_solid(const void *ctx, int px, int py) { return clc_map_solid_tile(clc_map_tile((const ClcWorld *)ctx, px, py)); }

static bool map_feet(const void *ctx, int px, int py, bool feet) {
    const ClcWorld *w = (const ClcWorld *)ctx;
    int t = clc_map_tile(w, px, py);
    if (clc_map_solid_tile(t)) return true;
    /* the top rung of a ladder holds her up */
    if (feet && t == MT_LADDER && (py % CLC_T) == 0 && clc_map_tile(w, px, py - 1) != MT_LADDER) return true;
    return false;
}

static bool map_ladder(const void *ctx, int px, int py) { return clc_map_tile((const ClcWorld *)ctx, px, py) == MT_LADDER; }

void clc_set_tile(ClcWorld *w, int c, int r, int t) {
    if (c < 0 || r < 0 || c >= w->w || r >= w->h || w->tile[r][c] == t) return;
    if (w->sim) return; /* a look-ahead doesn't break the real map */
    w->tile[r][c] = (uint8_t)t;
    w->ver++;
}

/* ---- the kinds ------------------------------------------------------------------ */

enum { KF_FOE = 1, KF_SHIPPROOF = 2, KF_INVULN = 4, KF_THROUGH = 8, KF_PICKUP = 16, KF_HARMLESS = 32 };
typedef struct Kind { int8_t hw, hh, hp; uint8_t dmg, coins, flags; } Kind;
static const Kind KIND[EK_COUNT] = {
    [EK_FLITTER] = {5, 4, 2, 4, 3, KF_FOE},
    [EK_CREEPER] = {5, 3, 2, 4, 3, KF_FOE},
    [EK_GRUB] = {9, 4, 6, 4, 50, KF_FOE},
    [EK_SNAP] = {6, 3, 3, 4, 3, KF_FOE},
    [EK_FIREDRONE] = {5, 5, 3, 4, 5, KF_FOE},
    [EK_WINDDRONE] = {5, 5, 3, 4, 5, KF_FOE},
    [EK_LEECH] = {3, 3, 1, 4, 2, KF_FOE},
    [EK_EYE] = {4, 4, 2, 4, 5, KF_FOE},
    [EK_ACID] = {4, 4, 1, 4, 3, KF_FOE},
    [EK_BLOB] = {5, 5, 2, 4, 5, KF_FOE},
    [EK_SENTRY] = {5, 4, 4, 4, 10, KF_FOE | KF_SHIPPROOF},
    [EK_SNAKE] = {4, 4, 6, 4, 50, KF_FOE},
    [EK_CRAB] = {4, 4, 3, 4, 5, KF_FOE},
    [EK_GHOST] = {5, 5, 4, 4, 10, KF_FOE | KF_THROUGH},
    [EK_JETFIRE] = {4, 4, 0, 4, 0, KF_FOE | KF_INVULN},
    [EK_BOOMER] = {4, 5, 3, 4, 10, KF_FOE},
    [EK_WORM] = {5, 5, 8, 4, 50, KF_FOE},
    [EK_CHASER] = {5, 5, 4, 4, 10, KF_FOE},
    [EK_BIGBELLY] = {10, 9, 10, 6, 50, KF_FOE}, /* the big one: 3 points */
    [EK_POD] = {4, 4, 2, 4, 5, KF_FOE},
    [EK_LATE] = {5, 5, 0, 99, 0, KF_FOE | KF_INVULN | KF_THROUGH},
    [EK_SEG] = {4, 4, 0, 4, 0, KF_FOE},
    [EK_NOTE] = {5, 5, 0, 0, 0, KF_PICKUP},
    [EK_FAKENOTE] = {5, 5, 0, 0, 0, KF_HARMLESS},
    [EK_PLUM] = {4, 4, 0, 0, 0, KF_PICKUP},
    [EK_COIN] = {3, 3, 0, 0, 0, KF_PICKUP},
    [EK_RING] = {6, 6, 0, 0, 0, KF_PICKUP},
    [EK_FUELCAN] = {4, 5, 0, 0, 0, KF_PICKUP},
    [EK_ARROW] = {4, 4, 0, 0, 0, KF_HARMLESS},
    [EK_NEST] = {6, 6, 3, 4, 5, KF_FOE},
    [EK_FALLER] = {4, 4, 0, 99, 0, KF_FOE | KF_INVULN},
    [EK_WALLEYE] = {4, 4, 2, 4, 5, KF_FOE},
    [EK_BMISSILE] = {3, 3, 1, 4, 0, KF_FOE},
};

int clc_ent_add(ClcWorld *w, int kind, int x, int y) {
    int i = -1;
    for (int k = 0; k < w->ne; k++)
        if (!w->e[k].on) { i = k; break; }
    if (i < 0) {
        if (w->ne >= CLC_ENTS) return -1;
        i = w->ne++;
    }
    ClcEnt *e = &w->e[i];
    memset(e, 0, sizeof *e);
    e->kind = (uint8_t)kind;
    e->on = 1;
    e->x = x * 256;
    e->y = y * 256;
    e->hx = (int16_t)x;
    e->hy = (int16_t)y;
    e->hp = KIND[kind].hp;
    e->dir = 1;
    e->hitno = 255;
    return i;
}

static void ent_box(const ClcEnt *e, int *x0, int *y0, int *x1, int *y1) {
    int hw = KIND[e->kind].hw, hh = KIND[e->kind].hh;
    *x0 = PX(e->x) - hw;
    *y0 = PX(e->y) - hh;
    *x1 = PX(e->x) + hw - 1;
    *y1 = PX(e->y) + hh - 1;
}

static void add_fx(ClcWorld *w, int x, int y, int kind) {
    if (w->nfx >= CLC_FX) return;
    w->fx[w->nfx++] = (ClcFx){(int16_t)x, (int16_t)y, (uint8_t)kind};
}

static ClcShot *shot_add(ClcWorld *w, int kind, int32_t x, int32_t y, int32_t vx, int32_t vy, int life, int dmg) {
    for (int k = 0; k < CLC_SHOTS; k++)
        if (!w->shot[k].on) {
            w->shot[k] = (ClcShot){x, y, vx, vy, (uint8_t)kind, 1, (uint8_t)dmg, 0, (uint16_t)life, -1};
            return &w->shot[k];
        }
    return NULL;
}

/* ---- the player ----------------------------------------------------------------- */

static int32_t player_x(const ClcWorld *w) { return w->on_foot ? w->cl.x : w->f.x; }
static int32_t player_y(const ClcWorld *w) { return w->on_foot ? w->cl.y - CLC_WALK_MAP.h * 128 : w->f.y; }

static void player_box(const ClcWorld *w, int *x0, int *y0, int *x1, int *y1) {
    if (w->on_foot) {
        int x = PX(w->cl.x), y = PX(w->cl.y);
        int h = w->cl.crouch ? CLC_WALK_MAP.h * 5 / 8 : CLC_WALK_MAP.h;
        *x0 = x - CLC_WALK_MAP.hw;
        *x1 = x + CLC_WALK_MAP.hw - 1;
        *y0 = y - h;
        *y1 = y - 1;
    } else {
        int x = PX(w->f.x), y = PX(w->f.y);
        *x0 = x - 4;
        *y0 = y - 4;
        *x1 = x + 3;
        *y1 = y + 3;
    }
}

static void die(ClcWorld *w, ClcPlayer *p) {
    if (p->dead && (w->ev & CEV_DIE)) return;
    p->dead = 1;
    p->hp = 0;
    w->ev |= CEV_DIE;
    int x0, y0, x1, y1;
    player_box(w, &x0, &y0, &x1, &y1);
    add_fx(w, (x0 + x1) / 2, (y0 + y1) / 2, FX_BOOM);
}

/* something hurts the player: the ship takes it off the bar, Clary outside
 * takes no hit at all */
static void hurt_player(ClcWorld *w, ClcPlayer *p, int half, int32_t from_x) {
    if (p->dead) return;
    if (w->on_foot) {
        if (w->cl.inv) return;
        die(w, p);
        return;
    }
    if (w->ship_inv) return;
    clc_hurt(p, half);
    w->ship_inv = CLC_SHIP_INV;
    w->ev |= CEV_HURT;
    /* knocked back */
    w->f.vx = w->f.x >= from_x ? 256 : -256;
    if (w->f.vy > -128) w->f.vy = -128;
    if (p->dead) die(w, p);
}

/* ---- starting ------------------------------------------------------------------ */

static void park_clary_beside(ClcWorld *w) {
    int sx = PX(w->f.x), floor_y = PX(w->f.y) + 4;
    int side = 1;
    /* step out to whichever side has floor */
    for (int k = 0; k < 2; k++) {
        int s = k == 0 ? 1 : -1;
        int cx = sx + s * 11;
        if (!clc_walker_blocked(&CLC_WALK_MAP, map_feet, w, cx * 256, floor_y * 256) &&
            map_feet(w, cx, floor_y, true)) { side = s; break; }
        side = -s;
    }
    w->cl = (ClcWalker){0};
    w->cl.x = (sx + side * 11) * 256;
    w->cl.y = floor_y * 256;
    w->cl.face = (int8_t)side;
    w->cl.ground = 1;
    w->cl.fall_from = w->cl.y;
    w->cl.inv = 20;
    w->on_foot = 1;
}

void clc_world_start(ClcWorld *w, ClcPlayer *p, bool on_foot) {
    (void)p;
    w->f = (ChmFlight){(w->spawn_c * CLC_T + CLC_T / 2) * 256, ((w->spawn_r + 1) * CLC_T - 4) * 256, 0, 0, 1};
    w->ship = SM_PARKED;
    w->on_foot = 0;
    w->enter = -1;
    w->timer = w->kind == WK_ESCAPE ? CLC_ESCAPE_T : CLC_TIMER;
    w->timer_on = w->kind == WK_ESCAPE;
    if (on_foot) park_clary_beside(w);
    else w->ship = SM_PILOT;
}

void clc_world_return(ClcWorld *w, int door) {
    const ClcDoor *d = &w->door[door];
    w->cl = (ClcWalker){0};
    w->cl.x = (d->c * CLC_T + CLC_T / 2) * 256;
    w->cl.y = ((d->r + 1) * CLC_T) * 256;
    w->cl.face = d->pad_c > d->c ? 1 : -1;
    w->cl.ground = 1;
    w->cl.fall_from = w->cl.y;
    w->cl.inv = 30;
    w->on_foot = 1;
    w->enter = -1;
}

/* ---- notes and the dash ---------------------------------------------------------- */

static void start_dash(ClcWorld *w) {
    if (w->timer_on || w->kind != WK_GEN) return;
    w->timer_on = 1;
    w->timer = CLC_TIMER;
    w->ev |= CEV_TIMER;
    if (w->gold_door < w->nd) w->door[w->gold_door].hidden = 0;
    for (int i = 0; i < w->ne; i++) {
        ClcEnt *e = &w->e[i];
        if (!e->on) continue;
        if (e->kind == EK_NOTE) e->kind = EK_PLUM;          /* the spare notes turn to plum coins */
        if (e->kind == EK_POD) e->flag = 1;                 /* the pods hatch */
    }
    for (int k = 0; k < w->nrings; k++) clc_ent_add(w, EK_RING, w->ring_c[k] * CLC_T + CLC_T, w->ring_r[k] * CLC_T + CLC_T);
}

static void take_note(ClcWorld *w, ClcPlayer *p, ClcEnt *e) {
    e->on = 0;
    w->notes++;
    clc_add_fuel(p, w->notes >= 9 ? 200 : 100);
    w->ev |= CEV_NOTE | CEV_FUEL;
    add_fx(w, PX(e->x), PX(e->y), FX_NOTE);
    if (w->notes >= CLC_NOTES_NEEDED) start_dash(w);
}

void clc_world_notes_done(ClcWorld *w, ClcPlayer *p) {
    for (int i = 0; i < w->ne && w->notes < CLC_NOTES_NEEDED; i++)
        if (w->e[i].on && w->e[i].kind == EK_NOTE) take_note(w, p, &w->e[i]);
}

/* ---- kills and coins ------------------------------------------------------------ */

static void drop_coins(ClcWorld *w, const ClcPlayer *p, int x, int y, int n) {
    n = clc_coin_drop(p, n);
    if (n <= 0) return;
    int i = clc_ent_add(w, EK_COIN, x, y);
    if (i < 0) return;
    w->e[i].a = (int16_t)n;
    w->e[i].vy = -256;
    w->e[i].vx = (int32_t)((w->t * 37) % 160) - 80;
}

static ClcEnt *head_of(ClcWorld *w, ClcEnt *e) {
    if (e->kind == EK_SEG && e->a >= 0 && e->a < w->ne && w->e[e->a].on) return &w->e[e->a];
    return e;
}

static void kill_ent(ClcWorld *w, ClcPlayer *p, ClcEnt *e, bool by_ship) {
    int x = PX(e->x), y = PX(e->y);
    w->ev |= CEV_KILL;
    w->kills++;
    add_fx(w, x, y, e->kind == EK_BIGBELLY || e->kind == EK_GRUB ? FX_BOOM : FX_POP);
    drop_coins(w, p, x, y, KIND[e->kind].coins * (e->champ ? 2 : 1));
    if (by_ship && clc_has(p, G_SIPHON)) {
        clc_add_fuel(p, 100);
        w->ev |= CEV_FUEL;
    }
    if (e->kind == EK_BLOB) {
        /* blobs come back where they started: a farm */
        e->flag = 1;
        e->t = 300;
        e->hp = KIND[EK_BLOB].hp;
        return;
    }
    e->on = 0;
    if (e->kind == EK_SNAKE || e->kind == EK_WORM) {
        int me = (int)(e - w->e);
        for (int i = 0; i < w->ne; i++)
            if (w->e[i].on && w->e[i].kind == EK_SEG && w->e[i].a == me) {
                w->e[i].on = 0;
                add_fx(w, PX(w->e[i].x), PX(w->e[i].y), FX_POP);
            }
    }
}

/* a hit on an enemy; returns true if it counted */
static bool hit_ent(ClcWorld *w, ClcPlayer *p, ClcEnt *e, int dmg, bool by_ship) {
    e = head_of(w, e);
    const Kind *k = &KIND[e->kind];
    if (!(k->flags & KF_FOE)) return false;
    if ((k->flags & KF_INVULN) || (by_ship && (k->flags & KF_SHIPPROOF))) {
        w->ev |= CEV_CLANG;
        return true;
    }
    if (e->kind == EK_BLOB && e->flag) return false;
    e->hp = (int16_t)(e->hp - dmg);
    w->ev |= CEV_HIT;
    if (e->hp <= 0) kill_ent(w, p, e, by_ship);
    return true;
}

static bool deadly(const ClcEnt *e) {
    if (!e->on) return false;
    if (!(KIND[e->kind].flags & KF_FOE)) return false;
    if (e->kind == EK_BLOB && e->flag) return false;
    if (e->kind == EK_SNAP && !e->flag) return false;      /* open: harmless until it snaps */
    if (e->kind == EK_JETFIRE) return false;                /* its flame is the danger */
    if (e->kind == EK_ACID && e->flag < 2) return false;    /* the pod itself is soft */
    if (e->kind == EK_LEECH) return true;
    return true;
}

/* ---- the ship ------------------------------------------------------------------------ */

static bool ship_slash_box(const ClcWorld *w, int side, int *x0, int *y0, int *x1, int *y1) {
    if (w->on_foot || w->slash_t <= SLASH_T - SLASH_ACTIVE) return false;
    int cx = PX(w->f.x), cy = PX(w->f.y);
    if (side >= 0) { *x0 = cx + 2; *x1 = cx + SLASH_REACH; }
    else { *x0 = cx - SLASH_REACH; *x1 = cx - 2; }
    *y0 = cy - 9;
    *y1 = cy + 9;
    return true;
}

static void slash_hits(ClcWorld *w, ClcPlayer *p) {
    for (int s = 0; s < 2; s++) {
        int side = s == 0 ? w->f.face : -w->f.face;
        if (s == 1 && !clc_has(p, G_TWIN)) break;
        int x0, y0, x1, y1;
        if (!ship_slash_box(w, side, &x0, &y0, &x1, &y1)) return;
        for (int i = 0; i < w->ne; i++) {
            ClcEnt *e = &w->e[i];
            if (!e->on || e->hitno == w->slash_no) continue;
            if (!(KIND[e->kind].flags & KF_FOE)) continue;
            int a0, b0, a1, b1;
            ent_box(e, &a0, &b0, &a1, &b1);
            if (!rects_overlap(x0, y0, x1 - x0 + 1, y1 - y0 + 1, a0, b0, a1 - a0 + 1, b1 - b0 + 1)) continue;
            e->hitno = w->slash_no;
            if (e->kind == EK_ACID && e->flag == 0) { hit_ent(w, p, e, 2, true); continue; }
            hit_ent(w, p, e, 2, true);
        }
        /* coin blocks break */
        for (int r = (y0) / CLC_T; r <= y1 / CLC_T; r++)
            for (int c = x0 / CLC_T; c <= x1 / CLC_T; c++)
                if (clc_map_tile(w, c * CLC_T, r * CLC_T) == MT_COIN) {
                    clc_set_tile(w, c, r, MT_AIR);
                    if (!w->sim) {
                        p->coins += clc_coin_drop(p, 10);
                        w->ev |= CEV_BREAK | CEV_COIN;
                        add_fx(w, c * CLC_T + 4, r * CLC_T + 4, FX_COINS);
                    }
                }
    }
}

static int nearest_foe(const ClcWorld *w, int32_t x, int32_t y, int range) {
    int best = -1, bd = range * range;
    for (int i = 0; i < w->ne; i++) {
        const ClcEnt *e = &w->e[i];
        if (!deadly(e) || (KIND[e->kind].flags & KF_INVULN) || e->kind == EK_SEG) continue;
        int dx = PX(e->x - x), dy = PX(e->y - y);
        if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; best = i; }
    }
    return best;
}

static void ship_weapons(ClcWorld *w, const ClcPlayer *p) {
    int f = w->f.face >= 0 ? 1 : -1;
    if (clc_has(p, G_SEEKER)) {
        int n = 0;
        for (int k = 0; k < CLC_SHOTS; k++) n += w->shot[k].on && w->shot[k].kind == SH_SEEKER;
        if (n < 2) shot_add(w, SH_SEEKER, w->f.x, w->f.y - 512, f * 256, -256, 300, 2);
    }
    if (clc_has(p, G_BOUNCE)) {
        int jit = (int)(w->t * 29 % 21) - 10;
        int a = (f > 0 ? 0 : 128) + jit;
        shot_add(w, SH_BEAM, w->f.x, w->f.y, clc_cos(a) * 5 * 256 / 127, clc_sin(a) * 5 * 256 / 127, 100, 2);
    }
    if (clc_has(p, G_SPIT)) shot_add(w, SH_SPIT, w->f.x + f * 1024, w->f.y, f * 900, 0, 26, 1);
}

static bool flat_floor_under(const ClcWorld *w, int x, int bottom) {
    /* both of the ship's feet over solid ground, and no ladder in the way */
    return clc_map_solid(w, x - 4, bottom) && clc_map_solid(w, x + 3, bottom) &&
           clc_map_tile(w, x - 4, bottom) != MT_GATE && clc_map_tile(w, x + 3, bottom) != MT_GATE;
}

static void ship_step(ClcWorld *w, ClcPlayer *p, unsigned held, unsigned pressed) {
    unsigned ctl = 0;
    if (held & BTN_LEFT) ctl |= CHF_LEFT;
    if (held & BTN_RIGHT) ctl |= CHF_RIGHT;
    bool thrust = (held & BTN_A) != 0;
    if (thrust) {
        if (p->fuel > 0) {
            p->drip = (uint8_t)(p->drip + CLC_BURN_NUM);
            while (p->drip >= CLC_BURN_DEN) {
                p->drip = (uint8_t)(p->drip - CLC_BURN_DEN);
                p->fuel = (int16_t)imax(0, p->fuel - 1);
            }
            ctl |= CHF_THRUST;
        } else if (p->coins > 0) {
            /* the tank is dry: coins go into the burner instead */
            ctl |= CHF_THRUST;
            if (++p->burn >= CLC_COIN_BURN) {
                p->burn = 0;
                if (!w->sim) {
                    p->coins--;
                    add_fx(w, PX(w->f.x), PX(w->f.y) + 6, FX_COINS);
                }
            }
        } else {
            w->ev |= CEV_DRY;
        }
    }
    if (w->slash_t) ctl |= CHF_SLASHING;
    w->dive = (held & BTN_DOWN) != 0;
    const ChmFlightTune *tune = w->dive ? &DIVE_TUNE : &CLC_TUNE;
    chm_flight_control(&w->f, tune, ctl);
    /* leeches weigh the ship down */
    if (w->leeches) w->f.vy = imin(tune->max_down, w->f.vy + w->leeches * 5);

    /* the slash */
    if (w->slash_cd) w->slash_cd--;
    if (w->slash_t) w->slash_t--;
    if ((pressed & BTN_B) && !w->slash_cd) {
        w->slash_t = SLASH_T;
        w->slash_cd = SLASH_CD;
        w->slash_no++;
        w->ev |= CEV_SLASH;
        ship_weapons(w, p);
    }

    /* outside forces: wind and magnets */
    for (int i = 0; i < w->ne; i++) {
        const ClcEnt *e = &w->e[i];
        if (!e->on || e->kind != EK_WINDDRONE) continue;
        int dx = PX(w->f.x - e->x), dy = PX(w->f.y - e->y);
        if (dx * e->dir > 0 && dx * e->dir < 88 && iabs(dy) < 20) w->f.vx = iclamp(w->f.vx + e->dir * 12, -640, 640);
    }
    if (!w->magnets_off && w->region == RG_SPIRE) {
        int sc = PX(w->f.x) / CLC_T, sr = PX(w->f.y) / CLC_T;
        for (int r = sr - 9; r <= sr + 9; r++)
            for (int c = sc - 9; c <= sc + 9; c++) {
                int t = clc_map_tile(w, c * CLC_T, r * CLC_T);
                if (t != MT_MAGNET_PULL && t != MT_MAGNET_PUSH) continue;
                int dx = c * CLC_T + 4 - PX(w->f.x), dy = r * CLC_T + 4 - PX(w->f.y);
                int d = clc_isqrt(dx * dx + dy * dy);
                if (d < 4 || d > 72) continue;
                int s = t == MT_MAGNET_PULL ? 1 : -1, pull = 14 * (72 - d) / 72 + 2;
                w->f.vx += s * dx * pull / d;
                w->f.vy += s * dy * pull / d;
            }
    }

    /* move */
    int32_t vx = w->f.vx, vy = w->f.vy;
    int hit = chm_flight_move(&w->f, &CLC_TUNE, clc_map_solid, w);
    int x = PX(w->f.x), y = PX(w->f.y);
    bool grounded = (hit & CHF_HIT_Y) && vy > 0;
    if (grounded && w->air_t < 12) {
        /* still sitting where it was parked: not a landing until it has been up */
        w->air_t = 0;
    } else if (grounded) {
        w->air_t = 0;
        bool flat = flat_floor_under(w, x, y + 4) || flat_floor_under(w, x, y + 5);
        /* a landing pad takes a little more */
        bool pad = clc_map_tile(w, x - 4, y + 5) == MT_PAD || clc_map_tile(w, x + 3, y + 5) == MT_PAD ||
                   clc_map_tile(w, x - 4, y + 4) == MT_PAD || clc_map_tile(w, x + 3, y + 4) == MT_PAD;
        int land_vy = pad ? CLC_PAD_VY : CLC_LAND_VY, land_vx = pad ? CLC_PAD_VX : CLC_LAND_VX;
        if (p->fuel <= 0 && p->coins <= 0) {
            /* nothing left to burn: down it goes, and that is the end */
            w->hurt_by = 203;
            die(w, p);
            return;
        }
        if (flat && vy <= land_vy && iabs(vx) <= land_vx) {
            /* a landing: set down, and Clary hops out */
            if (!clc_map_solid(w, x - 4, y + 4) && !clc_map_solid(w, x + 3, y + 4)) y++;
            w->f.y = (int32_t)y * 256;
            w->f.vx = w->f.vy = 0;
            w->ship = SM_PARKED;
            w->landed = 1;
            w->ev |= CEV_LAND;
            w->slash_t = 0;
            if (w->leeches) {
                for (int k = 0; k < w->leeches; k++) add_fx(w, x, y - 4, FX_POP);
                w->leeches = 0;
            }
            park_clary_beside(w);
            return;
        }
        if (vy > land_vy || iabs(vx) > land_vx) {
            /* a failed landing: a point off the bar */
            w->hurt_by = 201;
            hurt_player(w, p, 2, w->f.x);
            w->ev |= CEV_BADLAND;
            add_fx(w, x, y + 4, FX_DUST);
        }
    } else if (hit) {
        int sp = (hit & CHF_HIT_X) ? iabs(vx) : iabs(vy);
        if ((hit & CHF_HIT_X) && (hit & CHF_HIT_Y)) sp = imax(iabs(vx), iabs(vy));
        if (sp > CLC_WALL_HURT) {
            w->hurt_by = 200;
            hurt_player(w, p, 2, w->f.x - (vx > 0 ? 2560 : -2560));
            w->ev |= CEV_BUMP;
            add_fx(w, x + (vx > 0 ? 4 : vx < 0 ? -4 : 0), y + (vy > 0 ? 4 : vy < 0 ? -4 : 0), FX_DUST);
        }
    }
    if (!grounded && w->air_t < 255) w->air_t++;
    if (w->ship_inv) w->ship_inv--;

    /* where a landing would set down, for the safe-landing mark */
    w->landing_icon = -1;
    for (int dy = 4; dy < 9 * CLC_T; dy++) {
        if (clc_map_solid(w, x - 4, y + dy) || clc_map_solid(w, x + 3, y + dy)) {
            if (flat_floor_under(w, x, y + dy)) w->landing_icon = (int16_t)((y + dy) / CLC_T);
            break;
        }
    }
    slash_hits(w, p);
}

/* ---- Clary outside ---------------------------------------------------------------------- */

static int pistol_shots(const ClcShot *sh, int n) {
    int k = 0;
    for (int i = 0; i < n; i++) k += sh[i].on && (sh[i].kind == SH_PISTOL || sh[i].kind == SH_BIG);
    return k;
}

static void fire(ClcWorld *w, ClcPlayer *p, bool tap, unsigned held) {
    ClcWalker *c = &w->cl;
    int f = c->face, aim = 0;
    if (c->ground && c->crouch) aim = 0;
    else if (held & BTN_UP) aim = 1;
    else if (!c->ground && (held & BTN_DOWN)) aim = 2;
    int32_t x = c->x + f * 768, y = c->y - (c->crouch ? 3 : 5) * 256;
    int32_t vx = f * 768, vy = 0;
    if (aim == 1) { x = c->x; y = c->y - 9 * 256; vx = 0; vy = -768; }
    if (aim == 2) { x = c->x; y = c->y; vx = 0; vy = 768; }
    int kind = SH_PISTOL, dmg = 1, life = 32;
    if (tap && clc_has(p, G_BIGBANG) && p->coins > 0) {
        kind = SH_BIG;
        dmg = 3;
        if (!w->sim) p->coins--;
    }
    shot_add(w, kind, x, y, vx, vy, life, dmg);
    if (clc_has(p, G_FAN)) {
        /* two short shots fanned out beside it */
        int32_t ay = vy;
        if (aim == 0) {
            shot_add(w, SH_FAN, x, y, vx * 3 / 4, -320, 12, 1);
            shot_add(w, SH_FAN, x, y, vx * 3 / 4, 320, 12, 1);
        } else {
            shot_add(w, SH_FAN, x, y, -320, ay * 3 / 4, 12, 1);
            shot_add(w, SH_FAN, x, y, 320, ay * 3 / 4, 12, 1);
        }
    }
    w->ev |= CEV_SHOOT;
}

static void foot_step(ClcWorld *w, ClcPlayer *p, unsigned held, unsigned pressed) {
    ClcWalker *c = &w->cl;
    int cx = PX(c->x), cy = PX(c->y);
    /* UP at the ship climbs in; UP at a door goes through it */
    if ((pressed & BTN_UP) && c->ground) {
        if (w->ship == SM_PARKED && iabs(cx - PX(w->f.x)) <= 13 && iabs(cy - (PX(w->f.y) + 4)) <= 2) {
            w->on_foot = 0;
            w->ship = SM_PILOT;
            w->air_t = 0;
            w->f.vx = w->f.vy = 0;
            w->f.face = c->face;
            w->ev |= CEV_BOARD;
            return;
        }
        for (int k = 0; k < w->nd; k++) {
            ClcDoor *d = &w->door[k];
            if (d->hidden) continue;
            int dx = d->c * CLC_T + CLC_T / 2;
            if (iabs(cx - dx) > 6 || cy != (d->r + 1) * CLC_T) continue;
            bool gold = d->type == DT_GOLD;
            if (d->shut) { w->ev |= CEV_CLANG; return; }
            if (w->timer_on && !gold && w->kind == WK_GEN) { w->ev |= CEV_CLANG; return; }
            if (!w->timer_on && gold && w->kind == WK_GEN) { w->ev |= CEV_CLANG; return; }
            if (d->type == DT_YELLOW && !d->used) {
                if (!p->key) { w->ev |= CEV_CLANG; return; }
                if (!w->sim) { p->key = 0; d->used = 1; }
                w->ev |= CEV_OPEN;
            }
            w->enter = (int8_t)k;
            w->ev |= CEV_DOOR;
            return;
        }
    }
    /* the homing charm: a second jump in the air calls the ship */
    if ((pressed & BTN_A) && !c->ground && !c->ladder && clc_has(p, G_CHARM)) {
        int32_t sx = c->x, sy = c->y - 5 * 256;
        if (!chm_flight_blocked(4, clc_map_solid, w, sx, sy)) {
            w->f = (ChmFlight){sx, sy, c->vx / 2, imin(0, c->vy / 2), c->face};
            w->on_foot = 0;
            w->ship = SM_PILOT;
            w->air_t = 255;
            w->charm_jump = 1;
            w->ev |= CEV_BOARD;
            return;
        }
    }
    unsigned wk = 0;
    clc_walk(c, &CLC_WALK_MAP, held, pressed, wk, map_feet, map_ladder, w);
    if (c->landed && c->drop > CLC_FALL_DEATH) {
        die(w, p);
        return;
    }
    /* the pistol: three shots in the air at most; held fires steadily, a
     * fresh press as soon as there is room */
    if ((held & BTN_B) && pistol_shots(w->shot, CLC_SHOTS) < CLC_SHOTS_OUT) {
        bool tap = (pressed & BTN_B) != 0;
        if (tap || c->shot_cd == 0) {
            fire(w, p, tap, held);
            c->shot_cd = 14;
        }
    }
    if (w->ship_inv) w->ship_inv--;
}

/* ---- the enemies -------------------------------------------------------------------------- */

static bool solid_box(const ClcWorld *w, int x0, int y0, int x1, int y1) {
    return clc_map_solid(w, x0, y0) || clc_map_solid(w, x1, y0) || clc_map_solid(w, x0, y1) || clc_map_solid(w, x1, y1);
}

/* move e by its speed, turning back off walls; returns the hit bits */
static int ent_move(ClcWorld *w, ClcEnt *e, bool through) {
    int hit = 0;
    int hw = KIND[e->kind].hw, hh = KIND[e->kind].hh;
    int32_t nx = e->x + e->vx;
    if (through || !solid_box(w, PX(nx) - hw, PX(e->y) - hh, PX(nx) + hw - 1, PX(e->y) + hh - 1)) e->x = nx;
    else hit |= 1;
    int32_t ny = e->y + e->vy;
    if (through || !solid_box(w, PX(e->x) - hw, PX(ny) - hh, PX(e->x) + hw - 1, PX(ny) + hh - 1)) e->y = ny;
    else hit |= 2;
    return hit;
}

static void foe_shoot(ClcWorld *w, ClcEnt *e, int kind, int speed, int life) {
    int32_t vx, vy;
    clc_aim(e->x, e->y, player_x(w), player_y(w), speed, &vx, &vy);
    shot_add(w, kind, e->x, e->y, vx, vy, life, 2);
    w->ev |= CEV_FOESHOT;
}

static int dist_to_player(const ClcWorld *w, const ClcEnt *e) {
    int dx = PX(player_x(w) - e->x), dy = PX(player_y(w) - e->y);
    return clc_isqrt(dx * dx + dy * dy);
}

static void ent_step(ClcWorld *w, ClcPlayer *p, ClcEnt *e) {
    e->t++;
    int d = dist_to_player(w, e);
    int32_t px = player_x(w), py = player_y(w);
    switch (e->kind) {
    case EK_FLITTER:
        e->vx = e->dir * 160;
        e->vy = clc_sin(e->t * 3) * 2;
        if (ent_move(w, e, false) & 1) e->dir = (int8_t)-e->dir;
        if (iabs(PX(e->x) - e->hx) > 64) e->dir = (int8_t)(PX(e->x) > e->hx ? -1 : 1);
        break;
    case EK_CREEPER:
    case EK_GRUB: {
        /* along its floor (dir 1) or its ceiling (dir -1), back at an edge */
        int sp = e->kind == EK_GRUB ? 70 : 110;
        if (!e->b) e->b = 1;
        int hw = KIND[e->kind].hw, hh = KIND[e->kind].hh;
        int nx = PX(e->x + e->b * sp);
        int lead = nx + e->b * hw;
        int under = e->dir >= 0 ? PX(e->y) + hh + 1 : PX(e->y) - hh - 2;
        if (clc_map_solid(w, lead, PX(e->y)) || !clc_map_solid(w, lead, under)) e->b = (int16_t)-e->b;
        else e->x += e->b * sp;
        break;
    }
    case EK_SNAP:
        if (e->flag) {
            if (--e->a <= 0) e->flag = 0;
        } else if (iabs(PX(px - e->x)) < 12 && PX(py - e->y) < 4 && PX(py - e->y) > -28) {
            e->flag = 1;
            e->a = 70;
        }
        break;
    case EK_FIREDRONE:
        e->vx = e->dir * 100;
        e->vy = clc_sin(e->t * 2) * 1;
        if (ent_move(w, e, false) & 1) e->dir = (int8_t)-e->dir;
        if (iabs(PX(e->x) - e->hx) > 56) e->dir = (int8_t)(PX(e->x) > e->hx ? -1 : 1);
        if (e->t % 90 == 0 && iabs(PX(px - e->x)) < 40 && py > e->y && d < 110)
            shot_add(w, SH_FIREBALL, e->x, e->y + 1024, 0, 128, 120, 2), w->ev |= CEV_FOESHOT;
        break;
    case EK_WINDDRONE:
        e->y = e->hy * 256 + clc_sin(e->t * 2) * 3;
        if (e->t % 240 == 0) e->dir = (int8_t)-e->dir;
        break;
    case EK_LEECH:
        if (d < 100 && !w->on_foot) {
            clc_aim(e->x, e->y, px, py, 120, &e->vx, &e->vy);
            ent_move(w, e, false);
        } else if (d < 70) {
            clc_aim(e->x, e->y, px, py, 90, &e->vx, &e->vy);
            ent_move(w, e, false);
        }
        break;
    case EK_EYE:
        if (e->t % 100 == 0 && d < 120) {
            foe_shoot(w, e, SH_PELLET, 300, 160);
            if (e->champ) {
                /* a gold one fires three */
                for (int k = -1; k <= 1; k += 2) {
                    int32_t vx, vy;
                    clc_aim(e->x, e->y, px + k * 4096, py, 300, &vx, &vy);
                    shot_add(w, SH_PELLET, e->x, e->y, vx, vy, 160, 2);
                }
            }
        }
        break;
    case EK_WALLEYE:
        /* an eye in the wall that drinks the ship's fuel close by */
        e->flag = !w->on_foot && d < 22;
        if (e->flag && p->fuel > 0 && (e->t & 1)) {
            p->fuel--;
            w->ev |= CEV_DRY;
        }
        break;
    case EK_NEST: {
        /* it lets out flitters, two at a time, while the player is near */
        if (d < 130 && e->t % 200 == 100) {
            int n = 0, me = (int)(e - w->e);
            for (int i = 0; i < w->ne; i++) n += w->e[i].on && w->e[i].kind == EK_FLITTER && w->e[i].link == me + 1;
            if (n < 2) {
                int j = clc_ent_add(w, EK_FLITTER, PX(e->x), PX(e->y) - e->dir * 6);
                if (j >= 0) {
                    w->e[j].link = (uint8_t)(me + 1);
                    w->e[j].dir = (int8_t)(px > e->x ? 1 : -1);
                }
            }
        }
        break;
    }
    case EK_FALLER:
        /* a shudder first, then the drop */
        if (e->a < 20) { e->a++; e->x = e->hx * 256 + ((e->a & 2) ? 256 : -256); break; }
        e->x = e->hx * 256;
        e->vy = imin(900, e->vy + 30);
        e->y += e->vy;
        if (clc_map_solid(w, PX(e->x), PX(e->y) + 4)) {
            e->on = 0;
            add_fx(w, PX(e->x), PX(e->y), FX_DUST);
            w->ev |= CEV_BREAK;
        }
        break;
    case EK_BMISSILE:
        clc_aim(e->x, e->y, px, py, 130, &e->vx, &e->vy);
        e->x += e->vx;
        e->y += e->vy;
        if (++e->b > 360 || clc_map_solid(w, PX(e->x), PX(e->y))) {
            e->on = 0;
            add_fx(w, PX(e->x), PX(e->y), FX_POP);
        }
        break;
    case EK_ACID:
        if (e->flag == 0 && d < 28) { e->flag = 1; e->a = 30; }
        else if (e->flag == 1 && --e->a <= 0) {
            /* it bursts: acid all round */
            e->flag = 2;
            for (int k = 0; k < 8; k++)
                shot_add(w, SH_ACID, e->x, e->y, clc_cos(k * 32) * 2, clc_sin(k * 32) * 2, 22, 2);
            w->ev |= CEV_BLAST;
            add_fx(w, PX(e->x), PX(e->y), FX_POP);
            e->on = 0;
        }
        break;
    case EK_BLOB:
        if (e->flag) {
            if (e->t >= 1 && --e->t == 0) {
                e->flag = 0;
                e->x = e->hx * 256;
                e->y = e->hy * 256;
                e->vx = e->vy = 0;
            }
            break;
        }
        if (!e->vx) e->vx = e->dir * 120;
        e->vy = imin(400, e->vy + 12);
        {
            int h = ent_move(w, e, false);
            if (h & 1) e->vx = -e->vx;
            if (h & 2) e->vy = e->vy > 0 ? -380 : 0;
        }
        break;
    case EK_SENTRY:
        /* shoots the ship only: it stands down while Clary is out */
        if (!w->on_foot && !w->sentries_off && e->t % 70 == 0 && d < 140) foe_shoot(w, e, SH_PELLET, 360, 160);
        break;
    case EK_SNAKE:
        e->vx = e->dir * 140;
        e->vy = clc_sin(e->t * 4) * 3;
        if (ent_move(w, e, false) & 1) e->dir = (int8_t)-e->dir;
        if (iabs(PX(e->x) - e->hx) > 90) e->dir = (int8_t)(PX(e->x) > e->hx ? -1 : 1);
        break;
    case EK_WORM:
        if (!e->vx && !e->vy) { e->vx = e->dir * 150; e->vy = 110; }
        {
            int h = ent_move(w, e, false);
            if (h & 1) e->vx = -e->vx;
            if (h & 2) e->vy = -e->vy;
        }
        break;
    case EK_SEG: {
        /* follow the one ahead, a few pixels behind */
        const ClcEnt *l = &w->e[e->link];
        if (!l->on) { e->on = 0; break; }
        int dx = PX(l->x - e->x), dy = PX(l->y - e->y), dd = clc_isqrt(dx * dx + dy * dy);
        if (dd > 7) {
            e->x += (int32_t)((int64_t)(dd - 7) * 256 * dx / dd);
            e->y += (int32_t)((int64_t)(dd - 7) * 256 * dy / dd);
        }
        break;
    }
    case EK_CRAB:
        if (e->flag == 0) {
            e->y = e->hy * 256 + clc_sin(e->t * 2) * 6;
            int side = PX(px - e->x) * e->dir;
            if (side > 0 && side < 52 && iabs(PX(py - e->y)) < 20 && e->t % 30 == 0) {
                e->flag = 1;
                e->a = 0;
            }
        } else {
            /* a leap out and back */
            e->a++;
            int out = e->a < 20 ? e->a : 40 - e->a;
            e->x = e->hx * 256 + e->dir * out * 2 * 256 / 2;
            if (e->a >= 40) { e->flag = 0; e->x = e->hx * 256; }
        }
        break;
    case EK_FAKENOTE:
        if (d < 26) {
            e->kind = EK_GHOST;
            e->hp = KIND[EK_GHOST].hp;
            w->ev |= CEV_FOESHOT;
        }
        break;
    case EK_GHOST:
        clc_aim(e->x, e->y, px, py, 150, &e->vx, &e->vy);
        e->x += e->vx;
        e->y += e->vy;
        break;
    case EK_JETFIRE:
        /* on for a second, off for a second and a half */
        e->flag = (e->t % 150) < 60;
        if (e->flag) {
            int x0 = e->dir > 0 ? PX(e->x) + 4 : PX(e->x) - 28, x1 = x0 + 24;
            int a0, b0, a1, b1;
            player_box(w, &a0, &b0, &a1, &b1);
            if (rects_overlap(x0, PX(e->y) - 4, x1 - x0, 8, a0, b0, a1 - a0 + 1, b1 - b0 + 1)) { w->hurt_by = EK_JETFIRE; hurt_player(w, p, 4, e->x); }
        }
        break;
    case EK_BOOMER:
        if (e->t % 150 == 0 && d < 120) {
            ClcShot *s = shot_add(w, SH_BOOMERANG, e->x, e->y, 0, 0, 150, 2);
            if (s) {
                clc_aim(e->x, e->y, px, py, 360, &s->vx, &s->vy);
                s->target = (int16_t)(e - w->e);
            }
            w->ev |= CEV_FOESHOT;
        }
        break;
    case EK_CHASER:
        if (d < 110) {
            clc_aim(e->x, e->y, px, py, 180, &e->vx, &e->vy);
            ent_move(w, e, false);
            if (e->t % 130 == 0) {
                for (int k = e->champ ? -2 : -1; k <= (e->champ ? 2 : 1); k++) {
                    int32_t vx, vy;
                    clc_aim(e->x, e->y, px + k * 2048, py, 340, &vx, &vy);
                    shot_add(w, SH_FIREBALL, e->x, e->y, vx, vy, 40, 2);
                }
                w->ev |= CEV_FOESHOT;
            }
        } else {
            e->vx = e->dir * 80;
            e->vy = 0;
            if (ent_move(w, e, false) & 1) e->dir = (int8_t)-e->dir;
        }
        break;
    case EK_BIGBELLY:
        if (e->t % 120 == 0) {
            e->vx = (clc_sin(e->t + e->hx) > 0 ? 1 : -1) * 70;
            e->vy = (clc_cos(e->t * 3 + e->hy) > 0 ? 1 : -1) * 40;
        }
        {
            int h = ent_move(w, e, false);
            if (h & 1) e->vx = -e->vx;
            if (h & 2) e->vy = -e->vy;
        }
        if (iabs(PX(e->x) - e->hx) > 80) e->vx = PX(e->x) > e->hx ? -70 : 70;
        if (iabs(PX(e->y) - e->hy) > 40) e->vy = PX(e->y) > e->hy ? -40 : 40;
        /* a load of slow missiles when the player is near */
        if (d < 110 && e->t % 180 == 90) {
            for (int k = 0; k < 2; k++) clc_ent_add(w, EK_BMISSILE, PX(e->x) + (k ? 8 : -8), PX(e->y) + 6);
            w->ev |= CEV_FOESHOT;
        }
        break;
    case EK_POD:
        if (e->flag && e->t % 120 == 0 && d < 140) foe_shoot(w, e, SH_PELLET, 320, 160);
        break;
    case EK_LATE:
        clc_aim(e->x, e->y, px, py, 560, &e->vx, &e->vy);
        e->x += e->vx;
        e->y += e->vy;
        break;
    case EK_COIN:
        e->vy = imin(512, e->vy + 20);
        {
            int h = ent_move(w, e, false);
            if (h & 2) { e->vy = 0; e->vx = 0; }
            if (h & 1) e->vx = 0;
        }
        if (++e->b > 900) e->on = 0;
        break;
    case EK_RING:
        break;
    default: break;
    }
}

/* ---- shots -------------------------------------------------------------------------- */

static bool player_shot(int k) { return k <= SH_SPIT; }

static void shots_step(ClcWorld *w, ClcPlayer *p) {
    int a0, b0, a1, b1;
    player_box(w, &a0, &b0, &a1, &b1);
    for (int k = 0; k < CLC_SHOTS; k++) {
        ClcShot *s = &w->shot[k];
        if (!s->on) continue;
        if (s->life == 0 || --s->life == 0) { s->on = 0; continue; }
        /* homing and returning */
        if (s->kind == SH_SEEKER) {
            int t = nearest_foe(w, s->x, s->y, 140);
            if (t >= 0) {
                int32_t vx, vy;
                clc_aim(s->x, s->y, w->e[t].x, w->e[t].y, 420, &vx, &vy);
                s->vx += (vx - s->vx) / 10;
                s->vy += (vy - s->vy) / 10;
            }
        } else if (s->kind == SH_BOOMERANG) {
            /* it goes when the boomer that threw it goes; on the way back it
             * homes on the thrower */
            const ClcEnt *e = s->target >= 0 && s->target < w->ne ? &w->e[s->target] : NULL;
            if (!e || !e->on || e->kind != EK_BOOMER) { s->on = 0; continue; }
            if (s->life < 75) {
                int32_t vx, vy;
                clc_aim(s->x, s->y, e->x, e->y, 420, &vx, &vy);
                s->vx = vx;
                s->vy = vy;
                if (iabs(PX(s->x - e->x)) < 5 && iabs(PX(s->y - e->y)) < 5) s->on = 0;
            }
        } else if (s->kind == SH_FIREBALL && s->vx == 0) {
            s->vy = imin(640, s->vy + 10);
        }
        int32_t nx = s->x + s->vx, ny = s->y + s->vy;
        bool through = s->kind == SH_BOOMERANG || (s->kind <= SH_BIG && clc_has(p, G_GHOST));
        if (!through && clc_map_solid(w, PX(nx), PX(ny))) {
            if (s->kind == SH_BEAM && s->bounces < 3) {
                /* bounce off whichever face it struck */
                if (clc_map_solid(w, PX(nx), PX(s->y))) s->vx = -s->vx;
                if (clc_map_solid(w, PX(s->x), PX(ny))) s->vy = -s->vy;
                s->bounces++;
                continue;
            }
            if ((s->kind == SH_PISTOL || s->kind == SH_BIG) && clc_has(p, G_CREEPER)) {
                /* a creeper cap crawls on along the surface */
                ClcShot *c = shot_add(w, SH_FRAG, s->x, s->y, s->vx > 0 ? -200 : 200, 0, 60, 1);
                (void)c;
            }
            s->on = 0;
            add_fx(w, PX(s->x), PX(s->y), FX_SPARK);
            continue;
        }
        if (s->kind == SH_FRAG) {
            /* crawls along the floor below it */
            if (!clc_map_solid(w, PX(nx), PX(ny) + 3)) s->vy = 200;
            else s->vy = 0;
        }
        s->x = nx;
        s->y = ny;
        int sx = PX(s->x), sy = PX(s->y), r = s->kind == SH_BIG ? 3 : 2;
        if (player_shot(s->kind)) {
            for (int i = 0; i < w->ne; i++) {
                ClcEnt *e = &w->e[i];
                if (!e->on || !(KIND[e->kind].flags & KF_FOE)) continue;
                if (e->kind == EK_BLOB && e->flag) continue;
                int x0, y0, x1, y1;
                ent_box(e, &x0, &y0, &x1, &y1);
                if (sx + r < x0 || sx - r > x1 || sy + r < y0 || sy - r > y1) continue;
                bool ship = s->kind >= SH_SEEKER;
                if (hit_ent(w, p, e, s->dmg, ship)) {
                    s->on = 0;
                    add_fx(w, sx, sy, FX_HIT);
                    break;
                }
            }
        } else {
            if (sx + 2 >= a0 && sx - 2 <= a1 && sy + 2 >= b0 && sy - 2 <= b1) {
                w->hurt_by = (uint8_t)(100 + s->kind);
                hurt_player(w, p, 4, s->x);
                if (s->kind != SH_BOOMERANG) s->on = 0;
            }
        }
    }
}

/* ---- touching things ------------------------------------------------------------------ */

static void touches(ClcWorld *w, ClcPlayer *p) {
    int a0, b0, a1, b1;
    player_box(w, &a0, &b0, &a1, &b1);
    bool magnet = clc_has(p, G_MAGNET);
    int32_t px = player_x(w), py = player_y(w);
    for (int i = 0; i < w->ne; i++) {
        ClcEnt *e = &w->e[i];
        if (!e->on) continue;
        int k = e->kind;
        if ((k == EK_COIN || k == EK_PLUM) || (magnet && k == EK_NOTE)) {
            /* coins drift in from close by; the Magnet pulls from further, notes too */
            int dx = PX(px - e->x), dy = PX(py - e->y), reach = magnet ? 44 : 16;
            if (dx * dx + dy * dy < reach * reach) {
                int32_t vx, vy;
                clc_aim(e->x, e->y, px, py, 300, &vx, &vy);
                e->x += vx;
                e->y += vy;
            }
        }
        int x0, y0, x1, y1;
        ent_box(e, &x0, &y0, &x1, &y1);
        if (!rects_overlap(a0, b0, a1 - a0 + 1, b1 - b0 + 1, x0, y0, x1 - x0 + 1, y1 - y0 + 1)) continue;
        switch (k) {
        case EK_NOTE: take_note(w, p, e); break;
        case EK_PLUM:
            e->on = 0;
            p->coins += 25;
            w->ev |= CEV_COIN;
            add_fx(w, PX(e->x), PX(e->y), FX_COINS);
            break;
        case EK_COIN:
            e->on = 0;
            p->coins += e->a;
            w->ev |= CEV_COIN;
            add_fx(w, PX(e->x), PX(e->y), FX_COINS);
            break;
        case EK_RING:
            e->on = 0;
            clc_add_fuel(p, 200);
            w->ring_pause = CLC_RING_PAUSE;
            w->ev |= CEV_RING | CEV_FUEL;
            add_fx(w, PX(e->x), PX(e->y), FX_FUEL);
            break;
        case EK_FUELCAN:
            e->on = 0;
            clc_add_fuel(p, 500);
            w->ev |= CEV_FUEL;
            add_fx(w, PX(e->x), PX(e->y), FX_FUEL);
            break;
        case EK_LATE:
        case EK_FALLER:
            /* the latecomers, and a falling block: no bar saves her */
            w->hurt_by = (uint8_t)k;
            die(w, p);
            return;
        case EK_LEECH:
            if (!w->on_foot && !w->ship_inv) {
                /* it latches on */
                e->on = 0;
                if (w->leeches < 4) w->leeches++;
                w->ev |= CEV_HIT;
                break;
            }
            if (w->on_foot) hurt_player(w, p, 2, e->x);
            break;
        default:
            if (deadly(e)) { w->hurt_by = (uint8_t)k; hurt_player(w, p, KIND[k].dmg, e->x); }
            break;
        }
        if (p->dead) return;
    }
}

/* ---- the clock ---------------------------------------------------------------------- */

static void clock_step(ClcWorld *w, ClcPlayer *p) {
    (void)p;
    if (!w->timer_on) return;
    if (w->ring_pause) { w->ring_pause--; return; }
    if (w->timer > 0) {
        w->timer--;
        if (w->timer == 0) {
            w->late_on = 1;
            w->ev |= CEV_LATE;
        }
        return;
    }
    /* too late: the latecomers pour in from the edges of the view */
    if (w->t % 12 == 0) {
        int n = 0;
        for (int i = 0; i < w->ne; i++) n += w->e[i].on && w->e[i].kind == EK_LATE;
        if (n < 24) {
            int a = (int)((w->t * 53) & 255);
            int x = PX(player_x(w)) + clc_cos(a) * 190 / 127, y = PX(player_y(w)) + clc_sin(a) * 120 / 127;
            clc_ent_add(w, EK_LATE, x, y);
        }
    }
}

/* the Cellars' loose blocks drop when the player passes under them */
static void loose_blocks(ClcWorld *w) {
    int x = PX(player_x(w)), y = PX(player_y(w));
    int r0 = y / CLC_T;
    /* the columns right over her (or the ship) */
    for (int cc = (x - 4) / CLC_T; cc <= (x + 3) / CLC_T; cc++) {
        if (cc < 0 || cc >= w->w) continue;
        for (int r = r0 - 1; r >= 0 && r >= r0 - 8; r--) {
            int t = w->tile[r][cc];
            if (t == MT_AIR || t == MT_LADDER) continue;
            if (t == MT_LOOSE) {
                w->tile[r][cc] = MT_AIR;
                w->ver++;
                int i = clc_ent_add(w, EK_FALLER, cc * CLC_T + 4, r * CLC_T + 4);
                if (i >= 0) w->e[i].vy = 0;
                w->ev |= CEV_BREAK;
            }
            break;
        }
    }
}

/* ---- a step --------------------------------------------------------------------------- */

void clc_world_step(ClcWorld *w, ClcPlayer *p, unsigned buttons) {
    unsigned pressed = buttons & ~(unsigned)w->prev;
    w->ev = 0;
    w->nfx = 0;
    w->enter = -1;
    w->landed = 0;
    w->charm_jump = 0;
    if (!p->dead) {
        if (w->on_foot) foot_step(w, p, buttons, pressed);
        else if (w->ship == SM_PILOT) ship_step(w, p, buttons, pressed);
    }
    w->prev = (uint8_t)buttons;
    if (w->enter >= 0) return; /* through a door: the world waits */
    if (w->region == RG_CELLARS && !p->dead) loose_blocks(w);
    for (int i = 0; i < w->ne; i++)
        if (w->e[i].on) ent_step(w, p, &w->e[i]);
    shots_step(w, p);
    if (!p->dead) touches(w, p);
    clock_step(w, p);
    if (w->kind == WK_ESCAPE && !p->dead && !w->on_foot && PX(w->f.y) < w->exit_y) w->ev |= CEV_ESCAPED;
    w->t++;
}
