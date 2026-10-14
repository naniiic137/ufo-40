/* LIMPET - the demo player, for the tests. It presses real buttons.
 *
 * It finds its way with a search over the tank's own moves (roll either way
 * along the surface, warp across), using the same rules as the game. Each
 * deck has a short plan written next to its map (see lmp_levels.c): go to a
 * mark, fire, warp, shoot the bulb, take the honey, leave. Whenever danger is
 * near it looks ahead: it runs copies of the game with the move it wants,
 * with waiting, and with the other moves, and keeps one that survives.
 *
 * Plan tokens (spaces between):
 *   1U 2D 3L 4R 5*   go to mark 1 (cannon up / down / left / right / any way)
 *   F                fire a burst      f   one shot     W   warp      wN  wait N frames
 *   Tc               shoot (once) the nearest tile c in a line: Td a door switch,
 *                    TS a swap switch, TO an outline block, TB a breakable
 *   B                shoot the red bulb
 *   E                go into the exit pod
 *   H                touch the hidden honey (honey runs only)
 *   marks are the digits (not 7) and a g i j k l x
 *   ( ... )          only on honey runs
 *   m                wait for a moving platform under the warp line, then warp onto it
 *   V                shoot the switch worm until the swap blocks change over
 *   K                hunt: shoot every foe until the pod opens (survival, Turncoat)
 *   D                fight the Dreadnought */
#include "lmp.h"

int lmp_bot_honey;

#define NSTATE (LMP_MW * LMP_MH * 4)
static int16_t b_prev_x[NSTATE], b_prev_y[NSTATE];
static uint8_t b_prev_n[NSTATE], b_act[NSTATE], b_seen[NSTATE];
static int b_queue[NSTATE];
static int b_stamp;
static int b_found_x, b_found_y, b_found_n; /* the goal state the last search found */
static uint16_t b_depth[NSTATE];
static int b_goal_depth;                     /* moves to the state being tested */

enum { ACT_NONE, ACT_CW, ACT_CCW, ACT_WARP };

static int key(int x, int y, int n) { return (y * LMP_MW + x) * 4 + n / 2; }

/* where a move ends, with portals and hopper pads followed */
static bool settle(int *x, int *y, int *n) {
    for (int k = 0; k < 4; k++) {
        char here = lmp_tile(*x, *y);
        if (here == '{' || here == '}' || here == '<' || here == '>') {
            char want = here == '{' ? '}' : here == '}' ? '{' : here == '<' ? '>' : '<';
            for (int yy = 0; yy < lg.mh; yy++)
                for (int xx = 0; xx < lg.mw; xx++)
                    if (lg.map[yy][xx] == want) {
                        int nn = *n;
                        if (!lmp_state_ok(xx, yy, nn))
                            for (int d = 0; d < 8; d += 2)
                                if (lmp_state_ok(xx, yy, d)) { nn = d; break; }
                        *x = xx; *y = yy; *n = nn;
                        goto moved;
                    }
        moved:;
        }
        int sx = *x - LMP_DX[*n], sy = *y - LMP_DY[*n];
        char sup = lmp_tile(sx, sy);
        if (sup == '^' || lmp_is_crusher(sx, sy)) return false;
        if (sup != 'W') return true;
        int ox, oy, on;
        if (lmp_warp_target(*x, *y, *n, &ox, &oy, &on) < 0) return true;
        *x = ox; *y = oy; *n = on;
        if (lmp_is_spikes(ox - LMP_DX[on], oy - LMP_DY[on])) return false;
    }
    return true;
}

static bool edge(int x, int y, int n, int act, int *ox, int *oy, int *on) {
    if (act == ACT_WARP) {
        if (lmp_warp_target(x, y, n, ox, oy, on) < 0) return false;
        if (lmp_is_spikes(*ox - LMP_DX[*on], *oy - LMP_DY[*on]) || lmp_is_crusher(*ox - LMP_DX[*on], *oy - LMP_DY[*on])) return false;
    } else {
        int s = act == ACT_CW ? 1 : -1;
        int px[8], py[8], pn[8];
        int k = lmp_roll_path(x, y, n, s, px, py, pn, 8);
        if (k == 0) {
            /* a push block ahead that can move */
            int d = (n + 2 * s) & 7;
            int bx = x + LMP_DX[d], by = y + LMP_DY[d];
            if (lmp_tile(bx, by) != 'P') return false;
            char beyond = lmp_tile(bx + LMP_DX[d], by + LMP_DY[d]);
            if (beyond != ' ' && beyond != '.') return false;
            if (!lmp_face_solid(bx - LMP_DX[n], by - LMP_DY[n], n)) return false;
            *ox = bx; *oy = by; *on = n;
            return true;
        }
        *ox = px[k - 1]; *oy = py[k - 1]; *on = pn[k - 1];
    }
    return settle(ox, oy, on);
}

typedef bool (*Goal)(int x, int y, int n, const void *arg);

/* cells a foe sits in: the search goes round them */
static uint8_t b_foe_cell[LMP_MH][LMP_MW];
static bool b_nofoes; /* a search that ignores foes (to see if one is in the way) */
static void mark_foes(void) {
    memset(b_foe_cell, 0, sizeof b_foe_cell);
    if (b_nofoes) return;
    /* a crusher's path is off limits unless it is resting at the top with time to spare */
    for (int i = 0; i < lg.nmover; i++) {
        const LmpMover *m = &lg.mover[i];
        if (!m->lethal) continue;
        if ((m->pos == 0 && m->wait > 20) || (m->sign < 0 && m->pos <= 2)) continue;
        int dx = LMP_DX[m->dir], dy = LMP_DY[m->dir];
        for (int k = 0; k <= m->dist; k++)
            for (int y = 0; y < m->h; y++)
                for (int x = 0; x < m->w; x++) {
                    int cx = m->x - dx * m->pos + dx * k + x, cy = m->y - dy * m->pos + dy * k + y;
                    if (cx >= 0 && cy >= 0 && cx < LMP_MW && cy < LMP_MH) b_foe_cell[cy][cx] = 1;
                }
    }
    for (int i = 0; i < LMP_MAX_FOES; i++) {
        const LmpFoe *f = &lg.foe[i];
        if (!f->alive || f->kind == FO_FLAME) continue;
        if (f->kind == FO_DREAD) {
            /* its whole body and barrel, and a cell round them */
            for (int y = f->cy; y <= f->cy + 8 && y < LMP_MH; y++)
                for (int x = f->cx - 7; x <= f->cx + 7; x++)
                    if (x >= 0 && x < LMP_MW && (y <= f->cy + 5 || iabs(x - f->cx) <= 1)) b_foe_cell[y][x] = 1;
            continue;
        }
        int cx = f->x / LMP_FX / LMP_T, cy = f->y / LMP_FX / LMP_T;
        /* a foe that moves only counts where it is if it is close by: far off it will have moved on */
        bool fixed = f->kind == FO_TURRET || f->kind == FO_LASER || f->kind == FO_SAC;
        if (!fixed && (iabs(cx - lg.tk.x) > 6 || iabs(cy - lg.tk.y) > 6)) continue;
        /* fliers are in the air, not on the way: the look-ahead deals with them */
        if ((f->kind == FO_SPECK || f->kind == FO_EYE || f->kind == FO_DRONE || f->kind == FO_WORM || f->kind == FO_BALL) &&
            (iabs(cx - lg.tk.x) > 2 || iabs(cy - lg.tk.y) > 2))
            continue;
        if (cx >= 0 && cy >= 0 && cx < LMP_MW && cy < LMP_MH) b_foe_cell[cy][cx] = 1;
    }
}

/* breadth-first over the tank's states; returns the first move, ACT_NONE when
 * already there, -1 when there is no way */
static int bfs(Goal goal, const void *arg, int *dist) {
    const LmpTank *t = &lg.tk;
    b_stamp++;
    if (b_stamp >= 250) { memset(b_seen, 0, sizeof b_seen); b_stamp = 1; }
    b_goal_depth = 0;
    if (goal(t->x, t->y, t->n, arg)) {
        if (dist) *dist = 0;
        b_found_x = t->x; b_found_y = t->y; b_found_n = t->n;
        return ACT_NONE;
    }
    int head = 0, tail = 0;
    mark_foes();
    int s = key(t->x, t->y, t->n);
    b_seen[s] = (uint8_t)b_stamp;
    b_prev_x[s] = -1;
    b_depth[s] = 0;
    b_queue[tail++] = s;
    while (head < tail) {
        int k = b_queue[head++];
        int n = (k & 3) * 2, c = k >> 2, x = c % LMP_MW, y = c / LMP_MW;
        for (int a = ACT_CW; a <= ACT_WARP; a++) {
            int ox, oy, on;
            if (!edge(x, y, n, a, &ox, &oy, &on)) continue;
            if (!lmp_axis(on)) continue;
            if (b_foe_cell[oy][ox]) continue;
            int k2 = key(ox, oy, on);
            if (b_seen[k2] == b_stamp) continue;
            b_seen[k2] = (uint8_t)b_stamp;
            b_prev_x[k2] = (int16_t)x;
            b_prev_y[k2] = (int16_t)y;
            b_prev_n[k2] = (uint8_t)n;
            b_act[k2] = (uint8_t)a;
            b_depth[k2] = (uint16_t)(b_depth[k] + 1);
            b_goal_depth = b_depth[k2];
            if (goal(ox, oy, on, arg)) {
                b_found_x = ox; b_found_y = oy; b_found_n = on;
                /* walk back to the first move */
                int steps = 1, cur = k2;
                while (!(b_prev_x[cur] == t->x && b_prev_y[cur] == t->y && b_prev_n[cur] == t->n)) {
                    cur = key(b_prev_x[cur], b_prev_y[cur], b_prev_n[cur]);
                    steps++;
                    if (steps > NSTATE) return -1;
                }
                if (dist) *dist = steps;
                return b_act[cur];
            }
            if (tail < NSTATE) b_queue[tail++] = k2;
        }
    }
    return -1;
}

typedef struct { int x, y, n; } Spot;
static bool goal_spot(int x, int y, int n, const void *arg) {
    const Spot *s = arg;
    return x == s->x && y == s->y && (s->n < 0 || s->n == n);
}
static bool goal_support(int x, int y, int n, const void *arg) {
    const Spot *s = arg;
    return x - LMP_DX[n] == s->x && y - LMP_DY[n] == s->y;
}

/* the first thing a shot from this state would hit */
static char line_hit(int x, int y, int n, int *hx, int *hy) {
    for (int k = 1; k < LMP_MW; k++) {
        int cx = x + k * LMP_DX[n], cy = y + k * LMP_DY[n];
        if (cx < 0 || cy < 0 || cx >= lg.mw || cy >= lg.mh) return 0;
        char c = lmp_tile(cx, cy);
        bool plate_ok = (c == '=' && n == DR_U) || (c == '-' && n == DR_D) || (c == '[' && n == DR_R) || (c == ']' && n == DR_L);
        if (lmp_shot_blocked(cx, cy) && !plate_ok) { *hx = cx; *hy = cy; return c; }
    }
    return 0;
}
static bool goal_bulb(int x, int y, int n, const void *arg) {
    (void)arg;
    int hx, hy;
    return line_hit(x, y, n, &hx, &hy) == 'R';
}
static bool goal_hit(int x, int y, int n, const void *arg) {
    int hx, hy;
    return line_hit(x, y, n, &hx, &hy) == *(const char *)arg;
}

/* each foe's speed, measured frame to frame (pixels x 16 a frame), for aiming ahead */
static int f_lx[LMP_MAX_FOES], f_ly[LMP_MAX_FOES], f_vx[LMP_MAX_FOES], f_vy[LMP_MAX_FOES];
static void track_foes(void) {
    for (int i = 0; i < LMP_MAX_FOES; i++) {
        const LmpFoe *f = &lg.foe[i];
        int x = f->x / LMP_FX * 16, y = f->y / LMP_FX * 16;
        if (!f->alive) { f_vx[i] = f_vy[i] = 0; f_lx[i] = x; f_ly[i] = y; continue; }
        f_vx[i] = (f_vx[i] * 3 + (x - f_lx[i])) / 4;
        f_vy[i] = (f_vy[i] * 3 + (y - f_ly[i])) / 4;
        f_lx[i] = x;
        f_ly[i] = y;
    }
}

static bool b_fixed_only; /* only foes that sit still (turrets, lasers, sacs) */
static bool foe_in_line_ex(int x, int y, int n, int range, bool lead) {
    int px = x * LMP_T + 5, py = y * LMP_T + 5;
    int hx = -1, hy = -1;
    line_hit(x, y, n, &hx, &hy);
    int lim = hx >= 0 ? imax(iabs(hx - x), iabs(hy - y)) * LMP_T : range * LMP_T;
    lim = imin(lim, range * LMP_T);
    for (int i = 0; i < LMP_MAX_FOES; i++) {
        const LmpFoe *f = &lg.foe[i];
        if (!f->alive || f->kind == FO_BALL || f->kind == FO_FLAME || f->kind == FO_SWITCHWORM || f->kind == FO_DREAD) continue;
        if (b_fixed_only && f->kind != FO_TURRET && f->kind != FO_LASER && f->kind != FO_SAC) continue;
        int fx = f->x / LMP_FX, fy = f->y / LMP_FX;
        int along = (fx - px) * LMP_DX[n] + (fy - py) * LMP_DY[n];
        if (lead && along > 0 && f->kind != FO_TURNCOAT) {
            /* where it will be when a shot gets there */
            int t = along / 4;
            fx += f_vx[i] * t / 16;
            fy += f_vy[i] * t / 16;
            along = (fx - px) * LMP_DX[n] + (fy - py) * LMP_DY[n];
        }
        int across = LMP_DX[n] ? iabs(fy - py) : iabs(fx - px);
        if (along > 0 && along <= lim && across <= f->h / 2 + 1) return true;
    }
    return false;
}

/* where every foe will be over the next few seconds if the tank keeps still */
#define PRED_N 50
#define PRED_DT 3
static int16_t p_x[PRED_N][LMP_MAX_FOES], p_y[PRED_N][LMP_MAX_FOES];
static uint8_t p_alive[PRED_N][LMP_MAX_FOES];
static LmpGame p_save;
static void predict_foes(void) {
    memcpy(&p_save, &lg, sizeof lg);
    bool keep_sim = lmp_simulating;
    lmp_simulating = true;
    bool god = lg.god;
    lg.god = true; /* the foes go on even if they would get the tank */
    lmp_sim_prev = lmp_sim_cur = 0;
    for (int k = 0; k < PRED_N; k++) {
        for (int i = 0; i < LMP_MAX_FOES; i++) {
            const LmpFoe *f = &lg.foe[i];
            p_alive[k][i] = f->alive && f->kind != FO_BALL && f->kind != FO_FLAME && f->kind != FO_SWITCHWORM && f->kind != FO_DREAD;
            p_x[k][i] = (int16_t)(f->x / LMP_FX);
            p_y[k][i] = (int16_t)(f->y / LMP_FX);
        }
        for (int f = 0; f < PRED_DT && lg.state == GS_PLAY; f++) lmp_play_step();
    }
    lg.god = god;
    memcpy(&lg, &p_save, sizeof lg);
    lmp_simulating = keep_sim;
}

/* a state from which a shot, fired once the tank gets there, meets a foe */
static int b_hunt_kind; /* only this kind of foe (0: any) */
static bool goal_intercept(int x, int y, int n, const void *arg) {
    (void)arg;
    int hx = -1, hy = -1;
    line_hit(x, y, n, &hx, &hy);
    int lim = hx >= 0 ? imax(iabs(hx - x), iabs(hy - y)) * LMP_T : 16 * LMP_T;
    lim = imin(lim, 16 * LMP_T);
    int px = x * LMP_T + 5, py = y * LMP_T + 5;
    int t_arr = b_goal_depth * LMP_STEP_T + 4;
    for (int wait = 0; wait <= 30; wait += 6) {
        int k0 = (t_arr + wait) / PRED_DT;
        if (k0 >= PRED_N) break;
        for (int i = 0; i < LMP_MAX_FOES; i++) {
            if (!p_alive[k0][i]) continue;
            if (b_hunt_kind && lg.foe[i].kind != b_hunt_kind) continue;
            int along = (p_x[k0][i] - px) * LMP_DX[n] + (p_y[k0][i] - py) * LMP_DY[n];
            if (along <= 0 || along > lim) continue;
            int k = (t_arr + wait + along / 4) / PRED_DT;
            if (k >= PRED_N || !p_alive[k][i]) continue;
            int ax = p_x[k][i], ay = p_y[k][i];
            along = (ax - px) * LMP_DX[n] + (ay - py) * LMP_DY[n];
            int across = LMP_DX[n] ? iabs(ay - py) : iabs(ax - px);
            if (along > 0 && along <= lim && across <= 4) return true;
        }
    }
    return false;
}

static bool goal_hunt(int x, int y, int n, const void *arg) {
    /* where a foe will be in the line of fire by the time a shot gets there */
    (void)arg;
    return foe_in_line_ex(x, y, n, 14, true);
}

/* ---- looking ahead ----------------------------------------------------------- */

static LmpGame b_save;

static bool threat_at(int tx, int ty);
static bool threat_near(void) {
    int tx, ty;
    lmp_tank_centre(&tx, &ty);
    return threat_at(tx, ty);
}
static bool threat_at(int tx, int ty) {
    for (int i = 0; i < LMP_MAX_FOES; i++) {
        const LmpFoe *f = &lg.foe[i];
        if (!f->alive) continue;
        int r = f->kind == FO_DREAD ? 400 : f->kind == FO_FLAME ? 70 : f->kind == FO_TURRET || f->kind == FO_LASER ? 400 : 90;
        if (iabs(f->x / LMP_FX - tx) < r && iabs(f->y / LMP_FX - ty) < r) return true;
    }
    for (int i = 0; i < LMP_MAX_ESHOTS; i++)
        if (lg.es[i].alive && iabs(lg.es[i].x / LMP_FX - tx) < 120 && iabs(lg.es[i].y / LMP_FX - ty) < 120) return true;
    for (int i = 0; i < lg.nmover; i++)
        if (iabs(lg.mover[i].x - tx / LMP_T) < 6 && iabs(lg.mover[i].y - ty / LMP_T) < 6) return true;
    if (lg.timed_t > 0) return true;
    return false;
}

static int act_buttons(int act) {
    const LmpTank *t = &lg.tk;
    if (act == ACT_WARP) return BTN_B;
    if (act == ACT_CW || act == ACT_CCW) {
        int d = (t->n + (act == ACT_CW ? 2 : 6)) & 7;
        return d == DR_U ? BTN_UP : d == DR_D ? BTN_DOWN : d == DR_L ? BTN_LEFT : BTN_RIGHT;
    }
    return 0;
}

/* run the game ahead from where it is now: `buttons` for `hold` frames, then
 * nothing, for `frames` in all; returns the frames the tank lived (all of them
 * if it lives) */
static int run_ahead(int buttons, int hold, int frames) {
    lmp_sim_prev = 0;
    for (int f = 0; f < frames; f++) {
        lmp_sim_cur = f < hold ? (uint32_t)buttons : 0;
        lmp_play_step();
        lmp_sim_prev = lmp_sim_cur;
        if (lg.state == GS_DEAD) return f;
        if (lg.state != GS_PLAY) return frames;
    }
    return frames;
}

static LmpGame b_mid;

/* how long the tank lives after a move and then the best second move (or
 * waiting): a look two moves deep. The full answer is first + frames + 20. */
static int survive_t(int buttons, int hold, int frames, int *full) {
    memcpy(&b_save, &lg, sizeof lg);
    bool keep_sim = lmp_simulating;
    lmp_simulating = true;
    int first = hold > 1 ? hold + 2 : (buttons & BTN_B) ? LMP_WARP_COOL + 1 : LMP_STEP_T + 2;
    *full = first + frames + 20;
    int best = run_ahead(buttons, hold, first);
    if (best >= first && lg.state == GS_PLAY) {
        memcpy(&b_mid, &lg, sizeof lg);
        static const int SECOND[6] = {0, BTN_B, -1, -2, -3, -4};
        int top = 0;
        for (int k = 0; k < 6 && top < frames + 20; k++) {
            if (k > 0) memcpy(&lg, &b_mid, sizeof lg);
            int b = SECOND[k], h = 2;
            if (b < 0) {
                /* roll either way along the surface it is on: one cell, or keep going */
                int d = (lg.tk.n + ((b & 1) ? 2 : 6)) & 7;
                h = b <= -3 ? LMP_STEP_T * 4 : LMP_STEP_T;
                b = d == DR_U ? BTN_UP : d == DR_D ? BTN_DOWN : d == DR_L ? BTN_LEFT : BTN_RIGHT;
            }
            top = imax(top, run_ahead(b, h, frames + 20));
        }
        best = first + top;
    } else if (best >= first) {
        best = *full;
    }
    memcpy(&lg, &b_save, sizeof lg);
    lmp_simulating = keep_sim;
    return best;
}

static bool survives(int buttons, int hold, int frames) {
    int full;
    return survive_t(buttons, hold, frames, &full) >= full;
}

/* ---- the plan ------------------------------------------------------------------ */

static int r_level = -1, r_pos, r_wait, r_fire_t, r_level_t, r_depth;
static int r_check_pos, r_cx = -1, r_cy, r_cn;
static int r_v_pos = -1, r_v_par, r_v_t, r_stall;
static int r_hunt_t, r_hx, r_hy, r_hn;
static int f_shot_cache_frame = -1, f_shot_cache;
static bool r_restore;
static uint32_t r_prev_mask;
static bool r_skipping;

static const char *route_of(void) {
    const char *r = LMP_LEVEL[lg.level]->route;
    return r ? r : "B E";
}

static int next_token(const char *r, int pos, char *tok) {
    while (r[pos] == ' ') pos++;
    int n = 0;
    while (r[pos] && r[pos] != ' ' && n < 15) tok[n++] = r[pos++];
    tok[n] = 0;
    return pos;
}

static int press(int mask) {
    /* a fresh press needs the button up the frame before */
    if (mask & (BTN_A | BTN_B)) {
        if (r_prev_mask & mask & (BTN_A | BTN_B)) mask &= ~(BTN_A | BTN_B);
    }
    return mask;
}

/* one move toward a goal, looking ahead when it is dangerous */
static int go(Goal goal, const void *arg, bool *arrived) {
    LmpTank *t = &lg.tk;
    *arrived = false;
    if (t->pn > 0 && t->pt + 1 >= t->plen) {
        /* arriving this frame: hold the next roll's button now so it carries on
         * without a pause (the look-ahead assumes it does) */
        LmpTank keep = *t;
        t->x = t->px[t->pn - 1];
        t->y = t->py[t->pn - 1];
        t->n = t->pnn[t->pn - 1];
        t->pn = 0;
        int act = goal(t->x, t->y, t->n, arg) ? ACT_NONE : bfs(goal, arg, NULL);
        int want = (act == ACT_CW || act == ACT_CCW) ? act_buttons(act) : 0;
        if (want && keep.sense != 0 && keep.sense != (act == ACT_CW ? 1 : -1)) want = 0;
        /* never stop in a crusher's path: keep rolling */
        bool under = false;
        for (int i = 0; i < lg.nmover; i++) {
            const LmpMover *mv = &lg.mover[i];
            if (!mv->lethal) continue;
            if (t->x >= mv->x && t->x < mv->x + mv->w && t->y >= mv->y && t->y <= mv->y + mv->h + mv->dist) under = true;
        }
        *t = keep;
        if (under && keep.run_btn) return want ? want : keep.run_btn;
        if (!want) return 0;
        if (!threat_near()) return want;
        int full, full2;
        int wt = survive_t(want, LMP_STEP_T + 1, 50, &full);
        if (wt >= full) return want;
        /* stopping here is only better if it lives longer */
        return wt >= survive_t(0, 0, 50, &full2) ? want : 0;
    }
    if (t->pn > 0 || t->lag > 0 || t->hop_t > 0) return 0;
    int act = bfs(goal, arg, NULL);
    if (act == ACT_NONE) { *arrived = true; return 0; }
    bool danger = threat_near();
    if (act < 0) {
        /* a timed door shut again: shoot its switch */
        if (lg.timed_t == 0 && lmp_count_tiles('y') > 0 && lmp_count_tiles('Y') > 0) {
            char want_c = 'y';
            int d2;
            int a2 = bfs(goal_hit, &want_c, &d2);
            if (a2 == ACT_NONE) return (t->fire_gap == 0 && !(r_prev_mask & BTN_A)) ? BTN_A : 0;
            if (a2 > 0) return act_buttons(a2);
        }
        /* a foe sitting in the way: go and shoot it */
        int d3;
        b_nofoes = true;
        int clear = bfs(goal, arg, &d3);
        b_nofoes = false;
        if (clear >= 0) {
            b_fixed_only = true;
            int a3 = bfs(goal_hunt, NULL, &d3);
            b_fixed_only = false;
            if (a3 == ACT_NONE) return (t->fire_gap == 0 && !(r_prev_mask & BTN_A)) ? BTN_A : 0;
            if (a3 > 0) {
                int b3 = act_buttons(a3);
                if (!danger || survives(b3, a3 == ACT_WARP ? 1 : LMP_STEP_T, 50)) return b3;
            }
        }
        /* no way yet (a door, a platform): keep safe and wait */
        if (danger && !survives(0, 0, 45)) {
            static const int ALT[3] = {ACT_WARP, ACT_CW, ACT_CCW};
            for (int k = 0; k < 3; k++)
                if (survives(act_buttons(ALT[k]), 2, 45)) return act_buttons(ALT[k]);
        }
        return 0;
    }
    int want = act_buttons(act);
    if (act == ACT_WARP && t->warp_cool > 0) return 0;
    /* a held button keeps an old run going its old way: let go first */
    if ((act == ACT_CW || act == ACT_CCW) && (r_prev_mask & want) && t->sense != 0 &&
        t->sense != (act == ACT_CW ? 1 : -1))
        return 0;
    if (!danger) {
        /* where the move ends can be dangerous too */
        int ox, oy, on;
        if (edge(t->x, t->y, t->n, act, &ox, &oy, &on)) danger = threat_at(ox * LMP_T + 5, oy * LMP_T + 5);
    }
    if (!danger) return want;
    int full;
    int wt = survive_t(want, act == ACT_WARP ? 1 : LMP_STEP_T, 50, &full);
    if (wt >= full) { r_stall = 0; return want; }
    r_stall++;
    int best = 0, best_t = survive_t(0, 0, 50, &full);
    static const int ALT[3] = {ACT_WARP, ACT_CW, ACT_CCW};
    for (int k = 0; k < 3 && best_t < full; k++) {
        int b = act_buttons(ALT[k]);
        if (!b || b == want) continue;
        if (ALT[k] == ACT_WARP && t->warp_cool > 0) continue;
        int bt = survive_t(b, ALT[k] == ACT_WARP ? 1 : LMP_STEP_T, 50, &full);
        if (bt > best_t) { best_t = bt; best = b; }
    }
    if (wt >= best_t) return want;
    return best;
}

/* would a shot fired now hit a foe? The game is run ahead to see. */
static bool shot_would_hit(void) {
    memcpy(&b_save, &lg, sizeof lg);
    bool keep_sim = lmp_simulating;
    lmp_simulating = true;
    int16_t hp0[LMP_MAX_FOES];
    uint8_t kind0[LMP_MAX_FOES];
    for (int i = 0; i < LMP_MAX_FOES; i++) { hp0[i] = lg.foe[i].alive ? lg.foe[i].hp : 0; kind0[i] = lg.foe[i].kind; }
    lmp_sim_prev = 0;
    bool hit = false;
    for (int f = 0; f < 45 && !hit; f++) {
        lmp_sim_cur = f == 0 ? BTN_A : 0;
        lmp_play_step();
        lmp_sim_prev = lmp_sim_cur;
        if (lg.state != GS_PLAY) break;
        /* one of the foes there now is hurt (new specks from a sac don't count) */
        for (int i = 0; i < LMP_MAX_FOES && !hit; i++)
            if (hp0[i] > 0 && (!lg.foe[i].alive || lg.foe[i].kind != kind0[i] || lg.foe[i].hp < hp0[i])) hit = true;
        int alive = 0;
        for (int i = 0; i < LMP_MAX_SHOTS; i++) alive += lg.shot[i].alive;
        if (f > 2 && alive == 0) break;
    }
    memcpy(&lg, &b_save, sizeof lg);
    lmp_simulating = keep_sim;
    return hit;
}

static int fire_if_foe(void) {
    const LmpTank *t = &lg.tk;
    if (t->pn != 0 || !lmp_axis(t->n) || t->fire_gap != 0 || (r_prev_mask & BTN_A)) return 0;
    /* only worth a look if something is roughly ahead */
    int px = t->x * LMP_T + 5, py = t->y * LMP_T + 5;
    bool near = false;
    for (int i = 0; i < LMP_MAX_FOES && !near; i++) {
        const LmpFoe *f = &lg.foe[i];
        if (!f->alive || f->kind == FO_BALL || f->kind == FO_FLAME || f->kind == FO_DREAD) continue;
        int fx = f->x / LMP_FX, fy = f->y / LMP_FX;
        int along = (fx - px) * LMP_DX[t->n] + (fy - py) * LMP_DY[t->n];
        int across = LMP_DX[t->n] ? iabs(fy - py) : iabs(fx - px);
        if (along > 0 && along < 180 && across < 40) near = true;
    }
    if (!near) return 0;
    if (f_shot_cache_frame == lg.frame_t) return f_shot_cache;
    f_shot_cache_frame = lg.frame_t;
    f_shot_cache = shot_would_hit() ? BTN_A : 0;
    return f_shot_cache;
}

static int dread_plan(void) {
    const LmpTank *t = &lg.tk;
    const LmpFoe *d = NULL;
    for (int i = 0; i < LMP_MAX_FOES; i++)
        if (lg.foe[i].alive && lg.foe[i].kind == FO_DREAD) d = &lg.foe[i];
    if (!d) return -1;
    /* under the barrel while the core is open; two cells aside otherwise */
    int col = d->cx;
    int want_x;
    /* it holds still while its core is open */
    if (d->state == 2) want_x = col;
    else {
        /* wait nine cells off on the side it is on; cross under only while the
         * barrel is quiet, and only toward the side with room */
        int side = t->x >= col ? 1 : -1;
        int off = d->hp > 16 ? 4 : 7; /* its shells only throw sparks once it is hurt */
        want_x = col + side * off;
        if (want_x < 2 || want_x > lg.mw - 3) {
            if (d->state == 0 && d->t < 40) want_x = col - side * off;
            else want_x = side > 0 ? lg.mw - 3 : 2;
        }
    }
    want_x = iclamp(want_x, 1, lg.mw - 2);
    int fy = lg.mh - 2;
    while (fy > 0 && !lmp_state_ok(want_x, fy, DR_U)) fy--;
    Spot s = {want_x, fy, DR_U};
    bool arrived;
    int b = go(goal_spot, &s, &arrived);
    if (t->pn == 0 && t->n == DR_U && t->x == d->cx && d->state == 2 && d->t < (d->hp > 16 ? 135 : d->hp > 8 ? 165 : 145) && t->fire_gap == 0) {
        if (survives(BTN_A, 1, 40)) return b | BTN_A;
    }
    return b;
}

static int level_plan(void) {
    const LmpTank *t = &lg.tk;
    if (lg.level != r_level) {
        r_level = lg.level; r_pos = 0; r_wait = 0; r_fire_t = 0; r_level_t = 0; r_depth = 0; r_skipping = false;
        r_check_pos = 0; r_cx = lg.check.x; r_cy = lg.check.y; r_cn = lg.check.n; r_restore = false;
    }
    if (r_restore) { r_pos = r_check_pos; r_wait = 0; r_fire_t = 0; r_restore = false; }
    if (lg.check.x != r_cx || lg.check.y != r_cy || lg.check.n != r_cn) {
        r_cx = lg.check.x; r_cy = lg.check.y; r_cn = lg.check.n;
        r_check_pos = r_pos;
    }
    r_level_t++;
    if (r_wait > 0) { r_wait--; return fire_if_foe(); }
    if (r_fire_t > 0) { r_fire_t--; return r_fire_t > 2 ? BTN_A : 0; }
    const char *r = route_of();
    char tok[16];
    int np = next_token(r, r_pos, tok);
    if (!tok[0]) {
        /* plan done: head for the pod */
        Spot s = {lg.exit_x, lg.exit_y, -1};
        bool arrived;
        return go(goal_spot, &s, &arrived) | fire_if_foe();
    }
    /* the honey-only parts */
    if (tok[0] == '(') {
        if (!lmp_bot_honey) {
            int depth = 0, p = r_pos;
            do {
                p = next_token(r, p, tok);
                if (tok[0] == '(') depth++;
                if (tok[0] == ')') depth--;
            } while (depth > 0 && tok[0]);
            r_pos = p;
        } else r_pos = np;
        return 0;
    }
    if (tok[0] == ')') { r_pos = np; return 0; }
    int shoot = fire_if_foe();
    bool arrived = false;
    int b = 0;
    if (isdigit((unsigned char)tok[0]) || strchr("agijklx", tok[0])) {
        int mx, my;
        Spot s = {-1, -1, -1};
        for (int y = 0; y < lg.mh && s.x < 0; y++)
            for (int x = 0; x < lg.mw; x++)
                if (lg.map[y][x] == tok[0]) { s.x = x; s.y = y; break; }
        mx = s.x; my = s.y;
        (void)mx; (void)my;
        s.n = tok[1] == 'U' ? DR_U : tok[1] == 'D' ? DR_D : tok[1] == 'L' ? DR_L : tok[1] == 'R' ? DR_R : -1;
        if (s.x < 0) { r_pos = np; return 0; }
        b = go(goal_spot, &s, &arrived);
        if (arrived) r_pos = np;
        return b | (b ? 0 : shoot);
    }
    switch (tok[0]) {
    case 'F':
        if (t->pn > 0) return 0;
        r_fire_t = 24;
        r_pos = np;
        return 0;
    case 'f':
        /* one shot only (a swap switch flips with every hit) */
        if (t->pn > 0 || t->fire_gap > 0 || (r_prev_mask & BTN_A)) return 0;
        r_pos = np;
        r_wait = 24;
        return BTN_A;
    case 'W':
        if (t->pn > 0 || t->lag > 0 || t->warp_cool > 0 || (r_prev_mask & BTN_B)) return shoot;
        r_pos = np;
        return BTN_B;
    case 'w':
        r_wait = atoi(tok + 1);
        r_pos = np;
        return 0;
    case 'B':
        if (lg.bulb_out) { r_pos = np; return 0; }
        b = go(goal_bulb, NULL, &arrived);
        if (arrived && t->fire_gap == 0) return BTN_A;
        return b | (b ? 0 : shoot);
    case 'E': {
        /* a honey run that lost its jar to a death goes back for it if it can */
        if (lmp_bot_honey && lg.honey_x >= 0 && !lg.honey_held && lg.kind == LK_NORMAL) {
            Spot hs = {lg.honey_x, lg.honey_y, -1};
            int d;
            if (bfs(goal_support, &hs, &d) >= 0) { b = go(goal_support, &hs, &arrived); return b | (b ? 0 : shoot); }
        }
        Spot s = {lg.exit_x, lg.exit_y, -1};
        b = go(goal_spot, &s, &arrived);
        return b | (b ? 0 : shoot);
    }
    case 'H': {
        /* the hidden honey: touch its surface (the Dreadnought's jar: its cell) */
        if (lg.honey_held || lg.honey_x < 0) { r_pos = np; return 0; }
        Spot s = {lg.honey_x, lg.honey_y, -1};
        bool tile = lmp_tile(lg.honey_x, lg.honey_y) == 'H';
        b = go(tile ? goal_support : goal_spot, &s, &arrived);
        if (arrived) r_pos = np;
        return b | (b ? 0 : shoot);
    }
    case 'T': {
        char want = tok[1];
        b = go(goal_hit, &want, &arrived);
        if (arrived && t->fire_gap == 0 && !(r_prev_mask & BTN_A)) { r_pos = np; r_wait = 24; return BTN_A; }
        return b | (b ? 0 : shoot);
    }
    case 'm': {
        /* wait for a moving platform, then warp onto it */
        int ox, oy, on;
        if (t->pn > 0 || t->lag > 0 || t->warp_cool > 0 || (r_prev_mask & BTN_B)) return shoot;
        if (lmp_warp_target(t->x, t->y, t->n, &ox, &oy, &on) >= 0 && lmp_tile(ox - LMP_DX[on], oy - LMP_DY[on]) == 'm') {
            r_pos = np;
            return BTN_B;
        }
        return shoot;
    }
    case 'V': {
        /* shoot the switch worm until the swap blocks change over */
        if (r_v_pos != r_pos) { r_v_pos = r_pos; r_v_par = lg.parity; r_v_t = 0; }
        if (lg.parity != r_v_par) { r_pos = np; r_v_pos = -1; r_wait = 10; return 0; }
        if (++r_v_t > 1200) { r_pos = np; r_v_pos = -1; return 0; }
        if (t->pn > 0 || t->fire_gap > 0 || (r_prev_mask & BTN_A)) return 0;
        for (int i = 0; i < LMP_MAX_FOES; i++) {
            const LmpFoe *f = &lg.foe[i];
            if (!f->alive || f->kind != FO_SWITCHWORM || f->flash > 0) continue;
            int px = t->x * LMP_T + 5, py = t->y * LMP_T + 5;
            int fx = f->x / LMP_FX, fy = f->y / LMP_FX;
            int along = (fx - px) * LMP_DX[t->n] + (fy - py) * LMP_DY[t->n];
            if (along <= 0) continue;
            fx += f_vx[i] * (along / 4) / 16;
            fy += f_vy[i] * (along / 4) / 16;
            int across = LMP_DX[t->n] ? iabs(fy - py) : iabs(fx - px);
            if (across <= 3) return BTN_A;
        }
        return 0;
    }
    case 'K': {
        /* pick a place where a foe will cross the line of fire, go there and
         * wait for it (a while), then pick again */
        if (lg.bulb_out) { r_pos = np; r_hunt_t = 0; return 0; }
        if (shoot && (!threat_near() || survives(BTN_A, 1, 50))) return shoot;
        if (r_hunt_t <= 0 && t->pn == 0) {
            int d;
            predict_foes();
            /* sacs first: they only make more */
            b_hunt_kind = lmp_count_foes(FO_SAC) > 0 ? FO_SAC : 0;
            int found = bfs(goal_intercept, NULL, &d);
            if (found < 0 && b_hunt_kind) { b_hunt_kind = 0; found = bfs(goal_intercept, NULL, &d); }
            b_hunt_kind = 0;
            if (found >= 0 || bfs(goal_hunt, NULL, &d) >= 0) {
                r_hx = b_found_x; r_hy = b_found_y; r_hn = b_found_n;
                r_hunt_t = d * LMP_STEP_T + 50;
            } else return 0;
        }
        r_hunt_t--;
        Spot hs = {r_hx, r_hy, r_hn};
        b = go(goal_spot, &hs, &arrived);
        if (arrived && r_hunt_t > 45) r_hunt_t = 45; /* there: wait a moment for it */
        return b;
    }
    case 'D':
        if (lg.boss_dead) { r_pos = np; return 0; }
        b = dread_plan();
        return b < 0 ? 0 : b;
    default:
        r_pos = np;
        return 0;
    }
}

/* ---- the hub: which pod next ---------------------------------------------------- */

static int hub_plan(void) {
    /* the next deck: lowest number not cleared (or without its honey, on a honey run)
     * that can be reached; the bosses when their pods are open */
    int best = -1, best_d = 1 << 30;
    for (int pass = 0; pass < 2 && best < 0; pass++) {
        int k = 0;
        for (int y = 0; y < lg.mh; y++)
            for (int x = 0; x < lg.mw; x++) {
                if (lg.map[y][x] != '$') continue;
                int idx = k++;
                if (idx >= LMP_HUB_NPODS) continue;
                int lv = LMP_HUB_POD[idx].level;
                bool want;
                if (lv == LV_TURNCOAT) want = !lgs.turncoat;
                else if (lv == LV_DREAD) want = !lgs.dread || (lmp_bot_honey && !((lgs.honey >> 26) & 1));
                else if (lv == LMP_SECTORS - 1) want = !lgs.secret;
                else want = !((lgs.cleared >> lv) & 1) ||
                            (lmp_bot_honey && LMP_LEVEL[lv]->kind == LK_NORMAL && !((lgs.honey >> lv) & 1));
                if (!want) continue;
                /* the Dreadnought last of all on a honey run, once the decks are done */
                int order = lv < LMP_SECTORS ? lv : lv == LV_TURNCOAT ? 12 : 40;
                if (pass == 0 && lv == LV_DREAD && lmp_bot_honey && lmp_cleared_count() < LMP_NORMAL_DECKS) continue;
                Spot s = {x, y, -1};
                int d;
                if (bfs(goal_spot, &s, &d) < 0) continue;
                int score = order * 1000 + d;
                if (score < best_d) { best_d = score; best = idx; }
            }
    }
    if (best < 0) return 0;
    if (lg.pod_here == best && lg.tk.pn == 0) return BTN_B;
    int x, y, k = 0;
    for (y = 0; y < lg.mh; y++)
        for (x = 0; x < lg.mw; x++)
            if (lg.map[y][x] == '$') {
                if (k == best) {
                    Spot s = {x, y, -1};
                    bool arrived;
                    return go(goal_spot, &s, &arrived);
                }
                k++;
            }
    return 0;
}

int lmp_bot_last;

/* whatever the plan wants, it must not walk into death: check it against the
 * game run ahead, and if it dies, find something that doesn't */
static int safety(int m) {
    const LmpTank *t = &lg.tk;
    if (t->pn > 0 || t->hop_t > 0) return m;
    if (!threat_near()) return m;
    int hold = (m & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)) ? LMP_STEP_T : 1;
    int full;
    int mt = survive_t(m, hold, 60, &full);
    if (mt >= full || t->lag > 0) return m;
    int alt[5] = {0, BTN_B, 0, 0, 0};
    int d1 = (t->n + 2) & 7, d2 = (t->n + 6) & 7;
    alt[2] = d1 == DR_U ? BTN_UP : d1 == DR_D ? BTN_DOWN : d1 == DR_L ? BTN_LEFT : BTN_RIGHT;
    alt[3] = d2 == DR_U ? BTN_UP : d2 == DR_D ? BTN_DOWN : d2 == DR_L ? BTN_LEFT : BTN_RIGHT;
    alt[4] = fire_if_foe(); /* shooting what is coming may be the way to live */
    int best = m, best_t = mt;
    for (int k = 0; k < 5 && best_t < full; k++) {
        if (!alt[k] && k == 4) continue;
        if (alt[k] == m) continue;
        if (alt[k] == BTN_B && (t->warp_cool > 0 || (r_prev_mask & BTN_B))) continue;
        int at = survive_t(alt[k], k == 2 || k == 3 ? LMP_STEP_T : 1, 60, &full);
        if (at > best_t) { best_t = at; best = alt[k]; }
    }
    return best;
}

int lmp_bot_buttons(void) {
    int m = 0;
    track_foes();
    switch (lg.state) {
    case GS_TITLE:
        if (lg.talk_t > 0) m = (lg.frame_t & 1) ? (lg.talk_npc == 1 ? BTN_A : BTN_RIGHT) : 0;
        else m = (lg.frame_t & 1) ? BTN_A : 0;
        break;
    case GS_STORY: m = (lg.state_t > 30 && (lg.frame_t & 1)) ? BTN_A : 0; break;
    case GS_PROMPT:
        if (lg.menu != 0) m = (lg.frame_t & 1) ? BTN_UP : 0;
        else m = (lg.frame_t & 1) ? BTN_A : 0;
        r_restore = true; /* the plan goes back to where the checkpoint was taken */
        break;
    case GS_CLEAR: m = (lg.state_t > 50 && (lg.frame_t & 1)) ? BTN_A : 0; break;
    case GS_ENDING: case GS_CHERRY: m = BTN_A; break;
    case GS_PLAY:
        if (lg.level == LV_HUB) { r_level = -1; m = hub_plan(); }
        else {
            int keep_pos = r_pos, keep_wait = r_wait, keep_fire = r_fire_t;
            int want = press(level_plan());
            m = safety(want);
            if (m != want) { r_pos = keep_pos; r_wait = keep_wait; r_fire_t = keep_fire; }
        }
        break;
    default: break;
    }
    m = press(m);
    r_prev_mask = (uint32_t)m;
    lmp_bot_last = m;
    return m;
}

int lmp_bfs_reach(int tx, int ty, int tn) {
    Spot s = {tx, ty, tn};
    int d = -1;
    if (bfs(goal_spot, &s, &d) < 0) return -1;
    return d;
}

int lmp_bot_rpos(void) { return r_pos; }

int lmp_bot_debug(int what) {
    /* tests: 1 = a warp now keeps the tank alive, 2 = so does waiting */
    if (what == 1) return survives(BTN_B, 1, 60);
    if (what == 2) return survives(0, 0, 60);
    if (what == 3) { int f; return survive_t(BTN_RIGHT, LMP_STEP_T + 1, 50, &f) * 1000 + f; }
    if (what == 4) { int f; return survive_t(0, 0, 50, &f) * 1000 + f; }
    if (what >= 100) {
        /* the first move toward a mark (what - 100 is its character) */
        Spot sp = {-1, -1, -1};
        for (int y = 0; y < lg.mh && sp.x < 0; y++)
            for (int x = 0; x < lg.mw; x++)
                if (lg.map[y][x] == what - 100) { sp.x = x; sp.y = y; break; }
        sp.n = DR_D;
        int d;
        return bfs(goal_spot, &sp, &d) * 1000 + d;
    }
    return 0;
}
