/* WET PAINT - the course simulation: lanes, cars, foes, garages, power-ups
 * and paint. Every rule and where it comes from is in docs/games/04-wet-paint.md.
 * Positions are in 1/16 pixel; a mover is a tile, a heading and how far it
 * has gone from that tile's centre towards the next one. */
#include "wetpaint.h"

WpSim wp;
const int WP_DX[4] = {1, 0, -1, 0}, WP_DY[4] = {0, 1, 0, -1};
static const int PAD[4] = {BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_UP};

/* ---- feel: speeds in 1/16 pixel per frame, times in frames ---------------- */
#define CRUISE 16          /* your car: a pixel a frame, five tiles a second */
#define BRAKE_MIN 5        /* holding the brake slows you to this */
#define ACCEL 1            /* back up to cruising by itself */
#define ACCEL_HELD 2       /* quicker with the pad pushed the way you're going */
#define BRAKE 2
#define BOOST_TIME 60      /* a boost lasts a second; another one on top goes faster */
static const int BOOST_SPD[4] = {0, 32, 40, 48};
#define STUN_TIME 54       /* the car crumples, then fixes itself */
#define AFTER_STUN 40      /* and can't be hit again straight away */
#define SPIN_SPD 40        /* a jelly spin-out slides to the wall */
#define GUN_TIME 300       /* sprinkler and tack shooter: 5 s */
#define GUN_EVERY 12       /* a shot each way five times a second */
#define SHOT_SPD 64
#define FREEZE_TIME 450    /* the freeze pop: 7.5 s */
#define HELPER_SPD 12
#define HELPER_SAFE 60     /* a new helper can't be knocked out for a second */
#define BOLLARD_EVERY 150  /* the bollards rise and sink every 2.5 s */
#define GARAGE_FIRST 60    /* the first load leaves after a second (and staggered) */
#define GARAGE_FLASH 60    /* a garage flashes for a second before it opens */
#define GARAGE_GAP 24      /* foes of one load leave this far apart */
#define GARAGE_REST 180    /* an empty garage waits 3 s before the next load */
#define HIT 144            /* 9 pixels: touching */

/* foes: roller, duster, tanker, gloop, mother gloop, popper, hedgehog, conker, jelly */
/* tankers and mother gloops go twice the others' speed, still under yours */
static const int FOE_SPD[F_KINDS] = {7, 7, 14, 7, 14, 5, 7, 6, 7};
#define DUSTER_LIFE 600    /* a duster flies 10 s, then sprays for 2 and leaves */
#define DUSTER_SPRAY 120
#define POPPER_FUSE 660    /* a popper bursts 13 s after it leaves its garage */
#define POPPER_FLASH 120
#define POPPER_R2 20       /* ... over every tile within about 4 1/2 tiles */
#define GLOOP_EVERY 240    /* the mother gloop slows every 4 s */
#define GLOOP_SLOW 50      /* for under a second, and lays a little one */

int wp_opposite(int d) { return (d + 2) & 3; }
static bool inb(int x, int y) { return x >= 0 && y >= 0 && x < WP_W && y < WP_H; }

static void event(int kind, int32_t x, int32_t y) {
    if (wp.n_ev >= ARRAY_LEN(wp.ev)) return;
    wp.ev[wp.n_ev].kind = (uint8_t)kind;
    wp.ev[wp.n_ev].x = (int16_t)(x / WP_UNIT);
    wp.ev[wp.n_ev].y = (int16_t)(y / WP_UNIT);
    wp.n_ev++;
}

bool wp_solid(int x, int y) {
    if (!inb(x, y)) return true;
    const WpTile *t = &wp.tile[y][x];
    switch (t->kind) {
    case TK_WALL: case TK_GARAGE: case TK_BUMPER: return true;
    case TK_THORN: return wp.thorn[y][x] != 0;
    case TK_SWING: return t->alt ? wp.swing_open != 0 : wp.swing_open == 0;
    case TK_BOLLARD: return t->alt == wp.bollard_phase;
    default: return false;
    }
}

static bool paintable(int x, int y) {
    if (!inb(x, y)) return false;
    int k = wp.tile[y][x].kind;
    return k != TK_WALL && k != TK_GARAGE && k != TK_BUMPER;
}

int wp_count(int team) {
    int n = 0;
    for (int y = 0; y < WP_H; y++)
        for (int x = 0; x < WP_W; x++) n += wp.paint[y][x] == team;
    return n;
}

int wp_percent(int team) { return wp.total ? wp_count(team) * 100 / wp.total : 0; }

void wp_mover_pos(const WpMover *m, int32_t *x, int32_t *y) {
    *x = (m->tx * WP_TILE + WP_TILE / 2) * WP_UNIT + WP_DX[m->dir] * m->prog;
    *y = (m->ty * WP_TILE + WP_TILE / 2) * WP_UNIT + WP_DY[m->dir] * m->prog;
}

/* the tile the mover's centre is on */
static void mover_tile(const WpMover *m, int *x, int *y) {
    bool far = m->prog >= WP_TU / 2;
    *x = m->tx + (far ? WP_DX[m->dir] : 0);
    *y = m->ty + (far ? WP_DY[m->dir] : 0);
}

static bool open_dir(int x, int y, int d) { return !wp_solid(x + WP_DX[d], y + WP_DY[d]); }

/* Move along the lanes. At every tile centre on the way, center() picks the
 * heading (or returns true to stop there). A barrier that rises in front of
 * a mover still short of half-way sends it back to the centre. */
typedef bool (*CenterFn)(void *ctx, WpMover *m);
static void advance(WpMover *m, int dist, CenterFn center, void *ctx) {
    for (int guard = 0; dist > 0 && guard < 16; guard++) {
        if (m->prog == 0) {
            if (center(ctx, m)) return;
        } else if (m->prog < WP_TU / 2 && !open_dir(m->tx, m->ty, m->dir)) {
            m->prog = (int16_t)imax(0, m->prog - dist);
            return;
        }
        int step = imin(dist, WP_TU - m->prog);
        m->prog = (int16_t)(m->prog + step);
        dist -= step;
        if (m->prog >= WP_TU) {
            m->tx = (int8_t)(m->tx + WP_DX[m->dir]);
            m->ty = (int8_t)(m->ty + WP_DY[m->dir]);
            m->prog = 0;
        }
    }
}

/* conveyor belts: faster along, slower against */
static int belt_scale(const WpMover *m, int dist) {
    int x, y;
    mover_tile(m, &x, &y);
    if (!inb(x, y) || wp.tile[y][x].kind != TK_BELT) return dist;
    int b = wp.tile[y][x].dir;
    if (b == m->dir) return dist * 3 / 2;
    if (b == wp_opposite(m->dir)) return dist / 2;
    return dist;
}

static bool frozen(int team) { return wp.freeze_t > 0 && wp.freeze_team == team; }

/* ---- cars ---------------------------------------------------------------- */

bool wp_car_boosted(const WpCar *c) { return c->boost_t > 0 && c->speed > CRUISE + 4; }

static void give_boost(WpCar *c) {
    c->boost_lv = (uint8_t)(c->boost_t > 0 ? imin(c->boost_lv + 1, 3) : 1);
    c->boost_t = BOOST_TIME;
    c->speed = (int16_t)imax(c->speed, BOOST_SPD[c->boost_lv]);
    c->stopped = 0;
}

static void stun(WpCar *c) {
    if (c->stun_t > 0 || c->inv_t > 0) return;
    int32_t x, y;
    wp_mover_pos(&c->m, &x, &y);
    c->stun_t = STUN_TIME;
    if (c == &wp.car[0]) wp.stuns++;
    c->speed = 0;
    c->boost_t = 0;
    c->spin = 0;
    c->want = -1;
    event(EV_STUN, x, y);
}

static int held_dir(uint32_t hold, int prefer_not_axis) {
    int best = -1;
    for (int d = 0; d < 4; d++)
        if (hold & (uint32_t)PAD[d]) {
            if (best < 0 || (d & 1) != (prefer_not_axis & 1)) best = d;
        }
    return best;
}

static bool car_center(void *ctx, WpMover *m) {
    WpCar *c = (WpCar *)ctx;
    int d = m->dir;
    if (c->spin) {
        if (open_dir(m->tx, m->ty, d)) return false;
        c->spin = 0;
        c->inv_t = 0;
        stun(c);
        return true;
    }
    if (c->stopped) {
        /* halted at a wall: any open way goes, even straight back */
        int w = c->want >= 0 && (c->hold & (uint32_t)PAD[c->want]) ? c->want : held_dir(c->hold, d);
        if (w >= 0 && open_dir(m->tx, m->ty, w)) {
            m->dir = (int8_t)w;
            c->stopped = 0;
            c->want = -1;
        }
        return true; /* it sets off from standstill next frame */
    }
    int w = c->want;
    if (w >= 0 && w != wp_opposite(d) && w != d && open_dir(m->tx, m->ty, w)) {
        m->dir = (int8_t)w;
        c->want = -1;
        return false;
    }
    /* a turn that didn't fit here is forgotten unless the pad still holds it */
    if (w >= 0 && !(c->hold & (uint32_t)PAD[w])) c->want = -1;
    if (open_dir(m->tx, m->ty, d)) return false;
    int nx = m->tx + WP_DX[d], ny = m->ty + WP_DY[d];
    if (inb(nx, ny) && wp.tile[ny][nx].kind == TK_BUMPER) {
        /* a bumper turns you round and throws you off at boost speed */
        m->dir = (int8_t)wp_opposite(d);
        give_boost(c);
        int32_t x, y;
        wp_mover_pos(m, &x, &y);
        event(EV_BUMP, x, y);
        return false;
    }
    if (inb(nx, ny) && wp.tile[ny][nx].kind == TK_THORN && wp.thorn[ny][nx]) {
        /* a thorn hedge breaks; at normal speed it stops you, boosted you go on */
        wp.thorn[ny][nx] = 0;
        event(EV_THORN, (nx * WP_TILE + WP_TILE / 2) * WP_UNIT, (ny * WP_TILE + WP_TILE / 2) * WP_UNIT);
        if (wp_car_boosted(c)) return false;
        stun(c);
        return true;
    }
    c->stopped = 1;
    c->speed = 0;
    c->boost_t = 0;
    return true;
}

static void spawn_drone(WpCar *c) {
    for (int i = 0; i < WP_MAX_DRONES; i++) {
        WpDrone *d = &wp.drone[i];
        if (d->on) continue;
        memset(d, 0, sizeof *d);
        d->on = 1;
        d->team = c->team;
        int x, y;
        mover_tile(&c->m, &x, &y);
        d->m.tx = (int8_t)x;
        d->m.ty = (int8_t)y;
        d->m.dir = c->m.dir;
        if (!open_dir(x, y, d->m.dir))
            for (int k = 0; k < 4; k++)
                if (open_dir(x, y, k)) { d->m.dir = (int8_t)k; break; }
        d->inv_t = HELPER_SAFE;
        d->last_tx = (int8_t)x;
        d->last_ty = (int8_t)y;
        return;
    }
}

static void use_item(WpCar *c, int item, int x, int y) {
    int32_t px = (x * WP_TILE + WP_TILE / 2) * WP_UNIT, py = (y * WP_TILE + WP_TILE / 2) * WP_UNIT;
    event(EV_ITEM, px, py);
    switch (item) {
    case ITEM_SPRINKLER:
    case ITEM_TACK:
        c->gun = (uint8_t)item;
        c->gun_t = GUN_TIME;
        c->gun_cd = 0;
        break;
    case ITEM_HELPER: spawn_drone(c); break;
    case ITEM_FREEZE:
        wp.freeze_t = FREEZE_TIME;
        wp.freeze_team = (uint8_t)(c->team == PAINT_BLUE ? PAINT_PINK : PAINT_BLUE);
        break;
    }
}

/* driving onto a tile: arrows, the lever and power-ups (only on a tile that
 * isn't your colour yet) */
static void car_enter(WpCar *c, int x, int y) {
    WpTile *t = &wp.tile[y][x];
    int32_t px = (x * WP_TILE + WP_TILE / 2) * WP_UNIT, py = (y * WP_TILE + WP_TILE / 2) * WP_UNIT;
    if (t->kind == TK_ARROW && t->dir == c->m.dir && !c->spin) {
        give_boost(c);
        event(EV_BOOST, px, py);
    }
    if (t->kind == TK_LEVER) {
        wp.swing_open ^= 1;
        event(EV_LEVER, px, py);
    }
    if (t->item && wp.paint[y][x] != c->team && !c->spin) use_item(c, t->item, x, y);
}

static void add_shot(int kind, int team, int dir, int32_t x, int32_t y, int owner) {
    for (int i = 0; i < WP_MAX_SHOTS; i++)
        if (!wp.shot[i].on) {
            WpShot *s = &wp.shot[i];
            s->on = 1;
            s->kind = (uint8_t)kind;
            s->team = (uint8_t)team;
            s->dir = (int8_t)dir;
            s->x = x;
            s->y = y;
            s->owner = (int8_t)owner;
            return;
        }
}

static void car_update(WpCar *c) {
    if (!c->on || frozen(c->team)) return;
    if (c->inv_t > 0) c->inv_t--;
    if (c->stun_t > 0) {
        if (--c->stun_t == 0) c->inv_t = AFTER_STUN;
        return;
    }
    int d = c->m.dir;
    int speed_team = c->spin ? PAINT_PINK : c->team;
    if (c->spin) {
        advance(&c->m, SPIN_SPD, car_center, c);
    } else {
        /* the pad: a turn waits for the next tile; the other way brakes */
        int w = held_dir(c->hold, d);
        if (w >= 0 && w != d && w != wp_opposite(d)) c->want = (int8_t)w;
        else if (w >= 0 && c->stopped) c->want = (int8_t)w;
        c->braking = (c->hold & (uint32_t)PAD[wp_opposite(d)]) && !c->stopped;
        bool forward = (c->hold & (uint32_t)PAD[d]) != 0;
        if (c->boost_t > 0) c->boost_t--;
        int target = c->boost_t > 0 ? BOOST_SPD[c->boost_lv] : CRUISE;
        if (c->stopped) c->speed = 0;
        else if (c->braking) c->speed = (int16_t)imax(BRAKE_MIN, c->speed - BRAKE);
        else if (c->speed < target) c->speed = (int16_t)imin(target, c->speed + (forward ? ACCEL_HELD : ACCEL));
        else if (c->speed > target) c->speed = (int16_t)imax(target, c->speed - 1);
        if (c->stopped) {
            /* standing still, the pad can still set it off */
            car_center(c, &c->m);
            if (!c->stopped) c->speed = ACCEL_HELD;
        } else {
            advance(&c->m, belt_scale(&c->m, c->speed), car_center, c);
        }
    }
    int x, y;
    mover_tile(&c->m, &x, &y);
    if (x != c->last_tx || y != c->last_ty) {
        c->last_tx = (int8_t)x;
        c->last_ty = (int8_t)y;
        car_enter(c, x, y);
    }
    if (paintable(x, y)) wp.paint[y][x] = (uint8_t)speed_team;
    /* the sprinkler and the tack shooter fire ahead, left and right */
    if (c->gun_t > 0) {
        c->gun_t--;
        if (--c->gun_cd <= 0) {
            c->gun_cd = GUN_EVERY;
            int32_t px, py;
            wp_mover_pos(&c->m, &px, &py);
            int kind = c->gun == ITEM_TACK ? SH_TACK : SH_PAINT;
            add_shot(kind, c->team, c->m.dir, px, py, -1);
            add_shot(kind, c->team, (c->m.dir + 1) & 3, px, py, -1);
            add_shot(kind, c->team, (c->m.dir + 3) & 3, px, py, -1);
        }
        if (c->gun_t == 0) c->gun = 0;
    }
}

/* ---- foes ------------------------------------------------------------------- */

int wp_foe_count(void) {
    int n = 0;
    for (int i = 0; i < WP_MAX_FOES; i++) n += wp.foe[i].on;
    return n;
}

int wp_foe_count_kind(int kind) {
    int n = 0;
    for (int i = 0; i < WP_MAX_FOES; i++) n += wp.foe[i].on && wp.foe[i].kind == kind;
    return n;
}

static void foe_pos(const WpFoe *f, int32_t *x, int32_t *y) {
    if (f->kind == F_DUSTER) { *x = f->fx; *y = f->fy; }
    else wp_mover_pos(&f->m, x, y);
}

/* Which way a foe goes at a junction: never straight back unless it must,
 * and mostly towards tiles that aren't pink yet. A bumper counts as a way. */
static int foe_choose(int x, int y, int dir) {
    int opts[4], n = 0, good[4], ng = 0;
    for (int k = 0; k < 4; k++) {
        if (k == wp_opposite(dir)) continue;
        int nx = x + WP_DX[k], ny = y + WP_DY[k];
        bool bump = inb(nx, ny) && wp.tile[ny][nx].kind == TK_BUMPER;
        if (wp_solid(nx, ny) && !bump) continue;
        opts[n++] = k;
        if (!bump && wp.paint[ny][nx] != PAINT_PINK) good[ng++] = k;
    }
    if (n == 0) return open_dir(x, y, wp_opposite(dir)) ? wp_opposite(dir) : -1;
    if (ng > 0 && rng_chance(&wp.rng, 75)) return good[rng_range(&wp.rng, 0, ng - 1)];
    return opts[rng_range(&wp.rng, 0, n - 1)];
}

static void foe_kill(WpFoe *f) {
    int32_t x, y;
    foe_pos(f, &x, &y);
    f->on = 0;
    wp.kills++;
    event(EV_KILL, x, y);
}

int wp_add_foe(int kind, int tx, int ty, int dir, int garage) {
    for (int i = 0; i < WP_MAX_FOES; i++) {
        WpFoe *f = &wp.foe[i];
        if (f->on) continue;
        memset(f, 0, sizeof *f);
        f->on = 1;
        f->kind = (uint8_t)kind;
        f->m.tx = (int8_t)tx;
        f->m.ty = (int8_t)ty;
        f->m.dir = (int8_t)dir;
        f->plan = -1;
        f->armed = 1;
        f->garage = (int8_t)garage;
        f->last_tx = -1;
        f->last_ty = -1;
        f->turn_t = (int16_t)rng_range(&wp.rng, 60, 150);
        wp_mover_pos(&f->m, &f->fx, &f->fy);
        return i;
    }
    return -1;
}

static bool foe_center(void *ctx, WpMover *m) {
    WpFoe *f = (WpFoe *)ctx;
    int d = -1;
    if (f->kind == F_HEDGEHOG && f->plan >= 0) {
        int p = f->plan;
        int nx = m->tx + WP_DX[p], ny = m->ty + WP_DY[p];
        bool bump = inb(nx, ny) && wp.tile[ny][nx].kind == TK_BUMPER;
        if (p != wp_opposite(m->dir) && (!wp_solid(nx, ny) || bump)) d = p;
    }
    if (d < 0) d = foe_choose(m->tx, m->ty, m->dir);
    if (d < 0) return true;
    int nx = m->tx + WP_DX[d], ny = m->ty + WP_DY[d];
    if (inb(nx, ny) && wp.tile[ny][nx].kind == TK_BUMPER) {
        d = wp_opposite(d);
        f->boost_t = BOOST_TIME;
        int32_t x, y;
        wp_mover_pos(m, &x, &y);
        event(EV_BUMP, x, y);
        if (!open_dir(m->tx, m->ty, d)) return true;
    }
    m->dir = (int8_t)d;
    if (f->kind == F_HEDGEHOG) {
        /* it decides the next turn a tile early and shows it on its lights */
        int ax = m->tx + WP_DX[d], ay = m->ty + WP_DY[d];
        f->plan = (int8_t)foe_choose(ax, ay, d);
    }
    return false;
}

static void foe_spray(WpFoe *f) {
    for (int k = 0; k < 4; k++) add_shot(SH_SPRAY, PAINT_PINK, k, f->fx, f->fy, -1);
    event(EV_SPRAY, f->fx, f->fy);
}

static void popper_burst(WpFoe *f) {
    int x, y;
    mover_tile(&f->m, &x, &y);
    for (int yy = 0; yy < WP_H; yy++)
        for (int xx = 0; xx < WP_W; xx++)
            if ((xx - x) * (xx - x) + (yy - y) * (yy - y) <= POPPER_R2 && paintable(xx, yy) && !wp_solid(xx, yy))
                wp.paint[yy][xx] = PAINT_PINK;
    int32_t px, py;
    wp_mover_pos(&f->m, &px, &py);
    event(EV_POP, px, py);
    wp.shake = 16;
    f->on = 0;
}

static bool line_clear(int x0, int y0, int x1, int y1) {
    int dx = isign(x1 - x0), dy = isign(y1 - y0);
    int x = x0, y = y0;
    while (x != x1 || y != y1) {
        x += dx;
        y += dy;
        if (wp_solid(x, y)) return false;
    }
    return true;
}

static void duster_fly(WpFoe *f) {
    int spd = FOE_SPD[F_DUSTER];
    if (f->age < DUSTER_LIFE) {
        if (--f->turn_t <= 0) {
            f->m.dir = (int8_t)((f->m.dir + (rng_chance(&wp.rng, 50) ? 1 : 3)) & 3);
            f->turn_t = (int16_t)rng_range(&wp.rng, 90, 180);
        }
        int32_t nx = f->fx + WP_DX[f->m.dir] * spd, ny = f->fy + WP_DY[f->m.dir] * spd;
        int32_t lo = WP_TU / 2, hx = WP_W * WP_TU - WP_TU / 2, hy = WP_H * WP_TU - WP_TU / 2;
        if (nx < lo || ny < lo || nx > hx || ny > hy) f->m.dir = (int8_t)wp_opposite(f->m.dir);
        else { f->fx = nx; f->fy = ny; }
        int x = f->fx / WP_TU, y = f->fy / WP_TU;
        if (paintable(x, y) && !wp_solid(x, y)) wp.paint[y][x] = PAINT_PINK;
    } else {
        /* flashing: sprays paint four ways, then flies off */
        int s = f->age - DUSTER_LIFE;
        if (s % 30 == 0) foe_spray(f);
        if (s >= DUSTER_SPRAY) f->on = 0;
    }
}

static void foe_update(WpFoe *f) {
    if (!f->on) return;
    f->age++;
    if (f->kind == F_DUSTER) {
        if (!wp.foes_still || f->age >= DUSTER_LIFE) duster_fly(f);
        return;
    }
    if (f->kind == F_POPPER && f->age >= POPPER_FUSE + POPPER_FLASH) { popper_burst(f); return; }
    int spd = FOE_SPD[f->kind];
    if (f->kind == F_BIGGLOOP) {
        f->t++;
        if (!f->slow && f->t >= GLOOP_EVERY) { f->slow = 1; f->t = 0; }
        if (f->slow) {
            spd = 3;
            if (f->t >= GLOOP_SLOW) {
                /* lays a little gloop that heads off the other way */
                int x, y;
                mover_tile(&f->m, &x, &y);
                int dd = wp_opposite(f->m.dir);
                if (!open_dir(x, y, dd)) dd = f->m.dir;
                wp_add_foe(F_GLOOP, x, y, dd, f->garage);
                f->slow = 0;
                f->t = 0;
            }
        }
    }
    if (f->boost_t > 0) { f->boost_t--; spd *= 2; }
    if (!wp.foes_still) advance(&f->m, belt_scale(&f->m, spd), foe_center, f);
    int x, y;
    mover_tile(&f->m, &x, &y);
    if (x != f->last_tx || y != f->last_ty) {
        f->last_tx = (int8_t)x;
        f->last_ty = (int8_t)y;
        if (inb(x, y) && wp.tile[y][x].kind == TK_ARROW && wp.tile[y][x].dir == f->m.dir) f->boost_t = BOOST_TIME;
    }
    if (paintable(x, y)) wp.paint[y][x] = PAINT_PINK;
    /* a conker lets its spines go when you line up with it */
    if (f->kind == F_CONKER && f->armed) {
        const WpCar *c = &wp.car[0];
        if (c->on && c->stun_t == 0) {
            int cx, cy;
            mover_tile(&c->m, &cx, &cy);
            int32_t ax, ay, bx, by;
            wp_mover_pos(&c->m, &ax, &ay);
            wp_mover_pos(&f->m, &bx, &by);
            bool row = iabs(ay - by) < 4 * WP_UNIT && cy == y, col = iabs(ax - bx) < 4 * WP_UNIT && cx == x;
            if ((row || col) && line_clear(x, y, cx, cy)) {
                for (int k = 0; k < 4; k++) add_shot(SH_SPINE, PAINT_PINK, k, bx, by, (int)(f - wp.foe));
                f->armed = 0;
                event(EV_SPRAY, bx, by);
            }
        }
    }
}

/* ---- garages ---------------------------------------------------------------- */

enum { G_WAIT, G_FLASH, G_EMIT, G_IDLE };

static int deck_kind(char ch) {
    switch (ch) {
    case 'r': return F_ROLLER;
    case 'p': return F_DUSTER;
    case 'k': return F_TANKER;
    case 's': return F_GLOOP;
    case 'M': return F_BIGGLOOP;
    case 'g': return F_POPPER;
    case 't': return F_HEDGEHOG;
    case 'c': return F_CONKER;
    case 'b': return F_JELLY;
    }
    return F_ROLLER;
}

static bool car_near(int x, int y) {
    for (int i = 0; i < 2; i++) {
        const WpCar *c = &wp.car[i];
        if (!c->on) continue;
        int32_t cx, cy;
        wp_mover_pos(&c->m, &cx, &cy);
        int32_t tx = (x * WP_TILE + WP_TILE / 2) * WP_UNIT, ty = (y * WP_TILE + WP_TILE / 2) * WP_UNIT;
        if (iabs(cx - tx) < WP_TU && iabs(cy - ty) < WP_TU) return true;
    }
    return false;
}

static void garage_update(int gi) {
    WpGarage *g = &wp.garage[gi];
    int alive = 0;
    for (int i = 0; i < WP_MAX_FOES; i++) alive += wp.foe[i].on && wp.foe[i].garage == gi;
    g->alive = (uint8_t)alive;
    switch (g->phase) {
    case G_WAIT:
        if (--g->t <= 0) { g->phase = G_FLASH; g->t = GARAGE_FLASH; }
        break;
    case G_FLASH:
        if (--g->t <= 0) { g->phase = G_EMIT; g->left = (uint8_t)wp.group; g->t = 0; }
        break;
    case G_EMIT: {
        if (--g->t > 0) break;
        int ex = g->x + WP_DX[g->dir], ey = g->y + WP_DY[g->dir];
        if (car_near(ex, ey)) { g->t = 10; break; }
        int kind;
        if (g->left >= 100) { kind = F_GLOOP; g->left = (uint8_t)(g->left - 100); } /* the mother's first little one */
        else {
            kind = deck_kind(wp.deck[wp.deck_i % (int)strlen(wp.deck)]);
            wp.deck_i++;
            g->left--;
            /* a mother gloop comes out with a little one, and it counts in the load */
            if (kind == F_BIGGLOOP) g->left = (uint8_t)(g->left > 0 ? g->left - 1 + 100 : 100);
        }
        /* it rolls (or takes off) out of the garage towards the lane */
        int fi = wp_add_foe(kind, g->x, g->y, g->dir, gi);
        if (fi >= 0 && kind != F_DUSTER) wp.foe[fi].m.prog = 1;
        g->t = GARAGE_GAP;
        if (g->left == 0) g->phase = G_IDLE;
        break;
    }
    case G_IDLE:
        if (alive == 0) { g->phase = G_WAIT; g->t = GARAGE_REST; }
        break;
    }
}

/* ---- shots and helpers ---------------------------------------------------- */

static bool touching(int32_t ax, int32_t ay, int32_t bx, int32_t by, int r) { return iabs(ax - bx) < r && iabs(ay - by) < r; }

static void shot_update(WpShot *s) {
    for (int half = 0; half < 2 && s->on; half++) {
        s->x += WP_DX[s->dir] * SHOT_SPD / 2;
        s->y += WP_DY[s->dir] * SHOT_SPD / 2;
        int x = s->x / WP_TU, y = s->y / WP_TU;
        if (s->x < 0 || s->y < 0 || !inb(x, y)) { s->on = 0; return; }
        switch (s->kind) {
        case SH_SPRAY:
            if (paintable(x, y) && !wp_solid(x, y) && wp.paint[y][x] != PAINT_PINK) { wp.paint[y][x] = PAINT_PINK; s->on = 0; }
            break;
        case SH_PAINT:
            if (wp_solid(x, y)) { s->on = 0; break; }
            if (wp.paint[y][x] != s->team) { wp.paint[y][x] = s->team; s->on = 0; }
            break;
        case SH_TACK:
        case SH_SPINE:
            if (wp_solid(x, y)) { s->on = 0; break; }
            for (int i = 0; i < WP_MAX_FOES && s->on; i++) {
                WpFoe *f = &wp.foe[i];
                if (!f->on || i == s->owner) continue;
                int32_t fx, fy;
                foe_pos(f, &fx, &fy);
                if (touching(fx, fy, s->x, s->y, 8 * WP_UNIT)) { foe_kill(f); if (s->kind == SH_TACK) s->on = 0; }
            }
            for (int i = 0; i < 2 && s->on; i++) {
                WpCar *c = &wp.car[i];
                if (!c->on || c->team == s->team || c->stun_t || c->inv_t) continue;
                int32_t cx, cy;
                wp_mover_pos(&c->m, &cx, &cy);
                if (touching(cx, cy, s->x, s->y, 7 * WP_UNIT)) { stun(c); s->on = 0; }
            }
            break;
        }
    }
}

static bool drone_center(void *ctx, WpMover *m) {
    WpDrone *d = (WpDrone *)ctx;
    int opts[4], n = 0, good[4], ng = 0;
    for (int k = 0; k < 4; k++) {
        if (k == wp_opposite(m->dir) || !open_dir(m->tx, m->ty, k)) continue;
        opts[n++] = k;
        if (wp.paint[m->ty + WP_DY[k]][m->tx + WP_DX[k]] != d->team) good[ng++] = k;
    }
    if (n == 0) {
        if (!open_dir(m->tx, m->ty, wp_opposite(m->dir))) return true;
        m->dir = (int8_t)wp_opposite(m->dir);
        return false;
    }
    m->dir = (int8_t)(ng > 0 && rng_chance(&wp.rng, 85) ? good[rng_range(&wp.rng, 0, ng - 1)] : opts[rng_range(&wp.rng, 0, n - 1)]);
    return false;
}

static void drone_update(WpDrone *d) {
    if (!d->on || frozen(d->team)) return;
    if (d->inv_t > 0) d->inv_t--;
    advance(&d->m, belt_scale(&d->m, HELPER_SPD), drone_center, d);
    int x, y;
    mover_tile(&d->m, &x, &y);
    if (paintable(x, y)) wp.paint[y][x] = d->team;
    if (d->inv_t > 0) return;
    int32_t px, py;
    wp_mover_pos(&d->m, &px, &py);
    /* it is knocked out by anything of the other colour */
    bool hit = false;
    if (d->team == PAINT_BLUE)
        for (int i = 0; i < WP_MAX_FOES && !hit; i++) {
            if (!wp.foe[i].on) continue;
            int32_t fx, fy;
            foe_pos(&wp.foe[i], &fx, &fy);
            hit = touching(fx, fy, px, py, 8 * WP_UNIT);
        }
    for (int i = 0; i < 2 && !hit; i++) {
        const WpCar *c = &wp.car[i];
        if (!c->on || c->team == d->team) continue;
        int32_t cx, cy;
        wp_mover_pos(&c->m, &cx, &cy);
        hit = touching(cx, cy, px, py, 8 * WP_UNIT);
    }
    for (int i = 0; i < WP_MAX_DRONES && !hit; i++) {
        const WpDrone *o = &wp.drone[i];
        if (!o->on || o->team == d->team) continue;
        int32_t ox, oy;
        wp_mover_pos(&o->m, &ox, &oy);
        hit = touching(ox, oy, px, py, 7 * WP_UNIT);
    }
    if (hit) { d->on = 0; event(EV_KILL, px, py); }
}

/* ---- cars against foes and each other ---------------------------------------- */

/* is (tx, ty) in front of a mover at (fx, fy): ahead, and more ahead than aside */
static bool facing(const WpMover *m, int32_t fx, int32_t fy, int32_t tx, int32_t ty) {
    int32_t along = (tx - fx) * WP_DX[m->dir] + (ty - fy) * WP_DY[m->dir];
    int32_t side = (tx - fx) * WP_DY[m->dir] - (ty - fy) * WP_DX[m->dir];
    return along > 0 && along >= iabs(side);
}

static void car_vs_foes(WpCar *c) {
    if (!c->on || c->stun_t || c->inv_t || frozen(c->team)) return;
    int32_t cx, cy;
    wp_mover_pos(&c->m, &cx, &cy);
    for (int i = 0; i < WP_MAX_FOES; i++) {
        WpFoe *f = &wp.foe[i];
        if (!f->on) continue;
        int32_t fx, fy;
        foe_pos(f, &fx, &fy);
        if (!touching(cx, cy, fx, fy, HIT)) continue;
        bool boosted = wp_car_boosted(c);
        if (c->spin) continue;
        switch (f->kind) {
        case F_HEDGEHOG:
            /* its plough stops you head-on, unless you come in boosted */
            if (facing(&f->m, fx, fy, cx, cy) && !boosted) { stun(c); return; }
            foe_kill(f);
            break;
        case F_CONKER:
            foe_kill(f);
            if (f->armed && !boosted) { stun(c); return; }
            break;
        case F_JELLY:
            if (boosted) {
                /* it throws you: you spin straight to the wall, leaving pink */
                c->spin = 1;
                c->boost_t = 0;
                c->speed = SPIN_SPD;
                c->want = -1;
                event(EV_STUN, cx, cy);
                return;
            }
            foe_kill(f);
            break;
        default: foe_kill(f); break;
        }
    }
}

static void car_vs_car(void) {
    WpCar *a = &wp.car[0], *b = &wp.car[1];
    if (!a->on || !b->on) return;
    if (a->stun_t || b->stun_t || a->inv_t || b->inv_t || a->spin || b->spin) return;
    int32_t ax, ay, bx, by;
    wp_mover_pos(&a->m, &ax, &ay);
    wp_mover_pos(&b->m, &bx, &by);
    if (!touching(ax, ay, bx, by, HIT)) return;
    bool ab = wp_car_boosted(a), bb = wp_car_boosted(b);
    if (ab && !bb) { stun(b); return; }
    if (bb && !ab) { stun(a); return; }
    bool ra = facing(&a->m, ax, ay, bx, by) && !a->stopped, rb = facing(&b->m, bx, by, ax, ay) && !b->stopped;
    if (ra && !rb) stun(b);
    else if (rb && !ra) stun(a);
    else { stun(a); stun(b); }
}

/* ---- the course --------------------------------------------------------------- */

void wp_sim_start(int course, int mode, uint64_t seed) {
    memset(&wp, 0, sizeof wp);
    rng_seed(&wp.rng, seed);
    wp.course = course;
    wp.mode = mode;
    const WpCourse *cd = &WP_COURSE[course];
    wp.deck = mode == MODE_VERSUS ? "" : cd->deck;
    wp.group = cd->group;
    wp.frames_left = cd->secs * TICK_HZ;
    wp.bollard_t = BOLLARD_EVERY;
    int px = 0, py = 0, qx = WP_W - 1, qy = 0;
    for (int y = 0; y < WP_H; y++)
        for (int x = 0; x < WP_W; x++) {
            WpTile *t = &wp.tile[y][x];
            char ch = cd->rows[y][x];
            t->kind = TK_FLOOR;
            switch (ch) {
            case '#': t->kind = TK_WALL; break;
            case 'R': t->kind = TK_GARAGE; t->dir = WP_RIGHT; break;
            case 'L': t->kind = TK_GARAGE; t->dir = WP_LEFT; break;
            case 'U': t->kind = TK_GARAGE; t->dir = WP_UP; break;
            case 'D': t->kind = TK_GARAGE; t->dir = WP_DOWN; break;
            case '>': t->kind = TK_ARROW; t->dir = WP_RIGHT; break;
            case '<': t->kind = TK_ARROW; t->dir = WP_LEFT; break;
            case '^': t->kind = TK_ARROW; t->dir = WP_UP; break;
            case 'v': t->kind = TK_ARROW; t->dir = WP_DOWN; break;
            case '1': t->item = ITEM_SPRINKLER; break;
            case '2': t->item = ITEM_TACK; break;
            case '3': t->item = ITEM_HELPER; break;
            case '4': t->item = ITEM_FREEZE; break;
            case 'o': t->kind = TK_BUMPER; break;
            case 'x': t->kind = TK_THORN; wp.thorn[y][x] = 1; break;
            case 'T': t->kind = TK_SWING; break;
            case 't': t->kind = TK_SWING; t->alt = 1; break;
            case '!': t->kind = TK_LEVER; break;
            case 'A': t->kind = TK_BOLLARD; break;
            case 'B': t->kind = TK_BOLLARD; t->alt = 1; break;
            case '(': t->kind = TK_BELT; t->dir = WP_LEFT; break;
            case ')': t->kind = TK_BELT; t->dir = WP_RIGHT; break;
            case 'm': t->kind = TK_BELT; t->dir = WP_UP; break;
            case 'w': t->kind = TK_BELT; t->dir = WP_DOWN; break;
            case 'P': px = x; py = y; break;
            case 'Q': qx = x; qy = y; break;
            }
            if (t->kind == TK_GARAGE && wp.n_garages < ARRAY_LEN(wp.garage) && mode != MODE_VERSUS && wp.deck[0]) {
                WpGarage *g = &wp.garage[wp.n_garages];
                g->x = (int8_t)x;
                g->y = (int8_t)y;
                g->dir = (int8_t)t->dir;
                g->phase = G_WAIT;
                g->t = (int16_t)(GARAGE_FIRST + 45 * wp.n_garages);
                wp.n_garages++;
            }
            if (paintable(x, y)) wp.total++;
        }
    for (int i = 0; i < 2; i++) {
        WpCar *c = &wp.car[i];
        c->team = (uint8_t)(i == 0 ? PAINT_BLUE : PAINT_PINK);
        c->m.tx = (int8_t)(i == 0 ? px : qx);
        c->m.ty = (int8_t)(i == 0 ? py : qy);
        c->want = -1;
        c->speed = 0;
        c->last_tx = c->m.tx;
        c->last_ty = c->m.ty;
        /* face the first open way: up, then right or left */
        static const int ORDER[4] = {WP_UP, WP_RIGHT, WP_LEFT, WP_DOWN};
        c->m.dir = WP_UP;
        for (int k = 0; k < 4; k++)
            if (open_dir(c->m.tx, c->m.ty, ORDER[k])) { c->m.dir = (int8_t)ORDER[k]; break; }
        if (i == 1 && c->m.dir == WP_RIGHT && open_dir(c->m.tx, c->m.ty, WP_LEFT)) c->m.dir = WP_LEFT;
    }
    wp.car[0].on = 1;
    wp.car[0].human = 1;
    wp.car[1].on = (uint8_t)(course == WP_FINAL || mode == MODE_VERSUS);
    wp.car[1].human = (uint8_t)(mode == MODE_VERSUS);
    for (int i = 0; i < 2; i++)
        if (wp.car[i].on) wp.paint[wp.car[i].m.ty][wp.car[i].m.tx] = wp.car[i].team;
}

void wp_sim_step(void) {
    wp.frame++;
    wp.n_ev = 0;
    if (wp.shake > 0) wp.shake--;
    /* the course clock stops while a freeze pop runs */
    if (wp.freeze_t > 0) wp.freeze_t--;
    else if (wp.frames_left > 0 && !wp.no_clock) wp.frames_left--;
    /* bollards rise and sink in turn; one rising under a car stuns it */
    if (--wp.bollard_t <= 0) {
        wp.bollard_t = BOLLARD_EVERY;
        wp.bollard_phase ^= 1;
        for (int i = 0; i < 2; i++) {
            WpCar *c = &wp.car[i];
            if (!c->on) continue;
            int x, y;
            mover_tile(&c->m, &x, &y);
            if (inb(x, y) && wp.tile[y][x].kind == TK_BOLLARD && wp_solid(x, y)) stun(c);
        }
    }
    for (int i = 0; i < 2; i++) car_update(&wp.car[i]);
    if (!frozen(PAINT_PINK) && !wp.no_foes) {
        for (int i = 0; i < wp.n_garages; i++) garage_update(i);
        for (int i = 0; i < WP_MAX_FOES; i++) foe_update(&wp.foe[i]);
    }
    for (int i = 0; i < WP_MAX_DRONES; i++) drone_update(&wp.drone[i]);
    for (int i = 0; i < WP_MAX_SHOTS; i++)
        if (wp.shot[i].on) shot_update(&wp.shot[i]);
    for (int i = 0; i < 2; i++) car_vs_foes(&wp.car[i]);
    car_vs_car();
    /* the secret: parked in the far corner of course 1 while the pink takes over */
    if (wp.course == 0 && wp.mode == MODE_SOLO) {
        int x, y;
        mover_tile(&wp.car[0].m, &x, &y);
        if (x == WP_W - 1 && y == WP_H - 1 && wp_percent(PAINT_PINK) >= 80) wp.secret = 1;
    }
}

/* ---- the demo driver ---------------------------------------------------------- */

/* Breadth-first over the lanes from the tile the car will decide at next,
 * never straight back on the first step. It heads for the nearest tile not
 * yet its colour, a power-up it can use, or a foe it can safely ram. */
static int8_t bot_block[WP_H][WP_W];

static int bot_target_score(int who, int x, int y, int dist) {
    const WpCar *c = &wp.car[who];
    int team = c->team;
    if (bot_block[y][x]) return -1;
    if (wp.tile[y][x].item && wp.paint[y][x] != team && dist <= 10) return 3;
    for (int i = 0; i < WP_MAX_FOES; i++) {
        const WpFoe *f = &wp.foe[i];
        if (!f->on || f->kind == F_DUSTER || f->kind == F_HEDGEHOG || (f->kind == F_CONKER && f->armed)) continue;
        int fx, fy;
        mover_tile(&f->m, &fx, &fy);
        if (fx == x && fy == y && dist <= 6) return 2;
    }
    if (wp.paint[y][x] != team) return 1;
    return 0;
}

int wp_bot_buttons(int who) {
    WpCar *c = &wp.car[who];
    if (!c->on || c->stun_t) return 0;
    memset(bot_block, 0, sizeof bot_block);
    /* keep off the front of hedgehogs, the lines of armed conkers and thorns */
    for (int i = 0; i < WP_MAX_FOES; i++) {
        const WpFoe *f = &wp.foe[i];
        if (!f->on) continue;
        if (f->kind == F_HEDGEHOG) {
            int fx, fy;
            mover_tile(&f->m, &fx, &fy);
            for (int k = 0; k <= 2; k++) {
                int x = fx + WP_DX[f->m.dir] * k, y = fy + WP_DY[f->m.dir] * k;
                if (inb(x, y)) bot_block[y][x] = 1;
            }
        }
        if (f->kind == F_CONKER && f->armed) {
            int fx, fy;
            mover_tile(&f->m, &fx, &fy);
            for (int d = 0; d < 4; d++)
                for (int k = 0; k <= WP_W; k++) {
                    int x = fx + WP_DX[d] * k, y = fy + WP_DY[d] * k;
                    if (wp_solid(x, y)) break;
                    bot_block[y][x] = 1;
                }
        }
        if (f->kind == F_JELLY && wp_car_boosted(c)) {
            int fx, fy;
            mover_tile(&f->m, &fx, &fy);
            bot_block[fy][fx] = 1;
        }
    }
    int ox = c->m.tx, oy = c->m.ty, head = c->m.dir;
    if (c->m.prog > 0) { ox += WP_DX[head]; oy += WP_DY[head]; }
    /* our own tile is fine to leave even if it is marked */
    static int16_t dist[WP_H][WP_W];
    static int8_t first[WP_H][WP_W];
    static int16_t queue[WP_H * WP_W];
    for (int y = 0; y < WP_H; y++)
        for (int x = 0; x < WP_W; x++) dist[y][x] = -1;
    int qh = 0, qt = 0;
    dist[oy][ox] = 0;
    first[oy][ox] = -1;
    queue[qt++] = (int16_t)(oy * WP_W + ox);
    int best = -1, best_score = 0, best_d = 9999, fallback = -1;
    while (qh < qt) {
        int q = queue[qh++], x = q % WP_W, y = q / WP_W;
        for (int k = 0; k < 4; k++) {
            if (x == ox && y == oy && k == wp_opposite(head) && !c->stopped) continue;
            int nx = x + WP_DX[k], ny = y + WP_DY[k];
            if (wp_solid(nx, ny) || dist[ny][nx] >= 0) continue;
            if (inb(nx, ny) && wp.tile[ny][nx].kind == TK_THORN && wp.thorn[ny][nx]) continue;
            dist[ny][nx] = (int16_t)(dist[y][x] + 1);
            first[ny][nx] = (int8_t)(x == ox && y == oy ? k : first[y][x]);
            if (fallback < 0) fallback = first[ny][nx];
            int s = bot_target_score(who, nx, ny, dist[ny][nx]);
            if (s < 0) continue; /* don't route through danger */
            /* a better kind of target wins if it is not much farther */
            static const int PULL[4] = {0, 2, 10, 12};
            int d = dist[ny][nx] - PULL[s];
            if (s > 0 && (best < 0 || d < best_d)) { best = first[ny][nx]; best_score = s; best_d = d; }
            queue[qt++] = (int16_t)(ny * WP_W + nx);
        }
    }
    (void)best_score;
    int d = best >= 0 ? best : fallback;
    if (who == 1 && !c->human && rng_chance(&wp.rng, 10)) {
        /* the rival isn't perfect */
        int k = rng_range(&wp.rng, 0, 3);
        if (k != wp_opposite(head) && open_dir(ox, oy, k)) d = k;
    }
    int mask = 0;
    if (d >= 0) mask |= PAD[d];
    else if (c->stopped) {
        for (int k = 0; k < 4; k++)
            if (open_dir(c->m.tx, c->m.ty, k)) { mask |= PAD[k]; break; }
    }
    /* brake before ramming a jelly or a hedgehog's plough at boost speed */
    if (wp_car_boosted(c)) {
        int32_t cx, cy;
        wp_mover_pos(&c->m, &cx, &cy);
        for (int i = 0; i < WP_MAX_FOES; i++) {
            const WpFoe *f = &wp.foe[i];
            if (!f->on || f->kind != F_JELLY) continue;
            int32_t fx, fy;
            wp_mover_pos(&f->m, &fx, &fy);
            if (touching(cx, cy, fx, fy, 3 * WP_TU)) mask = PAD[wp_opposite(head)];
        }
    }
    return mask;
}
