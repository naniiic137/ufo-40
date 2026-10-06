/* TUSKWIND - the demo player, for the tests. It plays with the buttons
 * alone: on each islet it walks to whatever it should touch there (a
 * lighthouse, a chest it has a key for, a bell in a wind, the door), then
 * picks its next jump by trying aims and charges through the game's own
 * physics, walks to the spot, swings the aim to it, holds A for exactly
 * that charge and lets go; flapping only if no plain jump will do. In
 * "collect" mode it also goes out of its way for shells, and in the hall
 * it cashes in its terns, climbs the shaft and talks to the old one. */
#include "tuskwind.h"

int tkw_bot_plans, tkw_bot_fails;
int tkw_bot_dbg[4];

enum { PH_NONE, PH_WALK, PH_TURN, PH_AIM, PH_CHARGE, PH_AIR };

typedef struct {
    int target;
    float stand;
    int face;
    float aim;
    int charge;
    int f0, flen, fdx; /* flap from frame f0 for flen frames, the pad that way (fdx -1, 0, 1) and up */
    float score;
} Plan;

static Plan plan;
#define HIST 8
static int hist[HIST], hist_n;
static int phase, air_k, wait_t, last_mask, last_plat_fail, fail_streak;
static bool shaft_done;

void tkw_bot_reset(void) {
    phase = PH_NONE;
    air_k = 0;
    wait_t = 0;
    last_mask = 0;
    last_plat_fail = -1;
    fail_streak = 0;
    shaft_done = false;
    for (int k = 0; k < HIST; k++) hist[k] = -1;
    hist_n = 0;
    tkw_bot_plans = tkw_bot_fails = 0;
}

/* a fresh press: never the same button two frames running */
static int tap(int m) { return (last_mask & m) ? 0 : m; }

/* ---- the simulation -------------------------------------------------------- */

#define MAXP 48
static int pk_n;
static float pk_x[MAXP], pk_y[MAXP];
static int pk_v[MAXP];
static int bl_n;
static float bl_x[8], bl_y[8];

static void gather_pickups(float x0, float x1) {
    pk_n = 0;
    bl_n = 0;
    for (int i = 0; i < tkw_w.nth && bl_n < 8; i++) {
        const TkwThing *t = &tkw_w.th[i];
        if (t->alive && t->kind == TH_BELL && !t->state && t->x >= x0 && t->x <= x1) {
            bl_x[bl_n] = t->x;
            bl_y[bl_n] = t->y - 10;
            bl_n++;
        }
    }
    for (int i = 0; i < tkw_w.nth && pk_n < MAXP; i++) {
        const TkwThing *t = &tkw_w.th[i];
        if (!t->alive || t->x < x0 || t->x > x1) continue;
        int v = 0;
        switch (t->kind) {
        case TH_COCKLE: v = 1; break;
        case TH_WHELK: v = 4; break;
        case TH_KEY: v = 6; break;
        case TH_SPRAT: v = tkg.h.b.stamina < TKW_STAMINA / 2 ? 2 : 0; break;
        default: break;
        }
        if (!v) continue;
        pk_x[pk_n] = t->x;
        pk_y[pk_n] = t->y;
        pk_v[pk_n] = v;
        pk_n++;
    }
}

typedef struct { int plat; float x; int frames, value, stam; } SimOut;

static bool simulate(float stand, int face, float aim, int charge, int f0, int flen, int fdx, SimOut *o) {
    const TkwHero *h = &tkg.h;
    TkwBody s = h->b;
    s.x = stand;
    s.mode = BM_GROUND;
    s.landed = -1;
    tkw_body_launch(&s, &tkw_w, face, aim, charge);
    uint64_t got = 0;
    o->value = 0;
    float wind = (float)tkg.wind;
    for (int k = 0; k < 420; k++) {
        if (s.mode == BM_LUNGE) {
            tkw_body_ground(&s, &tkw_w, 0);
            if (s.mode == BM_GROUND) break;
            continue;
        }
        if (s.mode == BM_GROUND) break;
        bool flap = k >= f0 && k < f0 + flen;
        tkw_body_air(&s, &tkw_w, flap ? (float)fdx : 0, flap ? -1.0f : 0, flap, wind);
        if (s.y - TKW_BH < TKW_MANTA_Y + 14 && !tkg.in_hall) return false; /* the mantas' band */
        if (s.y > tkw_w.sea_y) return false;
        for (int p = 0; p < bl_n; p++)
            if (fabsf(bl_x[p] - s.x) < 11 && fabsf(bl_y[p] - (s.y - 6)) < 13) {
                if (!tkg.wind) return false; /* would start a wind */
                o->value += 8;
            }
        for (int p = 0; p < pk_n; p++)
            if (!(got >> p & 1) && fabsf(pk_x[p] - s.x) < 16 && fabsf(pk_y[p] - (s.y - 6)) < 16) {
                got |= 1ull << p;
                o->value += pk_v[p];
            }
        if (s.mode == BM_GROUND) { o->frames = k; break; }
        o->frames = k;
    }
    if (s.mode != BM_GROUND) return false;
    o->plat = s.plat;
    o->x = s.x;
    o->stam = s.stamina;
    return true;
}

static bool ram_on(int plat) {
    for (int i = 0; i < tkw_w.nth; i++) {
        const TkwThing *t = &tkw_w.th[i];
        if (t->alive && t->kind == TH_RAM && t->plat == plat && t->state != RAM_FALL) return true;
    }
    return false;
}

static bool thing_on(int plat, int kind, int state) {
    for (int i = 0; i < tkw_w.nth; i++) {
        const TkwThing *t = &tkw_w.th[i];
        if (t->alive && t->kind == kind && t->plat == plat && t->state == state) return true;
    }
    return false;
}

/* frames before the islet under him goes (a crumbling foam, a ram) */
static int time_left(void) {
    const TkwBody *b = &tkg.h.b;
    int left = 100000;
    const TkwPlat *p = &tkw_w.plat[b->plat];
    if (p->kind == PK_FOAM && p->timer >= 0) left = TKW_FOAM_T - p->timer;
    for (int i = 0; i < tkw_w.nth; i++) {
        const TkwThing *t = &tkw_w.th[i];
        if (!t->alive || t->kind != TH_RAM || t->plat != b->plat) continue;
        if (t->state == RAM_ALERT) left = imin(left, 40 - t->t + (int)(fabsf(t->x - b->x) / 2.0f));
        if (t->state == RAM_CHARGE) left = imin(left, (int)(fabsf(t->x - b->x) / 2.0f) - 4);
    }
    return left;
}

static float cand_score(int i, const SimOut *o, int frames, int flap) {
    const TkwPlat *p = &tkw_w.plat[i];
    const TkwPlat *cur = &tkw_w.plat[tkg.h.b.plat];
    float gain = (p->x + p->w / 2.0f) - (cur->x + cur->w / 2.0f);
    float sc = gain < 0 ? gain * 3 : gain;
    for (int k = 0; k < HIST; k++)
        if (hist[k] == i) sc -= 150; /* not back and forth */
    if (tkg.in_hall) sc = 0;
    if (ram_on(i)) sc -= 600;
    if (p->kind == PK_FOAM) sc -= 40;
    if (p->door) sc += 2000;
    if (thing_on(i, TH_LIGHTHOUSE, 0)) sc += 260;
    if (tkg.keys > 0 && thing_on(i, TH_CHEST, 0)) sc += tkg.bot_collect ? 400 : 60;
    if (tkg.wind && thing_on(i, TH_BELL, 0)) sc += 180;
    sc += o->value * (tkg.bot_collect ? 40.0f : 4.0f);
    sc -= frames * 0.15f;
    sc -= flap * 0.6f;
    /* land well inside the islet */
    float m = fminf(o->x - p->x, p->x + p->w - o->x);
    sc += fminf(m, 12.0f) * 2.0f;
    return sc;
}

/* one try: a jump with a flap policy; kept in best if it scores higher */
static void try_jump(Plan *best, int i, float st, int face, float aim, int c, int f0, int flen, int fdx, int frames) {
    const TkwPlat *p = &tkw_w.plat[i];
    SimOut o;
    if (!simulate(st, face, aim, c, f0, flen, fdx, &o) || o.plat != i) return;
    if (o.x < p->x + 3 || o.x > p->x + p->w - 3) return;
    float sc = cand_score(i, &o, frames + o.frames, flen);
    if (sc <= best->score) return; /* can't win */
    /* a near miss either side is a risky plan */
    SimOut o2;
    int bad = 0;
    if (c > 4 && !(simulate(st, face, aim, c - 2, f0, flen, fdx, &o2) && o2.plat == i)) bad++;
    if (c < TKW_CHARGE_T && !(simulate(st, face, aim, c + 2, f0, flen, fdx, &o2) && o2.plat == i)) bad++;
    sc -= bad * 90;
    if (sc <= best->score) return;
    best->score = sc;
    best->target = i;
    best->stand = st;
    best->face = face;
    best->aim = aim;
    best->charge = c;
    best->f0 = f0;
    best->flen = flen;
    best->fdx = fdx;
}

/* would walking from x to st ring a bell while it is calm? */
static bool rings_bell(float x, float st) {
    if (tkg.wind) return false;
    const TkwBody *b = &tkg.h.b;
    for (int i = 0; i < tkw_w.nth; i++) {
        const TkwThing *t = &tkw_w.th[i];
        if (!t->alive || t->kind != TH_BELL || t->state || t->plat != b->plat) continue;
        if (t->x > fminf(x, st) - 11 && t->x < fmaxf(x, st) + 11) return true;
    }
    return false;
}

/* the best jump from where he stands; false if none */
static bool make_plan(int max_frames) {
    const TkwHero *h = &tkg.h;
    const TkwBody *b = &h->b;
    const TkwPlat *cur = &tkw_w.plat[b->plat];
    tkw_bot_plans++;
    gather_pickups(b->x - 80, b->x + 420);
    int cands[24], nc = 0;
    for (int i = 0; i < tkw_w.nplat && nc < 24; i++) {
        const TkwPlat *p = &tkw_w.plat[i];
        if (i == b->plat || p->gone) continue;
        float cx = p->x + p->w / 2.0f;
        if (cx < b->x - 200 || cx > b->x + 340) continue;
        if (p->kind == PK_FOAM && p->timer >= 0) continue;
        cands[nc++] = i;
    }
    Plan best;
    best.score = -1e9f;
    best.target = -1;
    float stands[4];
    int ns = 0;
    stands[ns++] = b->x;
    if (max_frames > 200) {
        float more[3] = {cur->x + 4.0f, cur->x + cur->w - 4.0f, cur->x + cur->w / 2.0f};
        for (int k = 0; k < 3; k++)
            if (!rings_bell(b->x, more[k])) stands[ns++] = more[k];
    }
    /* plain jumps first */
    for (int ci = 0; ci < nc; ci++) {
        int i = cands[ci];
        const TkwPlat *p = &tkw_w.plat[i];
        for (int si = 0; si < ns; si++) {
            float st = stands[si];
            int face = p->x + p->w / 2.0f >= st ? 1 : -1;
            if (p->x <= st && st <= p->x + p->w) face = h->face;
            int walk = (int)(fabsf(st - b->x) / TKW_WALK) + 2;
            for (int ai = 0; ai <= 29; ai++) {
                float aim = ai * 3.0f;
                int aimf = (int)(fabsf(aim - h->aim) / TKW_AIM_STEP);
                for (int c = 4; c <= TKW_CHARGE_T; c += 4) {
                    int frames = walk + aimf + c + 2;
                    if (frames > max_frames) break;
                    try_jump(&best, i, st, face, aim, c, 0, 0, 0, frames);
                }
            }
        }
    }
    /* nothing that goes forward: flapping too, a coarser search */
    if (best.target < 0 || best.score < 0) {
        static const int FL[3] = {50, 110, 200}, F0[2] = {0, 20};
        for (int ci = 0; ci < nc; ci++) {
            int i = cands[ci];
            const TkwPlat *p = &tkw_w.plat[i];
            for (int si = 0; si < imin(ns, 2); si++) {
                float st = stands[si];
                int face = p->x + p->w / 2.0f >= st ? 1 : -1;
                if (p->x <= st && st <= p->x + p->w) face = h->face;
                int walk = (int)(fabsf(st - b->x) / TKW_WALK) + 2;
                for (int fl = 0; fl < 3; fl++) {
                    if (FL[fl] * TKW_FLAP_COST(b->kite) > b->stamina) continue;
                    for (int f0 = 0; f0 < 2; f0++)
                        for (int fdx = -1; fdx <= 1; fdx++)
                            for (int ai = 0; ai <= 14; ai++) {
                                float aim = ai * 6.0f;
                                int aimf = (int)(fabsf(aim - h->aim) / TKW_AIM_STEP);
                                for (int c = 4; c <= TKW_CHARGE_T; c += 8) {
                                    int frames = walk + aimf + c + 2;
                                    if (frames > max_frames) break;
                                    try_jump(&best, i, st, face, aim, c, F0[f0], FL[fl], fdx, frames);
                                }
                            }
                }
            }
        }
    }
    if (best.target < 0) return false;
    plan = best;
    return true;
}

/* ---- the chores on the islet he is on --------------------------------------- */

/* where on this islet he should walk before jumping (NAN: nowhere) */
static float chore_x(void) {
    const TkwBody *b = &tkg.h.b;
    if (b->mode != BM_GROUND) return NAN;
    for (int i = 0; i < tkw_w.nth; i++) {
        const TkwThing *t = &tkw_w.th[i];
        if (!t->alive || t->plat != b->plat) continue;
        if (t->kind == TH_LIGHTHOUSE && t->state == 0) return t->x;
        if (t->kind == TH_CHEST && t->state == 0 && tkg.keys > 0) return t->x;
        if (t->kind == TH_BELL && t->state == 0 && tkg.wind) return t->x;
        if (t->kind == TH_ELDER) return t->x - 12;
    }
    if (!tkg.in_hall && b->plat == tkw_w.goal_plat) {
        const TkwPlat *p = &tkw_w.plat[b->plat];
        return p->x + p->w - 24.0f;
    }
    return NAN;
}

static int walk_to(float x) {
    const TkwBody *b = &tkg.h.b;
    if (x > b->x + 0.35f) return BTN_RIGHT;
    if (x < b->x - 0.35f) return BTN_LEFT;
    return 0;
}

/* ---- the hall --------------------------------------------------------------- */

static int hall_buttons(void) {
    const TkwHero *h = &tkg.h;
    const TkwBody *b = &h->b;
    if (b->mode == BM_AIR) {
        /* up the shaft: flap straight up while rising */
        if (phase == PH_AIR && plan.target == -2) {
            air_k++;
            bool more = false;
            for (int i = 0; i < tkw_w.nth; i++)
                more |= tkw_w.th[i].alive && tkw_w.th[i].kind == TH_COCKLE && tkw_w.th[i].y < b->y - 6;
            return more && b->stamina > 0 ? BTN_A | BTN_UP : 0;
        }
        return -1; /* the ordinary flight code */
    }
    if (b->mode != BM_GROUND || h->charging) return -1;
    /* 1: west, to the perched terns */
    bool perched = false, falling = false;
    for (int i = 0; i < tkw_w.nth; i++) {
        const TkwThing *t = &tkw_w.th[i];
        if (!t->alive) continue;
        if (t->kind == TH_PERCH && t->state == 0) perched = true;
        if (t->kind == TH_SPIRAL) falling = true;
    }
    if (perched || falling) {
        if (b->plat != tkw_w.start_plat) return -1;
        return walk_to(TKW_HALL_EDGE + (perched ? 14.0f : 40.0f));
    }
    /* 2: the shaft, if there is stamina for it */
    if (tkg.bot_collect && !shaft_done && b->plat == tkw_w.start_plat && b->stamina >= 200) {
        if (fabsf(b->x - TKW_HALL_SHAFT) > 0.35f) return walk_to(TKW_HALL_SHAFT);
        if (h->aim < 90) return BTN_UP;
        plan.target = -2;
        plan.charge = TKW_CHARGE_T;
        shaft_done = true;
        phase = PH_CHARGE;
        return tap(BTN_A);
    }
    /* 3: the old one */
    if (b->plat == tkw_w.start_plat) {
        float ex = chore_x();
        if (!isnan(ex)) return walk_to(ex);
    }
    return -1;
}

/* ---- the journey ---------------------------------------------------------- */

static int journey_buttons(void) {
    const TkwHero *h = &tkg.h;
    const TkwBody *b = &h->b;
    switch (h->ps) {
    case PS_SHOP: return tap(BTN_B);       /* never buys: every shell counts */
    case PS_MENU: return 0;
    case PS_THROW: return tap(BTN_B);
    case PS_LOOK: return tap(BTN_B);
    case PS_PLAY: break;
    default: phase = PH_NONE; return 0;
    }
    if (tkg.in_hall) {
        if (phase == PH_CHARGE && plan.target == -2) {
            if (h->charging && h->charge < plan.charge) return BTN_A;
            if (h->charging) { phase = PH_AIR; air_k = 0; return 0; }
        }
        int m = hall_buttons();
        if (m >= 0) return m;
    }
    if (b->mode == BM_AIR) {
        if (phase != PH_AIR) {
            /* knocked or slid off: flap for the nearest islet below */
            phase = PH_NONE;
            if (b->vy > 0.5f && b->stamina > 0 && b->y > tkw_w.sea_y - 120) {
                float bd = 1e9f, tx = b->x;
                for (int i = 0; i < tkw_w.nplat; i++) {
                    const TkwPlat *p = &tkw_w.plat[i];
                    if (p->gone || p->y < b->y - 20) continue;
                    float cx = p->x + p->w / 2.0f, d = fabsf(cx - b->x) + (p->y - b->y);
                    if (d < bd) { bd = d; tx = cx; }
                }
                int m = BTN_A | BTN_UP;
                if (tx > b->x + 6) m |= BTN_RIGHT;
                if (tx < b->x - 6) m |= BTN_LEFT;
                return m;
            }
            return 0;
        }
        int k = air_k++;
        if (k >= plan.f0 && k < plan.f0 + plan.flen) {
            int m = BTN_A | BTN_UP;
            if (plan.fdx > 0) m |= BTN_RIGHT;
            if (plan.fdx < 0) m |= BTN_LEFT;
            return m;
        }
        return 0;
    }
    if (b->mode == BM_RIDE) {
        /* off the manta: straight down */
        if (h->charging) return h->charge < 20 ? BTN_A : 0;
        if (h->aim > 0) return BTN_DOWN;
        return tap(BTN_A);
    }
    if (b->mode == BM_LUNGE) {
        /* a planned flat jump slides first (the sim counts those frames too) */
        if (phase == PH_AIR) air_k++;
        else phase = PH_NONE;
        return 0;
    }
    /* on an islet */
    if (phase == PH_AIR || hist[(hist_n + HIST - 1) % HIST] != b->plat) {
        hist[hist_n++ % HIST] = b->plat;
    }
    if (phase == PH_AIR) {
        phase = PH_NONE;
        if (plan.target >= 0 && b->plat != plan.target) tkw_bot_fails++;
    }
    int left = time_left();
    if (phase == PH_NONE) {
        float cx = chore_x();
        if (!isnan(cx) && left > 300) {
            int m = walk_to(cx);
            if (m) return m;
            if (!tkg.in_hall && b->plat == tkw_w.goal_plat) return 0; /* the door takes him */
        }
        if (h->charging) return 0;
        if (!make_plan(left - 4)) {
            /* nothing: wait out a dying wind, else leap and trust the terns */
            if (tkg.wind && tkg.wind_t > 0 && left > 1000) return 0;
            tkw_bot_fails++;
            plan.target = -1;
            plan.stand = b->x;
            plan.face = 1;
            plan.aim = 45;
            plan.charge = 40;
            plan.flen = b->stamina / 2;
            plan.f0 = 10;
            plan.fdx = 1;
        }
        phase = PH_WALK;
    }
    if (phase == PH_WALK) {
        int m = walk_to(plan.stand);
        if (m) return m;
        phase = PH_TURN;
    }
    if (phase == PH_TURN) {
        if (h->face != plan.face) return plan.face > 0 ? BTN_RIGHT : BTN_LEFT;
        phase = PH_AIM;
    }
    if (phase == PH_AIM) {
        if (h->aim < plan.aim - 0.01f) return BTN_UP;
        if (h->aim > plan.aim + 0.01f) return BTN_DOWN;
        /* check it once more from exactly where he stands */
        if (plan.target >= 0) {
            SimOut o;
            gather_pickups(b->x - 80, b->x + 420);
            if (!simulate(b->x, h->face, h->aim, plan.charge, plan.f0, plan.flen, plan.fdx, &o) || o.plat != plan.target) {
                plan.stand = b->x;
                if (!make_plan(imin(left - 4, 120))) { phase = PH_NONE; return 0; }
                phase = PH_TURN;
                return 0;
            }
        }
        phase = PH_CHARGE;
        return tap(BTN_A);
    }
    if (phase == PH_CHARGE) {
        if (!h->charging) return tap(BTN_A);
        if (h->charge < plan.charge) return BTN_A;
        phase = PH_AIR;
        air_k = 0;
        return 0; /* let go: the jump */
    }
    return 0;
}

int tkw_bot_buttons(void) {
    int m = 0;
    tkw_bot_dbg[0] = phase;
    tkw_bot_dbg[1] = plan.target;
    tkw_bot_dbg[2] = plan.charge;
    tkw_bot_dbg[3] = (int)plan.aim;
    switch (tkg.state) {
    case TS_TITLE: m = tkg.sel == 0 ? tap(BTN_A) : tap(BTN_UP); break;
    case TS_INTRO:
    case TS_TALK:
    case TS_WAKE: m = tap(BTN_A); break;
    case TS_ENDING: m = tap(BTN_A); break;
    case TS_CREDITS: m = BTN_A; break;
    case TS_JOURNEY:
    case TS_HALL: m = journey_buttons(); break;
    default: m = 0; break;
    }
    last_mask = m;
    return m;
}
