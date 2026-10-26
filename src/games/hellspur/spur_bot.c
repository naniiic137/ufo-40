/* HELLSPUR - the demo player. It reads the game's state the way a player
 * reads the screen, and answers with buttons only: it walks the maps by
 * the shortest way, presses A on the red of the timing bar, heals the hurt,
 * rests at hotels, raises the dead at chapels, shops for better guns,
 * banks its money, grinds when the next fight is too much, and works
 * through the story one step at a time to the end. The tests use it to
 * prove the game can be finished; the title screen uses it for its demo. */
#include "spur.h"

int spur_bot_debug;
int spur_bot_goal = -1;

enum { GK_ONTO, GK_ADJ };
enum { ACT_NONE, ACT_TALK, ACT_SEARCH, ACT_ENTER };

typedef struct {
    int frame;
    int answer;                 /* yes/no questions: 0 YES, 1 NO */
    int intent;                 /* what to do at the next counter */
    int task;                   /* a job in the B menu */
    int t_who, t_slot, t_item, t_sk, t_tgt;
    int stuck, last_x, last_y, last_map;
    int grind_t, grind_dir;
    int shop_map, shop_teeth;   /* where and with how much money it last shopped */
    int dest_end;               /* going through a pass: the end to come out of (-1 none) */
    int pick[4], npick;
    int cool;                   /* frames to wait */
    int step;
    int rest_flag;
    int wander;
    int bank_map, bank_mode;    /* the town whose bank was emptied this visit; 1 deposit, 2 withdraw */
} Bot;
static int bot_extra[64];       /* levels added to a step after losing there */
static Bot bot;

enum { IN_NONE, IN_REST, IN_REVIVE, IN_SHOP, IN_BANK, IN_LEAVE };
enum { TK_NONE, TK_EQUIP, TK_ITEM, TK_SKILL, TK_MOUNT, TK_CLOSE };

#define TAP(b) ((bot.frame & 1) ? 0 : (b))

/* ---- the story, step by step ----------------------------------------------------------- */
enum {
    S_PICK, S_WREN, S_PUCK, S_CRANK, S_FERRY, S_HONEY, S_BRUTUS, S_TALLY, S_SCOURGE, S_AMBROSE, S_FORT, S_MOLLY, S_BELL,
    S_MIMIC, S_CELLAR, S_DOOR, S_PLUNGER, S_BLAST, S_CROW, S_CHARM, S_WORM, S_DREAM, S_MOTHER, S_SPURKEY, S_BLIGHT,
    S_CARNAGE, S_HUNGER, S_REAPER, S_CLAW, S_SPIKE1, S_SPIKE2, S_SPIKE3, S_GOV, S_FINAL, S_DONE
};
/* the average level wanted before each step, and the town to rest in */
static const struct { uint8_t level; uint8_t home; } STEP[S_DONE + 1] = {
    [S_PICK] = {1, M_GLADWELL}, [S_WREN] = {1, M_GLADWELL}, [S_PUCK] = {6, M_MERIDIAN}, [S_CRANK] = {6, M_MERIDIAN},
    [S_FERRY] = {6, M_MERIDIAN}, [S_HONEY] = {8, M_HOPE}, [S_BRUTUS] = {8, M_HOPE}, [S_TALLY] = {9, M_HOPE},
    [S_SCOURGE] = {13, M_HOPE}, [S_AMBROSE] = {13, M_HOPE}, [S_FORT] = {16, M_STEEPLE}, [S_MOLLY] = {18, M_DOLORES},
    [S_BELL] = {18, M_DOLORES}, [S_MIMIC] = {20, M_STEEPLE}, [S_CELLAR] = {20, M_STEEPLE}, [S_DOOR] = {20, M_DOLORES},
    [S_PLUNGER] = {23, M_HOLLOW}, [S_BLAST] = {23, M_FORT}, [S_CROW] = {25, M_FORT}, [S_CHARM] = {26, M_WIDOW},
    [S_WORM] = {28, M_WIDOW}, [S_DREAM] = {30, M_LASTCHANCE}, [S_MOTHER] = {31, M_LASTCHANCE},
    [S_SPURKEY] = {31, M_LASTCHANCE}, [S_BLIGHT] = {33, M_LASTCHANCE}, [S_CARNAGE] = {34, M_LASTCHANCE},
    [S_HUNGER] = {35, M_LASTCHANCE}, [S_REAPER] = {37, M_LASTCHANCE}, [S_CLAW] = {37, M_HERMITAGE},
    [S_SPIKE1] = {37, M_ASHMARKET}, [S_SPIKE2] = {37, M_LASTCHANCE}, [S_SPIKE3] = {37, M_ASHMARKET},
    [S_GOV] = {40, M_ASHMARKET}, [S_FINAL] = {42, M_ASHMARKET}, [S_DONE] = {1, M_GLADWELL},
};

static int current_step(void) {
    if (!spur_flag(F_POSSE)) return S_PICK;
    if (!spur_flag(F_WREN)) return S_WREN;
    if (!spur_flag(F_PUCKKING)) return S_PUCK;
    if (!spur_flag(F_FERRY) && !spur_key(KI_CRANK)) return S_CRANK;
    if (!spur_flag(F_FERRY)) return S_FERRY;
    if (!spur_flag(F_BRUTUS_GONE) && !spur_key(KI_HONEYCOMB)) return S_HONEY;
    if (!spur_flag(F_BRUTUS_GONE)) return S_BRUTUS;
    if (!spur_flag(F_HOPE_FREE)) return S_TALLY;
    if (!spur_flag(F_SCOURGE)) return S_SCOURGE;
    if (!spur_flag(F_AMBROSE)) return S_AMBROSE;
    if (!spur_flag(F_FORT)) return S_FORT;
    if (!spur_flag(F_MOLLY)) return S_MOLLY;
    if (!spur_flag(F_GIL_GAVE)) return S_BELL;
    if (!spur_flag(F_MIMIC)) return S_MIMIC;
    if (!spur_flag(F_ROOST_DOOR) && !spur_key(KI_CELLARKEY)) return S_CELLAR;
    if (!spur_flag(F_ROOST_DOOR)) return S_DOOR;
    if (!spur_flag(F_CUT_OPEN) && !spur_key(KI_PLUNGER)) return S_PLUNGER;
    if (!spur_flag(F_CUT_OPEN)) return S_BLAST;
    if (!spur_flag(F_STORMCROW)) return S_CROW;
    if (!spur_key(KI_FIRECHARM)) return S_CHARM;
    if (!spur_flag(F_WORM)) return S_WORM;
    if (!spur_flag(F_MISC0 + 1) && !spur_flag(F_MOTHER)) return S_DREAM;
    if (!spur_flag(F_MOTHER)) return S_MOTHER;
    if (!spur_key(KI_IRONSPUR) && !spur_flag(F_GATE_OPEN)) return S_SPURKEY;
    if (!spur_flag(F_BLIGHT)) return S_BLIGHT;
    if (!spur_flag(F_CARNAGE)) return S_CARNAGE;
    if (!spur_flag(F_HUNGER)) return S_HUNGER;
    if (!spur_flag(F_REAPER)) return S_REAPER;
    if (!spur_flag(F_CLAWBAR)) return S_CLAW;
    if (!spur_flag(F_SPIKE1)) return S_SPIKE1;
    if (!spur_flag(F_SPIKE2)) return S_SPIKE2;
    if (!spur_flag(F_SPIKE3)) return S_SPIKE3;
    if (!spur_flag(F_GOVERNOR)) return S_GOV;
    if (!spur_flag(F_WON)) return S_FINAL;
    return S_DONE;
}

int spur_bot_step(void) { return current_step(); }

void spur_bot_pick(const int *h, int n) {
    memset(&bot, 0, sizeof bot);
    memset(bot_extra, 0, sizeof bot_extra);
    bot.npick = imin(n, 4);
    for (int i = 0; i < bot.npick; i++) bot.pick[i] = h[i];
}

/* where a step is done: a map, and a tile (or a person) there */
typedef struct { int map, x, y, kind, act, npc; } Goal;

static int find_npc(int look, int pal) {
    for (int i = 0; i < sg.map.nnpc; i++)
        if (spur_npc_visible(i) && sg.map.npc[i].look == look && (pal < 0 || sg.map.npc[i].pal == pal)) return i;
    return -1;
}

static Goal step_goal(int s) {
    Goal g = {-1, 0, 0, GK_ADJ, ACT_TALK, -1};
    switch (s) {
    case S_WREN: g.map = M_STCLARE; g.npc = 1; break;
    case S_PUCK: g.map = M_SIXPENNY; g.npc = -2; break;              /* -2: the boss on the map */
    case S_CRANK: g.map = M_SIXPENNY; g.act = ACT_SEARCH; g.npc = -3; g.x = 1000 + KI_CRANK; break; /* -3: a chest holding x */
    case S_FERRY: g.map = M_WORLD; g.x = 58; g.y = 76; g.act = ACT_SEARCH; break;
    case S_HONEY: g.map = M_WORLD; g.x = 86; g.y = 84; g.act = ACT_SEARCH; break;
    case S_BRUTUS: g.map = M_HOPE; g.npc = 0; break;
    case S_TALLY: g.map = M_HOPE; g.npc = -4; g.kind = GK_ONTO; g.act = ACT_ENTER; break; /* -4: the story door */
    case S_SCOURGE: g.map = M_PEN2; g.npc = -2; break;
    case S_AMBROSE: g.map = M_PEN2; g.npc = -5; break;               /* -5: the story person */
    case S_FORT: g.map = M_FORT; g.npc = -6; g.act = ACT_NONE; break; /* -6: just get there */
    case S_MOLLY: g.map = M_ROOST; g.npc = 0; break;
    case S_BELL: g.map = M_DOLORES; g.npc = 1; break;
    case S_MIMIC: g.map = M_THORNS; g.npc = 0; break;
    case S_CELLAR: g.map = M_CRYPT; g.act = ACT_SEARCH; g.npc = -3; g.x = 1000 + KI_CELLARKEY; break;
    case S_DOOR: g.map = M_ROOST; g.x = 15; g.y = 6; g.act = ACT_SEARCH; break;
    case S_PLUNGER: g.map = M_HOLLOWMINE; g.act = ACT_SEARCH; g.npc = -3; g.x = 1000 + KI_PLUNGER; break;
    case S_BLAST: g.map = M_WORLD; g.x = 63; g.y = 44; g.act = ACT_SEARCH; break;
    case S_CROW: g.map = M_CUT; g.npc = -2; break;
    case S_CHARM: g.map = M_WIDOW; g.npc = 0; break;
    case S_WORM: g.map = M_SNAKE2; g.npc = -2; break;
    case S_DREAM: case S_SPURKEY: g.map = M_SHRINE; g.npc = 0; break;
    case S_MOTHER: g.map = M_DREAM2; g.npc = -2; break;
    case S_BLIGHT: g.map = M_TOWER1; g.npc = -2; break;
    case S_CARNAGE: g.map = M_TOWER2; g.npc = -2; break;
    case S_HUNGER: g.map = M_TOWER3; g.npc = -2; break;
    case S_REAPER: g.map = M_TOWER4; g.npc = -2; break;
    case S_CLAW: g.map = M_HERMITAGE; g.npc = 1; break;
    case S_SPIKE1: g.map = M_WORLD; g.x = 104; g.y = 7; g.act = ACT_SEARCH; break;
    case S_SPIKE2: g.map = M_WORLD; g.x = 33; g.y = 30; g.act = ACT_SEARCH; break;
    case S_SPIKE3: g.map = M_WORLD; g.x = 118; g.y = 70; g.act = ACT_SEARCH; break;
    case S_GOV: g.map = M_SPUR2; g.npc = -2; break;
    case S_FINAL: g.map = M_SPUR3; g.npc = 0; break;
    default: break;
    }
    return g;
}

/* ---- the way between maps ------------------------------------------------------------------ */
/* the maps you pass through from the big map to reach each one */
static int chain_of(int map, int *c) {
    int n = 0;
    switch (map) {
    case M_PEN2: c[n++] = M_PEN1; c[n++] = M_PEN2; break;
    case M_CRYPT: c[n++] = M_THORNS; c[n++] = M_CRYPT; break;
    case M_ROOSTMINE: c[n++] = M_ROOST; c[n++] = M_ROOSTMINE; break;
    case M_DREAM1: c[n++] = M_SHRINE; c[n++] = M_DREAM1; break;
    case M_DREAM2: c[n++] = M_SHRINE; c[n++] = M_DREAM1; c[n++] = M_DREAM2; break;
    case M_TOWER1: case M_TOWER2: case M_TOWER3: case M_TOWER4:
        c[n++] = M_HELL;
        for (int t = M_TOWER1; t <= map; t++) c[n++] = t;
        break;
    case M_SPUR2: c[n++] = M_SPUR1; c[n++] = M_SPUR2; break;
    case M_SPUR3: c[n++] = M_SPUR1; c[n++] = M_SPUR2; c[n++] = M_SPUR3; break;
    case M_HELL: c[n++] = M_HELL; break;
    default: c[n++] = map; break;
    }
    return n;
}

/* the big-map door of a map (its first step from outside) */
static void world_door(int map, int end, int *x, int *y) {
    static SpurMap w;
    static bool built;
    if (!built) { spur_world_build(&w, M_WORLD); built = true; }
    *x = SPUR_MAPINFO[map].px;
    *y = SPUR_MAPINFO[map].py;
    for (int i = 0; i < w.nlink; i++)
        if (w.link[i].map == map && ((end && w.link[i].tx == -2) || (!end && w.link[i].tx != -2))) { *x = w.link[i].x; *y = w.link[i].y; return; }
}

/* the passes: two big-map doors joined by a dungeon, once the story opens them */
typedef struct { int map; int8_t ax, ay, bx, by; } Pass;
static const Pass PASSES[] = {
    {M_NOTCH, 84, 56, 84, 51}, {M_CUT, 62, 44, 59, 38}, {M_SNAKE1, 46, 48, 38, 44}, {M_SNAKE2, 6, 42, 6, 34},
    {M_ROOSTMINE, 106, 14, 59, 12},
};
static bool pass_open(int i) {
    switch (PASSES[i].map) {
    case M_NOTCH: return spur_flag(F_AMBROSE);
    case M_CUT: return spur_flag(F_CUT_OPEN) && spur_flag(F_STORMCROW);
    case M_SNAKE1: return true;
    case M_SNAKE2: return spur_flag(F_WORM);
    case M_ROOSTMINE: return spur_flag(F_ROOST_DOOR);
    default: return false;
    }
}

/* ---- finding the way on a map --------------------------------------------------------------- */
#define FLOODS 6
static int16_t floods[FLOODS][SPUR_MAX_H][SPUR_MAX_W];
static int16_t (*dist)[SPUR_MAX_W] = floods[0];
static int qx[SPUR_MAX_W * SPUR_MAX_H], qy[SPUR_MAX_W * SPUR_MAX_H];
static const int DX[4] = {0, 0, -1, 1}, DY[4] = {1, -1, 0, 0};

static bool is_link(int x, int y) {
    for (int i = 0; i < sg.map.nlink; i++) if (sg.map.link[i].x == x && sg.map.link[i].y == y) return true;
    for (int i = 0; i < sg.map.ndoor; i++) if (sg.map.door[i].x == x && sg.map.door[i].y == y) return true;
    return false;
}

static bool loop_ambush(int x, int y) {
    for (int i = 0; i < sg.map.nambush; i++) if (sg.map.ambush[i].flag < 0 && sg.map.ambush[i].x == x && sg.map.ambush[i].y == y) return true;
    return false;
}

static bool passable(int x, int y, bool fly, int gx, int gy) {
    if (x < 0 || y < 0 || x >= sg.map.w || y >= sg.map.h) return false;
    if (x == gx && y == gy) return spur_walkable(x, y, fly) || is_link(x, y) || (SPUR_TILE_FLAGS[sg.map.tile[y][x]] & TF_ENTER);
    if (!spur_walkable(x, y, fly)) return false;
    if (spur_npc_at(x, y) >= 0) return false;
    if (is_link(x, y)) return false;
    if (sg.map.kind == MK_TOWN && (x == 0 || y == 0 || x == sg.map.w - 1 || y == sg.map.h - 1)) return false;
    if (loop_ambush(x, y)) return false;
    if (sg.map.kind == MK_WORLD && !fly && (SPUR_TILE_FLAGS[sg.map.tile[y][x]] & TF_ENTER)) return false;
    return true;
}

static bool pass_door(int x, int y, int *far_x, int *far_y) {
    if (sg.map_id != M_WORLD) return false;
    for (int i = 0; i < ARRAY_LEN(PASSES); i++) {
        if (!pass_open(i)) continue;
        const Pass *p = &PASSES[i];
        if (x == p->ax && y == p->ay) { *far_x = p->bx; *far_y = p->by; return true; }
        if (x == p->bx && y == p->by) { *far_x = p->ax; *far_y = p->ay; return true; }
    }
    return false;
}

/* distances from the goal back to every tile; a pass's two doors are joined (the dungeon between costs 40).
 * The last flood is kept while the goal and the map stay the same (people move: it is redone now and then). */
typedef struct { int map, gx, gy, kind, fly, frame, flags, used; } FloodKey;
static FloodKey fkeys[FLOODS];
static void flood_now(int gx, int gy, int kind, bool fly);
static void flood_from(int gx, int gy, int kind, bool fly) {
    int flags = sg.s.flags[F_FERRY / 8] ^ sg.s.flags[F_CUT_OPEN / 8] * 3 ^ sg.s.flags[F_WORM / 8] * 7 ^ (int)sg.s.keyitems * 13 ^ sg.map.nnpc * 17;
    int oldest = 0;
    for (int k = 0; k < FLOODS; k++) {
        FloodKey *f = &fkeys[k];
        if (f->used && f->map == sg.map_id && f->gx == gx && f->gy == gy && f->kind == kind && f->fly == (int)fly &&
            f->flags == flags && bot.frame - f->frame < 120 && floods[k][sg.pos[0].y][sg.pos[0].x] >= 0) {
            dist = floods[k];
            return;
        }
        if (!fkeys[k].used || fkeys[k].frame < fkeys[oldest].frame) oldest = k;
    }
    fkeys[oldest] = (FloodKey){sg.map_id, gx, gy, kind, fly, bot.frame, flags, 1};
    dist = floods[oldest];
    flood_now(gx, gy, kind, fly);
}
static void flood_now(int gx, int gy, int kind, bool fly) {
    for (int y = 0; y < sg.map.h; y++) for (int x = 0; x < sg.map.w; x++) dist[y][x] = -1;
    int qh = 0, qt = 0;
    if (kind == GK_ONTO) { dist[gy][gx] = 0; qx[qt] = gx; qy[qt] = gy; qt++; }
    else
        for (int k = 0; k < 4; k++) {
            int x = gx + DX[k], y = gy + DY[k];
            if (passable(x, y, fly, -1, -1) || (x == sg.pos[0].x && y == sg.pos[0].y && spur_walkable(x, y, fly))) {
                dist[y][x] = 0;
                qx[qt] = x; qy[qt] = y; qt++;
            }
        }
    while (qh < qt) {
        int x = qx[qh], y = qy[qh];
        qh++;
        int d = dist[y][x];
        int fx, fy;
        if (!fly && !(x == gx && y == gy) && pass_door(x, y, &fx, &fy) && dist[fy][fx] < 0) {
            dist[fy][fx] = (int16_t)(d + 40);
            qx[qt] = fx; qy[qt] = fy; qt++;
        }
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (nx < 0 || ny < 0 || nx >= sg.map.w || ny >= sg.map.h || dist[ny][nx] >= 0) continue;
            bool me = nx == sg.pos[0].x && ny == sg.pos[0].y;
            bool door = !fly && pass_door(nx, ny, &fx, &fy);
            if (!me && !door && !passable(nx, ny, fly, gx, gy)) continue;
            /* a pass door is only a way through, never a stop on the way */
            dist[ny][nx] = (int16_t)(d + 1);
            qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
}

/* the first step toward a goal: a direction, -2 when there, or -1 */
static int step_toward(int gx, int gy, int kind, bool fly) {
    flood_from(gx, gy, kind, fly);
    int x = sg.pos[0].x, y = sg.pos[0].y;
    if (kind == GK_ADJ && dist[y][x] == 0 && iabs(x - gx) + iabs(y - gy) == 1) return -2;
    if (kind == GK_ONTO && x == gx && y == gy) return -2;
    int best = -1, bd = dist[y][x] >= 0 ? dist[y][x] : 30000;
    for (int k = 0; k < 4; k++) {
        int nx = x + DX[k], ny = y + DY[k];
        if (nx < 0 || ny < 0 || nx >= sg.map.w || ny >= sg.map.h) continue;
        int d = dist[ny][nx];
        if (d < 0 || d >= bd) continue;
        int fx, fy;
        bool door = !fly && pass_door(nx, ny, &fx, &fy) && !(nx == gx && ny == gy);
        if (!door && !passable(nx, ny, fly, gx, gy)) continue;
        bd = d;
        best = k;
    }
    if (best >= 0) {
        int nx = x + DX[best], ny = y + DY[best], fx, fy;
        if (!fly && !(nx == gx && ny == gy) && pass_door(nx, ny, &fx, &fy)) bot.dest_end = 1000 + fx * 128 + fy;
    }
    return best;
}

/* ---- pressing in the menus ------------------------------------------------------------------- */
static int nav_to(int cur, int want, int n) {
    if (cur == want) return SPUR_OK;
    int down = (want - cur + n) % n, up = (cur - want + n) % n;
    return down <= up ? BTN_DOWN : BTN_UP;
}

/* ---- battle ------------------------------------------------------------------------------------ */
static int bat_cmd[4], bat_target[4], bat_skill[4], bat_item[4];

static int hp_pct(int h) { return spur_alive(h) ? sg.s.hero[h].hp * 100 / imax(1, spur_maxhp(h)) : 0; }

static int weakest_slot(int below) {
    int best = -1, bp = below;
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (!spur_alive(h)) continue;
        int p = hp_pct(h);
        if (p < bp) { bp = p; best = i; }
    }
    return best;
}

static int dead_slot(void) {
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (h >= 0 && !spur_alive(h)) return i;
    }
    return -1;
}

static int battle_item_index(int bag_slot) {
    int n = 0;
    for (int i = 0; i < SPUR_BAG; i++) {
        int it = sg.s.bag[i];
        if (it && SPUR_ITEM[it].kind == IK_USE && (SPUR_ITEM[it].flags & IF_BATTLE)) {
            if (i == bag_slot) return n;
            n++;
        }
    }
    return -1;
}

static int battle_skill_index(int h, int sk) {
    int all[SK_COUNT];
    int n = spur_skill_list(h, all), k = 0;
    for (int i = 0; i < n; i++) {
        if (!(SPUR_SKILL[all[i]].use & SKU_BATTLE)) continue;
        if (all[i] == sk) return k;
        k++;
    }
    return -1;
}

static int best_heal_item(void) {
    int best = -1, bv = 0;
    for (int i = 0; i < SPUR_BAG; i++) {
        int it = sg.s.bag[i];
        if (it && SPUR_ITEM[it].use == IU_HEAL && (SPUR_ITEM[it].flags & IF_BATTLE)) {
            int v = SPUR_ITEM[it].power;
            if (v > bv) { bv = v; best = i; }
        }
    }
    return best;
}

static int best_foe(void) {
    /* the last fight ends with Zenith */
    if (sg.b.troop == 100)
        for (int i = 0; i < sg.b.nfoe; i++)
            if (spur_foe_alive(i) && sg.b.foe[i].e == 100) return i;
    /* the siege ends with its captain */
    if (sg.b.troop == 90)
        for (int i = 0; i < sg.b.nfoe; i++)
            if (spur_foe_alive(i) && sg.b.foe[i].e == 87) return i;
    /* finish the weakest first */
    int best = -1;
    int32_t bh = 0x7FFFFFFF;
    for (int i = 0; i < sg.b.nfoe; i++)
        if (spur_foe_alive(i) && sg.b.foe[i].hp < bh) { bh = sg.b.foe[i].hp; best = i; }
    return best < 0 ? 0 : best;
}

static void decide_order(int slot) {
    int h = spur_hero_at(slot);
    SpurHero *hr = &sg.s.hero[h];
    bat_cmd[slot] = CMD_ATTACK;
    bat_target[slot] = best_foe();
    int weak = weakest_slot(40), dead = dead_slot();
    bool boss = sg.b.boss;
    /* raise the dead in a hard fight */
    if (dead >= 0 && (boss || spur_foes_left() > 1)) {
        if (spur_has_skill(h, SK_RAISE) && hr->sp >= SPUR_SKILL[SK_RAISE].sp) {
            bat_cmd[slot] = CMD_SKILL; bat_skill[slot] = SK_RAISE; bat_target[slot] = dead; return;
        }
        int oil = spur_bag_find(IT_HOLYOIL);
        if (oil >= 0 && boss) { bat_cmd[slot] = CMD_ITEM; bat_item[slot] = oil; bat_target[slot] = dead; return; }
    }
    if (weak >= 0) {
        static const int HEALS[] = {SK_MINUET, SK_SUTURE, SK_REEL, SK_MEND};
        for (int k = 0; k < 4; k++) {
            int sk = HEALS[k];
            if (spur_has_skill(h, sk) && hr->sp >= SPUR_SKILL[sk].sp) {
                bat_cmd[slot] = CMD_SKILL; bat_skill[slot] = sk; bat_target[slot] = weak; return;
            }
        }
        int it = best_heal_item();
        /* only one item user per round: the weakest-hit order goes to the first who can */
        bool taken = false;
        for (int i = 0; i < slot; i++) taken |= bat_cmd[i] == CMD_ITEM;
        if (it >= 0 && !taken && hp_pct(spur_hero_at(weak)) < 35) { bat_cmd[slot] = CMD_ITEM; bat_item[slot] = it; bat_target[slot] = weak; return; }
    }
    /* buffs at the start of a boss fight */
    if (boss && sg.b.turn <= 2) {
        static const int BUFF[] = {SK_CANCAN, SK_SHIMMY, SK_STEADY};
        for (int k = 0; k < 3; k++)
            if (spur_has_skill(h, BUFF[k]) && hr->sp >= SPUR_SKILL[BUFF[k]].sp + 10) {
                bat_cmd[slot] = CMD_SKILL; bat_skill[slot] = BUFF[k]; return;
            }
    }
    /* Quill takes notes on what he hasn't seen */
    if (h == H_QUILL && spur_has_skill(h, SK_OBSERVE) && hr->sp >= 2 && hr->observed < SPUR_QUILL_MAX - 1 && !boss) {
        for (int i = 0; i < sg.b.nfoe; i++) {
            if (!spur_foe_alive(i)) continue;
            int learn = SPUR_ENEMY[sg.b.foe[i].e].observe;
            if (learn && !spur_has_skill(h, learn)) { bat_cmd[slot] = CMD_SKILL; bat_skill[slot] = SK_OBSERVE; bat_target[slot] = i; return; }
        }
    }
    /* a big hit when there's soul to spare */
    static const int HITS[] = {SK_MAUL, SK_SWOOP, SK_HAYMAKER, SK_BUCK, SK_BITE, SK_JAB, SK_DUSTSTORM};
    if (hr->sp > spur_maxsp(h) / 2 && (boss || spur_foes_left() >= 3))
        for (int k = 0; k < 7; k++)
            if (spur_has_skill(h, HITS[k]) && hr->sp >= SPUR_SKILL[HITS[k]].sp + 8 && spur_weapon_class(h) != WC_HEAVY) {
                bat_cmd[slot] = CMD_SKILL; bat_skill[slot] = HITS[k]; return;
            }
}

static int battle_buttons(void) {
    SpurBattle *b = &sg.b;
    switch (b->phase) {
    case BP_INTRO: case BP_MSG: case BP_WIN: case BP_LEVELUP: case BP_LOSE: return TAP(SPUR_OK);
    case BP_COMMAND: {
        if (b->phase_t < 2) return 0;
        if (b->phase_t == 2 || bat_cmd[b->cur] < 0) decide_order(b->cur);
        int want = bat_cmd[b->cur];
        /* an order that can't be given falls back to ATTACK */
        if (want == CMD_SKILL && battle_skill_index(spur_hero_at(b->cur), bat_skill[b->cur]) < 0) want = bat_cmd[b->cur] = CMD_ATTACK;
        if (want == CMD_ITEM && battle_item_index(bat_item[b->cur]) < 0) want = bat_cmd[b->cur] = CMD_ATTACK;
        int k = nav_to(b->sel, want, CMD_COUNT);
        return TAP(k);
    }
    case BP_TARGET: {
        int want = bat_target[b->cur];
        if (!spur_foe_alive(want)) want = bat_target[b->cur] = best_foe();
        return TAP(b->target_sel == want ? SPUR_OK : BTN_RIGHT);
    }
    case BP_ALLYTARGET: {
        int want = bat_target[b->cur];
        int n = imax(1, sg.s.nposse);
        return TAP(nav_to(b->target_sel, iclamp(want, 0, n - 1), n));
    }
    case BP_SKILLMENU: {
        int idx = battle_skill_index(spur_hero_at(b->cur), bat_skill[b->cur]);
        if (idx < 0) return TAP(SPUR_BACK);
        int all[SK_COUNT], n = 0;
        int m = spur_skill_list(spur_hero_at(b->cur), all);
        for (int i = 0; i < m; i++) n += (SPUR_SKILL[all[i]].use & SKU_BATTLE) != 0;
        return TAP(nav_to(b->sub_sel, idx, imax(1, n)));
    }
    case BP_ITEMMENU: {
        int idx = battle_item_index(bat_item[b->cur]);
        if (idx < 0) return TAP(SPUR_BACK);
        int n = 0;
        for (int i = 0; i < SPUR_BAG; i++) { int it = sg.s.bag[i]; n += it && SPUR_ITEM[it].kind == IK_USE && (SPUR_ITEM[it].flags & IF_BATTLE); }
        return TAP(nav_to(b->sub_sel, idx, imax(1, n)));
    }
    case BP_BAR: {
        if (b->bar_auto || b->bar_done_t) return 0;
        if (b->bar_kind == WC_HEAVY) {
            int nx = b->bar_x + b->bar_speed;
            return nx >= b->z[0].a && nx <= b->z[0].b ? SPUR_OK : 0;
        }
        int nx = b->bar_x + b->bar_speed;
        for (int k = 0; k < b->nz; k++) {
            if (b->z[k].result != ZR_PENDING) continue;
            /* aim for the red; if the red is a single step wide, take it */
            if (nx >= b->z[k].red && nx <= b->z[k].b) return SPUR_OK;
            break;
        }
        return 0;
    }
    default: return 0;
    }
}

/* ---- shopping and the menus ---------------------------------------------------------------------- */
static int weapon_value(int h, int it) {
    SpurHero *hr = &sg.s.hero[h];
    int keep = hr->equip[EQ_RHAND], keepl = hr->equip[EQ_LHAND];
    hr->equip[EQ_RHAND] = (uint8_t)it;
    hr->equip[EQ_LHAND] = 0;
    int p = spur_attack_power(h);
    hr->equip[EQ_RHAND] = (uint8_t)keep;
    hr->equip[EQ_LHAND] = (uint8_t)keepl;
    const SpurItemDef *d = &SPUR_ITEM[it];
    int hits = d->wclass == WC_HEAVY ? 2 : imax(1, d->hits);
    int v = p * hits;
    if (d->wclass == WC_BOW && h != H_SAGE) v /= 3;    /* all red: too hard to hit */
    if (d->flags & IF_RELOAD && d->wclass != WC_HEAVY) v /= 2;
    return v;
}

static int slot_value(int h, int it, int slot) {
    if (!it) return 0;
    if (slot == EQ_RHAND) return weapon_value(h, it);
    return SPUR_ITEM[it].power * 4 + SPUR_ITEM[it].res;
}

static int equip_slot_for(int it) {
    switch (SPUR_ITEM[it].kind) {
    case IK_WEAPON: return EQ_RHAND;
    case IK_NECK: return EQ_NECK;
    case IK_BODY: return EQ_BODY;
    case IK_FEET: return EQ_FEET;
    default: return -1;
    }
}

/* the bag item that would most improve someone, and for whom */
static bool find_upgrade(int *who_slot, int *eq_slot, int *bag_slot) {
    int bestgain = 0;
    for (int i = 0; i < sg.s.nposse; i++) {
        int h = spur_hero_at(i);
        if (h < 0) continue;
        for (int b = 0; b < SPUR_BAG; b++) {
            int it = sg.s.bag[b];
            if (!it) continue;
            int s = equip_slot_for(it);
            if (s < 0 || !spur_can_equip(h, it, s)) continue;
            int gain = slot_value(h, it, s) - slot_value(h, sg.s.hero[h].equip[s], s);
            if (gain > bestgain) { bestgain = gain; *who_slot = i; *eq_slot = s; *bag_slot = b; }
        }
    }
    return bestgain > 0;
}

/* how many of a kind are in the bag */
static int count_of(int it) {
    int n = 0;
    for (int i = 0; i < SPUR_BAG; i++) n += sg.s.bag[i] == it;
    return n;
}

static int heal_count(void) {
    int n = 0;
    for (int i = 0; i < SPUR_BAG; i++) { int it = sg.s.bag[i]; n += it && SPUR_ITEM[it].use == IU_HEAL; }
    return n;
}

/* what to buy here: the best upgrade anyone gets, then medicine */
static int shop_want(int shop, int *idx) {
    const SpurShop *s = &SPUR_SHOP[shop];
    int budget = sg.s.teeth - 60;
    int best = -1, bestgain = 0;
    for (int k = 0; k < SPUR_SHOP_ITEMS && s->item[k]; k++) {
        int it = s->item[k];
        const SpurItemDef *d = &SPUR_ITEM[it];
        if (d->price > budget || spur_bag_count() >= SPUR_BAG - 1) continue;
        int slot = equip_slot_for(it);
        if (slot >= 0) {
            if (spur_bag_find(it) >= 0) continue;
            for (int i = 0; i < sg.s.nposse; i++) {
                int h = spur_hero_at(i);
                if (h < 0 || !spur_can_equip(h, it, slot)) continue;
                int gain = slot_value(h, it, slot) - slot_value(h, sg.s.hero[h].equip[slot], slot);
                if (gain > bestgain) { bestgain = gain; best = k; }
            }
        }
    }
    if (best >= 0) { *idx = best; return 1; }
    /* medicine: keep six of the best healer, two antidotes, a holy oil or two */
    int heal_it = 0;
    for (int k = 0; k < SPUR_SHOP_ITEMS && s->item[k]; k++) {
        int it = s->item[k];
        if (SPUR_ITEM[it].use == IU_HEAL && SPUR_ITEM[it].price <= budget && (!heal_it || SPUR_ITEM[it].power > SPUR_ITEM[heal_it].power)) {
            /* a med chest only once the purse can stand it */
            if (it == IT_MEDCHEST && sg.s.teeth < 6000) continue;
            heal_it = it;
        }
    }
    for (int k = 0; k < SPUR_SHOP_ITEMS && s->item[k]; k++) {
        int it = s->item[k];
        if (SPUR_ITEM[it].price > budget || spur_bag_count() >= SPUR_BAG - 2) continue;
        if (it == heal_it && heal_count() < 6) { *idx = k; return 1; }
        if (it == IT_SNAKEROOT && count_of(IT_SNAKEROOT) + count_of(IT_HALFROOT) < 2) { *idx = k; return 1; }
        if (it == IT_HOLYOIL && count_of(IT_HOLYOIL) < 2 && sg.s.teeth > 8000) { *idx = k; return 1; }
        if (it == IT_HALOPIN && count_of(IT_HALOPIN) < 1 && sg.s.teeth > 3000) { *idx = k; return 1; }
    }
    return 0;
}

/* what to sell: ores, and gear nobody wants */
static int sell_want(void) {
    for (int b = 0; b < SPUR_BAG; b++) {
        int it = sg.s.bag[b];
        if (!it) continue;
        const SpurItemDef *d = &SPUR_ITEM[it];
        if (it == IT_HELLITE || it == IT_DOOMSDAY || it == IT_BEADS || it == IT_STEADYEYE) continue;
        if (d->kind == IK_ORE) return b;
        int slot = equip_slot_for(it);
        if (slot < 0) continue;
        bool useful = false;
        for (int i = 0; i < sg.s.nposse && !useful; i++) {
            int h = spur_hero_at(i);
            if (h >= 0 && spur_can_equip(h, it, slot) && slot_value(h, it, slot) > slot_value(h, sg.s.hero[h].equip[slot], slot)) useful = true;
        }
        if (!useful) return b;
    }
    return -1;
}

static int menu_buttons(void) {
    /* a job in the B menu: equip, use an item, use a skill, mount up */
    int page = sg.menu_page;
    switch (bot.task) {
    case TK_EQUIP:
        if (page == FM_ROOT) return TAP(nav_to(sg.menu_sel, 2, 7));
        if (page == FM_EQUIP_WHO) return TAP(nav_to(sg.menu_who, bot.t_who, imax(1, sg.s.nposse)));
        if (page == FM_EQUIP) return TAP(nav_to(sg.menu_slot, bot.t_slot, EQ_COUNT));
        if (page == FM_EQUIP_PICK) {
            int h = spur_hero_at(bot.t_who);
            int no = 1, want = -1;
            for (int i = 0; i < SPUR_BAG; i++) if (sg.s.bag[i] && spur_can_equip(h, sg.s.bag[i], bot.t_slot)) { if (i == bot.t_item) want = no; no++; }
            if (want < 0) { bot.task = TK_CLOSE; return TAP(SPUR_BACK); }
            int k = nav_to(sg.menu_sel2, want, no);
            if (k == SPUR_OK && !(bot.frame & 1)) bot.task = TK_CLOSE;
            return TAP(k);
        }
        return TAP(SPUR_BACK);
    case TK_ITEM: {
        if (page == FM_ROOT) return TAP(nav_to(sg.menu_sel, 0, 7));
        if (page == FM_ITEM) {
            if (!sg.s.bag[bot.t_item]) { bot.task = TK_CLOSE; return TAP(SPUR_BACK); }
            if (sg.menu_slot == bot.t_item) return TAP(SPUR_OK);
            return TAP(nav_to(sg.menu_slot, bot.t_item, SPUR_BAG));
        }
        if (page == FM_ITEM_DO) return TAP(nav_to(sg.menu_sel2, 0, 2));
        if (page == FM_ITEM_WHO) {
            int k = nav_to(sg.menu_who, bot.t_who, imax(1, sg.s.nposse));
            if (k == SPUR_OK && !(bot.frame & 1)) bot.task = TK_CLOSE;
            return TAP(k);
        }
        return TAP(SPUR_BACK);
    }
    case TK_SKILL: {
        if (page == FM_ROOT) return TAP(nav_to(sg.menu_sel, 1, 7));
        if (page == FM_SKILL_WHO) return TAP(nav_to(sg.menu_who, bot.t_who, imax(1, sg.s.nposse)));
        if (page == FM_SKILL) {
            int list[SK_COUNT];
            int n = spur_skill_list(spur_hero_at(bot.t_who), list), want = -1;
            for (int i = 0; i < n; i++) if (list[i] == bot.t_sk) want = i;
            if (want < 0) { bot.task = TK_CLOSE; return TAP(SPUR_BACK); }
            int k = nav_to(sg.menu_skill, want, n);
            if (k == SPUR_OK && !(bot.frame & 1) && SPUR_SKILL[bot.t_sk].target != SKT_ALLY && SPUR_SKILL[bot.t_sk].target != SKT_DEADALLY) bot.task = TK_CLOSE;
            return TAP(k);
        }
        if (page == FM_SKILL_TGT) {
            int k = nav_to(sg.menu_sel2, bot.t_tgt, imax(1, sg.s.nposse));
            if (k == SPUR_OK && !(bot.frame & 1)) bot.task = TK_CLOSE;
            return TAP(k);
        }
        return TAP(SPUR_BACK);
    }
    case TK_MOUNT:
        if (page == FM_ROOT) {
            int n = sg.map.kind == MK_WORLD && (sg.s.horses || spur_key(KI_NIGHTMARES)) ? 7 : 6;
            if (n < 7) { bot.task = TK_CLOSE; return TAP(SPUR_BACK); }
            int k = nav_to(sg.menu_sel, 6, 7);
            if (k == SPUR_OK && !(bot.frame & 1)) bot.task = TK_CLOSE;
            return TAP(k);
        }
        return TAP(SPUR_BACK);
    default: return TAP(SPUR_BACK);
    }
}

/* ---- the counters ------------------------------------------------------------------------------ */
static int shop_buttons(void) {
    SpurUi *u = &sg.ui;
    if (u->mode == 0) {
        int idx;
        int want = shop_want(u->shop, &idx) ? 0 : sell_want() >= 0 ? 1 : 2;
        return TAP(nav_to(u->sel, want, 3));
    }
    if (u->mode == 1) {
        int idx;
        if (!shop_want(u->shop, &idx)) return TAP(SPUR_BACK);
        int n = 0;
        while (n < SPUR_SHOP_ITEMS && SPUR_SHOP[u->shop].item[n]) n++;
        return TAP(nav_to(u->sel2, idx, n));
    }
    int s = sell_want();
    if (s < 0) return TAP(SPUR_BACK);
    int n = spur_bag_count();
    return TAP(nav_to(u->sel2, s, imax(1, n)));
}

static int pick_buttons(void) {
    SpurUi *u = &sg.ui;
    int want = 0;
    if (bot.intent == IN_BANK) want = bot.bank_mode == 2 ? 1 : 0;
    if (bot.intent == IN_LEAVE) return TAP(SPUR_BACK);
    if (u->sel < want) return TAP(BTN_DOWN);
    if (u->sel > want) return TAP(BTN_UP);
    return TAP(SPUR_OK);
}

static int number_buttons(void) {
    SpurUi *u = &sg.ui;
    /* deposit all but a working purse; withdraw everything */
    int want = u->mode == 1 ? imax(0, imin(u->num_max, sg.s.teeth - 2000)) : u->num_max;
    if (u->num == want) {
        if (!(bot.frame & 1)) { bot.intent = IN_NONE; if (u->mode == 2) bot.bank_map = sg.map_id; }
        return TAP(SPUR_OK);
    }
    static const int POW[7] = {1, 10, 100, 1000, 10000, 100000, 1000000};
    /* the biggest digit that still needs to move */
    int diff = want - u->num;
    int d = 0;
    while (d < 6 && POW[d + 1] <= iabs(diff)) d++;
    if (u->sel < d) return TAP(BTN_LEFT);
    if (u->sel > d) return TAP(BTN_RIGHT);
    return TAP(diff > 0 ? BTN_UP : BTN_DOWN);
}

static int ui_buttons(void) {
    SpurUi *u = &sg.ui;
    switch (u->kind) {
    case UI_TEXT: return TAP(SPUR_OK);
    case UI_YESNO: {
        int last = u->page >= (u->nlines + 3) / 4 - 1;
        if (!last) return TAP(SPUR_OK);
        int want = bot.answer;
        if (u->sel != want) return TAP(BTN_LEFT);
        return TAP(SPUR_OK);
    }
    case UI_MENU: return menu_buttons();
    case UI_SHOP: return shop_buttons();
    case UI_PICK: return pick_buttons();
    case UI_NUMBER: return number_buttons();
    default: return TAP(SPUR_BACK);
    }
}

/* ---- the field ---------------------------------------------------------------------------------- */
static int avg_level(void) {
    int n = 0, s = 0;
    for (int i = 0; i < 4; i++) { int h = spur_hero_at(i); if (h >= 0) { s += sg.s.hero[h].level; n++; } }
    return n ? s / n : 1;
}

static int posse_hp_pct(void) {
    int hp = 0, mx = 0;
    for (int i = 0; i < 4; i++) { int h = spur_hero_at(i); if (h >= 0) { hp += imax(0, sg.s.hero[h].hp); mx += spur_maxhp(h); } }
    return mx ? hp * 100 / mx : 100;
}

static int walk(int dir) {
    static const int B4[4] = {BTN_DOWN, BTN_UP, BTN_LEFT, BTN_RIGHT};
    return dir >= 0 ? B4[dir] : 0;
}

/* face a tile next to us and press A */
static int face_and(int gx, int gy) {
    int x = sg.pos[0].x, y = sg.pos[0].y;
    for (int k = 0; k < 4; k++)
        if (x + DX[k] == gx && y + DY[k] == gy) {
            if (sg.face != k) {
                /* turn by pressing into it: blocked tiles turn without moving */
                return walk(k);
            }
            return TAP(SPUR_OK);
        }
    return 0;
}

/* go to a place on the current map; act there */
static int go_local(int gx, int gy, int kind, bool fly) {
    int d = step_toward(gx, gy, kind, fly);
    if (d == -2) return -2;
    if (d < 0) return -1;
    return walk(d);
}

/* the link on this map that leads out toward a map (or the far end of a pass) */
static bool exit_link(int target, int *x, int *y) {
    int chain[8];
    int n = chain_of(target, chain);
    for (int i = 0; i < n - 1; i++)
        if (chain[i] == sg.map_id)
            for (int k = 0; k < sg.map.nlink; k++)
                if (sg.map.link[k].map == chain[i + 1]) { *x = sg.map.link[k].x; *y = sg.map.link[k].y; return true; }
    /* in a pass, heading out the far side */
    if (bot.dest_end >= 1000) {
        int fx = (bot.dest_end - 1000) / 128, fy = (bot.dest_end - 1000) % 128;
        for (int k = 0; k < sg.map.nlink; k++)
            if (sg.map.link[k].map == M_WORLD && sg.map.link[k].tx == fx && sg.map.link[k].ty == fy) { *x = sg.map.link[k].x; *y = sg.map.link[k].y; return true; }
        /* the roost mine comes out in the valley; from the Roost side its exit leads into the place first */
        if (sg.map_id == M_ROOST && !(fx == SPUR_MAPINFO[M_ROOST].px && fy == SPUR_MAPINFO[M_ROOST].py))
            for (int k = 0; k < sg.map.nlink; k++) if (sg.map.link[k].map == M_ROOSTMINE) { *x = sg.map.link[k].x; *y = sg.map.link[k].y; return true; }
    }
    /* otherwise back the way out: the link toward the outside */
    int up = -1;
    for (int i = 1; i < 8; i++) {
        int c[8];
        int m = chain_of(sg.map_id, c);
        if (m >= 2) up = c[m - 2];
        break;
    }
    for (int k = 0; k < sg.map.nlink; k++) {
        int to = sg.map.link[k].map;
        if (to == up || (up < 0 && (to == M_WORLD || to == SPUR_MAPINFO[sg.map_id].parent))) { *x = sg.map.link[k].x; *y = sg.map.link[k].y; return true; }
    }
    for (int k = 0; k < sg.map.nlink; k++)
        if (sg.map.link[k].map == M_WORLD || sg.map.link[k].map == M_HELL) { *x = sg.map.link[k].x; *y = sg.map.link[k].y; return true; }
    return false;
}

/* a town: walk off the nearest edge */
static int leave_town(void) {
    int best = -1, bx = 0, by = 0;
    int x0 = sg.pos[0].x, y0 = sg.pos[0].y;
    for (int y = 0; y < sg.map.h; y++)
        for (int x = 0; x < sg.map.w; x++) {
            if (!(x == 0 || y == 0 || x == sg.map.w - 1 || y == sg.map.h - 1)) continue;
            if (!spur_walkable(x, y, false)) continue;
            int d = iabs(x - x0) + iabs(y - y0);
            if (best < 0 || d < best) { best = d; bx = x; by = y; }
        }
    if (best < 0) return 0;
    int r = go_local(bx, by, GK_ONTO, false);
    return r < 0 ? 0 : r;
}

/* the big map: where to walk for a target map, flying if the way is shut */
static int world_go(int target, int tx, int ty, int kind) {
    bool have_wings = spur_key(KI_NIGHTMARES);
    if (sg.mounted == 2) {
        /* land next to the goal */
        int ax = tx, ay = ty;
        int d = step_toward(ax, ay, GK_ADJ, true);
        if (d == -2) {
            int t = spur_tile_at(&sg.map, sg.pos[0].x, sg.pos[0].y);
            if (t >= 0 && (SPUR_TILE_FLAGS[t] & TF_LAND) && spur_tile_walk(t)) { bot.task = TK_MOUNT; return TAP(SPUR_BACK); }
            /* not a place to land: try the tiles round about */
            for (int k = 0; k < 4; k++) {
                int nx = sg.pos[0].x + DX[k], ny = sg.pos[0].y + DY[k];
                int t2 = spur_tile_at(&sg.map, nx, ny);
                if (t2 >= 0 && (SPUR_TILE_FLAGS[t2] & TF_LAND) && spur_tile_walk(t2)) return walk(k);
            }
            return walk(bot.frame / 30 % 4);
        }
        if (d >= 0) return walk(d);
        return 0;
    }
    int d = step_toward(tx, ty, kind, false);
    if (d == -2) return -2;
    if (d >= 0) return walk(d);
    if (have_wings && sg.map_id == M_WORLD) {
        /* the way on foot is shut: take wing */
        int t = spur_tile_at(&sg.map, sg.pos[0].x, sg.pos[0].y);
        (void)t;
        bot.task = TK_MOUNT;
        return TAP(SPUR_BACK);
    }
    (void)target;
    return 0;
}

/* go to a map from wherever we are */
static int travel_to(int target) {
    int chain[8];
    int n = chain_of(target, chain);
    bool in_chain = false;
    for (int i = 0; i < n; i++) in_chain |= chain[i] == sg.map_id;
    if (sg.map_id == M_WORLD) {
        int head = chain[0];
        int x, y;
        if (head == M_HELL) { x = 24; y = 27; }
        else world_door(head, 0, &x, &y);
        int r = world_go(target, x, y, GK_ONTO);
        if (r == -2) {
            /* standing on the door already (we came out of it): step off, then back on */
            for (int k = 0; k < 4; k++)
                if (passable(sg.pos[0].x + DX[k], sg.pos[0].y + DY[k], false, -1, -1)) return walk(k);
            return 0;
        }
        return r < 0 ? 0 : r;
    }
    if (sg.map_id == M_HELL) {
        if (in_chain && target != M_HELL) { int r = go_local(32, 6, GK_ONTO, false); return r < 0 ? 0 : r; }
        int r = go_local(32, 46, GK_ONTO, false);
        return r < 0 ? 0 : r;
    }
    if (sg.map.kind == MK_TOWN) return leave_town();
    int x, y;
    if (in_chain || bot.dest_end >= 1000 || exit_link(target, &x, &y)) {
        if (!exit_link(target, &x, &y)) return 0;
        int r = go_local(x, y, GK_ONTO, false);
        return r < 0 ? 0 : r;
    }
    return 0;
}

/* ---- keeping the posse alive ---------------------------------------------------------------------- */
static bool field_heal(void) {
    /* raise the dead with a skill, then patch the hurt */
    int dead = dead_slot();
    for (int i = 0; i < 4 && dead >= 0; i++) {
        int h = spur_hero_at(i);
        if (!spur_alive(h)) continue;
        static const int RAISE[] = {SK_RAISE, SK_SUTURE, SK_MEND};
        for (int k = 0; k < 3; k++)
            if (spur_has_skill(h, RAISE[k]) && sg.s.hero[h].sp >= SPUR_SKILL[RAISE[k]].sp) {
                bot.task = TK_SKILL; bot.t_who = i; bot.t_sk = RAISE[k]; bot.t_tgt = dead;
                return true;
            }
    }
    int weak = weakest_slot(60);
    if (weak < 0) return false;
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (!spur_alive(h)) continue;
        static const int HEAL[] = {SK_MINUET, SK_REEL, SK_SUTURE, SK_MEND};
        for (int k = 0; k < 4; k++)
            if (spur_has_skill(h, HEAL[k]) && sg.s.hero[h].sp >= SPUR_SKILL[HEAL[k]].sp + 4) {
                bot.task = TK_SKILL; bot.t_who = i; bot.t_sk = HEAL[k]; bot.t_tgt = weak;
                return true;
            }
    }
    if (hp_pct(spur_hero_at(weak)) < 40) {
        int it = best_heal_item();
        if (it >= 0 && heal_count() > 2) { bot.task = TK_ITEM; bot.t_item = it; bot.t_who = weak; return true; }
    }
    return false;
}

/* a town's counter: walk into its door */
static int door_of(int service) {
    for (int i = 0; i < sg.map.ndoor; i++) if (sg.map.door[i].service == service) return i;
    return -1;
}

static int use_door(int service, int intent) {
    int d = door_of(service);
    if (d < 0) return -1;
    bot.intent = intent;
    int r = go_local(sg.map.door[d].x, sg.map.door[d].y, GK_ONTO, false);
    if (r == -2) return walk(1);
    return r < 0 ? -1 : r;
}

static bool town_has(int map, int what) {
    int ti = map - SPUR_FIRST_TOWN;
    if (ti < 0 || ti >= SPUR_TOWNS) return false;
    const SpurTownDef *t = &SPUR_TOWN[ti];
    switch (what) {
    case SV_HOTEL: return t->hotel >= 0;
    case SV_CHAPEL: return t->chapel && !(map == M_HOPE && !spur_flag(F_HOPE_FREE));
    case SV_BANK: return t->bank;
    default: return false;
    }
}

/* in a town: rest, raise, shop, bank, equip. Returns buttons, or -1 when there's nothing to do here */
static int town_chores(void) {
    int map = sg.map_id;
    if (posse_hp_pct() >= 98) bot.rest_flag = 0;
    bool hurt = posse_hp_pct() < 85;
    int dead = dead_slot();
    int ti = map - SPUR_FIRST_TOWN;
    if (dead >= 0 && town_has(map, SV_CHAPEL) && sg.s.teeth >= 20 * sg.s.hero[spur_hero_at(dead)].level) {
        int r = use_door(SV_CHAPEL, IN_REVIVE);
        if (r >= 0) return r;
    }
    /* the bank: draw it all out to shop with */
    if (town_has(map, SV_BANK) && sg.s.bank > 0 && bot.bank_map != map) {
        bot.bank_mode = 2;
        int r = use_door(SV_BANK, IN_BANK);
        if (r >= 0) return r;
    }
    /* gear and medicine */
    if (ti >= 0 && ti < SPUR_TOWNS && (bot.shop_map != map || sg.s.teeth > bot.shop_teeth + 300)) {
        int shops[2] = {SPUR_TOWN[ti].shop_a, SPUR_TOWN[ti].shop_b};
        if (map == M_HOLLOW && !spur_flag(F_MISC0)) shops[0] = shops[1] = 0;
        if (map == M_HOPE && !spur_flag(F_HOPE_FREE)) { /* the stores still trade under the devils */ }
        for (int k = 0; k < 2; k++) {
            int idx;
            if (shops[k] > 0 && (shop_want(shops[k], &idx) || sell_want() >= 0)) {
                int r = use_door(k == 0 ? SV_SHOP_A : SV_SHOP_B, IN_SHOP);
                if (r >= 0) return r;
            }
        }
        bot.shop_map = map;
        bot.shop_teeth = sg.s.teeth;
    }
    {
        int who, slot, bag;
        if (find_upgrade(&who, &slot, &bag)) { bot.task = TK_EQUIP; bot.t_who = who; bot.t_slot = slot; bot.t_item = bag; return TAP(SPUR_BACK); }
    }
    /* ... and put back all but a working purse */
    if (town_has(map, SV_BANK) && sg.s.teeth > 3000 && sg.s.bank < 100000) {
        bot.bank_mode = 1;
        int r = use_door(SV_BANK, IN_BANK);
        if (r >= 0) return r;
    }
    if ((hurt || bot.rest_flag) && town_has(map, SV_HOTEL) && sg.s.teeth >= SPUR_TOWN[ti].hotel) {
        bot.answer = 0;
        int r = use_door(SV_HOTEL, IN_REST);
        if (r >= 0) return r;
    }
    bot.rest_flag = 0;
    return -1;
}

/* ---- grinding --------------------------------------------------------------------------------- */
static void grind_spot(int s, int *map, int *x, int *y) {
    *map = M_WORLD;
    if (s <= S_FERRY) { *x = 30; *y = 78; }
    else if (s <= S_AMBROSE) { *x = 74; *y = 66; }
    else if (s <= S_DOOR) { *x = 88; *y = 30; }
    else if (s == S_PLUNGER) { *x = 48; *y = 14; }
    else if (s <= S_CROW) { *x = 75; *y = 47; }
    else if (s <= S_WORM) { *x = 30; *y = 48; }
    else if (s <= S_SPURKEY) { *x = 20; *y = 30; }
    else { *map = M_HELL; *x = 13; *y = 32; }
}
static int grind(void) {
    /* pace up and down near the door of home, letting the wisps come */
    if (sg.map_id != M_WORLD && sg.map_id != M_HELL && sg.map.zone < 0) return -1;
    bot.grind_t++;
    if (bot.grind_t % 24 == 0) bot.grind_dir = (bot.grind_dir + 1 + (bot.frame / 7) % 2) % 4;
    int d = bot.grind_dir;
    int nx = sg.pos[0].x + DX[d], ny = sg.pos[0].y + DY[d];
    if (!passable(nx, ny, false, -1, -1)) { bot.grind_dir = (bot.grind_dir + 1) % 4; return 0; }
    return walk(d);
}

/* ---- the saloon: carry out the chosen four ---------------------------------------------------------- */
static int saloon(void) {
    if (bot.npick == 0) {
        static const int DEF[4] = {H_TUCKER, H_HATTIE, H_HOLLIS, H_SAGE};
        for (int i = 0; i < 4; i++) bot.pick[i] = DEF[i];
        bot.npick = 4;
    }
    for (int i = 0; i < bot.npick; i++) {
        int h = bot.pick[i];
        if (sg.saved[h]) continue;
        int n = h; /* stranger h stands as person h */
        bot.answer = 0;
        int r = go_local(sg.npc_x[n], sg.npc_y[n], GK_ADJ, false);
        if (r == -2) return face_and(sg.npc_x[n], sg.npc_y[n]);
        return r < 0 ? 0 : r;
    }
    bot.answer = 0;
    for (int k = 0; k < sg.map.nlink; k++) {
        int r = go_local(sg.map.link[k].x, sg.map.link[k].y, GK_ONTO, false);
        return r < 0 ? 0 : r;
    }
    return 0;
}

/* ---- the step at hand ----------------------------------------------------------------------------- */
static int do_goal(const Goal *g) {
    if (sg.map_id != g->map) {
        /* through a pass on the way? the far end is where we come out */
        if (sg.map_id == M_WORLD) bot.dest_end = -1;
        return travel_to(g->map);
    }
    bot.dest_end = -1;
    bot.answer = 0;
    int gx = g->x, gy = g->y;
    if (g->npc == -6) return 0; /* just being here does it */
    if (g->npc >= 0 || g->npc == -2 || g->npc == -5) {
        int n = g->npc >= 0 ? g->npc : g->npc == -2 ? find_npc(LOOK_BOSS, -1) : -1;
        if (g->npc == -5)
            for (int i = 0; i < sg.map.nnpc; i++)
                if (spur_npc_visible(i) && sg.map.npc[i].look != LOOK_BOSS && sg.map.npc[i].look != LOOK_ANIMAL) n = i;
        if (n < 0 || !spur_npc_visible(n)) return 0;
        gx = sg.npc_x[n];
        gy = sg.npc_y[n];
        /* across a counter: stand at the counter and talk over it */
        int r = go_local(gx, gy, GK_ADJ, false);
        if (r == -2) return face_and(gx, gy);
        if (r == -1) {
            /* maybe over a counter */
            for (int k = 0; k < 4; k++) {
                int cx = gx + DX[k], cy = gy + DY[k];
                if (spur_tile_at(&sg.map, cx, cy) == T_COUNTER) {
                    int r2 = go_local(cx, cy, GK_ADJ, false);
                    if (r2 == -2) return face_and(cx, cy);
                    if (r2 >= 0) return r2;
                }
            }
            return 0;
        }
        return r;
    }
    if (g->npc == -3) {
        for (int i = 0; i < sg.map.nchest; i++)
            if (sg.map.chest[i].item == gx && !spur_chest_open(sg.map.chest[i].id)) { gx = sg.map.chest[i].x; gy = sg.map.chest[i].y; break; }
    }
    if (g->npc == -4) {
        int d = door_of(SV_STORY);
        if (d < 0) return 0;
        bot.answer = 0;
        int r = go_local(sg.map.door[d].x, sg.map.door[d].y, GK_ONTO, false);
        if (r == -2) return walk(1);
        return r < 0 ? 0 : r;
    }
    if (sg.map.kind == MK_WORLD) {
        int r = world_go(g->map, gx, gy, GK_ADJ);
        if (r == -2) return face_and(gx, gy);
        return r < 0 ? 0 : r;
    }
    int r = go_local(gx, gy, GK_ADJ, false);
    if (r == -2) return face_and(gx, gy);
    return r < 0 ? 0 : r;
}

/* the home town for this step: rest and train there */
/* somewhere to sleep for nothing, when the purse is empty */
static int free_rest(int s) {
    if (s >= S_CLAW || spur_flag(F_REAPER)) return M_HERMITAGE;
    if (s >= S_CHARM) return M_WIDOW;
    if (s == S_PLUNGER) return M_HOLLOW;
    if (s >= S_MOLLY && spur_flag(F_FORT)) return M_FORT;
    return M_STCLARE;
}

static int go_home(int home) {
    int ti = home - SPUR_FIRST_TOWN;
    if (ti >= 0 && ti < SPUR_TOWNS && SPUR_TOWN[ti].hotel > sg.s.teeth) home = free_rest(current_step());
    if (sg.map_id == home) return -1;
    return travel_to(home);
}

static int field_buttons(void) {
    int s = current_step();
    if (s != bot.step && spur_bot_debug >= 1)
        printf("== step %d at frame %d: lv %d teeth %d battles %d map %d\n", s, bot.frame, avg_level(), sg.s.teeth, sg.s.battles, sg.map_id);
    bot.step = s;
    if (spur_bot_goal >= 0 && s > spur_bot_goal) return 0;
    if (sg.map.kind != MK_TOWN) bot.bank_map = -1;
    if (s == S_PICK) return saloon();
    if (s == S_DONE) return 0;
    if (sg.moving) return 0;
    /* stuck on the same tile too long: shuffle a little */
    if (sg.pos[0].x == bot.last_x && sg.pos[0].y == bot.last_y && sg.map_id == bot.last_map) bot.stuck++;
    else { bot.stuck = 0; bot.last_x = sg.pos[0].x; bot.last_y = sg.pos[0].y; bot.last_map = sg.map_id; }
    if (bot.stuck > 600 && bot.stuck < 640) return walk((bot.frame / 10) % 4);
    if (bot.stuck >= 640) bot.stuck = 0;

    /* mend in the field when it helps */
    if (sg.map.kind != MK_TOWN && field_heal()) return TAP(SPUR_BACK);
    int home = STEP[s].home;
    int need = STEP[s].level + bot_extra[s];
    bool weak = avg_level() < need;
    bool beaten = posse_hp_pct() < 45;
    if (sg.map.kind == MK_TOWN || sg.map_id == M_WIDOW) {
        int r = town_chores();
        if (r >= 0) return r;
        if (sg.map_id == M_WIDOW && posse_hp_pct() < 90) {
            /* the widow's bed */
            bot.answer = 0;
            int rr = go_local(2, 2, GK_ADJ, false);
            if (rr == -2) return face_and(2, 2);
            if (rr >= 0) return rr;
        }
    }
    /* the dead wait for a chapel, unless someone can raise them */
    int dead = dead_slot();
    if (dead >= 0) {
        bool raise = false;
        for (int i = 0; i < 4; i++) { int h = spur_hero_at(i); raise |= spur_alive(h) && spur_has_skill(h, SK_RAISE); }
        int ch = town_has(home, SV_CHAPEL) ? home : sg.s.chapel;
        bool afford = sg.s.teeth >= 20 * sg.s.hero[spur_hero_at(dead)].level;
        if (!raise && afford && spur_bag_find(IT_HOLYOIL) < 0 && sg.map_id != ch && ch >= SPUR_FIRST_TOWN) {
            int r = travel_to(ch);
            if (r) return r;
        }
    }
    if (beaten) {
        bot.rest_flag = 1;
        int r = go_home(home);
        if (r >= 0) return r;
    }
    if (weak) {
        /* train in open country suited to the step */
        int gm, gx, gy;
        grind_spot(s, &gm, &gx, &gy);
        if (sg.map.kind == MK_TOWN || sg.map.kind == MK_PLACE) {
            if (sg.map.kind == MK_TOWN) return leave_town();
            int r = travel_to(gm);
            if (r) return r;
        }
        if (sg.map_id == gm) {
            int dd = iabs(sg.pos[0].x - gx) + iabs(sg.pos[0].y - gy);
            if (dd > 10) { int r = world_go(gm, gx, gy, GK_ADJ); if (r >= 0 && r != -2) return r; }
            return grind();
        }
        int r = travel_to(gm);
        if (r) return r;
    }
    Goal g = step_goal(s);
    /* a boss waits: go in rested and whole */
    bool boss_step = g.npc == -2 || s == S_TALLY || s == S_FORT || s == S_MOLLY || s == S_MIMIC || s == S_FINAL;
    /* a fallen one only holds things up while there's a way to raise them */
    int fallen = dead_slot();
    if (fallen >= 0) {
        bool way = spur_bag_find(IT_HOLYOIL) >= 0 || sg.s.teeth >= 20 * sg.s.hero[spur_hero_at(fallen)].level;
        for (int i = 0; i < 4; i++) { int h = spur_hero_at(i); way |= spur_alive(h) && spur_has_skill(h, SK_RAISE); }
        if (!way) fallen = -1;
    }
    if (boss_step && sg.map_id != g.map && (posse_hp_pct() < 80 || fallen >= 0)) {
        if (sg.map.kind != MK_TOWN && field_heal()) return TAP(SPUR_BACK);
        bot.rest_flag = 1;
        int r = go_home(home);
        if (r >= 0) return r;
    }
    if (boss_step && sg.map_id == g.map && sg.map.kind == MK_DUNGEON && (posse_hp_pct() < 75 || fallen >= 0)) {
        if (field_heal()) return TAP(SPUR_BACK);
        if (posse_hp_pct() < 55 || fallen >= 0) {
            bot.rest_flag = 1;
            int r = go_home(home);
            if (r >= 0) return r;
        }
    }
    return do_goal(&g);
}

/* ---- the whole thing -------------------------------------------------------------------------------- */
int spur_bot_buttons(void) {
    bot.frame++;
    if (spur_bot_debug >= 2 && bot.frame % 300 == 0)
        printf("bot f%d step %d map %d (%d,%d) state %d ui %d page %d task %d who %d slot %d item %d sel %d msl %d lv %d hp %d%% teeth %d\n", bot.frame,
               current_step(), sg.map_id, sg.pos[0].x, sg.pos[0].y, sg.state, sg.ui.kind, sg.menu_page, bot.task, bot.t_who,
               bot.t_slot, bot.t_item, sg.menu_sel, sg.menu_slot, avg_level(), posse_hp_pct(), sg.s.teeth);
    if (sg.god && bot.frame < 2) memset(bat_cmd, 0, sizeof bat_cmd);
    switch (sg.state) {
    case GS_TITLE: {
        int want = spur_records.valid ? 0 : 1;
        if (sg.title_sel != want) return TAP(BTN_DOWN);
        return TAP(SPUR_OK);
    }
    case GS_INTRO: return TAP(SPUR_OK);
    case GS_BATTLE: {
        if (spur_bot_debug >= 1 && sg.b.phase == BP_INTRO && sg.b.phase_t == 1) {
            printf("  battle troop %d:", sg.b.troop);
            for (int i = 0; i < sg.b.nfoe; i++) printf(" %s", SPUR_ENEMY[sg.b.foe[i].e].name);
            printf(" | hp");
            for (int i = 0; i < 4; i++) { int h = spur_hero_at(i); if (h >= 0) printf(" %d/%d", sg.s.hero[h].hp, spur_maxhp(h)); }
            printf(" lv %d\n", avg_level());
        }
        if (sg.b.phase == BP_LOSE && sg.b.phase_t == 1 && sg.b.boss) bot_extra[current_step()] += 2;
        if (spur_bot_debug >= 1 && (sg.b.phase == BP_WIN || sg.b.phase == BP_LOSE) && sg.b.phase_t == 1) {
            printf("    %s after %d rounds | hp", sg.b.phase == BP_WIN ? "won" : "LOST", sg.b.turn);
            for (int i = 0; i < 4; i++) { int h = spur_hero_at(i); if (h >= 0) printf(" %d", sg.s.hero[h].hp); }
            printf("\n");
        }
        if (sg.b.phase == BP_COMMAND && sg.b.phase_t == 1) for (int i = 0; i < 4; i++) bat_cmd[i] = -1;
        return battle_buttons();
    }
    case GS_ENDING: return sg.end_t > 20 ? TAP(SPUR_OK) : 0;
    case GS_FIELD:
        if (spur_ui_open()) return ui_buttons();
        if (bot.task == TK_CLOSE) bot.task = TK_NONE;
        if (bot.task != TK_NONE && !spur_ui_open()) {
            /* open the menu for the job */
            return TAP(SPUR_BACK);
        }
        return field_buttons();
    default: return 0;
    }
}
