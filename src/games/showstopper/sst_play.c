/* SHOWSTOPPER - a stage being played: Clover (walking, the wand, the dash,
 * pits, the bubble), the camera and what comes into view, shots, enemy
 * fire, cards and hands, star passes, bombs and the ground's hazards. The
 * foes think in sst_foes.c and the bosses in sst_boss.c. Everything here
 * reads the buttons it is handed (sst_play_step), never the pad itself, so
 * the demo player can try a move in a copy of the game. */
#include "sst.h"

/* ---- numbers ------------------------------------------------------------------------- */

const int16_t SST_SIN[SST_ANG] = {
    0, 25, 50, 74, 98, 121, 142, 162, 181, 198, 213, 226, 237, 245, 251, 255,
    256, 255, 251, 245, 237, 226, 213, 198, 181, 162, 142, 121, 98, 74, 50, 25,
    0, -25, -50, -74, -98, -121, -142, -162, -181, -198, -213, -226, -237, -245, -251, -255,
    -256, -255, -251, -245, -237, -226, -213, -198, -181, -162, -142, -121, -98, -74, -50, -25,
};
const int8_t SST_DX8[8] = {1, 1, 0, -1, -1, -1, 0, 1};
const int8_t SST_DY8[8] = {0, 1, 1, 1, 0, -1, -1, -1};

int sst_dir_to(int dx, int dy) {
    if (dx == 0 && dy == 0) return 16;
    /* scale down so the products stay small */
    while (iabs(dx) > 30000 || iabs(dy) > 30000) { dx /= 2; dy /= 2; }
    long long best = -((long long)1 << 40);
    int bi = 0;
    for (int a = 0; a < SST_ANG; a++) {
        long long d = (long long)dx * sst_cos(a) + (long long)dy * sst_sin(a);
        if (d > best) { best = d; bi = a; }
    }
    return bi;
}

int sst_isqrt(int v) {
    if (v <= 0) return 0;
    int r = 0, bit = 1 << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; }
        else r >>= 1;
        bit >>= 2;
    }
    return r;
}

int sst_dist(int dx, int dy) {
    dx = iabs(dx);
    dy = iabs(dy);
    while (dx > 40000 || dy > 40000) return sst_isqrt((dx / 16) * (dx / 16) + (dy / 16) * (dy / 16)) * 16;
    return sst_isqrt(dx * dx + dy * dy);
}

int sst_dir8_of(uint32_t held) {
    int dx = ((held & BTN_RIGHT) != 0) - ((held & BTN_LEFT) != 0);
    int dy = ((held & BTN_DOWN) != 0) - ((held & BTN_UP) != 0);
    for (int d = 0; d < 8; d++)
        if (SST_DX8[d] == dx && SST_DY8[d] == dy) return d;
    return -1;
}

static inline int dir8_ang(int d) { return d * 8; }

/* ---- the map under things -------------------------------------------------------------- */

static int tile_px(int px, int py) { return sst_tile_at(px >> 4, py >> 4); }

bool sst_box_solid(int x, int y, int hw, int hh) {
    int x0 = SST_PX(x) - hw, x1 = SST_PX(x) + hw - 1, y0 = SST_PX(y) - hh, y1 = SST_PX(y) + hh - 1;
    for (int ty = y0 >> 4; ty <= y1 >> 4; ty++)
        for (int tx = x0 >> 4; tx <= x1 >> 4; tx++)
            if (sst_tile_solid(sst_tile_at(tx, ty))) return true;
    return false;
}

/* crates and barrels stand in the way too */
static bool box_blocked(int x, int y, int hw, int hh) {
    if (sst_box_solid(x, y, hw, hh)) return true;
    for (int i = 0; i < SST_MAX_FOES; i++) {
        const SstFoe *f = &sst_g.foe[i];
        if (!f->alive || !(SST_FOE[f->kind].flags & FF_SOLID)) continue;
        if (rects_overlap(SST_PX(x) - hw, SST_PX(y) - hh, hw * 2, hh * 2, SST_PX(f->x) - SST_FOE[f->kind].hw,
                          SST_PX(f->y) - SST_FOE[f->kind].hh, SST_FOE[f->kind].hw * 2, SST_FOE[f->kind].hh * 2))
            return true;
    }
    return false;
}

bool sst_on_screen(int x, int y, int margin) {
    int px = SST_PX(x), py = SST_PX(y) - sst_g.cam_y;
    return px > -margin && px < SCREEN_W + margin && py > -margin && py < SST_VIEW_H + margin;
}

int sst_player_ang(int x, int y) { return sst_dir_to(sst_g.pl.x - x, sst_g.pl.y - y); }

static bool firm_at(int x, int y) {
    int t = tile_px(SST_PX(x), SST_PX(y));
    return !sst_tile_pit(t) && !sst_tile_solid(t);
}

/* ---- adding things --------------------------------------------------------------------- */

int sst_add_bullet(int kind, int x, int y, int vx, int vy) {
    for (int i = 0; i < SST_MAX_BULLETS; i++) {
        SstBullet *b = &sst_g.eb[i];
        if (b->alive) continue;
        memset(b, 0, sizeof *b);
        b->alive = 1;
        b->kind = (uint8_t)kind;
        b->x = x;
        b->y = y;
        b->vx = vx;
        b->vy = vy;
        return i;
    }
    return -1;
}

int sst_aim_bullet(int kind, int x, int y, int speed, int ang_ofs) {
    int a = sst_player_ang(x, y) + ang_ofs;
    return sst_add_bullet(kind, x, y, speed * sst_cos(a) / 256, speed * sst_sin(a) / 256);
}

void sst_ring(int kind, int x, int y, int n, int speed, int ang0) {
    for (int k = 0; k < n; k++) {
        int a = ang0 + k * SST_ANG / n;
        sst_add_bullet(kind, x, y, speed * sst_cos(a) / 256, speed * sst_sin(a) / 256);
    }
}

int sst_add_haz(int kind, int x, int y, int t, int r) {
    for (int i = 0; i < SST_MAX_HAZ; i++) {
        SstHaz *h = &sst_g.hz[i];
        if (h->alive) continue;
        h->alive = 1;
        h->kind = (uint8_t)kind;
        h->x = x;
        h->y = y;
        h->t = t;
        h->r = r;
        return i;
    }
    return -1;
}

int sst_spawn_foe(int kind, int x, int y) {
    for (int i = 0; i < SST_MAX_FOES; i++) {
        SstFoe *f = &sst_g.foe[i];
        if (f->alive) continue;
        memset(f, 0, sizeof *f);
        f->alive = 1;
        f->kind = (uint8_t)kind;
        f->x = x;
        f->y = y;
        f->hp = SST_FOE[kind].hp;
        sst_foe_init(f);
        return i;
    }
    return -1;
}

int sst_live_foes(void) {
    int n = 0;
    for (int i = 0; i < SST_MAX_FOES; i++) {
        const SstFoe *f = &sst_g.foe[i];
        n += f->alive && f->kind != FK_CRATE && f->kind != FK_BARREL && f->kind != FK_SECRET;
    }
    return n;
}

void sst_drop_card(int x, int y) {
    for (int i = 0; i < SST_MAX_CARDS; i++) {
        SstCard *c = &sst_g.card[i];
        if (c->alive) continue;
        c->alive = 1;
        sst_g.cards_dropped++;
        c->col = (uint8_t)rng_range(&sst_g.rng, CC_LIFE, CC_SHIELD);
        c->x = x;
        c->y = y;
        c->t = 0;
        return;
    }
}

static void pop_pass(int x, int y) {
    for (int i = 0; i < SST_MAX_PASSES; i++) {
        SstPass *p = &sst_g.pass[i];
        if (p->alive) continue;
        p->alive = 1;
        p->x = x;
        p->y = y;
        p->t = 0;
        return;
    }
}

void sst_boom(int x, int y, int r) { sst_add_haz(HZ_BLAST, x, y, 14, r); }

/* ---- foes taking harm --------------------------------------------------------------------- */

void sst_kill_foe(int i) {
    SstFoe *f = &sst_g.foe[i];
    if (!f->alive) return;
    int kind = f->kind;
    uint8_t flags = SST_FOE[kind].flags;
    f->alive = 0;
    if (kind == FK_SECRET) {
        /* the hidden spot gives up its star pass */
        sst_g.found |= (uint16_t)(1u << (f->ord & 15));
        pop_pass(f->x, f->y);
        sst_sfx("sst_found");
        return;
    }
    sst_g.kills++;
    sst_sfx(kind == FK_CRATE || kind == FK_BARREL ? "sst_crate" : "sst_pop");
    if (kind == FK_EMBER && f->size > 1) {
        /* the living flame splits in two smaller ones that fly off */
        int size = f->size - 1, x = f->x, y = f->y;
        for (int k = 0; k < 2; k++) {
            int j = sst_spawn_foe(FK_EMBER, x, y);
            if (j < 0) continue;
            SstFoe *c = &sst_g.foe[j];
            c->size = (uint8_t)size;
            c->hp = c->size;
            int a = rng_range(&sst_g.rng, 0, 63);
            c->vx = 400 * sst_cos(a) / 256;
            c->vy = 400 * sst_sin(a) / 256;
            c->t = 0;
            c->a = 30; /* flies off a while before it hunts again */
        }
        return;
    }
    if (flags & FF_NOCARD) return;
    if ((flags & FF_ALWAYS) || rng_chance(&sst_g.rng, 50)) sst_drop_card(f->x, f->y);
}

bool sst_foe_hit(int i, int dmg) {
    SstFoe *f = &sst_g.foe[i];
    if (!f->alive) return false;
    if (f->kind == FK_EGG && f->t < 0) return false;
    f->hp -= dmg;
    f->hurt = 6;
    if (f->kind == FK_SECRET) sst_sfx("sst_tink");
    else sst_sfx("sst_hit");
    if (f->hp <= 0) sst_kill_foe(i);
    return true;
}

static bool foe_box(const SstFoe *f, int x, int y, int hw, int hh) {
    const SstFoeDef *d = &SST_FOE[f->kind];
    int fhw = d->hw, fhh = d->hh;
    if (f->kind == FK_EMBER) fhw = fhh = 2 + f->size * 2;
    return rects_overlap(SST_PX(x) - hw, SST_PX(y) - hh, hw * 2, hh * 2, SST_PX(f->x) - fhw, SST_PX(f->y) - fhh, fhw * 2,
                         fhh * 2);
}

/* a player-side blast: foes, the boss, enemy shots */
void sst_blast(int x, int y, int r, int dmg, bool player_side) {
    if (!player_side) return;
    for (int i = 0; i < SST_MAX_FOES; i++) {
        SstFoe *f = &sst_g.foe[i];
        if (!f->alive) continue;
        if (sst_dist(SST_PX(f->x - x), SST_PX(f->y - y)) <= r + SST_FOE[f->kind].hw) sst_foe_hit(i, dmg);
    }
    for (int i = 0; i < SST_MAX_BULLETS; i++) {
        SstBullet *b = &sst_g.eb[i];
        if (b->alive && b->kind != EB_ROCKET && b->kind != EB_JET && sst_dist(SST_PX(b->x - x), SST_PX(b->y - y)) <= r) b->alive = 0;
    }
    if (sst_g.boss.alive) sst_boss_shot(x, y, r, dmg);
}

/* ---- Clover -------------------------------------------------------------------------------- */

void sst_clear_powers(void) {
    SstPlayer *p = &sst_g.pl;
    memset(p->pow, 0, sizeof p->pow);
    memset(p->shield, 0, sizeof p->shield);
    p->max_hp = SST_START_HP;
}

int sst_hand_result(const uint8_t h[3], int *col) {
    int cnt[CC_COUNT] = {0};
    for (int k = 0; k < 3; k++) cnt[h[k] % CC_COUNT]++;
    for (int c = 1; c < CC_COUNT; c++)
        if (cnt[c] == 3) { *col = c; return HAND_B; }
    for (int c = 1; c < CC_COUNT; c++)
        if (cnt[c] == 2) { *col = c; return HAND_A; }
    *col = 0;
    return HAND_NOTHING;
}

void sst_apply_hand(const uint8_t h[3]) {
    SstPlayer *p = &sst_g.pl;
    int col;
    int res = sst_hand_result(h, &col);
    p->hand_res = res;
    p->hand_col = col;
    p->hand_t = 90;
    memcpy(p->shown, h, 3);
    if (res == HAND_NOTHING) {
        sst_g.hands_none++;
        sst_sfx("sst_dud");
        return;
    }
    if (res == HAND_A) sst_g.hands_a++;
    else sst_g.hands_b++;
    sst_sfx(res == HAND_B ? "sst_big" : "sst_power");
    int lv = res == HAND_B ? 2 : 1;
    switch (col) {
    case CC_LIFE:
        if (res == HAND_B && p->max_hp < SST_MAX_HP) p->max_hp = SST_MAX_HP;
        p->hp = imin(p->hp + 1, p->max_hp);
        break;
    case CC_SHIELD:
        if (lv == 2) {
            p->pow[CC_SHIELD] = 2;
            for (int k = 0; k < 5; k++) p->shield[k] = SST_SHIELD_HP;
        } else if (p->pow[CC_SHIELD] == 2) {
            /* the small hand tops up the weakest ball of the big shield */
            int w = 0;
            for (int k = 1; k < 5; k++)
                if (p->shield[k] < p->shield[w]) w = k;
            p->shield[w] = SST_SHIELD_HP;
        } else {
            p->pow[CC_SHIELD] = 1;
            p->shield[0] = SST_SHIELD_HP;
        }
        break;
    default:
        if (p->pow[col] < lv) p->pow[col] = (uint8_t)lv;
        if (col == CC_OPTION) {
            p->opt_x = p->x;
            p->opt_y = p->y;
        }
        break;
    }
}

void sst_give_card(int col) {
    SstPlayer *p = &sst_g.pl;
    sst_g.cards_got++;
    p->hand[p->nhand++] = (uint8_t)col;
    sst_sfx("sst_card");
    if (p->nhand < 3) return;
    /* the secret hand: red, then blue, then green */
    if (p->hand[0] == CC_BOMB && p->hand[1] == CC_SHIELD && p->hand[2] == CC_SHOT) {
        sst_g.code_t = 240;
        if (!sst_sim && !sst_sv.free_ride) {
            sst_sv.free_ride = 1;
            sst_save_now();
        }
    }
    sst_apply_hand(p->hand);
    p->nhand = 0;
    memset(p->hand, 0, sizeof p->hand);
}

void sst_player_hurt(int dmg) {
    SstPlayer *p = &sst_g.pl;
    if (sst_g.god || p->state != PS_WALK || p->inv > 0) return;
    p->hp -= dmg;
    p->inv = SST_INV_HIT;
    sst_g.hits_taken++;
    sst_g.shake = 6;
    sst_sfx("sst_ouch");
    if (p->hp <= 0) {
        p->hp = 0;
        p->state = PS_DOWN;
        p->st = 0;
        p->vx = p->vy = 0;
    }
}

/* a spot to come back on: the bubble pops on firm ground */
static void nearest_firm(int *x, int *y) {
    int bx = *x, by = *y, best = 1 << 30;
    for (int ty = (sst_g.cam_y >> 4); ty <= ((sst_g.cam_y + SST_VIEW_H) >> 4); ty++)
        for (int tx = 1; tx < SST_COLS - 1; tx++) {
            int cx = tx * 16 + 8, cy = ty * 16 + 8;
            if (cy < sst_g.cam_y + 12 || cy > sst_g.cam_y + SST_VIEW_H - 10) continue;
            int t = sst_tile_at(tx, ty);
            if (sst_tile_pit(t) || sst_tile_solid(t)) continue;
            /* its neighbours firm too, so it isn't on a lip */
            bool ok = true;
            for (int d = 0; d < 4 && ok; d++) {
                int nt = sst_tile_at(tx + (d == 0) - (d == 1), ty + (d == 2) - (d == 3));
                if (sst_tile_pit(nt)) ok = false;
            }
            if (!ok) continue;
            int dd = sst_dist(cx - SST_PX(*x), cy - SST_PX(*y));
            if (dd < best) { best = dd; bx = cx * SST_FX; by = cy * SST_FX; }
        }
    *x = bx;
    *y = by;
}

void sst_lose_life(bool fell) {
    SstPlayer *p = &sst_g.pl;
    if (fell) sst_g.falls++;
    sst_clear_powers();
    sst_g.lives--;
    sst_sfx("sst_lose");
    if (sst_g.lives <= 0) {
        sst_g.lives = 0;
        p->state = PS_GONE;
        p->st = 0;
        return;
    }
    /* back on the spot, in a bubble */
    p->state = PS_BUBBLE;
    p->st = 0;
    p->bub_t = SST_BUBBLE_T;
    p->vx = p->vy = 0;
    p->hp = SST_START_HP;
    p->firing = false;
    p->dash_t = 0;
    if (!sst_on_screen(p->x, p->y, -8)) nearest_firm(&p->x, &p->y);
}

static void pop_bubble(void) {
    SstPlayer *p = &sst_g.pl;
    if (!firm_at(p->x, p->y) || box_blocked(p->x, p->y, 5, 5)) {
        int x = p->safe_x, y = p->safe_y;
        if (!sst_on_screen(x, y, -12) || !firm_at(x, y)) {
            x = p->x;
            y = p->y;
            nearest_firm(&x, &y);
        }
        p->x = x;
        p->y = y;
    }
    p->state = PS_WALK;
    p->st = 0;
    p->inv = SST_INV_POP;
    p->hp = SST_START_HP;
    p->max_hp = SST_START_HP;
    sst_sfx("sst_bubble");
}

static void fire_one(int ang, bool extra) {
    SstPlayer *p = &sst_g.pl;
    int n = 0;
    for (int i = 0; i < SST_MAX_SHOTS; i++) n += sst_g.ps[i].alive;
    if (n >= SST_MAX_SHOTS - (extra ? 0 : 2)) return;
    for (int i = 0; i < SST_MAX_SHOTS; i++) {
        SstShot *s = &sst_g.ps[i];
        if (s->alive) continue;
        s->alive = 1;
        s->from_opt = 0;
        s->big = p->pow[CC_SHOT] == 2;
        s->dmg = s->big ? 2 : 1;
        s->x = p->x + 6 * sst_cos(ang);
        s->y = p->y - 2 * SST_FX + 6 * sst_sin(ang);
        s->vx = SST_SHOT_V * sst_cos(ang) / 256;
        s->vy = SST_SHOT_V * sst_sin(ang) / 256;
        return;
    }
}

static void option_fire(void) {
    SstPlayer *p = &sst_g.pl;
    if (!p->pow[CC_OPTION]) return;
    int ang = dir8_ang(p->face);
    for (int i = SST_MAX_SHOTS; i < SST_MAX_SHOTS + 8; i++) {
        SstShot *s = &sst_g.ps[i];
        if (s->alive) continue;
        s->alive = 1;
        s->from_opt = 1;
        s->big = 0;
        s->dmg = 1;
        s->x = p->opt_x;
        s->y = p->opt_y;
        s->vx = SST_SHOT_V * sst_cos(ang) / 256;
        s->vy = SST_SHOT_V * sst_sin(ang) / 256;
        return;
    }
}

/* one shot of a volley; the first of each volley flares two more out on
 * the diagonals with the green power */
static void fire_volley(bool first) {
    SstPlayer *p = &sst_g.pl;
    int ang = dir8_ang(p->face);
    fire_one(ang, false);
    if (first && p->pow[CC_SHOT]) {
        fire_one(ang - 8, true);
        fire_one(ang + 8, true);
    }
    option_fire();
    sst_g.shots_fired++;
    sst_sfx("sst_shot");
}

static void drop_bomb(void) {
    SstPlayer *p = &sst_g.pl;
    for (int i = 0; i < SST_MAX_BOMBS; i++) {
        SstBomb *b = &sst_g.bomb[i];
        if (b->alive) continue;
        b->alive = 1;
        b->x = p->x;
        b->y = p->y;
        b->t = 40;
        return;
    }
}

static void move_axis(int *pos, int delta, bool xaxis) {
    SstPlayer *p = &sst_g.pl;
    if (!delta) return;
    int step = delta > 0 ? 64 : -64;
    int left = delta;
    while (left) {
        int d = iabs(left) < 64 ? left : step;
        int nx = xaxis ? *pos + d : p->x, ny = xaxis ? p->y : *pos + d;
        if (box_blocked(nx, ny, 5, 5)) {
            if (xaxis) p->vx = 0;
            else p->vy = 0;
            return;
        }
        *pos += d;
        left -= d;
    }
}

static void player_move(int vx, int vy) {
    SstPlayer *p = &sst_g.pl;
    move_axis(&p->x, vx, true);
    move_axis(&p->y, vy, false);
}

static void clamp_to_view(void) {
    SstPlayer *p = &sst_g.pl;
    int top = (sst_g.cam_y + 8) * SST_FX, bot = (sst_g.cam_y + SST_VIEW_H - 6) * SST_FX;
    p->x = iclamp(p->x, 8 * SST_FX, (SCREEN_W - 8) * SST_FX);
    if (p->y < top) p->y = top;
    if (p->y > bot) {
        /* the view's bottom edge pushes (a chase up the road) */
        int ny = bot;
        if (p->state == PS_WALK && box_blocked(p->x, ny, 5, 5)) {
            for (int dx = 16; dx <= 160; dx += 16) {
                if (!box_blocked(p->x + dx * SST_FX, ny, 5, 5)) { p->x += dx * SST_FX; break; }
                if (!box_blocked(p->x - dx * SST_FX, ny, 5, 5)) { p->x -= dx * SST_FX; break; }
            }
        }
        p->y = ny;
    }
}

static void player_update(uint32_t held, uint32_t press) {
    SstPlayer *p = &sst_g.pl;
    p->st++;
    if (p->inv > 0) p->inv--;
    if (p->hand_t > 0) p->hand_t--;
    int dir = sst_dir8_of(held);
    switch (p->state) {
    case PS_BUBBLE: {
        /* float to a spot, any button pops it; time up, it pops anyway */
        if (dir >= 0) {
            p->x += SST_DX8[dir] * 256;
            p->y += SST_DY8[dir] * 256;
        }
        clamp_to_view();
        bool firm = firm_at(p->x, p->y) && !box_blocked(p->x, p->y, 5, 5);
        if (--p->bub_t <= 0 || ((press & (BTN_A | BTN_B)) && firm && p->st > 20)) pop_bubble();
        return;
    }
    case PS_FALL:
        if (p->st >= 30) sst_lose_life(true);
        return;
    case PS_DOWN:
        if (p->st >= 40) sst_lose_life(false);
        return;
    case PS_GONE:
        return;
    default: break;
    }
    if (p->shot_cd > 0) p->shot_cd--;
    if (p->dash_cd > 0) p->dash_cd--;

    /* the wand: a press fires at once (facing the pad if it is pressed), and
     * as long as it is held the facing stays put and bursts of three follow */
    if (press & BTN_A) {
        if (dir >= 0) p->face = dir;
        p->firing = true;
        p->hold_t = 0;
        if (p->shot_cd == 0) {
            fire_volley(true);
            p->shot_cd = SST_SHOT_CD;
        }
    } else if ((held & BTN_A) && p->firing) {
        p->hold_t++;
        int k = p->hold_t - SST_HOLD_FIRST;
        if (k >= 0) {
            int m = k % SST_BURST_EVERY;
            if (m == 0 || m == SST_BURST_GAP || m == 2 * SST_BURST_GAP) fire_volley(m == 0);
        }
        /* the stubby helper keeps firing while the wand is held */
        if (p->pow[CC_OPTION] == 2 && p->hold_t % 7 == 3) option_fire();
    } else {
        p->firing = false;
    }

    /* the dash */
    if ((press & BTN_B) && p->state == PS_WALK && p->dash_cd == 0) {
        p->state = PS_DASH;
        p->dash_t = SST_DASH_T;
        p->dash_dir = dir >= 0 ? dir : p->move_dir;
        p->move_dir = p->dash_dir;
        p->dash_bombs = 0;
        sst_g.dashes++;
        sst_sfx("sst_dash");
    }

    if (p->state == PS_DASH) {
        int d = p->dash_dir;
        int sp = (SST_DX8[d] && SST_DY8[d]) ? SST_DASH_V * 181 / 256 : SST_DASH_V;
        p->vx = SST_DX8[d] * sp;
        p->vy = SST_DY8[d] * sp;
        /* bombs: one where the dash starts, or a trail of three */
        int k = SST_DASH_T - p->dash_t;
        if (p->pow[CC_BOMB] == 1 && k == 0) drop_bomb();
        if (p->pow[CC_BOMB] == 2 && (k == 0 || k == 5 || k == 10)) drop_bomb();
        player_move(p->vx, p->vy);
        /* dashing through a foe stuns it */
        for (int i = 0; i < SST_MAX_FOES; i++) {
            SstFoe *f = &sst_g.foe[i];
            if (!f->alive || !(SST_FOE[f->kind].flags & FF_STUN) || f->stun) continue;
            if (foe_box(f, p->x, p->y, 6, 6)) {
                f->stun = SST_STUN;
                sst_g.stuns++;
            }
        }
        if (--p->dash_t <= 0) {
            p->state = PS_WALK;
            /* out of the dash at walking pace (on ice she slides on from there) */
            p->vx = p->vx * SST_WALK / SST_DASH_V;
            p->vy = p->vy * SST_WALK / SST_DASH_V;
            p->dash_cd = SST_DASH_CD;
            p->inv = imax(p->inv, 3);
        }
    } else {
        /* walking: eight ways at one pace; on ice she slides */
        int tvx = 0, tvy = 0;
        if (dir >= 0) {
            int sp = (SST_DX8[dir] && SST_DY8[dir]) ? SST_WALK_DIAG : SST_WALK;
            tvx = SST_DX8[dir] * sp;
            tvy = SST_DY8[dir] * sp;
            p->move_dir = dir;
            if (!p->firing) p->face = dir;
            p->step++;
        }
        if (tile_px(SST_PX(p->x), SST_PX(p->y)) == T_ICE) {
            p->vx += iclamp(tvx - p->vx, -10, 10);
            p->vy += iclamp(tvy - p->vy, -10, 10);
        } else {
            p->vx = tvx;
            p->vy = tvy;
        }
        player_move(p->vx, p->vy);
    }
    clamp_to_view();

    /* the trail the helper follows */
    p->trail_i = (p->trail_i + 1) % 16;
    p->trail_x[p->trail_i] = p->x;
    p->trail_y[p->trail_i] = p->y;
    if (p->pow[CC_OPTION]) {
        int j = (p->trail_i + 16 - 10) % 16;
        p->opt_x += (p->trail_x[j] - 14 * SST_FX - p->opt_x) / 6;
        p->opt_y += (p->trail_y[j] + 4 * SST_FX - p->opt_y) / 6;
    }
    p->shield_ang = (p->shield_ang + 2) & 63;

    /* pits: the dash carries her over; standing on one is a fall */
    if (p->state == PS_WALK) {
        if (sst_tile_pit(tile_px(SST_PX(p->x), SST_PX(p->y)))) {
            p->state = PS_FALL;
            p->st = 0;
            p->vx = p->vy = 0;
            p->firing = false;
            sst_sfx("sst_fall");
            return;
        }
        bool safe = true;
        for (int k = 0; k < 4 && safe; k++) {
            int cx = SST_PX(p->x) + (k & 1 ? 7 : -7), cy = SST_PX(p->y) + (k & 2 ? 7 : -7);
            if (sst_tile_pit(tile_px(cx, cy))) safe = false;
        }
        if (safe) {
            p->safe_x = p->x;
            p->safe_y = p->y;
        }
        /* roof snow: under an eave, now and then a clump drops */
        int tx = SST_PX(p->x) >> 4, ty = SST_PX(p->y) >> 4;
        if (sst_tile_at(tx, ty - 1) == T_ROOF && (sst_g.frame_t + tx * 13) % 70 == 0)
            sst_add_haz(HZ_SNOW, p->x, (ty * 16 + 4) * SST_FX, 34, 8);
    }
}

/* ---- the camera and what comes into view ---------------------------------------------------- */

static void spawn_from_list(void) {
    const SstSpawn *list;
    int n;
    sst_stage_spawns(sst_g.stage, &list, &n);
    while (sst_g.next_spawn < n) {
        const SstSpawn *sp = &list[sst_g.next_spawn];
        bool marker = sp->kind < 0;
        /* a boss wakes while it is still above the view; the view then
         * climbs on to frame its arena and stops there */
        int trigger = marker && sp->kind != -5 && sp->kind != -6 ? sp->y + 40 : sp->y + 24;
        if (sp->kind == -8) trigger = 1 << 20; /* the chase's stop is read by the chase */
        if (sst_g.cam_y > trigger) break;
        sst_g.next_spawn++;
        const SstStage *st = &SST_STAGE[sst_g.stage];
        switch (sp->kind) {
        case -1: case -2: case -9: {
            int bk = sp->kind == -1 ? st->boss1 : sp->kind == -2 ? st->boss2 : st->boss9;
            if (sst_g.stage == 4 && sst_g.half == 1) bk = BK_MAW;
            sst_g.boss_seen |= sp->kind == -1 ? 1 : sp->kind == -2 ? 2 : 4;
            sst_g.cam_lock = imax(0, sp->y - 40);
            sst_boss_start(bk, sp->x * SST_FX, sp->y * SST_FX);
            break;
        }
        case -7: {
            /* the chase: the Boiler backs up the road and the view goes with it */
            sst_g.chasing = true;
            sst_g.chase_stop = 0;
            for (int k = 0; k < n; k++)
                if (list[k].kind == -8) sst_g.chase_stop = imax(0, list[k].y - 40);
            sst_g.boss_seen |= 8;
            sst_boss_start(st->chase, sp->x * SST_FX, (sst_g.cam_y + 34) * SST_FX);
            break;
        }
        case -5: sst_g.half = 1; break;
        case -6: case -8: break;
        default: {
            if (sp->kind == FK_SECRET && (sst_g.found >> (sp->ord & 15)) & 1) break; /* found on an earlier try */
            int j = sst_spawn_foe(sp->kind, sp->x * SST_FX, sp->y * SST_FX);
            if (j >= 0) sst_g.foe[j].ord = sp->ord;
            break;
        }
        }
        if (sp->kind == -7 || sp->kind == -1 || sp->kind == -2 || sp->kind == -9) break;
    }
}

static void camera_update(void) {
    const SstStage *st = &SST_STAGE[sst_g.stage];
    if (st->kind != SK_SCROLL) {
        sst_g.cam_y = 0;
        return;
    }
    if (sst_g.chasing) {
        if (sst_g.cam_y > sst_g.chase_stop) {
            sst_g.chase_acc += 96; /* 0.375 px a frame */
            while (sst_g.chase_acc >= 256 && sst_g.cam_y > sst_g.chase_stop) {
                sst_g.chase_acc -= 256;
                sst_g.cam_y--;
            }
        }
    } else {
        int want = SST_PX(sst_g.pl.y) - 72;
        if (sst_g.pl.state == PS_WALK || sst_g.pl.state == PS_DASH)
            if (want < sst_g.cam_y) sst_g.cam_y = want;
    }
    if (sst_g.cam_lock >= 0 && sst_g.cam_y < sst_g.cam_lock) sst_g.cam_y = sst_g.cam_lock;
    if (sst_g.cam_y < 0) sst_g.cam_y = 0;
    spawn_from_list();
}

/* ---- shots, enemy fire, the ground --------------------------------------------------------------- */

static void shots_update(void) {
    for (int i = 0; i < SST_MAX_SHOTS + 8; i++) {
        SstShot *s = &sst_g.ps[i];
        if (!s->alive) continue;
        s->x += s->vx;
        s->y += s->vy;
        if (!sst_on_screen(s->x, s->y, 4)) { s->alive = 0; continue; }
        int hw = s->big ? 4 : 2;
        if (sst_tile_solid(tile_px(SST_PX(s->x), SST_PX(s->y)))) {
            /* a wall gun or torch in that wall takes the shot */
            for (int k = 0; k < SST_MAX_FOES; k++) {
                SstFoe *f = &sst_g.foe[k];
                if (f->alive && (SST_FOE[f->kind].flags & FF_WALL) && foe_box(f, s->x, s->y, hw + 2, hw + 2)) {
                    sst_foe_hit(k, s->dmg);
                    break;
                }
            }
            s->alive = 0;
            continue;
        }
        bool hit = false;
        for (int k = 0; k < SST_MAX_FOES && !hit; k++) {
            SstFoe *f = &sst_g.foe[k];
            if (!f->alive || !foe_box(f, s->x, s->y, hw, hw)) continue;
            if (f->kind == FK_BAT && f->a == 0) continue; /* still behind its window */
            hit = sst_foe_hit(k, s->dmg) || true;
        }
        if (!hit && sst_g.boss.alive) hit = sst_boss_shot(s->x, s->y, hw, s->dmg);
        /* shooting a bomb shot defuses it */
        if (!hit)
            for (int k = 0; k < SST_MAX_BULLETS && !hit; k++) {
                SstBullet *b = &sst_g.eb[k];
                if (b->alive && b->kind == EB_MINE && iabs(SST_PX(b->x - s->x)) < 6 && iabs(SST_PX(b->y - s->y)) < 6) {
                    b->alive = 0;
                    hit = true;
                    sst_sfx("sst_defuse");
                }
            }
        if (hit) s->alive = 0;
    }
}

/* where an enemy shot really is (the wavy ones swing round their line) */
static void bullet_pos(const SstBullet *b, int *x, int *y) {
    *x = b->x;
    *y = b->y;
    if (b->kind == EB_HELIX) {
        int off = sst_sin(b->t * 3) * 7 * b->a; /* px * 256 */
        int a = sst_dir_to(b->vx, b->vy) + 16;
        *x += off * sst_cos(a) / 256;
        *y += off * sst_sin(a) / 256;
    }
}

static bool shield_takes(int bx, int by) {
    SstPlayer *p = &sst_g.pl;
    if (!p->pow[CC_SHIELD] || p->state == PS_BUBBLE || p->state >= PS_FALL) return false;
    int n = p->pow[CC_SHIELD] == 2 ? 5 : 1;
    for (int k = 0; k < n; k++) {
        if (p->shield[k] <= 0) continue;
        int a = p->shield_ang + k * SST_ANG / n;
        int sx = p->x + 18 * sst_cos(a), sy = p->y + 18 * sst_sin(a);
        if (iabs(SST_PX(sx - bx)) < 6 && iabs(SST_PX(sy - by)) < 6) {
            if (--p->shield[k] <= 0) {
                sst_sfx("sst_ballgone");
                bool any = false;
                for (int j = 0; j < 5; j++) any |= p->shield[j] > 0;
                if (!any) p->pow[CC_SHIELD] = 0;
            }
            return true;
        }
    }
    return false;
}

static bool hits_player(int x, int y, int r) {
    const SstPlayer *p = &sst_g.pl;
    if (p->state != PS_WALK) return false;
    return iabs(SST_PX(x - p->x)) < r + 3 && iabs(SST_PX(y - p->y) + 1) < r + 3;
}

static void bullets_update(void) {
    for (int i = 0; i < SST_MAX_BULLETS; i++) {
        SstBullet *b = &sst_g.eb[i];
        if (!b->alive) continue;
        b->t++;
        switch (b->kind) {
        case EB_MINE:
            b->x += b->vx;
            b->y += b->vy;
            b->vx = b->vx * 7 / 8;
            b->vy = b->vy * 7 / 8;
            if (b->t >= 100) {
                b->alive = 0;
                sst_ring(EB_ROUND, b->x, b->y, 8, 300, 4);
                continue;
            }
            break;
        case EB_LOB:
            b->x += b->vx;
            b->y += b->vy;
            if (b->t >= b->a) {
                b->alive = 0;
                sst_boom(b->x, b->y, 14);
                sst_sfx("sst_boom");
            }
            continue;
        case EB_ROCKET:
            if (b->t >= b->a) {
                b->alive = 0;
                sst_boom(b->x, b->y, 24);
                sst_sfx("sst_boom");
            }
            continue;
        case EB_JET:
            if (b->t >= b->a) b->alive = 0;
            break;
        case EB_HOMING:
            if (b->t < 90) {
                int a = sst_dir_to(b->vx, b->vy), want = sst_player_ang(b->x, b->y);
                int d = ((want - a + 96) & 63) - 32;
                if (b->t % 4 == 0) a += isign(d);
                int sp = sst_dist(b->vx, b->vy);
                b->vx = sp * sst_cos(a) / 256;
                b->vy = sp * sst_sin(a) / 256;
            }
            b->x += b->vx;
            b->y += b->vy;
            break;
        default:
            b->x += b->vx;
            b->y += b->vy;
            break;
        }
        int bx, by;
        bullet_pos(b, &bx, &by);
        if (b->kind != EB_MINE && !sst_on_screen(bx, by, 24)) { b->alive = 0; continue; }
        if (b->kind == EB_MINE && !sst_on_screen(bx, by, 40)) { b->alive = 0; continue; }
        int r = b->kind == EB_BIG ? 5 : b->kind == EB_JET ? 6 : b->kind == EB_MINE ? 4 : 2;
        if (b->kind != EB_JET && shield_takes(bx, by)) { b->alive = 0; continue; }
        if (hits_player(bx, by, r)) {
            sst_player_hurt(1);
            if (b->kind != EB_JET) b->alive = 0;
        }
    }
}

static void haz_update(void) {
    for (int i = 0; i < SST_MAX_HAZ; i++) {
        SstHaz *h = &sst_g.hz[i];
        if (!h->alive) continue;
        h->t--;
        switch (h->kind) {
        case HZ_SNOW:
            if (h->t == 0 && hits_player(h->x, h->y, h->r)) sst_player_hurt(1);
            break;
        case HZ_BLAST:
            if (hits_player(h->x, h->y, h->r - 2)) sst_player_hurt(1);
            break;
        case HZ_SLICK:
        case HZ_SLIME:
            if (hits_player(h->x, h->y, h->r - 1)) sst_player_hurt(1);
            break;
        default: break;
        }
        if (h->t <= 0) h->alive = 0;
        else if (!sst_on_screen(h->x, h->y, 60)) h->alive = 0;
    }
    for (int i = 0; i < SST_MAX_BOMBS; i++) {
        SstBomb *b = &sst_g.bomb[i];
        if (!b->alive) continue;
        if (--b->t <= 0) {
            b->alive = 0;
            sst_add_haz(HZ_PBLAST, b->x, b->y, 12, 26);
            sst_blast(b->x, b->y, 26, 4, true);
            sst_sfx("sst_boom");
        }
    }
}

static void pickups_update(void) {
    SstPlayer *p = &sst_g.pl;
    bool can = p->state == PS_WALK || p->state == PS_DASH;
    for (int i = 0; i < SST_MAX_CARDS; i++) {
        SstCard *c = &sst_g.card[i];
        if (!c->alive) continue;
        c->t++;
        if (c->t > SST_CARD_LIFE || !sst_on_screen(c->x, c->y, 8)) { c->alive = 0; continue; }
        if (can && iabs(SST_PX(c->x - p->x)) < 10 && iabs(SST_PX(c->y - p->y)) < 11) {
            c->alive = 0;
            sst_give_card(c->col);
        }
    }
    for (int i = 0; i < SST_MAX_PASSES; i++) {
        SstPass *s = &sst_g.pass[i];
        if (!s->alive) continue;
        s->t++;
        /* scrolled away, it is lost */
        if (!sst_on_screen(s->x, s->y, 4)) { s->alive = 0; continue; }
        if (can && s->t > 20 && iabs(SST_PX(s->x - p->x)) < 10 && iabs(SST_PX(s->y - p->y)) < 11) {
            s->alive = 0;
            sst_g.passes++;
            sst_g.passes_run++;
            sst_sfx("sst_pass");
        }
    }
}

/* foes and boss parts touching Clover, the spike balls touching foes */
static void touch_update(void) {
    SstPlayer *p = &sst_g.pl;
    if (p->state != PS_WALK && p->state != PS_DASH) return;
    for (int i = 0; i < SST_MAX_FOES; i++) {
        SstFoe *f = &sst_g.foe[i];
        if (!f->alive) continue;
        uint8_t fl = SST_FOE[f->kind].flags;
        if (p->pow[CC_SHIELD] && sst_g.frame_t % 8 == 0 && !(fl & FF_NOHURT)) {
            int n = p->pow[CC_SHIELD] == 2 ? 5 : 1;
            for (int k = 0; k < n; k++) {
                if (p->shield[k] <= 0) continue;
                int a = p->shield_ang + k * SST_ANG / n;
                if (foe_box(f, p->x + 18 * sst_cos(a), p->y + 18 * sst_sin(a), 4, 4)) { sst_foe_hit(i, 1); break; }
            }
            if (!f->alive) continue;
        }
        if (fl & (FF_NOHURT | FF_WALL | FF_SOLID)) continue;
        if (f->kind == FK_BAT && f->a == 0) continue;
        if (f->kind == FK_EGG) continue;
        if (p->state == PS_WALK && foe_box(f, p->x, p->y, 4, 4)) sst_player_hurt(1);
    }
    if (sst_g.boss.alive && p->state == PS_WALK && sst_boss_touch(p->x, p->y, 4)) sst_player_hurt(1);
}

static void cull(void) {
    if (SST_STAGE[sst_g.stage].kind != SK_SCROLL) return;
    int bottom = sst_g.cam_y + SST_VIEW_H + 40;
    for (int i = 0; i < SST_MAX_FOES; i++) {
        SstFoe *f = &sst_g.foe[i];
        if (f->alive && SST_PX(f->y) > bottom) f->alive = 0;
    }
}

/* ---- the arenas: the train's and the lift's waves ---------------------------------------------- */

static void spawn_wave_foe(int kind, int k, int n) {
    bool left = (k + sst_g.wave) % 2 == 0;
    int x, y;
    switch (kind) {
    case FK_PUFFER:
        x = (40 + (k * 97 + sst_g.wave * 53) % 240) * SST_FX;
        y = -(16 + k * 30) * SST_FX;
        break;
    case FK_TOYTRAIN:
        x = left ? -12 * SST_FX : (SCREEN_W + 12) * SST_FX;
        y = sst_g.stage == 1 ? ((3 + (k * 2 + sst_g.wave) % 5) * 16 + 8) * SST_FX : (60 + k * 24) * SST_FX;
        break;
    default:
        x = left ? -10 * SST_FX : (SCREEN_W + 10) * SST_FX;
        y = (40 + (k * 41 + sst_g.wave * 17) % 90) * SST_FX;
        if (kind == FK_BAT || kind == FK_IMP) { x = (40 + (k * 71) % 240) * SST_FX; y = -12 * SST_FX; }
        if (kind == FK_BOOK) y = (50 + (k * 30) % 80) * SST_FX;
        break;
    }
    int j = sst_spawn_foe(kind, x, y);
    if (j >= 0) {
        SstFoe *f = &sst_g.foe[j];
        if (kind == FK_IMP) { f->b = x; f->d = (44 + (k * 23) % 70) * SST_FX; }
        f->dir = left ? 1 : -1;
        f->c = k * 20; /* staggered */
        if (kind == FK_BAT) f->a = 1; /* already out */
        (void)n;
    }
}

static void arena_update(void) {
    const SstStage *st = &SST_STAGE[sst_g.stage];
    if (st->kind == SK_SCROLL) return;
    if (st->kind == SK_LIFT && sst_g.half == 1) return; /* the crown room: the boss decides */
    sst_g.lift_y++;
    int total = st->kind == SK_TRAIN ? sst_train_wave_count() : sst_lift_wave_count();
    if (sst_live_foes() > 0) { sst_g.wave_t = 0; return; }
    if (++sst_g.wave_t < 70) return;
    if (sst_g.wave >= total) {
        if (st->kind == SK_TRAIN) {
            if (!sst_g.clear_t) sst_g.clear_t = 1;
        } else {
            /* the lift reaches the top: the crown room */
            sst_g.half = 1;
            sst_g.next_spawn = 0;
            memset(sst_g.eb, 0, sizeof sst_g.eb);
            memset(sst_g.hz, 0, sizeof sst_g.hz);
            memset(sst_g.card, 0, sizeof sst_g.card);
            sst_g.pl.x = 168 * SST_FX;
            sst_g.pl.y = 136 * SST_FX;
            sst_g.pl.safe_x = sst_g.pl.x;
            sst_g.pl.safe_y = sst_g.pl.y;
            sst_g.cam_y = 0;
            spawn_from_list();
        }
        return;
    }
    int kinds[8], n;
    if (st->kind == SK_TRAIN) sst_train_waves(sst_g.wave, kinds, &n, 8);
    else sst_lift_waves(sst_g.wave, kinds, &n, 8);
    for (int k = 0; k < n; k++) spawn_wave_foe(kinds[k], k, n);
    sst_g.wave++;
    sst_g.wave_t = 0;
}

/* ---- a stage ------------------------------------------------------------------------------------- */

void sst_start_stage(int stage) {
    sst_g.stage = iclamp(stage, 0, SST_STAGES - 1);
    sst_g.half = 0;
    memset(sst_g.foe, 0, sizeof sst_g.foe);
    memset(sst_g.eb, 0, sizeof sst_g.eb);
    memset(sst_g.ps, 0, sizeof sst_g.ps);
    memset(sst_g.card, 0, sizeof sst_g.card);
    memset(sst_g.pass, 0, sizeof sst_g.pass);
    memset(sst_g.hz, 0, sizeof sst_g.hz);
    memset(sst_g.bomb, 0, sizeof sst_g.bomb);
    memset(&sst_g.boss, 0, sizeof sst_g.boss);
    sst_g.cam_lock = -1;
    sst_g.chasing = false;
    sst_g.chase_acc = 0;
    sst_g.next_spawn = 0;
    sst_g.wave = sst_g.wave_t = 0;
    sst_g.lift_y = 0;
    sst_g.clear_t = 0;
    sst_g.stage_done = false;
    sst_g.boss_seen = 0;
    sst_g.heart_next = false;
    SstPlayer *p = &sst_g.pl;
    /* her powers carry over from stage to stage; the hand too */
    p->state = PS_WALK;
    p->st = 0;
    p->vx = p->vy = 0;
    p->inv = 60;
    p->firing = false;
    p->dash_t = p->dash_cd = p->shot_cd = 0;
    p->face = 6;
    p->move_dir = 6;
    p->hand_t = 0;
    /* where she starts */
    const SstSpawn *list;
    int n;
    sst_stage_spawns(sst_g.stage, &list, &n);
    int sx = 160, sy = sst_map_rows() * 16 - 24;
    for (int k = 0; k < n; k++)
        if (list[k].kind == -6) { sx = list[k].x; sy = list[k].y; }
    p->x = sx * SST_FX;
    p->y = sy * SST_FX;
    p->safe_x = p->x;
    p->safe_y = p->y;
    p->opt_x = p->x;
    p->opt_y = p->y;
    for (int k = 0; k < 16; k++) { p->trail_x[k] = p->x; p->trail_y[k] = p->y; }
    if (SST_STAGE[sst_g.stage].kind == SK_SCROLL) {
        sst_g.cam_y = imax(0, sst_map_rows() * 16 - SST_VIEW_H);
        sst_g.cam_y = imin(sst_g.cam_y, imax(0, sy - 120));
    } else {
        sst_g.cam_y = 0;
    }
    camera_update();
}

/* the boss fell: what comes next */
static void after_boss(void) {
    SstBoss *b = &sst_g.boss;
    if (!b->dying || b->die_t != 1) return;
    switch (b->kind) {
    case BK_GOGGLE: case BK_DOPPELS: case BK_MORROW: case BK_HEART:
        sst_g.clear_t = 1;
        break;
    case BK_MAW:
        if (sst_g.continues == 0) sst_g.heart_next = true;
        else sst_g.clear_t = 1;
        break;
    case BK_BOILER:
        sst_g.chasing = false;
        sst_g.cam_lock = -1;
        break;
    default:
        sst_g.cam_lock = -1;
        break;
    }
}

void sst_play_step(uint32_t held) {
    sst_g.held = held;
    uint32_t press = held & ~sst_g.prev;
    sst_g.frame_t++;
    if (sst_g.shake > 0) sst_g.shake--;
    if (sst_g.code_t > 0) sst_g.code_t--;
    player_update(held, press);
    camera_update();
    if (!sst_g.frozen) {
        sst_foes_update();
        if (sst_g.boss.alive) sst_boss_update();
    }
    after_boss();
    if (sst_g.heart_next && !sst_g.boss.alive) {
        /* a clean run: the Morrowmaw breaks open */
        sst_g.heart_next = false;
        memset(sst_g.eb, 0, sizeof sst_g.eb);
        sst_boss_start(BK_HEART, 160 * SST_FX, 90 * SST_FX);
    }
    shots_update();
    bullets_update();
    haz_update();
    pickups_update();
    touch_update();
    cull();
    arena_update();
    if (sst_g.clear_t > 0 && ++sst_g.clear_t > 150) sst_g.stage_done = true;
    sst_g.prev = held;
}
