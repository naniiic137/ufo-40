/* IRONVOW - the demo player. It reads the game the way a player with the
 * floor maps in hand reads the screen, and answers with button presses only
 * (tapped, then let go, so held buttons never jam the menus):
 *
 *   exploring  a shortest-path walk (doors opened, locked ones unlocked
 *              with a key, outlined walls opened with OPEN, pits and lava
 *              floated over, spikes timed with PASS TURN) to whatever comes
 *              next: the floor's own steps (levers, the knight, wells...),
 *              then chests and things lying about, torches, gem panels,
 *              the throne, and the stairs; between, it fights what comes.
 *   menus      it walks the cursor to the choice it wants and presses A.
 *   fighting   it lines up with the nearest foe in the front row, cuts
 *              each wind-up short with a blow, guards what it cannot cut
 *              short, rolls out of what no guard stops, steps away from
 *              clouds and marks on the floor, heals by spell when hurt, and
 *              throws LANCE at guards that will not open and THUNDER at
 *              crowds.
 * Everything it decides comes from the game state each frame, so a reload
 * after a death simply carries on. */
#include "ivw.h"

int ivw_bot_debug;

static int last_mask;
static int wait_frames;

/* ---- presses ------------------------------------------------------------------------------------ */
static int tap(int b) {
    if (last_mask & b) return 0; /* let go first */
    return b;
}

/* ---- the glyph to trace ---------------------------------------------------------------------------- */
static const char *cast_seq;
static int cast_pos, cast_spell = -1;
static bool cast_begun;

static bool start_cast(int sp) {
    if (ig.hero.mp < IVW_SPELL[sp].mana) return false;
    cast_seq = IVW_SPELL[sp].glyph;
    cast_pos = 0;
    cast_spell = sp;
    cast_begun = false;
    return true;
}

/* drives the casting: the buttons, or -1 when no cast is going on */
static int cast_step(void) {
    if (cast_spell < 0) return -1;
    if (!ig.cast.on) {
        if (cast_begun) { cast_spell = -1; return -1; }
        return tap(BTN_B);
    }
    cast_begun = true;
    if (cast_seq[cast_pos]) {
        int b = cast_seq[cast_pos] == 'U' ? BTN_UP : cast_seq[cast_pos] == 'R' ? BTN_RIGHT : cast_seq[cast_pos] == 'D' ? BTN_DOWN : BTN_LEFT;
        int m = tap(b);
        if (m) cast_pos++;
        return m;
    }
    return 0; /* the window runs out by itself */
}

/* the same, never -1: for the moment a cast is begun */
static int cast_buttons(void) {
    int m = cast_step();
    return m < 0 ? 0 : m;
}

/* ---- helpers on the knight and his pack --------------------------------------------------------------- */
static IvwHero *H(void) { return &ig.hero; }

static int pack_n(void) {
    int n = 0;
    while (n < H()->inv_cap && H()->inv[n]) n++;
    return n;
}

static bool worn(int s) { return s == H()->wpn || s == H()->arm || s == H()->shd; }

static int weapon_score(int it) {
    const IvwItem *d = &IVW_ITEM[it];
    if (d->kind != IK_WEAPON) return -1;
    if (it == IT_OATH) return -1; /* a gift for the knight */
    int s = d->a * 10;
    if (H()->stat[STAT_MIGHT] < d->req) s = s * 2 / 3 - 20;
    if (d->b == 0) s += 25; /* quick blades counter better */
    if (d->c & WF_SWEEP) s += 10;
    if (it == IT_DAWN) s += 1000; /* the cherry */
    return s;
}

static int armour_score(int it) {
    const IvwItem *d = &IVW_ITEM[it];
    if (d->kind != IK_ARMOUR) return -1;
    return d->a * 2 - d->b / (H()->stat[STAT_MIGHT] >= 10 ? 2 : 1) + (it == IT_MOULD ? 20 : 0);
}

static int shield_score(int it) {
    const IvwItem *d = &IVW_ITEM[it];
    if (d->kind != IK_SHIELD || (d->c & 1)) return -1;
    return d->a * 10 - d->b / 5 + (it == IT_WRAITH ? 30 : 0);
}

/* the slot of something better than what is worn, -1 if none */
static int better_gear(void) {
    IvwHero *h = H();
    int cw = ivw_weapon_item(), ca = ivw_armour_item(), cs = ivw_shield_item();
    for (int s = 0; s < pack_n(); s++) {
        if (worn(s)) continue;
        int it = h->inv[s];
        if (weapon_score(it) > (cw ? weapon_score(cw) : -100)) return s;
        if (armour_score(it) > (ca ? armour_score(ca) : -100)) return s;
        if (shield_score(it) > (cs ? shield_score(cs) : -100)) return s;
    }
    return -1;
}

/* something the knight need not keep: worse gear, spare tonics and parts beyond a few */
static int keep_quota(int it) {
    switch (it) {
    case IT_RED: return 3;
    case IT_BLUE: return 2;
    case IT_BLACK: case IT_GREY: case IT_HEART: case IT_OOZESKULL: case IT_BONESKULL: case IT_BALL: case IT_TUSK: return 1;
    default: return 0;
    }
}

static bool junk(int s) {
    IvwHero *h = H();
    if (worn(s)) return false;
    int it = h->inv[s];
    const IvwItem *d = &IVW_ITEM[it];
    int q = keep_quota(it);
    if (q) {
        int before = 0;
        for (int i = 0; i < s; i++) before += h->inv[i] == it;
        return before >= q;
    }
    switch (d->kind) {
    case IK_WEAPON: return it != IT_OATH && it != IT_DAWN;
    case IK_ARMOUR: case IK_SHIELD: return true;
    case IK_PART: return true;
    case IK_POTION: return true;
    case IK_SPECIAL: return it == IT_EGG || it == IT_GRUB;
    default: return false;
    }
}

static int first_junk(void) {
    for (int s = 0; s < pack_n(); s++) if (junk(s)) return s;
    return -1;
}

/* ---- the map as the planner sees it ------------------------------------------------------------------------ */
enum { STEP_NONE, STEP_WALK, STEP_OPEN, STEP_UNLOCK, STEP_SPELL_OPEN };
static int16_t cost_map[IVW_H][IVW_W];
static int8_t from_dir[IVW_H][IVW_W];

static bool bad_hidden(int x, int y) {
    /* an outlined wall with a pit behind it: the knight never opens that one */
    for (int d = 0; d < 4; d++)
        if (ivw_tile(x + IVW_DX[d], y + IVW_DY[d]) == T_PIT) return true;
    return false;
}

static bool wet(int t) { return (t == T_SLIME_A && !ig.w.slime_b) || (t == T_SLIME_B && ig.w.slime_b); }

/* the cost of entering a tile, or -1 */
static int enter_cost(int x, int y, bool goal, bool allow_air) {
    int t = ivw_tile(x, y);
    IvwObj *o = ivw_obj_at(x, y);
    if (o && o->kind != OB_BOSS) return -1;
    if (o && o->kind == OB_BOSS && !goal) return -1;
    switch (t) {
    case T_FLOOR: case T_DOOR_OPEN: case T_GATE_OPEN: return 1;
    case T_SPIKES: return 3;
    case T_SLIME_A: case T_SLIME_B: return wet(t) ? 6 : 1;
    case T_LAVA: case T_PIT: return ig.w.fly > 2 ? 2 : allow_air && H()->mp >= 12 ? 25 : -1;
    case T_STAIRS: return goal ? 1 : -1;
    case T_EXIT: return ig.w.escape ? 1 : -1;
    case T_SHIFT_A: return ig.w.shift ? 1 : -1;
    case T_SHIFT_B: return ig.w.shift ? -1 : 1;
    case T_DOOR: return 2;
    case T_LOCKED: return H()->keys ? 2 : -1;
    case T_HIDDEN: return !bad_hidden(x, y) && H()->mp >= 6 ? 4 : -1;
    default: return -1;
    }
}

/* Dijkstra from the knight; goal(x, y) marks the tiles wanted. Returns the
 * cost to the nearest goal and its tile, or -1. */
typedef bool (*GoalFn)(int x, int y);

/* a small binary heap of (cost, tile) */
static int32_t heap[IVW_W * IVW_H * 8];
static int heap_n;
static void heap_push(int cost, int node) {
    int i = heap_n++;
    int32_t v = cost * 4096 + node;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (heap[p] <= v) break;
        heap[i] = heap[p];
        i = p;
    }
    heap[i] = v;
}
static int32_t heap_pop(void) {
    int32_t top = heap[0], v = heap[--heap_n];
    int i = 0;
    for (;;) {
        int c = 2 * i + 1;
        if (c >= heap_n) break;
        if (c + 1 < heap_n && heap[c + 1] < heap[c]) c++;
        if (heap[c] >= v) break;
        heap[i] = heap[c];
        i = c;
    }
    heap[i] = v;
    return top;
}

static int plan(GoalFn goal, int *gx, int *gy, bool allow_air) {
    IvwHero *h = H();
    for (int y = 0; y < IVW_H; y++)
        for (int x = 0; x < IVW_W; x++) { cost_map[y][x] = -1; from_dir[y][x] = -1; }
    static bool done[IVW_H][IVW_W];
    memset(done, 0, sizeof done);
    heap_n = 0;
    cost_map[h->y][h->x] = 0;
    heap_push(0, h->y * IVW_W + h->x);
    while (heap_n > 0) {
        int32_t v = heap_pop();
        int node = v % 4096, bc = v / 4096;
        int bx = node % IVW_W, by = node / IVW_W;
        if (done[by][bx] || bc != cost_map[by][bx]) continue;
        done[by][bx] = true;
        if (goal(bx, by) && !(bx == h->x && by == h->y && bc == 0 && false)) { *gx = bx; *gy = by; return bc; }
        for (int d = 0; d < 4; d++) {
            int nx = bx + IVW_DX[d], ny = by + IVW_DY[d];
            if (nx < 0 || ny < 0 || nx >= IVW_W || ny >= IVW_H || done[ny][nx]) continue;
            int c = enter_cost(nx, ny, goal(nx, ny), allow_air);
            if (c < 0) continue;
            if (cost_map[ny][nx] < 0 || bc + c < cost_map[ny][nx]) {
                cost_map[ny][nx] = (int16_t)(bc + c);
                from_dir[ny][nx] = (int8_t)d;
                if (heap_n < (int)ARRAY_LEN(heap)) heap_push(bc + c, ny * IVW_W + nx);
            }
        }
    }
    return -1;
}

/* the first step of the planned way to (gx, gy) */
static int first_dir(int gx, int gy) {
    int x = gx, y = gy, d = -1;
    while (!(x == H()->x && y == H()->y)) {
        d = from_dir[y][x];
        if (d < 0) return -1;
        x -= IVW_DX[d];
        y -= IVW_DY[d];
    }
    return d;
}


/* ---- the shifting comb: a plan over (tile, shift) where pulling a comb
 * lever changes layer. Returns true and the lever to pull first, if the
 * goal can only be reached by pulling. */
static int16_t cost2[2][IVW_H][IVW_W];
static int8_t prev2[2][IVW_H][IVW_W];

static bool shift_lever_beside(int x, int y, int *lx, int *ly) {
    const IvwFloorDef *fd = &IVW_FLOOR[H()->floor];
    for (int d = 0; d < 4; d++) {
        int tx = x + IVW_DX[d], ty = y + IVW_DY[d];
        if (ivw_tile(tx, ty) != T_LEVER) continue;
        int id = ig.w.id[ty][tx];
        if (id < fd->nlevers && fd->levers[id].job == LV_SHIFT) { *lx = tx; *ly = ty; return true; }
    }
    return false;
}

static bool plan_shift(GoalFn goal, int *lever_x, int *lever_y) {
    IvwHero *h = H();
    int s0 = ig.w.shift;
    static bool done2[2][IVW_H][IVW_W];
    memset(done2, 0, sizeof done2);
    for (int l = 0; l < 2; l++)
        for (int y = 0; y < IVW_H; y++)
            for (int x = 0; x < IVW_W; x++) { cost2[l][y][x] = -1; prev2[l][y][x] = -1; }
    cost2[s0][h->y][h->x] = 0;
    int gl = -1, gx = -1, gy = -1;
    for (;;) {
        int bl = -1, bx = -1, by = -1, bc = 1 << 30;
        for (int l = 0; l < 2; l++)
            for (int y = 0; y < IVW_H; y++)
                for (int x = 0; x < IVW_W; x++)
                    if (!done2[l][y][x] && cost2[l][y][x] >= 0 && cost2[l][y][x] < bc) { bc = cost2[l][y][x]; bl = l; bx = x; by = y; }
        if (bl < 0) break;
        done2[bl][by][bx] = true;
        ig.w.shift = (uint8_t)bl;
        if (goal(bx, by)) { gl = bl; gx = bx; gy = by; break; }
        for (int d = 0; d < 4; d++) {
            int nx = bx + IVW_DX[d], ny = by + IVW_DY[d];
            if (nx < 0 || ny < 0 || nx >= IVW_W || ny >= IVW_H || done2[bl][ny][nx]) continue;
            int c = enter_cost(nx, ny, goal(nx, ny), false);
            if (c < 0) continue;
            if (cost2[bl][ny][nx] < 0 || bc + c < cost2[bl][ny][nx]) { cost2[bl][ny][nx] = (int16_t)(bc + c); prev2[bl][ny][nx] = (int8_t)d; }
        }
        int lx, ly, ol = bl ^ 1;
        if (shift_lever_beside(bx, by, &lx, &ly) && !done2[ol][by][bx] &&
            (cost2[ol][by][bx] < 0 || bc + 4 < cost2[ol][by][bx])) {
            cost2[ol][by][bx] = (int16_t)(bc + 4);
            prev2[ol][by][bx] = 4;
        }
    }
    ig.w.shift = (uint8_t)s0;
    if (gl < 0) return false;
    /* walk back: the first pull from the start */
    int l = gl, x = gx, y = gy;
    bool found = false;
    while (!(l == s0 && x == h->x && y == h->y)) {
        int p = prev2[l][y][x];
        if (p < 0) return false;
        if (p == 4) {
            int lx, ly;
            if (shift_lever_beside(x, y, &lx, &ly)) { *lever_x = lx; *lever_y = ly; found = true; }
            l ^= 1;
        } else {
            x -= IVW_DX[p];
            y -= IVW_DY[p];
        }
    }
    return found;
}

/* ---- goals ----------------------------------------------------------------------------------------------------- */
static int goal_x, goal_y;
static bool g_tile(int x, int y) { return x == goal_x && y == goal_y; }
static bool g_adjacent(int x, int y) {
    return iabs(x - goal_x) + iabs(y - goal_y) == 1 && enter_cost(x, y, false, false) >= 0 && ivw_tile(x, y) != T_SPIKES;
}

/* ---- an errand: walk somewhere, then do something -------------------------------------------------------------- */
enum { ER_NONE, ER_GOTO, ER_ACT, ER_CAST_AT, ER_STAIRS };
typedef struct {
    int kind, x, y, action, spell, arg;
} Errand;
static Errand er;
static int stuck_t;

/* the menu operation in hand */
enum { OP_NONE, OP_MENU, OP_PACK, OP_PICK, OP_SHOP, OP_THRONE, OP_WISH, OP_GEMS };
static int op, op_action, op_slot, op_sub, op_index;

static void face_and(int dir, int *mask) {
    int d = (dir - H()->dir + 4) & 3;
    if (d == 1 || d == 2) *mask = tap(BTN_RIGHT);
    else if (d == 3) *mask = tap(BTN_LEFT);
}

static int menu_index(int action) {
    for (int i = 0; i < ig.nmenu; i++) if (ig.menu[i] == action) return i;
    return -1;
}

static int move_cursor(int sel, int want) {
    if (sel < want) return tap(BTN_DOWN);
    if (sel > want) return tap(BTN_UP);
    return tap(BTN_A);
}

/* ---- the floors' own steps --------------------------------------------------------------------------------------- */
static bool tile_is(int x, int y, int t) { return ivw_tile(x, y) == t; }
static int st_at(int x, int y) { return ig.w.st[y][x]; }

static bool knight_here(void) {
    for (int i = 0; i < ig.w.nobj; i++) if (ig.w.obj[i].kind == OB_KNIGHT) return true;
    return false;
}
static bool obj_xy(int kind, int *x, int *y) {
    for (int i = 0; i < ig.w.nobj; i++)
        if (ig.w.obj[i].kind == kind) { *x = ig.w.obj[i].x; *y = ig.w.obj[i].y; return true; }
    return false;
}

static bool set_errand(int kind, int x, int y, int action, int spell) {
    er = (Errand){kind, x, y, action, spell, 0};
    return true;
}

static bool reachable_adjacent(int x, int y, bool air) {
    goal_x = x; goal_y = y;
    int gx, gy;
    return plan(g_adjacent, &gx, &gy, air) >= 0;
}


static int throne_target_stat(void);

/* a spare gift for the knight */
static int gift_slot(void) {
    IvwHero *h = H();
    int s = ivw_find_item(IT_OATH);
    if (s >= 0 && !h->coin_given) return s;
    for (s = 0; s < pack_n(); s++)
        if (junk(s) && IVW_ITEM[h->inv[s]].value >= 1) return s;
    return -1;
}

static bool g_chest_adj(int x, int y);
static bool g_search(int x, int y);
static bool g_stairs(int x, int y);
static bool g_throne_adj(int x, int y);
static int search_left(void);

static bool floor_steps(void) {
    IvwHero *h = H();
    int f = h->floor, kx, ky;
    /* the knight: the oathblade, junk for favour, then the Dawnbrand after the wish */
    if (knight_here() && obj_xy(OB_KNIGHT, &kx, &ky) && reachable_adjacent(kx, ky, true)) {
        if (h->honor && !h->dawn_given && pack_n() < h->inv_cap) return set_errand(ER_ACT, kx, ky, MA_TALK, 0);
        int g = gift_slot();
        if (g >= 0 && (h->inv[g] == IT_OATH || h->knight_favour < 40)) { op_slot = g; return set_errand(ER_ACT, kx, ky, MA_GIVE, g); }
    }
    /* the coin in a sweet well */
    int cs = ivw_find_item(IT_COIN);
    if (cs >= 0 && !h->honor && obj_xy(OB_SWEET_WELL, &kx, &ky) && reachable_adjacent(kx, ky, true))
        return set_errand(ER_ACT, kx, ky, MA_WELL_ITEM, cs);
    switch (f) {
    case 1:
        if (st_at(12, 13) == 0 && reachable_adjacent(12, 13, false)) return set_errand(ER_ACT, 12, 13, MA_LEVER, 0);
        if (tile_is(5, 15, T_PIT) && reachable_adjacent(2, 15, false)) return set_errand(ER_ACT, 2, 15, MA_LEVER, 0);
        if (tile_is(13, 16, T_HIDDEN) && h->mp >= 6 && reachable_adjacent(13, 16, false)) return set_errand(ER_CAST_AT, 13, 16, 0, SP_OPEN);
        if (ivw_count_item(IT_BOAR) == 0 && h->shards >= 6 && obj_xy(OB_PEDLAR, &kx, &ky) && ig.w.shop[0] == IT_BOAR &&
            reachable_adjacent(kx, ky, false))
            return set_errand(ER_ACT, kx, ky, MA_TRADE, 0);
        break;
    case 2:
        if (tile_is(27, 15, T_HIDDEN) && h->mp >= 6 && reachable_adjacent(27, 15, false)) return set_errand(ER_CAST_AT, 27, 15, 0, SP_OPEN);
        if (st_at(11, 23) == 0 && reachable_adjacent(11, 23, false)) return set_errand(ER_ACT, 11, 23, MA_LEVER, 0);
        break;
    case 3:
        /* dry the tunnel */
        if (!ig.w.slime_b && reachable_adjacent(8, 8, false)) return set_errand(ER_ACT, 8, 8, MA_SWITCH, 0);
        break;
    case 4: {
        /* the comb: if what comes next lies in the other layer, pull the lever that leads there */
        GoalFn goals[4] = {g_throne_adj, g_search, g_chest_adj, g_stairs};
        for (int i = 0; i < 4; i++) {
            int gx, gy, lx, ly;
            if (i == 0 && h->essence < IVW_FLOOR[4].throne_cost) continue;
            if (i == 1 && (search_left() <= 0 || (pack_n() >= h->inv_cap && search_left() <= 1))) continue;
            if (i == 2 && pack_n() >= h->inv_cap && first_junk() < 0) continue;
            if (plan(goals[i], &gx, &gy, false) >= 0) return false;
            if (plan_shift(goals[i], &lx, &ly)) return set_errand(ER_ACT, lx, ly, MA_LEVER, 0);
        }
        break;
    }
    case 5:
        if (st_at(14, 9) == 0 && reachable_adjacent(14, 9, false)) return set_errand(ER_ACT, 14, 9, MA_LEVER, 0);
        break;
    default: break;
    }
    return false;
}

/* ---- generic chores ---------------------------------------------------------------------------------------------- */

static bool g_chest_adj(int x, int y) {
    if (enter_cost(x, y, false, false) < 0 || ivw_tile(x, y) == T_SPIKES) return false;
    for (int d = 0; d < 4; d++) {
        IvwObj *o = ivw_obj_at(x + IVW_DX[d], y + IVW_DY[d]);
        if (o && (o->kind == OB_CHEST || o->kind == OB_MIMIC) && !o->state) return true;
    }
    return false;
}

static bool g_unlit_torch_adj(int x, int y) {
    if (enter_cost(x, y, false, false) < 0) return false;
    for (int d = 0; d < 4; d++) {
        int tx = x + IVW_DX[d], ty = y + IVW_DY[d];
        if (ivw_tile(tx, ty) == T_TORCH && !ig.w.st[ty][tx]) return true;
    }
    return false;
}

static bool g_gems_adj(int x, int y) {
    if (enter_cost(x, y, false, false) < 0) return false;
    for (int d = 0; d < 4; d++) {
        int tx = x + IVW_DX[d], ty = y + IVW_DY[d];
        if (ivw_tile(tx, ty) == T_GEMS && !ig.w.st[ty][tx]) return true;
    }
    return false;
}

static bool g_throne_adj(int x, int y) {
    if (enter_cost(x, y, false, false) < 0) return false;
    for (int d = 0; d < 4; d++) {
        IvwObj *o = ivw_obj_at(x + IVW_DX[d], y + IVW_DY[d]);
        if (o && o->kind == OB_THRONE && o->uses) return true;
    }
    return false;
}

static bool g_search(int x, int y) { return (ig.w.st[y][x] & 0x80) && enter_cost(x, y, false, false) >= 0; }
static bool g_stairs(int x, int y) { return ivw_tile(x, y) == T_STAIRS; }
static bool g_exit(int x, int y) { return ivw_tile(x, y) == T_EXIT; }
static bool g_spawn(int x, int y) {
    for (int i = 0; i < ig.w.nspawn; i++)
        if (iabs(ig.w.spawn_x[i] - x) + iabs(ig.w.spawn_y[i] - y) == 3) return enter_cost(x, y, false, false) == 1;
    return false;
}

static int dir_to(int x, int y, int tx, int ty) {
    for (int d = 0; d < 4; d++)
        if (x + IVW_DX[d] == tx && y + IVW_DY[d] == ty) return d;
    return -1;
}

/* the neighbour of (x, y) holding what g_* looked for */
static int find_adjacent(int x, int y, int what) {
    for (int d = 0; d < 4; d++) {
        int tx = x + IVW_DX[d], ty = y + IVW_DY[d];
        IvwObj *o = ivw_obj_at(tx, ty);
        if (what == 0 && o && (o->kind == OB_CHEST || o->kind == OB_MIMIC) && !o->state) return d;
        if (what == 1 && ivw_tile(tx, ty) == T_TORCH && !ig.w.st[ty][tx]) return d;
        if (what == 2 && ivw_tile(tx, ty) == T_GEMS && !ig.w.st[ty][tx]) return d;
    }
    return -1;
}

static int search_left(void) {
    int k = IVW_FLOOR[H()->floor].search_kind;
    int max = k == IVW_RANDOM ? 4 : k ? 5 : 0;
    return max - ig.w.searches;
}

static int throne_cost(void) { return IVW_FLOOR[H()->floor].throne_cost; }
static bool throne_open(int *x, int *y) {
    for (int i = 0; i < ig.w.nobj; i++)
        if (ig.w.obj[i].kind == OB_THRONE && ig.w.obj[i].uses) { *x = ig.w.obj[i].x; *y = ig.w.obj[i].y; return true; }
    return false;
}

/* how strong the knight should be before a floor's end */
static bool ready_to_leave(void) {
    IvwHero *h = H();
    int tx, ty;
    if (throne_open(&tx, &ty) && reachable_adjacent(tx, ty, true)) return false;
    /* the Gaol's torches: all lit before the Warden */
    if (h->floor <= 2) {
        int lit, n = ivw_torches(&lit);
        if (lit < n) return false;
    }
    /* enough health to go on */
    if (h->hp < ivw_max_hp(h) * 6 / 10) return false;
    return true;
}

/* worth carrying: keys and stones, quest things, good tonics and parts, better gear */
static bool wanted(int it) {
    const IvwItem *d = &IVW_ITEM[it];
    if (d->kind == IK_KEY || it == IT_OATH || it == IT_COIN || it == IT_DAWN) return true;
    if (keep_quota(it)) return ivw_count_item(it) < keep_quota(it);
    int cw = ivw_weapon_item(), ca = ivw_armour_item(), cs = ivw_shield_item();
    if (weapon_score(it) > (cw ? weapon_score(cw) : -100)) return true;
    if (armour_score(it) > (ca ? armour_score(ca) : -100)) return true;
    if (shield_score(it) > (cs ? shield_score(cs) : -100)) return true;
    return false;
}

static bool g_wanted_drop(int x, int y) {
    uint8_t it[8];
    int n = ivw_drops_here(x, y, it, 8);
    for (int i = 0; i < n; i++)
        if (it[i] == IT_IRONKEY || (wanted(it[i]) && (IVW_ITEM[it[i]].kind == IK_KEY || pack_n() < H()->inv_cap))) return true;
    return false;
}

static bool choose_errand(void) {
    IvwHero *h = H();
    int gx, gy, tx, ty;
    if (floor_steps()) return true;
    /* things lying about (keys always; items while the pack has room) */
    if (plan(g_wanted_drop, &gx, &gy, h->floor == 6) >= 0) return set_errand(ER_GOTO, gx, gy, MA_PICKUP, 0);
    if (pack_n() < h->inv_cap || first_junk() >= 0) {
        if (plan(g_chest_adj, &gx, &gy, h->floor == 6) >= 0) return set_errand(ER_GOTO, gx, gy, MA_CHEST, 0);
    }
    /* the stones of the last floor lie over lava: float there */
    if (throne_open(&tx, &ty) && h->essence >= throne_cost() && reachable_adjacent(tx, ty, true))
        return set_errand(ER_ACT, tx, ty, MA_SIT, 0);
    if (h->floor <= 2 && h->mp >= 8 && plan(g_unlit_torch_adj, &gx, &gy, false) >= 0) return set_errand(ER_GOTO, gx, gy, -1, SP_KINDLE);
    if (plan(g_gems_adj, &gx, &gy, false) >= 0) return set_errand(ER_GOTO, gx, gy, MA_GEMS, 0);
    if (search_left() > 0 && (pack_n() < h->inv_cap || search_left() > 1) && plan(g_search, &gx, &gy, false) >= 0)
        return set_errand(ER_GOTO, gx, gy, MA_SEARCH, 0);
    if (h->floor == 6) {
        int sx, sy;
        if (obj_xy(OB_SOCKET, &sx, &sy) && !ig.w.stones_set && ivw_find_item(IT_CINDER) >= 0 && ivw_find_item(IT_MOSS) >= 0 &&
            ivw_find_item(IT_FROST) >= 0 && h->hp >= ivw_max_hp(h) * 8 / 10)
            return set_errand(ER_ACT, sx, sy, MA_SOCKET, 0);
        if (ig.w.escape && plan(g_exit, &gx, &gy, true) >= 0) return set_errand(ER_GOTO, gx, gy, -1, 0);
    }
    /* a throne it cannot pay for yet: wait for flames to come and fight them */
    if (throne_open(&tx, &ty) && h->essence < throne_cost() && plan(g_spawn, &gx, &gy, false) >= 0)
        return set_errand(ER_GOTO, gx, gy, MA_PASS, 0);
    if (ready_to_leave() && h->floor < 6 && plan(g_stairs, &gx, &gy, true) >= 0) return set_errand(ER_STAIRS, gx, gy, 0, 0);
    /* nothing to do: let the flames come */
    if (plan(g_spawn, &gx, &gy, false) >= 0) return set_errand(ER_GOTO, gx, gy, MA_PASS, 0);
    return set_errand(ER_GOTO, h->x, h->y, MA_PASS, 0);
}

/* ---- the throne: where the points go ------------------------------------------------------------------------------ */
static int throne_target_stat(void) {
    IvwHero *h = H();
    /* the first throne: all three into MIGHT (8, the gift); then GRIT and LORE take turns with MIGHT */
    if (h->floor == 1) return STAT_MIGHT;
    int order[3] = {STAT_GRIT, STAT_LORE, STAT_MIGHT};
    int best = order[0];
    for (int i = 0; i < 3; i++) {
        int s = order[i];
        int want = s == STAT_MIGHT ? h->stat[s] - 3 : s == STAT_GRIT ? h->stat[s] : h->stat[s] + 1;
        int bw = best == STAT_MIGHT ? h->stat[best] - 3 : best == STAT_GRIT ? h->stat[best] : h->stat[best] + 1;
        if (want < bw) best = s;
    }
    return best;
}

/* the gem panel: a board with no two alike side by side, reached by swaps */
static uint8_t gem_goal[9];
static bool gem_solve(int i, const int *count) {
    if (i == 9) return true;
    int c[3] = {count[0], count[1], count[2]};
    for (int g = 0; g < 3; g++) {
        if (!c[g]) continue;
        if (i % 3 && gem_goal[i - 1] == g) continue;
        if (i >= 3 && gem_goal[i - 3] == g) continue;
        gem_goal[i] = (uint8_t)g;
        c[g]--;
        if (gem_solve(i + 1, c)) return true;
        c[g]++;
    }
    return false;
}

static int gem_buttons(void) {
    /* the first cell not yet right, and where its gem is */
    int count[3] = {0, 0, 0};
    for (int i = 0; i < 9; i++) count[ig.gem_board[i] % 3]++;
    if (!gem_solve(0, count)) return tap(BTN_B);
    int want = -1, from = -1;
    for (int i = 0; i < 9 && want < 0; i++)
        if (ig.gem_board[i] != gem_goal[i]) {
            want = i;
            for (int j = i + 1; j < 9; j++)
                if (ig.gem_board[j] == gem_goal[i]) { from = j; break; }
        }
    if (want < 0) return 0;
    int target = ig.gem_held < 0 ? from : want;
    if (ig.gem_held >= 0 && ig.gem_held != from) target = ig.gem_held; /* put it back where it was taken */
    int sx = ig.gem_sel % 3, sy = ig.gem_sel / 3, tx = target % 3, ty = target / 3;
    if (sx < tx) return tap(BTN_RIGHT);
    if (sx > tx) return tap(BTN_LEFT);
    if (sy < ty) return tap(BTN_DOWN);
    if (sy > ty) return tap(BTN_UP);
    return tap(BTN_A);
}

/* ---- the screens ------------------------------------------------------------------------------------------------- */
static int screen_buttons(void) {
    IvwHero *h = H();
    switch (ig.state) {
    case VS_TITLE: return ig.state_t > 15 ? tap(BTN_A) : 0;
    case VS_STORY: return ig.state_t > 25 ? tap(BTN_A) : 0;
    case VS_CARD: return ig.state_t > 20 ? tap(BTN_A) : 0;
    case VS_OVER: return ig.state_t > 65 ? tap(BTN_A) : 0;
    case VS_LOOT: return ig.state_t > 35 ? tap(BTN_A) : 0;
    case VS_TALK: return ig.state_t > 10 ? tap(BTN_A) : 0;
    case VS_GLYPH: case VS_MAP: return ig.state_t > 12 ? tap(BTN_B) : 0;
    case VS_ENDING: return BTN_A;
    case VS_TELEPORT: return tap(BTN_B);
    case VS_CONFIRM: {
        bool want = er.kind == ER_STAIRS;
        int sel = want ? 1 : 0;
        if (ig.confirm_sel != sel) return tap(BTN_RIGHT);
        return tap(BTN_A);
    }
    case VS_MENU: {
        int want = op == OP_MENU ? op_action : MA_PASS;
        int i = menu_index(want);
        if (i < 0) { op = OP_NONE; return tap(BTN_B); }
        int m = move_cursor(ig.menu_sel, i);
        if (m == BTN_A) {
            /* what comes after the menu */
            if (want == MA_GIVE || want == MA_WELL_ITEM) { op = OP_PICK; op_index = op_slot; }
            else if (want == MA_PICKUP) { op = OP_PICK; op_index = op_slot; }
            else if (want == MA_TRADE) { op = OP_SHOP; }
            else if (want == MA_SIT) op = OP_THRONE;
            else if (want == MA_GEMS) op = OP_GEMS;
            else if (want == MA_INV) op = OP_PACK;
            else op = OP_NONE;
        }
        return m;
    }
    case VS_PICK: {
        int want = op == OP_PICK ? op_index : 0;
        if (op != OP_PICK) return tap(BTN_B);
        int m = move_cursor(ig.pick_sel, want);
        if (m == BTN_A) op = OP_NONE;
        return m;
    }
    case VS_INV: {
        if (op != OP_PACK) return tap(BTN_B);
        return move_cursor(ig.inv_sel, op_slot);
    }
    case VS_ITEM: {
        if (op != OP_PACK) return tap(BTN_B);
        int want = -1;
        for (int i = 0; i < ig.nitem_menu; i++) if (ig.item_menu[i] == op_sub) want = i;
        if (want < 0) { op = OP_NONE; return tap(BTN_B); }
        int m = move_cursor(ig.item_sel, want);
        if (m == BTN_A) op = OP_NONE;
        return m;
    }
    case VS_SHOP: {
        /* buy the boar shield; otherwise sell junk; then leave */
        if (!ig.shop_buy) {
            int j = first_junk();
            if (j >= 0 && ig.w.shop[0] == IT_BOAR && h->shards < 6) return move_cursor(ig.shop_sel, j);
            return tap(BTN_LEFT);
        }
        for (int i = 0; i < 4; i++)
            if (ig.w.shop[i] == IT_BOAR && h->shards >= 6 && pack_n() < h->inv_cap) return move_cursor(ig.shop_sel, i);
        if (ig.w.shop[0] == IT_BOAR && h->shards < 6 && first_junk() >= 0) return tap(BTN_RIGHT);
        op = OP_NONE;
        return tap(BTN_B);
    }
    case VS_THRONE: {
        if (ig.throne_pts > 0) return move_cursor(ig.throne_sel, throne_target_stat());
        return move_cursor(ig.throne_sel, 3);
    }
    case VS_WISH: return move_cursor(ig.wish_sel, 2); /* honour */
    case VS_GEMS: return gem_buttons();
    default: return 0;
    }
}

/* ---- exploring ------------------------------------------------------------------------------------------------------- */
static int open_menu_for(int action) {
    op = OP_MENU;
    op_action = action;
    return tap(BTN_A);
}

static int pack_op(int slot, int sub) {
    op_slot = slot;
    op_sub = sub;
    return open_menu_for(MA_INV);
}

/* spikes: is it safe to step onto (x, y) now? */
static bool spikes_ok(int x, int y) {
    if (ivw_tile(x, y) != T_SPIKES) return true;
    int t = ig.w.turn;
    bool up1 = (((t + 1) / 2) & 1) != 0;
    int nx = x + (x - H()->x), ny = y + (y - H()->y);
    bool up2 = ivw_tile(nx, ny) == T_SPIKES && ((((t + 2) / 2) & 1) != 0);
    return !up1 && !up2;
}

static int explore_buttons(void) {
    IvwHero *h = H();
    int m;
    if (ig.anim_t > 0) return 0;
    if ((m = cast_step()) >= 0) return m;
    /* hurt: heal by spell, then by tonic */
    /* before the last fight it wants more than before the stairs */
    bool stones = ivw_find_item(IT_CINDER) >= 0 && ivw_find_item(IT_MOSS) >= 0 && ivw_find_item(IT_FROST) >= 0;
    int want_pct = stones && !ig.w.stones_set ? 85 : 60;
    if (h->hp < ivw_max_hp(h) * want_pct / 100 + 1) {
        if (h->mp >= 18 && ivw_max_hp(h) - h->hp >= 40 && start_cast(SP_RESTORE)) return cast_buttons();
        if (h->mp >= 8 && start_cast(SP_MEND)) return cast_buttons();
        int r = ivw_find_item(IT_RED);
        if (r >= 0 && h->hp < ivw_max_hp(h) * 35 / 100) return pack_op(r, 2); /* drink */
    }
    /* gear */
    int g = better_gear();
    if (g >= 0) return pack_op(g, 0); /* equip */
    /* a part readied for the next fight */
    for (int s = 0; s < pack_n(); s++) {
        int it = h->inv[s];
        if (!h->ready[s] && (it == IT_OOZESKULL || it == IT_BONESKULL || it == IT_BALL || it == IT_TUSK || it == IT_HEART ||
                             it == IT_BEETLE))
            return pack_op(s, 3);
    }
    /* a full pack with a chest still shut: drink a spare tonic to make room */
    if (pack_n() >= h->inv_cap && first_junk() < 0) {
        static const uint8_t ORDER[4] = {IT_GREY, IT_BLACK, IT_BLUE, IT_RED};
        int gx2, gy2;
        int kind = IVW_FLOOR[h->floor].search_kind;
        bool last_find = search_left() == 1 && kind && kind != IVW_RANDOM && plan(g_search, &gx2, &gy2, false) >= 0;
        if ((h->floor == 6 && plan(g_chest_adj, &gx2, &gy2, true) >= 0) || last_find)
            for (int k = 0; k < 4; k++) {
                int s = ivw_find_item(ORDER[k]);
                if (s >= 0) return pack_op(s, 2);
            }
    }
    /* a full pack: drop junk to make room for what matters */
    if (pack_n() >= h->inv_cap - 1 && first_junk() >= 0) return pack_op(first_junk(), 6);
    static int er_floor = -1;
    if (er_floor != h->floor) { er_floor = h->floor; er.kind = ER_NONE; }
    if (er.kind == ER_NONE) {
        choose_errand();
        stuck_t = 0;
    }
    if (++stuck_t > 1500) { er.kind = ER_NONE; return 0; }
    /* where the errand leads */
    int gx = er.x, gy = er.y, cost;
    bool air = h->floor >= 3;
    goal_x = er.x; goal_y = er.y;
    bool adjacent_goal = er.kind == ER_ACT || er.kind == ER_CAST_AT;
    bool there = adjacent_goal ? iabs(h->x - er.x) + iabs(h->y - er.y) == 1 : (h->x == er.x && h->y == er.y);
    if (!there) {
        cost = plan(adjacent_goal ? g_adjacent : g_tile, &gx, &gy, air);
        if (cost < 0) { er.kind = ER_NONE; return 0; }
        int d = first_dir(gx, gy);
        if (d < 0) { er.kind = ER_NONE; return 0; }
        if (d != h->dir) { face_and(d, &m); return m; }
        int nx = h->x + IVW_DX[d], ny = h->y + IVW_DY[d];
        int t = ivw_tile(nx, ny);
        if (t == T_DOOR) return open_menu_for(MA_OPEN);
        if (t == T_LOCKED) return open_menu_for(MA_UNLOCK);
        if (t == T_HIDDEN) return start_cast(SP_OPEN) ? cast_buttons() : 0;
        if ((t == T_PIT || t == T_LAVA) && ig.w.fly < 3) return start_cast(SP_FLOAT) ? cast_buttons() : open_menu_for(MA_PASS);
        if (!spikes_ok(nx, ny)) return open_menu_for(MA_PASS);
        return tap(BTN_UP);
    }
    /* arrived */
    switch (er.kind) {
    case ER_STAIRS: er.kind = ER_NONE; return 0; /* (the stairs ask on arrival) */
    case ER_GOTO:
        if (er.spell == SP_KINDLE) {
            int d = find_adjacent(h->x, h->y, 1);
            if (d < 0) { er.kind = ER_NONE; return 0; }
            if (d != h->dir) { face_and(d, &m); return m; }
            er.kind = ER_NONE;
            return start_cast(SP_KINDLE) ? cast_buttons() : 0;
        }
        if (er.action == MA_CHEST || er.action == MA_GEMS) {
            int d = find_adjacent(h->x, h->y, er.action == MA_CHEST ? 0 : 2);
            if (d < 0) { er.kind = ER_NONE; return 0; }
            if (d != h->dir) { face_and(d, &m); return m; }
            int a = er.action;
            er.kind = ER_NONE;
            return open_menu_for(a);
        }
        if (er.action == MA_PASS) {
            /* wait a while for flames; walk on now and then */
            if (++er.arg > 12) { er.kind = ER_NONE; return 0; }
            return open_menu_for(MA_PASS);
        }
        if (er.action == MA_PICKUP) {
            uint8_t it[16];
            int n = ivw_drops_here(h->x, h->y, it, 16);
            op_slot = -1;
            for (int i = 0; i < n && op_slot < 0; i++)
                if (it[i] == IT_IRONKEY || wanted(it[i])) op_slot = i;
            er.kind = ER_NONE;
            if (op_slot < 0) return 0;
            return open_menu_for(MA_PICKUP);
        }
        if (er.action >= 0) { int a = er.action; er.kind = ER_NONE; return open_menu_for(a); }
        er.kind = ER_NONE;
        return 0;
    case ER_ACT: case ER_CAST_AT: {
        int d = dir_to(h->x, h->y, er.x, er.y);
        if (d < 0) { er.kind = ER_NONE; return 0; }
        if (d != h->dir) { face_and(d, &m); return m; }
        int a = er.action, sp = er.spell, k = er.kind;
        er.kind = ER_NONE;
        if (k == ER_CAST_AT) return start_cast(sp) ? cast_buttons() : 0;
        if (a == MA_GIVE || a == MA_WELL_ITEM) op_slot = sp;
        return open_menu_for(a);
    }
    default: er.kind = ER_NONE; return 0;
    }
}

/* ---- fighting ------------------------------------------------------------------------------------------------------------ */
static int fight_buttons(void) {
    IvwFight *f = &ig.f;
    IvwHero *h = H();
    int m;
    if ((m = cast_step()) >= 0) return m;
    if (f->result) return 0;
    bool beetle = f->beetle && !f->beetle_out;
    int px = f->px;
    int mx_hp = ivw_max_hp(h);
    /* heal: the fight waits while the glyph is drawn */
    if (!beetle && (f->pstate == PS_IDLE || f->pstate == PS_GUARD)) {
        if (h->hp < mx_hp * 35 / 100 && h->mp >= 18 && start_cast(SP_RESTORE)) return cast_buttons();
        if (h->hp < mx_hp * 45 / 100 && h->mp >= 8 && start_cast(SP_MEND)) return cast_buttons();
        int alive = ivw_fight_foes_alive();
        if (alive >= 3 && h->mp >= 22 + 8 && start_cast(SP_THUNDER)) return cast_buttons();
    }
    /* the threats coming */
    int danger_block = 999, danger_roll = 999, danger_x = -1;
    for (int i = 0; i < f->nfoe; i++) {
        const IvwFoe *e = &f->foe[i];
        if (!e->alive || e->state != FS_WINDUP) continue;
        const IvwFoeDef *d = &IVW_FOE[e->kind];
        int w = d->windup;
        bool whole = false, unblock = false;
        if (e->kind == FK_COILER && e->alt) { w = 60; unblock = true; }
        if (e->kind == FK_WARDEN && e->strikes) { w = 50; whole = true; }
        if (e->kind == FK_REGENT && e->alt == 2) { w = 55; whole = true; unblock = true; }
        if (e->slow) w = w * 3 / 2;
        int left = w - e->t;
        bool hits = whole || (e->row == 0 && ivw_foe_aligned(e, 6));
        if (!hits) continue;
        if (unblock) { if (left < danger_roll) { danger_roll = left; danger_x = e->x; } }
        else if (left < danger_block) danger_block = left;
    }
    for (int i = 0; i < IVW_MAX_SHOTS; i++) {
        const IvwShot *s = &f->shot[i];
        if (!s->alive || s->kind == SH_MIST || s->kind == SH_MYBOLT) continue;
        if (s->kind == SH_CLOUD) {
            if (iabs(s->x - px) <= 26 * 16) { danger_x = s->x; if (danger_roll > 30) danger_roll = 30; }
            continue;
        }
        int left = s->t_max - s->t;
        if (iabs(s->x - px) > 16 * 16) continue;
        if (s->kind == SH_LIGHTNING) { if (left < danger_roll) { danger_roll = left; danger_x = s->x; } }
        else if (left < danger_block) danger_block = left;
    }
    int shd = ivw_shield_item();
    bool can_guard = beetle || (shd && !(IVW_ITEM[shd].c & 1) && f->blocks_left > 0);
    bool low_st = !beetle && h->st < 14 * 16;
    /* unblockable: roll away just before it lands, or walk out of line */
    if (danger_roll <= 14) {
        int dir = danger_x >= 0 ? (px < danger_x ? -1 : 1) : (px < 88 * 16 ? 1 : -1);
        if (px <= 20 * 16) dir = 1;
        if (px >= 156 * 16) dir = -1;
        if (!f->exhausted && h->st >= 20 * 16 && danger_roll <= 9 && f->pstate != PS_DODGE) {
            if (f->pstate != PS_GUARD || f->down_t > 6) return BTN_DOWN;
            return BTN_DOWN | (dir < 0 ? BTN_LEFT : BTN_RIGHT);
        }
        if (f->pstate == PS_DODGE) return 0;
        return dir < 0 ? BTN_LEFT : BTN_RIGHT;
    }
    /* blockable: guard (if the blow lands before a counter would) */
    const IvwFoe *tgt = NULL;
    int best = 1 << 30;
    for (int i = 0; i < f->nfoe; i++) {
        const IvwFoe *e = &f->foe[i];
        if (!e->alive || e->row != 0 || e->kind == FK_STONE || e->kind == FK_EGG) continue;
        if (e->kind == FK_MATRON && e->row != 0) continue;
        int dd = iabs(e->x - px);
        if (dd < best) { best = dd; tgt = e; }
    }
    if (!tgt) /* eggs and stones only, or a lone back row */
        for (int i = 0; i < f->nfoe; i++) {
            const IvwFoe *e = &f->foe[i];
            if (!e->alive || (e->kind == FK_STONE && e->aux > 0) || (e->row != 0 && e->kind != FK_STONE)) continue;
            int dd = iabs(e->x - px);
            if (dd < best) { best = dd; tgt = e; }
        }
    const IvwSwing *sw = &IVW_SWING[ivw_weapon_item() ? IVW_ITEM[ivw_weapon_item()].b : 1];
    if (danger_block <= sw->windup + 3 && can_guard && !f->exhausted) return BTN_DOWN;
    if (f->pstate == PS_SWING) {
        /* keep the combo going while it is safe */
        if (tgt && iabs(tgt->x - px) <= 16 * 16 && !low_st && f->combo < 3 && danger_block > 30) return tap(BTN_A);
        return 0;
    }
    if (f->pstate == PS_DODGE || f->pstate == PS_STAGGER) return 0;
    if (!tgt) {
        /* nothing in reach: the back row may be struck with LANCE */
        if (!beetle && h->mp >= 10 && f->t % 120 == 60) {
            for (int i = 0; i < f->nfoe; i++)
                if (f->foe[i].alive && iabs(f->foe[i].x - px) <= 16 * 16 && start_cast(SP_LANCE)) return cast_buttons();
        }
        /* drift under a back-row foe */
        for (int i = 0; i < f->nfoe; i++)
            if (f->foe[i].alive) {
                int dx = f->foe[i].x - px;
                if (iabs(dx) > 8 * 16) return dx < 0 ? BTN_LEFT : BTN_RIGHT;
                break;
            }
        return 0;
    }
    int dx = tgt->x - px;
    /* clouds on the floor: step clear */
    for (int i = 0; i < IVW_MAX_SHOTS; i++) {
        const IvwShot *s = &f->shot[i];
        if (s->alive && s->kind == SH_CLOUD && iabs(s->x - px) <= 22 * 16) return s->x < px ? BTN_RIGHT : BTN_LEFT;
    }
    if (iabs(dx) > 10 * 16) return dx < 0 ? BTN_LEFT : BTN_RIGHT;
    if (f->exhausted || low_st) return 0; /* get the breath back, out of the way of nothing */
    /* aligned: strike when it is open; cut a wind-up short; else wait for it to open */
    bool open = tgt->state == FS_RECOVER || tgt->state == FS_STUN || tgt->state == FS_ACTIVE || tgt->freeze > 0 ||
                f->invis > 0 || tgt->kind == FK_MATRON || tgt->kind == FK_STONE;
    const IvwFoeDef *td = &IVW_FOE[tgt->kind];
    if (!(td->flags & FF_GUARD) && tgt->kind != FK_PUFF) open = true;
    if (tgt->state == FS_WINDUP) {
        int w = td->windup;
        if (tgt->kind == FK_COILER && tgt->alt) w = 60;
        if (tgt->kind == FK_WARDEN && tgt->strikes) w = 50;
        if (tgt->kind == FK_REGENT && tgt->alt == 2) w = 55;
        if (w - tgt->t > sw->windup + 2) open = true;
    }
    if (tgt->state == FS_CAST) open = true;
    if (open) return tap(BTN_A);
    /* a guard that will not open: LANCE through it now and then */
    if (!beetle && h->mp >= 12 && tgt->cd > 70 && (td->flags & FF_GUARD) && start_cast(SP_LANCE)) return cast_buttons();
    return 0;
}

void ivw_bot_reset(void) {
    last_mask = 0;
    wait_frames = 0;
    er.kind = ER_NONE;
    op = OP_NONE;
    cast_spell = -1;
}

int ivw_bot_buttons(void) {
    int m = 0;
    if (wait_frames > 0) { wait_frames--; last_mask = 0; return 0; }
    switch (ig.state) {
    case VS_EXPLORE: m = explore_buttons(); break;
    case VS_FIGHT: m = fight_buttons(); break;
    default: cast_spell = ig.cast.on ? cast_spell : -1; m = screen_buttons(); break;
    }
    if (ivw_bot_debug && (m != last_mask || (ivw_bot_debug > 1 && ig.frame_t % 60 == 0)))
        printf("bot is%d f%d st%d (%d,%d) d%d er%d(%d,%d a%d) op%d/%d/%d m%02x hp%d mp%d ess%d inv %d %d %d %d %d %d %d %d %d %d\n",
               ig.inv_sel, (int)ig.f.t, ig.state, H()->x, H()->y, H()->dir, er.kind, er.x, er.y, er.action, op, op_slot, op_sub, m,
               H()->hp, H()->mp, (int)H()->essence, H()->inv[0], H()->inv[1], H()->inv[2], H()->inv[3], H()->inv[4],
               H()->inv[5], H()->inv[6], H()->inv[7], H()->inv[8], H()->inv[9]);
    m &= BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT | BTN_A | BTN_B; /* never START or SELECT */
    last_mask = m;
    return m;
}
