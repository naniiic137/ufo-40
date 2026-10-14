/* SHOWSTOPPER - the demo player, used by the tests and the screenshots. It
 * presses real buttons. In a stage it taps A at the nearest foe (or the
 * boss's weak spot) and, when trouble is near, tries a set of moves in a
 * copy of the game (walks eight ways, dashes eight ways, each followed by a
 * walk up the stage's guide) and keeps the one that loses nothing and gets
 * furthest or hurts the boss most. In the Hop it looks at where each next
 * hop would land and when. */
#include "sst.h"

int sst_bot_why;
int sst_bot_debug;

static int tog;
static SstGame keep;

static int aim_dir8(int ang) { return ((ang + 4) / 8) & 7; }

static uint32_t dir_mask(int d) {
    if (d < 0) return 0;
    uint32_t m = 0;
    if (SST_DX8[d] > 0) m |= BTN_RIGHT;
    if (SST_DX8[d] < 0) m |= BTN_LEFT;
    if (SST_DY8[d] > 0) m |= BTN_DOWN;
    if (SST_DY8[d] < 0) m |= BTN_UP;
    return m;
}

/* the thing to shoot at: the boss's soft spot, else the nearest foe or bomb shot on the screen */
static bool pick_target(int *tx, int *ty) {
    if (sst_boss_weak(tx, ty)) return true;
    const SstPlayer *p = &sst_g.pl;
    int best = 1 << 30;
    bool found = false;
    for (int i = 0; i < SST_MAX_FOES; i++) {
        const SstFoe *f = &sst_g.foe[i];
        if (!f->alive || !sst_on_screen(f->x, f->y, -6)) continue;
        if (f->kind == FK_BAT && f->a == 0) continue;
        int d = sst_dist(SST_PX(f->x - p->x), SST_PX(f->y - p->y));
        if (f->kind == FK_CRATE || f->kind == FK_BARREL || f->kind == FK_SECRET) d += 60;
        if (SST_PX(f->y) > SST_PX(p->y) + 40 && SST_STAGE[sst_g.stage].kind == SK_SCROLL) d += 80;
        if (d < best) { best = d; *tx = f->x; *ty = f->y; found = true; }
    }
    for (int i = 0; i < SST_MAX_BULLETS; i++) {
        const SstBullet *b = &sst_g.eb[i];
        if (!b->alive || b->kind != EB_MINE) continue;
        int d = sst_dist(SST_PX(b->x - p->x), SST_PX(b->y - p->y)) - 30;
        if (d < best) { best = d; *tx = b->x; *ty = b->y; found = true; }
    }
    return found;
}

/* the walk up the stage: toward the neighbouring tile nearest the top */
static int guide_dir(void) {
    const SstPlayer *p = &sst_g.pl;
    if (SST_STAGE[sst_g.stage].kind != SK_SCROLL) return -1;
    int px = SST_PX(p->x), py = SST_PX(p->y);
    int tx = px >> 4, ty = py >> 4;
    int here = sst_guide(tx, ty), best = here, bx = tx, by = ty;
    for (int d = 0; d < 8; d++) {
        int nx = tx + SST_DX8[d], ny = ty + SST_DY8[d];
        if (SST_DX8[d] && SST_DY8[d]) {
            /* a diagonal only round no corner */
            if (sst_tile_solid(sst_tile_at(nx, ty)) || sst_tile_solid(sst_tile_at(tx, ny))) continue;
        }
        int g = sst_guide(nx, ny);
        if (g < best) { best = g; bx = nx; by = ny; }
    }
    int cx = bx * 16 + 8, cy = by * 16 + 8;
    if (bx == tx && by == ty) cy -= 8;
    int dx = cx - px, dy = cy - py;
    int sx = dx > 2 ? 1 : dx < -2 ? -1 : 0, sy = dy > 2 ? 1 : dy < -2 ? -1 : 0;
    for (int d = 0; d < 8; d++)
        if (SST_DX8[d] == sx && SST_DY8[d] == sy) return d;
    return -1;
}

static bool fighting(void) { return sst_g.boss.alive && !sst_g.boss.dying; }

/* where to stand in a fight, and in the one-screen stages */
static int stand_dir(void) {
    const SstPlayer *p = &sst_g.pl;
    int px = SST_PX(p->x), py = SST_PX(p->y);
    int wx = 160, wy = sst_g.cam_y + 120;
    if (fighting()) {
        int bx, by;
        if (sst_boss_weak(&bx, &by)) {
            wx = SST_PX(bx);
            wy = imin(SST_PX(by) + 80, sst_g.cam_y + SST_VIEW_H - 24);
            if (sst_g.boss.kind == BK_HEART) { wx = 160; wy = SST_PX(sst_g.boss.y); }
            if (sst_g.boss.kind == BK_RIBBON || sst_g.boss.kind == BK_DOPPELS) wy = sst_g.cam_y + 130;
        }
    } else if (SST_STAGE[sst_g.stage].kind == SK_TRAIN) {
        wx = 160;
        wy = 88;
    } else if (SST_STAGE[sst_g.stage].kind == SK_LIFT) {
        wx = 168;
        wy = 90;
    }
    int dx = wx - px, dy = wy - py;
    int sx = dx > 6 ? 1 : dx < -6 ? -1 : 0, sy = dy > 6 ? 1 : dy < -6 ? -1 : 0;
    for (int d = 0; d < 8; d++)
        if (SST_DX8[d] == sx && SST_DY8[d] == sy) return d;
    return -1;
}

static int follow_dir(void) {
    if (fighting() || SST_STAGE[sst_g.stage].kind != SK_SCROLL || sst_g.cam_lock >= 0) {
        if (!fighting() && SST_STAGE[sst_g.stage].kind == SK_SCROLL) return guide_dir();
        return stand_dir();
    }
    return guide_dir();
}

/* A: tapped at the target, the pad on the target for the press */
static int fire_dir = -1;
static uint32_t fire_buttons(int frame, uint32_t move, int aim) {
    if (aim < 0) return move;
    if (frame % 6 == 0) return BTN_A | dir_mask(aim);
    return move;
}

/* ---- trying moves -------------------------------------------------------------------- */

typedef struct { int dir, dash, follow; } Plan;
static bool has_target;
static int tgt_x, tgt_y;

/* how far (px) the target lies off the nearest of the eight shooting lines */
static int aim_miss(int dx, int dy) {
    int best = 1 << 30;
    for (int d = 0; d < 8; d++) {
        int ex = SST_DX8[d], ey = SST_DY8[d];
        int dot = dx * ex + dy * ey;
        if (dot <= 0) continue;
        int cross = iabs(dx * ey - dy * ex);
        if (ex && ey) cross = cross * 181 / 256;
        if (cross < best) best = cross;
    }
    return best == 1 << 30 ? 200 : best;
}

static long long score_now(int lives0, int hits0, int bosshp0, int cards0, int passes0) {
    const SstPlayer *p = &sst_g.pl;
    long long s = 0;
    if (sst_g.lives < lives0 || p->state == PS_FALL || p->state == PS_DOWN || p->state == PS_BUBBLE || p->state == PS_GONE)
        s -= 10000000;
    s -= (long long)(sst_g.hits_taken - hits0) * 400000;
    s += (long long)(sst_g.cards_got - cards0) * 2000;
    s += (long long)(sst_g.passes_run - passes0) * 60000;
    int px = SST_PX(p->x), py = SST_PX(p->y);
    if (fighting() || bosshp0 > 0) s += (long long)(bosshp0 - (sst_g.boss.alive ? sst_g.boss.hp : 0)) * 3000;
    if (SST_STAGE[sst_g.stage].kind == SK_SCROLL && !fighting()) {
        int g = sst_guide(px >> 4, py >> 4);
        s -= (long long)g * 4000 + (py & 15) * 150;
    }
    /* keep clear of what is flying about */
    for (int i = 0; i < SST_MAX_BULLETS; i++) {
        const SstBullet *b = &sst_g.eb[i];
        if (!b->alive) continue;
        int d = sst_dist(SST_PX(b->x) - px, SST_PX(b->y) - py);
        if (d < 26) s -= (26 - d) * 400;
    }
    for (int i = 0; i < SST_MAX_FOES; i++) {
        const SstFoe *f = &sst_g.foe[i];
        if (!f->alive || (SST_FOE[f->kind].flags & (FF_NOHURT | FF_WALL | FF_SOLID))) continue;
        int d = sst_dist(SST_PX(f->x) - px, SST_PX(f->y) - py);
        if (d < 30) s -= (30 - d) * 500;
    }
    if (fighting() || SST_STAGE[sst_g.stage].kind != SK_SCROLL) {
        /* the place to stand */
        int wx = 160, wy = sst_g.cam_y + 120;
        int bx, by;
        if (fighting() && sst_boss_weak(&bx, &by)) {
            wx = SST_PX(bx);
            wy = imin(SST_PX(by) + 80, sst_g.cam_y + SST_VIEW_H - 24);
            /* lined up with the soft spot on one of the eight ways */
            s -= (long long)aim_miss(SST_PX(bx) - px, SST_PX(by) - py) * 900;
        }
        if (SST_STAGE[sst_g.stage].kind == SK_TRAIN) { wy = 88; if (!fighting()) wx = 160; }
        s -= (long long)sst_dist(px - wx, py - wy) * 60;
    }
    /* lined up with the thing being shot at */
    if (has_target && !fighting()) {
        int miss = aim_miss(SST_PX(tgt_x) - px, SST_PX(tgt_y) - py);
        s -= (long long)imin(miss, 40) * 350;
    }
    /* not hard against the bottom of the view while it climbs */
    if (sst_g.chasing && py > sst_g.cam_y + SST_VIEW_H - 30) s -= (py - (sst_g.cam_y + SST_VIEW_H - 30)) * 2000;
    return s;
}

static long long try_plan(Plan pl, int aim, int H) {
    memcpy(&keep, &sst_g, sizeof keep);
    sst_sim = true;
    int lives0 = sst_g.lives, hits0 = sst_g.hits_taken, cards0 = sst_g.cards_got, passes0 = sst_g.passes_run;
    int bosshp0 = sst_g.boss.alive ? sst_g.boss.hp : 0;
    long long worst = 0;
    for (int f = 0; f < H; f++) {
        int d = pl.dir == -2 ? follow_dir() : f < 10 || !pl.follow ? pl.dir : follow_dir();
        uint32_t m = dir_mask(d);
        if (pl.dash && f == 0) m |= BTN_B;
        m = fire_buttons(f + tog, m, aim);
        sst_play_step(m);
        if (sst_g.lives < lives0 || sst_g.pl.state == PS_FALL || sst_g.pl.state == PS_DOWN || sst_g.pl.state == PS_GONE) {
            worst = -10000000LL - (long long)(H - f) * 100000;
            break;
        }
        if (sst_g.hits_taken > hits0) {
            worst = -400000LL * (sst_g.hits_taken - hits0) - (long long)(H - f) * 5000;
            break;
        }
    }
    long long s = worst ? worst + score_now(lives0, hits0, bosshp0, cards0, passes0) / 4 : score_now(lives0, hits0, bosshp0, cards0, passes0);
    sst_sim = false;
    memcpy(&sst_g, &keep, sizeof keep);
    return s;
}

static bool danger_near(void) {
    const SstPlayer *p = &sst_g.pl;
    if (fighting() || SST_STAGE[sst_g.stage].kind != SK_SCROLL || sst_g.chasing) return true;
    for (int i = 0; i < SST_MAX_BULLETS; i++)
        if (sst_g.eb[i].alive && sst_dist(SST_PX(sst_g.eb[i].x - p->x), SST_PX(sst_g.eb[i].y - p->y)) < 90) return true;
    for (int i = 0; i < SST_MAX_FOES; i++) {
        const SstFoe *f = &sst_g.foe[i];
        if (f->alive && !(SST_FOE[f->kind].flags & (FF_NOHURT | FF_SOLID)) && sst_dist(SST_PX(f->x - p->x), SST_PX(f->y - p->y)) < 80)
            return true;
    }
    for (int i = 0; i < SST_MAX_HAZ; i++)
        if (sst_g.hz[i].alive && sst_dist(SST_PX(sst_g.hz[i].x - p->x), SST_PX(sst_g.hz[i].y - p->y)) < 50) return true;
    /* a pit ahead wants a dash */
    int tx = SST_PX(p->x) >> 4, ty = SST_PX(p->y) >> 4;
    for (int dy = -2; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (sst_tile_pit(sst_tile_at(tx + dx, ty + dy))) return true;
    return false;
}

static Plan cur = {-1, 0, 1};
static int plan_t;

static uint32_t play_buttons(void) {
    const SstPlayer *p = &sst_g.pl;
    if (p->state == PS_BUBBLE) {
        sst_bot_why = 9;
        /* float to firm ground away from trouble, then pop */
        int sx = SST_PX(p->safe_x), sy = SST_PX(p->safe_y);
        bool on = sst_on_screen(p->safe_x, p->safe_y, -12);
        if (!on) { sx = 160; sy = sst_g.cam_y + 110; }
        int t = sst_tile_at(SST_PX(p->x) >> 4, SST_PX(p->y) >> 4);
        bool firm = !sst_tile_pit(t) && !sst_tile_solid(t);
        if (firm && p->st > 24 && sst_dist(SST_PX(p->x) - sx, SST_PX(p->y) - sy) < 20) return (tog / 3) % 2 ? BTN_A : 0;
        int dx = sx - SST_PX(p->x), dy = sy - SST_PX(p->y);
        uint32_t m = 0;
        if (dx > 2) m |= BTN_RIGHT;
        if (dx < -2) m |= BTN_LEFT;
        if (dy > 2) m |= BTN_DOWN;
        if (dy < -2) m |= BTN_UP;
        if (!m && firm && p->st > 24) m = (tog / 3) % 2 ? BTN_A : 0;
        return m;
    }
    if (p->state != PS_WALK && p->state != PS_DASH) return 0;
    int aim = -1;
    int tx, ty;
    has_target = pick_target(&tx, &ty);
    if (has_target) {
        aim = aim_dir8(sst_dir_to(tx - p->x, ty - p->y));
        tgt_x = tx;
        tgt_y = ty;
    }
    fire_dir = aim;
    if (!danger_near()) {
        sst_bot_why = 1;
        plan_t = 0;
        int d = follow_dir();
        return fire_buttons(tog, dir_mask(d), aim);
    }
    if (plan_t <= 0) {
        sst_bot_why = 2;
        static const int H = 30;
        long long best = -((long long)1 << 62);
        Plan bp = cur;
        for (int d = -2; d < 8; d++)
            for (int fol = 0; fol < 2; fol++) {
                if (d == -2 && fol) continue;
                Plan pl = {d, 0, fol};
                long long s = try_plan(pl, aim, H);
                if (d == cur.dir && fol == cur.follow && !cur.dash) s += 3000; /* steady */
                if (d == -2) s += 1500;
                if (sst_bot_debug) fprintf(stderr, "plan d%d f%d: %lld\n", d, fol, s);
                if (s > best) { best = s; bp = pl; }
            }
        if (p->dash_cd == 0)
            for (int d = 0; d < 8; d++) {
                Plan pl = {d, 1, 1};
                long long s = try_plan(pl, aim, H) - 20000;
                if (s > best) { best = s; bp = pl; }
            }
        cur = bp;
        plan_t = 5;
        uint32_t m = dir_mask(cur.dir == -2 ? follow_dir() : cur.dir);
        if (cur.dash) m |= BTN_B;
        cur.dash = 0;
        plan_t--;
        return fire_buttons(tog, m, aim);
    }
    plan_t--;
    int d = cur.dir == -2 ? follow_dir() : cur.dir;
    return fire_buttons(tog, dir_mask(d), aim);
}

/* ---- the Hop ----------------------------------------------------------------------------- */

static bool hop_bad(int row, int lane, int frames_ahead) {
    if (lane < 0 || lane >= HOP_LANES) return true;
    int t = sst_hop_tile(row, lane);
    if (t == HT_HOLE) return true;
    int keep_f = sst_g.frame_t;
    sst_g.frame_t += frames_ahead;
    bool bad = sst_hop_deadly(row, lane);
    sst_g.frame_t = keep_f;
    return bad;
}

static uint32_t hop_buttons(void) {
    const SstHop *h = &sst_g.hop;
    if (h->state != HB_HOP && h->state != HB_READY) return 0;
    /* the next landing after this one: where it can safely be */
    int left = h->state == HB_HOP ? HOP_HOP_T - h->hop_t : 40 - h->st;
    int t1 = left + HOP_HOP_T, t2 = t1 + HOP_HOP_T;
    int r = h->row, l = h->lane;
    if (h->state == HB_READY) { t1 = left; t2 = left + HOP_HOP_T; }
    static const int ORDER[4] = {0, 1, 3, 2};
    int best = -1, bscore = -1000;
    for (int k = 0; k < 4; k++) {
        int d = ORDER[k];
        int nr = r + (d == 0) - (d == 2), nl = l + (d == 1) - (d == 3);
        if (hop_bad(nr, nl, t1 + 1)) continue;
        int sc = 10 - k * 2 + sst_hop_coin(nr, nl) * 3;
        /* and somewhere to go from there */
        int outs = 0;
        for (int e = 0; e < 4; e++) {
            int rr = nr + (e == 0) - (e == 2), ll = nl + (e == 1) - (e == 3);
            if (!hop_bad(rr, ll, t2 + 1)) outs++;
        }
        if (!outs) sc -= 20;
        if (sc > bscore) { bscore = sc; best = d; }
    }
    if (best < 0) best = 0;
    if (h->next_dir == best) return 0;
    if (tog % 2) return 0;
    static const uint32_t KEY[4] = {BTN_UP, BTN_RIGHT, BTN_DOWN, BTN_LEFT};
    return KEY[best];
}

/* ---- every screen ------------------------------------------------------------------------- */

int sst_bot_buttons(void) {
    tog++;
    uint32_t tap = (tog / 4) % 2 ? BTN_A : 0;
    switch (sst_g.state) {
    case SS_TITLE:
        if (sst_g.menu != 0) return (tog / 4) % 2 ? BTN_UP : 0;
        return tap;
    case SS_OFFER:
        if (sst_g.offer_sel != 0) return (tog / 4) % 2 ? BTN_LEFT : 0;
        return tap;
    case SS_CONTINUE:
        if (sst_g.cont_sel != 0) return (tog / 4) % 2 ? BTN_LEFT : 0;
        return tap;
    case SS_PLAY: return (int)play_buttons();
    case SS_HOP: return (int)hop_buttons();
    default: return tap;
    }
}
