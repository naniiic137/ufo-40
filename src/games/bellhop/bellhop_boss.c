/* BELLHOP - the five bosses, one at the end of each world (X-10).
 *
 *   A  THE MILLWHEEL   six buckets on a turning wheel. One bucket at a time
 *                      spits pellets; each takes two slashes, and the wheel
 *                      turns faster for every bucket gone.
 *   B  THE CIDER PRESS slash the apple into the press's chute, three times:
 *                      once in the open, once past thorns, and once while a
 *                      sprinkler's jet knocks the apple dead in the air.
 *   C  THE TWIN COGS   two cogs ringed with lamps. Only one cog's lamps are
 *                      lit at a time (and only lit lamps break); when they
 *                      are all out, the other cog lights up. Lamps shoot.
 *   D  THE GUMBALL MACHINE  rounds of gumballs, each gone in one slash; the
 *                      slashed piece flies off and bursts into shrapnel where
 *                      it lands, and a piece that lands on the lid makes the
 *                      machine spray. Last a giant that splits and splits.
 *   E  LADY HUSH       sprays of shots and lobbed spike balls. Red ones can't
 *                      be touched; a blue one slashed flies back at her, the
 *                      only thing that hurts her. Slashing her ship fills the
 *                      tank but does her no harm.
 *
 * A boss and all its parts start again when the ship is wrecked. */
#include "bellhop.h"

const int BHP_MILL_X = 160, BHP_MILL_Y = 94, BHP_MILL_R = 44;
const int BHP_COG_X[2] = {92, 228}, BHP_COG_Y = 96, BHP_COG_R = 22, BHP_LAMP_R = 32;
const int BHP_CHUTE_X0 = 296, BHP_CHUTE_X1 = 311, BHP_CHUTE_Y0 = 12 + 8 * 8, BHP_CHUTE_Y1 = 12 + 11 * 8 - 1;
const int BHP_LID_X = 160, BHP_LID_Y = 12 + 14 * 8 - 3;

#define HUSH_HP 8
#define APPLE_X 96
#define APPLE_Y 120

static int PX(int32_t v) { return (int)(v >> 8); }

void bhp_mill_bucket_pos(const BhpStage *s, int k, int *x, int *y) {
    int a = (int)((s->boss.ang >> 8) + k * 256 / BHP_MILL_N) & 255;
    *x = BHP_MILL_X + bhp_cos(a) * BHP_MILL_R / 127;
    *y = BHP_MILL_Y + bhp_sin(a) * BHP_MILL_R / 127 + 4; /* buckets hang below the rim */
}

void bhp_cog_lamp_pos(const BhpStage *s, int wheel, int k, int *x, int *y) {
    int a = (int)((wheel ? -s->boss.ang : s->boss.ang) >> 8) + k * 256 / BHP_COG_N + wheel * 25;
    a &= 255;
    *x = BHP_COG_X[wheel] + bhp_cos(a) * BHP_LAMP_R / 127;
    *y = BHP_COG_Y + bhp_sin(a) * BHP_LAMP_R / 127;
}

static void boss_down(BhpStage *s, int pts) {
    if (s->boss.down) return;
    s->boss.down = 1;
    s->boss.down_t = 0;
    s->pts += pts;
    s->ev |= BEV_BOSS_DOWN;
    for (int i = 0; i < s->ne; i++)
        if (s->e[i].kind >= BEK_BUCKET && s->e[i].on) {
            s->e[i].on = 0;
            bhp_add_fx(s, PX(s->e[i].x), PX(s->e[i].y), 8);
        }
    for (int k = 0; k < BHP_SHOTS; k++) s->shot[k].on = 0;
}

static void reset_apple(BhpStage *s) {
    for (int i = 0; i < s->ne; i++)
        if (s->e[i].kind == BEK_APPLE) {
            s->e[i].x = APPLE_X * CHF_ONE;
            s->e[i].y = APPLE_Y * CHF_ONE;
            s->e[i].vx = s->e[i].vy = 0;
        }
}

/* the press's thorns (phase 2 on) */
static void press_thorns(BhpStage *s) {
    for (int r = 13; r <= 19; r++) bhp_set_tile(s, 15, r, BTL_THORN);
    for (int r = 1; r <= 6; r++) bhp_set_tile(s, 25, r, BTL_THORN);
}

static void gum_round(BhpStage *s) {
    BhpBoss *b = &s->boss;
    b->round++;
    b->left = b->round <= 3 ? (uint8_t)(b->round + 2) : 1;
    b->t2 = 0;
    b->t = 0;
}

void bhp_boss_init(BhpStage *s) {
    BhpBoss *b = &s->boss;
    memset(b, 0, sizeof *b);
    b->kind = (uint8_t)(s->idx / BHP_PER_WORLD + 1);
    switch (b->kind) {
    case 1:
        for (int k = 0; k < BHP_MILL_N; k++) {
            int i = bhp_ent_add(s, BEK_BUCKET, BHP_MILL_X, BHP_MILL_Y);
            if (i >= 0) { s->e[i].a = (int16_t)k; s->e[i].hp = 2; }
        }
        b->spin = 90;
        break;
    case 2:
        bhp_ent_add(s, BEK_APPLE, APPLE_X, APPLE_Y);
        b->phase = 0;
        break;
    case 3:
        for (int w = 0; w < 2; w++)
            for (int k = 0; k < BHP_COG_N; k++) {
                int i = bhp_ent_add(s, BEK_LAMP, BHP_COG_X[w], BHP_COG_Y);
                if (i >= 0) { s->e[i].a = (int16_t)w; s->e[i].b = (int16_t)k; s->e[i].flag = w == 0; }
            }
        b->spin = 110;
        break;
    case 4:
        gum_round(s);
        break;
    case 5:
        b->hp = HUSH_HP;
        b->x = 160 * CHF_ONE;
        b->y = 46 * CHF_ONE;
        break;
    }
}

/* ---- each boss's frame ---------------------------------------------------------- */

static void step_mill(BhpStage *s) {
    BhpBoss *b = &s->boss;
    int alive = 0, first = -1;
    for (int i = 0; i < s->ne; i++)
        if (s->e[i].on && s->e[i].kind == BEK_BUCKET) { alive++; if (first < 0) first = s->e[i].a; }
    b->spin = 90 + 45 * (BHP_MILL_N - alive);
    b->ang += b->spin;
    for (int i = 0; i < s->ne; i++) {
        BhpEnt *e = &s->e[i];
        if (!e->on || e->kind != BEK_BUCKET) continue;
        int x, y;
        bhp_mill_bucket_pos(s, e->a, &x, &y);
        e->x = x * CHF_ONE;
        e->y = y * CHF_ONE;
    }
    /* the spitting bucket: the next one still there, every two and a half seconds */
    b->t++;
    if (b->t >= 150 || first < 0) {
        b->t = 0;
        for (int k = 1; k <= BHP_MILL_N; k++) {
            int n = (b->active + k) % BHP_MILL_N;
            bool there = false;
            for (int i = 0; i < s->ne; i++)
                if (s->e[i].on && s->e[i].kind == BEK_BUCKET && s->e[i].a == n) there = true;
            if (there) { b->active = (uint8_t)n; break; }
        }
    }
    if (b->t == 50 || b->t == 85 || b->t == 120) {
        for (int i = 0; i < s->ne; i++) {
            BhpEnt *e = &s->e[i];
            if (!e->on || e->kind != BEK_BUCKET || e->a != b->active) continue;
            int a = (int)((b->ang >> 8) + e->a * 256 / BHP_MILL_N) & 255;
            for (int d = -1; d <= 1; d++) {
                int aa = a + d * 18;
                bhp_shot_add(s, BSH_PELLET, e->x, e->y, bhp_cos(aa) * 256 / 127, bhp_sin(aa) * 256 / 127);
            }
            s->ev |= BEV_SHOOT;
        }
    }
    if (alive == 0) boss_down(s, 1000);
}

static void step_press(BhpStage *s) {
    BhpBoss *b = &s->boss;
    b->t++;
    for (int i = 0; i < s->ne; i++) {
        BhpEnt *e = &s->e[i];
        if (!e->on || e->kind != BEK_APPLE) continue;
        e->vy = imin(e->vy + 12, 512);
        int32_t nx = e->x + e->vx;
        if (bhp_solid(s, PX(nx) + (e->vx > 0 ? 6 : -6), PX(e->y)) || bhp_solid(s, PX(nx) + (e->vx > 0 ? 6 : -6), PX(e->y) + 4) ||
            bhp_solid(s, PX(nx) + (e->vx > 0 ? 6 : -6), PX(e->y) - 4))
            e->vx = -e->vx * 3 / 5;
        else e->x = nx;
        int32_t ny = e->y + e->vy;
        int edge = e->vy > 0 ? 6 : -6;
        if (bhp_solid(s, PX(e->x), PX(ny) + edge) || bhp_solid(s, PX(e->x) - 4, PX(ny) + edge) || bhp_solid(s, PX(e->x) + 4, PX(ny) + edge)) {
            if (e->vy > 0 && e->vy < 120) { e->vy = 0; e->vx = e->vx * 15 / 16; }
            else e->vy = -e->vy / 2;
        } else e->y = ny;
        /* into the chute */
        int x = PX(e->x), y = PX(e->y);
        if (x >= BHP_CHUTE_X0 + 2 && y >= BHP_CHUTE_Y0 && y <= BHP_CHUTE_Y1) {
            b->phase++;
            b->t2++;
            b->t = 0;
            s->pts += 500;
            s->ev |= BEV_DUNK | BEV_BOSS_HIT;
            bhp_add_fx(s, x, y, 9);
            reset_apple(s);
            if (b->phase == 1) press_thorns(s);
            if (b->phase == 2) {
                int k = bhp_ent_add(s, BEK_SPRINKLER, 160, BHP_OY + 19 * BHP_T + 3);
                (void)k;
            }
            if (b->phase >= 3) boss_down(s, 1000);
            return;
        }
        /* the sprinkler's jet stops the apple dead */
        for (int k = 0; k < BHP_SHOTS; k++) {
            BhpShot *j = &s->shot[k];
            if (!j->on || j->kind != BSH_JET) continue;
            if (iabs(PX(j->x) - x) <= 7 && iabs(PX(j->y) - y) <= 7) {
                j->on = 0;
                e->vx = e->vy = 0;
                bhp_add_fx(s, x, y, 15);
            }
        }
        if (b->phase >= 2 && b->t % 150 == 75) {
            for (int k = 0; k < s->ne; k++)
                if (s->e[k].on && s->e[k].kind == BEK_SPRINKLER) {
                    int32_t vx, vy;
                    bhp_aim(s->e[k].x, s->e[k].y - 6 * CHF_ONE, e->x, e->y, 768, &vx, &vy);
                    bhp_shot_add(s, BSH_JET, s->e[k].x, s->e[k].y - 6 * CHF_ONE, vx, vy);
                    s->ev |= BEV_SHOOT;
                }
        }
    }
}

static void step_cogs(BhpStage *s) {
    BhpBoss *b = &s->boss;
    b->ang += b->spin;
    int open = 0, total = 0;
    for (int i = 0; i < s->ne; i++) {
        BhpEnt *e = &s->e[i];
        if (e->kind != BEK_LAMP || !e->on) continue;
        int x, y;
        bhp_cog_lamp_pos(s, e->a, e->b, &x, &y);
        e->x = x * CHF_ONE;
        e->y = y * CHF_ONE;
        total++;
        if (e->a == b->active) open++;
    }
    if (total == 0) { boss_down(s, 1000); return; }
    if (open == 0) {
        b->active ^= 1;
        for (int i = 0; i < s->ne; i++)
            if (s->e[i].on && s->e[i].kind == BEK_LAMP && s->e[i].a == b->active) s->e[i].flag = 1;
        s->ev |= BEV_ROUND;
    }
    b->t++;
    if (b->t % 80 == 0 && s->mode == BSM_FLY) {
        /* the next lit lamp along shoots at the ship */
        int n = 0, pick = (b->t / 80) % BHP_COG_N;
        for (int k = 0; k < BHP_COG_N && n == 0; k++)
            for (int i = 0; i < s->ne; i++) {
                BhpEnt *e = &s->e[i];
                if (e->on && e->kind == BEK_LAMP && e->flag == 1 && e->b == (pick + k) % BHP_COG_N) {
                    int32_t vx, vy;
                    bhp_aim(e->x, e->y, s->f.x, s->f.y, 256, &vx, &vy);
                    bhp_shot_add(s, BSH_PELLET, e->x, e->y, vx, vy);
                    s->ev |= BEV_SHOOT;
                    n = 1;
                    break;
                }
            }
    }
}

static void shrapnel(BhpStage *s, int32_t x, int32_t y) {
    static const int16_t V[3][2] = {{-200, -260}, {0, -320}, {200, -260}};
    for (int k = 0; k < 3; k++) bhp_shot_add(s, BSH_PELLET, x, y, V[k][0], V[k][1]);
    s->ev |= BEV_SHOOT;
    bhp_add_fx(s, PX(x), PX(y), 4);
}

static void step_gums(BhpStage *s) {
    BhpBoss *b = &s->boss;
    b->t++;
    /* launch this round's gumballs, half a second apart */
    if (b->left && b->t >= 40 && (b->t - 40) % 30 == 0) {
        int i = bhp_ent_add(s, BEK_GUM, BHP_LID_X, BHP_LID_Y - 8);
        if (i >= 0) {
            BhpEnt *e = &s->e[i];
            e->size = b->round >= 4 ? 2 : 0;
            int side = (b->left & 1) ? 1 : -1;
            e->vx = side * rng_range(&s->rng, 100, 260);
            e->vy = -rng_range(&s->rng, 480, 600);
        }
        b->left--;
        s->ev |= BEV_SHOOT;
    }
    int gums = 0;
    for (int i = 0; i < s->ne; i++) {
        BhpEnt *e = &s->e[i];
        if (!e->on) continue;
        if (e->kind == BEK_GUM) {
            gums++;
            int r = e->size == 2 ? 11 : e->size == 1 ? 7 : 5;
            e->vy = imin(e->vy + 3, 220);
            int32_t nx = e->x + e->vx;
            if (bhp_solid(s, PX(nx) + (e->vx > 0 ? r : -r), PX(e->y))) e->vx = -e->vx;
            else e->x = nx;
            int32_t ny = e->y + e->vy;
            if (bhp_solid(s, PX(e->x), PX(ny) + (e->vy > 0 ? r : -r))) {
                if (e->vy > 0) e->vy = -imax(e->vy, 300);
                else e->vy = -e->vy;
            } else e->y = ny;
        } else if (e->kind == BEK_CHUNK) {
            e->vy = imin(e->vy + 14, 600);
            e->x += e->vx;
            e->y += e->vy;
            int x = PX(e->x), y = PX(e->y);
            if (iabs(x - BHP_LID_X) <= 14 && iabs(y - BHP_LID_Y) <= 6) {
                /* on the lid: the machine sprays */
                e->on = 0;
                for (int k = -2; k <= 2; k++) {
                    int a = 192 + k * 14;
                    bhp_shot_add(s, BSH_PELLET, BHP_LID_X * CHF_ONE, (BHP_LID_Y - 6) * CHF_ONE, bhp_cos(a) * 300 / 127, bhp_sin(a) * 300 / 127);
                }
                s->ev |= BEV_SHOOT;
            } else if (bhp_solid(s, x, y)) {
                e->on = 0;
                shrapnel(s, e->x - e->vx, e->y - e->vy);
            }
        }
    }
    if (!b->left && gums == 0 && b->t > 60) {
        if (b->round >= 4) boss_down(s, 1000);
        else if (!b->down_t) b->down_t = 1;
    }
    if (b->down_t && !b->down) {
        if (++b->down_t > 90) { b->down_t = 0; gum_round(s); s->ev |= BEV_ROUND; }
    }
}

static void step_hush(BhpStage *s) {
    BhpBoss *b = &s->boss;
    b->t++;
    int t = b->t;
    b->x = (160 + bhp_sin(t / 3) * 96 / 127) * CHF_ONE;
    b->y = (46 + bhp_sin(t * 2 / 5 + 30) * 12 / 127) * CHF_ONE;
    int period = b->hp > HUSH_HP / 2 ? 160 : 120;
    if (s->mode == BSM_FLY && t % period == 0) {
        int32_t vx, vy;
        bhp_aim(b->x, b->y + 6 * CHF_ONE, s->f.x, s->f.y, 282, &vx, &vy);
        int a0 = 0;
        /* turn the aimed shot by -2..2 steps of about 9 degrees */
        for (int k = -2; k <= 2; k++) {
            a0 = k * 6;
            int c = bhp_cos(a0), sn = bhp_sin(a0);
            int32_t rx = (vx * c - vy * sn) / 127, ry = (vx * sn + vy * c) / 127;
            bhp_shot_add(s, BSH_PELLET, b->x, b->y + 6 * CHF_ONE, rx, ry);
        }
        s->ev |= BEV_SHOOT;
    }
    if (s->mode == BSM_FLY && t % period == period / 2) {
        int i = bhp_ent_add(s, BEK_SPIKE, PX(b->x), PX(b->y) + 8);
        if (i >= 0) {
            BhpEnt *e = &s->e[i];
            e->flag = (b->lobs % 2 == 1); /* 1: blue, the kind that can be sent back */
            e->vx = iclamp((int)((s->f.x - b->x) / 100), -384, 384);
            e->vy = -300;
            b->lobs++;
        }
        s->ev |= BEV_SHOOT;
    }
    for (int i = 0; i < s->ne; i++) {
        BhpEnt *e = &s->e[i];
        if (!e->on || e->kind != BEK_SPIKE) continue;
        e->t++;
        e->vy = imin(e->vy + 5, 400);
        int32_t nx = e->x + e->vx;
        if (bhp_solid(s, PX(nx) + (e->vx > 0 ? 4 : -4), PX(e->y))) e->vx = -e->vx;
        else e->x = nx;
        int32_t ny = e->y + e->vy;
        if (bhp_solid(s, PX(e->x), PX(ny) + (e->vy > 0 ? 4 : -4))) e->vy = e->vy > 0 ? -imax(e->vy * 3 / 4, 260) : -e->vy;
        else e->y = ny;
        if (e->t > 480) { e->on = 0; bhp_add_fx(s, PX(e->x), PX(e->y), 3); }
    }
    /* sent-back spike balls home in on her */
    for (int k = 0; k < BHP_SHOTS; k++) {
        BhpShot *r = &s->shot[k];
        if (!r->on || r->kind != BSH_RETURN) continue;
        int32_t vx, vy;
        bhp_aim(r->x, r->y, b->x, b->y, 768, &vx, &vy);
        r->vx = (r->vx * 3 + vx) / 4;
        r->vy = (r->vy * 3 + vy) / 4;
        r->x += r->vx;
        r->y += r->vy;
        if (++r->life > 300) r->on = 0;
        if (iabs(PX(r->x) - PX(b->x)) <= 14 && iabs(PX(r->y) - PX(b->y)) <= 9) {
            r->on = 0;
            if (b->hp) b->hp--;
            b->t2++;
            s->pts += 500;
            s->ev |= BEV_BOSS_HIT;
            bhp_add_fx(s, PX(b->x), PX(b->y), 8);
            if (b->hp == 0) boss_down(s, 2000);
        }
    }
}

void bhp_boss_step(BhpStage *s) {
    BhpBoss *b = &s->boss;
    if (b->down) {
        if (b->down_t < 999 && ++b->down_t == 90) {
            s->exit_open = 1;
            s->ev |= BEV_EXIT_OPEN;
        }
        if (b->down_t < 90 && (b->down_t % 12) == 0) {
            int x = b->kind == 5 ? PX(b->x) : b->kind == 3 ? BHP_COG_X[(b->down_t / 12) & 1] : b->kind == 2 ? 300 : 160;
            int y = b->kind == 5 ? PX(b->y) : b->kind == 4 ? BHP_LID_Y : 94;
            bhp_add_fx(s, x + (b->down_t * 7) % 30 - 15, y + (b->down_t * 11) % 24 - 12, 8);
            s->ev |= BEV_BLAST;
        }
        return;
    }
    switch (b->kind) {
    case 1: step_mill(s); break;
    case 2: step_press(s); break;
    case 3: step_cogs(s); break;
    case 4: step_gums(s); break;
    case 5: step_hush(s); break;
    }
}

/* ---- the slash on a boss ---------------------------------------------------------- */

void bhp_boss_ent_slash(BhpStage *s, BhpEnt *e) {
    BhpBoss *b = &s->boss;
    switch (e->kind) {
    case BEK_BUCKET:
        b->t2++;
        bhp_refuel(s, 120);
        if (e->hp > 1) {
            e->hp--;
            s->pts += 100;
            s->ev |= BEV_BOSS_HIT;
            bhp_add_fx(s, PX(e->x), PX(e->y), 2);
        } else {
            e->on = 0;
            s->pts += 300;
            s->ev |= BEV_BOSS_HIT | BEV_KILL;
            bhp_add_fx(s, PX(e->x), PX(e->y), 8);
        }
        break;
    case BEK_APPLE: {
        int dy = PX(e->y) - PX(s->f.y);
        /* knocked away from the ship and up: the lower the ship, the higher */
        e->vx = s->f.face >= 0 ? 520 : -520;
        e->vy = iclamp(-700 + dy * 16, -820, -380);
        bhp_refuel(s, 90);
        s->ev |= BEV_HIT;
        bhp_add_fx(s, PX(e->x), PX(e->y), 2);
        break;
    }
    case BEK_LAMP:
        if (e->flag != 1) { s->ev |= BEV_HIT; break; }
        e->on = 0;
        b->t2++;
        s->pts += 200;
        bhp_refuel(s, 120);
        s->ev |= BEV_BOSS_HIT | BEV_KILL;
        bhp_add_fx(s, PX(e->x), PX(e->y), 2);
        break;
    case BEK_GUM:
        b->t2++;
        s->pts += 100;
        bhp_refuel(s, 90);
        s->ev |= BEV_BOSS_HIT;
        bhp_add_fx(s, PX(e->x), PX(e->y), 2);
        if (e->size > 0) {
            int size = e->size - 1;
            int32_t x = e->x, y = e->y;
            e->on = 0;
            for (int k = -1; k <= 1; k += 2) {
                int i = bhp_ent_add(s, BEK_GUM, PX(x) + k * 4, PX(y));
                if (i < 0) continue;
                s->e[i].size = (uint8_t)size;
                s->e[i].vx = k * 220;
                s->e[i].vy = -300;
                s->e[i].hitno = s->slash_no;
            }
        } else {
            e->kind = BEK_CHUNK;
            e->vx = s->f.face >= 0 ? 700 : -700;
            e->vy = -260;
        }
        break;
    case BEK_SPIKE:
        if (e->flag == 1) {
            e->on = 0;
            bhp_shot_add(s, BSH_RETURN, e->x, e->y, 0, -512);
            s->ev |= BEV_HIT;
            bhp_add_fx(s, PX(e->x), PX(e->y), 7);
        } else s->ev |= BEV_HIT;
        break;
    default: break;
    }
}

bool bhp_boss_slash(BhpStage *s, int x0, int y0, int x1, int y1) {
    BhpBoss *b = &s->boss;
    if (b->kind != 5 || b->down) return false;
    int x = PX(b->x), y = PX(b->y);
    if (!rects_overlap(x0, y0, x1 - x0 + 1, y1 - y0 + 1, x - 14, y - 9, 28, 18)) return false;
    if (s->fuel < BHP_FUEL_MAX) {
        s->fuel = BHP_FUEL_MAX;
        s->ev |= BEV_FUEL;
    }
    return true;
}

bool bhp_boss_hits_ship(const BhpStage *s) {
    const BhpBoss *b = &s->boss;
    if (b->down) return false;
    int sx = PX(s->f.x), sy = PX(s->f.y);
    switch (b->kind) {
    case 1: {
        int dx = sx - BHP_MILL_X, dy = sy - BHP_MILL_Y;
        return dx * dx + dy * dy < 13 * 13;
    }
    case 3:
        for (int w = 0; w < 2; w++) {
            int dx = sx - BHP_COG_X[w], dy = sy - BHP_COG_Y;
            if (dx * dx + dy * dy < (BHP_COG_R + 4) * (BHP_COG_R + 4)) return true;
        }
        return false;
    case 5:
        return bhp_ship_box_hits(s, PX(b->x) - 12, PX(b->y) - 7, PX(b->x) + 11, PX(b->y) + 6);
    default: return false;
    }
}

int bhp_boss_progress(const BhpStage *s) {
    const BhpBoss *b = &s->boss;
    if (b->down) return 100000;
    int p = b->t2 * 1000;
    if (b->kind == 2)
        for (int i = 0; i < s->ne; i++)
            if (s->e[i].on && s->e[i].kind == BEK_APPLE) {
                /* nearer the chute's mouth is better */
                int dx = PX(s->e[i].x) - BHP_CHUTE_X0, dy = PX(s->e[i].y) - (BHP_CHUTE_Y0 + BHP_CHUTE_Y1) / 2;
                p += 400 - imin(400, bhp_isqrt(dx * dx + dy * dy));
            }
    return p;
}

void bhp_boss_finish(BhpStage *s) {
    if (s->boss.kind) boss_down(s, 0);
}
