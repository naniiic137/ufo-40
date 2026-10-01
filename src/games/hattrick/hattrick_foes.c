/* HAT TRICK - the creatures, the bosses, the Timekeeper and their shots.
 * Three kinds a world, each moving the way its counterpart in the original
 * moves; every boss a big one of its world's kinds (the Lido has two). */
#include "hattrick.h"

const char *const HTK_FOE_NAME[FK_COUNT] = {
    "SPIKER", "FRISBEE", "BEACH BALL", "PIN", "BOWLER", "SPINNER", "BUOY", "POLO", "DUCKY",
    "PROP", "GLOVE", "BULLDOG", "KING SPIKER", "KINGPIN", "THE LIFEGUARD", "THE CHAIR", "THE CAPTAIN",
    "THE TIMEKEEPER",
};

static const int8_t SIZE[FK_COUNT][2] = {
    {6, 6}, {7, 3}, {6, 6}, {4, 7}, {6, 8}, {6, 6}, {6, 6}, {6, 7}, {6, 5},
    {7, 8}, {6, 6}, {7, 6}, {12, 12}, {8, 15}, {12, 13}, {9, 14}, {13, 16}, {6, 8},
};
static const int8_t BOSS_HP[5] = {12, 15, 40, 10, 20}; /* king spiker, kingpin, lifeguard, chair, captain */

static bool walks(int k) {
    return k == FK_SPIKER || k == FK_PIN || k == FK_BOWLER || k == FK_POLO || k == FK_PROP || k == FK_KINGSPIKER ||
           k == FK_TOWER || k == FK_CAPTAIN;
}

int htk_nearest_player(const HtkPlay *g, float x, float y) {
    int best = -1;
    float bd = 1e9f;
    for (int k = 0; k < 2; k++) {
        const HtkPlayer *p = &g->pl[k];
        if (!p->on || p->out || !p->alive || p->lifted) continue;
        float d = fabsf(htk_wrapdx(p->x - x)) + fabsf(htk_wrapdy(p->y - 10 - y));
        if (d < bd) { bd = d; best = k; }
    }
    return best;
}

void htk_foe_setup(HtkPlay *g, HtkFoe *f) {
    f->hw = SIZE[f->kind][0];
    f->hh = SIZE[f->kind][1];
    f->ax = f->x;
    f->ay = f->y;
    f->vdir = f->alt ? -1 : 1;
    if (walks(f->kind)) f->y = (float)((int)(f->y / HTK_T) * HTK_T + HTK_T) - f->hh;
    if (f->kind >= FK_KINGSPIKER && f->kind <= FK_CAPTAIN) {
        f->hp = f->maxhp = BOSS_HP[f->kind - FK_KINGSPIKER];
        f->wait = 60;
    }
    switch (f->kind) {
    case FK_SPIKER: case FK_POLO: f->fire_t = rng_range(&g->rng, 180, 360); break;
    case FK_BOWLER: f->fire_t = rng_range(&g->rng, 100, 260); break;
    case FK_PROP: f->fire_t = rng_range(&g->rng, 60, 200); break;
    case FK_SPINNER: f->ang = f->dir > 0 ? 0 : 3.14159265f; break;
    case FK_DUCKY: {
        /* a route round the platform it sits on */
        int c = (int)(f->x / HTK_T), r = (int)(f->y / HTK_T);
        int pr = -1;
        for (int k = 1; k < HTK_ROWS && pr < 0; k++) {
            int rr = (r + k) % HTK_ROWS;
            if (g->tile[rr][c] == T_SOLID || g->tile[rr][c] == T_LEDGE) pr = rr;
        }
        if (pr < 0) pr = HTK_ROWS - 1;
        int c0 = c, c1 = c;
        while (c0 > 0 && (g->tile[pr][c0 - 1] == T_SOLID || g->tile[pr][c0 - 1] == T_LEDGE)) c0--;
        while (c1 < HTK_COLS - 1 && (g->tile[pr][c1 + 1] == T_SOLID || g->tile[pr][c1 + 1] == T_LEDGE)) c1++;
        f->rx0 = c0 * HTK_T - (float)f->hw;
        f->rx1 = (c1 + 1) * HTK_T + (float)f->hw;
        f->ry0 = pr * HTK_T - (float)f->hh;
        f->ry1 = pr * HTK_T + HTK_T + (float)f->hh;
        f->ang = f->x - f->rx0; /* distance along the route, from the top left */
        break;
    }
    case FK_KINGPIN:
        f->rx0 = HTK_T + f->hw;
        f->rx1 = HTK_W - HTK_T - f->hw;
        f->ry0 = HTK_T + f->hh;
        f->ry1 = HTK_H - HTK_T - f->hh;
        f->ang = (f->rx1 - f->rx0) + (f->ry1 - f->ry0) + (f->rx1 - f->x); /* on the floor */
        f->y = f->ry1;
        f->fire_t = 100;
        break;
    case FK_LIFEGUARD: f->fire_t = 120; f->ax = f->x; f->ay = f->y; break;
    case FK_CAPTAIN: f->fire_t = 90; break;
    default: break;
    }
}

int htk_spawn_foe(HtkPlay *g, int kind, float x, float y, int dir, int alt) {
    for (int i = 0; i < HTK_MAX_FOES; i++) {
        HtkFoe *f = &g->foe[i];
        if (f->alive) continue;
        memset(f, 0, sizeof *f);
        f->kind = (int16_t)kind;
        f->x = x;
        f->y = y;
        f->dir = (int16_t)dir;
        f->alt = (uint8_t)alt;
        f->alive = 1;
        htk_foe_setup(g, f);
        return i;
    }
    return -1;
}

int htk_add_shot(HtkPlay *g, int kind, float x, float y, float vx, float vy) {
    for (int i = 0; i < HTK_MAX_SHOTS; i++) {
        HtkShot *s = &g->shot[i];
        if (s->alive) continue;
        *s = (HtkShot){x, y, vx, vy, (int16_t)kind, 0, 0, 1};
        return i;
    }
    return -1;
}

void htk_spawn_body(HtkPlay *g, float x, float y, int body, int item, float vx) {
    for (int i = 0; i < HTK_MAX_ITEMS; i++) {
        HtkItem *it = &g->item[i];
        if (it->alive) continue;
        memset(it, 0, sizeof *it);
        it->x = x;
        it->y = y;
        it->vx = vx;
        it->vy = body < 0 ? -1.0f : -2.6f;
        it->body = (int16_t)body;
        it->item = (int16_t)item;
        it->alive = 1;
        it->falling = 1;
        return;
    }
}

/* ---- moving about ---------------------------------------------------------- */

static void fall(HtkPlay *g, HtkFoe *f) {
    float oy = f->y + f->hh;
    f->vy = fminf(f->vy + HTK_GRAV, HTK_MAXFALL);
    f->y += f->vy;
    f->ground = 0;
    if (f->vy >= 0) {
        float top = htk_find_floor(g, f->x - f->hw + 1, f->x + f->hw - 1, oy, f->y + f->hh, true);
        if (top > -1e8f) {
            f->y = top - f->hh;
            f->vy = 0;
            f->ground = 1;
        }
    } else if (htk_box_solid(g, f->x - f->hw + 1, f->y - f->hh, f->x + f->hw - 1, f->y - f->hh, true)) {
        f->vy = 0;
        f->y = floorf((f->y - f->hh) / HTK_T) * HTK_T + HTK_T + f->hh;
    }
    if (f->y >= HTK_H) f->y -= HTK_H;
    if (f->y < 0) f->y += HTK_H;
}

static bool ground_under(const HtkPlay *g, float x, float y) {
    int t = htk_tile(g, x, y + 1);
    return t == T_SOLID || t == T_LEDGE;
}

/* along the ground; edges = turn back at the end of a platform */
static void walk(HtkPlay *g, HtkFoe *f, float speed, bool edges) {
    if (!f->ground) return;
    float nx = f->x + f->dir * speed, lead = nx + f->dir * f->hw;
    bool wall = htk_box_solid(g, lead, f->y - f->hh + 1, lead, f->y + f->hh - 1, true);
    bool gap = edges && !ground_under(g, lead, f->y + f->hh);
    if (wall || gap) f->dir = (int16_t)-f->dir;
    else f->x = htk_wrapx(nx);
}

/* through the air, turning back at walls */
static void drift_x(HtkPlay *g, HtkFoe *f) {
    float nx = f->x + f->vx, lead = nx + (f->vx > 0 ? f->hw : -f->hw);
    if (htk_box_solid(g, lead, f->y - f->hh + 1, lead, f->y + f->hh - 1, true)) f->vx = -f->vx;
    else f->x = htk_wrapx(nx);
    if (f->vx) f->dir = (int16_t)(f->vx > 0 ? 1 : -1);
}

static void home(HtkFoe *f, float tx, float ty, float speed) {
    float dx = htk_wrapdx(tx - f->x), dy = htk_wrapdy(ty - f->y), d = sqrtf(dx * dx + dy * dy);
    if (d < 0.5f) return;
    f->x = htk_wrapx(f->x + dx / d * speed);
    f->y += dy / d * speed;
    if (f->y >= HTK_H) f->y -= HTK_H;
    if (f->y < 0) f->y += HTK_H;
    if (fabsf(dx) > 0.5f) f->dir = (int16_t)(dx > 0 ? 1 : -1);
}

/* aimed at a kid; down_only keeps shots level or falling (stay above them) */
static bool aim(HtkPlay *g, HtkFoe *f, float speed, bool down_only, float *vx, float *vy) {
    int k = htk_nearest_player(g, f->x, f->y);
    if (k < 0) return false;
    float dx = htk_wrapdx(g->pl[k].x - f->x), dy = htk_wrapdy(g->pl[k].y - 9 - f->y);
    float d = sqrtf(dx * dx + dy * dy);
    if (d < 1) d = 1;
    *vx = dx / d * speed;
    *vy = dy / d * speed;
    if (down_only && *vy < 0.15f * speed) {
        *vy = 0.15f * speed;
        *vx = (dx < 0 ? -1 : 1) * sqrtf(speed * speed - *vy * *vy);
    }
    return true;
}

static float side_of_kid(HtkPlay *g, HtkFoe *f) {
    int k = htk_nearest_player(g, f->x, f->y);
    if (k < 0) return (float)f->dir;
    return htk_wrapdx(g->pl[k].x - f->x) < 0 ? -1.0f : 1.0f;
}

/* the shooters: walk, stop to wind up, fire, walk on */
static void shooter(HtkPlay *g, HtkFoe *f, float speed, int lo, int hi) {
    fall(g, f);
    if (f->state == 0) {
        walk(g, f, speed, true);
        if (--f->fire_t <= 0 && htk_nearest_player(g, f->x, f->y) >= 0 && f->ground) {
            f->state = 1;
            f->wait = 24;
            f->dir = (int16_t)side_of_kid(g, f);
        }
        return;
    }
    if (--f->wait > 0) return;
    float vx = 0, vy = 0;
    switch (f->kind) {
    case FK_SPIKER:
        if (aim(g, f, 1.4f, true, &vx, &vy)) htk_add_shot(g, SH_SERVE, f->x, f->y - 2, vx, vy);
        break;
    case FK_POLO:
        if (aim(g, f, 1.9f, true, &vx, &vy)) htk_add_shot(g, SH_SQUIRT, f->x + f->dir * 5, f->y - 2, vx, vy);
        break;
    case FK_BOWLER: {
        float s = side_of_kid(g, f);
        htk_add_shot(g, SH_BOWL, f->x + s * 6, f->y, s * 1.3f, 1.3f);
        break;
    }
    case FK_PROP: {
        float s = side_of_kid(g, f);
        htk_add_shot(g, SH_RUGBY, f->x + s * 6, f->y - 6, s * (1.5f + rng_float(&g->rng) * 0.6f), -2.4f);
        break;
    }
    default: break;
    }
    htk_sfx("htk_throw");
    f->state = 0;
    f->fire_t = rng_range(&g->rng, lo, hi);
}

/* round a platform: along the top, down the end, back underneath, up the other end */
static void route(HtkFoe *f, float speed) {
    float w = f->rx1 - f->rx0, h = f->ry1 - f->ry0, per = 2 * (w + h);
    f->ang += f->dir * speed;
    while (f->ang < 0) f->ang += per;
    while (f->ang >= per) f->ang -= per;
    float s = f->ang;
    if (s < w) { f->x = f->rx0 + s; f->y = f->ry0; }
    else if (s < w + h) { f->x = f->rx1; f->y = f->ry0 + (s - w); }
    else if (s < 2 * w + h) { f->x = f->rx1 - (s - w - h); f->y = f->ry1; }
    else { f->x = f->rx0; f->y = f->ry1 - (s - 2 * w - h); }
    f->x = htk_wrapx(f->x);
    if (f->y >= HTK_H) f->y -= HTK_H;
    if (f->y < 0) f->y += HTK_H;
}

/* ---- the bosses ----------------------------------------------------------------- */

static void boss_bodies(HtkPlay *g, HtkFoe *f, int small) {
    for (int k = 0; k < 5; k++)
        htk_spawn_body(g, f->x + (k - 2) * 6, f->y, small, IT_DRUMSTICK, (k - 2) * 0.55f);
    htk_burst(g, f->x, f->y, C_YELLOW, 20, 2.0f);
}

static int small_of(int boss) {
    switch (boss) {
    case FK_KINGSPIKER: return FK_SPIKER;
    case FK_KINGPIN: return FK_PIN;
    case FK_LIFEGUARD: return FK_POLO;
    default: return FK_PROP;
    }
}

static bool chair_up(const HtkPlay *g) {
    for (int i = 0; i < HTK_MAX_FOES; i++)
        if (g->foe[i].alive && g->foe[i].kind == FK_TOWER) return true;
    return false;
}

void htk_boss_hit(HtkPlay *g, int i, int dmg) {
    HtkFoe *f = &g->foe[i];
    f->hp = (int16_t)(f->hp - dmg);
    f->flash = 30;
    htk_sfx("htk_bosshit");
    if (f->hp > 0) return;
    htk_sfx("htk_bossdown");
    if (f->kind == FK_TOWER) {
        /* the chair was the one that mattered: the lifeguard goes with it */
        f->alive = 0;
        htk_burst(g, f->x, f->y, C_RED, 16, 2.0f);
        for (int k = 0; k < HTK_MAX_FOES; k++) {
            HtkFoe *l = &g->foe[k];
            if (!l->alive || l->kind != FK_LIFEGUARD) continue;
            if (!l->away) boss_bodies(g, l, FK_POLO);
            l->alive = 0;
        }
        return;
    }
    boss_bodies(g, f, small_of(f->kind));
    if (f->kind == FK_LIFEGUARD && chair_up(g)) {
        /* beaten, but the match goes on: he is back from the changing hut */
        f->away = 1;
        f->wait = 300;
        return;
    }
    f->alive = 0;
}

static void boss_update(HtkPlay *g, HtkFoe *f) {
    switch (f->kind) {
    case FK_KINGSPIKER:
        /* three hops at you, then three spades over in arcs */
        fall(g, f);
        if (!f->ground) { drift_x(g, f); break; }
        f->vx = 0;
        if (--f->wait > 0) break;
        if (f->state == 0) {
            if (f->hops < 3) {
                f->vy = -4.2f;
                f->vx = side_of_kid(g, f) * 1.3f;
                f->hops++;
                f->wait = 20;
                htk_sfx("htk_bosshop");
            } else {
                f->state = 1;
                f->wait = 30;
            }
        } else if (f->state == 1) {
            float s = side_of_kid(g, f);
            for (int k = 0; k < 3; k++) htk_add_shot(g, SH_SPADE, f->x, f->y - 10, s * (0.7f + 0.6f * k), -3.6f);
            htk_sfx("htk_throw");
            f->state = 2;
            f->wait = 60;
        } else {
            f->state = 0;
            f->hops = 0;
            f->wait = 20;
        }
        break;
    case FK_KINGPIN:
        /* round and round the edge of the lanes, throwing cans */
        route(f, 1.5f);
        if (--f->fire_t <= 0) {
            float vx, vy;
            if (aim(g, f, 1.7f, false, &vx, &vy)) htk_add_shot(g, SH_CAN, f->x, f->y, vx, vy);
            htk_sfx("htk_throw");
            f->fire_t = 100;
        }
        break;
    case FK_LIFEGUARD:
        if (f->away) {
            if (--f->wait <= 0) {
                f->away = 0;
                f->hp = f->maxhp;
                f->x = 160;
                f->y = 24;
                f->flash = 60;
                f->ax = 160;
                f->ay = 60;
            }
            break;
        }
        home(f, f->ax, f->ay, 0.9f);
        if ((fabsf(htk_wrapdx(f->ax - f->x)) < 3 && fabsf(f->ay - f->y) < 3) || f->t % 240 == 0) {
            f->ax = (float)rng_range(&g->rng, 40, 280);
            f->ay = (float)rng_range(&g->rng, 30, 136);
        }
        if (--f->fire_t <= 0) {
            float vx, vy;
            if (aim(g, f, 1.6f, false, &vx, &vy)) {
                float c = cosf(0.3f), s = sinf(0.3f);
                htk_add_shot(g, SH_RING, f->x, f->y, vx * c - vy * s, vx * s + vy * c);
                htk_add_shot(g, SH_RING, f->x, f->y, vx * c + vy * s, -vx * s + vy * c);
                htk_sfx("htk_throw");
            }
            f->fire_t = 110;
        }
        break;
    case FK_TOWER:
        /* the chair only hops about */
        fall(g, f);
        if (!f->ground) { drift_x(g, f); break; }
        f->vx = 0;
        if (--f->wait <= 0) {
            f->vy = -4.5f;
            f->vx = (rng_chance(&g->rng, 50) ? 1.0f : -1.0f) * 1.4f;
            f->wait = 50;
            htk_sfx("htk_bosshop");
        }
        break;
    case FK_CAPTAIN: {
        fall(g, f);
        if (!f->ground) drift_x(g, f);
        else {
            f->vx = 0;
            int k = htk_nearest_player(g, f->x, f->y);
            if (--f->wait <= 0 && k >= 0 && htk_wrapdy(g->pl[k].y - (f->y + f->hh)) < -20) {
                f->vy = -4.6f;
                f->vx = side_of_kid(g, f) * 1.2f;
                f->wait = 60;
            } else if (k >= 0) {
                f->dir = (int16_t)side_of_kid(g, f);
                walk(g, f, 0.7f, false);
            }
        }
        if (--f->fire_t <= 0) {
            float s = side_of_kid(g, f);
            htk_add_shot(g, SH_RUGBY, f->x + s * 10, f->y - 10, s * 1.5f, -2.6f);
            htk_add_shot(g, SH_RUGBY, f->x + s * 10, f->y - 10, s * 2.2f, -2.2f);
            htk_sfx("htk_throw");
            f->fire_t = 90;
        }
        break;
    }
    default: break;
    }
}

/* ---- every creature, every frame ------------------------------------------------------ */

void htk_foes_update(HtkPlay *g) {
    for (int i = 0; i < HTK_MAX_FOES; i++) {
        HtkFoe *f = &g->foe[i];
        if (!f->alive) continue;
        f->t++;
        if (f->flash > 0) f->flash--;
        if ((g->sub != LS_PLAY || g->still) && f->kind != FK_TIMEKEEPER) continue;
        if (g->still) continue;
        switch (f->kind) {
        case FK_SPIKER: shooter(g, f, 0.45f, 200, 380); break;
        case FK_POLO: shooter(g, f, 0.5f, 200, 340); break;
        case FK_BOWLER: shooter(g, f, 0.35f, 180, 320); break;
        case FK_PROP: shooter(g, f, 0.4f, 150, 260); break;
        case FK_FRISBEE: {
            float nx = f->x + f->dir * 0.8f, lead = nx + f->dir * f->hw;
            if (htk_box_solid(g, lead, f->ay - f->hh, lead, f->ay + f->hh, true) || fabsf(htk_wrapdx(nx - f->ax)) > 96)
                f->dir = (int16_t)-f->dir;
            else f->x = htk_wrapx(nx);
            f->y = f->ay + sinf(f->t * 0.08f) * 3;
            break;
        }
        case FK_BEACHBALL: {
            float nx = f->x + f->dir * 0.75f;
            if (htk_box_solid(g, nx - f->hw, f->y - f->hh, nx + f->hw, f->y + f->hh, false) ||
                (htk_tile(g, nx + f->dir * f->hw, f->y) == T_LEDGE))
                f->dir = (int16_t)-f->dir;
            else f->x = htk_wrapx(nx);
            float ny = f->y + f->vdir * 0.75f, edge = ny + f->vdir * f->hh;
            int t0 = htk_tile(g, f->x - f->hw + 1, edge), t1 = htk_tile(g, f->x + f->hw - 1, edge);
            if (t0 == T_SOLID || t1 == T_SOLID || t0 == T_LEDGE || t1 == T_LEDGE) f->vdir = (int16_t)-f->vdir;
            else {
                f->y = ny;
                if (f->y >= HTK_H) f->y -= HTK_H;
                if (f->y < 0) f->y += HTK_H;
            }
            break;
        }
        case FK_PIN:
            fall(g, f);
            walk(g, f, 1.1f, false);
            break;
        case FK_SPINNER:
            f->ang += f->dir * 0.035f;
            f->x = htk_wrapx(f->ax + cosf(f->ang) * 22);
            f->y = f->ay + sinf(f->ang) * 22;
            break;
        case FK_BUOY:
            if (f->alt) f->x = htk_wrapx(f->x + f->dir * 0.8f);
            else {
                f->y += 0.8f;
                if (f->y >= HTK_H) f->y -= HTK_H;
            }
            break;
        case FK_DUCKY: route(f, 0.7f); break;
        case FK_GLOVE:
            f->y += f->vdir * 1.9f;
            if (f->y >= HTK_H) f->y -= HTK_H;
            if (f->y < 0) f->y += HTK_H;
            break;
        case FK_BULLDOG: {
            /* asleep until a kid carries the ball, then straight for them */
            int c = g->ball.carrier;
            f->state = c >= 0 && g->pl[c].alive;
            if (f->state) home(f, g->pl[c].x, g->pl[c].y - 9, 0.95f);
            break;
        }
        case FK_TIMEKEEPER: {
            int k = htk_nearest_player(g, f->x, f->y);
            if (k >= 0) home(f, g->pl[k].x, g->pl[k].y - 9, 0.42f);
            break;
        }
        default: boss_update(g, f); break;
        }
    }
}

void htk_shots_update(HtkPlay *g) {
    for (int i = 0; i < HTK_MAX_SHOTS; i++) {
        HtkShot *s = &g->shot[i];
        if (!s->alive) continue;
        s->t++;
        if (s->kind == SH_RUGBY) {
            /* bounces hard and never quite the same way twice */
            s->vy = fminf(s->vy + 0.12f, 4.5f);
            float nx = s->x + s->vx;
            if (htk_box_solid(g, nx - 3, s->y - 2, nx + 3, s->y + 2, false)) s->vx = -s->vx;
            else s->x = htk_wrapx(nx);
            float oy = s->y;
            s->y += s->vy;
            if (s->vy > 0) {
                float top = htk_find_floor(g, s->x - 2, s->x + 2, oy + 3, s->y + 3, false);
                if (top > -1e8f) {
                    s->y = top - 3;
                    s->vy = -s->vy * (0.7f + rng_float(&g->rng) * 0.3f) - 0.6f;
                    s->vx += (rng_float(&g->rng) - 0.5f) * 1.2f;
                    s->vx = fclamp(s->vx, -2.6f, 2.6f);
                    if (fabsf(s->vx) < 0.4f) s->vx = s->vx < 0 ? -0.4f : 0.4f;
                    s->bounces++;
                }
            } else if (htk_box_solid(g, s->x - 2, s->y - 3, s->x + 2, s->y - 3, false)) {
                s->vy = -s->vy * 0.5f;
            }
            if (s->y >= HTK_H) s->y -= HTK_H;
            if (s->y < 0) s->y += HTK_H;
            if (s->bounces >= 5 || s->t > 330) s->alive = 0;
            continue;
        }
        if (s->kind == SH_SPADE) {
            s->vy = fminf(s->vy + 0.12f, 4.0f);
            float oy = s->y;
            s->x += s->vx;
            s->y += s->vy;
            if (s->vy > 0 && htk_find_floor(g, s->x - 2, s->x + 2, oy + 2, s->y + 2, false) > -1e8f) {
                s->alive = 0;
                htk_burst(g, s->x, s->y, C_GREY, 4, 0.8f);
            }
        } else {
            s->x += s->vx;
            s->y += s->vy;
        }
        if (s->x < -8 || s->x > HTK_W + 8 || s->y < -8 || s->y > HTK_H + 8 || s->t > 600) s->alive = 0;
    }
}
