/* BOOMTOWN - the rules, kept apart from the screens so the demo player can
 * try moves on a copy of the board. Every rule and its source is listed in
 * docs/games/10-boomtown.md. */
#include "boomtown.h"

static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};

static bool inb(int x, int y) { return x >= 0 && y >= 0 && x < BM_W && y < BM_H; }

/* Devilition's round table: pieces added and demons spawned, rounds 1-10.
 * Round 1's fifteen are the pieces you start with. */
static const uint8_t ADDED[BM_ROUNDS + 1] = {0, 0, 15, 10, 20, 15, 10, 20, 15, 15, 13};
static const uint8_t SPAWN[BM_ROUNDS + 1][3] = {
    {0, 0, 0}, {8, 0, 0}, {9, 0, 0}, {9, 1, 0}, {9, 2, 0}, {9, 3, 0},
    {9, 3, 1}, {9, 3, 2}, {9, 3, 3}, {9, 3, 4}, {0, 0, 0},
};

int bm_pieces_added(int round) { return round >= 1 && round <= BM_ROUNDS ? ADDED[round] : 0; }
int bm_spawn_count(int round, int kind) { return round >= 1 && round <= BM_ROUNDS && kind >= 0 && kind < 3 ? SPAWN[round][kind] : 0; }

bool bm_turns(int kind) { return kind == BM_CANDLE || kind == BM_TWIN || kind == BM_FOUNTAIN; }

int bm_tier(int kind) {
    switch (kind) {
    case BM_STARBURST: case BM_CANDLE: case BM_ROCKET: return 1;
    case BM_BANGER: case BM_PINWHEEL: case BM_JACK: return 2;
    case BM_TWIN: case BM_FOUNTAIN: return 3;
    default: return 0;
    }
}

bool bm_free(const BmGame *g, int x, int y) { return inb(x, y) && g->c[y][x].occ == BO_EMPTY; }

int bm_count(const BmGame *g, int occ) {
    int n = 0;
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++) n += g->c[y][x].occ == occ;
    return n;
}

int bm_pieces_on_board(const BmGame *g) { return bm_count(g, BO_PIECE); }

/* The tiles a firework hits (inside the square). A skyrocket hits nothing
 * itself: it flies up and comes down on its perch. */
int bm_attack(const BmGame *g, int x, int y, int kind, int dir, int8_t out[][2]) {
    (void)g;
    int n = 0;
#define ADD(ax, ay) do { int tx_ = (ax), ty_ = (ay); if (inb(tx_, ty_)) { out[n][0] = (int8_t)tx_; out[n][1] = (int8_t)ty_; n++; } } while (0)
    int d = dir & 3;
    switch (kind) {
    case BM_STARBURST:
    case BM_PERCH:
        for (int oy = -1; oy <= 1; oy++)
            for (int ox = -1; ox <= 1; ox++)
                if (ox || oy) ADD(x + ox, y + oy);
        break;
    case BM_CANDLE:
        for (int k = 1; k < BM_W + BM_H; k++) ADD(x + DX[d] * k, y + DY[d] * k);
        break;
    case BM_BANGER:
        for (int k = 0; k < 4; k++) ADD(x + DX[k], y + DY[k]);
        break;
    case BM_PINWHEEL:
        ADD(x - 1, y - 1); ADD(x + 1, y - 1); ADD(x - 1, y + 1); ADD(x + 1, y + 1);
        break;
    case BM_JACK:
        for (int k = 0; k < 4; k++) ADD(x + DX[k] * 2, y + DY[k] * 2);
        break;
    case BM_TWIN:
        ADD(x + DX[d], y + DY[d]);
        ADD(x - DX[d], y - DY[d]);
        break;
    case BM_FOUNTAIN: {
        /* straight ahead and both front corners */
        int fx = x + DX[d], fy = y + DY[d];
        int sx = DX[(d + 1) & 3], sy = DY[(d + 1) & 3];
        ADD(fx - sx, fy - sy); ADD(fx, fy); ADD(fx + sx, fy + sy);
        break;
    }
    default: break;
    }
#undef ADD
    return n;
}

/* ------------------------------------------------------------------ */
/* the bag                                                              */

/* A rocket colour is taken while its rocket or its perch is on the square,
 * on offer, or owed. */
bool bm_colour_free(const BmGame *g, int col) {
    if (g->perch_owed == col) return false;
    for (int i = 0; i < BM_ROW; i++)
        if (g->row[i] == BM_ROCKET && g->row_col[i] == col) return false;
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++) {
            const BmCell *c = &g->c[y][x];
            if (c->occ == BO_PIECE && (c->kind == BM_ROCKET || c->kind == BM_PERCH) && c->col == col) return false;
        }
    return true;
}

/* Bags of six: one tier-1, three tier-2 and two tier-3, in a shuffled order;
 * each piece is rolled within its tier as it is dealt. */
static int deal(BmGame *g, int *col) {
    if (g->bag_i >= 6) {
        static const uint8_t TIERS[6] = {1, 2, 2, 2, 3, 3};
        memcpy(g->bag, TIERS, 6);
        for (int i = 5; i > 0; i--) {
            int j = rng_range(&g->rng, 0, i);
            uint8_t t = g->bag[i]; g->bag[i] = g->bag[j]; g->bag[j] = t;
        }
        g->bag_i = 0;
    }
    int tier = g->bag[g->bag_i++];
    int kind;
    *col = 0;
    if (tier == 1) {
        static const uint8_t T1[3] = {BM_STARBURST, BM_CANDLE, BM_ROCKET};
        kind = T1[rng_range(&g->rng, 0, 2)];
        if (kind == BM_ROCKET) {
            /* only two rockets (red and green) at once */
            if (bm_colour_free(g, 1)) *col = 1;
            else if (bm_colour_free(g, 2)) *col = 2;
            else kind = rng_chance(&g->rng, 50) ? BM_STARBURST : BM_CANDLE;
        }
    } else if (tier == 2) {
        static const uint8_t T2[3] = {BM_BANGER, BM_PINWHEEL, BM_JACK};
        kind = T2[rng_range(&g->rng, 0, 2)];
    } else {
        kind = rng_chance(&g->rng, 50) ? BM_TWIN : BM_FOUNTAIN;
    }
    return kind;
}

int bm_row_count(const BmGame *g) {
    int n = 0;
    for (int i = 0; i < BM_ROW; i++) n += g->row[i] != BM_NONE;
    return n;
}

/* The row shows as many of the pieces in hand as fit in three places (an
 * owed perch holds one of them back). */
void bm_fill_row(BmGame *g) {
    int cap = g->hand - (g->perch_owed ? 1 : 0);
    if (cap < 0) cap = 0;
    int want = imin(BM_ROW, cap);
    /* a rocket's perch took the last piece: the rightmost goes */
    for (int i = BM_ROW - 1; i >= 0 && bm_row_count(g) > want; i--)
        if (g->row[i]) { g->row[i] = BM_NONE; g->row_col[i] = 0; }
    for (int i = 0; i < BM_ROW && bm_row_count(g) < want; i++)
        if (!g->row[i]) {
            int col;
            g->row[i] = (uint8_t)deal(g, &col);
            g->row_col[i] = (uint8_t)col;
        }
}

/* ------------------------------------------------------------------ */
/* setting up                                                           */

static bool random_empty(BmGame *g, int *ox, int *oy) {
    int cand[BM_W * BM_H], n = 0;
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++)
            if (g->c[y][x].occ == BO_EMPTY) cand[n++] = y * BM_W + x;
    if (n == 0) return false;
    int k = cand[rng_range(&g->rng, 0, n - 1)];
    *ox = k % BM_W;
    *oy = k / BM_W;
    return true;
}

static void add_folk(BmGame *g) {
    int x, y;
    if (!random_empty(g, &x, &y)) return;
    g->c[y][x] = (BmCell){BO_FOLK, 0, 0, 1, 0, (uint8_t)rng_range(&g->rng, 0, 3)};
    int n = bm_count(g, BO_FOLK);
    if (n > g->folk_most) g->folk_most = (uint8_t)n;
}

static void add_hole(BmGame *g) {
    int x, y;
    if (random_empty(g, &x, &y)) g->c[y][x] = (BmCell){BO_HOLE, 0, 0, 0, 0, (uint8_t)rng_range(&g->rng, 0, 3)};
}

void bm_new_game(BmGame *g, uint64_t seed) {
    memset(g, 0, sizeof *g);
    rng_seed(&g->rng, seed);
    g->bag_i = 6;
    for (int i = 0; i < BM_START_FOLK; i++) add_folk(g);
    g->hand = BM_START_PIECES;
    g->round = 0;
    bm_round_begin(g);
}

void bm_round_begin(BmGame *g) {
    g->round++;
    if (g->round < BM_ROUNDS) {
        add_hole(g);
        static const uint8_t HP[3] = {1, 2, 2};
        for (int k = 0; k < 3; k++)
            for (int i = 0; i < SPAWN[g->round][k]; i++) {
                int x, y;
                if (!random_empty(g, &x, &y)) break;
                g->c[y][x] = (BmCell){BO_BOGLE, (uint8_t)k, 0, HP[k], 0, (uint8_t)rng_range(&g->rng, 0, 3)};
            }
    } else {
        /* the last night: every bogle left sinks into a hole, the Bog King
         * rises in the middle four tiles and the fireworks round him go */
        for (int y = 0; y < BM_H; y++)
            for (int x = 0; x < BM_W; x++)
                if (g->c[y][x].occ == BO_BOGLE) g->c[y][x] = (BmCell){BO_HOLE, 0, 0, 0, 0, (uint8_t)((x + y) & 3)};
        for (int y = BM_H / 2 - 2; y <= BM_H / 2 + 1; y++)
            for (int x = BM_W / 2 - 2; x <= BM_W / 2 + 1; x++) {
                bool middle = x >= BM_W / 2 - 1 && x <= BM_W / 2 && y >= BM_H / 2 - 1 && y <= BM_H / 2;
                if (middle) g->c[y][x] = (BmCell){BO_KING, 0, 0, 0, 0, 0};
                else if (g->c[y][x].occ == BO_PIECE) g->c[y][x] = (BmCell){0};
            }
        int extra = rng_range(&g->rng, 2, 4);
        for (int i = 0; i < extra; i++) add_hole(g);
        g->king_hp = BM_BOSS_HP;
    }
    g->hand = (int16_t)(g->hand + (g->round > 1 ? ADDED[g->round] : 0));
    bm_fill_row(g);
}

/* ------------------------------------------------------------------ */
/* placing                                                              */

/* Put the piece on offer in `slot` at (x,y). Returns true if it was a
 * skyrocket, whose perch comes next (and costs a piece of its own). */
bool bm_place(BmGame *g, int slot, int x, int y, int dir) {
    if (slot < 0 || slot >= BM_ROW || !g->row[slot] || !bm_free(g, x, y) || g->perch_owed) return false;
    int kind = g->row[slot];
    g->c[y][x] = (BmCell){BO_PIECE, (uint8_t)kind, (uint8_t)(bm_turns(kind) ? dir & 3 : 0), 0, g->row_col[slot], 0};
    g->row[slot] = BM_NONE;
    g->row_col[slot] = 0;
    g->hand--;
    if (kind == BM_ROCKET) {
        g->perch_owed = g->c[y][x].col;
        return true;
    }
    bm_fill_row(g);
    return false;
}

bool bm_place_perch(BmGame *g, int x, int y) {
    if (!g->perch_owed || g->hand <= 0 || !bm_free(g, x, y)) return false;
    g->c[y][x] = (BmCell){BO_PIECE, BM_PERCH, 0, 0, g->perch_owed, 0};
    g->perch_owed = 0;
    g->hand--;
    bm_fill_row(g);
    return true;
}

/* ------------------------------------------------------------------ */
/* the chain                                                            */

static void enqueue(BmChain *ch, int x, int y, int t, int landing) {
    if (ch->n >= ARRAY_LEN(ch->q)) return;
    ch->q[ch->n++] = (BmFuse){(int8_t)x, (int8_t)y, (uint8_t)landing, (uint16_t)t};
}

void bm_chain_start(BmGame *g, BmChain *ch, int x, int y) {
    (void)g;
    memset(ch, 0, sizeof *ch);
    ch->lit[y][x] = 1;
    enqueue(ch, x, y, 0, 0);
}

static void fire(BmGame *g, BmChain *ch, int x, int y, const BmFx *fx) {
    BmCell *c = &g->c[y][x];
    if (c->occ != BO_PIECE) return; /* already gone off */
    int kind = c->kind, dir = c->dir, col = c->col;
    *c = (BmCell){0};
    ch->fired++;
    if (fx && fx->fire) fx->fire(x, y, kind, dir, col);
    if (kind == BM_ROCKET) {
        /* up it goes, and down on its perch if the perch is still there */
        for (int py = 0; py < BM_H; py++)
            for (int px = 0; px < BM_W; px++) {
                BmCell *p = &g->c[py][px];
                if (p->occ == BO_PIECE && p->kind == BM_PERCH && p->col == col) {
                    enqueue(ch, px, py, ch->now + BM_ROCKET_FLIGHT, 1);
                    ch->lit[py][px] = 1;
                    if (fx && fx->launch) fx->launch(x, y, col, px, py, 1);
                    return;
                }
            }
        if (fx && fx->launch) fx->launch(x, y, col, x, -2, 0);
        return;
    }
    int8_t t[BM_W + BM_H + 8][2];
    int n = bm_attack(g, x, y, kind, dir, t);
    for (int i = 0; i < n; i++) {
        int tx = t[i][0], ty = t[i][1];
        BmCell *h = &g->c[ty][tx];
        ch->hitmap[ty][tx] = 1;
        int what = h->occ, dead = 0;
        switch (what) {
        case BO_BOGLE:
            ch->hits++;
            if (h->hp > 0) h->hp--;
            if (h->hp == 0) { *h = (BmCell){0}; ch->killed++; dead = 1; }
            break;
        case BO_FOLK:
            *h = (BmCell){0};
            ch->folk_lost++;
            dead = 1;
            break;
        case BO_KING:
            if (g->king_hp > 0) { g->king_hp--; ch->king_hits++; }
            break;
        case BO_PIECE:
            if (!ch->lit[ty][tx]) {
                ch->lit[ty][tx] = 1;
                enqueue(ch, tx, ty, ch->now + BM_CHAIN_STEP, 0);
            }
            break;
        default: break;
        }
        if (fx && fx->hit) fx->hit(tx, ty, what, dead);
    }
}

bool bm_chain_tick(BmGame *g, BmChain *ch, const BmFx *fx) {
    for (int i = 0; i < ch->n;) {
        if (ch->q[i].t <= ch->now) {
            BmFuse f = ch->q[i];
            ch->q[i] = ch->q[--ch->n];
            fire(g, ch, f.x, f.y, fx);
            i = 0; /* the list changed: look again from the start */
        } else {
            i++;
        }
    }
    ch->now++;
    return ch->n > 0;
}

void bm_chain_run(BmGame *g, BmChain *ch) {
    int guard = 0;
    while (bm_chain_tick(g, ch, NULL) && guard++ < 20000) {}
}

/* ------------------------------------------------------------------ */
/* the end of a round                                                   */

int bm_round_end(BmGame *g) {
    if (g->round >= BM_ROUNDS) return g->king_hp == 0 ? BR_WON : BR_LOST;
    /* old bogles heal a wound */
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++) {
            BmCell *c = &g->c[y][x];
            if (c->occ == BO_BOGLE && c->kind == BG_OLD && c->hp < 2) c->hp = 2;
        }
    int bogles = bm_count(g, BO_BOGLE), folk = bm_count(g, BO_FOLK);
    if (bogles > folk) return BR_OVERRUN;
    if (bogles == 0) {
        add_folk(g);
        return BR_CLEARED;
    }
    return BR_HOLDS;
}

int bm_time_bonus(uint32_t frames) {
    int minutes = (int)((frames / 60u + 30u) / 60u);
    return imax(0, 15000 - 250 * minutes);
}

int bm_score(const BmGame *g, uint32_t frames, int *time_bonus) {
    int tb = bm_time_bonus(frames);
    if (time_bonus) *time_bonus = tb;
    return 10000 + 1000 * (bm_count(g, BO_FOLK) + bm_pieces_on_board(g) + g->hand) + tb;
}
