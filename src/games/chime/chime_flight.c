/* CHIME CIRCUIT - the chime-ship flight model (see chime_flight.h). */
#include "chime_flight.h"

/* Gravity 0.078 px a frame each frame; thrust lifts at 0.117 net; steering
 * 0.0625 (0.094 with thrust held too). A ship falls a full screen in about
 * a second, climbs one in a little more, and crosses it in two and a half
 * seconds flat out. */
const ChmFlightTune CHM_TUNE = {
    20,   /* gravity */
    50,   /* thrust */
    16,   /* accel_x */
    24,   /* accel_thrust_x */
    3,    /* drag_x */
    560,  /* max_vx: 2.19 px a frame */
    512,  /* max_up: 2 */
    704,  /* max_down: 2.75 */
    14,   /* over_decay */
    1792, /* top_speed: 7 */
    110,  /* bounce: 43 % */
    160,  /* min_bounce: 0.63 px a frame */
    224,  /* slash_drag: a slash takes an eighth off the fall each frame */
    4,    /* half: an 8 x 8 hit box */
};

static int32_t mx(int32_t a, int32_t b) { return a > b ? a : b; }
static int32_t mn(int32_t a, int32_t b) { return a < b ? a : b; }
static int32_t ab(int32_t a) { return a < 0 ? -a : a; }

/* A speed pushed past its cap: a normal push stops at the cap, but a ship
 * that was already faster (a boost, a knock) only slows back gradually. */
static int32_t settle(int32_t before, int32_t after, int32_t lo, int32_t hi, int32_t decay) {
    if (after > hi) return before > hi ? mx(hi, mn(after, before - decay)) : hi;
    if (after < lo) return before < lo ? mn(lo, mx(after, before + decay)) : lo;
    return after;
}

void chm_flight_control(ChmFlight *f, const ChmFlightTune *t, unsigned ctl) {
    int dir = ((ctl & CHF_RIGHT) ? 1 : 0) - ((ctl & CHF_LEFT) ? 1 : 0);
    int32_t vx = f->vx;
    if (dir) {
        vx += dir * ((ctl & CHF_THRUST) ? t->accel_thrust_x : t->accel_x);
        f->face = (int8_t)dir;
    } else if (vx > 0) {
        vx = mx(0, vx - t->drag_x);
    } else if (vx < 0) {
        vx = mn(0, vx + t->drag_x);
    }
    f->vx = settle(f->vx, vx, -t->max_vx, t->max_vx, t->over_decay);
    int32_t vy = f->vy + t->gravity - ((ctl & CHF_THRUST) ? t->thrust : 0);
    f->vy = settle(f->vy, vy, -t->max_up, t->max_down, t->over_decay);
    if ((ctl & CHF_SLASHING) && f->vy > 0) f->vy = f->vy * t->slash_drag / CHF_ONE;
    f->vx = mx(-t->top_speed, mn(t->top_speed, f->vx));
    f->vy = mx(-t->top_speed, mn(t->top_speed, f->vy));
}

static int px_of(int32_t v) { return v >= 0 ? (int)(v / CHF_ONE) : -(int)((-v + CHF_ONE - 1) / CHF_ONE); }

bool chm_flight_blocked(int half, ChmSolidFn solid, const void *ctx, int32_t x, int32_t y) {
    int cx = px_of(x), cy = px_of(y);
    int l = cx - half, r = cx + half - 1, u = cy - half, d = cy + half - 1;
    return solid(ctx, l, u) || solid(ctx, r, u) || solid(ctx, l, d) || solid(ctx, r, d) ||
           solid(ctx, l, cy) || solid(ctx, r, cy) || solid(ctx, cx, u) || solid(ctx, cx, d) ||
           solid(ctx, cx, cy);
}

static int move_steps(ChmFlight *f, const ChmFlightTune *t, ChmSolidFn solid, const void *ctx) {
    int32_t ox = f->x, oy = f->y, dx = f->vx, dy = f->vy;
    int steps = (int)(mx(ab(dx), ab(dy)) / (CHF_ONE / 2)) + 1; /* half-pixel steps */
    int hit = 0;
    for (int i = 1; i <= steps; i++) {
        if (!(hit & CHF_HIT_X)) {
            int32_t tx = ox + dx * i / steps;
            if (chm_flight_blocked(t->half, solid, ctx, tx, f->y)) hit |= CHF_HIT_X;
            else f->x = tx;
        }
        if (!(hit & CHF_HIT_Y)) {
            int32_t ty = oy + dy * i / steps;
            if (chm_flight_blocked(t->half, solid, ctx, f->x, ty)) hit |= CHF_HIT_Y;
            else f->y = ty;
        }
        if (hit == (CHF_HIT_X | CHF_HIT_Y)) break;
    }
    /* a still axis can't have hit anything */
    if (dx == 0) hit &= ~CHF_HIT_X;
    if (dy == 0) hit &= ~CHF_HIT_Y;
    return hit;
}

int chm_flight_move(ChmFlight *f, const ChmFlightTune *t, ChmSolidFn solid, const void *ctx) {
    int32_t dx = f->vx, dy = f->vy;
    int hit = move_steps(f, t, solid, ctx);
    if (hit & CHF_HIT_X) {
        int32_t v = -dx * t->bounce / CHF_ONE;
        if (ab(v) < t->min_bounce) v = dx > 0 ? -t->min_bounce : t->min_bounce;
        f->vx = v;
    }
    if (hit & CHF_HIT_Y) {
        int32_t v = -dy * t->bounce / CHF_ONE;
        if (ab(v) < t->min_bounce) v = dy > 0 ? -t->min_bounce : t->min_bounce;
        f->vy = v;
    }
    return hit;
}

int chm_flight_probe(ChmFlight *f, const ChmFlightTune *t, ChmSolidFn solid, const void *ctx) {
    int hit = move_steps(f, t, solid, ctx);
    if (hit & CHF_HIT_X) f->vx = 0;
    if (hit & CHF_HIT_Y) f->vy = 0;
    return hit;
}
