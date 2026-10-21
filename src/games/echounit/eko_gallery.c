/* ECHO UNIT - VESPER's shooting gallery: the Andes pass, and the Caldera's
 * launch pad with Madame Coil's gunship. Vesper stands at the bottom of the
 * screen and V.I.P.E.R. troops pop out of the scenery at three depths.
 * LEFT / RIGHT walk and dodge, UP / DOWN raise and lower the sight. Tap A
 * to shoot as you walk (tapping fast shoots faster); hold A and she plants
 * her feet: the pad then steers the sight. Either way she can only shoot
 * within a stretch in front of her (the sight stays within 96 px of her).
 * Standing on something on the ground, a tap of A picks it up instead: a
 * mango, or a live grenade lobbed at her, which the next tap throws back
 * at the sight before it goes off. B dives aside. Every kill feeds the
 * combo meter, which multiplies the points while it lasts. */
#include "eko.h"

#define G (&eg.ga)

const int GA_ROW_Y[GA_ROWS] = {70, 102, 140};
const int GA_ROW_SCALE[GA_ROWS] = {2, 3, 4};
const int EKO_GE_HP[GE_KINDS] = {0, 1, 1, 2, 6, 8, 1, 50, 50, 20, 30, 60, 1};
const int EKO_GE_POINTS[GE_KINDS] = {0, 10, 10, 20, 50, 50, 0, 200, 200, 100, 100, 500, 0};

static bool is_sky(int kind) {
    return kind == GE_CHOPPER || kind == GE_VULTRA || kind == GE_POD || kind == GE_CANNON || kind == GE_COIL || kind == GE_MISSILE;
}
static bool is_boss(int kind) { return kind == GE_RAMROD || kind == GE_VULTRA || kind == GE_POD || kind == GE_CANNON || kind == GE_COIL; }

/* a foe's box on the screen (px) */
static void foe_box(const EkoGFoe *f, int *x0, int *y0, int *w, int *h) {
    int x = EKO_PX(f->x), y = EKO_PX(f->y);
    if (is_sky(f->kind)) {
        int hw = 10, hh = 7;
        switch (f->kind) {
        case GE_CHOPPER: hw = 16; hh = 8; break;
        case GE_VULTRA: hw = 16; hh = 12; break;
        case GE_POD: hw = 12; hh = 9; break;
        case GE_CANNON: hw = 12; hh = 8; break;
        case GE_COIL: hw = 18; hh = 12; break;
        case GE_MISSILE: hw = 5; hh = 6; break;
        default: break;
        }
        *x0 = x - hw; *y0 = y - hh; *w = hw * 2; *h = hh * 2;
        return;
    }
    int s = GA_ROW_SCALE[f->row];
    int hw = 7 * s / 4, hh = 22 * s / 4;
    if (f->kind == GE_JEEP) { hw = 18 * s / 4; hh = 14 * s / 4; }
    if (f->kind == GE_BARREL) { hw = 5 * s / 4; hh = 10 * s / 4; }
    if (f->kind == GE_RAMROD) { hw = 14; hh = 30; }
    /* a trooper rising from cover shows only what is above it */
    if (f->kind == GE_RIFLE || f->kind == GE_MORTAR) {
        int up = f->state == 0 ? f->t * hh / 10 : f->state == 2 ? hh - f->t * hh / 10 : hh;
        hh = iclamp(up, 0, hh);
    }
    *x0 = x - hw; *y0 = y - hh; *w = hw * 2; *h = hh;
}

int eko_gallery_spawn(int kind, int row, int x, int dir) {
    for (int i = 0; i < GA_MAX_FOES; i++)
        if (!G->foe[i].alive) {
            EkoGFoe *f = &G->foe[i];
            memset(f, 0, sizeof *f);
            f->alive = 1;
            f->kind = (uint8_t)kind;
            f->row = (uint8_t)iclamp(row, 0, GA_ROWS - 1);
            f->x = x * EKO_FX;
            f->y = is_sky(kind) ? 40 * EKO_FX : GA_ROW_Y[f->row] * EKO_FX;
            f->dir = dir ? dir : (x < 160 ? 1 : -1);
            f->hp = EKO_GE_HP[kind];
            f->cd = 30 + rng_range(&eg.rng, 0, 30);
            if (kind == GE_RUNNER || kind == GE_JEEP) f->x = (dir > 0 ? -20 : SCREEN_W + 20) * EKO_FX;
            if (kind == GE_CHOPPER) { f->x = (dir > 0 ? -30 : SCREEN_W + 30) * EKO_FX; f->y = (34 + rng_range(&eg.rng, 0, 12)) * EKO_FX; }
            return i;
        }
    return -1;
}

static int add_shot(int kind, int sx, int sy, int tx, int ty, int dur) {
    for (int i = 0; i < GA_MAX_SHOTS; i++)
        if (!G->shot[i].alive) {
            EkoGShot *s = &G->shot[i];
            s->alive = 1;
            s->kind = (uint8_t)kind;
            s->sx = sx; s->sy = sy; s->tx = tx; s->ty = ty;
            s->t = 0;
            s->dur = dur;
            return i;
        }
    return -1;
}

/* a foe fires at where she stands now */
static void fire_at(const EkoGFoe *f, int off, int kind, int dur) {
    int x0, y0, w, h;
    foe_box(f, &x0, &y0, &w, &h);
    int tx = iclamp(EKO_PX(G->px) + off, GA_MINX, GA_MAXX);
    add_shot(kind, x0 + w / 2, y0 + h / 3, tx, GA_PY - 10, dur);
    eko_sfx(kind == 2 ? "eko_toss" : "eko_shot");
}

static void hurt_vesper(int dmg) {
    if (G->inv > 0 || G->dead_t || eg.god) return;
    if (G->roll_t > 2 && G->roll_t < GA_ROLL_T - 3) return; /* rolling: untouchable */
    eko_hurt(dmg);
    if (eko_hp() <= 0) { G->dead_t = 1; G->carry = 0; eko_sfx("eko_down"); return; }
    G->inv = EKO_OWL_INV[OWL_VESPER];
}

static void blast_foes(int bx, int by, int r, int boss_dmg, int dmg);

static void kill_foe(int i) {
    EkoGFoe *f = &G->foe[i];
    f->alive = 0; /* gone before any blast it sets off comes back round to it */
    int x0, y0, w, h;
    foe_box(f, &x0, &y0, &w, &h);
    eko_burst(x0 + w / 2, y0 + h / 2, f->kind == GE_BARREL ? C_ORANGE : C_YELLOW, f->kind == GE_JEEP || is_boss(f->kind) ? 18 : 8);
    if (f->kind != GE_BARREL) {
        G->combo = imin(GA_COMBO_MAX, G->combo + 1);
        G->best_combo = imax(G->best_combo, G->combo);
        G->meter = GA_COMBO_T;
    }
    /* the meter multiplies: x1, then one more for every three kills in a row */
    eko_add_score(EKO_GE_POINTS[f->kind] * (1 + G->combo / 3));
    G->kills++;
    if (f->kind == GE_JEEP) {
        /* the green truck's cargo: a mango falls to her row */
        add_shot(3, x0 + w / 2, y0, x0 + w / 2, GA_PY, 40);
    }
    if (f->kind == GE_BARREL) {
        /* the blast takes everything near it */
        eko_sfx("eko_boom");
        eg.shake = 8;
        blast_foes(EKO_PX(f->x), EKO_PX(f->y) - 8, 44, 8, 9);
    } else eko_sfx(is_boss(f->kind) ? "eko_bossdie" : "eko_ko");
}

/* a blast at (bx, by): everything within r takes dmg (a boss takes boss_dmg;
 * the gunship's core shrugs it off while it is shut) */
static void blast_foes(int bx, int by, int r, int boss_dmg, int dmg) {
    for (int k = 0; k < GA_MAX_FOES; k++) {
        EkoGFoe *o = &G->foe[k];
        if (!o->alive) continue;
        int ox, oy, ow, oh;
        foe_box(o, &ox, &oy, &ow, &oh);
        if (eko_dist(ox + ow / 2 - bx, oy + oh / 2 - by) >= r + (o->kind == GE_RAMROD ? 12 : 0)) continue;
        int d = is_boss(o->kind) ? boss_dmg : dmg;
        if (o->kind == GE_COIL && o->a == 0) d = 0;
        if (d <= 0) continue;
        o->hp -= d;
        o->flash = 8;
        if (o->hp <= 0) kill_foe(k);
    }
}

void eko_gallery_fire(int x, int y) {
    G->shots_fired++;
    G->last_shot_x = x;
    G->last_shot_y = y;
    G->flash_t = 4;
    eko_sfx("eko_gun");
    /* the nearest thing under the sight takes it */
    int best = -1, bd = -1;
    for (int i = 0; i < GA_MAX_FOES; i++) {
        EkoGFoe *f = &G->foe[i];
        if (!f->alive) continue;
        int x0, y0, w, h;
        foe_box(f, &x0, &y0, &w, &h);
        if (h <= 0) continue;
        if (x < x0 - 1 || x > x0 + w || y < y0 - 1 || y > y0 + h) continue;
        int depth = f->kind == GE_MISSILE ? 9 : is_sky(f->kind) ? 0 : 1 + f->row;
        if (depth > bd) { bd = depth; best = i; }
    }
    if (best < 0) return;
    EkoGFoe *f = &G->foe[best];
    /* the core shrugs it off while the pods and the cannon still fly */
    if (f->kind == GE_COIL && !f->a) { eko_sfx("eko_tink"); return; }
    G->hits++;
    f->hp--;
    f->flash = 6;
    if (f->hp <= 0) kill_foe(best);
    else eko_sfx("eko_hit");
}

/* ---- foes ---------------------------------------------------------------------------- */

static int count_kind(int kind) {
    int n = 0;
    for (int i = 0; i < GA_MAX_FOES; i++) n += G->foe[i].alive && G->foe[i].kind == kind;
    return n;
}

static void foes_update(void) {
    for (int i = 0; i < GA_MAX_FOES; i++) {
        EkoGFoe *f = &G->foe[i];
        if (!f->alive) continue;
        f->t++;
        if (f->flash) f->flash--;
        if (f->cd) f->cd--;
        if (f->t < -50000) continue; /* held still for a test */
        int s = GA_ROW_SCALE[f->row];
        switch (f->kind) {
        case GE_RIFLE:
            if (f->state == 0 && f->t >= 10) { f->state = 1; f->t = 0; }
            else if (f->state == 1) {
                if (f->t == 34 || (f->t == 84 && f->a)) fire_at(f, 0, 0, 44);
                if (f->t > 120) { f->state = 2; f->t = 0; }
            } else if (f->state == 2 && f->t >= 10) f->alive = 0; /* back behind cover: got away */
            break;
        case GE_MORTAR:
            if (f->state == 0 && f->t >= 10) { f->state = 1; f->t = 0; }
            else if (f->state == 1) {
                if (f->t == 40) fire_at(f, 0, 1, 44); /* a grenade: 44 frames up and over, then its fuse */
                if (f->t > 110) { f->state = 2; f->t = 0; }
            } else if (f->state == 2 && f->t >= 10) f->alive = 0;
            break;
        case GE_RUNNER:
            f->x += f->dir * (96 * s);
            if (f->t == 50) fire_at(f, 0, 0, 40);
            if (EKO_PX(f->x) < -30 || EKO_PX(f->x) > SCREEN_W + 30) f->alive = 0;
            break;
        case GE_JEEP:
            f->x += f->dir * (64 * s);
            if (f->t % 70 == 35) { fire_at(f, -10, 0, 40); }
            if (f->t % 70 == 41) { fire_at(f, 10, 0, 40); }
            if (EKO_PX(f->x) < -50 || EKO_PX(f->x) > SCREEN_W + 50) f->alive = 0;
            break;
        case GE_CHOPPER:
            f->x += f->dir * 200;
            f->y += eko_sin(f->t * 2) / 4;
            if (f->t % 80 == 40) { fire_at(f, -12, 0, 46); fire_at(f, 12, 0, 46); }
            if (EKO_PX(f->x) < -40 || EKO_PX(f->x) > SCREEN_W + 40) f->alive = 0;
            break;
        case GE_BARREL:
            break;
        case GE_RAMROD: {
            /* the ram on the near row: paces, then a three-way spread or a lobbed shell */
            bool rage = count_kind(GE_VULTRA) == 0;
            if (f->state == 0) {
                f->x += f->dir * (rage ? 288 : 192);
                if (EKO_PX(f->x) < 40) f->dir = 1;
                if (EKO_PX(f->x) > 280) f->dir = -1;
                if (!f->cd) { f->state = 1; f->t = 0; }
            } else if (f->state == 1) {
                if (f->t == 22) {
                    if (f->a % 3 == 2) fire_at(f, 0, 1, 36);
                    else { fire_at(f, -32, 0, 40); fire_at(f, 0, 0, 40); fire_at(f, 32, 0, 40); }
                    f->a++;
                }
                if (f->t > 40) { f->state = 0; f->cd = rage ? 50 : 90; }
            }
            break;
        }
        case GE_VULTRA: {
            bool rage = count_kind(GE_RAMROD) == 0;
            int per = rage ? 240 : 360;
            f->b++;
            f->x = (160 + eko_sin(f->b * 64 / per) * 124 / 256) * EKO_FX;
            f->y = (40 + eko_sin(f->b * 128 / per) * 10 / 256) * EKO_FX;
            if (!f->cd) {
                fire_at(f, -18, 2, 52);
                fire_at(f, 18, 2, 52);
                f->cd = rage ? 55 : 95;
            }
            break;
        }
        case GE_POD: case GE_CANNON: case GE_COIL: {
            /* the gunship: every part rides its sway */
            int sway = eko_sin(G->t / 2) * 40 / 256;
            f->x = (f->b + sway) * EKO_FX;
            if (f->kind == GE_POD && !f->cd) {
                int m = eko_gallery_spawn(GE_MISSILE, 2, EKO_PX(f->x), 0);
                if (m >= 0) { G->foe[m].y = f->y + 8 * EKO_FX; G->foe[m].x = f->x; }
                f->cd = 130;
                eko_sfx("eko_missile");
            }
            if (f->kind == GE_CANNON && !f->cd) {
                fire_at(f, -30, 0, 44); fire_at(f, 0, 0, 44); fire_at(f, 30, 0, 44);
                f->cd = 95;
            }
            if (f->kind == GE_COIL) {
                bool open = count_kind(GE_POD) == 0 && count_kind(GE_CANNON) == 0;
                if (open && !f->a) { f->a = 1; eko_say("THE CORE IS OPEN"); f->cd = 60; }
                if (f->a && !f->cd) {
                    for (int k = -1; k <= 1; k++) {
                        int m = eko_gallery_spawn(GE_MISSILE, 2, EKO_PX(f->x) + k * 26, 0);
                        if (m >= 0) G->foe[m].y = f->y + 10 * EKO_FX;
                    }
                    fire_at(f, -20, 0, 40);
                    fire_at(f, 20, 0, 40);
                    f->cd = 110;
                    eko_sfx("eko_missile");
                }
            }
            break;
        }
        case GE_MISSILE: {
            /* drops toward her, steering after her x; at the bottom it bursts */
            int tx = G->px;
            f->x += iclamp((tx - f->x) / 40, -192, 192);
            f->y += 200 + f->t * 2;
            if (EKO_PX(f->y) >= GA_PY - 14) {
                eko_burst(EKO_PX(f->x), GA_PY - 10, C_ORANGE, 10);
                eko_sfx("eko_boom");
                if (iabs(EKO_PX(f->x) - EKO_PX(G->px)) < 14) hurt_vesper(2);
                f->alive = 0;
            }
            break;
        }
        default: break;
        }
    }
}

static void shots_update(void) {
    for (int i = 0; i < GA_MAX_SHOTS; i++) {
        EkoGShot *s = &G->shot[i];
        if (!s->alive) continue;
        s->t++;
        if (s->kind == 3) {
            /* a mango: falls, then lies there a while (a tap of A takes it) */
            if (s->t > s->dur + 240) s->alive = 0;
            continue;
        }
        if (s->kind == 4) {
            /* hers, thrown back: it goes off where the sight was */
            if (s->t >= s->dur) {
                eko_burst(s->tx, s->ty, C_ORANGE, 12);
                eko_sfx("eko_boom");
                eg.shake = imax(eg.shake, 6);
                s->alive = 0;
                blast_foes(s->tx, s->ty, GA_THROW_R, 6, 3);
            }
            continue;
        }
        if (s->kind == 1) {
            /* a grenade: lands, lies there with its fuse burning, then goes off */
            if (s->t >= s->dur + GA_FUSE_T) {
                eko_burst(s->tx, s->ty, C_ORANGE, 6);
                eko_sfx("eko_boom");
                if (iabs(EKO_PX(G->px) - s->tx) < 20) hurt_vesper(2);
                s->alive = 0;
            }
            continue;
        }
        if (s->t >= s->dur) {
            int r = s->kind == 0 ? 8 : 18;
            if (s->kind) { eko_burst(s->tx, s->ty, C_ORANGE, 6); eko_sfx("eko_boom"); }
            if (iabs(EKO_PX(G->px) - s->tx) < r + 2) hurt_vesper(s->kind ? 2 : 1);
            s->alive = 0;
        }
    }
}

void eko_gallery_boss_start(void) {
    if (G->boss_on) return;
    G->boss_on = 1;
    if (G->script == GS_ANDES) {
        int r = eko_gallery_spawn(GE_RAMROD, 2, 80, 1);
        if (r >= 0) G->foe[r].cd = 90;
        int v = eko_gallery_spawn(GE_VULTRA, 0, 160, 1);
        if (v >= 0) G->foe[v].cd = 120;
        eko_gallery_spawn(GE_BARREL, 2, 40, 0);
        eko_gallery_spawn(GE_BARREL, 2, 280, 0);
        eko_say("RAMROD AND VULTRA!");
    } else {
        static const int PX[4] = {100, 220, 160, 160};
        static const int PY[4] = {44, 44, 58, 34};
        static const int K[4] = {GE_POD, GE_POD, GE_CANNON, GE_COIL};
        for (int k = 0; k < 4; k++) {
            int i = eko_gallery_spawn(K[k], 0, PX[k], 0);
            if (i >= 0) { G->foe[i].b = PX[k]; G->foe[i].y = PY[k] * EKO_FX; G->foe[i].cd = 80 + k * 40; G->foe[i].a = 0; }
        }
        eko_say("MADAME COIL'S GUNSHIP!");
    }
    eko_sfx("eko_bossin");
    if (!eko_simulating) music_play(G->script == GS_ANDES ? EKO_MUS_BOSS : EKO_MUS_FINAL);
}

/* ---- the frame ------------------------------------------------------------------------- */

void eko_gallery_start(int script) {
    memset(G, 0, sizeof *G);
    G->script = script;
    G->px = 160 * EKO_FX;
    G->cx = 160 * EKO_FX;
    G->cy = 100 * EKO_FX;
    G->face = 1;
}

/* a tap of A: throw back the grenade in hand, or take what she stands on.
 * True when the press went on that (and so fires no shot). */
static bool grab_or_throw(void) {
    int px = EKO_PX(G->px);
    if (G->carry) {
        add_shot(4, px, GA_PY - 22, EKO_PX(G->cx), EKO_PX(G->cy), GA_THROW_T);
        G->carry = 0;
        G->throws++;
        eko_sfx("eko_toss");
        return true;
    }
    for (int i = 0; i < GA_MAX_SHOTS; i++) {
        EkoGShot *s = &G->shot[i];
        if (!s->alive || s->t < s->dur || iabs(px - s->tx) >= GA_GRAB) continue;
        if (s->kind == 3) {
            eko_set_hp(eko_hp() + EKO_MANGO_HP);
            eko_sfx("eko_mango");
            eko_say("MANGO!");
            s->alive = 0;
            return true;
        }
        if (s->kind == 1) {
            /* a live grenade: hers now, its fuse still burning */
            G->carry = imax(GA_GRAB_FUSE, s->dur + GA_FUSE_T - s->t);
            G->grabs++;
            s->alive = 0;
            eko_sfx("eko_pickup");
            return true;
        }
    }
    return false;
}

void eko_gallery_update(void) {
    const EkoGScript *sc = &EKO_GSCRIPT[G->script];
    if (G->dead_t) {
        G->dead_t++;
        if (G->dead_t > 70) {
            eko_life_lost();
            if (eko_hp() > 0) { G->dead_t = 0; G->inv = 120; }
        }
        return;
    }
    G->t++;
    if (G->inv) G->inv--;
    if (G->flash_t) G->flash_t--;
    if (G->cd) G->cd--;
    if (G->roll_cd) G->roll_cd--;
    /* the combo meter runs down */
    if (G->meter > 0 && --G->meter == 0) G->combo = 0;
    bool L = ek_btn(BTN_LEFT), R = ek_btn(BTN_RIGHT), U = ek_btn(BTN_UP), D = ek_btn(BTN_DOWN);
    bool A = ek_btn(EKO_MAIN), Ap = ek_btnp(EKO_MAIN), Bp = ek_btnp(EKO_ALT);
    int dir = (R ? 1 : 0) - (L ? 1 : 0);
    if (G->roll_t > 0) {
        G->roll_t--;
        G->px += G->roll_dir * GA_ROLL_VX;
    } else {
        G->held_t = A ? G->held_t + 1 : 0;
        bool locked = G->held_t >= GA_LOCK_T;
        if (Bp && !G->roll_cd) {
            G->roll_t = GA_ROLL_T;
            G->roll_dir = dir ? dir : G->face;
            G->roll_cd = GA_ROLL_T + 12;
            G->held_t = 0;
            eko_sfx("eko_roll");
        } else if (locked) {
            /* planted: the pad steers the sight anywhere */
            G->cx += dir * GA_AIM;
        } else {
            G->px += dir * GA_WALK;
            if (dir) G->face = dir;
        }
        G->cy += ((D ? 1 : 0) - (U ? 1 : 0)) * (locked ? GA_AIM : GA_AIM_Y);
        if (!locked) {
            /* walking: the sight rides along with her */
            int d = G->px - G->cx;
            G->cx += iclamp(d, -1024, 1024);
        }
        /* a press: a grenade in hand goes back at the sight; standing on
         * something, it is picked up; otherwise it shoots */
        bool took = Ap && grab_or_throw();
        /* fire: a press shoots at once (mashing is fastest), a hold keeps shooting slower */
        if (took || G->carry) {
            /* the press went on the grenade (or the mango): no shot */
        } else if (Ap && G->cd <= 0) {
            eko_gallery_fire(EKO_PX(G->cx), EKO_PX(G->cy));
            G->cd = GA_TAP_CD;
        } else if (A && G->held_t > 1 && G->cd <= 0) {
            eko_gallery_fire(EKO_PX(G->cx), EKO_PX(G->cy));
            G->cd = GA_HOLD_CD;
        }
    }
    G->px = iclamp(G->px, GA_MINX * EKO_FX, GA_MAXX * EKO_FX);
    /* she can only shoot within a stretch in front of her */
    G->cx = iclamp(G->cx, G->px - GA_REACH * EKO_FX, G->px + GA_REACH * EKO_FX);
    G->cx = iclamp(G->cx, 4 * EKO_FX, (SCREEN_W - 4) * EKO_FX);
    /* a grenade held too long goes off in her hands */
    if (G->carry && --G->carry == 0) {
        eko_burst(EKO_PX(G->px), GA_PY - 20, C_ORANGE, 12);
        eko_sfx("eko_boom");
        hurt_vesper(2);
    }
    G->cy = iclamp(G->cy, GA_CROSS_MINY * EKO_FX, GA_CROSS_MAXY * EKO_FX);
    /* the script */
    while (G->spawn_i < sc->n && sc->spawns[G->spawn_i].t <= G->t) {
        const EkoGSpawn *sp = &sc->spawns[G->spawn_i++];
        int f = eko_gallery_spawn(sp->kind, sp->row, sp->x, sp->dir);
        if (f >= 0 && sp->kind == GE_RIFLE) G->foe[f].a = rng_range(&eg.rng, 0, 1);
    }
    if (!G->boss_on && G->t >= sc->boss_t) {
        /* the last of the waves cleared off, then the bosses */
        int n = 0;
        for (int i = 0; i < GA_MAX_FOES; i++) n += G->foe[i].alive && G->foe[i].kind != GE_BARREL;
        if (n == 0 || G->t > sc->boss_t + 600) eko_gallery_boss_start();
    }
    /* red barrels come back on the field during the fight */
    if (G->boss_on && !G->boss_dead && G->script == GS_ANDES) {
        if (++G->barrel_t > 600 && count_kind(GE_BARREL) < 2) {
            eko_gallery_spawn(GE_BARREL, 2, rng_range(&eg.rng, 0, 1) ? 50 : 270, 0);
            G->barrel_t = 0;
        }
    }
    foes_update();
    shots_update();
    if (G->boss_on && !G->boss_dead) {
        bool left = G->script == GS_ANDES ? count_kind(GE_RAMROD) + count_kind(GE_VULTRA) > 0 : count_kind(GE_COIL) > 0;
        if (!left) {
            G->boss_dead = 1;
            /* everything else falls with them */
            for (int i = 0; i < GA_MAX_FOES; i++)
                if (G->foe[i].alive) { eko_burst(EKO_PX(G->foe[i].x), EKO_PX(G->foe[i].y) - 6, C_ORANGE, 6); G->foe[i].alive = 0; }
            for (int i = 0; i < GA_MAX_SHOTS; i++) if (G->shot[i].kind != 3) G->shot[i].alive = 0;
            eg.shake = 30;
        }
    }
    if (G->boss_dead && ++G->done_t > 180) eko_section_done();
}
