/* HOMESPUN - the camp: what everything costs, what it makes and how fast,
 * the stores and their limits, and the menus at each building. Production
 * is plain arithmetic on elapsed milliseconds (hs_produce), so an hour
 * handed over at once comes out the same as an hour of single frames.
 * Every number and its source is in docs/games/44-homespun.md. */
#include "homespun.h"

HsSave sv;
HsMenu hm;

/*                      name            what                          bars data thread */
const HsResearch HS_RESEARCH[RS_COUNT] = {
    {"STEEL YO-YO",   "YO-YO HITS FOR 2",             150,    0,   0},
    {"FERTILIZER",    "BUDS: 1 GLINT EVERY 1.5 S",      0,  100,   0},
    {"BIG BINS",      "BINS CAN BE RAISED",           200,    0, 150},
    {"PEP PILL",      "ANVILS: 2 BARS A MINUTE",      100,  300, 100},
    {"STAR YO-YO",    "YO-YO HITS FOR 3",               0,  500, 200},
    {"STARFUEL",      "THE TUMBLEWEED CAN FLY",      1000, 1000,   0},
};

int hs_plant_cost(int n) { return 10 << iclamp(n, 0, 20); }

int hs_bin_level(void) { return sv.bins[0] + sv.bins[1]; }

int hs_cap(int res) {
    int l = 1 + hs_bin_level();
    switch (res) {
    case RES_GLINT: return 2500 * l;
    case RES_JERKY: return 5 * l;
    default: return 250 * l;
    }
}

int hs_hands(void) { return sv.huts[0] + sv.huts[1]; }

int hs_anvil_hands(void) {
    int n = 0;
    for (int i = 0; i < HS_ANVILS; i++) n += (sv.anvil_hand >> i) & 1;
    return n;
}

int hs_free_hands(void) { return hs_hands() - hs_anvil_hands() - sv.lab_hands; }

int hs_yoyo_dmg(void) {
    if (sv.research >> RS_STAR & 1) return 3;
    if (sv.research >> RS_METAL & 1) return 2;
    return 1;
}

int hs_hit_cost_s(int base_s) { return imax(1, base_s - 2 * sv.stone[ST_HIDE]); }
int hs_jerky_seconds(void) { return JERKY_SECONDS + 20 * sv.stone[ST_HUNGER]; }

void hs_add(int res, int n) {
    int cap = hs_cap(res);
    int64_t v = (int64_t)sv.res[res] + n;
    if (v > cap) v = sv.res[res] > cap ? sv.res[res] : cap; /* over the cap already: keep it, add none */
    if (v < 0) v = 0;
    sv.res[res] = (int32_t)v;
}

bool hs_pay(int bars, int glints, int data, int thread) {
    if (sv.res[RES_BAR] < bars || sv.res[RES_GLINT] < glints || sv.res[RES_DATA] < data || sv.res[RES_THREAD] < thread)
        return false;
    sv.res[RES_BAR] -= bars;
    sv.res[RES_GLINT] -= glints;
    sv.res[RES_DATA] -= data;
    sv.res[RES_THREAD] -= thread;
    return true;
}

/* ---- real-time production -------------------------------------------------------- */

/* period scaled by the HASTE stone: bars 5 % faster per mark, the rest 10 % */
static uint32_t haste(uint32_t ms, int pct) { return ms - ms * (uint32_t)(pct * sv.stone[ST_HASTE]) / 100u; }

/* acc += ms * workers; returns the units made; keeps the remainder */
static int64_t tick(uint32_t *acc, uint32_t ms, int workers, uint32_t period) {
    if (workers <= 0 || period == 0) return 0;
    uint64_t a = (uint64_t)*acc + (uint64_t)ms * (uint64_t)workers;
    int64_t made = (int64_t)(a / period);
    *acc = (uint32_t)(a % period);
    return made;
}

static void add_made(int res, int64_t n) {
    if (n <= 0) return;
    hs_add(res, (int)(n > 2000000000 ? 2000000000 : n));
}

void hs_produce(uint32_t ms) {
    if (!sv.started) return;
    /* glintbuds */
    uint32_t pp = haste((sv.research >> RS_FERT & 1) ? PLANT_FERT_MS : PLANT_MS, 10);
    add_made(RES_GLINT, tick(&sv.acc_plant, ms, sv.plants, pp));
    /* the smokehouse fills its racks, up to six strips */
    if (sv.smoke_built) {
        if (sv.smoke_stock >= HS_SMOKE_MAX) sv.acc_smoke = 0;
        else {
            int64_t n = tick(&sv.acc_smoke, ms, 1, haste(SMOKE_MS, 10));
            int room = HS_SMOKE_MAX - sv.smoke_stock;
            sv.smoke_stock = (uint8_t)(sv.smoke_stock + (n > room ? room : n));
            if (sv.smoke_stock >= HS_SMOKE_MAX) sv.acc_smoke = 0;
        }
    }
    /* hands at anvils */
    uint32_t ap = haste((sv.research >> RS_PILL & 1) ? ANVIL_PILL_MS : ANVIL_MS, 5);
    add_made(RES_BAR, tick(&sv.acc_anvil, ms, hs_anvil_hands(), ap));
    /* hands at the Thinker */
    if (sv.lab_fixed) add_made(RES_DATA, tick(&sv.acc_lab, ms, sv.lab_hands, haste(LAB_MS, 10)));
    /* Mother Loom */
    if (sv.loom_home) add_made(RES_THREAD, tick(&sv.acc_loom, ms, 1, LOOM_MS));
}

/* ---- camp spots ------------------------------------------------------------------ */

/* x, y, w, h in field pixels; solid. Camp is laid out in three bands with a
 * corridor between each: the tree, the gate and the ship at the top; huts,
 * anvils, the Glowstone, the workbench, the nest and the hopstone in the
 * middle; more huts and anvils, the smokehouse, the bins, the standing
 * stones, Dr. Orrery and the Thinker at the bottom. */
const HsSpot HS_SPOT[P_COUNT] = {
    [P_BURL] = {16, 16, 32, 32, 1},
    [P_TOLLY] = {184, 16, 16, 16, 1},
    [P_SHIP] = {216, 16, 80, 32, 1},
    [P_HUT1] = {16, 64, 24, 24, 1},
    [P_ANVIL1] = {56, 64, 16, 16, 1},
    [P_ANVIL2] = {80, 64, 16, 16, 1},
    [P_ANVIL3] = {104, 64, 16, 16, 1},
    [P_STONE] = {144, 64, 32, 32, 1},
    [P_BENCH] = {200, 64, 32, 16, 1},
    [P_NEST] = {240, 64, 24, 24, 1},
    [P_PAD] = {272, 64, 16, 16, 0},
    [P_HUT2] = {16, 112, 24, 24, 1},
    [P_ANVIL4] = {56, 112, 16, 16, 1},
    [P_ANVIL5] = {80, 112, 16, 16, 1},
    [P_ANVIL6] = {104, 112, 16, 16, 1},
    [P_SMOKE] = {128, 112, 32, 24, 1},
    [P_BIN1] = {168, 112, 16, 24, 1},
    [P_BIN2] = {192, 112, 16, 24, 1},
    [P_STONE_HASTE] = {216, 112, 8, 16, 1},
    [P_STONE_HIDE] = {232, 112, 8, 16, 1},
    [P_STONE_HUNGER] = {248, 112, 8, 16, 1},
    [P_ORRERY] = {264, 112, 16, 16, 1},
    [P_LAB] = {288, 104, 16, 32, 1},
};

bool hs_spot_shown(int p) {
    if (p == P_NEST) return sv.loom_home != 0;
    if (p >= P_STONE_HASTE && p <= P_STONE_HUNGER) return sv.escapes > 0 || sv.marks > 0;
    return true;
}

void hs_menu_clear(const char *title) {
    memset(&hm, 0, sizeof hm);
    snprintf(hm.title, sizeof hm.title, "%s", title);
}

void hs_menu_add(const char *label, const char *cost, int act, int arg, bool ok) {
    if (hm.n >= HS_MENU_MAX) return;
    HsMenuLine *l = &hm.line[hm.n++];
    snprintf(l->label, sizeof l->label, "%s", label);
    snprintf(l->cost, sizeof l->cost, "%s", cost ? cost : "");
    l->act = (uint8_t)act;
    l->arg = (uint8_t)arg;
    l->ok = ok;
}

static void cost_text(char *out, int n, int bars, int glints, int data, int thread) {
    out[0] = 0;
    char part[16];
    if (glints) { snprintf(part, sizeof part, "%dG ", glints); strncat(out, part, (size_t)n - strlen(out) - 1); }
    if (bars) { snprintf(part, sizeof part, "%dB ", bars); strncat(out, part, (size_t)n - strlen(out) - 1); }
    if (data) { snprintf(part, sizeof part, "%dD ", data); strncat(out, part, (size_t)n - strlen(out) - 1); }
    if (thread) { snprintf(part, sizeof part, "%dT ", thread); strncat(out, part, (size_t)n - strlen(out) - 1); }
    int l = (int)strlen(out);
    if (l > 0 && out[l - 1] == ' ') out[l - 1] = 0;
}

static bool can(int bars, int glints, int data, int thread) {
    return sv.res[RES_BAR] >= bars && sv.res[RES_GLINT] >= glints && sv.res[RES_DATA] >= data && sv.res[RES_THREAD] >= thread;
}

static bool research_open(int r) {
    if (sv.research >> r & 1) return false;
    if (r == RS_STAR && !(sv.research >> RS_METAL & 1)) return false;
    if (r == RS_FUEL && !(sv.research >> RS_BIGBIN & 1)) return false;
    return true;
}

void hs_base_menu(int p) {
    char c[24], l[28];
    switch (p) {
    case P_BURL:
        hs_menu_clear("OLD BURL");
        if (sv.plants < HS_PLANTS) {
            int k = hs_plant_cost(sv.plants);
            snprintf(c, sizeof c, "%dG", k);
            hs_menu_add("PLANT A GLINTBUD", c, M_PLANT, 0, sv.res[RES_GLINT] >= k);
        } else {
            hs_menu_add("ALL ELEVEN BUDS GROW", "", M_NOTHING, 0, false);
        }
        snprintf(l, sizeof l, "BUDS GROWING: %d/%d", sv.plants, HS_PLANTS);
        hs_menu_add(l, "", M_NOTHING, 0, false);
        break;
    case P_BENCH:
        hs_menu_clear("WORKBENCH");
        snprintf(c, sizeof c, "%dG", GLINTS_PER_BAR);
        hs_menu_add("CRAFT A BAR", c, M_CRAFT, 0, sv.res[RES_GLINT] >= GLINTS_PER_BAR && sv.res[RES_BAR] < hs_cap(RES_BAR));
        break;
    case P_SHIP: {
        hs_menu_clear("THE TUMBLEWEED");
        snprintf(l, sizeof l, "SHIP PARTS: %d/3", hs_parts_count());
        hs_menu_add(l, "", M_NOTHING, 0, false);
        hs_menu_add((sv.research >> RS_FUEL & 1) ? "STARFUEL: IN THE TANK" : "STARFUEL: NONE", "", M_NOTHING, 0, false);
        bool ready = hs_parts_count() == 3 && (sv.research >> RS_FUEL & 1);
        if (ready) hs_menu_add("LAUNCH FOR HOME", "", M_LAUNCH, 0, true);
        break;
    }
    case P_SMOKE:
        hs_menu_clear("GRISTLE'S SMOKEHOUSE");
        if (!sv.smoke_built) {
            snprintf(c, sizeof c, "%dB", SMOKE_BUILD);
            hs_menu_add("BUILD THE SMOKEHOUSE", c, M_SMOKE_BUILD, 0, sv.res[RES_BAR] >= SMOKE_BUILD);
        } else {
            snprintf(c, sizeof c, "%dG", JERKY_PRICE);
            hs_menu_add("BUY A STRIP OF JERKY", c, M_JERKY, 0,
                        sv.smoke_stock > 0 && sv.res[RES_GLINT] >= JERKY_PRICE && sv.res[RES_JERKY] < hs_cap(RES_JERKY));
            snprintf(l, sizeof l, "ON THE RACKS: %d/%d", sv.smoke_stock, HS_SMOKE_MAX);
            hs_menu_add(l, "", M_NOTHING, 0, false);
        }
        break;
    case P_HUT1: case P_HUT2: {
        int h = p - P_HUT1, lv = sv.huts[h];
        static const int COST[3] = {HUT_COST0, HUT_COST1, HUT_COST2};
        hs_menu_clear(h ? "EAST HUT" : "WEST HUT");
        if (lv < 3) {
            snprintf(c, sizeof c, "%dB", COST[lv]);
            hs_menu_add(lv == 0 ? "BUILD A HUT" : "ADD A ROOM", c, M_HUT, h, sv.res[RES_BAR] >= COST[lv]);
        }
        snprintf(l, sizeof l, "HANDS LIVING HERE: %d", lv);
        hs_menu_add(l, "", M_NOTHING, 0, false);
        break;
    }
    case P_BIN1: case P_BIN2: {
        int b = p - P_BIN1;
        hs_menu_clear(b ? "EAST BIN" : "WEST BIN");
        if (sv.bins[b] == 0) {
            snprintf(c, sizeof c, "%dB", BIN_COST);
            hs_menu_add("BUILD A BIN", c, M_BIN, b, sv.res[RES_BAR] >= BIN_COST);
        } else if (sv.bins[b] == 1 && (sv.research >> RS_BIGBIN & 1)) {
            snprintf(c, sizeof c, "%dB", BIN_RAISE);
            hs_menu_add("RAISE THE BIN", c, M_BIN, b, sv.res[RES_BAR] >= BIN_RAISE);
        } else if (sv.bins[b] == 1) {
            hs_menu_add("RAISING IT NEEDS BIG BINS", "", M_NOTHING, 0, false);
        } else {
            hs_menu_add("AS BIG AS BINS GET", "", M_NOTHING, 0, false);
        }
        break;
    }
    case P_ANVIL1: case P_ANVIL2: case P_ANVIL3: case P_ANVIL4: case P_ANVIL5: case P_ANVIL6: {
        int a = p - P_ANVIL1;
        hs_menu_clear("ANVIL");
        if (!(sv.anvils >> a & 1)) {
            snprintf(c, sizeof c, "%dB", ANVIL_COST);
            hs_menu_add("BUILD AN ANVIL", c, M_ANVIL_BUILD, a, sv.res[RES_BAR] >= ANVIL_COST);
        } else if (!(sv.anvil_hand >> a & 1)) {
            snprintf(c, sizeof c, "%d FREE", hs_free_hands());
            hs_menu_add("PUT A HAND HERE", c, M_ANVIL_HAND, a, hs_free_hands() > 0);
        } else {
            hs_menu_add("SEND THE HAND HOME", "", M_ANVIL_FREE, a, true);
        }
        break;
    }
    case P_LAB:
        hs_menu_clear("THE THINKER");
        if (!sv.lab_fixed) {
            hs_menu_add("IT NEEDS A GEAR", "", M_NOTHING, 0, false);
        } else {
            snprintf(c, sizeof c, "%d FREE", hs_free_hands());
            snprintf(l, sizeof l, "PUT A HAND HERE (%d/6)", sv.lab_hands);
            hs_menu_add(l, c, M_LAB_ADD, 0, hs_free_hands() > 0 && sv.lab_hands < 6);
            hs_menu_add("TAKE A HAND AWAY", "", M_LAB_SUB, 0, sv.lab_hands > 0);
        }
        break;
    case P_ORRERY:
        hs_menu_clear("DR. ORRERY");
        for (int r = 0; r < RS_COUNT; r++) {
            if (!research_open(r)) continue;
            const HsResearch *rs = &HS_RESEARCH[r];
            cost_text(c, sizeof c, rs->bars, 0, rs->data, rs->thread);
            hs_menu_add(rs->name, c, M_RESEARCH, r, can(rs->bars, 0, rs->data, rs->thread));
        }
        snprintf(c, sizeof c, "%dB", TRADE_BARS);
        snprintf(l, sizeof l, "TRADE FOR %d DATA", TRADE_DATA);
        hs_menu_add(l, c, M_TRADE, 0, sv.res[RES_BAR] >= TRADE_BARS && sv.res[RES_DATA] < hs_cap(RES_DATA));
        break;
    case P_STONE_HASTE: case P_STONE_HIDE: case P_STONE_HUNGER: {
        static const char *const N[3] = {"THE HASTE STONE", "THE HIDE STONE", "THE HUNGER STONE"};
        static const char *const W[3] = {"CAMP WORKS FASTER", "HITS COST 2 S LESS", "JERKY LASTS 20 S MORE"};
        int st = p - P_STONE_HASTE;
        hs_menu_clear(N[st]);
        hs_menu_add(W[st], "", M_NOTHING, 0, false);
        snprintf(l, sizeof l, "MARKS CARVED: %d/5", sv.stone[st]);
        hs_menu_add(l, "", M_NOTHING, 0, false);
        snprintf(c, sizeof c, "%d LEFT", sv.marks - sv.spent_marks);
        hs_menu_add("CARVE A MARK", c, M_STONE, st, sv.marks > sv.spent_marks && sv.stone[st] < 5);
        break;
    }
    case P_PAD: {
        hs_menu_clear("CAMP HOPSTONE");
        int any = 0;
        for (int i = 1; i < HS_PADS; i++)
            if (sv.pads >> i & 1) {
                snprintf(l, sizeof l, "HOP TO STONE %d", i);
                hs_menu_add(l, sv.res[RES_JERKY] > 0 ? "" : "NO JERKY", M_HOP, i, sv.res[RES_JERKY] > 0);
                any = 1;
            }
        if (!any) hs_menu_add("NO OTHER STONE IS AWAKE", "", M_NOTHING, 0, false);
        break;
    }
    default:
        return;
    }
    hm.open = 1;
    hm.sel = 0;
    for (int i = 0; i < hm.n; i++)
        if (hm.line[i].act != M_NOTHING) { hm.sel = i; break; }
}

bool hs_base_act(int act, int arg) {
    switch (act) {
    case M_PLANT: {
        int k = hs_plant_cost(sv.plants);
        if (sv.plants >= HS_PLANTS || !hs_pay(0, k, 0, 0)) return false;
        sv.plants++;
        return true;
    }
    case M_CRAFT:
        if (sv.res[RES_BAR] >= hs_cap(RES_BAR) || !hs_pay(0, GLINTS_PER_BAR, 0, 0)) return false;
        hs_add(RES_BAR, 1);
        return true;
    case M_SMOKE_BUILD:
        if (sv.smoke_built || !hs_pay(SMOKE_BUILD, 0, 0, 0)) return false;
        sv.smoke_built = 1;
        hs_add(RES_JERKY, 1); /* Gristle hands over the first strip for free */
        sv.smoke_stock = 0;
        sv.acc_smoke = 0;
        return true;
    case M_JERKY:
        if (sv.smoke_stock == 0 || sv.res[RES_JERKY] >= hs_cap(RES_JERKY) || !hs_pay(0, JERKY_PRICE, 0, 0)) return false;
        sv.smoke_stock--;
        hs_add(RES_JERKY, 1);
        return true;
    case M_HUT: {
        static const int COST[3] = {HUT_COST0, HUT_COST1, HUT_COST2};
        int h = iclamp(arg, 0, 1);
        if (sv.huts[h] >= 3 || !hs_pay(COST[sv.huts[h]], 0, 0, 0)) return false;
        sv.huts[h]++;
        return true;
    }
    case M_BIN: {
        int b = iclamp(arg, 0, 1);
        if (sv.bins[b] == 0) { if (!hs_pay(BIN_COST, 0, 0, 0)) return false; }
        else if (sv.bins[b] == 1 && (sv.research >> RS_BIGBIN & 1)) { if (!hs_pay(BIN_RAISE, 0, 0, 0)) return false; }
        else return false;
        sv.bins[b]++;
        return true;
    }
    case M_ANVIL_BUILD:
        if ((sv.anvils >> arg & 1) || !hs_pay(ANVIL_COST, 0, 0, 0)) return false;
        sv.anvils |= (uint8_t)(1 << arg);
        return true;
    case M_ANVIL_HAND:
        if (!(sv.anvils >> arg & 1) || (sv.anvil_hand >> arg & 1) || hs_free_hands() <= 0) return false;
        sv.anvil_hand |= (uint8_t)(1 << arg);
        return true;
    case M_ANVIL_FREE:
        if (!(sv.anvil_hand >> arg & 1)) return false;
        sv.anvil_hand &= (uint8_t)~(1 << arg);
        return true;
    case M_LAB_ADD:
        if (!sv.lab_fixed || hs_free_hands() <= 0 || sv.lab_hands >= 6) return false;
        sv.lab_hands++;
        return true;
    case M_LAB_SUB:
        if (sv.lab_hands == 0) return false;
        sv.lab_hands--;
        return true;
    case M_RESEARCH: {
        if (arg < 0 || arg >= RS_COUNT || !research_open(arg)) return false;
        const HsResearch *r = &HS_RESEARCH[arg];
        if (!hs_pay(r->bars, 0, r->data, r->thread)) return false;
        sv.research |= (uint8_t)(1 << arg);
        return true;
    }
    case M_TRADE:
        if (sv.res[RES_DATA] >= hs_cap(RES_DATA) || !hs_pay(TRADE_BARS, 0, 0, 0)) return false;
        hs_add(RES_DATA, TRADE_DATA);
        return true;
    case M_STONE:
        if (arg < 0 || arg > 2 || sv.marks <= sv.spent_marks || sv.stone[arg] >= 5) return false;
        sv.stone[arg]++;
        sv.spent_marks++;
        return true;
    default:
        return false;
    }
}
