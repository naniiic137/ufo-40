/* BANNERFALL - the rules. Pure functions of a Board, no drawing.
 * Every rule here is listed with its source in docs/games/09-bannerfall.md. */
#include "bannerfall.h"

const UnitInfo BF_UNITS[U_TYPES] = {
    {"", 0, "", ""},
    {"FOOTMAN", 3, "IN A COLUMN OF 3 ALLIES", "SHRUGS OFF MELEE"},
    {"BOWMAN", 2, "SHOOTS DOWN ITS ROW", "AND HOLDS ITS GROUND"},
    {"WARDEN", 5, "AT 4+ HP: NO ATTACK,", "ARROWS BOUNCE OFF"},
    {"RIDER", 3, "MOVES AGAIN IF THE", "WAY AHEAD IS CLEAR"},
    {"PIKEMAN", 3, "HITS THE TWO TILES", "IN FRONT OF IT"},
    {"SHADE", 2, "THROWS KNIVES UP AND", "DOWN, THEN MARCHES"},
    {"POWDERMAN", 2, "BLOWS UP WHEN IT DIES,", "HURTING EVERYONE NEAR"},
    {"CHAMPION", 5, "PIKE REACH, RIDES", "TWICE. PROMOTED"},
};

/* The campaign: flags, pools and the CPU's extra units follow Attactics'
 * 24 levels one for one; the names are ours. */
const LevelDef BF_LEVELS_DEF[BF_LEVELS] = {
    {"MORNING MUSTER", 2, 2, "F", "F", 0, 0, "FOOTMEN IN A COLUMN OF THREE SHRUG OFF BLOWS."},
    {"BOWS AT DAWN", 3, 3, "BF", "FF", 50, 0, "BOWMEN SHOOT DOWN THEIR ROW, BUT NEVER PAST A FRIEND."},
    {"THE LONG FIELD", 3, 3, "BF", "BF", 30, 0, NULL},
    {"RAIN OF REEDS", 3, 3, "WFFF", "FBBB", 30, 0, "A WARDEN'S SHIELD STOPS ARROWS UNTIL IT BREAKS."},
    {"HOOFBEATS", 3, 3, "RBFF", "FFBR", 30, 0, "RIDERS RUN TWICE AS FAR WHEN THE WAY IS CLEAR."},
    {"FULL GALLOP", 5, 5, "RBFF", "FFBR", 50, 0, NULL},
    {"LAST BANNER", 1, 5, "WRBF", "FFBR", 50, 0, NULL},
    {"LONG REACH", 5, 5, "RBFFPP", "FFBRWP", 40, 0, "PIKEMEN STRIKE TWO TILES AHEAD."},
    {"HEDGE OF PIKES", 5, 5, "PPPFFW", "FFFFRR", 60, 0, NULL},
    {"MUD AND METTLE", 5, 5, "RBFFWP", "FFFBRP", 50, 0, NULL},
    {"NIGHT KNIVES", 5, 5, "BBFFPSSR", "FFBBRSSP", 40, 0, "SHADES THROW KNIVES UP AND DOWN AS THEY MARCH."},
    {"THORN RIDGE", 5, 5, "PPRR", "BBRR", 50, 0, NULL},
    {"THE FLOOD", 5, 5, "SRBFWP", "FFFFFF", 75, 0, NULL},
    {"NO REST", 5, 5, "RBFFWPS", "FFBBRSP", 50, 0, NULL},
    {"LIT FUSES", 5, 5, "RBFFWP", "FFFFRX", 40, 0, "POWDERMEN BLOW UP WHEN THEY FALL. KEEP CLEAR!"},
    {"SMOKE AND SPARKS", 5, 5, "WXPR", "FBRP", 40, 0, NULL},
    {"HOLLOW WOODS", 5, 5, "SRFFXX", "FFRPPW", 40, 0, NULL},
    {"CALL TO ARMS", 5, 5, "RBFFWXPS", "FFBRSPXW", 60, 0, NULL},
    {"CROWDED LANES", 5, 5, "RBFWWXPS", "FBRSPXWW", 60, 0, NULL},
    {"CHAMPIONS RISE", 5, 5, "WSBC", "FBRSPX", 100, 0, NULL},
    {"PLAIN STEEL", 5, 5, "FFFF", "FFBRSPXW", 20, 0, NULL},
    {"BARRELS AND BOWS", 5, 5, "XXBB", "SSPP", 60, 0, NULL},
    {"THE GREY FIELD", 5, 5, "RBFFWXPS", "FFBRSPXW", 70, 0, NULL},
    {"BANNERFALL", 5, 5, "RBFFWXPS", "FFBRSPXW", 80, 1, "EVERY FIFTH PROMOTION CALLS A CHAMPION!"},
};

static const char FULL_POOL[] = "RBFFWXPS";

/* ------------------------------------------------------------------ */
/* helpers                                                              */

static int dir_of(int side) { return side == SIDE_L ? 1 : -1; }
static int spawn_col(int side) { return side == SIDE_L ? 0 : BF_COLS - 1; }
static bool on_board(int x, int y) { return x >= 0 && y >= 0 && x < BF_COLS && y < BF_ROWS; }
static bool is_unit(const Board *b, int x, int y) { return on_board(x, y) && b->g[y][x].type != U_NONE; }
static bool is_enemy(const Board *b, int x, int y, int side) { return is_unit(b, x, y) && b->g[y][x].side != side; }

bool bf_in_zone(int side, int x) { return side == SIDE_L ? (x >= 0 && x < BF_ZONE) : (x >= BF_COLS - BF_ZONE && x < BF_COLS); }

static int letter_type(char c) {
    switch (c) {
    case 'F': return U_FOOT;
    case 'B': return U_BOW;
    case 'W': return U_WARD;
    case 'R': return U_RIDER;
    case 'P': return U_PIKE;
    case 'S': return U_SHADE;
    case 'X': return U_POWDER;
    case 'C': return U_CHAMP;
    default: return U_NONE;
    }
}

void bf_pool_from_letters(Army *a, const char *letters) {
    a->pool_n = 0;
    for (const char *p = letters; *p && a->pool_n < BF_POOL_MAX; p++) {
        int t = letter_type(*p);
        if (t) a->pool[a->pool_n++] = (uint8_t)t;
    }
}

static bool same_pool(const Army *l, const Army *r) {
    int cl[U_TYPES] = {0}, cr[U_TYPES] = {0};
    for (int i = 0; i < l->pool_n; i++) cl[l->pool[i]]++;
    for (int i = 0; i < r->pool_n; i++) cr[r->pool[i]]++;
    for (int t = 0; t < U_TYPES; t++)
        if (cl[t] != cr[t]) return false;
    return l->pool_n > 0;
}

int bf_count(const Board *b, int side, int type) {
    int n = 0;
    for (int y = 0; y < BF_ROWS; y++)
        for (int x = 0; x < BF_COLS; x++) {
            const Unit *u = &b->g[y][x];
            if (u->type && u->side == side && (type == U_NONE || u->type == type)) n++;
        }
    return n;
}

int bf_timer_counts(const Board *b) {
    if (b->turn >= 60) return 4; /* double quick (Attactics: triple time) */
    if (b->turn >= 40) return 6; /* quick march (Attactics: double time) */
    return 9;
}

int bf_rank_change(int flags_l, int flags_r, int status) {
    if (status == BF_WIN_L) return flags_l >= 3 ? 10 : flags_l == 2 ? 4 : 2;
    if (status == BF_WIN_R) return flags_r >= 3 ? -5 : flags_r == 2 ? -3 : -1;
    return 0;
}

int bf_survival_stage(int turn) { return turn >= 80 ? 4 : turn >= 60 ? 3 : turn >= 40 ? 2 : turn >= 20 ? 1 : 0; }

static void add_ev(BEvents *ev, int type, int phase, int x, int y, int x2, int y2, int a, int bb, int id) {
    if (!ev || ev->n >= (int)(sizeof ev->ev / sizeof ev->ev[0])) return;
    BEvent *e = &ev->ev[ev->n++];
    e->type = (uint8_t)type;
    e->phase = (uint8_t)phase;
    e->x = (int8_t)x; e->y = (int8_t)y; e->x2 = (int8_t)x2; e->y2 = (int8_t)y2;
    e->a = (uint8_t)a; e->b = (uint8_t)bb;
    e->id = (uint16_t)id;
}

/* ------------------------------------------------------------------ */
/* setup                                                                */

void bf_setup(Board *b, int mode, uint64_t seed) {
    memset(b, 0, sizeof *b);
    b->mode = (uint8_t)mode;
    b->next_id = 1;
    b->status = BF_PLAYING;
    rng_seed(&b->rng, seed);
    for (int s = 0; s < 2; s++) {
        b->a[s].flags = 3;
        b->a[s].start_flags = 3;
    }
}

void bf_setup_level(Board *b, int level, uint64_t seed) {
    const LevelDef *d = &BF_LEVELS_DEF[iclamp(level, 0, BF_LEVELS - 1)];
    bf_setup(b, MODE_CAMPAIGN, seed);
    b->a[SIDE_L].flags = b->a[SIDE_L].start_flags = d->flags_l;
    b->a[SIDE_R].flags = b->a[SIDE_R].start_flags = d->flags_r;
    bf_pool_from_letters(&b->a[SIDE_L], d->pool_l);
    bf_pool_from_letters(&b->a[SIDE_R], d->pool_r);
    b->a[SIDE_R].handicap = d->handicap;
    b->a[SIDE_L].heroes = d->heroes;
    b->matched = same_pool(&b->a[SIDE_L], &b->a[SIDE_R]);
}

void bf_setup_ranked(Board *b, int rank, uint64_t seed) {
    bf_setup(b, MODE_RANKED, seed);
    bf_pool_from_letters(&b->a[SIDE_L], FULL_POOL);
    bf_pool_from_letters(&b->a[SIDE_R], FULL_POOL);
    b->a[SIDE_R].handicap = (uint16_t)imax(0, rank);
    b->a[SIDE_L].heroes = 1;
    b->matched = 1;
}

void bf_setup_survival(Board *b, uint64_t seed) {
    bf_setup(b, MODE_SURVIVAL, seed);
    bf_pool_from_letters(&b->a[SIDE_L], FULL_POOL);
    bf_pool_from_letters(&b->a[SIDE_R], FULL_POOL);
    b->a[SIDE_R].flags = b->a[SIDE_R].start_flags = BF_NO_FLAGS;
    b->a[SIDE_L].heroes = 1;
}

/* ------------------------------------------------------------------ */
/* spawning                                                             */

static int free_row(Board *b, int col) {
    int rows[BF_ROWS], n = 0;
    for (int y = 0; y < BF_ROWS; y++)
        if (!b->g[y][col].type) rows[n++] = y;
    if (n == 0) return -1;
    return rows[rng_range(&b->rng, 0, n - 1)];
}

static bool place(Board *b, int side, int type, BEvents *ev, int phase) {
    int col = spawn_col(side);
    int y = free_row(b, col);
    if (y < 0) return false;
    Unit *u = &b->g[y][col];
    u->type = (uint8_t)type;
    u->side = (uint8_t)side;
    u->hp = BF_UNITS[type].hp;
    u->promo = type == U_CHAMP;
    u->id = b->next_id++;
    add_ev(ev, BE_SPAWN, phase, col, y, col, y, side, type, u->id);
    return true;
}

static int pick(Board *b, const Army *a) {
    if (a->pool_n == 0) return U_FOOT;
    return a->pool[rng_range(&b->rng, 0, a->pool_n - 1)];
}

void bf_spawn(Board *b, BEvents *ev) {
    b->turn++;
    /* survival: the enemy brings more and more units */
    if (b->mode == MODE_SURVIVAL) b->a[SIDE_R].handicap = (uint16_t)(5 * (b->turn - 1));
    int n[2];
    for (int s = 0; s < 2; s++) {
        Army *a = &b->a[s];
        n[s] = 1 + a->taken;
        a->acc = (uint16_t)(a->acc + a->handicap);
        while (a->acc >= 100) { n[s]++; a->acc = (uint16_t)(a->acc - 100); }
    }
    bool hero[2] = {b->a[0].hero_due != 0, b->a[1].hero_due != 0};
    /* the CPU / right army's spawns first; with matching pools the player's
     * normal spawn copies one of them (not on a turn where only one side
     * summons a champion) */
    int types_r[16], nr = imin(n[1], 16);
    for (int i = 0; i < nr; i++) types_r[i] = pick(b, &b->a[1]);
    int types_l[16], nl = imin(n[0], 16);
    for (int i = 0; i < nl; i++) types_l[i] = pick(b, &b->a[0]);
    if (b->matched && hero[0] == hero[1]) types_l[0] = types_r[rng_range(&b->rng, 0, nr - 1)];
    if (hero[0]) { types_l[0] = U_CHAMP; b->a[0].hero_due = 0; }
    if (hero[1]) { types_r[0] = U_CHAMP; b->a[1].hero_due = 0; }
    for (int i = 0; i < nl; i++) place(b, SIDE_L, types_l[i], ev, 0);
    for (int i = 0; i < nr; i++) place(b, SIDE_R, types_r[i], ev, 0);
}

/* ------------------------------------------------------------------ */
/* combat                                                               */

typedef struct Hit {
    int8_t ax, ay, tx, ty;
    uint8_t dmg, kind; /* kind: 0 melee, 1 arrow, 2 knife, 3 blast */
} Hit;
enum { K_MELEE, K_ARROW, K_KNIFE, K_BLAST };

typedef struct Combat {
    Hit h[128];
    int n;
} Combat;

static int atk_dmg(const Unit *u) { return u->promo ? 2 : 1; }
static bool in_column3(const Board *b, int x, int y);

/* A unit in a vertical line of 3+ allies (itself included). */
bool bf_in_column(const Board *b, int x, int y) { return b->g[y][x].type && in_column3(b, x, y); }

static bool in_column3(const Board *b, int x, int y) {
    const Unit *u = &b->g[y][x];
    int n = 1;
    for (int yy = y - 1; yy >= 0 && b->g[yy][x].type && b->g[yy][x].side == u->side; yy--) n++;
    for (int yy = y + 1; yy < BF_ROWS && b->g[yy][x].type && b->g[yy][x].side == u->side; yy++) n++;
    return n >= 3;
}

static void add_hit(Combat *c, int ax, int ay, int tx, int ty, int dmg, int kind) {
    if (c->n >= (int)(sizeof c->h / sizeof c->h[0])) return;
    c->h[c->n++] = (Hit){(int8_t)ax, (int8_t)ay, (int8_t)tx, (int8_t)ty, (uint8_t)dmg, (uint8_t)kind};
}

static void promote(Board *b, Unit *u, int x, int y, BEvents *ev, int phase) {
    if (u->promo) return;
    u->promo = 1;
    add_ev(ev, BE_PROMOTE, phase, x, y, x, y, u->side, u->type, u->id);
    Army *a = &b->a[u->side];
    if (a->heroes) {
        a->promos++;
        if (a->promos >= 5) { a->promos = 0; a->hero_due = 1; }
    }
}

/* Apply every hit at once, then deaths, promotions and powder blasts. */
static void apply_hits(Board *b, Combat *c, BEvents *ev, int phase) {
    int dmg[BF_ROWS][BF_COLS];
    memset(dmg, 0, sizeof dmg);
    bool col3[BF_ROWS][BF_COLS];
    for (int y = 0; y < BF_ROWS; y++)
        for (int x = 0; x < BF_COLS; x++) col3[y][x] = b->g[y][x].type == U_FOOT && in_column3(b, x, y);
    for (int i = 0; i < c->n; i++) {
        Hit *h = &c->h[i];
        Unit *t = &b->g[h->ty][h->tx];
        if (!t->type) { h->dmg = 0; continue; }
        int d = h->dmg;
        /* the footman column: no melee damage unless the attacker is a footman in a column too */
        if (h->kind == K_MELEE && col3[h->ty][h->tx]) {
            bool att_col = on_board(h->ax, h->ay) && col3[h->ay][h->ax];
            if (!att_col) d = 0;
        }
        /* a warden at 4+ hp stops arrows */
        if (h->kind == K_ARROW && t->type == U_WARD && t->hp >= 4) d = 0;
        h->dmg = (uint8_t)d;
        dmg[h->ty][h->tx] += d;
    }
    for (int y = 0; y < BF_ROWS; y++)
        for (int x = 0; x < BF_COLS; x++) {
            Unit *t = &b->g[y][x];
            if (!t->type) continue;
            bool targeted = false;
            for (int i = 0; i < c->n; i++)
                if (c->h[i].tx == x && c->h[i].ty == y) targeted = true;
            if (!targeted) continue;
            add_ev(ev, BE_DAMAGE, phase, x, y, x, y, dmg[y][x], t->side, t->id);
            t->hp = (uint8_t)imax(0, t->hp - dmg[y][x]);
        }
    /* deaths: killers that are still standing are promoted */
    bool dead[BF_ROWS][BF_COLS];
    for (int y = 0; y < BF_ROWS; y++)
        for (int x = 0; x < BF_COLS; x++) dead[y][x] = b->g[y][x].type && b->g[y][x].hp == 0;
    for (int i = 0; i < c->n; i++) {
        Hit *h = &c->h[i];
        if (h->dmg == 0 || h->kind == K_BLAST || !on_board(h->ax, h->ay)) continue;
        if (!dead[h->ty][h->tx]) continue;
        Unit *a = &b->g[h->ay][h->ax];
        if (a->type && !dead[h->ay][h->ax]) promote(b, a, h->ax, h->ay, ev, phase);
    }
    /* remove the dead; powdermen go off, and blasts can kill more */
    for (int pass = 0; pass < 8; pass++) {
        bool any = false;
        Combat blast;
        blast.n = 0;
        for (int y = 0; y < BF_ROWS; y++)
            for (int x = 0; x < BF_COLS; x++) {
                Unit *t = &b->g[y][x];
                if (!t->type || t->hp > 0) continue;
                any = true;
                add_ev(ev, BE_DEATH, phase, x, y, x, y, t->side, t->type, t->id);
                if (t->type == U_POWDER) {
                    int d = dir_of(t->side);
                    add_ev(ev, BE_BLAST, phase, x, y, x + d, y, t->side, 3, t->id);
                    for (int dy = -1; dy <= 1; dy++)
                        for (int k = 0; k <= 1; k++) {
                            int bx = x + d * k, by = y + dy;
                            if ((bx != x || by != y) && is_unit(b, bx, by) && b->g[by][bx].hp > 0)
                                add_hit(&blast, -1, -1, bx, by, 3, K_BLAST);
                        }
                }
                memset(t, 0, sizeof *t);
            }
        if (!any || blast.n == 0) break;
        for (int i = 0; i < blast.n; i++) {
            Unit *t = &b->g[blast.h[i].ty][blast.h[i].tx];
            if (!t->type) continue;
            add_ev(ev, BE_DAMAGE, phase, blast.h[i].tx, blast.h[i].ty, 0, 0, 3, t->side, t->id);
            t->hp = (uint8_t)imax(0, t->hp - 3);
        }
    }
}

/* ------------------------------------------------------------------ */
/* the end of a turn                                                    */

static void keep_hit(Board *b, int side, const Unit *u, BEvents *ev, int phase, int from_y) {
    Army *foe = &b->a[side ^ 1];
    add_ev(ev, BE_KEEP, phase, side == SIDE_L ? BF_COLS : -1, from_y, 0, 0, side, u->type, u->id);
    b->keep_hits[side]++;
    if (foe->flags != BF_NO_FLAGS && foe->flags > 0) {
        foe->flags--;
        b->a[side].taken++;
    }
    /* the unit comes back on its own side */
    int col = spawn_col(side);
    int y = free_row(b, col);
    if (y >= 0) {
        b->g[y][col] = *u;
        add_ev(ev, BE_REAPPEAR, phase, col, y, col, y, side, u->type, u->id);
    }
}

void bf_resolve(Board *b, BEvents *ev, Board snaps[PH_COUNT + 1]) {
    if (snaps) snaps[0] = *b;
    bool acted[BF_ROWS][BF_COLS]; /* attacked this turn (by position during phase 2) */
    memset(acted, 0, sizeof acted);

    /* 1. knives */
    Combat c;
    c.n = 0;
    for (int y = 0; y < BF_ROWS; y++)
        for (int x = 0; x < BF_COLS; x++) {
            Unit *u = &b->g[y][x];
            if (u->type != U_SHADE) continue;
            /* like an arrow, but up and down: the first unit that way must be a foe */
            for (int dy = -1; dy <= 1; dy += 2)
                for (int yy = y + dy; yy >= 0 && yy < BF_ROWS; yy += dy) {
                    if (!is_unit(b, x, yy)) continue;
                    if (b->g[yy][x].side != u->side) {
                        add_hit(&c, x, y, x, yy, atk_dmg(u), K_KNIFE);
                        add_ev(ev, BE_KNIFE, PH_KNIVES, x, y, x, yy, u->promo, u->side, u->id);
                    }
                    break;
                }
        }
    if (c.n) apply_hits(b, &c, ev, PH_KNIVES);
    if (snaps) snaps[1] = *b;

    /* 2. attacks */
    c.n = 0;
    for (int y = 0; y < BF_ROWS; y++)
        for (int x = 0; x < BF_COLS; x++) {
            Unit *u = &b->g[y][x];
            if (!u->type) continue;
            int d = dir_of(u->side), s = u->side;
            if (u->type == U_WARD && u->hp >= 4) continue;
            if (u->type == U_PIKE || u->type == U_CHAMP) {
                for (int k = 1; k <= 2; k++)
                    if (is_enemy(b, x + d * k, y, s)) {
                        add_hit(&c, x, y, x + d * k, y, atk_dmg(u), K_MELEE);
                        add_ev(ev, BE_REACH, PH_ATTACK, x, y, x + d * k, y, u->promo, s, u->id);
                        acted[y][x] = true;
                    }
                continue;
            }
            if (is_enemy(b, x + d, y, s)) {
                add_hit(&c, x, y, x + d, y, atk_dmg(u), K_MELEE);
                add_ev(ev, BE_MELEE, PH_ATTACK, x, y, x + d, y, u->promo, s, u->id);
                acted[y][x] = true;
                continue;
            }
            /* a bowman shoots only when the first unit down its row is a foe:
             * an ally in front blocks the shot */
            if (u->type == U_BOW)
                for (int xx = x + d; xx >= 0 && xx < BF_COLS; xx += d) {
                    if (!is_unit(b, xx, y)) continue;
                    if (b->g[y][xx].side != s) {
                        add_hit(&c, x, y, xx, y, atk_dmg(u), K_ARROW);
                        add_ev(ev, BE_ARROW, PH_ATTACK, x, y, xx, y, u->promo, s, u->id);
                        acted[y][x] = true;
                    }
                    break;
                }
        }
    /* remember who attacked by id: the dead are removed before moving */
    uint16_t acted_id[BF_ROWS * BF_COLS];
    int n_acted = 0;
    for (int y = 0; y < BF_ROWS; y++)
        for (int x = 0; x < BF_COLS; x++)
            if (acted[y][x]) acted_id[n_acted++] = b->g[y][x].id;
    if (c.n) apply_hits(b, &c, ev, PH_ATTACK);
    if (snaps) snaps[2] = *b;

    /* 3. moves. First the clashes: two foes stepping into the same free tile
     * hit each other as usual, and then the one with the longer line of its
     * own men behind it pushes into the tile; an even line holds both still. */
    uint16_t stay_id[BF_ROWS * BF_COLS];
    int n_stay = 0;
    c.n = 0;
    for (int y = 0; y < BF_ROWS; y++)
        for (int x = 0; x + 2 < BF_COLS; x++) {
            Unit *l = &b->g[y][x], *r = &b->g[y][x + 2];
            if (!l->type || l->side != SIDE_L || b->g[y][x + 1].type || !r->type || r->side != SIDE_R) continue;
            bool l_acted = false, r_acted = false;
            for (int i = 0; i < n_acted; i++) {
                if (acted_id[i] == l->id) l_acted = true;
                if (acted_id[i] == r->id) r_acted = true;
            }
            if (l_acted || r_acted) continue;
            int nl = 0, nr = 0;
            for (int xx = x; xx >= 0 && b->g[y][xx].type && b->g[y][xx].side == SIDE_L; xx--) nl++;
            for (int xx = x + 2; xx < BF_COLS && b->g[y][xx].type && b->g[y][xx].side == SIDE_R; xx++) nr++;
            int winner = nl > nr ? 1 : nr > nl ? 2 : 0;
            add_ev(ev, BE_CLASH, PH_MOVE, x, y, x + 2, y, winner, 0, l->id);
            if (!(l->type == U_WARD && l->hp >= 4)) add_hit(&c, x, y, x + 2, y, atk_dmg(l), K_MELEE);
            if (!(r->type == U_WARD && r->hp >= 4)) add_hit(&c, x + 2, y, x, y, atk_dmg(r), K_MELEE);
            if (winner != 1) stay_id[n_stay++] = l->id;
            if (winner != 2) stay_id[n_stay++] = r->id;
        }
    if (c.n) apply_hits(b, &c, ev, PH_MOVE);

    Unit entered[2][BF_ROWS * 2];
    int entered_y[2][BF_ROWS * 2], n_entered[2] = {0, 0};
    uint16_t moved_id[BF_ROWS * BF_COLS];
    int n_moved = 0;
    for (int y = 0; y < BF_ROWS; y++) {
        bool want[BF_COLS];
        for (int x = 0; x < BF_COLS; x++) {
            Unit *u = &b->g[y][x];
            want[x] = false;
            if (!u->type) continue;
            bool held = false;
            for (int i = 0; i < n_acted; i++)
                if (acted_id[i] == u->id) held = true;
            for (int i = 0; i < n_stay; i++)
                if (stay_id[i] == u->id) held = true;
            want[x] = !held;
        }
        /* the left army marches right, front first */
        for (int x = BF_COLS - 1; x >= 0; x--) {
            Unit *u = &b->g[y][x];
            if (!u->type || u->side != SIDE_L || !want[x]) continue;
            if (x + 1 >= BF_COLS) {
                entered[0][n_entered[0]] = *u;
                entered_y[0][n_entered[0]++] = y;
                add_ev(ev, BE_MOVE, PH_MOVE, x, y, x + 1, y, u->side, u->type, u->id);
                memset(u, 0, sizeof *u);
            } else if (!b->g[y][x + 1].type) {
                add_ev(ev, BE_MOVE, PH_MOVE, x, y, x + 1, y, u->side, u->type, u->id);
                moved_id[n_moved++] = u->id;
                b->g[y][x + 1] = *u;
                memset(u, 0, sizeof *u);
            }
        }
        /* the right army marches left, front first */
        for (int x = 0; x < BF_COLS; x++) {
            Unit *u = &b->g[y][x];
            if (!u->type || u->side != SIDE_R || !want[x]) continue;
            if (x - 1 < 0) {
                entered[1][n_entered[1]] = *u;
                entered_y[1][n_entered[1]++] = y;
                add_ev(ev, BE_MOVE, PH_MOVE, x, y, x - 1, y, u->side, u->type, u->id);
                memset(u, 0, sizeof *u);
            } else if (!b->g[y][x - 1].type) {
                add_ev(ev, BE_MOVE, PH_MOVE, x, y, x - 1, y, u->side, u->type, u->id);
                moved_id[n_moved++] = u->id;
                b->g[y][x - 1] = *u;
                memset(u, 0, sizeof *u);
            }
        }
    }
    if (snaps) snaps[3] = *b;

    /* 4. keep attacks */
    for (int s = 0; s < 2; s++)
        for (int i = 0; i < n_entered[s]; i++) keep_hit(b, s, &entered[s][i], ev, PH_KEEP, entered_y[s][i]);
    if (snaps) snaps[4] = *b;

    /* 5. riders and champions that moved go again if the way is clear */
    for (int y = 0; y < BF_ROWS; y++) {
        bool go[BF_COLS];
        for (int x = 0; x < BF_COLS; x++) {
            Unit *u = &b->g[y][x];
            go[x] = false;
            if (u->type != U_RIDER && u->type != U_CHAMP) continue;
            for (int i = 0; i < n_moved; i++)
                if (moved_id[i] == u->id) go[x] = true;
        }
        for (int x = 0; x + 2 < BF_COLS; x++)
            if (go[x] && b->g[y][x].side == SIDE_L && !b->g[y][x + 1].type && go[x + 2] && b->g[y][x + 2].side == SIDE_R)
                go[x] = go[x + 2] = false;
        for (int x = BF_COLS - 1; x >= 0; x--) {
            Unit *u = &b->g[y][x];
            if (!go[x] || u->side != SIDE_L) continue;
            if (x + 1 >= BF_COLS) {
                Unit copy = *u;
                add_ev(ev, BE_MOVE, PH_MOVE2, x, y, x + 1, y, u->side, u->type, u->id);
                memset(u, 0, sizeof *u);
                keep_hit(b, SIDE_L, &copy, ev, PH_MOVE2, y);
            } else if (!b->g[y][x + 1].type) {
                add_ev(ev, BE_MOVE, PH_MOVE2, x, y, x + 1, y, u->side, u->type, u->id);
                b->g[y][x + 1] = *u;
                memset(u, 0, sizeof *u);
            }
        }
        for (int x = 0; x < BF_COLS; x++) {
            Unit *u = &b->g[y][x];
            if (!go[x] || !u->type || u->side != SIDE_R) continue;
            if (x - 1 < 0) {
                Unit copy = *u;
                add_ev(ev, BE_MOVE, PH_MOVE2, x, y, x - 1, y, u->side, u->type, u->id);
                memset(u, 0, sizeof *u);
                keep_hit(b, SIDE_R, &copy, ev, PH_MOVE2, y);
            } else if (!b->g[y][x - 1].type) {
                add_ev(ev, BE_MOVE, PH_MOVE2, x, y, x - 1, y, u->side, u->type, u->id);
                b->g[y][x - 1] = *u;
                memset(u, 0, sizeof *u);
            }
        }
    }
    if (snaps) snaps[5] = *b;

    /* the end of the turn */
    bool l_out = b->a[SIDE_L].flags == 0;
    bool r_out = b->a[SIDE_R].flags == 0;
    if (b->mode == MODE_SURVIVAL) {
        if (l_out) b->status = BF_WIN_R;
        else b->score += 1u + b->keep_hits[SIDE_L];
    } else if (l_out && r_out) {
        b->status = BF_DRAW;
    } else if (r_out) {
        b->status = BF_WIN_L;
    } else if (l_out) {
        b->status = BF_WIN_R;
    }
}

/* ------------------------------------------------------------------ */
/* the player's hand                                                    */

bool bf_drag(Board *b, int side, int *x, int *y, int dx, int dy, int pickup_col) {
    int nx = *x + dx, ny = *y + dy;
    if (!on_board(nx, ny) || !bf_in_zone(side, nx)) return false;
    if ((nx - pickup_col) * dir_of(side) > 0) return false; /* not past where it was picked up */
    Unit *t = &b->g[ny][nx];
    if (t->type && t->side != side) return false;
    Unit tmp = *t;
    *t = b->g[*y][*x];
    b->g[*y][*x] = tmp;
    *x = nx;
    *y = ny;
    return true;
}
