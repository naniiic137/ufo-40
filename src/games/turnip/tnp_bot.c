/* TURNIP TRUCK - the demo player. It plans a route over the city's cells
 * (a distance field from the destination, wrapping like the city), follows
 * it on the right-hand side of the road by pure pursuit, brakes before
 * corners, backs out when stuck, and slows for puddles. It only ever
 * chooses buttons; the truck answers them like anyone else's presses.
 * After the quota it takes overtime while the clock can pay for the trip
 * there and home again, then heads for the depot. */
#include "tnp.h"

static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
#define N (TNP_MAP * TNP_MAP)
#define INF 0x3fffffff

static struct {
    int tx, ty;            /* the cell the field leads to */
    int field[N];
    int steer, steer_t;    /* the turn being held, and for how long */
    int slow_t, back_t;    /* stuck: frames slow with the gas down; frames backing out */
    int back_steer;
    int home;              /* heading for the depot */
    int checked_dest;      /* the overtime delivery already weighed up (+1) */
    int last_frame;
} B;

static int cell_cost(int cx, int cy) {
    const TnpCell *c = tnp_cell(cx, cy);
    if (tnp_solid_type(c->type) || c->type == CT_BRINE) return -1;
    switch (c->type) {
    case CT_ROCK: return 40;
    case CT_GRASS: return 13;
    default: break;
    }
    if (c->feat == FT_CANS) return 80;
    if (c->feat == FT_RAMP) return 400;
    if (c->feat == FT_PIPE) return 12;
    return 10;
}

static void build_field(int tx, int ty) {
    static uint8_t done[N];
    for (int i = 0; i < N; i++) B.field[i] = INF;
    memset(done, 0, sizeof done);
    B.field[ty * TNP_MAP + tx] = 0;
    for (;;) {
        int best = -1, bd = INF;
        for (int i = 0; i < N; i++)
            if (!done[i] && B.field[i] < bd) { bd = B.field[i]; best = i; }
        if (best < 0) break;
        done[best] = 1;
        int x = best % TNP_MAP, y = best / TNP_MAP;
        int c0 = cell_cost(x, y);
        if (c0 < 0 && bd > 0) continue;
        for (int k = 0; k < 4; k++) {
            int nx = tnp_wrapc(x + DX[k]), ny = tnp_wrapc(y + DY[k]);
            int c = cell_cost(nx, ny);
            if (c < 0) continue;
            int nd = bd + c;
            if (nd < B.field[ny * TNP_MAP + nx]) B.field[ny * TNP_MAP + nx] = nd;
        }
    }
    B.tx = tx;
    B.ty = ty;
}

int tnp_path_len(int sx, int sy, int tx, int ty) {
    build_field(tnp_wrapc(tx), tnp_wrapc(ty));
    int v = B.field[tnp_wrapc(sy) * TNP_MAP + tnp_wrapc(sx)];
    B.tx = -1; /* the field was for this question only */
    return v >= INF ? -1 : v / 10;
}

void tnp_bot_reset(void) {
    memset(&B, 0, sizeof B);
    B.tx = B.ty = -1;
}

static int next_cell(int x, int y) {
    int best = -1, bd = B.field[y * TNP_MAP + x];
    for (int k = 0; k < 4; k++) {
        int nx = tnp_wrapc(x + DX[k]), ny = tnp_wrapc(y + DY[k]);
        int v = B.field[ny * TNP_MAP + nx];
        if (v < bd) { bd = v; best = k; }
    }
    return best;
}

static float angdiff(float a, float b) {
    float d = a - b;
    while (d > 3.14159265f) d -= 6.2831853f;
    while (d < -3.14159265f) d += 6.2831853f;
    return d;
}

/* seconds to drive a route of this many cells, roughly */
static int secs_for(int cells) { return cells * TNP_CELL / 95 + 3; }

uint16_t tnp_bot_buttons(TnpDay *d) {
    TnpTruck *t = &d->tr;
    if (d->phase != DP_PLAY || t->state != TS_DRIVE) return 0;
    if (t->air_t > 0 || t->spin_t > 0) return BTN_A;
    if (d->frame < B.last_frame) tnp_bot_reset();
    B.last_frame = d->frame;
    int cx = (int)(t->x / TNP_CELL), cy = (int)(t->y / TNP_CELL);

    /* where to: the delivery, or home */
    float gx, gy;
    tnp_dest_pos(d->dest, &gx, &gy);
    int gcx = TNP_DEST[d->dest].cx, gcy = TNP_DEST[d->dest].cy;
    if (!d->practice && d->delivered >= TNP_QUOTA) {
        if (!B.home && B.checked_dest != d->dest + 1) {
            B.checked_dest = d->dest + 1;
            int there = tnp_path_len(cx, cy, gcx, gcy), back = tnp_path_len(gcx, gcy, 14, 10);
            if (there < 0 || back < 0 || tnp_seconds(d) < secs_for(there + back) + 10) B.home = 1;
        }
        if (B.home) {
            gx = TNP_HQ_DOOR_X;
            gy = TNP_HQ_DOOR_Y + 7;
            gcx = 14;
            gcy = 10;
        }
    } else {
        B.home = 0;
        B.checked_dest = 0;
    }
    if (gcx != B.tx || gcy != B.ty) build_field(gcx, gcy);

    /* the route ahead as points on the right-hand side of the road */
    float px[6], py[6];
    int corner[6];
    int np = 0, x = cx, y = cy, prevk = -1;
    for (int i = 0; i < 5; i++) {
        int k = next_cell(x, y);
        if (k < 0) break;
        x = tnp_wrapc(x + DX[k]);
        y = tnp_wrapc(y + DY[k]);
        float ox = (float)(-DY[k]) * 8, oy = (float)DX[k] * 8;
        if (np > 0 && prevk != k) {
            /* the cell before is a corner: take it through the middle */
            px[np - 1] = tnp_cx(tnp_wrapc(x - DX[k]));
            py[np - 1] = tnp_cx(tnp_wrapc(y - DY[k]));
            corner[np - 1] = 1;
        }
        px[np] = tnp_cx(x) + ox;
        py[np] = tnp_cx(y) + oy;
        corner[np] = 0;
        np++;
        prevk = k;
        if (x == gcx && y == gcy) break;
    }
    float near_goal = tnp_dist(t->x, t->y, gx, gy);
    bool final_leg = np <= 1 || near_goal < 70;
    /* pure pursuit: the first point at least L ahead */
    float L = 30, ax = gx, ay = gy;
    if (!final_leg) {
        ax = px[np - 1];
        ay = py[np - 1];
        for (int i = 0; i < np; i++) {
            float dd = tnp_dist(t->x, t->y, px[i], py[i]);
            /* a corner is never cut: it is reached before the next point is aimed at */
            if (dd >= L || (corner[i] && dd > 12)) { ax = px[i]; ay = py[i]; break; }
        }
    }
    float want_ang = atan2f(tnp_wrapd(ay - t->y), tnp_wrapd(ax - t->x));
    float diff = angdiff(want_ang, t->ang);
    float f = tnp_forward(t);

    /* how fast: slow for the next corner, the goal, puddles */
    float vmax = TNP_TOP;
    float run = tnp_dist(t->x, t->y, tnp_cx(cx), tnp_cx(cy));
    (void)run;
    for (int i = 1; i < np; i++) {
        float ang1 = atan2f(tnp_wrapd(py[i] - py[i - 1]), tnp_wrapd(px[i] - px[i - 1]));
        float ang0 = i >= 2 ? atan2f(tnp_wrapd(py[i - 1] - py[i - 2]), tnp_wrapd(px[i - 1] - px[i - 2]))
                            : atan2f(tnp_wrapd(py[0] - t->y), tnp_wrapd(px[0] - t->x));
        if (fabsf(angdiff(ang1, ang0)) > 0.6f) {
            float dc = tnp_dist(t->x, t->y, px[i - 1], py[i - 1]);
            float v = sqrtf(1.4f * 1.4f + 2 * 0.07f * fmaxf(0, dc - 10));
            if (v < vmax) vmax = v;
            break;
        }
    }
    if (final_leg) vmax = fminf(vmax, B.home ? 0.6f + near_goal * 0.03f : 1.0f + near_goal * 0.04f);
    if (fabsf(diff) > 0.5f) vmax = fminf(vmax, 1.2f);
    for (int i = 0; i < TNP_MAX_PUDDLES; i++) {
        const TnpPuddle *p = &d->pud[i];
        if (!p->alive) continue;
        float rx = tnp_wrapd(p->x - t->x), ry = tnp_wrapd(p->y - t->y);
        float a = rx * cosf(t->ang) + ry * sinf(t->ang), s = fabsf(-rx * sinf(t->ang) + ry * cosf(t->ang));
        if (a > 0 && a < 40 && s < 14) vmax = fminf(vmax, 1.3f);
    }
    /* a car in the way: ease off and steer round it, or back off if it is right on the bumper */
    float dodge = 0;
    bool blocked = false, behind = false;
    for (int i = 0; i < TNP_MAX_CARS; i++) {
        const TnpCar *c = &d->car[i];
        if (!c->alive || c->wreck_t > 0) continue;
        float rx = tnp_wrapd(c->x - t->x), ry = tnp_wrapd(c->y - t->y);
        float a = rx * cosf(t->ang) + ry * sinf(t->ang), s = -rx * sinf(t->ang) + ry * cosf(t->ang);
        if (a > -4 && a < 48 && fabsf(s) < 16) {
            vmax = fminf(vmax, a < 24 ? 0.5f : 1.0f);
            dodge += s >= 0 ? -0.6f : 0.6f;
            if (a < 18 && fabsf(f) < 0.4f) blocked = true;
        }
        if (a < 0 && a > -44 && fabsf(s) < 16) behind = true;
    }

    uint16_t m = 0;
    /* stuck against something: back out */
    if (B.back_t > 0 && behind) B.back_t = 0; /* never back into a car */
    if (B.back_t > 0) {
        B.back_t--;
        m = BTN_B;
        if (B.back_steer > 0) m |= BTN_RIGHT;
        if (B.back_steer < 0) m |= BTN_LEFT;
        return m;
    }
    if (blocked && B.back_t == 0 && !behind) {
        B.back_t = 24;
        B.back_steer = 0;
        return BTN_B;
    }
    diff += dodge;
    /* brine close by: go gently, turn on the spot rather than swing wide, never roll into it */
    bool wet = false;
    for (int k = 0; k < 4; k++)
        if (tnp_cell(cx + DX[k], cy + DY[k])->type == CT_BRINE) wet = true;
    float pxf = t->x + t->vx * 14 + cosf(t->ang) * 6, pyf = t->y + t->vy * 14 + sinf(t->ang) * 6;
    bool doom = tnp_cell_at(pxf, pyf)->type == CT_BRINE;
    if (doom && f > 0.2f) return BTN_B; /* brake now */
    if (wet) vmax = fminf(vmax, 1.3f);
    if (fabsf(diff) > (wet ? 0.7f : 1.7f) && f < (wet ? 0.5f : 0.9f)) {
        /* the way on is behind: turn on the spot */
        m = BTN_A | BTN_B | (diff > 0 ? BTN_RIGHT : BTN_LEFT);
        B.slow_t = 0;
        return m;
    }
    if (f < vmax - 0.05f) m |= BTN_A;
    else if (f > vmax + 0.25f) m |= BTN_B;
    if (fabsf(f) < 0.25f && (m & BTN_A)) {
        if (++B.slow_t > 40 && !behind) {
            B.slow_t = 0;
            B.back_t = 35;
            B.back_steer = diff > 0 ? -1 : 1;
        }
    } else
        B.slow_t = 0;

    /* the wheel: hold a turn long enough that it never reads as a sidestep tap */
    int want = diff > 0.06f ? 1 : diff < -0.06f ? -1 : 0;
    if (B.steer != 0 && want != B.steer && B.steer_t <= TNP_TAP + 1) want = B.steer;
    if (want != B.steer) { B.steer = want; B.steer_t = 0; }
    B.steer_t++;
    if (B.steer > 0) m |= BTN_RIGHT;
    if (B.steer < 0) m |= BTN_LEFT;
    /* never brake and gas at once by accident (that is a powerslide) */
    if ((m & BTN_A) && (m & BTN_B)) m &= (uint16_t)~BTN_B;
    return m;
}
