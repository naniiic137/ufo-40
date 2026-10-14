/* FOSSIL FEUD - battles. When birds march onto a tile held by the other
 * side, a fight plays out by itself on a field shaped by the tile's
 * ground: the Rust Roost always starts on the left, the Slate Flock on the
 * right. Each side only gives its whole army orders: CHARGE, STAND, WRECK
 * (pull down the buildings, even your own) or FLEE. At most 40 birds a
 * side are on the field; the rest wait and come on as others fall. If
 * nobody is hurt and nobody moves for three seconds, the attackers go
 * home. */
#include "feud.h"

bool feud_quiet;

#define FX 256
#define PX(v) ((v) >> 8)

static int is_left(const FeudBattle *b, int side) { (void)b; return side == 0; }

static int cell_at(const FeudBattle *b, int px, int py) {
    int cx = px / FB_CELL, cy = py / FB_CELL;
    if (cx < 0 || cy < 0 || cx >= FB_GW || cy >= FB_GH) return BC_BLOCK;
    return b->grid[cy][cx];
}

/* ---- the field --------------------------------------------------------------- */

static void put(FeudBattle *b, int cx, int cy, int what, bool mirror) {
    if (cx < 0 || cy < 0 || cx >= FB_GW || cy >= FB_GH) return;
    if (mirror) cx = FB_GW - 1 - cx;
    b->grid[cy][cx] = (uint8_t)what;
}

/* laid out with the defenders on the right; turned round when they are on the left */
static void lay_field(FeudBattle *b, int terr, int tile) {
    bool mir = is_left(b, b->def);
    Rng r;
    rng_seed(&r, (uint64_t)(tile * 131 + terr * 7 + 1));
    memset(b->grid, 0, sizeof b->grid);
    switch (terr) {
    case FT_THICKET:
        for (int i = 0; i < 8; i++) {
            int cx = rng_range(&r, 7, 22), cy = rng_range(&r, 0, FB_GH - 2);
            put(b, cx, cy, BC_COVER, mir); put(b, cx + 1, cy, BC_COVER, mir);
            put(b, cx, cy + 1, BC_COVER, mir); put(b, cx + 1, cy + 1, BC_COVER, mir);
        }
        break;
    case FT_BONEYARD:
        for (int i = 0; i < 5; i++) {
            int cx = rng_range(&r, 7, 22), cy = rng_range(&r, 0, FB_GH - 1);
            put(b, cx, cy, BC_COVER, mir); put(b, cx + 1, cy, BC_COVER, mir);
        }
        for (int i = 0; i < 2; i++) put(b, rng_range(&r, 9, 20), rng_range(&r, 1, FB_GH - 2), BC_BLOCK, mir);
        break;
    case FT_SCREE:
        for (int i = 0; i < 6; i++) {
            int cx = rng_range(&r, 7, 23), cy = rng_range(&r, 0, FB_GH - 1);
            put(b, cx, cy, BC_BLOCK, mir);
            if (i & 1) put(b, cx + 1, cy, BC_BLOCK, mir);
        }
        for (int i = 0; i < 4; i++) {
            int cx = rng_range(&r, 4, 25), cy = rng_range(&r, 0, FB_GH - 2);
            for (int dx = 0; dx < 2; dx++)
                for (int dy = 0; dy < 2; dy++)
                    if (b->grid[cy + dy][mir ? FB_GW - 1 - (cx + dx) : cx + dx] == BC_OPEN) put(b, cx + dx, cy + dy, BC_HIGH, mir);
        }
        break;
    case FT_GORGE:
        /* a chasm across the middle, two narrow bridges */
        for (int cy = 0; cy < FB_GH; cy++)
            if (cy != 2 && cy != 8) { put(b, 14, cy, BC_BLOCK, mir); put(b, 15, cy, BC_BLOCK, mir); }
        break;
    case FT_PEAK:
        for (int cx = 20; cx < FB_GW; cx++)
            for (int cy = 0; cy < FB_GH; cy++) put(b, cx, cy, BC_HIGH, mir);
        put(b, 19, 3, BC_BLOCK, mir);
        put(b, 19, 7, BC_BLOCK, mir);
        break;
    case FT_EYRIE:
        /* walls with one gate, battlements behind */
        for (int cy = 0; cy < FB_GH; cy++)
            if (cy != 5) put(b, 21, cy, BC_BLOCK, mir);
        for (int cx = 22; cx < FB_GW; cx++)
            for (int cy = 0; cy < FB_GH; cy++) put(b, cx, cy, BC_HIGH, mir);
        break;
    default: break;
    }
}

static bool walker(int kind) { return kind != FU_SWIFT; }

static int free_y(FeudBattle *b, int px, int pref) {
    for (int d = 0; d < FB_FH; d += 3) {
        int ys[2] = {pref + d, pref - d};
        for (int i = 0; i < 2; i++)
            if (ys[i] > 4 && ys[i] < FB_FH - 4 && cell_at(b, px, ys[i]) != BC_BLOCK) return ys[i];
    }
    return pref;
}

static int on_field(const FeudBattle *b, int side) {
    int n = 0;
    for (int i = 0; i < FB_MAX; i++) n += b->s[i].alive && b->s[i].side == side;
    return n;
}

static void spawn(FeudBattle *b, int side, int kind, bool at_edge) {
    int slot = -1;
    for (int i = 0; i < FB_MAX; i++)
        if (!b->s[i].alive) { slot = i; break; }
    if (slot < 0) return;
    FeudSoldier *s = &b->s[slot];
    memset(s, 0, sizeof *s);
    s->alive = 1;
    s->side = (uint8_t)side;
    s->kind = (uint8_t)kind;
    s->hp = (int16_t)FEUD_UNIT[kind].hp;
    s->shield = (int16_t)FEUD_UNIT[kind].shield;
    s->tgt = -1;
    s->wp = -1;
    s->cd = rng_range(&b->rng, 10, 50);
    s->path_t = rng_range(&b->rng, 0, 19);
    s->frozen = side == b->def && b->frozen_def;
    static const int DEPTH[FU_KINDS] = {50, 52, 14, 26, 38};
    int depth = at_edge ? 3 : DEPTH[kind] + rng_range(&b->rng, -6, 6);
    int px = is_left(b, side) ? depth : FB_FW - depth;
    int py = free_y(b, px, rng_range(&b->rng, 8, FB_FH - 8));
    s->x = px * FX;
    s->y = py * FX;
    s->face = is_left(b, side) ? 1 : -1;
}

void feud_battle_begin(FeudBattle *b, const FeudMatch *m, int atk, int src, int dst, const uint8_t counts[FU_KINDS], bool surprise) {
    memset(b, 0, sizeof *b);
    rng_seed(&b->rng, ((uint64_t)m->turn << 16) ^ (uint64_t)(dst * 977 + src * 31 + 5) ^ m->rng.state);
    b->atk = atk;
    b->def = 1 - atk;
    b->src = src;
    b->dst = dst;
    const FeudTile *t = &m->t[dst];
    b->terr = t->terr;
    b->surprise = surprise;
    b->frozen_def = t->frozen > 0;
    lay_field(b, t->terr, dst);
    for (int k = 0; k < FU_KINDS; k++) {
        b->reserve[atk][k] = counts[k];
        b->reserve[b->def][k] = t->u[k];
    }
    /* the defenders' buildings, at their back */
    int dl = is_left(b, b->def);
    for (int k = 0; k < FB_KINDS; k++)
        for (int n = 0; n < t->b[k] && b->nbld < FD_BLD_MAX; n++) {
            FeudFieldBld *f = &b->bld[b->nbld];
            f->kind = k;
            f->hp = FB_BLD_HP;
            f->x = dl ? 14 : FB_FW - 14;
            f->y = 22 + b->nbld * 33;
            b->nbld++;
        }
    /* the first forty a side take the field, the rest wait */
    for (int side = 0; side < 2; side++) {
        static const int ORDER[FU_KINDS] = {FU_LANCER, FU_STOMPER, FU_SWIFT, FU_MYSTIC, FU_SLINGER};
        int placed = 0;
        bool more = true;
        while (more && placed < FB_ON_FIELD) {
            more = false;
            for (int i = 0; i < FU_KINDS && placed < FB_ON_FIELD; i++) {
                int k = ORDER[i];
                if (b->reserve[side][k] > 0) {
                    b->reserve[side][k]--;
                    spawn(b, side, k, false);
                    placed++;
                    more = true;
                }
            }
        }
    }
    b->order[0] = b->order[1] = FO_CHARGE;
    b->result = BR_NONE;
}

static int reserve_total(const FeudBattle *b, int side) {
    int n = 0;
    for (int k = 0; k < FU_KINDS; k++) n += b->reserve[side][k];
    return n;
}

int feud_battle_count(const FeudBattle *b, int side) { return on_field(b, side) + reserve_total(b, side); }

int feud_battle_power(const FeudBattle *b, int side) {
    int p = 0;
    for (int i = 0; i < FB_MAX; i++) {
        const FeudSoldier *s = &b->s[i];
        if (!s->alive || s->side != side) continue;
        const FeudUnitDef *u = &FEUD_UNIT[s->kind];
        p += u->worth * 16 * (s->hp + s->shield) / (u->hp + u->shield);
    }
    for (int k = 0; k < FU_KINDS; k++) p += b->reserve[side][k] * FEUD_UNIT[k].worth * 16;
    return p;
}

/* ---- paths ---------------------------------------------------------------- */

/* the next cell toward (tx, ty) round walls and chasms, from a breadth-first fill */
static int next_cell(const FeudBattle *b, int fx, int fy, int tx, int ty) {
    static int16_t dist[FB_GH * FB_GW];
    static int16_t q[FB_GH * FB_GW];
    int from = fy * FB_GW + fx, goal = ty * FB_GW + tx;
    if (from == goal) return goal;
    for (int i = 0; i < FB_GH * FB_GW; i++) dist[i] = -1;
    int qh = 0, qt = 0;
    dist[goal] = 0;
    q[qt++] = (int16_t)goal;
    while (qh < qt) {
        int c = q[qh++];
        int cx = c % FB_GW, cy = c / FB_GW;
        static const int D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (int d = 0; d < 4; d++) {
            int nx = cx + D[d][0], ny = cy + D[d][1];
            if (nx < 0 || ny < 0 || nx >= FB_GW || ny >= FB_GH) continue;
            int n = ny * FB_GW + nx;
            if (dist[n] >= 0 || (b->grid[ny][nx] == BC_BLOCK && n != from)) continue;
            dist[n] = (int16_t)(dist[c] + 1);
            q[qt++] = (int16_t)n;
        }
    }
    int best = -1, bd = 1 << 14;
    static const int D8[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (int d = 0; d < 8; d++) {
        int nx = fx + D8[d][0], ny = fy + D8[d][1];
        if (nx < 0 || ny < 0 || nx >= FB_GW || ny >= FB_GH) continue;
        if (b->grid[ny][nx] == BC_BLOCK) continue;
        if (d >= 4 && (b->grid[fy][nx] == BC_BLOCK || b->grid[ny][fx] == BC_BLOCK)) continue;
        int n = ny * FB_GW + nx;
        if (dist[n] >= 0 && dist[n] < bd) { bd = dist[n]; best = n; }
    }
    return best;
}

static bool line_clear(const FeudBattle *b, int x0, int y0, int x1, int y1) {
    int steps = imax(iabs(x1 - x0), iabs(y1 - y0)) / 4 + 1;
    for (int i = 1; i < steps; i++) {
        int x = x0 + (x1 - x0) * i / steps, y = y0 + (y1 - y0) * i / steps;
        if (cell_at(b, x, y) == BC_BLOCK) return false;
    }
    return true;
}

/* ---- harm ------------------------------------------------------------------ */

static void sfx(const char *n) {
    if (!feud_quiet) feud_sfx(n);
}

static void wake(FeudBattle *b, int side) {
    if (side == b->atk && b->surprise && !b->woke) { b->woke = 1; sfx("fd_alarm"); }
}

static void kill(FeudBattle *b, int i) {
    FeudSoldier *s = &b->s[i];
    s->alive = 0;
    b->lost[s->side][s->kind]++;
    sfx("fd_die");
}

/* hurt soldier i; from_high: the attacker stands on high ground */
static void hurt_ex(FeudBattle *b, int attacker_kind, int i, bool arrow, bool from_high) {
    FeudSoldier *s = &b->s[i];
    if (!s->alive) return;
    const FeudUnitDef *u = &FEUD_UNIT[attacker_kind];
    int lo = u->dmg_lo[s->kind], hi = u->dmg_hi[s->kind];
    if (lo < 0) return;
    int dmg = rng_range(&b->rng, lo, hi);
    int cell = s->kind == FU_SWIFT ? BC_OPEN : cell_at(b, PX(s->x), PX(s->y));
    bool ranged = attacker_kind == FU_SLINGER || attacker_kind == FU_MYSTIC;
    /* the ground: in cover half the shots miss; up high, shots do half and
     * blows from below one less */
    if (ranged && cell == BC_COVER && rng_chance(&b->rng, 50)) { b->blocked_shots++; return; }
    if (cell == BC_HIGH && !from_high) dmg = ranged ? dmg / 2 : dmg - 1;
    if (s->shield > 0) {
        if (arrow && s->engaged_t <= 0) { b->shield_saves++; sfx("fd_tink"); return; } /* the shield stops arrows outright */
        int a = imin(s->shield, dmg);
        s->shield = (int16_t)(s->shield - a);
        dmg -= a;
        b->still_t = 0;
        b->hits++;
    }
    if (dmg <= 0) return;
    s->hp = (int16_t)(s->hp - dmg);
    s->hit_t = 8;
    b->still_t = 0;
    b->hits++;
    if (s->hp <= 0) kill(b, i);
}

static void hurt(FeudBattle *b, int attacker_kind, int i, bool arrow) { hurt_ex(b, attacker_kind, i, arrow, false); }

static void hurt_bld(FeudBattle *b, int bi, int dmg) {
    if (bi < 0 || bi >= b->nbld || b->bld[bi].hp <= 0) return;
    b->bld[bi].hp -= imax(1, dmg);
    b->still_t = 0;
    if (b->bld[bi].hp <= 0) { b->bld[bi].hp = 0; sfx("fd_wreck"); }
}

static bool cover_between(const FeudBattle *b, int x0, int y0, int x1, int y1, int *sx, int *sy) {
    int steps = imax(iabs(x1 - x0), iabs(y1 - y0)) / 4 + 1;
    int c0 = (y0 / FB_CELL) * FB_GW + x0 / FB_CELL, c1 = (y1 / FB_CELL) * FB_GW + x1 / FB_CELL;
    for (int i = 1; i < steps; i++) {
        int x = x0 + (x1 - x0) * i / steps, y = y0 + (y1 - y0) * i / steps;
        int c = (y / FB_CELL) * FB_GW + x / FB_CELL;
        if (c == c0 || c == c1) continue;
        if (cell_at(b, x, y) == BC_COVER) { *sx = x; *sy = y; return true; }
    }
    return false;
}

static void shoot(FeudBattle *b, FeudSoldier *s, int tgt) {
    int slot = -1;
    for (int i = 0; i < FB_MAX_SHOTS; i++)
        if (!b->sh[i].alive) { slot = i; break; }
    int tx, ty;
    if (tgt >= 0) { tx = PX(b->s[tgt].x); ty = PX(b->s[tgt].y); }
    else { tx = b->bld[-2 - tgt].x; ty = b->bld[-2 - tgt].y; }
    if (slot < 0) return;
    FeudShot *h = &b->sh[slot];
    memset(h, 0, sizeof *h);
    h->alive = 1;
    h->kind = s->kind;
    h->side = s->side;
    h->x = s->x;
    h->y = s->y - 3 * FX;
    h->tx = tx;
    h->ty = ty;
    h->tgt = tgt;
    h->high = cell_at(b, PX(s->x), PX(s->y)) == BC_HIGH;
    int sx, sy;
    /* a thicket or a heap of bones takes most shots that pass through it */
    if (s->kind != FU_SWIFT && cover_between(b, PX(s->x), PX(s->y), tx, ty, &sx, &sy) && rng_chance(&b->rng, 60)) {
        h->blocked = 1;
        h->stop_x = sx;
        h->stop_y = sy;
        b->blocked_shots++;
    }
    int spd = s->kind == FU_SLINGER ? 3 * FX : s->kind == FU_MYSTIC ? 2 * FX : FX + FX / 2;
    int dx = tx * FX - h->x, dy = ty * FX - h->y;
    int len = imax(1, (int)sqrtf((float)dx * dx + (float)dy * dy));
    h->vx = (int)((long long)dx * spd / len);
    h->vy = (int)((long long)dy * spd / len);
    h->t = len / spd + 1;
    sfx(s->kind == FU_SLINGER ? "fd_sling" : s->kind == FU_MYSTIC ? "fd_zap" : "fd_drop");
}

static void land(FeudBattle *b, FeudShot *h) {
    if (h->kind == FU_SWIFT) {
        /* a seed bomb: everyone of theirs near where it lands */
        sfx("fd_bomb");
        for (int i = 0; i < FB_MAX; i++) {
            FeudSoldier *s = &b->s[i];
            if (!s->alive || s->side == h->side) continue;
            if (iabs(PX(s->x) - h->tx) <= 12 && iabs(PX(s->y) - h->ty) <= 12) hurt(b, FU_SWIFT, i, false);
        }
        if (h->tgt <= -2) hurt_bld(b, -2 - h->tgt, rng_range(&b->rng, 4, 6));
        return;
    }
    if (h->tgt >= 0) {
        FeudSoldier *s = &b->s[h->tgt];
        if (s->alive && s->side != h->side) hurt_ex(b, h->kind, h->tgt, h->kind == FU_SLINGER, h->high);
    } else if (h->tgt <= -2) {
        const FeudUnitDef *u = &FEUD_UNIT[h->kind];
        hurt_bld(b, -2 - h->tgt, rng_range(&b->rng, imax(1, u->dmg_lo[0]), imax(1, u->dmg_hi[0])));
    }
}

/* ---- each bird --------------------------------------------------------------- */

static int dist_to(const FeudSoldier *s, int tx, int ty) {
    int dx = PX(s->x) - tx, dy = PX(s->y) - ty;
    return (int)sqrtf((float)(dx * dx + dy * dy));
}

static int pick_target(FeudBattle *b, int i) {
    FeudSoldier *s = &b->s[i];
    const FeudUnitDef *u = &FEUD_UNIT[s->kind];
    int best = -1, bs = 1 << 30;
    int their_edge = is_left(b, 1 - s->side) ? 0 : FB_FW;
    for (int j = 0; j < FB_MAX; j++) {
        FeudSoldier *o = &b->s[j];
        if (!o->alive || o->side == s->side) continue;
        if (u->dmg_lo[o->kind] < 0) continue; /* stompers and lancers can't reach a swift */
        int d = dist_to(s, PX(o->x), PX(o->y));
        int score = d;
        if (s->kind == FU_SLINGER && o->kind == FU_LANCER) score -= 40; /* slingers love a lancer */
        if ((s->kind == FU_SLINGER || s->kind == FU_MYSTIC) && o->kind == FU_SWIFT) score += 30;
        if (s->kind == FU_SWIFT) score = iabs(PX(o->x) - their_edge) + d / 4; /* swifts fly past to the back */
        if (score < bs) { bs = score; best = j; }
    }
    return best;
}

static int pick_bld(FeudBattle *b, const FeudSoldier *s) {
    int best = -1, bd = 1 << 30;
    for (int k = 0; k < b->nbld; k++) {
        if (b->bld[k].hp <= 0) continue;
        int d = dist_to(s, b->bld[k].x, b->bld[k].y);
        if (d < bd) { bd = d; best = k; }
    }
    return best;
}

static bool step_toward(FeudBattle *b, FeudSoldier *s, int tx, int ty, int speed) {
    int px = PX(s->x), py = PX(s->y);
    int gx = tx, gy = ty;
    if (walker(s->kind) && !line_clear(b, px, py, tx, ty)) {
        if (--s->path_t <= 0 || s->wp < 0) {
            s->path_t = 20;
            s->wp = next_cell(b, px / FB_CELL, iclamp(py / FB_CELL, 0, FB_GH - 1), iclamp(tx / FB_CELL, 0, FB_GW - 1),
                              iclamp(ty / FB_CELL, 0, FB_GH - 1));
        }
        if (s->wp >= 0) {
            gx = (s->wp % FB_GW) * FB_CELL + FB_CELL / 2;
            gy = (s->wp / FB_GW) * FB_CELL + FB_CELL / 2;
            if (iabs(gx - px) <= 2 && iabs(gy - py) <= 2) s->path_t = 0;
        }
    } else {
        s->wp = -1;
    }
    int dx = gx * FX - s->x, dy = gy * FX - s->y;
    int len = imax(1, (int)sqrtf((float)dx * dx + (float)dy * dy));
    if (len < FX / 2) return false;
    int vx = (int)((long long)dx * speed / len), vy = (int)((long long)dy * speed / len);
    int nx = s->x + vx, ny = s->y + vy;
    if (walker(s->kind)) {
        if (cell_at(b, PX(nx), PX(ny)) == BC_BLOCK) {
            if (cell_at(b, PX(nx), PX(s->y)) != BC_BLOCK) ny = s->y;
            else if (cell_at(b, PX(s->x), PX(ny)) != BC_BLOCK) nx = s->x;
            else return false;
        }
    }
    s->x = iclamp(nx, 2 * FX, (FB_FW - 2) * FX);
    s->y = iclamp(ny, 4 * FX, (FB_FH - 2) * FX);
    if (vx) s->face = vx > 0 ? 1 : -1;
    s->anim++;
    return true;
}

static bool bird_step(FeudBattle *b, int i) {
    FeudSoldier *s = &b->s[i];
    const FeudUnitDef *u = &FEUD_UNIT[s->kind];
    if (s->hit_t) s->hit_t--;
    if (s->engaged_t) s->engaged_t--;
    if (s->cd > 0) s->cd--;
    if (s->frozen) return false;
    if (s->side == b->def && b->surprise && !b->woke) return false; /* caught napping */
    int order = b->order[s->side];
    if (order == FO_FLEE) {
        int edge = is_left(b, s->side) ? 0 : FB_FW;
        bool moved = step_toward(b, s, edge, PX(s->y), u->speed + 20);
        if ((is_left(b, s->side) && PX(s->x) <= 3) || (!is_left(b, s->side) && PX(s->x) >= FB_FW - 3)) {
            s->alive = 0;
            b->escaped[s->side][s->kind]++;
        }
        return moved;
    }
    /* a target: theirs, or under WRECK a building */
    int tx, ty;
    if (order == FO_WRECK && pick_bld(b, s) >= 0) {
        s->tgt = -2 - pick_bld(b, s);
    } else if (s->tgt < 0 || !b->s[s->tgt].alive || (b->t + i) % 20 == 0) {
        s->tgt = pick_target(b, i);
    }
    if (s->tgt <= -2 && order != FO_WRECK) s->tgt = pick_target(b, i);
    if (s->tgt == -1) return false;
    if (s->tgt >= 0) { tx = PX(b->s[s->tgt].x); ty = PX(b->s[s->tgt].y); }
    else { tx = b->bld[-2 - s->tgt].x; ty = b->bld[-2 - s->tgt].y; }
    int d = dist_to(s, tx, ty);
    int reach = u->reach_hi;
    bool ranged = s->kind == FU_SLINGER || s->kind == FU_MYSTIC;
    if (ranged && cell_at(b, PX(s->x), PX(s->y)) == BC_HIGH) reach += 24; /* high ground: farther */
    bool moved = false;
    if (s->kind == FU_SLINGER && d < u->reach_lo) {
        /* too close to sling: back off, slowly */
        if (order != FO_STAND) {
            int away = is_left(b, s->side) ? 0 : FB_FW;
            moved = step_toward(b, s, away, PX(s->y), u->speed);
        }
        return moved;
    }
    if (d > reach - (ranged ? 6 : 4)) {
        if (order != FO_STAND) moved = step_toward(b, s, tx, ty, u->speed);
        if (d > reach) return moved;
    }
    if (s->cd > 0) return moved;
    /* strike */
    s->cd = u->every;
    s->anim = 0;
    s->hit_t = 0;
    wake(b, s->side);
    if (s->tgt >= 0) {
        s->face = tx > PX(s->x) ? 1 : -1;
        if (s->kind == FU_STOMPER || s->kind == FU_LANCER) {
            b->s[s->tgt].engaged_t = 90;
            s->engaged_t = 90;
            sfx("fd_peck");
            hurt_ex(b, s->kind, s->tgt, false, cell_at(b, PX(s->x), PX(s->y)) == BC_HIGH);
        } else {
            shoot(b, s, s->tgt);
        }
    } else {
        if (s->kind == FU_STOMPER || s->kind == FU_LANCER) {
            sfx("fd_peck");
            hurt_bld(b, -2 - s->tgt, rng_range(&b->rng, imax(1, u->dmg_lo[0]), imax(1, u->dmg_hi[0])));
        } else {
            shoot(b, s, s->tgt);
        }
    }
    return moved;
}

/* birds of a side don't all stand in one spot */
static void spread(FeudBattle *b) {
    for (int i = 0; i < FB_MAX; i++) {
        FeudSoldier *s = &b->s[i];
        if (!s->alive) continue;
        for (int j = i + 1; j < FB_MAX; j++) {
            FeudSoldier *o = &b->s[j];
            if (!o->alive || o->side != s->side || walker(o->kind) != walker(s->kind)) continue;
            int dx = o->x - s->x, dy = o->y - s->y;
            if (iabs(dx) < 4 * FX && iabs(dy) < 4 * FX) {
                int push = dy >= 0 ? 48 : -48;
                if (cell_at(b, PX(o->x), PX(o->y + push)) != BC_BLOCK) o->y = iclamp(o->y + push, 4 * FX, (FB_FH - 2) * FX);
                if (cell_at(b, PX(s->x), PX(s->y - push)) != BC_BLOCK) s->y = iclamp(s->y - push, 4 * FX, (FB_FH - 2) * FX);
            }
        }
    }
}

void feud_battle_ai_order(FeudBattle *b, int side) {
    if (b->result != BR_NONE || b->t < 90) return;
    int mine = feud_battle_power(b, side), theirs = feud_battle_power(b, 1 - side);
    if (side == b->atk) {
        b->order[side] = mine * 100 < theirs * 45 ? FO_FLEE : FO_CHARGE;
        return;
    }
    if (mine * 100 < theirs * 25) { b->order[side] = FO_FLEE; return; }
    /* defenders behind walls, over a chasm or up a slope hold their ground */
    if (b->terr == FT_EYRIE || b->terr == FT_GORGE || b->terr == FT_PEAK || b->terr == FT_SCREE) {
        b->order[side] = FO_STAND;
        return;
    }
    b->order[side] = FO_CHARGE;
}

void feud_battle_step(FeudBattle *b) {
    if (b->result != BR_NONE) { b->end_t++; return; }
    b->t++;
    bool moving = false;
    int hits0 = b->hits;
    /* reserves come on as the field thins */
    if (b->t % 6 == 0)
        for (int side = 0; side < 2; side++) {
            if (b->order[side] == FO_FLEE || on_field(b, side) >= FB_ON_FIELD) continue;
            for (int k = 0; k < FU_KINDS; k++)
                if (b->reserve[side][k] > 0) { b->reserve[side][k]--; spawn(b, side, k, true); break; }
        }
    /* a side that flees takes its waiting birds with it */
    for (int side = 0; side < 2; side++)
        if (b->order[side] == FO_FLEE)
            for (int k = 0; k < FU_KINDS; k++) { b->escaped[side][k] += b->reserve[side][k]; b->reserve[side][k] = 0; }
    for (int i = 0; i < FB_MAX; i++)
        if (b->s[i].alive && bird_step(b, i)) moving = true;
    for (int i = 0; i < FB_MAX_SHOTS; i++) {
        FeudShot *h = &b->sh[i];
        if (!h->alive) continue;
        h->x += h->vx;
        h->y += h->vy;
        if (h->blocked && iabs(PX(h->x) - h->stop_x) < 4 && iabs(PX(h->y) - h->stop_y) < 4) { h->alive = 0; continue; }
        if (--h->t <= 0) { h->alive = 0; land(b, h); }
    }
    if (b->t % 2 == 0) spread(b);
    if (b->hits != hits0) moving = true;
    for (int i = 0; i < FB_MAX_SHOTS; i++)
        if (b->sh[i].alive) moving = true; /* something still in the air */
    b->still_t = moving ? 0 : b->still_t + 1;

    int na = feud_battle_count(b, b->atk), nd = feud_battle_count(b, b->def);
    if (b->order[b->atk] == FO_FLEE && on_field(b, b->atk) == 0) b->result = BR_ATK_BACK;
    else if (nd == 0 && na > 0) b->result = b->order[b->def] == FO_FLEE ? BR_DEF_FLED : BR_ATK_WIN;
    else if (na == 0) b->result = BR_DEF_WIN;
    else if (b->still_t >= FB_STILL_END || b->t >= FB_TIME_MAX) {
        /* nothing happening: the attackers go home */
        b->result = BR_ATK_BACK;
    }
    if (b->result != BR_NONE) {
        /* whoever is still standing walks off with the result */
        for (int i = 0; i < FB_MAX; i++) {
            FeudSoldier *s = &b->s[i];
            if (s->alive && s->side == b->atk && b->result == BR_ATK_BACK) { s->alive = 0; b->escaped[s->side][s->kind]++; }
        }
        if (b->result == BR_ATK_BACK)
            for (int k = 0; k < FU_KINDS; k++) { b->escaped[b->atk][k] += b->reserve[b->atk][k]; b->reserve[b->atk][k] = 0; }
        sfx(b->result == BR_ATK_WIN || b->result == BR_DEF_FLED ? "fd_take" : "fd_hold");
    }
}

/* ---- the result on the board ------------------------------------------------- */

static void survivors(const FeudBattle *b, int side, int out[FU_KINDS]) {
    for (int k = 0; k < FU_KINDS; k++) out[k] = b->reserve[side][k];
    for (int i = 0; i < FB_MAX; i++)
        if (b->s[i].alive && b->s[i].side == side) out[b->s[i].kind]++;
}

void feud_battle_apply(FeudMatch *m, const FeudBattle *b, int got[FR_KINDS]) {
    FeudTile *d = &m->t[b->dst];
    int sa[FU_KINDS], sd[FU_KINDS];
    survivors(b, b->atk, sa);
    survivors(b, b->def, sd);
    if (got) memset(got, 0, sizeof(int) * FR_KINDS);
    /* the buildings left standing */
    int left[FB_KINDS] = {0};
    for (int k = 0; k < b->nbld; k++)
        if (b->bld[k].hp > 0) left[b->bld[k].kind]++;
    for (int k = 0; k < FB_KINDS; k++) d->b[k] = (uint8_t)left[k];
    bool taken = b->result == BR_ATK_WIN || b->result == BR_DEF_FLED;
    /* defenders who ran off: home again if the tile held, else to a tile of theirs next door */
    int ed = 0;
    for (int k = 0; k < FU_KINDS; k++) ed += b->escaped[b->def][k];
    if (taken && ed) {
        int nb[4], n = 0;
        for (int t = 0; t < FD_TILES; t++)
            if (m->t[t].owner == b->def && feud_adjacent(t, b->dst, false)) nb[n++] = t;
        if (n) {
            int t = nb[rng_range(&m->rng, 0, n - 1)];
            for (int k = 0; k < FU_KINDS; k++) m->t[t].u[k] = (uint8_t)imin(FD_STACK_MAX, m->t[t].u[k] + b->escaped[b->def][k]);
        }
    }
    /* attackers who got away go back where they came from */
    if (b->src >= 0 && m->t[b->src].owner == b->atk) {
        FeudTile *s = &m->t[b->src];
        for (int k = 0; k < FU_KINDS; k++) {
            int n = b->escaped[b->atk][k] + (taken ? 0 : sa[k]);
            s->u[k] = (uint8_t)imin(FD_STACK_MAX, s->u[k] + n);
            s->moved[k] = (uint8_t)imin(s->u[k], s->moved[k] + n);
        }
    }
    if (taken) {
        d->owner = (int8_t)b->atk;
        d->frozen = 0;
        d->seen = 0;
        for (int k = 0; k < FU_KINDS; k++) {
            d->u[k] = (uint8_t)imin(FD_STACK_MAX, sa[k]);
            d->moved[k] = d->u[k];
        }
    } else {
        for (int k = 0; k < FU_KINDS; k++) d->u[k] = (uint8_t)imin(FD_STACK_MAX, sd[k] + b->escaped[b->def][k]);
    }
    if (d->owner == b->def) {
        for (int k = 0; k < FU_KINDS; k++) d->moved[k] = (uint8_t)imin(d->moved[k], d->u[k]);
    }
    feud_check_elimination(m);
}
