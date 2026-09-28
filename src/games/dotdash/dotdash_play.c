/* DOT & DASH - play: Dot's moves at every size, carrying and throwing,
 * the creatures, Dash, the bosses and the change of size. */
#include "dotdash.h"

Ent dd_ent[DD_MAX_ENTS];
Body dd_p;
int dd_hp, dd_hp_max;
int dd_carry = -1;
int dd_shrink_t, dd_grow_t;
int dd_hurt_t, dd_dead_t, dd_frozen_t, dd_flash_t;
float dd_cam_x, dd_cam_y;
int dd_dash = -1;
int dd_trans, dd_trans_dir, dd_trans_kind;
int dd_sniff_x = -1, dd_sniff_y = -1;
bool dd_no_foes, dd_god;
int dd_lamp_dark;

/* world helpers from dotdash_world.c */
void dd_push_level(const LevelDesc *d);
void dd_pop_level(void);
void dd_replace_level(const LevelDesc *d);
const Level *dd_parent_level(void);
int dd_door_target(int id, int *area, int *tx, int *ty);
bool dd_door_is(int id, const char *what);

#define SHRINK_T 90        /* hold DOWN this long to shrink */
#define GROW_T 90          /* hold UP this long to grow */
#define LIFT_T 16          /* frames to lift a thing (Grip Mitt II: 4) */
#define HURT_T 60
#define TAP_GAP 14         /* two presses within this many frames are a double tap */
#define FALL_TILES 12      /* a fall longer than this sends Dot back to full size */

const Phys DD_PHYS_FULL = {DD_TS0, 1.1f, 1.1f, 0.2f, 0.15f, 0.2f, 5.0f, 3.2f, 3.2f};
const Phys DD_PHYS_SMALL = {DD_TS, 1.25f, 2.6f, 0.2f, 0.15f, 0.25f, 5.5f, 4.47f, 5.48f};
static const float BEAN_V[3] = {4.47f, 5.48f, 6.0f};

const Phys *dd_phys(void) { return dd_scale == SC_FULL ? &DD_PHYS_FULL : &DD_PHYS_SMALL; }

/* ------------------------------------------------------------------ */
/* tile collision                                                       */

static int tflags(const Level *L, int ts, float px, float py) {
    int x = (int)floorf(px / (float)ts), y = (int)floorf(py / (float)ts);
    return DD_TILE[lv_tile(L, x, y)].flags;
}

bool dd_body_blocked(const Level *L, int ts, float x, float y, float w, float h) {
    int x0 = (int)floorf(x / (float)ts), x1 = (int)floorf((x + w - 0.01f) / (float)ts);
    int y0 = (int)floorf(y / (float)ts), y1 = (int)floorf((y + h - 0.01f) / (float)ts);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (DD_TILE[lv_tile(L, tx, ty)].flags & TF_SOLID) return true;
    return false;
}

/* the floor right under a rect: 0 none, else the tile flags */
static int floor_under(const Level *L, int ts, float x, float y, float w, float h, bool drop) {
    float by = y + h + 0.5f;
    int r = 0;
    for (float xx = x + 0.5f; xx < x + w; xx += (float)ts) r |= tflags(L, ts, xx, by);
    r |= tflags(L, ts, x + w - 0.5f, by);
    if (drop) r &= ~TF_ONEWAY;
    if (r & TF_ONEWAY) {
        /* a ledge only holds you if your feet are at its top */
        float top = floorf(by / (float)ts) * (float)ts;
        if (fabsf(y + h - top) > 1.0f) r &= ~TF_ONEWAY;
    }
    return r & (TF_SOLID | TF_ONEWAY | TF_BOUNCE);
}

void dd_body_step(Body *b, uint32_t in, uint32_t prev, const Level *L, const Phys *ph, bool bangle, int bean) {
    (void)bangle;
    int ts = ph->ts;
    float maxv = b->sprint ? ph->sprint : ph->walk;
    float target = 0;
    if (!b->crouch) {
        if (in & BTN_LEFT) target -= maxv;
        if (in & BTN_RIGHT) target += maxv;
    }
    float acc = b->ground ? (target == 0 ? ph->accel * 1.5f : ph->accel) : ph->air;
    b->vx = fapproach(b->vx, target, acc);
    if (target < 0) b->facing = 0;
    else if (target > 0) b->facing = 1;
    bool a_now = in & BTN_A, a_was = prev & BTN_A;
    b->landed_fall = 0;
    b->bounced = 0;
    if (a_now && !a_was && (b->ground || b->coyote > 0)) {
        float v = ph->jump;
        if (ts == DD_TS && (in & BTN_UP) && bean > 0) v = BEAN_V[iclamp(bean, 0, 2)];
        b->vy = -v;
        b->ground = 0;
        b->coyote = 0;
        b->jumping = 1;
        b->peak_y = (int16_t)b->y;
    }
    if (b->jumping && !a_now && b->vy < -1.2f) b->vy = -1.2f;
    if (b->vy >= 0) b->jumping = 0;
    b->vy += ph->grav;
    if (b->vy > ph->maxfall) b->vy = ph->maxfall;
    if (b->drop_t > 0) b->drop_t--;

    /* horizontal, with a one-tile step up while walking */
    float nx = b->x + b->vx;
    if (dd_body_blocked(L, ts, nx, b->y, b->w, b->h)) {
        if (b->ground && !dd_body_blocked(L, ts, nx, b->y - (float)ts, b->w, b->h) &&
            !dd_body_blocked(L, ts, b->x, b->y - (float)ts, b->w, b->h)) {
            b->y -= (float)ts;
            b->x = nx;
            /* settle back onto the step */
            float yy = b->y;
            for (int k = 0; k < ts && !dd_body_blocked(L, ts, b->x, yy + 1, b->w, b->h); k++) yy += 1;
            b->y = yy;
        } else {
            if (b->vx > 0) b->x = floorf((nx + b->w) / (float)ts) * (float)ts - (float)b->w - 0.01f;
            else if (b->vx < 0) b->x = (floorf(nx / (float)ts) + 1) * (float)ts;
            if (dd_body_blocked(L, ts, b->x, b->y, b->w, b->h)) b->x -= b->vx; /* safety */
            b->vx = 0;
        }
    } else b->x = nx;

    /* vertical */
    bool was_ground = b->ground;
    float oy = b->y, ny = b->y + b->vy;
    b->ground = 0;
    if (b->vy > 0) {
        int r0 = (int)floorf((oy + b->h) / (float)ts), r1 = (int)floorf((ny + b->h) / (float)ts);
        bool landed = false;
        for (int r = r0; r <= r1 && !landed; r++) {
            float top = (float)(r * ts);
            if (top < oy + b->h - 0.01f) continue;
            if (top > ny + b->h) break;
            int x0 = (int)floorf((b->x + 0.01f) / (float)ts), x1 = (int)floorf((b->x + b->w - 0.01f) / (float)ts);
            int fl = 0;
            for (int tx = x0; tx <= x1; tx++) fl |= DD_TILE[lv_tile(L, tx, r)].flags;
            if ((fl & TF_SOLID) || ((fl & TF_ONEWAY) && b->drop_t == 0)) {
                ny = top - (float)b->h;
                landed = true;
                if (fl & TF_BOUNCE) b->bounced = 1;
            }
        }
        b->y = ny;
        if (landed) {
            b->vy = 0;
            b->ground = 1;
            if (!was_ground) b->landed_fall = (int16_t)imax(0, (int)(b->y - b->peak_y));
            if (b->bounced) { b->vy = -6.5f; b->ground = 0; b->peak_y = (int16_t)b->y; }
        }
    } else if (b->vy < 0) {
        if (dd_body_blocked(L, ts, b->x, ny, b->w, b->h)) {
            ny = (floorf(ny / (float)ts) + 1) * (float)ts;
            b->vy = 0;
        }
        b->y = ny;
    }
    if (!b->ground && b->vy >= 0 && floor_under(L, ts, b->x, b->y, b->w, b->h, b->drop_t > 0)) {
        b->ground = 1;
        if (!was_ground) b->landed_fall = (int16_t)imax(0, (int)(b->y - b->peak_y));
    }
    if (b->ground) { b->coyote = 5; b->peak_y = (int16_t)b->y; }
    else {
        if (b->coyote > 0) b->coyote--;
        if (was_ground && !b->jumping) b->peak_y = (int16_t)b->y;
        if (b->y < b->peak_y) b->peak_y = (int16_t)b->y;
    }
}

/* ------------------------------------------------------------------ */
/* entities                                                             */

typedef struct FoeDef { int16_t w, h, hp, dmg; float speed; uint16_t flags; } FoeDef;
enum { FF_BUG = 1, FF_GERM = 2, FF_FLY = 4, FF_HOP = 8, FF_BOSS = 16, FF_SHOOT = 32, FF_NOLIFT = 64, FF_SAFE = 128, FF_SPIKY = 256 };
static const FoeDef FOE[F_KINDS] = {
    [F_ANT] = {8, 6, 1, 1, 0.5f, FF_BUG},
    [F_LANCER] = {8, 8, 2, 1, 0.45f, FF_BUG | FF_SHOOT},
    [F_AXEANT] = {8, 7, 2, 1, 0.4f, FF_BUG},
    [F_MOTH] = {10, 7, 1, 1, 0.4f, FF_BUG | FF_FLY},
    [F_BUZZER] = {8, 6, 1, 1, 1.2f, FF_BUG | FF_FLY},
    [F_FLUTTER] = {10, 8, 1, 0, 0.5f, FF_BUG | FF_FLY | FF_SAFE},
    [F_BUMBLE] = {8, 7, 1, 1, 0.8f, FF_BUG | FF_FLY},
    [F_SPRING] = {7, 6, 1, 1, 0.6f, FF_BUG | FF_HOP},
    [F_NIP] = {6, 6, 1, 1, 0.5f, FF_BUG | FF_HOP},
    [F_MITE] = {8, 6, 1, 1, 0.35f, FF_BUG},
    [F_PAPERFISH] = {10, 5, 2, 1, 1.0f, FF_BUG},
    [F_GERM] = {8, 8, 1, 1, 0.4f, FF_GERM | FF_FLY},
    [F_GERM2] = {8, 8, 1, 1, 0.4f, FF_GERM | FF_FLY},
    [F_WIGGLER] = {10, 4, 1, 1, 0.3f, FF_GERM},
    [F_GULP] = {12, 9, 3, 1, 0.6f, 0},
    [F_POD] = {8, 8, 3, 1, 0, FF_SHOOT | FF_NOLIFT},
    [F_TINMOUSE] = {10, 10, 3, 2, 0.6f, 0},
    [F_TINMAGE] = {10, 14, 6, 2, 0.3f, FF_FLY | FF_SHOOT | FF_NOLIFT},
    [F_WAXGUARD] = {8, 12, 2, 1, 0.4f, 0},
    [F_SPARK] = {6, 6, 1, 1, 0.7f, FF_FLY | FF_SPIKY},
    [F_BEETLE] = {10, 7, 9999, 1, 0.3f, FF_BUG},
    [F_HOPPER] = {8, 8, 1, 0, 0, FF_SAFE},
    [F_CLIMBER] = {8, 6, 2, 0, 0.4f, FF_BUG},
    [F_SPITTER] = {8, 8, 2, 1, 0, FF_BUG | FF_SHOOT},
    [F_OCTO] = {10, 8, 3, 1, 0.3f, 0},
    [F_FACE] = {8, 8, 1, 0, 0.2f, FF_SAFE},
    [F_SCAMPER] = {7, 7, 1, 0, 1.4f, FF_SAFE},
    [F_FERRY] = {16, 6, 99, 0, 1.0f, FF_FLY | FF_SAFE | FF_NOLIFT},
    [F_TANK] = {12, 8, 4, 1, 0.25f, FF_SHOOT | FF_NOLIFT},
    [F_PLANE] = {14, 6, 2, 1, 1.1f, FF_FLY | FF_SHOOT},
    [F_ROTIFER] = {32, 20, 10, 2, 0.5f, FF_BOSS | FF_NOLIFT},
    [F_EARWIG] = {40, 16, 60, 2, 0.8f, FF_BUG | FF_BOSS | FF_NOLIFT},
    [F_SPROCKET] = {32, 48, 1, 2, 0.5f, FF_BOSS | FF_NOLIFT},
};

void dd_foe_size(int sub, int *w, int *h) { *w = FOE[sub].w; *h = FOE[sub].h; }
bool dd_foe_is_bug(int sub) { return FOE[sub].flags & FF_BUG; }
bool dd_foe_is_germ(int sub) { return FOE[sub].flags & FF_GERM; }

int dd_add_ent(int kind, int sub, float x, float y) {
    for (int i = 0; i < DD_MAX_ENTS; i++)
        if (!dd_ent[i].alive) {
            Ent *e = &dd_ent[i];
            memset(e, 0, sizeof *e);
            e->alive = 1;
            e->kind = (uint8_t)kind;
            e->sub = (uint8_t)sub;
            e->x = x;
            e->y = y;
            e->w = 8;
            e->h = 8;
            e->dir = 1;
            if (kind == EK_FOE) {
                e->w = FOE[sub].w;
                e->h = FOE[sub].h;
                e->hp = FOE[sub].hp;
                e->home_x = (int16_t)x;
                e->home_y = (int16_t)y;
            }
            return i;
        }
    return -1;
}
int dd_add_obj(int sub, float x, float y) { return dd_add_ent(EK_OBJ, sub, x, y); }
int dd_add_foe(int sub, float x, float y) {
    if (dd_no_foes && sub < F_FIRST_BOSS) return -1;
    return dd_add_ent(EK_FOE, sub, x, y);
}
int dd_add_pick(int sub, float x, float y, int param) {
    int i = dd_add_ent(EK_PICK, sub, x, y);
    if (i >= 0) { dd_ent[i].param = param; dd_ent[i].w = 7; dd_ent[i].h = 7; }
    return i;
}

bool dd_obj_exists(int sub, int param) {
    for (int i = 0; i < DD_MAX_ENTS; i++)
        if (dd_ent[i].alive && dd_ent[i].kind == EK_OBJ && dd_ent[i].sub == sub && (param < 0 || dd_ent[i].param == param)) return true;
    if (dd_sv.carry == sub + 1 && (param < 0 || dd_sv.carry_param == param)) return true;
    for (int k = 0; k < 2; k++)
        if (dd_sv.satchel[k] == sub + 1 && (param < 0 || dd_sv.satchel_param[k] == param)) return true;
    return false;
}
int dd_obj_home_count(int sub) { return dd_obj_exists(sub, -1); }

/* ------------------------------------------------------------------ */
/* particles and words                                                   */

typedef struct Part { float x, y, vx, vy; int16_t life, col; } Part;
static Part parts[DD_MAX_PARTS];
typedef struct Pop { float x, y; int16_t life, col; char s[24]; } Pop;
static Pop pops[8];
static int shake_t, freeze_all_t;

void dd_part(float x, float y, float vx, float vy, int life, int col) {
    for (int i = 0; i < DD_MAX_PARTS; i++)
        if (parts[i].life <= 0) { parts[i] = (Part){x, y, vx, vy, (int16_t)life, (int16_t)col}; return; }
}
void dd_burst(float x, float y, int col, int n) {
    for (int k = 0; k < n; k++) {
        float a = (float)k * 6.283f / (float)n + (float)(engine_frame() % 7);
        dd_part(x, y, cosf(a) * 1.4f, sinf(a) * 1.4f - 0.5f, 14 + k % 8, col);
    }
}
void dd_popup(float x, float y, const char *s, int col) {
    int best = 0;
    for (int i = 0; i < 8; i++) if (pops[i].life < pops[best].life) best = i;
    pops[best].x = x; pops[best].y = y; pops[best].life = 50; pops[best].col = (int16_t)col;
    snprintf(pops[best].s, sizeof pops[best].s, "%s", s);
}

/* ------------------------------------------------------------------ */
/* the player's state                                                    */

static int lift_t, lift_target = -1, kick_t, throw_t, riding = -1;
static int tap_age[4] = {99, 99, 99, 99};
static bool dbl[4];
static int talk_lock, shield_toggle, goo_t, message_t;
static char message[64];
static int pending_action; /* scale change waiting for the iris */
static int save_wait;
static bool save_dirty;
static float ride_dx, ride_dy;
static int ride_lr = -1;

enum { PA_NONE, PA_SHRINK, PA_GROW, PA_DOOR, PA_DEATH, PA_SPECIAL };
static int pa_door;

static void flash_message(const char *s) { snprintf(message, sizeof message, "%s", s); message_t = 150; }
const char *dd_message(void) { return message_t > 0 ? message : NULL; }

int dd_pep(void) {
    int n = 1;
    for (int k = 0; k < 8; k++) n += (dd_sv.eggs_got >> k) & 1;
    return n;
}
int dd_hp_max_now(void) {
    int m = 6;
    for (int k = 0; k < 8; k++)
        if ((dd_sv.hearts_got >> k) & 1) m += k < 6 ? 2 : 1;
    return m;
}
int dd_upgrade_count(void) {
    int n = 0;
    for (int u = 0; u < U_ABILITIES; u++) n += dd_has(u);
    for (int k = 0; k < 8; k++) n += ((dd_sv.hearts_got >> k) & 1) + ((dd_sv.eggs_got >> k) & 1);
    return n;
}

void dd_autosave(void) {
    save_dirty = true;
    save_wait = 0;
}

static void sync_save(void) {
    dd_sv.px = (int16_t)dd_p.x;
    dd_sv.py = (int16_t)dd_p.y;
    dd_sv.depth = (uint8_t)dd_depth;
    memcpy(dd_sv.stack, dd_stack, sizeof dd_stack);
    dd_sv.full = dd_scale == SC_FULL;
    dd_sv.carry = 0;
    dd_sv.carry_param = 0;
    if (dd_carry >= 0 && dd_ent[dd_carry].alive && dd_ent[dd_carry].kind == EK_OBJ) {
        dd_sv.carry = (uint8_t)(dd_ent[dd_carry].sub + 1);
        dd_sv.carry_param = (int16_t)dd_ent[dd_carry].param;
    }
}

void dd_save_now(void) {
    sync_save();
    if (!dd_quiet) game_save_write(game_current_index(), &dd_sv, (int)sizeof dd_sv);
    save_dirty = false;
}

void dd_give_glints(int n) {
    dd_sv.glints += n;
    dd_sv.glints_total += n;
    dd_check_goals();
    dd_autosave();
}

/* upgrades with two levels: every spot that holds one gives the next level
 * the line is missing */
static int line_second(int u) {
    switch (u) {
    case U_MITT1: return U_MITT2;
    case U_TONIC1: return U_TONIC2;
    case U_SATCHEL1: return U_SATCHEL2;
    case U_BEAN1: return U_BEAN2;
    case U_CLOGS1: return U_CLOGS2;
    case U_BUZZ1: return U_BUZZ2;
    case U_TOP1: return U_TOP2;
    }
    return -1;
}
static int line_first(int u) {
    for (int b = 0; b < U_ABILITIES; b++)
        if (line_second(b) == u) return b;
    return u;
}
static bool got(int u) {
    if (u < U_ABILITIES) return dd_has(u);
    if (u < U_EGG0) return (dd_sv.hearts_got >> (u - U_HEART0)) & 1;
    return (dd_sv.eggs_got >> (u - U_EGG0)) & 1;
}

/* what a spot holding upgrade u gives now: the next level of its line, the
 * next heart button (whole or half) or the next pep egg; -1 when full */
int dd_resolve_upgrade(int u) {
    if (u < 0 || u >= U_TOTAL) return -1;
    if (u < U_ABILITIES) {
        int a = line_first(u), b = line_second(a);
        if (!dd_has(a)) return a;
        if (b >= 0 && !dd_has(b)) return b;
        return -1;
    }
    int lo = u < U_EGG0 ? (u - U_HEART0 < 6 ? U_HEART0 : U_HEART0 + 6) : U_EGG0;
    int hi = u < U_EGG0 ? (u - U_HEART0 < 6 ? U_HEART0 + 6 : U_EGG0) : U_TOTAL;
    for (int k = lo; k < hi; k++)
        if (!got(k)) return k;
    return -1;
}

/* a found or given upgrade: the next one of its kind, or glints when that
 * kind is full */
void dd_award_upgrade(int u) {
    int r = dd_resolve_upgrade(u);
    if (r >= 0) { dd_give_upgrade(r); return; }
    dd_give_glints(5);
    dd_popup(dd_p.x, dd_p.y - 12, "+5", C_YELLOW);
}

void dd_give_upgrade(int u) {
    if (u < U_ABILITIES) {
        dd_sv.ups |= 1u << u;
        if (line_first(u) != u) dd_sv.ups |= 1u << line_first(u); /* a second level brings the first */
    }
    else if (u < U_EGG0) dd_sv.hearts_got |= (uint8_t)(1u << (u - U_HEART0));
    else dd_sv.eggs_got |= (uint8_t)(1u << (u - U_EGG0));
    dd_hp_max = dd_hp_max_now();
    dd_hp = dd_hp_max;
    char buf[64];
    if (u < U_ABILITIES) snprintf(buf, sizeof buf, "%s! %s", DD_UPNAME[u], DD_UPDESC[u]);
    else if (u < U_EGG0) snprintf(buf, sizeof buf, "%s! MORE HEARTS", u - U_HEART0 < 6 ? "HEART BUTTON" : "HALF BUTTON");
    else snprintf(buf, sizeof buf, "PEP EGG! THROWS HIT HARDER");
    flash_message(buf);
    music_play(DD_JINGLE_UP);
    dd_autosave();
}

/* ------------------------------------------------------------------ */
/* hurting Dot                                                            */

void dd_hurt(int halves, float from_x) {
    if (dd_hurt_t > 0 || dd_dead_t > 0 || dd_god || dd_trans) return;
    if (dd_has(U_SHELL)) {
        if (halves == 1) { shield_toggle ^= 1; halves = shield_toggle; }
        else halves = (halves + 1) / 2;
    }
    dd_hurt_t = HURT_T;
    if (halves <= 0) { sfx_play_name("dd_block"); return; }
    dd_hp -= halves;
    sfx_play_name("dd_hurt");
    dd_p.vx = from_x < dd_p.x + dd_p.w / 2 ? 1.8f : -1.8f;
    dd_p.vy = -2.2f;
    dd_p.ground = 0;
    riding = -1;
    shake_t = 8;
    if (dd_hp <= 0) {
        dd_hp = 0;
        dd_dead_t = 70;
        sfx_play_name("dd_die");
    }
}

/* ------------------------------------------------------------------ */
/* moving things (objects, creatures, Dash) against the tiles            */

#define TSN (dd_scale == SC_FULL ? DD_TS0 : DD_TS)

static void ent_move(Ent *e, float grav, bool bounce) {
    const Level *L = &dd_lv;
    const int ts = TSN;
    e->vy += grav;
    if (e->vy > 5.0f) e->vy = 5.0f;
    float nx = e->x + e->vx;
    if (dd_body_blocked(L, ts, nx, e->y, e->w, e->h)) {
        if (bounce) e->vx = -e->vx * 0.5f;
        else e->vx = 0;
        e->param2 |= 0x10000; /* hit a wall this frame */
    } else e->x = nx;
    float ny = e->y + e->vy;
    e->ground = 0;
    if (e->vy > 0) {
        int r = (int)floorf((ny + e->h) / (float)ts);
        float top = (float)(r * ts);
        int fl = 0;
        for (int tx = (int)floorf((e->x + 0.5f) / ts); tx <= (int)floorf((e->x + e->w - 0.5f) / ts); tx++) fl |= DD_TILE[lv_tile(L, tx, r)].flags;
        if (top >= e->y + e->h - 0.01f && ((fl & TF_SOLID) || (fl & TF_ONEWAY))) {
            ny = top - e->h;
            e->vy = 0;
            e->ground = 1;
        } else if (dd_body_blocked(L, ts, e->x, ny, e->w, e->h)) {
            ny = floorf((ny + e->h) / (float)ts) * (float)ts - e->h;
            e->vy = 0;
            e->ground = 1;
        }
    } else if (e->vy < 0 && dd_body_blocked(L, ts, e->x, ny, e->w, e->h)) {
        ny = (floorf(ny / (float)ts) + 1) * (float)ts;
        e->vy = 0;
    }
    e->y = ny;
    if (e->y > L->h * ts + 40) e->alive = 0;
}

static bool ent_overlap(const Ent *a, const Ent *b) {
    return rects_overlap((int)a->x, (int)a->y, a->w, a->h, (int)b->x, (int)b->y, b->w, b->h);
}
static bool player_overlap(const Ent *e) {
    return rects_overlap((int)dd_p.x, (int)dd_p.y, dd_p.w, dd_p.h, (int)e->x, (int)e->y, e->w, e->h);
}

/* ------------------------------------------------------------------ */
/* creatures                                                              */

void dd_kill_foe(int i) {
    Ent *e = &dd_ent[i];
    if (!e->alive) return;
    int sub = e->sub;
    e->alive = 0;
    if (dd_carry == i) dd_carry = -1;
    if (riding == i) riding = -1;
    float ex = e->x, ey = e->y, ew = e->w;
    int eparam = e->param; /* read before the slot can be reused */
    dd_burst(ex + ew / 2, ey + e->h / 2, sub == F_GERM ? C_LIME : C_WHITE, 10);
    sfx_play_name("dd_pop");
    /* quests read the creature before its slot is reused */
    dd_on_kill(sub, i);
    if (FOE[sub].flags & FF_BOSS) dd_on_boss_dead(sub);
    if (!(FOE[sub].flags & FF_BOSS) && (engine_frame() + (unsigned)i) % 3 == 0) dd_add_pick(P_GLINT1, ex + ew / 2 - 3, ey, 0);
    if (sub == F_AXEANT) dd_add_obj(O_AXE, ex, ey);
    if (sub == F_OCTO && eparam == 1 && !dd_flag(FL_REDEGG_DONE) && !dd_obj_exists(O_REDEGG, -1)) dd_add_obj(O_REDEGG, ex, ey);
}

static void damage_foe(int i, int dmg, float from_x) {
    Ent *e = &dd_ent[i];
    if (!e->alive || e->kind != EK_FOE) return;
    if (e->sub == F_BEETLE || e->sub == F_SPROCKET || (FOE[e->sub].flags & FF_SAFE)) { sfx_play_name("dd_block"); return; }
    if (e->sub == F_ROTIFER && e->state != 1) { sfx_play_name("dd_block"); return; }
    if (e->hurt_t > 0) return;
    e->hp -= (int16_t)dmg;
    e->hurt_t = (FOE[e->sub].flags & FF_BOSS) ? 20 : 6;
    e->vx = from_x < e->x ? 1.5f : -1.5f;
    if (!(FOE[e->sub].flags & FF_BOSS)) e->vy = -1.5f;
    sfx_play_name("dd_hit");
    if (e->hp <= 0) dd_kill_foe(i);
}

static void foe_shoot(Ent *e, float vx, float vy, int kind) {
    int j = dd_add_ent(EK_SHOT, kind, e->x + e->w / 2 - 2, e->y + e->h / 3);
    if (j >= 0) { dd_ent[j].vx = vx; dd_ent[j].vy = vy; dd_ent[j].w = 4; dd_ent[j].h = 4; dd_ent[j].t = 200; dd_ent[j].param = kind == 1; }
}

static float px_c(void) { return dd_p.x + dd_p.w / 2; }

/* a fireball: it breaks weak blocks where it lands; Dot's own (from a
 * carried or commanded spitter) burn creatures, not her */
static void spit_fire(const Ent *e, bool friendly) {
    int j = dd_add_ent(EK_SHOT, 4, e->x + e->w / 2 - 3, e->y + 1);
    if (j >= 0) {
        dd_ent[j].vx = e->dir ? 2.6f : -2.6f;
        dd_ent[j].vy = 0;
        dd_ent[j].w = 6;
        dd_ent[j].h = 3;
        dd_ent[j].t = 160;
        dd_ent[j].param = friendly ? 2 : 1;
    }
    sfx_play_name("dd_shoot");
}
static float py_c(void) { return dd_p.y + dd_p.h / 2; }

static bool ledge_ahead(Ent *e) {
    float fx = e->dir ? e->x + e->w + 1 : e->x - 1;
    int fl = tflags(&dd_lv, TSN, fx, e->y + e->h + 2);
    return (fl & (TF_SOLID | TF_ONEWAY)) != 0;
}

static void boss_update(int i);
static void climber_update(Ent *e);
static void ferry_update(Ent *e);
static void gulp_swallow(int i);

static void foe_update(int i) {
    Ent *e = &dd_ent[i];
    const FoeDef *D = &FOE[e->sub];
    e->t++;
    if (e->hurt_t > 0) e->hurt_t--;
    if (e->held) return;
    if (e->state == 9) { /* thrown: a creature is a projectile until it hits */
        ent_move(e, e->st == 1 ? 0.25f : 0.1f, false);
        bool hit = (e->param2 & 0x10000) || e->ground;
        for (int j = 0; j < DD_MAX_ENTS && !hit; j++)
            if (j != i && dd_ent[j].alive && dd_ent[j].kind == EK_FOE && dd_ent[j].state != 9 && ent_overlap(e, &dd_ent[j])) {
                damage_foe(j, dd_pep() + 1, e->x);
                hit = true;
            }
        e->param2 &= ~0x10000;
        if (hit) {
            if (e->sub == F_BEETLE) { e->state = 0; e->vx = 0; }
            else dd_kill_foe(i);
        }
        return;
    }
    if (freeze_all_t > 0 || e->frozen) return;
    if (e->poison > 0) {
        if (e->t % 40 == 0) { damage_foe(i, 1, e->x); if (!e->alive) return; e->hurt_t = 0; }
        if (e->t % 6 == 0) dd_part(e->x + e->w / 2, e->y, 0, -0.4f, 12, C_LIME);
        for (int j = 0; j < DD_MAX_ENTS; j++)
            if (j != i && dd_ent[j].alive && dd_ent[j].kind == EK_FOE && !dd_ent[j].poison && !(FOE[dd_ent[j].sub].flags & FF_BOSS) && ent_overlap(e, &dd_ent[j]))
                dd_ent[j].poison = 1; /* contagious */
    }
    if (e->stun > 0) { e->stun--; ent_move(e, (D->flags & FF_FLY) ? 0 : 0.25f, false); e->vx *= 0.8f; return; }
    if (D->flags & FF_BOSS) { boss_update(i); return; }
    /* commanded by the Buzzbook */
    if (e->cmd) {
        e->st++;
        switch (e->sub) {
        case F_MOTH: case F_BUMBLE: case F_GERM: case F_GERM2: e->vx = 0; e->vy = -1.2f; break;
        case F_BUZZER: case F_FLUTTER: e->vx = e->dir ? 2.0f : -2.0f; e->vy = e->sub == F_FLUTTER ? -0.6f : 0; break;
        case F_SPRING: case F_NIP: if (e->ground) e->vy = -6.0f; e->vx = e->dir ? 1.0f : -1.0f; break;
        case F_SPITTER: e->vx = 0; if (e->st % 20 == 1) spit_fire(e, true); break;
        default: e->vx = e->dir ? 1.6f : -1.6f; break;
        }
        if (D->flags & FF_FLY) {
            float nx = e->x + e->vx, ny = e->y + e->vy;
            bool bx = dd_body_blocked(&dd_lv, DD_TS, nx, e->y, e->w, e->h), by = dd_body_blocked(&dd_lv, DD_TS, e->x, ny, e->w, e->h);
            if (!bx) e->x = nx;
            if (!by) e->y = ny;
            if ((bx && e->vx != 0) || (by && e->vy != 0) || e->st > 400) { e->cmd = 0; e->home_y = (int16_t)e->y; e->home_x = (int16_t)e->x; e->pad = 1; e->t = 0; }
        } else {
            ent_move(e, 0.25f, false);
            if ((e->param2 & 0x10000) || e->st > 300) e->cmd = 0;
            e->param2 &= ~0x10000;
        }
        return;
    }
    switch (e->sub) {
    case F_MOTH: {
        e->vx = 0;
        float target = (float)e->home_y + sinf((float)e->t * 0.02f) * (e->pad ? 3.0f : 30.0f);
        e->vy = iclamp((int)((target - e->y) * 10), -6, 6) / 10.0f;
        float ny = e->y + e->vy;
        if (!dd_body_blocked(&dd_lv, DD_TS, e->x, ny, e->w, e->h)) e->y = ny;
        e->dir = (e->t / 60) % 2;
        break;
    }
    case F_BUZZER: {
        float nx = e->x + (e->dir ? D->speed : -D->speed);
        if (dd_body_blocked(&dd_lv, DD_TS, nx, e->y, e->w, e->h) || fabsf(nx - e->home_x) > 90) e->dir ^= 1;
        else e->x = nx;
        e->y = (float)e->home_y + sinf((float)e->t * 0.1f) * 3.0f;
        break;
    }
    case F_FLUTTER: {
        float nx = e->x + (e->dir ? 0.4f : -0.4f), ny = (float)e->home_y + sinf((float)e->t * 0.05f) * 10.0f;
        if (dd_body_blocked(&dd_lv, DD_TS, nx, e->y, e->w, e->h) || fabsf(nx - e->home_x) > 60) e->dir ^= 1;
        else e->x = nx;
        if (!dd_body_blocked(&dd_lv, DD_TS, e->x, ny, e->w, e->h)) e->y = ny;
        break;
    }
    case F_BUMBLE: {
        float dx = px_c() - (e->x + e->w / 2), dy = py_c() - (e->y + e->h / 2);
        bool near = fabsf(dx) < 70 && fabsf(dy) < 50;
        float tx = near ? px_c() : (float)e->home_x + sinf((float)e->t * 0.03f) * 30;
        float ty = near ? py_c() - 4 : (float)e->home_y + sinf((float)e->t * 0.07f) * 6;
        e->vx = fclamp((tx - e->x) * 0.02f, -0.8f, 0.8f);
        e->vy = fclamp((ty - e->y) * 0.02f, -0.8f, 0.8f);
        float nx = e->x + e->vx, ny = e->y + e->vy;
        if (!dd_body_blocked(&dd_lv, DD_TS, nx, e->y, e->w, e->h)) e->x = nx;
        if (!dd_body_blocked(&dd_lv, DD_TS, e->x, ny, e->w, e->h)) e->y = ny;
        e->dir = e->vx > 0;
        break;
    }
    case F_GERM: case F_GERM2: case F_SPARK: {
        if (e->sub == F_SPARK) {
            if (e->vx == 0) { e->vx = 0.7f; e->vy = 0.5f; }
            float nx = e->x + e->vx, ny = e->y + e->vy;
            if (dd_body_blocked(&dd_lv, DD_TS, nx, e->y, e->w, e->h)) e->vx = -e->vx; else e->x = nx;
            if (dd_body_blocked(&dd_lv, DD_TS, e->x, ny, e->w, e->h)) e->vy = -e->vy; else e->y = ny;
        } else {
            float dx = px_c() - e->x;
            float nx = e->x + (fabsf(dx) < 100 ? (dx > 0 ? 0.25f : -0.25f) : 0);
            float ny = (float)e->home_y + sinf((float)e->t * 0.04f) * (e->pad ? 2.0f : 8.0f);
            if (!dd_body_blocked(&dd_lv, DD_TS, nx, e->y, e->w, e->h)) e->x = nx;
            if (!dd_body_blocked(&dd_lv, DD_TS, e->x, ny, e->w, e->h)) e->y = ny;
            e->dir = dx > 0;
        }
        break;
    }
    case F_SPRING: case F_NIP: {
        ent_move(e, 0.25f, false);
        e->param2 &= ~0x10000;
        if (e->ground) {
            e->vx = 0;
            if (e->t % 70 == (i * 13) % 70) {
                e->vy = -3.6f;
                e->dir = px_c() > e->x;
                e->vx = e->dir ? D->speed * 1.6f : -D->speed * 1.6f;
            }
        }
        break;
    }
    case F_POD: {
        if (e->t % 120 == 60) {
            sfx_play_name("dd_shoot");
            foe_shoot(e, e->dir ? 2.0f : -2.0f, 0, 1);
        }
        break;
    }
    case F_TINMAGE: {
        float ty = (float)e->home_y - 10 + sinf((float)e->t * 0.03f) * 8;
        float tx = (float)e->home_x + sinf((float)e->t * 0.011f) * 40;
        e->x += fclamp(tx - e->x, -0.5f, 0.5f);
        e->y += fclamp(ty - e->y, -0.5f, 0.5f);
        e->dir = px_c() > e->x;
        if (e->t % 100 == 50 && fabsf(px_c() - e->x) < 160) {
            float dx = px_c() - e->x, dy = py_c() - e->y, d = sqrtf(dx * dx + dy * dy) + 0.01f;
            foe_shoot(e, dx / d * 1.6f, dy / d * 1.6f, 2);
            sfx_play_name("dd_spell");
        }
        break;
    }
    case F_HOPPER:
        break;
    case F_CLIMBER: climber_update(e); break;
    case F_SPITTER:
        ent_move(e, 0.25f, false);
        e->param2 &= ~0x10000;
        if (fabsf(px_c() - e->x) < 140) e->dir = px_c() > e->x;
        if (e->t % 100 == 50 && fabsf(px_c() - e->x) < 140) spit_fire(e, false);
        break;
    case F_FERRY: ferry_update(e); break;
    case F_TANK:
        e->vx = e->dir ? D->speed : -D->speed;
        if (!ledge_ahead(e) && e->ground) e->dir ^= 1, e->vx = -e->vx;
        ent_move(e, 0.25f, false);
        if (e->param2 & 0x10000) e->dir ^= 1;
        e->param2 &= ~0x10000;
        if (e->t % 120 == 60 && fabsf(px_c() - e->x) < 160) {
            float dx = px_c() - (e->x + e->w / 2);
            int j = dd_add_ent(EK_SHOT, 3, e->x + e->w / 2 - 3, e->y - 4);
            if (j >= 0) { dd_ent[j].vx = fclamp(dx / 60.0f, -3, 3); dd_ent[j].vy = -3.5f; dd_ent[j].w = 6; dd_ent[j].h = 6; dd_ent[j].t = 300; }
            sfx_play_name("dd_boom");
        }
        break;
    case F_PLANE: {
        float nx = e->x + (e->dir ? D->speed : -D->speed);
        if (dd_body_blocked(&dd_lv, DD_TS, nx, e->y, e->w, e->h) || fabsf(nx - e->home_x) > 220) e->dir ^= 1;
        else e->x = nx;
        e->y = (float)e->home_y + sinf((float)e->t * 0.05f) * 2.0f;
        if (e->st > 0) e->st--;
        if (e->st == 0 && fabsf(px_c() - (e->x + e->w / 2)) < 10 && py_c() > e->y) {
            int j = dd_add_ent(EK_SHOT, 3, e->x + e->w / 2 - 3, e->y + e->h);
            if (j >= 0) { dd_ent[j].vx = 0; dd_ent[j].vy = 0.5f; dd_ent[j].w = 6; dd_ent[j].h = 6; dd_ent[j].t = 300; }
            e->st = 70;
            sfx_play_name("dd_drop");
        }
        break;
    }
    default: {
        /* the gulp blob goes after the nearest ant it can see */
        if (e->sub == F_GULP && e->t % 20 == 0) {
            float best = 130;
            for (int j = 0; j < DD_MAX_ENTS; j++) {
                const Ent *o = &dd_ent[j];
                if (!o->alive || o->kind != EK_FOE || (o->sub != F_ANT && o->sub != F_LANCER) || fabsf(o->y - e->y) > 16) continue;
                float d = fabsf(o->x - e->x);
                if (d < best) { best = d; e->dir = o->x > e->x; }
            }
        }
        /* walkers: turn at walls and edges */
        e->vx = e->dir ? D->speed : -D->speed;
        if (!ledge_ahead(e) && e->ground) e->dir ^= 1, e->vx = -e->vx;
        ent_move(e, 0.25f, false);
        if (e->param2 & 0x10000) e->dir ^= 1;
        e->param2 &= ~0x10000;
        if (e->sub == F_LANCER && e->t % 90 == 45 && fabsf(py_c() - (e->y + e->h / 2)) < 12 && fabsf(px_c() - e->x) < 130) {
            e->dir = px_c() > e->x;
            foe_shoot(e, e->dir ? 2.2f : -2.2f, 0, 0);
            sfx_play_name("dd_shoot");
        }
        if (e->sub == F_GULP || e->sub == F_TINMOUSE) {
            for (int j = 0; j < DD_MAX_ENTS; j++) {
                Ent *o = &dd_ent[j];
                if (j == i || !o->alive || o->kind != EK_FOE || !ent_overlap(e, o)) continue;
                if (e->sub == F_GULP && (o->sub == F_ANT || o->sub == F_LANCER)) { dd_kill_foe(j); e->st++; }
                if (e->sub == F_TINMOUSE && o->sub == F_LANCER && e->t % 30 == 0) damage_foe(j, 1, e->x);
            }
        }
        if (e->sub == F_GULP) gulp_swallow(i);
        break;
    }
    }
}

/* ---- creatures with their own ways --------------------------------------- */

/* the wall climber walks, climbs any wall it meets, steps over the top, and
 * climbs back down if a ceiling stops it; Dot can ride it up */
static void climber_update(Ent *e) {
    const float sp = FOE[F_CLIMBER].speed;
    float wx = e->dir ? e->x + e->w + 1 : e->x - 1;
    if (e->state == 0) {
        e->vx = e->dir ? sp : -sp;
        ent_move(e, 0.25f, false);
        bool hit = (e->param2 & 0x10000) != 0;
        e->param2 &= ~0x10000;
        if (hit && e->ground) { e->state = 1; e->vx = 0; }
        else if (e->ground && !ledge_ahead(e)) e->dir ^= 1;
    } else if (e->state == 1) {
        bool wall = (tflags(&dd_lv, TSN, wx, e->y + e->h - 1) & TF_SOLID) != 0;
        float ny = e->y - sp;
        /* a rider's head counts as its ceiling too */
        bool rider_stuck = riding >= 0 && &dd_ent[riding] == e && dd_body_blocked(&dd_lv, TSN, dd_p.x, dd_p.y - sp, dd_p.w, dd_p.h);
        if (!wall) { e->x += e->dir ? 3.0f : -3.0f; e->state = 0; e->vy = 0; }
        else if (rider_stuck || dd_body_blocked(&dd_lv, TSN, e->x, ny, e->w, e->h)) { if (++e->st > 90) { e->state = 2; e->st = 0; } }
        else { e->y = ny; e->st = 0; }
    } else {
        float ny = e->y + sp;
        if (dd_body_blocked(&dd_lv, TSN, e->x, ny, e->w, e->h)) { e->state = 0; e->dir ^= 1; }
        else e->y = ny;
    }
}

/* the ferry fly: waits, climbs to the cruising height, crosses, comes down,
 * waits, and flies back. home is one end, param/param2 the other */
static void ferry_update(Ent *e) {
    const float cruise = 12 * DD_TS;
    float ax = e->home_x, ay = e->home_y, bx = (float)e->param, by = (float)e->param2;
    float tx = e->x, ty = e->y;
    switch (e->state) {
    case 0: case 4: if (++e->st > 150) { e->state++; e->st = 0; } return;
    case 1: tx = ax; ty = cruise; break;
    case 2: tx = bx; ty = cruise; break;
    case 3: tx = bx; ty = by; break;
    case 5: tx = bx; ty = cruise; break;
    case 6: tx = ax; ty = cruise; break;
    case 7: tx = ax; ty = ay; break;
    }
    float dx = tx - e->x, dy = ty - e->y;
    e->x += fclamp(dx, -1.0f, 1.0f);
    e->y += fclamp(dy, -1.0f, 1.0f);
    e->dir = dx > 0 ? 1 : dx < 0 ? 0 : e->dir;
    if (fabsf(tx - e->x) < 0.01f && fabsf(ty - e->y) < 0.01f) { e->state = (int16_t)((e->state + 1) % 8); e->st = 0; }
}

/* the pink blob swallows a thing or a small creature it bumps into and spits
 * it out again a second later */
static void gulp_swallow(int i) {
    Ent *e = &dd_ent[i];
    if (e->param) {
        if (++e->fuse < 60) return;
        int kind = e->param / 1000, sub = e->param % 1000 - 1;
        int j = kind == EK_OBJ ? dd_add_obj(sub, e->x + (e->dir ? e->w : -8), e->y - 2) : dd_add_ent(EK_FOE, sub, e->x + (e->dir ? e->w : -8), e->y - 4);
        if (j >= 0) {
            Ent *o = &dd_ent[j];
            o->param = e->home_x;
            o->vx = e->dir ? 2.5f : -2.5f;
            o->vy = -2.5f;
            if (kind == EK_OBJ) { o->state = 2; o->st = 1; }
            else o->stun = 90;
        }
        e->param = 0;
        e->fuse = 0;
        sfx_play_name("dd_pop");
        return;
    }
    for (int j = 0; j < DD_MAX_ENTS; j++) {
        Ent *o = &dd_ent[j];
        if (j == i || !o->alive || o->held || !ent_overlap(e, o)) continue;
        bool obj = o->kind == EK_OBJ && o->state == 0 && !o->pid && o->sub != O_BIGBANG;
        bool small = o->kind == EK_FOE && !(FOE[o->sub].flags & (FF_BOSS | FF_NOLIFT | FF_SAFE)) && o->sub != F_GULP &&
                     o->sub != F_ANT && o->sub != F_LANCER && o->w <= 10;
        if (!obj && !small) continue;
        e->param = o->kind * 1000 + o->sub + 1;
        e->home_x = (int16_t)o->param;
        e->fuse = 0;
        o->alive = 0;
        if (dd_carry == j) dd_carry = -1;
        sfx_play_name("dd_gulp");
        return;
    }
}

/* ---- bosses ------------------------------------------------------------ */

static void boss_update(int i) {
    Ent *e = &dd_ent[i];
    switch (e->sub) {
    case F_ROTIFER:
        /* rolls to and fro; stomped, it gapes and shows its tongue */
        if (e->state == 1) {
            e->vx = 0;
            if (--e->st <= 0) e->state = 0;
        } else {
            e->vx = e->dir ? 0.6f : -0.6f;
            if (e->t % 150 == 0) e->dir = px_c() > e->x + e->w / 2;
        }
        ent_move(e, 0.25f, false);
        if (e->param2 & 0x10000) e->dir ^= 1;
        e->param2 &= ~0x10000;
        break;
    case F_EARWIG:
        /* charges from wall to wall, pausing to snap */
        if (e->cmd) { e->vx = 0; if (++e->st > 180) { e->cmd = 0; e->st = 0; } ent_move(e, 0.25f, false); break; }
        if (e->state == 0) {
            e->vx = 0;
            if (++e->st > 70) { e->state = 1; e->st = 0; e->dir = px_c() > e->x + e->w / 2; }
        } else {
            e->vx = e->dir ? 1.8f : -1.8f;
            ent_move(e, 0.25f, false);
            if (e->param2 & 0x10000) { e->state = 0; e->st = 0; shake_t = 6; sfx_play_name("dd_thud"); }
            e->param2 &= ~0x10000;
            break;
        }
        ent_move(e, 0.25f, false);
        break;
    case F_SPROCKET:
        /* the clockwork knight: stomps, and throws his head */
        if (e->state == 0) {
            e->vx = e->dir ? 0.5f : -0.5f;
            if (e->t % 60 == 0) e->dir = px_c() > e->x + 16;
            ent_move(e, 0.25f, false);
            e->param2 &= ~0x10000;
            if (++e->st > 200) { e->state = 1; e->st = 0; sfx_play_name("dd_boom"); }
            if (e->t % 140 == 70) {
                foe_shoot(e, 2.0f, 0, 4);
                foe_shoot(e, -2.0f, 0, 4);
                shake_t = 6;
            }
        } else {
            /* headless: his shoulders are a ledge, the head sails round */
            e->vx = 0;
            ent_move(e, 0.25f, false);
            if (++e->st > 300) { e->state = 0; e->st = 0; }
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* things you carry                                                       */

static bool obj_fragile(int sub) {
    switch (sub) {
    case O_PEBBLE: case O_HEARTBOX: case O_GIFTBOX: case O_DART: case O_VENOM: case O_CRACKER: case O_DRINK: case O_BIGBANG:
        return true;
    }
    return false;
}

static int obj_damage(int sub) {
    int p = dd_pep();
    switch (sub) {
    case O_AXE: return 3 + p;
    case O_CRACKER: return 2 + p;
    case O_DART: return p + 1;
    case O_HARD: return p + 1;
    case O_CINDER: return p + 2;
    default: return p;
    }
}

static void explode(float cx, float cy) {
    sfx_play_name("dd_boom");
    shake_t = 10;
    dd_burst(cx, cy, C_ORANGE, 16);
    dd_burst(cx, cy, C_YELLOW, 10);
    for (int j = 0; j < DD_MAX_ENTS; j++) {
        Ent *o = &dd_ent[j];
        if (!o->alive || o->kind != EK_FOE) continue;
        if (fabsf(o->x + o->w / 2 - cx) < 22 + o->w / 2 && fabsf(o->y + o->h / 2 - cy) < 22 + o->h / 2) damage_foe(j, 2 + dd_pep(), cx);
    }
    if (fabsf(px_c() - cx) < 18 && fabsf(py_c() - cy) < 18) dd_hurt(2, cx);
    int tx = (int)(cx / DD_TS), ty = (int)(cy / DD_TS);
    for (int y = ty - 2; y <= ty + 2; y++)
        for (int x = tx - 2; x <= tx + 2; x++)
            if (DD_TILE[lv_tile(&dd_lv, x, y)].flags & TF_BOMB) { lv_set(&dd_lv, x, y, T_AIR); dd_burst(x * 8 + 4.0f, y * 8 + 4.0f, C_BROWN, 4); }
}

/* the Big Bang goes off: every creature on the screen, every weak block,
 * and a good knock for Dot too. Another waits at the end of the SIEGE box */
static void big_bang(float cx, float cy) {
    sfx_play_name("dd_boom");
    shake_t = 40;
    dd_flash_t = 20;
    for (int k = 0; k < 4; k++) dd_burst(cx, cy, k % 2 ? C_YELLOW : C_ORANGE, 16);
    float vx0 = dd_cam_x, vy0 = dd_cam_y;
    for (int j = 0; j < DD_MAX_ENTS; j++) {
        Ent *o = &dd_ent[j];
        if (!o->alive || o->x + o->w < vx0 || o->x > vx0 + SCREEN_W || o->y + o->h < vy0 || o->y > vy0 + SCREEN_H) continue;
        if (o->kind == EK_FOE && !(FOE[o->sub].flags & FF_SAFE)) {
            if (FOE[o->sub].flags & FF_BOSS) { if (o->sub != F_SPROCKET) { o->hurt_t = 0; damage_foe(j, 30, cx); } }
            else dd_kill_foe(j);
        } else if (o->kind == EK_OBJ && !o->held && (o->sub == O_PEBBLE || o->sub == O_CINDER)) {
            o->alive = 0;
            dd_burst(o->x + 4, o->y + 4, C_TAN, 6);
        }
    }
    int x0 = (int)(vx0 / DD_TS), y0 = (int)(vy0 / DD_TS);
    for (int y = y0; y <= y0 + SCREEN_H / DD_TS + 1; y++)
        for (int x = x0; x <= x0 + SCREEN_W / DD_TS + 1; x++)
            if (DD_TILE[lv_tile(&dd_lv, x, y)].flags & TF_BOMB) lv_set(&dd_lv, x, y, T_AIR);
    dd_hurt_t = 0;
    dd_hurt(4, cx);
}

static void obj_break(int i) {
    Ent *e = &dd_ent[i];
    float cx = e->x + e->w / 2, cy = e->y + e->h / 2;
    e->alive = 0;
    switch (e->sub) {
    case O_HEARTBOX: {
        float gy = cy;
        while (gy < dd_lv.h * DD_TS && !(tflags(&dd_lv, DD_TS, cx, gy + 4) & (TF_SOLID | TF_ONEWAY))) gy += 1;
        dd_add_pick(P_HEART, cx - 3, gy - 3, 0);
        break;
    }
    case O_GIFTBOX: {
        /* the prize settles on the ground below where the box broke: the
         * next level of what the spot holds, or glints once that is full */
        float gy = cy;
        while (gy < dd_lv.h * DD_TS && !(tflags(&dd_lv, DD_TS, cx, gy + 4) & (TF_SOLID | TF_ONEWAY))) gy += 1;
        int r = dd_resolve_upgrade(e->param);
        int j = r >= 0 ? dd_add_pick(P_UPGRADE, cx - 3, gy - 3, r) : dd_add_pick(P_GLINT5, cx - 3, gy - 3, 0);
        if (j >= 0) dd_ent[j].pid = e->pid;
        break;
    }
    case O_CRACKER: explode(cx, cy); return;
    case O_BIGBANG: big_bang(cx, cy); return;
    default: break;
    }
    dd_burst(cx, cy, C_TAN, 8);
    sfx_play_name("dd_break");
}

/* an hourglass stops everything for three seconds; it can't be lifted
 * again until the time runs out, and it cracks after three turns */
static void hourglass_turn(int i) {
    Ent *e = &dd_ent[i];
    freeze_all_t = 180;
    sfx_play_name("dd_freeze");
    e->state = 0;
    e->vx = 0;
    if (++e->fuse >= 3) { e->alive = 0; dd_burst(e->x + 4, e->y + 4, C_ICE, 10); sfx_play_name("dd_break"); }
}

/* what a thrown thing does when it lands or hits */
static void obj_land(int i, bool on_ground) {
    Ent *e = &dd_ent[i];
    float cx = e->x + e->w / 2;
    switch (e->sub) {
    case O_SPORE:
        if (on_ground) {
            int tx = (int)(cx / DD_TS), ty = (int)((e->y + e->h) / DD_TS) - 1;
            if (lv_tile(&dd_lv, tx, ty) == T_AIR) lv_set(&dd_lv, tx, ty, T_SHROOM);
            e->alive = 0;
            dd_burst(cx, e->y, C_PINK, 8);
            sfx_play_name("dd_pop");
        }
        return;
    case O_PUPA:
        e->alive = 0;
        { int j = dd_add_ent(EK_FOE, F_FLUTTER, e->x, e->y - 4); if (j >= 0) { dd_ent[j].home_y = (int16_t)(e->y - 10); dd_ent[j].home_x = (int16_t)e->x; } }
        dd_burst(cx, e->y, C_YELLOW, 8);
        return;
    case O_MOLDFRUIT:
        if (on_ground) { e->sub = O_SEED; dd_burst(cx, e->y, C_LEAF, 8); sfx_play_name("dd_pop"); e->state = 0; }
        return;
    case O_QUAKE:
        if (on_ground) {
            shake_t = 20;
            sfx_play_name("dd_thud");
            for (int j = 0; j < DD_MAX_ENTS; j++) {
                Ent *o = &dd_ent[j];
                if (o->alive && o->kind == EK_FOE && o->ground && !(FOE[o->sub].flags & FF_BOSS)) { o->stun = 180; o->flip = 1; o->vy = -2; }
                if (o->alive && o->kind == EK_DOOR && dd_door_is(o->param, "owl") && fabsf(o->x - e->x) < 88 && fabsf(o->y - e->y) < 72 && !dd_has(U_WINGS)) {
                    dd_set(FL_WINGS_DONE);
                    dd_give_upgrade(U_WINGS);
                    dd_say("THE STONE OWL", "THE OLD OWL SHAKES OFF A CENTURY OF DUST. SOMETHING LIGHT AND FEATHERY FALLS AT DASH'S PAWS: KITE WINGS!");
                }
            }
            dd_burst(cx, e->y + e->h, C_GREY, 12);
            /* it cracks a little every time; the third quake breaks it */
            if (++e->fuse >= 3) { e->alive = 0; sfx_play_name("dd_break"); }
            else e->state = 0;
        }
        return;
    case O_HOURGLASS:
        hourglass_turn(i);
        return;
    case O_GLUE:
        {
            int tx = (int)(cx / DD_TS), ty = (int)((e->y + e->h / 2) / DD_TS);
            if (lv_tile(&dd_lv, tx, ty) == T_AIR) lv_set(&dd_lv, tx, ty, T_GLUE);
            e->alive = 0;
            sfx_play_name("dd_splat");
        }
        return;
    case O_ROLLER:
        if (on_ground) { e->state = 3; e->vx = e->dir ? 2.5f : -2.5f; }
        return;
    }
    if (obj_fragile(e->sub) && e->sub != O_CRACKER && e->sub != O_DRINK) {
        if (e->sub == O_HEARTBOX || e->sub == O_GIFTBOX || !on_ground) { obj_break(i); return; }
    }
    if (e->sub == O_CRACKER || e->sub == O_BIGBANG) { obj_break(i); return; }
    if (e->sub == O_DRINK) { e->alive = 0; dd_burst(cx, e->y, C_LIME, 8); return; }
    if (e->sub == O_CINDER && on_ground) {
        /* a cinder block lands with a thud and crumbles on the third */
        shake_t = 4;
        sfx_play_name("dd_thud");
        if (++e->fuse >= 3) { obj_break(i); return; }
    }
    e->state = 0;
}

static void obj_hit_foe(int i, int j) {
    Ent *e = &dd_ent[i], *o = &dd_ent[j];
    if (e->sub == O_VENOM || e->sub == O_DRINK) {
        if (!(o->sub == F_SPROCKET)) o->poison = 1;
        damage_foe(j, 1, e->x);
        e->alive = 0;
        dd_burst(e->x, e->y, C_LIME, 10);
        sfx_play_name("dd_splat");
        return;
    }
    int dmg = obj_damage(e->sub);
    if (o->sub == F_EARWIG && (e->sub == O_PEBBLE || e->sub == O_BABY)) dmg = 1; /* plain blocks barely hurt it */
    if (e->sub == O_BABY || e->sub == O_EGG || e->sub == O_REDEGG || e->sub == O_SPECS || e->sub == O_LETTER) dmg = 0;
    if (dmg > 0) damage_foe(j, dmg, e->x);
    if (e->sub == O_BOOMER || e->sub == O_AXE) { e->state = 4; return; }
    if (e->sub == O_HOURGLASS) { hourglass_turn(i); return; }
    if (e->sub == O_ROLLER) return;
    if (e->sub == O_CRACKER || e->sub == O_BIGBANG) { obj_break(i); return; }
    if (obj_fragile(e->sub)) { obj_break(i); return; }
    e->vx = -e->vx * 0.3f;
    e->state = 0;
}

static void obj_update(int i) {
    Ent *e = &dd_ent[i];
    e->t++;
    if (e->held) return;
    switch (e->state) {
    case 0: { /* resting: on the tiles, or stacked on another thing */
        float ob = e->y + e->h;
        ent_move(e, 0.25f, false);
        e->param2 &= ~0x10000;
        if (!e->ground && e->vy >= 0)
            for (int j = 0; j < DD_MAX_ENTS; j++) {
                Ent *o = &dd_ent[j];
                if (j == i || !o->alive || o->kind != EK_OBJ || o->state != 0 || o->held) continue;
                if (e->x + e->w <= o->x + 1 || e->x >= o->x + o->w - 1) continue;
                if (ob <= o->y + 1 && e->y + e->h >= o->y) { e->y = o->y - e->h; e->vy = 0; e->ground = 1; break; }
            }
        if (e->ground) e->vx *= 0.7f;
        break;
    }
    case 2: { /* flying */
        float g = e->sub == O_DART || e->param == -2 ? 0.0f : e->st == 1 ? 0.25f : 0.1f;
        if (e->param == -2 && e->t > 45) { e->alive = 0; return; }
        ent_move(e, g, false);
        bool wall = (e->param2 & 0x10000) != 0;
        e->param2 &= ~0x10000;
        for (int j = 0; j < DD_MAX_ENTS; j++) {
            Ent *o = &dd_ent[j];
            if (!o->alive || o->kind != EK_FOE || o->held || o->state == 9 || !ent_overlap(e, o)) continue;
            obj_hit_foe(i, j);
            if (!e->alive || e->state != 2) return;
        }
        if (wall) { if (e->sub == O_BOOMER || e->sub == O_AXE) e->state = 4; else obj_land(i, false); }
        else if (e->ground) obj_land(i, true);
        break;
    }
    case 3: /* rolling */
        e->vx = e->dir ? 2.5f : -2.5f;
        ent_move(e, 0.25f, false);
        for (int j = 0; j < DD_MAX_ENTS; j++)
            if (dd_ent[j].alive && dd_ent[j].kind == EK_FOE && ent_overlap(e, &dd_ent[j])) damage_foe(j, dd_pep(), e->x);
        if (e->param2 & 0x10000) { e->state = 0; e->vx = 0; }
        e->param2 &= ~0x10000;
        break;
    case 4: { /* a boomerang block coming home */
        float dx = px_c() - (e->x + 4), dy = dd_p.y - e->y, d = sqrtf(dx * dx + dy * dy) + 0.01f;
        e->x += dx / d * 3.0f;
        e->y += dy / d * 3.0f;
        for (int j = 0; j < DD_MAX_ENTS; j++)
            if (dd_ent[j].alive && dd_ent[j].kind == EK_FOE && ent_overlap(e, &dd_ent[j])) damage_foe(j, obj_damage(e->sub), e->x);
        if (d < 10 && dd_carry < 0 && dd_scale != SC_FULL) { e->held = 1; e->state = 1; dd_carry = i; sfx_play_name("dd_lift"); }
        else if (d < 10) { e->state = 0; }
        break;
    }
    case 5: /* a boomerang block going out */
        e->x += e->dir ? 3.0f : -3.0f;
        if (++e->st > 22 || dd_body_blocked(&dd_lv, DD_TS, e->x, e->y, e->w, e->h)) e->state = 4;
        for (int j = 0; j < DD_MAX_ENTS; j++)
            if (dd_ent[j].alive && dd_ent[j].kind == EK_FOE && ent_overlap(e, &dd_ent[j])) { damage_foe(j, obj_damage(e->sub), e->x); e->state = 4; }
        break;
    }
    /* fill a jar at a drip, a pool of slime or acid */
    if (e->sub == O_JAR && e->state == 0) {
        int tx = (int)((e->x + 4) / DD_TS), ty = (int)((e->y + e->h - 1) / DD_TS);
        int t = lv_tile(&dd_lv, tx, ty);
        if (t == T_SLIME) { e->sub = O_JARSLIME; sfx_play_name("dd_fill"); }
        else if (t == T_GOO) { e->sub = O_JARACID; sfx_play_name("dd_fill"); }
    }
}

/* Dash: the dog ambles about on his own, very slowly (UP while standing on
 * him stops him, or sets him off again). He can be carried, thrown and
 * ridden, and once his fleas are gone his nose stops him over any spot of
 * the room worth shrinking into */
static int sniff_skip_x = -1, sniff_skip_y = -1;
static bool dd_carry_is_dash(void) { return dd_carry >= 0 && dd_ent[dd_carry].kind == EK_DASH; }
void dd_dash_go(bool go) {
    if (dd_dash < 0 || !dd_ent[dd_dash].alive) return;
    Ent *e = &dd_ent[dd_dash];
    if (go) {
        if (e->state == 7) { sniff_skip_x = e->home_x; sniff_skip_y = e->home_y; }
        e->state = 0;
    } else e->state = 6;
}
static void dash_update(int i) {
    Ent *e = &dd_ent[i];
    e->t++;
    if (e->held) return;
    bool wings = dd_has(U_WINGS);
    if (e->state == 9 && wings) {
        /* with wings, thrown Dash is a boomerang: out, and home again,
         * biting every creature on the way and none the worse for it */
        e->x += e->dir ? 3.0f : -3.0f;
        if (++e->st > 24 || dd_body_blocked(&dd_lv, TSN, e->x, e->y, e->w, e->h)) { e->state = 4; e->st = 0; }
        for (int j = 0; j < DD_MAX_ENTS; j++)
            if (dd_ent[j].alive && dd_ent[j].kind == EK_FOE && ent_overlap(e, &dd_ent[j])) damage_foe(j, 2 + dd_pep(), e->x);
        return;
    }
    if (e->state == 9) { /* thrown */
        ent_move(e, e->st == 1 ? 0.25f : 0.1f, false);
        bool hit = (e->param2 & 0x10000) || e->ground;
        e->param2 &= ~0x10000;
        for (int j = 0; j < DD_MAX_ENTS; j++)
            if (dd_ent[j].alive && dd_ent[j].kind == EK_FOE && ent_overlap(e, &dd_ent[j])) {
                damage_foe(j, 2 + dd_pep(), e->x);
                hit = true;
                if (!dd_has(U_PLATE)) {
                    /* a yelp, and off he runs, back to his own size: he won't
                     * come back until Dot is full size too, or whistles */
                    e->alive = 0;
                    dd_dash = -1;
                    dd_sv.dash_away = 1;
                    dd_burst(e->x, e->y, C_WHITE, 8);
                    dd_popup(e->x, e->y - 8, "YIP!", C_WHITE);
                    sfx_play_name("dd_yelp");
                    return;
                }
                break;
            }
        if (hit) { e->state = 0; e->vx = 0; }
        return;
    }
    if (e->state == 4) { /* with wings he flies home to Dot */
        float dx = px_c() - e->x, dy = dd_p.y - e->y, d = sqrtf(dx * dx + dy * dy) + 0.01f;
        e->x += dx / d * 3.0f;
        e->y += dy / d * 3.0f;
        for (int j = 0; j < DD_MAX_ENTS; j++)
            if (dd_ent[j].alive && dd_ent[j].kind == EK_FOE && ent_overlap(e, &dd_ent[j])) damage_foe(j, 2 + dd_pep(), e->x);
        if (d < 12) { e->state = 0; e->vy = 0; e->pad = 1; }
        return;
    }
    dd_sniff_x = dd_sniff_y = -1;
    if (e->state == 8) {
        /* sitting by Dot until she moves off */
        e->vx = 0;
        if (!(wings && e->pad)) ent_move(e, 0.25f, false);
        e->param2 &= ~0x10000;
        if (!player_overlap(e) || fabsf(dd_p.vx) > 0.3f) e->state = 0;
        return;
    }
    if (e->state == 6 || e->state == 7) {
        /* told to stay, or pointing with his nose */
        e->vx = 0;
        if (!(wings && e->pad)) ent_move(e, 0.25f, false);
        e->param2 &= ~0x10000;
        if (e->state == 7) {
            if (dd_lv.d.kind != LV_AREA || !dd_sniff_spot(dd_lv.d.id, e->home_y, e->home_x)) e->state = 0;
            else { dd_sniff_x = e->home_x * DD_TS + 4; dd_sniff_y = e->home_y * DD_TS; }
        }
        return;
    }
    /* Dot standing still beside him for a moment: he sits and looks up at
     * her, ready to be talked to */
    if (player_overlap(e) && fabsf(dd_p.vx) < 0.05f && dd_p.ground && !dd_carry_is_dash()) {
        if (++e->st > 45) { e->state = 8; e->st = 0; }
    } else e->st = 0;
    const float sp = dd_scale == SC_FULL ? 0.3f : 0.35f;
    e->vx = e->dir ? sp : -sp;
    if (wings) {
        /* with wings he walks straight off ledges and on through the air,
         * keeping his height, until a wall turns him */
        if (e->ground) e->pad = 1;
        if (e->pad) {
            float nx = e->x + e->vx;
            if (dd_body_blocked(&dd_lv, TSN, nx, e->y, e->w, e->h)) e->dir ^= 1;
            else e->x = nx;
            e->ground = (tflags(&dd_lv, TSN, e->x + 1, e->y + e->h + 1) | tflags(&dd_lv, TSN, e->x + e->w - 1, e->y + e->h + 1)) & (TF_SOLID | TF_ONEWAY) ? 1 : 0;
            e->param2 &= ~0x10000;
            goto nose;
        }
    }
    if (e->ground && !ledge_ahead(e)) { e->dir ^= 1; e->vx = -e->vx; }
    ent_move(e, 0.25f, false);
    if (e->param2 & 0x10000) e->dir ^= 1;
    e->param2 &= ~0x10000;
    if ((e->t + i * 37) % 480 == 0) e->dir ^= 1; /* now and then he turns round */
nose:
    /* his nose, at small size in the room and between the walls */
    if (dd_scale == SC_SMALL && dd_flag(FL_DASH_CLEAN) && e->ground && dd_lv.d.kind == LV_AREA &&
        (dd_lv.d.id == AR_ROOM || dd_lv.d.id == AR_CAVITY)) {
        int tx = (int)((e->x + e->w / 2) / DD_TS), row = (int)((e->y + e->h + 1) / DD_TS);
        if (sniff_skip_x >= 0 && (iabs(tx - sniff_skip_x) > 2 || row != sniff_skip_y)) sniff_skip_x = sniff_skip_y = -1;
        if (!(tx == sniff_skip_x && row == sniff_skip_y) && dd_sniff_spot(dd_lv.d.id, row, tx)) {
            e->state = 7;
            e->home_x = (int16_t)tx;
            e->home_y = (int16_t)row;
            e->x = (float)(tx * DD_TS + 4) - e->w / 2;
            sfx_play_name("dd_sniff");
        }
    }
}

static void spawn_dash_near(void) {
    if (dd_flag(FL_PAID_QUEEN) && !dd_flag(FL_SPROCKET_DEAD)) { dd_dash = -1; return; }
    bool full = dd_scale == SC_FULL;
    int dw = full ? 14 : 12, dh = full ? 10 : 7;
    int i = dd_add_ent(EK_DASH, 0, dd_p.x + (dd_p.facing ? -dw : dd_p.w + 2), dd_p.y + dd_p.h - dh);
    if (i < 0) return;
    Ent *e = &dd_ent[i];
    e->w = (int16_t)dw;
    e->h = (int16_t)dh;
    if (dd_body_blocked(&dd_lv, TSN, e->x, e->y, e->w, e->h)) e->x = dd_p.x + dd_p.w / 2 - dw / 2;
    dd_dash = i;
    dd_sv.dash_away = 0;
}

static void call_dash(void) {
    if (dd_flag(FL_PAID_QUEEN) && !dd_flag(FL_SPROCKET_DEAD)) { flash_message("DASH IS GONE. THE QUEEN HAS HIM!"); return; }
    if (dd_dash >= 0 && dd_ent[dd_dash].alive) {
        Ent *e = &dd_ent[dd_dash];
        if (e->held) return;
        e->x = dd_p.x - 3;
        e->y = dd_p.y + dd_p.h - e->h;
        e->state = 0;
    } else spawn_dash_near();
    sfx_play_name("dd_whistle");
    dd_popup(dd_p.x, dd_p.y - 10, "WHEEET!", C_ICE);
}

/* ------------------------------------------------------------------ */
/* picking up, throwing, kicking, the satchel                            */

static bool liftable(int j) {
    Ent *o = &dd_ent[j];
    if (!o->alive || o->held) return false;
    if (o->kind == EK_OBJ) return o->state == 0 && !(o->sub == O_HOURGLASS && freeze_all_t > 0);
    if (o->kind == EK_DASH) return o->state == 0 || o->state == 6 || o->state == 7 || o->state == 8;
    if (o->kind == EK_FOE) return dd_scale != SC_FULL && dd_has(U_MITT1) && !(FOE[o->sub].flags & (FF_BOSS | FF_NOLIFT));
    return false;
}

static int thing_underfoot(void) {
    if (riding >= 0 && liftable(riding)) return riding;
    float fx = dd_p.x, fw = dd_p.w, fy = dd_p.y + dd_p.h;
    for (int j = 0; j < DD_MAX_ENTS; j++) {
        Ent *o = &dd_ent[j];
        if (!liftable(j)) continue;
        if (fx + fw > o->x && fx < o->x + o->w && fabsf(fy - o->y) < 3) return j;
    }
    /* or one Dot is standing in */
    for (int j = 0; j < DD_MAX_ENTS; j++)
        if (liftable(j) && dd_ent[j].kind != EK_FOE && player_overlap(&dd_ent[j])) return j;
    return -1;
}

static void try_pickup(void) {
    int j = thing_underfoot();
    if (j < 0) return;
    lift_target = j;
    lift_t = dd_has(U_MITT2) ? 4 : LIFT_T;
    if (dd_ent[j].kind == EK_FOE) dd_ent[j].stun = 250;
}

static void finish_pickup(void) {
    int j = lift_target;
    lift_target = -1;
    if (j < 0 || !liftable(j)) return;
    Ent *o = &dd_ent[j];
    if (o->kind == EK_FOE && o->sub == F_SCAMPER) {
        /* the living glint, caught at last */
        o->alive = 0;
        dd_set(FL_SCAMPER);
        dd_give_glints(25);
        dd_popup(dd_p.x, dd_p.y - 12, "+25", C_YELLOW);
        dd_say("THE LIVING GLINT", "SQUEAK! ALL RIGHT, ALL RIGHT, YOU CAUGHT ME. HAVE A PINCH OF MY SPARKLE AND LET ME GO.");
        sfx_play_name("dd_big");
        return;
    }
    /* holding a thing overhead makes Dot taller: no room, no lifting */
    if (dd_body_blocked(&dd_lv, TSN, dd_p.x, dd_p.y - o->h, dd_p.w, (float)o->h)) {
        flash_message("NO ROOM OVERHEAD");
        sfx_play_name("dd_block");
        return;
    }
    o->held = 1;
    o->cmd = 0;
    if (riding == j) riding = -1;
    dd_carry = j;
    sfx_play_name("dd_lift");
}

static void place_held(void) {
    if (dd_carry < 0) return;
    Ent *o = &dd_ent[dd_carry];
    o->x = dd_p.x + dd_p.w / 2 - o->w / 2;
    o->y = dd_p.y - o->h;
    o->vx = o->vy = 0;
    if (!o->alive) dd_carry = -1;
}

static void release_held(uint32_t in) {
    Ent *o = &dd_ent[dd_carry];
    int i = dd_carry;
    dd_carry = -1;
    o->held = 0;
    bool up = in & BTN_UP, down = in & BTN_DOWN;
    int face = dd_p.facing;
    if (down) {
        /* place it under Dot's feet: she steps up onto it (a way to build
         * steps); with no room above her, it goes down in front instead */
        float nx = dd_p.x + dd_p.w / 2 - o->w / 2, ny = dd_p.y + dd_p.h - o->h;
        if (dd_p.ground && !dd_body_blocked(&dd_lv, TSN, dd_p.x, dd_p.y - o->h, dd_p.w, dd_p.h)) {
            o->x = nx;
            o->y = ny;
            dd_p.y -= (float)o->h;
            dd_p.vy = 0;
            riding = i;
        } else {
            o->x = face ? dd_p.x + dd_p.w : dd_p.x - o->w;
            o->y = dd_p.y + dd_p.h - o->h;
            if (dd_body_blocked(&dd_lv, TSN, o->x, o->y, o->w, o->h)) o->x = nx, o->y = dd_p.y - o->h;
        }
        o->state = 0;
        o->vx = o->vy = 0;
        if (o->kind == EK_FOE) o->stun = 30;
        if (o->kind == EK_DASH) o->pad = (uint8_t)dd_has(U_WINGS); /* with wings he holds his height */
        sfx_play_name("dd_drop");
        return;
    }
    /* UP hurls it in a high arc; otherwise a low, flat throw */
    o->vx = up ? (face ? 2.2f : -2.2f) + dd_p.vx * 0.3f : (face ? 4.0f : -4.0f) + dd_p.vx * 0.4f;
    o->vy = up ? -5.5f : -0.5f;
    if (!up) {
        /* a throw leaves from chest height, low and flat, so it finds what's on the ground */
        float nx = face ? dd_p.x + dd_p.w - 2 : dd_p.x - o->w + 2, ny = dd_p.y + dd_p.h - o->h - 3;
        if (!dd_body_blocked(&dd_lv, TSN, nx, ny, o->w, o->h)) { o->x = nx; o->y = ny; }
    }
    if (o->kind == EK_OBJ && o->sub == O_CINDER) { o->vx *= 0.45f; if (up) o->vy = -3.0f; } /* heavy */
    if (dd_scale == SC_FULL) { o->vx *= 0.5f; if (up) o->vy = -3.5f; }
    o->dir = (uint8_t)face;
    o->st = up ? 1 : 0;
    if (o->kind == EK_OBJ && o->sub == O_DART && !up) o->vy = 0; /* a dart flies dead straight */
    o->state = o->kind == EK_OBJ ? 2 : 9;
    if (o->kind == EK_OBJ && (o->sub == O_BOOMER || o->sub == O_AXE) && !up) { o->state = 5; o->st = 0; o->vy = 0; }
    if (o->kind == EK_DASH && dd_has(U_WINGS) && !up) { o->st = 0; o->vy = 0; }
    if (o->kind == EK_FOE) o->stun = 0;
    throw_t = 10;
    sfx_play_name("dd_throw");
    (void)i;
}

static void fire_popgun(void) {
    int j = dd_add_ent(EK_OBJ, O_PEBBLE, dd_p.x + (dd_p.facing ? dd_p.w : -4), dd_p.y + 5);
    if (j >= 0) {
        Ent *o = &dd_ent[j];
        o->w = o->h = 4;
        o->vx = dd_p.facing ? 5.0f : -5.0f;
        o->vy = 0;
        o->state = 2;
        o->param = -2; /* a pea: it vanishes on landing */
    }
    sfx_play_name("dd_shoot");
}

static void kick(void) {
    kick_t = 12;
    sfx_play_name("dd_kick");
    float kx = dd_p.facing ? dd_p.x + dd_p.w : dd_p.x - 10;
    for (int j = 0; j < DD_MAX_ENTS; j++) {
        Ent *o = &dd_ent[j];
        if (!o->alive || o->held) continue;
        if (!rects_overlap((int)kx, (int)dd_p.y, 10, dd_p.h, (int)o->x, (int)o->y, o->w, o->h)) continue;
        if (o->kind == EK_OBJ && o->state == 0) {
            o->vx = dd_p.facing ? 4.5f : -4.5f;
            o->vy = -1.0f;
            o->dir = dd_p.facing;
            o->state = o->sub == O_ROLLER ? 3 : 2;
        } else if (o->kind == EK_FOE && !(FOE[o->sub].flags & FF_BOSS)) {
            o->vx = dd_p.facing ? 3.0f : -3.0f;
            o->vy = -2.0f;
            o->stun = 40;
            if (dd_has(U_CLOGS2)) damage_foe(j, dd_pep(), dd_p.x);
        } else if (o->kind == EK_FOE && dd_has(U_CLOGS2)) {
            damage_foe(j, dd_pep(), dd_p.x);
        }
    }
}

static void satchel_use(void) {
    int cap = dd_has(U_SATCHEL2) ? 2 : 1;
    if (dd_carry >= 0 && dd_ent[dd_carry].kind == EK_OBJ) {
        for (int k = 0; k < cap; k++)
            if (!dd_sv.satchel[k]) {
                dd_sv.satchel[k] = (uint8_t)(dd_ent[dd_carry].sub + 1);
                dd_sv.satchel_param[k] = (int16_t)dd_ent[dd_carry].param;
                dd_ent[dd_carry].alive = 0;
                dd_carry = -1;
                sfx_play_name("dd_stow");
                dd_autosave();
                return;
            }
        /* full: the thing in hand and the first stored one change places */
        uint8_t k0 = dd_sv.satchel[0];
        int16_t p0 = dd_sv.satchel_param[0];
        dd_sv.satchel[0] = (uint8_t)(dd_ent[dd_carry].sub + 1);
        dd_sv.satchel_param[0] = (int16_t)dd_ent[dd_carry].param;
        if (cap == 2) { dd_sv.satchel[0] = dd_sv.satchel[1]; dd_sv.satchel_param[0] = dd_sv.satchel_param[1];
                        dd_sv.satchel[1] = (uint8_t)(dd_ent[dd_carry].sub + 1); dd_sv.satchel_param[1] = (int16_t)dd_ent[dd_carry].param; }
        dd_ent[dd_carry].sub = (uint8_t)(k0 - 1);
        dd_ent[dd_carry].param = p0;
        dd_ent[dd_carry].fuse = 0;
        sfx_play_name("dd_stow");
        dd_autosave();
        return;
    }
    if (dd_carry < 0) {
        for (int k = cap - 1; k >= 0; k--)
            if (dd_sv.satchel[k]) {
                int j = dd_add_obj(dd_sv.satchel[k] - 1, dd_p.x, dd_p.y - 8);
                if (j >= 0) {
                    dd_ent[j].param = dd_sv.satchel_param[k];
                    dd_ent[j].held = 1;
                    dd_ent[j].state = 1;
                    dd_carry = j;
                }
                dd_sv.satchel[k] = 0;
                sfx_play_name("dd_stow");
                dd_autosave();
                return;
            }
    }
}

/* ------------------------------------------------------------------ */
/* changing size                                                          */

static int sizes_open(void) {
    /* how small Dot can get: small always, micro with the first tonic, and
     * with the second, tinier still inside the micro world */
    return dd_has(U_TONIC2) ? SC_DEEP : dd_has(U_TONIC1) ? SC_MICRO : SC_SMALL;
}

/* where the shrink leads: 0 = nowhere; fills a description.
 * 1 room small, 2 a special place, 3 a micro strip, 4 the tiny body */
static int shrink_target(LevelDesc *d, int *special_of) {
    *special_of = -1;
    int next = dd_scale + 1;
    if (next > sizes_open()) return 0;
    if (dd_scale == SC_FULL) return 1; /* the same room, small */
    if (dd_scale == SC_MICRO) return 4; /* smaller still, in the same place */
    /* standing on a creature or a seam */
    if (riding >= 0 && dd_ent[riding].alive) {
        Ent *o = &dd_ent[riding];
        if (o->kind == EK_DASH) *special_of = SP_DASHFUR;
        if (o->kind == EK_FOE && o->sub == F_SPROCKET && o->state == 1) *special_of = SP_SPROCKET;
        if (o->kind == EK_FOE && o->sub == F_FERRY) *special_of = SP_COCKPIT;
    }
    for (int j = 0; j < DD_MAX_ENTS && *special_of < 0; j++) {
        Ent *o = &dd_ent[j];
        if (!o->alive || !player_overlap(o)) continue;
        if (o->kind == EK_DOOR && dd_door_is(o->param, "keyhole")) *special_of = SP_KEYHOLE;
        if (o->kind == EK_DOOR && dd_door_is(o->param, "summit")) *special_of = SP_SUMMIT;
        if (o->kind == EK_NPC && o->sub == N_PUFFIN) *special_of = SP_PUFFFUR;
        if (o->kind == EK_NPC && o->sub == N_SHREW) *special_of = SP_SHREWHEAD;
    }
    if (*special_of >= 0) {
        memset(d, 0, sizeof *d);
        d->scale = (uint8_t)next;
        d->kind = LV_SPECIAL;
        d->id = (uint8_t)*special_of;
        return 2;
    }
    int tx = (int)((dd_p.x + dd_p.w / 2) / DD_TS), row = (int)((dd_p.y + dd_p.h + 1) / DD_TS);
    int x0, n;
    if (!dd_strip_run(&dd_lv, row, tx, &x0, &n)) return 0;
    memset(d, 0, sizeof *d);
    d->scale = (uint8_t)next;
    d->kind = LV_STRIP;
    d->row = (int16_t)row;
    d->x0 = (int16_t)x0;
    d->n = (int16_t)n;
    d->key = dd_lv.key;
    d->pabs = (int16_t)(dd_lv.d.kind == LV_STRIP ? dd_lv.d.x0 * CHUNK_W : 0);
    return 3;
}

typedef struct Keep { int kind, sub, param, param2; uint32_t pid; } Keep;

/* carried things and Dash come along to the next level */
static Keep kept_carry;
static bool keep_carry, keep_dash;

static void stash_companions(void) {
    keep_carry = false;
    keep_dash = false;
    /* the tin mage, carried out of the mouse's head, is beaten */
    if (dd_carry >= 0 && dd_ent[dd_carry].alive && dd_ent[dd_carry].kind == EK_FOE && dd_ent[dd_carry].sub == F_TINMAGE) {
        dd_on_kill(F_TINMAGE, dd_carry);
        dd_ent[dd_carry].alive = 0;
        dd_carry = -1;
    }
    if (dd_carry >= 0 && dd_ent[dd_carry].alive) {
        Ent *o = &dd_ent[dd_carry];
        kept_carry = (Keep){o->kind, o->sub, o->param, o->param2, o->pid};
        keep_carry = true;
        if (o->kind == EK_DASH) keep_dash = true;
    }
    if (dd_dash >= 0 && dd_ent[dd_dash].alive && !dd_ent[dd_dash].held) {
        Ent *e = &dd_ent[dd_dash];
        if (fabsf(e->x - dd_p.x) < 80 && fabsf(e->y - dd_p.y) < 60) keep_dash = true;
        else dd_sv.dash_away = 1;
    }
}

static void restore_companions(bool arrived_full) {
    dd_carry = -1;
    dd_dash = -1;
    bool full = dd_scale == SC_FULL;
    if (keep_carry) {
        int j = dd_add_ent(kept_carry.kind, kept_carry.sub, dd_p.x, dd_p.y - 8);
        if (j >= 0) {
            Ent *o = &dd_ent[j];
            o->param = kept_carry.param;
            o->param2 = kept_carry.param2;
            o->pid = kept_carry.pid;
            if (o->kind == EK_DASH) { o->w = full ? 14 : 12; o->h = full ? 10 : 7; dd_dash = j; }
            /* things grow and shrink with Dot: at full size a block is big */
            if (o->kind == EK_OBJ && full) o->w = o->h = 16;
            o->held = 1;
            o->state = 1;
            o->y = dd_p.y - o->h;
            dd_carry = j;
        }
    }
    if (dd_dash < 0 && ((keep_dash) || (arrived_full && !(dd_flag(FL_PAID_QUEEN) && !dd_flag(FL_SPROCKET_DEAD)))))
        spawn_dash_near();
    keep_carry = keep_dash = false;
}

static void clear_ents(void) {
    memset(dd_ent, 0, sizeof dd_ent);
    for (int i = 0; i < DD_MAX_PARTS; i++) parts[i].life = 0;
    riding = -1;
    dd_carry = -1;
    dd_dash = -1;
    lift_target = -1;
    lift_t = 0;
}

static void set_body_size(void) {
    if (dd_scale == SC_FULL) { dd_p.w = 8; dd_p.h = 24; }
    else if (dd_scale == SC_DEEP) { dd_p.w = 4; dd_p.h = 6; }
    else { dd_p.w = 6; dd_p.h = 13; }
}

/* find the nearest free spot for Dot's body around (x,y) */
static void settle_body(void) {
    const Phys *ph = dd_phys();
    if (!dd_body_blocked(&dd_lv, ph->ts, dd_p.x, dd_p.y, dd_p.w, dd_p.h)) return;
    for (int r = 1; r < 80; r++)
        for (int k = 0; k < 4; k++) {
            float dx = k == 0 ? 0 : k == 1 ? (float)-r : k == 2 ? (float)r : 0;
            float dy = k == 0 ? (float)-r : k == 3 ? (float)r : 0;
            if (!dd_body_blocked(&dd_lv, ph->ts, dd_p.x + dx, dd_p.y + dy, dd_p.w, dd_p.h)) { dd_p.x += dx; dd_p.y += dy; return; }
        }
}

static void camera_snap(void);

void dd_play_enter_level(int how) {
    (void)how;
    clear_ents();
    set_body_size();
    dd_spawn_level();
    riding = -1;
    dd_p.vx = dd_p.vy = 0;
    dd_p.sprint = 0;
    dd_p.peak_y = (int16_t)dd_p.y;
    dd_shrink_t = dd_grow_t = 0;
    camera_snap();
    int m = dd_music_for_level();
    if (!(dd_flag(FL_LAMP_OFF) && m == MU_TOWN && 0)) music_play(DD_MUS[m]);
}

static void do_shrink(void) {
    LevelDesc d;
    int sp;
    int k = shrink_target(&d, &sp);
    if (!k) return;
    stash_companions();
    if (k == 1) {
        /* full size to small: the same room, every cell a tile */
        float fx = dd_p.x + dd_p.w / 2, fy = dd_p.y + dd_p.h;
        dd_scale = SC_SMALL;
        set_body_size();
        dd_p.x = fx / DD_TS0 * DD_TS - dd_p.w / 2;
        dd_p.y = fy / DD_TS0 * DD_TS - dd_p.h;
        clear_ents();
        dd_spawn_level();
        settle_body();
        restore_companions(false);
        dd_sv.carry = 0;
    } else if (k == 4) {
        /* the second tonic: tinier, in the same place */
        float cx = dd_p.x + dd_p.w / 2, fy = dd_p.y + dd_p.h;
        dd_scale = SC_DEEP;
        set_body_size();
        dd_p.x = cx - dd_p.w / 2;
        dd_p.y = fy - dd_p.h;
    } else {
        float cx = dd_p.x + dd_p.w / 2;
        int tx = (int)(cx / DD_TS);
        float sub = cx - (float)(tx * DD_TS);
        dd_sv.counts[14] = 0;
        /* remember where Dot stood, for specials */
        dd_sv.fx = (int16_t)dd_p.x;
        dd_sv.fy = (int16_t)dd_p.y;
        dd_push_level(&d);
        set_body_size();
        if (k == 3) {
            int c = tx - d.x0;
            dd_p.x = (float)(c * CHUNK_W * DD_TS) + sub * (float)(CHUNK_W * DD_TS) / DD_TS - dd_p.w / 2;
            dd_p.x = fclamp(dd_p.x, (float)(c * CHUNK_W * DD_TS + 8), (float)((c + 1) * CHUNK_W * DD_TS - 16));
            int col = (int)((dd_p.x + dd_p.w / 2) / DD_TS);
            dd_p.y = (float)(dd_lv.surf[col] * DD_TS) - dd_p.h;
        } else {
            extern const int DD_SPECIAL_FLOOR[SP_COUNT];
            dd_p.x = 4 * DD_TS;
            dd_p.y = (float)((DD_SPECIAL_FLOOR[d.id] + 1) * DD_TS) - dd_p.h;
        }
        dd_play_enter_level(1);
        settle_body();
        restore_companions(false);
    }
    dd_p.vx = dd_p.vy = 0;
    camera_snap();
    music_play(DD_MUS[dd_music_for_level()]);
    sfx_play_name("dd_shrink");
    dd_autosave();
}

static bool tiny_can_grow(void) {
    float cx = dd_p.x + dd_p.w / 2, fy = dd_p.y + dd_p.h;
    return !dd_body_blocked(&dd_lv, DD_TS, cx - 3, fy - 13, 6, 13);
}

static void do_grow(void) {
    if (dd_scale == SC_DEEP) {
        /* back to the usual micro size, in the same place */
        float cx = dd_p.x + dd_p.w / 2, fy = dd_p.y + dd_p.h;
        dd_scale = SC_MICRO;
        set_body_size();
        dd_p.x = cx - dd_p.w / 2;
        dd_p.y = fy - dd_p.h;
        settle_body();
        sfx_play_name("dd_grow");
        return;
    }
    stash_companions();
    if (dd_scale == SC_SMALL) {
        /* back to full size: the clock moves on a minute */
        int ax = 0, ay = 0;
        float fx, fy;
        if (dd_lv.d.id != AR_ROOM) {
            ax = DD_AREA[dd_lv.d.id].anchor_x;
            ay = DD_AREA[dd_lv.d.id].anchor_y;
            LevelDesc room = {SC_SMALL, LV_AREA, AR_ROOM, 0, 0, 0, 0, 0};
            dd_replace_level(&room);
            fx = (float)(ax * DD_TS0);
            fy = (float)((ay + 1) * DD_TS0);
        } else {
            fx = (dd_p.x + dd_p.w / 2) / DD_TS * DD_TS0;
            fy = (dd_p.y + dd_p.h) / DD_TS * DD_TS0;
        }
        dd_scale = SC_FULL;
        dd_stack[0].scale = SC_SMALL;
        set_body_size();
        dd_p.x = fx - dd_p.w / 2;
        dd_p.y = fy - dd_p.h;
        clear_ents();
        dd_spawn_level();
        settle_body();
        dd_sv.clock_min++;
        restore_companions(true);
    } else {
        const LevelDesc cur = dd_lv.d;
        float cx = dd_p.x + dd_p.w / 2;
        dd_pop_level();
        set_body_size();
        if (cur.kind == LV_STRIP) {
            int c = iclamp((int)(cx / (CHUNK_W * DD_TS)), 0, cur.n - 1);
            float local = cx - (float)(c * CHUNK_W * DD_TS);
            dd_p.x = (float)((cur.x0 + c) * DD_TS) + local / (float)CHUNK_W - dd_p.w / 2;
            dd_p.y = (float)(cur.row * DD_TS) - dd_p.h;
        } else {
            dd_p.x = dd_sv.fx;
            dd_p.y = dd_sv.fy;
        }
        dd_play_enter_level(2);
        settle_body();
        restore_companions(false);
    }
    dd_p.vx = dd_p.vy = 0;
    camera_snap();
    music_play(DD_MUS[dd_music_for_level()]);
    sfx_play_name("dd_grow");
    dd_autosave();
}

void dd_change_scale(int dir) {
    if (dd_trans) return;
    if (dir > 0) {
        LevelDesc d;
        int sp;
        if (!shrink_target(&d, &sp)) {
            flash_message(dd_scale + 1 > sizes_open() ? "DOT CAN'T GET ANY SMALLER... YET" : "NOTHING TO SHRINK INTO HERE");
            sfx_play_name("dd_block");
            return;
        }
        pending_action = PA_SHRINK;
    } else {
        if (dd_scale == SC_FULL) return;
        if (dd_scale == SC_DEEP && !tiny_can_grow()) {
            flash_message("NO ROOM TO GROW HERE");
            sfx_play_name("dd_block");
            return;
        }
        pending_action = PA_GROW;
    }
    dd_trans = 36;
    dd_trans_dir = dir;
    dd_trans_kind = 0;
}

void dd_goto_area(int area, int tx, int ty) {
    LevelDesc d = {SC_SMALL, LV_AREA, (uint8_t)area, 0, 0, 0, 0, 0};
    stash_companions();
    dd_replace_level(&d);
    set_body_size();
    dd_p.x = (float)(tx * DD_TS + 1);
    dd_p.y = (float)((ty + 1) * DD_TS) - dd_p.h;
    dd_play_enter_level(3);
    settle_body();
    restore_companions(false);
    dd_autosave();
}

static void enter_door(int id) {
    int area, tx, ty;
    if (!dd_door_target(id, &area, &tx, &ty)) return;
    if (dd_door_is(id, "fried") && !dd_flag(FL_POWER_OFF)) {
        dd_hurt(1, dd_p.x + 20);
        flash_message("ZZAP! THE OUTLET IS STILL LIVE");
        return;
    }
    if (dd_door_is(id, "lair") && !dd_flag(FL_WORM_FRIENDS)) {
        dd_say("WOODWORM DOOR", "A ROUND DOOR GNAWED INTO THE STUD, SEALED WITH WOODWORM GUM. ONLY A FRIEND OF THE WORMS COULD OPEN IT.");
        return;
    }
    pa_door = id;
    pending_action = PA_DOOR;
    dd_trans = 30;
    dd_trans_kind = 1;
    sfx_play_name("dd_door");
}

void dd_after_death_room(void) {
    /* forced back to full size where she fell; carried things are lost */
    if (dd_carry >= 0) { dd_ent[dd_carry].alive = 0; dd_carry = -1; }
    while (dd_depth > 0) {
        const LevelDesc cur = dd_lv.d;
        float cx = dd_p.x + dd_p.w / 2;
        dd_pop_level();
        if (cur.kind == LV_STRIP) {
            int c = iclamp((int)(cx / (CHUNK_W * DD_TS)), 0, cur.n - 1);
            dd_p.x = (float)((cur.x0 + c) * DD_TS);
            dd_p.y = (float)(cur.row * DD_TS) - 13;
        } else {
            dd_p.x = dd_sv.fx;
            dd_p.y = dd_sv.fy;
        }
    }
    float fx = dd_p.x + 3, fy = dd_p.y + 13;
    if (dd_lv.d.id != AR_ROOM) {
        fx = (float)(DD_AREA[dd_lv.d.id].anchor_x * DD_TS);
        fy = (float)((DD_AREA[dd_lv.d.id].anchor_y + 1) * DD_TS);
        LevelDesc room = {SC_SMALL, LV_AREA, AR_ROOM, 0, 0, 0, 0, 0};
        dd_replace_level(&room);
    }
    dd_scale = SC_FULL;
    set_body_size();
    dd_p.x = fx / DD_TS * DD_TS0 - dd_p.w / 2;
    dd_p.y = fy / DD_TS * DD_TS0 - dd_p.h;
    clear_ents();
    dd_spawn_level();
    settle_body();
    dd_sv.clock_min++;
    dd_sv.carry = 0;
    dd_hp = dd_hp_max;
    dd_hurt_t = 90;
    keep_dash = false;
    keep_carry = false;
    restore_companions(true);
    camera_snap();
    music_play(DD_MUS[MU_ROOM]);
    dd_autosave();
}

static void transition_update(void) {
    dd_trans--;
    int mid = dd_trans_kind == 1 ? 15 : 18;
    if (dd_trans == mid) {
        int a = pending_action;
        pending_action = PA_NONE;
        if (a == PA_SHRINK) do_shrink();
        else if (a == PA_GROW) do_grow();
        else if (a == PA_DOOR) {
            int area, tx, ty;
            if (dd_door_target(pa_door, &area, &tx, &ty)) dd_goto_area(area, tx, ty);
        } else if (a == PA_DEATH) dd_after_death_room();
    }
}

/* ------------------------------------------------------------------ */
/* the camera                                                             */

static void camera_target(float *tx, float *ty) {
    if (dd_scale == SC_FULL) { *tx = 0; *ty = 0; return; }
    int lw = dd_lv.w * DD_TS, lh = dd_lv.h * DD_TS;
    *tx = fclamp(dd_p.x + dd_p.w / 2 - SCREEN_W / 2 + (dd_p.facing ? 24 : -24), 0, (float)imax(0, lw - SCREEN_W));
    *ty = fclamp(dd_p.y + dd_p.h / 2 - SCREEN_H / 2 - 8, 0, (float)imax(0, lh - SCREEN_H));
    if (lw < SCREEN_W) *tx = (float)(lw - SCREEN_W) / 2;
    if (lh < SCREEN_H) *ty = (float)(lh - SCREEN_H) / 2;
}
static void camera_snap(void) { camera_target(&dd_cam_x, &dd_cam_y); }
static void camera_update(void) {
    float tx, ty;
    camera_target(&tx, &ty);
    dd_cam_x += (tx - dd_cam_x) * 0.12f;
    dd_cam_y += (ty - dd_cam_y) * 0.12f;
    if (fabsf(tx - dd_cam_x) < 0.3f) dd_cam_x = tx;
    if (fabsf(ty - dd_cam_y) < 0.3f) dd_cam_y = ty;
}
int dd_shake(void) { return shake_t > 0 ? (shake_t % 2 ? 1 : -1) : 0; }

/* ------------------------------------------------------------------ */
/* the player each frame                                                  */

static bool pressed(uint32_t m) { return (dd_in & m) && !(dd_in_prev & m); }

static int ent_at_player(int kind) {
    for (int j = 0; j < DD_MAX_ENTS; j++)
        if (dd_ent[j].alive && dd_ent[j].kind == kind && player_overlap(&dd_ent[j])) return j;
    return -1;
}

void dd_dash_talk(void);

static bool interact(void) {
    int j;
    if ((j = ent_at_player(EK_NPC)) >= 0) { dd_talk(j); return true; }
    if ((j = ent_at_player(EK_STAND)) >= 0) { dd_buy(j); return true; }
    if ((j = ent_at_player(EK_DOOR)) >= 0) {
        int id = dd_ent[j].param;
        if (dd_door_is(id, "console")) {
            if (dd_flag(FL_GEAR_DONE)) {
                dd_say("THE CONSOLE", "YOU TAP IN SPINNER'S CODE: GEAR, GEAR, SPRING, KEY. EVERY LIGHT ON THE BOARD GOES RED. SOMEWHERE ABOVE, A MAINSPRING LETS GO.");
                dd_on_boss_dead(F_SPROCKET);
            } else dd_say("THE CONSOLE", "A BOARD OF LEVERS AND LITTLE LIGHTS. IT WANTS A CODE. SOMEONE WHO BUILDS CLOCKWORK MIGHT KNOW IT.");
            return true;
        }
        if (dd_door_is(id, "throne") && !dd_flag(FL_THRONE_OPEN) && !(riding >= 0 && dd_ent[riding].kind == EK_FOE)) {
            dd_say("THE LATCH GATE", "THE GATE INTO THE KNOB IS SHUT AND ITS BRIDGE IS UP. THE TIN MAGE IN LATCHTOWN WORKS THE BRIDGE, FOR A FEE.");
            return true;
        }
        if (dd_door_is(id, "owl") || dd_door_is(id, "seam")) return false;
        if (dd_scale == SC_SMALL) { enter_door(id); return true; }
    }
    /* standing on something: command it, or stop and start Dash */
    if (riding >= 0 && dd_ent[riding].alive) {
        Ent *o = &dd_ent[riding];
        if (o->kind == EK_FOE && ((dd_has(U_BUZZ1) && (FOE[o->sub].flags & FF_BUG)) || (dd_has(U_BUZZ2) && (FOE[o->sub].flags & FF_GERM)))) {
            o->cmd = 1;
            o->st = 0;
            o->dir = dd_p.facing;
            sfx_play_name("dd_command");
            dd_popup(o->x, o->y - 10, "GO!", C_YELLOW);
            return true;
        }
        if (o->kind == EK_DASH) {
            bool go = o->state == 6 || o->state == 7 || o->state == 8;
            dd_dash_go(go);
            dd_popup(o->x, o->y - 10, go ? "GO ON!" : "STAY!", C_WHITE);
            sfx_play_name("dd_command");
            return true;
        }
    }
    /* beside Dash while he sits and looks up at her: he has something to say */
    if ((j = ent_at_player(EK_DASH)) >= 0 && !dd_ent[j].held && (dd_ent[j].state == 8 || dd_ent[j].state == 6)) {
        dd_dash_talk();
        return true;
    }
    return false;
}

static void touch_pickups(void) {
    for (int j = 0; j < DD_MAX_ENTS; j++) {
        Ent *o = &dd_ent[j];
        if (!o->alive || o->kind != EK_PICK || !player_overlap(o)) continue;
        o->alive = 0;
        if (o->pid) dd_collect(o->pid);
        switch (o->sub) {
        case P_GLINT1: dd_give_glints(1); sfx_play_name("dd_glint"); break;
        case P_GLINT5: dd_give_glints(5); sfx_play_name("dd_glint5"); dd_popup(o->x, o->y, "+5", C_YELLOW); break;
        case P_GLINT50:
            dd_sv.bigs_found++;
            if (o->param > 0) dd_sv.caves_done |= 1u << ((o->param - 1) & 31);
            dd_give_glints(50);
            sfx_play_name("dd_big");
            dd_popup(o->x, o->y, "+50", C_YELLOW);
            flash_message("A BIG GLINT! +50");
            break;
        case P_HEART: dd_hp = dd_hp_max; sfx_play_name("dd_heart"); break;
        case P_UPGRADE: dd_award_upgrade(o->param); break;
        }
    }
}

/* a drop of pink slime: it burns, unless Dot holds a jar over her head to
 * catch it (an empty one fills) or has eaten the spore stew */
static bool holding_jar(void) {
    if (dd_carry < 0 || dd_ent[dd_carry].kind != EK_OBJ) return false;
    int s = dd_ent[dd_carry].sub;
    return s == O_JAR || s == O_JARWATER || s == O_JARSLIME || s == O_JARHONEY || s == O_JARACID;
}
static void slime_hits_dot(Ent *o) {
    o->alive = 0;
    if (holding_jar()) {
        if (dd_ent[dd_carry].sub == O_JAR) {
            dd_ent[dd_carry].sub = O_JARSLIME;
            sfx_play_name("dd_fill");
            dd_popup(dd_p.x, dd_p.y - 16, "SLIME!", C_PINK);
        } else sfx_play_name("dd_block");
        return;
    }
    if (dd_has(U_STEW)) { sfx_play_name("dd_splat"); return; }
    dd_hurt(1, o->x);
}

static void foe_contacts(void) {
    for (int j = 0; j < DD_MAX_ENTS; j++) {
        Ent *o = &dd_ent[j];
        if (!o->alive) continue;
        if (o->kind == EK_SHOT) {
            if (!player_overlap(o) || o->param == 2) continue; /* Dot's own fireballs */
            if (o->sub == 7) { slime_hits_dot(o); continue; }
            if (o->sub >= 5) continue; /* water and honey drips are harmless */
            /* the spin top flips shots away while Dot is jumping */
            if (dd_has(U_TOP1) && !dd_p.ground) {
                o->vx = -o->vx;
                o->vy = -2;
                o->param = 1;
                sfx_play_name("dd_flip");
                continue;
            }
            dd_hurt(o->sub == 3 ? 2 : 1, o->x);
            o->alive = 0;
            continue;
        }
        if (o->kind != EK_FOE || o->held || o->stun || o->state == 9 || riding == j) continue;
        const FoeDef *D = &FOE[o->sub];
        if (D->dmg == 0 || !player_overlap(o)) continue;
        if (o->sub == F_SPROCKET && o->state == 1 && dd_p.y + dd_p.h <= o->y + 18) continue;
        if (dd_has(U_MUSK) && (D->flags & FF_BUG) && !(D->flags & FF_BOSS)) continue;
        if (dd_has(U_MUSK) && o->sub == F_EARWIG) continue;
        if (dd_has(U_TOP1) && !dd_p.ground && (D->flags & FF_HOP)) {
            o->vx = dd_p.x < o->x ? 3.0f : -3.0f;
            o->vy = -3;
            o->stun = 40;
            if (dd_has(U_TOP2)) damage_foe(j, dd_pep(), dd_p.x);
            sfx_play_name("dd_flip");
            continue;
        }
        dd_hurt(D->dmg, o->x + o->w / 2);
    }
}

/* land on the top of a creature, an object or Dash */
static void ride_check(float old_bottom) {
    bool full = dd_scale == SC_FULL;
    if (riding >= 0) {
        Ent *o = &dd_ent[riding];
        bool still = o->alive && !o->held && dd_p.vy >= 0 && dd_p.x + dd_p.w > o->x && dd_p.x < o->x + o->w &&
                     fabsf(dd_p.y + dd_p.h - o->y) < 4;
        if (o->kind == EK_FOE && o->sub == F_SPROCKET) still = still && o->state == 1;
        if (!still) riding = -1;
    }
    if (dd_p.vy < 0) return;
    float nb = dd_p.y + dd_p.h;
    for (int j = 0; j < DD_MAX_ENTS && riding < 0; j++) {
        Ent *o = &dd_ent[j];
        if (!o->alive || o->held) continue;
        if (full && o->kind != EK_OBJ) continue; /* at full size only a set-down block holds her */
        bool plat = o->kind == EK_OBJ ? o->state == 0 : o->kind == EK_DASH ? o->state != 9 && o->state != 4 : o->kind == EK_FOE;
        if (!plat) continue;
        float top = o->y;
        if (o->kind == EK_FOE) {
            if (FOE[o->sub].flags & FF_SPIKY) continue;
            if (o->sub == F_SPROCKET) { if (o->state != 1) continue; top = o->y + 16; }
        }
        if (dd_p.x + dd_p.w <= o->x + 1 || dd_p.x >= o->x + o->w - 1) continue;
        if (old_bottom <= top + 1 && nb >= top - 0.5f) {
            dd_p.y = top - dd_p.h;
            if (!dd_p.ground || dd_p.landed_fall == 0) dd_p.landed_fall = (int16_t)imax(dd_p.landed_fall, (int)(dd_p.y - dd_p.peak_y));
            dd_p.peak_y = (int16_t)dd_p.y;
            dd_p.vy = 0;
            dd_p.ground = 1;
            dd_p.jumping = 0;
            riding = j;
            if (o->kind == EK_FOE && o->sub == F_ROTIFER && o->state == 0) {
                /* the stomp: it gapes */
                o->state = 1;
                o->st = 150;
                dd_p.vy = -4;
                dd_p.ground = 0;
                riding = -1;
                sfx_play_name("dd_stomp");
            }
            /* already gaping, she just stands on it: the tongue wants a throw */
        }
    }
    if (riding >= 0) {
        Ent *o = &dd_ent[riding];
        float top = o->y + (o->kind == EK_FOE && o->sub == F_SPROCKET ? 16 : 0);
        dd_p.y = top - dd_p.h;
        dd_p.ground = 1;
        if (dd_p.vy > 0) dd_p.vy = 0;
        dd_p.coyote = 5;
        (void)ride_dx;
        (void)ride_dy;
    }
}

static void fall_out(void) {
    if (dd_dead_t > 0 || dd_god || dd_trans) return;
    dd_hp = 0;
    dd_dead_t = 70;
    shake_t = 8;
    sfx_play_name("dd_die");
    flash_message("TOO FAR TO FALL!");
}

static void player_update(void) {
    uint32_t in = dd_in;
    bool small = dd_scale != SC_FULL;
    static const uint32_t DIRS[4] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT};
    for (int k = 0; k < 4; k++) {
        dbl[k] = false;
        if (pressed(DIRS[k])) { dbl[k] = tap_age[k] < TAP_GAP; tap_age[k] = 0; }
        else if (tap_age[k] < 99) tap_age[k]++;
    }
    if (dd_hurt_t > 0) dd_hurt_t--;
    if (kick_t > 0) kick_t--;
    if (throw_t > 0) throw_t--;
    if (message_t > 0) message_t--;
    const Phys *ph = dd_phys();
    int bean = dd_has(U_BEAN2) ? 2 : dd_has(U_BEAN1) ? 1 : 0;
    if (lift_t > 0) {
        dd_p.crouch = 1;
        if (--lift_t == 0) finish_pickup();
        dd_body_step(&dd_p, 0, 0, &dd_lv, ph, false, 0);
        dd_p.crouch = 0;
        place_held();
        return;
    }
    uint32_t bin = in;
    if (small && dd_has(U_SATCHEL1) && pressed(BTN_A) && (in & BTN_DOWN)) { satchel_use(); bin &= ~BTN_A; }
    if (small && dd_has(U_FIZZ) && (dbl[2] || dbl[3])) dd_p.sprint = 1;
    if (!(in & (BTN_LEFT | BTN_RIGHT))) dd_p.sprint = 0;
    if (small && dd_has(U_BANGLE) && dbl[1] && dd_p.ground) { dd_p.drop_t = 10; riding = -1; }
    if (small && dd_has(U_WHISTLE) && dbl[0]) call_dash();
    if (pressed(BTN_UP) && dd_p.ground && !(in & BTN_A)) { if (interact()) talk_lock = 1; }
    /* DOWN tells a commanded flyer to stop and hover */
    if (pressed(BTN_DOWN) && riding >= 0 && dd_ent[riding].kind == EK_FOE && dd_ent[riding].cmd) {
        Ent *o = &dd_ent[riding];
        o->cmd = 0;
        o->home_x = (int16_t)o->x;
        o->home_y = (int16_t)o->y;
        o->t = 0;
        o->pad = 1; /* a tamed flyer only drifts a little */
    }
    if (!(in & BTN_UP)) talk_lock = 0;
    /* hold DOWN to shrink, hold UP to grow */
    bool still = dd_p.ground && !(in & (BTN_LEFT | BTN_RIGHT | BTN_A | BTN_B));
    if (still && (in & BTN_DOWN) && !(in & BTN_UP)) {
        dd_p.crouch = 1;
        if (++dd_shrink_t >= SHRINK_T) { dd_shrink_t = 0; dd_change_scale(1); }
        if (dd_shrink_t % 20 == 1) sfx_play_name("dd_charge");
    } else { dd_shrink_t = 0; dd_p.crouch = 0; }
    if (still && (in & BTN_UP) && !(in & BTN_DOWN) && !talk_lock && dd_scale != SC_FULL) {
        if (++dd_grow_t >= GROW_T) { dd_grow_t = 0; dd_change_scale(-1); }
        if (dd_grow_t % 20 == 1) sfx_play_name("dd_charge");
    } else dd_grow_t = 0;
    /* B: throw, set down, kick or pick up (at full size too: a block the
     * size of Dot's head, to stand on) */
    if (pressed(BTN_B)) {
        if (dd_carry >= 0) {
            Ent *o = &dd_ent[dd_carry];
            if (o->kind == EK_OBJ && o->sub == O_POPGUN && !(in & BTN_DOWN)) fire_popgun();
            else if (o->kind == EK_FOE && o->sub == F_SPITTER && !(in & BTN_DOWN)) { o->dir = dd_p.facing; o->x = dd_p.x + dd_p.w / 2 - o->w / 2; spit_fire(o, true); }
            else release_held(in);
        } else if (small && (in & BTN_UP) && dd_has(U_CLOGS1)) kick();
        else try_pickup();
    }
    float old_bottom = dd_p.y + dd_p.h;
    /* carried along by what she stands on */
    if (riding >= 0 && dd_ent[riding].alive) {
        Ent *o = &dd_ent[riding];
        if (ride_lr == riding) {
            float dx = o->x - ride_dx;
            if (fabsf(dx) < 8 && !dd_body_blocked(&dd_lv, ph->ts, dd_p.x + dx, dd_p.y, dd_p.w, dd_p.h)) dd_p.x += dx;
        }
        ride_dx = o->x;
        ride_lr = riding;
    } else ride_lr = -1;
    /* what she holds overhead makes her that much taller */
    int ch = 0;
    if (dd_carry >= 0 && dd_ent[dd_carry].alive) ch = dd_ent[dd_carry].h;
    dd_p.y -= (float)ch;
    dd_p.peak_y = (int16_t)(dd_p.peak_y - ch);
    dd_p.h = (int16_t)(dd_p.h + ch);
    dd_body_step(&dd_p, bin, dd_in_prev, &dd_lv, ph, dd_has(U_BANGLE), bean);
    dd_p.h = (int16_t)(dd_p.h - ch);
    dd_p.y += (float)ch;
    dd_p.peak_y = (int16_t)(dd_p.peak_y + ch);
    ride_check(old_bottom);
    if (small && !dd_p.ground && dd_carry >= 0 && dd_ent[dd_carry].kind == EK_FOE && (FOE[dd_ent[dd_carry].sub].flags & FF_FLY)) {
        if (dd_p.vy > 1.0f) dd_p.vy = 1.0f;
        dd_p.peak_y = (int16_t)dd_p.y; /* a flyer in hand: a gentle glide */
    }
    if (pressed(BTN_A) && dd_p.jumping) sfx_play_name("dd_jump");
    /* a long fall is too much for someone so small: she grows back to full
     * size where she lands, as when she runs out of hearts. The feather
     * charm stops it, and so does holding a flyer, which slows the fall */
    if (small && dd_p.landed_fall > FALL_TILES * DD_TS && !dd_has(U_FEATHER)) fall_out();
    if (dd_p.bounced) sfx_play_name("dd_boing");
    place_held();
    if (!small) return;
    /* thorns and goo */
    int x0 = (int)((dd_p.x + 1) / DD_TS), x1 = (int)((dd_p.x + dd_p.w - 1) / DD_TS);
    int y0 = (int)((dd_p.y + 2) / DD_TS), y1 = (int)((dd_p.y + dd_p.h - 1) / DD_TS);
    int fl = 0;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++) fl |= DD_TILE[lv_tile(&dd_lv, tx, ty)].flags;
    if (fl & TF_HURT) dd_hurt(1, dd_p.x + (dd_p.facing ? 8 : -8)), dd_p.vy = -3.5f;
    /* wading into slime or acid with an empty jar fills it */
    if ((fl & TF_GOO) && dd_carry >= 0 && dd_ent[dd_carry].kind == EK_OBJ && dd_ent[dd_carry].sub == O_JAR) {
        int gt = 0;
        for (int ty = y0; ty <= y1 && !gt; ty++)
            for (int tx = x0; tx <= x1; tx++) { int t = lv_tile(&dd_lv, tx, ty); if (t == T_SLIME || t == T_GOO) { gt = t; break; } }
        dd_ent[dd_carry].sub = gt == T_SLIME ? O_JARSLIME : O_JARACID;
        sfx_play_name("dd_fill");
        dd_popup(dd_p.x, dd_p.y - 16, gt == T_SLIME ? "SLIME!" : "ACID!", C_LIME);
    }
    if ((fl & TF_GOO) && !dd_has(U_STEW)) {
        if (++goo_t % 30 == 1) dd_hurt(1, dd_p.x);
        dd_p.vx *= 0.7f;
    } else goo_t = 0;
    touch_pickups();
    foe_contacts();
    /* NPCs who react when you walk up carrying something */
    int n = ent_at_player(EK_NPC);
    if (n >= 0) dd_npc_touch(n);
}

/* ------------------------------------------------------------------ */
/* all the things each frame                                               */

static void shots_update(int i) {
    Ent *e = &dd_ent[i];
    e->x += e->vx;
    e->y += e->vy;
    if (e->sub == 3) e->vy += 0.12f; /* a cannonball arcs */
    if (--e->t <= 0 || dd_body_blocked(&dd_lv, DD_TS, e->x, e->y, e->w, e->h)) {
        if (e->sub == 3) dd_burst(e->x, e->y, C_ORANGE, 6);
        if (e->sub == 4) {
            /* a fireball breaks the weak block it hits */
            int tx = (int)((e->x + e->w / 2 + e->vx * 2) / DD_TS), ty = (int)((e->y + e->h / 2) / DD_TS);
            if (DD_TILE[lv_tile(&dd_lv, tx, ty)].flags & TF_BOMB) { lv_set(&dd_lv, tx, ty, T_AIR); dd_burst(tx * 8 + 4.0f, ty * 8 + 4.0f, C_BROWN, 6); sfx_play_name("dd_break"); }
            dd_burst(e->x, e->y, C_ORANGE, 4);
        }
        e->alive = 0;
        return;
    }
    /* a pea through a carried drink breaks it */
    if (e->param == 1 && dd_carry >= 0 && dd_ent[dd_carry].kind == EK_OBJ && dd_ent[dd_carry].sub == O_DRINK && ent_overlap(e, &dd_ent[dd_carry])) {
        dd_ent[dd_carry].alive = 0;
        dd_carry = -1;
        dd_burst(e->x, e->y, C_LIME, 8);
        sfx_play_name("dd_splat");
        flash_message("THE DRINK IS SPILT!");
        e->alive = 0;
        return;
    }
    /* pod shots, flipped shots and Dot's fireballs hit creatures too */
    if (e->param)
        for (int j = 0; j < DD_MAX_ENTS; j++) {
            Ent *o = &dd_ent[j];
            if (o->alive && o->kind == EK_FOE && o->sub != F_POD && !(o->sub == F_SPITTER && e->param == 1) && !o->held && ent_overlap(e, o)) {
                damage_foe(j, e->param == 2 ? 1 + dd_pep() : 2, e->x);
                e->alive = 0;
                return;
            }
        }
}

static void drip_update(int i) {
    Ent *e = &dd_ent[i];
    e->t++;
    if (e->t % 150 != 0) return;
    int j = dd_add_ent(EK_SHOT, e->sub == 2 ? 7 : 5 + e->sub, e->x, e->y + 2);
    if (j >= 0) { dd_ent[j].vy = 1.5f; dd_ent[j].w = 3; dd_ent[j].h = 4; dd_ent[j].t = 400; dd_ent[j].param = 0; }
}

static void drop_update(int i) {
    /* a falling drop of water (or honey): catch it in an empty jar */
    Ent *e = &dd_ent[i];
    if (e->vy < 4.0f) e->vy += 0.12f;
    e->y += e->vy;
    if (e->sub == 7 && player_overlap(e)) { slime_hits_dot(e); return; }
    if (e->sub == 7 && holding_jar() && ent_overlap(e, &dd_ent[dd_carry])) { slime_hits_dot(e); return; }
    if (e->sub != 7 && dd_carry >= 0 && dd_ent[dd_carry].kind == EK_OBJ && dd_ent[dd_carry].sub == O_JAR && ent_overlap(e, &dd_ent[dd_carry])) {
        dd_ent[dd_carry].sub = e->sub == 5 ? O_JARWATER : O_JARHONEY;
        e->alive = 0;
        sfx_play_name("dd_fill");
        dd_popup(dd_p.x, dd_p.y - 16, e->sub == 5 ? "WATER!" : "HONEY!", C_ICE);
        return;
    }
    if (--e->t <= 0 || dd_body_blocked(&dd_lv, DD_TS, e->x, e->y, e->w, e->h)) { dd_burst(e->x, e->y, C_ICE, 3); e->alive = 0; }
}

void dd_play_update(void) {
    if (dd_trans > 0) { transition_update(); return; }
    if (dd_dialog_active()) { dd_dialog_update(); return; }
    if (dd_dead_t > 0) {
        if (--dd_dead_t == 20) { pending_action = PA_DEATH; dd_trans = 36; dd_trans_kind = 0; }
        return;
    }
    if (shake_t > 0) shake_t--;
    if (freeze_all_t > 0) freeze_all_t--;
    player_update();
    if (dd_trans > 0) return;
    /* towns found, and the tune for the part of the strip she's in */
    dd_note_town();
    {
        static int last_mu = -1;
        int m = dd_music_for_level();
        if (dd_scale != SC_FULL && dd_lv.d.kind == LV_STRIP && m != last_mu && last_mu >= 0) music_play(DD_MUS[m]);
        last_mu = m;
    }
    for (int i = 0; i < DD_MAX_ENTS; i++) {
        Ent *e = &dd_ent[i];
        if (!e->alive) continue;
        switch (e->kind) {
        case EK_FOE: foe_update(i); break;
        case EK_OBJ:
            obj_update(i);
            if (e->alive && e->param == -2 && e->state == 0) e->alive = 0; /* spent peas */
            break;
        case EK_DASH: dash_update(i); break;
        case EK_SHOT: if (e->sub >= 5) drop_update(i); else shots_update(i); break;
        case EK_DRIP: drip_update(i); break;
        case EK_PICK: e->t++; break;
        case EK_NPC: e->t++; break;
        default: break;
        }
    }
    if (dd_dash >= 0 && !dd_ent[dd_dash].alive) dd_dash = -1;
    if (dd_carry >= 0 && !dd_ent[dd_carry].alive) dd_carry = -1;
    for (int i = 0; i < DD_MAX_PARTS; i++)
        if (parts[i].life > 0) { parts[i].x += parts[i].vx; parts[i].y += parts[i].vy; parts[i].vy += 0.08f; parts[i].life--; }
    for (int i = 0; i < 8; i++) if (pops[i].life > 0) { pops[i].life--; pops[i].y -= 0.4f; }
    camera_update();
    if (save_dirty && ++save_wait > 30) dd_save_now();
}

/* ---- read-only access for drawing and tests ---- */
int dd_riding(void) { return riding; }
int dd_lift_t(void) { return lift_t; }
int dd_kick_t(void) { return kick_t; }
int dd_throw_t(void) { return throw_t; }
int dd_freeze_t(void) { return freeze_all_t; }
void dd_parts_draw(int cx, int cy) {
    for (int i = 0; i < DD_MAX_PARTS; i++)
        if (parts[i].life > 0) gfx_pset((int)parts[i].x - cx, (int)parts[i].y - cy, parts[i].col);
    for (int i = 0; i < 8; i++)
        if (pops[i].life > 0) tiny_center(pops[i].s, (int)pops[i].x - cx + 4, (int)pops[i].y - cy, pops[i].life % 6 < 3 ? pops[i].col : C_WHITE);
}
void dd_set_message(const char *s) { flash_message(s); }
void dd_reset_play_state(void) {
    lift_t = kick_t = throw_t = 0;
    riding = -1;
    lift_target = -1;
    pending_action = PA_NONE;
    dd_trans = 0;
    dd_dead_t = 0;
    dd_hurt_t = 0;
    freeze_all_t = shake_t = 0;
    message_t = 0;
    save_dirty = false;
    for (int k = 0; k < 4; k++) tap_age[k] = 99;
}
bool dd_save_pending(void) { return save_dirty; }
void dd_restore_carry_after_load(void) {
    if (dd_sv.carry) {
        int j = dd_add_obj(dd_sv.carry - 1, dd_p.x, dd_p.y - 8);
        if (j >= 0) {
            dd_ent[j].param = dd_sv.carry_param;
            dd_ent[j].held = 1;
            dd_ent[j].state = 1;
            if (dd_scale == SC_FULL) dd_ent[j].w = dd_ent[j].h = 16;
            dd_carry = j;
        }
    }
}
void dd_spawn_dash_now(void) { if (dd_dash < 0) spawn_dash_near(); }
