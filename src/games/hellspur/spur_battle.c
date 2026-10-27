/* HELLSPUR - battles. Side-on, the posse on the left and the foes on the
 * right. Every round each of the posse is given an order (ATTACK, SKILL,
 * ITEM, DEFEND, RUN) from the top down; then everyone acts by speed (ties go
 * to the posse, in posse order). Every plain attack is a timing bar: a
 * marker sweeps across, and A on yellow hits (a foe can still dodge), on red
 * is a critical that can't be dodged, and anywhere else, or no press, is a
 * miss. Shotguns have one big yellow; heavy guns fire while A is held and
 * must be reloaded after; pistols that fire twice or thrice put that many
 * marks on the bar. The hound swings on his own. */
#include "spur.h"

#define B (sg.b)
#define MSG_T 40

int spur_foe_alive(int i) { return i >= 0 && i < B.nfoe && B.foe[i].e > 0 && B.foe[i].hp > 0 && !B.foe[i].fled; }
int spur_foes_left(void) {
    int n = 0;
    for (int i = 0; i < B.nfoe; i++) n += spur_foe_alive(i);
    return n;
}

static void say(const char *a, const char *b) {
    snprintf(B.msg[0], sizeof B.msg[0], "%s", a ? a : "");
    snprintf(B.msg[1], sizeof B.msg[1], "%s", b ? b : "");
    B.msg_t = 0;
    B.phase = BP_MSG;
    B.phase_t = 0;
}

static const SpurEnemyDef *ed(int i) { return &SPUR_ENEMY[B.foe[i].e]; }

/* where foes stand: by size, in columns from the middle out */
static void lay_out(void) {
    int x = 176, y = 14, colw = 0;
    for (int i = 0; i < B.nfoe; i++) {
        int w = spur_enemy_w(B.foe[i].e), h = spur_enemy_h(B.foe[i].e);
        if (y + h > 118) { x += colw + 8; y = 14; colw = 0; }
        B.foe[i].x = (int16_t)x;
        B.foe[i].y = (int16_t)y;
        y += h + 6;
        colw = imax(colw, w);
    }
    /* centre the whole formation in the right-hand side */
    int maxx = 0;
    for (int i = 0; i < B.nfoe; i++) maxx = imax(maxx, B.foe[i].x + spur_enemy_w(B.foe[i].e));
    int shift = (312 - maxx) / 2;
    for (int i = 0; i < B.nfoe; i++) B.foe[i].x = (int16_t)(B.foe[i].x + imax(0, shift));
}

static void add_foe(int e) {
    if (B.nfoe >= SPUR_FOES || e <= 0 || e >= SPUR_ENEMIES) return;
    SpurFoe *f = &B.foe[B.nfoe++];
    memset(f, 0, sizeof *f);
    f->e = (int16_t)e;
    f->hp = f->maxhp = SPUR_ENEMY[e].hp;
}

void spur_battle_begin(int troop, int flag) {
    memset(&B, 0, sizeof B);
    B.troop = troop;
    B.ambush_flag = flag;
    const SpurTroop *t = &SPUR_TROOP[troop];
    if (t->flags & TRF_TWOPOOL) {
        int n = rng_range(&sg.rng, t->min, t->max), m = rng_range(&sg.rng, 0, 2);
        for (int i = 0; i < n; i++) add_foe(t->e[0]);
        for (int i = 0; i < m; i++) add_foe(t->e[1]);
    } else if (t->flags & TRF_RANDOMN) {
        int pool = 0;
        while (pool < SPUR_TROOP_MAX && t->e[pool]) pool++;
        int n = rng_range(&sg.rng, t->min, t->max);
        for (int i = 0; i < n; i++) add_foe(t->e[rng_range(&sg.rng, 0, imax(0, pool - 1))]);
    } else {
        for (int i = 0; i < SPUR_TROOP_MAX && t->e[i]; i++) add_foe(t->e[i]);
    }
    for (int i = 0; i < B.nfoe; i++) if (SPUR_ENEMY[B.foe[i].e].flags & EF_BOSS) B.boss = true;
    B.norun = (t->flags & TRF_NORUN) || B.boss;
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (h >= 0) {
            sg.s.hero[h].reload = 0;
            if (sg.s.hero[h].equip[EQ_NECK] == IT_STEADYEYE) B.buff[i].steady = 1;
        }
    }
    lay_out();
    sg.s.battles++;
    sg.state = GS_BATTLE;
    sg.state_t = 0;
    B.phase = BP_INTRO;
    B.phase_t = 0;
    char a[64];
    if (B.nfoe == 1) snprintf(a, sizeof a, "%s%s!", B.boss ? "" : "A WISP FLARES UP: ", SPUR_ENEMY[B.foe[0].e].name);
    else snprintf(a, sizeof a, "%s%s AND MORE!", B.boss ? "" : "THE WISPS FLARE UP: ", SPUR_ENEMY[B.foe[0].e].name);
    snprintf(B.msg[0], sizeof B.msg[0], "%s", a);
    B.msg[1][0] = 0;
    music_play(B.boss ? SPUR_MUS_BOSS : (troop == 100 ? SPUR_MUS_FINAL : SPUR_MUS_BATTLE));
    if (troop == 100) music_play(SPUR_MUS_FINAL);
    spur_sfx("spur_encounter");
    input_consume();
}

/* ---- who can be given orders -------------------------------------------------------- */
static bool controllable(int slot) {
    int h = spur_hero_at(slot);
    if (!spur_alive(h)) return false;
    if (h == H_BISCUIT) return false;
    if (sg.s.hero[h].status & SS_PARALYZED) return false;
    if (B.buff[slot].rage) return false;
    return true;
}

static int next_commander(int from) {
    for (int i = from; i < 4; i++) if (controllable(i)) return i;
    return -1;
}

static int first_foe(void) {
    for (int i = 0; i < B.nfoe; i++) if (spur_foe_alive(i)) return i;
    return -1;
}

static void build_queue(void);
static void start_round(void) {
    B.turn++;
    for (int i = 0; i < 4; i++) {
        B.order[i] = (SpurOrder){CMD_ATTACK, 0, 0, 0, false};
        B.buff[i].defend = 0;
    }
    for (int i = 0; i < B.nfoe; i++) B.foe[i].guard = 0;
    B.cur = next_commander(0);
    B.sel = 0;
    B.phase = BP_COMMAND;
    B.phase_t = 0;
    if (B.cur < 0) {
        /* nobody to order (the hound alone, or all raging): straight into the round */
        build_queue();
        B.phase = BP_ACT;
    }
}

/* ---- the turn queue ----------------------------------------------------------------------- */
static int hero_spd(int slot) {
    int h = spur_hero_at(slot);
    int s = spur_stat(h, ST_SPD);
    return s + s * 15 * B.buff[slot].spd / 100;
}

static void build_queue(void) {
    B.nqueue = 0;
    /* DEFEND holds from the moment it is ordered, so it covers the whole round */
    for (int i = 0; i < 4; i++)
        if (B.order[i].cmd == CMD_DEFEND && spur_alive(spur_hero_at(i))) B.buff[i].defend = 1;
    for (int i = 0; i < 4; i++) if (spur_alive(spur_hero_at(i))) B.queue[B.nqueue++] = i;
    for (int i = 0; i < B.nfoe; i++) if (spur_foe_alive(i)) B.queue[B.nqueue++] = 10 + i;
    /* by speed; the posse wins ties, in posse order */
    for (int a = 0; a < B.nqueue; a++)
        for (int b = a + 1; b < B.nqueue; b++) {
            int qa = B.queue[a], qb = B.queue[b];
            int sa = qa < 10 ? hero_spd(qa) : SPUR_ENEMY[B.foe[qa - 10].e].spd;
            int sb = qb < 10 ? hero_spd(qb) : SPUR_ENEMY[B.foe[qb - 10].e].spd;
            bool swap = sb > sa || (sb == sa && qb < qa);
            if (swap) { B.queue[a] = qb; B.queue[b] = qa; }
        }
    B.qi = 0;
}

/* ---- damage ---------------------------------------------------------------------------- */
static int var(int v) { return v * rng_range(&sg.rng, 90, 110) / 100; }

static void hero_falls(int h) {
    sg.s.hero[h].hp = 0;
    sg.s.hero[h].status = 0;
    sg.s.deaths++;
    spur_sfx("spur_fall");
}

static void foe_hit(int i, int dmg);
/* tests: a foe struck down where it stands */
void spur_foe_kill(int i) {
    if (i >= 0 && i < B.nfoe && spur_foe_alive(i)) foe_hit(i, B.foe[i].hp);
}

/* returns the damage done (after Second Wind) */
static int hurt_hero(int slot, int dmg, bool *saved) {
    int h = spur_hero_at(slot);
    if (h < 0 || !spur_alive(h)) return 0;
    if (sg.god) dmg = 0;
    if (saved) *saved = false;
    SpurHero *hr = &sg.s.hero[h];
    if (dmg >= hr->hp) {
        int sw = spur_passive(h, PS_SURVIVOR);
        if (sw && rng_range(&sg.rng, 0, 99) < 15 * sw) {
            dmg = hr->hp - 1;
            if (saved) *saved = true;
        }
    }
    hr->hp = (int16_t)(hr->hp - dmg);
    if (hr->hp <= 0) hero_falls(h);
    return dmg;
}

static void foe_hit(int i, int dmg) {
    SpurFoe *f = &B.foe[i];
    f->hp -= dmg;
    f->flash = 8;
    if (f->stun) f->stun = 0; /* a blow wakes it */
    if (f->hp <= 0) {
        f->hp = 0;
        f->dying = 1;
        spur_sfx("spur_kill");
        /* the last fight: when Zenith falls, Nadir goes with him */
        if (B.troop == 100 && f->e == 100) {
            if (!B.final_kill) B.final_kill = 1;
            for (int k = 0; k < B.nfoe; k++)
                if (k != i && B.foe[k].hp > 0) { B.foe[k].hp = 0; B.foe[k].dying = 1; }
        }
        if (B.troop == 100 && f->e == 99 && !B.final_kill) B.final_kill = 2;
        /* the siege: when the captain falls, his troopers run */
        if (B.troop == 90 && f->e == 87)
            for (int k = 0; k < B.nfoe; k++)
                if (k != i && B.foe[k].hp > 0) B.foe[k].fled = 1;
    }
}

/* a plain hit on a foe: the zone's result, the swing's power */
static int strike(int slot, int foe, int result, int power, bool dog) {
    if (!spur_foe_alive(foe)) return -1;
    const SpurEnemyDef *e = ed(foe);
    if (result == ZR_MISS) return -1;
    if (result == ZR_HIT) {
        int evd = e->evd;
        if (B.foe[foe].stun || B.foe[foe].tied) evd = 0;
        if (rng_range(&sg.rng, 0, 99) < evd) return -2; /* dodged */
    }
    int p = power + power * 15 * B.buff[slot].atk / 100;
    int dmg;
    if (result == ZR_CRIT) dmg = dog ? var(p * 2) : var(p * 2) - e->def / 2;
    else dmg = var(p) - e->def / 2;
    if (B.foe[foe].guard) dmg /= 2;
    dmg = imax(1, dmg);
    foe_hit(foe, dmg);
    return dmg;
}

/* the foe a swing aimed at is gone: pick another */
static int retarget(int foe) {
    if (spur_foe_alive(foe)) return foe;
    return first_foe();
}

/* ---- the timing bar ------------------------------------------------------------------------ */
int spur_bar_zone_kind(int h) { return spur_weapon_class(h); }

static int weapon_hits(int it) { return it ? imax(1, SPUR_ITEM[it].hits) : 1; }

static void bar_setup(int slot) {
    int h = spur_hero_at(slot);
    const SpurHero *hr = &sg.s.hero[h];
    int r = hr->equip[EQ_RHAND], l = hr->equip[EQ_LHAND];
    int wc = r ? SPUR_ITEM[r].wclass : WC_MELEE;
    B.bar_kind = wc;
    B.nz = 0;
    int W = BAR_W * 16;
    int redw = W * 5 / 100;
    if (B.buff[slot].steady) redw = W * 12 / 100;
    redw += B.buff[slot].crit * W * 3 / 100;
    if (wc == WC_SHOTGUN) {
        B.z[B.nz++] = (SpurZoneBar){(int16_t)(W * 30 / 100), (int16_t)(W * 88 / 100), (int16_t)(W * 88 / 100), ZR_PENDING};
    } else if (wc == WC_HEAVY) {
        B.z[B.nz++] = (SpurZoneBar){(int16_t)(W * 12 / 100), (int16_t)(W * 92 / 100), (int16_t)(W * 92 / 100), ZR_PENDING};
    } else {
        int hits = weapon_hits(r) + (l ? weapon_hits(l) : 0);
        if (wc == WC_HEAVY) hits = 1;
        hits = imin(hits, BAR_MAXZ);
        int seg = (W * 92 / 100) / hits;
        for (int k = 0; k < hits; k++) {
            int end = W * 8 / 100 + seg * (k + 1);
            int yel = seg * 55 / 100;
            if (hits >= 4) yel = seg * 60 / 100;
            int rw = imin(redw, seg / 3);
            int a = end - yel, red = end - rw;
            /* a bow in any hands but Sage's is all red, no yellow */
            if (wc == WC_BOW && h != H_SAGE) a = red;
            B.z[B.nz++] = (SpurZoneBar){(int16_t)a, (int16_t)red, (int16_t)end, ZR_PENDING};
        }
    }
    B.bar_x = 0;
    B.bar_speed = W / 44;
    if (B.nz > 2) B.bar_speed = W / (44 + B.nz * 6);
    B.bar_heavy_t = 0;
    B.bar_shots = 0;
    B.bar_held = 0;
    B.bar_done_t = 0;
    /* the hound and the raging swing on their own */
    B.bar_auto = (h == H_BISCUIT || B.buff[slot].rage) ? 1 : 0;
    B.phase = BP_BAR;
    B.phase_t = 0;
}

static void zone_result(int k, int res) {
    if (B.z[k].result != ZR_PENDING) return;
    B.z[k].result = (uint8_t)res;
    spur_sfx(res == ZR_CRIT ? "spur_crit" : res == ZR_HIT ? "spur_hit" : "spur_miss");
}

/* where an automatic swing lands for a zone */
static int auto_roll(int slot) {
    int h = spur_hero_at(slot);
    int roll = rng_range(&sg.rng, 0, 99);
    bool steady = B.buff[slot].steady != 0;
    if (h == H_BISCUIT) {
        if (steady) return roll < 45 ? ZR_CRIT : ZR_HIT; /* with a steady eye he never misses the bar */
        return roll < 35 ? ZR_CRIT : roll < 65 ? ZR_HIT : ZR_MISS;
    }
    return roll < 50 ? ZR_CRIT : ZR_HIT;
}

static void finish_attack(int slot);

static void bar_update(int slot) {
    int W = BAR_W * 16;
    int h = spur_hero_at(slot);
    int spd = B.bar_speed * (btn(BTN_RIGHT) && !B.bar_auto ? 2 : 1);
    if (B.bar_done_t) {
        if (++B.bar_done_t > 14) finish_attack(slot);
        return;
    }
    B.bar_x += spd;
    B.bar_delta = spd;
    if (B.bar_kind == WC_HEAVY && !B.bar_auto) {
        /* hold A while the marker is in the band: a shot every six frames, eight at most */
        SpurZoneBar *z = &B.z[0];
        bool in = B.bar_x >= z->a && B.bar_x <= z->b;
        if (btn(SPUR_OK) && in && B.bar_shots < 8) {
            if (B.bar_heavy_t++ % 6 == 0) {
                B.bar_shots++;
                spur_sfx("spur_heavy");
            }
        }
        if (B.bar_x > z->b || B.bar_shots >= 8) { z->result = B.bar_shots ? ZR_HIT : ZR_MISS; B.bar_done_t = 1; }
        return;
    }
    if (B.bar_auto) {
        if (B.bar_kind == WC_HEAVY) { B.bar_shots = 8; B.z[0].result = ZR_HIT; B.bar_done_t = 1; return; }
        for (int k = 0; k < B.nz; k++)
            if (B.z[k].result == ZR_PENDING && B.bar_x >= B.z[k].b) zone_result(k, auto_roll(slot));
    } else {
        if (btnp(SPUR_OK)) {
            int hit = -1;
            for (int k = 0; k < B.nz && hit < 0; k++)
                if (B.z[k].result == ZR_PENDING && B.bar_x >= B.z[k].a && B.bar_x <= B.z[k].b) hit = k;
            if (hit >= 0) zone_result(hit, B.bar_x >= B.z[hit].red ? ZR_CRIT : ZR_HIT);
            else {
                /* pressed in the gap: the next mark is missed */
                for (int k = 0; k < B.nz; k++)
                    if (B.z[k].result == ZR_PENDING) { zone_result(k, ZR_MISS); break; }
            }
        }
        for (int k = 0; k < B.nz; k++)
            if (B.z[k].result == ZR_PENDING && B.bar_x > B.z[k].b) zone_result(k, ZR_MISS);
    }
    bool all = true;
    for (int k = 0; k < B.nz; k++) all &= B.z[k].result != ZR_PENDING;
    if (all || B.bar_x >= W) {
        for (int k = 0; k < B.nz; k++) if (B.z[k].result == ZR_PENDING) B.z[k].result = ZR_MISS;
        B.bar_done_t = 1;
    }
    (void)h;
}

static void finish_attack(int slot) {
    int h = spur_hero_at(slot);
    SpurHero *hr = &sg.s.hero[h];
    int target = retarget(B.order[slot].target);
    char a[64], b2[64];
    a[0] = b2[0] = 0;
    int total = 0, hits = 0, crits = 0, dodges = 0;
    bool dog = h == H_BISCUIT;
    if (B.bar_kind == WC_HEAVY) {
        int p = spur_attack_power(h) / 2;
        for (int s = 0; s < B.bar_shots; s++) {
            int f = first_foe();
            int nl = 0, live[SPUR_FOES];
            for (int i = 0; i < B.nfoe; i++) if (spur_foe_alive(i)) live[nl++] = i;
            if (!nl) break;
            f = live[rng_range(&sg.rng, 0, nl - 1)];
            int d = strike(slot, f, ZR_HIT, p, false);
            if (d > 0) { total += d; hits++; } else if (d == -2) dodges++;
        }
        hr->reload = 1;
        snprintf(a, sizeof a, "%s LETS RIP: %d SHOTS.", SPUR_HERO[h].name, B.bar_shots);
        if (hits) snprintf(b2, sizeof b2, "%d DAMAGE IN ALL.", total);
        else snprintf(b2, sizeof b2, "NOTHING FINDS A MARK.");
    } else if (B.bar_kind == WC_SHOTGUN) {
        int p = spur_attack_power(h);
        int res = B.z[0].result;
        int d = target >= 0 ? strike(slot, target, res, p, false) : -1;
        if (d > 0) {
            total += d;
            hits++;
            /* the spread: the others may catch some too, the big ones more often */
            for (int i = 0; i < B.nfoe; i++) {
                if (i == target || !spur_foe_alive(i)) continue;
                int chance = 45 + SPUR_ENEMY[B.foe[i].e].size * 20 - SPUR_ENEMY[B.foe[i].e].evd;
                if (rng_range(&sg.rng, 0, 99) < chance) {
                    int d2 = imax(1, var(p / 2) - SPUR_ENEMY[B.foe[i].e].def / 2);
                    foe_hit(i, d2);
                    total += d2;
                }
            }
            snprintf(a, sizeof a, "%s FIRES THE SCATTERGUN.", SPUR_HERO[h].name);
            snprintf(b2, sizeof b2, "%d DAMAGE.", total);
        } else {
            snprintf(a, sizeof a, "%s FIRES THE SCATTERGUN.", SPUR_HERO[h].name);
            snprintf(b2, sizeof b2, "%s", d == -2 ? "THE SHOT GOES WIDE." : "A MISS.");
        }
    } else {
        int r = hr->equip[EQ_RHAND], l = hr->equip[EQ_LHAND];
        int nr = weapon_hits(r);
        for (int k = 0; k < B.nz; k++) {
            int tgt = retarget(target);
            if (tgt < 0) break;
            target = tgt;
            int p = spur_attack_power(h);
            if (k >= nr && l) {
                /* the left hand's weapon */
                int keep = hr->equip[EQ_RHAND];
                hr->equip[EQ_RHAND] = (uint8_t)l;
                p = spur_attack_power(h);
                hr->equip[EQ_RHAND] = (uint8_t)keep;
            }
            int d = strike(slot, target, B.z[k].result, p, dog);
            if (d > 0) { total += d; hits++; crits += B.z[k].result == ZR_CRIT; }
            else if (d == -2) dodges++;
        }
        if (r && (SPUR_ITEM[r].flags & IF_RELOAD)) hr->reload = 1;
        const char *verb = dog ? "BITES" : r ? (SPUR_ITEM[r].wclass == WC_MELEE ? "SLASHES" : SPUR_ITEM[r].wclass == WC_BOW ? "LOOSES" : "FIRES") : "SWINGS";
        snprintf(a, sizeof a, "%s %s%s", SPUR_HERO[h].name, verb, crits ? " - A CRITICAL!" : ".");
        if (hits) snprintf(b2, sizeof b2, "%d DAMAGE%s.", total, B.nz > 1 ? (hits == B.nz ? ", EVERY ONE" : "") : "");
        else snprintf(b2, sizeof b2, "%s", dodges ? "THE FOE DODGES." : "A MISS.");
    }
    say(a, b2);
}

/* ---- skills ----------------------------------------------------------------------------- */
static int spirit(int h, int v) { return v * (2 + spur_passive(h, PS_SPIRIT)) / 2; }

/* damage a skill does that slips past armour */
static int skill_power(int h, int sk) {
    int base = SPUR_SKILL[sk].power + spur_stat(h, ST_RES) + spur_stat(h, ST_ATK) / 2;
    return var(spirit(h, base));
}

static int observe_cap(void) { return SPUR_QUILL_MAX - 1; }

static void do_skill(int slot) {
    int h = spur_hero_at(slot);
    SpurHero *hr = &sg.s.hero[h];
    int sk = B.order[slot].skill;
    const SpurSkillDef *d = &SPUR_SKILL[sk];
    char a[64], b2[64];
    snprintf(a, sizeof a, "%s: %s.", SPUR_HERO[h].name, d->name);
    b2[0] = 0;
    if (hr->sp < d->sp) { say(a, "NOT ENOUGH SOUL."); return; }
    hr->sp = (int16_t)(hr->sp - d->sp);
    int tf = retarget(B.order[slot].target);
    int ta = spur_hero_at(B.order[slot].target);
    spur_sfx("spur_skill");
    switch (sk) {
    case SK_REEL: case SK_MINUET: case SK_RENEW: {
        int amt = spur_skill_heal(h, sk);
        for (int i = 0; i < 4; i++) {
            int t = spur_hero_at(i);
            if (!spur_alive(t)) continue;
            sg.s.hero[t].hp = (int16_t)imin(spur_maxhp(t), sg.s.hero[t].hp + amt);
            if (sk == SK_RENEW) sg.s.hero[t].status &= (uint8_t)~SS_PARALYZED;
        }
        snprintf(b2, sizeof b2, "THE POSSE HEALS %d.", amt);
        break;
    }
    case SK_SHIMMY: case SK_CANCAN: case SK_TROT:
        for (int i = 0; i < 4; i++) {
            if (!spur_alive(spur_hero_at(i))) continue;
            if (sk == SK_SHIMMY) B.buff[i].evd++;
            if (sk == SK_CANCAN) B.buff[i].atk++;
            if (sk == SK_TROT) B.buff[i].spd++;
        }
        snprintf(b2, sizeof b2, "THE WHOLE POSSE %s.", sk == SK_SHIMMY ? "GETS LIGHT ON ITS FEET" : sk == SK_CANCAN ? "FIRES UP" : "PICKS UP THE PACE");
        break;
    case SK_LIELOW: B.buff[slot].evd += 2; B.buff[slot].lielow = 1; snprintf(b2, sizeof b2, "HARD TO SEE NOW."); break;
    case SK_BRACE: B.buff[slot].def += 2; snprintf(b2, sizeof b2, "SET LIKE A FENCEPOST."); break;
    case SK_STEADY: B.buff[slot].steady = 1; snprintf(b2, sizeof b2, "THE RED OF THE BAR GROWS."); break;
    case SK_COVER: B.buff[slot].covering = 1; snprintf(b2, sizeof b2, "STANDS IN FRONT OF THE OTHERS."); break;
    case SK_RAGE: B.buff[slot].rage = 3; B.buff[slot].atk += 1; snprintf(b2, sizeof b2, "SEES RED."); break;
    case SK_SPLINT: case SK_HOWL: case SK_FLUTTER: case SK_KEENEYE: {
        int t = B.order[slot].target;
        if (!spur_alive(spur_hero_at(t))) { snprintf(b2, sizeof b2, "TOO LATE."); break; }
        if (sk == SK_SPLINT) B.buff[t].def++;
        if (sk == SK_HOWL) B.buff[t].atk++;
        if (sk == SK_FLUTTER) B.buff[t].evd++;
        if (sk == SK_KEENEYE) { B.buff[t].crit++; B.buff[t].steady = 1; }
        snprintf(b2, sizeof b2, "%s %s.", SPUR_HERO[spur_hero_at(t)].name,
                 sk == SK_SPLINT ? "IS BRACED" : sk == SK_HOWL ? "HITS HARDER" : sk == SK_FLUTTER ? "DODGES BETTER" : "SEES THE RED");
        break;
    }
    case SK_MEND: case SK_SUTURE: {
        if (!spur_alive(ta)) { snprintf(b2, sizeof b2, "TOO LATE FOR THAT HERE."); break; }
        int amt = spur_skill_heal(h, sk);
        sg.s.hero[ta].hp = (int16_t)imin(spur_maxhp(ta), sg.s.hero[ta].hp + amt);
        snprintf(b2, sizeof b2, "%s HEALS %d.", SPUR_HERO[ta].name, amt);
        break;
    }
    case SK_PURGE:
        if (ta >= 0) sg.s.hero[ta].status &= (uint8_t)~SS_POISON;
        snprintf(b2, sizeof b2, "THE POISON IS DRAWN OUT.");
        break;
    case SK_RAISE:
        if (ta < 0 || spur_alive(ta)) { snprintf(b2, sizeof b2, "NOTHING TO RAISE."); break; }
        sg.s.hero[ta].hp = (int16_t)(spur_maxhp(ta) / 4);
        snprintf(b2, sizeof b2, "%s STANDS BACK UP.", SPUR_HERO[ta].name);
        spur_sfx("spur_raise");
        break;
    case SK_LIFT: {
        if (tf < 0) break;
        SpurFoe *f = &B.foe[tf];
        const SpurEnemyDef *e = ed(tf);
        if (f->lifted) { snprintf(b2, sizeof b2, "ITS POCKETS ARE EMPTY NOW."); break; }
        int roll = rng_range(&sg.rng, 0, 99);
        if (roll < 12 && e->lift_rare > 0) {
            f->lifted = 1;
            if (spur_bag_add(e->lift_rare) >= 0) snprintf(b2, sizeof b2, "LIFTS A %s!", SPUR_ITEM[e->lift_rare].name);
            else snprintf(b2, sizeof b2, "THE BAGS ARE FULL.");
        } else if (roll < 45 && e->lift_common > 0) {
            f->lifted = 1;
            if (spur_bag_add(e->lift_common) >= 0) snprintf(b2, sizeof b2, "LIFTS A %s.", SPUR_ITEM[e->lift_common].name);
            else snprintf(b2, sizeof b2, "THE BAGS ARE FULL.");
        } else if (roll < 80) {
            f->lifted = 1;
            int t = e->teeth / 2 + 5;
            sg.s.teeth += t;
            snprintf(b2, sizeof b2, "LIFTS %d BITS.", t);
        } else snprintf(b2, sizeof b2, "COMES UP EMPTY.");
        break;
    }
    case SK_PAYOFF: {
        if (tf < 0) break;
        const SpurEnemyDef *e = ed(tf);
        int cost = (int)((e->xp + e->teeth) / 2 + B.foe[tf].hp / 2 + 10);
        if (e->flags & EF_NOBRIBE) { snprintf(b2, sizeof b2, "%s WON'T TAKE A BRIBE.", e->name); break; }
        if (sg.s.teeth < cost) { snprintf(b2, sizeof b2, "IT WANTS %d BITS. YOU'RE SHORT.", cost); break; }
        if (rng_range(&sg.rng, 0, 99) < (e->flags & EF_BOSS ? 50 : 75)) {
            sg.s.teeth -= cost;
            B.foe[tf].fled = 1;
            B.foe[tf].hp = 0;
            snprintf(b2, sizeof b2, "%s TAKES %d BITS AND LEAVES.", e->name, cost);
            if (B.troop == 100 && B.foe[tf].e == 99) B.final_kill = 2; /* Nadir paid off */
        } else snprintf(b2, sizeof b2, "%s SPITS ON THE MONEY.", e->name);
        break;
    }
    case SK_OBSERVE: {
        if (tf < 0) break;
        const SpurEnemyDef *e = ed(tf);
        int learn = e->observe;
        if (!learn || spur_has_skill(h, learn)) { snprintf(b2, sizeof b2, "NOTHING NEW TO SEE THERE."); break; }
        if (hr->observed >= observe_cap()) { snprintf(b2, sizeof b2, "HIS NOTEBOOK IS FULL."); break; }
        int chance = B.foe[tf].stun || B.foe[tf].tied ? 100 : 45;
        if (rng_range(&sg.rng, 0, 99) < chance) {
            spur_learn_skill(h, learn);
            hr->observed++;
            snprintf(b2, sizeof b2, "PICKS UP %s!", SPUR_SKILL[learn].name);
            spur_sfx("spur_learn");
        } else snprintf(b2, sizeof b2, "LOSES THE THREAD.");
        break;
    }
    case SK_DEADSHOT: {
        if (tf < 0) break;
        const SpurEnemyDef *e = ed(tf);
        int chance = (e->flags & EF_BOSS) ? 3 : 35;
        if (rng_range(&sg.rng, 0, 99) < chance) { foe_hit(tf, B.foe[tf].hp); snprintf(b2, sizeof b2, "%s DROPS DEAD.", e->name); }
        else snprintf(b2, sizeof b2, "IT SHRUGS IT OFF.");
        break;
    }
    case SK_UPPERCUT: case SK_SNARE: case SK_SNARL: {
        if (tf < 0) break;
        const SpurEnemyDef *e = ed(tf);
        bool boss = (e->flags & EF_BOSS) != 0;
        if (sk == SK_SNARL) { B.foe[tf].weak++; snprintf(b2, sizeof b2, "%s LOSES ITS NERVE.", e->name); break; }
        if (sk == SK_UPPERCUT) foe_hit(tf, imax(1, skill_power(h, sk) / 3 - e->def / 4));
        if ((e->flags & EF_NOSTUN) || rng_range(&sg.rng, 0, 99) >= (boss ? 30 : 65)) { snprintf(b2, sizeof b2, "%s SHAKES IT OFF.", e->name); break; }
        if (sk == SK_UPPERCUT) B.foe[tf].stun = 1;
        else B.foe[tf].tied = 3;
        snprintf(b2, sizeof b2, "%s IS %s.", e->name, sk == SK_UPPERCUT ? "SEEING STARS" : "TIED UP");
        break;
    }
    case SK_SIPHON: case SK_JAB: case SK_BITE: case SK_BUCK: case SK_MAUL: {
        if (tf < 0) break;
        int dmg = skill_power(h, sk);
        if (sk == SK_BUCK) dmg = imax(1, dmg - ed(tf)->def / 2); /* a plain kick: armour counts */
        foe_hit(tf, dmg);
        if (sk == SK_SIPHON) hr->hp = (int16_t)imin(spur_maxhp(h), hr->hp + dmg / 2);
        snprintf(b2, sizeof b2, "%d DAMAGE.", dmg);
        break;
    }
    case SK_DUSTSTORM: case SK_SWOOP: case SK_HAYMAKER: {
        int total = 0;
        for (int i = 0; i < B.nfoe; i++) {
            if (!spur_foe_alive(i)) continue;
            int dmg = skill_power(h, sk) * (sk == SK_HAYMAKER ? 2 : 1) / (sk == SK_HAYMAKER ? 3 : 1);
            if (sk == SK_HAYMAKER) dmg = imax(1, (spur_attack_power(h) * 3 / 4 + SPUR_SKILL[sk].power / 4) - ed(i)->def / 2);
            if (sk == SK_SWOOP) dmg = imax(1, dmg - ed(i)->def / 2);
            foe_hit(i, dmg);
            total += dmg;
        }
        snprintf(b2, sizeof b2, "%d DAMAGE ACROSS THE FOES.", total);
        break;
    }
    default: snprintf(b2, sizeof b2, "NOTHING HAPPENS."); break;
    }
    say(a, b2);
}

static void do_item(int slot) {
    int h = spur_hero_at(slot);
    int bs = B.order[slot].item_slot;
    int it = sg.s.bag[bs];
    char a[64], b2[64];
    b2[0] = 0;
    if (!it) { snprintf(a, sizeof a, "%s REACHES FOR NOTHING.", SPUR_HERO[h].name); say(a, ""); return; }
    const SpurItemDef *d = &SPUR_ITEM[it];
    snprintf(a, sizeof a, "%s USES %s.", SPUR_HERO[h].name, d->name);
    int ta = spur_hero_at(B.order[slot].target);
    switch (d->use) {
    case IU_HEAL:
        if (!spur_alive(ta)) { snprintf(b2, sizeof b2, "TOO LATE FOR THAT."); say(a, b2); return; }
        sg.s.hero[ta].hp = (int16_t)imin(spur_maxhp(ta), sg.s.hero[ta].hp + spur_heal_amount(it));
        snprintf(b2, sizeof b2, "%s HEALS %d.", SPUR_HERO[ta].name, spur_heal_amount(it));
        break;
    case IU_SP: case IU_SPALL:
        if (!spur_alive(ta)) { say(a, "TOO LATE FOR THAT."); return; }
        sg.s.hero[ta].sp = (int16_t)imin(spur_maxsp(ta), sg.s.hero[ta].sp + d->power);
        snprintf(b2, sizeof b2, "%s'S SOUL STIRS.", SPUR_HERO[ta].name);
        break;
    case IU_REVIVE:
        if (ta < 0 || spur_alive(ta)) { say(a, "NOTHING TO REVIVE."); return; }
        sg.s.hero[ta].hp = (int16_t)(spur_doc_level() >= 4 ? spur_maxhp(ta) / 2 : 1);
        snprintf(b2, sizeof b2, "%s COMES BACK.", SPUR_HERO[ta].name);
        break;
    case IU_CURE:
        if (ta >= 0) sg.s.hero[ta].status &= (uint8_t)~SS_POISON;
        snprintf(b2, sizeof b2, "THE POISON IS GONE.");
        break;
    case IU_STUN: {
        int tf = retarget(B.order[slot].target);
        if (tf < 0) break;
        bool boss = (ed(tf)->flags & EF_BOSS) != 0;
        if (!(ed(tf)->flags & EF_NOSTUN) && rng_range(&sg.rng, 0, 99) < (boss ? 40 : 75)) {
            B.foe[tf].stun = 1;
            snprintf(b2, sizeof b2, "%s IS ROPED!", ed(tf)->name);
        } else snprintf(b2, sizeof b2, "THE LOOP FALLS SHORT.");
        break;
    }
    default: snprintf(b2, sizeof b2, "IT DOES NOTHING HERE."); say(a, b2); return;
    }
    /* the doctor's half */
    if (d->half > 0 && spur_doc_level() >= d->half_doc) sg.s.bag[bs] = (uint8_t)d->half;
    else spur_bag_remove(bs);
    /* later orders that pointed past this slot shift down */
    for (int i = 0; i < 4; i++)
        if (B.order[i].cmd == CMD_ITEM && !B.order[i].done && i != slot && B.order[i].item_slot > bs && sg.s.bag[bs] != (uint8_t)d->half)
            B.order[i].item_slot--;
    spur_sfx("spur_heal");
    say(a, b2);
}

static void do_run(int slot) {
    int h = spur_hero_at(slot);
    char a[64];
    snprintf(a, sizeof a, "%s TRIES TO RUN...", SPUR_HERO[h].name);
    if (B.norun) { say(a, "THERE'S NO RUNNING FROM THIS ONE."); return; }
    int ps = 0, pn = 0, fs = 0, fn = 0;
    for (int i = 0; i < 4; i++) if (spur_alive(spur_hero_at(i))) { ps += hero_spd(i); pn++; }
    for (int i = 0; i < B.nfoe; i++) if (spur_foe_alive(i)) { fs += ed(i)->spd; fn++; }
    int chance = iclamp(50 + (ps / imax(1, pn) - fs / imax(1, fn)) * 3 + B.run_tries * 10, 15, 90);
    B.run_tries++;
    if (rng_range(&sg.rng, 0, 99) < chance) {
        B.fled = true;
        say(a, "THE POSSE GETS AWAY!");
        spur_sfx("spur_run");
    } else say(a, "THE WAY IS BLOCKED.");
}

/* ---- the foes act --------------------------------------------------------------------------- */
static int pick_target(void) {
    static const int W[4] = {40, 30, 18, 12};
    int sum = 0;
    for (int i = 0; i < 4; i++) if (spur_alive(spur_hero_at(i))) sum += W[i];
    if (!sum) return -1;
    int r = rng_range(&sg.rng, 0, sum - 1);
    for (int i = 0; i < 4; i++) {
        if (!spur_alive(spur_hero_at(i))) continue;
        if (r < W[i]) return i;
        r -= W[i];
    }
    return 0;
}

static int hero_def(int slot) {
    int h = spur_hero_at(slot);
    return spur_stat(h, ST_DEF);
}

/* one blow from a foe to a posse member: damage after dodges, defending and bracing */
static int blow(int foe, int slot, int pct, bool pierce, char *out, int n) {
    int h = spur_hero_at(slot);
    const SpurEnemyDef *e = ed(foe);
    int evd = spur_stat(h, ST_EVD) + B.buff[slot].evd * 10;
    if (rng_range(&sg.rng, 0, 99) < imin(evd, 75)) { snprintf(out, (size_t)n, "%s DODGES.", SPUR_HERO[h].name); return 0; }
    int atk = e->atk - e->atk * 15 * B.foe[foe].weak / 100;
    atk = atk + atk * 15 * B.foe[foe].buff_atk / 100;
    int dmg = var(atk) * pct / 100;
    if (!pierce) dmg -= hero_def(slot) / 2;
    dmg = imax(imax(1, atk / 8), dmg);
    if (B.buff[slot].defend) dmg /= 2;
    dmg -= dmg * imin(60, 12 * B.buff[slot].def) / 100;
    dmg = imax(1, dmg);
    bool saved = false;
    int done = hurt_hero(slot, dmg, &saved);
    if (!spur_alive(h)) snprintf(out, (size_t)n, "%s TAKES %d. %s FALLS!", SPUR_HERO[h].name, done, SPUR_HERO[h].name);
    else if (saved) snprintf(out, (size_t)n, "%s TAKES %d BUT WON'T GO DOWN!", SPUR_HERO[h].name, done);
    else snprintf(out, (size_t)n, "%s TAKES %d.", SPUR_HERO[h].name, done);
    spur_sfx(done ? "spur_ouch" : "spur_miss");
    B.shake = 6;
    return done;
}

/* who really takes a single blow: a coverer, or the hound leaping in */
static int guard_target(int slot, int foe) {
    for (int i = 0; i < 4; i++)
        if (i != slot && B.buff[i].covering && spur_alive(spur_hero_at(i))) return i;
    int h = spur_hero_at(slot);
    for (int i = 0; i < 4; i++) {
        if (i == slot || spur_hero_at(i) != H_BISCUIT || !spur_alive(H_BISCUIT)) continue;
        /* the hound jumps in front of the badly hurt */
        if (sg.s.hero[h].hp * 4 < spur_maxhp(h) && rng_range(&sg.rng, 0, 99) < 50) return i;
    }
    (void)foe;
    return slot;
}

static void foe_act(int foe) {
    SpurFoe *f = &B.foe[foe];
    const SpurEnemyDef *e = ed(foe);
    char a[64], b2[64];
    b2[0] = 0;
    if (f->stun) { snprintf(a, sizeof a, "%s IS OUT COLD.", e->name); say(a, ""); return; }
    if (f->tied) { f->tied--; snprintf(a, sizeof a, "%s STRUGGLES IN THE SNARE.", e->name); say(a, ""); return; }
    int sum = 0;
    for (int k = 0; k < 4; k++) sum += e->act[k].weight;
    int r = rng_range(&sg.rng, 0, imax(1, sum) - 1), pick = 0;
    for (int k = 0; k < 4; k++) {
        if (r < e->act[k].weight) { pick = k; break; }
        r -= e->act[k].weight;
    }
    const SpurAct *ac = &e->act[pick];
    int slot = pick_target();
    if (slot < 0) return;
    int h = spur_hero_at(slot);
    snprintf(a, sizeof a, "%s ATTACKS.", e->name);
    switch (ac->kind) {
    case EA_HIT: {
        int s2 = guard_target(slot, foe);
        if (s2 != slot) snprintf(a, sizeof a, "%s ATTACKS - %s STEPS IN!", e->name, SPUR_HERO[spur_hero_at(s2)].name);
        blow(foe, s2, 100, false, b2, sizeof b2);
        break;
    }
    case EA_MULTI: {
        int n = imax(2, ac->power), total = 0, fell = 0;
        snprintf(a, sizeof a, "%s STRIKES %d TIMES.", e->name, n);
        for (int k = 0; k < n; k++) {
            int s2 = pick_target();
            if (s2 < 0) break;
            int was = spur_alive(spur_hero_at(s2));
            char tmp[64];
            total += blow(foe, s2, 80, false, tmp, sizeof tmp);
            fell += was && !spur_alive(spur_hero_at(s2));
        }
        snprintf(b2, sizeof b2, "%d DAMAGE IN ALL%s", total, fell ? ". SOMEONE FALLS!" : ".");
        break;
    }
    case EA_ALL: case EA_FIRE: {
        int total = 0;
        snprintf(a, sizeof a, ac->kind == EA_FIRE ? "%s BREATHES HELLFIRE!" : "%s LASHES OUT AT EVERYONE!", e->name);
        for (int i = 0; i < 4; i++) {
            if (!spur_alive(spur_hero_at(i))) continue;
            char tmp[64];
            if (ac->kind == EA_FIRE) {
                int dmg = imax(1, var(ac->power + e->atk / 4) - spur_stat(spur_hero_at(i), ST_RES) / 2);
                if (B.buff[i].defend) dmg /= 2;
                total += hurt_hero(i, dmg, NULL);
            } else total += blow(foe, i, 70, false, tmp, sizeof tmp);
        }
        snprintf(b2, sizeof b2, "%d DAMAGE ACROSS THE POSSE.", total);
        spur_sfx("spur_fire");
        break;
    }
    case EA_POISON: case EA_PARALYZE: {
        int done = blow(foe, slot, 80, false, b2, sizeof b2);
        if (done > 0 && spur_alive(h) && rng_range(&sg.rng, 0, 99) < 70 - spur_stat(h, ST_RES) / 2) {
            sg.s.hero[h].status |= ac->kind == EA_POISON ? SS_POISON : SS_PARALYZED;
            snprintf(b2, sizeof b2, "%s IS %s!", SPUR_HERO[h].name, ac->kind == EA_POISON ? "POISONED" : "STIFF AS A BOARD");
        }
        snprintf(a, sizeof a, "%s %s.", e->name, ac->kind == EA_POISON ? "BITES" : "STARES");
        break;
    }
    case EA_POISONALL:
        snprintf(a, sizeof a, "%s BREATHES A YELLOW CLOUD.", e->name);
        snprintf(b2, sizeof b2, "THE POSSE COUGHS.");
        for (int i = 0; i < 4; i++) {
            int t = spur_hero_at(i);
            if (spur_alive(t) && rng_range(&sg.rng, 0, 99) < 60 - spur_stat(t, ST_RES) / 2) sg.s.hero[t].status |= SS_POISON;
        }
        break;
    case EA_DEATH: case EA_DELETE: {
        snprintf(a, sizeof a, ac->kind == EA_DELETE ? "%s: DERAIL!" : "%s REACHES FOR A SOUL.", e->name);
        int chance = ac->kind == EA_DELETE ? 100 : imax(5, 45 - spur_stat(h, ST_RES) / 3);
        if (rng_range(&sg.rng, 0, 99) < chance && !sg.god) {
            bool saved = false;
            hurt_hero(slot, sg.s.hero[h].hp, &saved);
            if (saved) snprintf(b2, sizeof b2, "%s HANGS ON BY A THREAD!", SPUR_HERO[h].name);
            else snprintf(b2, sizeof b2, "%s DIES ON THE SPOT!", SPUR_HERO[h].name);
        } else snprintf(b2, sizeof b2, "%s SHAKES OFF THE CHILL.", SPUR_HERO[h].name);
        break;
    }
    case EA_DRAIN: {
        int dmg = imax(1, var(ac->power + e->atk / 2) - spur_stat(h, ST_RES) / 3);
        if (B.buff[slot].defend) dmg /= 2;
        int done = hurt_hero(slot, dmg, NULL);
        f->hp = imin(f->maxhp, f->hp + done);
        snprintf(a, sizeof a, "%s FEEDS.", e->name);
        snprintf(b2, sizeof b2, "%s LOSES %d.%s", SPUR_HERO[h].name, done, spur_alive(h) ? "" : " AND FALLS!");
        break;
    }
    case EA_GUARD: f->guard = 1; snprintf(a, sizeof a, "%s HUNKERS DOWN.", e->name); break;
    case EA_HEALALL:
        snprintf(a, sizeof a, "%s CALLS ON SOMETHING DARK.", e->name);
        for (int i = 0; i < B.nfoe; i++) if (spur_foe_alive(i)) B.foe[i].hp = imin(B.foe[i].maxhp, B.foe[i].hp + ac->power);
        snprintf(b2, sizeof b2, "THE FOES MEND.");
        break;
    case EA_CALL:
        if (spur_foes_left() < SPUR_FOES && B.nfoe < SPUR_FOES) {
            add_foe(ac->power);
            lay_out();
            snprintf(a, sizeof a, "%s WHISTLES.", e->name);
            snprintf(b2, sizeof b2, "%s JOINS IN!", SPUR_ENEMY[ac->power].name);
        } else { blow(foe, slot, 100, false, b2, sizeof b2); }
        break;
    case EA_WEAKEN:
        B.buff[slot].atk--;
        snprintf(a, sizeof a, "%s JEERS.", e->name);
        snprintf(b2, sizeof b2, "%s LOSES HEART.", SPUR_HERO[h].name);
        break;
    case EA_STEAL: {
        int t = imin(sg.s.teeth, imax(5, ac->power));
        sg.s.teeth -= t;
        snprintf(a, sizeof a, "%s PICKS A POCKET.", e->name);
        snprintf(b2, sizeof b2, "%d BITS GONE!", t);
        break;
    }
    case EA_FLEE:
        f->fled = 1;
        snprintf(a, sizeof a, "%s RUNS OFF!", e->name);
        break;
    case EA_IDLE: snprintf(a, sizeof a, "%s JITTERS.", e->name); break;
    default: break;
    }
    say(a, b2);
}

/* ---- the round ---------------------------------------------------------------------------- */
static bool posse_down(void) { return spur_posse_alive() == 0; }

static void win(void);
static void lose(void);

static void next_actor(void) {
    for (;;) {
        if (spur_foes_left() == 0) { win(); return; }
        if (posse_down()) { lose(); return; }
        if (B.fled) { B.phase = BP_FLED; B.phase_t = 0; return; }
        if (B.qi >= B.nqueue) {
            /* the round is over: poison bites, the rage burns down */
            for (int i = 0; i < 4; i++) {
                int h = spur_hero_at(i);
                if (!spur_alive(h)) continue;
                if (B.buff[i].rage) B.buff[i].rage--;
                if (sg.s.hero[h].status & SS_POISON) hurt_hero(i, imax(1, spur_maxhp(h) / 16), NULL);
            }
            if (posse_down()) { lose(); return; }
            start_round();
            return;
        }
        int a = B.queue[B.qi++];
        B.actor = a;
        if (a >= 10) {
            if (!spur_foe_alive(a - 10)) continue;
            foe_act(a - 10);
            return;
        }
        int h = spur_hero_at(a);
        if (!spur_alive(h)) continue;
        if (sg.s.hero[h].status & SS_PARALYZED) {
            char m[64];
            snprintf(m, sizeof m, "%s CAN'T MOVE.", SPUR_HERO[h].name);
            if (rng_range(&sg.rng, 0, 99) < 30) { sg.s.hero[h].status &= (uint8_t)~SS_PARALYZED; say(m, "...BUT LOOSENS UP."); }
            else say(m, "");
            return;
        }
        SpurOrder *o = &B.order[a];
        o->done = true;
        /* the hound and the raging always swing */
        if (h == H_BISCUIT || B.buff[a].rage) {
            o->cmd = CMD_ATTACK;
            int nl = 0, live[SPUR_FOES];
            for (int i = 0; i < B.nfoe; i++) if (spur_foe_alive(i)) live[nl++] = i;
            o->target = (uint8_t)live[rng_range(&sg.rng, 0, nl - 1)];
        }
        switch (o->cmd) {
        case CMD_ATTACK:
            if (sg.s.hero[h].reload) {
                char m[64];
                sg.s.hero[h].reload = 0;
                snprintf(m, sizeof m, "%s RELOADS.", SPUR_HERO[h].name);
                spur_sfx("spur_reload");
                say(m, "");
                return;
            }
            o->target = (uint8_t)imax(0, retarget(o->target));
            bar_setup(a);
            return;
        case CMD_SKILL: do_skill(a); return;
        case CMD_ITEM: do_item(a); return;
        case CMD_DEFEND: {
            char m[64];
            snprintf(m, sizeof m, "%s KEEPS HEAD DOWN.", SPUR_HERO[h].name);
            say(m, "");
            return;
        }
        case CMD_RUN: do_run(a); return;
        default: continue;
        }
    }
}

/* ---- the end of a fight ------------------------------------------------------------------- */
static void win(void) {
    B.won = true;
    int xp = 0, teeth = 0;
    for (int i = 0; i < B.nfoe; i++) {
        if (B.foe[i].fled) continue;
        xp += SPUR_ENEMY[B.foe[i].e].xp;
        teeth += SPUR_ENEMY[B.foe[i].e].teeth;
    }
    int alive = spur_posse_alive();
    B.xp_gain = alive ? xp / alive : 0;
    B.teeth_gain = teeth;
    sg.s.teeth = imin(9999999, sg.s.teeth + teeth);
    /* the bank pays two in a hundred after every fight won, rounded up, to 100,000 */
    if (sg.s.bank > 0 && sg.s.bank < 100000) sg.s.bank = imin(100000, sg.s.bank + (sg.s.bank * 2 + 99) / 100);
    for (int i = 0; i < 4; i++) {
        B.levelups[i] = 0;
        B.lvl_msg[i][0] = 0;
        int h = spur_hero_at(i);
        if (!spur_alive(h)) continue;
        sg.s.hero[h].xp += B.xp_gain;
        B.levelups[i] = spur_level_up_check(h, B.lvl_msg[i], sizeof B.lvl_msg[i]);
        if (sg.s.hero[h].level >= 10) spur_award_level_goal();
        if (sg.s.hero[h].level > sg.s.max_level) sg.s.max_level = sg.s.hero[h].level;
        sg.s.hero[h].status &= (uint8_t)~SS_PARALYZED;
    }
    if (B.ambush_flag >= 0) spur_set_flag(B.ambush_flag);
    /* the cherry: every one the angel saved still standing when Zenith falls */
    if (B.troop == 100) sg.cherry_run = spur_posse_alive() == sg.s.nposse && !sg.god;
    B.phase = BP_WIN;
    B.phase_t = 0;
    music_play(SPUR_MUS_WIN);
}

static void lose(void) {
    B.wiped = true;
    B.phase = BP_LOSE;
    B.phase_t = 0;
    music_play(SPUR_MUS_LOSE);
}

void spur_battle_end(void) {
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (h >= 0) { sg.s.hero[h].reload = 0; sg.s.hero[h].status &= (uint8_t)~SS_PARALYZED; }
    }
    if (B.wiped) { spur_respawn_at_chapel(); return; }
    int troop = B.troop;
    bool won = B.won;
    spur_field_after_battle(won);
    if (won) spur_battle_won_event(troop);
}

/* ---- the command phase --------------------------------------------------------------------- */
static int battle_items[SPUR_BAG], nbattle_items;
static int battle_skills[SK_COUNT], nbattle_skills;

static void gather_items(void) {
    nbattle_items = 0;
    for (int i = 0; i < SPUR_BAG; i++) {
        int it = sg.s.bag[i];
        if (it && SPUR_ITEM[it].kind == IK_USE && (SPUR_ITEM[it].flags & IF_BATTLE)) battle_items[nbattle_items++] = i;
    }
}

static void gather_skills(int h) {
    int all[SK_COUNT];
    int n = spur_skill_list(h, all);
    nbattle_skills = 0;
    for (int i = 0; i < n; i++) if (SPUR_SKILL[all[i]].use & SKU_BATTLE) battle_skills[nbattle_skills++] = all[i];
}

static void order_done(void) {
    B.order[B.cur].done = false;
    int nx = next_commander(B.cur + 1);
    if (nx < 0) {
        build_queue();
        B.phase = BP_ACT;
        B.phase_t = 0;
        next_actor();
        return;
    }
    B.cur = nx;
    B.sel = 0;
    B.phase = BP_COMMAND;
}

static void command_back(void) {
    for (int i = B.cur - 1; i >= 0; i--)
        if (controllable(i)) { B.cur = i; B.sel = B.order[i].cmd; B.phase = BP_COMMAND; spur_sfx("spur_back"); return; }
}

static void target_move(int d) {
    int n = B.nfoe;
    for (int k = 0; k < n; k++) {
        B.target_sel = (B.target_sel + d + n) % n;
        if (spur_foe_alive(B.target_sel)) break;
    }
    spur_sfx("spur_tick");
}

static int target_kind;   /* SKT_* for the ally / foe choice in progress */

static void command_update(void) {
    int h = spur_hero_at(B.cur);
    bool up = btn_repeat(BTN_UP), down = btn_repeat(BTN_DOWN), ok = btnp(SPUR_OK), back = btnp(SPUR_BACK);
    switch (B.phase) {
    case BP_COMMAND:
        if (up) { B.sel = (B.sel + CMD_COUNT - 1) % CMD_COUNT; spur_sfx("spur_tick"); }
        if (down) { B.sel = (B.sel + 1) % CMD_COUNT; spur_sfx("spur_tick"); }
        if (back) { command_back(); return; }
        if (!ok) return;
        B.order[B.cur].cmd = (uint8_t)B.sel;
        switch (B.sel) {
        case CMD_ATTACK:
            if (sg.s.hero[h].reload) { spur_sfx("spur_ok"); order_done(); return; }
            B.target_sel = imax(0, first_foe());
            target_kind = SKT_ENEMY;
            B.phase = BP_TARGET;
            spur_sfx("spur_ok");
            return;
        case CMD_SKILL:
            gather_skills(h);
            if (!nbattle_skills) { spur_sfx("spur_no"); return; }
            B.sub_sel = 0;
            B.phase = BP_SKILLMENU;
            spur_sfx("spur_ok");
            return;
        case CMD_ITEM:
            gather_items();
            if (!nbattle_items) { spur_sfx("spur_no"); return; }
            B.sub_sel = 0;
            B.phase = BP_ITEMMENU;
            spur_sfx("spur_ok");
            return;
        default: spur_sfx("spur_ok"); order_done(); return;
        }
    case BP_TARGET:
        if (btnp(BTN_UP) || btnp(BTN_LEFT)) target_move(-1);
        if (btnp(BTN_DOWN) || btnp(BTN_RIGHT)) target_move(1);
        if (back) { B.phase = BP_COMMAND; spur_sfx("spur_back"); return; }
        if (ok) { B.order[B.cur].target = (uint8_t)B.target_sel; spur_sfx("spur_ok"); order_done(); }
        return;
    case BP_ALLYTARGET: {
        int n = imax(1, sg.s.nposse);
        if (btnp(BTN_UP) || btnp(BTN_LEFT)) { B.target_sel = (B.target_sel + n - 1) % n; spur_sfx("spur_tick"); }
        if (btnp(BTN_DOWN) || btnp(BTN_RIGHT)) { B.target_sel = (B.target_sel + 1) % n; spur_sfx("spur_tick"); }
        if (back) { B.phase = B.order[B.cur].cmd == CMD_SKILL ? BP_SKILLMENU : BP_ITEMMENU; spur_sfx("spur_back"); return; }
        if (ok) { B.order[B.cur].target = (uint8_t)B.target_sel; spur_sfx("spur_ok"); order_done(); }
        return;
    }
    case BP_SKILLMENU: {
        int n = imax(1, nbattle_skills);
        if (up) { B.sub_sel = (B.sub_sel + n - 1) % n; spur_sfx("spur_tick"); }
        if (down) { B.sub_sel = (B.sub_sel + 1) % n; spur_sfx("spur_tick"); }
        if (back) { B.phase = BP_COMMAND; spur_sfx("spur_back"); return; }
        if (!ok) return;
        int sk = battle_skills[B.sub_sel];
        if (sg.s.hero[h].sp < SPUR_SKILL[sk].sp) { spur_sfx("spur_no"); return; }
        B.order[B.cur].skill = (uint8_t)sk;
        int t = SPUR_SKILL[sk].target;
        spur_sfx("spur_ok");
        if (t == SKT_ENEMY) { B.target_sel = imax(0, first_foe()); B.phase = BP_TARGET; }
        else if (t == SKT_ALLY || t == SKT_DEADALLY) { B.target_sel = B.cur; B.phase = BP_ALLYTARGET; }
        else order_done();
        return;
    }
    case BP_ITEMMENU: {
        int n = imax(1, nbattle_items);
        if (up) { B.sub_sel = (B.sub_sel + n - 1) % n; spur_sfx("spur_tick"); }
        if (down) { B.sub_sel = (B.sub_sel + 1) % n; spur_sfx("spur_tick"); }
        if (back) { B.phase = BP_COMMAND; spur_sfx("spur_back"); return; }
        if (!ok) return;
        int bs = battle_items[B.sub_sel];
        B.order[B.cur].item_slot = (uint8_t)bs;
        spur_sfx("spur_ok");
        if (SPUR_ITEM[sg.s.bag[bs]].use == IU_STUN) { B.target_sel = imax(0, first_foe()); B.phase = BP_TARGET; }
        else { B.target_sel = B.cur; B.phase = BP_ALLYTARGET; }
        return;
    }
    default: return;
    }
}

/* ---- one frame -------------------------------------------------------------------------------- */
void spur_battle_update(void) {
    B.phase_t++;
    if (B.shake) B.shake--;
    for (int i = 0; i < B.nfoe; i++) {
        if (B.foe[i].flash) B.foe[i].flash--;
        if (B.foe[i].dying && B.foe[i].dying < 30) B.foe[i].dying++;
    }
    switch (B.phase) {
    case BP_INTRO:
        if (B.phase_t > 50 || (B.phase_t > 12 && btnp(SPUR_OK))) { input_consume(); start_round(); }
        return;
    case BP_COMMAND: case BP_TARGET: case BP_ALLYTARGET: case BP_SKILLMENU: case BP_ITEMMENU:
        command_update();
        return;
    case BP_ACT:
        next_actor();
        return;
    case BP_BAR:
        bar_update(B.actor);
        return;
    case BP_MSG:
        B.msg_t++;
        if (B.msg_t > MSG_T || (B.msg_t > 10 && btnp(SPUR_OK))) {
            if (btnp(SPUR_OK)) input_consume();
            B.phase = BP_ACT;
            next_actor();
        }
        return;
    case BP_WIN:
        if (B.phase_t > 30 && btnp(SPUR_OK)) {
            input_consume();
            bool any = false;
            for (int i = 0; i < 4; i++) any |= B.levelups[i] > 0;
            if (any) { B.phase = BP_LEVELUP; B.phase_t = 0; music_play(SPUR_MUS_LEVEL); }
            else { B.phase = BP_DONE; spur_battle_end(); }
        }
        return;
    case BP_LEVELUP:
        if (B.phase_t > 30 && btnp(SPUR_OK)) { input_consume(); B.phase = BP_DONE; spur_battle_end(); }
        return;
    case BP_LOSE:
        if (B.phase_t > 90 && btnp(SPUR_OK)) { input_consume(); B.phase = BP_DONE; spur_battle_end(); }
        return;
    case BP_FLED:
        if (B.phase_t > 20) { B.phase = BP_DONE; spur_battle_end(); }
        return;
    default: return;
    }
}

/* ---- drawing ----------------------------------------------------------------------------------- */
static void bpanel(int x, int y, int w, int h) {
    gfx_rect(x, y, w, h, C_INK);
    gfx_rectb(x, y, w, h, C_CREAM);
    gfx_rectb(x + 1, y + 1, w - 2, h - 2, C_BROWN);
}

static void backdrop(void) {
    int id = sg.map_id;
    int sky = C_NIGHT, ground = C_EARTH, far = C_DUSK;
    if (sg.map.kind == MK_DUNGEON) { sky = C_INK; ground = C_BROWN; far = C_NIGHT; }
    if (id == M_HELL || (id >= M_TOWER1 && id <= M_TOWER4) || id == M_SPUR1 || id == M_SPUR2 || id == M_SPUR3) { sky = C_MAROON; ground = C_INK; far = C_WINE; }
    if (id == M_DREAM1 || id == M_DREAM2) { sky = C_PURPLE; ground = C_VIOLET; far = C_MAGENTA; }
    gfx_rect(0, 0, SCREEN_W, 124, sky);
    for (int x = 0; x < SCREEN_W; x += 2) {
        int hh = 70 + ((x * 37 + id * 11) % 23) / 2;
        gfx_vline(x, hh, 90, far);
        gfx_vline(x + 1, hh, 90, far);
    }
    gfx_rect(0, 90, SCREEN_W, 34, ground);
    for (int k = 0; k < 40; k++) gfx_pset((k * 53 + id * 7) % SCREEN_W, 92 + (k * 31) % 30, PAL_DARKER[ground]);
    if (sg.map.kind == MK_WORLD && id == M_WORLD)
        for (int k = 0; k < 24; k++) gfx_pset((k * 71) % SCREEN_W, (k * 29) % 60, C_GREY);
}

void spur_battle_draw(void) {
    int sx = B.shake ? (B.shake % 2 ? 2 : -2) : 0;
    backdrop();
    /* the foes */
    for (int i = 0; i < B.nfoe; i++) {
        SpurFoe *f = &B.foe[i];
        if (f->e <= 0 || f->fled) continue;
        if (f->hp <= 0 && f->dying >= 24) continue;
        if (f->dying && (f->dying / 3) % 2) continue;
        spur_draw_enemy(f->e, f->x, f->y, f->flash);
        if (f->stun) tiny_draw("ZZ", f->x + spur_enemy_w(f->e) - 6, f->y - 4, C_YELLOW);
        if (f->tied) tiny_draw("TIED", f->x, f->y - 4, C_TAN);
    }
    /* the posse */
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (h < 0) continue;
        int x = 34 + i * 8 + sx, y = 10 + i * 26;
        bool acting = (B.phase == BP_BAR || B.phase == BP_MSG) && B.actor == i;
        bool ordering = B.phase >= BP_COMMAND && B.phase <= BP_ITEMMENU && B.cur == i;
        if (acting || ordering) x += 10;
        if (!spur_alive(h)) { spur_draw_hero_big(h, 0, x, y, true); continue; }
        gfx_dither(x + 2, y + 30, 28, 3, C_INK, 8);
        spur_draw_hero_big(h, acting ? 1 + (B.phase == BP_BAR) : (sg.ani / 24) % 2 * 0, x, y, false);
        if (B.buff[i].defend) tiny_draw("DEF", x + 30, y + 4, C_SKY);
        if (B.buff[i].rage) tiny_draw("RAGE", x + 30, y + 12, C_RED);
    }
    /* the panel: the posse on the left, orders or words on the right */
    bpanel(0, 124, 128, 56);
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (h < 0) continue;
        const SpurHero *hr = &sg.s.hero[h];
        char b[32];
        int y = 128 + i * 12;
        bool cur = (B.phase >= BP_COMMAND && B.phase <= BP_ITEMMENU && B.cur == i);
        text_draw(SPUR_HERO[h].name, 6, y, hr->hp <= 0 ? C_GREY : cur ? C_YELLOW : C_WHITE);
        snprintf(b, sizeof b, "%d", hr->hp);
        tiny_draw(b, 66, y + 1, hr->hp <= 0 ? C_RED : hr->hp * 4 < spur_maxhp(h) ? C_ORANGE : C_WHITE);
        snprintf(b, sizeof b, "%d", hr->sp);
        tiny_draw(b, 96, y + 1, C_SKY);
        if (hr->status & SS_POISON) gfx_rect(120, y + 2, 3, 3, C_LIME);
        if (hr->status & SS_PARALYZED) gfx_rect(116, y + 2, 3, 3, C_SKY);
    }
    bpanel(128, 124, 192, 56);
    char b[64];
    switch (B.phase) {
    case BP_INTRO: case BP_MSG: case BP_FLED: case BP_LOSE:
        text_draw(B.msg[0], 136, 132, C_WHITE);
        text_draw(B.msg[1], 136, 144, C_CREAM);
        if (B.phase == BP_LOSE) {
            text_draw("THE POSSE HAS FALLEN.", 136, 132, C_RED);
            text_draw("THE WISPS DRAG YOU BACK...", 136, 144, C_CREAM);
        }
        break;
    case BP_COMMAND: case BP_TARGET: case BP_ALLYTARGET: {
        static const char *const CMD[CMD_COUNT] = {"ATTACK", "SKILL", "ITEM", "DEFEND", "RUN"};
        int h = spur_hero_at(B.cur);
        for (int i = 0; i < CMD_COUNT; i++) {
            const char *s = CMD[i];
            if (i == 0 && h >= 0 && sg.s.hero[h].reload) s = "RELOAD";
            bool dim = (i == CMD_RUN && B.norun);
            text_draw(s, 146, 128 + i * 10, i == B.sel ? C_YELLOW : dim ? C_SLATE : C_WHITE);
        }
        if (B.phase == BP_COMMAND && (sg.ani / 8) % 4 != 3) text_draw(GLYPH_RIGHT, 136, 128 + B.sel * 10, C_YELLOW);
        if (h >= 0) {
            snprintf(b, sizeof b, "%s", SPUR_HERO[h].name);
            text_draw(b, 230, 130, C_TAN);
            int it = sg.s.hero[h].equip[EQ_RHAND];
            tiny_draw(it ? SPUR_ITEM[it].name : "BARE HANDS", 230, 142, C_GREY);
        }
        if (B.phase == BP_TARGET && spur_foe_alive(B.target_sel)) {
            SpurFoe *f = &B.foe[B.target_sel];
            if ((sg.ani / 6) % 2) text_draw(GLYPH_DOWN, f->x + spur_enemy_w(f->e) / 2 - 3, f->y - 9, C_YELLOW);
            text_draw(SPUR_ENEMY[f->e].name, 230, 156, C_YELLOW);
        }
        if (B.phase == BP_ALLYTARGET) {
            int y = 10 + B.target_sel * 26;
            if ((sg.ani / 6) % 2) text_draw(GLYPH_RIGHT, 22 + B.target_sel * 8, y + 10, C_YELLOW);
            int t = spur_hero_at(B.target_sel);
            if (t >= 0) text_draw(SPUR_HERO[t].name, 230, 156, C_YELLOW);
        }
        break;
    }
    case BP_SKILLMENU: {
        int h = spur_hero_at(B.cur);
        int top = imax(0, B.sub_sel - 4);
        for (int i = top; i < nbattle_skills && i < top + 5; i++) {
            const SpurSkillDef *d = &SPUR_SKILL[battle_skills[i]];
            bool can = sg.s.hero[h].sp >= d->sp;
            text_draw(d->name, 146, 128 + (i - top) * 10, i == B.sub_sel ? C_YELLOW : can ? C_WHITE : C_SLATE);
            snprintf(b, sizeof b, "%d", d->sp);
            tiny_draw(b, 220, 129 + (i - top) * 10, C_SKY);
        }
        if ((sg.ani / 8) % 4 != 3) text_draw(GLYPH_RIGHT, 136, 128 + (B.sub_sel - top) * 10, C_YELLOW);
        if (nbattle_skills) {
            char lines[4][UI_WRAP_LEN];
            int nl = ui_wrap(SPUR_SKILL[battle_skills[B.sub_sel]].desc, 76, true, lines, 4);
            for (int i = 0; i < imin(nl, 4); i++) tiny_draw(lines[i], 238, 130 + i * 7, C_CREAM);
        }
        break;
    }
    case BP_ITEMMENU: {
        int top = imax(0, B.sub_sel - 4);
        for (int i = top; i < nbattle_items && i < top + 5; i++)
            text_draw(SPUR_ITEM[sg.s.bag[battle_items[i]]].name, 146, 128 + (i - top) * 10, i == B.sub_sel ? C_YELLOW : C_WHITE);
        if ((sg.ani / 8) % 4 != 3) text_draw(GLYPH_RIGHT, 136, 128 + (B.sub_sel - top) * 10, C_YELLOW);
        break;
    }
    case BP_BAR: {
        int h = spur_hero_at(B.actor);
        snprintf(b, sizeof b, "%s %s", SPUR_HERO[h].name, B.bar_auto ? "SWINGS" : B.bar_kind == WC_HEAVY ? "- HOLD " GLYPH_A : "- PRESS " GLYPH_A);
        text_draw(b, 136, 130, C_WHITE);
        int x0 = 160, y0 = 150, W = BAR_W;
        gfx_rect(x0 - 2, y0 - 2, W + 4, 12, C_INK);
        gfx_rectb(x0 - 2, y0 - 2, W + 4, 12, C_GREY);
        gfx_rect(x0, y0, W, 8, C_SLATE);
        for (int k = 0; k < B.nz; k++) {
            SpurZoneBar *z = &B.z[k];
            int ya = z->a / 16, yr = z->red / 16, yb = z->b / 16;
            int ycol = z->result == ZR_MISS ? C_GREY : C_YELLOW;
            if (yr > ya) gfx_rect(x0 + ya, y0, yr - ya, 8, ycol);
            if (yb > yr) gfx_rect(x0 + yr, y0, yb - yr, 8, z->result == ZR_MISS ? C_DUSK : C_RED);
            if (z->result == ZR_HIT) tiny_draw("HIT", x0 + ya, y0 + 11, C_YELLOW);
            if (z->result == ZR_CRIT) tiny_draw("CRIT", x0 + ya, y0 + 11, C_RED);
            if (z->result == ZR_MISS) tiny_draw("MISS", x0 + ya, y0 + 11, C_GREY);
        }
        int mx = x0 + imin(W - 1, B.bar_x / 16);
        gfx_rect(mx - 1, y0 - 3, 3, 14, C_WHITE);
        if (B.bar_kind == WC_HEAVY && B.bar_shots) {
            snprintf(b, sizeof b, "%d", B.bar_shots);
            text_draw(b, x0 + W + 6, y0, C_ORANGE);
        }
        break;
    }
    case BP_WIN: {
        text_draw("THE WISPS ARE SNUFFED OUT.", 136, 128, C_YELLOW);
        snprintf(b, sizeof b, "%d XP EACH, %d BITS", B.xp_gain, B.teeth_gain);
        text_draw(b, 136, 140, C_WHITE);
        if (sg.s.bank > 0) {
            snprintf(b, sizeof b, "THE BANK HOLDS %d", sg.s.bank);
            tiny_draw(b, 136, 154, C_TAN);
        }
        break;
    }
    case BP_LEVELUP: {
        int row = 0;
        for (int i = 0; i < 4 && row < 4; i++) {
            if (!B.levelups[i]) continue;
            int h = spur_hero_at(i);
            snprintf(b, sizeof b, "%s IS LEVEL %d!", SPUR_HERO[h].name, sg.s.hero[h].level);
            text_draw(b, 136, 128 + row * 12, C_YELLOW);
            if (B.lvl_msg[i][0]) tiny_draw(B.lvl_msg[i], 236, 130 + row * 12, C_CREAM);
            row++;
        }
        break;
    }
    default: break;
    }
}
#undef B
