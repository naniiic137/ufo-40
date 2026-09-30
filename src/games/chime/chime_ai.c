/* CHIME CIRCUIT - the pilots the machine flies.
 *
 * A pilot follows the track the way water runs downhill: from its ship it
 * walks the distance field to the next zone on its line, cell by cell, and
 * aims at a point a little way along that path. It slows for a bend it can
 * see coming and in tight spots, and steers and thrusts to hold that
 * velocity, as a player taps and holds the buttons. Before it commits, it
 * flies the next 24 frames forward on the real flight model; if that would
 * touch a wall, it tries again more slowly.
 *
 * CPU pilots are slower than a good player: each keeps to its own speed
 * limit for the race and only looks again every third frame. They have
 * moods: calm, jostling (a slash for anyone in reach) and hunting (they go
 * for a nearby player, even turning back to do it). Everyone jostles in the
 * opening scrum, and on TWIN FLUE they would rather wreck ships than race.
 *
 * The demo pilot (bot = true) flies player one's ship for the tests with
 * the same pilot at full pace, on the track's fastest line. */
#include "chime.h"

#define LOOK 24        /* frames flown forward to check a plan */
#define PATH 24        /* cells of path looked along */
#define CARROT 5       /* the cell aimed at */
#define FAR 16         /* the cell that shows a bend coming */
#define SCRUM 300      /* frames of the opening scrum */
#define HUNT_R 110     /* px: a player this close can be hunted */
#define FLUE 5         /* TWIN FLUE's track index */

static int PX(int32_t v) { return (int)(v >> 8); }
int chm_ai_debug; /* tests: print the next plan */

void chm_ai_mood(ChmRace *r, int i) {
    ChmShip *s = &r->s[i];
    int roll = rng_range(&r->rng, 0, 99);
    bool mean = r->track == FLUE;
    s->aggro = roll < (mean ? 40 : 10) ? AG_HUNT : roll < (mean ? 80 : 45) ? AG_JOSTLE : AG_CALM;
    s->aggro_t = (uint16_t)rng_range(&r->rng, 360, 720);
    s->prey = 255;
}

/* the buttons that hold a velocity */
static unsigned hold(const ChmFlight *f, int32_t dvx, int32_t dvy) {
    unsigned a = 0;
    if (f->vx < dvx - 10) a |= CHF_RIGHT;
    else if (f->vx > dvx + 10) a |= CHF_LEFT;
    if (f->vy + CHM_TUNE.gravity > dvy) a |= CHF_THRUST;
    return a;
}

static int isqrt(int v) {
    if (v <= 0) return 0;
    int r = 1;
    while ((r + 1) * (r + 1) <= v) r++;
    return r;
}

static bool nav_ok(int i, int j) { return i >= 0 && j >= 0 && i < CHM_NW && j < CHM_NH && chm_map.nav[j][i]; }
static uint16_t fld(int zone, int i, int j) { return chm_map.field[zone][j * CHM_NW + i]; }

/* Walk downhill from (x, y) toward zone z, then on toward zone zn; the
 * cells' centres go into px, py, and the cell it started from into *si, *sj
 * (the nearest cell a ship fits in, when (x, y) is up against a wall).
 * Returns how many. */
static int descend(int32_t x, int32_t y, int z, int zn, int n, int16_t *px, int16_t *py, int *si, int *sj) {
    int i = PX(x) / CHM_NC, j = (PX(y) - CHM_OY) / CHM_NC;
    if (!nav_ok(i, j)) {
        int bi = -1, bj = -1;
        uint16_t bv = CHM_FAR;
        for (int dj = -2; dj <= 2; dj++)
            for (int di = -2; di <= 2; di++)
                if (nav_ok(i + di, j + dj) && fld(z, i + di, j + dj) < bv) {
                    bv = fld(z, i + di, j + dj);
                    bi = i + di;
                    bj = j + dj;
                }
        if (bi < 0) return 0;
        i = bi;
        j = bj;
    }
    *si = i;
    *sj = j;
    int zone = z, k = 0;
    while (k < n) {
        uint16_t cur = fld(zone, i, j);
        if (cur == 0 && zone != zn) {
            zone = zn;
            cur = fld(zone, i, j);
        }
        int bi = -1, bj = -1;
        uint16_t bv = cur;
        for (int dj = -1; dj <= 1; dj++)
            for (int di = -1; di <= 1; di++) {
                if ((!di && !dj) || !nav_ok(i + di, j + dj)) continue;
                if (di && dj && (!nav_ok(i + di, j) || !nav_ok(i, j + dj))) continue;
                uint16_t v = fld(zone, i + di, j + dj);
                if (v < bv) { bv = v; bi = i + di; bj = j + dj; }
            }
        if (bi < 0) break;
        i = bi;
        j = bj;
        px[k] = (int16_t)(i * CHM_NC + CHM_NC / 2);
        py[k] = (int16_t)(CHM_OY + j * CHM_NC + CHM_NC / 2);
        k++;
    }
    return k;
}

/* can a ship fly straight from a to b (px) with a pixel to spare? */
static bool clear_line(int ax, int ay, int bx, int by) {
    int dx = bx - ax, dy = by - ay, n = imax(iabs(dx), iabs(dy)) / 2 + 1;
    for (int k = 1; k <= n; k++) {
        int x = ax + dx * k / n, y = ay + dy * k / n;
        if (chm_flight_blocked(5, chm_solid, &chm_map, x * CHF_ONE, y * CHF_ONE)) return false;
    }
    return true;
}

/* the velocity a pilot wants at (x, y): along the path, slower into bends
 * and tight spots; pace is its top speed */
static void flow(int32_t x, int32_t y, int z, int zn, int pace, int32_t *dvx, int32_t *dvy) {
    int16_t px[PATH], py[PATH];
    int si = 0, sj = 0;
    int n = descend(x, y, z, zn, PATH, px, py, &si, &sj);
    *dvx = *dvy = 0;
    if (n == 0) return;
    int cx = PX(x), cy = PX(y);
    bool pinched = !nav_ok(cx / CHM_NC, (cy - CHM_OY) / CHM_NC);
    /* aim at the furthest point of the path in plain sight, so as not to
     * cut a corner; squeezed against a wall, first back to the open */
    int ox = pinched ? si * CHM_NC + CHM_NC / 2 : cx, oy = pinched ? CHM_OY + sj * CHM_NC + CHM_NC / 2 : cy;
    int L = imin(n - 1, CARROT), F = imin(n - 1, FAR);
    while (L > 0 && !clear_line(ox, oy, px[L], py[L])) L--;
    int tx = pinched ? ox : px[L], ty = pinched ? oy : py[L];
    int ax = tx - cx, ay = ty - cy, bx = px[F] - px[L], by = py[F] - py[L];
    int la = isqrt(ax * ax + ay * ay), lb = isqrt(bx * bx + by * by);
    if (la == 0) {
        ax = px[L] - cx;
        ay = py[L] - cy;
        la = isqrt(ax * ax + ay * ay);
        if (la == 0) return;
    }
    int cosang = lb ? (ax * bx + ay * by) * 128 / (la * lb) : 128;
    int speed = pace * (40 + 60 * imax(0, cosang) / 128) / 100;
    int cl = chm_map.clear[(py[L] - CHM_OY) / CHM_NC][px[L] / CHM_NC];
    if (cl <= 1) speed = speed * 3 / 5;
    else if (cl <= 2) speed = speed * 4 / 5;
    speed = imax(speed, imin(pace, 150));
    *dvx = ax * speed / la;
    *dvy = ay * speed / la;
}

/* frames flown safely following the path at this pace */
static int try_pace(const ChmShip *s, int z, int zn, int pace, int half, const int32_t *prey) {
    ChmFlight f = s->f;
    int32_t dvx = 0, dvy = 0;
    for (int k = 0; k < LOOK; k++) {
        if ((k & 3) == 0) {
            if (prey) {
                int ax = PX(prey[0] - f.x), ay = PX(prey[1] - f.y), la = imax(1, isqrt(ax * ax + ay * ay));
                dvx = ax * pace / la;
                dvy = ay * pace / la;
            } else {
                flow(f.x, f.y, z, zn, pace, &dvx, &dvy);
            }
        }
        chm_flight_control(&f, &CHM_TUNE, hold(&f, dvx, dvy));
        f.x += f.vx;
        f.y += f.vy;
        if (chm_flight_blocked(half, chm_solid, &chm_map, f.x, f.y)) return k;
    }
    return LOOK;
}

/* the ship this one's slash would reach now, or -1 */
static int in_reach(const ChmRace *r, int i, int face, int only) {
    const ChmShip *s = &r->s[i];
    int cx = PX(s->f.x), cy = PX(s->f.y);
    for (int j = 0; j < CHM_SHIPS; j++) {
        const ChmShip *t = &r->s[j];
        if (j == i || !t->alive || t->finished || t->parked || (only >= 0 && j != only)) continue;
        int dx = (PX(t->f.x) - cx) * face, dy = PX(t->f.y) - cy;
        if (dx >= -2 && dx <= 20 && dy >= -11 && dy <= 11) return j;
    }
    return -1;
}

/* the nearest player (or, on TWIN FLUE, the nearest ship of any kind) */
static int nearest_player(const ChmRace *r, int i, int radius) {
    const ChmShip *s = &r->s[i];
    int best = -1, bd = radius * radius + 1;
    for (int j = 0; j < CHM_SHIPS; j++) {
        const ChmShip *t = &r->s[j];
        if (j == i || (!t->human && r->track != FLUE) || !t->alive || t->finished || t->parked) continue;
        int dx = PX(t->f.x - s->f.x), dy = PX(t->f.y - s->f.y), d = dx * dx + dy * dy;
        if (d < bd) { bd = d; best = j; }
    }
    return best;
}

static const uint8_t PACES[4] = {100, 70, 45, 25}; /* per cent of top speed */

unsigned chm_ai(ChmRace *r, int i, bool bot) {
    ChmShip *s = &r->s[i];
    const ChmMap *m = &chm_map;
    if (!s->alive || s->finished || s->parked) return 0;
    int n = m->route_len[s->route];
    if (!n) return 0;
    int z = m->route[s->route][s->rz % n], zn = m->route[s->route][(s->rz + 1) % n];
    int half = chm_flight_blocked(5, chm_solid, m, s->f.x, s->f.y) ? CHM_TUNE.half : 5;
    int32_t prey[2], *pp = NULL;

    bool scrum = r->t < SCRUM;
    if (!bot) {
        if (s->aggro_t > 0 && --s->aggro_t == 0) chm_ai_mood(r, i);
        if (s->aggro == AG_HUNT) {
            if (s->prey < CHM_SHIPS) {
                const ChmShip *t = &r->s[s->prey];
                if (!t->alive || t->finished || iabs(PX(t->f.x - s->f.x)) + iabs(PX(t->f.y - s->f.y)) > HUNT_R * 3 / 2)
                    s->prey = 255;
            }
            if (s->prey >= CHM_SHIPS) {
                int q = nearest_player(r, i, HUNT_R);
                if (q >= 0) s->prey = (uint8_t)q;
            }
            if (s->prey < CHM_SHIPS) {
                prey[0] = r->s[s->prey].f.x;
                prey[1] = r->s[s->prey].f.y;
                pp = prey;
            }
        } else {
            s->prey = 255;
        }
    }

    int top = bot ? CHM_TUNE.max_vx : s->skill;
    if (!bot && r->track == FLUE) top = top * 7 / 8; /* it never gets the hang of this one */
    if (bot || s->plan_t == 0) {
        int best = -1, pick = 3;
        for (int k = 0; k < 4; k++) {
            int ok = try_pace(s, z, zn, top * PACES[k] / 100, half, pp);
            if (chm_ai_debug) printf("  pace %d%%: %d frames\n", PACES[k], ok);
            if (ok > best) { best = ok; pick = k; }
            if (ok >= LOOK) break;
        }
        if (chm_ai_debug) { printf("  pick %d (z %d zn %d)\n", pick, z, zn); chm_ai_debug = 0; }
        s->act = (uint8_t)pick;
        s->plan_t = bot ? 0 : 2;
    } else {
        s->plan_t--;
    }
    int pace = top * PACES[s->act & 3] / 100;
    int32_t dvx, dvy;
    if (pp) {
        int ax = PX(pp[0] - s->f.x), ay = PX(pp[1] - s->f.y), la = imax(1, isqrt(ax * ax + ay * ay));
        dvx = ax * pace / la;
        dvy = ay * pace / la;
    } else {
        flow(s->f.x, s->f.y, z, zn, pace, &dvx, &dvy);
    }
    unsigned c = hold(&s->f, dvx, dvy);

    /* slashes. A hunter always swings at its prey; in the opening scrum
     * and when jostling a CPU swings at a player in reach now and then,
     * and at another CPU more rarely. The demo pilot swings at whoever
     * crowds it from in front. */
    if (!s->slash_cd) {
        int face = s->f.face >= 0 ? 1 : -1;
        if (bot) {
            if (in_reach(r, i, face, -1) >= 0) c |= CHM_CTL_SLASH;
        } else if (scrum || s->aggro != AG_CALM || r->track == FLUE) {
            bool hunting = s->aggro == AG_HUNT && s->prey < CHM_SHIPS;
            int only = hunting && !scrum ? s->prey : -1;
            int j = in_reach(r, i, face, only), back = in_reach(r, i, -face, only);
            int odds = hunting ? 1 : 5;
            if (j >= 0 && rng_range(&r->rng, 1, r->s[j].human || r->track == FLUE ? odds : 12) == 1) c |= CHM_CTL_SLASH;
            else if (back >= 0 && r->s[back].human && hunting) {
                /* turn round for it: the next frame's slash goes that way */
                c &= ~(unsigned)(CHF_LEFT | CHF_RIGHT);
                c |= face > 0 ? CHF_LEFT : CHF_RIGHT;
            }
        }
    }
    return c;
}
