/* LIMPET - the map: what every tile does, loading a deck or the hub,
 * checkpoints, movers, doors, switches, the honey and the exit pod.
 *
 * Tiles (one character each):
 *   ' ' '.'   open (the dot is the station's back wall)
 *   #  X  Z   hull, bone and gristle: solid
 *   J L 7 F   ramps, solid in the corner they are named after
 *             (J lower right, L lower left, 7 upper right, F upper left)
 *   M         magnet floor: no warping while on it
 *   = - [ ]   thin plates a warp (and a shot) passes going up, down, right, left
 *   ^         spikes
 *   B         breakable block        O  outline block (solid once shot)
 *   T K       teal and pink swap blocks; S flips which are solid
 *   P         push block
 *   D d       doors, and the switch that opens them all
 *   Y y       a timed door and its switch
 *   C         checkpoint            !  shows the deck's hidden honey
 *   H         where the honey hides  R  the red bulb that clears the exit pod
 *   E         the exit pod           @  the tank's start
 *   W         hopper pad (flings the tank across)
 *   { } < >   two pairs of portals
 *   ?         a survival deck's spawn point
 *   0-6 8 9   marks for the demo player (open)
 *   hub only: $ a pod, N a person, G a gate, V a paint bay, & the honey bridge
 *   foes: t turret, z wall laser, p drone, e ogler, c crawler, s sac, f speck,
 *         w worm, v switch worm, b spiked ball (across), q (up and down), n nozzle */
#include "lmp.h"

const int LMP_DX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
const int LMP_DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

char lmp_tile(int x, int y) {
    if (x < 0 || y < 0 || x >= lg.mw || y >= lg.mh) return '#';
    return lg.map[y][x];
}

int lmp_tri_normal(char c) {
    switch (c) {
    case 'J': return DR_UL;
    case 'L': return DR_UR;
    case '7': return DR_DL;
    case 'F': return DR_DR;
    default: return -1;
    }
}

bool lmp_solid(int x, int y) {
    char c = lmp_tile(x, y);
    switch (c) {
    case '#': case 'X': case 'Z': case 'M': case '=': case '-': case '[': case ']': case '^': case 'B':
    case 'o': case 'S': case 'P': case 'D': case 'd': case 'u': case 'y': case 'C': case 'H': case 'R':
    case 'r': case 'W': case 'm': case 'G': case 'V':
        return true;
    case 'Y': return lg.timed_t <= 0;
    case 'T': return lg.parity == 0;
    case 'K': return lg.parity == 1;
    case 'E': return !lg.bulb_out;
    case '&': return lmp_honey_count() >= LMP_HONEY_TOTAL && !lg.scene_hide;
    default: return false;
    }
}

bool lmp_open(int x, int y) {
    if (x < 0 || y < 0 || x >= lg.mw || y >= lg.mh) return false;
    return !lmp_solid(x, y) && lmp_tri_normal(lmp_tile(x, y)) < 0;
}

bool lmp_face_solid(int x, int y, int dir) {
    char c = lmp_tile(x, y);
    int tn = lmp_tri_normal(c);
    if (tn >= 0) {
        /* a ramp's two straight faces: the ones its open side doesn't touch */
        int dx = LMP_DX[dir], dy = LMP_DY[dir];
        return (dx != 0 && dx == -LMP_DX[tn]) || (dy != 0 && dy == -LMP_DY[tn]);
    }
    return lmp_solid(x, y);
}

bool lmp_is_spikes(int x, int y) { return lmp_tile(x, y) == '^'; }

/* a crusher's face (the demo player keeps off them) */
bool lmp_is_crusher(int x, int y) {
    if (lmp_tile(x, y) != 'm') return false;
    int i = lmp_mover_at(x, y);
    return i >= 0 && lg.mover[i].lethal;
}

bool lmp_warp_pass(int x, int y, int dir) {
    if (x < 0 || y < 0 || x >= lg.mw || y >= lg.mh) return false;
    char c = lmp_tile(x, y);
    if (c == '=') return dir == DR_U;
    if (c == '-') return dir == DR_D;
    if (c == '[') return dir == DR_R;
    if (c == ']') return dir == DR_L;
    if (c == 'O') return true;
    return lmp_open(x, y);
}

bool lmp_shot_blocked(int x, int y) {
    char c = lmp_tile(x, y);
    if (c == 'O') return true;
    if (lmp_tri_normal(c) >= 0) return true;
    return lmp_solid(x, y);
}

int lmp_count_tiles(char c) {
    int n = 0;
    for (int y = 0; y < lg.mh; y++)
        for (int x = 0; x < lg.mw; x++) n += lg.map[y][x] == c;
    return n;
}

void lmp_open_exit(void) {
    if (lg.bulb_out) return;
    lg.bulb_out = 1;
    lmp_sfx("lmp_open");
}

/* whatever is in a cell that just turned solid is crushed */
static void crush_check(void) {
    if (lg.state != GS_PLAY) return;
    int x = lg.tk.x, y = lg.tk.y;
    if (lmp_solid(x, y)) lmp_kill_tank(100);
}

void lmp_shoot_tile(int x, int y) {
    char c = lmp_tile(x, y);
    int px = x * LMP_T + 5, py = y * LMP_T + 5;
    switch (c) {
    case 'B':
        lg.map[y][x] = '.';
        lmp_burst(px, py, C_TAN, 8, 2);
        lmp_sfx("lmp_break");
        break;
    case 'O':
        lg.map[y][x] = 'o';
        lmp_burst(px, py, C_CYAN, 5, 1);
        lmp_sfx("lmp_solid");
        break;
    case 'S':
        lg.parity ^= 1;
        lmp_sfx("lmp_swap");
        crush_check();
        break;
    case 'd':
        lg.map[y][x] = 'u';
        for (int yy = 0; yy < lg.mh; yy++)
            for (int xx = 0; xx < lg.mw; xx++)
                if (lg.map[yy][xx] == 'D') { lg.map[yy][xx] = '.'; lmp_burst(xx * LMP_T + 5, yy * LMP_T + 5, C_GREY, 3, 1); }
        lmp_sfx("lmp_door");
        break;
    case 'y':
        lg.timed_t = 6 * 60;
        lmp_sfx("lmp_door");
        break;
    case 'R':
        lg.map[y][x] = 'r';
        lmp_burst(px, py, C_RED, 14, 2);
        lmp_open_exit();
        break;
    default:
        lmp_sfx("lmp_tink");
        break;
    }
}

/* ---- movers: platforms and crushers ------------------------------------- */

static void mover_paint(const LmpMover *m, char c) {
    for (int y = m->y; y < m->y + m->h; y++)
        for (int x = m->x; x < m->x + m->w; x++)
            if (x >= 0 && y >= 0 && x < lg.mw && y < lg.mh) lg.map[y][x] = c;
}

int lmp_mover_at(int x, int y) {
    for (int i = 0; i < lg.nmover; i++) {
        const LmpMover *m = &lg.mover[i];
        if (x >= m->x && x < m->x + m->w && y >= m->y && y < m->y + m->h) return i;
    }
    return -1;
}

static void mover_step(int i) {
    LmpMover *m = &lg.mover[i];
    int dx = LMP_DX[m->dir] * m->sign, dy = LMP_DY[m->dir] * m->sign;
    /* the cells it moves into must be free (or its own) */
    for (int y = m->y + dy; y < m->y + dy + m->h; y++)
        for (int x = m->x + dx; x < m->x + dx + m->w; x++) {
            if (lmp_mover_at(x, y) == i) continue;
            if (lmp_solid(x, y) || lmp_tri_normal(lmp_tile(x, y)) >= 0) { m->sign = -m->sign; m->wait = 20; return; }
        }
    /* the tank riding it (its surface is part of the mover) goes along */
    LmpTank *t = &lg.tk;
    int sx = t->x - LMP_DX[t->n], sy = t->y - LMP_DY[t->n];
    bool riding = lg.state == GS_PLAY && lmp_axis(t->n) && t->pn == 0 && lmp_mover_at(sx, sy) == i;
    mover_paint(m, '.');
    m->x += dx;
    m->y += dy;
    mover_paint(m, 'm');
    m->pos += m->sign;
    if (m->pos <= 0 || m->pos >= m->dist) { m->sign = -m->sign; m->wait = m->lethal ? (m->pos <= 0 ? 70 : 30) : 30; }
    if (riding) {
        if (lmp_open(t->x + dx, t->y + dy)) { t->x += dx; t->y += dy; lmp_tank_touch(); }
        else lmp_kill_tank(101);
    }
    if (lg.state == GS_PLAY && lmp_mover_at(t->x, t->y) == i) lmp_kill_tank(101);
}

static void movers_update(void) {
    for (int i = 0; i < lg.nmover; i++) {
        LmpMover *m = &lg.mover[i];
        if (m->wait > 0) { m->wait--; continue; }
        if (++m->t >= m->period) {
            m->t = 0;
            mover_step(i);
        }
    }
}

/* ---- checkpoints -------------------------------------------------------- */

void lmp_take_checkpoint(void) {
    LmpCheck *c = &lg.check;
    memcpy(c->map, lg.map, sizeof c->map);
    memcpy(c->mover, lg.mover, sizeof c->mover);
    c->nmover = lg.nmover;
    c->parity = lg.parity;
    c->honey_shown = lg.honey_shown;
    c->x = lg.tk.x;
    c->y = lg.tk.y;
    c->n = lg.tk.n;
    c->set = true;
}

void lmp_restore_checkpoint(void) {
    LmpCheck *c = &lg.check;
    memcpy(lg.map, c->map, sizeof lg.map);
    memcpy(lg.mover, c->mover, sizeof lg.mover);
    lg.nmover = c->nmover;
    lg.parity = c->parity;
    lg.honey_shown = c->honey_shown;
    lg.honey_held = false; /* a death loses the honey; it can be taken again */
    lg.timed_t = 0;
    /* a timed door shut at the checkpoint stays shut; the bulb stays as it was */
    lg.bulb_out = 0;
    for (int y = 0; y < lg.mh; y++)
        for (int x = 0; x < lg.mw; x++)
            if (lg.map[y][x] == 'r') lg.bulb_out = 1;
    if (lg.kind == LK_SURVIVAL || lg.kind == LK_TURNCOAT || lg.kind == LK_DREAD) lg.bulb_out = 0;
    lmp_tank_place(c->x, c->y, c->n);
    /* every foe back where it started */
    memset(lg.foe, 0, sizeof lg.foe);
    memset(lg.es, 0, sizeof lg.es);
    memset(lg.shot, 0, sizeof lg.shot);
    for (int i = 0; i < lg.nspawn; i++) {
        int f = lmp_spawn_foe(lg.spawn[i].kind, lg.spawn[i].x, lg.spawn[i].y, lg.spawn[i].n);
        if (f >= 0) lg.foe[f].spawn = i;
    }
    if (lg.kind == LK_SURVIVAL) { lg.wave = 0; lg.wave_t = 60; }
    if (lg.kind == LK_TURNCOAT || lg.kind == LK_DREAD) lmp_boss_start();
}

/* ---- loading ------------------------------------------------------------ */

static int attach_normal(int x, int y) {
    /* a fixture faces away from the hull it is fixed to: of the sides with
     * hull behind, the one with the longest open run in front */
    static const int ORDER[4] = {DR_U, DR_D, DR_R, DR_L};
    int best = DR_U, best_run = -1;
    for (int k = 0; k < 4; k++) {
        int n = ORDER[k];
        if (!lmp_solid(x - LMP_DX[n], y - LMP_DY[n])) continue;
        int run = 0;
        while (run < 60 && lmp_open(x + (run + 1) * LMP_DX[n], y + (run + 1) * LMP_DY[n])) run++;
        if (run > best_run) { best_run = run; best = n; }
    }
    return best;
}

static int foe_of(char c) {
    switch (c) {
    case 't': return FO_TURRET;
    case 'z': return FO_LASER;
    case 'p': return FO_DRONE;
    case 'e': return FO_EYE;
    case 'c': return FO_CRAWLER;
    case 's': return FO_SAC;
    case 'f': return FO_SPECK;
    case 'w': return FO_WORM;
    case 'v': return FO_SWITCHWORM;
    case 'b': return FO_BALL;
    case 'q': return FO_BALL;
    case 'n': return FO_FLAME;
    default: return FO_NONE;
    }
}

bool lmp_gate_open(int i) {
    const LmpGate *g = &LMP_HUB_GATE[i];
    switch (g->cond) {
    case GATE_CLEARED: return lmp_cleared_count() >= g->need;
    case GATE_TURNCOAT: return lgs.turncoat != 0;
    case GATE_DREAD: return lmp_cleared_count() >= g->need;
    case GATE_HONEY: return lmp_honey_count() >= g->need;
    default: return false;
    }
}

void lmp_load_level(int level) {
    const LmpLevel *L = LMP_LEVEL[level];
    lg.level = level;
    lg.kind = L->kind;
    memset(lg.map, 0, sizeof lg.map);
    lg.mw = lg.mh = 0;
    for (int y = 0; y < LMP_MH && L->rows[y]; y++) {
        int n = (int)strlen(L->rows[y]);
        if (n > LMP_MW) n = LMP_MW;
        memcpy(lg.map[y], L->rows[y], (size_t)n);
        lg.mh = y + 1;
        if (n > lg.mw) lg.mw = n;
    }
    for (int y = 0; y < lg.mh; y++) {
        int n = (int)strlen(lg.map[y]);
        for (int x = n; x < lg.mw; x++) lg.map[y][x] = '#';
    }
    lg.parity = 0;
    lg.honey_x = lg.honey_y = -1;
    lg.honey_shown = lg.honey_held = false;
    lg.bulb_out = 0;
    lg.timed_t = 0;
    lg.timer = 0;
    lg.wave = 0;
    lg.wave_t = 60;
    lg.boss_dead = false;
    lg.boss_t = 0;
    lg.nmover = 0;
    lg.nspawn = 0;
    lg.exit_x = lg.exit_y = -1;
    lg.shake = 0;
    memset(lg.foe, 0, sizeof lg.foe);
    memset(lg.es, 0, sizeof lg.es);
    memset(lg.shot, 0, sizeof lg.shot);
    memset(lg.part, 0, sizeof lg.part);
    int sx = 1, sy = 1;
    /* fixtures need their walls in place first: read the special cells */
    for (int y = 0; y < lg.mh; y++)
        for (int x = 0; x < lg.mw; x++) {
            char c = lg.map[y][x];
            if (c == '@') { sx = x; sy = y; lg.map[y][x] = '.'; }
            else if (c == 'H') { lg.honey_x = x; lg.honey_y = y; } /* a surface like any other */
            else if (c == 'E') { lg.exit_x = x; lg.exit_y = y; }
        }
    for (int y = 0; y < lg.mh; y++)
        for (int x = 0; x < lg.mw; x++) {
            int k = foe_of(lg.map[y][x]);
            if (k == FO_NONE) continue;
            char c = lg.map[y][x];
            lg.map[y][x] = '.';
            if (lg.nspawn >= LMP_MAX_SPAWNS) continue;
            LmpSpawn *s = &lg.spawn[lg.nspawn++];
            s->kind = (uint8_t)k;
            s->x = x;
            s->y = y;
            s->n = (k == FO_TURRET || k == FO_LASER || k == FO_FLAME || k == FO_SAC) ? attach_normal(x, y)
                 : (c == 'q' ? DR_D : DR_R);
        }
    if (level == LV_HUB) {
        for (int i = 0; i < LMP_HUB_NGATES; i++) {
            const LmpGate *g = &LMP_HUB_GATE[i];
            if (!lmp_gate_open(i)) continue;
            for (int y = g->y; y < g->y + g->h; y++)
                for (int x = g->x; x < g->x + g->w; x++)
                    if (lmp_tile(x, y) == 'G') lg.map[y][x] = '.';
        }
    }
    for (int i = 0; i < L->nthings; i++) {
        const LmpThing *t = &L->things[i];
        if (t->type != TH_MOVER || lg.nmover >= LMP_MAX_MOVERS) continue;
        LmpMover *m = &lg.mover[lg.nmover++];
        memset(m, 0, sizeof *m);
        m->x = t->x; m->y = t->y; m->w = t->w; m->h = t->h;
        m->dir = t->dir; m->dist = t->dist; m->period = t->period; m->lethal = t->lethal;
        m->sign = 1;
        mover_paint(m, 'm');
    }
    /* the boss arenas and survival decks have no bulb: their pods open when it's over */
    lmp_tank_place(sx, sy, lmp_state_ok(sx, sy, DR_U) ? DR_U : attach_normal(sx, sy));
    for (int i = 0; i < lg.nspawn; i++) {
        int f = lmp_spawn_foe(lg.spawn[i].kind, lg.spawn[i].x, lg.spawn[i].y, lg.spawn[i].n);
        if (f >= 0) lg.foe[f].spawn = i;
    }
    if (lg.kind == LK_TURNCOAT || lg.kind == LK_DREAD) lmp_boss_start();
    lmp_take_checkpoint();
}

/* ---- every frame ---------------------------------------------------------- */

void lmp_world_update(void) {
    movers_update();
    if (lg.timed_t > 0) {
        lg.timed_t--;
        if (lg.timed_t == 0) {
            lmp_sfx("lmp_door");
            if (lmp_tile(lg.tk.x, lg.tk.y) == 'Y') lmp_kill_tank(102);
        } else if (lg.timed_t < 90 && lg.timed_t % 15 == 0) {
            lmp_sfx("lmp_tick");
        }
    }
}
