/* TURNIP TRUCK - one workday: the truck, the clock, the deliveries, the
 * crates, the hazards and the traffic. The six chaos events are in
 * tnp_events.c.
 *
 * The truck turns relative to itself: LEFT turns it to its own left, which
 * is the screen's right when it drives down the screen. It is slippery
 * (sideways speed fades slowly), can't turn from a stand, brakes and then
 * reverses slowly, sidesteps on a tapped turn, powerslides with A+B and a
 * turn at speed, and spins out if the slide is held too long. Walls only
 * bounce and spin it; cars, canisters, brine and the chaos cost a heart. A
 * stretch at top speed brings a heart back, and every delivery heals it. */
#include "tnp.h"

TnpDay tnp;
bool tnp_quiet;

static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};

void tnp_sfx(const char *name) {
    if (!tnp_quiet) sfx_play_name(name);
}

float tnp_forward(const TnpTruck *t) { return t->vx * tnp_cos(t->ang) + t->vy * tnp_sin(t->ang); }
float tnp_speed(const TnpTruck *t) { return sqrtf(t->vx * t->vx + t->vy * t->vy); }
int tnp_seconds(const TnpDay *d) { return (d->time_f + 59) / 60; }

bool tnp_spin_attack(const TnpTruck *t) {
    int k = TNP_SPIN_T - t->spin_t; /* frames into the spin */
    return t->spin_t > 0 && k >= TNP_SPIN_ATTACK0 && k < TNP_SPIN_ATTACK1;
}

void tnp_dest_pos(int i, float *x, float *y) {
    *x = tnp_cx(TNP_DEST[i].cx);
    *y = tnp_cx(TNP_DEST[i].cy);
}

bool tnp_in_hq_zone(float x, float y) {
    float dx = tnp_wrapd(x - TNP_HQ_DOOR_X), dy = tnp_wrapd(y - TNP_HQ_DOOR_Y);
    return dy >= 0 && dx * dx + dy * dy < TNP_HQ_R * TNP_HQ_R;
}

void tnp_burst(TnpDay *d, float x, float y, int col, int n, float sp) {
    for (int i = 0; i < TNP_MAX_PARTS && n > 0; i++) {
        TnpPart *p = &d->part[i];
        if (p->life > 0) continue;
        float a = rng_float(&d->rng) * 6.2831853f, s = sp * (0.3f + rng_float(&d->rng));
        p->x = x;
        p->y = y;
        p->vx = tnp_cos(a) * s;
        p->vy = tnp_sin(a) * s;
        p->life = (int16_t)(16 + rng_range(&d->rng, 0, 14));
        p->col = (int16_t)col;
        n--;
    }
}

/* ---- the day ------------------------------------------------------------------------ */

void tnp_pick_dest(TnpDay *d) {
    int cand[TNP_DESTS], n = 0;
    for (int i = 0; i < TNP_DESTS; i++) {
        if (i == d->dest) continue;
        float x, y;
        tnp_dest_pos(i, &x, &y);
        if (tnp_dist(x, y, d->tr.x, d->tr.y) < 160) continue;
        cand[n++] = i;
    }
    d->prev_dest = d->dest;
    d->dest = n ? cand[rng_range(&d->rng, 0, n - 1)] : (d->dest + 1) % TNP_DESTS;
    d->dest_t = 0;
}

void tnp_day_begin(TnpDay *d, int day, int event, uint64_t seed, int practice, int flip) {
    memset(d, 0, sizeof *d);
    d->day = day;
    d->event = practice ? EV_NONE : event;
    d->practice = practice;
    rng_seed(&d->rng, seed);
    d->time_f = practice ? 0 : TNP_START_TIME * 60; /* the practice drive has no clock */
    TnpTruck *t = &d->tr;
    t->x = TNP_HQ_DOOR_X;
    t->y = TNP_HQ_DOOR_Y + TNP_CELL / 2;
    t->ang = 0;
    t->hearts = TNP_HEARTS;
    t->flip_reverse = (uint8_t)(flip != 0);
    for (int i = 0; i < TNP_MAX_CRATES; i++) d->crate_ok[i] = 1;
    for (int i = 0; i < TNP_MAX_CANS; i++) d->cans_ok[i] = 1;
    d->dest = -1;
    d->prev_dest = -1;
    tnp_pick_dest(d);
    tnp_events_begin(d);
}

static bool in_lot(float x, float y) {
    int cx = (int)(tnp_wrap(x) / TNP_CELL), cy = (int)(tnp_wrap(y) / TNP_CELL);
    return cx >= TNP_LOT_X0 && cx < TNP_LOT_X0 + 4 && cy >= TNP_LOT_Y0 && cy < TNP_LOT_Y0 + 4;
}

static void wreck(TnpDay *d) {
    TnpTruck *t = &d->tr;
    if (t->state == TS_WRECK || t->state == TS_BOOM) return;
    t->state = TS_WRECK;
    t->state_t = 0;
    t->spin_t = 0;
    d->died_in_lot = in_lot(t->x, t->y);
    tnp_burst(d, t->x, t->y, C_ORANGE, 14, 1.6f);
    tnp_sfx("tnp_wreck");
}

/* a heart lost (why: 0 a car, 1 brine, 2 canisters, 3 the chaos) */
void tnp_hurt(TnpDay *d, int why) {
    TnpTruck *t = &d->tr;
    if (d->practice || d->god || t->state != TS_DRIVE) return;
    if (why != 1 && t->inv > 0) return;
    t->hearts--;
    t->inv = TNP_INV_T;
    t->regen_t = 0;
    d->hits++;
    d->hit_by[why & 3]++;
    d->face = FACE_SHOCK;
    d->face_t = 30;
    d->ev_hurt = 1;
    tnp_sfx("tnp_hurt");
    if (t->hearts <= 0) {
        t->hearts = 0;
        wreck(d);
    }
}

/* back on the road at the nearest drop zone after a fall into the brine */
void tnp_truck_respawn(TnpDay *d) {
    TnpTruck *t = &d->tr;
    int best = 0;
    float bd = 1e9f;
    for (int i = 0; i < tnp_n_drops; i++) {
        float dd = tnp_dist(t->x, t->y, tnp_cx(tnp_drop_c[i][0]), tnp_cx(tnp_drop_c[i][1]));
        if (dd < bd) { bd = dd; best = i; }
    }
    int cx = tnp_drop_c[best][0], cy = tnp_drop_c[best][1];
    t->x = tnp_cx(cx);
    t->y = tnp_cx(cy);
    t->vx = t->vy = t->spin = 0;
    t->spin_t = t->air_t = t->drift_t = 0;
    /* face along the road, away from the water */
    int pick = 0, score = -9;
    for (int k = 0; k < 4; k++) {
        int nx = cx + DX[k], ny = cy + DY[k];
        if (!tnp_drivable(nx, ny)) continue;
        int s = 1 + (tnp_drivable(nx + DX[k], ny + DY[k]) ? 1 : 0);
        if (s > score) { score = s; pick = k; }
    }
    t->ang = (float)pick * 1.5707963f;
    t->state = TS_DRIVE;
    t->state_t = 0;
    t->inv = TNP_INV_T;
}

/* ---- the truck ------------------------------------------------------------------------ */

static void bounce_off(TnpTruck *t, float nx, float ny, float pen, float e) {
    t->x += nx * pen;
    t->y += ny * pen;
    float vn = t->vx * nx + t->vy * ny;
    if (vn < 0) {
        t->vx -= (1 + e) * vn * nx;
        t->vy -= (1 + e) * vn * ny;
        float impact = -vn;
        if (impact > 0.7f) {
            float cross = tnp_cos(t->ang) * ny - tnp_sin(t->ang) * nx;
            t->spin += (cross >= 0 ? 1.0f : -1.0f) * impact * TNP_WALL_SPIN;
            t->regen_t = 0;
            if (t->hit_wall_t <= 0) tnp_sfx("tnp_bump");
            t->hit_wall_t = 12;
        }
    }
}

/* walls (buildings, trees, fences) only bounce and spin the truck */
static void collide_world(TnpDay *d) {
    TnpTruck *t = &d->tr;
    int cx = (int)floorf(t->x / TNP_CELL), cy = (int)floorf(t->y / TNP_CELL);
    for (int pass = 0; pass < 2; pass++)
        for (int oy = -1; oy <= 1; oy++)
            for (int ox = -1; ox <= 1; ox++) {
                int ux = cx + ox, uy = cy + oy;
                const TnpCell *c = tnp_cell(ux, uy);
                float x0 = (float)(ux * TNP_CELL), y0 = (float)(uy * TNP_CELL);
                if (tnp_solid_type(c->type)) {
                    float qx = fclamp(t->x, x0, x0 + TNP_CELL), qy = fclamp(t->y, y0, y0 + TNP_CELL);
                    float dx = t->x - qx, dy = t->y - qy, d2 = dx * dx + dy * dy;
                    if (d2 >= TNP_R * TNP_R) continue;
                    if (d2 < 1e-6f) {
                        /* inside: out by the shortest way */
                        float l = t->x - x0, r = x0 + TNP_CELL - t->x, u = t->y - y0, b = y0 + TNP_CELL - t->y;
                        float m = fminf(fminf(l, r), fminf(u, b));
                        if (m == l) bounce_off(t, -1, 0, l + TNP_R, TNP_BOUNCE);
                        else if (m == r) bounce_off(t, 1, 0, r + TNP_R, TNP_BOUNCE);
                        else if (m == u) bounce_off(t, 0, -1, u + TNP_R, TNP_BOUNCE);
                        else bounce_off(t, 0, 1, b + TNP_R, TNP_BOUNCE);
                        continue;
                    }
                    float dist = sqrtf(d2);
                    bounce_off(t, dx / dist, dy / dist, TNP_R - dist, TNP_BOUNCE);
                } else if (c->feat == FT_CANS && t->air_t <= 0 && pass == 0) {
                    int k = -1;
                    for (int i = 0; i < tnp_n_cans; i++)
                        if (tnp_cans_c[i][0] == tnp_wrapc(ux) && tnp_cans_c[i][1] == tnp_wrapc(uy)) k = i;
                    if (k < 0 || !d->cans_ok[k]) continue;
                    float px = x0 + TNP_CELL / 2, py = y0 + TNP_CELL / 2;
                    float dx = t->x - px, dy = t->y - py, d2 = dx * dx + dy * dy, rr = TNP_R + 9;
                    if (d2 < rr * rr) {
                        d->cans_ok[k] = 0;
                        tnp_burst(d, px, py, C_ORANGE, 18, 2.0f);
                        tnp_burst(d, px, py, C_YELLOW, 10, 1.4f);
                        tnp_sfx("tnp_boom");
                        float dist = sqrtf(fmaxf(d2, 1e-6f));
                        float nx = d2 > 1e-6f ? dx / dist : -tnp_cos(t->ang), ny = d2 > 1e-6f ? dy / dist : -tnp_sin(t->ang);
                        t->vx = nx * 1.8f;
                        t->vy = ny * 1.8f;
                        tnp_hurt(d, 2);
                    }
                }
            }
    /* the hydrants at the kerbs knock the truck about like a wall */
    for (int i = 0; i < tnp_n_pipes && t->air_t <= 0; i++) {
        float dx = tnp_wrapd(t->x - tnp_pipe_px[i][0]), dy = tnp_wrapd(t->y - tnp_pipe_px[i][1]);
        float d2 = dx * dx + dy * dy, rr = TNP_R + 6;
        if (d2 < rr * rr && d2 > 1e-6f) {
            float dist = sqrtf(d2);
            bounce_off(t, dx / dist, dy / dist, rr - dist, TNP_BOUNCE);
        }
    }
    t->x = tnp_wrap(t->x);
    t->y = tnp_wrap(t->y);
}

void tnp_spinout(TnpTruck *t, int dir) {
    t->spin_t = TNP_SPIN_T;
    t->spin_steer = 0;
    t->drift_t = 0;
    t->tap_dir = 0;
    t->spin = dir >= 0 ? 0.0001f : -0.0001f;
    tnp_sfx("tnp_skid");
}

static void truck_drive(TnpDay *d, uint16_t pad) {
    TnpTruck *t = &d->tr;
    bool A = (pad & BTN_A) != 0, B = (pad & BTN_B) != 0;
    int steer = ((pad & BTN_RIGHT) ? 1 : 0) - ((pad & BTN_LEFT) ? 1 : 0);
    uint16_t pressed = (uint16_t)(pad & ~t->prev);
    t->prev = pad;
    const TnpCell *cell = tnp_cell_at(t->x, t->y);
    bool rock = cell->type == CT_ROCK && t->air_t <= 0;

    if (t->inv > 0) t->inv--;
    if (t->hit_wall_t > 0) t->hit_wall_t--;

    if (t->air_t > 0) {
        /* in the air: no steering, no pedals, no drag */
        t->air_t--;
        t->x = tnp_wrap(t->x + t->vx);
        t->y = tnp_wrap(t->y + t->vy);
        collide_world(d);
        if (t->air_t == 0) tnp_sfx("tnp_land");
        return;
    }

    if (t->spin_t > 0) {
        /* a spin-out: no control; it slides on in a straight line */
        int k = TNP_SPIN_T - t->spin_t;
        float rate = k < TNP_SPIN_ATTACK0 ? 0.08f + 0.3f * (float)k / TNP_SPIN_ATTACK0
                   : k < TNP_SPIN_ATTACK1 ? 0.38f : 0.38f * (float)t->spin_t / (TNP_SPIN_T - TNP_SPIN_ATTACK1);
        t->ang += t->spin >= 0 ? rate : -rate;
        t->ang += (float)steer * TNP_SPIN_STEER; /* a little say in where the spin ends */
        t->spin_steer += (float)steer * TNP_SPIN_STEER;
        t->spin_t--;
        t->vx *= 0.988f;
        t->vy *= 0.988f;
        if (rock) { t->vx *= TNP_ROCK_DRAG; t->vy *= TNP_ROCK_DRAG; }
        t->x = tnp_wrap(t->x + t->vx);
        t->y = tnp_wrap(t->y + t->vy);
        collide_world(d);
        if (t->spin_t == 0) t->spin = 0;
        t->regen_t = 0;
        return;
    }

    float f0 = tnp_forward(t);
    /* the turn: a tap is a sidestep, a hold turns the wheel */
    if (steer != 0 && (pressed & (BTN_LEFT | BTN_RIGHT))) {
        t->tap_t = 0;
        t->tap_dir = (int16_t)steer;
        t->tap_ang = t->ang;
    }
    int jink = 0; /* the sidestep's way: +1 the truck's right, -1 its left */
    if (t->tap_dir != 0) {
        if (steer == t->tap_dir) t->tap_t++;
        else {
            if (t->tap_t <= TNP_TAP && !(A && B)) jink = t->tap_dir;
            t->tap_dir = 0;
        }
    }
    bool drifting = A && B && steer != 0 && (f0 > TNP_DRIFT_MIN || (t->drift_t > 0 && tnp_speed(t) > 0.6f));
    bool pivot = A && B && steer != 0 && !drifting && fabsf(f0) < 0.8f;

    float rate = 0;
    if (pivot) rate = TNP_PIVOT;
    else {
        rate = TNP_TURN * fminf(1.0f, fabsf(f0) / TNP_TURN_FULL);
        if (drifting) rate = TNP_TURN * TNP_DRIFT_TURN * fminf(1.0f, tnp_speed(t) / TNP_TURN_FULL);
        else if (A && f0 > 2.0f) rate *= TNP_GAS_TURN;
    }
    /* reversing steers like a steering wheel (the nose swings the other way); not in a slide or a turn on the spot */
    float sign = (f0 < -0.05f && !t->flip_reverse && !drifting && !pivot) ? -1.0f : 1.0f;
    t->ang += (float)steer * rate * sign;
    t->ang += t->spin;
    t->spin *= 0.88f;
    if (fabsf(t->spin) < 0.002f) t->spin = 0;
    if (jink) t->ang = t->tap_ang; /* a sidestep keeps the heading */
    if (t->ang > 3.14159265f) t->ang -= 6.2831853f;
    if (t->ang < -3.14159265f) t->ang += 6.2831853f;

    float ca = tnp_cos(t->ang), sa = tnp_sin(t->ang);
    float f = t->vx * ca + t->vy * sa, l = -t->vx * sa + t->vy * ca;
    float acc = TNP_ACC * (rock ? TNP_ROCK_GAS : 1.0f);
    if (drifting) {
        /* sliding: the pedals fight each other and the truck slides on, slowly losing speed */
        f *= 0.988f;
    } else if (pivot) {
        f = fapproach(f, 0, TNP_BRAKE);
    } else if (A && !B) {
        if (f < 0) f = fminf(0, f + TNP_BRAKE);
        else if (f < TNP_TOP) f = fminf(TNP_TOP, f + acc);
    } else if (B && !A) {
        if (f > 0.05f) f = fmaxf(0, f - TNP_BRAKE);
        else f = fmaxf(-TNP_REV_TOP, f - TNP_REV_ACC * (rock ? TNP_ROCK_GAS : 1.0f));
    } else if (A && B) {
        f = fapproach(f, 0, TNP_BRAKE * 0.5f);
    } else {
        f = fapproach(f, 0, TNP_COAST);
    }
    if (f > TNP_TOP) f = fapproach(f, TNP_TOP, 0.05f);
    l *= drifting ? TNP_DRIFT_GRIP : TNP_GRIP;
    if (jink) {
        l += (float)jink * (fabsf(f0) >= 0.5f ? TNP_JINK : TNP_JINK_STILL);
        tnp_sfx("tnp_jink");
    }
    t->vx = f * ca - l * sa;
    t->vy = f * sa + l * ca;
    if (rock) { t->vx *= TNP_ROCK_DRAG; t->vy *= TNP_ROCK_DRAG; }

    if (drifting) {
        if (++t->drift_t > TNP_DRIFT_LIMIT) tnp_spinout(t, steer);
    } else
        t->drift_t = 0;

    /* the chaos may push */
    float px = 0, py = 0;
    tnp_brine_push(d, t->x, t->y, &px, &py);
    t->vx += px;
    t->vy += py;

    t->x = tnp_wrap(t->x + t->vx);
    t->y = tnp_wrap(t->y + t->vy);
    collide_world(d);

    /* speed heals */
    float fwd = tnp_forward(t);
    if (fwd >= TNP_REGEN_SPEED && !rock && t->hearts < TNP_HEARTS) {
        if (++t->regen_t >= TNP_REGEN_T) {
            t->hearts++;
            t->regen_t = 0;
            d->ev_heal = 1;
            tnp_sfx("tnp_heal");
        }
    } else
        t->regen_t = 0;
}

static void truck_update(TnpDay *d, uint16_t pad) {
    TnpTruck *t = &d->tr;
    switch (t->state) {
    case TS_DRIVE: truck_drive(d, pad); break;
    case TS_SINK:
        t->state_t++;
        t->vx *= 0.8f;
        t->vy *= 0.8f;
        if (t->state_t >= 40) tnp_truck_respawn(d);
        break;
    case TS_WRECK:
    case TS_BOOM:
        t->state_t++;
        t->vx *= 0.93f;
        t->vy *= 0.93f;
        t->x = tnp_wrap(t->x + t->vx);
        t->y = tnp_wrap(t->y + t->vy);
        if (t->state_t % 8 == 0) tnp_burst(d, t->x, t->y, t->state_t % 16 ? C_ORANGE : C_GREY, 2, 0.6f);
        if (t->state_t >= 100) {
            d->phase = DP_FAIL;
            d->fail_reason = t->state == TS_BOOM ? FAIL_TIME : FAIL_WRECK;
            d->ev_fail = 1;
        }
        break;
    }
}

/* ramps, brine, crates, the delivery circle and the depot */
static void truck_ground(TnpDay *d) {
    TnpTruck *t = &d->tr;
    if (t->state != TS_DRIVE) return;
    int cx = (int)(t->x / TNP_CELL), cy = (int)(t->y / TNP_CELL);
    const TnpCell *c = tnp_cell(cx, cy);
    if (t->air_t <= 0 && t->spin_t <= 0 && c->feat == FT_RAMP) {
        float rx = (float)DX[c->ramp_dir], ry = (float)DY[c->ramp_dir];
        float along = t->vx * rx + t->vy * ry;
        float past = (t->x - tnp_cx(cx)) * rx + (t->y - tnp_cx(cy)) * ry;
        if (along >= TNP_RAMP_MIN && past > 0) {
            t->air_max = t->air_t = (int16_t)(14 + along * 18);
            tnp_sfx("tnp_ramp");
        }
    }
    if (t->air_t <= 0 && c->type == CT_BRINE) {
        d->falls++;
        tnp_burst(d, t->x, t->y, C_LIME, 12, 1.2f);
        tnp_sfx("tnp_splash");
        if (d->practice || d->god) {
            t->state = TS_SINK;
            t->state_t = 0;
        } else {
            t->hearts--;
            t->regen_t = 0;
            d->hits++;
            d->hit_by[1]++;
            d->face = FACE_SHOCK;
            d->face_t = 30;
            d->ev_hurt = 1;
            if (t->hearts <= 0) {
                t->hearts = 0;
                wreck(d);
            } else {
                t->state = TS_SINK;
                t->state_t = 0;
            }
        }
        return;
    }
    /* crates hang from balloons: only a truck in the air reaches one */
    for (int i = 0; i < tnp_n_crates && t->air_t > 0; i++) {
        if (!d->crate_ok[i]) continue;
        if (tnp_dist(t->x, t->y, tnp_cx(tnp_crate_c[i][0]), tnp_cx(tnp_crate_c[i][1])) < 16) {
            d->crate_ok[i] = 0;
            d->crates_broken++;
            if (!d->practice) {
                d->time_f += TNP_ADD_CRATE * 60;
                d->last_add = TNP_ADD_CRATE;
                d->add_t = 90;
            }
            tnp_burst(d, tnp_cx(tnp_crate_c[i][0]), tnp_cx(tnp_crate_c[i][1]), C_TAN, 12, 1.4f);
            tnp_sfx("tnp_crate");
        }
    }
    if (t->air_t > 0) return;
    float dx, dy;
    tnp_dest_pos(d->dest, &dx, &dy);
    if (tnp_dist(t->x, t->y, dx, dy) < TNP_DEST_R) {
        d->delivered++;
        if (!d->practice) {
            int add = d->delivered < TNP_QUOTA ? TNP_ADD_EARLY : d->delivered == TNP_QUOTA ? TNP_ADD_FIFTH : 0;
            d->time_f += add * 60;
            d->last_add = add;
            d->add_t = 90;
        }
        t->hearts = TNP_HEARTS;
        d->ev_delivered = 1;
        d->face = FACE_HAPPY;
        d->face_t = 45;
        tnp_burst(d, dx, dy, C_VIOLET, 14, 1.5f);
        tnp_burst(d, dx, dy, C_WHITE, 8, 1.0f);
        tnp_sfx(d->delivered == TNP_QUOTA ? "tnp_quota" : "tnp_deliver");
        tnp_pick_dest(d);
    }
    if (!d->practice && d->delivered >= TNP_QUOTA && tnp_in_hq_zone(t->x, t->y)) {
        d->phase = DP_DONE;
        d->ev_clock_out = 1;
        t->vx = t->vy = 0;
    }
}

/* ---- traffic: right-hand lanes, U-turns at road ends, a token effort to avoid you ---- */

static void car_target(const TnpCar *c, float *tx, float *ty) {
    float off = 12 + c->nudge;
    *tx = tnp_cx(c->ncx) + (float)(-DY[c->dir]) * off;
    *ty = tnp_cx(c->ncy) + (float)(DX[c->dir]) * off;
}

static int car_choose(TnpDay *d, TnpCar *c, int cx, int cy) {
    int opts[4], n = 0, back = (c->dir + 2) & 3;
    for (int k = 0; k < 4; k++)
        if (k != back && tnp_roadlike(cx + DX[k], cy + DY[k])) opts[n++] = k;
    if (n == 0) return back; /* the road ends: turn round */
    if (c->kind == CK_POLICE && c->partner >= 0 && d->car[c->partner].alive) {
        /* the police follow the gang car */
        const TnpCar *g = &d->car[c->partner];
        int best = opts[0];
        float bd = 1e9f;
        for (int i = 0; i < n; i++) {
            float dd = tnp_dist(tnp_cx(cx + DX[opts[i]]), tnp_cx(cy + DY[opts[i]]), g->x, g->y);
            if (dd < bd) { bd = dd; best = opts[i]; }
        }
        return best;
    }
    if (c->kind == CK_GANG && c->chase_t > 0) {
        /* after the truck: the way that gets closest to it */
        int best = opts[0];
        float bd = 1e9f;
        for (int i = 0; i < n; i++) {
            float dd = tnp_dist(tnp_cx(cx + DX[opts[i]]), tnp_cx(cy + DY[opts[i]]), d->tr.x, d->tr.y);
            if (dd < bd) { bd = dd; best = opts[i]; }
        }
        return best;
    }
    if (c->kind == CK_GANG && c->partner >= 0 && d->car[c->partner].alive) {
        /* the gang car flees: never the way towards its police car if it can help it */
        const TnpCar *p = &d->car[c->partner];
        int best = opts[rng_range(&d->rng, 0, n - 1)];
        float bd = tnp_dist(tnp_cx(cx + DX[best]), tnp_cx(cy + DY[best]), p->x, p->y);
        for (int i = 0; i < n; i++) {
            float dd = tnp_dist(tnp_cx(cx + DX[opts[i]]), tnp_cx(cy + DY[opts[i]]), p->x, p->y);
            if (dd > bd + 20) { bd = dd; best = opts[i]; }
        }
        return best;
    }
    /* mostly straight on */
    for (int i = 0; i < n; i++)
        if (opts[i] == c->dir && rng_chance(&d->rng, 55)) return c->dir;
    return opts[rng_range(&d->rng, 0, n - 1)];
}

int tnp_car_spawn(TnpDay *d, int kind, int cx, int cy, int dir) {
    for (int i = 0; i < TNP_MAX_CARS; i++) {
        TnpCar *c = &d->car[i];
        if (c->alive) continue;
        memset(c, 0, sizeof *c);
        c->alive = 1;
        c->kind = (int16_t)kind;
        c->dir = (int16_t)dir;
        c->partner = -1;
        cx = tnp_wrapc(cx);
        cy = tnp_wrapc(cy);
        c->x = tnp_cx(cx) + (float)(-DY[dir]) * 12;
        c->y = tnp_cx(cy) + (float)(DX[dir]) * 12;
        c->ncx = (int16_t)(cx + DX[dir]);
        c->ncy = (int16_t)(cy + DY[dir]);
        c->ang = (float)dir * 1.5707963f;
        c->speed = kind == CK_TRAFFIC ? 1.3f : 2.1f;
        c->col = (int16_t)rng_range(&d->rng, 0, 5);
        c->fire_t = (int16_t)rng_range(&d->rng, 20, 60);
        return i;
    }
    return -1;
}

/* a cell just out of sight where a car may appear (not ahead of you going your way) */
bool tnp_spawn_spot(TnpDay *d, int *ocx, int *ocy, int *odir, float dmin, float dmax) {
    const TnpTruck *t = &d->tr;
    int tcx = (int)(t->x / TNP_CELL), tcy = (int)(t->y / TNP_CELL);
    float sp = tnp_speed(t);
    float hx = sp > 0.5f ? t->vx / sp : tnp_cos(t->ang), hy = sp > 0.5f ? t->vy / sp : tnp_sin(t->ang);
    for (int tries = 0; tries < 16; tries++) {
        int cx = tcx + rng_range(&d->rng, -7, 7), cy = tcy + rng_range(&d->rng, -6, 6);
        if (!tnp_roadlike(cx, cy)) continue;
        float rx = tnp_wrapd(tnp_cx(cx) - t->x), ry = tnp_wrapd(tnp_cx(cy) - t->y);
        float dd = sqrtf(rx * rx + ry * ry);
        if (dd < dmin || dd > dmax) continue;
        if (fabsf(rx) < 150 && fabsf(ry) < 110) continue; /* in sight */
        int opts[4], n = 0;
        for (int k = 0; k < 4; k++)
            if (tnp_roadlike(cx + DX[k], cy + DY[k])) opts[n++] = k;
        if (!n) continue;
        int dir = opts[rng_range(&d->rng, 0, n - 1)];
        bool ahead = rx * hx + ry * hy > 0;
        bool same_way = (float)DX[dir] * hx + (float)DY[dir] * hy > 0.5f;
        if (ahead && same_way) continue;
        *ocx = tnp_wrapc(cx);
        *ocy = tnp_wrapc(cy);
        *odir = dir;
        return true;
    }
    return false;
}

static void car_hit_truck(TnpDay *d, TnpCar *c) {
    TnpTruck *t = &d->tr;
    if (t->state != TS_DRIVE || t->air_t > 0 || c->wreck_t > 0) return;
    float dx = tnp_wrapd(t->x - c->x), dy = tnp_wrapd(t->y - c->y), d2 = dx * dx + dy * dy;
    float rr = TNP_CAR_R * 2;
    if (d2 >= rr * rr) return;
    if (tnp_spin_attack(t)) {
        /* the spin attack: the car is done for, the truck is fine */
        c->wreck_t = 90;
        tnp_burst(d, c->x, c->y, C_ORANGE, 12, 1.6f);
        tnp_sfx("tnp_smash");
        return;
    }
    float dist = sqrtf(fmaxf(d2, 1e-4f)), nx = dist > 0.01f ? dx / dist : 1, ny = dist > 0.01f ? dy / dist : 0;
    if (c->kind != CK_TRAFFIC) {
        /* the gang's and the police's cars careen about but do no damage by contact: a bounce */
        t->x = tnp_wrap(t->x + nx * (rr - dist));
        t->y = tnp_wrap(t->y + ny * (rr - dist));
        float vn2 = t->vx * nx + t->vy * ny;
        if (vn2 < 0) {
            t->vx -= 1.4f * vn2 * nx;
            t->vy -= 1.4f * vn2 * ny;
        }
        t->vx += nx * 0.6f;
        t->vy += ny * 0.6f;
        if (t->hit_wall_t <= 0) tnp_sfx("tnp_bump");
        t->hit_wall_t = 12;
        return;
    }
    /* how fast the two came together */
    float cvx = tnp_cos(c->ang) * c->speed, cvy = tnp_sin(c->ang) * c->speed;
    float closing = -((t->vx - cvx) * nx + (t->vy - cvy) * ny);
    t->x = tnp_wrap(t->x + nx * (rr - dist));
    t->y = tnp_wrap(t->y + ny * (rr - dist));
    float vn = t->vx * nx + t->vy * ny;
    if (closing < TNP_CRASH_MIN) {
        /* a nudge at walking pace: pushed apart, no harm */
        if (vn < 0) {
            t->vx -= vn * nx;
            t->vy -= vn * ny;
        }
        t->vx += nx * 0.15f;
        t->vy += ny * 0.15f;
        c->speed *= 0.5f;
        return;
    }
    if (vn < 0) {
        t->vx -= 1.5f * vn * nx;
        t->vy -= 1.5f * vn * ny;
    }
    t->vx += nx * 0.8f;
    t->vy += ny * 0.8f;
    c->speed = 0;
    c->bump_t = 50;
    if (t->inv <= 0) tnp_sfx("tnp_crash");
    tnp_hurt(d, 0);
}

void tnp_traffic_update(TnpDay *d) {
    TnpTruck *t = &d->tr;
    int n_traffic = 0;
    for (int i = 0; i < TNP_MAX_CARS; i++) {
        TnpCar *c = &d->car[i];
        if (!c->alive) continue;
        if (c->kind == CK_TRAFFIC) n_traffic++;
        if (c->wreck_t > 0) {
            if (--c->wreck_t == 0) {
                c->alive = 0;
                if (c->partner >= 0) d->car[c->partner].partner = -1;
            }
            if (c->wreck_t % 10 == 0) tnp_burst(d, c->x, c->y, C_GREY, 1, 0.4f);
            continue;
        }
        if (c->kind == CK_TRAFFIC && tnp_dist(c->x, c->y, t->x, t->y) > 470) {
            c->alive = 0;
            if (c->partner >= 0) d->car[c->partner].partner = -1;
            continue;
        }
        float want = c->kind == CK_TRAFFIC ? 1.3f : 2.1f;
        /* a token effort: slow down and pull over if the truck is (or is about to be) just ahead */
        float hx = tnp_cos(c->ang), hy = tnp_sin(c->ang);
        bool wary = false;
        for (int k = 0; k < 2 && c->kind == CK_TRAFFIC; k++) {
            float rx = tnp_wrapd(t->x + t->vx * (float)(k * 14) - c->x), ry = tnp_wrapd(t->y + t->vy * (float)(k * 14) - c->y);
            float ahead = rx * hx + ry * hy, side = fabsf(-rx * hy + ry * hx);
            if (ahead > -4 && ahead < 46 && side < 18) {
                want = fminf(want, ahead < 28 ? 0 : 0.4f);
                wary = true;
            }
        }
        c->nudge = wary ? fapproach(c->nudge, 7, 0.4f) : fapproach(c->nudge, 0, 0.2f);
        if (c->kind == CK_TRAFFIC) {
            /* a car kept waiting creeps on and edges the truck aside (too slowly to hurt) */
            if (wary && want < 0.1f) {
                if (++c->fire_t > 70) want = 0.45f;
                if (c->fire_t > 150) c->fire_t = 0;
            } else
                c->fire_t = 0;
        }
        /* queue behind a car going the same way */
        for (int j = 0; j < TNP_MAX_CARS; j++) {
            const TnpCar *o = &d->car[j];
            if (j == i || !o->alive || o->dir != c->dir) continue;
            float ox = tnp_wrapd(o->x - c->x), oy = tnp_wrapd(o->y - c->y);
            float a = ox * hx + oy * hy, s = fabsf(-ox * hy + oy * hx);
            if (a > 0 && a < 22 && s < 10) want = 0;
        }
        if (c->bump_t > 0) {
            c->bump_t--;
            want = 0;
        }
        c->speed = fapproach(c->speed, want, want < c->speed ? 0.12f : 0.05f);
        float tx, ty;
        car_target(c, &tx, &ty);
        float vx = tnp_wrapd(tx - c->x), vy = tnp_wrapd(ty - c->y), dd = sqrtf(vx * vx + vy * vy);
        if (dd <= c->speed + 0.6f) {
            /* at the next cell: pick the way on */
            int cx = tnp_wrapc(c->ncx), cy = tnp_wrapc(c->ncy);
            c->dir = (int16_t)car_choose(d, c, cx, cy);
            c->ncx = (int16_t)tnp_wrapc(cx + DX[c->dir]);
            c->ncy = (int16_t)tnp_wrapc(cy + DY[c->dir]);
        } else if (dd > 0.01f) {
            c->x = tnp_wrap(c->x + vx / dd * c->speed);
            c->y = tnp_wrap(c->y + vy / dd * c->speed);
            float want_ang = tnp_atan2(vy, vx), da = want_ang - c->ang;
            while (da > 3.14159265f) da -= 6.2831853f;
            while (da < -3.14159265f) da += 6.2831853f;
            c->ang += da * 0.25f;
        }
        car_hit_truck(d, c);
    }
    /* keep the streets busy round the truck */
    if (!d->no_traffic && d->frame % 15 == 0 && n_traffic < 12) {
        int cx, cy, dir;
        if (tnp_spawn_spot(d, &cx, &cy, &dir, 170, 380) && tnp_car_spawn(d, CK_TRAFFIC, cx, cy, dir) >= 0) {
            /* (counted for the tests: none may appear ahead of a moving truck going its way) */
            float sp = tnp_speed(t);
            float rx = tnp_wrapd(tnp_cx(cx) - t->x), ry = tnp_wrapd(tnp_cx(cy) - t->y);
            d->spawned++;
            if (sp > 0.5f && rx * t->vx + ry * t->vy > 0 && ((float)DX[dir] * t->vx + (float)DY[dir] * t->vy) / sp > 0.5f)
                d->spawn_bad++;
        }
    }
}

/* ---- one frame ------------------------------------------------------------------------- */

void tnp_step(TnpDay *d, uint16_t pad) {
    d->ev_delivered = d->ev_clock_out = d->ev_fail = d->ev_heal = d->ev_hurt = 0;
    if (d->phase != DP_PLAY) return;
    d->frame++;
    d->dest_t++;
    if (d->face_t > 0 && --d->face_t == 0) d->face = FACE_DRIVE;
    if (d->add_t > 0) d->add_t--;
    TnpTruck *t = &d->tr;
    if (!d->practice && (t->state == TS_DRIVE || t->state == TS_SINK)) {
        if (d->time_f > 0) d->time_f--;
        if (d->time_f <= 0) {
            /* out of time: the truck catches fire */
            t->state = TS_BOOM;
            t->state_t = 0;
            t->spin_t = t->air_t = 0;
            d->died_in_lot = in_lot(t->x, t->y);
            tnp_burst(d, t->x, t->y, C_RED, 16, 1.8f);
            tnp_sfx("tnp_wreck");
        }
    }
    truck_update(d, pad);
    truck_ground(d);
    tnp_traffic_update(d);
    if (!d->no_events) tnp_events_update(d);
    for (int i = 0; i < TNP_MAX_PARTS; i++) {
        TnpPart *p = &d->part[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vx *= 0.92f;
        p->vy *= 0.92f;
    }
}
