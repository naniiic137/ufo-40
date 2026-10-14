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
    CLICK(S_CELL_WALL), CLICK(S_CELL_BARREL), USE(I_NAIL, S_CELL_DOOR), USE(I_STONE, S_CELL_DOOR),
    GO(R_HALL), CLICK(S_HALL_TABLE),
    GO(R_KITCHEN), CLICK(S_KIT_KNIVES),
    GO(R_SCULLERY), CLICK(S_SCU_DRAWER),
    GO(R_GARDEN), USE(I_KNIFE, S_GDN_HIVES), USE(I_PHOTO, S_GDN_SHEDDOOR),
    GO(R_SHED), CLICK(S_SHE_CUTTERS),
    GO(R_YARD), USE(I_TAPKEY, S_YRD_BUTT), COMB(I_KNIFE, I_TIN),
    GO(R_GARAGE), USE(I_ROTOR, S_GAR_CAR), USE(I_CUTTERS, S_GAR_DOOR), CLICK(S_GAR_DOOR),
    USE(I_CARKEY, S_GAR_CAR),
    {OP_END, 0, 0},
};

/* the cellar: everything, every page */
#define CHERRY_STEPS                                                                                            \
    CLICK(S_CELL_WALL), CLICK(S_CELL_BARREL), USE(I_NAIL, S_CELL_DOOR), USE(I_STONE, S_CELL_DOOR),              \
    GO(R_PASSAGE), CLICK(S_PAS_RUG),                                                                            \
    GO(R_SICKROOM), CLICK(S_SICK_BED), CLICK(S_SICK_STAND),                                                     \
    GO(R_HALL), CLICK(S_HALL_TABLE), CLICK(S_HALL_UMBRELLA),                                                    \
    GO(R_PARLOUR), CLICK(S_PAR_IRONS), CLICK(S_PAR_SOFA),                                                       \
    GO(R_SMOKING), CLICK(S_SMO_PIPES), CLICK(S_SMO_CHAIR), USE(I_POKER, S_SMO_CABINET),                         \
    GO(R_WASHROOM), CLICK(S_WAS_CISTERN),                                                                       \
    GO(R_GUEST), CLICK(S_GUE_SEW), CLICK(S_GUE_DRESSER),                                                        \
    COMB(I_FUSE, I_FUSEWIRE),                                                                                   \
    GO(R_DINING), CLICK(S_DIN_SIDE), CLICK(S_DIN_DIARY),                                                        \
    GO(R_KITCHEN), CLICK(S_KIT_KNIVES), CLICK(S_KIT_SALT), CLICK(S_KIT_BOOK),                                   \
    GO(R_SCULLERY), CLICK(S_SCU_DRAWER), CLICK(S_SCU_BUCKET), CLICK(S_SCU_DIARY), USE(I_SALT, S_SCU_ICEBOX),    \
    USE(I_GOODFUSE, S_SCU_FUSEBOX), USE(I_BUCKET, S_SCU_SINK),                                                  \
    GO(R_GARAGE), CLICK(S_GAR_SCREW), CLICK(S_GAR_GREASE), CLICK(S_GAR_SHELF), CLICK(S_GAR_CAR),                \
    GO(R_PARLOUR), USE(I_SCREWDRIVER, S_PAR_RADIO),                                                             \
    GO(R_GLASSHOUSE), CLICK(S_GLA_LAMP), CLICK(S_GLA_SULPHATE), CLICK(S_GLA_TROWEL),                            \
    COMB(I_LAMP, I_PARAFFIN), COMB(I_MATCHES, I_LAMP),                                                          \
    GO(R_GARDEN), USE(I_KNIFE, S_GDN_HOSE), USE(I_PHOTO, S_GDN_SHEDDOOR),                                       \
    GO(R_SHED), CLICK(S_SHE_PLIERS), CLICK(S_SHE_CUTTERS), CLICK(S_SHE_SMOKER), CLICK(S_SHE_SPRAYER),           \
    CLICK(S_SHE_GLOVES), CLICK(S_SHE_DIARY),                                                                    \
    GO(R_GARDEN), USE(I_CUTTERS, S_GDN_WELL),                                                                   \
    GO(R_POND), CLICK(S_POND_BOAT), COMB(I_KNIFE, I_TACKLE), COMB(I_MAGNET, I_LINE),                            \
    GO(R_GARDEN), USE(I_MAGNETLINE, S_GDN_WELL),                                                                \
    GO(R_YARD), CLICK(S_YRD_KENNEL), CLICK(S_YRD_PINE), COMB(I_SMOKER, I_CONES), COMB(I_MATCHES, I_SMOKER),     \
    GO(R_GARAGE), USE(I_TROWEL, S_GAR_SACK),                                                                    \
    GO(R_HALL), USE(I_LAMP, S_HALL_STAIRS_DARK),                                                                \
    GO(R_LANDING), USE(I_SCREWDRIVER, S_LAN_MASTER_SHUT),                                                       \
    GO(R_MASTER), CLICK(S_MAS_BED), CLICK(S_MAS_POST),                                                          \
    GO(R_BATH), CLICK(S_BAT_CABINET), CLICK(S_BAT_DIARY), COMB(I_RAZOR, I_BELT),                                \
    GO(R_STUDY), CLICK(S_STU_DIARY), USE(I_BUREAUKEY, S_STU_BUREAU),                                            \
    GO(R_BALCONY), USE(I_BINOCULARS, S_BAL_VIEW),                                                               \
    GO(R_NURSERY), CLICK(S_NUR_DIARY), USE(I_SCREWDRIVER, S_NUR_HORSE),                                         \
    GO(R_CHILDREN), CLICK(S_CHI_DIARY), CLICK(S_CHI_POLE),                                                      \
    GO(R_LANDING), USE(I_POLE, S_LAN_HATCH),                                                                    \
    GO(R_ATTIC), CLICK(S_ATT_DIARY), CLICK(S_ATT_BOXES), USE(I_SMOKER, S_ATT_NEST),                             \
    GO(R_STUDY), USE(I_REEL, S_STU_RECORDER), USE(I_TAPE, S_STU_RECORDER), CLICK(S_STU_RECORDER),               \
    COMB(I_SPRAYER, I_SULPHATE), COMB(I_SPRAYER, I_LIME), COMB(I_SPRAYER, I_WATER),                             \
    GO(R_COURTYARD), USE(I_BEE_BRASS, S_CRT_HIVE), USE(I_BEE_COPPER, S_CRT_HIVE), USE(I_BEE_SILVER, S_CRT_HIVE), \
    USE(I_BEE_IRON, S_CRT_HIVE),                                                                                \
    GO(R_MAZE), {OP_MAZE, 0, 0}, USE(I_PLIERS, S_HRT_OAK),                                                      \
    COMB(I_CROSSBOW, I_BOLT),                                                                                   \
    GO(R_WASHROOM), {OP_KILL, 0, S_WAS_TOILET}, {OP_FLOOR, I_BLACKKEY, 0},                                      \
    GO(R_DRIVE), USE(I_BLACKKEY, S_DRV_HATCH),                                                                  \
    GO(R_CELLAR), CLICK(S_CEL_DIARY), USE(I_PUMPHANDLE, S_CEL_PUMP), USE(I_WASHER, S_CEL_PUMP),                 \
    USE(I_HOSE, S_CEL_PUMP), USE(I_GREASE, S_CEL_PUMP),                                                         \
    GO(R_SPORE), USE(I_MIXTURE, S_SPO_FACE), {OP_END, 0, 0}

static const Step PLAN_CHERRY[] = {CHERRY_STEPS};
/* the same, but it lets him catch it once, early */
static const Step PLAN_CHERRY_CAUGHT[] = {
    CLICK(S_CELL_WALL), CLICK(S_CELL_BARREL), USE(I_NAIL, S_CELL_DOOR), USE(I_STONE, S_CELL_DOOR),
    GO(R_DINING), {OP_CAUGHT, 0, S_DIN_PIANOLA},
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

/* the step the demo player is on (tests): 0 op, 1 a, 2 b */
int wth_bot_cur(int what) {
    if (!plan || wth_bot_step >= plan_n) return -1;
    const Step *st = &plan[wth_bot_step];
    return what == 0 ? st->op : what == 1 ? st->a : st->b;
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

/* walking to a room (a chase can carry the player off between steps) */
static unsigned go_room(int r, bool *there) {
    bool ready;
    *there = wth.s.room == r;
    if (*there) return 0;
    unsigned m = hold(0, &ready);
    if (!ready) return m;
    int s = way_to(r);
    return s < 0 ? 0 : aim_spot(s, NULL);
}

static unsigned do_step(const Step *st) {
    bool ready = false, clicked = false, there = false;
    unsigned m;
    if (st->op == OP_CLICK || st->op == OP_USE || st->op == OP_KILL || st->op == OP_CAUGHT) {
        int r = WTH_SPOT[st->b].room;
        bool need = st->op != OP_USE || wth_has(st->a);
        if (st->op == OP_CLICK && !wth_spot_live(st->b)) need = false;
        if (st->op == OP_KILL && wth.k.state != E_NONE) need = false;
        if (st->op == OP_CAUGHT && wth.k.state != E_NONE) need = false;
        if (need) {
            m = go_room(r, &there);
            if (!there) return m;
        }
    }
    if (st->op == OP_FLOOR) {
        int fr = -1;
        for (int r = 0; r < R_COUNT && fr < 0; r++)
            for (int i = 0; i < WTH_FLOOR; i++)
                if (wth.s.floor[r][i] == st->a) fr = r;
        if (fr >= 0) {
            m = go_room(fr, &there);
            if (!there) return m;
        }
    }
    if (st->op == OP_MAZE && wth.s.room != R_MAZE && wth.s.room != R_HEART) {
        m = go_room(R_MAZE, &there);
        if (!there) return m;
    }
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
