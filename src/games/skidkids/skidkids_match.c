/* SKID KIDS - the rules of a match: kids, bags, the Coach and his items,
 * the special throws and moves. Pure logic: no drawing, no input. */
#include "skidkids.h"

const char *const SKID_THROW_NAME[ST_COUNT] = {"COMET", "RIPPLE", "YO-YO", "POPPER", "MARBLE BAG", "HOMING POPPER"};
const char *const SKID_MOVE_NAME[SA_COUNT] = {"STOMP", "GUST", "REEL", "SPLASH", "POUCH GRAB", "SWEEPER"};

/* Twelve kids with the original's twelve kits, one for one (see the doc's
 * name map), and the two partners the tournament saves for last. */
const KidDef SKID_KID[SKID_ROSTER] = {
    {"NOODLE", "WIGGLY THROWS", "MY THROWS GO ALL WIGGLY. NOBODY KNOWS WHY. NOT EVEN ME.",
     P_WAVY, ST_COMET, SA_STOMP, C_TAN, C_BROWN, C_SLATE, C_WHITE},
    {"PIPPA", "GETS UP FAST", "KNOCK ME DOWN ALL YOU LIKE. I BOUNCE.",
     P_QUICKUP, ST_RIPPLE, SA_GUST, C_CREAM, C_ORANGE, C_PURPLE, C_WHITE},
    {"HOPS", "JUMPS HIGH AND QUICK", "I CAN TOUCH THE TOP OF THE DOORFRAME. CAN YOU?",
     P_HIGHJUMP, ST_RIPPLE, SA_REEL, C_HIDE, C_INK, C_FOREST, C_LIME},
    {"MILO", "HIGH, LONG LOBS", "I WORKED OUT THE ARC. YOU WON'T CLEAR IT.",
     P_LOB, ST_POPPER, SA_REEL, C_CREAM, C_AMBER, C_SLATE, C_GREY},
    {"SPARKY", "QUICK WIND-UP", "WIND-UP? WHAT WIND-UP? I'M ALREADY THROWING.",
     P_QUICKCHARGE, ST_POPPER, SA_SPLASH, C_TAN, C_YELLOW, C_NIGHT, C_ORANGE},
    {"NELL", "SHARES HER STARS", "ON OUR TEAM WE SHARE EVERYTHING. ESPECIALLY STARS.",
     P_SHARE, ST_YOYO, SA_STOMP, C_HIDE, C_MAROON, C_TEAL, C_WHITE},
    {"KIKI", "CARRIES TWO BAGS", "ONE BAG IN EACH HAND. DO THE MATH.",
     P_TWOBAGS, ST_YOYO, SA_GUST, C_CREAM, C_INK, C_MAGENTA, C_PINK},
    {"ROXIE", "FAST, NEVER SLIPS", "NEW HIGH-TOPS. ZERO SLIP. TRY TO KEEP UP.",
     P_FAST | P_NOSLIP, ST_COMET, SA_GUST, C_TAN, C_PINK, C_VIOLET, C_WHITE},
    {"TOBY", "QUICK HANDS", "GRAB, THROW, GRAB, THROW. I'M VERY BUSY.",
     P_QUICKGRAB, ST_COMET, SA_SPLASH, C_CREAM, C_EARTH, C_JADE, C_AMBER},
    {"SID", "IGNORES THE LINE", "WHAT LINE? I DON'T SEE ANY LINE.",
     P_CROSS, ST_POPPER, SA_STOMP, C_TAN, C_SLATE, C_BROWN, C_INK},
    {"BUZZY", "THE COACH'S PET", "THE COACH LOVES THE HORNET. EVERYBODY LOVES THE HORNET.",
     P_FAVORED, ST_RIPPLE, SA_SPLASH, C_CREAM, C_YELLOW, C_YELLOW, C_INK},
    {"MOOSE", "RUNS RIVALS OVER", "EXCUSE ME. COMING THROUGH.",
     P_PUSH, ST_YOYO, SA_REEL, C_HIDE, C_EARTH, C_DUSK, C_BROWN},
    {"BOOMER", "HOPS, POUCH, SPEED", "THUMP. THUMP THUMP.",
     P_HIGHJUMP | P_TWOBAGS | P_FAST | P_NOSLIP, ST_MARBLE, SA_POUCH, C_AMBER, C_BROWN, C_TAN, C_BROWN},
    {"BENCHBOT", "LOBS, WINDS, SHOVES", "BENCHBOT ONLINE. OBJECTIVE: THE TEAM THAT LEFT MY MAKER ON THE BENCH.",
     P_LOB | P_QUICKCHARGE | P_PUSH | P_BIGPUSH, ST_HOMING, SA_SWEEPER, C_GREY, C_SLATE, C_GREY, C_SLATE},
};

/* ------------------------------------------------------------------ */
/* numbers                                                              */

#define WALK 1.0f
#define WALK_FAST 1.3f
#define JUMP_V 2.1f
#define JUMP_G 0.135f
#define HIGH_V 2.7f
#define HIGH_G 0.2f
#define PICK_R_X 9
#define PICK_R_Y 7
#define STOP_SPEED 0.3f
#define FRICTION 0.03f
#define BAG_G 0.18f
#define LOB_G 0.12f
#define HURT_TIME 14
#define THROW_TIME 8
#define POP_RADIUS 26.0f
#define POP_FUSE 20
#define GUST_TIME 100
#define PUDDLE_LIFE 600
#define MARBLE_LIFE 1200

bool skid_airborne(const Kid *k) { return k->z > 0.01f; }
int skid_charge_full(const Kid *k) { return (k->pas & P_QUICKCHARGE) ? 30 : 60; }
int skid_pick_time(const Kid *k) { return (k->pas & P_QUICKGRAB) ? 7 : 15; }
int skid_down_time(const Kid *k) { return (k->pas & P_QUICKUP) ? 38 : 75; }
int skid_capacity(const Kid *k) { return (k->pas & P_TWOBAGS) ? 2 : 1; }
static float kid_speed(const Kid *k) { return (k->pas & P_FAST) ? WALK_FAST : WALK; }
static float kid_grav(const Kid *k) { return (k->pas & P_HIGHJUMP) ? HIGH_G : JUMP_G; }
static int mate_of(int i) { return i ^ 1; }
static int team_dir(int team) { return team == 0 ? 1 : -1; }

void skid_rules_default(Rules *r) {
    r->goal = 15;
    r->max_bags = 5;
    r->cost = 1;
    r->mate_ai = 0;
    r->mate_jump = 0;
}

void skid_kid_bounds(const Match *m, int i, float *x0, float *x1) {
    const Kid *k = &m->k[i];
    if (k->pas & P_CROSS) {
        *x0 = SKID_LEFT + 6;
        *x1 = SKID_RIGHT - 6;
    } else if (k->team == 0) {
        *x0 = SKID_LEFT + 6;
        *x1 = SKID_MID - 4;
    } else {
        *x0 = SKID_MID + 4;
        *x1 = SKID_RIGHT - 6;
    }
}

int skid_bags_total(const Match *m) {
    int n = 0;
    for (int b = 0; b < SKID_MAX_BAGS; b++) n += m->bag[b].state != BG_GONE;
    return n;
}

int skid_bag_add(Match *m) {
    for (int b = 0; b < SKID_MAX_BAGS; b++)
        if (m->bag[b].state == BG_GONE) {
            Bag *g = &m->bag[b];
            memset(g, 0, sizeof *g);
            g->state = BG_LOOSE;
            g->team = -1;
            g->owner = -1;
            g->target = -1;
            g->radius = 2;
            g->friction = FRICTION;
            return b;
        }
    return -1;
}

int skid_item_add(Match *m, int kind) {
    for (int i = 0; i < SKID_MAX_ITEMS; i++)
        if (m->it[i].kind == IT_NONE) {
            memset(&m->it[i], 0, sizeof m->it[i]);
            m->it[i].kind = (uint8_t)kind;
            m->it[i].team = -1;
            return i;
        }
    return -1;
}

static void ev(Match *m, uint32_t bits, float x, float y) {
    m->ev |= bits;
    m->ev_x = x;
    m->ev_y = y;
}

static void bark(Match *m, int b) {
    if (m->bark_t > 40 && b != BARK_GAME && b != BARK_PLAY) return; /* he's still talking */
    m->bark = b;
    m->bark_t = 90;
}

/* ------------------------------------------------------------------ */
/* set-up                                                               */

static void place_kid(Match *m, int i) {
    Kid *k = &m->k[i];
    k->x = k->team == 0 ? 62.0f : 258.0f;
    k->y = k->slot == 0 ? 84.0f : 136.0f;
    k->face = (int8_t)team_dir(k->team);
    k->aimx = k->face;
    k->aimy = 0;
}

void skid_match_init(Match *m, const int who[4], const Rules *r, uint64_t seed) {
    memset(m, 0, sizeof *m);
    m->rules = *r;
    m->rules.goal = iclamp(m->rules.goal, 1, 999);
    m->rules.max_bags = iclamp(m->rules.max_bags, 1, 7);
    m->rules.cost = iclamp(m->rules.cost, 1, 2);
    rng_seed(&m->rng, seed);
    m->favored = -1;
    for (int i = 0; i < 4; i++) {
        Kid *k = &m->k[i];
        k->who = (uint8_t)iclamp(who[i], 0, SKID_ROSTER - 1);
        k->team = (uint8_t)(i / 2);
        k->slot = (uint8_t)(i % 2);
        k->pas = SKID_KID[k->who].passives;
        k->state = KS_STAND;
        k->ai_seen = -1;
        k->ai_target = -1;
        if (k->pas & P_FAVORED) m->favored = k->team;
        place_kid(m, i);
    }
    m->player[0] = CTRL_P1;
    m->player[1] = CTRL_AI;
    m->ctrl[0] = 0;
    m->ctrl[1] = 2;
    m->winner = -1;
    m->state = MS_READY;
    /* one bag on each side to start; the Coach's favourite gets the other
     * team's bag put further from them */
    for (int t = 0; t < 2; t++) {
        int b = skid_bag_add(m);
        Bag *g = &m->bag[b];
        float near = t == 0 ? 102.0f : 218.0f;
        float far = t == 0 ? 150.0f : 170.0f;
        g->x = (m->favored >= 0 && m->favored != t) ? far : near;
        g->y = 110;
    }
    m->coach_next = 40; /* the Coach tosses one more out to the line */
}

/* ------------------------------------------------------------------ */
/* stars, hits and knockdowns                                           */

static void add_half(Match *m, int i, int n) {
    Kid *k = &m->k[i];
    k->stars = imin(6, k->stars + n);
    if (k->pas & P_SHARE) {
        Kid *o = &m->k[mate_of(i)];
        o->stars = imin(6, o->stars + n);
    }
    m->n_half += n;
    ev(m, EV_HALFSTAR, k->x, k->y);
}

static void cancel_actions(Kid *k) {
    k->armed = false;
    k->charge = 0;
    k->pick_kind = 0;
}

void skid_knockdown(Match *m, int i, int frames) {
    Kid *k = &m->k[i];
    if (k->state == KS_DOWN || k->state == KS_WIN || k->state == KS_SAD) return;
    cancel_actions(k);
    k->state = KS_DOWN;
    k->t = 0;
    k->timer = frames;
    if (k->vz > 0) k->vz = 0;
    m->n_down++;
    ev(m, EV_DOWN, k->x, k->y);
}

static void hurt(Match *m, int i) {
    Kid *k = &m->k[i];
    if (k->state == KS_DOWN || k->state == KS_WIN || k->state == KS_SAD) return;
    cancel_actions(k);
    k->state = KS_HURT;
    k->t = 0;
    k->timer = HURT_TIME;
}

/* A point for team `scorer` on kid i, with a knockdown or a flinch. A kid
 * already down can be hit (and scored on) but stays down no longer. */

static void score_on(Match *m, int i, int scorer, bool knock) {
    Kid *k = &m->k[i];
    if (m->state != MS_PLAY) return; /* nothing counts once it's over */
    m->score[scorer] = imin(999, m->score[scorer] + 1);
    m->n_hit++;
    m->hits_on[i]++;
    m->hit_when[k->state == KS_DOWN ? 3 : (k->state == KS_PICKUP || k->state == KS_CHARGE || k->state == KS_HURT) ? 2 : skid_airborne(k) ? 1 : 0]++;
    ev(m, EV_HIT, k->x, k->y);
    if (knock) skid_knockdown(m, i, skid_down_time(k));
    else hurt(m, i);
    if (rng_chance(&m->rng, 30)) bark(m, BARK_OUCH);
}

static bool can_hit(const Bag *g, const Kid *k) {
    return g->team < 0 || g->team != (int)k->team;
}

static void bag_hits_kid(Match *m, int b, int i) {
    Bag *g = &m->bag[b];
    Kid *k = &m->k[i];
    g->hitmask |= (uint8_t)(1u << i);
    int scorer = g->team < 0 ? 1 - k->team : g->team;
    bool knock = g->heavy || (g->toss && g->z > 1.0f);
    score_on(m, i, scorer, knock);
    if (g->kind == BK_YOYO) return; /* it flies on, and turns for home to hit again */
    g->dead = true;
    g->vx *= -0.3f;
    g->vy *= -0.3f;
    if (g->state == BG_AIR && g->vz > 0) g->vz = 0;
}

/* ------------------------------------------------------------------ */
/* throwing, passing, picking up                                        */

static int held_bag(const Match *m, int i) {
    for (int b = 0; b < SKID_MAX_BAGS; b++)
        if (m->bag[b].state == BG_HELD && m->bag[b].owner == i) return b;
    return -1;
}

static void aim_of(const Kid *k, float *dx, float *dy) {
    float ax = k->aimx, ay = k->aimy;
    if (ax == 0 && ay == 0) ax = (float)team_dir(k->team);
    float l = sqrtf(ax * ax + ay * ay);
    *dx = ax / l;
    *dy = ay / l;
}

static void start_wave(Match *m, int i, int b) {
    for (int w = 0; w < SKID_MAX_WAVES; w++)
        if (!m->wave[w].live) {
            Wave *v = &m->wave[w];
            memset(v, 0, sizeof *v);
            v->live = true;
            v->team = (int8_t)m->k[i].team;
            v->dir = (int8_t)team_dir(m->k[i].team);
            v->x = m->k[i].x + v->dir * 6;
            v->y = m->k[i].y;
            v->half = 22;
            v->bag = b;
            m->bag[b].state = BG_WAVE;
            return;
        }
    m->bag[b].state = BG_SLIDE; /* no room for a wave: an ordinary throw */
}

/* Let go of a bag. charge = frames B was held. */
static void throw_bag(Match *m, int i, int charge, bool forced) {
    Kid *k = &m->k[i];
    int b = held_bag(m, i);
    if (b < 0) {
        k->state = KS_STAND; /* nothing in hand after all */
        k->armed = false;
        k->charge = 0;
        k->held = 0;
        return;
    }
    Bag *g = &m->bag[b];
    int full_t = skid_charge_full(k);
    bool full = charge >= full_t;
    float p = fclamp((float)(charge - SKID_TAP) / (float)(full_t - SKID_TAP), 0, 1);
    float dx, dy;
    aim_of(k, &dx, &dy);
    g->team = (int8_t)k->team;
    g->owner = (int8_t)i;
    g->hitmask = g->jumpmask = 0;
    g->age = 0;
    g->phase = 0;
    g->fuse = 0;
    g->dead = false;
    g->toss = g->lob = g->wavy = false;
    g->heavy = full;
    g->kind = BK_NORMAL;
    g->radius = 2;
    g->friction = FRICTION;
    g->x = k->x + dx * 6;
    g->y = k->y + dy * 3;
    g->z = 2;
    g->vz = 0;
    k->held--;
    k->hold_t = 0;
    k->armed = false;
    k->charge = 0;
    k->state = KS_THROW;
    k->t = 0;
    if (dx != 0) k->face = (int8_t)(dx > 0 ? 1 : -1);
    m->n_throw++;
    ev(m, EV_THROW, k->x, k->y);
    if (forced) {
        m->n_forced++;
        ev(m, EV_FORCED, k->x, k->y);
    }
    int cost = m->rules.cost * 2;
    if (full && k->stars >= cost) {
        const KidDef *d = &SKID_KID[k->who];
        k->stars -= cost;
        m->n_special_throw++;
        ev(m, EV_SPECIAL, k->x, k->y);
        float sp = 3.8f;
        switch (d->throw_kind) {
        case ST_COMET:
            g->kind = BK_COMET;
            g->radius = 7;
            g->friction = FRICTION * 0.5f;
            sp = 4.6f;
            break;
        case ST_RIPPLE:
            g->vx = g->vy = 0;
            start_wave(m, i, b);
            return;
        case ST_YOYO: g->kind = BK_YOYO; g->friction = 0; sp = 3.4f; break;
        case ST_POPPER: g->kind = BK_POPPER; break;
        case ST_MARBLE: g->kind = BK_MARBLE; break;
        default: g->kind = BK_HOMING; break;
        }
        g->vx = dx * sp;
        g->vy = dy * sp;
        g->state = BG_SLIDE;
        if ((k->pas & P_LOB) && g->kind != BK_YOYO && g->kind != BK_COMET) {
            g->state = BG_AIR;
            g->lob = true;
            g->vz = 1.2f;
            g->z = 4;
        }
        return;
    }
    if (p < 0.2f) {
        /* the light toss: a short lob that lands by your feet and stops */
        bool far = (k->pas & P_LOB) != 0;
        float sp = far ? 2.0f : 1.0f;
        g->vx = dx * sp;
        g->vy = dy * sp;
        g->z = 7;
        g->vz = far ? 2.4f : 1.3f;
        g->state = BG_AIR;
        g->toss = true;
        g->lob = true;
        m->n_toss++;
        return;
    }
    float sp = 1.3f + 2.5f * p;
    g->vx = dx * sp;
    g->vy = dy * sp;
    g->state = BG_SLIDE;
    if (k->pas & P_WAVY) {
        g->wavy = true;
        g->wamp = 7;
    }
    if (k->pas & P_LOB) {
        g->state = BG_AIR;
        g->lob = true;
        g->z = 4;
        g->vz = 0.9f + 0.4f * p; /* a low arc, top of a jump to clear it */
    }
}

static void pass_bag(Match *m, int i) {
    Kid *k = &m->k[i];
    int b = held_bag(m, i);
    if (b < 0) return;
    Bag *g = &m->bag[b];
    int j = mate_of(i);
    g->state = BG_PASS;
    g->target = (int8_t)j;
    g->team = (int8_t)k->team;
    g->owner = (int8_t)i;
    g->x = k->x;
    g->y = k->y;
    g->z = 7;
    float d = sqrtf((m->k[j].x - k->x) * (m->k[j].x - k->x) + (m->k[j].y - k->y) * (m->k[j].y - k->y));
    int T = imax(14, (int)(d / 3.0f));
    g->phase = T; /* frames of flight left */
    g->vz = (0.5f * BAG_G * T * T - 7) / T;
    k->held--;
    k->hold_t = 0;
    k->armed = false;
    k->charge = 0;
    m->n_pass++;
    ev(m, EV_PASS, k->x, k->y);
}

/* The thing within reach to pick up: 1 = a bag, 2 = a juice box; the
 * nearest wins. Juice only with empty hands. */
static int reach_thing(const Match *m, int i, int *idx) {
    const Kid *k = &m->k[i];
    float best = 1e9f;
    int kind = 0;
    if (k->held < skid_capacity(k))
        for (int b = 0; b < SKID_MAX_BAGS; b++) {
            const Bag *g = &m->bag[b];
            if (g->state != BG_LOOSE) continue;
            float dx = fabsf(g->x - k->x), dy = fabsf(g->y - k->y);
            if (dx > PICK_R_X || dy > PICK_R_Y) continue;
            if (dx + dy < best) { best = dx + dy; kind = 1; *idx = b; }
        }
    if (k->held == 0)
        for (int t = 0; t < SKID_MAX_ITEMS; t++) {
            const Item *it = &m->it[t];
            if (it->kind != IT_JUICE || it->flying) continue;
            float dx = fabsf(it->x - k->x), dy = fabsf(it->y - k->y);
            if (dx > PICK_R_X || dy > PICK_R_Y) continue;
            if (dx + dy < best) { best = dx + dy; kind = 2; *idx = t; }
        }
    return kind;
}

static void start_pickup(Match *m, int i, int kind, int idx) {
    Kid *k = &m->k[i];
    k->state = KS_PICKUP;
    k->t = 0;
    k->timer = skid_pick_time(k);
    k->pick_kind = kind;
    k->pick_i = idx;
}

static void finish_pickup(Match *m, int i) {
    Kid *k = &m->k[i];
    k->state = KS_STAND;
    k->t = 0;
    if (k->pick_kind == 1) {
        Bag *g = &m->bag[k->pick_i];
        if (g->state == BG_LOOSE && fabsf(g->x - k->x) <= PICK_R_X + 4 && fabsf(g->y - k->y) <= PICK_R_Y + 4 &&
            k->held < skid_capacity(k)) {
            g->state = BG_HELD;
            g->owner = (int8_t)i;
            g->team = (int8_t)k->team;
            g->kind = BK_NORMAL;
            if (k->held == 0) k->hold_t = 0;
            k->held++;
            m->n_pick++;
            ev(m, EV_PICK, k->x, k->y);
        }
    } else if (k->pick_kind == 2) {
        Item *it = &m->it[k->pick_i];
        if (it->kind == IT_JUICE && !it->flying && k->held == 0) {
            it->kind = IT_NONE;
            m->n_drink++;
            add_half(m, i, 1);
            ev(m, EV_DRINK, k->x, k->y);
        }
    }
    k->pick_kind = 0;
}

static void swap_to_mate(Match *m, int i) {
    int team = m->k[i].team;
    int j = mate_of(i);
    m->ctrl[team] = j;
    m->n_swap++;
    ev(m, EV_SWAP, m->k[j].x, m->k[j].y);
}

/* the kid's team is driven by a player who swaps between its two kids */
static bool swapper(const Match *m, int i) {
    int team = m->k[i].team;
    return m->player[team] != CTRL_AI && !(m->coop && team == 0) && m->ctrl[team] == i;
}

/* ------------------------------------------------------------------ */
/* special moves                                                        */

static void reel_nearest(Match *m, int i) {
    Kid *k = &m->k[i];
    int best = -1;
    float bd = 150.0f * 150.0f;
    for (int b = 0; b < SKID_MAX_BAGS; b++) {
        Bag *g = &m->bag[b];
        if (g->state == BG_GONE || g->state == BG_HELD || g->state == BG_REEL || g->state == BG_WAVE) continue;
        float d = (g->x - k->x) * (g->x - k->x) + (g->y - k->y) * (g->y - k->y);
        if (d < bd) { bd = d; best = b; }
    }
    if (best < 0) return;
    Bag *g = &m->bag[best];
    g->state = BG_REEL;
    g->target = (int8_t)i;
    g->team = (int8_t)k->team;
    g->dead = true;
    g->kind = BK_NORMAL;
    m->n_reel++;
    ev(m, EV_REEL, g->x, g->y);
}

static void throw_balloon(Match *m, float x, float y, float tx, float ty, int team, int T) {
    int n = skid_item_add(m, IT_BALLOON);
    if (n < 0) return;
    Item *it = &m->it[n];
    it->flying = true;
    it->team = (int8_t)team;
    it->x = x;
    it->y = y;
    it->z = 10;
    it->vx = (tx - x) / T;
    it->vy = (ty - y) / T;
    it->vz = (0.5f * BAG_G * T * T - 10) / T;
    it->life = T;
}

static void special_move(Match *m, int i) {
    Kid *k = &m->k[i];
    const KidDef *d = &SKID_KID[k->who];
    k->stars -= m->rules.cost * 2;
    k->air_move = true;
    k->vz = fmaxf(k->vz, 1.4f); /* a little second hop */
    m->n_special_move++;
    ev(m, EV_MOVE, k->x, k->y);
    int foe = 1 - k->team;
    switch (d->move_kind) {
    case SA_STOMP:
        /* the floor under the other team shakes: everyone on it falls */
        for (int j = foe * 2; j < foe * 2 + 2; j++)
            if (!skid_airborne(&m->k[j])) skid_knockdown(m, j, skid_down_time(&m->k[j]));
        m->quake_t = 30;
        ev(m, EV_STOMP, k->x, k->y);
        break;
    case SA_GUST:
        m->gust_t = GUST_TIME;
        m->gust_team = k->team;
        ev(m, EV_GUST, k->x, k->y);
        break;
    case SA_REEL: reel_nearest(m, i); break;
    case SA_POUCH:
        reel_nearest(m, i);
        k->queued_reel = 18;
        break;
    case SA_SPLASH: {
        float dx, dy;
        aim_of(k, &dx, &dy);
        float tx = fclamp(k->x + dx * 90, SKID_LEFT + 8, SKID_RIGHT - 8);
        float ty = fclamp(k->y + dy * 70, SKID_TOP + 6, SKID_BOT - 6);
        throw_balloon(m, k->x, k->y, tx, ty, k->team, 34);
        break;
    }
    default: /* SA_SWEEPER */
        if (!m->sweep.live) {
            Sweeper *s = &m->sweep;
            memset(s, 0, sizeof *s);
            s->live = true;
            s->team = (int8_t)k->team;
            s->dir = (int8_t)team_dir(foe); /* from the other team's wall toward the line */
            s->x = foe == 0 ? SKID_LEFT + 2 : SKID_RIGHT - 2;
            ev(m, EV_CLANK, s->x, 110);
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* a kid's frame                                                        */

static void do_jump(Match *m, int i) {
    Kid *k = &m->k[i];
    if (skid_airborne(k)) return;
    if (k->state != KS_STAND && k->state != KS_THROW) return;
    k->vz = (k->pas & P_HIGHJUMP) ? HIGH_V : JUMP_V;
    k->z = 0.01f;
    k->jumping = true;
    k->air_move = false;
    m->n_jump++;
    ev(m, EV_JUMP, k->x, k->y);
}

/* The whole team jumps together, unless its rule says each on its own (and
 * a CPU team's kids, and co-op's players, always jump on their own). */
static void team_jump(Match *m, int i) {
    do_jump(m, i);
    int team = m->k[i].team;
    if (m->player[team] == CTRL_AI || (m->coop && team == 0) || m->rules.mate_jump) return;
    do_jump(m, mate_of(i));
}

static void clamp_kid(Match *m, int i) {
    Kid *k = &m->k[i];
    float x0, x1;
    skid_kid_bounds(m, i, &x0, &x1);
    k->x = fclamp(k->x, x0, x1);
    k->y = fclamp(k->y, SKID_TOP + 2, SKID_BOT - 2);
}

static void kid_update(Match *m, int i, const Pad *p) {
    Kid *k = &m->k[i];
    k->t++;
    k->anim++;
    k->moved = false;
    if (k->slip_cd > 0) k->slip_cd--;
    if (k->state != KS_DOWN) k->since_up++;
    /* height */
    if (skid_airborne(k) || k->vz > 0) {
        k->z += k->vz;
        k->vz -= kid_grav(k);
        if (k->z <= 0) {
            k->z = 0;
            k->vz = 0;
            k->jumping = false;
            k->air_move = false;
        }
    }
    if (k->queued_reel > 0 && --k->queued_reel == 0) reel_nearest(m, i);
    if (m->state == MS_OVER) return;
    switch (k->state) {
    case KS_DOWN:
        if (--k->timer <= 0) {
            k->state = KS_STAND;
            k->t = 0;
            k->since_up = 0;
            k->slip_cd = 60;
        }
        return;
    case KS_HURT:
        if (--k->timer <= 0) { k->state = KS_STAND; k->t = 0; }
        return;
    case KS_PICKUP:
        if (--k->timer <= 0) finish_pickup(m, i);
        return;
    case KS_THROW:
        if (k->t >= THROW_TIME) { k->state = KS_STAND; k->t = 0; }
        break;
    default: break;
    }
    if (m->state != MS_PLAY) return;
    bool swap_only = m->swap_only && m->player[k->team] != CTRL_AI && m->ctrl[k->team] == i && !(m->coop && k->team == 0);
    /* the forced throw: a bag can't be kept for ever */
    if (k->held > 0) {
        k->hold_t++;
        if (k->hold_t >= SKID_FORCED) {
            int c = k->state == KS_CHARGE ? k->charge : (SKID_TAP + skid_charge_full(k)) / 2;
            if (k->state != KS_CHARGE) {
                k->aimx = (int8_t)team_dir(k->team);
                k->aimy = 0;
            }
            throw_bag(m, i, c, true);
            return;
        }
    }
    /* the throw button: B normally, A under the SWAP-ONLY code */
    bool tp = swap_only ? p->ap : p->bp, th = swap_only ? p->a : p->b, tr = swap_only ? (!p->a && k->armed) : p->br;
    if (k->state == KS_CHARGE) {
        /* the pad aims (the last way pressed sticks), and the kid stays put */
        if (p->dx || p->dy) {
            k->aimx = p->dx;
            k->aimy = p->dy;
        }
        if (k->aimx) k->face = k->aimx;
        bool cancel = swap_only ? p->bp : p->ap;
        if (cancel) {
            /* the jump button (B under SWAP-ONLY) calls the throw off */
            k->state = KS_STAND;
            k->t = 0;
            k->armed = false;
            k->charge = 0;
            m->n_cancel++;
            ev(m, EV_CANCEL, k->x, k->y);
            return;
        }
        if (th) {
            k->charge++;
            return;
        }
        throw_bag(m, i, k->charge, false);
        return;
    }
    /* moving */
    if (p->dx || p->dy) {
        float s = kid_speed(k);
        float mx = p->dx, my = p->dy;
        if (mx && my) { mx *= 0.7071f; my *= 0.7071f; }
        float ox = k->x, oy = k->y;
        k->x += mx * s;
        k->y += my * s;
        clamp_kid(m, i);
        k->moved = fabsf(k->x - ox) > 0.01f || fabsf(k->y - oy) > 0.01f;
        if (p->dx) k->face = p->dx;
        k->aimx = p->dx;
        k->aimy = p->dy;
    } else {
        k->aimx = (int8_t)team_dir(k->team);
        k->aimy = 0;
    }
    /* SWAP-ONLY: B swaps (and tosses the bag across if the kid left has one
     * and the other has none) */
    if (swap_only && p->bp) {
        Kid *o = &m->k[mate_of(i)];
        if (k->held > 0 && o->held == 0) pass_bag(m, i);
        swap_to_mate(m, i);
        return;
    }
    /* the jump button */
    bool jp = swap_only ? (p->ap && k->held == 0) : p->ap;
    if (swap_only && p->ap && k->held == 0) {
        int idx = 0, kind = reach_thing(m, i, &idx);
        if (kind && !skid_airborne(k)) { start_pickup(m, i, kind, idx); return; }
    }
    if (swap_only && p->ap && k->held > 0 && skid_airborne(k)) jp = true;
    if (jp) {
        if (skid_airborne(k)) {
            if (!k->air_move && k->stars >= m->rules.cost * 2) special_move(m, i);
        } else if (m->player[k->team] != CTRL_AI && !(m->coop && k->team == 0) && m->ctrl[k->team] == i) {
            team_jump(m, i);
        } else if (m->player[k->team] == CTRL_AI || (m->coop && k->team == 0) || m->rules.mate_jump) {
            do_jump(m, i); /* a CPU kid, a co-op player, or a mate on its own */
        }
    }
    /* the action button */
    if (k->held == 0) {
        if (!swap_only && p->bp) {
            int idx = 0, kind = reach_thing(m, i, &idx);
            if (kind) {
                if (!skid_airborne(k)) start_pickup(m, i, kind, idx);
            } else if (swapper(m, i)) {
                swap_to_mate(m, i);
            }
        }
        return;
    }
    if (tp && !(swap_only && skid_airborne(k))) {
        k->armed = true;
        k->charge = 0;
    }
    if (!k->armed) return;
    if (th) {
        if (skid_airborne(k)) {
            k->charge = imin(k->charge + 1, SKID_TAP - 1); /* a wind-up waits for the floor */
        } else if (++k->charge >= SKID_TAP) {
            k->state = KS_CHARGE;
            k->t = 0;
        }
        return;
    }
    if (tr || !th) {
        /* a tap: a second bag within reach (two-bag kids), else a pass */
        k->armed = false;
        if (swap_only) {
            throw_bag(m, i, k->charge, false); /* the light toss */
            return;
        }
        int idx = 0, kind = reach_thing(m, i, &idx);
        if (kind == 1 && !skid_airborne(k)) {
            start_pickup(m, i, kind, idx);
        } else {
            pass_bag(m, i);
            if (swapper(m, i)) swap_to_mate(m, i);
        }
    }
}

/* ------------------------------------------------------------------ */
/* bags                                                                 */

static void explode(Match *m, int b) {
    Bag *g = &m->bag[b];
    m->n_boom++;
    ev(m, EV_BOOM, g->x, g->y);
    if (g->kind == BK_MARBLE) {
        /* a ring of marbles: any kid who steps on one slips */
        for (int n = 0; n < 8; n++) {
            float a = n * 0.7854f + 0.39f;
            int t = skid_item_add(m, IT_MARBLE);
            if (t < 0) break;
            Item *it = &m->it[t];
            it->x = fclamp(g->x + cosf(a) * 22, SKID_LEFT + 4, SKID_RIGHT - 4);
            it->y = fclamp(g->y + sinf(a) * 16, SKID_TOP + 2, SKID_BOT - 2);
            it->life = MARBLE_LIFE;
            it->r = 4;
        }
    } else {
        for (int j = 0; j < 4; j++) {
            Kid *k = &m->k[j];
            if (k->team == (unsigned)g->team || k->z > 4) continue;
            float dx = k->x - g->x, dy = (k->y - g->y) * 1.4f;
            if (dx * dx + dy * dy > POP_RADIUS * POP_RADIUS) continue;
            score_on(m, j, g->team, true);
        }
    }
    bool home = g->kind == BK_HOMING && g->owner >= 0;
    g->kind = BK_NORMAL;
    g->fuse = 0;
    g->dead = true;
    if (home) {
        /* the robot's popper flies home after it goes off */
        g->state = BG_REEL;
        g->target = g->owner;
        g->z = 4;
    } else {
        g->state = BG_LOOSE;
        g->vx = g->vy = 0;
    }
}

static void bag_stop(Match *m, int b) {
    Bag *g = &m->bag[b];
    g->vx = g->vy = 0;
    g->z = 0;
    g->vz = 0;
    if (g->kind == BK_POPPER || g->kind == BK_MARBLE || g->kind == BK_HOMING) {
        if (g->fuse == 0) g->fuse = POP_FUSE;
        g->state = BG_SLIDE; /* sits fizzing */
        g->dead = true;
        return;
    }
    g->state = BG_LOOSE;
    g->kind = BK_NORMAL;
    g->dead = false;
    g->heavy = false;
    g->toss = false;
    g->wavy = false;
    g->radius = 2;
    g->friction = FRICTION;
    (void)m;
}

static void walls(Match *m, Bag *g) {
    bool bounced = false;
    if (g->x < SKID_LEFT + 2) { g->x = SKID_LEFT + 2; g->vx = fabsf(g->vx) * 0.8f; bounced = true; }
    if (g->x > SKID_RIGHT - 2) { g->x = SKID_RIGHT - 2; g->vx = -fabsf(g->vx) * 0.8f; bounced = true; }
    if (g->y < SKID_TOP) { g->y = SKID_TOP; g->vy = fabsf(g->vy) * 0.8f; bounced = true; }
    if (g->y > SKID_BOT) { g->y = SKID_BOT; g->vy = -fabsf(g->vy) * 0.8f; bounced = true; }
    if (bounced) {
        ev(m, EV_WALL, g->x, g->y);
        if (g->kind == BK_YOYO && g->phase == 0) { g->phase = 1; g->hitmask = 0; }
    }
}

/* hits and jumps over a dangerous bag */
static void bag_contacts(Match *m, int b) {
    Bag *g = &m->bag[b];
    for (int i = 0; i < 4; i++) {
        Kid *k = &m->k[i];
        if (!can_hit(g, k)) continue;
        float r = g->radius;
        if (fabsf(g->x - k->x) > 5 + r || fabsf(g->y - k->y) > 4 + r) continue;
        bool high = k->z >= g->z + 3;
        if (!high && g->z < k->z + 16 && !(g->hitmask & (1u << i))) {
            bag_hits_kid(m, b, i);
            if (g->dead) return;
        } else if (high && !(g->jumpmask & (1u << i)) && !(g->hitmask & (1u << i))) {
            g->jumpmask |= (uint8_t)(1u << i);
            add_half(m, i, 1);
        }
    }
}

static void bag_update(Match *m, int b) {
    Bag *g = &m->bag[b];
    g->age++;
    switch (g->state) {
    case BG_HELD: {
        Kid *k = &m->k[g->owner];
        g->x = k->x + k->face * 5;
        g->y = k->y;
        g->z = k->z + 7;
        return;
    }
    case BG_LOOSE: return;
    case BG_WAVE: return; /* moved by its wave */
    case BG_PASS: {
        Kid *t = &m->k[g->target];
        int left = imax(1, g->phase);
        g->vx = (t->x - g->x) / left;
        g->vy = (t->y - g->y) / left;
        g->x += g->vx;
        g->y += g->vy;
        g->z += g->vz;
        g->vz -= BAG_G;
        g->phase--;
        bool near = fabsf(g->x - t->x) < 8 && fabsf(g->y - t->y) < 8;
        if (near && g->z < 16 && t->state != KS_DOWN && t->state != KS_PICKUP && t->held < skid_capacity(t)) {
            g->state = BG_HELD;
            g->owner = g->target;
            if (t->held == 0) t->hold_t = 0;
            t->held++;
            return;
        }
        if (g->z <= 0 || g->phase < -30) {
            g->z = 0;
            bag_stop(m, b);
        }
        return;
    }
    case BG_REEL: {
        Kid *t = &m->k[g->target];
        float dx = t->x - g->x, dy = t->y - g->y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < 6) {
            if (t->held < skid_capacity(t) && t->state != KS_DOWN) {
                g->state = BG_HELD;
                g->owner = g->target;
                g->team = (int8_t)t->team;
                g->kind = BK_NORMAL;
                if (t->held == 0) t->hold_t = 0;
                t->held++;
            } else if (t->state == KS_DOWN) {
                bag_stop(m, b); /* dropped beside a fallen kid */
            } else {
                g->state = BG_GONE; /* hands full: it's gone */
            }
            return;
        }
        g->x += dx / d * 5;
        g->y += dy / d * 5;
        g->z = 8;
        return;
    }
    case BG_COACH: {
        g->x += g->vx;
        g->y += g->vy;
        g->z += g->vz;
        g->vz -= 0.12f;
        if (g->z <= 0) {
            g->z = 0;
            /* a bag from the Coach that lands on a kid is a point for the
             * other team */
            for (int i = 0; i < 4; i++) {
                Kid *k = &m->k[i];
                if (fabsf(k->x - g->x) <= 6 && fabsf(k->y - g->y) <= 5 && k->z < 8) {
                    score_on(m, i, 1 - k->team, false);
                    break;
                }
            }
            bag_stop(m, b);
        }
        return;
    }
    case BG_AIR:
        g->x += g->vx;
        g->y += g->vy;
        g->z += g->vz;
        g->vz -= g->lob && !g->toss ? LOB_G : BAG_G;
        walls(m, g);
        if (!g->dead) bag_contacts(m, b);
        if (g->state != BG_AIR) return;
        if (g->z <= 0) {
            g->z = g->lob && !g->toss ? 2 : 0;
            g->vz = 0;
            float keep = g->toss ? 0.35f : 0.9f;
            g->vx *= keep;
            g->vy *= keep;
            g->toss = false;
            g->state = BG_SLIDE;
        }
        return;
    case BG_SLIDE: {
        if (g->fuse > 0) {
            if (--g->fuse == 0) explode(m, b);
            return;
        }
        if (g->kind == BK_YOYO && g->phase == 1 && g->owner >= 0) {
            /* coming home */
            Kid *o = &m->k[g->owner];
            float dx = o->x - g->x, dy = o->y - g->y;
            float d = sqrtf(dx * dx + dy * dy);
            if (d < 8 || g->age > 240) {
                if (d < 8 && o->state != KS_DOWN && o->held < skid_capacity(o)) {
                    g->state = BG_HELD;
                    g->kind = BK_NORMAL;
                    if (o->held == 0) o->hold_t = 0;
                    o->held++;
                } else {
                    bag_stop(m, b);
                }
                return;
            }
            g->vx = dx / d * 3.4f;
            g->vy = dy / d * 3.4f;
        } else if (g->kind == BK_YOYO && g->age > 40) {
            g->phase = 1;
            g->hitmask = 0;
        }
        g->x += g->vx;
        g->y += g->vy;
        if (g->wavy && !g->dead) {
            /* a wiggle across the line of the throw */
            float sp = sqrtf(g->vx * g->vx + g->vy * g->vy);
            if (sp > 0.01f) {
                float nx = -g->vy / sp, ny = g->vx / sp;
                float w0 = sinf(g->wphase) * g->wamp;
                g->wphase += 0.2f;
                float w1 = sinf(g->wphase) * g->wamp;
                g->x += nx * (w1 - w0);
                g->y += ny * (w1 - w0);
            }
        }
        walls(m, g);
        if (!g->dead) bag_contacts(m, b);
        if (g->state != BG_SLIDE) return;
        float sp = sqrtf(g->vx * g->vx + g->vy * g->vy);
        if (g->friction > 0) {
            float ns = sp - g->friction * (g->dead ? 3 : 1);
            if (ns < STOP_SPEED) {
                bag_stop(m, b);
                return;
            }
            g->vx *= ns / sp;
            g->vy *= ns / sp;
        }
        return;
    }
    default: return;
    }
}

/* ------------------------------------------------------------------ */
/* waves, the sweeper, the gust                                         */

static void shove(Match *m, float x0, float x1, float y0, float y1, int dir) {
    for (int b = 0; b < SKID_MAX_BAGS; b++) {
        Bag *g = &m->bag[b];
        if (g->state != BG_LOOSE) continue;
        if (g->x < x0 || g->x > x1 || g->y < y0 || g->y > y1) continue;
        g->state = BG_SLIDE;
        g->dead = true;
        g->vx = dir * 1.4f;
        g->vy = 0;
    }
    for (int t = 0; t < SKID_MAX_ITEMS; t++) {
        Item *it = &m->it[t];
        if ((it->kind != IT_JUICE && it->kind != IT_MARBLE) || it->flying) continue;
        if (it->x < x0 || it->x > x1 || it->y < y0 || it->y > y1) continue;
        it->x = fclamp(it->x + dir * 10, SKID_LEFT + 4, SKID_RIGHT - 4);
    }
}

static void wave_update(Match *m, int w) {
    Wave *v = &m->wave[w];
    v->x += v->dir * 2.4f;
    Bag *g = &m->bag[v->bag];
    g->x = v->x;
    g->y = v->y;
    g->z = 3;
    shove(m, v->dir > 0 ? v->x : v->x - 3, v->dir > 0 ? v->x + 3 : v->x, v->y - v->half, v->y + v->half, v->dir);
    for (int i = 0; i < 4; i++) {
        Kid *k = &m->k[i];
        if (k->team == (unsigned)v->team) continue;
        if (fabsf(k->y - v->y) > v->half || fabsf(k->x - v->x) > 4) continue;
        if (k->z < 6) {
            if (!(v->hitmask & (1u << i))) {
                v->hitmask |= (uint8_t)(1u << i);
                score_on(m, i, v->team, true);
            }
        } else if (!(v->jumpmask & (1u << i)) && !(v->hitmask & (1u << i))) {
            v->jumpmask |= (uint8_t)(1u << i);
            add_half(m, i, 1);
        }
    }
    if (v->x <= SKID_LEFT + 2 || v->x >= SKID_RIGHT - 2) {
        v->live = false;
        g->x = fclamp(v->x, SKID_LEFT + 3, SKID_RIGHT - 3);
        bag_stop(m, v->bag);
        ev(m, EV_WALL, g->x, g->y);
    }
}

static void sweeper_update(Match *m) {
    Sweeper *s = &m->sweep;
    s->x += s->dir * 1.3f;
    /* it gathers every bag on the floor in front of it */
    for (int b = 0; b < SKID_MAX_BAGS; b++) {
        Bag *g = &m->bag[b];
        if (g->state != BG_LOOSE && !(g->state == BG_SLIDE && g->fuse == 0)) continue;
        float ahead = (g->x - s->x) * s->dir;
        if (ahead < -2 || ahead > 5) continue;
        bool have = false;
        for (int n = 0; n < s->n_swept; n++) have |= s->swept[n] == b;
        if (!have && s->n_swept < SKID_MAX_BAGS) s->swept[s->n_swept++] = b;
        g->state = BG_LOOSE;
        g->vx = g->vy = 0;
        g->x = s->x + s->dir * 5;
    }
    for (int n = 0; n < s->n_swept; n++) {
        Bag *g = &m->bag[s->swept[n]];
        if (g->state == BG_LOOSE) g->x = s->x + s->dir * 5;
    }
    /* and trips every kid of the other team standing in its way */
    for (int i = 0; i < 4; i++) {
        Kid *k = &m->k[i];
        if (k->team == (unsigned)s->team || skid_airborne(k) || (s->hitmask & (1u << i))) continue;
        if (fabsf(k->x - s->x) > 4) continue;
        s->hitmask |= (uint8_t)(1u << i);
        skid_knockdown(m, i, skid_down_time(k));
    }
    if ((s->dir > 0 && s->x >= SKID_MID) || (s->dir < 0 && s->x <= SKID_MID)) {
        /* at the line it lets go: the bags roll to the robot's side, and
         * some come out crushed */
        for (int n = 0; n < s->n_swept; n++) {
            Bag *g = &m->bag[s->swept[n]];
            if (g->state != BG_LOOSE) continue;
            if (rng_range(&m->rng, 0, 2) == 0) {
                g->state = BG_GONE;
                m->n_crushed++;
                continue;
            }
            g->x = SKID_MID + s->dir * 16;
            g->state = BG_SLIDE;
            g->dead = true;
            g->vx = s->dir * 1.2f;
            g->vy = 0;
        }
        s->live = false;
        ev(m, EV_CLANK, s->x, 110);
    }
}

static void gust_update(Match *m) {
    int dir = team_dir(m->gust_team);
    for (int b = 0; b < SKID_MAX_BAGS; b++) {
        Bag *g = &m->bag[b];
        if (g->state != BG_LOOSE && g->state != BG_SLIDE && g->state != BG_AIR) continue;
        if (g->fuse > 0) continue;
        if (g->state == BG_LOOSE) {
            g->state = BG_SLIDE;
            g->kind = BK_NORMAL;
            g->vx = dir * 0.5f; /* the wind gets it moving */
            g->vy = 0;
        }
        g->vx = fclamp(g->vx + dir * 0.16f, -3.4f, 3.4f);
        if (g->vx * dir > 0.8f && g->team != m->gust_team) {
            /* turned round: now it's the gust's team's throw */
            g->team = (int8_t)m->gust_team;
            g->hitmask = 0;
            g->jumpmask = 0;
            g->owner = -1;
        }
        if (g->vx * dir > 0.8f) g->dead = false;
    }
    for (int t = 0; t < SKID_MAX_ITEMS; t++) {
        Item *it = &m->it[t];
        if ((it->kind == IT_JUICE || it->kind == IT_MARBLE) && !it->flying)
            it->x = fclamp(it->x + dir * 0.8f, SKID_LEFT + 4, SKID_RIGHT - 4);
    }
}

/* ------------------------------------------------------------------ */
/* items                                                                */

static void item_update(Match *m, int n) {
    Item *it = &m->it[n];
    it->age++;
    if (it->flying) {
        it->x += it->vx;
        it->y += it->vy;
        it->z += it->vz;
        it->vz -= BAG_G;
        if (it->z > 0) return;
        it->z = 0;
        it->flying = false;
        if (it->kind == IT_BALLOON) {
            /* it bursts into a puddle; a kid under it goes down */
            for (int i = 0; i < 4; i++) {
                Kid *k = &m->k[i];
                if (fabsf(k->x - it->x) <= 9 && fabsf(k->y - it->y) <= 7 && k->z < 6) skid_knockdown(m, i, skid_down_time(k));
            }
            it->kind = IT_PUDDLE;
            it->r = 13;
            it->life = PUDDLE_LIFE;
            ev(m, EV_SPLASH, it->x, it->y);
            return;
        }
        if (it->kind == IT_JUICE) {
            /* a juice box on the head: down you go, but it's no point */
            for (int i = 0; i < 4; i++) {
                Kid *k = &m->k[i];
                if (fabsf(k->x - it->x) <= 6 && fabsf(k->y - it->y) <= 5 && k->z < 8) {
                    skid_knockdown(m, i, skid_down_time(k));
                    it->x = fclamp(it->x + (k->team == 0 ? 8 : -8), SKID_LEFT + 4, SKID_RIGHT - 4);
                    bark(m, BARK_WHOA);
                    break;
                }
            }
            it->life = 1500;
        }
        return;
    }
    if (it->life > 0 && --it->life == 0) it->kind = IT_NONE;
}

static void slips(Match *m) {
    for (int i = 0; i < 4; i++) {
        Kid *k = &m->k[i];
        if (!k->moved || skid_airborne(k) || k->state != KS_STAND || (k->pas & P_NOSLIP) || k->slip_cd > 0) continue;
        for (int n = 0; n < SKID_MAX_ITEMS; n++) {
            Item *it = &m->it[n];
            if (it->flying) continue;
            float dx = k->x - it->x, dy = k->y - it->y;
            if (it->kind == IT_PUDDLE && dx * dx + dy * dy * 2.5f < it->r * it->r) {
                skid_knockdown(m, i, skid_down_time(k));
                m->n_slip++;
                ev(m, EV_SLIP, k->x, k->y);
                break;
            }
            if (it->kind == IT_MARBLE && fabsf(dx) <= it->r + 2 && fabsf(dy) <= it->r) {
                /* a marble: a long, helpless slide; the marble is spent */
                skid_knockdown(m, i, 150);
                it->kind = IT_NONE;
                m->n_slip++;
                ev(m, EV_SLIP, k->x, k->y);
                break;
            }
        }
    }
}

/* running into a rival knocks them flat; the robot's push beats any other */
static void pushes(Match *m) {
    for (int i = 0; i < 4; i++) {
        Kid *a = &m->k[i];
        if (!(a->pas & P_PUSH) || !a->moved || skid_airborne(a) || a->state != KS_STAND) continue;
        for (int j = 0; j < 4; j++) {
            Kid *b = &m->k[j];
            if (b->team == a->team || skid_airborne(b)) continue;
            if (b->state == KS_DOWN || b->state == KS_WIN || b->state == KS_SAD) continue;
            if (fabsf(a->x - b->x) > 9 || fabsf(a->y - b->y) > 6) continue;
            int loser = j;
            if ((b->pas & P_BIGPUSH) && !(a->pas & P_BIGPUSH)) loser = i;
            skid_knockdown(m, loser, skid_down_time(&m->k[loser]));
            m->n_push++;
            ev(m, EV_PUSH, b->x, b->y);
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* the Coach                                                            */

void skid_coach_throw(Match *m, int kind, float tx, float ty) {
    const int T = 40;
    float x0 = SKID_COACH_X, y0 = SKID_COACH_Y + 2, z0 = 18;
    tx = fclamp(tx, SKID_LEFT + 8, SKID_RIGHT - 8);
    ty = fclamp(ty, SKID_TOP + 6, SKID_BOT - 6);
    m->coach_throw_t = 16;
    m->coach_items++;
    if (kind == 0) {
        int b = skid_bag_add(m);
        if (b < 0) return;
        Bag *g = &m->bag[b];
        g->state = BG_COACH;
        g->team = -1;
        g->x = x0;
        g->y = y0;
        g->z = z0;
        g->vx = (tx - x0) / T;
        g->vy = (ty - y0) / T;
        g->vz = (0.06f * T * T - z0) / T;
        m->n_coach_bag++;
    } else if (kind == 1) {
        int n = skid_item_add(m, IT_JUICE);
        if (n < 0) return;
        Item *it = &m->it[n];
        it->flying = true;
        it->from_coach = true;
        it->x = x0;
        it->y = y0;
        it->z = z0;
        it->vx = (tx - x0) / T;
        it->vy = (ty - y0) / T;
        it->vz = (0.5f * BAG_G * T * T - z0) / T;
        m->n_coach_juice++;
    } else {
        throw_balloon(m, x0, y0, tx, ty, -1, T);
        m->n_coach_balloon++;
    }
    ev(m, EV_COACH, x0, y0);
}

static void coach_update(Match *m) {
    if (m->coach_throw_t > 0) m->coach_throw_t--;
    if (m->bark_t > 0) m->bark_t--;
    if (m->state != MS_PLAY || m->coach_off) return;
    m->coach_t++;
    if (m->coach_t < m->coach_next) return;
    bool first = m->coach_items == 0;
    m->coach_next = m->coach_t + rng_range(&m->rng, 240, 480);
    int kind = skid_bags_total(m) < m->rules.max_bags && (first || rng_chance(&m->rng, 60)) ? 0 : 1;
    /* mostly onto the centre line */
    float tx = SKID_MID + (rng_chance(&m->rng, 70) ? rng_range(&m->rng, -10, 10) : rng_range(&m->rng, -70, 70));
    float ty = (float)rng_range(&m->rng, SKID_TOP + 14, SKID_BOT - 14);
    if (first) {
        tx = SKID_MID;
        ty = 110;
        bark(m, BARK_PLAY);
    }
    if (m->favored >= 0 && !first) {
        /* the Coach's pet: his team gets the items, and now and then the
         * other team gets a water balloon */
        if (rng_chance(&m->rng, 25)) {
            int foe = 1 - m->favored;
            const Kid *k = &m->k[foe * 2 + rng_range(&m->rng, 0, 1)];
            skid_coach_throw(m, 2, k->x, k->y);
            bark(m, BARK_FAIR);
            return;
        }
        if (rng_chance(&m->rng, 70)) tx = (float)(SKID_MID - team_dir(m->favored) * rng_range(&m->rng, 20, 60));
    }
    skid_coach_throw(m, kind, tx, ty);
    if (!first) bark(m, kind == 0 ? BARK_HEADS : BARK_DRINK);
}

/* ------------------------------------------------------------------ */
/* the frame                                                            */

void skid_match_update(Match *m, const Pad pads[4]) {
    m->frame++;
    m->state_t++;
    if (m->quake_t > 0) m->quake_t--;
    if (m->state == MS_READY && m->state_t >= 60) {
        m->state = MS_PLAY;
        m->state_t = 0;
        ev(m, EV_WHISTLE, SKID_COACH_X, SKID_COACH_Y);
    }
    for (int i = 0; i < 4; i++) kid_update(m, i, &pads[i]);
    if (m->state == MS_PLAY) {
        pushes(m);
        slips(m);
    }
    for (int w = 0; w < SKID_MAX_WAVES; w++)
        if (m->wave[w].live) wave_update(m, w);
    if (m->sweep.live) sweeper_update(m);
    if (m->gust_t > 0) {
        m->gust_t--;
        gust_update(m);
    }
    for (int b = 0; b < SKID_MAX_BAGS; b++)
        if (m->bag[b].state != BG_GONE) bag_update(m, b);
    for (int n = 0; n < SKID_MAX_ITEMS; n++)
        if (m->it[n].kind != IT_NONE) item_update(m, n);
    coach_update(m);
    /* jumping kids clear what they jumped: note good jumps for the Coach */
    if ((m->ev & EV_HALFSTAR) && rng_chance(&m->rng, 12)) bark(m, BARK_JUMP);
    if (m->state == MS_PLAY && !m->endless) {
        for (int t = 0; t < 2; t++)
            if (m->score[t] >= m->rules.goal) {
                m->state = MS_OVER;
                m->state_t = 0;
                m->winner = t;
                bark(m, BARK_GAME);
                ev(m, EV_WHISTLE, SKID_COACH_X, SKID_COACH_Y);
                for (int i = 0; i < 4; i++) {
                    Kid *k = &m->k[i];
                    cancel_actions(k);
                    k->state = k->team == t ? KS_WIN : KS_SAD;
                    k->t = 0;
                }
                break;
            }
    }
    if (m->state == MS_PLAY && m->state_t % 900 == 450 && rng_chance(&m->rng, 50)) bark(m, BARK_HUSTLE);
}
