/* FENNEC FOUNTAIN - the offline room solver and room generator. Not part of
 * the game: a development tool that runs the game's own rule code
 * (src/games/fennec/fennec_logic.c) to prove rooms solvable, to find their
 * shortest solutions, and to grow new rooms of our own by hill climbing.
 *
 * Build (w64devkit or any C11 compiler):
 *   gcc -O2 -std=c11 -DFN_NO_SHELL -o build/fnsolve tools/fennec/fnsolve.c \
 *       src/games/fennec/fennec_logic.c src/games/fennec/fennec_rooms.c
 *
 * Usage:
 *   fnsolve check [CAP]                 solve every room in fennec_rooms.c
 *   fnsolve solve FILE [CAP]            solve the rooms in FILE (see below)
 *   fnsolve gen SPEC SEED ITERS [OUT]   grow a room for chapter SPEC (A..G)
 *
 * A room file holds rooms as rows of room letters, one room after another,
 * separated by a blank line. Lines starting with ';' are comments.
 *
 * Moves are U R D L; '.' lets go of a basalt push (it shrinks then). The
 * search is breadth-first over whole game states, so a solution it finds is
 * a shortest one, counted in steps ('.' costs nothing). */
#include "../../src/games/fennec/fennec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* state keys                                                           */

#define KMAX 160

typedef struct Solver {
    const FnRoom *r;
    int keylen, ng, nb0;
    uint8_t *keys;
    int32_t *parent;
    char *mv;
    int n, cap;
    uint32_t *tab;
    uint32_t tmask;
} Solver;

static int encode(const Solver *sv, const FnState *s, uint8_t *k) {
    int i = 0;
    k[i++] = s->px;
    k[i++] = s->py;
    /* geckos are alike: sort them */
    int g[FN_MAX_GECKOS];
    for (int a = 0; a < s->ng; a++) g[a] = s->gy[a] * 64 + s->gx[a];
    for (int a = 1; a < s->ng; a++)
        for (int b = a; b > 0 && g[b] < g[b - 1]; b--) { int t = g[b]; g[b] = g[b - 1]; g[b - 1] = t; }
    for (int a = 0; a < s->ng; a++) { k[i++] = (uint8_t)(g[a] % 64); k[i++] = (uint8_t)(g[a] / 64); }
    /* blocks in reading order */
    int idx[FN_MAX_BLOCKS];
    for (int a = 0; a < s->nb; a++) idx[a] = a;
    for (int a = 1; a < s->nb; a++)
        for (int b = a; b > 0; b--) {
            const FnBlock *p = &s->b[idx[b]], *q = &s->b[idx[b - 1]];
            if (p->y * 64 + p->x < q->y * 64 + q->x) { int t = idx[b]; idx[b] = idx[b - 1]; idx[b - 1] = t; }
            else break;
        }
    int push = -1;
    for (int a = 0; a < s->nb; a++)
        if (idx[a] == s->push_blk) push = a;
    k[i++] = (uint8_t)(push + 1);
    k[i++] = (uint8_t)(push >= 0 ? s->push_dir + 1 : 0);
    k[i++] = s->door_open;
    k[i++] = s->nb;
    for (int a = 0; a < s->nb; a++) {
        const FnBlock *b = &s->b[idx[a]];
        k[i++] = b->x;
        k[i++] = b->y;
        k[i++] = (uint8_t)(b->kind << 4 | b->n);
    }
    while (i < sv->keylen) k[i++] = 0xFF;
    return i;
}

static void decode(const Solver *sv, const uint8_t *k, FnState *s) {
    memset(s, 0, sizeof *s);
    int i = 0;
    s->px = k[i++];
    s->py = k[i++];
    s->ng = (uint8_t)sv->ng;
    for (int a = 0; a < sv->ng; a++) { s->gx[a] = k[i++]; s->gy[a] = k[i++]; }
    int push = k[i++] - 1;
    int pd = k[i++] - 1;
    s->door_open = k[i++];
    s->nb = k[i++];
    for (int a = 0; a < s->nb; a++) {
        s->b[a].x = k[i++];
        s->b[a].y = k[i++];
        s->b[a].kind = k[i] >> 4;
        s->b[a].n = k[i++] & 15;
    }
    s->push_blk = (int8_t)push;
    s->push_dir = (int8_t)(push >= 0 ? pd : -1);
}

static uint32_t hash_key(const uint8_t *k, int n) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < n; i++) { h ^= k[i]; h *= 16777619u; }
    return h;
}

/* returns the index of the key, adding it if new (*added = 1); -1 when full */
static int intern(Solver *sv, const uint8_t *k, int parent, char mv, int *added) {
    uint32_t h = hash_key(k, sv->keylen) & sv->tmask;
    for (;;) {
        uint32_t e = sv->tab[h];
        if (!e) break;
        if (!memcmp(sv->keys + (size_t)(e - 1) * sv->keylen, k, (size_t)sv->keylen)) { *added = 0; return (int)e - 1; }
        h = (h + 1) & sv->tmask;
    }
    if (sv->n >= sv->cap) return -1;
    int id = sv->n++;
    memcpy(sv->keys + (size_t)id * sv->keylen, k, (size_t)sv->keylen);
    sv->parent[id] = parent;
    sv->mv[id] = mv;
    sv->tab[h] = (uint32_t)id + 1;
    *added = 1;
    return id;
}

static Solver g_sv;

static void solver_init(int cap) {
    Solver *sv = &g_sv;
    if (sv->cap == cap) return;
    free(sv->keys); free(sv->parent); free(sv->mv); free(sv->tab);
    sv->cap = cap;
    sv->keys = malloc((size_t)cap * KMAX);
    sv->parent = malloc(sizeof(int32_t) * (size_t)cap);
    sv->mv = malloc((size_t)cap);
    uint32_t ts = 1;
    while (ts < (uint32_t)cap * 2u) ts <<= 1;
    sv->tab = malloc(sizeof(uint32_t) * ts);
    sv->tmask = ts - 1;
    if (!sv->keys || !sv->parent || !sv->mv || !sv->tab) { fprintf(stderr, "out of memory\n"); exit(1); }
}

/* Breadth-first search. Returns the number of steps of a shortest solution
 * and writes it to out, -1 if there is none, -2 if the search ran out of
 * room. *explored gets the number of states seen. */
static int solve(const FnRoom *r, const FnState *start, char *out, int outmax, int *explored) {
    Solver *sv = &g_sv;
    sv->r = r;
    sv->ng = start->ng;
    sv->nb0 = start->nb;
    sv->keylen = 6 + 2 * start->ng + 3 * start->nb;
    if (sv->keylen > KMAX) return -2;
    memset(sv->tab, 0, sizeof(uint32_t) * (sv->tmask + 1));
    sv->n = 0;
    uint8_t k[KMAX];
    int added;
    encode(sv, start, k);
    intern(sv, k, -1, 0, &added);
    int head = 0, goal = -1, full = 0;
    FnState s, t;
    while (head < sv->n && goal < 0) {
        int cur = head++;
        decode(sv, sv->keys + (size_t)cur * sv->keylen, &s);
        for (int d = 0; d < 5 && goal < 0; d++) {
            char m;
            t = s;
            if (d < 4) {
                fn_step(r, &t, d, NULL);
                m = "URDL"[d];
            } else {
                /* let go of the basalt, then push it again the same way */
                if (s.push_blk < 0) continue;
                fn_release(r, &t, NULL);
                fn_step(r, &t, s.push_dir, NULL);
                m = "urdl"[s.push_dir];
            }
            encode(sv, &t, k);
            int id = intern(sv, k, cur, m, &added);
            if (id < 0) { full = 1; continue; }
            if (added && t.won) goal = id;
        }
    }
    if (explored) *explored = sv->n;
    if (goal < 0) return full ? -2 : -1;
    /* walk back */
    int len = 0, steps = 0;
    for (int i = goal; sv->parent[i] >= 0; i = sv->parent[i]) len += sv->mv[i] >= 'a' ? 2 : 1;
    if (len + 1 > outmax) return -2;
    out[len] = 0;
    int p = len;
    for (int i = goal; sv->parent[i] >= 0; i = sv->parent[i]) {
        char m = sv->mv[i];
        if (m >= 'a') { out[--p] = (char)(m - 32); out[--p] = '.'; }
        else out[--p] = m;
        steps++;
    }
    return steps;
}

/* ------------------------------------------------------------------ */
/* what a solution does                                                 */

typedef struct Usage {
    int steps, pushes, merges, shrinks, lapis, gecko_pushes, arrows, doors, marble;
} Usage;

static bool is_arrow_tile(int t) { return t >= FT_ARROW_U && t <= FT_ARROW_L; }

static void usage_of(const FnRoom *r, const FnState *start, const char *sol, Usage *u) {
    memset(u, 0, sizeof *u);
    FnState s = *start;
    FnEvents ev;
    for (const char *p = sol; *p; p++) {
        ev.n = 0;
        FnState before = s;
        if (*p == '.') { fn_release(r, &s, &ev); }
        else {
            int d = *p == 'U' ? DIR_UP : *p == 'R' ? DIR_RIGHT : *p == 'D' ? DIR_DOWN : DIR_LEFT;
            fn_step(r, &s, d, &ev);
            u->steps++;
        }
        bool walked = false, pushed = false;
        for (int i = 0; i < ev.n; i++) {
            FnEvent *e = &ev.ev[i];
            switch (e->type) {
            case FE_WALK: walked = true; break;
            case FE_PUSH: {
                pushed = true;
                if (walked) u->gecko_pushes++;
                /* the block's kind before the step */
                for (int b = 0; b < before.nb; b++)
                    if (before.b[b].x == e->x && before.b[b].y == e->y && before.b[b].kind == BK_LAPIS) u->lapis++;
                if (e->x2 >= 0 && e->y2 >= 0 && e->x2 < FN_W && e->y2 < FN_H && is_arrow_tile(r->tile[e->y2][e->x2])) u->arrows++;
                if (is_arrow_tile(r->tile[e->y][e->x])) u->arrows++;
                break;
            }
            case FE_MERGE: u->merges++; break;
            case FE_SHRINK: case FE_CRUMBLE: u->shrinks++; break;
            case FE_MARBLE: u->marble++; break;
            case FE_OPEN: u->doors++; break;
            default: break;
            }
        }
        if (pushed) u->pushes++;
    }
}

/* ------------------------------------------------------------------ */
/* rooms as text                                                        */

typedef struct Grid {
    int w, h;
    char c[FN_H][FN_W + 1];
} Grid;

static int grid_parse(const Grid *g, FnRoom *r, FnState *s) {
    const char *rows[FN_H];
    for (int y = 0; y < FN_H; y++) rows[y] = y < g->h ? g->c[y] : NULL;
    return fn_parse(rows, r, s);
}

static void grid_print(FILE *f, const Grid *g) {
    for (int y = 0; y < g->h; y++) fprintf(f, "%s\n", g->c[y]);
}

/* ------------------------------------------------------------------ */
/* check: the game's rooms                                              */

static int cmd_check(int cap) {
    solver_init(cap);
    static char out[8192];
    int total = 0, bad = 0;
    for (int i = 0; i < FN_ROOMS; i++) {
        FnRoom r;
        FnState s;
        if (fn_parse(FN_ROOMS_DEF[i].rows, &r, &s)) { printf("%2d parse error\n", i + 1); bad++; continue; }
        int ex = 0;
        clock_t t0 = clock();
        int n = solve(&r, &s, out, sizeof out, &ex);
        double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
        bool stored = fn_solution_check(&r, &s, FN_ROOMS_DEF[i].solution);
        int slen = 0;
        for (const char *p = FN_ROOMS_DEF[i].solution; *p; p++) slen += *p != '.';
        Usage u;
        if (n > 0) usage_of(&r, &s, out, &u);
        else memset(&u, 0, sizeof u);
        printf("%2d %-20s shortest %4d stored %4d %s states %8d %.1fs pushes %d merges %d shrinks %d lapis %d gecko %d arrows %d doors %d\n",
               i + 1, FN_ROOMS_DEF[i].name, n, slen, stored ? "ok " : "BAD", ex, secs, u.pushes, u.merges, u.shrinks, u.lapis,
               u.gecko_pushes, u.arrows, u.doors);
        if (!stored || n != slen) bad++;
        if (n > 0) total += n;
    }
    printf("total shortest steps %d, %d rooms not matching\n", total, bad);
    return bad ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/* solve: rooms from a file                                             */

static int read_rooms(const char *path, Grid *gs, int max) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return 0; }
    char line[256];
    int n = 0;
    Grid cur;
    memset(&cur, 0, sizeof cur);
    while (fgets(line, sizeof line, f)) {
        size_t l = strlen(line);
        while (l && (line[l - 1] == '\n' || line[l - 1] == '\r')) line[--l] = 0;
        if (line[0] == ';') continue;
        if (!l) {
            if (cur.h && n < max) gs[n++] = cur;
            memset(&cur, 0, sizeof cur);
            continue;
        }
        if (cur.h < FN_H && l <= FN_W) {
            memcpy(cur.c[cur.h], line, l + 1);
            if ((int)l > cur.w) cur.w = (int)l;
            cur.h++;
        }
    }
    if (cur.h && n < max) gs[n++] = cur;
    fclose(f);
    return n;
}

static int cmd_solve(const char *path, int cap) {
    solver_init(cap);
    static Grid gs[128];
    static char out[16384];
    int n = read_rooms(path, gs, 128);
    for (int i = 0; i < n; i++) {
        FnRoom r;
        FnState s;
        int e = grid_parse(&gs[i], &r, &s);
        if (e) { printf("room %d: parse error %d\n", i + 1, e); continue; }
        int ex = 0;
        clock_t t0 = clock();
        int len = solve(&r, &s, out, sizeof out, &ex);
        Usage u;
        if (len > 0) usage_of(&r, &s, out, &u);
        else memset(&u, 0, sizeof u);
        printf("room %d: shortest %d states %d %.1fs pushes %d merges %d shrinks %d lapis %d gecko %d arrows %d doors %d\n%s\n",
               i + 1, len, ex, (double)(clock() - t0) / CLOCKS_PER_SEC, u.pushes, u.merges, u.shrinks, u.lapis, u.gecko_pushes,
               u.arrows, u.doors, len > 0 ? out : "-");
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* gen: growing a room by hill climbing                                 */

static uint64_t rs = 88172645463325252ull;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (uint32_t)(rs >> 11); }
static int rn(int n) { return n <= 1 ? 0 : (int)(rnd() % (uint32_t)n); }

/* a chapter: what a room may hold */
typedef struct Spec {
    int w, h;              /* outside size, walls included */
    int sand_min, sand_max, sand_top; /* sandstone count and highest number */
    int basalt_max, basalt_big;       /* basalt count; may it be 2 x 2 */
    int lapis_max, gecko_max, marble_max;
    int arrow_max, patch_max, plates;  /* plates: 0 none, else a door pair */
    int scenery;           /* may use planters and statues */
    int need;              /* features the solution must use (bit mask) */
    int min_len, max_len;
} Spec;

enum { NEED_BASALT = 1, NEED_LAPIS = 2, NEED_GECKO = 4, NEED_ARROW = 8, NEED_DOOR = 16, NEED_MERGE = 32 };

static const Spec SPECS[] = {
    /* A: sandstone and merging */
    {12, 8, 2, 4, 3, 0, 0, 0, 0, 1, 0, 0, 0, 1, NEED_MERGE, 60, 200},
    /* B: basalt */
    {14, 8, 1, 3, 3, 3, 1, 0, 0, 1, 0, 0, 0, 1, NEED_BASALT, 70, 220},
    /* C: bigger gardens, sand and basalt */
    {16, 9, 2, 5, 4, 3, 1, 0, 0, 1, 0, 0, 0, 1, NEED_BASALT | NEED_MERGE, 90, 260},
    /* D: lapis */
    {16, 9, 1, 4, 4, 2, 1, 3, 0, 1, 0, 0, 0, 1, NEED_LAPIS, 90, 260},
    /* E: geckos */
    {16, 9, 1, 4, 4, 2, 1, 2, 2, 1, 0, 0, 0, 1, NEED_GECKO, 90, 280},
    /* F: arrows, patches, doors */
    {18, 10, 1, 4, 4, 2, 1, 2, 1, 1, 4, 4, 1, 1, NEED_ARROW | NEED_DOOR, 100, 300},
    /* G: the finale, everything */
    {20, 10, 2, 5, 4, 3, 1, 2, 1, 1, 3, 3, 1, 1, NEED_BASALT | NEED_LAPIS | NEED_GECKO, 120, 320},
};

static bool inside(const Grid *g, int x, int y) { return x > 0 && y > 0 && x < g->w - 1 && y < g->h - 1; }

static int count_of(const Grid *g, bool (*pred)(char)) {
    int n = 0;
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++) n += pred(g->c[y][x]);
    return n;
}
static bool is_sand(char c) { return c >= '1' && c <= '4'; }
static bool is_lapis(char c) { return c >= 'a' && c <= 'd'; }
static bool is_basalt1(char c) { return c == 'w'; }
static bool is_basalt2(char c) { return c == 'x'; }
static bool is_gecko(char c) { return c == 'g'; }
static bool is_marble(char c) { return c == '5'; }
static bool is_arrow(char c) { return c == '^' || c == '>' || c == 'v' || c == '<'; }
static bool is_patch(char c) { return c == ':'; }
static bool is_scenery(char c) { return c == 'P' || c == 'H'; }

/* a random free floor tile, or 0 */
static bool free_tile(const Grid *g, int *ox, int *oy) {
    for (int t = 0; t < 200; t++) {
        int x = 1 + rn(g->w - 2), y = 1 + rn(g->h - 2);
        if (g->c[y][x] == '.') { *ox = x; *oy = y; return true; }
    }
    return false;
}

static bool free_square(const Grid *g, int x, int y, int n) {
    for (int yy = y; yy < y + n; yy++)
        for (int xx = x; xx < x + n; xx++)
            if (!inside(g, xx, yy) || g->c[yy][xx] != '.') return false;
    return true;
}

static void clear_square_of(Grid *g, int x, int y) {
    /* remove the multi-tile piece covering (x, y) */
    char c = g->c[y][x];
    int n = c == 'x' || c == 'P' ? 2 : c == 'H' ? 3 : 1;
    if (n == 1) { g->c[y][x] = '.'; return; }
    /* find its anchor: the top-left tile of an aligned square of c */
    for (int ay = y - n + 1; ay <= y; ay++)
        for (int ax = x - n + 1; ax <= x; ax++) {
            bool ok = ay >= 0 && ax >= 0;
            for (int yy = ay; ok && yy < ay + n; yy++)
                for (int xx = ax; ok && xx < ax + n; xx++)
                    if (yy >= g->h || xx >= g->w || g->c[yy][xx] != c) ok = false;
            if (ok) {
                for (int yy = ay; yy < ay + n; yy++)
                    for (int xx = ax; xx < ax + n; xx++) g->c[yy][xx] = '.';
                return;
            }
        }
    g->c[y][x] = '.';
}

static void place_square(Grid *g, int x, int y, int n, char c) {
    for (int yy = y; yy < y + n; yy++)
        for (int xx = x; xx < x + n; xx++) g->c[yy][xx] = c;
}

static void random_room(Grid *g, const Spec *sp) {
    memset(g, 0, sizeof *g);
    g->w = sp->w;
    g->h = sp->h;
    for (int y = 0; y < g->h; y++) {
        for (int x = 0; x < g->w; x++) g->c[y][x] = (x == 0 || y == 0 || x == g->w - 1 || y == g->h - 1) ? '#' : '.';
        g->c[y][g->w] = 0;
    }
    int nw = (g->w - 2) * (g->h - 2) / 6;
    for (int i = 0; i < nw; i++) { int x, y; if (free_tile(g, &x, &y)) g->c[y][x] = '#'; }
    const char one[] = "KGS";
    for (int i = 0; i < 3; i++) { int x, y; if (free_tile(g, &x, &y)) g->c[y][x] = one[i]; }
    int ns = sp->sand_min + rn(sp->sand_max - sp->sand_min + 1);
    for (int i = 0; i < ns; i++) { int x, y; if (free_tile(g, &x, &y)) g->c[y][x] = (char)('1' + rn(sp->sand_top)); }
    for (int i = 0; i < sp->basalt_max; i++) { int x, y; if (free_tile(g, &x, &y)) g->c[y][x] = 'w'; }
    for (int i = 0; i < sp->lapis_max; i++) { int x, y; if (free_tile(g, &x, &y)) g->c[y][x] = (char)('a' + rn(3)); }
    for (int i = 0; i < sp->gecko_max; i++) { int x, y; if (free_tile(g, &x, &y)) g->c[y][x] = 'g'; }
    for (int i = 0; i < sp->arrow_max; i++) { int x, y; if (free_tile(g, &x, &y)) g->c[y][x] = "^>v<"[rn(4)]; }
    for (int i = 0; i < sp->patch_max; i++) { int x, y; if (free_tile(g, &x, &y)) g->c[y][x] = ':'; }
    if (sp->plates) {
        int x, y;
        if (free_tile(g, &x, &y)) g->c[y][x] = 'o';
        if (free_tile(g, &x, &y)) g->c[y][x] = '|';
    }
}

static void mutate(Grid *g, const Spec *sp) {
    int kind = rn(12);
    int x, y;
    switch (kind) {
    case 0: case 1: case 2: {
        /* a wall appears or goes */
        x = 1 + rn(g->w - 2); y = 1 + rn(g->h - 2);
        if (g->c[y][x] == '.') g->c[y][x] = '#';
        else if (g->c[y][x] == '#') g->c[y][x] = '.';
        break;
    }
    case 3: case 4: case 5: {
        /* a piece moves */
        for (int t = 0; t < 50; t++) {
            x = 1 + rn(g->w - 2); y = 1 + rn(g->h - 2);
            char c = g->c[y][x];
            if (c == '.' || c == '#') continue;
            int n = c == 'x' || c == 'P' ? 2 : c == 'H' ? 3 : 1;
            clear_square_of(g, x, y);
            for (int u = 0; u < 60; u++) {
                int nx = 1 + rn(g->w - 2), ny = 1 + rn(g->h - 2);
                if (free_square(g, nx, ny, n)) { place_square(g, nx, ny, n, c); return; }
            }
            place_square(g, x, y, 1, c == 'x' ? 'w' : c == 'P' || c == 'H' ? '#' : c);
            return;
        }
        break;
    }
    case 6: {
        /* a number changes */
        for (int t = 0; t < 50; t++) {
            x = 1 + rn(g->w - 2); y = 1 + rn(g->h - 2);
            char c = g->c[y][x];
            if (is_sand(c)) { g->c[y][x] = (char)('1' + rn(sp->sand_top)); return; }
            if (is_lapis(c)) { g->c[y][x] = (char)('a' + rn(4)); return; }
            if (c == 'w' && sp->basalt_big && free_square(g, x + 1, y, 1) && free_square(g, x, y + 1, 2)) {
                place_square(g, x, y, 2, 'x');
                return;
            }
            if (c == 'x') { clear_square_of(g, x, y); g->c[y][x] = 'w'; return; }
            if (is_arrow(c)) { g->c[y][x] = "^>v<"[rn(4)]; return; }
        }
        break;
    }
    case 7: case 8: {
        /* a piece is added */
        if (!free_tile(g, &x, &y)) break;
        int pick = rn(9);
        if (pick <= 2 && count_of(g, is_sand) < sp->sand_max) g->c[y][x] = (char)('1' + rn(sp->sand_top));
        else if (pick == 3 && count_of(g, is_basalt1) + count_of(g, is_basalt2) / 4 < sp->basalt_max) g->c[y][x] = 'w';
        else if (pick == 4 && count_of(g, is_lapis) < sp->lapis_max) g->c[y][x] = (char)('a' + rn(3));
        else if (pick == 5 && count_of(g, is_gecko) < sp->gecko_max) g->c[y][x] = 'g';
        else if (pick == 6 && count_of(g, is_arrow) < sp->arrow_max) g->c[y][x] = "^>v<"[rn(4)];
        else if (pick == 7 && count_of(g, is_patch) < sp->patch_max) g->c[y][x] = ':';
        else if (pick == 8 && count_of(g, is_marble) < sp->marble_max) g->c[y][x] = '5';
        break;
    }
    case 9: {
        /* a piece is taken away */
        for (int t = 0; t < 50; t++) {
            x = 1 + rn(g->w - 2); y = 1 + rn(g->h - 2);
            char c = g->c[y][x];
            if (is_sand(c) && count_of(g, is_sand) <= sp->sand_min) continue;
            if (is_sand(c) || is_lapis(c) || c == 'w' || c == 'x' || c == 'g' || is_arrow(c) || c == ':' || c == '5' || is_scenery(c)) {
                clear_square_of(g, x, y);
                return;
            }
        }
        break;
    }
    case 10: {
        /* scenery: a planter or a statue */
        if (!sp->scenery) break;
        int n = rn(3) ? 2 : 3;
        x = 1 + rn(g->w - 2); y = 1 + rn(g->h - 2);
        if (free_square(g, x, y, n)) place_square(g, x, y, n, n == 2 ? 'P' : 'H');
        break;
    }
    default: {
        /* a stretch of wall: two or three in a row */
        x = 1 + rn(g->w - 2); y = 1 + rn(g->h - 2);
        int horiz = rn(2), len = 2 + rn(2);
        char to = g->c[y][x] == '#' ? '.' : '#';
        for (int k = 0; k < len; k++) {
            int xx = x + (horiz ? k : 0), yy = y + (horiz ? 0 : k);
            if (!inside(g, xx, yy)) break;
            if (g->c[yy][xx] == '.' || g->c[yy][xx] == '#') g->c[yy][xx] = to;
        }
        break;
    }
    }
}

typedef struct Eval {
    int len, states;
    Usage u;
    double fit;
    char sol[4096];
} Eval;

static int features(const Usage *u) {
    int f = 0;
    if (u->shrinks) f |= NEED_BASALT;
    if (u->lapis) f |= NEED_LAPIS;
    if (u->gecko_pushes) f |= NEED_GECKO;
    if (u->arrows) f |= NEED_ARROW;
    if (u->doors) f |= NEED_DOOR;
    if (u->merges) f |= NEED_MERGE;
    return f;
}

static double evaluate(const Grid *g, const Spec *sp, Eval *e) {
    FnRoom r;
    FnState s;
    e->fit = -1;
    e->len = -1;
    if (grid_parse(g, &r, &s)) return -1;
    if (r.goal_x == FN_NONE) return -1;
    int nstone = 0;
    for (int i = 0; i < s.nb; i++) nstone += s.b[i].kind == BK_STONE;
    if (nstone != 1) return -1;
    e->len = solve(&r, &s, e->sol, sizeof e->sol, &e->states);
    if (e->len <= 0) return -1;
    usage_of(&r, &s, e->sol, &e->u);
    int f = features(&e->u);
    const Usage *u = &e->u;
    /* pushing counts for more than walking: the loops should move blocks */
    double fit = 3.0 * u->pushes + 0.5 * e->len;
    fit += 4 * (u->merges < 3 ? u->merges : 3) + 3 * (u->shrinks < 4 ? u->shrinks : 4);
    fit += 2 * (u->lapis < 6 ? u->lapis : 6) + 2 * (u->gecko_pushes < 6 ? u->gecko_pushes : 6);
    fit += 2 * (u->arrows < 6 ? u->arrows : 6) + 5 * (u->doors < 2 ? u->doors : 2);
    int missing = sp->need & ~f;
    if (missing) fit *= 0.3;
    if (e->len > sp->max_len) fit -= (e->len - sp->max_len) * 4;
    /* tidy gardens: a lone wall tile in the open costs a little */
    for (int y = 1; y < g->h - 1; y++)
        for (int x = 1; x < g->w - 1; x++) {
            if (g->c[y][x] != '#') continue;
            int nb = (g->c[y - 1][x] == '#' || is_scenery(g->c[y - 1][x])) + (g->c[y + 1][x] == '#' || is_scenery(g->c[y + 1][x])) +
                     (g->c[y][x - 1] == '#' || is_scenery(g->c[y][x - 1])) + (g->c[y][x + 1] == '#' || is_scenery(g->c[y][x + 1]));
            if (!nb) fit -= 1.5;
        }
    e->fit = fit;
    return fit;
}

static int cmd_gen(const char *spec, unsigned seed, int iters, const char *outpath, int cap) {
    int si = spec[0] - 'A';
    if (si < 0 || si >= (int)(sizeof SPECS / sizeof SPECS[0])) { fprintf(stderr, "bad spec\n"); return 1; }
    const Spec *sp = &SPECS[si];
    rs ^= (uint64_t)seed * 0x9E3779B97F4A7C15ull;
    for (int i = 0; i < 8; i++) rnd();
    solver_init(cap);
    static Grid cur, best, cand;
    static Eval ec, eb, en;
    /* a solvable start */
    for (int t = 0;; t++) {
        random_room(&cur, sp);
        if (evaluate(&cur, sp, &ec) > 0) break;
        if (t > 20000) { fprintf(stderr, "no solvable start\n"); return 1; }
    }
    best = cur;
    eb = ec;
    int since = 0;
    for (int it = 0; it < iters; it++) {
        cand = cur;
        int nm = 1 + rn(3);
        for (int k = 0; k < nm; k++) mutate(&cand, sp);
        double f = evaluate(&cand, sp, &en);
        if (f < 0) continue;
        /* take it if no worse, or now and then a little worse, to get out of ruts */
        if (f >= ec.fit || (since > 300 && f >= ec.fit * 0.93 && rn(8) == 0)) {
            cur = cand;
            ec = en;
            if (f > eb.fit) { best = cur; eb = ec; since = 0; }
        }
        if (++since > 1500) { cur = best; ec = eb; since = 0; }
        if (it % 500 == 0) {
            fprintf(stderr, "[%s %u] it %d best len %d fit %.0f states %d\n", spec, seed, it, eb.len, eb.fit, eb.states);
        }
    }
    FILE *f = outpath ? fopen(outpath, "a") : stdout;
    if (!f) f = stdout;
    fprintf(f, "; spec %s seed %u len %d pushes %d merges %d shrinks %d lapis %d gecko %d arrows %d doors %d states %d\n", spec, seed,
            eb.len, eb.u.pushes, eb.u.merges, eb.u.shrinks, eb.u.lapis, eb.u.gecko_pushes, eb.u.arrows, eb.u.doors, eb.states);
    fprintf(f, "; %s\n", eb.sol);
    grid_print(f, &best);
    fprintf(f, "\n");
    if (f != stdout) fclose(f);
    printf("len %d\n", eb.len);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: fnsolve check [CAP] | solve FILE [CAP] | gen SPEC SEED ITERS [OUT] [CAP]\n");
        return 2;
    }
    if (!strcmp(argv[1], "check")) return cmd_check(argc > 2 ? atoi(argv[2]) : 3000000);
    if (!strcmp(argv[1], "solve") && argc > 2) return cmd_solve(argv[2], argc > 3 ? atoi(argv[3]) : 3000000);
    if (!strcmp(argv[1], "gen") && argc > 4)
        return cmd_gen(argv[2], (unsigned)atoi(argv[3]), atoi(argv[4]), argc > 5 ? argv[5] : NULL, argc > 6 ? atoi(argv[6]) : 300000);
    fprintf(stderr, "bad arguments\n");
    return 2;
}
