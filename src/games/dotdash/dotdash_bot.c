/* DOT & DASH - the demo player for the tests. It plans a route over the
 * tiles Dot can stand on, where every step (a walk, a hop, a jump steered
 * for so many frames) is tried with the game's own physics first, then it
 * presses those same buttons for real, one frame at a time. */
#include "dotdash.h"

int dd_bot_state;

/* ---- the moves --------------------------------------------------------- */
typedef struct Prog { int8_t dir; uint8_t a, s, d, up; } Prog;
#define MAXPROG 80
static Prog prog[MAXPROG];
static int n_prog;

static void build_progs(void) {
    n_prog = 0;
    static const uint8_t WALK[] = {4, 8, 14, 22};
    for (int d = -1; d <= 1; d += 2)
        for (int k = 0; k < 4; k++) prog[n_prog++] = (Prog){(int8_t)d, 0, 0, WALK[k], 0};
    static const uint8_t A[] = {5, 12, 40};
    static const uint8_t S[] = {0, 12};
    static const uint8_t D[] = {6, 12, 20, 30, 45, 70};
    for (int up = 0; up <= 1; up++)
        for (int a = 0; a < 3; a++) {
            if (up && a < 2) continue;
            prog[n_prog++] = (Prog){0, A[a], 0, 0, (uint8_t)up};
            for (int d = -1; d <= 1; d += 2)
                for (int s = 0; s < 2; s++)
                    for (int k = 0; k < 6; k++) {
                        if (n_prog >= MAXPROG) break;
                        if (s == 1 && k < 2) continue;
                        prog[n_prog++] = (Prog){(int8_t)d, A[a], S[s], D[k], (uint8_t)up};
                    }
        }
}

static uint32_t prog_buttons(const Prog *p, int f) {
    uint32_t b = 0;
    if (p->a && f < p->a) b |= BTN_A;
    if (p->up && f < p->a) b |= BTN_UP;
    if (p->dir && f >= p->s && f < p->s + p->d) b |= p->dir < 0 ? BTN_LEFT : BTN_RIGHT;
    return b;
}

/* ---- the graph ----------------------------------------------------------- */
#define NMAX (LV_MAXW * LV_MAXH)
#define EMAX 300000
typedef struct Edge { int32_t dest; uint16_t cost; uint8_t prog, pad; } Edge;
static Edge edge[EMAX];
static int n_edge;
static int32_t first[NMAX];
static uint8_t count[NMAX];
static uint8_t done_[NMAX];
static uint32_t sig;

static int TS, LW, LH, BW, BH;
static const Phys *PH;
static int BEAN;

static uint32_t level_sig(void) {
    uint32_t h = dd_hash((uint32_t)dd_lv.w, (uint32_t)dd_lv.h) ^ dd_lv.key ^ ((uint32_t)dd_scale << 20) ^ (dd_sv.ups * 31u);
    for (int i = 0; i < dd_lv.w * dd_lv.h; i += 1) h = h * 16777619u ^ dd_lv.t[i];
    return h ^ (uint32_t)dd_lv.d.x0 * 977u ^ (uint32_t)dd_lv.d.row;
}

static void graph_reset(void) {
    TS = dd_scale == SC_FULL ? DD_TS0 : DD_TS;
    LW = dd_lv.w;
    LH = dd_lv.h;
    BW = dd_p.w;
    BH = dd_p.h;
    PH = dd_phys();
    BEAN = dd_scale == SC_FULL ? 0 : dd_has(U_BEAN2) ? 2 : dd_has(U_BEAN1) ? 1 : 0;
    n_edge = 0;
    memset(done_, 0, (size_t)(LW * LH));
    if (!n_prog) build_progs();
}

static void ensure_graph(void) {
    uint32_t s = level_sig();
    if (s != sig || TS != (dd_scale == SC_FULL ? DD_TS0 : DD_TS) || BW != dd_p.w) { sig = s; graph_reset(); }
}

static float node_x(int tx) { return (float)(tx * TS) + (float)TS / 2 - (float)BW / 2; }
static float node_y(int ty) { return (float)((ty + 1) * TS) - (float)BH; }

static bool standable(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= LW || ty >= LH) return false;
    float x = node_x(tx), y = node_y(ty);
    if (dd_body_blocked(&dd_lv, TS, x, y, (float)BW, (float)BH)) return false;
    int fl = 0;
    for (int xx = (int)(x + 0.5f) / TS; xx <= (int)(x + BW - 0.5f) / TS; xx++) fl |= DD_TILE[lv_tile(&dd_lv, xx, ty + 1)].flags;
    if (!(fl & (TF_SOLID | TF_ONEWAY))) return false;
    if (fl & TF_BOUNCE) return false;
    return true;
}

static bool hazard(const Body *b) {
    if (TS != DD_TS) return false;
    int x0 = (int)((b->x + 1) / DD_TS), x1 = (int)((b->x + b->w - 1) / DD_TS);
    int y0 = (int)((b->y + 2) / DD_TS), y1 = (int)((b->y + b->h - 1) / DD_TS);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (DD_TILE[lv_tile(&dd_lv, tx, ty)].flags & (TF_HURT | TF_GOO)) return true;
    return false;
}

/* try one move from a node: the node it ends on and the frames it takes */
static int simulate(int tx, int ty, const Prog *p, int *dest, int *frames) {
    Body b;
    memset(&b, 0, sizeof b);
    b.w = (int16_t)BW;
    b.h = (int16_t)BH;
    b.x = node_x(tx);
    b.y = node_y(ty);
    b.ground = 1;
    b.facing = p->dir >= 0;
    b.peak_y = (int16_t)b.y;
    uint32_t prev = 0;
    int f;
    for (f = 0; f < 160; f++) {
        uint32_t in = prog_buttons(p, f);
        dd_body_step(&b, in, prev, &dd_lv, PH, false, BEAN);
        prev = in;
        if (hazard(&b)) return 0;
        if (b.y > LH * TS) return 0;
        bool active = (int)f < (int)p->s + p->d || f < p->a;
        if (!active && b.ground && fabsf(b.vx) < 0.05f && f > 2) break;
    }
    if (f >= 160 || !b.ground) return 0;
    if (TS == DD_TS && b.landed_fall > 12 * DD_TS && !dd_has(U_FEATHER)) return 0; /* would hurt */
    int nx = (int)((b.x + b.w / 2) / TS), ny = (int)((b.y + b.h - 1) / TS);
    if (!standable(nx, ny)) {
        /* settled off-centre: still counts where the feet are */
        if (nx < 0 || ny < 0 || nx >= LW || ny >= LH) return 0;
    }
    *dest = ny * LW + nx;
    *frames = f + 1;
    return 1;
}

static void expand(int node) {
    if (done_[node]) return;
    done_[node] = 1;
    first[node] = n_edge;
    count[node] = 0;
    int tx = node % LW, ty = node / LW;
    for (int k = 0; k < n_prog && n_edge < EMAX; k++) {
        const Prog *p = &prog[k];
        if (p->up && !BEAN) continue;
        int dest, frames;
        if (!simulate(tx, ty, p, &dest, &frames) || dest == node) continue;
        /* keep the quickest move to each place */
        int j;
        for (j = first[node]; j < first[node] + count[node]; j++)
            if (edge[j].dest == dest) break;
        if (j < first[node] + count[node]) {
            if (frames < edge[j].cost) { edge[j].cost = (uint16_t)frames; edge[j].prog = (uint8_t)k; }
            continue;
        }
        if (count[node] == 255) break;
        edge[n_edge++] = (Edge){dest, (uint16_t)frames, (uint8_t)k, 0};
        count[node]++;
    }
}

/* ---- A* -------------------------------------------------------------- */
static int32_t gcost[NMAX], parent[NMAX];
static uint8_t parent_prog[NMAX];
static uint32_t stamp[NMAX], cur_stamp;
typedef struct HeapN { int32_t f, n; } HeapN;
static HeapN heap[EMAX];
static int hn;

static void hpush(int f, int n) {
    if (hn >= EMAX) return;
    int i = hn++;
    heap[i] = (HeapN){f, n};
    while (i > 0 && heap[(i - 1) / 2].f > heap[i].f) { HeapN t = heap[i]; heap[i] = heap[(i - 1) / 2]; heap[(i - 1) / 2] = t; i = (i - 1) / 2; }
}
static HeapN hpop(void) {
    HeapN top = heap[0];
    heap[0] = heap[--hn];
    int i = 0;
    for (;;) {
        int l = i * 2 + 1, r = l + 1, m = i;
        if (l < hn && heap[l].f < heap[m].f) m = l;
        if (r < hn && heap[r].f < heap[m].f) m = r;
        if (m == i) break;
        HeapN t = heap[i]; heap[i] = heap[m]; heap[m] = t;
        i = m;
    }
    return top;
}

static int heur(int a, int b) {
    int ax = a % LW, ay = a / LW, bx = b % LW, by = b / LW;
    return (iabs(ax - bx) * TS) * 4 / 5 + iabs(ay - by) * TS / 4;
}

/* the route from start to goal as a list of moves; returns its length or -1 */
#define ROUTE_MAX 512
static uint8_t route_prog[ROUTE_MAX];
static int32_t route_node[ROUTE_MAX];
static int route_len;

static int astar(int start, int goal, int max_expand) {
    cur_stamp++;
    hn = 0;
    gcost[start] = 0;
    parent[start] = -1;
    stamp[start] = cur_stamp;
    hpush(heur(start, goal), start);
    int expanded = 0;
    while (hn > 0) {
        HeapN h = hpop();
        int n = h.n;
        if (n == goal) break;
        if (h.f - heur(n, goal) > gcost[n]) continue;
        if (++expanded > max_expand) return -1;
        expand(n);
        for (int j = first[n]; j < first[n] + count[n]; j++) {
            int d = edge[j].dest, g = gcost[n] + edge[j].cost + 4;
            if (stamp[d] != cur_stamp || g < gcost[d]) {
                stamp[d] = cur_stamp;
                gcost[d] = g;
                parent[d] = n;
                parent_prog[d] = edge[j].prog;
                hpush(g + heur(d, goal), d);
            }
        }
    }
    if (stamp[goal] != cur_stamp) return -1;
    int len = 0;
    for (int n = goal; parent[n] >= 0 && len < ROUTE_MAX; n = parent[n]) len++;
    route_len = len;
    int i = len - 1;
    for (int n = goal; parent[n] >= 0 && i >= 0; n = parent[n], i--) { route_prog[i] = parent_prog[n]; route_node[i] = n; }
    return len;
}

/* ---- running a route with real presses ------------------------------------ */
static int goal_x = -1, goal_y = -1, exec_f = -1, exec_prog, center_t, wait_t, fails;
static int exec_total;

void dd_bot_goto(int tx, int ty) {
    goal_x = tx;
    goal_y = ty;
    exec_f = -1;
    dd_bot_state = 1;
    center_t = 0;
    fails = 0;
}
void dd_bot_clear(void) { goal_x = goal_y = -1; dd_bot_state = 0; exec_f = -1; }
int dd_bot_plan_len(void) { return route_len; }

void dd_bot_goto_place(const char *name) {
    int tx, ty;
    if (dd_find_place(name, &tx, &ty)) dd_bot_goto(tx, ty);
    else { dd_bot_state = 3; fprintf(stderr, "bot: no place '%s' here\n", name); }
}

static int here_node(void) {
    int tx = (int)((dd_p.x + dd_p.w / 2) / TS), ty = (int)((dd_p.y + dd_p.h - 1) / TS);
    return ty * LW + tx;
}

uint32_t dd_bot_buttons(void) {
    if (dd_bot_state != 1 || goal_x < 0) return 0;
    if (dd_state != ST_PLAY || dd_trans || dd_dead_t) return 0;
    if (dd_dialog_active()) return 0;
    ensure_graph();
    if (exec_f >= 0) {
        const Prog *p = &prog[exec_prog];
        uint32_t b = prog_buttons(p, exec_f);
        exec_f++;
        if (exec_f >= exec_total + 2 || (exec_f > p->s + p->d && exec_f > p->a && dd_p.ground && exec_f > 3)) exec_f = -1;
        return b;
    }
    if (!dd_p.ground) return 0;
    int here = here_node();
    int goal = goal_y * LW + goal_x;
    if (here == goal) { dd_bot_state = 2; return 0; }
    /* line up on the middle of the tile first, so the practised move fits */
    float cx = dd_p.x + dd_p.w / 2, want = (float)((here % LW) * TS) + (float)TS / 2;
    float dx = want - cx;
    if (fabsf(dx) > 1.5f && center_t < 40) {
        center_t++;
        if (fabsf(dd_p.vx) > 0.5f) return 0;
        return dx > 0 ? BTN_RIGHT : BTN_LEFT;
    }
    if (fabsf(dd_p.vx) > 0.1f && wait_t < 20) { wait_t++; return 0; }
    wait_t = 0;
    center_t = 0;
    int len = astar(here, goal, 60000);
    if (len <= 0) {
        if (++fails > 3) { dd_bot_state = 3; fprintf(stderr, "bot: no route from %d,%d to %d,%d\n", here % LW, here / LW, goal_x, goal_y); }
        return 0;
    }
    exec_prog = route_prog[0];
    exec_f = 0;
    exec_total = 0;
    int dest, frames;
    if (simulate(here % LW, here / LW, &prog[exec_prog], &dest, &frames)) exec_total = frames;
    const Prog *p = &prog[exec_prog];
    uint32_t b = prog_buttons(p, 0);
    exec_f = 1;
    return b;
}

bool dd_bot_can_reach(int fx, int fy, int tx, int ty) {
    ensure_graph();
    if (fx < 0 || fy < 0 || tx < 0 || ty < 0 || fx >= LW || tx >= LW || fy >= LH || ty >= LH) return false;
    return astar(fy * LW + fx, ty * LW + tx, 200000) >= 0;
}

int dd_reach_count(int tx, int ty) {
    ensure_graph();
    (void)tx; (void)ty;
    return n_edge;
}

/* tests: how many moves lead away from the tile Dot stands in */
int dd_bot_moves_here(void) {
    ensure_graph();
    int n = here_node();
    expand(n);
    for (int j = first[n]; j < first[n] + count[n]; j++)
        fprintf(stderr, "  move %d -> %d,%d (%d frames)\n", edge[j].prog, edge[j].dest % LW, edge[j].dest / LW, edge[j].cost);
    return count[n];
}
