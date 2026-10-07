/* HOOPLA - one match: the rules every fighter shares. The hoops (held,
 * fresh and on fire, knocked loose), hits and blocks, melee against shots,
 * blasts, and who wins. Each fighter's own moves are in hoop_fighters.c. */
#include "hoop.h"

extern Rng hoop_rng;

void hoop_sfx(const char *name) { sfx_play_name(name); }

bool hoop_touch(const Fighter *f, int x, int y, int w, int h) {
    return rects_overlap(HPX(f->x), HPX(f->y), HOOP_FW, HOOP_FH, x, y, w, h);
}

int hoop_feet_y(const Fighter *f) { return f->g > 0 ? HPX(f->y) + HOOP_FH : HPX(f->y); }

bool hoop_on_ledge(const Fighter *f) { return f->ground >= 0 && f->ground < HOOP_MAX_LEDGES; }

void hoop_burst(int x, int y, int col, int n) {
    Match *m = &hg.m;
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < HOOP_MAX_PARTS; i++) {
            Part *p = &m->part[i];
            if (p->life > 0) continue;
            int a = (k * 72 / (n > 0 ? n : 1) + m->t * 7) % 72;
            int sp = 1 + (k % 3);
            p->x = x * HQ;
            p->y = y * HQ;
            p->vx = HOOP_COS[a] * sp / 2;
            p->vy = HOOP_SIN[a] * sp / 2;
            p->life = 14 + (k % 5) * 3;
            p->col = col;
            break;
        }
    }
}

int hoop_ring_rule_default(void) {
    return hsv.to_win == HOOP_DEF_WIN && hsv.start_rings == HOOP_DEF_START && hsv.every == HOOP_DEF_EVERY &&
           hsv.burn == HOOP_DEF_BURN;
}

/* ---- starting a match --------------------------------------------------- */

static int last_arena = -1;

void hoop_match_begin(const int *kinds, const int *pals, const int *ctrls, int n, int skill) {
    Match *m = &hg.m;
    memset(m, 0, sizeof *m);
    rng_seed(&m->rng, (uint64_t)rng_next(&hoop_rng) * 2654435761u + 17u);
    do m->arena = rng_range(&m->rng, 0, HOOP_ARENAS - 1);
    while (m->arena == last_arena);
    last_arena = m->arena;
    m->pal = rng_range(&m->rng, 0, HOOP_PALS - 1);
    m->n = iclamp(n, 2, HOOP_MAX_IN);
    m->winner = -1;
    m->ready_t = 90;
    m->old_rules = hg.old_rules;
    if (hg.state == HS_ATTRACT) {
        m->to_win = HOOP_DEF_WIN;
        m->start_rings = HOOP_DEF_START;
        m->every = HOOP_DEF_EVERY;
        m->burn = HOOP_DEF_BURN;
    } else {
        m->to_win = hsv.to_win;
        m->start_rings = hsv.start_rings;
        m->every = hsv.every;
        m->burn = hsv.burn;
    }
    hoop_ledges_place(m);
    const ArenaDef *a = &HOOP_ARENA[m->arena];
    for (int i = 0; i < m->n; i++) {
        Fighter *f = &m->f[i];
        memset(f, 0, sizeof *f);
        f->on = true;
        f->kind = kinds[i];
        f->pal = pals[i];
        f->ctrl = ctrls[i];
        f->x = (a->start[i][0] - HOOP_FW / 2) * HQ;
        f->y = (a->start[i][1] - HOOP_FH) * HQ;
        f->g = 1;
        f->face = a->start[i][0] < 160 ? 1 : -1;
        if (i == 2) f->face = hg.frame_t & 1 ? 1 : -1;
        f->ground = GND_FLOOR;
        f->rings = m->start_rings;
        f->hp = HOOP_OLD_HP;
        f->held_bomb = -1;
        f->claw_ring = -1;
        f->jumps = 1;
        f->lift_skip = -1;
        hoop_brain_init(&m->brain[i], ctrls[i] == CTRL_CPU ? skill : SK_ACE, i);
    }
    for (int i = 0; i < HOOP_MAX_RINGS; i++) m->ring[i].claw = -1;
    m->spawn_t = 0;
}

bool hoop_match_over(void) { return hg.m.winner >= 0 && hg.m.win_t > 150; }

/* ---- hoops ------------------------------------------------------------------ */

static int free_ring(void) {
    Match *m = &hg.m;
    for (int i = 0; i < HOOP_MAX_RINGS; i++)
        if (!m->ring[i].alive) return i;
    return -1;
}

int hoop_spawn_ring(void) {
    Match *m = &hg.m;
    const ArenaDef *a = &HOOP_ARENA[m->arena];
    int r = free_ring();
    if (r < 0) return -1;
    /* a random spot without a fresh hoop on it already */
    int spot = rng_range(&m->rng, 0, HOOP_SPOTS - 1);
    for (int tries = 0; tries < HOOP_SPOTS; tries++) {
        int s = (spot + tries) % HOOP_SPOTS;
        bool busy = false;
        for (int i = 0; i < HOOP_MAX_RINGS; i++) {
            const Ring *q = &m->ring[i];
            if (q->alive && q->state == RG_FRESH && iabs(HPX(q->x) - a->spot[s][0]) < 12 &&
                iabs(HPX(q->y) - a->spot[s][1]) < 12)
                busy = true;
        }
        if (!busy) { spot = s; break; }
    }
    Ring *g = &m->ring[r];
    memset(g, 0, sizeof *g);
    g->alive = 1;
    g->state = RG_FRESH;
    g->x = a->spot[spot][0] * HQ;
    g->y = a->spot[spot][1] * HQ;
    g->burn_t = m->burn ? 150 : 0;
    g->claw = -1;
    hoop_sfx("hoop_spawn");
    return r;
}

void hoop_drop_rings(int who, int n) {
    Match *m = &hg.m;
    Fighter *f = &m->f[who];
    for (int k = 0; k < n && f->rings > 0; k++) {
        int r = free_ring();
        if (r < 0) break;
        f->rings--;
        Ring *g = &m->ring[r];
        memset(g, 0, sizeof *g);
        g->alive = 1;
        g->state = RG_LOOSE;
        g->x = f->x + HOOP_FW * HQ / 2;
        g->y = f->y + HOOP_FH * HQ / 2;
        /* up and out, anywhere from 20 to 160 degrees above level */
        int a = rng_range(&m->rng, 40, 68);
        int sp = 820 + rng_range(&m->rng, 0, 200);
        g->vx = HOOP_COS[a] * sp / 256;
        g->vy = HOOP_SIN[a] * sp / 256;
        g->nograb_t = HOOP_NOGRAB_T;
        g->claw = -1;
    }
}

void hoop_take_ring(int who, int r) {
    Match *m = &hg.m;
    Fighter *f = &m->f[who];
    m->ring[r].alive = 0;
    f->rings++;
    f->rings_taken++;
    hoop_sfx("hoop_ring");
    hoop_burst(HPX(m->ring[r].x), HPX(m->ring[r].y), C_YELLOW, 5);
    if (!m->old_rules && f->rings >= m->to_win && m->winner < 0) {
        m->winner = who;
        m->win_t = 0;
    }
}

static void ring_bounce_ledges(Ring *g, int oldy) {
    Match *m = &hg.m;
    if (g->vy <= 0) return;
    int x = HPX(g->x);
    for (int i = 0; i < m->nledges; i++) {
        const Ledge *l = &m->ledge[i];
        if (x < l->x || x >= l->x + l->w) continue;
        int top = l->y * HQ - 4 * HQ;
        if (oldy <= top && g->y >= top) {
            g->y = top;
            g->vy = -imax(g->vy * 85 / 100, 560);
        }
    }
}

static void rings_update(void) {
    Match *m = &hg.m;
    if (!m->old_rules) {
        if (m->spawn_t <= 0) {
            hoop_spawn_ring();
            m->spawn_t = m->every * 60;
        }
        m->spawn_t--;
    }
    for (int i = 0; i < HOOP_MAX_RINGS; i++) {
        Ring *g = &m->ring[i];
        if (!g->alive) continue;
        g->t++;
        if (g->claw >= 0) continue; /* riding a claw: the claw moves it */
        if (g->state == RG_FRESH) {
            if (g->burn_t > 0) g->burn_t--;
        } else {
            if (g->nograb_t > 0) g->nograb_t--;
            int oldy = g->y;
            g->vy += 30;
            if (g->vy > 900) g->vy = 900;
            g->x += g->vx;
            g->y += g->vy;
            if (g->x < (HOOP_AL + 5) * HQ) { g->x = (HOOP_AL + 5) * HQ; g->vx = iabs(g->vx); }
            if (g->x > (HOOP_AR - 5) * HQ) { g->x = (HOOP_AR - 5) * HQ; g->vx = -iabs(g->vx); }
            if (g->y < (HOOP_AT + 5) * HQ) { g->y = (HOOP_AT + 5) * HQ; g->vy = iabs(g->vy); }
            if (g->y > (HOOP_AB - 5) * HQ) {
                g->y = (HOOP_AB - 5) * HQ;
                g->vy = -imax(iabs(g->vy) * 85 / 100, 600);
            }
            ring_bounce_ledges(g, oldy);
        }
        /* touching a fighter */
        int rx = HPX(g->x) - 5, ry = HPX(g->y) - 5;
        for (int k = 0; k < m->n; k++) {
            Fighter *f = &m->f[k];
            if (!f->on || !hoop_touch(f, rx, ry, 10, 10)) continue;
            if (g->state == RG_FRESH && g->burn_t > 0) {
                if (f->inv_t == 0) hoop_hurt(k, -1, false);
                continue;
            }
            if (g->state == RG_LOOSE && g->nograb_t > 0) continue;
            hoop_take_ring(k, i);
            break;
        }
    }
}

/* ---- hits ----------------------------------------------------------------- */

void hoop_hurt(int victim, int attacker, bool melee) {
    Match *m = &hg.m;
    Fighter *f = &m->f[victim];
    if (!f->on || f->inv_t > 0 || m->winner >= 0) return;
    int n = f->dizzy_t > 0 ? 2 : 1;
    if (m->old_rules) {
        f->hp -= n;
        if (f->hp < 0) f->hp = 0;
    } else {
        hoop_drop_rings(victim, n);
    }
    f->hurt_t = HOOP_HURT_T;
    f->inv_t = HOOP_INV_T;
    f->dizzy_t = 0;
    int dir = attacker >= 0 ? (m->f[attacker].x < f->x ? 1 : -1) : -f->face;
    f->vx = dir * 2 * HQ;
    f->vy = -460 * f->g;
    f->ground = GND_AIR;
    f->melee_t = 0;
    f->swing_t = 0;
    f->glow_jump = 0;
    f->charge_t = 0;
    f->thrust_t = 0;
    f->hover_t = 0;
    f->aiming = 0;
    f->lift = 0;
    if (f->claw) {
        f->claw = 0;
        if (f->claw_ring >= 0) { m->ring[f->claw_ring].claw = -1; f->claw_ring = -1; }
    }
    if (f->held_bomb >= 0) {
        Shot *b = &m->shot[f->held_bomb];
        b->held = 0;
        b->vx = 0;
        b->vy = 0;
        f->held_bomb = -1;
    }
    if (attacker >= 0) m->f[attacker].hits_dealt++;
    hoop_sfx(n == 2 ? "hoop_hit2" : "hoop_hit");
    hoop_burst(HPX(f->x) + HOOP_FW / 2, HPX(f->y) + HOOP_FH / 2, n == 2 ? C_RED : C_WHITE, 6);
    m->shake = n == 2 ? 8 : 4;
    (void)melee;
    if (m->old_rules && f->hp == 0) {
        f->on = false;
        int alive = 0, last = -1;
        for (int k = 0; k < m->n; k++)
            if (m->f[k].on) { alive++; last = k; }
        if (alive == 1) { m->winner = last; m->win_t = 0; }
    }
}

/* the melee box: the body pushed out the way it faces */
static void melee_box(const Fighter *f, int *x, int *y, int *w, int *h) {
    *x = HPX(f->x) + (f->face > 0 ? 2 : -8);
    *y = HPX(f->y) + 1;
    *w = HOOP_FW + 6;
    *h = HOOP_FH - 2;
    if (f->swing_t > 0 || f->glow_jump) { /* the whole body, a bit bigger */
        *x = HPX(f->x) - 3;
        *y = HPX(f->y) - 3;
        *w = HOOP_FW + 6;
        *h = HOOP_FH + 6;
    }
}

bool hoop_melee_on(const Fighter *f) { return f->melee_t > 0 || f->swing_t > 0 || f->glow_jump; }

static void melee_update(void) {
    Match *m = &hg.m;
    for (int a = 0; a < m->n; a++) {
        Fighter *f = &m->f[a];
        if (!f->on || !hoop_melee_on(f) || f->melee_hit) continue;
        int x, y, w, h;
        melee_box(f, &x, &y, &w, &h);
        for (int v = 0; v < m->n; v++) {
            if (v == a) continue;
            Fighter *t = &m->f[v];
            if (!t->on || !hoop_touch(t, x, y, w, h) || t->inv_t > 0) continue;
            /* CLAMP mid-swing on his line blocks for free */
            bool swinging = t->kind == HF_CLAMP && t->claw == 2 && (t->swing_t > 0 || iabs(t->vx) > 160);
            if (t->blocking || swinging) {
                /* the block holds and the attacker reels */
                f->melee_t = 0;
                f->swing_t = 0;
                f->glow_jump = 0;
                f->dizzy_t = HOOP_DIZZY_T;
                f->vx = (f->x < t->x ? -1 : 1) * 300;
                if (f->claw) {
                    f->claw = 0;
                    if (f->claw_ring >= 0) { m->ring[f->claw_ring].claw = -1; f->claw_ring = -1; }
                }
                f->melee_hit = 1;
                hoop_sfx("hoop_block");
                hoop_burst(HPX(t->x) + HOOP_FW / 2, HPX(t->y) + 4, C_CYAN, 6);
                break;
            }
            hoop_hurt(v, a, true);
            f->melee_hit = 1;
            break;
        }
    }
}

static void blasts_update(void) {
    Match *m = &hg.m;
    for (int i = 0; i < HOOP_MAX_BLASTS; i++) {
        Blast *b = &m->blast[i];
        if (!b->alive) continue;
        b->t++;
        if (b->t <= 6) {
            for (int k = 0; k < m->n; k++) {
                Fighter *f = &m->f[k];
                if (!f->on || (k == b->owner && !b->hurts_owner) || (b->done & (1 << k))) continue;
                int cx = HPX(f->x) + HOOP_FW / 2, cy = HPX(f->y) + HOOP_FH / 2;
                int dx = cx - b->x, dy = cy - b->y;
                int rr = b->r + 5;
                if (dx * dx + dy * dy <= rr * rr) {
                    b->done |= 1 << k;
                    hoop_hurt(k, k == b->owner ? -1 : b->owner, false);
                }
            }
        }
        if (b->t > 18) b->alive = 0;
    }
}

void hoop_add_blast(int owner, int x, int y, int r, bool hurts_owner) {
    Match *m = &hg.m;
    for (int i = 0; i < HOOP_MAX_BLASTS; i++) {
        Blast *b = &m->blast[i];
        if (b->alive) continue;
        memset(b, 0, sizeof *b);
        b->alive = 1;
        b->owner = (uint8_t)owner;
        b->hurts_owner = hurts_owner;
        b->x = x;
        b->y = y;
        b->r = r;
        hoop_sfx("hoop_boom");
        m->shake = 6;
        hoop_burst(x, y, C_ORANGE, 8);
        return;
    }
}

static void parts_update(void) {
    Match *m = &hg.m;
    for (int i = 0; i < HOOP_MAX_PARTS; i++) {
        Part *p = &m->part[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vy += 12;
    }
}

/* ---- the buttons ----------------------------------------------------------- */

static int read_pad(int i) {
    Match *m = &hg.m;
    Fighter *f = &m->f[i];
    int b = 0;
    if (f->ctrl == CTRL_CPU) return hoop_brain_think(i);
    if (f->ctrl == CTRL_P1) {
        b |= btn(BTN_LEFT) ? HP_L : 0;
        b |= btn(BTN_RIGHT) ? HP_R : 0;
        b |= btn(BTN_UP) ? HP_U : 0;
        b |= btn(BTN_DOWN) ? HP_D : 0;
        b |= btn(BTN_A) ? HP_A : 0;
        b |= btn(BTN_B) ? HP_B : 0;
    } else {
        b |= btn2(BTN_LEFT) ? HP_L : 0;
        b |= btn2(BTN_RIGHT) ? HP_R : 0;
        b |= btn2(BTN_UP) ? HP_U : 0;
        b |= btn2(BTN_DOWN) ? HP_D : 0;
        b |= btn2(BTN_A) ? HP_A : 0;
        b |= btn2(BTN_B) ? HP_B : 0;
    }
    /* the antigravity option: Moss's left and right turn over with him */
    if (f->kind == HF_MOSS && f->g < 0 && hsv.antigrav) {
        int l = b & HP_L, r = b & HP_R;
        b &= ~(HP_L | HP_R);
        if (l) b |= HP_R;
        if (r) b |= HP_L;
    }
    return b;
}

void hoop_match_update(void) {
    Match *m = &hg.m;
    if (m->shake > 0) m->shake--;
    parts_update();
    if (m->winner >= 0) {
        m->win_t++;
        for (int i = 0; i < m->n; i++) m->f[i].anim_t++;
        return;
    }
    if (m->ready_t > 0) {
        m->ready_t--;
        if (m->ready_t == 0) hoop_sfx("hoop_go");
        for (int i = 0; i < m->n; i++) {
            Fighter *f = &m->f[i];
            f->held = f->ctrl == CTRL_CPU ? 0 : read_pad(i);
            f->prev = f->held;
        }
        return;
    }
    m->t++;
    hoop_ledges_update(m);
    for (int i = 0; i < m->n; i++) {
        Fighter *f = &m->f[i];
        f->prev = f->held;
        f->held = f->on ? read_pad(i) : 0;
    }
    for (int i = 0; i < m->n; i++) {
        if (!m->f[i].on) continue;
        hoop_fighter_act(i);
        hoop_fighter_physics(i);
    }
    hoop_shots_update();
    hoop_mines_update();
    melee_update();
    blasts_update();
    rings_update();
}
