/* WITHERLEA - the demo player. It plays with real button presses: it moves
 * the hand with the pad, opens the bag with B, picks things with A, and
 * clicks them on the house, following a written-out walkthrough (a plan of
 * steps). It finds its own way between rooms. When the Keeper comes it hides
 * if the room has somewhere to hide (and never misses a breath), and
 * otherwise runs, never doubling back into him and keeping out of dead ends.
 * With the loaded crossbow it waits for him instead. Every press is let go
 * the next frame, so each one counts as a fresh press. */
#include "wth.h"

int wth_bot_step;
int wth_bot_debug;

enum { OP_GO, OP_CLICK, OP_USE, OP_COMB, OP_FLOOR, OP_MAZE, OP_KILL, OP_CAUGHT, OP_END };
typedef struct { uint8_t op; uint8_t a; int16_t b; } Step;

#define GO(r) {OP_GO, r, 0}
#define CLICK(s) {OP_CLICK, 0, s}
#define USE(i, s) {OP_USE, i, s}
#define COMB(i, j) {OP_COMB, i, j}

/* the car: the fewest things it takes */
static const Step PLAN_GOLD[] = {
    CLICK(S_CELL_STRAW), USE(I_SHOEHORN, S_CELL_VENT), CLICK(S_CELL_VENT), USE(I_WIRE, S_CELL_DOOR),
    GO(R_HALL), CLICK(S_HALL_UMBRELLA),
    GO(R_GUEST), CLICK(S_GUE_SEW),
    GO(R_GLASSHOUSE), CLICK(S_GLA_TRELLIS),
    GO(R_GARAGE), CLICK(S_GAR_TWINE), CLICK(S_GAR_SPANNER),
    COMB(I_CANE, I_CUPHOOK),
    GO(R_POND), USE(I_LONGHOOK, S_POND_BODY),
    GO(R_GARDEN), USE(I_SHEDKEY, S_GDN_SHEDDOOR),
    GO(R_SHED), CLICK(S_SHE_SPADE), USE(I_SPANNER, S_SHE_GRINDER),
    GO(R_GARDEN), USE(I_SPADE, S_GDN_GRAVE),
    GO(R_GARAGE), USE(I_MOTOR, S_GAR_OPENER), USE(I_OIL, S_GAR_OPENER), CLICK(S_GAR_BUTTON),
    USE(I_CARKEYS, S_GAR_CAR),
    {OP_END, 0, 0},
};

/* the cellar: everything, every page */
#define CHERRY_STEPS                                                                                            \
    CLICK(S_CELL_STRAW), CLICK(S_CELL_TRAY), USE(I_SHOEHORN, S_CELL_VENT), CLICK(S_CELL_VENT), CLICK(S_CELL_VENT), \
    USE(I_WIRE, S_CELL_DOOR),                                                                                   \
    GO(R_PASSAGE), CLICK(S_PAS_COAT), CLICK(S_PAS_RUG),                                                         \
    GO(R_SICKROOM), CLICK(S_SICK_BED), CLICK(S_SICK_STAND),                                                     \
    GO(R_KITCHEN), CLICK(S_KIT_STOVE), CLICK(S_KIT_KNIVES), CLICK(S_KIT_BOOK),                                  \
    GO(R_SCULLERY), CLICK(S_SCU_TUB), CLICK(S_SCU_SODA), CLICK(S_SCU_DIARY),                                    \
    GO(R_DINING), CLICK(S_DIN_SIDE), CLICK(S_DIN_TABLE), USE(I_PENNIES, S_DIN_ORCH),                            \
    GO(R_HALL), CLICK(S_HALL_UMBRELLA),                                                                         \
    GO(R_PARLOUR), CLICK(S_PAR_PIG), CLICK(S_PAR_SOFA),                                                         \
    GO(R_SMOKING), CLICK(S_SMO_HUMIDOR), CLICK(S_SMO_DESK), CLICK(S_SMO_CHAIR),                                 \
    COMB(I_TORCH, I_BATTERIES),                                                                                 \
    GO(R_WASHROOM), CLICK(S_WAS_CAB), USE(I_SODA, S_WAS_BATH), USE(I_TONGS, S_WAS_BATH),                        \
    GO(R_GUEST), CLICK(S_GUE_SEW), USE(I_TINKEY, S_GUE_BOX),                                                    \
    GO(R_HALL), USE(I_COPPERKEY, S_HALL_CLOCK), {OP_FLOOR, I_GEAR, 0},                                          \
    GO(R_GLASSHOUSE), CLICK(S_GLA_TRELLIS),                                                                     \
    GO(R_GARAGE), CLICK(S_GAR_TWINE), CLICK(S_GAR_SPANNER), CLICK(S_GAR_SHELF), CLICK(S_GAR_CAR),               \
    COMB(I_CANE, I_CUPHOOK),                                                                                    \
    GO(R_PARLOUR), USE(I_PARAFFIN, S_PAR_FIRE), USE(I_MATCHES, S_PAR_FIRE), CLICK(S_PAR_FIRE),                 \
    GO(R_POND), USE(I_LONGHOOK, S_POND_BODY),                                                                   \
    GO(R_GARDEN), USE(I_SHEDKEY, S_GDN_SHEDDOOR),                                                               \
    GO(R_SHED), CLICK(S_SHE_SPADE), CLICK(S_SHE_SCREW), CLICK(S_SHE_SHEARS), CLICK(S_SHE_SAW),                  \
    CLICK(S_SHE_SACK), CLICK(S_SHE_DIARY), USE(I_SPANNER, S_SHE_GRINDER), USE(I_PIG, S_SHE_VISE),               \
    GO(R_GARDEN), USE(I_SHEARS, S_GDN_IVY),                                                                     \
    GO(R_HALL), USE(I_TORCH, S_HALL_STAIRS_DARK),                                                               \
    GO(R_LANDING), USE(I_BRASSKEY, S_LAN_MASTER_SHUT),                                                          \
    GO(R_MASTER), CLICK(S_MAS_DRESSER), CLICK(S_MAS_BED),                                                       \
    GO(R_BALCONY), USE(I_SAW, S_BAL_ROPE),                                                                      \
    GO(R_STUDY), CLICK(S_STU_DIARY), USE(I_BAR, S_STU_CASE),                                                    \
    GO(R_BATH), CLICK(S_BAT_SHELF), CLICK(S_BAT_DIARY), USE(I_BAR, S_BAT_MIRROR),                               \
    GO(R_MASTER), USE(I_COMBO, S_MAS_SAFE),                                                                     \
    GO(R_CHILDREN), CLICK(S_CHI_DIARY), CLICK(S_CHI_DOLL), COMB(I_KNIFE, I_DOLL), USE(I_LENS, S_CHI_HOUSE),     \
    GO(R_STUDY), USE(I_PASSWORD, S_STU_COMPUTER),                                                               \
    COMB(I_MUG, I_SNUFF), COMB(I_MUG, I_CLOVE), COMB(I_MUG, I_BORAX),                                           \
    GO(R_NURSERY), CLICK(S_NUR_DIARY), USE(I_SILVERKEY, S_NUR_BOX), USE(I_GLASSES, S_NUR_WALL),                 \
    GO(R_LANDING), USE(I_LONGHOOK, S_LAN_HATCH),                                                                \
    GO(R_ATTIC), CLICK(S_ATT_DIARY), USE(I_PEWTERKEY, S_ATT_TRUNK), USE(I_HYMN, S_ATT_HARM),                    \
    GO(R_GARDEN), CLICK(S_GDN_FALLEN),                                                                          \
    GO(R_DRIVE), USE(I_RING, S_DRV_STATUE), CLICK(S_DRV_FOUNTAIN),                                              \
    GO(R_COURTYARD), USE(I_AMBER, S_CRT_HARE0), USE(I_JADE, S_CRT_HARE1), USE(I_GARNET, S_CRT_HARE2),           \
    USE(I_OPAL, S_CRT_HARE3),                                                                                   \
    GO(R_MAZE), {OP_MAZE, 0, 0}, CLICK(S_HRT_GIRL),                                                             \
    COMB(I_CROSSBOW, I_BOLT),                                                                                   \
    GO(R_WASHROOM), {OP_KILL, 0, S_WAS_TOILET}, {OP_FLOOR, I_KEEPERKEY, 0},                                     \
    GO(R_DRIVE), USE(I_KEEPERKEY, S_DRV_STORM),                                                                 \
    GO(R_CELLAR), CLICK(S_CEL_DIARY), USE(I_GEAR, S_CEL_MECH), USE(I_STEELWIRE, S_CEL_MECH),                    \
    USE(I_SCREWDRIVER, S_CEL_MECH), USE(I_OIL, S_CEL_MECH),                                                     \
    GO(R_SPORE), USE(I_BREW, S_SPO_FACE), {OP_END, 0, 0}

static const Step PLAN_CHERRY[] = {CHERRY_STEPS};
/* the same, but it lets him catch it once, early */
static const Step PLAN_CHERRY_CAUGHT[] = {
    CLICK(S_CELL_STRAW), USE(I_SHOEHORN, S_CELL_VENT), CLICK(S_CELL_VENT), USE(I_WIRE, S_CELL_DOOR),
    GO(R_PARLOUR), {OP_CAUGHT, 0, S_PAR_RADIO},
    GO(R_CELL), CHERRY_STEPS,
};

static const Step *plan;
static int plan_n;
static unsigned prev;
static int wait_t;
static uint16_t caught_at_start;
static uint8_t noised;

void wth_bot_reset(int which) {
    plan = which == BOT_GOLD ? PLAN_GOLD : which == BOT_CHERRY ? PLAN_CHERRY : PLAN_CHERRY_CAUGHT;
    plan_n = which == BOT_GOLD ? ARRAY_LEN(PLAN_GOLD) : which == BOT_CHERRY ? ARRAY_LEN(PLAN_CHERRY) : ARRAY_LEN(PLAN_CHERRY_CAUGHT);
    wth_bot_step = 0;
    prev = 0;
    wait_t = 0;
    noised = 0;
    caught_at_start = 0;
}

/* ------------------------------------------------------------------ */
/* the hand                                                             */

static int px(void) { return (int)(wth.cx >> 8) + wth.shake_x; }
static int py(void) { return (int)(wth.cy >> 8) + wth.shake_y; }

/* toward a point; A once there */
static unsigned aim(int tx, int ty, bool click) {
    int dx = tx - px(), dy = ty - py();
    unsigned m = 0;
    if (dx > 1) m |= BTN_RIGHT;
    if (dx < -1) m |= BTN_LEFT;
    if (dy > 1) m |= BTN_DOWN;
    if (dy < -1) m |= BTN_UP;
    if (!m && click) m = BTN_A;
    return m;
}

/* a point of the spot that the Keeper isn't standing in front of */
static bool spot_point(int s, int *tx, int *ty) {
    const WthSpot *p = &WTH_SPOT[s];
    const int fx[5] = {2, 1, 3, 2, 2}, fy[5] = {2, 2, 2, 3, 1};
    int kx = 0, ky = 0, kw = 0, kh = 0;
    bool vis = wth_keeper_visible();
    if (vis) wth_keeper_rect(&kx, &ky, &kw, &kh);
    for (int i = 0; i < 5; i++) {
        int x = p->x + p->w * fx[i] / 4, y = p->y + p->h * fy[i] / 4;
        if (vis && x >= kx - 3 && y >= ky - 3 && x < kx + kw + 3 && y < ky + kh + 3) continue;
        *tx = x;
        *ty = y;
        return true;
    }
    return false;
}

static unsigned aim_spot(int s, bool *clicked) {
    int tx, ty;
    if (!spot_point(s, &tx, &ty)) return 0;
    unsigned m = aim(tx, ty, true);
    if (m == BTN_A && wth_spot_at(px(), py()) == s) {
        if (clicked) *clicked = true;
        return BTN_A;
    }
    return m == BTN_A ? 0 : m;
}

/* ------------------------------------------------------------------ */
/* the bag                                                              */

static unsigned bag_to(int slot) {
    int c = wth.bag_sel % 10, r = wth.bag_sel / 10, tc = slot % 10, tr = slot / 10;
    if (tc != c) return ((tc - c + 10) % 10) <= 5 ? BTN_RIGHT : BTN_LEFT;
    if (tr != r) return ((tr - r + 3) % 3) == 1 ? BTN_DOWN : BTN_UP;
    return BTN_A;
}

/* holding x (0: holding nothing), with the bag shut */
static unsigned hold(int x, bool *ready) {
    *ready = false;
    if (wth.scene == SC_PLAY) {
        if (wth.held == x) { *ready = true; return 0; }
        return BTN_B;
    }
    if (wth.scene == SC_BAG) {
        if (wth.held && wth.held != x) return bag_to(wth_slot_of(wth.held)); /* A on it: back in the bag */
        if (!wth.held && x) {
            int s = wth_slot_of(x);
            if (s < 0) return BTN_B;
            return bag_to(s);
        }
        return BTN_B;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* finding the way                                                      */

static bool usable_exit(int s, int avoid) {
    const WthSpot *p = &WTH_SPOT[s];
    if (p->room != wth.s.room || p->kind != HS_EXIT) return false;
    int d = wth_exit_dest(s);
    if (d < 0 || d == avoid) return false;
    if (wth.s.room == R_MAZE && p->dir != DIR_DOWN && p->dir != WTH_MAZE_WAY[wth.s.maze]) return false;
    return true;
}

/* the first exit on the way to a room (-1: none) */
static int way_to(int target) {
    static int16_t from_spot[R_COUNT];
    static int8_t seen[R_COUNT];
    int q[R_COUNT], h = 0, t = 0;
    memset(seen, 0, sizeof seen);
    seen[wth.s.room] = 1;
    from_spot[wth.s.room] = -1;
    q[t++] = wth.s.room;
    while (h < t) {
        int r = q[h++];
        if (r == target) break;
        for (int s = 0; s < S_COUNT; s++) {
            const WthSpot *p = &WTH_SPOT[s];
            if (p->room != r || p->kind != HS_EXIT || !wth_spot_live(s)) continue;
            if (r == R_MAZE) continue; /* the maze is walked by the directions */
            int d = p->dest;
            if (s == S_DRV_CELLAR && !wth_flag(F_STAIRS_LIT)) continue;
            if (d == R_MAZE && target != R_MAZE) continue;
            if (seen[d]) continue;
            seen[d] = 1;
            from_spot[d] = (int16_t)(r == wth.s.room ? s : from_spot[r]);
            q[t++] = d;
        }
    }
    if (!seen[target] || target == wth.s.room) return -1;
    return from_spot[target];
}

/* how many ways out a room has, not counting the one back to 'from' */
static int ways_on(int room, int from) {
    int n = 0;
    for (int s = 0; s < S_COUNT; s++) {
        const WthSpot *p = &WTH_SPOT[s];
        if (p->room != room || p->kind != HS_EXIT || !wth_spot_live(s)) continue;
        if (room == R_MAZE) continue;
        if (p->dest != from && !(s == S_DRV_CELLAR && !wth_flag(F_STAIRS_LIT))) n++;
    }
    return n;
}

/* running from him: the nearest way out that isn't back into him and
 * doesn't lead into a dead end (when there is a choice) */
static int flee_exit(void) {
    int avoid = wth.k.state == E_FOLLOW ? wth.k.room : -1;
    int best = -1, best_score = 1 << 30;
    for (int s = 0; s < S_COUNT; s++) {
        if (!usable_exit(s, avoid)) continue;
        const WthSpot *p = &WTH_SPOT[s];
        int d = wth_exit_dest(s);
        int dx = p->x + p->w / 2 - px(), dy = p->y + p->h / 2 - py();
        int score = iabs(dx) + iabs(dy), tx, ty;
        if (!spot_point(s, &tx, &ty)) continue; /* he's standing in the way */
        if (d == R_MAZE || d == R_HEART) score += 1000;
        if (ways_on(d, wth.s.room) == 0) score += 400;
        if (score < best_score) { best_score = score; best = s; }
    }
    return best;
}

static int hide_spot_here(void) {
    for (int s = 0; s < S_COUNT; s++) {
        const WthSpot *p = &WTH_SPOT[s];
        if (p->room != wth.s.room || p->kind != HS_HIDE) continue;
        int f = wth_hide_one_use_flag(s);
        if (f >= 0 && wth_flag(f)) continue;
        return s;
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* the plan                                                             */

static unsigned do_step(const Step *st) {
    bool ready = false, clicked = false;
    unsigned m;
    switch (st->op) {
    case OP_GO: {
        if (wth.s.room == st->a) return wth_bot_step++, 0;
        m = hold(0, &ready);
        if (!ready) return m;
        int s = way_to(st->a);
        if (s < 0) return 0;
        return aim_spot(s, &clicked);
    }
    case OP_CLICK:
        if (!wth_spot_live(st->b)) return wth_bot_step++, 0;
        m = hold(0, &ready);
        if (!ready) return m;
        m = aim_spot(st->b, &clicked);
        if (clicked) wth_bot_step++;
        return m;
    case OP_USE:
        if (!wth_has(st->a)) return wth_bot_step++, 0; /* already done */
        m = hold(st->a, &ready);
        if (!ready) return m;
        m = aim_spot(st->b, &clicked);
        if (clicked) wth_bot_step++;
        return m;
    case OP_COMB:
        if (!wth_has(st->a) || !wth_has(st->b)) return wth_bot_step++, 0;
        if (wth.scene == SC_BAG && wth.held == st->a) {
            m = bag_to(wth_slot_of(st->b));
            if (m == BTN_A) wth_bot_step++;
            return m;
        }
        if (wth.scene == SC_PLAY && wth.held == st->a) return BTN_B;
        return hold(st->a, &ready);
    case OP_FLOOR:
        for (int i = 0; i < WTH_FLOOR; i++)
            if (wth.s.floor[wth.s.room][i] == st->a) {
                m = hold(0, &ready);
                if (!ready) return m;
                m = aim(70 + i * 60 + 8, 118, true);
                if (m == BTN_A) wth_bot_step++;
                return m;
            }
        return wth_bot_step++, 0;
    case OP_MAZE:
        if (wth.s.room == R_HEART) return wth_bot_step++, 0;
        if (wth.s.room != R_MAZE) return 0;
        m = hold(0, &ready);
        if (!ready) return m;
        for (int s = 0; s < S_COUNT; s++)
            if (WTH_SPOT[s].room == R_MAZE && WTH_SPOT[s].dir == WTH_MAZE_WAY[wth.s.maze]) return aim_spot(s, &clicked);
        return 0;
    case OP_KILL:
        if (wth_flag(F_KEEPER_DEAD)) return wth_bot_step++, 0;
        if (wth.k.state == E_NONE && !noised) {
            m = hold(0, &ready);
            if (!ready) return m;
            m = aim_spot(st->b, &clicked);
            if (clicked) noised = 1;
            return m;
        }
        if (wth.k.state == E_NONE && noised) noised = 0; /* he went away: make another noise */
        return 0;                                          /* wait for him (handled in wth_bot) */
    case OP_CAUGHT:
        if (wth.s.caught > caught_at_start) return wth_bot_step++, 0;
        if (wth.k.state == E_NONE) {
            caught_at_start = wth.s.caught;
            m = hold(0, &ready);
            if (!ready) return m;
            return aim_spot(st->b, &clicked);
        }
        return 0; /* stand still and let him */
    default: return 0;
    }
}

/* ------------------------------------------------------------------ */
/* each frame                                                           */

static unsigned think(void) {
    if (!plan) return 0;
    switch (wth.scene) {
    case SC_WARNING: case SC_INTRO: case SC_WAKE: case SC_ENDING: case SC_CREDITS: case SC_STINGER: return BTN_A;
    case SC_TITLE: {
        bool has = false;
        WthSave tmp;
        has = game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.started;
        if (has && wth.title_sel != 1) return BTN_DOWN;
        return BTN_A;
    }
    case SC_DIARY: return BTN_B;
    default: break;
    }
    if (wth_bot_step >= plan_n || plan[wth_bot_step].op == OP_END) return 0;
    const Step *st = &plan[wth_bot_step];
    WthKeeper *k = &wth.k;
    /* the breathing game: press when the mark will be in the box */
    if (k->state == E_HIDING) {
        const WthMini *mi = &k->mini;
        if (mi->pause > 0) return 0;
        int32_t p = mi->pos + mi->vel, lim = WTH_BAR_W * 256;
        if (p >= lim) p = 2 * lim - p;
        if (p < 0) p = -p;
        int x = (int)(p >> 8);
        return x >= mi->zone + 1 && x <= mi->zone + mi->zw - 2 ? BTN_A : 0;
    }
    if (wth_talking()) return BTN_A;
    if (k->state == E_CAUGHT || wth.trans_t > 0) return 0;
    if (st->op == OP_CAUGHT && k->state != E_NONE) return wth.scene == SC_BAG ? BTN_B : 0;
    bool kill = st->op == OP_KILL && wth_has(I_CROSSBOW) && wth_flag(F_BOW_LOADED);
    if (kill && wth_keeper_visible()) {
        bool ready;
        unsigned m = hold(I_CROSSBOW, &ready);
        if (!ready) return m;
        int x, y, w, h;
        wth_keeper_rect(&x, &y, &w, &h);
        m = aim(x + w / 2, y + h / 2, true);
        return m;
    }
    if (kill && (k->state == E_COMING || k->state == E_FOLLOW)) return wth.scene == SC_BAG ? BTN_B : 0;
    if (k->state == E_HERE || k->state == E_FOLLOW || k->state == E_STUN) {
        if (wth.scene == SC_BAG) return BTN_B;
        int h = hide_spot_here();
        if (h >= 0) return aim_spot(h, NULL);
        int s = flee_exit();
        if (s >= 0) return aim_spot(s, NULL);
        return 0;
    }
    return do_step(st);
}

unsigned wth_bot(void) {
    /* after a press, a frame with everything let go (the pad too, in menus) */
    unsigned edge = BTN_A | BTN_B;
    if (wth.scene != SC_PLAY) edge |= BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT;
    if (prev & edge) {
        prev = 0;
        return 0;
    }
    unsigned out = think();
    if (wth_bot_debug && (wth.frame % 60) == 0)
        printf("bot step %d op %d room %d scene %d enc %d held %d out %x\n", wth_bot_step,
               plan && wth_bot_step < plan_n ? plan[wth_bot_step].op : -1, wth.s.room, wth.scene, wth.k.state, wth.held, out);
    prev = out;
    return out;
}
