/* LIMPET - the Gristle's creatures and the station's own guns turned on
 * the tank: turrets, wall lasers, drones, oglers, crawlers, sacs and their
 * specks, cable worms, spiked balls and flame nozzles; the foes' shots;
 * the survival decks' waves; the Turncoat and the Dreadnought. */
#include "lmp.h"

const char *const LMP_FOE_NAME[FO_KINDS] = {
    "", "TURRET", "WALL LASER", "DRONE", "OGLER", "CRAWLER", "SAC", "SPECK", "CABLE WORM", "SWITCH WORM",
    "SPIKED BALL", "NOZZLE", "THE TURNCOAT", "THE DREADNOUGHT",
};
const int LMP_FOE_HP[FO_KINDS] = {0, 2, 3, 3, 3, 3, 5, 1, 4, 999, 999, 999, 16, 24};

#define TURRET_COOL 90
#define TURRET_AIM 14
#define LASER_BEAT 130
#define LASER_WARN 26
#define FLAME_OFF 90
#define FLAME_WARN 24
#define FLAME_ON 60
#define FLAME_CELLS 4
#define CRAWL_T 14
#define TURNCOAT_T 9
#define DREAD_W 12                /* cells */
#define DREAD_H 5
#define DREAD_BARREL 3

/* ---- helpers ------------------------------------------------------------ */

static void tank_px(int *x, int *y) { lmp_tank_centre(x, y); }

static bool line_clear(int ax, int ay, int bx, int by) {
    /* cells between two points on one axis, as a shot sees them */
    int dx = isign(bx - ax), dy = isign(by - ay);
    int steps = imax(iabs(bx - ax), iabs(by - ay)) / 5;
    for (int k = 1; k < steps; k++) {
        int x = ax + (bx - ax) * k / steps, y = ay + (by - ay) * k / steps;
        (void)dx; (void)dy;
        if (lmp_shot_blocked(x / LMP_T, y / LMP_T)) return false;
    }
    return true;
}

int lmp_add_eshot(int kind, int x, int y, int vx, int vy) {
    for (int i = 0; i < LMP_MAX_ESHOTS; i++) {
        LmpEShot *e = &lg.es[i];
        if (e->alive) continue;
        memset(e, 0, sizeof *e);
        e->alive = 1;
        e->kind = (uint8_t)kind;
        e->x = x;
        e->y = y;
        e->vx = vx;
        e->vy = vy;
        e->life = kind == ES_BEAM ? 420 : kind == ES_RING ? 300 : 600;
        return i;
    }
    return -1;
}

int lmp_spawn_foe(int kind, int cx, int cy, int n) {
    for (int i = 0; i < LMP_MAX_FOES; i++) {
        LmpFoe *f = &lg.foe[i];
        if (f->alive) continue;
        memset(f, 0, sizeof *f);
        f->alive = 1;
        f->kind = (uint8_t)kind;
        f->hp = (int16_t)LMP_FOE_HP[kind];
        f->cx = cx;
        f->cy = cy;
        f->n = n;
        f->x = (cx * LMP_T + 5) * LMP_FX;
        f->y = (cy * LMP_T + 5) * LMP_FX;
        f->w = f->h = 8;
        f->spawn = -1;
        f->dir = 1;
        switch (kind) {
        case FO_TURRET: f->t = 30; break;
        case FO_LASER: f->t = (cx * 7 + cy * 13) % LASER_BEAT; break;
        case FO_FLAME: f->t = (cx * 11 + cy * 5) % (FLAME_OFF + FLAME_WARN + FLAME_ON); break;
        case FO_SAC: f->w = f->h = 10; f->t = 60; break;
        case FO_SPECK: f->w = f->h = 6; break;
        case FO_DRONE: f->w = 10; f->h = 8; f->t = 60 + (cx * 17) % 90; f->vx = LMP_FX / 2; break;
        case FO_EYE: f->t = 50 + (cx * 13) % 70; break;
        case FO_BALL:
            f->w = f->h = 8;
            f->vx = n == DR_D ? 0 : LMP_FX;
            f->vy = n == DR_D ? LMP_FX : 0;
            break;
        case FO_WORM:
        case FO_SWITCHWORM:
            f->w = f->h = 8;
            f->vx = LMP_FX;
            for (int k = 0; k < 24; k++) { f->trail[k][0] = (int16_t)(cx * LMP_T + 5); f->trail[k][1] = (int16_t)(cy * LMP_T + 5); }
            break;
        case FO_CRAWLER: {
            /* cling to the first wall round it */
            static const int ORDER[4] = {DR_U, DR_D, DR_R, DR_L};
            f->n = DR_U;
            for (int k = 0; k < 4; k++)
                if (lmp_state_ok(cx, cy, ORDER[k])) { f->n = ORDER[k]; break; }
            f->w = f->h = 9;
            f->ox = cx; f->oy = cy; f->on = f->n;
            break;
        }
        case FO_TURNCOAT:
            f->w = f->h = 9;
            f->ox = cx; f->oy = cy; f->on = n;
            f->t = 60;
            break;
        case FO_DREAD:
            f->w = DREAD_W * LMP_T;
            f->h = DREAD_H * LMP_T;
            f->t = 0;
            break;
        default: break;
        }
        return i;
    }
    return -1;
}

int lmp_foes_alive(void) {
    int n = 0;
    for (int i = 0; i < LMP_MAX_FOES; i++)
        n += lg.foe[i].alive && lg.foe[i].kind != FO_BALL && lg.foe[i].kind != FO_FLAME && lg.foe[i].kind != FO_SWITCHWORM;
    return n;
}

int lmp_count_foes(int kind) {
    int n = 0;
    for (int i = 0; i < LMP_MAX_FOES; i++) n += lg.foe[i].alive && lg.foe[i].kind == kind;
    return n;
}

static void boss_dead(LmpFoe *f);

void lmp_hurt_foe(int i, int dmg) {
    LmpFoe *f = &lg.foe[i];
    if (!f->alive) return;
    if (f->kind == FO_BALL || f->kind == FO_FLAME) { lmp_sfx("lmp_tink"); return; }
    if (f->kind == FO_SWITCHWORM) {
        if (f->flash == 0) {
            lg.parity ^= 1;
            f->flash = 30;
            lmp_sfx("lmp_swap");
            if (lg.state == GS_PLAY && lmp_solid(lg.tk.x, lg.tk.y)) lmp_kill_tank(100);
        }
        return;
    }
    if (f->kind == FO_DREAD) return; /* only through the barrel: see dread_shots */
    if (f->kind == FO_TURNCOAT) {
        /* hit, it can't be hurt for a moment and scuttles away out of the line */
        if (f->flash > 0) { lmp_sfx("lmp_tink"); return; }
        f->hp = (int16_t)(f->hp - dmg);
        f->flash = 45;
        f->st = 0;
        f->a = 0;
        f->c = 6;
        if (f->hp <= 0) boss_dead(f);
        else lmp_sfx("lmp_hit");
        return;
    }
    f->hp = (int16_t)(f->hp - dmg);
    f->flash = 8;
    if (f->hp > 0) { lmp_sfx("lmp_hit"); return; }
    if (f->kind == FO_TURNCOAT) { boss_dead(f); return; }
    f->alive = 0;
    lmp_burst(f->x / LMP_FX, f->y / LMP_FX, f->kind == FO_SAC || f->kind == FO_SPECK || f->kind == FO_CRAWLER ? C_PINK : C_ORANGE, 10, 2);
    lmp_sfx("lmp_pop");
}

/* ---- grid walkers (crawlers, the Turncoat) ------------------------------- */

static void grid_px(const LmpFoe *f, int *px, int *py, int dur) {
    int ax = f->ox * LMP_T + 5, ay = f->oy * LMP_T + 5, bx = f->cx * LMP_T + 5, by = f->cy * LMP_T + 5;
    if (!lmp_axis(f->on)) { ax += LMP_DX[f->on]; ay += LMP_DY[f->on]; }
    if (!lmp_axis(f->n)) { bx += LMP_DX[f->n]; by += LMP_DY[f->n]; }
    int m = imin(f->mt, dur);
    *px = ax + (bx - ax) * m / dur;
    *py = ay + (by - ay) * m / dur;
}

static bool grid_walk(LmpFoe *f, int dur) {
    /* returns true when a step has just finished */
    if (f->mt < dur) {
        f->mt++;
        int px, py;
        grid_px(f, &px, &py, dur);
        f->x = px * LMP_FX;
        f->y = py * LMP_FX;
        return f->mt == dur;
    }
    return false;
}

static void grid_go(LmpFoe *f) {
    int nx, ny, nn;
    for (int tries = 0; tries < 2; tries++) {
        if (lmp_surface_step(f->cx, f->cy, f->n, f->dir, &nx, &ny, &nn) && lmp_tile(nx, ny) != '^' &&
            !(lmp_axis(nn) && lmp_is_spikes(nx - LMP_DX[nn], ny - LMP_DY[nn]))) {
            f->ox = f->cx; f->oy = f->cy; f->on = f->n;
            f->cx = nx; f->cy = ny; f->n = nn;
            f->mt = 0;
            return;
        }
        f->dir = -f->dir;
    }
}

/* ---- each kind ------------------------------------------------------------ */

static void turret_update(LmpFoe *f, int tx, int ty) {
    int fx = f->x / LMP_FX, fy = f->y / LMP_FX;
    if (f->state == 0) {
        if (f->t > 0) { f->t--; return; }
        bool on_line = lmp_axis(f->n) &&
            (LMP_DX[f->n] != 0 ? (iabs(ty - fy) <= 3 && isign(tx - fx) == LMP_DX[f->n])
                                : (iabs(tx - fx) <= 3 && isign(ty - fy) == LMP_DY[f->n]));
        if (on_line && line_clear(fx, fy, tx, ty)) { f->state = 1; f->st = TURRET_AIM; lmp_sfx("lmp_aim"); }
    } else if (--f->st <= 0) {
        lmp_add_eshot(ES_BULLET, f->x + LMP_DX[f->n] * 5 * LMP_FX, f->y + LMP_DY[f->n] * 5 * LMP_FX,
                      LMP_DX[f->n] * 640, LMP_DY[f->n] * 640);
        lmp_sfx("lmp_turret");
        f->state = 0;
        f->t = TURRET_COOL;
    }
}

static void laser_update(LmpFoe *f) {
    f->t = (f->t + 1) % LASER_BEAT;
    f->state = f->t >= LASER_BEAT - LASER_WARN ? 1 : 0;
    if (f->t == 0) {
        lmp_add_eshot(ES_LASER, f->x + LMP_DX[f->n] * 5 * LMP_FX, f->y + LMP_DY[f->n] * 5 * LMP_FX,
                      LMP_DX[f->n] * 6 * LMP_FX, LMP_DY[f->n] * 6 * LMP_FX);
        lmp_sfx("lmp_laser");
    }
}

int lmp_flame_reach(const LmpFoe *f) {
    /* the vent's own cell burns too (k = 0), then up to four cells out */
    int cyc = f->t % (FLAME_OFF + FLAME_WARN + FLAME_ON);
    if (cyc < FLAME_OFF + FLAME_WARN) return 0;
    int n = 0;
    for (int k = 1; k <= FLAME_CELLS; k++) {
        int x = f->cx + k * LMP_DX[f->n], y = f->cy + k * LMP_DY[f->n];
        if (lmp_shot_blocked(x, y)) break;
        n = k;
    }
    return n;
}

static void flame_update(LmpFoe *f, int tx, int ty) {
    f->t++;
    int cyc = f->t % (FLAME_OFF + FLAME_WARN + FLAME_ON);
    f->state = cyc < FLAME_OFF ? 0 : cyc < FLAME_OFF + FLAME_WARN ? 1 : 2;
    if (cyc == FLAME_OFF + FLAME_WARN) lmp_sfx("lmp_flame");
    int r = lmp_flame_reach(f);
    if (r == 0 && f->state != 2) return;
    for (int k = 0; k <= r; k++) {
        int cx = (f->cx + k * LMP_DX[f->n]) * LMP_T + 5, cy = (f->cy + k * LMP_DY[f->n]) * LMP_T + 5;
        if (iabs(tx - cx) <= 6 && iabs(ty - cy) <= 6) { lmp_kill_tank(FO_FLAME); return; }
    }
}

static void drone_update(LmpFoe *f, int tx, int ty) {
    int nx = f->x + f->vx;
    int ahead = (nx / LMP_FX + isign(f->vx) * 6) / LMP_T;
    if (lmp_solid(ahead, f->y / LMP_FX / LMP_T) || lmp_tri_normal(lmp_tile(ahead, f->y / LMP_FX / LMP_T)) >= 0) f->vx = -f->vx;
    else f->x = nx;
    f->a++;
    f->y += (f->a / 16) % 2 ? 24 : -24;
    if (--f->t <= 0) {
        f->t = 150;
        int sx = isign(tx - f->x / LMP_FX);
        if (sx == 0) sx = 1;
        int sy = isign(ty - f->y / LMP_FX);
        if (sy == 0) sy = 1;
        int e = lmp_add_eshot(ES_BEAM, f->x, f->y + 4 * LMP_FX, sx * 300, sy * 300);
        if (e >= 0) lg.es[e].bounces = 6;
        lmp_sfx("lmp_beam");
    }
}

static void eye_update(LmpFoe *f, int tx, int ty) {
    f->a++;
    f->y += (f->a / 20) % 2 ? 16 : -16;
    int dx = tx - f->x / LMP_FX, dy = ty - f->y / LMP_FX;
    f->dir = dx < 0 ? -1 : 1;
    if (--f->t <= 0) {
        f->t = 120;
        int d = imax(1, iabs(dx) + iabs(dy));
        if (d < 200) {
            lmp_add_eshot(ES_RING, f->x, f->y, dx * 330 / d, dy * 330 / d);
            lmp_sfx("lmp_ring");
        }
    }
}

static void sac_update(LmpFoe *f) {
    f->a++;
    if (--f->t > 0) return;
    f->t = 170;
    int near = 0;
    for (int i = 0; i < LMP_MAX_FOES; i++)
        if (lg.foe[i].alive && lg.foe[i].kind == FO_SPECK && lg.foe[i].b == (int)(f - lg.foe) + 1) near++;
    if (near >= 3) return;
    int s = lmp_spawn_foe(FO_SPECK, f->cx + LMP_DX[f->n], f->cy + LMP_DY[f->n], DR_U);
    if (s >= 0) { lg.foe[s].b = (int)(f - lg.foe) + 1; lg.foe[s].spawn = -3; lmp_sfx("lmp_spawn"); }
}

static void speck_update(LmpFoe *f, int tx, int ty) {
    int dx = tx * LMP_FX - f->x, dy = ty * LMP_FX - f->y;
    f->vx += isign(dx) * 12;
    f->vy += isign(dy) * 12;
    f->vx = iclamp(f->vx, -180, 180);
    f->vy = iclamp(f->vy, -180, 180);
    int nx = f->x + f->vx, ny = f->y + f->vy;
    if (lmp_shot_blocked(nx / LMP_FX / LMP_T, f->y / LMP_FX / LMP_T)) f->vx = -f->vx / 2; else f->x = nx;
    if (lmp_shot_blocked(f->x / LMP_FX / LMP_T, ny / LMP_FX / LMP_T)) f->vy = -f->vy / 2; else f->y = ny;
    f->a++;
}

static void worm_update(LmpFoe *f) {
    int hx = f->x / LMP_FX, hy = f->y / LMP_FX;
    int horiz = f->n != DR_D;
    int sp = horiz ? f->vx : f->vy;
    if (sp == 0) sp = LMP_FX;
    int front = horiz ? (hx + isign(sp) * 6) : (hy + isign(sp) * 6);
    bool blocked = horiz ? lmp_shot_blocked(front / LMP_T, hy / LMP_T) : lmp_shot_blocked(hx / LMP_T, front / LMP_T);
    if (blocked) sp = -sp;
    if (horiz) { f->vx = sp; f->x += sp; } else { f->vy = sp; f->y += sp; }
    if (f->flash > 0) f->flash--;
    if (++f->a % 3 == 0) {
        for (int k = 23; k > 0; k--) { f->trail[k][0] = f->trail[k - 1][0]; f->trail[k][1] = f->trail[k - 1][1]; }
        f->trail[0][0] = (int16_t)(f->x / LMP_FX);
        f->trail[0][1] = (int16_t)(f->y / LMP_FX);
    }
}

static void ball_update(LmpFoe *f) {
    if (f->b > 0) {
        /* a ball the Dreadnought dropped: it lands, rolls away and breaks up */
        if (--f->b == 0) { f->alive = 0; lmp_burst(f->x / LMP_FX, f->y / LMP_FX, C_GREY, 6, 2); return; }
        if (f->vy > 0 && lmp_shot_blocked(f->x / LMP_FX / LMP_T, (f->y / LMP_FX + 6) / LMP_T)) {
            f->vy = 0;
            f->vx = (f->x / LMP_FX < lg.mw * LMP_T / 2) ? -LMP_FX : LMP_FX;
        }
    }
    int nx = f->x + f->vx, ny = f->y + f->vy;
    int fx = (nx / LMP_FX + isign(f->vx) * 5) / LMP_T, fy = (ny / LMP_FX + isign(f->vy) * 5) / LMP_T;
    if (f->vx && (lmp_shot_blocked(fx, f->y / LMP_FX / LMP_T))) f->vx = -f->vx;
    else if (f->vy && lmp_shot_blocked(f->x / LMP_FX / LMP_T, fy)) f->vy = -f->vy;
    else { f->x = nx; f->y = ny; }
    f->a++;
}

static void crawler_update(LmpFoe *f) {
    if (f->mt >= CRAWL_T || (f->cx == f->ox && f->cy == f->oy && f->mt == 0)) {
        if (f->t > 0) { f->t--; return; }
        grid_go(f);
    }
    if (grid_walk(f, CRAWL_T)) f->t = 10; /* a rest after every step */
    f->a++;
}

/* ---- the Turncoat: a tank like Limpet, turned ------------------------------ */

static void turncoat_update(LmpFoe *f, int tx, int ty) {
    LmpTank *tk = &lg.tk;
    int fast = f->hp <= LMP_FOE_HP[FO_TURNCOAT] / 2;
    int dur = f->c > 0 ? 4 : fast ? TURNCOAT_T - 3 : TURNCOAT_T; /* fleeing after a hit: quick */
    if (f->flash > 0) f->flash--;
    if (f->mt < dur && !(f->cx == f->ox && f->cy == f->oy && f->on == f->n)) {
        grid_walk(f, dur);
        return;
    }
    f->ox = f->cx; f->oy = f->cy; f->on = f->n;
    f->x = (f->cx * LMP_T + 5) * LMP_FX;
    f->y = (f->cy * LMP_T + 5) * LMP_FX;
    if (f->b > 0) f->b--;
    if (f->a > 0) f->a--;
    f->state = f->t <= 30 ? 1 : 0; /* it glows before it warps */
    if (f->st > 0) {
        /* a burst of three, like Limpet's */
        f->st--;
        if (f->st % 8 == 0) {
            lmp_add_eshot(ES_BULLET, f->x + LMP_DX[f->n] * 5 * LMP_FX, f->y + LMP_DY[f->n] * 5 * LMP_FX,
                          LMP_DX[f->n] * 3 * LMP_FX, LMP_DY[f->n] * 3 * LMP_FX);
            lmp_sfx("lmp_turret");
        }
        return;
    }
    int fx = f->x / LMP_FX, fy = f->y / LMP_FX;
    if (f->c > 0) {
        /* hit: it scuttles off along its surface, away from the tank's line */
        f->c--;
        int away = LMP_DX[f->n] != 0 ? isign(fy - ty) : isign(fx - tx);
        if (away == 0) away = f->dir;
        int t1 = (f->n + 2) & 7;
        int along = LMP_DX[f->n] != 0 ? LMP_DY[t1] : LMP_DX[t1];
        f->dir = along == away ? 1 : -1;
        grid_go(f);
        return;
    }
    bool on_line = LMP_DX[f->n] != 0 ? (iabs(ty - fy) <= 4 && isign(tx - fx) == LMP_DX[f->n])
                                      : (iabs(tx - fx) <= 4 && isign(ty - fy) == LMP_DY[f->n]);
    if (on_line && f->b == 0) { f->st = 17; f->b = fast ? 40 : 60; return; }
    if (f->a > 0) return; /* a rest between steps */
    if (--f->t <= 0) {
        /* warp to the opposite surface, flares and all */
        f->t = fast ? 110 : 170;
        int ox, oy, on;
        if (lmp_warp_target(f->cx, f->cy, f->n, &ox, &oy, &on) >= 0) {
            lmp_burst(fx, fy, C_RED, 8, 2);
            f->cx = f->ox = ox; f->cy = f->oy = oy; f->n = f->on = on;
            f->x = (ox * LMP_T + 5) * LMP_FX;
            f->y = (oy * LMP_T + 5) * LMP_FX;
            f->a = 20;
            lmp_sfx("lmp_warp");
            int tdir = (on + 2) & 7;
            for (int k = -2; k <= 2; k++) {
                if (!k) continue;
                int cx = (ox + k * LMP_DX[tdir]) * LMP_T + 5, cy = (oy + k * LMP_DY[tdir]) * LMP_T + 5;
                if (iabs(tx - cx) <= 6 && iabs(ty - cy) <= 6) lmp_kill_tank(FO_TURNCOAT);
            }
            return;
        }
    }
    /* roll toward the tank's line */
    int want;
    if (LMP_DX[f->n] != 0) want = isign(ty - fy); else want = isign(tx - fx);
    if (want == 0) want = f->dir;
    /* which sense moves that way along this surface */
    int t1 = (f->n + 2) & 7;
    int along = LMP_DX[f->n] != 0 ? LMP_DY[t1] : LMP_DX[t1];
    f->dir = along == want ? 1 : -1;
    (void)tk;
    grid_go(f);
    f->a = fast ? 6 : 12;
    if (f->cx == f->ox && f->cy == f->oy) f->t -= 4;
}

/* ---- the Dreadnought ------------------------------------------------------ */
/* It clings to the arena's ceiling with its barrel hanging down. It can only
 * be hurt by a shot that goes straight up the barrel into its core, while the
 * core is open after a shell. */

enum { DN_CLOSED, DN_CHARGE, DN_OPEN, DN_DYING };

static int dread_col(const LmpFoe *f) { return f->cx; }
static int dread_mouth_y(const LmpFoe *f) { return (f->cy + DREAD_H + DREAD_BARREL) * LMP_T; }

static void dread_update(LmpFoe *f, int tx, int ty) {
    int phase = f->hp > 16 ? 0 : f->hp > 8 ? 1 : 2;
    if (f->flash > 0) f->flash--;
    f->x = (f->cx * LMP_T + 5) * LMP_FX;
    f->y = ((f->cy * LMP_T) + DREAD_H * LMP_T / 2) * LMP_FX;
    if (f->state == DN_DYING) {
        f->st++;
        if (f->st % 8 == 0) {
            lmp_burst(f->cx * LMP_T + rng_range(&lg.rng, -55, 55), f->cy * LMP_T + rng_range(&lg.rng, 0, 50), C_ORANGE, 10, 3);
            lmp_sfx("lmp_boom");
            lg.shake = 6;
        }
        if (f->st >= 150) boss_dead(f);
        return;
    }
    /* crawl along the ceiling; it braces, still, while it charges and while its core is open */
    int speed = phase == 2 ? 14 : phase == 1 ? 18 : 22;
    if (f->state == DN_CLOSED && ++f->a >= speed) {
        f->a = 0;
        int nx = f->cx + f->dir;
        if (nx - DREAD_W / 2 < 1 || nx + DREAD_W / 2 > lg.mw - 2) f->dir = -f->dir;
        else f->cx = nx;
    }
    f->t++;
    switch (f->state) {
    case DN_CLOSED:
        if (f->t >= (phase == 2 ? 90 : phase == 1 ? 100 : 110)) { f->state = DN_CHARGE; f->t = 0; lmp_sfx("lmp_charge"); }
        break;
    case DN_CHARGE:
        if (f->t >= 36) {
            int mx = dread_col(f) * LMP_T + 5;
            int sh = lmp_add_eshot(ES_SHELL, mx * LMP_FX, dread_mouth_y(f) * LMP_FX, 0, 4 * LMP_FX);
            if (sh >= 0) lg.es[sh].bounces = phase >= 1;
            lmp_sfx("lmp_shell");
            lg.shake = 4;
            f->state = DN_OPEN;
            f->t = 0;
            f->b = 0;
        }
        break;
    case DN_OPEN:
        if (f->t >= (phase == 2 ? 160 : phase == 1 ? 180 : 150)) {
            f->state = DN_CLOSED;
            f->t = 0;
        }
        break;
    }
    /* touching its body or barrel */
    int bx0 = (f->cx - DREAD_W / 2) * LMP_T, bx1 = (f->cx + DREAD_W / 2 + 1) * LMP_T;
    int by0 = f->cy * LMP_T, by1 = (f->cy + DREAD_H) * LMP_T;
    if (tx > bx0 - 3 && tx < bx1 + 3 && ty > by0 - 3 && ty < by1 + 3) lmp_kill_tank(FO_DREAD);
    int mx = dread_col(f) * LMP_T + 5;
    if (iabs(tx - mx) < 8 && ty > by1 - 3 && ty < dread_mouth_y(f) + 3) lmp_kill_tank(FO_DREAD);
}

/* a shot reaching the barrel's mouth */
static void dread_shots(LmpFoe *f) {
    if (f->state == DN_DYING) return;
    int mx = dread_col(f) * LMP_T + 5, my = dread_mouth_y(f);
    int bx0 = (f->cx - DREAD_W / 2) * LMP_T, bx1 = (f->cx + DREAD_W / 2 + 1) * LMP_T;
    for (int i = 0; i < LMP_MAX_SHOTS; i++) {
        LmpShot *s = &lg.shot[i];
        if (!s->alive) continue;
        int px = s->x / LMP_FX, py = s->y / LMP_FX;
        if (s->vy < 0 && iabs(px - mx) <= 3 && py <= my && py > my - 8) {
            s->alive = 0;
            if (f->state == DN_OPEN) {
                f->hp--;
                f->flash = 8;
                /* three hits and the core shuts again */
                if (++f->b >= 3 && f->hp > 0) { f->state = DN_CLOSED; f->t = 0; }
                lmp_sfx("lmp_corehit");
                lmp_burst(mx, my - 20, C_YELLOW, 6, 2);
                if (f->hp <= 0) { f->state = DN_DYING; f->st = 0; lmp_sfx("lmp_bossdie"); }
            } else {
                lmp_sfx("lmp_tink");
            }
        } else if (px > bx0 && px < bx1 && py < (f->cy + DREAD_H) * LMP_T && py > f->cy * LMP_T) {
            s->alive = 0;
            lmp_sfx("lmp_tink");
        } else if (iabs(px - mx) < 6 && py < my && py > (f->cy + DREAD_H) * LMP_T) {
            s->alive = 0;
            lmp_sfx("lmp_tink");
        }
    }
}

void lmp_boss_start(void) {
    for (int i = 0; i < LMP_MAX_FOES; i++)
        if (lg.foe[i].alive && (lg.foe[i].kind == FO_TURNCOAT || lg.foe[i].kind == FO_DREAD)) return;
    if (lg.kind == LK_TURNCOAT) {
        /* it waits on the far ceiling */
        int x = lg.mw - 4, y = 1;
        while (y < lg.mh - 1 && !lmp_state_ok(x, y, DR_D)) y++;
        int f = lmp_spawn_foe(FO_TURNCOAT, x, y, DR_D);
        if (f >= 0) lg.foe[f].spawn = -4;
    } else if (lg.kind == LK_DREAD) {
        int f = lmp_spawn_foe(FO_DREAD, lg.mw / 2 + 4, 1, DR_D);
        if (f >= 0) lg.foe[f].spawn = -4;
    }
}

static void boss_dead(LmpFoe *f) {
    int kind = f->kind;
    lmp_burst(f->x / LMP_FX, f->y / LMP_FX, C_ORANGE, 30, 3);
    lmp_sfx("lmp_bossdie");
    f->alive = 0;
    lg.boss_dead = true;
    lg.shake = 12;
    /* everything else goes with it */
    for (int i = 0; i < LMP_MAX_FOES; i++)
        if (lg.foe[i].alive) { lmp_burst(lg.foe[i].x / LMP_FX, lg.foe[i].y / LMP_FX, C_PINK, 6, 2); lg.foe[i].alive = 0; }
    memset(lg.es, 0, sizeof lg.es);
    if (kind == FO_DREAD) {
        /* it drops the last honey on the floor below where it hung */
        int x = iclamp(f->cx, 2, lg.mw - 3), y = lg.mh - 2;
        while (y > 1 && !lmp_state_ok(x, y, DR_U)) y--;
        lg.honey_x = x;
        lg.honey_y = y;
        lg.honey_shown = true;
    }
    lmp_open_exit();
}

/* ---- survival decks -------------------------------------------------------- */

void lmp_survival_update(void) {
    if (lg.kind != LK_SURVIVAL || lg.bulb_out) return;
    const LmpLevel *L = LMP_LEVEL[lg.level];
    int left = 0;
    for (int i = 0; i < LMP_MAX_FOES; i++) left += lg.foe[i].alive && (lg.foe[i].spawn == -2 || lg.foe[i].spawn == -3);
    if (lg.wave_t > 0) {
        if (--lg.wave_t > 0) return;
        if (lg.wave >= L->nwaves) { lmp_open_exit(); return; }
        /* the wave comes out of the spawn points in turn */
        int pts[32][2], np = 0;
        for (int y = 0; y < lg.mh && np < 32; y++)
            for (int x = 0; x < lg.mw && np < 32; x++)
                if (lg.map[y][x] == '?') { pts[np][0] = x; pts[np][1] = y; np++; }
        const char *w = L->waves[lg.wave];
        for (int k = 0; w[k] && np > 0; k++) {
            int kind = FO_NONE;
            switch (w[k]) {
            case 'c': kind = FO_CRAWLER; break;
            case 'f': kind = FO_SPECK; break;
            case 'e': kind = FO_EYE; break;
            case 'p': kind = FO_DRONE; break;
            case 's': kind = FO_SAC; break;
            case 'w': kind = FO_WORM; break;
            default: break;
            }
            if (kind == FO_NONE) continue;
            int px = pts[(k + lg.wave * 3) % np][0], py = pts[(k + lg.wave * 3) % np][1];
            int n = DR_U;
            static const int ORDER[4] = {DR_U, DR_D, DR_R, DR_L};
            for (int o = 0; o < 4; o++)
                if (lmp_solid(px - LMP_DX[ORDER[o]], py - LMP_DY[ORDER[o]])) { n = ORDER[o]; break; }
            int f = lmp_spawn_foe(kind, px, py, n);
            if (f >= 0) { lg.foe[f].spawn = -2; lg.foe[f].t = 40 + k * 10; }
            lmp_burst(px * LMP_T + 5, py * LMP_T + 5, C_PINK, 6, 2);
        }
        lmp_sfx("lmp_wave");
        lg.wave++;
        return;
    }
    if (left == 0) lg.wave_t = lg.wave >= L->nwaves ? 30 : 100;
}

/* ---- every frame ----------------------------------------------------------- */

void lmp_foes_update(void) {
    int tx, ty;
    tank_px(&tx, &ty);
    for (int i = 0; i < LMP_MAX_FOES; i++) {
        LmpFoe *f = &lg.foe[i];
        if (!f->alive) continue;
        if (f->flash > 0 && f->kind != FO_SWITCHWORM && f->kind != FO_TURNCOAT && f->kind != FO_DREAD) f->flash--;
        switch (f->kind) {
        case FO_TURRET: turret_update(f, tx, ty); break;
        case FO_LASER: laser_update(f); break;
        case FO_FLAME: flame_update(f, tx, ty); break;
        case FO_DRONE: drone_update(f, tx, ty); break;
        case FO_EYE: eye_update(f, tx, ty); break;
        case FO_SAC: sac_update(f); break;
        case FO_SPECK: speck_update(f, tx, ty); break;
        case FO_WORM: case FO_SWITCHWORM: worm_update(f); break;
        case FO_BALL: ball_update(f); break;
        case FO_CRAWLER: crawler_update(f); break;
        case FO_TURNCOAT: turncoat_update(f, tx, ty); break;
        case FO_DREAD: dread_shots(f); dread_update(f, tx, ty); break;
        default: break;
        }
        if (!f->alive || lg.state != GS_PLAY) continue;
        if (f->kind == FO_FLAME || f->kind == FO_DREAD) continue;
        /* a touch is the end of the tank */
        if (iabs(tx - f->x / LMP_FX) < f->w / 2 + 3 && iabs(ty - f->y / LMP_FX) < f->h / 2 + 3) lmp_kill_tank(f->kind);
        if (f->kind == FO_WORM || f->kind == FO_SWITCHWORM)
            for (int k = 3; k < 24; k += 4)
                if (iabs(tx - f->trail[k][0]) < 7 && iabs(ty - f->trail[k][1]) < 7) { lmp_kill_tank(f->kind); break; }
    }
}

void lmp_eshots_update(void) {
    int tx, ty;
    tank_px(&tx, &ty);
    for (int i = 0; i < LMP_MAX_ESHOTS; i++) {
        LmpEShot *e = &lg.es[i];
        if (!e->alive) continue;
        e->t++;
        if (--e->life <= 0) { e->alive = 0; continue; }
        int nx = e->x + e->vx, ny = e->y + e->vy;
        int cx = nx / LMP_FX / LMP_T, cy = ny / LMP_FX / LMP_T;
        if (nx < 0 || ny < 0 || cx >= lg.mw || cy >= lg.mh) { e->alive = 0; continue; }
        if (lmp_shot_blocked(cx, cy)) {
            if (e->kind == ES_BEAM && e->bounces > 0) {
                e->bounces--;
                if (lmp_shot_blocked(nx / LMP_FX / LMP_T, e->y / LMP_FX / LMP_T)) e->vx = -e->vx;
                if (lmp_shot_blocked(e->x / LMP_FX / LMP_T, ny / LMP_FX / LMP_T)) e->vy = -e->vy;
                lmp_sfx("lmp_tink");
                continue;
            }
            if (e->kind == ES_SHELL && e->bounces) {
                /* once it is hurt, its shells burst into sparks along the floor */
                lmp_burst(nx / LMP_FX, ny / LMP_FX, C_ORANGE, 10, 2);
                lmp_sfx("lmp_boom");
                /* two sparks run off along the floor */
                int b1 = lmp_add_eshot(ES_BOLT, e->x - 3 * LMP_FX, e->y, -2 * LMP_FX, 0);
                int b2 = lmp_add_eshot(ES_BOLT, e->x + 3 * LMP_FX, e->y, 2 * LMP_FX, 0);
                if (b1 >= 0) lg.es[b1].life = 24; /* they fizzle out five cells on */
                if (b2 >= 0) lg.es[b2].life = 24;
            }
            e->alive = 0;
            continue;
        }
        e->x = nx;
        e->y = ny;
        int r = e->kind == ES_RING ? 5 : e->kind == ES_SHELL ? 5 : 3;
        if (lg.state == GS_PLAY && iabs(tx - e->x / LMP_FX) <= r + 1 && iabs(ty - e->y / LMP_FX) <= r + 1) {
            lmp_kill_tank(200 + e->kind);
            if (!lg.god && !lg.tally_hits) e->alive = 0;
            else if (lg.tally_hits) e->alive = 0;
        }
    }
}
