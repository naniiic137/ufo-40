/* FULL PEAL - the demo pilot, for the tests: it plays the real rules with
 * real button presses. Each frame it lists what could hit the ship over
 * the next few dozen frames (shots on the plane, shells about to land and
 * the crosses they burst into, foes arriving or sitting on the plane, the
 * fists, a bourdon's marked lane), tries each of the nine ways to steer,
 * and takes the safest that brings it nearest its target: a lane to fire
 * the forward gun down, or a row or column from which the side blaster
 * reaches something already on the plane. */
#include "fpl.h"

int fpl_bot_dbg[4];

#define H 22            /* frames looked ahead */
#define MAXHZ 400

typedef struct { float x, y, vx, vy, r; int t0, t1; bool box; float hw, hh; } Hazard;
static Hazard hz[MAXHZ];
static int nhz;

static void add_hz(float x, float y, float vx, float vy, float r, int t0, int t1) {
    if (nhz >= MAXHZ || t1 < 0 || t0 > H) return;
    hz[nhz++] = (Hazard){x, y, vx, vy, r, t0, t1, false, 0, 0};
}

static void add_box(float x, float y, float hw, float hh, float vx, int t0, int t1) {
    if (nhz >= MAXHZ) return;
    hz[nhz++] = (Hazard){x, y, vx, 0, 0, t0, t1, true, hw, hh};
}

static void burst_hz(float x, float y, int burst, int t) {
    const float v = 0.034f;
    switch (burst) {
    case BURST_CROSS:
        add_hz(x, y, v, 0, 0.12f, t, H); add_hz(x, y, -v, 0, 0.12f, t, H);
        add_hz(x, y, 0, v, 0.12f, t, H); add_hz(x, y, 0, -v, 0.12f, t, H);
        break;
    case BURST_FORK: add_hz(x, y, 0, v, 0.12f, t, H); add_hz(x, y, 0, -v, 0.12f, t, H); break;
    case BURST_TRI: add_hz(x, y, 0, 0, 0.75f, t, t + 18); break;
    default: break;
    }
}

static float foe_radius(int k) {
    return k == EK_BOURDON ? 0.46f : (k == EK_MOTE || k == EK_WISP || k == EK_FLARE) ? 0.24f : k == EK_CALTROP ? 0.34f : 0.3f;
}

static void gather(void) {
    nhz = 0;
    for (int i = 0; i < FPL_MAX_ESHOTS; i++) {
        const FplEShot *e = &fpg.es[i];
        if (!e->alive) continue;
        if (e->kind == ES_PLANE) {
            float r = e->spin != 0 ? 0.2f : 0.12f;
            add_hz(e->x, e->y, e->vx, e->vy, r, 0, H);
        } else {
            int n = (int)ceilf(e->z / -e->vz);
            float lx = e->x + e->vx * n, ly = e->y + e->vy * n;
            add_hz(lx, ly, 0, 0, e->burst ? 0.18f : 0.22f, n - 2, n + 1);
            if (e->burst) burst_hz(lx, ly, e->burst, n);
        }
    }
    for (int i = 0; i < FPL_MAX_FOES; i++) {
        const FplFoe *f = &fpg.foe[i];
        if (!f->alive) continue;
        float r = foe_radius(f->kind) * 0.85f;
        int k = f->kind;
        if (f->t < 0) {
            /* still warning: where it will come in */
            int wait = -f->t;
            if (k == EK_BOURDON) continue;
            float vx = (k == EK_WISP ? 0.03f : 0.032f) * f->dir, vy = k == EK_FLARE ? 0.03f : 0;
            float sx = f->x, sy = f->y;
            if (k == EK_LOOKOUT) continue;
            if (k == EK_MOTE || k == EK_NIBBLER) { add_hz(sx, sy, 0, 0, r + 0.25f, wait, H); continue; }
            add_hz(sx, sy, vx, vy, r, wait, H);
            continue;
        }
        if (k == EK_BOURDON) {
            if (f->state == FS_IN) {
                int n = f->x == f->tx ? (int)(f->z / FPL_FOE[k].vz) : (int)(fabsf(f->x - f->tx) / 0.07f + 0.5f / FPL_FOE[k].vz);
                add_hz(f->tx, f->ty, 0, 0, r, n - 4, H);
            } else if (f->state == FS_HOLD) add_hz(f->x, f->y, 0, 0, r, 0, H);
            continue;
        }
        if (f->state == FS_OUT) continue;
        if (f->z > FPL_PLANE_Z) {
            /* on its way in: where and when it reaches the plane */
            if (k == EK_CROSSHEAD || k == EK_FORKER || k == EK_BROODER) continue;
            float vz = -f->vz;
            if (vz <= 0) continue;
            int n = (int)(f->z / vz);
            if (k == EK_SALLY || k == EK_QUICKSALLY) { add_hz(f->x, f->y, 0, 0, 0.8f, n - 3, n + 18); continue; }
            if (k == EK_CALTROP) { add_hz(f->x, f->y, 0, 0, r + 0.05f, n - 2, n + 6); continue; }
            if (k == EK_PENDULUM || k == EK_SPITE) { add_hz(f->x, f->y, 0, f->vy, r, n - 1, H); continue; }
            add_hz(f->x, f->y, 0, 0, r, n - 2, H);
            continue;
        }
        float vx = 0, vy = 0;
        switch (k) {
        case EK_MOTE: case EK_NIBBLER: vx = f->vx; vy = f->vy; break;
        case EK_WISP: vx = 0.03f * f->dir; break;
        case EK_FLARE: vy = 0.03f; break;
        case EK_PENDULUM: case EK_SPITE: vy = f->vy; break;
        case EK_LOOKOUT: break;
        default: if (f->arg == 9) vx = 0.032f * f->dir; break;
        }
        add_hz(f->x, f->y, vx, vy, r + (k == EK_NIBBLER ? 0.1f : 0), 0, H);
        if (k == EK_LOOKOUT) {
            /* its row is a firing line */
            add_hz(f->x, f->y, f->dir * 0.06f, 0, 0.18f, 0, H);
            add_box(0, f->y, 3.2f, 0.18f, 0, 0, H);
        }
    }
    const FplBoss *b = &fpg.boss;
    if (b->on && !b->dead && b->kind == BOSS_KNUCKLEBELL) {
        for (int k = 0; k < 2; k++) {
            float vx = b->fist_t[k] > 0 ? (k ? 0.11f : -0.11f) : (k ? -0.016f : 0.016f);
            add_box(b->fx[k], b->fy, FPL_FIST_HW + 0.12f, FPL_FIST_HH + 0.12f, vx, 0, H);
        }
    }
}

/* the first frame (1..H) at which the ship, steered dx, dy, would be hit; H + 1 if never */
static int first_hit(float x, float y, int dx, int dy, int hold) {
    float sp = FPL_SHIP_SPEED * ((dx && dy) ? 0.7071f : 1.0f);
    for (int t = 1; t <= H; t++) {
        if (t <= hold) {
            x = fclamp(x + dx * sp, -FPL_SHIP_MX, FPL_SHIP_MX);
            y = fclamp(y + dy * sp, -FPL_SHIP_MY, FPL_SHIP_MY);
        }
        for (int i = 0; i < nhz; i++) {
            const Hazard *h = &hz[i];
            if (t < h->t0 || t > h->t1) continue;
            float hx = h->x + h->vx * (t - (h->t0 > 0 ? h->t0 : 0)), hy = h->y + h->vy * (t - (h->t0 > 0 ? h->t0 : 0));
            if (h->box) {
                if (fabsf(x - hx) < h->hw + FPL_SHIP_R * 0.8f && fabsf(y - hy) < h->hh + FPL_SHIP_R * 0.8f) return t;
                continue;
            }
            float r = h->r + FPL_SHIP_R + 0.08f;
            float ddx = x - hx, ddy = y - hy;
            if (ddx * ddx + ddy * ddy < r * r) return t;
        }
    }
    return H + 1;
}

/* ------------------------------------------------------------------ */
/* targets                                                              */

static bool killable(const FplFoe *f) { return f->alive && f->t >= 0 && FPL_FOE[f->kind].hp > 0; }

/* a foe on the plane for the side blaster: where to sit and which way to fire */
static bool side_target(float *tx, float *ty, int *fdx, int *fdy, float *best_d) {
    bool found = false;
    *best_d = 99;
    for (int i = 0; i < FPL_MAX_FOES; i++) {
        const FplFoe *f = &fpg.foe[i];
        if (!killable(f) || f->z > FPL_PLANE_Z || f->state == FS_OUT) continue;
        if (f->x < -3.2f || f->x > 3.2f) continue;
        float dx = f->x - fpg.x, dy = f->y - fpg.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d >= *best_d) continue;
        *best_d = d;
        found = true;
        /* line up on its row, or its column, from a little way off */
        if (fabsf(dy) <= fabsf(dx) || fabsf(f->x) > 2.4f) {
            float side = dx > 0 ? -1.0f : 1.0f;
            if (f->x + side * 1.4f < -FPL_SHIP_MX || f->x + side * 1.4f > FPL_SHIP_MX) side = -side;
            *tx = fabsf(dx) > 1.1f ? fpg.x : f->x + side * 1.4f;
            *ty = f->y;
            *fdx = *tx < f->x ? 1 : -1;
            *fdy = 0;
        } else {
            float side = dy > 0 ? -1.0f : 1.0f;
            if (f->y + side * 1.1f < -FPL_SHIP_MY || f->y + side * 1.1f > FPL_SHIP_MY) side = -side;
            *tx = f->x;
            *ty = fabsf(dy) > 0.9f ? fpg.y : f->y + side * 1.1f;
            *fdx = 0;
            *fdy = *ty < f->y ? 1 : -1;
        }
    }
    return found;
}

/* something in the distance for the forward gun: the lane to sit in */
static bool fwd_target(float *tx, float *ty) {
    float best = 1e9f;
    bool found = false;
    for (int i = 0; i < FPL_MAX_FOES; i++) {
        const FplFoe *f = &fpg.foe[i];
        if (!killable(f) || f->z <= FPL_PLANE_Z + 0.03f) continue;
        float fy = f->y;
        if (f->kind == EK_PENDULUM || f->kind == EK_SPITE) {
            /* where it will be when a shot gets there */
            float n = f->z / FPL_FWD_VZ;
            fy = f->y + f->vy * n;
            while (fy > 1.5f || fy < -1.5f) fy = fy > 1.5f ? 3.0f - fy : -3.0f - fy;
        }
        float lx = fpl_lane_x(fpl_col(f->x)), ly = fpl_lane_y(fpl_row(fy));
        float cost = fabsf(lx - fpg.x) + fabsf(ly - fpg.y) + f->z * 1.5f;
        if (f->kind == EK_SALLY || f->kind == EK_QUICKSALLY) cost -= 1.0f;
        if (f->kind == EK_CROSSHEAD || f->kind == EK_FORKER || f->kind == EK_BROODER) cost -= 0.6f;
        if (cost < best) { best = cost; *tx = lx; *ty = ly; found = true; }
    }
    for (int i = 0; i < fpg.bonus_spawned && fpg.state == PS_BONUS; i++) {
        const FplBalloon *b = &fpg.bal[i];
        if (!b->alive || b->z <= FPL_PLANE_Z + 0.05f) continue;
        float n = b->z / FPL_FWD_VZ, by = b->y - 0.0075f * n;
        float lx = fpl_lane_x(fpl_col(b->x)), ly = fpl_lane_y(fpl_row(by));
        float cost = fabsf(lx - fpg.x) + fabsf(ly - fpg.y) + b->z * 2.0f - (b->orange ? 1.2f : 0);
        if (cost < best) { best = cost; *tx = lx; *ty = ly; found = true; }
    }
    return found;
}

static int bot_play(void) {
    int m = 0;
    if (!fpg.alive) return 0;
    gather();
    float tx = 0, ty = 0.6f;
    bool want_fwd = false, want_side = false;
    int sdx = 0, sdy = 0;
    float sd;
    const FplBoss *b = &fpg.boss;
    if (side_target(&tx, &ty, &sdx, &sdy, &sd) && sd < 3.0f) {
        want_side = true;
    } else if (b->on && !b->dead && fpg.state == PS_BOSS) {
        float wx, wy, wr;
        fpl_boss_weak(&wx, &wy, &wr);
        tx = fpl_lane_x(fpl_col(wx));
        ty = fpl_lane_y(fpl_row(wy));
        want_fwd = wr > 0;
        if (b->kind == BOSS_SHELLBACK || b->kind == BOSS_KNUCKLEBELL) { /* sit in the lane, as low as the lane allows */ }
        if (b->kind == BOSS_KNUCKLEBELL) {
            /* knock back whichever fist is closer if it's near */
            for (int k = 0; k < 2; k++)
                if (fabsf(b->fx[k] - fpg.x) < 1.8f && fabsf(b->fy - fpg.y) < 0.8f) {
                    want_side = true;
                    want_fwd = false;
                    sdx = b->fx[k] > fpg.x ? 1 : -1;
                    sdy = 0;
                }
        }
    } else if (fwd_target(&tx, &ty)) {
        want_fwd = true;
    }
    /* steer: the safest of the nine ways, then the nearest to the target */
    int best_dx = 0, best_dy = 0, best_hit = -1;
    float best_cost = 1e9f;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int hit = first_hit(fpg.x, fpg.y, dx, dy, H);
            int hit2 = first_hit(fpg.x, fpg.y, dx, dy, 6);
            int h = imax(hit, hit2);
            float sp = FPL_SHIP_SPEED * ((dx && dy) ? 0.7071f : 1.0f);
            float nx = fclamp(fpg.x + dx * sp * 6, -FPL_SHIP_MX, FPL_SHIP_MX), ny = fclamp(fpg.y + dy * sp * 6, -FPL_SHIP_MY, FPL_SHIP_MY);
            float cost = fabsf(nx - tx) + fabsf(ny - ty);
            if (fabsf(nx - tx) < 0.04f && fabsf(ny - ty) < 0.04f) cost -= 0.01f;
            if (h > best_hit || (h == best_hit && cost < best_cost)) { best_hit = h; best_cost = cost; best_dx = dx; best_dy = dy; }
        }
    /* don't jitter on the target */
    if (best_hit > H && fabsf(fpg.x - tx) < 0.05f && fabsf(fpg.y - ty) < 0.05f && first_hit(fpg.x, fpg.y, 0, 0, H) > H) best_dx = best_dy = 0;
    if (best_dx > 0) m |= BTN_RIGHT;
    if (best_dx < 0) m |= BTN_LEFT;
    if (best_dy > 0) m |= BTN_DOWN;
    if (best_dy < 0) m |= BTN_UP;
    fpl_bot_dbg[0] = best_hit;
    fpl_bot_dbg[1] = want_side ? 2 : want_fwd ? 1 : 0;
    /* guns */
    if (want_side) {
        bool aligned = sdx ? fabsf(fpg.y - ty) < 0.3f : fabsf(fpg.x - tx) < 0.3f;
        if (fpg.side_locked && (fpg.prev_in & BTN_A)) {
            if (fpg.side_dx == sdx && fpg.side_dy == sdy) m |= BTN_A;   /* keep firing that way */
            /* else let go, and aim afresh next frame */
        } else if (aligned) {
            /* press A while moving away from the target: the blaster fires toward it */
            m &= ~(BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN);
            if (sdx > 0) m |= BTN_LEFT;
            if (sdx < 0) m |= BTN_RIGHT;
            if (sdy > 0) m |= BTN_UP;
            if (sdy < 0) m |= BTN_DOWN;
            m |= BTN_A;
        }
    } else if (want_fwd) {
        if (fpl_col(fpg.x) == fpl_col(tx) && fpl_row(fpg.y) == fpl_row(ty)) m |= BTN_B;
    }
    return m;
}

int fpl_bot_buttons(void) {
    switch (fpg.state) {
    case PS_TITLE:
        return fpg.state_t > 20 && (fpg.state_t / 4) % 2 ? BTN_A : 0;
    case PS_TALLY:
        return fpg.state_t > 210 && (fpg.state_t / 4) % 2 ? BTN_A : 0;
    case PS_ENDING:
        return fpg.state_t > 310 && (fpg.state_t / 4) % 2 ? BTN_A : 0;
    case PS_OVER:
        return 0;
    case PS_RADIO: case PS_WAVE: case PS_GRADE: case PS_BOSS: case PS_BOSS_DOWN:
    case PS_BONUS_IN: case PS_BONUS: case PS_BONUS_END:
        return bot_play();
    default:
        return 0;
    }
}
