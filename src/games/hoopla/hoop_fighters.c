/* HOOPLA - the eight fighters. Every one walks, blocks (hold DOWN) and
 * lunges (DOWN + A); B is their own way of moving and A their own weapon,
 * with a second version on UP + A:
 *
 *   TANSY    B jumps, and once more in the air in a spin that turns knives;
 *            A a knife that bounces off two walls (and can cut her);
 *            UP+A a short knife that drops
 *   CLAMP    B shoots a claw line (diagonal while LEFT/RIGHT is held) that
 *            bites ledges and the ceiling, then swings; the first swing of a
 *            diagonal line is a melee; B again lets go; the claw takes hoops
 *            A a cog that comes back below, UP+A above. No lunge: DOWN+A is A
 *   MOSS     B turns his gravity over; A a rocket that curves with his
 *            gravity and bursts; UP+A one that doesn't burst
 *   BRISTLE  B sets a spring trap (armed in 1 s: it throws her up, and hurts
 *            anyone else); A three quills, up on the ground, down in the air,
 *            which stop her in the air; a long flat lunge
 *   PEWIT    B jumps, then flaps in the air for as long as you like; A a
 *            horseshoe in an arc, UP+A higher; a lunge in the air leaves her
 *            hovering
 *   COLLIER  B calls his cage lift up (DOWN+B down); it stops level with a
 *            ledge; B again gets off with some speed; A a charge on a 2 s
 *            fuse, held: a shorter fuse and a shield; UP+A thrown high
 *   ASTRA    B held fires her rocket pack, harder the longer it burns; A a
 *            ray down and ahead that bounces once, UP+A up and ahead
 *   GULP     B held charges a leap (LEFT/RIGHT lean it); after 1 s he glows
 *            and the leap is an uppercut to its top; A a dart, held to aim it
 *            (up on the ground, down in the air); UP+A a fast dart that
 *            sticks in walls and locks his darts for 2 s. DOWN+A is A */
#include "hoop.h"

#define PRESSED(f, b) (((f)->held & (b)) && !((f)->prev & (b)))
#define HELD(f, b) (((f)->held & (b)) != 0)
#define RELEASED(f, b) (!((f)->held & (b)) && ((f)->prev & (b)))

/* each shot's box (w, h) and how it ranks when two meet */
static const uint8_t SHOT_W[SH_KINDS] = {8, 6, 8, 8, 8, 4, 7, 6, 6, 7, 8};
static const uint8_t SHOT_H[SH_KINDS] = {3, 3, 8, 4, 4, 4, 6, 6, 6, 3, 3};
static const uint8_t SHOT_PRIO[SH_KINDS] = {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 2};

static bool grounded(const Fighter *f) { return f->ground != GND_AIR; }

static int approach(int v, int target, int step) {
    if (v < target) return v + step > target ? target : v + step;
    return v - step < target ? target : v - step;
}

static int count_shots(int owner, int kind) {
    int n = 0;
    for (int i = 0; i < HOOP_MAX_SHOTS; i++) {
        const Shot *s = &hg.m.shot[i];
        n += s->alive && s->owner == owner && s->kind == kind;
    }
    return n;
}

int hoop_add_shot(int kind, int owner, int x, int y, int vx, int vy) {
    Match *m = &hg.m;
    for (int i = 0; i < HOOP_MAX_SHOTS; i++) {
        Shot *s = &m->shot[i];
        if (s->alive) continue;
        memset(s, 0, sizeof *s);
        s->alive = 1;
        s->kind = (uint8_t)kind;
        s->owner = (uint8_t)owner;
        s->x = x;
        s->y = y;
        s->vx = vx;
        s->vy = vy;
        s->prio = SHOT_PRIO[kind];
        s->fuse = HOOP_BOMB_FUSE;
        return i;
    }
    return -1;
}

static int cx_of(const Fighter *f) { return f->x + HOOP_FW * HQ / 2; }
static int cy_of(const Fighter *f) { return f->y + HOOP_FH * HQ / 2; }

/* ---- B: the ways to move -------------------------------------------------- */

static void drop_through(Fighter *f) {
    f->drop_t = 10;
    f->ground = GND_AIR;
    f->y += f->g * 2 * HQ;
    f->vy = f->g * 128;
}

static void jump(Fighter *f, int v) {
    f->vy = -v * f->g;
    f->ground = GND_AIR;
    f->jump_hold = 1;
    hoop_sfx("hoop_jump");
}

static void plant_mine(int i) {
    Match *m = &hg.m;
    Fighter *f = &m->f[i];
    /* three traps each at most: the oldest goes */
    int n = 0, oldest = -1, oldest_t = -1;
    for (int k = 0; k < HOOP_MAX_MINES; k++) {
        Mine *q = &m->mine[k];
        if (!q->alive || q->owner != i) continue;
        n++;
        if (q->arm_t > oldest_t) { oldest_t = q->arm_t; oldest = k; }
    }
    if (n >= 3 && oldest >= 0) m->mine[oldest].alive = 0;
    for (int k = 0; k < HOOP_MAX_MINES; k++) {
        Mine *q = &m->mine[k];
        if (q->alive) continue;
        memset(q, 0, sizeof *q);
        q->alive = 1;
        q->owner = (uint8_t)i;
        q->x = cx_of(f);
        q->y = f->y + HOOP_FH * HQ;
        q->landed = 0;
        if (f->ground == GND_FLOOR) q->landed = 1;
        else if (hoop_on_ledge(f)) q->landed = (uint8_t)(2 + f->ground);
        hoop_sfx("hoop_trap");
        break;
    }
    f->fire_cd = imax(f->fire_cd, 12);
    f->act_t = 10;
}

static void fire_claw(Fighter *f) {
    int dir = (HELD(f, HP_L) ? -1 : 0) + (HELD(f, HP_R) ? 1 : 0);
    f->claw = 1;
    f->cx = cx_of(f);
    f->cy = f->y;
    f->claw_dist = 0;
    f->swing_dir = dir;
    if (dir) {
        f->face = dir;
        f->cvx = dir * 1086; /* 6 px a frame at 45 degrees */
        f->cvy = -1086;
    } else {
        f->cvx = 0;
        f->cvy = -1536;
    }
    hoop_sfx("hoop_claw");
}

static void lift_start(Fighter *f, int dir) {
    if (dir > 0 && f->ground == GND_FLOOR) return;
    f->lift = dir;
    f->lift_skip = hoop_on_ledge(f) ? f->ground : -1;
    f->ground = GND_AIR;
    f->vx = 0;
    f->vy = dir * 2 * HQ;
    hoop_sfx("hoop_lift");
}

static void moves(int i) {
    Match *m = &hg.m;
    Fighter *f = &m->f[i];
    bool down = HELD(f, HP_D);
    bool on_ledge = hoop_on_ledge(f);
    switch (f->kind) {
    case HF_TANSY:
        if (PRESSED(f, HP_B)) {
            if (down && on_ledge) drop_through(f);
            else if (grounded(f)) jump(f, 1075);
            else if (f->jumps > 0) {
                f->jumps = 0;
                jump(f, 970);
                f->spin_t = 26;
            }
        }
        break;
    case HF_PEWIT:
        if (PRESSED(f, HP_B)) {
            if (down && on_ledge) drop_through(f);
            else if (grounded(f)) jump(f, 940);
            else {
                /* a flap: up again, as often as you like */
                f->vy = imin(f->vy, -520);
                f->hover_t = 0;
                f->act_t = 8;
                f->jump_hold = 0;
                hoop_sfx("hoop_flap");
            }
        }
        break;
    case HF_CLAMP:
        if (PRESSED(f, HP_B)) {
            if (f->claw == 2) {
                /* let go, with a little hop */
                f->claw = 0;
                f->swing_t = 0;
                f->vy = imin(f->vy, 0) - 820;
                hoop_sfx("hoop_jump");
            } else if (down && on_ledge && f->claw == 0) {
                drop_through(f);
            } else if (f->claw == 0) {
                fire_claw(f);
            }
        }
        break;
    case HF_MOSS:
        if (PRESSED(f, HP_B) && f->spin_t <= 0) {
            f->g = -f->g;
            f->ground = GND_AIR;
            f->vy = f->g * 128;
            f->spin_t = 6; /* Moss's own wait between turns */
            hoop_sfx("hoop_flip");
        }
        break;
    case HF_BRISTLE:
        if (PRESSED(f, HP_B)) {
            if (down && on_ledge) drop_through(f);
            else if (f->act_t <= 0) plant_mine(i);
        }
        break;
    case HF_COLLIER:
        if (PRESSED(f, HP_B)) {
            if (f->lift) {
                /* off the lift, keeping some of its speed */
                f->vy = f->lift * 2 * HQ * 6 / 10;
                f->lift = 0;
                f->lift_skip = -1;
            } else {
                lift_start(f, down ? 1 : -1);
            }
        }
        break;
    case HF_ASTRA:
        if (PRESSED(f, HP_B) && down && on_ledge) {
            drop_through(f);
        } else if (HELD(f, HP_B) && !(down && on_ledge)) {
            /* the pack: harder the longer it burns, up to a top speed */
            f->thrust_t++;
            int push = 70 + imin(f->thrust_t, 30) * 3;
            f->vy -= push;
            if (f->vy < -900) f->vy = -900;
            f->ground = GND_AIR;
            if ((f->thrust_t & 7) == 1) hoop_sfx("hoop_jet");
        } else {
            f->thrust_t = 0;
        }
        break;
    case HF_GULP:
        if (PRESSED(f, HP_B) && down && on_ledge) {
            drop_through(f);
        } else if (HELD(f, HP_B) && grounded(f) && (f->charge_t > 0 || PRESSED(f, HP_B))) {
            f->charge_t = imin(f->charge_t + 1, 90);
            if (f->charge_t == HOOP_GLOW_T) hoop_sfx("hoop_glow");
        } else if (!HELD(f, HP_B) && f->charge_t > 0) {
            /* the leap: higher for a longer hold, leaned by the pad */
            int c = imin(f->charge_t, HOOP_GLOW_T);
            int v = 560 + c * 23; /* a full second's leap: floor to ceiling */
            int lean = (HELD(f, HP_R) ? 1 : 0) - (HELD(f, HP_L) ? 1 : 0);
            f->glow_jump = f->charge_t >= HOOP_GLOW_T;
            f->charge_t = 0;
            jump(f, v);
            f->jump_hold = 0;
            f->vx = lean * 480;
            if (lean) f->face = lean;
            f->melee_hit = 0;
        }
        break;
    }
    /* a short hop for a quick tap (the jumpers) */
    if (f->jump_hold && !HELD(f, HP_B) && f->vy * f->g < -300) {
        f->vy = f->vy / 2;
        f->jump_hold = 0;
    }
    if (f->jump_hold && f->vy * f->g >= 0) f->jump_hold = 0;
}

/* ---- A: weapons and the lunge --------------------------------------------- */

static void lunge(int i) {
    Fighter *f = &hg.m.f[i];
    f->melee_t = HOOP_MELEE_T;
    f->melee_hit = 0;
    f->melee_cd = HOOP_MELEE_CD;
    f->blocking = false;
    f->act_t = HOOP_MELEE_T;
    f->vx = f->face * 900;
    switch (f->kind) {
    case HF_BRISTLE: /* long, flat, and gravity can wait */
        f->melee_t = 22;
        f->act_t = 22;
        f->vx = f->face * 1100;
        f->vy = 0;
        break;
    case HF_ASTRA: /* a kick, down and ahead in the air */
        if (!grounded(f)) { f->vx = f->face * 768; f->vy = 768 * f->g; }
        break;
    case HF_PEWIT:
        if (!grounded(f)) f->hover_t = HOOP_MELEE_T + 50;
        break;
    default:
        if (!grounded(f) && f->vy * f->g > 0) f->vy = f->vy / 2;
        break;
    }
    hoop_sfx("hoop_lunge");
}

static void throw_bomb(Fighter *f, int idx, bool high) {
    Shot *s = &hg.m.shot[idx];
    s->held = 0;
    s->vx = f->face * (high ? 300 : 640);
    s->vy = high ? -1030 : -610;
    hoop_sfx("hoop_throw");
}

static void dart_at(int i, int a) {
    Fighter *f = &hg.m.f[i];
    hoop_add_shot(SH_DART, i, cx_of(f), cy_of(f) - 2 * HQ, HOOP_COS[a] * 970 / 256, HOOP_SIN[a] * 970 / 256);
    f->fire_cd = 26;
    f->act_t = 10;
    hoop_sfx("hoop_throw");
}

/* Gulp's sight: 0 to 90 degrees, up on the ground and down in the air */
int hoop_gulp_angle(const Fighter *f) {
    int steps = imin(f->aim_t / 2, 18);
    if (grounded(f)) return f->face > 0 ? (72 - steps) % 72 : 36 + steps;
    return f->face > 0 ? steps : 36 - steps;
}

static void weapon(int i, bool up) {
    Match *m = &hg.m;
    Fighter *f = &m->f[i];
    int x = cx_of(f), y = cy_of(f) - 2 * HQ;
    f->act_t = 10;
    switch (f->kind) {
    case HF_TANSY:
        if (up) {
            if (count_shots(i, SH_SHORTKNIFE) >= 2) return;
            hoop_add_shot(SH_SHORTKNIFE, i, x, y, f->face * 640, 0);
        } else {
            if (count_shots(i, SH_KNIFE) >= 2) return;
            hoop_add_shot(SH_KNIFE, i, x + f->face * 4 * HQ, y, f->face * 1024, 0);
        }
        f->fire_cd = 22;
        hoop_sfx("hoop_throw");
        break;
    case HF_CLAMP: {
        if (count_shots(i, SH_COG) >= 1) return;
        int s = hoop_add_shot(SH_COG, i, x, y, f->face * 1150, 0);
        if (s >= 0) m->shot[s].arc = up ? 2 : 1; /* 1 comes back below, 2 above */
        f->fire_cd = 20;
        hoop_sfx("hoop_cog");
        break;
    }
    case HF_MOSS:
        if (count_shots(i, SH_ROCKET) + count_shots(i, SH_DUD) >= 1) return;
        hoop_add_shot(up ? SH_DUD : SH_ROCKET, i, x, y, f->face * 512, 0);
        f->fire_cd = 30;
        hoop_sfx("hoop_rocket");
        break;
    case HF_BRISTLE: {
        /* three quills: up and ahead on the ground, down and ahead in the air */
        static const int GROUND[3] = {69, 66, 63}, AIR[3] = {3, 6, 9};
        for (int k = 0; k < 3; k++) {
            int a = grounded(f) ? GROUND[k] : AIR[k];
            if (f->face < 0) a = (36 - a + 72) % 72;
            hoop_add_shot(SH_QUILL, i, x, y, HOOP_COS[a] * 900 / 256, HOOP_SIN[a] * 900 / 256);
        }
        if (!grounded(f)) { f->vy = 0; f->vx /= 2; }
        f->fire_cd = 26;
        hoop_sfx("hoop_quill");
        break;
    }
    case HF_PEWIT:
        if (count_shots(i, SH_SHOE) >= 2) return;
        if (up) hoop_add_shot(SH_SHOE, i, x, y, f->face * 360, -1075);
        else hoop_add_shot(SH_SHOE, i, x, y, f->face * 720, -660);
        f->fire_cd = 28;
        hoop_sfx("hoop_throw");
        break;
    case HF_ASTRA:
        if (count_shots(i, SH_RAY) >= 2) return;
        hoop_add_shot(SH_RAY, i, x + f->face * 3 * HQ, y, f->face * 1024, up ? -1024 : 1024);
        f->fire_cd = 30;
        hoop_sfx("hoop_ray");
        break;
    default:
        break;
    }
}

static void attacks(int i) {
    Match *m = &hg.m;
    Fighter *f = &m->f[i];
    bool up = HELD(f, HP_U), down = HELD(f, HP_D);
    bool has_lunge = f->kind != HF_CLAMP && f->kind != HF_GULP;
    if (PRESSED(f, HP_A) && down && has_lunge) {
        if (f->melee_cd <= 0 && f->melee_t == 0 && f->claw != 2 && f->lift == 0) lunge(i);
        return;
    }
    if (f->kind == HF_COLLIER) {
        /* a charge in hand: the fuse burns twice as fast, and it shields */
        if (f->held_bomb >= 0) {
            Shot *s = &m->shot[f->held_bomb];
            if (!s->alive || !s->held) {
                f->held_bomb = -1;
            } else if (!HELD(f, HP_A)) {
                throw_bomb(f, f->held_bomb, s->arc || up);
                f->held_bomb = -1;
                f->fire_cd = 24;
            } else {
                s->fuse--;
            }
        } else if (PRESSED(f, HP_A) && f->fire_cd <= 0 && count_shots(i, SH_BOMB) < 2) {
            int s = hoop_add_shot(SH_BOMB, i, cx_of(f) + f->face * 8 * HQ, cy_of(f) - 2 * HQ, 0, 0);
            if (s >= 0) {
                m->shot[s].held = 1;
                m->shot[s].fuse = HOOP_BOMB_FUSE;
                m->shot[s].arc = up;
                f->held_bomb = s;
                f->act_t = 10;
            }
        }
        return;
    }
    if (f->kind == HF_GULP) {
        if (f->lock_t > 0) { f->aiming = 0; return; }
        if (PRESSED(f, HP_A) && f->fire_cd <= 0) {
            if (up) {
                hoop_add_shot(SH_FASTDART, i, cx_of(f), cy_of(f) - 2 * HQ, f->face * 1800, 0);
                f->fire_cd = 30;
                f->act_t = 10;
                hoop_sfx("hoop_throw");
            } else {
                f->aiming = 1;
                f->aim_t = 0;
            }
        } else if (f->aiming && HELD(f, HP_A)) {
            f->aim_t++;
        } else if (f->aiming && !HELD(f, HP_A)) {
            f->aiming = 0;
            dart_at(i, f->aim_t < 4 ? (f->face > 0 ? 0 : 36) : hoop_gulp_angle(f));
        }
        return;
    }
    if (PRESSED(f, HP_A) && f->fire_cd <= 0 && f->melee_t == 0) {
        if (f->kind == HF_CLAMP && f->claw == 2 && down) return;
        weapon(i, up);
    }
}

void hoop_fighter_act(int i) {
    Match *m = &hg.m;
    Fighter *f = &m->f[i];
    f->anim_t++;
    if (f->hurt_t > 0) f->hurt_t--;
    if (f->inv_t > 0) f->inv_t--;
    if (f->dizzy_t > 0) f->dizzy_t--;
    if (f->melee_cd > 0) f->melee_cd--;
    if (f->fire_cd > 0) f->fire_cd--;
    if (f->lock_t > 0) f->lock_t--;
    if (f->spin_t > 0) f->spin_t--;
    if (f->hover_t > 0) f->hover_t--;
    if (f->drop_t > 0) f->drop_t--;
    if (f->swing_t > 0) f->swing_t--;
    if (f->act_t > 0) f->act_t--;
    if (f->melee_t > 0) {
        f->melee_t--;
        if (f->melee_t == 0) {
            f->melee_hit = 0;
            if (f->kind == HF_BRISTLE) f->vx = f->face * 300;
        }
    }
    bool control = f->hurt_t == 0 && f->dizzy_t == 0;
    if (!control) {
        f->blocking = false;
        f->charge_t = 0;
        f->thrust_t = 0;
        f->aiming = 0;
        if (f->held_bomb >= 0) {
            m->shot[f->held_bomb].held = 0;
            f->held_bomb = -1;
        }
        return;
    }
    int b = f->held;
    if (f->melee_t == 0 && f->claw != 2) {
        if ((b & HP_L) && !(b & HP_R)) f->face = -1;
        else if ((b & HP_R) && !(b & HP_L)) f->face = 1;
    }
    bool can_block = (grounded(f) || f->claw == 2) && f->lift == 0 && f->charge_t == 0 && f->melee_t == 0 &&
                     f->swing_t == 0 && !f->glow_jump;
    f->blocking = (b & HP_D) && can_block;
    moves(i);
    attacks(i);
    if (f->melee_t > 0 || f->charge_t > 0 || f->lift || f->glow_jump) f->blocking = false;
    if (f->blocking && grounded(f)) f->charge_t = 0;
}

/* ---- moving and landing -------------------------------------------------- */

static void collide(Fighter *f, int oldy) {
    Match *m = &hg.m;
    int x0 = HOOP_AL * HQ, x1 = (HOOP_AR - HOOP_FW) * HQ;
    if (f->x < x0) { f->x = x0; if (f->vx < 0) f->vx = 0; }
    if (f->x > x1) { f->x = x1; if (f->vx > 0) f->vx = 0; }
    f->ground = GND_AIR;
    int top = HOOP_AT * HQ, bot = (HOOP_AB - HOOP_FH) * HQ;
    if (f->y >= bot) {
        f->y = bot;
        if (f->vy > 0) f->vy = 0;
        if (f->g > 0) f->ground = GND_FLOOR;
    }
    if (f->y <= top) {
        f->y = top;
        if (f->vy < 0) f->vy = 0;
        if (f->g < 0) f->ground = GND_CEIL;
    }
    if (f->drop_t > 0 || f->ground != GND_AIR) return;
    for (int i = 0; i < m->nledges; i++) {
        if (f->lift && i == f->lift_skip) continue;
        const Ledge *l = &m->ledge[i];
        if (f->x + HOOP_FW * HQ <= l->x * HQ || f->x >= (l->x + l->w) * HQ) continue;
        if (f->g > 0 && f->vy >= 0) {
            int lt = l->y * HQ, plt = (l->y - l->dy) * HQ;
            int ob = oldy + HOOP_FH * HQ, nb = f->y + HOOP_FH * HQ;
            if (ob <= plt + 64 && nb >= lt) {
                f->y = lt - HOOP_FH * HQ;
                f->vy = 0;
                f->ground = i;
                return;
            }
        } else if (f->g < 0 && f->vy <= 0) {
            int ub = (l->y + HOOP_PLAT_H) * HQ, pub = (l->y - l->dy + HOOP_PLAT_H) * HQ;
            if (oldy >= pub - 64 && f->y <= ub) {
                f->y = ub;
                f->vy = 0;
                f->ground = i;
                return;
            }
        }
    }
}

static void claw_physics(int i) {
    Match *m = &hg.m;
    Fighter *f = &m->f[i];
    if (f->claw == 1) {
        f->cx += f->cvx;
        f->cy += f->cvy;
        f->claw_dist += iabs(f->cvy) > iabs(f->cvx) ? iabs(f->cvy) : iabs(f->cvx) * 141 / 100;
        /* the claw takes any hoop it passes that isn't on fire */
        for (int r = 0; r < HOOP_MAX_RINGS; r++) {
            Ring *g = &m->ring[r];
            if (!g->alive || (g->state == RG_FRESH && g->burn_t > 0) || (g->state == RG_LOOSE && g->nograb_t > 0)) continue;
            if (iabs(g->x - f->cx) < 9 * HQ && iabs(g->y - f->cy) < 9 * HQ) hoop_take_ring(i, r);
        }
        int caught = 0;
        if (f->cy <= HOOP_AT * HQ) { f->cy = HOOP_AT * HQ; caught = 1; f->claw_ledge = -1; }
        for (int k = 0; k < m->nledges && !caught; k++) {
            const Ledge *l = &m->ledge[k];
            if (f->cx >= l->x * HQ && f->cx < (l->x + l->w) * HQ && f->cy >= l->y * HQ &&
                f->cy <= (l->y + HOOP_PLAT_H + 2) * HQ && f->cy < f->y) {
                caught = 1;
                f->claw_ledge = k;
                f->cy = (l->y + HOOP_PLAT_H) * HQ;
            }
        }
        if (caught) {
            f->claw = 2;
            int dx = cx_of(f) - f->cx, dy = cy_of(f) - f->cy;
            f->rope = hoop_isqrt((int64_t)dx * dx + (int64_t)dy * dy);
            if (f->rope < 12 * HQ) f->rope = 12 * HQ;
            f->swing_t = f->swing_dir ? 34 : 0;
            f->melee_hit = 0;
            f->ground = GND_AIR;
            hoop_sfx("hoop_bite");
        } else if (f->cx < HOOP_AL * HQ || f->cx > HOOP_AR * HQ || f->claw_dist > 110 * HQ) {
            f->claw = 0;
        }
    }
}

static void swing(int i, int oldy) {
    Match *m = &hg.m;
    Fighter *f = &m->f[i];
    if (f->claw_ledge >= 0) {
        f->cx += m->ledge[f->claw_ledge].dx * HQ;
        f->cy += m->ledge[f->claw_ledge].dy * HQ;
    }
    f->vy += HOOP_GRAV;
    if (!f->blocking && f->hurt_t == 0 && f->dizzy_t == 0) {
        if (HELD(f, HP_L)) f->vx -= 12;
        if (HELD(f, HP_R)) f->vx += 12;
    }
    if (f->rope > 8 * HQ) f->rope -= 200; /* the line reels in */
    int px = cx_of(f) + f->vx, py = cy_of(f) + f->vy;
    int64_t dx = px - f->cx, dy = py - f->cy;
    int d = hoop_isqrt(dx * dx + dy * dy);
    if (d > f->rope && d > 0) {
        px = f->cx + (int)(dx * f->rope / d);
        py = f->cy + (int)(dy * f->rope / d);
        int64_t vr = ((int64_t)f->vx * dx + (int64_t)f->vy * dy) / d;
        if (vr > 0) {
            f->vx -= (int)(vr * dx / d);
            f->vy -= (int)(vr * dy / d);
        }
    }
    f->vx = f->vx * 254 / 256;
    f->x = px - HOOP_FW * HQ / 2;
    f->y = py - HOOP_FH * HQ / 2;
    collide(f, oldy);
    if (f->ground != GND_AIR && d < f->rope - 2 * HQ) { /* feet down and the line slack: let go */
        f->claw = 0;
        f->swing_t = 0;
    } else {
        f->ground = GND_AIR;
    }
}

static void lift_move(Fighter *f, int oldy) {
    Match *m = &hg.m;
    f->vx = 0;
    f->vy = f->lift * 2 * HQ;
    f->y += f->vy;
    if (f->lift < 0) {
        for (int i = 0; i < m->nledges; i++) {
            if (i == f->lift_skip) continue;
            const Ledge *l = &m->ledge[i];
            if (f->x + HOOP_FW * HQ <= l->x * HQ || f->x >= (l->x + l->w) * HQ) continue;
            int lt = l->y * HQ;
            if (oldy + HOOP_FH * HQ >= lt && f->y + HOOP_FH * HQ <= lt) {
                /* level with a ledge: the lift stops and he steps on */
                f->y = lt - HOOP_FH * HQ;
                f->vy = 0;
                f->lift = 0;
                f->lift_skip = -1;
                f->ground = i;
                hoop_sfx("hoop_ding");
                return;
            }
        }
        if (f->y <= HOOP_AT * HQ) {
            f->y = HOOP_AT * HQ;
            f->vy = 0;
            f->lift = 0;
            f->lift_skip = -1;
        }
        f->ground = GND_AIR;
    } else {
        collide(f, oldy);
        if (f->ground != GND_AIR) {
            f->lift = 0;
            f->lift_skip = -1;
            hoop_sfx("hoop_ding");
        }
    }
}

void hoop_fighter_physics(int i) {
    Match *m = &hg.m;
    Fighter *f = &m->f[i];
    if (hoop_on_ledge(f) && f->lift == 0) {
        const Ledge *l = &m->ledge[f->ground];
        f->x += l->dx * HQ;
        f->y += l->dy * HQ;
    }
    int oldy = f->y;
    claw_physics(i);
    if (f->claw == 2) { swing(i, oldy); return; }
    if (f->lift) { lift_move(f, oldy); return; }
    bool control = f->hurt_t == 0 && f->dizzy_t == 0;
    bool was_ground = f->ground != GND_AIR;
    if (f->melee_t > 0) {
        /* the lunge carries itself */
    } else if (control && !f->blocking && f->charge_t == 0) {
        int target = ((f->held & HP_R) ? HOOP_WALK : 0) - ((f->held & HP_L) ? HOOP_WALK : 0);
        if (was_ground) f->vx = target;
        else f->vx = approach(f->vx, target, 40);
    } else if (was_ground) {
        f->vx = approach(f->vx, 0, 48);
    } else {
        f->vx = approach(f->vx, 0, 6);
    }
    /* gravity */
    if (f->kind == HF_BRISTLE && f->melee_t > 0) {
        f->vy = 0;
    } else if (f->kind == HF_PEWIT && f->hover_t > 0) {
        f->vy += f->g * HOOP_GRAV / 4;
        if (f->vy * f->g > 100) f->vy = 100 * f->g;
    } else {
        f->vy += f->g * HOOP_GRAV;
    }
    if (f->vy * f->g > HOOP_MAXFALL) f->vy = HOOP_MAXFALL * f->g;
    f->x += f->vx;
    f->y += f->vy;
    collide(f, oldy);
    if (f->ground != GND_AIR) {
        f->jumps = 1;
        f->hover_t = 0;
        f->glow_jump = 0;
        if (!was_ground) hoop_sfx("hoop_land");
    }
    if (f->glow_jump && f->vy * f->g >= 0) f->glow_jump = 0;
    if (f->ground != GND_AIR && f->vx != 0) f->step++;
}

/* ---- shots, traps and the like ------------------------------------------------ */

static void kill_shot(Shot *s, bool spark) {
    if (spark) hoop_burst(HPX(s->x), HPX(s->y), C_LIGHT, 3);
    s->alive = 0;
}

static void explode(Shot *s, int idx) {
    Match *m = &hg.m;
    bool held = s->held;
    hoop_add_blast(s->owner, HPX(s->x), HPX(s->y), s->kind == SH_BOMB ? 18 : 16, held);
    if (held && m->f[s->owner].held_bomb == idx) m->f[s->owner].held_bomb = -1;
    s->alive = 0;
}

static bool hits_ledge_top(Shot *s, int oldy) {
    Match *m = &hg.m;
    if (s->vy <= 0) return false;
    for (int i = 0; i < m->nledges; i++) {
        const Ledge *l = &m->ledge[i];
        if (s->x < l->x * HQ || s->x >= (l->x + l->w) * HQ) continue;
        int top = l->y * HQ;
        if (oldy <= top && s->y >= top) { s->y = top; return true; }
    }
    return false;
}

static void shot_move(Shot *s, int idx) {
    Match *m = &hg.m;
    int oldy = s->y;
    const Fighter *o = &m->f[s->owner];
    int w = SHOT_W[s->kind] * HQ / 2, h = SHOT_H[s->kind] * HQ / 2;
    bool wall = false, flat = false;
    switch (s->kind) {
    case SH_SHORTKNIFE:
    case SH_SHOE:
        if (s->kind == SH_SHORTKNIFE && s->t > 18) s->vy += 48;
        if (s->kind == SH_SHOE) s->vy += 40;
        break;
    case SH_COG:
        if (!s->home) {
            s->vx -= (s->vx > 0 ? 1 : -1) * 48;
            if (iabs(s->vx) < 60) s->home = 1;
        } else {
            int dir = o->x + HOOP_FW * HQ / 2 > s->x ? 1 : -1;
            s->vx += dir * 48;
            if (iabs(s->vx) > 1150) s->vx = isign(s->vx) * 1150;
            s->vy += s->arc == 2 ? -20 : 20;
            if (iabs(s->vy) > 520) s->vy = isign(s->vy) * 520;
            int dx = o->x + HOOP_FW * HQ / 2 - s->x, dy = o->y + HOOP_FH * HQ / 2 - s->y;
            if (iabs(dx) < 9 * HQ && iabs(dy) < 12 * HQ) { s->alive = 0; return; }
        }
        if (s->t > 170) { kill_shot(s, true); return; }
        break;
    case SH_ROCKET:
    case SH_DUD:
        if (iabs(s->vx) < 1024) s->vx += isign(s->vx) * 24;
        if (s->t > 20 && iabs(s->vy) < 160) s->vy += o->g * 2; /* a slight curve with his gravity */
        break;
    case SH_QUILL:
        if (s->t > 14) { s->alive = 0; return; }
        break;
    case SH_BOMB:
        if (s->held) {
            s->x = o->x + HOOP_FW * HQ / 2 + o->face * 8 * HQ;
            s->y = o->y + HOOP_FH * HQ / 2 - 2 * HQ;
            s->vx = s->vy = 0;
        } else {
            s->vy += 40;
        }
        s->fuse--;
        if (s->fuse <= 0) { explode(s, idx); return; }
        break;
    case SH_FASTDART:
        if (s->stuck) {
            if (s->t > HOOP_DART_LOCK) s->alive = 0;
            return;
        }
        break;
    default:
        break;
    }
    s->x += s->vx;
    s->y += s->vy;
    if (s->x - w < HOOP_AL * HQ) { s->x = HOOP_AL * HQ + w; wall = true; }
    if (s->x + w > HOOP_AR * HQ) { s->x = HOOP_AR * HQ - w; wall = true; }
    if (s->y - h < HOOP_AT * HQ) { s->y = HOOP_AT * HQ + h; flat = true; }
    if (s->y + h > HOOP_AB * HQ) { s->y = HOOP_AB * HQ - h; flat = true; }
    switch (s->kind) {
    case SH_KNIFE:
        if (wall) {
            if (s->bounces >= 2) { kill_shot(s, true); return; }
            s->bounces++;
            s->vx = -s->vx;
            s->self_ok = true; /* coming back, it can cut her too */
            hoop_sfx("hoop_ting");
        }
        if (flat) { kill_shot(s, true); return; }
        break;
    case SH_SHORTKNIFE:
    case SH_SHOE:
    case SH_QUILL:
    case SH_DART:
        if (wall || flat || (s->kind != SH_QUILL && s->kind != SH_DART && hits_ledge_top(s, oldy))) {
            kill_shot(s, true);
            return;
        }
        break;
    case SH_COG:
        if (wall && !s->home) { s->home = 1; s->vx = -s->vx / 2; }
        if (flat) s->vy = -s->vy;
        break;
    case SH_ROCKET:
        if (wall || flat) { explode(s, idx); return; }
        break;
    case SH_DUD:
        if (wall || flat) { kill_shot(s, true); return; }
        break;
    case SH_BOMB:
        if (wall) s->vx = -s->vx * 6 / 10;
        if (flat || hits_ledge_top(s, oldy)) {
            if (s->vy > 0) {
                s->vy = -s->vy * 45 / 100;
                if (iabs(s->vy) < 100) s->vy = 0;
            } else if (s->vy < 0) {
                s->vy = -s->vy;
            }
            s->vx = s->vx * 8 / 10;
        }
        break;
    case SH_RAY:
        if (wall || flat) {
            if (s->bounces >= 1) { kill_shot(s, true); return; }
            s->bounces++;
            if (wall) s->vx = -s->vx;
            if (flat) s->vy = -s->vy;
            hoop_sfx("hoop_ting");
        }
        break;
    case SH_FASTDART:
        if (wall) {
            s->stuck = 1;
            s->t = 0;
            m->f[s->owner].lock_t = HOOP_DART_LOCK;
            hoop_sfx("hoop_thunk");
        }
        if (flat) { kill_shot(s, true); return; }
        break;
    default:
        break;
    }
}

static void shot_hits(Shot *s, int idx) {
    Match *m = &hg.m;
    if (!s->alive || s->held || s->stuck) return;
    int w = SHOT_W[s->kind], h = SHOT_H[s->kind];
    int x = HPX(s->x) - w / 2, y = HPX(s->y) - h / 2;
    for (int k = 0; k < m->n; k++) {
        Fighter *f = &m->f[k];
        if (!f->on || !hoop_touch(f, x, y, w, h)) continue;
        /* Tansy's spin turns any knife away, down and across */
        if (s->kind == SH_KNIFE && f->kind == HF_TANSY && f->spin_t > 0) {
            if (s->owner != k || s->self_ok) {
                s->owner = (uint8_t)k;
                s->self_ok = false;
                s->vx = (s->vx > 0 ? -1 : 1) * 760;
                s->vy = 760;
                s->bounces = 0;
                hoop_sfx("hoop_ting");
            }
            continue;
        }
        if (k == s->owner && !s->self_ok) continue;
        if (f->inv_t > 0) continue;
        if (hoop_melee_on(f) && k != s->owner) {
            /* a lunge goes straight through shots */
            kill_shot(s, true);
            return;
        }
        if (s->kind == SH_ROCKET || s->kind == SH_BOMB) {
            hoop_hurt(k, s->owner, false);
            explode(s, idx);
            return;
        }
        hoop_hurt(k, k == s->owner ? -1 : s->owner, false);
        if (s->kind != SH_COG) {
            s->alive = 0;
            return;
        }
    }
}

/* two shots meet: the higher rank wins, equal ranks both go; most shots
 * (rank 0) just pass each other */
static void shot_clash(void) {
    Match *m = &hg.m;
    for (int i = 0; i < HOOP_MAX_SHOTS; i++) {
        Shot *a = &m->shot[i];
        if (!a->alive || a->stuck) continue;
        for (int j = i + 1; j < HOOP_MAX_SHOTS; j++) {
            Shot *b = &m->shot[j];
            if (!b->alive || b->stuck || a->owner == b->owner) continue;
            if (a->prio == 0 && b->prio == 0) continue;
            if (iabs(a->x - b->x) * 2 > (SHOT_W[a->kind] + SHOT_W[b->kind]) * HQ) continue;
            if (iabs(a->y - b->y) * 2 > (SHOT_H[a->kind] + SHOT_H[b->kind]) * HQ) continue;
            if (a->prio >= b->prio) {
                if (b->kind == SH_BOMB && b->held) m->f[b->owner].held_bomb = -1;
                kill_shot(b, true);
            }
            if (b->prio >= a->prio) {
                if (a->kind == SH_BOMB && a->held) m->f[a->owner].held_bomb = -1;
                kill_shot(a, true);
                break;
            }
        }
    }
}

void hoop_shots_update(void) {
    Match *m = &hg.m;
    for (int i = 0; i < HOOP_MAX_SHOTS; i++) {
        Shot *s = &m->shot[i];
        if (!s->alive) continue;
        s->t++;
        shot_move(s, i);
        shot_hits(s, i);
    }
    shot_clash();
}

void hoop_mines_update(void) {
    Match *m = &hg.m;
    for (int i = 0; i < HOOP_MAX_MINES; i++) {
        Mine *q = &m->mine[i];
        if (!q->alive) continue;
        if (q->landed >= 2) {
            const Ledge *l = &m->ledge[q->landed - 2];
            q->x += l->dx * HQ;
            q->y += l->dy * HQ;
        } else if (q->landed == 0) {
            int oldy = q->y;
            q->vy += HOOP_GRAV;
            q->y += q->vy;
            if (q->y >= HOOP_AB * HQ) { q->y = HOOP_AB * HQ; q->landed = 1; }
            for (int k = 0; k < m->nledges && !q->landed; k++) {
                const Ledge *l = &m->ledge[k];
                if (q->x < l->x * HQ || q->x >= (l->x + l->w) * HQ) continue;
                if (oldy <= l->y * HQ && q->y >= l->y * HQ) { q->y = l->y * HQ; q->landed = (uint8_t)(2 + k); }
            }
        }
        if (q->arm_t < HOOP_MINE_ARM) {
            q->arm_t++;
            if (q->arm_t == HOOP_MINE_ARM) hoop_sfx("hoop_arm");
            continue;
        }
        int x = HPX(q->x) - 4, y = HPX(q->y) - 4;
        for (int k = 0; k < m->n; k++) {
            Fighter *f = &m->f[k];
            if (!f->on || !hoop_touch(f, x, y, 8, 4)) continue;
            if (k == q->owner) {
                /* her own spring: up she goes */
                f->vy = -1640 * f->g;
                f->ground = GND_AIR;
                f->melee_t = 0;
                hoop_sfx("hoop_spring");
            } else {
                hoop_hurt(k, q->owner, false);
                f->vy = -900 * f->g;
                hoop_sfx("hoop_snap");
            }
            hoop_burst(HPX(q->x), HPX(q->y), C_AMBER, 5);
            q->alive = 0;
            break;
        }
    }
}
