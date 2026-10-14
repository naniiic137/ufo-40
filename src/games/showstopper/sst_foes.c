/* SHOWSTOPPER - the foes: the show's runaways and hired hands. Each kind
 * does what its counterpart in the original does (sst.h lists them by
 * stage); the names, looks and numbers are ours. */
#include "sst.h"

const SstFoeDef SST_FOE[FK_COUNT] = {
    [FK_NONE] = {"NONE", 1, 4, 4, FF_NOHURT | FF_NOCARD},
    [FK_CRATE] = {"CRATE", 3, 7, 7, FF_SOLID | FF_NOHURT | FF_ALWAYS},
    [FK_BARREL] = {"BARREL", 3, 6, 7, FF_SOLID | FF_NOHURT | FF_ALWAYS},
    [FK_SECRET] = {"SPOT", SST_PASS_HITS, 6, 6, FF_NOHURT | FF_NOCARD},
    [FK_GUMDROP] = {"GUMDROP", 2, 6, 5, FF_STUN},
    [FK_LANTERN] = {"LANTERN", 4, 5, 7, FF_NOHURT},
    [FK_JESTER] = {"JESTER", 3, 6, 7, FF_STUN},
    [FK_BAT] = {"BAT", 1, 5, 4, FF_FLY | FF_STUN},
    [FK_GUN] = {"WALL GUN", 5, 7, 7, FF_WALL},
    [FK_SNOWBALL] = {"SNOWBALL", 2, 5, 5, FF_STUN},
    [FK_GOURD] = {"GOURD CLOAK", 3, 6, 7, FF_FLY | FF_STUN},
    [FK_PUFFER] = {"PUFFER", 2, 6, 6, FF_FLY},
    [FK_TOYTRAIN] = {"TOY TRAIN", 4, 9, 5, FF_FLY},
    [FK_BOMBER] = {"BOMBER", 3, 6, 7, FF_WALL},
    [FK_SNOWMAN] = {"SNOWMAN", 5, 7, 8, FF_STUN},
    [FK_ROCKETEER] = {"ROCKETEER", 4, 6, 7, FF_STUN},
    [FK_COPTER] = {"COPTER", 8, 6, 6, FF_FLY | FF_ALWAYS},
    [FK_GUN3] = {"TRIPLE GUN", 6, 7, 7, FF_WALL},
    [FK_TURTLE] = {"TURTLE", 6, 7, 6, FF_STUN | FF_ALWAYS},
    [FK_HARLEQUIN] = {"HARLEQUIN", 6, 6, 8, FF_STUN | FF_ALWAYS},
    [FK_SKULLJET] = {"SKULL JET", 6, 7, 7, FF_WALL},
    [FK_EMBER] = {"EMBERLING", 3, 6, 6, FF_FLY},
    [FK_BOOK] = {"BOOK", 2, 5, 4, FF_FLY},
    [FK_IMP] = {"PAGE IMP", 2, 5, 5, FF_FLY | FF_STUN},
    [FK_RACECAR] = {"RACECAR", 6, 8, 6, FF_ALWAYS},
    [FK_TORCH] = {"TORCH", 4, 6, 7, FF_WALL},
    [FK_MOUSE] = {"CLOCKWORK MOUSE", 2, 4, 4, FF_STUN | FF_NOCARD},
    [FK_SKULL] = {"HOPSKULL", 2, 5, 5, FF_STUN | FF_NOCARD},
    [FK_EGG] = {"EGG", 3, 4, 5, FF_NOHURT | FF_NOCARD},
    [FK_WISP] = {"WICK FIRE", 1, 4, 4, FF_FLY | FF_NOCARD},
    [FK_SWOOP] = {"SHOW BAT", 1, 5, 4, FF_FLY | FF_NOCARD},
    [FK_SPIKE] = {"SPIKE BLOCK", 2, 7, 7, FF_NOCARD},
};

static int px_x(const SstFoe *f) { return SST_PX(f->x); }
static int px_y(const SstFoe *f) { return SST_PX(f->y); }

void sst_foe_init(SstFoe *f) {
    if (f->kind == FK_GUN && sst_g.stage >= 2) {
        f->kind = FK_GUN3;
        f->hp = SST_FOE[FK_GUN3].hp;
    }
    /* their clocks start apart so a row of them doesn't fire as one */
    f->t = (px_x(f) * 7 + px_y(f) * 3) % 50;
    switch (f->kind) {
    case FK_EMBER:
        f->size = 3;
        f->hp = 3;
        break;
    case FK_SKULLJET: {
        /* it faces away from its wall */
        int tx = px_x(f) >> 4, ty = px_y(f) >> 4;
        if (!sst_tile_solid(sst_tile_at(tx + 1, ty))) f->dir = 0;
        else if (!sst_tile_solid(sst_tile_at(tx - 1, ty))) f->dir = 32;
        else if (!sst_tile_solid(sst_tile_at(tx, ty + 1))) f->dir = 16;
        else f->dir = 48;
        break;
    }
    case FK_RACECAR:
        f->dir = 1;
        break;
    case FK_EGG:
        f->t = 0;
        break;
    default: break;
    }
}

/* a walker's step: no walls, no pits */
static bool walk(SstFoe *f, int vx, int vy) {
    int nx = f->x + vx, ny = f->y + vy;
    int hw = SST_FOE[f->kind].hw - 1, hh = SST_FOE[f->kind].hh - 1;
    if (sst_box_solid(nx, ny, hw, hh)) return false;
    int t = sst_tile_at(SST_PX(nx) >> 4, SST_PX(ny) >> 4);
    if (sst_tile_pit(t)) return false;
    f->x = nx;
    f->y = ny;
    return true;
}

static void steer(SstFoe *f, int speed, int turn) {
    int a = (f->vx || f->vy) ? sst_dir_to(f->vx, f->vy) : sst_player_ang(f->x, f->y);
    int want = sst_player_ang(f->x, f->y);
    int d = ((want - a + 96) & 63) - 32;
    if (d > turn) d = turn;
    if (d < -turn) d = -turn;
    a += d;
    f->vx = speed * sst_cos(a) / 256;
    f->vy = speed * sst_sin(a) / 256;
}

static int pdist(const SstFoe *f) { return sst_dist(SST_PX(sst_g.pl.x - f->x), SST_PX(sst_g.pl.y - f->y)); }

static bool can_fire(const SstFoe *f) {
    return sst_on_screen(f->x, f->y, -4) && sst_g.pl.state != PS_BUBBLE && sst_g.pl.state != PS_GONE;
}

static void lob_at_player(const SstFoe *f, int frames) {
    int i = sst_add_bullet(EB_LOB, f->x, f->y, (sst_g.pl.x - f->x) / frames, (sst_g.pl.y - f->y) / frames);
    if (i >= 0) sst_g.eb[i].a = frames;
}

static void think(SstFoe *f, int idx) {
    SstPlayer *p = &sst_g.pl;
    switch (f->kind) {
    case FK_GUMDROP:
        if (f->t % 40 == 0) {
            int a = sst_player_ang(f->x, f->y) + rng_range(&sst_g.rng, -10, 10);
            f->vx = 128 * sst_cos(a) / 256;
            f->vy = 128 * sst_sin(a) / 256;
        }
        if (!walk(f, f->vx, f->vy)) f->vx = -f->vx, f->vy = -f->vy;
        break;
    case FK_LANTERN:
        if (f->t % 150 == 0 && can_fire(f)) {
            int i = sst_aim_bullet(EB_HOMING, f->x, f->y, 200, 0);
            if (i >= 0) sst_g.eb[i].kind = EB_HOMING;
            sst_sfx("sst_puff");
        }
        break;
    case FK_JESTER:
        if (!f->dir) f->dir = 1;
        if (!walk(f, f->dir * 100, 0) || f->t % 120 == 0) f->dir = -f->dir;
        if (f->t % 100 == 50 && can_fire(f))
            for (int k = -1; k <= 1; k++) sst_aim_bullet(EB_ROUND, f->x, f->y, 320, k * 4);
        break;
    case FK_BAT:
        if (f->a == 0) {
            if (pdist(f) < 70 && sst_on_screen(f->x, f->y, -2)) { f->a = 1; f->t = 0; sst_sfx("sst_flap"); }
            break;
        }
        steer(f, 360, 2);
        f->x += f->vx;
        f->y += f->vy + sst_sin(f->t * 4) * 64 / 256;
        break;
    case FK_GUN:
        if (f->t % 110 == 0 && can_fire(f)) sst_aim_bullet(EB_ROUND, f->x, f->y, 320, 0);
        break;
    case FK_GUN3:
        if (f->t % 120 == 0 && can_fire(f))
            for (int k = -1; k <= 1; k++) sst_aim_bullet(EB_ROUND, f->x, f->y, 320, k * 5);
        break;
    case FK_SNOWBALL: {
        int k = f->t % 40;
        if (k == 0) {
            int a = sst_player_ang(f->x, f->y);
            f->vx = 256 * sst_cos(a) / 256;
            f->vy = 256 * sst_sin(a) / 256;
        }
        if (k < 24) walk(f, f->vx, f->vy);
        f->a = k < 24 ? sst_sin(k * 32 / 24) * 8 / 256 : 0; /* the hop's height, for drawing */
        break;
    }
    case FK_GOURD: {
        /* in from its side, then hover and fire wavy pairs; every third
         * time it circles the car leaving bomb shots */
        if (f->c > 0) { f->c--; break; }
        int cycle = f->t % 600;
        if (f->b == 0) {
            int tx = f->dir > 0 ? 50 + (idx * 37) % 70 : 270 - (idx * 37) % 70;
            f->x += isign(tx * SST_FX - f->x) * imin(iabs(tx * SST_FX - f->x), 256);
            if (iabs(tx * SST_FX - f->x) < 256) f->b = 1;
            break;
        }
        if (cycle < 400) {
            f->x += sst_sin(f->t * 2) * 48 / 256;
            f->y += sst_cos(f->t * 3) * 32 / 256;
            if (cycle % 80 == 40 && can_fire(f)) {
                for (int s = -1; s <= 1; s += 2) {
                    int i = sst_aim_bullet(EB_HELIX, f->x, f->y, 300, 0);
                    if (i >= 0) sst_g.eb[i].a = s;
                }
                sst_sfx("sst_wave");
            }
        } else {
            /* round the car */
            int a = (cycle - 400) * 64 / 200 + (f->dir > 0 ? 32 : 0);
            int cx = 160 * SST_FX, cy = 88 * SST_FX;
            int tx = cx + 110 * sst_cos(a) * SST_FX / 256, ty = cy + 64 * sst_sin(a) * SST_FX / 256;
            f->x += (tx - f->x) / 8;
            f->y += (ty - f->y) / 8;
            if (cycle % 40 == 20) {
                int i = sst_add_bullet(EB_MINE, f->x, f->y, 0, 0);
                (void)i;
            }
        }
        break;
    }
    case FK_PUFFER:
        if (f->c > 0) { f->c--; break; }
        f->vy = 720;
        if (f->t % 8 == 0) f->vx = isign(p->x - f->x) * 96;
        f->x += f->vx;
        f->y += f->vy;
        if (SST_PX(f->y) - sst_g.cam_y > SST_VIEW_H + 20) f->alive = 0;
        break;
    case FK_TOYTRAIN:
        if (f->c > 0) { f->c--; break; }
        f->x += f->dir * 512;
        if ((f->dir > 0 && SST_PX(f->x) > SCREEN_W + 20) || (f->dir < 0 && SST_PX(f->x) < -20)) f->alive = 0;
        break;
    case FK_BOMBER:
        if (f->t % 130 == 0 && can_fire(f)) lob_at_player(f, 50);
        break;
    case FK_SNOWMAN: {
        /* rush, stop, a short star volley from the carrot */
        if (f->a == 0) {
            if (pdist(f) < 100 && can_fire(f) && f->t > 40) {
                f->a = 1;
                f->b = 0;
                int ang = sst_player_ang(f->x, f->y);
                f->vx = 720 * sst_cos(ang) / 256;
                f->vy = 720 * sst_sin(ang) / 256;
                sst_sfx("sst_rush");
            }
        } else {
            f->b++;
            if (f->a == 1) {
                if (!walk(f, f->vx, f->vy) || f->b >= 18) { f->a = 2; f->b = 0; }
            } else if (f->a == 2 && f->b == 20) {
                for (int k = -2; k <= 2; k++) sst_aim_bullet(EB_STAR, f->x, f->y + 2 * SST_FX, 384, k * 3);
                sst_sfx("sst_stars");
            } else if (f->a == 2 && f->b > 90) {
                f->a = 0;
                f->t = 0;
            }
        }
        break;
    }
    case FK_ROCKETEER:
        if (!f->dir) f->dir = 1;
        if (!walk(f, f->dir * 64, 0) || f->t % 150 == 75) f->dir = -f->dir;
        if (f->t % 150 == 0 && can_fire(f)) {
            int i = sst_add_bullet(EB_ROCKET, p->x, p->y, 0, 0);
            if (i >= 0) sst_g.eb[i].a = 60;
            sst_add_haz(HZ_MARK, p->x, p->y, 60, 24);
            sst_sfx("sst_launch");
        }
        break;
    case FK_COPTER: {
        f->b = (f->b + 2) & 63;
        steer(f, 120, 1);
        f->x += f->vx;
        f->y += f->vy;
        /* the long arms sweep round: they hurt, only the middle can be hit */
        if (p->state == PS_WALK && p->inv == 0)
            for (int arm = 0; arm < 2; arm++)
                for (int r = 9; r <= 21; r += 6) {
                    int ax = f->x + r * sst_cos(f->b + arm * 32), ay = f->y + r * sst_sin(f->b + arm * 32);
                    if (iabs(SST_PX(ax - p->x)) < 5 && iabs(SST_PX(ay - p->y)) < 5) sst_player_hurt(1);
                }
        break;
    }
    case FK_TURTLE:
        if (f->t % 6 == 0) {
            int a = sst_player_ang(f->x, f->y);
            walk(f, 64 * sst_cos(a) / 256 * 6, 64 * sst_sin(a) / 256 * 6);
        }
        if (f->t % 130 == 65 && can_fire(f)) {
            sst_ring(EB_ROUND, f->x, f->y, 12, 256, f->t % 5);
            sst_sfx("sst_ring");
        }
        break;
    case FK_HARLEQUIN:
        if (!f->dir) f->dir = 1;
        if (!walk(f, f->dir * 150, 0) || f->t % 140 == 0) f->dir = -f->dir;
        if (f->t % 170 == 85 && can_fire(f)) {
            for (int k = 0; k < 4; k++) sst_add_bullet(EB_MINE, f->x + SST_DX8[k * 2] * 14 * SST_FX, f->y + SST_DY8[k * 2] * 14 * SST_FX, 0, 0);
            sst_sfx("sst_drop");
        }
        if (f->t % 60 == 30 && can_fire(f)) sst_aim_bullet(EB_ROUND, f->x, f->y, 384, 0);
        break;
    case FK_SKULLJET: {
        int k = f->t % 200;
        f->a = k >= 140 && k < 170; /* the glow before the flame */
        if (k == 170 && can_fire(f)) {
            for (int s = 1; s <= 6; s++) {
                int i = sst_add_bullet(EB_JET, f->x + s * 16 * sst_cos(f->dir), f->y + s * 16 * sst_sin(f->dir), 0, 0);
                if (i >= 0) sst_g.eb[i].a = 60;
            }
            sst_sfx("sst_jet");
        }
        break;
    }
    case FK_EMBER: {
        int sp = f->size == 3 ? 150 : f->size == 2 ? 220 : 300;
        if (f->a > 0) {
            f->a--;
        } else {
            steer(f, sp, 2);
        }
        f->x += f->vx;
        f->y += f->vy;
        f->hp = imin(f->hp, f->size);
        break;
    }
    case FK_BOOK:
        if (f->a == 0 && f->d == 0) {
            /* the first one becomes the head of a train of six */
            f->d = 1;
            int side = SST_PX(f->x) < 160 ? -1 : 1;
            f->dir = -side;
            f->b = f->y;
            f->x = (side < 0 ? -12 : SCREEN_W + 12) * SST_FX;
            for (int k = 1; k < 6; k++) {
                int j = sst_spawn_foe(FK_BOOK, f->x, f->y);
                if (j < 0) break;
                SstFoe *g = &sst_g.foe[j];
                g->d = 1;
                g->dir = f->dir;
                g->b = f->b;
                g->c = k * 12;
                g->t = 0;
            }
            f->t = 0;
        }
        if (f->c > 0) { f->c--; break; }
        f->x += f->dir * 384;
        f->y = f->b + sst_sin(f->t * 2) * 18;
        if ((f->dir > 0 && SST_PX(f->x) > SCREEN_W + 30) || (f->dir < 0 && SST_PX(f->x) < -30)) f->alive = 0;
        break;
    case FK_IMP:
        /* hovers about its spot (flying in to it first) */
        if (!f->b) { f->b = f->x; f->d = f->y; }
        f->x = f->b + sst_sin(f->t) * 20 * SST_FX / 256;
        if (iabs(f->d - f->y) > 2 * SST_FX) f->y += (f->d - f->y) / 12;
        else f->y = f->d + sst_sin(f->t * 3) * 6 * SST_FX / 256;
        if (f->t % 110 == 55 && can_fire(f)) lob_at_player(f, 40);
        break;
    case FK_RACECAR:
        if (!sst_on_screen(f->x, f->y, -8) && f->a == 0) break;
        f->a = 1;
        if (!walk(f, f->dir * 640, 0)) f->dir = -f->dir;
        if (SST_PX(f->x) < 24 || SST_PX(f->x) > SCREEN_W - 24) {
            if ((SST_PX(f->x) < 24 && f->dir < 0) || (SST_PX(f->x) > SCREEN_W - 24 && f->dir > 0)) f->dir = -f->dir;
        }
        if (f->t % 6 == 0) sst_add_haz(HZ_SLICK, f->x, f->y + 3 * SST_FX, 300, 6);
        break;
    case FK_TORCH:
        if (f->t % 120 == 60 && can_fire(f)) sst_aim_bullet(EB_FIRE, f->x, f->y, 384, 0);
        break;
    case FK_MOUSE:
        steer(f, 300, 2);
        if (!walk(f, f->vx, f->vy) && !walk(f, f->vx, 0)) walk(f, 0, f->vy);
        if (f->t > 360) f->alive = 0;
        break;
    case FK_SKULL: {
        int k = f->t % 30;
        if (k == 0) {
            int a = sst_player_ang(f->x, f->y);
            f->vx = 320 * sst_cos(a) / 256;
            f->vy = 320 * sst_sin(a) / 256;
        }
        if (k < 16) walk(f, f->vx, f->vy);
        f->a = k < 16 ? sst_sin(k * 2) * 6 / 256 : 0;
        break;
    }
    case FK_EGG:
        if (f->t >= 120) {
            f->alive = 0;
            int j = sst_spawn_foe(FK_SKULL, f->x, f->y);
            if (j >= 0) sst_g.foe[j].t = 0;
            sst_sfx("sst_hatch");
        }
        break;
    case FK_WISP:
        if (!f->vx && !f->vy) {
            int a = rng_range(&sst_g.rng, 0, 63);
            f->vx = 256 * sst_cos(a) / 256;
            f->vy = 256 * sst_sin(a) / 256;
        }
        if (sst_box_solid(f->x + f->vx, f->y, 3, 3) || SST_PX(f->x + f->vx) < 20 || SST_PX(f->x + f->vx) > 300) f->vx = -f->vx;
        if (sst_box_solid(f->x, f->y + f->vy, 3, 3) || SST_PX(f->y + f->vy) < sst_g.cam_y + 16 || SST_PX(f->y + f->vy) > sst_g.cam_y + SST_VIEW_H - 10) f->vy = -f->vy;
        f->x += f->vx;
        f->y += f->vy;
        if (f->t > 420) f->alive = 0;
        break;
    case FK_SWOOP:
        steer(f, 420, 3);
        f->x += f->vx;
        f->y += f->vy;
        if (f->t > 400) f->alive = 0;
        break;
    case FK_SPIKE:
        if (f->b > 0 && --f->b == 0) f->alive = 0;
        break;
    default: break;
    }
}

void sst_foes_update(void) {
    for (int i = 0; i < SST_MAX_FOES; i++) {
        SstFoe *f = &sst_g.foe[i];
        if (!f->alive) continue;
        if (f->hurt > 0) f->hurt--;
        if (f->stun > 0) { f->stun--; continue; }
        f->t++;
        think(f, i);
        /* the ones that only pass through are gone once well off the screen */
        if (f->alive && (SST_FOE[f->kind].flags & FF_FLY) && f->t > 200 && !sst_on_screen(f->x, f->y, 60)) f->alive = 0;
    }
}
