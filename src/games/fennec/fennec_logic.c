/* FENNEC FOUNTAIN - the rules. Pure functions of an FnRoom and an FnState,
 * shared by the game and the offline solver that checked every room.
 * Every rule is listed with its source in docs/games/15-fennec-fountain.md. */
#include "fennec.h"

static const int8_t DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};

int fn_weight(const FnBlock *b) {
    switch (b->kind) {
    case BK_MARBLE: return 5;
    case BK_STONE: return 1;
    default: return b->n;
    }
}

static int size_of(const FnBlock *b) { return b->kind == BK_BASALT ? b->n : 1; }

static bool mergeable(const FnBlock *b) {
    return (b->kind == BK_SAND || b->kind == BK_LAPIS || b->kind == BK_BASALT) && size_of(b) == 1;
}

static bool covers(const FnBlock *b, int x, int y) {
    int s = size_of(b);
    return b->kind != BK_NONE && x >= b->x && y >= b->y && x < b->x + s && y < b->y + s;
}

int fn_block_at(const FnState *s, int x, int y) {
    for (int i = 0; i < s->nb; i++)
        if (covers(&s->b[i], x, y)) return i;
    return -1;
}

/* ------------------------------------------------------------------ */
/* who stands where                                                     */

#define OCC_FENNEC 100
#define OCC_GECKO 101

typedef struct Occ {
    int8_t o[FN_H][FN_W]; /* -1 empty, block index, or a creature */
} Occ;

static void build_occ(const FnState *s, Occ *m) {
    memset(m->o, -1, sizeof m->o);
    for (int i = 0; i < s->nb; i++) {
        const FnBlock *b = &s->b[i];
        if (b->kind == BK_NONE) continue;
        int sz = size_of(b);
        for (int y = b->y; y < b->y + sz && y < FN_H; y++)
            for (int x = b->x; x < b->x + sz && x < FN_W; x++) m->o[y][x] = (int8_t)i;
    }
    for (int g = 0; g < s->ng; g++)
        if (s->gx[g] < FN_W && s->gy[g] < FN_H) m->o[s->gy[g]][s->gx[g]] = (int8_t)(OCC_GECKO + g);
    if (s->px < FN_W && s->py < FN_H) m->o[s->py][s->px] = OCC_FENNEC;
}

static bool open_tile(const FnRoom *r, int x, int y) {
    return x >= 0 && y >= 0 && x < r->w && y < r->h && !r->wall[y][x];
}

/* ------------------------------------------------------------------ */
/* planning a push                                                      */

typedef struct Plan {
    uint8_t moving[FN_MAX_BLOCKS];
    uint8_t consumed[FN_MAX_BLOCKS];
    uint8_t grow[FN_MAX_BLOCKS];
    uint8_t newkind[FN_MAX_BLOCKS];
    uint8_t merged[FN_MAX_BLOCKS];   /* took part in a merge */
} Plan;

static bool plan_push(const FnRoom *r, const FnState *s, const Occ *m, Plan *p, int bi, int dir, bool by_block, int w) {
    const FnBlock *b = &s->b[bi];
    if (p->moving[bi]) return true;
    if (p->consumed[bi]) return true;
    /* lapis only moves when a block pushes it; a block pushes only weights up to its own */
    if (b->kind == BK_LAPIS && !by_block) return false;
    if (by_block && w < fn_weight(b)) return false;
    Plan save = *p;
    p->moving[bi] = 1;
    int sz = size_of(b), dx = DX[dir], dy = DY[dir];
    for (int yy = b->y; yy < b->y + sz; yy++)
        for (int xx = b->x; xx < b->x + sz; xx++) {
            int tx = xx + dx, ty = yy + dy;
            if (covers(b, tx, ty)) continue;
            if (!open_tile(r, tx, ty)) goto fail;
            int o = m->o[ty][tx];
            if (o >= OCC_FENNEC) goto fail;
            if (o < 0 || o == bi) continue;
            if (p->moving[o]) continue; /* it moves out of the way too */
            if (p->consumed[o]) continue;
            const FnBlock *c = &s->b[o];
            bool both_basalt = b->kind == BK_BASALT && c->kind == BK_BASALT;
            if (sz == 1 && mergeable(b) && mergeable(c) && !both_basalt && (c->n + p->grow[o] == 1 || b->n + p->grow[bi] == 1)) {
                if (c->n + p->grow[o] == 1) {
                    /* the pushed block lands on a 1 and adds it */
                    p->consumed[o] = 1;
                    p->grow[bi] = (uint8_t)(p->grow[bi] + 1);
                    if (b->kind == BK_BASALT) p->newkind[bi] = c->kind; /* the non-basalt colour wins */
                } else {
                    /* a 1 pushed onto a block adds itself to it */
                    p->consumed[bi] = 1;
                    p->moving[bi] = 0;
                    p->grow[o] = (uint8_t)(p->grow[o] + 1);
                }
                p->merged[bi] = p->merged[o] = 1;
                continue;
            }
            if (!plan_push(r, s, m, p, o, dir, true, fn_weight(b))) goto fail;
        }
    return true;
fail:
    *p = save;
    return false;
}

static void add_ev(FnEvents *ev, int type, int a, int x, int y, int x2, int y2) {
    if (!ev || ev->n >= (int)(sizeof ev->ev / sizeof ev->ev[0])) return;
    FnEvent *e = &ev->ev[ev->n++];
    e->type = (uint8_t)type;
    e->a = (uint8_t)a;
    e->x = (int8_t)x; e->y = (int8_t)y; e->x2 = (int8_t)x2; e->y2 = (int8_t)y2;
}

static void apply_plan(FnState *s, const Plan *p, int dir, FnEvents *ev) {
    for (int i = 0; i < s->nb; i++) {
        FnBlock *b = &s->b[i];
        if (b->kind == BK_NONE) continue;
        if (p->consumed[i]) {
            add_ev(ev, FE_MERGE, i, b->x, b->y, b->x + DX[dir], b->y + DY[dir]);
            b->kind = BK_NONE;
            continue;
        }
        if (p->moving[i]) {
            add_ev(ev, FE_PUSH, i, b->x, b->y, b->x + DX[dir], b->y + DY[dir]);
            b->x = (uint8_t)(b->x + DX[dir]);
            b->y = (uint8_t)(b->y + DY[dir]);
        }
        if (p->newkind[i]) b->kind = p->newkind[i];
        if (p->grow[i]) {
            b->n = (uint8_t)(b->n + p->grow[i]);
            if (b->n >= 5 && b->kind != BK_STONE) {
                /* five turns to marble, which never changes again */
                b->kind = BK_MARBLE;
                b->n = 5;
                add_ev(ev, FE_MARBLE, i, b->x, b->y, b->x, b->y);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* basalt shrinks when you stop pushing it                              */

static void shrink(FnState *s, int bi, FnEvents *ev) {
    FnBlock *b = &s->b[bi];
    if (b->kind != BK_BASALT) return;
    if (b->n <= 1) {
        add_ev(ev, FE_CRUMBLE, bi, b->x, b->y, b->x, b->y);
        b->kind = BK_NONE;
    } else {
        b->n--;
        add_ev(ev, FE_SHRINK, bi, b->x, b->y, b->x, b->y);
    }
}

static void compact(FnState *s, FnEvents *ev) {
    int map[FN_MAX_BLOCKS], n = 0;
    for (int i = 0; i < s->nb; i++) {
        if (s->b[i].kind == BK_NONE) { map[i] = -1; continue; }
        map[i] = n;
        s->b[n++] = s->b[i];
    }
    if (s->push_blk >= 0) s->push_blk = (int8_t)map[(int)s->push_blk];
    if (ev)
        for (int k = 0; k < ev->n; k++) {
            FnEvent *e = &ev->ev[k];
            if (e->type == FE_GECKO || e->type == FE_WALK || e->type == FE_BUMP || e->type == FE_WIN || e->type == FE_EXIT) continue;
            e->a = (uint8_t)(map[e->a] < 0 ? 255 : map[e->a]);
        }
    s->nb = (uint8_t)n;
}

/* ------------------------------------------------------------------ */
/* a step                                                               */

static bool creature_move(const FnRoom *r, FnState *s, int x, int y, int dir, FnEvents *ev, int *pushed, bool *merged) {
    int tx = x + DX[dir], ty = y + DY[dir];
    *pushed = -1;
    *merged = false;
    if (!open_tile(r, tx, ty)) return false;
    Occ m;
    build_occ(s, &m);
    int o = m.o[ty][tx];
    if (o >= OCC_FENNEC) return false;
    if (o >= 0) {
        Plan p;
        memset(&p, 0, sizeof p);
        if (!plan_push(r, s, &m, &p, o, dir, false, 0)) return false;
        *pushed = o;
        *merged = p.merged[o] != 0;
        apply_plan(s, &p, dir, ev);
    }
    return true;
}

bool fn_step(const FnRoom *r, FnState *s, int dir, FnEvents *ev) {
    if (s->won || s->exited) return false;
    int dx = DX[dir], dy = DY[dir];
    int tx = s->px + dx, ty = s->py + dy;
    /* stopping a basalt push shrinks it */
    if (s->push_blk >= 0) {
        bool cont = dir == s->push_dir && fn_block_at(s, tx, ty) == s->push_blk;
        if (!cont) {
            shrink(s, s->push_blk, ev);
            s->push_blk = -1;
        }
    }
    /* the way out */
    if (tx == r->door_x && ty == r->door_y) {
        add_ev(ev, FE_EXIT, 0, s->px, s->py, tx, ty);
        s->px = (uint8_t)tx;
        s->py = (uint8_t)ty;
        s->exited = 1;
        compact(s, ev);
        return true;
    }
    int pushed;
    bool merged;
    if (!creature_move(r, s, s->px, s->py, dir, ev, &pushed, &merged)) {
        add_ev(ev, FE_BUMP, 0, s->px, s->py, tx, ty);
        compact(s, ev);
        return false;
    }
    add_ev(ev, FE_WALK, 0, s->px, s->py, tx, ty);
    s->px = (uint8_t)tx;
    s->py = (uint8_t)ty;
    if (pushed >= 0 && s->b[pushed].kind == BK_BASALT && !merged) {
        s->push_blk = (int8_t)pushed;
        s->push_dir = (int8_t)dir;
    } else {
        s->push_blk = -1;
    }
    /* the geckos copy the step, the ones in front first */
    int order[FN_MAX_GECKOS];
    for (int g = 0; g < s->ng; g++) order[g] = g;
    for (int a = 1; a < s->ng; a++)
        for (int b = a; b > 0; b--) {
            int ga = order[b], gb = order[b - 1];
            int ka = s->gx[ga] * dx + s->gy[ga] * dy, kb = s->gx[gb] * dx + s->gy[gb] * dy;
            if (ka > kb) { order[b] = gb; order[b - 1] = ga; }
        }
    for (int k = 0; k < s->ng; k++) {
        int g = order[k];
        int p2;
        bool m2;
        int gx = s->gx[g], gy = s->gy[g];
        if (gx + dx == r->door_x && gy + dy == r->door_y) continue;
        if (creature_move(r, s, gx, gy, dir, ev, &p2, &m2)) {
            add_ev(ev, FE_GECKO, g, gx, gy, gx + dx, gy + dy);
            s->gx[g] = (uint8_t)(gx + dx);
            s->gy[g] = (uint8_t)(gy + dy);
        }
    }
    compact(s, ev);
    for (int i = 0; i < s->nb; i++)
        if (s->b[i].kind == BK_STONE && s->b[i].x == r->goal_x && s->b[i].y == r->goal_y) {
            s->won = 1;
            add_ev(ev, FE_WIN, i, s->b[i].x, s->b[i].y, 0, 0);
        }
    return true;
}

/* ------------------------------------------------------------------ */
/* rooms from text                                                      */

int fn_parse(const char *const *rows, FnRoom *room, FnState *st) {
    memset(room, 0, sizeof *room);
    memset(st, 0, sizeof *st);
    room->goal_x = room->goal_y = FN_NONE;
    room->door_x = room->door_y = FN_NONE;
    st->push_blk = -1;
    st->push_dir = -1;
    st->px = st->py = FN_NONE;
    uint8_t claimed[FN_H][FN_W];
    memset(claimed, 0, sizeof claimed);
    int h = 0, w = 0;
    for (int y = 0; y < FN_H && rows[y]; y++) {
        int len = (int)strlen(rows[y]);
        if (len > FN_W) return 1;
        if (len > w) w = len;
        h = y + 1;
    }
    room->w = (uint8_t)w;
    room->h = (uint8_t)h;
    for (int y = 0; y < FN_H; y++)
        for (int x = 0; x < FN_W; x++) room->wall[y][x] = 1;
    for (int y = 0; y < h; y++) {
        int len = (int)strlen(rows[y]);
        for (int x = 0; x < len; x++) {
            char c = rows[y][x];
            if (c != '#' && c != ' ') room->wall[y][x] = 0;
            if (claimed[y][x]) continue;
            switch (c) {
            case 'K': st->px = (uint8_t)x; st->py = (uint8_t)y; break;
            case 'D': room->door_x = (uint8_t)x; room->door_y = (uint8_t)y; break;
            case 'G': room->goal_x = (uint8_t)x; room->goal_y = (uint8_t)y; break;
            case 'g':
                if (st->ng < FN_MAX_GECKOS) { st->gx[st->ng] = (uint8_t)x; st->gy[st->ng] = (uint8_t)y; st->ng++; }
                break;
            default: {
                FnBlock b = {0, 0, (uint8_t)x, (uint8_t)y};
                if (c == 'S') { b.kind = BK_STONE; b.n = 1; }
                else if (c >= '1' && c <= '4') { b.kind = BK_SAND; b.n = (uint8_t)(c - '0'); }
                else if (c == '5') { b.kind = BK_MARBLE; b.n = 5; }
                else if (c >= 'a' && c <= 'd') { b.kind = BK_LAPIS; b.n = (uint8_t)(c - 'a' + 1); }
                else if (c >= 'w' && c <= 'z') {
                    b.kind = BK_BASALT;
                    b.n = (uint8_t)(c - 'w' + 1);
                    for (int yy = y; yy < y + b.n; yy++)
                        for (int xx = x; xx < x + b.n; xx++) {
                            if (yy >= h || xx >= (int)strlen(rows[yy]) || rows[yy][xx] != c) return 2;
                            claimed[yy][xx] = 1;
                        }
                }
                if (b.kind) {
                    if (st->nb >= FN_MAX_BLOCKS) return 3;
                    st->b[st->nb++] = b;
                }
                break;
            }
            }
        }
    }
    if (st->px == FN_NONE) return 4;
    return 0;
}

bool fn_solution_check(const FnRoom *r, const FnState *start, const char *moves) {
    FnState s = *start;
    for (const char *p = moves; *p; p++) {
        int d = *p == 'U' ? DIR_UP : *p == 'R' ? DIR_RIGHT : *p == 'D' ? DIR_DOWN : *p == 'L' ? DIR_LEFT : -1;
        if (d < 0) continue;
        fn_step(r, &s, d, NULL);
    }
    return s.won != 0;
}
