/* LIMPET - the tank: the surface it clings to, rolling (and round ramps),
 * the warp to the opposite surface, the landing flares and the shots.
 *
 * The tank is a cell and a normal, the way its cannon points (away from its
 * surface). Each surface piece is a segment: the face of a solid cell, or
 * the slope of a ramp tile. Rolling goes from a segment to the one that
 * shares its end, turning at most 45 degrees, so a flat floor carries on,
 * a ramp turns it a quarter in two halves, and a sheer wall or a ledge
 * stops it. Everything is checked on doubled coordinates (cell corners are
 * even, centres odd). */
#include "lmp.h"

bool lmp_state_ok(int x, int y, int n) {
    if (lmp_axis(n)) return lmp_open(x, y) && lmp_face_solid(x - LMP_DX[n], y - LMP_DY[n], n);
    return lmp_tri_normal(lmp_tile(x, y)) == n;
}

/* the end of a state's segment when rolling the given way (doubled coordinates) */
static void seg_end(int x, int y, int n, int sense, int *vx, int *vy) {
    int cx = 2 * x + 1, cy = 2 * y + 1;
    if (lmp_axis(n)) { cx -= LMP_DX[n]; cy -= LMP_DY[n]; }
    int t = (n + 2 * sense) & 7;
    *vx = cx + LMP_DX[t];
    *vy = cy + LMP_DY[t];
}

int lmp_surface_step(int x, int y, int n, int sense, int *ox, int *oy, int *on) {
    int vx, vy;
    seg_end(x, y, n, sense, &vx, &vy);
    static const int TURN[3] = {0, 1, -1};
    for (int k = 0; k < 3; k++) {
        int n2 = (n + TURN[k]) & 7;
        for (int c = 0; c < 4; c++) {
            int cx = vx / 2 - 1 + (c & 1), cy = vy / 2 - 1 + (c >> 1);
            if (cx == x && cy == y && n2 == n) continue;
            if (!lmp_state_ok(cx, cy, n2)) continue;
            int sx, sy;
            seg_end(cx, cy, n2, -sense, &sx, &sy);
            if (sx != vx || sy != vy) continue;
            *ox = cx;
            *oy = cy;
            *on = n2;
            return 1;
        }
    }
    return 0;
}

int lmp_roll_path(int x, int y, int n, int sense, int *px, int *py, int *pn, int max) {
    int k = 0;
    int cx = x, cy = y, cn = n;
    do {
        int nx, ny, nn;
        if (k >= max || !lmp_surface_step(cx, cy, cn, sense, &nx, &ny, &nn)) return 0;
        px[k] = nx;
        py[k] = ny;
        pn[k] = nn;
        k++;
        cx = nx; cy = ny; cn = nn;
    } while (!lmp_axis(cn));
    return k;
}

/* -1: nowhere to land; -2: a magnet floor holds it; else the cells travelled */
int lmp_warp_target(int x, int y, int n, int *ox, int *oy, int *on) {
    if (!lmp_axis(n)) return -1;
    if (lmp_tile(x - LMP_DX[n], y - LMP_DY[n]) == 'M') return -2;
    for (int k = 1; k < LMP_MW + LMP_MH; k++) {
        int cx = x + k * LMP_DX[n], cy = y + k * LMP_DY[n];
        if (cx < 0 || cy < 0 || cx >= lg.mw || cy >= lg.mh) return -1;
        if (lmp_warp_pass(cx, cy, n)) continue;
        int lx = cx - LMP_DX[n], ly = cy - LMP_DY[n], ln = (n + 4) & 7;
        if (!lmp_state_ok(lx, ly, ln)) return -1;
        *ox = lx;
        *oy = ly;
        *on = ln;
        return k - 1;
    }
    return -1;
}

/* ---- where it is --------------------------------------------------------- */

static void state_px(int x, int y, int n, int *px, int *py) {
    *px = x * LMP_T + 5;
    *py = y * LMP_T + 5;
    if (!lmp_axis(n)) { *px += LMP_DX[n]; *py += LMP_DY[n]; }
}

static int step_dur(const LmpTank *t, int i) {
    int prev_n = i == 0 ? t->sn : t->pnn[i - 1];
    return (!lmp_axis(prev_n) || !lmp_axis(t->pnn[i])) ? LMP_RAMP_T : LMP_STEP_T;
}

void lmp_tank_centre(int *x, int *y) {
    const LmpTank *t = &lg.tk;
    if (t->pn == 0) { state_px(t->x, t->y, t->n, x, y); return; }
    int at = t->pt;
    int ax, ay, bx, by;
    state_px(t->sx, t->sy, t->sn, &ax, &ay);
    for (int i = 0; i < t->pn; i++) {
        int d = step_dur(t, i);
        state_px(t->px[i], t->py[i], t->pnn[i], &bx, &by);
        if (at < d || i == t->pn - 1) {
            int f = imin(at, d);
            *x = ax + (bx - ax) * f / d;
            *y = ay + (by - ay) * f / d;
            return;
        }
        at -= d;
        ax = bx;
        ay = by;
    }
    *x = ax;
    *y = ay;
}

/* the normal it shows now (on a ramp, the slope's) */
static int shown_normal(void) {
    const LmpTank *t = &lg.tk;
    if (t->pn == 0) return t->n;
    int at = t->pt;
    for (int i = 0; i < t->pn; i++) {
        int d = step_dur(t, i);
        if (at < d / 2) return i == 0 ? t->sn : t->pnn[i - 1];
        if (at < d) return t->pnn[i];
        at -= d;
    }
    return t->n;
}

bool lmp_tank_moving(void) { return lg.tk.pn > 0; }

void lmp_tank_place(int x, int y, int n) {
    LmpTank *t = &lg.tk;
    int col = t->colour;
    memset(t, 0, sizeof *t);
    t->colour = col;
    t->x = x;
    t->y = y;
    t->n = n;
    t->portal_lock = true;
}

/* ---- effects ------------------------------------------------------------- */

void lmp_burst(int x, int y, int col, int n, int sp) {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < LMP_MAX_PARTS; i++) {
            LmpPart *p = &lg.part[i];
            if (p->life > 0) continue;
            p->x = x * LMP_FX;
            p->y = y * LMP_FX;
            p->vx = rng_range(&lg.rng, -sp * 200, sp * 200);
            p->vy = rng_range(&lg.rng, -sp * 200, sp * 200);
            p->life = rng_range(&lg.rng, 10, 26);
            p->col = col;
            break;
        }
    }
}

void lmp_parts_update(void) {
    for (int i = 0; i < LMP_MAX_PARTS; i++) {
        LmpPart *p = &lg.part[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vx = p->vx * 15 / 16;
        p->vy = p->vy * 15 / 16;
    }
}

void lmp_kill_tank(int cause) {
    if (lg.state != GS_PLAY) return;
    if (lg.tally_hits) { lg.hits++; lg.last_hurt = cause; return; }
    if (lg.god) return;
    lg.last_hurt = cause;
    lmp_tank_died(cause);
}

/* ---- arriving somewhere ------------------------------------------------- */

static void portal_partner(char c, int *ox, int *oy) {
    char want = c == '{' ? '}' : c == '}' ? '{' : c == '<' ? '>' : '<';
    *ox = *oy = -1;
    for (int y = 0; y < lg.mh; y++)
        for (int x = 0; x < lg.mw; x++)
            if (lg.map[y][x] == want) { *ox = x; *oy = y; return; }
}

static void arrive(void) {
    LmpTank *t = &lg.tk;
    char here = lmp_tile(t->x, t->y);
    if (here == '{' || here == '}' || here == '<' || here == '>') {
        if (!t->portal_lock) {
            int px, py;
            portal_partner(here, &px, &py);
            if (px >= 0) {
                int nn = t->n;
                if (!lmp_state_ok(px, py, nn))
                    for (int k = 0; k < 8; k += 2)
                        if (lmp_state_ok(px, py, k)) { nn = k; break; }
                int ax, ay;
                state_px(t->x, t->y, t->n, &ax, &ay);
                lmp_burst(ax, ay, here == '{' || here == '}' ? C_ORANGE : C_SKY, 8, 2);
                t->x = px;
                t->y = py;
                t->n = nn;
                t->portal_lock = true;
                state_px(px, py, nn, &ax, &ay);
                lmp_burst(ax, ay, here == '{' || here == '}' ? C_ORANGE : C_SKY, 8, 2);
                lmp_sfx("lmp_portal");
            }
        }
    } else {
        t->portal_lock = false;
    }
    if (!lmp_axis(t->n)) return;
    int sx = t->x - LMP_DX[t->n], sy = t->y - LMP_DY[t->n];
    char sup = lmp_tile(sx, sy);
    if (sup == '^') { lmp_kill_tank(1); return; }
    if (sup == 'C' && lg.level != LV_HUB) {
        if (!lg.check.set || lg.check.x != t->x || lg.check.y != t->y || lg.check.n != t->n) {
            bool was = lg.check.set && (lg.check.x / 3 == t->x / 3) && (lg.check.y / 3 == t->y / 3);
            lmp_take_checkpoint();
            if (!was) lmp_sfx("lmp_check");
        }
    }
    if (sup == '!' && lg.honey_x >= 0 && !lg.honey_shown) {
        lg.honey_shown = true;
        lmp_sfx("lmp_reveal");
        lmp_burst(lg.honey_x * LMP_T + 5, lg.honey_y * LMP_T + 5, C_YELLOW, 12, 2);
    }
    if (sup == 'W') t->hop_t = 8;
    if (lg.honey_shown && !lg.honey_held && t->x == lg.honey_x && t->y == lg.honey_y) {
        lg.honey_held = true;
        lmp_sfx("lmp_honey");
        lmp_burst(t->x * LMP_T + 5, t->y * LMP_T + 5, C_AMBER, 14, 2);
    }
    if (lg.bulb_out && t->x == lg.exit_x && t->y == lg.exit_y && lg.level != LV_HUB) lmp_level_cleared();
}

/* a tank carried by a platform still picks up the honey and finds the pod */
void lmp_tank_touch(void) {
    LmpTank *t = &lg.tk;
    if (lg.honey_shown && !lg.honey_held && t->x == lg.honey_x && t->y == lg.honey_y) {
        lg.honey_held = true;
        lmp_sfx("lmp_honey");
    }
    if (lg.bulb_out && t->x == lg.exit_x && t->y == lg.exit_y && lg.level != LV_HUB) lmp_level_cleared();
}

/* ---- warping ------------------------------------------------------------- */

static void blast_hits(void) {
    LmpTank *t = &lg.tk;
    /* lateral flares along the surface, two cells each side of the landing */
    int tx = (t->n + 2) & 7;
    for (int i = 0; i < LMP_MAX_FOES; i++) {
        LmpFoe *f = &lg.foe[i];
        if (!f->alive || f->kind == FO_BALL || f->kind == FO_FLAME) continue;
        int fx = f->x / LMP_FX, fy = f->y / LMP_FX;
        for (int k = -2; k <= 2; k++) {
            if (k == 0) continue;
            int cx = (t->x + k * LMP_DX[tx]) * LMP_T + 5, cy = (t->y + k * LMP_DY[tx]) * LMP_T + 5;
            if (iabs(fx - cx) < 5 + f->w / 2 && iabs(fy - cy) < 5 + f->h / 2) { lmp_hurt_foe(i, 2); break; }
        }
    }
}

static bool do_warp(void) {
    LmpTank *t = &lg.tk;
    int ox, oy, on;
    int r = lmp_warp_target(t->x, t->y, t->n, &ox, &oy, &on);
    if (r < 0) {
        t->fizz_t = 14;
        lmp_sfx("lmp_fizzle");
        return false;
    }
    state_px(t->x, t->y, t->n, &t->warp_from_x, &t->warp_from_y);
    t->warp_fx_t = 12;
    t->x = ox;
    t->y = oy;
    t->n = on;
    t->lag = LMP_WARP_LAG;
    t->warp_cool = LMP_WARP_COOL;
    t->blast_t = LMP_BLAST_T;
    t->blast_x = ox;
    t->blast_y = oy;
    t->blast_n = on;
    t->sense = 0;
    lmp_sfx("lmp_warp");
    blast_hits();
    arrive();
    return true;
}

/* ---- shooting ------------------------------------------------------------- */

static void fire(void) {
    LmpTank *t = &lg.tk;
    int alive = 0;
    for (int i = 0; i < LMP_MAX_SHOTS; i++) alive += lg.shot[i].alive;
    if (alive >= LMP_MAX_SHOTS) return;
    int n = shown_normal();
    int cx, cy;
    lmp_tank_centre(&cx, &cy);
    for (int i = 0; i < LMP_MAX_SHOTS; i++) {
        LmpShot *s = &lg.shot[i];
        if (s->alive) continue;
        s->alive = 1;
        s->n = n;
        s->x = (cx + LMP_DX[n] * 4) * LMP_FX;
        s->y = (cy + LMP_DY[n] * 4) * LMP_FX;
        s->vx = LMP_DX[n] * LMP_SHOT_SPEED;
        s->vy = LMP_DY[n] * LMP_SHOT_SPEED;
        break;
    }
    t->fire_gap = LMP_MASH_GAP;
    lmp_sfx("lmp_shot");
}

static bool plate_lets(char c, int n) {
    return (c == '=' && n == DR_U) || (c == '-' && n == DR_D) || (c == '[' && n == DR_R) || (c == ']' && n == DR_L);
}

void lmp_shots_update(void) {
    for (int i = 0; i < LMP_MAX_SHOTS; i++) {
        LmpShot *s = &lg.shot[i];
        if (!s->alive) continue;
        /* two half moves, so a shot never skips a thin thing */
        for (int h = 0; h < 2 && s->alive; h++) {
            s->x += s->vx / 2;
            s->y += s->vy / 2;
            int px = s->x / LMP_FX, py = s->y / LMP_FX;
            if (px < 0 || py < 0 || px >= lg.mw * LMP_T || py >= lg.mh * LMP_T) { s->alive = 0; break; }
            int cx = px / LMP_T, cy = py / LMP_T;
            char c = lmp_tile(cx, cy);
            if (lmp_shot_blocked(cx, cy) && !plate_lets(c, s->n)) {
                lmp_shoot_tile(cx, cy);
                s->alive = 0;
                break;
            }
            for (int f = 0; f < LMP_MAX_FOES; f++) {
                LmpFoe *fo = &lg.foe[f];
                if (!fo->alive) continue;
                if (iabs(fo->x / LMP_FX - px) <= fo->w / 2 + 1 && iabs(fo->y / LMP_FX - py) <= fo->h / 2 + 1) {
                    lmp_hurt_foe(f, 1);
                    s->alive = 0;
                    break;
                }
            }
        }
    }
}

/* ---- the tank each frame ------------------------------------------------- */

static int held_dir(void) {
    if (lmp_btn(BTN_UP)) return DR_U;
    if (lmp_btn(BTN_DOWN)) return DR_D;
    if (lmp_btn(BTN_LEFT)) return DR_L;
    if (lmp_btn(BTN_RIGHT)) return DR_R;
    return -1;
}

static int btn_of(int d) {
    return d == DR_U ? BTN_UP : d == DR_D ? BTN_DOWN : d == DR_L ? BTN_LEFT : BTN_RIGHT;
}

static bool try_push(int sense) {
    LmpTank *t = &lg.tk;
    int d = (t->n + 2 * sense) & 7;
    int bx = t->x + LMP_DX[d], by = t->y + LMP_DY[d];
    if (lmp_tile(bx, by) != 'P') return false;
    int cx = bx + LMP_DX[d], cy = by + LMP_DY[d];
    if (lmp_tile(cx, cy) != ' ' && lmp_tile(cx, cy) != '.') return false;
    if (cx < 0 || cy < 0 || cx >= lg.mw || cy >= lg.mh) return false;
    /* only if the tank has something to roll on where the block was */
    if (!lmp_face_solid(bx - LMP_DX[t->n], by - LMP_DY[t->n], t->n)) return false;
    lg.map[cy][cx] = 'P';
    lg.map[by][bx] = ' ';
    lmp_sfx("lmp_push");
    return true;
}

static void start_roll(int sense) {
    LmpTank *t = &lg.tk;
    int n = lmp_roll_path(t->x, t->y, t->n, sense, t->px, t->py, t->pnn, 8);
    if (n == 0 && try_push(sense)) n = lmp_roll_path(t->x, t->y, t->n, sense, t->px, t->py, t->pnn, 8);
    if (n == 0) {
        /* running into spikes is as bad as landing on them */
        int d = (t->n + 2 * sense) & 7;
        if (lmp_is_spikes(t->x + LMP_DX[d], t->y + LMP_DY[d])) lmp_kill_tank(2);
        t->sense = 0;
        return;
    }
    t->sx = t->x;
    t->sy = t->y;
    t->sn = t->n;
    t->pn = n;
    t->pt = 0;
    t->plen = 0;
    for (int i = 0; i < n; i++) t->plen += step_dur(t, i);
    t->sense = sense;
}

void lmp_tank_update(void) {
    LmpTank *t = &lg.tk;
    if (t->lag > 0) t->lag--;
    if (t->warp_cool > 0) t->warp_cool--;
    if (t->fire_gap > 0) t->fire_gap--;
    if (t->blast_t > 0) t->blast_t--;
    if (t->warp_fx_t > 0) t->warp_fx_t--;
    if (t->fizz_t > 0) t->fizz_t--;

    /* shooting: a press fires, holding fires the rest of a burst of three */
    if (lmp_btnp(BTN_A)) {
        if (t->fire_gap == 0) {
            fire();
            t->burst = LMP_BURST - 1;
            t->burst_t = LMP_BURST_GAP;
        }
    } else if (lmp_btn(BTN_A) && t->burst > 0) {
        if (--t->burst_t <= 0) {
            fire();
            t->burst--;
            t->burst_t = LMP_BURST_GAP;
        }
    }
    if (!lmp_btn(BTN_A)) t->burst = 0;

    /* rolling */
    if (t->pn > 0) {
        t->pt++;
        if (t->pt >= t->plen) {
            t->x = t->px[t->pn - 1];
            t->y = t->py[t->pn - 1];
            t->n = t->pnn[t->pn - 1];
            t->pn = 0;
            arrive();
        }
        if (t->pn > 0 || lg.state != GS_PLAY) return;
    }
    if (lg.state != GS_PLAY) return;
    lmp_tank_touch(); /* a jar dropped right where it waits */
    if (lg.state != GS_PLAY) return;
    if (t->hop_t > 0) {
        if (--t->hop_t == 0) { lmp_sfx("lmp_hop"); do_warp(); }
        return;
    }
    /* the warp (in the hub, B at a pod opens it instead: lmp.c) */
    if (lmp_btnp(BTN_B) && t->warp_cool == 0 && !(lg.level == LV_HUB && lg.pod_here >= 0)) {
        do_warp();
        return;
    }
    if (t->lag > 0) return;
    int d = held_dir();
    int sense = 0;
    if (d >= 0) {
        if (d == ((t->n + 2) & 7)) sense = 1;
        else if (d == ((t->n + 6) & 7)) sense = -1;
        else if (t->sense != 0 && t->run_btn && lmp_btn(t->run_btn)) sense = t->sense;
    }
    if (sense == 0) { t->sense = 0; t->run_btn = 0; return; }
    if (t->sense == 0 || !lmp_btn(t->run_btn)) t->run_btn = btn_of(d);
    start_roll(sense);
}
