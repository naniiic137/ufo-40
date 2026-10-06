/* FORLORN HOPE - the seventeen kinds of foe, the thorn hearts, and the
 * drains, combs and bells that send out more. Foes keep their wounds and
 * stay dead for the whole run; only what came out of a drain, a comb or a
 * bell is swept away when a volunteer is lost. Nothing here rolls dice:
 * every foe does the same thing every run. */
#include "forlorn.h"

const char *const FRL_FOE_NAME[FK_COUNT] = {
    "", "WALL-EYE", "OOZLE", "MIDGE", "HORNET", "SHELLBACK", "HATCHETEER", "SQUAWKER",
    "TUSKER", "IDOL", "DRAKE", "HORNET BELL", "RUST KNIGHT", "BLOATER", "BROODHEN",
    "HORNHEAD", "STINGBACK", "GULPER", "THORN HEART",
};
/* hit points (the original's table): 0 = can't be hurt */
const int FRL_FOE_HP[FK_COUNT] = {0, 1, 1, 1, 2, 2, 3, 3, 6, 10, 10, 10, 10, 30, 50, 50, 50, 0, 30};

void frl_foe_size(int kind, int *bw, int *bh) {
    static const int8_t S[FK_COUNT][2] = {
        {0, 0}, {10, 10}, {8, 5}, {6, 6}, {8, 7}, {10, 8}, {10, 12}, {8, 8},
        {16, 10}, {12, 12}, {16, 12}, {12, 14}, {10, 14}, {20, 20}, {18, 18},
        {18, 22}, {22, 14}, {20, 10}, {20, 20},
    };
    *bw = S[kind][0];
    *bh = S[kind][1];
}

int frl_foe_add(FrlWorld *w, int kind, int px, int py) {
    /* the map's foes take the first slots; minions reuse any free one after */
    int i = -1;
    for (int k = w->nplaced; k < FRL_FOES; k++)
        if (!w->foe[k].on) { i = k; break; }
    if (i < 0) return -1;
    FrlFoe *f = &w->foe[i];
    memset(f, 0, sizeof *f);
    int bw, bh;
    frl_foe_size(kind, &bw, &bh);
    f->kind = (uint8_t)kind;
    f->on = 1;
    f->w = (int16_t)bw;
    f->h = (int16_t)bh;
    f->x = px << 8;
    f->y = py << 8;
    f->hx = (int16_t)px;
    f->hy = (int16_t)py;
    f->hp = (int16_t)FRL_FOE_HP[kind];
    f->face = -1;
    f->owner = -1;
    if (i + 1 > w->nfoe) w->nfoe = i + 1;
    return i;
}

bool frl_foe_deadly(const FrlFoe *f) { return f->kind != FK_WALLEYE; }

static int fcx(const FrlFoe *f) { return (f->x >> 8) + f->w / 2; }
static int fcy(const FrlFoe *f) { return (f->y >> 8) + f->h / 2; }

/* where the foes look: the volunteer, if one is out */
static bool target(const FrlWorld *w, int *x, int *y) {
    if (w->phase != FWP_PLAY || w->u.mode != FUM_WALK) return false;
    *x = frl_unit_cx(w);
    *y = frl_unit_cy(w);
    return true;
}

static int focus_x(const FrlWorld *w) { return w->phase == FWP_SELECT ? w->base_x : frl_unit_cx(w); }
static int focus_y(const FrlWorld *w) { return w->phase == FWP_SELECT ? w->base_y : frl_unit_cy(w); }

bool frl_foe_active(const FrlWorld *w, const FrlFoe *f) {
    return iabs(fcx(f) - focus_x(w)) < 240 && iabs(fcy(f) - focus_y(w)) < 150;
}

/* within (dx, dy) of the volunteer: the distances to it */
static bool sees(const FrlWorld *w, const FrlFoe *f, int rx, int ry, int *dx, int *dy) {
    int x, y;
    if (!target(w, &x, &y)) return false;
    *dx = x - fcx(f);
    *dy = y - fcy(f);
    return iabs(*dx) <= rx && iabs(*dy) <= ry;
}

static void shoot_at(FrlWorld *w, const FrlFoe *f, int kind, int speed, int life) {
    int x, y;
    if (!target(w, &x, &y)) return;
    int32_t vx, vy;
    frl_aim(fcx(f) << 8, fcy(f) << 8, x << 8, y << 8, speed, &vx, &vy);
    frl_shot_add(w, kind, fcx(f) << 8, fcy(f) << 8, vx, vy, life, false);
    w->ev |= FEV_FOE_SHOT;
}

/* a shot at an angle (1/256 turns), from the foe's middle */
static const int16_t SIN64[65] = {
    0, 6, 13, 19, 25, 31, 38, 44, 50, 56, 62, 68, 74, 80, 86, 92, 98, 103, 109, 115, 120, 126, 131, 136,
    142, 147, 152, 157, 162, 167, 171, 176, 181, 185, 189, 193, 197, 201, 205, 209, 212, 216, 219, 222,
    225, 228, 231, 234, 236, 238, 241, 243, 244, 246, 248, 249, 251, 252, 253, 254, 254, 255, 255, 256, 256,
};
static int isin(int a) {
    a &= 255;
    if (a < 64) return SIN64[a];
    if (a < 128) return SIN64[128 - a];
    if (a < 192) return -SIN64[a - 128];
    return -SIN64[256 - a];
}
static int icos(int a) { return isin(a + 64); }

static void shoot_angle(FrlWorld *w, const FrlFoe *f, int kind, int ang, int speed, int life) {
    frl_shot_add(w, kind, fcx(f) << 8, fcy(f) << 8, icos(ang) * speed / 256, isin(ang) * speed / 256, life, false);
    w->ev |= FEV_FOE_SHOT;
}

static int angle_to(int dx, int dy) {
    /* a rough angle in 1/256 turns, enough to fan shots about */
    int best = 0, bd = 1 << 30;
    for (int a = 0; a < 256; a += 4) {
        int c = icos(a) * dy - isin(a) * dx;
        int dot = icos(a) * dx + isin(a) * dy;
        if (dot <= 0) continue;
        if (iabs(c) < bd) { bd = iabs(c); best = a; }
    }
    return best;
}

/* walking: gravity, the box against the map, and whether it bumped */
static bool foe_move(FrlWorld *w, FrlFoe *f, bool gravity, bool *hit_wall) {
    if (gravity) f->vy = imin(f->vy + FRL_GRAV, FRL_FALL_MAX);
    *hit_wall = false;
    int32_t nx = f->x + f->vx;
    int px = f->x >> 8, py = f->y >> 8, tx = nx >> 8;
    while (px != tx) {
        int s = tx > px ? 1 : -1;
        if (frl_box_solid(w, px + s, py, f->w, f->h) || px + s < 0 || px + s + f->w > FRL_MW * FRL_T) {
            nx = (int32_t)(s > 0 ? (px << 8) | 0xFF : px << 8);
            *hit_wall = true;
            break;
        }
        px += s;
    }
    f->x = nx;
    int32_t ny = f->y + f->vy;
    px = f->x >> 8;
    int ty = ny >> 8;
    while (py != ty) {
        int s = ty > py ? 1 : -1;
        if (frl_box_solid(w, px, py + s, f->w, f->h)) {
            ny = (int32_t)(s > 0 ? (py << 8) | 0xFF : py << 8);
            f->vy = 0;
            break;
        }
        py += s;
    }
    f->y = ny;
    f->ground = frl_box_solid(w, f->x >> 8, (f->y >> 8) + 1, f->w, f->h);
    return f->ground;
}

/* is there floor ahead (so a walker turns at a ledge)? */
static bool floor_ahead(const FrlWorld *w, const FrlFoe *f) {
    int px = f->face > 0 ? (f->x >> 8) + f->w : (f->x >> 8) - 1;
    return frl_box_solid(w, px, (f->y >> 8) + f->h, 1, 1);
}

static void walker(FrlWorld *w, FrlFoe *f, int speed, bool turn_at_edges) {
    if (f->ground && turn_at_edges && !floor_ahead(w, f)) f->face = (int8_t)-f->face;
    f->vx = f->face * speed;
    bool wall;
    foe_move(w, f, true, &wall);
    if (wall) f->face = (int8_t)-f->face;
}

static int count_owned(const FrlWorld *w, int kind, int owner_tag) {
    int n = 0;
    for (int i = w->nplaced; i < w->nfoe; i++)
        n += w->foe[i].on && w->foe[i].minion && w->foe[i].kind == kind && w->foe[i].owner == owner_tag;
    return n;
}

/* ------------------------------------------------------------------ */
/* each kind                                                            */

static void step_foe(FrlWorld *w, int i) {
    FrlFoe *f = &w->foe[i];
    int dx = 0, dy = 0;
    f->t++;
    if (f->flash) f->flash--;
    bool wall;
    switch (f->kind) {
    case FK_WALLEYE:
        /* sits in the rock and shoots when you're close */
        if (sees(w, f, 80, 80, &dx, &dy)) {
            if (f->t2 == 0) f->t2 = 40; /* opens its eye first */
            if (--f->t2 == 0) {
                shoot_at(w, f, FS_PELLET, 384, 120);
                f->t2 = 110;
            }
            f->state = 1;
        } else {
            f->state = 0;
            f->t2 = 0;
        }
        break;
    case FK_OOZLE:
        /* crawls along, falls off edges, turns at walls */
        if (f->ground) f->vx = f->face * 96;
        else f->vx = f->vx * 15 / 16;
        foe_move(w, f, true, &wall);
        if (wall) f->face = (int8_t)-f->face;
        break;
    case FK_MIDGE: case FK_HORNET: {
        /* fly at the volunteer, through anything */
        int sp = f->kind == FK_MIDGE ? 110 : 180;
        int x, y;
        if (target(w, &x, &y)) {
            int32_t vx, vy;
            frl_aim(fcx(f) << 8, fcy(f) << 8, x << 8, y << 8, sp, &vx, &vy);
            f->vx = (f->vx * 7 + vx) / 8;
            f->vy = (f->vy * 7 + vy) / 8;
            f->face = (int8_t)(vx < 0 ? -1 : 1);
        } else {
            f->vx = f->vx * 15 / 16;
            f->vy = f->vy * 15 / 16;
        }
        f->x += f->vx;
        f->y += f->vy + (f->kind == FK_MIDGE ? isin((int)f->t * 6) / 3 : isin((int)f->t * 10) / 4);
        break;
    }
    case FK_SHELLBACK:
        walker(w, f, 90, true);
        break;
    case FK_HATCHET:
        /* turns to you and throws an axe that arcs over */
        foe_move(w, f, true, &wall);
        if (sees(w, f, 150, 90, &dx, &dy)) {
            f->face = (int8_t)(dx < 0 ? -1 : 1);
            if (++f->t2 >= 120) {
                f->t2 = 0;
                int vx = iclamp(dx * 256 / 60, -420, 420);
                if (iabs(vx) < 160) vx = f->face * 160;
                frl_shot_add(w, FS_AXE, fcx(f) << 8, ((f->y >> 8) + 2) << 8, vx, -820, 140, false);
                w->ev |= FEV_FOE_SHOT;
                f->state = 12;
            }
        } else if (f->t2 > 60) f->t2 = 60;
        if (f->state) f->state--;
        break;
    case FK_SQUAWKER: {
        /* hops about madly, never far from home */
        static const int16_t HOP[8][3] = {
            {14, 200, -760}, {9, -150, -560}, {22, 90, -900}, {6, -260, -640},
            {17, 180, -480}, {11, -90, -880}, {25, -220, -720}, {8, 240, -600},
        };
        if (f->ground) {
            f->vx = 0;
            const int16_t *h = HOP[f->state & 7];
            if (++f->t2 >= h[0]) {
                f->t2 = 0;
                int vx = h[1];
                int off = (f->x >> 8) - f->hx;
                if ((off > 24 && vx > 0) || (off < -24 && vx < 0)) vx = -vx;
                f->vx = vx;
                f->vy = h[2];
                f->face = (int8_t)(vx < 0 ? -1 : 1);
                f->state++;
            }
        }
        foe_move(w, f, true, &wall);
        if (wall) f->vx = -f->vx;
        break;
    }
    case FK_TUSKER:
        /* sees you on its level and charges, and keeps going past you */
        if (f->state == 0) {
            walker(w, f, 70, true);
            if (sees(w, f, 150, 10, &dx, &dy) && (dx < 0 ? -1 : 1) == f->face) { f->state = 1; f->t2 = 0; }
        } else if (f->state == 1) {
            f->vx = f->face * 640;
            foe_move(w, f, true, &wall);
            if (wall) { f->state = 2; f->t2 = 0; w->shake = imax(w->shake, 6); }
        } else {
            f->vx = 0;
            foe_move(w, f, true, &wall);
            if (++f->t2 >= 50) { f->state = 0; f->face = (int8_t)-f->face; }
        }
        break;
    case FK_IDOL:
        /* faces one way and spits bubbles; get behind it, or over it */
        if (sees(w, f, 170, 60, &dx, &dy) && ++f->t2 >= 100) {
            f->t2 = 0;
            frl_shot_add(w, FS_BUBBLE, (fcx(f) + f->face * 7) << 8, (fcy(f) + 1) << 8, f->face * 220, 0, 230, false);
            w->ev |= FEV_FOE_SHOT;
            f->state = 10;
        }
        if (f->state) f->state--;
        break;
    case FK_DRAKE:
        /* breathes a fan of shot */
        if (sees(w, f, 170, 110, &dx, &dy)) {
            f->face = (int8_t)(dx < 0 ? -1 : 1);
            f->t2++;
            if (f->t2 >= 120 && f->t2 < 156 && (f->t2 - 120) % 4 == 0) {
                int a = angle_to(dx, dy);
                static const int8_t FAN[9] = {0, -10, 10, -20, 20, -6, 6, -16, 16};
                shoot_angle(w, f, FS_PELLET, a + FAN[(f->t2 - 120) / 4], 400, 120);
            }
            if (f->t2 >= 180) f->t2 = 0;
        } else if (f->t2 > 100) f->t2 = 100;
        break;
    case FK_BELL:
        /* hangs still; its hornets do the work (in the spawners) */
        break;
    case FK_KNIGHT:
        /* patrols, then stops and slashes crescents your way */
        if (sees(w, f, 110, 30, &dx, &dy)) {
            f->face = (int8_t)(dx < 0 ? -1 : 1);
            f->vx = 0;
            foe_move(w, f, true, &wall);
            if (++f->t2 >= 90) {
                f->t2 = 0;
                frl_shot_add(w, FS_CRESCENT, (fcx(f) + f->face * 6) << 8, ((f->y >> 8) + 3) << 8, f->face * 560, -300, 50, false);
                w->ev |= FEV_FOE_SHOT;
                f->state = 14;
            }
        } else {
            if (f->t2 > 60) f->t2 = 60;
            walker(w, f, 100, true);
        }
        if (f->state) f->state--;
        break;
    case FK_BLOATER:
        /* sits in the way and lets out rings of shot */
        foe_move(w, f, true, &wall);
        if (sees(w, f, 170, 110, &dx, &dy) && ++f->t2 >= 150) {
            f->t2 = 0;
            int off = (f->state++ & 1) ? 16 : 0;
            for (int k = 0; k < 8; k++) shoot_angle(w, f, FS_PELLET, k * 32 + off, 300, 150);
        }
        break;
    case FK_BROODHEN:
        /* wanders and shoots one at a time, round and round */
        walker(w, f, 64, true);
        if (sees(w, f, 170, 110, &dx, &dy) && ++f->t2 >= 40) {
            f->t2 = 0;
            shoot_angle(w, f, FS_PELLET, (int)(f->state++ * 72 + 20) & 255, 330, 150);
        }
        break;
    case FK_HORNHEAD:
        /* leaps to land on you */
        if (f->ground && f->vy >= 0) {
            f->vx = 0;
            if (f->t2) f->t2--;
            else if (sees(w, f, 150, 80, &dx, &dy)) {
                f->vy = -1280;
                f->vx = iclamp(dx * 256 / 46, -768, 768);
                f->face = (int8_t)(dx < 0 ? -1 : 1);
                f->t2 = 80;
                f->state = 1;
            }
        }
        {
            bool was = f->ground;
            foe_move(w, f, true, &wall);
            if (!was && f->ground && f->state) { f->state = 0; w->shake = imax(w->shake, 8); f->vx = 0; }
        }
        break;
    case FK_STINGBACK:
        /* creeps about and throws oozles at you */
        walker(w, f, 60, true);
        if (sees(w, f, 150, 100, &dx, &dy)) {
            if (++f->t2 >= 150 && count_owned(w, FK_OOZLE, i) < 2) {
                f->t2 = 0;
                int k = frl_foe_add(w, FK_OOZLE, fcx(f) - 4, (f->y >> 8) - 4);
                if (k >= 0) {
                    FrlFoe *o = &w->foe[k];
                    f = &w->foe[i];
                    o->minion = 1;
                    o->owner = (int16_t)i;
                    o->vx = iclamp(dx * 256 / 50, -420, 420);
                    o->vy = -760;
                    o->face = (int8_t)(dx < 0 ? -1 : 1);
                    w->ev |= FEV_FOE_SHOT;
                }
            }
        }
        break;
    case FK_GULPER:
    case FK_HEART:
    default:
        break;
    }
}

void frl_foes_step(FrlWorld *w) {
    for (int i = 0; i < w->nfoe; i++) {
        FrlFoe *f = &w->foe[i];
        if (!f->on || !frl_foe_active(w, f)) continue;
        step_foe(w, i);
        f = &w->foe[i];
        /* anything that falls out of the map is gone */
        if ((f->y >> 8) > FRL_MH * FRL_T) f->on = 0;
    }
}

/* ------------------------------------------------------------------ */
/* drains, combs and bells                                              */

static int add_minion(FrlWorld *w, int kind, int px, int py, int owner) {
    int k = frl_foe_add(w, kind, px, py);
    if (k < 0) return -1;
    w->foe[k].minion = 1;
    w->foe[k].owner = (int16_t)owner;
    w->ev |= FEV_SPAWN;
    return k;
}

void frl_spawners_step(FrlWorld *w) {
    int ux, uy;
    if (!target(w, &ux, &uy)) return;
    /* drains: an oozle when you first come near, then one every few seconds;
     * a stone over a drain stops it for good */
    for (int i = 0; i < w->ndrain; i++) {
        FrlDrain *d = &w->drain[i];
        int cx = d->tx * FRL_T + 5, cy = d->ty * FRL_T + 5;
        bool near = iabs(ux - cx) < 130 && iabs(uy - cy) < 90;
        if (w->tile[d->ty][d->tx] == FT_STONE || !near) { d->near = 0; continue; }
        int tag = -2 - i;
        int kids = count_owned(w, FK_OOZLE, tag);
        if (!d->near) { d->near = 1; d->t = 160; }
        if (++d->t >= 200 && kids < 3) {
            d->t = 0;
            int k = add_minion(w, FK_OOZLE, cx - 4, d->ceiling ? d->ty * FRL_T : (d->ty + 1) * FRL_T - 5, tag);
            if (k >= 0) w->foe[k].face = (int8_t)(ux < cx ? -1 : 1);
        }
    }
    /* combs: midges */
    for (int i = 0; i < w->ncomb; i++) {
        FrlComb *c = &w->comb[i];
        int cx = c->tx * FRL_T + 5, cy = c->ty * FRL_T + 5;
        if (iabs(ux - cx) >= 140 || iabs(uy - cy) >= 100) continue;
        int tag = -20 - i;
        if (++c->t >= 170 && count_owned(w, FK_MIDGE, tag) < 2) {
            c->t = 0;
            add_minion(w, FK_MIDGE, ux < cx ? cx - 12 : cx + 6, cy - 3, tag);
        }
    }
    /* bells: hornets, for as long as the bell hangs */
    for (int i = 0; i < w->nplaced; i++) {
        FrlFoe *f = &w->foe[i];
        if (!f->on || f->kind != FK_BELL) continue;
        if (iabs(ux - fcx(f)) >= 150 || iabs(uy - fcy(f)) >= 110) continue;
        if (++f->t2 >= 150 && count_owned(w, FK_HORNET, i) < 3) {
            f->t2 = 0;
            add_minion(w, FK_HORNET, fcx(f) - 4, (f->y >> 8) + f->h, i);
        }
    }
}

void frl_clear_minions(FrlWorld *w) {
    for (int i = w->nplaced; i < FRL_FOES; i++) w->foe[i].on = 0;
    w->nfoe = w->nplaced;
    for (int i = 0; i < w->nplaced; i++) w->foe[i].kids = 0;
    for (int i = 0; i < w->ndrain; i++) w->drain[i].near = 0;
    for (int i = 0; i < FRL_SHOTS; i++)
        if (!w->shot[i].mine) w->shot[i].on = 0;
}
