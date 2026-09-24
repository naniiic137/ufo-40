/* GRUB SHIFT - the rules. Everything here is a pure function of Board, so the
 * autoplay bot (and the tests) can try moves on copies of the board.
 * The rules follow Bug Hunter (UFO 50 #2) as documented in
 * docs/games/02-grub-shift.md; names, art and wording are ours. */
#include "grubshift.h"

/*  name        kind          cost 2step  description (tiny font, ~80 chars) */
const ChipInfo CHIPS[TOOL_COUNT] = {
    {"ROLL", KIND_MOVE, 2, 0, "ROLL 1-2 TILES STRAIGHT. PUSHES GRUBS AND COLLECTS PODS. NOT UPHILL."},
    {"DASH", KIND_MOVE, 2, 0, "ROLL 1-2 TILES IN 8 DIRECTIONS. PUSHES GRUBS. NOT UPHILL."},
    {"STREAK", KIND_MOVE, 2, 0, "ROLL ANY DISTANCE STRAIGHT. PUSHES GRUBS. NOT UPHILL."},
    {"RUSH", KIND_MOVE, 4, 0, "ROLL ANY DISTANCE IN 8 DIRECTIONS. PUSHES GRUBS. NOT UPHILL."},
    {"SKIP", KIND_MOVE, 2, 0, "HOP 1-2 TILES STRAIGHT TO A FREE TILE. ANY HEIGHT. NO PUSH."},
    {"LEAP", KIND_MOVE, 2, 0, "HOP 1-2 TILES IN 8 DIRECTIONS TO A FREE TILE. ANY HEIGHT."},
    {"VAULT", KIND_MOVE, 4, 0, "HOP TO ANY FREE TILE WITHIN TWO."},
    {"WARP", KIND_MOVE, 3, 0, "BEAM ONTO ANY FIZZ POD AND COLLECT IT."},
    {"PERCH", KIND_MOVE, 3, 0, "BEAM ONTO ANY FREE PLANTER."},
    {"DIVE", KIND_MOVE, 3, 0, "BEAM ONTO ANY FREE TILE NEXT TO A SINKHOLE."},
    {"HUSTLE", KIND_MOVE, 4, 0, "ROLL 1-2 TILES STRAIGHT. RECHARGES WHENEVER A GRUB IS SQUASHED."},
    {"ZAP", KIND_ATTACK, 2, 0, "SHOOT 1-2 TILES STRAIGHT, HITTING THE WHOLE LINE. NOT UPHILL."},
    {"ARC", KIND_ATTACK, 2, 0, "SHOOT 1-2 TILES IN 8 DIRECTIONS, HITTING THE WHOLE LINE. NOT UPHILL."},
    {"BEAM", KIND_ATTACK, 2, 0, "SHOOT A WHOLE ROW OR COLUMN. BLOCKED BY HIGHER GROUND."},
    {"FLARE", KIND_ATTACK, 4, 0, "SHOOT ALL THE WAY IN 8 DIRECTIONS. BLOCKED BY HIGHER GROUND."},
    {"TOSS", KIND_ATTACK, 2, 0, "LOB AT ONE TILE UP TO 2 AWAY, STRAIGHT. ANY HEIGHT."},
    {"LOB", KIND_ATTACK, 2, 0, "LOB AT ONE TILE UP TO 2 AWAY IN 8 DIRECTIONS. ANY HEIGHT."},
    {"MORTAR", KIND_ATTACK, 4, 0, "LOB AT ANY TILE WITHIN TWO. ANY HEIGHT."},
    {"DETONATE", KIND_ATTACK, 4, 0, "SET OFF ANY FIZZ POD ON THE FIELD."},
    {"QUAKE", KIND_ATTACK, 4, 0, "HIT EVERY TILE AROUND ONE SINKHOLE, HIGH OR LOW."},
    {"PULSE", KIND_ATTACK, 3, 0, "HIT ALL 8 TILES AROUND TILLY AT ONCE."},
    {"HAIL", KIND_ATTACK, 4, 0, "HIT EVERY PLANTER. ONLY WORKS FROM THE GROUND."},
    {"CRACK", KIND_ATTACK, 3, 0, "BLOW UP ANY EGG. THE BLAST HITS THE TILES AROUND IT."},
    {"TRACK", KIND_ATTACK, 4, 0, "SHOOT 1-2 TILES STRAIGHT. RECHARGES WHEN TILLY CHANGES HEIGHT."},
    {"SEED", KIND_SPECIAL, 2, 0, "DROP A FIZZ POD WITHIN TWO. A THIRD POD ON A TILE GOES BOOM!"},
    {"SHIFT", KIND_SPECIAL, 2, 1, "LOWER ONE PLANTER, THEN RAISE ANOTHER TILE."},
    {"OVERDRIVE", KIND_SPECIAL, 4, 0, "UNTIL THE SHIFT ENDS, GRUBS TILLY TOUCHES ARE SQUASHED OUTRIGHT."},
    {"GATHER", KIND_SPECIAL, 2, 0, "COLLECT A POD AND EVERY POD AROUND IT."},
    {"RELOAD", KIND_SPECIAL, 4, 0, "RECHARGE EVERY USED ATTACK TOOL."},
    {"REFUEL", KIND_SPECIAL, 4, 0, "RECHARGE EVERY USED MOVE TOOL."},
    {"BOOST", KIND_SPECIAL, 4, 0, "UNTIL THE SHIFT ENDS, TOOLS REACH AS FAR AS THEY CAN."},
    {"DIG", KIND_SPECIAL, 4, 0, "OPEN A NEW SINKHOLE NEXT TO AN OLD ONE."},
    {"VOLATILE", KIND_SPECIAL, 4, 0, "UNTIL THE SHIFT ENDS, GRUBS POPPED BY ATTACKS EXPLODE."},
    {"DEVOLVE", KIND_SPECIAL, 3, 0, "TURN A GRUB AND ITS NEIGHBOURS BACK INTO LARVAE."},
    {"SPRAY", KIND_SPECIAL, 2, 1, "SPRAY TWO TILES. NO GRUB HATCHES THERE; ONE PUSHED THERE DIES."},
    {"RESTOCK", KIND_SPECIAL, 4, 0, "CALL DOWN FOUR NEW FIZZ PODS."},
    {"RECHARGE", KIND_SPECIAL, 4, 0, "RECHARGE THE TOOLS ON EITHER SIDE OF THIS ONE."},
    {"FLIP", KIND_SPECIAL, 2, 0, "SWAP HIGH AND LOW GROUND EVERYWHERE."},
    {"HOVER", KIND_SPECIAL, 2, 0, "UNTIL THE SHIFT ENDS, ROLLS IGNORE HEIGHT."},
    {"SIGHT", KIND_SPECIAL, 2, 0, "UNTIL THE SHIFT ENDS, SHOTS IGNORE HEIGHT."},
    {"SHOO", KIND_SPECIAL, 5, 0, "EVERY GRUB SCUTTLES TO A FREE TILE BESIDE IT."},
};

static const int8_t DIR8[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
static const uint8_t PAIR_A[PAIR_COUNT] = {SP_SPARK, SP_MOUND, SP_SHELL};
static const uint8_t PAIR_B[PAIR_COUNT] = {SP_HIVE, SP_SOUR, SP_BURROW};

enum { CAUSE_ATTACK, CAUSE_BOOM, CAUSE_SPARK, CAUSE_STOMP, CAUSE_HOLE, CAUSE_TOUCH, CAUSE_SPRAY };

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

int gs_days_for(int contract) { return contract <= 3 ? 10 : contract <= 6 ? 9 : 8; }

/* ------------------------------------------------------------------ */
/* setup                                                                */

static void roll_shop(Board *b) {
    int n = 0;
    uint8_t pool[TOOL_COUNT];
    /* five cheap offers, one mid, two dear (the dear ones include SHOO) */
    for (int tier = 0; tier < 3; tier++) {
        int want = tier == 0 ? 5 : tier == 1 ? 1 : 2;
        int np = 0;
        for (int c = 0; c < TOOL_COUNT; c++) {
            int cost = CHIPS[c].cost;
            if ((tier == 0 && cost == 2) || (tier == 1 && cost == 3) || (tier == 2 && cost >= 4)) pool[np++] = (uint8_t)c;
        }
        for (int i = 0; i < want && np > 0; i++) {
            int k = rng_range(&b->rng, 0, np - 1);
            b->shop[n++] = pool[k];
            pool[k] = pool[--np];
        }
    }
    while (n < OFFERS) b->shop[n++] = TOOL_NONE;
}

static bool spawnable(const Board *b, int x, int y) {
    return !b->hole[y][x] && !b->bsp[y][x] && !is_player(b, x, y) && !b->spray[y][x] && !b->pods[y][x] && !b->sour[y][x];
}

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

static bool drone_free(const Board *b, int x, int y) {
    return inb(x, y) && !b->hole[y][x] && !b->bsp[y][x] && !is_player(b, x, y);
}

static void convert_sour(Board *b, int x, int y, Events *ev) {
    /* a grown sourmite spoils the fizz pods beside it */
    for (int d = 0; d < 8; d++) {
        int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
        if (!inb(nx, ny) || !b->pods[ny][nx]) continue;
        b->sour[ny][nx] = (uint8_t)imin(2, b->sour[ny][nx] + b->pods[ny][nx]);
        b->pods[ny][nx] = 0;
        emit(ev, EV_SOUR, nx, ny, 0, 0, b->sour[ny][nx]);
    }
}

static bool hive_has_drone(const Board *b, int x, int y) {
    for (int d = 0; d < 8; d++) {
        int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
        if (inb(nx, ny) && b->bsp[ny][nx] == SP_DRONE) return true;
    }
    return false;
}

/* Abilities that happen when a grub grows up. */
static void on_grow(Board *b, int x, int y, Events *ev) {
    int sp = b->bsp[y][x], lv = b->blv[y][x];
    emit(ev, EV_GROW, x, y, 0, 0, sp);
    if (sp == SP_MOUND && lv == LV_ADULT) {
        /* raises its own planter and flattens the ones beside it (once) */
        b->elev[y][x] = 1;
        for (int d = 0; d < 4; d++) {
            int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
            if (inb(nx, ny) && b->elev[ny][nx]) { b->elev[ny][nx] = 0; emit(ev, EV_RAISE, nx, ny, 0, 0, 0); }
        }
        emit(ev, EV_RAISE, x, y, 0, 0, 1);
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
    /* two of the three colours grow this contract */
    int skip = rng_range(&b->rng, 0, 2), k = 0;
    for (int p = 0; p < PAIR_COUNT; p++)
        if (p != skip) b->evolvers[k++] = (uint8_t)p;
    if (rng_chance(&b->rng, 50)) { uint8_t t = b->evolvers[0]; b->evolvers[0] = b->evolvers[1]; b->evolvers[1] = t; }
    /* Tilly starts away from the walls */
    b->px = (int8_t)rng_range(&b->rng, 1, GW - 2);
    b->py = (int8_t)rng_range(&b->rng, 1, GH - 2);
    /* one or two sinkholes, never beside Tilly */
    int holes = rng_range(&b->rng, 1, 2);
    for (int placed = 0, tries = 0; placed < holes && tries < 200; tries++) {
        int x = rng_range(&b->rng, 0, GW - 1), y = rng_range(&b->rng, 0, GH - 1);
        if (b->hole[y][x] || (iabs(x - b->px) <= 1 && iabs(y - b->py) <= 1)) continue;
        b->hole[y][x] = 1;
        b->elev[y][x] = 0;
        placed++;
    }
    /* raised planters, grown in little clumps: 4 on the first contract up to 8 */
    int platforms = 4 + imin(contract - 1, 4);
    for (int placed = 0, tries = 0; placed < platforms && tries < 200; tries++) {
        int x = rng_range(&b->rng, 0, GW - 1), y = rng_range(&b->rng, 0, GH - 1);
        if (b->elev[y][x] || b->hole[y][x] || is_player(b, x, y)) continue;
        b->elev[y][x] = 1;
        placed++;
        int d = rng_range(&b->rng, 0, 3);
        int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
        if (placed < platforms && inb(nx, ny) && !b->elev[ny][nx] && !b->hole[ny][nx] && !is_player(b, nx, ny) && rng_chance(&b->rng, 60)) {
            b->elev[ny][nx] = 1;
            placed++;
        }
    }
    /* the opening grubs; late contracts start with grown ones */
    int x, y;
    for (int i = 0; i < 5; i++)
        if (random_tile(b, &x, &y, spawnable)) place_bug(b, x, y, b->pair_sp[rng_range(&b->rng, 0, 2)], LV_LARVA);
    int j = contract > 15 ? 13 + (contract - 13) % 3 : contract;
    int pre = j >= 13 ? 2 : j >= 10 ? 1 : 0;
    for (int i = 0; i < pre; i++)
        if (random_tile(b, &x, &y, spawnable)) {
            place_bug(b, x, y, b->pair_sp[b->evolvers[i % 2]], LV_ADULT);
            on_grow(b, x, y, NULL);
        }
    for (int i = 0; i < 3; i++)
        if (random_tile(b, &x, &y, spawnable)) b->pods[y][x]++;
    static const uint8_t kit[SLOTS] = {CH_ROLL, CH_ROLL, CH_ROLL, CH_ROLL, CH_SKIP, CH_ZAP, CH_TOSS};
    memcpy(b->chips, kit, SLOTS);
    roll_shop(b);
}

/* ------------------------------------------------------------------ */
/* targeting                                                            */

static int reach(const Board *b, int n) { return (b->fx & FX_BOOST) ? 99 : n; }

static int range_of(const Board *b, int chip) {
    switch (chip) {
    case CH_STREAK: case CH_RUSH: case CH_BEAM: case CH_FLARE: return 99;
    default: return reach(b, 2);
    }
}
static int dirs_of(int chip) {
    switch (chip) {
    case CH_DASH: case CH_RUSH: case CH_LEAP: case CH_ARC: case CH_FLARE: case CH_LOB: return 8;
    default: return 4;
    }
}
static bool is_roll(int chip) {
    return chip == CH_ROLL || chip == CH_DASH || chip == CH_STREAK || chip == CH_RUSH || chip == CH_HUSTLE;
}
static bool is_line_shot(int chip) {
    return chip == CH_ZAP || chip == CH_ARC || chip == CH_BEAM || chip == CH_FLARE || chip == CH_TRACK;
}

/* Can a rolling move enter tile (nx,ny) from (cx,cy)? Also reports whether
 * entering it ends the roll (a push or a stomp). */
static bool roll_step_ok(const Board *b, int cx, int cy, int dx, int dy, bool *ends) {
    int nx = cx + dx, ny = cy + dy;
    *ends = false;
    if (!inb(nx, ny) || b->hole[ny][nx]) return false;
    bool hover = (b->fx & FX_HOVER) != 0;
    if (!hover && b->elev[ny][nx] > b->elev[cy][cx]) return false;
    if (!b->bsp[ny][nx]) return true;
    if (b->fx & FX_OVERDRIVE) return true; /* touched grubs just pop */
    *ends = true;
    if (b->elev[cy][cx] > b->elev[ny][nx]) return true; /* stomp */
    int bx = nx + dx, by = ny + dy;
    if (!inb(bx, by)) return false;
    if (b->hole[by][bx]) return true;
    if (b->bsp[by][bx] || is_player(b, bx, by)) return false;
    if (b->elev[by][bx] > b->elev[ny][nx]) return false;
    return true;
}

static bool hop_ok(const Board *b, int x, int y) {
    if (!inb(x, y) || b->hole[y][x] || is_player(b, x, y)) return false;
    return !b->bsp[y][x] || (b->fx & FX_OVERDRIVE);
}

static bool near_hole(const Board *b, int x, int y) {
    for (int d = 0; d < 8; d++) {
        int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
        if (inb(nx, ny) && b->hole[ny][nx]) return true;
    }
    return false;
}

void gs_targets(const Board *b, int chip, uint8_t out[GH][GW]) {
    memset(out, 0, GW * GH);
    int px = b->px, py = b->py;
    int r = range_of(b, chip);
    if (is_roll(chip)) {
        for (int d = 0; d < dirs_of(chip); d++) {
            int cx = px, cy = py;
            for (int k = 1; k <= r; k++) {
                bool ends;
                if (!roll_step_ok(b, cx, cy, DIR8[d][0], DIR8[d][1], &ends)) break;
                cx += DIR8[d][0];
                cy += DIR8[d][1];
                out[cy][cx] = 1;
                if (ends) break;
            }
        }
        return;
    }
    if (is_line_shot(chip)) {
        bool sight = (b->fx & FX_SIGHT) != 0;
        for (int d = 0; d < dirs_of(chip); d++)
            for (int k = 1; k <= r; k++) {
                int x = px + DIR8[d][0] * k, y = py + DIR8[d][1] * k;
                if (!inb(x, y) || (!sight && b->elev[y][x] > b->elev[py][px])) break;
                out[y][x] = 1;
            }
        return;
    }
    switch (chip) {
    case CH_SKIP: case CH_LEAP:
        for (int d = 0; d < dirs_of(chip); d++)
            for (int k = 1; k <= r; k++)
                if (hop_ok(b, px + DIR8[d][0] * k, py + DIR8[d][1] * k)) out[py + DIR8[d][1] * k][px + DIR8[d][0] * k] = 1;
        break;
    case CH_VAULT: {
        int rr = reach(b, 2);
        for (int y = py - rr; y <= py + rr; y++)
            for (int x = px - rr; x <= px + rr; x++)
                if (hop_ok(b, x, y)) out[y][x] = 1;
        break;
    }
    case CH_WARP:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->pods[y][x] && hop_ok(b, x, y)) out[y][x] = 1;
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
    case CH_TOSS: case CH_LOB:
        for (int d = 0; d < dirs_of(chip); d++)
            for (int k = 1; k <= r; k++) {
                int x = px + DIR8[d][0] * k, y = py + DIR8[d][1] * k;
                if (inb(x, y)) out[y][x] = 1;
            }
        break;
    case CH_MORTAR: case CH_SEED: {
        int rr = reach(b, 2);
        for (int y = py - rr; y <= py + rr; y++)
            for (int x = px - rr; x <= px + rr; x++)
                if (inb(x, y) && !(x == px && y == py) && !(chip == CH_SEED && b->hole[y][x])) out[y][x] = 1;
        break;
    }
    case CH_DEVOLVE: {
        int rr = reach(b, 2);
        for (int y = py - rr; y <= py + rr; y++)
            for (int x = px - rr; x <= px + rr; x++)
                if (inb(x, y) && b->bsp[y][x] && b->bsp[y][x] != SP_DRONE) out[y][x] = 1;
        break;
    }
    case CH_DETONATE:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->pods[y][x]) out[y][x] = 1;
        break;
    case CH_QUAKE:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->hole[y][x]) out[y][x] = 1;
        break;
    case CH_HAIL:
        if (b->elev[py][px] == 0) out[py][px] = 1;
        break;
    case CH_CRACK:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->bsp[y][x] && b->blv[y][x] == LV_EGG) out[y][x] = 1;
        break;
    case CH_SHIFT:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->elev[y][x] && !b->hole[y][x]) out[y][x] = 1;
        break;
    case CH_GATHER:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->pods[y][x] || b->sour[y][x]) out[y][x] = 1;
        break;
    case CH_DIG:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (!b->hole[y][x] && !is_player(b, x, y) && near_hole(b, x, y)) out[y][x] = 1;
        break;
    case CH_SPRAY:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (!b->hole[y][x] && !b->spray[y][x]) out[y][x] = 1;
        break;
    case CH_PULSE: case CH_OVERDRIVE: case CH_RELOAD: case CH_REFUEL: case CH_BOOST: case CH_VOLATILE:
    case CH_RESTOCK: case CH_RECHARGE: case CH_FLIP: case CH_HOVER: case CH_SIGHT: case CH_SHOO:
        out[py][px] = 1; /* confirm on Tilly herself */
        break;
    }
}

void gs_targets2(const Board *b, int chip, int fx, int fy, uint8_t out[GH][GW]) {
    memset(out, 0, GW * GH);
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            if (x == fx && y == fy) continue;
            if (chip == CH_SHIFT && !b->hole[y][x] && !b->elev[y][x]) out[y][x] = 1;
            if (chip == CH_SPRAY && !b->hole[y][x] && !b->spray[y][x]) out[y][x] = 1;
        }
}

/* ------------------------------------------------------------------ */
/* damage resolution                                                    */

typedef struct { int8_t x, y; uint8_t cause, elev; } Hit;
typedef struct { Hit q[400]; int qh, qt; uint8_t boomed[GH][GW]; } HitQ;

static void push_hit(HitQ *h, int x, int y, int cause, int elev) {
    if (h->qt < (int)(sizeof h->q / sizeof h->q[0])) h->q[h->qt++] = (Hit){(int8_t)x, (int8_t)y, (uint8_t)cause, (uint8_t)elev};
}

/* A blast on a tile: it and its 8 neighbours at the same height. */
static void blast(Board *b, HitQ *h, int x, int y, int elev, Events *ev) {
    emit(ev, EV_BOOM, x, y, 0, 0, 0);
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx, ny = y + dy;
            if (!inb(nx, ny)) continue;
            if (!(dx == 0 && dy == 0) && (b->hole[ny][nx] || b->elev[ny][nx] != elev)) continue;
            push_hit(h, nx, ny, CAUSE_BOOM, elev);
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
    if (!damaging(cause)) return; /* stomps, pits and sprays never trigger abilities */
    if (lv >= LV_ADULT) {
        if (sp == SP_SPARK) {
            for (int d = 0; d < 4; d++)
                for (int k = 1; k <= 2; k++) {
                    int sx = x + DIR8[d][0] * k, sy = y + DIR8[d][1] * k;
                    if (!inb(sx, sy)) break;
                    emit(ev, EV_SPARK, x, y, sx, sy, 0);
                    push_hit(h, sx, sy, CAUSE_SPARK, 0);
                }
        } else if (sp == SP_BURROW) {
            b->hole[y][x] = 1;
            b->pods[y][x] = 0;
            b->sour[y][x] = 0;
            b->spray[y][x] = 0;
            b->elev[y][x] = 0;
            emit(ev, EV_HOLE, x, y, 0, 0, 0);
        }
    }
    if ((b->fx & FX_VOLATILE) && cause != CAUSE_BOOM && !b->hole[y][x]) blast(b, h, x, y, b->elev[y][x], ev);
}

static void check_drones(Board *b, Events *ev) {
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            if (b->bsp[y][x] != SP_DRONE) continue;
            bool near = false;
            for (int d = 0; d < 8; d++) {
                int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
                if (inb(nx, ny) && b->bsp[ny][nx] == SP_HIVE && b->blv[ny][nx] >= LV_ADULT && b->blv[ny][nx] <= LV_QUEEN) near = true;
            }
            if (!near) { b->bsp[y][x] = SP_NONE; emit(ev, EV_KILL, x, y, 0, 0, SP_DRONE); }
        }
}

static void run_hits(Board *b, HitQ *h, Events *ev) {
    while (h->qh < h->qt) {
        Hit hit = h->q[h->qh++];
        int x = hit.x, y = hit.y, cause = hit.cause;
        if (!inb(x, y)) continue;
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
        int sp = b->bsp[y][x], lv = b->blv[y][x];
        bool grown = lv == LV_ADULT || lv == LV_QUEEN;
        if (damaging(cause)) {
            if (sp == SP_HIVE && grown && hive_has_drone(b, x, y)) { emit(ev, EV_HIT, x, y, 0, 0, sp); continue; }
            if (sp == SP_SOUR && grown && cause == CAUSE_BOOM) { emit(ev, EV_HIT, x, y, 0, 0, sp); continue; }
            if (b->bhp[y][x] > 1) {
                b->bhp[y][x]--;
                emit(ev, EV_HIT, x, y, 0, 0, sp);
                continue;
            }
        }
        kill_bug(b, h, x, y, cause, ev);
    }
    check_drones(b, ev);
    if (b->status == ST_PLAYING && b->kills >= b->quota) b->status = ST_WON;
}

static void resolve1(Board *b, int x, int y, int cause, Events *ev) {
    HitQ h;
    h.qh = h.qt = 0;
    memset(h.boomed, 0, sizeof h.boomed);
    push_hit(&h, x, y, cause, 0);
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
        b->spray[y][x] = 0;
        b->hole[y][x] = 1;
        b->elev[y][x] = 0;
        emit(ev, EV_HOLE, x, y, 0, 0, 0);
        HitQ h;
        h.qh = h.qt = 0;
        memset(h.boomed, 0, sizeof h.boomed);
        h.boomed[y][x] = 1;
        if (b->bsp[y][x]) push_hit(&h, x, y, CAUSE_HOLE, 0);
        if (is_player(b, x, y)) { b->status = ST_DEAD; emit(ev, EV_DIE, x, y, 0, 0, 0); }
        emit(ev, EV_BOOM, x, y, 0, 0, 1);
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int nx = x + dx, ny = y + dy;
                if ((dx || dy) && inb(nx, ny) && !b->hole[ny][nx] && b->elev[ny][nx] == elev) push_hit(&h, nx, ny, CAUSE_BOOM, elev);
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

/* A grub lands on a tile after a push (or a SHOO): sprays kill it, sour pods
 * make it grow a stage for good, and an egg shoved into sour pods hatches. */
static void landed(Board *b, int x, int y, Events *ev) {
    if (!b->bsp[y][x]) return;
    if (b->spray[y][x]) { resolve1(b, x, y, CAUSE_SPRAY, ev); return; }
    if (b->sour[y][x] && b->bsp[y][x] != SP_DRONE) {
        b->sour[y][x] = 0;
        if (b->blv[y][x] == LV_EGG) {
            b->status = ST_HATCHED;
            emit(ev, EV_EGG, x, y, 0, 0, 255);
            return;
        }
        place_bug(b, x, y, b->bsp[y][x], b->blv[y][x] + 1);
        if (b->blv[y][x] <= LV_QUEEN) on_grow(b, x, y, ev);
    }
}

/* ------------------------------------------------------------------ */
/* actions                                                              */

static void move_tilly(Board *b, int nx, int ny, int hop, Events *ev) {
    emit(ev, EV_MOVE, b->px, b->py, nx, ny, hop);
    b->px = (int8_t)nx;
    b->py = (int8_t)ny;
    collect(b, nx, ny, ev);
}

static void do_roll(Board *b, int tx, int ty, Events *ev) {
    int dx = isign(tx - b->px), dy = isign(ty - b->py);
    int steps = imax(iabs(tx - b->px), iabs(ty - b->py));
    for (int k = 0; k < steps && b->status == ST_PLAYING; k++) {
        bool ends;
        if (!roll_step_ok(b, b->px, b->py, dx, dy, &ends)) break;
        int nx = b->px + dx, ny = b->py + dy;
        if (b->bsp[ny][nx]) {
            if (b->fx & FX_OVERDRIVE) {
                resolve1(b, nx, ny, CAUSE_TOUCH, ev);
                move_tilly(b, nx, ny, 0, ev);
                continue;
            }
            if (b->elev[b->py][b->px] > b->elev[ny][nx]) {
                resolve1(b, nx, ny, CAUSE_STOMP, ev);
            } else {
                int bx = nx + dx, by = ny + dy;
                emit(ev, EV_PUSH, nx, ny, bx, by, b->bsp[ny][nx]);
                if (b->hole[by][bx]) {
                    resolve1(b, nx, ny, CAUSE_HOLE, ev);
                } else {
                    b->bsp[by][bx] = b->bsp[ny][nx];
                    b->blv[by][bx] = b->blv[ny][nx];
                    b->bhp[by][bx] = b->bhp[ny][nx];
                    b->bsp[ny][nx] = SP_NONE;
                    landed(b, bx, by, ev);
                    check_drones(b, ev);
                }
            }
            move_tilly(b, nx, ny, 0, ev);
            break;
        }
        move_tilly(b, nx, ny, 0, ev);
    }
}

static void hop_to(Board *b, int tx, int ty, Events *ev) {
    if (b->bsp[ty][tx] && (b->fx & FX_OVERDRIVE)) resolve1(b, tx, ty, CAUSE_TOUCH, ev);
    move_tilly(b, tx, ty, 1, ev);
}

static void shoot_line(Board *b, int tx, int ty, Events *ev) {
    int dx = isign(tx - b->px), dy = isign(ty - b->py);
    int steps = imax(iabs(tx - b->px), iabs(ty - b->py));
    HitQ h;
    h.qh = h.qt = 0;
    memset(h.boomed, 0, sizeof h.boomed);
    bool sight = (b->fx & FX_SIGHT) != 0;
    for (int k = 1; k <= steps; k++) {
        int x = b->px + dx * k, y = b->py + dy * k;
        if (!inb(x, y) || (!sight && b->elev[y][x] > b->elev[b->py][b->px])) break;
        push_hit(&h, x, y, CAUSE_ATTACK, 0);
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

static void drop_pods(Board *b, int n, Events *ev) {
    /* never onto Tilly, never a third on one tile */
    for (int i = 0; i < n; i++)
        for (int tries = 0; tries < 40; tries++) {
            int x = rng_range(&b->rng, 0, GW - 1), y = rng_range(&b->rng, 0, GH - 1);
            if (b->hole[y][x] || is_player(b, x, y) || b->pods[y][x] >= 2 || b->sour[y][x]) continue;
            b->pods[y][x]++;
            emit(ev, EV_POD, x, y, 0, 0, b->pods[y][x]);
            break;
        }
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (b->bsp[y][x] == SP_SOUR && b->blv[y][x] >= LV_ADULT && b->blv[y][x] <= LV_QUEEN) convert_sour(b, x, y, ev);
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
        int opts[4], m = 0;
        for (int d = 0; d < 4; d++) {
            int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
            if (inb(nx, ny) && !b->hole[ny][nx] && !b->bsp[ny][nx] && !is_player(b, nx, ny)) opts[m++] = d;
        }
        if (m == 0) continue;
        int d = opts[rng_range(&b->rng, 0, m - 1)];
        int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
        b->bsp[ny][nx] = b->bsp[y][x];
        b->blv[ny][nx] = b->blv[y][x];
        b->bhp[ny][nx] = b->bhp[y][x];
        b->bsp[y][x] = SP_NONE;
        moved[ny][nx] = 1;
        emit(ev, EV_PUSH, x, y, nx, ny, b->bsp[ny][nx]);
        landed(b, nx, ny, ev);
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
        case CH_SKIP: case CH_LEAP: case CH_VAULT: case CH_WARP: case CH_PERCH: case CH_DIVE:
            hop_to(b, tx, ty, ev);
            break;
        case CH_TOSS: case CH_LOB: case CH_MORTAR: case CH_DETONATE:
            emit(ev, EV_SHOT, b->px, b->py, tx, ty, 1);
            resolve1(b, tx, ty, CAUSE_ATTACK, ev);
            break;
        case CH_PULSE: case CH_QUAKE: {
            HitQ h;
            h.qh = h.qt = 0;
            memset(h.boomed, 0, sizeof h.boomed);
            int cx = chip == CH_PULSE ? b->px : tx, cy = chip == CH_PULSE ? b->py : ty;
            for (int d = 0; d < 8; d++) push_hit(&h, cx + DIR8[d][0], cy + DIR8[d][1], CAUSE_ATTACK, 0);
            emit(ev, EV_SHOT, b->px, b->py, cx, cy, 2);
            run_hits(b, &h, ev);
            break;
        }
        case CH_HAIL: {
            HitQ h;
            h.qh = h.qt = 0;
            memset(h.boomed, 0, sizeof h.boomed);
            for (int y = 0; y < GH; y++)
                for (int x = 0; x < GW; x++)
                    if (b->elev[y][x]) push_hit(&h, x, y, CAUSE_ATTACK, 1);
            emit(ev, EV_SHOT, b->px, b->py, b->px, b->py, 2);
            run_hits(b, &h, ev);
            break;
        }
        case CH_CRACK: {
            HitQ h;
            h.qh = h.qt = 0;
            memset(h.boomed, 0, sizeof h.boomed);
            emit(ev, EV_EGG, tx, ty, 0, 0, b->bsp[ty][tx]);
            b->bsp[ty][tx] = SP_NONE;
            blast(b, &h, tx, ty, b->elev[ty][tx], ev);
            run_hits(b, &h, ev);
            break;
        }
        case CH_SEED: add_pod(b, tx, ty, ev); break;
        case CH_SHIFT:
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
        case CH_RECHARGE:
            for (int s = slot - 1; s <= slot + 1; s += 2)
                if (s >= 0 && s < SLOTS && b->spent[s]) { b->spent[s] = 0; emit(ev, EV_REFRESH, s, 0, 0, 0, 0); }
            break;
        case CH_DIG:
            b->pods[ty][tx] = 0;
            b->sour[ty][tx] = 0;
            b->spray[ty][tx] = 0;
            b->hole[ty][tx] = 1;
            b->elev[ty][tx] = 0;
            emit(ev, EV_HOLE, tx, ty, 0, 0, 0);
            if (b->bsp[ty][tx]) resolve1(b, tx, ty, CAUSE_HOLE, ev);
            check_drones(b, ev);
            break;
        case CH_DEVOLVE:
            for (int y = ty - 1; y <= ty + 1; y++)
                for (int x = tx - 1; x <= tx + 1; x++)
                    if (inb(x, y) && b->bsp[y][x] && b->bsp[y][x] != SP_DRONE) {
                        place_bug(b, x, y, b->bsp[y][x], LV_LARVA);
                        emit(ev, EV_HIT, x, y, 0, 0, b->bsp[y][x]);
                    }
            check_drones(b, ev);
            break;
        case CH_SPRAY:
            b->spray[ty][tx] = 1;
            b->spray[ty2][tx2] = 1;
            emit(ev, EV_SPRAY, tx, ty, 0, 0, 0);
            emit(ev, EV_SPRAY, tx2, ty2, 0, 0, 0);
            break;
        case CH_RESTOCK: drop_pods(b, 4, ev); break;
        case CH_FLIP:
            for (int y = 0; y < GH; y++)
                for (int x = 0; x < GW; x++)
                    if (!b->hole[y][x]) b->elev[y][x] ^= 1;
            emit(ev, EV_RAISE, b->px, b->py, 0, 0, 1);
            break;
        case CH_SHOO: do_shoo(b, ev); break;
        }
    }
    /* HUSTLE recharges on a squash, TRACK when Tilly changes height */
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

bool gs_buy(Board *b, int offer, int slot, Events *ev) {
    if (offer < 0 || offer >= OFFERS || slot < 0 || slot >= SLOTS) return false;
    int chip = b->shop[offer];
    if (chip == TOOL_NONE || b->energy < CHIPS[chip].cost) return false;
    b->energy = (uint16_t)(b->energy - CHIPS[chip].cost);
    b->chips[slot] = (uint8_t)chip;
    b->spent[slot] = 0;
    b->shop[offer] = TOOL_NONE;
    emit(ev, EV_BUY, slot, 0, 0, 0, chip);
    return true;
}

static bool hole_ok(const Board *b, int x, int y) {
    return !b->hole[y][x] && !b->bsp[y][x] && !is_player(b, x, y) && !b->pods[y][x] && !b->sour[y][x];
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
    /* a field sprayed from wall to wall leaves nowhere to hatch: the job is void */
    bool open = false;
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) open |= !b->hole[y][x] && !b->spray[y][x];
    if (!open) { b->status = ST_MISSED; return; }
    /* queens settle down and lay eggs */
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (b->bsp[y][x] && b->bsp[y][x] != SP_DRONE && b->blv[y][x] == LV_QUEEN) {
                b->blv[y][x] = LV_EGG;
                b->bhp[y][x] = 1;
                emit(ev, EV_EGG, x, y, 0, 0, 253);
            }
    /* one colour grows tonight */
    int pair = b->evolvers[(b->day - 1) % 2];
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            int sp = b->bsp[y][x];
            if (!sp || sp == SP_DRONE || gs_pair_of(sp) != pair || b->blv[y][x] >= LV_QUEEN) continue;
            place_bug(b, x, y, sp, b->blv[y][x] + 1);
            on_grow(b, x, y, ev);
        }
    /* new larvae wriggle up */
    int x, y;
    for (int i = 0; i < b->spawn_n; i++)
        if (random_tile(b, &x, &y, spawnable)) {
            place_bug(b, x, y, b->pair_sp[rng_range(&b->rng, 0, 2)], LV_LARVA);
            emit(ev, EV_GROW, x, y, 0, 0, 0);
        }
    /* fizz pods rain down */
    drop_pods(b, b->pods_n, ev);
    /* and the ground gives way somewhere */
    if (random_tile(b, &x, &y, hole_ok)) {
        b->hole[y][x] = 1;
        b->elev[y][x] = 0;
        b->spray[y][x] = 0;
        emit(ev, EV_HOLE, x, y, 0, 0, 0);
    }
    check_drones(b, ev);
    b->day++;
    b->fx = 0;
    memset(b->spent, 0, sizeof b->spent);
    roll_shop(b);
}

/* ------------------------------------------------------------------ */
/* autoplay bot (tests / balance)                                       */

static int chip_value(int chip) {
    switch (chip) {
    case CH_FLARE: case CH_MORTAR: return 9;
    case CH_PULSE: case CH_VOLATILE: return 8;
    case CH_BEAM: case CH_ARC: case CH_LOB: case CH_RELOAD: return 7;
    case CH_DETONATE: case CH_HUSTLE: case CH_TRACK: case CH_BOOST: return 6;
    case CH_ZAP: case CH_TOSS: case CH_RUSH: case CH_QUAKE: case CH_HAIL: return 5;
    case CH_LEAP: case CH_VAULT: case CH_DASH: case CH_REFUEL: case CH_OVERDRIVE: return 4;
    case CH_SKIP: case CH_STREAK: case CH_WARP: case CH_CRACK: case CH_DIG: return 3;
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

bool gs_bot_turn(Board *b, Events *ev) {
    if (b->status != ST_PLAYING) return false;
    int base = evaluate(b);
    int best = base, bs = -1, bx = 0, by = 0, bx2 = 0, by2 = 0;
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
                for (int k = 0; k < n2; k++) {
                    int x2 = k % GW, y2 = k / GW;
                    if (CHIPS[chip].two_step && !valid2[y2][x2]) continue;
                    tmp = *b;
                    if (!gs_apply(&tmp, s, x, y, x2, y2, NULL)) continue;
                    int v = evaluate(&tmp) + chip_value(chip) * -3;
                    if (v > best) { best = v; bs = s; bx = x; by = y; bx2 = x2; by2 = y2; }
                }
            }
    }
    if (bs < 0) return false;
    return gs_apply(b, bs, bx, by, bx2, by2, ev);
}

void gs_bot_contract(Board *b) {
    for (int guard = 0; guard < 40 && b->status == ST_PLAYING; guard++) {
        bot_shop(b);
        for (int k = 0; k < 20 && gs_bot_turn(b, NULL); k++) {}
        if (b->status != ST_PLAYING) break;
        gs_rest(b, NULL);
    }
}
