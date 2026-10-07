/* SOUNDINGS - the demo player. It plays the whole game with button presses
 * (the headless runner presses what it returns, frame by frame): it shops
 * and sets the kit at the raft, swims the map along a breadth-first distance
 * field, opens chests, pulls levers, breaks cracked rock with charges, picks
 * up heads, grinds where its level says, surfaces when hurt, gives every
 * diver an order in every fight, and at last stills the Gloamheart.
 *
 * It reads the game's state to decide, never changes it, and uses only
 * whole numbers, so a seed plays the same run everywhere. */
#include "sdg.h"

#define P (sdg.prog)

int sdg_bot_debug;
int sdg_bot_task;

static unsigned last;   /* what was pressed last frame */
static int16_t field[SDG_MH][SDG_MW];
static int field_key = -1, field_age;
static uint8_t field_doors, field_walls;
static int stuck_t, stuck_x, stuck_y, wiggle;
static int dive_t;
static int blocked_task = -1;
static int grind_dry;

/* ---- the plan --------------------------------------------------------------- */

enum { TK_GRIND, TK_LEVER, TK_CHEST, TK_BOMB, TK_HEAD, TK_BOSS };
typedef struct Task { uint8_t kind, arg, minlv, cherry; } Task;
static const Task PLAN[] = {
    {TK_GRIND, 3, 0, 0},
    {TK_LEVER, LV_WEST, 3, 0},
    {TK_CHEST, 4, 3, 0},           /* the fins */
    {TK_CHEST, 3, 3, 0},
    {TK_CHEST, 1, 3, 0},
    {TK_CHEST, 2, 3, 0},
    {TK_GRIND, 5, 0, 0},
    {TK_BOMB, 0, 5, 0},
    {TK_CHEST, 5, 5, 0},           /* the zap gaff */
    {TK_GRIND, 6, 0, 0},
    {TK_CHEST, 10, 6, 0},
    {TK_LEVER, LV_SHORT, 6, 0},
    {TK_CHEST, 6, 6, 0},           /* the reef gaff */
    {TK_CHEST, 9, 6, 0},
    {TK_BOMB, 1, 6, 0},
    {TK_CHEST, 7, 6, 0},
    {TK_GRIND, 8, 0, 0},
    {TK_HEAD, 0, 8, 1},
    {TK_GRIND, 9, 0, 0},
    {TK_CHEST, 0, 9, 0},           /* the leech club, past the slater packs */
    {TK_HEAD, 1, 9, 1},
    {TK_BOMB, 2, 9, 0},
    {TK_CHEST, 8, 9, 0},
    {TK_GRIND, 10, 0, 0},
    {TK_BOSS, 1, 10, 0},
    {TK_CHEST, 11, 10, 0},
    {TK_HEAD, 2, 10, 1},
    {TK_GRIND, 11, 0, 0},
    {TK_LEVER, LV_FINAL_W, 11, 0},
    {TK_CHEST, 12, 11, 0},
    {TK_CHEST, 13, 11, 0},
    {TK_GRIND, 12, 0, 0},
    {TK_LEVER, LV_FINAL_E, 12, 0},
    {TK_CHEST, 14, 12, 0},
    {TK_GRIND, 14, 0, 0},
    {TK_CHEST, 15, 14, 0},
    {TK_CHEST, 16, 14, 0},
    {TK_BOSS, 2, 14, 0},
};
#define NTASK ARRAY_LEN(PLAN)

static bool task_done(const Task *t) {
    switch (t->kind) {
    case TK_GRIND: return P.level >= t->arg;
    case TK_LEVER: return (P.levers >> t->arg) & 1;
    case TK_CHEST: return (P.chests >> t->arg) & 1;
    case TK_BOMB: return (P.walls >> t->arg) & 1;
    case TK_HEAD: return sdg.bot_goal != BOTG_CHERRY || ((P.heads >> t->arg) & 1);
    case TK_BOSS: return t->arg == 1 ? (P.doors >> DOOR_WARDEN) & 1 : P.wins > 0;
    default: return true;
    }
}

static int current_task(void) {
    for (int i = 0; i < NTASK; i++)
        if (!task_done(&PLAN[i])) return i;
    return NTASK;
}

void sdg_bot_reset(void) {
    last = 0;
    field_key = -1;
    stuck_t = 0;
    dive_t = 0;
}

static unsigned press(unsigned m) { return (last & m) ? 0 : m; }

/* ---- what to buy and how to arm ---------------------------------------------- */

static int best_weapon(int el, const uint8_t *taken) {
    int best = 0, bp = 0;
    for (int it = 1; it < IT_COUNT; it++) {
        const SdgItem *I = &SDG_ITEM[it];
        if (!P.owned[it] || I->el != el || (I->kind != K_POLE && I->kind != K_HAMMER)) continue;
        int avail = P.owned[it] - (taken ? taken[it] : 0);
        if (avail <= 0) continue;
        int pw = I->power + ((I->flags & IF_HOLY) ? 60 : 0);
        if (pw > bp) { bp = pw; best = it; }
    }
    return best;
}

static int best_potion(const uint8_t *taken) {
    static const uint8_t ORDER[] = {IT_HOLY, IT_LARGE, IT_MEDIUM, IT_SMALL};
    for (int k = 0; k < ARRAY_LEN(ORDER); k++)
        if (P.owned[ORDER[k]] > (taken ? taken[ORDER[k]] : 0)) return ORDER[k];
    return 0;
}

static int best_shield(const uint8_t *taken) {
    int best = 0, bd = 0;
    for (int it = 1; it < IT_COUNT; it++) {
        const SdgItem *I = &SDG_ITEM[it];
        if (!P.owned[it] || I->kind != K_SHIELD || P.owned[it] <= (taken ? taken[it] : 0)) continue;
        if (I->def * 10 + I->power / 50 > bd) { bd = I->def * 10 + I->power / 50; best = it; }
    }
    return best;
}

static bool bomb_task_pending(void) {
    int k = current_task();
    for (int i = k; i < NTASK && i < k + 3; i++)
        if (PLAN[i].kind == TK_BOMB && !task_done(&PLAN[i])) return true;
    return false;
}

static void want_loadout(uint8_t out[3][2]) {
    uint8_t taken[IT_COUNT];
    memset(taken, 0, sizeof taken);
    memset(out, 0, 6);
    static const int EL[3] = {EL_REEF, EL_ZAP, EL_OOZE};
    for (int d = 0; d < 3; d++) {
        int w = best_weapon(EL[d], taken);
        if (w) { out[d][0] = (uint8_t)w; taken[w]++; }
    }
    /* Moss: fins, else a shield; Reed: the best tonic; Skip: a charge when
     * one is wanted, else a bulwark, else a second tonic */
    if (P.owned[IT_FLIPPERS]) { out[0][1] = IT_FLIPPERS; taken[IT_FLIPPERS]++; }
    int pot = best_potion(taken);
    if (pot) { out[1][1] = (uint8_t)pot; taken[pot]++; }
    if (bomb_task_pending() && P.owned[IT_BOMB]) { out[2][1] = IT_BOMB; taken[IT_BOMB]++; }
    else {
        int sh = best_shield(taken);
        if (sh && SDG_ITEM[sh].def >= 25) { out[2][1] = (uint8_t)sh; taken[sh]++; }
        else {
            int p2 = best_potion(taken);
            if (p2) { out[2][1] = (uint8_t)p2; taken[p2]++; }
        }
    }
    if (!out[0][1]) {
        int sh = best_shield(taken);
        if (sh) { out[0][1] = (uint8_t)sh; taken[sh]++; }
    }
}

/* the next thing to buy, or 0 */
static int want_buy(void) {
    static const uint8_t LIST[] = {
        IT_POKER_R, IT_POKER_Z, IT_POKER_O, IT_SMALL, IT_BOMB, IT_GAFF_O, IT_MEDIUM, IT_GAFF_Z, IT_GAFF_R, IT_SMALL,
        IT_EGG, IT_HARPOON_O, IT_BULWARK_O, IT_MIST, IT_HOLY, IT_HARPOON_R, IT_HARPOON_Z, IT_BULWARK_R, IT_GODBLOOD,
    };
    static const uint8_t COUNT[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    for (int k = 0; k < ARRAY_LEN(LIST); k++) {
        int it = LIST[k];
        if (P.owned[it] >= COUNT[k]) continue;
        /* a gaff or harpoon of an element already beaten by a better one is skipped */
        if (sdg_is_weapon(it)) {
            int el = SDG_ITEM[it].el;
            int have = best_weapon(el, NULL);
            if (have && SDG_ITEM[have].power >= SDG_ITEM[it].power) continue;
        }
        if (it == IT_BOMB && P.owned[IT_BOMB] && !bomb_task_pending()) continue;
        if (sdg_can_buy(&P, it)) return it;
        /* the first pokers and the first sip come before anything else */
        if (k < 4) return 0;
    }
    return 0;
}

/* ---- swimming ------------------------------------------------------------------ */

static int qc[SDG_MW * SDG_MH], qr[SDG_MW * SDG_MH];

static void build_field(const int16_t *tc, const int16_t *tr, int n) {
    for (int r = 0; r < SDG_MH; r++)
        for (int c = 0; c < SDG_MW; c++) field[r][c] = 32767;
    int h = 0, t = 0;
    for (int i = 0; i < n; i++) {
        if (tc[i] < 0 || tr[i] < 0 || tc[i] >= SDG_MW || tr[i] >= SDG_MH) continue;
        if (field[tr[i]][tc[i]] == 0) continue;
        field[tr[i]][tc[i]] = 0;
        qc[t] = tc[i];
        qr[t++] = tr[i];
    }
    static const int DC[4] = {1, -1, 0, 0}, DR[4] = {0, 0, 1, -1};
    while (h < t) {
        int c = qc[h], r = qr[h++];
        for (int k = 0; k < 4; k++) {
            int nc = c + DC[k], nr = r + DR[k];
            if (nc < 0 || nr < 0 || nc >= SDG_MW || nr >= SDG_MH || field[nr][nc] != 32767) continue;
            if (!sdg_cell_open(nc, nr)) continue;
            field[nr][nc] = (int16_t)(field[r][c] + 1);
            qc[t] = nc;
            qr[t++] = nr;
        }
    }
}

/* steers along the field; returns the held d-pad, or 0 when there (or lost) */
static unsigned steer(int *dist_out) {
    int x = (int)(sdg.dive.x >> 8), y = (int)(sdg.dive.y >> 8);
    int c = x / SDG_T, r = y / SDG_T;
    int here = field[r][c];
    if (dist_out) *dist_out = here;
    if (here == 32767) return 0;
    int tx = c * SDG_T + SDG_T / 2, ty = r * SDG_T + SDG_T / 2;
    if (here > 0) {
        static const int DC[4] = {1, -1, 0, 0}, DR[4] = {0, 0, 1, -1};
        int best = -1, bv = here;
        for (int k = 0; k < 4; k++) {
            int nc = c + DC[k], nr = r + DR[k];
            if (nc < 0 || nr < 0 || nc >= SDG_MW || nr >= SDG_MH) continue;
            if (field[nr][nc] < bv) { bv = field[nr][nc]; best = k; }
        }
        if (best >= 0) {
            int nc = c + DC[best], nr = r + DR[best];
            tx = nc * SDG_T + SDG_T / 2;
            ty = nr * SDG_T + SDG_T / 2;
            /* keep to the middle of the lane before turning */
            if (DR[best] == 0 && iabs(y - (r * SDG_T + SDG_T / 2)) > 2) tx = x;
            if (DC[best] == 0 && iabs(x - (c * SDG_T + SDG_T / 2)) > 2) ty = y;
        }
    }
    unsigned m = 0;
    if (tx > x + 1) m |= BTN_RIGHT;
    if (tx < x - 1) m |= BTN_LEFT;
    if (ty > y + 1) m |= BTN_DOWN;
    if (ty < y - 1) m |= BTN_UP;
    return m;
}

/* ---- the dive's decisions -------------------------------------------------------- */

static int total_hp(void) {
    int s = 0;
    for (int d = 0; d < 3; d++) s += imax(0, P.hp[d]);
    return s;
}

static int total_max(void) {
    int s = 0;
    for (int d = 0; d < 3; d++) s += sdg_diver_maxhp(&P, d);
    return s;
}

static int weapon_uses(void) {
    int n = 0;
    for (int d = 0; d < 3; d++)
        for (int s = 0; s < 2; s++)
            if (P.hp[d] > 0 && sdg_is_weapon(P.equip[d][s])) n += P.uses[d][s];
    return n;
}

static int heal_slot(int kind_mask) {
    /* a living diver's tonic (or lamp rod) with uses: d*2+s, or -1 */
    for (int d = 0; d < 3; d++)
        for (int s = 0; s < 2; s++) {
            int it = P.equip[d][s];
            if (!it || P.hp[d] <= 0 || P.uses[d][s] == 0) continue;
            int k = SDG_ITEM[it].kind;
            if ((kind_mask & 1) && k == K_POTION && it != IT_EVIL) return d * 2 + s;
            if ((kind_mask & 2) && k == K_EGG) return d * 2 + s;
            if ((kind_mask & 4) && (SDG_ITEM[it].flags & IF_HOLY)) return d * 2 + s;
        }
    return -1;
}

static int lowest_ratio_diver(int *ratio) {
    int best = -1, bv = 1001;
    for (int d = 0; d < 3; d++) {
        if (P.hp[d] <= 0) continue;
        int v = P.hp[d] * 1000 / sdg_diver_maxhp(&P, d);
        if (v < bv) { bv = v; best = d; }
    }
    if (ratio) *ratio = bv;
    return best;
}

static bool should_surface(void) {
    int dead = 0;
    for (int d = 0; d < 3; d++) dead += P.hp[d] <= 0;
    if (dead && heal_slot(2) < 0) return true;
    if (total_hp() * 100 < total_max() * 60) return true; /* a careful diver turns back early */
    int ratio;
    lowest_ratio_diver(&ratio);
    if (ratio < 300 && heal_slot(1 | 4) < 0) return true;
    if (weapon_uses() < 10) return true;
    if (P.npearl > 0) return true;
    if (dive_t > 60 * 60 * 6) return true;
    return false;
}

/* the regions to grind in for a level goal */
static unsigned grind_regions(int goal) {
    if (goal <= 3) return 1u << RG_SHELF;
    if (goal <= 5) return (1u << RG_SHELF) | (1u << RG_GUMWELL);
    if (goal <= 6) return (1u << RG_GUMWELL) | (1u << RG_SHELF);
    if (goal <= 8) return (1u << RG_SHRINE) | (1u << RG_GUMWELL);
    if (goal <= 10) return (P.doors >> DOOR_WARDEN) & 1 ? (1u << RG_LANTERN) | (1u << RG_SHRINE) : (1u << RG_SHRINE);
    if (goal <= 12) return (1u << RG_LANTERN) | (1u << RG_STILT);
    return 1u << RG_DEEP;
}

static int grind_goal_for_level(void) {
    /* the grind goal of the last grind task not yet done, else the level + 1 */
    int k = current_task();
    for (int i = k; i < NTASK; i++)
        if (PLAN[i].kind == TK_GRIND) return PLAN[i].arg;
    return P.level + 1;
}

/* targets for the field: returns how many cells */
static int16_t tcs[256], trs[256];

static int targets_for(int task, int *key) {
    int n = 0;
    const Task *t = task < NTASK ? &PLAN[task] : NULL;
    if (!t) return 0;
    switch (t->kind) {
    case TK_LEVER: tcs[0] = sdg_mi.lever[t->arg].c; trs[0] = sdg_mi.lever[t->arg].r; n = 1; *key = 1000 + t->arg; break;
    case TK_CHEST: tcs[0] = sdg_mi.chest[t->arg].c; trs[0] = sdg_mi.chest[t->arg].r; n = 1; *key = 2000 + t->arg; break;
    case TK_HEAD: tcs[0] = sdg_mi.head[t->arg].c; trs[0] = sdg_mi.head[t->arg].r; n = 1; *key = 3000 + t->arg; break;
    case TK_BOSS:
        if (t->arg == 1) { tcs[0] = sdg_mi.warden.c; trs[0] = sdg_mi.warden.r; }
        else { tcs[0] = sdg_mi.gloam.c; trs[0] = sdg_mi.gloam.r; }
        n = 1;
        *key = 4000 + t->arg;
        break;
    case TK_BOMB:
        for (int r = 0; r < SDG_MH && n < 256; r++)
            for (int c = 0; c < SDG_MW && n < 256; c++) {
                if (sdg_mi.wall_of[r][c] != t->arg) continue;
                for (int dr = -2; dr <= 2; dr++)
                    for (int dc = -2; dc <= 2; dc++) {
                        int nc = c + dc, nr = r + dr;
                        if (n >= 255 || iabs(dc) + iabs(dr) > 2 || !sdg_cell_open(nc, nr)) continue;
                        tcs[n] = (int16_t)nc;
                        trs[n++] = (int16_t)nr;
                    }
            }
        *key = 5000 + t->arg;
        break;
    default: break;
    }
    return n;
}

static int grind_targets(unsigned regions) {
    int n = 0;
    for (int i = 0; i < sdg.dive.nmob && n < 256; i++) {
        const SdgMob *m = &sdg.dive.mob[i];
        if (!((regions >> m->region) & 1)) continue;
        if (m->kind == MK_WARDEN || m->kind == MK_GLOAM || m->kind == MK_HAUNT) continue;
        if (m->state == MS_GONE) continue;
        if ((m->kind == MK_SMOGVENT || m->kind == MK_CLAMPVENT) && m->state != MS_ACTIVE) {
            /* a vent: wait by it */
        }
        if (m->kind == MK_NEST && P.level < 6) continue;
        if (m->kind == MK_WHORL && P.level < 7) continue;
        tcs[n] = (int16_t)((m->x >> 8) / SDG_T);
        trs[n++] = (int16_t)((m->y >> 8) / SDG_T);
    }
    return n;
}

static bool can_do(int task) {
    if (task >= NTASK) return false;
    const Task *t = &PLAN[task];
    if (P.level < t->minlv) return false;
    if (t->kind == TK_BOMB) {
        bool has = false;
        for (int d = 0; d < 3; d++)
            for (int s = 0; s < 2; s++)
                if (P.equip[d][s] == IT_BOMB && P.uses[d][s] && P.hp[d] > 0) has = true;
        if (!has) return false;
    }
    return true;
}

static unsigned use_menu_for(int slot, int target) {
    /* open the item menu: the dive handles B */
    (void)slot;
    (void)target;
    return press(BTN_B);
}

static int menu_want_slot = -1, menu_want_target = -1;

static unsigned bot_dive(void) {
    dive_t++;
    if (sdg.scene_t <= 1) return 0;
    int task = current_task();
    int ratio;
    int low = lowest_ratio_diver(&ratio);
    /* mend with a tonic when someone is low */
    if (ratio < 450 && low >= 0) {
        int hs = heal_slot(1 | 4);
        if (hs >= 0) { menu_want_slot = hs; menu_want_target = low; return use_menu_for(hs, low); }
    }
    for (int d = 0; d < 3; d++)
        if (P.hp[d] <= 0) {
            int es = heal_slot(2);
            if (es >= 0) { menu_want_slot = es; menu_want_target = d; return use_menu_for(es, d); }
        }
    int key = -1, n = 0;
    bool home = should_surface() || grind_dry;
    bool grinding = false;
    if (home) {
        for (int c = SDG_SURF_C0; c <= SDG_SURF_C1; c++) { tcs[n] = (int16_t)c; trs[n++] = SDG_SURF_ROW; }
        key = 1;
    } else if (can_do(task) && PLAN[task].kind != TK_GRIND && task != blocked_task) {
        n = targets_for(task, &key);
    }
    if (n == 0 && !home) {
        int goal = task < NTASK && PLAN[task].kind == TK_GRIND ? PLAN[task].arg : grind_goal_for_level();
        /* a task we can't do yet (no charge, too low): go home to shop, or grind */
        if (task < NTASK && PLAN[task].kind == TK_BOMB && P.level >= PLAN[task].minlv && P.owned[IT_BOMB] && !can_do(task)) {
            for (int c = SDG_SURF_C0; c <= SDG_SURF_C1; c++) { tcs[n] = (int16_t)c; trs[n++] = SDG_SURF_ROW; }
            key = 1;
            home = true;
        } else {
            if (task < NTASK && PLAN[task].kind != TK_GRIND && P.level >= PLAN[task].minlv) goal = imax(goal, P.level + 1);
            n = grind_targets(grind_regions(goal));
            key = 9000;
            grinding = true;
        }
    }
    if (n == 0) {
        for (int c = SDG_SURF_C0; c <= SDG_SURF_C1; c++) { tcs[n] = (int16_t)c; trs[n++] = SDG_SURF_ROW; }
        key = 1;
        home = true;
    }
    field_age++;
    if (key != field_key || field_doors != P.doors || field_walls != P.walls || (grinding && field_age > 20)) {
        build_field(tcs, trs, n);
        field_key = key;
        field_age = 0;
        field_doors = P.doors;
        field_walls = P.walls;
    }
    int dist = 0;
    unsigned m = steer(&dist);
    int x = (int)(sdg.dive.x >> 8), y = (int)(sdg.dive.y >> 8);
    if (dist == 32767) {
        /* unreachable from here: leave that task for this dive and grind */
        if (!home && !grinding) blocked_task = task;
        else if (grinding) grind_dry = 1; /* nothing left to fight that can be reached: home */
        field_key = -1;
        return 0;
    }
    /* not yet away from the raft: dive a little before turning back */
    if (home && !sdg.dive.left_surface) return BTN_DOWN;
    if (home && y / SDG_T <= SDG_SURF_ROW + 1) m = BTN_UP | (m & (BTN_LEFT | BTN_RIGHT));
    if (dist == 0 && !home && !grinding) {
        const Task *t = &PLAN[task];
        if (t->kind == TK_BOMB) {
            /* the charge: open the menu, use it */
            for (int d = 0; d < 3; d++)
                for (int s = 0; s < 2; s++)
                    if (P.equip[d][s] == IT_BOMB && P.uses[d][s] && P.hp[d] > 0) { menu_want_slot = d * 2 + s; menu_want_target = d; }
            return press(BTN_B);
        }
        if (t->kind == TK_CHEST || t->kind == TK_LEVER) return press(BTN_A) | m;
    }
    /* unstick */
    if (iabs(x - stuck_x) + iabs(y - stuck_y) > 2) { stuck_x = x; stuck_y = y; stuck_t = 0; }
    else if (m) stuck_t++;
    if (stuck_t > 90) { wiggle = 20; stuck_t = 0; }
    if (wiggle > 0) {
        wiggle--;
        static const unsigned W[4] = {BTN_UP, BTN_LEFT, BTN_DOWN, BTN_RIGHT};
        return W[(dive_t / 20) & 3];
    }
    return m;
}

static unsigned bot_menu(void) {
    if (sdg.scene_t <= 1) return 0;
    if (menu_want_slot < 0) return press(BTN_B);
    if (!sdg.menu_pick) {
        if (sdg.menu_sel != menu_want_slot) {
            if (sdg.menu_sel / 2 != menu_want_slot / 2) return press(BTN_DOWN);
            return press(BTN_RIGHT);
        }
        int it = P.equip[menu_want_slot / 2][menu_want_slot % 2];
        if (!sdg_dive_usable(it) || P.uses[menu_want_slot / 2][menu_want_slot % 2] == 0) { menu_want_slot = -1; return press(BTN_B); }
        int k = SDG_ITEM[it].kind;
        unsigned a = press(BTN_A);
        if (a && (k == K_BOMB || k == K_MIST)) menu_want_slot = -1;
        return a;
    }
    if (sdg.menu_target != menu_want_target) return press(BTN_RIGHT);
    unsigned a = press(BTN_A);
    if (a) menu_want_slot = -1;
    return a;
}

/* ---- the raft, the shop, the kit ----------------------------------------------- */

static unsigned bot_raft(void) {
    if (sdg.scene_t <= 2) return 0;
    uint8_t want[3][2];
    want_loadout(want);
    int goal = 2;
    if (want_buy()) goal = 0;
    else if (memcmp(want, P.equip, 6) != 0) goal = 1;
    if (sdg.raft_sel != goal) return press(BTN_RIGHT);
    if (goal == 2) { dive_t = 0; field_key = -1; blocked_task = -1; grind_dry = 0; }
    return press(BTN_A);
}

static unsigned bot_shop(void) {
    if (sdg.scene_t <= 2) return 0;
    int it = want_buy();
    if (!it) return press(BTN_B);
    int page = SDG_ITEM[it].shop;
    if (sdg.shop_page != page) return press(BTN_RIGHT);
    uint8_t list[16];
    int n = sdg_shop_list(page, list, 16), idx = 0;
    for (int k = 0; k < n; k++)
        if (list[k] == it) idx = k;
    if (sdg.shop_sel != idx) return press(BTN_DOWN);
    return press(BTN_A);
}

static unsigned kit_goto(int k) {
    /* to grid slot k (0..5, 6 return) from wherever the cursor is */
    if (sdg.kit_col == 0) return press(BTN_RIGHT);
    if (sdg.kit_sel == k) return 0;
    int r0 = sdg.kit_sel >= 6 ? 3 : sdg.kit_sel / 2, r1 = k >= 6 ? 3 : k / 2;
    if (r0 < r1) return press(BTN_DOWN);
    if (r0 > r1) return press(BTN_UP);
    if ((sdg.kit_sel & 1) < (k & 1)) return press(BTN_RIGHT);
    return press(BTN_LEFT);
}

static unsigned bot_kit(void) {
    if (sdg.scene_t <= 2) return 0;
    uint8_t want[3][2];
    want_loadout(want);
    /* holding something: put it where it belongs, or back */
    if (sdg.kit_held) {
        int dest = -1;
        for (int k = 0; k < 6 && dest < 0; k++)
            if (want[k / 2][k % 2] == sdg.kit_held && P.equip[k / 2][k % 2] != sdg.kit_held) dest = k;
        if (dest < 0) dest = sdg.kit_from >= 0 ? sdg.kit_from : -2;
        if (dest == -2) return press(BTN_B);
        unsigned m = kit_goto(dest);
        return m ? m : press(BTN_A);
    }
    /* first clear every hand that holds the wrong thing */
    for (int k = 0; k < 6; k++) {
        int have = P.equip[k / 2][k % 2], w = want[k / 2][k % 2];
        if (have && have != w) {
            unsigned m = kit_goto(k);
            if (m) return m;
            return press(BTN_A); /* pick it up; next it goes back into itself (storage) */
        }
    }
    /* then fill the empty ones from storage */
    for (int k = 0; k < 6; k++) {
        int have = P.equip[k / 2][k % 2], w = want[k / 2][k % 2];
        if (!have && w && sdg_stored(&P, w) > 0) {
            int n = 0, idx = -1;
            for (int it = 1; it < IT_COUNT; it++)
                if (P.owned[it] > 0) { if (it == w) idx = n; n++; }
            if (idx < 0) break;
            if (sdg.kit_col == 1) {
                if (sdg.kit_sel >= 6 || (sdg.kit_sel & 1)) return kit_goto(sdg.kit_sel >= 6 ? 4 : sdg.kit_sel - 1);
                return press(BTN_LEFT);
            }
            if (sdg.kit_sel != idx) return press(sdg.kit_sel < idx ? BTN_DOWN : BTN_UP);
            return press(BTN_A);
        }
    }
    return press(BTN_B);
}

/* ---- the fights ------------------------------------------------------------------ */

typedef struct Plan { int slot, mode, target; } BPlan;

static BPlan choose(int d) {
    const SdgBattle *B = &sdg.bat;
    BPlan pl = {2, MODE_ATTACK, 0};
    int ratio;
    int low = lowest_ratio_diver(&ratio);
    /* revive */
    for (int s = 0; s < 2; s++) {
        int it = P.equip[d][s];
        if (it && SDG_ITEM[it].kind == K_EGG && P.uses[d][s])
            for (int t = 0; t < 3; t++)
                if (P.hp[t] <= 0) { pl.slot = s; pl.mode = MODE_HEAL; pl.target = t; return pl; }
    }
    /* mend */
    if (ratio < 420) {
        for (int s = 0; s < 2; s++) {
            int it = P.equip[d][s];
            if (!it || !P.uses[d][s]) continue;
            if ((SDG_ITEM[it].kind == K_POTION && it != IT_EVIL) || (SDG_ITEM[it].flags & IF_HOLY)) {
                pl.slot = s;
                pl.mode = MODE_HEAL;
                pl.target = low;
                return pl;
            }
        }
    }
    /* guard someone a creature is looking at */
    for (int i = 0; i < B->nfoe; i++)
        if (B->foe[i].alive && B->foe[i].mark >= 0)
            for (int s = 0; s < 2; s++) {
                int it = P.equip[d][s];
                if (it && SDG_ITEM[it].kind == K_SHIELD && P.uses[d][s]) { pl.slot = s; pl.mode = MODE_DEFEND; pl.target = B->foe[i].mark; return pl; }
            }
    /* the charge on a big one */
    for (int s = 0; s < 2; s++) {
        int it = P.equip[d][s];
        if (it == IT_BOMB && P.uses[d][s] && B->boss) {
            for (int i = 0; i < B->nfoe; i++)
                if (B->foe[i].alive && (SDG_FOE[B->foe[i].kind].flags & FF_BOSS)) { pl.slot = s; pl.mode = MODE_ATTACK; pl.target = i; return pl; }
        }
    }
    /* strike: a weak spot first, the weakest creature on it; else the best blow on the weakest */
    int best_s = -1, best_t = -1, bv = -1;
    for (int s = 0; s < 2; s++) {
        int it = P.equip[d][s];
        if (!sdg_is_weapon(it) || !P.uses[d][s]) continue;
        const SdgItem *I = &SDG_ITEM[it];
        for (int i = 0; i < B->nfoe; i++) {
            if (!B->foe[i].alive) continue;
            int dmg = sdg_attack_damage(I->power, sdg_diver_level(&P, d), I->el, B->foe[i].weak, 100);
            /* thorny creatures hurt a spear or hammer: a little less keen */
            if ((SDG_FOE[B->foe[i].kind].flags & FF_THORNS) && I->kind != K_SHIELD) dmg = dmg * 8 / 10;
            int v = dmg * 4 - B->foe[i].hp / 8 + (dmg >= B->foe[i].hp ? 4000 : 0);
            if (v > bv) { bv = v; best_s = s; best_t = i; }
        }
    }
    if (best_s >= 0) { pl.slot = best_s; pl.mode = MODE_ATTACK; pl.target = best_t; return pl; }
    /* nothing to strike with: wait, or run when it is going badly */
    pl.slot = !B->boss && total_hp() * 100 < total_max() * 50 ? 3 : 2;
    return pl;
}

static BPlan cur_plan;
static int plan_for = -1, plan_round = -1;

static unsigned bot_battle(void) {
    const SdgBattle *B = &sdg.bat;
    if (B->nmsg > 0 || B->phase == BP_WIN || B->phase == BP_LOSE || B->phase == BP_FLED) return press(BTN_A);
    if (B->phase != BP_ORDER && B->phase != BP_TARGET) return 0;
    int d = B->cur;
    if (d < 0) return 0;
    if (plan_for != d || plan_round != B->round) {
        cur_plan = choose(d);
        /* a hopeless fight: run */
        int ratio;
        lowest_ratio_diver(&ratio);
        if (!B->boss && total_hp() * 100 < total_max() * 20 && heal_slot(1 | 4) < 0) cur_plan.slot = 3;
        plan_for = d;
        plan_round = B->round;
    }
    if (B->phase == BP_ORDER) {
        if (B->menu != cur_plan.slot) return press(BTN_DOWN);
        return press(BTN_A);
    }
    /* the target screen */
    int it = P.equip[d][B->menu];
    bool weapon = sdg_is_weapon(it);
    int want_mode = cur_plan.mode;
    if (weapon) {
        bool at = B->tmode == MODE_ATTACK;
        if ((want_mode == MODE_ATTACK) != at) return press(BTN_UP);
    }
    if (B->tsel != cur_plan.target) {
        if (want_mode == MODE_ATTACK && (cur_plan.target < 0 || !B->foe[cur_plan.target].alive)) return press(BTN_A);
        return press(BTN_RIGHT);
    }
    return press(BTN_A);
}

/* ---- every frame ----------------------------------------------------------------- */

unsigned sdg_bot(void) {
    unsigned m = 0;
    sdg_bot_task = current_task();
    switch (sdg.scene) {
    case SC_TITLE:
        if (sdg.scene_t > 20) m = press(BTN_A);
        break;
    case SC_RAFT: m = bot_raft(); break;
    case SC_SHOP: m = bot_shop(); break;
    case SC_KIT: m = bot_kit(); break;
    case SC_DIVE: m = bot_dive(); break;
    case SC_MENU: m = bot_menu(); break;
    case SC_BATTLE: m = bot_battle(); break;
    case SC_SURFACE:
        if (sdg.scene_t > 24) m = press(BTN_A);
        break;
    case SC_WIPE:
        if (sdg.scene_t > 64) m = press(BTN_A);
        break;
    case SC_ENDING:
        if (sdg.ending_t > 604) m = press(BTN_A);
        break;
    default: break;
    }
    if (sdg_bot_debug && (sdg.frame % 3600) == 0)
        fprintf(stderr, "bot f%u scene %d task %d lv %d xp %ld gold %ld hp %d/%d/%d at %d,%d deepest %d wins %d dives %d\n",
                (unsigned)sdg.frame, sdg.scene, sdg_bot_task, P.level, (long)P.xp, (long)P.gold, P.hp[0], P.hp[1], P.hp[2],
                (int)(sdg.dive.x >> 8) / SDG_T, (int)(sdg.dive.y >> 8) / SDG_T, P.deepest, P.wins, P.dives);
    if (sdg_bot_debug >= 2)
        fprintf(stderr, "  f%u sc %d st %d m %02x x %d y %d menu %d/%d want %d/%d field %d\n", (unsigned)sdg.frame, sdg.scene, sdg.scene_t, m,
                (int)(sdg.dive.x >> 8), (int)(sdg.dive.y >> 8), sdg.menu_sel, sdg.menu_pick, menu_want_slot, menu_want_target, field_key);
    last = m;
    return m;
}
