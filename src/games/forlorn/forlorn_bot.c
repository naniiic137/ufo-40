/* FORLORN HOPE - the demo player. It plays a route, volunteer by volunteer:
 * each one's plan is a line of commands, and the bot turns them into the
 * buttons for each frame, which the tests then press for real.
 *
 *   C<n>    at the door: pick trade n (0 mason, 1 hunter, 2 runner, 3 tinker, 4 sapper)
 *   > < =   hold right, hold left, hold neither (kept until changed)
 *   X<px>   walk the held way until the volunteer's middle reaches px
 *   W<n>    wait n frames             A<n>   hold A (jump) n frames
 *   G       wait until on the ground  Y<py> / y<py>  wait until middle y <= / >= py
 *   B<n>    hold B n frames, then let go (50 or more: give yourself up)
 *   H / h   start / stop holding B (a death while held sets the gift off)
 *   T<n>    attack n times             K<i>  attack until foe i is dead
 *   U / D   press up / down once       F<i>  wait until foe i is dead
 *   S<n>    wait until plate n is down
 *   M<k>    a mason's duel with the first foe of kind k: keep just out of its
 *           reach, swing whenever it is in range, and strike anything else
 *           deadly that comes close first, until it is dead
 *   N<n>    stand guard n frames, striking anything deadly that comes close
 *   Z<n>    jump (A held n frames) and swing near the top of it
 *   Q<n>    swing at heart n (0 upper left, 1 upper right, 2 lower left,
 *           3 lower right) until it bursts
 *   f<n>    stand n frames, facing the held way, attacking whatever comes
 *   @<px>   line up on px (walk either way, stop there)
 *   a<n>    hold A n frames, straight up at first, then drifting the held way
 *   k<px>   fight the way to px: walk the held way, but stop and attack
 *           whatever deadly thing is just ahead (swing range, or shot range)
 * Every wait gives up after a while and marks the bot stuck. */
#include "forlorn.h"

int frl_bot_debug;

/* the route, one line per volunteer; filled in once the map is played through */
static const char *const PLAN[] = {
    /* 1 mason: walks off the camp's edge holding B, and dies in the spike
     * pit as a stone */
    "C0 H > X400",
    /* 2 mason: lands on that stone, takes key 1, and stones plate 1 */
    "C0 > X241 G = W4 > A16 G A16 G X372 G k570 = W8 H W60",
    /* 3 runner: up the Old Yew with double jumps, along the canopy, and a
     * waystone beside plate 2 */
    "C2 > X169 = W10 A12 W2 > A16 W4 G = < X182 = W10 A12 W2 < A16 W4 G = > X165 = W10 A12 W2 > A16 W4 G > X215 = W10 A12 W2 > A16 W4 G > k386 A14 G k476 A12 G X560 = W8 H W50",
    /* 4 mason: through the waystone, and a stone on plate 2 */
    "C0 > X112 = W8 U W30 G < X546 = W8 H W50",
    /* 5 sapper: down the pit (onto the stone), east, and the loose rock
     * goes up with them */
    "C4 > X241 G = W4 > A16 G A16 G X372 G X452 = W6 H W50",
    /* 6 runner: down through the broken rock, over the bridge plate 1
     * raised and up the steps plate 2 raised, to a waystone by the gulper */
    "C2 > X241 G = W4 > A16 G A16 G X372 G X452 = W8 y500 G > X462 A14 G A12 G k586 X596 A14 G A14 G A14 G X693 = W8 H W50",
    /* 7 mason: through the waystone; a stone on plate 3 */
    "C0 > X112 = W8 U W30 G < X685 = W8 H W50",
    /* 8 sapper: through the waystone, and into the gulper's mouth */
    "C4 > X112 = W8 U W30 G > X720",
    /* 9 mason: down the gulper's shaft onto the bridge plate 3 raised; a
     * mallet duel with the stingback (keeping just out of its reach), then
     * up the tower striking the wall-eye and the bell, and a stone over the
     * drain at the top */
    "C0 > X112 = W8 U W30 G > X705 y720 G M16 N240 k1082 A14 G < k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G < k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G N200 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 N40 > X1100 G X1155 G = W4 H W50",
    /* 10 runner: up the tower too, and a waystone at the top */
    "C2 > X112 = W8 U W30 G > X705 y720 G k1040 k1082 A14 G < k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G < k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G > X1152 = W8 H W50",
    /* 11-13 sappers: through the waystone, and the seal goes, two columns a blast */
    "C4 > X112 = W8 U W30 G = W4 H W50",
    "C4 > X112 = W8 U W30 G > X1175 = W6 H W50",
    "C4 > X112 = W8 U W30 G > X1195 = W6 H W50",
    /* 14-15 sappers: down the walkway's west hole to the upper left heart */
    "C4 > X112 = W8 U W30 G > X1281 = y390 G H > X1340",
    "C4 > X112 = W8 U W30 G > X1281 = y390 G H > X1340",
    /* 16-17 sappers: over the west hole, down the east one, the upper right heart */
    "C4 > X112 = W8 U W30 G > X1268 A14 G X1515 = y390 G H < X1460",
    "C4 > X112 = W8 U W30 G > X1268 A14 G X1515 = y390 G H < X1460",
    /* 18-19 sappers: the upper left platform, off it and back under, the lower left heart */
    "C4 > X112 = W8 U W30 G > X1281 = y390 G < X1272 y410 G H > X1340",
    "C4 > X112 = W8 U W30 G > X1281 = y390 G < X1272 y410 G H > X1340",
    /* 20-21 sappers: the upper right platform, off it and back under, the lower right heart */
    "C4 > X112 = W8 U W30 G > X1268 A14 G X1515 = y390 G > X1545 y410 G H < X1460",
    "C4 > X112 = W8 U W30 G > X1268 A14 G X1515 = y390 G > X1545 y410 G H < X1460",
    NULL,
};

/* the castle way (a route test runs it with the foes sent away, to show
 * the map can be crossed that way too): key 1, a stone over the camp's
 * drop, the meadow, the gate, the courtyard's key behind the idol, hall
 * 1's key, plate 4 held by a stone, the bridge it raises, the key past the
 * bloater, the dungeon, the tower's landing and out through door 4 to the
 * heart chamber */
/* over the drop on the stone, and over the meadow's outcrop and hole */
#define TO_MEADOW "> X233 A10 G X247 A16 G X430 A16 G X512 A16 G "
static const char *const PLAN_CASTLE[] = {
    /* the spike pit stoned, key 1 taken (keys stay counted for everyone) */
    "C0 H > X400",
    "C0 > X241 G = W4 > A16 G A16 G X372 G = W4 H W50",
    /* a mason walks off the camp's edge and turns to stone over the drop */
    "C0 H > X243 h",
    "C0 " TO_MEADOW "X640 A14 G X950 A14 G < X962 A14 G X922 A14 G X882 A14 G X765 > X1000 y300 G "
    "< X884 A14 G X845 X776 y390 G @775 = W4 H W50",
    "C4 " TO_MEADOW "X640 A14 G X1000 y300 G < X776 W4 G > A12 G X800 y430 G A14 G X939 G X995 A12 G < X1012 y560 G > "
    "X1070 y590 G > X1118 A14 G X1250",
    NULL,
};
/* the expert's way (a test, not the demo): the tower route with as few
 * volunteers as it takes. The first eight are the demo's; then one mason
 * duels the stingback, climbs the tower, strikes the wall-eye and the bell
 * and caps the drain; three sappers open the seal; and a last mason bursts
 * all four hearts with the mallet and lives. */
static const char *const PLAN_EXPERT[] = {
    /* 1 mason: walks off the camp's edge holding B, and dies in the spike
     * pit as a stone */
    "C0 H > X400",
    /* 2 mason: lands on that stone, takes key 1, and stones plate 1 */
    "C0 > X241 G = W4 > A16 G A16 G X372 G k570 = W8 H W60",
    /* 3 runner: up the Old Yew with double jumps, along the canopy, and a
     * waystone beside plate 2 */
    "C2 > X169 = W10 A12 W2 > A16 W4 G = < X182 = W10 A12 W2 < A16 W4 G = > X165 = W10 A12 W2 > A16 W4 G > X215 = W10 A12 W2 > A16 W4 G > k386 A14 G k476 A12 G X560 = W8 H W50",
    /* 4 mason: through the waystone, and a stone on plate 2 */
    "C0 > X112 = W8 U W30 G < X546 = W8 H W50",
    /* 5 sapper: down the pit (onto the stone), east, and the loose rock
     * goes up with them */
    "C4 > X241 G = W4 > A16 G A16 G X372 G X452 = W6 H W50",
    /* 6 runner: down through the broken rock, over the bridge plate 1
     * raised and up the steps plate 2 raised, to a waystone by the gulper */
    "C2 > X241 G = W4 > A16 G A16 G X372 G X452 = W8 y500 G > X462 A14 G A12 G k586 X596 A14 G A14 G A14 G X693 = W8 H W50",
    /* 7 mason: through the waystone; a stone on plate 3 */
    "C0 > X112 = W8 U W30 G < X685 = W8 H W50",
    /* 8 sapper: through the waystone, and into the gulper's mouth */
    "C4 > X112 = W8 U W30 G > X720",
    "C0 > X112 = W8 U W30 G > X705 y720 G M16 N240 k1082 A14 G < k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G < k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G N200 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 < k1058 = N60 < W1 = Z12 N40 > X1100 G X1155 G = W4 H W50",
    "C4 > X112 = W8 U W30 G > X705 y720 G X1082 A14 G < X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G < X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G > X1118 A14 G X1155 = W4 H W50",
    "C4 > X112 = W8 U W30 G > X705 y720 G X1082 A14 G < X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G < X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G > X1118 A14 G X1175 = W6 H W50",
    "C4 > X112 = W8 U W30 G > X705 y720 G X1082 A14 G < X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G < X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G X1118 A14 G < X1131 A14 G X1091 A14 G > X1078 A14 G > X1118 A14 G X1195 = W6 H W50",
    "C0 > X112 = W8 U W30 G > X705 y720 G k1040 k1082 A14 G < k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G < k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G k1118 A14 G < k1131 A14 G k1091 A14 G > k1078 A14 G > k1118 A14 G X1281 = y390 G @1305 > W1 = Q0 < X1279 y410 G > X1281 = y490 G @1305 > W1 = Q2 < X1265 y515 G > X1300 y570 G X1330 A14 G X1357 A14 G X1387 A14 G X1417 A14 G X1447 A14 G X1490 G X1552 A14 G @1563 < a16 G @1557 > a16 G > X1577 < X1561 A14 G @1513 < W1 = Q3 < X1490 > X1546 A14 G @1563 < a16 G @1557 > a16 G @1563 < a16 G @1543 < a16 G @1513 < W1 = Q1",
    NULL,
};
/* the bottom-right way (a test puts the volunteers at its foot, with the
 * foes away and the three plates held): a sapper climbs the chimney, whose
 * ledges the plates raise, and blows the heart chamber's east wall open;
 * the next walks in */
static const char *const PLAN_CHIMNEY[] = {
    "C4 W40 > X1702 A14 G < X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G > X1698 A14 G X1738 A14 G < X1751 A14 G X1711 A14 G < X1674 A14 G X1606 = W4 H W50",
    "C0 W40 < X1500",
    NULL,
};
static const char *const *plans = PLAN;
static int nplans = ARRAY_LEN(PLAN);

void frl_bot_route(int r) {
    plans = r == 1 ? PLAN_CASTLE : r == 2 ? PLAN_EXPERT : r == 3 ? PLAN_CHIMNEY : PLAN;
    nplans = r == 1 ? ARRAY_LEN(PLAN_CASTLE) : r == 2 ? ARRAY_LEN(PLAN_EXPERT) : r == 3 ? ARRAY_LEN(PLAN_CHIMNEY) : ARRAY_LEN(PLAN);
}

int frl_bot_plans(void) {
    int n = 0;
    while (n < nplans && plans[n]) n++;
    return n;
}

void frl_bot_reset(FrlBot *b) {
    memset(b, 0, sizeof *b);
    b->unit = -1;
}

/* the n-th command of a plan (copied into buf), or false at the end */
static bool token(const char *plan, int n, char *buf, int cap) {
    const char *p = plan;
    for (;;) {
        while (*p == ' ') p++;
        if (!*p) return false;
        const char *e = p;
        while (*e && *e != ' ') e++;
        if (n == 0) {
            int len = (int)(e - p);
            if (len >= cap) len = cap - 1;
            memcpy(buf, p, (size_t)len);
            buf[len] = 0;
            return true;
        }
        n--;
        p = e;
    }
}

/* a deadly foe ahead within reach of this trade's attack, on the same level */
static bool foe_ahead(const FrlWorld *w, int dir) {
    static const int REACH[FRC_COUNT] = {14, 110, 60, 46, 0};
    int reach = REACH[w->u.cls];
    if (!reach || !dir) return false;
    if (FRL_AMMO[w->u.cls] && w->u.ammo <= 0) return false;
    int cx = frl_unit_cx(w), uy = w->u.y >> 8;
    for (int i = 0; i < w->nfoe; i++) {
        const FrlFoe *f = &w->foe[i];
        if (!f->on || f->kind == FK_GULPER || f->kind == FK_HEART) continue;
        int fx0 = f->x >> 8, fx1 = fx0 + f->w, fy0 = f->y >> 8, fy1 = fy0 + f->h;
        if (fy1 < uy - 2 || fy0 > uy + FRL_UH + 2) continue;
        int d = dir > 0 ? fx0 - cx : cx - fx1;
        if (d < -4 || d > reach) continue;
        /* only what it can see: no wall between */
        int my = uy + FRL_UH / 2, x1 = dir > 0 ? fx0 : fx1, clear = 1;
        for (int x = cx; dir > 0 ? x < x1 : x > x1; x += dir * 3)
            if (frl_solid(w, x / FRL_T, my / FRL_T)) { clear = 0; break; }
        if (clear) return true;
    }
    return false;
}

/* anything deadly within 40 px ahead (whatever the ammo) */
static bool foe_near(const FrlWorld *w, int dir) {
    int cx = frl_unit_cx(w), uy = w->u.y >> 8;
    for (int i = 0; i < w->nfoe; i++) {
        const FrlFoe *f = &w->foe[i];
        if (!f->on || f->kind == FK_GULPER || f->kind == FK_WALLEYE) continue;
        int fx0 = f->x >> 8, fx1 = fx0 + f->w, fy0 = f->y >> 8, fy1 = fy0 + f->h;
        if (fy1 < uy - 2 || fy0 > uy + FRL_UH + 2) continue;
        int d = dir > 0 ? fx0 - cx : cx - fx1;
        if (d >= -4 && d <= 40) return true;
    }
    return false;
}

static int first_of(const FrlWorld *w, int kind, int nth) {
    for (int i = 0; i < w->nplaced; i++)
        if (w->foe[i].kind == kind && nth-- == 0) return i;
    return -1;
}

/* the gap between the volunteer's box and a foe's, on the side it is on
 * (negative: overlapping); dir gets the side */
static int gap_to(const FrlWorld *w, const FrlFoe *f, int *dir) {
    int ux0 = w->u.x >> 8, ux1 = ux0 + FRL_UW - 1;
    int fx0 = f->x >> 8, fx1 = fx0 + f->w - 1;
    if (fx0 + fx1 >= ux0 + ux1) { *dir = 1; return fx0 - ux1 - 1; }
    *dir = -1;
    return ux0 - fx1 - 1;
}

static bool level_with(const FrlWorld *w, const FrlFoe *f) {
    int uy = w->u.y >> 8, fy0 = f->y >> 8, fy1 = fy0 + f->h;
    return fy1 >= uy - 3 && fy0 <= uy + FRL_UH + 3;
}

/* one frame of a mallet duel with foe i */
static uint32_t duel(const FrlWorld *w, FrlBot *b, int i) {
    const FrlUnit *u = &w->u;
    uint32_t m = 0;
    int want = 0, toward = 0;
    /* first anything else deadly closing in */
    int best = 99, bdir = 0;
    for (int k = 0; k < w->nfoe; k++) {
        const FrlFoe *f = &w->foe[k];
        if (k == i || !f->on || f->kind == FK_WALLEYE || f->kind == FK_GULPER || !level_with(w, f)) continue;
        int d, g = gap_to(w, f, &d);
        if (g < best) { best = g; bdir = d; }
    }
    int tdir = 0, tg = 99;
    if (i >= 0) tg = gap_to(w, &w->foe[i], &tdir);
    if (i < 0 && best > 9) return 0;          /* standing guard, nothing near */
    if (best <= 9 && best < tg) {
        toward = bdir;
        if (best < 1) want = -bdir;
    } else {
        toward = tdir;
        if (tg < 3) want = -tdir;           /* too close: back off */
        else if (tg > 8 && i >= 0) want = tdir; /* out of reach: close in */
    }
    if (want) m |= want > 0 ? BTN_RIGHT : BTN_LEFT;
    else if ((u->face > 0) != (toward > 0)) m |= toward > 0 ? BTN_RIGHT : BTN_LEFT; /* turn to it */
    int g = toward == tdir && !(best <= 9 && best < tg) ? tg : best;
    if (g <= 10 && (u->face > 0) == (toward > 0) && !u->atk_cd && !(b->prev & BTN_B)) m |= BTN_B;
    return m;
}

static void stuck(FrlBot *b, const char *why) {
    if (!b->stuck && frl_bot_debug) fprintf(stderr, "bot stuck: volunteer %d step %d (%s)\n", b->unit, b->step, why);
    if (!b->stuck) { b->stuck_unit = b->unit; b->stuck_step = b->step; }
    b->stuck = true;
}

uint32_t frl_bot(const FrlWorld *w, FrlBot *b) {
    int n = frl_bot_plans();
    if (w->phase == FWP_SELECT) {
        int u = w->units;
        if (u >= n) return 0;
        char tk[24];
        if (!token(plans[u], 0, tk, sizeof tk) || tk[0] != 'C') { stuck(b, "no trade"); return 0; }
        int want = atoi(tk + 1);
        b->unit = u;
        b->step = 1;
        b->t = 0;
        b->dir = 0;
        b->hold_b = false;
        if (w->phase_t < 14) return 0;
        uint32_t m = 0;
        if (w->sel != want) m = BTN_RIGHT;
        else m = BTN_A;
        if (b->prev & m) m = 0; /* every press a fresh one */
        b->prev = m;
        return m;
    }
    if (w->phase != FWP_PLAY || b->unit < 0 || b->unit >= n || b->unit != w->units - 1) { b->prev = 0; return 0; }
    const FrlUnit *u = &w->u;
    int cx = frl_unit_cx(w), cy = frl_unit_cy(w);
    uint32_t m = 0;
    for (int guard = 0; guard < 4; guard++) {
        char tk[24];
        if (!token(plans[b->unit], b->step, tk, sizeof tk)) break; /* the plan is done: stand still */
        int v = atoi(tk + 1);
        bool next = false;
        m = (uint32_t)(b->dir > 0 ? BTN_RIGHT : b->dir < 0 ? BTN_LEFT : 0);
        if (b->hold_b) m |= BTN_B;
        switch (tk[0]) {
        case '>': b->dir = 1; next = true; break;
        case '<': b->dir = -1; next = true; break;
        case '=': b->dir = 0; next = true; break;
        case 'H': b->hold_b = true; next = true; break;
        case 'h': b->hold_b = false; next = true; break;
        case 'X':
            if ((b->dir >= 0 && cx >= v) || (b->dir < 0 && cx <= v)) next = true;
            else if (b->t > 1500) stuck(b, tk);
            break;
        case 'W': if (b->t >= v) next = true; break;
        case 'M': {
            int i = first_of(w, v, 0);
            if (i < 0 || !w->foe[i].on) next = true;
            else if (b->t > 6000) stuck(b, tk);
            else m = duel(w, b, i);
            break;
        }
        case 'N':
            if (b->t >= v) next = true;
            else m = duel(w, b, -1);
            break;
        case 'Z':
            if (b->t >= v + 4) next = true;
            else {
                if (b->t < v) m |= BTN_A;
                if (b->t == 9 && !u->atk_cd) m |= BTN_B;
            }
            break;
        case 'Q': {
            int i = first_of(w, FK_HEART, v);
            if (i < 0 || i >= w->nfoe || !w->foe[i].on) next = true;
            else if (b->t > 3000) stuck(b, tk);
            else {
                m &= ~(uint32_t)(BTN_LEFT | BTN_RIGHT);
                if (!u->atk_cd && !(b->prev & BTN_B)) m |= BTN_B;
            }
            break;
        }
        case '@':
            m &= ~(uint32_t)(BTN_LEFT | BTN_RIGHT);
            if (iabs(cx - v) <= 0 && iabs(u->vx) < 40 && u->ground) next = true;
            else if (b->t > 600) stuck(b, tk);
            else if (cx < v && u->vx <= 64) m |= BTN_RIGHT;
            else if (cx > v && u->vx >= -64) m |= BTN_LEFT;
            break;
        case 'a':
            if (b->t >= v) next = true;
            else {
                m |= BTN_A;
                if (b->t < 6) m &= ~(uint32_t)(BTN_LEFT | BTN_RIGHT);
            }
            break;
        case 'f':
            if (b->t >= v) next = true;
            else {
                m &= ~(uint32_t)(BTN_LEFT | BTN_RIGHT);
                if ((u->face > 0) != (b->dir > 0) && b->dir) m |= b->dir > 0 ? BTN_RIGHT : BTN_LEFT;
                if (u->ground && foe_ahead(w, b->dir) && !u->atk_cd && !(b->prev & BTN_B)) m |= BTN_B;
            }
            break;
        case 'k':
            if ((b->dir >= 0 && cx >= v) || (b->dir < 0 && cx <= v)) next = true;
            else if (b->t > 3000) stuck(b, tk);
            else if (u->ground && FRL_AMMO[u->cls] && u->ammo <= 0 && foe_near(w, b->dir)) next = true; /* out of shot */
            else if (u->ground && foe_ahead(w, b->dir)) {
                /* stand (still facing) and strike */
                m &= ~(uint32_t)(BTN_LEFT | BTN_RIGHT);
                if (u->vx == 0 && (u->face > 0) != (b->dir > 0)) m |= b->dir > 0 ? BTN_RIGHT : BTN_LEFT;
                if (!u->atk_cd && !(b->prev & BTN_B)) m |= BTN_B;
            }
            break;
        case 'A':
            if (b->t >= v) next = true;
            else m |= BTN_A;
            break;
        case 'G':
            if (u->ground && b->t > 1) next = true;
            else if (b->t > 600) stuck(b, tk);
            break;
        case 'Y':
            if (cy <= v) next = true;
            else if (b->t > 900) stuck(b, tk);
            break;
        case 'y':
            if (cy >= v) next = true;
            else if (b->t > 900) stuck(b, tk);
            break;
        case 'B':
            if (b->t >= v) next = true; /* let go this frame */
            else m |= BTN_B;
            break;
        case 'T':
            if (b->t >= v * 12) next = true;
            else if (b->t % 12 == 0) m |= BTN_B;
            break;
        case 'K':
            if (v < 0 || v >= w->nfoe || !w->foe[v].on) next = true;
            else if (b->t > 4000) stuck(b, tk);
            else if (b->t % 12 == 0) m |= BTN_B;
            break;
        case 'F':
            if (v < 0 || v >= w->nfoe || !w->foe[v].on) next = true;
            else if (b->t > 6000) stuck(b, tk);
            break;
        case 'S':
            if (frl_plate_down(w, v)) next = true;
            else if (b->t > 600) stuck(b, tk);
            break;
        case 'U':
            if (b->t >= 2) next = true;
            else if (b->t == 0) m |= BTN_UP;
            break;
        case 'D':
            if (b->t >= 2) next = true;
            else if (b->t == 0) m |= BTN_DOWN;
            break;
        default: stuck(b, tk); next = true; break;
        }
        if (!next) { b->t++; break; }
        if (frl_bot_debug > 1) fprintf(stderr, "bot %d.%d %s done at t=%d x=%d y=%d\n", b->unit, b->step, tk, (int)w->t, cx, cy);
        b->step++;
        b->t = 0;
        /* commands that only set things up run on into the next in the same frame */
        if (!strchr("><=Hh", tk[0])) {
            m = (uint32_t)(b->dir > 0 ? BTN_RIGHT : b->dir < 0 ? BTN_LEFT : 0);
            if (b->hold_b) m |= BTN_B;
            break;
        }
    }
    b->prev = m;
    return m;
}
