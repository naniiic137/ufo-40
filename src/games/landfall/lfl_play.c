/* LANDFALL - play: Wick on foot, the eight tools, the creatures, pickups,
 * hazards, room changes (a Zelda-style scroll), the respawn rule (rooms
 * three away come back), the teleporting bog, the gnats that come for a
 * cooked chop, and the ship. */
#include "lfl.h"

#define PW 10      /* Wick's box */
#define PW_SMALL 6
#define WALK 320   /* 1.25 px a frame */
#define INV_HIT 60
#define INV_MORPH 60
#define SHRINK_T 360
#define MORPH_T 360
#define POD_FUSE 90
#define SHARD_DELAY 24
#define GNAT_WAIT 300
#define SCROLL_T 40
#define DROP_HOLD 30

static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};

int lfl_death_cause[8];

void lfl_sfx(const char *name) { sfx_play_name(name); }

void lfl_say(const char *m) {
    snprintf(lg.msg, sizeof lg.msg, "%s", m);
    lg.msg_t = 150;
}

static bool holding(int tool) { return lg.pl.hold[0] == tool || lg.pl.hold[1] == tool; }
static int pw(void) { return lg.pl.shrink_t > 0 ? PW_SMALL : PW; }
static int pcx(void) { return LFL_PX(lg.pl.x) + pw() / 2; }
static int pcy(void) { return LFL_PX(lg.pl.y) + pw() / 2; }

static LflRoom *cur(void) { return &lp.room[lg.cur]; }
static int tile_at(int px, int py) {
    if (px < 0 || py < 0) return T_FLOOR;
    return lfl_tile(lg.cur, px / LFL_TILE, py / LFL_TILE);
}

bool lfl_solid_for_player(int t, bool shrunk) {
    switch (t) {
    case T_FLOOR: case T_WOOD: case T_TECHDOWN: case T_SWAMP: case T_WARP: case T_STAIRS_DN: case T_STAIRS_UP: return false;
    case T_HOLE: case T_LOG: return !shrunk;
    case T_SECRET: return !(shrunk && lp.room[lg.cur].secret_key == SK_SMALL);
    default: return true;
    }
}

static bool mob_solid_tile(int t) {
    return !(t == T_FLOOR || t == T_WOOD || t == T_TECHDOWN || t == T_SWAMP || t == T_WARP || t == T_STAIRS_DN || t == T_STAIRS_UP);
}

/* off the room's edge counts as open ground (the scroll takes over) */
bool lfl_tile_solid_at(int px, int py, bool shrunk) {
    if (px < 0 || py < 0 || px >= LFL_ROOM_PW || py >= LFL_ROOM_PH) {
        int d = py < 0 ? DIR_UP : py >= LFL_ROOM_PH ? DIR_DOWN : px < 0 ? DIR_LEFT : DIR_RIGHT;
        return lfl_neighbor(lg.cur, d) < 0;
    }
    return lfl_solid_for_player(tile_at(px, py), shrunk);
}

static bool box_hits_tiles(int x, int y, int w, int h, bool shrunk) {
    for (int yy = y; yy < y + h; yy += 4)
        for (int xx = x; xx < x + w; xx += 4) {
            int ax = imin(xx, x + w - 1), ay = imin(yy, y + h - 1);
            if (lfl_tile_solid_at(ax, ay, shrunk)) return true;
        }
    if (lfl_tile_solid_at(x + w - 1, y + h - 1, shrunk)) return true;
    if (lfl_tile_solid_at(x + w - 1, y, shrunk)) return true;
    if (lfl_tile_solid_at(x, y + h - 1, shrunk)) return true;
    return false;
}

static bool mob_blocks_player(const LflMob *m, int x, int y, int w) {
    if (!m->alive || m->sp != SP_SNAIL) return false;
    if (!rects_overlap(x, y, w, w, LFL_PX(m->x), LFL_PX(m->y), m->w, m->h)) return false;
    /* a shell lying along the way lets a small Wick through */
    if ((m->flags & MF_SHELL) && lg.pl.shrink_t > 0) {
        int gate = m->gate;
        if (gate >= 0) {
            int side = lp.gate[gate].side;
            return (m->face & 1) != (side & 1);
        }
        return false;
    }
    return true;
}

static bool player_box_free(int x, int y, int w) {
    if (box_hits_tiles(x, y, w, w, lg.pl.shrink_t > 0)) return false;
    for (int i = 0; i < LFL_MAX_MOBS; i++)
        if (mob_blocks_player(&lg.mob[i], x, y, w)) return false;
    return true;
}

/* ------------------------------------------------------------------------ */

void lfl_hurt_player(int dmg, int why) {
    LflPlayer *p = &lg.pl;
    if (dmg <= 0 || p->inv > 0 || lg.state != LS_PLAY || p->hp <= 0) return;
    lg.last_hurt = why;
    lg.hits_taken++;
    if (lg.god) { p->inv = INV_HIT; return; }
    p->hp -= dmg;
    p->inv = INV_HIT;
    p->hurt_flash = 12;
    lfl_sfx("lfl_hurt");
    if (p->hp <= 0) {
        p->hp = 0;
        lfl_death_cause[iclamp(why, 0, 7)]++;
        lg.state = LS_DYING;
        lg.state_t = 0;
        lfl_sfx("lfl_die");
    }
}

void lfl_heal(int n) { lg.pl.hp = imin(lg.pl.hpmax, lg.pl.hp + n); }

int lfl_add_res(int r, int n) {
    int before = lg.res[r];
    lg.res[r] = imin(LFL_RES_CAP, lg.res[r] + n);
    return lg.res[r] - before;
}

static void dirty(void) { cur()->dirty = 1; }

/* ------------------------------------------------------------------------ */
/* drops (room-only)                                                        */

static int add_drop(int kind, int sub, int x, int y, int spawn) {
    for (int i = 0; i < LFL_MAX_DROPS; i++) {
        LflDrop *d = &lg.drop[i];
        if (d->alive) continue;
        memset(d, 0, sizeof *d);
        d->alive = 1;
        d->kind = (uint8_t)kind;
        d->sub = (uint8_t)sub;
        d->x = iclamp(x, 0, LFL_ROOM_PW - 8) * LFL_FX;
        d->y = iclamp(y, 0, LFL_ROOM_PH - 8) * LFL_FX;
        d->spawn = (int16_t)spawn;
        if (kind == DR_CHOP) {
            int t = tile_at(x + 4, y + 4);
            d->cooked = t == T_SWAMP || t == T_WARP;
        }
        return i;
    }
    return -1;
}

static void take_drop(int i) {
    LflDrop *d = &lg.drop[i];
    if (d->kind == DR_RES) {
        lfl_add_res(d->sub, 1);
        lfl_sfx("lfl_res");
    } else {
        lfl_heal(3);
        lfl_sfx("lfl_eat");
    }
    if (d->spawn >= 0) { cur()->spawn[d->spawn].taken = 1; dirty(); }
    d->alive = 0;
}

/* ------------------------------------------------------------------------ */
/* things (persistent)                                                      */

void lfl_give_tool_drop(int tool, int x, int y) {
    for (int i = 0; i < LFL_MAX_THINGS; i++) {
        LflThing *t = &lp.thing[i];
        /* reuse a tool's old slot, never a cell's or a chart piece's */
        if (i < lp.nthing && (t->alive || t->kind != TH_TOOL)) continue;
        memset(t, 0, sizeof *t);
        t->alive = 1;
        t->kind = TH_TOOL;
        t->sub = (uint8_t)tool;
        t->n = 1;
        t->room = (uint8_t)lg.cur;
        t->x = (int16_t)iclamp(x, 0, LFL_ROOM_PW - 12);
        t->y = (int16_t)iclamp(y, 0, LFL_ROOM_PH - 12);
        t->cd = 60;
        if (i >= lp.nthing) lp.nthing = i + 1;
        return;
    }
}

void lfl_take_map(int from) {
    lg.maps++;
    lg.map_from |= (uint8_t)from;
    lfl_sfx("lfl_chart");
    music_play(LFL_MUS_CHART);
    if (!lsv.found_map) lsv.found_map = 1;
    game_award(GOAL_BEACON);
}

static void take_thing(int i, bool by_disc) {
    LflThing *t = &lp.thing[i];
    LflPlayer *p = &lg.pl;
    for (int g = 0; g < lp.ngate; g++)
        if (lp.gate[g].prize_thing == i) lp.gate[g].used = 1; /* the prize is out */
    switch (t->kind) {
    case TH_TOOL: {
        int tool = t->sub;
        int slot = p->hold[p->act] < 0 ? p->act : p->hold[1 - p->act] < 0 ? 1 - p->act : -1;
        if (slot >= 0) {
            p->hold[slot] = (int8_t)tool;
            t->alive = 0;
        } else {
            /* hands full: the active tool goes down where Wick stands */
            int old = p->hold[p->act];
            p->hold[p->act] = (int8_t)tool;
            t->alive = 0;
            lfl_give_tool_drop(old, LFL_PX(p->x) + pw() / 2 - 6, LFL_PX(p->y) + pw() / 2 - 6);
        }
        if (!(lg.tools_found & (1 << tool))) {
            lg.tools_found |= (uint8_t)(1 << tool);
            lg.items_found++;
            if (lg.items_found > lsv.most_items) lsv.most_items = (uint8_t)lg.items_found;
        }
        (void)by_disc;
        lfl_sfx("lfl_tool");
        break;
    }
    case TH_CUBE:
        p->hpmax = imin(LFL_HP_MAX, p->hpmax + 1);
        p->hp = p->hpmax;
        lg.cubes_found++;
        if (lg.cubes_found > lsv.most_energy) lsv.most_energy = (uint8_t)lg.cubes_found;
        t->alive = 0;
        lfl_sfx("lfl_cube");
        break;
    case TH_MAP:
        t->alive = 0;
        lfl_take_map(lp.room[t->room].kind == RK_DUN ? 4 : 1);
        break;
    case TH_BUNDLE:
        lfl_add_res(t->sub, t->n);
        t->alive = 0;
        lfl_sfx("lfl_bundle");
        break;
    default: break;
    }
}

/* ------------------------------------------------------------------------ */
/* creatures                                                                */

int lfl_spawn_mob(int sp, int x, int y, int spawn) {
    for (int i = 0; i < LFL_MAX_MOBS; i++) {
        LflMob *m = &lg.mob[i];
        if (m->alive) continue;
        memset(m, 0, sizeof *m);
        m->alive = 1;
        m->sp = (uint8_t)sp;
        m->w = m->h = sp == SP_SNAIL ? 28 : sp == SP_GNAT ? 8 : 12;
        m->x = x * LFL_FX;
        m->y = y * LFL_FX;
        m->hp = lp.sp[sp].hp;
        m->spawn = (int16_t)spawn;
        m->gate = -1;
        m->face = DIR_DOWN;
        m->dir = -1;
        m->line = -1;
        m->fire_t = 60 + lp.sp[sp].fire_every * 4;
        if (lp.sp[sp].friendly) m->flags |= MF_FRIENDLY;
        return i;
    }
    return -1;
}

int lfl_count_mobs(int any, int none) {
    int n = 0;
    for (int i = 0; i < LFL_MAX_MOBS; i++) {
        const LflMob *m = &lg.mob[i];
        if (!m->alive) continue;
        if (any && !(m->flags & any)) continue;
        if (m->flags & none) continue;
        n++;
    }
    return n;
}

static void add_foe_shot(int kind, int x, int y, int vx, int vy, int dmg) {
    for (int i = 0; i < LFL_MAX_SHOTS; i++) {
        LflShot *s = &lg.shot[i];
        if (s->alive) continue;
        memset(s, 0, sizeof *s);
        s->alive = 1;
        s->kind = (uint8_t)kind;
        s->x = x * LFL_FX;
        s->y = y * LFL_FX;
        s->vx = vx;
        s->vy = vy;
        s->life = kind == SH_SHARD ? 22 : 240;
        s->dmg = dmg;
        s->carry_drop = s->carry_thing = -1;
        return;
    }
}

static void kill_mob(int i, bool by_pulse);

static void leave_swamp(int cx, int cy) {
    int tx = cx / LFL_TILE, ty = cy / LFL_TILE;
    for (int d = -1; d < 4; d++) {
        int x = tx + (d < 0 ? 0 : DX[d]), y = ty + (d < 0 ? 0 : DY[d]);
        if (x < 1 || y < 1 || x >= LFL_RW - 1 || y >= LFL_RH - 1) continue;
        uint8_t *t = &cur()->t[y][x];
        if (*t == T_FLOOR || *t == T_WOOD) *t = T_SWAMP;
    }
}

static void kill_mob(int i, bool by_pulse) {
    LflMob *m = &lg.mob[i];
    if (!m->alive) return;
    const LflSpecies *s = &lp.sp[m->sp];
    int cx = LFL_PX(m->x) + m->w / 2, cy = LFL_PX(m->y) + m->h / 2;
    if (m->sp == SP_SNAIL) {
        if (!by_pulse) return;
        /* the thumper leaves an empty shell */
        m->flags |= MF_SHELL;
        if (m->gate >= 0) lp.gate[m->gate].snail = 2;
        lfl_sfx("lfl_crunch");
        return;
    }
    m->alive = 0;
    lg.kills++;
    if (m->spawn >= 0) { cur()->spawn[m->spawn].taken = 1; dirty(); }
    lfl_sfx("lfl_pop");
    if (m->flags & MF_MORPHED) {
        /* a hatchling always drops the second-rarest resource */
        add_drop(DR_RES, lp.res_of_tier[TIER_MORE], cx - 4, cy - 4, -1);
        return;
    }
    if (m->sp == SP_GNAT) {
        add_drop(DR_RES, lp.res_of_tier[TIER_RARE], cx - 8, cy - 4, -1);
        add_drop(DR_RES, lp.res_of_tier[TIER_RARE], cx, cy - 4, -1);
        return;
    }
    if (s->friendly) {
        /* only the locals carry the rare one; never a chop */
        if (rng_chance(&g_rng, 50)) add_drop(DR_RES, lp.res_of_tier[TIER_RARE], cx - 4, cy - 4, -1);
        return;
    }
    if (s->death == DE_SWAMP) leave_swamp(cx, cy);
    if (s->drop >= 0) add_drop(DR_RES, s->drop, cx - 4, cy - 4, -1);
    else add_drop(DR_CHOP, 0, cx - 4, cy - 4, -1);
    if (s->death == DE_EXPLODE) lfl_explode(cx, cy, 24, 3, 4, true);
}

static void hurt_mob(int i, int dmg, int how) {
    LflMob *m = &lg.mob[i];
    if (!m->alive || m->hurt_t > 0) return;
    if (m->sp == SP_SNAIL) { lfl_sfx("lfl_tink"); return; }
    if (m->flags & MF_MORPHED) {
        if (how == SH_HATCH) return;
        kill_mob(i, false);
        return;
    }
    m->hurt_t = 10;
    if (m->flags & MF_FRIENDLY) m->flags |= MF_ANGRY; /* hurt a local and it turns on you */
    m->hp -= dmg;
    lfl_sfx("lfl_hit");
    if (m->hp > 0) return;
    if (how == SH_HATCH && m->sp != SP_GNAT) {
        /* the last hit from the hatcher: a harmless hatchling for a while */
        m->flags |= MF_MORPHED;
        m->flags &= (uint8_t)~MF_ANGRY;
        m->hp = lp.sp[m->sp].hp;
        m->morph_t = MORPH_T;
        int cx = LFL_PX(m->x) + m->w / 2, cy = LFL_PX(m->y) + m->h / 2;
        m->w = m->h = 8;
        m->x = (cx - 4) * LFL_FX;
        m->y = (cy - 4) * LFL_FX;
        lfl_sfx("lfl_morph");
        return;
    }
    kill_mob(i, false);
}

static bool mob_box_free(const LflMob *m, int32_t nx, int32_t ny) {
    int x = LFL_PX(nx), y = LFL_PX(ny);
    if (x < 0 || y < 0 || x + m->w > LFL_ROOM_PW || y + m->h > LFL_ROOM_PH) return false;
    for (int yy = y; yy < y + m->h + 3; yy += 4)
        for (int xx = x; xx < x + m->w + 3; xx += 4)
            if (mob_solid_tile(tile_at(imin(xx, x + m->w - 1), imin(yy, y + m->h - 1)))) return false;
    return true;
}

static bool mob_move(LflMob *m, int vx, int vy) {
    bool moved = false;
    if (vx && mob_box_free(m, m->x + vx, m->y)) { m->x += vx; moved = true; }
    if (vy && mob_box_free(m, m->x, m->y + vy)) { m->y += vy; moved = true; }
    return moved;
}

static int nearest_chop(int cx, int cy, bool cooked_only) {
    int best = -1, bd = 1 << 30;
    for (int i = 0; i < LFL_MAX_DROPS; i++) {
        const LflDrop *d = &lg.drop[i];
        if (!d->alive || d->kind != DR_CHOP || (cooked_only && !d->cooked)) continue;
        int dd = iabs(LFL_PX(d->x) + 4 - cx) + iabs(LFL_PX(d->y) + 4 - cy);
        if (dd < bd) { bd = dd; best = i; }
    }
    return best;
}

static int dir_toward(int dx, int dy) {
    if (iabs(dx) >= iabs(dy)) return dx >= 0 ? DIR_RIGHT : DIR_LEFT;
    return dy >= 0 ? DIR_DOWN : DIR_UP;
}

static void mob_update(int i) {
    LflMob *m = &lg.mob[i];
    const LflSpecies *s = &lp.sp[m->sp];
    LflPlayer *p = &lg.pl;
    m->t++;
    if (m->hurt_t > 0) m->hurt_t--;
    int cx = LFL_PX(m->x) + m->w / 2, cy = LFL_PX(m->y) + m->h / 2;
    int px = pcx(), py = pcy();
    if (m->flags & MF_SHELL) return;
    if (m->flags & MF_MORPHED) {
        if (--m->morph_t <= 0) {
            /* grows back, whole again */
            m->flags &= (uint8_t)~MF_MORPHED;
            m->hp = s->hp;
            int w = m->sp == SP_SNAIL ? 28 : 12;
            m->x = (cx - w / 2) * LFL_FX;
            m->y = (cy - w / 2) * LFL_FX;
            m->w = m->h = w;
            if (!mob_box_free(m, m->x, m->y)) { m->x = (cx - 4) * LFL_FX; m->y = (cy - 4) * LFL_FX; m->w = m->h = 8; m->flags |= MF_MORPHED; m->morph_t = 30; }
            return;
        }
        if (m->t % 50 == 0 || m->dir < 0) m->dir = rng_range(&g_rng, 0, 3);
        if (!mob_move(m, DX[m->dir] * 64, DY[m->dir] * 64)) m->dir = -1;
        /* bog boots stomp a hatchling */
        if (holding(TL_BOOTS) && rects_overlap(LFL_PX(p->x), LFL_PX(p->y), pw(), pw(), LFL_PX(m->x), LFL_PX(m->y), m->w, m->h))
            kill_mob(i, false);
        return;
    }
    if (m->sp == SP_SNAIL) {
        m->face = dir_toward(px - cx, py - cy);
        int c = nearest_chop(cx, cy, false);
        if (c >= 0) {
            const LflDrop *d = &lg.drop[c];
            int tx = LFL_PX(d->x) + 4, ty = LFL_PX(d->y) + 4;
            if (iabs(tx - cx) <= 6 && iabs(ty - cy) <= 6) {
                lg.drop[c].alive = 0; /* eaten */
                lfl_sfx("lfl_eat");
            } else {
                int vx = isign(tx - cx) * s->speed, vy = isign(ty - cy) * s->speed;
                if (iabs(tx - cx) <= 2) vx = 0;
                if (iabs(ty - cy) <= 2) vy = 0;
                m->face = dir_toward(tx - cx, ty - cy);
                mob_move(m, vx, vy);
                m->flags |= MF_LURED;
            }
        }
        if (m->gate >= 0) {
            LflGate *g = &lp.gate[m->gate];
            g->sx = m->x;
            g->sy = m->y;
            g->snail_face = (uint8_t)m->face;
            int gx = g->tx * LFL_TILE, gy = g->ty * LFL_TILE;
            if (!rects_overlap(LFL_PX(m->x), LFL_PX(m->y), m->w, m->h, gx, gy, g->w * LFL_TILE, g->h * LFL_TILE) && g->snail == 0) {
                g->snail = 1; /* lured off the way for good */
                lg.gate_opened++;
            }
        }
        return;
    }
    if (m->sp == SP_GNAT) {
        int c = nearest_chop(cx, cy, true);
        int tx = px, ty = py;
        if (c >= 0) { tx = LFL_PX(lg.drop[c].x) + 4; ty = LFL_PX(lg.drop[c].y) + 4; }
        int dx = tx - cx, dy = ty - cy;
        int len = imax(1, iabs(dx) + iabs(dy));
        m->x += dx * s->speed / len + (int)(((m->t / 6) & 1) ? 40 : -40);
        m->y += dy * s->speed / len;
        m->face = dir_toward(dx, dy);
        if (c >= 0 && iabs(dx) < 5 && iabs(dy) < 5) { lg.drop[c].alive = 0; lfl_sfx("lfl_eat"); }
        return;
    }
    bool friendly = (m->flags & MF_FRIENDLY) && !(m->flags & MF_ANGRY);
    if (friendly) {
        m->face = DIR_DOWN; /* locals always face down */
        m->talk_t = iabs(px - cx) + iabs(py - cy) < 40 ? m->talk_t + 1 : 0;
        return;
    }
    m->face = dir_toward(px - cx, py - cy); /* hostiles face Wick */
    int move = (m->flags & MF_ANGRY) ? MV_CHASE : s->move;
    int tx = px, ty = py;
    int c = move != MV_STILL ? nearest_chop(cx, cy, false) : -1;
    if (c >= 0) {
        /* a chop draws any creature */
        tx = LFL_PX(lg.drop[c].x) + 4;
        ty = LFL_PX(lg.drop[c].y) + 4;
        move = MV_CHASE;
        if (iabs(tx - cx) < 6 && iabs(ty - cy) < 6) { lg.drop[c].alive = 0; lfl_sfx("lfl_eat"); }
    }
    int sp = s->speed;
    if (move == MV_WANDER) {
        if (m->dir < 0 || m->t % 90 == 0) m->dir = rng_range(&g_rng, 0, 3);
        if (!mob_move(m, DX[m->dir] * sp, DY[m->dir] * sp)) m->dir = -1;
    } else if (move == MV_CHASE) {
        int d = dir_toward(tx - cx, ty - cy);
        if (iabs(tx - cx) + iabs(ty - cy) > 2 && !mob_move(m, DX[d] * sp, DY[d] * sp)) {
            /* round the corner along the other axis */
            int d2 = (iabs(tx - cx) >= iabs(ty - cy)) ? (ty >= cy ? DIR_DOWN : DIR_UP) : (tx >= cx ? DIR_RIGHT : DIR_LEFT);
            mob_move(m, DX[d2] * sp, DY[d2] * sp);
        }
    }
    /* shots: always at Wick */
    if (s->attack != AT_NONE && !(m->flags & MF_FRIENDLY)) {
        if (--m->fire_t <= 0) {
            m->fire_t = s->fire_every * 4;
            int dx = px - cx, dy = py - cy;
            int v = 256; /* a little slower than her walk */
            if (s->attack == AT_FOUR) {
                int d = dir_toward(dx, dy);
                add_foe_shot(SH_FOE, cx - 3, cy - 3, DX[d] * v, DY[d] * v, s->dmg);
            } else {
                int len = imax(1, (int)sqrtf((float)(dx * dx + dy * dy)));
                add_foe_shot(SH_FOE, cx - 3, cy - 3, dx * v / len, dy * v / len, s->dmg);
            }
            lfl_sfx("lfl_spit");
        }
    }
}

/* ------------------------------------------------------------------------ */
/* blasts                                                                   */

static void make_warp(int room, int tx, int ty) {
    /* the old pair turns back into plain bog */
    for (int k = 0; k < 2; k++)
        if (lp.warp_room[k] >= 0 && lfl_tile(lp.warp_room[k], lp.warp_x[k], lp.warp_y[k]) == T_WARP)
            lp.room[lp.warp_room[k]].t[(int)lp.warp_y[k]][(int)lp.warp_x[k]] = T_SWAMP;
    lp.warp_room[0] = lp.warp_room[1] = -1;
    /* a partner: any other bog tile on the planet */
    int n = 0;
    for (int r = 0; r < LFL_MAX_ROOMS; r++) {
        if (!lp.room[r].exists) continue;
        for (int y = 0; y < LFL_RH; y++)
            for (int x = 0; x < LFL_RW; x++)
                if (lp.room[r].t[y][x] == T_SWAMP && !(r == room && iabs(x - tx) + iabs(y - ty) < 3)) n++;
    }
    if (!n) return;
    int pick = rng_range(&g_rng, 0, n - 1);
    for (int r = 0; r < LFL_MAX_ROOMS; r++) {
        if (!lp.room[r].exists) continue;
        for (int y = 0; y < LFL_RH; y++)
            for (int x = 0; x < LFL_RW; x++)
                if (lp.room[r].t[y][x] == T_SWAMP && !(r == room && iabs(x - tx) + iabs(y - ty) < 3) && pick-- == 0) {
                    lp.room[room].t[ty][tx] = T_WARP;
                    lp.room[r].t[y][x] = T_WARP;
                    lp.warp_room[0] = (int8_t)room; lp.warp_x[0] = (int8_t)tx; lp.warp_y[0] = (int8_t)ty;
                    lp.warp_room[1] = (int8_t)r; lp.warp_x[1] = (int8_t)x; lp.warp_y[1] = (int8_t)y;
                    return;
                }
    }
}

static void add_blast(int kind, int x, int y, int r, int t) {
    for (int i = 0; i < LFL_MAX_BLASTS; i++)
        if (!lg.blast[i].alive) {
            LflBlast *b = &lg.blast[i];
            b->alive = 1;
            b->kind = (uint8_t)kind;
            b->x = x;
            b->y = y;
            b->r = r;
            b->t = t;
            return;
        }
}

void lfl_open_secret(void) {
    LflRoom *r = cur();
    if (r->secret_x < 0 || r->t[(int)r->secret_y][(int)r->secret_x] != T_SECRET) return;
    r->t[(int)r->secret_y][(int)r->secret_x] = T_FLOOR;
    lfl_sfx("lfl_secret");
}

void lfl_explode(int x, int y, int rad, int dmg_player, int dmg_mob, bool from_creature) {
    (void)from_creature;
    add_blast(0, x, y, rad, 20);
    lfl_sfx("lfl_boom");
    LflRoom *r = cur();
    int tx0 = x / LFL_TILE, ty0 = y / LFL_TILE;
    bool warped = false;
    for (int ty = ty0 - 1; ty <= ty0 + 1; ty++)
        for (int tx = tx0 - 1; tx <= tx0 + 1; tx++) {
            if (tx < 0 || ty < 0 || tx >= LFL_RW || ty >= LFL_RH) continue;
            uint8_t *t = &r->t[ty][tx];
            switch (*t) {
            case T_FOLIAGE: case T_FLESH: *t = T_FLOOR; break;
            case T_CRYSTAL:
                *t = T_FLOOR;
                /* a blown crystal throws shards a moment later */
                add_blast(1, tx * LFL_TILE + 8, ty * LFL_TILE + 8, 0, SHARD_DELAY);
                break;
            case T_TECH:
                if (r->area == AREA_DUN) *t = T_TECHDOWN; /* blue walls in the dungeon give to a blast */
                break;
            case T_SWAMP:
                if (!warped) { make_warp(lg.cur, tx, ty); warped = true; }
                break;
            case T_SECRET:
                if (r->secret_key == SK_PODS) lfl_open_secret();
                break;
            default: break;
            }
        }
    /* energy cells caught in it are lost for good */
    for (int i = 0; i < lp.nthing; i++) {
        LflThing *t = &lp.thing[i];
        if (t->alive && t->kind == TH_CUBE && t->room == lg.cur && iabs(t->x + 6 - x) < rad + 6 && iabs(t->y + 6 - y) < rad + 6) {
            t->alive = 0;
            lfl_sfx("lfl_crunch");
        }
    }
    for (int i = 0; i < LFL_MAX_MOBS; i++) {
        LflMob *m = &lg.mob[i];
        if (!m->alive) continue;
        int cx = LFL_PX(m->x) + m->w / 2, cy = LFL_PX(m->y) + m->h / 2;
        if (iabs(cx - x) < rad + m->w / 2 && iabs(cy - y) < rad + m->h / 2) {
            m->hurt_t = 0;
            hurt_mob(i, dmg_mob, 0);
        }
    }
    if (iabs(pcx() - x) < rad + 3 && iabs(pcy() - y) < rad + 3) lfl_hurt_player(dmg_player, 2);
}

/* ------------------------------------------------------------------------ */
/* rooms                                                                    */

void lfl_room_reset(int room) {
    LflRoom *r = &lp.room[room];
    for (int i = 0; i < r->nspawn; i++) r->spawn[i].taken = 0;
    r->dirty = 0;
    /* a snail passed as a shell comes back */
    for (int g = 0; g < lp.ngate; g++)
        if (lp.gate[g].snail_room == room && lp.gate[g].kind == GK_SNAIL && lp.gate[g].snail == 2) {
            lp.gate[g].snail = 0;
            lp.gate[g].sx = (lp.gate[g].tx * LFL_TILE + 2) * LFL_FX;
            lp.gate[g].sy = (lp.gate[g].ty * LFL_TILE + 2) * LFL_FX;
        }
}

static void leave_room(void) {
    /* morphed crystals turn back */
    for (int i = 0; i < LFL_MAX_FLESH; i++)
        if (lg.flesh[i].alive) {
            if (cur()->t[lg.flesh[i].ty][lg.flesh[i].tx] == T_FLESH) cur()->t[lg.flesh[i].ty][lg.flesh[i].tx] = T_CRYSTAL;
            lg.flesh[i].alive = 0;
        }
}

void lfl_enter_room(int room, bool reset_far) {
    if (lg.cur >= 0 && lg.cur < LFL_MAX_ROOMS) leave_room();
    int old = lg.cur;
    lg.cur = room;
    LflRoom *r = &lp.room[room];
    r->seen = 1;
    bool area_change = old >= 0 && old < LFL_MAX_ROOMS && lp.room[old].area != r->area;
    if (area_change) {
        /* going in or out of the dungeon resets everything */
        for (int i = 0; i < LFL_MAX_ROOMS; i++)
            if (lp.room[i].exists) lfl_room_reset(i);
    } else if (reset_far) {
        for (int i = 0; i < LFL_MAX_ROOMS; i++) {
            LflRoom *q = &lp.room[i];
            if (!q->exists || q->area != r->area || !q->dirty) continue;
            if (iabs(q->gx - r->gx) + iabs(q->gy - r->gy) >= 3) lfl_room_reset(i);
        }
    }
    /* a disc carrying something is delivered at once */
    for (int i = 0; i < LFL_MAX_SHOTS; i++) {
        LflShot *s = &lg.shot[i];
        if (s->alive && s->kind == SH_DISC && s->carry_thing >= 0) {
            LflThing *t = &lp.thing[s->carry_thing];
            if (t->alive) take_thing(s->carry_thing, true);
        }
        if (s->alive && s->kind == SH_DISC && s->carry_drop >= 0) {
            LflDrop *d = &lg.drop[s->carry_drop];
            if (d->alive) {
                if (d->kind == DR_RES) lfl_add_res(d->sub, 1);
                else lfl_heal(3);
            }
        }
    }
    memset(lg.mob, 0, sizeof lg.mob);
    memset(lg.shot, 0, sizeof lg.shot);
    memset(lg.drop, 0, sizeof lg.drop);
    memset(lg.bomb, 0, sizeof lg.bomb);
    memset(lg.blast, 0, sizeof lg.blast);
    memset(lg.flesh, 0, sizeof lg.flesh);
    int px = pcx(), py = pcy();
    for (int i = 0; i < r->nspawn; i++) {
        LflSpawn *s = &r->spawn[i];
        if (s->taken) continue;
        int x = s->tx * LFL_TILE, y = s->ty * LFL_TILE;
        if (s->type == SPW_RES) {
            add_drop(DR_RES, s->sub, x + 4, y + 4, i);
            continue;
        }
        if (lg.no_mobs) continue;
        int sp = s->sub & 0x7F;
        int w = s->type == SPW_SNAIL ? 28 : 12;
        /* walking in on top of a creature's spot: it isn't there */
        if (rects_overlap(px - 6, py - 6, 12, 12, x + 2, y + 2, w, w)) continue;
        int m = lfl_spawn_mob(sp, x + 2, y + 2, i);
        if (m >= 0 && (s->sub & 0x80)) lg.mob[m].flags |= MF_GIVER;
        if (m >= 0 && room == lp.mute_room && !(s->sub & 0x80) && lp.sp[sp].glyph) {
            /* the first villager here speaks a line no talk box can read */
            bool first = true;
            for (int j = 0; j < i; j++) first &= !(r->spawn[j].type == SPW_MOB && !(r->spawn[j].sub & 0x80));
            if (first) lg.mob[m].flags |= MF_MUTE;
        }
        if (m >= 0) lg.mob[m].line = (int16_t)(i + room * 3);
    }
    for (int g = 0; g < lp.ngate; g++) {
        LflGate *gt = &lp.gate[g];
        if (gt->snail_room != room || gt->kind != GK_SNAIL) continue;
        int m = lfl_spawn_mob(SP_SNAIL, 0, 0, -1);
        if (m < 0) continue;
        lg.mob[m].x = gt->sx;
        lg.mob[m].y = gt->sy;
        lg.mob[m].gate = (int16_t)g;
        lg.mob[m].flags |= MF_GATE;
        lg.mob[m].face = gt->snail_face;
        if (gt->snail == 2) lg.mob[m].flags |= MF_SHELL;
    }
}

/* ------------------------------------------------------------------------ */
/* the tools                                                                */

static LflShot *new_shot(int kind) {
    for (int i = 0; i < LFL_MAX_SHOTS; i++)
        if (!lg.shot[i].alive) {
            LflShot *s = &lg.shot[i];
            memset(s, 0, sizeof *s);
            s->alive = 1;
            s->kind = (uint8_t)kind;
            s->carry_drop = s->carry_thing = -1;
            return s;
        }
    return NULL;
}

static int count_shots(int kind) {
    int n = 0;
    for (int i = 0; i < LFL_MAX_SHOTS; i++) n += lg.shot[i].alive && lg.shot[i].kind == kind;
    return n;
}

static bool pay(int tool) {
    int r = lp.cost_res[tool];
    if (r >= RS_COUNT) return true;
    if (lg.res[r] < 1) {
        /* not enough: a buzz and the resource flashes */
        lg.res_flash[r] = 60;
        lfl_sfx("lfl_nope");
        return false;
    }
    lg.res[r]--;
    return true;
}

static void front_point(int dist, int *x, int *y) {
    *x = pcx() + DX[lg.pl.face] * dist;
    *y = pcy() + DY[lg.pl.face] * dist;
}

static void use_tool(bool fresh) {
    LflPlayer *p = &lg.pl;
    int tool = p->hold[p->act];
    if (tool < 0 || p->shrink_t > 0) return;
    int cx = pcx(), cy = pcy();
    switch (tool) {
    case TL_RIVET:
        if (p->fire_cd > 0 || count_shots(SH_RIVET) >= 3) return;
        {
            LflShot *s = new_shot(SH_RIVET);
            if (!s) return;
            s->x = (cx - 2) * LFL_FX;
            s->y = (cy - 2) * LFL_FX;
            s->vx = DX[p->face] * 1024;
            s->vy = DY[p->face] * 1024;
            s->life = 18;
            s->dmg = 2;
            p->fire_cd = 8;
            lfl_sfx("lfl_rivet");
        }
        break;
    case TL_DISC:
        if (!fresh || count_shots(SH_DISC)) return;
        {
            LflShot *s = new_shot(SH_DISC);
            if (!s) return;
            s->x = (cx - 4) * LFL_FX;
            s->y = (cy - 4) * LFL_FX;
            s->vx = DX[p->face] * 896;
            s->vy = DY[p->face] * 896;
            s->life = 144 * LFL_FX / 896; /* out to half the screen */
            s->dmg = 2;
            lfl_sfx("lfl_disc");
        }
        break;
    case TL_HATCHER:
        if (p->fire_cd > 0 || count_shots(SH_HATCH) >= 2) return;
        {
            LflShot *s = new_shot(SH_HATCH);
            if (!s) return;
            s->x = (cx - 3) * LFL_FX;
            s->y = (cy - 3) * LFL_FX;
            s->vx = DX[p->face] * 640;
            s->vy = DY[p->face] * 640;
            s->life = 150;
            s->dmg = 1;
            p->fire_cd = 30;
            lfl_sfx("lfl_hatch");
        }
        break;
    case TL_PODS: {
        if (!fresh) return;
        int n = 0;
        for (int i = 0; i < LFL_MAX_BOMBS; i++) n += lg.bomb[i].alive;
        if (n >= 2) return;
        if (!pay(TL_PODS)) return;
        for (int i = 0; i < LFL_MAX_BOMBS; i++)
            if (!lg.bomb[i].alive) {
                lg.bomb[i].alive = 1;
                lg.bomb[i].x = (cx / LFL_TILE * LFL_TILE + 8) * LFL_FX;
                lg.bomb[i].y = (cy / LFL_TILE * LFL_TILE + 8) * LFL_FX;
                lg.bomb[i].t = POD_FUSE;
                break;
            }
        lfl_sfx("lfl_pod");
        break;
    }
    case TL_THUMPER:
        if (!fresh || lg.pulse_flash > 0) return;
        if (!pay(TL_THUMPER)) return;
        lg.pulse_flash = 40;
        lfl_sfx("lfl_thump");
        for (int i = 0; i < LFL_MAX_MOBS; i++) {
            LflMob *m = &lg.mob[i];
            if (!m->alive || ((m->flags & MF_FRIENDLY) && !(m->flags & MF_ANGRY))) continue;
            kill_mob(i, true);
        }
        for (int y = 0; y < LFL_RH; y++)
            for (int x = 0; x < LFL_RW; x++)
                if (cur()->t[y][x] == T_TECH) cur()->t[y][x] = T_TECHDOWN;
        if (cur()->secret_key == SK_THUMPER) lfl_open_secret();
        break;
    case TL_CHOPS: {
        if (!fresh) return;
        int n = 0;
        for (int i = 0; i < LFL_MAX_DROPS; i++) n += lg.drop[i].alive && lg.drop[i].kind == DR_CHOP;
        if (n >= 4) return;
        int fx, fy;
        front_point(14, &fx, &fy);
        if (lfl_solid_for_player(tile_at(fx, fy), false) || fx < 4 || fy < 4 || fx > LFL_ROOM_PW - 4 || fy > LFL_ROOM_PH - 4) {
            fx = cx;
            fy = cy;
        }
        if (!pay(TL_CHOPS)) return;
        add_drop(DR_CHOP, 0, fx - 4, fy - 4, -1);
        /* a fresh chop isn't eaten at once */
        for (int i = 0; i < LFL_MAX_DROPS; i++)
            if (lg.drop[i].alive && lg.drop[i].kind == DR_CHOP && lg.drop[i].cd == 0) lg.drop[i].cd = 40;
        lfl_sfx("lfl_print");
        break;
    }
    case TL_BOOTS:
    case TL_TALKBOX:
        if (!fresh) return;
        lg.talk_kind = 2 + (tool == TL_TALKBOX);
        lfl_start_talk(-1);
        break;
    default: break;
    }
}

static void deliver_disc(LflShot *s) {
    if (s->carry_thing >= 0 && lp.thing[s->carry_thing].alive) take_thing(s->carry_thing, true);
    if (s->carry_drop >= 0 && lg.drop[s->carry_drop].alive) take_drop(s->carry_drop);
    s->alive = 0;
}

static bool shot_tile_blocks(int kind, int t) {
    switch (t) {
    case T_FLOOR: case T_WOOD: case T_TECHDOWN: case T_SWAMP: case T_WARP: case T_PIT: case T_STAIRS_DN: case T_STAIRS_UP: return false;
    case T_LOG: return kind != SH_DISC; /* the disc flies through a hollow log */
    default: return true;
    }
}

static void shot_hits_tile(LflShot *s, int tx, int ty) {
    LflRoom *r = cur();
    int t = lfl_tile(lg.cur, tx, ty);
    if (t == T_CRYSTAL && s->kind == SH_RIVET) {
        r->t[ty][tx] = T_FLOOR; /* rivets break crystals safely */
        lfl_sfx("lfl_crystal");
    }
    if (t == T_CRYSTAL && s->kind == SH_HATCH) {
        r->t[ty][tx] = T_FLESH;
        for (int i = 0; i < LFL_MAX_FLESH; i++)
            if (!lg.flesh[i].alive) {
                lg.flesh[i].alive = 1;
                lg.flesh[i].tx = (uint8_t)tx;
                lg.flesh[i].ty = (uint8_t)ty;
                lg.flesh[i].t = MORPH_T;
                break;
            }
    }
    if (t == T_SECRET) {
        if ((s->kind == SH_RIVET && r->secret_key == SK_RIVET) || (s->kind == SH_DISC && r->secret_key == SK_DISC)) lfl_open_secret();
    }
}

static void update_shots(void) {
    LflPlayer *p = &lg.pl;
    int px = LFL_PX(p->x), py = LFL_PX(p->y), w = pw();
    for (int i = 0; i < LFL_MAX_SHOTS; i++) {
        LflShot *s = &lg.shot[i];
        if (!s->alive) continue;
        int size = s->kind == SH_DISC ? 8 : s->kind == SH_HATCH ? 6 : 4;
        if (s->kind == SH_DISC && s->returning) {
            /* home on Wick */
            int dx = pcx() - (LFL_PX(s->x) + 4), dy = pcy() - (LFL_PX(s->y) + 4);
            int len = imax(1, iabs(dx) + iabs(dy));
            s->vx = dx * 1024 / len;
            s->vy = dy * 1024 / len;
            if (iabs(dx) < 7 && iabs(dy) < 7) { deliver_disc(s); continue; }
            s->x += s->vx;
            s->y += s->vy;
        } else {
            int32_t nx = s->x + s->vx, ny = s->y + s->vy;
            int cx = LFL_PX(nx) + size / 2, cy = LFL_PX(ny) + size / 2;
            bool off = cx < 0 || cy < 0 || cx >= LFL_ROOM_PW || cy >= LFL_ROOM_PH;
            int t = off ? T_STONE : tile_at(cx, cy);
            bool block = off || shot_tile_blocks(s->kind, t);
            if (block) {
                if (!off) shot_hits_tile(s, cx / LFL_TILE, cy / LFL_TILE);
                if (s->kind == SH_DISC) { s->returning = 1; continue; }
                if (s->kind == SH_HATCH && !off && s->bounces < 4) {
                    /* the hatcher's shot ricochets */
                    int bx = LFL_PX(s->x + s->vx) + size / 2, by = LFL_PX(s->y) + size / 2;
                    if (shot_tile_blocks(SH_HATCH, tile_at(bx, by))) s->vx = -s->vx;
                    else s->vy = -s->vy;
                    s->bounces++;
                    lfl_sfx("lfl_bounce");
                    continue;
                }
                if (s->kind == SH_HATCH && off && s->bounces < 4) {
                    if (cx < 0 || cx >= LFL_ROOM_PW) s->vx = -s->vx; else s->vy = -s->vy;
                    s->bounces++;
                    continue;
                }
                s->alive = 0;
                continue;
            }
            s->x = nx;
            s->y = ny;
            if (--s->life <= 0) {
                if (s->kind == SH_DISC) s->returning = 1;
                else s->alive = 0;
                continue;
            }
        }
        int sx = LFL_PX(s->x), sy = LFL_PX(s->y);
        if (s->kind == SH_FOE || s->kind == SH_SHARD) {
            if (rects_overlap(sx, sy, size, size, px, py, w, w)) {
                lfl_hurt_player(s->dmg, s->kind == SH_SHARD ? 5 : 3);
                if (s->kind == SH_FOE) s->alive = 0;
            }
            continue;
        }
        if (s->kind == SH_HATCH && s->bounces > 0 && rects_overlap(sx, sy, size, size, px, py, w, w)) {
            /* a ricochet into Wick herself: small, or big again */
            s->alive = 0;
            if (p->shrink_t > 0) p->grow_wait = 1;
            else {
                int cx = pcx(), cy = pcy();
                p->shrink_t = SHRINK_T;
                p->x = (cx - PW_SMALL / 2) * LFL_FX;
                p->y = (cy - PW_SMALL / 2) * LFL_FX;
                p->inv = imax(p->inv, INV_MORPH);
                lfl_sfx("lfl_shrink");
            }
            continue;
        }
        for (int m = 0; m < LFL_MAX_MOBS && s->alive; m++) {
            LflMob *mb = &lg.mob[m];
            if (!mb->alive || (mb->flags & MF_SHELL)) continue;
            if (!rects_overlap(sx, sy, size, size, LFL_PX(mb->x), LFL_PX(mb->y), mb->w, mb->h)) continue;
            hurt_mob(m, s->dmg, s->kind);
            if (s->kind != SH_DISC) s->alive = 0;
        }
        if (s->kind == SH_DISC && s->carry_thing < 0 && s->carry_drop < 0) {
            /* the disc grabs what it touches */
            for (int k = 0; k < LFL_MAX_DROPS; k++) {
                LflDrop *d = &lg.drop[k];
                if (d->alive && rects_overlap(sx, sy, size, size, LFL_PX(d->x), LFL_PX(d->y), 8, 8)) { s->carry_drop = (int8_t)k; s->returning = 1; break; }
            }
            for (int k = 0; k < lp.nthing && s->carry_drop < 0; k++) {
                LflThing *t = &lp.thing[k];
                if (t->alive && t->room == lg.cur && rects_overlap(sx, sy, size, size, t->x, t->y, 12, 12)) { s->carry_thing = (int8_t)k; s->returning = 1; break; }
            }
        }
        if (s->kind == SH_DISC) {
            if (s->carry_drop >= 0) { lg.drop[s->carry_drop].x = s->x; lg.drop[s->carry_drop].y = s->y; }
            if (s->carry_thing >= 0) { lp.thing[s->carry_thing].x = (int16_t)(sx - 2); lp.thing[s->carry_thing].y = (int16_t)(sy - 2); }
        }
    }
}

static void update_bombs(void) {
    for (int i = 0; i < LFL_MAX_BOMBS; i++) {
        LflBomb *b = &lg.bomb[i];
        if (!b->alive) continue;
        if (--b->t <= 0) {
            b->alive = 0;
            lfl_explode(LFL_PX(b->x), LFL_PX(b->y), 22, 2, 4, false);
        }
    }
    for (int i = 0; i < LFL_MAX_BLASTS; i++) {
        LflBlast *b = &lg.blast[i];
        if (!b->alive) continue;
        if (--b->t > 0) continue;
        b->alive = 0;
        if (b->kind == 1) {
            /* crystal shards, eight ways */
            for (int d = 0; d < 8; d++) {
                static const int VX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, VY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
                int sp = (VX[d] && VY[d]) ? 452 : 640;
                add_foe_shot(SH_SHARD, b->x - 2, b->y - 2, VX[d] * sp, VY[d] * sp, 5);
            }
            lfl_sfx("lfl_shards");
        }
    }
    for (int i = 0; i < LFL_MAX_FLESH; i++) {
        LflFlesh *f = &lg.flesh[i];
        if (!f->alive) continue;
        if (--f->t <= 0) {
            f->alive = 0;
            if (cur()->t[f->ty][f->tx] == T_FLESH) cur()->t[f->ty][f->tx] = T_CRYSTAL;
        }
    }
}

/* cooked chops call a gnat, one at a time */
static void update_chops(void) {
    bool gnat = false;
    for (int i = 0; i < LFL_MAX_MOBS; i++) gnat |= lg.mob[i].alive && lg.mob[i].sp == SP_GNAT;
    for (int i = 0; i < LFL_MAX_DROPS; i++) {
        LflDrop *d = &lg.drop[i];
        if (!d->alive || d->kind != DR_CHOP) continue;
        if (d->cd > 0) d->cd--;
        if (!d->cooked) {
            int t = tile_at(LFL_PX(d->x) + 4, LFL_PX(d->y) + 4);
            if (t == T_SWAMP || t == T_WARP) d->cooked = 1;
            continue;
        }
        if (d->cooked == 2) continue;
        d->t++;
        if (d->t >= GNAT_WAIT && !gnat && !lg.no_mobs) {
            /* in from the nearest edge */
            int x = LFL_PX(d->x), y = LFL_PX(d->y);
            int ex = x < LFL_ROOM_PW / 2 ? 0 : LFL_ROOM_PW - 8, ey = y;
            if (imin(y, LFL_ROOM_PH - y) < imin(x, LFL_ROOM_PW - x)) { ex = x; ey = y < LFL_ROOM_PH / 2 ? 0 : LFL_ROOM_PH - 8; }
            lfl_spawn_mob(SP_GNAT, ex, ey, -1);
            lg.mosquitos++;
            gnat = true;
            d->cooked = 2; /* this chop has called its gnat */
            lfl_sfx("lfl_buzz");
        }
    }
}

/* ------------------------------------------------------------------------ */
/* the player                                                               */

int lfl_talk_target(void) {
    int px = pcx(), py = pcy(), best = -1, bd = 26;
    for (int i = 0; i < LFL_MAX_MOBS; i++) {
        const LflMob *m = &lg.mob[i];
        if (!m->alive || !(m->flags & MF_FRIENDLY) || (m->flags & (MF_ANGRY | MF_MORPHED))) continue;
        int d = iabs(LFL_PX(m->x) + 6 - px) + iabs(LFL_PX(m->y) + 6 - py);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

bool lfl_near_ship(void) {
    if (lg.cur != lp.ship_room || lg.state != LS_PLAY) return false;
    int sx = lp.ship_tx * LFL_TILE, sy = lp.ship_ty * LFL_TILE;
    return rects_overlap(LFL_PX(lg.pl.x) - 2, LFL_PX(lg.pl.y) - 2, pw() + 4, pw() + 4, sx, sy, 32, 32);
}

static void try_move(int dir, int32_t speed) {
    LflPlayer *p = &lg.pl;
    int w = pw();
    int32_t nx = p->x + DX[dir] * speed, ny = p->y + DY[dir] * speed;
    if (player_box_free(LFL_PX(nx), LFL_PX(ny), w)) { p->x = nx; p->y = ny; return; }
    /* pixel by pixel up to the wall */
    for (int k = 0; k < 2; k++) {
        int32_t sx = p->x + DX[dir] * LFL_FX, sy = p->y + DY[dir] * LFL_FX;
        if (!player_box_free(LFL_PX(sx), LFL_PX(sy), w)) break;
        p->x = sx; p->y = sy;
    }
    /* the corner slide: walking into the side of a block eases her toward its edge */
    int px = LFL_PX(p->x), py = LFL_PX(p->y);
    for (int k = 1; k <= 8; k++)
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            int ox = (dir == DIR_UP || dir == DIR_DOWN) ? sgn * k : 0;
            int oy = (dir == DIR_LEFT || dir == DIR_RIGHT) ? sgn * k : 0;
            if (player_box_free(px + ox + DX[dir], py + oy + DY[dir], w) && player_box_free(px + isign(ox), py + isign(oy), w)) {
                p->x += isign(ox) * LFL_FX;
                p->y += isign(oy) * LFL_FX;
                return;
            }
        }
    /* along the edge of the room against a wall: slide on (the side walk) */
}

static void start_scroll(int dir) {
    int n = lfl_neighbor(lg.cur, dir);
    if (n < 0) return;
    lg.scroll_dir = dir;
    lg.scroll_t = 0;
    lg.scroll_to = n;
    lg.state = LS_SCROLL;
}

void lfl_scroll_update(void) {
    lg.scroll_t++;
    if (!lg.no_timer) lg.run_t++;
    if (lg.scroll_t < SCROLL_T) return;
    LflPlayer *p = &lg.pl;
    int d = lg.scroll_dir, w = pw();
    int x = LFL_PX(p->x), y = LFL_PX(p->y);
    if (d == DIR_RIGHT) x = 1;
    if (d == DIR_LEFT) x = LFL_ROOM_PW - w - 1;
    if (d == DIR_DOWN) y = 1;
    if (d == DIR_UP) y = LFL_ROOM_PH - w - 1;
    p->x = x * LFL_FX;
    p->y = y * LFL_FX;
    lg.prev_room = lg.cur;
    lfl_enter_room(lg.scroll_to, true);
    lg.state = LS_PLAY;
}

static void stairs_and_warps(void) {
    LflPlayer *p = &lg.pl;
    int t = tile_at(pcx(), pcy());
    if (t != T_STAIRS_DN && t != T_STAIRS_UP) lg.stairs_lock = 0;
    if (t != T_WARP) lg.warp_lock = 0;
    if ((t == T_STAIRS_DN || t == T_STAIRS_UP) && !lg.stairs_lock) {
        lg.stairs_lock = 1;
        lfl_sfx("lfl_stairs");
        int to, tx, ty;
        if (t == T_STAIRS_DN) {
            to = lp.dun_room[0];
            tx = 8; ty = 4;
        } else {
            to = lp.dun_entry_room;
            tx = ty = 0;
            for (int y = 0; y < LFL_RH; y++)
                for (int x = 0; x < LFL_RW; x++)
                    if (lp.room[to].t[y][x] == T_STAIRS_DN) { tx = x; ty = y; }
        }
        p->x = (tx * LFL_TILE + 8 - pw() / 2) * LFL_FX;
        p->y = (ty * LFL_TILE + 8 - pw() / 2) * LFL_FX;
        lg.prev_room = lg.cur;
        lfl_enter_room(to, true);
        music_play(lp.room[to].area == AREA_DUN ? LFL_MUS_DUNGEON : LFL_MUS_PLANET);
        return;
    }
    if (t == T_WARP && !lg.warp_lock) {
        int here = -1;
        int tx = pcx() / LFL_TILE, ty = pcy() / LFL_TILE;
        for (int k = 0; k < 2; k++)
            if (lp.warp_room[k] == lg.cur && lp.warp_x[k] == tx && lp.warp_y[k] == ty) here = k;
        if (here < 0) return;
        int k = 1 - here;
        if (lp.warp_room[k] < 0) return;
        lg.warp_lock = 1;
        lg.warps_taken++;
        lfl_sfx("lfl_warp");
        p->x = (lp.warp_x[k] * LFL_TILE + 8 - pw() / 2) * LFL_FX;
        p->y = (lp.warp_y[k] * LFL_TILE + 8 - pw() / 2) * LFL_FX;
        int to = lp.warp_room[k];
        bool area_change = lp.room[to].area != lp.room[lg.cur].area;
        lg.prev_room = lg.cur;
        lfl_enter_room(to, true);
        if (area_change) music_play(lp.room[to].area == AREA_DUN ? LFL_MUS_DUNGEON : LFL_MUS_PLANET);
    }
}

static void player_update(void) {
    LflPlayer *p = &lg.pl;
    if (p->inv > 0) p->inv--;
    if (p->hurt_flash > 0) p->hurt_flash--;
    if (p->fire_cd > 0) p->fire_cd--;
    /* small for a while; grows back when there is room */
    if (p->shrink_t > 0) {
        p->shrink_t--;
        if (p->shrink_t <= 0 || p->grow_wait) {
            int cx = pcx(), cy = pcy();
            int nx = cx - PW / 2, ny = cy - PW / 2;
            bool room = !box_hits_tiles(nx, ny, PW, PW, false);
            if (room) {
                p->shrink_t = 0;
                p->grow_wait = 0;
                p->x = nx * LFL_FX;
                p->y = ny * LFL_FX;
                p->inv = imax(p->inv, INV_MORPH);
                lfl_sfx("lfl_grow");
            } else if (p->shrink_t <= 0) p->shrink_t = 1;
        }
    }
    /* the pad: four ways, the newest press wins */
    bool h = btn(BTN_LEFT) || btn(BTN_RIGHT), v = btn(BTN_UP) || btn(BTN_DOWN);
    if (btnp(BTN_LEFT) || btnp(BTN_RIGHT)) lg.axis = 0;
    if (btnp(BTN_UP) || btnp(BTN_DOWN)) lg.axis = 1;
    int dir = -1;
    if (h && (!v || lg.axis == 0)) dir = btn(BTN_LEFT) ? DIR_LEFT : DIR_RIGHT;
    else if (v) dir = btn(BTN_UP) ? DIR_UP : DIR_DOWN;
    /* A: the active tool; held, it keeps the facing (strafe) */
    if (btn(BTN_A)) p->a_held++;
    else p->a_held = 0;
    if (dir >= 0) {
        if (p->a_held == 0) p->face = dir;
        try_move(dir, WALK);
        p->walk_t++;
    }
    if (btnp(BTN_A)) use_tool(true);
    else if (p->a_held > 1) use_tool(false);
    /* B: talk, else swap hands; held, set the active tool down */
    if (btn(BTN_B)) {
        p->b_held++;
        if (p->b_held == DROP_HOLD && !p->b_used && p->hold[p->act] >= 0 && p->shrink_t <= 0) {
            int tool = p->hold[p->act];
            bool flying = tool == TL_DISC && count_shots(SH_DISC);
            if (!flying) {
                p->hold[p->act] = TL_NONE;
                lfl_give_tool_drop(tool, pcx() - 6, pcy() - 6);
                lfl_sfx("lfl_drop");
            }
            p->b_used = 1;
        }
    } else {
        if (p->b_held > 0 && p->b_held < DROP_HOLD && !p->b_used) {
            int who = lfl_talk_target();
            if (who >= 0) {
                lg.talk_kind = 0;
                lfl_start_talk(who);
            } else {
                p->act ^= 1;
                lfl_sfx("lfl_swap");
            }
        }
        p->b_held = 0;
        p->b_used = 0;
    }
    if (lg.state != LS_PLAY) return;
    /* off the edge: the next room */
    int x = LFL_PX(p->x), y = LFL_PX(p->y), w = pw();
    if (x < 0) { start_scroll(DIR_LEFT); return; }
    if (y < 0) { start_scroll(DIR_UP); return; }
    if (x + w > LFL_ROOM_PW) { start_scroll(DIR_RIGHT); return; }
    if (y + w > LFL_ROOM_PH) { start_scroll(DIR_DOWN); return; }
    /* pickups */
    for (int i = 0; i < LFL_MAX_DROPS; i++) {
        LflDrop *d = &lg.drop[i];
        if (!d->alive || d->cd > 0) continue;
        if (!rects_overlap(x, y, w, w, LFL_PX(d->x), LFL_PX(d->y), 8, 8)) continue;
        bool carried = false;
        for (int s = 0; s < LFL_MAX_SHOTS; s++) carried |= lg.shot[s].alive && lg.shot[s].carry_drop == i;
        if (carried) continue;
        if (d->kind == DR_CHOP && p->hp >= p->hpmax) continue; /* not hungry */
        take_drop(i);
    }
    for (int i = 0; i < lp.nthing; i++) {
        LflThing *t = &lp.thing[i];
        if (!t->alive || t->room != lg.cur) continue;
        bool touch = rects_overlap(x, y, w, w, t->x, t->y, 12, 12);
        /* a tool just set down waits until she has stepped off it */
        if (t->cd > 0) { if (!touch) t->cd--; continue; }
        if (!touch) continue;
        if (t->kind == TH_TOOL && p->shrink_t > 0) continue;
        take_thing(i, false);
    }
    /* the bog bites, a tile at a time, unless she wears the boots */
    int under = tile_at(pcx(), pcy());
    if ((under == T_SWAMP || under == T_WARP) && !holding(TL_BOOTS)) lfl_hurt_player(1, 1);
    stairs_and_warps();
    /* creatures she touches */
    for (int i = 0; i < LFL_MAX_MOBS; i++) {
        LflMob *m = &lg.mob[i];
        if (!m->alive || (m->flags & (MF_SHELL | MF_MORPHED))) continue;
        if ((m->flags & MF_FRIENDLY) && !(m->flags & MF_ANGRY)) continue;
        if (rects_overlap(x, y, w, w, LFL_PX(m->x), LFL_PX(m->y), m->w, m->h)) lfl_hurt_player(lp.sp[m->sp].dmg, 4);
    }
    /* the ship */
    if (lfl_near_ship()) {
        if (lg.maps >= 3) { lfl_win(); return; }
        bool rich = true;
        for (int r = 0; r < RS_COUNT; r++) rich &= lg.res[r] >= 31;
        if (rich && lg.joke_t == 0) {
            lfl_say("THEY SAY SKYWELL IS NOT HALF THIS GOOD");
            lg.joke_t = 1;
        }
    } else lg.joke_t = 0;
}

void lfl_play_update(void) {
    if (!lg.no_timer) lg.run_t++;
    if (lg.msg_t > 0) lg.msg_t--;
    if (lg.pulse_flash > 0) lg.pulse_flash--;
    for (int r = 0; r < RS_COUNT; r++)
        if (lg.res_flash[r] > 0) lg.res_flash[r]--;
    player_update();
    if (lg.state != LS_PLAY) return;
    for (int i = 0; i < LFL_MAX_MOBS; i++)
        if (lg.mob[i].alive) mob_update(i);
    update_shots();
    update_bombs();
    update_chops();
}
