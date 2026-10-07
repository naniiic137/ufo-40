/* DUKES UP - the demo player, for the tests. It only ever answers with
 * buttons. On the street it plays the way the cherry guides teach: walk on
 * slowly so ghouls come a few at a time, stand a little below the one you
 * fight, four punches and a breath, spin when they get round both sides,
 * stay out of every lane something is coming down; in the shop it buys a
 * heal when it needs one, then POWER, TOUGH and RECOV. Integer arithmetic
 * only, so it plays the same everywhere. */
#include "dku.h"

int dku_bot_plan; /* 0: the story, 1: the gym */

static int me_x, me_y; /* px */
static int tolerate; /* ground danger we accept to reach a spot */
static int exempt = -1; /* the ghoul we mean to fight level with (pinned to the front edge) */

static bool alive_foe(const Actor *o) {
    return o->alive && o->team == 1 && o->state != AS_DEAD && o->state != AS_FALL && o->state != AS_GONE;
}

static bool awake_foe(const Actor *o) {
    if (!alive_foe(o) || o->state == AS_DORMANT) return false;
    if (o->state == AS_RISE && o->from == FROM_ROOF) return false;
    if (o->kind == AK_UNDERTOW) return false;
    return true;
}

/* frames until a lobbed shot lands, and where */
static void landing(const Shot *s, int *lx, int *ly) {
    int z = s->z, vz = s->vz, x = s->x, y = s->y;
    for (int t = 0; t < 120 && z > 0; t++) {
        x += s->vx;
        y += s->vy;
        z += vz;
        vz -= 4;
    }
    *lx = dku_px(x);
    *ly = dku_px(y);
}

/* how bad it would be to stand at (x, y) */
static int danger_at(int x, int y, bool far) {
    int d = 0;
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        const Hazard *h = &dku_g.hz[i];
        if (!h->alive) continue;
        switch (h->kind) {
        case HZ_PIT: case HZ_WATER:
            if (x >= h->x - 10 && x < h->x + h->w + 10 && y >= h->y - 8 && y < h->y + h->h + 8) d += 2000;
            /* a knock-back from here could end in it */
            else if (!far && x >= h->x - 36 && x < h->x + h->w + 36 && y >= h->y - 4 && y < h->y + h->h + 4) d += 250;
            break;
        case HZ_MINE:
            if (iabs(x - (h->x + 5)) < 14 && iabs(y - (h->y + 3)) < 9) d += 900;
            break;
        case HZ_LAMP: {
            int dx = x - h->x, dy = y - h->y;
            if (iabs(dx) < h->w / 2 + 8 && iabs(dy) < h->h / 2 + 6) d += h->arg == 2 ? 1500 : 700;
            break;
        }
        case HZ_FIRE: case HZ_FIREWALL:
            if (x >= h->x - 8 && x < h->x + h->w + 8 && y >= h->y - 5 && y < h->y + h->h + 5) d += 600;
            break;
        case HZ_CLOUD:
            if (iabs(x - h->x) <= h->w + 6 && iabs(y - h->y) * 2 <= h->w + 8) d += 500;
            break;
        case HZ_CAR:
            if (iabs(y - h->y) < 15 && x > h->x - 40) d += 1500;
            break;
        case HZ_THRESHER:
            if (h->x < h->arg - h->w ? x < h->x + h->w + 30 : x < h->x + h->w + 6) d += 1500 + (h->x + h->w + 30 - x) * 20;
            break;
        default: break;
        }
    }
    for (int i = 0; i < DKU_MAX_SHOTS; i++) {
        const Shot *s = &dku_g.sh[i];
        if (!s->alive || s->team != 1) continue;
        if (s->kind == SH_CLEAVER || s->kind == SH_RAY) {
            int sx = dku_px(s->x), sy = dku_px(s->y);
            bool coming = (s->vx > 0 && sx < x + 8) || (s->vx < 0 && sx > x - 8);
            if (coming && iabs(sy - y) < 10 && iabs(sx - x) < 120) d += 800;
        } else {
            int lx, ly;
            landing(s, &lx, &ly);
            int r = s->kind == SH_GAS ? 36 : 32;
            if (iabs(lx - x) < r && iabs(ly - y) * 2 < r) d += 900;
        }
    }
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        const Actor *o = &dku_g.a[i];
        if (!alive_foe(o)) continue;
        int ox = dku_px(o->x), oy = dku_px(o->y);
        int dx = x - ox, dy = y - oy;
        /* the rammer's lane, the howler's spring */
        if ((o->kind == AK_RAMMER && (o->state == AS_WINDUP || o->state == AS_RUSH)) ||
            (o->kind == AK_HOWLER && (o->state == AS_WINDUP || o->state == AS_RUSH)) ||
            (o->kind == AK_GRIST && (o->state == AS_RUSH || o->state == AS_SLAM || (o->state == AS_WINDUP && o->atk == AT_E_ELBOW)))) {
            if (iabs(dy) < 16 && (dx * o->face > -10)) d += 1200;
        }
        if ((o->kind == AK_TUSKER || o->kind == AK_GRIST) && o->state == AS_SLAM) {
            if (iabs(x - dku_px(o->tx)) < 34 && iabs(y - dku_px(o->ty)) < 16) d += 1200;
        }
        /* a thrower winding up: its lane */
        if ((o->kind == AK_CROW || o->kind == AK_VISITOR) && o->state == AS_WINDUP && o->atk == AT_NONE &&
            iabs(dy) < 11 && dx * o->face > 0)
            d += 700;
        if (o->kind == AK_TORCH && o->state == AS_WINDUP && iabs(dx) < 34 && iabs(dy) < 18) d += 500;
        if (o->kind == AK_TORCH && o->exploding && o->state == AS_DEAD) {
            if (iabs(dx) < 40 && iabs(dy) < 20) d += 1500;
        }
        if (o->state == AS_DORMANT || o->kind == AK_UNDERTOW) continue;
        /* in reach of its fists: being a little below is safe */
        int reach = DKU_KINDS[o->kind].reach + 6;
        if (!far && i != exempt && reach > 6 && iabs(dx) < reach + DKU_KINDS[o->kind].hw && dy >= -DKU_REACH_UP - 2 && dy <= DKU_REACH_DOWN + 1) {
            d += o->state == AS_WINDUP ? 300 : 0;
        }
    }
    return d;
}

static int danger(int x, int y) { return danger_at(x, y, false); }

/* the floor's own dangers only (holes, mines, fire, lamps, clouds) */
static int ground_danger(int x, int y) {
    int d = 0;
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        const Hazard *h = &dku_g.hz[i];
        if (!h->alive) continue;
        switch (h->kind) {
        case HZ_PIT: case HZ_WATER:
            if (x >= h->x - 10 && x < h->x + h->w + 10 && y >= h->y - 8 && y < h->y + h->h + 8) d += 2000;
            break;
        case HZ_MINE:
            if (iabs(x - (h->x + 5)) < 14 && iabs(y - (h->y + 3)) < 9) d += 900;
            break;
        case HZ_LAMP:
            if (iabs(x - h->x) < h->w / 2 + 8 && iabs(y - h->y) < h->h / 2 + 6) d += 700;
            break;
        case HZ_FIRE: case HZ_FIREWALL:
            if (x >= h->x - 8 && x < h->x + h->w + 8 && y >= h->y - 5 && y < h->y + h->h + 5) d += 600;
            break;
        case HZ_CLOUD:
            if (iabs(x - h->x) <= h->w + 6 && iabs(y - h->y) * 2 <= h->w + 8) d += 500;
            break;
        default: break;
        }
    }
    return d;
}

static int path_cost(int x0, int x1, int y) {
    int c = 0, dir = x1 >= x0 ? 1 : -1;
    for (int x = x0; dir > 0 ? x <= x1 : x >= x1; x += dir * 6) c += ground_danger(x, y);
    return c;
}

static int greedy(int tx, int ty);

/* a way round the floor's dangers: along the cleanest lane */
static int step_toward(int tx, int ty) {
    ty = iclamp(ty, dku_floor_lo(), dku_floor_hi());
    int lo = dku_floor_lo(), hi = dku_floor_hi();
    /* is the straight way clear? */
    bool clear = true;
    int n = imax(iabs(tx - me_x) / 6, iabs(ty - me_y) / 6) + 1;
    for (int k = 1; k <= n && clear; k++) {
        int x = me_x + (tx - me_x) * k / n, y = me_y + (ty - me_y) * k / n;
        if (ground_danger(x, y) >= 500) clear = false;
    }
    if (clear || ground_danger(tx, ty) >= 500) return greedy(tx, ty);
    int best = me_y, bc = 1 << 30;
    for (int y = lo + 2; y <= hi; y += 6) {
        int c = path_cost(me_x, tx, y) + path_cost(me_x, me_x, y) + iabs(y - me_y) * 3 + iabs(y - ty) * 3;
        int vcost = 0;
        int a = imin(y, me_y), b = imax(y, me_y);
        for (int yy = a; yy <= b; yy += 4) vcost += ground_danger(me_x, yy);
        c += vcost;
        if (c < bc) { bc = c; best = y; }
    }
    if (iabs(best - me_y) > 3) return greedy(me_x, best);
    /* along the lane until past the trouble, then in */
    int vcost = 0;
    int a = imin(best, ty), b = imax(best, ty);
    for (int yy = a; yy <= b; yy += 4) vcost += ground_danger(tx, yy);
    if (vcost < 500) return greedy(tx, best);
    return greedy(tx, ty);
}

static int greedy(int tx, int ty) {
    int best = 0, bs = 1 << 30;
    bool near_goal = iabs(tx - me_x) + iabs(ty - me_y) < 20;
    for (int dir = 0; dir < 9; dir++) {
        int hx = dir % 3 - 1, vy = dir / 3 - 1;
        int nx = me_x + hx * 5, ny = iclamp(me_y + vy * 4, dku_floor_lo(), dku_floor_hi());
        int fx = me_x + hx * 16, fy = iclamp(me_y + vy * 14, dku_floor_lo(), dku_floor_hi());
        int dist = iabs(me_x + hx - tx) + iabs(iclamp(me_y + vy, dku_floor_lo(), dku_floor_hi()) - ty) * 2;
        int dn = danger(nx, ny), df = near_goal ? 0 : danger_at(fx, fy, true);
        dn = dn > tolerate ? dn - tolerate : 0;
        df = df > tolerate ? df - tolerate : 0;
        int s = dist * 4 + dn + df / 2;
        if (hx == 0 && vy == 0) s -= 2;
        if (s < bs) { bs = s; best = dir; }
    }
    int hx = best % 3 - 1, vy = best / 3 - 1;
    int m = 0;
    if (hx > 0) m |= BTN_RIGHT;
    if (hx < 0) m |= BTN_LEFT;
    if (vy > 0) m |= BTN_DOWN;
    if (vy < 0) m |= BTN_UP;
    return m;
}

static int pick_target(const Actor *me) {
    int best = -1, bd = 1 << 30;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        const Actor *o = &dku_g.a[i];
        if (!awake_foe(o)) continue;
        if (o->kind == AK_SAUCER) continue;
        int ox = dku_px(o->x);
        if (ox < dku_view_left() - 4 || ox > dku_view_right() + 4) continue;
        int d = iabs(ox - dku_px(me->x)) + iabs(dku_px(o->y) - dku_px(me->y)) * 2;
        if (o->state == AS_DOWN || o->state == AS_BLINK) d += 60;
        if (o->kind == AK_TORCH) d -= 20;
        if (o->kind == AK_FEELER) d += 80;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

static int nearest_dormant(void) {
    int best = -1, bd = 1 << 30;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        const Actor *o = &dku_g.a[i];
        if (!alive_foe(o) || awake_foe(o) || o->kind == AK_UNDERTOW) continue;
        if (o->state == AS_RISE) continue;
        int d = iabs(dku_px(o->x) - me_x);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

static int press(int b) { return (dku_g.bot_t & 1) ? 0 : b; }

/* something coming down my lane right now? */
static int lane_threat(void) {
    int worst = 0;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        const Actor *o = &dku_g.a[i];
        if (!alive_foe(o)) continue;
        int dx = me_x - dku_px(o->x), dy = me_y - dku_px(o->y);
        bool ahead = dx * o->face > -6;
        if (iabs(dy) >= 13 || !ahead) continue;
        if (o->kind == AK_RAMMER && ((o->state == AS_WINDUP && o->atk == AT_E_RUSH && o->st > 6) || o->state == AS_RUSH)) worst = imax(worst, 3);
        if (o->kind == AK_HOWLER && o->state == AS_WINDUP && o->atk == AT_E_POUNCE && iabs(dx) < 100) worst = imax(worst, 2);
        if (o->kind == AK_GRIST && o->state == AS_WINDUP && o->atk == AT_E_ELBOW && iabs(dx) < 120) worst = imax(worst, 2);
    }
    for (int i = 0; i < DKU_MAX_SHOTS; i++) {
        const Shot *s = &dku_g.sh[i];
        if (!s->alive || s->team != 1 || (s->kind != SH_CLEAVER && s->kind != SH_RAY)) continue;
        int sx = dku_px(s->x), sy = dku_px(s->y);
        bool coming = (s->vx > 0 && sx < me_x) || (s->vx < 0 && sx > me_x);
        if (coming && iabs(sy - me_y) < 9 && iabs(sx - me_x) < 70) worst = imax(worst, 1);
    }
    for (int i = 0; i < DKU_MAX_HAZARDS; i++)
        if (dku_g.hz[i].alive && dku_g.hz[i].kind == HZ_CAR && iabs(dku_g.hz[i].y - me_y) < 15) worst = imax(worst, 3);
    return worst;
}

static int dodge_phase, dodge_btn, run_phase;

/* a long way to go: break into a run (tap, let go, hold) */
static int maybe_run(const Actor *me, int m, int tx) {
    int hd = m & (BTN_LEFT | BTN_RIGHT);
    if (me->running) return m;
    if (!hd || iabs(tx - me_x) < 70 || me->state != AS_FREE || me->carry >= 0) { run_phase = 0; return m; }
    run_phase++;
    if (run_phase == 2) return m & ~hd;
    if (run_phase >= 3) run_phase = 0;
    return m;
}

/* things about to go off where I stand */
static int blast_threat(int x, int y) {
    int d = 0;
    for (int i = 0; i < DKU_MAX_SHOTS; i++) {
        const Shot *s = &dku_g.sh[i];
        if (!s->alive || s->team != 1) continue;
        if (s->kind != SH_BOTTLE && s->kind != SH_BOMB && s->kind != SH_GAS && s->kind != SH_SPIT) continue;
        int lx, ly;
        landing(s, &lx, &ly);
        int r = s->kind == SH_GAS ? 36 : 30;
        if (iabs(lx - x) < r && iabs(ly - y) * 2 < r) d += 900;
    }
    for (int i = 2; i < DKU_MAX_ACTORS; i++) {
        const Actor *o = &dku_g.a[i];
        if (o->alive && (o->kind == AK_GRIST || o->kind == AK_TUSKER) && o->state == AS_SLAM &&
            iabs(dku_px(o->x) - x) < 40 && iabs(dku_px(o->y) - y) < 16)
            d += 1200;
        if (o->alive && o->kind == AK_TORCH && o->exploding && o->state == AS_DEAD &&
            iabs(dku_px(o->x) - x) < 40 && iabs(dku_px(o->y) - y) < 20)
            d += 1500;
    }
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        const Hazard *h = &dku_g.hz[i];
        if (h->alive && h->kind == HZ_LAMP && (h->t > 0 || h->arg == 2) && iabs(x - h->x) < h->w / 2 + 8 &&
            iabs(y - h->y) < h->h / 2 + 6)
            d += 1500;
        if (h->alive && (h->kind == HZ_CLOUD || h->kind == HZ_FIRE) && iabs(x - (h->kind == HZ_CLOUD ? h->x : h->x + h->w / 2)) <= (h->kind == HZ_CLOUD ? h->w + 4 : h->w / 2 + 4) &&
            iabs(y - (h->kind == HZ_CLOUD ? h->y : h->y + h->h / 2)) <= (h->kind == HZ_CLOUD ? h->w / 2 + 4 : h->h / 2 + 4))
            d += 600;
    }
    return d;
}

static int greedy(int tx, int ty);

/* get out from under it */
static int escape(void) {
    int bx = me_x, by = me_y, bs = 1 << 30;
    for (int ddx = -30; ddx <= 30; ddx += 10)
        for (int ddy = -21; ddy <= 21; ddy += 7) {
            int x = me_x + ddx, y = iclamp(me_y + ddy, dku_floor_lo(), dku_floor_hi());
            if (x < dku_view_left() + 7 || x > dku_view_right() - 7) continue;
            int s = blast_threat(x, y) * 4 + danger(x, y) + iabs(ddx) + iabs(ddy) * 2;
            if (ground_danger(x, y) >= 2000) s += 20000;
            if (s < bs) { bs = s; bx = x; by = y; }
        }
    return greedy(bx, by);
}

/* the shop: a heal when hurt, then POWER, TOUGH, RECOV */
static int shop_buttons(void) {
    const Actor *a = &dku_g.a[dku_g.shop_who];
    int want = 4;
    if (a->hp < 72 && dku_g.cash >= DKU_PRICE_HEAL) want = 0;
    else if (dku_g.cash >= DKU_PRICE_STAT + (a->hp < 90 ? DKU_PRICE_HEAL : 0)) {
        const uint8_t *st = dku_g.pr[dku_g.shop_who].stat;
        if (st[DK_POWER] < 3) want = 1;
        else if (st[DK_TOUGH] < 3) want = 3;
        else if (st[DK_RECOV] < 3) want = 2;
    } else if (a->hp < 95 && dku_g.cash >= DKU_PRICE_HEAL) want = 0;
    if (dku_g.shop_sel != want) return press(dku_g.shop_sel < want ? BTN_DOWN : BTN_UP);
    return press(BTN_A);
}

int dku_bot_why, dku_bot_target;
#define WHY(n, v) do { dku_bot_why = (n); return (v); } while (0)

static int fight(void) {
    Actor *me = &dku_g.a[0];
    me_x = dku_px(me->x);
    me_y = dku_px(me->y);
    if (!me->alive || me->state == AS_DEAD || me->state == AS_GONE) WHY(1, 0);
    const DkuSection *sec = dku_g.gym ? &DKU_GYM : &DKU_NIGHT[dku_g.night].sec[dku_g.sec];

    /* the gym: walk left as night 1 begins */
    if (dku_bot_plan == 1 && !dku_g.gym && dku_g.night == 0 && dku_g.sec == 0 && dku_g.cam == 0) WHY(2, BTN_LEFT);

    if (me->state == AS_DOWN) {
        /* roll away from the nearest ghoul */
        int t = pick_target(me);
        if (t < 0) WHY(3, 0);
        int ox = dku_px(dku_g.a[t].x);
        WHY(4, step_toward(me_x + (ox > me_x ? -30 : 30), me_y));
    }
    if (me->state == AS_HURT || me->state == AS_SUPLEX || me->state == AS_DODGE) { dodge_phase = 0; WHY(5, 0); }
    /* a dodge in progress: tap, let go, tap */
    if (dodge_phase > 0) {
        int b = dodge_phase == 2 ? 0 : dodge_btn;
        if (++dodge_phase > 3) dodge_phase = 0;
        WHY(6, b);
    }
    if (me->state == AS_FREE && me->land_t == 0 && lane_threat() > 0) {
        int up = 0, down = 0;
        for (int k = 8; k <= 26; k += 6) { up += danger(me_x, me_y - k); down += danger(me_x, me_y + k); }
        if (me_y - 24 < dku_floor_lo()) up += 5000;
        if (me_y + 24 > dku_floor_hi()) down += 5000;
        for (int k = 2; k <= 28; k += 2) {
            if (ground_danger(me_x, me_y - k) >= 2000) up += 20000;
            if (ground_danger(me_x, me_y + k) >= 2000) down += 20000;
        }
        if (imin(up, down) < 20000) {
            dodge_btn = up < down ? BTN_UP : BTN_DOWN;
            dodge_phase = 2;
            WHY(7, dodge_btn);
        }
    }

    if (me->carry >= 0) {
        /* whatever we picked up, throw it at somebody */
        int t = pick_target(me);
        int dir = t >= 0 ? (dku_g.a[t].x > me->x ? 1 : -1) : 1;
        if (me->face != dir) WHY(8, dir > 0 ? BTN_RIGHT : BTN_LEFT);
        if (me->state == AS_FREE) WHY(9, press(BTN_B));
        WHY(10, 0);
    }

    if (me->state == AS_GRAB) {
        /* the Undertow: carry it to the edge and throw it in */
        int boss = dku_g.boss;
        if (boss >= 0 && dku_g.a[boss].kind == AK_UNDERTOW) {
            for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
                const Hazard *h = &dku_g.hz[i];
                if (!h->alive || h->kind != HZ_LAMP || iabs(me_x - h->x) >= h->w / 2 + 8 || iabs(me_y - h->y) >= h->h / 2 + 6) continue;
                /* a feeler coming down: carry it out from under, up or down the jetty */
                int vy = me_y >= h->y ? BTN_DOWN : BTN_UP;
                if (vy == BTN_DOWN && me_y > dku_floor_hi() - 6) vy = BTN_UP;
                if (vy == BTN_UP && ground_danger(me_x, me_y - 10) >= 2000) vy = BTN_DOWN;
                if (h->t < 8 && me_x >= 150) WHY(47, press(BTN_RIGHT | BTN_A) | BTN_RIGHT);
                WHY(52, vy | (me_x < 188 ? BTN_RIGHT : 0));
            }
            if (me_x < 188) WHY(11, BTN_RIGHT | (me_y < 140 ? BTN_DOWN : 0));
            WHY(12, press(BTN_RIGHT | BTN_A) | BTN_RIGHT);
        }
        if (me->jab_t >= 12) WHY(13, press(BTN_A));
        WHY(14, 0);
    }

    exempt = -1;
    tolerate = 0;
    /* a cleaver or a ray about to land: spin it back */
    if (me->state == AS_ATTACK || me->state == AS_FREE || me->state == AS_HURT) {
        for (int i = 0; i < DKU_MAX_SHOTS; i++) {
            const Shot *s = &dku_g.sh[i];
            if (!s->alive || s->team != 1 || (s->kind != SH_CLEAVER && s->kind != SH_RAY)) continue;
            int sx = dku_px(s->x), sy = dku_px(s->y);
            bool coming = (s->vx > 0 && sx < me_x) || (s->vx < 0 && sx > me_x);
            int lead = s->kind == SH_RAY ? 16 : 26;
            if (coming && iabs(sy - me_y) < 8 && iabs(sx - me_x) < lead && (me->state != AS_FREE || dodge_phase == 0))
                WHY(15, (dku_g.bot_t & 1) ? 0 : (BTN_A | BTN_B));
        }
        /* a firebottle or a bomb about to come down on me: spin it away (free) */
        for (int i = 0; i < DKU_MAX_SHOTS; i++) {
            const Shot *s = &dku_g.sh[i];
            if (!s->alive || s->team != 1 || (s->kind != SH_BOTTLE && s->kind != SH_BOMB)) continue;
            int lx, ly;
            landing(s, &lx, &ly);
            int sx = dku_px(s->x), sz = dku_px(s->z);
            if (iabs(lx - me_x) < 22 && iabs(ly - me_y) < 10 && iabs(sx - me_x) < 24 && sz < 26 && s->vz < 0)
                WHY(58, (dku_g.bot_t & 1) ? 0 : (BTN_A | BTN_B));
        }
    }
    /* a pounce or a charge about to land and nowhere to go: spin into it */
    if (me->state == AS_ATTACK || me->state == AS_FREE || me->state == AS_HURT) {
        for (int i = 2; i < DKU_MAX_ACTORS; i++) {
            const Actor *o = &dku_g.a[i];
            if (!alive_foe(o) || o->state != AS_RUSH || o->atk == AT_NONE) continue;
            int dx = me_x - dku_px(o->x), dy = me_y - dku_px(o->y);
            if (iabs(dy) < 12 && dx * o->face > 0 && dx * o->face < 34)
                WHY(40, (dku_g.bot_t & 1) ? 0 : (BTN_A | BTN_B));
        }
    }
    /* the runaway thresher: keep well ahead of it */
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) {
        const Hazard *h = &dku_g.hz[i];
        if (h->alive && h->kind == HZ_THRESHER && me_x < h->x + h->w + 70 && h->x < h->arg - h->w)
            WHY(16, greedy(me_x + 30, me_y));
    }
    if ((me->state == AS_FREE || me->state == AS_ATTACK) && blast_threat(me_x, me_y) > 0) {
        int m = escape();
        if (m) WHY(17, m);
    }
    int t = pick_target(me);
    /* hurt, and food close by with nobody near it: go and eat */
    if (me->hp < 70 && me->state == AS_FREE) {
        int best = -1, bd = 1 << 30;
        for (int i = 0; i < DKU_MAX_ITEMS; i++) {
            const Item *it = &dku_g.it[i];
            if (it->alive != 1 || DKU_ITEMS[it->kind].kind != IK_FOOD) continue;
            int ix = dku_px(it->x), iy = dku_px(it->y);
            int d = iabs(ix - me_x) + iabs(iy - me_y);
            if (d > 70 || danger(ix, iy) >= 300) continue;
            bool crowded = false;
            for (int k = 2; k < DKU_MAX_ACTORS; k++) {
                const Actor *o = &dku_g.a[k];
                if (awake_foe(o) && o->state != AS_HURT && o->state != AS_DOWN && iabs(dku_px(o->x) - ix) < 34 && iabs(dku_px(o->y) - iy) < 16) crowded = true;
            }
            if (!crowded && d < bd) { bd = d; best = i; }
        }
        if (best >= 0) {
            const Item *it = &dku_g.it[best];
            int ix = dku_px(it->x), iy = dku_px(it->y);
            if (iabs(ix - me_x) <= 8 && iabs(iy - me_y) <= 5) WHY(45, press(BTN_A));
            WHY(46, step_toward(ix, iy));
        }
        /* or a container with food in it (the penthouse's bin) */
        if (me->hp < 55) {
            for (int i = 0; i < DKU_MAX_PROPS; i++) {
                const Prop *p = &dku_g.pr_[i];
                if (!p->alive || (p->content == 0 || DKU_ITEMS[p->content].kind != IK_FOOD)) continue;
                int px = dku_px(p->x), py = dku_px(p->y);
                if (px < dku_view_left() + 8 || px > dku_view_right() - 8) continue;
                int side = me_x <= px ? -1 : 1;
                int dxp = px - me_x, dyp = py - me_y;
                int fp = dxp > 0 ? 1 : -1;
                if (iabs(dxp) >= (me->face == fp ? 2 : 4) && iabs(dxp) <= 19 && dyp >= -9 && dyp <= 1) {
                    if (me->face != fp) WHY(53, fp > 0 ? BTN_RIGHT : BTN_LEFT);
                    if (p->shake == 0) WHY(54, press(BTN_A));
                    WHY(55, 0);
                }
                WHY(56, step_toward(px + side * 14, py + 4));
            }
        }
    }
    /* surrounded: spin */
    if (t >= 0 && me->hp > 24 && (me->state == AS_FREE || me->state == AS_ATTACK)) {
        int front = 0, back = 0;
        for (int i = 2; i < DKU_MAX_ACTORS; i++) {
            const Actor *o = &dku_g.a[i];
            if (!awake_foe(o) || o->state == AS_DOWN || o->state == AS_HURT || o->kind == AK_FEELER) continue;
            int dx = dku_px(o->x) - me_x, dy = dku_px(o->y) - me_y;
            if (iabs(dx) > 24 || dy < -10 || dy > 4) continue;
            if (dx * me->face >= 0) front++;
            /* only one about to strike from behind is worth the health */
            else if (o->state == AS_WINDUP || o->state == AS_ATTACK || (o->state == AS_FREE && o->cool < 8)) back++;
        }
        if (back > 0 && (front > 0 || back > 1) && (dku_g.bot_t & 1) == 0) WHY(18, BTN_A | BTN_B);
    }

    if (t < 0) {
        /* nobody awake: the boss fight's Undertow wants bodies */
        if (dku_g.boss >= 0 && dku_g.a[dku_g.boss].kind == AK_UNDERTOW && dku_g.a[dku_g.boss].alive) WHY(19, step_toward(150, 150));

        /* a saucer low over the street: $10 inside */
        for (int i = 2; i < DKU_MAX_ACTORS; i++) {
            const Actor *o = &dku_g.a[i];
            if (!o->alive || o->kind != AK_SAUCER || o->state == AS_DEAD) continue;
            int ox = dku_px(o->x), oy = dku_px(o->y);
            if (ox < dku_view_left() + 30) continue;
            int dx = ox - me_x;
            if (dku_px(o->z) <= 14 && dx >= 4 && dx <= 26 && iabs(oy - me_y) <= 4) {
                if (me->face != 1) WHY(48, BTN_RIGHT);
                if (me->state == AS_FREE) WHY(49, press(BTN_A));
                WHY(50, 0);
            }
            /* wait in its path, a little ahead of it */
            int wx = imax(dku_view_left() + 12, imin(ox - 30, dku_view_left() + 150));
            if (dx < 0) continue;
            WHY(51, step_toward(wx, oy));
        }
        /* the gym's passers-by: a punch shakes something loose */
        for (int i = 2; i < DKU_MAX_ACTORS; i++) {
            const Actor *o = &dku_g.a[i];
            if (!o->alive || o->kind != AK_PASSER || o->mode != 0) continue;
            int ox = dku_px(o->x), oy = dku_px(o->y);
            if (ox < dku_view_left() + 10 || ox > dku_view_right() - 10) continue;
            int dx = ox - me_x, dy = oy - me_y;
            if (iabs(dx) <= 20 && iabs(dx) >= 3 && dy >= -9 && dy <= 1) {
                int f = dx > 0 ? 1 : -1;
                if (me->face != f) WHY(41, f > 0 ? BTN_RIGHT : BTN_LEFT);
                if (me->state == AS_FREE) WHY(42, press(BTN_A));
                WHY(43, 0);
            }
            /* meet it head on: it runs left */
            WHY(44, step_toward(ox - 16, oy + 4));
        }
        /* food and cash first */
        int best = -1, bd = 1 << 30;
        for (int i = 0; i < DKU_MAX_ITEMS; i++) {
            const Item *it = &dku_g.it[i];
            if (it->alive != 1) continue;
            int k = DKU_ITEMS[it->kind].kind;
            if (k != IK_FOOD && k != IK_CASH) continue;
            int ix = dku_px(it->x);
            if (ix < dku_view_left() || ix > dku_view_right()) continue;
            if (danger(ix, dku_px(it->y)) >= 900) continue;
            int d = iabs(ix - me_x) + iabs(dku_px(it->y) - me_y);
            if (d < bd) { bd = d; best = i; }
        }
        if (best >= 0) {
            const Item *it = &dku_g.it[best];
            int ix = dku_px(it->x), iy = dku_px(it->y);
            if (iabs(ix - me_x) <= 8 && iabs(iy - me_y) <= 5 && me->state == AS_FREE) WHY(20, press(BTN_A));
            WHY(21, step_toward(ix, iy));
        }
        /* things to break */
        int bp = -1;
        bd = 1 << 30;
        for (int i = 0; i < DKU_MAX_PROPS; i++) {
            const Prop *p = &dku_g.pr_[i];
            if (!p->alive) continue;
            int px = dku_px(p->x);
            if (px < dku_view_left() + 8 || px > dku_view_right() - 8) continue;
            if (danger(px - 16, dku_px(p->y) + 4) >= 900 && danger(px + 16, dku_px(p->y) + 4) >= 900) continue;
            int d = iabs(px - me_x);
            if (d < bd) { bd = d; bp = i; }
        }
        if (bp >= 0) {
            const Prop *p = &dku_g.pr_[bp];
            int px = dku_px(p->x), py = dku_px(p->y);
            int side = me_x <= px ? -1 : 1;
            int sx = px + side * 14, sy = py + 4;
            int dxp = px - me_x, dyp = py - me_y;
            int fp = dxp > 0 ? 1 : -1;
            if (iabs(dxp) >= (me->face == fp ? 2 : 4) && iabs(dxp) <= 19 && dyp >= -9 && dyp <= 1) {
                int f = fp;
                if (me->face != f) WHY(22, f > 0 ? BTN_RIGHT : BTN_LEFT);
                if (me->state == AS_FREE && p->shake == 0) WHY(23, press(BTN_A));
                WHY(24, 0);
            }
            WHY(25, step_toward(sx, sy));
        }
        /* dormant ghouls in the way: go and wake them, carefully */
        int dm = nearest_dormant();
        if (dm >= 0 && iabs(dku_px(dku_g.a[dm].x) - me_x) < 200) {
            const Actor *o = &dku_g.a[dm];
            int ox = dku_px(o->x);
            WHY(26, step_toward(ox + (me_x < ox ? -36 : 36), imin(dku_px(o->y) + 7, dku_floor_hi())));
        }
        /* on we go */
        if (sec->kind == SEC_LIFT || sec->kind == SEC_BOSS || sec->kind == SEC_GYM) WHY(27, step_toward(160, 150));
        int beam = -1;
        for (int i = 0; i < DKU_MAX_HAZARDS; i++) if (dku_g.hz[i].alive && dku_g.hz[i].kind == HZ_BEAM) beam = i;
        if (dku_g.exit_t > 0 && beam >= 0) WHY(28, step_toward(dku_g.hz[beam].x + dku_g.hz[beam].w / 2, 140));
        /* the hotel's lift opens early: take it, don't go looking for the pair beyond */
        for (int i = 0; i < DKU_MAX_HAZARDS; i++)
            if (dku_g.exit_t > 0 && dku_g.hz[i].alive && dku_g.hz[i].kind == HZ_DOOR && dku_g.hz[i].arg == 1)
                WHY(57, step_toward(dku_g.hz[i].x + dku_g.hz[i].w / 2, 140));
        /* walk on slowly: a step at a time so ghouls come a few at once */
        int lane = 146, lc = 1 << 30;
        for (int y = dku_floor_lo() + 4; y <= dku_floor_hi() - 2; y += 4) {
            int c = (ground_danger(me_x + 40, y) + ground_danger(me_x + 20, y)) * 10 + iabs(y - 146) + iabs(y - me_y) / 2;
            if (c < lc) { lc = c; lane = y; }
        }
        if (dku_g.cam_lock >= 0 && dku_g.cam >= dku_g.cam_lock) WHY(29, step_toward(me_x, lane));
        bool chase = false;
        for (int i = 0; i < DKU_MAX_HAZARDS; i++) if (dku_g.hz[i].alive && dku_g.hz[i].kind == HZ_THRESHER && dku_g.hz[i].x < dku_g.hz[i].arg - dku_g.hz[i].w) chase = true;
        if (!chase && (dku_g.bot_t % 8) >= 5 && dku_g.exit_t == 0) WHY(30, step_toward(me_x, lane));
        WHY(31, step_toward(me_x + 40, lane));
    }

    Actor *o = &dku_g.a[t];
    dku_bot_target = t;
    bool undertow = dku_g.boss >= 0 && dku_g.a[dku_g.boss].kind == AK_UNDERTOW && dku_g.a[dku_g.boss].alive;
    int ox = dku_px(o->x), oy = dku_px(o->y);
    int side = ox >= me_x ? 1 : -1;      /* where it is */
    int want_x = ox - side * 18, want_y = imin(oy + 7, dku_floor_hi());
    exempt = oy + 5 > dku_floor_hi() ? t : -1;
    {
        /* the best spot to punch it from: below it, on whichever side is safer */
        int bs = 1 << 30;
        for (int sd = -1; sd <= 1; sd += 2)
            for (int ddx = 12; ddx <= 20; ddx += 4)
                for (int ddy = 3; ddy <= 8; ddy += 5) {
                    int x = ox + sd * ddx, y = imin(oy + ddy, dku_floor_hi());
                    if (x < dku_view_left() + 7 || x > dku_view_right() - 7) continue;
                    int s = ground_danger(x, y) * 4 + iabs(x - me_x) + iabs(y - me_y) + (sd == -side ? 0 : 30) + iabs(ddx - 18);
                    /* keep the rest of them in front: none at your back */
                    for (int k = 2; k < DKU_MAX_ACTORS; k++) {
                        const Actor *e = &dku_g.a[k];
                        if (k == t || !awake_foe(e) || e->kind == AK_FEELER) continue;
                        int ex = dku_px(e->x);
                        if ((ex - x) * sd > 0 && iabs(ex - x) < 90) s += 25;
                    }
                    if (s < bs) { bs = s; want_x = x; want_y = y; }
                }
        tolerate = ground_danger(want_x, want_y);
    }
    if (o->kind == AK_FEELER) want_y = oy + 7;
    if (undertow && want_x > 222) want_x = 222;
    if (o->kind == AK_UNDERTOW) want_x = me_x;
    int dx = ox - me_x, dy = oy - me_y;
    bool in_reach = iabs(dx) >= (me->face == side ? 2 : 5) && iabs(dx) <= 21 + DKU_KINDS[o->kind].hw / 2 && dy >= -DKU_REACH_UP + 1 && dy <= 0;
    bool punchable = o->state != AS_BLINK && o->state != AS_RISE && !(o->kind == AK_HOWLER && o->state == AS_RUSH);
    if (in_reach && punchable) {
        if (me->face != side && !(undertow && o->kind == AK_SKIPPER && o->state == AS_HURT)) {
            /* turning steps toward it: never into a grab */
            if (o->state == AS_HURT && iabs(dx) < 17) WHY(39, side > 0 ? BTN_LEFT : BTN_RIGHT);
            WHY(32, side > 0 ? BTN_RIGHT : BTN_LEFT);
        }
        /* four punches, then a breath */
        if (me->combo >= 4) {
            WHY(33, 0);
        }
        bool guarding = o->state == AS_GUARD && o->kind == AK_TUSKER;
        if (guarding && o->guard_hits >= 1) WHY(34, 0); /* let it swing, then punish */
        if ((me->state == AS_FREE || (me->state == AS_ATTACK && me->atk == AT_JAB && me->atk_t > 4)) &&
            danger(me_x, me_y) < 1000) {
            /* the Undertow's skippers: stun one, then walk in and grab it */
            if (undertow && o->kind == AK_SKIPPER && o->state == AS_HURT)
                WHY(35, side > 0 ? BTN_RIGHT : BTN_LEFT);
            WHY(36, press(BTN_A));
        }
        WHY(37, 0);
    }
    /* don't walk into a stunned ghoul by mistake: that grabs it */
    int m = maybe_run(me, step_toward(want_x, want_y), want_x);
    if (o->state == AS_HURT && iabs(dx) < 17 && iabs(dy) <= 6) m &= ~(BTN_LEFT | BTN_RIGHT);
    WHY(38, m);
}

/* never a step toward a hole, whatever the plan says */
static int safe_mask(int m) {
    const Actor *me = &dku_g.a[0];
    if (!me->alive || me->state == AS_DEAD) return m;
    int x = dku_px(me->x), y = dku_px(me->y);
    /* the real edge, a few pixels off (a dodge covers 24) */
    int reach = dodge_phase > 0 ? 26 : 7;
    for (int k = 2; k <= reach; k += 2) {
        if ((m & BTN_UP) && dku_in_pit(x, y - k)) m &= ~BTN_UP;
        if ((m & BTN_DOWN) && dku_in_pit(x, y + k)) m &= ~BTN_DOWN;
    }
    for (int k = 2; k <= 9; k += 2) {
        if ((m & BTN_LEFT) && dku_in_pit(x - k, y)) m &= ~BTN_LEFT;
        if ((m & BTN_RIGHT) && dku_in_pit(x + k, y)) m &= ~BTN_RIGHT;
    }
    return m;
}

int dku_bot_buttons(void) {
    dku_g.bot_t++;
    switch (dku_g.state) {
    case DS_TITLE:
        if (dku_g.menu != 0) return press(BTN_DOWN);
        return dku_g.state_t > 10 ? press(BTN_A) : 0;
    case DS_STORY: return dku_g.state_t > 25 ? press(BTN_A) : 0;
    case DS_SELECT:
        if (dku_g.sel[0] != DK_ROOK) return press(BTN_LEFT);
        return dku_g.state_t > 12 ? press(BTN_A) : 0;
    case DS_CARD: return 0;
    case DS_PLAY: return safe_mask(fight());
    case DS_CLEAR: return dku_g.state_t > 62 ? press(BTN_A) : 0;
    case DS_SHOP: return shop_buttons();
    case DS_CONTINUE: return dku_g.state_t > 32 ? press(BTN_A) : 0;
    case DS_OVER: case DS_GYMOVER: return dku_g.state_t > 62 ? press(BTN_A) : 0;
    case DS_ENDING: return dku_g.state_t > 152 ? press(BTN_A) : 0;
    default: return 0;
    }
}
