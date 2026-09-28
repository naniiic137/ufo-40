/* LOST LINKS - how the ball rolls. The one set of rules used by the game,
 * the aiming line and the planning probes, so a plan is exactly what the
 * ball will do (the moving creatures aside).
 *
 * Numbers no source gives (see "Readings" in the design document): a full
 * roll on the fairway runs 18 tiles and the lightest 1 tile; the ground
 * slows a ball at a steady rate (rough twice the fairway, the green a little
 * over half); a slope pulls harder than the fairway grips, so a ball never
 * rests on a fairway or green slope but can on rough. */
#include "lostlinks.h"

#define FR_FAIRWAY 0.050f
#define FR_ROUGH 0.110f
#define FR_GREEN 0.028f
#define FR_SAND 0.60f
#define FR_TREAD 0.060f
#define FR_SKIP 0.018f
#define SLOPE 0.075f
#define GRAV 0.20f
#define CHIP_VZ 2.4f         /* a chip is in the air 24 frames */
#define HOP_VZ 2.6f          /* a hop 26 frames */
#define REST 0.55f           /* how much of a bounce off a wall is kept */
#define CUP_SPEED 3.2f       /* faster than this a ball lips out of a hole */
#define DIVOT_SPEED 2.0f
#define BRAKE_DECEL 0.25f    /* Backspin, held */
#define BRAKE_BUDGET 50      /* frames of it a stroke (a third of that on slopes) */
#define FLOWER_VZ 2.4f       /* a jumping flower throws the ball up */
#define REST_SPEED 0.05f
#define SETTLE_SPEED 0.30f
#define SETTLE_FRAMES 50

float lnk_dir_x(int dir) { return cosf((float)dir * 6.2831853f / LNK_DIRS); }
float lnk_dir_y(int dir) { return sinf((float)dir * 6.2831853f / LNK_DIRS); }
float lnk_roll_dist(int level) { return 16.0f + (float)(iclamp(level, 1, LNK_LEVELS) - 1) * (272.0f / 11.0f); }
float lnk_chip_dist(int level) { return 10.0f + 7.0f * (float)iclamp(level, 1, LNK_LEVELS); }

char lnk_tile(const LnkCtx *c, int layer, int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= LNK_MW || ty >= LNK_MH) return '#';
    return (char)c->tiles[layer][ty][tx];
}

bool lnk_solid_char(char ch) { return ch == '#' || ch == 'T' || ch == 'H' || ch == 'X'; }

void lnk_slope(char ch, float *ax, float *ay) {
    static const signed char DX[10] = {0, -1, 0, 1, -1, 0, 1, -1, 0, 1};
    static const signed char DY[10] = {0, 1, 1, 1, 0, 0, 0, -1, -1, -1};
    *ax = *ay = 0;
    if (ch < '1' || ch > '9' || ch == '5') return;
    int k = ch - '0';
    float s = (DX[k] && DY[k]) ? SLOPE * 0.7071f : SLOPE;
    *ax = DX[k] * s;
    *ay = DY[k] * s;
}

static bool has(const LnkCtx *c, int ab) { return (c->abilities >> ab) & 1; }

static float friction(const LnkCtx *c, char ch) {
    switch (ch) {
    case ',': return FR_ROUGH;
    case ':': return FR_GREEN;
    case 's': return has(c, AB_TREAD) ? FR_TREAD : FR_SAND;
    case '~': return FR_SKIP;
    default: return FR_FAIRWAY;
    }
}

static char here(const LnkBall *b, const LnkCtx *c) {
    return lnk_tile(c, b->layer, (int)floorf(b->x / LNK_T), (int)floorf(b->y / LNK_T));
}

bool lnk_on_sand(const LnkBall *b, const LnkCtx *c) { return here(b, c) == 's'; }

bool lnk_is_chip(const LnkBall *b, const LnkCtx *c) {
    if (b->lie == LIE_CUP || b->lie == LIE_DIVOT) return true;
    return b->lie == LIE_SAND && !has(c, AB_TREAD);
}

void lnk_hit(LnkBall *b, const LnkCtx *c, int dir, int level) {
    level = iclamp(level, 1, LNK_LEVELS);
    b->sx = b->x;
    b->sy = b->y;
    b->slayer = b->layer;
    b->slie = b->lie;
    float dx = lnk_dir_x(dir), dy = lnk_dir_y(dir), sp;
    if (lnk_is_chip(b, c)) {
        /* out of sand, a divot or the cup the ball leaves the ground */
        float air = 2.0f * CHIP_VZ / GRAV;
        sp = lnk_chip_dist(level) / air;
        b->vz = CHIP_VZ;
        b->z = 0.01f;
    } else {
        sp = sqrtf(2.0f * FR_FAIRWAY * lnk_roll_dist(level));
        b->vz = b->z = 0;
    }
    b->vx = dx * sp;
    b->vy = dy * sp;
    b->lie = LIE_GROUND;
    b->moving = true;
    b->hopped = b->lipped = b->braked = false;
    b->slow_t = b->roll_t = 0;
    b->brake_left = BRAKE_BUDGET;
}

/* the Dune Tread: "tap secondary button to jump while rolling over sand" */
bool lnk_hop(LnkBall *b, const LnkCtx *c) {
    if (!b->moving || b->z > 0 || b->hopped || !has(c, AB_TREAD) || here(b, c) != 's') return false;
    b->vz = HOP_VZ;
    b->z = 0.01f;
    b->hopped = true;
    return true;
}

/* Backspin: "hold primary button to slow your roll for a little while".
 * One frame of braking; the budget is spent three times as fast on a slope
 * (it "wears out on sloped surfaces"), and there is none on the water. */
bool lnk_brake(LnkBall *b, const LnkCtx *c) {
    if (!b->moving || b->z > 0 || !has(c, AB_BACKSPIN) || b->brake_left <= 0) return false;
    char ch = here(b, c);
    if (ch == '~') return false;
    float ax, ay;
    lnk_slope(ch, &ax, &ay);
    b->brake_left -= (ax != 0 || ay != 0) ? 3 : 1;
    b->braked = true;
    float sp = sqrtf(b->vx * b->vx + b->vy * b->vy);
    if (sp > 0) {
        float ns = fmaxf(0, sp - BRAKE_DECEL);
        b->vx *= ns / sp;
        b->vy *= ns / sp;
    }
    return true;
}

/* 0 = open, 1 = solid, 2 = a block the Hammerhead breaks */
static int blocked(LnkBall *b, const LnkCtx *c, int tx, int ty, int axis, float v) {
    char ch = lnk_tile(c, b->layer, tx, ty);
    if (ch == '#' || ch == 'T' || ch == 'H') return 1;
    if (ch == 'g') return !(c->opened & 1);
    if (ch == 'k') return !(c->opened & 2);
    if ((ch == 'X' || ch == 'b') && c->probe)
        for (int i = 0; i < b->nbroken; i++)
            if (b->broken[i][0] == b->layer && b->broken[i][1] == tx && b->broken[i][2] == ty) return 0;
    /* cracked blocks: with the Hammerhead the ball crashes through them "like
     * bushes"; a bush it crashes through any time, and flies over */
    if (ch == 'X') return has(c, AB_HAMMER) ? 2 : 1;
    if (ch == 'b') return b->z >= 3.0f ? 0 : 2;
    if (ch == 'v' && axis == 1 && v < 0) {
        /* a ledge can't be climbed: entering one from the south is a wall */
        int cy = (int)floorf(b->y / LNK_T);
        return cy != ty;
    }
    return 0;
}

static int smash(LnkBall *b, const LnkCtx *c, int tx, int ty) {
    if (!c->probe) c->tiles[b->layer][ty][tx] = '.';
    else if (b->nbroken < 6) {
        b->broken[b->nbroken][0] = b->layer;
        b->broken[b->nbroken][1] = (uint8_t)tx;
        b->broken[b->nbroken][2] = (uint8_t)ty;
        b->nbroken++;
    }
    b->bx = (int16_t)tx;
    b->by = (int16_t)ty;
    b->vx *= 0.6f;
    b->vy *= 0.6f;
    return EV_BLOCK;
}

static int move_axis(LnkBall *b, const LnkCtx *c, int axis, float d) {
    if (d == 0) return EV_NONE;
    float nx = b->x + (axis == 0 ? d : 0), ny = b->y + (axis == 1 ? d : 0);
    int ev = EV_NONE;
    for (int k = -1; k <= 1; k++) {
        float px, py;
        if (axis == 0) { px = nx + (d > 0 ? LNK_R : -LNK_R); py = b->y + k * (LNK_R - 1); }
        else { px = b->x + k * (LNK_R - 1); py = ny + (d > 0 ? LNK_R : -LNK_R); }
        int tx = (int)floorf(px / LNK_T), ty = (int)floorf(py / LNK_T);
        int r = blocked(b, c, tx, ty, axis, d);
        if (r == 2) {
            ev = smash(b, c, tx, ty);
        } else if (r == 1) {
            if (axis == 0) { b->vx = -b->vx * REST; b->vy *= 0.92f; }
            else { b->vy = -b->vy * REST; b->vx *= 0.92f; }
            return EV_BOUNCE;
        }
    }
    b->x = nx;
    b->y = ny;
    return ev;
}

static void settle(LnkBall *b, const LnkCtx *c) {
    b->vx = b->vy = 0;
    b->moving = false;
    char ch = here(b, c);
    if (ch == 's' && !has(c, AB_TREAD)) b->lie = LIE_SAND;
    else if (ch == 'u') b->lie = LIE_DIVOT;
    else b->lie = LIE_GROUND;
}

static int sink(LnkBall *b) {
    b->x = b->sx;
    b->y = b->sy;
    b->layer = b->slayer;
    b->lie = b->slie;
    b->vx = b->vy = b->vz = b->z = 0;
    b->moving = false;
    return EV_SINK;
}

/* the ground under a rolling (or just landed) ball */
static int ground(LnkBall *b, const LnkCtx *c, bool landed) {
    int tx = (int)floorf(b->x / LNK_T), ty = (int)floorf(b->y / LNK_T);
    char ch = lnk_tile(c, b->layer, tx, ty);
    float sp = sqrtf(b->vx * b->vx + b->vy * b->vy);
    float cx = tx * LNK_T + LNK_T / 2.0f, cy = ty * LNK_T + LNK_T / 2.0f;
    float ddx = b->x - cx, ddy = b->y - cy, dist = sqrtf(ddx * ddx + ddy * ddy);
    if (ch != 'o' && ch != 'O') b->lipped = false;
    /* a pit: the ball falls, and is back where the stroke was hit from */
    if (ch == 'p' || ch == 'y') return sink(b);
    /* a jumping flower throws a rolling ball up */
    if (ch == 'J' && sp > 0.5f && (!landed || sp > 1.0f)) {
        b->vz = FLOWER_VZ;
        b->z = 0.01f;
        return EV_FLOWER;
    }
    /* the Skipper only works rolling onto the water, never landing in it */
    if (ch == '~' && (!has(c, AB_SKIPPER) || landed)) return sink(b);
    if ((ch == 'o' || ch == 'O') && dist < 5.0f) {
        if (sp < CUP_SPEED) {
            b->hx = (int16_t)tx;
            b->hy = (int16_t)ty;
            b->x = cx;
            b->y = cy;
            b->vx = b->vy = 0;
            b->moving = false;
            b->lie = LIE_CUP;
            return EV_HOLE;
        }
        if (!b->lipped) {
            /* too fast: it rattles round the rim and flies on, turned */
            float s = (ddx * b->vy - ddy * b->vx) >= 0 ? 0.45f : -0.45f;
            float co = cosf(s), si = sinf(s);
            float vx = (b->vx * co - b->vy * si) * 0.75f, vy = (b->vx * si + b->vy * co) * 0.75f;
            b->vx = vx;
            b->vy = vy;
            b->lipped = true;
            return EV_BOUNCE;
        }
    }
    if (ch == 'u' && dist < 6.0f && sp < DIVOT_SPEED) {
        b->x = cx;
        b->y = cy;
        settle(b, c);
        return EV_REST;
    }
    if (ch == 's' && !has(c, AB_TREAD) && landed) {
        settle(b, c);
        return EV_REST;
    }
    if (landed) {
        b->vx *= 0.6f;
        b->vy *= 0.6f;
        sp *= 0.6f;
    }
    float ax, ay;
    lnk_slope(ch, &ax, &ay);
    float fr = friction(c, ch);
    b->vx += ax;
    b->vy += ay;
    sp = sqrtf(b->vx * b->vx + b->vy * b->vy);
    if (sp > 0) {
        float ns = sp - fr;
        if (ns < 0) ns = 0;
        b->vx *= ns / sp;
        b->vy *= ns / sp;
        sp = ns;
    }
    float pull = sqrtf(ax * ax + ay * ay);
    if (sp < REST_SPEED && pull <= fr) {
        if (ch == '~') return sink(b); /* stopped on the water: under it goes */
        settle(b, c);
        return EV_REST;
    }
    if (sp < SETTLE_SPEED) b->slow_t++;
    else b->slow_t = 0;
    if (b->slow_t > SETTLE_FRAMES || b->roll_t > 1800) {
        if (ch == '~') return sink(b);
        settle(b, c);
        return EV_REST;
    }
    return landed ? EV_LAND : EV_NONE;
}

int lnk_ball_step(LnkBall *b, const LnkCtx *c) {
    if (!b->moving) return EV_NONE;
    b->roll_t++;
    int ev = EV_NONE;
    float m = fmaxf(fabsf(b->vx), fabsf(b->vy));
    int n = (int)ceilf(m / 3.0f);
    if (n < 1) n = 1;
    for (int i = 0; i < n; i++) {
        int e = move_axis(b, c, 0, b->vx / n);
        if (e != EV_NONE) ev = e;
        e = move_axis(b, c, 1, b->vy / n);
        if (e != EV_NONE) ev = e;
    }
    if (b->z > 0) {
        b->z += b->vz;
        b->vz -= GRAV;
        if (b->z > 0) return ev;
        b->z = b->vz = 0;
        int g = ground(b, c, true);
        return g != EV_NONE ? g : ev;
    }
    int g = ground(b, c, false);
    return g != EV_NONE ? g : ev;
}
