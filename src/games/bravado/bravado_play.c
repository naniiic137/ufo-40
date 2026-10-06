/* BRAVADO - a fight in the Glass Pit: Dice (and Domino in two-player), the
 * gun, bombs, the dash and the drone, lava, medkits, the spawn queue and
 * every collision. The monsters' own behaviour is in bravado_foes.c. */
#include "bravado.h"

BrvGame bv;
BrvSave bvs;

static const float DIAG = 0.70710678f;

/* the three pools every fight starts with (ours): left, right, and one
 * across the bottom wall that can only be walked round over the top */
static const Pool BASE_POOLS[3] = {
    {74, 58, 22, 11, 1, 0},
    {246, 116, 22, 11, 1, 0},
    {160, 163, 30, 10, 1, 0},
};
/* where a pack of slag can pour three more */
static const int16_t SLAG_SPOTS[][2] = {
    {58, 112}, {112, 40}, {208, 40}, {262, 62}, {104, 140}, {216, 142},
    {38, 84}, {284, 92}, {126, 112}, {196, 70}, {150, 34}, {262, 146},
    {90, 86}, {232, 92}, {170, 132}, {46, 140},
};

void brv_sfx(const char *name, int gap) {
    static int last[8];
    static const char *names[8];
    int slot = -1;
    for (int i = 0; i < 8; i++)
        if (names[i] == name) { slot = i; break; }
    if (slot < 0)
        for (int i = 0; i < 8; i++)
            if (!names[i]) { names[i] = name; slot = i; break; }
    if (slot < 0) { sfx_play_name(name); return; }
    if (bv.frame_t - last[slot] < gap && bv.frame_t >= last[slot]) return;
    last[slot] = bv.frame_t;
    sfx_play_name(name);
}

void brv_save_now(void) { game_save_write(game_current_index(), &bvs, (int)sizeof bvs); }

void brv_burst(float x, float y, int col, int n, float sp) {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < BRV_MAX_PARTS; i++) {
            Part *p = &bv.part[i];
            if (p->life > 0) continue;
            float a = rng_float(&g_rng) * 6.2831853f, s = sp * (0.3f + rng_float(&g_rng));
            *p = (Part){x, y, cosf(a) * s, sinf(a) * s, 14 + rng_range(&g_rng, 0, 12), col};
            break;
        }
    }
}

/* ---- the arena ------------------------------------------------------------ */

bool brv_in_lava(float x, float y) { return brv_pool_at(x, y) >= 0; }

int brv_pool_at(float x, float y) {
    for (int i = 0; i < BRV_MAX_POOLS; i++) {
        const Pool *p = &bv.pool[i];
        if (!p->alive || p->cooled) continue;
        float dx = (x - p->x) / p->rx, dy = (y - p->y) / p->ry;
        if (dx * dx + dy * dy < 1.0f) return i;
    }
    return -1;
}

static bool pool_clear(float x, float y) {
    for (int i = 0; i < BRV_MAX_POOLS; i++) {
        const Pool *p = &bv.pool[i];
        if (!p->alive) continue;
        if (fabsf(p->x - x) < p->rx + 20 && fabsf(p->y - y) < p->ry + 12) return false;
    }
    return true;
}

void brv_add_pools(int n) {
    int order[ARRAY_LEN(SLAG_SPOTS)];
    int m = ARRAY_LEN(SLAG_SPOTS);
    for (int i = 0; i < m; i++) order[i] = i;
    for (int i = m - 1; i > 0; i--) {
        int j = rng_range(&g_rng, 0, i), t = order[i];
        order[i] = order[j];
        order[j] = t;
    }
    for (int k = 0; k < m && n > 0; k++) {
        float x = SLAG_SPOTS[order[k]][0], y = SLAG_SPOTS[order[k]][1];
        if (!pool_clear(x, y)) continue;
        for (int i = 0; i < BRV_MAX_POOLS; i++) {
            if (bv.pool[i].alive) continue;
            bv.pool[i] = (Pool){x, y, (float)rng_range(&g_rng, 15, 19), (float)rng_range(&g_rng, 8, 10), 1, 0};
            n--;
            break;
        }
    }
}

/* the corner pads, top-left, top-right, bottom-left, bottom-right */
static void pad_pos(int i, float *x, float *y) {
    *x = (i & 1) ? BRV_AX1 - 16 : BRV_AX0 + 16;
    *y = (i & 2) ? BRV_AY1 - 14 : BRV_AY0 + 14;
}

/* ---- the run --------------------------------------------------------------- */

static void reset_player(Player *p, float x) {
    p->down = false;
    p->x = x;
    p->y = BRV_CY;
    p->face = 6;
    p->locked = false;
    p->maxhp = brv_max_health();
    p->hp = p->maxhp;
    p->inv = 30;
    p->hurt_t = 0;
    p->cd = 0;
    p->lava_t = 0;
    p->bombs = brv_stock_bombs();
    p->blast_sh = bv.gear[GR_BLASTGD];
    p->shot_sh = bv.gear[GR_SHOTGD];
    p->walks = bv.gear[GR_FIREWALK];
    p->tap_t = 0;
    p->fire_tap_t = 0;
    p->dash_t = p->dash_cd = 0;
    p->drone_on = bv.gear[GR_BUDDY] > 0;
    p->drone_hp = bv.gear[GR_BUDDY] >= 2 ? 6 : 3;
    p->drone_inv = 0;
    p->drone_cd = 0;
    p->dx = x;
    p->dy = BRV_CY;
    p->drone_face = p->face;
    p->echo_i = p->echo_n = 0;
}

void brv_new_run(int players) {
    int menu = bv.menu, reading = bv.dash_reading;
    memset(&bv, 0, sizeof bv);
    bv.menu = menu;
    bv.dash_reading = reading;
    bv.players = players;
    bv.sale = bv.hike = -1;
    bv.last_bought = -1;
    bv.p[0].on = true;
    bv.p[1].on = players == 2;
    bv.round = 0;
    bv.cash = 0;
    if (bvs.runs < 65535) bvs.runs++;
    brv_save_now();
    input_set_versus(players == 2);
    brv_new_lineup();
}

void brv_clear_fight(void) {
    memset(bv.foe, 0, sizeof bv.foe);
    memset(bv.es, 0, sizeof bv.es);
    memset(bv.ps, 0, sizeof bv.ps);
    memset(bv.bomb, 0, sizeof bv.bomb);
    memset(bv.blast, 0, sizeof bv.blast);
    memset(bv.med, 0, sizeof bv.med);
}

static int med_every(void) { return bv.gear[GR_MEDKIT] >= 2 ? 900 : bv.gear[GR_MEDKIT] == 1 ? 1800 : 0; }

void brv_start_fight(void) {
    brv_clear_fight();
    memset(bv.part, 0, sizeof bv.part);
    memset(bv.pool, 0, sizeof bv.pool);
    for (int i = 0; i < 3; i++) bv.pool[i] = BASE_POOLS[i];
    bv.nqueue = bv.qi = 0;
    for (int g = 0; g < bv.ngroups; g++) {
        const Group *gr = &bv.lineup[g];
        if (gr->kind == MK_SLAG) { brv_add_pools(3); continue; }
        for (int k = 0; k < gr->count && bv.nqueue < BRV_MAX_QUEUE; k++) bv.queue[bv.nqueue++] = gr->kind;
    }
    if (bv.round == BRV_ROUNDS - 1) bv.queue[bv.nqueue++] = MK_BOSS;
    bool two = bv.p[1].on;
    reset_player(&bv.p[0], two ? BRV_CX - 14 : BRV_CX);
    if (two) reset_player(&bv.p[1], BRV_CX + 14);
    bv.spawn_t = 60;
    bv.med_t = med_every();
    bv.fight_t = 0;
    bv.boss_out = bv.boss_dead = false;
    bv.fight_kills = 0;
    bv.peeper_tone_t = 0;
}

/* ---- foes: spawning, hurting, killing -------------------------------------- */

int brv_spawn_foe(int kind, float x, float y) {
    for (int i = 0; i < BRV_MAX_FOES; i++) {
        Foe *f = &bv.foe[i];
        if (f->alive) continue;
        memset(f, 0, sizeof *f);
        f->alive = 1;
        f->kind = (uint8_t)kind;
        f->x = x;
        f->y = y;
        f->hp = BRV_KIND_HP[kind];
        f->dir = rng_range(&g_rng, 0, 7);
        brv_foe_init(f);
        return i;
    }
    return -1;
}

/* counted: the ones that hold the fight open (the boss's fizzers don't) */
int brv_alive_foes(bool counted_only) {
    int n = 0;
    for (int i = 0; i < BRV_MAX_FOES; i++)
        if (bv.foe[i].alive && (!counted_only || bv.foe[i].kind != MK_FIZZER)) n++;
    return n;
}

int brv_enemies_left(void) { return (bv.nqueue - bv.qi) + brv_alive_foes(true); }

/* explosions waiting to go off (a keg killed by a blast blows up next) */
typedef struct { float x, y, r; int dmg, boss; } PendingBoom;
static PendingBoom pend[32];
static int npend;
static float last_shot_vx, last_shot_vy;

static void queue_boom(float x, float y, float r, int dmg) {
    if (npend < ARRAY_LEN(pend)) pend[npend++] = (PendingBoom){x, y, r, dmg, 40};
}

void brv_kill_foe(int i, int how) {
    Foe *f = &bv.foe[i];
    if (!f->alive) return;
    f->alive = 0;
    float x = f->x, y = f->y;
    int kind = f->kind;
    if (how != KILL_QUIET && kind != MK_FIZZER) {
        bv.kills++;
        bv.fight_kills++;
    }
    int size = f->size; /* f's slot may be handed straight to a split half */
    if (kind == MK_GASBAG && size < 2 && how == KILL_SHOT) {
        /* splits in two, off at 45 degrees either side of the shot */
        float a = atan2f(last_shot_vy, last_shot_vx);
        for (int s = -1; s <= 1; s += 2) {
            float b = a + (float)s * 0.7853982f;
            int c = brv_spawn_foe(MK_GASBAG, x + cosf(b) * 4, y + sinf(b) * 4);
            if (c < 0) continue;
            Foe *g = &bv.foe[c];
            g->size = (uint8_t)(size + 1);
            g->hp = BRV_KIND_HP[MK_GASBAG];
            g->vx = cosf(b) * 1.3f;
            g->vy = sinf(b) * 1.3f;
            g->state = 2; /* flung */
            g->st = 22;
            g->inv = 6;
        }
        brv_burst(x, y, C_VIOLET, 6, 1.2f);
        brv_sfx("brv_pop", 3);
        return;
    }
    if (kind == MK_KEG) queue_boom(x, y, BRV_KEG_R, BRV_KEG_HIT);
    if (kind == MK_FIZZER && how != KILL_QUIET) queue_boom(x, y, BRV_FIZZ_R, BRV_HIT);
    if (kind == MK_BOSS) {
        bv.boss_dead = true;
        for (int k = 0; k < BRV_MAX_FOES; k++)
            if (bv.foe[k].alive && bv.foe[k].kind == MK_FIZZER) { bv.foe[k].alive = 0; brv_burst(bv.foe[k].x, bv.foe[k].y, C_YELLOW, 6, 1.0f); }
        for (int k = 0; k < BRV_MAX_ESHOTS; k++) bv.es[k].alive = 0;
        brv_burst(x, y, C_RED, 40, 2.5f);
        brv_burst(x, y, C_YELLOW, 30, 1.8f);
        bv.shake = 30;
        gfx_set_flash(6);
        brv_sfx("brv_bossdie", 0);
        return;
    }
    static const uint8_t COL[MK_ALL] = {C_ORANGE, C_VIOLET, C_BROWN, C_AMBER, C_SKY, C_LIME, C_RED, C_RED, C_YELLOW};
    brv_burst(x, y, COL[kind], 8, 1.4f);
    if (kind != MK_KEG && kind != MK_FIZZER) brv_sfx("brv_pop", 2);
}

static float foe_radius(const Foe *f) {
    switch (f->kind) {
    case MK_MITE: return 4;
    case MK_GASBAG: return f->size == 0 ? 7 : f->size == 1 ? 5 : 3.5f;
    case MK_BRUTE: return 6;
    case MK_KEG: return 5;
    case MK_STILTER: return 5;
    case MK_PEEPER: return 6;
    case MK_BOSS: return 14;
    default: return 4;
    }
}

/* a peeper underground, or anything still arriving on a pad, can't be hit */
static bool foe_solid(const Foe *f) {
    if (!f->alive || f->warp > 0) return false;
    if (f->kind == MK_PEEPER) return f->state == 2;
    return true;
}

static void hurt_foe(int i, int dmg, int how, float push_x, float push_y) {
    Foe *f = &bv.foe[i];
    if (!foe_solid(f) || f->inv > 0) return;
    f->hp -= dmg;
    f->inv = 4;
    f->flash = 4;
    if (f->kind != MK_BOSS && f->kind != MK_PEEPER) {
        f->x = fclamp(f->x + push_x, BRV_AX0 + 4, BRV_AX1 - 4);
        f->y = fclamp(f->y + push_y, BRV_AY0 + 4, BRV_AY1 - 4);
    }
    if (f->hp <= 0) brv_kill_foe(i, how);
    else brv_sfx("brv_tick", 3);
}

/* a blast catches the whole of her, head and all */
static void blast_players(float x, float y, float r, int dmg) {
    if (dmg <= 0) return;
    for (int w = 0; w < 2; w++) {
        Player *p = &bv.p[w];
        if (!p->on || p->down) continue;
        float dx = fmaxf(fabsf(p->x - x) - 4, 0), dy = fmaxf(fabsf(p->y - 1 - y) - 6, 0);
        if (dx * dx + dy * dy <= r * r) { bv.hurt_src = 300; brv_hurt_player(w, dmg, HURT_BLAST); }
    }
}

void brv_explode(float x, float y, float r, int dmg_player, bool hurts_player, int boss_dmg) {
    if (npend < ARRAY_LEN(pend)) pend[npend++] = (PendingBoom){x, y, r, hurts_player ? dmg_player : 0, boss_dmg};
    int guard = 0;
    while (npend > 0 && guard++ < 200) {
        PendingBoom b = pend[--npend];
        for (int k = 0; k < BRV_MAX_BLASTS; k++)
            if (!bv.blast[k].alive) { bv.blast[k] = (Blast){b.x, b.y, b.r, 0, b.dmg, 1}; break; }
        brv_burst(b.x, b.y, C_AMBER, 10, 2.0f);
        bv.shake = imax(bv.shake, 8);
        brv_sfx("brv_boom", 2);
        for (int i = 0; i < BRV_MAX_FOES; i++) {
            Foe *f = &bv.foe[i];
            if (!foe_solid(f)) continue;
            float dx = f->x - b.x, dy = f->y - b.y, rr = b.r + foe_radius(f);
            if (dx * dx + dy * dy > rr * rr) continue;
            if (f->kind == MK_BOSS) {
                if (f->inv == 0) {
                    f->hp -= b.boss;
                    f->inv = 4;
                    f->flash = 6;
                    if (f->hp <= 0) brv_kill_foe(i, KILL_BLAST);
                }
            } else {
                brv_kill_foe(i, KILL_BLAST);
            }
        }
        blast_players(b.x, b.y, b.r, b.dmg);
    }
    npend = 0;
}

/* ---- the players -------------------------------------------------------------- */

void brv_hurt_player(int who, int dmg, int how) {
    Player *p = &bv.p[who];
    if (!p->on || p->down || bv.god) return;
    if (p->dash_t > 0) return;
    if (how != HURT_LAVA && p->inv > 0) return;
    if (how == HURT_BLAST && p->blast_sh > 0) {
        p->blast_sh--;
        p->inv = 30;
        brv_sfx("brv_guard", 0);
        return;
    }
    if (how == HURT_SHOT && p->shot_sh > 0) {
        p->shot_sh--;
        p->inv = 30;
        brv_sfx("brv_guard", 0);
        return;
    }
    p->hp -= dmg;
    p->hurt_t = 16;
    if (how != HURT_LAVA) p->inv = BRV_INV_T;
    if (p->hp <= 0) {
        p->hp = 0;
        p->down = true;
        p->dash_t = 0;
        brv_burst(p->x, p->y, C_RED, 24, 2.0f);
        brv_burst(p->x, p->y, C_WHITE, 12, 1.4f);
        bv.shake = 20;
        brv_sfx("brv_die", 0);
    } else {
        brv_sfx(how == HURT_LAVA ? "brv_sizzle" : "brv_hurt", 4);
    }
}

static int dir_of(int mx, int my) {
    for (int d = 0; d < 8; d++)
        if (BRV_DX[d] == mx && BRV_DY[d] == my) return d;
    return -1;
}

static void add_pshot(int kind, float x, float y, float vx, float vy, int dmg, int bounces, int owner, int life) {
    for (int i = 0; i < BRV_MAX_PSHOTS; i++) {
        PShot *s = &bv.ps[i];
        if (s->alive) continue;
        *s = (PShot){x, y, vx, vy, 1, (uint8_t)kind, (uint8_t)bounces, (uint8_t)owner, dmg, life};
        return;
    }
}

static void fire(Player *p, int who, float x, float y, int face, bool drone) {
    int n = 1 + bv.gear[GR_FAN];
    float base = atan2f((float)BRV_DY[face], (float)BRV_DX[face]);
    static const float OFFS[3][3] = {{0, 0, 0}, {-0.09f, 0.09f, 0}, {-0.17f, 0, 0.17f}};
    int dmg = 2 + bv.gear[GR_HEAVY];
    for (int k = 0; k < n; k++) {
        float a = base + OFFS[n - 1][k];
        add_pshot(drone ? PS_DRONE : PS_GUN, x + cosf(a) * 5, y - 1 + sinf(a) * 5, cosf(a) * 4.0f, sinf(a) * 4.0f, dmg,
                  bv.gear[GR_RICO], who, 200);
    }
    (void)p;
    brv_sfx("brv_shot", 3);
}

int brv_drop_bomb(int who, float x, float y, bool drone) {
    for (int i = 0; i < BRV_MAX_BOMBS; i++) {
        Bomb *b = &bv.bomb[i];
        if (b->alive) continue;
        *b = (Bomb){x, y + 3, BRV_FUSE, 0, 1, (uint8_t)who, (uint8_t)drone};
        brv_sfx("brv_drop", 0);
        return i;
    }
    return -1;
}

static void bomb_button(Player *p, int who, float x, float y) {
    if (bv.gear[GR_CLICKER]) {
        bool any = false;
        for (int i = 0; i < BRV_MAX_BOMBS; i++) {
            Bomb *b = &bv.bomb[i];
            if (b->alive && b->owner == who && !b->drone && b->age >= 10) { b->fuse = 0; any = true; }
        }
        if (any) { brv_sfx("brv_click", 0); return; }
    }
    if (p->bombs <= 0) { brv_sfx("ui_error", 6); return; }
    p->bombs--;
    p->bomb_now = true;
    brv_drop_bomb(who, x, y, false);
}

static void start_dash(Player *p) {
    if (p->dash_cd > 0) return;
    p->dash_t = BRV_DASH_T;
    p->dash_face = p->face;
    p->dash_id = ++bv.dash_counter;
    p->dash_cd = BRV_DASH_T + BRV_DASH_CD;
    p->dashes++;
    brv_sfx("brv_dash", 0);
}

static void clamp_player(Player *p) {
    p->x = fclamp(p->x, BRV_AX0 + 5, BRV_AX1 - 5);
    p->y = fclamp(p->y, BRV_AY0 + 7, BRV_AY1 - 5);
}

static void player_update(int who) {
    Player *p = &bv.p[who];
    if (!p->on || p->down) return;
    bool two = who == 1;
    int held = 0, pressed = 0;
    static const int BTNS[6] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B};
    for (int k = 0; k < 6; k++) {
        if (two ? btn2(BTNS[k]) : btn(BTNS[k])) held |= BTNS[k];
        if (two ? btnp2(BTNS[k]) : btnp(BTNS[k])) pressed |= BTNS[k];
    }
    p->shot_now = p->bomb_now = false;
    if (p->inv > 0) p->inv--;
    if (p->hurt_t > 0) p->hurt_t--;
    if (p->cd > 0) p->cd--;
    if (p->dash_cd > 0) p->dash_cd--;
    if (p->fire_tap_t > 0) p->fire_tap_t++;
    if (p->fire_tap_t > BRV_TAP_WIN) p->fire_tap_t = 0;

    int mx = ((held & BTN_RIGHT) != 0) - ((held & BTN_LEFT) != 0);
    int my = ((held & BTN_DOWN) != 0) - ((held & BTN_UP) != 0);
    p->moving = mx || my;

    if (p->dash_t > 0) {
        /* the dash: straight on, safe, through lava, flattening things */
        p->dash_t--;
        p->x += BRV_DX[p->dash_face] * BRV_DASH_SPEED * (BRV_DX[p->dash_face] && BRV_DY[p->dash_face] ? DIAG : 1.0f);
        p->y += BRV_DY[p->dash_face] * BRV_DASH_SPEED * (BRV_DX[p->dash_face] && BRV_DY[p->dash_face] ? DIAG : 1.0f);
        clamp_player(p);
        if (bv.frame_t % 2 == 0) brv_burst(p->x, p->y, C_CYAN, 1, 0.3f);
        if (p->dash_t == 0) p->inv = imax(p->inv, 6);
    } else {
        bool fire_held = (held & BTN_A) != 0;
        /* the facing follows the pad unless fire is held, and was already */
        if (p->moving && !(fire_held && p->locked)) p->face = dir_of(mx, my);
        p->locked = fire_held;
        float sp = BRV_PLAYER_SPEED * (mx && my ? DIAG : 1.0f);
        p->x += (float)mx * sp;
        p->y += (float)my * sp;
        clamp_player(p);
        if (p->moving) p->step++;
        if ((pressed & BTN_A) || (fire_held && p->cd == 0)) {
            if (p->cd == 0) {
                fire(p, who, p->x, p->y, p->face, false);
                p->shot_now = true;
                p->fired++;
                p->cd = bv.gear[GR_TRIGGER] >= 2 ? 7 : bv.gear[GR_TRIGGER] == 1 ? 10 : 14;
            }
        }
        /* the dash: fire tapped twice on the move (TV Tropes' reading; the
         * first tap is an ordinary shot). The wiki's reading, the bomb
         * button tapped twice, is kept for the tests only: there the first
         * tap is an ordinary bomb. */
        bool dash_b = bv.dash_reading == 1;
        if (bv.gear[GR_DASH] && !dash_b && (pressed & BTN_A)) {
            if (p->fire_tap_t > 0 && p->moving) { p->fire_tap_t = 0; start_dash(p); }
            else p->fire_tap_t = 1;
        }
        if (pressed & BTN_B) {
            if (bv.gear[GR_DASH] && dash_b && p->tap_t > 0) {
                p->tap_t = 0;
                start_dash(p);
            } else {
                bomb_button(p, who, p->x, p->y);
                if (bv.gear[GR_DASH] && dash_b) p->tap_t = 1;
            }
        }
    }
    if (p->tap_t > 0 && ++p->tap_t > BRV_TAP_WIN) p->tap_t = 0;

    /* lava */
    int pool = p->dash_t > 0 ? -1 : brv_pool_at(p->x, p->y + 4);
    if (pool >= 0 && p->walks > 0) {
        p->walks--;
        bv.pool[pool].cooled = 1;
        brv_burst(bv.pool[pool].x, bv.pool[pool].y, C_GREY, 14, 1.0f);
        brv_sfx("brv_sizzle", 0);
        pool = -1;
    }
    if (pool >= 0) {
        /* a bite as you step in, then one every BRV_LAVA_EVERY frames */
        if (--p->lava_t <= 0) {
            bv.hurt_src = 400;
            brv_hurt_player(who, 1, HURT_LAVA);
            p->lava_t = BRV_LAVA_EVERY;
        }
        if (bv.frame_t % 6 == 0) brv_burst(p->x, p->y + 4, C_ORANGE, 1, 0.6f);
    } else {
        p->lava_t = 0;
    }

    /* medkits */
    for (int i = 0; i < BRV_MAX_MEDS; i++) {
        Medkit *m = &bv.med[i];
        if (!m->alive || p->down) continue;
        if (fabsf(m->x - p->x) < 8 && fabsf(m->y - p->y) < 9) {
            m->alive = 0;
            if (p->hp < BRV_MED_CAP) p->hp = imin(BRV_MED_CAP, p->hp + BRV_MED_HEAL);
            brv_burst(m->x, m->y, C_WHITE, 8, 1.0f);
            brv_sfx("brv_heal", 0);
        }
    }

    /* the drone: what its owner did, BRV_DRONE_LAG frames ago */
    Echo e = {p->x, p->y, (uint8_t)p->face, p->shot_now, p->bomb_now};
    if (p->drone_on) {
        if (p->echo_n == BRV_DRONE_LAG) {
            Echo old = p->echo[p->echo_i];
            p->dx = old.x;
            p->dy = old.y;
            p->drone_face = old.face;
            if (old.shot) fire(p, who, p->dx, p->dy, old.face, true);
            if (old.bomb) brv_drop_bomb(who, p->dx, p->dy, true);
        }
        p->echo[p->echo_i] = e;
        p->echo_i = (p->echo_i + 1) % BRV_DRONE_LAG;
        if (p->echo_n < BRV_DRONE_LAG) p->echo_n++;
        if (p->drone_inv > 0) p->drone_inv--;
    }
}

static void drone_hit(Player *p) {
    if (p->drone_inv > 0) return;
    p->drone_hp--;
    p->drone_inv = 20;
    if (p->drone_hp <= 0) {
        p->drone_on = false;
        brv_burst(p->dx, p->dy, C_LIGHT, 12, 1.4f);
        brv_sfx("brv_pop", 0);
    } else {
        brv_sfx("brv_tick", 3);
    }
}

/* ---- shots, bombs, blasts ------------------------------------------------------ */

static void pshots_update(void) {
    for (int i = 0; i < BRV_MAX_PSHOTS; i++) {
        PShot *s = &bv.ps[i];
        if (!s->alive) continue;
        if (--s->t <= 0) { s->alive = 0; continue; }
        s->x += s->vx;
        s->y += s->vy;
        bool wall = false;
        if (s->x < BRV_AX0 || s->x > BRV_AX1) { wall = true; s->vx = -s->vx; s->x = fclamp(s->x, BRV_AX0, BRV_AX1); }
        if (s->y < BRV_AY0 || s->y > BRV_AY1) { wall = true; s->vy = -s->vy; s->y = fclamp(s->y, BRV_AY0, BRV_AY1); }
        if (wall) {
            if (s->kind == PS_NAIL || s->bounces == 0) { s->alive = 0; continue; }
            s->bounces--;
        }
        float push = bv.gear[GR_KICK] ? 4.0f : 1.5f;
        for (int k = 0; k < BRV_MAX_FOES; k++) {
            Foe *f = &bv.foe[k];
            if (!foe_solid(f)) continue;
            float r = foe_radius(f) + 2;
            if (fabsf(f->x - s->x) > r || fabsf(f->y - s->y) > r) continue;
            if (f->inv > 0) { if (s->kind != PS_NAIL) s->alive = 0; break; }
            float l = sqrtf(s->vx * s->vx + s->vy * s->vy);
            last_shot_vx = s->vx;
            last_shot_vy = s->vy;
            hurt_foe(k, s->dmg, KILL_SHOT, s->vx / l * push, s->vy / l * push);
            s->alive = 0;
            break;
        }
    }
}

static void eshots_update(void) {
    for (int i = 0; i < BRV_MAX_ESHOTS; i++) {
        EShot *s = &bv.es[i];
        if (!s->alive) continue;
        s->t++;
        s->x += s->vx;
        s->y += s->vy;
        if (s->x < BRV_AX0 - 2 || s->x > BRV_AX1 + 2 || s->y < BRV_AY0 - 2 || s->y > BRV_AY1 + 2) { s->alive = 0; continue; }
        for (int w = 0; w < 2 && s->alive; w++) {
            Player *p = &bv.p[w];
            if (!p->on || p->down) continue;
            if (p->drone_on && fabsf(p->dx - s->x) < 4 && fabsf(p->dy - BRV_DRONE_UP - s->y) < 4) {
                s->alive = 0;
                drone_hit(p);
                break;
            }
            if (fabsf(p->x - s->x) < 5 && s->y - p->y > -8 && s->y - p->y < 6) {
                if (p->dash_t > 0) continue;
                if (p->inv > 0) continue;
                s->alive = 0;
                bv.hurt_src = 200 + s->kind;
                brv_hurt_player(w, BRV_HIT, HURT_SHOT);
            }
        }
    }
}

int brv_add_eshot(int kind, float x, float y, float vx, float vy) {
    for (int i = 0; i < BRV_MAX_ESHOTS; i++) {
        EShot *s = &bv.es[i];
        if (s->alive) continue;
        *s = (EShot){x, y, vx, vy, 1, (uint8_t)kind, 0};
        return i;
    }
    return -1;
}

static void bombs_update(void) {
    for (int i = 0; i < BRV_MAX_BOMBS; i++) {
        Bomb *b = &bv.bomb[i];
        if (!b->alive) continue;
        b->age++;
        if (b->fuse-- > 0) continue;
        b->alive = 0;
        brv_explode(b->x, b->y, BRV_BLAST_R, BRV_HIT, true, 40);
        int nails = bv.gear[GR_NAILS];
        if (nails) {
            int n = nails >= 2 ? 14 : 8, dmg = nails >= 2 ? 8 : 4;
            for (int k = 0; k < n; k++) {
                float a = 6.2831853f * (float)k / (float)n + 0.2f;
                add_pshot(PS_NAIL, b->x, b->y, cosf(a) * 3.0f, sinf(a) * 3.0f, dmg, 0, b->owner, 26);
            }
        }
    }
    /* a blast stays deadly for a moment: walk (or dash) into it and it
     * still hurts; monsters that wander in go too (the boss only takes
     * its first hit) */
    for (int i = 0; i < BRV_MAX_BLASTS; i++) {
        Blast *bl = &bv.blast[i];
        if (!bl->alive) continue;
        if (++bl->t > 18) { bl->alive = 0; continue; }
        if (bl->t >= BRV_BLAST_LINGER) continue;
        blast_players(bl->x, bl->y, bl->r, bl->dmg);
        for (int k = 0; k < BRV_MAX_FOES; k++) {
            Foe *f = &bv.foe[k];
            if (!foe_solid(f) || f->kind == MK_BOSS) continue;
            float dx = f->x - bl->x, dy = f->y - bl->y, rr = bl->r + foe_radius(f);
            if (dx * dx + dy * dy <= rr * rr) brv_kill_foe(k, KILL_BLAST);
        }
    }
}

/* ---- contact ---------------------------------------------------------------- */

static void contact(void) {
    for (int w = 0; w < 2; w++) {
        Player *p = &bv.p[w];
        if (!p->on || p->down) continue;
        for (int i = 0; i < BRV_MAX_FOES; i++) {
            Foe *f = &bv.foe[i];
            if (!foe_solid(f)) continue;
            float r = foe_radius(f) + 3;
            if (p->drone_on && fabsf(f->x - p->dx) < r && fabsf(f->y - (p->dy - BRV_DRONE_UP)) < r && f->kind != MK_BOSS) drone_hit(p);
            /* a dash sweeps a wider lane than her body */
            if (p->dash_t > 0) r = foe_radius(f) + 9;
            if (fabsf(f->x - p->x) > r || fabsf(f->y - p->y) > r + 1) continue;
            if (p->dash_t > 0) {
                if (f->kind == MK_BOSS) {
                    if (f->dash_id != p->dash_id) {
                        f->dash_id = p->dash_id;
                        f->hp -= BRV_DASH_BOSS;
                        f->flash = 8;
                        bv.shake = 10;
                        brv_sfx("brv_bonk", 0);
                        if (f->hp <= 0) brv_kill_foe(i, KILL_DASH);
                    }
                } else {
                    brv_kill_foe(i, KILL_DASH);
                }
                continue;
            }
            if (f->kind == MK_FIZZER) {
                /* runs into you and goes off */
                brv_kill_foe(i, KILL_SHOT);
                continue;
            }
            bv.hurt_src = 100 + f->kind;
            brv_hurt_player(w, BRV_HIT, HURT_HIT);
        }
    }
}

/* shot kills of kegs and fizzers queue their blasts: set them off */
static void flush_booms(void) {
    if (!npend) return;
    PendingBoom b = pend[--npend];
    brv_explode(b.x, b.y, b.r, b.dmg, true, b.boss);
}

/* ---- spawning ---------------------------------------------------------------- */

static int target_player(void) {
    int alive[2], n = 0;
    for (int w = 0; w < 2; w++)
        if (bv.p[w].on && !bv.p[w].down) alive[n++] = w;
    return n ? alive[rng_range(&g_rng, 0, n - 1)] : 0;
}

static void spawn_next(void) {
    int kind = bv.queue[bv.qi++];
    if (kind == MK_PEEPER) {
        /* sprouts right under you */
        Player *p = &bv.p[target_player()];
        brv_spawn_foe(MK_PEEPER, p->x, p->y + 2);
        return;
    }
    if (kind == MK_BOSS) {
        int i = brv_spawn_foe(MK_BOSS, BRV_CX, BRV_AY0 + 22);
        if (i >= 0) bv.foe[i].warp = 60;
        bv.boss_out = true;
        brv_sfx("brv_bossin", 0);
        music_play(BRV_MUS_BOSS);
        return;
    }
    /* from one of the two pads farthest from player 1 */
    const Player *p = &bv.p[bv.p[0].down && bv.p[1].on ? 1 : 0];
    float best[4];
    int order[4] = {0, 1, 2, 3};
    for (int i = 0; i < 4; i++) {
        float x, y;
        pad_pos(i, &x, &y);
        best[i] = (x - p->x) * (x - p->x) + (y - p->y) * (y - p->y);
    }
    for (int a = 0; a < 4; a++)
        for (int b = a + 1; b < 4; b++)
            if (best[order[b]] > best[order[a]]) { int t = order[a]; order[a] = order[b]; order[b] = t; }
    float x, y;
    pad_pos(order[rng_range(&g_rng, 0, 1)], &x, &y);
    int i = brv_spawn_foe(kind, x + (float)rng_range(&g_rng, -3, 3), y + (float)rng_range(&g_rng, -3, 3));
    if (i >= 0) bv.foe[i].warp = 30;
    brv_sfx("brv_warp", 8);
}

static void spawning(void) {
    if (bv.no_spawn || bv.qi >= bv.nqueue) return;
    if (--bv.spawn_t > 0) return;
    int next = bv.queue[bv.qi];
    if (next == MK_BOSS || brv_alive_foes(false) < BRV_ON_SCREEN) {
        spawn_next();
        bv.spawn_t = brv_spawn_interval(bv.prize);
    } else {
        bv.spawn_t = 1; /* the floor is full: the next waits for a gap */
    }
}

static void medkits(void) {
    int every = med_every();
    if (!every) return;
    if (--bv.med_t > 0) return;
    bv.med_t = every;
    for (int tries = 0; tries < 30; tries++) {
        float x = (float)rng_range(&g_rng, BRV_AX0 + 20, BRV_AX1 - 20), y = (float)rng_range(&g_rng, BRV_AY0 + 16, BRV_AY1 - 14);
        if (brv_in_lava(x, y + 3)) continue;
        for (int i = 0; i < BRV_MAX_MEDS; i++)
            if (!bv.med[i].alive) { bv.med[i] = (Medkit){x, y, 0, 1}; brv_sfx("brv_medin", 0); return; }
        return;
    }
}

/* ---- one frame of a fight ---------------------------------------------------- */

void brv_fight_update(void) {
    bv.fight_t++;
    if (bv.shake > 0) bv.shake--;
    if (bv.peeper_tone_t > 0) bv.peeper_tone_t--;
    for (int w = 0; w < 2; w++) player_update(w);
    spawning();
    medkits();
    for (int i = 0; i < BRV_MAX_MEDS; i++) if (bv.med[i].alive) bv.med[i].t++;
    brv_foes_update();
    pshots_update();
    flush_booms();
    eshots_update();
    bombs_update();
    flush_booms();
    contact();
    flush_booms();
    for (int i = 0; i < BRV_MAX_PARTS; i++) {
        Part *p = &bv.part[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vx *= 0.92f;
        p->vy *= 0.92f;
    }
}
