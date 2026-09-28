/* RIMSHIRE - the flick battles. The field's four corners come from the four
 * board tiles around the spot where the banners met: woods (grass, trees
 * and huts), crags (cliffs close the corner in, cut at an angle), dunes
 * (sand round the edge and a bunker) and lakes (water round the edge and a
 * pool). Each turn a side picks one of the first three disks in its queue
 * (five with COMMAND), aims it, charges it and lets it go, as many times as
 * the disk has moves, then may fire its projectile. While a shot rolls,
 * every knock between one of your disks and a foe hurts the foe by your
 * disk's own melee, and two foes knocked together take 1 each; your disks
 * never hurt each other. Water drowns disks that can't swim; after five
 * turns each the haze closes in, and the disk that ends its turn touching
 * it is gone; five rounds with nothing touched is a stalemate.
 * Every rule and where it comes from is in docs/games/41-rimshire.md. */
#include "rimshire.h"

RshBattle rb;

#define WALL_E 0.84f
#define DISK_E 0.90f
#define STUN_PIPS 2

static const float RADIUS[3] = {6.0f, 8.0f, 11.0f};
static const float MASS[3] = {1.0f, 1.6f, 3.0f};
static const int HP_CAP[3] = {7, 10, 12};

float rsh_pip_speed(int pips) { return pips <= 0 ? 0.0f : 1.3f + 0.95f * (float)pips; }
static float proj_speed(int pips) { return pips <= 0 ? 0.0f : 1.4f + 0.8f * (float)pips; }

int rsh_terrain_at(float x, float y) {
    int cx = (int)(x / RSH_CELL), cy = (int)(y / RSH_CELL);
    if (cx < 0 || cy < 0 || cx >= RSH_CW || cy >= RSH_CH) return T_STONE;
    return rb.cell[cy][cx];
}

void rsh_fog_rect(int *x0, int *y0, int *x1, int *y1) {
    int fx = rb.fog, fy = rb.fog * 2 / 3;
    *x0 = fx;
    *y0 = fy;
    *x1 = RSH_AW - fx;
    *y1 = RSH_AH - fy;
}

int rsh_touches_fog(const RshDisk *d) {
    if (rb.fog <= 0) return 0;
    int x0, y0, x1, y1;
    rsh_fog_rect(&x0, &y0, &x1, &y1);
    return d->x - d->r < (float)x0 || d->y - d->r < (float)y0 || d->x + d->r > (float)x1 || d->y + d->r > (float)y1;
}

int rsh_in_fog(float x, float y) {
    if (rb.fog <= 0) return 0;
    int x0, y0, x1, y1;
    rsh_fog_rect(&x0, &y0, &x1, &y1);
    return x < (float)x0 || y < (float)y0 || x > (float)x1 || y > (float)y1;
}

static void ev(RshPhys *p, int kind, float x, float y, int a) {
    if (p->sim || p->n_ev >= (int)ARRAY_LEN(p->ev)) return;
    p->ev[p->n_ev].kind = (uint8_t)kind;
    p->ev[p->n_ev].x = (int16_t)x;
    p->ev[p->n_ev].y = (int16_t)y;
    p->ev[p->n_ev].a = (int16_t)a;
    p->n_ev++;
}

static int hp_cap(const RshDisk *d) {
    if (d->kind == K_EMPRESS) return 12;
    return HP_CAP[RSH_KIND[d->kind].size];
}

/* ------------------------------------------------------------------ */
/* hits, heals and deaths                                              */

static void kill(RshPhys *p, int i, int water) {
    RshDisk *d = &p->d[i];
    if (!d->on) return;
    d->on = 0;
    d->vx = d->vy = 0;
    if (d->proj) return;
    p->kills[d->side]++;
    if (water) { p->water_deaths++; d->hp = -99; }
    ev(p, water ? BE_SPLASH : BE_KILL, d->x, d->y, d->kind);
}

static void attack(RshPhys *p, int i, int dmg, int fx) {
    RshDisk *d = &p->d[i];
    if (!d->on || d->proj) return;
    if (fx == FX_POISON) {
        /* the poison lands before the damage is counted: a poisoned disk
         * dies to any damage, this very hit included */
        int was = d->poison;
        d->poison = 1;
        if (!was) ev(p, BE_POISON, d->x, d->y, 0);
        if (dmg > 0) { p->hits++; kill(p, i, 0); }
        return;
    }
    if (dmg <= 0) return;
    p->hits++;
    if (d->poison) { kill(p, i, 0); return; }
    d->hp = (int8_t)(d->hp - dmg);
    ev(p, BE_HIT, d->x, d->y, dmg);
    if (fx == FX_STUN && !d->stun) { d->stun = 1; ev(p, BE_STUN, d->x, d->y, 0); }
    if (d->hp <= 0) kill(p, i, 0);
}

/* healing: tonics, springs and the friar's balm. It cures poison (the
 * healing goes into that) and stun, and never goes past the disk's size
 * cap. A leech is hurt instead, by 'sting'. */
static void heal(RshPhys *p, int i, int n, int sting) {
    RshDisk *d = &p->d[i];
    if (!d->on || d->proj) return;
    if (RSH_KIND[d->kind].drain) { attack(p, i, sting, FX_NONE); return; }
    if (d->poison) d->poison = 0;
    else d->hp = (int8_t)imin(d->hp + n, imax(hp_cap(d), d->hp));
    d->stun = 0;
    p->heals += n;
    ev(p, BE_HEAL, d->x, d->y, n);
}

/* the hexer's spell: a ring of six embers where it stops */
static void burst_embers(RshPhys *p, float x, float y) {
    for (int e = 0; e < 6; e++) {
        float a = (float)e * (6.2831853f / 6.0f) + 0.26f;
        float ex = fclamp(x + cosf(a) * 16.0f, 6, RSH_AW - 6), ey = fclamp(y + sinf(a) * 16.0f, 6, RSH_AH - 6);
        if (rsh_terrain_at(ex, ey) == T_STONE) continue;
        for (int k = 0; k < RSH_MAXO; k++)
            if (!p->o[k].on) {
                p->o[k] = (RshObj){1, O_EMBER, 1, 0, ex, ey, 3.0f};
                for (int i = 0; i < RSH_MAXD; i++) p->ocontact[i][k] = 0;
                break;
            }
    }
    ev(p, BE_EMBER, x, y, 0);
}

/* two disks knock together while a shot is rolling */
static void chain_hit(RshPhys *p, int i, int j) {
    int s = p->chain_side;
    if (s < 0) return;
    RshDisk *a = &p->d[i], *b = &p->d[j];
    p->touches++;
    if (a->proj || b->proj) {
        int xi = a->proj ? j : i;
        RshDisk *pr = a->proj ? a : b, *x = &p->d[xi];
        if (x->proj) return;
        switch (RSH_KIND[pr->kind].rkind) {
        case RG_HIT: case RG_STUN: case RG_EMBERS:
            if (x->side != s) { p->combo++; attack(p, xi, p->chain_dmg, p->chain_fx); }
            break;
        case RG_HEAL: /* the balm heals whoever it touches, friend or foe */
            heal(p, xi, RSH_KIND[pr->kind].ranged, 4);
            break;
        case RG_STAR: /* so does the tune: +1 attack, five at most */
            if (x->stars < RSH_MAX_STARS) { x->stars++; p->stars_given++; ev(p, BE_STAR, x->x, x->y, 0); }
            break;
        }
        return;
    }
    bool ea = a->side != s, eb = b->side != s;
    if (!ea && !eb) return; /* your disks never hurt each other */
    p->combo++;
    if (p->combo > p->best_combo) p->best_combo = p->combo;
    if (ea && eb) {
        /* two foes knocked together: 1 each */
        attack(p, i, 1, FX_NONE);
        attack(p, j, 1, FX_NONE);
        return;
    }
    /* one of yours and a foe: the foe takes your disk's melee and its sting */
    int m = ea ? j : i, e = ea ? i : j;
    RshDisk *md = &p->d[m];
    const RshKind *mk = &RSH_KIND[md->kind];
    attack(p, e, mk->melee + md->stars, mk->fx);
    /* the leech drinks from every foe it touches */
    if (mk->drain && md->on && md->hp < hp_cap(md)) md->hp++;
}

/* ------------------------------------------------------------------ */
/* the physics                                                          */

static float inv_mass(const RshPhys *p, int i) {
    const RshDisk *d = &p->d[i];
    /* a menhir can't be knocked about; it moves only when it is launched */
    if (RSH_KIND[d->kind].anchored && !d->proj && !(p->chain_disk == i && !p->chain_ranged && p->chain_side >= 0)) return 0.0f;
    return 1.0f / d->m;
}

static void collide(RshPhys *p, int i, int j) {
    RshDisk *a = &p->d[i], *b = &p->d[j];
    float dx = b->x - a->x, dy = b->y - a->y, rr = a->r + b->r;
    float d2 = dx * dx + dy * dy;
    if (d2 >= rr * rr) {
        if (p->contact[i][j] && d2 > (rr + 1.5f) * (rr + 1.5f)) p->contact[i][j] = p->contact[j][i] = 0;
        return;
    }
    float d = sqrtf(d2), nx = 1, ny = 0;
    if (d > 0.0001f) { nx = dx / d; ny = dy / d; }
    float ia = inv_mass(p, i), ib = inv_mass(p, j);
    if (ia + ib <= 0) { ia = ib = 1; }
    float over = rr - d;
    a->x -= nx * over * ia / (ia + ib);
    a->y -= ny * over * ia / (ia + ib);
    b->x += nx * over * ib / (ia + ib);
    b->y += ny * over * ib / (ia + ib);
    float rel = (b->vx - a->vx) * nx + (b->vy - a->vy) * ny;
    if (rel < 0) {
        float jm = -(1.0f + DISK_E) * rel / (ia + ib);
        a->vx -= jm * ia * nx;
        a->vy -= jm * ia * ny;
        b->vx += jm * ib * nx;
        b->vy += jm * ib * ny;
    }
    if (!p->contact[i][j]) {
        p->contact[i][j] = p->contact[j][i] = 1;
        if (rel < -0.25f) chain_hit(p, i, j);
    }
}

/* pickups (coins, shards, tonics, embers) go to the first disk over them,
 * whoever's turn it is, but never to a projectile; springs, coin rocks and
 * shard rocks stand firm and pay a little for every knock (projectiles can
 * mine rocks, not springs); trees and huts are just in the way */
static void touch_obj(RshPhys *p, int i, int k) {
    RshDisk *d = &p->d[i];
    RshObj *o = &p->o[k];
    float dx = o->x - d->x, dy = o->y - d->y, rr = d->r + o->r;
    float d2 = dx * dx + dy * dy;
    if (d2 >= rr * rr) {
        if (p->ocontact[i][k] && d2 > (rr + 1.5f) * (rr + 1.5f)) p->ocontact[i][k] = 0;
        return;
    }
    int mult = RSH_KIND[d->kind].triple && !d->proj ? 3 : 1;
    int solid = o->kind == O_WELL || o->kind == O_PILE || o->kind == O_CLUSTER || o->kind == O_TREE;
    if (!solid) {
        if (d->proj) return;
        p->touches++;
        switch (o->kind) {
        case O_COIN: p->coins[d->side] += 2 * mult; ev(p, BE_PICK, o->x, o->y, O_COIN); break;
        case O_SHARD: p->shards[d->side] += o->left * mult; ev(p, BE_PICK, o->x, o->y, O_SHARD); break;
        case O_TONIC: heal(p, i, 2, 4); break;
        case O_EMBER: ev(p, BE_EMBER, o->x, o->y, 1); attack(p, i, 1, FX_NONE); break;
        }
        o->on = 0;
        return;
    }
    float d0 = sqrtf(d2), nx = 1, ny = 0;
    if (d0 > 0.0001f) { nx = dx / d0; ny = dy / d0; }
    d->x -= nx * (rr - d0);
    d->y -= ny * (rr - d0);
    float rel = -(d->vx * nx + d->vy * ny);
    if (rel < 0) {
        d->vx += (1.0f + WALL_E) * rel * nx;
        d->vy += (1.0f + WALL_E) * rel * ny;
    }
    if (!p->ocontact[i][k]) {
        p->ocontact[i][k] = 1;
        if (rel > -0.2f || o->kind == O_TREE) return;
        if (d->proj && o->kind == O_WELL) return;
        p->touches++;
        ev(p, BE_STRIKE, o->x, o->y, o->kind);
        if (o->kind == O_WELL) heal(p, i, 1, 3);
        else if (o->kind == O_PILE) p->coins[d->side] += mult;
        else p->shards[d->side] += mult;
        if (--o->left <= 0) o->on = 0;
    }
}

/* the cliffs of a crag corner: straight walls and the angled cut */
static void touch_walls(RshPhys *p, RshDisk *d) {
    for (int w = 0; w < rb.nwall; w++) {
        const float *s = rb.wall[w];
        float ex = s[2] - s[0], ey = s[3] - s[1], len2 = ex * ex + ey * ey;
        float t = len2 > 0 ? ((d->x - s[0]) * ex + (d->y - s[1]) * ey) / len2 : 0;
        t = fclamp(t, 0, 1);
        float cx = s[0] + ex * t, cy = s[1] + ey * t, dx = d->x - cx, dy = d->y - cy;
        float dd = dx * dx + dy * dy;
        if (dd >= d->r * d->r || dd < 0.0001f) continue;
        float dist = sqrtf(dd), nx = dx / dist, ny = dy / dist;
        d->x = cx + nx * d->r;
        d->y = cy + ny * d->r;
        float vn = d->vx * nx + d->vy * ny;
        if (vn < 0) {
            d->vx -= (1.0f + WALL_E) * vn * nx;
            d->vy -= (1.0f + WALL_E) * vn * ny;
            ev(p, BE_WALL, d->x, d->y, 0);
        }
    }
}

static float friction(const RshPhys *p, const RshDisk *d) {
    if (d->proj) return 0.045f;
    switch (rsh_terrain_at(d->x, d->y)) {
    case T_SAND: return (p->skills[d->side] & SK_HOBNAILS) ? 0.06f : 0.22f;
    case T_WATER: return 0.08f;
    default: return 0.06f;
    }
}

void rsh_phys_step(RshPhys *p) {
    if (!p->moving) return;
    p->frames++;
    float maxv = 0;
    for (int i = 0; i < RSH_MAXD; i++) {
        RshDisk *d = &p->d[i];
        if (!d->on) continue;
        float v = fabsf(d->vx) + fabsf(d->vy);
        if (v > maxv) maxv = v;
    }
    int n = 1 + (int)(maxv / 2.5f);
    if (n > 6) n = 6;
    float inv = 1.0f / (float)n;
    for (int s = 0; s < n; s++) {
        for (int i = 0; i < RSH_MAXD; i++) {
            RshDisk *d = &p->d[i];
            if (!d->on || (d->vx == 0 && d->vy == 0)) continue;
            d->x += d->vx * inv;
            d->y += d->vy * inv;
            if (d->x < d->r) { d->x = d->r; d->vx = -d->vx * WALL_E; ev(p, BE_WALL, d->x, d->y, 0); }
            if (d->x > RSH_AW - d->r) { d->x = RSH_AW - d->r; d->vx = -d->vx * WALL_E; ev(p, BE_WALL, d->x, d->y, 0); }
            if (d->y < d->r) { d->y = d->r; d->vy = -d->vy * WALL_E; ev(p, BE_WALL, d->x, d->y, 0); }
            if (d->y > RSH_AH - d->r) { d->y = RSH_AH - d->r; d->vy = -d->vy * WALL_E; ev(p, BE_WALL, d->x, d->y, 0); }
            if (rb.nwall) touch_walls(p, d);
        }
        for (int i = 0; i < RSH_MAXD; i++) {
            if (!p->d[i].on) continue;
            bool mi = p->d[i].vx != 0 || p->d[i].vy != 0;
            for (int j = i + 1; j < RSH_MAXD; j++) {
                if (!p->d[j].on) continue;
                bool mj = p->d[j].vx != 0 || p->d[j].vy != 0;
                if (!mi && !mj && !p->contact[i][j]) continue;
                collide(p, i, j);
                if (!p->d[i].on) break;
            }
        }
        for (int i = 0; i < RSH_MAXD; i++) {
            RshDisk *d = &p->d[i];
            if (!d->on) continue;
            bool mv = d->vx != 0 || d->vy != 0;
            for (int k = 0; k < RSH_MAXO && d->on; k++)
                if (p->o[k].on && (mv || p->ocontact[i][k])) touch_obj(p, i, k);
            /* water: anything that can't swim goes under once its middle is in */
            if (d->on && !d->proj && !RSH_KIND[d->kind].aqua && rsh_terrain_at(d->x, d->y) == T_WATER) kill(p, i, 1);
        }
    }
    int moving = 0;
    for (int i = 0; i < RSH_MAXD; i++) {
        RshDisk *d = &p->d[i];
        if (!d->on || (d->vx == 0 && d->vy == 0)) continue;
        float v = sqrtf(d->vx * d->vx + d->vy * d->vy), nv = v - friction(p, d);
        if (nv <= 0.02f) {
            d->vx = d->vy = 0;
            if (d->proj) {
                /* a projectile that comes to rest: the hexer's spell bursts */
                if (RSH_KIND[d->kind].rkind == RG_EMBERS) burst_embers(p, d->x, d->y);
                kill(p, i, 0);
            }
            continue;
        }
        d->vx *= nv / v;
        d->vy *= nv / v;
        moving = 1;
    }
    if (p->frames > 900) { /* a safety net: nothing rolls forever */
        for (int i = 0; i < RSH_MAXD; i++) {
            p->d[i].vx = p->d[i].vy = 0;
            if (p->d[i].proj) p->d[i].on = 0;
        }
        moving = 0;
    }
    p->moving = moving;
    if (!moving) p->chain_side = -1;
}

static void dir_of(int angle, float *dx, float *dy) {
    float a = (float)angle * (6.2831853f / 256.0f);
    *dx = cosf(a);
    *dy = sinf(a);
}

static int proj_slot(const RshPhys *p) {
    for (int i = 0; i < RSH_MAXD; i++)
        if (!p->d[i].on) return i;
    return -1;
}

/* a projectile needs room to come out: not into a disk, a rock or a wall */
bool rsh_proj_room(const RshPhys *p, int disk, int angle) {
    const RshDisk *d = &p->d[disk];
    float dx, dy;
    dir_of(angle, &dx, &dy);
    float x = d->x + dx * (d->r + 4.5f), y = d->y + dy * (d->r + 4.5f);
    if (x < 3 || y < 3 || x > RSH_AW - 3 || y > RSH_AH - 3) return false;
    if (rsh_terrain_at(x, y) == T_STONE) return false;
    for (int i = 0; i < RSH_MAXD; i++) {
        const RshDisk *o = &p->d[i];
        if (!o->on || i == disk) continue;
        float ex = o->x - x, ey = o->y - y, rr = o->r + 3.0f;
        if (ex * ex + ey * ey < rr * rr) return false;
    }
    for (int k = 0; k < RSH_MAXO; k++) {
        const RshObj *o = &p->o[k];
        if (!o->on || !(o->kind == O_WELL || o->kind == O_PILE || o->kind == O_CLUSTER || o->kind == O_TREE)) continue;
        float ex = o->x - x, ey = o->y - y, rr = o->r + 3.0f;
        if (ex * ex + ey * ey < rr * rr) return false;
    }
    return true;
}

void rsh_launch(RshPhys *p, int disk, int angle, int pips, int ranged) {
    RshDisk *d = &p->d[disk];
    const RshKind *k = &RSH_KIND[d->kind];
    float dx, dy;
    dir_of(angle, &dx, &dy);
    p->chain_side = d->side;
    p->chain_kind = d->kind;
    p->chain_ranged = ranged;
    p->combo = 0;
    if (!ranged) {
        p->chain_disk = disk;
        p->chain_dmg = k->melee + d->stars;
        p->chain_fx = k->fx;
        float v = rsh_pip_speed(pips);
        d->vx = dx * v;
        d->vy = dy * v;
    } else {
        int i = proj_slot(p);
        if (i < 0) { p->chain_side = -1; return; }
        p->chain_disk = -1;
        p->chain_dmg = (k->rkind == RG_HEAL || k->rkind == RG_STAR) ? 0 : k->ranged + d->stars;
        p->chain_fx = k->rkind == RG_STUN ? FX_STUN : FX_NONE;
        RshDisk *pr = &p->d[i];
        memset(pr, 0, sizeof *pr);
        pr->on = 1;
        pr->proj = 1;
        pr->side = d->side;
        pr->kind = d->kind;
        pr->hp = 1;
        pr->r = 3.0f;
        pr->m = 0.5f;
        pr->x = d->x + dx * (d->r + 4.5f);
        pr->y = d->y + dy * (d->r + 4.5f);
        float v = proj_speed(pips);
        pr->vx = dx * v;
        pr->vy = dy * v;
        for (int j = 0; j < RSH_MAXD; j++) p->contact[i][j] = p->contact[j][i] = 0;
        for (int o = 0; o < RSH_MAXO; o++) p->ocontact[i][o] = 0;
        p->shards[d->side]--;
    }
    p->moving = 1;
}

int rsh_disks_left(int side) {
    int n = 0;
    for (int i = 0; i < RSH_MAXD; i++) n += rb.p.d[i].on && !rb.p.d[i].proj && rb.p.d[i].side == side;
    return n;
}

int rsh_eligible(int side, int *out) {
    int want = (rb.p.skills[side] & SK_COMMAND) ? 5 : 3, n = 0;
    for (int q = 0; q < rb.qn[side] && n < want; q++) {
        int i = rb.queue[side][q];
        if (rb.p.d[i].on) out[n++] = i;
    }
    return n;
}

/* the most pips the chosen disk may charge now */
static int max_pips(int ranged) {
    const RshDisk *d = &rb.p.d[rb.sel];
    const RshKind *k = &RSH_KIND[d->kind];
    if (ranged) return k->pcharge;
    return rb.stun_pick ? imin(STUN_PIPS, k->charge) : k->charge;
}

/* ------------------------------------------------------------------ */
/* setting the field                                                     */

static bool spot_free(const RshPhys *p, float x, float y, float r) {
    if (x < r + 4 || y < r + 4 || x > RSH_AW - r - 4 || y > RSH_AH - r - 4) return false;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int t = rsh_terrain_at(x + (float)dx * (r + 2), y + (float)dy * (r + 2));
            if (t == T_WATER || t == T_STONE) return false;
        }
    for (int i = 0; i < RSH_MAXD; i++) {
        const RshDisk *d = &p->d[i];
        if (!d->on) continue;
        float ex = d->x - x, ey = d->y - y, rr = d->r + r + 3;
        if (ex * ex + ey * ey < rr * rr) return false;
    }
    for (int k = 0; k < RSH_MAXO; k++) {
        const RshObj *o = &p->o[k];
        if (!o->on) continue;
        float ex = o->x - x, ey = o->y - y, rr = o->r + r + 4;
        if (ex * ex + ey * ey < rr * rr) return false;
    }
    return true;
}

/* the nearest free spot to (x, y), searching outward */
static void find_spot(const RshPhys *p, float *x, float *y, float r) {
    for (int ring = 0; ring < 60; ring++) {
        for (int k = 0; k < 8 + ring * 4; k++) {
            float a = (float)k / (float)(8 + ring * 4) * 6.2831853f;
            float tx = *x + cosf(a) * (float)ring * 5.0f, ty = *y + sinf(a) * (float)ring * 5.0f;
            if (spot_free(p, tx, ty, r)) { *x = tx; *y = ty; return; }
        }
    }
}

/* quarter q in local coordinates: (0,0) is the field's corner, x and y run
 * in toward the middle */
static int loc_x(int q, float lx) { return (int)((q & 1) ? RSH_AW - lx : lx); }
static int loc_y(int q, float ly) { return (int)((q >> 1) ? RSH_AH - ly : ly); }

static void set_cell_world(int wx, int wy, int t) {
    int cx = wx / RSH_CELL, cy = wy / RSH_CELL;
    if (cx >= 0 && cy >= 0 && cx < RSH_CW && cy < RSH_CH) rb.cell[cy][cx] = (uint8_t)t;
}

/* a blob of sand or water centred at local (lx, ly) */
static void blob(int q, float lx, float ly, int rx, int ry, int t) {
    for (int dy = -ry; dy <= ry; dy += 4)
        for (int dx = -rx; dx <= rx; dx += 4) {
            float fx = (float)dx / (float)rx, fy = (float)dy / (float)ry;
            if (fx * fx + fy * fy > 1.0f) continue;
            int wx = loc_x(q, lx + (float)dx), wy = loc_y(q, ly + (float)dy);
            int cx = wx / RSH_CELL, cy = wy / RSH_CELL;
            if (cx >= 0 && cy >= 0 && cx < RSH_CW && cy < RSH_CH && rb.cell[cy][cx] != T_STONE) rb.cell[cy][cx] = (uint8_t)t;
        }
}

/* sand or water round the two outer edges of a quarter, its line wobbling */
static void edge_band(Rng *r, int q, int t) {
    int half_w = RSH_CW / 2, half_h = RSH_CH / 2;
    int depth = rng_range(r, 2, 4);
    for (int i = 0; i < half_w; i++) {
        depth = iclamp(depth + rng_range(r, -1, 1), 2, 5);
        for (int k = 0; k < depth; k++) set_cell_world(loc_x(q, (float)(i * RSH_CELL + 4)), loc_y(q, (float)(k * RSH_CELL + 4)), t);
    }
    depth = rng_range(r, 2, 4);
    for (int i = 0; i < half_h; i++) {
        depth = iclamp(depth + rng_range(r, -1, 1), 2, 6);
        for (int k = 0; k < depth; k++) set_cell_world(loc_x(q, (float)(k * RSH_CELL + 4)), loc_y(q, (float)(i * RSH_CELL + 4)), t);
    }
}

static void add_wall(int q, float x0, float y0, float x1, float y1) {
    if (rb.nwall >= 16) return;
    float *w = rb.wall[rb.nwall++];
    w[0] = (float)loc_x(q, x0);
    w[1] = (float)loc_y(q, y0);
    w[2] = (float)loc_x(q, x1);
    w[3] = (float)loc_y(q, y1);
}

/* crags: cliffs close the corner in along both edges, cut at an angle in
 * the very corner so disks can be banked round it */
static void crag(Rng *r, int q) {
    float dh = (float)rng_range(r, 22, 34), dv = (float)rng_range(r, 22, 34), c = (float)rng_range(r, 18, 30);
    float hw = RSH_AW / 2.0f, hh = RSH_AH / 2.0f;
    for (int cy = 0; cy < RSH_CH / 2; cy++)
        for (int cx = 0; cx < RSH_CW / 2; cx++) {
            float lx = (float)(cx * RSH_CELL + 4), ly = (float)(cy * RSH_CELL + 4);
            if (ly < dh || lx < dv || lx + ly < dv + dh + c) set_cell_world(loc_x(q, lx), loc_y(q, ly), T_STONE);
        }
    add_wall(q, dv + c, dh, hw, dh);
    add_wall(q, dv, dh + c, dv, hh);
    add_wall(q, dv, dh + c, dv + c, dh);
    add_wall(q, hw, 0, hw, dh);
    add_wall(q, 0, hh, dv, hh);
    if (rng_chance(r, 40)) blob(q, (float)rng_range(r, 90, 150), (float)rng_range(r, 60, 100), 14, 10, T_SAND);
}

static void add_obj_at(int kind, int left, float x, float y) {
    RshPhys *p = &rb.p;
    float rad = (kind == O_WELL || kind == O_PILE || kind == O_CLUSTER) ? 7.0f : kind == O_TREE ? 8.0f : 4.0f;
    for (int k = 0; k < RSH_MAXO; k++)
        if (!p->o[k].on) { p->o[k] = (RshObj){1, (uint8_t)kind, (uint8_t)left, 0, x, y, rad}; return; }
}

static void add_obj(Rng *r, int kind, int left) {
    float rad = (kind == O_WELL || kind == O_PILE || kind == O_CLUSTER) ? 7.0f : 4.0f;
    for (int tries = 0; tries < 80; tries++) {
        float x = (float)rng_range(r, 100, RSH_AW - 100), y = (float)rng_range(r, 20, RSH_AH - 20);
        if (!spot_free(&rb.p, x, y, rad + 2)) continue;
        add_obj_at(kind, left, x, y);
        return;
    }
}

static void corner(Rng *r, int q) {
    switch (rb.quad[q]) {
    case T_STONE: crag(r, q); break;
    case T_SAND:
        edge_band(r, q, T_SAND);
        blob(q, (float)rng_range(r, 110, 150), (float)rng_range(r, 70, 100), rng_range(r, 14, 22), rng_range(r, 10, 16), T_SAND);
        break;
    case T_WATER:
        edge_band(r, q, T_WATER);
        blob(q, (float)rng_range(r, 115, 150), (float)rng_range(r, 75, 100), rng_range(r, 10, 16), rng_range(r, 8, 12), T_WATER);
        break;
    default:
        if (rng_chance(r, 25)) blob(q, (float)rng_range(r, 100, 150), (float)rng_range(r, 60, 100), 12, 9, T_SAND);
        else if (rng_chance(r, 20)) blob(q, (float)rng_range(r, 110, 150), (float)rng_range(r, 70, 100), 10, 8, T_WATER);
        break;
    }
}

/* woods: a few trees and huts to bounce off */
static void trees(Rng *r, int q) {
    int n = rng_range(r, 1, 3);
    for (int i = 0; i < n; i++)
        for (int tries = 0; tries < 30; tries++) {
            float lx = (float)rng_range(r, 50, 180), ly = (float)rng_range(r, 30, 118);
            float x = (float)loc_x(q, lx), y = (float)loc_y(q, ly);
            if (!spot_free(&rb.p, x, y, 10)) continue;
            add_obj_at(O_TREE, 0, x, y);
            break;
        }
}

/* every disk of a side, in a random order, dropped at random in its corner:
 * Brass the top left, Plum the bottom right */
static void add_side(Rng *r, int side) {
    RshSide *s = &rw.s[side];
    RshPhys *p = &rb.p;
    int order[RSH_ARMY];
    for (int a = 0; a < s->n_army; a++) order[a] = a;
    for (int a = s->n_army - 1; a > 0; a--) {
        int b = rng_range(r, 0, a), t = order[a];
        order[a] = order[b];
        order[b] = t;
    }
    rb.qn[side] = 0;
    int q = side ? 3 : 0;
    for (int n = 0; n < s->n_army; n++) {
        int a = order[n], i = proj_slot(p);
        if (i < 0) break;
        RshDisk *d = &p->d[i];
        const RshKind *k = &RSH_KIND[s->army[a]];
        memset(d, 0, sizeof *d);
        d->side = (uint8_t)side;
        d->kind = s->army[a];
        d->slot = (uint8_t)a;
        d->maxhp = (int8_t)(k->hp + ((s->skills & SK_REMEDY) ? 1 : 0));
        d->hp = d->maxhp;
        d->r = RADIUS[k->size];
        d->m = MASS[k->size];
        float x = (float)loc_x(q, (float)rng_range(r, 30, 140)), y = (float)loc_y(q, (float)rng_range(r, 28, 100));
        find_spot(p, &x, &y, d->r);
        d->x = x;
        d->y = y;
        d->on = 1;
        rb.queue[side][rb.qn[side]++] = (uint8_t)i;
    }
}

static void begin_turn(void);

void rsh_battle_start(int attacker, uint32_t seed) {
    Rng r;
    rng_seed(&r, seed);
    memset(&rb, 0, sizeof rb);
    rb.p.chain_side = -1;
    rb.p.chain_disk = -1;
    rb.proj = -1;
    rb.winner = -1;
    rb.ai_disk = -1;
    rb.attacker = attacker;
    /* the four corners take the four tiles around the board node */
    int x = rw.bx, y = rw.by;
    rb.quad[0] = rw.tile[y][x];
    rb.quad[1] = rw.tile[y][x + 1];
    rb.quad[2] = rw.tile[y + 1][x];
    rb.quad[3] = rw.tile[y + 1][x + 1];
    for (int cy = 0; cy < RSH_CH; cy++)
        for (int cx = 0; cx < RSH_CW; cx++) rb.cell[cy][cx] = T_GRASS;
    for (int q = 0; q < 4; q++) corner(&r, q);
    for (int s = 0; s < 2; s++) {
        rb.p.skills[s] = rw.s[s].skills;
        rb.p.shards[s] = (rw.s[s].skills & SK_STOCKPILE) ? 5 : 2;
        rb.cpu[s] = rw.s[s].cpu;
    }
    add_side(&r, 0);
    add_side(&r, 1);
    for (int q = 0; q < 4; q++)
        if (rb.quad[q] == T_GRASS) trees(&r, q);
    /* what lies about the field */
    int coins = rng_range(&r, 2, 4) + rw.battle_coins;
    for (int i = 0; i < coins; i++) add_obj(&r, O_COIN, 1);
    int shards = rng_range(&r, 2, 3);
    for (int i = 0; i < shards; i++) add_obj(&r, O_SHARD, (uint8_t)(rng_chance(&r, 30) ? 2 : 1));
    int tonics = rng_range(&r, 0, 2);
    for (int i = 0; i < tonics; i++) add_obj(&r, O_TONIC, 1);
    if (rng_chance(&r, 60)) add_obj(&r, O_WELL, 5);
    if (rng_chance(&r, 60)) add_obj(&r, O_PILE, 5);
    if (rng_chance(&r, 50)) add_obj(&r, O_CLUSTER, 3);
    rb.side = attacker;
    rb.phase = B_INTRO;
    rb.sel = -1;
    rb.cam_x = attacker ? RSH_AW - SCREEN_W : 0;
    rb.cam_y = attacker ? RSH_AH - RSH_VIEW_H : 0;
}

/* ------------------------------------------------------------------ */
/* turns                                                                 */

static int angle_to(float dx, float dy) {
    float a = atan2f(dy, dx) * (256.0f / 6.2831853f);
    int ai = (int)floorf(a + 0.5f);
    ai &= 255;
    return ai & ~1;
}

static int nearest_enemy_angle(int i) {
    const RshDisk *d = &rb.p.d[i];
    float best = 1e9f;
    int ang = d->side ? 128 : 0;
    for (int j = 0; j < RSH_MAXD; j++) {
        const RshDisk *e = &rb.p.d[j];
        if (!e->on || e->proj || e->side == d->side) continue;
        float dx = e->x - d->x, dy = e->y - d->y, dd = dx * dx + dy * dy;
        if (dd < best) { best = dd; ang = angle_to(dx, dy); }
    }
    return ang;
}

static void check_winner(void) {
    int a = rsh_disks_left(0), b = rsh_disks_left(1);
    if (a && b) return;
    rb.winner = a ? B_WIN0 : b ? B_WIN1 : B_BOTH;
    rb.phase = B_OVER;
    rb.phase_t = 0;
}

static void begin_turn(void) {
    check_winner();
    if (rb.winner >= 0) return;
    int el[5];
    if (rsh_eligible(rb.side, el) == 0) { rb.side ^= 1; }
    rb.phase = B_SELECT;
    rb.phase_t = 0;
    rb.cursor = 0;
    rb.sel = -1;
    rb.pips = 0;
    rb.charging = 0;
}

static void choose(int i) {
    RshDisk *d = &rb.p.d[i];
    rb.sel = i;
    rb.stun_pick = d->stun;
    rb.moves = d->stun ? 1 : RSH_KIND[d->kind].moves;
    rb.move_i = 0;
    rb.ranged = RSH_KIND[d->kind].rkind != RG_NONE;
    rb.angle = nearest_enemy_angle(i);
    rb.pips = 0;
    rb.charging = 0;
    rb.phase = B_AIM;
    rb.phase_t = 0;
}

static void end_turn(void) {
    RshDisk *d = rb.sel >= 0 ? &rb.p.d[rb.sel] : NULL;
    if (d && d->on) {
        /* the haze takes a disk that ends its turn touching it */
        if (rsh_touches_fog(d)) {
            d->on = 0;
            rb.p.kills[d->side]++;
            rb.fog_deaths++;
            ev(&rb.p, BE_KILL, d->x, d->y, d->kind);
        } else {
            d->stun = 0; /* moving clears a stun */
        }
    }
    /* to the back of the queue */
    int s = rb.side, q = 0;
    for (int k = 0; k < rb.qn[s]; k++)
        if (rb.queue[s][k] != rb.sel) rb.queue[s][q++] = rb.queue[s][k];
    if (rb.sel >= 0 && q < rb.qn[s]) rb.queue[s][q++] = (uint8_t)rb.sel;
    rb.qn[s] = q;
    rb.turns[s]++;
    rb.round = imin(rb.turns[0], rb.turns[1]);
    if (rb.round >= RSH_FOG_ROUND) rb.fog = imin(RSH_FOG_MAX, (rb.round - RSH_FOG_ROUND + 1) * RSH_FOG_STEP);
    /* five rounds with nothing touched: a stalemate */
    if (rb.p.touches == rb.touch_mark) rb.quiet++;
    else rb.quiet = 0;
    rb.touch_mark = rb.p.touches;
    rb.sel = -1;
    rb.side ^= 1;
    rb.phase = B_END;
    rb.phase_t = 0;
    check_winner();
    if (rb.winner < 0 && rb.quiet >= RSH_STALE_TURNS) {
        rb.winner = B_STALE;
        rb.phase = B_OVER;
    }
}

static const int DIR_ANGLE[16] = {
    /* bits: up 1, down 2, left 4, right 8 */
    -1, 192, 64, -1, 128, 160, 96, 128, 0, 224, 32, 0, -1, 192, 64, -1,
};

static int held_dir_angle(uint32_t held) {
    int m = 0;
    if (held & BTN_UP) m |= 1;
    if (held & BTN_DOWN) m |= 2;
    if (held & BTN_LEFT) m |= 4;
    if (held & BTN_RIGHT) m |= 8;
    return DIR_ANGLE[m];
}

int rsh_aim_dir_for(int from, int to) {
    static const int MASK[8] = {BTN_RIGHT, BTN_RIGHT | BTN_DOWN, BTN_DOWN, BTN_DOWN | BTN_LEFT,
                                BTN_LEFT, BTN_LEFT | BTN_UP, BTN_UP, BTN_UP | BTN_RIGHT};
    from &= 255;
    to &= 255;
    int diff = ((to - from + 128) & 255) - 128;
    if (diff == 0) return 0;
    int sg = diff > 0 ? 1 : -1;
    int d = sg > 0 ? ((to + 31) / 32) * 32 : (to / 32) * 32;
    int dd = ((d - from + 128) & 255) - 128;
    if (dd * sg <= 0 || iabs(dd) < iabs(diff) || iabs(dd) >= 127) d = (from / 32) * 32 + sg * 64;
    return MASK[((d & 255) / 32) & 7];
}

static void rotate_aim(uint32_t held) {
    int t = held_dir_angle(held);
    if (t < 0) return;
    int diff = ((t - rb.angle + 128) & 255) - 128;
    if (diff == 0) return;
    int step = iabs(diff) <= 2 ? iabs(diff) : 2;
    rb.angle = (rb.angle + (diff > 0 ? step : -step)) & 255;
}

static void cam_follow(void) {
    float tx = -1, ty = -1;
    if (rb.phase == B_ROLL) {
        float best = 0.05f;
        for (int i = 0; i < RSH_MAXD; i++) {
            const RshDisk *d = &rb.p.d[i];
            float v = fabsf(d->vx) + fabsf(d->vy);
            if (d->on && v > best) { best = v; tx = d->x; ty = d->y; }
        }
    } else if (rb.sel >= 0 && rb.p.d[rb.sel].on) {
        tx = rb.p.d[rb.sel].x;
        ty = rb.p.d[rb.sel].y;
    } else if (rb.phase == B_SELECT) {
        int el[5], n = rsh_eligible(rb.side, el);
        if (n > 0) {
            const RshDisk *d = &rb.p.d[el[iclamp(rb.cursor, 0, n - 1)]];
            tx = d->x;
            ty = d->y;
        }
    }
    if (tx < 0) return;
    int wx = iclamp((int)tx - SCREEN_W / 2, 0, RSH_AW - SCREEN_W), wy = iclamp((int)ty - RSH_VIEW_H / 2, 0, RSH_AH - RSH_VIEW_H);
    rb.cam_x += (wx - rb.cam_x) / 6 + isign(wx - rb.cam_x) * (iabs(wx - rb.cam_x) < 6 && wx != rb.cam_x);
    rb.cam_y += (wy - rb.cam_y) / 6 + isign(wy - rb.cam_y) * (iabs(wy - rb.cam_y) < 6 && wy != rb.cam_y);
}

/* the aim and the charge: hold A to add pips, let go to launch (a tap that
 * never reached one pip does nothing, or skips a projectile) */
static void aim_step(uint32_t held, uint32_t pressed, uint32_t released, int ranged) {
    int max = max_pips(ranged);
    bool dpad = (held & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)) != 0;
    if (!rb.cam_free) rotate_aim(held);
    if (ranged) rb.no_room = !rsh_proj_room(&rb.p, rb.sel, rb.angle);
    if (pressed & BTN_A) { rb.charging = 1; rb.charge_t = 0; rb.pips = 0; rb.still_t = 0; }
    if (rb.charging && (held & BTN_A)) {
        rb.charge_t++;
        rb.pips = imin(max, rb.charge_t / RSH_PIP_FRAMES);
        if (dpad) rb.still_t = 0;
        else if (++rb.still_t >= RSH_RESET_FRAMES) { rb.charge_t = 0; rb.pips = 0; rb.still_t = 0; }
    }
    if (rb.charging && (released & BTN_A)) {
        rb.charging = 0;
        if (rb.pips < 1) {
            rb.pips = 0;
            if (ranged) { rb.skip_ranged++; end_turn(); }
            return;
        }
        if (ranged && rb.no_room) { rb.pips = 0; sfx_play_name("rsh_nope"); return; }
        rsh_launch(&rb.p, rb.sel, rb.angle, rb.pips, ranged);
        if (ranged) {
            for (int i = 0; i < RSH_MAXD; i++)
                if (rb.p.d[i].on && rb.p.d[i].proj) rb.proj = i;
        }
        rb.launches++;
        sfx_play_name(ranged ? "rsh_shoot" : "rsh_flick");
        rb.phase = B_ROLL;
        rb.phase_t = 0;
        rb.skip_ranged = 0;
        if (ranged) { rb.ranged = 0; rb.move_i = rb.moves; }
        else rb.move_i++;
    }
}

void rsh_battle_step(uint32_t held, uint32_t pressed, uint32_t released) {
    rb.phase_t++;
    rb.p.n_ev = 0;
    /* camera mode: hold B and the d-pad moves the view, at any time in your
     * turn, even while charging; let go and it follows the action again */
    rb.cam_free = (held & BTN_B) && (rb.phase == B_SELECT || rb.phase == B_AIM || rb.phase == B_RAIM);
    if (rb.cam_free) {
        if (held & BTN_LEFT) rb.cam_x -= 3;
        if (held & BTN_RIGHT) rb.cam_x += 3;
        if (held & BTN_UP) rb.cam_y -= 3;
        if (held & BTN_DOWN) rb.cam_y += 3;
        rb.cam_x = iclamp(rb.cam_x, 0, RSH_AW - SCREEN_W);
        rb.cam_y = iclamp(rb.cam_y, 0, RSH_AH - RSH_VIEW_H);
    } else {
        cam_follow();
    }
    switch (rb.phase) {
    case B_INTRO:
        if (rb.phase_t > 50) begin_turn();
        break;
    case B_SELECT: {
        int el[5], n = rsh_eligible(rb.side, el);
        if (n == 0) { check_winner(); break; }
        rb.cursor = iclamp(rb.cursor, 0, n - 1);
        if (!rb.cam_free) {
            if (pressed & BTN_LEFT) { rb.cursor = (rb.cursor + n - 1) % n; sfx_play_name("ui_move"); }
            if (pressed & BTN_RIGHT) { rb.cursor = (rb.cursor + 1) % n; sfx_play_name("ui_move"); }
        }
        if (pressed & BTN_A) { sfx_play_name("ui_ok"); choose(el[rb.cursor]); }
        break;
    }
    case B_AIM:
        if (!rb.p.d[rb.sel].on) { end_turn(); break; }
        aim_step(held, pressed, released, 0);
        break;
    case B_RAIM:
        if (!rb.p.d[rb.sel].on) { end_turn(); break; }
        aim_step(held, pressed, released, 1);
        break;
    case B_ROLL:
        rsh_phys_step(&rb.p);
        /* healed on its way, a stunned disk gets its whole turn back */
        if (rb.stun_pick && rb.sel >= 0 && rb.p.d[rb.sel].on && !rb.p.d[rb.sel].stun) {
            rb.stun_pick = 0;
            rb.moves = RSH_KIND[rb.p.d[rb.sel].kind].moves;
        }
        if (rb.p.moving) break;
        rb.proj = -1;
        check_winner();
        if (rb.winner >= 0) break;
        if (rb.sel < 0 || !rb.p.d[rb.sel].on) { end_turn(); break; }
        rb.pips = 0;
        rb.charging = 0;
        rb.phase_t = 0;
        if (rb.move_i < rb.moves) rb.phase = B_AIM;
        else if (rb.ranged && rb.p.shards[rb.side] > 0) rb.phase = B_RAIM;
        else end_turn();
        break;
    case B_END:
        if (rb.winner >= 0) { rb.phase = B_OVER; break; }
        if (rb.phase_t > 16) begin_turn();
        break;
    case B_OVER:
        rb.over_t++;
        break;
    }
}

/* ------------------------------------------------------------------ */
/* the aim line. Without SIGHTLINE, a few dots off the disk; with it, the
 * path up to the first thing it meets (a short way only), then where the
 * thing it strikes will go (kind 1) and where the shot bounces (kind 2) */

int rsh_count_trace(int angle, int pips, int16_t *xs, int16_t *ys, uint8_t *kinds, int max) {
    if (rb.sel < 0) return 0;
    const RshDisk *d = &rb.p.d[rb.sel];
    bool ranged = rb.phase == B_RAIM;
    float dx, dy;
    dir_of(angle, &dx, &dy);
    float x = d->x, y = d->y, r = ranged ? 3.0f : d->r;
    int n = 0;
    if (!(rb.p.skills[d->side] & SK_SIGHTLINE)) {
        for (int i = 0; i < 4 && n < max; i++) {
            xs[n] = (int16_t)(x + dx * (d->r + 5.0f + (float)i * 6.0f));
            ys[n] = (int16_t)(y + dy * (d->r + 5.0f + (float)i * 6.0f));
            kinds[n++] = 0;
        }
        return n;
    }
    int p = pips > 0 ? pips : max_pips(ranged);
    float v = ranged ? proj_speed(p) : rsh_pip_speed(p), fr = ranged ? 0.045f : 0.06f;
    float left = fminf(v * v / (2.0f * fr), 110.0f);
    if (ranged) { x += dx * (d->r + 4.5f); y += dy * (d->r + 4.5f); }
    float step = 3.0f, run = 0;
    while (left > 0 && n < max) {
        x += dx * step;
        y += dy * step;
        left -= step;
        run += step;
        if (x < r) { x = r; dx = -dx; }
        if (x > RSH_AW - r) { x = RSH_AW - r; dx = -dx; }
        if (y < r) { y = r; dy = -dy; }
        if (y > RSH_AH - r) { y = RSH_AH - r; dy = -dy; }
        float hx = 0, hy = 0, hr = 0;
        bool hit = false;
        for (int i = 0; i < RSH_MAXD && !hit; i++) {
            const RshDisk *o = &rb.p.d[i];
            if (!o->on || i == rb.sel) continue;
            float ex = o->x - x, ey = o->y - y, rr = o->r + r;
            if (ex * ex + ey * ey < rr * rr) { hit = true; hx = o->x; hy = o->y; hr = o->r; }
        }
        for (int k = 0; k < RSH_MAXO && !hit; k++) {
            const RshObj *o = &rb.p.o[k];
            if (!o->on || !(o->kind == O_WELL || o->kind == O_PILE || o->kind == O_CLUSTER || o->kind == O_TREE)) continue;
            float ex = o->x - x, ey = o->y - y, rr = o->r + r;
            if (ex * ex + ey * ey < rr * rr) { hit = true; hx = o->x; hy = o->y; hr = o->r; }
        }
        if (run >= 6.0f || hit) {
            run = 0;
            xs[n] = (int16_t)x;
            ys[n] = (int16_t)y;
            kinds[n++] = 0;
        }
        if (hit) {
            float nx = hx - x, ny = hy - y, nl = sqrtf(nx * nx + ny * ny);
            if (nl < 0.001f) break;
            nx /= nl;
            ny /= nl;
            for (int i = 1; i <= 4 && n < max; i++) {
                xs[n] = (int16_t)(hx + nx * (hr + (float)i * 5.0f));
                ys[n] = (int16_t)(hy + ny * (hr + (float)i * 5.0f));
                kinds[n++] = 1;
            }
            float vn = dx * nx + dy * ny, bx = dx - 2 * vn * nx, by = dy - 2 * vn * ny;
            for (int i = 1; i <= 3 && n < max; i++) {
                xs[n] = (int16_t)(x + bx * (float)i * 5.0f);
                ys[n] = (int16_t)(y + by * (float)i * 5.0f);
                kinds[n++] = 2;
            }
            break;
        }
    }
    return n;
}

/* ------------------------------------------------------------------ */
/* the computer's shots: it tries shots on a copy of the field and keeps
 * the one that scores best. Early on it is sloppy (it takes one of its
 * better ideas, not the best, and its hand shakes); later its aim is
 * perfect. It loves trick shots that knock many disks and pick up much,
 * cares little which disks it hurts, and never quite sees its own disks
 * drowning coming. The player's demo player uses the same search, with a
 * steady hand, a sense of what each disk is worth and a fear of water. */

typedef struct { int disk, angle, pips; float score; } Cand;

static struct {
    int side, bot, mode;       /* mode 0: choose and first move, 1: next move, 2: projectile */
    int el[5], ne;
    int stage, idx, total;
    int final_move;
    Cand top[6];
    int ntop;
    Cand coarse_best;
} pl;
int rsh_ai_busy;

static RshPhys sim;

static float score_shot(int disk, int final) {
    const RshPhys *b = &rb.p, *a = &sim;
    int s = pl.side;
    float sc = 0;
    for (int i = 0; i < RSH_MAXD; i++) {
        const RshDisk *x0 = &b->d[i], *x1 = &a->d[i];
        if (!x0->on || x0->proj) continue;
        int val = pl.bot ? rsh_kind_value(x0->kind) : 12;
        if (x0->side != s) {
            if (!x1->on) sc += (float)(val * 6 + 10);
            else {
                sc += 3.0f * (float)(x0->hp - x1->hp);
                if (x1->hp < x0->hp) sc += pl.bot ? 0.0f : 3.0f; /* one more disk knocked */
                if (x1->poison && !x0->poison) sc += 5.0f + (float)val * 0.5f;
                if (x1->stun && !x0->stun) sc += 2.0f;
                sc -= 4.0f * (float)(x1->stars - x0->stars); /* a foe tuned up */
                if (x1->hp > x0->hp) sc -= 2.0f * (float)(x1->hp - x0->hp);
            }
        } else {
            if (!x1->on) {
                /* the computer never quite sees water coming */
                if (!(x1->hp == -99 && !pl.bot)) sc -= (float)(val * 6 + 10);
            } else {
                sc += 2.0f * (float)(x1->hp - x0->hp);
                sc += 3.0f * (float)(x1->stars - x0->stars);
                if (x0->poison && !x1->poison) sc += 4.0f;
            }
        }
    }
    float greed = pl.bot ? 1.0f : 2.5f;
    sc += (float)(a->coins[s] - b->coins[s]) * greed;
    sc += (float)(a->shards[s] - b->shards[s]) * 2.0f * greed;
    /* embers left by the shot: good near foes, bad near friends */
    for (int k = 0; k < RSH_MAXO; k++) {
        const RshObj *o = &a->o[k];
        if (!o->on || o->kind != O_EMBER || b->o[k].on) continue;
        for (int i = 0; i < RSH_MAXD; i++) {
            const RshDisk *x = &a->d[i];
            if (!x->on || x->proj) continue;
            float dx = x->x - o->x, dy = x->y - o->y;
            if (dx * dx + dy * dy < 30 * 30) sc += x->side == s ? -1.5f : 1.5f;
        }
    }
    const RshDisk *me = &a->d[disk];
    if (me->on) {
        if (rsh_touches_fog(me)) sc -= final ? (float)(rsh_kind_value(me->kind) * 6 + 10) : 6.0f;
        /* the next haze step */
        if (final && rb.round + 1 >= RSH_FOG_ROUND) {
            int keep = rb.fog;
            rb.fog = imin(RSH_FOG_MAX, (rb.round + 2 - RSH_FOG_ROUND) * RSH_FOG_STEP);
            if (rsh_touches_fog(me)) sc -= 4.0f;
            rb.fog = keep;
        }
        float best = 1e9f;
        for (int i = 0; i < RSH_MAXD; i++) {
            const RshDisk *e = &a->d[i];
            if (!e->on || e->proj || e->side == s) continue;
            float dx = e->x - me->x, dy = e->y - me->y, dd = sqrtf(dx * dx + dy * dy);
            if (dd < best) best = dd;
        }
        if (best < 1e8f) sc -= best * 0.012f;
        if (pl.bot && !RSH_KIND[me->kind].aqua) {
            /* keep off the water's edge */
            for (int k = 0; k < 8; k++) {
                float ang = (float)k * 0.785f;
                if (rsh_terrain_at(me->x + cosf(ang) * (me->r + 10), me->y + sinf(ang) * (me->r + 10)) == T_WATER) { sc -= 1.5f; break; }
            }
        }
    }
    return sc;
}

static float run_sim(int disk, int angle, int pips, int ranged) {
    memcpy(&sim, &rb.p, sizeof sim);
    sim.sim = 1;
    if (ranged && (!rsh_proj_room(&sim, disk, angle) || sim.shards[pl.side] <= 0)) return -1e9f;
    rsh_launch(&sim, disk, angle, pips, ranged);
    for (int f = 0; f < 700 && sim.moving; f++) rsh_phys_step(&sim);
    return score_shot(disk, pl.final_move);
}

static int cand_max(int disk, int ranged) {
    const RshDisk *d = &rb.p.d[disk];
    const RshKind *k = &RSH_KIND[d->kind];
    if (ranged) return k->pcharge;
    return d->stun ? imin(STUN_PIPS, k->charge) : k->charge;
}

static int pip_set(int disk, int i, int ranged) {
    int c = cand_max(disk, ranged);
    if (i == 0) return imax(1, c / 3);
    if (i == 1) return imax(1, (2 * c + 2) / 3);
    return c;
}

static void keep_top(Cand c) {
    int n = pl.ntop;
    if (n < 6) pl.top[pl.ntop++] = c;
    else if (c.score > pl.top[5].score) pl.top[5] = c;
    else return;
    for (int i = pl.ntop - 1; i > 0 && pl.top[i].score > pl.top[i - 1].score; i--) {
        Cand t = pl.top[i];
        pl.top[i] = pl.top[i - 1];
        pl.top[i - 1] = t;
    }
}

void rsh_ai_begin(int side, int bot) {
    memset(&pl, 0, sizeof pl);
    pl.side = side;
    pl.bot = bot;
    if (rb.phase == B_SELECT) {
        pl.mode = 0;
        pl.ne = rsh_eligible(side, pl.el);
    } else {
        pl.mode = rb.phase == B_RAIM ? 2 : 1;
        pl.ne = 1;
        pl.el[0] = rb.sel;
    }
    pl.total = pl.ne * 32 * (pl.mode == 2 ? 2 : 3);
    pl.stage = 0;
    rsh_ai_busy = 1;
}

static int final_for(int disk) {
    const RshDisk *d = &rb.p.d[disk];
    int moves = d->stun ? 1 : RSH_KIND[d->kind].moves;
    int done = pl.mode == 0 ? 0 : rb.move_i;
    bool ranged_next = RSH_KIND[d->kind].rkind != RG_NONE && rb.p.shards[pl.side] > 0;
    if (pl.mode == 2) return 1;
    return done + 1 >= moves && !ranged_next;
}

int rsh_ai_work(int budget) {
    if (!rsh_ai_busy) return 1;
    int ranged = pl.mode == 2;
    while (budget-- > 0) {
        if (pl.stage == 0) {
            if (pl.idx >= pl.total) {
                pl.stage = 1;
                pl.idx = 0;
                pl.coarse_best = pl.ntop ? pl.top[0] : (Cand){pl.el[0], 0, 1, -1e9f};
                continue;
            }
            int per = 32 * (ranged ? 2 : 3);
            int di = pl.idx / per, rest = pl.idx % per;
            int ai = rest % 32, pi = rest / 32;
            int disk = pl.el[di];
            int ang = (ai * 8 + (pl.side ? 4 : 0)) & 255;
            int pips = pip_set(disk, ranged ? pi + 1 : pi, ranged);
            pl.final_move = final_for(disk);
            Cand c = {disk, ang, pips, run_sim(disk, ang, pips, ranged)};
            keep_top(c);
            pl.idx++;
        } else if (pl.stage == 1) {
            /* refine round the best: finer angles, every power */
            Cand b = pl.coarse_best;
            int c = cand_max(b.disk, ranged);
            int n = 7 * c;
            if (pl.idx >= n) { pl.stage = 2; break; }
            int off = (pl.idx % 7 - 3) * 2, pips = pl.idx / 7 + 1;
            if (off != 0 || pips != b.pips) {
                int ang = (b.angle + off) & 255;
                pl.final_move = final_for(b.disk);
                keep_top((Cand){b.disk, ang, pips, run_sim(b.disk, ang, pips, ranged)});
            }
            pl.idx++;
        } else break;
    }
    if (pl.stage < 2) return 0;
    /* decide */
    rsh_ai_busy = 0;
    Cand pick = pl.ntop ? pl.top[0] : (Cand){pl.el[0], rb.angle, 1, 0};
    int lvl = pl.bot ? 10 : iclamp(rb.cpu[pl.side], 0, 10);
    if (!pl.bot && pl.ntop > 1) {
        int k = imin(pl.ntop, 1 + (10 - lvl) / 3);
        pick = pl.top[rng_range(&g_rng, 0, k - 1)];
        int shake = (10 - lvl);
        if (shake > 0) pick.angle = (pick.angle + rng_range(&g_rng, -shake, shake) * 2) & 255;
        if (lvl < 5 && rng_chance(&g_rng, 40)) {
            int c = cand_max(pick.disk, ranged);
            pick.pips = iclamp(pick.pips + (rng_chance(&g_rng, 50) ? 1 : -1), 1, c);
        }
    }
    rb.ai_disk = pick.disk;
    rb.ai_angle = pick.angle & ~1;
    rb.ai_pips = pick.pips;
    /* a projectile that does nothing useful is skipped with a tap */
    rb.ai_skip = ranged && pick.score <= 0.5f;
    return 1;
}

/* The computer plays through the same buttons as a player: LEFT/RIGHT and
 * A to pick its disk, the d-pad to turn the aim, A held for the pips and
 * let go to launch (a tap to skip a projectile). */
uint32_t rsh_battle_buttons(int side, int bot, uint32_t prev) {
    int ph = rb.phase;
    if (rb.side != side || (ph != B_SELECT && ph != B_AIM && ph != B_RAIM)) return 0;
    int key = 1 + (((rb.turns[0] + rb.turns[1]) * 8 + ph) * 8 + rb.move_i);
    if (rb.ai_key != key) {
        rb.ai_key = key;
        rb.ai_ready = 0;
        rsh_ai_begin(side, bot);
    }
    if (!rb.ai_ready) {
        if (!rsh_ai_work(bot ? 1000 : 8)) return 0; /* 8 tries a frame: light enough for the Vita */
        rb.ai_ready = 1;
    }
    if (ph == B_SELECT) {
        int el[5], n = rsh_eligible(side, el), want = 0;
        for (int i = 0; i < n; i++)
            if (el[i] == rb.ai_disk) want = i;
        if (rb.cursor != want) return (prev & BTN_RIGHT) ? 0 : BTN_RIGHT;
        return (prev & BTN_A) ? 0 : BTN_A;
    }
    if (rb.ai_skip && ph == B_RAIM) return rb.charging || (prev & BTN_A) ? 0 : BTN_A;
    if (!rb.charging) {
        if (rb.angle != rb.ai_angle) return (uint32_t)rsh_aim_dir_for(rb.angle, rb.ai_angle);
        return (prev & BTN_A) ? 0 : BTN_A;
    }
    return rb.pips < rb.ai_pips ? BTN_A : 0;
}
