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
 *   F                fire a burst      W   warp      wN  wait N frames
 *   Tc               shoot (once) the nearest tile c in a line: Td a door switch,
 *                    TS a swap switch, TO an outline block, TB a breakable
 *   B                shoot the red bulb
 *   E                go into the exit pod
 *   !  H             stand on the ! block, take the honey (honey runs only)
 *   ( ... )          only on honey runs
 *   K                hunt: shoot every foe until the pod opens (survival, Turncoat)
 *   D                fight the Dreadnought */
#include "lmp.h"

int lmp_bot_honey;

#define NSTATE (LMP_MW * LMP_MH * 4)
static int16_t b_prev_x[NSTATE], b_prev_y[NSTATE];
static uint8_t b_prev_n[NSTATE], b_act[NSTATE], b_seen[NSTATE];
static int b_queue[NSTATE];
static int b_stamp;

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
        if (sup == '^') return false;
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
        if (lmp_is_spikes(*ox - LMP_DX[*on], *oy - LMP_DY[*on])) return false;
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

/* breadth-first over the tank's states; returns the first move, ACT_NONE when
 * already there, -1 when there is no way */
static int bfs(Goal goal, const void *arg, int *dist) {
    const LmpTank *t = &lg.tk;
    b_stamp++;
    if (b_stamp >= 250) { memset(b_seen, 0, sizeof b_seen); b_stamp = 1; }
    if (goal(t->x, t->y, t->n, arg)) { if (dist) *dist = 0; return ACT_NONE; }
    int head = 0, tail = 0;
    int s = key(t->x, t->y, t->n);
    b_seen[s] = (uint8_t)b_stamp;
    b_prev_x[s] = -1;
    b_queue[tail++] = s;
    while (head < tail) {
        int k = b_queue[head++];
        int n = (k & 3) * 2, c = k >> 2, x = c % LMP_MW, y = c / LMP_MW;
        for (int a = ACT_CW; a <= ACT_WARP; a++) {
            int ox, oy, on;
            if (!edge(x, y, n, a, &ox, &oy, &on)) continue;
            if (!lmp_axis(on)) continue;
            int k2 = key(ox, oy, on);
            if (b_seen[k2] == b_stamp) continue;
            b_seen[k2] = (uint8_t)b_stamp;
            b_prev_x[k2] = (int16_t)x;
            b_prev_y[k2] = (int16_t)y;
            b_prev_n[k2] = (uint8_t)n;
            b_act[k2] = (uint8_t)a;
            if (goal(ox, oy, on, arg)) {
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
    for (int k = 1; k < 40; k++) {
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

static bool foe_in_line(int x, int y, int n, int range) {
    int px = x * LMP_T + 5, py = y * LMP_T + 5;
    int hx = -1, hy = -1;
    line_hit(x, y, n, &hx, &hy);
    int lim = hx >= 0 ? imax(iabs(hx - x), iabs(hy - y)) * LMP_T : range * LMP_T;
    lim = imin(lim, range * LMP_T);
    for (int i = 0; i < LMP_MAX_FOES; i++) {
        const LmpFoe *f = &lg.foe[i];
        if (!f->alive || f->kind == FO_BALL || f->kind == FO_FLAME || f->kind == FO_SWITCHWORM || f->kind == FO_DREAD) continue;
        int fx = f->x / LMP_FX, fy = f->y / LMP_FX;
        int along = (fx - px) * LMP_DX[n] + (fy - py) * LMP_DY[n];
        int across = LMP_DX[n] ? iabs(fy - py) : iabs(fx - px);
        if (along > 0 && along <= lim && across <= f->h / 2 + 1) return true;
    }
    return false;
}
static bool goal_hunt(int x, int y, int n, const void *arg) {
    (void)arg;
    return foe_in_line(x, y, n, 14);
}

/* ---- looking ahead ----------------------------------------------------------- */

static LmpGame b_save;

static bool threat_near(void) {
    int tx, ty;
    lmp_tank_centre(&tx, &ty);
    for (int i = 0; i < LMP_MAX_FOES; i++) {
        const LmpFoe *f = &lg.foe[i];
        if (!f->alive) continue;
        int r = f->kind == FO_DREAD ? 400 : f->kind == FO_FLAME ? 70 : f->kind == FO_TURRET || f->kind == FO_LASER ? 400 : 90;
        if (iabs(f->x / LMP_FX - tx) < r && iabs(f->y / LMP_FX - ty) < r) return true;
    }
    for (int i = 0; i < LMP_MAX_ESHOTS; i++)
        if (lg.es[i].alive && iabs(lg.es[i].x / LMP_FX - tx) < 120 && iabs(lg.es[i].y / LMP_FX - ty) < 120) return true;
    for (int i = 0; i < lg.nmover; i++)
        if (iabs(lg.mover[i].x - lg.tk.x) < 6 && iabs(lg.mover[i].y - lg.tk.y) < 6) return true;
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

/* run the game ahead: the buttons for `hold` frames, then nothing; true if the tank lives */
static bool survives(int buttons, int hold, int frames) {
    memcpy(&b_save, &lg, sizeof lg);
    bool keep_sim = lmp_simulating;
    lmp_simulating = true;
    lmp_sim_prev = 0;
    bool alive = true;
    for (int f = 0; f < frames; f++) {
        lmp_sim_cur = f < hold ? (uint32_t)buttons : 0;
        lmp_play_step();
        lmp_sim_prev = lmp_sim_cur;
        if (lg.state == GS_DEAD) { alive = false; break; }
        if (lg.state != GS_PLAY) break;
    }
    memcpy(&lg, &b_save, sizeof lg);
    lmp_simulating = keep_sim;
    return alive;
}

/* ---- the plan ------------------------------------------------------------------ */

static int r_level = -1, r_pos, r_wait, r_fire_t, r_level_t, r_depth;
static int r_check_pos, r_cx = -1, r_cy, r_cn;
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
    const LmpTank *t = &lg.tk;
    *arrived = false;
    if (t->pn > 0 || t->lag > 0 || t->hop_t > 0) return 0;
    int act = bfs(goal, arg, NULL);
    if (act == ACT_NONE) { *arrived = true; return 0; }
    bool danger = threat_near();
    if (act < 0) {
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
    if (!danger) return want;
    if (survives(want, act == ACT_WARP ? 1 : LMP_STEP_T, 50)) return want;
    if (survives(0, 0, 50)) return 0;
    static const int ALT[3] = {ACT_WARP, ACT_CW, ACT_CCW};
    for (int k = 0; k < 3; k++) {
        int b = act_buttons(ALT[k]);
        if (b && survives(b, 2, 50)) return b;
    }
    return want;
}

static int fire_if_foe(void) {
    const LmpTank *t = &lg.tk;
    if (t->pn == 0 && lmp_axis(t->n) && foe_in_line(t->x, t->y, t->n, 14) && t->fire_gap == 0) return BTN_A;
    return 0;
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
    if (d->state == 2) want_x = col;
    else want_x = col + (t->x <= col ? -3 : 3);
    want_x = iclamp(want_x, 1, lg.mw - 2);
    int fy = lg.mh - 2;
    while (fy > 0 && !lmp_state_ok(want_x, fy, DR_U)) fy--;
    Spot s = {want_x, fy, DR_U};
    bool arrived;
    int b = go(goal_spot, &s, &arrived);
    if (t->pn == 0 && t->n == DR_U && t->x == col && d->state == 2 && t->fire_gap == 0) {
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
    if (isdigit((unsigned char)tok[0])) {
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
    case 'W':
        if (t->pn > 0 || t->lag > 0 || t->warp_cool > 0) return shoot;
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
        Spot s = {lg.exit_x, lg.exit_y, -1};
        b = go(goal_spot, &s, &arrived);
        return b | (b ? 0 : shoot);
    }
    case '!': {
        if (lg.honey_shown) { r_pos = np; return 0; }
        Spot s = {-1, -1, -1};
        for (int y = 0; y < lg.mh && s.x < 0; y++)
            for (int x = 0; x < lg.mw; x++)
                if (lg.map[y][x] == '!') { s.x = x; s.y = y; break; }
        b = go(goal_support, &s, &arrived);
        if (arrived) r_pos = np;
        return b | (b ? 0 : shoot);
    }
    case 'H': {
        if (lg.honey_held || lg.honey_x < 0) { r_pos = np; return 0; }
        Spot s = {lg.honey_x, lg.honey_y, -1};
        b = go(goal_spot, &s, &arrived);
        if (arrived) r_pos = np;
        return b | (b ? 0 : shoot);
    }
    case 'T': {
        char want = tok[1];
        b = go(goal_hit, &want, &arrived);
        if (arrived && t->fire_gap == 0) { r_pos = np; r_wait = 24; return BTN_A; }
        return b | (b ? 0 : shoot);
    }
    case 'K':
        if (lg.bulb_out) { r_pos = np; return 0; }
        if (shoot) return shoot;
        b = go(goal_hunt, NULL, &arrived);
        return b;
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

int lmp_bot_buttons(void) {
    int m = 0;
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
        else m = level_plan();
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
