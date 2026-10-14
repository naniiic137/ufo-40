/* SHOWSTOPPER - the bosses. Each one fights the way its counterpart in the
 * original does (where it can be hit, what it throws, how it moves); the
 * names, looks and numbers are ours. One boss at a time; its pieces (hands,
 * eyes, a snake's segments, blocks) are parts. */
#include "sst.h"

const char *const SST_BOSS_NAME[BK_COUNT] = {
    "", "THE SNOW ENGINE", "GRANNY GOGGLE", "THE BOILER", "THE DOPPELS", "OLD WICKS", "THE RIBBON",
    "MORROW THE MARVELLOUS", "THE MORROWMAW", "THE HEARTSTACK",
};
const int SST_BOSS_HP[BK_COUNT] = {0, 150, 220, 170, 300, 180, 170, 340, 400, 486};

enum { PK_BODY, PK_WEAK, PK_HAND, PK_TORCH, PK_SEG, PK_EYE, PK_MOUTH, PK_SNAKE, PK_RED, PK_GREEN };

#define B (sst_g.boss)
#define FX SST_FX

static SstPart *add_part(int kind, int x, int y, int hw, int hh, int hp) {
    if (B.nparts >= SST_MAX_PARTS) return &B.part[SST_MAX_PARTS - 1];
    SstPart *p = &B.part[B.nparts++];
    memset(p, 0, sizeof *p);
    p->alive = 1;
    p->kind = (uint8_t)kind;
    p->x = x;
    p->y = y;
    p->hw = (int8_t)hw;
    p->hh = (int8_t)hh;
    p->hp = hp;
    return p;
}

/* the heart's twelve places, around its middle (px) */
static const int8_t HEART_PT[12][2] = {
    {-24, -18}, {-12, -26}, {0, -16}, {12, -26}, {24, -18}, {-28, -4},
    {28, -4}, {-18, 10}, {18, 10}, {-8, 22}, {8, 22}, {0, 32},
};

void sst_boss_start(int kind, int x, int y) {
    memset(&B, 0, sizeof B);
    if (kind <= BK_NONE || kind >= BK_COUNT) return;
    B.alive = 1;
    B.kind = (uint8_t)kind;
    B.x = x;
    B.y = y;
    B.hp = B.maxhp = SST_BOSS_HP[kind];
    switch (kind) {
    case BK_ENGINE:
        add_part(PK_BODY, x, y, 26, 14, 0);
        break;
    case BK_GOGGLE:
        add_part(PK_BODY, x, y - 12 * FX, 28, 12, 0);
        add_part(PK_WEAK, x, y + 6 * FX, 9, 7, 0);
        add_part(PK_HAND, x - 46 * FX, y + 18 * FX, 8, 8, 0);
        add_part(PK_HAND, x + 46 * FX, y + 18 * FX, 8, 8, 0);
        break;
    case BK_BOILER:
        B.y = (sst_g.cam_y + 34) * FX;
        add_part(PK_BODY, x, B.y, 22, 14, 0);
        add_part(PK_WEAK, x, B.y - 6 * FX, 8, 7, 0);
        break;
    case BK_DOPPELS:
        add_part(PK_WEAK, x - 50 * FX, y + 10 * FX, 8, 8, 0);
        add_part(PK_WEAK, x + 50 * FX, y + 10 * FX, 8, 8, 0);
        break;
    case BK_WICKS: {
        add_part(PK_WEAK, x, y + 20 * FX, 7, 10, 0);
        /* the four torches on the crypt's walls */
        int top = y - 32 * FX, bot = y + 112 * FX;
        add_part(PK_TORCH, 24 * FX, top, 0, 0, 0);
        add_part(PK_TORCH, 296 * FX, top, 0, 0, 0);
        add_part(PK_TORCH, 24 * FX, bot, 0, 0, 0);
        add_part(PK_TORCH, 296 * FX, bot, 0, 0, 0);
        break;
    }
    case BK_RIBBON:
        add_part(PK_WEAK, x, y + 30 * FX, 7, 7, 0);
        for (int k = 0; k < 10; k++) add_part(PK_SEG, x, y + (30 - (k + 1) * 9) * FX, 6, 6, 0);
        break;
    case BK_MORROW:
        add_part(PK_WEAK, x, y + 10 * FX, 9, 12, 0);
        add_part(PK_WEAK, x, y + 10 * FX, 9, 12, 0)->alive = 0; /* his double, while he is split */
        break;
    case BK_MAW:
        B.x = 160 * FX;
        B.y = 46 * FX;
        add_part(PK_EYE, B.x - 30 * FX, B.y - 4 * FX, 9, 7, 200);
        add_part(PK_EYE, B.x + 30 * FX, B.y - 4 * FX, 9, 7, 200);
        add_part(PK_EYE, 36 * FX, 62 * FX, 6, 6, 6);
        add_part(PK_EYE, 284 * FX, 62 * FX, 6, 6, 6);
        add_part(PK_EYE, 64 * FX, 104 * FX, 6, 6, 6);
        add_part(PK_EYE, 256 * FX, 104 * FX, 6, 6, 6);
        add_part(PK_MOUTH, B.x, B.y + 18 * FX, 16, 6, 0);
        add_part(PK_SNAKE, B.x, B.y + 18 * FX, 6, 6, 0);
        for (int k = 2; k < 6; k++) B.part[k].t = (k - 2) * 50;
        break;
    case BK_HEART:
        B.x = 160 * FX;
        B.y = 92 * FX;
        for (int k = 0; k < 23; k++) {
            bool red = k % 2 == 0; /* twelve red, eleven green, in turns */
            SstPart *p = add_part(red ? PK_RED : PK_GREEN, B.x, B.y, 6, 6, red ? 24 : 18);
            p->a = k * SST_ANG * 4 / 23; /* angle, in quarter units */
        }
        break;
    default: break;
    }
    sst_sfx("sst_bossin");
}

/* ---- helpers --------------------------------------------------------------------- */

static int pang(int x, int y) { return sst_player_ang(x, y); }

static void aim(int kind, int x, int y, int sp, int ofs) { sst_aim_bullet(kind, x, y, sp, ofs); }

static void fan(int kind, int x, int y, int n, int step, int sp) {
    for (int k = 0; k < n; k++) aim(kind, x, y, sp, (k - (n - 1) / 2) * step - ((n - 1) % 2) * step / 2);
}

static void move_to(SstPart *p, int tx, int ty, int div) {
    p->x += (tx - p->x) / div;
    p->y += (ty - p->y) / div;
}

static bool firing_ok(void) { return sst_g.pl.state == PS_WALK || sst_g.pl.state == PS_DASH; }

/* ---- each boss ----------------------------------------------------------------- */

static void engine(void) {
    /* strafes along the yard; crows on top stream shots; a clockwork mouse
     * now and then; snowballs hop in from the sides */
    if (!B.a) B.a = 1;
    B.x += B.a * 256;
    if (SST_PX(B.x) > 262) B.a = -1;
    if (SST_PX(B.x) < 58) B.a = 1;
    B.part[0].x = B.x;
    B.part[0].y = B.y;
    int k = B.t % 90;
    if (k < 40 && k % 8 == 0 && firing_ok()) aim(EB_ROUND, B.x + ((k / 8) % 2 ? 12 : -12) * FX, B.y - 14 * FX, 340, 0);
    if (B.t % 180 == 120) {
        int j = sst_spawn_foe(FK_MOUSE, B.x, B.y + 18 * FX);
        if (j >= 0) sst_g.foe[j].t = 0;
        sst_sfx("sst_mouse");
    }
    if (B.t % 220 == 200) {
        int j = sst_spawn_foe(FK_SNOWBALL, ((B.t / 220) % 2 ? 300 : 20) * FX, B.y + 70 * FX);
        (void)j;
    }
}

static void goggle(void) {
    /* only her eye; her gloves throw fire, fire volleys, turn into spiked
     * balls that hunt; she coughs up bombs */
    SstPart *eye = &B.part[1], *h0 = &B.part[2], *h1 = &B.part[3];
    int dur[4] = {180, 200, 260, 160};
    B.pt++;
    if (B.pt > dur[B.phase] + 30) { B.phase = (uint8_t)((B.phase + 1) % 4); B.pt = 0; }
    int ph = B.phase, t = B.pt;
    int bob = sst_sin(B.t * 2) * 3;
    B.part[0].y = B.y - 12 * FX + bob;
    eye->y = B.y + 6 * FX + bob;
    for (int h = 0; h < 2; h++) {
        SstPart *hd = h ? h1 : h0;
        int hx = B.x + (h ? 46 : -46) * FX, hy = B.y + 18 * FX + sst_sin(B.t * 3 + h * 20) * 4;
        hd->a = ph == 2 && t < dur[2] - 40; /* a spiked ball */
        if (hd->a) {
            int a = pang(hd->x, hd->y);
            hd->vx += (330 * sst_cos(a) / 256 - hd->vx) / 12;
            hd->vy += (330 * sst_sin(a) / 256 - hd->vy) / 12;
            hd->x += hd->vx;
            hd->y += hd->vy;
        } else {
            move_to(hd, hx, hy, 8);
            hd->vx = hd->vy = 0;
        }
        if (t >= dur[ph] || !firing_ok()) continue;
        if (ph == 0 && t % 60 == 20 + h * 20) { fan(EB_FIRE, hd->x, hd->y, 5, 4, 330); sst_sfx("sst_fire"); }
        if (ph == 1 && t % 34 == h * 17) { sst_ring(EB_ROUND, hd->x, hd->y, 10, 280, (t / 34) * 3); sst_sfx("sst_ring"); }
    }
    if (ph == 3 && t < dur[3] && t % 22 == 0 && t <= 110) {
        int a = 16 + rng_range(&sst_g.rng, -12, 12);
        int i = sst_add_bullet(EB_MINE, B.x, B.y + 20 * FX, 700 * sst_cos(a) / 256, 700 * sst_sin(a) / 256);
        (void)i;
        sst_sfx("sst_cough");
    }
}

static void boiler(void) {
    /* backs up the winding road, the view going with it; only the turret's
     * head can be hurt; missiles come down on red marks; rings of shots */
    B.y = (sst_g.cam_y + 34) * FX;
    int ty = SST_PX(B.y) >> 4;
    int lo = -1, hi = -1;
    for (int tx = 0; tx < SST_COLS; tx++) {
        int t = sst_tile_at(tx, ty + 1);
        if (!sst_tile_pit(t) && !sst_tile_solid(t)) { if (lo < 0) lo = tx; hi = tx; }
    }
    if (lo >= 0) {
        int want = (lo + hi + 1) * 8 * FX;
        B.x += iclamp(want - B.x, -200, 200);
    }
    B.part[0].x = B.x;
    B.part[0].y = B.y;
    B.part[1].x = B.x;
    B.part[1].y = B.y - 6 * FX;
    if (!firing_ok()) return;
    if (B.t % 140 == 70) {
        for (int k = -1; k <= 1; k++) {
            int x = sst_g.pl.x + k * 26 * FX, y = sst_g.pl.y + (k ? 8 : -6) * FX;
            int i = sst_add_bullet(EB_ROCKET, x, y, 0, 0);
            if (i >= 0) sst_g.eb[i].a = 56;
            sst_add_haz(HZ_MARK, x, y, 56, 16);
        }
        sst_sfx("sst_launch");
    }
    if (B.t % 110 == 0) { sst_ring(EB_ROUND, B.part[1].x, B.part[1].y, 10, 270, B.t % 7); sst_sfx("sst_ring"); }
}

static const int8_t SPIKE_PAT[3][8][2] = {
    {{3, 3}, {16, 3}, {3, 8}, {16, 8}, {7, 5}, {12, 5}, {9, 9}, {10, 2}},
    {{5, 2}, {14, 2}, {2, 6}, {17, 6}, {6, 9}, {13, 9}, {9, 4}, {10, 7}},
    {{4, 4}, {15, 4}, {4, 9}, {15, 9}, {8, 2}, {11, 2}, {8, 10}, {11, 10}},
};

static void doppels(void) {
    /* two of them on one bar, changing shape together: urchins that dive
     * across, devil faces with spreads, aliens that lay spike blocks */
    B.pt++;
    if (B.pt >= 420) { B.phase = (uint8_t)((B.phase + 1) % 3); B.pt = 0; }
    int top = SST_PX(B.y) - 24;
    for (int k = 0; k < 2; k++) {
        SstPart *p = &B.part[k];
        int t = (B.pt + k * 60) % 140;
        switch (B.phase) {
        case 0: /* urchins: aim, then dive across in a line */
            if (t < 40) {
                move_to(p, (k ? 250 : 70) * FX, (top + 30 + k * 60) * FX, 10);
                p->a = pang(p->x, p->y);
            } else if (t < 100) {
                p->x += 1100 * sst_cos(p->a) / 256;
                p->y += 1100 * sst_sin(p->a) / 256;
                p->x = iclamp(p->x, 20 * FX, 300 * FX);
                p->y = iclamp(p->y, (top + 10) * FX, (top + 150) * FX);
                if (t == 40) sst_sfx("sst_rush");
            }
            break;
        case 1: /* devil faces */
            move_to(p, (k ? 240 : 80) * FX + sst_sin(B.t * 2 + k * 32) * 30 * FX / 256, (top + 34) * FX, 12);
            if (t % 70 == 35 && firing_ok()) { fan(EB_ROUND, p->x, p->y, 5, 4, 330); sst_sfx("sst_fire"); }
            break;
        default: /* aliens */
            move_to(p, (k ? 230 : 90) * FX, (top + 30) * FX + sst_sin(B.t * 2 + k * 20) * 10 * FX / 256, 12);
            if (t % 80 == 40 && firing_ok()) aim(EB_BIG, p->x, p->y, 260, 0);
            break;
        }
    }
    if (B.phase == 2 && (B.pt == 20 || B.pt == 220)) {
        /* new spike blocks: the old ones go */
        for (int i = 0; i < SST_MAX_FOES; i++)
            if (sst_g.foe[i].alive && sst_g.foe[i].kind == FK_SPIKE) sst_g.foe[i].alive = 0;
        int pat = (B.pt / 200 + B.t / 420) % 3;
        for (int s = 0; s < 8; s++) {
            int x = SPIKE_PAT[pat][s][0] * 16 + 8, y = top - 24 + SPIKE_PAT[pat][s][1] * 16 + 8;
            if (sst_dist(x - SST_PX(sst_g.pl.x), y - SST_PX(sst_g.pl.y)) < 24) continue;
            int j = sst_spawn_foe(FK_SPIKE, x * FX, y * FX);
            if (j >= 0) sst_g.foe[j].b = 400;
        }
        sst_sfx("sst_spikes");
    }
    if (B.phase != 2)
        for (int i = 0; i < SST_MAX_FOES; i++)
            if (sst_g.foe[i].alive && sst_g.foe[i].kind == FK_SPIKE && sst_g.foe[i].b > 30) sst_g.foe[i].b = 30;
}

static const int16_t WICKS_SPOT[6][2] = {{160, 40}, {70, 60}, {250, 60}, {100, 110}, {220, 110}, {160, 85}};

static void wicks(void) {
    /* blinks from place to place, lays eggs that hatch into hopping skulls;
     * the crypt's four torches keep sending out roaming fire */
    SstPart *w = &B.part[0];
    int top = SST_PX(B.y) - 40;
    int k = B.t % 160;
    w->a = k >= 140 ? 2 : k < 20 && B.t > 20 ? 1 : 0; /* 2 fading out, 1 fading in */
    if (k == 150) {
        int s = (B.t / 160 * 7 + 3) % 6;
        if (sst_dist(WICKS_SPOT[s][0] - SST_PX(sst_g.pl.x), top + WICKS_SPOT[s][1] - SST_PX(sst_g.pl.y)) < 40) s = (s + 3) % 6;
        w->x = WICKS_SPOT[s][0] * FX;
        w->y = (top + WICKS_SPOT[s][1]) * FX;
        sst_sfx("sst_blink");
    }
    if (k == 40 && firing_ok()) {
        for (int e = -1; e <= 1; e += 2) {
            int j = sst_spawn_foe(FK_EGG, w->x + e * 14 * FX, w->y + 8 * FX);
            if (j >= 0) sst_g.foe[j].t = 0;
        }
        sst_sfx("sst_egg");
    }
    if (k == 90 && firing_ok()) fan(EB_ROUND, w->x, w->y, 3, 5, 320);
    for (int i = 1; i <= 4; i++) {
        SstPart *tp = &B.part[i];
        if ((B.t + i * 45) % 180 == 0 && firing_ok()) {
            int j = sst_spawn_foe(FK_WISP, tp->x + (i % 2 ? 12 : -12) * FX, tp->y + (i <= 2 ? 12 : -12) * FX);
            if (j >= 0) sst_g.foe[j].t = 0;
        }
    }
}

static void ribbon(void) {
    /* slithers about the ballroom: only the head; the body blocks shots;
     * rings from the head; the tail leaves slime */
    SstPart *h = &B.part[0];
    int top = SST_PX(B.y) - 40;
    if (B.t % 90 == 1) {
        if ((B.t / 90) % 3 == 2) { B.a = SST_PX(sst_g.pl.x); B.b = SST_PX(sst_g.pl.y); }
        else { B.a = rng_range(&sst_g.rng, 40, 280); B.b = top + rng_range(&sst_g.rng, 20, 150); }
    }
    int a = sst_dir_to(B.a * FX - h->x, B.b * FX - h->y);
    int cur = (h->vx || h->vy) ? sst_dir_to(h->vx, h->vy) : a;
    int d = ((a - cur + 96) & 63) - 32;
    cur += iclamp(d, -2, 2);
    h->vx = 380 * sst_cos(cur) / 256;
    h->vy = 380 * sst_sin(cur) / 256;
    h->x = iclamp(h->x + h->vx, 24 * FX, 296 * FX);
    h->y = iclamp(h->y + h->vy, (top + 12) * FX, (top + 168) * FX);
    for (int k = 1; k <= 10; k++) {
        SstPart *s = &B.part[k], *pv = &B.part[k - 1];
        int dx = SST_PX(pv->x - s->x), dy = SST_PX(pv->y - s->y);
        int dd = sst_dist(pv->x - s->x, pv->y - s->y);
        if (dd > 9 * FX) {
            s->x += (pv->x - s->x) * (dd - 9 * FX) / dd;
            s->y += (pv->y - s->y) * (dd - 9 * FX) / dd;
        }
        (void)dx;
        (void)dy;
    }
    if (B.t % 9 == 0) sst_add_haz(HZ_SLIME, B.part[10].x, B.part[10].y, 240, 7);
    if (B.t % 150 == 75 && firing_ok()) { sst_ring(EB_ROUND, h->x, h->y, 8, 300, B.t % 8); sst_sfx("sst_ring"); }
}

static void morrow(void) {
    /* the magician: shots streamed from the hat, a star wall to dash
     * through, a swarm of bats, and a split into two that fire together */
    SstPart *m0 = &B.part[0], *m1 = &B.part[1];
    static const int DUR[4] = {170, 120, 130, 300};
    B.pt++;
    if (B.pt > DUR[B.phase] + 30) {
        B.phase = (uint8_t)((B.phase + 1) % 4);
        B.pt = 0;
    }
    int t = B.pt, ph = B.phase;
    int hx = B.x + sst_sin(B.t) * 40, hy = B.y + 10 * FX + sst_sin(B.t * 2) * 6;
    if (ph != 3 || t >= DUR[3]) {
        move_to(m0, hx, hy, 8);
        if (m1->alive) {
            move_to(m1, hx, hy, 8);
            if (iabs(m1->x - m0->x) < 4 * FX) m1->alive = 0;
        }
    }
    if (!firing_ok() || t >= DUR[ph]) return;
    switch (ph) {
    case 0:
        if (t % 5 == 0) {
            int a = 16 + sst_sin(t * 2) * 14 / 256;
            sst_add_bullet(EB_ROUND, m0->x, m0->y - 14 * FX, 360 * sst_cos(a) / 256, 360 * sst_sin(a) / 256);
        }
        break;
    case 1:
        if (t == 40 || t == 70) { sst_ring(EB_STAR, m0->x, m0->y, 56, 300, t == 70 ? 1 : 0); sst_sfx("sst_stars"); }
        break;
    case 2:
        if (t % 10 == 0 && t <= 70) {
            int j = sst_spawn_foe(FK_SWOOP, m0->x, m0->y - 12 * FX);
            if (j >= 0) {
                sst_g.foe[j].t = 0;
                sst_g.foe[j].vx = (t % 20 ? 400 : -400);
            }
            sst_sfx("sst_flap");
        }
        break;
    case 3:
        if (t == 1) {
            m1->alive = 1;
            m1->x = m0->x;
            m1->y = m0->y;
            sst_sfx("sst_blink");
        }
        if (t < 260) {
            move_to(m0, 44 * FX, B.y + 50 * FX, 6);
            move_to(m1, 276 * FX, B.y + 50 * FX, 6);
            if (t > 40 && t % 44 == 0) {
                fan(EB_ROUND, m0->x, m0->y, 3, 4, 340);
                fan(EB_ROUND, m1->x, m1->y, 3, 4, 340);
                sst_sfx("sst_fire");
            }
        }
        break;
    }
}

static void maw(void) {
    /* the magician's true face: the two great eyes hold its life; the eyes
     * on its arms fire rounds and can be shot shut a while; the mouth lets
     * out a snake that only shooting drives back */
    int hp = 0;
    for (int k = 0; k < 2; k++) hp += imax(0, B.part[k].hp);
    B.hp = hp;
    for (int k = 2; k < 6; k++) {
        SstPart *e = &B.part[k];
        e->t++;
        if (e->shut) {
            if (--e->b <= 0) { e->shut = 0; e->hp = 6; }
            continue;
        }
        int c = e->t % 200;
        if ((c == 0 || c == 16 || (c == 32 && k % 2)) && firing_ok()) sst_ring(EB_ROUND, e->x, e->y, 10, 250, c / 16 * 3);
    }
    for (int k = 0; k < 2; k++) {
        SstPart *e = &B.part[k];
        if (e->hp <= 0) { e->shut = 2; continue; }
        if (B.t % 150 == 75 + k * 30 && firing_ok()) fan(EB_BIG, e->x, e->y, 3, 5, 280);
    }
    /* the snake: its length (a, px*FX), pushed back by hits */
    SstPart *s = &B.part[7];
    SstPart *mo = &B.part[6];
    if (s->b > 0) {
        s->b--;
        s->a = imax(0, s->a - 512);
    } else {
        s->a = imin(s->a + 150, 150 * FX);
    }
    int ang = s->t ? s->t : 16;
    int want = pang(mo->x, mo->y);
    int d = ((want - ang + 96) & 63) - 32;
    if (B.t % 3 == 0) ang += isign(d);
    ang = iclamp(ang, 4, 28);
    s->t = ang;
    s->x = mo->x + SST_PX(s->a) * sst_cos(ang);
    s->y = mo->y + SST_PX(s->a) * sst_sin(ang);
}

static void heart(void) {
    /* 23 blocks circle clockwise; red ones fire across the room, green ones
     * break; with twelve left the red ones gather into a heart that sprays
     * rings until it is broken */
    int left = 0, greens = 0, hp = 0;
    for (int k = 0; k < 23; k++) {
        SstPart *p = &B.part[k];
        if (!p->alive) continue;
        left++;
        greens += p->kind == PK_GREEN;
        hp += p->hp;
    }
    B.hp = hp;
    if (greens == 0 && B.phase == 0) {
        B.phase = 1;
        B.pt = 0;
        sst_sfx("sst_heart");
    }
    B.pt++;
    if (B.phase == 0) {
        int spin = B.t / 3;
        for (int k = 0; k < 23; k++) {
            SstPart *p = &B.part[k];
            if (!p->alive) continue;
            int a = (p->a + spin * 1) / 4;
            p->x = B.x + 76 * sst_cos(a);
            p->y = B.y + 64 * sst_sin(a);
        }
        if (B.t % 100 == 50 && firing_ok()) {
            for (int n = 0; n < 3; n++) {
                int k = ((B.t / 100) * 5 + n * 8) % 23;
                if (k % 2) k = (k + 1) % 23;
                SstPart *p = &B.part[k];
                if (!p->alive) continue;
                int a = sst_dir_to(B.x - p->x, B.y - p->y);
                sst_add_bullet(EB_BIG, p->x, p->y, 300 * sst_cos(a) / 256, 300 * sst_sin(a) / 256);
            }
            sst_sfx("sst_fire");
        }
    } else {
        int n = 0;
        for (int k = 0; k < 23; k++) {
            SstPart *p = &B.part[k];
            if (!p->alive) continue;
            int i = n++ % 12;
            move_to(p, B.x + HEART_PT[i][0] * FX, B.y + HEART_PT[i][1] * FX, 10);
        }
        if (B.pt > 60 && B.pt % 36 == 0 && firing_ok()) {
            sst_ring(EB_ROUND, B.x, B.y, 16, 240, (B.pt / 36) % 2 * 2);
            sst_sfx("sst_ring");
        }
    }
    if (left == 0) B.hp = 0;
}

/* ---- the shared parts --------------------------------------------------------------- */

static bool weak_part(const SstPart *p) {
    switch (p->kind) {
    case PK_WEAK: return true;
    case PK_EYE: return !p->shut && p->hp > 0;
    case PK_GREEN: return true;
    case PK_RED: return B.phase == 1;
    default: return false;
    }
}

static bool part_box(const SstPart *p, int x, int y, int hw) {
    return iabs(SST_PX(p->x - x)) < p->hw + hw && iabs(SST_PX(p->y - y)) < p->hh + hw;
}

static void hurt_boss(SstPart *p, int dmg) {
    if (B.dying) return;
    B.hurt = 4;
    sst_sfx("sst_bosshit");
    if (B.kind == BK_MAW || B.kind == BK_HEART) {
        p->hp -= dmg;
        if (p->hp <= 0) {
            if (B.kind == BK_MAW && p->kind == PK_EYE) {
                if (p - B.part >= 2) { p->shut = 1; p->b = 300; }
                sst_sfx("sst_shut");
            } else {
                p->alive = 0;
                sst_sfx("sst_crate");
            }
        }
        return;
    }
    B.hp -= dmg;
}

bool sst_boss_shot(int x, int y, int hw, int dmg) {
    if (!B.alive || B.dying) return false;
    if (B.kind == BK_WICKS && B.part[0].a) {
        /* fading: shots pass through him */
        for (int k = 1; k < B.nparts; k++)
            if (B.part[k].alive && part_box(&B.part[k], x, y, hw)) return true;
        return false;
    }
    /* the snake's head: hits drive it back */
    if (B.kind == BK_MAW) {
        SstPart *s = &B.part[7];
        if (s->a > 8 * FX && part_box(s, x, y, hw)) {
            s->a = imax(0, s->a - 6 * FX);
            s->b = 6;
            sst_sfx("sst_hit");
            return true;
        }
    }
    /* the soft spots first ... */
    for (int k = 0; k < B.nparts; k++) {
        SstPart *p = &B.part[k];
        if (!p->alive || p->kind == PK_TORCH || p->kind == PK_SNAKE) continue;
        if (!part_box(p, x, y, hw)) continue;
        if (weak_part(p) || (B.kind == BK_ENGINE && p->kind == PK_BODY)) { hurt_boss(p, dmg); return true; }
    }
    /* ... then armour, which stops the shot (the Boiler's low hull doesn't:
     * shots fly over it at the turret) */
    for (int k = 0; k < B.nparts; k++) {
        SstPart *p = &B.part[k];
        if (!p->alive || !part_box(p, x, y, hw)) continue;
        if (B.kind == BK_BOILER && p->kind == PK_BODY) continue;
        if (p->kind == PK_BODY || p->kind == PK_SEG || p->kind == PK_RED || p->kind == PK_MOUTH || p->kind == PK_HAND ||
            (p->kind == PK_EYE && p->shut)) {
            sst_sfx("sst_hit");
            return true;
        }
    }
    return false;
}

bool sst_boss_touch(int x, int y, int hw) {
    if (!B.alive || B.dying || B.t < SST_BOSS_INTRO) return false;
    for (int k = 0; k < B.nparts; k++) {
        const SstPart *p = &B.part[k];
        if (!p->alive || p->kind == PK_TORCH || p->kind == PK_MOUTH) continue;
        if (B.kind == BK_WICKS && k == 0 && p->a) continue;
        if (p->kind == PK_EYE && B.kind == BK_MAW) continue;
        if (p->kind == PK_SNAKE) {
            /* the snake's whole length */
            const SstPart *m = &B.part[6];
            for (int s = 0; s <= 6; s++) {
                int sx = m->x + (p->x - m->x) * s / 6, sy = m->y + (p->y - m->y) * s / 6;
                if (p->a > 10 * FX && iabs(SST_PX(sx - x)) < 5 + hw && iabs(SST_PX(sy - y)) < 5 + hw) return true;
            }
            continue;
        }
        if (part_box(p, x, y, hw)) return true;
    }
    return false;
}

int sst_boss_weak(int *x, int *y) {
    if (!B.alive || B.dying) return 0;
    int best = 1 << 30, found = 0;
    for (int k = 0; k < B.nparts; k++) {
        const SstPart *p = &B.part[k];
        if (!p->alive || !(weak_part(p) || (B.kind == BK_ENGINE && p->kind == PK_BODY))) continue;
        if (B.kind == BK_WICKS && p->a) continue;
        if (B.kind == BK_MAW && k >= 2 && k < 6) continue;
        int d = sst_dist(SST_PX(p->x - sst_g.pl.x), SST_PX(p->y - sst_g.pl.y));
        if (d < best) { best = d; *x = p->x; *y = p->y; found = 1; }
    }
    if (B.kind == BK_MAW && B.part[7].a > 40 * FX) { *x = B.part[7].x; *y = B.part[7].y; found = 1; }
    return found;
}

void sst_boss_update(void) {
    if (!B.alive) return;
    if (B.hurt > 0) B.hurt--;
    if (B.dying) {
        B.die_t++;
        if (B.die_t % 8 == 1 && !sst_sim) sst_sfx("sst_boom");
        if (B.die_t % 6 == 1) sst_add_haz(HZ_PBLAST, B.x + rng_range(&sst_g.rng, -30, 30) * FX, B.y + rng_range(&sst_g.rng, -20, 20) * FX, 10, 14);
        if (B.die_t > 90) B.alive = 0;
        return;
    }
    B.t++;
    /* a moment to come on before it fights */
    if (B.t < SST_BOSS_INTRO && B.kind != BK_BOILER) return;
    switch (B.kind) {
    case BK_ENGINE: engine(); break;
    case BK_GOGGLE: goggle(); break;
    case BK_BOILER: boiler(); break;
    case BK_DOPPELS: doppels(); break;
    case BK_WICKS: wicks(); break;
    case BK_RIBBON: ribbon(); break;
    case BK_MORROW: morrow(); break;
    case BK_MAW: maw(); break;
    case BK_HEART: heart(); break;
    default: break;
    }
    if (B.hp <= 0) {
        B.hp = 0;
        B.dying = 1;
        B.die_t = 1;
        memset(sst_g.eb, 0, sizeof sst_g.eb);
        /* its minions go with it */
        for (int i = 0; i < SST_MAX_FOES; i++) {
            int k = sst_g.foe[i].kind;
            if (sst_g.foe[i].alive && (k == FK_MOUSE || k == FK_EGG || k == FK_SKULL || k == FK_WISP || k == FK_SWOOP || k == FK_SPIKE))
                sst_g.foe[i].alive = 0;
        }
        sst_sfx("sst_bossdie");
    }
}
