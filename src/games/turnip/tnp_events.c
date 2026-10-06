/* TURNIP TRUCK - the six chaos days. Monday is quiet; from Tuesday to
 * Sunday each day brings one of these, each once a week in a shuffled order:
 *
 *   BRINE BURST  the hydrants at the crossings' kerbs spray brine across the
 *                roads in turn and shove the truck; waves of brine sweep
 *                across three roads towards the brine beside them. No
 *                damage, only the shove (which can put you in the brine).
 *   THE BIG BEET a giant beetroot charges at the truck when it sees it and
 *                rolls after it along the streets when it doesn't; a touch
 *                is a heart, and a truck it corners is soon done for.
 *   MUSH MOB     hordes of mushmen shuffle about; they never hurt, but
 *                running one over slows the truck a lot and drags it onto
 *                the body, a jittery crawl through a crowd.
 *   RADISH RING  one or two chases at a time, gang cars fleeing police cars
 *                all over town, with quiet spells between; a gang car that
 *                sees the truck close by chases it and shoots, badly. Their
 *                cars do no harm by contact.
 *   DOWNPOUR     rain all day, the music drowned out; puddles appear in an
 *                instant on the roads (sometimes just ahead) and spin the
 *                truck out if it hits one fast; reversing through is safe.
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
#define SPRAY_LEN 56.0f
#define SPRAY_HALF 9.0f
#define SPRAY_PUSH 0.12f
/* the brine waves: each runs across one stretch of road towards the brine beside it */
const TnpWave TNP_WAVE[TNP_WAVES] = {
    {0, 24, 0, 10, DIR_N},  /* the works' street, into the channel north of it */
    {1, 0, 24, 28, DIR_W},  /* the dock road, into the harbour basin over the map's edge */
    {0, 18, 0, 10, DIR_S},  /* the riverside, into the canal */
};
#define WAVE_PERIOD 240
#define WAVE_SPEED 1.6f
#define WAVE_W 20.0f
#define WAVE_PUSH 0.10f

/* the way a hydrant sprays now (across the road, away from its kerb), or -1 */
int tnp_pipe_spray_dir(const TnpDay *d, int pipe) {
    if (d->event != EV_BRINE) return -1;
    int p = (d->frame + pipe * 37) % SPRAY_CYCLE;
    if (p < 100) return tnp_pipe_side[pipe][1] > 0 ? DIR_N : DIR_S;
    if (p >= 120 && p < 220) return tnp_pipe_side[pipe][0] > 0 ? DIR_W : DIR_E;
    return -1;
}

/* the band of a wave in the world (x, y, w, h); 0 while it rests */
int tnp_wave_rect(const TnpDay *d, int w, float *x, float *y, float *bw, float *bh) {
    if (d->event != EV_BRINE) return 0;
    const TnpWave *v = &TNP_WAVE[w];
    int p = (d->frame + w * 80) % WAVE_PERIOD;
    float front = (float)p * WAVE_SPEED;                 /* from the far kerb, across */
    if (front - WAVE_W > TNP_CELL + 16) return 0;
    float a0 = fmaxf(0, front - WAVE_W), a1 = fminf((float)TNP_CELL + 16, front);
    float len0 = (float)(v->a0 * TNP_CELL), len = (float)((v->a1 - v->a0 + 1) * TNP_CELL);
    float base = (float)(v->line * TNP_CELL); /* the road's top or left edge */
    switch (v->flow) {
    case DIR_N: *x = len0; *bw = len; *y = base + TNP_CELL - a1; *bh = a1 - a0; break;
    case DIR_S: *x = len0; *bw = len; *y = base + a0; *bh = a1 - a0; break;
    case DIR_W: *y = len0; *bh = len; *x = base + TNP_CELL - a1; *bw = a1 - a0; break;
    default: *y = len0; *bh = len; *x = base + a0; *bw = a1 - a0; break;
    }
    if (!v->vertical && (v->flow == DIR_E || v->flow == DIR_W)) return 0;
    return 1;
}

void tnp_brine_push(const TnpDay *d, float x, float y, float *fx, float *fy) {
    static const int SDX[4] = {1, 0, -1, 0}, SDY[4] = {0, 1, 0, -1};
    *fx = *fy = 0;
    if (d->event != EV_BRINE || d->tr.air_t > 0) return;
    for (int i = 0; i < tnp_n_pipes; i++) {
        int k = tnp_pipe_spray_dir(d, i);
        if (k < 0) continue;
        float rx = tnp_wrapd(x - tnp_pipe_px[i][0]), ry = tnp_wrapd(y - tnp_pipe_px[i][1]);
        float along = rx * (float)SDX[k] + ry * (float)SDY[k], side = fabsf(-rx * (float)SDY[k] + ry * (float)SDX[k]);
        if (along > 4 && along < SPRAY_LEN && side < SPRAY_HALF) {
            *fx += (float)SDX[k] * SPRAY_PUSH;
            *fy += (float)SDY[k] * SPRAY_PUSH;
        }
    }
    for (int w = 0; w < TNP_WAVES; w++) {
        float bx, by, bw, bh;
        if (!tnp_wave_rect(d, w, &bx, &by, &bw, &bh)) continue;
        float rx = tnp_wrapd(x - bx), ry = tnp_wrapd(y - by);
        if (rx >= 0 && rx <= bw && ry >= 0 && ry <= bh) {
            *fx += (float)SDX[TNP_WAVE[w].flow] * WAVE_PUSH;
            *fy += (float)SDY[TNP_WAVE[w].flow] * WAVE_PUSH;
        }
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
        float ox = tnp_cos(a) * r, oy = tnp_sin(a) * r;
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
    case EV_RADISH: d->gang_t = rng_range(&d->rng, 300, 600); break;
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
#define BEET_REST_HIT 68    /* about the truck's safety window: a cornered truck is done for in seconds */
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
        float ox = tnp_wrapd(mx - m->x), oy = tnp_wrapd(my - m->y);
        /* at the middle (reached, not just left behind) */
        if (fabsf(ox) < BEET_ROLL && fabsf(oy) < BEET_ROLL && ox * m->vx + oy * m->vy >= 0) {
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
            push_truck(t, m->x, m->y, 1.0f);
            tnp_hurt(d, 3);
            /* it stops dead and gathers itself, just long enough, before the next charge */
            m->state = 0;
            m->t = 0;
            m->fire_t = BEET_REST_HIT;
            m->vx = m->vy = 0;
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
        m->vx = tnp_cos(a) * 0.2f;
        m->vy = tnp_sin(a) * 0.2f;
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
            /* the truck lurches onto it: a jittery crawl through a crowd */
            t->x = tnp_wrap(t->x + tnp_wrapd(m->x - t->x) * 0.4f);
            t->y = tnp_wrap(t->y + tnp_wrapd(m->y - t->y) * 0.4f);
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
        m->vx = tnp_cos(a) * sp;
        m->vy = tnp_sin(a) * sp;
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

/* The Radish Ring: one or two chases at a time roam the whole city (not just round the
 * truck), a new one now and then, so there are long spells with no gang about. A gang car
 * that sees the truck close by goes after it for a while, shooting badly. */
#define GANG_SEE 100.0f
#define GANG_CHASE_T 180
#define GANG_FIRE_R 84.0f

static void radish_update(TnpDay *d) {
    TnpTruck *t = &d->tr;
    int gangs = 0;
    for (int i = 0; i < TNP_MAX_CARS; i++) {
        TnpCar *c = &d->car[i];
        if (!c->alive || c->kind != CK_GANG) continue;
        gangs++;
        if (c->wreck_t > 0) continue;
        float dist = tnp_dist(c->x, c->y, t->x, t->y);
        /* a chase ends some time after it began, out of sight: the pair goes */
        if (c->life > 0) c->life--;
        if (c->life == 0 && dist > 220) {
            c->alive = 0;
            if (c->partner >= 0) d->car[c->partner].alive = 0;
            continue;
        }
        if (dist < GANG_SEE && t->state == TS_DRIVE && clear_line(c->x, c->y, t->x, t->y)) c->chase_t = GANG_CHASE_T;
        else if (c->chase_t > 0) c->chase_t--;
        if (c->fire_t > 0) c->fire_t--;
        if (dist < GANG_FIRE_R && c->fire_t <= 0 && t->state == TS_DRIVE) {
            /* a poor shot: anywhere in a wide cone towards the truck */
            float a = tnp_atan2(tnp_wrapd(t->y - c->y), tnp_wrapd(t->x - c->x)) + (rng_float(&d->rng) - 0.5f) * 1.2f;
            shot_new(d, SK_BULLET, c->x, c->y, tnp_cos(a) * 2.4f, tnp_sin(a) * 2.4f, 55);
            c->fire_t = 90;
            tnp_sfx("tnp_bang");
        }
    }
    if (d->gang_t > 0) d->gang_t--;
    if (gangs < 2 && d->gang_t <= 0) {
        /* a new chase starts somewhere in town, out of sight */
        for (int k = 0; k < 40; k++) {
            int cx = rng_range(&d->rng, 0, TNP_MAP - 1), cy = rng_range(&d->rng, 0, TNP_MAP - 1);
            if (!tnp_roadlike(cx, cy) || tnp_dist(tnp_cx(cx), tnp_cx(cy), t->x, t->y) < 240) continue;
            int opts[4], n = 0;
            for (int q = 0; q < 4; q++)
                if (tnp_roadlike(cx + DX[q], cy + DY[q])) opts[n++] = q;
            if (!n) continue;
            int dir = opts[rng_range(&d->rng, 0, n - 1)];
            int g = tnp_car_spawn(d, CK_GANG, cx, cy, dir);
            int p = g >= 0 ? tnp_car_spawn(d, CK_POLICE, cx, cy, dir) : -1;
            if (g >= 0 && p >= 0) {
                /* the police a few lengths behind */
                d->car[p].x = tnp_wrap(d->car[p].x - (float)DX[dir] * 22);
                d->car[p].y = tnp_wrap(d->car[p].y - (float)DY[dir] * 22);
                d->car[g].partner = (int16_t)p;
                d->car[p].partner = (int16_t)g;
                d->car[g].life = (int16_t)rng_range(&d->rng, 1800, 3600);
            } else if (g >= 0)
                d->car[g].alive = 0;
            break;
        }
        d->gang_t = rng_range(&d->rng, 900, 1800);
    }
}

#define PUDDLE_IN 9 /* frames for a new puddle to appear: next to nothing */

static void new_puddle(TnpDay *d, float x, float y) {
    for (int i = 0; i < TNP_MAX_PUDDLES; i++) {
        TnpPuddle *p = &d->pud[i];
        if (p->alive) continue;
        p->alive = 1;
        p->x = x;
        p->y = y;
        p->t = 0;
        p->life = (int16_t)rng_range(&d->rng, 600, 1000);
        return;
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
        bool slick = p->t >= PUDDLE_IN && p->t < p->life - 30;
        if (slick && t->state == TS_DRIVE && t->air_t <= 0 && t->spin_t <= 0 &&
            tnp_dist(p->x, p->y, t->x, t->y) < 8 + 3 && tnp_forward(t) > 1.5f)
            tnp_spinout(t, (d->frame & 1) ? 1 : -1);
    }
    /* new puddles anywhere on the roads round the truck; one in three right on the road ahead */
    if (n < 28 && d->frame % 6 == 0) {
        bool ahead = rng_range(&d->rng, 0, 2) == 0 && tnp_speed(t) > 0.5f;
        for (int k = 0; k < 8; k++) {
            float x, y;
            if (ahead) {
                float r = (float)rng_range(&d->rng, 36, 70), sd = (float)rng_range(&d->rng, -10, 10);
                float ca = tnp_cos(t->ang), sa = tnp_sin(t->ang);
                x = tnp_wrap(t->x + ca * r - sa * sd);
                y = tnp_wrap(t->y + sa * r + ca * sd);
            } else {
                around(d, 50, 300, &x, &y);
                if (tnp_dist(x, y, t->x, t->y) < 50) continue;
            }
            if (!tnp_roadlike((int)(x / TNP_CELL), (int)(y / TNP_CELL))) continue;
            new_puddle(d, x, y);
            if (ahead) d->puddles_ahead++;
            break;
        }
    }
}

/* the mushmen come in hordes of 3 to 12, out of sight, with empty streets between */
static void mush_spawn(TnpDay *d) {
    int n = count_mobs(d, MK_MUSH);
    if (n > 36 - 12 || d->frame % 45) return;
    for (int k = 0; k < 10; k++) {
        float x, y;
        around(d, 160, 300, &x, &y);
        int cx = (int)(x / TNP_CELL), cy = (int)(y / TNP_CELL);
        if (!tnp_roadlike(cx, cy)) continue;
        int size = rng_range(&d->rng, 3, 12);
        for (int j = 0; j < size && n < 36; j++, n++)
            mob_new(d, MK_MUSH, tnp_cx(cx) + (float)rng_range(&d->rng, -16, 16), tnp_cx(cy) + (float)rng_range(&d->rng, -16, 16));
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
    if (d->event == EV_MUSH) mush_spawn(d);
    if (d->event == EV_RADISH) radish_update(d);
    if (d->event == EV_RAIN) rain_update(d);
    for (int i = 0; i < TNP_MAX_MOBS; i++) {
        TnpMob *m = &d->mob[i];
        if (!m->alive) continue;
        switch (m->kind) {
        case MK_BEET: beet_update(d, m); break;
        case MK_MUSH: mush_update(d, m); break;
        default: saucer_update(d, m); break;
        }
    }
    shots_update(d);
}
