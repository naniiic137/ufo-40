/* SHUTTERBUG - the demo player. It only chooses buttons (the tests press
 * them for real). Every frame, for each ship, it plans a lane through the
 * rock ahead, picks something to shoot or photograph, then tries each way
 * the pad can point, flying the ship (and the scroll) a few dozen frames
 * ahead against every shot and foe near it, and takes the safest move that
 * also heads for its goal. It holds B to fire, lets go to throw charged
 * rings or to call bulbs in, and taps A when the photo would catch
 * something worth it: a letter, a red repair foe, a boss's weak point, a
 * crowd, a green foe or a foe on the rock. */
#include "shutterbug.h"

bool shb_bot_letters = true;

#define H 20          /* frames looked ahead */
#define NEAR 110.0f
#define MAXT 200
#define LANE_COLS 30

static uint32_t tick;
static float fpx[SHB_MAX_FOES], fpy[SHB_MAX_FOES];
static uint8_t fseen[SHB_MAX_FOES];
static bool prev_a[2];

typedef struct { float x[H + 1], y[H + 1], rx, ry; bool laser; int idx; } Threat;
static Threat thr[MAXT];
static int nthr;

static bool box_hits_rock(float x, float y) {
    for (float yy = y - SHB_BOX_H; yy <= y + SHB_BOX_H; yy += SHB_BOX_H)
        for (float xx = x - SHB_BOX_W; xx <= x + SHB_BOX_W; xx += SHB_BOX_W)
            if (shb_solid_at(xx, yy)) return true;
    return false;
}

static float scroll_speed(void) {
    const StageDef *st = &SHB_STAGE[sb.stage];
    if (sb.hold != HOLD_NONE || sb.cam_x >= st->end_x) return 0;
    return st->scroll[sb.scroll_i].speed / 4.0f;
}

static void gather(const Ship *s) {
    nthr = 0;
    for (int i = 0; i < SHB_MAX_ESHOTS && nthr < MAXT; i++) {
        EShot *e = &sb.es[i];
        if (!e->alive) continue;
        if (fabsf(e->x - s->x) > NEAR || fabsf(e->y - s->y) > NEAR) continue;
        Threat *t = &thr[nthr++];
        t->rx = t->ry = e->r;
        t->laser = false;
        t->idx = -1;
        EShot c = *e;
        for (int k = 0; k <= H; k++) {
            t->x[k] = c.x;
            t->y[k] = c.y;
            if (c.alive) shb_eshot_step(&c);
            else { c.x = -9999; }
        }
    }
    for (int i = 0; i < SHB_MAX_FOES && nthr < MAXT; i++) {
        Foe *e = &sb.foe[i];
        if (!e->alive || e->t < 0) continue;
        if (e->kind == K_LASER) {
            if (fabsf(e->x - s->x) > NEAR) continue;
            Threat *t = &thr[nthr++];
            t->laser = true;
            t->idx = i;
            t->rx = e->hw + 1;
            t->ry = e->hh;
            for (int k = 0; k <= H; k++) { t->x[k] = e->x; t->y[k] = e->y; }
            continue;
        }
        if (!shb_foe_harmful(e) && !(e->kind == K_GHOST)) continue;
        if (fabsf(e->x - s->x) > NEAR + e->hw || fabsf(e->y - s->y) > NEAR + e->hh) continue;
        float vx = fseen[i] ? e->x - fpx[i] : 0, vy = fseen[i] ? e->y - fpy[i] : 0;
        if (e->stun > 0 && !e->falling) vx = vy = 0;
        Threat *t = &thr[nthr++];
        t->laser = false;
        t->idx = i;
        t->rx = e->hw + 1;
        t->ry = e->hh + 1;
        for (int k = 0; k <= H; k++) { t->x[k] = e->x + vx * k; t->y[k] = e->y + vy * k; }
    }
}

static bool laser_on_at(int i, int k) {
    const Foe *e = &sb.foe[i];
    int c = (e->t + k + e->arg2) % (e->arg * 2);
    return c < e->arg || c > e->arg * 2 - 4; /* a little early, to be safe */
}

typedef struct { float score; int hit; } Eval;

static Eval evaluate(const Ship *s, int dx, int dy, float gx, float gy, float sv) {
    float sp = SHB_SPEED * (dx && dy ? 0.7071f : 1.0f);
    float px = s->x, py = s->y, cam = sb.cam_x;
    int hit = H + 1;
    float clear = 30;
    bool crushed = false;
    for (int k = 1; k <= H && hit > H; k++) {
        cam += sv;
        float nx = fclamp(px + dx * sp, cam + 8, cam + SCREEN_W - 10);
        float ny = fclamp(py + dy * sp, sb.cam_y + 6, sb.cam_y + SHB_PF_H - 6);
        if (s->ghost > 0) { px = nx; py = ny; }
        else {
            if (!box_hits_rock(nx, py)) px = nx;
            if (!box_hits_rock(px, ny)) py = ny;
        }
        if (px < cam + 8) {
            px = cam + 8;
            if (box_hits_rock(px, py) && s->ghost == 0) { crushed = true; hit = k; break; }
        }
        for (int i = 0; i < nthr; i++) {
            Threat *t = &thr[i];
            if (t->laser && !laser_on_at(t->idx, k)) continue;
            float ddx = fabsf(t->x[k] - px) - t->rx - SHB_HIT_R, ddy = fabsf(t->y[k] - py) - t->ry - SHB_HIT_R;
            float d = fmaxf(ddx, ddy);
            float margin = 1.5f + k * 0.08f;
            if (d < margin) { hit = k; break; }
            if (d < clear) clear = d;
        }
    }
    Eval ev;
    ev.hit = hit;
    float sc = 0;
    if (hit <= H) sc -= 10000 - hit * 400 + (crushed ? 2000 : 0);
    sc += clear * 2.5f;
    float gdx = gx - px, gdy = gy - py;
    sc -= sqrtf(gdx * gdx * 0.5f + gdy * gdy * 1.0f) * 2.0f;
    /* not too near the left edge, where the scroll can pin the ship */
    if (px < cam + 30 && sv > 0) sc -= (cam + 30 - px) * 4;
    ev.score = sc;
    return ev;
}

/* ---- lanes through the rock -------------------------------------------- */

static bool free_at(int c, int r) {
    if (r < 0 || r >= shb_rows) return false;
    return !shb_solid_at(c * SHB_TILE + 4.0f, r * SHB_TILE + 4.0f);
}

/* the world y of the best lane: reachable far ahead, and near want_y */
static float plan_lane(const Ship *s, float want_y) {
    static uint8_t reach[LANE_COLS + 1][SHB_MAX_ROWS];
    int c0 = (int)(s->x / SHB_TILE);
    float lo = fminf(shb_band_lo(s->x), sb.cam_y), hi = fmaxf(shb_band_hi(s->x + 160), sb.cam_y);
    int rlo = imax(0, (int)(lo / SHB_TILE)), rhi = imin(shb_rows - 2, (int)((hi + SHB_PF_H) / SHB_TILE) - 2);
    for (int r = 0; r < SHB_MAX_ROWS; r++) reach[LANE_COLS][r] = r >= rlo && r <= rhi;
    for (int k = LANE_COLS - 1; k >= 0; k--) {
        int c = c0 + k;
        for (int r = 0; r < shb_rows; r++) {
            reach[k][r] = 0;
            if (r < rlo || r > rhi) continue;
            if (!free_at(c, r) || !free_at(c, r + 1) || !free_at(c + 1, r) || !free_at(c + 1, r + 1)) continue;
            for (int d = -2; d <= 2 && !reach[k][r]; d++) {
                int r2 = r + d;
                if (r2 < 0 || r2 >= shb_rows || !reach[k + 1][r2]) continue;
                bool ok = true;
                for (int m = imin(r, r2); m <= imax(r, r2) + 1; m++) if (!free_at(c + 1, m)) ok = false;
                if (ok) reach[k][r] = 1;
            }
        }
    }
    /* the reachable row in the next column or two nearest the wish */
    float best = s->y, bd = 1e9f;
    for (int k = 1; k <= 2; k++)
        for (int r = rlo; r <= rhi; r++) {
            if (!reach[k][r]) continue;
            float y = r * SHB_TILE + 8.0f;
            float d = fabsf(y - want_y) + fabsf(y - s->y) * 0.15f;
            if (d < bd) { bd = d; best = y; }
        }
    return best;
}

/* ---- what to aim at ------------------------------------------------------ */

static bool needs_photo(const Foe *e) {
    switch (e->kind) {
    case K_TEAPOT: return !e->open;
    case K_SIGNAL: return e->stun <= 0 && e->phase != 2;
    case K_JAW: return e->stun <= 0;
    case K_KALEI: return !e->open && e->state > 0;
    default: return false;
    }
}

static int pick_target(const Ship *s) {
    int best = -1;
    float bs = -1e9f;
    for (int i = 0; i < SHB_MAX_FOES; i++) {
        Foe *e = &sb.foe[i];
        if (!e->alive || e->t < 0 || e->role == ROLE_PROP || e->kind == K_LASER || e->kind == K_SHARD) continue;
        float sx = e->x - sb.cam_x;
        if (sx < 0 || sx > SCREEN_W + 10 || e->x < s->x + 4) continue;
        if (e->y < sb.cam_y - 4 || e->y > sb.cam_y + SHB_PF_H + 4) continue;
        float sc = -fabsf(e->y - s->y) * 1.5f - (e->x - s->x) * 0.3f;
        if (e->role == ROLE_BOSS || e->role == ROLE_MID) sc += 200;
        if (e->kind == K_GEN) sc += 120;
        if (e->kind == K_CHUTE) sc += 40;
        if (e->kind == K_GHOST && e->state >= 2 && e->stun <= 0) sc -= 200;
        if (sc > bs) { bs = sc; best = i; }
    }
    return best;
}

/* is something worth a photo in the frame if it were snapped now? */
static int photo_worth(const Ship *s, bool *letter_ahead) {
    float cx = s->x + SHB_CURSOR_DX, cy = s->y;
    float hw = SHB_PHOTO_W / 2.0f - 2, hh = SHB_PHOTO_H / 2.0f - 2;
    int worth = 0, foes = 0;
    *letter_ahead = false;
    for (int i = 0; i < SHB_MAX_FOES; i++) {
        Foe *e = &sb.foe[i];
        if (!e->alive || e->t < 0) continue;
        bool in = fabsf(e->x - cx) < hw + e->hw * 0.5f && fabsf(e->y - cy) < hh + e->hh * 0.5f;
        if (e->kind == K_LETTER && !((sb.letters >> e->arg) & 1)) {
            if (e->x > s->x && e->x < s->x + 420 && shb_bot_letters) *letter_ahead = true;
            if (in && shb_bot_letters) worth += 100;
            continue;
        }
        if (!in) continue;
        if (e->kind == K_SECRET && e->state == 0) worth += 6;
        if (e->role == ROLE_PROP || e->kind == K_LASER || e->kind == K_SHARD) continue;
        if (e->stun > 0 || e->falling) continue;
        if (e->kind == K_SIGNAL && e->phase == 2) continue; /* faded: the photo would be wasted */
        if (e->flags & F_RED) worth += 60;
        if (needs_photo(e)) worth += 50;
        if (e->role == ROLE_MID || e->role == ROLE_BOSS) worth += 30;
        if (SHB_FOE[e->kind].mount) worth += 4;
        if (e->flags & F_GREEN) worth += 3;
        if (e->kind == K_CART || e->kind == K_RAILBOMB || e->kind == K_GEN || e->kind == K_BURSTER) worth += 5;
        foes++;
    }
    worth += foes >= 3 ? 8 : foes == 2 ? 4 : 0;
    return worth;
}

static int ship_buttons(int p) {
    Ship *s = &sb.ship[p];
    if (!s->on || !s->alive) { prev_a[p] = false; return 0; }
    float sv = scroll_speed();
    gather(s);
    int tgt = pick_target(s);
    bool letter_ahead = false;
    int worth = photo_worth(s, &letter_ahead);
    bool full = s->flash >= SHB_FLASH_MAX;
    float want_x = sb.cam_x + (p ? 70 : 96), want_y = s->y;
    /* the big foes: photograph their weak point, then hit it up close */
    bool boss_fight = false;
    if (tgt >= 0) {
        Foe *e = &sb.foe[tgt];
        want_y = e->y;
        if (e->role == ROLE_BOSS || e->role == ROLE_MID) {
            boss_fight = true;
            if (needs_photo(e) && full) { want_x = e->x - SHB_CURSOR_DX; }
            else want_x = e->x - (e->stun > 0 || e->open ? 52 : 80);
            if (e->kind == K_JAW) want_y = e->y + 4;
        }
    }
    /* a letter, a secret or a red foe ahead: fly to frame it */
    for (int i = 0; i < SHB_MAX_FOES; i++) {
        Foe *e = &sb.foe[i];
        if (!e->alive || e->t < 0) continue;
        bool want = (e->kind == K_LETTER && shb_bot_letters && !((sb.letters >> e->arg) & 1)) || ((e->flags & F_RED) && e->stun <= 0);
        if (!want || e->x < s->x + 40 || e->x > s->x + 330) continue;
        want_y = e->y;
        if (e->kind == K_LETTER) want_x = fmaxf(want_x, e->x - SHB_CURSOR_DX);
        break;
    }
    /* a wrench to catch */
    for (int i = 0; i < SHB_MAX_PICKUPS; i++) {
        Pickup *k = &sb.pk[i];
        if (k->alive && k->kind == PK_WRENCH && (!s->armour || p == 0)) { want_x = k->x; want_y = k->y; break; }
    }
    want_x = fclamp(want_x, sb.cam_x + 30, sb.cam_x + SCREEN_W - 20);
    float gy = sv > 0 || SHB_STAGE[sb.stage].theme != TH_SPACE ? plan_lane(s, want_y) : want_y;
    if (boss_fight && sv == 0) gy = want_y;
    gy = fclamp(gy, sb.cam_y + 10, sb.cam_y + SHB_PF_H - 10);
    int bdx = 0, bdy = 0;
    Eval best = {-1e9f, 0};
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            Eval ev = evaluate(s, dx, dy, want_x, gy, sv);
            if (ev.score > best.score) { best = ev; bdx = dx; bdy = dy; }
        }
    int m = 0;
    if (bdx < 0) m |= BTN_LEFT;
    if (bdx > 0) m |= BTN_RIGHT;
    if (bdy < 0) m |= BTN_UP;
    if (bdy > 0) m |= BTN_DOWN;
    /* fire: hold B, except to throw ready rings at something close, or to
     * call the bulbs in when the camera wants them */
    bool fire = true;
    if (s->hold_t >= SHB_CHARGE_T) {
        bool close = false;
        for (int i = 0; i < SHB_MAX_FOES; i++) {
            Foe *e = &sb.foe[i];
            if (!e->alive || e->t < 0 || e->role == ROLE_PROP || e->kind == K_LASER || e->kind == K_SHARD) continue;
            float ddx = e->x - s->x;
            if (ddx > 0 && ddx < (e->role == ROLE_FOE ? 90 : 120) && fabsf(e->y - s->y) < 50 && shb_foe_hittable(e)) close = true;
        }
        if (close) fire = false;
    }
    bool crystals = false;
    for (int i = 0; i < SHB_MAX_PICKUPS; i++)
        if (sb.pk[i].alive && sb.pk[i].kind == PK_CRYSTAL) crystals = true;
    if (crystals && !full && (tick % 40) < 30) fire = false;
    if (fire) m |= BTN_B;
    /* the camera */
    bool snap = false;
    if (full) {
        int need = letter_ahead ? 100 : boss_fight ? 30 : 6;
        if (worth >= need) snap = true;
    }
    if (snap && !prev_a[p]) m |= BTN_A;
    prev_a[p] = (m & BTN_A) != 0;
    return m;
}

int shb_bot_buttons(void) {
    tick++;
    int m = 0;
    bool edge = tick % 2 == 0;
    switch (sb.state) {
    case SS_TITLE:
        if (edge && sb.state_t > 10) m = BTN_A; /* whichever mode the cursor is on */
        break;
    case SS_STORY: if (edge && sb.state_t > 20) m = BTN_A; break;
    case SS_PLAY:
        m = ship_buttons(0);
        if (sb.players == 2) m |= ship_buttons(1) << BTN_P2_SHIFT;
        break;
    case SS_LOST: case SS_OVER: case SS_CLEAR: case SS_ENDING:
        if (edge && sb.state_t > 70) m = BTN_A;
        break;
    case SS_CREDITS: m = BTN_A; break;
    default: break;
    }
    for (int i = 0; i < SHB_MAX_FOES; i++) { fpx[i] = sb.foe[i].x; fpy[i] = sb.foe[i].y; fseen[i] = sb.foe[i].alive; }
    return m;
}
