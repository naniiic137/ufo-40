/* KIPPER - the six big creatures. Four roam their area along loose loops
 * and come after the sub when they see it; the nautilus keeps to its
 * chamber; Xolloth waits in its lair. Damage done to one is kept once the
 * sub docks (lost with the rest if the fuel runs out first); beaten, one
 * flees for good and leaves a strange light. */
#include "kpr.h"

const char *const KPR_BOSS_NAME[KPR_BOSSES] = {"GNASHER", "INKWELL", "LONGJAW", "THE WHORL", "NIGHTCAPE", "XOLLOTH"};
const int KPR_BOSS_HP[KPR_BOSSES] = {500, 500, 600, 800, 1000, 1400};
static const int8_t BW[KPR_BOSSES] = {36, 28, 22, 30, 28, 48};
static const int8_t BH[KPR_BOSSES] = {18, 26, 14, 30, 34, 44};
static const float ROAM[KPR_BOSSES] = {0.8f, 0.5f, 0.75f, 0, 0.7f, 0};
static const float CHASE[KPR_BOSSES] = {0.92f, 0.6f, 0.85f, 0, 0.8f, 0};

static bool in_rect(const KprRect *r, float x, float y) {
    int tx = (int)(x / KPR_TILE), ty = (int)(y / KPR_TILE);
    return tx >= r->x0 && tx <= r->x1 && ty >= r->y0 && ty <= r->y1;
}

static float pt_x(int b, int i) { return KPR_ROUTES[b].pts[i].x * KPR_TILE + 8.0f; }
static float pt_y(int b, int i) { return KPR_ROUTES[b].pts[i].y * KPR_TILE + 8.0f; }

void kpr_bosses_reset(void) {
    for (int b = 0; b < KPR_BOSSES; b++) {
        KprBoss *o = &kg.boss[b];
        memset(o, 0, sizeof *o);
        o->on = !kg.cur.boss_dead[b];
        o->hp = kg.cur.boss_hp[b];
        o->x = pt_x(b, 0);
        o->y = pt_y(b, 0);
        o->route = KPR_ROUTES[b].n > 1 ? 1 : 0;
        o->dir = -1;
        for (int k = 0; k < 10; k++) { o->sx[k] = o->x + k * 8.0f; o->sy[k] = o->y; }
    }
    kg.fight = -1;
}

bool kpr_boss_box(int b, float *x, float *y, int *w, int *h) {
    const KprBoss *o = &kg.boss[b];
    if (!o->on || o->dying) return false;
    *x = o->x;
    *y = o->y;
    *w = BW[b];
    *h = BH[b];
    return true;
}

bool kpr_boss_vulnerable(int b) {
    const KprBoss *o = &kg.boss[b];
    if (b == BS_NIGHTCAPE && o->state == 3) return false;   /* the cape shut */
    if (b == BS_WHORL && !in_rect(&KPR_ARENA_WHORL, kg.x, kg.y)) return false;
    return true;
}

void kpr_boss_hurt(int b, int dmg) {
    KprBoss *o = &kg.boss[b];
    if (!o->on || o->dying) return;
    if (!kpr_boss_vulnerable(b)) return;
    o->hp -= dmg;
    o->flash = 6;
    kg.cur.boss_hp[b] = (int16_t)imax(0, o->hp);
    kpr_sfx("kpr_bosshit");
    if (o->hp <= 0) {
        o->dying = 1;
        o->t = 0;
        kpr_sfx("kpr_bigboom");
    }
}

static bool line_clear(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    int n = (int)(d / 8) + 1;
    for (int i = 1; i < n; i++)
        if (kpr_shot_solid(x0 + dx * i / n, y0 + dy * i / n)) return false;
    return true;
}

static bool boss_move(int b, float dx, float dy) {
    KprBoss *o = &kg.boss[b];
    bool hit = false;
    if (!kpr_box_solid(o->x + dx, o->y, BW[b] - 6, BH[b] - 6)) o->x += dx;
    else hit = true;
    if (!kpr_box_solid(o->x, o->y + dy, BW[b] - 6, BH[b] - 6)) o->y += dy;
    else hit = true;
    if (dx != 0) o->dir = dx > 0 ? 1 : -1;
    return hit;
}

static void toward(int b, float tx, float ty, float sp, bool walls) {
    KprBoss *o = &kg.boss[b];
    float dx = tx - o->x, dy = ty - o->y, d = sqrtf(dx * dx + dy * dy);
    if (d < 0.5f) return;
    float k = fminf(sp, d) / d;
    if (walls) boss_move(b, dx * k, dy * k);
    else {
        o->x += dx * k;
        o->y += dy * k;
        if (dx != 0) o->dir = dx > 0 ? 1 : -1;
    }
}

/* a roaming boss: along its loop; after the sub when it sees it (leaving
 * breadcrumbs), then back the way it came */
static void roam(int b) {
    KprBoss *o = &kg.boss[b];
    float dx = kg.x - o->x, dy = kg.y - o->y, d2 = dx * dx + dy * dy;
    bool same_area = kpr_area_of(kg.y) == kpr_area_of(o->y);
    bool sees = !kg.calm && same_area && d2 < 150.0f * 150.0f && line_clear(o->x, o->y, kg.x, kg.y);
    if (o->state == 0) {
        if (sees && o->seen_t <= 0) { o->state = 1; o->t = 0; o->nb = 0; o->bx[0] = o->x; o->by[0] = o->y; o->nb = 1; }
        if (o->seen_t > 0) o->seen_t--;
        float tx = pt_x(b, o->route), ty = pt_y(b, o->route);
        float ex = tx - o->x, ey = ty - o->y;
        if (ex * ex + ey * ey < 4.0f) o->route = (o->route + 1) % KPR_ROUTES[b].n;
        toward(b, tx, ty, ROAM[b], false);
    } else if (o->state == 1) {
        /* the chase */
        float sp = CHASE[b];
        if (b == BS_GNASHER && o->t % 150 > 110 && fabsf(dy) < 24) sp = 2.4f;    /* a lunge */
        if (b == BS_LONGJAW && o->t % 120 > 96 && d2 < 110.0f * 110.0f) sp = 3.0f;
        toward(b, kg.x, kg.y, sp, true);
        float lx = o->bx[o->nb - 1], ly = o->by[o->nb - 1];
        if ((o->x - lx) * (o->x - lx) + (o->y - ly) * (o->y - ly) > 144.0f && o->nb < 40) {
            o->bx[o->nb] = o->x;
            o->by[o->nb] = o->y;
            o->nb++;
        }
        /* out of sight for a second, it gives up */
        if (line_clear(o->x, o->y, kg.x, kg.y)) o->ax = 0;
        else o->ax += 1;
        if (o->t > 240 || o->nb >= 40 || !same_area || d2 > 260.0f * 260.0f || o->ax > 60) { o->state = 2; o->t = 0; }
    } else {
        /* back along the breadcrumbs to the loop */
        if (o->nb <= 0) { o->state = 0; o->seen_t = 120; return; }
        float tx = o->bx[o->nb - 1], ty = o->by[o->nb - 1];
        toward(b, tx, ty, ROAM[b] * 1.3f, false);
        if ((o->x - tx) * (o->x - tx) + (o->y - ty) * (o->y - ty) < 4.0f) o->nb--;
    }
}

static void tentacles(int b) {
    /* Inkwell's arms reach out of the walls near the sub */
    KprBoss *o = &kg.boss[b];
    float dx = kg.x - o->x, dy = kg.y - o->y;
    if (o->tent_t > 0) {
        o->tent_t++;
        if (o->tent_t > 40 && o->tent_t < 90) {
            /* out: a line from the wall toward where the sub was */
            float len = fminf(1.0f, (o->tent_t - 40) / 10.0f) * 44.0f;
            float ex = o->tx + cosf(o->aang) * len, ey = o->ty + sinf(o->aang) * len;
            for (int k = 0; k <= 4; k++) {
                float px = o->tx + (ex - o->tx) * k / 4, py = o->ty + (ey - o->ty) * k / 4;
                if (fabsf(px - kg.x) < 9 && fabsf(py - kg.y) < 8) kpr_sub_hurt(px, py, KPR_HIT);
            }
        }
        if (o->tent_t >= 100) o->tent_t = 0;
        return;
    }
    if (dx * dx + dy * dy > 190.0f * 190.0f || kg.calm || o->t % 200 != 0) return;
    /* the wall tile nearest the sub */
    int sx = (int)(kg.x / KPR_TILE), sy = (int)(kg.y / KPR_TILE);
    for (int r = 1; r <= 3; r++)
        for (int yy = -r; yy <= r; yy++)
            for (int xx = -r; xx <= r; xx++) {
                if (imax(iabs(xx), iabs(yy)) != r || !kpr_solid_tile(sx + xx, sy + yy)) continue;
                if (kpr_tile(sx + xx, sy + yy) == '^') continue;
                o->tx = (sx + xx) * KPR_TILE + 8.0f;
                o->ty = (sy + yy) * KPR_TILE + 8.0f;
                o->aang = atan2f(kg.y - o->ty, kg.x - o->tx);
                o->tent_t = 1;
                kpr_sfx("kpr_rumble");
                return;
            }
}

static void whorl(int b) {
    KprBoss *o = &kg.boss[b];
    const KprRect *r = &KPR_ARENA_WHORL;
    bool inside = in_rect(r, kg.x, kg.y) && !kg.calm;
    if (!inside) {
        toward(b, pt_x(b, 0), pt_y(b, 0), 0.6f, false);
        o->state = 0;
        return;
    }
    if (o->state == 0) {
        o->state = 1;
        o->t = 0;
        o->vx = 1.5f;
        o->vy = 1.1f;
    }
    /* bounding about its chamber */
    float nx = o->x + o->vx, ny = o->y + o->vy;
    float x0 = r->x0 * KPR_TILE + BW[b] / 2.0f, x1 = (r->x1 + 1) * KPR_TILE - BW[b] / 2.0f;
    float y0 = r->y0 * KPR_TILE + BH[b] / 2.0f, y1 = (r->y1 + 1) * KPR_TILE - BH[b] / 2.0f;
    if (nx < x0 || nx > x1 || kpr_box_solid(nx, o->y, BW[b] - 8, BH[b] - 8)) o->vx = -o->vx;
    if (ny < y0 || ny > y1 || kpr_box_solid(o->x, ny, BW[b] - 8, BH[b] - 8)) o->vy = -o->vy;
    boss_move(b, o->vx, o->vy);
    /* spirals of shots: three seconds on, one and a half off */
    int cyc = o->t % 270;
    if (cyc < 180 && o->t % 7 == 0) {
        o->aang += 0.42f;
        for (int k = 0; k < 2; k++) {
            float a = o->aang + k * 3.14159f;
            kpr_add_eshot(ES_PELLET, o->x, o->y, cosf(a) * 1.3f, sinf(a) * 1.3f);
        }
        if (o->t % 21 == 0) kpr_sfx("kpr_spit");
    }
}

static void nightcape(int b) {
    KprBoss *o = &kg.boss[b];
    if (o->state == 3) {
        /* the cape shut: spines all round, no harm gets in */
        if (o->t == 1 || o->t == 45) {
            for (int k = 0; k < 12; k++) {
                float a = k * 0.5236f + (o->t == 45 ? 0.26f : 0);
                kpr_add_eshot(ES_SPINE, o->x, o->y, cosf(a) * 1.5f, sinf(a) * 1.5f);
            }
            kpr_sfx("kpr_spines");
        }
        if (o->t > 90) { o->state = 1; o->t = 0; }
        return;
    }
    roam(b);
    if (o->state == 1 && o->t > 0 && o->t % 240 == 120) { o->state = 3; o->t = 0; }
}

static void xolloth(int b) {
    KprBoss *o = &kg.boss[b];
    const KprRect *r = &KPR_ARENA_XOLLOTH;
    if (!in_rect(r, kg.x, kg.y) || kg.calm) {
        o->state = 0;
        o->beam_t = 0;
        return;
    }
    if (o->state == 0) { o->state = 1; o->t = 0; o->ax = kg.x; o->ay = kg.y; }
    int len = o->hp > KPR_BOSS_HP[b] / 2 ? 600 : 480;
    int c = o->t % len;
    float hx = pt_x(b, 0), hy = pt_y(b, 0);
    if (c < 220) {
        /* the laser: a line to an aim point that trails the sub; across it
         * diagonally and it can't keep up */
        toward(b, hx, hy - 10, 0.8f, false);
        float ex = kg.x - o->ax, ey = kg.y - o->ay, e = sqrtf(ex * ex + ey * ey);
        float sp = c < 60 ? 1.4f : 0.7f;
        if (e > sp) { o->ax += ex / e * sp; o->ay += ey / e * sp; }
        o->beam_t = c < 60 ? 1 : (c < 200 ? 2 : 0);
        if (c == 60) kpr_sfx("kpr_laser");
        if (o->beam_t == 2) {
            float lx = o->ax - o->x, ly = o->ay - o->y, l = sqrtf(lx * lx + ly * ly);
            if (l > 1) {
                lx /= l;
                ly /= l;
                float px = kg.x - o->x, py = kg.y - o->y;
                float along = px * lx + py * ly, across = fabsf(px * ly - py * lx);
                if (along > 0 && across < 7) kpr_sub_hurt(kg.x - lx * 6, kg.y - ly * 6, KPR_HIT);
            }
        }
    } else if (c < 380) {
        /* the spin: whirling round the lair; keep away */
        o->beam_t = 0;
        if (c == 220) {
            o->vx = kg.x > o->x ? 2.2f : -2.2f;
            o->vy = 1.6f;
            for (int k = 0; k < 8; k++) {
                float a = k * 0.785f;
                kpr_add_eshot(ES_SPORE, o->x, o->y, cosf(a) * 1.2f, sinf(a) * 1.2f);
            }
            kpr_sfx("kpr_spin");
        }
        float x0 = r->x0 * KPR_TILE + BW[b] / 2.0f, x1 = (r->x1 + 1) * KPR_TILE - BW[b] / 2.0f;
        float y0 = r->y0 * KPR_TILE + BH[b] / 2.0f, y1 = (r->y1 + 1) * KPR_TILE - BH[b] / 2.0f;
        if (o->x + o->vx < x0 || o->x + o->vx > x1 || kpr_box_solid(o->x + o->vx, o->y, BW[b] - 12, BH[b] - 12)) o->vx = -o->vx;
        if (o->y + o->vy < y0 || o->y + o->vy > y1 || kpr_box_solid(o->x, o->y + o->vy, BW[b] - 12, BH[b] - 12)) o->vy = -o->vy;
        o->x += o->vx;
        o->y += o->vy;
        o->aang += 0.3f;
    } else {
        /* spores, aimed */
        toward(b, hx, hy, 1.0f, false);
        if ((c - 380) % 50 == 10) {
            for (int k = -1; k <= 1; k++) {
                float a = atan2f(kg.y - o->y, kg.x - o->x) + k * 0.3f;
                kpr_add_eshot(ES_SPORE, o->x, o->y, cosf(a) * 1.5f, sinf(a) * 1.5f);
            }
            kpr_sfx("kpr_spit");
        }
    }
}

static void touch(int b) {
    KprBoss *o = &kg.boss[b];
    if (fabsf(o->x - kg.x) * 2 < BW[b] + KPR_SW - 6 && fabsf(o->y - kg.y) * 2 < BH[b] + KPR_SH - 6) {
        kpr_sub_hurt(o->x, o->y, KPR_HIT);
        return;
    }
    if (b == BS_LONGJAW)
        for (int k = 1; k < 10; k++)
            if (fabsf(o->sx[k] - kg.x) < 10 && fabsf(o->sy[k] - kg.y) < 9) { kpr_sub_hurt(o->sx[k], o->sy[k], KPR_HIT); return; }
}

void kpr_bosses_update(void) {
    kg.fight = -1;
    for (int b = 0; b < KPR_BOSSES; b++) {
        KprBoss *o = &kg.boss[b];
        if (!o->on) continue;
        o->t++;
        if (o->flash > 0) o->flash--;
        if (o->dying) {
            /* a chain of blasts, then it flees for good */
            if (o->t % 8 == 0) kpr_burst(o->x + (float)((o->t * 7) % 30 - 15), o->y + (float)((o->t * 11) % 24 - 12), C_ORANGE, 8, 1.6f);
            if (o->t % 16 == 0) kpr_sfx("kpr_boom");
            o->x += o->dir * 0.6f;
            if (o->t > 100) {
                o->on = 0;
                if (b == BS_XOLLOTH) kpr_final_defeated();
                else kpr_boss_defeated(b);
                if (kg.state != KS_PLAY) return;
            }
            continue;
        }
        switch (b) {
        case BS_INKWELL:
            roam(b);
            tentacles(b);
            if (!kg.calm && o->t % 140 == 70) {
                float dx = kg.x - o->x, dy = kg.y - o->y;
                if (dx * dx + dy * dy < 170.0f * 170.0f && line_clear(o->x, o->y, kg.x, kg.y)) {
                    for (int k = -1; k <= 1; k++) {
                        float a = atan2f(dy, dx) + k * 0.35f;
                        kpr_add_eshot(ES_INK, o->x, o->y, cosf(a) * 1.2f, sinf(a) * 1.2f);
                    }
                    kpr_sfx("kpr_spit");
                }
            }
            break;
        case BS_WHORL: whorl(b); break;
        case BS_NIGHTCAPE: nightcape(b); break;
        case BS_XOLLOTH: xolloth(b); break;
        default: roam(b); break;
        }
        if (b == BS_LONGJAW) {
            o->sx[0] = o->x - o->dir * 8.0f;
            o->sy[0] = o->y;
            for (int k = 1; k < 10; k++) {
                float ex = o->sx[k - 1] - o->sx[k], ey = o->sy[k - 1] - o->sy[k], e = sqrtf(ex * ex + ey * ey);
                if (e > 8.0f) { o->sx[k] += ex / e * (e - 8.0f); o->sy[k] += ey / e * (e - 8.0f); }
            }
        }
        if (!kg.calm) touch(b);
        if (kg.state != KS_PLAY) return;
        /* a fight going on: near, in the same area */
        float dx = kg.x - o->x, dy = kg.y - o->y, d2 = dx * dx + dy * dy;
        if (kpr_area_of(o->y) == kpr_area_of(kg.y) && d2 < 200.0f * 200.0f &&
            (b != BS_WHORL || in_rect(&KPR_ARENA_WHORL, kg.x, kg.y)) &&
            (b != BS_XOLLOTH || in_rect(&KPR_ARENA_XOLLOTH, kg.x, kg.y)))
            kg.fight = b;
        /* the rumble: something big is near */
        if (d2 < 220.0f * 220.0f && d2 > 110.0f * 110.0f && kpr_area_of(o->y) == kpr_area_of(kg.y) && o->warn_t <= 0 &&
            b != BS_XOLLOTH) {
            o->warn_t = 900;
            kg.shake = imax(kg.shake, 20);
            kpr_sfx("kpr_rumble");
            kpr_say("THE WATER'S RUMBLING... SOMETHING BIG.");
        }
        if (o->warn_t > 0) o->warn_t--;
    }
}
