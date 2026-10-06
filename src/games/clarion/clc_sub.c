/* CLARION CALL - behind the doors: the closer view.
 *
 * Caves run left to right with a skull drifting in from the left that
 * shoots once it catches up; a red cave ends with a chest and a door back
 * out, a gold cave with fuel and the way on (or, in a region's second
 * area, with the Lobber and a Hush Engine). Rooms are single screens: a
 * friendly sort with free fuel and a word of advice, a shop where one thing
 * may be bought a visit, a health stall, the wriggler trial, the cursed
 * encounter, a sexton, a light or magnet switch, the boon, the armour, or
 * nothing at all. Here Clary takes hits on the bar like the ship does.
 * Enemies have no invincibility after a hit, so mashing B beats holding it. */
#include "clc.h"

static int PX(int32_t v) { return (int)(v / 256) - (v < 0 && v % 256 ? 1 : 0); }

#define FLOOR_Y ((CLC_SH - 1) * CLC_ST)
#define HIT_INV 60

/* ---- lines ----------------------------------------------------------------------------- */

const char *const CLC_NPC_LINE[] = {
    "TEN NOTES OPEN THE GOLD DOOR.",
    "THE KEY IS NEARER THAN YOU THINK.",
    "A HOMING CHARM CALLS YOUR SHIP TO YOU.",
    "SOME PESTS ARE EASIER TO SQUASH ON FOOT.",
    "NOT EVERY NOTE THAT GLOWS IS A NOTE.",
    "A LUCKY THIMBLE KEEPS THE SPOOKS AWAY. SO THEY SAY.",
    "GRANDSIRE TOCK IS WAITING FOR YOU. MIND HOW YOU GO.",
};
const int CLC_NPC_LINES = ARRAY_LEN(CLC_NPC_LINE);

/* ---- tiles -------------------------------------------------------------------------------- */

bool clc_sub_solid_tile(int t, bool feet) {
    switch (t) {
    case ST_WALL: case ST_WALL2: case ST_BLOCK: case ST_ACID: case ST_ICE: case ST_SPIKE: return true;
    case ST_THIN: return feet;
    default: return false;
    }
}

int clc_sub_tile(const ClcSub *s, int px, int py) {
    if (px < 0 || px >= s->w * CLC_ST) return ST_WALL;
    if (py < 0) return ST_WALL;
    if (py >= CLC_SH * CLC_ST) return ST_WALL;
    return s->tile[py / CLC_ST][px / CLC_ST];
}

static const ClcEnt *find_kind(const ClcSub *s, int kind) {
    for (int i = 0; i < s->ne; i++)
        if (s->e[i].on && s->e[i].kind == kind) return &s->e[i];
    return NULL;
}

/* Tock's machine is a floor to stand on */
static bool on_machine(const ClcSub *s, int px, int py) {
    const ClcEnt *m = find_kind(s, EK_TOCK);
    if (!m) return false;
    int top = PX(m->y) - 32;
    return py == top && px >= PX(m->x) - 32 && px < PX(m->x) + 32;
}

bool clc_sub_solid(const void *ctx, int px, int py, bool feet) {
    const ClcSub *s = (const ClcSub *)ctx;
    if (s->locked && s->arena && px < s->arena_c * CLC_ST) return true;
    int t = clc_sub_tile(s, px, py);
    if (t == ST_THIN) return feet && (py % CLC_ST) == 0;
    if (clc_sub_solid_tile(t, false)) return true;
    if (feet && on_machine(s, px, py)) return true;
    return false;
}

/* ---- the kinds ------------------------------------------------------------------------------ */

enum { SF_FOE = 1, SF_INVULN = 2, SF_HAZARD = 4 };
typedef struct SKind { int8_t hw, hh, hp; uint8_t dmg, coins, flags; } SKind;
static const SKind SK[EK_COUNT] = {
    [EK_STINGER] = {6, 6, 2, 4, 3, SF_FOE},
    [EK_DROPPER] = {6, 6, 2, 4, 3, SF_FOE},
    [EK_LOUSE] = {5, 4, 1, 4, 1, SF_FOE},
    [EK_JET] = {6, 6, 0, 4, 0, SF_INVULN | SF_HAZARD},
    [EK_BRUTE] = {10, 13, 6, 6, 60, SF_FOE},
    [EK_RINGWORM] = {6, 7, 3, 4, 5, SF_FOE},
    [EK_SLURP] = {6, 6, 2, 4, 3, SF_FOE},
    [EK_PELTER] = {6, 8, 3, 4, 5, SF_FOE},
    [EK_HOVERBOT] = {6, 6, 2, 4, 3, SF_FOE},
    [EK_TROOPER] = {6, 9, 3, 4, 5, SF_FOE},
    [EK_COCOON] = {6, 8, 3, 4, 5, SF_FOE},
    [EK_BEE] = {4, 4, 1, 4, 1, SF_FOE},
    [EK_SPITTER] = {7, 7, 3, 4, 5, SF_FOE},
    [EK_WHEEL] = {4, 4, 0, 4, 0, SF_INVULN | SF_HAZARD},
    [EK_WISPNEST] = {8, 8, 0, 0, 0, SF_INVULN},
    [EK_WISP] = {4, 4, 1, 4, 1, SF_FOE},
    [EK_SKULL] = {9, 9, 0, 4, 0, SF_FOE | SF_INVULN},
    [EK_WRIGGLER] = {7, 5, 2, 4, 0, SF_FOE},
    [EK_FACE] = {10, 10, 8, 4, 0, SF_FOE},
    [EK_FLAME] = {6, 6, 0, 4, 0, SF_INVULN | SF_HAZARD},
    [EK_BARREL] = {7, 8, 1, 0, 5, 0},
    [EK_CHEST] = {9, 7, 0, 0, 0, 0},
    [EK_ITEM] = {8, 8, 0, 0, 0, 0},
    [EK_NPC] = {7, 12, 0, 0, 0, 0},
    [EK_SWITCH] = {6, 10, 0, 0, 0, 0},
    [EK_LOBBER] = {12, 18, 12, 4, 20, SF_FOE},
    [EK_BOMB] = {5, 5, 3, 0, 0, SF_FOE},
    [EK_ENGINE] = {16, 40, 20, 0, 0, SF_FOE},
    [EK_HUSH] = {26, 9, 0, 6, 0, SF_FOE},
    [EK_HUSHSLURP] = {6, 6, 2, 4, 0, SF_FOE},
    [EK_TOCK] = {32, 20, 0, 4, 0, SF_FOE | SF_INVULN},
    [EK_TOCKHEAD] = {8, 7, 0, 0, 0, SF_FOE},
    [EK_MISSILE] = {3, 5, 1, 4, 0, SF_FOE},
};

int clc_sub_ent_add(ClcSub *s, int kind, int x, int y) {
    int i = -1;
    for (int k = 0; k < s->ne; k++)
        if (!s->e[k].on) { i = k; break; }
    if (i < 0) {
        if (s->ne >= CLC_SENTS) return -1;
        i = s->ne++;
    }
    ClcEnt *e = &s->e[i];
    memset(e, 0, sizeof *e);
    e->kind = (uint8_t)kind;
    e->on = 1;
    e->x = x * 256;
    e->y = y * 256;
    e->hx = (int16_t)x;
    e->hy = (int16_t)y;
    e->hp = SK[kind].hp;
    e->dir = -1;
    /* things that stand on the floor keep their feet at y: their middle is higher */
    switch (kind) {
    case EK_STINGER: case EK_LOUSE: case EK_BRUTE: case EK_PELTER: case EK_TROOPER: case EK_SPITTER:
    case EK_BARREL: case EK_CHEST: case EK_NPC: case EK_SWITCH: case EK_LOBBER: case EK_ENGINE:
    case EK_RINGWORM: case EK_JET: case EK_FLAME:
        e->y -= SK[kind].hh * 256;
        break;
    case EK_TOCK:
        e->y = y * 256; /* its y is the floor; the body is drawn above it */
        break;
    default: break;
    }
    if (kind == EK_TOCK) {
        int h = clc_sub_ent_add(s, EK_TOCKHEAD, x, y - 46);
        if (h >= 0) s->e[h].link = (uint8_t)i;
    }
    if (kind == EK_LOUSE) e->dir = 1;
    return i;
}

static void ebox(const ClcEnt *e, int *x0, int *y0, int *x1, int *y1) {
    int hw = SK[e->kind].hw, hh = SK[e->kind].hh;
    if (e->kind == EK_TOCK) {
        *x0 = PX(e->x) - 32;
        *x1 = PX(e->x) + 31;
        *y0 = PX(e->y) - 32;
        *y1 = PX(e->y) - 1;
        return;
    }
    *x0 = PX(e->x) - hw;
    *y0 = PX(e->y) - hh;
    *x1 = PX(e->x) + hw - 1;
    *y1 = PX(e->y) + hh - 1;
}

static void sfx_(ClcSub *s, int x, int y, int kind) {
    if (s->nfx >= CLC_FX) return;
    s->fx[s->nfx++] = (ClcFx){(int16_t)x, (int16_t)y, (uint8_t)kind};
}

static ClcShot *sshot(ClcSub *s, int kind, int32_t x, int32_t y, int32_t vx, int32_t vy, int life, int dmg) {
    for (int k = 0; k < CLC_SSHOTS; k++)
        if (!s->shot[k].on) {
            s->shot[k] = (ClcShot){x, y, vx, vy, (uint8_t)kind, 1, (uint8_t)dmg, 0, (uint16_t)life, -1};
            return &s->shot[k];
        }
    return NULL;
}

/* ---- Clary --------------------------------------------------------------------------------- */

static int32_t cl_mid_y(const ClcSub *s) { return s->cl.y - (s->cl.crouch ? 5 : 8) * 256; }

static void cl_box(const ClcSub *s, int *x0, int *y0, int *x1, int *y1) {
    int x = PX(s->cl.x), y = PX(s->cl.y);
    int h = s->cl.crouch ? CLC_WALK_SUB.h * 5 / 8 : CLC_WALK_SUB.h;
    *x0 = x - CLC_WALK_SUB.hw;
    *x1 = x + CLC_WALK_SUB.hw - 1;
    *y0 = y - h;
    *y1 = y - 1;
}

static void hurt(ClcSub *s, ClcPlayer *p, int half, int32_t from_x) {
    if (s->cl.inv || p->dead) return;
    clc_hurt(p, half);
    s->cl.inv = HIT_INV;
    s->ev |= CEV_HURT;
    s->cl.vx = s->cl.x >= from_x ? 400 : -400;
    if (s->cl.ground) s->cl.vy = -500;
    if (p->dead) {
        s->ev |= CEV_DIE;
        sfx_(s, PX(s->cl.x), PX(s->cl.y) - 8, FX_BOOM);
    }
}

static void coins_in(ClcSub *s, ClcPlayer *p, int n, int x, int y) {
    n = clc_coin_drop(p, n);
    if (n <= 0 || s->sim) return;
    p->coins += n;
    s->ev |= CEV_COIN;
    sfx_(s, x, y, FX_COINS);
}

static void sub_fire(ClcSub *s, ClcPlayer *p, bool tap, unsigned held) {
    ClcWalker *c = &s->cl;
    int f = c->face, aim = 0;
    if (held & BTN_UP) aim = 1;
    else if (!c->ground && (held & BTN_DOWN)) aim = 2;
    int32_t x = c->x + f * 6 * 256, y = c->y - (c->crouch ? 6 : 10) * 256;
    int32_t vx = f * 1152, vy = 0;
    if (aim == 1) { x = c->x + f * 2 * 256; y = c->y - 17 * 256; vx = 0; vy = -1152; }
    if (aim == 2) { x = c->x; y = c->y; vx = 0; vy = 1152; }
    int kind = SH_PISTOL, dmg = 1;
    if (tap && clc_has(p, G_BIGBANG) && p->coins > 0) {
        kind = SH_BIG;
        dmg = 3;
        if (!s->sim) p->coins--;
    }
    sshot(s, kind, x, y, vx, vy, 40, dmg);
    if (clc_has(p, G_FAN)) {
        if (aim == 0) {
            sshot(s, SH_FAN, x, y, vx * 3 / 4, -600, 13, 1);
            sshot(s, SH_FAN, x, y, vx * 3 / 4, 600, 13, 1);
        } else {
            sshot(s, SH_FAN, x, y, -600, vy * 3 / 4, 13, 1);
            sshot(s, SH_FAN, x, y, 600, vy * 3 / 4, 13, 1);
        }
    }
    s->ev |= CEV_SHOOT;
}

/* ---- hits on things ---------------------------------------------------------------------------- */

static bool boss_open(const ClcSub *s, const ClcEnt *e) {
    if (e->kind == EK_ENGINE) return !find_kind(s, EK_LOBBER);
    if (e->kind == EK_TOCKHEAD) return e->flag != 0;
    return true;
}

static void explode(ClcSub *s, ClcPlayer *p, int32_t x, int32_t y, int r, bool hurts_foes);

static void kill(ClcSub *s, ClcPlayer *p, ClcEnt *e) {
    int x = PX(e->x), y = PX(e->y);
    e->on = 0;
    s->ev |= CEV_KILL;
    sfx_(s, x, y, e->kind == EK_BRUTE || e->kind == EK_LOBBER || e->kind == EK_ENGINE ? FX_BOOM : FX_POP);
    coins_in(s, p, SK[e->kind].coins, x, y);
    switch (e->kind) {
    case EK_BARREL: explode(s, p, e->x, e->y, 34, true); break;
    case EK_LOBBER:
        s->ev |= CEV_BOSS_HIT;
        /* its bombs go out with it */
        for (int i = 0; i < s->ne; i++)
            if (s->e[i].on && s->e[i].kind == EK_BOMB) s->e[i].on = 0;
        break;
    case EK_ENGINE: {
        s->done = 1;
        s->exit_open = 1;
        s->ev |= CEV_BOSS_DOWN;
        /* and, as at the end of every gold cave, fuel */
        int j = clc_sub_ent_add(s, EK_ITEM, x - 48, FLOOR_Y - 20);
        if (j >= 0) { s->e[j].a = IT_FLASK; s->e[j].b = 0; }
        break;
    }
    default: break;
    }
}

static bool hit(ClcSub *s, ClcPlayer *p, ClcEnt *e, int dmg) {
    const SKind *k = &SK[e->kind];
    if (e->kind == EK_SWITCH) {
        if (!s->lit_switch) { s->lit_switch = 1; s->ev |= CEV_OPEN; }
        return true;
    }
    if (e->kind == EK_NPC && e->flag == 1) {
        /* the dozing tortoise doesn't mind */
        if (!s->sim && s->npc_shots < 255) s->npc_shots++;
        return true;
    }
    if (!(k->flags & SF_FOE) && e->kind != EK_BARREL) return false;
    if ((k->flags & SF_INVULN) || !boss_open(s, e)) {
        s->ev |= CEV_CLANG;
        return true;
    }
    if (e->kind == EK_HUSH || e->kind == EK_TOCKHEAD) {
        s->boss_hp = (int16_t)imax(0, s->boss_hp - dmg);
        s->ev |= CEV_BOSS_HIT;
        if (s->boss_hp == 0 && !s->done) {
            s->done = 1;
            s->exit_open = 1;
            s->ev |= CEV_BOSS_DOWN;
            sfx_(s, PX(e->x), PX(e->y), FX_BOOM);
            for (int i = 0; i < s->ne; i++) {
                ClcEnt *o = &s->e[i];
                if (o->on && (o->kind == EK_HUSHSLURP || o->kind == EK_MISSILE)) o->on = 0;
            }
            for (int k2 = 0; k2 < CLC_SSHOTS; k2++)
                if (s->shot[k2].kind >= SH_PELLET) s->shot[k2].on = 0;
        }
        return true;
    }
    e->hp = (int16_t)(e->hp - dmg);
    s->ev |= e->kind == EK_LOBBER || e->kind == EK_ENGINE ? CEV_BOSS_HIT : CEV_HIT;
    if (e->hp <= 0) kill(s, p, e);
    return true;
}

static void explode(ClcSub *s, ClcPlayer *p, int32_t x, int32_t y, int r, bool hurts_foes) {
    s->ev |= CEV_BLAST;
    sfx_(s, PX(x), PX(y), FX_BOOM);
    int a0, b0, a1, b1;
    cl_box(s, &a0, &b0, &a1, &b1);
    int cx = PX(x), cy = PX(y);
    if (a1 >= cx - r && a0 <= cx + r && b1 >= cy - r && b0 <= cy + r) hurt(s, p, 4, x);
    if (!hurts_foes) return;
    for (int i = 0; i < s->ne; i++) {
        ClcEnt *e = &s->e[i];
        if (!e->on || !(SK[e->kind].flags & SF_FOE) || (SK[e->kind].flags & SF_INVULN)) continue;
        if (e->kind == EK_LOBBER || e->kind == EK_ENGINE || e->kind == EK_HUSH || e->kind == EK_TOCKHEAD) continue;
        if (iabs(PX(e->x) - cx) < r + SK[e->kind].hw && iabs(PX(e->y) - cy) < r + SK[e->kind].hh) {
            e->hp = 0;
            kill(s, p, e);
            coins_in(s, p, 5, PX(e->x), PX(e->y)); /* a barrel's bonus */
        }
    }
}

/* ---- enemies --------------------------------------------------------------------------------- */

static bool box_solid(const ClcSub *s, int x0, int y0, int x1, int y1) {
    return clc_sub_solid(s, x0, y0, false) || clc_sub_solid(s, x1, y0, false) || clc_sub_solid(s, x0, y1, true) ||
           clc_sub_solid(s, x1, y1, true) || clc_sub_solid(s, (x0 + x1) / 2, y1, true);
}

static int emove(ClcSub *s, ClcEnt *e) {
    int hw = SK[e->kind].hw, hh = SK[e->kind].hh, h = 0;
    int32_t nx = e->x + e->vx;
    if (!box_solid(s, PX(nx) - hw, PX(e->y) - hh, PX(nx) + hw - 1, PX(e->y) + hh - 2)) e->x = nx;
    else h |= 1;
    int32_t ny = e->y + e->vy;
    if (!box_solid(s, PX(e->x) - hw, PX(ny) - hh, PX(e->x) + hw - 1, PX(ny) + hh - 1)) e->y = ny;
    else h |= 2;
    return h;
}

/* a walker on the floor: falls, turns at walls and ledges */
static void walk_on(ClcSub *s, ClcEnt *e, int speed, bool turn_at_edge) {
    e->vy = imin(1024, e->vy + 60);
    e->vx = e->dir * speed;
    int hw = SK[e->kind].hw, hh = SK[e->kind].hh;
    bool ground = clc_sub_solid(s, PX(e->x), PX(e->y) + hh, true);
    if (ground && turn_at_edge && !clc_sub_solid(s, PX(e->x) + e->dir * (hw + 2), PX(e->y) + hh, true)) {
        e->dir = (int8_t)-e->dir;
        e->vx = 0;
    }
    int h = emove(s, e);
    if (h & 1) e->dir = (int8_t)-e->dir;
    if (h & 2) e->vy = 0;
}

static void eshoot(ClcSub *s, ClcEnt *e, int kind, int32_t vx, int32_t vy, int life) {
    sshot(s, kind, e->x, e->y, vx, vy, life, 4);
    s->ev |= CEV_FOESHOT;
}

static bool on_screen(const ClcSub *s, const ClcEnt *e) {
    int x = PX(e->x), cam = PX(s->cam);
    return x > cam - 16 && x < cam + SCREEN_W + 16;
}

static void estep(ClcSub *s, ClcPlayer *p, ClcEnt *e) {
    e->t++;
    int32_t cx = s->cl.x, cy = cl_mid_y(s);
    int dx = PX(cx - e->x), dy = PX(cy - e->y);
    int adx = iabs(dx);
    bool near = adx < 200 && on_screen(s, e);
    switch (e->kind) {
    case EK_STINGER:
        if (!near) break;
        walk_on(s, e, 110, true);
        if (iabs(dy) < 18 && adx < 150 && e->t % 90 == 0) {
            e->dir = (int8_t)(dx > 0 ? 1 : -1);
            /* a sting at her chest height: crouch under it */
            sshot(s, SH_PELLET, e->x, e->y - 6 * 256, e->dir * 640, 0, 120, 4);
            s->ev |= CEV_FOESHOT;
        }
        break;
    case EK_DROPPER:
        if (e->flag == 0) {
            if (adx < 26 && dy > 0) { e->flag = 1; e->vy = 0; }
        } else {
            e->dir = (int8_t)(dx > 0 ? 1 : -1);
            walk_on(s, e, 80, false);
        }
        break;
    case EK_LOUSE:
        if (!near) break;
        if (e->t % 50 == 0) e->dir = (int8_t)(dx > 0 ? 1 : -1);
        walk_on(s, e, 190, false);
        if (e->t % 70 == 35 && clc_sub_solid(s, PX(e->x), PX(e->y) + SK[EK_LOUSE].hh, true)) e->vy = -900;
        break;
    case EK_JET:
    case EK_FLAME:
        e->flag = (e->t % 150) < 60;
        break;
    case EK_BRUTE:
        if (!near) break;
        if (e->t % 60 == 0) e->dir = (int8_t)(dx > 0 ? 1 : -1);
        walk_on(s, e, 90, false);
        break;
    case EK_RINGWORM:
        if (near && e->t % 120 == 0) {
            for (int k = 0; k < 6; k++) eshoot(s, e, SH_RING, clc_cos(k * 43) * 3, clc_sin(k * 43) * 3, 90);
        }
        break;
    case EK_SLURP:
        if (e->flag == 0) {
            if (adx < 60 && e->t > 40) { e->flag = 1; e->vy = -1300; e->vx = dx > 0 ? 160 : -160; }
        } else {
            e->vy += 50;
            e->x += e->vx;
            e->y += e->vy;
            if (e->y >= e->hy * 256 && e->vy > 0) { e->y = e->hy * 256; e->flag = 0; e->t = 0; e->vx = 0; }
        }
        break;
    case EK_PELTER:
        if (!near) break;
        walk_on(s, e, 60, true);
        if (e->t % 100 == 0 && adx < 180) {
            e->dir = (int8_t)(dx > 0 ? 1 : -1);
            int t = 40; /* frames to land */
            sshot(s, SH_ROCK, e->x, e->y - 6 * 256, dx * 256 / t, -1000, 160, 4);
            s->ev |= CEV_FOESHOT;
        }
        break;
    case EK_HOVERBOT:
        if (!near) break;
        if (e->flag == 0) {
            e->vx = e->dir * 120;
            e->vy = clc_sin(e->t * 4) * 2;
            if (emove(s, e) & 1) e->dir = (int8_t)-e->dir;
            if (iabs(PX(e->x) - e->hx) > 48) e->dir = (int8_t)(PX(e->x) > e->hx ? -1 : 1);
            if (adx < 90 && e->t % 110 == 0) { e->flag = 1; e->a = 0; clc_aim(e->x, e->y, cx, cy, 520, &e->vx, &e->vy); }
        } else {
            if (emove(s, e) || ++e->a > 50) { e->flag = 0; e->hx = (int16_t)PX(e->x); e->hy = (int16_t)PX(e->y); }
        }
        break;
    case EK_TROOPER:
        if (!near) break;
        if ((e->t / 80) & 1) {
            e->dir = (int8_t)(dx > 0 ? 1 : -1);
            if (e->t % 20 == 0 && iabs(dy) < 30) eshoot(s, e, SH_PELLET, e->dir * 700, 0, 100);
            e->vx = 0;
        } else {
            walk_on(s, e, 100, true);
        }
        break;
    case EK_COCOON:
        if (near && e->t % 150 == 0) {
            int n = 0;
            for (int i = 0; i < s->ne; i++) n += s->e[i].on && s->e[i].kind == EK_BEE;
            if (n < 4) clc_sub_ent_add(s, EK_BEE, PX(e->x), PX(e->y) + 10);
        }
        break;
    case EK_BEE:
    case EK_WISP:
    case EK_HUSHSLURP:
        if (e->kind == EK_HUSHSLURP) {
            walk_on(s, e, 0, false);
            if (clc_sub_solid(s, PX(e->x), PX(e->y) + SK[e->kind].hh, true) && e->t % 40 == 0) {
                e->vy = -1100;
                e->vx = dx > 0 ? 260 : -260;
            }
            if (e->vy != 0) e->x += dx > 0 ? 200 : -200;
            break;
        }
        clc_aim(e->x, e->y, cx, cy, e->kind == EK_BEE ? 260 : 220, &e->vx, &e->vy);
        e->vy += clc_sin(e->t * 8) * 3;
        emove(s, e);
        break;
    case EK_SPITTER:
        if (near && e->t % 110 == 0) {
            e->dir = (int8_t)(dx > 0 ? 1 : -1);
            sshot(s, SH_FLAMELET, e->x, e->y - 4 * 256, e->dir * 500, -700, 150, 4);
            s->ev |= CEV_FOESHOT;
        }
        break;
    case EK_WHEEL:
        e->a = (int16_t)((e->a + 2) & 255);
        break;
    case EK_WISPNEST:
        if (on_screen(s, e) && e->t % 50 == 0) {
            int n = 0;
            for (int i = 0; i < s->ne; i++) n += s->e[i].on && s->e[i].kind == EK_WISP && s->e[i].link == (uint8_t)(e - s->e);
            if (n < 3) {
                int j = clc_sub_ent_add(s, EK_WISP, PX(e->x), PX(e->y));
                if (j >= 0) s->e[j].link = (uint8_t)(e - s->e);
            }
        }
        break;
    case EK_SKULL: {
        /* it drifts after her, slower than she runs, and shoots when close */
        if (s->locked) { e->on = 0; break; }
        e->x += 140;
        int32_t ty = cy;
        e->y += (ty > e->y ? 1 : -1) * 120;
        if (adx < 110 && e->t % 80 == 0) {
            int32_t vx, vy;
            clc_aim(e->x, e->y, cx, cy, 600, &vx, &vy);
            eshoot(s, e, SH_PELLET, vx, vy, 120);
        }
        /* never far behind */
        if (cx - e->x > 360 * 256) e->x = cx - 360 * 256;
        break;
    }
    case EK_WRIGGLER:
        e->x += e->dir * 240;
        e->y = e->hy * 256 + clc_sin(e->t * 3) * 28 * 2;
        if (PX(e->x) > s->w * CLC_ST + 12) e->x = -12 * 256;
        if (PX(e->x) < -12) e->x = (s->w * CLC_ST + 12) * 256;
        break;
    case EK_FACE:
        if (!e->vx) { e->vx = e->dir * 330; e->vy = 260; }
        {
            int h = emove(s, e);
            if (h & 1) e->vx = -e->vx;
            if (h & 2) e->vy = -e->vy;
        }
        break;
    case EK_BARREL:
        e->vy = imin(1024, e->vy + 60);
        e->vx = 0;
        if (emove(s, e) & 2) e->vy = 0;
        break;
    case EK_NPC:
        if (e->flag == 3 && adx < 56 && !s->done) {
            /* the cursed one: it splits in two and comes for her */
            e->on = 0;
            s->locked = 1;
            for (int k = 0; k < 2; k++) {
                int j = clc_sub_ent_add(s, EK_FACE, PX(e->x) + (k ? 14 : -14), PX(e->y) - 20);
                if (j >= 0) { s->e[j].dir = (int8_t)(k ? 1 : -1); s->e[j].c = e->c; }
            }
            s->ev |= CEV_BLAST;
            sfx_(s, PX(e->x), PX(e->y), FX_BOOM);
        }
        break;
    case EK_LOBBER:
        if (!s->locked) break;
        e->dir = (int8_t)(dx > 0 ? 1 : -1);
        if (e->t % 90 == 0) e->vy = -900;
        e->vy = imin(1024, e->vy + 60);
        e->vx = 0;
        if (emove(s, e) & 2) e->vy = 0;
        /* bombs once she keeps her distance */
        if (adx > 70 && e->t % 70 == 0) {
            int j = clc_sub_ent_add(s, EK_BOMB, PX(e->x) - 8, PX(e->y) - 16);
            if (j >= 0) {
                int t = 50;
                s->e[j].vx = (int32_t)dx * 256 / t;
                s->e[j].vy = -1200;
                s->e[j].a = 180;
            }
            s->ev |= CEV_FOESHOT;
        }
        break;
    case EK_BOMB:
        e->vy = imin(1024, e->vy + 48);
        {
            int h = emove(s, e);
            if (h & 2) { e->vy = 0; e->vx = e->vx * 3 / 4; }
            if (h & 1) e->vx = -e->vx / 2;
        }
        if (--e->a <= 0) {
            e->on = 0;
            explode(s, p, e->x, e->y, 28, false);
        }
        break;
    case EK_HUSH: {
        /* Lady Hush's saucer: shots down, slurps, and now and then the
         * whole saucer dropped on spikes */
        int floor_mid = FLOOR_Y - 10;
        switch (s->phase) {
        case 0:
            e->x = (int32_t)(160 + clc_sin(e->t) * 120 / 127) * 256;
            e->y = 52 * 256 + clc_sin(e->t * 3) * 3 * 2;
            if (e->t % 70 == 0)
                for (int k = -1; k <= 1; k++) eshoot(s, e, SH_ENERGY, k * 260, 700, 120);
            if (++s->phase_t > 330) {
                s->phase_t = 0;
                s->phase = (uint8_t)(((e->t / 331) & 1) ? 1 : 2);
            }
            break;
        case 1: /* slurps */
            if (s->phase_t == 10)
                for (int k = 0; k < 3; k++) {
                    int j = clc_sub_ent_add(s, EK_HUSHSLURP, PX(e->x) + (k - 1) * 16, PX(e->y) + 12);
                    if (j >= 0) s->e[j].vx = (k - 1) * 200;
                }
            if (++s->phase_t > 60) { s->phase = 0; s->phase_t = 0; }
            break;
        case 2: /* the shake, the drop, a rest on its spikes, then up again */
            if (s->phase_t < 50) e->x += ((s->phase_t & 2) ? 256 : -256);
            else if (s->phase_t < 140) e->y = imin(floor_mid * 256, e->y + 900);
            else e->y -= 300;
            ++s->phase_t;
            if (s->phase_t > 140 && PX(e->y) <= 52) {
                e->y = 52 * 256;
                s->phase = 0;
                s->phase_t = 0;
            }
            break;
        }
        break;
    }
    case EK_TOCK: {
        /* the machine trundles after her, and charges now and then */
        ClcEnt *head = NULL;
        for (int i = 0; i < s->ne; i++)
            if (s->e[i].on && s->e[i].kind == EK_TOCKHEAD) head = &s->e[i];
        int32_t was = e->x;
        bool riding = PX(s->cl.y) == PX(e->y) - 32 && iabs(PX(s->cl.x - e->x)) < 34;
        if (e->flag == 0) {
            /* up to her, not into her */
            e->vx = !riding && adx > 58 ? (dx > 0 ? 70 : -70) : 0;
            if (e->t % 260 == 200 && !riding) { e->flag = 1; e->b = 36; e->vx = 0; }
        } else if (e->flag == 1) {
            /* a shudder, then the charge */
            e->x += (e->b & 2) ? 256 : -256;
            if (--e->b <= 0) { e->flag = 2; e->vx = dx > 0 ? 760 : -760; }
        }
        int32_t nx = e->x + e->vx;
        if (PX(nx) - 32 < CLC_ST || PX(nx) + 32 > (s->w - 1) * CLC_ST) {
            e->vx = 0;
            if (e->flag) { e->flag = 0; s->ev |= CEV_BLAST; }
        } else e->x = nx;
        if (riding) s->cl.x += e->x - was; /* she rides along */
        if (head) {
            head->x = e->x;
            /* the hatch opens for two seconds in five */
            head->flag = (e->t % 300) >= 160 && (e->t % 300) < 280;
            head->y = (PX(e->y) - 32 - (head->flag ? 8 : 0)) * 256;
            if (head->flag && e->t % 300 == 170)
                for (int k = 0; k < 8; k++) eshoot(s, head, SH_RING, clc_cos(k * 32) * 3, clc_sin(k * 32) * 3, 100);
        }
        if (e->t % 240 == 100) {
            /* missiles up, then down on her */
            for (int k = -1; k <= 1; k++) {
                int j = clc_sub_ent_add(s, EK_MISSILE, PX(cx) + k * 40, -10);
                if (j >= 0) { s->e[j].vy = 420; s->e[j].t = (uint16_t)(k * 15 + 15); }
            }
            s->ev |= CEV_FOESHOT;
        }
        break;
    }
    case EK_MISSILE:
        if (e->t > 0 && e->t < 30) break; /* waits up above */
        e->y += e->vy;
        if (clc_sub_solid(s, PX(e->x), PX(e->y) + 5, true) || PX(e->y) > FLOOR_Y) {
            e->on = 0;
            explode(s, p, e->x, e->y, 16, false);
        }
        break;
    default: break;
    }
}

/* ---- touching -------------------------------------------------------------------------------- */

static int chest_draw(ClcSub *s, const ClcPlayer *p, int item) {
    if (item >= 0 && item < G_COUNT && clc_has(p, item)) item = -1;
    if (item >= 0) return item;
    /* a draw: an upgrade not yet had, or a toffee if hurt, a money sack if not */
    if (rng_chance(&s->rng, 40)) {
        int pool[G_COUNT], n = 0;
        for (int g = 0; g < G_COUNT; g++)
            if (!clc_has(p, g) && g != G_CHARM && g != G_PLATE) pool[n++] = g;
        if (n) return pool[rng_range(&s->rng, 0, n - 1)];
    }
    return p->hp < p->hpmax ? IT_TOFFEE : IT_SACK;
}

static void take_item(ClcSub *s, ClcPlayer *p, ClcEnt *e) {
    int item = e->a, price = e->b;
    if (price > 0) {
        if (s->bought || p->coins < price) {
            if (!e->c) s->ev |= CEV_CLANG;
            e->c = 1; /* (say so once a touch) */
            return;
        }
        if (!s->sim) p->coins -= price;
        s->bought = 1;
        s->ev |= CEV_BUY;
    }
    e->on = 0;
    s->got_item = (int16_t)item;
    if (!s->sim) clc_give(p, item);
    s->ev |= CEV_ITEM;
    sfx_(s, PX(e->x), PX(e->y), FX_SPARK);
    if (price > 0 || e->flag == 1) {
        /* one a visit: the rest are gone */
        for (int i = 0; i < s->ne; i++)
            if (s->e[i].on && s->e[i].kind == EK_ITEM && (s->e[i].b > 0 || e->flag == 1)) {
                s->e[i].on = 0;
                sfx_(s, PX(s->e[i].x), PX(s->e[i].y), FX_POP);
            }
    }
}

static void touches(ClcSub *s, ClcPlayer *p) {
    int a0, b0, a1, b1;
    cl_box(s, &a0, &b0, &a1, &b1);
    s->talk = 0;
    for (int i = 0; i < s->ne; i++) {
        ClcEnt *e = &s->e[i];
        if (!e->on) continue;
        int x0, y0, x1, y1;
        ebox(e, &x0, &y0, &x1, &y1);
        /* hazards reach further than their bodies */
        if (e->kind == EK_JET && e->flag) { y0 = 2 * CLC_ST; }
        if (e->kind == EK_FLAME && e->flag) { y0 -= 18; }
        if (e->kind == EK_WHEEL) {
            /* two arms of fire turning round the hub */
            for (int arm = 0; arm < 2; arm++)
                for (int k = 1; k <= 3; k++) {
                    int ang = e->a + arm * 128;
                    int fx = PX(e->x) + clc_cos(ang) * k * 8 / 127, fy = PX(e->y) + clc_sin(ang) * k * 8 / 127;
                    if (fx + 3 >= a0 && fx - 3 <= a1 && fy + 3 >= b0 && fy - 3 <= b1) hurt(s, p, 4, e->x);
                }
            continue;
        }
        bool talk_range = e->kind == EK_NPC && iabs(PX(s->cl.x - e->x)) < 30;
        if (talk_range) {
            s->talk = 1;
            if (e->flag != 3 && e->flag != 4 && !e->b && e->flag != 2) {
                /* a free tank's worth of fuel, once */
                e->b = 1;
                if (!s->sim) clc_add_fuel(p, 500);
                s->gave_fuel = 500;
                s->ev |= CEV_FUEL | CEV_TALK;
            }
            if (e->flag == 2 && !e->b) {
                e->b = 1;
                s->got_item = IT_SHEET;
                s->ev |= CEV_ITEM | CEV_TALK;
            }
        }
        if (!rects_overlap(a0, b0, a1 - a0 + 1, b1 - b0 + 1, x0, y0, x1 - x0 + 1, y1 - y0 + 1)) continue;
        switch (e->kind) {
        case EK_ITEM: take_item(s, p, e); break;
        case EK_CHEST:
            if (!e->flag) {
                e->flag = 1;
                int it = chest_draw(s, p, e->a);
                int j = clc_sub_ent_add(s, EK_ITEM, PX(e->x), PX(e->y) - 26);
                if (j >= 0) { s->e[j].a = (int16_t)it; s->e[j].b = 0; }
                s->chest_item = (uint8_t)it;
                s->ev |= CEV_OPEN;
            }
            break;
        case EK_SWITCH:
            if (!s->lit_switch) { s->lit_switch = 1; s->ev |= CEV_OPEN; }
            break;
        case EK_JET:
        case EK_FLAME:
            if (e->flag) hurt(s, p, SK[e->kind].dmg, e->x);
            break;
        case EK_HUSH:
            if (s->phase == 2 && s->phase_t >= 50) hurt(s, p, 6, e->x);
            break;
        case EK_TOCK: {
            /* standing on it is fine; its front hurts when it charges */
            if (PX(s->cl.y) <= y0 + 1) break;
            if (e->flag == 2) {
                hurt(s, p, 4, e->x);
                e->flag = 0;
                e->vx = 0;
            }
            /* never caught inside it: out to the side, or up on top if a wall is there */
            int side = s->cl.x > e->x ? 1 : -1;
            int out = PX(e->x) + side * (32 + CLC_WALK_SUB.hw + 1);
            if (clc_walker_blocked(&CLC_WALK_SUB, clc_sub_solid, s, out * 256, s->cl.y)) {
                s->cl.y = (int32_t)y0 * 256;
                s->cl.vy = 0;
            } else s->cl.x = (int32_t)out * 256;
            break;
        }
        case EK_TOCKHEAD:
        case EK_ENGINE:
        case EK_BOMB:
        case EK_BARREL:
        case EK_NPC:
            break;
        default:
            if (SK[e->kind].flags & SF_FOE) hurt(s, p, SK[e->kind].dmg, e->x);
            break;
        }
        if (p->dead) return;
    }
    /* spikes and acid underfoot */
    int fx = PX(s->cl.x), fy = PX(s->cl.y);
    int t = clc_sub_tile(s, fx, fy);
    if (s->cl.ground && (t == ST_SPIKE || t == ST_ACID)) {
        hurt(s, p, 4, s->cl.x);
        s->cl.vy = -900;
        s->cl.ground = 0;
    }
}

/* ---- shots ---------------------------------------------------------------------------------- */

static void shots(ClcSub *s, ClcPlayer *p) {
    int a0, b0, a1, b1;
    cl_box(s, &a0, &b0, &a1, &b1);
    for (int k = 0; k < CLC_SSHOTS; k++) {
        ClcShot *sh = &s->shot[k];
        if (!sh->on) continue;
        if (--sh->life == 0) { sh->on = 0; continue; }
        if (sh->kind == SH_ROCK || sh->kind == SH_FLAMELET) sh->vy = imin(1200, sh->vy + 50);
        int32_t nx = sh->x + sh->vx, ny = sh->y + sh->vy;
        bool mine = sh->kind <= SH_SPIT;
        bool through = mine && sh->kind <= SH_BIG && clc_has(p, G_GHOST);
        int tx = PX(nx), ty = PX(ny);
        if (!through && clc_sub_solid(s, tx, ty, sh->vy > 0)) {
            /* blocks give up their coins */
            int t = clc_sub_tile(s, tx, ty);
            if (mine && t == ST_BLOCK) {
                if (!s->sim) {
                    s->tile[ty / CLC_ST][tx / CLC_ST] = ST_AIR;
                    s->ver++;
                }
                coins_in(s, p, 1, tx, ty);
                s->ev |= CEV_BREAK;
                sfx_(s, (tx / CLC_ST) * CLC_ST + 8, (ty / CLC_ST) * CLC_ST + 8, FX_DUST);
            }
            if (sh->kind == SH_FLAMELET && sh->vy > 0) {
                /* the spitter's flame spreads along the floor */
                sh->vy = 0;
                sh->y = (int32_t)((ty / CLC_ST) * CLC_ST - 4) * 256;
                sh->vx = sh->vx > 0 ? 300 : -300;
                continue;
            }
            if (mine && (sh->kind == SH_PISTOL || sh->kind == SH_BIG) && clc_has(p, G_CREEPER)) {
                ClcShot *c = sshot(s, SH_FRAG, sh->x, sh->y, sh->vx > 0 ? -420 : 420, 0, 70, 1);
                (void)c;
            }
            sh->on = 0;
            sfx_(s, PX(sh->x), PX(sh->y), FX_SPARK);
            continue;
        }
        if (sh->kind == SH_FRAG) {
            if (!clc_sub_solid(s, tx, ty + 4, true)) sh->vy = 500;
            else sh->vy = 0;
        }
        sh->x = nx;
        sh->y = ny;
        int r = sh->kind == SH_BIG ? 4 : 2;
        if (mine) {
            for (int i = 0; i < s->ne; i++) {
                ClcEnt *e = &s->e[i];
                if (!e->on) continue;
                int k2 = e->kind;
                if (k2 == EK_CHEST || k2 == EK_ITEM || (k2 == EK_NPC && e->flag != 1)) continue;
                if (k2 == EK_JET || k2 == EK_FLAME || k2 == EK_WHEEL || k2 == EK_WISPNEST) continue;
                int x0, y0, x1, y1;
                ebox(e, &x0, &y0, &x1, &y1);
                if (tx + r < x0 || tx - r > x1 || ty + r < y0 || ty - r > y1) continue;
                if (hit(s, p, e, sh->dmg)) {
                    sh->on = 0;
                    sfx_(s, tx, ty, FX_HIT);
                    break;
                }
            }
        } else if (tx + 2 >= a0 && tx - 2 <= a1 && ty + 2 >= b0 && ty - 2 <= b1) {
            hurt(s, p, sh->dmg, sh->x);
            sh->on = 0;
        }
    }
}

/* ---- a step ------------------------------------------------------------------------------------ */

static bool all_dead(const ClcSub *s, int kind) {
    for (int i = 0; i < s->ne; i++)
        if (s->e[i].on && s->e[i].kind == kind) return false;
    return true;
}

void clc_sub_step(ClcSub *s, ClcPlayer *p, unsigned buttons) {
    unsigned pressed = buttons & ~(unsigned)s->prev;
    s->ev = 0;
    s->nfx = 0;
    s->got_item = -1;
    s->gave_fuel = 0;
    s->prev = (uint8_t)buttons;
    if (p->dead) { s->t++; return; }
    ClcWalker *c = &s->cl;
    int cx = PX(c->x), cy = PX(c->y);
    /* doors: UP at the way in goes back out; UP at the way on moves on */
    if ((pressed & BTN_UP) && clc_bot_debug > 2)
        fprintf(stderr, "UP at %d,%d ground %d door %d locked %d sim %d\n", cx, cy, c->ground, s->door_c, s->locked, s->sim);
    if ((pressed & BTN_UP) && c->ground) {
        if (s->door_c >= 0 && iabs(cx - (s->door_c * CLC_ST + 8)) <= 8 && !s->locked && cy == FLOOR_Y) {
            s->leave = 1;
            s->ev |= CEV_DOOR;
            return;
        }
        if (s->exit_c >= 0 && s->exit_open && iabs(cx - (s->exit_c * CLC_ST + 8)) <= 10 && cy == FLOOR_Y) {
            s->leave = s->room == RM_CAVE ? 1 : 2;
            s->ev |= CEV_DOOR | CEV_EXIT;
            return;
        }
    }
    unsigned flags = 0;
    if (clc_has(p, G_FEATHER)) flags |= WK_FEATHER;
    if (s->ice && clc_sub_tile(s, cx, cy) == ST_ICE) flags |= WK_ICE;
    clc_walk(c, &CLC_WALK_SUB, buttons, pressed, flags, clc_sub_solid, NULL, s);
    if (buttons & BTN_B) {
        bool tap = (pressed & BTN_B) != 0;
        if ((tap && c->shot_cd <= 8) || c->shot_cd == 0) {
            sub_fire(s, p, tap, buttons);
            c->shot_cd = 14;
        }
    }
    /* the arena shuts behind her */
    if (s->arena && !s->locked && PX(c->x) > s->arena_c * CLC_ST + 28) {
        s->locked = 1;
        s->ev |= CEV_TIMER;
    }
    for (int i = 0; i < s->ne; i++)
        if (s->e[i].on) estep(s, p, &s->e[i]);
    shots(s, p);
    touches(s, p);
    /* the trial and the cursed encounter end when the last one falls */
    if (s->room == RM_TRIAL && !s->done && all_dead(s, EK_WRIGGLER)) {
        s->done = 1;
        /* 150 coins, purse or no purse */
        if (!s->sim) p->coins += 150;
        s->ev |= CEV_OPEN | CEV_COIN;
        sfx_(s, PX(c->x), PX(c->y) - 20, FX_COINS);
    }
    if (s->room == RM_CURSED && s->locked && !s->done && all_dead(s, EK_FACE) && !find_kind(s, EK_NPC)) {
        s->done = 1;
        s->locked = 0;
        int item = s->chest_item;
        int j = clc_sub_ent_add(s, EK_ITEM, 168, FLOOR_Y - 20);
        if (j >= 0) { s->e[j].a = (int16_t)item; s->e[j].b = 0; }
        s->ev |= CEV_OPEN;
    }
    /* the camera */
    int vw = SCREEN_W;
    int target = PX(c->x) - 140;
    if (s->locked && s->arena) target = s->arena_c * CLC_ST;
    target = iclamp(target, 0, imax(0, s->w * CLC_ST - vw));
    s->cam = (int32_t)target * 256;
    s->t++;
}

void clc_sub_boss_down(ClcSub *s) {
    for (int i = 0; i < s->ne; i++) {
        ClcEnt *e = &s->e[i];
        if (!e->on) continue;
        if (e->kind == EK_LOBBER || e->kind == EK_BOMB || e->kind == EK_ENGINE || e->kind == EK_HUSHSLURP || e->kind == EK_MISSILE ||
            e->kind == EK_WRIGGLER || e->kind == EK_FACE)
            e->on = 0;
    }
    if (s->boss_kind >= 2) s->boss_hp = 0;
    s->done = 1;
    s->exit_open = 1;
    s->locked = s->room == RM_CURSED ? 0 : s->locked;
}
