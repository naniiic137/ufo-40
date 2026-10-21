/* ECHO UNIT - the jailbreak: a captured bat's rescue, played turn by turn by
 * a bat still free. Every jail is rolled anew: one to three rooms on the
 * grid, the cell in any outer wall, the keycard anywhere well away from it
 * with guards round it. Every step (or skill) is one turn, then the
 * guards' side takes its turn: shots in flight fly on, grenades go off,
 * every guard acts, and any guard that now sees the bat raises its "!" and
 * acts on it the turn after. Steps go only the four straight ways; crates
 * push like a block puzzle and stop sight and shots.
 *
 * Five kinds of guard, each in its own colour (see eko.h). Guards have
 * health (2; the gatling ones 4). Their shots are things in flight: three
 * tiles a turn, hitting the first thing they come to, the bat or another
 * guard alike, so luring them into each other works. Grenades land on the
 * bat's tile and go off two turns later, three tiles square, guards and all.
 *
 * Each bat has two skills; used, a skill comes back after a few turns, and
 * pressed before then it does nothing at all:
 *   FLINT  A punch: knocks the guard in front two tiles   2   B brace (the shots that turn bounce off)  4
 *            back; one it is knocked into takes 2 too and
 *            flies on, so a row can go down in one shove
 *   VESPER A shoot down the line (2, loud)                3   B roll two tiles                          2
 *   DART   A gun down the line (drops anything, loud)     2   B dash exactly two tiles, through guards  3
 *                                                               (2 to each passed)
 *   SABLE  A dart: asleep three turns (silent)            3   B cloak: unseen up to five turns; B again ends it;
 *                                                               it recharges as long as it was on (four at most)
 * Pressing toward a guard turns the bat to face it; pressing again strikes
 * it (1; Sable's knife 2). Take the card, bring it to the cell door, and
 * the captive walks free. Out of health, the rescuer is caught too. */
#include "eko.h"

const int EKO_RS_HP[OWL_COUNT] = {3, 2, 4, 2};
/* Sable's B, the cloak: its recharge is the turns it was on (set when it ends) */
const int EKO_RS_CD[OWL_COUNT][2] = {{2, 4}, {3, 2}, {2, 3}, {3, 0}};
const char *const EKO_RS_SKILL[OWL_COUNT][2] = {{"PUNCH", "BRACE"}, {"SHOOT", "ROLL"}, {"GUN", "DASH"}, {"DART", "CLOAK"}};
const int EKO_RK_HP[RK_COUNT] = {2, 2, 2, 2, 2, 4};
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

static int manhattan(int x0, int y0, int x1, int y1) { return iabs(x0 - x1) + iabs(y0 - y1); }

static int dir_toward(int dx, int dy) {
    if (iabs(dx) >= iabs(dy)) return dx >= 0 ? 0 : 2;
    return dy >= 0 ? 1 : 3;
}

/* the line a guard looks along: does it reach the bat? (a cloaked Sable is never seen) */
int eko_rescue_sees(const EkoRescue *r, int gi) {
    const EkoRGuard *g = &r->g[gi];
    if (!g->alive || g->state == RGS_SLEEP || r->cloak > 0) return 0;
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

/* a guard onto the bat: the straight way that has nothing but floor (and
 * maybe other guards: they shoot regardless) between it and the bat, or -1 */
static int fire_line(const EkoRescue *r, int gi) {
    const EkoRGuard *g = &r->g[gi];
    if (r->cloak > 0) return -1;
    for (int d = 0; d < 4; d++) {
        int x = g->x, y = g->y;
        for (int k = 0; k < RS_W; k++) {
            x += GDX[d];
            y += GDY[d];
            if (!inside(x, y) || r->tile[y][x] != '.') break;
            if (x == r->ox && y == r->oy) return d;
        }
    }
    return -1;
}

/* breadth-first distances to (tx, ty) over open floor; guards (other than
 * skip) and crates block unless loose; guards keep off the card and the cell */
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
            bool open = loose ? (c == '.' || c == 'K' || c == 'o') : c == '.';
            if (!open) continue;
            if (!loose) { int gi = guard_at(r, nx, ny); if (gi >= 0 && gi != skip) continue; }
            dist[ny][nx] = dist[y][x] + 1;
            qx[t] = nx; qy[t] = ny; t++;
        }
    }
}

static bool guard_can_stand(const EkoRescue *r, int x, int y) {
    return inside(x, y) && r->tile[y][x] == '.' && guard_at(r, x, y) < 0 && !(x == r->ox && y == r->oy);
}

static void guard_step_to(EkoRescue *r, int gi, int tx, int ty) {
    EkoRGuard *g = &r->g[gi];
    static int dist[RS_H][RS_W];
    bfs(r, tx, ty, gi, false, dist);
    int best = -1, bd = dist[(int)g->y][(int)g->x];
    for (int d = 0; d < 4; d++) {
        int nx = g->x + GDX[d], ny = g->y + GDY[d];
        if (!guard_can_stand(r, nx, ny)) continue;
        if (dist[ny][nx] < bd) { bd = dist[ny][nx]; best = d; }
    }
    if (best >= 0) {
        g->x = (int8_t)(g->x + GDX[best]);
        g->y = (int8_t)(g->y + GDY[best]);
        g->dir = (int8_t)best;
    }
}

/* away from the bat, one tile, if any way is further off */
static void guard_step_away(EkoRescue *r, int gi) {
    EkoRGuard *g = &r->g[gi];
    int best = -1, bd = manhattan(g->x, g->y, r->ox, r->oy);
    for (int d = 0; d < 4; d++) {
        int nx = g->x + GDX[d], ny = g->y + GDY[d];
        if (!guard_can_stand(r, nx, ny)) continue;
        int m = manhattan(nx, ny, r->ox, r->oy);
        if (m > bd) { bd = m; best = d; }
    }
    if (best >= 0) {
        g->x = (int8_t)(g->x + GDX[best]);
        g->y = (int8_t)(g->y + GDY[best]);
    }
    g->dir = (int8_t)dir_toward(r->ox - g->x, r->oy - g->y);
}

/* a guard turns onto the bat (a gatling spins up instead) */
static void alert(EkoRescue *r, int gi) {
    EkoRGuard *g = &r->g[gi];
    if (!g->alive || g->state == RGS_SLEEP || r->cloak > 0) return;
    if (g->state == RGS_ALERT || g->state == RGS_SPIN || g->state == RGS_FIRE) return;
    g->flash = 1;
    g->t = 0;
    if (g->kind == RK_GATLING) {
        int d = fire_line(r, gi);
        if (d < 0) { g->state = RGS_ALERT; return; }
        g->dir = (int8_t)d;
        g->state = RGS_SPIN;
    } else g->state = RGS_ALERT;
}

/* damage to a guard; by_bat: the rescuer's doing (else its own side's) */
static void hurt_g(EkoRescue *r, int gi, int dmg, bool by_bat) {
    EkoRGuard *g = &r->g[gi];
    if (!g->alive) return;
    g->hp = (uint8_t)imax(0, g->hp - dmg);
    if (g->hp == 0) {
        g->alive = 0;
        if (by_bat) r->kos++;
        else r->ff++;
        return;
    }
    if (by_bat) alert(r, gi); /* a sleeper stays asleep: stunlock it and strike again */
}

static void noise(EkoRescue *r, int range) {
    r->noise_t = 12;
    r->noise_x = r->ox;
    r->noise_y = r->oy;
    for (int i = 0; i < r->ng; i++) {
        EkoRGuard *g = &r->g[i];
        if (!g->alive || g->state != RGS_POST) continue;
        if (g->kind != RK_GUN && g->kind != RK_KNIFE && g->kind != RK_GREN) continue; /* the watchers keep their posts */
        if (manhattan(g->x, g->y, r->ox, r->oy) > range) continue;
        g->state = RGS_SEEK;
        g->tx = (int8_t)r->ox;
        g->ty = (int8_t)r->oy;
    }
}

/* the bat's shot down the line it faces: the first guard it meets, or -1.
 * The keycard stops it while it still has health (only Vesper's go
 * through once it is spent); gunfire wears it down. */
static int line_target(EkoRescue *r, bool gunfire) {
    int x = r->ox, y = r->oy;
    for (int k = 0; k < RS_W; k++) {
        x += DX[r->dir];
        y += DY[r->dir];
        if (!floorish(r, x, y)) break;
        if (r->tile[y][x] == 'K' && (r->card_hp > 0 || r->owl != OWL_VESPER)) {
            if (gunfire && r->card_hp > 0) r->card_hp--;
            break;
        }
        int gi = guard_at(r, x, y);
        if (gi >= 0) { r->shot_x2 = x; r->shot_y2 = y; return gi; }
    }
    r->shot_x2 = x - DX[r->dir];
    r->shot_y2 = y - DY[r->dir];
    return -1;
}

static void shot_fx(EkoRescue *r) {
    r->shot_t = 8;
    r->shot_x = r->ox;
    r->shot_y = r->oy;
}

/* ---- the guards' side: shots, grenades ---------------------------------------------------- */

static void fire(EkoRescue *r, int gi) {
    EkoRGuard *g = &r->g[gi];
    for (int i = 0; i < RS_MAX_SHOTS; i++)
        if (!r->shot[i].alive) {
            r->shot[i] = (EkoRShot){1, 0, g->x, g->y, g->dir};
            r->gshots++;
            return;
        }
}

/* a shot meets what is on (x, y): true when it is spent there */
static bool shot_hits(EkoRescue *r, EkoRShot *s, int x, int y) {
    if (!inside(x, y)) { s->alive = 0; return true; }
    char c = r->tile[y][x];
    if (c == '#' || c == 'o' || c == 'C') { s->alive = 0; return true; }
    if (c == 'K') { if (r->card_hp > 0) r->card_hp--; s->alive = 0; return true; }
    if (x == r->ox && y == r->oy) {
        s->alive = 0;
        if (!r->brace) { r->hp--; r->hits++; } /* braced: it bounces off */
        return true;
    }
    int j = guard_at(r, x, y);
    if (j >= 0) { hurt_g(r, j, 1, false); s->alive = 0; return true; }
    return false;
}

static void shots_fly(EkoRescue *r) {
    for (int i = 0; i < RS_MAX_SHOTS; i++) {
        EkoRShot *s = &r->shot[i];
        if (!s->alive) continue;
        /* something walked into it where it hung last turn */
        if (s->age > 0 && shot_hits(r, s, s->x, s->y)) continue;
        s->age = 1;
        for (int k = 0; k < RS_SHOT_SPEED && s->alive; k++) {
            int nx = s->x + GDX[(int)s->dir], ny = s->y + GDY[(int)s->dir];
            if (inside(nx, ny)) { s->x = (int8_t)nx; s->y = (int8_t)ny; }
            shot_hits(r, s, nx, ny);
        }
    }
}

static void throw_grenade(EkoRescue *r) {
    for (int i = 0; i < RS_MAX_GRENS; i++)
        if (!r->gren[i].alive) {
            r->gren[i] = (EkoRGren){1, (int8_t)r->ox, (int8_t)r->oy, 2};
            r->grens++;
            return;
        }
}

static void grenades(EkoRescue *r) {
    for (int i = 0; i < RS_MAX_GRENS; i++) {
        EkoRGren *n = &r->gren[i];
        if (!n->alive || --n->fuse > 0) continue;
        n->alive = 0;
        r->boom_t = 14;
        r->boom_x = n->x;
        r->boom_y = n->y;
        /* three tiles square: whoever stands there, either side */
        if (iabs(r->ox - n->x) <= 1 && iabs(r->oy - n->y) <= 1) { r->hp -= 2; r->hits++; }
        for (int j = 0; j < r->ng; j++)
            if (r->g[j].alive && iabs(r->g[j].x - n->x) <= 1 && iabs(r->g[j].y - n->y) <= 1) hurt_g(r, j, 2, false);
    }
}

/* the jetpack: up and down again two tiles off the bat, lined up on it */
static void jet_jump(EkoRescue *r, int gi) {
    EkoRGuard *g = &r->g[gi];
    int best = -1, bd = 9999;
    for (int d = 0; d < 4; d++) {
        int x = r->ox + GDX[d] * 2, y = r->oy + GDY[d] * 2;
        if (!guard_can_stand(r, x, y)) continue;
        int m = manhattan(x, y, g->x, g->y);
        if (m < bd) { bd = m; best = d; }
    }
    if (best < 0) { guard_step_to(r, gi, r->ox, r->oy); return; }
    g->x = (int8_t)(r->ox + GDX[best] * 2);
    g->y = (int8_t)(r->oy + GDY[best] * 2);
    g->dir = (int8_t)((best + 2) & 3);
}

static void guard_act(EkoRescue *r, int gi) {
    EkoRGuard *g = &r->g[gi];
    int dist = manhattan(g->x, g->y, r->ox, r->oy);
    if (g->flash) g->flash--;
    switch (g->state) {
    case RGS_SLEEP:
        if (--g->sleep <= 0) { g->state = RGS_POST; g->sleep = 0; }
        return;
    case RGS_SEEK:
        if (g->x == g->tx && g->y == g->ty) g->state = RGS_POST;
        else {
            int ox = g->x, oy = g->y;
            guard_step_to(r, gi, g->tx, g->ty);
            if ((g->x == ox && g->y == oy) || manhattan(g->x, g->y, g->tx, g->ty) <= 1) g->state = RGS_POST;
        }
        return;
    case RGS_SPIN:
        /* the gun winds up this turn; the volley comes the next four */
        g->state = RGS_FIRE;
        g->t = 4;
        return;
    case RGS_FIRE:
        fire(r, gi);
        if (--g->t <= 0) { g->state = RGS_POST; g->cd = 2; }
        return;
    case RGS_ALERT:
        switch (g->kind) {
        case RK_GUN: {
            /* closes in, and fires after each move when it has a line */
            if (dist > 1) guard_step_to(r, gi, r->ox, r->oy);
            int d = fire_line(r, gi);
            if (d >= 0) { g->dir = (int8_t)d; fire(r, gi); }
            break;
        }
        case RK_KNIFE:
            if (dist == 1) {
                g->dir = (int8_t)dir_toward(r->ox - g->x, r->oy - g->y);
                r->hp--;
                r->hits++;
                r->stabs++;
            } else guard_step_to(r, gi, r->ox, r->oy);
            break;
        case RK_SENTRY: {
            /* stays put; one shot at a time along any open line */
            if (g->cd > 0) { g->cd--; break; }
            int d = fire_line(r, gi);
            if (d >= 0) { g->dir = (int8_t)d; fire(r, gi); g->cd = 1; }
            break;
        }
        case RK_GREN:
            if (g->cd > 0) g->cd--;
            if (dist <= 2) guard_step_away(r, gi);
            else if (g->cd == 0 && dist <= 6) { throw_grenade(r); g->cd = 2; g->dir = (int8_t)dir_toward(r->ox - g->x, r->oy - g->y); }
            else if (dist > 6) guard_step_to(r, gi, r->ox, r->oy);
            break;
        case RK_JET: {
            /* a shot, then a jump to two tiles off the bat, and so on */
            int d = g->t == 0 ? fire_line(r, gi) : -1;
            if (d >= 0) { g->dir = (int8_t)d; fire(r, gi); g->t = 1; }
            else { jet_jump(r, gi); g->t = 0; }
            break;
        }
        default: {
            /* a gatling that lost its line: it turns to find one */
            int d = fire_line(r, gi);
            if (d >= 0) { g->dir = (int8_t)d; g->state = RGS_SPIN; }
            break;
        }
        }
        return;
    default: /* on its beat or watch */
        switch (g->kind) {
        case RK_GUN: case RK_KNIFE: case RK_GREN: {
            int nx = g->x + GDX[(int)g->dir], ny = g->y + GDY[(int)g->dir];
            if (!guard_can_stand(r, nx, ny)) g->dir = (int8_t)((g->dir + 2) & 3);
            else { g->x = (int8_t)nx; g->y = (int8_t)ny; }
            break;
        }
        case RK_JET: case RK_GATLING:
            if (g->cd > 0) g->cd--;
            else if ((r->turns + gi) % 2 == 0) g->dir = (int8_t)((g->dir + 1) & 3);
            break;
        default: break;
        }
        return;
    }
}

static void enemies_turn(EkoRescue *r) {
    shots_fly(r);
    grenades(r);
    /* the cloak: nobody keeps track of her */
    if (r->cloak > 0)
        for (int i = 0; i < r->ng; i++)
            if (r->g[i].alive && (r->g[i].state == RGS_ALERT || r->g[i].state == RGS_SPIN || r->g[i].state == RGS_FIRE)) {
                r->g[i].state = RGS_POST;
                r->g[i].t = 0;
            }
    for (int i = 0; i < r->ng; i++)
        if (r->g[i].alive) guard_act(r, i);
    /* who sees the bat now: "!" now, and it acts on it next turn */
    for (int i = 0; i < r->ng; i++) {
        EkoRGuard *g = &r->g[i];
        if (!g->alive || (g->state != RGS_POST && g->state != RGS_SEEK)) continue;
        if (g->kind == RK_GATLING && g->cd > 0) continue; /* still cooling its barrels */
        if (eko_rescue_sees(r, i)) alert(r, i);
    }
}

/* ---- the bat ------------------------------------------------------------------------------- */

/* one step (or a push, or a blow): false if nothing happened (no turn taken) */
static bool step(EkoRescue *r, int d) {
    bool facing = r->dir == d;
    r->dir = d;
    int nx = r->ox + DX[d], ny = r->oy + DY[d];
    if (!inside(nx, ny)) return false;
    char c = r->tile[ny][nx];
    int gi = guard_at(r, nx, ny);
    if (gi >= 0) {
        /* toward a guard: the first press turns to face it, the next strikes */
        if (!facing) return false;
        hurt_g(r, gi, r->owl == OWL_SABLE ? 2 : 1, true);
        r->strikes++;
        return true;
    }
    if (c == 'C') {
        if (!r->card) return false;
        r->won = true;
        r->ox = nx; r->oy = ny;
        return true;
    }
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

/* Flint's shove: guard gi flies up to two tiles along d; one it runs into
 * is knocked on in turn (and so on down a row), then both take 2 */
static void knock(EkoRescue *r, int gi, int d, int depth) {
    for (int s = 0; s < 2 && r->g[gi].alive; s++) {
        EkoRGuard *g = &r->g[gi];
        int nx = g->x + DX[d], ny = g->y + DY[d];
        int j = guard_at(r, nx, ny);
        if (j >= 0) {
            if (depth < RS_MAX_GUARDS) knock(r, j, d, depth + 1);
            hurt_g(r, gi, 2, true);
            hurt_g(r, j, 2, true);
            return;
        }
        if (!inside(nx, ny) || r->tile[ny][nx] != '.' || (nx == r->ox && ny == r->oy)) return;
        g->x = (int8_t)nx;
        g->y = (int8_t)ny;
    }
}

/* the bat's skill k (0 on A, 1 on B): false if it could not be used */
static bool skill(EkoRescue *r, int k) {
    int fx = r->ox + DX[r->dir], fy = r->oy + DY[r->dir];
    switch (r->owl * 2 + k) {
    case OWL_FLINT * 2: {
        /* the punch (a swing at nothing still costs the turn) */
        int gi = guard_at(r, fx, fy);
        if (gi >= 0) {
            knock(r, gi, r->dir, 0);
            hurt_g(r, gi, 2, true);
        }
        return true;
    }
    case OWL_FLINT * 2 + 1:
        r->brace = true; /* the shots this turn bounce off */
        return true;
    case OWL_VESPER * 2: {
        int gi = line_target(r, true);
        shot_fx(r);
        if (gi >= 0) hurt_g(r, gi, 2, true);
        noise(r, 5);
        return true;
    }
    case OWL_DART * 2: {
        int gi = line_target(r, true);
        shot_fx(r);
        if (gi >= 0) hurt_g(r, gi, 99, true); /* one shot drops anything */
        noise(r, 5);
        return true;
    }
    case OWL_VESPER * 2 + 1:
        /* a roll: two tiles on, both clear */
        if (floorish(r, fx, fy) && guard_at(r, fx, fy) < 0 && floorish(r, fx + DX[r->dir], fy + DY[r->dir]) &&
            guard_at(r, fx + DX[r->dir], fy + DY[r->dir]) < 0) {
            int d = r->dir;
            step(r, d);
            step(r, d);
            return true;
        }
        return false;
    case OWL_DART * 2 + 1: {
        /* the dash: exactly two tiles, through a guard in the way (2 to it) */
        int tx = fx + DX[r->dir], ty = fy + DY[r->dir];
        if (!floorish(r, fx, fy)) return false;
        bool land = (floorish(r, tx, ty) && guard_at(r, tx, ty) < 0) || (inside(tx, ty) && r->tile[ty][tx] == 'C' && r->card);
        if (!land) return false;
        shot_fx(r);
        int gi = guard_at(r, fx, fy);
        if (gi >= 0) hurt_g(r, gi, 2, true);
        if (r->tile[fy][fx] == 'K') { r->card = true; r->tile[fy][fx] = '.'; }
        r->shot_x2 = tx;
        r->shot_y2 = ty;
        if (r->tile[ty][tx] == 'C') r->won = true;
        r->ox = tx;
        r->oy = ty;
        if (r->tile[ty][tx] == 'K') { r->card = true; r->tile[ty][tx] = '.'; }
        return true;
    }
    case OWL_SABLE * 2: {
        int gi = line_target(r, false);
        shot_fx(r);
        /* asleep: three Zs now, one gone each turn after, awake once they are all gone */
        if (gi >= 0) { r->g[gi].state = RGS_SLEEP; r->g[gi].sleep = RS_SLEEP_T + 1; r->g[gi].flash = 0; }
        return true;
    }
    case OWL_SABLE * 2 + 1:
        /* the cloak: unseen, and whoever was onto her loses her */
        r->cloak = RS_CLOAK_T;
        r->cloak_used = 0;
        for (int i = 0; i < r->ng; i++)
            if (r->g[i].alive && (r->g[i].state == RGS_ALERT || r->g[i].state == RGS_SPIN || r->g[i].state == RGS_FIRE)) {
                r->g[i].state = RGS_POST;
                r->g[i].t = 0;
                r->g[i].flash = 0;
            }
        return true;
    default: return false;
    }
}

static void cloak_off(EkoRescue *r) {
    r->cloak = 0;
    r->cd[1] = iclamp(r->cloak_used, 1, RS_CLOAK_CD);
}

void eko_rescue_turn(EkoRescue *r, int action) {
    if (r->won || r->lost || action == RA_NONE) return;
    bool took = false;
    int used = -1;
    r->brace = false;
    if (action >= RA_RIGHT && action <= RA_UP) {
        took = step(r, action);
    } else if (action == RA_A || action == RA_B) {
        int k = action == RA_A ? 0 : 1;
        if (k == 1 && r->owl == OWL_SABLE && r->cloak > 0) { cloak_off(r); return; } /* B again: she shows (no turn) */
        if (r->cd[k] > 0) return; /* not back yet: nothing happens */
        took = skill(r, k);
        if (took) used = k;
    }
    if (!took) return;
    r->turns++;
    /* the skills count down; the one just used starts over */
    for (int k = 0; k < 2; k++) if (r->cd[k] > 0) r->cd[k]--;
    if (used >= 0) r->cd[used] = EKO_RS_CD[r->owl][used];
    if (r->won) return;
    enemies_turn(r);
    if (r->cloak > 0) {
        r->cloak_used++;
        if (--r->cloak == 0) cloak_off(r);
    }
    if (r->hp <= 0) r->lost = true;
}

/* ---- rolling a jail -------------------------------------------------------------------- */

static void add_g(EkoRescue *r, int kind, int x, int y, int dir) {
    if (r->ng >= RS_MAX_GUARDS) return;
    EkoRGuard *g = &r->g[r->ng++];
    memset(g, 0, sizeof *g);
    g->alive = 1;
    g->kind = (uint8_t)kind;
    g->hp = (uint8_t)EKO_RK_HP[kind];
    g->state = RGS_POST;
    g->x = g->hx = (int8_t)x;
    g->y = g->hy = (int8_t)y;
    g->dir = g->hdir = (int8_t)dir;
}

static bool near_door(const EkoRescue *r, int x, int y, int reach) {
    for (int i = 0; i < r->ndoor; i++)
        if (iabs(r->door_x[i] - x) <= reach && iabs(r->door_y[i] - y) <= reach) return true;
    return false;
}

static bool free_spot(const EkoRescue *r, int x, int y) {
    return inside(x, y) && r->tile[y][x] == '.' && guard_at(r, x, y) < 0 && !(x == r->start_x && y == r->start_y);
}

int eko_rescue_check(const EkoRescue *r) {
    static int d2[RS_H][RS_W];
    if (r->tile[r->cell_y][r->cell_x] != 'C' || r->tile[r->key_y][r->key_x] != 'K') return 9;
    /* the guards round the card stand in the way: count them as open for the path */
    bfs(r, r->key_x, r->key_y, -1, true, d2);
    if (d2[r->start_y][r->start_x] >= 9999) return 1;
    /* the way back to the cell door */
    bfs(r, r->cell_x, r->cell_y, -1, true, d2);
    if (d2[r->key_y][r->key_x] >= 9999) return 2;
    /* nobody sees the start, nobody stands near it */
    for (int i = 0; i < r->ng; i++) {
        if (eko_rescue_sees(r, i)) return 3;
        if (manhattan(r->g[i].x, r->g[i].y, r->start_x, r->start_y) <= 3) return 4;
    }
    /* the card has guards round it */
    int round = 0;
    for (int d = 0; d < 4; d++) round += guard_at(r, r->key_x + GDX[d], r->key_y + GDY[d]) >= 0;
    if (round < 2) return 5;
    /* the bat can always make a first step: a free tile beside it */
    int free = 0;
    for (int d = 0; d < 4; d++) {
        int x = r->start_x + GDX[d], y = r->start_y + GDY[d];
        free += floorish(r, x, y) && guard_at(r, x, y) < 0;
    }
    if (!free) return 6;
    return 0;
}

/* a wall across the grid (or part of it), with doorways */
static void divider(EkoRescue *r, Rng *rng, bool vert, int at, int from, int to, int doors) {
    for (int k = from; k <= to; k++) {
        if (vert) r->tile[k][at] = '#';
        else r->tile[at][k] = '#';
    }
    for (int n = 0; n < doors && r->ndoor < 6; n++) {
        int k = rng_range(rng, from, to);
        int x = vert ? at : k, y = vert ? k : at;
        if (r->tile[y][x] == '.') continue;
        r->tile[y][x] = '.';
        r->door_x[r->ndoor] = (int8_t)x;
        r->door_y[r->ndoor] = (int8_t)y;
        r->ndoor++;
    }
}

static int roll_kind(Rng *rng, bool ring) {
    int v = rng_range(rng, 0, 99);
    if (ring) return v < 35 ? RK_SENTRY : v < 55 ? RK_GATLING : v < 75 ? RK_JET : v < 90 ? RK_KNIFE : RK_GUN;
    return v < 30 ? RK_GUN : v < 50 ? RK_KNIFE : v < 72 ? RK_GREN : v < 88 ? RK_SENTRY : RK_JET;
}

int eko_rescue_gen(EkoRescue *r, uint32_t seed, int owl, int captive) {
    Rng rng;
    rng_seed(&rng, seed * 2654435761u + 77);
    for (int attempt = 0; attempt < 400; attempt++) {
        memset(r, 0, sizeof *r);
        r->owl = owl;
        r->captive = captive;
        r->seed = (int)seed;
        r->hp = EKO_RS_HP[owl];
        r->dir = RA_RIGHT;
        r->card_hp = RS_CARD_HP;
        r->start_x = r->start_y = -1;
        for (int y = 0; y < RS_H; y++)
            for (int x = 0; x < RS_W; x++)
                r->tile[y][x] = (x == 0 || y == 0 || x == RS_W - 1 || y == RS_H - 1) ? '#' : '.';
        /* one big room, two, or three */
        int roll = rng_range(&rng, 0, 99);
        r->rooms = roll < 25 ? 1 : roll < 62 ? 2 : 3;
        if (r->rooms >= 2) {
            bool vert = rng_range(&rng, 0, 2) != 0;
            int at = vert ? rng_range(&rng, 4, RS_W - 5) : rng_range(&rng, 3, RS_H - 4);
            divider(r, &rng, vert, at, 1, vert ? RS_H - 2 : RS_W - 2, rng_range(&rng, 1, 2));
            if (r->rooms == 3) {
                /* the second wall splits one side of the first */
                bool side = rng_range(&rng, 0, 1);
                int at2 = vert ? rng_range(&rng, 3, RS_H - 4) : rng_range(&rng, 4, RS_W - 5);
                bool clash = false;
                for (int i = 0; i < r->ndoor; i++) clash |= vert ? r->door_y[i] == at2 : r->door_x[i] == at2;
                if (clash) continue;
                if (vert) divider(r, &rng, false, at2, side ? at + 1 : 1, side ? RS_W - 2 : at - 1, 1);
                else divider(r, &rng, true, at2, side ? at + 1 : 1, side ? RS_H - 2 : at - 1, 1);
            }
        }
        /* a few pillars (never by a doorway) */
        for (int k = 0, n = rng_range(&rng, 0, 3); k < n; k++) {
            int x = rng_range(&rng, 2, RS_W - 3), y = rng_range(&rng, 2, RS_H - 3);
            if (!near_door(r, x, y, 1)) r->tile[y][x] = '#';
        }
        /* the cell, in any outer wall */
        for (int tries = 0; tries < 30 && !r->cell_x && !r->cell_y; tries++) {
            int side = rng_range(&rng, 0, 3);
            int x, y, ix, iy;
            if (side == 0) { x = 0; y = rng_range(&rng, 1, RS_H - 2); ix = 1; iy = y; }
            else if (side == 1) { x = RS_W - 1; y = rng_range(&rng, 1, RS_H - 2); ix = x - 1; iy = y; }
            else if (side == 2) { y = 0; x = rng_range(&rng, 1, RS_W - 2); ix = x; iy = 1; }
            else { y = RS_H - 1; x = rng_range(&rng, 1, RS_W - 2); ix = x; iy = y - 1; }
            if (r->tile[iy][ix] != '.') continue;
            r->tile[y][x] = 'C';
            r->cell_x = x;
            r->cell_y = y;
        }
        if (!r->cell_x && !r->cell_y) continue;
        /* the card, well away from the cell, with room round it */
        for (int tries = 0; tries < 60 && !r->key_x; tries++) {
            int x = rng_range(&rng, 2, RS_W - 3), y = rng_range(&rng, 2, RS_H - 3);
            if (r->tile[y][x] != '.' || manhattan(x, y, r->cell_x, r->cell_y) < 9 || near_door(r, x, y, 2)) continue;
            bool room = true;
            for (int d = 0; d < 4; d++) room &= r->tile[y + GDY[d]][x + GDX[d]] == '.';
            if (!room) continue;
            r->key_x = x;
            r->key_y = y;
            r->tile[y][x] = 'K';
        }
        if (!r->key_x) continue;
        /* the bat comes in well clear of the card */
        for (int tries = 0; tries < 60 && r->start_x < 0; tries++) {
            int x = rng_range(&rng, 1, RS_W - 2), y = rng_range(&rng, 1, RS_H - 2);
            if (r->tile[y][x] != '.' || manhattan(x, y, r->key_x, r->key_y) < 7 || manhattan(x, y, r->cell_x, r->cell_y) < 2) continue;
            r->start_x = x;
            r->start_y = y;
        }
        if (r->start_x < 0) continue;
        r->ox = r->start_x;
        r->oy = r->start_y;
        /* the guards round the card, looking out */
        int around = rng_range(&rng, 2, 3), first = rng_range(&rng, 0, 3);
        for (int k = 0; k < around; k++) {
            int d = (first + k) & 3;
            add_g(r, roll_kind(&rng, true), r->key_x + GDX[d], r->key_y + GDY[d], d);
        }
        /* and some about the place */
        for (int k = 0, n = rng_range(&rng, 1, 3); k < n; k++) {
            int x = rng_range(&rng, 1, RS_W - 2), y = rng_range(&rng, 1, RS_H - 2);
            if (!free_spot(r, x, y) || manhattan(x, y, r->start_x, r->start_y) < 5 || manhattan(x, y, r->key_x, r->key_y) <= 1) continue;
            add_g(r, roll_kind(&rng, false), x, y, rng_range(&rng, 0, 3));
        }
        /* crates (never by a doorway, the card or the cell) */
        for (int k = 0, n = rng_range(&rng, 3, 6); k < n; k++) {
            int x = rng_range(&rng, 2, RS_W - 3), y = rng_range(&rng, 2, RS_H - 3);
            if (!free_spot(r, x, y) || near_door(r, x, y, 1) || manhattan(x, y, r->key_x, r->key_y) <= 1) continue;
            if (manhattan(x, y, r->cell_x, r->cell_y) <= 2) continue;
            r->tile[y][x] = 'o';
        }
        if (eko_rescue_check(r) == 0) return attempt;
    }
    return -1;
}

/* ---- the demo player's look-ahead --------------------------------------------------------- */

/* the ways to the card and to the cell door, crates counted as open: walls
 * never move, so these are worked out once a decision */
static int bot_dkey[RS_H][RS_W], bot_dcell[RS_H][RS_W];
/* what a point of health is worth to it: dear at first, cheaper once it has
 * gone a long while without getting any nearer (a knock for headway) */
static int bot_hp_w = 3000;

/* trouble already on its way: shots that will reach the bat next turn,
 * grenades about to go off round it, gatlings lined up on it */
static int danger(const EkoRescue *r) {
    int d = 0;
    for (int i = 0; i < RS_MAX_SHOTS; i++) {
        const EkoRShot *s = &r->shot[i];
        if (!s->alive) continue;
        int x = s->x, y = s->y;
        for (int k = 0; k <= RS_SHOT_SPEED; k++) {
            if (k > 0) { x += GDX[(int)s->dir]; y += GDY[(int)s->dir]; }
            if (!inside(x, y) || (k > 0 && r->tile[y][x] != '.')) break;
            if (x == r->ox && y == r->oy) { if (k > 0 || s->age) d += 2000; break; }
            if (k > 0 && guard_at(r, x, y) >= 0) break;
        }
    }
    for (int i = 0; i < RS_MAX_GRENS; i++)
        if (r->gren[i].alive && iabs(r->gren[i].x - r->ox) <= 1 && iabs(r->gren[i].y - r->oy) <= 1) d += r->gren[i].fuse <= 1 ? 4000 : 1500;
    for (int i = 0; i < r->ng; i++) {
        const EkoRGuard *g = &r->g[i];
        if (!g->alive) continue;
        if (g->state == RGS_SPIN || g->state == RGS_FIRE) {
            int x = g->x, y = g->y;
            for (int k = 0; k < RS_W; k++) {
                x += GDX[(int)g->dir]; y += GDY[(int)g->dir];
                if (!inside(x, y) || r->tile[y][x] != '.') break;
                if (x == r->ox && y == r->oy) { d += 1500; break; }
            }
        }
        if (g->state == RGS_ALERT && g->kind == RK_KNIFE && manhattan(g->x, g->y, r->ox, r->oy) <= 2) d += 900;
        if (g->state == RGS_ALERT && (g->kind == RK_GUN || g->kind == RK_SENTRY || g->kind == RK_JET) && fire_line(r, i) >= 0) d += 500;
    }
    return d;
}

static int eval(const EkoRescue *r, uint8_t visits[RS_H][RS_W]) {
    if (r->lost) return -1000000;
    if (r->won) return 1000000 - r->turns;
    /* a cell door with no open floor before it shuts the way out */
    int open = 0;
    for (int d = 0; d < 4; d++) open += floorish(r, r->cell_x + GDX[d], r->cell_y + GDY[d]);
    if (!open) return -900000;
    /* nor any crate in a doorway between the rooms, or just beside it */
    int jam = 0;
    for (int i = 0; i < r->ndoor; i++)
        for (int d = -1; d < 4; d++) {
            int x = r->door_x[i] + (d >= 0 ? GDX[d] : 0), y = r->door_y[i] + (d >= 0 ? GDY[d] : 0);
            jam += inside(x, y) && r->tile[y][x] == 'o';
        }
    int s = (r->hp >= 2 ? (r->hp - 1) * bot_hp_w + 3000 : r->hp * 3000) + (r->kos + r->ff) * 150;
    int d = r->card ? bot_dcell[r->oy][r->ox] : bot_dkey[r->oy][r->ox];
    if (d >= 9999) d = 60;
    s -= d * 50;
    if (r->card) s += 20000;
    for (int i = 0; i < r->ng; i++) {
        const EkoRGuard *g = &r->g[i];
        if (!g->alive) continue;
        if (g->state == RGS_ALERT || g->state == RGS_FIRE || g->state == RGS_SPIN) s -= 120;
        s -= g->hp * 20;
    }
    s -= danger(r) * (bot_hp_w >= 3000 ? 1 : 0);
    s -= visits[r->oy][r->ox] * 60;
    s -= (r->cd[0] + r->cd[1]) * 15;
    s -= jam * 6000;
    return s;
}

/* the moves worth trying from here. Codes: 1..4 steps, 5 / 6 the skills as
 * faced, 7 + 2 * (dir - 1) + k the skill k after turning to dir (pressing
 * toward a wall or a guard turns the bat without taking a turn), 15 + (dir - 1)
 * a turn to face a guard and then a blow at it */
static int moves(const EkoRescue *r, int *out) {
    int n = 0;
    for (int a = RA_RIGHT; a <= RA_UP; a++) out[n++] = a;
    if (r->cd[0] == 0) out[n++] = RA_A;
    if (r->cd[1] == 0 && !r->cloak) out[n++] = RA_B;
    for (int d = RA_RIGHT; d <= RA_UP; d++) {
        if (d == r->dir) continue;
        EkoRescue c = *r;
        eko_rescue_turn(&c, d);
        if (c.turns != r->turns || c.won) continue; /* that way is a step, not a turn */
        if (r->cd[0] == 0) out[n++] = 7 + 2 * (d - 1);
        if (r->cd[1] == 0 && !r->cloak) out[n++] = 8 + 2 * (d - 1);
        if (guard_at(r, r->ox + DX[d], r->oy + DY[d]) >= 0) out[n++] = 15 + (d - 1);
    }
    return n;
}

static void play_move(EkoRescue *r, int m) {
    if (m >= 15) {
        r->dir = 1 + (m - 15);
        eko_rescue_turn(r, 1 + (m - 15));
    } else if (m >= 7) {
        r->dir = 1 + (m - 7) / 2;
        eko_rescue_turn(r, (m - 7) % 2 ? RA_B : RA_A);
    } else eko_rescue_turn(r, m);
}

static int search(const EkoRescue *r, int depth, uint8_t visits[RS_H][RS_W]) {
    if (depth == 0 || r->won || r->lost) return eval(r, visits);
    int best = -2000000;
    int mv[24], n = moves(r, mv);
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
    bfs(r, r->cell_x, r->cell_y, -1, true, bot_dcell);
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
    int mv[24], n = moves(r, mv);
    for (int i = 0; i < n; i++) {
        EkoRescue c = *r;
        play_move(&c, mv[i]);
        if (c.turns == r->turns && !c.won) continue;
        /* the look-ahead's best, plus a little of how things stand right
         * after this move: of two ways to the same end, the sooner one */
        int v = search(&c, 3, bot_visits) + eval(&c, bot_visits) / 4;
        if (v > bv) { bv = v; best = mv[i]; }
    }
    if (best == 0) {
        /* nothing takes a turn from here but a skill still cooling: any step will do */
        return RA_NONE;
    }
    /* a skill or a blow after a turn: the turn first (it takes no turn), the rest on the next press */
    if (best >= 15) return 1 + (best - 15);
    if (best >= 7) return 1 + (best - 7) / 2;
    return best;
}
