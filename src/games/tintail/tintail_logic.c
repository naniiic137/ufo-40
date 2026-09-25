/* TINTAIL - the rules: terrain, sight, storks, the chameleon's beat, and a
 * breadth-first solver the tests use to prove every level can be escaped.
 * Pure C with no shell dependencies (build with -DTN_NO_SHELL for tools). */
#include "tintail.h"

static const int8_t DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};

static int gcd(int a, int b) { while (b) { int t = a % b; a = b; b = t; } return a; }
static int iabs_(int v) { return v < 0 ? -v : v; }

/* "x y speed MOVES": moves are U R D L, each optionally followed by a count */
static int parse_stork(const char *spec, TtStork *s) {
    int x, y, sp, n = 0;
    if (sscanf(spec, "%d %d %d %n", &x, &y, &sp, &n) != 3) return 1;
    const char *p = spec + n;
    s->len = 0;
    s->speed = (uint8_t)(sp < 1 ? 1 : sp);
    while (*p) {
        int d = *p == 'U' ? DIR_UP : *p == 'R' ? DIR_RIGHT : *p == 'D' ? DIR_DOWN : *p == 'L' ? DIR_LEFT : -1;
        p++;
        if (d < 0) continue;
        int cnt = 0;
        while (*p >= '0' && *p <= '9') cnt = cnt * 10 + (*p++ - '0');
        if (cnt == 0) cnt = 1;
        for (int k = 0; k < cnt; k++) {
            if (s->len >= TN_PATH_MAX) return 2;
            s->px[s->len] = (uint8_t)x;
            s->py[s->len] = (uint8_t)y;
            s->dir[s->len] = (uint8_t)d;
            s->len++;
            x += DX[d];
            y += DY[d];
        }
    }
    /* a stork's walk must bring it back where it started */
    if (s->len == 0 || x != s->px[0] || y != s->py[0]) return 3;
    return 0;
}

int tn_parse(const TtLevelDef *def, TtLevel *lv, TtState *st) {
    memset(lv, 0, sizeof *lv);
    lv->baby_x = lv->baby_y = TN_NONE;
    lv->start_x = lv->hole_x = TN_NONE;
    for (int y = 0; y < TN_H; y++) {
        const char *g = def->ground[y], *t = def->things[y];
        if (!g || !t || (int)strlen(g) != TN_W || (int)strlen(t) != TN_W) return 1;
        for (int x = 0; x < TN_W; x++) {
            int k;
            switch (g[x]) {
            case 'g': k = TN_GRASS; break;
            case 's': k = TN_SAND; break;
            case 'w': k = TN_SWAMP; break;
            case 'r': k = TN_ROCK; break;
            case 'u': k = TN_SWITCHGRASS; lv->wet0[y][x] = 1; break;
            case 'd': k = TN_SWITCHGRASS; break;
            case '~': k = TN_SEA; break;
            case 'T': k = TN_PALM; break;
            case 'B': k = TN_BOULDER; break;
            case 'o': k = TN_BUSH; break;
            case '-': k = TN_LOG_H; break;
            case '|': k = TN_LOG_V; break;
            case 'H': k = TN_HOLE; lv->hole_x = (uint8_t)x; lv->hole_y = (uint8_t)y; break;
            default: return 2;
            }
            lv->kind[y][x] = (uint8_t)k;
            switch (t[x]) {
            case '.': break;
            case 'K': lv->start_x = (uint8_t)x; lv->start_y = (uint8_t)y; break;
            case 'F':
                if (lv->nfruit >= 2) return 3;
                lv->fruit_x[lv->nfruit] = (uint8_t)x;
                lv->fruit_y[lv->nfruit] = (uint8_t)y;
                lv->nfruit++;
                break;
            case 'Y': lv->baby_x = (uint8_t)x; lv->baby_y = (uint8_t)y; break;
            case 'R': if (lv->nrain < TN_MAX_SWITCH) { lv->rain_x[lv->nrain] = (uint8_t)x; lv->rain_y[lv->nrain++] = (uint8_t)y; } break;
            case 'S': if (lv->nsun < TN_MAX_SWITCH) { lv->sun_x[lv->nsun] = (uint8_t)x; lv->sun_y[lv->nsun++] = (uint8_t)y; } break;
            case '^': case '>': case 'v': case '<':
                if (lv->ntoad >= TN_MAX_TOADS) return 4;
                lv->toad_x[lv->ntoad] = (uint8_t)x;
                lv->toad_y[lv->ntoad] = (uint8_t)y;
                lv->toad_dir[lv->ntoad] = (uint8_t)(t[x] == '^' ? DIR_UP : t[x] == '>' ? DIR_RIGHT : t[x] == 'v' ? DIR_DOWN : DIR_LEFT);
                lv->ntoad++;
                break;
            default: return 5;
            }
        }
    }
    if (lv->start_x == TN_NONE || lv->hole_x == TN_NONE) return 6;
    lv->period = 1;
    for (int i = 0; i < TN_MAX_STORKS && def->storks[i]; i++) {
        if (parse_stork(def->storks[i], &lv->stork[i])) return 7;
        lv->nstork++;
        int cyc = lv->stork[i].len * lv->stork[i].speed;
        lv->period = lv->period / gcd(lv->period, cyc) * cyc;
    }
    if (st) tn_reset(lv, st);
    return 0;
}

void tn_reset(const TtLevel *lv, TtState *st) {
    memset(st, 0, sizeof *st);
    st->x = st->px = lv->start_x;
    st->y = st->py = lv->start_y;
    st->face = DIR_RIGHT;
    st->camo = TC_NONE;
    st->bx = lv->baby_x;
    st->by = lv->baby_y;
}

int tn_colour(const TtLevel *lv, const TtState *st, int x, int y) {
    if (x < 0 || y < 0 || x >= TN_W || y >= TN_H) return TC_NONE;
    switch (lv->kind[y][x]) {
    case TN_GRASS: return TC_GRASS;
    case TN_SAND: return TC_SAND;
    case TN_SWAMP: return TC_SWAMP;
    case TN_ROCK: return TC_ROCK;
    case TN_SWITCHGRASS: {
        bool wet = st->sw == SW_START ? lv->wet0[y][x] : st->sw == SW_WET;
        return wet ? TC_GRASS : TC_SAND; /* dry grass is the colour of sand */
    }
    default: return TC_NONE;
    }
}

static bool is_log(const TtLevel *lv, int x, int y) {
    return lv->kind[y][x] == TN_LOG_H || lv->kind[y][x] == TN_LOG_V;
}

bool tn_walkable(const TtLevel *lv, int x, int y) {
    if (x < 0 || y < 0 || x >= TN_W || y >= TN_H) return false;
    int k = lv->kind[y][x];
    if (k == TN_SEA || k == TN_PALM || k == TN_BOULDER || k == TN_BUSH) return false;
    for (int i = 0; i < lv->ntoad; i++)
        if (lv->toad_x[i] == x && lv->toad_y[i] == y) return false;
    return true;
}

void tn_stork_at(const TtLevel *lv, int i, uint32_t beat, int *x, int *y, int *dir) {
    const TtStork *s = &lv->stork[i];
    int n = (int)((beat / s->speed) % s->len);
    *x = s->px[n];
    *y = s->py[n];
    if (dir) *dir = s->dir[n];
}

static bool sight_blocker(const TtLevel *lv, const int *sx, const int *sy, int ns, int x, int y) {
    int k = lv->kind[y][x];
    if (k == TN_PALM || k == TN_BOULDER || k == TN_BUSH) return true;
    for (int i = 0; i < lv->ntoad; i++)
        if (lv->toad_x[i] == x && lv->toad_y[i] == y) return true;
    for (int i = 0; i < ns; i++)
        if (sx[i] == x && sy[i] == y) return true;
    return false;
}

/* is the line from (x0,y0) to (x1,y1) clear, not counting its two ends? */
static bool line_clear(const TtLevel *lv, const int *sx, const int *sy, int ns, int x0, int y0, int x1, int y1) {
    int dx = iabs_(x1 - x0), dy = -iabs_(y1 - y0);
    int stx = x0 < x1 ? 1 : -1, sty = y0 < y1 ? 1 : -1;
    int err = dx + dy, x = x0, y = y0;
    for (;;) {
        if (x == x1 && y == y1) return true;
        if (!(x == x0 && y == y0) && sight_blocker(lv, sx, sy, ns, x, y)) return false;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += stx; }
        if (e2 <= dx) { err += dx; y += sty; }
    }
}

static void cone(const TtLevel *lv, const int *sx, const int *sy, int ns, int ox, int oy, int dir, int range, int id,
                 uint8_t danger[TN_H][TN_W]) {
    int fx = DX[dir], fy = DY[dir], lx = -fy, ly = fx;
    for (int d = 1; d <= range; d++) {
        int hw = d / 2;
        for (int l = -hw; l <= hw; l++) {
            int x = ox + fx * d + lx * l, y = oy + fy * d + ly * l;
            if (x < 0 || y < 0 || x >= TN_W || y >= TN_H) continue;
            if (danger[y][x]) continue;
            if (sight_blocker(lv, sx, sy, ns, x, y)) continue; /* a blocker hides itself too */
            if (line_clear(lv, sx, sy, ns, ox, oy, x, y)) danger[y][x] = (uint8_t)id;
        }
    }
}

void tn_danger(const TtLevel *lv, const TtState *st, uint32_t beat, uint8_t danger[TN_H][TN_W]) {
    (void)st;
    memset(danger, 0, TN_H * TN_W);
    int sx[TN_MAX_STORKS], sy[TN_MAX_STORKS], sd[TN_MAX_STORKS];
    for (int i = 0; i < lv->nstork; i++) tn_stork_at(lv, i, beat, &sx[i], &sy[i], &sd[i]);
    /* storks first: they are the ones that move, and they're the closer threat */
    for (int i = 0; i < lv->nstork; i++) {
        /* a stork doesn't block its own sight: take it out of the list */
        int ox = sx[i], oy = sy[i];
        sx[i] = -9;
        cone(lv, sx, sy, lv->nstork, ox, oy, sd[i], 3, 11 + i, danger);
        sx[i] = ox;
    }
    for (int i = 0; i < lv->ntoad; i++)
        cone(lv, sx, sy, lv->nstork, lv->toad_x[i], lv->toad_y[i], lv->toad_dir[i], 4, 1 + i, danger);
}

bool tn_hidden(const TtLevel *lv, const TtState *st, int who) {
    int x = who ? st->bx : st->x, y = who ? st->by : st->y;
    if (is_log(lv, x, y)) return true;
    if (st->camo_t > 0 || st->camo == TC_NONE) return false;
    return st->camo == tn_colour(lv, st, x, y);
}

int tn_collected(const TtState *st) {
    return (st->fruit & 1) + ((st->fruit >> 1) & 1) + (st->baby ? 1 : 0);
}

/* ------------------------------------------------------------------ */
/* one beat                                                             */

static bool can_camo_with(const TtLevel *lv, const TtState *st, uint8_t dn[TN_H][TN_W]) {
    if (st->camo_t > 0 || is_log(lv, st->x, st->y)) return false;
    int c = tn_colour(lv, st, st->x, st->y);
    if (c == TC_NONE || c == st->camo) return false;
    if (dn[st->y][st->x]) return false;
    if (st->baby && dn[st->by][st->bx]) return false;
    return true;
}

bool tn_can_camo(const TtLevel *lv, const TtState *st) {
    uint8_t dn[TN_H][TN_W];
    tn_danger(lv, st, st->beat, dn);
    return can_camo_with(lv, st, dn);
}

static bool stork_on(const TtLevel *lv, uint32_t beat, int x, int y, int *who) {
    for (int i = 0; i < lv->nstork; i++) {
        int sx, sy;
        tn_stork_at(lv, i, beat, &sx, &sy, NULL);
        if (sx == x && sy == y) { if (who) *who = i; return true; }
    }
    return false;
}

static bool can_enter(const TtLevel *lv, int fx, int fy, int dir) {
    int x = fx + DX[dir], y = fy + DY[dir];
    if (!tn_walkable(lv, x, y)) return false;
    bool horiz = dir == DIR_LEFT || dir == DIR_RIGHT;
    /* logs: in and out through the ends only */
    int k = lv->kind[y][x], kf = lv->kind[fy][fx];
    if (k == TN_LOG_H && !horiz) return false;
    if (k == TN_LOG_V && horiz) return false;
    if (kf == TN_LOG_H && !horiz) return false;
    if (kf == TN_LOG_V && horiz) return false;
    return true;
}

/* the core, with the danger maps for this beat and the next supplied */
static int step_core(const TtLevel *lv, TtState *st, int act, uint8_t dn[TN_H][TN_W], uint8_t dn1[TN_H][TN_W]) {
    int ev = 0;
    if (st->dead || st->won) return 0;
    bool moved = false;
    if (st->camo_t > 0) {
        /* still changing colour */
        st->camo_t--;
        if (st->camo_t == 0) {
            st->camo = (uint8_t)tn_colour(lv, st, st->x, st->y);
            ev |= TE_CAMO_DONE;
        }
    } else if (act == ACT_CAMO) {
        if (can_camo_with(lv, st, dn)) {
            st->camo_t = TN_CAMO_BEATS - 1;
            ev |= TE_CAMO_START;
        } else {
            ev |= TE_REFUSED;
        }
    } else if (act >= ACT_UP && act <= ACT_LEFT) {
        int d = act - ACT_UP;
        st->face = (uint8_t)d;
        int nx = st->x + DX[d], ny = st->y + DY[d];
        if (can_enter(lv, st->x, st->y, d) && !stork_on(lv, st->beat, nx, ny, NULL)) {
            st->px = st->x;
            st->py = st->y;
            st->x = (uint8_t)nx;
            st->y = (uint8_t)ny;
            if (st->baby) { st->bx = st->px; st->by = st->py; }
            moved = true;
            ev |= TE_STEP;
            if (is_log(lv, nx, ny)) ev |= TE_LOG;
        } else {
            ev |= TE_BUMP;
        }
    }
    st->beat++;
    if (moved) {
        for (int i = 0; i < lv->nfruit; i++)
            if (!(st->fruit >> i & 1) && lv->fruit_x[i] == st->x && lv->fruit_y[i] == st->y) {
                st->fruit |= (uint8_t)(1 << i);
                ev |= TE_FRUIT;
            }
        if (!st->baby && lv->baby_x == st->x && lv->baby_y == st->y) {
            st->baby = 1;
            st->bx = st->px;
            st->by = st->py;
            ev |= TE_BABY;
        }
        for (int i = 0; i < lv->nrain; i++)
            if (lv->rain_x[i] == st->x && lv->rain_y[i] == st->y && st->sw != SW_WET) { st->sw = SW_WET; ev |= TE_RAIN; }
        for (int i = 0; i < lv->nsun; i++)
            if (lv->sun_x[i] == st->x && lv->sun_y[i] == st->y && st->sw != SW_DRY) { st->sw = SW_DRY; ev |= TE_SUN; }
        if (st->x == lv->hole_x && st->y == lv->hole_y) {
            st->won = 1;
            return ev | TE_WIN;
        }
    }
    /* walked into by a stork: camouflage or not */
    int who;
    if (stork_on(lv, st->beat, st->x, st->y, &who)) {
        st->dead = 1; st->eaten_by = 1; st->eater = (uint8_t)who; st->baby_eaten = 0;
        return ev | TE_EATEN;
    }
    if (st->baby && stork_on(lv, st->beat, st->bx, st->by, &who)) {
        st->dead = 1; st->eaten_by = 1; st->eater = (uint8_t)who; st->baby_eaten = 1;
        return ev | TE_EATEN;
    }
    /* seen while exposed */
    for (int w = 0; w < 1 + (st->baby ? 1 : 0); w++) {
        int x = w ? st->bx : st->x, y = w ? st->by : st->y;
        int id = dn1[y][x];
        if (id && !tn_hidden(lv, st, w)) {
            st->dead = 1;
            st->baby_eaten = (uint8_t)w;
            if (id >= 11) { st->eaten_by = 1; st->eater = (uint8_t)(id - 11); }
            else { st->eaten_by = 0; st->eater = (uint8_t)(id - 1); }
            return ev | TE_EATEN;
        }
    }
    return ev;
}

int tn_step(const TtLevel *lv, TtState *st, int act) {
    uint8_t dn[TN_H][TN_W], dn1[TN_H][TN_W];
    tn_danger(lv, st, st->beat, dn);
    tn_danger(lv, st, st->beat + 1, dn1);
    return step_core(lv, st, act, dn, dn1);
}

/* ------------------------------------------------------------------ */
/* the solver                                                           */

#define NPOS (TN_W * TN_H)

static uint32_t pack(const TtState *st, int period) {
    int baby = 0;
    if (st->baby) {
        for (int d = 0; d < 4; d++)
            if (st->x + DX[d] == st->bx && st->y + DY[d] == st->by) baby = 1 + d;
    }
    uint32_t v = st->beat % (uint32_t)period;
    v = v * 3 + st->sw;
    v = v * 4 + st->fruit;
    v = v * 5 + (uint32_t)baby;
    v = v * 2 + (st->camo_t ? 1 : 0);
    v = v * TC_COUNT + st->camo;
    v = v * NPOS + (uint32_t)(st->y * TN_W + st->x);
    return v;
}

static void unpack(const TtLevel *lv, uint32_t v, TtState *st) {
    memset(st, 0, sizeof *st);
    int pos = (int)(v % NPOS); v /= NPOS;
    st->camo = (uint8_t)(v % TC_COUNT); v /= TC_COUNT;
    st->camo_t = (uint8_t)(v % 2); v /= 2;
    int baby = (int)(v % 5); v /= 5;
    st->fruit = (uint8_t)(v % 4); v /= 4;
    st->sw = (uint8_t)(v % 3); v /= 3;
    st->beat = v;
    st->x = st->px = (uint8_t)(pos % TN_W);
    st->y = st->py = (uint8_t)(pos / TN_W);
    if (baby) {
        st->baby = 1;
        st->bx = (uint8_t)(st->x + DX[baby - 1]);
        st->by = (uint8_t)(st->y + DY[baby - 1]);
    } else {
        st->bx = lv->baby_x;
        st->by = lv->baby_y;
    }
}

int tn_solve(const TtLevel *lv, bool need_all, char *out, int out_max) {
    int P = lv->period;
    uint32_t total = (uint32_t)P * 3 * 4 * 5 * 2 * TC_COUNT * NPOS;
    uint8_t *seen = (uint8_t *)calloc(total / 8 + 1, 1);
    uint8_t (*dcache)[TN_H][TN_W] = malloc(sizeof(*dcache) * (size_t)P);
    int cap = 1 << 16, n = 0, head = 0;
    uint32_t *qs = (uint32_t *)malloc(sizeof(uint32_t) * (size_t)cap);
    int32_t *qp = (int32_t *)malloc(sizeof(int32_t) * (size_t)cap);
    uint8_t *qa = (uint8_t *)malloc((size_t)cap);
    int result = -1, goal = -1, goal_act = 0;
    if (!seen || !dcache || !qs || !qp || !qa) goto done;
    TtState st0;
    tn_reset(lv, &st0);
    for (int p = 0; p < P; p++) tn_danger(lv, &st0, (uint32_t)p, dcache[p]);
    uint32_t s0 = pack(&st0, P);
    seen[s0 >> 3] |= (uint8_t)(1 << (s0 & 7));
    qs[0] = s0; qp[0] = -1; qa[0] = 0; n = 1;
    int need = lv->nfruit + (lv->baby_x != TN_NONE ? 1 : 0);
    while (head < n && goal < 0) {
        TtState st;
        unpack(lv, qs[head], &st);
        int ph = (int)st.beat;
        for (int act = 0; act <= ACT_CAMO; act++) {
            if (st.camo_t && act) break; /* changing colour: only time passes */
            TtState nx = st;
            step_core(lv, &nx, act, dcache[ph], dcache[(ph + 1) % P]);
            if (nx.dead) continue;
            if (nx.won) {
                if (need_all && tn_collected(&nx) < need) continue;
                goal = head;
                goal_act = act;
                break;
            }
            nx.beat %= (uint32_t)P;
            uint32_t v = pack(&nx, P);
            if (seen[v >> 3] & (1 << (v & 7))) continue;
            seen[v >> 3] |= (uint8_t)(1 << (v & 7));
            if (n >= cap) {
                int nc = cap * 2;
                uint32_t *a = realloc(qs, sizeof(uint32_t) * (size_t)nc);
                if (!a) goto done;
                qs = a;
                int32_t *b = realloc(qp, sizeof(int32_t) * (size_t)nc);
                if (!b) goto done;
                qp = b;
                uint8_t *c = realloc(qa, (size_t)nc);
                if (!c) goto done;
                qa = c;
                cap = nc;
            }
            qs[n] = v; qp[n] = head; qa[n] = (uint8_t)act;
            n++;
        }
        head++;
    }
    if (goal >= 0) {
        int len = 1;
        for (int i = goal; qp[i] >= 0; i = qp[i]) len++;
        result = len;
        if (out && out_max > len) {
            out[len] = 0;
            int k = len;
            out[--k] = ".URDLC"[goal_act];
            for (int i = goal; qp[i] >= 0; i = qp[i]) out[--k] = ".URDLC"[qa[i]];
        } else if (out && out_max > 0) {
            out[0] = 0;
        }
    }
done:
    free(seen);
    free(dcache);
    free(qs);
    free(qp);
    free(qa);
    return result;
}
