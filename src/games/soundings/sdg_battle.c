/* SOUNDINGS - the fights. One round: Moss, Reed and Skip are given their
 * orders in turn (left hand, right hand, wait, or run), then the round plays:
 * the divers act left to right, then the creatures left to right.
 *
 * - Every use of an item costs one use, whatever comes of it: a miss, a
 *   guard nobody needed, a tonic on someone already well.
 * - Running is a flat 25 %. A failed run throws away the whole round's
 *   orders, the ones given before it too; the creatures still act.
 * - Shields take their share off every hit while held; defending with one
 *   doubles that for the holder, who also takes the hits meant for the one
 *   he defends. Two bulwarks and a defend take a hit to nothing.
 * - Creatures telegraph (a look, then a heavy blow), cover each other, heal,
 *   bring each other back, hide, prickle and drink what they bite. */
#include "sdg.h"
#include <stdarg.h>

#define B (sdg.bat)
#define P (sdg.prog)

/* ---- the arithmetic ------------------------------------------------------ */

int sdg_attack_damage(int power, int level, int el, int weak, int roll_pct) {
    int d = power * sdg_level_mul(level) / 100;
    if (el != EL_NONE && el == weak) d = d * SDG_WEAK_NUM / SDG_WEAK_DEN;
    return d * roll_pct / 100;
}

int sdg_reduce(int dmg, int pct) {
    pct = iclamp(pct, 0, 100);
    return dmg * (100 - pct) / 100;
}

/* ---- the groups a creature on the map stands for --------------------------- */

typedef struct Grp { uint8_t n, k[4]; } Grp;
#define G1(a) {1, {a, 0, 0, 0}}
#define G2(a, b) {2, {a, b, 0, 0}}
#define G3(a, b, c) {3, {a, b, c, 0}}
#define G4(a, b, c, d) {4, {a, b, c, d}}

static const Grp GR_FIZZLE[] = {G1(EN_FIZZLE), G2(EN_FIZZLE, EN_FIZZLE), G1(EN_PRICKLE)};
static const Grp GR_PRICKLE[] = {G1(EN_PRICKLE), G2(EN_PRICKLE, EN_FIZZLE)};
static const Grp GR_FROND_SHELF[] = {G1(EN_FROND)};
/* the hollow's frond is a slater pack, the gift's guard */
static const Grp GR_FROND_HOLLOW[] = {G3(EN_LOUSE, EN_LOUSE, EN_LOUSE), G4(EN_LOUSE, EN_NIPPER, EN_NIPPER, EN_LOUSE)};
static const Grp GR_FROND_SHRINE[] = {G3(EN_FROND, EN_WHORL, EN_FROND)};
static const Grp GR_FROND_LANTERN[] = {G4(EN_FROND, EN_JELLY, EN_GROPER, EN_FROND)};
static const Grp GR_SCHOOL[] = {G3(EN_NIPPER, EN_NIPPER, EN_NIPPER), G3(EN_NIPPER, EN_PRICKLE, EN_NIPPER)};
static const Grp GR_GLOB[] = {G2(EN_GLOB, EN_GLOB), G3(EN_GLOB, EN_GLOB, EN_GLOB), G2(EN_GLOB, EN_CLAMPER)};
static const Grp GR_SMOG[] = {G3(EN_SMOG, EN_SMOG, EN_SMOG), G2(EN_SMOG, EN_CLAMPER)};
static const Grp GR_CLAMPER[] = {G2(EN_CLAMPER, EN_GLOB), G3(EN_SMOG, EN_CLAMPER, EN_SMOG)};
static const Grp GR_TINFIN[] = {G3(EN_FIZZLE, EN_FROND, EN_FIZZLE), G3(EN_FIZZLE, EN_TINFIN, EN_FIZZLE), G2(EN_TINFIN, EN_TINFIN)};
static const Grp GR_WHORL[] = {G1(EN_WHORL), G3(EN_FROND, EN_WHORL, EN_FROND)};
static const Grp GR_JELLY[] = {G3(EN_JELLY, EN_JELLY, EN_JELLY), G4(EN_JELLY, EN_BURRNUT, EN_JELLY, EN_BURRNUT)};
static const Grp GR_GROPER[] = {G2(EN_GROPER, EN_GROPER), G4(EN_FROND, EN_JELLY, EN_GROPER, EN_FROND)};
static const Grp GR_GROPER_DEEP[] = {G3(EN_GROPER, EN_GRINFISH, EN_GROPER)};
static const Grp GR_BURRNUT[] = {G2(EN_BURRNUT, EN_BURRNUT), G4(EN_JELLY, EN_BURRNUT, EN_JELLY, EN_BURRNUT)};
static const Grp GR_STILTER[] = {G4(EN_STILTER, EN_STILTER, EN_STILTER, EN_STILTER), G3(EN_STILTER, EN_LOUSE, EN_STILTER)};
static const Grp GR_LOUSE[] = {G3(EN_LOUSE, EN_LOUSE, EN_LOUSE), G3(EN_STILTER, EN_LOUSE, EN_STILTER)};
static const Grp GR_HAUNT[] = {G2(EN_HAUNT, EN_HAUNT), G4(EN_HAUNT, EN_LOUSE, EN_HAUNT, EN_LOUSE)};
static const Grp GR_HAUNT_DEEP[] = {G2(EN_SQUID, EN_HAUNT)};
static const Grp GR_SQUID[] = {G1(EN_SQUID), G2(EN_SQUID, EN_HAUNT)};
static const Grp GR_GRINFISH[] = {G3(EN_GROPER, EN_GRINFISH, EN_GROPER)};
static const Grp GR_WORM[] = {G3(EN_WORM, EN_GRINFISH, EN_GRINFISH)};
static const Grp GR_WARDEN[] = {G3(EN_FROND, EN_WARDEN, EN_FROND)};
static const Grp GR_GLOAM[] = {G4(EN_ARM, EN_EYE, EN_EYE, EN_ARM)};

#define PICK(arr) do { g = arr; ng = ARRAY_LEN(arr); } while (0)

void sdg_group_for(int mobkind, int region, uint8_t *kinds, int *n) {
    const Grp *g = GR_FIZZLE;
    int ng = 1;
    switch (mobkind) {
    case MK_FIZZLE: PICK(GR_FIZZLE); break;
    case MK_PRICKLE: PICK(GR_PRICKLE); break;
    case MK_FROND:
        if (region == RG_HOLLOW) PICK(GR_FROND_HOLLOW);
        else if (region == RG_SHRINE) PICK(GR_FROND_SHRINE);
        else if (region == RG_LANTERN) PICK(GR_FROND_LANTERN);
        else PICK(GR_FROND_SHELF);
        break;
    case MK_NEST:
    case MK_SCHOOL: PICK(GR_SCHOOL); break;
    case MK_GLOB: PICK(GR_GLOB); break;
    case MK_SMOGVENT: PICK(GR_SMOG); break;
    case MK_CLAMPVENT: PICK(GR_CLAMPER); break;
    case MK_TINFIN: PICK(GR_TINFIN); break;
    case MK_WHORL: PICK(GR_WHORL); break;
    case MK_JELLY: PICK(GR_JELLY); break;
    case MK_GROPER:
        if (region == RG_DEEP) PICK(GR_GROPER_DEEP);
        else PICK(GR_GROPER);
        break;
    case MK_BURRNUT: PICK(GR_BURRNUT); break;
    case MK_STILTER: PICK(GR_STILTER); break;
    case MK_LOUSE: PICK(GR_LOUSE); break;
    case MK_HAUNT:
        if (region == RG_DEEP) PICK(GR_HAUNT_DEEP);
        else PICK(GR_HAUNT);
        break;
    case MK_SQUID: PICK(GR_SQUID); break;
    case MK_GRINFISH: PICK(GR_GRINFISH); break;
    case MK_WORM: PICK(GR_WORM); break;
    case MK_WARDEN: PICK(GR_WARDEN); break;
    case MK_GLOAM: PICK(GR_GLOAM); break;
    default: break;
    }
    const Grp *pick = &g[ng > 1 ? sdg_rand(0, ng - 1) : 0];
    *n = pick->n;
    for (int i = 0; i < pick->n; i++) kinds[i] = pick->k[i];
}

/* ---- messages -------------------------------------------------------------- */

static void say(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void say(const char *fmt, ...) {
    if (B.nmsg >= SDG_MSGS) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(B.msg[B.nmsg].text, sizeof B.msg[0].text, fmt, ap);
    va_end(ap);
    B.nmsg++;
}

static const char *fname(int i) { return SDG_FOE[B.foe[i].kind].name; }

static int foes_alive(void) {
    int n = 0;
    for (int i = 0; i < B.nfoe; i++) n += B.foe[i].alive;
    return n;
}

static int first_alive_foe(void) {
    for (int i = 0; i < B.nfoe; i++)
        if (B.foe[i].alive) return i;
    return -1;
}

static int random_alive_diver(void) {
    int pick[3], n = 0;
    for (int d = 0; d < 3; d++)
        if (P.hp[d] > 0) pick[n++] = d;
    return n ? pick[sdg_rand(0, n - 1)] : -1;
}

/* ---- starting a fight ---------------------------------------------------- */

static int next_orderable(int from) {
    for (int d = from; d < 3; d++)
        if (P.hp[d] > 0) return d;
    return -1;
}

static void start_round(void) {
    for (int d = 0; d < 3; d++) {
        B.ord[d].act = ACT_NONE;
        B.guarding[d] = -1;
        B.guard_kind[d] = 0;
    }
    B.cur = next_orderable(0);
    B.menu = 0;
    B.phase = BP_ORDER;
    B.ran = 0;
    B.round++;
}

void sdg_battle_start(int mob, const uint8_t *kinds, int n, int boss) {
    memset(&B, 0, sizeof B);
    B.mob = mob;
    B.boss = (uint8_t)boss;
    B.nfoe = iclamp(n, 1, SDG_FOES);
    B.drop = -1;
    B.region = (uint8_t)(mob >= 0 && mob < sdg.dive.nmob ? sdg.dive.mob[mob].region : RG_SHELF);
    for (int i = 0; i < B.nfoe; i++) {
        SdgFoe *f = &B.foe[i];
        const SdgFoeDef *D = &SDG_FOE[kinds[i]];
        f->kind = kinds[i];
        f->alive = 1;
        f->maxhp = D->hp;
        f->hp = (D->flags & FF_STARTHURT) ? D->hp * 3 / 4 : D->hp;
        f->weak = D->weak;
        if (D->flags & FF_SHIFT) {
            static const uint8_t W3[3] = {EL_REEF, EL_OOZE, EL_ZAP};
            f->weak = W3[sdg_rand(0, 2)];
        }
        f->mark = -1;
        f->guard = -1;
        /* haunts that swim with the deep's squids keep no bones */
        if (kinds[i] == EN_HAUNT && B.region == RG_DEEP) f->nodrop = 1;
    }
    if (boss == 1) say("THE ABBOT WAKES!");
    else if (boss == 2) say("THE GLOAMHEART OPENS ITS EYES!");
    else if (B.nfoe == 1) say("A %s COMES CLOSE!", fname(0));
    else say("%s AND %d MORE COME CLOSE!", fname(0), B.nfoe - 1);
    B.phase = BP_INTRO;
}

/* ---- what can be ordered ---------------------------------------------------- */

bool sdg_order_valid(int d, int slot) {
    int it = P.equip[d][slot];
    if (!sdg_battle_usable(it)) return false;
    if (P.uses[d][slot] == 0) return false;
    if (SDG_ITEM[it].kind == K_MIST && B.boss) return false;
    return true;
}

static bool target_ok(int mode, int t) {
    if (mode == MODE_ATTACK) return t >= 0 && t < B.nfoe && B.foe[t].alive;
    return t >= 0 && t < 3;
}

static int cycle_target(int mode, int t, int dir) {
    int n = mode == MODE_ATTACK ? B.nfoe : 3;
    for (int k = 0; k < n; k++) {
        t = (t + dir + n) % n;
        if (mode == MODE_ATTACK ? B.foe[t].alive : 1) return t;
    }
    return t;
}

static void open_target(void) {
    int it = P.equip[B.cur][B.menu];
    int k = SDG_ITEM[it].kind;
    if (k == K_POTION || k == K_EGG) {
        B.tmode = MODE_HEAL;
        /* the most hurt diver (for an egg, the first one down) */
        int best = B.cur, bv = 1 << 30;
        for (int d = 0; d < 3; d++) {
            int v = k == K_EGG ? (P.hp[d] > 0) * 100000 + d : (P.hp[d] > 0 ? P.hp[d] * 1000 / sdg_diver_maxhp(&P, d) : 1 << 29);
            if (v < bv) { bv = v; best = d; }
        }
        B.tsel = best;
    } else {
        B.tmode = MODE_ATTACK;
        B.tsel = first_alive_foe();
    }
    B.phase = BP_TARGET;
}

static void begin_resolve(void) {
    B.phase = BP_RESOLVE;
    B.actor = 0;
    B.sub = 0;
    for (int d = 0; d < 3; d++) B.guarding[d] = -1;
}

static void next_diver(void) {
    int n = next_orderable(B.cur + 1);
    if (n < 0) { begin_resolve(); return; }
    B.cur = n;
    B.menu = 0;
    B.phase = BP_ORDER;
}

static void prev_diver(void) {
    for (int d = B.cur - 1; d >= 0; d--)
        if (P.hp[d] > 0) {
            B.cur = d;
            B.ord[d].act = ACT_NONE;
            B.menu = 0;
            B.phase = BP_ORDER;
            return;
        }
}

/* ---- the divers' turns ------------------------------------------------------- */

static bool battle_won(void) { return foes_alive() == 0; }

static void hurt_foe(int i, int dmg) {
    SdgFoe *f = &B.foe[i];
    sfx_play_name(f->weakhit ? "sdg_weak" : "sdg_hit");
    f->hp = (int16_t)imax(0, f->hp - dmg);
    f->flash = 12;
    if (f->hp == 0) {
        f->alive = 0;
        f->mark = -1;
        f->guard = -1;
        say("THE %s IS FINISHED.", fname(i));
    }
}

/* a foe covering the target takes the blow instead, and half of it */
static int cover_for(int t) {
    for (int g = 0; g < B.nfoe; g++)
        if (g != t && B.foe[g].alive && B.foe[g].guard == t) return g;
    return -1;
}

static void diver_attack(int d, int it, int t) {
    const SdgItem *I = &SDG_ITEM[it];
    if (!B.foe[t].alive) t = first_alive_foe();
    if (t < 0) return;
    int g = cover_for(t);
    bool covered = g >= 0;
    if (covered) {
        say("THE %s COVERS THE %s!", fname(g), fname(t));
        t = g;
    }
    int hitpct = I->kind == K_BOMB ? 85 : SDG_HIT;
    if (!sdg_chance(hitpct)) {
        sfx_play_name("sdg_miss");
        say("%s MISSES THE %s.", sdg_diver_name(d), fname(t));
        return;
    }
    SdgFoe *f = &B.foe[t];
    int dmg;
    if (I->kind == K_BOMB) dmg = sdg_rand(1500, 2000);
    else dmg = sdg_attack_damage(I->power, sdg_diver_level(&P, d), I->el, f->weak, sdg_rand(90, 110));
    if (covered) dmg /= 2;
    if (f->hidden) {
        /* in its shell a whorl shrugs off any weapon; a charge still shakes it */
        if (I->kind != K_BOMB) {
            say("THE %s'S SHELL TURNS THE BLOW.", fname(t));
            sfx_play_name("sdg_miss");
            goto spines;
        }
        dmg /= 2;
    }
    if (I->kind != K_BOMB && I->el == f->weak) f->weakhit = 24;
    dmg = imax(1, dmg);
    if (I->kind == K_BOMB) say("THE CHARGE BURSTS ON THE %s: %d!", fname(t), dmg);
    else say("%s HITS THE %s: %d!", sdg_diver_name(d), fname(t), dmg);
    hurt_foe(t, dmg);
    /* the leech club drinks half of it, unless that blow ended the fight */
    if ((I->flags & IF_LEECH) && !battle_won() && P.hp[d] > 0) {
        int h = imin(dmg / 2, sdg_diver_maxhp(&P, d) - P.hp[d]);
        if (h > 0) {
            P.hp[d] = (int16_t)(P.hp[d] + h);
            B.leech_healed = (int16_t)(B.leech_healed + h);
            say("%s DRINKS %d.", sdg_diver_name(d), h);
        }
    }
    /* spines: only a shield's blow is safe from them */
spines:;
    const SdgFoeDef *D = &SDG_FOE[f->kind];
    if (I->kind != K_SHIELD && I->kind != K_BOMB && ((D->flags & FF_THORNS) || f->hidden) && P.hp[d] > 1) {
        int c = sdg_rand(60, 120);
        P.hp[d] = (int16_t)imax(1, P.hp[d] - c);
        B.shake[d] = 12;
        B.thorns_taken++;
        sfx_play_name("sdg_ouch");
        say("SPINES! %s TAKES %d.", sdg_diver_name(d), c);
    }
}

static void diver_act(int d) {
    SdgOrder *o = &B.ord[d];
    if (P.hp[d] <= 0 || o->act != ACT_ITEM) return;
    int it = P.equip[d][o->slot];
    if (!it || P.uses[d][o->slot] == 0) return;
    P.uses[d][o->slot]--;
    const SdgItem *I = &SDG_ITEM[it];
    switch (I->kind) {
    case K_POLE:
    case K_HAMMER:
    case K_SHIELD:
        if (o->mode == MODE_ATTACK) diver_attack(d, it, o->target);
        else if (o->mode == MODE_HEAL) {
            int t = o->target;
            if (P.hp[t] > 0) {
                int h = imin(sdg_rand(300, 400), sdg_diver_maxhp(&P, t) - P.hp[t]);
                P.hp[t] = (int16_t)(P.hp[t] + imax(0, h));
                say("THE LAMP ROD GLOWS. %s +%d.", sdg_diver_name(t), imax(0, h));
            } else say("THE LAMP ROD GLOWS. NOTHING.");
        } else {
            B.guarding[d] = o->target;
            B.guard_kind[d] = I->kind;
            if (o->target == d) say("%s GUARDS.", sdg_diver_name(d));
            else say("%s GUARDS %s.", sdg_diver_name(d), sdg_diver_name(o->target));
        }
        break;
    case K_POTION: {
        int t = o->target;
        if (P.hp[t] > 0) {
            int h = imin(I->heal, sdg_diver_maxhp(&P, t) - P.hp[t]);
            P.hp[t] = (int16_t)(P.hp[t] + h);
            say("%s DRINKS A %s: +%d.", sdg_diver_name(t), I->name, h);
        } else say("THE %s IS WASTED.", I->name);
        break;
    }
    case K_EGG: {
        int t = o->target;
        if (P.hp[t] <= 0) {
            P.hp[t] = (int16_t)(sdg_diver_maxhp(&P, t) / 2);
            say("THE EGG CRACKS. %s IS BACK!", sdg_diver_name(t));
        } else say("THE EGG CRACKS. NOTHING.");
        break;
    }
    case K_BOMB: diver_attack(d, it, o->target); break;
    default: break;
    }
}

/* ---- the creatures' turns ------------------------------------------------------ */

/* a blow meant for diver x: who takes it, and how much is left of it */
static void hit_diver(int fi, int x, int dmg) {
    int who = x;
    bool redirected = false;
    for (int d = 0; d < 3; d++)
        if (P.hp[d] > 0 && B.guarding[d] == x) {
            who = d;
            redirected = d != x;
            break;
        }
    if (P.hp[who] <= 0) return;
    /* the shields held, then the defend: with a shield it squares what is
     * left (two bulwarks and a defend leave nothing); a hammer defends like
     * a shield of its cut; a pole only takes the blow. The bonus covers the
     * defender's own blows as well as the ones he takes for another. */
    int m = sdg_shield_mul(&P, who);
    if (B.guarding[who] >= 0 && B.ord[who].act == ACT_ITEM) {
        int used = P.equip[who][B.ord[who].slot];
        if (B.guard_kind[who] == K_SHIELD) {
            int big = 0;
            for (int s = 0; s < 2; s++) big += P.equip[who][s] && SDG_ITEM[P.equip[who][s]].kind == K_SHIELD && SDG_ITEM[P.equip[who][s]].def >= 60;
            m = big >= 2 ? 0 : m * m / 1000;
        } else if (B.guard_kind[who] == K_HAMMER) {
            int h = 100 - SDG_ITEM[used].guard;
            m = m * h * h / 10000;
        }
    }
    dmg = dmg * m / 1000;
    B.last_dmg = (int16_t)dmg;
    B.last_target = (int8_t)who;
    if (redirected) B.covered++;
    if (dmg > 0) sfx_play_name("sdg_ouch");
    if (redirected) say("%s TAKES IT FOR %s: %d.", sdg_diver_name(who), sdg_diver_name(x), dmg);
    else if (dmg == 0) say("THE %s STRIKES %s. NOTHING!", fname(fi), sdg_diver_name(who));
    else say("THE %s STRIKES %s: %d.", fname(fi), sdg_diver_name(who), dmg);
    P.hp[who] = (int16_t)imax(0, P.hp[who] - dmg);
    if (dmg > 0) B.shake[who] = 12;
    SdgFoe *f = &B.foe[fi];
    if (dmg > 0 && (SDG_FOE[f->kind].flags & FF_LEECH) && f->alive) {
        int h = imin(dmg, f->maxhp - f->hp);
        if (h > 0) {
            f->hp = (int16_t)(f->hp + h);
            say("THE %s DRINKS %d.", fname(fi), h);
        }
    }
    if (P.hp[who] == 0) {
        say("%s IS DOWN!", sdg_diver_name(who));
        B.guarding[who] = -1;
    }
    /* the spine targe bites back */
    if (sdg_holds(&P, who, IT_SPINE) && f->alive) {
        int c = sdg_rand(50, 150);
        say("SPINES BITE THE %s: %d.", fname(fi), c);
        hurt_foe(fi, c);
    }
}

static int lowest_diver(void) {
    int best = -1, bv = 1 << 30;
    bool all_full = true;
    for (int d = 0; d < 3; d++) {
        if (P.hp[d] <= 0) continue;
        if (P.hp[d] < sdg_diver_maxhp(&P, d)) all_full = false;
        if (P.hp[d] < bv) { bv = P.hp[d]; best = d; }
    }
    if (all_full) return next_orderable(0);
    return best;
}

static void foe_attack(int i, int target, bool heavy) {
    const SdgFoeDef *D = &SDG_FOE[B.foe[i].kind];
    if (target < 0 || P.hp[target] <= 0) target = (D->flags & FF_LOWEST) ? lowest_diver() : random_alive_diver();
    if (target < 0) return;
    if (!sdg_chance(SDG_HIT)) {
        say("THE %s MISSES %s.", fname(i), sdg_diver_name(target));
        return;
    }
    int dmg = sdg_rand(D->lo, D->hi);
    if (heavy) dmg = dmg * 9 / 5;
    B.last_heavy = heavy;
    hit_diver(i, target, dmg);
}

static void foe_act(int i) {
    SdgFoe *f = &B.foe[i];
    if (!f->alive) return;
    const SdgFoeDef *D = &SDG_FOE[f->kind];
    f->guard = -1;
    if (sdg_alive_count(&P) == 0) return;
    /* the whorl's shell */
    if (D->flags & FF_HIDE) {
        if (f->hidden) {
            if (sdg_chance(50)) { say("THE %s STAYS IN ITS SHELL.", fname(i)); return; }
            f->hidden = 0;
            say("THE %s COMES OUT.", fname(i));
        } else if (sdg_chance(30)) {
            f->hidden = 1;
            say("THE %s PULLS INTO ITS SHELL.", fname(i));
            return;
        }
    }
    /* a look, then the heavy blow */
    if (f->mark >= 0) {
        int t = f->mark;
        f->mark = -1;
        if (P.hp[t] > 0) {
            B.last_heavy = 1;
            if (sdg_chance(SDG_HIT)) hit_diver(i, t, sdg_rand(D->lo, D->hi) * 9 / 5);
            else say("THE %s MISSES %s.", fname(i), sdg_diver_name(t));
            return;
        }
    }
    /* bring a fallen one back */
    if (D->flags & FF_REVIVE) {
        int dead = -1, ndead = 0, nfr = 0;
        for (int k = 0; k < B.nfoe; k++) {
            if (k == i || SDG_FOE[B.foe[k].kind].flags & FF_BOSS) continue;
            nfr++;
            if (!B.foe[k].alive) { ndead++; if (dead < 0) dead = k; }
        }
        bool must = f->kind == EN_WARDEN && ndead > 0 && ndead == nfr;
        if (dead >= 0 && (must || sdg_chance(30))) {
            SdgFoe *r = &B.foe[dead];
            r->alive = 1;
            r->hp = (int16_t)(r->maxhp / 2);
            r->mark = -1;
            r->guard = -1;
            B.revives++;
            say("THE %s BRINGS BACK THE %s!", fname(i), fname(dead));
            return;
        }
    }
    /* heal the most hurt */
    if (D->flags & FF_HEAL) {
        int best = -1, bv = 1000;
        for (int k = 0; k < B.nfoe; k++) {
            if (!B.foe[k].alive) continue;
            int v = B.foe[k].hp * 100 / B.foe[k].maxhp;
            if (v < 60 && v < bv) { bv = v; best = k; }
        }
        if (best >= 0 && sdg_chance(f->kind == EN_EYE ? 60 : 40)) {
            SdgFoe *h = &B.foe[best];
            int amt = imin(D->heal, h->maxhp - h->hp);
            h->hp = (int16_t)(h->hp + amt);
            B.heals++;
            if (best == i) say("THE %s MENDS ITSELF: +%d.", fname(i), amt);
            else say("THE %s MENDS THE %s: +%d.", fname(i), fname(best), amt);
            return;
        }
    }
    if ((D->flags & FF_TELEGRAPH) && sdg_chance(45)) {
        int t = random_alive_diver();
        f->mark = (int8_t)t;
        say("THE %s IS LOOKING AT %s.", fname(i), sdg_diver_name(t));
        return;
    }
    if ((D->flags & FF_GUARD) && foes_alive() > 1 && sdg_chance(25)) {
        int pick[SDG_FOES], n = 0;
        for (int k = 0; k < B.nfoe; k++)
            if (k != i && B.foe[k].alive) pick[n++] = k;
        f->guard = (int8_t)pick[sdg_rand(0, n - 1)];
        say("THE %s COVERS THE %s.", fname(i), fname(f->guard));
        return;
    }
    if ((D->flags & FF_IDLE) && sdg_chance(20)) {
        say("THE %s NOODLES ABOUT.", fname(i));
        return;
    }
    int t = (D->flags & FF_LOWEST) ? lowest_diver() : random_alive_diver();
    int hits = (D->flags & FF_DOUBLE) ? 2 : ((D->flags & FF_MAYDOUBLE) && sdg_chance(50)) ? 2 : 1;
    for (int h = 0; h < hits && f->alive && sdg_alive_count(&P) > 0; h++) {
        foe_attack(i, h == 0 ? t : -1, false);
    }
}

/* ---- winning and losing ------------------------------------------------------- */

static void victory(void) {
    B.xp = 0;
    B.gold = 0;
    for (int i = 0; i < B.nfoe; i++) {
        B.xp += SDG_FOE[B.foe[i].kind].xp;
        B.gold += SDG_FOE[B.foe[i].kind].gold;
    }
    int lv = P.level;
    say("THE WATER GOES QUIET.");
    if (B.xp || B.gold) say("%ld XP AND %ld GOLD.", (long)B.xp, (long)B.gold);
    sdg_add_xp(&P, (int)B.xp);
    P.gold = sdg_cap_gold(P.gold + B.gold);
    for (int i = 0; i < B.nfoe; i++) {
        const SdgFoeDef *D = &SDG_FOE[B.foe[i].kind];
        if (D->drop < 0 || B.foe[i].nodrop) continue;
        bool sure = (D->flags & FF_BOSS) != 0;
        if (sure || sdg_chance(SDG_DROP)) {
            if (sdg_relic_total(&P, D->drop) < SDG_RELIC_MAX) {
                sdg_gain_relic(&P, D->drop);
                say("FOUND A %s.", SDG_RELIC_NAME[D->drop]);
                B.drop = D->drop;
            }
        }
    }
    if (P.level > lv) {
        say("LEVEL %d!", P.level);
        B.levelled = 1;
    }
    B.phase = BP_WIN;
}

static void defeat(void) {
    say("THE DIVERS ARE LOST TO THE DARK...");
    B.wipe = 1;
    B.phase = BP_LOSE;
}

static void end_round(void) {
    for (int i = 0; i < B.nfoe; i++) {
        SdgFoe *f = &B.foe[i];
        if (f->alive && (SDG_FOE[f->kind].flags & FF_SHIFT)) {
            /* yellow (reef), red (ooze), green (zap), round and round */
            f->weak = f->weak == EL_REEF ? EL_OOZE : f->weak == EL_OOZE ? EL_ZAP : EL_REEF;
        }
    }
    start_round();
}

static void resolve_next(void) {
    for (;;) {
        if (battle_won()) { victory(); return; }
        if (sdg_alive_count(&P) == 0) { defeat(); return; }
        int a = B.actor++;
        if (a < 3) {
            if (B.ord[a].act == ACT_ITEM && P.hp[a] > 0) { diver_act(a); return; }
            continue;
        }
        int i = a - 3;
        if (i < B.nfoe) {
            if (B.foe[i].alive) { foe_act(i); return; }
            continue;
        }
        end_round();
        return;
    }
}

/* ---- one frame ------------------------------------------------------------------ */

static void try_run(bool sure) {
    B.ran = 1;
    if (sure || sdg_chance(SDG_ESCAPE)) {
        say(sure ? "INK CLOUDS THE WATER. AWAY!" : "THE DIVERS SLIP AWAY!");
        B.phase = BP_FLED;
        return;
    }
    say("NO WAY OUT!");
    for (int d = 0; d < 3; d++) B.ord[d].act = ACT_NONE;
    begin_resolve();
    B.actor = 3;
}

void sdg_battle_step(unsigned pr) {
    for (int i = 0; i < B.nfoe; i++) {
        if (B.foe[i].flash > 0) B.foe[i].flash--;
        if (B.foe[i].weakhit > 0) B.foe[i].weakhit--;
    }
    for (int d = 0; d < 3; d++)
        if (B.shake[d] > 0) B.shake[d]--;
    if (B.nmsg > 0) {
        B.msg_t++;
        if (B.msg_t >= SDG_MSG_T || ((pr & BTN_A) && B.msg_t >= SDG_MSG_MIN)) {
            for (int k = 1; k < B.nmsg; k++) B.msg[k - 1] = B.msg[k];
            B.nmsg--;
            B.msg_t = 0;
        }
        return;
    }
    switch (B.phase) {
    case BP_INTRO: start_round(); break;
    case BP_ORDER: {
        int d = B.cur;
        if (d < 0) { begin_resolve(); break; }
        if (pr & BTN_UP) { B.menu = (B.menu + 3) % 4; sfx_play_name("sdg_tick"); }
        if (pr & BTN_DOWN) { B.menu = (B.menu + 1) % 4; sfx_play_name("sdg_tick"); }
        if (pr & BTN_B) { prev_diver(); sfx_play_name("ui_back"); break; }
        if (pr & BTN_A) {
            if (B.menu == 2) {
                B.ord[d].act = ACT_PASS;
                sfx_play_name("sdg_tick");
                next_diver();
            } else if (B.menu == 3) {
                if (B.boss) { sfx_play_name("sdg_nope"); break; }
                sfx_play_name("sdg_tick");
                try_run(false);
            } else if (sdg_order_valid(d, B.menu)) {
                int it = P.equip[d][B.menu];
                B.ord[d].act = ACT_ITEM;
                B.ord[d].slot = (uint8_t)B.menu;
                sfx_play_name("sdg_tick");
                if (SDG_ITEM[it].kind == K_MIST) {
                    P.uses[d][B.menu]--;
                    try_run(true);
                } else open_target();
            } else sfx_play_name("sdg_nope");
        }
        break;
    }
    case BP_TARGET: {
        int d = B.cur, it = P.equip[d][B.menu];
        int k = SDG_ITEM[it].kind;
        bool weapon = k == K_POLE || k == K_HAMMER || k == K_SHIELD;
        if (weapon && (pr & (BTN_UP | BTN_DOWN))) {
            int alt = (SDG_ITEM[it].flags & IF_HOLY) ? MODE_HEAL : MODE_DEFEND;
            if (B.tmode == MODE_ATTACK) { B.tmode = alt; B.tsel = d; }
            else { B.tmode = MODE_ATTACK; B.tsel = first_alive_foe(); }
            sfx_play_name("sdg_tick");
        }
        int tm = B.tmode == MODE_ATTACK ? MODE_ATTACK : MODE_DEFEND;
        if (pr & BTN_LEFT) { B.tsel = cycle_target(tm, B.tsel, -1); sfx_play_name("sdg_tick"); }
        if (pr & BTN_RIGHT) { B.tsel = cycle_target(tm, B.tsel, 1); sfx_play_name("sdg_tick"); }
        if (pr & BTN_B) { B.phase = BP_ORDER; B.ord[d].act = ACT_NONE; sfx_play_name("ui_back"); break; }
        if ((pr & BTN_A) && target_ok(tm, B.tsel)) {
            B.ord[d].mode = (uint8_t)B.tmode;
            B.ord[d].target = (int8_t)B.tsel;
            sfx_play_name("sdg_tick");
            next_diver();
        }
        break;
    }
    case BP_RESOLVE: resolve_next(); break;
    case BP_WIN:
    case BP_LOSE:
    case BP_FLED:
        if (pr & BTN_A) B.phase = BP_DONE;
        break;
    default: break;
    }
}
