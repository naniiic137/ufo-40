/* DOT & DASH - the demo player for the tests. It plans a route over the
 * tiles Dot can stand on, where every step (a walk, a hop, a jump steered
 * for so many frames) is tried with the game's own physics first, then it
 * presses those same buttons for real, one frame at a time. */
#include "dotdash.h"

int dd_bot_state;

/* ---- the moves --------------------------------------------------------- */
typedef struct Prog { int8_t dir; uint8_t a, s, d, up; } Prog;
#define MAXPROG 120
static Prog prog[MAXPROG];
static int n_prog;

static void build_progs(void) {
    n_prog = 0;
    static const uint8_t WALK[] = {4, 8, 14, 22};
    for (int d = -1; d <= 1; d += 2)
        for (int k = 0; k < 4; k++) prog[n_prog++] = (Prog){(int8_t)d, 0, 0, WALK[k], 0};
    static const uint8_t A[] = {5, 12, 40};
    static const uint8_t S[] = {0, 12};
    static const uint8_t D[] = {6, 9, 12, 16, 20, 25, 30, 45, 70};
    for (int up = 0; up <= 1; up++)
        for (int a = 0; a < 3; a++) {
            if (up && a < 2) continue;
            prog[n_prog++] = (Prog){0, A[a], 0, 0, (uint8_t)up};
            for (int d = -1; d <= 1; d += 2)
                for (int s = 0; s < 2; s++)
                    for (int k = 0; k < 9; k++) {
                        if (n_prog >= MAXPROG) break;
                        if (s == 1 && k < 3) continue;
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
static int simulate_body(Body b, const Prog *p, int *dest, int *frames);
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
    return simulate_body(b, p, dest, frames);
}

static int simulate_body(Body b, const Prog *p, int *dest, int *frames) {
    uint32_t prev = 0;
    int f;
    bool start_bad = hazard(&b); /* already in goo: moves may climb out */
    for (f = 0; f < 160; f++) {
        uint32_t in = prog_buttons(p, f);
        dd_body_step(&b, in, prev, &dd_lv, PH, false, BEAN);
        prev = in;
        if (hazard(&b)) { if (!start_bad) return 0; }
        else start_bad = false;
        if (b.y > LH * TS) return 0;
        bool active = (int)f < (int)p->s + p->d || f < p->a;
        if (!active && b.ground && fabsf(b.vx) < 0.05f && f > 2) break;
    }
    if (f >= 160 || !b.ground || hazard(&b)) return 0;
    if (TS == DD_TS && b.landed_fall > 12 * DD_TS && !dd_has(U_FEATHER)) return 0; /* would hurt */
    int nx = (int)((b.x + b.w / 2) / TS), ny = (int)((b.y + b.h - 1) / TS);
    if (!standable(nx, ny)) {
        /* settled on the edge of a ledge: the node is the tile holding her */
        int l = (int)((b.x + 0.5f) / TS), r = (int)((b.x + b.w - 0.5f) / TS);
        if (l != nx && standable(l, ny)) nx = l;
        else if (r != nx && standable(r, ny)) nx = r;
        else return 0; /* standing on nothing we know (a creature, a thing) */
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
static int hunt_sub = -1, hunt_t;
static uint32_t hunt_prev;
static int exec_total;

void dd_bot_goto(int tx, int ty) {
    goal_x = tx;
    goal_y = ty;
    exec_f = -1;
    dd_bot_state = 1;
    center_t = 0;
    fails = 0;
}
void dd_bot_clear(void) { goal_x = goal_y = -1; dd_bot_state = 0; exec_f = -1; hunt_sub = -1; }
int dd_bot_plan_len(void) { return route_len; }

void dd_bot_goto_place(const char *name) {
    int tx, ty;
    if (dd_find_place(name, &tx, &ty)) {
        /* something floating: stand on the ground beneath it */
        ensure_graph();
        for (int k = 0; k < 10 && !standable(tx, ty) && ty + 1 < LH; k++) ty++;
        dd_bot_goto(tx, ty);
    }
    else { dd_bot_state = 3; fprintf(stderr, "bot: no place '%s' here\n", name); }
}

static int here_node(void) {
    int tx = (int)((dd_p.x + dd_p.w / 2) / TS), ty = (int)((dd_p.y + dd_p.h - 1) / TS);
    /* standing on the very edge of a ledge: count the tile that holds her */
    if (!standable(tx, ty)) {
        int l = (int)((dd_p.x + 0.5f) / TS), r = (int)((dd_p.x + dd_p.w - 0.5f) / TS);
        if (l != tx && standable(l, ty)) tx = l;
        else if (r != tx && standable(r, ty)) tx = r;
    }
    return ty * LW + tx;
}

/* hunting: walk into line with the nearest creature of a kind and shoot it
 * with the popgun Dot carries */
void dd_bot_hunt(int sub) { hunt_sub = sub; dd_bot_state = 1; exec_f = -1; hunt_t = 0; fails = 0; goal_x = goal_y = -1; }

static uint32_t plan_step(void);

static uint32_t hunt_buttons(void) {
    int best = -1;
    float bd = 1e9f, pcx = dd_p.x + dd_p.w / 2, pb = dd_p.y + dd_p.h;
    for (int i = 0; i < DD_MAX_ENTS; i++) {
        const Ent *e = &dd_ent[i];
        if (!e->alive || e->kind != EK_FOE || e->sub != hunt_sub || e->held) continue;
        float d = fabsf(e->x + e->w / 2 - pcx) + fabsf(e->y + e->h - pb) * 2;
        if (d < bd) { bd = d; best = i; }
    }
    if (best < 0) { hunt_sub = -1; dd_bot_state = 2; return 0; }
    if (dd_carry < 0 || dd_ent[dd_carry].kind != EK_OBJ || dd_ent[dd_carry].sub != O_POPGUN) { dd_bot_state = 3; return 0; }
    const Ent *e = &dd_ent[best];
    float dx = e->x + e->w / 2 - pcx;
    hunt_t++;
    bool clear = true;
    for (float t = 0; t < fabsf(dx); t += 3)
        if (dd_body_blocked(&dd_lv, TS, pcx + (dx > 0 ? t : -t), dd_p.y + 5, 1, 4)) { clear = false; break; }
    /* the pea flies at chest height: it must meet the creature's body */
    float pea_top = dd_p.y + 5, pea_bot = pea_top + 4;
    bool level = pea_bot > e->y && pea_top < e->y + e->h;
    if (exec_f < 0 && dd_p.ground && clear && level && fabsf(dx) < 100 && fabsf(dx) > 2) {
        bool facing_ok = (dx > 0) == (dd_p.facing != 0);
        uint32_t b = 0;
        if (!facing_ok) b = dx > 0 ? BTN_RIGHT : BTN_LEFT;
        else if (!(hunt_prev & BTN_B)) b = BTN_B;
        hunt_prev = b;
        return b;
    }
    hunt_prev = 0;
    /* walk to a tile on its level, a few tiles off */
    if (exec_f < 0) {
        int ftx = (int)((e->x + e->w / 2) / TS), fty = (int)((e->y + e->h - 1) / TS);
        int side = dx > 0 ? -1 : 1;
        if (!clear) side = -side;
        goal_x = -1;
        for (int k = 4; k <= 9 && goal_x < 0; k++)
            for (int sgn = 0; sgn < 2 && goal_x < 0; sgn++) {
                int gx = ftx + (sgn ? -side : side) * k;
                if (standable(gx, fty)) { goal_x = gx; goal_y = fty; }
            }
        if (goal_x < 0) { goal_x = ftx; goal_y = fty; }
    }
    return plan_step();
}

/* talking: press A through the pages (a question is left for the test) */
static int talk_mode, talk_t;
void dd_bot_talk(void) { talk_mode = 1; talk_t = 0; dd_bot_state = 1; }

uint32_t dd_bot_buttons(void) {
    if (talk_mode) {
        extern bool dd_dialog_asking(void);
        if (!dd_dialog_active() || dd_dialog_asking()) {
            /* stop on a frame with A let go, so the next press counts */
            if (talk_t % 4 == 1) { talk_t++; return 0; }
            talk_mode = 0;
            dd_bot_state = 2;
            return 0;
        }
        return (++talk_t % 4) == 1 ? BTN_A : 0;
    }
    if (hunt_sub >= 0 && dd_bot_state == 1) {
        if (dd_state != ST_PLAY || dd_trans || dd_dead_t || dd_dialog_active()) return 0;
        ensure_graph();
        return hunt_buttons();
    }
    if (dd_bot_state != 1 || goal_x < 0) return 0;
    return plan_step();
}

static uint32_t plan_step(void) {
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
    /* line up on the middle of the tile first, so the practised move fits
     * (and so that arriving means standing right in the goal tile) */
    float cx = dd_p.x + dd_p.w / 2, want = (float)((here % LW) * TS) + (float)TS / 2;
    float dx = want - cx;
    if (fabsf(dx) > 0.6f && center_t < 60) {
        center_t++;
        if (fabsf(dd_p.vx) > 0.05f) return 0;   /* let the last tap settle */
        return fabsf(dx) > 3.0f || (center_t % 3) == 0 ? (dx > 0 ? BTN_RIGHT : BTN_LEFT) : 0;
    }
    /* arrived: in the goal tile, or standing on something in it */
    if (here == goal || (here % LW == goal_x && here / LW == goal_y - 1)) { center_t = 0; if (hunt_sub < 0) dd_bot_state = 2; return 0; }
    if (fabsf(dd_p.vx) > 0.1f && wait_t < 20) { wait_t++; return 0; }
    wait_t = 0;
    center_t = 0;
    int len = astar(here, goal, 60000);
    if (len <= 0) {
        if (++fails > 3 && hunt_sub < 0) { dd_bot_state = 3; fprintf(stderr, "bot: no route from %d,%d to %d,%d\n", here % LW, here / LW, goal_x, goal_y); }
        return 0;
    }
    exec_prog = route_prog[0];
    exec_f = 0;
    exec_total = 0;
    int dest, frames;
    /* try the move from where Dot really stands; if it wouldn't land where
     * planned, find one that does from here */
    Body real = dd_p;
    real.jumping = 0;
    if (!simulate_body(real, &prog[exec_prog], &dest, &frames) || dest != route_node[0]) {
        for (int k = 0; k < n_prog; k++) {
            if (prog[k].up && !BEAN) continue;
            int d2, f2;
            if (simulate_body(real, &prog[k], &d2, &f2) && d2 == route_node[0]) { exec_prog = k; frames = f2; dest = d2; break; }
        }
    }
    exec_total = frames;
    const Prog *p = &prog[exec_prog];
    uint32_t b = prog_buttons(p, 0);
    exec_f = 1;
    return b;
}

/* walk to the nearest spot clear of anyone to talk to, doors and stands, so
 * that holding UP there grows Dot instead of starting a conversation */
static bool spot_clear(int tx, int ty) {
    if (!standable(tx, ty)) return false;
    float x = node_x(tx), y = node_y(ty);
    for (int i = 0; i < DD_MAX_ENTS; i++) {
        const Ent *e = &dd_ent[i];
        if (!e->alive || (e->kind != EK_NPC && e->kind != EK_DOOR && e->kind != EK_STAND)) continue;
        if (rects_overlap((int)x, (int)y, BW, BH, (int)e->x, (int)e->y, e->w, e->h)) return false;
    }
    return true;
}
void dd_bot_free(void) {
    ensure_graph();
    int tx = (int)((dd_p.x + dd_p.w / 2) / TS), ty = (int)((dd_p.y + dd_p.h - 1) / TS);
    for (int d = 0; d < 30; d++)
        for (int sgn = -1; sgn <= 1; sgn += 2)
            for (int dy = 0; dy <= 2; dy++) {
                int x = tx + sgn * d, y = ty + (dy == 2 ? -1 : dy);
                if (spot_clear(x, y)) { dd_bot_goto(x, y); return; }
            }
    dd_bot_state = 2;
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

/* ---- proving a level: flood every tile reachable from where Dot stands,
 * then check every treasure, thing and person in it can be reached ---- */
static uint8_t reach_mark[NMAX];
static int32_t queue_[NMAX];

static int flood_from_here(void) {
    ensure_graph();
    memset(reach_mark, 0, (size_t)(LW * LH));
    int start = here_node(), qh = 0, qt = 0, n = 0;
    queue_[qt++] = start;
    reach_mark[start] = 1;
    while (qh < qt) {
        int nd = queue_[qh++];
        n++;
        expand(nd);
        for (int j = first[nd]; j < first[nd] + count[nd]; j++) {
            int d = edge[j].dest;
            if (!reach_mark[d]) { reach_mark[d] = 1; if (qt < NMAX) queue_[qt++] = d; }
        }
    }
    return n;
}

static bool node_reached(int tx, int ty) {
    /* the tile, or standing on something in it, or the ground just below */
    for (int dy = -1; dy <= 3; dy++) {
        int y = ty + dy;
        if (tx >= 0 && y >= 0 && tx < LW && y < LH && reach_mark[y * LW + tx]) return true;
    }
    return false;
}

/* how many things in this level can't be reached from Dot (and says which) */
int dd_bot_unreachable(void) {
    flood_from_here();
    int bad = 0;
    for (int i = 0; i < DD_MAX_ENTS; i++) {
        const Ent *e = &dd_ent[i];
        if (!e->alive || e->held) continue;
        if (e->kind != EK_PICK && e->kind != EK_OBJ && e->kind != EK_NPC && e->kind != EK_DOOR && e->kind != EK_STAND) continue;
        int tx = (int)((e->x + e->w / 2) / TS), ty = (int)((e->y + e->h - 1) / TS);
        bool ok = node_reached(tx, ty) || node_reached(tx - 1, ty) || node_reached(tx + 1, ty);
        if (!ok) { bad++; fprintf(stderr, "  unreachable: kind %d sub %d at %d,%d\n", e->kind, e->sub, tx, ty); }
    }
    /* the first ground from the top, across hand-made places */
    if (dd_lv.d.kind != LV_STRIP)
        for (int x = 2; x < LW - 2; x += 5)
            for (int y = 1; y < LH - 1; y++)
                if (standable(x, y)) {
                    if (DD_TILE[lv_tile(&dd_lv, x, y)].flags & (TF_HURT | TF_GOO)) break; /* a pit of thorns */
                    if (!node_reached(x, y) && !node_reached(x + 1, y) && !node_reached(x - 1, y)) { bad++; fprintf(stderr, "  unreachable ground at %d,%d\n", x, y); }
                    break;
                }
    /* and the ground of every chunk of a strip */
    if (dd_lv.d.kind == LV_STRIP)
        for (int c = 0; c < dd_lv.d.n; c++)
            for (int x = c * CHUNK_W + 2; x < (c + 1) * CHUNK_W - 2; x += 6) {
                int y = dd_lv.surf[x] - 1;
                if (!standable(x, y)) continue;
                if (!node_reached(x, y) && !node_reached(x + 1, y) && !node_reached(x - 1, y)) { bad++; fprintf(stderr, "  unreachable ground at %d,%d\n", x, y); break; }
            }
    return bad;
}
