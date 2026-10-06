/* TUSKWIND - the body's physics, and the three kinds of map: the journey
 * (new every run), the hall at its end (always the same) and the brawl's
 * single screen (new every round). */
#include "tuskwind.h"

TkwWorld tkw_w;

#define PI_F 3.14159265f

/* ---- the islets ---------------------------------------------------------- */

bool tkw_plat_solid(const TkwWorld *w, int i) { return i >= 0 && i < w->nplat && !w->plat[i].gone; }
float tkw_plat_top(const TkwWorld *w, int i) { return (float)w->plat[i].y; }

static int bucket_of(int x) { return iclamp(x / TKW_BUCKET_W, 0, TKW_BUCKETS - 1); }

void tkw_bucket_add(TkwWorld *w, int i) {
    const TkwPlat *p = &w->plat[i];
    for (int b = bucket_of(p->x); b <= bucket_of(p->x + p->w); b++)
        if (w->bucket_n[b] < TKW_BUCKET_N) w->bucket[b][w->bucket_n[b]++] = (int16_t)i;
}

void tkw_buckets_build(TkwWorld *w) {
    memset(w->bucket_n, 0, sizeof w->bucket_n);
    for (int i = 0; i < w->nplat; i++) tkw_bucket_add(w, i);
}

/* drop the newest islet again (the generator trying another spot) */
static void pop_plat(TkwWorld *w) {
    int i = --w->nplat;
    const TkwPlat *p = &w->plat[i];
    for (int b = bucket_of(p->x); b <= bucket_of(p->x + p->w); b++)
        if (w->bucket_n[b] && w->bucket[b][w->bucket_n[b] - 1] == i) w->bucket_n[b]--;
}

int tkw_add_plat(TkwWorld *w, int x, int y, int wd, int kind) {
    if (w->nplat >= TKW_MAX_PLATS) return -1;
    int i = w->nplat++;
    TkwPlat *p = &w->plat[i];
    memset(p, 0, sizeof *p);
    p->x = (int16_t)x;
    p->y = (int16_t)y;
    p->w = (int16_t)wd;
    p->h = (int16_t)(kind == PK_ROCK ? 14 : kind == PK_FOAM ? 7 : 6);
    p->kind = (uint8_t)kind;
    p->timer = -1;
    tkw_bucket_add(w, i);
    return i;
}

int tkw_add_thing(TkwWorld *w, int kind, float x, float y, int plat) {
    if (w->nth >= TKW_MAX_THINGS) return -1;
    int i = w->nth++;
    TkwThing *t = &w->th[i];
    memset(t, 0, sizeof *t);
    t->kind = (uint8_t)kind;
    t->alive = 1;
    t->x = x;
    t->y = y;
    t->plat = (int16_t)plat;
    t->dir = 1;
    return i;
}

int tkw_count_things(int kind) {
    int n = 0;
    for (int i = 0; i < tkw_w.nth; i++) n += tkw_w.th[i].alive && tkw_w.th[i].kind == kind;
    return n;
}

int tkw_progress_at(float x) {
    const TkwWorld *w = &tkw_w;
    if (w->goal_x <= w->start_x) return 0;
    return iclamp((int)((x - w->start_x) * 100.0f / (float)(w->goal_x - w->start_x)), 0, 100);
}

void tkw_manta_pos(const TkwManta *m, int t, float *x, float *y) {
    *x = m->cx + m->span * sinf(m->phase + t * m->speed);
    *y = m->y0 + 3.0f * sinf(t * 0.05f + m->phase * 3.0f);
}

/* ---- the body ------------------------------------------------------------ */

float tkw_launch_speed(int charge) {
    return TKW_VMIN + (TKW_VMAX - TKW_VMIN) * (float)iclamp(charge, 0, TKW_CHARGE_T) / TKW_CHARGE_T;
}

void tkw_body_launch(TkwBody *b, const TkwWorld *w, int face, float aim, int charge) {
    float v = tkw_launch_speed(charge), a = aim * PI_F / 180.0f;
    bool rock = b->plat >= 0 && w->plat[b->plat].kind == PK_ROCK;
    b->ignore = -1;
    b->landed = -1;
    if (fabsf(aim) < 0.01f || (aim < 0 && rock)) {
        /* a flat jump: a lunge along the ground, no air at all */
        b->mode = BM_LUNGE;
        b->vx = face * cosf(a) * v;
        b->vy = 0;
        return;
    }
    b->vx = cosf(a) * v * face;
    b->vy = -sinf(a) * v;
    if (b->vy > 0) b->ignore = b->plat; /* aimed down: off through the islet */
    b->mode = BM_AIR;
    b->plat = -1;
    b->ride = -1;
}

void tkw_body_ground(TkwBody *b, const TkwWorld *w, int dir) {
    b->landed = -1;
    b->bumped = false;
    if (!tkw_plat_solid(w, b->plat)) {
        b->mode = BM_AIR;
        b->plat = -1;
        b->vy = 0;
        return;
    }
    const TkwPlat *p = &w->plat[b->plat];
    if (b->mode == BM_LUNGE) {
        b->x += b->vx;
        b->vx = fapproach(b->vx, 0, TKW_LUNGE_DECEL);
        if (b->x < p->x - 1 || b->x > p->x + p->w + 1) {
            /* slid off the end */
            b->mode = BM_AIR;
            b->plat = -1;
            b->vy = 0;
            return;
        }
        if (b->vx == 0) b->mode = BM_GROUND;
        b->y = (float)p->y;
        return;
    }
    if (dir) b->x = fclamp(b->x + dir * TKW_WALK, p->x + 2.0f, p->x + p->w - 2.0f);
    b->y = (float)p->y;
}

static bool box_hits(const TkwBody *b, const TkwPlat *p) {
    return b->x + TKW_HW > p->x && b->x - TKW_HW < p->x + p->w && b->y > p->y && b->y - TKW_BH < p->y + p->h;
}

/* the islets near x (each once) */
static int near_plats(const TkwWorld *w, float x, int *out, int max) {
    int n = 0;
    int b0 = bucket_of((int)x - TKW_HW - 2), b1 = bucket_of((int)x + TKW_HW + 2);
    for (int b = b0; b <= b1; b++)
        for (int k = 0; k < w->bucket_n[b]; k++) {
            int i = w->bucket[b][k];
            bool seen = false;
            for (int j = 0; j < n; j++) seen |= out[j] == i;
            if (!seen && n < max) out[n++] = i;
        }
    return n;
}

void tkw_body_air(TkwBody *b, const TkwWorld *w, float fdx, float fdy, bool flap, float wind) {
    b->landed = -1;
    b->bumped = false;
    b->vy += b->kite ? TKW_G_KITE : TKW_G;
    b->vx += wind * TKW_WIND;
    if (flap && b->stamina > 0) {
        /* holding the flap always lifts; LEFT/RIGHT trim the speed across
         * (UP and DOWN do nothing to it) */
        (void)fdy;
        float ax = (float)isign((int)fdx) * TKW_FLAP_SIDE, ay = -TKW_FLAP;
        if (b->vy + ay < -TKW_FLAP_RISE) ay = fminf(0, -TKW_FLAP_RISE - b->vy);
        if (ax > 0 && b->vx + ax > TKW_FLAP_MAXVX) ax = fmaxf(0, TKW_FLAP_MAXVX - b->vx);
        if (ax < 0 && b->vx + ax < -TKW_FLAP_MAXVX) ax = fminf(0, -TKW_FLAP_MAXVX - b->vx);
        b->vx += ax;
        b->vy += ay;
        b->stamina = imax(0, b->stamina - TKW_FLAP_COST(b->kite));
    }
    b->vy = fminf(b->vy, TKW_FALL_MAX);
    b->vx = fclamp(b->vx, -6.0f, 6.0f);

    int near[24], n;
    /* across */
    b->x += b->vx;
    n = near_plats(w, b->x, near, 24);
    for (int k = 0; k < n; k++) {
        const TkwPlat *p = &w->plat[near[k]];
        if (p->gone || p->kind != PK_ROCK || !box_hits(b, p)) continue;
        /* a rock: bounced off its side, the speed kept */
        if (b->vx > 0) b->x = p->x - TKW_HW - 0.01f;
        else b->x = p->x + p->w + TKW_HW + 0.01f;
        b->vx = -b->vx * 0.8f;
        b->bumped = true;
    }
    /* down (or up) */
    float oy = b->y;
    b->y += b->vy;
    n = near_plats(w, b->x, near, 24);
    int land = -1;
    for (int k = 0; k < n; k++) {
        int i = near[k];
        const TkwPlat *p = &w->plat[i];
        if (p->gone) continue;
        if (p->kind == PK_ROCK) {
            if (!box_hits(b, p)) continue;
            if (b->vy >= 0 && oy <= p->y + 0.5f) {
                if (land < 0 || p->y < w->plat[land].y) land = i;
            } else if (b->vy < 0) {
                b->y = p->y + p->h + TKW_BH + 0.01f;
                b->vy = -b->vy * 0.8f;
                b->bumped = true;
            } else {
                b->y = (float)p->y;
                if (land < 0) land = i;
            }
            continue;
        }
        /* a ledge or foam: only from above */
        if (i == b->ignore || b->vy < 0) continue;
        if (oy <= p->y && b->y >= p->y && b->x >= p->x - 1 && b->x <= p->x + p->w + 1)
            if (land < 0 || p->y < w->plat[land].y) land = i;
    }
    if (land >= 0) {
        b->y = (float)w->plat[land].y;
        b->vy = 0;
        b->plat = land;
        b->landed = land;
        b->kite = false;
        b->ignore = -1;
        /* stops on a dime, unless a tailwind is carrying him */
        if (wind != 0 && b->vx * wind > 1.5f) {
            b->mode = BM_LUNGE;
            b->vx *= 0.6f;
        } else {
            b->mode = BM_GROUND;
            b->vx = 0;
        }
    }
    if (w->wrap) {
        if (b->x < 0) b->x += w->w;
        if (b->x >= w->w) b->x -= w->w;
    } else {
        if (b->x < TKW_HW) { b->x = TKW_HW; b->vx = fmaxf(b->vx, 0); }
        if (b->x > w->w - TKW_HW) { b->x = (float)(w->w - TKW_HW); b->vx = fminf(b->vx, 0); }
    }
    if (b->y - TKW_BH < 0) { b->y = TKW_BH; b->vy = fmaxf(b->vy, 0); }
}

/* ---- reachability -------------------------------------------------------- */

/* Simulates a calm, flapless jump from x on islet a; returns the islet it
 * lands on (-1: the sea, or too high: into the mantas' band). */
static int sim_jump(const TkwWorld *w, int a, float x, int face, float aim, int charge, float *land_x) {
    TkwBody s;
    memset(&s, 0, sizeof s);
    s.x = x;
    s.y = (float)w->plat[a].y;
    s.plat = a;
    s.ride = -1;
    s.mode = BM_GROUND;
    tkw_body_launch(&s, w, face, aim, charge);
    for (int f = 0; f < 400; f++) {
        if (s.mode == BM_LUNGE) {
            tkw_body_ground(&s, w, 0);
            if (s.mode == BM_GROUND) { *land_x = s.x; return s.plat; }
            continue;
        }
        if (s.mode == BM_GROUND) { *land_x = s.x; return s.plat; }
        tkw_body_air(&s, w, 0, 0, false, 0);
        if (s.y - TKW_BH < TKW_MANTA_Y + 16) return -1;
        if (s.y > w->sea_y) return -1;
        if (s.landed >= 0 && s.mode == BM_GROUND) { *land_x = s.x; return s.plat; }
    }
    return -1;
}

bool tkw_can_reach(const TkwWorld *w, int a, int b) {
    const TkwPlat *pa = &w->plat[a], *pb = &w->plat[b];
    float stands[3] = {pa->x + 4.0f, pa->x + pa->w / 2.0f, pa->x + pa->w - 4.0f};
    int good = 0;
    for (int s = 0; s < 3; s++) {
        int face = pb->x + pb->w / 2 >= stands[s] ? 1 : -1;
        for (int ai = 0; ai <= 14; ai++) {
            float aim = (float)(ai * 6);
            for (int c = 8; c <= TKW_CHARGE_T; c += 4) {
                float lx = 0;
                if (sim_jump(w, a, stands[s], face, aim, c, &lx) == b && lx >= pb->x + 3 && lx <= pb->x + pb->w - 3)
                    if (++good >= 2) return true;
            }
        }
    }
    return false;
}

/* ---- the journey --------------------------------------------------------- */

static bool overlaps_any(const TkwWorld *w, int x, int y, int wd, int h, int margin) {
    for (int i = 0; i < w->nplat; i++) {
        const TkwPlat *p = &w->plat[i];
        if (rects_overlap(x - margin, y - margin - 24, wd + 2 * margin, h + 2 * margin + 24, p->x, p->y - 24, p->w, p->h + 24))
            return true;
    }
    return false;
}

static int route_list[TKW_MAX_PLATS];

/* the route islet nearest to a progress figure, preferring a free one */
static int route_at(int pct, const uint8_t *used, int min_w) {
    int best = -1, bd = 1 << 30;
    for (int k = 1; k < tkw_w.nroute - 1; k++) {
        int i = route_list[k];
        const TkwPlat *p = &tkw_w.plat[i];
        if (used && used[i]) continue;
        if (p->kind == PK_FOAM || p->w < min_w) continue;
        int d = iabs(tkw_progress_at(p->x + p->w / 2.0f) - pct) * 1000 + iabs(p->w - 64);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

/* widen a route islet to at least wd, keeping clear of its neighbours */
static void widen(int i, int wd) {
    TkwPlat *p = &tkw_w.plat[i];
    if (p->w >= wd) return;
    int need = wd - p->w;
    int right_room = TKW_MAP_W, left_room = TKW_MAP_W;
    for (int j = 0; j < tkw_w.nplat; j++) {
        if (j == i) continue;
        const TkwPlat *q = &tkw_w.plat[j];
        if (q->y > p->y + 40 || q->y + q->h < p->y - 40) continue;
        if (q->x >= p->x + p->w) right_room = imin(right_room, q->x - (p->x + p->w) - 10);
        if (q->x + q->w <= p->x) left_room = imin(left_room, p->x - (q->x + q->w) - 10);
    }
    int r = iclamp(need, 0, imax(0, right_room));
    p->w = (int16_t)(p->w + r);
    need -= r;
    int l = iclamp(need, 0, imax(0, left_room));
    p->x = (int16_t)(p->x - l);
    p->w = (int16_t)(p->w + l);
    tkw_buckets_build(&tkw_w);
}

static bool thing_clear(float x, float y, float r) {
    for (int i = 0; i < tkw_w.nth; i++) {
        const TkwThing *t = &tkw_w.th[i];
        if (fabsf(t->x - x) < r && fabsf(t->y - y) < r) return false;
    }
    /* not inside an islet either */
    for (int i = 0; i < tkw_w.nplat; i++) {
        const TkwPlat *p = &tkw_w.plat[i];
        if (x > p->x - 6 && x < p->x + p->w + 6 && y > p->y - 6 && y < p->y + p->h + 6) return false;
    }
    return true;
}

void tkw_gen_journey(uint64_t seed) {
    TkwWorld *w = &tkw_w;
    Rng r;
    rng_seed(&r, seed);
    memset(w, 0, sizeof *w);
    w->w = TKW_MAP_W;
    w->h = TKW_MAP_H;
    w->sea_y = TKW_SEA_Y;
    w->start_plat = tkw_add_plat(w, 24, 200, 88, PK_LEDGE);
    w->plat[w->start_plat].route = 1;
    w->start_x = 24 + 44;
    int goal_left = TKW_MAP_W - 150;
    w->goal_x = goal_left + 60;
    int nr = 0;
    route_list[nr++] = w->start_plat;
    int prev = w->start_plat;

    while (w->plat[prev].x + w->plat[prev].w < goal_left - 150 && w->nplat < TKW_MAX_PLATS - 24) {
        const TkwPlat *pp = &w->plat[prev];
        int x = pp->x + pp->w;
        int pct = tkw_progress_at((float)x);
        int got = -1;
        for (int tr = 0; tr < 24 && got < 0; tr++) {
            int gap = imax(12, rng_range(&r, 24, 100) - tr * 3);
            int dy = rng_range(&r, -48, 56) * (24 - tr) / 24;
            int y = iclamp(pp->y + dy, 88, 292);
            int roll = rng_range(&r, 0, 99), kind = PK_LEDGE;
            if (pct > 3 && roll < 13) kind = PK_ROCK;
            else if (pct > 6 && roll < 25) kind = PK_FOAM;
            int wd = kind == PK_ROCK ? rng_range(&r, 24, 40) : kind == PK_FOAM ? rng_range(&r, 28, 40) : rng_range(&r, 28, 64);
            int i = tkw_add_plat(w, x + gap, y, wd, kind);
            if (tkw_can_reach(w, prev, i)) got = i;
            else pop_plat(w);
        }
        if (got < 0) got = tkw_add_plat(w, x + 16, pp->y, 44, PK_LEDGE); /* always within reach */
        w->plat[got].route = 1;
        route_list[nr++] = got;
        /* a second islet in the same stretch, above or below */
        const TkwPlat *g = &w->plat[got];
        if (rng_chance(&r, 62)) {
            int y2 = g->y < 190 ? g->y + rng_range(&r, 56, 104) : g->y - rng_range(&r, 56, 104);
            y2 = iclamp(y2, 80, 304);
            int roll = rng_range(&r, 0, 99), kind = roll < 18 ? PK_ROCK : roll < 30 ? PK_FOAM : PK_LEDGE;
            int w2 = kind == PK_ROCK ? rng_range(&r, 24, 40) : rng_range(&r, 28, 60);
            int x2 = g->x + rng_range(&r, -30, 30);
            if (iabs(y2 - g->y) >= 50 && !overlaps_any(w, x2, y2, w2, 14, 10)) {
                int a = tkw_add_plat(w, x2, y2, w2, kind);
                if (!tkw_can_reach(w, prev, got)) pop_plat(w);
                else (void)a;
            }
        }
        prev = got;
    }
    /* the last islet, with the door: always within reach of the one before */
    int gy = iclamp(w->plat[prev].y, 120, 240), goal = -1;
    for (int tr = 0; tr < 12 && goal < 0; tr++) {
        int i = tkw_add_plat(w, goal_left, gy, 120, PK_LEDGE);
        if (tkw_can_reach(w, prev, i)) goal = i;
        else {
            pop_plat(w);
            /* a stepping stone, then try again */
            const TkwPlat *pp = &w->plat[prev];
            int s = tkw_add_plat(w, pp->x + pp->w + 30, (pp->y + gy) / 2, 40, PK_LEDGE);
            w->plat[s].route = 1;
            route_list[nr++] = s;
            prev = s;
        }
    }
    if (goal < 0) {
        goal = tkw_add_plat(w, goal_left, w->plat[prev].y, 120, PK_LEDGE);
        TkwPlat *pp = &w->plat[prev];
        pp->w = (int16_t)(goal_left - 20 - pp->x); /* bridge it */
        tkw_buckets_build(w);
    }
    w->plat[goal].route = 1;
    w->plat[goal].door = 1;
    route_list[nr++] = goal;
    w->goal_plat = goal;
    w->goal_x = w->plat[goal].x + 60;
    w->nroute = nr;

    /* ---- what stands on the islets ---- */
    uint8_t used[TKW_MAX_PLATS];
    memset(used, 0, sizeof used);
    used[w->start_plat] = used[goal] = 1;
    /* six lighthouses, each with its tern */
    for (int k = 0; k < TKW_TERNS; k++) {
        int i = route_at(10 + k * 14 + rng_range(&r, -2, 3), used, 28);
        if (i < 0) continue;
        widen(i, 40);
        used[i] = 1;
        tkw_add_thing(w, TH_LIGHTHOUSE, w->plat[i].x + w->plat[i].w / 2.0f, (float)w->plat[i].y, i);
    }
    /* three of the duchess's stalls, two wares each */
    static const int SHOP_PCT[3] = {22, 48, 72};
    for (int k = 0; k < 3; k++) {
        int i = route_at(SHOP_PCT[k] + rng_range(&r, -3, 3), used, 28);
        if (i < 0) continue;
        widen(i, 48);
        used[i] = 1;
        int s = tkw_add_thing(w, TH_SHOP, w->plat[i].x + w->plat[i].w - 14.0f, (float)w->plat[i].y, i);
        int a = rng_range(&r, 0, IT_COUNT - 1), b = rng_range(&r, 0, IT_COUNT - 2);
        if (b >= a) b++;
        if (s >= 0) w->th[s].arg = a | b << 4;
    }
    /* 22 signs in order: the first right beside him where he wakes up in the
     * dream, the other 21 evenly along the way */
    uint8_t signed_[TKW_MAX_PLATS];
    memset(signed_, 0, sizeof signed_);
    signed_[w->start_plat] = signed_[goal] = 1;
    tkw_add_thing(w, TH_SIGN, w->start_x - 18.0f, (float)w->plat[w->start_plat].y, w->start_plat);
    for (int k = 1; k < TKW_SIGNS; k++) {
        int i = route_at(4 + (k - 1) * 91 / 20, signed_, 20);
        if (i < 0) continue;
        signed_[i] = 1;
        int s = tkw_add_thing(w, TH_SIGN, w->plat[i].x + 5.0f, (float)w->plat[i].y, i);
        if (s >= 0) w->th[s].arg = k;
    }
    /* signs are numbered in the order they stand */
    {
        int ids[TKW_SIGNS], n = 0;
        for (int t = 0; t < w->nth && n < TKW_SIGNS; t++)
            if (w->th[t].kind == TH_SIGN) ids[n++] = t;
        for (int a = 1; a < n; a++) /* by position, left to right */
            for (int b = a; b > 0 && w->th[ids[b]].x < w->th[ids[b - 1]].x; b--) {
                int tmp = ids[b];
                ids[b] = ids[b - 1];
                ids[b - 1] = tmp;
            }
        for (int k = 0; k < n; k++) w->th[ids[k]].arg = k;
    }
    /* the side islets (off the route), for chests, bells and rams */
    int side[TKW_MAX_PLATS], ns = 0;
    for (int i = 0; i < w->nplat; i++)
        if (!w->plat[i].route && w->plat[i].kind != PK_FOAM) side[ns++] = i;
    /* two chests, off the beaten track */
    for (int k = 0; k < 2; k++) {
        int lo = k == 0 ? 22 : 52, hi = k == 0 ? 50 : 88, pick = -1;
        for (int tr = 0; tr < 60 && pick < 0; tr++) {
            int i = ns ? side[rng_range(&r, 0, ns - 1)] : -1;
            if (i < 0 || used[i] || w->plat[i].w < 24) continue;
            int pc = tkw_progress_at((float)w->plat[i].x);
            if (pc >= lo && pc <= hi) pick = i;
        }
        if (pick < 0) pick = route_at((lo + hi) / 2, used, 24);
        if (pick < 0) continue;
        used[pick] = 1;
        tkw_add_thing(w, TH_CHEST, w->plat[pick].x + w->plat[pick].w / 2.0f, (float)w->plat[pick].y, pick);
    }
    /* two keys, floating high */
    for (int k = 0; k < 2; k++) {
        for (int tr = 0; tr < 40; tr++) {
            int i = route_at(rng_range(&r, k == 0 ? 8 : 40, k == 0 ? 38 : 76), NULL, 0);
            if (i < 0) continue;
            float kx = w->plat[i].x + w->plat[i].w / 2.0f, ky = fmaxf(76.0f, w->plat[i].y - rng_range(&r, 44, 64.0f));
            if (!thing_clear(kx, ky, 12)) continue;
            tkw_add_thing(w, TH_KEY, kx, ky, -1);
            break;
        }
    }
    /* five wind bells */
    static const int BELL_LO[5] = {12, 30, 50, 68, 84}, BELL_HI[5] = {26, 46, 66, 82, 96};
    for (int k = 0; k < 5; k++) {
        int pick = -1;
        for (int tr = 0; tr < 60 && pick < 0; tr++) {
            int i = ns && tr < 40 ? side[rng_range(&r, 0, ns - 1)] : route_at(rng_range(&r, BELL_LO[k], BELL_HI[k]), used, 20);
            if (i < 0 || used[i]) continue;
            int pc = tkw_progress_at((float)w->plat[i].x);
            if (pc >= BELL_LO[k] && pc <= BELL_HI[k]) pick = i;
        }
        if (pick < 0) continue;
        used[pick] = 1;
        tkw_add_thing(w, TH_BELL, w->plat[pick].x + w->plat[pick].w / 2.0f, w->plat[pick].y - 1.0f, pick);
    }
    /* ten rams: mostly on the side islets, a few on the way itself */
    int rams = 0;
    for (int tr = 0; tr < 400 && rams < 10; tr++) {
        bool on_route = rams >= 6;
        int i = on_route ? route_list[rng_range(&r, 3, nr - 3)] : (ns ? side[rng_range(&r, 0, ns - 1)] : -1);
        if (i < 0 || used[i] == 2 || w->plat[i].kind == PK_FOAM) continue;
        if (w->plat[i].w < (on_route ? 52 : 40)) continue;
        int pc = tkw_progress_at((float)w->plat[i].x);
        if (pc < 12 || pc > 95) continue;
        bool busy = false;
        for (int t = 0; t < w->nth; t++)
            if (w->th[t].plat == i && (w->th[t].kind == TH_LIGHTHOUSE || w->th[t].kind == TH_SHOP || w->th[t].kind == TH_RAM)) busy = true;
        if (busy) continue;
        used[i] = 2;
        int t = tkw_add_thing(w, TH_RAM, w->plat[i].x + w->plat[i].w * 0.5f + rng_range(&r, -8, 8), (float)w->plat[i].y, i);
        if (t >= 0) w->th[t].dir = rng_chance(&r, 50) ? 1 : -1;
        rams++;
    }
    /* sprats: 40, along the hops */
    for (int tr = 0, n = 0; tr < 2000 && n < 40; tr++) {
        int k = rng_range(&r, 0, nr - 2);
        const TkwPlat *a = &w->plat[route_list[k]], *b = &w->plat[route_list[k + 1]];
        float fx = (a->x + a->w + b->x) / 2.0f + rng_range(&r, -8, 8);
        float fy = fminf((float)a->y, (float)b->y) - rng_range(&r, 18, 48);
        if (fy < 80 || !thing_clear(fx, fy, 14)) continue;
        tkw_add_thing(w, TH_SPRAT, fx, fy, -1);
        n++;
    }
    /* 36 shells: cockles in little trails, three or four whelks in harder spots */
    int whelks = 3 + rng_range(&r, 0, 1), shells = 0;
    for (int tr = 0; tr < 3000 && shells < TKW_SHELLS_FLOAT; tr++) {
        bool whelk = shells < whelks;
        float sx, sy;
        if (whelk) {
            int i = rng_range(&r, 0, w->nplat - 1);
            const TkwPlat *p = &w->plat[i];
            if (i == w->start_plat || i == goal) continue;
            sx = p->x + p->w / 2.0f;
            sy = rng_chance(&r, 50) ? fmaxf(74.0f, p->y - rng_range(&r, 60, 90.0f)) : fminf(326.0f, p->y + rng_range(&r, 40, 70.0f));
            if (!thing_clear(sx, sy, 14)) continue;
            tkw_add_thing(w, TH_WHELK, sx, sy, -1);
            shells++;
            continue;
        }
        int k = rng_range(&r, 0, nr - 2);
        const TkwPlat *a = &w->plat[route_list[k]], *b = &w->plat[route_list[k + 1]];
        int len = imin(rng_range(&r, 1, 3), TKW_SHELLS_FLOAT - shells);
        float bx = a->x + a->w + 2.0f, by = fminf((float)a->y, (float)b->y) - rng_range(&r, 14, 40);
        float span = fmaxf(8.0f, (float)(b->x - (a->x + a->w)));
        bool ok = true;
        for (int j = 0; j < len && ok; j++) ok = thing_clear(bx + span * (j + 1) / (len + 1), by - (j == 1 ? 6 : 0), 10) && by > 76;
        if (!ok) continue;
        for (int j = 0; j < len; j++) tkw_add_thing(w, TH_COCKLE, bx + span * (j + 1) / (len + 1), by - (j == 1 ? 6 : 0), -1);
        shells += len;
    }
    /* the mantas, gliding high; the first one passes the top-left corner */
    w->nmanta = 8;
    for (int k = 0; k < w->nmanta; k++) {
        TkwManta *m = &w->manta[k];
        if (k == 0) {
            m->cx = 200;
            m->span = 176;
            m->y0 = TKW_MANTA_Y;
            m->speed = 0.007f;
            m->phase = 0;
        } else {
            m->cx = 700.0f + (k - 1) * 1050 + rng_range(&r, -150, 150);
            m->span = (float)rng_range(&r, 100, 260);
            m->y0 = (float)rng_range(&r, TKW_MANTA_Y - 10, TKW_MANTA_Y + 10);
            m->speed = rng_range(&r, 8, 14) / 1000.0f;
            m->phase = rng_float(&r) * 6.28f;
        }
        m->alive = 1;
        tkw_manta_pos(m, 0, &m->x, &m->y);
        m->px = m->x;
    }
    /* and in the top-left corner, behind the bar, a sign nobody put there */
    tkw_add_thing(w, TH_SECRET, 14, 12, -1);
}

/* ---- the hall at the end -------------------------------------------------- */

void tkw_gen_hall(int terns) {
    TkwWorld *w = &tkw_w;
    memset(w, 0, sizeof *w);
    w->w = TKW_HALL_W;
    w->h = TKW_MAP_H;
    w->sea_y = TKW_SEA_Y;
    /* the floor, east of the chasm */
    int floor = tkw_add_plat(w, TKW_HALL_EDGE, 296, TKW_HALL_W - TKW_HALL_EDGE, PK_ROCK);
    w->plat[floor].h = 48;
    /* the vault, with a shaft up through it east of the door */
    int c1 = tkw_add_plat(w, TKW_HALL_EDGE, 0, TKW_HALL_SHAFT - 16 - TKW_HALL_EDGE, PK_ROCK);
    w->plat[c1].h = 112;
    int c2 = tkw_add_plat(w, TKW_HALL_SHAFT + 16, 0, TKW_HALL_W - TKW_HALL_SHAFT - 16, PK_ROCK);
    w->plat[c2].h = 112;
    /* the hidden shelf over the chasm, far west */
    int shelf = tkw_add_plat(w, 6, 148, 34, PK_LEDGE);
    w->start_plat = floor;
    w->goal_plat = floor;
    w->start_x = 0;
    w->goal_x = 0;
    /* the terns that came along perch at the west end of the floor */
    for (int k = 0; k < terns; k++) tkw_add_thing(w, TH_PERCH, TKW_HALL_EDGE + 10.0f + k * 9, 296, floor);
    /* sixteen cockles up the shaft */
    for (int k = 0; k < 16; k++) tkw_add_thing(w, TH_COCKLE, TKW_HALL_SHAFT, 200.0f - k * 11, -1);
    tkw_add_thing(w, TH_CHEST, 24, 148, shelf);
    tkw_add_thing(w, TH_ELDER, TKW_HALL_ELDER, 296, floor);
}

/* ---- the brawl's screen ----------------------------------------------------- */

void tkw_gen_brawl(uint64_t seed) {
    TkwWorld *w = &tkw_w;
    Rng r;
    rng_seed(&r, seed);
    memset(w, 0, sizeof *w);
    w->w = TKW_BRAWL_W;
    w->h = TKW_BRAWL_H;
    w->sea_y = TKW_BRAWL_SEA;
    w->wrap = true;
    /* the two home islets, then six more wherever they fit */
    int y0 = rng_range(&r, 104, 132);
    tkw_add_plat(w, 28, y0, 60, PK_LEDGE);
    tkw_add_plat(w, TKW_BRAWL_W - 88, y0, 60, PK_LEDGE);
    for (int tr = 0, n = 0; tr < 400 && n < 6; tr++) {
        int kind = rng_range(&r, 0, 9) < 2 ? PK_ROCK : PK_LEDGE;
        int wd = kind == PK_ROCK ? rng_range(&r, 24, 36) : rng_range(&r, 30, 64);
        int x = rng_range(&r, 8, TKW_BRAWL_W - 8 - wd), y = rng_range(&r, 48, 150);
        if (overlaps_any(w, x, y, wd, 14, 10)) continue;
        tkw_add_plat(w, x, y, wd, kind);
        n++;
    }
    w->start_plat = 0;
    w->goal_plat = 1;
}
