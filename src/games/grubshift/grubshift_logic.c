/* GRUB SHIFT - the rules. Everything here is a pure function of Board, so the
 * autoplay bot (and the tests) can try moves on copies of the board.
 * The rules follow Bug Hunter (UFO 50 #2) as documented in
 * docs/games/02-grub-shift.md; names, art and wording are ours. */
#include "grubshift.h"

/*  name        kind          cost 2step  description (tiny font, ~80 chars) */
const ChipInfo CHIPS[TOOL_COUNT] = {
    {"ROLL", KIND_MOVE, 2, 0, "ROLL 1-2 TILES STRAIGHT. SHOVES GRUBS AND PICKS UP PODS. NOT UPHILL."},
    {"SCURRY", KIND_MOVE, 2, 0, "ROLL 1-2 TILES IN 8 DIRECTIONS. SHOVES GRUBS. NOT UPHILL."},
    {"STREAK", KIND_MOVE, 2, 0, "ROLL ANY DISTANCE STRAIGHT. SHOVES GRUBS. NOT UPHILL."},
    {"RUSH", KIND_MOVE, 4, 0, "ROLL ANY DISTANCE IN 8 DIRECTIONS. SHOVES GRUBS. NOT UPHILL."},
    {"SKIP", KIND_MOVE, 2, 0, "HOP 1-2 TILES STRAIGHT TO A FREE TILE. ANY HEIGHT. NO SHOVE."},
    {"LEAP", KIND_MOVE, 2, 0, "HOP 1-2 TILES IN 8 DIRECTIONS TO A FREE TILE. ANY HEIGHT."},
    {"BOUND", KIND_MOVE, 4, 0, "HOP TO ANY FREE TILE WITHIN TWO."},
    {"BLINK", KIND_MOVE, 3, 0, "BEAM ONTO ANY POD, SOUR ONES TOO, AND PICK IT UP."},
    {"PERCH", KIND_MOVE, 3, 0, "BEAM ONTO ANY FREE PLANTER."},
    {"DIVE", KIND_MOVE, 3, 0, "BEAM ONTO ANY FREE TILE NEXT TO A SINKHOLE."},
    {"HUSTLE", KIND_MOVE, 4, 0, "ROLL 1-2 TILES STRAIGHT. RECHARGES WHENEVER A GRUB DIES."},
    {"ZAP", KIND_ATTACK, 2, 0, "SHOOT 1-2 TILES STRAIGHT, HITTING THE WHOLE LINE. NOT UPHILL."},
    {"ARC", KIND_ATTACK, 2, 0, "SHOOT 1-2 TILES IN 8 DIRECTIONS, HITTING THE WHOLE LINE. NOT UPHILL."},
    {"BEAM", KIND_ATTACK, 2, 0, "SHOOT A WHOLE ROW OR COLUMN. BLOCKED BY HIGHER GROUND."},
    {"FLARE", KIND_ATTACK, 4, 0, "SHOOT ALL THE WAY IN 8 DIRECTIONS. BLOCKED BY HIGHER GROUND."},
    {"TOSS", KIND_ATTACK, 2, 0, "LOB AT ONE TILE UP TO 2 AWAY, STRAIGHT. ANY HEIGHT."},
    {"PITCH", KIND_ATTACK, 2, 0, "LOB AT ONE TILE UP TO 2 AWAY IN 8 DIRECTIONS. ANY HEIGHT."},
    {"MORTAR", KIND_ATTACK, 4, 0, "LOB AT ANY TILE WITHIN TWO. ANY HEIGHT."},
    {"IGNITE", KIND_ATTACK, 4, 0, "SET OFF ANY FIZZ POD ON THE FIELD."},
    {"QUAKE", KIND_ATTACK, 4, 0, "HIT EVERY TILE AROUND ONE SINKHOLE, HIGH OR LOW."},
    {"PULSE", KIND_ATTACK, 3, 0, "HIT ALL 8 TILES AROUND TILLY AT ONCE."},
    {"HAIL", KIND_ATTACK, 4, 0, "HIT EVERY PLANTER. USE IT FROM THE GROUND OR IT HITS TILLY TOO!"},
    {"CRACK", KIND_ATTACK, 3, 0, "BLOW UP ANY EGG. THE BLAST HITS THE TILES AROUND IT."},
    {"TRACK", KIND_ATTACK, 4, 0, "SHOOT 1-2 TILES STRAIGHT. RECHARGES WHEN TILLY CHANGES HEIGHT."},
    {"SEED", KIND_SPECIAL, 2, 0, "DROP A FIZZ POD ON ANY FREE TILE. A THIRD POD ON A TILE GOES BOOM!"},
    {"TILL", KIND_SPECIAL, 2, 1, "LOWER ONE PLANTER, THEN RAISE ANOTHER TILE."},
    {"OVERDRIVE", KIND_SPECIAL, 4, 0, "UNTIL THE SHIFT ENDS, GRUBS TILLY TOUCHES ARE SQUASHED OUTRIGHT."},
    {"GATHER", KIND_SPECIAL, 2, 0, "COLLECT A POD AND EVERY POD AROUND IT."},
    {"RELOAD", KIND_SPECIAL, 4, 0, "RECHARGE EVERY USED ATTACK TOOL."},
    {"REFUEL", KIND_SPECIAL, 4, 0, "RECHARGE EVERY USED MOVE TOOL."},
    {"BOOST", KIND_SPECIAL, 4, 0, "UNTIL THE SHIFT ENDS, STRAIGHT TOOLS REACH ALL THE WAY, 8 WAYS."},
    {"BORE", KIND_SPECIAL, 4, 0, "OPEN A NEW SINKHOLE NEXT TO AN OLD ONE."},
    {"VOLATILE", KIND_SPECIAL, 4, 0, "UNTIL THE SHIFT ENDS, GRUBS POPPED BY ATTACKS EXPLODE."},
    {"REWIND", KIND_SPECIAL, 3, 0, "TURN ANY GRUB AND ITS NEIGHBOURS BACK INTO LARVAE. EGGS TOO."},
    {"MIST", KIND_SPECIAL, 2, 1, "MIST TWO FREE TILES. NO GRUB HATCHES THERE; ONE SHOVED THERE DIES."},
    {"RESTOCK", KIND_SPECIAL, 4, 0, "CALL DOWN FOUR NEW FIZZ PODS."},
    {"JUMPER", KIND_SPECIAL, 4, 0, "RECHARGE THE TOOLS ON EITHER SIDE OF THIS ONE."},
    {"FLIP", KIND_SPECIAL, 2, 0, "SWAP HIGH AND LOW GROUND EVERYWHERE."},
    {"HOVER", KIND_SPECIAL, 2, 0, "UNTIL THE SHIFT ENDS, ROLLS IGNORE HEIGHT."},
    {"SIGHT", KIND_SPECIAL, 2, 0, "UNTIL THE SHIFT ENDS, SHOTS IGNORE HEIGHT."},
    {"SHOO", KIND_SPECIAL, 5, 0, "EVERY GRUB SCUTTLES TO A TILE BESIDE IT, EVEN INTO A PIT OR A POD."},
};

static const int8_t DIR8[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
static const uint8_t PAIR_A[PAIR_COUNT] = {SP_SPARK, SP_MOUND, SP_SHELL};
static const uint8_t PAIR_B[PAIR_COUNT] = {SP_HIVE, SP_SOUR, SP_BURROW};

/* How a grub dies. Only attacks, blasts and sparks trigger abilities. */
enum { CAUSE_ATTACK, CAUSE_BOOM, CAUSE_SPARK, CAUSE_STOMP, CAUSE_HOLE, CAUSE_TOUCH, CAUSE_SPRAY, CAUSE_POD };

int gs_pair_of(int sp) {
    switch (sp) {
    case SP_SPARK: case SP_HIVE: return PAIR_GOLD;
    case SP_MOUND: case SP_SOUR: return PAIR_LEAF;
    case SP_SHELL: case SP_BURROW: return PAIR_SKY;
    default: return -1;
    }
}

static bool inb(int x, int y) { return x >= 0 && y >= 0 && x < GW && y < GH; }
static bool is_player(const Board *b, int x, int y) { return b->px == x && b->py == y; }

static void emit(Events *ev, int type, int x, int y, int x2, int y2, int a) {
    if (!ev || ev->n >= (int)(sizeof ev->ev / sizeof ev->ev[0])) return;
    Event *e = &ev->ev[ev->n++];
    e->type = (uint8_t)type;
    e->x = (int8_t)x; e->y = (int8_t)y; e->x2 = (int8_t)x2; e->y2 = (int8_t)y2;
    e->a = (uint8_t)a;
}

int gs_count_bugs(const Board *b, int level) {
    int n = 0;
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (b->bsp[y][x] && b->bsp[y][x] != SP_DRONE && (level < 0 || b->blv[y][x] == level)) n++;
    return n;
}

int gs_job_of(int contract) { return contract > 15 ? 13 + (contract - 13) % 3 : contract; }
int gs_days_for(int contract) {
    int j = gs_job_of(contract);
    return j <= 3 ? 10 : j <= 6 ? 9 : 8;
}

/* a grown grub (adult or queen) of this species */
static bool grown_sp(const Board *b, int x, int y, int sp) {
    return inb(x, y) && b->bsp[y][x] == sp && (b->blv[y][x] == LV_ADULT || b->blv[y][x] == LV_QUEEN);
}

static bool hive_has_drone(const Board *b, int x, int y) {
    for (int d = 0; d < 8; d++) {
        int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
        if (inb(nx, ny) && b->bsp[ny][nx] == SP_DRONE) return true;
    }
    return false;
}

/* a hivebug with its drone beside it can't be hurt, squashed or touched */
static bool shielded(const Board *b, int x, int y) { return grown_sp(b, x, y, SP_HIVE) && hive_has_drone(b, x, y); }

static bool near_grown_sour(const Board *b, int x, int y) {
    for (int d = 0; d < 8; d++)
        if (grown_sp(b, x + DIR8[d][0], y + DIR8[d][1], SP_SOUR)) return true;
    return false;
}

/* ------------------------------------------------------------------ */
/* the shop                                                             */

/* Five offers cost 2, one costs 3 and two cost 4 (the 5-cost SHOO sits with
 * the 4s). A bought offer is replaced at once by another of its price. */
static int tier_of(int chip) { return CHIPS[chip].cost <= 2 ? 0 : CHIPS[chip].cost == 3 ? 1 : 2; }
static int tier_of_offer(int o) { return o < 5 ? 0 : o < 6 ? 1 : 2; }

static void refill_offer(Board *b, int o, int not_this) {
    uint8_t pool[TOOL_COUNT];
    int np = 0;
    for (int c = 0; c < TOOL_COUNT; c++) {
        if (tier_of(c) != tier_of_offer(o) || c == not_this) continue;
        bool shown = false;
        for (int k = 0; k < OFFERS; k++) shown |= b->shop[k] == c;
        if (!shown) pool[np++] = (uint8_t)c;
    }
    b->shop[o] = np ? pool[rng_range(&b->rng, 0, np - 1)] : TOOL_NONE;
}

static void roll_shop(Board *b) {
    memset(b->shop, TOOL_NONE, sizeof b->shop);
    for (int o = 0; o < OFFERS; o++) refill_offer(b, o, -1);
}

/* ------------------------------------------------------------------ */
/* grubs, terrain and pods                                              */

static bool random_tile(Board *b, int *ox, int *oy, bool (*ok)(const Board *, int, int)) {
    int cand[GW * GH], n = 0;
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (ok(b, x, y)) cand[n++] = y * GW + x;
    if (n == 0) return false;
    int k = cand[rng_range(&b->rng, 0, n - 1)];
    *ox = k % GW;
    *oy = k / GW;
    return true;
}

static void place_bug(Board *b, int x, int y, int sp, int lv) {
    b->bsp[y][x] = (uint8_t)sp;
    b->blv[y][x] = (uint8_t)lv;
    b->bhp[y][x] = (uint8_t)((sp == SP_SHELL && (lv == LV_ADULT || lv == LV_QUEEN)) ? 2 : 1);
}

static void convert_sour(Board *b, int x, int y, Events *ev) {
    /* a grown sourmite spoils the fizz pods beside it, at any height */
    for (int d = 0; d < 8; d++) {
        int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
        if (!inb(nx, ny) || !b->pods[ny][nx]) continue;
        b->sour[ny][nx] = (uint8_t)imin(2, b->sour[ny][nx] + b->pods[ny][nx]);
        b->pods[ny][nx] = 0;
        emit(ev, EV_SOUR, nx, ny, 0, 0, b->sour[ny][nx]);
    }
}

/* the spoiling never stops while a sourmite lives */
static void sour_pass(Board *b, Events *ev) {
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (grown_sp(b, x, y, SP_SOUR)) convert_sour(b, x, y, ev);
}

/* A moundmaker's private planter: it raises its own tile and lowers the
 * planters beside it, except under another moundmaker or a pod. It does this
 * when it appears and again every morning. */
static void mound_turn(Board *b, int x, int y, Events *ev) {
    if (b->hole[y][x]) return;
    if (!b->elev[y][x]) { b->elev[y][x] = 1; emit(ev, EV_RAISE, x, y, 0, 0, 1); }
    for (int d = 0; d < 4; d++) {
        int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
        if (!inb(nx, ny) || !b->elev[ny][nx] || grown_sp(b, nx, ny, SP_MOUND) || b->pods[ny][nx] || b->sour[ny][nx]) continue;
        b->elev[ny][nx] = 0;
        emit(ev, EV_RAISE, nx, ny, 0, 0, 0);
    }
}

static bool drone_free(const Board *b, int x, int y) {
    return inb(x, y) && !b->hole[y][x] && !b->bsp[y][x] && !is_player(b, x, y);
}

/* What a grub does the moment it is grown (by growing up or hatching grown). */
static void on_grow(Board *b, int x, int y, Events *ev) {
    int sp = b->bsp[y][x];
    emit(ev, EV_GROW, x, y, 0, 0, sp);
    if (sp == SP_MOUND) {
        mound_turn(b, x, y, ev);
    } else if (sp == SP_HIVE && !hive_has_drone(b, x, y)) {
        int opts[8], n = 0;
        for (int d = 0; d < 8; d++)
            if (drone_free(b, x + DIR8[d][0], y + DIR8[d][1])) opts[n++] = d;
        if (n > 0) {
            int d = opts[rng_range(&b->rng, 0, n - 1)];
            int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
            place_bug(b, nx, ny, SP_DRONE, 0);
            emit(ev, EV_DRONE, nx, ny, x, y, 0);
        }
    } else if (sp == SP_SOUR) {
        convert_sour(b, x, y, ev);
    }
}

static bool spawnable(const Board *b, int x, int y) {
    return !b->hole[y][x] && !b->bsp[y][x] && !is_player(b, x, y) && !b->spray[y][x] && !b->pods[y][x] && !b->sour[y][x];
}

/* New grubs hatch at the level their colour has reached. */
static void spawn_bugs(Board *b, int n, Events *ev) {
    int x, y;
    for (int i = 0; i < n; i++) {
        if (!random_tile(b, &x, &y, spawnable)) return;
        int c = rng_range(&b->rng, 0, PAIR_COUNT - 1);
        place_bug(b, x, y, b->pair_sp[c], b->stage[c]);
        emit(ev, EV_GROW, x, y, 0, 0, 0);
        if (b->stage[c] >= LV_ADULT) on_grow(b, x, y, ev);
    }
}

static bool terrain_hole_ok(const Board *b, int x, int y) {
    return !b->hole[y][x] && !b->bsp[y][x] && !is_player(b, x, y) && !b->pods[y][x] && !b->sour[y][x];
}
static bool terrain_plat_ok(const Board *b, int x, int y) { return !b->hole[y][x] && !b->elev[y][x]; }

/* Each morning the ground is dealt again: the job table's holes (1 or 2)
 * and raised planters (4-6, or 7-8 on every third contract). */
static void deal_terrain(Board *b, Events *ev) {
    int j = gs_job_of(b->contract);
    int holes = j % 3 == 2 ? 2 : 1;
    int plats = j % 3 == 0 ? rng_range(&b->rng, 7, 8) : rng_range(&b->rng, 4, 6);
    memset(b->elev, 0, sizeof b->elev);
    memset(b->hole, 0, sizeof b->hole);
    int x, y;
    for (int i = 0; i < holes; i++)
        if (random_tile(b, &x, &y, terrain_hole_ok)) { b->hole[y][x] = 1; emit(ev, EV_HOLE, x, y, 0, 0, 1); }
    for (int i = 0; i < plats; i++)
        if (random_tile(b, &x, &y, terrain_plat_ok)) b->elev[y][x] = 1;
    emit(ev, EV_RAISE, b->px, b->py, 0, 0, 3);
}

/* ------------------------------------------------------------------ */
/* setup                                                                */

static void add_pod(Board *b, int x, int y, Events *ev);
static void check_drones(Board *b, Events *ev);

static bool drop_ok(const Board *b, int x, int y) { return !is_player(b, x, y) && !b->bsp[y][x] && !b->sour[y][x]; }

/* Pods rain on distinct random tiles. One that lands in a pit is lost, and a
 * third pod on a tile blows it open (not in the opening drop). */
static void drop_pods(Board *b, int n, bool opening, Events *ev) {
    uint8_t used[GH][GW];
    memset(used, 0, sizeof used);
    for (int i = 0; i < n; i++) {
        int cand[GW * GH], m = 0;
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (drop_ok(b, x, y) && !used[y][x] && !(opening && (b->hole[y][x] || b->pods[y][x] >= 2))) cand[m++] = y * GW + x;
        if (m == 0) break;
        int k = cand[rng_range(&b->rng, 0, m - 1)];
        int x = k % GW, y = k / GW;
        used[y][x] = 1;
        if (b->hole[y][x]) emit(ev, EV_POD, x, y, 0, 0, 9);
        else add_pod(b, x, y, ev);
    }
    sour_pass(b, ev);
}

void gs_new_contract(Board *b, int contract, int pair_mask, uint64_t seed) {
    memset(b, 0, sizeof *b);
    rng_seed(&b->rng, seed);
    b->contract = (uint8_t)contract;
    b->days = (uint8_t)gs_days_for(contract);
    b->quota = QUOTA;
    b->spawn_n = 4;
    b->pods_n = 4;
    b->day = 1;
    b->status = ST_PLAYING;
    for (int p = 0; p < PAIR_COUNT; p++) b->pair_sp[p] = ((pair_mask >> p) & 1) ? PAIR_B[p] : PAIR_A[p];
    /* from contract 10 one colour, from 13 two, hatch already grown */
    int j = gs_job_of(contract), pre = j >= 13 ? 2 : j >= 10 ? 1 : 0;
    int order[PAIR_COUNT] = {0, 1, 2};
    for (int i = PAIR_COUNT - 1; i > 0; i--) {
        int k = rng_range(&b->rng, 0, i);
        int t = order[i]; order[i] = order[k]; order[k] = t;
    }
    for (int i = 0; i < pre; i++) {
        b->stage[order[i]] = LV_ADULT;
        b->grown_mask |= (uint8_t)(1 << order[i]);
    }
    /* Tilly starts away from the walls */
    b->px = (int8_t)rng_range(&b->rng, 1, GW - 2);
    b->py = (int8_t)rng_range(&b->rng, 1, GH - 2);
    deal_terrain(b, NULL);
    drop_pods(b, 3, true, NULL);
    spawn_bugs(b, 5, NULL);
    sour_pass(b, NULL);
    check_drones(b, NULL);
    static const uint8_t kit[SLOTS] = {CH_ROLL, CH_ROLL, CH_ROLL, CH_ROLL, CH_SKIP, CH_ZAP, CH_TOSS};
    memcpy(b->chips, kit, SLOTS);
    roll_shop(b);
}

/* ------------------------------------------------------------------ */
/* damage resolution                                                    */

typedef struct { int8_t x, y; uint8_t cause; } Hit;
typedef struct {
    Hit q[400];
    int qh, qt;
    uint8_t boomed[GH][GW];
    uint8_t revert[GH][GW]; /* sour pods to turn sweet again once the dust settles */
} HitQ;

static void hitq_init(HitQ *h) {
    h->qh = h->qt = 0;
    memset(h->boomed, 0, sizeof h->boomed);
    memset(h->revert, 0, sizeof h->revert);
}

static void push_hit(HitQ *h, int x, int y, int cause) {
    if (h->qt < (int)(sizeof h->q / sizeof h->q[0])) h->q[h->qt++] = (Hit){(int8_t)x, (int8_t)y, (uint8_t)cause};
}

/* A blast on a tile: it and its 8 neighbours at the same height. */
static void blast(Board *b, HitQ *h, int x, int y, int elev, Events *ev) {
    emit(ev, EV_BOOM, x, y, 0, 0, 0);
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx, ny = y + dy;
            if (!inb(nx, ny)) continue;
            if (!(dx == 0 && dy == 0) && (b->hole[ny][nx] || b->elev[ny][nx] != elev)) continue;
            push_hit(h, nx, ny, CAUSE_BOOM);
        }
}

static bool damaging(int cause) { return cause == CAUSE_ATTACK || cause == CAUSE_BOOM || cause == CAUSE_SPARK; }

static void kill_bug(Board *b, HitQ *h, int x, int y, int cause, Events *ev) {
    int sp = b->bsp[y][x], lv = b->blv[y][x];
    b->bsp[y][x] = SP_NONE;
    if (sp == SP_DRONE) { emit(ev, EV_KILL, x, y, 0, 0, sp); return; }
    if (lv == LV_EGG) { emit(ev, EV_EGG, x, y, 0, 0, sp); return; }
    b->kills++;
    emit(ev, cause == CAUSE_STOMP ? EV_STOMP : EV_KILL, x, y, 0, 0, sp);
    bool grown = lv == LV_ADULT || lv == LV_QUEEN;
    /* a dead sourmite's sour pods turn back into fizz pods */
    if (sp == SP_SOUR && grown)
        for (int d = 0; d < 8; d++)
            if (inb(x + DIR8[d][0], y + DIR8[d][1])) h->revert[y + DIR8[d][1]][x + DIR8[d][0]] = 1;
    if (!damaging(cause) || !grown) return; /* stomps, pits, pods and sprays never trigger abilities */
    if (sp == SP_SPARK) {
        /* sparks fly two tiles each way at the grub's own height; higher
         * ground stops them, lower ground and pits pass beneath */
        int e0 = b->elev[y][x];
        for (int d = 0; d < 4; d++)
            for (int k = 1; k <= 2; k++) {
                int sx = x + DIR8[d][0] * k, sy = y + DIR8[d][1] * k;
                if (!inb(sx, sy) || (!b->hole[sy][sx] && b->elev[sy][sx] > e0)) break;
                emit(ev, EV_SPARK, x, y, sx, sy, 0);
                if (!b->hole[sy][sx] && b->elev[sy][sx] == e0) push_hit(h, sx, sy, CAUSE_SPARK);
            }
    } else if (sp == SP_BURROW) {
        b->hole[y][x] = 1;
        b->pods[y][x] = 0;
        b->sour[y][x] = 0;
        b->elev[y][x] = 0;
        emit(ev, EV_HOLE, x, y, 0, 0, 0);
    }
    if ((b->fx & FX_VOLATILE) && cause == CAUSE_ATTACK && !b->hole[y][x]) blast(b, h, x, y, b->elev[y][x], ev);
}

static void check_drones(Board *b, Events *ev) {
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            if (b->bsp[y][x] != SP_DRONE) continue;
            bool near = false;
            for (int d = 0; d < 8; d++) near |= grown_sp(b, x + DIR8[d][0], y + DIR8[d][1], SP_HIVE);
            if (!near) { b->bsp[y][x] = SP_NONE; emit(ev, EV_KILL, x, y, 0, 0, SP_DRONE); }
        }
}

static void run_hits(Board *b, HitQ *h, Events *ev) {
    while (h->qh < h->qt) {
        Hit hit = h->q[h->qh++];
        int x = hit.x, y = hit.y, cause = hit.cause;
        if (!inb(x, y)) continue;
        if (damaging(cause)) emit(ev, EV_AREA, x, y, 0, 0, cause);
        if (is_player(b, x, y) && damaging(cause) && (b->status == ST_PLAYING || b->status == ST_WON)) {
            b->status = ST_DEAD;
            emit(ev, EV_DIE, x, y, 0, 0, 0);
        }
        if ((cause == CAUSE_ATTACK || cause == CAUSE_SPARK) && b->sour[y][x]) {
            /* shooting a sour pod only feeds it; three of them become an egg */
            b->sour[y][x]++;
            emit(ev, EV_SOUR, x, y, 0, 0, b->sour[y][x]);
            if (b->sour[y][x] >= 3) {
                b->sour[y][x] = 0;
                if (!b->bsp[y][x]) {
                    place_bug(b, x, y, b->pair_sp[rng_range(&b->rng, 0, 2)], LV_EGG);
                    emit(ev, EV_EGG, x, y, 0, 0, 254);
                    continue; /* the shot is spent making the egg */
                }
            }
        }
        if (damaging(cause) && b->pods[y][x] && !h->boomed[y][x]) {
            h->boomed[y][x] = 1;
            b->pods[y][x] = 0;
            blast(b, h, x, y, b->elev[y][x], ev);
            continue; /* the tile itself is in the queue again as part of the blast */
        }
        if (!b->bsp[y][x]) continue;
        if ((damaging(cause) || cause == CAUSE_TOUCH || cause == CAUSE_STOMP) && shielded(b, x, y)) {
            emit(ev, EV_HIT, x, y, 0, 0, b->bsp[y][x]);
            continue;
        }
        if (damaging(cause) && b->bhp[y][x] > 1) {
            b->bhp[y][x]--;
            emit(ev, EV_HIT, x, y, 0, 0, b->bsp[y][x]);
            continue;
        }
        kill_bug(b, h, x, y, cause, ev);
    }
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (h->revert[y][x] && b->sour[y][x] && !near_grown_sour(b, x, y)) {
                b->pods[y][x] = b->sour[y][x];
                b->sour[y][x] = 0;
                emit(ev, EV_POD, x, y, 0, 0, b->pods[y][x]);
            }
    memset(h->revert, 0, sizeof h->revert);
    check_drones(b, ev);
    if (b->status == ST_PLAYING && b->kills >= b->quota) b->status = ST_WON;
}

static void resolve1(Board *b, int x, int y, int cause, Events *ev) {
    HitQ h;
    hitq_init(&h);
    push_hit(&h, x, y, cause);
    run_hits(b, &h, ev);
}

static void add_pod(Board *b, int x, int y, Events *ev) {
    if (!inb(x, y) || b->hole[y][x]) return;
    b->pods[y][x]++;
    emit(ev, EV_POD, x, y, 0, 0, b->pods[y][x]);
    if (b->pods[y][x] >= 3) {
        /* overload: the tile blows open into a sinkhole */
        int elev = b->elev[y][x];
        b->pods[y][x] = 0;
        b->sour[y][x] = 0;
        b->hole[y][x] = 1;
        b->elev[y][x] = 0;
        emit(ev, EV_HOLE, x, y, 0, 0, 0);
        HitQ h;
        hitq_init(&h);
        h.boomed[y][x] = 1;
        if (b->bsp[y][x]) push_hit(&h, x, y, CAUSE_HOLE);
        if (is_player(b, x, y)) { b->status = ST_DEAD; emit(ev, EV_DIE, x, y, 0, 0, 0); }
        emit(ev, EV_BOOM, x, y, 0, 0, 1);
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int nx = x + dx, ny = y + dy;
                if ((dx || dy) && inb(nx, ny) && !b->hole[ny][nx] && b->elev[ny][nx] == elev) push_hit(&h, nx, ny, CAUSE_BOOM);
            }
        run_hits(b, &h, ev);
    }
}

static void collect(Board *b, int x, int y, Events *ev) {
    if (b->pods[y][x]) {
        b->energy = (uint16_t)(b->energy + b->pods[y][x]);
        emit(ev, EV_POD, x, y, 0, 0, 0);
        b->pods[y][x] = 0;
    }
    if (b->sour[y][x]) {
        b->energy = (uint16_t)imax(0, (int)b->energy - b->sour[y][x]);
        emit(ev, EV_SOUR, x, y, 0, 0, 0);
        b->sour[y][x] = 0;
    }
}

/* A grub lands on a tile after a shove (or a SHOO): a sprayed tile or a fizz
 * pod kills it (the pod is used up, no blast); sour pods make it grow a stage
 * for good, turn a drone into a larva, and hatch an egg. */
static void landed(Board *b, int x, int y, Events *ev) {
    if (!b->bsp[y][x]) return;
    if (b->spray[y][x]) { resolve1(b, x, y, CAUSE_SPRAY, ev); return; }
    if (b->pods[y][x]) {
        b->pods[y][x] = 0;
        emit(ev, EV_POD, x, y, 0, 0, 9);
        resolve1(b, x, y, CAUSE_POD, ev);
        return;
    }
    if (b->sour[y][x]) {
        b->sour[y][x] = 0;
        emit(ev, EV_SOUR, x, y, 0, 0, 0);
        if (b->bsp[y][x] == SP_DRONE) {
            place_bug(b, x, y, b->pair_sp[PAIR_GOLD], LV_LARVA);
            emit(ev, EV_GROW, x, y, 0, 0, 0);
        } else if (b->blv[y][x] == LV_EGG) {
            b->status = ST_HATCHED;
            emit(ev, EV_EGG, x, y, 0, 0, 255);
            return;
        } else {
            place_bug(b, x, y, b->bsp[y][x], b->blv[y][x] + 1);
            if (b->blv[y][x] <= LV_QUEEN) on_grow(b, x, y, ev);
            else emit(ev, EV_EGG, x, y, 0, 0, 253);
        }
    }
    sour_pass(b, ev);
}

/* ------------------------------------------------------------------ */
/* moving                                                               */

static void move_tilly(Board *b, int nx, int ny, int hop, Events *ev) {
    emit(ev, EV_MOVE, b->px, b->py, nx, ny, hop);
    b->px = (int8_t)nx;
    b->py = (int8_t)ny;
    collect(b, nx, ny, ev);
}

static void shove(Board *b, int x, int y, int bx, int by, Events *ev) {
    emit(ev, EV_PUSH, x, y, bx, by, b->bsp[y][x]);
    if (b->hole[by][bx]) {
        resolve1(b, x, y, CAUSE_HOLE, ev);
        return;
    }
    b->bsp[by][bx] = b->bsp[y][x];
    b->blv[by][bx] = b->blv[y][x];
    b->bhp[by][bx] = b->bhp[y][x];
    b->bsp[y][x] = SP_NONE;
    landed(b, bx, by, ev);
    check_drones(b, ev);
}

/* One tile of a rolling move. Rolls can't climb. Rolling down onto a grub
 * squashes it and the roll goes on; rolling into one at the same height
 * shoves it a tile ahead (not uphill, not into another grub or Tilly), and
 * the roll goes on, shoving again. Returns false if Tilly can't go on. */
static bool roll_step(Board *b, int dx, int dy, Events *ev) {
    int cx = b->px, cy = b->py, nx = cx + dx, ny = cy + dy;
    if (!inb(nx, ny) || b->hole[ny][nx]) return false;
    if (!(b->fx & FX_HOVER) && b->elev[ny][nx] > b->elev[cy][cx]) return false;
    if (b->bsp[ny][nx]) {
        bool shield = shielded(b, nx, ny);
        if ((b->fx & FX_OVERDRIVE) && !shield) {
            resolve1(b, nx, ny, CAUSE_TOUCH, ev);
        } else if (b->elev[cy][cx] > b->elev[ny][nx] && !shield) {
            resolve1(b, nx, ny, CAUSE_STOMP, ev);
        } else {
            int bx = nx + dx, by = ny + dy;
            if (!inb(bx, by)) return false;
            if (!b->hole[by][bx] && (b->bsp[by][bx] || is_player(b, bx, by) || b->elev[by][bx] > b->elev[ny][nx])) return false;
            shove(b, nx, ny, bx, by, ev);
        }
        if (b->bsp[ny][nx]) return false; /* still there somehow: stop */
    }
    move_tilly(b, nx, ny, 0, ev);
    return true;
}

static void do_roll(Board *b, int tx, int ty, Events *ev) {
    int dx = isign(tx - b->px), dy = isign(ty - b->py);
    int steps = imax(iabs(tx - b->px), iabs(ty - b->py));
    for (int k = 0; k < steps && b->status == ST_PLAYING; k++)
        if (!roll_step(b, dx, dy, ev)) break;
}

/* ------------------------------------------------------------------ */
/* targeting                                                            */

static bool is_roll(int chip) {
    return chip == CH_ROLL || chip == CH_SCURRY || chip == CH_STREAK || chip == CH_RUSH || chip == CH_HUSTLE;
}
static bool is_line_shot(int chip) {
    return chip == CH_ZAP || chip == CH_ARC || chip == CH_BEAM || chip == CH_FLARE || chip == CH_TRACK;
}
/* tools that go in straight lines (BOOST stretches these) */
static bool directional(int chip) {
    return is_roll(chip) || is_line_shot(chip) || chip == CH_SKIP || chip == CH_LEAP || chip == CH_TOSS || chip == CH_PITCH;
}
static int range_of(const Board *b, int chip) {
    if (chip == CH_STREAK || chip == CH_RUSH || chip == CH_BEAM || chip == CH_FLARE) return 99;
    return ((b->fx & FX_BOOST) && directional(chip)) ? 99 : 2;
}
static int dirs_of(const Board *b, int chip) {
    if (chip == CH_SCURRY || chip == CH_RUSH || chip == CH_LEAP || chip == CH_ARC || chip == CH_FLARE || chip == CH_PITCH) return 8;
    return ((b->fx & FX_BOOST) && directional(chip)) ? 8 : 4;
}

static bool hop_ok(const Board *b, int x, int y) {
    if (!inb(x, y) || b->hole[y][x] || is_player(b, x, y)) return false;
    return !b->bsp[y][x] || ((b->fx & FX_OVERDRIVE) && !shielded(b, x, y));
}

static bool near_hole(const Board *b, int x, int y) {
    for (int d = 0; d < 8; d++) {
        int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
        if (inb(nx, ny) && b->hole[ny][nx]) return true;
    }
    return false;
}

static bool free_tile(const Board *b, int x, int y) {
    return inb(x, y) && !b->hole[y][x] && !b->bsp[y][x] && !is_player(b, x, y);
}

void gs_targets(const Board *b, int chip, uint8_t out[GH][GW]) {
    memset(out, 0, GW * GH);
    int px = b->px, py = b->py;
    int r = range_of(b, chip);
    if (is_roll(chip)) {
        /* play the roll out on a copy: every tile it can end on is a target */
        for (int d = 0; d < dirs_of(b, chip); d++) {
            Board t = *b;
            for (int k = 1; k <= r; k++) {
                if (!roll_step(&t, DIR8[d][0], DIR8[d][1], NULL) || t.status != ST_PLAYING) break;
                out[t.py][t.px] = 1;
            }
        }
        return;
    }
    if (is_line_shot(chip)) {
        bool sight = (b->fx & FX_SIGHT) != 0;
        for (int d = 0; d < dirs_of(b, chip); d++)
            for (int k = 1; k <= r; k++) {
                int x = px + DIR8[d][0] * k, y = py + DIR8[d][1] * k;
                if (!inb(x, y) || (!sight && b->elev[y][x] > b->elev[py][px])) break;
                out[y][x] = 1;
            }
        return;
    }
    switch (chip) {
    case CH_SKIP: case CH_LEAP:
        for (int d = 0; d < dirs_of(b, chip); d++)
            for (int k = 1; k <= r; k++)
                if (hop_ok(b, px + DIR8[d][0] * k, py + DIR8[d][1] * k)) out[py + DIR8[d][1] * k][px + DIR8[d][0] * k] = 1;
        break;
    case CH_BOUND:
        for (int y = py - 2; y <= py + 2; y++)
            for (int x = px - 2; x <= px + 2; x++)
                if (hop_ok(b, x, y)) out[y][x] = 1;
        break;
    case CH_BLINK:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if ((b->pods[y][x] || b->sour[y][x]) && hop_ok(b, x, y)) out[y][x] = 1;
        break;
    case CH_PERCH:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->elev[y][x] && hop_ok(b, x, y)) out[y][x] = 1;
        break;
    case CH_DIVE:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (near_hole(b, x, y) && hop_ok(b, x, y)) out[y][x] = 1;
        break;
    case CH_TOSS: case CH_PITCH:
        for (int d = 0; d < dirs_of(b, chip); d++)
            for (int k = 1; k <= r; k++) {
                int x = px + DIR8[d][0] * k, y = py + DIR8[d][1] * k;
                if (inb(x, y)) out[y][x] = 1;
            }
        break;
    case CH_MORTAR:
        for (int y = py - 2; y <= py + 2; y++)
            for (int x = px - 2; x <= px + 2; x++)
                if (inb(x, y) && !(x == px && y == py)) out[y][x] = 1;
        break;
    case CH_SEED:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (free_tile(b, x, y) && !b->sour[y][x]) out[y][x] = 1;
        break;
    case CH_REWIND:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->bsp[y][x] && b->bsp[y][x] != SP_DRONE) out[y][x] = 1;
        break;
    case CH_IGNITE:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->pods[y][x]) out[y][x] = 1;
        break;
    case CH_QUAKE:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->hole[y][x]) out[y][x] = 1;
        break;
    case CH_CRACK:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->bsp[y][x] && b->blv[y][x] == LV_EGG) out[y][x] = 1;
        break;
    case CH_TILL:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->elev[y][x] && !b->hole[y][x]) out[y][x] = 1;
        break;
    case CH_GATHER:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->pods[y][x] || b->sour[y][x]) out[y][x] = 1;
        break;
    case CH_BORE:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (!b->hole[y][x] && !is_player(b, x, y) && near_hole(b, x, y)) out[y][x] = 1;
        break;
    case CH_MIST:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (free_tile(b, x, y) && !b->spray[y][x]) out[y][x] = 1;
        break;
    case CH_PULSE: case CH_HAIL: case CH_OVERDRIVE: case CH_RELOAD: case CH_REFUEL: case CH_BOOST: case CH_VOLATILE:
    case CH_RESTOCK: case CH_JUMPER: case CH_FLIP: case CH_HOVER: case CH_SIGHT: case CH_SHOO:
        out[py][px] = 1; /* confirm on Tilly herself */
        break;
    }
}

void gs_targets2(const Board *b, int chip, int fx, int fy, uint8_t out[GH][GW]) {
    memset(out, 0, GW * GH);
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            if (x == fx && y == fy) continue;
            if (chip == CH_TILL && !b->hole[y][x] && !b->elev[y][x]) out[y][x] = 1;
            if (chip == CH_MIST && free_tile(b, x, y) && !b->spray[y][x]) out[y][x] = 1;
        }
}

/* ------------------------------------------------------------------ */
/* actions                                                              */

static void hop_to(Board *b, int tx, int ty, Events *ev) {
    if (b->bsp[ty][tx] && (b->fx & FX_OVERDRIVE)) resolve1(b, tx, ty, CAUSE_TOUCH, ev);
    move_tilly(b, tx, ty, 1, ev);
}

static void shoot_line(Board *b, int tx, int ty, Events *ev) {
    int dx = isign(tx - b->px), dy = isign(ty - b->py);
    int steps = imax(iabs(tx - b->px), iabs(ty - b->py));
    HitQ h;
    hitq_init(&h);
    bool sight = (b->fx & FX_SIGHT) != 0;
    for (int k = 1; k <= steps; k++) {
        int x = b->px + dx * k, y = b->py + dy * k;
        if (!inb(x, y) || (!sight && b->elev[y][x] > b->elev[b->py][b->px])) break;
        push_hit(&h, x, y, CAUSE_ATTACK);
    }
    emit(ev, EV_SHOT, b->px, b->py, tx, ty, 0);
    run_hits(b, &h, ev);
}

static void refresh_kind(Board *b, int kind, int except, Events *ev) {
    for (int i = 0; i < SLOTS; i++)
        if (i != except && b->chips[i] != TOOL_NONE && CHIPS[b->chips[i]].kind == kind && b->spent[i]) {
            b->spent[i] = 0;
            emit(ev, EV_REFRESH, i, 0, 0, 0, 0);
        }
}

static void do_shoo(Board *b, Events *ev) {
    int order[GW * GH], n = 0;
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (b->bsp[y][x] && b->bsp[y][x] != SP_DRONE) order[n++] = y * GW + x;
    for (int i = n - 1; i > 0; i--) {
        int j = rng_range(&b->rng, 0, i);
        int t = order[i]; order[i] = order[j]; order[j] = t;
    }
    uint8_t moved[GH][GW];
    memset(moved, 0, sizeof moved);
    for (int i = 0; i < n && b->status == ST_PLAYING; i++) {
        int x = order[i] % GW, y = order[i] / GW;
        if (!b->bsp[y][x] || moved[y][x]) continue;
        /* any neighbour, diagonals too, that no grub or Tilly is on:
         * a pit or a pod there is the end of it */
        int opts[8], m = 0;
        for (int d = 0; d < 8; d++) {
            int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
            if (inb(nx, ny) && !b->bsp[ny][nx] && !is_player(b, nx, ny)) opts[m++] = d;
        }
        if (m == 0) continue;
        int d = opts[rng_range(&b->rng, 0, m - 1)];
        int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
        moved[ny][nx] = 1;
        shove(b, x, y, nx, ny, ev);
    }
    check_drones(b, ev);
}

bool gs_apply(Board *b, int slot, int tx, int ty, int tx2, int ty2, Events *ev) {
    if (b->status != ST_PLAYING || slot < 0 || slot >= SLOTS || b->spent[slot]) return false;
    int chip = b->chips[slot];
    if (chip == TOOL_NONE) return false;
    uint8_t valid[GH][GW];
    gs_targets(b, chip, valid);
    if (!inb(tx, ty) || !valid[ty][tx]) return false;
    if (CHIPS[chip].two_step) {
        gs_targets2(b, chip, tx, ty, valid);
        if (!inb(tx2, ty2) || !valid[ty2][tx2]) return false;
    }
    b->spent[slot] = 1;
    int kills0 = b->kills, elev0 = b->elev[b->py][b->px];
    if (is_roll(chip)) {
        do_roll(b, tx, ty, ev);
    } else if (is_line_shot(chip)) {
        shoot_line(b, tx, ty, ev);
    } else {
        switch (chip) {
        case CH_SKIP: case CH_LEAP: case CH_BOUND: case CH_BLINK: case CH_PERCH: case CH_DIVE:
            hop_to(b, tx, ty, ev);
            break;
        case CH_TOSS: case CH_PITCH: case CH_MORTAR: case CH_IGNITE:
            emit(ev, EV_SHOT, b->px, b->py, tx, ty, 1);
            resolve1(b, tx, ty, CAUSE_ATTACK, ev);
            break;
        case CH_PULSE: case CH_QUAKE: {
            HitQ h;
            hitq_init(&h);
            int cx = chip == CH_PULSE ? b->px : tx, cy = chip == CH_PULSE ? b->py : ty;
            for (int d = 0; d < 8; d++) push_hit(&h, cx + DIR8[d][0], cy + DIR8[d][1], CAUSE_ATTACK);
            emit(ev, EV_SHOT, b->px, b->py, cx, cy, 2);
            run_hits(b, &h, ev);
            break;
        }
        case CH_HAIL: {
            /* every planter, Tilly's own included */
            HitQ h;
            hitq_init(&h);
            for (int y = 0; y < GH; y++)
                for (int x = 0; x < GW; x++)
                    if (b->elev[y][x]) push_hit(&h, x, y, CAUSE_ATTACK);
            emit(ev, EV_SHOT, b->px, b->py, b->px, b->py, 2);
            run_hits(b, &h, ev);
            break;
        }
        case CH_CRACK: {
            HitQ h;
            hitq_init(&h);
            emit(ev, EV_EGG, tx, ty, 0, 0, b->bsp[ty][tx]);
            b->bsp[ty][tx] = SP_NONE;
            blast(b, &h, tx, ty, b->elev[ty][tx], ev);
            run_hits(b, &h, ev);
            break;
        }
        case CH_SEED: add_pod(b, tx, ty, ev); break;
        case CH_TILL:
            b->elev[ty][tx] = 0;
            b->elev[ty2][tx2] = 1;
            emit(ev, EV_RAISE, tx, ty, 0, 0, 0);
            emit(ev, EV_RAISE, tx2, ty2, 0, 0, 1);
            break;
        case CH_OVERDRIVE: b->fx |= FX_OVERDRIVE; emit(ev, EV_RAISE, b->px, b->py, 0, 0, 2); break;
        case CH_BOOST: b->fx |= FX_BOOST; emit(ev, EV_RAISE, b->px, b->py, 0, 0, 2); break;
        case CH_VOLATILE: b->fx |= FX_VOLATILE; emit(ev, EV_RAISE, b->px, b->py, 0, 0, 2); break;
        case CH_HOVER: b->fx |= FX_HOVER; emit(ev, EV_RAISE, b->px, b->py, 0, 0, 2); break;
        case CH_SIGHT: b->fx |= FX_SIGHT; emit(ev, EV_RAISE, b->px, b->py, 0, 0, 2); break;
        case CH_GATHER:
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    if (inb(tx + dx, ty + dy)) collect(b, tx + dx, ty + dy, ev);
            break;
        case CH_RELOAD: refresh_kind(b, KIND_ATTACK, slot, ev); break;
        case CH_REFUEL: refresh_kind(b, KIND_MOVE, slot, ev); break;
        case CH_JUMPER:
            for (int s = slot - 1; s <= slot + 1; s += 2)
                if (s >= 0 && s < SLOTS && b->spent[s]) { b->spent[s] = 0; emit(ev, EV_REFRESH, s, 0, 0, 0, 0); }
            break;
        case CH_BORE:
            b->pods[ty][tx] = 0;
            b->sour[ty][tx] = 0;
            b->hole[ty][tx] = 1;
            b->elev[ty][tx] = 0;
            emit(ev, EV_HOLE, tx, ty, 0, 0, 0);
            if (b->bsp[ty][tx]) resolve1(b, tx, ty, CAUSE_HOLE, ev);
            check_drones(b, ev);
            break;
        case CH_REWIND:
            for (int y = ty - 1; y <= ty + 1; y++)
                for (int x = tx - 1; x <= tx + 1; x++)
                    if (inb(x, y) && b->bsp[y][x] && b->bsp[y][x] != SP_DRONE) {
                        place_bug(b, x, y, b->bsp[y][x], LV_LARVA);
                        emit(ev, EV_HIT, x, y, 0, 0, b->bsp[y][x]);
                    }
            check_drones(b, ev);
            break;
        case CH_MIST:
            b->spray[ty][tx] = 1;
            b->spray[ty2][tx2] = 1;
            emit(ev, EV_SPRAY, tx, ty, 0, 0, 0);
            emit(ev, EV_SPRAY, tx2, ty2, 0, 0, 0);
            break;
        case CH_RESTOCK: drop_pods(b, 4, false, ev); break;
        case CH_FLIP:
            for (int y = 0; y < GH; y++)
                for (int x = 0; x < GW; x++)
                    if (!b->hole[y][x]) b->elev[y][x] ^= 1;
            emit(ev, EV_RAISE, b->px, b->py, 0, 0, 1);
            break;
        case CH_SHOO: do_shoo(b, ev); break;
        }
    }
    sour_pass(b, ev);
    check_drones(b, ev);
    /* HUSTLE recharges when a grub dies, TRACK when Tilly changes height */
    for (int i = 0; i < SLOTS; i++) {
        if (!b->spent[i]) continue;
        if ((b->chips[i] == CH_HUSTLE && b->kills > kills0) ||
            (b->chips[i] == CH_TRACK && b->elev[b->py][b->px] != elev0)) {
            b->spent[i] = 0;
            emit(ev, EV_REFRESH, i, 0, 0, 0, 0);
        }
    }
    if (b->status == ST_PLAYING && b->kills >= b->quota) b->status = ST_WON;
    return true;
}

/* Buying overwrites a slot and the new tool works at once; the shop fills the
 * gap with another tool of the same price. */
bool gs_buy(Board *b, int offer, int slot, Events *ev) {
    if (b->status != ST_PLAYING || offer < 0 || offer >= OFFERS || slot < 0 || slot >= SLOTS) return false;
    int chip = b->shop[offer];
    if (chip == TOOL_NONE || b->energy < CHIPS[chip].cost) return false;
    b->energy = (uint16_t)(b->energy - CHIPS[chip].cost);
    b->chips[slot] = (uint8_t)chip;
    b->spent[slot] = 0;
    b->shop[offer] = TOOL_NONE;
    refill_offer(b, offer, chip);
    emit(ev, EV_BUY, slot, 0, 0, 0, chip);
    return true;
}

/* The night, after the queens have settled into eggs: every grown grub ages
 * (adult to queen). Then one colour whose larvae are still alive grows up:
 * those larvae become adults and its new grubs hatch as adults from now on.
 * At most two colours ever grow in a contract; larvae of the others stay
 * harmless larvae. */
static void grow_night(Board *b, Events *ev) {
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (b->bsp[y][x] && b->bsp[y][x] != SP_DRONE && b->blv[y][x] == LV_ADULT) {
                place_bug(b, x, y, b->bsp[y][x], LV_QUEEN);
                emit(ev, EV_GROW, x, y, 0, 0, b->bsp[y][x]);
            }
    int cand[PAIR_COUNT], n = 0, grown_types = 0;
    for (int c = 0; c < PAIR_COUNT; c++) grown_types += (b->grown_mask >> c) & 1;
    for (int c = 0; c < PAIR_COUNT && grown_types < 2; c++) {
        if ((b->grown_mask >> c) & 1) continue;
        bool alive = false;
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                alive |= b->bsp[y][x] && b->bsp[y][x] != SP_DRONE && b->blv[y][x] == LV_LARVA && gs_pair_of(b->bsp[y][x]) == c;
        if (alive) cand[n++] = c;
    }
    if (n > 0) {
        int c = cand[rng_range(&b->rng, 0, n - 1)];
        b->stage[c] = LV_ADULT;
        b->grown_mask |= (uint8_t)(1 << c);
    }
    /* larvae of a grown colour grow up (tonight's, or ones sent back by REWIND) */
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            int sp = b->bsp[y][x];
            if (!sp || sp == SP_DRONE || b->blv[y][x] != LV_LARVA || !((b->grown_mask >> gs_pair_of(sp)) & 1)) continue;
            place_bug(b, x, y, sp, LV_ADULT);
            on_grow(b, x, y, ev);
        }
}

void gs_rest(Board *b, Events *ev) {
    if (b->status != ST_PLAYING) return;
    /* an unbroken egg hatches: the dome is overrun */
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (b->bsp[y][x] && b->blv[y][x] == LV_EGG) {
                b->status = ST_HATCHED;
                emit(ev, EV_EGG, x, y, 0, 0, 255);
                return;
            }
    if (b->day >= b->days) {
        b->status = b->kills >= b->quota ? ST_WON : ST_MISSED;
        return;
    }
    /* a field misted from wall to wall leaves nowhere to hatch: the job is void */
    bool open = false;
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) open |= !b->spray[y][x];
    if (!open) { b->status = ST_MISSED; return; }
    /* the night: queens settle into eggs, then one colour grows */
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (b->bsp[y][x] && b->bsp[y][x] != SP_DRONE && b->blv[y][x] == LV_QUEEN) {
                b->blv[y][x] = LV_EGG;
                b->bhp[y][x] = 1;
                emit(ev, EV_EGG, x, y, 0, 0, 253);
            }
    check_drones(b, ev);
    grow_night(b, ev);
    /* the next morning: fresh ground, the moundmakers raise their planters,
     * pods fall and new grubs hatch */
    b->day++;
    b->fx = 0;
    memset(b->spent, 0, sizeof b->spent);
    deal_terrain(b, ev);
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (grown_sp(b, x, y, SP_MOUND)) mound_turn(b, x, y, ev);
    drop_pods(b, b->pods_n, false, ev);
    if (b->status == ST_DEAD) return;
    spawn_bugs(b, b->spawn_n, ev);
    sour_pass(b, ev);
    check_drones(b, ev);
    if (b->status == ST_PLAYING && b->kills >= b->quota) b->status = ST_WON;
    roll_shop(b);
}

/* ------------------------------------------------------------------ */
/* autoplay bot (tests / balance)                                       */

static int chip_value(int chip) {
    switch (chip) {
    case CH_FLARE: case CH_MORTAR: return 9;
    case CH_PULSE: case CH_VOLATILE: return 8;
    case CH_BEAM: case CH_ARC: case CH_PITCH: case CH_RELOAD: return 7;
    case CH_IGNITE: case CH_HUSTLE: case CH_TRACK: case CH_BOOST: return 6;
    case CH_ZAP: case CH_TOSS: case CH_RUSH: case CH_QUAKE: case CH_HAIL: return 5;
    case CH_LEAP: case CH_BOUND: case CH_SCURRY: case CH_REFUEL: case CH_OVERDRIVE: return 4;
    case CH_SKIP: case CH_STREAK: case CH_BLINK: case CH_CRACK: case CH_BORE: case CH_REWIND: return 3;
    case CH_ROLL: return 2;
    default: return 1;
    }
}

static int evaluate(const Board *b) {
    if (b->status == ST_DEAD || b->status == ST_HATCHED) return -1000000;
    if (b->status == ST_WON) return 1000000;
    int s = b->kills * 1000 + b->energy * 60;
    int nearest = 99, near_egg = 99, near_queen = 99;
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            if (!b->bsp[y][x] || b->bsp[y][x] == SP_DRONE) continue;
            int lv = b->blv[y][x];
            s -= lv == LV_EGG ? 3000 : lv == LV_QUEEN ? 600 : lv == LV_ADULT ? 150 : 40;
            int d = iabs(x - b->px) + iabs(y - b->py);
            if (d < nearest) nearest = d;
            if (lv == LV_EGG && d < near_egg) near_egg = d;
            if (lv == LV_QUEEN && d < near_queen) near_queen = d;
        }
    if (nearest < 99) s -= nearest * 25;
    if (near_egg < 99) s -= near_egg * 150;
    if (near_queen < 99) s -= near_queen * 40;
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) s -= b->sour[y][x] * 50;
    return s;
}

static void bot_shop(Board *b) {
    for (int guard = 0; guard < 8; guard++) {
        int best_o = -1, best_v = 0;
        for (int o = 0; o < OFFERS; o++) {
            int c = b->shop[o];
            if (c == TOOL_NONE || CHIPS[c].cost > b->energy || CHIPS[c].two_step) continue;
            if (chip_value(c) > best_v) { best_v = chip_value(c); best_o = o; }
        }
        if (best_o < 0) return;
        int worst_s = -1, worst_v = 99, rolls = 0;
        for (int s = 0; s < SLOTS; s++) rolls += b->chips[s] == CH_ROLL;
        for (int s = 0; s < SLOTS; s++) {
            int v = chip_value(b->chips[s]);
            if (b->chips[s] == CH_ROLL && rolls <= 2) v = 5;
            if (v < worst_v) { worst_v = v; worst_s = s; }
        }
        if (worst_s < 0 || best_v <= worst_v) return;
        gs_buy(b, best_o, worst_s, NULL);
    }
}

typedef struct { int v; int8_t s, x, y, x2, y2; } BotMove;

/* Every legal action with its score one move ahead. With eggs_only, only
 * actions that break an egg. */
static int bot_moves(const Board *b, BotMove *out, int max, bool eggs_only) {
    int n = 0, eggs0 = gs_count_bugs(b, LV_EGG);
    Board tmp;
    uint8_t valid[GH][GW], valid2[GH][GW];
    for (int s = 0; s < SLOTS; s++) {
        if (b->spent[s] || b->chips[s] == TOOL_NONE) continue;
        int chip = b->chips[s];
        gs_targets(b, chip, valid);
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++) {
                if (!valid[y][x]) continue;
                int n2 = 1;
                if (CHIPS[chip].two_step) {
                    gs_targets2(b, chip, x, y, valid2);
                    n2 = GW * GH;
                }
                for (int k = 0; k < n2 && n < max; k++) {
                    int x2 = k % GW, y2 = k / GW;
                    if (CHIPS[chip].two_step && !valid2[y2][x2]) continue;
                    tmp = *b;
                    if (!gs_apply(&tmp, s, x, y, x2, y2, NULL)) continue;
                    if (eggs_only && gs_count_bugs(&tmp, LV_EGG) >= eggs0) continue;
                    out[n++] = (BotMove){evaluate(&tmp) + chip_value(chip) * -3, (int8_t)s, (int8_t)x, (int8_t)y, (int8_t)x2, (int8_t)y2};
                }
            }
    }
    return n;
}

/* One greedy action, looking two moves ahead from its five best first
 * moves. Eggs first, like any sensible hunter: if some action breaks an egg,
 * the choice is made among those. */
bool gs_bot_turn(Board *b, Events *ev) {
    if (b->status != ST_PLAYING) return false;
    static BotMove mv[1200], mv2[1200];
    int base = evaluate(b);
    int n = 0;
    if (gs_count_bugs(b, LV_EGG) > 0) n = bot_moves(b, mv, 1200, true);
    if (n == 0) n = bot_moves(b, mv, 1200, false);
    if (n == 0) return false;
    /* the five best first moves */
    for (int i = 0; i < n && i < 5; i++)
        for (int j = i + 1; j < n; j++)
            if (mv[j].v > mv[i].v) { BotMove t = mv[i]; mv[i] = mv[j]; mv[j] = t; }
    int best = base, bi = -1;
    for (int i = 0; i < n && i < 5; i++) {
        Board t = *b;
        gs_apply(&t, mv[i].s, mv[i].x, mv[i].y, mv[i].x2, mv[i].y2, NULL);
        int v = mv[i].v;
        if (t.status == ST_PLAYING) {
            int m = bot_moves(&t, mv2, 1200, false);
            for (int k = 0; k < m; k++) v = imax(v, mv2[k].v);
        }
        if (v > best || (bi < 0 && mv[i].v > base)) { best = v; bi = i; }
    }
    if (bi < 0) return false;
    return gs_apply(b, mv[bi].s, mv[bi].x, mv[bi].y, mv[bi].x2, mv[bi].y2, ev);
}

void gs_bot_contract(Board *b) {
    for (int guard = 0; guard < 40 && b->status == ST_PLAYING; guard++) {
        bot_shop(b);
        for (int k = 0; k < 20 && gs_bot_turn(b, NULL); k++) {}
        if (b->status != ST_PLAYING) break;
        gs_rest(b, NULL);
    }
}
