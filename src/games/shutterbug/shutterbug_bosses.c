/* SHUTTERBUG - the two mid-bosses, the three stage bosses and the true
 * boss. Each has its own rule for the camera: Madame Scone is simply
 * stunned; the Teapot only shows its weak spot (under the lid) in a photo;
 * Old Croak can be left alone and slinks off; the Signalman is a ghost and
 * only a photo makes him solid; King Thunderjaw is hurt only in the mouth,
 * which a photo forces open; the Kaleidoscope's shards unwind from its core
 * when the core is photographed. See docs/games/24-shutterbug.md. */
#include "shutterbug.h"

#define CROAK_LEAVES 1500   /* Old Croak gives up and slinks into the chasm */
#define KALEI_SHARDS 8
#define KALEI_UNWOUND 330

static float scr_x(const Foe *e) { return e->x - sb.cam_x; }

/* screen position -> world */
static float wx(float sx) { return sb.cam_x + sx; }
static float wy(float sy) { return sb.cam_y + sy; }

void shb_boss_init(Foe *e) {
    e->state = 0;   /* coming in */
    e->st = 0;
    e->open = false;
    switch (e->kind) {
    case K_SCONE: case K_TEAPOT: case K_CROAK: case K_SIGNAL: case K_JAW:
        e->x = wx(SCREEN_W + 40);
        break;
    case K_KALEI:
        e->x = wx(SCREEN_W + 30);
        e->y = wy(SHB_PF_H / 2);
        break;
    default: break;
    }
    if (e->kind == K_TEAPOT) e->y = wy(70);
    if (e->kind == K_SCONE) e->y = wy(84);
    if (e->kind == K_SIGNAL) e->y = wy(84);
    if (e->kind == K_CROAK) e->y = wy(SHB_PF_H - 3 * SHB_TILE - 15);
    if (e->kind == K_JAW) e->y = wy(84);
}

static int find_boss(int kind) {
    for (int i = 0; i < SHB_MAX_FOES; i++)
        if (sb.foe[i].alive && sb.foe[i].kind == kind) return i;
    return -1;
}

static int count_kind(int kind) {
    int n = 0;
    for (int i = 0; i < SHB_MAX_FOES; i++) n += sb.foe[i].alive && sb.foe[i].kind == kind;
    return n;
}

bool shb_boss_photo(int i, int photo) {
    Foe *e = &sb.foe[i];
    switch (e->kind) {
    case K_SHARD: {
        /* the shards themselves don't mind; it's the core that counts */
        return false;
    }
    case K_KALEI:
        if (e->state < 1) return false;
        e->stun = SHB_BOSS_STUN_T;
        e->photo = photo;
        if (!e->open) {
            e->open = true;
            e->st = 0;
            e->phase = 1; /* the shards unwind and wander */
            sfx_play_name("shb_unwind");
        }
        return true;
    case K_TEAPOT:
        e->stun = SHB_BOSS_STUN_T;
        e->photo = photo;
        e->open = true; /* the lid pops: the weak spot shows */
        sfx_play_name("shb_lid");
        return true;
    case K_JAW:
        e->stun = SHB_BOSS_STUN_T;
        e->photo = photo;
        e->open = true; /* the flash forces the jaw open */
        return true;
    case K_CHUTE:
        e->stun = SHB_STUN_T;
        e->photo = photo;
        return true;
    default:
        e->stun = SHB_BOSS_STUN_T;
        e->photo = photo;
        return true;
    }
}

bool shb_boss_hit(int i, int dmg, float x, float y) {
    Foe *e = &sb.foe[i];
    (void)dmg;
    (void)x;
    (void)y;
    if (e->state == 0 && e->kind != K_CHUTE) return false; /* still coming in */
    switch (e->kind) {
    case K_TEAPOT: return e->open;
    case K_SIGNAL: return e->stun > 0;
    case K_JAW: return e->open;
    case K_KALEI: return e->open;
    case K_SHARD: return false;
    default: return true;
    }
}

void shb_boss_dead(int i) {
    Foe *e = &sb.foe[i];
    switch (e->kind) {
    case K_TEAPOT: case K_SIGNAL: case K_JAW: case K_KALEI:
        /* the stage's boss: everything else flees */
        for (int k = 0; k < SHB_MAX_FOES; k++) {
            Foe *o = &sb.foe[k];
            if (!o->alive || o->role == ROLE_PROP) continue;
            shb_burst(o->x, o->y, C_GREY, 6, 1.0f);
            o->alive = 0;
        }
        shb_stage_won();
        /* the first planet's boss beaten: the Beacon goal */
        if (e->kind == K_TEAPOT && !sb.beacon_given) { sb.beacon_given = true; game_award(GOAL_BEACON); }
        /* King Thunderjaw beaten is the game beaten (the Saucer), true boss or not */
        if (e->kind == K_JAW) game_award(GOAL_SAUCER);
        break;
    default: break;
    }
}

/* ------------------------------------------------------------------------ */

static void enter(Foe *e, float sx, float speed) {
    e->x -= speed;
    if (scr_x(e) <= sx) { e->x = wx(sx); e->state = 1; e->st = 0; }
}

static void scone_update(Foe *e) {
    if (e->state == 0) { enter(e, 236, 1.2f); return; }
    if (e->stun > 0) { e->stun--; return; }
    e->st++;
    e->y = wy(84 + sinf(e->st * 0.02f) * 40);
    e->x = wx(236 + sinf(e->st * 0.013f) * 30);
    /* a volley of raisins that spreads as it flies */
    if (e->st % 100 == 40) { shb_aimed(e->x - 16, e->y, 1.5f, 5, 13); shb_sfx("shb_volley", 6); }
    /* crumbs fall from the sky; every other one hides a bulb */
    if (e->st % 64 == 0) {
        int j = shb_add_eshot(ES_FALL, wx(40 + (e->st * 37) % 200), wy(-4), 0, 0.4f);
        if (j >= 0) { sb.es[j].arg = (e->st / 64) % 2; sb.es[j].r = 3.5f; }
    }
}

static void teapot_update(Foe *e) {
    if (e->state == 0) {
        enter(e, 220, 1.4f);
        if (e->state == 1) { e->vx = -1.1f; e->vy = 0.8f; }
        return;
    }
    if (e->stun > 0) {
        e->stun--;
        if (e->stun == 0) e->open = false;
        return;
    }
    e->st++;
    /* it bounces about the room */
    float nx = e->x + e->vx, ny = e->y + e->vy;
    e->vy = fminf(e->vy + 0.035f, 2.6f);
    if (scr_x(e) + e->vx < 40 || scr_x(e) + e->vx > SCREEN_W - 24) e->vx = -e->vx;
    if (shb_solid_at(nx, ny + e->hh) || ny + e->hh > wy(SHB_PF_H - 2)) { e->vy = -2.4f - (e->st % 3) * 0.25f; ny = e->y; shb_sfx("shb_bonk", 8); }
    if (shb_solid_at(nx, ny - e->hh) && e->vy < 0) e->vy = 0.5f;
    e->x += e->vx;
    e->y = ny;
    if (e->st % 120 == 60) { shb_aimed(e->x - 10, e->y - 6, 1.6f, 3, 18); shb_sfx("shb_volley", 6); }
    /* drips from the ceiling (shoot them for bulbs) */
    if (e->st % 56 == 20) {
        /* out of one of the four holes in the ceiling */
        float hx = 580 * SHB_TILE + ((e->st / 56) % 4) * 10 * SHB_TILE + SHB_TILE;
        int j = shb_add_eshot(ES_FALL, hx, wy(10), 0, 0.3f);
        if (j >= 0) sb.es[j].arg = 1;
    }
    /* toasties come in from the far end and pop crumbs that bounce */
    if (e->st % 300 == 150 && count_kind(K_TOAST) < 2) {
        int j = shb_spawn(K_TOAST, wx(SCREEN_W + 6), wy(SHB_PF_H - 20), (e->st / 300) % 2 ? F_CEIL | F_CRYSTAL : F_CRYSTAL, 0);
        (void)j;
    }
}

static void croak_update(Foe *e) {
    if (e->state == 0) {
        enter(e, 230, 1.2f);
        if (e->state == 1) e->ay = e->y;
        return;
    }
    if (e->state == 2) {
        /* slinking off into the chasm */
        e->y += 1.5f;
        if (e->y > wy(SHB_PF_H + 40)) e->alive = 0;
        return;
    }
    if (e->stun > 0) { e->stun--; return; }
    e->st++;
    if (e->st > CROAK_LEAVES) { e->state = 2; sb.msg = "OLD CROAK SLINKS AWAY"; sb.msg_t = 90; return; }
    /* hops about, and spits shots that ricochet */
    if (e->phase == 0) {
        if (e->st % 80 == 0) {
            int p = shb_near_ship(e->x, e->y);
            float tx = p >= 0 ? sb.ship[p].x : wx(160);
            e->vx = fclamp((tx - e->x) / 60.0f, -1.6f, 1.6f);
            if (scr_x(e) < 120) e->vx = fabsf(e->vx);
            e->vy = -3.0f;
            e->phase = 1;
        }
    } else {
        e->vy += 0.09f;
        e->x += e->vx;
        e->y += e->vy;
        if (scr_x(e) > SCREEN_W - 20) e->x = wx(SCREEN_W - 20);
        if (scr_x(e) < 60) e->x = wx(60);
        if (e->vy > 0 && (shb_solid_at(e->x, e->y + e->hh) || e->y >= e->ay)) {
            e->y = fminf(e->y, e->ay);
            e->phase = 0;
            for (int k = 0; k < 3; k++) {
                float a = 3.14159f + (k - 1) * 0.5f - 0.3f;
                int j = shb_add_eshot(ES_BOUNCE, e->x - 8, e->y - 6, cosf(a) * 1.8f, sinf(a) * 1.8f);
                if (j >= 0) sb.es[j].bounces = 3;
            }
            shb_sfx("shb_croak", 8);
        }
    }
}

/* the Signalman's three tracks, as screen heights */
static const int TRACK_Y[3] = {40, 84, 128};

static void signal_update(Foe *e) {
    if (e->state == 0) { enter(e, 262, 1.0f); return; }
    if (e->stun > 0) { e->stun--; return; }
    e->st++;
    e->y = wy(84 + sinf(e->st * 0.018f) * 50);
    /* a green spark down to a track, where it sets pink sparks crawling */
    if (e->st % 150 == 30) {
        int tr = (e->st / 150) % 3;
        int j = shb_add_eshot(ES_BIG, e->x - 10, e->y, -1.2f, (wy(TRACK_Y[tr]) - e->y) / 40.0f);
        if (j >= 0) { sb.es[j].arg = 100 + tr; sb.es[j].hp = 40; }
        shb_sfx("shb_spark", 6);
    }
    for (int k = 0; k < SHB_MAX_ESHOTS; k++) {
        EShot *s = &sb.es[k];
        if (!s->alive || s->kind != ES_BIG || s->arg < 100) continue;
        if (--s->hp <= 0) {
            int tr = s->arg - 100;
            float ty = wy(TRACK_Y[tr]);
            for (int d = -1; d <= 1; d += 2) {
                int j = shb_add_eshot(ES_CRAWL, s->x, ty, d * 1.1f, 0);
                (void)j;
            }
            s->alive = 0;
            shb_burst(s->x, ty, C_PINK, 6, 1.0f);
        }
    }
    if (e->st % 260 == 130 && count_kind(K_GHOST) < 2) shb_spawn(K_GHOST, wx(SCREEN_W + 10), wy(20 + (e->st % 3) * 60), F_CRYSTAL, 0);
    if (e->st % 420 == 300) {
        int tr = (e->st / 420) % 3, g = ++sb.group_seq;
        for (int k = 0; k < 3; k++) {
            int j = shb_spawn(K_CART, wx(SCREEN_W + 12 + k * 18), wy(TRACK_Y[tr]) - 5, k == 2 ? F_CRYSTAL : 0, 0);
            if (j >= 0) { sb.foe[j].group = g; sb.foe[j].idx = k; }
        }
    }
    if (e->st % 110 == 80) shb_aimed(e->x - 8, e->y, 1.5f, 1, 0);
}

static void jaw_update(Foe *e) {
    if (e->state == 0) {
        enter(e, 250, 0.8f);
        if (e->state == 1) {
            /* the bone chutes, top and bottom */
            for (int k = 0; k < 2; k++) {
                int j = shb_spawn(K_CHUTE, wx(200), wy(k ? SHB_PF_H - 10 : 10), k ? 0 : F_CEIL, k);
                if (j >= 0) { sb.foe[j].x = wx(196); sb.foe[j].y = wy(k ? SHB_PF_H - 10 : 10); sb.foe[j].state = 1; }
            }
        }
        return;
    }
    if (e->stun > 0) {
        e->stun--;
        if (e->stun == 0) { e->open = false; e->st = 0; }
        return;
    }
    e->st++;
    e->y = wy(84 + sinf(e->st * 0.016f) * 34);
    /* the jaw opens now and then to fire */
    int c = e->st % 210;
    e->open = c >= 150;
    if (c == 160) { shb_aimed(e->x - 14, e->y + 4, 1.9f, 5, 16); shb_sfx("shb_roar", 10); }
    if (c == 185) {
        for (int k = 0; k < 4; k++) shb_add_eshot(ES_ARROW, wx(60 + k * 50 + (e->st % 30)), wy(4), -0.2f, 0.6f);
    }
    if (c == 60) shb_aimed(e->x - 14, e->y, 1.5f, 1, 0);
}

static void chute_update(Foe *e) {
    if (e->stun > 0) { e->stun--; return; }
    e->st++;
    if ((e->st + e->arg * 60) % 140 == 0) {
        float vy = e->arg ? -1.0f : 1.0f;
        int j = shb_add_eshot(ES_BIG, e->x, e->y + vy * 8, -1.1f, vy * 0.9f);
        (void)j;
        shb_sfx("shb_chute", 8);
    }
}

static void kalei_update(int i) {
    Foe *e = &sb.foe[i];
    if (e->state == 0) {
        enter(e, 228, 0.9f);
        if (e->state == 1) {
            for (int k = 0; k < KALEI_SHARDS; k++) {
                int j = shb_spawn(K_SHARD, e->x, e->y, 0, k);
                if (j >= 0) { sb.foe[j].state = 1; sb.foe[j].ax = e->x; sb.foe[j].ay = e->y; }
            }
        }
        return;
    }
    if (e->stun > 0) e->stun--;
    if (e->phase == 1) {
        e->st++;
        if (e->st > KALEI_UNWOUND) { e->phase = 0; e->open = false; e->st = 0; sfx_play_name("shb_unwind"); }
    } else {
        e->st++;
    }
    if (e->stun > 0) return;
    e->arg2++;
    e->y = wy(84 + sinf(e->arg2 * 0.012f) * 36);
    if (e->arg2 % 100 == 50) { shb_ring(e->x, e->y, 1.1f, 12, e->arg2 * 0.05f, ES_BIG); shb_sfx("shb_volley", 6); }
    if (e->arg2 % 100 == 0) shb_aimed(e->x, e->y, 1.6f, 3, 20);
    /* bread from the top and the bottom */
    if (e->arg2 % 360 == 200 && count_kind(K_TOAST) < 2) {
        shb_spawn(K_TOAST, wx(SCREEN_W + 6), wy(SHB_PF_H - 8), F_CRYSTAL, 0);
        shb_spawn(K_TOAST, wx(SCREEN_W + 30), wy(8), F_CEIL | F_CRYSTAL, 0);
    }
}

static void shard_update(Foe *e) {
    int b = find_boss(K_KALEI);
    if (b < 0) { e->alive = 0; return; }
    Foe *k = &sb.foe[b];
    e->t++;
    float a = e->arg * 6.2832f / KALEI_SHARDS + e->t * 0.03f;
    if (k->phase == 1) {
        /* unwound: the shards meander about the screen */
        float tx = wx(160 + sinf(e->t * 0.011f + e->arg * 1.7f) * 130);
        float ty = wy(84 + sinf(e->t * 0.017f + e->arg * 2.3f) * 70);
        e->x = fapproach(e->x, tx, 1.6f);
        e->y = fapproach(e->y, ty, 1.6f);
    } else {
        float tx = k->x + cosf(a) * 26, ty = k->y + sinf(a) * 26;
        e->x = fapproach(e->x, tx, 2.2f);
        e->y = fapproach(e->y, ty, 2.2f);
    }
}

void shb_boss_update(int i) {
    Foe *e = &sb.foe[i];
    e->t = imax(e->t, 0);
    switch (e->kind) {
    case K_SCONE: scone_update(e); break;
    case K_TEAPOT: teapot_update(e); break;
    case K_CROAK: croak_update(e); break;
    case K_SIGNAL: signal_update(e); break;
    case K_JAW: jaw_update(e); break;
    case K_CHUTE: chute_update(e); break;
    case K_KALEI: kalei_update(i); break;
    case K_SHARD: shard_update(e); break;
    default: break;
    }
}
