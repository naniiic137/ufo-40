/* CLARION CALL - the gear, the player's bar, tank and purse, and the walker
 * Clary uses on a map and behind a door.
 *
 * Items show in the game as icons with a price under them and no names (as
 * the original's do); the names here are for the docs, the tests and the
 * credits. */
#include "clc.h"

/* prices: the sixteen upgrades, then the stats, the consumables, the key and
 * the sexton's sheet (the last ones are never sold) */
const int16_t CLC_PRICE[IT_COUNT] = {
    100, 250, 150, 250, 300,      /* twin swipe, seeker bells, bounce beam, fuel siphon, spit gun */
    100, 200, 150, 250, 300, 150, /* feather boots, fan shot, ghost lead, creeper caps, big bang, lucky thimble */
    400, 200, 100,                /* homing charm (100 at the Arboretum's stall), magnet, door dowser */
    200, 800,                     /* penny purse, tin plate */
    150, 150,                     /* heart pin, spare tank */
    100, 100, 250,                /* toffee, fuel flask, fuel drum */
    0, 0, 0,                      /* money sack, yellow key, peal sheet */
};

const char *const CLC_ITEM_NAME[IT_COUNT] = {
    "TWIN SWIPE", "SEEKER BELLS", "BOUNCE BEAM", "FUEL SIPHON", "SPIT GUN",
    "FEATHER BOOTS", "FAN SHOT", "GHOST LEAD", "CREEPER CAPS", "BIG BANG", "LUCKY THIMBLE",
    "HOMING CHARM", "MAGNET", "DOOR DOWSER", "PENNY PURSE", "TIN PLATE",
    "HEART PIN", "SPARE TANK", "TOFFEE", "FUEL FLASK", "FUEL DRUM", "MONEY SACK", "YELLOW KEY", "PEAL SHEET",
};

bool clc_has(const ClcPlayer *p, int g) { return g >= 0 && g < G_COUNT && (p->gear >> g & 1u); }

void clc_hurt(ClcPlayer *p, int half) {
    if (half <= 0 || p->dead) return;
    if (clc_has(p, G_PLATE)) half = (half + 1) / 2;
    p->hp = (int16_t)(p->hp - half);
    if (p->hp <= 0) {
        p->hp = 0;
        p->dead = 1;
    }
}

void clc_heal(ClcPlayer *p, int half) { p->hp = (int16_t)imin(p->hpmax, p->hp + half); }

void clc_add_fuel(ClcPlayer *p, int n) { p->fuel = (int16_t)imin(p->fuelmax, p->fuel + n); }

int clc_coin_drop(const ClcPlayer *p, int n) { return clc_has(p, G_PURSE) ? n + (n + 1) / 2 : n; }

void clc_give(ClcPlayer *p, int item) {
    if (item < 0) return;
    if (item < G_COUNT) { p->gear |= 1u << item; return; }
    switch (item) {
    case IT_HEARTPIN: p->hpmax = (int16_t)(p->hpmax + 4); p->hp = (int16_t)(p->hp + 4); break;
    case IT_SPARETANK: p->fuelmax = (int16_t)(p->fuelmax + 200); clc_add_fuel(p, 200); break;
    case IT_TOFFEE: clc_heal(p, 10); break;
    case IT_FLASK: clc_add_fuel(p, 500); break;
    case IT_DRUM: clc_add_fuel(p, 1000); break;
    case IT_SACK: p->coins += 200; break;
    case IT_KEY: p->key = 1; break;
    default: break; /* the peal sheet: the run keeps count */
    }
}

/* ---- the walker ----------------------------------------------------------- */

/* On a map Clary is small: 6 x 8, a jump of about two and a half tiles. In
 * the closer view she is twice that, with the same feel. */
const ClcWalkTune CLC_WALK_MAP = {3, 8, 192, 48, 40, 624, 220, 768, 160};
const ClcWalkTune CLC_WALK_SUB = {5, 15, 330, 80, 62, 1140, 400, 1280, 260};

static int PXF(int32_t v) { return v >= 0 ? (int)(v >> 8) : -(int)((-v + 255) >> 8); }

static bool row_hit(const ClcWalkTune *t, ClcSolidFn solid, const void *ctx, int x, int py, bool feet) {
    int x0 = x - t->hw, x1 = x + t->hw - 1;
    for (int px = x0;; px += 3) {
        if (px > x1) px = x1;
        if (solid(ctx, px, py, feet)) return true;
        if (px == x1) break;
    }
    return false;
}

static bool col_hit(const ClcWalkTune *t, ClcSolidFn solid, const void *ctx, int px, int y, int h) {
    for (int py = y - h;; py += 3) {
        if (py > y - 1) py = y - 1;
        if (solid(ctx, px, py, false)) return true;
        if (py == y - 1) break;
    }
    return false;
}

bool clc_walker_blocked(const ClcWalkTune *t, ClcSolidFn solid, const void *ctx, int32_t x, int32_t y) {
    int cx = PXF(x), cy = PXF(y);
    return col_hit(t, solid, ctx, cx - t->hw, cy, t->h) || col_hit(t, solid, ctx, cx + t->hw - 1, cy, t->h);
}

static bool on_ladder(ClcLadderFn ladder, const void *ctx, int x, int y) { return ladder && ladder(ctx, x, y); }

void clc_walk(ClcWalker *w, const ClcWalkTune *t, unsigned held, unsigned pressed, unsigned flags,
              ClcSolidFn solid, ClcLadderFn ladder, const void *ctx) {
    int cx = PXF(w->x), cy = PXF(w->y);
    int dir = ((held & BTN_RIGHT) ? 1 : 0) - ((held & BTN_LEFT) ? 1 : 0);
    w->landed = 0;
    if (w->drop_t) w->drop_t--;
    bool feet = w->drop_t == 0;
    int h = w->crouch ? t->h * 5 / 8 : t->h;

    /* ladders: up or down onto one, climb, jump off */
    if (!w->ladder && ladder) {
        bool up = (held & BTN_UP) && on_ladder(ladder, ctx, cx, cy - t->h / 2);
        bool down = (held & BTN_DOWN) && w->ground && on_ladder(ladder, ctx, cx, cy + 1);
        if (up || down) {
            w->ladder = 1;
            w->x = (int32_t)(((cx / CLC_T) * CLC_T + CLC_T / 2) * 256);
            w->vx = w->vy = 0;
            w->crouch = 0;
            if (down) w->drop_t = 6;
        }
    }
    if (w->ladder) {
        cx = PXF(w->x);
        w->vx = 0;
        w->vy = (held & BTN_UP) ? -t->climb : (held & BTN_DOWN) ? t->climb : 0;
        if (pressed & BTN_A) {
            w->ladder = 0;
            w->vy = -t->jump * 2 / 3;
            w->jumping = 1;
            w->vx = dir * t->walk;
            if (dir) w->face = (int8_t)dir;
        } else if (!on_ladder(ladder, ctx, cx, cy - 1) && !on_ladder(ladder, ctx, cx, cy - t->h / 2)) {
            w->ladder = 0;
        }
    }

    w->crouch = !w->ladder && w->ground && (held & BTN_DOWN) && !(pressed & BTN_A) ? 1 : 0;
    if (!w->ladder) {
        /* walking (or sliding on ice); a crouch stands still */
        int32_t target = w->crouch ? 0 : dir * t->walk;
        int32_t acc = (flags & WK_ICE) && w->ground ? t->accel / 8 : t->accel;
        if (w->vx < target) w->vx = imin(target, w->vx + acc);
        else if (w->vx > target) w->vx = imax(target, w->vx - acc);
        /* she only turns on the ground */
        if (dir && (w->ground || w->ladder)) w->face = (int8_t)dir;
        /* jumping: down + jump on a thin floor drops through it */
        if ((pressed & BTN_A) && w->ground) {
            if ((held & BTN_DOWN) && !solid(ctx, cx, cy, false) && solid(ctx, cx, cy, true)) {
                w->drop_t = 10;
                w->ground = 0;
            } else {
                w->vy = -t->jump;
                w->jumping = 1;
                w->ground = 0;
            }
            w->crouch = 0;
        }
        if (w->jumping && !(held & BTN_A) && w->vy < -t->jump_cut) w->vy = -t->jump_cut;
        if (w->vy >= 0) w->jumping = 0;
        int32_t g = t->gravity, maxf = t->max_fall;
        if ((flags & WK_FEATHER) && (held & BTN_A) && w->vy > 0) {
            g /= 3;
            maxf /= 3;
        }
        w->vy = imin(maxf, w->vy + g);
    }
    h = w->crouch ? t->h * 5 / 8 : t->h;

    /* move across, a pixel at a time */
    int32_t nx = w->x + w->vx;
    int sx = PXF(w->x), ex = PXF(nx);
    while (sx != ex) {
        int step = ex > sx ? 1 : -1;
        int edge = step > 0 ? sx + t->hw : sx - t->hw - 1;
        if (col_hit(t, solid, ctx, edge, PXF(w->y), h)) {
            w->vx = 0;
            nx = (int32_t)sx * 256 + (step > 0 ? 255 : 0);
            break;
        }
        sx += step;
    }
    w->x = nx;
    cx = PXF(w->x);

    /* and up or down */
    int32_t ny = w->y + w->vy;
    int sy = PXF(w->y), ey = PXF(ny);
    bool was_ground = w->ground;
    w->ground = 0;
    while (sy != ey) {
        if (ey > sy) {
            /* the row under the feet */
            if (row_hit(t, solid, ctx, cx, sy, feet)) {
                ny = (int32_t)sy * 256;
                w->vy = 0;
                w->ground = 1;
                break;
            }
            sy++;
        } else {
            if (row_hit(t, solid, ctx, cx, sy - h - 1, false)) {
                ny = (int32_t)sy * 256;
                w->vy = 0;
                break;
            }
            sy--;
        }
    }
    w->y = ny;
    cy = PXF(w->y);
    if (!w->ground && w->vy >= 0 && row_hit(t, solid, ctx, cx, cy, feet)) {
        w->ground = 1;
        if (w->vy > 0) w->vy = 0;
    }
    if (w->ladder) {
        if (w->ground && w->vy > 0) w->ladder = 0;
        w->fall_from = w->y;
    }
    if (w->ground) {
        if (!was_ground) {
            w->landed = 1;
            w->drop = (int16_t)(cy - PXF(w->fall_from));
            w->jumping = 0;
        }
        w->fall_from = w->y;
    }
    if (w->shot_cd) w->shot_cd--;
    if (w->inv) w->inv--;
}
