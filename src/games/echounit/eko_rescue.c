/* ECHO UNIT - the jailbreak: a captured bat's rescue, played turn by turn by
 * a bat still free. Three rooms are rolled each time: the cell on the left,
 * guards in the middle, and the keycard on the right with guards all round
 * it. Every step (or skill) is one turn, then every guard takes its turn;
 * steps go only the four straight ways. Crates push like a block puzzle and
 * stop sight and shots. A guard reacts only to what it sees along its line
 * (or a noise it hears), and it takes a turn to act: seen, it lines up its
 * gun (the line shows), and fires on its next turn, at whatever is first in
 * that line by then, the bat or another guard. Lure them into each other.
 * Each bat has two skills; used, a skill comes back after a few turns:
 *   FLINT  A punch (the guard in front is out)    2   B brace (stops a shot)       4
 *   VESPER A shoot (down the line, loud)          3   B roll two tiles             2
 *   DART   A gun (down the line, one shot drops   2   B dash up to three tiles,    3
 *            anything, loud)                            felling a guard in the way
 *   SABLE  A dart (asleep for 6 turns, silent)    3   B whistle (they come to look) 2
 * A or B while that skill is still coming back: the bat waits a turn.
 * Take the card, bring it to the cell door, and the captive walks free.
 * Out of health, the rescuer is caught too. */
#include "eko.h"

const int EKO_RS_HP[OWL_COUNT] = {3, 2, 4, 2};
const int EKO_RS_CD[OWL_COUNT][2] = {{2, 4}, {3, 2}, {2, 3}, {3, 2}};
const char *const EKO_RS_SKILL[OWL_COUNT][2] = {{"PUNCH", "BRACE"}, {"SHOOT", "ROLL"}, {"GUN", "DASH"}, {"DART", "WHISTLE"}};
static const int DX[5] = {0, 1, 0, -1, 0}, DY[5] = {0, 0, 1, 0, -1}; /* RA_RIGHT.. index 1..4 */
/* guard dirs 0..3 = right, down, left, up */
static const int GDX[4] = {1, 0, -1, 0}, GDY[4] = {0, 1, 0, -1};

static bool inside(int x, int y) { return x >= 0 && y >= 0 && x < RS_W && y < RS_H; }

static int guard_at(const EkoRescue *r, int x, int y) {
    for (int i = 0; i < r->ng; i++)
        if (r->g[i].alive && r->g[i].x == x && r->g[i].y == y) return i;
    return -1;
}

static bool floorish(const EkoRescue *r, int x, int y) {
    if (!inside(x, y)) return false;
    char c = r->tile[y][x];
    return c == '.' || c == 'K';
}

/* the line a guard looks along: does it reach the bat? */
int eko_rescue_sees(const EkoRescue *r, int gi) {
    const EkoRGuard *g = &r->g[gi];
    if (!g->alive || g->state == RGS_SLEEP) return 0;
    int x = g->x, y = g->y;
    for (int k = 0; k < RS_W; k++) {
        x += GDX[(int)g->dir];
        y += GDY[(int)g->dir];
        if (!floorish(r, x, y)) return 0;
        if (x == r->ox && y == r->oy) return 1;
        if (guard_at(r, x, y) >= 0) return 0;
    }
    return 0;
}

/* a hunting guard's open, straight line to the bat (any of the four ways), or -1 */
static int line_to_bat(const EkoRescue *r, int gi) {
    const EkoRGuard *g = &r->g[gi];
    for (int d = 0; d < 4; d++) {
        int x = g->x, y = g->y;
        for (int k = 0; k < RS_W; k++) {
            x += GDX[d];
            y += GDY[d];
            if (!floorish(r, x, y)) break;
            if (x == r->ox && y == r->oy) return d;
            if (guard_at(r, x, y) >= 0) break;
        }
    }
    return -1;
}

/* breadth-first distances to (tx, ty) over open floor; guards (other than
 * skip) and crates block unless loose */
static void bfs(const EkoRescue *r, int tx, int ty, int skip, bool loose, int dist[RS_H][RS_W]) {
    for (int y = 0; y < RS_H; y++) for (int x = 0; x < RS_W; x++) dist[y][x] = 9999;
    int qx[RS_W * RS_H], qy[RS_W * RS_H], h = 0, t = 0;
    dist[ty][tx] = 0;
    qx[t] = tx; qy[t] = ty; t++;
    while (h < t) {
        int x = qx[h], y = qy[h]; h++;
        for (int d = 0; d < 4; d++) {
            int nx = x + GDX[d], ny = y + GDY[d];
            if (!inside(nx, ny) || dist[ny][nx] != 9999) continue;
            char c = r->tile[ny][nx];
            bool open = c == '.' || c == 'K' || (loose && c == 'o');
            if (!open) continue;
            if (!loose) { int gi = guard_at(r, nx, ny); if (gi >= 0 && gi != skip) continue; }
            dist[ny][nx] = dist[y][x] + 1;
            qx[t] = nx; qy[t] = ny; t++;
        }
    }
}

static void guard_step_to(EkoRescue *r, int gi, int tx, int ty) {
    EkoRGuard *g = &r->g[gi];
    static int dist[RS_H][RS_W];
    bfs(r, tx, ty, gi, false, dist);
    int best = -1, bd = dist[(int)g->y][(int)g->x];
    for (int d = 0; d < 4; d++) {
        int nx = g->x + GDX[d], ny = g->y + GDY[d];
        if (!inside(nx, ny)) continue;
        if (nx == r->ox && ny == r->oy) continue;
        if (!floorish(r, nx, ny) || guard_at(r, nx, ny) >= 0) continue;
        if (dist[ny][nx] < bd) { bd = dist[ny][nx]; best = d; }
    }
    if (best >= 0) {
        g->x = (int8_t)(g->x + GDX[best]);
        g->y = (int8_t)(g->y + GDY[best]);
        g->dir = (int8_t)best;
    }
}

static void noise(EkoRescue *r, int range) {
    r->noise_t = 12;
    r->noise_x = r->ox;
    r->noise_y = r->oy;
    for (int i = 0; i < r->ng; i++) {
        EkoRGuard *g = &r->g[i];
        if (!g->alive || g->state == RGS_SLEEP || g->state == RGS_ALERT || g->state == RGS_AIM) continue;
        if (iabs(g->x - r->ox) + iabs(g->y - r->oy) > range) continue;
        g->state = RGS_SEEK;
        g->tx = (int8_t)r->ox;
        g->ty = (int8_t)r->oy;
    }
}

/* a shot down the line the bat faces: the first guard it meets */
static int line_target(const EkoRescue *r) {
    int x = r->ox, y = r->oy;
    for (int k = 0; k < RS_W; k++) {
        x += DX[r->dir];
        y += DY[r->dir];
        if (!floorish(r, x, y)) return -1;
        int gi = guard_at(r, x, y);
        if (gi >= 0) return gi;
    }
    return -1;
}

static void ko(EkoRescue *r, int gi) {
    r->g[gi].alive = 0;
    r->g[gi].state = RGS_OUT;
    r->kos++;
}

static void shot_fx(EkoRescue *r, int gi) {
    r->shot_t = 8;
    r->shot_x = r->ox; r->shot_y = r->oy;
    if (gi >= 0) { r->shot_x2 = r->g[gi].x; r->shot_y2 = r->g[gi].y; }
    else {
        int x = r->ox, y = r->oy;
        while (floorish(r, x + DX[r->dir], y + DY[r->dir])) { x += DX[r->dir]; y += DY[r->dir]; }
        r->shot_x2 = x; r->shot_y2 = y;
    }
}

/* a guard fires down the line it aimed along: the first thing there takes
 * it, the bat or another guard (a crate or a wall stops it) */
static void guard_fire(EkoRescue *r, int gi) {
    EkoRGuard *g = &r->g[gi];
    int x = g->x, y = g->y;
    r->gshots++;
    r->gshot_t = 10;
    r->gshot_x = g->x;
    r->gshot_y = g->y;
    for (int k = 0; k < RS_W + RS_H; k++) {
        int nx = x + GDX[(int)g->dir], ny = y + GDY[(int)g->dir];
        if (!floorish(r, nx, ny)) break;
        x = nx;
        y = ny;
        if (x == r->ox && y == r->oy) {
            if (r->brace) r->brace = false; /* braced: it bounces off */
            else { r->hp--; r->hits++; }
            break;
        }
        int j = guard_at(r, x, y);
        if (j >= 0) {
            r->g[j].alive = 0;
            r->g[j].state = RGS_OUT;
            r->ff++;
            break;
        }
    }
    r->gshot_x2 = x;
    r->gshot_y2 = y;
}

/* try to step the bat one tile: false if nothing moved (no turn taken) */
static bool step(EkoRescue *r, int d) {
    int nx = r->ox + DX[d], ny = r->oy + DY[d];
    if (!inside(nx, ny)) return false;
    char c = r->tile[ny][nx];
    if (c == 'C') {
        if (!r->card) return false;
        r->won = true;
        r->ox = nx; r->oy = ny;
        return true;
    }
    if (guard_at(r, nx, ny) >= 0) return false;
    if (c == 'o') {
        int bx = nx + DX[d], by = ny + DY[d];
        if (!inside(bx, by) || r->tile[by][bx] != '.' || guard_at(r, bx, by) >= 0) return false;
        r->tile[by][bx] = 'o';
        r->tile[ny][nx] = '.';
    } else if (c != '.' && c != 'K') return false;
    r->ox = nx;
    r->oy = ny;
    if (r->tile[ny][nx] == 'K') { r->card = true; r->tile[ny][nx] = '.'; }
    return true;
}

/* the bat's skill k (0 on A, 1 on B): false if it could not be used */
static bool skill(EkoRescue *r, int k) {
    int fx = r->ox + DX[r->dir], fy = r->oy + DY[r->dir];
    switch (r->owl * 2 + k) {
    case OWL_FLINT * 2: {
        /* the punch: the guard in front is out (a swing at nothing still costs the turn) */
        int gi = guard_at(r, fx, fy);
        if (gi >= 0) ko(r, gi);
        return true;
    }
    case OWL_FLINT * 2 + 1:
        r->brace = true; /* the next shot this turn bounces off */
        return true;
    case OWL_VESPER * 2: case OWL_DART * 2: {
        int gi = line_target(r);
        shot_fx(r, gi);
        if (gi >= 0) ko(r, gi);
        noise(r, 5);
        return true;
    }
    case OWL_VESPER * 2 + 1:
        /* a roll: two tiles on, both clear */
        if (floorish(r, fx, fy) && guard_at(r, fx, fy) < 0 && floorish(r, fx + DX[r->dir], fy + DY[r->dir]) &&
            guard_at(r, fx + DX[r->dir], fy + DY[r->dir]) < 0) {
            step(r, r->dir);
            step(r, r->dir);
            return true;
        }
        return false;
    case OWL_DART * 2 + 1: {
        /* a dash: up to three tiles; a guard in the way is knocked out */
        bool moved = false;
        for (int s = 0; s < 3; s++) {
            int nx = r->ox + DX[r->dir], ny = r->oy + DY[r->dir];
            int gi = guard_at(r, nx, ny);
            if (gi >= 0) { ko(r, gi); return true; }
            if (!floorish(r, nx, ny)) break;
            step(r, r->dir);
            moved = true;
        }
        return moved;
    }
    case OWL_SABLE * 2: {
        int gi = line_target(r);
        shot_fx(r, gi);
        if (gi >= 0) { r->g[gi].state = RGS_SLEEP; r->g[gi].sleep = 6; }
        return true;
    }
    case OWL_SABLE * 2 + 1:
        noise(r, 6);
        return true;
    default: return false;
    }
}

void eko_rescue_turn(EkoRescue *r, int action) {
    if (r->won || r->lost || action == RA_NONE) return;
    bool took = false;
    int used = -1;
    r->brace = false;
    if (action >= RA_RIGHT && action <= RA_UP) {
        r->dir = action;
        took = step(r, action);
    } else if (action == RA_A || action == RA_B) {
        int k = action == RA_A ? 0 : 1;
        if (r->cd[k] > 0) took = true; /* not back yet: the bat just waits a turn */
        else {
            took = skill(r, k);
            if (took) used = k;
        }
    }
    if (!took) return;
    r->turns++;
    /* the skills count down; the one just used starts over */
    for (int k = 0; k < 2; k++) if (r->cd[k] > 0) r->cd[k]--;
    if (used >= 0) r->cd[used] = EKO_RS_CD[r->owl][used];
    if (r->won) return;
    /* the guards' turn */
    for (int i = 0; i < r->ng; i++) {
        EkoRGuard *g = &r->g[i];
        if (!g->alive) continue;
        switch (g->state) {
        case RGS_SLEEP:
            if (--g->sleep <= 0) { g->state = RGS_POST; g->sleep = 0; }
            break;
        case RGS_AIM:
            /* lined up last turn: it fires now, at whatever is there */
            guard_fire(r, i);
            g->state = RGS_ALERT;
            break;
        case RGS_ALERT: {
            int d = line_to_bat(r, i);
            if (d >= 0) { g->dir = (int8_t)d; g->state = RGS_AIM; } /* lines up: fires next turn */
            else guard_step_to(r, i, r->ox, r->oy);
            break;
        }
        case RGS_SEEK:
            if (g->x == g->tx && g->y == g->ty) g->state = RGS_POST;
            else {
                int ox = g->x, oy = g->y;
                guard_step_to(r, i, g->tx, g->ty);
                if (g->x == ox && g->y == oy) g->state = RGS_POST;
                else if (iabs(g->x - g->tx) + iabs(g->y - g->ty) <= 1 && guard_at(r, g->tx, g->ty) < 0 &&
                         (g->tx == r->ox && g->ty == r->oy)) g->state = RGS_POST;
            }
            break;
        default:
            if (g->role == RG_TURNER) g->dir = (int8_t)((g->dir + 1) & 3);
            else if (g->role == RG_PATROL) {
                int nx = g->x + GDX[(int)g->dir], ny = g->y + GDY[(int)g->dir];
                if (!floorish(r, nx, ny) || guard_at(r, nx, ny) >= 0 || (nx == r->ox && ny == r->oy)) g->dir = (int8_t)((g->dir + 2) & 3);
                else { g->x = (int8_t)nx; g->y = (int8_t)ny; }
            }
            break;
        }
    }
    /* who sees the bat now: it lines up its gun, and fires on its next turn */
    for (int i = 0; i < r->ng; i++) {
        EkoRGuard *g = &r->g[i];
        if (g->alive && g->state != RGS_ALERT && g->state != RGS_AIM && eko_rescue_sees(r, i)) g->state = RGS_AIM;
    }
    if (r->hp <= 0) r->lost = true;
}

/* ---- rolling a jail -------------------------------------------------------------------- */

static bool free_spot(const EkoRescue *r, int x, int y) {
    return inside(x, y) && r->tile[y][x] == '.' && guard_at(r, x, y) < 0 && !(x == r->start_x && y == r->start_y);
}

static void add_g(EkoRescue *r, int role, int x, int y, int dir) {
    if (r->ng >= RS_MAX_GUARDS) return;
    EkoRGuard *g = &r->g[r->ng++];
    memset(g, 0, sizeof *g);
    g->alive = 1;
    g->role = (uint8_t)role;
    g->state = RGS_POST;
    g->x = g->hx = (int8_t)x;
    g->y = g->hy = (int8_t)y;
    g->dir = g->hdir = (int8_t)dir;
}

/* the dividers between the three rooms */
#define RS_WALL1 4
#define RS_WALL2 9

int eko_rescue_check(const EkoRescue *r) {
    static int d2[RS_H][RS_W];
    /* the guards round the card stand in the way: count them as open for the path */
    bfs(r, r->key_x, r->key_y, -1, true, d2);
    if (d2[r->start_y][r->start_x] >= 9999) return 1;
    /* the way back to the cell door: open floor next to it */
    int cx = r->cell_x + 1, cy = r->cell_y;
    bfs(r, cx, cy, -1, true, d2);
    if (d2[r->key_y][r->key_x] >= 9999) return 2;
    /* nobody sees the start, nobody stands near it */
    for (int i = 0; i < r->ng; i++) {
        if (eko_rescue_sees(r, i)) return 3;
        if (iabs(r->g[i].x - r->start_x) + iabs(r->g[i].y - r->start_y) <= 2) return 4;
    }
    /* the card has guards all round it */
    int round = 0;
    for (int d = 0; d < 4; d++) round += guard_at(r, r->key_x + GDX[d], r->key_y + GDY[d]) >= 0;
    if (round < 2) return 5;
    return 0;
}

int eko_rescue_gen(EkoRescue *r, uint32_t seed, int owl, int captive) {
    Rng rng;
    rng_seed(&rng, seed * 2654435761u + 77);
    for (int attempt = 0; attempt < 200; attempt++) {
        memset(r, 0, sizeof *r);
        r->owl = owl;
        r->captive = captive;
        r->seed = (int)seed;
        r->hp = EKO_RS_HP[owl];
        r->dir = RA_RIGHT;
        for (int y = 0; y < RS_H; y++)
            for (int x = 0; x < RS_W; x++)
                r->tile[y][x] = (x == 0 || y == 0 || x == RS_W - 1 || y == RS_H - 1 || x == RS_WALL1 || x == RS_WALL2) ? '#' : '.';
        /* doorways in the two dividers */
        for (int wall = RS_WALL1; wall <= RS_WALL2; wall += RS_WALL2 - RS_WALL1) {
            int n = rng_range(&rng, 1, 2);
            for (int k = 0; k < n; k++) r->tile[rng_range(&rng, 1, RS_H - 2)][wall] = '.';
        }
        /* a few pillars to break the rooms up (never against a doorway) */
        for (int k = 0, n = rng_range(&rng, 1, 3); k < n; k++) {
            int x = rng_range(&rng, RS_WALL1 + 2, RS_W - 2), y = rng_range(&rng, 2, RS_H - 3);
            if (x >= RS_WALL2 - 1 && x <= RS_WALL2 + 1) continue;
            r->tile[y][x] = '#';
        }
        /* the cell on the left wall, the way in beside it */
        r->cell_x = 0;
        r->cell_y = rng_range(&rng, 1, RS_H - 2);
        r->tile[r->cell_y][0] = 'C';
        r->tile[r->cell_y][1] = '.';
        r->start_x = rng_range(&rng, 2, RS_WALL1 - 1);
        r->start_y = rng_range(&rng, 1, RS_H - 2);
        r->ox = r->start_x;
        r->oy = r->start_y;
        /* the card, with guards round it, looking out */
        r->key_x = rng_range(&rng, RS_WALL2 + 2, RS_W - 3);
        r->key_y = rng_range(&rng, 2, RS_H - 3);
        r->tile[r->key_y][r->key_x] = 'K';
        for (int d = 0; d < 4; d++) r->tile[r->key_y + GDY[d]][r->key_x + GDX[d]] = '.';
        int around = rng_range(&rng, 2, 3);
        int first = rng_range(&rng, 0, 3);
        for (int k = 0; k < around; k++) {
            int d = (first + k) & 3;
            add_g(r, RG_SENTRY, r->key_x + GDX[d], r->key_y + GDY[d], d);
        }
        /* the middle room's watch */
        int mid = rng_range(&rng, 1, 2);
        for (int k = 0; k < mid; k++) {
            int x = rng_range(&rng, RS_WALL1 + 1, RS_WALL2 - 1), y = rng_range(&rng, 1, RS_H - 2);
            if (!free_spot(r, x, y)) continue;
            int role = rng_range(&rng, 0, 2) == 0 ? RG_TURNER : RG_PATROL;
            add_g(r, role, x, y, rng_range(&rng, 0, 3));
        }
        /* one walking the right room now and then */
        if (rng_range(&rng, 0, 1)) {
            int x = rng_range(&rng, RS_WALL2 + 1, RS_W - 2), y = rng_range(&rng, 1, RS_H - 2);
            if (free_spot(r, x, y) && iabs(x - r->key_x) + iabs(y - r->key_y) > 1) add_g(r, RG_PATROL, x, y, rng_range(&rng, 0, 1) * 2 + 1);
        }
        /* crates (never against a doorway) */
        for (int k = 0, n = rng_range(&rng, 3, 6); k < n; k++) {
            int x = rng_range(&rng, 2, RS_W - 2), y = rng_range(&rng, 2, RS_H - 3);
            if (iabs(x - RS_WALL1) <= 1 || iabs(x - RS_WALL2) <= 1) continue;
            if (!free_spot(r, x, y) || iabs(x - r->key_x) + iabs(y - r->key_y) <= 1) continue;
            if (y == r->cell_y && x <= 2) continue;
            r->tile[y][x] = 'o';
        }
        if (eko_rescue_check(r) == 0) return attempt;
    }
    return -1;
}

/* ---- the demo player's look-ahead --------------------------------------------------------- */

/* is guard gi's aimed line (or, hunting, any open line) on the bat? */
static bool on_line(const EkoRescue *r, int gi, bool any) {
    const EkoRGuard *g = &r->g[gi];
    if (any) return line_to_bat(r, gi) >= 0;
    int x = g->x, y = g->y;
    for (int k = 0; k < RS_W; k++) {
        x += GDX[(int)g->dir];
        y += GDY[(int)g->dir];
        if (!floorish(r, x, y)) return false;
        if (x == r->ox && y == r->oy) return true;
        if (guard_at(r, x, y) >= 0) return false;
    }
    return false;
}

/* the ways to the card and to the cell door, crates counted as open: walls
 * never move, so these are worked out once a decision */
static int bot_dkey[RS_H][RS_W], bot_dcell[RS_H][RS_W];
/* what a point of health is worth to it: dear at first, cheaper once it has
 * gone a long while without getting any nearer (a knock for headway) */
static int bot_hp_w = 3000;

static int eval(const EkoRescue *r, uint8_t visits[RS_H][RS_W]) {
    if (r->lost) return -1000000;
    if (r->won) return 1000000 - r->turns;
    /* a crate shoved into the cell door's tile shuts the way out for good */
    if (r->tile[r->cell_y][r->cell_x + 1] == 'o') return -900000;
    /* nor any crate in a doorway between the rooms, or just either side of it */
    int jam = 0;
    for (int y = 1; y < RS_H - 1; y++)
        for (int w = RS_WALL1; w <= RS_WALL2; w += RS_WALL2 - RS_WALL1)
            if (r->tile[y][w] != '#')
                for (int x = w - 1; x <= w + 1; x++) jam += r->tile[y][x] == 'o';
    int s = (r->hp >= 2 ? (r->hp - 1) * bot_hp_w + 3000 : r->hp * 3000) + (r->kos + r->ff) * 120;
    int d = r->card ? bot_dcell[r->oy][r->ox] : bot_dkey[r->oy][r->ox];
    if (d >= 9999) d = 60;
    s -= d * 50;
    if (r->card) s += 20000;
    for (int i = 0; i < r->ng; i++) {
        const EkoRGuard *g = &r->g[i];
        if (!g->alive) continue;
        /* a gun lined up on the bat fires next turn; a hunter with a line will line up */
        if (g->state == RGS_AIM && on_line(r, i, false)) s -= r->brace ? 300 : 2400;
        else if (g->state == RGS_ALERT && on_line(r, i, true)) s -= 500;
        else if (g->state == RGS_ALERT) s -= 100;
    }
    s -= visits[r->oy][r->ox] * 60;
    s -= (r->cd[0] + r->cd[1]) * 15;
    s -= jam * 6000;
    return s;
}

/* the moves worth trying from here: the four steps, and each skill, also
 * after a free turn toward a blocked side (pressing toward a wall or a
 * guard turns the bat without taking a turn). Codes: 1..4 steps, 5 / 6 the
 * skills as faced, 7 + 2 * (dir - 1) + k the skill k after turning to dir. */
static int moves(const EkoRescue *r, int *out) {
    int n = 0;
    for (int a = RA_RIGHT; a <= RA_B; a++) out[n++] = a;
    for (int d = RA_RIGHT; d <= RA_UP; d++) {
        if (d == r->dir) continue;
        EkoRescue c = *r;
        eko_rescue_turn(&c, d);
        if (c.turns != r->turns || c.won) continue; /* that way is a step, not a turn */
        out[n++] = 7 + 2 * (d - 1);
        out[n++] = 8 + 2 * (d - 1);
    }
    return n;
}

static void play_move(EkoRescue *r, int m) {
    if (m >= 7) {
        r->dir = 1 + (m - 7) / 2;
        eko_rescue_turn(r, (m - 7) % 2 ? RA_B : RA_A);
    } else eko_rescue_turn(r, m);
}

static int search(const EkoRescue *r, int depth, uint8_t visits[RS_H][RS_W]) {
    if (depth == 0 || r->won || r->lost) return eval(r, visits);
    int best = -2000000;
    int mv[16], n = moves(r, mv);
    for (int i = 0; i < n; i++) {
        EkoRescue c = *r;
        play_move(&c, mv[i]);
        if (c.turns == r->turns && !c.won) continue;
        int v = search(&c, depth - 1, visits);
        if (v > best) best = v;
    }
    return best == -2000000 ? eval(r, visits) : best;
}

static uint8_t bot_visits[RS_H][RS_W];
static int bot_seed = -1, bot_turn = -1, bot_best = 9999, bot_since = 0;

int eko_rescue_bot_action(const EkoRescue *r) {
    if (r->won || r->lost) return RA_NONE;
    if (bot_seed != r->seed || r->turns < bot_turn) {
        memset(bot_visits, 0, sizeof bot_visits);
        bot_seed = r->seed;
        bot_best = 9999;
        bot_since = 0;
    }
    bfs(r, r->key_x, r->key_y, -1, true, bot_dkey);
    bfs(r, r->cell_x + 1, r->cell_y, -1, true, bot_dcell);
    if (r->turns != bot_turn) {
        bot_turn = r->turns;
        if (bot_visits[r->oy][r->ox] < 200) bot_visits[r->oy][r->ox]++;
        /* headway: the steps still to go (the card counts as half the way) */
        int left = r->card ? bot_dcell[r->oy][r->ox] : 100 + bot_dkey[r->oy][r->ox];
        if (left < bot_best) { bot_best = left; bot_since = 0; }
        else bot_since++;
    }
    bot_hp_w = bot_since > 48 ? 100 : bot_since > 24 ? 600 : 3000;
    int best = 0, bv = -3000000;
    int mv[16], n = moves(r, mv);
    for (int i = 0; i < n; i++) {
        EkoRescue c = *r;
        play_move(&c, mv[i]);
        if (c.turns == r->turns && !c.won) continue;
        /* the look-ahead's best, plus a little of how things stand right
         * after this move: of two ways to the same end, the sooner one */
        int v = search(&c, 4, bot_visits) + eval(&c, bot_visits) / 4;
        if (v > bv) { bv = v; best = mv[i]; }
    }
    if (best == 0) return RA_NONE;
    /* a skill after a turn: the turn first (it takes no turn), the skill on the next press */
    if (best >= 7) return 1 + (best - 7) / 2;
    return best;
}
