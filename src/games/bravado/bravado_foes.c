/* BRAVADO - what each monster does: mites and brutes chase, gasbags float
 * or follow, powder kegs charge along the eight ways, stilters walk about
 * and shoot with no warning, peepers sprout under you, and the Pit Boss
 * mixes fans of bullets, charges and pairs of fizzers. With BAIT BOMBS,
 * everything but the peepers goes for the nearest bomb instead of you. */
#include "bravado.h"

static const float DIAG = 0.70710678f;

/* what a monster heads for: the nearest bomb (bait), else the nearest player */
static bool target_of(const Foe *f, float *tx, float *ty, bool bait_ok) {
    float best = 1e9f;
    bool got = false;
    if (bait_ok && bv.gear[GR_BAIT] && f->kind != MK_PEEPER) {
        for (int i = 0; i < BRV_MAX_BOMBS; i++) {
            const Bomb *b = &bv.bomb[i];
            if (!b->alive) continue;
            float d = (b->x - f->x) * (b->x - f->x) + (b->y - f->y) * (b->y - f->y);
            if (d < best) { best = d; *tx = b->x; *ty = b->y; got = true; }
        }
        if (got) return true;
    }
    for (int w = 0; w < 2; w++) {
        const Player *p = &bv.p[w];
        if (!p->on || p->down) continue;
        float d = (p->x - f->x) * (p->x - f->x) + (p->y - f->y) * (p->y - f->y);
        if (d < best) { best = d; *tx = p->x; *ty = p->y; got = true; }
    }
    return got;
}

static void step_toward(Foe *f, float tx, float ty, float speed) {
    float dx = tx - f->x, dy = ty - f->y, l = sqrtf(dx * dx + dy * dy);
    if (l < 0.5f) return;
    f->vx = dx / l * speed;
    f->vy = dy / l * speed;
    f->x += f->vx;
    f->y += f->vy;
}

static float radius_of(const Foe *f) {
    if (f->kind == MK_BOSS) return 14;
    if (f->kind == MK_GASBAG) return f->size == 0 ? 7 : 5;
    return 5;
}

/* keep inside the walls; true if it bumped one */
static bool clamp_foe(Foe *f) {
    float r = radius_of(f);
    float x = fclamp(f->x, BRV_AX0 + r, BRV_AX1 - r), y = fclamp(f->y, BRV_AY0 + r, BRV_AY1 - r);
    bool hit = x != f->x || y != f->y;
    f->x = x;
    f->y = y;
    return hit;
}

void brv_foe_init(Foe *f) {
    switch (f->kind) {
    case MK_STILTER: f->fire_t = rng_range(&g_rng, 70, 130); f->st = 0; break;
    case MK_PEEPER:
        f->state = 0;
        f->st = 45;
        bv.peeper_tone_t = 45;
        brv_sfx("brv_tone", 0);
        break;
    case MK_GASBAG: f->state = 0; f->st = rng_range(&g_rng, 40, 120); break;
    case MK_KEG: f->state = 0; f->st = rng_range(&g_rng, 50, 100); break;
    case MK_BOSS: f->state = 0; f->st = 90; f->fire_t = 0; break;
    default: break;
    }
}

static int line_dir(float dx, float dy) {
    float ax = fabsf(dx), ay = fabsf(dy);
    if (ay < 5) return dx > 0 ? 0 : 4;
    if (ax < 5) return dy > 0 ? 2 : 6;
    if (fabsf(ax - ay) < 5) {
        if (dx > 0) return dy > 0 ? 1 : 7;
        return dy > 0 ? 3 : 5;
    }
    return -1;
}

static void aimed(float x, float y, float tx, float ty, float speed, int n, float spread, int kind) {
    float a = atan2f(ty - y, tx - x);
    for (int k = 0; k < n; k++) {
        float b = a + (n > 1 ? spread * ((float)k - (float)(n - 1) / 2.0f) : 0);
        brv_add_eshot(kind, x, y, cosf(b) * speed, sinf(b) * speed);
    }
}

static void mite(Foe *f) {
    float tx, ty;
    if (!target_of(f, &tx, &ty, true)) return;
    float wob = sinf((float)(f->t + f->dir * 13) * 0.15f) * 6;
    step_toward(f, tx + wob, ty - wob, 0.8f);
    clamp_foe(f);
}

static void brute(Foe *f) {
    float tx, ty;
    if (!target_of(f, &tx, &ty, true)) return;
    step_toward(f, tx, ty, 0.45f);
    clamp_foe(f);
}

static void gasbag(Foe *f) {
    float tx, ty;
    bool has = target_of(f, &tx, &ty, true);
    if (f->state == 2) {
        /* flung off from a split */
        f->x += f->vx;
        f->y += f->vy;
        f->vx *= 0.96f;
        f->vy *= 0.96f;
        if (clamp_foe(f)) { f->vx = -f->vx; f->vy = -f->vy; }
        if (--f->st <= 0) { f->state = 0; f->st = rng_range(&g_rng, 40, 100); }
        return;
    }
    if (--f->st <= 0) {
        f->state = (uint8_t)(rng_chance(&g_rng, 55) ? 1 : 0);
        f->st = f->state ? rng_range(&g_rng, 90, 180) : rng_range(&g_rng, 60, 140);
        if (f->state == 0) {
            float a = rng_float(&g_rng) * 6.2831853f;
            f->vx = cosf(a) * 0.5f;
            f->vy = sinf(a) * 0.5f;
        }
    }
    if (f->state == 1 && has) {
        step_toward(f, tx, ty, f->size == 2 ? 0.6f : 0.45f);
        clamp_foe(f);
    } else {
        f->x += f->vx;
        f->y += f->vy;
        float r = radius_of(f);
        if (f->x < BRV_AX0 + r || f->x > BRV_AX1 - r) f->vx = -f->vx;
        if (f->y < BRV_AY0 + r || f->y > BRV_AY1 - r) f->vy = -f->vy;
        clamp_foe(f);
    }
}

static void keg(Foe *f) {
    float tx = 0, ty = 0;
    bool has = target_of(f, &tx, &ty, true);
    switch (f->state) {
    case 0: { /* rolling about along one of the eight ways */
        float sp = 0.6f * (BRV_DX[f->dir] && BRV_DY[f->dir] ? DIAG : 1.0f);
        f->vx = BRV_DX[f->dir] * sp;
        f->vy = BRV_DY[f->dir] * sp;
        f->x += f->vx;
        f->y += f->vy;
        if (clamp_foe(f) || --f->st <= 0) {
            f->dir = rng_range(&g_rng, 0, 7);
            f->st = rng_range(&g_rng, 50, 100);
        }
        if (has) {
            float dx = tx - f->x, dy = ty - f->y;
            int d = line_dir(dx, dy);
            if (d >= 0 && dx * dx + dy * dy < 200.0f * 200.0f) {
                f->dir = d;
                f->state = 1;
                f->st = 12;
                brv_sfx("brv_keg", 6);
            }
        }
        break;
    }
    case 1: /* the wind-up */
        f->vx = f->vy = 0;
        if (--f->st <= 0) { f->state = 2; f->st = 120; }
        break;
    case 2: { /* the charge, to the wall */
        float sp = 2.6f * (BRV_DX[f->dir] && BRV_DY[f->dir] ? DIAG : 1.0f);
        f->vx = BRV_DX[f->dir] * sp;
        f->vy = BRV_DY[f->dir] * sp;
        f->x += f->vx;
        f->y += f->vy;
        if (clamp_foe(f) || --f->st <= 0) { f->state = 3; f->st = 40; f->vx = f->vy = 0; }
        break;
    }
    default: /* getting its breath back */
        f->vx = f->vy = 0;
        if (--f->st <= 0) { f->state = 0; f->dir = rng_range(&g_rng, 0, 7); f->st = rng_range(&g_rng, 40, 80); }
        break;
    }
}

static void stilter(Foe *f) {
    float tx, ty, px, py;
    bool has = target_of(f, &tx, &ty, true);
    bool see = target_of(f, &px, &py, false);
    if (--f->st <= 0 && has) {
        float a = rng_float(&g_rng) * 6.2831853f, d = (float)rng_range(&g_rng, 55, 95);
        f->tx = fclamp(tx + cosf(a) * d, BRV_AX0 + 10, BRV_AX1 - 10);
        f->ty = fclamp(ty + sinf(a) * d, BRV_AY0 + 10, BRV_AY1 - 10);
        f->st = rng_range(&g_rng, 80, 120);
    }
    if (f->st > 0) step_toward(f, f->tx, f->ty, 0.65f);
    clamp_foe(f);
    /* no wind-up: it just fires */
    if (--f->fire_t <= 0 && see) {
        aimed(f->x, f->y - 3, px, py, 1.6f, 1, 0, ES_PLAIN);
        f->fire_t = rng_range(&g_rng, 100, 160);
        brv_sfx("brv_efire", 4);
    }
}

static void peeper(Foe *f) {
    float px = f->x, py = f->y;
    bool see = target_of(f, &px, &py, false);
    switch (f->state) {
    case 0: /* the tone: a mark where it will come up */
        if (--f->st <= 0) { f->state = 1; f->st = 15; brv_sfx("brv_sprout", 0); }
        break;
    case 1: /* coming up */
        if (--f->st <= 0) { f->state = 2; f->st = 240; f->fire_t = BRV_PEEP_WINDUP; }
        break;
    case 2: /* up for 4 s: its eye glows for nearly 3, then one slow pair */
        if (f->fire_t > 0 && --f->fire_t == 0 && see) {
            aimed(f->x, f->y - 4, px, py, 1.0f, 2, 0.12f, ES_SLOW);
            brv_sfx("brv_efire", 4);
        }
        if (--f->st <= 0) { f->state = 3; f->st = 20; }
        break;
    case 3: /* sinking */
        if (--f->st <= 0) { f->state = 4; f->st = 120; }
        break;
    default: /* gone under; comes up under you again */
        if (--f->st <= 0) {
            if (see) { f->x = px; f->y = py + 2; }
            f->state = 0;
            f->st = 45;
            bv.peeper_tone_t = 45;
            brv_sfx("brv_tone", 0);
        }
        break;
    }
}

static void boss(Foe *f) {
    float tx = BRV_CX, ty = BRV_CY;
    target_of(f, &tx, &ty, true);
    float px = tx, py = ty;
    target_of(f, &px, &py, false);
    switch (f->state) {
    case 0: /* drifting to a new spot */
        if (f->st == 90 || f->tx == 0) {
            f->tx = (float)rng_range(&g_rng, BRV_AX0 + 40, BRV_AX1 - 40);
            f->ty = (float)rng_range(&g_rng, BRV_AY0 + 30, BRV_AY1 - 40);
        }
        step_toward(f, f->tx, f->ty, 0.6f);
        clamp_foe(f);
        if (--f->st <= 0) {
            int pat = f->dir % 3;
            f->dir++;
            f->state = (uint8_t)(1 + pat);
            f->st = pat == 0 ? 120 : pat == 1 ? 50 : 80;
            f->fire_t = 0;
        }
        break;
    case 1: /* three fans of bullets */
        if (f->st % 40 == 0) {
            aimed(f->x, f->y, px, py, 1.3f, 7, 0.21f, ES_BOSS);
            brv_sfx("brv_efire", 0);
        }
        if (--f->st <= 0) { f->state = 0; f->st = 90; f->tx = 0; }
        break;
    case 2: /* a pair of fizzers */
        if (f->st == 25) {
            for (int s = -1; s <= 1; s += 2) brv_spawn_foe(MK_FIZZER, f->x + (float)s * 16, f->y + 6);
            brv_sfx("brv_warp", 0);
        }
        if (--f->st <= 0) { f->state = 0; f->st = 90; f->tx = 0; }
        break;
    default: /* a charge: winds up, then runs at you */
        if (f->st > 50) {
            if (f->st == 51) {
                float dx = px - f->x, dy = py - f->y, l = sqrtf(dx * dx + dy * dy);
                if (l < 1) l = 1;
                f->vx = dx / l * 2.2f;
                f->vy = dy / l * 2.2f;
            } else {
                f->vx = f->vy = 0;
            }
        } else {
            f->x += f->vx;
            f->y += f->vy;
            if (clamp_foe(f)) f->st = 1;
        }
        if (--f->st <= 0) { f->state = 0; f->st = 90; f->tx = 0; f->vx = f->vy = 0; }
        break;
    }
}

static void fizzer(Foe *f) {
    float tx, ty;
    if (!target_of(f, &tx, &ty, true)) return;
    step_toward(f, tx, ty, 1.25f);
    clamp_foe(f);
}

void brv_foes_update(void) {
    for (int i = 0; i < BRV_MAX_FOES; i++) {
        Foe *f = &bv.foe[i];
        if (!f->alive) continue;
        if (f->inv > 0) f->inv--;
        if (f->flash > 0) f->flash--;
        if (f->warp > 0) {
            f->warp--;
            if (f->kind == MK_BOSS) f->y = BRV_AY0 + 22 - (float)f->warp * 0.4f;
            continue;
        }
        f->t++;
        if (f->still) continue;
        switch (f->kind) {
        case MK_MITE: mite(f); break;
        case MK_GASBAG: gasbag(f); break;
        case MK_BRUTE: brute(f); break;
        case MK_KEG: keg(f); break;
        case MK_STILTER: stilter(f); break;
        case MK_PEEPER: peeper(f); break;
        case MK_BOSS: boss(f); break;
        case MK_FIZZER: fizzer(f); break;
        default: break;
        }
    }
}
