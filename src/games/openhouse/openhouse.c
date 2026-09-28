/* OPEN HOUSE - summer parties in a whitewashed house by the sea.
 * Cartridge 25 of UFO 40, a tribute to Party House (UFO 50 #25).
 * See docs/games/25-open-house.md. The rules live in openhouse_logic.c;
 * this file is the menus, the party and shop screens, saving and the flow. */
#include "openhouse.h"
#include <stddef.h>

enum {
    S_TITLE, S_MODE, S_SCEN, S_INTRO, S_TURN, S_PARTY, S_TARGET, S_FETCH, S_PEEK, S_CONFIRM,
    S_BUST, S_BAN, S_RESULT, S_SHOP, S_WIN, S_LOSE, S_LIST, S_CODE, S_RPICK, S_CUSTOM,
    S_LEGEND   /* the icon guide (the owner's) */
};
/* S_PEEK is no longer used: a peek shows the guest at the door by the door
 * itself and the party goes on (the owner's); the number stays for the tests */

typedef struct Save {
    uint32_t magic;
    uint8_t won;          /* a bit per set scenario, bit 5 for any Random win */
    uint8_t streak, best_streak, has_run, run_phase;
    uint8_t code[3];      /* the last Random code (bit 23: typed in by hand) */
    uint16_t random_wins, parties;
    uint16_t night_best[PH_BASES]; /* OPEN ALL NIGHT, per list played: the most star parties in one run */
    uint16_t night_fame[PH_BASES]; /* OPEN ALL NIGHT, per list played: the most fame held at once */
    /* the custom list (the owner's) */
    uint8_t cset[8];      /* a bit per guest type in it (the locked ones always) */
    uint8_t cset_made;    /* 0: never set up; it starts as list 1's guests */
    uint8_t cendless;     /* play it as OPEN ALL NIGHT */
    uint8_t cfastest;     /* its own best: the fewest nights to a win, 0 none yet */
    uint8_t ban_tag;      /* card + 1 banned from the party after this one (or tonight's, once it
                             started); shown as a red BANNED tag. It was a spare byte */
    uint16_t cwins;       /* its own record: wins (they never count for the goals) */
    PhGame run;
} Save;
#define SAVE_MAGIC 0x50480004u

/* the third save layout (before the custom list): a smaller shop and one
 * best fewer for OPEN ALL NIGHT; read once and carried over */
#define SAVE_MAGIC_V3 0x50480003u
typedef struct PhGameV3 {
    uint8_t scen, npool, pool[PH_POOL_MAX_V3], bought[G_COUNT], players, turn, winner, done;
    Rng rng;
    PhPlayer pl[2];
    PhParty party;
    uint8_t goal, star_party;
    uint16_t score, nights;
    uint8_t strikes;
    uint16_t top_pop;
    uint8_t base;
} PhGameV3;
typedef struct SaveV3 {
    uint32_t magic;
    uint8_t won, streak, best_streak, has_run, run_phase;
    uint8_t code[3];
    uint16_t random_wins, parties;
    uint16_t night_best[PH_BASES_V3], night_fame[PH_BASES_V3];
    PhGameV3 run;
} SaveV3;

/* the second save layout (before the owner's two guests): the same size,
 * but the stock bought had 46 guests, so the bytes after it sat two earlier */
#define SAVE_MAGIC_V2 0x50480002u
typedef struct PhGameV2 {
    uint8_t scen, npool, pool[PH_POOL_MAX_V3], bought[PH_GUESTS_V2], players, turn, winner, done;
    Rng rng;
    PhPlayer pl[2];
    PhParty party;
    uint8_t goal, star_party;
    uint16_t score, nights;
    uint8_t strikes;
    uint16_t top_pop;
    uint8_t base;
} PhGameV2;
typedef struct SaveV2 {
    uint32_t magic;
    uint8_t won, streak, best_streak, has_run, run_phase;
    uint8_t code[3];
    uint16_t random_wins, parties;
    uint16_t night_best[PH_BASES_V3], night_fame[PH_BASES_V3];
    PhGameV2 run;
} SaveV2;

/* the first save layout (before OPEN ALL NIGHT): read once and carried over */
#define SAVE_MAGIC_V1 0x50480001u
typedef struct PhGameV1 {
    uint8_t scen, npool, pool[PH_POOL_MAX_V1], bought[PH_GUESTS_V2], players, turn, winner, done;
    Rng rng;
    PhPlayer pl[2];
    PhParty party;
} PhGameV1;
typedef struct SaveV1 {
    uint32_t magic;
    uint8_t won, streak, best_streak, has_run, run_phase;
    uint8_t code[3];
    uint16_t random_wins, parties;
    PhGameV1 run;
} SaveV1;

static Save sv;
static PhGame G;
static int state, state_t, frame_t, back_state;
static int cur = 100, sel, tcur, fcur, menu_sel, scen_sel, mode_players = 1;
static uint8_t fetch_list[G_COUNT];
static int nfetch;
static int arrive[PH_MAX_CARDS];   /* frame each guest walked in, for the walk from the door */
static int info_card = -1;         /* what the info panel describes */
static int msg_t;
static const char *msg;
static int run_code = -1;          /* the Random list's six-digit code, -1 for a set list */
static int code_dig[6], code_cur;  /* typing a code in */
static int shop_cur;               /* the shop grid: guests, then SPACE, NEXT PARTY, GUEST BOOK */
static bool scen_endless;          /* the list grid is picking a list for OPEN ALL NIGHT */
static int shop_fresh;             /* times OPEN ALL NIGHT's big mix has turned over (tests) */
static int legend_back;            /* where the icon guide goes back to */
static uint8_t ban_tag[2];         /* each player's banned card + 1, from the ban to the end of the party they miss */
static int fire_by = -1;           /* the house slot of the guest whose friend didn't fit (the fire marshal) */
static bool msg_red;               /* the message is a red "can't" */

#define SLOT_W 26
#define SLOT_H 28
#define GRID_X 8
#define GRID_Y 22
#define PANEL_X 220
#define CUR_DOOR 100
#define CUR_END 101
#define CUR_BOOK 102
#define CUR_AWAY 103   /* TURN AWAY, under the door while a peeked guest waits */

/* ------------------------------------------------------------------ */
/* save & goals                                                         */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    if (G.players == 1 && sv.has_run) sv.run = G;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

/* the last Random code (plus one, 0 = none) lives in three spare bytes of
 * the save; bit 23 marks a code typed in by hand, which doesn't count for
 * the streak (a known list could be replayed until it wins) */
#define CODE_TYPED 0x800000
static int code_get(void) {
    int v = (sv.code[0] | sv.code[1] << 8 | sv.code[2] << 16) & 0x7fffff;
    return v ? v - 1 : -1;
}
static bool code_typed(void) { return (sv.code[2] & 0x80) != 0; }
static void code_put(int code, bool typed) {
    int v = (code + 1) | (typed ? CODE_TYPED : 0);
    sv.code[0] = (uint8_t)v; sv.code[1] = (uint8_t)(v >> 8); sv.code[2] = (uint8_t)(v >> 16);
}
static uint64_t code_seed(int code) { return (uint64_t)(code + 1) * 0x9E3779B97F4A7C15ull ^ 0x2525u; }

static void game_from_v1(PhGame *g, const PhGameV1 *o) {
    memset(g, 0, sizeof *g);
    g->scen = o->scen;
    g->npool = o->npool > PH_POOL_MAX_V1 ? PH_POOL_MAX_V1 : o->npool;
    memcpy(g->pool, o->pool, PH_POOL_MAX_V1);
    memcpy(g->bought, o->bought, sizeof o->bought);
    g->players = o->players;
    g->turn = o->turn;
    g->winner = o->winner;
    g->done = o->done;
    g->rng = o->rng;
    memcpy(g->pl, o->pl, sizeof g->pl);
    g->party = o->party;
    g->party.jinx = 0;
    g->goal = 4;
}

static void game_from_v2(PhGame *g, const PhGameV2 *o) {
    memset(g, 0, sizeof *g);
    g->scen = o->scen;
    g->npool = o->npool > PH_POOL_MAX_V3 ? PH_POOL_MAX_V3 : o->npool;
    memcpy(g->pool, o->pool, sizeof o->pool);
    memcpy(g->bought, o->bought, sizeof o->bought);
    g->players = o->players;
    g->turn = o->turn;
    g->winner = o->winner;
    g->done = o->done;
    g->rng = o->rng;
    memcpy(g->pl, o->pl, sizeof g->pl);
    g->party = o->party;
    g->party.jinx = 0;
    g->goal = o->goal;
    g->star_party = o->star_party;
    g->score = o->score;
    g->nights = o->nights;
    g->strikes = o->strikes;
    g->top_pop = o->top_pop;
    g->base = o->base;
}

/* the third layout had a shop of 24 at most; everything else is the same */
static void game_from_v3(PhGame *g, const PhGameV3 *o) {
    memset(g, 0, sizeof *g);
    g->scen = o->scen;
    g->npool = o->npool > PH_POOL_MAX_V3 ? PH_POOL_MAX_V3 : o->npool;
    memcpy(g->pool, o->pool, sizeof o->pool);
    memcpy(g->bought, o->bought, sizeof o->bought);
    g->players = o->players;
    g->turn = o->turn;
    g->winner = o->winner;
    g->done = o->done;
    g->rng = o->rng;
    memcpy(g->pl, o->pl, sizeof g->pl);
    g->party = o->party;
    g->goal = o->goal;
    g->star_party = o->star_party;
    g->score = o->score;
    g->nights = o->nights;
    g->strikes = o->strikes;
    g->top_pop = o->top_pop;
    g->base = o->base;
}

/* the albatross's flag took a padding byte and the stock grew into padding:
 * a save from before still has the same size and layout past them */
_Static_assert(offsetof(PhParty, jinx) == offsetof(PhParty, wild) + PH_MAX_CARDS, "jinx sits in the old padding");
_Static_assert(offsetof(PhParty, peek) == offsetof(PhParty, jinx) + 1, "the party keeps its layout");
_Static_assert(offsetof(PhParty, overflow) == offsetof(PhParty, over) + 1 &&
               offsetof(PhParty, got_pop) == offsetof(PhParty, over) + 2, "overflow sits in the old padding");
_Static_assert(sizeof(PhGameV2) == sizeof(PhGameV3) && offsetof(PhGameV2, rng) == offsetof(PhGameV3, rng), "same run size");
_Static_assert(sizeof(SaveV2) == sizeof(SaveV3), "same save size");
_Static_assert(G_COUNT <= 8 * sizeof(((Save *)0)->cset), "a bit per guest");

/* the start of every layout: the lists won, the streak and the rest */
#define COPY_HEAD(o) do { \
        memset(&sv, 0, sizeof sv); \
        sv.magic = SAVE_MAGIC; \
        sv.won = (o).won; \
        sv.streak = (o).streak; \
        sv.best_streak = (o).best_streak; \
        sv.has_run = (o).has_run; \
        sv.run_phase = (o).run_phase; \
        memcpy(sv.code, (o).code, sizeof sv.code); \
        sv.random_wins = (o).random_wins; \
        sv.parties = (o).parties; \
    } while (0)

static bool custom_in(int type) { return (sv.cset[type >> 3] >> (type & 7)) & 1; }
static void custom_put(int type, bool in) {
    if (in) sv.cset[type >> 3] |= (uint8_t)(1 << (type & 7));
    else sv.cset[type >> 3] &= (uint8_t)~(1 << (type & 7));
}

/* the custom list starts as list 1's guests; the locked ones are always in */
static void custom_fix(void) {
    if (!sv.cset_made) {
        memset(sv.cset, 0, sizeof sv.cset);
        for (int i = 0; i < PH_SCEN[0].n; i++) custom_put(PH_SCEN[0].pool[i], true);
        sv.cset_made = 1;
    }
    for (int t = 0; t < G_COUNT; t++)
        if (ph_custom_locked(t)) custom_put(t, true);
    for (int t = G_COUNT; t < 8 * (int)sizeof sv.cset; t++) custom_put(t, false);
}

static void load_save(void) {
    static union { Save now; SaveV3 v3; SaveV2 v2; SaveV1 v1; } tmp;
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp.now && tmp.now.magic == SAVE_MAGIC) sv = tmp.now;
    else if (n == (int)sizeof tmp.v3 && tmp.v3.magic == SAVE_MAGIC_V3) {
        /* a save from before the custom list keeps everything */
        static SaveV3 o;
        o = tmp.v3;
        COPY_HEAD(o);
        memcpy(sv.night_best, o.night_best, sizeof o.night_best);
        memcpy(sv.night_fame, o.night_fame, sizeof o.night_fame);
        game_from_v3(&sv.run, &o.run);
    } else if (n == (int)sizeof tmp.v2 && tmp.v2.magic == SAVE_MAGIC_V2) {
        /* a save from before the owner's guests keeps everything */
        static SaveV2 o;
        o = tmp.v2;
        COPY_HEAD(o);
        memcpy(sv.night_best, o.night_best, sizeof o.night_best);
        memcpy(sv.night_fame, o.night_fame, sizeof o.night_fame);
        game_from_v2(&sv.run, &o.run);
    } else if (n == (int)sizeof tmp.v1 && tmp.v1.magic == SAVE_MAGIC_V1) {
        /* an older save keeps its lists won, streak, code and run */
        static SaveV1 o;
        o = tmp.v1;
        COPY_HEAD(o);
        game_from_v1(&sv.run, &o.run);
    } else { memset(&sv, 0, sizeof sv); sv.magic = SAVE_MAGIC; }
    custom_fix();
}

#define SCEN_ROWS (PH_CUSTOM + 1)   /* the five lists, Random, OPEN ALL NIGHT, the custom list */

/* every list is open from the start (the owner's): a win only turns its
 * tile gold (BEATEN) and counts for the goals */
static bool beaten(int scen) {
    if (scen == PH_CUSTOM) return sv.cwins > 0;
    return scen <= PH_RANDOM && ((sv.won >> scen) & 1);
}

static void check_goals(void) {
    if (sv.won) game_award(GOAL_BEACON);
    if ((sv.won & 31) == 31) game_award(GOAL_SAUCER);
    if (sv.best_streak >= 5) game_award(GOAL_ALIEN);
}

/* ------------------------------------------------------------------ */
/* flow                                                                 */

/* a new run: nobody is banned */
static void clear_bans(void) { ban_tag[0] = ban_tag[1] = 0; sv.ban_tag = 0; }

static void say(const char *m) { msg = m; msg_t = 90; msg_red = false; }
/* a red notice: something that can't happen */
static void say_red(const char *m) { say(m); msg_red = true; }
static void shop_home(void);
static int endless_prev_best;      /* OPEN ALL NIGHT's best when this run began */

/* OPEN ALL NIGHT keeps its best as it goes, so quitting doesn't lose it */
static void note_endless_best(void) {
    if (G.scen != PH_ENDLESS || G.players != 1) return;
    int b = G.base % PH_BASES;
    if (G.score > sv.night_best[b]) sv.night_best[b] = G.score;
    if (G.top_pop > sv.night_fame[b]) sv.night_fame[b] = G.top_pop;
    save_now();
}

/* ------------------------------------------------------------------ */
/* the tally (the owner's): after a party ends well, each guest's fame and  */
/* cash is counted up in turn with a tick; holding A speeds it up.          */
/* The rules have already paid everyone (ph_end_party); this only shows it, */
/* in the original's order: every guest's fame and income, then the guests */
/* who charge are paid (or cost 7 fame).                                    */

enum { TL_WAIT, TL_EARN, TL_PAY, TL_DONE };
enum { WHY_HAND, WHY_FULL, WHY_NOBODY };
#define TL_START 40      /* frames before the first guest */
#define TL_UNIT 8        /* frames per point counted */
#define TL_GUEST 24      /* frames between guests */
#define TL_FAST 10       /* holding A: this many steps a frame */
static struct {
    int phase, slot, unit, wait, age;
    int pop, cash;               /* the counters in the top bar */
    int got_pop, got_cash, pen;  /* tonight's totals so far */
    int now, nowc;               /* fame and cash counted so far for the guest in turn */
    int why, frames, done_t, tick;
    int16_t vpop[PH_MAX_HOUSE], vcash[PH_MAX_HOUSE];
    uint8_t unpaid[PH_MAX_HOUSE];
} T;

static void tally_finish(void) {
    PhParty *pa = &G.party;
    PhPlayer *p = ph_me(&G);
    T.phase = TL_DONE;
    T.pop = p->pop;
    T.cash = p->cash;
    T.got_pop = pa->end_pop;
    T.got_cash = pa->end_cash;
    T.pen = pa->penalty;
    T.slot = -1;
    T.done_t = 0;
}

static void tally_setup(int pop0, int cash0, bool finished) {
    PhParty *pa = &G.party;
    PhPlayer *p = ph_me(&G);
    memset(&T, 0, sizeof T);
    for (int i = 0; i < pa->n; i++) {
        T.vpop[i] = (int16_t)ph_value_pop(&G, pa->house[i]);
        T.vcash[i] = (int16_t)ph_value_cash(&G, pa->house[i]);
    }
    ph_pay_order(&G, p->cash - pa->end_cash, T.unpaid);
    T.pop = pop0;
    T.cash = cash0;
    T.wait = TL_START;
    T.slot = -1;
    if (finished) { tally_finish(); T.done_t = 100; }
}

/* the next guest with something to count in this pass, or n */
static int tally_next(int from) {
    int n = G.party.n;
    for (int i = from; i < n; i++)
        if (T.phase == TL_EARN || T.vcash[i] < 0) return i;
    return n;
}

static void tally_begin_guest(int slot) {
    T.slot = slot;
    T.unit = 0;
    T.now = 0;
    T.nowc = 0;
    T.age = 0;
}

/* one step of the count */
static void tally_step(void) {
    if (T.phase == TL_DONE) return;
    if (T.wait > 0) { T.wait--; return; }
    int n = G.party.n;
    if (T.phase == TL_WAIT) {
        T.phase = TL_EARN;
        tally_begin_guest(tally_next(0));
    }
    if (T.phase == TL_EARN && T.slot >= n) {
        /* then the guests who charge are paid; fame stops at its cap */
        T.phase = TL_PAY;
        T.pop = ph_add_pop(T.pop - T.got_pop, T.got_pop);
        tally_begin_guest(tally_next(0));
    }
    if (T.phase == TL_PAY && T.slot >= n) {
        tally_finish();
        sfx_play_name(G.star_party ? "ph_star" : "ph_cash");
        return;
    }
    int s = T.slot, v = T.vpop[s], c = T.vcash[s];
    if (T.phase == TL_EARN) {
        int av = v < 0 ? -v : v, ac = c > 0 ? c : 0;
        if (T.unit < av) {
            int d = v < 0 ? -1 : 1;
            T.got_pop += d; T.pop += d; T.now += d;
            T.unit++; T.wait = TL_UNIT; T.tick = 1;
            return;
        }
        if (T.unit < av + ac) {
            /* cash stops at its cap: the guest still pays it, it just isn't kept */
            if (T.cash < PH_CASH_CAP) { T.got_cash++; T.cash++; }
            T.nowc++;
            T.unit++; T.wait = TL_UNIT; T.tick = 2;
            return;
        }
        T.wait = av + ac ? TL_GUEST : TL_GUEST / 2;
        tally_begin_guest(tally_next(s + 1));
        return;
    }
    /* TL_PAY */
    if (T.unpaid[s]) {
        if (T.unit == 0) {
            T.pen += 7;
            T.pop = T.pop - 7 < 0 ? 0 : T.pop - 7;
            T.unit = 1; T.wait = TL_GUEST * 2; T.tick = 3;
            return;
        }
    } else if (T.unit < -c) {
        T.got_cash--; T.cash--; T.nowc--;
        T.unit++; T.wait = TL_UNIT; T.tick = 2;
        return;
    }
    T.wait = TL_GUEST;
    tally_begin_guest(tally_next(s + 1));
}

static void tally_update(void) {
    if (T.phase == TL_DONE) { T.done_t++; return; }
    T.frames++;
    int steps = btn(BTN_A) ? TL_FAST : 1;
    T.tick = 0;
    for (int k = 0; k < steps && T.phase != TL_DONE; k++) tally_step();
    T.age += steps;
    /* one tick a frame at most (every other frame when hurried) */
    if (T.tick && (steps == 1 || (frame_t & 1)))
        sfx_play_name(T.tick == 1 ? "ph_pop" : T.tick == 2 ? "ph_coin" : "ph_no");
}

/* the fame shown while the tally runs: the guests' fame goes in all
 * together, so it only meets the cap (or 0) as a whole */
static int tally_pop_shown(void) {
    if (T.phase == TL_EARN || T.phase == TL_WAIT) return ph_add_pop(T.pop - T.got_pop, T.got_pop);
    return imax(0, T.pop);
}

/* a party that ended well: the rules pay up, the tally shows it */
static void start_tally(int why) {
    PhPlayer *p = ph_me(&G);
    int pop0 = p->pop, cash0 = p->cash;
    ph_end_party(&G);
    tally_setup(pop0, cash0, false);
    T.why = why;
    state = S_RESULT;
    state_t = 0;
    sfx_play_name("ph_cash");
    note_endless_best();
}

static void begin_party(void) {
    state = G.players == 2 ? S_TURN : S_PARTY;
    state_t = 0;
    cur = CUR_DOOR;
    info_card = -1;
    fire_by = -1;
    memset(arrive, 0, sizeof arrive);
    game_set_pausable(true);
    if (state == S_PARTY) music_play(PH_MUS_PARTY);
    else music_restart(PH_MUS_NIGHT);
    if (G.players == 1) { sv.run_phase = 0; save_now(); }
}

/* code: a typed-in Random code, or -1 to deal a new one */
static void new_run(int scen, int players, int code) {
    uint64_t seed = (uint64_t)rng_next(&g_rng) << 32 | rng_next(&g_rng);
    bool typed = code >= 0;
    if (players == 1) {
        /* walking out on a dealt Random run in progress ends the streak */
        if (sv.has_run && sv.run.scen == PH_RANDOM && !sv.run.done && !code_typed()) sv.streak = 0;
        sv.has_run = 1;
    }
    run_code = -1;
    if (scen == PH_RANDOM) {
        /* every Random list comes from a six-digit code, so it can be played again */
        run_code = typed ? code % 1000000 : (int)(rng_next(&g_rng) % 1000000u);
        seed = code_seed(run_code);
        if (players == 1) code_put(run_code, typed);
    }
    ph_new(&G, scen, players, seed);
    clear_bans();
    endless_prev_best = sv.night_best[G.base % PH_BASES];
    state = S_INTRO;
    state_t = 0;
    music_play(PH_MUS_TITLE);
    if (players == 1) save_now();
}

/* OPEN ALL NIGHT on a chosen list (0..4, PH_RANDOM, or PH_ENDLESS for the big mix) */
static void new_endless_run(int base) {
    uint64_t seed = (uint64_t)rng_next(&g_rng) << 32 | rng_next(&g_rng);
    if (sv.has_run && sv.run.scen == PH_RANDOM && !sv.run.done && !code_typed()) sv.streak = 0;
    sv.has_run = 1;
    run_code = -1;
    ph_new_endless(&G, base, seed);
    clear_bans();
    endless_prev_best = sv.night_best[G.base % PH_BASES];
    state = S_INTRO;
    state_t = 0;
    music_play(PH_MUS_TITLE);
    save_now();
}

/* the custom list (the owner's): the shop sells the chosen guests, by the
 * usual rules or (the editor's toggle) as OPEN ALL NIGHT */
static int custom_count(bool stars) {
    int n = 0;
    for (int t = 0; t < G_COUNT; t++)
        if (custom_in(t) && !ph_custom_locked(t) && ((PH_GUESTS[t].traits & T_STAR) != 0) == stars) n++;
    return n;
}

static void new_custom_run(void) {
    uint64_t seed = (uint64_t)rng_next(&g_rng) << 32 | rng_next(&g_rng);
    uint8_t types[G_COUNT];
    int n = 0;
    for (int t = 0; t < G_COUNT; t++)
        if (custom_in(t) && !ph_custom_locked(t)) types[n++] = (uint8_t)t;
    bool endless = sv.cendless != 0;
    int players = endless ? 1 : mode_players;
    if (players == 1) {
        if (sv.has_run && sv.run.scen == PH_RANDOM && !sv.run.done && !code_typed()) sv.streak = 0;
        sv.has_run = 1;
    }
    run_code = -1;
    ph_new_custom(&G, types, n, endless, players, seed);
    clear_bans();
    endless_prev_best = sv.night_best[G.base % PH_BASES];
    state = S_INTRO;
    state_t = 0;
    music_play(PH_MUS_TITLE);
    if (players == 1) save_now();
}

static void on_scenario_over(void) {
    if (G.players == 1) {
        bool won = G.winner == 1;
        if (won && G.base == PH_CUSTOM) {
            /* the custom list keeps its own record; it never opens lists,
             * counts for the streak or wins goals */
            int nights = ph_me(&G)->night;
            if (sv.cwins < 60000) sv.cwins++;
            if (!sv.cfastest || nights < sv.cfastest) sv.cfastest = (uint8_t)nights;
        } else if (won) {
            if (G.scen < PH_SCENARIOS) sv.won |= (uint8_t)(1 << G.scen);
            else {
                sv.won |= 32;
                if (sv.random_wins < 60000) sv.random_wins++;
                if (!code_typed() && sv.streak < 255) sv.streak++;
                if (sv.streak > sv.best_streak) sv.best_streak = sv.streak;
            }
        } else if (G.scen == PH_RANDOM && !code_typed()) {
            sv.streak = 0;
        }
        sv.has_run = 0;
        note_endless_best();
        check_goals();
        save_now();
    }
    state = G.winner ? S_WIN : S_LOSE;
    state_t = 0;
    /* OPEN ALL NIGHT ends on the fanfare when the run beat the best */
    bool fanfare = G.winner || (G.scen == PH_ENDLESS && G.score > endless_prev_best);
    music_restart(fanfare ? PH_MUS_WIN : PH_MUS_LOSE);
}

/* the party is over and paid (or shut down and a guest banned) */
static void after_party(void) {
    if (sv.parties < 60000) sv.parties++;
    /* the red BANNED tag: the guest just banned (or nobody), until the
     * party they miss is over */
    ban_tag[G.turn & 1] = ph_me(&G)->banned;
    if (G.players == 1) sv.ban_tag = ban_tag[0];
    /* OPEN ALL NIGHT: the third shutdown closes the house */
    if (G.party.over == PO_POLICE || G.party.over == PO_FIRE) ph_endless_strike(&G);
    if (G.done) { on_scenario_over(); return; } /* four stars: won */
    PhPlayer *p = ph_me(&G);
    if (p->night >= ph_night_limit(&G)) {
        ph_next_turn(&G);
        if (G.done) { on_scenario_over(); return; }
        begin_party();
        return;
    }
    state = S_SHOP;
    state_t = 0;
    shop_home();
    if (ph_endless_refresh(&G)) { say("NEW FACES IN TOWN!"); shop_fresh++; }
    music_play(PH_MUS_SHOP);
    if (G.players == 1) { sv.run_phase = 1; save_now(); }
}

static void leave_shop(void) {
    ph_next_turn(&G);
    if (G.done) { on_scenario_over(); return; }
    begin_party();
}

/* ------------------------------------------------------------------ */
/* the party screen: input                                              */

static int slot_x(int i) { return GRID_X + (i % PH_ROW) * SLOT_W; }
static int slot_y(int i) { return GRID_Y + (i / PH_ROW) * SLOT_H; }

/* The fire marshal: whose friend didn't fit. The knock's guests came in
 * depth first (each brings theirs before the next); the one still owed a
 * guest when the house overflowed is the one who brought one too many. */
static int fire_cause(void) {
    const PhParty *pa = &G.party;
    const PhPlayer *p = ph_me_c(&G);
    int stack[PH_MAX_HOUSE], owed[PH_MAX_HOUSE], sp = 0;
    for (int i = 0; i < pa->nlast; i++) {
        while (sp > 0 && owed[sp - 1] == 0) sp--;
        if (sp > 0) owed[sp - 1]--;
        uint16_t tr = PH_GUESTS[p->card[pa->last[i]].type].traits;
        stack[sp] = pa->last[i];
        owed[sp] = tr & T_BRING2 ? 2 : tr & T_BRING1 ? 1 : 0;
        if (sp < PH_MAX_HOUSE - 1) sp++;
    }
    while (sp > 0 && owed[sp - 1] == 0) sp--;
    if (sp == 0) return -1;
    for (int i = 0; i < pa->n; i++)
        if (pa->house[i] == stack[sp - 1]) return i;
    return -1;
}

static void note_arrivals(void) {
    fire_by = G.party.over == PO_FIRE ? fire_cause() : -1;
    for (int i = 0; i < G.party.nlast; i++) arrive[G.party.last[i]] = frame_t + i * 6;
    bool star = false, wild = false;
    for (int i = 0; i < G.party.nlast; i++) {
        int c = G.party.last[i];
        star |= (PH_GUESTS[ph_me(&G)->card[c].type].traits & T_STAR) != 0;
        wild |= ph_is_wild(&G, c) != 0;
    }
    sfx_play_name(star ? "ph_star" : wild ? "ph_trouble" : "ph_enter");
    G.party.nlast = 0;
}

static void after_change(int was_trouble) {
    PhParty *pa = &G.party;
    if (pa->over == PO_POLICE || pa->over == PO_FIRE) {
        state = S_BUST;
        state_t = 0;
        music_restart(PH_MUS_BUST);
        /* a shutdown owes a ban: quitting now must not skip it */
        if (G.players == 1) { sv.run_phase = 2; save_now(); }
        return;
    }
    if (ph_trouble(&G) >= 2 && was_trouble < 2) sfx_play_name("ph_warn");
    if (cur < CUR_DOOR && cur >= pa->n) cur = pa->n ? pa->n - 1 : CUR_DOOR;
    if (ph_should_end(&G)) start_tally(pa->n >= ph_me(&G)->cap ? WHY_FULL : WHY_NOBODY);
}

static void open_door(void) {
    int t = ph_trouble(&G);
    PhPlayer *p = ph_me(&G);
    if (G.party.n >= p->cap) { sfx_play_name("ph_no"); say_red(G.party.peek >= 0 ? "NO ROOM FOR THEM" : "THE HOUSE IS FULL"); return; }
    sfx_play_name("ph_knock");
    if (!ph_open_door(&G)) { sfx_play_name("ph_no"); say_red("NOBODY LEFT TO INVITE"); return; }
    note_arrivals();
    after_change(t);
}

static void use_action(int slot) {
    PhPlayer *p = ph_me(&G);
    int card = G.party.house[slot];
    int act = PH_GUESTS[p->card[card].type].action;
    if (!ph_can_act(&G, slot)) { sfx_play_name("ph_no"); return; }
    sel = slot;
    int t = ph_trouble(&G);
    switch (act) {
    case A_BOOT: case A_PHOTO: case A_STYLE: case A_MAGIC: case A_CUPID: case A_ENCORE:
        state = S_TARGET;
        state_t = 0;
        for (tcur = 0; tcur < G.party.n && !ph_target_ok(&G, sel, tcur); tcur++) {}
        sfx_play_name("ui_ok");
        return;
    case A_FETCH:
        nfetch = 0;
        for (int ty = 0; ty < G_COUNT; ty++)
            if (ph_fetch_ok(&G, ty)) fetch_list[nfetch++] = (uint8_t)ty;
        fcur = 0;
        state = S_FETCH;
        state_t = 0;
        sfx_play_name("ui_ok");
        return;
    case A_PEEK:
        /* the guest at the door waits there, shown by the door with their
         * badges, and the party goes on (the owner's): open the door to let
         * exactly them in, TURN AWAY, use other guests or end the party */
        if (ph_act(&G, slot, 0)) { cur = CUR_DOOR; sfx_play_name("ph_knock"); }
        return;
    default:
        if (ph_act(&G, slot, 0)) {
            sfx_play_name(act == A_RESHUFFLE ? "ph_shuffle" : "ph_act");
            if (act == A_GREET) note_arrivals();
            if (act == A_RESHUFFLE) cur = CUR_DOOR;
            after_change(t);
        }
        return;
    }
}

/* the buttons by the door, top to bottom: TURN AWAY only while a peeked
 * guest waits */
static int panel_buttons(int *order) {
    int k = 0;
    order[k++] = CUR_DOOR;
    if (G.party.peek >= 0) order[k++] = CUR_AWAY;
    order[k++] = CUR_END;
    order[k++] = CUR_BOOK;
    return k;
}

static void move_grid_cursor(int *c, int n, bool allow_panel) {
    if (*c >= CUR_DOOR) {
        int order[4], k = panel_buttons(order), at = 0;
        while (at < k - 1 && order[at] != *c) at++;
        if (btn_repeat(BTN_UP)) { *c = order[(at + k - 1) % k]; sfx_play_name("ph_move"); }
        if (btn_repeat(BTN_DOWN)) { *c = order[(at + 1) % k]; sfx_play_name("ph_move"); }
        if (btn_repeat(BTN_LEFT) && n > 0) { *c = imin(n - 1, PH_ROW - 1); sfx_play_name("ph_move"); }
        return;
    }
    int c0 = *c;
    if (btn_repeat(BTN_LEFT) && *c % PH_ROW > 0) (*c)--;
    if (btn_repeat(BTN_RIGHT)) {
        if (*c % PH_ROW < PH_ROW - 1 && *c + 1 < n) (*c)++;
        else if (allow_panel) *c = CUR_DOOR;
    }
    if (btn_repeat(BTN_UP) && *c < CUR_DOOR && *c >= PH_ROW) *c -= PH_ROW;
    if (btn_repeat(BTN_DOWN) && *c < CUR_DOOR && *c + PH_ROW < n) *c += PH_ROW;
    if (*c != c0) sfx_play_name("ph_move");
}

static void update_party(void) {
    PhParty *pa = &G.party;
    /* nothing more can happen (after a turned-away guest, say): it ends */
    if (ph_should_end(&G)) { start_tally(pa->n >= ph_me(&G)->cap ? WHY_FULL : WHY_NOBODY); return; }
    if (cur == CUR_AWAY && pa->peek < 0) cur = CUR_DOOR;
    move_grid_cursor(&cur, pa->n, true);
    info_card = cur < CUR_DOOR && cur < pa->n ? pa->house[cur] : ((cur == CUR_DOOR || cur == CUR_AWAY) && pa->peek >= 0 ? pa->peek : -1);
    /* B asks to end the party (the owner's); the answer starts on NO */
    if (btnp(BTN_B)) { state = S_CONFIRM; menu_sel = 1; sfx_play_name("ui_ok"); return; }
    if (!btnp(BTN_A)) return;
    if (cur == CUR_DOOR) open_door();
    else if (cur == CUR_END) { state = S_CONFIRM; menu_sel = 1; sfx_play_name("ui_ok"); }
    else if (cur == CUR_BOOK) { back_state = S_PARTY; state = S_LIST; state_t = 0; fcur = 0; sfx_play_name("ui_ok"); }
    else if (cur == CUR_AWAY) {
        /* the peeked guest is turned away: out till the next party */
        if (ph_peek_decide(&G, false)) sfx_play_name("ph_boot");
        cur = CUR_DOOR;
    }
    else use_action(cur);
}

static void update_target(void) {
    int n = G.party.n;
    int c0 = tcur;
    move_grid_cursor(&tcur, n, false);
    if (tcur != c0) info_card = G.party.house[tcur];
    if (btnp(BTN_B)) { state = S_PARTY; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A)) {
        int t = ph_trouble(&G);
        int act = PH_GUESTS[ph_me(&G)->card[G.party.house[sel]].type].action;
        if (!ph_target_ok(&G, sel, tcur)) { sfx_play_name("ph_no"); return; }
        if (ph_act(&G, sel, tcur)) {
            state = S_PARTY;
            sfx_play_name(act == A_BOOT || act == A_CUPID ? "ph_boot" : act == A_PHOTO || act == A_ENCORE ? "ph_cash" : "ph_act");
            if (act == A_MAGIC) note_arrivals();
            after_change(t);
        }
    }
}

static void update_fetch(void) {
    if (btn_repeat(BTN_UP) && fcur > 0) { fcur--; sfx_play_name("ph_move"); }
    if (btn_repeat(BTN_DOWN) && fcur < nfetch - 1) { fcur++; sfx_play_name("ph_move"); }
    if (btnp(BTN_B)) { state = S_PARTY; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A) && nfetch > 0) {
        int t = ph_trouble(&G);
        if (ph_act(&G, sel, fetch_list[fcur])) {
            state = S_PARTY;
            note_arrivals();
            after_change(t);
        }
    }
}

static void update_confirm(void) {
    if (btn_repeat(BTN_LEFT) || btn_repeat(BTN_RIGHT) || btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { menu_sel ^= 1; sfx_play_name("ph_move"); }
    if (btnp(BTN_B)) { state = S_PARTY; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A)) {
        if (menu_sel == 0) {
            start_tally(WHY_HAND);
        } else {
            state = S_PARTY;
            sfx_play_name("ui_back");
        }
    }
}

static void update_ban(void) {
    int n = G.party.n;
    move_grid_cursor(&tcur, n, false);
    if (n == 0) { after_party(); return; }
    if (btnp(BTN_A)) {
        ph_ban(&G, G.party.house[tcur]);
        sfx_play_name("ph_boot");
        after_party();
    }
}

/* ------------------------------------------------------------------ */
/* the shop                                                             */

/* The shop is a grid of guest cards, five to a row, over a row of three
 * buttons (SPACE, GUEST BOOK, NEXT PARTY). The original's planner lists its
 * guests differently; the grid is the owner's choice. shop_cur runs through
 * the pool, then SPACE (npool), NEXT PARTY (npool + 1), GUEST BOOK (+ 2). */
#define SHOP_COLS 5
#define SHOP_SHOWN 3     /* rows of cards on screen; OPEN ALL NIGHT's bigger shop scrolls */
enum { SH_SPACE, SH_NEXT, SH_BOOK };
static int shop_top;     /* the first row of cards shown */

/* The buttons run SPACE, GUEST BOOK, NEXT PARTY from left to right; each
 * sits under a column of cards (0, 2 and 4). Every move wraps (the owner's):
 * LEFT and RIGHT go round a row, UP from the top row goes to NEXT PARTY,
 * DOWN from the last row reaches the button below, and UP and DOWN from the
 * buttons go to the last and the first row. B goes to NEXT PARTY, and a
 * second B starts it. */
static const int SH_ORDER[3] = {SH_SPACE, SH_BOOK, SH_NEXT};
static const int SH_COLUMN[3] = {0, 2, 4};

static void shop_home(void) { shop_cur = 0; shop_top = 0; }

/* the guest type under the cursor, or -1 on a button */
static int shop_type(void) { return shop_cur < G.npool ? G.pool[shop_cur] : -1; }

static int shop_rows(void) { return (G.npool + SHOP_COLS - 1) / SHOP_COLS; }
static int shop_card(int row, int col) {
    int len = imin(SHOP_COLS, G.npool - row * SHOP_COLS);
    return row * SHOP_COLS + imin(col, len - 1);
}

static void update_shop(void) {
    int c0 = shop_cur;
    if (btnp(BTN_B)) {
        if (shop_cur == G.npool + SH_NEXT) { sfx_play_name("ui_ok"); leave_shop(); return; }
        shop_cur = G.npool + SH_NEXT;
    } else if (shop_cur < G.npool) {
        int row = shop_cur / SHOP_COLS, col = shop_cur % SHOP_COLS;
        int len = imin(SHOP_COLS, G.npool - row * SHOP_COLS);
        if (btn_repeat(BTN_LEFT)) shop_cur = row * SHOP_COLS + (col + len - 1) % len;
        else if (btn_repeat(BTN_RIGHT)) shop_cur = row * SHOP_COLS + (col + 1) % len;
        else if (btn_repeat(BTN_UP)) shop_cur = row > 0 ? shop_card(row - 1, col) : G.npool + SH_NEXT;
        else if (btn_repeat(BTN_DOWN))
            shop_cur = row + 1 < shop_rows() ? shop_card(row + 1, col) : G.npool + SH_ORDER[col <= 1 ? 0 : col == 2 ? 1 : 2];
    } else {
        int k = 0;
        while (k < 2 && G.npool + SH_ORDER[k] != shop_cur) k++;
        if (btn_repeat(BTN_LEFT)) shop_cur = G.npool + SH_ORDER[(k + 2) % 3];
        else if (btn_repeat(BTN_RIGHT)) shop_cur = G.npool + SH_ORDER[(k + 1) % 3];
        else if (btn_repeat(BTN_UP) && G.npool > 0) shop_cur = shop_card(shop_rows() - 1, SH_COLUMN[k]);
        else if (btn_repeat(BTN_DOWN) && G.npool > 0) shop_cur = shop_card(0, SH_COLUMN[k]);
    }
    if (shop_cur < G.npool) {
        int row = shop_cur / SHOP_COLS;
        if (row < shop_top) shop_top = row;
        if (row >= shop_top + SHOP_SHOWN) shop_top = row - SHOP_SHOWN + 1;
    }
    if (shop_cur != c0) sfx_play_name("ph_move");
    if (!btnp(BTN_A)) return;
    if (shop_cur == G.npool + SH_BOOK) { back_state = S_SHOP; state = S_LIST; state_t = 0; fcur = 0; sfx_play_name("ui_ok"); return; }
    if (shop_cur < G.npool) {
        int ty = G.pool[shop_cur];
        if (ph_buy(&G, ty)) sfx_play_name(PH_GUESTS[ty].traits & T_STAR ? "ph_star" : "ph_buy");
        else { sfx_play_name("ph_no"); say_red(!(PH_GUESTS[ty].traits & T_STAR) && G.bought[ty] >= PH_STOCK ? "SOLD OUT" : "NOT ENOUGH FAME"); }
    } else if (shop_cur == G.npool + SH_SPACE) {
        if (ph_expand(&G)) sfx_play_name("ph_build");
        else { sfx_play_name("ph_no"); say_red(ph_me(&G)->cap >= PH_MAX_HOUSE ? "THE HOUSE CAN'T GROW" : "NOT ENOUGH CASH"); }
    } else {
        sfx_play_name("ui_ok");
        leave_shop();
    }
}

/* ------------------------------------------------------------------ */
/* menus                                                                */

static void update_title(void) {
    game_set_pausable(false);
    if (btnp(BTN_B) && state_t > 10) { game_exit_to_library(); return; }
    if (state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) {
        sfx_play_name("ui_ok");
        input_consume();
        state = S_MODE;
        state_t = 0;
        menu_sel = sv.has_run ? 0 : 1;
    }
}

static void update_mode(void) {
    int n = 3;
    if (btn_repeat(BTN_UP)) { menu_sel = (menu_sel + n - 1) % n; if (menu_sel == 0 && !sv.has_run) menu_sel = n - 1; sfx_play_name("ph_move"); }
    if (btn_repeat(BTN_DOWN)) { menu_sel = (menu_sel + 1) % n; if (menu_sel == 0 && !sv.has_run) menu_sel = 1; sfx_play_name("ph_move"); }
    if (btnp(BTN_B)) { state = S_TITLE; state_t = 0; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A)) {
        sfx_play_name("ui_ok");
        if (menu_sel == 0 && sv.has_run) {
            G = sv.run;
            run_code = code_get();
            ban_tag[0] = sv.ban_tag;
            ban_tag[1] = 0;
            endless_prev_best = sv.night_best[G.base % PH_BASES];
            game_set_pausable(true);
            if (sv.run_phase == 1) { state = S_SHOP; state_t = 0; shop_home(); music_play(PH_MUS_SHOP); }
            else if (G.party.over == PO_POLICE || G.party.over == PO_FIRE) {
                /* shut down before the ban was chosen: the ban is still owed */
                state = S_BAN; state_t = 0; tcur = 0; music_play(PH_MUS_PARTY);
            } else if (G.party.over == PO_ENDED) {
                /* back on the tally, already counted */
                state = S_RESULT; state_t = 40; music_play(PH_MUS_PARTY);
                tally_setup(ph_me(&G)->pop, ph_me(&G)->cash, true);
            }
            else { state = S_PARTY; state_t = 0; cur = CUR_DOOR; music_play(PH_MUS_PARTY); }
            return;
        }
        mode_players = menu_sel == 2 ? 2 : 1;
        scen_endless = false;
        state = S_SCEN;
        state_t = 0;
        scen_sel = 0;
        for (int i = 0; i <= PH_RANDOM; i++)
            if (!beaten(i)) { scen_sel = i; break; }
    }
}

/* The lists sit in a grid of tiles (the owner's): three to a row (1 2 3,
 * 4 5 RANDOM), with OPEN ALL NIGHT (two columns wide) and CUSTOM (the third
 * column) underneath. LEFT and RIGHT go round a row, UP and DOWN round the
 * rows (keeping the column; the wide tile keeps the one it was entered by). */
#define SCEN_COLS 3
static int scen_col;     /* the column UP and DOWN keep through the wide tile */

static int scen_row(int i) { return i >= PH_ENDLESS ? 2 : i / SCEN_COLS; }
static int scen_at(int r, int col) { return r == 2 ? (col == 2 ? PH_CUSTOM : PH_ENDLESS) : r * SCEN_COLS + col; }

static void scen_move(void) {
    int r = scen_row(scen_sel), s0 = scen_sel;
    if (r < 2) scen_col = scen_sel % SCEN_COLS;
    else if (scen_sel == PH_CUSTOM) scen_col = 2;
    else if (scen_col == 2) scen_col = 0;
    if (btn_repeat(BTN_LEFT)) {
        if (r < 2) scen_sel = r * SCEN_COLS + (scen_col + SCEN_COLS - 1) % SCEN_COLS;
        else if (scen_sel == PH_CUSTOM) { scen_sel = PH_ENDLESS; scen_col = 1; }
        else { scen_sel = PH_CUSTOM; scen_col = 2; }
    } else if (btn_repeat(BTN_RIGHT)) {
        if (r < 2) scen_sel = r * SCEN_COLS + (scen_col + 1) % SCEN_COLS;
        else if (scen_sel == PH_CUSTOM) { scen_sel = PH_ENDLESS; scen_col = 0; }
        else { scen_sel = PH_CUSTOM; scen_col = 2; }
    }
    else if (btn_repeat(BTN_UP)) { r = (r + 2) % 3; scen_sel = scen_at(r, scen_col); }
    else if (btn_repeat(BTN_DOWN)) { r = (r + 1) % 3; scen_sel = scen_at(r, scen_col); }
    if (scen_sel != s0) sfx_play_name("ph_move");
}

/* ------------------------------------------------------------------ */
/* the custom list's editor (the owner's): every guest in a grid of eight   */
/* by six, in roster order; A puts one in or takes it out. The buttons on   */
/* the right play, switch OPEN ALL NIGHT on or off, clear and randomise.    */

#define ED_COLS 8
#define ED_ROWS 6
#define ED_BTN G_COUNT      /* ed_cur past the guests: the buttons */
enum { EB_PLAY, EB_NIGHT, EB_CLEAR, EB_RANDOM, EB_COUNT };
enum { EW_NONE, EW_STAR, EW_GUESTS, EW_ONE_PLAYER };
static int ed_cur, ed_row, ed_btn, ed_warn;
_Static_assert(G_COUNT == ED_COLS * ED_ROWS, "every guest has a cell");
_Static_assert(PH_CUSTOM_MIN == 6, "the editor's texts say six guests");

static void open_custom(bool all_night) {
    if (all_night && !sv.cendless) { sv.cendless = 1; save_now(); }
    state = S_CUSTOM;
    state_t = 0;
    ed_cur = G_FIRST_BUYABLE;
    ed_row = 0;
    ed_btn = EB_PLAY;
    ed_warn = EW_NONE;
}

/* a random custom list that can always be played: one to three stars and
 * six to sixteen other guests from the whole roster */
static void custom_randomise(void) {
    uint8_t stars[G_COUNT], others[G_COUNT];
    int ns = 0, no = 0;
    for (int t = 0; t < G_COUNT; t++) {
        if (ph_custom_locked(t)) continue;
        custom_put(t, false);
        if (PH_GUESTS[t].traits & T_STAR) stars[ns++] = (uint8_t)t;
        else others[no++] = (uint8_t)t;
    }
    int want_s = 1 + (int)(rng_next(&g_rng) % 3u);
    int want_o = PH_CUSTOM_MIN + (int)(rng_next(&g_rng) % 11u);
    for (int k = 0; k < want_s && k < ns; k++) {
        int i = k + (int)(rng_next(&g_rng) % (uint32_t)(ns - k));
        uint8_t tmp = stars[k]; stars[k] = stars[i]; stars[i] = tmp;
        custom_put(stars[k], true);
    }
    for (int k = 0; k < want_o && k < no; k++) {
        int i = k + (int)(rng_next(&g_rng) % (uint32_t)(no - k));
        uint8_t tmp = others[k]; others[k] = others[i]; others[i] = tmp;
        custom_put(others[k], true);
    }
}

static void custom_play(void) {
    if (custom_count(true) < PH_CUSTOM_MIN_STARS) ed_warn = EW_STAR;
    else if (custom_count(false) < PH_CUSTOM_MIN) ed_warn = EW_GUESTS;
    else if (sv.cendless && mode_players == 2) ed_warn = EW_ONE_PLAYER;
    if (ed_warn) { sfx_play_name("ph_no"); return; }
    sfx_play_name("ui_ok");
    new_custom_run();
}

static void update_custom(void) {
    /* a warning stays up until A or B */
    if (ed_warn) {
        if (btnp(BTN_A) || btnp(BTN_B)) { ed_warn = EW_NONE; sfx_play_name("ui_back"); }
        return;
    }
    int c0 = ed_cur;
    if (ed_cur < ED_BTN) {
        int r = ed_cur / ED_COLS, c = ed_cur % ED_COLS;
        if (btn_repeat(BTN_LEFT)) { if (c == 0) { ed_row = r; ed_cur = ED_BTN + ed_btn; } else ed_cur--; }
        else if (btn_repeat(BTN_RIGHT)) { if (c == ED_COLS - 1) { ed_row = r; ed_cur = ED_BTN + ed_btn; } else ed_cur++; }
        else if (btn_repeat(BTN_UP)) ed_cur = ((r + ED_ROWS - 1) % ED_ROWS) * ED_COLS + c;
        else if (btn_repeat(BTN_DOWN)) ed_cur = ((r + 1) % ED_ROWS) * ED_COLS + c;
    } else {
        int b = ed_cur - ED_BTN;
        if (btn_repeat(BTN_UP)) ed_cur = ED_BTN + (b + EB_COUNT - 1) % EB_COUNT;
        else if (btn_repeat(BTN_DOWN)) ed_cur = ED_BTN + (b + 1) % EB_COUNT;
        else if (btn_repeat(BTN_LEFT)) ed_cur = ed_row * ED_COLS + ED_COLS - 1;
        else if (btn_repeat(BTN_RIGHT)) ed_cur = ed_row * ED_COLS;
        if (ed_cur >= ED_BTN) ed_btn = ed_cur - ED_BTN;
    }
    if (ed_cur != c0) sfx_play_name("ph_move");
    if (btnp(BTN_B)) {
        /* the list is saved as it is edited; B goes back to its tile */
        sfx_play_name("ui_back");
        state = S_SCEN;
        state_t = 0;
        scen_sel = PH_CUSTOM;
        scen_col = 2;
        return;
    }
    if (!btnp(BTN_A)) return;
    if (ed_cur < ED_BTN) {
        if (ph_custom_locked(ed_cur)) { sfx_play_name("ph_no"); say("ALWAYS INVITED"); return; }
        bool in = !custom_in(ed_cur);
        custom_put(ed_cur, in);
        sfx_play_name(in ? (PH_GUESTS[ed_cur].traits & T_STAR ? "ph_star" : "ph_buy") : "ph_boot");
        save_now();
        return;
    }
    switch (ed_cur - ED_BTN) {
    case EB_PLAY: custom_play(); return;
    case EB_NIGHT: sv.cendless = !sv.cendless; sfx_play_name("ui_ok"); break;
    case EB_CLEAR:
        for (int t = 0; t < G_COUNT; t++)
            if (!ph_custom_locked(t)) custom_put(t, false);
        sfx_play_name("ph_boot");
        break;
    case EB_RANDOM: custom_randomise(); sfx_play_name("ph_shuffle"); break;
    }
    save_now();
}

static void update_scen(void) {
    scen_move();
    if (btnp(BTN_B)) {
        sfx_play_name("ui_back");
        /* back out of OPEN ALL NIGHT's grid onto its tile */
        if (scen_endless) { scen_endless = false; scen_sel = PH_ENDLESS; scen_col = 0; return; }
        state = S_MODE; state_t = 0;
        return;
    }
    if (!btnp(BTN_A)) return;
    if (scen_sel == PH_CUSTOM) {
        /* the custom list's editor; from OPEN ALL NIGHT's grid it starts on ALL NIGHT */
        sfx_play_name("ui_ok");
        open_custom(scen_endless);
        return;
    }
    if (scen_endless) {
        /* OPEN ALL NIGHT on this list (the wide tile is the big mix) */
        sfx_play_name("ui_ok");
        new_endless_run(scen_sel);
        return;
    }
    if (scen_sel == PH_ENDLESS) {
        if (mode_players == 2) { sfx_play_name("ph_no"); say_red("OPEN ALL NIGHT IS FOR ONE PLAYER"); return; }
        /* the same grid again, now picking the list to play all night */
        sfx_play_name("ui_ok");
        scen_endless = true;
        return;
    }
    sfx_play_name("ui_ok");
    if (scen_sel == PH_RANDOM) { state = S_RPICK; state_t = 0; menu_sel = 0; return; }
    new_run(scen_sel, mode_players, -1);
}

/* the Random tile: deal a new list, or type in a code to play one again */
static void update_rpick(void) {
    if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { menu_sel ^= 1; sfx_play_name("ph_move"); }
    if (btnp(BTN_B)) { state = S_SCEN; state_t = 0; sfx_play_name("ui_back"); return; }
    if (!btnp(BTN_A)) return;
    sfx_play_name("ui_ok");
    if (menu_sel == 0) { new_run(PH_RANDOM, mode_players, -1); return; }
    int c = code_get();
    if (c < 0) c = 0;
    for (int i = 5; i >= 0; i--) { code_dig[i] = c % 10; c /= 10; }
    code_cur = 0;
    state = S_CODE;
    state_t = 0;
}

static void update_code(void) {
    if (btn_repeat(BTN_LEFT) && code_cur > 0) { code_cur--; sfx_play_name("ph_move"); }
    if (btn_repeat(BTN_RIGHT) && code_cur < 5) { code_cur++; sfx_play_name("ph_move"); }
    if (btn_repeat(BTN_UP)) { code_dig[code_cur] = (code_dig[code_cur] + 1) % 10; sfx_play_name("ph_move"); }
    if (btn_repeat(BTN_DOWN)) { code_dig[code_cur] = (code_dig[code_cur] + 9) % 10; sfx_play_name("ph_move"); }
    if (btnp(BTN_B)) { state = S_SCEN; state_t = 0; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A)) {
        int c = 0;
        for (int i = 0; i < 6; i++) c = c * 10 + code_dig[i];
        sfx_play_name("ui_ok");
        new_run(PH_RANDOM, mode_players, c);
    }
}

/* ------------------------------------------------------------------ */
/* update                                                               */

/* the icon guide, from the guest book or the START menu */
static void open_legend(void) {
    if (state == S_LEGEND) return;
    legend_back = state;
    state = S_LEGEND;
    sfx_play_name("ui_ok");
}
static const char *const PAUSE_ITEMS[1] = {"ICON GUIDE"};
static void pause_pick(int i) { if (i == 0) open_legend(); }

static void oh_update(void) {
    frame_t++;
    state_t++;
    if (msg_t > 0) msg_t--;
    switch (state) {
    case S_TITLE: update_title(); break;
    case S_MODE: update_mode(); break;
    case S_SCEN: update_scen(); break;
    case S_INTRO:
        if (state_t > 20 && btnp(BTN_A)) { sfx_play_name("ui_ok"); begin_party(); }
        if (btnp(BTN_B)) { state = G.base == PH_CUSTOM ? S_CUSTOM : S_SCEN; state_t = 0; }
        break;
    case S_TURN:
        if (state_t > 30 && btnp(BTN_A)) { state = S_PARTY; state_t = 0; music_play(PH_MUS_PARTY); }
        break;
    case S_PARTY: update_party(); break;
    case S_TARGET: update_target(); break;
    case S_FETCH: update_fetch(); break;
    case S_CONFIRM: update_confirm(); break;
    case S_BUST:
        if (state_t > 100 && btnp(BTN_A)) {
            /* OPEN ALL NIGHT's last strike: no ban, the house closes */
            if (G.scen == PH_ENDLESS && G.strikes + 1 >= PH_ENDLESS_STRIKES) { after_party(); break; }
            state = S_BAN; state_t = 0; tcur = 0; music_play(PH_MUS_PARTY);
        }
        break;
    case S_BAN: update_ban(); break;
    case S_RESULT:
        /* holding A hurries the count; a fresh press once it's done goes on */
        tally_update();
        if (T.phase == TL_DONE && T.done_t > 6 && btnp(BTN_A)) { sfx_play_name("ui_ok"); after_party(); }
        break;
    case S_SHOP: update_shop(); break;
    case S_WIN: case S_LOSE:
        if (state_t > 90 && btnp(BTN_A)) { state = S_SCEN; state_t = 0; music_play(PH_MUS_TITLE); game_set_pausable(true); }
        break;
    case S_CODE: update_code(); break;
    case S_RPICK: update_rpick(); break;
    case S_CUSTOM: update_custom(); break;
    case S_LIST:
        if (btn_repeat(BTN_UP) && fcur > 0) fcur--;
        if (btn_repeat(BTN_DOWN)) fcur++;
        /* LEFT or RIGHT turns to the icon guide */
        if (btnp(BTN_LEFT) || btnp(BTN_RIGHT)) { open_legend(); break; }
        if (btnp(BTN_B) || btnp(BTN_A)) { state = back_state; sfx_play_name("ui_back"); }
        break;
    case S_LEGEND:
        if (btnp(BTN_B) || btnp(BTN_A) || (legend_back == S_LIST && (btnp(BTN_LEFT) || btnp(BTN_RIGHT)))) {
            state = legend_back;
            sfx_play_name("ui_back");
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing helpers                                                      */

static void draw_num(int v, int x, int y, int col, const char *pre) {
    char b[16];
    snprintf(b, sizeof b, "%s%d", pre, v);
    tiny_draw(b, x, y, col);
}

/* the tiny font has no line breaks of its own */
static void tiny_lines(const char *s, int x, int y, int col) {
    char line[64];
    while (*s) {
        int n = 0;
        while (s[n] && s[n] != '\n' && n < 63) n++;
        memcpy(line, s, (size_t)n);
        line[n] = 0;
        tiny_draw(line, x, y, col);
        y += 6;
        s += n;
        if (*s == '\n') s++;
    }
}

/* the tiny font, word-wrapped to a width (a line break in the text is a
 * space here); returns the y under the last line */
static int tiny_wrap(const char *s, int x, int y, int w, int col) {
    char line[160], word[64];
    int ln = 0;
    line[0] = 0;
    while (*s) {
        while (*s == ' ' || *s == '\n') s++;
        int n = 0;
        while (s[n] && s[n] != ' ' && s[n] != '\n' && n < 63) n++;
        if (!n) break;
        memcpy(word, s, (size_t)n);
        word[n] = 0;
        s += n;
        char next[160];
        snprintf(next, sizeof next, "%s%s%s", line, ln ? " " : "", word);
        if (ln && tiny_width(next) > w) {
            tiny_draw(line, x, y, col);
            y += 6;
            snprintf(line, sizeof line, "%s", word);
        } else {
            snprintf(line, sizeof line, "%s", next);
        }
        ln = (int)strlen(line);
    }
    if (ln) { tiny_draw(line, x, y, col); y += 6; }
    return y;
}

static void icon_pop(int x, int y) {
    gfx_rect(x + 1, y, 5, 7, C_YELLOW);
    gfx_rect(x, y + 1, 7, 5, C_YELLOW);
    gfx_pset(x + 3, y + 3, C_AMBER);
    gfx_pset(x + 2, y + 2, C_CREAM);
}

static void icon_cash(int x, int y) {
    gfx_rect(x, y + 1, 7, 5, C_JADE);
    gfx_rectb(x, y + 1, 7, 5, C_FOREST);
    gfx_pset(x + 3, y + 3, C_LIME);
}

static void icon_star(int x, int y, int col) { text_draw(GLYPH_STAR, x, y, col); }

static void icon_trouble(int x, int y, int col) {
    gfx_line(x, y, x + 4, y + 4, col);
    gfx_line(x + 4, y, x, y + 4, col);
    gfx_line(x + 1, y, x + 5, y + 4, col);
    gfx_line(x + 5, y, x + 1, y + 4, col);
}

/* ------------------------------------------------------------------ */
/* the badges (the owner's): every guest's fame and cash, RUCKUS!, star   */
/* and ability family, drawn the same way wherever a guest is shown: in   */
/* the house, at the door, in the shop, the guest book, the editor and    */
/* the fetch list. The icon guide (S_LEGEND) explains each one.           */

/* what the last frame drew, for the tests (the "seen_" queries draw a
 * frame first) */
static struct {
    int notice;          /* the red notice: 1 OVER CAPACITY, 2 TOO MUCH TROUBLE */
    int culprits;        /* guests marked in red */
    int badges;          /* house guests drawn with their badges */
    int ban_slot;        /* the ban screen's BANNED tag, on slot + 1 */
    int ban_house;       /* tonight's banned guest, told on the bottom line */
    int ban_shop;        /* the shop's banned card + 1 */
    int ban_book;        /* the guest book's BANNED row: type + 1 */
    int pick_rows;       /* fetch list rows drawn with their cost and badges */
    int inf;             /* no-limit signs drawn */
    int door;            /* the guest at the door, drawn by the door: card + 1 */
    int legend;          /* the icon guide's rows */
    int crowns;          /* list tiles drawn gold (BEATEN) */
} seen;

typedef struct Badge {
    int pop, cash;       /* what they pay (0: not shown) */
    bool trouble, star;
    int abil;            /* PI_* */
    bool action;         /* the ability is an action, once a party */
    bool used, ready;    /* used tonight (dimmed); can be used right now */
} Badge;

/* what a guest is, from the table */
static Badge badge_type(int ty) {
    const PhGuest *t = &PH_GUESTS[ty];
    Badge b;
    memset(&b, 0, sizeof b);
    b.pop = t->pop;
    b.cash = t->cash;
    b.trouble = (t->traits & (T_TROUBLE | T_MOON)) != 0;
    b.star = (t->traits & T_STAR) != 0;
    b.abil = ph_ability_icon(ty);
    b.action = t->action != A_NONE;
    return b;
}

/* a guest in the house: tonight's pay, RUCKUS! right now, the action */
static Badge badge_slot(int slot) {
    int card = G.party.house[slot];
    Badge b = badge_type(ph_me(&G)->card[card].type);
    b.pop = ph_value_pop(&G, card);
    b.cash = ph_value_cash(&G, card);
    b.trouble = ph_is_wild(&G, card);
    b.used = b.action && G.party.used[card];
    b.ready = ph_can_act(&G, slot);
    return b;
}

/* a guest who isn't in: what they would be if they came in now (the one
 * at the door) */
static Badge badge_card(int card) {
    const PhCard *c = &ph_me(&G)->card[card];
    const PhGuest *t = &PH_GUESTS[c->type];
    Badge b = badge_type(c->type);
    b.pop = t->pop + c->bonus;
    if (t->traits & T_UPSTART) b.pop = imin(c->visits + 1 + c->bonus, 9);
    b.trouble = (t->traits & T_TROUBLE) || ((t->traits & T_MOON) && ((c->visits + 1) & 1)) ||
                (G.party.jinx && !(t->traits & T_JINX));
    return b;
}

/* the small numbers: a fame coin and the fame on the left, the cash as $
 * on the right of a width w; what's taken away is red. Only what isn't 0. */
static void draw_stat_pop(int pop, int x, int y) {
    if (!pop) return;
    char b[12];
    snprintf(b, sizeof b, "%d", pop);
    ph_icon(PI_FAME, x, y + 1, C_YELLOW);
    tiny_draw(b, x + 5, y, pop > 0 ? C_YELLOW : C_RED);
}
static const char *cash_text(int cash, char *b, int n) {
    cash = iclamp(cash, -999, 999);
    if (cash > 0) snprintf(b, (size_t)n, "$%d", cash);
    else snprintf(b, (size_t)n, "-$%d", -cash);
    return b;
}
static void draw_stat_cash(int cash, int x, int y) {
    if (!cash) return;
    char b[24];
    tiny_draw(cash_text(cash, b, sizeof b), x, y, cash > 0 ? C_LIME : C_RED);
}
static void draw_stats(int x, int y, int w, int pop, int cash) {
    char b[24];
    draw_stat_pop(pop, x, y);
    if (cash) draw_stat_cash(cash, x + w - tiny_width(cash_text(cash, b, sizeof b)), y);
}

/* RUCKUS!: a red tag with a white "!" (7 x 9 with its outline) */
static void badge_trouble(int x, int y) {
    gfx_rect(x, y - 1, 5, 9, C_INK);
    gfx_rect(x - 1, y, 7, 7, C_INK);
    gfx_rect(x + 1, y, 3, 7, C_RED);
    gfx_rect(x, y + 1, 5, 5, C_RED);
    gfx_vline(x + 2, y + 1, y + 3, C_WHITE);
    gfx_pset(x + 2, y + 5, C_WHITE);
}

/* the ability: its icon on a dark chip (7 x 7); dimmed once used tonight,
 * with a blinking spark while it can be used */
static void badge_abil(int x, int y, const Badge *b) {
    if (!b->abil) return;
    gfx_rect(x + 1, y, 5, 7, C_INK);
    gfx_rect(x, y + 1, 7, 5, C_INK);
    ph_icon(b->abil, x + 1, y + 1, b->used ? C_SLATE : -1);
    if (b->ready && (frame_t / 12) % 2) gfx_pset(x + 6, y, C_WHITE);
}

/* the corner badges of a guest whose face is 16 x 16 at (fx, fy): RUCKUS!
 * top left, the star top right, the ability under the star */
static void draw_corner_badges(const Badge *b, int fx, int fy) {
    if (b->trouble) badge_trouble(fx - 3, fy);
    if (b->star) ph_icon(PI_STAR, fx + 15, fy, C_YELLOW);
    badge_abil(fx + 14, fy + 9, b);
}

/* a small tag in a box: red for the BANNED and CAN'T notices */
static void tag(const char *s, int cx, int y, int fill, int col) {
    int w = tiny_width(s) + 4;
    gfx_rect(cx - w / 2 - 1, y - 1, w + 2, 9, C_INK);
    gfx_rect(cx - w / 2, y, w, 7, fill);
    tiny_draw(s, cx - w / 2 + 2, y + 1, col);
}
static void red_tag(const char *s, int cx, int y) { tag(s, cx, y, C_RED, C_WHITE); }

/* a red banner: small red text on a dark band with red rules */
static void red_banner(const char *s, int cx, int y, int w) {
    gfx_rect(cx - w / 2, y, w, 11, C_INK);
    gfx_hline(cx - w / 2, cx + w / 2 - 1, y, C_RED);
    gfx_hline(cx - w / 2, cx + w / 2 - 1, y + 10, C_RED);
    tiny_center(s, cx, y + 3, (frame_t / 16) % 4 ? C_RED : C_ORANGE);
}

/* a star can be bought again and again: the no-limit sign; the others
 * show the buys left of four */
static void draw_stock(int ty, int x, int y) {
    if (PH_GUESTS[ty].traits & T_STAR) { ph_icon(PI_INF, x, y + 1, -1); seen.inf++; return; }
    int left = PH_STOCK - G.bought[ty];
    char b[8];
    snprintf(b, sizeof b, "%d/%d", left, PH_STOCK);
    tiny_draw(b, x, y, left > 0 ? C_LIME : C_RED);
}

static const char *const SECRET_NAMES[8] = {"UNCLE BERT", "AUNT MAVIS", "OLD GUS", "MISS PENNY",
                                            "COUSIN TOM", "COUSIN MAE", "COUSIN NED", "COUSIN IVY"};

static const char *card_name(int card) {
    const PhCard *c = &ph_me(&G)->card[card];
    if (c->name) return SECRET_NAMES[(c->name - 1) % 8];
    return PH_GUESTS[c->type].name;
}

/* the red BANNED tag: the card banned from (or missing) the next party, -1 none */
static int banned_card(void) {
    int b = ban_tag[G.turn & 1] - 1;
    return b >= 0 && b < ph_me(&G)->ncards ? b : -1;
}
static int banned_type(void) { int b = banned_card(); return b < 0 ? -1 : ph_me(&G)->card[b].type; }

/* who brought the shutdown on: every RUCKUS! guest (the police), or the
 * guest whose friend didn't fit (the fire marshal) */
static bool culprit(int slot) {
    if (state != S_BUST && state != S_BAN) return false;
    if (G.party.over == PO_POLICE) return ph_is_wild(&G, G.party.house[slot]);
    return G.party.over == PO_FIRE && slot == fire_by;
}

/* a guest in a house slot */
static void draw_slot(int i, bool hi, bool target_ok, bool dim) {
    PhParty *pa = &G.party;
    int x = slot_x(i), y = slot_y(i);
    int card = pa->house[i];
    PhCard *c = &ph_me(&G)->card[card];
    const PhGuest *t = &PH_GUESTS[c->type];
    /* walking in from the door */
    int dt = frame_t - arrive[card];
    int ox = 0, oy = 0;
    if (dt >= 0 && dt < 14 && arrive[card]) {
        float f = 1.0f - dt / 14.0f;
        ox = (int)((PANEL_X + 30 - x) * f);
        oy = (int)((40 - y) * f);
    } else if (dt < 0) {
        return; /* not in yet */
    }
    Badge bd = badge_slot(i);
    /* the guest stands on a little plinth: amber for a star, wine for
     * RUCKUS!; its numbers sit on the dark top so they read on any */
    int rug = t->traits & T_STAR ? C_AMBER : bd.trouble ? C_WINE : C_DUSK;
    gfx_rect(x + 1, y + 17, SLOT_W - 2, 9, rug);
    gfx_rect(x + 2, y + 18, SLOT_W - 4, 7, C_INK);
    if (culprit(i)) {
        seen.culprits++;
        int rc = (frame_t / 6) % 2 ? C_RED : C_ORANGE;
        gfx_rectb(x, y - 1, SLOT_W, SLOT_H, hi ? ((frame_t / 8) % 2 ? C_WHITE : C_YELLOW) : rc);
        gfx_rectb(x + 1, y, SLOT_W - 2, SLOT_H - 2, rc);
    } else if (hi) gfx_rectb(x, y - 1, SLOT_W, SLOT_H, (frame_t / 8) % 2 ? C_WHITE : C_YELLOW);
    else if (target_ok) gfx_rectb(x, y - 1, SLOT_W, SLOT_H, C_SKY);
    int bob = (frame_t / 16 + i) % 4 == 0 ? 1 : 0;
    if (dim) spr_draw_ex(&ph_spr[c->type], x + 5 + ox, y + 1 + oy - bob, 0, NULL, C_SLATE);
    else spr_draw(&ph_spr[c->type], x + 5 + ox, y + 1 + oy - bob, 0);
    if (ox || oy) return;
    draw_stats(x + 3, y + 19, SLOT_W - 5, bd.pop, bd.cash);
    draw_corner_badges(&bd, x + 5, y + 1);
    seen.badges++;
    /* the ban: the guest under the cursor takes the blame */
    if (state == S_BAN && hi) {
        seen.ban_slot = i + 1;
        if ((frame_t / 10) % 3) red_tag("BANNED", x + SLOT_W / 2, y + 9);
    }
}

static void draw_room(void) {
    PhPlayer *p = ph_me(&G);
    int w = PANEL_X - 4;
    /* the back wall: whitewash, a tiled dado and a window on the sea */
    gfx_rect(0, 16, w, 150, C_LIGHT);
    gfx_rect(0, 16, w, 4, C_WHITE);
    for (int x = 0; x < w; x += 8) {
        gfx_rect(x, 20, 8, 2, (x / 8) % 2 ? C_BLUE : C_SKY);
    }
    /* the floor: warm terracotta and a blue-and-white rug under the guests */
    gfx_rect(0, 22, w, 144, C_TAN);
    for (int y = 22; y < 166; y += 8)
        for (int x = ((y - 22) / 8) % 2 * 8; x < w; x += 16) gfx_rect(x, y, 8, 8, C_EARTH);
    int rows = (p->cap + PH_ROW - 1) / PH_ROW;
    int fh = rows * SLOT_H;
    gfx_rect(GRID_X - 5, GRID_Y - 3, PH_ROW * SLOT_W + 10, fh + 4, C_NAVY);
    gfx_rectb(GRID_X - 5, GRID_Y - 3, PH_ROW * SLOT_W + 10, fh + 4, C_CREAM);
    gfx_rectb(GRID_X - 3, GRID_Y - 1, PH_ROW * SLOT_W + 6, fh, C_BLUE);
    for (int x = GRID_X; x < GRID_X + PH_ROW * SLOT_W; x += 6) { gfx_pset(x, GRID_Y - 3, C_WHITE); gfx_pset(x + 3, GRID_Y + fh, C_WHITE); }
    /* the free spaces on the rug */
    for (int i = G.party.n; i < p->cap; i++) {
        int x = slot_x(i), y = slot_y(i);
        gfx_dither(x + 5, y + 17, SLOT_W - 10, 7, C_BLUE, 8);
        gfx_rectb(x + 4, y + 16, SLOT_W - 8, 9, C_BLUE);
    }
    /* below the rug: the rest of the room, a sofa and a lemon tree */
    int below = GRID_Y + fh + 6;
    if (below < 150) {
        gfx_rect(12, below + 4, 60, 12, C_WINE);
        gfx_rect(12, below, 60, 6, C_RED);
        gfx_rect(8, below + 2, 6, 14, C_WINE);
        gfx_rect(70, below + 2, 6, 14, C_WINE);
        gfx_rect(170, below + 8, 14, 10, C_ORANGE);
        gfx_circ(177, below + 2, 9, C_FOREST);
        gfx_circ(176, below, 7, C_JADE);
        gfx_pset(173, below - 1, C_YELLOW);
        gfx_pset(180, below + 3, C_YELLOW);
        gfx_pset(177, below - 4, C_YELLOW);
    }
    /* paper lanterns strung across the ceiling */
    for (int i = 0; i < 9; i++) {
        int x = 10 + i * 24, sw = (frame_t / 20 + i) % 2;
        gfx_rect(x - 1 + sw, 16, 4, 4, i % 3 == 0 ? C_ORANGE : i % 3 == 1 ? C_AMBER : C_PINK);
    }
}

/* a paper lantern: the parties still to throw stand beside it */
static void icon_nights(int x, int y) {
    gfx_rect(x + 1, y + 1, 5, 6, C_ORANGE);
    gfx_rect(x, y + 2, 7, 4, C_ORANGE);
    gfx_hline(x + 2, x + 4, y, C_SLATE);
    gfx_pset(x + 3, y + 3, C_YELLOW);
}

/* nights left, counting tonight's party when one is on */
static int nights_left(const PhPlayer *p, bool tonight) {
    /* OPEN ALL NIGHT has no last night: its lantern counts the nights up */
    if (G.scen == PH_ENDLESS) return G.nights;
    int n = ph_night_limit(&G) - p->night + (tonight ? 1 : 0);
    return n < 0 ? 0 : n;
}

/* OPEN ALL NIGHT's shutdowns so far, as crosses by the neighbour's lamp */
static void draw_strikes(int x, int y) {
    if (G.scen != PH_ENDLESS) return;
    for (int k = 0; k < PH_ENDLESS_STRIKES; k++)
        icon_trouble(x + k * 9, y, k < G.strikes ? C_RED : C_DUSK);
}

/* the four star pips: blue until a star guest is in the house. OPEN ALL
 * NIGHT's goal grows past four, so it shows stars in / stars needed. */
static void draw_star_meter(int x, int y) {
    if (G.scen == PH_ENDLESS) {
        char b[32];
        int n = ph_stars(&G), goal = ph_goal(&G);
        icon_star(x, y, n >= goal ? C_YELLOW : C_BLUE);
        snprintf(b, sizeof b, "%d/%d", n, goal);
        text_draw(b, x + 10, y, n >= goal ? C_YELLOW : C_SKY);
        snprintf(b, sizeof b, "PARTIES %d", G.score);
        tiny_draw(b, x + 46, y + 1, C_CREAM);
        return;
    }
    int n = imin(ph_stars(&G), 4);
    for (int i = 0; i < 4; i++) icon_star(x + i * 9, y, i < n ? C_YELLOW : C_BLUE);
}

static void draw_topbar(void) {
    PhPlayer *p = ph_me(&G);
    gfx_rect(0, 0, SCREEN_W, 15, C_INK);
    gfx_hline(0, SCREEN_W - 1, 15, C_DUSK);
    char b[32];
    int x = 4;
    if (G.players == 2) { snprintf(b, sizeof b, "P%d", G.turn + 1); x = text_draw(b, 4, 4, C_CREAM) + 6; }
    icon_nights(x, 4);
    snprintf(b, sizeof b, "%d", nights_left(p, true));
    text_draw(b, x + 10, 4, C_CREAM);
    /* during the tally the counters run up with it */
    bool counting = state == S_RESULT && T.phase != TL_DONE;
    icon_pop(60, 4);
    int pop = counting ? tally_pop_shown() : p->pop, cash = counting ? T.cash : p->cash;
    snprintf(b, sizeof b, "%d", pop);
    int ex = text_draw(b, 70, 4, C_YELLOW);
    if (pop >= PH_POP_CAP) tiny_draw("MAX", ex + 1, 5, C_ORANGE);   /* Party House's caps */
    icon_cash(104, 4);
    snprintf(b, sizeof b, "%d", cash);
    ex = text_draw(b, 114, 4, C_LIME);
    if (cash >= PH_CASH_CAP) tiny_draw("MAX", ex + 1, 5, C_ORANGE);
    draw_star_meter(150, 4);
    /* the neighbour's window lights up at two rowdies: one more brings the police */
    draw_strikes(268, 5);
    bool lamp = ph_trouble(&G) >= 2;
    gfx_rect(300, 2, 12, 11, lamp ? C_YELLOW : C_NIGHT);
    gfx_rectb(300, 2, 12, 11, C_SLATE);
    gfx_vline(306, 3, 12, C_SLATE);
}

static void draw_info(int card, int x, int y, int w) {
    if (card < 0) return;
    const PhCard *c = &ph_me(&G)->card[card];
    const PhGuest *t = &PH_GUESTS[c->type];
    Badge b = badge_card(card);
    for (int i = 0; i < G.party.n; i++)
        if (G.party.house[i] == card) b = badge_slot(i);
    ui_panel(x, y, w, 60, C_NIGHT, t->traits & T_STAR ? C_AMBER : C_DUSK);
    tiny_draw(card_name(card), x + 4, y + 4, t->traits & T_STAR ? C_YELLOW : C_WHITE);
    int yy = y + 12;
    icon_pop(x + 4, yy);
    draw_num(b.pop, x + 13, yy + 1, b.pop < 0 ? C_RED : C_YELLOW, b.pop > 0 ? "+" : "");
    icon_cash(x + 34, yy);
    draw_num(b.cash, x + 43, yy + 1, b.cash < 0 ? C_RED : C_LIME, b.cash > 0 ? "+" : "");
    if (b.trouble) badge_trouble(x + w - 30, yy);
    if (b.star) ph_icon(PI_STAR, x + w - 21, yy + 1, C_YELLOW);
    badge_abil(x + w - 11, yy, &b);
    tiny_lines(t->does[0] ? t->does : "NO SPECIAL TALENT.", x + 4, yy + 10, C_LIGHT);
    tiny_lines(t->flavour, x + 4, y + 44, C_SLATE);
}

/* the guest waiting at the door (a peek), or the one who couldn't get in
 * (the fire marshal): the face in the doorway with the same badges */
static void draw_doorway_guest(int card, int dx, int dy, const char *red) {
    const PhCard *c = &ph_me(&G)->card[card];
    Badge b = badge_card(card);
    int fx = dx + 8, fy = dy + 12;
    spr_draw(&ph_spr[c->type], fx, fy, 0);
    draw_corner_badges(&b, fx, fy);
    if (b.pop || b.cash) {
        gfx_rect(dx + 4, fy + 17, 24, 7, C_INK);
        draw_stats(dx + 5, fy + 18, 22, b.pop, b.cash);
    }
    if (red && (frame_t / 10) % 3) red_tag(red, dx + 16, fy + 6);
}

static void draw_panel(void) {
    PhParty *pa = &G.party;
    int x = PANEL_X;
    gfx_rect(x - 4, 16, SCREEN_W - x + 4, 150, C_NIGHT);
    /* the door, in its arch */
    int dx = x + 22, dy = 22;
    gfx_rect(dx - 6, dy - 2, 44, 40, C_LIGHT);
    gfx_rect(dx - 4, dy, 40, 38, C_WHITE);
    bool full = pa->n >= ph_me(&G)->cap;
    bool open = pa->peek >= 0 || (frame_t - arrive[pa->n ? pa->house[pa->n - 1] : 0]) < 10;
    if (open) {
        gfx_rect(dx + 4, dy + 4, 24, 32, C_NAVY);
        for (int i = 0; i < 6; i++) gfx_pset(dx + 6 + (i * 7) % 20, dy + 6 + (i * 5) % 12, C_WHITE);
        if (pa->peek >= 0) {
            /* a peek: they wait here, badges and all, while the party goes on */
            draw_doorway_guest(pa->peek, dx, dy, full ? "NO ROOM" : NULL);
            seen.door = pa->peek + 1;
        }
    } else {
        spr_draw_scaled(&ph_spr[PS_DOOR], dx, dy + 4, 2, 0);
    }
    bool party = state == S_PARTY;
    /* the buttons: a peek adds TURN AWAY under the door, so they close up */
    int order[4], nb = panel_buttons(order), step = nb == 4 ? 11 : 14, bh = nb == 4 ? 10 : 12;
    for (int k = 0; k < nb; k++) {
        int by = 62 + k * step, id = order[k];
        bool hi = party && cur == id;
        int fill = id == CUR_DOOR ? C_JADE : id == CUR_END ? C_WINE : id == CUR_AWAY ? C_TEAL : C_NAVY;
        const char *l = id == CUR_DOOR ? (pa->peek >= 0 ? "LET THEM IN" : "OPEN THE DOOR") : id == CUR_END ? "END THE PARTY"
                      : id == CUR_AWAY ? "TURN AWAY" : "GUEST BOOK";
        gfx_rect(x, by, 92, bh, hi ? fill : C_DUSK);
        int tc = hi ? C_WHITE : C_GREY;
        if (id == CUR_DOOR && full) tc = hi ? C_ORANGE : C_SLATE;   /* no room: the door won't open */
        text_center(l, x + 46, by + (bh - 7) / 2, tc);
        if (hi) ui_cursor(x - 8, by + (bh - 7) / 2, frame_t);
    }
    int shown = info_card;
    if (state == S_TARGET || state == S_BAN) shown = tcur < pa->n ? pa->house[tcur] : -1;
    draw_info(shown, x - 2, 106, 98);
}

static void draw_party_screen(void) {
    gfx_cls(C_INK);
    draw_room();
    PhParty *pa = &G.party;
    for (int i = 0; i < pa->n; i++) {
        bool hi = (state == S_PARTY && cur == i) || ((state == S_TARGET || state == S_BAN) && tcur == i) ||
                  (state == S_RESULT && T.phase != TL_DONE && T.slot == i);
        bool tok = state == S_TARGET && ph_target_ok(&G, sel, i);
        bool dim = state == S_TARGET && !tok;
        draw_slot(i, hi, tok, dim);
        if (state == S_TARGET && i == sel) gfx_rectb(slot_x(i) + 1, slot_y(i), SLOT_W - 2, SLOT_H - 2, C_LIME);
        /* the matchmaker's pair */
        if (state == S_TARGET && hi && PH_GUESTS[ph_me(&G)->card[pa->house[sel]].type].action == A_CUPID && tok)
            gfx_rectb(slot_x(i + 1), slot_y(i + 1) - 1, SLOT_W, SLOT_H, C_PINK);
    }
    draw_panel();
    draw_topbar();
    /* the bottom line: the question after a shutdown, the red notice of
     * why it came, or who is banned from tonight */
    gfx_rect(0, 166, SCREEN_W, 14, C_INK);
    char b[80];
    int ban = banned_card();
    if (state == S_BAN) text_draw("WHO TAKES THE BLAME? THEY MISS THE NEXT PARTY.", 6, 169, C_GREY);
    else if (state == S_BUST && G.party.over == PO_POLICE) {
        snprintf(b, sizeof b, "TOO MUCH TROUBLE! %d RUCKUS! GUESTS AT ONCE", ph_trouble_status(&G));
        tiny_draw(b, 6, 170, C_RED);
    } else if (state == S_BUST) {
        if (fire_by >= 0 && fire_by < G.party.n)
            snprintf(b, sizeof b, "OVER CAPACITY! %s BROUGHT ONE TOO MANY", card_name(G.party.house[fire_by]));
        else snprintf(b, sizeof b, "OVER CAPACITY! NO ROOM FOR ONE MORE");
        tiny_draw(b, 6, 170, C_RED);
    } else if (ban >= 0 && G.party.where[ban] == W_OUT && state != S_RESULT) {
        seen.ban_house = 1;
        red_tag("BANNED", 20, 169);
        snprintf(b, sizeof b, "%s MISSES TONIGHT'S PARTY", card_name(ban));
        tiny_draw(b, 38, 170, C_RED);
    }
    if (msg_t > 0 && msg) {
        ui_panel(60, 70, 150, 20, C_NIGHT, msg_red ? C_RED : C_ORANGE);
        text_center(msg, 135, 76, msg_red ? C_RED : C_CREAM);
    }
}

/* a guest type told in full in a small panel (the fetch list's choice) */
static void draw_type_info(int ty, int x, int y, int w) {
    const PhGuest *t = &PH_GUESTS[ty];
    Badge b = badge_type(ty);
    ui_panel(x, y, w, 60, C_NIGHT, b.star ? C_AMBER : C_DUSK);
    tiny_draw(t->name, x + 4, y + 4, b.star ? C_YELLOW : C_WHITE);
    int yy = y + 12;
    icon_pop(x + 4, yy);
    draw_num(b.pop, x + 13, yy + 1, b.pop < 0 ? C_RED : C_YELLOW, b.pop > 0 ? "+" : "");
    icon_cash(x + 34, yy);
    draw_num(b.cash, x + 43, yy + 1, b.cash < 0 ? C_RED : C_LIME, b.cash > 0 ? "+" : "");
    if (b.trouble) badge_trouble(x + w - 30, yy);
    if (b.star) ph_icon(PI_STAR, x + w - 21, yy + 1, C_YELLOW);
    badge_abil(x + w - 11, yy, &b);
    tiny_lines(t->does[0] ? t->does : "NO SPECIAL TALENT.", x + 4, yy + 10, C_LIGHT);
    tiny_lines(t->flavour, x + 4, y + 44, C_SLATE);
}

/* one row of a guest list: the face, the name, the cost, what they pay and
 * the badges (the fetch list; the columns are the same as the guest book's) */
#define ROW_COST 110
#define ROW_POP 134
#define ROW_CASH 148
#define ROW_BADGE 164
static void draw_guest_row(int ty, int x, int y, bool hi, bool cost) {
    const PhGuest *t = &PH_GUESTS[ty];
    Badge b = badge_type(ty);
    spr_draw(&ph_spr[ty], x + 2, y - 4, 0);
    text_draw(t->name, x + 20, y, hi ? C_WHITE : b.star ? C_YELLOW : C_GREY);
    char buf[16];
    if (cost && t->cost >= 0) {
        icon_pop(x + ROW_COST, y);
        snprintf(buf, sizeof buf, "%d", t->cost);
        text_draw(buf, x + ROW_COST + 9, y, hi ? C_YELLOW : C_AMBER);
    }
    draw_stat_pop(b.pop, x + ROW_POP, y + 1);
    draw_stat_cash(b.cash, x + ROW_CASH, y + 1);
    if (b.trouble) badge_trouble(x + ROW_BADGE, y - 1);
    if (b.star) ph_icon(PI_STAR, x + ROW_BADGE + 8, y, C_YELLOW);
    badge_abil(x + ROW_BADGE + 15, y - 1, &b);
}

/* the fetch list (cabbie, sleuth, wish fish): every guest that can come,
 * with its cost, pay and badges; the one chosen is told in full by the door */
static void draw_fetch(void) {
    draw_party_screen();
    gfx_darken_rect(0, 16, PANEL_X - 4, 150, 2);
    ui_panel(4, 20, 208, 144, C_NIGHT, C_JADE);
    text_center("WHO SHOULD COME?", 108, 25, C_LIME);
    tiny_draw("GUEST", 30, 36, C_SLATE);
    tiny_draw("COST", 8 + ROW_COST, 36, C_SLATE);
    tiny_draw("PAYS", 8 + ROW_POP, 36, C_SLATE);
    tiny_draw("HAVE", 192, 36, C_SLATE);
    int top = imax(0, fcur - 9);
    for (int i = top; i < nfetch && i < top + 10; i++) {
        int y = 47 + (i - top) * 11;
        int ty = fetch_list[i];
        if (i == fcur) gfx_rect(8, y - 2, 200, 11, C_DUSK);
        draw_guest_row(ty, 8, y, i == fcur, true);
        seen.pick_rows++;
        char b[8];
        snprintf(b, sizeof b, "X%d", ph_count_type(&G, ty, W_POOL));
        tiny_draw(b, 198, y + 1, C_SLATE);
    }
    if (top > 0) tiny_draw(GLYPH_DOT GLYPH_DOT GLYPH_DOT, 104, 40, C_SKY);
    if (top + 10 < nfetch) tiny_draw(GLYPH_DOT GLYPH_DOT GLYPH_DOT, 104, 157, C_SKY);
    if (nfetch > 0 && fcur < nfetch) draw_type_info(fetch_list[fcur], PANEL_X - 2, 106, 98);
}

static void draw_confirm(void) {
    draw_party_screen();
    ui_panel(70, 64, 130, 46, C_NIGHT, C_WINE);
    text_center("END THE PARTY?", 135, 70, C_CREAM);
    text_draw("YES", 104, 88, menu_sel == 0 ? C_WHITE : C_SLATE);
    text_draw("NO", 150, 88, menu_sel == 1 ? C_WHITE : C_SLATE);
    ui_cursor(menu_sel == 0 ? 96 : 142, 88, frame_t);
}

/* the shutdown: the house stays in view with the culprits in red, the
 * notice says why in red over the door panel, and the van drives up */
static void draw_bust(void) {
    draw_party_screen();
    PhParty *pa = &G.party;
    bool police = pa->over == PO_POLICE;
    int t = state_t;
    int c = (t / 6) % 2 ? (police ? C_BLUE : C_RED) : (police ? C_RED : C_ORANGE);
    gfx_rect(0, 16, PANEL_X - 4, 2, c);
    gfx_rect(0, 164, PANEL_X - 4, 2, c);
    int x = PANEL_X - 2;
    gfx_rect(x - 2, 16, SCREEN_W - x + 2, 150, C_NIGHT);
    ui_panel(x, 18, 98, 146, C_NIGHT, c);
    static const uint8_t g1[] = {C_WHITE, C_SKY, C_BLUE}, g2[] = {C_WHITE, C_YELLOW, C_RED};
    ui_fancy_center(police ? "THE POLICE!" : "FIRE MARSHAL!", x + 49, 24, 1, police ? g1 : g2, 3, C_INK, -1);
    red_banner(police ? "TOO MUCH TROUBLE!" : "OVER CAPACITY!", x + 49, 36, 90);
    seen.notice = police ? 2 : 1;
    int yy = 52;
    if (police) {
        /* the RUCKUS! guests, lit up in the house */
        tiny_center("THREE RUCKUS! AT ONCE.", x + 49, yy, C_CREAM);
        tiny_center("THEY'RE MARKED IN RED.", x + 49, yy + 8, C_GREY);
    } else {
        int oc = pa->overflow - 1;
        if (oc >= 0 && oc < ph_me(&G)->ncards) {
            /* the one who couldn't get in */
            int ty = ph_me(&G)->card[oc].type;
            spr_draw(&ph_spr[ty], x + 8, yy - 2, 0);
            Badge bd = badge_card(oc);
            draw_corner_badges(&bd, x + 8, yy - 2);
            red_tag("CAN'T GET IN", x + 64, yy);
            tiny_center(card_name(oc), x + 64, yy + 9, C_CREAM);
        } else {
            tiny_center("THE HOUSE WAS FULL.", x + 49, yy + 4, C_CREAM);
        }
    }
    tiny_center("NO PAY TONIGHT.", x + 49, yy + 20, C_LIGHT);
    if (G.scen == PH_ENDLESS) {
        char b[40];
        snprintf(b, sizeof b, "STRIKE %d OF %d", G.strikes + 1, PH_ENDLESS_STRIKES);
        tiny_center(b, x + 49, yy + 29, G.strikes + 1 >= PH_ENDLESS_STRIKES ? C_RED : C_ORANGE);
    }
    /* the van drives up to the door */
    int vx = x - 30 + imin(t * 2, 56);
    gfx_rect(x + 2, 116, 94, 26, C_INK);
    gfx_hline(x + 2, x + 95, 141, C_SLATE);
    gfx_clip(x + 2, 116, 94, 26);
    spr_draw_scaled(&ph_spr[police ? PS_POLICE : PS_FIRE], vx, 120, 2, 0);
    if ((t / 6) % 2) gfx_rect(vx + 8, 117, 8, 3, police ? C_SKY : C_YELLOW);
    gfx_noclip();
    if (t > 100 && (t / 20) % 2) text_center("PRESS " GLYPH_A, x + 49, 148, C_WHITE);
}

/* the tally: the house stays in view, the guest being counted lights up
 * with its points rising over it, and the board on the right adds up */
static void draw_result(void) {
    draw_party_screen();
    PhParty *pa = &G.party;
    char b[48];
    /* the points rising over the guest in turn */
    bool pen = T.phase == TL_PAY && T.slot >= 0 && T.slot < pa->n && T.unpaid[T.slot] && T.unit;
    if (T.phase != TL_DONE && T.slot >= 0 && T.slot < pa->n && (T.now || T.nowc || pen)) {
        int s = T.slot, k = 0;
        b[0] = 0;
        if (pen) k = snprintf(b, sizeof b, "-7");
        if (T.now) k += snprintf(b + k, sizeof b - (size_t)k, "%+d", T.now);
        if (T.nowc) snprintf(b + k, sizeof b - (size_t)k, "%s%s$%d", k ? " " : "", T.nowc < 0 ? "-" : "+", T.nowc < 0 ? -T.nowc : T.nowc);
        int x = slot_x(s) + SLOT_W / 2, y = slot_y(s) + 6 - imin(T.age / 2, 6);
        int w = tiny_width(b);
        gfx_rect(x - w / 2 - 2, y - 1, w + 4, 7, C_INK);
        tiny_draw(b, x - w / 2, y, pen ? C_ORANGE : T.phase == TL_PAY ? C_ORANGE : C_YELLOW);
    }
    /* the board, over the door panel */
    int x = PANEL_X - 2;
    gfx_rect(x - 2, 16, SCREEN_W - x + 2, 150, C_NIGHT);
    ui_panel(x, 20, 98, 142, C_NIGHT, G.star_party || G.winner ? C_YELLOW : C_AMBER);
    static const uint8_t g1[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center("WHAT A NIGHT!", x + 49, 26, 1, g1, 3, C_INK, -1);
    if (T.why == WHY_FULL) tiny_center("THE HOUSE IS FULL", x + 49, 38, C_SKY);
    else if (T.why == WHY_NOBODY) tiny_center("NOBODY LEFT TO COME", x + 49, 38, C_SKY);
    icon_pop(x + 6, 50);
    snprintf(b, sizeof b, "FAME %+d", T.got_pop);
    text_draw(b, x + 18, 50, C_YELLOW);
    icon_cash(x + 6, 64);
    snprintf(b, sizeof b, "CASH %+d", T.got_cash);
    text_draw(b, x + 18, 64, C_LIME);
    if (T.pen) { snprintf(b, sizeof b, "UNPAID -%d FAME", T.pen); tiny_draw(b, x + 6, 78, C_ORANGE); }
    if (T.phase != TL_DONE && T.slot >= 0 && T.slot < pa->n) {
        tiny_center(T.phase == TL_PAY ? "PAYING" : "COUNTING", x + 49, 92, C_SLATE);
        tiny_center(card_name(pa->house[T.slot]), x + 49, 100, C_WHITE);
    }
    if (T.phase == TL_DONE) {
        if (G.star_party) {
            text_center(GLYPH_STAR " STAR PARTY! " GLYPH_STAR, x + 49, 90, (frame_t / 8) % 2 ? C_YELLOW : C_WHITE);
            snprintf(b, sizeof b, "%d SO FAR", G.score);
            tiny_center(b, x + 49, 102, C_CREAM);
            snprintf(b, sizeof b, "NEXT: %d STARS", ph_goal(&G));
            tiny_center(b, x + 49, 110, C_SKY);
        } else if (G.winner) {
            text_center(GLYPH_STAR " FOUR STARS! " GLYPH_STAR, x + 49, 90, (frame_t / 8) % 2 ? C_YELLOW : C_WHITE);
        }
        if ((T.done_t / 20) % 2 == 0) text_center("PRESS " GLYPH_A, x + 49, 146, C_WHITE);
    } else {
        text_center("HOLD " GLYPH_A " FASTER", x + 49, 146, btn(BTN_A) ? C_WHITE : C_SLATE);
    }
}

/* a guest card for the shop: the face with its badges, the price, what
 * they pay, and the buys left (a star: no limit) */
static void draw_card(int ty, int x, int y, bool hi, bool can) {
    const PhGuest *t = &PH_GUESTS[ty];
    Badge bd = badge_type(ty);
    bool star = bd.star;
    int bg = star ? C_BROWN : C_NIGHT;
    gfx_rect(x, y, 40, 40, bg);
    gfx_rectb(x, y, 40, 40, hi ? ((frame_t / 8) % 2 ? C_WHITE : C_YELLOW) : star ? C_AMBER : C_DUSK);
    spr_draw(&ph_spr[ty], x + 12, y + 3, 0);
    draw_corner_badges(&bd, x + 12, y + 3);
    char b[8];
    snprintf(b, sizeof b, "%d", t->cost);
    icon_pop(x + 3, y + 21);
    text_draw(b, x + 12, y + 21, can ? C_YELLOW : C_SLATE);
    draw_stock(ty, x + (star ? 29 : 26), y + 22);
    gfx_hline(x + 3, x + 36, y + 30, star ? C_TAN : C_DUSK);
    draw_stats(x + 3, y + 32, 34, bd.pop, bd.cash);
    if (!star && G.bought[ty] >= PH_STOCK) red_tag("SOLD OUT", x + 20, y + 11);
}

static void draw_shop(void) {
    gfx_cls(C_NIGHT);
    /* a market street at dusk */
    for (int y = 16; y < 180; y += 4) gfx_dither(0, y, SCREEN_W, 2, C_DUSK, 2);
    PhPlayer *p = ph_me(&G);
    gfx_rect(0, 0, SCREEN_W, 15, C_INK);
    gfx_hline(0, SCREEN_W - 1, 15, C_DUSK);
    char b[48];
    if (G.players == 2) snprintf(b, sizeof b, "P%d  THE SHOP", G.turn + 1);
    else snprintf(b, sizeof b, "THE SHOP");
    text_draw(b, 4, 4, C_CREAM);
    icon_nights(84, 4);
    snprintf(b, sizeof b, "%d", nights_left(p, false));
    text_draw(b, 94, 4, C_CREAM);
    icon_pop(130, 4);
    snprintf(b, sizeof b, "%d", p->pop);
    int ex = text_draw(b, 140, 4, C_YELLOW);
    if (p->pop >= PH_POP_CAP) tiny_draw("MAX", ex + 1, 5, C_ORANGE);
    icon_cash(176, 4);
    snprintf(b, sizeof b, "%d", p->cash);
    ex = text_draw(b, 186, 4, C_LIME);
    if (p->cash >= PH_CASH_CAP) tiny_draw("MAX", ex + 1, 5, C_ORANGE);
    snprintf(b, sizeof b, "HOUSE %d", p->cap);
    text_draw(b, 222, 4, C_SKY);
    for (int i = 0; i < G.npool; i++) {
        int row = i / SHOP_COLS - shop_top;
        if (row < 0 || row >= SHOP_SHOWN) continue;
        int x = 6 + (i % SHOP_COLS) * 43, y = 20 + row * 43;
        draw_card(G.pool[i], x, y, shop_cur == i, ph_can_buy(&G, G.pool[i]));
    }
    /* more rows above or below (OPEN ALL NIGHT's bigger shop) */
    int blink = (frame_t / 12) % 2 ? C_WHITE : C_SKY;
    if (shop_top > 0)
        for (int k = 0; k < 3; k++) gfx_hline(110 - k, 110 + k, 16 + k, blink);
    if (shop_top + SHOP_SHOWN < shop_rows())
        for (int k = 0; k < 3; k++) gfx_hline(110 - k, 110 + k, 151 - k, blink);
    /* space, the guest book and the next party */
    bool hs = shop_cur == G.npool + SH_SPACE, hn = shop_cur == G.npool + SH_NEXT, hb = shop_cur == G.npool + SH_BOOK;
    int cost = ph_expand_cost(p);
    gfx_rect(6, 152, 70, 20, hs ? C_TEAL : C_INK);
    gfx_rectb(6, 152, 70, 20, hs ? C_CYAN : C_DUSK);
    snprintf(b, sizeof b, "+1 SPACE $%d", cost);
    text_center(p->cap >= PH_MAX_HOUSE ? "FULL SIZE" : b, 41, 158, p->cash >= cost && p->cap < PH_MAX_HOUSE ? C_WHITE : C_SLATE);
    gfx_rect(80, 152, 66, 20, hb ? C_NAVY : C_INK);
    gfx_rectb(80, 152, 66, 20, hb ? C_SKY : C_DUSK);
    text_center("GUEST BOOK", 113, 158, hb ? C_WHITE : C_GREY);
    gfx_rect(150, 152, 68, 20, hn ? C_JADE : C_INK);
    gfx_rectb(150, 152, 68, 20, hn ? C_LIME : C_DUSK);
    text_center("NEXT PARTY", 184, 158, hn ? C_WHITE : C_GREY);
    /* the card on the counter */
    int ty = shop_type();
    if (ty >= 0) {
        const PhGuest *t = &PH_GUESTS[ty];
        Badge bd = badge_type(ty);
        ui_panel(222, 20, 94, 152, C_INK, t->traits & T_STAR ? C_AMBER : C_DUSK);
        spr_draw_scaled(&ph_spr[ty], 253, 24, 2, 0);
        if (bd.trouble) badge_trouble(229, 26);
        if (bd.star) ph_icon(PI_STAR, 302, 26, C_YELLOW);
        tiny_draw(t->name, 226, 59, t->traits & T_STAR ? C_YELLOW : C_WHITE);
        icon_pop(226, 67);
        draw_num(t->pop, 235, 68, t->pop < 0 ? C_RED : C_YELLOW, t->pop > 0 ? "+" : "");
        icon_cash(256, 67);
        draw_num(t->cash, 265, 68, t->cash < 0 ? C_RED : C_LIME, t->cash > 0 ? "+" : "");
        /* the ability family, and whether it's once a party */
        if (bd.abil) {
            badge_abil(226, 77, &bd);
            tiny_draw(PH_ICON_NAME[bd.abil], 236, 78, PH_ICON_COL[bd.abil]);
        }
        tiny_lines(t->does[0] ? t->does : "NO SPECIAL TALENT.", 226, 88, C_LIGHT);
        /* the stock: no limit for a star, four of the others */
        if (bd.star) {
            ph_icon(PI_INF, 226, 115, -1);
            tiny_draw("NO LIMIT", 236, 114, C_SKY);
        } else {
            int left = PH_STOCK - G.bought[ty];
            snprintf(b, sizeof b, left ? "%d OF %d LEFT" : "SOLD OUT", left, PH_STOCK);
            tiny_draw(b, 226, 114, left ? C_LIME : C_RED);
        }
        tiny_lines(t->flavour, 226, 124, C_SLATE);
    } else {
        ui_panel(222, 20, 94, 152, C_INK, C_DUSK);
        static const char *const BUTTON_INFO[3] = {
            [SH_SPACE] = "ONE MORE SPACE IN\nTHE HOUSE. EACH ONE\nCOSTS $1 MORE.",
            [SH_NEXT] = "DONE SHOPPING.\nTHROW THE NEXT\nPARTY.\n\nB FROM ANYWHERE\nCOMES HERE.",
            [SH_BOOK] = "EVERYONE IN YOUR\nBOOK OF GUESTS."};
        tiny_lines(BUTTON_INFO[iclamp(shop_cur - G.npool, 0, 2)], 226, 28, C_LIGHT);
    }
    /* the guest banned after the shutdown, till the next party is over */
    int ban = banned_card();
    if (ban >= 0) {
        seen.ban_shop = ban + 1;
        gfx_rect(224, 142, 90, 28, C_NIGHT);
        gfx_rectb(224, 142, 90, 28, C_RED);
        red_tag("BANNED", 247, 145);
        tiny_draw(card_name(ban), 228, 154, C_WHITE);
        tiny_draw("MISSES THE NEXT PARTY", 228, 161, C_RED);
    }
    if (msg_t > 0 && msg) {
        ui_panel(40, 80, 150, 20, C_NIGHT, msg_red ? C_RED : C_ORANGE);
        text_center(msg, 115, 86, msg_red ? C_RED : C_CREAM);
    }
}

/* the guest book: who is in the rolodex and how many of each. It doesn't
 * split them into here / still to come / out: nothing says the original
 * shows the draws that are left, and players say they can't see them. */
static void draw_list(void) {
    gfx_cls(C_NIGHT);
    PhPlayer *p = ph_me(&G);
    text_center("THE GUEST BOOK", 160, 4, C_CREAM);
    tiny_draw("GUEST", 20, 16, C_SLATE);
    tiny_draw("COST", ROW_COST, 16, C_SLATE);
    tiny_draw("PAYS", ROW_POP, 16, C_SLATE);
    tiny_draw("IN THE BOOK", 206, 16, C_SLATE);
    int rows = 0, shown = 0;
    int top = fcur, bty = banned_type();
    for (int ty = 0; ty < G_COUNT; ty++) {
        int total = 0;
        for (int i = 0; i < p->ncards; i++) total += p->card[i].type == ty;
        if (!total) continue;
        if (rows++ < top) continue;
        if (shown >= 13) continue;
        int y = 26 + shown * 11;
        draw_guest_row(ty, 0, y, false, true);
        char b[16];
        snprintf(b, sizeof b, "X%d", total);
        text_draw(b, 212, y, C_WHITE);
        /* the guest banned from the next party */
        if (ty == bty) { red_tag("BANNED", 256, y); seen.ban_book = ty + 1; }
        shown++;
    }
    char b[24];
    snprintf(b, sizeof b, "%d GUESTS", p->ncards);
    tiny_draw(b, 270, 171, C_GREY);
    text_draw(GLYPH_LEFT GLYPH_RIGHT " ICON GUIDE   " GLYPH_B " BACK", 4, 170, C_SLATE);
    if (fcur > 0 && fcur >= rows) fcur = rows - 1;
}

/* the icon guide (the owner's): every badge and ability icon, told in words.
 * It opens from the guest book (LEFT or RIGHT) and the START menu. */
static void legend_row(int id, int x, int y, const char *what) {
    Badge b;
    memset(&b, 0, sizeof b);
    b.abil = id;
    badge_abil(x, y - 1, &b);
    tiny_draw(what, x + 11, y, C_LIGHT);
}

static void draw_legend(void) {
    gfx_cls(C_NIGHT);
    gfx_rect(0, 0, SCREEN_W, 15, C_INK);
    gfx_hline(0, SCREEN_W - 1, 15, C_DUSK);
    text_center("ICON GUIDE", 160, 4, C_CREAM);
    /* the badges every guest wears */
    int x = 8, y = 22;
    tiny_draw("ON EVERY GUEST", x, y, C_AMBER);
    y += 10;
    draw_stat_pop(2, x, y);
    tiny_draw("FAME THEY BRING", x + 22, y, C_LIGHT);
    y += 9;
    draw_stat_pop(-1, x, y);
    tiny_draw("FAME THEY COST", x + 22, y, C_LIGHT);
    y += 9;
    draw_stat_cash(1, x, y);
    tiny_draw("CASH THEY BRING", x + 22, y, C_LIGHT);
    y += 9;
    draw_stat_cash(-1, x, y);
    tiny_draw("CASH THEY CHARGE", x + 22, y, C_LIGHT);
    y += 10;
    badge_trouble(x + 1, y - 1);
    tiny_draw("RUCKUS! THREE BRING", x + 22, y, C_LIGHT);
    tiny_draw("THE POLICE", x + 22, y + 6, C_LIGHT);
    y += 15;
    ph_icon(PI_STAR, x + 1, y, C_YELLOW);
    tiny_draw("STAR GUEST: FOUR WIN", x + 22, y, C_LIGHT);
    y += 10;
    ph_icon(PI_INF, x, y + 1, -1);
    tiny_draw("STAR: BUY WITHOUT LIMIT", x + 22, y, C_LIGHT);
    y += 9;
    tiny_draw("3/4", x, y, C_LIME);
    tiny_draw("BUYS LEFT OF FOUR", x + 22, y, C_LIGHT);
    y += 10;
    tag("BANNED", x + 13, y, C_RED, C_WHITE);
    tiny_draw("MISSES THE NEXT PARTY", x + 30, y + 1, C_LIGHT);
    y += 12;
    {
        Badge b;
        memset(&b, 0, sizeof b);
        b.abil = PI_BOOT;
        b.ready = true;
        badge_abil(x, y - 1, &b);
        b.ready = false;
        b.used = true;
        badge_abil(x + 9, y - 1, &b);
        tiny_draw("ACTION READY / USED", x + 22, y, C_LIGHT);
    }
    y += 10;
    tiny_draw("RED: WHAT CAN'T HAPPEN", x, y, C_RED);
    /* the abilities, one icon per family */
    int cx = 128, cy = 22;
    tiny_draw("ABILITIES", cx, cy, C_AMBER);
    static const struct { uint8_t id; const char *what; } ROWS[] = {
        {PI_FETCH, "FETCH A GUEST YOU CHOOSE"},
        {PI_BRING, "BRINGS GUESTS ALONG"},
        {PI_BOOT, "SENDS GUESTS HOME"},
        {PI_PEEK, "PEEKS AT THE DOOR"},
        {PI_SHUFFLE, "EVERYONE OUT, SHUFFLE"},
        {PI_SCORE, "A GUEST PAYS NOW"},
        {PI_STYLE, "+1 FAME FOR GOOD"},
        {PI_REFRESH, "ACTIONS AGAIN"},
        {PI_SWAP, "SWAPS A STAR"},
        {PI_CALM, "CALMS RUCKUS!"},
        {PI_FAMEUP, "MORE FAME (SEE THEM)"},
        {PI_CASHUP, "MORE CASH FOR RUCKUS!"},
        {PI_MOON, "RUCKUS! EVERY OTHER TIME"},
        {PI_CURSE, "LATER GUESTS ARE RUCKUS!"},
        {PI_ENCORE, "PAY NOW + ACTIONS AGAIN"},
    };
    for (int i = 0; i < ARRAY_LEN(ROWS); i++) {
        int ry = cy + 10 + i * 9;
        legend_row(ROWS[i].id, cx, ry, ROWS[i].what);
        seen.legend++;
    }
    text_draw(GLYPH_B " BACK", 4, 170, C_SLATE);
}

/* ------------------------------------------------------------------ */
/* title, menus, win and loss                                           */

static void draw_villa(int t, int base) {
    /* the night sea, the house and its lit windows */
    gfx_cls(C_NIGHT);
    for (int i = 0; i < 40; i++) gfx_pset((i * 71) % 320, (i * 37) % 70, (t / 30 + i) % 5 ? C_GREY : C_WHITE);
    gfx_circ(270, 26, 10, C_CREAM);
    gfx_circ(274, 24, 9, C_NIGHT);
    gfx_rect(0, base, SCREEN_W, SCREEN_H - base, C_NAVY);
    for (int y = base + 2; y < SCREEN_H; y += 5) gfx_hline((t / 3 + y * 7) % 40, (t / 3 + y * 7) % 40 + 12, y, C_BLUE);
    int hx = 70, hy = base - 70;
    gfx_rect(hx, hy + 14, 180, 70, C_LIGHT);
    gfx_rect(hx - 6, hy + 8, 192, 8, C_WHITE);
    gfx_rect(hx + 60, hy - 6, 60, 16, C_WHITE);
    gfx_circ(hx + 90, hy - 6, 14, C_WHITE);
    gfx_rect(hx + 88, hy - 26, 4, 8, C_WHITE);
    for (int i = 0; i < 5; i++) {
        int wx = hx + 10 + i * 34, wy = hy + 26;
        bool lit = (t / 40 + i * 3) % 7 != 0;
        gfx_rect(wx, wy, 18, 24, lit ? C_YELLOW : C_DUSK);
        gfx_circ(wx + 9, wy, 9, lit ? C_YELLOW : C_DUSK);
        if (lit) {
            /* dancers in the windows */
            int s = (t / 12 + i) % 2;
            gfx_rect(wx + 5 + s, wy + 8, 4, 10, C_BROWN);
            gfx_circ(wx + 7 + s, wy + 6, 2, C_BROWN);
            gfx_rect(wx + 11 - s, wy + 10, 3, 8, C_BROWN);
        }
        gfx_rect(wx - 1, wy + 24, 20, 2, C_BLUE);
    }
    gfx_rect(hx + 82, hy + 56, 16, 28, C_BLUE);
    gfx_circ(hx + 90, hy + 56, 8, C_BLUE);
    /* lantern strings */
    for (int i = 0; i < 12; i++) {
        int lx = hx - 4 + i * 17, ly = hy + 12 + (i % 2) * 3;
        gfx_rect(lx, ly, 3, 4, (t / 15 + i) % 3 ? C_ORANGE : C_AMBER);
    }
}

static void draw_title(void) {
    draw_villa(frame_t, 150);
    /* fireworks */
    for (int k = 0; k < 3; k++) {
        int ph = (frame_t + k * 50) % 150;
        if (ph > 60) continue;
        int cx = 60 + k * 100, cy = 40 + (k % 2) * 10;
        for (int i = 0; i < 12; i++) {
            float a = i / 12.0f * 6.2832f;
            gfx_pset(cx + (int)(cosf(a) * ph * 0.5f), cy + (int)(sinf(a) * ph * 0.5f) + ph / 8, k == 0 ? C_PINK : k == 1 ? C_CYAN : C_YELLOW);
        }
    }
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_center("OPEN HOUSE", 160, 8, 3, grad, 4, C_INK, C_WINE);
    tiny_center("FOUR STARS UNDER ONE ROOF", 160, 36, C_CREAM);
    if (state_t > 20 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 160, C_WHITE);
}

static void draw_mode(void) {
    draw_villa(frame_t, 150);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(90, 40, 140, 80, C_NIGHT, C_AMBER);
    static const char *const M[3] = {"CONTINUE", "1 PLAYER", "2 PLAYERS"};
    for (int i = 0; i < 3; i++) {
        int y = 52 + i * 16;
        bool off = i == 0 && !sv.has_run;
        if (i == menu_sel) gfx_rect(96, y - 3, 128, 13, C_DUSK);
        text_draw(M[i], 116, y, off ? C_DUSK : i == menu_sel ? C_WHITE : C_GREY);
        if (i == menu_sel) ui_cursor(104, y, frame_t);
    }
    char b[48];
    snprintf(b, sizeof b, "STREAK %d  " GLYPH_DOT "  BEST %d", sv.streak, sv.best_streak);
    tiny_center(b, 160, 130, C_CREAM);
}

static const char *scen_name(int i) {
    return i == PH_CUSTOM ? "CUSTOM LIST" : i == PH_ENDLESS ? "OPEN ALL NIGHT" : i == PH_RANDOM ? "RANDOM GUEST LIST" : PH_SCEN[i % PH_SCENARIOS].name;
}
/* the list an OPEN ALL NIGHT run plays */
static const char *base_name(int b) { return b == PH_ENDLESS ? "THE BIG MIX" : b == PH_CUSTOM ? "THE CUSTOM LIST" : scen_name(b); }

/* a tile's place on the list grid */
static void scen_tile(int i, int *x, int *y, int *w, int *h) {
    if (i == PH_ENDLESS) { *x = 9; *y = 108; *w = 200; *h = 26; return; }
    if (i == PH_CUSTOM) { *x = 213; *y = 108; *w = 98; *h = 26; return; }
    *x = 9 + (i % SCEN_COLS) * 102;
    *y = 20 + (i / SCEN_COLS) * 44;
    *w = 98;
    *h = 40;
}

static void draw_scen(void) {
    draw_villa(frame_t, 150);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    bool ns = scen_endless;   /* picking a list for OPEN ALL NIGHT */
    ui_panel(4, 4, 312, 172, C_NIGHT, ns ? C_SKY : C_AMBER);
    text_center(ns ? "OPEN ALL NIGHT: PICK A GUEST LIST" : mode_players == 2 ? "2 PLAYERS: CHOOSE A GUEST LIST" : "CHOOSE A GUEST LIST",
                160, 9, ns ? C_SKY : C_CREAM);
    char b[48];
    for (int i = 0; i < SCEN_ROWS; i++) {
        int x, y, w, h;
        scen_tile(i, &x, &y, &w, &h);
        bool hi = i == scen_sel;
        /* BEATEN: a gold tile with a crown (the owner's); every list is open */
        bool won = !ns && beaten(i);
        gfx_rect(x, y, w, h, hi ? C_DUSK : won ? C_BROWN : C_INK);
        if (won) gfx_rectb(x + 1, y + 1, w - 2, h - 2, C_AMBER);
        gfx_rectb(x, y, w, h, hi ? ((frame_t / 8) % 2 ? C_WHITE : C_YELLOW) : won ? C_YELLOW : ns ? C_BLUE : C_SLATE);
        if (won) { ph_icon(PI_CROWN, x + w - 11, y + 3, -1); seen.crowns++; }
        if (!ns && i != PH_ENDLESS) {
            const char *st = won ? "BEATEN" : "UNBEATEN";
            int sx = i == PH_CUSTOM ? x + w - 4 - tiny_width(st) : x + 4;
            tiny_draw(st, sx, i == PH_CUSTOM ? y + 15 : y + h - 8, won ? C_YELLOW : C_SLATE);
        }
        int col = hi ? C_WHITE : C_GREY;
        if (i < PH_RANDOM) {
            snprintf(b, sizeof b, "%d %s", i + 1, PH_SCEN[i].name);
            tiny_draw(b, x + 4, y + 4, won ? C_YELLOW : col);
            /* the list's stars */
            int k = 0;
            for (int j = 0; j < PH_SCEN[i].n; j++) {
                int ty = PH_SCEN[i].pool[j];
                if (!(PH_GUESTS[ty].traits & T_STAR)) continue;
                spr_draw(&ph_spr[ty], x + 8 + k * 22, y + 14 - (hi && (frame_t / 12 + k) % 2), 0);
                k++;
            }
            if (!ns) for (int j = 0; j < k; j++) icon_star(x + w - 12 - j * 9, y + h - 10, C_AMBER);
        } else if (i == PH_RANDOM) {
            tiny_draw("? RANDOM LIST", x + 4, y + 4, won ? C_YELLOW : col);
            /* a pair of dice */
            for (int d = 0; d < 2; d++) {
                int dx = x + 10 + d * 20, dy = y + 14 + d * 2;
                gfx_rect(dx, dy, 14, 14, C_CREAM);
                gfx_rectb(dx, dy, 14, 14, C_SLATE);
                gfx_rect(dx + 3, dy + 3, 2, 2, C_INK);
                gfx_rect(dx + 9, dy + 9, 2, 2, C_INK);
                if (d) gfx_rect(dx + 6, dy + 6, 2, 2, C_INK);
            }
            if (!ns) { snprintf(b, sizeof b, "STREAK %d", sv.streak); tiny_draw(b, x + 54, y + 20, C_CREAM); }
        } else if (i == PH_ENDLESS) {
            /* the wide tile: OPEN ALL NIGHT, or (picking for it) the big mix */
            gfx_circ(x + 12, y + 13, 7, C_CREAM);
            gfx_circ(x + 15, y + 11, 6, hi ? C_DUSK : C_INK);
            if (ns) {
                text_draw("+ THE BIG MIX", x + 26, y + 5, col);
                tiny_draw("THE WHOLE ROSTER, AT RANDOM", x + 26, y + 16, C_GREY);
            } else {
                text_draw("+ OPEN ALL NIGHT", x + 26, y + 5, col);
                tiny_draw("NO LAST NIGHT. PICK A LIST", x + 26, y + 16, C_SKY);
            }
        } else {
            /* the custom list: a guest list with ticks */
            gfx_rect(x + 5, y + 5, 11, 15, C_CREAM);
            for (int k = 0; k < 3; k++) {
                gfx_hline(x + 7, x + 9, y + 8 + k * 4, C_JADE);
                gfx_hline(x + 11, x + 14, y + 8 + k * 4, C_SLATE);
            }
            tiny_draw("CUSTOM LIST", x + 21, y + 5, won ? C_YELLOW : col);
            if (!ns) {
                snprintf(b, sizeof b, "%d + %d", custom_count(false), custom_count(true));
                tiny_draw(b, x + 21, y + 15, C_GREY);
            }
        }
        /* picking for OPEN ALL NIGHT: each list's own best */
        if (ns) {
            snprintf(b, sizeof b, "BEST %d", sv.night_best[i]);
            if (i >= PH_ENDLESS) tiny_draw(b, x + w - 4 - tiny_width(b), y + 5 + (i == PH_CUSTOM) * 10, C_YELLOW);
            else tiny_draw(b, x + w - 4 - tiny_width(b), y + h - 9, C_YELLOW);
        }
    }
    /* the chosen tile, told in full underneath */
    int s = scen_sel, ty0 = 140;
    if (ns) {
        tiny_lines("NO LAST NIGHT. EACH STAR PARTY NEEDS\nONE STAR MORE. THE THIRD SHUTDOWN\nCLOSES THE HOUSE FOR GOOD.", 12, ty0, C_LIGHT);
        snprintf(b, sizeof b, "%s", base_name(s));
        tiny_draw(b, 200, ty0, C_WHITE);
        snprintf(b, sizeof b, "BEST %d STAR PARTIES", sv.night_best[s]);
        tiny_draw(b, 200, ty0 + 8, C_YELLOW);
        snprintf(b, sizeof b, "TOP FAME %d", sv.night_fame[s]);
        tiny_draw(b, 200, ty0 + 16, C_GREY);
        tiny_draw("B: BACK", 270, ty0 + 24, C_SLATE);
    } else if (s == PH_CUSTOM) {
        tiny_lines("THE OWNER'S OWN: PICK THE GUESTS AND STARS\nTHE SHOP SELLS. 25 NIGHTS AND FOUR STARS\nTO WIN, OR PLAY IT ALL NIGHT.", 12, ty0, C_LIGHT);
        snprintf(b, sizeof b, "WINS %d", sv.cwins);
        tiny_draw(b, 200, ty0, C_CREAM);
        if (sv.cfastest) snprintf(b, sizeof b, "FASTEST %d NIGHTS", sv.cfastest);
        else snprintf(b, sizeof b, "FASTEST -");
        tiny_draw(b, 200, ty0 + 8, C_YELLOW);
        snprintf(b, sizeof b, "ALL NIGHT BEST %d", sv.night_best[PH_CUSTOM]);
        tiny_draw(b, 200, ty0 + 16, C_GREY);
    } else if (s == PH_ENDLESS) {
        tiny_lines("THE OWNER'S ENDLESS MODE. PLAY ANY LIST,\nRANDOM OR THE BIG MIX, WITH NO LAST\nNIGHT, FOR A BEST SCORE.", 12, ty0, C_LIGHT);
        if (mode_players == 2) tiny_draw("ONE PLAYER ONLY.", 220, ty0, C_SLATE);
    } else if (s < PH_SCENARIOS) {
        tiny_lines(PH_SCEN[s].blurb, 12, ty0, C_LIGHT);
        int k = 0;
        for (int i = 0; i < PH_SCEN[s].n; i++) {
            int ty = PH_SCEN[s].pool[i];
            if (!(PH_GUESTS[ty].traits & T_STAR)) continue;
            tiny_draw(PH_GUESTS[ty].name, 200, ty0 + k * 8, C_YELLOW);
            k++;
        }
    } else {
        tiny_lines("TWO STARS AND ELEVEN GUESTS,\nDEALT AT RANDOM. EVERY LIST\nHAS A CODE TO PLAY IT AGAIN.", 12, ty0, C_LIGHT);
        snprintf(b, sizeof b, "STREAK %d  BEST %d", sv.streak, sv.best_streak);
        tiny_draw(b, 180, ty0, C_CREAM);
        int c = code_get();
        if (c >= 0) { snprintf(b, sizeof b, "LAST CODE %06d", c); tiny_draw(b, 180, ty0 + 8, C_GREY); }
        tiny_draw("A TYPED CODE DOESN'T COUNT", 180, ty0 + 16, C_SLATE);
    }
    if (msg_t > 0 && msg) {
        ui_panel(60, 76, 200, 20, C_NIGHT, C_ORANGE);
        text_center(msg, 160, 82, C_CREAM);
    }
}

/* the custom list's editor: the grid of guests, the guest under the
 * cursor told in full, and the buttons */
#define ED_X 4
#define ED_Y 18
#define ED_W 26
#define ED_H 25
#define ED_PX 214

static void draw_custom(void) {
    gfx_cls(C_NIGHT);
    char b[64];
    /* the top bar: the counters and the list's own record */
    gfx_rect(0, 0, SCREEN_W, 15, C_INK);
    gfx_hline(0, SCREEN_W - 1, 15, C_DUSK);
    text_draw("CUSTOM LIST", 4, 4, C_CREAM);
    int ng = custom_count(false), ns = custom_count(true);
    snprintf(b, sizeof b, "GUESTS %d", ng);
    text_draw(b, 82, 4, ng >= PH_CUSTOM_MIN ? C_LIME : C_ORANGE);
    icon_star(140, 4, ns >= PH_CUSTOM_MIN_STARS ? C_YELLOW : C_ORANGE);
    snprintf(b, sizeof b, "STARS %d", ns);
    text_draw(b, 150, 4, ns >= PH_CUSTOM_MIN_STARS ? C_YELLOW : C_ORANGE);
    if (sv.cendless) snprintf(b, sizeof b, "ALL NIGHT BEST %d", sv.night_best[PH_CUSTOM]);
    else if (sv.cfastest) snprintf(b, sizeof b, "WINS %d  FASTEST %d", sv.cwins, sv.cfastest);
    else snprintf(b, sizeof b, "WINS %d", sv.cwins);
    tiny_draw(b, SCREEN_W - 4 - tiny_width(b), 5, C_GREY);
    /* every guest: in (bright, a tick), out (a shadow) or locked in */
    for (int t = 0; t < G_COUNT; t++) {
        int x = ED_X + (t % ED_COLS) * ED_W, y = ED_Y + (t / ED_COLS) * ED_H;
        const PhGuest *g = &PH_GUESTS[t];
        bool star = g->traits & T_STAR, lock = ph_custom_locked(t), in = custom_in(t), hi = ed_cur == t;
        gfx_rect(x, y, ED_W - 2, ED_H - 2, in ? (star ? C_BROWN : C_DUSK) : C_INK);
        gfx_rectb(x, y, ED_W - 2, ED_H - 2, hi ? ((frame_t / 8) % 2 ? C_WHITE : C_YELLOW) : in ? (star ? C_AMBER : C_SLATE) : C_NIGHT);
        spr_draw(&ph_spr[t], x + 3, y + 1, 0);
        if (!in) gfx_darken_rect(x + 3, y + 1, 16, 16, 2);   /* out: the face in the shade */
        /* the same badges as everywhere, dimmed while out */
        Badge bd = badge_type(t);
        bd.used = !in;
        if (bd.trouble) badge_trouble(x + 2, y + 2);
        if (star) ph_icon(PI_STAR, x + 17, y + 2, in ? C_YELLOW : C_SLATE);
        badge_abil(x + 16, y + 9, &bd);
        if (g->cost >= 0) {
            snprintf(b, sizeof b, "%d", g->cost);
            int ex = tiny_draw(b, x + 2, y + 17, in ? C_YELLOW : C_SLATE);
            if (star) ph_icon(PI_INF, ex, y + 18, in ? -1 : C_SLATE);   /* a star: no limit */
        }
        if (lock) text_draw(GLYPH_LOCK, x + 17, y + 16, C_CREAM);
        else if (in) text_draw(GLYPH_CHECK, x + 17, y + 16, C_LIME);
    }
    /* the guest under the cursor, or what the button does */
    if (ed_cur < ED_BTN) {
        const PhGuest *g = &PH_GUESTS[ed_cur];
        bool star = g->traits & T_STAR, lock = ph_custom_locked(ed_cur);
        ui_panel(ED_PX, ED_Y, 102, 96, C_INK, star ? C_AMBER : C_DUSK);
        tiny_draw(g->name, ED_PX + 4, ED_Y + 4, star ? C_YELLOW : C_WHITE);
        const char *st = ed_cur == G_ROWDY ? "IN EVERY GUEST BOOK" : lock ? "ALWAYS IN THE SHOP"
                       : custom_in(ed_cur) ? "IN THE SHOP" : "NOT INVITED";
        int sc = lock ? C_CREAM : custom_in(ed_cur) ? C_LIME : C_SLATE, sx = ED_PX + 4;
        if (lock || custom_in(ed_cur)) sx = text_draw(lock ? GLYPH_LOCK : GLYPH_CHECK, sx, ED_Y + 11, sc) + 3;
        tiny_draw(st, sx, ED_Y + 12, sc);
        Badge bd = badge_type(ed_cur);
        if (g->cost >= 0) snprintf(b, sizeof b, star ? "COST %d FAME" : "COST %d FAME, 4 EACH", g->cost);
        else snprintf(b, sizeof b, "CAN'T BE BOUGHT");
        int ex = tiny_draw(b, ED_PX + 4, ED_Y + 21, C_YELLOW);
        if (star && g->cost >= 0) {
            /* a star can be bought again and again */
            ph_icon(PI_INF, ex + 2, ED_Y + 22, -1);
            tiny_draw("NO LIMIT", ex + 11, ED_Y + 21, C_SKY);
        }
        icon_pop(ED_PX + 4, ED_Y + 29);
        draw_num(g->pop, ED_PX + 13, ED_Y + 30, g->pop < 0 ? C_RED : C_YELLOW, g->pop > 0 ? "+" : "");
        icon_cash(ED_PX + 36, ED_Y + 29);
        draw_num(g->cash, ED_PX + 45, ED_Y + 30, g->cash < 0 ? C_RED : C_LIME, g->cash > 0 ? "+" : "");
        if (bd.trouble) badge_trouble(ED_PX + 72, ED_Y + 29);
        if (bd.star) ph_icon(PI_STAR, ED_PX + 81, ED_Y + 30, C_YELLOW);
        if (bd.abil) {
            badge_abil(ED_PX + 4, ED_Y + 39, &bd);
            tiny_draw(PH_ICON_NAME[bd.abil], ED_PX + 14, ED_Y + 40, PH_ICON_COL[bd.abil]);
        }
        int yy = tiny_wrap(g->does[0] ? g->does : "NO SPECIAL TALENT.", ED_PX + 4, ED_Y + (bd.abil ? 49 : 41), 94, C_LIGHT);
        tiny_wrap(g->flavour, ED_PX + 4, imax(yy + 3, ED_Y + 74), 94, C_SLATE);
    } else {
        static const char *const BUTTON_INFO[EB_COUNT] = {
            [EB_PLAY] = "PLAY THIS LIST. IT\nNEEDS A STAR AND 6\nGUESTS BESIDES THE\nLOCKED ONES.",
            [EB_NIGHT] = "ON: PLAY IT AS OPEN\nALL NIGHT, WITH NO\nLAST NIGHT (ONE\nPLAYER). OFF: 25\nNIGHTS, FOUR STARS\nTO WIN.",
            [EB_CLEAR] = "TAKE EVERYONE OUT\nBUT THE LOCKED\nGUESTS.",
            [EB_RANDOM] = "A RANDOM LIST: ONE\nTO THREE STARS AND\n6 TO 16 GUESTS."};
        ui_panel(ED_PX, ED_Y, 102, 96, C_INK, C_DUSK);
        tiny_lines(BUTTON_INFO[ed_cur - ED_BTN], ED_PX + 4, ED_Y + 6, C_LIGHT);
    }
    static const char *const LABEL[EB_COUNT] = {"PLAY", NULL, "CLEAR ALL", "RANDOMISE"};
    for (int k = 0; k < EB_COUNT; k++) {
        int y = 118 + k * 14;
        bool hi = ed_cur == ED_BTN + k;
        int fill = k == EB_PLAY ? C_JADE : k == EB_NIGHT ? C_NAVY : C_TEAL;
        gfx_rect(ED_PX, y, 102, 12, hi ? fill : C_INK);
        gfx_rectb(ED_PX, y, 102, 12, hi ? C_WHITE : C_DUSK);
        const char *l = LABEL[k] ? LABEL[k] : sv.cendless ? "ALL NIGHT: ON" : "ALL NIGHT: OFF";
        text_center(l, ED_PX + 51, y + 3, hi ? C_WHITE : k == EB_NIGHT && sv.cendless ? C_SKY : C_GREY);
    }
    text_draw(GLYPH_DPAD " MOVE  " GLYPH_A " IN/OUT  " GLYPH_B " BACK", ED_X, 170, C_SLATE);
    if (msg_t > 0 && msg) {
        ui_panel(40, 76, 150, 20, C_NIGHT, C_ORANGE);
        text_center(msg, 115, 82, C_CREAM);
    }
    /* a list that can't be played says why */
    if (ed_warn) {
        static const char *const TITLE[] = {"", "PICK A STAR GUEST", "PICK AT LEAST 6 GUESTS", "OPEN ALL NIGHT IS 1P"};
        static const char *const WHY[] = {"",
            "YOU WIN WITH FOUR STARS AT ONE PARTY,\nSO THE SHOP MUST SELL AT LEAST ONE.\nA STAR CAN BE BOUGHT AGAIN AND AGAIN.",
            "6 BESIDES THE LOCKED ONES AND THE STARS:\nHALF A SET LIST, SO THE SHOP HAS REAL\nCHOICES FOR CALMING THE ROWDY MATES\nAND EARNING THE FAME STARS COST.",
            "TWO PLAYERS PLAY 25 NIGHTS.\nTURN ALL NIGHT OFF TO PLAY THIS LIST."};
        ui_panel(30, 56, 260, 66, C_NIGHT, C_ORANGE);
        text_center(TITLE[ed_warn], 160, 62, C_CREAM);
        tiny_lines(WHY[ed_warn], 40, 76, C_LIGHT);
        text_center("PRESS " GLYPH_A, 160, 110, (frame_t / 20) % 2 ? C_WHITE : C_GREY);
    }
}

static void draw_rpick(void) {
    draw_scen();
    ui_panel(90, 60, 140, 46, C_NIGHT, C_SKY);
    text_draw("DEAL A NEW LIST", 112, 70, menu_sel == 0 ? C_WHITE : C_GREY);
    text_draw("TYPE A CODE", 112, 86, menu_sel == 1 ? C_WHITE : C_GREY);
    ui_cursor(100, menu_sel ? 86 : 70, frame_t);
}

static void draw_intro(void) {
    draw_villa(frame_t, 150);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(10, 10, 300, 160, C_NIGHT, C_AMBER);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center(scen_name(G.scen), 160, 16, 1, grad, 3, C_INK, -1);
    if (G.scen == PH_ENDLESS) {
        char sub[64];
        snprintf(sub, sizeof sub, "%s, WITH NO LAST NIGHT%s", base_name(G.base), G.base == PH_ENDLESS ? ". NEW FACES EVERY 5 NIGHTS" : "");
        tiny_center(sub, 160, 32, C_GREY);
    } else {
        tiny_center(G.scen < PH_SCENARIOS ? "THE GUESTS YOU CAN INVITE THIS SUMMER"
                    : G.scen == PH_CUSTOM ? "THE GUESTS YOU PICKED, FOR 25 NIGHTS" : "TONIGHT'S RANDOM GUEST LIST", 160, 32, C_GREY);
    }
    /* a big custom list packs its faces closer */
    bool big = G.npool > 24;
    for (int i = 0; i < G.npool; i++) {
        int ty = G.pool[i];
        int x = big ? 16 + (i % 12) * 24 : 22 + (i % 8) * 36, y = big ? 42 + (i / 12) * 24 : 44 + (i / 8) * 40;
        spr_draw(&ph_spr[ty], x + 6, y, 0);
        if (PH_GUESTS[ty].traits & T_STAR) icon_star(x + (big ? 16 : 20), y - 2, C_YELLOW);
        char b[8];
        snprintf(b, sizeof b, "%d", PH_GUESTS[ty].cost);
        tiny_center(b, x + 14, y + 17 + !big, C_YELLOW);
    }
    if (G.scen == PH_RANDOM && run_code >= 0) {
        char b[24];
        snprintf(b, sizeof b, "CODE %06d", run_code);
        text_center(b, 160, 140, C_SKY);
    }
    if (state_t > 20 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 154, C_WHITE);
}

static void draw_code(void) {
    draw_scen();
    ui_panel(80, 56, 160, 70, C_NIGHT, C_SKY);
    text_center("TYPE IN A CODE", 160, 62, C_SKY);
    for (int i = 0; i < 6; i++) {
        int x = 106 + i * 18, y = 80;
        bool hi = i == code_cur;
        gfx_rect(x, y, 14, 16, hi ? C_DUSK : C_INK);
        gfx_rectb(x, y, 14, 16, hi ? ((frame_t / 8) % 2 ? C_WHITE : C_YELLOW) : C_SLATE);
        char d[2] = {(char)('0' + code_dig[i]), 0};
        text_center(d, x + 7, y + 5, hi ? C_WHITE : C_GREY);
    }
    text_center(GLYPH_UP GLYPH_DOWN " DIGIT  " GLYPH_A " PLAY", 160, 106, C_GREY);
}

static void draw_turn(void) {
    draw_villa(frame_t, 150);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    ui_panel(70, 30, 180, 90, C_NIGHT, G.turn ? C_MAGENTA : C_SKY);
    static const uint8_t g1[] = {C_WHITE, C_CYAN, C_SKY}, g2[] = {C_WHITE, C_PINK, C_MAGENTA};
    char b[64];
    snprintf(b, sizeof b, "PLAYER %d", G.turn + 1);
    ui_fancy_center(b, 160, 36, 2, G.turn ? g2 : g1, 3, C_INK, C_NIGHT);
    snprintf(b, sizeof b, "%d NIGHTS LEFT", nights_left(ph_me(&G), true));
    text_center(b, 160, 60, C_CREAM);
    for (int p = 0; p < 2; p++) {
        snprintf(b, sizeof b, "P%d  POP %d  $%d  HOUSE %d", p + 1, G.pl[p].pop, G.pl[p].cash, G.pl[p].cap);
        tiny_center(b, 160, 76 + p * 9, p == G.turn ? C_WHITE : C_SLATE);
    }
    if (state_t > 30 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 102, C_WHITE);
}

static void draw_end(void) {
    bool win = state == S_WIN;
    if (win) {
        draw_villa(frame_t, 150);
        for (int k = 0; k < 5; k++) {
            int ph = (frame_t * 2 + k * 37) % 120;
            if (ph > 70) continue;
            int cx = 30 + k * 64, cy = 30 + (k % 3) * 12;
            for (int i = 0; i < 16; i++) {
                float a = i / 16.0f * 6.2832f;
                gfx_rect(cx + (int)(cosf(a) * ph * 0.6f), cy + (int)(sinf(a) * ph * 0.6f) + ph / 6, 2, 2, (k + i) % 3 == 0 ? C_YELLOW : k % 2 ? C_PINK : C_CYAN);
            }
        }
    } else {
        draw_villa(0, 150);
        gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 3);
    }
    ui_panel(40, 60, 240, 70, C_NIGHT, win ? C_YELLOW : C_SLATE);
    static const uint8_t g1[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER}, g2[] = {C_LIGHT, C_GREY, C_SLATE};
    char b[48];
    if (G.players == 2) snprintf(b, sizeof b, G.winner ? "PLAYER %d WINS!" : "NOBODY WINS", G.winner);
    else if (G.scen == PH_ENDLESS) snprintf(b, sizeof b, "LIGHTS OUT");
    else snprintf(b, sizeof b, win ? "FOUR STARS!" : "SUMMER'S OVER");
    ui_fancy_center(b, 160, 68, 2, win ? g1 : g2, win ? 4 : 3, C_INK, win ? C_WINE : C_NIGHT);
    if (win) {
        /* the four stars on the terrace */
        int k = 0;
        PhPlayer *p = ph_me(&G);
        for (int i = 0; i < G.party.n && k < 4; i++) {
            int ty = p->card[G.party.house[i]].type;
            if (!(PH_GUESTS[ty].traits & T_STAR)) continue;
            spr_draw(&ph_spr[ty], 116 + k * 22, 92 - ((frame_t / 8 + k) % 2), 0);
            k++;
        }
        if (G.players == 1 && G.scen == PH_RANDOM) { snprintf(b, sizeof b, "STREAK %d", sv.streak); tiny_center(b, 160, 116, C_CREAM); }
        if (G.players == 1 && G.scen == PH_CUSTOM) {
            snprintf(b, sizeof b, "YOUR LIST IN %d NIGHTS  " GLYPH_DOT "  FASTEST %d", ph_me(&G)->night, sv.cfastest);
            tiny_center(b, 160, 116, C_CREAM);
        }
    } else if (G.scen == PH_ENDLESS) {
        /* OPEN ALL NIGHT is over when its clock runs out */
        tiny_center("THREE SHUTDOWNS. THE NEIGHBOURS HAVE HAD ENOUGH.", 160, 88, C_GREY);
        snprintf(b, sizeof b, "%d STAR PART%s IN %d NIGHTS", G.score, G.score == 1 ? "Y" : "IES", G.nights);
        tiny_center(b, 160, 96, C_CREAM);
        snprintf(b, sizeof b, "TOP FAME %d", G.top_pop);
        tiny_center(b, 160, 105, C_GREY);
        if (G.score > endless_prev_best) tiny_center("A NEW BEST!", 160, 115, (frame_t / 8) % 2 ? C_YELLOW : C_WHITE);
        else { snprintf(b, sizeof b, "BEST %d ON %s", sv.night_best[G.base % PH_BASES], base_name(G.base)); tiny_center(b, 160, 115, C_SLATE); }
    } else {
        tiny_center(G.players == 2 ? "BOTH HOUSES RAN OUT OF NIGHTS." : "25 NIGHTS AND NEVER FOUR STARS AT ONCE.", 160, 100, C_GREY);
        if (G.scen == PH_RANDOM && G.players == 1) tiny_center("THE STREAK STARTS AGAIN.", 160, 110, C_SLATE);
    }
    if (state_t > 90 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 146, C_WHITE);
}

static bool sheet_mode;

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    for (int i = 0; i < G_COUNT; i++) {
        int x = 4 + (i % 12) * 26, y = 4 + (i / 12) * 26;
        spr_draw(&ph_spr[i], x, y, 0);
    }
    spr_draw(&ph_spr[PS_DOOR], 4, 120, 0);
    spr_draw(&ph_spr[PS_LAMP], 30, 120, 0);
    spr_draw(&ph_spr[PS_POLICE], 50, 120, 0);
    spr_draw(&ph_spr[PS_FIRE], 80, 120, 0);
}

static void oh_draw(void) {
    memset(&seen, 0, sizeof seen);
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_MODE: draw_mode(); break;
    case S_SCEN: draw_scen(); break;
    case S_INTRO: draw_intro(); break;
    case S_TURN: draw_turn(); break;
    case S_PARTY: case S_TARGET: case S_BAN: draw_party_screen(); break;
    case S_FETCH: draw_fetch(); break;
    case S_CONFIRM: draw_confirm(); break;
    case S_BUST: draw_bust(); break;
    case S_RESULT: draw_result(); break;
    case S_SHOP: draw_shop(); break;
    case S_WIN: case S_LOSE: draw_end(); break;
    case S_LIST: draw_list(); break;
    case S_CODE: draw_code(); break;
    case S_RPICK: draw_rpick(); break;
    case S_CUSTOM: draw_custom(); break;
    case S_LEGEND: draw_legend(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void oh_load(void) {
    ph_art_load();
    ph_audio_load();
}

static bool keep_old_save;         /* tests: the planted old save must survive the quit */

static void oh_start(void) {
    load_save();
    keep_old_save = false;
    memset(&G, 0, sizeof G);
    state = S_TITLE;
    state_t = 0;
    sheet_mode = false;
    game_set_pausable(false);
    game_pause_items(1, PAUSE_ITEMS, pause_pick);
    music_play(PH_MUS_TITLE);
}

static void oh_quit(void) {
    if (keep_old_save) return; /* until the next start reads it */
    if (G.players == 1 && sv.has_run && !G.done && G.pl[0].ncards) sv.run = G;
    save_now();
}

static void oh_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_NIGHT);
    for (int i = 0; i < 16; i++) gfx_pset(x + (i * 37) % w, y + (i * 13) % 24, (t / 30 + i) % 4 ? C_GREY : C_WHITE);
    gfx_rect(x, y + 50, w, 14, C_NAVY);
    gfx_rect(x + 20, y + 24, 102, 30, C_LIGHT);
    gfx_rect(x + 16, y + 20, 110, 5, C_WHITE);
    for (int i = 0; i < 4; i++) {
        bool lit = (t / 30 + i) % 5 != 0;
        gfx_rect(x + 26 + i * 24, y + 30, 12, 16, lit ? C_YELLOW : C_DUSK);
    }
    static const int GUESTS[4] = {G_PILOT, G_PUNK, G_GRANNY, G_GOAT};
    for (int i = 0; i < 4; i++) spr_draw(&ph_spr[GUESTS[i]], x + 24 + i * 24, y + 30 - ((t / 10 + i) % 2), 0);
    for (int i = 0; i < 8; i++) gfx_rect(x + 18 + i * 14, y + 19 + (i % 2) * 2, 3, 4, (t / 15 + i) % 2 ? C_ORANGE : C_AMBER);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_text("OPEN HOUSE", x + 4, y + 3, 1, grad, 3, C_INK, -1);
}

static int oh_query(const char *key, int *out) {
    PhPlayer *p = ph_me(&G);
    if (!strcmp(key, "state")) { *out = state; return 1; }
    /* what a frame draws (the badges, the red notices, the tags) */
    if (!strncmp(key, "seen_", 5)) {
        oh_draw();
        const char *k = key + 5;
        if (!strcmp(k, "notice")) *out = seen.notice;
        else if (!strcmp(k, "culprits")) *out = seen.culprits;
        else if (!strcmp(k, "badges")) *out = seen.badges;
        else if (!strcmp(k, "ban_slot")) *out = seen.ban_slot;
        else if (!strcmp(k, "ban_house")) *out = seen.ban_house;
        else if (!strcmp(k, "ban_shop")) *out = seen.ban_shop;
        else if (!strcmp(k, "ban_book")) *out = seen.ban_book;
        else if (!strcmp(k, "pick_rows")) *out = seen.pick_rows;
        else if (!strcmp(k, "inf")) *out = seen.inf;
        else if (!strcmp(k, "door")) *out = seen.door;
        else if (!strcmp(k, "legend")) *out = seen.legend;
        else if (!strcmp(k, "crowns")) *out = seen.crowns;
        else return 0;
        return 1;
    }
    /* a house guest's badges: bpop0, bcash0, btrouble0, bstar0, babil0, bused0, bready0 */
    if (key[0] == 'b' && (!strncmp(key, "bpop", 4) || !strncmp(key, "bcash", 5) || !strncmp(key, "btrouble", 8) ||
                          !strncmp(key, "bstar", 5) || !strncmp(key, "babil", 5) || !strncmp(key, "bused", 5) ||
                          !strncmp(key, "bready", 6))) {
        const char *d = key;
        while (*d && (*d < '0' || *d > '9')) d++;
        int i = atoi(d);
        if (i < 0 || i >= G.party.n) { *out = -99; return 1; }
        Badge b = badge_slot(i);
        *out = !strncmp(key, "bpop", 4) ? b.pop : !strncmp(key, "bcash", 5) ? b.cash : !strncmp(key, "btrouble", 8) ? b.trouble
             : !strncmp(key, "bstar", 5) ? b.star : !strncmp(key, "babil", 5) ? b.abil : !strncmp(key, "bused", 5) ? b.used : b.ready;
        return 1;
    }
    if (!strncmp(key, "abil", 4)) { *out = ph_ability_icon(atoi(key + 4) % G_COUNT); return 1; }
    if (!strcmp(key, "notice")) { *out = state == S_BUST ? (G.party.over == PO_POLICE ? 2 : 1) : 0; return 1; }
    if (!strncmp(key, "culprit", 7)) { int i = atoi(key + 7); *out = i < G.party.n && culprit(i); return 1; }
    if (!strcmp(key, "overflow")) {
        int oc = G.party.overflow - 1;
        *out = G.party.over == PO_FIRE && oc >= 0 && oc < p->ncards ? p->card[oc].type : -1;
        return 1;
    }
    if (!strcmp(key, "fireby")) { *out = fire_by; return 1; }
    if (!strcmp(key, "bantag")) { *out = ban_tag[G.turn & 1]; return 1; }
    if (!strcmp(key, "bantype")) { *out = banned_type(); return 1; }
    if (!strcmp(key, "peektype")) { *out = G.party.peek >= 0 ? p->card[G.party.peek].type : -1; return 1; }
    if (!strncmp(key, "pick_", 5)) {
        /* the fetch list's row under the cursor, as drawn */
        if (fcur >= nfetch) { *out = -99; return 1; }
        Badge b = badge_type(fetch_list[fcur]);
        const char *k = key + 5;
        if (!strcmp(k, "cost")) *out = PH_GUESTS[fetch_list[fcur]].cost;
        else if (!strcmp(k, "pop")) *out = b.pop;
        else if (!strcmp(k, "cash")) *out = b.cash;
        else if (!strcmp(k, "trouble")) *out = b.trouble;
        else if (!strcmp(k, "star")) *out = b.star;
        else if (!strcmp(k, "abil")) *out = b.abil;
        else return 0;
        return 1;
    }
    if (!strncmp(key, "beaten", 6)) { *out = beaten(atoi(key + 6) % SCEN_ROWS); return 1; }
    if (!strcmp(key, "legend_back")) { *out = legend_back; return 1; }
    if (!strcmp(key, "msgred")) { *out = msg_t > 0 && msg_red; return 1; }
    /* the custom list and its editor */
    if (!strcmp(key, "ed_cur")) { *out = ed_cur; return 1; }
    if (!strcmp(key, "ed_warn")) { *out = ed_warn; return 1; }
    if (!strncmp(key, "cin", 3)) { *out = custom_in(atoi(key + 3) % G_COUNT); return 1; }
    if (!strcmp(key, "cguests")) { *out = custom_count(false); return 1; }
    if (!strcmp(key, "cstars")) { *out = custom_count(true); return 1; }
    if (!strcmp(key, "cendless")) { *out = sv.cendless; return 1; }
    if (!strcmp(key, "cwins")) { *out = sv.cwins; return 1; }
    if (!strcmp(key, "cfastest")) { *out = sv.cfastest; return 1; }
    if (!strcmp(key, "poolcustom")) {
        /* every guest the shop sells is a neighbour, a cousin or one picked */
        int ok = 1;
        for (int i = 0; i < G.npool; i++) ok &= custom_in(G.pool[i]) && G.pool[i] != G_ROWDY;
        *out = ok;
        return 1;
    }
    if (!strcmp(key, "scen")) { *out = G.scen + 1; return 1; }
    if (!strcmp(key, "night")) { *out = p->night; return 1; }
    if (!strcmp(key, "pop")) { *out = p->pop; return 1; }
    if (!strcmp(key, "cash")) { *out = p->cash; return 1; }
    if (!strcmp(key, "cap")) { *out = p->cap; return 1; }
    if (!strcmp(key, "n")) { *out = G.party.n; return 1; }
    if (!strcmp(key, "over")) { *out = G.party.over; return 1; }
    if (!strcmp(key, "trouble")) { *out = ph_trouble(&G); return 1; }
    if (!strcmp(key, "stars")) { *out = ph_stars(&G); return 1; }
    if (!strcmp(key, "cur")) { *out = cur; return 1; }
    if (!strcmp(key, "cards")) { *out = p->ncards; return 1; }
    if (!strcmp(key, "peek")) { *out = G.party.peek; return 1; }
    if (!strcmp(key, "banned")) { *out = p->banned; return 1; }
    if (!strcmp(key, "turn")) { *out = G.turn + 1; return 1; }
    if (!strcmp(key, "winner")) { *out = G.winner; return 1; }
    if (!strcmp(key, "done")) { *out = G.done; return 1; }
    if (!strcmp(key, "won")) { *out = sv.won; return 1; }
    if (!strcmp(key, "streak")) { *out = sv.streak; return 1; }
    if (!strcmp(key, "best_streak")) { *out = sv.best_streak; return 1; }
    if (!strcmp(key, "has_run")) { *out = sv.has_run; return 1; }
    if (!strcmp(key, "end_pop")) { *out = G.party.end_pop; return 1; }
    if (!strcmp(key, "end_cash")) { *out = G.party.end_cash; return 1; }
    if (!strcmp(key, "penalty")) { *out = G.party.penalty; return 1; }
    if (!strcmp(key, "got_pop")) { *out = G.party.got_pop; return 1; }
    if (!strcmp(key, "got_cash")) { *out = G.party.got_cash; return 1; }
    if (!strcmp(key, "collected")) { *out = G.party.got_pop + G.party.got_cash; return 1; }
    if (!strncmp(key, "wout", 4)) { *out = ph_count_type(&G, atoi(key + 4), W_OUT); return 1; }
    if (!strncmp(key, "wpool", 5)) { *out = ph_count_type(&G, atoi(key + 5), W_POOL); return 1; }
    if (!strcmp(key, "poolstars")) {
        int n = 0;
        for (int i = 0; i < G.npool; i++) n += (PH_GUESTS[G.pool[i]].traits & T_STAR) != 0;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "npool")) { *out = G.npool; return 1; }
    if (!strcmp(key, "code")) { *out = run_code; return 1; }
    if (!strcmp(key, "typed")) { *out = code_typed(); return 1; }
    if (!strcmp(key, "shopcur")) { *out = shop_cur; return 1; }
    if (!strcmp(key, "shoptop")) { *out = shop_top; return 1; }
    if (!strcmp(key, "osel")) { *out = menu_sel; return 1; }
    if (!strcmp(key, "scen_sel")) { *out = scen_sel; return 1; }
    if (!strcmp(key, "goal")) { *out = ph_goal(&G); return 1; }
    if (!strcmp(key, "score")) { *out = G.score; return 1; }
    if (!strcmp(key, "limit")) { *out = ph_night_limit(&G); return 1; }
    if (!strcmp(key, "strikes")) { *out = G.strikes; return 1; }
    if (!strcmp(key, "nights")) { *out = G.nights; return 1; }
    if (!strcmp(key, "tallywhy")) { *out = T.why; return 1; }
    if (!strcmp(key, "fresh")) { *out = shop_fresh; return 1; }
    if (!strcmp(key, "fetchsel")) { *out = fcur < nfetch ? fetch_list[fcur] : -1; return 1; }
    if (!strncmp(key, "gcost", 5)) { *out = PH_GUESTS[atoi(key + 5) % G_COUNT].cost; return 1; }
    if (!strncmp(key, "gpop", 4)) { *out = PH_GUESTS[atoi(key + 4) % G_COUNT].pop; return 1; }
    if (!strncmp(key, "gcash", 5)) { *out = PH_GUESTS[atoi(key + 5) % G_COUNT].cash; return 1; }
    if (!strncmp(key, "inpool", 6)) {
        int ty = atoi(key + 6), n = 0;
        for (int i = 0; i < G.npool; i++) n += G.pool[i] == ty;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "sells", 5)) { *out = ph_list_sells(G.base, atoi(key + 5)); return 1; }
    if (!strcmp(key, "jinx")) { *out = G.party.jinx; return 1; }
    if (!strcmp(key, "top_pop")) { *out = G.top_pop; return 1; }
    if (!strcmp(key, "star_party")) { *out = G.star_party; return 1; }
    if (!strcmp(key, "night_best")) { *out = sv.night_best[G.base % PH_BASES]; return 1; }
    if (!strcmp(key, "night_fame")) { *out = sv.night_fame[G.base % PH_BASES]; return 1; }
    if (!strncmp(key, "nbest", 5)) { *out = sv.night_best[atoi(key + 5) % PH_BASES]; return 1; }
    if (!strncmp(key, "nfame", 5)) { *out = sv.night_fame[atoi(key + 5) % PH_BASES]; return 1; }
    if (!strcmp(key, "base")) { *out = G.base; return 1; }
    if (!strcmp(key, "scen_endless")) { *out = scen_endless; return 1; }
    if (!strcmp(key, "tally")) { *out = T.phase; return 1; }
    if (!strcmp(key, "tallypop")) { *out = tally_pop_shown(); return 1; }
    if (!strcmp(key, "tallycash")) { *out = T.cash; return 1; }
    if (!strcmp(key, "tallygot")) { *out = T.got_pop; return 1; }
    if (!strcmp(key, "tallyframes")) { *out = T.frames; return 1; }
    if (!strcmp(key, "tallyslot")) { *out = T.slot; return 1; }
    if (!strcmp(key, "shoptype")) { *out = shop_type(); return 1; }
    if (!strcmp(key, "players")) { *out = G.players; return 1; }
    if (!strncmp(key, "slot", 4)) { int i = atoi(key + 4); *out = i < G.party.n ? p->card[G.party.house[i]].type : -1; return 1; }
    if (!strncmp(key, "vpop", 4)) { int i = atoi(key + 4); *out = i < G.party.n ? ph_value_pop(&G, G.party.house[i]) : 0; return 1; }
    if (!strncmp(key, "vcash", 5)) { int i = atoi(key + 5); *out = i < G.party.n ? ph_value_cash(&G, G.party.house[i]) : 0; return 1; }
    if (!strncmp(key, "canact", 6)) { *out = ph_can_act(&G, atoi(key + 6)); return 1; }
    if (!strncmp(key, "have", 4)) {
        int ty = atoi(key + 4), n = 0;
        for (int i = 0; i < p->ncards; i++) n += p->card[i].type == ty;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "bought", 6)) { *out = G.bought[atoi(key + 6) % G_COUNT]; return 1; }
    if (!strncmp(key, "pool", 4)) { int i = atoi(key + 4); *out = i < G.npool ? G.pool[i] : -1; return 1; }
    if (!strncmp(key, "named", 5)) {
        int n = 0;
        for (int i = 0; i < p->ncards; i++) n += p->card[i].name != 0;
        *out = n;
        return 1;
    }
    return 0;
}

/* cheats build exact parties for the tests: "door TYPE TYPE ..." lets
 * those guests in, one knock each, in that order */
static int take_card_of(int ty) {
    PhPlayer *p = ph_me(&G);
    for (int i = 0; i < p->ncards; i++)
        if (p->card[i].type == ty && G.party.where[i] == W_POOL && i != G.party.peek) return i;
    return -1;
}

static int oh_cheat(const char *cmd) {
    int a, b;
    PhPlayer *p = ph_me(&G);
    if (sscanf(cmd, "new %d %d", &a, &b) == 2) {
        ph_new(&G, a - 1, b, 1234 + (uint64_t)a);
        clear_bans();
        endless_prev_best = sv.night_best[G.base % PH_BASES];
        if (b == 1) { sv.has_run = 1; }
        state = S_PARTY; state_t = 0; cur = CUR_DOOR;
        game_set_pausable(true);
        return 1;
    }
    if (sscanf(cmd, "give %d %d", &a, &b) == 2) {
        for (int i = 0; i < b && p->ncards < PH_MAX_CARDS; i++) {
            memset(&p->card[p->ncards], 0, sizeof p->card[0]);
            p->card[p->ncards++].type = (uint8_t)a;
        }
        return 1;
    }
    int c3;
    if (sscanf(cmd, "card %d %d %d", &a, &b, &c3) == 3) {
        /* every card of type a: visits so far b, the tailor's bonus c3 */
        for (int i = 0; i < p->ncards; i++)
            if (p->card[i].type == a) { p->card[i].visits = (uint8_t)b; p->card[i].bonus = (int8_t)c3; }
        return 1;
    }
    if (sscanf(cmd, "pop %d", &a) == 1) { p->pop = (int16_t)a; return 1; }
    if (sscanf(cmd, "cash %d", &a) == 1) { p->cash = (int16_t)a; return 1; }
    if (sscanf(cmd, "cap %d", &a) == 1) { p->cap = (uint8_t)a; return 1; }
    if (sscanf(cmd, "night %d", &a) == 1) { p->night = (uint8_t)a; return 1; }
    if (sscanf(cmd, "nights %d", &a) == 1) { G.nights = (uint16_t)a; p->night = (uint8_t)imin(a, 255); return 1; }
    if (!strncmp(cmd, "door ", 5)) {
        /* let in exactly these guest types, one knock each */
        const char *s = cmd + 5;
        while (*s) {
            while (*s == ' ') s++;
            if (!*s) break;
            int ty = atoi(s);
            while (*s && *s != ' ') s++;
            int c = take_card_of(ty);
            if (c < 0 || G.party.over) continue;
            G.party.peek = (int16_t)c;
            int t = ph_trouble(&G);
            if (ph_open_door(&G)) { note_arrivals(); after_change(t); }
            if (G.party.over) break;
        }
        return 1;
    }
    if (sscanf(cmd, "act %d %d", &a, &b) == 2) {
        int t = ph_trouble(&G);
        if (ph_act(&G, a, b)) { if (G.party.nlast) note_arrivals(); if (state == S_PARTY || state == S_TARGET) after_change(t); }
        return 1;
    }
    if (sscanf(cmd, "shop %d", &a) == 1) { ph_buy(&G, a); return 1; }
    if (sscanf(cmd, "decide %d", &a) == 1) {
        int t = ph_trouble(&G);
        if (ph_peek_decide(&G, a != 0) && a) { note_arrivals(); after_change(t); }
        return 1;
    }
    if (sscanf(cmd, "away %d", &a) == 1) {
        /* every guest of that type still in the guest book stays home tonight */
        for (int i = 0; i < p->ncards; i++)
            if (p->card[i].type == a && G.party.where[i] == W_POOL && i != G.party.peek) G.party.where[i] = W_OUT;
        return 1;
    }
    if (!strcmp(cmd, "expand")) { ph_expand(&G); return 1; }
    if (!strcmp(cmd, "end")) { start_tally(WHY_HAND); return 1; }
    if (sscanf(cmd, "oldsave3 %d", &a) == 1) {
        /* a save in the third layout (before the custom list), with the run
         * in progress (checked before "oldsave2" and "oldsave") */
        static SaveV3 o;
        memset(&o, 0, sizeof o);
        o.magic = SAVE_MAGIC_V3;
        o.won = (uint8_t)a;
        o.streak = 2;
        o.best_streak = 5;
        o.has_run = 1;
        o.parties = 11;
        o.night_best[PH_ENDLESS] = 4;
        o.night_fame[PH_ENDLESS] = 77;
        o.run.scen = G.scen;
        o.run.npool = (uint8_t)imin(G.npool, PH_POOL_MAX_V3);
        memcpy(o.run.pool, G.pool, sizeof o.run.pool);
        memcpy(o.run.bought, G.bought, sizeof o.run.bought);
        o.run.players = G.players;
        o.run.turn = G.turn;
        o.run.rng = G.rng;
        memcpy(o.run.pl, G.pl, sizeof o.run.pl);
        o.run.party = G.party;
        o.run.goal = G.goal;
        o.run.nights = G.nights;
        o.run.base = G.base;
        game_save_write(game_current_index(), &o, (int)sizeof o);
        keep_old_save = true;
        return 1;
    }
    if (sscanf(cmd, "oldsave2 %d", &a) == 1) {
        /* a save in the second layout (before the owner's two guests), with
         * the run in progress (checked before "oldsave", which would match) */
        static SaveV2 o;
        memset(&o, 0, sizeof o);
        o.magic = SAVE_MAGIC_V2;
        o.won = (uint8_t)a;
        o.streak = 1;
        o.best_streak = 4;
        o.has_run = 1;
        o.parties = 9;
        o.night_best[2] = 3;
        o.run.scen = G.scen;
        o.run.npool = G.npool;
        memcpy(o.run.pool, G.pool, sizeof o.run.pool);
        memcpy(o.run.bought, G.bought, sizeof o.run.bought);
        o.run.players = G.players;
        o.run.turn = G.turn;
        o.run.winner = G.winner;
        o.run.done = G.done;
        o.run.rng = G.rng;
        memcpy(o.run.pl, G.pl, sizeof o.run.pl);
        o.run.party = G.party;
        o.run.goal = G.goal;
        o.run.nights = G.nights;
        o.run.base = G.base;
        game_save_write(game_current_index(), &o, (int)sizeof o);
        keep_old_save = true;
        return 1;
    }
    if (sscanf(cmd, "oldsave %d", &a) == 1) {
        /* a save in the first layout (before OPEN ALL NIGHT), with the run in progress */
        static SaveV1 o;
        memset(&o, 0, sizeof o);
        o.magic = SAVE_MAGIC_V1;
        o.won = (uint8_t)a;
        o.streak = 2;
        o.best_streak = 3;
        o.has_run = 1;
        o.parties = 7;
        o.run.scen = G.scen;
        o.run.npool = (uint8_t)imin(G.npool, PH_POOL_MAX_V1);
        memcpy(o.run.pool, G.pool, PH_POOL_MAX_V1);
        memcpy(o.run.bought, G.bought, sizeof o.run.bought);
        o.run.players = G.players;
        o.run.turn = G.turn;
        o.run.rng = G.rng;
        memcpy(o.run.pl, G.pl, sizeof o.run.pl);
        o.run.party = G.party;
        game_save_write(game_current_index(), &o, (int)sizeof o);
        keep_old_save = true;
        return 1;
    }
    if (!strcmp(cmd, "next")) { after_party(); return 1; }
    if (!strcmp(cmd, "leave")) { leave_shop(); return 1; }
    if (sscanf(cmd, "ban %d", &a) == 1) { ph_ban(&G, G.party.house[a]); after_party(); return 1; }
    if (sscanf(cmd, "won %d", &a) == 1) { sv.won = (uint8_t)a; save_now(); return 1; }
    if (sscanf(cmd, "streak %d", &a) == 1) { sv.streak = (uint8_t)a; if (a > sv.best_streak) sv.best_streak = (uint8_t)a; save_now(); return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    if (!strcmp(cmd, "menu")) { state = S_SCEN; state_t = 0; mode_players = 1; scen_endless = false; return 1; }
    if (!strcmp(cmd, "custom")) { mode_players = 1; scen_endless = false; open_custom(false); return 1; }
    return 0;
}

const GameDef GAME_OPENHOUSE = {
    "openhouse",
    "OPEN HOUSE",
    "1986",
    "STRATEGY",
    "THROW 25 SUMMER PARTIES AND GET FOUR STAR GUESTS UNDER ONE ROOF. OR STAY OPEN ALL NIGHT.",
    {"WIN A GUEST LIST", "WIN ALL FIVE GUEST LISTS", "WIN 5 RANDOM LISTS IN A ROW"},
    "D-PAD\tMOVE THE CURSOR\n"
    GLYPH_A "\tOPEN THE DOOR / USE /\n\tCHOOSE / BUY\n"
    GLYPH_B "\tBACK / END THE PARTY /\n\tTO NEXT PARTY (SHOP)\n"
    "START\tPAUSE / ICON GUIDE",
    C_WINE, C_YELLOW,
    oh_load, oh_start, oh_update, oh_draw, oh_quit, oh_label, oh_query, oh_cheat,
    "PARTY HOUSE", 25,
};
