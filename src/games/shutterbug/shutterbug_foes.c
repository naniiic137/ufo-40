/* SHUTTERBUG - every foe and prop in the five stages: how each moves and
 * fires, what a photo does to it, and the stage's spawn list. The bosses
 * are in shutterbug_bosses.c. See docs/games/24-shutterbug.md. */
#include "shutterbug.h"

/*                  name          hp  points  hw  hh  role        mount */
const FoeDef SHB_FOE[K_COUNT] = {
    [K_NONE]     = {"",            1,    0,   1,  1, ROLE_PROP, 0},
    [K_PUFF]     = {"SEEDPUFF",    1,  100,   6,  5, ROLE_FOE, 0},
    [K_MINT]     = {"MINT",        2,  100,   5,  5, ROLE_FOE, 0},
    [K_TURRET]   = {"SPOUT",       6,  300,   6,  5, ROLE_FOE, 1},
    [K_WALLBOMB] = {"CLINGER",     3,  200,   5,  5, ROLE_FOE, 1},
    [K_CHOMPER]  = {"DANGLER",     5,  300,   6,  7, ROLE_FOE, 1},
    [K_CUBE]     = {"SUGAR CUBE",  1,  100,   5,  5, ROLE_FOE, 0},
    [K_TOAST]    = {"TOASTIE",     4,  200,   6,  6, ROLE_FOE, 0},
    [K_SWIRL]    = {"SWIRL",       1,  100,   5,  5, ROLE_FOE, 0},
    [K_ERUPTER]  = {"GEYSER",      3,  200,   6,  6, ROLE_FOE, 0},
    [K_ROCKBIG]  = {"BIG COMET",  10,  300,  10, 10, ROLE_FOE, 0},
    [K_ROCK]     = {"COMET",       2,   50,   4,  4, ROLE_FOE, 0},
    [K_BURSTER]  = {"POPPER",      4,  200,   6,  6, ROLE_FOE, 0},
    [K_DART]     = {"DART",        2,  100,   7,  3, ROLE_FOE, 0},
    [K_HOPPER]   = {"BOUNDER",     3,  200,   6,  6, ROLE_FOE, 0},
    [K_GHOST]    = {"WISP",        4,  300,   6,  7, ROLE_FOE, 0},
    [K_CART]     = {"GHOST COACH", 4,  200,   7,  5, ROLE_FOE, 0},
    [K_RAILBOMB] = {"RAIL CRAWLER",16, 500,   8,  6, ROLE_FOE, 0},
    [K_SILO]     = {"LAUNCHER",    8,  300,   6,  6, ROLE_FOE, 0},
    [K_ROCKET]   = {"ROCKET",      1,   50,   3,  3, ROLE_FOE, 0},
    [K_NIP]      = {"NIPPER",      2,  150,   5,  4, ROLE_FOE, 0},
    [K_KITE]     = {"KITEWING",    3,  200,   7,  4, ROLE_FOE, 0},
    [K_LASER]    = {"LASER GATE",  1,    0,   3, 80, ROLE_FOE, 0},
    [K_GEN]      = {"GENERATOR",  24, 1000,   8,  7, ROLE_FOE, 0},
    [K_MOVER]    = {"BLOCK",       1,    0,  12, 12, ROLE_PROP, 0},
    [K_SECRET]   = {"ODDITY",      1,    0,   6,  6, ROLE_PROP, 0},
    [K_LETTER]   = {"LETTER",      1,    0,   5,  6, ROLE_PROP, 0},
    [K_ORB]      = {"ORB",         1,    0,   4,  4, ROLE_PROP, 0},
    [K_SIGN]     = {"SIGN",        1,    0,   1,  1, ROLE_PROP, 0},
    [K_SCONE]    = {"MADAME SCONE", 200, 3000, 22, 20, ROLE_MID, 0},
    [K_TEAPOT]   = {"THE TEAPOT",  180, 8000, 20, 18, ROLE_BOSS, 0},
    [K_CROAK]    = {"OLD CROAK",   400, 5000, 18, 14, ROLE_MID, 0},
    [K_SIGNAL]   = {"THE SIGNALMAN", 200, 8000, 14, 20, ROLE_BOSS, 0},
    [K_JAW]      = {"KING THUNDERJAW", 260, 10000, 18, 14, ROLE_BOSS, 0},
    [K_CHUTE]    = {"BONE CHUTE",  14,  500,   8,  8, ROLE_PART, 0},
    [K_KALEI]    = {"THE KALEIDOSCOPE", 260, 20000, 10, 10, ROLE_BOSS, 0},
    [K_SHARD]    = {"SHARD",        1,    0,   5,  5, ROLE_PART, 0},
};

int shb_nmover;
int shb_mover[8];
static float scroll_v; /* how far the view moved this frame */

bool shb_foe_hittable(const Foe *e) {
    if (e->t < 0) return false;
    switch (e->kind) {
    case K_GHOST: return e->stun > 0 || e->state == 0;
    case K_ERUPTER: return e->state > 0;
    case K_HOPPER: return e->state > 0;
    case K_LASER: return false;
    case K_SHARD: return false;
    default: return e->role != ROLE_PROP;
    }
}

bool shb_foe_harmful(const Foe *e) {
    if (e->t < 0 || e->role == ROLE_PROP) return false;
    switch (e->kind) {
    case K_GHOST: return e->stun > 0 || e->state <= 1;
    case K_ERUPTER: return e->state > 0;
    case K_LASER: return e->state == 1;
    case K_SIGNAL: return e->stun > 0 || e->phase != 2; /* a ghost: gone, he can't touch you */
    case K_HOPPER: return e->state > 0;
    default: return true;
    }
}

/* the surface of the rock under (or over) x,y: the y to stand on */
static float snap_to(float x, float y, bool ceil, int hh) {
    if (ceil) {
        for (float yy = y; yy > 0; yy -= 1)
            if (shb_solid_at(x, yy)) return yy + hh + 1;
        return hh + 1.0f;
    }
    for (float yy = y; yy < shb_rows * SHB_TILE; yy += 1)
        if (shb_solid_at(x, yy)) return yy - hh - 1;
    return shb_rows * SHB_TILE - hh - 1.0f;
}

int shb_spawn(int kind, float x, float y, int flags, int arg) {
    for (int i = 0; i < SHB_MAX_FOES; i++) {
        Foe *e = &sb.foe[i];
        if (e->alive) continue;
        const FoeDef *d = &SHB_FOE[kind];
        memset(e, 0, sizeof *e);
        e->kind = (uint8_t)kind;
        e->alive = 1;
        e->role = d->role;
        e->flags = (uint8_t)flags;
        e->hw = d->hw;
        e->hh = d->hh;
        e->hp = e->maxhp = d->hp;
        e->value = d->points;
        e->x = x;
        e->y = y;
        e->ax = x;
        e->ay = y;
        e->arg = arg;
        if (d->mount || kind == K_TOAST || kind == K_NIP || kind == K_SILO || kind == K_GEN || kind == K_RAILBOMB)
            e->y = e->ay = snap_to(x, y, (flags & F_CEIL) != 0, e->hh);
        if (kind == K_LASER) {
            /* a gate spans the gap it stands in */
            float top = y, bot = y;
            while (top > 0 && !shb_solid_at(x, top - 1)) top -= 1;
            while (bot < shb_rows * SHB_TILE && !shb_solid_at(x, bot + 1)) bot += 1;
            e->y = (top + bot) / 2;
            e->hh = (int8_t)imin(120, (int)((bot - top) / 2));
            e->arg2 = arg >> 8;      /* the phase */
            e->arg = arg & 255 ? arg & 255 : 90;
        }
        if (d->role == ROLE_MID || d->role == ROLE_BOSS || kind == K_CHUTE) shb_boss_init(e);
        return i;
    }
    return -1;
}

/* members of a formation come one after another; things on the rock are
 * laid out along it */
static bool spaced_in_px(int kind) {
    return SHB_FOE[kind].mount || kind == K_TOAST || kind == K_NIP || kind == K_SILO || kind == K_GEN ||
           kind == K_CART || kind == K_RAILBOMB || kind == K_LASER || kind == K_SECRET || kind == K_LETTER ||
           kind == K_MOVER || kind == K_SIGN;
}

void shb_run_spawns(void) {
    const StageDef *st = &SHB_STAGE[sb.stage];
    while (sb.spawn_i < st->nspawns && st->spawns[sb.spawn_i].x <= sb.cam_x + SCREEN_W + 24) {
        const SpawnDef *d = &st->spawns[sb.spawn_i++];
        int group = ++sb.group_seq;
        for (int k = 0; k < imax(1, d->n); k++) {
            bool px = spaced_in_px(d->kind);
            float x = d->x + (px ? k * d->gap : 0);
            /* geysers and bounders burst from the top or bottom edge on
             * screen: their y in the list is the screen x of the first */
            bool edge = d->kind == K_ERUPTER || d->kind == K_HOPPER;
            if (edge) x = sb.cam_x + fminf(300.0f, (float)(d->y + k * 30));
            int flags = d->flags;
            /* only the first member of a group carries the red mark, and only
             * the last one drops a bulb when the whole group carries them */
            if ((flags & F_RED) && k != 0) flags &= ~F_RED;
            if ((flags & F_CRYSTAL) && d->n > 1 && k != d->n - 1) flags &= ~F_CRYSTAL;
            int i = shb_spawn(d->kind, x, d->y, flags, d->arg);
            if (i < 0) continue;
            Foe *e = &sb.foe[i];
            e->group = group;
            e->idx = k;
            if (!px) e->t = -k * d->gap;
            if (d->kind == K_DART) e->t -= 50;  /* the "!" first */
        }
    }
}

/* ------------------------------------------------------------------------ */
/* a photo                                                                    */

static void mark_kind(int kind) {
    if (kind < 32) sb.kinds |= 1u << kind;
    else sb.kinds2 |= 1u << (kind - 32);
}

void shb_foe_photographed(int i, int photo) {
    Foe *e = &sb.foe[i];
    /* every foe kind caught on film counts (and gets its picture in the credits) */
    if (e->role != ROLE_PROP) mark_kind(e->kind);
    switch (e->kind) {
    case K_LETTER: {
        int bit = 1 << iclamp(e->arg, 0, 2);
        if (!(sb.letters & bit)) {
            sb.letters |= (uint8_t)bit;
            sb.secrets++;
            sfx_play_name("shb_letter");
            static const char *const LIT[3] = {"THE U LIGHTS UP!", "THE F LIGHTS UP!", "THE O LIGHTS UP!"};
            sb.msg = LIT[iclamp(e->arg, 0, 2)];
            sb.msg_t = 120;
            shb_burst(e->x, e->y, C_YELLOW, 16, 1.4f);
        }
        return;
    }
    case K_SECRET:
        if (e->state == 0) {
            e->state = 1;
            sb.secrets++;
            sfx_play_name("shb_secret");
            for (int k = 0; k < 5; k++) {
                int j = shb_spawn(K_ORB, e->x, e->y, 0, 0);
                if (j < 0) continue;
                float a = k * 1.2566f;
                sb.foe[j].vx = cosf(a) * 1.1f;
                sb.foe[j].vy = sinf(a) * 1.1f;
            }
        }
        return;
    case K_MOVER:
        e->frozen = true;
        e->stun = SHB_STUN_T;
        return;
    case K_ORB: case K_SIGN: case K_LASER:
        return;
    default: break;
    }
    if (e->role == ROLE_PROP) return;
    if (e->role == ROLE_MID || e->role == ROLE_BOSS || e->role == ROLE_PART) {
        shb_boss_photo(i, photo);
        return;
    }
    if (e->flags & F_RED) {
        /* the red one turns into a wrench */
        e->alive = 0;
        shb_drop(PK_WRENCH, e->x, e->y);
        shb_burst(e->x, e->y, C_RED, 10, 1.2f);
        sfx_play_name("shb_repair");
        return;
    }
    e->stun = SHB_STUN_T;
    e->photo = photo;
    if (SHB_FOE[e->kind].mount && !e->falling) {
        /* knocked off its rock */
        e->falling = true;
        e->vx = 0;
        e->vy = (e->flags & F_CEIL) ? 0.2f : -1.6f;
    }
    if (e->kind == K_CART) {
        /* one coach in the picture stops the whole train */
        for (int j = 0; j < SHB_MAX_FOES; j++) {
            Foe *o = &sb.foe[j];
            if (o->alive && o->kind == K_CART && o->group == e->group) { o->stun = SHB_STUN_T; o->photo = photo; }
        }
    }
}

/* ------------------------------------------------------------------------ */
/* each foe, a frame at a time                                                */

static bool on_screen(const Foe *e, int m) {
    return e->x > sb.cam_x - m && e->x < sb.cam_x + SCREEN_W + m && e->y > sb.cam_y - m && e->y < sb.cam_y + SHB_PF_H + m;
}

static int nearest(const Foe *e) { return shb_near_ship(e->x, e->y); }

static void fall_update(int i) {
    Foe *e = &sb.foe[i];
    e->vy = fminf(e->vy + 0.15f, 4.0f);
    e->y += e->vy;
    /* it lands on whatever is under it */
    for (int j = 0; j < SHB_MAX_FOES; j++) {
        Foe *o = &sb.foe[j];
        if (j == i || !o->alive || o->role != ROLE_FOE || o->t < 0 || o->kind == K_LASER) continue;
        if (fabsf(o->x - e->x) < o->hw + e->hw && fabsf(o->y - e->y) < o->hh + e->hh) {
            o->stun = imax(o->stun, 1);
            o->photo = -1;
            shb_kill_foe(j, true);
        }
    }
    if ((e->vy > 0 && shb_solid_at(e->x, e->y + e->hh)) || e->y > sb.cam_y + SHB_PF_H + 20) shb_kill_foe(i, false);
}

static void foe_update(int i) {
    Foe *e = &sb.foe[i];
    if (e->t < 0) {
        e->t++;
        e->x += scroll_v; /* waiting off screen, it keeps up with the view */
        return;
    }
    if (e->flash > 0) e->flash--;
    if (e->role == ROLE_MID || e->role == ROLE_BOSS || e->kind == K_CHUTE || e->kind == K_SHARD) {
        shb_boss_update(i);
        return;
    }
    if (e->falling) { fall_update(i); return; }
    if (e->stun > 0) {
        e->stun--;
        if (e->kind == K_MOVER && e->stun == 0) e->frozen = false;
        return;
    }
    if (e->still) return;
    e->t++;
    int p = nearest(e);
    float px = p >= 0 ? sb.ship[p].x : e->x - 100, py = p >= 0 ? sb.ship[p].y : e->y;
    float sx = e->x - sb.cam_x; /* where it is on screen */
    bool vis = on_screen(e, 0);
    switch (e->kind) {
    case K_PUFF:
        e->x += -0.6f;
        e->y = e->ay + sinf(e->t * 0.05f + e->idx) * 10;
        if (e->arg == 1 && e->t == 90) shb_aimed(e->x, e->y, 1.4f, 1, 0);
        break;
    case K_MINT:
        e->x += -1.1f;
        e->y += fclamp(py - e->y, -0.35f, 0.35f);
        if (e->arg && vis && e->t % 150 == 60) shb_aimed(e->x, e->y, 1.6f, 1, 0);
        break;
    case K_TURRET:
        if (vis && sx > 40 && (e->t + e->idx * 37) % 120 == 0) shb_aimed(e->x, e->y, 1.7f, e->arg ? 3 : 1, 14);
        break;
    case K_WALLBOMB:
        if (e->state == 0) {
            if (vis && fabsf(px - e->x) < 90 && e->t > 20) {
                float a = atan2f(py - e->y, px - e->x);
                e->vx = cosf(a) * 1.7f;
                e->vy = sinf(a) * 1.7f;
                e->state = 1;
                shb_sfx("shb_cling", 8);
            }
        } else {
            e->x += e->vx;
            e->y += e->vy;
        }
        break;
    case K_CHOMPER: {
        /* drops on a cord when you pass under, then reels back up */
        bool under = fabsf(px - e->x) < 22 && py > e->ay;
        if (e->state == 0 && under && e->t > 30) { e->state = 1; e->st = 0; }
        if (e->state == 1) {
            e->st++;
            e->y = e->ay + fminf(e->st * 3.0f, 44.0f);
            if (e->st > 40) e->state = 2;
        } else if (e->state == 2) {
            e->y = fmaxf(e->ay, e->y - 1.0f);
            if (e->y <= e->ay) { e->state = 0; e->t = 0; }
        }
        break;
    }
    case K_CUBE:
        e->x += -1.3f;
        e->y = e->ay + sinf(e->t * 0.055f) * 26;
        break;
    case K_TOAST:
        e->x += -0.35f * (e->arg ? -1 : 1);
        if (vis && (e->t + e->idx * 41) % 110 == 50) {
            int j = shb_add_eshot(ES_CRUMB, e->x, e->y, (px < e->x ? -1.0f : 1.0f) * 1.2f, (e->flags & F_CEIL) ? 1.0f : -2.2f);
            if (j >= 0) sb.es[j].bounces = 3;
            shb_sfx("shb_pop2", 6);
        }
        break;
    case K_SWIRL:
        if (e->state == 0) {
            e->x += -1.8f + scroll_v;
            if (sx < (e->arg ? e->arg : 190)) { e->state = 1; e->st = 0; e->ax = e->x; e->ay = e->y; }
        } else if (e->state == 1) {
            e->st++;
            float a = e->st * 0.07f;
            e->x = e->ax - sinf(a) * 26 + scroll_v * e->st;
            e->y = e->ay + (1 - cosf(a)) * 26 * ((e->flags & F_TOP) ? -1 : 1);
            if (e->st == 45 && e->idx % 2 == 0) shb_aimed(e->x, e->y, 1.5f, 1, 0);
            if (a > 6.2832f) e->state = 2;
        } else {
            e->x += -2.0f + scroll_v;
        }
        break;
    case K_ERUPTER: {
        bool top = (e->flags & F_TOP) != 0;
        if (e->state == 0) {
            /* the warning: a burst rising at the edge */
            e->y = top ? sb.cam_y - 8 : sb.cam_y + SHB_PF_H + 8;
            e->x += scroll_v;
            if (e->t >= 45) { e->state = 1; e->vy = top ? 3.4f : -3.4f; e->vx = -0.7f; }
        } else {
            e->vy += top ? -0.06f : 0.06f;
            e->x += e->vx + scroll_v;
            e->y += e->vy;
            if (e->state == 1 && fabsf(e->vy) < 0.3f) { e->state = 2; shb_aimed(e->x, e->y, 1.7f, 1, 0); }
        }
        break;
    }
    case K_ROCKBIG: case K_ROCK:
        if (e->vx == 0 && e->vy == 0) { e->vx = -0.9f - (e->idx % 3) * 0.2f; e->vy = (e->arg - 2) * 0.12f; }
        e->x += e->vx + scroll_v;
        e->y += e->vy;
        if (e->y < sb.cam_y + 4 || e->y > sb.cam_y + SHB_PF_H - 4) e->vy = -e->vy;
        break;
    case K_BURSTER: {
        float goal = sb.cam_x + (e->arg ? e->arg : 220);
        e->x += scroll_v + fclamp(goal - e->x, -1.6f, 0);
        if (e->t >= 240) {
            /* left alive too long: it bursts */
            shb_ring(e->x, e->y, 1.4f, 10, e->t * 0.1f, ES_SHOT);
            shb_burst(e->x, e->y, C_PINK, 12, 1.4f);
            shb_sfx("shb_burst", 4);
            e->alive = 0;
        }
        break;
    }
    case K_DART:
        if (e->state == 0) {
            e->state = 1;
            e->x = sb.cam_x - 12;
        }
        if (e->state == 1) {
            e->x += 4.2f + scroll_v;
            if ((e->flags & F_LAST) && e->x - sb.cam_x >= 170) { e->state = 2; e->st = 0; }
            if (e->x > sb.cam_x + SCREEN_W + 20) e->alive = 0; /* gone off the right */
        } else if (e->state == 2) {
            e->x += scroll_v;
            if (++e->st > 50) e->state = 3;
        } else {
            e->x += -2.4f + scroll_v;
            if (e->x < sb.cam_x - 20) e->alive = 0;
        }
        break;
    case K_HOPPER: {
        /* like a geyser, a rising burst at the edge first; then it springs
         * out and bounces from edge to edge, steeper than a geyser's arc */
        bool top = (e->flags & F_TOP) != 0;
        if (e->state == 0) {
            e->y = top ? sb.cam_y - 8 : sb.cam_y + SHB_PF_H + 8;
            e->x += scroll_v;
            if (e->t >= 45) { e->state = 1; e->vy = top ? 2.9f : -2.9f; e->vx = -1.1f; }
            break;
        }
        e->x += e->vx + scroll_v;
        e->y += e->vy;
        /* each bounce a little lower and quicker, then high again: hard to read */
        float kick = 2.9f - (e->st % 3) * 0.5f;
        if (e->y < sb.cam_y + 8 && e->vy < 0) { e->y = sb.cam_y + 8; e->vy = kick; e->st++; }
        if (e->y > sb.cam_y + SHB_PF_H - 8 && e->vy > 0) { e->y = sb.cam_y + SHB_PF_H - 8; e->vy = -kick; e->st++; }
        break;
    }
    case K_GHOST: {
        /* 0 there, 1 fading, 2 gone (can't be touched or hurt), 3 coming back */
        int c = (e->t + e->idx * 50) % 240;
        e->state = c < 90 ? 0 : c < 120 ? 1 : c < 210 ? 2 : 3;
        float a = atan2f(py - e->y, px - e->x);
        e->x += cosf(a) * 0.45f + scroll_v * 0.5f;
        e->y += sinf(a) * 0.45f;
        if (e->state == 0 && vis && c == 60) shb_aimed(e->x, e->y, 1.5f, 1, 0);
        break;
    }
    case K_CART:
        e->x += e->arg ? 1.2f : -1.0f;
        if (e->idx == 0 && vis && e->t % 130 == 70) shb_aimed(e->x, e->y - 4, 1.6f, 1, 0);
        break;
    case K_RAILBOMB:
        e->x += -0.3f;
        if (vis && (e->t + e->idx * 30) % 100 == 40) shb_aimed(e->x, e->y, 1.5f, 3, 18);
        break;
    case K_SILO:
        if (vis && sx > 30 && (e->t + e->idx * 60) % 160 == 70) {
            int j = shb_spawn(K_ROCKET, e->x, e->y - 8, 0, 0);
            if (j >= 0) { sb.foe[j].vx = 0; sb.foe[j].vy = (e->flags & F_CEIL) ? 1.6f : -1.6f; }
            shb_sfx("shb_launch", 6);
        }
        break;
    case K_ROCKET: {
        float want = atan2f(py - e->y, px - e->x), have = atan2f(e->vy, e->vx);
        float d = want - have;
        while (d > 3.14159f) d -= 6.28318f;
        while (d < -3.14159f) d += 6.28318f;
        if (e->t < 150) have += fclamp(d, -0.035f, 0.035f);
        e->vx = cosf(have) * 1.6f;
        e->vy = sinf(have) * 1.6f;
        e->x += e->vx;
        e->y += e->vy;
        if (e->t > 260 || shb_solid_at(e->x, e->y)) { shb_burst(e->x, e->y, C_ORANGE, 5, 1.0f); e->alive = 0; }
        break;
    }
    case K_NIP:
        if (e->state == 0) {
            e->x += px < e->x ? -0.25f : 0.25f;
            if (fabsf(px - e->x) < 80 && e->t > 50 && vis) {
                e->state = 1;
                e->vy = -3.3f;
                e->vx = fclamp((px - e->x) / 40.0f, -1.6f, 1.6f);
            }
        } else {
            e->vy += 0.12f;
            e->x += e->vx;
            e->y += e->vy;
            if (e->vy > 0 && shb_solid_at(e->x, e->y + e->hh + 1)) {
                e->y = snap_to(e->x, e->y - 4, false, e->hh);
                e->state = 0;
                e->t = 0;
            }
            if (e->y > sb.cam_y + SHB_PF_H + 30) e->alive = 0;
        }
        break;
    case K_KITE:
        e->x += -0.9f + scroll_v * 0.3f;
        e->y = e->ay + sinf(e->t * 0.04f) * 6;
        if (vis && fabsf(px - e->x) < 34 && py > e->y && e->t - e->st > 70) {
            e->st = e->t;
            shb_add_eshot(ES_ARROW, e->x, e->y + 5, 0, 0.5f);
            shb_sfx("shb_drop", 6);
        }
        break;
    case K_LASER: {
        int c = (e->t + e->arg2) % (e->arg * 2);
        e->state = c < e->arg ? 1 : 0;
        if (c == e->arg - 20 && vis) shb_sfx("shb_hum", 20);
        break;
    }
    case K_GEN:
        if (vis && (e->t + e->idx * 50) % 140 == 0) shb_aimed(e->x, e->y, 1.4f, 1, 0);
        break;
    case K_MOVER: {
        if (e->frozen) break;
        float oy = e->y;
        e->y = e->ay + sinf(e->t * 0.025f + e->arg2) * e->arg;
        float dy = e->y - oy;
        /* it shoves the ships, and pins them against the rock */
        for (int q = 0; q < 2; q++) {
            Ship *s = &sb.ship[q];
            if (!s->on || !s->alive) continue;
            if (fabsf(s->x - e->x) < e->hw + SHB_BOX_W && fabsf(s->y - e->y) < e->hh + SHB_BOX_H) {
                s->y += dy > 0 ? (e->y + e->hh + SHB_BOX_H + 0.5f) - s->y : (e->y - e->hh - SHB_BOX_H - 0.5f) - s->y;
                bool pinned = false;
                for (float xx = s->x - SHB_BOX_W; xx <= s->x + SHB_BOX_W; xx += SHB_BOX_W)
                    for (float yy = s->y - SHB_BOX_H; yy <= s->y + SHB_BOX_H; yy += SHB_BOX_H)
                        if (tile_solid(shb_tile_at(xx, yy))) pinned = true;
                if (pinned && s->ghost == 0) { shb_hurt_ship(q); s->ghost = 1; }
            }
        }
        break;
    }
    case K_SECRET: case K_LETTER: case K_SIGN:
        if (e->kind == K_SIGN && sx < 220 && e->state == 0) { e->state = 1; sb.tip = e->arg; sb.tip_t = 0; }
        break;
    case K_ORB:
        e->x += e->vx;
        e->y += e->vy;
        e->vx *= 0.97f;
        e->vy *= 0.97f;
        if (e->t > 420) e->alive = 0;
        break;
    default: break;
    }
}

void shb_foes_update(void) {
    static float prev = 0;
    scroll_v = sb.cam_x - prev;
    if (scroll_v < 0 || scroll_v > 4 || sb.stage_t <= 1) scroll_v = 0;
    prev = sb.cam_x;
    shb_nmover = 0;
    for (int i = 0; i < SHB_MAX_FOES && shb_nmover < 8; i++)
        if (sb.foe[i].alive && sb.foe[i].kind == K_MOVER) shb_mover[shb_nmover++] = i;
    for (int i = 0; i < SHB_MAX_FOES; i++) {
        Foe *e = &sb.foe[i];
        if (!e->alive) continue;
        foe_update(i);
        if (!e->alive) continue;
        bool boss = e->role == ROLE_MID || e->role == ROLE_BOSS || e->role == ROLE_PART;
        /* gone past the left edge, or far off the top or bottom */
        if (!boss && e->t >= 0 && (e->x < sb.cam_x - 48 || e->y < sb.cam_y - 90 || e->y > sb.cam_y + SHB_PF_H + 90)) {
            if (e->kind == K_LETTER && e->x > sb.cam_x - 48) continue;
            e->alive = 0;
        }
    }
}
