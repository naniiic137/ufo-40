/* IRONVOW - the fights. Real time, seen over the knight's shoulder: the
 * knight and his foes move only side to side, and a blow lands on whoever
 * stands in line with it. Two rows of foes, at most two in each; only the
 * front row can be struck by most blades, and it does most of the striking.
 * Most foes guard until they strike: bait the blow, guard or roll, then
 * punish the opening. Stamina pays for swings, blocks and rolls.
 * Positions are in sixteenths of a pixel. */
#include "ivw.h"

#define PX(v) ((v) * 16)
#define ARENA_MIN PX(12)
#define ARENA_MAX PX(IVW_ARENA_W - 12)
#define ROW_GAP PX(26)
#define ROLL_FRAMES 16
#define ROLL_COST 18
#define ROLL_WINDOW 8
#define CHARGE_FRAMES 40

static IvwFight *F = &ig.f;
int ivw_no_loot; /* tests: fights leave nothing */

/* ---- the knight's kit ------------------------------------------------------------------------ */
int ivw_weapon_item(void) { return ig.hero.wpn >= 0 ? ig.hero.inv[ig.hero.wpn] : IT_NONE; }
int ivw_shield_item(void) { return ig.hero.shd >= 0 ? ig.hero.inv[ig.hero.shd] : IT_NONE; }
int ivw_armour_item(void) { return ig.hero.arm >= 0 ? ig.hero.inv[ig.hero.arm] : IT_NONE; }

static bool playing_beetle(void) { return F->beetle && !F->beetle_out; }

int ivw_player_speed(void) {
    if (playing_beetle()) return 26;
    int sp = 20;
    int arm = ivw_armour_item();
    int slow = arm ? IVW_ITEM[arm].b : 0;
    if (slow && ig.hero.stat[STAT_MIGHT] >= 10) slow /= 2;
    sp = sp * (100 - slow) / 100;
    int shd = ivw_shield_item();
    if (shd && (IVW_ITEM[shd].c & 1)) sp = sp * 13 / 10;
    if (F->haste > 0) sp = sp * 17 / 10;
    if (F->exhausted) sp /= 2;
    return imax(4, sp);
}

static int weapon_flags(void) {
    int w = ivw_weapon_item();
    int f = w ? IVW_ITEM[w].c : 0;
    int shd = ivw_shield_item();
    if (shd && (IVW_ITEM[shd].c & 8)) f |= WF_CHARGE;
    return f;
}

static const IvwSwing *swing_of(void) {
    int w = ivw_weapon_item();
    if (playing_beetle()) return &IVW_SWING[1];
    return &IVW_SWING[w ? IVW_ITEM[w].b : 1];
}

static int swing_cost(void) {
    if (playing_beetle()) return PX(4);
    int w = ivw_weapon_item(), arm = ivw_armour_item();
    int c = swing_of()->cost + (arm ? IVW_ITEM[arm].c : 0) - ig.hero.stat[STAT_MIGHT] / 3;
    if (w && ig.hero.stat[STAT_MIGHT] < IVW_ITEM[w].req) c = c * 3 / 2;
    return PX(imax(3, c));
}

static int blow_damage(void) {
    if (playing_beetle()) return 9;
    int w = ivw_weapon_item();
    int d = w ? IVW_ITEM[w].a : 2;
    d = d * (8 + ig.hero.stat[STAT_MIGHT]) / 13;
    if (w && ig.hero.stat[STAT_MIGHT] < IVW_ITEM[w].req) d = d * 2 / 3;
    if (ig.buff & 1) d = d * 3 / 2;
    return imax(1, d);
}

/* ---- foes ---------------------------------------------------------------------------------------- */

bool ivw_foe_aligned(const IvwFoe *e, int extra) {
    int r = IVW_FOE[e->kind].range + extra;
    return iabs(e->x - F->px) <= PX(r);
}

int ivw_fight_foes_alive(void) {
    int n = 0;
    for (int i = 0; i < F->nfoe; i++) n += F->foe[i].alive && F->foe[i].kind != FK_STONE;
    return n;
}

static int row_count(int row) {
    int n = 0;
    for (int i = 0; i < F->nfoe; i++)
        if (F->foe[i].alive && F->foe[i].row == row && F->foe[i].kind != FK_CLAW && F->foe[i].kind != FK_STONE &&
            F->foe[i].kind != FK_MATRON) n++;
    return n;
}

static IvwFoe *add_foe(int kind, int row, int x) {
    if (F->nfoe >= IVW_MAX_FOES) {
        /* reuse a dead slot */
        for (int i = 0; i < F->nfoe; i++)
            if (!F->foe[i].alive) { memmove(&F->foe[i], &F->foe[i + 1], sizeof F->foe[0] * (size_t)(F->nfoe - i - 1)); F->nfoe--; break; }
        if (F->nfoe >= IVW_MAX_FOES) return NULL;
    }
    IvwFoe *e = &F->foe[F->nfoe++];
    memset(e, 0, sizeof *e);
    e->kind = (uint8_t)kind;
    e->alive = 1;
    e->row = (uint8_t)row;
    e->x = (int16_t)PX(x);
    e->tx = e->x;
    e->hp = e->hpmax = IVW_FOE[kind].hp;
    e->cd = (int16_t)rng_range(&ig.rng, 40, 100);
    e->aux = (int16_t)rng_range(&ig.rng, 240, 420);
    e->state = kind == FK_EGG ? FS_HATCH : FS_MOVE;
    if (kind == FK_EGG) e->aux = 0; /* the hatching clock */
    return e;
}

static void foe_hurt(IvwFoe *e, int dmg, bool pierce, int stun);

/* ---- shots ----------------------------------------------------------------------------------------- */
static IvwShot *add_shot(int kind, int from_x, int to_x, int t_max, int dmg, int stdmg) {
    for (int i = 0; i < IVW_MAX_SHOTS; i++)
        if (!F->shot[i].alive) {
            IvwShot *s = &F->shot[i];
            *s = (IvwShot){(uint8_t)kind, 1, 0, (int16_t)iclamp(to_x, ARENA_MIN, ARENA_MAX), (int16_t)from_x, 0,
                           (int16_t)t_max, (uint8_t)dmg, (uint8_t)stdmg};
            return s;
        }
    return NULL;
}

/* ---- the knight is struck -------------------------------------------------------------------------- */
enum { HIT_BLOCKABLE = 1, HIT_POISON = 2, HIT_BURN = 4, HIT_SLOW = 8 };

static void player_dead_check(void);

static void exhaust(void) {
    if (F->exhausted) return;
    F->exhausted = 1;
    ivw_sfx("ivw_gasp");
    /* chitins punish an empty tank at once, front row or back */
    for (int i = 0; i < F->nfoe; i++) {
        IvwFoe *e = &F->foe[i];
        if (e->alive && e->kind == FK_CHITIN && !e->freeze && e->state != FS_STUN) {
            e->state = FS_ACTIVE;
            e->t = 0;
            if (F->pstate != PS_DODGE) {
                int dmg = IVW_FOE[FK_CHITIN].dmg;
                if (playing_beetle()) ig.hero.beetle_hp = (int16_t)(ig.hero.beetle_hp - dmg);
                else ig.hero.hp = (int16_t)(ig.hero.hp - dmg);
                F->flash_hurt = 12;
            }
        }
    }
    player_dead_check();
}

static void spend_st(int amount) {
    if (playing_beetle()) return;
    ig.hero.st = (int16_t)(ig.hero.st - amount);
    if (ig.hero.st <= 0) { ig.hero.st = 0; exhaust(); }
}

static bool player_invulnerable(void) {
    return F->pstate == PS_DODGE && F->pt >= 1 && F->pt <= 12;
}

/* returns true if the blow was taken (not dodged, warded or blocked) */
static bool player_hit(int dmg, int stdmg, int flags) {
    IvwHero *h = &ig.hero;
    if (F->result) return false;
    if (player_invulnerable()) return false;
    if (F->ward > 0) {
        F->ward = (int16_t)(F->ward - dmg);
        ivw_sfx("ivw_ward");
        if (F->ward <= 0) { F->ward = 0; ivw_say("THE WARD SHATTERS."); }
        return false;
    }
    if (playing_beetle()) {
        if (F->pstate == PS_GUARD && (flags & HIT_BLOCKABLE)) { ivw_sfx("ivw_block"); return false; }
        h->beetle_hp = (int16_t)(h->beetle_hp - dmg);
        F->flash_hurt = 12;
        ivw_sfx("ivw_hurt");
        player_dead_check();
        return true;
    }
    int shd = ivw_shield_item();
    bool can_block = shd && !(IVW_ITEM[shd].c & 1) && F->blocks_left > 0 && !F->exhausted;
    if (F->pstate == PS_GUARD && (flags & HIT_BLOCKABLE) && can_block) {
        int pct = IVW_ITEM[shd].b;
        if (ig.buff & 4) pct /= 2;
        int cost = PX(stdmg) * pct / 100;
        F->blocks_left--;
        ivw_sfx("ivw_block");
        if (IVW_ITEM[shd].c & 2) h->mp = (int16_t)imin(ivw_max_mp(h), h->mp + 4);
        if (F->blocks_left <= 0) ivw_say("YOUR %s GIVES WAY!", IVW_ITEM[shd].name);
        if (h->st < cost) {
            /* nothing left to hold it with: the guard breaks */
            h->st = 0;
            F->pstate = PS_STAGGER;
            F->pt = -40;
            exhaust();
            ivw_say("YOUR GUARD BREAKS!");
        } else spend_st(cost);
        return false;
    }
    int arm = ivw_armour_item();
    int g = arm ? IVW_ITEM[arm].a : 0;
    int taken = imax(1, dmg * (100 - g) / 100);
    h->hp = (int16_t)(h->hp - taken);
    if (arm == IT_MOULD) {
        F->recover_hp = (int16_t)(F->recover_hp + taken * 3 / 4);
        for (int i = 0; i < F->nfoe; i++) {
            IvwFoe *e = &F->foe[i];
            if (e->alive && e->row == 0 && ivw_foe_aligned(e, 8) && !(IVW_FOE[e->kind].flags & FF_NOPOISON))
                e->hp = (int16_t)(e->hp - 2);
        }
    }
    if (flags & HIT_POISON) h->poison = (uint8_t)imax(h->poison, 8);
    if (flags & HIT_BURN) h->burn = (uint8_t)imax(h->burn, 4);
    F->flash_hurt = 12;
    if (F->pstate == PS_SWING || F->pstate == PS_CHARGE || F->pstate == PS_IDLE || F->pstate == PS_GUARD) {
        F->pstate = PS_STAGGER;
        F->pt = -14;
    }
    ivw_sfx("ivw_hurt");
    player_dead_check();
    return true;
}

static void player_dead_check(void) {
    IvwHero *h = &ig.hero;
    if (F->beetle && !F->beetle_out && h->beetle_hp <= 0) {
        h->beetle_hp = 0;
        int s = ivw_find_item(IT_BEETLE);
        if (s >= 0) ivw_remove_slot(s);
        F->beetle_out = 1;
        ivw_say("YOUR BEETLE FALLS. YOU STEP IN.");
        F->pstate = PS_IDLE;
        return;
    }
    if (h->hp <= 0) {
        h->hp = 0;
        F->result = 2;
    }
}

/* ---- the knight's blows ------------------------------------------------------------------------------ */

static void foe_hurt(IvwFoe *e, int dmg, bool pierce, int stun) {
    const IvwFoeDef *d = &IVW_FOE[e->kind];
    (void)pierce;
    if (!e->alive) return;
    if (e->kind == FK_MATRON) /* behind her claws, nothing reaches her */
        for (int i = 0; i < F->nfoe; i++)
            if (F->foe[i].alive && F->foe[i].kind == FK_CLAW) return;
    e->hp = (int16_t)(e->hp - dmg);
    if (e->kind == FK_EGG) { if (e->hp <= 0) e->alive = 0; return; }
    if (e->hp <= 0) {
        e->hp = 0;
        e->alive = 0;
        e->state = FS_DEAD;
        ivw_sfx("ivw_kill");
        if (e->kind == FK_REGENT) {
            ig.dawn_kill = ivw_weapon_item() == IT_DAWN && !playing_beetle();
            for (int i = 0; i < F->nfoe; i++) F->foe[i].alive = 0;
        }
        if (e->kind == FK_CLAW) {
            bool any = false;
            for (int i = 0; i < F->nfoe; i++) any |= F->foe[i].alive && F->foe[i].kind == FK_CLAW;
            if (!any)
                for (int i = 0; i < F->nfoe; i++)
                    if (F->foe[i].alive && F->foe[i].kind == FK_MATRON) {
                        F->foe[i].row = 0;
                        ivw_say("THE MATRON SINKS, OPEN TO YOUR BLADE!");
                    }
        }
        return;
    }
    if (d->flags & FF_BOSS && e->kind != FK_CLAW && e->kind != FK_STONE) stun = stun / 2;
    if (e->state == FS_WINDUP || e->state == FS_ACTIVE || e->state == FS_CAST) {
        /* a counter: the blow is cut off and the foe reels */
        e->state = FS_STUN;
        e->t = 0;
        e->stun = (int16_t)imax(stun, e->kind == FK_REGENT || e->kind == FK_WARDEN ? 30 : 50);
        e->strikes = 0;
    } else if (stun > 0) {
        e->state = FS_STUN;
        e->t = 0;
        e->stun = (int16_t)imax(e->stun, stun);
    } else if (e->state != FS_STUN) {
        e->state = FS_STUN;
        e->t = 0;
        e->stun = 12;
    }
}

/* can this foe turn the blow aside? */
static bool foe_guards(const IvwFoe *e) {
    const IvwFoeDef *d = &IVW_FOE[e->kind];
    if (F->invis > 0 || e->freeze > 0) return false;
    if (e->kind == FK_PUFF) return e->state == FS_CAST;
    if (!(d->flags & FF_GUARD)) return false;
    if (e->kind == FK_COILER && e->alt && e->state == FS_WINDUP) return false;
    if (e->kind == FK_MATRON) return false;
    return e->state == FS_MOVE || e->state == FS_HATCH;
}

/* the blow lands: on whom? */
static void player_blow(bool pierce, int mult_num, int mult_den) {
    int flags = playing_beetle() ? 0 : weapon_flags();
    int reach = (flags & WF_SWEEP) ? PX(30) : PX(16);
    int dmg = blow_damage() * mult_num / mult_den;
    int stun = (flags & WF_STUN) && rng_chance(&ig.rng, 30) ? 45 : 0;
    bool hit_any = false;
    for (int pass = 0; pass < 2; pass++) {
        int best = -1, bestd = 1 << 30;
        for (int i = 0; i < F->nfoe; i++) {
            IvwFoe *e = &F->foe[i];
            if (!e->alive) continue;
            bool front = e->row == 0;
            if (e->kind == FK_STONE) front = e->aux <= 0; /* a stone near the ground */
            if (e->kind == FK_MATRON && e->row != 0) continue;
            if (pass == 0 ? !front : (front || !(flags & WF_REACH))) continue;
            int dx = iabs(e->x - F->px);
            if (dx > reach) continue;
            if (flags & WF_SWEEP && pass == 0) {
                if (!pierce && foe_guards(e)) { e->blocks++; ivw_sfx("ivw_clink"); }
                else foe_hurt(e, dmg, pierce, stun);
                hit_any = true;
                continue;
            }
            if (dx < bestd) { bestd = dx; best = i; }
        }
        if (best >= 0) {
            IvwFoe *e = &F->foe[best];
            if (!pierce && foe_guards(e)) {
                e->blocks++;
                ivw_sfx("ivw_clink");
                /* pressed too long behind its guard, it hits back */
                if (e->blocks >= 3 && IVW_FOE[e->kind].dmg) { e->blocks = 0; e->state = FS_WINDUP; e->t = IVW_FOE[e->kind].windup / 2; }
            } else {
                foe_hurt(e, dmg, pierce, stun);
                if ((flags & 4) == 0 && ivw_shield_item() == IT_SKINKB && !(IVW_FOE[e->kind].flags & FF_NOPOISON))
                    e->poison = 400;
                ivw_sfx("ivw_hit");
            }
            hit_any = true;
        }
    }
    if (!hit_any) ivw_sfx("ivw_swish");
    /* the twin strikes alongside */
    if (F->twin > 0)
        for (int i = 0; i < F->nfoe; i++) {
            IvwFoe *e = &F->foe[i];
            if (e->alive && e->row == 0 && iabs(e->x - F->px) <= PX(40) && e->kind != FK_STONE) {
                foe_hurt(e, imax(1, dmg / 2), true, 0);
                break;
            }
        }
    /* hale with the Dawnbrand: a bolt of light flies up the line */
    if ((flags & WF_BOLT) && ig.hero.hp * 10 >= ivw_max_hp(&ig.hero) * 9 && ig.hero.mp >= 2) {
        ig.hero.mp = (int16_t)(ig.hero.mp - 2);
        IvwShot *s = add_shot(SH_MYBOLT, F->px, F->px, 18, 6 + ig.hero.stat[STAT_LORE] / 2, 0);
        if (s) s->from = 1;
    }
}

/* ---- starting and ending ------------------------------------------------------------------------------- */

void ivw_fight_begin(const uint8_t *kinds, int n, int boss) {
    IvwHero *h = &ig.hero;
    memset(F, 0, sizeof *F);
    F->active = 1;
    F->boss = (uint8_t)boss;
    F->flame = 255;
    F->mimic_obj = 255;
    F->px = (int16_t)PX(88);
    F->pstate = PS_IDLE;
    int shd = ivw_shield_item();
    F->blocks_left = (int16_t)(shd ? IVW_ITEM[shd].a : 0);
    if (ig.buff & 4) F->blocks_left = (int16_t)(F->blocks_left * 3 / 2 + 2);
    if (ig.buff & 2) F->haste = 30000;
    if (h->st < ivw_max_st(h) / 2) h->st = (int16_t)(ivw_max_st(h) / 2);
    if (boss == BOSS_MATRON) {
        IvwFoe *m = add_foe(FK_MATRON, 1, 88);
        if (m) m->cd = 120;
        add_foe(FK_CLAW, 0, 40);
        add_foe(FK_CLAW, 0, 136);
        add_foe(FK_CHITIN, 0, 70);
        add_foe(FK_CHITIN, 0, 106);
    } else if (boss == BOSS_REGENT) {
        add_foe(FK_REGENT, 0, 88);
        for (int k = 0; k < 3; k++) {
            IvwFoe *s = add_foe(FK_STONE, 1, 40 + 48 * k);
            if (s) { s->alt = (uint8_t)k; s->aux = (int16_t)(200 + 90 * k); s->cd = (int16_t)(120 + 60 * k); }
        }
        F->regent_phase = 1;
    } else {
        static const int X1[1] = {88}, X2[2] = {58, 118};
        int nf = imin(n, 2), nb = n - nf;
        for (int i = 0; i < nf; i++) add_foe(kinds[i], 0, nf == 1 ? X1[0] : X2[i]);
        for (int i = 0; i < nb; i++) add_foe(kinds[nf + i], 1, nb == 1 ? X1[0] : X2[i]);
        if (boss == BOSS_WARDEN) {
            IvwFoe *w = &F->foe[0];
            int lit = 0, all = 0, l;
            for (int f = 0; f < 2; f++) { lit += h->torch_lit[f]; all += h->torch_all[f]; }
            all += ivw_torches(&l);
            lit += l;
            if (lit == all && all > 0) { w->hp = (int16_t)(w->hp / 2); ivw_say("THE TORCHLIGHT HAS WEAKENED HIM!"); }
        }
    }
    /* the Warden's foes: none. A beetle gone wild joins the other side */
    int bs = ivw_find_item(IT_BEETLE);
    if (bs >= 0 && h->ready[bs]) {
        if (h->beetle_fights >= 20 && rng_chance(&ig.rng, 10)) {
            ivw_remove_slot(bs);
            add_foe(FK_BEETLE, row_count(0) < 2 ? 0 : 1, 30);
            ivw_say("YOUR BEETLE HAS GONE WILD!");
        } else {
            F->beetle = 1;
            if (h->beetle_hp <= 0) h->beetle_hp = 50;
            h->beetle_fights = (uint8_t)imin(250, h->beetle_fights + 1);
        }
    }
    /* readied parts go off now */
    for (int s = 0; s < h->inv_cap; s++) {
        if (!h->inv[s] || !h->ready[s]) continue;
        int it = h->inv[s];
        if (IVW_ITEM[it].kind != IK_PART) continue;
        IvwFoe *t = NULL;
        for (int i = 0; i < F->nfoe; i++) if (F->foe[i].alive && F->foe[i].kind != FK_STONE) { t = &F->foe[i]; break; }
        switch (it) {
        case IT_HEART: h->hp = (int16_t)imin(ivw_max_hp(h), h->hp + IVW_ITEM[it].a); break;
        case IT_TUSK: if (t) foe_hurt(t, IVW_ITEM[it].a + h->stat[STAT_MIGHT], true, 20); break;
        case IT_SAC: if (t && !(IVW_FOE[t->kind].flags & FF_NOPOISON)) t->poison = (int16_t)IVW_ITEM[it].a; break;
        case IT_OOZESKULL: case IT_BONESKULL: case IT_BALL:
            F->ball_kind = (uint8_t)it;
            F->ball_t = (int16_t)IVW_ITEM[it].a;
            F->ball_x = (int16_t)PX(20);
            F->ball_v = 40;
            break;
        default: continue;
        }
        ivw_remove_slot(s);
        s--;
    }
    if (!F->ball_kind && ivw_find_item(IT_GRUB) >= 0) {
        F->ball_kind = IT_GRUB;
        F->ball_t = 30000;
        F->ball_x = (int16_t)PX(150);
        F->ball_v = -36;
    }
    if (ig.buff & 8)
        for (int i = 0; i < F->nfoe; i++) { F->foe[i].state = F->foe[i].state == FS_HATCH ? FS_HATCH : FS_STUN; F->foe[i].stun = 150; }
    ig.cast.on = 0;
    ivw_set_state(VS_FIGHT);
    ivw_sfx("ivw_fight");
    if (boss) music_play(boss == BOSS_REGENT ? IVW_MUS_REGENT : IVW_MUS_BOSS);
}

static int essence_for(int kind) {
    int e = IVW_FOE[kind].essence;
    int b = ig.w.battles;
    if (b < 6) return e * (2 + ig.hero.floor) / 2;
    if (b < 12) return e;
    return imin(e, 5);
}

static void fight_won(void) {
    IvwHero *h = &ig.hero;
    F->result = 1;
    int ess = 0;
    F->nloot = 0;
    for (int i = 0; i < F->nfoe; i++) {
        IvwFoe *e = &F->foe[i];
        if (e->state != FS_DEAD) continue;
        ess += essence_for(e->kind);
        const IvwFoeDef *d = &IVW_FOE[e->kind];
        if (d->drop && F->nloot < 4 && !ivw_no_loot && rng_chance(&ig.rng, 30)) {
            int it = d->drop2 && rng_chance(&ig.rng, 50) ? d->drop2 : d->drop;
            F->loot[F->nloot++] = (uint8_t)it;
        }
    }
    if (F->boss == BOSS_WARDEN) {
        int lit = 0, l;
        for (int f = 0; f < 2; f++) lit += h->torch_lit[f];
        ivw_torches(&l);
        lit += l;
        if (lit == 0) F->loot[F->nloot++] = IT_WRAITH;
    }
    if (F->boss == BOSS_MATRON && F->nloot < 4) F->loot[F->nloot++] = IT_MOULD;
    if (F->pedlar) ess = IVW_FOE[FK_PEDLAR].essence;
    F->essence_won = (int16_t)ess;
    h->essence += ess;
    ig.w.battles = (uint8_t)imin(250, ig.w.battles + 1);
    if (igs.battles < 0xFFFFFFF0u && !ig.demo) igs.battles++;
    /* the egg, the grub, the beetle grow with every fight */
    int s = ivw_find_item(IT_EGG);
    if (s >= 0 && ++h->egg_fights >= 6) { h->inv[s] = IT_GRUB; ivw_say("THE EGG HATCHES: A FIGHTING GRUB!"); }
    s = ivw_find_item(IT_GRUB);
    if (s >= 0 && F->ball_kind == IT_GRUB && ++h->grub_fights >= 8) {
        h->inv[s] = IT_BEETLE;
        h->beetle_hp = 50;
        ivw_say("THE GRUB IS A WAR BEETLE NOW!");
    }
    if (F->beetle && h->beetle_hp > 0) h->beetle_hp = 50;
    ig.buff = 0;
    ivw_set_state(VS_LOOT);
    if (F->boss == BOSS_REGENT) music_stop();
    ivw_sfx("ivw_win");
}

/* ---- each frame of a fight ---------------------------------------------------------------------------------- */

static int foe_target_x(const IvwFoe *e) {
    /* the foe nearest the knight in its row comes to him; another stands aside */
    if (e->kind == FK_CLAW) return e->tx;
    int nearest = 1 << 30, me = iabs(e->x - F->px);
    for (int i = 0; i < F->nfoe; i++) {
        const IvwFoe *o = &F->foe[i];
        if (o == e || !o->alive || o->row != e->row || o->kind == FK_STONE || o->kind == FK_CLAW) continue;
        nearest = imin(nearest, iabs(o->x - F->px));
    }
    if (me <= nearest) return F->px;
    return F->px + (e->x < F->px ? -PX(30) : PX(30));
}

static void foe_strike(IvwFoe *e) {
    const IvwFoeDef *d = &IVW_FOE[e->kind];
    int flags = HIT_BLOCKABLE;
    int dmg = d->dmg, st = d->stdmg;
    switch (e->kind) {
    case FK_COILER:
        if (e->alt) { flags = 0; dmg = 16; }
        e->alt ^= 1;
        break;
    case FK_WARDEN:
        if (e->strikes) { /* the yellow charge: right across the hall */
            if (!player_invulnerable()) player_hit(12, 25, HIT_BLOCKABLE);
            return;
        }
        break;
    case FK_REGENT:
        if (e->alt == 2) { /* the dark sweep: no guard holds it */
            if (!player_invulnerable()) player_hit(14, 0, 0);
            return;
        }
        break;
    case FK_SILVER: flags |= HIT_POISON; break;
    case FK_SKINK: e->alt ^= 1; break;
    default: break;
    }
    if (ivw_foe_aligned(e, 0)) player_hit(dmg, st, flags);
}

/* a caster's or thrower's turn: what it starts */
static void foe_begin_cast(IvwFoe *e) {
    e->state = FS_CAST;
    e->t = 0;
}

static void foe_cast_release(IvwFoe *e) {
    int tx = F->px;
    switch (e->kind) {
    case FK_LIDMAW: {
        int n = rng_range(&ig.rng, 2, 3);
        for (int k = 0; k < n; k++)
            add_shot(SH_PELLET, e->x, tx + PX(rng_range(&ig.rng, -14, 14)), 40 + k * 6, 6, 6);
        break;
    }
    case FK_SKINK: add_shot(SH_CLOUD, e->x, e->x, 150, 0, 0); e->alt ^= 1; break;
    case FK_PEDLAR:
        if (e->alt) add_shot(SH_LIGHTNING, tx, tx, 50, 12, 0);
        else add_shot(SH_FIRE, e->x, tx, 36, 10, 10);
        e->alt ^= 1;
        break;
    case FK_PUFF: {
        bool hurt = false;
        for (int i = 0; i < F->nfoe; i++)
            if (F->foe[i].alive && &F->foe[i] != e && F->foe[i].hp < F->foe[i].hpmax) hurt = true;
        if (hurt && e->alt == 0) {
            add_shot(SH_MIST, e->x, e->x, 30, 0, 0);
            for (int i = 0; i < F->nfoe; i++) {
                IvwFoe *o = &F->foe[i];
                if (o->alive && o != e && iabs(o->x - e->x) <= PX(70)) o->hp = (int16_t)imin(o->hpmax, o->hp + 12);
            }
        } else
            for (int k = 0; k < 5; k++)
                add_shot(SH_SPIKE, PX(rng_range(&ig.rng, 20, 156)), tx + PX(rng_range(&ig.rng, -36, 36)), 30 + k * 10, 4, 4);
        e->alt ^= 1;
        break;
    }
    case FK_PUPPET: add_shot(SH_NEEDLE, e->x, tx, 30, 5, 5); break;
    case FK_MUMMER:
        if (e->alt && ivw_fight_foes_alive() < 6) {
            int row = row_count(0) < 2 ? 0 : row_count(1) < 2 ? 1 : -1;
            if (row >= 0) add_foe(FK_PUPPET, row, rng_range(&ig.rng, 30, 146));
            else add_shot(SH_BALL, e->x, tx, 40, 7, 8);
        } else add_shot(SH_BALL, e->x, tx, 40, 7, 8);
        e->alt ^= 1;
        break;
    case FK_GRASPER:
        if (e->alt) add_shot(SH_ROCK, tx, tx, 50, 9, 10);
        else add_shot(SH_BOLT, e->x, tx, 30, 7, 7);
        e->alt ^= 1;
        break;
    case FK_SILVER: add_shot(SH_ACID, e->x, tx, 36, 6, 6); break;
    case FK_WARDEN: add_shot(SH_SHIELD, e->x, tx, 32, 8, 10); break;
    case FK_MATRON: {
        bool eggs = false;
        if (e->alt == 2) {
            for (int k = 0; k < 2; k++) {
                int row = row_count(0) < 2 ? 0 : row_count(1) < 2 ? 1 : -1;
                if (row >= 0 && ivw_fight_foes_alive() < 7) { add_foe(FK_EGG, row, rng_range(&ig.rng, 30, 146)); eggs = true; }
            }
        }
        if (!eggs) {
            if (e->alt == 1) add_shot(SH_CLOUD, tx, tx, 150, 0, 0);
            else add_shot(SH_ACID, e->x, tx, 40, 10, 10);
        }
        e->alt = (uint8_t)((e->alt + 1) % 3);
        break;
    }
    case FK_STONE:
        if (e->alt == 0) add_shot(SH_FIRE, e->x, tx, 36, 9, 9);
        else if (e->alt == 1) {
            for (int i = 0; i < F->nfoe; i++)
                if (F->foe[i].alive && F->foe[i].kind == FK_REGENT)
                    F->foe[i].hp = (int16_t)imin(F->foe[i].hpmax, F->foe[i].hp + 18);
            add_shot(SH_MIST, e->x, e->x, 30, 0, 0);
        } else {
            int guards = 0;
            for (int i = 0; i < F->nfoe; i++) guards += F->foe[i].alive && F->foe[i].kind == FK_GUARD;
            int row = row_count(0) < 2 ? 0 : row_count(1) < 2 ? 1 : -1;
            if (guards < 2 && row >= 0) add_foe(FK_GUARD, row, rng_range(&ig.rng, 24, 152));
            else add_shot(SH_FIRE, e->x, tx, 36, 9, 9);
        }
        break;
    default: break;
    }
}

static void foe_update(IvwFoe *e, int idx) {
    const IvwFoeDef *d = &IVW_FOE[e->kind];
    (void)idx;
    /* statuses */
    if (e->poison > 0) { e->poison--; if (e->poison % 40 == 0) { e->hp--; if (e->hp <= 0) { foe_hurt(e, 1, true, 0); return; } } }
    if (e->burn > 0) { e->burn--; if (e->burn % 20 == 0) { e->hp--; if (e->hp <= 0) { foe_hurt(e, 1, true, 0); return; } } }
    if (e->slow > 0) e->slow--;
    if (e->freeze > 0) { e->freeze--; return; }
    if (e->kind == FK_EGG) {
        if (++e->aux >= 600) {
            e->kind = FK_GRUB;
            e->hp = e->hpmax = IVW_FOE[FK_GRUB].hp;
            e->state = FS_MOVE;
            e->cd = 30;
        }
        return;
    }
    int speed = d->speed;
    if (e->slow) speed /= 2;
    int wind = d->windup * (e->slow ? 3 : 2) / 2;
    if (e->kind == FK_REGENT && F->regent_phase >= 3) wind = wind * 2 / 3;
    /* moving: toward its place, never through a neighbour */
    if (e->state == FS_MOVE || e->state == FS_RECOVER) {
        int tx = foe_target_x(e);
        if (e->kind == FK_GRASPER && e->state == FS_MOVE && rng_chance(&ig.rng, 1)) e->x = (int16_t)PX(rng_range(&ig.rng, 20, 156));
        int nx = e->x;
        if (e->state == FS_MOVE && speed) {
            if (nx < tx) nx = imin(tx, nx + speed);
            else if (nx > tx) nx = imax(tx, nx - speed);
        }
        nx = iclamp(nx, ARENA_MIN, ARENA_MAX);
        bool blocked = false;
        if (e->kind != FK_CLAW && e->kind != FK_STONE && e->kind != FK_MATRON)
            for (int i = 0; i < F->nfoe; i++) {
                IvwFoe *o = &F->foe[i];
                if (o == e || !o->alive || o->row != e->row || o->kind == FK_STONE || o->kind == FK_MATRON) continue;
                if (iabs(o->x - nx) < ROW_GAP && iabs(o->x - nx) < iabs(o->x - e->x)) blocked = true;
            }
        if (!blocked) e->x = (int16_t)nx;
    }
    /* the back row: now and then a foe there trades places with the front */
    if (e->row == 1 && e->kind != FK_STONE && e->kind != FK_MATRON) {
        if (--e->aux <= 0) {
            e->aux = (int16_t)rng_range(&ig.rng, 300, 480);
            if (row_count(0) < 2) e->row = 0;
            else
                for (int i = 0; i < F->nfoe; i++) {
                    IvwFoe *o = &F->foe[i];
                    if (o->alive && o->row == 0 && o->state == FS_MOVE && o->kind != FK_CLAW && o->kind != FK_REGENT &&
                        o->kind != FK_WARDEN) {
                        o->row = 1;
                        o->aux = (int16_t)rng_range(&ig.rng, 300, 480);
                        e->row = 0;
                        F->swaps++;
                        break;
                    }
                }
        }
        if (row_count(0) == 0) e->row = 0;
    }
    /* a stone rises and sinks */
    if (e->kind == FK_STONE) {
        if (--e->aux < -150) e->aux = (int16_t)rng_range(&ig.rng, 280, 420);
    }
    switch (e->state) {
    case FS_MOVE: {
        if (e->cd > 0) { e->cd--; break; }
        bool front = e->row == 0;
        bool near = front && ivw_foe_aligned(e, 4);
        bool silenced = F->silence > 0;
        bool melee = false, cast = false;
        switch (e->kind) {
        case FK_MATRON: cast = true; break;
        case FK_STONE: case FK_PUFF: cast = !silenced; break;
        case FK_PEDLAR: case FK_MUMMER: melee = near; cast = !near && !silenced; break;
        case FK_LIDMAW: melee = near && rng_chance(&ig.rng, 40); cast = !melee; break;
        case FK_GRASPER: case FK_SILVER: melee = near && rng_chance(&ig.rng, 50); cast = !melee; break;
        case FK_PUPPET: melee = near; cast = !near && rng_chance(&ig.rng, 2); break;
        case FK_SKINK:
            if (front && e->alt) cast = true;
            else melee = near;
            break;
        case FK_WARDEN: melee = near; cast = !near && rng_chance(&ig.rng, 3); break;
        case FK_REGENT: melee = near || (F->regent_phase >= 2 && rng_chance(&ig.rng, 1)); break;
        case FK_EGG: break;
        default: melee = near && d->dmg; break;
        }
        if (cast) { foe_begin_cast(e); break; }
        if (!melee) { if (e->kind == FK_STONE || e->kind == FK_PUFF) e->cd = 60; break; }
        e->state = FS_WINDUP;
        e->t = 0;
        if (e->kind == FK_WARDEN) {
            /* below full health, the yellow charge: the lower, the more strikes */
            if (e->hp < e->hpmax && rng_chance(&ig.rng, 35)) e->strikes = (uint8_t)(1 + 3 * (e->hpmax - e->hp) / e->hpmax);
        }
        if (e->kind == FK_REGENT) e->alt = F->regent_phase >= 2 && (!near || rng_chance(&ig.rng, 35)) ? 2 : 0;
        break;
    }
    case FS_WINDUP: {
        int w = wind;
        if (e->kind == FK_COILER && e->alt) w = 60;
        if (e->kind == FK_WARDEN && e->strikes) w = 50;
        if (e->kind == FK_REGENT && e->alt == 2) w = 55;
        if (++e->t >= w) { e->state = FS_ACTIVE; e->t = 0; foe_strike(e); }
        break;
    }
    case FS_ACTIVE:
        if (++e->t >= d->active) {
            if (e->kind == FK_WARDEN && e->strikes > 1) {
                e->strikes--;
                e->state = FS_WINDUP;
                e->t = 50 - 12;
                break;
            }
            e->strikes = 0;
            e->state = FS_RECOVER;
            e->t = 0;
        }
        break;
    case FS_RECOVER:
        if (++e->t >= d->recover) {
            e->state = FS_MOVE;
            e->cd = (int16_t)rng_range(&ig.rng, 40, 110);
            if (e->kind == FK_GRUB) e->cd = 10;
            if (e->kind == FK_REGENT && F->regent_phase >= 3) e->cd = (int16_t)(e->cd * 2 / 3);
        }
        break;
    case FS_STUN:
        if (++e->t >= e->stun) { e->state = FS_MOVE; e->stun = 0; e->cd = (int16_t)rng_range(&ig.rng, 20, 60); }
        break;
    case FS_CAST:
        if (++e->t >= wind + 10) {
            foe_cast_release(e);
            e->state = FS_RECOVER;
            e->t = 0;
        }
        break;
    default: break;
    }
}

/* ---- the knight's own frame ------------------------------------------------------------------------------- */

static void start_swing(bool combo) {
    if (F->exhausted && !playing_beetle()) return;
    F->pstate = PS_SWING;
    F->pt = combo ? swing_of()->windup / 2 : 0;
    F->queued = 0;
    spend_st(swing_cost());
}

static void player_update(void) {
    IvwHero *h = &ig.hero;
    const IvwSwing *sw = swing_of();
    bool l = btn(BTN_LEFT), r = btn(BTN_RIGHT), dn = btn(BTN_DOWN);
    /* stamina comes back, but not while guarding */
    if (F->pstate != PS_GUARD && !playing_beetle()) {
        int regen = 3 + h->stat[STAT_MIGHT] / 3;
        h->st = (int16_t)imin(ivw_max_st(h), h->st + regen);
        if (F->recover_hp > 0 && (F->pstate == PS_SWING || F->pstate == PS_DODGE)) {
            /* the mould cloak: spending stamina wins back lost health */
            int g = imin(F->recover_hp, 1);
            if (F->t % 6 == 0) { h->hp = (int16_t)imin(ivw_max_hp(h), h->hp + g); F->recover_hp = (int16_t)(F->recover_hp - g); }
        }
    }
    if (F->exhausted && h->st >= ivw_max_st(h) / 2) F->exhausted = 0;
    if (btnp(BTN_DOWN)) F->down_t = 0;
    else if (F->down_t < 1000) F->down_t++;
    /* the beetle: B calls it back and the knight steps in */
    if (playing_beetle() && btnp(BTN_B)) {
        F->beetle_out = 1;
        F->pstate = PS_IDLE;
        input_consume();
        ivw_say("THE BEETLE STEPS BACK. YOUR TURN.");
        return;
    }
    switch (F->pstate) {
    case PS_IDLE: case PS_GUARD: {
        /* a roll: DOWN, then quickly LEFT or RIGHT */
        if ((btnp(BTN_LEFT) || btnp(BTN_RIGHT)) && F->down_t <= ROLL_WINDOW && !F->exhausted && dn) {
            F->pstate = PS_DODGE;
            F->pt = 0;
            F->roll_dir = btnp(BTN_LEFT) ? -1 : 1;
            spend_st(PX(ROLL_COST));
            ivw_sfx("ivw_roll");
            break;
        }
        if (btnp(BTN_A)) {
            if (weapon_flags() & WF_CHARGE && !playing_beetle()) { F->pstate = PS_CHARGE; F->pt = 0; F->charge = 0; break; }
            start_swing(false);
            F->combo = 1;
            break;
        }
        if (dn && !F->exhausted) { F->pstate = PS_GUARD; break; }
        F->pstate = PS_IDLE;
        int sp = ivw_player_speed();
        if (l && !r) F->px = (int16_t)imax(ARENA_MIN, F->px - sp);
        if (r && !l) F->px = (int16_t)imin(ARENA_MAX, F->px + sp);
        break;
    }
    case PS_CHARGE:
        F->pt++;
        if (!btn(BTN_A)) {
            if (F->pt >= CHARGE_FRAMES) {
                /* the charged blow: through any guard */
                spend_st(swing_cost() * 3 / 2);
                player_blow(true, 3, 2);
                F->pstate = PS_SWING;
                F->pt = sw->windup + sw->active;
                F->combo = 3;
            } else {
                start_swing(false);
                F->combo = 1;
            }
        }
        break;
    case PS_SWING:
        F->pt++;
        if (btnp(BTN_A) && F->pt > sw->windup && F->combo < 3) F->queued = 1;
        if (F->pt == sw->windup) player_blow(false, F->combo >= 3 ? 3 : 2, 2);
        if (F->pt >= sw->windup + sw->active + sw->recover) {
            if (F->queued && F->combo < 3 && !F->exhausted) { F->combo++; start_swing(true); }
            else { F->pstate = PS_IDLE; F->combo = 0; }
        }
        break;
    case PS_DODGE:
        F->pt++;
        F->px = (int16_t)iclamp(F->px + F->roll_dir * PX(2), ARENA_MIN, ARENA_MAX);
        if (F->pt >= ROLL_FRAMES) F->pstate = PS_IDLE;
        break;
    case PS_STAGGER:
        if (++F->pt >= 0) F->pstate = PS_IDLE;
        break;
    default: F->pstate = PS_IDLE; break;
    }
}

/* a shot reaches its mark */
static void shot_update(IvwShot *s) {
    s->t++;
    if (s->kind == SH_CLOUD) {
        if (iabs(s->x - F->px) <= PX(20) && !playing_beetle()) {
            spend_st(PX(1));
            if (s->t % 30 == 0) ig.hero.poison = (uint8_t)imax(ig.hero.poison, 4);
        }
        if (s->t >= s->t_max) s->alive = 0;
        return;
    }
    if (s->kind == SH_MIST) { if (s->t >= s->t_max) s->alive = 0; return; }
    if (s->t < s->t_max) return;
    s->alive = 0;
    if (s->kind == SH_MYBOLT) {
        /* the knight's bolt: the first foe in its line, flyers and the back row first */
        IvwFoe *best = NULL;
        for (int i = 0; i < F->nfoe; i++) {
            IvwFoe *e = &F->foe[i];
            if (!e->alive || iabs(e->x - s->x) > PX(16)) continue;
            if (!best || e->row > best->row || e->kind == FK_STONE) best = e;
        }
        if (best) foe_hurt(best, s->dmg, true, 10);
        return;
    }
    if (iabs(s->x - F->px) > PX(12)) return;
    int flags = HIT_BLOCKABLE;
    if (s->kind == SH_LIGHTNING) flags = 0;
    if (s->kind == SH_FIRE) flags |= HIT_BURN;
    if (s->kind == SH_ACID) flags |= HIT_POISON;
    player_hit(s->dmg, s->stdmg, flags);
}

/* the bouncing skull, ball or grub */
static void ball_update(void) {
    if (!F->ball_kind) return;
    if (--F->ball_t <= 0) { F->ball_kind = 0; return; }
    F->ball_x = (int16_t)(F->ball_x + F->ball_v);
    if (F->ball_x < ARENA_MIN || F->ball_x > ARENA_MAX) F->ball_v = (int16_t)-F->ball_v;
    if (F->t % 30) return;
    for (int i = 0; i < F->nfoe; i++) {
        IvwFoe *e = &F->foe[i];
        if (!e->alive || e->row != 0 || e->kind == FK_STONE || iabs(e->x - F->ball_x) > PX(14)) continue;
        foe_hurt(e, F->ball_kind == IT_GRUB ? 3 : 2, true, F->ball_kind == IT_GRUB ? 0 : 30);
    }
}

void ivw_fight_update(void) {
    IvwHero *h = &ig.hero;
    if (F->result) return;
    if (ig.cast.on) { ivw_cast_update(); return; }
    F->t++;
    if (F->flash_hurt) F->flash_hurt--;
    /* B starts a spell (the fight waits while the glyph is drawn) */
    if (btnp(BTN_B) && !playing_beetle() && (F->pstate == PS_IDLE || F->pstate == PS_GUARD)) {
        ivw_cast_begin();
        return;
    }
    player_update();
    if (F->result) goto end;
    /* timers */
    if (F->invis > 0) F->invis--;
    if (F->haste > 0) F->haste--;
    if (F->twin > 0) F->twin--;
    if (F->silence > 0) F->silence--;
    if (F->ward > 0 && F->t % 30 == 0) { /* a ward fades */ }
    if (h->poison && F->t % 60 == 0) { if (h->hp > 1) h->hp--; if (F->t % 600 == 0) h->poison--; }
    if (h->burn && F->t % 30 == 0) {
        h->hp--;
        if (F->t % 120 == 0) h->burn--;
        player_dead_check();
        if (F->result) goto end;
    }
    /* the Regent's phases */
    for (int i = 0; i < F->nfoe; i++)
        if (F->foe[i].alive && F->foe[i].kind == FK_REGENT) {
            IvwFoe *e = &F->foe[i];
            int ph = e->hp * 3 <= e->hpmax ? 3 : e->hp * 3 <= e->hpmax * 2 ? 2 : 1;
            if (ph != F->regent_phase) {
                F->regent_phase = (int16_t)ph;
                ivw_say(ph == 2 ? "THE REGENT DRAWS ON THE DARK!" : "THE REGENT HOWLS. HE IS NEARLY SPENT!");
            }
        }
    for (int i = 0; i < F->nfoe; i++) {
        if (!F->foe[i].alive) continue;
        foe_update(&F->foe[i], i);
        if (F->result) goto end;
    }
    for (int i = 0; i < IVW_MAX_SHOTS; i++)
        if (F->shot[i].alive) {
            shot_update(&F->shot[i]);
            if (F->result) goto end;
        }
    ball_update();
    if (ivw_fight_foes_alive() == 0) {
        for (int i = 0; i < F->nfoe; i++) F->foe[i].alive = 0;
        fight_won();
        return;
    }
end:
    if (F->result == 2) {
        F->active = 0;
        ivw_say("YOU FALL...");
        ivw_game_over();
    }
}

/* ---- a spell lands in the fight ------------------------------------------------------------------------------------ */
void ivw_fight_spell(int sp) {
    IvwHero *h = &ig.hero;
    int lore = h->stat[STAT_LORE];
    for (int i = 0; i < F->nfoe; i++) {
        IvwFoe *e = &F->foe[i];
        if (!e->alive) continue;
        const IvwFoeDef *d = &IVW_FOE[e->kind];
        bool line = iabs(e->x - F->px) <= PX(16);
        bool front = e->row == 0 || (e->kind == FK_STONE && e->aux <= 0);
        switch (sp) {
        case SP_LANCE: if (line) foe_hurt(e, 6 + lore / 2, true, 30); break;
        case SP_KINDLE: if (line && front) { foe_hurt(e, 5 + lore / 2, true, 0); e->slow = 300; } break;
        case SP_GALE:
            if (line && front && !(d->flags & FF_BOSS)) { e->alive = 0; e->state = FS_STUN; ivw_say("THE GALE TAKES THE %s.", d->name); }
            break;
        case SP_MIRE: if (front) e->slow = 600; break;
        case SP_RIME:
            if (!(d->flags & FF_NOFREEZE)) e->freeze = (int16_t)(e->kind == FK_COILER ? 540 : 180);
            break;
        case SP_FLARE:
            if (line) { foe_hurt(e, 10 + lore, true, 0); if (!(d->flags & FF_NOBURN)) e->burn = 360; }
            break;
        case SP_THUNDER: foe_hurt(e, 12 + lore, true, 20); break;
        default: break;
        }
    }
    switch (sp) {
    case SP_MEND: h->hp = (int16_t)imin(ivw_max_hp(h), h->hp + 15 + lore); break;
    case SP_RESTORE: h->hp = (int16_t)imin(ivw_max_hp(h), h->hp + 40 + 2 * lore); break;
    case SP_VEIL: F->invis = (int16_t)(600 + lore * 30); break;
    case SP_QUICKEN: F->haste = 900; break;
    case SP_HUSH: F->silence = 900; break;
    case SP_TWIN: F->twin = 900; break;
    case SP_WARD: F->ward = (int16_t)(30 + lore * 2); break;
    default: break;
    }
    if (sp == SP_THUNDER) ivw_sfx("ivw_thunder");
    if (ivw_fight_foes_alive() == 0 && !F->result) {
        for (int i = 0; i < F->nfoe; i++) F->foe[i].alive = 0;
        fight_won();
    }
}
