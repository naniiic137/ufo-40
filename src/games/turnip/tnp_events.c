/* TURNIP TRUCK - the six chaos days. Monday is quiet; from Tuesday to
 * Sunday each day brings one of these, each once a week in a shuffled order:
 *
 *   BRINE BURST  the pipes at the crossings spray brine across the roads and
 *                push the truck; waves of brine roll along the Pickle Works'
 *                streets. No damage, only the shove.
 *   THE BIG BEET a giant beetroot dashes round town after you; a touch is a
 *                heart, and it keeps coming while you are slow.
 *   MUSH MOB     crowds of mushmen shuffle towards the truck; running one
 *                over is harmless but slows you a lot, and a slow truck
 *                they gather round loses hearts.
 *   RADISH RING  the gang's cars flee the police all over town and shoot at
 *                you when you are close; their aim is poor.
 *   DOWNPOUR     rain all day, the music drowned out; puddles come and go on
 *                every road and spin the truck out if it hits one fast
 *                (reversing through them is safe).
 *   MOON RAID    saucers of three sizes wander the sky and bomb where you
 *                are when you come near; the big one is slow to fire.
 */
#include "tnp.h"

const char *const TNP_EVENT_NAME[EV_COUNT] = {
    "NONE", "BRINE BURST", "THE BIG BEET", "MUSH MOB", "RADISH RING", "DOWNPOUR", "MOON RAID",
};

static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};

/* ---- brine burst ------------------------------------------------------------------- */

#define SPRAY_CYCLE 240
#define SPRAY_LEN 64.0f
#define SPRAY_HALF 9.0f
#define SPRAY_PUSH 0.10f
const int TNP_WAVE_ROW[TNP_WAVES] = {22, 24, 28};
#define WAVE_PERIOD 420
#define WAVE_SPEED 1.6f
#define WAVE_W 22.0f
#define WAVE_END (11 * TNP_CELL)
#define WAVE_PUSH 0.13f

bool tnp_pipe_spraying(const TnpDay *d, int pipe, int dir) {
    if (d->event != EV_BRINE) return false;
    int cx = tnp_pipe_c[pipe][0], cy = tnp_pipe_c[pipe][1];
    if (!tnp_drivable(cx + DX[dir], cy + DY[dir])) return false;
    int p = (d->frame + pipe * 37) % SPRAY_CYCLE;
    if (dir == DIR_N || dir == DIR_S) return p < 100;
    return p >= 120 && p < 220;
}

int tnp_wave_band(const TnpDay *d, int lane, float *x0, float *x1) {
    if (d->event != EV_BRINE) return 0;
    int p = (d->frame + lane * 140) % WAVE_PERIOD;
    float front = -40 + (float)p * WAVE_SPEED;
    if (front - WAVE_W > WAVE_END) return 0;
    *x0 = front - WAVE_W;
    *x1 = front;
    return 1;
}

void tnp_brine_push(const TnpDay *d, float x, float y, float *fx, float *fy) {
    *fx = *fy = 0;
    if (d->event != EV_BRINE || d->tr.air_t > 0) return;
    for (int i = 0; i < tnp_n_pipes; i++) {
        float px = tnp_cx(tnp_pipe_c[i][0]), py = tnp_cx(tnp_pipe_c[i][1]);
        float rx = tnp_wrapd(x - px), ry = tnp_wrapd(y - py);
        if (fabsf(rx) > SPRAY_LEN + 8 || fabsf(ry) > SPRAY_LEN + 8) continue;
        for (int k = 0; k < 4; k++) {
            if (!tnp_pipe_spraying(d, i, k)) continue;
            float along = rx * (float)DX[k] + ry * (float)DY[k], side = fabsf(-rx * (float)DY[k] + ry * (float)DX[k]);
            if (along > 6 && along < SPRAY_LEN && side < SPRAY_HALF) {
                *fx += (float)DX[k] * SPRAY_PUSH;
                *fy += (float)DY[k] * SPRAY_PUSH;
            }
        }
    }
    for (int w = 0; w < TNP_WAVES; w++) {
        float x0, x1;
        if (!tnp_wave_band(d, w, &x0, &x1)) continue;
        float cy = tnp_cx(TNP_WAVE_ROW[w]);
        float ux = tnp_wrap(x);
        if (ux > WAVE_END + 100) ux -= TNP_WORLD; /* the works sit at the map's west edge */
        if (fabsf(tnp_wrapd(y - cy)) < TNP_CELL / 2 && ux >= x0 && ux <= x1) *fx += WAVE_PUSH;
    }
}

/* ---- helpers ------------------------------------------------------------------------ */

static TnpMob *mob_new(TnpDay *d, int kind, float x, float y) {
    for (int i = 0; i < TNP_MAX_MOBS; i++) {
        TnpMob *m = &d->mob[i];
        if (m->alive) continue;
        memset(m, 0, sizeof *m);
        m->alive = 1;
        m->kind = (int16_t)kind;
        m->x = tnp_wrap(x);
        m->y = tnp_wrap(y);
        m->fire_t = (int16_t)rng_range(&d->rng, 30, 90);
        m->t = (int16_t)rng_range(&d->rng, 0, 40);
        return m;
    }
    return NULL;
}

static int count_mobs(const TnpDay *d, int kind) {
    int n = 0;
    for (int i = 0; i < TNP_MAX_MOBS; i++) n += d->mob[i].alive && d->mob[i].kind == kind && d->mob[i].dead_t == 0;
    return n;
}

static void shot_new(TnpDay *d, int kind, float x, float y, float vx, float vy, int t) {
    for (int i = 0; i < TNP_MAX_SHOTS; i++) {
        TnpShot *s = &d->shot[i];
        if (s->alive) continue;
        s->alive = 1;
        s->kind = (int16_t)kind;
        s->x = x;
        s->y = y;
        s->vx = vx;
        s->vy = vy;
        s->t = (int16_t)t;
        return;
    }
}

/* a point round the truck, out of sight, at a distance in [dmin, dmax] */
static void around(TnpDay *d, float dmin, float dmax, float *x, float *y) {
    const TnpTruck *t = &d->tr;
    for (int k = 0; k < 20; k++) {
        float a = rng_float(&d->rng) * 6.2831853f, r = dmin + rng_float(&d->rng) * (dmax - dmin);
        float ox = cosf(a) * r, oy = sinf(a) * r;
        *x = tnp_wrap(t->x + ox);
        *y = tnp_wrap(t->y + oy);
        if (fabsf(ox) > 140 || fabsf(oy) > 100) return;
    }
}

static void push_truck(TnpTruck *t, float fromx, float fromy, float force) {
    float dx = tnp_wrapd(t->x - fromx), dy = tnp_wrapd(t->y - fromy), dd = sqrtf(dx * dx + dy * dy);
    if (dd < 0.01f) { dx = 1; dy = 0; dd = 1; }
    t->vx += dx / dd * force;
    t->vy += dy / dd * force;
}

/* ---- setting up a day ----------------------------------------------------------------------- */

void tnp_events_begin(TnpDay *d) {
    switch (d->event) {
    case EV_BEET: {
        float x = d->tr.x, y = d->tr.y;
        for (int k = 0; k < 30; k++) {
            around(d, 300, 360, &x, &y);
            if (tnp_drivable((int)(x / TNP_CELL), (int)(y / TNP_CELL))) break;
        }
        mob_new(d, MK_BEET, tnp_cx((int)(x / TNP_CELL)), tnp_cx((int)(y / TNP_CELL)));
        break;
    }
    case EV_MOON: {
        static const int KINDS[8] = {MK_SAUCER_L, MK_SAUCER_M, MK_SAUCER_M, MK_SAUCER_S,
                                     MK_SAUCER_S, MK_SAUCER_S, MK_SAUCER_S, MK_SAUCER_S};
        for (int i = 0; i < 8; i++) {
            float x, y;
            around(d, 220, 420, &x, &y);
            TnpMob *m = mob_new(d, KINDS[i], x, y);
            if (m) m->fire_t = (int16_t)rng_range(&d->rng, 90, 200);
        }
        break;
    }
    default: break;
    }
}

/* ---- every frame ----------------------------------------------------------------------- */

#define BEET_DASH 3.0f       /* a charge: faster than the truck, but only in bursts */
#define BEET_DASH_T 32
#define BEET_REST 80
#define BEET_REST_HIT 150
#define BEET_ROLL 1.6f       /* out of sight it rolls along the streets after you, slower than the truck */
#define BEET_SIGHT 170.0f

static bool clear_line(float x0, float y0, float x1, float y1) {
    float dx = tnp_wrapd(x1 - x0), dy = tnp_wrapd(y1 - y0), dd = sqrtf(dx * dx + dy * dy);
    int n = (int)(dd / 8) + 1;
    for (int i = 1; i <= n; i++) {
        float f = (float)i / (float)n;
        if (tnp_solid_type(tnp_cell_at(x0 + dx * f, y0 + dy * f)->type)) return false;
    }
    return true;
}

/* the way along the streets from cell (cx, cy) towards the truck */
static int street_dir(const TnpDay *d, int cx, int cy) {
    static int16_t dist[TNP_MAP * TNP_MAP];
    static int16_t q[TNP_MAP * TNP_MAP];
    for (int i = 0; i < TNP_MAP * TNP_MAP; i++) dist[i] = -1;
    int tx = (int)(d->tr.x / TNP_CELL), ty = (int)(d->tr.y / TNP_CELL), n = 0;
    dist[ty * TNP_MAP + tx] = 0;
    q[n++] = (int16_t)(ty * TNP_MAP + tx);
    for (int h = 0; h < n; h++) {
        int x = q[h] % TNP_MAP, y = q[h] / TNP_MAP;
        for (int k = 0; k < 4; k++) {
            int nx = tnp_wrapc(x + DX[k]), ny = tnp_wrapc(y + DY[k]);
            if (dist[ny * TNP_MAP + nx] >= 0 || tnp_solid_type(tnp_map[ny][nx].type)) continue;
            dist[ny * TNP_MAP + nx] = (int16_t)(dist[q[h]] + 1);
            q[n++] = (int16_t)(ny * TNP_MAP + nx);
        }
    }
    int best = -1, bd = dist[tnp_wrapc(cy) * TNP_MAP + tnp_wrapc(cx)];
    if (bd < 0) bd = 9999;
    for (int k = 0; k < 4; k++) {
        int v = dist[tnp_wrapc(cy + DY[k]) * TNP_MAP + tnp_wrapc(cx + DX[k])];
        if (v >= 0 && v < bd) { bd = v; best = k; }
    }
    return best;
}

static void beet_update(TnpDay *d, TnpMob *m) {
    TnpTruck *t = &d->tr;
    float dist = tnp_dist(m->x, m->y, t->x, t->y);
    if (dist > 520) {
        /* left far behind: it turns up again somewhere round the truck */
        for (int k = 0; k < 12; k++) {
            float x, y;
            around(d, 280, 330, &x, &y);
            if (!tnp_drivable((int)(x / TNP_CELL), (int)(y / TNP_CELL))) continue;
            m->x = tnp_cx((int)(x / TNP_CELL));
            m->y = tnp_cx((int)(y / TNP_CELL));
            break;
        }
        m->state = 0;
        m->t = 0;
        m->vx = m->vy = 0;
    }
    m->t++;
    bool sight = dist < BEET_SIGHT && clear_line(m->x, m->y, t->x, t->y);
    if (m->state == 0) {
        /* gathers itself, wobbling, then charges at the truck if it can see it */
        m->vx *= 0.85f;
        m->vy *= 0.85f;
        if (m->t >= m->fire_t) {
            if (sight) {
                float dx = tnp_wrapd(t->x - m->x), dy = tnp_wrapd(t->y - m->y), dd = sqrtf(dx * dx + dy * dy);
                if (dd < 1) dd = 1;
                m->vx = dx / dd * BEET_DASH;
                m->vy = dy / dd * BEET_DASH;
                m->state = 1;
            } else
                m->state = 2;
            m->t = 0;
        }
    } else if (m->state == 1) {
        if (m->t >= BEET_DASH_T) {
            m->state = 0;
            m->t = 0;
            m->fire_t = BEET_REST;
        }
    } else {
        /* rolling along the streets: at each cell's middle it picks the way towards the truck */
        int cx = (int)(m->x / TNP_CELL), cy = (int)(m->y / TNP_CELL);
        float mx = tnp_cx(cx), my = tnp_cx(cy);
        if (fabsf(tnp_wrapd(m->x - mx)) < BEET_ROLL && fabsf(tnp_wrapd(m->y - my)) < BEET_ROLL) {
            m->x = mx;
            m->y = my;
            int k = street_dir(d, cx, cy);
            m->vx = k >= 0 ? (float)DX[k] * BEET_ROLL : 0;
            m->vy = k >= 0 ? (float)DY[k] * BEET_ROLL : 0;
            if (sight && m->t > 20) {
                /* there you are */
                m->state = 0;
                m->t = 0;
                m->fire_t = 20;
            }
        } else if (fabsf(m->vx) + fabsf(m->vy) < 0.01f) {
            /* off the middle line: back to it */
            float dx = tnp_wrapd(mx - m->x), dy = tnp_wrapd(my - m->y), dd = sqrtf(dx * dx + dy * dy);
            m->vx = dx / dd * BEET_ROLL;
            m->vy = dy / dd * BEET_ROLL;
        }
    }
    if (m->state == 1 && m->t % 12 == 0 && dist < 260) tnp_sfx("tnp_beet");
    float nx = tnp_wrap(m->x + m->vx), ny = tnp_wrap(m->y + m->vy);
    if (tnp_solid_type(tnp_cell_at(nx, ny)->type)) {
        /* it can't go through buildings: a charge into a wall ends there */
        m->vx = m->vy = 0;
        if (m->state == 1) {
            m->state = 0;
            m->t = 0;
            m->fire_t = 40;
        }
    } else {
        m->x = nx;
        m->y = ny;
    }
    if (t->state == TS_DRIVE && t->air_t <= 0 && tnp_dist(m->x, m->y, t->x, t->y) < 13 + TNP_R) {
        if (t->inv <= 0) {
            push_truck(t, m->x, m->y, 1.6f);
            tnp_hurt(d, 3);
            /* it bounces off and gathers itself before the next charge */
            m->state = 0;
            m->t = 0;
            m->fire_t = BEET_REST_HIT;
            m->vx = -m->vx * 0.5f;
            m->vy = -m->vy * 0.5f;
        }
    }
}

static void mush_update(TnpDay *d, TnpMob *m) {
    TnpTruck *t = &d->tr;
    if (m->dead_t > 0) {
        if (--m->dead_t == 0) m->alive = 0;
        return;
    }
    float dist = tnp_dist(m->x, m->y, t->x, t->y);
    if (dist > 340) { m->alive = 0; return; }
    m->t++;
    float dx = tnp_wrapd(t->x - m->x), dy = tnp_wrapd(t->y - m->y);
    float sp = 0.35f;
    if (dist < 170 && dist > 0.5f) {
        m->vx = dx / dist * sp;
        m->vy = dy / dist * sp;
    } else if (m->t % 60 == 0) {
        float a = rng_float(&d->rng) * 6.2831853f;
        m->vx = cosf(a) * 0.2f;
        m->vy = sinf(a) * 0.2f;
    }
    float nx = tnp_wrap(m->x + m->vx), ny = tnp_wrap(m->y + m->vy);
    if (tnp_drivable((int)(nx / TNP_CELL), (int)(ny / TNP_CELL))) {
        m->x = nx;
        m->y = ny;
    }
    if (t->state != TS_DRIVE || t->air_t > 0) return;
    if (dist < TNP_R + 4) {
        float sp2 = tnp_speed(t);
        if (sp2 > 0.6f) {
            /* squashed: harmless, but it drags the truck down */
            m->dead_t = 40;
            d->squashed++;
            t->vx *= 0.55f;
            t->vy *= 0.55f;
            t->regen_t = 0;
            tnp_burst(d, m->x, m->y, C_CREAM, 5, 0.8f);
            tnp_sfx("tnp_squish");
        }
    }
}

#define BOMB_FUSE 50
#define BOMB_R 10

static void saucer_update(TnpDay *d, TnpMob *m) {
    TnpTruck *t = &d->tr;
    float sp = m->kind == MK_SAUCER_S ? 1.5f : m->kind == MK_SAUCER_M ? 1.0f : 0.6f;
    float range = m->kind == MK_SAUCER_S ? 100.0f : m->kind == MK_SAUCER_M ? 120.0f : 150.0f;
    int cool = m->kind == MK_SAUCER_S ? 80 : m->kind == MK_SAUCER_M ? 100 : 160;
    float dist = tnp_dist(m->x, m->y, t->x, t->y);
    if (dist > 520) {
        float x, y;
        around(d, 260, 360, &x, &y);
        m->x = x;
        m->y = y;
    }
    if (--m->t <= 0) {
        /* erratic: a new heading now and then, drawn back towards the truck when far */
        float a = rng_float(&d->rng) * 6.2831853f;
        m->vx = cosf(a) * sp;
        m->vy = sinf(a) * sp;
        if (dist > 180) {
            float dx = tnp_wrapd(t->x - m->x), dy = tnp_wrapd(t->y - m->y);
            m->vx = (m->vx + dx / dist * sp) * 0.6f;
            m->vy = (m->vy + dy / dist * sp) * 0.6f;
        }
        m->t = (int16_t)rng_range(&d->rng, 40, 70);
    }
    m->x = tnp_wrap(m->x + m->vx);
    m->y = tnp_wrap(m->y + m->vy);
    if (m->fire_t > 0) m->fire_t--;
    if (dist < range && m->fire_t <= 0 && t->state == TS_DRIVE) {
        float tx = t->x + (float)rng_range(&d->rng, -18, 18), ty = t->y + (float)rng_range(&d->rng, -18, 18);
        shot_new(d, SK_BOMB, tnp_wrap(tx), tnp_wrap(ty), m->x, m->y, BOMB_FUSE);
        m->fire_t = (int16_t)cool;
        tnp_sfx("tnp_zap");
    }
}

static void radish_update(TnpDay *d) {
    TnpTruck *t = &d->tr;
    int gangs = 0;
    for (int i = 0; i < TNP_MAX_CARS; i++) {
        TnpCar *c = &d->car[i];
        if (!c->alive || c->kind != CK_GANG) continue;
        gangs++;
        if (c->wreck_t > 0) continue;
        if (c->fire_t > 0) c->fire_t--;
        float dist = tnp_dist(c->x, c->y, t->x, t->y);
        if (dist < 84 && c->fire_t <= 0 && t->state == TS_DRIVE) {
            /* a poor shot: anywhere in a wide cone towards the truck */
            float a = atan2f(tnp_wrapd(t->y - c->y), tnp_wrapd(t->x - c->x)) + (rng_float(&d->rng) - 0.5f) * 1.2f;
            shot_new(d, SK_BULLET, c->x, c->y, cosf(a) * 2.4f, sinf(a) * 2.4f, 55);
            c->fire_t = 90;
            tnp_sfx("tnp_bang");
        }
    }
    if (gangs < 3 && d->frame % 40 == 0) {
        int cx, cy, dir;
        if (tnp_spawn_spot(d, &cx, &cy, &dir, 200, 420)) {
            int g = tnp_car_spawn(d, CK_GANG, cx, cy, dir);
            int p = g >= 0 ? tnp_car_spawn(d, CK_POLICE, cx, cy, dir) : -1;
            if (g >= 0 && p >= 0) {
                /* the police a few lengths behind */
                d->car[p].x = tnp_wrap(d->car[p].x - (float)DX[dir] * 22);
                d->car[p].y = tnp_wrap(d->car[p].y - (float)DY[dir] * 22);
                d->car[g].partner = (int16_t)p;
                d->car[p].partner = (int16_t)g;
            } else if (g >= 0)
                d->car[g].alive = 0;
        }
    }
}

static void rain_update(TnpDay *d) {
    TnpTruck *t = &d->tr;
    int n = 0;
    for (int i = 0; i < TNP_MAX_PUDDLES; i++) {
        TnpPuddle *p = &d->pud[i];
        if (!p->alive) continue;
        if (++p->t >= p->life || tnp_dist(p->x, p->y, t->x, t->y) > 360) { p->alive = 0; continue; }
        n++;
        bool slick = p->t >= 60 && p->t < p->life - 30;
        if (slick && t->state == TS_DRIVE && t->air_t <= 0 && t->spin_t <= 0 &&
            tnp_dist(p->x, p->y, t->x, t->y) < 8 + 3 && tnp_forward(t) > 1.5f)
            tnp_spinout(t, (d->frame & 1) ? 1 : -1);
    }
    /* new puddles anywhere on the roads round the truck */
    if (n < 28 && d->frame % 6 == 0) {
        for (int k = 0; k < 8; k++) {
            float x, y;
            around(d, 50, 300, &x, &y);
            int cx = (int)(x / TNP_CELL), cy = (int)(y / TNP_CELL);
            if (!tnp_roadlike(cx, cy) || tnp_dist(x, y, t->x, t->y) < 50) continue;
            for (int i = 0; i < TNP_MAX_PUDDLES; i++) {
                TnpPuddle *p = &d->pud[i];
                if (p->alive) continue;
                p->alive = 1;
                p->x = x;
                p->y = y;
                p->t = 0;
                p->life = (int16_t)rng_range(&d->rng, 600, 1000);
                break;
            }
            break;
        }
    }
}

static void mush_spawn(TnpDay *d) {
    if (count_mobs(d, MK_MUSH) >= 36 || d->frame % 4) return;
    for (int k = 0; k < 6; k++) {
        float x, y;
        around(d, 150, 280, &x, &y);
        int cx = (int)(x / TNP_CELL), cy = (int)(y / TNP_CELL);
        if (!tnp_drivable(cx, cy)) continue;
        mob_new(d, MK_MUSH, x, y);
        return;
    }
}

static void shots_update(TnpDay *d) {
    TnpTruck *t = &d->tr;
    for (int i = 0; i < TNP_MAX_SHOTS; i++) {
        TnpShot *s = &d->shot[i];
        if (!s->alive) continue;
        if (s->kind == SK_BULLET) {
            s->x = tnp_wrap(s->x + s->vx);
            s->y = tnp_wrap(s->y + s->vy);
            if (--s->t <= 0 || tnp_solid_type(tnp_cell_at(s->x, s->y)->type)) { s->alive = 0; continue; }
            if (t->state == TS_DRIVE && t->air_t <= 0 && tnp_dist(s->x, s->y, t->x, t->y) < TNP_R + 2) {
                s->alive = 0;
                tnp_burst(d, s->x, s->y, C_YELLOW, 4, 0.8f);
                tnp_hurt(d, 3);
            }
        } else {
            /* a bomb: x, y is where it lands; vx, vy where it fell from */
            if (--s->t <= 0) {
                s->alive = 0;
                tnp_burst(d, s->x, s->y, C_ORANGE, 10, 1.5f);
                tnp_burst(d, s->x, s->y, C_MAGENTA, 6, 1.0f);
                tnp_sfx("tnp_boom");
                if (t->state == TS_DRIVE && t->air_t <= 0 && tnp_dist(s->x, s->y, t->x, t->y) < BOMB_R + TNP_R) {
                    push_truck(t, s->x, s->y, 1.2f);
                    tnp_hurt(d, 3);
                }
            }
        }
    }
}

void tnp_events_update(TnpDay *d) {
    TnpTruck *t = &d->tr;
    if (d->event == EV_MUSH) mush_spawn(d);
    if (d->event == EV_RADISH) radish_update(d);
    if (d->event == EV_RAIN) rain_update(d);
    int swarm = 0;
    for (int i = 0; i < TNP_MAX_MOBS; i++) {
        TnpMob *m = &d->mob[i];
        if (!m->alive) continue;
        switch (m->kind) {
        case MK_BEET: beet_update(d, m); break;
        case MK_MUSH:
            mush_update(d, m);
            if (m->alive && m->dead_t == 0 && tnp_dist(m->x, m->y, t->x, t->y) < 12) swarm++;
            break;
        default: saucer_update(d, m); break;
        }
    }
    /* a slow truck with mushmen all round it loses hearts */
    if (swarm >= 3 && t->state == TS_DRIVE) {
        if (++d->swarm_t >= 50) {
            d->swarm_t = 0;
            tnp_hurt(d, 3);
        }
    } else
        d->swarm_t = 0;
    t->swarmed = (uint8_t)(swarm >= 3);
    shots_update(d);
}
