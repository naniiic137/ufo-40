/* KIPPER - the demo player. It presses the buttons a player would, from the
 * title to the credits, and knows the map like someone who has played it.
 *
 * At a dock it plans the next dive: the nearest things it can reach and get
 * home from with the fuel it has (with a quarter spare and room for
 * knocks), up to three, or a blast to open a sealed way (a rocket up into a
 * floor of blue blocks, a sinker bomb down onto one), or Auntie Roe when it
 * tows eggs, or a fight with a big creature once nothing else is in reach.
 * It fits what the dive needs (the lamp in the Blackwater, the auger for
 * dirt, the ram torpedoes for a switch) and fills the other slots with what
 * helps most. Out in the sea it follows a distance map toward each target,
 * taps the fire button at anything in front of it, dashes through dirt,
 * rubs cracked walls open, lights switches, goes back for anything knocked
 * or stolen from its trail, and turns for home as soon as the fuel left
 * would only just get it there. */
#include "kpr.h"

int kpr_bot_goal, kpr_bot_log;

#define FBIG 0xFFFF
typedef uint16_t KprField[KPR_H][KPR_W];

/* caps: what the bot counts as passable */
enum { CP_DRILL = 1, CP_BUSTER = 2, CP_SPOT = 4, CP_ALL = 7, CP_OPEN = 8 /* reach check: blasts open */ };

static KprField f_dock[8][KPR_NDOCKS];
static uint8_t f_dock_ok[8];
static uint32_t f_dock_key;           /* what the cached dock fields were made for */
static KprField f_goal, f_tmp;
static int goal_x = -1, goal_y = -1, goal_caps = -1;
static uint32_t goal_key;

/* ---- distance maps ---------------------------------------------------------- */

static bool pass(int x, int y, int caps) {
    if (x < 0 || y < KPR_SKY || x >= KPR_W || y >= KPR_H) return false;
    int c = kg.tile[y][x];
    if ((caps & CP_OPEN) && (c == 'B' || c == 'X')) return true;
    if (y >= KPR_AREA3 && !(caps & CP_SPOT)) return false;
    switch (c) {
    case '.': case 'k': case 'c': case '?': return true;
    case 'd': return (caps & CP_DRILL) != 0;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
        return (caps & CP_BUSTER) || ((kg.gates >> (c - '0')) & 1);
    default: return false;
    }
}

/* Dijkstra over tiles, 10 a step and 14 a diagonal (no cutting corners),
 * from one tile; cracked walls cost a little more (they take rubbing) */
static int heap_n;
static int32_t heap[KPR_W * KPR_H * 8];
static void heap_push(int32_t v) {
    int i = heap_n++;
    heap[i] = v;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (heap[p] <= heap[i]) break;
        int32_t t = heap[p]; heap[p] = heap[i]; heap[i] = t;
        i = p;
    }
}
static int32_t heap_pop(void) {
    int32_t top = heap[0];
    heap[0] = heap[--heap_n];
    int i = 0;
    for (;;) {
        int l = i * 2 + 1, r = l + 1, m = i;
        if (l < heap_n && heap[l] < heap[m]) m = l;
        if (r < heap_n && heap[r] < heap[m]) m = r;
        if (m == i) break;
        int32_t t = heap[m]; heap[m] = heap[i]; heap[i] = t;
        i = m;
    }
    return top;
}

static void field_from(KprField f, int sx0, int sy0, int caps) {
    memset(f, 0xFF, sizeof(KprField));
    if (sx0 < 0 || sy0 < 0 || sx0 >= KPR_W || sy0 >= KPR_H) return;
    heap_n = 0;
    f[sy0][sx0] = 0;
    heap_push(sy0 * KPR_W + sx0);
    static const int8_t D[8][3] = {{1, 0, 10}, {-1, 0, 10}, {0, 1, 10}, {0, -1, 10}, {1, 1, 14}, {1, -1, 14}, {-1, 1, 14}, {-1, -1, 14}};
    while (heap_n > 0) {
        int32_t v = heap_pop();
        int pos = v & 0x7FFF, d = v >> 15;
        int x = pos % KPR_W, y = pos / KPR_W;
        if (d > f[y][x]) continue;
        for (int k = 0; k < 8; k++) {
            int nx = x + D[k][0], ny = y + D[k][1];
            if (!pass(nx, ny, caps)) continue;
            if (D[k][0] && D[k][1] && (!pass(x + D[k][0], y, caps) || !pass(x, y + D[k][1], caps))) continue;
            int nd = d + D[k][2] + (kg.tile[ny][nx] == '?' ? 40 : 0);
            if (nd < f[ny][nx] && nd < FBIG && heap_n < KPR_W * KPR_H * 8) {
                f[ny][nx] = (uint16_t)nd;
                heap_push((int32_t)(nd << 15) | (ny * KPR_W + nx));
            }
        }
    }
}

/* everything the bot knows that changes the map: blasts banked, dirt, gates */
static uint32_t sea_key(void) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < (int)sizeof kg.cur.broken; i++) h = (h ^ kg.cur.broken[i]) * 16777619u;
    return h;
}

static uint16_t dock_dist(int caps, int d, int x, int y) {
    uint32_t key = sea_key();
    if (key != f_dock_key) { memset(f_dock_ok, 0, sizeof f_dock_ok); f_dock_key = key; }
    if (!f_dock_ok[caps]) {
        for (int k = 0; k < KPR_NDOCKS; k++) field_from(f_dock[caps][k], KPR_DOCKS[k].x, KPR_SKY, caps);
        f_dock_ok[caps] = 1;
    }
    return f_dock[caps][d][y][x];
}

/* the nearest known dock from a tile */
static int home_dist(int caps, int x, int y, int *which) {
    int best = FBIG, w = kg.cur.dock;
    for (int d = 0; d < KPR_NDOCKS; d++) {
        int v = dock_dist(caps, d, x, y);
        if (v < best) { best = v; w = d; }
    }
    if (which) *which = w;
    return best;
}

static void set_goal(int x, int y, int caps) {
    uint32_t key = sea_key() ^ kg.gates;
    if (x == goal_x && y == goal_y && caps == goal_caps && key == goal_key) return;
    goal_x = x;
    goal_y = y;
    goal_caps = caps;
    goal_key = key;
    field_from(f_goal, x, y, caps);
}

/* ---- the sealed ways ---------------------------------------------------------- */

/* up: a rocket from right under it (at); down: a bomb, sitting still at at
 * after swimming in from ap (so the bomb trailing behind drops beside the
 * sub, not on top of it), then back off to re */
typedef struct { int16_t x0, y0, x1, y1; int8_t up; int16_t at_x, at_y, ap_x, ap_y, re_x, re_y; } Plug;
static const Plug PLUGS[] = {
    {35, 48, 37, 49, 1, 36, 51, 0, 0, 0, 0},              /* the Auger's floor */
    {114, 48, 116, 49, 1, 115, 51, 0, 0, 0, 0},           /* the Sinker Bombs' floor */
    {99, 87, 101, 89, 1, 100, 91, 0, 0, 0, 0},            /* the Reliquary (the Lamp) */
    {132, 98, 134, 100, 0, 133, 96, 137, 96, 128, 93},    /* the Blackwater's blue floor */
    {78, 125, 81, 126, 0, 78, 123, 81, 123, 78, 117},     /* over Xolloth */
    {48, 98, 50, 98, 0, 49, 96, 52, 96, 45, 93},          /* sealed niche in the Undercroft */
    {31, 33, 33, 33, 0, 32, 31, 35, 31, 28, 28},          /* sealed nook under the reef */
};
#define NPLUGS ARRAY_LEN(PLUGS)

static bool plug_open(int p) {
    const Plug *q = &PLUGS[p];
    for (int y = q->y0; y <= q->y1; y++)
        for (int x = q->x0; x <= q->x1; x++) {
            int c = kg.tile[y][x];
            if (c == 'B' || c == 'X') return false;
        }
    return true;
}
static bool plug_banked(int p) {
    const Plug *q = &PLUGS[p];
    for (int y = q->y0; y <= q->y1; y++)
        for (int x = q->x0; x <= q->x1; x++) {
            int b = kpr_brk_index(x, y);
            if (b >= 0 && !kpr_brk_banked(b)) return false;
        }
    return true;
}

/* gates: where to stand to light each switch (facing it) */
typedef struct { int8_t gate; int16_t at_x, at_y; int8_t face; } SwitchSpot;
static const SwitchSpot SWITCHES[] = {{0, 47, 59, -1}, {1, 75, 128, -1}};

/* ---- the plan ---------------------------------------------------------------- */

enum { T_NONE, T_ITEM, T_ROE, T_PLUG, T_BOSS, T_HOME, T_LOOSE };
typedef struct { int kind, id; } Task;

static Task plan[6];
static int nplan, plan_i;
static int dive_caps;                  /* passability this dive */
static uint16_t want_on;               /* the fitting wanted */
static int mode;                       /* 0 at a dock, 1 out */
static int last_state = -1, state_frames, release_t;
static int stuck_t, last_tx, last_ty, plug_t, fail_dive[KPR_NITEMS + 8];
static int want_dir;                   /* the d-pad the path wants */
static int force_dock = -1;            /* a dive to move to the other dock */
static int switch_pending = -1;        /* a gate in the way: its switch first */
static int fight_frames;
static char why = '-';
static int chase_t, unstick_n, dive_frames, idle_plans;

static int caps_of(uint16_t on) {
    int c = 0;
    if ((on >> EQ_DRILL) & 1) c |= CP_DRILL;
    if ((on >> EQ_BUSTER) & 1) c |= CP_BUSTER;
    if ((on >> EQ_SPOT) & 1) c |= CP_SPOT;
    return c;
}
static uint16_t equip_of(int caps) {
    uint16_t e = 0;
    if (caps & CP_DRILL) e |= 1 << EQ_DRILL;
    if (caps & CP_BUSTER) e |= 1 << EQ_BUSTER;
    if (caps & CP_SPOT) e |= 1 << EQ_SPOT;
    return e;
}
static int popc(unsigned v) { int n = 0; for (; v; v &= v - 1) n++; return n; }

/* the least it must fit to reach (x, y) from dock d, or -1 */
static int need_caps(int d, int x, int y, int *dist) {
    static const int ORDER[8] = {0, CP_SPOT, CP_BUSTER, CP_DRILL, CP_BUSTER | CP_SPOT, CP_DRILL | CP_SPOT,
                                 CP_DRILL | CP_BUSTER, CP_ALL};
    int have = caps_of(kg.cur.have), slots = kpr_slots();
    int best = -1, bscore = FBIG * 2;
    for (int k = 0; k < 8; k++) {
        int c = ORDER[k];
        if ((c & have) != c || popc(c) > slots) continue;
        uint16_t v = dock_dist(c, d, x, y);
        if (v == FBIG) continue;
        int score = v + popc(c) * 150;       /* a slot spent is worth 15 tiles */
        if (score < bscore) { bscore = score; best = c; *dist = v; }
    }
    return best;
}

/* fuel for a route of `tiles10` (tenths of a tile), with what's fitted */
static int route_fuel(int tiles10, uint16_t on) {
    int u = tiles10 * 16 / 10 * 5 / 4;             /* a quarter spare */
    if ((on >> EQ_EFFICIENT) & 1) u = u * 3 / 4;
    return u + 250;                                /* room for knocks */
}

static uint16_t fill_slots(uint16_t on) {
    static const int PREF[] = {EQ_EFFICIENT, EQ_BOOSTER, EQ_ARMOR, EQ_TARGET, EQ_RADAR, EQ_MISSILE, EQ_CHARGE};
    for (int k = 0; k < ARRAY_LEN(PREF) && popc(on) < kpr_slots(); k++)
        if ((kg.cur.have >> PREF[k]) & 1) on |= (uint16_t)(1 << PREF[k]);
    return on;
}

static int trail_eggs(void) {
    int n = 0;
    for (int s = 0; s < kg.trail_n; s++) n += kg.trail[s].kind == TR_ITEM && KPR_ITEMS[kg.trail[s].id].kind == IK_EGG;
    return n;
}

static void log_plan(const char *why) {
    if (!kpr_bot_log) return;
    printf("[bot %u] dive %d from dock %d, fuel %d bars, on %03x, %s:", (unsigned)kg.cur.frames, kg.dives, kg.cur.dock,
           kpr_bars_max(), want_on, why);
    for (int i = 0; i < nplan; i++) printf(" %d/%d", plan[i].kind, plan[i].id);
    printf("\n");
}

static int boss_ready_level(int b) {
    static const int L[KPR_BOSSES] = {5, 6, 9, 12, 14, 16};
    return L[b];
}

/* eggs are worth towing only when Auntie Roe is in reach of a dive */
static bool roe_in_reach(void) {
    int dist, c = need_caps(kg.cur.dock, KPR_ROE.x, KPR_ROE.y, &dist);
    if (c < 0) return false;
    int back = home_dist(c, KPR_ROE.x, KPR_ROE.y, NULL);
    return route_fuel(dist + back, fill_slots(equip_of(c))) <= kpr_bars_max() * KPR_BAR;
}

static void make_plan(void) {
    int d = kg.cur.dock;
    force_dock = -1;
    /* more fuel or new equipment: what failed before is worth a new try */
    static int last_bars = -1, last_have = -1;
    if (kpr_bars_max() != last_bars || kg.cur.have != last_have) {
        memset(fail_dive, 0, sizeof fail_dive);
        last_bars = kpr_bars_max();
        last_have = kg.cur.have;
    }
    int fuel = kpr_bars_max() * KPR_BAR;
    nplan = plan_i = 0;
    want_on = 0;
    /* eggs in tow go to Auntie Roe first */
    if (trail_eggs() > 0) {
        int dist, c = need_caps(d, KPR_ROE.x, KPR_ROE.y, &dist);
        if (kpr_bot_log) printf("[bot] eggs %d: roe caps %d dist %d need %d of %d\n", trail_eggs(), c, dist,
                                c >= 0 ? route_fuel(dist * 2, fill_slots(equip_of(c))) : -1, fuel);
        int back = c >= 0 ? home_dist(c, KPR_ROE.x, KPR_ROE.y, NULL) : FBIG;
        if (c >= 0 && back != FBIG && route_fuel(dist + back, fill_slots(equip_of(c))) <= fuel) {
            want_on = fill_slots(equip_of(c));
            plan[nplan++] = (Task){T_ROE, 0};
            /* and anything handy on the way */
        }
    }
    /* a sealed way it can open now */
    if (!nplan) {
        for (int p = 0; p < NPLUGS && !nplan; p++) {
            if (plug_banked(p)) continue;
            /* the Blackwater's ways only once there is a lamp to see by; the
             * last one only when Xolloth is all that is left */
            if ((p == 3 || p == 4) && !((kg.cur.have >> EQ_SPOT) & 1)) continue;
            const Plug *q = &PLUGS[p];
            int weapon = q->up ? EQ_MISSILE : EQ_CHARGE;
            if (!((kg.cur.have >> weapon) & 1)) continue;
            int dist, c = need_caps(d, q->at_x, q->at_y, &dist);
            if (c < 0 || popc(c) + 1 > kpr_slots()) continue;
            uint16_t on = fill_slots((uint16_t)(equip_of(c) | (1 << weapon)));
            if (route_fuel(dist * 2 + 200, on) > fuel) continue;
            want_on = on;
            plan[nplan++] = (Task){T_PLUG, p};
        }
    }
    /* things to tow home: the nearest one in reach, then the nearest to that */
    if (!nplan) {
        int best = -1, bd = FBIG, bc = 0;
        for (int i = 0; i < KPR_NITEMS; i++) {
            if (kg.cur.item[i] != IS_HOME || fail_dive[i] >= 4) continue;
            if (KPR_ITEMS[i].kind == IK_EGG && !roe_in_reach()) continue;
            int dist, c = need_caps(d, KPR_ITEMS[i].x, KPR_ITEMS[i].y, &dist);
            if (c < 0) continue;
            int x = KPR_ITEMS[i].x, y = KPR_ITEMS[i].y;
            int back = home_dist(c, x, y, NULL);
            if (back == FBIG) continue;
            uint16_t on = fill_slots(equip_of(c));
            if (route_fuel(dist + back, on) > fuel) continue;
            int score = dist + fail_dive[i] * 300;
            if (score < bd) { bd = score; best = i; bc = c; }
        }
        if (best >= 0) {
            plan[nplan++] = (Task){T_ITEM, best};
            int caps = bc, room = KPR_TRAIL - kg.trail_n;
            int spent = dock_dist(caps, d, KPR_ITEMS[best].x, KPR_ITEMS[best].y);
            int last = best;
            while (nplan < room) {
                field_from(f_tmp, KPR_ITEMS[last].x, KPR_ITEMS[last].y, CP_ALL & caps_of(kg.cur.have));
                int nb = -1, nd = FBIG, ncaps = caps;
                for (int i = 0; i < KPR_NITEMS; i++) {
                    if (kg.cur.item[i] != IS_HOME || fail_dive[i] >= 4) continue;
                    bool dup = false;
                    for (int k = 0; k < nplan; k++) dup |= plan[k].id == i;
                    if (dup) continue;
                    if (KPR_ITEMS[i].kind == IK_EGG && !roe_in_reach()) continue;
                    int x = KPR_ITEMS[i].x, y = KPR_ITEMS[i].y;
                    int dd = f_tmp[y][x];
                    if (dd == FBIG || dd > 400) continue;
                    int dist2, c2 = need_caps(d, x, y, &dist2);
                    if (c2 < 0) continue;
                    int cc = caps | c2;
                    if (popc(cc) > kpr_slots()) continue;
                    int back = home_dist(cc, x, y, NULL);
                    if (back == FBIG) continue;
                    if (route_fuel(spent + dd + back, fill_slots(equip_of(cc))) > fuel) continue;
                    if (dd < nd) { nd = dd; nb = i; ncaps = cc; }
                }
                if (nb < 0) break;
                plan[nplan++] = (Task){T_ITEM, nb};
                spent += nd;
                caps = ncaps;
                last = nb;
            }
            want_on = fill_slots(equip_of(caps));
        }
    }
    /* a fight, once nothing else is in reach (or the creature stands in
     * the way of the slots it needs) */
    if (!nplan) {
        for (int b = 0; b < BS_XOLLOTH && !nplan; b++) {
            if (kg.cur.boss_dead[b] || kg.cur.boosters < boss_ready_level(b)) continue;
            int bx = KPR_ROUTES[b].pts[0].x, by = KPR_ROUTES[b].pts[0].y;
            int dist, c = need_caps(d, bx, by, &dist);
            if (c < 0) continue;
            want_on = fill_slots(equip_of(c));
            plan[nplan++] = (Task){T_BOSS, b};
        }
    }
    /* Xolloth, with everything done (or nothing else left in reach for a
     * good while) */
    if (!nplan && kpr_parts_done() < 75) idle_plans++;
    if (!nplan && kpr_bot_log && kpr_parts_done() < 75 && idle_plans == 1)
        for (int i = 0; i < KPR_NITEMS; i++)
            if (kg.cur.item[i] != IS_CARRIED && kg.cur.item[i] != IS_TAKEN) {
                int dist, c = need_caps(d, KPR_ITEMS[i].x, KPR_ITEMS[i].y, &dist);
                printf("[bot] not done: item %d at %d,%d caps %d dist %d fail %d\n", i, KPR_ITEMS[i].x, KPR_ITEMS[i].y, c,
                       dist, fail_dive[i]);
            }
    if (!nplan && !kg.cur.boss_dead[BS_XOLLOTH] && (kpr_parts_done() >= 75 || idle_plans > 12)) {
        int dist, c = need_caps(d, 80, 135, &dist);
        if (c >= 0) {
            want_on = fill_slots(equip_of(c));
            plan[nplan++] = (Task){T_BOSS, BS_XOLLOTH};
        }
    }
    if (!nplan && kpr_bot_log && !(kg.dives % 200)) {
        for (int i = 0; i < KPR_NITEMS; i++) {
            if (kg.cur.item[i] != IS_HOME) continue;
            int dist, c = need_caps(d, KPR_ITEMS[i].x, KPR_ITEMS[i].y, &dist);
            int back = c >= 0 ? home_dist(c, KPR_ITEMS[i].x, KPR_ITEMS[i].y, NULL) : -1;
            printf("[bot]   left %d at %d,%d: caps %d dist %d back %d need %d fail %d\n", i, KPR_ITEMS[i].x,
                   KPR_ITEMS[i].y, c, dist, back, c >= 0 ? route_fuel(dist + back, fill_slots(equip_of(c))) : -1,
                   fail_dive[i]);
        }
    }
    if (!nplan) {
        /* nothing in reach from here: move to the other dock if more is in
         * reach from there */
        want_on = fill_slots(0);
        int o = 1 - d, n_there = 0;
        for (int i = 0; i < KPR_NITEMS && !n_there; i++) {
            if (kg.cur.item[i] != IS_HOME) continue;
            int dist, c = need_caps(o, KPR_ITEMS[i].x, KPR_ITEMS[i].y, &dist);
            if (c < 0) continue;
            int back = home_dist(c, KPR_ITEMS[i].x, KPR_ITEMS[i].y, NULL);
            if (back != FBIG && route_fuel(dist + back, fill_slots(equip_of(c))) <= fuel) n_there++;
        }
        for (int p = 0; p < NPLUGS && !n_there; p++) {
            if (plug_banked(p) || ((p == 3 || p == 4) && !((kg.cur.have >> EQ_SPOT) & 1))) continue;
            int weapon = PLUGS[p].up ? EQ_MISSILE : EQ_CHARGE;
            if (!((kg.cur.have >> weapon) & 1)) continue;
            int dist, c = need_caps(o, PLUGS[p].at_x, PLUGS[p].at_y, &dist);
            if (c >= 0 && route_fuel(dist * 2 + 200, fill_slots((uint16_t)(equip_of(c) | (1 << weapon)))) <= fuel) n_there++;
        }
        int dist_o = FBIG, c_o = need_caps(d, KPR_DOCKS[o].x, KPR_SKY, &dist_o);
        if (kpr_bot_log && !(kg.dives % 200)) printf("[bot] other dock %d: %d things, caps %d dist %d\n", o, n_there, c_o, dist_o);
        if (n_there && c_o >= 0 && route_fuel(dist_o, fill_slots(equip_of(c_o))) <= fuel) {
            want_on = fill_slots(equip_of(c_o));
            force_dock = o;
        }
        plan[nplan++] = (Task){T_HOME, 0};
    }
    dive_caps = caps_of(want_on);
    log_plan("plan");
}

/* ---- steering ------------------------------------------------------------------ */

static int tile_x(void) { return (int)(kg.x / KPR_TILE); }
static int tile_y(void) { return (int)(kg.y / KPR_TILE); }

/* the d-pad toward the downhill neighbour of the goal field; dashes into
 * dirt, pushes into cracked walls */
static int downhill(KprField f, int tx, int ty, int *nx_out, int *ny_out) {
    int best = f[ty][tx], bx = tx, by = ty;
    static const int8_t D[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (int k = 0; k < 8; k++) {
        int nx = tx + D[k][0], ny = ty + D[k][1];
        if (nx < 0 || ny < 0 || nx >= KPR_W || ny >= KPR_H) continue;
        if (f[ny][nx] < best) {
            if (D[k][0] && D[k][1] && (f[ty][nx] == FBIG || f[ny][tx] == FBIG)) continue;
            best = f[ny][nx];
            bx = nx;
            by = ny;
        }
    }
    *nx_out = bx;
    *ny_out = by;
    return best;
}

/* the sub's box can swim straight from here to (x, y) */
static bool box_line(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    int n = (int)(d / 4) + 1;
    for (int i = 1; i <= n; i++)
        if (kpr_box_solid(x0 + dx * i / n, y0 + dy * i / n, KPR_SW + 2, KPR_SH + 2)) return false;
    return true;
}

/* the d-pad toward a point a few tiles down the goal field that the sub
 * can swim to in a straight line (less zigzag than tile by tile) */
static int steer(KprField f, int *next_tile) {
    int tx = tile_x(), ty = tile_y();
    int bx, by;
    downhill(f, tx, ty, &bx, &by);
    *next_tile = kpr_tile(bx, by);
    want_dir = (bx > tx ? BTN_RIGHT : bx < tx ? BTN_LEFT : 0) | (by > ty ? BTN_DOWN : by < ty ? BTN_UP : 0);
    float gx = bx * KPR_TILE + 8.0f, gy = by * KPR_TILE + 8.0f;
    bool plain = (*next_tile == '.' || *next_tile == 'k' || *next_tile == 'c');
    if (plain) {
        int cx = bx, cy = by;
        for (int k = 0; k < 6; k++) {
            int nx, ny;
            downhill(f, cx, cy, &nx, &ny);
            if (nx == cx && ny == cy) break;
            int c = kpr_tile(nx, ny);
            if (c != '.' && c != 'k' && c != 'c') break;
            float px = nx * KPR_TILE + 8.0f, py = ny * KPR_TILE + 8.0f;
            if (!box_line(kg.x, kg.y, px, py)) break;
            gx = px;
            gy = py;
            cx = nx;
            cy = ny;
        }
    }
    float ddx = gx - kg.x, ddy = gy - kg.y;
    int m = 0;
    float ax = fabsf(ddx), ay = fabsf(ddy);
    /* eight ways: the diagonal when the aim is within 22.5 degrees of it */
    if (ax > 1.5f && ax > ay * 0.414f) m |= ddx > 0 ? BTN_RIGHT : BTN_LEFT;
    if (ay > 1.5f && ay > ax * 0.414f) m |= ddy > 0 ? BTN_DOWN : BTN_UP;
    return m;
}

/* something worth a torpedo in front of the nose */
static bool target_ahead(void) {
    int range = kpr_torp_range();
    for (int i = 0; i < KPR_MAX_FOES; i++) {
        const KprFoe *f = &kg.foe[i];
        if (!f->alive || f->pacified) continue;
        float dx = (f->x - kg.x) * kg.face, dy = fabsf(f->y - kg.y);
        if (dx > 0 && dx < range + 6 && dy < 10) return true;
    }
    for (int b = 0; b < KPR_BOSSES; b++) {
        float bx, by;
        int bw, bh;
        if (!kpr_boss_box(b, &bx, &by, &bw, &bh)) continue;
        float dx = (bx - kg.x) * kg.face - bw / 2.0f, dy = fabsf(by - kg.y);
        if (dx < range && dx > -bw && dy < bh / 2.0f + 4) return true;
    }
    return false;
}

static bool cracked_ahead(void) {
    int tx = (int)((kg.x + kg.face * 14) / KPR_TILE), ty = tile_y();
    return kpr_tile(tx, ty) == '?' || kpr_tile(tx + kg.face, ty) == '?';
}

/* fire: tap (quicker, and the facing still follows the pad) */
static int tap_fire(void) { return (kg.frame_t % 8) < 2 ? BTN_A : 0; }

/* ---- out on a dive ---------------------------------------------------------------- */

static int fuel_home_need(void) {
    int back = home_dist(dive_caps | (kg.cur.on & (1 << EQ_SPOT) ? CP_SPOT : 0), tile_x(), tile_y(), NULL);
    if (back == FBIG) back = 4000;
    int u = back * 16 / 10 * 5 / 4;                /* a quarter spare on the way back */
    if (kpr_on(EQ_EFFICIENT)) u = u * 3 / 4;
    return u + 160;
}

static int go_home(void) {
    int which = kg.cur.dock;
    home_dist(dive_caps, tile_x(), tile_y(), &which);
    if (force_dock >= 0) which = force_dock;
    int dx = KPR_DOCKS[which].x;
    float cx = dx * KPR_TILE + 8.0f;
    if (tile_y() <= KPR_SKY + 1 && fabsf(kg.x - cx) < 12) return BTN_UP;
    set_goal(dx, KPR_SKY, dive_caps);
    int nt;
    return steer(f_goal, &nt);
}

static bool task_done(const Task *t) {
    switch (t->kind) {
    case T_ITEM: return kg.cur.item[t->id] != IS_HOME || kg.where[t->id] == IW_TRAIL || kg.trail_n >= KPR_TRAIL;
    case T_ROE: return trail_eggs() == 0;
    case T_PLUG: return plug_open(t->id);
    case T_BOSS:
        /* an egg picked up on the way to Xolloth goes to Auntie Roe first */
        if (t->id == BS_XOLLOTH && trail_eggs() > 0) return true;
        return kg.cur.boss_dead[t->id] || !kg.boss[t->id].on;
    default: return false;
    }
}

/* the way to a gate: light its switch first */
static int switch_routine(int gate) {
    for (int k = 0; k < ARRAY_LEN(SWITCHES); k++) {
        const SwitchSpot *s = &SWITCHES[k];
        if (s->gate != gate) continue;
        float gx = s->at_x * KPR_TILE + 8.0f, gy = s->at_y * KPR_TILE + 8.0f;
        if (fabsf(kg.x - gx) < 4 && fabsf(kg.y - gy) < 3) {
            if (kg.face != s->face) return s->face > 0 ? BTN_RIGHT : BTN_LEFT;
            return tap_fire();
        }
        set_goal(s->at_x, s->at_y, dive_caps & ~CP_BUSTER);
        int nt;
        return steer(f_goal, &nt);
    }
    return 0;
}

static bool clear_line(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    int n = (int)(d / 6) + 1;
    for (int i = 1; i < n; i++)
        if (kpr_shot_solid(x0 + dx * i / n, y0 + dy * i / n)) return false;
    return true;
}

/* a step along the map toward (gx, gy): switches lit first for a gate in
 * the way, dirt dashed through, cracked walls shot, the thruster on long
 * straight swims */
static int path_step(int gx, int gy) {
    set_goal(gx, gy, dive_caps);
    int nt;
    int m = steer(f_goal, &nt);
    if (nt >= '0' && nt <= '7' && !((kg.gates >> (nt - '0')) & 1)) switch_pending = nt - '0';
    if (switch_pending >= 0) {
        if ((kg.gates >> switch_pending) & 1) switch_pending = -1;
        else return switch_routine(switch_pending);
    }
    if (nt == 'd' && kg.dash_t == 0 && (kg.frame_t % 6) == 0) m |= BTN_B;
    if (nt == '?' && cracked_ahead()) m |= tap_fire();
    if (target_ahead()) m |= tap_fire();
    /* a long straight swim: the thruster, while the fuel is easy */
    if (kpr_on(EQ_BOOSTER) && kg.fuel > fuel_home_need() * 2 && (m & (BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN)) &&
        f_goal[tile_y()][tile_x()] > 600 && !(m & BTN_A))
        m |= (kg.boosting || kg.dash_t > 0 || (kg.frame_t % 30) == 0) ? BTN_B : 0;
    return m;
}

static int plug_phase;
static int plug_routine(int p) {
    const Plug *q = &PLUGS[p];
    float gx = q->at_x * KPR_TILE + 8.0f, gy = q->at_y * KPR_TILE + 8.0f;
    int nt;
    plug_t++;
    if (plug_t == 1) plug_phase = 0;
    if (q->up) {
        if (fabsf(kg.x - gx) < 8 && fabsf(kg.y - gy) < 8) {
            /* right under it: hold fire (nudging into place), let go: a rocket */
            if (kg.missile_ready) return 0;
            int m = BTN_A;
            if (gx - kg.x > 2) m |= BTN_RIGHT;
            if (gx - kg.x < -2) m |= BTN_LEFT;
            if (gy - kg.y > 2) m |= BTN_DOWN;
            if (gy - kg.y < -2) m |= BTN_UP;
            return m;
        }
        set_goal(q->at_x, q->at_y, dive_caps);
        return steer(f_goal, &nt);
    }
    bool live = false;
    for (int i = 0; i < KPR_MAX_BOMBS; i++) live |= kg.bomb[i].alive != 0;
    if (live) {
        /* a bomb is down: clear off before it goes */
        set_goal(q->re_x, q->re_y, dive_caps);
        return steer(f_goal, &nt);
    }
    if (kpr_trail_count(TR_CHARGE) == 0) {
        /* no bombs left (spent, knocked off or stolen): home */
        plan_i = nplan;
        return 0;
    }
    if (plug_phase == 0) {
        float ax = q->ap_x * KPR_TILE + 8.0f, ay = q->ap_y * KPR_TILE + 8.0f;
        if (fabsf(kg.x - ax) < 4 && fabsf(kg.y - ay) < 4) plug_phase = 1;
        set_goal(q->ap_x, q->ap_y, dive_caps);
        return steer(f_goal, &nt);
    }
    if (fabsf(kg.x - gx) < 3 && fabsf(kg.y - gy) < 3 && fabsf(kg.vx) + fabsf(kg.vy) < 0.3f) {
        /* sitting still: hold the dash button until the bomb is ready, then
         * let go (and let go first if it was held from before) */
        if (kg.charge_ready) return 0;
        if (kg.b_still <= 0) return (plug_t & 1) ? 0 : BTN_B;
        return BTN_B;
    }
    /* straight across to the drop point, so the trail lies to the side */
    int m = 0;
    if (gx - kg.x > 1.5f) m |= BTN_RIGHT;
    if (gx - kg.x < -1.5f) m |= BTN_LEFT;
    if (gy - kg.y > 1.5f) m |= BTN_DOWN;
    if (gy - kg.y < -1.5f) m |= BTN_UP;
    return m;
}

static int boss_routine(int b) {
    const KprBoss *o = &kg.boss[b];
    float bx = o->x, by = o->y;
    int range = kpr_torp_range();
    float dx = bx - kg.x, dy = by - kg.y;
    bool in_reach = fabsf(dx) < 220 && fabsf(dy) < 160;
    if (!in_reach || !clear_line(kg.x, kg.y, bx, by)) return path_step((int)(bx / KPR_TILE), (int)(by / KPR_TILE));
    fight_frames++;
    int m = 0;
    int side = dx > 0 ? 1 : -1;
    float want = range * 0.7f + 12;
    if (want > 110) want = 110;
    /* line up across from it, at most a torpedo's reach away */
    if (dy > 3) m |= BTN_DOWN;
    if (dy < -3) m |= BTN_UP;
    float dist = fabsf(dx);
    if (dist > want + 8) m |= side > 0 ? BTN_RIGHT : BTN_LEFT;
    else if (dist < want - 20) {
        /* too close: back off with the fire held (the facing stays) */
        if (kg.face != side) return side > 0 ? BTN_RIGHT : BTN_LEFT;
        m |= side > 0 ? BTN_LEFT : BTN_RIGHT;
        return m | BTN_A;
    }
    if (kg.face != side && !(m & (BTN_LEFT | BTN_RIGHT))) m |= side > 0 ? BTN_RIGHT : BTN_LEFT;
    if (fabsf(dy) < 14) m |= tap_fire();
    /* the laser: cross it diagonally */
    if (b == BS_XOLLOTH && o->beam_t == 2) m = (m & ~(BTN_UP | BTN_DOWN)) | ((kg.frame_t / 40) & 1 ? BTN_UP : BTN_DOWN);
    return m;
}

/* a big one coming: swim off, dashing; home-ward if that also takes the
 * sub away from it */
static int flee(void) {
    int cur_fight = plan_i < nplan && plan[plan_i].kind == T_BOSS ? plan[plan_i].id : -1;
    for (int b = 0; b < KPR_BOSSES; b++) {
        const KprBoss *o = &kg.boss[b];
        if (!o->on || o->dying || b == cur_fight || b == BS_WHORL || b == BS_XOLLOTH) continue;
        float dx = kg.x - o->x, dy = kg.y - o->y, d2 = dx * dx + dy * dy;
        if (d2 > 110.0f * 110.0f || (o->state != 1 && d2 > 60.0f * 60.0f)) continue;
        int tx = tile_x(), ty = tile_y(), best = -1;
        float bd = -1e9f;
        int which = kg.cur.dock;
        home_dist(dive_caps, tx, ty, &which);
        set_goal(KPR_DOCKS[which].x, KPR_SKY, dive_caps);
        static const int8_t D[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
        for (int k = 0; k < 8; k++) {
            int nx = tx + D[k][0], ny = ty + D[k][1];
            if (!pass(nx, ny, dive_caps) || kpr_tile(nx, ny) == 'd' || kpr_tile(nx, ny) == '?') continue;
            if (D[k][0] && D[k][1] && (!pass(tx + D[k][0], ty, dive_caps) || !pass(tx, ty + D[k][1], dive_caps))) continue;
            float ex = nx * KPR_TILE + 8.0f - o->x, ey = ny * KPR_TILE + 8.0f - o->y;
            float gain = sqrtf(ex * ex + ey * ey) - sqrtf(d2);
            int room = 0;
            for (int r = 1; r <= 3; r++) room += pass(tx + D[k][0] * r, ty + D[k][1] * r, dive_caps);
            float home = (float)(f_goal[ty][tx] - f_goal[ny][nx]);   /* > 0: nearer home */
            float sc = gain * 3.0f + room * 4.0f + (gain > 0 ? home * 0.5f : 0);
            if (sc > bd) { bd = sc; best = k; }
        }
        if (best < 0) return -1;
        int m = 0;
        if (D[best][0] > 0) m |= BTN_RIGHT;
        if (D[best][0] < 0) m |= BTN_LEFT;
        if (D[best][1] > 0) m |= BTN_DOWN;
        if (D[best][1] < 0) m |= BTN_UP;
        if (kg.dash_t == 0 && (kg.frame_t % 16) == 0 && kg.fuel > fuel_home_need()) m |= BTN_B;
        return m;
    }
    return -1;
}


/* a creature close by: line up with it and shoot it calm, or slip past
 * one coming from straight above or below */
static int threat(void) {
    int best = -1;
    float bd = 60.0f * 60.0f;
    for (int i = 0; i < KPR_MAX_FOES; i++) {
        const KprFoe *f = &kg.foe[i];
        if (!f->alive || f->pacified) continue;
        if (f->kind == FK_SNAPWORM && f->state < 2) continue;
        if (f->kind == FK_FILCHER && f->carry < 0 && f->state != 0) continue;
        float dx = f->x - kg.x, dy = f->y - kg.y, d2 = dx * dx + dy * dy;
        if (d2 < bd && clear_line(kg.x, kg.y, f->x, f->y)) { bd = d2; best = i; }
    }
    if (best < 0) return -1;
    const KprFoe *f = &kg.foe[best];
    float dx = f->x - kg.x, dy = f->y - kg.y;
    int side = dx > 0 ? 1 : -1;
    /* level with it: face it and shoot; otherwise leave it to the look-ahead */
    if (fabsf(dy) > 8 || fabsf(dx) > kpr_torp_range()) return -1;
    if (kg.face != side) return side > 0 ? BTN_RIGHT : BTN_LEFT;
    return tap_fire();
}

/* a shot that would hit: slip sideways off its line */
static int dodge(void) {
    for (int i = 0; i < KPR_MAX_ESHOTS; i++) {
        const KprEShot *e = &kg.es[i];
        if (!e->alive || e->kind == ES_CLOUD) continue;
        float rx = e->x - kg.x, ry = e->y - kg.y, v2 = e->vx * e->vx + e->vy * e->vy;
        if (v2 < 0.01f || rx * rx + ry * ry > 90.0f * 90.0f) continue;
        float t = -(rx * e->vx + ry * e->vy) / v2;
        if (t < 0 || t > 45) continue;
        float cx = rx + e->vx * t, cy = ry + e->vy * t;
        if (cx * cx + cy * cy > 11.0f * 11.0f) continue;
        /* off to the side the shot isn't */
        float px = -e->vy, py = e->vx;
        if (px * -rx + py * -ry < 0) { px = -px; py = -py; }
        int m = 0;
        if (px > 0.3f) m |= BTN_RIGHT;
        if (px < -0.3f) m |= BTN_LEFT;
        if (py > 0.3f) m |= BTN_DOWN;
        if (py < -0.3f) m |= BTN_UP;
        int tx = tile_x() + (px > 0.3f) - (px < -0.3f), ty = tile_y() + (py > 0.3f) - (py < -0.3f);
        if (!pass(tx, ty, dive_caps)) {
            m = 0;
            if (px > 0.3f) m |= BTN_LEFT;
            if (px < -0.3f) m |= BTN_RIGHT;
            if (py > 0.3f) m |= BTN_UP;
            if (py < -0.3f) m |= BTN_DOWN;
        }
        return m;
    }
    return -1;
}

/* a creature sitting in the way, close ahead along the path */
static bool blocked_ahead(void) {
    float vx = (float)(((want_dir & BTN_RIGHT) != 0) - ((want_dir & BTN_LEFT) != 0));
    float vy = (float)(((want_dir & BTN_DOWN) != 0) - ((want_dir & BTN_UP) != 0));
    float v = sqrtf(vx * vx + vy * vy);
    if (v < 0.2f) { vx = kg.vx; vy = kg.vy; v = sqrtf(vx * vx + vy * vy); }
    if (v < 0.2f) return false;
    vx /= v;
    vy /= v;
    for (int i = 0; i < KPR_MAX_FOES; i++) {
        const KprFoe *f = &kg.foe[i];
        if (!f->alive || f->pacified) continue;
        float dx = f->x - kg.x, dy = f->y - kg.y;
        float along = dx * vx + dy * vy, across = fabsf(dx * vy - dy * vx);
        if (along > 0 && along < 48 && across < 14) return true;
    }
    return false;
}

/* a thief after the trail: turn on it and shoot it calm */
static int guard(void) {
    if (kg.trail_n == 0) return -1;
    for (int i = 0; i < KPR_MAX_FOES; i++) {
        const KprFoe *f = &kg.foe[i];
        if (!f->alive || f->pacified || f->kind != FK_FILCHER || f->state != 0) continue;
        float dx = f->x - kg.x, dy = f->y - kg.y;
        if (dx * dx + dy * dy > 110.0f * 110.0f || !clear_line(kg.x, kg.y, f->x, f->y)) continue;
        int side = dx > 0 ? 1 : -1, m = 0;
        if (fabsf(dy) > 4) m |= dy > 0 ? BTN_DOWN : BTN_UP;
        if (kg.face != side) m |= side > 0 ? BTN_RIGHT : BTN_LEFT;
        return m | tap_fire();
    }
    return -1;
}

static int out_buttons(void) {
    if (++dive_frames > 9000 && plan_i < nplan) {
        if (kpr_bot_log) printf("[bot %u] the dive drags on: home\n", (unsigned)kg.cur.frames);
        for (int i = plan_i; i < nplan; i++)
            if (plan[i].kind == T_ITEM) fail_dive[plan[i].id]++;
        plan_i = nplan;
    }
    if (stuck_t == 301 && kpr_bot_log > 1) {
        printf("[bot] stuck at %d,%d goal %d,%d caps %d:", tile_x(), tile_y(), goal_x, goal_y, goal_caps);
        for (int yy = -2; yy <= 2; yy++)
            for (int xx = -2; xx <= 2; xx++) printf(" %d", f_goal[tile_y() + yy][tile_x() + xx]);
        printf("\n");
    }
    if (stuck_t > 300) {
        /* hemmed in: shake loose, never mind the creatures */
        static const int W[4] = {BTN_UP | BTN_LEFT, BTN_DOWN | BTN_RIGHT, BTN_UP | BTN_RIGHT, BTN_DOWN | BTN_LEFT};
        if (stuck_t < 380) return W[(stuck_t / 20) % 4];
        stuck_t = 0;
        last_tx = -1;
        /* the third time on one task: give it up for this dive */
        if (++unstick_n >= 3 && plan_i < nplan && plan[plan_i].kind != T_BOSS) {
            if (plan[plan_i].kind == T_ITEM) fail_dive[plan[plan_i].id] += 2;
            plan_i++;
            unstick_n = 0;
        }
    }
    {
        int fm = guard();
        if (fm >= 0) return fm;
        (void)dodge;
        (void)flee;
        if ((plan_i >= nplan || plan[plan_i].kind != T_BOSS) && blocked_ahead()) {
            fm = threat();
            why = 't';
            if (fm >= 0) return fm;
        }
        why = '-';
    }
    /* the fuel says home */
    if (plan_i < nplan && plan[plan_i].kind != T_HOME && kg.fuel < fuel_home_need()) {
        if (kpr_bot_log) printf("[bot %u] fuel %d < %d: home\n", (unsigned)kg.cur.frames, kg.fuel, fuel_home_need());
        for (int i = plan_i; i < nplan; i++)
            if (plan[i].kind == T_ITEM) fail_dive[plan[i].id]++;
        plan_i = nplan;
    }
    while (plan_i < nplan && task_done(&plan[plan_i])) { plan_i++; plug_t = 0; fight_frames = 0; unstick_n = 0; }
    /* anything knocked loose or snatched from the trail comes first */
    int m = 0;
    for (int i = 0; i < KPR_NITEMS && kg.trail_n < KPR_TRAIL; i++) {
        if (kg.cur.item[i] != IS_HOME) continue;
        if (kg.where[i] == IW_LOOSE) {
            float dx = kg.ix[i] - kg.x, dy = kg.iy[i] - kg.y;
            if (dx * dx + dy * dy < 200.0f * 200.0f && kg.fuel > fuel_home_need() + 200) {
                set_goal((int)(kg.ix[i] / KPR_TILE), (int)(kg.iy[i] / KPR_TILE), dive_caps);
                int nt;
                m = steer(f_goal, &nt);
                if (fabsf(dx) < 12 && fabsf(dy) < 12) {
                    m = 0;
                    if (dx > 2) m |= BTN_RIGHT;
                    if (dx < -2) m |= BTN_LEFT;
                    if (dy > 2) m |= BTN_DOWN;
                    if (dy < -2) m |= BTN_UP;
                }
                return m | (target_ahead() ? tap_fire() : 0);
            }
        }
        if (kg.where[i] == IW_HELD && chase_t < 900)
            for (int f = 0; f < KPR_MAX_FOES; f++) {
                const KprFoe *o = &kg.foe[f];
                if (!o->alive || o->carry != i) continue;
                float dx = o->x - kg.x, dy = o->y - kg.y;
                if (dx * dx + dy * dy > 240.0f * 240.0f || kg.fuel < fuel_home_need() + 200) continue;
                chase_t++;
                int side = dx > 0 ? 1 : -1;
                if (fabsf(dx) < 70 && fabsf(dy) < 7 && clear_line(kg.x, kg.y, o->x, o->y)) {
                    /* in line: face the thief and shoot it calm */
                    if (kg.face != side) return side > 0 ? BTN_RIGHT : BTN_LEFT;
                    return tap_fire();
                }
                /* get level with it, a little way off, by the map */
                int gx = (int)((o->x - side * 40) / KPR_TILE), gy = (int)(o->y / KPR_TILE);
                if (!pass(gx, gy, dive_caps)) gx = (int)(o->x / KPR_TILE);
                set_goal(gx, gy, dive_caps);
                int nt;
                m = steer(f_goal, &nt);
                return m | (target_ahead() ? tap_fire() : 0);
            }
    }
    if (plan_i >= nplan) return go_home() | (target_ahead() ? tap_fire() : 0);
    const Task *t = &plan[plan_i];
    kpr_bot_goal = t->kind * 100 + t->id;
    int gx = -1, gy = -1;
    switch (t->kind) {
    case T_ITEM: gx = KPR_ITEMS[t->id].x; gy = KPR_ITEMS[t->id].y; break;
    case T_ROE: gx = KPR_ROE.x; gy = KPR_ROE.y; break;
    case T_PLUG: return plug_routine(t->id);
    case T_BOSS: return boss_routine(t->id);
    default: return go_home();
    }
    return path_step(gx, gy);
}

/* ---- at a dock: the fitting, then launch ------------------------------------------ */

static int dock_buttons(void) {
    if (kg.dock_phase == 0) return (state_frames > 22 && (state_frames & 1)) ? BTN_A : 0;
    if (mode != 0 || nplan == 0) { make_plan(); mode = 0; }
    if (state_frames < 8 || (state_frames & 1)) return 0;
    /* set the fitting: take off what isn't wanted first */
    int want = -1;
    for (int e = 0; e < KPR_EQUIP && want < 0; e++)
        if (kpr_on(e) && !((want_on >> e) & 1)) want = e;
    for (int e = 0; e < KPR_EQUIP && want < 0; e++)
        if (!kpr_on(e) && ((want_on >> e) & 1) && ((kg.cur.have >> e) & 1)) want = e;
    if (want >= 0) {
        int cx = kg.dock_cur % 5, cy = kg.dock_cur / 5, wx = want % 5, wy = want / 5;
        if (cy < wy) return BTN_DOWN;
        if (cy > wy) return BTN_UP;
        if (cx < wx) return BTN_RIGHT;
        if (cx > wx) return BTN_LEFT;
        return BTN_A;
    }
    mode = 1;
    plan_i = 0;
    stuck_t = 0;
    chase_t = 0;
    dive_frames = 0;
    switch_pending = -1;
    plug_t = 0;
    fight_frames = 0;
    unstick_n = 0;
    return BTN_B;
}

/* ---- looking ahead ----------------------------------------------------------------- */

static KprGame sim_keep;
static int commit_mask, commit_t;

/* the real game, run ahead on a copy with these buttons held (fire tapped);
 * how many knocks it takes, and how near the goal it ends up */
static int simulate(int mask, int dash_first, int frames, int *hurt) {
    memcpy(&sim_keep, &kg, sizeof kg);
    bool was = kpr_simulating;
    kpr_simulating = true;
    uint32_t keep_cur = kpr_sim_cur, keep_prev = kpr_sim_prev;
    int h0 = kg.hits_taken, t0 = kg.trail_n, c0 = kpr_trail_count(TR_CHARGE);
    kpr_sim_cur = 0;
    bool dry = false;
    for (int i = 0; i < frames; i++) {
        int m = mask;
        if (m & BTN_A) m = (i % 8) < 2 ? m : (m & ~BTN_A);
        if (dash_first && i < 2) m |= BTN_B;
        kpr_sim_prev = kpr_sim_cur;
        kpr_sim_cur = (uint32_t)m;
        kg.frame_t++;
        kg.state_t++;
        kpr_play_step();
        if (kg.state != KS_PLAY) { dry = kg.state == KS_TOWED; break; }
    }
    /* things knocked off or snatched from the trail count as badly as a knock */
    int lost = imax(0, t0 - kg.trail_n);
    int bombs_lost = imax(0, c0 - kpr_trail_count(TR_CHARGE));
    *hurt = (kg.hits_taken - h0) + (dry ? 50 : 0) + lost + bombs_lost;
    int end = f_goal[tile_y()][tile_x()];
    memcpy(&kg, &sim_keep, sizeof kg);
    kpr_simulating = was;
    kpr_sim_cur = keep_cur;
    kpr_sim_prev = keep_prev;
    return end;
}

/* the plan's buttons, unless the look-ahead sees them get knocked: then the
 * safest of the other ways that still gets on */
static int look_ahead(int want) {
    if (commit_t > 0) { commit_t--; return commit_mask; }
    if (stuck_t > 400) return want;        /* hemmed in too long: push on through */
    bool danger = false;
    for (int i = 0; i < KPR_MAX_FOES && !danger; i++) {
        const KprFoe *f = &kg.foe[i];
        if (f->alive && !f->pacified && fabsf(f->x - kg.x) < 110 && fabsf(f->y - kg.y) < 90) danger = true;
    }
    for (int i = 0; i < KPR_MAX_ESHOTS && !danger; i++)
        if (kg.es[i].alive && fabsf(kg.es[i].x - kg.x) < 110 && fabsf(kg.es[i].y - kg.y) < 90) danger = true;
    for (int b = 0; b < KPR_BOSSES && !danger; b++)
        if (kg.boss[b].on && fabsf(kg.boss[b].x - kg.x) < 200 && fabsf(kg.boss[b].y - kg.y) < 160) danger = true;
    if (!danger) return want;
    int hurt;
    int start = f_goal[tile_y()][tile_x()];
    simulate(want, 0, 36, &hurt);
    if (hurt == 0) return want;
    static const int DIRS[9] = {0, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_UP | BTN_LEFT, BTN_UP | BTN_RIGHT,
                                BTN_DOWN | BTN_LEFT, BTN_DOWN | BTN_RIGHT};
    int best = want, best_dash = 0, best_score = -1000000;
    for (int k = 0; k < 9; k++)
        for (int dsh = 0; dsh < 2; dsh++) {
            if (dsh && !DIRS[k]) continue;
            int m = DIRS[k] | BTN_A;
            int end = simulate(m, dsh, 36, &hurt);
            int prog = (start == FBIG || end == FBIG) ? 0 : start - end;
            int score = prog - hurt * 400 - dsh * 350;   /* a dash costs as much as a knock */
            if (score > best_score) { best_score = score; best = m; best_dash = dsh; }
        }
    commit_mask = best | (best_dash ? BTN_B : 0);
    commit_t = 5;
    return commit_mask;
}

int kpr_bot_buttons(void) {
    if (kg.state != last_state) { last_state = kg.state; state_frames = 0; }
    state_frames++;
    /* let go of everything now and then (a scene change swallows held buttons) */
    if (state_frames < 3 || ++release_t % 97 == 0) return 0;
    switch (kg.state) {
    case KS_TITLE: return (state_frames & 1) && state_frames > 10 ? BTN_A : 0;
    case KS_INTRO: return state_frames > 25 && (state_frames & 1) ? BTN_A : 0;
    case KS_DOCK: return dock_buttons();
    case KS_ROE: return state_frames > 25 && (state_frames & 1) ? BTN_A : 0;
    case KS_TOWED:
        if (state_frames == 3)
            for (int i = 0; i < nplan; i++)
                if (plan[i].kind == T_ITEM) fail_dive[plan[i].id]++;
        return state_frames > 45 && (state_frames & 1) ? BTN_A : 0;
    case KS_ENDING:
    case KS_CREDITS: return BTN_A;
    case KS_PLAY:
        if (mode == 0) {
            /* launched without a plan (a new game, a continue) */
            make_plan();
            mode = 1;
            plan_i = 0;
        }
        {
            int tx = tile_x(), ty = tile_y();
            if (tx == last_tx && ty == last_ty) stuck_t++;
            else { stuck_t = 0; last_tx = tx; last_ty = ty; }
            int m = out_buttons();
            bool at_plug = false;
            if (plan_i < nplan && plan[plan_i].kind == T_PLUG) {
                const Plug *q = &PLUGS[plan[plan_i].id];
                at_plug = fabsf(kg.x - (q->at_x * KPR_TILE + 8.0f)) < 10 && fabsf(kg.y - (q->at_y * KPR_TILE + 8.0f)) < 10;
            }
            if (kg.state == KS_PLAY && !at_plug) m = look_ahead(m);
            if (kpr_bot_log > 1 && kg.cur.frames % 30 == 0)
                printf("  [%u] %c at %d,%d fuel %d goal %d/%d plan %d/%d m %02x hits %d trail %d\n", (unsigned)kg.cur.frames, why,
                       tile_x(), tile_y(), kg.fuel, plan_i < nplan ? plan[plan_i].kind : -1,
                       plan_i < nplan ? plan[plan_i].id : -1, plan_i, nplan, m, kg.hits_taken, kg.trail_n);
            return m;
        }
    default: return 0;
    }
}

int kpr_bot_idle(void) { return 0; }

/* a fresh start (the title's demo) */
void kpr_bot_reset(void) {
    nplan = plan_i = 0;
    mode = 0;
    last_state = -1;
    commit_t = 0;
    stuck_t = plug_t = chase_t = dive_frames = unstick_n = idle_plans = 0;
    switch_pending = force_dock = -1;
    memset(fail_dive, 0, sizeof fail_dive);
}

/* tests: with every way open, every pick-up, Roe and both docks can be
 * reached from the Pier */
int kpr_reach_check(void) {
    static uint8_t keep[KPR_H][KPR_W];
    memcpy(keep, kg.tile, sizeof keep);
    uint8_t gates = kg.gates;
    for (int y = 0; y < KPR_H; y++)
        for (int x = 0; x < KPR_W; x++) kg.tile[y][x] = (uint8_t)KPR_ROWS[y][x];
    field_from(f_tmp, KPR_DOCKS[0].x, KPR_SKY, CP_ALL | CP_OPEN);
    memcpy(kg.tile, keep, sizeof keep);
    kg.gates = gates;
    int bad = 0;
    for (int i = 0; i < KPR_NITEMS; i++)
        if (f_tmp[KPR_ITEMS[i].y][KPR_ITEMS[i].x] == FBIG) { bad++; fprintf(stderr, "item %d unreachable\n", i); }
    if (f_tmp[KPR_ROE.y][KPR_ROE.x] == FBIG) bad++;
    if (f_tmp[KPR_SKY][KPR_DOCKS[1].x] == FBIG) bad++;
    if (f_tmp[140][80] == FBIG) bad++;
    return bad;
}
