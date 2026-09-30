/* TILTSHOT - the demo player and the route finder.
 *
 * A shot is an aim, how long A is held and when (if at all) to slam. The
 * demo player turns a shot into button presses frame by frame, from the
 * play state alone, so the same shot gives the same stroke whether the game
 * runs it or the route finder tries it on a copy. The route finder searches
 * shot after shot (a beam of the best lies, judged by how far the ball is
 * from the cup through open air) until the ball drops; its routes, stored in
 * tiltshot_routes.c, are what the tests replay with real presses to show
 * every hole can be finished. */
#include "tiltshot.h"

uint8_t tsh_bot_buttons(const TshPlay *p, const TshShot *s) {
    switch (p->phase) {
    case TP_AIM:
        if (p->aim != s->aim) {
            if (p->prev & (TB_LEFT | TB_RIGHT)) return 0; /* let go between steps */
            return p->aim < s->aim ? TB_LEFT : TB_RIGHT;
        }
        if (p->prev & (TB_A | TB_LEFT | TB_RIGHT)) return 0;
        return TB_A;
    case TP_CHARGE:
        return p->meter >= s->power ? 0 : TB_A;
    case TP_FLIGHT:
        if (s->slam >= 0 && p->fly_t == s->slam && !(p->prev & TB_A)) return TB_A;
        return 0;
    default:
        return 0;
    }
}

int tsh_try_shot(TshPlay *p, const TshCourse *c, const TshShot *s) {
    bool fired = false;
    for (int f = 0; f < 6000; f++) {
        tsh_play_step(p, c, tsh_bot_buttons(p, s));
        p->fx = 0;
        if (p->phase == TP_FLIGHT) fired = true;
        else if (p->phase == TP_HOLED || p->phase == TP_LOST || p->phase == TP_BOOM) return p->phase;
        else if (fired && p->phase == TP_AIM) return TP_AIM;
    }
    return -1;
}

/* ---- the route finder --------------------------------------------------------- */

#define FAR 0x7fff
static int16_t dist[TSH_ROWS][TSH_MAXCOLS];

/* steps through open tiles from the cup */
static void flow(const TshCourse *c) {
    static int16_t qx[TSH_ROWS * TSH_MAXCOLS], qy[TSH_ROWS * TSH_MAXCOLS];
    for (int r = 0; r < TSH_ROWS; r++)
        for (int x = 0; x < TSH_MAXCOLS; x++) dist[r][x] = FAR;
    int h = 0, t = 0;
    dist[c->cup_row][c->cup_col] = 0;
    qx[t] = (int16_t)c->cup_col;
    qy[t++] = (int16_t)c->cup_row;
    while (h < t) {
        int x = qx[h], y = qy[h++];
        static const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (nx < 0 || nx >= c->cols || ny < 0 || ny >= TSH_ROWS) continue;
            if (dist[ny][nx] != FAR || tsh_tile_solid(c->tile[ny][nx])) continue;
            dist[ny][nx] = (int16_t)(dist[y][x] + 1);
            qx[t] = (int16_t)nx;
            qy[t++] = (int16_t)ny;
        }
    }
}

static int judge(const TshPlay *p) {
    int col = (int)floorf(p->x / TSH_T), row = (int)floorf(p->y / TSH_T);
    if (col < 0 || col >= TSH_MAXCOLS) return FAR;
    if (row < 0) row = 0;
    if (row >= TSH_ROWS) return FAR;
    int d = dist[row][col];
    return d == FAR ? FAR : d * 16 + (int)(fabsf(p->x - (col * TSH_T + 4)));
}

typedef struct Node {
    TshPlay p, before;   /* now, and before the last shot */
    TshShot shot[TSH_ROUTE_MAX];
    int n, score;
} Node;

#define BEAM_MAX 12
#define CAND_MAX 64
static Node beam[BEAM_MAX], next[BEAM_MAX], cand[CAND_MAX];
static int ncand;

/* keep the best CAND_MAX results, one per spot */
static void offer(const Node *from, const TshShot *s, const TshPlay *res, int score) {
    int cx = (int)(res->x / 6), cy = (int)(res->y / 6);
    for (int i = 0; i < ncand; i++) {
        if ((int)(cand[i].p.x / 6) != cx || (int)(cand[i].p.y / 6) != cy) continue;
        if (cand[i].score <= score) return;
        cand[i] = *from;
        cand[i].before = from->p;
        cand[i].p = *res;
        cand[i].shot[cand[i].n++] = *s;
        cand[i].score = score;
        return;
    }
    int at = ncand;
    if (ncand >= CAND_MAX) {
        int worst = 0;
        for (int i = 1; i < ncand; i++)
            if (cand[i].score > cand[worst].score) worst = i;
        if (cand[worst].score <= score) return;
        at = worst;
    } else ncand++;
    cand[at] = *from;
    cand[at].before = from->p;
    cand[at].p = *res;
    cand[at].shot[cand[at].n++] = *s;
    cand[at].score = score;
}

static const Node *won;
static Node won_node;
static long tries;

static void attempt(const TshCourse *c, const Node *from, const TshShot *s) {
    TshPlay q = from->p;
    tries++;
    int ph = tsh_try_shot(&q, c, s);
    if (ph == TP_HOLED) {
        if (!won || won_node.n > from->n + 1) {
            won_node = *from;
            won_node.shot[won_node.n++] = *s;
            won = &won_node;
        }
        return;
    }
    if (ph != TP_AIM) return;
    int sc = judge(&q);
    if (sc >= FAR) return;
    offer(from, s, &q, sc);
}

static int cmp_node(const void *a, const void *b) { return ((const Node *)a)->score - ((const Node *)b)->score; }

int tsh_solve(const TshCourse *c, const TshPlay *start, TshShot *out, int max, int beam_w, bool log) {
    flow(c);
    beam_w = iclamp(beam_w, 1, BEAM_MAX);
    int nb = 1;
    memset(&beam[0], 0, sizeof beam[0]);
    if (start) beam[0].p = *start;
    else tsh_play_begin(&beam[0].p, c);
    beam[0].p.fx = 0;
    beam[0].score = judge(&beam[0].p);
    won = NULL;
    tries = 0;
    for (int depth = 1; depth <= max && depth <= TSH_ROUTE_MAX - 1; depth++) {
        ncand = 0;
        for (int b = 0; b < nb && !won; b++) {
            const Node *from = &beam[b];
            /* every aim at every other power, no slam */
            for (int a = 0; a < TSH_AIMS && !won; a++)
                for (int pw = 0; pw <= TSH_FILL && !won; pw += 2) {
                    TshShot s = {(int8_t)a, (int8_t)pw, -1};
                    attempt(c, from, &s);
                }
            /* slams on a coarser grid */
            for (int a = 0; a < TSH_AIMS && !won; a += 2)
                for (int pw = 4; pw <= TSH_FILL && !won; pw += 4)
                    for (int sl = 8; sl <= 120 && !won; sl += 8) {
                        TshShot s = {(int8_t)a, (int8_t)pw, (int16_t)sl};
                        attempt(c, from, &s);
                    }
        }
        /* polish the best few: nudge each part of the last shot */
        qsort(cand, (size_t)ncand, sizeof cand[0], cmp_node);
        int polish = imin(ncand, 6);
        for (int i = 0; i < polish && !won; i++) {
            Node base = cand[i];
            base.n--;
            base.p = cand[i].before;
            TshShot s0 = cand[i].shot[cand[i].n - 1];
            for (int da = -1; da <= 1 && !won; da++)
                for (int dp = -3; dp <= 3 && !won; dp++)
                    for (int ds = -6; ds <= 6 && !won; ds += (s0.slam < 0 ? 13 : 1)) {
                        TshShot s = {(int8_t)iclamp(s0.aim + da, 0, TSH_AIMS - 1), (int8_t)iclamp(s0.power + dp, 0, TSH_FILL),
                                     (int16_t)(s0.slam < 0 ? -1 : imax(2, s0.slam + ds))};
                        attempt(c, &base, &s);
                    }
        }
        if (log) {
            qsort(cand, (size_t)ncand, sizeof cand[0], cmp_node);
            printf("  hole %d depth %d: %ld tries, best %d at (%d,%d)%s\n", c->hole + 1, depth, tries,
                   ncand ? cand[0].score : -1, ncand ? (int)cand[0].p.x : -1, ncand ? (int)cand[0].p.y : -1,
                   won ? " HOLED" : "");
            fflush(stdout);
        }
        if (won) {
            for (int i = 0; i < won->n; i++) out[i] = won->shot[i];
            return won->n;
        }
        qsort(cand, (size_t)ncand, sizeof cand[0], cmp_node);
        nb = imin(ncand, beam_w);
        for (int i = 0; i < nb; i++) next[i] = cand[i];
        for (int i = 0; i < nb; i++) beam[i] = next[i];
        if (!nb) break;
    }
    return 0;
}

/* A player with an unsteady hand: from each lie it weighs a grid of shots
 * (aims two steps apart, strengths four frames apart, and slams at a few
 * moments), each tried a few times with the strength and the slam a little
 * off, and plays the one that does best on average; then its own swing is a
 * little off too. It knows the course but not the exact outcome, much like
 * a player who has learned the holes. Returns the strokes it took (40 means
 * it gave up). A yardstick for the hole designs. */
static int wobble(Rng *r, int spread) { return rng_range(r, -spread, spread); }

int tsh_steady_play(const TshCourse *c, uint64_t seed, int *lost_out) {
    flow(c);
    Rng r;
    rng_seed(&r, seed);
    TshPlay p;
    tsh_play_begin(&p, c);
    int lost = 0;
    while (p.strokes < 40) {
        TshShot best = {0, 0, -1};
        float best_sc = 1e30f;
        for (int kind = 0; kind < 2; kind++)
            for (int a = 0; a < TSH_AIMS; a += kind ? 4 : 2)
                for (int pw = 4; pw <= TSH_FILL; pw += kind ? 8 : 4)
                    for (int sl = kind ? 15 : -1; sl <= (kind ? 75 : -1); sl += 15) {
                        TshShot s = {(int8_t)a, (int8_t)pw, (int16_t)sl};
                        float tot = 0;
                        for (int k = 0; k < 4; k++) {
                            TshShot t = s;
                            static const int8_t DP[4] = {-5, -2, 2, 5}, DS[4] = {-6, 3, -3, 6};
                            t.power = (int8_t)iclamp(pw + DP[k], 0, TSH_FILL);
                            if (sl >= 0) t.slam = (int16_t)imax(2, sl + DS[k]);
                            TshPlay q = p;
                            int ph = tsh_try_shot(&q, c, &t);
                            tot += ph == TP_HOLED ? -40.0f : ph == TP_AIM ? (float)judge(&q) : (float)judge(&p) + 80.0f;
                        }
                        if (tot < best_sc) { best_sc = tot; best = s; }
                    }
        TshShot t = best;
        t.power = (int8_t)iclamp(best.power + wobble(&r, 5), 0, TSH_FILL);
        if (t.slam >= 0) t.slam = (int16_t)imax(2, t.slam + wobble(&r, 6));
        int ph = tsh_try_shot(&p, c, &t);
        if (ph == TP_HOLED) break;
        if (ph == TP_LOST) {
            lost++;
            for (int f = 0; f < TSH_LOST_T + 2 && p.phase == TP_LOST; f++) tsh_play_step(&p, c, 0);
        }
        p.fx = 0;
    }
    if (lost_out) *lost_out = lost;
    return p.strokes;
}
