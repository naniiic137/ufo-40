/* GRUB SHIFT - the rules. Everything here is a pure function of Board, so the
 * autoplay bot (and the tests) can try moves on copies of the board. */
#include "grubshift.h"

const ChipInfo CHIPS[TOOL_COUNT] = {
    {"ROLL", KIND_MOVE, 2, "ROLL 1-2 TILES STRAIGHT. PUSHES A GRUB AND STOPS. CAN'T ROLL UPHILL."},
    {"DASH", KIND_MOVE, 2, "ROLL 1-2 TILES IN 8 DIRECTIONS. PUSHES GRUBS. CAN'T ROLL UPHILL."},
    {"STREAK", KIND_MOVE, 2, "ROLL AS FAR AS YOU LIKE IN A STRAIGHT LINE. PUSHES GRUBS."},
    {"SKIP", KIND_MOVE, 2, "HOP 1-2 TILES STRAIGHT ONTO AN EMPTY TILE. ANY HEIGHT."},
    {"LEAP", KIND_MOVE, 2, "HOP 1-2 TILES IN 8 DIRECTIONS ONTO AN EMPTY TILE. ANY HEIGHT."},
    {"VAULT", KIND_MOVE, 4, "HOP TO ANY EMPTY TILE WITHIN TWO."},
    {"WARP", KIND_MOVE, 3, "BEAM ONTO ANY FIZZ POD AND COLLECT IT."},
    {"ZAP", KIND_ATTACK, 2, "SHOOT 1-2 TILES STRAIGHT, HITTING EVERY TILE IN THE LINE. NOT UPHILL."},
    {"ARC", KIND_ATTACK, 2, "SHOOT 1-2 TILES IN 8 DIRECTIONS, HITTING THE WHOLE LINE. NOT UPHILL."},
    {"BEAM", KIND_ATTACK, 2, "SHOOT A WHOLE ROW OR COLUMN. BLOCKED BY HIGHER GROUND."},
    {"TOSS", KIND_ATTACK, 2, "LOB AT ONE TILE UP TO 2 AWAY, STRAIGHT. ANY HEIGHT."},
    {"LOB", KIND_ATTACK, 2, "LOB AT ONE TILE UP TO 2 AWAY IN 8 DIRECTIONS. ANY HEIGHT."},
    {"MORTAR", KIND_ATTACK, 4, "LOB AT ANY TILE WITHIN TWO. ANY HEIGHT."},
    {"PULSE", KIND_ATTACK, 3, "HIT ALL 8 TILES AROUND TILLY AT ONCE."},
    {"DETONATE", KIND_ATTACK, 4, "SET OFF ANY FIZZ POD ON THE FIELD."},
    {"SEED", KIND_SPECIAL, 2, "DROP A FIZZ POD WITHIN TWO. A THIRD POD ON A TILE GOES BOOM!"},
    {"RAISE", KIND_SPECIAL, 2, "RAISE OR LOWER A TILE WITHIN TWO."},
    {"RECHARGE", KIND_SPECIAL, 4, "REFRESH EVERY USED ATTACK TOOL."},
    {"REBOOT", KIND_SPECIAL, 4, "REFRESH EVERY USED MOVE TOOL."},
    {"DEVOLVE", KIND_SPECIAL, 3, "TURN A GRUB AND ITS NEIGHBOURS BACK INTO LARVAE."},
};

static const int8_t DIR8[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};

enum { CAUSE_ATTACK, CAUSE_BOOM, CAUSE_SPARK, CAUSE_STOMP, CAUSE_HOLE };

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
            if (b->bsp[y][x] && (level < 0 || b->blv[y][x] == level)) n++;
    return n;
}

/* ------------------------------------------------------------------ */
/* setup                                                                */

static void roll_shop(Board *b) {
    int n = 0;
    uint8_t pool[TOOL_COUNT];
    for (int cost = 2; cost <= 4; cost++) {
        int want = cost == 2 ? 5 : cost == 3 ? 1 : 2;
        int np = 0;
        for (int c = 0; c < TOOL_COUNT; c++)
            if (CHIPS[c].cost == cost) pool[np++] = (uint8_t)c;
        for (int i = 0; i < want && np > 0; i++) {
            int k = rng_range(&b->rng, 0, np - 1);
            b->shop[n++] = pool[k];
            pool[k] = pool[--np];
        }
    }
    while (n < OFFERS) b->shop[n++] = TOOL_NONE;
}

static bool random_empty(Board *b, int *ox, int *oy, bool allow_pods) {
    for (int tries = 0; tries < 200; tries++) {
        int x = rng_range(&b->rng, 0, GW - 1), y = rng_range(&b->rng, 0, GH - 1);
        if (b->hole[y][x] || b->bsp[y][x] || is_player(b, x, y)) continue;
        if (!allow_pods && b->pods[y][x]) continue;
        *ox = x; *oy = y;
        return true;
    }
    return false;
}

static void place_bug(Board *b, int x, int y, int sp, int lv) {
    b->bsp[y][x] = (uint8_t)sp;
    b->blv[y][x] = (uint8_t)lv;
    b->bhp[y][x] = (uint8_t)((sp == SP_SHELL && lv >= LV_ADULT && lv <= LV_QUEEN) ? 2 : 1);
    if (sp == SP_MOUND && (lv == LV_ADULT || lv == LV_QUEEN)) b->elev[y][x] = 1;
}

void gs_new_contract(Board *b, int contract, uint64_t seed) {
    memset(b, 0, sizeof *b);
    rng_seed(&b->rng, seed);
    b->contract = (uint8_t)contract;
    b->days = (uint8_t)(contract <= 3 ? 10 : contract <= 5 ? 9 : 8);
    b->quota = (uint16_t)imin(18 + 2 * (contract - 1), 30);
    b->spawn_n = (uint8_t)(contract <= 2 ? 3 : 4);
    b->pods_n = 4;
    b->day = 1;
    b->status = ST_PLAYING;
    /* three species this contract, two of which evolve */
    int sp[4] = {SP_SPARK, SP_SHELL, SP_BURROW, SP_MOUND};
    for (int i = 3; i > 0; i--) {
        int j = rng_range(&b->rng, 0, i);
        int t = sp[i]; sp[i] = sp[j]; sp[j] = t;
    }
    for (int i = 0; i < 3; i++) b->species[i] = (uint8_t)sp[i];
    b->evolvers[0] = b->species[0];
    b->evolvers[1] = b->species[1];
    /* Tilly starts near the middle */
    b->px = (int8_t)rng_range(&b->rng, 2, 5);
    b->py = (int8_t)rng_range(&b->rng, 2, 3);
    /* raised planters, grown in little clumps */
    int platforms = 4 + imin(contract, 4);
    for (int placed = 0, tries = 0; placed < platforms && tries < 100; tries++) {
        int x = rng_range(&b->rng, 0, GW - 1), y = rng_range(&b->rng, 0, GH - 1);
        if (b->elev[y][x] || is_player(b, x, y)) continue;
        b->elev[y][x] = 1;
        placed++;
        int d = rng_range(&b->rng, 0, 3);
        int nx = x + DIR8[d][0], ny = y + DIR8[d][1];
        if (placed < platforms && inb(nx, ny) && !b->elev[ny][nx] && !is_player(b, nx, ny)) {
            b->elev[ny][nx] = 1;
            placed++;
        }
    }
    /* sinkholes, never next to Tilly */
    int holes = contract >= 3 ? 3 : 2;
    for (int placed = 0, tries = 0; placed < holes && tries < 100; tries++) {
        int x = rng_range(&b->rng, 0, GW - 1), y = rng_range(&b->rng, 0, GH - 1);
        if (b->hole[y][x] || iabs(x - b->px) + iabs(y - b->py) <= 1) continue;
        b->hole[y][x] = 1;
        b->elev[y][x] = 0;
        placed++;
    }
    /* opening grubs */
    int x, y;
    for (int i = 0; i < 4; i++)
        if (random_empty(b, &x, &y, false)) place_bug(b, x, y, b->species[i % 3], LV_LARVA);
    int pre = contract >= 2 ? imin(contract - 1, 3) : 0;
    for (int i = 0; i < pre; i++)
        if (random_empty(b, &x, &y, false)) place_bug(b, x, y, b->evolvers[i % 2], LV_ADULT);
    for (int i = 0; i < 3; i++)
        if (random_empty(b, &x, &y, false)) b->pods[y][x]++;
    static const uint8_t kit[SLOTS] = {CH_ROLL, CH_ROLL, CH_ROLL, CH_ROLL, CH_SKIP, CH_ZAP, CH_TOSS};
    memcpy(b->chips, kit, SLOTS);
    roll_shop(b);
}

/* ------------------------------------------------------------------ */
/* targeting                                                            */

static int range_of(int chip) {
    switch (chip) {
    case CH_STREAK: case CH_BEAM: return 7;
    default: return 2;
    }
}
static int dirs_of(int chip) {
    switch (chip) {
    case CH_DASH: case CH_LEAP: case CH_ARC: case CH_LOB: return 8;
    default: return 4;
    }
}

/* Can a rolling move enter tile (nx,ny) from (cx,cy)? Also reports whether
 * entering means pushing / stomping a grub (which ends the roll). */
static bool roll_step_ok(const Board *b, int cx, int cy, int dx, int dy, bool *ends) {
    int nx = cx + dx, ny = cy + dy;
    *ends = false;
    if (!inb(nx, ny) || b->hole[ny][nx]) return false;
    if (b->elev[ny][nx] > b->elev[cy][cx]) return false;
    if (b->bsp[ny][nx]) {
        *ends = true;
        if (b->elev[cy][cx] > b->elev[ny][nx]) return true; /* stomp */
        int bx = nx + dx, by = ny + dy;
        if (!inb(bx, by)) return false;
        if (b->hole[by][bx]) return true;
        if (b->bsp[by][bx] || is_player(b, bx, by)) return false;
        if (b->elev[by][bx] > b->elev[ny][nx]) return false;
        return true;
    }
    return true;
}

void gs_targets(const Board *b, int chip, uint8_t out[GH][GW]) {
    memset(out, 0, GW * GH);
    int px = b->px, py = b->py;
    switch (chip) {
    case CH_ROLL: case CH_DASH: case CH_STREAK: {
        for (int d = 0; d < dirs_of(chip); d++) {
            int cx = px, cy = py;
            for (int k = 1; k <= range_of(chip); k++) {
                bool ends;
                if (!roll_step_ok(b, cx, cy, DIR8[d][0], DIR8[d][1], &ends)) break;
                cx += DIR8[d][0];
                cy += DIR8[d][1];
                out[cy][cx] = 1;
                if (ends) break;
            }
        }
        break;
    }
    case CH_SKIP: case CH_LEAP:
        for (int d = 0; d < dirs_of(chip); d++)
            for (int k = 1; k <= 2; k++) {
                int x = px + DIR8[d][0] * k, y = py + DIR8[d][1] * k;
                if (inb(x, y) && !b->hole[y][x] && !b->bsp[y][x]) out[y][x] = 1;
            }
        break;
    case CH_VAULT:
        for (int y = py - 2; y <= py + 2; y++)
            for (int x = px - 2; x <= px + 2; x++)
                if (inb(x, y) && !(x == px && y == py) && !b->hole[y][x] && !b->bsp[y][x]) out[y][x] = 1;
        break;
    case CH_WARP:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->pods[y][x] && !b->bsp[y][x] && !is_player(b, x, y)) out[y][x] = 1;
        break;
    case CH_ZAP: case CH_ARC: case CH_BEAM:
        for (int d = 0; d < dirs_of(chip); d++)
            for (int k = 1; k <= range_of(chip); k++) {
                int x = px + DIR8[d][0] * k, y = py + DIR8[d][1] * k;
                if (!inb(x, y) || b->elev[y][x] > b->elev[py][px]) break;
                out[y][x] = 1;
            }
        break;
    case CH_TOSS: case CH_LOB:
        for (int d = 0; d < dirs_of(chip); d++)
            for (int k = 1; k <= 2; k++) {
                int x = px + DIR8[d][0] * k, y = py + DIR8[d][1] * k;
                if (inb(x, y)) out[y][x] = 1;
            }
        break;
    case CH_MORTAR: case CH_SEED: case CH_RAISE:
        for (int y = py - 2; y <= py + 2; y++)
            for (int x = px - 2; x <= px + 2; x++)
                if (inb(x, y) && !(x == px && y == py) && !b->hole[y][x]) out[y][x] = 1;
        break;
    case CH_DEVOLVE:
        for (int y = py - 2; y <= py + 2; y++)
            for (int x = px - 2; x <= px + 2; x++)
                if (inb(x, y) && b->bsp[y][x]) out[y][x] = 1;
        break;
    case CH_PULSE: case CH_RECHARGE: case CH_REBOOT:
        out[py][px] = 1;
        break;
    case CH_DETONATE:
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++)
                if (b->pods[y][x]) out[y][x] = 1;
        break;
    }
}

/* ------------------------------------------------------------------ */
/* damage resolution                                                    */

typedef struct { int8_t x, y; uint8_t cause; } Hit;

static void kill_bug(Board *b, int x, int y, int cause, Hit *q, int *qt, Events *ev) {
    int sp = b->bsp[y][x], lv = b->blv[y][x];
    b->bsp[y][x] = SP_NONE;
    if (lv == LV_EGG) { emit(ev, EV_EGG, x, y, 0, 0, sp); return; }
    b->kills++;
    emit(ev, cause == CAUSE_STOMP ? EV_STOMP : EV_KILL, x, y, 0, 0, sp);
    bool ability = (cause == CAUSE_ATTACK || cause == CAUSE_BOOM || cause == CAUSE_SPARK) && lv >= LV_ADULT;
    if (!ability) return;
    if (sp == SP_SPARK) {
        int range = lv == LV_QUEEN ? 2 : 1;
        for (int d = 0; d < 4; d++)
            for (int k = 1; k <= range; k++) {
                int sx = x + DIR8[d][0] * k, sy = y + DIR8[d][1] * k;
                if (!inb(sx, sy)) break;
                emit(ev, EV_SPARK, x, y, sx, sy, 0);
                if (*qt < 250) q[(*qt)++] = (Hit){(int8_t)sx, (int8_t)sy, CAUSE_SPARK};
            }
    } else if (sp == SP_BURROW) {
        b->hole[y][x] = 1;
        b->pods[y][x] = 0;
        b->elev[y][x] = 0;
        emit(ev, EV_HOLE, x, y, 0, 0, 0);
    }
}

static void resolve(Board *b, const Hit *init, int n, Events *ev) {
    Hit q[256];
    int qh = 0, qt = 0;
    for (int i = 0; i < n && qt < 250; i++) q[qt++] = init[i];
    uint8_t boomed[GH][GW];
    memset(boomed, 0, sizeof boomed);
    int kills0 = b->kills;
    while (qh < qt) {
        Hit h = q[qh++];
        int x = h.x, y = h.y;
        if (!inb(x, y)) continue;
        if (is_player(b, x, y) && (h.cause == CAUSE_BOOM || h.cause == CAUSE_SPARK || h.cause == CAUSE_ATTACK)) {
            if (b->status == ST_PLAYING || b->status == ST_WON) {
                b->status = ST_DEAD;
                emit(ev, EV_DIE, x, y, 0, 0, 0);
            }
        }
        if (h.cause != CAUSE_STOMP && h.cause != CAUSE_HOLE && b->pods[y][x] && !boomed[y][x]) {
            boomed[y][x] = 1;
            b->pods[y][x] = 0;
            emit(ev, EV_BOOM, x, y, 0, 0, 0);
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    if (qt < 250) q[qt++] = (Hit){(int8_t)(x + dx), (int8_t)(y + dy), CAUSE_BOOM};
            continue; /* the tile itself is in the queue again as a blast */
        }
        if (!b->bsp[y][x]) continue;
        if (b->bhp[y][x] > 1 && h.cause != CAUSE_STOMP && h.cause != CAUSE_HOLE) {
            b->bhp[y][x]--;
            emit(ev, EV_HIT, x, y, 0, 0, b->bsp[y][x]);
            continue;
        }
        kill_bug(b, x, y, h.cause, q, &qt, ev);
    }
    int combo = b->kills - kills0;
    if (combo > b->best_combo) b->best_combo = (uint8_t)combo;
    if (b->status == ST_PLAYING && b->kills >= b->quota) b->status = ST_WON;
}

static void add_pod(Board *b, int x, int y, Events *ev) {
    if (!inb(x, y) || b->hole[y][x]) return;
    b->pods[y][x]++;
    emit(ev, EV_POD, x, y, 0, 0, b->pods[y][x]);
    if (b->pods[y][x] >= 3) {
        /* overload: the tile blows open into a sinkhole */
        b->pods[y][x] = 0;
        b->hole[y][x] = 1;
        b->elev[y][x] = 0;
        emit(ev, EV_BOOM, x, y, 0, 0, 1);
        emit(ev, EV_HOLE, x, y, 0, 0, 0);
        Hit hits[9];
        int n = 0;
        if (b->bsp[y][x]) hits[n++] = (Hit){(int8_t)x, (int8_t)y, CAUSE_HOLE};
        if (is_player(b, x, y)) { b->status = ST_DEAD; emit(ev, EV_DIE, x, y, 0, 0, 0); }
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                if (dx || dy) hits[n++] = (Hit){(int8_t)(x + dx), (int8_t)(y + dy), CAUSE_BOOM};
        resolve(b, hits, n, ev);
    }
}

static void collect(Board *b, int x, int y, Events *ev) {
    if (b->pods[y][x]) {
        b->energy = (uint16_t)(b->energy + b->pods[y][x]);
        emit(ev, EV_POD, x, y, 0, 0, 0);
        b->pods[y][x] = 0;
    }
}

/* ------------------------------------------------------------------ */
/* actions                                                              */

static bool do_roll(Board *b, int tx, int ty, Events *ev) {
    int dx = isign(tx - b->px), dy = isign(ty - b->py);
    int steps = imax(iabs(tx - b->px), iabs(ty - b->py));
    for (int k = 0; k < steps; k++) {
        bool ends;
        if (!roll_step_ok(b, b->px, b->py, dx, dy, &ends)) break;
        int nx = b->px + dx, ny = b->py + dy;
        if (b->bsp[ny][nx]) {
            if (b->elev[b->py][b->px] > b->elev[ny][nx]) {
                Hit h = {(int8_t)nx, (int8_t)ny, CAUSE_STOMP};
                resolve(b, &h, 1, ev);
            } else {
                int bx = nx + dx, by = ny + dy;
                emit(ev, EV_PUSH, nx, ny, bx, by, b->bsp[ny][nx]);
                if (b->hole[by][bx]) {
                    Hit h = {(int8_t)nx, (int8_t)ny, CAUSE_HOLE};
                    resolve(b, &h, 1, ev);
                } else {
                    b->bsp[by][bx] = b->bsp[ny][nx];
                    b->blv[by][bx] = b->blv[ny][nx];
                    b->bhp[by][bx] = b->bhp[ny][nx];
                    b->bsp[ny][nx] = SP_NONE;
                }
            }
            emit(ev, EV_MOVE, b->px, b->py, nx, ny, 0);
            b->px = (int8_t)nx;
            b->py = (int8_t)ny;
            collect(b, nx, ny, ev);
            break;
        }
        emit(ev, EV_MOVE, b->px, b->py, nx, ny, 0);
        b->px = (int8_t)nx;
        b->py = (int8_t)ny;
        collect(b, nx, ny, ev);
    }
    return true;
}

static void hop_to(Board *b, int tx, int ty, Events *ev) {
    emit(ev, EV_MOVE, b->px, b->py, tx, ty, 1);
    b->px = (int8_t)tx;
    b->py = (int8_t)ty;
    collect(b, tx, ty, ev);
}

static void shoot_line(Board *b, int tx, int ty, Events *ev) {
    int dx = isign(tx - b->px), dy = isign(ty - b->py);
    int steps = imax(iabs(tx - b->px), iabs(ty - b->py));
    Hit hits[8];
    int n = 0;
    for (int k = 1; k <= steps; k++) {
        int x = b->px + dx * k, y = b->py + dy * k;
        if (!inb(x, y) || b->elev[y][x] > b->elev[b->py][b->px]) break;
        hits[n++] = (Hit){(int8_t)x, (int8_t)y, CAUSE_ATTACK};
    }
    emit(ev, EV_SHOT, b->px, b->py, tx, ty, 0);
    resolve(b, hits, n, ev);
}

bool gs_apply(Board *b, int slot, int tx, int ty, Events *ev) {
    if (b->status != ST_PLAYING || slot < 0 || slot >= SLOTS || b->spent[slot]) return false;
    int chip = b->chips[slot];
    if (chip == TOOL_NONE) return false;
    uint8_t valid[GH][GW];
    gs_targets(b, chip, valid);
    if (!inb(tx, ty) || !valid[ty][tx]) return false;
    b->spent[slot] = 1;
    switch (chip) {
    case CH_ROLL: case CH_DASH: case CH_STREAK: do_roll(b, tx, ty, ev); break;
    case CH_SKIP: case CH_LEAP: case CH_VAULT: case CH_WARP: hop_to(b, tx, ty, ev); break;
    case CH_ZAP: case CH_ARC: case CH_BEAM: shoot_line(b, tx, ty, ev); break;
    case CH_TOSS: case CH_LOB: case CH_MORTAR: case CH_DETONATE: {
        Hit h = {(int8_t)tx, (int8_t)ty, CAUSE_ATTACK};
        emit(ev, EV_SHOT, b->px, b->py, tx, ty, 1);
        resolve(b, &h, 1, ev);
        break;
    }
    case CH_PULSE: {
        Hit hits[8];
        int n = 0;
        for (int d = 0; d < 8; d++) hits[n++] = (Hit){(int8_t)(b->px + DIR8[d][0]), (int8_t)(b->py + DIR8[d][1]), CAUSE_ATTACK};
        emit(ev, EV_SHOT, b->px, b->py, b->px, b->py, 2);
        resolve(b, hits, n, ev);
        break;
    }
    case CH_SEED: add_pod(b, tx, ty, ev); break;
    case CH_RAISE:
        b->elev[ty][tx] ^= 1;
        emit(ev, EV_RAISE, tx, ty, 0, 0, b->elev[ty][tx]);
        break;
    case CH_RECHARGE: case CH_REBOOT: {
        int kind = chip == CH_RECHARGE ? KIND_ATTACK : KIND_MOVE;
        for (int i = 0; i < SLOTS; i++)
            if (i != slot && b->chips[i] != TOOL_NONE && CHIPS[b->chips[i]].kind == kind) b->spent[i] = 0;
        emit(ev, EV_RAISE, b->px, b->py, 0, 0, 2);
        break;
    }
    case CH_DEVOLVE:
        for (int y = ty - 1; y <= ty + 1; y++)
            for (int x = tx - 1; x <= tx + 1; x++)
                if (inb(x, y) && b->bsp[y][x]) {
                    b->blv[y][x] = LV_LARVA;
                    b->bhp[y][x] = 1;
                    emit(ev, EV_HIT, x, y, 0, 0, b->bsp[y][x]);
                }
        break;
    }
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
    /* queens settle down and lay eggs */
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (b->bsp[y][x] && b->blv[y][x] == LV_QUEEN) {
                b->blv[y][x] = LV_EGG;
                b->bhp[y][x] = 1;
            }
    /* one species grows */
    int sp = b->evolvers[b->day % 2];
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (b->bsp[y][x] == sp && b->blv[y][x] < LV_QUEEN) {
                place_bug(b, x, y, sp, b->blv[y][x] + 1);
                emit(ev, EV_RAISE, x, y, 0, 0, 3);
            }
    /* new larvae wriggle up */
    int x, y;
    for (int i = 0; i < b->spawn_n; i++)
        if (random_empty(b, &x, &y, false)) place_bug(b, x, y, b->species[rng_range(&b->rng, 0, 2)], LV_LARVA);
    /* fizz pods rain down (never onto Tilly, never a third on a tile) */
    for (int i = 0; i < b->pods_n; i++)
        for (int tries = 0; tries < 30; tries++) {
            x = rng_range(&b->rng, 0, GW - 1);
            y = rng_range(&b->rng, 0, GH - 1);
            if (b->hole[y][x] || is_player(b, x, y) || b->pods[y][x] >= 2) continue;
            b->pods[y][x]++;
            break;
        }
    b->day++;
    memset(b->spent, 0, sizeof b->spent);
    roll_shop(b);
}

/* ------------------------------------------------------------------ */
/* autoplay bot (tests / balance)                                       */

static int chip_value(int chip) {
    switch (chip) {
    case CH_MORTAR: return 9;
    case CH_PULSE: return 8;
    case CH_BEAM: case CH_ARC: case CH_LOB: return 7;
    case CH_DETONATE: case CH_RECHARGE: return 6;
    case CH_ZAP: case CH_TOSS: return 5;
    case CH_LEAP: case CH_VAULT: case CH_DASH: return 4;
    case CH_SKIP: case CH_STREAK: case CH_WARP: return 3;
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
            if (!b->bsp[y][x]) continue;
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
    return s;
}

static void bot_shop(Board *b) {
    for (int guard = 0; guard < 8; guard++) {
        int best_o = -1, best_v = 0;
        for (int o = 0; o < OFFERS; o++) {
            int c = b->shop[o];
            if (c == TOOL_NONE || CHIPS[c].cost > b->energy) continue;
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
    int best = base, bs = -1, bx = 0, by = 0;
    Board tmp;
    uint8_t valid[GH][GW];
    for (int s = 0; s < SLOTS; s++) {
        if (b->spent[s] || b->chips[s] == TOOL_NONE) continue;
        gs_targets(b, b->chips[s], valid);
        for (int y = 0; y < GH; y++)
            for (int x = 0; x < GW; x++) {
                if (!valid[y][x]) continue;
                tmp = *b;
                if (!gs_apply(&tmp, s, x, y, NULL)) continue;
                int v = evaluate(&tmp) + chip_value(b->chips[s]) * -3;
                if (v > best) { best = v; bs = s; bx = x; by = y; }
            }
    }
    if (bs < 0) return false;
    return gs_apply(b, bs, bx, by, ev);
}

void gs_bot_contract(Board *b) {
    for (int guard = 0; guard < 40 && b->status == ST_PLAYING; guard++) {
        bot_shop(b);
        for (int k = 0; k < 20 && gs_bot_turn(b, NULL); k++) {}
        if (b->status != ST_PLAYING) break;
        gs_rest(b, NULL);
    }
}
