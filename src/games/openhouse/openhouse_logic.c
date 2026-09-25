/* OPEN HOUSE - the rules: guests, scenarios, the party, scoring and the
 * shop. Pure C, no shell dependencies (build with -DPH_NO_SHELL for tools).
 * The numbers are Party House's, from the design doc's table. */
#include "openhouse.h"

#define FETCH "BRING IN ANY GUEST YOU\nCHOOSE (NEEDS ROOM)."
#define BOOT "SEND A GUEST HOME TILL\nTHE NEXT PARTY."
#define PEEK "SEE WHO'S AT THE DOOR,\nTHEN LET THEM IN OR NOT."
#define SHUF "EVERYONE OUT! THE ROLODEX\nIS SHUFFLED. USED ACTIONS\nSTAY USED."
#define PEACE "CANCELS ONE TROUBLE!"
#define TRBL "TROUBLE!"

const PhGuest PH_GUESTS[G_COUNT] = {
    /* name            cost pop cash action     traits */
    {"OLD NEIGHBOUR",   2,  1,  0, A_NONE,      0, "", "BRINGS HUMMUS. ALWAYS\nHUMMUS."},
    {"RICH COUSIN",     3,  0,  1, A_NONE,      0, "", "PAYS FOR THE ICE."},
    {"ROWDY MATE",     -1,  2,  0, A_NONE,      T_TROUBLE, TRBL, "SINGS ON THE ROOF\nAT THREE IN THE MORNING."},
    {"CABBIE",          3,  0,  0, A_FETCH,     0, FETCH, "KNOWS EVERY ADDRESS\nIN TOWN."},
    {"SLEUTH",          4,  2, -1, A_FETCH,     0, FETCH, "FINDS ANYONE. FOR A FEE."},
    {"SURFER",          4,  1,  0, A_NONE,      T_PEACE, PEACE, "CALM AS A FLAT SEA."},
    {"KITTEN",          7,  2,  0, A_NONE,      T_PEACE, PEACE, "EVERYONE STOPS TO PET IT."},
    {"BOUNCER",         4,  0,  0, A_BOOT,      0, BOOT, "CAN EVEN THROW HIMSELF\nOUT."},
    {"STRONGMAN",       9,  2,  0, A_BOOT,      0, BOOT, "LIFTS THE SOFA. WITH\nGUESTS ON IT."},
    {"PARROT",          4,  2,  0, A_PEEK,      0, PEEK, "SQUAWKS THE NAME OF\nWHOEVER KNOCKS."},
    {"DOORMAN",         8,  0,  2, A_PEEK,      0, PEEK, "CHECKS THE LIST TWICE."},
    {"FIREWORKER",      5,  2,  0, A_RESHUFFLE, 0, SHUF, "FIREWORKS ON THE BEACH!"},
    {"TOUR GUIDE",      6,  1,  1, A_RESHUFFLE, 0, SHUF, "A MIDNIGHT WALK TO THE\nLIGHTHOUSE."},
    {"DRUMMER",         7,  0,  0, A_NONE,      T_DRUM, "DRUMMERS PAY 1, 4, 9 OR\n16 TOGETHER.", "EVERY BEAT DRAWS\nANOTHER DANCER."},
    {"SOCIALITE",       5,  3,  0, A_NONE,      T_BRING1, "BRINGS ONE RANDOM GUEST\nALONG.", "NEVER ARRIVES ALONE."},
    {"POP IDOL",       11,  2,  3, A_NONE,      T_BRING2, "BRINGS TWO RANDOM GUESTS\nALONG.", "HAS AN ENTOURAGE."},
    {"COAT CHECK",      4, -1,  2, A_NONE,      0, "", "TIPS ONLY, THANK YOU."},
    {"STORYTELLER",     5,  0, -1, A_NONE,      T_STORY, "+5 POPULARITY IF THE\nHOUSE ENDS UP FULL.", "NEEDS A PACKED ROOM."},
    {"PAPARAZZO",       5,  1, -1, A_PHOTO,     0, "COLLECT ANOTHER GUEST'S\nPAY RIGHT NOW. THEY PAY\nAGAIN AT THE END.", "FLASH! YOUR GOOD SIDE."},
    {"PASTRY CHEF",     5,  4, -1, A_NONE,      0, "", "BAKLAVA FOR EVERYONE."},
    {"MERCHANT",        9,  0,  3, A_NONE,      0, "", "SELLS JASMINE AT THE\nDOOR."},
    {"GRANNY",          5,  1,  0, A_NONE,      T_GRANNY, "+1 FOR EACH OLD\nNEIGHBOUR HERE.", "KNOWS EVERY NEIGHBOUR\nBY NAME."},
    {"BOOKWORM",        4,  1,  0, A_NONE,      T_BOOK, "+1 FOR EACH EMPTY SPACE\nIN THE HOUSE.", "LIKES A QUIET CORNER."},
    {"TAILOR",          7,  0, -1, A_STYLE,     0, "A GUEST GETS +1\nPOPULARITY FOR GOOD\n(UNDER 9 ONLY).", "A STITCH HERE, A TUCK\nTHERE."},
    {"BARISTA",        11,  1,  0, A_NONE,      T_BARISTA, "+2 CASH FOR EACH\nTROUBLE! HERE.", "ROWDY GUESTS ARE\nTHIRSTY."},
    {"POET",            8,  1,  0, A_NONE,      T_POET, "+2 POPULARITY FOR EACH\nTROUBLE! HERE.", "WRITES ABOUT THE CHAOS."},
    {"UPSTART",        12,  0,  0, A_NONE,      T_UPSTART, "+1 POPULARITY EVERY TIME\nTHEY COME, UP TO 9.", "CLIMBING THE GUEST LIST."},
    {"BAND LEADER",     5,  1,  0, A_CHEER,     0, "EVERYONE ELSE CAN USE\nTHEIR ACTION AGAIN (NOT\nOTHER BAND LEADERS).", "ONE MORE TIME!"},
    {"USHER",           5,  1,  0, A_GREET,     0, "LET IN THE NEXT GUEST\nAND COLLECT THEIR PAY\nNOW (AND ANY THEY BRING).", "WELCOME, COME IN!"},
    {"FORTUNE TELLER",  5,  1,  0, A_MAGIC,     0, "SWAP A GUEST FOR A STAR\nFROM THE ROLODEX, OR A\nSTAR FOR A GUEST.", "SEES A STAR IN YOUR\nFUTURE."},
    {"MATCHMAKER",      8,  1,  0, A_CUPID,     0, "SEND HOME TWO GUESTS\nSTANDING SIDE BY SIDE.", "THEY LEFT TOGETHER.\nHOW LOVELY."},
    {"OLD SAGE",        7,  0,  0, A_CALM,      0, "TAKES THE TROUBLE! OUT\nOF EVERYONE HERE.", "A GOOD TALKING-TO."},
    {"MOON CHILD",      5,  4,  0, A_NONE,      T_MOON, "TROUBLE! EVERY OTHER\nVISIT, THE FIRST TOO.", "WILD WHEN THE MOON\nIS FULL."},
    {"GOAT",            3,  4,  0, A_NONE,      T_TROUBLE, TRBL, "HOW DID A GOAT GET IN?"},
    {"RAI SINGER",      5,  3,  2, A_NONE,      T_TROUBLE, TRBL, "TOO LOUD. TOO GOOD."},
    {"SMUGGLER",        6,  0,  4, A_NONE,      T_TROUBLE, TRBL, "DON'T ASK WHAT'S IN\nTHE BOX."},
    {"CARD SHARK",      7,  2,  3, A_NONE,      T_TROUBLE, TRBL, "ANYONE FOR A LITTLE\nGAME?"},
    {"SAUCER PILOT",   40,  0,  0, A_NONE,      T_STAR, "STAR GUEST.", "PARKED ON THE ROOF."},
    {"SULTAN",         50,  0,  3, A_NONE,      T_STAR, "STAR GUEST.", "TIPS IN GOLD."},
    {"WISH FISH",      55,  0,  0, A_FETCH,     T_STAR, "STAR GUEST. " FETCH, "ONE WISH A NIGHT."},
    {"SEA SERPENT",    30,  0, -3, A_NONE,      T_STAR, "STAR GUEST.", "EATS THE WHOLE BUFFET."},
    {"CYCLOPS",        25,  0,  0, A_NONE,      T_STAR | T_TROUBLE, "STAR GUEST. " TRBL, "ONE EYE, NO MANNERS."},
    {"PHOENIX",        35,  0,  0, A_NONE,      T_STAR | T_BRING1, "STAR GUEST. BRINGS ONE\nRANDOM GUEST ALONG.", "RISES WITH A FRIEND."},
    {"SHADOW",         45,  0,  0, A_BOOT,      T_STAR, "STAR GUEST. " BOOT, "LURKS IN THE CORNERS."},
    {"SPHINX",         45,  0,  0, A_NONE,      T_STAR | T_PEACE, "STAR GUEST. " PEACE, "ASKS RIDDLES. KEEPS\nTHE PEACE."},
    {"CHAMPION",       50,  3,  0, A_NONE,      T_STAR, "STAR GUEST.", "SIGNS AUTOGRAPHS."},
};

const PhScenario PH_SCEN[PH_SCENARIOS] = {
    {"SAUCER NIGHT", "ONE STAR GUEST, FOUR TIMES\nOVER. THE ROOF IS READY.", 13,
     {G_CABBIE, G_GOAT, G_BOUNCER, G_COATCHECK, G_PARROT, G_SURFER, G_RAI, G_STORYTELLER, G_PASTRY,
      G_SOCIALITE, G_DRUMMER, G_MERCHANT, G_PILOT}},
    {"HIGH TIDE, LOW TIDE", "A CHEAP STAR THAT BRINGS\nFRIENDS, A DEAR ONE THAT\nDOESN'T.", 13,
     {G_SLEUTH, G_BOOKWORM, G_FIREWORKER, G_GRANNY, G_SMUGGLER, G_KITTEN, G_CARDSHARK, G_DOORMAN, G_POET,
      G_STRONGMAN, G_UPSTART, G_PHOENIX, G_CHAMPION}},
    {"THREE WISHES", "A WISH-GRANTING FISH AND\nA GIANT WITH NO MANNERS.", 13,
     {G_GOAT, G_SURFER, G_PAPARAZZO, G_BANDLEADER, G_RAI, G_GUIDE, G_TAILOR, G_SAGE, G_STRONGMAN, G_IDOL,
      G_BARISTA, G_WISHFISH, G_CYCLOPS}},
    {"THE ACCOUNTS", "ONE STAR EATS YOUR CASH,\nTHE OTHER PAYS IN GOLD.", 13,
     {G_SLEUTH, G_COATCHECK, G_BOUNCER, G_PAPARAZZO, G_STORYTELLER, G_PASTRY, G_SMUGGLER, G_GUIDE, G_TAILOR,
      G_KITTEN, G_DOORMAN, G_SERPENT, G_SULTAN}},
    {"MIDSUMMER MAGIC", "NOBODY HERE CALMS THE\nPARTY BUT THE SPHINX.", 13,
     {G_BOOKWORM, G_PARROT, G_MOONCHILD, G_USHER, G_FORTUNE, G_CARDSHARK, G_DRUMMER, G_MATCHMAKER,
      G_MERCHANT, G_IDOL, G_UPSTART, G_SHADOW, G_SPHINX}},
};

static int rnd(PhGame *g, int n) { return n <= 1 ? 0 : (int)(rng_next(&g->rng) % (uint32_t)n); }
static bool is_star(int type) { return (PH_GUESTS[type].traits & T_STAR) != 0; }

/* ------------------------------------------------------------------ */
/* setting up                                                           */

static void add_card(PhPlayer *p, int type) {
    if (p->ncards >= PH_MAX_CARDS) return;
    PhCard *c = &p->card[p->ncards++];
    memset(c, 0, sizeof *c);
    c->type = (uint8_t)type;
}

static int pool_cmp(const void *a, const void *b) {
    int x = *(const uint8_t *)a, y = *(const uint8_t *)b;
    int sx = is_star(x), sy = is_star(y);
    if (sx != sy) return sx - sy;
    if (PH_GUESTS[x].cost != PH_GUESTS[y].cost) return PH_GUESTS[x].cost - PH_GUESTS[y].cost;
    return x - y;
}

void ph_new(PhGame *g, int scen, int players, uint64_t seed) {
    memset(g, 0, sizeof *g);
    g->scen = (uint8_t)scen;
    g->players = (uint8_t)(players == 2 ? 2 : 1);
    rng_seed(&g->rng, seed);
    g->pool[g->npool++] = G_NEIGHBOUR;
    g->pool[g->npool++] = G_COUSIN;
    if (scen < PH_SCENARIOS) {
        for (int i = 0; i < PH_SCEN[scen].n; i++) g->pool[g->npool++] = PH_SCEN[scen].pool[i];
    } else {
        /* the Random Scenario: two different stars, eleven different guests */
        uint8_t stars[9], guests[G_LAST_BUYABLE - G_FIRST_BUYABLE + 1];
        int ns = 0, ng = 0;
        for (int t = G_FIRST_STAR; t < G_COUNT; t++) stars[ns++] = (uint8_t)t;
        for (int t = G_FIRST_BUYABLE; t <= G_LAST_BUYABLE; t++) guests[ng++] = (uint8_t)t;
        for (int k = 0; k < 2; k++) {
            int i = k + rnd(g, ns - k);
            uint8_t tmp = stars[k]; stars[k] = stars[i]; stars[i] = tmp;
            g->pool[g->npool++] = stars[k];
        }
        for (int k = 0; k < 11; k++) {
            int i = k + rnd(g, ng - k);
            uint8_t tmp = guests[k]; guests[k] = guests[i]; guests[i] = tmp;
            g->pool[g->npool++] = guests[k];
        }
    }
    qsort(g->pool, g->npool, 1, pool_cmp);
    for (int p = 0; p < g->players; p++) {
        PhPlayer *pl = &g->pl[p];
        pl->cap = PH_START_HOUSE;
        for (int i = 0; i < 4; i++) add_card(pl, G_NEIGHBOUR);
        for (int i = 0; i < 2; i++) add_card(pl, G_COUSIN);
        for (int i = 0; i < 4; i++) add_card(pl, G_ROWDY);
    }
    g->turn = 0;
    ph_start_party(g);
}

void ph_start_party(PhGame *g) {
    PhPlayer *p = ph_me(g);
    PhParty *pa = &g->party;
    memset(pa, 0, sizeof *pa);
    pa->peek = -1;
    if (p->banned) pa->where[p->banned - 1] = W_OUT;
    p->banned = 0;
    p->night++;
}

/* ------------------------------------------------------------------ */
/* reading the party                                                    */

int ph_count_type(const PhGame *g, int type, int where) {
    const PhPlayer *p = ph_me_c(g);
    int n = 0;
    for (int i = 0; i < p->ncards; i++)
        if (p->card[i].type == type && g->party.where[i] == where) n++;
    return n;
}

bool ph_is_wild(const PhGame *g, int card) { return g->party.wild[card] && !g->party.calm[card]; }

int ph_trouble_status(const PhGame *g) {
    int n = 0;
    for (int i = 0; i < g->party.n; i++) n += ph_is_wild(g, g->party.house[i]);
    return n;
}

int ph_trouble(const PhGame *g) {
    const PhPlayer *p = ph_me_c(g);
    int calm = 0;
    for (int i = 0; i < g->party.n; i++) calm += (PH_GUESTS[p->card[g->party.house[i]].type].traits & T_PEACE) != 0;
    int t = ph_trouble_status(g) - calm;
    return t < 0 ? 0 : t;
}

int ph_stars(const PhGame *g) {
    const PhPlayer *p = ph_me_c(g);
    int n = 0;
    for (int i = 0; i < g->party.n; i++) n += is_star(p->card[g->party.house[i]].type);
    return n;
}

int ph_value_pop(const PhGame *g, int card) {
    const PhPlayer *p = ph_me_c(g);
    const PhCard *c = &p->card[card];
    const PhGuest *t = &PH_GUESTS[c->type];
    int v = t->pop + c->bonus;
    if (t->traits & T_UPSTART) v = (c->visits > 9 ? 9 : c->visits) + c->bonus;
    if (t->traits & T_DRUM) v += ph_count_type(g, G_DRUMMER, W_HOUSE);
    if (t->traits & T_GRANNY) v += ph_count_type(g, G_NEIGHBOUR, W_HOUSE);
    if (t->traits & T_BOOK) v += p->cap - g->party.n;
    if (t->traits & T_POET) v += 2 * ph_trouble_status(g);
    if ((t->traits & T_STORY) && g->party.n >= p->cap) v += 5;
    return v;
}

int ph_value_cash(const PhGame *g, int card) {
    const PhCard *c = &ph_me_c(g)->card[card];
    const PhGuest *t = &PH_GUESTS[c->type];
    int v = t->cash;
    if (t->traits & T_BARISTA) v += 2 * ph_trouble_status(g);
    return v;
}

int ph_draw(PhGame *g) {
    PhPlayer *p = ph_me(g);
    int n = 0;
    for (int i = 0; i < p->ncards; i++) n += g->party.where[i] == W_POOL && i != g->party.peek;
    if (n == 0) return -1;
    int k = rnd(g, n);
    for (int i = 0; i < p->ncards; i++)
        if (g->party.where[i] == W_POOL && i != g->party.peek && k-- == 0) return i;
    return -1;
}

/* ------------------------------------------------------------------ */
/* guests coming and going                                              */

static void admit(PhGame *g, int card) {
    PhParty *pa = &g->party;
    PhPlayer *p = ph_me(g);
    if (pa->over) return;
    if (pa->n >= p->cap) { pa->over = PO_FIRE; return; }
    PhCard *c = &p->card[card];
    const PhGuest *t = &PH_GUESTS[c->type];
    pa->where[card] = W_HOUSE;
    pa->house[pa->n++] = (uint8_t)card;
    if (pa->nlast < PH_MAX_HOUSE) pa->last[pa->nlast++] = (uint8_t)card;
    pa->calm[card] = 0;
    if (c->visits < 250) c->visits++;
    pa->wild[card] = (t->traits & T_TROUBLE) || ((t->traits & T_MOON) && (c->visits & 1));
    if (ph_trouble(g) >= 2) pa->warned = 1;
    if (ph_trouble(g) >= 3) { pa->over = PO_POLICE; return; }
    int bring = (t->traits & T_BRING2) ? 2 : (t->traits & T_BRING1) ? 1 : 0;
    for (int k = 0; k < bring && !pa->over; k++) {
        int d = ph_draw(g);
        if (d < 0) break;
        if (pa->n >= p->cap) { pa->over = PO_FIRE; return; }
        admit(g, d);
    }
}

static void check_win(PhGame *g) {
    if (!g->party.over && ph_stars(g) >= 4) {
        g->party.over = PO_WON;
        ph_me(g)->won = 1;
        g->winner = (uint8_t)(g->turn + 1);
        g->done = 1;
    }
}

static void remove_slot(PhGame *g, int slot, int where) {
    PhParty *pa = &g->party;
    int card = pa->house[slot];
    pa->where[card] = (uint8_t)where;
    pa->calm[card] = 0;
    for (int i = slot; i + 1 < pa->n; i++) pa->house[i] = pa->house[i + 1];
    pa->n--;
}

/* pay out one guest now (paparazzo, usher): a drummer brings the whole band's pay */
static void collect(PhGame *g, int card) {
    PhPlayer *p = ph_me(g);
    int v = ph_value_pop(g, card);
    if (PH_GUESTS[p->card[card].type].traits & T_DRUM) v *= ph_count_type(g, G_DRUMMER, W_HOUSE);
    int c = ph_value_cash(g, card);
    p->pop = (int16_t)(p->pop + v < 0 ? 0 : p->pop + v);
    g->party.got_pop = (int16_t)(g->party.got_pop + v);
    if (c >= 0) {
        p->cash = (int16_t)(p->cash + c);
        g->party.got_cash = (int16_t)(g->party.got_cash + c);
    } else if (p->cash >= -c) {
        p->cash = (int16_t)(p->cash + c);
        g->party.got_cash = (int16_t)(g->party.got_cash + c);
    } else {
        p->pop = (int16_t)(p->pop - 7 < 0 ? 0 : p->pop - 7);
        g->party.got_pop = (int16_t)(g->party.got_pop - 7);
    }
}

bool ph_open_door(PhGame *g) {
    PhParty *pa = &g->party;
    if (pa->over || pa->n >= ph_me(g)->cap) return false;
    int card = pa->peek >= 0 ? pa->peek : ph_draw(g);
    if (card < 0) return false;
    pa->peek = -1;
    pa->nlast = 0;
    admit(g, card);
    check_win(g);
    return true;
}

bool ph_peek_decide(PhGame *g, bool let_in) {
    PhParty *pa = &g->party;
    if (pa->peek < 0 || pa->over) return false;
    if (let_in) return ph_open_door(g);
    pa->where[pa->peek] = W_OUT; /* turned away for tonight */
    pa->peek = -1;
    return true;
}

/* ------------------------------------------------------------------ */
/* actions                                                              */

static bool any_in_pool(const PhGame *g, int want_star) {
    const PhPlayer *p = ph_me_c(g);
    for (int i = 0; i < p->ncards; i++)
        if (g->party.where[i] == W_POOL && i != g->party.peek && (want_star < 0 || is_star(p->card[i].type) == want_star)) return true;
    return false;
}

bool ph_fetch_ok(const PhGame *g, int type) {
    return g->party.n < ph_me_c(g)->cap && ph_count_type(g, type, W_POOL) - (g->party.peek >= 0 && ph_me_c(g)->card[g->party.peek].type == type) > 0;
}

bool ph_target_ok(const PhGame *g, int slot, int target) {
    const PhParty *pa = &g->party;
    const PhPlayer *p = ph_me_c(g);
    if (slot < 0 || slot >= pa->n || target < 0 || target >= pa->n) return false;
    int act = PH_GUESTS[p->card[pa->house[slot]].type].action;
    int tc = pa->house[target];
    switch (act) {
    case A_BOOT: return true;
    case A_PHOTO: return target != slot;
    case A_STYLE: return ph_value_pop(g, tc) < 9;
    case A_MAGIC: return any_in_pool(g, !is_star(p->card[tc].type));
    case A_CUPID: return target + 1 < pa->n && (target % PH_ROW) != PH_ROW - 1;
    default: return false;
    }
}

bool ph_can_act(const PhGame *g, int slot) {
    const PhParty *pa = &g->party;
    const PhPlayer *p = ph_me_c(g);
    if (pa->over || slot < 0 || slot >= pa->n) return false;
    int card = pa->house[slot];
    int act = PH_GUESTS[p->card[card].type].action;
    if (act == A_NONE || pa->used[card]) return false;
    switch (act) {
    case A_FETCH:
        for (int t = 0; t < G_COUNT; t++)
            if (ph_fetch_ok(g, t)) return true;
        return false;
    case A_PEEK: return pa->peek < 0 && any_in_pool(g, -1);
    case A_RESHUFFLE: return true;
    case A_CHEER:
        for (int i = 0; i < pa->n; i++) {
            int c = pa->house[i];
            if (pa->used[c] && p->card[c].type != G_BANDLEADER) return true;
        }
        return false;
    case A_GREET: return pa->n < p->cap && (pa->peek >= 0 || any_in_pool(g, -1));
    case A_CALM: return ph_trouble_status(g) > 0;
    default:
        for (int t = 0; t < pa->n; t++)
            if (ph_target_ok(g, slot, t)) return true;
        return false;
    }
}

bool ph_act(PhGame *g, int slot, int target) {
    if (!ph_can_act(g, slot)) return false;
    PhParty *pa = &g->party;
    PhPlayer *p = ph_me(g);
    int card = pa->house[slot];
    int act = PH_GUESTS[p->card[card].type].action;
    switch (act) {
    case A_FETCH: {
        if (target < 0 || target >= G_COUNT || !ph_fetch_ok(g, target)) return false;
        int n = ph_count_type(g, target, W_POOL), k = rnd(g, n);
        for (int i = 0; i < p->ncards; i++)
            if (p->card[i].type == target && pa->where[i] == W_POOL && i != pa->peek && k-- == 0) {
                pa->used[card] = 1;
                pa->nlast = 0;
                admit(g, i);
                check_win(g);
                return true;
            }
        return false;
    }
    case A_BOOT:
        if (!ph_target_ok(g, slot, target)) return false;
        pa->used[card] = 1;
        remove_slot(g, target, W_OUT);
        return true;
    case A_PEEK:
        pa->peek = (int16_t)ph_draw(g);
        if (pa->peek < 0) return false;
        pa->used[card] = 1;
        return true;
    case A_RESHUFFLE:
        pa->used[card] = 1;
        while (pa->n > 0) remove_slot(g, pa->n - 1, W_POOL);
        return true;
    case A_PHOTO:
        if (!ph_target_ok(g, slot, target)) return false;
        pa->used[card] = 1;
        collect(g, pa->house[target]);
        return true;
    case A_STYLE:
        if (!ph_target_ok(g, slot, target)) return false;
        pa->used[card] = 1;
        p->card[pa->house[target]].bonus++;
        return true;
    case A_CHEER:
        pa->used[card] = 1;
        for (int i = 0; i < pa->n; i++)
            if (p->card[pa->house[i]].type != G_BANDLEADER) pa->used[pa->house[i]] = 0;
        return true;
    case A_GREET: {
        int c = pa->peek >= 0 ? pa->peek : ph_draw(g);
        if (c < 0) return false;
        pa->used[card] = 1;
        pa->peek = -1;
        pa->nlast = 0;
        admit(g, c);
        if (!pa->over)
            for (int i = 0; i < pa->nlast; i++)
                if (pa->where[pa->last[i]] == W_HOUSE) collect(g, pa->last[i]);
        check_win(g);
        return true;
    }
    case A_MAGIC: {
        if (!ph_target_ok(g, slot, target)) return false;
        int tc = pa->house[target];
        int want = !is_star(p->card[tc].type);
        int n = 0;
        for (int i = 0; i < p->ncards; i++) n += pa->where[i] == W_POOL && i != pa->peek && is_star(p->card[i].type) == want;
        int k = rnd(g, n);
        for (int i = 0; i < p->ncards; i++)
            if (pa->where[i] == W_POOL && i != pa->peek && is_star(p->card[i].type) == want && k-- == 0) {
                pa->used[card] = 1;
                remove_slot(g, target, W_OUT);
                pa->nlast = 0;
                admit(g, i);
                check_win(g);
                return true;
            }
        return false;
    }
    case A_CUPID:
        if (!ph_target_ok(g, slot, target)) return false;
        pa->used[card] = 1;
        remove_slot(g, target + 1, W_OUT);
        remove_slot(g, target, W_OUT);
        return true;
    case A_CALM:
        pa->used[card] = 1;
        for (int i = 0; i < pa->n; i++) pa->calm[pa->house[i]] = 1;
        return true;
    default: return false;
    }
}

bool ph_should_end(const PhGame *g) {
    if (g->party.over || g->party.n < ph_me_c(g)->cap) return false;
    for (int i = 0; i < g->party.n; i++)
        if (ph_can_act(g, i)) return false;
    return true;
}

/* the tally: every guest's popularity, then the cash they bring, then the
 * guests who charge; one you can't pay costs 7 popularity instead */
void ph_end_party(PhGame *g) {
    PhParty *pa = &g->party;
    PhPlayer *p = ph_me(g);
    if (pa->over) return;
    int pop = 0, income = 0;
    for (int i = 0; i < pa->n; i++) {
        pop += ph_value_pop(g, pa->house[i]);
        int c = ph_value_cash(g, pa->house[i]);
        if (c > 0) income += c;
    }
    int np = p->pop + pop;
    p->pop = (int16_t)(np < 0 ? 0 : np);
    p->cash = (int16_t)(p->cash + income);
    int spent = 0, penalty = 0;
    for (int i = 0; i < pa->n; i++) {
        int c = ph_value_cash(g, pa->house[i]);
        if (c >= 0) continue;
        if (p->cash >= -c) { p->cash = (int16_t)(p->cash + c); spent -= c; }
        else { penalty += 7; p->pop = (int16_t)(p->pop - 7 < 0 ? 0 : p->pop - 7); }
    }
    pa->end_pop = (int16_t)pop;
    pa->end_cash = (int16_t)(income - spent);
    pa->penalty = (int16_t)penalty;
    pa->over = PO_ENDED;
}

void ph_ban(PhGame *g, int card) {
    if (card >= 0 && card < ph_me(g)->ncards) ph_me(g)->banned = (uint8_t)(card + 1);
}

/* after a party is over (and any ban chosen): the next night, or the end */
void ph_next_turn(PhGame *g) {
    if (g->done) return;
    PhPlayer *p = ph_me(g);
    if (!p->won && p->night >= PH_NIGHTS) p->lost = 1;
    if (g->players == 2) {
        int other = g->turn ^ 1;
        if (!g->pl[other].lost) g->turn = (uint8_t)other;
        if (g->pl[0].lost && g->pl[1].lost) { g->done = 1; return; }
    } else if (p->lost) {
        g->done = 1;
        return;
    }
    ph_start_party(g);
}

/* ------------------------------------------------------------------ */
/* the shop                                                             */

int ph_expand_cost(const PhPlayer *p) { return p->expansions + 2 > 12 ? 12 : p->expansions + 2; }

bool ph_can_buy(const PhGame *g, int type) {
    const PhPlayer *p = ph_me_c(g);
    bool on_sale = false;
    for (int i = 0; i < g->npool; i++) on_sale |= g->pool[i] == type;
    if (!on_sale || PH_GUESTS[type].cost < 0 || p->pop < PH_GUESTS[type].cost) return false;
    if (!is_star(type) && g->bought[type] >= PH_STOCK) return false;
    return p->ncards < PH_MAX_CARDS;
}

bool ph_buy(PhGame *g, int type) {
    if (!ph_can_buy(g, type)) return false;
    PhPlayer *p = ph_me(g);
    p->pop = (int16_t)(p->pop - PH_GUESTS[type].cost);
    add_card(p, type);
    if (g->bought[type] < 255) g->bought[type]++;
    /* the secret: buy every neighbour (or cousin) and they tell you their names */
    if ((type == G_NEIGHBOUR || type == G_COUSIN) && g->bought[type] == PH_STOCK) {
        int k = 0;
        for (int i = 10; i < p->ncards; i++)
            if (p->card[i].type == type) p->card[i].name = (uint8_t)(1 + (type == G_COUSIN ? 4 : 0) + k++ % 4);
    }
    return true;
}

bool ph_expand(PhGame *g) {
    PhPlayer *p = ph_me(g);
    int c = ph_expand_cost(p);
    if (p->cap >= PH_MAX_HOUSE || p->cash < c) return false;
    p->cash = (int16_t)(p->cash - c);
    p->cap++;
    p->expansions++;
    return true;
}
