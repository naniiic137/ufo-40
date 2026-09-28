/* LOST LINKS - planning: the flow of the ground towards a goal, and strokes
 * tried out in advance with the real rolling rules. The demo player (the
 * "bot" test hook) uses these to play the whole game with real button
 * presses; the reachability tests use the flow to check what each ability
 * opens. */
#include "lostlinks.h"

#define NODES (LNK_LAYERS * LNK_MH * LNK_MW)
#define CHIP_TILES 5
#define CHIP_TILES_DIAG 4

static int node(int l, int x, int y) { return (l * LNK_MH + y) * LNK_MW + x; }

static bool has(const LnkCtx *c, int ab) { return (c->abilities >> ab) & 1; }

static char T(const LnkCtx *c, int l, int x, int y) { return lnk_tile(c, l, x, y); }

static bool is_hole(char ch) { return ch == 'o' || ch == 'O'; }

/* the ball can be on this tile (rolling or airborne) */
static bool passable(const LnkCtx *c, char ch) {
    if (ch == '#' || ch == 'T' || ch == 'H') return false;
    if (ch == 'g') return (c->opened & 1) != 0;
    if (ch == 'k') return (c->opened & 2) != 0;
    if (ch == 'X') return has(c, AB_HAMMER);
    if (ch == '~') return has(c, AB_SKIPPER);
    if (ch == 'r') return has(c, AB_TREAD);
    return true;
}

/* a chip can land here and stay */
static bool standable(const LnkCtx *c, char ch) {
    if (ch == 'r' || ch == '~') return false; /* landing in water sinks, Skipper or not */
    return passable(c, ch);
}

/* does the flight of a chip clear this tile? */
static bool flies_over(const LnkCtx *c, char ch) {
    if (ch == '#' || ch == 'T' || ch == 'H') return false;
    if (ch == 'g') return (c->opened & 1) != 0;
    if (ch == 'k') return (c->opened & 2) != 0;
    if (ch == 'X') return has(c, AB_HAMMER);
    return true;
}

static bool chip_from(const LnkCtx *c, char ch) {
    return is_hole(ch) || ch == 'u' || (ch == 's' && !has(c, AB_TREAD));
}

/* how many slope tiles in a row a ball going (dx, dy) would have to climb */
static int uphill_run(const LnkCtx *c, int l, int x, int y, int dx, int dy) {
    int n = 0;
    for (int k = 0; k < 12; k++) {
        char ch = T(c, l, x + dx * k, y + dy * k);
        float ax, ay;
        lnk_slope(ch, &ax, &ay);
        if (ax * dx + ay * dy >= 0 || (ax == 0 && ay == 0)) break;
        n++;
    }
    return n;
}

/* the cost of going from (l, x, y) to (m, u, v), or -1 */
static int edge_cost(const LnkCtx *c, int l, int x, int y, int m, int u, int v) {
    char a = T(c, l, x, y), b = T(c, m, u, v);
    if (!passable(c, a) || !passable(c, b)) return -1;
    if (l != m) return (x == u && y == v && is_hole(a) && is_hole(b)) ? 1 : -1;
    int dx = u - x, dy = v - y;
    if (iabs(dx) + iabs(dy) == 1) {
        if (b == 'v' && dy < 0 && a != 'v') return -1; /* a ledge */
        if (uphill_run(c, m, u, v, dx, dy) >= 7) return -1; /* too long a climb */
        if (a == 's' && !has(c, AB_TREAD)) return 4;
        if (a == 'r' || b == 'r') return 3; /* a hop */
        return b == ',' ? 2 : 1;
    }
    /* a chip: in a straight line (eight ways), clearing water and rails */
    if (!chip_from(c, a) || !standable(c, b)) return -1;
    int sx = isign(dx), sy = isign(dy), k = imax(iabs(dx), iabs(dy));
    if ((dx && dy && iabs(dx) != iabs(dy)) || k < 2) return -1;
    if (k > ((sx && sy) ? CHIP_TILES_DIAG : CHIP_TILES)) return -1;
    for (int i = 1; i < k; i++)
        if (!flies_over(c, T(c, l, x + sx * i, y + sy * i))) return -1;
    return 5;
}

/* ---- Dijkstra over the tiles ------------------------------------------ */

static int heap[NODES], hpos[NODES], hn;

static void hswap(int i, int j) {
    int t = heap[i];
    heap[i] = heap[j];
    heap[j] = t;
    hpos[heap[i]] = i;
    hpos[heap[j]] = j;
}

static uint16_t *DIST;

static void hup(int i) {
    while (i > 0) {
        int p = (i - 1) / 2;
        if (DIST[heap[p]] <= DIST[heap[i]]) break;
        hswap(i, p);
        i = p;
    }
}

static void hdown(int i) {
    for (;;) {
        int l = i * 2 + 1, r = l + 1, m = i;
        if (l < hn && DIST[heap[l]] < DIST[heap[m]]) m = l;
        if (r < hn && DIST[heap[r]] < DIST[heap[m]]) m = r;
        if (m == i) break;
        hswap(i, m);
        i = m;
    }
}

static void hpush_or_lower(int n) {
    if (hpos[n] < 0) {
        heap[hn] = n;
        hpos[n] = hn++;
    }
    hup(hpos[n]);
}

static int hpop(void) {
    int n = heap[0];
    hswap(0, --hn);
    hpos[n] = -2; /* done */
    if (hn) hdown(0);
    return n;
}

static void flow(LnkFlow *f, const LnkCtx *c, int layer, int tx, int ty, bool reverse) {
    DIST = &f->d[0][0][0];
    for (int i = 0; i < NODES; i++) { DIST[i] = LNK_FAR; hpos[i] = -1; }
    hn = 0;
    int s = node(layer, tx, ty);
    DIST[s] = 0;
    hpush_or_lower(s);
    while (hn) {
        int n = hpop();
        int l = n / (LNK_MH * LNK_MW), rem = n % (LNK_MH * LNK_MW), y = rem / LNK_MW, x = rem % LNK_MW;
        /* the other tiles an edge can join this one with */
        int cand[64][3], nc = 0;
        static const int D4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        static const int D8[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
        for (int i = 0; i < 4; i++) { cand[nc][0] = l; cand[nc][1] = x + D4[i][0]; cand[nc][2] = y + D4[i][1]; nc++; }
        cand[nc][0] = 1 - l; cand[nc][1] = x; cand[nc][2] = y; nc++;
        for (int i = 0; i < 8; i++)
            for (int k = 2; k <= CHIP_TILES; k++) {
                cand[nc][0] = l; cand[nc][1] = x + D8[i][0] * k; cand[nc][2] = y + D8[i][1] * k; nc++;
            }
        for (int i = 0; i < nc; i++) {
            int m = cand[i][0], u = cand[i][1], v = cand[i][2];
            if (u < 0 || v < 0 || u >= LNK_MW || v >= LNK_MH) continue;
            int w = reverse ? edge_cost(c, m, u, v, l, x, y) : edge_cost(c, l, x, y, m, u, v);
            if (w < 0) continue;
            int o = node(m, u, v);
            if (hpos[o] == -2) continue;
            int nd = DIST[n] + w;
            if (nd < DIST[o]) {
                DIST[o] = (uint16_t)nd;
                hpush_or_lower(o);
            }
        }
    }
}

void lnk_flow_to(LnkFlow *f, const LnkCtx *c, int layer, int tx, int ty) { flow(f, c, layer, tx, ty, true); }
void lnk_flow_from(LnkFlow *f, const LnkCtx *c, int layer, int tx, int ty) { flow(f, c, layer, tx, ty, false); }

/* ---- strokes tried out in advance ------------------------------------- */

int lnk_sim(LnkBall *b, const LnkCtx *c, int dir, int level, int hop_at, int brake_at, int layer, float px, float py,
            float rad, bool *touched) {
    *touched = false;
    lnk_hit(b, c, dir, level);
    for (int f = 0; f < 2400; f++) {
        if (f == hop_at) lnk_hop(b, c);
        if (f == brake_at) lnk_brake(b, c);
        int ev = lnk_ball_step(b, c);
        if (b->layer == layer) {
            float dx = b->x - px, dy = b->y - py;
            if (dx * dx + dy * dy < rad * rad) { *touched = true; return EV_REST; }
        }
        if (ev == EV_HOLE) {
            b->layer ^= 1; /* drops (or climbs) to the other layer, in the cup */
            if (b->layer == layer) {
                float dx = b->x - px, dy = b->y - py;
                if (dx * dx + dy * dy < rad * rad) *touched = true;
            }
            return EV_HOLE;
        }
        if (ev == EV_SINK || ev == EV_REST) return ev;
    }
    return EV_REST;
}

static int flow_at(const LnkFlow *f, const LnkBall *b) {
    int x = (int)floorf(b->x / LNK_T), y = (int)floorf(b->y / LNK_T);
    if (x < 0 || y < 0 || x >= LNK_MW || y >= LNK_MH) return LNK_FAR;
    return f->d[b->layer][y][x];
}

typedef struct Cand { int dir, level, hop, brake, score; LnkBall end; } Cand;

/* every stroke from here, scored: lower is better (a touch is best). Pass 0
 * plain strokes, pass 1 with a hop (the Dune Tread), pass 2 with a brake
 * (Backspin) at a few moments of the roll. */
static int try_all(const LnkBall *b, const LnkCtx *c, const LnkFlow *f, int layer, float px, float py, float rad,
                   Cand *out, int max, bool coarse) {
    static const int HOPS[4] = {6, 14, 24, 40};
    static const int BRAKES[5] = {8, 16, 26, 40, 60};
    int n = 0;
    bool chip = lnk_is_chip(b, c);
    for (int pass = 0; pass < 3; pass++) {
        if (pass > 0 && (chip || coarse)) break;
        if (pass == 1 && !has(c, AB_TREAD)) continue;
        if (pass == 2 && !has(c, AB_BACKSPIN)) continue;
        int nv = pass == 0 ? 1 : pass == 1 ? 4 : 5;
        for (int dir = 0; dir < LNK_DIRS; dir += coarse ? 2 : 1)
            for (int lv = 1; lv <= LNK_LEVELS; lv++) {
                if (pass == 1 && (lv < 4 || lv % 2)) continue;
                if (pass == 2 && (lv < 3 || lv % 2 == 0)) continue;
                for (int h = 0; h < nv; h++) {
                    int hop = pass == 1 ? HOPS[h] : -1, brk = pass == 2 ? BRAKES[h] : -1;
                    LnkBall e = *b;
                    bool touched;
                    int ev = lnk_sim(&e, c, dir, lv, hop, brk, layer, px, py, rad, &touched);
                    if (ev == EV_SINK) continue;
                    int fl = flow_at(f, &e);
                    int score;
                    if (touched) score = -100000 + lv;
                    else if (fl == LNK_FAR) continue;
                    else score = fl * 16 + lv;
                    if (n < max) out[n++] = (Cand){dir, lv, hop, brk, score, e};
                }
            }
    }
    return n;
}

static int cmp_cand(const void *a, const void *b) {
    const Cand *x = a, *y = b;
    return x->score - y->score;
}

bool lnk_plan_shot(const LnkBall *b, const LnkCtx *c, const LnkFlow *f, int layer, float px, float py, float rad,
                   LnkShot *out) {
    static Cand first[LNK_DIRS * LNK_LEVELS * 5], second[LNK_DIRS * LNK_LEVELS];
    LnkCtx pc = *c;
    pc.probe = true;
    int n = try_all(b, &pc, f, layer, px, py, rad, first, ARRAY_LEN(first), false);
    if (n == 0) return false;
    qsort(first, (size_t)n, sizeof first[0], cmp_cand);
    int best = 0, best_score = first[0].score;
    if (best_score > -50000) {
        /* look one stroke further for the few best (and different) places */
        int looked = 0;
        for (int i = 0; i < n && looked < 6; i++) {
            bool same = false;
            for (int j = 0; j < i && !same; j++)
                same = first[j].end.layer == first[i].end.layer && (int)(first[j].end.x / 8) == (int)(first[i].end.x / 8) &&
                       (int)(first[j].end.y / 8) == (int)(first[i].end.y / 8);
            if (same) continue;
            looked++;
            int m = try_all(&first[i].end, &pc, f, layer, px, py, rad, second, ARRAY_LEN(second), true);
            int s2 = 1 << 30;
            for (int k = 0; k < m; k++) s2 = imin(s2, second[k].score);
            int total = imin(first[i].score, s2 < -50000 ? -50000 + first[i].level : s2 + 24);
            if (total < best_score) { best_score = total; best = i; }
        }
    }
    out->dir = first[best].dir;
    out->level = first[best].level;
    out->hop_at = first[best].hop;
    out->brake_at = first[best].brake;
    out->score = best_score;
    return true;
}
