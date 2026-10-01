/* BUZZBOLT - the ship, its fire, the letters and their words, shots,
 * scoring, lives and the flow of a wave. See docs/games/39-buzzbolt.md. */
#include "buzzbolt.h"

BzzGame bz;
BzzSave bzs;

const char *const BZZ_SHIP_NAME[BZ_SHIPS] = {"LACEWING", "SHIELDBUG", "FIREFLY"};

/* extra ships at these scores, and only these */
static const uint32_t EXTEND_AT[3] = {25000, 100000, 200000};
/* each ship's option: how many hits it takes */
static const int OPT_HP[BZ_SHIPS] = {10, 24, 8};
#define POWER_T 600    /* the shieldbug's power-up lasts ten seconds */
#define BOMB_FUSE 36   /* a bomb goes off this long after launch */
#define BLAST_R 40.0f
#define BLAST_T 26
#define HIVE_T 190     /* the lacewing's big ally: arrives, fires, leaves */
#define HIVE_FIRE0 22
#define HIVE_FIRE1 170
#define CLEAR_WAIT 150 /* after the last boss: time to catch its letters */

/* ------------------------------------------------------------------------ */
/* small helpers                                                              */

void bzz_burst(float x, float y, int col, int n, float sp) {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < BZZ_MAX_PARTS; i++) {
            Part *p = &bz.part[i];
            if (p->life > 0) continue;
            float a = rng_float(&g_rng) * 6.2832f, s = sp * (0.3f + rng_float(&g_rng));
            *p = (Part){x, y, cosf(a) * s, sinf(a) * s, 14 + rng_range(&g_rng, 0, 14), col, 0};
            break;
        }
    }
}

static int add_pshot(int kind, float x, float y, float vx, float vy, float w, float h, int dmg) {
    for (int i = 0; i < BZZ_MAX_PSHOTS; i++) {
        PShot *s = &bz.ps[i];
        if (s->alive) continue;
        *s = (PShot){x, y, vx, vy, w, h, (uint8_t)kind, 1, dmg, 0, 0, -1, 0};
        return i;
    }
    return -1;
}

int bzz_add_eshot(int kind, float x, float y, float vx, float vy) {
    static const float R[] = {2.0f, 2.5f, 5.0f, 4.0f, 2.5f};
    for (int i = 0; i < BZZ_MAX_ESHOTS; i++) {
        EShot *s = &bz.es[i];
        if (s->alive) continue;
        *s = (EShot){x, y, vx, vy, R[kind], (uint8_t)kind, 1, 0, 0};
        return i;
    }
    return -1;
}

/* n shots in a fan of spread_deg, centred on the ship */
void bzz_aimed(float x, float y, float speed, int n, float spread_deg) {
    float a = bzz_aim(x, y);
    for (int k = 0; k < n; k++) {
        float o = n > 1 ? (k - (n - 1) / 2.0f) * spread_deg * 0.017453f : 0;
        bzz_add_eshot(ES_AIMED, x, y, cosf(a + o) * speed, sinf(a + o) * speed);
    }
}

void bzz_ring(float x, float y, float speed, int n, float rot, int kind) {
    for (int k = 0; k < n; k++) {
        float a = rot + k * 6.2832f / n;
        bzz_add_eshot(kind, x, y, cosf(a) * speed, sinf(a) * speed);
    }
}

/* One frame of an enemy shot. Homing shots turn toward the ship for a
 * while; rings bounce off the bottom of the screen once. The demo player
 * runs the same code to look ahead. */
void bzz_eshot_step(EShot *s, float px, float py) {
    if ((s->kind == ES_HOMING || s->kind == ES_BIG) && s->t < 80) {
        float want = atan2f(py - s->y, px - s->x), have = atan2f(s->vy, s->vx);
        float d = want - have;
        while (d > 3.14159f) d -= 6.28318f;
        while (d < -3.14159f) d += 6.28318f;
        float lim = s->kind == ES_BIG ? 0.035f : 0.045f;
        float turn = fclamp(d, -lim, lim);
        float sp = sqrtf(s->vx * s->vx + s->vy * s->vy);
        s->vx = cosf(have + turn) * sp;
        s->vy = sinf(have + turn) * sp;
    }
    s->x += s->vx;
    s->y += s->vy;
    if (s->kind == ES_RING && !s->bounced && s->y > BZZ_H - s->r && s->vy > 0) {
        s->vy = -s->vy;
        s->bounced = 1;
    }
    s->t++;
}

void bzz_save_now(void) {
    bzs.magic = 0x425A0001u;
    game_save_write(game_current_index(), &bzs, (int)sizeof bzs);
}

void bzz_clear_world(void) {
    memset(bz.foe, 0, sizeof bz.foe);
    memset(bz.es, 0, sizeof bz.es);
    memset(bz.ps, 0, sizeof bz.ps);
    memset(bz.lt, 0, sizeof bz.lt);
    memset(bz.blast, 0, sizeof bz.blast);
    memset(bz.bomb, 0, sizeof bz.bomb);
}

/* ------------------------------------------------------------------------ */
/* score, letters, words                                                      */

void bzz_add_score(uint32_t pts) {
    bz.score += pts;
    while (bz.extends < 3 && bz.score >= EXTEND_AT[bz.extends]) {
        bz.extends++;
        bz.lives++;
        sfx_play_name("bzz_extend");
    }
}

/* the letter the next kill drops: B, Z, Z, B, Z, Z ... from the run's first kill */
int bzz_next_drop(void) { return bz.drop_idx++ % 3 == 0 ? LT_B : LT_Z; }

int bzz_word_of(const uint8_t *w) {
    if (w[0] == LT_B && w[1] == LT_Z && w[2] == LT_Z) return W_BZZ;
    if (w[0] == LT_Z && w[1] == LT_Z && w[2] == LT_Z) return W_ZZZ;
    if (w[0] == LT_B && w[1] == LT_B && w[2] == LT_B) return W_BBB;
    if (w[0] == LT_Z && w[1] == LT_B && w[2] == LT_B) return W_ZBB;
    return W_WRONG;
}

static void add_option(void) {
    if (bz.nopt >= BZZ_MAX_OPTIONS) {
        /* both out already: the word patches them up */
        for (int i = 0; i < bz.nopt; i++) bz.opt[i].hp = bz.opt[i].maxhp;
        return;
    }
    Option *o = &bz.opt[bz.nopt++];
    memset(o, 0, sizeof *o);
    o->alive = 1;
    o->x = bz.px;
    o->y = bz.py + 6;
    o->hp = o->maxhp = OPT_HP[bz.ship];
    o->target = -1;
}

static void screen_clear(void) {
    for (int i = 0; i < BZZ_MAX_ESHOTS; i++) {
        EShot *s = &bz.es[i];
        if (!s->alive) continue;
        s->alive = 0;
        bzz_burst(s->x, s->y, C_YELLOW, 1, 0.8f);
    }
    for (int i = 0; i < BZZ_MAX_FOES; i++) {
        Foe *e = &bz.foe[i];
        if (e->alive && e->y > -8 && e->y < BZZ_H + 8 && e->x > -8 && e->x < BZZ_W + 8) bzz_hurt_foe(i, 10);
    }
    bz.flash = 8;
}

void bzz_word_effect(int w) {
    switch (w) {
    case W_BZZ:
        bz.mult++;
        if (bz.mult > bz.best_mult) bz.best_mult = bz.mult;
        if (bz.mult >= 10 && !bz.beacon_given) { bz.beacon_given = true; game_award(GOAL_BEACON); }
        sfx_play_name("bzz_mult");
        break;
    case W_ZZZ: add_option(); sfx_play_name("bzz_option"); break;
    case W_BBB:
        if (bz.ship == BZ_LACEWING) bz.orb[0] = bz.orb[1] = true;
        else if (bz.ship == BZ_SHIELDBUG) bz.power_t = POWER_T; /* a fresh ten seconds, never more */
        else if (bz.bombs < BZZ_MAX_BOMBS) bz.bombs++;
        sfx_play_name("bzz_special");
        break;
    case W_ZBB:
        if (bz.ship == BZ_LACEWING) { bz.hive.on = true; bz.hive.x = bz.px; bz.hive.y = BZZ_H + 30; bz.hive.t = 0; }
        else if (bz.ship == BZ_SHIELDBUG) screen_clear();
        else { bz.fly.on = true; bz.fly.x = -24; bz.fly.y = 62; bz.fly.t = 0; bz.fly.fired = 0; }
        sfx_play_name("bzz_special");
        break;
    default:
        bz.mult = 1;
        sfx_play_name("bzz_wrong");
        break;
    }
}

void bzz_catch_letter(int letter) {
    bz.word[bz.nword++] = (uint8_t)letter;
    bz.letters_caught++;
    sfx_play_name(letter == LT_B ? "bzz_letb" : "bzz_letz");
    if (bz.nword < 3) return;
    int w = bzz_word_of(bz.word);
    bz.last_word = w;
    bz.word_t = 40;
    memcpy(bz.shown, bz.word, 3);
    bzz_word_effect(w);
    bz.nword = 0;
    memset(bz.word, 0, sizeof bz.word);
}

static void drop_letter(float x, float y, float vx) {
    for (int i = 0; i < BZZ_MAX_LETTERS; i++) {
        Letter *l = &bz.lt[i];
        if (l->alive) continue;
        *l = (Letter){x, y, vx, -1.4f, 1, (uint8_t)bzz_next_drop(), 0};
        return;
    }
    bz.drop_idx++; /* no room: the letter is lost, the cycle still moves on */
}

/* ------------------------------------------------------------------------ */
/* foes: hurt and kill                                                        */

int bzz_foe_value(const Foe *e) {
    return e->value;
}

void bzz_kill_foe(int i) {
    Foe *e = &bz.foe[i];
    if (!e->alive) return;
    e->alive = 0;
    bz.kills++;
    bz.kills_wave++;
    bzz_add_score((uint32_t)bzz_foe_value(e) * (uint32_t)bz.mult);
    int col = e->kind == EK_GOLDBUG ? C_YELLOW : e->kind == EK_BLISTER ? C_RED : e->kind == EK_ROTWALL ? C_VIOLET : C_AMBER;
    bool big = e->role != ROLE_FOE;
    bzz_burst(e->x, e->y, col, big ? 40 : 8, big ? 2.6f : 1.4f);
    if (big) { bz.shake = 20; bz.flash = 6; sfx_play_name("bzz_bigboom"); }
    else sfx_play_name("bzz_pop");
    if (big) {
        drop_letter(e->x - 10, e->y, -0.9f);
        drop_letter(e->x, e->y, 0);
        drop_letter(e->x + 10, e->y, 0.9f);
    } else {
        drop_letter(e->x, e->y, 0);
    }
    /* a big spore-rock bursts into two small ones */
    if (e->kind == EK_BIGPUFF) {
        for (int k = 0; k < 2; k++) {
            int j = bzz_spawn_foe(EK_PUFF, FM_DRIFT, e->x + (k ? 6 : -6), e->y, k, k ? 1 : -1, 0, 0);
            if (j >= 0) { bz.foe[j].vx = k ? 0.9f : -0.9f; bz.foe[j].vy = 1.0f; }
        }
    }
    if (e->role == ROLE_BOSS) {
        bz.boss_count--;
        if (bz.boss_count <= 0 && !bz.boss_dead) {
            bz.boss_dead = true;
            bz.clear_t = 0;
            bz.time_bonus = imax(0, BZZ_WAVE[bz.wave].par_s - bz.wave_t / 60) * 100;
            /* the rest flee and every shot fizzles out */
            for (int k = 0; k < BZZ_MAX_FOES; k++) if (bz.foe[k].alive) bz.foe[k].leaving = 1;
            for (int k = 0; k < BZZ_MAX_ESHOTS; k++)
                if (bz.es[k].alive) { bz.es[k].alive = 0; bzz_burst(bz.es[k].x, bz.es[k].y, C_GREY, 1, 0.5f); }
        }
    }
}

void bzz_hurt_foe(int i, int dmg) {
    Foe *e = &bz.foe[i];
    if (!e->alive || dmg <= 0) return;
    if (e->kind == EK_ROTWALL && !e->state) return; /* a gate that is off can't be hit */
    e->hp -= dmg;
    e->flash = 3;
    if (e->hp <= 0) bzz_kill_foe(i);
}

/* ------------------------------------------------------------------------ */
/* the ship                                                                   */

static bool foe_on_screen(const Foe *e) { return e->alive && e->y > -6 && e->y < BZZ_H && e->x > -6 && e->x < BZZ_W + 6; }

static int nearest_foe(float x, float y, bool above) {
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < BZZ_MAX_FOES; i++) {
        Foe *e = &bz.foe[i];
        if (!foe_on_screen(e) || (e->kind == EK_ROTWALL && !e->state)) continue;
        if (above && e->y > y) continue;
        float d = (e->x - x) * (e->x - x) + (e->y - y) * (e->y - y);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

void bzz_kill_ship(void) {
    if (!bz.alive || bz.god) return;
    bz.alive = false;
    bz.dead_t = BZZ_RESPAWN_T;
    bz.deaths++;
    bzz_burst(bz.px, bz.py, C_WHITE, 30, 2.4f);
    bzz_burst(bz.px, bz.py, C_ORANGE, 20, 1.6f);
    bz.shake = 24;
    sfx_play_name("bzz_die");
    /* everything the letters gave is lost, and the multiplier with it */
    bz.mult = 1;
    bz.nword = 0;
    memset(bz.word, 0, sizeof bz.word);
    bz.nopt = 0;
    memset(bz.opt, 0, sizeof bz.opt);
    bz.orb[0] = bz.orb[1] = false;
    bz.power_t = 0;
    bz.bombs = 0;
    bz.hive.on = false;
    bz.fly.on = false;
    bz.charge = 0;
    bz.laser_t = 0;
    bz.fire_hold = 0;
    bz.spread_win = 0;
    bz.focused = false;
}

static void respawn(void) {
    bz.alive = true;
    bz.px = BZZ_W / 2;
    bz.py = 160;
    bz.inv = BZZ_INV_T;
    bz.charge = 0;
}

static void fire_spread(void) {
    int s = bz.ship;
    int v = bz.volley++;
    if (s == BZ_LACEWING) {
        /* weaving pairs that sweep from side to side */
        float a = sinf(v * 0.55f) * 0.45f;
        for (int k = -1; k <= 1; k += 2) {
            int j = add_pshot(PS_WAVE, bz.px + k * 4, bz.py - 6, sinf(a) * 6.0f, -cosf(a) * 6.0f, 2, 3, 2);
            if (j >= 0) bz.ps[j].wob = (float)v;
        }
    } else if (s == BZ_SHIELDBUG) {
        int dmg = bz.power_t > 0 ? 4 : 2;
        for (int k = -3; k <= 3; k++) {
            float a = k * 0.35f;
            add_pshot(PS_BOLT, bz.px, bz.py - 6, sinf(a) * 5.5f, -cosf(a) * 5.5f, 2, 2, dmg);
        }
    } else {
        for (int k = -1; k <= 1; k++) {
            float a = k * 0.31f;
            add_pshot(PS_BOLT, bz.px + k * 3, bz.py - 7, sinf(a) * 7.0f, -cosf(a) * 7.0f, 2, 3, 2);
        }
    }
    /* options beside the ship fire with it */
    for (int i = 0; i < bz.nopt; i++) {
        Option *o = &bz.opt[i];
        if (s == BZ_LACEWING) add_pshot(PS_BOLT, o->x, o->y - 4, 0, -6.5f, 2, 3, 2);
        else if (s == BZ_SHIELDBUG)
            for (int k = -1; k <= 1; k++) add_pshot(PS_BOLT, o->x, o->y - 4, sinf(k * 0.4f) * 5.5f, -cosf(k * 0.4f) * 5.5f, 2, 2, bz.power_t > 0 ? 4 : 2);
        /* the firefly's option only fires while fire is held */
    }
    sfx_play_name("bzz_shot");
}

static void fire_focus(void) {
    int s = bz.ship;
    if (s == BZ_LACEWING) {
        add_pshot(PS_BOLT, bz.px - 3, bz.py - 7, 0, -7.5f, 2, 4, 3);
        add_pshot(PS_BOLT, bz.px + 3, bz.py - 7, 0, -7.5f, 2, 4, 3);
    } else if (s == BZ_SHIELDBUG) {
        bool pw = bz.power_t > 0;
        add_pshot(PS_CRESCENT, bz.px, bz.py - 8, 0, -6.5f, pw ? 12 : 8, 3, pw ? 14 : 7);
        for (int i = 0; i < bz.nopt; i++)
            add_pshot(PS_CRESCENT, bz.opt[i].x, bz.opt[i].y - 4, 0, -6.5f, 5, 2, pw ? 8 : 4);
    }
    sfx_play_name("bzz_shot2");
}

static void fire_lance(void) {
    float c = (float)bz.charge;
    int j = add_pshot(PS_LANCE, bz.px, bz.py - 14, 0, -7.0f, 3 + c / 30.0f, 10 + c / 10.0f, 8 + bz.charge / 5);
    if (j >= 0) bz.ps[j].lance = ++bz.lance_id;
    bz.charge = 0;
    sfx_play_name("bzz_lance");
}

void bzz_launch_bomb(void) {
    if (bz.bombs <= 0) return;
    for (int i = 0; i < BZZ_MAX_BOMBS; i++) {
        Bomb *b = &bz.bomb[i];
        if (b->alive) continue;
        *b = (Bomb){bz.px, bz.py - 8, -3.2f, 1, 0};
        bz.bombs--;
        sfx_play_name("bzz_bomb");
        return;
    }
}

/* the firefly's laser: straight up to the first foe in its way */
static void laser_update(void) {
    bz.laser_t++;
    int hit = -1;
    float top = -10;
    for (int i = 0; i < BZZ_MAX_FOES; i++) {
        Foe *e = &bz.foe[i];
        if (!e->alive || e->y > bz.py || (e->kind == EK_ROTWALL && !e->state)) continue;
        if (fabsf(e->x - bz.px) > e->hw + 3) continue;
        float fy = e->y + e->hh;
        if (fy > top) { top = fy; hit = i; }
    }
    bz.laser_len = (int)(bz.py - 8 - top);
    if (++bz.laser_tick >= 3) {
        bz.laser_tick = 0;
        if (hit >= 0) {
            bzz_hurt_foe(hit, 4);
            if (rng_chance(&g_rng, 50)) bzz_burst(bz.px, top, C_YELLOW, 1, 1.0f);
        }
    }
}

static void ship_update(void) {
    if (!bz.alive) {
        if (bz.dead_t > 0 && --bz.dead_t == 0) {
            if (bz.lives > 0) { bz.lives--; respawn(); }
            else {
                bz.state = BS_OVER;
                bz.state_t = 0;
                music_restart(BZZ_MUS_OVER);
            }
        }
        return;
    }
    if (bz.inv > 0) bz.inv--;
    bool a = btn(BTN_A), ap = btnp(BTN_A);
    bz.fire_hold = a ? bz.fire_hold + 1 : 0;
    bz.focused = bz.fire_hold >= BZZ_HOLD_T;

    /* moving: much faster while not holding fire */
    int dx = btn(BTN_RIGHT) - btn(BTN_LEFT), dy = btn(BTN_DOWN) - btn(BTN_UP);
    float sp = bz.focused ? BZZ_SLOW : BZZ_FAST;
    if (dx && dy) sp *= 0.7071f;
    bz.px = fclamp(bz.px + dx * sp, BZZ_MIN_X, BZZ_MAX_X);
    bz.py = fclamp(bz.py + dy * sp, BZZ_MIN_Y, BZZ_MAX_Y);

    /* firing */
    if (ap) {
        if (bz.ship == BZ_LACEWING && bz.charge >= BZZ_CHARGE_MIN) fire_lance();
        bz.charge = 0;
        bz.spread_win = BZZ_SPREAD_WIN;
        bz.spread_cd = 0;
    }
    if (a && !bz.focused) bz.spread_win = BZZ_SPREAD_WIN;
    if (bz.spread_cd > 0) bz.spread_cd--;
    if (bz.focus_cd > 0) bz.focus_cd--;
    if (bz.focused) {
        bz.spread_win = 0;
        if (bz.ship == BZ_FIREFLY) laser_update();
        else if (bz.focus_cd == 0) { fire_focus(); bz.focus_cd = bz.ship == BZ_LACEWING ? 6 : 8; }
    } else {
        bz.laser_t = 0;
        if (bz.spread_win > 0) {
            bz.spread_win--;
            if (bz.spread_cd == 0) { fire_spread(); bz.spread_cd = bz.ship == BZ_SHIELDBUG ? 9 : 6; }
        } else if (bz.ship == BZ_LACEWING && !a) {
            /* not firing at all: the lance builds */
            if (bz.charge < BZZ_CHARGE_MAX) bz.charge++;
        }
    }
    if (bz.ship == BZ_FIREFLY && btnp(BTN_B)) bzz_launch_bomb();
    if (bz.power_t > 0) bz.power_t--;
}

/* ------------------------------------------------------------------------ */
/* options, orbs, bombs, the allies                                          */

static void options_update(void) {
    for (int i = 0; i < bz.nopt; i++) {
        Option *o = &bz.opt[i];
        float side = i == 0 ? -1.0f : 1.0f;
        float tx = bz.px + side * 14, ty = bz.py + 4, k = 0.3f;
        if (o->fire_cd > 0) o->fire_cd--;
        if (o->touch_cd > 0) o->touch_cd--;
        if (bz.ship == BZ_LACEWING && bz.focused) {
            /* rushes out at the nearest foe and fights it up close */
            int f = nearest_foe(o->x, o->y, false);
            o->target = f;
            if (f >= 0) {
                Foe *e = &bz.foe[f];
                tx = e->x + side * 6;
                ty = e->y + e->hh + 6;
                float ddx = tx - o->x, ddy = ty - o->y, d = sqrtf(ddx * ddx + ddy * ddy);
                if (d > 3.2f) { tx = o->x + ddx / d * 3.2f; ty = o->y + ddy / d * 3.2f; }
                k = 1.0f;
                if (o->fire_cd == 0) { add_pshot(PS_BOLT, o->x, o->y - 4, 0, -6.5f, 2, 3, 2); o->fire_cd = 6; }
            }
        } else if (bz.ship == BZ_SHIELDBUG && bz.focused) {
            /* moves out in front as a shield */
            tx = bz.px + side * 7;
            ty = bz.py - 16;
            k = 0.4f;
        } else if (bz.ship == BZ_FIREFLY && bz.focused && o->fire_cd == 0) {
            int f = nearest_foe(o->x, o->y, false);
            int j = add_pshot(PS_ROCKET, o->x, o->y - 4, side * 1.0f, -3.0f, 2, 2, 8);
            if (j >= 0) bz.ps[j].target = f;
            o->fire_cd = 18;
        }
        o->x += (tx - o->x) * k;
        o->y += (ty - o->y) * k;
        /* bumping foes: the lacewing's option rams them, every option gets hurt */
        for (int j = 0; j < BZZ_MAX_FOES; j++) {
            Foe *e = &bz.foe[j];
            if (!e->alive || (e->kind == EK_ROTWALL && !e->state)) continue;
            if (fabsf(e->x - o->x) > e->hw + 3 || fabsf(e->y - o->y) > e->hh + 3) continue;
            if (o->touch_cd == 0) {
                bzz_hurt_foe(j, bz.ship == BZ_LACEWING ? 4 : 1);
                o->hp--;
                o->touch_cd = 8;
            }
        }
    }
    /* the broken ones go */
    int n = 0;
    for (int i = 0; i < bz.nopt; i++) {
        if (bz.opt[i].hp > 0) bz.opt[n++] = bz.opt[i];
        else { bzz_burst(bz.opt[i].x, bz.opt[i].y, C_LIGHT, 10, 1.4f); sfx_play_name("bzz_optlost"); }
    }
    bz.nopt = n;
}

static void orbs_pos(int i, float *x, float *y) {
    float a = bz.orb_a + i * 3.14159f;
    *x = bz.px + cosf(a) * 17;
    *y = bz.py + sinf(a) * 17;
}

static void allies_update(void) {
    bz.orb_a += 0.09f;
    /* the orbs break on a foe's body */
    for (int o = 0; o < 2; o++) {
        if (!bz.orb[o] || !bz.alive) continue;
        float ox, oy;
        orbs_pos(o, &ox, &oy);
        for (int j = 0; j < BZZ_MAX_FOES; j++) {
            Foe *e = &bz.foe[j];
            if (!e->alive || (e->kind == EK_ROTWALL && !e->state)) continue;
            if (fabsf(e->x - ox) < e->hw + 3 && fabsf(e->y - oy) < e->hh + 3) {
                bz.orb[o] = false;
                bzz_hurt_foe(j, 6);
                bzz_burst(ox, oy, C_CYAN, 10, 1.3f);
                sfx_play_name("bzz_optlost");
                break;
            }
        }
    }
    /* bombs, then their blasts */
    for (int i = 0; i < BZZ_MAX_BOMBS; i++) {
        Bomb *b = &bz.bomb[i];
        if (!b->alive) continue;
        b->y += b->vy;
        b->vy *= 0.97f;
        if (++b->t >= BOMB_FUSE || b->y < 14) {
            b->alive = 0;
            for (int k = 0; k < BZZ_MAX_BLASTS; k++)
                if (!bz.blast[k].alive) { bz.blast[k] = (Blast){b->x, b->y, 4, 0, 1}; break; }
            bz.shake = 10;
            sfx_play_name("bzz_blast");
        }
    }
    for (int i = 0; i < BZZ_MAX_BLASTS; i++) {
        Blast *b = &bz.blast[i];
        if (!b->alive) continue;
        b->t++;
        b->r = fminf(BLAST_R, 4 + b->t * 4.0f);
        for (int k = 0; k < BZZ_MAX_ESHOTS; k++) {
            EShot *s = &bz.es[k];
            if (s->alive && (s->x - b->x) * (s->x - b->x) + (s->y - b->y) * (s->y - b->y) < b->r * b->r) s->alive = 0;
        }
        if (b->t % 2 == 0)
            for (int k = 0; k < BZZ_MAX_FOES; k++) {
                Foe *e = &bz.foe[k];
                if (!e->alive) continue;
                float ddx = fmaxf(0, fabsf(e->x - b->x) - e->hw), ddy = fmaxf(0, fabsf(e->y - b->y) - e->hh);
                if (ddx * ddx + ddy * ddy < b->r * b->r) bzz_hurt_foe(k, 5);
            }
        if (b->t >= BLAST_T) b->alive = 0;
    }
    /* the lacewing's big ally rises under the ship and beams straight up */
    if (bz.hive.on) {
        bz.hive.t++;
        float want = bz.hive.t < HIVE_FIRE1 ? 158 : BZZ_H + 40;
        bz.hive.y += (want - bz.hive.y) * 0.12f;
        if (bz.hive.t >= HIVE_FIRE0 && bz.hive.t < HIVE_FIRE1 && bz.hive.t % 2 == 0)
            for (int k = 0; k < BZZ_MAX_FOES; k++) {
                Foe *e = &bz.foe[k];
                if (e->alive && e->y < bz.hive.y && fabsf(e->x - bz.hive.x) < e->hw + 7) bzz_hurt_foe(k, 4);
            }
        if (bz.hive.t >= HIVE_T) bz.hive.on = false;
    }
    /* the firefly's dragonfly: across the screen, nine shots at three angles */
    if (bz.fly.on) {
        bz.fly.t++;
        bz.fly.x += 2.8f;
        bz.fly.y = 62 + sinf(bz.fly.t * 0.08f) * 6;
        if (bz.fly.t == 20 || bz.fly.t == 48 || bz.fly.t == 76) {
            int f = nearest_foe(bz.fly.x, bz.fly.y, false);
            float a = f >= 0 ? atan2f(bz.foe[f].y - bz.fly.y, bz.foe[f].x - bz.fly.x) : -1.5708f;
            for (int k = -1; k <= 1; k++) {
                add_pshot(PS_ALLY, bz.fly.x, bz.fly.y, cosf(a + k * 0.38f) * 5.0f, sinf(a + k * 0.38f) * 5.0f, 3, 3, 15);
                bz.fly.fired++;
            }
            sfx_play_name("bzz_shot2");
        }
        for (int k = 0; k < BZZ_MAX_ESHOTS; k++) {
            EShot *s = &bz.es[k];
            if (s->alive && fabsf(s->x - bz.fly.x) < 11 && fabsf(s->y - bz.fly.y) < 7) s->alive = 0;
        }
        if (bz.fly.x > BZZ_W + 24) bz.fly.on = false;
    }
}

/* ------------------------------------------------------------------------ */
/* shots                                                                      */

static bool shot_hits(const PShot *s, const Foe *e) {
    return fabsf(s->x - e->x) < s->w + e->hw && fabsf(s->y - e->y) < s->h + e->hh;
}

static void pshots_update(void) {
    for (int i = 0; i < BZZ_MAX_PSHOTS; i++) {
        PShot *s = &bz.ps[i];
        if (!s->alive) continue;
        s->t++;
        if (s->kind == PS_WAVE) s->x += sinf(s->t * 0.45f + s->wob) * 0.9f;
        if (s->kind == PS_ROCKET) {
            if (s->target < 0 || !bz.foe[s->target].alive) s->target = nearest_foe(s->x, s->y, false);
            if (s->target >= 0) {
                Foe *e = &bz.foe[s->target];
                float a = atan2f(e->y - s->y, e->x - s->x);
                s->vx += cosf(a) * 0.5f;
                s->vy += sinf(a) * 0.5f;
            }
            float sp = sqrtf(s->vx * s->vx + s->vy * s->vy);
            if (sp > 5.0f) { s->vx *= 5.0f / sp; s->vy *= 5.0f / sp; }
        }
        s->x += s->vx;
        s->y += s->vy;
        if (s->y < -24 || s->y > BZZ_H + 16 || s->x < -16 || s->x > BZZ_W + 16 || s->t > 200) { s->alive = 0; continue; }
        for (int j = 0; j < BZZ_MAX_FOES && s->alive; j++) {
            Foe *e = &bz.foe[j];
            if (!e->alive || (e->kind == EK_ROTWALL && !e->state) || !shot_hits(s, e)) continue;
            if (s->kind == PS_LANCE) {
                if (e->lance == s->lance) continue; /* it pierces: each foe once */
                e->lance = s->lance;
                bzz_hurt_foe(j, s->dmg);
            } else {
                bzz_hurt_foe(j, s->dmg);
                s->alive = 0;
                bzz_burst(s->x, s->y, C_CREAM, 1, 0.8f);
            }
        }
    }
}

static bool ship_touches(float x, float y, float r) {
    float dx = x - bz.px, dy = y - bz.py;
    return dx * dx + dy * dy < (r + BZZ_HIT_R) * (r + BZZ_HIT_R);
}

static void eshots_update(void) {
    float ox[2], oy[2];
    for (int o = 0; o < 2; o++) orbs_pos(o, &ox[o], &oy[o]);
    for (int i = 0; i < BZZ_MAX_ESHOTS; i++) {
        EShot *s = &bz.es[i];
        if (!s->alive) continue;
        bzz_eshot_step(s, bz.px, bz.py);
        if (s->y < -20 || s->y > BZZ_H + 20 || s->x < -20 || s->x > BZZ_W + 20) { s->alive = 0; continue; }
        if (!bz.alive) continue;
        /* the orbs soak up any number of shots */
        bool gone = false;
        for (int o = 0; o < 2 && !gone; o++)
            if (bz.orb[o] && fabsf(s->x - ox[o]) < s->r + 3 && fabsf(s->y - oy[o]) < s->r + 3) gone = true;
        /* options soak them up too, at a cost */
        for (int k = 0; k < bz.nopt && !gone; k++) {
            Option *op = &bz.opt[k];
            if (fabsf(s->x - op->x) < s->r + 4 && fabsf(s->y - op->y) < s->r + 4) { op->hp--; gone = true; }
        }
        if (gone) { s->alive = 0; bzz_burst(s->x, s->y, C_ICE, 2, 0.7f); continue; }
        if (bz.inv == 0 && ship_touches(s->x, s->y, s->r)) { s->alive = 0; bzz_kill_ship(); }
    }
}

static void letters_update(void) {
    for (int i = 0; i < BZZ_MAX_LETTERS; i++) {
        Letter *l = &bz.lt[i];
        if (!l->alive) continue;
        l->t++;
        l->vy = fminf(l->vy + 0.06f, 0.9f);
        l->vx *= 0.96f;
        l->x += l->vx;
        l->y += l->vy;
        if (l->y > BZZ_H + 8) { l->alive = 0; continue; }
        if (bz.alive && fabsf(l->x - bz.px) < BZZ_PICK_R && fabsf(l->y - bz.py) < BZZ_PICK_R) {
            l->alive = 0;
            bzz_catch_letter(l->letter);
            bzz_burst(l->x, l->y, l->letter == LT_B ? C_AMBER : C_CYAN, 6, 1.0f);
        }
    }
}

static void bodies_update(void) {
    if (!bz.alive || bz.inv > 0) return;
    for (int i = 0; i < BZZ_MAX_FOES; i++) {
        Foe *e = &bz.foe[i];
        if (!e->alive || (e->kind == EK_ROTWALL && !e->state)) continue;
        if (fabsf(e->x - bz.px) < e->hw + BZZ_HIT_R && fabsf(e->y - bz.py) < e->hh + BZZ_HIT_R) { bzz_kill_ship(); return; }
    }
}

static void parts_update(void) {
    for (int i = 0; i < BZZ_MAX_PARTS; i++) {
        Part *p = &bz.part[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vx *= 0.94f;
        p->vy *= 0.94f;
    }
}

/* ------------------------------------------------------------------------ */
/* the run and the waves                                                     */

void bzz_new_run(int ship) {
    bzz_clear_world();
    memset(bz.part, 0, sizeof bz.part);
    bz.ship = ship;
    bz.score = 0;
    bz.mult = 1;
    bz.best_mult = 1;
    bz.lives = BZZ_START_SHIPS - 1;
    bz.extends = 0;
    bz.deaths = 0;
    bz.nword = 0;
    memset(bz.word, 0, sizeof bz.word);
    bz.drop_idx = 0;
    bz.kills = 0;
    bz.letters_caught = 0;
    bz.escaped = 0;
    bz.beacon_given = false;
    bz.won = false;
    bz.nopt = 0;
    bz.orb[0] = bz.orb[1] = false;
    bz.power_t = 0;
    bz.bombs = 0;
    bz.hive.on = bz.fly.on = false;
    bz.volley = 0;
    bz.lance_id = 0;
    bz.last_word = W_NONE;
    bz.word_t = 0;
    memset(bz.es_sum, 0, sizeof bz.es_sum);
    memset(bz.es_frames, 0, sizeof bz.es_frames);
    respawn();
    bz.inv = 0;
    if (bzs.runs < 65535) bzs.runs++;
    bzz_save_now();
    bzz_start_wave(0);
}

void bzz_start_wave(int w) {
    bz.wave = w;
    bz.wave_t = -120; /* the banner, then the first foes */
    bz.spawn_i = 0;
    bz.boss_out = bz.boss_dead = false;
    bz.boss_count = 0;
    bz.clear_t = 0;
    bz.time_bonus = 0;
    bz.kills_wave = 0;
    bz.pest_t = 0;
    memset(bz.foe, 0, sizeof bz.foe);
    memset(bz.es, 0, sizeof bz.es);
    memset(bz.lt, 0, sizeof bz.lt);
    bz.state = BS_PLAY;
    bz.state_t = 0;
    if (!bz.alive && bz.dead_t == 0) respawn();
    music_play(BZZ_MUS_WAVE[w]);
}

static void wave_done(void) {
    bz.state = BS_CLEAR;
    bz.state_t = 0;
    bz.bonus_shown = 0;
    uint32_t before = bz.score;
    bzz_add_score((uint32_t)bz.time_bonus); /* never multiplied */
    bz.bonus_given = (int)(bz.score - before);
    if (bz.wave + 1 > bzs.best_wave) bzs.best_wave = (uint8_t)(bz.wave + 1);
    music_restart(BZZ_MUS_CLEAR);
}

void bzz_play_update(void) {
    bz.frame_t++;
    bz.scroll += 3.0f;
    if (bz.shake > 0) bz.shake--;
    if (bz.flash > 0) bz.flash--;
    if (bz.word_t > 0) bz.word_t--;
    bz.wave_t++;
    if (bz.wave_t >= 0 && !bz.boss_dead) bzz_run_spawns();
    ship_update();
    if (bz.state != BS_PLAY) return;
    options_update();
    allies_update();
    bzz_foes_update();
    pshots_update();
    eshots_update();
    if (bz.wave_t >= 0) {
        int n = 0;
        for (int i = 0; i < BZZ_MAX_ESHOTS; i++) n += bz.es[i].alive;
        bz.es_sum[bz.wave] += n;
        bz.es_frames[bz.wave]++;
    }
    bodies_update();
    letters_update();
    parts_update();
    if (bz.boss_dead && ++bz.clear_t >= CLEAR_WAIT && bz.alive) wave_done();
}
