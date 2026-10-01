/* BUZZBOLT - the foes: how each formation flies and fires, when foes give
 * up and leave, and the five waves' bosses. All patterns are our own. */
#include "buzzbolt.h"

/* name, base points (times the multiplier), hit points, hitbox half sizes.
 * The points are the original's, one for one; the rest is ours. */
const FoeDef BZZ_FOE[EK_COUNT] = {
    {"GNAT", 10, 2, 4, 3},
    {"IRONBACK", 300, 220, 13, 10},
    {"MIDGE", 10, 2, 3, 3},
    {"WHIRLER", 20, 6, 5, 5},
    {"CRICKET", 200, 40, 6, 6},
    {"BLOATFLY", 400, 900, 18, 12},
    {"BLISTER", 80, 12, 5, 5},
    {"BIG PUFFBALL", 50, 30, 9, 9},
    {"PUFFBALL", 20, 6, 4, 4},
    {"QUEEN TICK", 500, 1300, 24, 14},
    {"ROT WALL", 50, 16, 9, 5},
    {"GOLDBUG", 300, 6, 5, 4},
    {"SCYTHEWING", 600, 1000, 17, 13},
    {"DUSTWING", 400, 650, 15, 10},
    {"SPOREHEART", 0, 2200, 26, 20},
};
#define MIDBOSS_HP 150

float bzz_aim(float x, float y) { return atan2f(bz.py - y, bz.px - x); }

static float shot_speed(float base) { return base + 0.1f * (float)bz.wave; }

static bool on_screen(const Foe *e) { return e->y > -4 && e->y < BZZ_H - 6 && e->x > -4 && e->x < BZZ_W + 4; }

int bzz_spawn_foe(int kind, int form, float x, float y, int idx, int dir, int shots, int arg) {
    for (int i = 0; i < BZZ_MAX_FOES; i++) {
        Foe *e = &bz.foe[i];
        if (e->alive) continue;
        memset(e, 0, sizeof *e);
        const FoeDef *d = &BZZ_FOE[kind];
        e->alive = 1;
        e->kind = (uint8_t)kind;
        e->form = (uint8_t)form;
        e->role = form == FM_BOSS ? ROLE_BOSS : form == FM_MIDBOSS ? ROLE_MIDBOSS : ROLE_FOE;
        e->hp = e->maxhp = form == FM_MIDBOSS ? MIDBOSS_HP : d->hp;
        e->hw = d->hw;
        e->hh = d->hh;
        e->value = d->points;
        e->dir = (int8_t)dir;
        e->shots = (int8_t)shots;
        e->idx = idx;
        e->arg = arg;
        e->x = x;
        e->y = y;
        e->ax = x;
        e->ay = y;
        e->lance = -1;
        e->fire_t = 1 << 30;
        e->state = 1;
        /* gnats that turn up during the second boss are worth ten times more */
        if (kind == EK_GNAT && bz.wave == 1 && bz.boss_out) e->value = 100;
        switch (form) {
        case FM_DIVE:
            e->t = -idx * 11;
            e->y = -10;
            e->x = x + (idx % 2 ? 8 : -8) * (idx > 0);
            break;
        case FM_COLUMN:
            e->t = -idx * 13;
            e->y = -10;
            e->vy = 1.7f;
            e->fire_t = 24 + idx % 3 * 9;
            break;
        case FM_SIDE:
            e->t = -idx * 12;
            e->x = dir > 0 ? -10 : BZZ_W + 10;
            e->vx = dir * 2.1f;
            e->fire_t = 50 + idx * 5;
            break;
        case FM_ARC: {
            float c = arg ? arg : 34;
            e->ax = x + (idx - (shots >> 4)) * c; /* shots' high bits carry the group size */
            e->shots = (int8_t)(shots & 15);
            float off = (e->ax - x) / 120.0f;
            e->ay = y + off * off * 40;
            e->x = e->ax;
            e->y = -12;
            e->t = -idx * 6;
            e->life = 240;
            e->fire_t = 70 + idx * 7;
            break;
        }
        case FM_HOVER:
            e->y = -14;
            e->life = arg ? arg : 300;
            e->fire_t = 60;
            break;
        case FM_DRIFT:
            e->y = y ? y : -12;
            e->vx = dir * 0.35f;
            e->vy = 0.75f + (idx % 3) * 0.15f;
            break;
        case FM_ZIG:
            e->t = -idx * 14;
            e->y = -8;
            e->vy = 1.6f;
            e->vx = (idx % 2 ? 1.2f : -1.2f);
            e->fire_t = 40;
            break;
        case FM_STREAM:
            e->t = -idx * 9;
            e->x = dir > 0 ? -10 : BZZ_W + 10;
            e->fire_t = 90;
            break;
        case FM_WALL:
            e->y = -8;
            e->vy = 1.2f;
            break;
        case FM_PEST:
            e->x = bz.px < BZZ_W / 2 ? BZZ_W + 8 : -8;
            e->y = 70;
            e->fire_t = 50;
            e->shots = 127;
            break;
        default: break;
        }
        return i;
    }
    return -1;
}

/* ------------------------------------------------------------------------ */
/* the spawn list                                                           */

static void spawn_row(const Spawn *s) {
    int bits = s->arg;
    for (int c = 0; c < 16; c++) {
        if (!((bits >> c) & 1)) continue;
        int i = bzz_spawn_foe(EK_ROTWALL, FM_WALL, 10 + c * 20, -8, c, 0, 0, 0);
        if (i < 0) continue;
        /* a gate: these blocks switch on and off, out of step with each other */
        if (s->shots) bz.foe[i].sub = s->shots * 10;
        if (s->shots && ((s->dir >> (c % 4)) & 1)) bz.foe[i].pt = s->shots * 10;
    }
}

void bzz_run_spawns(void) {
    const WaveDef *w = &BZZ_WAVE[bz.wave];
    while (bz.spawn_i < w->count && w->spawns[bz.spawn_i].t <= bz.wave_t) {
        const Spawn *s = &w->spawns[bz.spawn_i++];
        if (s->form == FM_WALL) { spawn_row(s); continue; }
        if (s->form == FM_MIDBOSS) { bzz_spawn_foe(s->kind, FM_MIDBOSS, s->x, s->y, 0, s->dir, s->shots, s->arg); continue; }
        for (int k = 0; k < s->n; k++) {
            float x = s->x, y = s->y;
            if (s->form == FM_DRIFT) x = s->x + k * (s->arg ? s->arg : 40);
            int shots = s->shots;
            if (s->form == FM_ARC) shots = (shots & 15) | (((s->n - 1) / 2) << 4);
            bzz_spawn_foe(s->kind, s->form, x, y, k, s->dir, shots, s->form == FM_ARC ? 0 : s->arg);
        }
    }
    if (!bz.boss_out && bz.wave_t >= w->boss_t) {
        bz.boss_out = true;
        bzz_spawn_boss(bz.wave);
        music_play(bz.wave == BZZ_WAVES - 1 ? BZZ_MUS_FINAL : BZZ_MUS_BOSS);
    }
}

/* ------------------------------------------------------------------------ */
/* the bosses                                                               */

static int boss(int kind, float x, float y) {
    int i = bzz_spawn_foe(kind, FM_BOSS, x, -40, 0, 0, 0, 0);
    if (i >= 0) {
        bz.foe[i].ax = x;
        bz.foe[i].ay = y;
        bz.boss_count++;
    }
    return i;
}

void bzz_spawn_boss(int wave) {
    switch (wave) {
    case 0: {
        int a = boss(EK_IRONBACK, 96, 42), b = boss(EK_IRONBACK, 224, 42);
        if (a >= 0) { bz.foe[a].sub = 0; bz.foe[a].dir = -1; }
        if (b >= 0) { bz.foe[b].sub = 45; bz.foe[b].dir = 1; }
        break;
    }
    case 1: boss(EK_BLOATFLY, 160, 44); break;
    case 2: boss(EK_TICK, 160, 36); break;
    case 3: boss(EK_SCYTHE, 108, 40); boss(EK_DUSTWING, 214, 52); break;
    default: boss(EK_SPOREHEART, 160, 38); bz.pest_t = 60; break;
    }
}

/* a boss glides down to its place first */
static bool boss_enter(Foe *e) {
    if (e->phase > 0) return false;
    e->y += (e->ay - e->y) * 0.05f + 0.3f;
    if (e->y >= e->ay - 1) { e->y = e->ay; e->phase = 1; e->pt = 0; }
    return true;
}

static void ironback_boss(Foe *e) {
    if (boss_enter(e)) return;
    e->pt++;
    e->x = e->ax + sinf(e->pt * 0.018f + (e->dir > 0 ? 3.1f : 0)) * 52;
    e->y = e->ay + sinf(e->pt * 0.031f) * 10;
    /* the pair take turns: spread volleys, and every third a wide double fan */
    int c = (e->pt + e->sub) % 64;
    if (c == 0) {
        e->phase++;
        if (e->phase % 3 == 0) { bzz_aimed(e->x, e->y + 8, shot_speed(1.9f), 11, 10); sfx_play_name("bzz_efire2"); }
        else { bzz_aimed(e->x, e->y + 8, shot_speed(2.3f), 5, 14); sfx_play_name("bzz_efire"); }
    }
    if (e->phase % 3 == 0 && c == 14) bzz_aimed(e->x, e->y + 8, shot_speed(1.9f), 10, 10);
    if (e->phase % 3 == 1 && c == 32) bzz_aimed(e->x, e->y + 8, shot_speed(2.5f), 3, 8);
}

static void bloatfly_boss(Foe *e) {
    if (boss_enter(e)) return;
    e->pt++;
    e->x = e->ax + sinf(e->pt * 0.013f) * 88;
    e->y = e->ay + sinf(e->pt * 0.026f) * 14;
    int cyc = (e->pt / 300) % 3, c = e->pt % 300;
    if (cyc == 0 && c % 34 == 10) {
        /* homing shots from the wings, three at a time */
        for (int k = -1; k <= 1; k++) {
            float a = 1.5708f + k * 1.0f;
            bzz_add_eshot(ES_HOMING, e->x + k * 14, e->y + 6, cosf(a) * shot_speed(2.1f), sinf(a) * shot_speed(2.1f));
        }
        sfx_play_name("bzz_homing");
    } else if (cyc == 1 && c % 50 == 20) {
        for (int k = -2; k <= 2; k++) {
            float a = 1.5708f + k * 0.45f;
            bzz_add_eshot(ES_HOMING, e->x, e->y + 8, cosf(a) * shot_speed(2.0f), sinf(a) * shot_speed(2.0f));
        }
        bzz_aimed(e->x, e->y + 8, shot_speed(2.4f), 5, 12);
        sfx_play_name("bzz_homing");
    } else if (cyc == 2) {
        /* it calls in gnats (worth 100 each while it lives) and sprays */
        if (c == 20) for (int k = 0; k < 5; k++) bzz_spawn_foe(EK_GNAT, FM_SIDE, 0, 70, k, 1, 2, 0);
        if (c == 110) for (int k = 0; k < 5; k++) bzz_spawn_foe(EK_GNAT, FM_SIDE, 0, 92, k, -1, 2, 0);
        if (c % 60 == 40) bzz_ring(e->x, e->y + 6, shot_speed(1.7f), 18, e->pt * 0.1f, ES_AIMED);
    }
}

static void tick_boss(Foe *e) {
    if (boss_enter(e)) return;
    e->pt++;
    e->x = e->ax + sinf(e->pt * 0.017f) * 100;
    e->y = e->ay + sinf(e->pt * 0.043f) * 4;
    bool angry = e->hp * 2 < e->maxhp;
    if (e->pt % 44 == 10) {
        /* rings that bounce off the bottom of the screen */
        int n = angry ? 4 : 3;
        for (int k = 0; k < n; k++) {
            float vx = (k - (n - 1) / 2.0f) * 1.1f;
            bzz_add_eshot(ES_RING, e->x, e->y + 10, vx, shot_speed(2.0f));
        }
        sfx_play_name("bzz_ring");
    }
    if (e->pt % 60 == 36) { bzz_aimed(e->x, e->y + 12, shot_speed(2.5f), 9, 10); sfx_play_name("bzz_efire"); }
    if (e->pt % 100 == 70) bzz_ring(e->x, e->y + 8, shot_speed(1.7f), angry ? 24 : 16, e->pt * 0.07f, ES_AIMED);
}

static void scythe_boss(Foe *e) {
    if (boss_enter(e)) return;
    e->pt++;
    e->x = e->ax + sinf(e->pt * 0.015f) * 58;
    e->y = e->ay + sinf(e->pt * 0.028f) * 8;
    if (e->pt % 72 == 20) {
        /* big homing shots, heavier than the bloatfly's */
        bzz_add_eshot(ES_BIG, e->x - 14, e->y + 8, -1.2f, shot_speed(1.9f));
        bzz_add_eshot(ES_BIG, e->x + 14, e->y + 8, 1.2f, shot_speed(1.9f));
        sfx_play_name("bzz_homing");
    }
    if (e->pt % 110 == 56) {
        /* a scything sweep */
        bzz_aimed(e->x, e->y + 10, shot_speed(2.4f), 11, 9);
        sfx_play_name("bzz_efire2");
    }
}

static void dustwing_boss(Foe *e) {
    if (boss_enter(e)) return;
    e->pt++;
    e->x = e->ax + sinf(e->pt * 0.023f) * 68;
    e->y = e->ay + sinf(e->pt * 0.05f) * 16;
    if (e->pt % 54 == 5) { bzz_add_eshot(ES_HOMING, e->x, e->y + 6, 0, shot_speed(2.1f)); sfx_play_name("bzz_homing"); }
    if (e->pt % 100 == 40) { bzz_ring(e->x, e->y, 1.25f, 16, e->pt * 0.2f, ES_SPORE); sfx_play_name("bzz_puff"); }
}

static void sporeheart_boss(Foe *e) {
    if (boss_enter(e)) return;
    e->pt++;
    e->x = e->ax + sinf(e->pt * 0.011f) * 14;
    int stage = e->hp * 3 > e->maxhp * 2 ? 0 : e->hp * 3 > e->maxhp ? 1 : 2;
    float rot = e->pt * 0.19f;
    if (stage == 0) {
        /* a turning spray of spores, and volleys aimed at the ship */
        if (e->pt % 5 == 0)
            for (int k = 0; k < 3; k++) bzz_add_eshot(ES_SPORE, e->x, e->y + 6, cosf(rot + k * 2.0944f) * 1.7f, sinf(rot + k * 2.0944f) * 1.7f);
        if (e->pt % 70 == 50) { bzz_aimed(e->x, e->y + 14, shot_speed(2.4f), 7, 11); sfx_play_name("bzz_efire"); }
    } else if (stage == 1) {
        if (e->pt % 40 == 0) { bzz_ring(e->x, e->y + 6, 1.7f, 22, e->pt * 0.05f, ES_SPORE); sfx_play_name("bzz_puff"); }
        if (e->pt % 30 == 15) bzz_aimed(e->x, e->y + 14, shot_speed(2.5f), 3, 10);
    } else {
        if (e->pt % 6 == 0)
            for (int k = 0; k < 4; k++) bzz_add_eshot(ES_SPORE, e->x, e->y + 6, cosf(-rot * 0.8f + k * 1.5708f) * 1.7f, sinf(-rot * 0.8f + k * 1.5708f) * 1.7f);
        if (e->pt % 80 == 30) {
            for (int k = -1; k <= 1; k += 2)
                bzz_add_eshot(ES_HOMING, e->x + k * 20, e->y + 10, k * 0.9f, 2.0f);
            bzz_aimed(e->x, e->y + 14, shot_speed(2.4f), 5, 12);
            sfx_play_name("bzz_homing");
        }
    }
}

/* ------------------------------------------------------------------------ */
/* ordinary foes                                                            */

static void fire_kind(Foe *e) {
    switch (e->kind) {
    case EK_GNAT: bzz_aimed(e->x, e->y + 3, shot_speed(2.3f), 1, 0); break;
    case EK_MIDGE: bzz_aimed(e->x, e->y + 3, shot_speed(2.0f), 1, 0); break;
    case EK_WHIRLER: bzz_aimed(e->x, e->y + 4, shot_speed(2.4f), 3, 13); break;
    case EK_CRICKET: {
        float a = bzz_aim(e->x, e->y);
        for (int k = -1; k <= 1; k += 2)
            bzz_add_eshot(ES_HOMING, e->x + k * 4, e->y + 4, cosf(a + k * 0.7f) * shot_speed(2.2f), sinf(a + k * 0.7f) * shot_speed(2.2f));
        bzz_aimed(e->x, e->y + 4, shot_speed(2.4f), 3, 16);
        sfx_play_name("bzz_homing");
        return;
    }
    case EK_BLISTER:
        if (e->sub++ % 2 == 0) bzz_ring(e->x, e->y, shot_speed(1.7f), 16, e->t * 0.13f, ES_AIMED);
        else bzz_aimed(e->x, e->y + 4, shot_speed(2.4f), 7, 11);
        break;
    case EK_IRONBACK:
        bzz_aimed(e->x, e->y + 8, shot_speed(2.1f), e->sub++ % 2 ? 9 : 7, 11);
        break;
    case EK_GOLDBUG: bzz_aimed(e->x, e->y + 3, shot_speed(2.2f), 1, 0); break;
    default: return;
    }
    sfx_play_name("bzz_efire");
}

static void move_foe(Foe *e) {
    switch (e->form) {
    case FM_DIVE:
        if (e->phase == 0) {
            e->vx = 0;
            e->vy = 2.4f;
            if (e->y >= e->ay) { e->phase = 1; e->fire_t = e->t; }
        } else {
            e->vx = fclamp(e->vx + e->dir * 0.13f, -3.0f, 3.0f);
            e->vy = fmaxf(e->vy - 0.07f, -1.6f);
        }
        break;
    case FM_COLUMN: break;
    case FM_SIDE:
        e->y = e->ay + sinf(e->t * 0.07f) * 12;
        break;
    case FM_ARC:
        if (e->t == e->life && on_screen(e) && bz.alive) fire_kind(e); /* a parting shot */
        if (e->t < e->life) {
            e->x += (e->ax - e->x) * 0.08f;
            e->y += (e->ay + sinf(e->t * 0.05f + e->idx) * 3 - e->y) * 0.07f;
            e->vx = e->vy = 0;
        } else {
            e->vy = fmaxf(e->vy - 0.08f, -2.4f);
        }
        break;
    case FM_HOVER:
        if (e->t == e->life && e->dir && on_screen(e) && bz.alive) fire_kind(e); /* a parting shot */
        if (e->t < e->life) {
            float sway = e->dir ? 1.0f : 0.0f; /* a test dummy (dir 0) sits still */
            e->x += (e->ax + sinf(e->t * 0.03f) * 30 * sway - e->x) * 0.05f;
            e->y += (e->ay + sinf(e->t * 0.06f) * 6 * sway - e->y) * 0.05f;
            e->vx = e->vy = 0;
        } else {
            e->vy = fmaxf(e->vy - 0.06f, -2.0f);
            e->vx = e->dir * 0.8f;
        }
        break;
    case FM_ZIG:
        if (e->t % 26 == 25) e->vx = -e->vx;
        break;
    case FM_STREAM: {
        /* a long sweeping loop over the top half, then away */
        float u = e->t * 0.021f;
        float sx = e->dir > 0 ? -10 + e->t * 1.9f : BZZ_W + 10 - e->t * 1.9f;
        e->vx = sx - e->x;
        e->vy = (e->ay + sinf(u * 2.0f) * 38 + (1 - cosf(u)) * 10) - e->y;
        break;
    }
    case FM_WALL: break;
    case FM_PEST: {
        /* hangs about near the ship, weaving, and pesters it */
        float tx = bz.px + sinf(e->t * 0.02f) * 70, ty = fmaxf(26, bz.py - 60 + sinf(e->t * 0.05f) * 14);
        e->vx = fclamp((tx - e->x) * 0.04f, -1.8f, 1.8f);
        e->vy = fclamp((ty - e->y) * 0.04f, -1.5f, 1.5f);
        break;
    }
    case FM_MIDBOSS:
        if (e->t < 60) { e->y += (e->ay - e->y) * 0.06f + 0.3f; e->x = e->ax; }
        else if (e->t < (e->arg ? e->arg : 900)) {
            e->x = e->ax + sinf((e->t - 60) * 0.02f) * 60 * e->dir;
            e->y = e->ay + sinf(e->t * 0.04f) * 6;
            if (e->t % 44 == 0) fire_kind(e);
        } else {
            e->leaving = 1;
        }
        e->vx = e->vy = 0;
        break;
    default: break;
    }
}

void bzz_foes_update(void) {
    for (int i = 0; i < BZZ_MAX_FOES; i++) {
        Foe *e = &bz.foe[i];
        if (!e->alive) continue;
        if (e->flash > 0) e->flash--;
        if (e->t < 0) { e->t++; continue; }
        e->t++;
        if (e->kind == EK_ROTWALL && e->sub > 0) {
            /* a gate: on for a while (solid, can be shot), then off (harmless) */
            if (++e->pt >= e->sub * 2) e->pt = 0;
            e->state = e->pt < e->sub;
        }
        if (e->role == ROLE_BOSS && !e->leaving) {
            switch (e->kind) {
            case EK_IRONBACK: ironback_boss(e); break;
            case EK_BLOATFLY: bloatfly_boss(e); break;
            case EK_TICK: tick_boss(e); break;
            case EK_SCYTHE: scythe_boss(e); break;
            case EK_DUSTWING: dustwing_boss(e); break;
            default: sporeheart_boss(e); break;
            }
            continue;
        }
        if (e->leaving) {
            /* giving up: off the way it came, or up and away */
            if (e->form == FM_SIDE || e->form == FM_STREAM) e->vx = e->vx ? e->vx * 1.03f : 2.0f;
            else e->vy = fmaxf(e->vy - 0.1f, -2.6f);
        } else {
            move_foe(e);
            if (e->shots > 0 && e->t >= e->fire_t && on_screen(e) && bz.alive) {
                fire_kind(e);
                e->shots--;
                e->fire_t = e->t + (e->kind == EK_BLISTER ? 36 : e->kind == EK_CRICKET ? 60 : e->kind == EK_WHIRLER ? 22 : 16);
            }
        }
        e->x += e->vx;
        e->y += e->vy;
        /* gone off an edge: it got away, and its letter with it */
        bool out = e->y > BZZ_H + 16 || e->y < -40 || e->x < -30 || e->x > BZZ_W + 30;
        if (out && (e->t > 30 || e->leaving)) {
            if (e->kind == EK_GNAT && e->form == FM_PEST) bz.pest_t = 180;
            e->alive = 0;
            bz.escaped++;
        }
    }
    /* wave 5: a gnat keeps pestering the ship through the last fight */
    if (bz.wave == BZZ_WAVES - 1 && bz.boss_out && !bz.boss_dead) {
        bool pest = false;
        for (int i = 0; i < BZZ_MAX_FOES; i++) pest |= bz.foe[i].alive && bz.foe[i].form == FM_PEST;
        if (!pest && bz.pest_t > 0 && --bz.pest_t == 0) bzz_spawn_foe(EK_GNAT, FM_PEST, 0, 0, 0, 0, 0, 0);
        if (!pest && bz.pest_t == 0) bz.pest_t = 150;
    }
}
