/* ECHO UNIT - the jailbreak: a captured bat's rescue, played turn by turn by
 * a bat still free. Three rooms are rolled each time: the cell on the left,
 * guards in the middle, and the keycard on the right with guards all round
 * it. Every step (or action) is one turn, then every guard takes its turn.
 * Crates push like a block puzzle and block a guard's view. A guard that
 * sees you along its line comes for you and strikes when it is next to
 * you; draw them out one at a time. Each bat has two moves of its own:
 *   FLINT  A punch (the guard in front is out)     B brace (blocks a blow)
 *   VESPER A shoot (three shots, loud)             B roll two tiles
 *   DART   A shoot (no limit, loud, one shot each) B dash up to three tiles
 *   SABLE  A dart (three, silent: asleep 6 turns)   B whistle (they come to look)
 * Take the card, bring it to the cell door, and the captive walks free. */
#include "eko.h"

const int EKO_RS_HP[OWL_COUNT] = {3, 2, 4, 2};
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
        if (!g->alive || g->state == RGS_SLEEP || g->state == RGS_ALERT) continue;
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

void eko_rescue_turn(EkoRescue *r, int action) {
    if (r->won || r->lost || action == RA_NONE) return;
    bool took = false;
    r->brace = false;
    if (action >= RA_RIGHT && action <= RA_UP) {
        r->dir = action;
        took = step(r, action);
    } else if (action == RA_A) {
        took = true;
        switch (r->owl) {
        case OWL_FLINT: {
            int gi = guard_at(r, r->ox + DX[r->dir], r->oy + DY[r->dir]);
            if (gi >= 0) ko(r, gi);
            break;
        }
        case OWL_VESPER: case OWL_DART: {
            if (r->owl == OWL_VESPER && r->ammo <= 0) { took = false; break; }
            if (r->owl == OWL_VESPER) r->ammo--;
            int gi = line_target(r);
            shot_fx(r, gi);
            if (gi >= 0) ko(r, gi);
            noise(r, 5);
            break;
        }
        case OWL_SABLE: {
            if (r->ammo <= 0) { took = false; break; }
            r->ammo--;
            int gi = line_target(r);
            shot_fx(r, gi);
            if (gi >= 0) { r->g[gi].state = RGS_SLEEP; r->g[gi].sleep = 6; }
            break;
        }
        }
    } else if (action == RA_B) {
        switch (r->owl) {
        case OWL_FLINT: r->brace = true; took = true; break;
        case OWL_VESPER:
            /* a roll: two tiles on, both clear */
            if (floorish(r, r->ox + DX[r->dir], r->oy + DY[r->dir]) && guard_at(r, r->ox + DX[r->dir], r->oy + DY[r->dir]) < 0 &&
                floorish(r, r->ox + 2 * DX[r->dir], r->oy + 2 * DY[r->dir]) && guard_at(r, r->ox + 2 * DX[r->dir], r->oy + 2 * DY[r->dir]) < 0) {
                step(r, r->dir);
                step(r, r->dir);
                took = true;
            }
            break;
        case OWL_DART: {
            /* a dash: up to three tiles; a guard in the way is knocked out */
            for (int k = 0; k < 3; k++) {
                int nx = r->ox + DX[r->dir], ny = r->oy + DY[r->dir];
                int gi = guard_at(r, nx, ny);
                if (gi >= 0) { ko(r, gi); took = true; break; }
                if (!floorish(r, nx, ny)) break;
                step(r, r->dir);
                took = true;
            }
            break;
        }
        case OWL_SABLE: noise(r, 6); took = true; break;
        }
    }
    if (!took) return;
    r->turns++;
    if (r->won) return;
    /* the guards' turn */
    for (int i = 0; i < r->ng; i++) {
        EkoRGuard *g = &r->g[i];
        if (!g->alive) continue;
        switch (g->state) {
        case RGS_SLEEP:
            if (--g->sleep <= 0) { g->state = RGS_POST; g->sleep = 0; }
            break;
        case RGS_ALERT:
            if (iabs(g->x - r->ox) + iabs(g->y - r->oy) == 1) {
                for (int d = 0; d < 4; d++) if (g->x + GDX[d] == r->ox && g->y + GDY[d] == r->oy) g->dir = (int8_t)d;
                if (r->brace) r->brace = false;
                else r->hp--;
            } else guard_step_to(r, i, r->ox, r->oy);
            break;
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
    /* who sees the bat now */
    for (int i = 0; i < r->ng; i++)
        if (r->g[i].alive && r->g[i].state != RGS_ALERT && eko_rescue_sees(r, i)) r->g[i].state = RGS_ALERT;
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

int eko_rescue_check(const EkoRescue *r) {
    static int d1[RS_H][RS_W], d2[RS_H][RS_W];
    bfs(r, r->key_x, r->key_y, -1, false, d1);
    /* the guards round the card stand in the way: count them as open for the path */
    bfs(r, r->key_x, r->key_y, -1, true, d2);
    if (d2[r->start_y][r->start_x] >= 9999) return 1;
    /* the way back to the cell door: open floor next to it */
    int cx = r->cell_x + 1, cy = r->cell_y;
    bfs(r, cx, cy, -1, true, d2);
    if (d2[r->key_y][r->key_x] >= 9999) return 2;
    /* nobody sees the start, nobody stands next to it */
    for (int i = 0; i < r->ng; i++) {
        if (eko_rescue_sees(r, i)) return 3;
        if (iabs(r->g[i].x - r->start_x) + iabs(r->g[i].y - r->start_y) <= 2) return 4;
    }
    /* the card has guards all round it */
    int round = 0;
    for (int d = 0; d < 4; d++) round += guard_at(r, r->key_x + GDX[d], r->key_y + GDY[d]) >= 0;
    if (round < 2) return 5;
    (void)d1;
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
        r->ammo = owl == OWL_VESPER || owl == OWL_SABLE ? 3 : 0;
        r->dir = RA_RIGHT;
        for (int y = 0; y < RS_H; y++)
            for (int x = 0; x < RS_W; x++)
                r->tile[y][x] = (x == 0 || y == 0 || x == RS_W - 1 || y == RS_H - 1 || x == 6 || x == 12) ? '#' : '.';
        /* doorways in the two dividers */
        for (int wall = 6; wall <= 12; wall += 6) {
            int n = rng_range(&rng, 1, 2);
            for (int k = 0; k < n; k++) r->tile[rng_range(&rng, 1, RS_H - 2)][wall] = '.';
        }
        /* a few pillars to break the rooms up */
        for (int k = 0, n = rng_range(&rng, 1, 3); k < n; k++) {
            int x = rng_range(&rng, 8, 17), y = rng_range(&rng, 2, RS_H - 3);
            if (x == 11 || x == 12 || x == 13) continue;
            r->tile[y][x] = '#';
        }
        /* the cell on the left wall, the way in beside it */
        r->cell_x = 0;
        r->cell_y = rng_range(&rng, 1, RS_H - 2);
        r->tile[r->cell_y][0] = 'C';
        r->tile[r->cell_y][1] = '.';
        r->start_x = rng_range(&rng, 2, 4);
        r->start_y = rng_range(&rng, 1, RS_H - 2);
        r->ox = r->start_x;
        r->oy = r->start_y;
        /* the card, with guards round it, looking out */
        r->key_x = rng_range(&rng, 14, 16);
        r->key_y = rng_range(&rng, 2, RS_H - 3);
        r->tile[r->key_y][r->key_x] = 'K';
        for (int d = 0; d < 4; d++) {
            int x = r->key_x + GDX[d], y = r->key_y + GDY[d];
            r->tile[y][x] = '.';
        }
        int around = rng_range(&rng, 2, 3);
        int first = rng_range(&rng, 0, 3);
        for (int k = 0; k < around; k++) {
            int d = (first + k) & 3;
            add_g(r, RG_SENTRY, r->key_x + GDX[d], r->key_y + GDY[d], d);
        }
        /* the middle room's watch */
        int mid = rng_range(&rng, 1, 2);
        for (int k = 0; k < mid; k++) {
            int x = rng_range(&rng, 7, 11), y = rng_range(&rng, 1, RS_H - 2);
            if (!free_spot(r, x, y)) continue;
            int role = rng_range(&rng, 0, 2) == 0 ? RG_TURNER : RG_PATROL;
            add_g(r, role, x, y, rng_range(&rng, 0, 3));
        }
        /* one walking the right room now and then */
        if (rng_range(&rng, 0, 1)) {
            int x = rng_range(&rng, 13, 17), y = rng_range(&rng, 1, RS_H - 2);
            if (free_spot(r, x, y) && iabs(x - r->key_x) + iabs(y - r->key_y) > 1) add_g(r, RG_PATROL, x, y, rng_range(&rng, 0, 1) * 2 + 1);
        }
        /* crates */
        for (int k = 0, n = rng_range(&rng, 3, 6); k < n; k++) {
            int x = rng_range(&rng, 2, 17), y = rng_range(&rng, 2, RS_H - 3);
            if (x == 6 || x == 12 || x == 5 || x == 7 || x == 11 || x == 13) continue; /* never against a doorway */
            if (!free_spot(r, x, y) || iabs(x - r->key_x) + iabs(y - r->key_y) <= 1) continue;
            if (y == r->cell_y && x <= 2) continue;
            r->tile[y][x] = 'o';
        }
        if (eko_rescue_check(r) == 0) return attempt;
    }
    return -1;
}

/* ---- the demo player's look-ahead --------------------------------------------------------- */

static int eval(const EkoRescue *r, uint8_t visits[RS_H][RS_W]) {
    if (r->lost) return -1000000;
    if (r->won) return 1000000 - r->turns;
    static int dist[RS_H][RS_W];
    int s = r->hp * 3000 + r->kos * 120;
    int tx = r->card ? r->cell_x + 1 : r->key_x, ty = r->card ? r->cell_y : r->key_y;
    bfs(r, tx, ty, -1, true, dist);
    int d = dist[r->oy][r->ox];
    if (d >= 9999) d = 60;
    s -= d * 50;
    if (r->card) s += 20000;
    for (int i = 0; i < r->ng; i++) {
        const EkoRGuard *g = &r->g[i];
        if (!g->alive) continue;
        int md = iabs(g->x - r->ox) + iabs(g->y - r->oy);
        if (g->state == RGS_ALERT && md <= 1) s -= 1500;
        else if (g->state == RGS_ALERT && md == 2) s -= 300;
    }
    s -= visits[r->oy][r->ox] * 60;
    if (r->owl == OWL_VESPER || r->owl == OWL_SABLE) s += r->ammo * 40;
    return s;
}

static int search(const EkoRescue *r, int depth, uint8_t visits[RS_H][RS_W]) {
    if (depth == 0 || r->won || r->lost) return eval(r, visits);
    int best = -2000000;
    for (int a = RA_RIGHT; a <= RA_B; a++) {
        EkoRescue c = *r;
        int t0 = c.turns;
        eko_rescue_turn(&c, a);
        if (c.turns == t0 && !c.won) continue;
        int v = search(&c, depth - 1, visits);
        if (v > best) best = v;
    }
    return best == -2000000 ? eval(r, visits) : best;
}

static uint8_t bot_visits[RS_H][RS_W];
static int bot_seed = -1, bot_turn = -1;

int eko_rescue_bot_action(const EkoRescue *r) {
    if (r->won || r->lost) return RA_NONE;
    if (bot_seed != r->seed || r->turns < bot_turn) { memset(bot_visits, 0, sizeof bot_visits); bot_seed = r->seed; }
    if (r->turns != bot_turn) {
        bot_turn = r->turns;
        if (bot_visits[r->oy][r->ox] < 200) bot_visits[r->oy][r->ox]++;
    }
    int best = RA_NONE, bv = -3000000;
    for (int a = RA_RIGHT; a <= RA_B; a++) {
        EkoRescue c = *r;
        int t0 = c.turns;
        eko_rescue_turn(&c, a);
        if (c.turns == t0 && !c.won) continue;
        int v = search(&c, 3, bot_visits);
        if (v > bv) { bv = v; best = a; }
    }
    return best;
}
