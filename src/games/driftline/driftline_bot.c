/* DRIFTLINE - the demo player. It only chooses buttons (the tests press
 * them for real). Every frame it tries a handful of ways to steer for the
 * next second or so, follows every shot, foe, beam, leg and wave that far
 * ahead, and takes the safest one that also lines the gun up on something,
 * or drifts left to charge the meter when there is room. In the bonus stage
 * it reads where the coin will come down and gets under it. */
#include "driftline.h"

#define H 34              /* frames looked ahead */
#define MAXT 260

typedef struct {
    float x0, y0, x1, y1;     /* the box at frame 0 */
    float vx, vy, g;          /* how it moves */
    int from, to;             /* the frames it is dangerous (inclusive) */
} Threat;

static Threat thr[MAXT];
static int nthr;
static float fpx[DFL_MAX_FOES], fpy[DFL_MAX_FOES];
static int fseen_t[DFL_MAX_FOES];
static int last_frame = -1;
int dfl_bot_dbg[4];   /* the last frame's goal, plan, plans that hit, and threats (tests) */
static float boss_px, boss_py;
static int boss_seen = -10;

static void add(float x0, float y0, float x1, float y1, float vx, float vy, float g, int from, int to) {
    if (nthr >= MAXT) return;
    thr[nthr++] = (Threat){x0, y0, x1, y1, vx, vy, g, from, to};
}

static void gather(int p) {
    nthr = 0;
    const DflCar *c = &dfg.car[p];
    for (int i = 0; i < DFL_MAX_ESHOTS; i++) {
        const DflEShot *s = &dfg.es[i];
        if (!s->alive) continue;
        if (fabsf(s->x - c->x) > 140) continue;
        float r = s->r + 1.5f;
        add(s->x - r, s->y - r, s->x + r, s->y + r, s->vx, s->vy, s->g, 0, H);
    }
    for (int i = 0; i < DFL_MAX_FOES; i++) {
        const DflFoe *e = &dfg.foe[i];
        if (!e->alive || e->t < 0) continue;
        const DflFoeDef *d = &DFL_FOE[e->kind];
        float vx = 0, vy = 0;
        if (fseen_t[i] == dfg.frame_t - 1) { vx = e->x - fpx[i]; vy = e->y - fpy[i]; }
        float m = 2.5f;
        if (e->kind == FK_SNAPPER) m = 6;   /* it lunges */
        if (e->kind == FK_SKULL || e->kind == FK_DARTFISH) m = 4;
        if (e->kind == FK_SHEET && e->state == 2) continue;
        if (e->kind == FK_PRISM && (e->state == 2 || e->state == 3)) {
            /* the beam: now, or in a moment */
            float dir = e->x < 160 ? 0.9f : -0.9f;
            int from = e->state == 2 ? 0 : imax(0, 40 - (e->sub % 220 - 60));
            add(e->x - 6, e->y + 10, e->x + 6, DFL_ROAD_Y, dir, 0, 0, from, H);
        }
        if (e->kind == FK_SLAB && e->state >= 1 && e->state <= 3) {
            /* the column under it */
            add(e->x - d->hw - 4, e->y, e->x + d->hw + 4, DFL_ROAD_Y, 0, 0, 0, e->state == 1 ? 20 : 0, H);
            continue;
        }
        float g = e->kind == FK_HOOP || e->kind == FK_PLANET ? (e->kind == FK_HOOP ? 0.09f : 0.1f) : e->kind == FK_TUMBLER && !e->ground ? 0.14f : 0;
        add(e->x - d->hw - m, e->y - d->hh - m, e->x + d->hw + m, e->y + d->hh + m, vx, vy, g, 0, H);
    }
    /* stage 4's waves */
    for (int k = 0; k < DFL_MAX_SWELLS; k++) {
        const DflSwell *w = &dfg.swell[k];
        if (!w->alive) continue;
        int from = DFL_SWELL_T - w->t - 3, to = DFL_SWELL_T + DFL_SWELL_HIT - w->t + 2;
        if (to < 0) continue;
        add(w->x - DFL_SWELL_HW - 3, DFL_ROAD_TOP, w->x + DFL_SWELL_HW + 3, DFL_ROAD_BOT, 0, 0, 0, imax(0, from), to);
    }
    /* the bosses' reaching parts */
    const DflBoss *b = &dfg.boss;
    if (b->on && !b->dead) {
        if (b->kind == BOSS_MOON && b->hand_state == 2) {
            for (int k = 0; k <= H; k += 2) {
                float hx = b->hx - 1.6f * k, hy = dfl_fist_y(hx);
                add(hx - DFL_FIST_HW - 4, hy - DFL_FIST_HH - 3, hx + DFL_FIST_HW + 4, hy + DFL_FIST_HH + 3, 0, 0, 0, k, imin(H, k + 2));
            }
        }
        if (b->kind == BOSS_CRAB)
            for (int k = 0; k < 6; k++) {
                int pt = b->part_t[k];
                if (pt < 0) continue;
                int from = DFL_LEG_WARN + 2 - pt, to = DFL_LEG_WARN + DFL_LEG_DROP + DFL_LEG_STAY + DFL_LEG_LIFT + 2 - pt;
                if (to < 0) continue;
                float lx = dfl_leg_x(k);
                add(lx - 11, DFL_LEG_TOP_Y, lx + 11, DFL_ROAD_Y, 0, 0, 0, imax(0, from), to);
            }
    }
}

static void remember_foes(void) {
    boss_px = dfg.boss.x;
    boss_py = dfg.boss.y;
    boss_seen = dfg.frame_t;
    for (int i = 0; i < DFL_MAX_FOES; i++) {
        fpx[i] = dfg.foe[i].x;
        fpy[i] = dfg.foe[i].y;
        fseen_t[i] = dfg.foe[i].alive ? dfg.frame_t : -10;
    }
}

/* steering plans: a direction for a while, then let go */
typedef struct { int dir, frames, then; } Plan;
static const Plan PLANS[] = {{0, 0, 0}, {-1, H, 0}, {1, H, 0}, {-1, 6, 0}, {1, 6, 0}, {-1, 14, 0}, {1, 14, 0},
                             {-1, 8, 1}, {1, 8, -1}, {-1, 16, 1}, {1, 16, -1}};
#define NPLANS ((int)(sizeof PLANS / sizeof PLANS[0]))

/* the first frame a plan runs into something (H + 1: never), and the car's
 * x after 12 frames */
static int simulate(int p, const Plan *pl, float *x12, float *xend, float *clear) {
    const DflCar *c = &dfg.car[p];
    float x = c->x, vx = c->vx;
    *clear = 99;
    for (int k = 1; k <= H; k++) {
        int dir = k <= pl->frames ? pl->dir : pl->then;
        if (dir) vx = fapproach(vx, dir * DFL_SPEED, DFL_ACCEL);
        else vx = fapproach(vx, 0, DFL_FRICTION);
        x += vx;
        if (x < DFL_MIN_X) { x = DFL_MIN_X; vx = 0; }
        if (x > DFL_MAX_X) { x = DFL_MAX_X; vx = 0; }
        if (k == 12) *x12 = x;
        *xend = x;
        if (k < c->inv - 2) continue;
        float cx0 = x - DFL_HW - 1, cx1 = x + DFL_HW + 1, cy0 = DFL_HIT_TOP - 1, cy1 = DFL_ROAD_Y;
        for (int i = 0; i < nthr; i++) {
            const Threat *t = &thr[i];
            if (k < t->from || k > t->to) continue;
            float dx = t->vx * k, dy = t->vy * k + 0.5f * t->g * k * k;
            float x0 = t->x0 + dx, x1 = t->x1 + dx, y0 = t->y0 + dy, y1 = t->y1 + dy;
            if (x1 > cx0 && x0 < cx1 && y1 > cy0 && y0 < cy1) return k;
            if (y1 > cy0 - 20 && k < 16) {
                float gap = x0 > cx1 ? x0 - cx1 : cx0 > x1 ? cx0 - x1 : 0;
                if (gap < *clear) *clear = gap;
            }
        }
    }
    return H + 1;
}

/* where the main gun wants the car, for a target at (tx, ty) moving (vx, vy) */
static float aim_spot(const DflCar *c, float tx, float ty, float vx, float vy) {
    float h = DFL_CAR_TOP - ty;
    if (h < 8) h = 8;
    float tt = h / DFL_SHOT_SPEED[dfl_tier(c->meter)]; /* about how long the shot takes */
    tx += vx * tt;
    /* where the gun points now (standing still, UP brings it round to
     * straight up, and this follows it) */
    float a = c->aim * 3.14159265f / 180.0f;
    return tx - h * tanf(a);
}


typedef struct { float x, y, vx, vy; bool ground; int prio; } Target;

/* this frame's pick, for the gunner in 2P */
static Target bot_tg;
static bool bot_has;

static bool pick_target(int p, Target *out) {
    const DflCar *c = &dfg.car[p];
    const DflBoss *b = &dfg.boss;
    float best = -1e9f;
    bool any = false;
    for (int i = 0; i < DFL_MAX_FOES; i++) {
        const DflFoe *e = &dfg.foe[i];
        if (!e->alive || e->t < 0 || e->kind == FK_PLANET) continue;
        if (e->x < 4 || e->x > SCREEN_W - 4 || e->y < DFL_HUD_H) continue;
        if (e->kind == FK_SHEET && e->state == 2) continue;
        bool ground = e->ground && e->y > DFL_ROAD_TOP - 4;
        if (e->kind == FK_TUMBLER && e->ground) ground = true;
        float vx = 0, vy = 0;
        if (fseen_t[i] == dfg.frame_t - 1) { vx = e->x - fpx[i]; vy = e->y - fpy[i]; }
        float d = fabsf(e->x - c->x);
        float s = -d * 0.5f + DFL_FOE[e->kind].points * 0.02f;
        if (ground) s += d < 120 ? 120 : 0;
        if (e->kind == FK_BUZZER || e->kind == FK_KITE) s += e->sub * 0.15f; /* before they turn nasty */
        if (e->kind == FK_SNAPPER || e->kind == FK_SKULL || e->kind == FK_SHARD) s += 60 - e->y * 0.1f + e->y * 0.6f;
        if (e->y > 120 && !ground) s += 40;
        if (s > best) {
            best = s;
            *out = (Target){e->x, e->y, vx, vy, ground, 0};
            any = true;
        }
    }
    if (b->on && !b->dead) {
        Target bt = {0, 0, 0, 0, false, 1};
        bool have = true;
        switch (b->kind) {
        case BOSS_ZEPHYR: {
            int k = -1;
            float bd = 1e9f;
            for (int j = 0; j < 3; j++) {
                if (b->part_hp[j] <= 0) continue;
                float tx = b->x + (j - 1) * 48.0f, dd = fabsf(tx - c->x);
                if (dd < bd) { bd = dd; k = j; }
            }
            if (k < 0 || b->y < 36) { have = false; break; }
            bt.x = b->x + (k - 1) * 48.0f;
            bt.y = b->y + (k == 1 ? 20.0f : 17.0f);
            break;
        }
        case BOSS_ORRERY: bt.x = b->x; bt.y = b->y; break;
        case BOSS_MOON: if (!b->revealed) have = false; bt.x = 236; bt.y = 30; break;
        default: if (b->y < 36) have = false; bt.x = b->x; bt.y = b->y - 6; break;
        }
        if (boss_seen == dfg.frame_t - 1) { bt.vx = b->x - boss_px; bt.vy = b->y - boss_py; }
        /* the boss comes first unless something is about to hurt */
        if (have && (best < 80 || !any)) { *out = bt; any = true; }
    }
    return any;
}

static int play_buttons(int p) {
    DflCar *c = &dfg.car[p];
    if (!c->on || !c->alive || c->enter > 0) return DFL_BTN_MAIN;
    gather(p);
    Target tg = {0, 0, 0, 0, false, 0};
    bool has = pick_target(p, &tg);
    bot_tg = tg;
    bot_has = has;
    /* charge by drifting left while there is road to drift on; at the left
     * end, go and shoot instead (the swing back right is the swerve) */
    bool charging = c->meter < DFL_TIER2 + 30 && c->x > 80 && !(has && tg.ground && fabsf(tg.x - c->x) < 80);
    float goal = c->x;
    if (has) {
        if (tg.ground) goal = tg.x + (tg.x > c->x ? -60.0f : 60.0f);
        else goal = aim_spot(c, tg.x, tg.y, tg.vx, tg.vy);
    }
    goal = fclamp(goal, 40, SCREEN_W - 40);
    float best = -1e9f;
    int pick = 0, hits = 0;
    for (int i = 0; i < NPLANS; i++) {
        float x12 = c->x, xend = c->x, clear;
        int hit = simulate(p, &PLANS[i], &x12, &xend, &clear);
        float s = 0;
        if (hit <= H) s -= 20000 - hit * 500;
        s += fminf(clear, 24) * 6;
        if (charging) {
            if (PLANS[i].dir < 0) s += 70 + (PLANS[i].frames >= 14 ? 20 : 0);
            s -= fabsf(x12 - goal) * 0.2f;
        } else {
            s -= fabsf(x12 - goal) * 2.0f;
            if (PLANS[i].dir == 0 && fabsf(c->x - goal) < 10) s += 40;
        }
        /* keep room to move: being pinned to the edge is how cars are lost */
        if (xend < 60) s -= (60 - xend) * 8;
        if (xend > SCREEN_W - 60) s -= (xend - (SCREEN_W - 60)) * 8;
        if (s > best) { best = s; pick = i; }
        if (hit <= H) hits |= 1 << i;
    }
    dfl_bot_dbg[0] = (int)goal;
    dfl_bot_dbg[1] = pick;
    dfl_bot_dbg[2] = hits;
    dfl_bot_dbg[3] = nthr;
    int m = DFL_BTN_MAIN;
    int dir = PLANS[pick].frames > 0 ? PLANS[pick].dir : 0;
    if (dir < 0) m |= BTN_LEFT;
    if (dir > 0) m |= BTN_RIGHT;
    /* standing still: bring the gun back toward straight up */
    if (!dir) m |= BTN_UP;
    /* a ground target: the side guns, with a tap its way to point them */
    if (has && tg.ground && fabsf(tg.x - c->x) < 170) {
        int want = tg.x < c->x ? -1 : 1;
        if (c->side_cd == 0) {
            m &= ~(BTN_LEFT | BTN_RIGHT | BTN_UP);
            m |= (want < 0 ? BTN_LEFT : BTN_RIGHT) | DFL_BTN_SIDE;
        } else if ((dir ? dir : c->face) == want) m |= DFL_BTN_SIDE;
    }
    /* any low flier close by: the side guns' upper arm reaches it */
    for (int i = 0; i < DFL_MAX_FOES; i++) {
        const DflFoe *e = &dfg.foe[i];
        if (!e->alive || e->t < 0 || e->y < 100) continue;
        int want = e->x < c->x ? -1 : 1;
        if (fabsf(e->x - c->x) < 90 && (dir ? dir : c->face) == want) m |= DFL_BTN_SIDE;
    }

    return m;
}

/* the bonus stage: under the coin, angling it toward the blocks */
static int bonus_buttons(int p) {
    DflCar *c = &dfg.car[p];
    if (!c->on || !c->alive) return 0;
    int m = DFL_BTN_MAIN;
    if (!dfg.ball_live) return m;
    /* where the coin comes down: its path played out, blocks and all */
    static uint8_t grid[DFL_BROWS][DFL_BCOLS];
    memcpy(grid, dfg.block, sizeof grid);
    float x = dfg.bx, y = dfg.by, vx = dfg.bvx, vy = dfg.bvy;
    for (int k = 0; k < 1200; k++) {
        if (vy > 0 && y + 6 >= DFL_CAR_TOP) break;
        x += vx * 0.5f;
        y += vy * 0.5f;
        if (x < 14) { x = 14; vx = fabsf(vx); }
        if (x > 306) { x = 306; vx = -fabsf(vx); }
        if (y < 18) { y = 18; vy = fabsf(vy); }
        for (int axis = 0; axis < 2; axis++) {
            float px = axis == 0 ? x + (vx > 0 ? 6 : -6) : x, py = axis == 0 ? y : y + (vy > 0 ? 6 : -6);
            int c = (int)floorf((px - DFL_BLOCK_X0) / DFL_BLOCK_W), r = (int)floorf((py - DFL_BLOCK_Y0) / DFL_BLOCK_H);
            if (c < 0 || c >= DFL_BCOLS || r < 0 || r >= DFL_BROWS || !grid[r][c]) continue;
            grid[r][c] = 0;
            if (axis == 0) vx = -vx;
            else vy = -vy;
        }
    }
    /* the column with the most blocks */
    int bestc = 0, bestn = -1;
    for (int cc = 0; cc < DFL_BCOLS; cc++) {
        int n = 0;
        for (int r = 0; r < DFL_BROWS; r++) n += dfg.block[r][cc] > 0;
        if (n > bestn) { bestn = n; bestc = cc; }
    }
    float tx = DFL_BLOCK_X0 + bestc * DFL_BLOCK_W + DFL_BLOCK_W / 2.0f;
    float off = fclamp((tx - x) / 200.0f, -0.6f, 0.6f);
    float want = x - off * 15;
    float d = want - c->x;
    /* stopping distance */
    float stop = c->vx * c->vx / (2 * DFL_FRICTION) * (c->vx > 0 ? 1 : -1);
    if (d - stop > 2) m |= BTN_RIGHT;
    else if (d - stop < -2) m |= BTN_LEFT;
    else m |= BTN_UP;
    return m;
}

/* 2P: the gunner swings the gun onto the driver's target with its own pad,
 * keeps the main gun going, and turns the side guns on road targets */
static int gunner_buttons(void) {
    const DflCar *c = &dfg.car[0];
    int m = DFL_BTN_MAIN;
    if (dfg.state == DS_BONUS) return m | BTN_UP;
    if (!c->alive) return m | BTN_UP;
    /* anything low and close: the side guns' V reaches it */
    int low = 0;
    float lowd = 91;
    for (int i = 0; i < DFL_MAX_FOES; i++) {
        const DflFoe *e = &dfg.foe[i];
        if (!e->alive || e->t < 0 || e->y < 100 || e->kind == FK_PLANET) continue;
        float d = fabsf(e->x - c->x);
        if (d < lowd) { lowd = d; low = e->x < c->x ? -1 : 1; }
    }
    if (low) {
        if (c->side_cd == 0) m |= (low < 0 ? BTN_LEFT : BTN_RIGHT) | DFL_BTN_SIDE;
        else if (c->face == low) m |= DFL_BTN_SIDE;
        return m;
    }
    if (!bot_has) return m | BTN_UP;
    if (bot_tg.ground && fabsf(bot_tg.x - c->x) < 170) {
        int want = bot_tg.x < c->x ? -1 : 1;
        if (c->side_cd == 0) m |= (want < 0 ? BTN_LEFT : BTN_RIGHT) | DFL_BTN_SIDE;
        return m;
    }
    float h = fmaxf(8.0f, DFL_CAR_TOP - bot_tg.y);
    float tx = bot_tg.x + bot_tg.vx * h / DFL_SHOT_SPEED[dfl_tier(c->meter)];
    float want = fclamp(atan2f(tx - c->x, h) * 180.0f / 3.14159265f, -DFL_AIM_MAX, DFL_AIM_MAX);
    if (c->aim < want - 1.5f) m |= BTN_RIGHT;
    else if (c->aim > want + 1.5f) m |= BTN_LEFT;
    return m;
}

int dfl_bot_buttons(int p) {
    static int tick, gun;
    tick++;
    /* 2P: the first call plays the driver and works out the gunner's buttons
     * for the second */
    if (dfg.players == 2 && p == 1) return gun;
    int m = 0;
    switch (dfg.state) {
    case DS_TITLE:
        m = (tick / 4) % 2 ? BTN_A : 0;
        break;
    case DS_PLAY:
        m = play_buttons(p);
        break;
    case DS_BONUS:
        m = bonus_buttons(p);
        break;
    case DS_CREDITS:
        m = BTN_A;
        break;
    default:
        m = (tick / 4) % 2 ? BTN_A : 0;
        break;
    }
    if (dfg.players == 2) {
        bool driving = dfg.state == DS_PLAY || dfg.state == DS_BONUS;
        gun = driving ? gunner_buttons() : 0;
        if (driving) m &= BTN_LEFT | BTN_RIGHT; /* the driver only steers */
    }
    /* A also starts the stage, and a consumed press stays blocked until it is let
     * go: like a player, let the trigger up for a frame now and then */
    if ((dfg.state == DS_PLAY || dfg.state == DS_BONUS) && tick % 24 == 0) {
        m &= ~(DFL_BTN_MAIN | DFL_BTN_SIDE);
        gun &= ~(DFL_BTN_MAIN | DFL_BTN_SIDE);
    }
    /* the foes' places this frame */
    if (last_frame != dfg.frame_t) { remember_foes(); last_frame = dfg.frame_t; }
    return m;
}
