/* FLINTHOLD - the stage simulation: the field, the routes, Pim, the towers,
 * hens and fire pits, the foes, the waves and the meat. Every rule and where
 * it comes from is in docs/games/30-flinthold.md. Positions are in 1/16
 * pixel, hit points in tenths. It runs on buttons alone (held and pressed),
 * so the demo player at the bottom plays it the same way a person does. */
#include "flinthold.h"

FhSim fh;
const int FH_DX[4] = {1, 0, -1, 0}, FH_DY[4] = {0, 1, 0, -1};
static const int PAD[4] = {BTN_RIGHT, BTN_DOWN, BTN_LEFT, BTN_UP};

/* ---- the numbers ------------------------------------------------------------ */

/* speed is in 1/64 pixel a frame: 20 is a walker's pace, about 1.2 tiles a
 * second; fangcats go twice that, gliders, shagtusks and Lord Jaw half, Lord
 * Plate and Lord Gale a quarter, Lord Snap a walker's pace.
 *                    name        hp  armour fire arrow speed meat cave flies boss */
const FhFoeDef FH_FOE[E_KINDS] = {
    {"NIPPER",       50,   0,   0,   0, 20,  1,  1, 0, 0},
    {"REDBACK",      80,  10,   0,  10, 20,  2,  2, 0, 0},
    {"FANGCAT",      40,   0,   0,   0, 40,  1,  1, 0, 0},
    {"CLUBTAIL",    120,  80,   0,  80, 20,  2,  2, 0, 0},
    {"GLIDER",       80,  60,  30,   0, 10,  1,  3, 1, 0},
    {"GNAT",         60,   0,   0,   0, 20,  1,  2, 1, 0},
    {"SHAGTUSK",   2000,  50,   0,  50, 10, 10, 10, 0, 1},
    {"LORD JAW",   3000,  50,   0,  50, 10, 10, 30, 0, 1},
    {"LORD PLATE", 4000, 100,   0, 100,  5, 10, 20, 0, 1},
    {"LORD GALE",  2000,  50,  25,   0,  5, 10, 20, 1, 1},
    {"LORD SNAP",  2000,   0,   0,   0, 20, 10, 15, 0, 1},
};

/*                  name       cost from       dmg every range type      shot */
const FhUnitDef FH_UNIT[U_KINDS] = {
    {"THROWER",  10, -1,          10, 50, 2, D_NORMAL, 48},
    {"SPEAR",    20, U_THROWER,   30, 50, 2, D_NORMAL, 48},
    {"BARB",     40, U_SPEAR,     50, 50, 2, D_NORMAL, 48},
    {"BOW",      40, U_SPEAR,    100, 120, 6, D_ARROW, 64},
    {"SLING",    20, U_THROWER,    8, 25, 3, D_NORMAL, 48},
    {"HURLER",   40, U_SLING,     10, 15, 4, D_NORMAL, 56},
    {"BOULDER",  40, U_SLING,      5, 70, 2, D_NORMAL, 32},
    {"TORCH",    20, U_THROWER,   20, 50, 2, D_FIRE,   48},
    {"BLAZE",    40, U_TORCH,      5,  5, 2, D_FIRE,   24},
    {"PITCH",    40, U_TORCH,     10, 30, 2, D_NORMAL, 48},
};

#define HEN_COST 10
#define HEN_SELL 10
#define HEN_COOKED 30
#define HEN_WAVE 5
#define FIRE_COST 10
#define DIG_BUSH 5
#define DIG_ROCK 10
static const int THROW_COST[3] = {0, 30, 50};
static const int WEAPON_COST[3] = {0, 30, 50};
static const int THROW_EVERY[3] = {30, 20, 10};  /* 2, 3 and 6 a second */
static const int THROW_RANGE[3] = {2, 3, 4};
static const int WEAPON_DMG[3] = {10, 20, 30};   /* bones, stone axe, fire axe */

#define PIM_SPEED 11       /* 11/16 of a pixel a frame: about twice a walker */
#define PIM_SHOT 64
#define PIM_DIAG 45        /* PIM_SHOT on each axis of a diagonal throw */
#define PIM_STUN 60        /* knocked down, then gone, then back at the cave: 3 s */
#define PIM_GONE 120
#define PAYOUT_TIME 150    /* after a wave, 2.5 s while its meat comes in */
#define PAYOUT_HENS 75     /* the hens pay half way through; cooking at the end */
#define PIM_R 5            /* half her body, in pixels */
#define TOUCH 10           /* a foe this close to Pim hits her (pixels) */
#define GNAT_SEES 48       /* a gnat goes for Pim within three tiles */
#define GAP_FOE 48         /* frames between foes of a group */
#define GAP_CAT 30
#define GAP_BOSS 150
#define GAP_GROUP 90
#define SLOW_BARB 60
#define SLOW_TAR 120
#define ARROW_FOLLOW 10
#define ROLL_PATH (3 * FH_TU)
#define ROLL_BUDGET (8 * FH_TU)

/* ---- small helpers ---------------------------------------------------------- */

static void ev(int kind, int x, int y, int a) {
    if (fh.n_ev >= FH_MAX_EV) return;
    fh.ev[fh.n_ev++] = (FhEvent){(uint8_t)kind, (int16_t)x, (int16_t)y, (int16_t)a};
}

static bool in_field(int x, int y) { return x >= 0 && y >= 0 && x < FH_W && y < FH_H; }

static void add_meat(int n) {
    int room = FH_MEAT_MAX - fh.meat;
    if (n > room) { fh.wasted += n - room; n = room; }
    fh.meat += n;
}

static int isqrt32(int64_t v) {
    if (v <= 0) return 0;
    int64_t r = (int64_t)sqrt((double)v);
    while (r * r > v) r--;
    while ((r + 1) * (r + 1) <= v) r++;
    return (int)r;
}

bool fh_walkable(int x, int y) {
    if (!in_field(x, y)) return false;
    int t = fh.tile[y][x];
    return (t == TL_GRASS || t == TL_FLOWER || t == TL_PATH) && fh.obj[y][x] == O_NONE;
}

bool fh_blocks_shot(int x, int y) {
    if (!in_field(x, y)) return false;
    int t = fh.tile[y][x];
    return t == TL_THICKET || t == TL_BUSH || t == TL_ROCK || t == TL_HUT;
}

bool fh_buildable(int x, int y) {
    if (!in_field(x, y)) return false;
    int t = fh.tile[y][x];
    return (t == TL_GRASS || t == TL_FLOWER) && fh.obj[y][x] == O_NONE && !fh.near_path[y][x];
}

int fh_fires_near(int x, int y) {
    int n = 0;
    for (int d = 0; d < 4; d++) {
        int nx = x + FH_DX[d], ny = y + FH_DY[d];
        if (in_field(nx, ny) && fh.obj[ny][nx] == O_FIRE) n++;
    }
    return n;
}

/* each fire pit beside a unit: +50 % damage and slow time, up to +200 % */
static int fire_pct(int x, int y) { return 100 + imin(fh_fires_near(x, y), 4) * 50; }

int fh_unit_count(void) {
    int n = 0;
    for (int i = 0; i < FH_MAX_UNITS; i++) n += fh.unit[i].on;
    return n;
}

int fh_hen_count(void) {
    int n = 0;
    for (int y = 0; y < FH_H; y++)
        for (int x = 0; x < FH_W; x++) n += fh.obj[y][x] == O_HEN;
    return n;
}

int fh_foe_count(void) {
    int n = 0;
    for (int i = 0; i < FH_MAX_FOES; i++) n += fh.foe[i].on;
    return n;
}

int fh_throw_every(void) { return THROW_EVERY[fh.throw_lv]; }
int fh_throw_range(void) { return THROW_RANGE[fh.throw_lv]; }
int fh_weapon_dmg(void) { return WEAPON_DMG[fh.weapon_lv]; }

/* ---- routes ----------------------------------------------------------------- */

int fh_foe_pos_at(int r, int32_t dist, int32_t *x, int32_t *y) {
    for (int i = 0; i + 1 < fh.rn[r]; i++) {
        int32_t ax = fh.rx[r][i], ay = fh.ry[r][i], bx = fh.rx[r][i + 1], by = fh.ry[r][i + 1];
        int32_t len = iabs(bx - ax) + iabs(by - ay);
        if (dist <= len || i + 2 == fh.rn[r]) {
            int32_t d = imin(dist, len);
            *x = ax + isign(bx - ax) * d;
            *y = ay + isign(by - ay) * d;
            return i;
        }
        dist -= len;
    }
    *x = fh.rx[r][0];
    *y = fh.ry[r][0];
    return 0;
}

void fh_foe_pos(const FhFoe *f, int *x, int *y) {
    *x = f->x / FH_U;
    *y = f->y / FH_U;
}

static int32_t cave_cx(void) { return fh.cave_x * FH_TU + FH_TU / 2; }
static int32_t cave_cy(void) { return fh.cave_y * FH_TU + FH_TU / 2; }

/* how far a foe still has to go to the cave (targets pick the smallest) */
static int32_t foe_left(const FhFoe *f) {
    if (FH_FOE[f->kind].flies) {
        int64_t dx = cave_cx() - f->x, dy = cave_cy() - f->y;
        return isqrt32(dx * dx + dy * dy);
    }
    return fh.rlen[f->route] - f->dist;
}

/* ---- setting up a stage ----------------------------------------------------- */

static int kind_of_letter(char c) {
    static const char L[] = "nrfcgqMJPGS";
    const char *p = strchr(L, c);
    return p && c ? (int)(p - L) : -1;
}

static void parse_waves(const char *s) {
    fh.n_waves = 0;
    memset(fh.waves, 0, sizeof fh.waves);
    FhWave *w = &fh.waves[0];
    int gi = 0;
    while (*s) {
        if (*s == '|') {
            if (fh.n_waves < FH_MAX_WAVES - 1) { fh.n_waves++; w = &fh.waves[fh.n_waves]; gi = 0; }
            s++;
            continue;
        }
        if (*s < '0' || *s > '9') { s++; continue; }
        int n = 0;
        while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
        int k = kind_of_letter(*s);
        if (k < 0) { s++; continue; }
        s++;
        int route = gi % imax(1, fh.n_routes);
        if (*s == '@') { route = s[1] - '0'; s += 2; }
        else if (*s == '*') { route = -1; s++; }
        if (w->n < FH_MAX_GROUPS) w->g[w->n++] = (FhGroup){(int8_t)k, (int8_t)route, (int8_t)n};
        gi++;
    }
    fh.n_waves++;
}

static void pim_home(void) {
    /* Pim comes back on the first open tile by the cave, off the path if she can */
    static const int ORDER[4] = {FH_DOWN, FH_LEFT, FH_RIGHT, FH_UP};
    int bx = -1, by = -1, bd = FH_DOWN;
    for (int pass = 0; pass < 2 && bx < 0; pass++)
        for (int k = 0; k < 4; k++) {
            int d = ORDER[k], x = fh.cave_x + FH_DX[d], y = fh.cave_y + FH_DY[d];
            if (!fh_walkable(x, y) || (pass == 0 && fh.tile[y][x] == TL_PATH)) continue;
            bx = x; by = y; bd = d;
            break;
        }
    if (bx < 0) { bx = fh.cave_x; by = imin(fh.cave_y + 1, FH_H - 1); }
    fh.px = bx * FH_TU + FH_TU / 2;
    fh.py = by * FH_TU + FH_TU / 2;
    fh.face = bd;
    fh.aim_dx = FH_DX[bd];
    fh.aim_dy = FH_DY[bd];
}

void fh_sim_start(int level) {
    memset(&fh, 0, sizeof fh);
    fh.level = level;
    const FhStage *st = &FH_STAGE[level];
    for (int y = 0; y < FH_H; y++)
        for (int x = 0; x < FH_W; x++) {
            char c = st->rows[y][x];
            int t = TL_GRASS;
            switch (c) {
            case ',': t = TL_FLOWER; break;
            case '#': t = TL_THICKET; break;
            case 'b': t = TL_BUSH; break;
            case 'r': t = TL_ROCK; break;
            case '~': t = TL_WATER; break;
            case 'C': t = TL_CAVE; fh.cave_x = x; fh.cave_y = y; break;
            case 'h': t = TL_HUT; break;
            default: break;
            }
            if (c >= '1' && c <= '4') { fh.obj[y][x] = O_NPC; fh.oarg[y][x] = (uint8_t)(c - '1'); }
            fh.tile[y][x] = (uint8_t)t;
        }
    /* lay the routes: every tile between two corners is path */
    fh.n_routes = st->n_routes;
    for (int r = 0; r < st->n_routes; r++) {
        const FhRoute *ro = &st->route[r];
        fh.rn[r] = ro->n;
        fh.rlen[r] = 0;
        for (int i = 0; i < ro->n; i++) {
            fh.rx[r][i] = ro->x[i] * FH_TU + FH_TU / 2;
            fh.ry[r][i] = ro->y[i] * FH_TU + FH_TU / 2;
            if (i == 0) continue;
            int x0 = ro->x[i - 1], y0 = ro->y[i - 1], x1 = ro->x[i], y1 = ro->y[i];
            fh.rlen[r] += (iabs(x1 - x0) + iabs(y1 - y0)) * FH_TU;
            for (int x = x0, y = y0;; x += isign(x1 - x0), y += isign(y1 - y0)) {
                if (in_field(x, y) && fh.tile[y][x] != TL_CAVE) fh.tile[y][x] = TL_PATH;
                if (x == x1 && y == y1) break;
            }
        }
    }
    /* spawn points: the different starts */
    fh.n_spawns = 0;
    for (int r = 0; r < fh.n_routes; r++) {
        bool dup = false;
        for (int q = 0; q < r; q++) dup |= st->route[q].x[0] == st->route[r].x[0] && st->route[q].y[0] == st->route[r].y[0];
        if (!dup) fh.n_spawns++;
    }
    for (int y = 0; y < FH_H; y++)
        for (int x = 0; x < FH_W; x++) {
            bool np = fh.tile[y][x] == TL_PATH || fh.tile[y][x] == TL_CAVE;
            for (int d = 0; d < 4 && !np; d++) {
                int nx = x + FH_DX[d], ny = y + FH_DY[d];
                np = in_field(nx, ny) && (fh.tile[ny][nx] == TL_PATH || fh.tile[ny][nx] == TL_CAVE);
            }
            fh.near_path[y][x] = np;
        }
    parse_waves(st->waves);
    fh.phase = PH_BUILD;
    fh.meat = FH_START_MEAT;
    fh.cave_hp = FH_CAVE_HP;
    fh.talk_npc = -1;
    pim_home();
}

/* ---- the A menu ------------------------------------------------------------- */

int fh_faced(int *tx, int *ty) {
    int cx = fh.px / FH_U + FH_DX[fh.face] * 12, cy = fh.py / FH_U + FH_DY[fh.face] * 12;
    if (cx < 0 || cy < 0) return 0;
    *tx = cx / FH_T;
    *ty = cy / FH_T;
    return in_field(*tx, *ty);
}

int fh_sell_value(int x, int y) {
    switch (fh.obj[y][x]) {
    case O_UNIT: return fh.unit[fh.oarg[y][x]].spent / 2;
    case O_HEN: return fh.oarg[y][x] >= 2 ? HEN_COOKED : HEN_SELL;
    case O_FIRE: return FIRE_COST / 2;
    default: return 0;
    }
}

static void menu_add(int act, int arg, int cost) {
    if (fh.menu_n >= FH_MENU_MAX) return;
    fh.menu[fh.menu_n++] = (FhMenuItem){(uint8_t)act, (uint8_t)arg, (int16_t)cost, (uint8_t)(cost <= fh.meat)};
}

void fh_menu_build(int tx, int ty) {
    fh.menu_n = 0;
    fh.menu_tx = tx;
    fh.menu_ty = ty;
    bool build = fh.phase == PH_BUILD;
    /* facing the edge of the field, or anything that offers nothing else:
     * start the next wave from right here, or leave the stage */
    if (!in_field(tx, ty)) {
        if (build) menu_add(M_FIGHT, 0, 0);
        menu_add(M_LEAVE, 0, 0);
        return;
    }
    int t = fh.tile[ty][tx], o = fh.obj[ty][tx];
    if (t == TL_CAVE) {
        if (build) menu_add(M_FIGHT, 0, 0);
        if (fh.throw_lv < 2) menu_add(M_THROW_UP, 0, THROW_COST[fh.throw_lv + 1]);
        if (fh.weapon_lv < 2) menu_add(M_WEAPON_UP, 0, WEAPON_COST[fh.weapon_lv + 1]);
        /* Pim's own upgrades only while no wave is running */
        for (int i = 0; i < fh.menu_n; i++)
            if (!build && fh.menu[i].act != M_FIGHT) fh.menu[i].ok = 0;
        menu_add(M_LEAVE, 0, 0);
        return;
    }
    if (t == TL_BUSH) { menu_add(M_DIG, 0, DIG_BUSH); return; }
    if (t == TL_ROCK) { menu_add(M_DIG, 0, DIG_ROCK); return; }
    if (o == O_UNIT) {
        const FhUnit *u = &fh.unit[fh.oarg[ty][tx]];
        for (int k = 0; k < U_KINDS; k++)
            if (FH_UNIT[k].from == u->kind) menu_add(M_UPGRADE, k, FH_UNIT[k].cost);
        menu_add(M_SELL, 0, 0);
        return;
    }
    if (o == O_HEN || o == O_FIRE) { menu_add(M_SELL, 0, 0); return; }
    if (o == O_NPC) return;
    if (fh_buildable(tx, ty)) {
        menu_add(M_THROWER, 0, FH_UNIT[U_THROWER].cost);
        menu_add(M_HEN, 0, HEN_COST);
        menu_add(M_FIRE, 0, FIRE_COST);
    }
    if (build) menu_add(M_FIGHT, 0, 0);
    menu_add(M_LEAVE, 0, 0);
}

const char *fh_menu_label(const FhMenuItem *m, char *buf, int n) {
    switch (m->act) {
    case M_THROWER: snprintf(buf, (size_t)n, "THROWER"); break;
    case M_HEN: snprintf(buf, (size_t)n, "HEN"); break;
    case M_FIRE: snprintf(buf, (size_t)n, "FIRE PIT"); break;
    case M_DIG: snprintf(buf, (size_t)n, "DIG OUT"); break;
    case M_UPGRADE: snprintf(buf, (size_t)n, "%s", FH_UNIT[m->arg].name); break;
    case M_SELL: snprintf(buf, (size_t)n, "SELL"); break;
    case M_FIGHT: snprintf(buf, (size_t)n, "SOUND THE HORN"); break;
    case M_THROW_UP: snprintf(buf, (size_t)n, fh.throw_lv == 0 ? "QUICK ARM" : "QUICKER ARM"); break;
    case M_WEAPON_UP: snprintf(buf, (size_t)n, fh.weapon_lv == 0 ? "STONE AXE" : "FIRE AXE"); break;
    case M_LEAVE: snprintf(buf, (size_t)n, "LEAVE STAGE"); break;
    default: snprintf(buf, (size_t)n, "-"); break;
    }
    return buf;
}

static int new_unit(int x, int y, int kind, int spent) {
    for (int i = 0; i < FH_MAX_UNITS; i++)
        if (!fh.unit[i].on) {
            fh.unit[i] = (FhUnit){1, (uint8_t)kind, (uint8_t)x, (uint8_t)y, 0, (int16_t)spent, 0, 0, 99};
            fh.obj[y][x] = O_UNIT;
            fh.oarg[y][x] = (uint8_t)i;
            return i;
        }
    return -1;
}

static void note_counts(void) {
    fh.most_units = imax(fh.most_units, fh_unit_count());
    fh.most_hens = imax(fh.most_hens, fh_hen_count());
}

int fh_menu_do(int i) {
    if (i < 0 || i >= fh.menu_n) return 0;
    FhMenuItem m = fh.menu[i];
    int x = fh.menu_tx, y = fh.menu_ty, px = x * FH_T + 8, py = y * FH_T + 8;
    if (!m.ok || m.cost > fh.meat) { ev(EV_NOPE, px, py, 0); return 0; }
    switch (m.act) {
    case M_THROWER:
        if (!fh_buildable(x, y) || new_unit(x, y, U_THROWER, m.cost) < 0) { ev(EV_NOPE, px, py, 0); return 0; }
        fh.meat -= m.cost;
        ev(EV_BUILD, px, py, O_UNIT);
        break;
    case M_HEN:
        if (!fh_buildable(x, y)) { ev(EV_NOPE, px, py, 0); return 0; }
        fh.obj[y][x] = O_HEN;
        fh.oarg[y][x] = 0;
        fh.meat -= m.cost;
        ev(EV_BUILD, px, py, O_HEN);
        break;
    case M_FIRE:
        if (!fh_buildable(x, y)) { ev(EV_NOPE, px, py, 0); return 0; }
        fh.obj[y][x] = O_FIRE;
        fh.meat -= m.cost;
        ev(EV_BUILD, px, py, O_FIRE);
        break;
    case M_DIG:
        fh.tile[y][x] = TL_GRASS;
        fh.meat -= m.cost;
        ev(EV_DIG, px, py, 0);
        break;
    case M_UPGRADE: {
        FhUnit *u = &fh.unit[fh.oarg[y][x]];
        u->kind = m.arg;
        u->spent = (int16_t)(u->spent + m.cost);
        u->cool = imin(u->cool, 20);
        fh.meat -= m.cost;
        fh.spent_upgrades += m.cost;
        ev(EV_UPGRADE, px, py, m.arg);
        break;
    }
    case M_SELL: {
        int v = fh_sell_value(x, y);
        if (fh.obj[y][x] == O_UNIT) fh.unit[fh.oarg[y][x]].on = 0;
        fh.obj[y][x] = O_NONE;
        fh.oarg[y][x] = 0;
        add_meat(v);
        ev(EV_SELL, px, py, v);
        break;
    }
    case M_FIGHT:
        if (fh.phase != PH_BUILD) return 0;
        fh.phase = PH_BATTLE;
        fh.phase_t = 0;
        fh.grp = 0;
        fh.grp_left = fh.waves[fh.wave].n ? fh.waves[fh.wave].g[0].count : 0;
        fh.spawn_t = 30;
        break;
    case M_THROW_UP:
        fh.throw_lv++;
        fh.meat -= m.cost;
        fh.spent_upgrades += m.cost;
        ev(EV_UPGRADE, px, py, -1);
        break;
    case M_WEAPON_UP:
        fh.weapon_lv++;
        fh.meat -= m.cost;
        fh.spent_upgrades += m.cost;
        ev(EV_UPGRADE, px, py, -2);
        break;
    case M_LEAVE:
        fh.leave = 1;
        break;
    default: return 0;
    }
    note_counts();
    return 1;
}

/* ---- foes ------------------------------------------------------------------- */

int fh_add_foe(int kind, int route) {
    for (int i = 0; i < FH_MAX_FOES; i++) {
        FhFoe *f = &fh.foe[i];
        if (f->on) continue;
        memset(f, 0, sizeof *f);
        f->on = 1;
        f->kind = (uint8_t)kind;
        f->route = (uint8_t)iclamp(route, 0, imax(0, fh.n_routes - 1));
        f->hp = FH_FOE[kind].hp * 10;
        f->x = fh.rx[f->route][0];
        f->y = fh.ry[f->route][0];
        f->fx0 = f->x;
        f->fy0 = f->y;
        f->face = 1;
        ev(EV_SPAWN, f->x / FH_U, f->y / FH_U, kind);
        return i;
    }
    return -1;
}

static void kill_foe(FhFoe *f) {
    f->on = 0;
    fh.kills++;
    add_meat(FH_FOE[f->kind].meat);
    ev(EV_KILL, f->x / FH_U, f->y / FH_U, f->kind);
}

/* dmg in tenths, before armour */
static void hurt(FhFoe *f, int32_t dmg, int dtype) {
    if (!f->on) return;
    const FhFoeDef *d = &FH_FOE[f->kind];
    int res = dtype == D_NORMAL ? d->res_normal : dtype == D_FIRE ? d->res_fire : dtype == D_ARROW ? d->res_arrow : 0;
    int32_t got = dmg * (100 - res) / 100;
    if (dtype == D_FIRE && f->tar_t > 0) got = got * 12 / 10; /* tar: +20 % fire damage */
    if (got <= 0) { f->flash_t = 3; return; }
    f->hp -= got;
    f->flash_t = 4;
    if (f->hp <= 0) kill_foe(f);
}

static void pim_hit(void) {
    if (fh.zstate != Z_OK) return;
    fh.zstate = Z_STUN;
    fh.zt = PIM_STUN;
    fh.throwing = 0;
    fh.menu_open = 0;
    fh.talk_npc = -1;
    if (!fh.god) { fh.cave_hp--; fh.cave_hits++; }
    ev(EV_PIM_HIT, fh.px / FH_U, fh.py / FH_U, 0);
}

static void move_foes(void) {
    for (int i = 0; i < FH_MAX_FOES; i++) {
        FhFoe *f = &fh.foe[i];
        if (!f->on) continue;
        const FhFoeDef *d = &FH_FOE[f->kind];
        int spd4 = d->speed;
        if (f->slow_t > 0 || f->tar_t > 0) spd4 /= 2;
        /* to 1/16 pixel, keeping the remainder */
        f->sub += spd4;
        int spd = f->sub / 4;
        f->sub %= 4;
        if (f->slow_t > 0) f->slow_t--;
        if (f->tar_t > 0) f->tar_t--;
        if (f->flash_t > 0) f->flash_t--;
        int32_t ox = f->x;
        bool arrived = false;
        if (fh.hold_foes) continue;
        if (d->flies) {
            int32_t tx = cave_cx(), ty = cave_cy();
            if (f->kind == E_GNAT && fh.zstate == Z_OK) {
                int64_t zx = fh.px - f->x, zy = fh.py - f->y;
                if (zx * zx + zy * zy < (int64_t)(GNAT_SEES * FH_U) * (GNAT_SEES * FH_U)) { tx = fh.px; ty = fh.py; }
            }
            int64_t dx = tx - f->x, dy = ty - f->y;
            int dist = isqrt32(dx * dx + dy * dy);
            if (spd == 0) {
                /* no step this frame */
            } else if (dist <= spd) {
                f->x = tx;
                f->y = ty;
            } else {
                /* carry the remainders, or slow fliers would lose their slant */
                int64_t nx = dx * spd + f->remx, ny = dy * spd + f->remy;
                f->x += (int32_t)(nx / dist);
                f->y += (int32_t)(ny / dist);
                f->remx = (int32_t)(nx % dist);
                f->remy = (int32_t)(ny % dist);
            }
            f->dist += spd;
            int64_t cx = cave_cx() - f->x, cy = cave_cy() - f->y;
            arrived = cx * cx + cy * cy <= (int64_t)(6 * FH_U) * (6 * FH_U);
        } else {
            f->dist += spd;
            fh_foe_pos_at(f->route, f->dist, &f->x, &f->y);
            arrived = f->dist >= fh.rlen[f->route];
        }
        if (f->x != ox) f->face = f->x > ox ? 1 : -1;
        if (arrived) {
            f->on = 0;
            if (!fh.god) {
                fh.cave_hp -= d->cave;
                fh.cave_hits += d->cave;
            }
            ev(EV_CAVE, cave_cx() / FH_U, cave_cy() / FH_U, d->cave);
            continue;
        }
        /* any beast that touches Pim knocks her down */
        if (fh.zstate == Z_OK) {
            int32_t dx = iabs(fh.px - f->x), dy = iabs(fh.py - f->y);
            int reach = (TOUCH + (d->boss ? 6 : 0)) * FH_U;
            if (dx < reach && dy < reach) pim_hit();
        }
    }
}

/* ---- waves ------------------------------------------------------------------- */

static int gap_for(int kind) {
    if (FH_FOE[kind].boss) return GAP_BOSS;
    return kind == E_FANGCAT ? GAP_CAT : GAP_FOE;
}

static void wave_end(void) {
    fh.wave++;
    fh.phase_t = 0;
    if (fh.wave >= fh.n_waves) {
        fh.phase = PH_WON;
        ev(EV_WON, 0, 0, 0);
        return;
    }
    /* the pay-out: the spawn points' meat ticks in first; spending while it
     * does is fine, and what won't fit is lost */
    fh.phase = PH_PAYOUT;
    fh.payout_t = 0;
    fh.payout_left = FH_WAVE_MEAT * fh.n_spawns;
    ev(EV_WAVE_END, 0, 0, fh.wave);
}

static void payout(void) {
    fh.payout_t++;
    if (fh.payout_left > 0) { add_meat(1); fh.payout_left--; }
    /* half way: every hen not yet cooked lays 5, even one put down just now */
    if (fh.payout_t == PAYOUT_HENS)
        for (int y = 0; y < FH_H; y++)
            for (int x = 0; x < FH_W; x++)
                if (fh.obj[y][x] == O_HEN && fh.oarg[y][x] < 2) fh.payout_left += HEN_WAVE;
    if (fh.payout_t < PAYOUT_TIME) return;
    add_meat(fh.payout_left);
    fh.payout_left = 0;
    /* then the cooking: a fire pit beside a hen cooks it over two waves, two
     * or more pits in one */
    int cooked = 0;
    for (int y = 0; y < FH_H; y++)
        for (int x = 0; x < FH_W; x++) {
            if (fh.obj[y][x] != O_HEN || fh.oarg[y][x] >= 2) continue;
            int f = fh_fires_near(x, y);
            if (f >= 2) fh.oarg[y][x] = 2;
            else if (f == 1) fh.oarg[y][x]++;
            if (f) cooked++;
        }
    if (cooked) ev(EV_COOK, 0, 0, cooked);
    fh.phase = PH_BUILD;
    fh.phase_t = 0;
}

static void run_wave(void) {
    const FhWave *w = &fh.waves[fh.wave];
    if (fh.grp < w->n) {
        if (--fh.spawn_t <= 0) {
            const FhGroup *g = &w->g[fh.grp];
            if (!fh.no_spawn) {
                if (g->route < 0)
                    for (int r = 0; r < fh.n_routes; r++) fh_add_foe(g->kind, r);
                else fh_add_foe(g->kind, g->route);
            }
            fh.spawn_t = gap_for(g->kind);
            if (--fh.grp_left <= 0) {
                fh.grp++;
                if (fh.grp < w->n) {
                    fh.grp_left = w->g[fh.grp].count;
                    fh.spawn_t = imax(fh.spawn_t, GAP_GROUP);
                }
            }
        }
        return;
    }
    if (fh_foe_count() == 0) wave_end();
}

int fh_next_wave_kinds(uint8_t kinds[E_KINDS]) {
    memset(kinds, 0, E_KINDS);
    if (fh.wave >= fh.n_waves) return 0;
    int n = 0;
    const FhWave *w = &fh.waves[fh.wave];
    for (int i = 0; i < w->n; i++) {
        if (!kinds[w->g[i].kind]) n++;
        kinds[w->g[i].kind] = 1;
    }
    return n;
}

/* ---- towers and shots ------------------------------------------------------- */

static int find_target(int x, int y, int range, bool ground_only) {
    int32_t cx = x * FH_TU + FH_TU / 2, cy = y * FH_TU + FH_TU / 2;
    int64_t r = (int64_t)(range * FH_T + 8) * FH_U;
    int best = -1;
    int32_t best_left = INT32_MAX;
    for (int i = 0; i < FH_MAX_FOES; i++) {
        const FhFoe *f = &fh.foe[i];
        if (!f->on || (ground_only && FH_FOE[f->kind].flies)) continue;
        int64_t dx = f->x - cx, dy = f->y - cy;
        if (dx * dx + dy * dy > r * r) continue;
        int32_t left = foe_left(f);
        if (left < best_left) { best_left = left; best = i; }
    }
    return best;
}

static FhShot *new_shot(void) {
    for (int i = 0; i < FH_MAX_SHOTS; i++)
        if (!fh.shot[i].on) { memset(&fh.shot[i], 0, sizeof fh.shot[i]); fh.shot[i].on = 1; return &fh.shot[i]; }
    return NULL;
}

static void aim(FhShot *s, int32_t tx, int32_t ty, int spd) {
    int64_t dx = tx - s->x, dy = ty - s->y;
    int d = isqrt32(dx * dx + dy * dy);
    if (d <= 0) return;
    s->vx = (int32_t)(dx * spd / d);
    s->vy = (int32_t)(dy * spd / d);
}

static const uint8_t SHOT_OF[U_KINDS] = {SH_BONE, SH_SPEAR, SH_BARB, SH_ARROW, SH_ROCK, SH_ROCK, SH_ROCK, SH_FIRE, SH_EMBER, SH_TAR};

static void towers(void) {
    for (int i = 0; i < FH_MAX_UNITS; i++) {
        FhUnit *u = &fh.unit[i];
        if (!u->on) continue;
        if (u->throw_t < 99) u->throw_t++;
        if (u->cool > 0) { u->cool--; continue; }
        const FhUnitDef *d = &FH_UNIT[u->kind];
        int t = find_target(u->x, u->y, d->range, u->kind == U_BOULDER);
        if (t < 0) continue;
        const FhFoe *f = &fh.foe[t];
        u->cool = (int16_t)d->every;
        u->aim_x = (int16_t)(f->x / FH_U);
        u->aim_y = (int16_t)(f->y / FH_U);
        u->throw_t = 0;
        int pct = fire_pct(u->x, u->y);
        int32_t ux = u->x * FH_TU + FH_TU / 2, uy = u->y * FH_TU + FH_TU / 2;
        if (u->kind == U_BOULDER) {
            for (int k = 0; k < FH_MAX_ROLLS; k++) {
                FhRoll *r = &fh.roll[k];
                if (r->on) continue;
                memset(r, 0, sizeof *r);
                r->on = 1;
                r->x = ux;
                r->y = uy;
                int64_t dx = f->x - ux, dy = f->y - uy;
                int dd = imax(1, isqrt32(dx * dx + dy * dy));
                r->vx = (int32_t)(dx * d->shot_speed / dd);
                r->vy = (int32_t)(dy * d->shot_speed / dd);
                r->left = ROLL_PATH;
                r->budget = ROLL_BUDGET;
                r->dmg = d->dmg * 10 * pct / 100;
                break;
            }
            ev(EV_THROW, u->x, u->y, u->kind);
            continue;
        }
        /* thrown straight at where the beast is now: a quick one can be missed */
        FhShot *s = new_shot();
        if (!s) continue;
        s->kind = SHOT_OF[u->kind];
        s->dtype = (uint8_t)d->dtype;
        s->target = -1;
        s->x = ux;
        s->y = uy;
        s->dmg = d->dmg * 10 * pct / 100;
        /* it flies its reach and a tile more, then drops */
        s->life = (int16_t)(((d->range + 1) * FH_T + 8) * FH_U / d->shot_speed);
        if (u->kind == U_BARB) s->slow = (int16_t)(SLOW_BARB * pct / 100);
        if (u->kind == U_PITCH) s->slow = (int16_t)(SLOW_TAR * pct / 100);
        aim(s, f->x, f->y, d->shot_speed);
        fh.tower_throws++;
        ev(EV_THROW, u->x, u->y, u->kind);
    }
}

static void shot_hits(FhShot *s, FhFoe *f, int fi) {
    if (s->kind != SH_PIM && !s->follow) fh.tower_hits++;
    hurt(f, s->dmg, s->dtype);
    if (s->kind == SH_BARB && f->on) f->slow_t = (int16_t)imax(f->slow_t, s->slow);
    if (s->kind == SH_TAR && f->on && !f->tarred) { f->tarred = 1; f->tar_t = s->slow; }
    if (s->kind == SH_ARROW) {
        /* an arrow carries on a moment into whatever is behind */
        if (s->hit_n < 8) s->hits[s->hit_n++] = (uint8_t)fi;
        if (!s->follow) s->follow = ARROW_FOLLOW;
        return;
    }
    ev(EV_HIT, s->x / FH_U, s->y / FH_U, s->kind);
    s->on = 0;
}

static void move_shots(void) {
    for (int i = 0; i < FH_MAX_SHOTS; i++) {
        FhShot *s = &fh.shot[i];
        if (!s->on) continue;
        if (--s->life <= 0) { s->on = 0; continue; }
        if (s->follow && --s->follow == 0) { s->on = 0; continue; }
        s->x += s->vx;
        s->y += s->vy;
        int tx = s->x / FH_TU, ty = s->y / FH_TU;
        if (s->x < 0 || s->y < 0 || !in_field(tx, ty)) { s->on = 0; continue; }
        if (s->kind != SH_ARROW && fh_blocks_shot(tx, ty)) { s->on = 0; ev(EV_HIT, s->x / FH_U, s->y / FH_U, s->kind); continue; }
        /* every shot flies straight and hits the first beast it meets */
        for (int k = 0; k < FH_MAX_FOES && s->on; k++) {
            FhFoe *f = &fh.foe[k];
            if (!f->on) continue;
            int reach = ((s->kind == SH_PIM ? 8 : 6) + (FH_FOE[f->kind].boss ? 6 : 0)) * FH_U;
            if (iabs(f->x - s->x) >= reach || iabs(f->y - s->y) >= reach) continue;
            bool seen = false;
            for (int h = 0; h < s->hit_n; h++) seen |= s->hits[h] == k;
            if (seen) continue;
            shot_hits(s, f, k);
        }
    }
    for (int i = 0; i < FH_MAX_ROLLS; i++) {
        FhRoll *r = &fh.roll[i];
        if (!r->on) continue;
        r->x += r->vx;
        r->y += r->vy;
        int tx = r->x / FH_TU, ty = r->y / FH_TU;
        if (r->x < 0 || r->y < 0 || !in_field(tx, ty) || fh_blocks_shot(tx, ty) || fh.tile[ty][tx] == TL_WATER) { r->on = 0; continue; }
        int step = imax(iabs(r->vx), iabs(r->vy));
        if (fh.tile[ty][tx] == TL_PATH) r->on_path = 1;
        if (r->on_path) r->left -= step;
        r->budget -= step;
        if (r->left <= 0 || r->budget <= 0) { r->on = 0; continue; }
        /* it crushes every walker it rolls over, every frame */
        for (int k = 0; k < FH_MAX_FOES; k++) {
            FhFoe *f = &fh.foe[k];
            if (!f->on || FH_FOE[f->kind].flies) continue;
            if (iabs(f->x - r->x) < 13 * FH_U && iabs(f->y - r->y) < 13 * FH_U) hurt(f, r->dmg, D_NORMAL);
        }
    }
}

/* ---- Pim -------------------------------------------------------------------- */

static bool box_free(int32_t x, int32_t y) {
    int x0 = (x / FH_U - PIM_R), x1 = (x / FH_U + PIM_R - 1), y0 = (y / FH_U - PIM_R), y1 = (y / FH_U + PIM_R - 1);
    if (x0 < 0 || y0 < 0 || x1 >= FH_W * FH_T || y1 >= FH_H * FH_T) return false;
    return fh_walkable(x0 / FH_T, y0 / FH_T) && fh_walkable(x1 / FH_T, y0 / FH_T) &&
           fh_walkable(x0 / FH_T, y1 / FH_T) && fh_walkable(x1 / FH_T, y1 / FH_T);
}

static void pim_throw(void) {
    FhShot *s = new_shot();
    if (!s) return;
    int dx = fh.aim_dx, dy = fh.aim_dy;
    if (!dx && !dy) { dx = FH_DX[fh.face]; dy = FH_DY[fh.face]; }
    int spd = dx && dy ? PIM_DIAG : PIM_SHOT;
    s->kind = SH_PIM;
    s->dtype = fh.weapon_lv == 2 ? D_PIERCE : D_NORMAL;
    s->target = -1;
    s->x = fh.px + dx * 6 * FH_U;
    s->y = fh.py + dy * 6 * FH_U - (dy == 0 ? 2 * FH_U : 0);
    s->vx = dx * spd;
    s->vy = dy * spd;
    s->dmg = WEAPON_DMG[fh.weapon_lv] * 10;
    s->life = (int16_t)((THROW_RANGE[fh.throw_lv] * FH_T + 4) * FH_U / PIM_SHOT);
    s->slow = (int16_t)fh.weapon_lv; /* for drawing: bone, axe, fire axe */
    ev(EV_THROW, fh.px / FH_U, fh.py / FH_U, -1);
}

static void pim_update(uint32_t held, uint32_t pressed) {
    if (fh.throw_cool > 0) fh.throw_cool--;
    if (fh.zstate == Z_STUN) {
        if (--fh.zt <= 0) { fh.zstate = Z_GONE; fh.zt = PIM_GONE; }
        return;
    }
    if (fh.zstate == Z_GONE) {
        if (--fh.zt <= 0) { fh.zstate = Z_OK; pim_home(); }
        return;
    }
    if (fh.talk_npc >= 0) {
        fh.talk_t++;
        if (fh.talk_t > 10 && (pressed & (BTN_A | BTN_B))) fh.talk_npc = -1;
        return;
    }
    if (fh.menu_open) {
        if (pressed & BTN_UP) fh.menu_sel = (fh.menu_sel + fh.menu_n - 1) % fh.menu_n;
        if (pressed & BTN_DOWN) fh.menu_sel = (fh.menu_sel + 1) % fh.menu_n;
        if (pressed & BTN_B) { fh.menu_open = 0; return; }
        if (pressed & BTN_A) {
            if (fh_menu_do(fh.menu_sel)) fh.menu_open = 0;
            else if (fh.menu_n == 0) fh.menu_open = 0;
        }
        /* costs change as meat comes in */
        if (fh.menu_open) {
            int sel = fh.menu_sel;
            fh_menu_build(fh.menu_tx, fh.menu_ty);
            fh.menu_sel = fh.menu_n ? iclamp(sel, 0, fh.menu_n - 1) : 0;
            if (!fh.menu_n) fh.menu_open = 0;
        }
        return;
    }
    /* hold B: throw the way she faces, again and again; she can't walk or turn */
    fh.throwing = (held & BTN_B) != 0;
    if (fh.throwing) {
        if (fh.throw_cool <= 0) { pim_throw(); fh.throw_cool = THROW_EVERY[fh.throw_lv]; }
        return;
    }
    /* A: whatever the faced tile offers */
    if (pressed & BTN_A) {
        int tx, ty;
        if (!fh_faced(&tx, &ty)) tx = ty = -1;
        if (tx >= 0 && fh.obj[ty][tx] == O_NPC) { fh.talk_npc = fh.oarg[ty][tx]; fh.talk_t = 0; return; }
        fh_menu_build(tx, ty);
        if (fh.menu_n) { fh.menu_open = 1; fh.menu_sel = 0; return; }
        ev(EV_NOPE, fh.px / FH_U, fh.py / FH_U, 0);
        return;
    }
    /* walk: eight ways, facing the last way pushed; she throws the way the
     * pad last pointed, diagonals too */
    int dx = ((held & BTN_RIGHT) ? 1 : 0) - ((held & BTN_LEFT) ? 1 : 0);
    int dy = ((held & BTN_DOWN) ? 1 : 0) - ((held & BTN_UP) ? 1 : 0);
    if (dx || dy) { fh.aim_dx = dx; fh.aim_dy = dy; }
    /* a fresh press the other way only turns her, so she can face a tile
     * right next to her without stepping onto it */
    int turned = 0;
    for (int d = 0; d < 4; d++)
        if ((pressed & PAD[d]) && fh.face != d) { fh.face = d; turned = 1; }
    if (turned) { fh.walk_t = 0; return; }
    if (!(held & PAD[fh.face])) {
        for (int d = 0; d < 4; d++)
            if (held & PAD[d]) { fh.face = d; break; }
    }
    if (dx || dy) {
        fh.walk_t++;
        int32_t nx = fh.px + dx * PIM_SPEED;
        if (dx && box_free(nx, fh.py)) fh.px = nx;
        else if (dx && !dy) {
            /* slide round a corner she is nearly past */
            int32_t cy = (fh.py / FH_TU) * FH_TU + FH_TU / 2;
            if (fh.py != cy && box_free(nx, cy)) fh.py += isign(cy - fh.py) * PIM_SPEED / 2;
        }
        int32_t ny = fh.py + dy * PIM_SPEED;
        if (dy && box_free(fh.px, ny)) fh.py = ny;
        else if (dy && !dx) {
            int32_t cx = (fh.px / FH_TU) * FH_TU + FH_TU / 2;
            if (fh.px != cx && box_free(cx, ny)) fh.px += isign(cx - fh.px) * PIM_SPEED / 2;
        }
    } else {
        fh.walk_t = 0;
    }
}

/* ---- one frame --------------------------------------------------------------- */

void fh_sim_step(uint32_t held, uint32_t pressed) {
    fh.n_ev = 0;
    fh.frame++;
    fh.phase_t++;
    if (fh.phase == PH_WON || fh.phase == PH_LOST) return;
    pim_update(held, pressed);
    if (fh.phase == PH_BATTLE) run_wave();
    else if (fh.phase == PH_PAYOUT) payout();
    if (fh.phase == PH_BATTLE || fh.phase == PH_PAYOUT) fh.busy_frames++;
    towers();
    move_shots();
    move_foes();
    note_counts();
    if (fh.cave_hp <= 0) {
        fh.cave_hp = 0;
        fh.phase = PH_LOST;
        fh.menu_open = 0;
        ev(EV_LOST, 0, 0, 0);
    }
}

/* ---- the demo player ----------------------------------------------------------
 * It follows a plan for each stage (flinthold_levels.c): what to build where,
 * wave by wave, and where to stand and throw. It walks there, faces the tile,
 * opens the menu with A and picks the line, all with button presses. Between
 * waves it also sells every cooked hen and puts a new one in its place. */

extern const char *const FH_PLAN[FH_LEVELS];

int fh_bot_step;
static int bot_post_x = -1, bot_post_y = -1, bot_post_d = FH_UP, bot_post_ax, bot_post_ay = -1;
static int hen_x = -1, hen_y = -1;

void fh_bot_reset(void) {
    fh_bot_step = 0;
    bot_post_x = bot_post_y = -1;
    bot_post_d = FH_UP;
    bot_post_ax = 0;
    bot_post_ay = -1;
    hen_x = hen_y = -1;
}

typedef struct { int ok, wave, x, y, act, arg; } Step;

static int unit_letter(char c) {
    static const char L[] = "?sbwlhotzp"; /* thrower, spear, barb, bow, sling, hurler, boulder, torch, blaze, pitch */
    const char *p = strchr(L, c);
    return p && c ? (int)(p - L) : -1;
}

/* the n-th step of the plan: letter, two digits of x, one digit of y, maybe a kind */
static Step plan_step(int n) {
    Step st = {0, 0, 0, 0, 0, 0};
    const char *p = FH_PLAN[fh.level];
    if (!p) return st;
    int k = 0, wave = 0;
    while (*p) {
        if (*p == '|') { wave++; p++; continue; }
        if (*p == ' ') { p++; continue; }
        const char *s = p;
        while (*p && *p != ' ' && *p != '|') p++;
        if (k++ != n) continue;
        st.ok = 1;
        st.wave = wave;
        char c = s[0];
        if (c == 'T' || c == 'W') {
            /* T1 T2: Pim's arm to that level; W1 W2: her weapon */
            st.x = fh.cave_x; st.y = fh.cave_y;
            st.act = c == 'T' ? M_THROW_UP : M_WEAPON_UP;
            st.arg = p - s > 1 ? s[1] - '0' : 1;
            return st;
        }
        if (p - s < 4) { st.ok = 2; return st; }
        st.x = (s[1] - '0') * 10 + (s[2] - '0');
        st.y = s[3] - '0';
        switch (c) {
        case 't': st.act = M_THROWER; break;
        case 'h': st.act = M_HEN; break;
        case 'f': st.act = M_FIRE; break;
        case 'd': st.act = M_DIG; break;
        case 'x': st.act = M_SELL; break;
        case 'u': st.act = M_UPGRADE; st.arg = p - s > 4 ? unit_letter(s[4]) : 0; break;
        case 'P': {
            /* r d l u, or a diagonal: a down-left, b down-right, c up-left, e up-right */
            st.act = -1;
            static const char D[] = "rdluabce";
            const char *q = p - s > 4 ? strchr(D, s[4]) : NULL;
            st.arg = q && s[4] ? (int)(q - D) : FH_UP;
            break;
        }
        default: st.ok = 2; break;
        }
        return st;
    }
    return st;
}

/* is this step already done (or can never be done)? */
static bool step_done(const Step *s) {
    if (s->act == M_THROW_UP) return fh.throw_lv >= s->arg;
    if (s->act == M_WEAPON_UP) return fh.weapon_lv >= s->arg;
    if (!in_field(s->x, s->y)) return true;
    int o = fh.obj[s->y][s->x], t = fh.tile[s->y][s->x];
    switch (s->act) {
    case M_THROWER: return o == O_UNIT || !fh_buildable(s->x, s->y);
    case M_HEN: return o == O_HEN || !fh_buildable(s->x, s->y);
    case M_FIRE: return o == O_FIRE || !fh_buildable(s->x, s->y);
    case M_DIG: return t != TL_BUSH && t != TL_ROCK;
    case M_SELL: return o == O_NONE;
    case M_UPGRADE: {
        if (o != O_UNIT) return true;
        int k = fh.unit[fh.oarg[s->y][s->x]].kind;
        if (k == s->arg) return true;
        /* done if it is already past this kind */
        for (int j = s->arg; j >= 0; j = FH_UNIT[j].from)
            if (j == k) return false;
        return true;
    }
    default: return true;
    }
}

static int step_cost(const Step *s) {
    switch (s->act) {
    case M_THROWER: return FH_UNIT[U_THROWER].cost;
    case M_HEN: return HEN_COST;
    case M_FIRE: return FIRE_COST;
    case M_DIG: return fh.tile[s->y][s->x] == TL_ROCK ? DIG_ROCK : DIG_BUSH;
    case M_UPGRADE: {
        /* the next rung towards the kind wanted */
        int k = fh.unit[fh.oarg[s->y][s->x]].kind, j = s->arg;
        while (FH_UNIT[j].from >= 0 && FH_UNIT[j].from != k) j = FH_UNIT[j].from;
        return FH_UNIT[j].cost;
    }
    case M_THROW_UP: return fh.throw_lv < 2 ? THROW_COST[fh.throw_lv + 1] : 999;
    case M_WEAPON_UP: return fh.weapon_lv < 2 ? WEAPON_COST[fh.weapon_lv + 1] : 999;
    default: return 0;
    }
}

/* the next rung of an upgrade chain (for the menu line to pick) */
static int step_arg(const Step *s) {
    if (s->act != M_UPGRADE) return s->arg;
    int k = fh.unit[fh.oarg[s->y][s->x]].kind, j = s->arg;
    while (FH_UNIT[j].from >= 0 && FH_UNIT[j].from != k) j = FH_UNIT[j].from;
    return j;
}

/* walk towards a tile beside (tx, ty) and face it; 1 once there */
static int bot_go_face(int tx, int ty, int *btns) {
    int px = fh.px / FH_TU, py = fh.py / FH_TU;
    /* already beside it and facing it? */
    int fx, fy;
    if (fh_faced(&fx, &fy) && fx == tx && fy == ty) {
        int cx = px * FH_TU + FH_TU / 2, cy = py * FH_TU + FH_TU / 2;
        if (iabs(fh.px - cx) <= FH_U * 2 && iabs(fh.py - cy) <= FH_U * 2) return 1;
    }
    /* breadth-first over walkable tiles to any tile next to the target */
    static int16_t prev[FH_H * FH_W];
    static int16_t q[FH_H * FH_W];
    for (int i = 0; i < FH_H * FH_W; i++) prev[i] = -2;
    int qh = 0, qt = 0, start = py * FH_W + px, goal = -1;
    prev[start] = -1;
    q[qt++] = (int16_t)start;
    while (qh < qt) {
        int c = q[qh++], cx = c % FH_W, cy = c / FH_W;
        if (iabs(cx - tx) + iabs(cy - ty) == 1) { goal = c; break; }
        for (int d = 0; d < 4; d++) {
            int nx = cx + FH_DX[d], ny = cy + FH_DY[d];
            if (!in_field(nx, ny) || prev[ny * FH_W + nx] != -2) continue;
            if (!fh_walkable(nx, ny)) continue;
            prev[ny * FH_W + nx] = (int16_t)c;
            q[qt++] = (int16_t)(ny * FH_W + nx);
        }
    }
    if (goal < 0) return -1;
    /* the first step from where she is */
    int next = goal;
    while (prev[next] != start && prev[next] >= 0) next = prev[next];
    int gx, gy;
    if (goal == start) { gx = px; gy = py; }
    else { gx = next % FH_W; gy = next / FH_W; }
    int32_t cx = gx * FH_TU + FH_TU / 2, cy = gy * FH_TU + FH_TU / 2;
    if (goal == start) {
        /* centre up, then turn to face */
        if (iabs(fh.px - cx) > FH_U * 2) { *btns = fh.px < cx ? BTN_RIGHT : BTN_LEFT; return 0; }
        if (iabs(fh.py - cy) > FH_U * 2) { *btns = fh.py < cy ? BTN_DOWN : BTN_UP; return 0; }
        int d = tx > px ? FH_RIGHT : tx < px ? FH_LEFT : ty > py ? FH_DOWN : FH_UP;
        /* a one-frame tap turns her (it may step her a pixel) */
        *btns = (fh.frame & 1) ? PAD[d] : 0;
        return 0;
    }
    /* line up across the way first, then go */
    if (gx != px) {
        if (iabs(fh.py - cy) > FH_U) { *btns = fh.py < cy ? BTN_DOWN : BTN_UP; return 0; }
        *btns = gx > px ? BTN_RIGHT : BTN_LEFT;
    } else {
        if (iabs(fh.px - cx) > FH_U) { *btns = fh.px < cx ? BTN_RIGHT : BTN_LEFT; return 0; }
        *btns = gy > py ? BTN_DOWN : BTN_UP;
    }
    return 0;
}

/* open the menu on (tx, ty) and pick the line with act/arg; 1 when done */
static int bot_menu(int tx, int ty, int act, int arg, int *btns) {
    if (fh.menu_open) {
        if (fh.menu_tx != tx || fh.menu_ty != ty) { *btns = (fh.frame & 1) ? BTN_B : 0; return 0; }
        int want = -1;
        for (int i = 0; i < fh.menu_n; i++)
            if (fh.menu[i].act == act && (act != M_UPGRADE || fh.menu[i].arg == arg)) want = i;
        if (want < 0) { *btns = (fh.frame & 1) ? BTN_B : 0; return -1; }
        if (fh.frame & 1) { *btns = 0; return 0; }
        if (fh.menu_sel != want) *btns = fh.menu_sel < want ? BTN_DOWN : BTN_UP;
        else *btns = BTN_A;
        return 0;
    }
    int r = bot_go_face(tx, ty, btns);
    if (r < 0) return -1;
    if (r == 1) *btns = (fh.frame & 1) ? BTN_A : 0;
    return 0;
}

/* road tiles within a thrower's reach of (x, y) */
static int cover(int x, int y) {
    int n = 0;
    for (int yy = y - 2; yy <= y + 2; yy++)
        for (int xx = x - 2; xx <= x + 2; xx++) {
            if (!in_field(xx, yy) || fh.tile[yy][xx] != TL_PATH) continue;
            int dx = (xx - x) * FH_T, dy = (yy - y) * FH_T;
            if (dx * dx + dy * dy <= 40 * 40) n++;
        }
    return n;
}

static int bot_extra(int *b) {
    static const int NEXT[U_KINDS] = {U_SPEAR, U_BOW, -1, -1, U_HURLER, -1, -1, U_BLAZE, -1, -1};
    for (int i = 0; i < FH_MAX_UNITS; i++) {
        const FhUnit *u = &fh.unit[i];
        if (!u->on || NEXT[u->kind] < 0 || FH_UNIT[NEXT[u->kind]].cost > fh.meat) continue;
        if (bot_menu(u->x, u->y, M_UPGRADE, NEXT[u->kind], b) >= 0) return 1;
    }
    int bx = -1, by = -1, best = 2;
    for (int y = 0; y < FH_H; y++)
        for (int x = 0; x < FH_W; x++) {
            if (!fh_buildable(x, y) || (x == hen_x && y == hen_y)) continue;
            /* keep a tile beside every hen free to reach it */
            bool hen_side = false;
            for (int d = 0; d < 4; d++) {
                int nx = x + FH_DX[d], ny = y + FH_DY[d];
                if (in_field(nx, ny) && fh.obj[ny][nx] == O_HEN) hen_side = true;
            }
            if (hen_side) continue;
            int c = cover(x, y);
            if (c > best) { best = c; bx = x; by = y; }
        }
    if (bx >= 0 && fh.meat >= FH_UNIT[U_THROWER].cost && bot_menu(bx, by, M_THROWER, 0, b) >= 0) return 1;
    return 0;
}

int fh_bot_buttons(void) {
    int b = 0;
    if (fh.phase == PH_WON || fh.phase == PH_LOST) return 0;
    if (fh.zstate != Z_OK) return 0;
    if (fh.talk_npc >= 0) return (fh.frame & 1) ? BTN_A : 0;
    if (fh.phase == PH_BUILD || fh.phase == PH_PAYOUT) {
        /* a hen just sold goes straight back before anything else is bought */
        if (hen_x >= 0) {
            if (fh.obj[hen_y][hen_x] == O_HEN || !fh_buildable(hen_x, hen_y) || fh.meat < HEN_COST) hen_x = -1;
            else if (bot_menu(hen_x, hen_y, M_HEN, 0, &b) >= 0) return b;
            else hen_x = -1;
        }
        /* the posts in the plan are read as they come */
        for (;;) {
            Step s = plan_step(fh_bot_step);
            if (!s.ok || s.wave > fh.wave) break;
            if (s.ok == 2) { fh_bot_step++; continue; }
            if (s.act == -1) {
                static const int AX[8] = {1, 0, -1, 0, -1, 1, -1, 1}, AY[8] = {0, 1, 0, -1, 1, 1, -1, -1};
                static const int FACE[8] = {FH_RIGHT, FH_DOWN, FH_LEFT, FH_UP, FH_DOWN, FH_DOWN, FH_UP, FH_UP};
                bot_post_x = s.x;
                bot_post_y = s.y;
                bot_post_d = FACE[s.arg];
                bot_post_ax = AX[s.arg];
                bot_post_ay = AY[s.arg];
                fh_bot_step++;
                continue;
            }
            if (step_done(&s)) { fh_bot_step++; continue; }
            if (step_cost(&s) > fh.meat) break; /* not yet: sell a hen, or fight the wave first */
            int r = bot_menu(s.x, s.y, s.act, step_arg(&s), &b);
            if (r < 0) { fh_bot_step++; continue; }
            return b;
        }
        /* cooked hens: sell one (while the meat has room for it) and put a new one down */
        if (fh.meat + HEN_COOKED <= FH_MEAT_MAX)
            for (int y = 0; y < FH_H; y++)
                for (int x = 0; x < FH_W; x++) {
                    if (fh.obj[y][x] != O_HEN || fh.oarg[y][x] < 2) continue;
                    int r = bot_menu(x, y, M_SELL, 0, &b);
                    if (r < 0) continue;
                    hen_x = x;
                    hen_y = y;
                    return b;
                }
        /* the plan is done: keep strengthening (upgrades, then throwers where
         * they cover the most road) while there is meat to spare */
        if (!plan_step(fh_bot_step).ok && fh.meat >= 20) {
            int r = bot_extra(&b);
            if (r > 0) return b;
        }
        /* nothing more to do: sound the horn, from the post if it faces the
         * road (the horn is offered facing the road), else at the cave */
        if (fh.phase == PH_PAYOUT) {
            if (fh.menu_open) return (fh.frame & 1) ? BTN_B : 0;
            return 0;
        }
        int hx = fh.cave_x, hy = fh.cave_y;
        if (bot_post_x >= 0) {
            int tx = bot_post_x + FH_DX[bot_post_d], ty = bot_post_y + FH_DY[bot_post_d];
            if (in_field(tx, ty) && fh.tile[ty][tx] == TL_PATH) { hx = tx; hy = ty; }
        }
        if (fh.menu_open && (fh.menu_tx != hx || fh.menu_ty != hy)) return (fh.frame & 1) ? BTN_B : 0;
        bot_menu(hx, hy, M_FIGHT, 0, &b);
        return b;
    }
    /* battle: go to the post and throw */
    if (fh.menu_open) return (fh.frame & 1) ? BTN_B : 0;
    if (bot_post_x < 0) return BTN_B;
    int px = fh.px / FH_TU, py = fh.py / FH_TU;
    if (px == bot_post_x && py == bot_post_y) {
        int32_t cx = px * FH_TU + FH_TU / 2, cy = py * FH_TU + FH_TU / 2;
        if (fh.aim_dx == bot_post_ax && fh.aim_dy == bot_post_ay && iabs(fh.px - cx) <= FH_U * 3 && iabs(fh.py - cy) <= FH_U * 3) return BTN_B;
    }
    /* walk to the post */
    if (!(px == bot_post_x && py == bot_post_y)) {
        static int16_t prev[FH_H * FH_W];
        static int16_t q[FH_H * FH_W];
        for (int i = 0; i < FH_H * FH_W; i++) prev[i] = -2;
        int qh = 0, qt = 0, start = py * FH_W + px, goal = bot_post_y * FH_W + bot_post_x;
        prev[start] = -1;
        q[qt++] = (int16_t)start;
        while (qh < qt) {
            int c = q[qh++], cx = c % FH_W, cy = c / FH_W;
            if (c == goal) break;
            for (int d = 0; d < 4; d++) {
                int nx = cx + FH_DX[d], ny = cy + FH_DY[d];
                if (!in_field(nx, ny) || prev[ny * FH_W + nx] != -2 || !fh_walkable(nx, ny)) continue;
                prev[ny * FH_W + nx] = (int16_t)c;
                q[qt++] = (int16_t)(ny * FH_W + nx);
            }
        }
        if (prev[goal] == -2) return BTN_B;
        int next = goal;
        while (prev[next] != start && prev[next] >= 0) next = prev[next];
        int gx = next % FH_W, gy = next / FH_W;
        int32_t cx = gx * FH_TU + FH_TU / 2, cy = gy * FH_TU + FH_TU / 2;
        if (gx != px) {
            if (iabs(fh.py - cy) > FH_U) return fh.py < cy ? BTN_DOWN : BTN_UP;
            return gx > px ? BTN_RIGHT : BTN_LEFT;
        }
        if (iabs(fh.px - cx) > FH_U) return fh.px < cx ? BTN_RIGHT : BTN_LEFT;
        return gy > py ? BTN_DOWN : BTN_UP;
    }
    /* on the post: centre, then face the way */
    int32_t cx = px * FH_TU + FH_TU / 2, cy = py * FH_TU + FH_TU / 2;
    if (iabs(fh.px - cx) > FH_U * 2) return fh.px < cx ? BTN_RIGHT : BTN_LEFT;
    if (iabs(fh.py - cy) > FH_U * 2) return fh.py < cy ? BTN_DOWN : BTN_UP;
    /* tap the way to throw (both pads for a diagonal) */
    if (!(fh.frame & 1)) return 0;
    return (bot_post_ax > 0 ? BTN_RIGHT : bot_post_ax < 0 ? BTN_LEFT : 0) | (bot_post_ay > 0 ? BTN_DOWN : bot_post_ay < 0 ? BTN_UP : 0);
}
