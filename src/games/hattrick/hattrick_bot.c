/* HAT TRICK - the demo player, for the tests. It plays with the same
 * buttons a person has: every few frames it makes up a couple of dozen short
 * plans (run this way so long, jump here, kick there with this aim, or
 * slide), plays each one ahead on a copy of the screen, and keeps the plan
 * that loses no life and gets the most done (kills, boss hits, food, the
 * ball back). The runner then presses its buttons for real. Fewer plans a
 * decision make a weaker player: the fairness check uses one. */
#include "hattrick.h"

int htk_bot_width = 64;

#define PLAN_MAX 160
#define COMMIT 10
#define TAIL 24

typedef struct {
    uint16_t plan[PLAN_MAX];
    int len, pos;
    int since_touch, since_hit, kills_seen;
    Rng rng;
} BotMind;

static BotMind mind[2];
static HtkPlay sim;
static int nav_level;

void htk_bot_reset(void) {
    memset(mind, 0, sizeof mind);
    nav_level = -99;
    rng_seed(&mind[0].rng, 11);
    rng_seed(&mind[1].rng, 1984);
}

/* ---- finding the way: which ledges lead to which ------------------------------------ */

#define NAV_N (HTK_ROWS * HTK_COLS)
#define NAV_MAXE 200
static int16_t nav_edge[NAV_N][NAV_MAXE];  /* the nodes that lead here (reversed) */
static uint16_t nav_ne[NAV_N];
static uint8_t nav_ok[NAV_N];               /* a place a kid can stand */
static int16_t nav_ball[NAV_N], nav_foe[NAV_N];
static int nav_level = -99;

static int wr(int r) { r %= HTK_ROWS; return r < 0 ? r + HTK_ROWS : r; }
static int wc(int c) { c %= HTK_COLS; return c < 0 ? c + HTK_COLS : c; }
static bool t_solid(const HtkPlay *g, int r, int c) { int t = g->tile[wr(r)][wc(c)]; return t == T_SOLID || t == T_GOAL; }
static bool t_ground(const HtkPlay *g, int r, int c) { return g->tile[wr(r)][wc(c)] != T_EMPTY; }
static bool standable(const HtkPlay *g, int r, int c) {
    if (!t_ground(g, r, c)) return false;
    for (int k = 1; k <= 3; k++)
        if (t_solid(g, r - k, c)) return false;
    return true;
}
static int fall_row(const HtkPlay *g, int r, int c) {
    for (int k = 0; k < 2 * HTK_ROWS; k++)
        if (t_ground(g, r + k, c)) return wr(r + k);
    return -1;
}
static void add_edge(int from, int to) {
    if (from == to || nav_ne[to] >= NAV_MAXE) return;
    for (int i = 0; i < nav_ne[to]; i++)
        if (nav_edge[to][i] == from) return;
    nav_edge[to][nav_ne[to]++] = (int16_t)from;
}

static void nav_build(const HtkPlay *g) {
    if (nav_level == g->level) return;
    nav_level = g->level;
    memset(nav_ne, 0, sizeof nav_ne);
    for (int r = 0; r < HTK_ROWS; r++)
        for (int c = 0; c < HTK_COLS; c++) nav_ok[r * HTK_COLS + c] = standable(g, r, c) && r > 0;
    for (int r = 0; r < HTK_ROWS; r++)
        for (int c = 0; c < HTK_COLS; c++) {
            int u = r * HTK_COLS + c;
            if (!nav_ok[u]) continue;
            for (int dc = -1; dc <= 1; dc += 2) {
                int c2 = wc(c + dc);
                if (nav_ok[r * HTK_COLS + c2]) add_edge(u, r * HTK_COLS + c2);
                else if (!t_solid(g, r - 1, c2) && !t_solid(g, r - 2, c2))
                    for (int d2 = 0; d2 <= 2; d2++) {
                        int c3 = wc(c2 + d2 * dc), f = fall_row(g, r, c3);
                        if (f >= 0 && nav_ok[f * HTK_COLS + c3]) add_edge(u, f * HTK_COLS + c3);
                    }
            }
            for (int up = -6; up <= 4; up++) {
                /* (up 0: a jump across a gap on the same level) */
                int reach = up == 4 ? 3 : up > 0 ? 5 : 6; /* a jump to the next ledge up: 3 tiles across at most */
                bool blocked = false;
                for (int k = 1; k <= up + 3 && up > 0; k++)
                    if (t_solid(g, r - k, c)) blocked = true;
                if (blocked || r - up < 0 || r - up >= HTK_ROWS) continue;
                for (int dc = -reach; dc <= reach; dc++) {
                    int r2 = wr(r - up), c2 = wc(c + dc);
                    if (!nav_ok[r2 * HTK_COLS + c2]) continue;
                    /* no wall in the way across */
                    bool wall = false;
                    for (int k = 1; k <= abs(dc) && !wall; k++) {
                        int cc = c + (dc > 0 ? k : -k);
                        int top = imin(r, r2) - 3, bot = k == abs(dc) ? r2 - 1 : imax(r, r2) - 1;
                        for (int rr = top; rr <= bot && !wall; rr++)
                            if (t_solid(g, rr, cc)) wall = true;
                    }
                    if (!wall) add_edge(u, r2 * HTK_COLS + c2);
                }
            }
        }
}

/* the steps from every place to the nearest of the targets */
static void nav_bfs(int16_t *dist, const int *targets, int nt) {
    static int16_t q[NAV_N];
    int qh = 0, qt = 0;
    for (int i = 0; i < NAV_N; i++) dist[i] = -1;
    for (int i = 0; i < nt; i++)
        if (targets[i] >= 0 && dist[targets[i]] < 0) { dist[targets[i]] = 0; q[qt++] = (int16_t)targets[i]; }
    while (qh < qt) {
        int v = q[qh++];
        for (int i = 0; i < nav_ne[v]; i++) {
            int u = nav_edge[v][i];
            if (dist[u] >= 0) continue;
            dist[u] = (int16_t)(dist[v] + 1);
            q[qt++] = (int16_t)u;
        }
    }
}

/* the place under a point (the ground it would land on), or -1 */
static int node_under(const HtkPlay *g, float x, float y) {
    int c = wc((int)floorf(x / HTK_T)), r = (int)floorf(y / HTK_T);
    int f = fall_row(g, r, c);
    for (int k = 0; k < 3 && f >= 0; k++) {
        int cc = wc(c + (k == 1 ? 1 : k == 2 ? -1 : 0));
        if (nav_ok[f * HTK_COLS + cc]) return f * HTK_COLS + cc;
    }
    return -1;
}

static int kid_node(const HtkPlay *g, const HtkPlayer *p) { return node_under(g, p->x, p->y - 1); }

/* ---- making a plan ----------------------------------------------------------------- */

static int make_plan(BotMind *m, const HtkPlay *g, int who, uint16_t *out) {
    Rng *r = &m->rng;
    const HtkPlayer *p = &g->pl[who];
    bool carrying = g->ball.carrier == who;
    int explore = m->since_touch > 900 || m->since_hit > 900 ? 2 : 1;
    int d = rng_range(r, -1, 1);
    /* lean towards the ball when we haven't got it */
    if (!carrying && rng_chance(r, 45)) {
        float dx = htk_wrapdx(g->ball.x - p->x);
        d = dx > 4 ? 1 : dx < -4 ? -1 : 0;
    }
    int move_n = rng_range(r, 0, 40 * explore);
    int jump_at = rng_chance(r, 45) ? rng_range(r, 0, imax(0, move_n)) : -1;
    int len = move_n + 1;
    uint16_t dirb = d > 0 ? BTN_RIGHT : d < 0 ? BTN_LEFT : 0;
    for (int f = 0; f < PLAN_MAX; f++) out[f] = 0;
    for (int f = 0; f < move_n && f < PLAN_MAX; f++) out[f] |= dirb;
    if (jump_at >= 0) {
        /* mostly a full jump (A held all the way up), now and then a hop */
        int hold = rng_chance(r, 70) ? 22 : rng_range(r, 1, 10);
        for (int f = jump_at; f < jump_at + hold && f < PLAN_MAX; f++) out[f] |= BTN_A;
        len = imax(len, imin(jump_at + hold, PLAN_MAX - 1));
    }
    if (rng_chance(r, 8) && !carrying) {
        /* duck for a moment */
        int n = rng_range(r, 8, 30);
        for (int f = 0; f < n && f < PLAN_MAX; f++) out[f] = BTN_DOWN;
        len = imax(len, n);
    }
    if (carrying && rng_chance(r, 85)) {
        int at = rng_range(r, 0, move_n + 10);
        int charge = rng_chance(r, 30) ? HTK_CHARGE_T + 2 : 1;
        int aim = rng_range(r, 0, 3); /* 0 none, 1 up, 2 down, 3 sideways only */
        int kd = rng_range(r, -1, 1);
        uint16_t aimb = (aim == 1 ? BTN_UP : aim == 2 ? BTN_DOWN : 0) | (kd > 0 ? BTN_RIGHT : kd < 0 ? BTN_LEFT : 0);
        if (aim == 3 && !kd) aimb = p->facing > 0 ? BTN_RIGHT : BTN_LEFT;
        int end = at + charge;
        if (end + 1 >= PLAN_MAX) { at = PLAN_MAX - charge - 3; end = at + charge; }
        for (int f = at; f < end; f++) out[f] |= BTN_B;
        for (int f = imax(0, end - 3); f <= end; f++) out[f] = (uint16_t)((out[f] & (BTN_A | BTN_B)) | aimb);
        len = imax(len, end + 2);
    } else if (!carrying && rng_chance(r, 30)) {
        int at = rng_range(r, 0, imax(1, move_n));
        out[at] |= BTN_B;
        len = imax(len, at + 2);
    }
    return imin(len, PLAN_MAX - 1);
}

/* ---- judging a plan ------------------------------------------------------------------- */

static float dist(float ax, float ay, float bx, float by) {
    float dx = htk_wrapdx(ax - bx), dy = htk_wrapdy(ay - by);
    return sqrtf(dx * dx + dy * dy);
}

static int boss_hp_total(const HtkPlay *g) {
    int n = 0;
    for (int i = 0; i < HTK_MAX_FOES; i++) {
        const HtkFoe *f = &g->foe[i];
        if (f->alive && f->kind >= FK_KINGSPIKER && f->kind != FK_TIMEKEEPER)
            n += f->away ? 0 : f->hp + (f->kind == FK_TOWER ? 100 : 0);
    }
    return n;
}

static float judge(const HtkPlay *g0, int who, const uint16_t *plan, int len, uint16_t other) {
    memcpy(&sim, g0, sizeof sim);
    int spare0 = sim.pl[who].spare, kills0 = sim.kills, boss0 = boss_hp_total(&sim);
    uint32_t score0 = sim.pl[who].score;
    float danger = 0, s = 0, near = 1e9f;
    int dead_at = -1, lit0 = sim.ball.lit;
    for (int f = 0; f < len + TAIL; f++) {
        uint16_t b = f < len ? plan[f] : 0;
        if (f >= len && sim.ball.carrier == who && sim.pl[who].charging) b = BTN_B; /* don't let go of a charge by accident */
        if (who == 0) htk_step(&sim, b, other);
        else htk_step(&sim, other, b);
        const HtkPlayer *p = &sim.pl[who];
        if (!p->alive || p->out || p->spare < spare0) { dead_at = f; break; }
        /* how close things come */
        if (f < len + 8) {
            float py = p->y - 9;
            for (int i = 0; i < HTK_MAX_FOES; i++) {
                const HtkFoe *e = &sim.foe[i];
                if (!e->alive || e->away) continue;
                float d = dist(e->x, e->y, p->x, py) - e->hw - 6;
                if (d < 14) danger += (14 - d) * 1.5f;
            }
            for (int i = 0; i < HTK_MAX_SHOTS; i++) {
                const HtkShot *e = &sim.shot[i];
                if (!e->alive) continue;
                float d = dist(e->x, e->y, p->x, py) - 6;
                if (d < 12) danger += (12 - d) * 1.5f;
            }
        }
        /* a flying ball that passes close to something is a good try */
        if (sim.ball.carrier < 0 && htk_ball_speed(&sim) > HTK_KILL_SPEED && sim.ball.last == who)
            for (int i = 0; i < HTK_MAX_FOES; i++) {
                const HtkFoe *e = &sim.foe[i];
                if (!e->alive || e->away || e->kind == FK_TIMEKEEPER) continue;
                near = fminf(near, dist(e->x, e->y, sim.ball.x, sim.ball.y) - e->hw);
            }
        if (sim.sub == LS_EXIT) break;
    }
    if (dead_at >= 0) return -1e7f + dead_at * 1000.0f;
    const HtkPlayer *p = &sim.pl[who];
    s += (sim.kills - kills0) * 5000.0f;
    s += (boss0 - boss_hp_total(&sim)) * 1600.0f;
    s += (float)(p->score - score0);
    if (g0->sub == LS_PLAY && sim.sub != LS_PLAY) s += 300000.0f;
    s -= danger;
    if (near < 50) s += (50 - near) * 40;
    if (lit0 && !sim.ball.lit) s -= 1500;
    if (sim.sub == LS_CLEAR || sim.sub == LS_EXIT) {
        /* pick up the food */
        float best = 1e9f;
        for (int i = 0; i < HTK_MAX_ITEMS; i++) {
            const HtkItem *it = &sim.item[i];
            if (it->alive && !it->falling) best = fminf(best, dist(it->x, it->y, p->x, p->y - 8));
        }
        if (best < 1e8f) s -= best * 12;
        return s;
    }
    const HtkBall *b = &sim.ball;
    if (b->carrier == who) {
        s += 800;
        /* and get within reach of something to kick it at */
        float best = 1e9f;
        for (int i = 0; i < HTK_MAX_FOES; i++) {
            const HtkFoe *e = &sim.foe[i];
            if (e->alive && !e->away && e->kind != FK_TIMEKEEPER) best = fminf(best, dist(e->x, e->y, p->x, p->y - 9));
        }
        if (best < 1e8f) s -= fabsf(best - 60) * 2;
        int n = kid_node(&sim, p), d = n >= 0 ? nav_foe[n] : -1;
        s -= d >= 0 ? d * 120.0f : 2500.0f;
        if (b->lit) s += 300;
    } else if (b->carrier < 0) {
        int n = kid_node(&sim, p), d = n >= 0 ? nav_ball[n] : -1;
        s -= d >= 0 ? d * 150.0f : 3000.0f;
        s -= dist(b->x, b->y, p->x, p->y - 6) * 6;
        if (htk_ball_speed(&sim) > HTK_KILL_SPEED) s += 400; /* still flying: it may yet hit something */
    }
    return s;
}

static void replan(HtkPlay *g, int who) {
    BotMind *m = &mind[who];
    static uint16_t cand[PLAN_MAX], best[PLAN_MAX];
    float best_s = -1e30f;
    int best_len = 0;
    bool was_sim = htk_sim;
    htk_sim = true;
    uint16_t other = 0;
    /* where the ball is, and where the creatures are, as places to get to */
    nav_build(g);
    int bt = node_under(g, g->ball.x, g->ball.y);
    nav_bfs(nav_ball, &bt, 1);
    /* places to shoot from: the creature's own ledge, and anywhere a little
     * below it and to either side */
    static int ft[NAV_N];
    int nf = 0;
    for (int i = 0; i < HTK_MAX_FOES; i++) {
        const HtkFoe *e = &g->foe[i];
        if (!e->alive || e->away || e->kind == FK_TIMEKEEPER) continue;
        int fr = (int)floorf(e->y / HTK_T), fc = (int)floorf(e->x / HTK_T);
        for (int dr = 1; dr <= 7; dr++)
            for (int dc = -7; dc <= 7; dc++) {
                int cc = fc + dc, rr = fr + dr;
                /* round through an edge only where the screen is open there */
                if ((cc < 0 || cc >= HTK_COLS) && t_solid(g, rr, 0)) continue;
                int n = wr(rr) * HTK_COLS + wc(cc);
                if (nav_ok[n] && nf < NAV_N) ft[nf++] = n;
            }
    }
    nav_bfs(nav_foe, ft, nf);
    /* the rest of the plan we were following is always a candidate */
    if (m->pos < m->len) {
        int n = m->len - m->pos;
        memcpy(cand, m->plan + m->pos, (size_t)n * sizeof cand[0]);
        float s = judge(g, who, cand, n, other);
        best_s = s;
        best_len = n;
        memcpy(best, cand, (size_t)n * sizeof best[0]);
    }
    /* every running jump: run one way, take off at each moment in turn, and
     * keep running in the air */
    if (htk_bot_width >= 16)
        for (int side = -1; side <= 1; side += 2)
            for (int j = 0; j <= 42; j += 3) {
                uint16_t dirb = side > 0 ? BTN_RIGHT : BTN_LEFT;
                int len = j + 34;
                memset(cand, 0, sizeof cand);
                for (int f = 0; f < len; f++) cand[f] = dirb;
                for (int f = j; f < j + 22; f++) cand[f] |= BTN_A; /* held: the full height */
                float sc = judge(g, who, cand, len, other);
                if (sc > best_s) {
                    best_s = sc;
                    best_len = len;
                    memcpy(best, cand, (size_t)len * sizeof best[0]);
                }
            }
    /* with the ball at its feet: every straight shot from here, each aim,
     * tapped or driven, either way */
    if (g->ball.carrier == who && g->pl[who].ground && htk_bot_width >= 16) {
        static const uint16_t AIMS[5] = {0, BTN_UP, BTN_DOWN, BTN_UP | 0x8000, 0x8000};
        for (int side = -1; side <= 1; side += 2)
            for (int a = 0; a < 5; a++)
                for (int charge = 0; charge < 2; charge++) {
                    uint16_t dirb = side > 0 ? BTN_RIGHT : BTN_LEFT;
                    uint16_t aim = (uint16_t)(AIMS[a] & 0xFF);
                    if (AIMS[a] & 0x8000) aim |= dirb;
                    int n = charge ? HTK_CHARGE_T + 2 : 1, len = n + 2;
                    memset(cand, 0, sizeof cand);
                    cand[0] = dirb; /* turn that way first */
                    for (int f = 1; f <= n; f++) cand[f] = BTN_B;
                    cand[n] = (uint16_t)(BTN_B | aim);
                    cand[n + 1] = aim;
                    float sc = judge(g, who, cand, len, other);
                    if (sc > best_s) {
                        best_s = sc;
                        best_len = len;
                        memcpy(best, cand, (size_t)len * sizeof best[0]);
                    }
                }
    }
    /* when every plan so far loses the life, look much harder; and when
     * nothing has been hit for a long while, try more and longer plans */
    int tries = htk_bot_width * (m->since_hit > 900 ? 3 : 1);
    for (int k = 0; k < tries; k++) {
        if (k == tries - 1 && best_s < -1e6f && tries < htk_bot_width * 6) tries += htk_bot_width;
        int len = make_plan(m, g, who, cand);
        float s = judge(g, who, cand, len, other);
        if (s > best_s) {
            best_s = s;
            best_len = len;
            memcpy(best, cand, (size_t)len * sizeof best[0]);
        }
    }
    htk_sim = was_sim;
    memcpy(m->plan, best, (size_t)best_len * sizeof best[0]);
    m->len = best_len;
    m->pos = 0;
}

uint16_t htk_bot_buttons(HtkPlay *g, int who) {
    BotMind *m = &mind[who];
    const HtkPlayer *p = &g->pl[who];
    if (!p->on || p->out || !p->alive || g->sub == LS_INTRO || g->sub == LS_EXIT || g->sub == LS_GOAL) {
        m->len = m->pos = 0;
        return 0;
    }
    if (g->ball.carrier == who) m->since_touch = 0;
    else m->since_touch++;
    int hits = g->kills * 100;
    for (int i = 0; i < HTK_MAX_FOES; i++)
        if (g->foe[i].alive && g->foe[i].kind >= FK_KINGSPIKER && g->foe[i].kind != FK_TIMEKEEPER) hits -= g->foe[i].hp;
    if (hits != m->kills_seen) { m->kills_seen = hits; m->since_hit = 0; }
    else m->since_hit++;
    if (m->pos >= m->len || m->pos >= COMMIT) replan(g, who);
    return m->pos < m->len ? m->plan[m->pos++] : 0;
}

/* ---- how fair a screen is ------------------------------------------------------------------ */

static HtkPlay fair;
int htk_fair_killer[128]; /* what took the life in the tries that lost one */

/* n tries at screen L by a player who weighs `width` plans a decision, with
 * no spare lives: returns how many keep the life until the screen is
 * cleared or the clock runs out (before extra time). Also
 * the shortest time a kid who just stands at the start lasts (cap 600). */
int htk_fair_runs(int level, int n, int width, int *worst_first_t) {
    bool was_sim = htk_sim;
    int keep_width = htk_bot_width, ok = 0, worst = 600;
    BotMind keep[2];
    memcpy(keep, mind, sizeof keep);
    htk_sim = true;
    htk_bot_width = width;
    memset(htk_fair_killer, 0, sizeof htk_fair_killer);
    for (int r = 0; r < n; r++) {
        for (int idle = 0; idle < 2; idle++) {
            memset(&fair, 0, sizeof fair);
            fair.mode = MODE_1P;
            fair.pl[0].on = 1;
            fair.pl[0].ch = (uint8_t)(r & 1);
            rng_seed(&fair.rng, (uint64_t)(r * 7919 + level * 31 + 5));
            htk_load_level(&fair, level);
            fair.sub = LS_PLAY;
            htk_bot_reset();
            rng_seed(&mind[0].rng, (uint64_t)(r * 104729 + 17));
            int f = 0;
            for (; f < (idle ? 600 : HTK_COUNTS * HTK_TICK); f++) {
                uint16_t b = idle ? 0 : htk_bot_buttons(&fair, 0);
                htk_step(&fair, b, 0);
                if (!fair.pl[0].alive || fair.pl[0].out || fair.sub != LS_PLAY) break;
            }
            if (idle) {
                if (!fair.pl[0].alive || fair.pl[0].out) worst = imin(worst, f);
            } else if (fair.pl[0].alive && !fair.pl[0].out) ok++; /* cleared, or the clock ran out first */
            else if (!fair.pl[0].alive && fair.pl[0].hit_by >= 0 && fair.pl[0].hit_by < 128) htk_fair_killer[fair.pl[0].hit_by]++;
        }
    }
    htk_bot_width = keep_width;
    memcpy(mind, keep, sizeof keep);
    htk_sim = was_sim;
    if (worst_first_t) *worst_first_t = worst;
    return ok;
}

/* for the tests: how far (in steps) the kid stands from the ball and from a
 * place to shoot from, as the demo player saw it at its last decision */
int htk_bot_nav(const HtkPlay *g, int who, int what) {
    if (nav_level != g->level) return -2;
    int n = kid_node(g, &g->pl[who]);
    if (n < 0) return -3;
    return what ? nav_foe[n] : nav_ball[n];
}

/* For the tests: on the screen in play, how many places a kid can stand
 * that can't be reached from the start, or can't get back to it, and how
 * many ledges a ball could come to rest on with no room for a kid. Both
 * must be 0: wherever the ball settles, a kid can get to it. */
int htk_reach_log;
void htk_reach_check(const HtkPlay *g, int *unreach, int *dead) {
    nav_level = -99;
    nav_build(g);
    static int16_t back[NAV_N];
    static uint8_t fwd[NAV_N];
    int start = node_under(g, g->pl[0].start_x, g->pl[0].start_y - 1);
    nav_bfs(back, &start, 1);
    memset(fwd, 0, sizeof fwd);
    static int16_t q[NAV_N];
    int qh = 0, qt = 0;
    if (start >= 0) { fwd[start] = 1; q[qt++] = (int16_t)start; }
    while (qh < qt) {
        int u = q[qh++];
        for (int v = 0; v < NAV_N; v++) {
            if (fwd[v]) continue;
            for (int i = 0; i < nav_ne[v]; i++)
                if (nav_edge[v][i] == u) { fwd[v] = 1; q[qt++] = (int16_t)v; break; }
        }
    }
    *unreach = 0;
    *dead = 0;
    for (int r = 1; r < HTK_ROWS; r++)
        for (int c = 0; c < HTK_COLS; c++) {
            int n = r * HTK_COLS + c;
            if (nav_ok[n] && (!fwd[n] || back[n] < 0)) {
                (*unreach)++;
                if (htk_reach_log) printf("  # unreachable %d,%d (%s)\n", r, c, !fwd[n] ? "can't get there" : "can't get back");
            }
            if (t_ground(g, r, c) && g->tile[r - 1][c] == T_EMPTY && !nav_ok[n]) (*dead)++;
        }
    nav_level = -99;
}
