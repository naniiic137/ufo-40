/* CHIME CIRCUIT - the race rules.
 *
 * Six ships, eight laps. Walls, floors and ceilings take a hit point (three
 * to a ship); a slash only knocks a ship away, and ships bump without harm.
 * A ship with no hit points left is wrecked: a replacement launches from
 * under the start line a moment later and has to fly that lap again. Every
 * finished lap mends one hit point. Pickups wait at stations, with a "!!"
 * before each one appears, and work the moment a ship flies into them.
 * A ship pushed out of the track altogether (which a hard enough knock
 * could do in the original) is wrecked and relaunched rather than lost. */
#include "chime.h"

const int CHM_POINTS[CHM_SHIPS] = {9, 7, 5, 3, 2, 1};
const char *const CHM_POWER_NAME[PW_COUNT] = {"", "BULLETS", "MINES", "FIREBALLS", "BIG SLASH", "PAYLOAD"};
/* how long each pickup lasts, in frames */
const uint16_t CHM_POWER_T[PW_COUNT] = {0, 240, 240, 300, 360, 180};

#define KNOCK 922          /* a slash sends a ship off at 3.6 px a frame */
#define SUPER_KNOCK 1638   /* ... and a big slash at 6.4 */
#define BOOST_SPEED 1075   /* an arrow: 4.2 */
#define SHOT_SPEED 896
#define SHOT_EVERY 16
#define SHOT_LIFE 90
#define MINE_EVERY 20
#define MINE_ARM 30        /* a fresh mine can't go off yet (so it isn't under its layer) */
#define MINE_LIFE 480
#define FIRE_LIFE 300
#define ORBIT_R 14
#define BLAST_R 36
#define BLAST_PUSH 1024
#define WARN_T 60          /* the "!!" shows this long before a pickup appears */
#define TOUCH 9            /* px: flying into a pickup */

static const int8_t SIN64[64] = {
    0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126, 127, 126, 125, 122, 117, 112,
    106, 98, 90, 81, 71, 60, 49, 37, 25, 12, 0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106,
    -112, -117, -122, -125, -126, -127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60,
    -49, -37, -25, -12};
int chm_sin(int a) { return SIN64[(a >> 2) & 63]; }       /* a in 1/256 turns */
int chm_cos(int a) { return SIN64[((a >> 2) + 16) & 63]; }
#define isin chm_sin
#define icos chm_cos

static int PX(int32_t v) { return (int)(v >> 8); }
static const ChmMap *M(void) { return &chm_map; }

/* ---- particles ------------------------------------------------------------ */
enum { PK_SPARK, PK_SMOKE, PK_BOOM, PK_STAR };

static void part(ChmRace *r, int x, int y, int vx, int vy, int life, int col, int kind) {
    for (int i = 0; i < CHM_PARTS; i++) {
        ChmPart *p = &r->part[i];
        if (p->on) continue;
        *p = (ChmPart){(int16_t)(x * 16), (int16_t)(y * 16), (int16_t)vx, (int16_t)vy, (uint8_t)life, (uint8_t)col,
                       (uint8_t)kind, 1};
        return;
    }
}

static void burst(ChmRace *r, int x, int y, int n, int col, int kind, int speed) {
    for (int k = 0; k < n; k++) {
        int a = rng_range(&r->rng, 0, 255), sp = rng_range(&r->rng, speed / 3, speed);
        part(r, x, y, icos(a) * sp / 127, isin(a) * sp / 127, rng_range(&r->rng, 14, 30), col, kind);
    }
}

static void parts_step(ChmRace *r) {
    for (int i = 0; i < CHM_PARTS; i++) {
        ChmPart *p = &r->part[i];
        if (!p->on) continue;
        p->x = (int16_t)(p->x + p->vx);
        p->y = (int16_t)(p->y + p->vy);
        if (p->kind == PK_SMOKE) p->vy = (int16_t)(p->vy - 1);
        else if (p->kind == PK_SPARK) p->vy = (int16_t)(p->vy + 1);
        if (--p->life == 0) p->on = 0;
    }
}

/* ---- the tournament ---------------------------------------------------------- */

void chm_cup_new(ChmCup *c, int humans, const uint8_t *pilots, Rng *rng) {
    memset(c, 0, sizeof *c);
    c->humans = (uint8_t)iclamp(humans, 0, 2); /* 0: the title screen's show race */
    bool used[CHM_PILOTS] = {false};
    for (int i = 0; i < c->humans; i++) {
        c->pilot[i] = (uint8_t)(pilots[i] % CHM_PILOTS);
        used[c->pilot[i]] = true;
    }
    /* the pilots nobody picked fly the CPU ships */
    int k = c->humans;
    for (int p = 0; p < CHM_PILOTS && k < CHM_SHIPS; p++)
        if (!used[p]) c->pilot[k++] = (uint8_t)p;
    /* the first grid is drawn by lot */
    for (int i = 0; i < CHM_SHIPS; i++) c->grid[i] = (uint8_t)i;
    for (int i = CHM_SHIPS - 1; i > 0; i--) {
        int j = rng_range(rng, 0, i);
        uint8_t t = c->grid[i];
        c->grid[i] = c->grid[j];
        c->grid[j] = t;
    }
}

void chm_cup_score(ChmCup *c, const ChmRace *r) {
    if (c->race >= CHM_RACES) return;
    uint8_t by_place[CHM_SHIPS];
    for (int i = 0; i < CHM_SHIPS; i++) {
        int pl = iclamp(r->s[i].place, 1, CHM_SHIPS);
        c->place[c->race][i] = (uint8_t)pl;
        c->points[i] = (uint8_t)(c->points[i] + CHM_POINTS[pl - 1]);
        by_place[pl - 1] = (uint8_t)i;
    }
    /* the next grid runs the other way: the winner starts at the back */
    for (int k = 0; k < CHM_SHIPS; k++) c->grid[k] = by_place[CHM_SHIPS - 1 - k];
    c->race++;
}

int chm_cup_wins(const ChmCup *c, int ship) {
    int n = 0;
    for (int k = 0; k < c->race && k < CHM_RACES; k++) n += c->place[k][ship] == 1;
    return n;
}

/* a beats b: more points; then more wins, more seconds and so on; then the
 * better place in the latest race */
static bool beats(const ChmCup *c, int a, int b) {
    if (c->points[a] != c->points[b]) return c->points[a] > c->points[b];
    for (int pl = 1; pl <= CHM_SHIPS; pl++) {
        int na = 0, nb = 0;
        for (int k = 0; k < c->race && k < CHM_RACES; k++) {
            na += c->place[k][a] == pl;
            nb += c->place[k][b] == pl;
        }
        if (na != nb) return na > nb;
    }
    if (c->race > 0) {
        int k = imin(c->race, CHM_RACES) - 1;
        if (c->place[k][a] != c->place[k][b]) return c->place[k][a] < c->place[k][b];
    }
    return a < b;
}

int chm_cup_rank(const ChmCup *c, int ship) {
    int n = 1;
    for (int j = 0; j < CHM_SHIPS; j++)
        if (j != ship && beats(c, j, ship)) n++;
    return n;
}

int chm_cup_leader(const ChmCup *c) {
    for (int i = 0; i < CHM_SHIPS; i++)
        if (chm_cup_rank(c, i) == 1) return i;
    return 0;
}

/* ---- ships -------------------------------------------------------------------- */

int chm_ship_by_human(const ChmRace *r, int human) {
    for (int i = 0; i < CHM_SHIPS; i++)
        if (r->s[i].human == human) return i;
    return -1;
}

static bool in_play(const ChmShip *s) { return s->alive && !s->finished && !s->parked; }

static void pick_route(ChmRace *r, int i) {
    ChmShip *s = &r->s[i];
    const ChmTrackDef *d = &CHM_TRACK[r->track];
    if (s->human) s->route = (uint8_t)imin(d->bot_route, M()->nroutes - 1);
    else s->route = (uint8_t)rng_range(&r->rng, 0, imax(1, imin(d->cpu_routes, M()->nroutes)) - 1);
    s->rz = 1;
}

void chm_race_begin(ChmRace *r, const ChmCup *c, int track, uint64_t seed) {
    memset(r, 0, sizeof *r);
    rng_seed(&r->rng, seed);
    r->track = (uint8_t)(track % CHM_TRACKS);
    r->humans = c->humans;
    r->fast_loot = c->fast_loot;
    chm_map_build(&chm_map, r->track);
    for (int slot = 0; slot < CHM_SHIPS; slot++) {
        int i = c->grid[slot] % CHM_SHIPS;
        ChmShip *s = &r->s[i];
        s->pilot = c->pilot[i];
        s->human = (uint8_t)(i < c->humans ? i + 1 : 0);
        chm_grid_pos(M(), slot, &s->f.x, &s->f.y);
        s->f.face = M()->dir;
        s->hp = CHM_HP;
        s->cp = 1;
        s->alive = 1;
        s->grid = (uint8_t)slot;
        pick_route(r, i);
        if (!s->human) {
            /* CPU pilots are slower than a good player: each has its own
             * speed limit for the race */
            s->skill = (int16_t)rng_range(&r->rng, 400, 470);
            chm_ai_mood(r, i);
        }
    }
    int k = 0;
    for (int row = 0; row < CHM_TH; row++)
        for (int col = 0; col < CHM_TW; col++)
            if (M()->station[row][col] && k < CHM_STATIONS) {
                ChmStation *st = &r->st[k++];
                st->x = (int16_t)(col * CHM_TILE + 4);
                st->y = (int16_t)(CHM_OY + row * CHM_TILE + 4);
                st->t = (uint16_t)(rng_range(&r->rng, 240, 420) / (r->fast_loot ? 3 : 1));
            }
    r->nst = (uint8_t)k;
    for (int slot = 0; slot < CHM_SHIPS; slot++) r->order[slot] = (uint8_t)(c->grid[slot] % CHM_SHIPS);
    r->phase = RP_COUNTDOWN;
}

void chm_race_wreck(ChmRace *r, int i) {
    ChmShip *s = &r->s[i];
    if (!s->alive || s->finished) return;
    s->alive = 0;
    s->hp = 0;
    s->wreck_t = CHM_WRECK_T;
    s->power = PW_NONE;
    s->power_t = 0;
    s->stun = 0;
    s->slash_t = 0;
    s->wrecks++;
    /* the lap starts over */
    s->cp = 1;
    s->rz = 1;
    int x = PX(s->f.x), y = PX(s->f.y);
    burst(r, x, y, 18, C_ORANGE, PK_BOOM, 40);
    burst(r, x, y, 10, C_YELLOW, PK_SPARK, 30);
    r->ev |= EV_WRECK;
    if (s->human) r->ev_human |= EV_WRECK;
}

void chm_race_hurt(ChmRace *r, int i) {
    ChmShip *s = &r->s[i];
    if (!in_play(s) || s->mercy) return;
    s->hp--;
    s->mercy = CHM_MERCY;
    r->ev |= EV_HURT;
    if (s->human) r->ev_human |= EV_HURT;
    if (s->hp <= 0) chm_race_wreck(r, i);
}

static void relaunch(ChmRace *r, int i) {
    ChmShip *s = &r->s[i];
    s->alive = 1;
    s->hp = CHM_HP;
    s->f.x = M()->spawn_x;
    s->f.y = M()->spawn_y;
    s->f.vx = 0;
    s->f.vy = -384; /* launched gently: it clears the floor without reaching the roof */
    s->f.face = M()->dir;
    s->mercy = CHM_LAUNCH_MERCY;
    s->cp = 1;
    s->rz = 1;
    if (!s->human) pick_route(r, i);
    burst(r, PX(s->f.x), PX(s->f.y) + 4, 8, C_LIGHT, PK_SMOKE, 16);
    r->ev |= EV_LAUNCH;
}

void chm_race_power(ChmRace *r, int i, int kind) {
    ChmShip *s = &r->s[i];
    s->power = (uint8_t)kind;
    s->power_t = CHM_POWER_T[kind % PW_COUNT];
    s->power_tick = 0;
    s->ball_x = s->f.x;
    s->ball_y = s->f.y + 10 * CHF_ONE;
    r->ev |= EV_PICK;
    if (s->human) r->ev_human |= EV_PICK;
}

void chm_station_fill(ChmRace *r, int k, int kind) {
    if (k < 0 || k >= r->nst) return;
    r->st[k].state = 2;
    r->st[k].kind = (uint8_t)kind;
}

static uint16_t refill_time(ChmRace *r) {
    int t = rng_range(&r->rng, 480, 840);
    return (uint16_t)(r->fast_loot ? t / 4 : t);
}

/* ---- laps ------------------------------------------------------------------------ */

static void lap_done(ChmRace *r, int i) {
    ChmShip *s = &r->s[i];
    s->laps++;
    s->last_lap = (uint16_t)imin((int)(r->t - s->lap_t0), 65535);
    s->lap_t0 = r->t;
    s->cp = 1;
    s->rz = 1;
    if (s->hp < CHM_HP) s->hp++;
    if (!s->human) pick_route(r, i);
    r->ev |= EV_LAP;
    if (s->human) r->ev_human |= s->laps == CHM_LAPS - 1 ? EV_LASTLAP : EV_LAP;
    if (s->laps >= CHM_LAPS) {
        s->finished = 1;
        s->place = ++r->nfinished;
        s->finish_t = r->t;
        s->power = PW_NONE;
        burst(r, PX(s->f.x), PX(s->f.y), 16, C_YELLOW, PK_STAR, 36);
        r->ev |= EV_FINISH;
        if (s->human) r->ev_human |= EV_FINISH;
    }
}

/* the AI's line follows the checkpoints a ship really passes */
static void sync_route(ChmShip *s, int zone) {
    const ChmMap *m = M();
    int n = m->route_len[s->route];
    for (int k = 0; k < n; k++)
        if (m->route[s->route][k] == zone) { s->rz = (uint8_t)((k + 1) % n); return; }
}

static void pass_zone(ChmRace *r, int i, int z) {
    ChmShip *s = &r->s[i];
    const ChmMap *m = M();
    if (z < 0) return;
    int n = m->route_len[s->route];
    if (n && z == m->route[s->route][s->rz % n] && z >= 10) s->rz = (uint8_t)((s->rz + 1) % n);
    if (z >= 1 && z <= 9 && z == s->cp) {
        s->cp++;
        sync_route(s, z);
    } else if (z == CHM_ZONE_S && s->cp == m->ncp + 1) {
        lap_done(r, i);
    }
}

/* ---- pieces of the frame --------------------------------------------------------- */

static bool out_of_track(const ChmShip *s) {
    int x = PX(s->f.x), y = PX(s->f.y);
    if (x < 0 || x >= CHM_TW * CHM_TILE || y < CHM_OY || y >= CHM_OY + CHM_TH * CHM_TILE) return true;
    return chm_solid(M(), x, y);
}

static bool try_shift(ChmShip *s, int32_t dx, int32_t dy) {
    if (chm_flight_blocked(CHM_TUNE.half, chm_solid, M(), s->f.x + dx, s->f.y + dy)) return false;
    s->f.x += dx;
    s->f.y += dy;
    return true;
}

/* a ship caught against scenery (it shouldn't happen) is eased out */
static void unstick(ChmRace *r, int i) {
    ChmShip *s = &r->s[i];
    if (!chm_flight_blocked(CHM_TUNE.half, chm_solid, M(), s->f.x, s->f.y)) return;
    for (int d = 1; d <= 6; d++)
        for (int k = 0; k < 8; k++) {
            static const int8_t DX[8] = {0, 0, 1, -1, 1, 1, -1, -1}, DY[8] = {-1, 1, 0, 0, -1, 1, -1, 1};
            if (try_shift(s, DX[k] * d * CHF_ONE, DY[k] * d * CHF_ONE)) return;
        }
    s->lost++;
    chm_race_wreck(r, i);
}

static void do_slash(ChmRace *r, int i) {
    ChmShip *s = &r->s[i];
    bool super = s->power == PW_SUPER;
    int cx = PX(s->f.x), cy = PX(s->f.y), f = s->f.face >= 0 ? 1 : -1;
    int reach = super ? 24 : 19;
    int x0 = f > 0 ? cx + 1 : cx - reach - 1, x1 = x0 + reach, y0 = cy - (super ? 13 : 10), y1 = cy + (super ? 12 : 9);
    for (int j = 0; j < CHM_SHIPS; j++) {
        ChmShip *t = &r->s[j];
        if (j == i || !in_play(t) || (s->slash_hits & (1 << j))) continue;
        int tx = PX(t->f.x), ty = PX(t->f.y);
        if (tx + 3 < x0 || tx - 4 > x1 || ty + 3 < y0 || ty - 4 > y1) continue;
        s->slash_hits |= (uint8_t)(1 << j);
        t->f.vx = f * (super ? SUPER_KNOCK : KNOCK);
        t->f.vy = t->f.vy / 2 - 160;
        t->stun = super ? CHM_SUPER_STUN : CHM_STUN;
        s->knocks++;
        burst(r, tx, ty, 6, C_WHITE, PK_SPARK, 28);
        r->ev |= EV_KNOCK;
        if (s->human || t->human) r->ev_human |= EV_KNOCK;
    }
}

static void fire_shot(ChmRace *r, int owner, int dx, int dy) {
    for (int k = 0; k < CHM_BULLETS; k++) {
        ChmShot *b = &r->shot[k];
        if (b->on) continue;
        const ChmShip *s = &r->s[owner];
        *b = (ChmShot){s->f.x + dx * 6 * CHF_ONE, s->f.y + dy * 6 * CHF_ONE, dx * SHOT_SPEED, dy * SHOT_SPEED,
                       (uint8_t)owner, SHOT_LIFE, 1};
        return;
    }
}

static void add_fire(ChmRace *r, int x, int y) {
    if (chm_flight_blocked(3, chm_solid, M(), x * CHF_ONE, y * CHF_ONE)) return;
    for (int k = 0; k < CHM_FIRES; k++)
        if (!r->fire[k].on) {
            r->fire[k] = (ChmFire){(int16_t)x, (int16_t)y, 1, 0};
            return;
        }
}

void chm_race_fire(ChmRace *r, int x, int y) { add_fire(r, x, y); }

static void blast(ChmRace *r, int owner, int32_t bx, int32_t by) {
    int x = PX(bx), y = PX(by);
    for (int j = 0; j < CHM_SHIPS; j++) {
        ChmShip *t = &r->s[j];
        if (!in_play(t)) continue;
        int dx = PX(t->f.x) - x, dy = PX(t->f.y) - y;
        int d2 = dx * dx + dy * dy;
        if (d2 > BLAST_R * BLAST_R) continue;
        int d = 1;
        while (d * d < d2) d++;
        if (d2 == 0) { dx = 0; dy = -1; d = 1; }
        t->f.vx = BLAST_PUSH * dx / d;
        t->f.vy = BLAST_PUSH * dy / d;
        t->stun = 12;
    }
    /* three fires in a spread triangle */
    for (int k = 0; k < 3; k++) {
        int a = 192 + k * 85; /* up, then down-right, then down-left */
        int fx = x + icos(a) * 16 / 127, fy = y + isin(a) * 16 / 127;
        if (chm_flight_blocked(3, chm_solid, M(), fx * CHF_ONE, fy * CHF_ONE)) {
            fx = x + icos(a) * 8 / 127;
            fy = y + isin(a) * 8 / 127;
        }
        add_fire(r, fx, fy);
    }
    burst(r, x, y, 22, C_ORANGE, PK_BOOM, 44);
    r->ev |= EV_BLAST;
    (void)owner;
}

static void powers(ChmRace *r, int i) {
    ChmShip *s = &r->s[i];
    if (!s->power || !in_play(s)) return;
    int x = PX(s->f.x), y = PX(s->f.y);
    switch (s->power) {
    case PW_BULLETS:
        if (s->power_tick % SHOT_EVERY == 0) {
            fire_shot(r, i, 1, 0);
            fire_shot(r, i, -1, 0);
            fire_shot(r, i, 0, 1);
            fire_shot(r, i, 0, -1);
            r->ev |= EV_SHOT;
        }
        break;
    case PW_MINES:
        if (s->power_tick % MINE_EVERY == MINE_EVERY / 2)
            for (int k = 0; k < CHM_MINES; k++)
                if (!r->mine[k].on) {
                    int mx = x - s->f.face * 9, my = y;
                    if (!chm_flight_blocked(3, chm_solid, M(), mx * CHF_ONE, my * CHF_ONE)) {
                        r->mine[k] = (ChmMine){(int16_t)mx, (int16_t)my, (uint8_t)i, 1, 0};
                        r->ev |= EV_MINE;
                    }
                    break;
                }
        break;
    case PW_FIREBALLS:
        s->orbit = (uint16_t)(s->orbit + 8);
        for (int b = 0; b < 2; b++) {
            int a = s->orbit + b * 128;
            int fx = x + icos(a) * ORBIT_R / 127, fy = y + isin(a) * ORBIT_R / 127;
            for (int j = 0; j < CHM_SHIPS; j++) {
                ChmShip *t = &r->s[j];
                if (j == i || !in_play(t)) continue;
                if (iabs(PX(t->f.x) - fx) <= 6 && iabs(PX(t->f.y) - fy) <= 6) {
                    if (!t->mercy) r->ev |= EV_FIRE;
                    chm_race_hurt(r, j);
                }
            }
        }
        break;
    case PW_PAYLOAD: {
        /* the ball hangs behind on its chain */
        int32_t tx = s->f.x - s->f.vx * 5, ty = s->f.y + 12 * CHF_ONE - s->f.vy * 3;
        s->ball_x += (tx - s->ball_x) / 6;
        s->ball_y += (ty - s->ball_y) / 6;
        break;
    }
    default: break;
    }
    s->power_tick++;
    if (s->power_t > 0 && --s->power_t == 0) {
        if (s->power == PW_PAYLOAD) blast(r, i, s->ball_x, s->ball_y);
        s->power = PW_NONE;
    }
}

static void shots_step(ChmRace *r) {
    for (int k = 0; k < CHM_BULLETS; k++) {
        ChmShot *b = &r->shot[k];
        if (!b->on) continue;
        b->x += b->vx;
        b->y += b->vy;
        int x = PX(b->x), y = PX(b->y);
        if (chm_solid(M(), x, y) || --b->life == 0) {
            b->on = 0;
            part(r, x, y, 0, -4, 8, C_YELLOW, PK_SPARK);
            continue;
        }
        for (int j = 0; j < CHM_SHIPS; j++) {
            ChmShip *t = &r->s[j];
            if (j == b->owner || !in_play(t)) continue;
            if (iabs(PX(t->f.x) - x) <= 5 && iabs(PX(t->f.y) - y) <= 5) {
                chm_race_hurt(r, j);
                b->on = 0;
                burst(r, x, y, 4, C_YELLOW, PK_SPARK, 20);
                break;
            }
        }
    }
    for (int k = 0; k < CHM_MINES; k++) {
        ChmMine *mn = &r->mine[k];
        if (!mn->on) continue;
        if (++mn->t > MINE_LIFE) { mn->on = 0; continue; }
        if (mn->t < MINE_ARM) continue;
        for (int j = 0; j < CHM_SHIPS; j++) {
            ChmShip *t = &r->s[j];
            if (!in_play(t)) continue;
            /* the layer is not spared */
            if (iabs(PX(t->f.x) - mn->x) <= 7 && iabs(PX(t->f.y) - mn->y) <= 7) {
                chm_race_hurt(r, j);
                mn->on = 0;
                burst(r, mn->x, mn->y, 10, C_ORANGE, PK_BOOM, 30);
                r->ev |= EV_BLAST;
                break;
            }
        }
    }
    for (int k = 0; k < CHM_FIRES; k++) {
        ChmFire *f = &r->fire[k];
        if (!f->on) continue;
        if (++f->t > FIRE_LIFE) { f->on = 0; continue; }
        for (int j = 0; j < CHM_SHIPS; j++) {
            ChmShip *t = &r->s[j];
            if (!in_play(t)) continue;
            if (iabs(PX(t->f.x) - f->x) <= 7 && iabs(PX(t->f.y) - f->y) <= 8) {
                /* a fire hurts and slows */
                t->f.vx = t->f.vx * 2 / 5;
                t->f.vy = t->f.vy * 2 / 5;
                if (!t->mercy) r->ev |= EV_FIRE;
                chm_race_hurt(r, j);
            }
        }
    }
}

static void stations_step(ChmRace *r) {
    for (int k = 0; k < r->nst; k++) {
        ChmStation *st = &r->st[k];
        if (st->state == 0) {
            if (st->t > 0) st->t--;
            if (st->t == 0) {
                st->state = 1;
                st->t = WARN_T;
                r->ev |= EV_WARN;
            }
        } else if (st->state == 1) {
            if (--st->t == 0) {
                st->state = 2;
                st->kind = (uint8_t)rng_range(&r->rng, 1, PW_COUNT - 1);
            }
        } else {
            for (int j = 0; j < CHM_SHIPS; j++) {
                ChmShip *t = &r->s[j];
                if (!in_play(t)) continue;
                if (iabs(PX(t->f.x) - st->x) <= TOUCH && iabs(PX(t->f.y) - st->y) <= TOUCH) {
                    chm_race_power(r, j, st->kind);
                    burst(r, st->x, st->y, 8, C_YELLOW, PK_STAR, 24);
                    st->state = 0;
                    st->t = refill_time(r);
                    break;
                }
            }
        }
    }
}

/* ships bump off each other; nobody is hurt */
static void bumps(ChmRace *r) {
    const int32_t SIZE = 8 * CHF_ONE;
    for (int i = 0; i < CHM_SHIPS; i++)
        for (int j = i + 1; j < CHM_SHIPS; j++) {
            ChmShip *a = &r->s[i], *b = &r->s[j];
            if (!in_play(a) || !in_play(b)) continue;
            int32_t dx = b->f.x - a->f.x, dy = b->f.y - a->f.y;
            if (dx >= SIZE || dx <= -SIZE || dy >= SIZE || dy <= -SIZE) continue;
            int32_t ox = SIZE - (dx < 0 ? -dx : dx), oy = SIZE - (dy < 0 ? -dy : dy);
            if (ox <= oy) {
                int sg = dx >= 0 ? 1 : -1;
                int32_t half = ox / 2 + 1;
                if (!try_shift(a, -sg * half, 0)) try_shift(b, sg * ox, 0);
                if (!try_shift(b, sg * half, 0)) try_shift(a, -sg * ox, 0);
                if ((b->f.vx - a->f.vx) * sg < 0) {
                    int32_t t = a->f.vx;
                    a->f.vx = b->f.vx * 9 / 10;
                    b->f.vx = t * 9 / 10;
                }
            } else {
                int sg = dy >= 0 ? 1 : -1;
                int32_t half = oy / 2 + 1;
                if (!try_shift(a, 0, -sg * half)) try_shift(b, 0, sg * oy);
                if (!try_shift(b, 0, sg * half)) try_shift(a, 0, -sg * oy);
                if ((b->f.vy - a->f.vy) * sg < 0) {
                    int32_t t = a->f.vy;
                    a->f.vy = b->f.vy * 9 / 10;
                    b->f.vy = t * 9 / 10;
                }
            }
            r->ev |= EV_BUMP;
        }
}

uint32_t chm_progress(const ChmRace *r, int i) {
    const ChmShip *s = &r->s[i];
    const ChmMap *m = M();
    if (s->finished) return 0xF0000000u - (uint32_t)s->place;
    if (s->parked) return 0;
    int zone = s->cp <= m->ncp ? s->cp : CHM_ZONE_S;
    int32_t x = s->alive ? s->f.x : m->spawn_x, y = s->alive ? s->f.y : m->spawn_y;
    uint32_t d = chm_field_at(m, zone, x, y);
    return (uint32_t)(s->laps * (m->ncp + 1) + (s->cp - 1)) * 65536u + (65535u - d);
}

static void sort_order(ChmRace *r) {
    uint32_t p[CHM_SHIPS];
    for (int i = 0; i < CHM_SHIPS; i++) p[i] = chm_progress(r, i);
    for (int i = 0; i < CHM_SHIPS; i++) r->order[i] = (uint8_t)i;
    for (int a = 1; a < CHM_SHIPS; a++)
        for (int b = a; b > 0; b--) {
            int x = r->order[b - 1], y = r->order[b];
            if (p[y] > p[x] || (p[y] == p[x] && y < x)) {
                r->order[b - 1] = (uint8_t)y;
                r->order[b] = (uint8_t)x;
            } else break;
        }
}

static void ship_step(ChmRace *r, int i, const unsigned ctl[2]) {
    ChmShip *s = &r->s[i];
    const ChmMap *m = M();
    if (s->finished || s->parked) return;
    if (!s->alive) {
        if (s->wreck_t > 0 && --s->wreck_t == 0) relaunch(r, i);
        return;
    }
    unsigned c = s->human ? ctl[(s->human - 1) & 1] : s->idle ? 0 : chm_ai(r, i, false);
    s->ctl = (uint8_t)c;
    if (s->stun) {
        s->stun--;
        c = 0;
    }
    chm_flight_control(&s->f, &CHM_TUNE, c & (CHF_LEFT | CHF_RIGHT | CHF_THRUST));
    if ((c & CHM_CTL_SLASH) && !s->slash_cd) {
        s->slash_t = CHM_SLASH_T;
        s->slash_cd = CHM_SLASH_CD;
        s->slash_hits = 0;
        r->ev |= EV_SLASH;
        if (s->human) r->ev_human |= EV_SLASH;
    }
    if (s->slash_t > CHM_SLASH_T - CHM_SLASH_ACTIVE) do_slash(r, i);
    int32_t ox = s->f.x, oy = s->f.y;
    int hit = chm_flight_move(&s->f, &CHM_TUNE, chm_solid, m);
    if (hit) {
        s->wall_hits++;
        int hx = PX(s->f.x) + ((hit & CHF_HIT_X) ? (s->f.vx > 0 ? -4 : 4) : 0);
        int hy = PX(s->f.y) + ((hit & CHF_HIT_Y) ? (s->f.vy > 0 ? -4 : 4) : 0);
        burst(r, hx, hy, 5, C_WHITE, PK_SPARK, 24);
        r->ev |= EV_WALL;
        if (s->human) r->ev_human |= EV_WALL;
        chm_race_hurt(r, i);
        if (!s->alive) return;
    }
    if (out_of_track(s)) {
        s->lost++;
        chm_race_wreck(r, i);
        return;
    }
    /* zones: halfway and at the end, so a fast ship can't skip a line */
    int z1 = chm_zone_at(m, PX((ox + s->f.x) / 2), PX((oy + s->f.y) / 2));
    int z2 = chm_zone_at(m, PX(s->f.x), PX(s->f.y));
    pass_zone(r, i, z1);
    if (z2 != z1 && !s->finished) pass_zone(r, i, z2);
    if (s->finished) return;
    int b = chm_boost_at(m, PX(s->f.x), PX(s->f.y));
    if (b) {
        if (b == 1 && s->f.vx < BOOST_SPEED) s->f.vx = BOOST_SPEED;
        if (b == 2 && s->f.vx > -BOOST_SPEED) s->f.vx = -BOOST_SPEED;
        if (b == 3 && s->f.vy > -BOOST_SPEED) s->f.vy = -BOOST_SPEED;
        if (b == 4 && s->f.vy < BOOST_SPEED) s->f.vy = BOOST_SPEED;
        if (!s->boost_t) {
            r->ev |= EV_BOOST;
            if (s->human) r->ev_human |= EV_BOOST;
        }
        s->boost_t = 12;
    }
    /* a ship on its last hit point smokes */
    if (s->hp == 1 && (r->t + (uint32_t)i * 3) % 6 == 0)
        part(r, PX(s->f.x) - s->f.face * 3, PX(s->f.y) - 3, rng_range(&r->rng, -3, 3), -6, 24, C_GREY, PK_SMOKE);
}

void chm_race_step(ChmRace *r, const unsigned ctl[2]) {
    r->ev = r->ev_human = 0;
    if (r->phase == RP_COUNTDOWN) {
        if (r->count_t % 40 == 0) r->ev |= EV_BEEP;
        if (++r->count_t >= CHM_COUNTDOWN) {
            r->phase = RP_RUN;
            r->ev |= EV_GO;
        }
        parts_step(r);
        return;
    }
    if (r->phase == RP_OVER) {
        parts_step(r);
        return;
    }
    r->t++;
    for (int i = 0; i < CHM_SHIPS; i++) ship_step(r, i, ctl);
    for (int i = 0; i < CHM_SHIPS; i++) powers(r, i);
    shots_step(r);
    stations_step(r);
    bumps(r);
    for (int i = 0; i < CHM_SHIPS; i++) {
        ChmShip *s = &r->s[i];
        if (in_play(s)) unstick(r, i);
        if (s->mercy) s->mercy--;
        if (s->slash_t) s->slash_t--;
        if (s->slash_cd) s->slash_cd--;
        if (s->boost_t) s->boost_t--;
    }
    parts_step(r);
    sort_order(r);
    /* the race ends a little after the last player crosses the line */
    bool all = true, players = true;
    for (int i = 0; i < CHM_SHIPS; i++) {
        all &= r->s[i].finished || r->s[i].parked;
        if (r->s[i].human) players &= r->s[i].finished;
    }
    if (r->humans == 0 ? all : players && (all || ++r->over_t >= CHM_END_WAIT)) {
        for (int k = 0; k < CHM_SHIPS; k++) {
            ChmShip *s = &r->s[r->order[k]];
            if (!s->place) s->place = ++r->nfinished;
        }
        r->phase = RP_OVER;
    }
}
