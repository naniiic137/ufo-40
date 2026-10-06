/* FULL PEAL - the five bosses, one at the end of each stage. Each hangs
 * in the distance with one weak point the forward gun must find down its
 * lane (the rest of the body stops shots), and each turns nastier once
 * it is down to half its health.
 *
 *   A  the Gloameye: drifts in curves; three orbs circle it and take turns
 *      lobbing crosses; hit the eye; at half it goes gold and fires twice
 *      as often
 *   B  Knucklebell: flies in a square; two great fists close in along the
 *      plane from both ends (the side blaster knocks them back; they can't
 *      be hurt, and they bounce apart when they meet); hit the nose
 *   C  the Inkwell: drifts in curves, drops flares down the plane from the
 *      top and now and then lobs two pairs of crosses; at half it moves
 *      faster and drops only flares
 *   D  Shellback: sweeps along the top firing fans of 3, 4 and 3; its two
 *      pods fire one aimed shot each now and then
 *   E  Queen Sordina: her mouth is the only weak point, and only while it
 *      is open; a hit shuts it; left open too long it spits caltrops; wisps
 *      cross the plane. Emptied, she heals to full, until Clary flies in
 *      and knocks her down to a third: then any hit on her counts, her two
 *      pods lob crosses, and Clary fires alongside you. */
#include "fpl.h"

const char *const FPL_BOSS_NAME[BOSS_COUNT] = {"THE GLOAMEYE", "KNUCKLEBELL", "THE INKWELL", "SHELLBACK", "QUEEN SORDINA"};
const int FPL_BOSS_HP[BOSS_COUNT] = {150, 190, 230, 270, 300};

#define FIST_HW 0.42f
#define FIST_HH 0.55f
#define FIST_HOME 3.25f
#define SORDINA_OPEN_MAX 150
#define SORDINA_SHUT_T 110
#define CLARY_DELAY 240

/* each boss's body (half sizes in plane units at its depth: shots that
 * land inside stop) */
static const float BODY_HW[BOSS_COUNT] = {0.95f, 1.1f, 0.95f, 1.25f, 1.1f};
static const float BODY_HH[BOSS_COUNT] = {1.0f, 1.0f, 1.1f, 0.8f, 1.2f};

static FplBoss *B(void) { return &fpg.boss; }

void fpl_boss_start(int kind) {
    FplBoss *b = B();
    memset(b, 0, sizeof *b);
    b->on = true;
    b->kind = kind;
    b->hp = b->maxhp = FPL_BOSS_HP[kind];
    b->x = 0;
    b->y = -0.3f;
    b->fire_t = 120;
    b->fx[0] = -FIST_HOME;
    b->fx[1] = FIST_HOME;
    b->fy = 0.5f;
    b->mouth_t = SORDINA_SHUT_T;
}

/* the orbs and pods: where option k is now */
void fpl_boss_option(int k, float *x, float *y) {
    const FplBoss *b = &fpg.boss;
    if (b->kind == BOSS_GLOAMEYE) {
        float a = b->ang + k * 2.0943951f;
        *x = b->x + cosf(a) * 1.55f;
        *y = b->y + sinf(a) * 1.0f;
    } else if (b->kind == BOSS_SHELLBACK) {
        *x = b->x + (k ? 1.9f : -1.9f);
        *y = b->y + 0.45f;
    } else {
        *x = b->x + (k ? 1.7f : -1.7f);
        *y = b->y + 0.7f;
    }
}

int fpl_boss_noptions(void) {
    const FplBoss *b = &fpg.boss;
    if (!b->on || b->dead) return 0;
    return b->kind == BOSS_GLOAMEYE ? 3 : (b->kind == BOSS_SHELLBACK || (b->kind == BOSS_SORDINA && b->phase == 2)) ? 2 : 0;
}

void fpl_boss_weak(float *x, float *y, float *r) {
    const FplBoss *b = &fpg.boss;
    *x = b->x;
    *y = b->y;
    *r = 0.5f;
    switch (b->kind) {
    case BOSS_KNUCKLEBELL: *y = b->y + 0.21f; break;
    case BOSS_GLOAMEYE: *y = b->y - 0.36f; break;
    case BOSS_INKWELL: break;
    case BOSS_SHELLBACK: *y = b->y - 0.48f; break;
    case BOSS_SORDINA:
        *y = b->y - 0.18f;
        if (b->phase == 2) { *y = b->y; *r = 1.2f; }
        else if (!b->mouth) *r = 0;
        break;
    default: break;
    }
}

bool fpl_boss_weak_open(void) {
    float x, y, r;
    fpl_boss_weak(&x, &y, &r);
    return fpg.boss.on && !fpg.boss.dead && r > 0;
}

static void hurt(int dmg) {
    FplBoss *b = B();
    if (b->dead) return;
    if (b->kind == BOSS_SORDINA && b->phase == 1) { b->flash = 4; return; } /* healing: nothing gets through */
    b->hp -= dmg;
    b->flash = 6;
    b->hits++;
    fpl_sfx("fpl_bhit", 3);
    if (b->kind == BOSS_SORDINA && b->phase == 0 && b->mouth && !b->shut_by_hit) {
        b->mouth_t = imin(b->mouth_t, 12); /* a hit shuts her mouth */
        b->shut_by_hit = true;
    }
    if (!b->angry && b->hp <= b->maxhp / 2 && b->kind != BOSS_SORDINA) {
        b->angry = true;
        fpl_sfx("fpl_angry", 0);
    }
    if (b->hp <= 0) {
        if (b->kind == BOSS_SORDINA && b->phase == 0) {
            /* emptied: she heals, and holds out until Clary comes */
            b->hp = 0;
            b->phase = 1;
            b->heal_t = 0;
            b->mouth = false;
            fpl_sfx("fpl_heal", 0);
            return;
        }
        b->hp = 0;
        b->dead = true;
        b->dead_t = 0;
        fpg.shake = 30;
        for (int i = 0; i < FPL_MAX_FOES; i++) if (fpg.foe[i].alive) fpl_kill_foe(i, false);
        for (int i = 0; i < FPL_MAX_ESHOTS; i++) fpg.es[i].alive = 0;
        fpl_sfx("fpl_bigboom", 0);
    }
}

bool fpl_boss_shot(FplPShot *s) {
    FplBoss *b = B();
    float lx = fpl_lane_x(s->lane_c), ly = fpl_lane_y(s->lane_r);
    float wx, wy, wr;
    fpl_boss_weak(&wx, &wy, &wr);
    if (wr > 0 && fabsf(lx - wx) < wr + 0.25f && fabsf(ly - wy) < wr + 0.25f) {
        hurt(1);
        fpl_burst(lx, ly, FPL_BOSS_Z, C_WHITE, 4);
        return true;
    }
    if (fabsf(lx - b->x) < BODY_HW[b->kind] && fabsf(ly - b->y) < BODY_HH[b->kind]) {
        b->blocked++;
        fpl_sfx("fpl_tink", 4);
        return true;
    }
    for (int k = 0; k < fpl_boss_noptions(); k++) {
        float ox, oy;
        fpl_boss_option(k, &ox, &oy);
        if (fabsf(lx - ox) < 0.5f && fabsf(ly - oy) < 0.5f) { b->blocked++; fpl_sfx("fpl_tink", 4); return true; }
    }
    return false;
}

bool fpl_boss_side_shot(FplPShot *s) {
    FplBoss *b = B();
    if (b->kind != BOSS_KNUCKLEBELL || b->dead) return false;
    for (int k = 0; k < 2; k++) {
        if (fabsf(s->x - b->fx[k]) < FIST_HW + 0.1f && fabsf(s->y - b->fy) < FIST_HH + 0.1f) {
            /* a fist can't be hurt, but each shot knocks it back toward its end */
            float out = k == 0 ? -1.0f : 1.0f;
            b->fvx[k] = out * 0.06f;
            b->fx[k] += out * 0.12f;
            fpl_sfx("fpl_tink", 3);
            return true;
        }
    }
    return false;
}

/* ------------------------------------------------------------------ */

static void fire_cross_at_ship(float x, float y) { fpl_shell_at(x, y, FPL_BOSS_Z, fpg.x, fpg.y, 55, BURST_CROSS); }

static void gloameye(FplBoss *b) {
    b->x = 1.6f * sinf(b->t * 0.011f);
    b->y = -0.2f + 0.75f * sinf(b->t * 0.017f);
    b->ang += 0.02f;
    if (--b->fire_t <= 0) {
        float ox, oy;
        fpl_boss_option(b->sub % 3, &ox, &oy);
        fire_cross_at_ship(ox, oy);
        fpl_sfx("fpl_efire", 0);
        b->sub++;
        b->fired++;
        b->fire_t = b->period = b->angry ? 34 : 68;
    }
}

static void knucklebell(FplBoss *b) {
    /* the square: along the top, down the right, back along the bottom, up the left */
    static const float CX[4] = {-1.4f, 1.4f, 1.4f, -1.4f}, CY[4] = {-0.8f, -0.8f, 0.6f, 0.6f};
    int c = (b->sub + 1) % 4;
    b->x = fapproach(b->x, CX[c], 0.012f);
    b->y = fapproach(b->y, CY[c], 0.012f);
    if (b->x == CX[c] && b->y == CY[c]) b->sub = c;
    /* the fists: they close in along the ship's height, slowly following it */
    float in = b->angry ? 0.018f : 0.012f;
    b->fy = fapproach(b->fy, fpg.y, 0.008f);
    for (int k = 0; k < 2; k++) {
        float dir = k == 0 ? 1.0f : -1.0f;
        if (b->fist_t[k] > 0) {
            /* bouncing back after a clap */
            b->fist_t[k]--;
            b->fx[k] -= dir * 0.11f;
        } else {
            b->fx[k] += dir * in + b->fvx[k];
        }
        b->fvx[k] *= 0.85f;
        if (b->fx[0] < -FIST_HOME) b->fx[0] = -FIST_HOME;
        if (b->fx[1] > FIST_HOME) b->fx[1] = FIST_HOME;
    }
    if (b->fx[1] - b->fx[0] < FIST_HW * 2 + 0.05f) {
        /* they meet: a clap, and back they go */
        b->fist_t[0] = b->fist_t[1] = 22;
        float mid = (b->fx[0] + b->fx[1]) / 2;
        b->fx[0] = mid - FIST_HW - 0.03f;
        b->fx[1] = mid + FIST_HW + 0.03f;
        fpg.shake = 8;
        fpl_sfx("fpl_clap", 0);
    }
    for (int k = 0; k < 2; k++)
        if (fpg.alive && fpg.inv == 0 && fabsf(fpg.x - b->fx[k]) < FIST_HW + FPL_SHIP_R * 0.8f &&
            fabsf(fpg.y - b->fy) < FIST_HH + FPL_SHIP_R * 0.8f)
            fpl_lose_ship(CAUSE_FIST);
    if (--b->fire_t <= 0) {
        fpl_shell_at(b->x, b->y + 0.25f, FPL_BOSS_Z, fpg.x, fpg.y, 60, BURST_NONE);
        fpl_sfx("fpl_efire", 0);
        b->fire_t = b->angry ? 60 : 95;
    }
}

static void inkwell(FplBoss *b) {
    float sp = b->angry ? 1.7f : 1.0f;
    b->ang += 0.012f * sp;
    b->x = 1.8f * sinf(b->ang);
    b->y = -0.4f + 0.6f * sinf(b->ang * 1.6f);
    if (--b->fire_t <= 0) {
        /* a flare down the column the ship is in, or the next one along */
        int i = fpl_spawn_foe(EK_FLARE, iclamp(fpl_col(fpg.x) + (b->sub % 3) - 1, 0, 5), 0, 0);
        (void)i;
        b->sub++;
        b->fire_t = b->angry ? 34 : 64;
        fpl_sfx("fpl_launch", 0);
    }
    if (!b->angry && b->t % 190 == 160) {
        /* two pairs of crosses */
        fpl_shell_at(b->x, b->y, FPL_BOSS_Z, fpg.x - 1.0f, fpg.y - 0.6f, 55, BURST_CROSS);
        fpl_shell_at(b->x, b->y, FPL_BOSS_Z, fpg.x + 1.0f, fpg.y + 0.6f, 55, BURST_CROSS);
        fpl_shell_at(b->x, b->y, FPL_BOSS_Z, fpg.x - 1.0f, fpg.y + 0.6f, 75, BURST_CROSS);
        fpl_shell_at(b->x, b->y, FPL_BOSS_Z, fpg.x + 1.0f, fpg.y - 0.6f, 75, BURST_CROSS);
        b->fired++;
        fpl_sfx("fpl_efire", 0);
    }
}

static void shellback(FplBoss *b) {
    b->ang += b->angry ? 0.014f : 0.01f;
    b->x = 2.0f * sinf(b->ang);
    b->y = -1.05f;
    if (--b->fire_t <= 0) {
        /* a fan: 3, then 4, then 3 */
        static const int N[3] = {3, 4, 3};
        int n = N[b->sub % 3];
        for (int k = 0; k < n; k++) {
            float off = (k - (n - 1) * 0.5f) * 0.95f;
            fpl_shell_at(b->x, b->y, FPL_BOSS_Z, b->x * 0.4f + off, 1.0f + (k % 2) * 0.4f, 64, BURST_NONE);
        }
        b->sub++;
        b->fired++;
        b->last_fan = n;
        b->fire_t = b->sub % 3 == 0 ? (b->angry ? 80 : 120) : 28;
        fpl_sfx("fpl_efire", 0);
    }
    if (b->t % 100 == 50 || (b->angry && b->t % 100 == 0)) {
        /* a pod's one aimed shot */
        float ox, oy;
        fpl_boss_option((b->t / 100) % 2, &ox, &oy);
        fpl_shell_at(ox, oy, FPL_BOSS_Z, fpg.x, fpg.y, 50, BURST_NONE);
    }
}

static void sordina(FplBoss *b) {
    b->ang += 0.009f;
    b->x = 1.5f * sinf(b->ang);
    b->y = -0.3f + 0.6f * sinf(b->ang * 1.5f);
    int wisp_every = b->phase == 2 ? 80 : 150;
    if (b->t % wisp_every == wisp_every - 1) {
        int side = (b->t / wisp_every) % 2 ? -1 : 6;
        fpl_spawn_foe(EK_WISP, side, fpl_row(fpg.y), 0);
    }
    if (b->phase == 0 || b->phase == 1) {
        /* the mouth: shut, then open; a hit shuts it again; left open too long, caltrops */
        if (--b->mouth_t <= 0) {
            if (!b->mouth) {
                b->mouth = true;
                b->shut_by_hit = false;
                b->mouth_t = SORDINA_OPEN_MAX;
                fpl_sfx("fpl_mouth", 0);
            } else {
                if (!b->shut_by_hit) {
                    /* open too long: caltrops fly out at the ship */
                    int c = fpl_col(fpg.x), r = fpl_row(fpg.y);
                    for (int k = -1; k <= 1; k++) {
                        int i = fpl_spawn_foe(EK_CALTROP, iclamp(c + k, 0, 5), iclamp(r + (k ? 1 - (r > 1) * 2 : 0), 0, 3), 0);
                        if (i >= 0) { fpg.foe[i].z = FPL_BOSS_Z; fpg.foe[i].vz = -FPL_BOSS_Z / 45; }
                    }
                    fpl_sfx("fpl_spit", 0);
                }
                b->mouth = false;
                b->mouth_t = SORDINA_SHUT_T;
            }
        }
    }
    if (b->phase == 1) {
        /* she heals to full; then, after a while, Clary comes */
        b->heal_t++;
        if (b->hp < b->maxhp && b->t % 2 == 0) b->hp++;
        if (b->heal_t == CLARY_DELAY) {
            b->clary = true;
            b->clx = -3.6f;
            b->cly = 1.4f;
            b->clary_t = 0;
            fpl_radio("CLARY: HOLD ON, ANSEL! I'M COMING IN!");
            fpl_sfx("fpl_clary", 0);
        }
    }
    if (b->clary) {
        b->clary_t++;
        float tx = fpg.x < 0 ? 1.6f : -1.6f, ty = 1.3f;
        b->clx = fapproach(b->clx, tx, 0.05f);
        b->cly = fapproach(b->cly, ty, 0.03f);
        if (b->phase == 1 && b->clary_t == 70) {
            /* her big shot: down to a third */
            b->phase = 2;
            b->hp = b->maxhp / 3;
            b->flash = 20;
            b->mouth = false;
            fpg.shake = 20;
            fpl_burst(b->x, b->y, FPL_BOSS_Z, C_PINK, 30);
            fpl_sfx("fpl_bigboom", 0);
        }
        if (b->phase == 2 && b->clary_t % 24 == 0 && !b->dead) {
            b->clary_fire++;
            hurt(1);
        }
    }
    if (b->phase == 2 && --b->fire_t <= 0) {
        float ox, oy;
        fpl_boss_option(b->sub % 2, &ox, &oy);
        fire_cross_at_ship(ox, oy);
        b->sub++;
        b->fire_t = 60;
        fpl_sfx("fpl_efire", 0);
    }
}

void fpl_boss_update(void) {
    FplBoss *b = B();
    if (!b->on) return;
    if (b->flash > 0) b->flash--;
    if (b->dead) {
        b->dead_t++;
        if (b->dead_t % 9 == 0 && b->dead_t < FPL_BOSS_FALL_T - 30) {
            fpl_burst(b->x + (rng_float(&g_rng) - 0.5f) * 2, b->y + (rng_float(&g_rng) - 0.5f) * 1.4f, FPL_BOSS_Z, C_ORANGE, 8);
            fpl_sfx("fpl_boom", 0);
        }
        return;
    }
    if (fpg.still) return;
    b->t++;
    switch (b->kind) {
    case BOSS_GLOAMEYE: gloameye(b); break;
    case BOSS_KNUCKLEBELL: knucklebell(b); break;
    case BOSS_INKWELL: inkwell(b); break;
    case BOSS_SHELLBACK: shellback(b); break;
    case BOSS_SORDINA: sordina(b); break;
    default: break;
    }
}
