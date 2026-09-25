/* DUNE EXPRESS - the rules: a platformer on a moving train that runs in
 * real time until a lawman is up and about, then in turns (ten seconds for
 * the outlaws, three and a half for the lawmen). Pure C, no shell
 * dependencies (build with -DDX_NO_SHELL for tools).
 * Every rule is listed with its source in docs/games/28-dune-express.md. */
#include "dune.h"

#define GRAV 0.28f
#define MAXFALL 5.0f
#define WALK 1.25f
#define LAW_WALK 0.9f
#define JUMP_V (-4.9f)
#define JUMP_CUT 0.45f       /* letting go of A early: a short hop */
#define HEAVY_JUMP_V (-3.0f)
#define BOOTS_V (-5.6f)
#define LAW_HOP_V (-3.9f)    /* a lawman hopping over a box in his way */
#define CLIMB 1.0f
#define GLIDE 0.7f
#define PUSH_DELAY 12        /* walking into a thing this long starts to push it */
#define PUSH_SPEED 0.6f
#define RAM_WALK 0.9f
#define RAM_RUN 2.6f
#define SIGHT 9              /* tiles a lawman can see */
#define AW 12
#define AH 30
#define DUCK_H 14

static int fl(float v) { return (int)floorf(v); }
static int iabs_c(int v) { return v < 0 ? -v : v; }
static int imin_c(int a, int b) { return a < b ? a : b; }

/* ------------------------------------------------------------------ */
/* tiles                                                                */

int dx_tile(const DxWorld *w, int tx, int ty) {
    if (tx < 0 || tx >= w->w || ty < 0 || ty >= DX_H) return TL_AIR;
    return w->tile[ty][tx];
}

bool dx_solid(const DxWorld *w, int tx, int ty) {
    int t = dx_tile(w, tx, ty);
    return t == TL_ROOF || t == TL_WALL || t == TL_FLOOR || t == TL_ARMOR || t == TL_GATE || t == TL_COUPLING || t == TL_CELL;
}

/* plank walls, floors and roofs break; armor only to blasts and the iron fist */
static bool breakable(int t, bool hard) {
    if (t == TL_ROOF || t == TL_WALL || t == TL_FLOOR) return true;
    return hard && (t == TL_ARMOR);
}

int dx_car_at(const DxWorld *w, float x) {
    int tx = fl(x / DX_T);
    if (tx < 0 || tx >= w->w) return -1;
    return w->car_of[tx] == 255 ? -1 : w->car_of[tx];
}

static void clear_tile(DxWorld *w, int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= w->w || ty >= DX_H) return;
    w->tile[ty][tx] = TL_AIR;
    w->dmg[ty][tx] = 0;
    w->tflash[ty][tx] = 0;
}

/* ------------------------------------------------------------------ */
/* spawning                                                             */

static int add_obj(DxWorld *w, int type, float x, float y) {
    for (int i = 0; i < DX_MAX_OBJS; i++) {
        DxObj *o = &w->o[i];
        if (o->alive) continue;
        memset(o, 0, sizeof *o);
        o->type = (uint8_t)type;
        o->alive = 1;
        o->x = x;
        o->y = y;
        o->w = 14;
        o->h = 14;
        o->held = -1;
        o->marked = 1;
        if (type == OB_COIN || type == OB_AMMO || type == OB_POWER) { o->w = 8; o->h = 8; }
        if (type == OB_RAM) { o->w = 26; o->h = 14; }
        if (type == OB_CAMEL) { o->w = 28; o->h = 30; }
        if (type == OB_GUN) o->ammo = DX_GUN_SHOTS;
        if (i >= w->no) w->no = i + 1;
        return i;
    }
    return -1;
}

static int add_actor(DxWorld *w, int kind, int who, float x, float y, int face) {
    if (w->na >= DX_MAX_ACTORS) return -1;
    DxActor *a = &w->a[w->na];
    memset(a, 0, sizeof *a);
    a->kind = (uint8_t)kind;
    a->who = (uint8_t)who;
    a->alive = 1;
    a->x = x;
    a->y = y;
    a->face = (uint8_t)face;
    a->carry = -1;
    a->armed = kind != AK_OUTLAW;
    a->state = LS_GUARD;
    a->ground = 1; /* everyone starts on their feet */
    return w->na++;
}

static void compute_cars(DxWorld *w) {
    w->ncars = 0;
    int cur = -1;
    for (int x = 0; x < w->w; x++) {
        /* a car is wherever there is a floor or a roof; couplings and the
         * ladders between cars don't count */
        int t3 = w->tile[3][x], t7 = w->tile[7][x];
        bool body = t7 == TL_FLOOR || t7 == TL_ARMOR || t3 == TL_ROOF || t3 == TL_ARMOR;
        if (!body) { w->car_of[x] = 255; cur = -1; continue; }
        if (cur < 0 && w->ncars < DX_MAX_CARS) {
            cur = w->ncars++;
            w->car_x0[cur] = (int16_t)x;
        }
        if (cur < 0) { w->car_of[x] = 255; continue; }
        w->car_of[x] = (uint8_t)cur;
        w->car_x1[cur] = (int16_t)x;
    }
}

int dx_load(DxWorld *w, const DxMission *m, int mission) {
    memset(w, 0, sizeof *w);
    w->mission = (uint8_t)mission;
    int width = (int)strlen(m->tiles[0]);
    if (width > DX_MAX_W) return 1;
    w->w = width;
    int outlaw_n = 0;
    for (int y = 0; y < DX_H; y++) {
        if ((int)strlen(m->tiles[y]) != width || (int)strlen(m->things[y]) != width) return 2;
        for (int x = 0; x < width; x++) {
            int t;
            switch (m->tiles[y][x]) {
            case '=': t = TL_ROOF; break;
            case '#': t = TL_WALL; break;
            case '_': t = TL_FLOOR; break;
            case 'A': t = TL_ARMOR; break;
            case 'G': t = TL_GATE; break;
            case 'g': t = TL_GATE_OPEN; break;
            case 'H': t = TL_LADDER; break;
            case '-': t = TL_SHELF; break;
            case 'w': t = TL_WHEELS; break;
            case 'c': t = TL_COUPLING; break;
            case 'X': t = TL_CELL; break;
            default: t = TL_AIR; break;
            }
            w->tile[y][x] = (uint8_t)t;
        }
    }
    for (int y = 0; y < DX_H; y++)
        for (int x = 0; x < width; x++) {
            char c = m->things[y][x];
            float ax = (float)(x * DX_T + 2), ay = (float)((y - 1) * DX_T + 2);
            float ox = (float)(x * DX_T + 1), oy = (float)(y * DX_T + 2);
            int id;
            switch (c) {
            case 'K': case 'V': case 'C': {
                int slot = c == 'K' ? 0 : 1;
                id = add_actor(w, AK_OUTLAW, m->who[slot], ax, ay, 1);
                if (id >= 0) {
                    w->a[id].ammo = m->start_ammo;
                    if (c == 'C') w->a[id].escaped = 2; /* locked in the cell until freed */
                }
                outlaw_n++;
                break;
            }
            case 'l': case 'r': id = add_actor(w, AK_GUARD, 0, ax, ay, c == 'r'); break;
            case 'p': case 'q':
                id = add_actor(w, AK_GUARD, 0, ax, ay, c == 'q');
                if (id >= 0) w->a[id].state = LS_PATROL;
                break;
            case 'u': case 'v': {
                /* a guard with his hands full: a crate, no gun */
                id = add_actor(w, AK_GUARD, 0, ax, ay, c == 'v');
                if (id >= 0) {
                    int ob = add_obj(w, OB_CRATE, ax, ay - 14);
                    w->a[id].armed = 0;
                    w->a[id].carry = (int16_t)ob;
                    w->a[id].carries_default = 1;
                    if (ob >= 0) w->o[ob].held = (int16_t)id;
                }
                break;
            }
            case 'S': id = add_actor(w, AK_GOVERNOR, 0, ax, ay, 0); break;
            case '$': add_obj(w, OB_LOOT, ox, oy); break;
            case '%': id = add_obj(w, OB_LOOT, ox, oy); if (id >= 0) w->o[id].marked = 0; break;
            case 'b': add_obj(w, OB_BARREL, ox, oy); break;
            case 'x': add_obj(w, OB_CRATE, ox, oy); break;
            case 'k': add_obj(w, OB_ANVIL, ox, oy); break;
            case 'd': add_obj(w, OB_DYNAMITE, ox, oy); break;
            case 'h': add_obj(w, OB_GOOSE, ox, oy); break;
            case 'm': case 'n': id = add_obj(w, OB_RAM, ox, oy); if (id >= 0) w->o[id].face = c == 'n'; break;
            case 'T': case 't': id = add_obj(w, OB_GUN, ox, oy); if (id >= 0) w->o[id].face = c == 't'; break;
            case 's': add_obj(w, OB_LEVER, ox, oy); break;
            case 'o': add_obj(w, OB_AMMO, ox + 3, oy + 6); break;
            case 'Z': add_obj(w, OB_CAMEL, ox, oy - 16); break;
            default:
                if (c >= '1' && c <= '6') {
                    id = add_obj(w, OB_CRATE, ox, oy);
                    if (id >= 0) w->o[id].content = (uint8_t)(c - '0');
                } else if (c >= '7' && c <= '9') {
                    /* a barrel hiding spare rounds, a charm or the quick holster */
                    id = add_obj(w, OB_BARREL, ox, oy);
                    if (id >= 0) w->o[id].content = (uint8_t)(c == '7' ? PW_ROUNDS : c == '8' ? PW_CHARM : PW_QUICK);
                }
                break;
            }
        }
    if (outlaw_n == 0) return 3;
    /* number the strongboxes */
    int lid = 0;
    for (int i = 0; i < w->no; i++)
        if (w->o[i].alive && w->o[i].type == OB_LOOT) w->o[i].loot_id = (uint8_t)(lid++ % 8);
    for (int i = 0; i < w->na; i++)
        if (w->a[i].kind != AK_OUTLAW) w->lawmen_total++;
    compute_cars(w);
    w->ctrl = 0;
    for (int i = 0; i < w->na; i++)
        if (w->a[i].kind == AK_OUTLAW && !w->a[i].escaped) { w->ctrl = i; break; }
    w->objective = m->objective;
    w->need = m->need;
    w->master = m->seconds * 60;
    w->phase = PH_FREE;
    rng_seed(&w->rng, 2800u + (uint64_t)mission);
    return 0;
}

/* ------------------------------------------------------------------ */
/* boxes and collisions                                                 */

static float ah(const DxActor *a) { return a->duck || a->hidden ? DUCK_H : AH; }
static float atop(const DxActor *a) { return a->y + (AH - ah(a)); }

static bool obj_solid(const DxObj *o) {
    if (!o->alive || o->held >= 0 || o->thrower) return false;
    return o->type == OB_BARREL || o->type == OB_CRATE || o->type == OB_ANVIL || o->type == OB_LOOT || o->type == OB_LEVER ||
           o->type == OB_GUN || o->type == OB_RAM;
}

static bool pushable(const DxObj *o) {
    return o->type == OB_BARREL || o->type == OB_CRATE || o->type == OB_ANVIL || o->type == OB_LOOT || o->type == OB_LEVER ||
           o->type == OB_GUN || o->type == OB_DYNAMITE;
}

static bool box_hits_tiles(const DxWorld *w, float x, float y, float bw, float bh) {
    int x0 = fl(x / DX_T), x1 = fl((x + bw - 1) / DX_T), y0 = fl(y / DX_T), y1 = fl((y + bh - 1) / DX_T);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (dx_solid(w, tx, ty)) return true;
    return false;
}

static int box_hits_objs(const DxWorld *w, float x, float y, float bw, float bh, int ignore) {
    for (int i = 0; i < w->no; i++) {
        const DxObj *o = &w->o[i];
        if (i == ignore || !obj_solid(o)) continue;
        if (x < o->x + o->w && o->x < x + bw && y < o->y + o->h && o->y < y + bh) return i;
    }
    return -1;
}

static bool overlap(float ax, float ay, float aw, float ahh, float bx, float by, float bw, float bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ahh;
}

/* can an actor stand here (for the fall-through shelves: only from above) */
static bool shelf_under(const DxWorld *w, float x, float bottom_before, float bottom_after) {
    int tx0 = fl(x / DX_T), tx1 = fl((x + AW - 1) / DX_T);
    int row = fl(bottom_after / DX_T);
    if (fl(bottom_before / DX_T) >= row && fmodf(bottom_before, DX_T) > 0.5f) return false;
    for (int tx = tx0; tx <= tx1; tx++) {
        int t = dx_tile(w, tx, row);
        if (t == TL_SHELF) return true;
        if (t == TL_LADDER && dx_tile(w, tx, row - 1) != TL_LADDER) return true; /* the top of a ladder */
    }
    return false;
}

/* an outlaw can stand on a lawman's head (lawmen only look straight ahead) */
static int head_under(const DxWorld *w, int ai, float x, float bottom_before, float bottom_after) {
    const DxActor *me = &w->a[ai];
    if (me->kind != AK_OUTLAW) return -1;
    for (int i = 0; i < w->na; i++) {
        const DxActor *l = &w->a[i];
        if (l->kind == AK_OUTLAW || !l->alive || l->dying) continue;
        float top = atop(l);
        if (x < l->x + AW - 3 && l->x + 3 < x + AW && bottom_before <= top + 1 && bottom_after >= top) return i;
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* noise, kills, breaking                                               */

/* the car at x, or, in a gap between two, the car behind it */
static int car_near(const DxWorld *w, float x) {
    int c = dx_car_at(w, x);
    if (c < 0) c = dx_car_at(w, x - DX_T);
    if (c < 0) c = dx_car_at(w, x + DX_T);
    return c;
}

#define HEARING (10 * DX_T)     /* how far a gunshot carries */

static void set_target(DxActor *l, float x, float feet) {
    l->has_tgt = 1;
    l->tgt_x = x;
    l->tgt_y = feet;
    l->look_t = 0;
}

/* a gunshot or a blast: every lawman nearby is alert and heads for it */
static void alert_near(DxWorld *w, float x, float feet) {
    int car = car_near(w, x);
    for (int i = 0; i < w->na; i++) {
        DxActor *a = &w->a[i];
        if (a->kind == AK_OUTLAW || !a->alive) continue;
        if (fabsf(a->x + 6 - x) <= HEARING) {
            if (a->state != LS_ALERT) w->ev_alert++;
            a->state = LS_ALERT;
            set_target(a, x, feet);
        }
    }
    /* rams spook at gunshots too */
    for (int i = 0; i < w->no; i++) {
        DxObj *o = &w->o[i];
        if (o->alive && o->type == OB_RAM && o->timer == 0 && dx_car_at(w, o->x) == car) o->timer = 1;
    }
}

/* dir: which way a shot or a blow throws the body (0: it just drops) */
static void kill_actor(DxWorld *w, int i, int why, int dir) {
    DxActor *a = &w->a[i];
    if (!a->alive) return;
    if (a->kind == AK_OUTLAW && a->charm && why != WHY_FELL) {
        /* the horseshoe takes the blow, once */
        a->charm = 0;
        a->stun = 30;
        return;
    }
    a->alive = 0;
    a->climb = 0;
    a->hidden = 0;
    if (a->carry >= 0) {
        DxObj *o = &w->o[a->carry];
        o->held = -1;
        o->vy = -1;
        a->carry = -1;
    }
    if (why != WHY_FELL) {
        /* shot, blown or trampled: the body flies off in an arc to the rails */
        a->dying = 1;
        a->vx = (dir ? dir : (a->face ? -1 : 1)) * 1.8f;
        a->vy = -3.6f;
    }
    if (a->kind == AK_OUTLAW) {
        if (!w->result) w->end_t = DX_END_FRAMES;
        w->result = DX_LOST;
        w->why = (uint8_t)why;
    } else if (a->kind == AK_GOVERNOR) {
        w->governor_dead = 1;
    } else {
        w->kills++;
    }
}

static void spill(DxWorld *w, DxObj *o) {
    /* read everything first: the new things may reuse this very slot */
    float cx = o->x + o->w / 2.0f, cy = o->y;
    uint8_t type = o->type, content = o->content, lid = o->loot_id;
    if (type == OB_LOOT) {
        for (int k = 0; k < 4; k++) {
            int c = add_obj(w, OB_COIN, cx - 4 + (k - 1.5f) * 4, cy);
            if (c >= 0) { w->o[c].vx = (k - 1.5f) * 0.35f; w->o[c].vy = -1.6f - (k % 2) * 0.4f; w->o[c].loot_id = lid; }
        }
    } else if (content) {
        int c = add_obj(w, OB_POWER, cx - 4, cy);
        if (c >= 0) { w->o[c].content = content; w->o[c].vy = -2.0f; }
    }
}

static void explode(DxWorld *w, float cx, float cy, float reach_px, float kill_px);

/* a barrel someone hides in breaks: out they tumble, unhurt */
static void unhide(DxWorld *w, int oi) {
    for (int i = 0; i < w->na; i++) {
        DxActor *a = &w->a[i];
        if (!a->hidden) continue;
        if (fabsf(a->x - (w->o[oi].x + 1)) < 8) { a->hidden = 0; a->duck = 1; }
    }
}

static void break_obj(DxWorld *w, int i) {
    DxObj *o = &w->o[i];
    if (!o->alive) return;
    if (o->held >= 0) { w->a[o->held].carry = -1; o->held = -1; }
    switch (o->type) {
    case OB_BARREL: case OB_CRATE: case OB_LOOT:
        if (o->type == OB_BARREL && o->timer == -5) unhide(w, i);
        o->alive = 0;
        spill(w, o);
        w->ev_break++;
        break;
    case OB_DYNAMITE:
        o->alive = 0;
        explode(w, o->x + 7, o->y + 7, 24, 28);
        break;
    case OB_GOOSE: case OB_ANVIL: case OB_LEVER: case OB_GUN:
        o->alive = 0;
        w->ev_break++;
        break;
    default: break;
    }
}

static void flip_gates(DxWorld *w) {
    for (int y = 0; y < DX_H; y++)
        for (int x = 0; x < w->w; x++) {
            if (w->tile[y][x] == TL_GATE) w->tile[y][x] = TL_GATE_OPEN;
            else if (w->tile[y][x] == TL_GATE_OPEN) w->tile[y][x] = TL_GATE;
        }
    w->ev_gate++;
    for (int i = 0; i < w->no; i++)
        if (w->o[i].alive && w->o[i].type == OB_LEVER) w->o[i].face ^= 1;
    /* the cell in the jailbreak: whoever was locked up is free */
    for (int i = 0; i < w->na; i++)
        if (w->a[i].kind == AK_OUTLAW && w->a[i].escaped == 2) {
            w->a[i].escaped = 0;
            w->rescue_open = 1;
            /* the rescuer slips away to the camel and the freed outlaw takes over */
            int old = w->ctrl;
            w->a[old].escaped = 1;
            w->ctrl = i;
        }
}

/* a blast: walls, roofs and plate within reach go (the floor holds), and
 * whoever stands within kill_px dies */
static void explode(DxWorld *w, float cx, float cy, float reach_px, float kill_px) {
    w->ev_boom++;
    int tx0 = fl((cx - reach_px) / DX_T), tx1 = fl((cx + reach_px) / DX_T), ty0 = fl((cy - reach_px) / DX_T), ty1 = fl((cy + reach_px) / DX_T);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++)
            if (tx >= 0 && ty >= 0 && tx < w->w && ty < DX_H && breakable(w->tile[ty][tx], true) && w->tile[ty][tx] != TL_FLOOR)
                clear_tile(w, tx, ty);
    for (int i = 0; i < w->na; i++) {
        DxActor *a = &w->a[i];
        if (!a->alive || a->escaped) continue;
        float dx = a->x + 6 - cx, dy = a->y + 15 - cy;
        if (dx * dx + dy * dy < kill_px * kill_px) kill_actor(w, i, WHY_BLAST, dx > 0 ? 1 : -1);
    }
    for (int i = 0; i < w->no; i++) {
        DxObj *o = &w->o[i];
        if (!o->alive || o->type == OB_CAMEL || o->type == OB_COIN || o->type == OB_POWER || o->type == OB_AMMO) continue;
        float dx = o->x + o->w / 2.0f - cx, dy = o->y + o->h / 2.0f - cy;
        if (dx * dx + dy * dy < (kill_px + 2) * (kill_px + 2)) break_obj(w, i);
    }
    alert_near(w, cx, cy + 15);
}

/* ------------------------------------------------------------------ */
/* sight                                                                */

/* does this thing stop a look or a bullet? */
static bool obj_blocks(const DxObj *o, int shooter) {
    if (!o->alive || o->type == OB_COIN || o->type == OB_POWER || o->type == OB_AMMO || o->type == OB_CAMEL) return false;
    if (o->held >= 0) return o->held != shooter;
    return obj_solid(o) || o->thrower;
}

/* walk a ray from (x, y) in dir; returns what it meets first:
 * 1 + actor index, -(1 + object index), or 0 (a wall, or nothing in range).
 * *hx is where it stopped. */
static int ray_ex(const DxWorld *w, float x, float y, int dir, int shooter, int skip_obj, bool want_outlaws, bool want_law,
                  int max_tiles, float *hx) {
    float px = x;
    for (int step = 0; step < max_tiles * 4; step++) {
        px = x + dir * step * 4.0f;
        int tx = fl(px / DX_T), ty = fl(y / DX_T);
        if (px < -DX_T || px > (w->w + 1) * DX_T) break;
        if (dx_solid(w, tx, ty)) { if (hx) *hx = px; return 0; }
        for (int i = 0; i < w->no; i++) {
            const DxObj *o = &w->o[i];
            if (i == skip_obj || !obj_blocks(o, shooter)) continue;
            if (px >= o->x && px < o->x + o->w && y >= o->y && y < o->y + o->h) { if (hx) *hx = px; return -(1 + i); }
        }
        for (int i = 0; i < w->na; i++) {
            const DxActor *a = &w->a[i];
            if (i == shooter || !a->alive || a->hidden || a->escaped || a->dying) continue;
            bool out = a->kind == AK_OUTLAW;
            if ((out && !want_outlaws) || (!out && !want_law)) continue;
            if (px >= a->x && px < a->x + AW && y >= atop(a) && y < a->y + AH) { if (hx) *hx = px; return 1 + i; }
        }
    }
    if (hx) *hx = px;
    return 0;
}

static int ray(const DxWorld *w, float x, float y, int dir, int shooter, bool want_outlaws, bool want_law, int max_tiles) {
    return ray_ex(w, x, y, dir, shooter, -1, want_outlaws, want_law, max_tiles, NULL);
}

/* a lawman looks straight ahead at head and knee height; other lawmen don't
 * block his view, anything solid does */
bool dx_sees(const DxWorld *w, int li, int target) {
    const DxActor *l = &w->a[li];
    if (!l->alive || l->stun) return false;
    int dir = l->face ? 1 : -1;
    float ex = l->x + 6 + dir * 6;
    for (int k = 0; k < 2; k++) {
        int hit = ray(w, ex, l->y + (k ? 24 : 6), dir, li, true, false, SIGHT);
        if (hit == 1 + target) return true;
    }
    return false;
}

int dx_active_lawmen(const DxWorld *w) {
    int n = 0;
    for (int i = 0; i < w->na; i++) {
        const DxActor *a = &w->a[i];
        if (a->kind != AK_OUTLAW && a->alive && a->state != LS_GUARD) n++;
    }
    return n;
}

/* a streak of light where a bullet went: the hit itself is dealt at once */
static void tracer(DxWorld *w, float x, float y, int dir, float len, int owner, bool hostile) {
    for (int i = 0; i < DX_MAX_SHOTS; i++) {
        DxShot *s = &w->s[i];
        if (s->alive) continue;
        memset(s, 0, sizeof *s);
        s->alive = 1;
        s->owner = (int8_t)owner;
        s->x = x;
        s->y = y;
        s->vx = dir * 9.0f;
        s->hostile = hostile;
        s->tracer = 1;
        s->range = len;
        break;
    }
}

/* a gunshot: whoever or whatever is first in line takes it, there and then.
 * shooter: actor index, or -2 - object index for a crank gun. A lawman's
 * bullet takes whoever is in the way, a fellow lawman too; an outlaw's
 * passes the other outlaw. */
static void shoot(DxWorld *w, int shooter, float x, float y, int dir, bool blast) {
    bool from_gun = shooter <= -2;
    int gun = from_gun ? -2 - shooter : -1;
    bool from_law = !from_gun && w->a[shooter].kind != AK_OUTLAW;
    bool hits_outlaws = from_gun || from_law;
    float hx = x;
    int hit = ray_ex(w, x, y, dir, from_gun ? -1 : shooter, gun, hits_outlaws, true, 40, &hx);
    tracer(w, x, y, dir, fabsf(hx - x), from_gun ? -2 : shooter, hits_outlaws);
    w->ev_shot++;
    if (hit > 0) {
        DxActor *t = &w->a[hit - 1];
        kill_actor(w, hit - 1, from_gun ? WHY_GUN : WHY_SHOT, dir);
        if (t->kind != AK_OUTLAW) alert_near(w, t->x + 6, t->y + AH);
    } else if (hit < 0) {
        int oi = -hit - 1;
        DxObj *o = &w->o[oi];
        if (o->type == OB_LEVER) flip_gates(w);
        else if (o->type == OB_RAM) { if (o->timer == 0) o->timer = 1; }
        else if (o->type != OB_ANVIL && o->type != OB_GUN) break_obj(w, oi); /* an anvil just rings */
    }
    /* blast rounds: a three by three tile blast where the bullet stops */
    if (blast && !from_law && !from_gun) {
        int ctx = fl(hx / DX_T), cty = fl(y / DX_T);
        explode(w, (ctx + 0.5f) * DX_T, (cty + 0.5f) * DX_T, DX_T, 22);
    }
    if (!from_gun) {
        const DxActor *s = &w->a[shooter];
        alert_near(w, s->x + 6, s->y + AH);
    } else {
        alert_near(w, w->o[gun].x + 7, w->o[gun].y + 14);
    }
}

/* ------------------------------------------------------------------ */
/* the outlaw's hands                                                   */

static bool liftable(const DxObj *o) {
    return o->alive && o->held < 0 && (o->type == OB_BARREL || o->type == OB_CRATE || o->type == OB_ANVIL || o->type == OB_DYNAMITE ||
                                        o->type == OB_GOOSE || o->type == OB_LEVER || o->type == OB_LOOT || o->type == OB_GUN);
}

static int obj_in_front(DxWorld *w, DxActor *a, bool low) {
    float hx = a->face ? a->x + AW - 2 : a->x - 12, hy = low ? a->y + 16 : a->y + 4;
    for (int i = 0; i < w->no; i++) {
        DxObj *o = &w->o[i];
        if (!o->alive || o->held >= 0 || o->type == OB_COIN || o->type == OB_POWER || o->type == OB_AMMO || o->type == OB_CAMEL) continue;
        if (o->type == OB_BARREL && o->timer == -5) continue; /* someone's inside */
        if (overlap(hx, hy, 14, low ? 14 : 24, o->x, o->y, o->w, o->h)) return i;
    }
    return -1;
}

static void pick_up(DxWorld *w, int ai, int oi) {
    DxActor *a = &w->a[ai];
    DxObj *o = &w->o[oi];
    if (o->type == OB_GUN && a->kind == AK_OUTLAW) o->friendly = 1; /* picked up: it's ours now */
    o->held = (int16_t)ai;
    o->thrower = 0;
    o->timer = 0;
    a->carry = (int16_t)oi;
    a->gun = 0;
}

static void let_go(DxWorld *w, DxActor *a, bool thrown, bool place) {
    if (a->carry < 0) return;
    DxObj *o = &w->o[a->carry];
    int dir = a->face ? 1 : -1;
    int ai = (int)(a - w->a);
    o->held = -1;
    a->carry = -1;
    o->face = a->face;
    if (thrown) {
        /* thrown from the chest, so it fits through a doorway */
        o->x = a->x + 6 - o->w / 2.0f + dir * 8;
        o->y = a->y + 2;
        o->vx = dir * 6.4f + a->vx * 0.5f;
        o->vy = -2.6f;
        o->thrower = (int16_t)(ai + 1);
        if (o->type == OB_DYNAMITE) o->timer = 0;
    } else if (place) {
        float nx = a->face ? a->x + AW + 1 : a->x - o->w - 1;
        float ny = a->y + AH - o->h;
        if (box_hits_tiles(w, nx, ny, o->w, o->h) || box_hits_objs(w, nx, ny, o->w, o->h, (int)(o - w->o)) >= 0) { nx = a->x; ny = a->y + AH - o->h; }
        o->x = nx;
        o->y = ny;
        o->vx = o->vy = 0;
    } else {
        o->x = a->x;
        o->y = a->y + AH - o->h - 2;
        o->vx = 0;
        o->vy = 0;
    }
}

static void stun_actor(DxWorld *w, int i, float kvx, float kvy) {
    DxActor *a = &w->a[i];
    if (!a->alive) return;
    a->stun = DX_STUN;
    a->vx = kvx;
    a->vy = kvy;
    a->climb = 0;
    a->gun = 0;
    a->rolling = 0;
    if (a->hidden) a->hidden = 0;
    if (a->carry >= 0) let_go(w, a, false, false);
    if (a->kind != AK_OUTLAW && a->state == LS_GUARD) a->state = LS_PATROL; /* stunned guards start to patrol */
    w->ev_stun++;
}

/* a punch on a tile: planks take DX_TILE_HP punches and armor
 * DX_ARMOR_HP (flashing as they crack); the iron fist breaks anything,
 * gates included, at once */
static bool punch_tile(DxWorld *w, int tx, int ty, bool fist) {
    int t = dx_tile(w, tx, ty);
    if (fist && (breakable(t, true) || t == TL_GATE)) { clear_tile(w, tx, ty); w->ev_break++; return true; }
    if (!breakable(t, true)) return false;
    w->tflash[ty][tx] = 8;
    if (++w->dmg[ty][tx] >= (t == TL_ARMOR ? DX_ARMOR_HP : DX_TILE_HP)) clear_tile(w, tx, ty);
    w->ev_break++;
    return true;
}

/* a punch: characters, then things, then the wall, floor or ceiling */
static void punch(DxWorld *w, int ai) {
    DxActor *a = &w->a[ai];
    int dir = a->face ? 1 : -1;
    bool low = a->duck, up = !a->ground && !a->climb;
    bool fist = a->fist;
    w->ev_punch++;
    float hx = a->face ? a->x + AW - 2 : a->x - 12, hy = low ? a->y + 16 : a->y + 2;
    float hw = 14, hh = low ? 14 : 26;
    for (int i = 0; i < w->na; i++) {
        DxActor *t = &w->a[i];
        if (i == ai || !t->alive || t->escaped || t->dying) continue;
        if (!overlap(hx, hy, hw, hh, t->x, atop(t), AW, ah(t))) continue;
        if (fist && a->kind == AK_OUTLAW && t->kind != AK_OUTLAW) { kill_actor(w, i, WHY_SHOT, dir); alert_near(w, t->x + 6, t->y + AH); return; }
        if (low) stun_actor(w, i, dir * 3.2f, -3.6f); /* the low punch sends them flying */
        else stun_actor(w, i, dir * 2.6f, -0.5f);
        return;
    }
    int oi = obj_in_front(w, a, low);
    if (oi >= 0) {
        DxObj *o = &w->o[oi];
        switch (o->type) {
        case OB_BARREL: case OB_CRATE: case OB_LOOT: break_obj(w, oi); return;
        case OB_LEVER: flip_gates(w); return;
        case OB_RAM: o->timer = 1; o->face = (uint8_t)(dir > 0); return; /* a poke sends it charging */
        case OB_GUN:
            /* from behind or above it turns on the lawmen */
            if ((o->face && a->x < o->x) || (!o->face && a->x > o->x) || a->y + AH <= o->y + 2) o->friendly = 1;
            if (fist) break_obj(w, oi);
            return;
        case OB_ANVIL: case OB_GOOSE:
            if (fist) break_obj(w, oi);
            else { o->vx = dir * 2.0f; o->vy = -1.5f; }
            return;
        default: break;
        }
    }
    /* the train itself */
    int tx, ty;
    if (low) { tx = fl((a->x + 6 + dir * 4) / DX_T); ty = fl((a->y + AH + 2) / DX_T); }
    else if (up) { tx = fl((a->x + 6) / DX_T); ty = fl((a->y - 2) / DX_T); }
    else { tx = fl((a->x + 6 + dir * 12) / DX_T); ty = fl((a->y + 8) / DX_T); }
    for (int k = 0; k < (low || up ? 1 : 2); k++)
        if (punch_tile(w, tx, ty + k, fist)) return;
}

/* ------------------------------------------------------------------ */
/* moving bodies                                                        */

/* is anything sitting on the top or the foot of the ladder in column tx? */
static bool ladder_blocked(const DxWorld *w, int tx) {
    int top = -1, bot = -1;
    for (int y = 0; y < DX_H; y++)
        if (w->tile[y][tx] == TL_LADDER) { if (top < 0) top = y; bot = y; }
    if (top < 0) return false;
    float x0 = (float)(tx * DX_T), x1 = x0 + DX_T;
    for (int i = 0; i < w->no; i++) {
        const DxObj *o = &w->o[i];
        if (!obj_solid(o)) continue;
        if (o->x + o->w <= x0 + 2 || o->x >= x1 - 2) continue;
        float ob = o->y + o->h;
        if (fabsf(ob - top * DX_T) < 4 || fabsf(ob - (bot + 1) * DX_T) < 4) return true;
    }
    return false;
}

static void move_actor(DxWorld *w, int ai, float dvx) {
    DxActor *a = &w->a[ai];
    /* a body shot off the train: an arc to the rails, through everything */
    if (a->dying) {
        a->x += a->vx;
        a->y += a->vy;
        a->vy += GRAV;
        if (a->y > DX_DEATH_Y + 48) a->dying = 0;
        return;
    }
    /* ladders */
    if (a->climb) {
        a->y += a->vy;
        int tx = fl((a->x + 6) / DX_T);
        int fr = fl((a->y + AH - 1) / DX_T);
        if (a->vy < 0 && box_hits_objs(w, a->x, a->y, AW, 4, a->carry) >= 0) { a->y -= a->vy; a->vy = 0; } /* something sits on top */
        if (a->vy < 0 && dx_tile(w, tx, fr) != TL_LADDER && dx_tile(w, tx, fr + 1) == TL_LADDER) {
            /* over the top: step off onto the roof */
            a->y = (float)((fr + 1) * DX_T - AH);
            a->climb = 0;
            a->ground = 1;
            a->vy = 0;
            return;
        }
        int tt = fl((a->y + 4) / DX_T), tb = fl((a->y + AH - 1) / DX_T);
        bool on = dx_tile(w, tx, tt) == TL_LADDER || dx_tile(w, tx, tb) == TL_LADDER || dx_tile(w, tx, tb + 1) == TL_LADDER;
        if (a->vy > 0 && box_hits_tiles(w, a->x, a->y, AW, AH)) {
            a->y = (float)(fl((a->y + AH) / DX_T) * DX_T - AH);
            a->climb = 0;
            a->ground = 1;
        }
        if (!on) { a->climb = 0; if (a->vy < 0) a->vy = -1.5f; }
        return;
    }
    /* horizontal */
    float h = ah(a), top = atop(a);
    float nx = a->x + a->vx + dvx;
    if (box_hits_tiles(w, nx, top, AW, h)) {
        nx = a->vx + dvx > 0 ? (float)(fl((nx + AW) / DX_T) * DX_T - AW) - 0.01f : (float)((fl(nx / DX_T) + 1) * DX_T) + 0.01f;
        if (box_hits_tiles(w, nx, top, AW, h)) nx = a->x;
        a->vx = 0;
        a->push_t = 0;
    }
    int ob = box_hits_objs(w, nx, top, AW, h - 1, a->carry);
    if (ob >= 0) {
        /* walk into a thing for a moment and it starts to slide ahead of you */
        DxObj *o = &w->o[ob];
        float d = nx - a->x;
        bool pushed = false;
        if (a->kind == AK_OUTLAW && a->ground && dvx != 0 && pushable(o) && o->timer != -5 && ++a->push_t > PUSH_DELAY) {
            float step = d > 0 ? PUSH_SPEED : -PUSH_SPEED;
            float ox = o->x + step;
            if (!box_hits_tiles(w, ox, o->y, o->w, o->h) && box_hits_objs(w, ox, o->y, o->w, o->h, ob) < 0) {
                o->x = ox;
                nx = a->x + step;
                pushed = box_hits_objs(w, nx, top, AW, h - 1, a->carry) < 0;
            }
        }
        if (!pushed) { nx = a->x; a->vx = 0; }
    } else {
        a->push_t = 0;
    }
    a->x = nx;
    /* vertical */
    float before = a->y + AH;
    a->y += a->vy;
    a->ground = 0;
    top = atop(a);
    if (a->vy >= 0) {
        bool land = box_hits_tiles(w, a->x, top, AW, h) || shelf_under(w, a->x, before, a->y + AH);
        int lo = box_hits_objs(w, a->x, top, AW, h, a->carry);
        if (lo >= 0 && before <= w->o[lo].y + 1) land = true;
        else if (lo >= 0) lo = -1;
        int hd = land ? -1 : head_under(w, ai, a->x, before, a->y + AH);
        if (hd >= 0) {
            a->y = atop(&w->a[hd]) - AH;
            a->vy = 0;
            a->ground = 1;
            a->jumps = 0;
        } else if (land) {
            if (lo >= 0) a->y = w->o[lo].y - AH;
            else a->y = (float)(fl((a->y + AH) / DX_T) * DX_T - AH);
            a->vy = 0;
            a->ground = 1;
            a->jumps = 0;
        } else {
            /* resting exactly on something: still on the ground */
            float feet = a->y + AH;
            if (box_hits_tiles(w, a->x, top + 1, AW, h) || shelf_under(w, a->x, feet, feet + 1)) { a->ground = 1; a->vy = 0; a->jumps = 0; }
            else {
                int ro = box_hits_objs(w, a->x, top + 1, AW, h, a->carry);
                if (ro >= 0 && feet <= w->o[ro].y + 1) { a->ground = 1; a->vy = 0; a->jumps = 0; }
                else if (head_under(w, ai, a->x, feet, feet + 1) >= 0) { a->ground = 1; a->vy = 0; a->jumps = 0; }
            }
        }
    } else if (box_hits_tiles(w, a->x, top, AW, h) || box_hits_objs(w, a->x, top, AW, h, a->carry) >= 0) {
        a->y = (float)((fl(top / DX_T) + 1) * DX_T) - (AH - h);
        a->vy = 0;
    }
    if (!a->ground) {
        bool glide = a->carry >= 0 && w->o[a->carry].type == OB_GOOSE;
        a->vy += GRAV;
        if (a->vy > (glide ? GLIDE : MAXFALL)) a->vy = glide ? GLIDE : MAXFALL;
    }
    /* friction on knockbacks */
    if (a->ground) a->vx *= 0.7f;
    else a->vx *= 0.98f;
    if (fabsf(a->vx) < 0.05f) a->vx = 0;
    /* off the train */
    if (a->y + AH > DX_DEATH_Y + 8) kill_actor(w, ai, WHY_FELL, 0);
    /* the carried thing rides overhead */
    if (a->carry >= 0) {
        DxObj *o = &w->o[a->carry];
        o->x = a->x + 6 - o->w / 2.0f;
        o->y = atop(a) - o->h;
        o->vx = o->vy = 0;
    }
}

static bool at_ladder(const DxWorld *w, const DxActor *a, int dy) {
    int tx = fl((a->x + 6) / DX_T);
    int ty = dy < 0 ? fl((a->y + AH - 4) / DX_T) : fl((a->y + AH + 2) / DX_T);
    return dx_tile(w, tx, ty) == TL_LADDER;
}

/* ------------------------------------------------------------------ */
/* the outlaw at the controls                                           */

/* the barrel an outlaw hides in */
static int hide_barrel(const DxWorld *w, const DxActor *a) {
    for (int i = 0; i < w->no; i++) {
        const DxObj *o = &w->o[i];
        if (o->alive && o->type == OB_BARREL && o->timer == -5 && fabsf(o->x - (a->x - 1)) < 8 && fabsf(o->y + o->h - (a->y + AH)) < 20) return i;
    }
    return -1;
}

static void control_outlaw(DxWorld *w, int ai, const DxInput *in) {
    DxActor *a = &w->a[ai];
    if (!a->alive || a->escaped) return;
    if (a->stun > 0) { move_actor(w, ai, 0); return; }
    if (a->punch_t) a->punch_t--;
    /* inside a barrel: left and right roll it, up or A climbs out on top,
     * B climbs out holding it */
    if (a->hidden) {
        int bi = hide_barrel(w, a);
        if (bi < 0) { a->hidden = 0; return; }
        DxObj *o = &w->o[bi];
        o->roll = (int8_t)(in->right - in->left);
        if (in->up || in->a_pressed) {
            a->hidden = 0;
            a->duck = 0;
            o->timer = 0;
            o->roll = 0;
            a->y = o->y - AH;
            a->vy = 0;
        } else if (in->b_pressed) {
            a->hidden = 0;
            a->duck = 0;
            o->timer = 0;
            o->roll = 0;
            pick_up(w, ai, bi);
        }
        return;
    }
    bool heavy = a->carry >= 0 && w->o[a->carry].type == OB_ANVIL;
    /* ladders */
    if (a->climb) {
        a->vy = in->up ? -CLIMB : in->down ? CLIMB : 0;
        if (in->a_pressed) { a->climb = 0; a->vy = JUMP_V * 0.7f; a->jump_hold = 1; }
        if ((in->left || in->right) && !in->up && !in->down) {
            if (in->left) a->face = 0;
            if (in->right) a->face = 1;
            a->climb = 0;
        }
        move_actor(w, ai, 0);
        return;
    }
    if (a->carry < 0 && !a->duck && ((in->up && at_ladder(w, a, -1)) || (in->down && at_ladder(w, a, 1) && a->ground))) {
        a->climb = 1;
        a->gun = 0;
        a->x = (float)(fl((a->x + 6) / DX_T) * DX_T + 2);
        a->vx = 0;
        if (in->down) a->y += 2;
        move_actor(w, ai, 0);
        return;
    }
    /* holding a barrel, duck into it */
    if (a->carry >= 0 && w->o[a->carry].type == OB_BARREL && in->down_pressed && a->ground) {
        DxObj *o = &w->o[a->carry];
        let_go(w, a, false, false);
        o->x = a->x - 1;
        o->y = a->y + AH - o->h;
        o->timer = -5;
        o->roll = 0;
        o->suspect = 0;
        a->hidden = 1;
        a->duck = 1;
        a->rolling = 0;
        return;
    }
    /* the gun: hold up, standing still, to load and draw */
    if (a->gun == -1) {
        if (in->left || in->right || in->down || in->a_pressed) a->gun = 0;
        else if (in->b_pressed) {
            int dir = a->face ? 1 : -1;
            if (a->ammo > 0) {
                a->ammo--;
                shoot(w, ai, a->x + 6 + dir * 8, a->y + 9, dir, a->blast);
            }
            a->gun = 0;
            move_actor(w, ai, 0);
            return;
        }
    } else if (in->up && a->ground && a->carry < 0 && a->ammo > 0 && !in->left && !in->right && !in->down) {
        a->gun++;
        if (a->gun >= (a->quick ? DX_QUICKDRAW : DX_DRAW)) a->gun = -1;
    } else if (a->gun > 0) {
        a->gun = 0;
    }
    /* ducking, and the roll: down + A rolls on for as long as down is held
     * (or until there's room to stand), as fast as walking */
    bool room = !box_hits_tiles(w, a->x, a->y, AW, AH) && box_hits_objs(w, a->x, a->y, AW, AH - DUCK_H, a->carry) < 0;
    if (a->ground && in->down && in->a_pressed && a->gun <= 0) { a->rolling = 1; a->duck = 1; }
    if (a->rolling) {
        if (in->left) a->face = 0;
        if (in->right) a->face = 1;
        if (!in->down && room && a->ground) a->rolling = 0;
    }
    a->duck = a->rolling || (in->down && a->ground) || (a->duck && !room);
    float move = 0;
    if (a->rolling) move = a->face ? WALK : -WALK;
    else if (a->gun != -1 && a->gun <= 0) {
        if (in->left) a->face = 0;
        if (in->right) a->face = 1;
        if (!a->duck) move = (in->right - in->left) * WALK;
    }
    /* jump: hold A for the full height, let go early for a hop */
    if (in->a_pressed && a->gun <= 0 && !in->down) {
        if (a->ground) { a->vy = heavy ? HEAVY_JUMP_V : JUMP_V; a->ground = 0; a->duck = 0; a->jump_hold = 1; }
        else if (a->boots && !a->jumps && !heavy) { a->vy = BOOTS_V; a->jumps = 1; a->jump_hold = 1; }
    }
    if (a->jump_hold && (!in->a || a->vy >= 0)) {
        if (!in->a && a->vy < 0) a->vy *= JUMP_CUT;
        a->jump_hold = 0;
    }
    if (in->b_pressed && a->gun != -1) {
        if (a->carry >= 0) {
            let_go(w, a, !in->down, in->down);
        } else if (a->duck) {
            int oi = obj_in_front(w, a, true);
            if (oi < 0) {
                /* standing on it counts too */
                for (int i = 0; i < w->no; i++) {
                    DxObj *o = &w->o[i];
                    if (liftable(o) && o->timer != -5 && overlap(a->x - 4, a->y + AH - 4, AW + 8, 8, o->x, o->y, o->w, o->h)) { oi = i; break; }
                }
            }
            if (oi >= 0 && liftable(&w->o[oi])) pick_up(w, ai, oi);
            else { a->punch_t = 12; punch(w, ai); }
        } else {
            a->punch_t = 12;
            punch(w, ai);
        }
    }
    a->vx = a->vx * 0.5f;
    float before = a->x;
    move_actor(w, ai, move);
    if (a->rolling && move != 0 && fabsf(a->x - before) < 0.01f && a->ground && room) a->rolling = 0; /* rolled into a wall */
    if (move != 0 || !a->ground) a->anim++;
}

/* ------------------------------------------------------------------ */
/* lawmen                                                               */

/* somewhere to put a foot ahead: level, or a step of up to a tile down
 * (off a box); not the edge of a car or a hole */
static bool ground_ahead(const DxWorld *w, const DxActor *a, int dir) {
    for (int drop = 0; drop <= DX_T; drop += DX_T) {
        int tx = fl((a->x + 6 + dir * 10) / DX_T), ty = fl((a->y + AH + 2 + drop) / DX_T);
        int t = dx_tile(w, tx, ty);
        if (dx_solid(w, tx, ty) || t == TL_SHELF || t == TL_LADDER) return true;
        if (box_hits_objs(w, a->x + dir * 10, a->y + AH + 1 + drop, AW, 2, -1) >= 0) return true;
    }
    return false;
}

/* (the bottom pixel is left out: whatever he stands on isn't in his way) */
static bool wall_ahead(const DxWorld *w, const DxActor *a, int dir) {
    return box_hits_tiles(w, a->x + dir * 2, a->y, AW, AH - 1) || box_hits_objs(w, a->x + dir * 2, a->y, AW, AH - 1, a->carry) >= 0;
}

/* only something a tile high in the way, with room to hop over it */
static bool low_obstacle(const DxWorld *w, const DxActor *a, int dir) {
    if (box_hits_tiles(w, a->x + dir * 2, a->y, AW, AH - 1)) return false;
    int oi = box_hits_objs(w, a->x + dir * 2, a->y, AW, AH - 1, a->carry);
    if (oi < 0) return false;
    const DxObj *o = &w->o[oi];
    if (o->y + o->h < a->y + AH - 2 || o->y < a->y + AH - 18) return false;
    float over = o->x + (dir > 0 ? o->w + 2 : -AW - 2);
    return !box_hits_tiles(w, dir > 0 ? o->x : over, a->y - 18, AW + o->w, 30);
}

static int visible_outlaw(const DxWorld *w, int li) {
    for (int i = 0; i < w->na; i++)
        if (w->a[i].kind == AK_OUTLAW && w->a[i].alive && !w->a[i].escaped && dx_sees(w, li, i)) return i;
    return -1;
}

/* walk toward x on this floor; returns true once there */
static bool walk_to(DxWorld *w, DxActor *l, float x, float *move) {
    float dx = x - l->x;
    if (fabsf(dx) < 2) return true;
    int dir = dx > 0 ? 1 : -1;
    l->face = dir > 0;
    if (!l->ground) {}
    else if (low_obstacle(w, l, dir)) { l->vy = LAW_HOP_V; l->ground = 0; }
    else if (wall_ahead(w, l, dir) || !ground_ahead(w, l, dir)) return true; /* as far as he can get */
    *move = fabsf(dx) < LAW_WALK ? dx : dir * LAW_WALK;
    return false;
}

static void lawman_think(DxWorld *w, int li) {
    DxActor *l = &w->a[li];
    if (!l->alive) return;
    if (l->dying) { move_actor(w, li, 0); return; }
    if (l->stun > 0) { l->stun--; move_actor(w, li, 0); return; }
    if (l->punch_t) l->punch_t--;
    if (l->throw_cd) l->throw_cd--;
    int dir = l->face ? 1 : -1;
    float move = 0;
    if (l->climb) {
        float want = l->has_tgt ? l->tgt_y : l->y + AH;
        l->vy = want < l->y + AH - 4 ? -CLIMB : CLIMB;
        move_actor(w, li, 0);
        return;
    }
    if (l->state == LS_PATROL) {
        /* a fixed beat: back and forth, hopping over anything a tile high */
        if (!l->ground) {}
        else if (low_obstacle(w, l, dir)) { l->vy = LAW_HOP_V; l->ground = 0; }
        else if (wall_ahead(w, l, dir) || !ground_ahead(w, l, dir)) { l->face ^= 1; dir = -dir; }
        move = dir * LAW_WALK;
    } else if (l->state == LS_ALERT) {
        /* seeking: the outlaw he can see, else the last noise or sighting,
         * else a search back and forth */
        int seen = visible_outlaw(w, li);
        if (seen >= 0) set_target(l, w->a[seen].x, w->a[seen].y + AH);
        if (seen >= 0 && !l->armed) {
            const DxActor *o = &w->a[seen];
            float gap = fabsf(o->x - l->x);
            /* hands full: throw it at them */
            if (l->carry >= 0 && gap < 8 * DX_T && l->throw_cd == 0) { let_go(w, l, true, false); l->throw_cd = 60; }
            else if (gap > 13) walk_to(w, l, o->x, &move);
        } else if (l->has_tgt) {
            float dy = l->tgt_y - (l->y + AH);
            if (fabsf(dy) < 10) {
                int tdir = l->tgt_x > l->x ? 1 : -1;
                if (fabsf(l->tgt_x - l->x) > 2 && wall_ahead(w, l, tdir) && !low_obstacle(w, l, tdir)) {
                    /* break through whatever is in the way */
                    l->face = tdir > 0;
                    int oi = obj_in_front(w, l, false);
                    if (!l->armed && l->carry < 0 && oi >= 0 && liftable(&w->o[oi]) && w->o[oi].type != OB_LOOT) pick_up(w, li, oi);
                    else if (l->punch_t == 0) { l->punch_t = 20; punch(w, li); }
                } else if (walk_to(w, l, l->tgt_x, &move)) {
                    /* here, and nobody: look both ways, then search */
                    if (++l->look_t == 30 || l->look_t == 60) l->face ^= 1;
                    if (l->look_t > 75) l->has_tgt = 0;
                }
            } else {
                /* another floor: the nearest ladder that isn't blocked */
                int ltx = fl((l->x + 6) / DX_T), fy = fl((l->y + AH - 4) / DX_T), best = -1, bd = 999;
                for (int tx = ltx - 12; tx <= ltx + 12; tx++) {
                    bool up = dy < 0 && (dx_tile(w, tx, fy) == TL_LADDER || dx_tile(w, tx, fy - 1) == TL_LADDER);
                    bool down = dy > 0 && dx_tile(w, tx, fy + 1) == TL_LADDER;
                    if ((up || down) && !ladder_blocked(w, tx) && iabs_c(tx - ltx) < bd) { bd = iabs_c(tx - ltx); best = tx; }
                }
                if (best >= 0) {
                    float lx = (float)(best * DX_T + 2);
                    if (fabsf(lx - l->x) < 1.5f) {
                        l->x = lx;
                        l->climb = 1;
                        l->vy = dy < 0 ? -CLIMB : CLIMB;
                        if (dy > 0) l->y += 2;
                    } else {
                        walk_to(w, l, lx, &move);
                    }
                } else {
                    l->has_tgt = 0; /* no way there: search here */
                }
            }
        } else {
            if (!l->ground) {}
            else if (low_obstacle(w, l, dir)) { l->vy = LAW_HOP_V; l->ground = 0; }
            else if (wall_ahead(w, l, dir) || !ground_ahead(w, l, dir)) { l->face ^= 1; dir = -dir; }
            move = dir * LAW_WALK;
        }
    }
    l->vx *= 0.5f;
    move_actor(w, li, move);
    if (move != 0) l->anim++;
}

/* what a lawman sees ahead, and what he does about it */
static void lawmen_look(DxWorld *w) {
    for (int i = 0; i < w->na; i++) {
        DxActor *l = &w->a[i];
        if (l->kind == AK_OUTLAW || !l->alive || l->stun || l->dying) continue;
        int dir = l->face ? 1 : -1;
        float ex = l->x + 6 + dir * 6;
        for (int k = 0; k < 2; k++) {
            float ry = l->y + (k ? 24 : 6);
            int hit = ray(w, ex, ry, dir, i, true, false, SIGHT);
            if (hit == 0) continue;
            bool shoot_it = false, alert = false;
            float tx = 0, tfeet = l->y + AH;
            if (hit > 0) {
                /* an outlaw in plain sight */
                const DxActor *o = &w->a[hit - 1];
                shoot_it = true;
                alert = true;
                tx = o->x;
                tfeet = o->y + AH;
            } else {
                const DxObj *o = &w->o[-hit - 1];
                tx = o->x;
                if (o->held >= 0 && w->a[o->held].kind == AK_OUTLAW) shoot_it = alert = true;   /* a box held up in his sight */
                else if (o->type == OB_GOOSE && o->thrower) shoot_it = alert = true;          /* a goose on the wing */
                else if (o->type == OB_BARREL && o->timer == -5) {
                    /* a barrel with someone in it: falling, or one he saw roll, gets shot */
                    if (o->vy > 0.5f || (o->suspect && l->state == LS_ALERT && w->phase == PH_LAW)) shoot_it = alert = true;
                    else if (o->roll) { w->o[-hit - 1].suspect = 1; alert = true; }
                } else if (o->thrower) alert = true;                                        /* something thrown past him */
            }
            if (shoot_it && l->armed) {
                shoot(w, i, ex, ry, dir, false);
                l->armed = 0;
            }
            if (alert) {
                if (l->state != LS_ALERT) w->ev_alert++;
                l->state = LS_ALERT;
                set_target(l, tx, tfeet);
            }
            if (shoot_it || alert) break;
        }
    }
}

/* two unarmed characters bumping into each other: the mover punches */
static void auto_punch(DxWorld *w, bool outlaws_moving) {
    for (int i = 0; i < w->na; i++) {
        DxActor *o = &w->a[i];
        if (o->kind != AK_OUTLAW || !o->alive || o->escaped || o->hidden || o->stun) continue;
        for (int j = 0; j < w->na; j++) {
            DxActor *l = &w->a[j];
            if (l->kind == AK_OUTLAW || !l->alive || l->armed || l->stun || l->dying) continue;
            if (!overlap(o->x - 2, atop(o), AW + 4, ah(o), l->x, l->y, AW, AH)) continue;
            int dir = l->x > o->x ? 1 : -1;
            if (outlaws_moving) { o->face = dir > 0; o->punch_t = 12; stun_actor(w, j, dir * 2.6f, -0.5f); w->ev_punch++; }
            else { l->face = dir < 0; l->punch_t = 12; stun_actor(w, i, -dir * 2.6f, -0.5f); w->ev_punch++; }
        }
    }
}

/* ------------------------------------------------------------------ */
/* things in motion                                                     */

static void obj_hits_actor(DxWorld *w, int oi, int ai) {
    DxObj *o = &w->o[oi];
    int dir = o->vx > 0 ? 1 : -1;
    if (o->type == OB_ANVIL) kill_actor(w, ai, WHY_CRUSHED, dir);
    else if (o->type == OB_DYNAMITE) { o->alive = 0; explode(w, o->x + 7, o->y + 7, 24, 28); }
    else stun_actor(w, ai, dir * 2.0f, -1.0f);
    o->vx = -o->vx * 0.3f;
    o->thrower = 0;
    /* barrels, crates and strongboxes burst on whoever they hit */
    if (o->type == OB_BARREL || o->type == OB_CRATE || o->type == OB_LOOT) break_obj(w, oi);
}

/* the ram: it plods back and forth; poked, shot at or hearing a shot it
 * charges until it hits a wall. Either way it runs down whoever is in front */
static void step_ram(DxWorld *w, int i) {
    DxObj *o = &w->o[i];
    int dir = o->face ? 1 : -1;
    float sp = o->timer > 0 ? RAM_RUN : RAM_WALK;
    float nx = o->x + dir * sp;
    bool edge = !box_hits_tiles(w, nx + (dir > 0 ? o->w - 4 : 0), o->y + o->h + 1, 4, 1) &&
                box_hits_objs(w, nx + (dir > 0 ? o->w - 4 : 0), o->y + o->h + 1, 4, 1, i) < 0;
    int blocked = box_hits_objs(w, nx, o->y, o->w, o->h, i);
    if (box_hits_tiles(w, nx, o->y, o->w, o->h) || (o->timer == 0 && (edge || blocked >= 0))) {
        o->timer = 0;
        o->face ^= 1;
        return;
    }
    o->x = nx;
    if (edge) {
        /* charging over the edge */
        o->y += 4;
        if (o->y > DX_DEATH_Y) { o->alive = 0; return; }
    }
    for (int a = 0; a < w->na; a++) {
        DxActor *t = &w->a[a];
        if (!t->alive || t->escaped || t->dying || t->hidden) continue;
        float front = dir > 0 ? o->x + o->w - 6 : o->x;
        if (overlap(front, o->y, 6, o->h, t->x, atop(t), AW, ah(t))) {
            kill_actor(w, a, WHY_TRAMPLED, dir);
            if (t->kind != AK_OUTLAW) alert_near(w, t->x + 6, t->y + AH);
        }
    }
    if (o->timer > 0)
        for (int b = 0; b < w->no; b++)
            if (b != i && obj_solid(&w->o[b]) && w->o[b].type != OB_RAM && overlap(o->x, o->y, o->w, o->h, w->o[b].x, w->o[b].y, w->o[b].w, w->o[b].h)) break_obj(w, b);
}

static void step_objects(DxWorld *w) {
    for (int i = 0; i < w->no; i++) {
        DxObj *o = &w->o[i];
        if (!o->alive || o->held >= 0) continue;
        if (o->type == OB_CAMEL) continue;
        if (o->type == OB_RAM) { step_ram(w, i); continue; }
        /* a barrel with someone inside: rolled by the one inside */
        bool occupied = o->type == OB_BARREL && o->timer == -5;
        if (occupied) o->vx = o->roll * WALK * 0.8f;
        /* dynamite that was thrown and has stopped goes off */
        if (o->type == OB_DYNAMITE && o->timer > 0 && --o->timer == 0) { o->alive = 0; explode(w, o->x + 7, o->y + 7, 24, 28); continue; }
        float before = o->y + o->h;
        o->vy += 0.25f;
        if (o->vy > MAXFALL) o->vy = MAXFALL;
        /* horizontal */
        float nx = o->x + o->vx;
        if (box_hits_tiles(w, nx, o->y, o->w, o->h) && o->thrower) {
            /* a throw that just clips the top of a doorway ducks under it */
            float ny = (float)((fl(o->y / DX_T) + 1) * DX_T);
            if (ny - o->y <= 10 && !box_hits_tiles(w, nx, ny, o->w, o->h)) { o->y = ny; if (o->vy < 0) o->vy = 0; }
        }
        if (box_hits_tiles(w, nx, o->y, o->w, o->h)) {
            if (o->type == OB_DYNAMITE && o->thrower) { o->alive = 0; explode(w, o->x + 7, o->y + 7, 24, 28); continue; }
            o->vx = -o->vx * 0.2f;
        } else if (box_hits_objs(w, nx, o->y, o->w, o->h, i) >= 0 && (o->thrower || occupied)) {
            o->vx = -o->vx * 0.2f;
        } else {
            o->x = nx;
        }
        /* vertical */
        o->y += o->vy;
        if (o->vy < 0 && box_hits_tiles(w, o->x, o->y, o->w, o->h)) {
            /* a ceiling: bump and fall back */
            o->y = (float)((fl(o->y / DX_T) + 1) * DX_T);
            o->vy = 0;
        }
        /* thrown things hit people (before a landing ends the throw) */
        if (o->thrower) {
            bool hit = false;
            for (int a = 0; a < w->na && !hit; a++) {
                DxActor *t = &w->a[a];
                if (a == o->thrower - 1 || !t->alive || t->escaped || t->hidden || t->dying) continue;
                if (overlap(o->x, o->y, o->w, o->h, t->x, atop(t), AW, ah(t))) { obj_hits_actor(w, i, a); hit = true; }
            }
            if (!o->alive) continue;
        }
        bool land = o->vy >= 0 && (box_hits_tiles(w, o->x, o->y, o->w, o->h) || shelf_under(w, o->x, before, o->y + o->h));
        int under = box_hits_objs(w, o->x, o->y, o->w, o->h, i);
        if (under >= 0 && before <= w->o[under].y + 1 && o->type != OB_COIN && o->type != OB_POWER && o->type != OB_AMMO) {
            o->y = w->o[under].y - o->h;
            land = false;
            o->vy = 0;
            if (!occupied) o->vx *= 0.7f;
        }
        if (land) {
            o->y = (float)(fl((o->y + o->h) / DX_T) * DX_T - o->h);
            if (o->vy > 0) o->vy = 0;
            if (!occupied) o->vx *= 0.75f;
            if (fabsf(o->vx) < 0.05f) o->vx = 0;
            if (o->thrower) {
                if (o->type == OB_LOOT) { break_obj(w, i); continue; }
                if (o->type == OB_DYNAMITE) { o->timer = o->vx == 0 ? 20 : 0; if (o->vx == 0) o->thrower = 0; }
                else o->thrower = 0;
            }
        }
        if (o->y > DX_DEATH_Y + 16) { o->alive = 0; continue; } /* lost off the train */
        /* the crank gun: a warning flash (green at outlaws, red once it's
         * ours), then a burst of three, whatever is in the way; two bursts */
        if (o->type == OB_GUN && o->ammo > 0) {
            int dir = o->face ? 1 : -1;
            float mx = o->x + 7 + dir * 9, my = o->y + 6;
            if (o->burst > 0) {
                if (--o->timer <= 0) {
                    shoot(w, -2 - i, mx, my, dir, false);
                    o->ammo--;
                    o->burst--;
                    o->timer = o->burst ? 8 : -40; /* a breather after each burst */
                }
            } else if (o->timer < 0) {
                o->timer++;
            } else {
                int hit = ray_ex(w, mx, my, dir, -1, i, !o->friendly, o->friendly != 0, 14, NULL);
                bool target = hit > 0;
                if (target && o->timer == 0) o->timer = 24;
                if (o->timer > 0 && --o->timer == 0) { o->burst = 3; o->timer = 1; }
            }
        }
    }
}

static void step_shots(DxWorld *w) {
    for (int i = 0; i < DX_MAX_SHOTS; i++) {
        DxShot *s = &w->s[i];
        if (!s->alive) continue;
        s->x += s->vx;
        s->range -= fabsf(s->vx);
        if (s->range <= 0) s->alive = 0;
    }
}

/* the hidden outlaw goes where the barrel goes */
static void ride_barrels(DxWorld *w) {
    for (int i = 0; i < w->na; i++) {
        DxActor *a = &w->a[i];
        if (!a->hidden || !a->alive) continue;
        int bi = hide_barrel(w, a);
        if (bi < 0) { a->hidden = 0; continue; }
        const DxObj *o = &w->o[bi];
        a->x = o->x + 1;
        a->y = o->y + o->h - AH;
        if (o->y > DX_DEATH_Y) kill_actor(w, i, WHY_FELL, 0);
    }
}

/* ------------------------------------------------------------------ */
/* pickups, the camel, the objective                                    */

static bool objective_done(const DxWorld *w) {
    switch (w->objective) {
    case OBJ_LOOT: return w->loot >= w->need;
    case OBJ_RESCUE: return w->rescue_open;
    case OBJ_AMMO: return w->a[w->ctrl].ammo >= DX_MAX_AMMO;
    case OBJ_GOVERNOR: return w->governor_dead;
    default: return true;
    }
}

static void pickups(DxWorld *w) {
    for (int ai = 0; ai < w->na; ai++) {
        DxActor *a = &w->a[ai];
        if (a->kind != AK_OUTLAW || !a->alive || a->escaped) continue;
        for (int i = 0; i < w->no; i++) {
            DxObj *o = &w->o[i];
            if (!o->alive || o->held >= 0 || !overlap(a->x, atop(a), AW, ah(a), o->x, o->y, o->w, o->h)) continue;
            if (o->type == OB_COIN) {
                o->alive = 0;
                w->ev_coin++;
                if (w->versus) { w->coins++; continue; }
                int id = o->loot_id % 8;
                if (++w->loot_coins[id] >= 4 && !w->loot_done[id]) { w->loot_done[id] = 1; w->loot++; }
            } else if (o->type == OB_AMMO) {
                if (a->ammo < DX_MAX_AMMO) { a->ammo++; o->alive = 0; w->ev_coin++; }
            } else if (o->type == OB_POWER) {
                o->alive = 0;
                w->ev_coin++;
                switch (o->content) {
                case PW_ROUNDS: a->ammo = (uint8_t)imin_c(DX_MAX_AMMO, a->ammo + (w->mission == 0 ? 1 : 2)); break;
                case PW_BLAST: a->blast = 1; break;
                case PW_BOOTS: a->boots = 1; break;
                case PW_CHARM: a->charm = 1; break;
                case PW_FIST: a->fist = 1; break;
                case PW_QUICK: a->quick = 1; break;
                default: break;
                }
            } else if (o->type == OB_CAMEL) {
                /* a strongbox carried all the way counts */
                if (a->carry >= 0 && w->o[a->carry].type == OB_LOOT) {
                    DxObj *box = &w->o[a->carry];
                    box->alive = 0;
                    a->carry = -1;
                    if (!w->loot_done[box->loot_id % 8]) { w->loot_done[box->loot_id % 8] = 1; w->loot++; }
                    w->ev_coin++;
                }
                if (objective_done(w)) {
                    a->escaped = 1;
                    if (a->carry >= 0) let_go(w, a, false, false);
                }
            }
        }
    }
}

void dx_swap_outlaw(DxWorld *w) {
    for (int k = 1; k <= w->na; k++) {
        int i = (w->ctrl + k) % w->na;
        const DxActor *a = &w->a[i];
        if (a->kind == AK_OUTLAW && a->alive && !a->escaped) { w->ctrl = i; return; }
    }
}

int dx_stars(const DxWorld *w) {
    int s = 0;
    if (w->kills == 0) s |= 1;
    bool all = true;
    for (int i = 0; i < w->na; i++)
        if (w->a[i].kind != AK_OUTLAW && w->a[i].alive) all = false;
    if (all && w->lawmen_total > 0) s |= 2;
    const DxMission *m = &DX_MISSIONS_DEF[w->mission % DX_MISSIONS];
    if (w->elapsed <= m->star_seconds * 60) s |= 4;
    return s;
}

/* ------------------------------------------------------------------ */
/* one frame                                                            */

static void step_bodies(DxWorld *w) {
    for (int i = 0; i < w->na; i++)
        if (w->a[i].dying) move_actor(w, i, 0);
}

void dx_step(DxWorld *w, const DxInput *in, const DxInput *in2) {
    for (int y = 0; y < DX_H; y++)
        for (int x = 0; x < w->w; x++)
            if (w->tflash[y][x]) w->tflash[y][x]--;
    if (w->result) {
        /* the heist is over: let the last fall play out */
        step_bodies(w);
        step_shots(w);
        if (w->end_t > 0) w->end_t--;
        return;
    }
    int active = dx_active_lawmen(w);
    /* time and turns */
    if (w->phase == PH_FREE && active > 0) {
        w->phase = PH_PLAYER;
        w->turn_t = DX_TURN_FRAMES;
        w->ev_turn++;
    } else if (w->phase != PH_FREE && active == 0 && !w->versus) {
        w->phase = PH_FREE;
        w->ev_turn++;
    }
    if (w->phase != PH_LAW) {
        w->master--;
        w->elapsed++;
        if (w->master <= 0 && !w->versus) { w->result = DX_LOST; w->why = WHY_TIME; w->end_t = DX_END_FRAMES; return; }
    }
    if (w->phase != PH_FREE && --w->turn_t <= 0) {
        if (w->phase == PH_PLAYER) {
            w->phase = PH_LAW;
            w->turn_t = w->versus ? DX_TURN_FRAMES : DX_LAW_FRAMES;
            if (w->versus) {
                /* the guards' player drives the next guard standing */
                for (int k = 1; k <= w->na; k++) {
                    int i = (w->law_ctrl + k) % w->na;
                    if (w->a[i].kind != AK_OUTLAW && w->a[i].alive) { w->law_ctrl = (uint8_t)i; break; }
                }
            }
            /* the outlaws let go: a barrel stops rolling; one in mid-jump
             * keeps going the way he was heading */
            for (int i = 0; i < w->no; i++) w->o[i].roll = 0;
            DxActor *c = &w->a[w->ctrl];
            if (!c->ground && !c->climb && !c->hidden) c->vx = (float)(in->right - in->left) * WALK;
        } else {
            w->phase = PH_PLAYER;
            w->turn_t = DX_TURN_FRAMES;
            if (w->turns < 255) w->turns++;
            dx_swap_outlaw(w); /* two outlaws: they take turns */
        }
        w->ev_turn++;
    }
    DxActor *me = &w->a[w->ctrl];
    if (w->phase == PH_FREE && in->select_pressed) dx_swap_outlaw(w);
    if (w->phase != PH_LAW) {
        /* the outlaws move in real time and on their turn */
        for (int i = 0; i < w->na; i++) {
            DxActor *a = &w->a[i];
            if (a->kind != AK_OUTLAW || !a->alive || a->escaped) continue;
            if (a->stun > 0) a->stun--;
            if (i == w->ctrl) control_outlaw(w, i, in);
            else if (!a->hidden) move_actor(w, i, 0);
        }
        auto_punch(w, true);
    } else {
        /* the lawmen's turn: the outlaws are locked (a jump or a fall still
         * finishes); in 2P the second player drives one of the lawmen */
        for (int i = 0; i < w->na; i++) {
            DxActor *a = &w->a[i];
            if (a->kind == AK_OUTLAW && a->alive && !a->escaped && !a->hidden && !a->climb) {
                a->rolling = 0;
                move_actor(w, i, 0);
            }
        }
        for (int i = 0; i < w->na; i++) {
            DxActor *a = &w->a[i];
            if (a->kind == AK_OUTLAW || !a->alive) continue;
            if (w->versus && i == w->law_ctrl && in2) {
                if (a->stun > 0) { a->stun--; move_actor(w, i, 0); continue; }
                DxInput li = *in2;
                bool was_armed = a->armed;
                a->kind = AK_OUTLAW; /* borrow the outlaw's hands for a moment */
                control_outlaw(w, i, &li);
                a->kind = AK_GUARD;
                a->armed = was_armed;
                a->gun = 0;
            } else if (!w->versus && a->state != LS_GUARD) {
                lawman_think(w, i);
            } else if (a->stun > 0) {
                a->stun--;
                move_actor(w, i, 0);
            }
        }
        auto_punch(w, false);
        /* a drawn gun fires by itself at a lawman who steps into its sight,
         * before he can fire himself */
        for (int i = 0; i < w->na; i++) {
            DxActor *a = &w->a[i];
            if (a->kind != AK_OUTLAW || !a->alive || a->gun != -1 || a->ammo == 0) continue;
            int dir = a->face ? 1 : -1;
            int hit = ray(w, a->x + 6 + dir * 8, a->y + 9, dir, i, false, true, 20);
            if (hit > 0) {
                a->ammo--;
                a->gun = 0;
                shoot(w, i, a->x + 6 + dir * 8, a->y + 9, dir, a->blast);
            }
        }
    }
    /* in real time, stunned lawmen wake up too */
    if (w->phase == PH_FREE)
        for (int i = 0; i < w->na; i++) {
            DxActor *a = &w->a[i];
            if (a->kind != AK_OUTLAW && a->alive && a->stun > 0) { a->stun--; move_actor(w, i, 0); }
        }
    step_bodies(w);
    lawmen_look(w);
    step_objects(w);
    ride_barrels(w);
    step_shots(w);
    pickups(w);
    /* the fog lifts from every car an outlaw walks into */
    for (int i = 0; i < w->na; i++) {
        DxActor *a = &w->a[i];
        if (a->kind != AK_OUTLAW || !a->alive) continue;
        int c = dx_car_at(w, a->x + 6);
        if (c >= 0 && a->y + AH > 4 * DX_T) w->seen[c] = 1;
    }
    /* how it ends */
    bool any_left = false, all_out = true;
    for (int i = 0; i < w->na; i++) {
        DxActor *a = &w->a[i];
        if (a->kind != AK_OUTLAW) continue;
        if (!a->alive) {
            if (!w->result) w->end_t = DX_END_FRAMES;
            w->result = DX_LOST;
            if (!w->why) w->why = WHY_SHOT;
            return;
        }
        if (a->escaped != 1) all_out = false;
        if (!a->escaped) any_left = true;
    }
    if (w->versus) {
        if (w->coins >= 10) { w->result = DX_WON; return; }
        if (w->turns >= 10) { w->result = DX_LOST; w->why = WHY_TIME; return; }
    } else if (all_out && objective_done(w)) {
        w->result = DX_WON;
        return;
    }
    if (me->escaped && any_left) dx_swap_outlaw(w);
}

/* ------------------------------------------------------------------ */
/* 2P Versus: a long train, dealt fresh each time                       */

static void vs_set(DxWorld *w, int x, int y, int t) {
    if (x >= 0 && x < DX_MAX_W && y >= 0 && y < DX_H) w->tile[y][x] = (uint8_t)t;
}

void dx_versus_train(DxWorld *w, uint64_t seed) {
    memset(w, 0, sizeof *w);
    rng_seed(&w->rng, seed);
    w->versus = 1;
    int x = 0, ncar = 9 + rng_range(&w->rng, 0, 2);
    int boxes = 7, guards = 4;
    int box_car[DX_MAX_CARS], guard_car[DX_MAX_CARS];
    memset(box_car, 0, sizeof box_car);
    memset(guard_car, 0, sizeof guard_car);
    for (int k = 0; k < boxes; k++) box_car[1 + rng_range(&w->rng, 0, ncar - 2)]++;
    for (int k = 0; k < guards; k++) guard_car[2 + rng_range(&w->rng, 0, ncar - 3)]++;
    for (int c = 0; c < ncar && x < DX_MAX_W - 20; c++) {
        int cw = 12 + rng_range(&w->rng, 0, 4);
        int kind = c == 0 ? 0 : rng_range(&w->rng, 0, 3); /* 0 flat, 1 box car, 2 passenger, 3 tank */
        if (kind == 0) {
            for (int k = 0; k < cw; k++) vs_set(w, x + k, 7, TL_FLOOR);
        } else {
            for (int k = 0; k < cw; k++) { vs_set(w, x + k, 3, TL_ROOF); vs_set(w, x + k, 7, TL_FLOOR); }
            if (kind == 3) for (int k = 1; k < cw - 1; k++) for (int y = 4; y <= 6; y++) vs_set(w, x + k, y, TL_ARMOR);
            else {
                int h = 2 + rng_range(&w->rng, 0, cw - 5);
                for (int y = 3; y <= 6; y++) vs_set(w, x + h, y, TL_LADDER);
                if (kind == 2 && cw > 13) {
                    /* a partition with a low gap to roll through */
                    int p = cw / 2 + (h < cw / 2 ? 2 : -2);
                    vs_set(w, x + p, 4, TL_WALL);
                    vs_set(w, x + p, 5, TL_WALL);
                }
            }
        }
        vs_set(w, x + 1, 8, TL_WHEELS);
        vs_set(w, x + cw - 2, 8, TL_WHEELS);
        /* what's inside: strongboxes, cover, the guards */
        for (int k = 0; k < box_car[c]; k++) {
            int bx = x + 2 + rng_range(&w->rng, 0, cw - 4);
            int row = kind == 3 ? 2 : 6;
            int id = add_obj(w, OB_LOOT, (float)(bx * DX_T + 1), (float)(row * DX_T + 2));
            if (id >= 0) w->o[id].loot_id = (uint8_t)(k % 8);
        }
        int cover = rng_range(&w->rng, 0, 2);
        for (int k = 0; k < cover; k++) {
            int bx = x + 2 + rng_range(&w->rng, 0, cw - 4);
            add_obj(w, rng_range(&w->rng, 0, 1) ? OB_CRATE : OB_BARREL, (float)(bx * DX_T + 1), (float)((kind == 3 ? 2 : 6) * DX_T + 2));
        }
        for (int k = 0; k < guard_car[c]; k++) {
            int gx = x + 3 + rng_range(&w->rng, 0, cw - 6);
            int row = kind == 3 ? 3 : 7;
            add_actor(w, AK_GUARD, 0, (float)(gx * DX_T + 2), (float)((row - 2) * DX_T + 2), rng_range(&w->rng, 0, 1));
        }
        x += cw;
        /* the coupling, and a ladder up to the roofs */
        vs_set(w, x, 7, TL_COUPLING);
        for (int y = 3; y <= 6; y++) vs_set(w, x, y, TL_LADDER);
        x += 1;
    }
    w->w = x;
    add_actor(w, AK_OUTLAW, OUT_WADE, 2 * DX_T + 2, 5 * DX_T + 2, 1);
    add_actor(w, AK_OUTLAW, OUT_PEARL, 4 * DX_T + 2, 5 * DX_T + 2, 1);
    for (int i = 0; i < w->na; i++) if (w->a[i].kind == AK_OUTLAW) { w->a[i].ammo = 2; }
    for (int i = 0; i < w->na; i++) if (w->a[i].kind != AK_OUTLAW) { w->law_ctrl = (uint8_t)i; w->lawmen_total++; }
    compute_cars(w);
    for (int i = 0; i < w->na; i++)
        if (w->a[i].kind == AK_OUTLAW) { w->ctrl = i; break; }
    w->phase = PH_PLAYER;
    w->turn_t = DX_TURN_FRAMES;
    w->master = 1 << 30;
}
