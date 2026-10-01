/* BUZZBOLT - the demo player. It only chooses buttons (the tests press
 * them for real): every frame it tries each way the pad can point, at full
 * speed (tapping fire) and slowed (holding fire), looks a few dozen frames
 * ahead at every shot and foe near it, and takes the safest move that also
 * brings it under a target or onto a letter that keeps its word good. */
#include "buzzbolt.h"

int bzz_bot_ship = BZ_SHIELDBUG;

#define H 22          /* frames looked ahead */
#define NEAR 96.0f    /* shots further off than this are ignored */
#define MAXT 220

static uint32_t tick;
static float fpx[BZZ_MAX_FOES], fpy[BZZ_MAX_FOES];
static uint8_t fseen[BZZ_MAX_FOES];

/* a threat, with where it will be over the next H frames */
typedef struct { float x[H + 1], y[H + 1], r; bool homing; int idx; } Threat;
static Threat thr[MAXT];
static int nthr;

/* the letter that keeps the word on course for BZZ (0 = any good one) */
static int good_letter(int letter) {
    uint8_t w[3] = {bz.word[0], bz.word[1], bz.word[2]};
    int n = bz.nword;
    if (n == 0) return letter == LT_B ? 2 : 1;          /* B starts BZZ; Z is fine */
    w[n] = (uint8_t)letter;
    if (n == 1) {
        if (w[0] == LT_B) return letter == LT_Z ? 2 : 1; /* BZ.. is best, BB.. is fine */
        return 1;                                         /* ZZ.. and ZB.. both come good */
    }
    return bzz_word_of(w) == W_WRONG ? 0 : 2;
}

static void add_threat_foe(int i) {
    Foe *e = &bz.foe[i];
    if (nthr >= MAXT) return;
    float vx = fseen[i] ? e->x - fpx[i] : e->vx, vy = fseen[i] ? e->y - fpy[i] : e->vy;
    Threat *t = &thr[nthr++];
    t->homing = false;
    t->idx = -1;
    t->r = (float)(e->hw > e->hh ? e->hw : e->hh);
    for (int k = 0; k <= H; k++) { t->x[k] = e->x + vx * k; t->y[k] = e->y + vy * k; }
    /* boxes: stretch the circle over the long side */
    t->r = sqrtf((float)(e->hw * e->hw + e->hh * e->hh)) * 0.85f;
}

static void gather(void) {
    nthr = 0;
    for (int i = 0; i < BZZ_MAX_ESHOTS && nthr < MAXT; i++) {
        EShot *s = &bz.es[i];
        if (!s->alive) continue;
        if (fabsf(s->x - bz.px) > NEAR || fabsf(s->y - bz.py) > NEAR) continue;
        Threat *t = &thr[nthr++];
        t->r = s->r;
        t->homing = s->kind == ES_HOMING || s->kind == ES_BIG;
        t->idx = i;
        EShot c = *s;
        for (int k = 0; k <= H; k++) {
            t->x[k] = c.x;
            t->y[k] = c.y;
            bzz_eshot_step(&c, bz.px, bz.py);
        }
    }
    for (int i = 0; i < BZZ_MAX_FOES; i++) {
        Foe *e = &bz.foe[i];
        if (!e->alive || e->t < 0 || (e->kind == EK_ROTWALL && !e->state && e->sub == 0)) continue;
        if (fabsf(e->x - bz.px) > NEAR + e->hw || fabsf(e->y - bz.py) > NEAR + e->hh) continue;
        add_threat_foe(i);
    }
}

typedef struct { float score; int hit; } Eval;

static Eval evaluate(int dx, int dy, bool slow, float gx, float gy) {
    float sp = slow ? BZZ_SLOW : BZZ_FAST;
    if (dx && dy) sp *= 0.7071f;
    float px = bz.px, py = bz.py;
    int hit = H + 1;
    float clear = 30;
    /* homing shots turn toward wherever the ship will be, so they are
     * followed again for each move tried */
    static EShot hc[MAXT];
    for (int i = 0; i < nthr; i++)
        if (thr[i].homing) hc[i] = bz.es[thr[i].idx];
    for (int k = 1; k <= H && hit > H; k++) {
        px = fclamp(px + dx * sp, BZZ_MIN_X, BZZ_MAX_X);
        py = fclamp(py + dy * sp, BZZ_MIN_Y, BZZ_MAX_Y);
        float margin = 1.8f + k * 0.06f;
        for (int i = 0; i < nthr; i++) {
            Threat *t = &thr[i];
            float tx = t->x[k], ty = t->y[k];
            if (t->homing) {
                bzz_eshot_step(&hc[i], px, py);
                tx = hc[i].x;
                ty = hc[i].y;
            }
            float ddx = tx - px, ddy = ty - py, d = sqrtf(ddx * ddx + ddy * ddy) - t->r - BZZ_HIT_R;
            if (d < margin) { hit = k; break; }
            if (d < clear) clear = d;
        }
    }
    Eval ev;
    ev.hit = hit;
    float s = 0;
    if (hit <= H) s -= 10000 - hit * 400;
    s += clear * 3.0f;
    float gdx = gx - px, gdy = gy - py;
    s -= sqrtf(gdx * gdx * 1.0f + gdy * gdy * 0.6f) * 2.0f;
    /* stay off the edges */
    if (px < 24 || px > BZZ_W - 24) s -= 30;
    if (py < 60) s -= (60 - py) * 2;
    /* letters that would spoil the word are avoided */
    for (int i = 0; i < BZZ_MAX_LETTERS; i++) {
        Letter *l = &bz.lt[i];
        if (!l->alive || good_letter(l->letter)) continue;
        float ly = l->y + 0.8f * H * 0.5f;
        if (fabsf(l->x - px) < BZZ_PICK_R + 4 && fabsf(ly - py) < BZZ_PICK_R + 6) s -= 400;
        if (fabsf(l->x - bz.px) < BZZ_PICK_R + 2 && fabsf(l->y - bz.py) < BZZ_PICK_R + 2 && dx == 0 && dy == 0) s -= 400;
    }
    ev.score = s;
    return ev;
}

static int pick_target(void) {
    int best = -1;
    float bs = -1e9f;
    for (int i = 0; i < BZZ_MAX_FOES; i++) {
        Foe *e = &bz.foe[i];
        if (!e->alive || e->t < 0 || e->y < -4 || e->y > bz.py - 6 || e->x < 4 || e->x > BZZ_W - 4) continue;
        if (e->kind == EK_ROTWALL && !e->state) continue;
        float s = -fabsf(e->x - bz.px) - (bz.py - e->y) * 0.25f;
        if (e->role == ROLE_BOSS) s += 60 - e->hp * 0.02f;
        if (e->kind == EK_GOLDBUG) s += 30;
        if (e->leaving) s -= 40;
        if (s > bs) { bs = s; best = i; }
    }
    return best;
}

static int pick_letter(void) {
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < BZZ_MAX_LETTERS; i++) {
        Letter *l = &bz.lt[i];
        if (!l->alive || good_letter(l->letter) < 2 || l->y < 50) continue;
        float d = fabsf(l->x - bz.px) + fabsf(l->y - bz.py) * 0.5f;
        if (d < bd) { bd = d; best = i; }
    }
    return bd < 150 ? best : -1;
}

static int play_buttons(void) {
    if (!bz.alive) return 0;
    gather();
    int tgt = pick_target(), let = pick_letter();
    float gx = BZZ_W / 2, gy = 150;
    bool aligned = false;
    bool big = false;
    if (tgt >= 0) {
        Foe *e = &bz.foe[tgt];
        /* lead a moving target by the time the shots take to get there */
        float vx = fseen[tgt] ? e->x - fpx[tgt] : e->vx;
        gx = fclamp(e->x + vx * (bz.py - e->y) / 7.0f, BZZ_MIN_X, BZZ_MAX_X);
        aligned = fabsf(gx - bz.px) < (float)e->hw * 0.7f + 3;
        big = e->role != ROLE_FOE || e->maxhp >= 30;
    }
    if (let >= 0) { gx = bz.lt[let].x; gy = fmaxf(bz.lt[let].y + 10, 70); }
    else if (tgt >= 0 && bz.foe[tgt].role == ROLE_BOSS) gy = 140;
    int best_dx = 0, best_dy = 0;
    bool best_slow = false;
    Eval best = {-1e9f, 0};
    for (int s = 0; s < 2; s++)
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                Eval ev = evaluate(dx, dy, s == 1, gx, gy);
                if (s == 1 && aligned && let < 0) ev.score += big ? 60 : 25; /* focused fire does the damage */
                if (ev.score > best.score) { best = ev; best_dx = dx; best_dy = dy; best_slow = s == 1; }
            }
    int m = 0;
    if (best_dx < 0) m |= BTN_LEFT;
    if (best_dx > 0) m |= BTN_RIGHT;
    if (best_dy < 0) m |= BTN_UP;
    if (best_dy > 0) m |= BTN_DOWN;
    if (best_slow) {
        /* already holding? keep holding; otherwise start */
        m |= BTN_A;
    } else if (bz.focused) {
        /* let go (one frame) to get back to full speed */
    } else if (tick % 6 < 2) {
        m |= BTN_A;
    }
    /* the firefly bombs its way out of trouble, or at a boss */
    if (bz.ship == BZ_FIREFLY && bz.bombs > 0 && tick % 2 == 0) {
        bool boss_above = tgt >= 0 && bz.foe[tgt].role == ROLE_BOSS && aligned;
        if (best.hit <= 8 || (boss_above && bz.bombs >= 2)) m |= BTN_B;
    }
    return m;
}

int bzz_bot_buttons(void) {
    tick++;
    int m = 0;
    bool edge = tick % 2 == 0;
    switch (bz.state) {
    case BS_INTRO: case BS_TITLE: if (edge && bz.state_t > 10) m = BTN_A; break;
    case BS_SELECT:
        if (!edge || bz.state_t < 10) break;
        if (bz.sel < bzz_bot_ship) m = BTN_RIGHT;
        else if (bz.sel > bzz_bot_ship) m = BTN_LEFT;
        else m = BTN_A;
        break;
    case BS_PLAY: m = play_buttons(); break;
    case BS_CLEAR: case BS_OVER: case BS_SCORES: case BS_ENDING: case BS_CREDITS:
        if (edge && bz.state_t > 30) m = BTN_A;
        break;
    case BS_NAME: if (edge && bz.state_t > 20) m = BTN_START; break;
    default: break;
    }
    for (int i = 0; i < BZZ_MAX_FOES; i++) { fpx[i] = bz.foe[i].x; fpy[i] = bz.foe[i].y; fseen[i] = bz.foe[i].alive; }
    return m;
}
