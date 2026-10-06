/* BRAVADO - the demo player. It reads the game the way a player reads the
 * screen and answers with buttons only (the "bot" query): the tests press
 * exactly what it returns. In a fight it weighs nine moves (eight ways or
 * standing still) against where every monster, shot, bomb and pool will be
 * a moment later, keeps fire held while its target sits in the line of
 * fire, lets go and turns when it doesn't, and drops bombs on crowds and
 * on the peepers' tone. In the shop it follows a buying list and raises
 * the prize to a target for each fight. */
#include "bravado.h"

int brv_bot_plan; /* 0: play to win; 1: play for the cherry (bank 1,300+) */

static int prev_mask;     /* what it held last frame */
static int turn_t;        /* frames the target has sat off the facing */
static int last_move = -1;
static bool just_turned;   /* fire comes back on without moving, so the turn sticks */

/* what it buys, in order (item, tier it brings the item up to) */
static const int8_t BUY[][2] = {
    {GR_BOMBBAG, 1}, {GR_SHOTGD, 1}, {GR_TRIGGER, 1}, {GR_BLASTGD, 1}, {GR_HEART, 1}, {GR_FAN, 1},
    {GR_TRIGGER, 2}, {GR_SHOTGD, 2}, {GR_HEAVY, 1}, {GR_MEDKIT, 1}, {GR_HEART, 2}, {GR_FAN, 2},
    {GR_BLASTGD, 2}, {GR_KICK, 1}, {GR_BOMBBAG, 2}, {GR_HEAVY, 2}, {GR_MEDKIT, 2}, {GR_HEART, 3},
    {GR_HEART, 4}, {GR_NAILS, 1}, {GR_BOMBBAG, 3}, {GR_RICO, 1},
};
/* packs to raise to before fights 2..7 */
static const int RAISE_WIN[BRV_ROUNDS] = {1, 3, 5, 6, 7, 8, 8, 12};
static const int RAISE_CHERRY[BRV_ROUNDS] = {1, 4, 6, 8, 10, 12, 12, 12};

/* edges: a button counts as pressed only on a frame it wasn't held */
static int tap(int b) { return (prev_mask & b) ? 0 : b; }

static int reserve(void) {
    if (brv_bot_plan != 1) return 0;
    if (bv.round >= BRV_ROUNDS - 1) return 1300;
    if (bv.round >= 5) return 500;
    return 0;
}

static int want_item(void) {
    for (int i = 0; i < ARRAY_LEN(BUY); i++) {
        int it = BUY[i][0], tier = BUY[i][1];
        if (bv.gear[it] >= tier) continue;
        if (bv.gear[it] != tier - 1) continue;
        if (brv_price(it) <= bv.cash - reserve()) return it;
        return -1; /* save up for this one */
    }
    return -1;
}

static int shop_buttons(void) {
    int target = (brv_bot_plan == 1 ? RAISE_CHERRY : RAISE_WIN)[bv.round];
    int item = want_item();
    if (item >= 0) {
        if (bv.shop_row == 0) {
            if (bv.shop_btn != 0) return tap(BTN_LEFT);
            return tap(BTN_A);
        }
        int s = bv.shop_sel;
        if (s == item) return tap(BTN_A);
        if ((s & 3) < (item & 3)) return tap(BTN_RIGHT);
        if ((s & 3) > (item & 3)) return tap(BTN_LEFT);
        if (s / 4 < item / 4) return tap(BTN_DOWN);
        return tap(BTN_UP);
    }
    if (bv.shop_row == 1) return tap(BTN_B);
    bool last = bv.round == BRV_ROUNDS - 1;
    if (!last && bv.ngroups < target) {
        if (bv.shop_btn != 1) return tap(bv.shop_btn < 1 ? BTN_RIGHT : BTN_LEFT);
        return tap(BTN_A);
    }
    if (bv.shop_btn != 2) return tap(BTN_RIGHT);
    return tap(BTN_A);
}

/* ---- the fight ---------------------------------------------------------------- */

static int line_dir(float dx, float dy, float tol) {
    float ax = fabsf(dx), ay = fabsf(dy);
    if (ay < tol) return dx > 0 ? 0 : 4;
    if (ax < tol) return dy > 0 ? 2 : 6;
    if (fabsf(ax - ay) < tol) {
        if (dx > 0) return dy > 0 ? 1 : 7;
        return dy > 0 ? 3 : 5;
    }
    return -1;
}

static int dir8(float dx, float dy) {
    float a = atan2f(dy, dx);
    int d = (int)lroundf(a / 0.7853982f);
    return (d + 8) % 8;
}

static float foe_r(const Foe *f) {
    switch (f->kind) {
    case MK_GASBAG: return f->size == 0 ? 7 : f->size == 1 ? 5 : 3.5f;
    case MK_BRUTE: case MK_PEEPER: return 6;
    case MK_BOSS: return 14;
    case MK_MITE: case MK_FIZZER: return 4;
    default: return 5;
    }
}

static bool solid(const Foe *f) {
    if (!f->alive || f->warp > 0) return false;
    if (f->kind == MK_PEEPER) return f->state == 2;
    return true;
}

/* how bad it is to stand at (x, y) k frames from now */
static float danger_at(const Player *p, float x, float y, int k) {
    float d = 0;
    for (int i = 0; i < BRV_MAX_FOES; i++) {
        const Foe *f = &bv.foe[i];
        if (!f->alive) continue;
        if (f->kind == MK_PEEPER) {
            if (f->state == 4) continue;
            float dx = f->x - x, dy = f->y - y, dist = sqrtf(dx * dx + dy * dy);
            float R = f->state == 2 ? 16 : 13;
            if (dist < R) d += (R - dist) * (R - dist) * 6;
            continue;
        }
        float fx = f->x, fy = f->y;
        if (f->warp > 0) {
            if (f->warp > 20) continue;
        } else {
            fx += f->vx * (float)k;
            fy += f->vy * (float)k;
        }
        float dx = fx - x, dy = fy - y, dist = sqrtf(dx * dx + dy * dy);
        float R = foe_r(f) + 14 + (f->kind == MK_KEG && f->state >= 1 ? 6 : 0) + (f->kind == MK_BOSS ? 10 : 0);
        if (f->kind == MK_KEG || f->kind == MK_FIZZER) R += 8; /* they blow up when they die */
        if (dist < R) d += (R - dist) * (R - dist) * 4;
        if (f->kind == MK_KEG && f->state <= 1 && f->warp == 0) {
            float ex = x - f->x, ey = y - f->y;
            int ld = line_dir(ex, ey, 7);
            if (ld >= 0 && ex * ex + ey * ey < 200 * 200) d += f->state == 1 && ld == f->dir ? 300 : 25;
        }
    }
    for (int i = 0; i < BRV_MAX_ESHOTS; i++) {
        const EShot *s = &bv.es[i];
        if (!s->alive) continue;
        float sx = s->x + s->vx * (float)k, sy = s->y + s->vy * (float)k;
        float dx = sx - x, dy = sy - y, dist = sqrtf(dx * dx + dy * dy);
        if (dist < 11) d += (11 - dist) * (11 - dist) * 8;
    }
    for (int i = 0; i < BRV_MAX_BOMBS; i++) {
        const Bomb *b = &bv.bomb[i];
        if (!b->alive || b->drone) continue;
        float dx = b->x - x, dy = b->y - y, dist = sqrtf(dx * dx + dy * dy), R = BRV_BLAST_R + 12;
        if (dist < R) d += (R - dist) * (b->fuse - k < 40 ? 12.0f : 3.0f);
    }
    for (int i = 0; i < BRV_MAX_BLASTS; i++) {
        const Blast *b = &bv.blast[i];
        if (!b->alive) continue;
        float dx = b->x - x, dy = b->y - y;
        if (dx * dx + dy * dy < (b->r + 6) * (b->r + 6)) d += 200;
    }
    if (brv_in_lava(x, y + 4)) d += 2000;
    else if (brv_in_lava(x, y + 9) || brv_in_lava(x + 5, y + 4) || brv_in_lava(x - 5, y + 4) || brv_in_lava(x, y - 1)) d += 150;
    /* walls and corners (the pads) */
    float m = 18;
    if (x < BRV_AX0 + m) d += (BRV_AX0 + m - x) * 3;
    if (x > BRV_AX1 - m) d += (x - (BRV_AX1 - m)) * 3;
    if (y < BRV_AY0 + m) d += (BRV_AY0 + m - y) * 3;
    if (y > BRV_AY1 - m) d += (y - (BRV_AY1 - m)) * 3;
    for (int c = 0; c < 4; c++) {
        float px = (c & 1) ? BRV_AX1 - 16 : BRV_AX0 + 16, py = (c & 2) ? BRV_AY1 - 14 : BRV_AY0 + 14;
        float dx = px - x, dy = py - y;
        if (dx * dx + dy * dy < 30 * 30) d += 40;
    }
    (void)p;
    return d;
}

static int pick_target(const Player *p) {
    int best = -1;
    float bs = 1e9f;
    for (int i = 0; i < BRV_MAX_FOES; i++) {
        const Foe *f = &bv.foe[i];
        if (!solid(f)) continue;
        float dx = f->x - p->x, dy = f->y - p->y, dist = sqrtf(dx * dx + dy * dy);
        float s = dist;
        if (f->kind == MK_STILTER) s -= 40;
        if (f->kind == MK_PEEPER) s -= 30;
        if (f->kind == MK_FIZZER) s -= 30;
        if (f->kind == MK_KEG && dist < 40) s += 60; /* not up close */
        /* already in the line of fire */
        int d8 = dir8(dx, dy);
        if (d8 == p->face) s -= 25;
        if (s < bs) { bs = s; best = i; }
    }
    return best;
}

static int fight_buttons(void) {
    const Player *p = &bv.p[0];
    if (!p->on || p->down) return 0;
    int tgt = pick_target(p);
    int mask = 0;

    /* where to stand */
    static const int MV[9][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
    static const int KS[4] = {3, 8, 14, 22};
    static const float KW[4] = {1.6f, 1.2f, 0.8f, 0.5f};
    float best = 1e12f;
    int bm = 0;
    for (int m = 0; m < 9; m++) {
        float sp = (MV[m][0] && MV[m][1]) ? 0.7071f : 1.0f;
        float score = 0;
        for (int k = 0; k < 4; k++) {
            float x = fclamp(p->x + MV[m][0] * sp * KS[k], BRV_AX0 + 5, BRV_AX1 - 5);
            float y = fclamp(p->y + MV[m][1] * sp * KS[k], BRV_AY0 + 7, BRV_AY1 - 5);
            score += danger_at(p, x, y, KS[k]) * KW[k];
        }
        float x = fclamp(p->x + MV[m][0] * sp * 8, BRV_AX0 + 5, BRV_AX1 - 5);
        float y = fclamp(p->y + MV[m][1] * sp * 8, BRV_AY0 + 7, BRV_AY1 - 5);
        if (tgt >= 0) {
            const Foe *f = &bv.foe[tgt];
            float dx = f->x - x, dy = f->y - y, dist = sqrtf(dx * dx + dy * dy);
            float want = f->kind == MK_BOSS ? 85 : f->kind == MK_KEG ? 75 : 60;
            score += fabsf(dist - want) * 0.25f;
            /* line it up with the way the gun points */
            float fx = (float)BRV_DX[p->face], fy = (float)BRV_DY[p->face];
            float fl = sqrtf(fx * fx + fy * fy);
            fx /= fl;
            fy /= fl;
            float along = dx * fx + dy * fy, perp = fabsf(dx * fy - dy * fx);
            if (along > 0) score += perp * 0.9f;
            else score += 30;
        } else {
            float dx = BRV_CX - x, dy = BRV_CY - y;
            score += sqrtf(dx * dx + dy * dy) * 0.1f;
        }
        /* medkits */
        for (int i = 0; i < BRV_MAX_MEDS; i++) {
            const Medkit *md = &bv.med[i];
            if (!md->alive) continue;
            float dx = md->x - x, dy = md->y - y;
            score -= 400.0f / (sqrtf(dx * dx + dy * dy) + 20.0f);
        }
        if (m != last_move) score += 2;
        if (score < best) { best = score; bm = m; }
    }
    last_move = bm;

    /* aiming: hold fire while the target is in line; otherwise turn */
    bool fire = tgt >= 0;
    if (tgt >= 0) {
        const Foe *f = &bv.foe[tgt];
        float dx = f->x - p->x, dy = f->y - p->y;
        int want = dir8(dx, dy);
        float fx = (float)BRV_DX[p->face], fy = (float)BRV_DY[p->face];
        float fl = sqrtf(fx * fx + fy * fy);
        float along = (dx * fx + dy * fy) / fl, perp = fabsf(dx * fy - dy * fx) / fl;
        bool in_line = along > 0 && perp < foe_r(f) + 3;
        if (in_line) turn_t = 0;
        else turn_t++;
        if (want != p->face && (turn_t > 10 || along <= 0)) {
            /* let go for a frame and point the pad the new way */
            turn_t = 0;
            mask |= (BRV_DX[want] > 0 ? BTN_RIGHT : BRV_DX[want] < 0 ? BTN_LEFT : 0) |
                    (BRV_DY[want] > 0 ? BTN_DOWN : BRV_DY[want] < 0 ? BTN_UP : 0);
            fire = false;
            just_turned = true;
        } else if (just_turned) {
            just_turned = false;
            if (fire) return BTN_A;
        }
    }
    if (!(mask & (BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN))) {
        int mx = MV[bm][0], my = MV[bm][1];
        if (mx > 0) mask |= BTN_RIGHT;
        if (mx < 0) mask |= BTN_LEFT;
        if (my > 0) mask |= BTN_DOWN;
        if (my < 0) mask |= BTN_UP;
    }
    /* a fresh press of fire every so often, or the hold would never start
     * again after a scene change swallowed it */
    if (fire && !((prev_mask & BTN_A) && !(input_held() & BTN_A) && bv.fight_t > 2)) mask |= BTN_A;

    /* bombs: on a crowd, and on a peeper's tone */
    if (p->bombs > 0) {
        int crowd = 0;
        bool peep = false;
        for (int i = 0; i < BRV_MAX_FOES; i++) {
            const Foe *f = &bv.foe[i];
            if (!f->alive || f->warp > 0) continue;
            float dx = f->x - p->x, dy = f->y - p->y, d2 = dx * dx + dy * dy;
            if (f->kind == MK_PEEPER && f->state == 0 && f->st > 30 && d2 < 14 * 14) peep = true;
            if (solid(f) && d2 < 34 * 34) crowd += f->kind == MK_BRUTE || f->kind == MK_STILTER ? 2 : 1;
        }
        bool near_bomb = false;
        for (int i = 0; i < BRV_MAX_BOMBS; i++)
            if (bv.bomb[i].alive) {
                float dx = bv.bomb[i].x - p->x, dy = bv.bomb[i].y - p->y;
                if (dx * dx + dy * dy < 40 * 40) near_bomb = true;
            }
        if (!near_bomb && (peep || crowd >= 4)) mask |= tap(BTN_B);
    }
    return mask;
}

int brv_bot_buttons(void) {
    int mask = 0;
    bv.bot_t++;
    switch (bv.state) {
    case BS_TITLE: mask = bv.menu == 0 ? tap(BTN_A) : tap(BTN_UP); break;
    case BS_STORY: mask = bv.state_t > 22 ? tap(BTN_A) : 0; break;
    case BS_BANNER: mask = 0; break;
    case BS_FIGHT: mask = fight_buttons(); break;
    case BS_WON: mask = bv.state_t > 42 ? tap(BTN_A) : 0; break;
    case BS_SHOP: mask = shop_buttons(); break;
    case BS_OVER: mask = bv.state_t > 62 ? tap(BTN_A) : 0; break;
    case BS_ENDING: mask = bv.state_t > 122 ? tap(BTN_A) : 0; break;
    case BS_CREDITS: mask = BTN_A; break;
    default: break;
    }
    prev_mask = mask;
    return mask;
}
