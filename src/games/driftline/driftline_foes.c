/* DRIFTLINE - the foes of the four stages, stage 4's swells, and the four
 * bosses. Every foe and boss is ours; how each one behaves follows the
 * kind of foe the original puts in that stage (see the design document). */
#include "driftline.h"

/*                     name        points  hp   hw  hh ground */
const DflFoeDef DFL_FOE[FK_COUNT] = {
    [FK_KITE] = {"KITE", 100, 2, 8, 5, 0},
    [FK_BUZZER] = {"BUZZER", 50, 1, 5, 5, 0},
    [FK_HOG] = {"ROAD HOG", 0, 4, 12, 5, 1},
    [FK_WRECK] = {"WRECK", 500, 4, 12, 5, 1},
    [FK_ROTOR] = {"ROTOR", 500, 10, 11, 6, 0},
    [FK_SHARD] = {"SHARD", 75, 2, 6, 6, 0},
    [FK_PRISM] = {"PRISM", 1000, 24, 8, 10, 0},
    [FK_HOOP] = {"HOOP", 500, 8, 8, 8, 0},
    [FK_TUMBLER] = {"TUMBLER", 1000, 8, 7, 7, 1},
    [FK_SKULL] = {"SKULL", 250, 3, 7, 7, 0},
    [FK_SHEET] = {"SHEET", 500, 8, 8, 9, 0},
    [FK_SNAPPER] = {"SNAPPER", 500, 10, 8, 8, 0},
    [FK_SLAB] = {"SLAB", 1500, 30, 12, 12, 0},
    [FK_SAUCER] = {"SAUCER", 3000, 80, 19, 7, 0},
    [FK_JELLY] = {"JELLY", 100, 2, 6, 7, 0},
    [FK_DARTFISH] = {"DARTFISH", 200, 3, 7, 4, 0},
    [FK_SQUIRT] = {"SQUIRT", 1000, 14, 9, 8, 0},
    [FK_SHARK] = {"SHARK", 5000, 50, 16, 7, 0},
    [FK_PLANET] = {"PLANET", 0, 9999, 6, 6, 0},
};

const int DFL_BOSS_POINTS[DFL_STAGES] = {10000, 20000, 30000, 40000};

#define PI_F 3.14159265f

static float aim_x(float x) {
    int p = dfl_nearest_car(x);
    return p >= 0 ? dfg.car[p].x : 160.0f;
}

int dfl_spawn_foe(int kind, float x, float y, int dir, int arg) {
    for (int i = 0; i < DFL_MAX_FOES; i++) {
        DflFoe *e = &dfg.foe[i];
        if (e->alive) continue;
        memset(e, 0, sizeof *e);
        e->alive = 1;
        e->kind = (uint8_t)kind;
        e->ground = DFL_FOE[kind].ground;
        e->hp = DFL_FOE[kind].hp;
        e->x = x;
        e->y = y;
        e->ax = x;
        e->ay = y;
        e->dir = dir ? dir : 1;
        e->arg = arg;
        return i;
    }
    return -1;
}

/* where a foe of this kind starts, given its spawn entry */
static void place(DflFoe *e, const DflSpawn *s, int idx) {
    float side = s->dir > 0 ? -16.0f : SCREEN_W + 16.0f;
    float road = (float)(DFL_ROAD_Y - DFL_FOE[e->kind].hh);
    switch (e->kind) {
    case FK_KITE:
        e->x = side;
        e->y = s->y;
        e->ax = s->x + idx * s->arg * s->dir * -1.0f;
        e->ay = s->y + (idx % 2) * 10;
        break;
    case FK_ROTOR:
        e->x = side;
        e->y = s->y - 30;
        e->ax = s->x + idx * s->arg;
        e->ay = s->y;
        break;
    case FK_HOG:
    case FK_TUMBLER:
        e->x = side;
        e->y = road;
        e->vx = s->dir * (e->kind == FK_HOG ? s->arg / 10.0f : 1.1f);
        break;
    case FK_SQUIRT:
    case FK_SAUCER:
        e->x = side;
        e->y = s->y + (idx % 2) * 14;
        break;
    case FK_SHARK:
        e->x = s->dir > 0 ? 30.0f : SCREEN_W - 30.0f;
        e->y = 140; /* out at sea, behind the road */
        e->state = 0;
        break;
    default: /* from the top */
        e->x = s->x + idx * s->arg;
        e->y = -10;
        e->ax = e->x;
        e->ay = s->y;
        break;
    }
}

void dfl_run_spawns(void) {
    const DflStage *st = &DFL_STAGE[dfg.stage];
    while (dfg.spawn_i < st->count && st->spawns[dfg.spawn_i].t <= dfg.stage_t) {
        const DflSpawn *s = &st->spawns[dfg.spawn_i++];
        for (int k = 0; k < s->n; k++) {
            int i = dfl_spawn_foe(s->kind, 0, 0, s->dir, s->arg);
            if (i < 0) break;
            DflFoe *e = &dfg.foe[i];
            place(e, s, k);
            e->t = -k * s->gap; /* waits its turn */
            dfg.foes_stage++;
        }
    }
}

void dfl_kill_foe(int i, bool scored) {
    DflFoe *e = &dfg.foe[i];
    if (!e->alive) return;
    e->alive = 0;
    int pts = e->arg == DFL_ADD ? 0 : DFL_FOE[e->kind].points; /* a boss's adds pay nothing */
    if (scored && pts > 0) {
        dfl_add_score((uint32_t)pts);
        dfg.kills++;
        dfg.kills_stage++;
        dfg.stage_points += pts;
    }
    int col = e->kind == FK_SKULL || e->kind == FK_SNAPPER ? C_VIOLET : e->kind >= FK_JELLY ? C_CYAN : C_ORANGE;
    dfl_burst(e->x, e->y, col, 10 + DFL_FOE[e->kind].hw, 1.9f);
    dfl_burst(e->x, e->y, C_WHITE, 4, 1.0f);
    dfl_sfx(pts >= 1000 ? "dfl_boom" : "dfl_pop2", 3);
    if (e->kind == FK_SHARD) {
        /* shrapnel, flung out both ways */
        dfl_add_eshot(ES_SHRAPNEL, e->x, e->y, -1.3f, -1.5f);
        dfl_add_eshot(ES_SHRAPNEL, e->x, e->y, 1.3f, -1.5f);
        dfl_add_eshot(ES_SHRAPNEL, e->x, e->y, -0.7f, 0.2f);
        dfl_add_eshot(ES_SHRAPNEL, e->x, e->y, 0.7f, 0.2f);
    }
}

static void escape(DflFoe *e) {
    e->alive = 0;
    if (DFL_FOE[e->kind].points > 0 && e->arg != DFL_ADD) dfg.escaped++;
}

void dfl_hurt_foe(int i, int dmg, bool side, float svx) {
    DflFoe *e = &dfg.foe[i];
    switch (e->kind) {
    case FK_PLANET:
        /* knocked off the Orrery: it can't be broken, only juggled */
        e->vy = -3.4f;
        e->vx = fclamp(e->vx + svx * 0.12f, -2.0f, 2.0f);
        dfl_sfx("dfl_clink", 3);
        return;
    case FK_TUMBLER:
        if (e->ground) {
            /* on the road the main gun can't touch it; a side shot pops it up */
            if (side) {
                e->ground = 0;
                e->vy = -5.2f;
                dfl_sfx("dfl_clink", 3);
            }
            return;
        }
        /* in the air every hit throws it higher (and on, the way the shot went) */
        e->vy = fminf(e->vy, -2.4f);
        e->vx = fclamp(e->vx + svx * 0.1f, -2.0f, 2.0f);
        break;
    case FK_PRISM:
        /* every hit bounces it back toward its own side */
        e->x += e->x < 160 ? -2.5f : 2.5f;
        e->y = fmaxf(16.0f, e->y - 0.6f);
        break;
    case FK_SNAPPER:
        e->vx = fclamp(e->vx + svx * 0.18f, -2.5f, 2.5f);
        e->vy = fmaxf(e->vy - 0.9f, -2.5f);
        break;
    default: break;
    }
    e->hp -= dmg;
    e->flash = 4;
    if (e->hp > 0) { dfl_sfx("dfl_hit", 3); return; }
    if (e->kind == FK_HOG) {
        /* a hit car becomes a burning wreck; only the wreck pays */
        e->kind = FK_WRECK;
        e->hp = DFL_FOE[FK_WRECK].hp;
        e->vx = -DFL_SCROLL * 0.7f;
        e->t = 0;
        dfl_burst(e->x, e->y, C_ORANGE, 8, 1.5f);
        dfl_sfx("dfl_crunch", 3);
        return;
    }
    dfl_kill_foe(i, true);
}

static bool offscreen(const DflFoe *e, float m) {
    return e->x < -m || e->x > SCREEN_W + m || e->y < -40 || e->y > SCREEN_H + 20;
}

/* ---- each kind's frame ------------------------------------------------------------------ */

static void kite(DflFoe *e) {
    switch (e->state) {
    case 0: /* fly in to its spot */
        e->x += e->dir * 1.6f;
        e->y = fapproach(e->y, e->ay, 0.5f);
        if ((e->dir > 0 && e->x >= e->ax) || (e->dir < 0 && e->x <= e->ax)) { e->state = 1; e->sub = 0; }
        break;
    case 1: /* loiter; left too long, it starts shooting */
        e->sub++;
        e->x = e->ax + sinf(e->sub * 0.035f) * 26 * e->dir;
        e->y = e->ay + sinf(e->sub * 0.07f) * 7;
        if (e->sub > 280 && (e->sub - 281) % 60 == 0) { dfl_aimed(e->x, e->y + 4, 1.8f, 1, 0); dfl_sfx("dfl_efire", 4); }
        if (e->sub > 640) e->state = 2;
        break;
    default:
        e->x += e->dir * 2.2f;
        e->y -= 0.3f;
        if (offscreen(e, 20)) escape(e);
        break;
    }
}

static void buzzer(DflFoe *e) {
    switch (e->state) {
    case 0:
        e->y += 1.2f;
        if (e->y >= e->ay) { e->state = 1; e->sub = 0; }
        break;
    case 1: /* hovering; left too long, it bursts */
        e->sub++;
        e->y = e->ay + sinf(e->sub * 0.1f) * 3;
        e->x -= 0.25f;
        if (e->sub > 200) { e->state = 2; e->sub = 0; }
        break;
    default:
        e->sub++;
        if (e->sub > 50) {
            dfl_ring(e->x, e->y, 1.5f, 6, PI_F / 6);
            dfl_burst(e->x, e->y, C_ORANGE, 8, 1.6f);
            dfl_sfx("dfl_pop2", 3);
            e->alive = 0;
            if (e->arg != DFL_ADD) dfg.escaped++;
        }
        break;
    }
    if (offscreen(e, 20)) escape(e);
}

static void road_car(DflFoe *e) {
    e->x += e->vx;
    if (e->kind == FK_WRECK && (e->t & 7) == 0) dfl_burst(e->x - 4, e->y - 4, C_GREY, 1, 0.5f);
    if (offscreen(e, 24)) escape(e);
}

static void rotor(DflFoe *e) {
    switch (e->state) {
    case 0:
        e->x = fapproach(e->x, e->ax, 1.5f);
        e->y = fapproach(e->y, e->ay, 0.8f);
        if (fabsf(e->x - e->ax) < 1 && fabsf(e->y - e->ay) < 1) { e->state = 1; e->sub = 0; }
        break;
    case 1:
        e->sub++;
        e->x = e->ax + sinf(e->sub * 0.03f) * 14;
        e->y = e->ay + sinf(e->sub * 0.05f) * 4;
        if (e->sub % 110 == 40 || e->sub % 110 == 50 || e->sub % 110 == 60) { dfl_aimed(e->x, e->y + 6, 2.0f, 1, 0); dfl_sfx("dfl_efire", 4); }
        if (e->sub > 560) e->state = 2;
        break;
    default:
        e->y -= 1.2f;
        e->x += e->dir * 0.6f;
        if (offscreen(e, 20)) escape(e);
        break;
    }
}

static void shard(DflFoe *e) {
    float tx = aim_x(e->x);
    if (!e->ground) {
        e->vx = fapproach(e->vx, tx > e->x ? 0.7f : -0.7f, 0.02f);
        e->x += e->vx;
        e->y += 0.7f;
        if (e->y >= DFL_ROAD_Y - 6) { e->y = DFL_ROAD_Y - 6; e->ground = 1; e->dir = tx > e->x ? 1 : -1; }
        return;
    }
    /* down on the road it keeps coming at the car, then slides on away */
    e->vx = fapproach(e->vx, e->dir * 0.9f, 0.04f);
    e->x += e->vx;
    if (offscreen(e, 16)) escape(e);
}

/* a prism's beam: on while state is 2 */
static void prism(DflFoe *e) {
    if (e->state == 0 && e->y < e->ay) { e->y += 1.5f; return; }
    e->sub++;
    int c = e->sub % 220;
    if (c < 60) e->state = 1;        /* hover */
    else if (c < 100) e->state = 3;  /* glowing: about to fire */
    else {
        if (e->state != 2) dfl_sfx("dfl_beam", 20);
        e->state = 2;                /* the beam walks inward */
        e->x += e->x < 160 ? 0.9f : -0.9f;
    }
    if (c == 219) e->fire_t++;
    if (e->fire_t >= (e->arg > 0 ? e->arg : 3)) {
        e->state = 4;
        e->y -= 1.5f;
        if (e->y < -20) escape(e);
    }
    /* the beam, top to road */
    if (e->state == 2)
        for (int p = 0; p < DFL_CARS; p++)
            if (dfl_car_hit_rect(p, e->x - 3, e->y + 10, e->x + 3, DFL_ROAD_Y)) dfl_kill_car(p, CAUSE_BEAM);
}

static void hoop(DflFoe *e) {
    if (e->state == 0) { e->vx = e->dir * 1.0f; e->state = 1; }
    e->vy += 0.09f;
    e->x += e->vx;
    e->y += e->vy;
    if (e->y + 8 >= DFL_ROAD_Y) { e->y = DFL_ROAD_Y - 8; e->vy = -4.4f; e->sub++; dfl_sfx("dfl_bounce", 4); }
    if (e->sub < 8) {
        if (e->x < 10) { e->x = 10; e->vx = fabsf(e->vx); }
        if (e->x > SCREEN_W - 10) { e->x = SCREEN_W - 10; e->vx = -fabsf(e->vx); }
    } else if (offscreen(e, 20)) escape(e);
}

static void tumbler(DflFoe *e) {
    e->x += e->vx;
    if (!e->ground) {
        e->vy += 0.14f;
        e->y += e->vy;
        float road = (float)(DFL_ROAD_Y - 7);
        if (e->y >= road) { e->y = road; e->vy = 0; e->ground = 1; dfl_sfx("dfl_thud", 4); }
    }
    if (offscreen(e, 24)) escape(e);
}

static void skull(DflFoe *e) {
    e->sub++;
    if (e->sub > 700) {
        e->vy = fapproach(e->vy, -1.2f, 0.05f);
    } else {
        float tx = aim_x(e->x), ty = DFL_CAR_TOP + 4;
        float a = atan2f(ty - e->y, tx - e->x);
        e->vx = fapproach(e->vx, cosf(a) * 0.85f, 0.03f);
        e->vy = fapproach(e->vy, sinf(a) * 0.85f, 0.03f);
    }
    e->x += e->vx;
    e->y += e->vy;
    if (e->y > DFL_ROAD_Y - 7) e->y = DFL_ROAD_Y - 7;
    if (e->sub > 700 && offscreen(e, 20)) escape(e);
}

static void sheet(DflFoe *e) {
    if (e->y < e->ay && e->sub == 0) { e->y += 1.2f; return; }
    e->sub++;
    e->x = e->ax + sinf(e->sub * 0.02f) * 40;
    int c = e->sub % 200;
    e->state = c >= 150 ? 2 : 1; /* fading through: can't be hit or touched */
    if (e->state == 1 && (c == 60 || c == 72 || c == 120 || c == 132)) {
        float tx = aim_x(e->x);
        float s = tx >= e->x ? 1.0f : -1.0f;
        dfl_add_eshot(ES_FLARE, e->x, e->y + 6, s * 1.4f, 1.4f); /* always 45 degrees down */
        dfl_sfx("dfl_efire", 4);
    }
    if (e->sub > 900) { e->ay = -40; e->y -= 1.0f; if (e->y < -20) escape(e); }
}

static void snapper(DflFoe *e) {
    if (e->state == 0 && e->y < e->ay) { e->y += 1.4f; return; }
    if (e->state == 0) e->state = 1;
    e->sub++;
    int lunge = e->state == 1 ? 60 : 36;
    if (e->sub % lunge == 0) {
        int p = dfl_nearest_car(e->x);
        float tx = p >= 0 ? dfg.car[p].x : 160, ty = DFL_CAR_TOP + 2;
        float a = atan2f(ty - e->y, tx - e->x), sp = e->state == 1 ? 2.2f : 2.8f;
        e->vx = cosf(a) * sp;
        e->vy = sinf(a) * sp;
        dfl_sfx("dfl_snap", 6);
    }
    e->vx *= 0.97f;
    e->vy *= 0.97f;
    e->x += e->vx;
    e->y += e->vy;
    if (e->y > DFL_ROAD_Y - 8) { e->y = DFL_ROAD_Y - 8; e->vy = -fabsf(e->vy); }
    if (e->y < 16) { e->y = 16; e->vy = fabsf(e->vy); }
    if (e->x < 8) { e->x = 8; e->vx = fabsf(e->vx); }
    if (e->x > SCREEN_W - 8) { e->x = SCREEN_W - 8; e->vx = -fabsf(e->vx); }
    if (e->state == 1 && e->sub >= 300) e->state = 2;   /* blue, then purple... */
    if (e->state == 2 && e->sub >= 420) {                /* ...then it bursts */
        dfl_ring(e->x, e->y, 1.6f, 8, 0);
        dfl_burst(e->x, e->y, C_MAGENTA, 12, 2.0f);
        dfl_sfx("dfl_pop2", 3);
        e->alive = 0;
        if (e->arg != DFL_ADD) dfg.escaped++;
    }
}

static void slab(DflFoe *e) {
    float top = 28;
    switch (e->state) {
    case 0: /* in from the top */
        e->y += 1.0f;
        if (e->y >= top) { e->y = top; e->state = 1; e->sub = 0; }
        break;
    case 1: /* follow the car along the top */
        e->sub++;
        e->x = fapproach(e->x, aim_x(e->x), 0.6f);
        if (e->sub > 40 && fabsf(aim_x(e->x) - e->x) < 10) { e->state = 2; e->sub = 0; }
        break;
    case 2: /* shake */
        e->sub++;
        if (e->sub > 30) { e->state = 3; e->vy = 0; }
        break;
    case 3: /* drop */
        e->vy = fminf(e->vy + 0.4f, 6.0f);
        e->y += e->vy;
        if (e->y >= DFL_ROAD_Y - 12) {
            e->y = DFL_ROAD_Y - 12;
            e->state = 4;
            e->sub = 0;
            dfg.shake = 8;
            dfl_burst(e->x, DFL_ROAD_Y, C_GREY, 10, 1.6f);
            dfl_sfx("dfl_slam", 4);
        }
        break;
    case 4: /* sit on the road */
        e->sub++;
        if (e->sub > 40) { e->state = 5; e->fire_t++; }
        break;
    default: /* back up, and again, three times */
        e->y -= 1.2f;
        if (e->fire_t >= 3) { if (e->y < -20) escape(e); }
        else if (e->y <= top) { e->y = top; e->state = 1; e->sub = 0; }
        break;
    }
}

static void saucer(DflFoe *e) {
    e->sub++;
    e->x += e->dir * 0.55f;
    e->y = e->ay + sinf(e->sub * 0.05f) * 3;
    int c = e->sub % 45;
    if (e->sub > 40 && (c == 0 || c == 6 || c == 12)) { dfl_add_eshot(ES_PELLET, e->x, e->y + 6, 0, 2.4f); dfl_sfx("dfl_efire", 4); }
    if (e->sub > 60 && offscreen(e, 24)) escape(e);
}

static void jelly(DflFoe *e) {
    e->sub++;
    if (e->sub < 520) e->y = fapproach(e->y, e->ay, 0.5f);
    else e->y -= 0.6f;
    e->x = e->ax + sinf(e->sub * 0.05f) * 12;
    e->ax -= 0.15f;
    if (e->sub > 520 && offscreen(e, 20)) escape(e);
    if (e->x < -20) escape(e);
}

static void dartfish(DflFoe *e) {
    e->sub++;
    if (!e->ground) {
        e->vx = ((e->sub / 24) % 2 ? -1.4f : 1.4f) * e->dir;
        e->vy = 1.1f;
        e->x += e->vx;
        e->y += e->vy;
        if (e->y >= DFL_ROAD_Y - 4) { e->y = DFL_ROAD_Y - 4; e->ground = 1; e->state = 1; e->vx = 0; e->fire_t = e->sub; }
    } else if (e->sub - e->fire_t < 500) {
        float tx = aim_x(e->x);
        e->vx = fapproach(e->vx, tx > e->x ? 1.7f : -1.7f, 0.06f);
        e->x += e->vx;
    } else {
        e->vx = fapproach(e->vx, -2.0f, 0.06f);
        e->x += e->vx;
        if (offscreen(e, 20)) escape(e);
    }
    e->x = e->ground && e->sub - e->fire_t < 500 ? fclamp(e->x, 4, SCREEN_W - 4) : e->x;
}

static void squirt(DflFoe *e) {
    e->sub++;
    int c = e->sub % 56;
    if (c == 1) e->vx = e->dir * 3.0f;
    e->vx *= 0.94f;
    e->x += e->vx;
    e->y = e->ay + sinf(e->sub * 0.06f) * 5;
    if (c == 30) {
        /* three shots back and down, behind it */
        float base = atan2f(1.0f, -(float)e->dir);
        for (int k = -1; k <= 1; k++) {
            float a = base + k * 0.32f;
            dfl_add_eshot(ES_PELLET, e->x - e->dir * 6, e->y + 4, cosf(a) * 1.6f, sinf(a) * 1.6f);
        }
        dfl_sfx("dfl_efire", 4);
    }
    if (e->sub > 30 && offscreen(e, 24)) escape(e);
}

static void shark(DflFoe *e) {
    /* three leaps over the road, back and forth, spraying at the top */
    if (e->state % 2 == 0) {
        e->vx = e->dir * 2.0f;
        e->vy = -3.6f;
        e->state++;
        dfl_burst(e->x, 140, C_ICE, 10, 1.6f);
        dfl_sfx("dfl_splash", 4);
    }
    e->vy += 0.06f;
    e->x += e->vx;
    e->y += e->vy;
    if (e->vy > 0 && e->vy - 0.06f <= 0) {
        for (int k = -2; k <= 2; k++) dfl_add_eshot(ES_BUBBLE, e->x, e->y + 6, k * 0.5f, 1.5f);
        dfl_sfx("dfl_efire", 4);
    }
    if (e->vy > 0 && e->y >= 140) {
        dfl_burst(e->x, 140, C_ICE, 10, 1.6f);
        e->dir = -e->dir;
        e->state++;
        if (e->state >= 6) escape(e);
    }
}

static void planet(DflFoe *e) {
    e->vy += 0.1f;
    e->x += e->vx;
    e->y += e->vy;
    if (e->y + 6 >= DFL_ROAD_Y) { e->y = DFL_ROAD_Y - 6; e->vy = -4.4f; dfl_sfx("dfl_bounce", 6); }
    if (e->y < 18) { e->y = 18; e->vy = fabsf(e->vy); }
    if (e->x < 8) { e->x = 8; e->vx = fabsf(e->vx); }
    if (e->x > SCREEN_W - 8) { e->x = SCREEN_W - 8; e->vx = -fabsf(e->vx); }
}

void dfl_foes_update(void) {
    for (int i = 0; i < DFL_MAX_FOES; i++) {
        DflFoe *e = &dfg.foe[i];
        if (!e->alive) continue;
        if (e->t < 0) { e->t++; continue; }
        e->t++;
        if (e->flash > 0) e->flash--;
        if (e->still) continue;
        switch (e->kind) {
        case FK_KITE: kite(e); break;
        case FK_BUZZER: buzzer(e); break;
        case FK_HOG: case FK_WRECK: road_car(e); break;
        case FK_ROTOR: rotor(e); break;
        case FK_SHARD: shard(e); break;
        case FK_PRISM: prism(e); break;
        case FK_HOOP: hoop(e); break;
        case FK_TUMBLER: tumbler(e); break;
        case FK_SKULL: skull(e); break;
        case FK_SHEET: sheet(e); break;
        case FK_SNAPPER: snapper(e); break;
        case FK_SLAB: slab(e); break;
        case FK_SAUCER: saucer(e); break;
        case FK_JELLY: jelly(e); break;
        case FK_DARTFISH: dartfish(e); break;
        case FK_SQUIRT: squirt(e); break;
        case FK_SHARK: shark(e); break;
        case FK_PLANET: planet(e); break;
        default: break;
        }
    }
}

/* ---- stage 4's swells ------------------------------------------------------------------ */

void dfl_swells_update(void) {
    if (dfg.stage != 3) return;
    int lt = dfg.stage_t - dfg.swell_loop * DFL_SWELL_LOOP;
    while (dfg.swell_i < DFL_SWELL_COUNT && DFL_SWELLS[dfg.swell_i].t <= lt) {
        for (int k = 0; k < DFL_MAX_SWELLS; k++)
            if (!dfg.swell[k].alive) {
                dfg.swell[k] = (DflSwell){DFL_SWELLS[dfg.swell_i].x, 0, 1};
                sfx_play_name("dfl_breach");
                break;
            }
        dfg.swell_i++;
    }
    if (lt >= DFL_SWELL_LOOP) { dfg.swell_loop++; dfg.swell_i = 0; }
    for (int k = 0; k < DFL_MAX_SWELLS; k++) {
        DflSwell *w = &dfg.swell[k];
        if (!w->alive) continue;
        w->t++;
        if (w->t == DFL_SWELL_T) { sfx_play_name("dfl_wave"); dfl_burst((float)w->x, DFL_ROAD_Y - 6, C_ICE, 14, 2.0f); }
        if (dfl_swell_breaking(k))
            for (int p = 0; p < DFL_CARS; p++)
                if (dfl_car_hit_rect(p, (float)(w->x - DFL_SWELL_HW), DFL_ROAD_TOP, (float)(w->x + DFL_SWELL_HW), DFL_ROAD_BOT)) dfl_kill_car(p, CAUSE_SWELL);
        if (w->t >= DFL_SWELL_T + DFL_SWELL_HIT) w->alive = 0;
    }
}

bool dfl_swell_breaking(int i) {
    const DflSwell *w = &dfg.swell[i];
    return w->alive && w->t >= DFL_SWELL_T && w->t < DFL_SWELL_T + DFL_SWELL_HIT;
}

/* ---- the bosses ------------------------------------------------------------------------ */

void dfl_boss_start(int stage) {
    DflBoss *b = &dfg.boss;
    memset(b, 0, sizeof *b);
    b->on = true;
    b->kind = stage;
    b->x = 160;
    switch (stage) {
    case BOSS_ZEPHYR:
        b->y = -40;
        for (int k = 0; k < 3; k++) b->part_hp[k] = DFL_TURRET_HP;
        b->hp = b->maxhp = 3 * DFL_TURRET_HP;
        break;
    case BOSS_ORRERY:
        b->y = -30;
        b->hp = b->maxhp = DFL_ORRERY_HP;
        for (int k = 0; k < 4; k++) b->part_hp[k] = DFL_ORBITER_HP;
        break;
    case BOSS_MOON:
        b->x = 236;
        b->y = 30;
        b->hp = b->maxhp = DFL_MOON_HP;
        b->hx = SCREEN_W + 30;
        b->hy = 70;
        break;
    default:
        b->y = -30;
        b->hp = b->maxhp = DFL_CRAB_HP;
        for (int k = 0; k < 6; k++) b->part_t[k] = -1;
        break;
    }
    music_play(stage == 3 ? DFL_MUS_FINAL : DFL_MUS_BOSS);
}

static void boss_die(void) {
    DflBoss *b = &dfg.boss;
    b->dead = true;
    b->dead_t = 0;
    dfl_add_score((uint32_t)DFL_BOSS_POINTS[b->kind]);
    dfg.kills++;
    sfx_play_name("dfl_bigboom");
    /* whatever is left of the fight goes with it */
    for (int i = 0; i < DFL_MAX_FOES; i++)
        if (dfg.foe[i].alive) { dfl_burst(dfg.foe[i].x, dfg.foe[i].y, C_WHITE, 6, 1.5f); dfg.foe[i].alive = 0; }
    memset(dfg.es, 0, sizeof dfg.es);
    music_stop();
}

static float turret_x(int k) { return dfg.boss.x + (k - 1) * 48.0f; }
static float turret_y(int k) { return dfg.boss.y + (k == 1 ? 20.0f : 17.0f); }
static float orbiter_x(int k) { return dfg.boss.x + cosf(dfg.boss.orbit_a + k * PI_F / 2) * 30; }
static float orbiter_y(int k) { return dfg.boss.y + sinf(dfg.boss.orbit_a + k * PI_F / 2) * 30; }
static const float LEG_DX[6] = {-74, -52, -30, 30, 52, 74};
static const int LEG_ORDER[6] = {2, 5, 0, 3, 1, 4};
#define LEG_WARN DFL_LEG_WARN
#define LEG_DROP DFL_LEG_DROP
#define LEG_STAY DFL_LEG_STAY
#define LEG_LIFT DFL_LEG_LIFT
#define LEG_TOP_Y DFL_LEG_TOP_Y
float dfl_leg_x(int k) { return dfg.boss.x + LEG_DX[k]; }
/* how far a leg's foot reaches down (LEG_TOP_Y at rest, the road at a slam) */
float dfl_leg_foot(int k) {
    int t = dfg.boss.part_t[k];
    if (t < LEG_WARN) return LEG_TOP_Y;
    t -= LEG_WARN;
    float road = DFL_ROAD_Y;
    if (t < LEG_DROP) return LEG_TOP_Y + (road - LEG_TOP_Y) * t / LEG_DROP;
    t -= LEG_DROP;
    if (t < LEG_STAY) return road;
    t -= LEG_STAY;
    return road - (road - LEG_TOP_Y) * t / LEG_LIFT;
}

/* the moon's fist sweeping low */
#define FIST_HW DFL_FIST_HW
#define FIST_HH DFL_FIST_HH
float dfl_fist_y(float x) { return 120.0f + sinf((SCREEN_W - x) * 0.028f) * 26.0f; }
#define fist_y dfl_fist_y

static void zephyr(DflBoss *b) {
    if (b->y < 40) { b->y += 0.6f; return; }
    b->x = 160 + sinf(b->t * 0.01f) * 50;
    int dead = 0;
    for (int k = 0; k < 3; k++) dead += b->part_hp[k] <= 0;
    int period = 110 - 30 * dead; /* the fewer turrets left, the faster they fire */
    for (int k = 0; k < 3; k++) {
        if (b->part_hp[k] <= 0) continue;
        if ((b->t + k * 43) % period == 0) { dfl_aimed(turret_x(k), turret_y(k) + 4, 1.5f, 3, 22); dfl_sfx("dfl_efire", 4); }
    }
}

static void orrery(DflBoss *b) {
    if (b->y < 52 && b->t < 100) { b->y += 0.8f; b->orbit_a += 0.025f; return; }
    b->x = 160 + sinf(b->t * 0.012f) * 90;
    b->y = 52 + sinf(b->t * 0.024f) * 14;
    b->orbit_a += 0.025f;
    int third = b->hp * 3 / b->maxhp;
    int ring = third >= 1 ? 120 : 80;
    if (b->t % ring == 0) { dfl_ring(b->x, b->y, 1.4f, 10, b->t * 0.01f); dfl_sfx("dfl_efire", 4); }
    if (third < 2 && b->t % 70 == 35) dfl_aimed(b->x, b->y + 10, 1.9f, 3, 12);
}

static void moon(DflBoss *b) {
    if (!b->revealed) { if (b->t > 90) b->revealed = true; return; }
    int idle = b->hp * 2 < b->maxhp ? 40 : 60;
    b->hand_t++;
    switch (b->hand_state) {
    case 0: /* resting beside the face */
        b->hx = fapproach(b->hx, 200, 3);
        b->hy = fapproach(b->hy, 84, 2);
        if (b->hand_t > idle) { b->hand_state = 1 + b->attack % 3; b->hand_t = 0; b->attack++; }
        break;
    case 1: /* the five-way spread from the mouth, three times */
        if (b->hand_t == 20 || b->hand_t == 60 || b->hand_t == 100) { dfl_aimed(232, 64, 1.8f, 5, 15); dfl_sfx("dfl_efire", 4); }
        if (b->hand_t > 140) { b->hand_state = 0; b->hand_t = 0; }
        break;
    case 2: /* the fist sweeps low in a wave: drive under it */
        if (b->hand_t == 1) { b->hx = SCREEN_W + 20; dfl_sfx("dfl_whoosh", 10); }
        b->hx -= 1.6f;
        b->hy = fist_y(b->hx);
        if (b->hx < -30) { b->hand_state = 0; b->hand_t = 0; b->hx = SCREEN_W + 30; b->hy = 84; }
        for (int p = 0; p < DFL_CARS; p++)
            if (dfl_car_hit_rect(p, b->hx - FIST_HW, b->hy - FIST_HH, b->hx + FIST_HW, b->hy + FIST_HH)) dfl_kill_car(p, CAUSE_FIST);
        break;
    default: /* the hand flies over and drops four chasers */
        if (b->hand_t == 1) { b->hx = SCREEN_W + 20; b->hy = 60; }
        b->hx -= 1.5f;
        if (b->hand_t == 40 || b->hand_t == 80 || b->hand_t == 120 || b->hand_t == 160) {
            int i = dfl_spawn_foe(FK_SNAPPER, b->hx, b->hy + 8, 1, DFL_ADD);
            if (i >= 0) { dfg.foe[i].ay = b->hy + 24; dfg.foe[i].state = 1; }
            dfl_sfx("dfl_snap", 2);
        }
        if (b->hx < -30) { b->hand_state = 0; b->hand_t = 0; b->hx = SCREEN_W + 30; b->hy = 84; }
        break;
    }
}

static void crab(DflBoss *b) {
    if (b->y < 36) { b->y += 0.6f; return; }
    b->x = 160 + sinf(b->t * 0.008f) * 60;
    /* the legs: one after another (two at once once it is hurt), each
     * changing colour before it slams the road */
    int gap = b->hp * 2 < b->maxhp ? 50 : 70;
    if (b->t % gap == 0) {
        int k = LEG_ORDER[b->pt % 6];
        if (b->part_t[k] < 0) b->part_t[k] = 0;
        b->pt++;
    }
    for (int k = 0; k < 6; k++) {
        if (b->part_t[k] < 0) continue;
        b->part_t[k]++;
        int t = b->part_t[k] - LEG_WARN;
        if (t == LEG_DROP) { dfg.shake = 6; dfl_burst(dfl_leg_x(k), DFL_ROAD_Y, C_GREY, 8, 1.5f); dfl_sfx("dfl_slam", 4); }
        /* down, it hurts until it is all the way back up */
        if (t >= 0 && t < LEG_DROP + LEG_STAY + LEG_LIFT) {
            float foot = dfl_leg_foot(k);
            for (int p = 0; p < DFL_CARS; p++)
                if (dfl_car_hit_rect(p, dfl_leg_x(k) - 7, LEG_TOP_Y - 20, dfl_leg_x(k) + 7, foot)) dfl_kill_car(p, CAUSE_LEG);
        }
        if (t >= LEG_DROP + LEG_STAY + LEG_LIFT) b->part_t[k] = -1;
    }
    /* bubbles from the mouth: 3, 4 and 5 at a time */
    if (b->t % 90 == 45) {
        int n = 3 + (b->phase++ % 3);
        dfl_aimed(b->x, b->y + 14, 1.7f, n, 16);
        dfl_sfx("dfl_bubble", 4);
        for (int i = 0; i < DFL_MAX_ESHOTS; i++)
            if (dfg.es[i].alive && dfg.es[i].kind == ES_PELLET && dfg.es[i].t == 0 && fabsf(dfg.es[i].y - (b->y + 14)) < 1) {
                dfg.es[i].kind = ES_BUBBLE;
                dfg.es[i].r = 1.0f; /* the bubbles' hitbox is generous to you */
            }
    }
}

void dfl_boss_update(void) {
    DflBoss *b = &dfg.boss;
    if (b->dead) {
        b->dead_t++;
        if (b->dead_t % 8 == 0 && b->dead_t < 130) {
            float ex = b->x + rng_range(&g_rng, -50, 50), ey = b->y + rng_range(&g_rng, -16, 20);
            if (b->kind == BOSS_MOON) { ex = 236 + rng_range(&g_rng, -40, 40); ey = 40 + rng_range(&g_rng, -30, 30); }
            dfl_burst(ex, ey, b->dead_t % 16 ? C_ORANGE : C_YELLOW, 14, 2.4f);
            dfl_sfx("dfl_boom", 6);
        }
        return;
    }
    if (b->flash > 0) b->flash--;
    if (dfg.boss_still && b->y >= 36) return; /* tests: a boss that holds still */
    b->t++;
    switch (b->kind) {
    case BOSS_ZEPHYR: zephyr(b); break;
    case BOSS_ORRERY: orrery(b); break;
    case BOSS_MOON: moon(b); break;
    default: crab(b); break;
    }
}

/* the boss takes hits only where its weak spots are; the rest of it stops shots */
bool dfl_boss_shot(DflPShot *s) {
    DflBoss *b = &dfg.boss;
    if (!b->on || b->dead) return false;
    switch (b->kind) {
    case BOSS_ZEPHYR: {
        if (b->y < 36) return false;
        for (int k = 0; k < 3; k++) {
            if (b->part_hp[k] <= 0) continue;
            if (fabsf(s->x - turret_x(k)) < 8 && fabsf(s->y - turret_y(k)) < 7) {
                b->part_hp[k] -= s->dmg;
                b->hp = imax(0, b->part_hp[0]) + imax(0, b->part_hp[1]) + imax(0, b->part_hp[2]);
                b->flash = 3;
                dfl_sfx("dfl_hit", 3);
                if (b->part_hp[k] <= 0) {
                    dfl_burst(turret_x(k), turret_y(k), C_ORANGE, 16, 2.2f);
                    dfl_sfx("dfl_boom", 2);
                    if (b->hp <= 0) boss_die();
                }
                return true;
            }
        }
        float dx = (s->x - b->x) / 74.0f, dy = (s->y - b->y) / 15.0f;
        return dx * dx + dy * dy < 1.0f; /* the gasbag soaks it up */
    }
    case BOSS_ORRERY:
        for (int k = 0; k < 4; k++) {
            if (b->part_hp[k] <= 0) continue;
            float ox = orbiter_x(k), oy = orbiter_y(k);
            if (fabsf(s->x - ox) < 7 && fabsf(s->y - oy) < 7) {
                b->part_hp[k] -= s->dmg;
                dfl_sfx("dfl_clink", 3);
                if (b->part_hp[k] <= 0) {
                    /* knocked off: it goes dark and bounces round the road */
                    int i = dfl_spawn_foe(FK_PLANET, ox, oy, 1, 0);
                    if (i >= 0) { dfg.foe[i].vx = s->vx >= 0 ? 1.3f : -1.3f; dfg.foe[i].vy = -2.0f; }
                    dfl_sfx("dfl_crunch", 2);
                }
                return true;
            }
        }
        if ((s->x - b->x) * (s->x - b->x) + (s->y - b->y) * (s->y - b->y) < 15 * 15) {
            b->hp -= s->dmg;
            b->flash = 3;
            dfl_sfx("dfl_hit", 3);
            if (b->hp <= 0) boss_die();
            return true;
        }
        return false;
    case BOSS_MOON:
        if (!b->revealed) return false;
        if ((s->x - 236) * (s->x - 236) + (s->y - 30) * (s->y - 30) < 13 * 13) {
            b->hp -= s->dmg;
            b->flash = 3;
            dfl_sfx("dfl_hit", 3);
            if (b->hp <= 0) boss_die();
            return true;
        }
        return false; /* the rest of the face is the night sky's: shots go by it */
    default: {
        if (b->y < 36) return false;
        if (fabsf(s->x - b->x) < 8 && fabsf(s->y - (b->y - 6)) < 8) {
            b->hp -= s->dmg;
            b->flash = 3;
            dfl_sfx("dfl_hit", 3);
            if (b->hp <= 0) boss_die();
            return true;
        }
        /* the shell stops the rest, except straight up the middle at the mark */
        float dx = (s->x - b->x) / 54.0f, dy = (s->y - b->y) / 20.0f;
        if (fabsf(s->x - b->x) >= 8 && dx * dx + dy * dy < 1.0f) { dfl_sfx("dfl_clink", 6); return true; }
        return false;
    }
    }
}

int dfl_boss_hazards(float *r, int max) {
    DflBoss *b = &dfg.boss;
    int n = 0;
    if (!b->on || b->dead) return 0;
    if (b->kind == BOSS_MOON && b->hand_state == 2 && n < max) {
        /* the whole sweep ahead of the fist: where it will be low */
        for (float x = b->hx; x > b->hx - 120 && n < max; x -= 8) {
            float y = fist_y(x);
            r[n * 4] = x - FIST_HW;
            r[n * 4 + 1] = y - FIST_HH;
            r[n * 4 + 2] = x + FIST_HW;
            r[n * 4 + 3] = y + FIST_HH;
            n++;
        }
    }
    if (b->kind == BOSS_CRAB)
        for (int k = 0; k < 6 && n < max; k++) {
            if (b->part_t[k] < 0) continue;
            r[n * 4] = dfl_leg_x(k) - 9;
            r[n * 4 + 1] = LEG_TOP_Y;
            r[n * 4 + 2] = dfl_leg_x(k) + 9;
            r[n * 4 + 3] = b->part_t[k] < LEG_WARN - 30 ? LEG_TOP_Y : dfl_leg_foot(k) > LEG_TOP_Y ? dfl_leg_foot(k) : DFL_ROAD_Y;
            n++;
        }
    return n;
}

uint32_t dfl_stage_points(int s) {
    uint32_t pts = 0;
    const DflStage *st = &DFL_STAGE[s];
    for (int i = 0; i < st->count; i++) {
        int k = st->spawns[i].kind == FK_HOG ? FK_WRECK : st->spawns[i].kind;
        pts += (uint32_t)(DFL_FOE[k].points * st->spawns[i].n);
    }
    return pts;
}

int dfl_stage_foes(int s) {
    int n = 0;
    const DflStage *st = &DFL_STAGE[s];
    for (int i = 0; i < st->count; i++) n += st->spawns[i].n;
    return n;
}
