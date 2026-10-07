/* RESTLESS - the demo player, used by the tests and the screenshots. It
 * reads the game and presses buttons like a player would: it walks the
 * stage, shoots what is ahead, ducks and jumps what comes at it, waits out
 * traps, takes the ember from a wheel, fights each boss its own way, and in
 * the Low Glow looks ahead to thread the wisp between the guardians. Marks
 * in the map ('!', '^', ',' ...) tell it where a jump or a wait is needed. */
#include "rsl.h"

int rsl_main_h(void);
char rsl_hint(int tx, int ty);

int rsl_bot_plan;

static int bot_dir = 1;
static int fire_t;      /* frames until the next tap */
static int charge_t;    /* holding the attack for a charged shot */
static int stuck_t, last_x, back_t;

static int px(void) { return RSL_PX(rg.pl.x); }
static int py(void) { return RSL_PX(rg.pl.y); }

/* ------------------------------------------------------------------------ */
/* the Low Glow                                                             */

static int step_toward(int fx, int fy, int tx, int ty) {
    int dx = tx > fx + 2 * RSL_FX ? 1 : tx < fx - 2 * RSL_FX ? -1 : 0;
    int dy = ty > fy + 2 * RSL_FX ? 1 : ty < fy - 2 * RSL_FX ? -1 : 0;
    return (dx + 1) * 3 + (dy + 1);
}

static int spirit_buttons(void) {
    const RslSpirit *s = &rg.sp;
    if (s->failed || s->got >= s->n || s->t <= 30) return 0;
    /* the target: the nearest piece left whose guardian is resting; if no
     * piece comes for a while, try another and press harder */
    static int last_got = -1, since = 0, banned = -1;
    if (s->got != last_got || s->t < 2) { last_got = s->got; since = 0; banned = -1; }
    since++;
    int urge = since > 300 ? 3 : 1;
    int tgt = -1, bd = 1 << 30;
    for (int i = 0; i < s->n; i++) {
        if (s->piece[i].taken) continue;
        int d = rsl_dist((s->piece[i].x - s->x) / RSL_FX, (s->piece[i].y - s->y) / RSL_FX);
        for (int g = 0; g < s->nguards; g++)
            if (s->guard[g].piece == i) d += s->guard[g].lunge > 0 ? 50 : s->guard[g].lunge < -20 ? -25 : 0;
        if (i == banned) d += 500;
        if (d < bd) { bd = d; tgt = i; }
    }
    if (tgt < 0) return 0;
    if (since % 300 == 299) banned = tgt;
    static RslSpirit sim;
    long long best = -((long long)1 << 60);
    int bestd = 4; /* code (dx+1)*3+(dy+1); 4 = still */
    const int H = 56;
    int tg = -1; /* the target's guardian */
    for (int g = 0; g < s->nguards; g++)
        if (s->guard[g].piece == tgt) tg = g;
    for (int d1 = 0; d1 < 9; d1++)
        for (int pol = 0; pol < 4; pol++) {
            sim = *s;
            long long score = 0;
            int hit = -1;
            for (int f = 0; f < H; f++) {
                int code = d1;
                if (f >= 8) {
                    if (pol == 0) code = step_toward(sim.x, sim.y, sim.piece[tgt].x, sim.piece[tgt].y);
                    else if (pol == 1) code = 4;
                    else if (pol == 3 && tg >= 0) {
                        /* bait: up to the edge of its reach, then away */
                        const RslGuard *gg = &sim.guard[tg];
                        int dd = rsl_dist((gg->x - sim.x) / RSL_FX, (gg->y - sim.y) / RSL_FX);
                        if (dd > 50 && gg->lunge >= 0) code = step_toward(sim.x, sim.y, gg->x, gg->y);
                        else if (gg->lunge < -10) code = step_toward(sim.x, sim.y, sim.piece[tgt].x, sim.piece[tgt].y);
                        else code = step_toward(sim.x, sim.y, 2 * sim.x - gg->x, 2 * sim.y - gg->y);
                    } else {
                        /* away from the nearest guardian */
                        int ng = -1, nd = 1 << 30;
                        for (int g = 0; g < sim.nguards; g++) {
                            int dd = rsl_dist((sim.guard[g].x - sim.x) / RSL_FX, (sim.guard[g].y - sim.y) / RSL_FX);
                            if (dd < nd) { nd = dd; ng = g; }
                        }
                        code = ng >= 0 ? step_toward(sim.x, sim.y, 2 * sim.x - sim.guard[ng].x, 2 * sim.y - sim.guard[ng].y) : 4;
                    }
                }
                int dx = code / 3 - 1, dy = code % 3 - 1;
                int sp = dx && dy ? 272 : 384;
                sim.x = iclamp(sim.x + dx * sp, 14 * RSL_FX, 306 * RSL_FX);
                sim.y = iclamp(sim.y + dy * sp, 24 * RSL_FX, 170 * RSL_FX);
                for (int g = 0; g < sim.nguards; g++) rsl_guard_step(&sim.guard[g], &sim, rg.deaths);
                rsl_spirit_torches(&sim, rg.deaths);
                if (sim.inv > 0) sim.inv--;
                for (int i = 0; i < sim.n; i++)
                    if (!sim.piece[i].taken && rsl_dist((sim.piece[i].x - sim.x) / RSL_FX, (sim.piece[i].y - sim.y) / RSL_FX) < 9) {
                        sim.piece[i].taken = true;
                        sim.got++;
                        sim.inv = rsl_pick_inv(rg.deaths);
                        score += 6000 - f * 50;
                    }
                if (sim.inv == 0) {
                    int mind = 1 << 30;
                    for (int g = 0; g < sim.nguards; g++) {
                        int dd = rsl_dist((sim.guard[g].x - sim.x) / RSL_FX, (sim.guard[g].y - sim.y) / RSL_FX);
                        if (dd < mind) mind = dd;
                    }
                    if (mind < 11) { hit = f; break; }
                    if (mind < 20) score -= (20 - mind) * 6;
                }
            }
            if (hit >= 0) score -= 2000000 - hit * 30000;
            if (!sim.piece[tgt].taken) {
                score -= rsl_dist((sim.piece[tgt].x - sim.x) / RSL_FX, (sim.piece[tgt].y - sim.y) / RSL_FX) * 10 * urge;
                /* a guardian that has just lunged leaves its piece open: worth baiting */
                for (int g = 0; g < sim.nguards; g++)
                    if (sim.guard[g].piece == tgt && sim.guard[g].lunge < -10) score += 900;
            }
            if (score > best) { best = score; bestd = d1; }
        }
    int dx = bestd / 3 - 1, dy = bestd % 3 - 1;
    int m = 0;
    if (dx > 0) m |= BTN_RIGHT;
    if (dx < 0) m |= BTN_LEFT;
    if (dy > 0) m |= BTN_DOWN;
    if (dy < 0) m |= BTN_UP;
    return m;
}

/* ------------------------------------------------------------------------ */
/* sensing                                                                  */

static bool floor_at(int x, int y) { return rsl_floor_px(x, y) || rsl_lift_under(x, y) >= 0; }

/* the height of the wall right ahead, in tiles (0 none, 9 too high) */
static int wall_ahead(int dir, int reach) {
    int x = px() + dir * (RSL_PW / 2 + reach), feet = py();
    if (!rsl_solid_px(x, feet - 2)) return 0;
    for (int h = 1; h <= 3; h++)
        if (!rsl_solid_px(x, feet - 2 - h * RSL_TILE)) return h;
    return 9;
}

/* will the next step leave the ground? */
static bool edge_ahead(int dir) {
    int x = px() + dir * 2;
    for (int dx = -4; dx <= 2; dx += 3)
        if (floor_at(x + dx, py())) return false;
    return true;
}

/* is there ground to land on within a jump, not lower than a fall we'd take */
static bool landing_ahead(int dir) {
    for (int d = 16; d <= 40; d += 4) {
        int x = px() + dir * d;
        for (int dy = -32; dy <= 48; dy += 2)
            if (rsl_floor_px(x, py() + dy) && !rsl_solid_px(x, py() + dy - 1)) return true;
    }
    return false;
}


static bool is_post(int k) {
    return k == FO_BLOOM || k == FO_SPITTER || k == FO_HIVE || k == FO_FIREMAW || k == FO_SHIELD || k == FO_WEEPER;
}

static int nearest_foe(int *dxo, int *dyo, bool ahead_only, int maxd) {
    int best = -1, bd = maxd;
    for (int i = 0; i < RSL_MAX_FOES; i++) {
        const RslFoe *f = &rg.foe[i];
        if (!f->alive) continue;
        if ((f->flags & FF_HARMLESS) && f->kind != FO_LACKEY) continue;
        if (f->kind == FO_CRAB && f->state == 2) continue;
        int dx = RSL_PX(f->x) - px(), dy = RSL_PX(f->y) - (py() - 12);
        if (ahead_only && dx * rg.pl.face < -8) continue;
        int d = iabs(dx) + iabs(dy) / 2;
        if (is_post(f->kind) && iabs(iabs(dx) - iabs(py() - 17 - RSL_PX(f->y))) < 14) d -= 50; /* lined up: take it */
        if (dx * rg.pl.face < 0 && iabs(dx) < 60 && iabs(dy) < 30) d -= 30;
        if (d < bd) { bd = d; best = i; if (dxo) *dxo = dx; if (dyo) *dyo = dy; }
    }
    return best;
}

static bool hazard_ahead(int dir) {
    /* a fire face about to breathe, or flames, across our way */
    for (int i = 0; i < rg.nspawn; i++) {
        const RslSpawn *s = &rg.spawn[i];
        int sx = s->tx * RSL_TILE + 8, sy = s->ty * RSL_TILE + 8;
        int dx = sx - px();
        if (s->type == SP_FIREWALL) {
            /* wait for the window: from the end of the fire, 160 frames to cross */
            bool near = dir * dx > -70 && dir * dx < 120;
            if (iabs(sy - (py() - 8)) < 24 && near && s->b >= 40 && s->b < 150) {
                /* wait only short of its flames; inside them or past, hurry on */
                int z0 = s->a > 0 ? sx : sx - 64, z1 = s->a > 0 ? sx + 64 : sx;
                bool before = dir > 0 ? px() < z0 - 4 : px() > z1 + 4;
                if (before) return true;
            }
        }
        if (s->type == SP_ROCKFALL && (s->b > 0) && dir * dx > -20 && dir * dx < 44) return true;
    }
    for (int i = 0; i < RSL_MAX_ESHOTS; i++) {
        const RslEShot *e = &rg.es[i];
        if (!e->alive) continue;
        int dx = RSL_PX(e->x) - px();
        if (e->kind == ES_ROCK && dir * dx > -16 && dir * dx < 40 && RSL_PX(e->y) < py()) return true;
        if ((e->kind == ES_FLAME || e->kind == ES_BLAZE) && dir * dx > -10 && dir * dx < 40 && iabs(RSL_PX(e->y) - (py() - 8)) < 16) return true;
        if (e->kind == ES_TRAP) {
            /* a spear on its way: stay put if it will pass in front, jump if it comes at us */
            if (iabs(RSL_PX(e->y) - (py() - 8)) < 14 && ((e->vx > 0 && dx < 0) || (e->vx < 0 && dx > 0)) && iabs(dx) < 200) return true;
        }
    }
    return false;
}

static bool button_ahead(int dir) {
    for (int i = 0; i < rg.nspawn; i++) {
        const RslSpawn *s = &rg.spawn[i];
        if (s->type != SP_BUTTON) continue;
        int bx = s->tx * RSL_TILE + 8;
        int dx = bx - px();
        if (dir * dx > 0 && dir * dx < 22 && iabs((s->ty + 1) * RSL_TILE - py()) < 4) return true;
    }
    return false;
}

/* ---- looking ahead: how long would a move keep him clear? ------------- */

enum { ACT_GO, ACT_STAND, ACT_BACK, ACT_DUCK, ACT_JUMP, ACT_JUMPBACK, ACT_JUMPUP, ACT_N };

static bool boxes_meet(int ax0, int ax1, int ay0, int ay1, int bx, int by, int hw, int hh) {
    return ax1 > bx - hw && ax0 < bx + hw && ay1 > by - hh && ay0 < by + hh;
}

static bool shooting;
static int tap(int hold);
static int rh_plan(int heur, int walk_dir);
static int rh_t;
static void rh_reset(void);
static int duck_t;

static int sim_clear(int act, int dir, int frames) {
    const RslPlayer *p = &rg.pl;
    int x = p->x, y = p->y, vy = p->vy, jd = p->jump_dir, lag = p->lag;
    bool ground = p->ground, duck = false;
    int inv = p->inv;
    if (ground && lag == 0) {
        if (act == ACT_JUMP || act == ACT_JUMPBACK || act == ACT_JUMPUP) {
            vy = RSL_JUMP_VY;
            ground = false;
            jd = act == ACT_JUMP ? dir : act == ACT_JUMPBACK ? -dir : 0;
        }
        duck = act == ACT_DUCK;
    }
    for (int f = 1; f <= frames; f++) {
        if (ground) {
            if (lag > 0) lag--;
            else {
                int nx = x + (act == ACT_GO ? dir : act == ACT_BACK ? -dir : 0) * RSL_WALK;
                if (!rsl_solid_px(RSL_PX(nx) + 5 * (nx > x ? 1 : -1), RSL_PX(y) - 6)) x = nx;
            }
            if (!rsl_floor_px(RSL_PX(x), RSL_PX(y)) && !rsl_floor_px(RSL_PX(x) - 4, RSL_PX(y)) &&
                !rsl_floor_px(RSL_PX(x) + 3, RSL_PX(y)) && rsl_lift_under(RSL_PX(x), RSL_PX(y)) < 0) {
                ground = false;
                vy = 0;
                jd = act == ACT_GO ? dir : act == ACT_BACK ? -dir : 0;
            }
        } else {
            int nx = x + jd * RSL_WALK;
            if (!rsl_solid_px(RSL_PX(nx) + 5 * jd, RSL_PX(y) - 6)) x = nx;
            vy = imin(vy + RSL_GRAV, RSL_MAX_FALL);
            int oy = RSL_PX(y);
            y += vy;
            int ny = RSL_PX(y);
            if (vy > 0)
                for (int t = oy + 1; t <= ny; t++)
                    if (t % RSL_TILE == 0 && rsl_floor_px(RSL_PX(x), t)) { y = t * RSL_FX; ground = true; lag = RSL_LAND_LAG; break; }
        }
        if (inv > f) continue;
        int bx = RSL_PX(x), by = RSL_PX(y);
        int h = duck && ground ? RSL_PH_DUCK : RSL_PH;
        int x0 = bx - 4, x1 = bx + 4, y0 = by - h + 2, y1 = by - 1;
        if (by > rsl_main_h() * RSL_TILE + 8 && rg.pit < 0) return f - 1; /* a pit is a setback, not a death, but avoid it */
        for (int i = 0; i < RSL_MAX_ESHOTS; i++) {
            const RslEShot *e = &rg.es[i];
            if (!e->alive || e->life < f) continue;
            int ex, ey, hw = e->w / 2 + 1, hh = e->h / 2 + 1;
            if (e->kind == ES_TONGUE) { ex = RSL_PX(e->x); ey = RSL_PX(e->y); hw = 22; }
            else if (e->kind == ES_BLADE || e->kind == ES_BLAZE) { ex = RSL_PX(e->x); ey = RSL_PX(e->y); }
            else {
                ex = RSL_PX(e->x + e->vx * f);
                ey = RSL_PX(e->y + e->vy * f + e->grav * f * (f + 1) / 2);
            }
            if (boxes_meet(x0, x1, y0, y1, ex, ey, hw, hh)) return f - 1;
        }
        for (int i = 0; i < RSL_MAX_FOES; i++) {
            const RslFoe *fo = &rg.foe[i];
            if (!fo->alive || fo->kind == FO_KING) continue;
            if (fo->kind == FO_CRAB && fo->state == 2 && fo->st > f) continue;
            bool soon = (fo->flags & FF_HARMLESS) && ((fo->kind == FO_LACKEY && fo->state == 0 && fo->t + f > 24) ||
                                                     (fo->kind == FO_TUMBLER && fo->st + f > 20) ||
                                                     (fo->kind == FO_TOAD && fo->t + f > 20));
            if ((fo->flags & FF_HARMLESS) && !soon) continue;
            if (iabs(RSL_PX(fo->x) - bx) > 160) continue;
            /* a weak foe in our line of fire is as good as dead */
            if (shooting && fo->hp <= 10 && (RSL_PX(fo->x) - RSL_PX(p->x)) * p->face > 0 &&
                iabs(RSL_PX(fo->x) - RSL_PX(p->x)) < 90 && iabs(RSL_PX(fo->y) - (RSL_PX(p->y) - 15)) < fo->h / 2 + 3)
                continue;
            int fx, fy;
            if (fo->kind == FO_WISP) {
                fx = RSL_PX(fo->x + fo->dir * 300 * f);
                fy = RSL_PX(fo->b + rsl_sin((fo->t + f) * 2 + fo->var * 8) * 22 * RSL_FX / 256);
            } else if (fo->kind == FO_CLOCKFIRE) {
                int sp = 256 + imin(rg.deaths, 6) * 16;
                int a = rsl_dir_to(x - fo->x, y - 16 * RSL_FX - fo->y);
                fx = RSL_PX(fo->x + rsl_cos(a) * sp / 256 * f);
                fy = RSL_PX(fo->y + rsl_sin(a) * sp / 256 * f);
            } else {
                int vx = fo->x - fo->ox, vyy = fo->y - fo->oy;
                if (fo->flags & FF_GROUNDED) vyy = 0;
                fx = RSL_PX(fo->x + vx * f);
                fy = RSL_PX(fo->y + vyy * f);
            }
            if (boxes_meet(x0, x1, y0, y1, fx, fy, fo->w / 2 - 1, fo->h / 2 - 1)) return f - 1;
        }
    }
    return frames;
}

static int act_of(int m, int dir) {
    if (m & BTN_B) {
        if (m & (dir > 0 ? BTN_RIGHT : BTN_LEFT)) return ACT_JUMP;
        if (m & (dir > 0 ? BTN_LEFT : BTN_RIGHT)) return ACT_JUMPBACK;
        return ACT_JUMPUP;
    }
    if (m & BTN_DOWN) return ACT_DUCK;
    if (m & (dir > 0 ? BTN_RIGHT : BTN_LEFT)) return ACT_GO;
    if (m & (dir > 0 ? BTN_LEFT : BTN_RIGHT)) return ACT_BACK;
    return ACT_STAND;
}



/* ---- the real look-ahead: run the game itself on a copy ----------------- */

typedef struct { int m1, k, m2; } Plan;
typedef struct { int survived, dx, hurt, pit, score; } SimOut;

static RslGame sim_keep;
static RslSave sim_keep_save;
static uint32_t last_out;

static SimOut sim_real(Plan pl, int n, int dir) {
    memcpy(&sim_keep, &rg, sizeof rg);
    sim_keep_save = rgs;
    rsl_simulating = true;
    rsl_sim_cur = last_out;
    SimOut o = {n, 0, 0, 0, 0};
    bool owl = rg.owl.on;
    int x0 = rg.pl.x, pit0 = rg.pit;
    for (int f = 0; f < n; f++) {
        uint32_t m = (uint32_t)(f < pl.k ? pl.m1 : pl.m2);
        if ((m & BTN_A) && (f % 6) != 0) m &= ~(uint32_t)BTN_A;
        rsl_sim_prev = rsl_sim_cur;
        rsl_sim_cur = m;
        rg.state_t++;
        rg.frame_t++;
        rsl_play_step();
        if (rg.state == RS_DYING || rg.hits != sim_keep.hits) { o.survived = f; break; }
        if (rg.state != RS_PLAY) break;
        if (owl && !rg.owl.on) o.hurt = 1;
        if (rg.pit != pit0 && pit0 < 0) o.pit = 1;
    }
    o.dx = (rg.pl.x - x0) * dir / RSL_FX;
    o.score = (int)rg.score;
    rsl_simulating = false;
    memcpy(&rg, &sim_keep, sizeof rg);
    rgs = sim_keep_save;
    return o;
}

static bool quiet_around(int r) {
    for (int i = 0; i < RSL_MAX_FOES; i++)
        if (rg.foe[i].alive && iabs(RSL_PX(rg.foe[i].x) - px()) < r + rg.foe[i].w / 2 && iabs(RSL_PX(rg.foe[i].y) - py()) < r)
            return false;
    for (int i = 0; i < RSL_MAX_ESHOTS; i++)
        if (rg.es[i].alive && iabs(RSL_PX(rg.es[i].x) - px()) < r + 40 && iabs(RSL_PX(rg.es[i].y) - py()) < r + 40) return false;
    return true;
}

static int plan_score(SimOut o, int n) {
    int sc = o.survived * 1000;
    if (o.survived >= n) sc += 100000;
    if (o.hurt) sc -= 60000;
    if (o.pit) sc -= 30000;
    sc += o.dx * 20;
    return sc;
}

/* keep the plan if the game says it's safe; otherwise the best of a few others */
static int safe_choice(int m, int dir) {
    const RslPlayer *p = &rg.pl;
    int keep = m & (BTN_A | BTN_UP);
    shooting = (m & BTN_A) || rg.pl.fire_t > 0;
    if (!p->ground) {
        /* in the air: shoot down the slant at what's below */
        int fdx = 0, fdy = 0;
        int fi = nearest_foe(&fdx, &fdy, true, 120);
        if (fi >= 0 && fdy > 6 && iabs(iabs(fdx) - (RSL_PX(rg.foe[fi].y) - (py() - 8))) < 14) m |= BTN_DOWN | tap(4);
        return m & (BTN_A | BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT);
    }
    const int H = 40;
    if (quiet_around(90) && sim_clear(act_of(m, dir), dir, 30) >= 30) return m;
    int fwd = dir > 0 ? BTN_RIGHT : BTN_LEFT, bwd = dir > 0 ? BTN_LEFT : BTN_RIGHT;
    int fire = keep;
    Plan plans[11] = {
        {m, H, m},
        {BTN_DOWN | (fire & BTN_A), H, BTN_DOWN | (fire & BTN_A)},
        {fire, H, fire},
        {bwd, H, bwd},
        {BTN_DOWN | (fire & BTN_A), 14, m},
        {fire, 14, m},
        {bwd, 14, m},
        {BTN_B | fwd, 2, fwd | fire},
        {BTN_B | bwd, 2, bwd},
        {BTN_B, 2, fire},
        {fwd | fire, H, fwd | fire},
    };
    int n = (p->lag > 0) ? 7 : 11;
    SimOut o = sim_real(plans[0], H, dir);
    if (o.survived >= H && !o.hurt && !o.pit) return m;
    int best = 0, bs = plan_score(o, H);
    for (int k = 1; k < n; k++) {
        int jumpy = plans[k].m1 & BTN_B;
        SimOut q = sim_real(plans[k], jumpy ? 56 : H, dir);
        if (jumpy && q.survived >= 56) q.survived = H;
        else if (jumpy) q.survived = imin(q.survived, H - 1);
        int sc = plan_score(q, H);
        if (sc > bs) { bs = sc; best = k; }
    }
    if ((plans[best].m1 & BTN_DOWN) && duck_t > 90) {
        /* crouched too long: anything else that survives */
        for (int k = 0; k < n; k++) {
            if (plans[k].m1 & BTN_DOWN) continue;
            SimOut q = sim_real(plans[k], H, dir);
            if (q.survived >= H && !q.hurt) { best = k; break; }
        }
    }
    int out = plans[best].m1;
    if (out & BTN_DOWN) out &= ~BTN_UP;
    if (charge_t > 0) out |= m & BTN_A; /* never let go of a charge to dodge */
    return out;
}

/* is the way clear for a shot from the muzzle to (x, y)? */
static bool line_clear(int x, int y, bool duck) {
    int sx = px() + rg.pl.face * 9, sy = py() - (duck ? 8 : 17);
    int n = imax(iabs(x - sx), iabs(y - sy)) / 4 + 1;
    for (int k = 1; k < n; k++) {
        int qx = sx + (x - sx) * k / n, qy = sy + (y - sy) * k / n;
        if (rsl_solid_px(qx, qy)) return false;
    }
    return true;
}

static int tap(int hold) {
    /* tap the attack every few frames (a held button would be blocked) */
    if (fire_t > 0) { fire_t--; return 0; }
    fire_t = hold;
    return BTN_A;
}

/* aim at a foe from where he stands: up the slant, straight, or crouched */
static int aim_at(const RslFoe *f) {
    int m = 0;
    int fx = RSL_PX(f->x), fy = RSL_PX(f->y);
    int dx = fx - px(), adx = iabs(dx);
    int top = fy - f->h / 2, bot = fy + f->h / 2;
    int mz = py() - 17;
    if ((dx > 0) != (rg.pl.face > 0) && adx > 3) return dx > 0 ? BTN_RIGHT : BTN_LEFT;
    if (bot < mz - 6) {
        /* above: up the slant, if it lines up */
        if (iabs((mz - fy) - adx) < 18 + f->h / 2) m |= BTN_UP;
        else return 0;
    } else if (top > mz + 1) {
        if (rg.pl.ground) m |= BTN_DOWN;
    }
    if (!(m & BTN_UP) && !line_clear(fx, (m & BTN_DOWN) ? py() - 8 : mz, (m & BTN_DOWN) != 0)) {
        /* something in the way: the slant, if it reaches */
        if (iabs((mz - top) - adx) < 24) m = (m & ~BTN_DOWN) | BTN_UP;
        else return 0;
    }
    return m | tap(5);
}

/* ------------------------------------------------------------------------ */
/* walking a half                                                           */

static int want_item(int *tx) {
    /* a weapon of a wheel (ember first), or treasure close by */
    int best = -1, bd = 1 << 30;
    for (int i = 0; i < RSL_MAX_ITEMS; i++) {
        const RslItem *it = &rg.item[i];
        if (!it->alive || (it->kind == IT_ASH && rg.deaths == 0)) continue;
        int dx = RSL_PX(it->x) - px(), dy = RSL_PX(it->y) - py();
        if (iabs(dx) > 120 || dy < -60 || dy > 24) continue;
        if (it->kind == IT_CHARM) continue;
        int pri = iabs(dx);
        if (it->kind == IT_WEAPON) {
            if (it->var == rg.pl.weapon) continue;
            if (it->var != WP_EMBER) continue;
            pri -= 200;
        }
        if (it->kind == IT_ASH) pri -= 300;
        if (it->kind == IT_WEAPON || it->kind == IT_ASH || it->landed) {
            if (it->kind != IT_ASH && it->kind != IT_WEAPON) {
                int g = rsl_ground_below(RSL_PX(it->x), RSL_PX(it->y) - 4);
                if (g < 0 || iabs(g - py()) > 2) continue;
                /* nothing in the way */
                bool wall = false;
                for (int x = px(); x != RSL_PX(it->x); x += dx > 0 ? 1 : -1)
                    if (rsl_solid_px(x, py() - 4)) { wall = true; break; }
                if (wall) continue;
            }
            if (pri < bd) { bd = pri; best = i; }
        }
    }
    if (best >= 0) *tx = RSL_PX(rg.item[best].x);
    return best;
}

static int lift_ready(void) {
    /* waiting at a ',' mark: is a lift against the end of our ledge, level with it? */
    int dir = bot_dir;
    int end = px();
    while (iabs(end - px()) < 64 && rsl_floor_px(end + dir, py())) end += dir;
    for (int i = 0; i < rg.nlift; i++) {
        const RslLift *l = &rg.lift[i];
        int lx = RSL_PX(l->x), ly = RSL_PX(l->y);
        if (iabs(ly - py()) > 1) continue;
        int near = dir > 0 ? lx : lx + l->w - 1;
        if (l->vertical) {
            if (iabs(near - end) <= 6 && l->t % l->period < l->period / 2 - 10) return 1;
        } else if (iabs(near - end) <= 4) {
            return 1;
        }
    }
    return 0;
}

static int fight_t;

static int walk_buttons(void) {
    RslPlayer *p = &rg.pl;
    int m = 0;
    int tx = px() / RSL_TILE, ty = (py() - 1) / RSL_TILE;
    char h = rsl_hint(tx, ty);
    if (h == '<') bot_dir = -1;
    if (h == '>') bot_dir = 1;
    int dir = bot_dir;
    bool go = true, turn = false;

    if (back_t > 0) { back_t--; dir = -dir; }

    /* items worth a detour */
    int itx = 0;
    int it = want_item(&itx);
    if (it >= 0 && back_t == 0) {
        if (iabs(itx - px()) > 3) dir = itx > px() ? 1 : -1;
        else go = false;
    }

    /* the nearest foe: face it and shoot; stop for anything ahead */
    int fdx = 0, fdy = 0;
    int fi = nearest_foe(&fdx, &fdy, false, 220);
    if (fi >= 0) {
        const RslFoe *f = &rg.foe[fi];
        int adx = iabs(fdx);
        int reach = p->weapon == WP_EMBER ? 150 : p->weapon == WP_SCATTER ? 50 : p->weapon == WP_SEEKER ? 140 : 60;
        bool harmless = (f->flags & FF_HARMLESS) != 0;
        int fx = RSL_PX(f->x), fy = RSL_PX(f->y), mz = py() - 17;
        bool above = fy + f->h / 2 < mz - 6;
        if (!harmless && adx < reach + 80 && fdy > -120 && fdy < 50 && fight_t < 300) {
            fight_t++;
            if (above) {
                /* to the spot where a slanted shot meets it */
                int rise = mz - fy;
                int side = fdx > 0 ? -1 : 1;
                int spot = fx + side * rise;
                if (rsl_solid_px(spot, py() - 8) || !rsl_floor_px(spot, py())) spot = fx - side * rise;
                if (iabs(spot - px()) > 5) { dir = spot > px() ? 1 : -1; go = true; }
                else {
                    go = false;
                    int a = aim_at(f);
                    if (a & (BTN_LEFT | BTN_RIGHT)) m |= a & (BTN_LEFT | BTN_RIGHT);
                    else m |= a;
                }
            } else if (fy - f->h / 2 > py() - 2 && adx < 64 && adx > 8) {
                /* below us, off a ledge: jump and shoot down the slant */
                if ((fdx > 0) != (p->face > 0)) { dir = fdx > 0 ? 1 : -1; m |= dir > 0 ? BTN_RIGHT : BTN_LEFT; turn = true; }
                else if (p->ground && p->lag == 0) m |= BTN_B;
                go = false;
            } else {
                int a = aim_at(f);
                if (a & (BTN_LEFT | BTN_RIGHT)) {
                    /* turn to it: one press of the pad */
                    dir = a & BTN_RIGHT ? 1 : -1;
                    m |= a & (BTN_LEFT | BTN_RIGHT);
                    turn = true;
                } else m |= a;
                /* keep moving and shooting; stand for a post in reach or anything close */
                go = !(adx < 36 || (is_post(f->kind) && adx < reach && (fdx > 0) == (dir > 0)));
                if ((fdx > 0) != (dir > 0)) go = false;
            }
        }
        if (harmless && f->kind == FO_LACKEY && adx < 40 && iabs(fdy) < 20) go = false;
    } else {
        fight_t = 0;
    }
    /* torches ahead in line */
    for (int i = 0; i < rg.nspawn; i++) {
        const RslSpawn *s = &rg.spawn[i];
        if (s->type != SP_TORCH || s->used) continue;
        int dx = s->tx * RSL_TILE + 8 - px(), dy = s->ty * RSL_TILE + 4 - (py() - 17);
        if (dx * p->face > 0 && iabs(dx) < 90 && !(m & BTN_A)) {
            if (iabs(dy) < 10) m |= tap(6);
            else if (dy < -20 && iabs(iabs(dx) - iabs(dy)) < 20) m |= BTN_UP | tap(6);
            else if (dy < -20 && iabs(dx) > iabs(dy) + 18 && iabs(dx) < iabs(dy) + 60) go = true;
        }
    }

    if (hazard_ahead(dir)) go = false;
    if (h == ',' && !lift_ready() && p->lift < 0) go = false;
    /* on a lift: stay on until there's floor right ahead */
    if (p->lift >= 0 && p->ground) {
        const RslLift *l = &rg.lift[p->lift];
        if (!l->vertical) {
            /* ride at its front edge, step off when the ledge is there */
            int edge = dir > 0 ? RSL_PX(l->x) + l->w : RSL_PX(l->x) - 1;
            bool ledge = rsl_floor_px(edge + dir * 2, py()) && !rsl_solid_px(edge + dir * 2, py() - 4);
            go = ledge || iabs(edge - px()) > 8;
        }
    }

    if (go && !(m & BTN_DOWN)) {
        m |= dir > 0 ? BTN_RIGHT : BTN_LEFT;
        if (p->ground && p->lag == 0) {
            int wh = wall_ahead(dir, 3);
            if (wh >= 1 && wh <= 2) m |= BTN_B;
            else if (edge_ahead(dir)) {
                int g = rsl_ground_below(px() + dir * 10, py());
                bool pit = g < 0 || g > py() + 40;
                if (pit) {
                    if (landing_ahead(dir)) m |= BTN_B;
                    else m &= ~(BTN_LEFT | BTN_RIGHT);
                }
            }
            if (button_ahead(dir)) m |= BTN_B;
            if (h == '!') m |= BTN_B;
        }
    }
    if (h == '^' && p->ground && p->lag == 0 && go) { m &= ~(BTN_LEFT | BTN_RIGHT); m |= BTN_B; }
    if (turn && !(m & (BTN_B | BTN_DOWN))) {
        /* a turn is a single step: let it through unless it walks into something */
        if (sim_clear(ACT_GO, dir, 16) >= 16) return m;
    }
    if (!quiet_around(110) && (p->ground || rh_t > 0)) m = rh_plan(m, dir);
    else { rh_reset(); m = safe_choice(m, dir); }
    duck_t = (m & BTN_DOWN) ? duck_t + 1 : 0;
    /* stuck: back off a little and try again */
    if ((m & (BTN_LEFT | BTN_RIGHT)) && p->ground && px() == last_x) {
        if (++stuck_t > 90) { stuck_t = 0; back_t = 24; }
    } else {
        stuck_t = 0;
    }
    last_x = px();
    return m;
}

/* ------------------------------------------------------------------------ */
/* the fights                                                               */

static int boss_index(int kind) {
    for (int i = 0; i < RSL_MAX_FOES; i++)
        if (rg.foe[i].alive && rg.foe[i].kind == kind) return i;
    return -1;
}

static int go_to(int x, int tol) {
    if (px() < x - tol) return BTN_RIGHT;
    if (px() > x + tol) return BTN_LEFT;
    return 0;
}

static int face_to(int x) {
    /* a one-frame press turns him; walking a step is fine */
    if ((x > px()) != (rg.pl.face > 0) && iabs(x - px()) > 4) return x > px() ? BTN_RIGHT : BTN_LEFT;
    return 0;
}

/* fire at a point: straight or up the slant */
static int shoot_at(int x, int y) {
    int m = face_to(x);
    int dx = iabs(x - px()), dy = (py() - 17) - y;
    if (dy > 18 && dy > dx / 3) m |= BTN_UP;
    return m | tap(5);
}

static int charged_at(int x, int y) {
    /* hold the attack, let go when ready */
    int m = face_to(x);
    int dy = (py() - 17) - y;
    if (dy > 18) m |= BTN_UP;
    if (charge_t > 0) {
        charge_t--;
        if (charge_t > 0) m |= BTN_A;
        return m;
    }
    if (!rg.pl.charging) { charge_t = RSL_CHARGE_T + 4; return m | BTN_A; }
    return m;
}

static int dodge(int m) {
    int dir = rg.pl.face;
    if (m & BTN_RIGHT) dir = 1;
    if (m & BTN_LEFT) dir = -1;
    return safe_choice(m, dir);
}

static int ax0(void) { return rg.lock_x; }

/* ---- a fight planner: try each plan in the real game, keep the best ---- */

/* where to stand to hit the nearest boss part: up the slant, or level with it */
static int fight_spot_x(void) {
    int best = -1, bd = 1 << 30;
    int mz = py() - 17;
    for (int i = 0; i < RSL_MAX_FOES; i++) {
        const RslFoe *f = &rg.foe[i];
        if (!f->alive || !RSL_IS_BOSS(f->kind) || f->kind == FO_SKULL) continue;
        int fx = RSL_PX(f->x), fy = RSL_PX(f->y);
        if (f->kind == FO_KING) { fx -= 22; fy -= 10; }
        int rise = mz - fy;
        int cand[2];
        if (rise > 20) { cand[0] = fx - rise - 9; cand[1] = fx + rise + 9; }
        else { cand[0] = fx - 70; cand[1] = fx + 70; }
        for (int c = 0; c < 2; c++) {
            int x = cand[c];
            if (x < rg.lock_x + 14 || x > rg.lock_x + SCREEN_W - 14) continue;
            if (f->kind == FO_KING && x > fx) continue;
            int d = iabs(x - px());
            if (d < bd) { bd = d; best = x; }
        }
    }
    return best;
}

/* ---- the look-ahead planner -------------------------------------------
 * Every few frames it tries some two dozen button plans 72 frames long in a
 * copy of the game (the real rules), keeps the best, and plays its first
 * frames. A plan is three moves one after another; the candidates are last
 * time's best (moved on), the walker's own idea, each move on its own, and
 * a few mixed at random. */

#define RH_LEN 72
#define RH_STEP 6
enum { RH_FIRE, RH_UPFIRE, RH_DUCKFIRE, RH_LFIRE, RH_RFIRE, RH_LUP, RH_RUP, RH_JL, RH_JR, RH_JUP, RH_L, RH_R,
       RH_DUCK, RH_HEUR, RH_ACTS };

typedef struct { uint8_t act[RH_LEN], first[RH_LEN]; } RhPlan;

static RhPlan rh_best;
static int rh_valid, rh_heur;
static uint32_t rh_seed = 12345u;

static uint32_t rh_rand(void) {
    rh_seed = rh_seed * 1103515245u + 12345u;
    return rh_seed >> 8;
}

static int rh_mask(int act, int first, int frame) {
    int a = (frame % 6) == 0 ? BTN_A : 0;
    switch (act) {
    case RH_FIRE: return a;
    case RH_UPFIRE: return BTN_UP | a;
    case RH_DUCKFIRE: return BTN_DOWN | a;
    case RH_LFIRE: return BTN_LEFT | a;
    case RH_RFIRE: return BTN_RIGHT | a;
    case RH_LUP: return BTN_LEFT | BTN_UP | a;
    case RH_RUP: return BTN_RIGHT | BTN_UP | a;
    case RH_JL: return first ? (BTN_B | BTN_LEFT) : (BTN_LEFT | a);
    case RH_JR: return first ? (BTN_B | BTN_RIGHT) : (BTN_RIGHT | a);
    case RH_JUP: return first ? BTN_B : (BTN_UP | a);
    case RH_L: return BTN_LEFT;
    case RH_R: return BTN_RIGHT;
    case RH_DUCK: return BTN_DOWN;
    case RH_HEUR: {
        int m = rh_heur & ~BTN_A;
        if (!first) m &= ~BTN_B;
        return m | ((rh_heur & BTN_A) ? a : 0);
    }
    default: return 0;
    }
}

static void rh_fill(RhPlan *p, int from, int act) {
    for (int i = from; i < RH_LEN; i++) { p->act[i] = (uint8_t)act; p->first[i] = i == from; }
}

/* run a plan in a copy of the game and score it */
static int rh_eval(const RhPlan *pl, int walk_dir, int spot) {
    memcpy(&sim_keep, &rg, sizeof rg);
    sim_keep_save = rgs;
    rsl_simulating = true;
    rsl_sim_cur = last_out;
    int hp0 = rsl_boss_hp_total(), k0 = rg.kills;
    bool owl = rg.owl.on;
    int x0 = rg.pl.x, pit0 = rg.pit, half0 = rg.half;
    int survived = RH_LEN, hurt = 0, pitted = 0;
    for (int f = 0; f < RH_LEN; f++) {
        rsl_sim_prev = rsl_sim_cur;
        rsl_sim_cur = (uint32_t)rh_mask(pl->act[f], pl->first[f], f);
        rg.state_t++;
        rg.frame_t++;
        rsl_play_step();
        if (rg.state == RS_DYING || rg.hits != sim_keep.hits) { survived = f; break; }
        if (rg.state != RS_PLAY || rg.boss_dead) break;
        if (owl && !rg.owl.on) hurt = 1;
        if (rg.pit != pit0 && pit0 < 0) { pitted = 1; break; }
    }
    int prog = rg.half == half0 ? (rg.pl.x - x0) / RSL_FX : 0;
    int dmg = hp0 - (rg.boss_dead ? 0 : rsl_boss_hp_total());
    int kills = rg.kills - k0;
    int end_x = RSL_PX(rg.pl.x);
    int edge = rg.lock_x >= 0 ? imin(end_x - rg.lock_x, rg.lock_x + SCREEN_W - end_x) : 99;
    rsl_simulating = false;
    memcpy(&rg, &sim_keep, sizeof rg);
    rgs = sim_keep_save;
    int sc = survived * 2000 + (survived >= RH_LEN ? 300000 : 0) - hurt * 120000 - pitted * 150000 + dmg * 300;
    if (walk_dir) sc += prog * walk_dir * (sim_keep.time < 50 * 60 ? 120 : 60) + kills * 1500;
    else {
        sc += kills * 200;
        if (spot >= 0) sc -= iabs(end_x - spot) * 25;
    }
    if (edge < 40) sc -= (40 - edge) * 20;
    return sc;
}

static void rh_reset(void) { rh_valid = 0; rh_t = 0; }

/* walk_dir: +1/-1 walking (progress counts), 0 in a fight */
static int rh_plan(int heur, int walk_dir) {
    const RslPlayer *p = &rg.pl;
    rh_heur = heur;
    if (rh_valid && rh_t > 0) {
        /* play on the chosen plan */
        int i = RH_STEP - rh_t;
        rh_t--;
        return rh_mask(rh_best.act[i], rh_best.first[i], i);
    }
    int spot = walk_dir ? -1 : fight_spot_x();
    static RhPlan cand[32];
    int n = 0;
    /* last time's best, moved on */
    if (rh_valid) {
        RhPlan *c = &cand[n++];
        for (int i = 0; i < RH_LEN; i++) {
            int j = i + RH_STEP < RH_LEN ? i + RH_STEP : RH_LEN - 1;
            c->act[i] = rh_best.act[j];
            c->first[i] = i + RH_STEP < RH_LEN ? rh_best.first[j] : 0;
        }
    }
    /* the walker's idea, and each move held throughout */
    if (walk_dir) rh_fill(&cand[n++], 0, RH_HEUR);
    for (int a = 0; a < RH_HEUR; a++) {
        if ((a == RH_JL || a == RH_JR || a == RH_JUP) && (!p->ground || p->lag > 0)) continue;
        rh_fill(&cand[n++], 0, a);
    }
    /* mixes of three */
    while (n < 26) {
        RhPlan *c = &cand[n++];
        int s1 = 6 + (int)(rh_rand() % 24), s2 = s1 + 8 + (int)(rh_rand() % 24);
        int a1 = (int)(rh_rand() % RH_HEUR), a2 = (int)(rh_rand() % RH_HEUR), a3 = (int)(rh_rand() % RH_HEUR);
        if (walk_dir && rh_rand() % 3 == 0) a1 = RH_HEUR;
        rh_fill(c, 0, a1);
        rh_fill(c, s1, a2);
        if (s2 < RH_LEN) rh_fill(c, s2, a3);
    }
    int best = 0, bs = -(1 << 30);
    for (int i = 0; i < n; i++) {
        int sc = rh_eval(&cand[i], walk_dir, spot);
        if (sc > bs) { bs = sc; best = i; }
    }
    rh_best = cand[best];
    rh_valid = 1;
    rh_t = RH_STEP - 1;
    return rh_mask(rh_best.act[0], rh_best.first[0], 0);
}

static int boss_buttons(void) {
    if (rsl_bot_plan >= 0) return rh_plan(0, 0);
    int m = 0;
    RslPlayer *p = &rg.pl;
    int i;
    /* small fry first */
    int fdx = 0, fdy = 0;
    int fi = nearest_foe(&fdx, &fdy, false, 90);
    if (fi >= 0 && !RSL_IS_BOSS(rg.foe[fi].kind)) {
        const RslFoe *f = &rg.foe[fi];
        m = shoot_at(RSL_PX(f->x), RSL_PX(f->y));
        if (iabs(fdx) < 26 && iabs(fdy) < 24) m |= fdx > 0 ? BTN_LEFT : BTN_RIGHT;
        return dodge(m);
    }
    switch (rg.boss_kind) {
    case BOSS_KEEPER:
        if ((i = boss_index(FO_KEEPER)) >= 0) {
            const RslFoe *f = &rg.foe[i];
            int bx = RSL_PX(f->x);
            int land = f->state >= 1 ? bx + f->b : bx;
            int safe = land < ax0() + 160 ? land + 110 : land - 110;
            safe = iclamp(safe, ax0() + 16, ax0() + 304);
            if (f->state == 0 || iabs(px() - land) < 60) m |= go_to(safe, 8);
            if (!(m & (BTN_LEFT | BTN_RIGHT))) m |= shoot_at(bx, RSL_PX(f->y));
            else m |= tap(6);
            /* bombs above */
            for (int e = 0; e < RSL_MAX_ESHOTS; e++)
                if (rg.es[e].alive && rg.es[e].kind == ES_BOMB && iabs(RSL_PX(rg.es[e].x) - px()) < 40)
                    m = (m & ~(BTN_LEFT | BTN_RIGHT)) | (RSL_PX(rg.es[e].x) > px() ? BTN_LEFT : BTN_RIGHT);
        }
        break;
    case BOSS_RATTLE:
        if ((i = boss_index(FO_RATTLE)) >= 0) {
            const RslFoe *f = &rg.foe[i];
            int bx = RSL_PX(f->x), by = RSL_PX(f->y);
            if (f->b == 0 && f->state == 1) {
                /* on the floor and running: jump him */
                int dx = bx - px();
                if (dx * f->dir < 0 && iabs(dx) < 44 && p->ground) m |= BTN_B | (dx > 0 ? BTN_RIGHT : BTN_LEFT);
                else m |= shoot_at(bx, by);
            } else {
                /* keep him up and to one side, shoot up the slant */
                int want = bx + (bx > ax0() + 160 ? -(py() - 17 - by) : (py() - 17 - by));
                want = iclamp(want, ax0() + 14, ax0() + 306);
                m |= go_to(want, 6);
                m |= (m & (BTN_LEFT | BTN_RIGHT)) ? tap(6) : shoot_at(bx, by);
                if (!(m & (BTN_LEFT | BTN_RIGHT))) m |= BTN_UP;
            }
            for (int e = 0; e < RSL_MAX_ESHOTS; e++)
                if (rg.es[e].alive && rg.es[e].kind == ES_BOMB && iabs(RSL_PX(rg.es[e].x) - px()) < 20 && RSL_PX(rg.es[e].y) < py())
                    m = (m & ~(BTN_LEFT | BTN_RIGHT)) | (RSL_PX(rg.es[e].x) > px() ? BTN_LEFT : BTN_RIGHT);
        }
        break;
    case BOSS_TONGUE: {
        /* under a head and off to the side by its height, charged shots up the slant */
        int best = -1, bd = 1 << 30;
        for (int k = 0; k < RSL_MAX_FOES; k++)
            if (rg.foe[k].alive && rg.foe[k].kind == FO_HEAD) {
                int d = iabs(RSL_PX(rg.foe[k].x) - px());
                if (d < bd) { bd = d; best = k; }
            }
        if (best >= 0) {
            const RslFoe *f = &rg.foe[best];
            int hx = RSL_PX(f->x), hy = RSL_PX(f->y);
            int off = (py() - 17) - hy;
            int want = hx + (hx > ax0() + 160 ? -off : off);
            want = iclamp(want, ax0() + 14, ax0() + 306);
            m |= go_to(want, 4);
            bool far_gun = rg.pl.weapon == WP_EMBER || rg.pl.weapon == WP_SEEKER;
            bool lined = iabs(want - px()) < 12;
            if (far_gun && lined) m |= tap(5) | BTN_UP;
            else if (!far_gun && (lined || charge_t > 0)) m |= charged_at(hx, hy) | BTN_UP;
        }
        for (int e = 0; e < RSL_MAX_ESHOTS; e++) {
            const RslEShot *q = &rg.es[e];
            if (!q->alive) continue;
            if ((q->kind == ES_FIRE && iabs(RSL_PX(q->x) - px()) < 22 && RSL_PX(q->y) < py()) ||
                (q->kind == ES_BLAZE && iabs(RSL_PX(q->x) - px()) < 16))
                m = (m & ~(BTN_LEFT | BTN_RIGHT)) | (RSL_PX(q->x) > px() ? BTN_LEFT : BTN_RIGHT);
        }
        break;
    }
    case BOSS_PAIR: {
        int g = boss_index(FO_GUST), sl = boss_index(FO_GASH);
        int mz = py() - 17;
        int want = -1;
        /* keep Gash at arm's length */
        if (sl >= 0) {
            const RslFoe *f = &rg.foe[sl];
            int dx = RSL_PX(f->x) - px();
            int away = dx > 0 ? -1 : 1;
            int room = away < 0 ? px() - (ax0() + 16) : ax0() + 304 - px();
            if (iabs(dx) < 70) {
                if (room > 50) want = px() + away * 40;
                else if (f->state != 1 && iabs(dx) >= 30 && iabs(dx) < 56 && p->ground && p->lag == 0) {
                    /* cornered: over his head */
                    m |= BTN_B | (dx > 0 ? BTN_RIGHT : BTN_LEFT);
                    return m;
                } else if (iabs(dx) < 30) want = px() + away * 40;
            } else if (iabs(dx) > 120) {
                want = px() + (dx > 0 ? 30 : -30); /* don't let him get the middle */
            }
        }
        /* a target in line: Gust when low or on the slant, else Gash */
        int shot = 0;
        if (g >= 0) {
            const RslFoe *f = &rg.foe[g];
            int gx = RSL_PX(f->x + (f->x - f->ox) * 14), gy = RSL_PX(f->y);
            int dx = gx - px(), rise = mz - gy;
            if (f->state == 1 && iabs(dx) < 120 && gy > py() - 44) { m &= ~(BTN_LEFT | BTN_RIGHT | BTN_B); return m | BTN_DOWN; }
            if (iabs(gy - mz) < 10) shot = shoot_at(gx, gy);
            else if (rise > 0 && iabs(iabs(dx) - rise) < 10) shot = face_to(gx) | BTN_UP | tap(5);
            else if (want < 0 && rise > 0) want = gx - (dx > 0 ? rise : -rise) * (gx > px() ? 1 : 1);
        }
        if (!shot && sl >= 0) shot = shoot_at(RSL_PX(rg.foe[sl].x), RSL_PX(rg.foe[sl].y));
        if (want >= 0) {
            want = iclamp(want, ax0() + 14, ax0() + 306);
            m |= go_to(want, 4);
        }
        if (!(m & (BTN_LEFT | BTN_RIGHT))) m |= shot;
        else m |= shot & (BTN_A | BTN_UP);
        break;
    }
    case BOSS_RAMMER:
        if ((i = boss_index(FO_RAMMER)) >= 0) {
            const RslFoe *f = &rg.foe[i];
            int dx = RSL_PX(f->x) - px();
            if (f->state == 1 && dx * f->dir < 0 && iabs(dx) < 66 && p->ground && p->lag == 0)
                m |= BTN_B | (dx > 0 ? BTN_RIGHT : BTN_LEFT);
            else {
                m |= shoot_at(RSL_PX(f->x), RSL_PX(f->y));
                if (f->state == 0) m |= go_to(ax0() + 160 + (dx > 0 ? -60 : 60), 20);
            }
        }
        break;
    case BOSS_KING:
        if ((i = boss_index(FO_KING)) >= 0) {
            const RslFoe *f = &rg.foe[i];
            int ex = RSL_PX(f->x) - 22, ey = RSL_PX(f->y) - 10;
            int want = ex - ((py() - 17) - ey) - 9;
            m |= go_to(want, 3);
            if (!(m & (BTN_LEFT | BTN_RIGHT))) m |= charged_at(ex, ey) | BTN_UP;
            else if (charge_t > 0) m |= BTN_A;
            for (int e = 0; e < RSL_MAX_ESHOTS; e++)
                if (rg.es[e].alive && rg.es[e].kind == ES_TEAR && iabs(RSL_PX(rg.es[e].x) - px()) < 14)
                    m = (m & ~(BTN_LEFT | BTN_RIGHT)) | (RSL_PX(rg.es[e].x) > px() ? BTN_LEFT : BTN_RIGHT);
        }
        break;
    default: break;
    }
    return dodge(m);
}

/* ------------------------------------------------------------------------ */

static int bot_core(void);

int rsl_bot_last;

int rsl_bot_buttons(void) {
    int m = bot_core();
    last_out = (uint32_t)m;
    rsl_bot_last = m;
    return m;
}

static int bot_core(void) {
    rg.bot_t++;
    int edge = rg.bot_t & 1; /* menus: a press every other frame */
    switch (rg.state) {
    case RS_TITLE: rg.menu = rg.menu ? rg.menu : 0; return edge && rg.state_t > 10 ? BTN_A : 0;
    case RS_STORY: return edge && rg.state_t > 30 ? BTN_A : 0;
    case RS_SPIRIT: return spirit_buttons();
    case RS_TALLY: return edge && rg.state_t > 70 ? BTN_A : 0;
    case RS_OVER: return edge && rg.state_t > 110 ? BTN_A : 0;
    case RS_NAME: return edge ? BTN_A : 0;
    case RS_SCORES: return edge && rg.state_t > 30 ? BTN_A : 0;
    case RS_ENDING: return BTN_A;
    case RS_PLAY: break;
    default: return 0;
    }
    if (rg.boss_on && !rg.boss_dead) return boss_buttons();
    if (rg.boss_dead) {
        /* gather what the boss left */
        int tx = 0;
        if (want_item(&tx) >= 0) return go_to(tx, 2);
        return 0;
    }
    if (rg.pit >= 0) {
        const RslHalf *H = &RSL_HALF[rg.half];
        if (H->treasure_pit == rg.pit) {
            /* light every torch, take everything, then out */
            int tx = 0;
            if (want_item(&tx) >= 0) return go_to(tx, 2);
            for (int i = 0; i < rg.nspawn; i++) {
                const RslSpawn *s = &rg.spawn[i];
                if (s->type == SP_TORCH && !s->used && s->ty >= rsl_main_h() && s->tx / RSL_SW == rg.pit) {
                    int sx = s->tx * RSL_TILE + 8;
                    int dx = sx - px();
                    if (iabs(dx) > 40) return go_to(sx - (dx > 0 ? 30 : -30), 2);
                    return shoot_at(sx, s->ty * RSL_TILE + 4);
                }
            }
            for (int i = 0; i < rg.nspawn; i++)
                if (rg.spawn[i].type == SP_SECRET && rg.spawn[i].a == 'Z' && rg.spawn[i].tx / RSL_SW == rg.pit)
                    return go_to(rg.spawn[i].tx * RSL_TILE + 8, 1);
            return 0;
        }
        int fdx = 0, fdy = 0;
        int fi = nearest_foe(&fdx, &fdy, false, 400);
        if (fi >= 0) {
            const RslFoe *f = &rg.foe[fi];
            int m = aim_at(f);
            int reach = rg.pl.weapon == WP_EMBER ? 120 : rg.pl.weapon == WP_SCATTER ? 44 : 56;
            if (!(m & (BTN_LEFT | BTN_RIGHT))) {
                if (iabs(fdx) > reach || !(m & BTN_A)) m |= fdx > 0 ? BTN_RIGHT : BTN_LEFT;
                if (iabs(fdx) < 26 && iabs(fdy) < 24) m = (m & ~(BTN_LEFT | BTN_RIGHT | BTN_DOWN)) | (fdx > 0 ? BTN_LEFT : BTN_RIGHT);
            }
            return dodge(m);
        }
        int tx = 0;
        if (want_item(&tx) >= 0) return go_to(tx, 2);
        return 0;
    }
    if (rsl_bot_plan == 1 && rg.deaths < 3 && rg.half == 0 && rg.pl.inv == 0 && rg.state_t > 60) {
        /* the cherry plan: die early on purpose, for the richer world */
        int fdx = 0, fdy = 0;
        int fi = nearest_foe(&fdx, &fdy, false, 200);
        if (fi >= 0) return iabs(fdx) > 2 ? (fdx > 0 ? BTN_RIGHT : BTN_LEFT) : 0;
    }
    return walk_buttons();
}
