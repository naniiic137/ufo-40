/* SOUNDINGS - the party's rules: levels and HP, what a diver holds, buying,
 * refilling at the raft, experience and relics, and the game's own dice. */
#include "sdg.h"

SdgState sdg;

static const char *const NAMES[3] = {"MOSS", "REED", "SKIP"};
const char *sdg_diver_name(int d) { return d >= 0 && d < 3 ? NAMES[d] : "?"; }

int sdg_rand(int lo, int hi) { return rng_range(&sdg.rng, lo, hi); }
int sdg_chance(int pct) { return rng_range(&sdg.rng, 0, 99) < pct; }

int sdg_isqrt(int v) {
    if (v <= 0) return 0;
    int r = 0, b = 1 << 30;
    while (b > v) b >>= 2;
    while (b) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
        b >>= 2;
    }
    return r;
}

bool sdg_holds(const SdgProg *p, int d, int it) { return p->equip[d][0] == it || p->equip[d][1] == it; }

bool sdg_party_holds(const SdgProg *p, int it) {
    for (int d = 0; d < 3; d++)
        if (sdg_holds(p, d, it)) return true;
    return false;
}

int sdg_diver_level(const SdgProg *p, int d) {
    int lv = p->level;
    if (sdg_holds(p, d, IT_GODBLOOD)) lv++;
    if (sdg_holds(p, d, IT_EVIL)) lv--;
    return iclamp(lv, 1, SDG_MAX_LEVEL);
}

int sdg_diver_maxhp(const SdgProg *p, int d) { return SDG_HP_AT[sdg_diver_level(p, d)]; }

/* every shield in a diver's hands takes its cut off every hit: two shields
 * multiply (a bulwark leaves 400 thousandths, two leave 160) */
int sdg_shield_mul(const SdgProg *p, int d) {
    int m = 1000;
    for (int s = 0; s < 2; s++) {
        int it = p->equip[d][s];
        if (it && SDG_ITEM[it].kind == K_SHIELD) m = m * (100 - SDG_ITEM[it].def) / 100;
    }
    return m;
}

int sdg_cap_gold(int32_t g) { return (int)iclamp((int)g, 0, SDG_GOLD_MAX); }

int sdg_alive_count(const SdgProg *p) {
    int n = 0;
    for (int d = 0; d < 3; d++) n += p->hp[d] > 0;
    return n;
}

int sdg_stored(const SdgProg *p, int it) {
    int n = p->owned[it];
    for (int d = 0; d < 3; d++)
        for (int s = 0; s < 2; s++) n -= p->equip[d][s] == it;
    return n;
}

int sdg_relic_total(const SdgProg *p, int r) { return p->relic[r] + p->carry[r]; }

void sdg_gain_relic(SdgProg *p, int r) {
    if (r < 0 || r >= RL_COUNT) return;
    if (sdg_relic_total(p, r) < SDG_RELIC_MAX) p->carry[r]++;
}

bool sdg_can_buy(const SdgProg *p, int it) {
    const SdgItem *I = &SDG_ITEM[it];
    if (I->shop < 0 || I->price <= 0) return false;
    if (p->owned[it] >= I->max) return false;
    if (p->gold < I->price) return false;
    for (int k = 0; k < 2; k++)
        if (I->rel[k] >= 0 && p->relic[I->rel[k]] < I->reln[k]) return false;
    return true;
}

bool sdg_buy(SdgProg *p, int it) {
    if (!sdg_can_buy(p, it)) return false;
    const SdgItem *I = &SDG_ITEM[it];
    p->gold -= I->price;
    for (int k = 0; k < 2; k++)
        if (I->rel[k] >= 0) p->relic[I->rel[k]] = (uint8_t)(p->relic[I->rel[k]] - I->reln[k]);
    p->owned[it]++;
    return true;
}

void sdg_refill(SdgProg *p) {
    for (int d = 0; d < 3; d++) {
        for (int s = 0; s < 2; s++) p->uses[d][s] = p->equip[d][s] ? SDG_ITEM[p->equip[d][s]].uses : 0;
        p->hp[d] = (int16_t)sdg_diver_maxhp(p, d);
    }
}

void sdg_add_xp(SdgProg *p, int xp) {
    p->xp += xp;
    while (p->level < SDG_MAX_LEVEL && p->xp >= SDG_XP_AT[p->level + 1]) p->level++;
    if (p->level >= SDG_MAX_LEVEL && p->xp > SDG_XP_AT[SDG_MAX_LEVEL]) p->xp = SDG_XP_AT[SDG_MAX_LEVEL];
    /* a level gained on a dive raises the most HP, not the HP */
    for (int d = 0; d < 3; d++)
        if (p->hp[d] > sdg_diver_maxhp(p, d)) p->hp[d] = (int16_t)sdg_diver_maxhp(p, d);
}
