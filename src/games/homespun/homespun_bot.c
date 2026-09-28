/* HOMESPUN - the demo player. It plays from the title to the ending with
 * the buttons alone: it walks camp to each building, opens its menu with A,
 * steps to the line it wants and presses A; it farms the Glowstone at the
 * start, waits for jerky, walks out of the gate, finds its way across the
 * Wilds room by room, breaks cracked walls, fights with the yo-yo (keeping
 * its distance and lining up first), picks up what drops, talks to Hush and
 * Tanger, beats the guardians, Mother Loom and Mawbo, and brings everything
 * home. It knows the map (it reads the world), not the future. The tests
 * use it to show that every generated world can be finished. */
#include "homespun.h"

int hs_bot_goal;
int hs_bot_idle; /* the last frame's choice was to wait at camp */
bool hs_bot_cherry = true;
bool hs_bot_arena;

enum { G_NONE, G_GEAR, G_LETTER, G_PIPE, G_DUN1, G_DUN2, G_DUN3, G_LOOM, G_MAWBO, G_HOME, G_ODDS, G_TONIC };

static int bf;               /* the bot's own frame count */
static int stuck_t, stuck_x, stuck_y, jiggle_t, jiggle_b;
static int menu_wait, swap_phase;
static uint32_t tonic_trip = 0xFFFFFFFFu; /* the trip on which tonics were bought */
static int last_room = -1;

void hs_bot_reset(void) {
    hs_bot_goal = G_NONE;
    bf = 0;
    stuck_t = jiggle_t = 0;
    menu_wait = 0;
    last_room = -1;
}

static int wx(void) { return hs.px / HS_U; }
static int wy(void) { return hs.py / HS_U; }
static int tap(int b) { return (bf & 1) ? b : 0; }

static int dir_bits(int dx, int dy) {
    int b = 0;
    if (dx > 0) b |= BTN_RIGHT;
    if (dx < 0) b |= BTN_LEFT;
    if (dy > 0) b |= BTN_DOWN;
    if (dy < 0) b |= BTN_UP;
    return b;
}

/* ---- walking inside a room: breadth-first over 8-pixel cells ----------------------------- */

#define CW 8
#define GW (HS_FW / CW)
#define GH (HS_FH / CW)
static uint8_t pass[GH][GW];
static int pass_room = -1, pass_t;

static void build_pass(void) {
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) pass[y][x] = hs_box_free(x * CW, y * CW, 5);
    pass_room = hs.room;
    pass_t = bf;
}

/* The next step toward any cell within tol pixels of (tx, ty): writes the
 * point to steer at; false if there is no way. */
static bool step_to(int tx, int ty, int tol, int *sx, int *sy) {
    if (pass_room != hs.room || bf - pass_t > 30) build_pass();
    static int16_t prev[GH * GW];
    static int16_t q[GH * GW];
    int start_x = iclamp((wx() + CW / 2) / CW, 0, GW - 1), start_y = iclamp((wy() + CW / 2) / CW, 0, GH - 1);
    int s = start_y * GW + start_x;
    for (int i = 0; i < GH * GW; i++) prev[i] = -2;
    int qh = 0, qt = 0, goal = -1;
    prev[s] = -1;
    q[qt++] = (int16_t)s;
    while (qh < qt) {
        int c = q[qh++], cx = c % GW, cy = c / GW;
        int px = cx * CW, py = cy * CW;
        if (iabs(px - tx) <= tol && iabs(py - ty) <= tol) { goal = c; break; }
        static const int D8[8][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
        for (int d = 0; d < 8; d++) {
            int nx = cx + D8[d][0], ny = cy + D8[d][1];
            if (nx < 0 || ny < 0 || nx >= GW || ny >= GH) continue;
            int n = ny * GW + nx;
            if (prev[n] != -2 || !pass[ny][nx]) continue;
            if (d >= 4 && (!pass[cy][nx] || !pass[ny][cx])) continue; /* no cutting corners */
            prev[n] = (int16_t)c;
            q[qt++] = (int16_t)n;
        }
    }
    if (goal < 0) return false;
    if (goal == s) { *sx = tx; *sy = ty; return true; }
    int c = goal;
    /* look two steps ahead for smoother walking */
    int path[GH * GW], n = 0;
    while (c != s && c >= 0 && n < GH * GW) { path[n++] = c; c = prev[c]; }
    int k = n - 1;
    int pick = path[k];
    *sx = (pick % GW) * CW;
    *sy = (pick / GW) * CW;
    if (n == 1) { *sx = tx; *sy = ty; }
    return true;
}

static int steer(int sx, int sy);

/* nothing solid on the straight line between two points */
static bool clear_line(int x0, int y0, int x1, int y1) {
    int n = imax(iabs(x1 - x0), iabs(y1 - y0)) / 4;
    for (int i = 1; i < n; i++)
        if (hs_solid_px(x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n)) return false;
    return true;
}

/* Run from a foe at (mx, my): the reachable spot within a few steps that
 * is farthest from it (a corner is a trap: open ground scores higher). */
static int flee_from(int mx, int my) {
    if (pass_room != hs.room || bf - pass_t > 30) build_pass();
    static int16_t prev[GH * GW];
    static int8_t depth[GH * GW];
    static int16_t q[GH * GW];
    int start_x = iclamp((wx() + CW / 2) / CW, 0, GW - 1), start_y = iclamp((wy() + CW / 2) / CW, 0, GH - 1);
    int s = start_y * GW + start_x;
    for (int i = 0; i < GH * GW; i++) prev[i] = -2;
    int qh = 0, qt = 0, best = s, bscore = -99999;
    prev[s] = -1;
    depth[s] = 0;
    q[qt++] = (int16_t)s;
    while (qh < qt) {
        int c = q[qh++], cx = c % GW, cy = c / GW;
        int px = cx * CW, py = cy * CW;
        int free = 0;
        for (int d = 0; d < 4; d++) {
            int nx = cx + HS_DX[d], ny = cy + HS_DY[d];
            free += nx >= 0 && ny >= 0 && nx < GW && ny < GH && pass[ny][nx];
        }
        int score = iabs(px - mx) + iabs(py - my) + free * 6 - depth[c] * 2;
        if (score > bscore && c != s) { bscore = score; best = c; }
        if (depth[c] >= 8) continue;
        for (int d = 0; d < 4; d++) {
            int nx = cx + HS_DX[d], ny = cy + HS_DY[d];
            if (nx < 0 || ny < 0 || nx >= GW || ny >= GH) continue;
            int n = ny * GW + nx;
            if (prev[n] != -2 || !pass[ny][nx]) continue;
            prev[n] = (int16_t)c;
            depth[n] = (int8_t)(depth[c] + 1);
            q[qt++] = (int16_t)n;
        }
    }
    int c = best;
    while (prev[c] != s && prev[c] >= 0) c = prev[c];
    return steer((c % GW) * CW, (c / GW) * CW);
}

static int steer(int sx, int sy) {
    int dx = sx - wx(), dy = sy - wy();
    return dir_bits(iabs(dx) > 1 ? dx : 0, iabs(dy) > 1 ? dy : 0);
}

/* walk toward a point; returns buttons, or -1 when there is no way */
static int walk_to(int tx, int ty, int tol) {
    int sx, sy;
    /* close by: head straight there */
    if (iabs(tx - wx()) <= 8 && iabs(ty - wy()) <= 8 && hs_box_free(tx, ty, 5)) return steer(tx, ty);
    if (!step_to(tx, ty, imax(tol, 5), &sx, &sy)) return -1;
    return steer(sx, sy);
}

/* ---- the rooms of Oddmoor as a graph --------------------------------------------------------- */

static bool solid_t(int t) { return t == T_WALL || t == T_WATER || t == T_LAVA || t == T_BLOCK || t == T_SHUT; }

/* a dungeon edge from r toward d: its gap exists, and the far side is not a shut crack */
static bool dun_edge_ok(int r, int d, int n) {
    uint8_t a[HS_TH][HS_TW], b[HS_TH][HS_TW];
    hs_room_tiles(r, a, NULL);
    hs_room_tiles(n, b, NULL);
    for (int k = 0; k < HS_TW; k++) {
        int ax, ay, bx, by;
        if (d == D_UP || d == D_DOWN) {
            if (k >= HS_TW) break;
            ax = bx = k;
            ay = d == D_UP ? 0 : HS_TH - 1;
            by = d == D_UP ? HS_TH - 1 : 0;
        } else {
            if (k >= HS_TH) break;
            ay = by = k;
            ax = d == D_LEFT ? 0 : HS_TW - 1;
            bx = d == D_LEFT ? HS_TW - 1 : 0;
        }
        int ta = a[ay][ax], tb = b[by][bx];
        if (ta == T_WALL) continue;
        if (tb == T_CRACK || solid_t(tb)) continue;
        return true; /* an open gap (a crack on this side is broken on the way) */
    }
    return false;
}

static int neighbours(int r, int out[6], int how[6]) {
    int n = 0;
    int a = hs_room_area(r);
    if (a == AR_BASE) { out[n] = HS_START_SCREEN; how[n++] = D_UP; return n; }
    if (a == AR_OVER) {
        uint8_t e = hs_exits(r);
        for (int d = 0; d < 4; d++) {
            if (!(e >> d & 1)) continue;
            int nb = hs_neighbour(r, d);
            if (nb < 0) continue;
            if (r == hw.ruins && nb == hw.lair && !sv.volt_down) continue;
            if (r == hw.lair && nb == hw.ruins && !sv.volt_down) continue;
            out[n] = nb;
            how[n++] = d;
        }
        if (r == HS_START_SCREEN) { out[n] = HS_R_BASE; how[n++] = D_DOWN; }
        if (hw.cave_of[r] >= 0) {
            int c = hw.cave_of[r], k = hw.cave_kind[c];
            out[n] = k <= K_DUN3 ? HS_R_DUN0 + k * HS_DROOMS : HS_R_CAVE0 + c;
            how[n++] = 4; /* into the cave mouth */
        }
        return n;
    }
    if (a == AR_CAVE) { out[n] = hw.cave_screen[r - HS_R_CAVE0]; how[n++] = D_DOWN; return n; }
    int d0 = hs_dun_of(r);
    if ((r - HS_R_DUN0) % HS_DROOMS == 0) { out[n] = hw.cave_screen[hs_dun_cave(d0)]; how[n++] = D_DOWN; }
    /* the dungeon's doors only change when a crack breaks: keep them until then */
    static int8_t dok[HS_ROOMS][4];
    static uint64_t dsig = ~0ull;
    static uint32_t dseed;
    uint64_t sig = 0;
    for (int k = HS_R_DUN0; k < HS_R_BASE; k++) sig = sig * 3 + ((sv.opened[k] >> 15) & 1);
    if (sig != dsig || dseed != hw.seed) {
        for (int k = HS_R_DUN0; k < HS_R_BASE; k++)
            for (int d = 0; d < 4; d++) {
                int nb = hs_dun_neighbour(k, d);
                dok[k][d] = (int8_t)(nb >= 0 && dun_edge_ok(k, d, nb));
            }
        dsig = sig;
        dseed = hw.seed;
    }
    for (int d = 0; d < 4; d++) {
        int nb = hs_dun_neighbour(r, d);
        if (nb >= 0 && dok[r][d]) { out[n] = nb; how[n++] = d; }
    }
    return n;
}

/* the first move from `from` toward `to` (its direction, 4 = a cave mouth), and the distance */
static int route(int from, int to, int *dist) {
    static int16_t prev[HS_ROOMS];
    static int8_t via[HS_ROOMS];
    int q[HS_ROOMS], qh = 0, qt = 0;
    for (int i = 0; i < HS_ROOMS; i++) prev[i] = -2;
    prev[from] = -1;
    q[qt++] = from;
    while (qh < qt) {
        int r = q[qh++];
        if (r == to) break;
        int nb[6], how[6];
        int n = neighbours(r, nb, how);
        for (int i = 0; i < n; i++) {
            if (prev[nb[i]] != -2) continue;
            prev[nb[i]] = (int16_t)r;
            via[nb[i]] = (int8_t)how[i];
            q[qt++] = nb[i];
        }
    }
    if (prev[to] == -2) { *dist = 999; return -1; }
    int r = to, steps = 0;
    while (prev[r] != from && prev[r] >= 0) { r = prev[r]; steps++; }
    *dist = steps + (to != from);
    return to == from ? -1 : via[r];
}

/* ---- goals ---------------------------------------------------------------------------------- */

static int cave_room(int kind) {
    for (int c = 0; c < HS_CAVES; c++)
        if (hw.cave_kind[c] == kind) return HS_R_CAVE0 + c;
    return -1;
}

static int boss_room(int d) {
    for (int i = 0; i < HS_DROOMS; i++) {
        int r = HS_R_DUN0 + d * HS_DROOMS + i;
        if (hs_dun_boss_room(r)) return r;
    }
    return -1;
}

/* strips for a trip there and back plus a fight, by rooms from camp */
static int trip_jerky(int dist) { return (dist * 70 + 360) / 120 + 1; }

static int dist_from_camp(int room) {
    int d = 0;
    route(HS_R_BASE, room, &d);
    return d;
}

/* the guardian still to beat whose hall is nearest camp */
static int next_dungeon(void) {
    int best = -1, bd = 9999;
    for (int d = 0; d < HS_DUNS; d++) {
        if ((sv.parts | sv.carry) >> d & 1) continue;
        int dd = dist_from_camp(boss_room(d));
        if (dd < bd) { bd = dd; best = d; }
    }
    return best;
}

static int want_jerky(int g) {
    int cap = hs_cap(RES_JERKY);
    switch (g) {
    case G_MAWBO: case G_LOOM: return cap;
    case G_DUN1: case G_DUN2: case G_DUN3:
        return imin(cap, imax(5, trip_jerky(dist_from_camp(boss_room(g - G_DUN1)))));
    default: return imin(cap, 5);
    }
}

/* both of Kit's chests opened on this trip */
static bool kit_emptied(void) {
    if (!sv.on_trip) return false;
    int r = cave_room(K_SIS);
    HsThingSpawn ts[HS_MAX_THINGS];
    int n = hs_room_things(r, ts, HS_MAX_THINGS), left = 0;
    for (int i = 0; i < n && i < 15; i++)
        if (ts[i].kind == TH_CHEST && !(sv.opened[r] >> i & 1)) left++;
    return left == 0;
}

/* the room and tile of the noodler selling ware v (-1: none this save) */
static int vendor_room(int v, int *tx, int *ty) {
    for (int r = HS_R_DUN0; r < HS_R_BASE; r++) {
        HsThingSpawn ts[HS_MAX_THINGS];
        int n = hs_room_things(r, ts, HS_MAX_THINGS);
        for (int i = 0; i < n; i++)
            if (ts[i].kind == TH_VENDOR && ts[i].arg == v) { *tx = ts[i].tx; *ty = ts[i].ty; return r; }
    }
    return -1;
}

/* the next thing worth a trip (G_NONE: nothing) */
static int wild_goal(void) {
    if (!sv.lab_fixed && !(sv.carry & 8)) return G_GEAR;
    if (!sv.pipe && !sv.letter) return G_LETTER;
    if (!sv.pipe && sv.letter) return G_PIPE;
    int nd = next_dungeon();
    if (nd >= 0) return G_DUN1 + nd;
    if (!sv.loom_home) return G_LOOM;
    if (hs_bot_cherry && sv.parts == 7 && !sv.idol && !(sv.carry & 16)) {
        /* the players' way: hoard odds first (Kit's chests fill up every
         * trip), then buy tonics on the way to Mawbo */
        if (sv.odds < 12 && !(sv.on_trip && tonic_trip == sv.excursion)) return G_ODDS;
        return G_MAWBO;
    }
    return G_NONE;
}

/* goals that need a stronger camp first */
static bool ready_for(int g) {
    if (g >= G_DUN1 && g <= G_DUN3) {
        /* a camp at work first, the steel yo-yo after the first guardian,
         * and enough jerky for the way there and back */
        int k = hs_parts_count();
        int need = trip_jerky(dist_from_camp(boss_room(g - G_DUN1)));
        return hs_hands() >= 2 + 2 * k && (k == 0 || (sv.research >> RS_METAL & 1)) && hs_cap(RES_JERKY) >= need - 1;
    }
    if (g == G_LOOM) return (sv.research >> RS_METAL & 1) && hs_cap(RES_JERKY) >= 10;
    if (g == G_MAWBO) return (sv.research >> RS_STAR & 1) && hs_cap(RES_JERKY) >= 20;
    return true;
}

/* ---- fighting -------------------------------------------------------------------------------- */

static bool hittable(const HsMob *m) {
    if (!m->on) return false;
    if ((m->kind == E_GRUBLET || m->kind == E_SPLOSH) && !m->up) return false;
    return true;
}

static int nearest_mob(int range) {
    int best = -1, bd = range;
    for (int i = 0; i < HS_MAX_MOBS; i++) {
        const HsMob *m = &hs.mob[i];
        if (!hittable(m)) continue;
        int d = iabs(m->x / HS_U - wx()) + iabs(m->y / HS_U - wy());
        if (HS_FOE[m->kind].tier == 3) d -= 30; /* bosses first */
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

/* a hostile shot within `range` whose line passes close to her */
static bool shot_coming(int range) {
    for (int i = 0; i < HS_MAX_SHOTS; i++) {
        const HsShot *s = &hs.shot[i];
        if (!s->on || s->mine) continue;
        int dx = wx() - s->x / HS_U, dy = wy() - s->y / HS_U;
        if (s->kind == SH_BOMB || s->kind == SH_BLAST) {
            if (iabs(dx) < 30 && iabs(dy) < 30) return true;
            continue;
        }
        if (iabs(dx) > range || iabs(dy) > range) continue;
        if ((int64_t)s->vx * dx + (int64_t)s->vy * dy <= 0) continue;
        float v = sqrtf((float)s->vx * (float)s->vx + (float)s->vy * (float)s->vy);
        if (v < 1) continue;
        if (fabsf(((float)dx * (float)s->vy - (float)dy * (float)s->vx) / v) <= 11) return true;
    }
    return false;
}

/* where each foe was last frame: the demo player leads a moving target */
static int32_t seen_x[HS_MAX_MOBS], seen_y[HS_MAX_MOBS];
static int seen_room = -1;

static void note_foes(void) {
    for (int i = 0; i < HS_MAX_MOBS; i++) {
        seen_x[i] = hs.mob[i].x;
        seen_y[i] = hs.mob[i].y;
    }
    seen_room = hs.room;
}

static int fight(const HsMob *m) {
    int mx = m->x / HS_U, my = m->y / HS_U;
    int slot = (int)(m - hs.mob);
    if (seen_room == hs.room && slot >= 0 && slot < HS_MAX_MOBS && !m->stun) {
        /* aim where it will be when the yo-yo gets there */
        int lead = 8;
        mx += (int)((m->x - seen_x[slot]) * lead / HS_U);
        my += (int)((m->y - seen_y[slot]) * lead / HS_U);
    }
    int dx = mx - wx(), dy = my - wy();
    const HsFoeDef *fd = &HS_FOE[m->kind];
    int size = fd->size;
    bool pipe = sv.pipe && sv.weapon == 1;
    /* touching range; how far the foe comes while one attack plays out; the
     * nearest and farthest an attack should start from */
    int close = size / 2 + (fd->tier == 3 ? 12 : 7); /* boxes touch sooner on a diagonal */
    int step = fd->speed * (pipe ? PIPE_COOL : 2 * YOYO_OUT) / 16;
    int reach = pipe ? 110 : YOYO_REACH + size / 2 - 2;
    int lo = imin(close + step + 4, reach - 4);
    /* something as quick as she is can't be outrun: stand and swing at it */
    bool quick = fd->tier < 3 && (m->kind == E_GNATTER || m->kind == E_SPINNER || m->kind == E_SKITTER ||
                                  m->kind == E_LEECHLING || m->kind == E_DRIPLET);
    if (quick) lo = close;
    float d = sqrtf((float)(dx * dx + dy * dy));
    /* the best of eight directions, and how far off its line the foe is */
    float ang = atan2f((float)dy, (float)dx);
    int oct = (int)lroundf(ang / 0.785398f);
    int fx = (int)lroundf(cosf((float)oct * 0.785398f)), fy = (int)lroundf(sinf((float)oct * 0.785398f));
    float along = (float)dx * (float)fx + (float)dy * (float)fy;
    if (fx && fy) along *= 0.7071f;
    float off = fabsf((float)dx * (float)fy - (float)dy * (float)fx);
    if (fx && fy) off *= 0.7071f;
    /* a boss leaping or diving at her: get off its line */
    /* a boss about to leap or dive (it shakes): no throw that would still be out when it comes */
    bool busy_soon = false;
    if (fd->tier == 3) {
        HsMob c = *m;
        for (int k = 0; k <= 24 && !busy_soon; k += 4) { c.t = (int16_t)(m->t + k); busy_soon = hs_mob_tell(&c); }
    }
    bool leaping = m->kind == E_GRUMMKING || (m->kind == E_MAWBO && (m->t / 360) % 5 == 0);
    if (fd->tier == 3 && m->st == 1 && d < 140) {
        if (leaping) /* get clear of where it lands */
            return flee_from((m->x + m->tx * m->cool2) / HS_U, (m->y + m->ty * m->cool2) / HS_U);
        /* a dive is a straight line: step off it */
        int px = m->vy > 0 ? 1 : m->vy < 0 ? -1 : 0, py = m->vx > 0 ? -1 : m->vx < 0 ? 1 : 0;
        if (iabs(m->vx) > 2 * iabs(m->vy)) px = 0;
        if (iabs(m->vy) > 2 * iabs(m->vx)) py = 0;
        if (hs_box_free(wx() + px * 8, wy() + py * 8, 5)) return dir_bits(px, py);
        if (hs_box_free(wx() - px * 8, wy() - py * 8, 5)) return dir_bits(-px, -py);
        return flee_from(mx, my);
    }
    if (busy_soon && d < 150) {
        /* it is about to come at her: keep moving across its line, so it aims at
         * where she was */
        int px = -isign(dy), py = isign(dx);
        if ((bf / 90) % 2) { px = -px; py = -py; }
        if (hs_box_free(wx() + px * 8, wy() + py * 8, 5)) return dir_bits(px, py);
        if (hs_box_free(wx() - px * 8, wy() - py * 8, 5)) return dir_bits(-px, -py);
        return flee_from(mx, my);
    }
    if (!quick && d < (float)(pipe && fd->speed > 0 ? lo + 8 : fd->speed > 0 ? imax(close + 5, lo - 8) : close + 5)) {
        /* too close: get away to open ground */
        return flee_from(mx, my);
    }
    if (along >= (float)(lo - 3) && along <= (float)reach && off <= (float)size / 2 + 1 && clear_line(wx(), wy(), mx, my) &&
        (pipe || !shot_coming(40))) {
        int b = dir_bits(fx, fy);
        if (hs.yo_t == 0 && hs.shoot_cool == 0) b |= tap(BTN_B);
        return b;
    }
    /* in range but off the line: sidestep onto the nearest of the eight lines */
    if (d >= (float)(lo - 3) && d <= (float)reach + 6) {
        float cr = (float)dx * (float)fy - (float)dy * (float)fx; /* which side of the line she is on */
        int px = cr > 0 ? fy : -fy, py = cr > 0 ? -fx : fx;       /* perpendicular, toward the line */
        if ((px || py) && hs_box_free(wx() + px * 6, wy() + py * 6, 5)) { return dir_bits(px, py); }
    }
    /* too far, or the sidestep is walled: close in (or find another line) */
    if (d > (float)reach) {
        int b = walk_to(mx, my, imax(8, reach - 10));
        if (b >= 0) { return b; }
    }
    int want = (lo + reach) / 2;
    static const int ORDER[8] = {0, 1, -1, 2, -2, 3, -3, 4};
    for (int k = 0; k < 8; k++) {
        int o = oct + ORDER[k];
        int ox = (int)lroundf(cosf((float)o * 0.785398f)), oy = (int)lroundf(sinf((float)o * 0.785398f));
        int w = (ox && oy) ? want * 7 / 10 : want;
        int tx = mx - ox * w, ty = my - oy * w;
        if (tx < 12 || ty < 12 || tx > HS_FW - 12 || ty > HS_FH - 12 || !hs_box_free(tx, ty, 5)) continue;
        if (!clear_line(tx, ty, mx, my)) continue;
        int b = walk_to(tx, ty, 6);
        if (b >= 0) return b;
    }
    return flee_from(mx, my);
}

/* ---- the Wilds ---------------------------------------------------------------------------------- */

static int thing_of(int kind, int arg) {
    for (int i = 0; i < HS_MAX_THINGS; i++)
        if (hs.thing[i].on && hs.thing[i].kind == kind && (arg < 0 || hs.thing[i].arg == arg)) return i;
    return -1;
}

/* walk up to a thing and press A at it */
static int use_thing(int i) {
    const HsThing *t = &hs.thing[i];
    int cx = t->tx * HS_T + 8, cy = t->ty * HS_T + 8;
    static const int SIDE[4][2] = {{0, 1}, {0, -1}, {-1, 0}, {1, 0}};
    for (int k = 0; k < 4; k++) {
        int px = cx + SIDE[k][0] * 16, py = cy + SIDE[k][1] * 16;
        if (!hs_box_free(px, py, 5)) continue;
        if (iabs(wx() - px) <= 2 && iabs(wy() - py) <= 2) {
            int fx = -SIDE[k][0], fy = -SIDE[k][1];
            if (hs.dirx != fx || hs.diry != fy) return dir_bits(fx, fy);
            return tap(BTN_A);
        }
        int b = walk_to(px, py, 2);
        if (b >= 0) return b ? b : dir_bits(px - wx(), py - wy());
    }
    return 0;
}

static int leave_by(int how) {
    int x = wx(), y = wy();
    if (how == 4) {
        int cx = hw.cave_x[hs.room] * HS_T + 8;
        if (iabs(x - cx) <= 2 && y < 26) return BTN_UP;
        int b = walk_to(cx, 22, 2);
        return b < 0 ? BTN_UP : (b ? b : BTN_UP);
    }
    /* find the gap on that side: the cells next to the edge that are free */
    int best = -1, bd = 9999, bx = 0, by = 0;
    for (int k = 0; k < (how == D_UP || how == D_DOWN ? GW : GH); k++) {
        int px, py;
        if (how == D_UP || how == D_DOWN) { px = k * CW; py = how == D_UP ? 6 : HS_FH - 7; }
        else { py = k * CW; px = how == D_LEFT ? 6 : HS_FW - 7; }
        if (!hs_box_free(px, py, 5)) continue;
        /* the edge tile itself must be a way out, not a cave mouth */
        int ex = how == D_LEFT ? 0 : how == D_RIGHT ? HS_TW - 1 : iclamp(px / HS_T, 0, HS_TW - 1);
        int ey = how == D_UP ? 0 : how == D_DOWN ? HS_TH - 1 : iclamp(py / HS_T, 0, HS_TH - 1);
        int et = hs.tile[ey][ex];
        if (et == T_CAVE || et == T_WALL) continue;
        if ((px - 5) / HS_T != (px + 4) / HS_T && (how == D_UP || how == D_DOWN)) {
            int et2 = hs.tile[ey][iclamp((px + 4) / HS_T, 0, HS_TW - 1)], et3 = hs.tile[ey][iclamp((px - 5) / HS_T, 0, HS_TW - 1)];
            if (et2 == T_CAVE || et2 == T_WALL || et3 == T_CAVE || et3 == T_WALL) continue;
        }
        int d = iabs(px - x) + iabs(py - y);
        if (d < bd) { bd = d; best = k; bx = px; by = py; }
    }
    if (best < 0) {
        /* a cracked wall on this side: stand before it and throw */
        int gx = how == D_LEFT ? 32 : how == D_RIGHT ? HS_FW - 33 : HS_FW / 2;
        int gy = how == D_UP ? 36 : how == D_DOWN ? HS_FH - 37 : HS_FH / 2;
        if (iabs(x - gx) > 3 || iabs(y - gy) > 3) {
            int b = walk_to(gx, gy, 3);
            return b < 0 ? 0 : b;
        }
        int fx = HS_DX[how], fy = HS_DY[how];
        int b = dir_bits(fx, fy);
        if (hs.yo_t == 0) b |= tap(BTN_B);
        return b;
    }
    if (iabs(x - bx) <= 3 && iabs(y - by) <= 3) return dir_bits(HS_DX[how], HS_DY[how]);
    int b = walk_to(bx, by, 3);
    if (b < 0) return dir_bits(HS_DX[how], HS_DY[how]);
    return b ? b : dir_bits(HS_DX[how], HS_DY[how]);
}

static int pick_items(void) {
    int best = -1, bd = 90;
    for (int i = 0; i < HS_MAX_ITEMS; i++) {
        const HsItem *it = &hs.item[i];
        if (!it->on || !hs_box_free(it->x / HS_U, it->y / HS_U, 3)) continue;
        int d = iabs(it->x / HS_U - wx()) + iabs(it->y / HS_U - wy());
        if (it->kind == IT_IDOL || it->kind == IT_BIGTONIC) d -= 40;
        if (d < bd) { bd = d; best = i; }
    }
    if (best < 0) return -1;
    int b = walk_to(hs.item[best].x / HS_U, hs.item[best].y / HS_U, 5);
    if (b <= 0) b = dir_bits(hs.item[best].x / HS_U - wx(), hs.item[best].y / HS_U - wy());
    return b;
}

static int goal_room(int g) {
    switch (g) {
    case G_GEAR: return hw.gear_screen;
    case G_LETTER: return cave_room(K_HUSH);
    case G_PIPE: return cave_room(K_TANGER);
    case G_DUN1: case G_DUN2: case G_DUN3: return boss_room(g - G_DUN1);
    case G_LOOM: return sv.volt_down ? hw.lair : hw.ruins;
    case G_MAWBO: return sv.mawbo_room;
    case G_ODDS: return cave_room(K_SIS);
    case G_TONIC: { int tx, ty; return vendor_room(V_TONIC2, &tx, &ty); }
    default: return HS_R_BASE;
    }
}

/* a shot about to hit: step off its line, to whichever side is free */
static int dodge(void) {
    for (int i = 0; i < HS_MAX_SHOTS; i++) {
        const HsShot *s = &hs.shot[i];
        if (!s->on || s->mine || s->kind == SH_BOMB || s->kind == SH_BLAST) continue;
        int sx = s->x / HS_U, sy = s->y / HS_U, dx = wx() - sx, dy = wy() - sy;
        if (iabs(dx) > 36 || iabs(dy) > 36) continue;
        int32_t vx = s->vx, vy = s->vy;
        if ((int64_t)vx * dx + (int64_t)vy * dy <= 0) continue; /* going away */
        /* how close its line passes */
        float v = sqrtf((float)vx * (float)vx + (float)vy * (float)vy);
        if (v < 1) continue;
        float miss = fabsf(((float)dx * (float)vy - (float)dy * (float)vx) / v);
        if (miss > 9) continue;
        int px = vy > 0 ? 1 : -1, py = vx > 0 ? -1 : 1; /* perpendicular */
        if (iabs(vx) < iabs(vy) / 3) py = 0;
        if (iabs(vy) < iabs(vx) / 3) px = 0;
        if (!px && !py) px = 1;
        if (hs_box_free(wx() + px * 8, wy() + py * 8, 5)) return dir_bits(px, py);
        if (hs_box_free(wx() - px * 8, wy() - py * 8, 5)) return dir_bits(-px, -py);
    }
    for (int i = 0; i < HS_MAX_SHOTS; i++) {
        /* and keep clear of a bomb about to go off */
        const HsShot *s = &hs.shot[i];
        if (!s->on || (s->kind != SH_BOMB && s->kind != SH_BLAST)) continue;
        int dx = wx() - s->x / HS_U, dy = wy() - s->y / HS_U;
        if (iabs(dx) < 26 && iabs(dy) < 26) return flee_from(s->x / HS_U, s->y / HS_U);
    }
    return 0;
}

static int wilds(void) {
    int g = wild_goal();
    int dist_home = 0;
    route(hs.room, HS_R_BASE, &dist_home);
    int secs = sv.time_f / 60;
    bool carrying = (sv.carry & 31) != 0;
    int need = dist_home * 18 + 40;
    if (g == G_ODDS && kit_emptied()) g = G_HOME;
    if (g == G_MAWBO && tonic_trip != sv.excursion) {
        int tx, ty;
        if (sv.odds >= 2 && vendor_room(V_TONIC2, &tx, &ty) >= 0) g = G_TONIC;
        else tonic_trip = sv.excursion;
    }
    if (g == G_NONE || carrying || secs < need || !ready_for(g)) g = G_HOME;
    hs_bot_goal = g;
    /* sidestep what is flying at her (not while a throw is out: she can't move then) */
    if (hs.yo_t == 0) {
        int dg = dodge();
        if (dg) return dg;
    }
    /* a boss in the room is the fight; otherwise only what comes close
     * (in the arena of the tests, whatever is in the room) */
    int m = hs.boss >= 0 && hs.mob[hs.boss].on && (g != G_HOME || hs_bot_arena) ? hs.boss : nearest_mob(hs_bot_arena ? 999 : 46);
    if (m == hs.boss && m >= 0) {
        /* clear the small fry near her first: they are quick to beat and slow to shake off */
        int best = -1, bd = iabs(hs.mob[m].x / HS_U - wx()) + iabs(hs.mob[m].y / HS_U - wy()) > 90 ? 72 : 48;
        for (int i = 0; i < HS_MAX_MOBS; i++) {
            const HsMob *o = &hs.mob[i];
            if (i == hs.boss || !hittable(o)) continue;
            int d = iabs(o->x / HS_U - wx()) + iabs(o->y / HS_U - wy());
            /* not the ones at its feet, unless they are at hers */
            if (d > 28 && iabs(o->x - hs.mob[hs.boss].x) + iabs(o->y - hs.mob[hs.boss].y) < 50 * HS_U) continue;
            if (d < bd) { bd = d; best = i; }
        }
        if (best >= 0) m = best;
    }
    if (m >= 0 && (HS_FOE[hs.mob[m].kind].tier == 3 || g != G_HOME || iabs(hs.mob[m].x / HS_U - wx()) + iabs(hs.mob[m].y / HS_U - wy()) < 30))
        return fight(&hs.mob[m]);
    if (hs_bot_arena) return 0;
    int it = pick_items();
    if (it > 0) return it;
    int target = goal_room(g);
    if (hs.room == target) {
        switch (g) {
        case G_ODDS:
            for (int i = 0; i < HS_MAX_THINGS; i++)
                if (hs.thing[i].on && hs.thing[i].kind == TH_CHEST && !hs.thing[i].open) return use_thing(i);
            break;
        case G_TONIC: {
            int t = thing_of(TH_VENDOR, V_TONIC2);
            if (t >= 0) return use_thing(t);
            tonic_trip = sv.excursion;
            break;
        }
        case G_GEAR: { int t = thing_of(TH_GEAR, -1); if (t >= 0) return walk_to(hs.thing[t].tx * HS_T + 8, hs.thing[t].ty * HS_T + 8, 4); break; }
        case G_LETTER: { int t = thing_of(TH_NPC, N_HUSH); if (t >= 0) return use_thing(t); break; }
        case G_PIPE: { int t = thing_of(TH_NPC, N_TANGER); if (t >= 0) return use_thing(t); break; }
        case G_DUN1: case G_DUN2: case G_DUN3: {
            int t = thing_of(TH_PART, -1);
            if (t >= 0 && hs.boss < 0) return walk_to(hs.thing[t].tx * HS_T + 8, hs.thing[t].ty * HS_T + 8, 3);
            int bm = nearest_mob(999);
            if (bm >= 0) return fight(&hs.mob[bm]);
            return walk_to(HS_FW / 2, HS_FH / 2, 8);
        }
        default: {
            int bm = nearest_mob(999);
            if (bm >= 0) return fight(&hs.mob[bm]);
            return walk_to(HS_FW / 2, HS_FH / 2, 8);
        }
        }
    }
    int dist = 0, how = route(hs.room, target, &dist);
    if (how < 0) return (bf / 30) % 2 ? BTN_LEFT : BTN_RIGHT;
    return leave_by(how);
}

/* ---- camp --------------------------------------------------------------------------------------- */

/* a camp job: the spot, and the menu line (act, arg) to press */
typedef struct Job { int spot, act, arg; } Job;

static bool research_ready(int r) {
    if (sv.research >> r & 1) return false;
    if (r == RS_FUEL && !(sv.research >> RS_BIGBIN & 1)) return false;
    const HsResearch *x = &HS_RESEARCH[r];
    return sv.res[RES_BAR] >= x->bars && sv.res[RES_DATA] >= x->data && sv.res[RES_THREAD] >= x->thread;
}

static bool camp_job(Job *j) {
    int glints = sv.res[RES_GLINT], bars = sv.res[RES_BAR], jerky = sv.res[RES_JERKY];
    int hands = hs_hands();
    int built = 0;
    for (int a = 0; a < HS_ANVILS; a++) built += sv.anvils >> a & 1;
    /* fly home when the ship is ready (and, going for the Alien, the idol is aboard) */
    if (hs_parts_count() == 3 && (sv.research >> RS_FUEL & 1) && (sv.idol || !hs_bot_cherry)) {
        *j = (Job){P_SHIP, M_LAUNCH, 0};
        return true;
    }
    /* hands to work: the Thinker takes up to half of them */
    if (hs_free_hands() > 0) {
        int lab_want = sv.lab_fixed ? imax(1, hands / 2) : 0;
        if (sv.lab_fixed && sv.lab_hands < lab_want) { *j = (Job){P_LAB, M_LAB_ADD, 0}; return true; }
        for (int a = 0; a < HS_ANVILS; a++)
            if ((sv.anvils >> a & 1) && !(sv.anvil_hand >> a & 1)) { *j = (Job){P_ANVIL1 + a, M_ANVIL_HAND, a}; return true; }
        if (sv.lab_fixed && sv.lab_hands < 6) { *j = (Job){P_LAB, M_LAB_ADD, 0}; return true; }
    }
    /* the Thinker came late: move hands over from the anvils */
    if (sv.lab_fixed && hs_free_hands() == 0 && sv.lab_hands < imax(1, hands / 2))
        for (int a = 0; a < HS_ANVILS; a++)
            if (sv.anvil_hand >> a & 1) { *j = (Job){P_ANVIL1 + a, M_ANVIL_FREE, a}; return true; }
    if (!sv.smoke_built && bars >= SMOKE_BUILD) { *j = (Job){P_SMOKE, M_SMOKE_BUILD, 0}; return true; }
    int next = sv.plants < HS_PLANTS ? hs_plant_cost(sv.plants) : 1 << 30;
    int reserve = sv.smoke_built && sv.plants >= 3 ? JERKY_PRICE : 0;
    if (sv.plants < HS_PLANTS && glints >= next + reserve) { *j = (Job){P_BURL, M_PLANT, 0}; return true; }
    /* the first hut, then anvils for its hands, then more rooms */
    static const int HUT[3] = {HUT_COST0, HUT_COST1, HUT_COST2};
    for (int h = 0; h < HS_HUTS; h++)
        if (sv.huts[h] == 0 && bars >= HUT[0]) { *j = (Job){P_HUT1 + h, M_HUT, h}; return true; }
    if (built < imin(HS_ANVILS, hands) && bars >= ANVIL_COST)
        for (int a = 0; a < HS_ANVILS; a++)
            if (!(sv.anvils >> a & 1)) { *j = (Job){P_ANVIL1 + a, M_ANVIL_BUILD, a}; return true; }
    for (int h = 0; h < HS_HUTS; h++)
        if (sv.huts[h] > 0 && sv.huts[h] < 3 && bars >= HUT[sv.huts[h]]) { *j = (Job){P_HUT1 + h, M_HUT, h}; return true; }
    for (int b = 0; b < HS_BINS; b++) {
        if (sv.bins[b] == 0 && bars >= BIN_COST + 5) { *j = (Job){P_BIN1 + b, M_BIN, b}; return true; }
        if (sv.bins[b] == 1 && (sv.research >> RS_BIGBIN & 1) && bars >= BIN_RAISE) { *j = (Job){P_BIN1 + b, M_BIN, b}; return true; }
    }
    static const int ORDER[RS_COUNT] = {RS_METAL, RS_FERT, RS_BIGBIN, RS_PILL, RS_STAR, RS_FUEL};
    for (int k = 0; k < RS_COUNT; k++)
        if (research_ready(ORDER[k])) { *j = (Job){P_ORRERY, M_RESEARCH, ORDER[k]}; return true; }
    /* bars piling up at the limit while data is short: trade some */
    if (bars >= hs_cap(RES_BAR) - 20 && sv.res[RES_DATA] + TRADE_DATA <= hs_cap(RES_DATA) && !(sv.research >> RS_FUEL & 1)) {
        *j = (Job){P_ORRERY, M_TRADE, 0};
        return true;
    }
    /* jerky for the next trip */
    int g = wild_goal();
    if (g != G_NONE && sv.smoke_built && sv.smoke_stock > 0 && jerky < want_jerky(g) && glints >= JERKY_PRICE) {
        *j = (Job){P_SMOKE, M_JERKY, 0};
        return true;
    }
    /* bars from glints once the cheap buds are in */
    bool urgent = !sv.smoke_built || sv.huts[0] == 0;
    if (glints >= GLINTS_PER_BAR && bars < hs_cap(RES_BAR) && (urgent || next > 1280 || next > hs_cap(RES_GLINT))) {
        bool keep_for_jerky = g != G_NONE && jerky < want_jerky(g) && sv.smoke_built && glints < GLINTS_PER_BAR + JERKY_PRICE;
        if (!keep_for_jerky) { *j = (Job){P_BENCH, M_CRAFT, 0}; return true; }
    }
    return false;
}

/* stand in front of a camp spot, face it, press A */
static int at_spot(int p) {
    const HsSpot *s = &HS_SPOT[p];
    if (p == P_PAD) {
        int cx = s->x + s->w / 2, cy = s->y + s->h / 2;
        if (iabs(wx() - cx) <= 2 && iabs(wy() - cy) <= 2) return tap(BTN_A);
        int b = walk_to(cx, cy, 2);
        return b < 0 ? 0 : b;
    }
    /* below, above, left or right of it */
    int cand[4][4] = {
        {s->x + s->w / 2, s->y + s->h + 7, 0, -1},
        {s->x + s->w / 2, s->y - 7, 0, 1},
        {s->x - 7, s->y + s->h / 2, 1, 0},
        {s->x + s->w + 7, s->y + s->h / 2, -1, 0},
    };
    for (int k = 0; k < 4; k++) {
        int px = cand[k][0], py = cand[k][1];
        if (!hs_box_free(px, py, 5)) continue;
        if (iabs(wx() - px) <= 2 && iabs(wy() - py) <= 2) {
            if (hs.dirx != cand[k][2] || hs.diry != cand[k][3]) return dir_bits(cand[k][2], cand[k][3]);
            return tap(BTN_A);
        }
        int b = walk_to(px, py, 2);
        if (b >= 0) return b ? b : dir_bits(px - wx(), py - wy());
    }
    return 0;
}

static int camp(void) {
    hs_bot_goal = wild_goal();
    Job j;
    if (camp_job(&j)) return at_spot(j.spot);
    int g = wild_goal();
    if (g != G_NONE && ready_for(g) && sv.res[RES_JERKY] >= want_jerky(g)) {
        /* out through the gate */
        if (iabs(wx() - 160) <= 3 && wy() < 30) return BTN_UP;
        int b = walk_to(160, 12, 3);
        return b < 0 ? BTN_UP : (b ? b : BTN_UP);
    }
    /* early on, knock glints out of the Glowstone */
    if (sv.plants < 3) {
        if (iabs(wx() - 160) > 2 || iabs(wy() - 105) > 2) {
            int b = walk_to(160, 105, 2);
            return b < 0 ? 0 : b;
        }
        if (hs.diry != -1 || hs.dirx != 0) return BTN_UP;
        return tap(BTN_B);
    }
    hs_bot_idle = 1;
    return 0; /* nothing to do: wait for the camp to make more */
}

/* ---- menus ------------------------------------------------------------------------------------ */

static int menu(void) {
    if (++menu_wait < 6) return 0;
    /* what the bot came for */
    int want_act = -1, want_arg = -1;
    if (hs.area == AR_BASE) {
        Job j;
        if (camp_job(&j) && hs_front_spot() == j.spot) { want_act = j.act; want_arg = j.arg; }
        /* the launch question: say yes */
        for (int i = 0; i < hm.n; i++)
            if (hm.line[i].act == M_LAUNCH && hm.line[i].arg == 1) { want_act = M_LAUNCH; want_arg = 1; }
    }
    /* a noodler: tonics while the odds last (and then the trip's shopping is done) */
    if (hs.area != AR_BASE && hm.n > 0 && hm.line[0].act == M_BUY) {
        if (hm.line[0].ok && hm.line[0].arg == V_TONIC2) return tap(BTN_A);
        tonic_trip = sv.excursion;
        return tap(BTN_B);
    }
    if (want_act < 0) return tap(BTN_B);
    int line = -1;
    for (int i = 0; i < hm.n; i++)
        if (hm.line[i].act == want_act && (hm.line[i].arg == want_arg || want_act == M_LAB_ADD || want_act == M_PLANT ||
                                           want_act == M_JERKY || want_act == M_CRAFT))
            line = i;
    if (line < 0 || !hm.line[line].ok) return tap(BTN_B);
    if (hm.sel != line) return tap(BTN_DOWN);
    return tap(BTN_A);
}

/* ---- the frame -------------------------------------------------------------------------------- */

int hs_bot_buttons(void) {
    bf++;
    hs_bot_idle = 0;
    switch (hs_game_state()) {
    case S_TITLE: return tap(BTN_A);
    case S_INTRO: case S_FADED: case S_HOME: case S_LAUNCH: case S_CREDITS: return tap(BTN_A);
    default: break;
    }
    /* the START menu: swap weapons with its second row, else leave it */
    /* the thunderpipe until the star yo-yo outhits it (3 a throw against 1 a shot) */
    bool want_pipe = sv.pipe && !(sv.research >> RS_STAR & 1) && sv.plants >= 4 && hs_total(RES_GLINT) >= 300;
    if (game_paused()) {
        if ((sv.weapon == 1) == want_pipe) { swap_phase = 0; return tap(BTN_START); }
        swap_phase++;
        if (swap_phase == 2) return BTN_DOWN;
        if (swap_phase == 4) return BTN_A;
        if (swap_phase > 8) swap_phase = 0;
        return 0;
    }
    swap_phase = 0;
    if (sv.pipe && (sv.weapon == 1) != want_pipe && !hs_talk_open() && !hm.open && !hs_gamble_state()) return tap(BTN_START);
    if (hs_talk_open()) return tap(BTN_A);
    if (hs_gamble_state()) return tap(BTN_A);
    if (hm.open) return menu();
    menu_wait = 0;
    if (hs.room != last_room) { last_room = hs.room; stuck_t = 0; jiggle_t = 0; }
    if (jiggle_t > 0) { jiggle_t--; return jiggle_b; }
    int b = sv.on_trip ? wilds() : camp();
    note_foes();
    /* unstick: if walking has not moved Wick for two seconds, wiggle */
    if (b & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)) {
        if (++stuck_t > 120) {
            if (iabs(wx() - stuck_x) + iabs(wy() - stuck_y) < 4) {
                static const int W[4] = {BTN_UP, BTN_LEFT, BTN_DOWN, BTN_RIGHT};
                jiggle_b = W[(bf / 7) % 4] | W[(bf / 13 + 1) % 4];
                jiggle_t = 16;
            }
            stuck_t = 0;
            stuck_x = wx();
            stuck_y = wy();
        }
    }
    return b;
}
