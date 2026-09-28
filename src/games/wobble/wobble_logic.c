/* WOBBLE DERBY - the rules: the book of wobblers, the odds, the punters'
 * moves, the race itself, the payouts and the rival punters' thinking.
 * No drawing here. See docs/games/47-wobble-derby.md for where each rule
 * comes from. */
#include "wobble.h"

const uint8_t WB_LENGTH[WB_LENGTHS] = {3, 6, 9, 12};

/* The book. Clumsiness and speed follow the original's table band for band
 * (five of each stability band and each speed band, and one wild one); the
 * names, colours and markings are ours. Markings: 0 plain, 1 spots,
 * 2 stripes, 3 mask, 4 tuft. */
const WbDef WB_DEF[WB_RACERS] = {
    {"PUDDING",    3, 4,  45, 55, C_TAN,     C_BROWN,  1},
    {"HOTFOOT",    7, 8,  55, 65, C_RED,     C_YELLOW, 2},
    {"SNIFFLES",   5, 6,  40, 50, C_LIME,    C_FOREST, 0},
    {"BIG CHEESE", 5, 6,  50, 60, C_YELLOW,  C_AMBER,  1},
    {"ZOOMER",     9, 10, 55, 65, C_ORANGE,  C_INK,    2},
    {"CHECKERS",   9, 10, 45, 55, C_WHITE,   C_INK,    1},
    {"THE DUKE",   1, 2,  55, 65, C_PURPLE,  C_YELLOW, 4},
    {"WEASEL",     7, 8,  40, 50, C_HIDE,    C_EARTH,  3},
    {"TORNADO",    1, 2,  45, 55, C_SKY,     C_WHITE,  2},
    {"SUNNY",      1, 2,  40, 50, C_AMBER,   C_CREAM,  0},
    {"BUMBLES",    9, 10, 35, 45, C_YELLOW,  C_INK,    2},
    {"COMET",      5, 6,  55, 65, C_BLUE,    C_CYAN,   4},
    {"SPARKY",     7, 8,  45, 55, C_CYAN,    C_YELLOW, 3},
    {"THISTLE",    7, 8,  50, 60, C_VIOLET,  C_JADE,   4},
    {"DUCHESS",    1, 2,  50, 60, C_PINK,    C_WHITE,  4},
    {"OLD TUBBY",  1, 2,  35, 45, C_GREY,    C_LIGHT,  0},
    {"HICCUP",     7, 8,  35, 45, C_MAGENTA, C_PINK,   1},
    {"PEEWEE",     3, 4,  35, 45, C_LEAF,    C_LIME,   1},
    {"HOWLER",     3, 4,  40, 50, C_TEAL,    C_ICE,    3},
    {"MIDNIGHT",   3, 4,  50, 60, C_NAVY,    C_VIOLET, 0},
    {"FLURRY",     9, 10, 50, 60, C_ICE,     C_WHITE,  1},
    {"FRECKLES",   9, 10, 40, 50, C_CREAM,   C_ORANGE, 1},
    {"ACE HIGH",   3, 4,  55, 65, C_RED,     C_WHITE,  4},
    {"BEAKY",      5, 6,  45, 55, C_JADE,    C_AMBER,  0},
    {"NOODLE",     5, 6,  35, 45, C_WINE,    C_PINK,   2},
    {"DICEY",      1, 10, 35, 65, C_SLATE,   C_WHITE,  1},
};

const WbChar WB_CHAR[WB_CHARS] = {
    {"GLOOB",      "A GREEN BLOB WITH A HOT TIP"},
    {"VEXA",       "SHE READS THE FORM BOOK IN BED"},
    {"ZORP",       "ONE EYE, TWO WALLETS"},
    {"MADAME OOZ", "PEARLS, SLIME AND NERVES OF STEEL"},
    {"K-7",        "A ROBOT WHO COUNTS EVERYTHING"},
    {"NIBBS",      "SMALL, GREY AND ALWAYS BROKE"},
    {"BARON FUZZ", "OLD MONEY, SHORT TEMPER"},
    {"QUILLA",     "A BIRD WHO NEVER BLINKS"},
    {"MO",         "A MOLE UP FROM THE MINE"},
    {"TILLY",      "A FARM ROBOT ON HER NIGHT OFF"},
    {"PEPPER",     "A COURIER CAT WITH HER WAGES"},
    {"FOXY",       "SHE LIKES THINGS FAST"},
    {"POSY",       "THE PALACE GARDENER, IN DISGUISE"},
    {"KIP",        "A SCRAP-DIVER WITH SCRAP MONEY"},
    {"TWIG",       "YOU WON'T SEE HER COMING"},
    {"WADE",       "SPENDING SOMEBODY ELSE'S MONEY"},
};

const int WB_JOB_COST[J_COUNT] = {0, 50, 20, 50, 80, 120, 50};
const char *const WB_JOB_NAME[J_COUNT] = {"", "PEP SNACK", "PEEL TOSS", "NOBBLE", "FIZZ PILLS", "NIGHTSHADE", "MINDER"};

/* the biggest bet of each round; the last round has none */
static const int CAPS[] = {200, 300, 500, 750, 1000, 1500, 2000, 2500, 3000, 3500, 4000};

int wb_cap_of(int round, int nraces) {
    if (round >= nraces - 1) return 0;
    if (round < 0) round = 0;
    if (round >= ARRAY_LEN(CAPS)) round = ARRAY_LEN(CAPS) - 1;
    return CAPS[round];
}
int wb_cap(const WbGame *g) { return wb_cap_of(g->round, g->nraces); }
bool wb_last_round(const WbGame *g) { return g->round >= g->nraces - 1; }

/* the lender lends the round's cap; in the open last round, the one before */
int wb_loan_size(const WbGame *g) {
    int c = wb_cap(g);
    return c ? c : wb_cap_of(g->round - 1, g->nraces);
}
/* the tip booth charges a tenth of what may be bet */
int wb_tip_fee(const WbGame *g) {
    int f = wb_loan_size(g) / 10;
    return f < 10 ? 10 : f;
}
int wb_sponsor_fee(const WbGame *g, int racer) { return 100 + 50 * g->r[racer].wins; }

int wb_spd_lo(const WbGame *g, int racer) { return imax(10, WB_DEF[racer].spd_lo + g->r[racer].mod); }
int wb_spd_hi(const WbGame *g, int racer) { return imax(12, WB_DEF[racer].spd_hi + g->r[racer].mod); }
bool wb_active(const WbGame *g, int racer) { return g->r[racer].out == OUT_NO; }

int wb_sponsored_count(const WbGame *g, int p) {
    int n = 0;
    for (int i = 0; i < WB_RACERS; i++) n += g->r[i].sponsor == p + 1 && wb_active(g, i);
    return n;
}

/* ------------------------------------------------------------------ */
/* the race simulation                                                  */

/* a lane's pace this tick, in 1/256 px */
static const int16_t FIZZ_PACE[6] = {0, 40, 100, 170, 230, -40};

static void lane_setup(const WbGame *g, WbRace *rc, int lane, Rng *rng) {
    int racer = rc->field[lane];
    WbLane *L = &rc->lane[lane];
    memset(L, 0, sizeof *L);
    L->form = (int16_t)rng_range(rng, wb_spd_lo(g, racer), wb_spd_hi(g, racer));
    L->stab = (uint8_t)rng_range(rng, WB_DEF[racer].stab_lo, WB_DEF[racer].stab_hi);
    L->fizz_f = 100;
}

static void add_litter(WbLane *L, int n, Rng *rng) {
    for (int i = 0; i < n && L->nlitter < WB_MAX_LITTER; i++)
        L->litter[L->nlitter++] = (uint16_t)rng_range(rng, 40, WB_TRACK_PX - 24);
}

static void event_setup(WbRace *rc, Rng *rng) {
    if (rc->event == EV_SPILL)
        for (int l = 0; l < WB_FIELD; l++) add_litter(&rc->lane[l], 3, rng);
    if (rc->event == EV_SMOG)
        for (int l = 0; l < WB_FIELD; l++) rc->lane[l].form = (int16_t)(rc->lane[l].form * 85 / 100);
    if (rc->event == EV_METEORS) {
        rc->nmet = (uint8_t)rng_range(rng, 4, WB_MAX_METEORS);
        for (int i = 0; i < rc->nmet; i++) {
            WbMeteor *m = &rc->met[i];
            m->lane = (uint8_t)rng_range(rng, 0, WB_FIELD - 1);
            m->land = (uint16_t)rng_range(rng, 120, 560);
            /* aimed near where that lane's runner should be by then */
            int px = rc->lane[m->lane].form * 2 * m->land / 256;
            m->x = (int16_t)iclamp(px + rng_range(rng, -26, 26), 30, WB_TRACK_PX - 10);
        }
    }
}

static void trip(WbLane *L) {
    if (L->trip > 0) return;
    L->trip = WB_TRIP_TICKS;
    if (L->trips < 255) L->trips++;
}

/* one tick of the race; true while it is still running */
static bool race_tick(WbRace *rc, Rng *rng) {
    if (rc->over) return false;
    rc->t++;
    /* meteors land */
    for (int i = 0; i < rc->nmet; i++) {
        WbMeteor *m = &rc->met[i];
        if (m->land != rc->t) continue;
        WbLane *L = &rc->lane[m->lane];
        if (L->dead || L->done) continue;
        int d = iabs(L->x / 256 - m->x);
        if (d <= 6) L->dead = OUT_METEOR;
        else if (d <= 18) trip(L);
    }
    int crossed = -1, crossed_x = 0;
    int smog = rc->event == EV_SMOG ? 2 : 1;
    for (int l = 0; l < WB_FIELD; l++) {
        WbLane *L = &rc->lane[l];
        if (L->dead || L->done) continue;
        if (L->poison_at && rc->t >= L->poison_at) { L->dead = OUT_COLLAPSED; continue; }
        int v = L->form * 2;
        if (L->fizz) {
            if (rc->t % 30 == 1) L->fizz_f = FIZZ_PACE[rng_range(rng, 0, 5)];
            v = v * L->fizz_f / 100;
        }
        if (L->trip > 0) {
            L->trip--;
            v = v / 5;                     /* tumbling on, still moving */
        } else if ((int)(rng_next(rng) % 10000) < L->stab * 5 * smog) {
            trip(L);
        }
        int old_px = L->x / 256;
        L->x += v;
        if (L->x < 0) L->x = 0;
        int new_px = L->x / 256;
        for (int i = 0; i < L->nlitter; i++) {
            if (L->litter_hit & (1u << i)) continue;
            if (L->litter[i] > old_px && L->litter[i] <= new_px) {
                L->litter_hit |= (uint16_t)(1u << i);
                if (rng_range(rng, 0, 99) < 10 + 9 * L->stab) trip(L);
            }
        }
        if (L->x >= WB_TRACK) {
            L->done = 1;
            L->finish_t = rc->t;
            if (rc->winner == WB_NONE && (crossed < 0 || L->x > crossed_x)) { crossed = l; crossed_x = L->x; }
        }
    }
    if (crossed >= 0) rc->winner = (uint8_t)crossed;
    bool running = false;
    for (int l = 0; l < WB_FIELD; l++) running |= !rc->lane[l].dead && !rc->lane[l].done;
    if (!running || rc->t > 4000) { rc->over = 1; rc->over_t = rc->t; }
    return !rc->over;
}

/* ------------------------------------------------------------------ */
/* odds                                                                 */

static int win_chance_mil(const WbGame *g, const uint8_t *field, int lane) {
    int w[WB_FIELD], sum = 0;
    for (int l = 0; l < WB_FIELD; l++) {
        const WbRacer *r = &g->r[field[l]];
        w[l] = (r->wins + 1) * 3000 / (r->races + 3);
        sum += w[l];
    }
    return sum ? w[lane] * 1000 / sum : 333;
}

/* X : 1 from the record, with the track keeping a tenth */
int wb_odds_for(const WbGame *g, const uint8_t *field, int lane) {
    int p = win_chance_mil(g, field, lane);
    if (p < 20) p = 20;
    int x = ((1000 - p) * 900 / p + 500) / 1000;
    return iclamp(x, 1, 20);
}

/* ------------------------------------------------------------------ */
/* setting up                                                           */

static void deal_field(WbGame *g, uint8_t *field) {
    int pool[WB_RACERS], n = 0;
    for (int i = 0; i < WB_RACERS; i++)
        if (wb_active(g, i)) pool[n++] = i;
    for (int l = 0; l < WB_FIELD; l++) {
        int k = rng_range(&g->rng, l, n - 1);
        int t = pool[l]; pool[l] = pool[k]; pool[k] = t;
        field[l] = (uint8_t)pool[l];
    }
}

void wb_start_round(WbGame *g) {
    WbRace *rc = &g->race;
    memset(rc, 0, sizeof *rc);
    rc->winner = WB_NONE;
    deal_field(g, rc->field);
    for (int l = 0; l < WB_FIELD; l++) rc->odds[l] = (uint8_t)wb_odds_for(g, rc->field, l);
    /* the weather: rare, and the tip booth knows it in advance */
    rc->event = rng_range(&g->rng, 0, 99) < 9 ? (uint8_t)rng_range(&g->rng, EV_METEORS, EV_SMOG) : EV_NONE;
    for (int p = 0; p < WB_PLAYERS; p++) {
        WbPlayer *P = &g->pl[p];
        P->bet_on = WB_NONE;
        P->bet = 0;
        P->job = J_NONE;
        P->job_lane = 0;
        P->tips = 0;
        P->trains = 0;
        P->won = P->bonus = P->fine = P->interest = 0;
    }
    for (int i = 0; i < WB_RACERS; i++) g->r[i].trained = 0;
    g->turn = 0;
    g->secret = 0;
    g->nnews = 0;
}

void wb_new(WbGame *g, int nraces, int humans, const uint8_t *chars, uint64_t seed) {
    memset(g, 0, sizeof *g);
    rng_seed(&g->rng, seed);
    g->nraces = (uint8_t)nraces;
    g->humans = (uint8_t)iclamp(humans, 1, WB_PLAYERS);
    for (int p = 0; p < WB_PLAYERS; p++) {
        g->pl[p].human = p < g->humans;
        g->pl[p].chr = chars ? chars[p] : (uint8_t)p;
        g->pl[p].cash = WB_START_CASH;
        g->pl[p].bet_on = WB_NONE;
    }
    /* the form book: twenty races nobody sees, run on the same rules
     * (no weather and no fixing) */
    for (int i = 0; i < WB_PRERACES; i++) {
        WbRace rc;
        memset(&rc, 0, sizeof rc);
        rc.winner = WB_NONE;
        deal_field(g, rc.field);
        for (int l = 0; l < WB_FIELD; l++) lane_setup(g, &rc, l, &g->rng);
        while (race_tick(&rc, &g->rng)) {}
        for (int l = 0; l < WB_FIELD; l++) g->r[rc.field[l]].races++;
        if (rc.winner != WB_NONE) g->r[rc.field[rc.winner]].wins++;
    }
    wb_start_round(g);
}

/* ------------------------------------------------------------------ */
/* the punters' moves                                                   */

bool wb_bet(WbGame *g, int p, int lane, int amount) {
    WbPlayer *P = &g->pl[p];
    if (lane < 0 || lane >= WB_FIELD || amount < 0) return false;
    int cap = wb_cap(g);
    int have = P->cash + (P->bet_on != WB_NONE ? P->bet : 0);
    if (amount > have || (cap && amount > cap)) return false;
    P->cash = have - amount;
    P->bet = amount;
    P->bet_on = amount > 0 ? (uint8_t)lane : WB_NONE;
    if (amount == 1990) g->secret = 1;
    return true;
}

bool wb_tip(WbGame *g, int p, int lane) {
    WbPlayer *P = &g->pl[p];
    int fee = wb_tip_fee(g);
    if (lane < 0 || lane >= WB_FIELD || (P->tips & (1 << lane)) || P->cash < fee) return false;
    P->cash -= fee;
    P->tips |= (uint8_t)(1 << lane);
    return true;
}

bool wb_job(WbGame *g, int p, int job, int lane) {
    WbPlayer *P = &g->pl[p];
    if (job <= J_NONE || job >= J_COUNT || lane < 0 || lane >= WB_FIELD) return false;
    if (P->job != J_NONE || P->cash < WB_JOB_COST[job]) return false;
    P->cash -= WB_JOB_COST[job];
    P->job = (uint8_t)job;
    P->job_lane = (uint8_t)lane;
    return true;
}

bool wb_borrow(WbGame *g, int p) {
    WbPlayer *P = &g->pl[p];
    if (P->debt > 0) return false;
    int n = wb_loan_size(g);
    P->debt = n;
    P->cash += n;
    return true;
}

bool wb_repay(WbGame *g, int p) {
    WbPlayer *P = &g->pl[p];
    if (P->debt <= 0 || P->cash < P->debt) return false;
    P->cash -= P->debt;
    P->debt = 0;
    return true;
}

bool wb_sponsor(WbGame *g, int p, int racer) {
    WbPlayer *P = &g->pl[p];
    if (racer < 0 || racer >= WB_RACERS || !wb_active(g, racer) || g->r[racer].sponsor) return false;
    if (wb_sponsored_count(g, p) >= WB_MAX_SPONSOR) return false;
    int fee = wb_sponsor_fee(g, racer);
    if (P->cash < fee) return false;
    P->cash -= fee;
    g->r[racer].sponsor = (uint8_t)(p + 1);
    return true;
}

bool wb_train(WbGame *g, int p, int racer) {
    WbPlayer *P = &g->pl[p];
    if (racer < 0 || racer >= WB_RACERS || g->r[racer].sponsor != p + 1 || !wb_active(g, racer)) return false;
    if (g->r[racer].trained || P->trains >= WB_TRAINS_A_ROUND || P->cash < WB_TRAIN_COST) return false;
    P->cash -= WB_TRAIN_COST;
    P->trains++;
    g->r[racer].trained = 1;
    g->r[racer].mod = (int8_t)imin(g->r[racer].mod + WB_TRAIN_GAIN, 60);
    return true;
}

/* ------------------------------------------------------------------ */
/* the race, for real                                                   */

static void news(WbGame *g, int kind, int a, int b, int32_t v) {
    if (g->nnews >= WB_NEWS_MAX) return;
    g->news[g->nnews++] = (WbNews){(uint8_t)kind, (uint8_t)a, (uint8_t)b, v};
}

static void take(WbPlayer *P, int32_t n) {
    /* what can't be paid in cash goes on the slate */
    if (P->cash >= n) P->cash -= n;
    else { P->debt += n - P->cash; P->cash = 0; }
}

void wb_race_begin(WbGame *g) {
    WbRace *rc = &g->race;
    if (rc->started) return;
    rc->started = 1;
    for (int l = 0; l < WB_FIELD; l++) lane_setup(g, rc, l, &g->rng);
    /* a minder on the lane stops every job but a peel toss */
    for (int p = 0; p < WB_PLAYERS; p++)
        if (g->pl[p].job == J_MINDER) rc->lane[g->pl[p].job_lane].minded = 1;
    for (int p = 0; p < WB_PLAYERS; p++) {
        WbPlayer *P = &g->pl[p];
        int j = P->job, l = P->job_lane;
        if (j == J_NONE || j == J_MINDER) continue;
        WbLane *L = &rc->lane[l];
        int racer = rc->field[l];
        if (j == J_PEEL) { add_litter(L, 3, &g->rng); continue; }
        if (L->minded) {
            int fine = WB_JOB_COST[j] * (j == J_NIGHTSHADE ? 2 : 1);
            take(P, fine);
            P->fine += fine;
            news(g, NW_FINE, p, racer, fine);
            continue;
        }
        switch (j) {
        case J_PEP: L->pep = 1; L->form = (int16_t)(L->form + WB_PEP_GAIN); break;
        case J_NOBBLE:
            L->nobbled = 1;
            L->form = (int16_t)imax(10, L->form - WB_NOBBLE_LOSS);
            g->r[racer].mod = (int8_t)imax(g->r[racer].mod - WB_NOBBLE_LOSS, -40);
            break;
        case J_FIZZ: L->fizz = 1; break;
        case J_NIGHTSHADE:
            /* it doesn't always work */
            if (rng_range(&g->rng, 0, 99) < 60) L->poison_at = (uint16_t)rng_range(&g->rng, 150, 520);
            break;
        }
    }
    event_setup(rc, &g->rng);
}

void wb_race_step(WbGame *g) {
    if (!g->race.started) wb_race_begin(g);
    race_tick(&g->race, &g->rng);
}

void wb_race_run(WbGame *g) {
    if (!g->race.started) wb_race_begin(g);
    while (race_tick(&g->race, &g->rng)) {}
}

void wb_race_settle(WbGame *g) {
    WbRace *rc = &g->race;
    wb_race_run(g);
    int w = rc->winner;
    if (w != WB_NONE) {
        news(g, NW_WIN, rc->field[w], w, rc->odds[w]);
        /* the rest of the order: finishers by time, then by how far they got */
        int ord[WB_FIELD], n = 0;
        for (int l = 0; l < WB_FIELD; l++)
            if (l != w && !rc->lane[l].dead) ord[n++] = l;
        if (n == 2) {
            const WbLane *a = &rc->lane[ord[0]], *b = &rc->lane[ord[1]];
            bool swap = a->done && b->done ? b->finish_t < a->finish_t : b->x > a->x;
            if (swap) { int t = ord[0]; ord[0] = ord[1]; ord[1] = t; }
            news(g, NW_PLACES, rc->field[ord[0]], rc->field[ord[1]], 2);
        } else if (n == 1) news(g, NW_PLACES, rc->field[ord[0]], 0, 1);
    } else news(g, NW_NOWIN, 0, 0, 0);
    if (rc->event) news(g, NW_EVENT, rc->event, 0, 0);
    for (int l = 0; l < WB_FIELD; l++) {
        int racer = rc->field[l];
        WbRacer *R = &g->r[racer];
        if (R->races < 255) R->races++;
        if ((int)l == w && R->wins < 255) R->wins++;
        if (rc->lane[l].dead) {
            R->out = rc->lane[l].dead;
            R->sponsor = 0;
            news(g, rc->lane[l].dead == OUT_METEOR ? NW_METEOR_DEAD : NW_COLLAPSED, racer, 0, 0);
        } else {
            if (rc->lane[l].trips >= 3) news(g, NW_TRIPS, racer, rc->lane[l].trips, 0);
            /* a race takes it out of them */
            if (rng_range(&g->rng, 0, 1)) R->mod = (int8_t)imax(R->mod - 1, -40);
        }
    }
    for (int p = 0; p < WB_PLAYERS; p++) {
        WbPlayer *P = &g->pl[p];
        if (P->bet_on != WB_NONE) {
            if (P->bet_on == w) { P->won = P->bet * (rc->odds[w] + 1); P->cash += P->won; }
            else P->won = -P->bet;
        }
    }
    if (w != WB_NONE) {
        int s = g->r[rc->field[w]].sponsor;
        if (s) {
            WbPlayer *P = &g->pl[s - 1];
            P->cash += WB_SPONSOR_BONUS;
            P->bonus += WB_SPONSOR_BONUS;
            if (P->sponsor_wins < 255) P->sponsor_wins++;
            news(g, NW_SPONSOR, s - 1, rc->field[w], WB_SPONSOR_BONUS);
        }
    }
    /* the lender's fifteen per cent, every round a loan is out (the last
     * race's debt is simply collected) */
    if (!wb_last_round(g))
        for (int p = 0; p < WB_PLAYERS; p++) {
            WbPlayer *P = &g->pl[p];
            if (P->debt > 0) {
                int32_t nd = (P->debt * (100 + WB_INTEREST) + 50) / 100;
                P->interest = nd - P->debt;
                P->debt = nd;
            }
        }
    /* old-timers hang up their boots now and then */
    int active = 0;
    for (int i = 0; i < WB_RACERS; i++) active += wb_active(g, i);
    for (int i = 0; i < WB_RACERS && active > 8; i++) {
        if (!wb_active(g, i) || g->r[i].races < 6) continue;
        bool ran = false;
        for (int l = 0; l < WB_FIELD; l++) ran |= rc->field[l] == i;
        if (ran || rng_range(&g->rng, 0, 99) >= 4) continue;
        g->r[i].out = OUT_RETIRED;
        g->r[i].sponsor = 0;
        active--;
        news(g, NW_RETIRED, i, 0, 0);
    }
    if (g->secret) news(g, NW_SECRET, 0, 0, 1990);
}

bool wb_next_round(WbGame *g) {
    if (g->round + 1 >= g->nraces) { g->done = 1; return false; }
    g->round++;
    wb_start_round(g);
    return true;
}

int wb_final(const WbGame *g, int p) { return g->pl[p].cash - g->pl[p].debt; }

int wb_rank(const WbGame *g, int p) {
    int r = 1;
    for (int q = 0; q < WB_PLAYERS; q++)
        if (q != p && wb_final(g, q) > wb_final(g, p)) r++;
    return r;
}

/* ------------------------------------------------------------------ */
/* judging a race: win chances in 1/1000                                */

void wb_estimate(const WbGame *g, int *pmil, bool know_stats, uint32_t seed) {
    if (!know_stats) {
        for (int l = 0; l < WB_FIELD; l++) pmil[l] = win_chance_mil(g, g->race.field, l);
        return;
    }
    int wins[WB_FIELD] = {0, 0, 0};
    Rng r;
    rng_seed(&r, 0x5eedu + seed);
    const int N = 60;
    for (int i = 0; i < N; i++) {
        WbRace rc;
        memset(&rc, 0, sizeof rc);
        rc.winner = WB_NONE;
        memcpy(rc.field, g->race.field, sizeof rc.field);
        rc.event = g->race.event;
        for (int l = 0; l < WB_FIELD; l++) lane_setup(g, &rc, l, &r);
        event_setup(&rc, &r);
        while (race_tick(&rc, &r)) {}
        if (rc.winner != WB_NONE) wins[rc.winner]++;
    }
    for (int l = 0; l < WB_FIELD; l++) pmil[l] = wins[l] * 1000 / N;
}

/* ------------------------------------------------------------------ */
/* the rival punters                                                    */

static int best_rival_lane(const int *est, int not_lane) {
    int b = -1;
    for (int l = 0; l < WB_FIELD; l++)
        if (l != not_lane && (b < 0 || est[l] > est[b])) b = l;
    return b;
}

void wb_cpu_turn(WbGame *g, int p) {
    WbPlayer *P = &g->pl[p];
    Rng *rng = &g->rng;
    bool last = wb_last_round(g);
    int leader = 1;
    for (int q = 0; q < WB_PLAYERS; q++)
        if (q != p && wb_final(g, q) >= wb_final(g, p)) leader = 0;
    if (P->debt > 0 && P->cash >= P->debt + 150) wb_repay(g, p);
    /* a look at the tip booth when it's affordable */
    int fee = wb_tip_fee(g);
    bool know = P->cash >= fee * 3 + 60 && rng_range(rng, 0, 99) < 40;
    if (know)
        for (int l = 0; l < WB_FIELD; l++) wb_tip(g, p, l);
    int est[WB_FIELD];
    wb_estimate(g, est, know, rng_next(rng));
    int pick = 0, best_ev = -1;
    for (int l = 0; l < WB_FIELD; l++) {
        int ev = est[l] * (g->race.odds[l] + 1) * (80 + rng_range(rng, 0, 40)) / 100;
        if (ev > best_ev) { best_ev = ev; pick = l; }
    }
    /* behind at the end: borrow and go all in on a price worth it */
    if (last && !leader && P->debt == 0 && g->race.odds[pick] >= 2) wb_borrow(g, p);
    /* sponsoring early on, and the coach */
    if (g->round * 2 < g->nraces && wb_sponsored_count(g, p) < 2 && rng_range(rng, 0, 99) < 30) {
        int b = -1, bs = -1;
        for (int i = 0; i < WB_RACERS; i++) {
            if (!wb_active(g, i) || g->r[i].sponsor || g->r[i].races < 2) continue;
            int s = (g->r[i].wins + 1) * 1000 / (g->r[i].races + 2);
            if (s > bs) { bs = s; b = i; }
        }
        if (b >= 0 && P->cash >= wb_sponsor_fee(g, b) + 250) wb_sponsor(g, p, b);
    }
    for (int i = 0; i < WB_RACERS; i++)
        if (g->r[i].sponsor == p + 1 && P->cash >= WB_TRAIN_COST + 250 && rng_range(rng, 0, 99) < 60) wb_train(g, p, i);
    /* the fixer */
    if (rng_range(rng, 0, 99) < (last ? 75 : 45)) {
        int rival = best_rival_lane(est, pick);
        int roll = rng_range(rng, 0, 99);
        int job, lane;
        if (est[pick] >= 450 && roll < 35) { job = J_MINDER; lane = pick; }
        else if (roll < 50) { job = J_NIGHTSHADE; lane = rival; }
        else if (roll < 68) { job = J_NOBBLE; lane = rival; }
        else if (roll < 80) { job = J_FIZZ; lane = rival; }
        else if (roll < 92) { job = J_PEP; lane = pick; }
        else { job = J_PEEL; lane = rival; }
        if (P->cash >= WB_JOB_COST[job] + 30) wb_job(g, p, job, lane);
    }
    /* the bet */
    int cap = wb_cap(g);
    int room = cap ? imin(cap, P->cash) : P->cash;
    int amt;
    if (last && !leader) amt = room;
    else if (best_ev >= 1300) amt = room * rng_range(rng, 75, 100) / 100;
    else if (best_ev >= 1050) amt = room * rng_range(rng, 35, 60) / 100;
    else amt = room * rng_range(rng, 8, 20) / 100;
    if (!cap && leader) amt = imin(amt, P->cash / 2);
    amt = amt / 10 * 10;
    if (amt < 10 && P->cash >= 10) amt = 10;
    if (amt == 1990) amt = 1980;
    if (amt > 0) wb_bet(g, p, pick, amt);
}
