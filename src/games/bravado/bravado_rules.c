/* BRAVADO - the rules with no drawing in them: the gear and its prices, the
 * shop's sale and hike, the next fight's lineup and upping the stakes. */
#include "bravado.h"

const int8_t BRV_DX[8] = {1, 1, 0, -1, -1, -1, 0, 1};
const int8_t BRV_DY[8] = {0, 1, 1, 1, 0, -1, -1, -1};

/* The original's prices tier by tier (7,900 for everything); the names and
 * words are ours. */
const GearDef BRV_GEAR[GR_COUNT] = {
    {"HEART PLATE", 4, {300, 300, 350, 350},
     {"+4 MOST HEALTH.", "+4 MOST HEALTH.", "+4 MOST HEALTH.", "+4 MOST HEALTH."}},
    {"FIRST AID", 2, {150, 300}, {"A MEDKIT TURNS UP EVERY 30 SECONDS.", "A MEDKIT EVERY 15 SECONDS."}},
    {"BLAST GUARD", 2, {100, 100}, {"SHRUG OFF 1 BLAST EACH FIGHT.", "SHRUG OFF 2 BLASTS EACH FIGHT."}},
    {"SHOT GUARD", 2, {100, 100}, {"SHRUG OFF 1 SHOT EACH FIGHT.", "SHRUG OFF 2 SHOTS EACH FIGHT."}},
    {"QUICK TRIGGER", 2, {150, 250}, {"SHOOT FASTER.", "SHOOT FASTER STILL."}},
    {"HEAVY ROUNDS", 2, {400, 400}, {"HARDER-HITTING SHOTS.", "HARDER-HITTING STILL."}},
    {"FAN FIRE", 2, {350, 350}, {"2 SHOTS AT A TIME.", "3 SHOTS AT A TIME."}},
    {"RICOCHET", 2, {350, 250}, {"SHOTS CAROM OFF 1 WALL.", "SHOTS CAROM OFF 2 WALLS."}},
    {"KICKBACK", 1, {200}, {"SHOTS SHOVE MONSTERS BACK HARDER."}},
    {"BUDDY BOT", 2, {600, 300}, {"A DRONE COPIES YOU, A MOMENT LATER.", "YOUR DRONE TAKES TWICE THE KNOCKS."}},
    {"FIREWALKERS", 2, {150, 150}, {"WADE THROUGH 1 LAVA POOL EACH FIGHT.", "WADE THROUGH 2 LAVA POOLS EACH FIGHT."}},
    {"ROCKET DASH", 1, {350}, {"TAP A TWICE ON THE MOVE: A DASH THAT FLATTENS THINGS."}},
    {"BOMB BAG", 4, {100, 100, 150, 200},
     {"CARRY 3 MORE BOMBS.", "CARRY 3 MORE BOMBS.", "CARRY 3 MORE BOMBS.", "CARRY 3 MORE BOMBS."}},
    {"CLICKER", 1, {300}, {"PRESS B TO SET YOUR BOMBS OFF."}},
    {"NAIL BOMBS", 2, {100, 200}, {"BOMBS SPRAY NAILS (NEVER AT YOU).", "MORE NAILS, AND HARDER ONES."}},
    {"BAIT BOMBS", 1, {350}, {"MONSTERS GO FOR YOUR BOMBS, NOT YOU."}},
};

const char *const BRV_KIND_NAME[MK_ALL] = {
    "MITES", "GASBAGS", "BRUTES", "POWDER KEGS", "STILTERS", "PEEPERS", "SLAG", "PIT BOSS", "FIZZER",
};

/* hit points; a plain shot takes 2 (so a mite is one shot, a powder keg
 * three, a stilter eight) */
const int BRV_KIND_HP[MK_ALL] = {2, 2, 14, 6, 16, 14, 0, BRV_BOSS_HP, 2};

int brv_base_price(int item, int tier) {
    if (item < 0 || item >= GR_COUNT || tier < 0 || tier >= BRV_GEAR[item].tiers) return 0;
    return BRV_GEAR[item].price[tier];
}

bool brv_maxed(int item) { return bv.gear[item] >= BRV_GEAR[item].tiers; }

int brv_price(int item) {
    if (brv_maxed(item)) return 0;
    int p = brv_base_price(item, bv.gear[item]);
    if (item == bv.hike) p += 100;
    if (item == bv.sale) p = imax(50, p - 100);
    return p;
}

int brv_total_gear_cost(void) {
    int t = 0;
    for (int i = 0; i < GR_COUNT; i++)
        for (int k = 0; k < BRV_GEAR[i].tiers; k++) t += BRV_GEAR[i].price[k];
    return t;
}

int brv_stock_bombs(void) { return 1 + 3 * bv.gear[GR_BOMBBAG]; }
int brv_max_health(void) { return BRV_START_HP + 4 * bv.gear[GR_HEART]; }

bool brv_buy(int item) {
    if (item < 0 || item >= GR_COUNT || brv_maxed(item)) return false;
    int p = brv_price(item);
    if (p > bv.cash) return false;
    bv.cash -= p;
    bv.gear[item]++;
    bv.upgrades++;
    /* the sale or the hike was for that tier only */
    if (item == bv.hike) {
        bv.hike = -1;
        bv.hiked_bought++;
    }
    if (item == bv.sale) bv.sale = -1;
    bv.last_bought = item;
    return true;
}

/* One item goes up by 100 and another down by 100 (never under 50) for
 * this visit. On the first visit nothing goes up, and only an item the
 * sale would bring to 100 or less can go on sale. */
void brv_shop_changes(void) {
    int cand[GR_COUNT], n = 0;
    bv.sale = bv.hike = -1;
    bool first = bv.round <= 1;
    if (!first) {
        for (int i = 0; i < GR_COUNT; i++)
            if (!brv_maxed(i)) cand[n++] = i;
        if (n > 0) bv.hike = cand[rng_range(&g_rng, 0, n - 1)];
    }
    n = 0;
    for (int i = 0; i < GR_COUNT; i++) {
        if (brv_maxed(i) || i == bv.hike) continue;
        if (first && imax(50, brv_base_price(i, bv.gear[i]) - 100) > 100) continue;
        cand[n++] = i;
    }
    if (n > 0) bv.sale = cand[rng_range(&g_rng, 0, n - 1)];
}

/* how many monsters a pack of a kind holds (lava: three pools) */
int brv_group_count(int kind) {
    switch (kind) {
    case MK_MITE: case MK_GASBAG: return 9;
    case MK_BRUTE: return rng_range(&g_rng, 3, 6);
    case MK_KEG: return rng_range(&g_rng, 3, 6);
    case MK_STILTER: return rng_range(&g_rng, 2, 5);
    case MK_PEEPER: return rng_range(&g_rng, 2, 4);
    default: return 3;
    }
}

void brv_random_group(Group *g) {
    g->kind = (uint8_t)rng_range(&g_rng, 0, MK_KINDS - 1);
    g->count = (uint8_t)brv_group_count(g->kind);
}

/* The next fight as it first stands: fight 1 is fixed (a pack of mites
 * for 100); fights 2-7 start at one random pack for 100; the last fight is
 * twelve random packs and the boss, for 3,200, and can't be raised. */
void brv_new_lineup(void) {
    bv.ngroups = 0;
    if (bv.round == 0) {
        bv.lineup[0] = (Group){MK_MITE, 9};
        bv.ngroups = 1;
        bv.prize = BRV_STEP;
    } else if (bv.round == BRV_ROUNDS - 1) {
        for (int i = 0; i < BRV_LAST_GROUPS; i++) brv_random_group(&bv.lineup[bv.ngroups++]);
        bv.prize = BRV_LAST_PRIZE;
    } else {
        brv_random_group(&bv.lineup[bv.ngroups++]);
        bv.prize = BRV_STEP;
    }
}

bool brv_raise(void) {
    if (bv.round == 0 || bv.round >= BRV_ROUNDS - 1 || bv.ngroups >= BRV_MAX_GROUPS) return false;
    brv_random_group(&bv.lineup[bv.ngroups++]);
    bv.prize += BRV_STEP;
    return true;
}

int brv_lineup_monsters(void) {
    int n = 0;
    for (int i = 0; i < bv.ngroups; i++)
        if (bv.lineup[i].kind != MK_SLAG) n += bv.lineup[i].count;
    return n;
}

/* Frames between spawns: the bigger the prize, the quicker they come, up to
 * a prize of 900 (from 100 frames at 100 to 30 at 900 and over). */
int brv_spawn_interval(int prize) {
    int p = iclamp(prize, BRV_STEP, BRV_RATE_CAP);
    return 100 - (p - BRV_STEP) * 70 / (BRV_RATE_CAP - BRV_STEP);
}
