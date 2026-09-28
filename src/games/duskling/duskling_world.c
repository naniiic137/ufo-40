/* DUSKLING - the world: the two-sided controls, the duskling's body, the
 * foes and bosses of each room, the hidden things, and a route finder the
 * tests use to prove every room can be crossed.
 * Cartridge 13 of UFO 40, a tribute to Mooncat (UFO 50 #13). */
#include "duskling.h"

#define TS DK_TS
#define PW 7            /* the duskling's body */
#define PH 9
#define WALK 1.2f
#define SPRINT 2.2f
#define ACC_G 0.18f
#define DEC_G 0.22f
#define SLOW_G 0.06f    /* landing faster than a walk: the speed bleeds away */
#define ACC_A 0.07f
#define GRAV 0.24f
#define GRAV_HOLD 0.11f /* while the jump is still held on the way up */
#define JUMP_V 3.3f
#define JUMP_HOLD 16
#define LOW_V 2.3f
#define FALL_MAX 4.5f
#define POUND_V 5.0f
#define POUND_VX 1.3f
#define BOUNCE_V 2.4f   /* a slam on the ground bounces you back up */
#define STOMP_V 3.8f    /* a slam on a foe */
#define FROG_V 5.2f
#define SPRING_V 5.4f
#define SPIN_VX 1.8f
#define SPIN_MAX 3.0f
#define SPIN_LIFT 1.3f
#define SLIDE_V 3.0f
#define SLIDE_T 18
#define COYOTE 5
#define COYOTE_SPRINT 12
#define DTAP 14         /* a second press this soon is a double tap */
#define SIMUL 3         /* presses this close together count as both at once */
#define W_GRAV 0.07f
#define W_FALL 1.2f
#define W_STROKE 2.1f
#define W_MAX 1.0f
#define W_LEAP 3.2f     /* a stroke that breaks the surface */
#define DEAD_T 36
#define THUMP_R 72.0f   /* a slam wakes ceiling eyes this far across */
#define STUN_T 45

DKWorld dk_w;
bool dk_sim_quiet;
int dk_nerf; /* tests only: NERF_* switches that take one secret away */
int dk_solve_ok = -1;
int dk_last_event;

static char map[DK_MAXH][DK_MAXW + 1];
static uint8_t mut_idx[DK_MAXH][DK_MAXW]; /* 1 + index into dk_w.mut */
static uint8_t wet[DK_MAXH][DK_MAXW];     /* water, and the marks that sit in it */
static int RW, RH, n_mut;
static bool has_clock; /* the room has something that keeps time */

#define W dk_w

/* ------------------------------------------------------------------ */
/* tiles                                                                */

int dk_room_w(void) { return RW; }
int dk_room_h(void) { return RH; }

char dk_tile_raw(int tx, int ty) {
    if (tx < 0 || tx >= RW || ty < 0 || ty >= RH) return ' ';
    return map[ty][tx];
}

static bool mut_on(int tx, int ty) {
    if (tx < 0 || tx >= RW || ty < 0 || ty >= RH) return false;
    int i = mut_idx[ty][tx];
    return i && W.mut[i - 1];
}

bool dk_hidden_shown(int tx, int ty) { return mut_on(tx, ty); }
bool dk_wet(int tx, int ty) { return tx >= 0 && tx < RW && ty >= 0 && ty < RH && wet[ty][tx]; }

static bool rect_wet(float x, float y, float w, float h) {
    int x0 = (int)floorf(x / TS), x1 = (int)floorf((x + w - 0.01f) / TS), y0 = (int)floorf(y / TS), y1 = (int)floorf((y + h - 0.01f) / TS);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (dk_wet(tx, ty)) return true;
    return false;
}
bool dk_face_awake(int tx, int ty) { return mut_on(tx, ty); }

char dk_tile(int tx, int ty) {
    if (tx < 0 || tx >= RW) return '#';
    if (ty < 0) return ' ';
    if (ty >= RH) return ' ';
    return map[ty][tx];
}

static bool solid_tile(int tx, int ty) {
    char c = dk_tile(tx, ty);
    switch (c) {
    case '#': case '%': return true;
    case '?': return mut_on(tx, ty);
    case 'D': return !W.boss_down;
    default: return false;
    }
}
bool dk_solid_at(int tx, int ty) { return solid_tile(tx, ty); }

static int tof(float v) { return (int)floorf(v / TS); }

static bool rect_has(float x, float y, float w, float h, char c) {
    int x0 = tof(x), x1 = tof(x + w - 0.01f), y0 = tof(y), y1 = tof(y + h - 0.01f);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (dk_tile(tx, ty) == c) return true;
    return false;
}

/* ------------------------------------------------------------------ */
/* foe boxes: gulpers, sitting eyes and curled beetles are solid; a
 * walking beetle is a ledge from above; a frog is a spring */

static void foe_box(const DKFoe *f, float *x, float *y, float *w, float *h) {
    switch (f->kind) {
    case F_GULPER: *x = f->x; *y = f->y; *w = 20; *h = 10; break;
    case F_EYE: *x = f->x; *y = f->y; *w = 10; *h = 10; break;
    case F_BEETLE: *x = f->x; *y = f->y + 2; *w = 10; *h = 8; break;
    case F_FROG: *x = f->x; *y = f->y + 3; *w = 10; *h = 7; break;
    case F_WASP: case F_WASPV: *x = f->x + 1; *y = f->y + 1; *w = 8; *h = 7; break;
    case F_FISH: *x = f->x + 1; *y = f->y + 2; *w = 8; *h = 6; break;
    case F_PRICKLE: *x = f->x + 1; *y = f->y + 2; *w = 8; *h = 8; break;
    case F_NEWT: *x = f->x + 1; *y = f->y + 1; *w = 8; *h = 9; break;
    default: *x = f->x; *y = f->y; *w = 10; *h = 10; break;
    }
}

static bool foe_is_block(const DKFoe *f) {
    if (!f->alive) return false;
    if (f->kind == F_GULPER) return true;
    if (f->kind == F_EYE) return f->state == 2 || f->state == 3;
    if (f->kind == F_BEETLE) return f->state == 2;
    return false;
}
static bool foe_is_ledge(const DKFoe *f) { return f->alive && f->kind == F_BEETLE && f->state == 0; }

static bool rect_solid(float x, float y, float w, float h) {
    int x0 = tof(x), x1 = tof(x + w - 0.01f), y0 = tof(y), y1 = tof(y + h - 0.01f);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (solid_tile(tx, ty)) return true;
    for (int i = 0; i < DK_FOES; i++) {
        const DKFoe *f = &W.foe[i];
        if (!foe_is_block(f)) continue;
        float fx, fy, fw, fh;
        foe_box(f, &fx, &fy, &fw, &fh);
        if (x < fx + fw && fx < x + w && y < fy + fh && fy < y + h) return true;
    }
    return false;
}

/* a one-way top between old and new feet: '=' tiles and walking beetles */
static bool oneway_top(float x, float w, float oldb, float newb, int *foe_i) {
    *foe_i = -1;
    if (newb <= oldb) return false;
    int x0 = tof(x), x1 = tof(x + w - 0.01f);
    int ty = tof(newb - 0.01f);
    float top = (float)(ty * TS);
    if (oldb <= top + 0.01f && newb > top)
        for (int tx = x0; tx <= x1; tx++)
            if (dk_tile(tx, ty) == '=') return true;
    for (int i = 0; i < DK_FOES; i++) {
        const DKFoe *f = &W.foe[i];
        if (!foe_is_ledge(f)) continue;
        float fx, fy, fw, fh;
        foe_box(f, &fx, &fy, &fw, &fh);
        if (x < fx + fw && fx < x + w && oldb <= fy + 0.5f && newb > fy) { *foe_i = i; return true; }
    }
    return false;
}

/* ------------------------------------------------------------------ */
/* loading a room                                                       */

static const char *FOE_CH = "wvypFMerncx";

static void spawn_foe(char c, int tx, int ty) {
    const char *k = strchr(FOE_CH, c);
    if (!k) return;
    for (int i = 0; i < DK_FOES; i++) {
        DKFoe *f = &W.foe[i];
        if (f->alive) continue;
        memset(f, 0, sizeof *f);
        f->kind = (uint8_t)(k - FOE_CH);
        f->alive = 1;
        f->x = f->x0 = (float)(tx * TS);
        f->y = f->y0 = (float)(ty * TS);
        f->dir = -1;
        f->phase = (int16_t)((tx * 37 + ty * 11) % 97);
        switch (f->kind) {
        case F_GULPER: f->phase = (int16_t)((tx * 13) % 150); break;
        case F_EYE: {
            /* hangs from the ceiling above its mark */
            int cy = ty;
            while (cy > 0 && !solid_tile(tx, cy - 1)) cy--;
            f->y = f->y0 = (float)(cy * TS);
            break;
        }
        case F_PUFF: f->phase = (int16_t)(tx * 29 % 120); break;
        default: break;
        }
        return;
    }
}

static void find_marker(char m, float *x, float *y) {
    for (int ty = 0; ty < RH; ty++)
        for (int tx = 0; tx < RW; tx++)
            if (map[ty][tx] == m) {
                *x = (float)(tx * TS + (TS - PW) / 2);
                *y = (float)(ty * TS + TS - PH);
                return;
            }
    /* no such mark: the plain way in */
    if (m != 'S') find_marker('S', x, y);
    else { *x = 12; *y = 12; }
}

static void spawn_player(int i) {
    DKPlayer *p = &W.P[i];
    int16_t keep_anim = p->anim;
    memset(p, 0, sizeof *p);
    char m = W.arrive == 1 ? '@' : W.arrive == 2 ? '&' : 'S';
    find_marker(m, &p->x, &p->y);
    if (i == 1) p->x += 10;
    p->alive = 1;
    p->face = 1;
    p->on_foe = -1;
    if (W.room == RM_M0 && W.t == 0 && i == 0) {
        /* the dayling comes down out of the clouds first */
        p->y = -30;
        p->ride = 1;
    }
    for (int s = 0; s < 3; s++) p->press_t[s] = p->last_press_t[s] = p->release_t[s] = -1000;
    p->anim = keep_anim;
}

static int16_t cur_room;

static void build_room(void) {
    const DKRoom *R = &DK_ROOM[cur_room];
    RH = 0;
    RW = (int)strlen(R->rows[0]);
    if (RW > DK_MAXW) RW = DK_MAXW;
    for (int y = 0; R->rows[y] && y < DK_MAXH; y++) {
        memset(map[y], ' ', DK_MAXW);
        memcpy(map[y], R->rows[y], (size_t)imin(RW, (int)strlen(R->rows[y])));
        map[y][RW] = 0;
        RH = y + 1;
    }
    memset(mut_idx, 0, sizeof mut_idx);
    memset(wet, 0, sizeof wet);
    for (int ty = 0; ty < RH; ty++)
        for (int tx = 0; tx < RW; tx++) {
            char c = map[ty][tx];
            if (c == '~') wet[ty][tx] = 1;
            else if (strchr("S@&>123box", c)) {
                /* a mark in the water is water too */
                if ((tx > 0 && map[ty][tx - 1] == '~') || (tx + 1 < RW && map[ty][tx + 1] == '~') ||
                    (ty > 0 && map[ty - 1][tx] == '~') || (ty + 1 < RH && map[ty + 1][tx] == '~'))
                    wet[ty][tx] = 1;
            }
        }
    n_mut = 0;
    has_clock = false;
    for (int ty = 0; ty < RH; ty++)
        for (int tx = 0; tx < RW; tx++) {
            char c = map[ty][tx];
            if ((c == '?' || c == '%' || c == 'I') && n_mut < DK_MUT) mut_idx[ty][tx] = (uint8_t)(++n_mut);
            if (strchr("vyMenWHG", c)) has_clock = true;
        }
}

static void populate(void) {
    memset(W.foe, 0, sizeof W.foe);
    memset(W.shot, 0, sizeof W.shot);
    memset(W.mut, 0, sizeof W.mut);
    memset(&W.boss, 0, sizeof W.boss);
    W.boss_down = 0;
    for (int ty = 0; ty < RH; ty++)
        for (int tx = 0; tx < RW; tx++) {
            char c = map[ty][tx];
            if (strchr(FOE_CH, c)) {
                spawn_foe(c, tx, ty);
                map[ty][tx] = ' ';
            } else if (c == 'W' || c == 'H' || c == 'G') {
                DKBoss *b = &W.boss;
                b->kind = c == 'W' ? BOSS_WARDEN : c == 'H' ? BOSS_HERMIT : BOSS_BADGER;
                b->alive = 1;
                b->solid = 1;
                b->hp = b->kind == BOSS_WARDEN ? 6 : 5;
                b->x = (float)(tx * TS);
                b->y = (float)(ty * TS);
                b->dir = -1;
                map[ty][tx] = ' ';
            }
        }
    /* the newts face the way in */
    float sx, sy;
    find_marker('S', &sx, &sy);
    for (int i = 0; i < DK_FOES; i++)
        if (W.foe[i].alive && W.foe[i].kind == F_NEWT) W.foe[i].dir = (int8_t)(sx < W.foe[i].x ? -1 : 1);
    W.t = 0;
    W.freeze = 0;
}

void dk_room_load(int room, int arrive, int nplayers) {
    cur_room = (int16_t)iclamp(room, 0, DK_ROOMS - 1);
    W.room = cur_room;
    W.arrive = (uint8_t)arrive;
    W.nplayers = (uint8_t)iclamp(nplayers, 1, DK_PLAYERS);
    build_room();
    populate();
    for (int i = 0; i < DK_PLAYERS; i++) {
        memset(&W.P[i], 0, sizeof W.P[i]);
        if (i < W.nplayers) spawn_player(i);
    }
}

void dk_room_reset(void) {
    int16_t hb = W.head_bounces;
    dk_room_load(W.room, W.arrive, W.nplayers);
    W.head_bounces = hb;
}

int dk_foe_count(int kind) {
    int n = 0;
    for (int i = 0; i < DK_FOES; i++) n += W.foe[i].alive && (kind < 0 || W.foe[i].kind == kind);
    return n;
}

void dk_player_hitbox(int i, float *x, float *y, float *w, float *h) {
    *x = W.P[i].x; *y = W.P[i].y; *w = PW; *h = PH;
}

/* ------------------------------------------------------------------ */
/* hidden things                                                        */

static void reveal_near(int cx, int cy) {
    for (int ty = imax(0, cy - 12); ty <= imin(RH - 1, cy + 12); ty++)
        for (int tx = imax(0, cx - 20); tx <= imin(RW - 1, cx + 20); tx++)
            if (map[ty][tx] == '?' && mut_idx[ty][tx]) W.mut[mut_idx[ty][tx] - 1] = 1;
}

static void faces_check(DKPlayer *p) {
    if (p->ground) return;
    int x0 = tof(p->x), x1 = tof(p->x + PW - 0.01f);
    int yb = tof(p->y + PH);
    /* a stone face with the duskling in the air right over it (up to four tiles) */
    for (int tx = x0; tx <= x1; tx++)
        for (int ty = yb; ty < imin(RH, yb + 5); ty++) {
            char c = dk_tile(tx, ty);
            if (c == 'I') {
                if (p->y + PH <= ty * TS - 1 && !mut_on(tx, ty) && !(dk_nerf & NERF_FACES)) {
                    W.mut[mut_idx[ty][tx] - 1] = 1;
                    reveal_near(tx, ty);
                    dk_fx("dk_reveal", (float)(tx * TS + 5), (float)(ty * TS), C_CYAN, 10);
                }
                break;
            }
            if (solid_tile(tx, ty)) break;
        }
}

static void springs_touch(DKPlayer *p) {
    int x0 = tof(p->x - 1), x1 = tof(p->x + PW + 0.99f), y0 = tof(p->y - 1), y1 = tof(p->y + PH + 0.99f);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (dk_tile(tx, ty) == '%' && !mut_on(tx, ty)) {
                W.mut[mut_idx[ty][tx] - 1] = 1;
                dk_fx("dk_reveal", (float)(tx * TS + 5), (float)(ty * TS + 5), C_PINK, 6);
            }
}

/* ------------------------------------------------------------------ */
/* foes                                                                 */

static int add_shot(int kind, float x, float y, float vx, float vy) {
    for (int i = 0; i < DK_SHOTS; i++)
        if (!W.shot[i].alive) {
            W.shot[i] = (DKShot){x, y, vx, vy, (uint8_t)kind, 1, 0};
            return i;
        }
    return -1;
}

static bool ground_below(float x, float y) { return rect_solid(x, y, 1, 1) || rect_has(x, y, 1, 1, '='); }

static void carry_riders(int fi, float dx, float dy) {
    for (int i = 0; i < W.nplayers; i++) {
        DKPlayer *p = &W.P[i];
        if (!p->alive || p->on_foe != fi) continue;
        if (!rect_solid(p->x + dx, p->y, PW, PH)) p->x += dx;
        if (dy < 0 && !rect_solid(p->x, p->y + dy, PW, PH)) p->y += dy;
        else if (dy > 0) p->y += dy;
    }
}

/* a slam on the ground: hanging eyes drop, sitting eyes leap up and take
 * whoever stands on them along. Returns true if the slammer rode one up. */
static bool thump(float x, float y, const DKPlayer *slammer) {
    bool rode = false;
    dk_fx("dk_thump", x, y, C_LIGHT, 6);
    for (int i = 0; i < DK_FOES; i++) {
        DKFoe *f = &W.foe[i];
        if (!f->alive || f->kind != F_EYE || (dk_nerf & NERF_EYES)) continue;
        if (fabsf(f->x + 5 - x) > THUMP_R || fabsf(f->y - y) > 140) continue;
        if (f->state == 0) { f->state = 1; f->vy = 0; dk_fx("dk_eye", f->x + 5, f->y + 5, C_RED, 0); }
        else if (f->state == 2) {
            f->state = 3;
            f->vy = -5.2f;
            dk_fx("dk_eye", f->x + 5, f->y + 5, C_RED, 0);
            for (int k = 0; k < W.nplayers; k++) {
                DKPlayer *p = &W.P[k];
                if (!p->alive || p->x + PW <= f->x || p->x >= f->x + 10 || fabsf(p->y + PH - f->y) > 1.5f) continue;
                p->vy = -5.6f;
                p->ground = 0;
                p->on_foe = -1;
                p->act = ACT_BOUNCE;
                p->spun = 0;
                if (p == slammer) rode = true;
            }
        }
    }
    return rode;
}

static void foe_step(int fi) {
    DKFoe *f = &W.foe[fi];
    f->t++;
    float fx, fy, fw, fh;
    switch (f->kind) {
    case F_PRICKLE: {
        float nx = f->x + f->dir * 0.6f;
        float ahead = f->dir > 0 ? nx + 10 : nx - 1;
        if (rect_solid(f->dir > 0 ? nx + 9 : nx, f->y, 1, 9) || !ground_below(ahead, f->y + 10.5f)) f->dir = (int8_t)-f->dir;
        else f->x = nx;
        break;
    }
    case F_WASP: {
        float nx = f->x + f->dir * 0.9f;
        if (rect_solid(f->dir > 0 ? nx + 9 : nx, f->y0, 1, 9) || fabsf(nx - f->x0) > 5 * TS) f->dir = (int8_t)-f->dir;
        else f->x = nx;
        f->y = f->y0 + sinf((f->t + f->phase) * 0.08f) * 3.0f;
        break;
    }
    case F_WASPV: {
        float ny = f->y + f->dir * 0.8f;
        if (rect_solid(f->x, f->dir > 0 ? ny + 9 : ny, 9, 1) || fabsf(ny - f->y0) > 3 * TS) f->dir = (int8_t)-f->dir;
        else f->y = ny;
        break;
    }
    case F_PUFF:
        f->x -= 0.35f;
        f->y = f->y0 + sinf((f->t + f->phase) * 0.05f) * 6.0f;
        if (f->x < -12) f->x = (float)(RW * TS + 4);
        break;
    case F_FROG: {
        int per = 100, k = (f->t + f->phase) % per;
        if (k == 0 && f->state == 0) { f->state = 1; f->vy = -2.6f; }
        if (f->state == 1) {
            f->vy += GRAV;
            f->y += f->vy;
            if (f->y >= f->y0) { f->y = f->y0; f->vy = 0; f->state = 0; }
        }
        break;
    }
    case F_GULPER: {
        int k = (f->t + f->phase) % 150;
        f->state = (uint8_t)(k < 95 ? 0 : k < 140 ? 1 : 0); /* 1 = open */
        break;
    }
    case F_EYE:
        if (f->state == 1 || f->state == 3) {
            float oy = f->y;
            f->vy = fminf(f->vy + 0.3f, 5.0f);
            float ny = f->y + f->vy;
            if (f->vy > 0 && rect_solid(f->x, ny, 10, 10)) {
                /* lands on the floor */
                while (!rect_solid(f->x, f->y + 1, 10, 10) && f->y < RH * TS) f->y += 1;
                f->vy = 0;
                f->state = 2;
                dk_fx("dk_clank", f->x + 5, f->y + 10, C_GREY, 4);
            } else if (f->vy < 0 && rect_solid(f->x, ny, 10, 10)) {
                f->vy = 0;
            } else {
                f->y = ny;
            }
            if (f->y > RH * TS + 20) f->alive = 0;
            (void)oy;
        }
        break;
    case F_BEETLE:
        if (f->state == 0) {
            float nx = f->x + f->dir * 0.35f;
            float ahead = f->dir > 0 ? nx + 10 : nx - 1;
            if (rect_solid(f->dir > 0 ? nx + 9 : nx, f->y + 2, 1, 7) || !ground_below(ahead, f->y + 10.5f)) f->dir = (int8_t)-f->dir;
            else { carry_riders(fi, nx - f->x, 0); f->x = nx; }
        } else if (f->state == 1) {
            f->vy = fminf(f->vy + GRAV, FALL_MAX);
            float nx = f->x + f->vx;
            if (rect_solid(nx, f->y + 2, 10, 8)) f->vx = -f->vx * 0.3f;
            else f->x = nx;
            float ny = f->y + f->vy;
            if (f->vy > 0 && (rect_solid(f->x, ny + 2, 10, 8) || (dk_tile(tof(f->x + 5), tof(ny + 10)) == '=' && f->y + 10 <= tof(ny + 10) * TS))) {
                while (!rect_solid(f->x, f->y + 3, 10, 8) && !(dk_tile(tof(f->x + 5), tof(f->y + 10.5f)) == '=' && fmodf(f->y + 10, TS) < 0.5f) && f->y < RH * TS) f->y += 0.5f;
                f->y = floorf(f->y);
                f->vx = f->vy = 0;
                f->state = 2;
                dk_fx("dk_clank", f->x + 5, f->y + 10, C_TAN, 4);
            } else f->y = ny;
            if (f->y > RH * TS + 20) f->alive = 0;
        }
        break;
    case F_NEWT: {
        int k = (f->t + f->phase) % 110;
        f->state = (uint8_t)(k >= 90);
        if (k == 100) {
            add_shot(SH_SPEAR, f->x + (f->dir > 0 ? 8 : -6), f->y + 3, f->dir * 2.2f, 0);
            dk_fx("dk_spear", f->x + 5, f->y + 3, 0, 0);
        }
        break;
    }
    case F_FISH: {
        float nx = f->x + f->dir * 0.7f;
        int tx = tof(f->dir > 0 ? nx + 10 : nx);
        if (!dk_wet(tx, tof(f->y + 5))) f->dir = (int8_t)-f->dir;
        else f->x = nx;
        f->y = f->y0 + sinf((f->t + f->phase) * 0.06f) * 2.0f;
        break;
    }
    default: break;
    }
    foe_box(f, &fx, &fy, &fw, &fh);
}

static void shots_step(void) {
    for (int i = 0; i < DK_SHOTS; i++) {
        DKShot *s = &W.shot[i];
        if (!s->alive) continue;
        s->t++;
        s->x += s->vx;
        s->y += s->vy;
        float r = s->kind == SH_BLAST ? 5 : 3;
        if (s->kind != SH_SPARK && rect_solid(s->x - r + 1, s->y - r + 1, 2 * r - 2, 2 * r - 2)) s->alive = 0;
        if (s->kind == SH_SPARK) {
            /* runs along the floor and stops at a wall */
            if (rect_solid(s->x + (s->vx > 0 ? 3 : -4), s->y - 2, 1, 4)) s->alive = 0;
        }
        if (s->x < -20 || s->x > RW * TS + 20 || s->y < -40 || s->y > RH * TS + 20 || s->t > 600) s->alive = 0;
    }
}

/* ------------------------------------------------------------------ */
/* bosses                                                               */

static void boss_box(float *x, float *y, float *w, float *h) {
    const DKBoss *b = &W.boss;
    switch (b->kind) {
    case BOSS_WARDEN: *x = b->x + 2; *y = b->y; *w = 14; *h = 20; break;
    case BOSS_HERMIT: *x = b->x + 2; *y = b->y; *w = 12; *h = 16; break;
    default: *x = b->x + 1; *y = b->y + 2; *w = 22; *h = 16; break;
    }
}

static int nearest_player(void) {
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < W.nplayers; i++)
        if (W.P[i].alive) {
            float d = fabsf(W.P[i].x - W.boss.x);
            if (d < bd) { bd = d; best = i; }
        }
    return best;
}

static void boss_fall(DKBoss *b, float w, float h, bool ledges) {
    b->vy = fminf(b->vy + GRAV, FALL_MAX);
    b->ground = 0;
    int n = (int)ceilf(fabsf(b->vy));
    if (n < 1) n = 1;
    float s = b->vy / n;
    for (int k = 0; k < n; k++) {
        float ny = b->y + s;
        if (s > 0) {
            if (rect_solid(b->x, ny, w, h)) { b->vy = 0; b->ground = 1; return; }
            if (ledges) {
                float ob = b->y + h, nb = ny + h;
                int ty = tof(nb - 0.01f);
                float top = (float)(ty * TS);
                if (ob <= top + 0.01f && nb > top)
                    for (int tx = tof(b->x + 2); tx <= tof(b->x + w - 2.01f); tx++)
                        if (dk_tile(tx, ty) == '=') { b->y = top - h; b->vy = 0; b->ground = 1; return; }
            }
        } else if (rect_solid(b->x, ny, w, h)) {
            b->vy = 0;
            return;
        }
        b->y = ny;
    }
}

static void boss_step(void) {
    DKBoss *b = &W.boss;
    if (!b->alive) return;
    b->t++;
    if (b->flash > 0) b->flash--;
    int pi = nearest_player();
    float px = pi >= 0 ? W.P[pi].x : b->x, py = pi >= 0 ? W.P[pi].y : b->y;
    switch (b->kind) {
    case BOSS_WARDEN: {
        /* walks at you, then fades and glides across; now and then a spark
         * runs along the floor. Faster with every hit. */
        float sp = 0.6f + (6 - b->hp) * 0.12f;
        if (b->state == 0) {
            b->solid = b->flash == 0;
            b->dir = (int8_t)(px + 3 < b->x + 9 ? -1 : 1);
            float nx = b->x + b->dir * sp;
            if (!rect_solid(nx, b->y, 18, 20)) b->x = nx;
            if (b->t >= 130) {
                b->state = 1;
                b->t = 0;
                b->cycle++;
                b->dir = (int8_t)(b->x + 9 < RW * TS / 2 ? 1 : -1);
                dk_fx("dk_fade", b->x + 9, b->y + 10, C_GREY, 0);
            }
            if (b->t == 70 && b->cycle % 2 == 1) {
                add_shot(SH_SPARK, b->x + 9, b->y + 17, b->dir * 1.7f, 0);
                dk_fx("dk_zap", b->x + 9, b->y + 17, 0, 0);
            }
        } else {
            b->solid = 0;
            float nx = b->x + b->dir * (2.4f + (6 - b->hp) * 0.2f);
            if (!rect_solid(nx, b->y, 18, 20)) b->x = nx;
            if (b->t >= 55 || rect_solid(nx, b->y, 18, 20)) { b->state = 0; b->t = 0; }
        }
        boss_fall(b, 18, 20, false);
        break;
    }
    case BOSS_HERMIT: {
        /* four perches: he comes, throws three sparks, and goes */
        static const float PX[4] = {40, 262, 96, 206}, PY[4] = {126, 126, 56, 56};
        int n = b->perch % 4;
        b->x = PX[n];
        b->y = PY[n];
        if (b->state == 0) {         /* coming: not yet there */
            b->solid = 0;
            if (b->t >= 30) { b->state = 1; b->t = 0; }
        } else if (b->state == 1) {  /* here */
            b->solid = b->flash == 0;
            b->dir = (int8_t)(px < b->x ? -1 : 1);
            if (b->t == 45) {
                float dx = px + 3 - (b->x + 8), dy = py + 4 - (b->y + 8);
                float d = sqrtf(dx * dx + dy * dy);
                if (d < 1) d = 1;
                float a = atan2f(dy, dx);
                for (int k = -1; k <= 1; k++)
                    add_shot(SH_FIRE, b->x + 8, b->y + 8, cosf(a + k * 0.3f) * 1.5f, sinf(a + k * 0.3f) * 1.5f);
                dk_fx("dk_fire", b->x + 8, b->y + 8, 0, 0);
            }
            if (b->t >= 100 - (5 - b->hp) * 8) { b->state = 2; b->t = 0; }
        } else {                      /* going */
            b->solid = 0;
            if (b->t >= 30) {
                static const int ORDER[4] = {2, 1, 3, 0};
                b->perch = (int16_t)ORDER[n];
                b->state = 0;
                b->t = 0;
            }
        }
        break;
    }
    case BOSS_BADGER: {
        /* slow; climbs to the ledge you stand on and blasts at you */
        b->solid = b->flash == 0;
        if (b->state == 0 && b->ground) {
            bool up = pi >= 0 && W.P[pi].ground && py + PH < b->y + 18 - 6;
            bool down = pi >= 0 && W.P[pi].ground && py + PH > b->y + 18 + 6;
            b->dir = (int8_t)(px + 3 < b->x + 12 ? -1 : 1);
            float nx = b->x + b->dir * 0.45f;
            if (fabsf(px + 3 - (b->x + 12)) > 2 && !rect_solid(nx, b->y + 2, 24, 16)) b->x = nx;
            if (up && b->t >= 60) {
                /* a big hop up to your ledge */
                float rise = (b->y + 18) - (py + PH) + 10;
                b->vy = -sqrtf(2 * GRAV * fmaxf(rise, 10));
                b->vx = (px + 3 - (b->x + 12)) / 36.0f;
                b->state = 1;
                b->t = 0;
                dk_fx("dk_hop", b->x + 12, b->y + 18, 0, 0);
            } else if (down && b->t >= 90) {
                /* you went down: it drops through its ledge after you */
                b->vx = 0;
                b->vy = 0;
                b->state = 2;
                b->t = 0;
            }
        }
        if (b->state == 1 || b->state == 2) {
            float nx = b->x + b->vx;
            if (!rect_solid(nx, b->y + 2, 24, 16)) b->x = nx;
        }
        boss_fall(b, 24, 18, b->state != 2);
        if (b->state != 0 && b->ground && b->vy == 0 && b->t > 2) {
            b->state = 0;
            b->t = 0;
            b->vx = 0;
            add_shot(SH_BLAST, b->x + 12, b->y + 8, (px + 3 < b->x + 12 ? -1 : 1) * 1.2f, 0);
            dk_fx("dk_blast", b->x + 12, b->y + 8, 0, 0);
        }
        break;
    }
    default: break;
    }
}

/* ------------------------------------------------------------------ */
/* the duskling                                                         */

static int dirof(int side) { return side == SIDE_L ? -1 : 1; }
static int other(int side) { return side == SIDE_L ? SIDE_R : SIDE_L; }

static void die(int i) {
    DKPlayer *p = &W.P[i];
    if (!p->alive) return;
    p->alive = 0;
    p->dead_t = DEAD_T;
    dk_fx("dk_die", p->x + 3, p->y + 4, C_PINK, 14);
}

static void start_jump(DKPlayer *p, int d, float v, int trigger) {
    p->vy = -v;
    float keep = p->vx * d;
    p->vx = d * fmaxf(WALK, keep);
    p->ground = 0;
    p->coyote = 0;
    p->act = ACT_JUMP;
    p->jump_side = (uint8_t)trigger;
    p->jump_hold = (int16_t)(trigger ? JUMP_HOLD : 0);
    p->jump_dir = (int8_t)d;
    p->face = (int8_t)d;
    p->spun = 0;
    p->on_foe = -1;
    dk_fx(v < JUMP_V ? "dk_hop" : "dk_jump", p->x + 3, p->y + PH, 0, 0);
}

static void land(DKPlayer *p, int i, int foe_i) {
    (void)i;
    bool was_pound = p->act == ACT_POUND;
    p->ground = 1;
    p->spun = 0;
    p->on_foe = (int8_t)foe_i;
    if (foe_i < 0) {
        /* a spring block underfoot: a slam onto it springs higher */
        int ty = tof(p->y + PH + 0.5f);
        for (int tx = tof(p->x); tx <= tof(p->x + PW - 0.01f); tx++)
            if (dk_tile(tx, ty) == '%' && !(dk_nerf & NERF_SPRINGS)) {
                if (!mut_on(tx, ty)) W.mut[mut_idx[ty][tx] - 1] = 1;
                p->vy = -(SPRING_V + (was_pound ? 1.0f : 0));
                p->ground = 0;
                p->on_foe = -1;
                p->act = ACT_BOUNCE;
                dk_fx("dk_spring", p->x + 3, p->y + PH, C_PINK, 4);
                return;
            }
    }
    if (was_pound) {
        if (thump(p->x + 3, p->y + PH, p)) return;
        p->vy = -BOUNCE_V;
        p->ground = 0;
        p->on_foe = -1;
        p->act = ACT_BOUNCE;
        /* steer the bounce with whichever side is held */
        if (p->held == SIDE_L || p->held == SIDE_R) p->vx = dirof(p->held) * fmaxf(WALK, fabsf(p->vx));
        else p->vx *= 0.5f;
        dk_fx("dk_pound", p->x + 3, p->y + PH, C_LIGHT, 5);
        return;
    }
    if (p->act == ACT_JUMP || p->act == ACT_BOUNCE) {
        /* land running: a fast landing with that side held keeps the sprint */
        int d = p->vx > 0 ? 1 : -1;
        if (fabsf(p->vx) > WALK + 0.3f && (p->held & (d > 0 ? SIDE_R : SIDE_L))) { p->act = ACT_SPRINT; p->act_t = 0; }
        else p->act = ACT_NONE;
        dk_fx("dk_land", p->x + 3, p->y + PH, 0, 0);
    }
    p->vy = 0;
}

/* move one axis at a time, a pixel at most per sub-step */
static void move_player(DKPlayer *p, int i) {
    float dx = p->vx, dy = p->vy;
    int n = (int)ceilf(fmaxf(fabsf(dx), fabsf(dy)));
    if (n < 1) n = 1;
    float sx = dx / n, sy = dy / n;
    bool was_ground = p->ground;
    p->ground = 0;
    int ride = -1;
    for (int k = 0; k < n; k++) {
        if (sx != 0) {
            if (!rect_solid(p->x + sx, p->y, PW, PH)) p->x += sx;
            else {
                /* step up a one-pixel lip */
                if (was_ground && !rect_solid(p->x + sx, p->y - 1, PW, PH)) { p->x += sx; p->y -= 1; }
                else { p->vx = 0; sx = 0; if (p->act == ACT_SLIDE || p->act == ACT_SPRINT) p->act = ACT_NONE; }
            }
        }
        if (sy != 0) {
            float ny = p->y + sy;
            if (sy > 0) {
                int fi = -1;
                if (rect_solid(p->x, ny, PW, PH)) {
                    p->y = floorf(ny);
                    while (rect_solid(p->x, p->y, PW, PH)) p->y -= 1;
                    land(p, i, -1);
                    if (!p->ground) return; /* bounced off a slam */
                    sy = 0;
                    break;
                }
                if (p->act != ACT_POUND && oneway_top(p->x, PW, p->y + PH, ny + PH, &fi)) {
                    float top = fi >= 0 ? W.foe[fi].y + 2 : (float)(tof(ny + PH - 0.01f) * TS);
                    p->y = top - PH;
                    land(p, i, fi);
                    if (!p->ground) return;
                    ride = fi;
                    sy = 0;
                    break;
                }
                p->y = ny;
            } else {
                if (rect_solid(p->x, ny, PW, PH)) { p->vy = 0; sy = 0; p->jump_hold = 0; }
                else p->y = ny;
            }
        }
    }
    /* standing still: is there still ground (or a ledge) under the feet? */
    if (!p->ground && p->vy >= 0) {
        int fi = -1;
        float b = p->y + PH;
        if (rect_solid(p->x, p->y + 1, PW, PH)) { p->ground = 1; p->on_foe = -1; }
        else if (oneway_top(p->x, PW, b - 0.5f, b + 0.5f, &fi) && p->act != ACT_POUND) {
            p->ground = 1;
            p->on_foe = (int8_t)fi;
        }
        if (p->ground && p->act == ACT_POUND) land(p, i, -1);
    }
    if (ride >= 0) p->on_foe = (int8_t)ride;
    if (!p->ground) p->on_foe = -1;
    /* resting on a gulper, a sitting eye or a curled beetle: remember which */
    if (p->ground && p->on_foe < 0)
        for (int k = 0; k < DK_FOES; k++) {
            const DKFoe *f = &W.foe[k];
            if (!foe_is_block(f)) continue;
            float fx, fy, fw, fh;
            foe_box(f, &fx, &fy, &fw, &fh);
            if (p->x < fx + fw && fx < p->x + PW && fabsf(p->y + PH - fy) < 1.01f) { p->on_foe = (int8_t)k; break; }
        }
}

static void player_input(DKPlayer *p, uint32_t pad) {
    uint8_t h = 0;
    if (pad & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)) h |= SIDE_L;
    if (pad & (BTN_A | BTN_B)) h |= SIDE_R;
    p->prev = p->held;
    p->held = p->stun > 0 ? 0 : h;
    uint8_t pr = p->held & ~p->prev, rl = p->prev & ~p->held;
    for (int s = 1; s <= 2; s++) {
        if (pr & s) { p->last_press_t[s] = p->press_t[s]; p->press_t[s] = W.t; }
        if (rl & s) p->release_t[s] = W.t;
    }
    if (p->held == 3) {
        if (pr == SIDE_L) p->first = SIDE_R;
        else if (pr == SIDE_R) p->first = SIDE_L;
        else if (pr == 3) p->first = 0;
    } else {
        p->first = p->held;
    }
}

static bool double_tap(const DKPlayer *p, int s) {
    return W.t - p->last_press_t[s] <= DTAP && p->release_t[s] > p->last_press_t[s] && p->release_t[s] < p->press_t[s];
}

static void player_step(int i, uint32_t pad) {
    DKPlayer *p = &W.P[i];
    if (!p->alive) return;
    player_input(p, pad);
    if (p->stun > 0) p->stun--;
    p->anim++;
    if (p->ride) {
        /* drifting down: no steering until the feet touch the ground */
        p->held = p->prev = 0;
        p->vx = 0;
        p->vy = 1.4f;
        move_player(p, i);
        if (p->ground) { p->ride = 0; dk_fx("dk_land", p->x + 3, p->y + PH, C_LIGHT, 4); }
        return;
    }
    uint8_t pr = p->held & ~p->prev;
    bool water = rect_wet(p->x + 2, p->y + 3, 3, 3);
    bool was_water = p->in_water;
    p->in_water = water;

    /* what the two sides mean this frame */
    int jump_d = 0, jump_trig = 0;
    bool low = false;
    for (int s = 1; s <= 2 && !jump_d; s++) {
        if (!(pr & s)) continue;
        int o = other(s);
        if (!(p->held & o)) continue;
        if (W.t - p->press_t[o] < SIMUL) {
            /* both at once: a low hop, towards the side tapped just before */
            low = true;
            if (pr == 3) {
                int32_t la = p->last_press_t[SIDE_L], lb = p->last_press_t[SIDE_R];
                jump_d = W.t - la <= 20 && la > lb ? -1 : W.t - lb <= 20 && lb > la ? 1 : p->face;
            } else {
                jump_d = dirof(o);
            }
        } else {
            /* one side held, the other pressed: jump towards the held side */
            jump_d = dirof(o);
            jump_trig = s;
        }
    }

    bool on_ground = p->ground || p->coyote > 0;
    if (water) {
        /* swimming: any jump is a stroke */
        if (jump_d) {
            p->vy = -W_STROKE;
            p->vx = jump_d * W_MAX;
            p->face = (int8_t)jump_d;
            p->act = ACT_NONE;
            p->ground = 0;
            dk_fx("dk_swim", p->x + 3, p->y, 0, 0);
        } else if (p->act == ACT_POUND) {
            p->vy = 2.2f;
        }
        int hs = p->held == 3 ? p->face : p->held ? dirof(p->held) : 0;
        if (p->held == SIDE_L || p->held == SIDE_R) p->face = (int8_t)hs;
        p->vx = fapproach(p->vx, hs * W_MAX, 0.08f);
        if (p->act != ACT_POUND) p->vy = fminf(p->vy + W_GRAV, W_FALL);
        p->coyote = 0;
    } else if (on_ground && jump_d && p->act != ACT_POUND) {
        start_jump(p, jump_d, low ? LOW_V : JUMP_V, low ? 0 : jump_trig);
    } else if (on_ground && p->ground) {
        /* on the ground: double taps slide (and sprint while held) */
        for (int s = 1; s <= 2; s++)
            if ((pr & s) && !(p->held & other(s)) && double_tap(p, s)) {
                p->act = ACT_SLIDE;
                p->act_t = SLIDE_T;
                p->face = (int8_t)dirof(s);
                p->vx = dirof(s) * SLIDE_V;
                dk_fx("dk_slide", p->x + 3, p->y + PH, C_LIGHT, 3);
            }
        int d = p->face;
        if (p->act == ACT_SLIDE) {
            p->vx = d * (SPRINT + (SLIDE_V - SPRINT) * p->act_t / (float)SLIDE_T);
            if (--p->act_t <= 0) {
                p->act = (p->held & (d > 0 ? SIDE_R : SIDE_L)) ? ACT_SPRINT : ACT_NONE;
                if (p->act == ACT_SPRINT) dk_fx("dk_sprint", p->x + 3, p->y + PH, 0, 0);
            }
        } else if (p->act == ACT_SPRINT) {
            int ds = p->vx >= 0 ? 1 : -1;
            if (p->held & (ds > 0 ? SIDE_R : SIDE_L)) { p->vx = fapproach(p->vx, ds * SPRINT, ACC_G); p->face = (int8_t)ds; }
            else p->act = ACT_NONE;
        }
        if (p->act == ACT_NONE || p->act == ACT_JUMP || p->act == ACT_BOUNCE) {
            p->act = ACT_NONE;
            int hs = p->held == 3 ? (p->first ? dirof(p->first) : p->face) : p->held ? dirof(p->held) : 0;
            if (hs) {
                p->face = (int8_t)hs;
                if (p->vx * hs > WALK) p->vx = fapproach(p->vx, hs * WALK, SLOW_G);
                else p->vx = fapproach(p->vx, hs * WALK, ACC_G);
            } else {
                p->vx = fapproach(p->vx, 0, DEC_G);
            }
        }
        p->coyote = (int16_t)(p->act == ACT_SPRINT ? COYOTE_SPRINT : COYOTE);
    } else {
        /* in the air */
        if (p->coyote > 0) p->coyote--;
        for (int s = 1; s <= 2; s++) {
            if (!(pr & s)) continue;
            if ((p->held & other(s)) && p->act != ACT_POUND) {
                /* one side held, the other pressed again: the slam */
                int d = dirof(other(s));
                p->act = ACT_POUND;
                p->vy = POUND_V;
                p->vx = d * fmaxf(POUND_VX, fminf(fabsf(p->vx), 2.0f));
                p->face = (int8_t)d;
                p->jump_hold = 0;
                p->pounded = 1;
                p->coyote = 0;
                dk_fx("dk_slam", p->x + 3, p->y, 0, 0);
                break;
            }
            if (!(p->held & other(s)) && double_tap(p, s) && !p->spun && p->act != ACT_POUND) {
                /* a double tap in the air: the somersault, which adds or takes speed */
                int d = dirof(s);
                p->vx = fclamp(p->vx + d * SPIN_VX, -SPIN_MAX, SPIN_MAX);
                p->vy = fminf(p->vy, -SPIN_LIFT);
                p->spun = 1;
                p->face = (int8_t)d;
                p->jump_hold = 0;
                if (p->act != ACT_JUMP) p->act = ACT_JUMP;
                dk_fx("dk_spin", p->x + 3, p->y + 4, 0, 0);
            }
        }
        if (p->act != ACT_POUND) {
            int hs = p->held == SIDE_L ? -1 : p->held == SIDE_R ? 1 : p->held == 3 ? p->face : 0;
            if (hs && p->vx * hs < WALK) p->vx = fminf(fmaxf(p->vx + hs * ACC_A, -fmaxf(WALK, -p->vx)), fmaxf(WALK, p->vx));
            float g = GRAV;
            if (p->act == ACT_JUMP && p->jump_hold > 0 && (p->held & p->jump_side) && p->vy < 0) { g = GRAV_HOLD; p->jump_hold--; }
            else p->jump_hold = 0;
            p->vy = fminf(p->vy + g, FALL_MAX);
            if (p->act == ACT_SLIDE || p->act == ACT_SPRINT) {
                /* ran off an edge: keep going, still a sprint for the coyote time */
                if (p->coyote <= 0) p->act = ACT_JUMP;
            }
        }
    }
    if (water && !was_water && p->vy > 1.5f) { p->vy = 1.5f; dk_fx("dk_splash", p->x + 3, p->y + PH, C_SKY, 6); if (p->act == ACT_POUND) p->act = ACT_NONE; }
    if (!water && was_water && p->vy < 0) { p->vy = -W_LEAP; p->act = ACT_JUMP; p->jump_hold = 0; }

    float oldb = p->y + PH;
    move_player(p, i);
    (void)oldb;
    if (p->act == ACT_JUMP && p->ground) p->act = ACT_NONE;
    faces_check(p);
    springs_touch(p);
}

/* ------------------------------------------------------------------ */
/* contacts                                                             */

static bool overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static void stomp_bounce(DKPlayer *p, float v) {
    p->vy = -v;
    p->act = ACT_BOUNCE;
    p->ground = 0;
    p->spun = 0;
    p->on_foe = -1;
}

static void contacts(int i) {
    DKPlayer *p = &W.P[i];
    if (!p->alive) return;
    float x = p->x, y = p->y;
    /* thorns: the lower part of the tile */
    int x0 = tof(x + 1), x1 = tof(x + PW - 1.01f), y0 = tof(y + 1), y1 = tof(y + PH - 0.01f);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (dk_tile(tx, ty) == '^' && y + PH > ty * TS + 4) { die(i); return; }
    bool falling = p->vy > 0;
    bool slam = p->act == ACT_POUND;
    for (int k = 0; k < DK_FOES; k++) {
        DKFoe *f = &W.foe[k];
        if (!f->alive) continue;
        float fx, fy, fw, fh;
        foe_box(f, &fx, &fy, &fw, &fh);
        /* an open gulper bites whoever stands on it */
        if (f->kind == F_GULPER && f->state == 1 && x + PW > fx + 3 && x < fx + fw - 3 && y + PH >= fy - 1 && y + PH <= fy + 2) { die(i); return; }
        if (!overlap(x, y, PW, PH, fx, fy, fw, fh)) continue;
        bool from_above = falling && y + PH <= fy + 6;
        switch (f->kind) {
        case F_PUFF: case F_CROW: break;
        case F_FROG:
            if (from_above && !(dk_nerf & NERF_FROGS)) { stomp_bounce(p, FROG_V + (slam ? 0.8f : 0)); f->state = 0; f->y = f->y0; dk_fx("dk_boing", fx + 5, fy, 0, 0); }
            break;
        case F_GULPER:
            if (f->state == 1 && y + PH <= fy + 2 && x + PW > fx + 3 && x < fx + fw - 3) { die(i); return; }
            break;
        case F_EYE:
            if ((f->state == 1 || f->state == 3) && f->vy > 0 && y > fy + 2) { die(i); return; }
            break;
        case F_BEETLE:
            if (slam && from_above && f->state != 1) {
                f->state = 1;
                f->vx = (p->vx >= 0 ? 1 : -1) * 2.6f;
                f->vy = -3.4f;
                stomp_bounce(p, STOMP_V);
                dk_fx("dk_knock", fx + 5, fy, C_TAN, 4);
            }
            break;
        default:
            if (slam && from_above) {
                f->alive = 0;
                stomp_bounce(p, STOMP_V);
                dk_fx("dk_stomp", fx + 5, fy + 5, C_WHITE, 8);
            } else {
                die(i);
                return;
            }
            break;
        }
    }
    for (int k = 0; k < DK_SHOTS; k++) {
        DKShot *s = &W.shot[k];
        if (!s->alive) continue;
        float r = s->kind == SH_BLAST ? 5 : s->kind == SH_SPEAR ? 2.5f : 3;
        float sw = s->kind == SH_SPEAR ? 5 : r;
        if (overlap(x + 1, y + 1, PW - 2, PH - 2, s->x - sw, s->y - r, 2 * sw, 2 * r)) { die(i); return; }
    }
    DKBoss *b = &W.boss;
    if (b->alive) {
        float bx, by, bw, bh;
        boss_box(&bx, &by, &bw, &bh);
        if (overlap(x, y, PW, PH, bx, by, bw, bh) && b->solid) {
            if (slam && falling && y + PH <= by + 10) {
                b->hp--;
                b->flash = 45;
                b->solid = 0;
                stomp_bounce(p, 4.0f);
                dk_fx("dk_bosshit", bx + bw / 2, by, C_WHITE, 12);
                if (b->hp <= 0) {
                    b->alive = 0;
                    W.boss_down = 1;
                    for (int k = 0; k < DK_SHOTS; k++) W.shot[k].alive = 0;
                    dk_fx("dk_bossdown", bx + bw / 2, by + bh / 2, C_YELLOW, 30);
                }
            } else {
                die(i);
                return;
            }
        }
    }
    /* two dusklings: landing on the other's head */
    if (W.nplayers == 2) {
        DKPlayer *q = &W.P[1 - i];
        float feet = y + PH;
        if (q->alive && falling && x < q->x + PW && q->x < x + PW && feet >= q->y && feet - p->vy <= q->y + 1.5f) {
            stomp_bounce(p, 3.2f);
            q->stun = STUN_T;
            W.head_bounces++;
            dk_fx("dk_boing", q->x + 3, q->y, 0, 0);
        }
    }
}

/* ------------------------------------------------------------------ */
/* one frame                                                            */

int dk_world_step(uint32_t pad1, uint32_t pad2) {
    int ev = EV_NONE;
    W.t++;
    for (int i = 0; i < DK_FOES; i++)
        if (W.foe[i].alive) foe_step(i);
    shots_step();
    boss_step();
    for (int i = 0; i < W.nplayers; i++) {
        DKPlayer *p = &W.P[i];
        if (!p->alive) {
            if (p->dead_t > 0 && --p->dead_t == 0) {
                bool other_alive = W.nplayers == 2 && W.P[1 - i].alive;
                if (other_alive) spawn_player(i);
                else { dk_room_reset(); return EV_NONE; }
            }
            continue;
        }
        player_step(i, i == 0 ? pad1 : pad2);
        contacts(i);
        if (!p->alive) continue;
        if (p->y > RH * TS + 12) {
            if (W.room == RM_M0) return EV_REBIRTH;
            die(i);
            continue;
        }
        float cx = p->x + PW / 2.0f, cy = p->y + PH / 2.0f;
        char c = dk_tile(tof(cx), tof(cy));
        if (c == '>') ev = EV_EXIT;
        else if (c >= '1' && c <= '3') ev = EV_WARP1 + (c - '1');
        else if (c == 'Q' || rect_has(p->x, p->y, PW, PH, 'Q')) ev = EV_EGG;
        if (ev) break;
    }
    dk_last_event = ev;
    return ev;
}

/* ------------------------------------------------------------------ */
/* the route finder: a best-first search over short button patterns on
 * the real rules. It is only used to write the route tests; the tests
 * then press the buttons it found. */

typedef struct {
    uint8_t script[64];
    uint8_t len, coast;
} Macro;

#define MAX_MACROS 96
static Macro macros[MAX_MACROS];
static int n_macros;

static void mac_begin(void) { macros[n_macros].len = 0; macros[n_macros].coast = 0; }
static void mac_add(int sides, int frames) {
    Macro *m = &macros[n_macros];
    for (int i = 0; i < frames && m->len < 64; i++) m->script[m->len++] = (uint8_t)sides;
}
static void mac_end(int coast) { macros[n_macros].coast = (uint8_t)coast; if (n_macros < MAX_MACROS - 1) n_macros++; }

static void build_macros(void) {
    n_macros = 0;
    for (int dd = 0; dd < 2; dd++) {
        int X = dd ? SIDE_R : SIDE_L, Y = other(X);
        static const int WALKS[3] = {6, 14, 30};
        for (int k = 0; k < 3; k++) { mac_begin(); mac_add(X, WALKS[k]); mac_end(0); }
        static const int HS[3] = {1, 7, 16};
        for (int h = 0; h < 3; h++) {
            int coasts[3] = {X, 0, Y};
            for (int c = 0; c < 3; c++) { mac_begin(); mac_add(X, 5); mac_add(X | Y, HS[h]); mac_end(coasts[c]); }
        }
        /* the low hop: a tap, then both at once */
        mac_begin(); mac_add(X, 2); mac_add(0, 2); mac_add(X | Y, 2); mac_end(X);
        /* sprint and jump */
        static const int RUN[2] = {10, 30};
        for (int r = 0; r < 2; r++)
            for (int h = 1; h < 3; h++) { mac_begin(); mac_add(X, 2); mac_add(0, 2); mac_add(X, RUN[r]); mac_add(X | Y, HS[h]); mac_end(X); }
        /* jump, then the somersault */
        static const int KS[2] = {4, 14};
        for (int h = 1; h < 3; h++)
            for (int k = 0; k < 2; k++) {
                mac_begin(); mac_add(X, 5); mac_add(X | Y, HS[h]); mac_add(0, KS[k]); mac_add(X, 2); mac_add(0, 2); mac_add(X, 2); mac_end(X);
            }
        for (int r = 0; r < 2; r++) {
            mac_begin(); mac_add(X, 2); mac_add(0, 2); mac_add(X, RUN[r]); mac_add(X | Y, 16); mac_add(0, 6); mac_add(X, 2); mac_add(0, 2); mac_add(X, 2); mac_end(X);
        }
        /* jump, then the slam */
        static const int PK[2] = {3, 12};
        for (int h = 1; h < 3; h++)
            for (int k = 0; k < 2; k++)
                for (int c = 0; c < 3; c++) {
                    int coast = c == 0 ? X : c == 1 ? 0 : Y;
                    mac_begin(); mac_add(X, 5); mac_add(X | Y, HS[h]); mac_add(X, PK[k]); mac_add(X | Y, 2); mac_add(coast == Y ? 0 : coast, 3); mac_end(coast);
                }
        /* slide, and sprint */
        mac_begin(); mac_add(X, 2); mac_add(0, 2); mac_add(X, 2); mac_end(0);
        mac_begin(); mac_add(X, 2); mac_add(0, 2); mac_add(X, 40); mac_end(0);
    }
    mac_begin(); mac_add(0, 10); mac_end(0);
    mac_begin(); mac_add(0, 40); mac_end(0);
}

static uint32_t sides_to_pad(int s) { return (s & SIDE_L ? BTN_LEFT : 0) | (s & SIDE_R ? BTN_A : 0); }

typedef struct {
    DKWorld w;
    int32_t parent;
    uint16_t macro, frames;
    float pri;
} Node;

static Node *nodes;
static int n_nodes, cap_nodes;
static int32_t *heap;
static int heap_n;
static uint64_t *seen;
#define SEEN_BITS 20

static void heap_push(int32_t id) {
    int i = heap_n++;
    heap[i] = id;
    while (i > 0) {
        int pa = (i - 1) / 2;
        if (nodes[heap[pa]].pri <= nodes[heap[i]].pri) break;
        int32_t t = heap[pa]; heap[pa] = heap[i]; heap[i] = t;
        i = pa;
    }
}
static int32_t heap_pop(void) {
    int32_t top = heap[0];
    heap[0] = heap[--heap_n];
    int i = 0;
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < heap_n && nodes[heap[l]].pri < nodes[heap[m]].pri) m = l;
        if (r < heap_n && nodes[heap[r]].pri < nodes[heap[m]].pri) m = r;
        if (m == i) break;
        int32_t t = heap[m]; heap[m] = heap[i]; heap[i] = t;
        i = m;
    }
    return top;
}

static uint64_t mix(uint64_t h, uint64_t v) {
    h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
    return h * 0xff51afd7ed558ccdull;
}

static uint64_t state_key(const DKWorld *w) {
    const DKPlayer *p = &w->P[0];
    uint64_t h = 1469598103934665603ull;
    h = mix(h, (uint64_t)(int)floorf(p->x / 3));
    h = mix(h, (uint64_t)(int)floorf(p->y / 3));
    h = mix(h, (uint64_t)(int)lroundf(p->vx * 2));
    h = mix(h, (uint64_t)(p->ground | p->act << 1 | p->in_water << 4));
    if (has_clock) {
        /* time only matters near something that keeps time */
        bool near = w->boss.alive;
        for (int i = 0; i < DK_FOES && !near; i++) {
            const DKFoe *f = &w->foe[i];
            if (!f->alive) continue;
            if (f->kind != F_WASP && f->kind != F_WASPV && f->kind != F_GULPER && f->kind != F_NEWT && f->kind != F_FISH && f->kind != F_PRICKLE) continue;
            if (fabsf(f->x - p->x) < 150 && fabsf(f->y - p->y) < 120) near = true;
        }
        for (int i = 0; i < DK_SHOTS && !near; i++) near = w->shot[i].alive;
        if (near) h = mix(h, (uint64_t)((w->t / 15) % 40) + 77);
    }
    for (int i = 0; i < n_mut; i++) if (w->mut[i]) h = mix(h, (uint64_t)i + 1000);
    for (int i = 0; i < DK_FOES; i++) {
        const DKFoe *f = &w->foe[i];
        if (f->kind == F_EYE || f->kind == F_BEETLE) h = mix(h, (uint64_t)(f->alive | f->state << 1) + (uint64_t)((int)f->x / 5) * 16 + (uint64_t)((int)f->y / 5) * 4096);
        else h = mix(h, f->alive);
    }
    h = mix(h, (uint64_t)(w->boss.hp + 7 * w->boss.alive + 100 * w->boss_down));
    if (w->boss.alive) h = mix(h, (uint64_t)((int)w->boss.x / 16 + 64 * w->boss.state));
    return h | 1;
}

static bool seen_add(uint64_t k) {
    uint32_t mask = (1u << SEEN_BITS) - 1;
    uint32_t i = (uint32_t)(k >> 20) & mask;
    for (int probe = 0; probe < 64; probe++) {
        uint64_t v = seen[(i + probe) & mask];
        if (v == k) return false;
        if (v == 0) { seen[(i + probe) & mask] = k; return true; }
    }
    return true;
}

static int tgt_n;
static int16_t tgt_x[256], tgt_y[256];
static int solve_target;

static float heur(const DKWorld *w) {
    const DKPlayer *p = &w->P[0];
    float cx = p->x + PW / 2.0f, cy = p->y + PH / 2.0f;
    float best = 1e9f;
    for (int i = 0; i < tgt_n; i++) {
        float d = fabsf(cx - (tgt_x[i] * TS + 5)) + 1.3f * fabsf(cy - (tgt_y[i] * TS + 5));
        if (d < best) best = d;
    }
    if (w->boss.alive) {
        float d = fabsf(cx - (w->boss.x + 10)) + fabsf(cy - w->boss.y);
        best = 2000.0f * w->boss.hp + d;
    }
    return best;
}

static int tile_tx = -1, tile_ty = -1;
void dk_solve_tile(int tx, int ty) { tile_tx = tx; tile_ty = ty; }

static int want_event(void) {
    switch (solve_target) {
    case TG_WARP1: return EV_WARP1;
    case TG_WARP2: return EV_WARP2;
    case TG_WARP3: return EV_WARP3;
    case TG_EGG: return EV_EGG;
    default: return EV_EXIT;
    }
}

/* play a macro from the world as it is; returns the event, or -1 on a death */
static int run_macro(int mi, int *frames_out) {
    const Macro *m = &macros[mi];
    int f = 0, ev = EV_NONE, room = dk_w.room;
    int cap = m->len + 160;
    for (; f < cap; f++) {
        int s = f < m->len ? m->script[f] : m->coast;
        ev = dk_world_step(sides_to_pad(s), 0);
        if (tile_tx >= 0 && dk_w.P[0].alive && tof(dk_w.P[0].x + PW / 2.0f) == tile_tx && tof(dk_w.P[0].y + PH / 2.0f) == tile_ty) { ev = 99; f++; break; }
        if (ev != EV_NONE || dk_w.room != room) { f++; break; }
        if (!dk_w.P[0].alive) { *frames_out = f + 1; return -1; }
        if (f + 1 >= m->len) {
            const DKPlayer *p = &dk_w.P[0];
            if (p->ground && p->act != ACT_SLIDE) { f++; break; }
            if (p->in_water && f + 1 >= m->len + 24) { f++; break; }
        }
    }
    *frames_out = f;
    return ev;
}

static void print_path(int32_t id, FILE *out) {
    /* collect the chain, then replay it to print every frame's buttons */
    int32_t chain[4096];
    int n = 0;
    for (int32_t k = id; k > 0 && n < 4096; k = nodes[k].parent) chain[n++] = k;
    int last = -1, run = 0, total = 0;
    for (int c = n - 1; c >= 0; c--) {
        const Node *nd = &nodes[chain[c]];
        const Macro *m = &macros[nd->macro];
        for (int f = 0; f < nd->frames; f++) {
            int s = f < m->len ? m->script[f] : m->coast;
            if (s != last && run > 0) {
                if (last == 0) fprintf(out, "wait %d\n", run);
                else fprintf(out, "hold %s %d\n", last == 3 ? "LEFT+A" : last == SIDE_L ? "LEFT" : "A", run);
                run = 0;
            }
            last = s;
            run++;
            total++;
        }
    }
    if (run > 0) {
        if (last == 0) fprintf(out, "wait %d\n", run);
        else fprintf(out, "hold %s %d\n", last == 3 ? "LEFT+A" : last == SIDE_L ? "LEFT" : "A", run);
    }
    fprintf(out, "# %d frames\n", total);
}

int dk_solve(int room, int arrive, int target, int max_nodes, FILE *out) {
    build_macros();
    dk_room_load(room, arrive, 1);
    solve_target = target;
    tgt_n = 0;
    char want = target == TG_EXIT ? '>' : target == TG_EGG ? 'Q' : (char)('1' + target - TG_WARP1);
    if (tile_tx >= 0) { tgt_x[0] = (int16_t)tile_tx; tgt_y[0] = (int16_t)tile_ty; tgt_n = 1; }
    else
        for (int ty = 0; ty < RH; ty++)
            for (int tx = 0; tx < RW; tx++)
                if (map[ty][tx] == want && tgt_n < 256) { tgt_x[tgt_n] = (int16_t)tx; tgt_y[tgt_n] = (int16_t)ty; tgt_n++; }
    if (target == TG_EXIT && tgt_n == 0) {
        /* an open edge: the right-hand side, or the top */
        for (int ty = 0; ty < RH && tgt_n < 256; ty++) { tgt_x[tgt_n] = (int16_t)RW; tgt_y[tgt_n] = (int16_t)ty; tgt_n++; }
    }
    cap_nodes = max_nodes;
    nodes = (Node *)malloc(sizeof(Node) * (size_t)cap_nodes);
    heap = (int32_t *)malloc(sizeof(int32_t) * (size_t)cap_nodes * 2);
    seen = (uint64_t *)calloc((size_t)1 << SEEN_BITS, sizeof(uint64_t));
    if (!nodes || !heap || !seen) { free(nodes); free(heap); free(seen); return 0; }
    dk_sim_quiet = true;
    n_nodes = 1;
    heap_n = 0;
    nodes[0].w = dk_w;
    nodes[0].parent = -1;
    nodes[0].pri = 0;
    seen_add(state_key(&dk_w));
    heap_push(0);
    int found = -1, expanded = 0;
    int ev_want = tile_tx >= 0 ? 99 : want_event();
    while (heap_n > 0 && found < 0) {
        int32_t cur = heap_pop();
        expanded++;
        for (int mi = 0; mi < n_macros && found < 0; mi++) {
            dk_w = nodes[cur].w;
            int frames = 0;
            int ev = run_macro(mi, &frames);
            if (ev < 0) continue;
            if (ev != EV_NONE && ev != ev_want) continue;
            if (n_nodes >= cap_nodes) break;
            if (ev == EV_NONE && !seen_add(state_key(&dk_w))) continue;
            Node *nd = &nodes[n_nodes];
            nd->w = dk_w;
            nd->parent = cur;
            nd->macro = (uint16_t)mi;
            nd->frames = (uint16_t)frames;
            float depth = 0;
            for (int32_t k = cur; k > 0; k = nodes[k].parent) depth += nodes[k].frames;
            nd->pri = heur(&dk_w) + (depth + frames) * 0.08f;
            if (ev == ev_want) found = n_nodes;
            n_nodes++;
            if (found < 0) heap_push(n_nodes - 1);
        }
        if (n_nodes >= cap_nodes) break;
    }
    dk_sim_quiet = false;
    tile_tx = tile_ty = -1;
    int ok = found >= 0;
    dk_solve_ok = ok;
    if (ok) print_path(found, out);
    else fprintf(out, "# no route found (%d nodes, %d expanded)\n", n_nodes, expanded);
    free(nodes);
    free(heap);
    free(seen);
    nodes = NULL;
    dk_room_load(room, arrive, 1);
    return ok;
}
