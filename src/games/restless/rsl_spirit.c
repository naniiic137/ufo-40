/* RESTLESS - the Low Glow. After every death Gaunt's wisp must gather the
 * pieces of his soul, each guarded by a spirit that circles it and lunges.
 * One touch and it is over for good. Each piece taken keeps the wisp safe
 * for a moment, so a brave player threads from piece to piece. From four
 * deaths, blue flames bounce about as well. */
#include "rsl.h"

#define ARENA_X0 14
#define ARENA_Y0 24
#define ARENA_X1 306
#define ARENA_Y1 170

int rsl_pieces_for(int deaths) {
    static const int N[6] = {1, 1, 3, 6, 9, 12};
    return N[iclamp(deaths, 0, 5)];
}

static int dl_cap(int deaths) { return iclamp(deaths, 1, 5); }

void rsl_spirit_start(void) {
    RslSpirit *s = &rg.sp;
    memset(s, 0, sizeof *s);
    s->x = 160 * RSL_FX;
    s->y = 100 * RSL_FX;
    s->n = rsl_pieces_for(rg.deaths);
    s->inv = 60;
    int dl = dl_cap(rg.deaths);
    for (int i = 0; i < s->n; i++) {
        int px = 0, py = 0;
        for (int tries = 0; tries < 200; tries++) {
            px = rng_range(&rg.rng, ARENA_X0 + 22, ARENA_X1 - 22);
            py = rng_range(&rg.rng, ARENA_Y0 + 20, ARENA_Y1 - 20);
            bool ok = rsl_dist(px - 160, py - 100) > 46;
            for (int j = 0; j < i && ok; j++)
                ok = rsl_dist(px - RSL_PX(s->piece[j].x), py - RSL_PX(s->piece[j].y)) > 36;
            if (ok) break;
        }
        s->piece[i].x = px * RSL_FX;
        s->piece[i].y = py * RSL_FX;
        RslGuard *g = &s->guard[s->nguards++];
        memset(g, 0, sizeof *g);
        g->piece = i;
        g->ang = rng_range(&rg.rng, 0, 63) * 16;
        g->r = 20;
        g->speed = (10 + dl * 4) * (i % 2 ? -1 : 1);
        g->x = s->piece[i].x + rsl_cos(g->ang >> 4) * g->r;
        g->y = s->piece[i].y + rsl_sin(g->ang >> 4) * g->r;
        g->lunge = -rng_range(&rg.rng, 30, 90);
    }
    /* blue flames from four deaths */
    int flames = rg.deaths >= 4 ? imin(4, rg.deaths - 2) : 0;
    for (int k = 0; k < flames && s->nguards < RSL_MAX_GUARDS; k++) {
        RslGuard *g = &s->guard[s->nguards++];
        memset(g, 0, sizeof *g);
        g->piece = -1;
        g->x = (k % 2 ? ARENA_X1 - 10 : ARENA_X0 + 10) * RSL_FX;
        g->y = (k < 2 ? ARENA_Y0 + 10 : ARENA_Y1 - 10) * RSL_FX;
        g->vx = (k % 2 ? -1 : 1) * (200 + dl * 20);
        g->vy = (k < 2 ? 1 : -1) * (150 + dl * 15);
    }
    rsl_music(RSL_MUS_SPIRIT);
}

/* one frame of one guardian: a pure function of the wisp's place, so the
 * demo player can look ahead */
void rsl_guard_step(RslGuard *g, const RslSpirit *s, int deaths) {
    int dl = dl_cap(deaths);
    g->t++;
    if (g->piece < 0) {
        g->x += g->vx;
        g->y += g->vy;
        if (g->x < ARENA_X0 * RSL_FX || g->x > ARENA_X1 * RSL_FX) g->vx = -g->vx;
        if (g->y < ARENA_Y0 * RSL_FX || g->y > ARENA_Y1 * RSL_FX) g->vy = -g->vy;
        return;
    }
    const RslPiece *p = &s->piece[g->piece];
    if (g->lunge > 0) {
        g->x += g->vx;
        g->y += g->vy;
        g->lunge--;
        if (g->lunge == 0) g->lunge = -imax(40, 100 - dl * 10);
        return;
    }
    if (g->lunge < 0) g->lunge++;
    g->ang += g->speed;
    int a = (g->ang >> 4) & 63;
    int tx = p->x + rsl_cos(a) * g->r, ty = p->y + rsl_sin(a) * g->r;
    int dx = tx - g->x, dy = ty - g->y;
    int d = rsl_dist(dx / 16, dy / 16) * 16; /* fixed point */
    int maxs = 2 * RSL_FX;
    if (d > maxs) {
        g->x += (int)((long long)dx * maxs / d);
        g->y += (int)((long long)dy * maxs / d);
    } else {
        g->x = tx;
        g->y = ty;
    }
    if (g->lunge == 0) {
        int range = 34 + dl * 4;
        int ex = s->x - g->x, ey = s->y - g->y;
        if (rsl_dist(ex / RSL_FX, ey / RSL_FX) < range) {
            int sp = 230 + dl * 34;
            int b = rsl_dir_to(ex, ey);
            g->vx = rsl_cos(b) * sp / 256;
            g->vy = rsl_sin(b) * sp / 256;
            g->lunge = 26;
        }
    }
}

void rsl_spirit_update(void) {
    RslSpirit *s = &rg.sp;
    s->t++;
    if (s->failed) {
        s->done_t++;
        return;
    }
    if (s->got >= s->n) {
        s->done_t++;
        return;
    }
    if (s->inv > 0) s->inv--;
    /* the wisp flies eight ways */
    int dx = (rsl_btn(BTN_RIGHT) ? 1 : 0) - (rsl_btn(BTN_LEFT) ? 1 : 0);
    int dy = (rsl_btn(BTN_DOWN) ? 1 : 0) - (rsl_btn(BTN_UP) ? 1 : 0);
    int sp = dx && dy ? 272 : 384;
    if (s->t > 30) {
        s->x = iclamp(s->x + dx * sp, ARENA_X0 * RSL_FX, ARENA_X1 * RSL_FX);
        s->y = iclamp(s->y + dy * sp, ARENA_Y0 * RSL_FX, ARENA_Y1 * RSL_FX);
        for (int i = 0; i < s->nguards; i++) rsl_guard_step(&s->guard[i], s, rg.deaths);
    }
    for (int i = 0; i < s->n; i++) {
        RslPiece *p = &s->piece[i];
        if (p->taken) continue;
        if (rsl_dist((p->x - s->x) / RSL_FX, (p->y - s->y) / RSL_FX) < 9) {
            p->taken = true;
            s->got++;
            s->inv = RSL_SPIRIT_PICK_INV;
            rsl_sfx(s->got == s->n ? "rsl_whole" : "rsl_piece");
        }
    }
    if (s->got >= s->n) return;
    if (s->inv == 0 && !rg.god) {
        for (int i = 0; i < s->nguards; i++) {
            const RslGuard *g = &s->guard[i];
            if (rsl_dist((g->x - s->x) / RSL_FX, (g->y - s->y) / RSL_FX) < (g->piece < 0 ? 8 : 8)) {
                s->failed = true;
                rsl_sfx("rsl_final");
                rsl_music_stop();
                break;
            }
        }
    }
}
