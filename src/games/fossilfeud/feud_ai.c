/* FOSSIL FEUD - the planner. The Slate Flock thinks with it, and so does
 * the demo player in the tests (which then presses the buttons). It picks
 * an Ancestor by trying each one on offer on a copy of the board, with its
 * three actions done greedily and battles guessed, and scoring what is
 * left; then, one step at a time, what to enlist, build, move, trade or
 * call down.
 *
 * skill 0 (FIRST MOULT): often prays at random, leaves its eyries bare,
 * attacks when it shouldn't, buys the cheapest birds, uses no omen but
 * the tithe. skill 1 (SECOND MOULT): better. skill 2 (FULL PLUME): weighs
 * everything, still leaves a little too few at home. skill 3: the demo
 * player. */
#include "feud.h"

/* how much a bird is worth against a mix of foes, x16 (worth x16 alone) */
static const int EFF[FU_KINDS][FU_KINDS] = {
    {10, 5, 12, 13, 0},
    {12, 10, 16, 7, 0},
    {10, 3, 10, 12, 13},
    {10, 15, 10, 8, 10},
    {15, 10, 10, 5, 5},
};

int feud_unit_power(int kind, const int enemy[FU_KINDS]) {
    int n = 0, e = 0;
    for (int j = 0; j < FU_KINDS; j++) { n += enemy[j]; e += EFF[kind][j] * enemy[j]; }
    if (n == 0) return FEUD_UNIT[kind].worth * 16;
    return FEUD_UNIT[kind].worth * 16 * e / (10 * n);
}

static const int TERR_DEF[FT_KINDS] = {90, 95, 90, 100, 105, 150, 170};

static int counts_power(const int c[FU_KINDS], const int enemy[FU_KINDS]) {
    int p = 0;
    for (int k = 0; k < FU_KINDS; k++) p += c[k] * feud_unit_power(k, enemy);
    return p;
}

int feud_tile_power(const FeudMatch *m, int t, const int enemy[FU_KINDS], bool defending) {
    int c[FU_KINDS];
    for (int k = 0; k < FU_KINDS; k++) c[k] = m->t[t].u[k];
    int p = counts_power(c, enemy);
    if (defending) {
        p = p * TERR_DEF[m->t[t].terr] / 100;
        if (m->t[t].frozen) p /= 8;
    }
    return p;
}

static void tile_counts(const FeudMatch *m, int t, int c[FU_KINDS]) {
    for (int k = 0; k < FU_KINDS; k++) c[k] = m->t[t].u[k];
}

static void side_counts(const FeudMatch *m, int side, int c[FU_KINDS]) {
    memset(c, 0, sizeof(int) * FU_KINDS);
    for (int t = 0; t < FD_TILES; t++)
        if (m->t[t].owner == side)
            for (int k = 0; k < FU_KINDS; k++) c[k] += m->t[t].u[k];
}

/* isqrt of a fraction, x1000 */
static int frac_sqrt(int num, int den) {
    if (den <= 0) return 0;
    if (num >= den) return 0;
    float f = 1.0f - (float)num * (float)num / ((float)den * (float)den);
    return (int)(sqrtf(f) * 1000.0f);
}

int feud_estimate(const FeudMatch *m, int atk, int dst, const uint8_t counts[FU_KINDS], bool surprise, int surv_a[FU_KINDS],
                  int surv_d[FU_KINDS]) {
    int ca[FU_KINDS], cd[FU_KINDS];
    for (int k = 0; k < FU_KINDS; k++) ca[k] = counts[k];
    tile_counts(m, dst, cd);
    int A = counts_power(ca, cd);
    int D = feud_tile_power(m, dst, ca, true);
    if (surprise) D = D * 85 / 100;
    if (A > D) {
        int f = frac_sqrt(D, A);
        for (int k = 0; k < FU_KINDS; k++) { surv_a[k] = (ca[k] * f + 500) / 1000; surv_d[k] = 0; }
        int any = 0;
        for (int k = 0; k < FU_KINDS; k++) any += surv_a[k];
        if (!any)
            for (int k = 0; k < FU_KINDS; k++)
                if (ca[k]) { surv_a[k] = 1; break; }
        return 1;
    }
    /* beaten: the attackers break off and run, the defenders bloodied */
    for (int k = 0; k < FU_KINDS; k++) {
        surv_a[k] = ca[k] * 55 / 100;
        surv_d[k] = cd[k] - cd[k] * (D > 0 ? 30 * A / D : 0) / 100;
    }
    return 0;
}

/* ---- the score of a board --------------------------------------------------- */

static int enemy_near_power(const FeudMatch *m, int side, int t, const int mine[FU_KINDS]) {
    int p = 0;
    for (int e = 0; e < FD_TILES; e++)
        if (m->t[e].owner == 1 - side && feud_adjacent(e, t, true) && !m->t[e].frozen) p += feud_tile_power(m, e, mine, false);
    return p;
}

static int threat_of(const FeudMatch *m, int side, int t) {
    int c[FU_KINDS];
    tile_counts(m, t, c);
    int any = 0;
    for (int k = 0; k < FU_KINDS; k++) any += c[k];
    if (!any) c[FU_STOMPER] = 1;
    return enemy_near_power(m, side, t, c);
}

static int side_value(const FeudMatch *m, int side) {
    static const int BV[FB_KINDS] = {16, 10, 12, 12};
    int v = 0;
    int their[FU_KINDS];
    side_counts(m, 1 - side, their);
    for (int t = 0; t < FD_TILES; t++) {
        const FeudTile *x = &m->t[t];
        if (x->owner != side) continue;
        v += 18 + (x->flag ? 120 : 0) + (x->eyrie ? 10 : 0);
        for (int k = 0; k < FB_KINDS; k++) v += BV[k] * x->b[k];
        int mine = 0;
        for (int k = 0; k < FU_KINDS; k++) mine += x->u[k] * FEUD_UNIT[k].worth;
        v += mine * 6;
        if (x->flag) {
            int th = threat_of(m, side, t);
            int def = feud_tile_power(m, t, their, true);
            if (th > def) v -= 70;
        }
        /* birds near the enemy's totems are worth a little more */
        int nd = 9;
        for (int e = 0; e < FD_TILES; e++)
            if (m->t[e].owner != side && m->t[e].flag) nd = imin(nd, feud_dist(t, e));
        v += mine * imax(0, 3 - nd) / 2;
    }
    const FeudSide *s = &m->s[side];
    v += s->res[FR_GRAIN] + s->res[FR_TOILERS] * 4 + s->res[FR_RELICS] * 4 + s->blessings * 12;
    if (s->hold || feud_flags_held(m, side) >= FD_WIN_FLAGS) v += 80;
    if (m->winner == side) v += 100000;
    return v;
}

int feud_eval(const FeudMatch *m, int side) { return side_value(m, side) - side_value(m, 1 - side); }

/* ---- choosing ------------------------------------------------------------------ */

static const int GARRISON_BASE[4] = {20, 50, 80, 100};
static const int DARE_BASE[4] = {90, 115, 125, 120}; /* attack when this % stronger */
static int g_turn;
/* the demo player grows bolder as a match drags on, to break a standoff */
static int garrison(int skill) { return skill == 3 ? imax(45, 100 - imax(0, g_turn - 60) / 2) : GARRISON_BASE[skill]; }
static int dare(int skill) { return skill == 3 ? imax(80, 120 - imax(0, g_turn - 50) / 2) : DARE_BASE[skill]; }

static int nearest_enemy(const FeudMatch *m, int side, int t) {
    int d = 9;
    for (int e = 0; e < FD_TILES; e++)
        if (m->t[e].owner == 1 - side) d = imin(d, feud_dist(t, e));
    return d;
}

static int nearest_open(const FeudMatch *m, int side, int t) {
    int d = 9;
    for (int e = 0; e < FD_TILES; e++)
        if (m->t[e].owner != side) d = imin(d, feud_dist(t, e) * 2 - (m->t[e].flag ? 1 : 0));
    return d;
}

static int deficit(const FeudMatch *m, int side, int t, int skill) {
    int their[FU_KINDS];
    side_counts(m, 1 - side, their);
    int need = threat_of(m, side, t) * garrison(skill) / 100;
    if (!m->t[t].flag && !m->t[t].eyrie) need = need * 3 / 10;
    return need - feud_tile_power(m, t, their, true);
}

static int res_cost(const FeudOffer *o) { return o->cost * (o->res == FR_GRAIN ? 1 : 5); }

static bool pick_enlist(const FeudMatch *m, int side, int act, int skill, FeudStep *st) {
    const FeudOffer *o;
    int n = feud_offers_of(m, side, act, &o);
    int their[FU_KINDS];
    side_counts(m, 1 - side, their);
    int free_slots = 0;
    for (int t = 0; t < FD_TILES; t++)
        if (feud_can_build_on(m, side, t)) free_slots++;
    int best = -1, bv = 0;
    for (int i = 0; i < n; i++) {
        if (!feud_can_afford(m, side, &o[i])) continue;
        if (o[i].res == FR_TOILERS && m->s[side].res[FR_TOILERS] < 3 && free_slots > 0 && skill > 0) continue;
        if (o[i].res == FR_RELICS && m->s[side].res[FR_RELICS] < 3 && skill > 1) continue;
        int v = feud_unit_power(o[i].what, their) * 100 / imax(1, res_cost(&o[i]));
        if (skill == 0) v = 1000 - res_cost(&o[i]);
        if (v > bv) { bv = v; best = i; }
    }
    if (best < 0) return false;
    int bt = -1, btv = -1000000000;
    for (int t = 0; t < FD_TILES; t++) {
        if (!feud_is_recruit_point(m, side, t) || m->t[t].u[o[best].what] >= FD_STACK_MAX) continue;
        int v = deficit(m, side, t, imax(1, skill)) * 4 - nearest_open(m, side, t) * 30;
        if (v > btv) { btv = v; bt = t; }
    }
    if (bt < 0) return false;
    memset(st, 0, sizeof *st);
    st->type = ST_ENLIST;
    st->tile = (int8_t)bt;
    st->idx = (uint8_t)best;
    return true;
}

static bool pick_build(const FeudMatch *m, int side, int act, int skill, Rng *r, FeudStep *st) {
    const FeudOffer *o;
    int n = feud_offers_of(m, side, act, &o);
    int best = -1, bv = 0;
    for (int i = 0; i < n; i++) {
        if (!feud_can_afford(m, side, &o[i])) continue;
        int have = feud_bld_count(m, side, o[i].what), v = 0;
        switch (o[i].what) {
        case FB_GRANARY: v = 34 - 4 * have; break;
        case FB_BATH: v = 24 - 6 * have; break;
        case FB_SHRINE: v = 22 - 6 * have; break;
        case FB_ROOST: v = 20 - 6 * have; break;
        }
        if (skill == 0) v = rng_range(r, 1, 30);
        if (v > bv) { bv = v; best = i; }
    }
    if (best < 0) return false;
    bool roost = o[best].what == FB_ROOST;
    int bt = -1, btv = -1000000000;
    for (int t = 0; t < FD_TILES; t++) {
        if (!feud_can_build_on(m, side, t)) continue;
        if (roost && feud_is_recruit_point(m, side, t)) continue;
        int ne = nearest_enemy(m, side, t);
        int v = roost ? -ne * 10 : ne * 10 - threat_of(m, side, t) / 8;
        if (m->t[t].eyrie && !roost) v += 6;
        if (v > btv) { btv = v; bt = t; }
    }
    if (bt < 0) return false;
    memset(st, 0, sizeof *st);
    st->type = ST_BUILD;
    st->tile = (int8_t)bt;
    st->idx = (uint8_t)best;
    return true;
}

/* which birds stay to guard a tile, and which may go */
static int spare_of(const FeudMatch *m, int side, int t, int skill, uint8_t spare[FU_KINDS]) {
    static const int KEEP_ORDER[FU_KINDS] = {FU_SLINGER, FU_MYSTIC, FU_LANCER, FU_STOMPER, FU_SWIFT};
    int their[FU_KINDS];
    side_counts(m, 1 - side, their);
    int need = threat_of(m, side, t) * garrison(skill) / 100;
    if (!m->t[t].flag && !m->t[t].eyrie) need = need * 3 / 10;
    need = need * 100 / TERR_DEF[m->t[t].terr];
    int total = 0;
    for (int k = 0; k < FU_KINDS; k++) spare[k] = (uint8_t)feud_movable(m, t, k);
    /* the birds that can't move (they moved already) count as guards */
    int kept = 0;
    for (int k = 0; k < FU_KINDS; k++) kept += (m->t[t].u[k] - spare[k]) * feud_unit_power(k, their);
    if (m->t[t].flag && skill >= 2 && need > 0) need = imax(need, feud_unit_power(FU_STOMPER, their));
    for (int i = 0; i < FU_KINDS && kept < need; i++) {
        int k = KEEP_ORDER[i];
        while (spare[k] > 0 && kept < need) {
            spare[k]--;
            kept += feud_unit_power(k, their);
        }
    }
    for (int k = 0; k < FU_KINDS; k++) total += spare[k];
    return total;
}

static int drop_value(const FeudTile *x) {
    static const int V[FR_KINDS] = {12, 14, 12};
    int v = 0;
    for (int i = 0; i < x->ndrop; i++) v += V[x->drop[i]];
    return v;
}

static int bld_worth(const FeudTile *x) { return feud_tile_blds(x) * 20; }

/* one bird to claim open ground: the cheapest walker, else anyone */
static void one_bird(const uint8_t spare[FU_KINDS], uint8_t out[FU_KINDS]) {
    static const int PREF[FU_KINDS] = {FU_STOMPER, FU_SLINGER, FU_LANCER, FU_MYSTIC, FU_SWIFT};
    memset(out, 0, FU_KINDS);
    for (int i = 0; i < FU_KINDS; i++)
        if (spare[PREF[i]]) { out[PREF[i]] = 1; return; }
}

static int count_sum(const uint8_t c[FU_KINDS]) {
    int n = 0;
    for (int k = 0; k < FU_KINDS; k++) n += c[k];
    return n;
}

/* the best move from src to one of mask's tiles; returns its value */
static int best_move_from(const FeudMatch *m, int side, int src, uint32_t mask, int mflags, int skill, Rng *r, FeudStep *out) {
    uint8_t spare[FU_KINDS];
    if (spare_of(m, side, src, skill, spare) == 0) return 0;
    int best = 0;
    int their_all[FU_KINDS];
    side_counts(m, 1 - side, their_all);
    for (int dst = 0; dst < FD_TILES; dst++) {
        if (!(mask & (1u << dst))) continue;
        const FeudTile *d = &m->t[dst];
        uint8_t send[FU_KINDS];
        int v = 0;
        if (d->owner == side) {
            /* shore up a threatened totem, or close in on the enemy */
            int def = deficit(m, side, dst, imax(1, skill));
            int sp = 0;
            for (int k = 0; k < FU_KINDS; k++) sp += spare[k] * feud_unit_power(k, their_all);
            if (d->flag && def > 0) v = 10 + imin(def, sp) / 16;
            int gain = nearest_open(m, side, src) - nearest_open(m, side, dst);
            if (gain > 0) v = imax(v, gain * imin(40, sp / 24));
            if (v <= 4) continue;
            memcpy(send, spare, sizeof send);
        } else if (d->owner < 0 || feud_units_on(m, dst) == 0) {
            v = (d->owner < 0 ? 30 + drop_value(d) : 50 + bld_worth(d)) + (d->flag ? (d->owner < 0 ? 90 : 130) : 0);
            if (d->eyrie) v += 20;
            one_bird(spare, send);
            /* where the enemy is next door, send enough to hold it */
            int th = threat_of(m, side, dst);
            if (th > 0 && skill >= 1) {
                int sent = 0;
                memset(send, 0, sizeof send);
                for (int k = 0; k < FU_KINDS && sent * TERR_DEF[d->terr] / 100 < th; k++)
                    while (send[k] < spare[k] && sent * TERR_DEF[d->terr] / 100 < th) { send[k]++; sent += feud_unit_power(k, their_all); }
                if (!count_sum(send)) one_bird(spare, send);
                if (sent * TERR_DEF[d->terr] / 100 < th / 2) v -= 25;
            }
        } else {
            /* a real fight: only if it looks won */
            int sa[FU_KINDS], sd[FU_KINDS];
            memcpy(send, spare, sizeof send);
            int ca[FU_KINDS], cd[FU_KINDS];
            for (int k = 0; k < FU_KINDS; k++) { ca[k] = send[k]; cd[k] = d->u[k]; }
            int A = counts_power(ca, cd), D = feud_tile_power(m, dst, ca, true);
            if (mflags & MF_SURPRISE) D = D * 85 / 100;
            if (A * 100 < D * dare(skill)) continue;
            feud_estimate(m, side, dst, send, (mflags & MF_SURPRISE) != 0, sa, sd);
            int lost = 0, killed = 0;
            for (int k = 0; k < FU_KINDS; k++) { lost += (send[k] - sa[k]) * FEUD_UNIT[k].worth; killed += d->u[k] * FEUD_UNIT[k].worth; }
            v = 70 + (d->flag ? 150 : 0) + bld_worth(d) + killed * 6 - lost * 5;
            if (d->eyrie) v += 20;
        }
        if (skill == 0) v += rng_range(r, -20, 20);
        if (v > best && count_sum(send) > 0) {
            best = v;
            memset(out, 0, sizeof *out);
            out->type = ST_MOVE;
            out->tile = (int8_t)src;
            out->tile2 = (int8_t)dst;
            memcpy(out->counts, send, FU_KINDS);
        }
    }
    return best;
}

static bool pick_move(const FeudMatch *m, int side, int act, int skill, Rng *r, FeudStep *st) {
    int mflags = feud_lv(m, side, act)->mflags;
    int best = 0;
    FeudStep tmp;
    for (int src = 0; src < FD_TILES; src++) {
        if (!feud_can_move_from(m, side, src)) continue;
        int v = best_move_from(m, side, src, feud_march_dests(m, side, src, mflags), mflags, skill, r, &tmp);
        if (v > best) { best = v; *st = tmp; }
    }
    return best > 0;
}

static bool pick_trade(const FeudMatch *m, int side, int act, FeudStep *st) {
    const FeudOffer *o;
    int n = feud_offers_of(m, side, act, &o);
    const FeudSide *s = &m->s[side];
    for (int i = 0; i < n; i++) {
        if (s->res[o[i].what] < o[i].cost) continue;
        bool want = false;
        if (o[i].what == FR_RELICS && o[i].res == FR_GRAIN) want = s->res[FR_RELICS] > 4 || (s->res[FR_GRAIN] < 8 && s->res[FR_RELICS] > 1);
        if (o[i].what == FR_GRAIN && o[i].res == FR_TOILERS) want = s->res[FR_TOILERS] < 3 && s->res[FR_GRAIN] >= o[i].cost + 8;
        if (want && s->res[o[i].res] < (o[i].res == FR_GRAIN ? FD_RES_CAP_GRAIN - 9 : FD_RES_CAP_OTHER)) {
            memset(st, 0, sizeof *st);
            st->type = ST_TRADE;
            st->idx = (uint8_t)i;
            return true;
        }
    }
    return false;
}

static int tithe_target(const FeudMatch *m, int side) {
    int best = -1, bv = -1;
    for (int a = 0; a < FD_ANC; a++) {
        if (!feud_can_bless(m, side, a)) continue;
        if (m->s[side].favour[a] % 2 == 0) continue; /* one short of a blessing */
        int v = m->s[side].favour[a] * 4 + m->s[side].picks[a];
        if (v > bv) { bv = v; best = a; }
    }
    return best;
}

static bool pick_omen(const FeudMatch *m, int side, int act, int skill, Rng *r, FeudStep *st) {
    const FeudOffer *o;
    int n = feud_offers_of(m, side, act, &o);
    int best = 0;
    FeudStep pick;
    memset(&pick, 0, sizeof pick);
    int mine[FU_KINDS];
    side_counts(m, side, mine);
    for (int i = 0; i < n; i++) {
        if (m->s[side].res[FR_RELICS] < o[i].cost) continue;
        int om = o[i].what;
        if (skill == 0 && om != OM_TITHE) continue;
        switch (om) {
        case OM_TITHE: {
            int a = tithe_target(m, side);
            if (a >= 0 && 40 > best) { best = 40; pick.type = ST_OMEN; pick.idx = (uint8_t)i; pick.anc = (uint8_t)a; }
            break;
        }
        case OM_BOLT:
            for (int t = 0; t < FD_TILES; t++) {
                if (m->t[t].owner != 1 - side || !feud_units_on(m, t)) continue;
                int w = 0;
                for (int k = 0; k < FU_KINDS; k++) w += imin(2, m->t[t].u[k]) * FEUD_UNIT[k].worth;
                int v = w * 2 + (m->t[t].flag ? 20 : 0) - o[i].cost * 4;
                if (w >= 18 && v > best) { best = v; memset(&pick, 0, sizeof pick); pick.type = ST_OMEN; pick.idx = (uint8_t)i; pick.tile = (int8_t)t; }
            }
            break;
        case OM_HAIL:
            for (int t = 0; t < FD_TILES; t++) {
                if (m->t[t].owner != 1 - side || !feud_units_on(m, t) || m->t[t].frozen) continue;
                int p = feud_tile_power(m, t, mine, false), worst = 0;
                for (int u = 0; u < FD_TILES; u++)
                    if (m->t[u].owner == side && m->t[u].flag && feud_adjacent(t, u, true)) {
                        int their[FU_KINDS];
                        tile_counts(m, t, their);
                        worst = imax(worst, p - feud_tile_power(m, u, their, true));
                    }
                int v = worst > 0 ? 30 + worst / 32 : 0;
                if (skill >= 2) {
                    /* or freeze them so a march can take the tile while they stand */
                    int their[FU_KINDS];
                    tile_counts(m, t, their);
                    int D = feud_tile_power(m, t, mine, true) / 8;
                    for (int u = 0; u < FD_TILES; u++) {
                        if (m->t[u].owner != side || !feud_adjacent(t, u, false)) continue;
                        uint8_t sp[FU_KINDS];
                        spare_of(m, side, u, skill, sp);
                        int ca[FU_KINDS];
                        for (int k = 0; k < FU_KINDS; k++) ca[k] = sp[k];
                        int P = counts_power(ca, their);
                        if (P > 0 && P * 100 >= D * dare(skill) && P * 100 < feud_tile_power(m, t, ca, true) * dare(skill))
                            v = imax(v, 55 + (m->t[t].flag ? 80 : 0) + bld_worth(&m->t[t]) / 2);
                    }
                }
                if (v > best) { best = v; memset(&pick, 0, sizeof pick); pick.type = ST_OMEN; pick.idx = (uint8_t)i; pick.tile = (int8_t)t; }
            }
            break;
        case OM_UPDRAFT:
            for (int src = 0; src < FD_TILES; src++) {
                if (!feud_can_move_from(m, side, src)) continue;
                uint32_t mask = feud_flight_dests(m, side, src), keep = 0;
                for (int t = 0; t < FD_TILES; t++)
                    if ((mask >> t & 1) && m->t[t].owner != side && m->t[t].flag) keep |= 1u << t;
                FeudStep mv;
                int v = best_move_from(m, side, src, keep, 0, skill, r, &mv) - o[i].cost * 6;
                if (v > best && v > 60) {
                    best = v;
                    pick = mv;
                    pick.type = ST_OMEN;
                    pick.idx = (uint8_t)i;
                }
            }
            break;
        case OM_HATCH:
            for (int t = 0; t < FD_TILES; t++)
                if (feud_is_recruit_point(m, side, t) && 18 > best) {
                    best = 18;
                    memset(&pick, 0, sizeof pick);
                    pick.type = ST_OMEN;
                    pick.idx = (uint8_t)i;
                    pick.tile = (int8_t)t;
                }
            break;
        default: break; /* the planner never shifts peaks or looks with far sight */
        }
    }
    if (best <= 0) return false;
    *st = pick;
    return true;
}

bool feud_ai_next(const FeudMatch *m, int side, int act, int skill, FeudStep *st) {
    g_turn = m->turn;
    static Rng r;
    static bool seeded;
    if (!seeded) { rng_seed(&r, 12); seeded = true; }
    switch (FEUD_ACT[act].kind) {
    case AK_ENLIST: return pick_enlist(m, side, act, skill, st);
    case AK_BUILD: return pick_build(m, side, act, skill, &r, st);
    case AK_MARCH: return pick_move(m, side, act, skill, &r, st);
    case AK_BARTER: return pick_trade(m, side, act, st);
    case AK_OMEN: return pick_omen(m, side, act, skill, &r, st);
    default: return false;
    }
}

/* a step on a copy of the board, with any battle guessed */
static void est_move(FeudMatch *m, int side, int src, int dst, const uint8_t counts[FU_KINDS], bool surprise) {
    if (feud_move(m, side, src, dst, counts) != 1) return;
    int sa[FU_KINDS], sd[FU_KINDS];
    FeudTile *d = &m->t[dst];
    uint8_t c[FU_KINDS];
    memcpy(c, counts, FU_KINDS);
    if (feud_estimate(m, side, dst, c, surprise, sa, sd)) {
        d->owner = (int8_t)side;
        for (int k = 0; k < FU_KINDS; k++) { d->u[k] = (uint8_t)sa[k]; d->moved[k] = d->u[k]; }
        d->frozen = 0;
    } else {
        for (int k = 0; k < FU_KINDS; k++) {
            d->u[k] = (uint8_t)imax(0, sd[k]);
            m->t[src].u[k] = (uint8_t)imin(FD_STACK_MAX, m->t[src].u[k] + sa[k]);
            m->t[src].moved[k] = (uint8_t)imin(m->t[src].u[k], m->t[src].moved[k] + sa[k]);
        }
    }
    feud_check_elimination(m);
}

void feud_ai_apply_est(FeudMatch *m, int side, int act, const FeudStep *st) {
    switch (st->type) {
    case ST_ENLIST: feud_enlist(m, side, act, st->tile, st->idx); break;
    case ST_BUILD: feud_build(m, side, act, st->tile, st->idx); break;
    case ST_TRADE: feud_trade(m, side, act, st->idx); break;
    case ST_MOVE: est_move(m, side, st->tile, st->tile2, st->counts, (feud_lv(m, side, act)->mflags & MF_SURPRISE) != 0); break;
    case ST_OMEN: {
        const FeudOffer *o;
        feud_offers_of(m, side, act, &o);
        int om = o[st->idx].what;
        m->s[side].res[FR_RELICS] -= o[st->idx].cost;
        switch (om) {
        case OM_TITHE:
            if (feud_add_favour(m, side, st->anc)) feud_bless(m, side, feud_ai_pick_bless(m, side, st->anc));
            break;
        case OM_BOLT: feud_bolt(m, st->tile); break;
        case OM_HAIL: feud_hail(m, st->tile); break;
        case OM_HATCH: feud_hatch(m, side, st->tile); break;
        case OM_UPDRAFT: est_move(m, side, st->tile, st->tile2, st->counts, false); break;
        default: break;
        }
        break;
    }
    default: break;
    }
}

int feud_ai_pick_bless(const FeudMatch *m, int side, int anc) {
    static const int PRI[7] = {6, 6, 5, 4, 3, 1, 3}; /* gather enlist callup build march barter omen */
    int best = -1, bv = -1;
    for (int i = 0; i < 3; i++) {
        int act = m->deck[anc][i];
        if (m->star[side][act] >= 2) continue;
        int v = PRI[FEUD_ACT[act].kind] * 4 - m->star[side][act];
        if (v > bv) { bv = v; best = act; }
    }
    return best;
}

/* run an Ancestor's whole turn on a copy and score the board after */
static int try_anc(const FeudMatch *m, int side, int anc, int skill) {
    static FeudMatch c;
    c = *m;
    c.side = side;
    if (feud_pray(&c, anc)) {
        int b = feud_ai_pick_bless(&c, side, anc);
        if (b >= 0) feud_bless(&c, side, b);
    }
    for (int i = 0; i < 3; i++) {
        int act = c.deck[anc][i];
        int kind = FEUD_ACT[act].kind;
        if (kind == AK_GATHER) { feud_gather(&c, side, act, NULL); continue; }
        if (kind == AK_CALLUP) { feud_callup(&c, side, act, NULL); continue; }
        if (kind == AK_MARCH) feud_reset_moved(&c);
        FeudStep st;
        int most = kind == AK_MARCH ? 1 : 24; /* a march is one move */
        for (int n = 0; n < most && feud_ai_next(&c, side, act, skill, &st); n++) feud_ai_apply_est(&c, side, act, &st);
    }
    return feud_eval(&c, side);
}

int feud_ai_pick_anc(const FeudMatch *m, int side, int skill, Rng *r) {
    g_turn = m->turn;
    if (m->noffer == 0) return 0;
    if (skill == 0 && rng_chance(r, 60)) return m->offer[rng_range(r, 0, m->noffer - 1)];
    static const int NOISE[4] = {0, 60, 12, 0};
    int best = m->offer[0], bv = -1000000000;
    for (int i = 0; i < m->noffer; i++) {
        int v = try_anc(m, side, m->offer[i], skill);
        if (NOISE[skill]) v += rng_range(r, -NOISE[skill], NOISE[skill]);
        if (v > bv) { bv = v; best = m->offer[i]; }
    }
    return best;
}
