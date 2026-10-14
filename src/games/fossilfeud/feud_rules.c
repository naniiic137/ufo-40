/* FOSSIL FEUD - the rules of the board: the tables (the Ancestors and
 * their actions with both blessings, the birds, the buildings), making a
 * board, praying and blessing, every action, the omens, turns and totems.
 * Used by the screens, the Slate Flock's planner and the demo player. */
#include "feud.h"

const FeudAncDef FEUD_ANC[FD_ANC] = {
    {"GRANDJAW", "THE OLD KING", "GRAIN AND HOSTS OF CHEAP BIRDS", C_RED},
    {"PLATEBACK", "THE WAR BEAST", "QUICK GROUND, COSTLY BIRDS", C_ORANGE},
    {"LONGNECK", "THE TALL ONE", "PLENTY, AND THE SHAPE OF THE LAND", C_LEAF},
    {"SKYCREST", "THE FIRST FLIER", "FREE MUSTERS AND FAIR TRADES", C_SKY},
    {"THREEHORN", "THE BONE SAGE", "RELICS AND OMENS THAT STRIKE", C_VIOLET},
};

/* the five birds: hit points, shield, speed (1/256 px a frame), reach (px),
 * frames between attacks, harm to each kind (stomper lancer slinger mystic
 * swift; -1 can't reach it), and worth */
const FeudUnitDef FEUD_UNIT[FU_KINDS] = {
    {"STOMPER", "STOMPERS", 6, 0, 102, 12, 20, 80, {1, 1, 1, 1, -1}, {3, 1, 3, 1, -1}, 4},
    {"LANCER", "LANCERS", 5, 5, 102, 12, 20, 80, {2, 2, 2, 2, -1}, {5, 5, 5, 2, -1}, 9},
    {"SLINGER", "SLINGERS", 4, 0, 51, 32, 160, 100, {0, 0, 0, 0, 0}, {3, 3, 3, 3, 3}, 6},
    {"MYSTIC", "MYSTICS", 3, 0, 51, 0, 88, 110, {3, 3, 3, 2, 3}, {5, 5, 5, 3, 3}, 8},
    {"SWIFT", "SWIFTS", 4, 0, 154, 0, 24, 110, {5, 4, 4, 2, 1}, {7, 6, 6, 2, 2}, 9},
};

const char *const FEUD_BLD_NAME[FB_KINDS] = {"GRANARY", "ROOST", "SHRINE", "DUSTBATH"};
const char *const FEUD_TERR_NAME[FT_KINDS] = {"MEADOW", "THICKET", "BONEYARD", "SCREE", "GORGE", "PEAK", "EYRIE"};
const char *const FEUD_RES_NAME[FR_KINDS] = {"GRAIN", "TOILERS", "RELICS"};
const char *const FEUD_OMEN_NAME[OM_KINDS] = {"TITHE", "SHIFT PEAK", "HAILSTORM", "FAR SIGHT", "UPDRAFT", "THUNDERBOLT", "HATCHING"};
const char *const FEUD_SIDE_NAME[2] = {"RUST ROOST", "SLATE FLOCK"};

#define O(w, r, c) {w, r, c, 0}
#define TR(from, n, to, got) {from, to, n, got}
#define G FR_GRAIN
#define T FR_TOILERS
#define R FR_RELICS

/* every action at no blessing, one and two */
const FeudActDef FEUD_ACT[FD_ACTS] = {
    /* GRANDJAW */
    {"GATHER", AK_GATHER, {{.g = {10, 3, 0, 0, 0}}, {.g = {10, 3, 1, 0, 0}}, {.g = {18, 3, 1, 0, 0}}}},
    {"ENLIST", AK_ENLIST,
     {{.n = 2, .off = {O(FU_STOMPER, G, 4), O(FU_SLINGER, G, 6)}},
      {.n = 3, .off = {O(FU_STOMPER, G, 3), O(FU_SLINGER, G, 4), O(FU_STOMPER, T, 1)}},
      {.n = 3, .off = {O(FU_STOMPER, G, 2), O(FU_SLINGER, G, 3), O(FU_LANCER, T, 1)}}}},
    {"MARCH", AK_MARCH, {{.mflags = 0}, {.mflags = MF_DIAG}, {.mflags = MF_DIAG | MF_SURPRISE}}},
    /* PLATEBACK */
    {"MARCH", AK_MARCH, {{.mflags = 0}, {.mflags = MF_SURPRISE}, {.mflags = MF_SURPRISE | MF_THROUGH}}},
    {"BUILD", AK_BUILD,
     {{.n = 3, .off = {O(FB_GRANARY, T, 2), O(FB_ROOST, T, 3), O(FB_SHRINE, T, 3)}},
      {.n = 3, .off = {O(FB_GRANARY, T, 1), O(FB_ROOST, T, 2), O(FB_SHRINE, T, 2)}},
      {.n = 3, .off = {O(FB_GRANARY, T, 1), O(FB_ROOST, T, 1), O(FB_SHRINE, T, 1)}}}},
    {"ENLIST", AK_ENLIST,
     {{.n = 2, .off = {O(FU_LANCER, G, 8), O(FU_SWIFT, G, 10)}},
      {.n = 3, .off = {O(FU_LANCER, G, 5), O(FU_SWIFT, G, 7), O(FU_MYSTIC, R, 2)}},
      {.n = 3, .off = {O(FU_LANCER, G, 4), O(FU_SWIFT, G, 5), O(FU_MYSTIC, R, 1)}}}},
    /* LONGNECK */
    {"GATHER", AK_GATHER, {{.g = {0, 4, 1, 0, 1}}, {.g = {0, 5, 2, 0, 1}}, {.g = {0, 6, 2, 0, 2}}}},
    {"BUILD", AK_BUILD,
     {{.n = 3, .off = {O(FB_BATH, G, 4), O(FB_GRANARY, T, 2), O(FB_SHRINE, T, 3)}},
      {.n = 3, .off = {O(FB_BATH, G, 3), O(FB_GRANARY, T, 1), O(FB_SHRINE, T, 2)}},
      {.n = 3, .off = {O(FB_BATH, G, 2), O(FB_GRANARY, T, 1), O(FB_SHRINE, T, 1)}}}},
    {"OMEN", AK_OMEN,
     {{.n = 3, .off = {O(OM_TITHE, R, 3), O(OM_SHIFT, R, 3), O(OM_HAIL, R, 6)},
       .nf = 3, .offf = {O(OM_TITHE, R, 3), O(OM_SHIFT, R, 3), O(OM_HAIL, R, 7)}},
      {.n = 3, .off = {O(OM_TITHE, R, 2), O(OM_SHIFT, R, 2), O(OM_HAIL, R, 3)},
       .nf = 3, .offf = {O(OM_TITHE, R, 2), O(OM_SHIFT, R, 2), O(OM_HAIL, R, 5)}},
      {.n = 3, .off = {O(OM_TITHE, R, 1), O(OM_SHIFT, R, 1), O(OM_HAIL, R, 2)},
       .nf = 3, .offf = {O(OM_TITHE, R, 1), O(OM_SHIFT, R, 1), O(OM_HAIL, R, 3)}}}},
    /* SKYCREST */
    {"CALL UP", AK_CALLUP, {{.g = {2, 0, 0, 0}}, {.g = {2, 1, 1, 0}}, {.g = {2, 1, 1, 1}}}},
    {"MARCH", AK_MARCH, {{.mflags = 0}, {.mflags = MF_THROUGH}, {.mflags = MF_THROUGH | MF_DIAG}}},
    {"BARTER", AK_BARTER,
     {{.n = 2, .off = {TR(R, 1, G, 4), TR(G, 4, T, 1)}},
      {.n = 2, .off = {TR(R, 1, G, 6), TR(G, 3, T, 1)}},
      {.n = 2, .off = {TR(R, 1, G, 9), TR(G, 2, T, 1)}}}},
    /* THREEHORN */
    {"GATHER", AK_GATHER, {{.g = {0, 0, 0, 3, 2}}, {.g = {8, 0, 0, 3, 2}}, {.g = {16, 0, 0, 3, 2}}}},
    {"ENLIST", AK_ENLIST,
     {{.n = 1, .off = {O(FU_MYSTIC, G, 8)}},
      {.n = 3, .off = {O(FU_MYSTIC, G, 5), O(FU_SWIFT, G, 8), O(FU_SLINGER, T, 1)}},
      {.n = 3, .off = {O(FU_MYSTIC, G, 4), O(FU_SWIFT, G, 6), O(FU_MYSTIC, T, 1)}}}},
    {"OMEN", AK_OMEN,
     {{.n = 3, .off = {O(OM_SIGHT, R, 3), O(OM_UPDRAFT, R, 6), O(OM_BOLT, R, 9)},
       .nf = 2, .offf = {O(OM_HATCH, R, 3), O(OM_UPDRAFT, R, 6)}},
      {.n = 3, .off = {O(OM_SIGHT, R, 2), O(OM_UPDRAFT, R, 4), O(OM_BOLT, R, 7)},
       .nf = 3, .offf = {O(OM_HATCH, R, 2), O(OM_UPDRAFT, R, 5), O(OM_BOLT, R, 9)}},
      {.n = 3, .off = {O(OM_SIGHT, R, 1), O(OM_UPDRAFT, R, 3), O(OM_BOLT, R, 4)},
       .nf = 3, .offf = {O(OM_HATCH, R, 1), O(OM_UPDRAFT, R, 4), O(OM_BOLT, R, 6)}}}},
};
#undef G
#undef T
#undef R

int feud_last_got[FR_KINDS];

/* ---- the board ------------------------------------------------------------- */

static int col_of(int t) { return t % FD_COLS; }
static int row_of(int t) { return t / FD_COLS; }
static int at(int c, int r) { return r * FD_COLS + c; }
static int mirror(int t) { return at(FD_COLS - 1 - col_of(t), FD_ROWS - 1 - row_of(t)); }

bool feud_adjacent(int a, int b, bool diag) {
    int dc = iabs(col_of(a) - col_of(b)), dr = iabs(row_of(a) - row_of(b));
    if (a == b) return false;
    if (diag) return dc <= 1 && dr <= 1;
    return dc + dr == 1;
}

int feud_dist(int a, int b) { return imax(iabs(col_of(a) - col_of(b)), iabs(row_of(a) - row_of(b))); }

int feud_tile_blds(const FeudTile *t) {
    int n = 0;
    for (int k = 0; k < FB_KINDS; k++) n += t->b[k];
    return n;
}

int feud_units_on(const FeudMatch *m, int t) {
    int n = 0;
    for (int k = 0; k < FU_KINDS; k++) n += m->t[t].u[k];
    return n;
}

int feud_army(const FeudMatch *m, int side) {
    int n = 0;
    for (int t = 0; t < FD_TILES; t++)
        if (m->t[t].owner == side) n += feud_units_on(m, t);
    return n;
}

int feud_flags_held(const FeudMatch *m, int side) {
    int n = 0;
    for (int t = 0; t < FD_TILES; t++) n += m->t[t].owner == side && m->t[t].flag;
    return n;
}

int feud_tiles_held(const FeudMatch *m, int side) {
    int n = 0;
    for (int t = 0; t < FD_TILES; t++) n += m->t[t].owner == side;
    return n;
}

int feud_bld_count(const FeudMatch *m, int side, int kind) {
    int n = 0;
    for (int t = 0; t < FD_TILES; t++)
        if (m->t[t].owner == side) n += m->t[t].b[kind];
    return n;
}

void feud_add_res(FeudMatch *m, int side, int r, int n) {
    int cap = r == FR_GRAIN ? FD_RES_CAP_GRAIN : FD_RES_CAP_OTHER;
    m->s[side].res[r] = iclamp(m->s[side].res[r] + n, 0, cap);
}

/* every tile not a peak reachable from every other, a step at a time */
static bool connected(const FeudMatch *m) {
    int seen[FD_TILES] = {0}, stack[FD_TILES], sp = 0, start = -1, total = 0;
    for (int t = 0; t < FD_TILES; t++)
        if (m->t[t].terr != FT_PEAK) { total++; if (start < 0) start = t; }
    if (start < 0) return false;
    stack[sp++] = start;
    seen[start] = 1;
    int n = 0;
    while (sp) {
        int t = stack[--sp];
        n++;
        for (int u = 0; u < FD_TILES; u++)
            if (!seen[u] && m->t[u].terr != FT_PEAK && feud_adjacent(t, u, false)) { seen[u] = 1; stack[sp++] = u; }
    }
    return n == total;
}

/* a free tile on our half (or the middle), whose mirror is free too */
static int pick_pair(FeudMatch *m, const uint8_t *used) {
    int cand[FD_TILES], n = 0;
    for (int t = 0; t < FD_TILES; t++) {
        int c = col_of(t), r = row_of(t);
        if (c > 2 || (c == 2 && r > 1)) continue;
        if (used[t] || used[mirror(t)]) continue;
        cand[n++] = t;
    }
    return n ? cand[rng_range(&m->rng, 0, n - 1)] : -1;
}

static int pick_middle(FeudMatch *m, const uint8_t *used) {
    int cand[FD_ROWS], n = 0;
    for (int r = 0; r < FD_ROWS; r++)
        if (!used[at(2, r)]) cand[n++] = at(2, r);
    return n ? cand[rng_range(&m->rng, 0, n - 1)] : -1;
}

static uint8_t terrain_drop(FeudMatch *m, int terr) {
    switch (terr) {
    case FT_MEADOW: return rng_chance(&m->rng, 60) ? FR_TOILERS : FR_GRAIN;
    case FT_THICKET: return rng_chance(&m->rng, 75) ? FR_GRAIN : FR_TOILERS;
    case FT_BONEYARD: return FR_RELICS;
    default: return (uint8_t)rng_range(&m->rng, 0, FR_KINDS - 1);
    }
}

static void place_board(FeudMatch *m, const FeudSetup *su) {
    static const int TW[5] = {30, 24, 16, 15, 15}; /* meadow thicket boneyard scree gorge */
    uint8_t used[FD_TILES];
    memset(m->t, 0, sizeof m->t);
    memset(used, 0, sizeof used);
    for (int t = 0; t < FD_TILES; t++) m->t[t].owner = -1;
    /* the ground, mirrored through the middle so both sides get the same */
    for (int t = 0; t < FD_TILES; t++) {
        int c = col_of(t), r = row_of(t);
        if (c > 2 || (c == 2 && r > 1)) continue;
        int roll = rng_range(&m->rng, 0, 99), k = 0;
        while (roll >= TW[k]) { roll -= TW[k]; k++; }
        m->t[t].terr = (uint8_t)k;
        m->t[mirror(t)].terr = (uint8_t)k;
    }
    /* the two homes on opposite sides */
    int r0 = rng_range(&m->rng, 0, FD_ROWS - 1);
    int home[2] = {at(0, r0), mirror(at(0, r0))};
    int eyr = iclamp(su->eyries, 2, 6);
    for (int s = 0; s < 2; s++) {
        FeudTile *h = &m->t[home[s]];
        h->owner = (int8_t)s;
        h->terr = FT_EYRIE;
        h->eyrie = 1;
        h->flag = 1;
        h->u[FU_STOMPER] = 2;
        h->b[FB_GRANARY] = 1;
        h->b[FB_ROOST] = 1;
        used[home[s]] = 1;
    }
    int left = eyr - 2;
    while (left >= 2) {
        int t = pick_pair(m, used);
        if (t < 0) break;
        int u[2] = {t, mirror(t)};
        for (int i = 0; i < 2; i++) { m->t[u[i]].terr = FT_EYRIE; m->t[u[i]].eyrie = 1; m->t[u[i]].flag = 1; used[u[i]] = 1; }
        left -= 2;
    }
    if (left == 1) {
        int t = pick_middle(m, used);
        if (t >= 0) { m->t[t].terr = FT_EYRIE; m->t[t].eyrie = 1; m->t[t].flag = 1; used[t] = 1; }
    }
    /* peaks, never cutting the board in two */
    FeudTile keep[FD_TILES];
    memcpy(keep, m->t, sizeof keep);
    uint8_t keep_used[FD_TILES];
    memcpy(keep_used, used, sizeof used);
    for (int tries = 0; tries < 60; tries++) {
        memcpy(m->t, keep, sizeof keep);
        memcpy(used, keep_used, sizeof used);
        int pk = iclamp(su->peaks, 0, 8) - tries / 20 * 2;
        if (pk < 0) pk = 0;
        while (pk >= 2) {
            int t = pick_pair(m, used);
            if (t < 0) break;
            m->t[t].terr = FT_PEAK;
            m->t[mirror(t)].terr = FT_PEAK;
            used[t] = used[mirror(t)] = 1;
            pk -= 2;
        }
        if (pk == 1) {
            int t = pick_middle(m, used);
            if (t >= 0) { m->t[t].terr = FT_PEAK; used[t] = 1; }
        }
        if (connected(m)) break;
    }
    /* the totems: every eyrie, and the rest on open ground */
    int flags = 0;
    for (int t = 0; t < FD_TILES; t++) flags += m->t[t].flag;
    while (FD_FLAGS - flags >= 2) {
        int t = pick_pair(m, used);
        if (t < 0) break;
        m->t[t].flag = m->t[mirror(t)].flag = 1;
        used[t] = used[mirror(t)] = 1;
        flags += 2;
    }
    if (FD_FLAGS - flags == 1) {
        int t = pick_middle(m, used);
        if (t >= 0) { m->t[t].flag = 1; used[t] = 1; }
    }
    /* what each tile gives its first claimer: a peak two things, an eyrie none */
    for (int t = 0; t < FD_TILES; t++) {
        int c = col_of(t), r = row_of(t);
        if (c > 2 || (c == 2 && r > 1)) continue;
        FeudTile *a = &m->t[t], *b = &m->t[mirror(t)];
        if (a->eyrie) continue;
        a->ndrop = a->terr == FT_PEAK ? 2 : 1;
        for (int i = 0; i < a->ndrop; i++) a->drop[i] = terrain_drop(m, a->terr);
        if (!b->eyrie && b->owner < 0) {
            b->ndrop = a->ndrop;
            b->drop[0] = a->drop[0];
            b->drop[1] = a->drop[1];
            if (b->terr != a->terr) b->ndrop = b->terr == FT_PEAK ? 2 : 1;
        }
    }
    for (int t = 0; t < FD_TILES; t++)
        if (m->t[t].eyrie || m->t[t].owner >= 0) m->t[t].ndrop = 0;
}

void feud_setup_match(FeudMatch *m, const FeudSetup *su) {
    memset(m, 0, sizeof *m);
    rng_seed(&m->rng, su->seed);
    m->fog = su->fog;
    m->ordeal = su->ordeal;
    m->winner = -1;
    for (int a = 0; a < FD_ANC; a++)
        for (int i = 0; i < 3; i++) m->deck[a][i] = (uint8_t)(a * 3 + i);
    place_board(m, su);
    for (int s = 0; s < 2; s++) {
        m->s[s].res[FR_GRAIN] = 10;
        m->s[s].res[FR_TOILERS] = 3;
        m->s[s].res[FR_RELICS] = 3;
    }
    int o = m->ordeal;
    if (o == 1 || o == 5) m->s[1].res[FR_GRAIN] += 20; /* ordeal 1: a full larder */
    if (o == 2 || o == 5) {
        /* ordeal 2: a second eyrie already theirs, the one nearest home */
        int home = -1, best = -1, bd = 99;
        for (int t = 0; t < FD_TILES; t++)
            if (m->t[t].owner == 1 && m->t[t].eyrie) home = t;
        for (int t = 0; t < FD_TILES; t++)
            if (m->t[t].eyrie && m->t[t].owner < 0 && home >= 0) {
                int d = feud_dist(t, home) * 4 + (col_of(t) < 2 ? 8 : 0);
                if (d < bd) { bd = d; best = t; }
            }
        if (best >= 0) { m->t[best].owner = 1; m->t[best].ndrop = 0; }
    }
    if (o == 4 || o == 5) {
        /* ordeal 4: the Ancestors' actions are dealt out afresh */
        uint8_t all[FD_ACTS];
        for (int i = 0; i < FD_ACTS; i++) all[i] = (uint8_t)i;
        for (int i = FD_ACTS - 1; i > 0; i--) {
            int j = rng_range(&m->rng, 0, i);
            uint8_t k = all[i];
            all[i] = all[j];
            all[j] = k;
        }
        memcpy(m->deck, all, sizeof m->deck);
        m->shuffled = 1;
    }
    if (o == 3 || o == 5) {
        /* ordeal 3: they start with an Ancestor's blessing */
        int a = rng_range(&m->rng, 0, FD_ANC - 1), i = rng_range(&m->rng, 0, 2);
        m->star[1][m->deck[a][i]] = 1;
        m->s[1].blessings = 1;
    }
    m->recent[0] = m->recent[1] = -1;
    m->turn = 1;
    m->side = 0;
    feud_make_offers(m);
}

/* three Ancestors on offer: the five less the two prayed to in the last two
 * turns (on the first two turns, three of those free at random) */
void feud_make_offers(FeudMatch *m) {
    uint8_t free_[FD_ANC];
    int n = 0;
    for (int a = 0; a < FD_ANC; a++)
        if (a != m->recent[0] && a != m->recent[1]) free_[n++] = (uint8_t)a;
    while (n > 3) {
        int k = rng_range(&m->rng, 0, n - 1);
        for (int i = k; i < n - 1; i++) free_[i] = free_[i + 1];
        n--;
    }
    m->noffer = (uint8_t)n;
    for (int i = 0; i < n; i++) m->offer[i] = free_[i];
}

int feud_level(const FeudMatch *m, int side, int act) { return m->star[side][act]; }
const FeudLevel *feud_lv(const FeudMatch *m, int side, int act) { return &FEUD_ACT[act].lv[m->star[side][act]]; }

int feud_offers_of(const FeudMatch *m, int side, int act, const FeudOffer **out) {
    const FeudLevel *l = feud_lv(m, side, act);
    if (FEUD_ACT[act].kind == AK_OMEN && !m->fog) { *out = l->offf; return l->nf; }
    *out = l->off;
    return l->n;
}

bool feud_anc_maxed(const FeudMatch *m, int side, int anc) {
    for (int i = 0; i < 3; i++)
        if (m->star[side][m->deck[anc][i]] < 2) return false;
    return true;
}

bool feud_can_bless(const FeudMatch *m, int side, int anc) { return !feud_anc_maxed(m, side, anc); }

bool feud_bless_due(const FeudMatch *m, int side, int anc) {
    return m->s[side].favour[anc] > 0 && m->s[side].favour[anc] % 2 == 0 && feud_can_bless(m, side, anc);
}

bool feud_add_favour(FeudMatch *m, int side, int anc) {
    if (m->s[side].favour[anc] < 255) m->s[side].favour[anc]++;
    return feud_bless_due(m, side, anc);
}

bool feud_pray(FeudMatch *m, int anc) {
    int side = m->side;
    if (m->s[side].picks[anc] < 255) m->s[side].picks[anc]++;
    m->recent[1] = m->recent[0];
    m->recent[0] = (int8_t)anc;
    return feud_add_favour(m, side, anc);
}

bool feud_bless(FeudMatch *m, int side, int act) {
    if (m->star[side][act] >= 2) return false;
    m->star[side][act]++;
    m->s[side].blessings++;
    return true;
}

bool feud_gift_check(const FeudMatch *m, int side) {
    for (int a = 0; a < FD_ANC; a++)
        if (feud_anc_maxed(m, side, a)) return true;
    return false;
}

/* ---- the actions ------------------------------------------------------------ */

void feud_gather(FeudMatch *m, int side, int act, int got[FR_KINDS]) {
    const int8_t *g = feud_lv(m, side, act)->g;
    int gran = feud_bld_count(m, side, FB_GRANARY), bath = feud_bld_count(m, side, FB_BATH),
        shr = feud_bld_count(m, side, FB_SHRINE);
    int add[FR_KINDS] = {g[0] + g[1] * gran, g[2] * bath, g[3] + g[4] * shr};
    for (int r = 0; r < FR_KINDS; r++) {
        int before = m->s[side].res[r];
        feud_add_res(m, side, r, add[r]);
        if (got) got[r] = m->s[side].res[r] - before;
    }
}

static void add_units(FeudTile *t, int kind, int n) { t->u[kind] = (uint8_t)imin(FD_STACK_MAX, t->u[kind] + n); }

void feud_callup(FeudMatch *m, int side, int act, int got[FU_KINDS]) {
    const int8_t *g = feud_lv(m, side, act)->g;
    if (got) memset(got, 0, sizeof(int) * FU_KINDS);
    for (int t = 0; t < FD_TILES; t++) {
        FeudTile *x = &m->t[t];
        if (x->owner != side) continue;
        int add[FU_KINDS] = {g[0] * x->b[FB_ROOST], g[1] * x->b[FB_ROOST], 0, g[2] * x->b[FB_SHRINE], g[3] * x->b[FB_BATH]};
        for (int k = 0; k < FU_KINDS; k++) {
            int before = x->u[k];
            add_units(x, k, add[k]);
            if (got) got[k] += x->u[k] - before;
        }
    }
}

bool feud_is_recruit_point(const FeudMatch *m, int side, int t) {
    return m->t[t].owner == side && (m->t[t].eyrie || m->t[t].b[FB_ROOST] > 0);
}

bool feud_can_afford(const FeudMatch *m, int side, const FeudOffer *o) { return m->s[side].res[o->res] >= o->cost; }

bool feud_enlist(FeudMatch *m, int side, int act, int t, int idx) {
    const FeudOffer *o;
    int n = feud_offers_of(m, side, act, &o);
    if (idx < 0 || idx >= n || !feud_is_recruit_point(m, side, t) || !feud_can_afford(m, side, &o[idx])) return false;
    if (m->t[t].u[o[idx].what] >= FD_STACK_MAX) return false;
    m->s[side].res[o[idx].res] -= o[idx].cost;
    add_units(&m->t[t], o[idx].what, 1);
    return true;
}

bool feud_can_build_on(const FeudMatch *m, int side, int t) {
    return m->t[t].owner == side && feud_tile_blds(&m->t[t]) < FD_BLD_MAX;
}

bool feud_build(FeudMatch *m, int side, int act, int t, int idx) {
    const FeudOffer *o;
    int n = feud_offers_of(m, side, act, &o);
    if (idx < 0 || idx >= n || !feud_can_build_on(m, side, t) || !feud_can_afford(m, side, &o[idx])) return false;
    m->s[side].res[o[idx].res] -= o[idx].cost;
    m->t[t].b[o[idx].what]++;
    return true;
}

bool feud_trade(FeudMatch *m, int side, int act, int idx) {
    const FeudOffer *o;
    int n = feud_offers_of(m, side, act, &o);
    if (idx < 0 || idx >= n) return false;
    if (m->s[side].res[o[idx].what] < o[idx].cost) return false;
    m->s[side].res[o[idx].what] -= o[idx].cost;
    feud_add_res(m, side, o[idx].res, o[idx].gain);
    return true;
}

int feud_movable(const FeudMatch *m, int t, int kind) {
    if (m->t[t].frozen) return 0;
    return imax(0, m->t[t].u[kind] - m->t[t].moved[kind]);
}

bool feud_can_move_from(const FeudMatch *m, int side, int t) {
    if (m->t[t].owner != side) return false;
    for (int k = 0; k < FU_KINDS; k++)
        if (feud_movable(m, t, k) > 0) return true;
    return false;
}

uint32_t feud_march_dests(const FeudMatch *m, int side, int src, int mflags) {
    uint32_t mask = 0;
    for (int t = 0; t < FD_TILES; t++) {
        if (!feud_adjacent(src, t, (mflags & MF_DIAG) != 0)) continue;
        if (m->t[t].terr == FT_PEAK && !(mflags & MF_THROUGH)) continue;
        mask |= 1u << t;
    }
    return mask;
}

/* the updraft: up to two tiles away, over anything, onto anything */
uint32_t feud_flight_dests(const FeudMatch *m, int side, int src) {
    uint32_t mask = 0;
    for (int t = 0; t < FD_TILES; t++)
        if (t != src && feud_dist(src, t) <= 2) mask |= 1u << t;
    return mask;
}

void feud_claim(FeudMatch *m, int side, int t, int got[FR_KINDS]) {
    static const int GIVE[FR_KINDS] = {12, 3, 4};
    FeudTile *x = &m->t[t];
    if (got) memset(got, 0, sizeof(int) * FR_KINDS);
    for (int i = 0; i < x->ndrop; i++) {
        int r = x->drop[i];
        int before = m->s[side].res[r];
        feud_add_res(m, side, r, GIVE[r]);
        if (got) got[r] += m->s[side].res[r] - before;
    }
    x->ndrop = 0;
}

int feud_move(FeudMatch *m, int side, int src, int dst, const uint8_t counts[FU_KINDS]) {
    FeudTile *a = &m->t[src], *d = &m->t[dst];
    memset(feud_last_got, 0, sizeof feud_last_got);
    for (int k = 0; k < FU_KINDS; k++) a->u[k] = (uint8_t)(a->u[k] - imin(a->u[k], counts[k]));
    if (d->owner != side && d->owner >= 0 && feud_units_on(m, dst) > 0) return 1;
    if (d->owner < 0) feud_claim(m, side, dst, feud_last_got);
    if (d->owner != side) {
        /* an empty enemy tile, or open ground: ours now, buildings and all */
        memset(d->u, 0, sizeof d->u);
        memset(d->moved, 0, sizeof d->moved);
        d->frozen = 0;
        d->seen = 0;
        d->owner = (int8_t)side;
    }
    for (int k = 0; k < FU_KINDS; k++) {
        int before = d->u[k];
        add_units(d, k, counts[k]);
        d->moved[k] = (uint8_t)imin(d->u[k], d->moved[k] + (d->u[k] - before));
    }
    feud_check_elimination(m);
    return 0;
}

void feud_reset_moved(FeudMatch *m) {
    for (int t = 0; t < FD_TILES; t++) memset(m->t[t].moved, 0, sizeof m->t[t].moved);
}

/* ---- the omens -------------------------------------------------------------- */

int feud_omen_cost(const FeudMatch *m, int side, int act, int omen) {
    const FeudOffer *o;
    int n = feud_offers_of(m, side, act, &o);
    for (int i = 0; i < n; i++)
        if (o[i].what == omen) return o[i].cost;
    return -1;
}

uint32_t feud_omen_targets(const FeudMatch *m, int side, int omen) {
    uint32_t mask = 0;
    for (int t = 0; t < FD_TILES; t++) {
        const FeudTile *x = &m->t[t];
        bool ok = false;
        switch (omen) {
        case OM_SHIFT: ok = x->terr == FT_PEAK; break;
        case OM_HAIL:
        case OM_BOLT: ok = x->owner == 1 - side && feud_units_on(m, t) > 0; break;
        case OM_SIGHT: ok = x->owner == 1 - side; break;
        case OM_UPDRAFT: ok = feud_can_move_from(m, side, t); break;
        case OM_HATCH: ok = feud_is_recruit_point(m, side, t); break;
        default: break;
        }
        if (ok) mask |= 1u << t;
    }
    return mask;
}

/* a peak moves anywhere that isn't a peak; the ground it leaves is scree */
uint32_t feud_shift_dests(const FeudMatch *m, int src) {
    uint32_t mask = 0;
    for (int t = 0; t < FD_TILES; t++)
        if (t != src && m->t[t].terr != FT_PEAK) mask |= 1u << t;
    return mask;
}

void feud_shift(FeudMatch *m, int src, int dst) {
    m->t[dst].terr = FT_PEAK; /* an eyrie under it keeps its totem, not its walls */
    m->t[src].terr = FT_SCREE;
}

int feud_bolt(FeudMatch *m, int t) {
    int n = 0;
    for (int k = 0; k < FU_KINDS; k++) {
        int kill = imin(2, m->t[t].u[k]);
        m->t[t].u[k] = (uint8_t)(m->t[t].u[k] - kill);
        m->t[t].moved[k] = (uint8_t)imin(m->t[t].moved[k], m->t[t].u[k]);
        n += kill;
    }
    return n;
}

int feud_hail(FeudMatch *m, int t) {
    m->t[t].frozen = 3; /* through their turn and to the end of your next */
    return feud_units_on(m, t);
}

int feud_hatch(FeudMatch *m, int side, int t) {
    /* one bird at random; the costly ones come likelier as the war drags on */
    int late = imin(12, m->turn / 3);
    int w[FU_KINDS] = {14, 2 + late, 7, 2 + late, 2 + late};
    int sum = 0;
    for (int k = 0; k < FU_KINDS; k++) sum += w[k];
    int roll = rng_range(&m->rng, 0, sum - 1), k = 0;
    while (roll >= w[k]) { roll -= w[k]; k++; }
    add_units(&m->t[t], k, 1);
    return k;
}

/* ---- turns and totems -------------------------------------------------------- */

void feud_check_elimination(FeudMatch *m) {
    if (m->winner >= 0) return;
    for (int s = 0; s < 2; s++)
        if (feud_tiles_held(m, s) == 0) { m->winner = (int8_t)(1 - s); return; }
}

void feud_start_turn(FeudMatch *m) {
    /* four totems held since the end of your last turn, through theirs: won */
    FeudSide *s = &m->s[m->side];
    if (m->winner < 0 && s->hold && feud_flags_held(m, m->side) >= FD_WIN_FLAGS) m->winner = (int8_t)m->side;
    feud_make_offers(m);
    feud_reset_moved(m);
}

void feud_end_turn(FeudMatch *m) {
    int side = m->side;
    m->s[side].hold = feud_flags_held(m, side) >= FD_WIN_FLAGS;
    for (int t = 0; t < FD_TILES; t++) {
        if (m->t[t].frozen) m->t[t].frozen--;
        if (side == 0) m->t[t].seen = 0;
    }
    m->side = 1 - side;
    m->turn++;
    feud_check_elimination(m);
    feud_start_turn(m);
}

/* ---- words ------------------------------------------------------------------- */

/* "1 TOILER", "3 TOILERS" (grain is grain) */
static const char *res_word(int r, int n) {
    static const char *const ONE[FR_KINDS] = {"GRAIN", "TOILER", "RELIC"};
    return n == 1 ? ONE[r] : FEUD_RES_NAME[r];
}

static void cat(char *buf, int n, const char *s) {
    int l = (int)strlen(buf);
    if (l < n - 1) snprintf(buf + l, (size_t)(n - l), "%s", s);
}

int feud_offer_text(const FeudMatch *m, int act, const FeudOffer *o, char *buf, int n) {
    switch (FEUD_ACT[act].kind) {
    case AK_ENLIST: return snprintf(buf, (size_t)n, "%s %d %s", FEUD_UNIT[o->what].name, o->cost, res_word(o->res, o->cost));
    case AK_BUILD: return snprintf(buf, (size_t)n, "%s %d %s", FEUD_BLD_NAME[o->what], o->cost, res_word(o->res, o->cost));
    case AK_BARTER:
        return snprintf(buf, (size_t)n, "%d %s FOR %d %s", o->cost, res_word(o->what, o->cost), o->gain, res_word(o->res, o->gain));
    case AK_OMEN: return snprintf(buf, (size_t)n, "%s %d %s", FEUD_OMEN_NAME[o->what], o->cost, res_word(FR_RELICS, o->cost));
    default: break;
    }
    (void)m;
    buf[0] = 0;
    return 0;
}

/* what an action does at a level, in a line or two of capitals */
int feud_level_text(const FeudMatch *m, int side, int act, int lvl, char *buf, int n) {
    const FeudActDef *d = &FEUD_ACT[act];
    const FeudLevel *l = &d->lv[iclamp(lvl, 0, 2)];
    char part[64];
    buf[0] = 0;
    switch (d->kind) {
    case AK_GATHER: {
        static const char *const W[5] = {"GRAIN", "GRAIN A GRANARY", "%s A DUSTBATH", "%s", "%s A SHRINE"};
        static const int R[5] = {FR_GRAIN, FR_GRAIN, FR_TOILERS, FR_RELICS, FR_RELICS};
        for (int i = 0; i < 5; i++) {
            if (!l->g[i]) continue;
            char w[40];
            snprintf(w, sizeof w, W[i], res_word(R[i], l->g[i]));
            snprintf(part, sizeof part, "%s+%d %s", buf[0] ? ", " : "", l->g[i], w);
            cat(buf, n, part);
        }
        break;
    }
    case AK_CALLUP: {
        static const char *const W[4] = {"STOMPERS A ROOST", "LANCER A ROOST", "MYSTIC A SHRINE", "SWIFT A DUSTBATH"};
        cat(buf, n, "FREE: ");
        for (int i = 0; i < 4; i++) {
            if (!l->g[i]) continue;
            snprintf(part, sizeof part, "%s%d %s", i ? ", " : "", l->g[i], W[i]);
            cat(buf, n, part);
        }
        break;
    }
    case AK_MARCH:
        cat(buf, n, "MOVE BIRDS A STEP");
        if (l->mflags & MF_DIAG) cat(buf, n, ", DIAGONALS TOO");
        if (l->mflags & MF_THROUGH) cat(buf, n, ", ONTO PEAKS");
        if (l->mflags & MF_SURPRISE) cat(buf, n, ", SURPRISE ATTACK");
        break;
    default: {
        const FeudOffer *o = l->off;
        int k = l->n;
        if (d->kind == AK_OMEN && m && !m->fog) { o = l->offf; k = l->nf; }
        for (int i = 0; i < k; i++) {
            if (d->kind == AK_OMEN) snprintf(part, sizeof part, "%s%s %d", i ? ", " : "", FEUD_OMEN_NAME[o[i].what], o[i].cost);
            else if (d->kind == AK_BARTER)
                snprintf(part, sizeof part, "%s%d %s FOR %d %s", i ? "; " : "", o[i].cost, res_word(o[i].what, o[i].cost), o[i].gain,
                         res_word(o[i].res, o[i].gain));
            else
                snprintf(part, sizeof part, "%s%s %d %s", i ? ", " : "",
                         d->kind == AK_ENLIST ? FEUD_UNIT[o[i].what].name : FEUD_BLD_NAME[o[i].what], o[i].cost,
                         res_word(o[i].res, o[i].cost));
            cat(buf, n, part);
        }
        if (d->kind == AK_OMEN) cat(buf, n, " RELICS");
        break;
    }
    }
    (void)side;
    return (int)strlen(buf);
}
