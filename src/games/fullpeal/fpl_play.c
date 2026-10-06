/* FULL PEAL - the rules of flight: the ship on its plane, the forward gun
 * and the side blaster, every foe, the shots on the plane and in depth,
 * and the wave runner that releases each formation and grades the wave. */
#include "fpl.h"
#include <stddef.h>

FplGame fpg;

/* name, hit points (0: can't be hurt), depth a frame on the way in,
 * counted toward the grade, colour */
const FplFoeDef FPL_FOE[EK_COUNT] = {
    {"CLAPPER", 1, 1.0f / 150, 1, C_RED},
    {"TENOR", 3, 1.0f / 150, 1, C_JADE},
    {"TREBLE", 1, 1.0f / 62, 1, C_LIGHT},
    {"DODGER", 2, 1.0f / 230, 1, C_SKY},
    {"BOURDON", 0, 1.0f / 120, 0, C_GREY},
    {"CROSSHEAD", 2, 1.0f / 110, 1, C_VIOLET},
    {"FORKER", 2, 1.0f / 110, 1, C_BLUE},
    {"PENDULUM", 1, 1.0f / 105, 1, C_YELLOW},
    {"SPITE", 1, 1.0f / 105, 1, C_AMBER},
    {"SALLY", 1, 1.0f / 170, 1, C_CYAN},
    {"QUICKSALLY", 1, 1.0f / 95, 1, C_LEAF},
    {"LOOKOUT", 2, 0, 1, C_ORANGE},
    {"MOTE", 1, 0, 1, C_PINK},
    {"NIBBLER", 2, 0, 1, C_MAGENTA},
    {"CALTROP", 0, 1.0f / 90, 0, C_SLATE},
    {"BROODER", 2, 1.0f / 110, 1, C_LIME},
    {"WISP", 1, 0, 0, C_PINK},
    {"FLARE", 1, 0, 0, C_ORANGE},
};

#define HOLD_T 240            /* a clapper sits on the plane this long */
#define FAR_HOLD_Z 0.55f      /* the gunners keep this far off */
#define FAR_HOLD_T 420
#define WARN_T 45             /* the edge arrow before something comes in from a side */

static float foe_r(int kind) {
    switch (kind) {
    case EK_BOURDON: return 0.46f;
    case EK_MOTE: case EK_WISP: case EK_FLARE: return 0.24f;
    case EK_CALTROP: return 0.34f;
    default: return 0.3f;
    }
}

void fpl_sfx(const char *name, int gap) {
    static int last_t[8];
    static const char *last_n[8];
    int slot = 0;
    for (int i = 0; i < 8; i++) {
        if (last_n[i] == name) { slot = i; goto found; }
        if (!last_n[i]) slot = i;
    }
    last_n[slot] = name;
    last_t[slot] = -1000;
found:
    if (fpg.frame_t - last_t[slot] < gap && fpg.frame_t >= last_t[slot]) return;
    last_t[slot] = fpg.frame_t;
    sfx_play_name(name);
}

void fpl_radio(const char *s) {
    fpg.radio = s;
    fpg.radio_t = 0;
}

/* ------------------------------------------------------------------ */
/* the run                                                              */

void fpl_clear_world(void) {
    memset(fpg.foe, 0, sizeof fpg.foe);
    memset(fpg.es, 0, sizeof fpg.es);
    memset(fpg.ps, 0, sizeof fpg.ps);
}

static void ship_spawn(void) {
    fpg.alive = true;
    fpg.x = 0;
    fpg.y = 0.5f;
    fpg.inv = FPL_INV_T;
    fpg.dead_t = 0;
    fpg.fwd_cd = fpg.side_cd = 0;
    fpg.side_locked = false;
}

void fpl_new_run(bool hugs) {
    bool god = fpg.god, sandbox = fpg.sandbox;
    int ft = fpg.frame_t;
    memset(&fpg.stage, 0, sizeof fpg - offsetof(FplGame, stage));
    fpg.god = god;
    fpg.sandbox = sandbox;
    fpg.frame_t = ft;
    fpg.hugs = hugs;
    fpg.lives = FPL_LIVES;
    fpg.continues = FPL_CONTINUES;
    for (int s = 0; s < FPL_STAGES; s++)
        for (int w = 0; w < FPL_WAVES; w++) fpg.grade[s][w] = -1;
    fpg.face_x = 0;
    fpg.face_y = 1;
    fpg.side_dx = 1;
    fpg.side_dy = 0;
    ship_spawn();
}

void fpl_start_stage(int s) {
    fpg.stage = iclamp(s, 0, FPL_STAGES - 1);
    for (int w = 0; w < FPL_WAVES; w++) fpg.grade[fpg.stage][w] = -1;
    fpg.perfect_stage = 0;
    fpg.bonus_due = false;
    fpg.wave = 0;
    fpg.stage_t = 0;
    fpg.owl = fpg.meta = false;
    fpl_clear_world();
    memset(&fpg.boss, 0, sizeof fpg.boss);
    if (!fpg.alive && fpg.lives > 0) ship_spawn();
    fpg.form_i = FPL_WAVES * 99; /* nothing runs until a wave starts */
}

void fpl_start_wave(int w) {
    fpg.wave = iclamp(w, 0, FPL_WAVES - 1);
    fpg.form_i = 0;
    fpg.form_t = 0;
    fpg.form_gap = 20;
    fpg.spawn_i = 0;
    fpg.wave_total = fpl_wave_total(fpg.stage, fpg.wave);
    fpg.wave_kills = fpg.wave_escaped = fpg.wave_spawned = 0;
    fpg.owl = fpg.meta = false;
}

int fpl_wave_grade(void) {
    if (fpg.wave_total <= 0) return 100;
    return fpg.wave_kills * 100 / fpg.wave_total;
}

int fpl_count_foes(int kind) {
    int n = 0;
    for (int i = 0; i < FPL_MAX_FOES; i++) n += fpg.foe[i].alive && (kind < 0 || fpg.foe[i].kind == kind);
    return n;
}

static int form_alive(int tag) {
    int n = 0;
    for (int i = 0; i < FPL_MAX_FOES; i++) n += fpg.foe[i].alive && fpg.foe[i].form == tag;
    return n;
}

bool fpl_wave_done(void) {
    const FplWave *wv = &FPL_STAGE[fpg.stage].wave[fpg.wave];
    return fpg.form_i >= wv->n && fpl_count_foes(-1) == 0;
}

/* release the wave's formations, one after another: the next comes a
 * moment after every foe of the last is gone (shot down or flown off) */
static void run_wave(void) {
    if (fpg.sandbox) return;
    const FplWave *wv = &FPL_STAGE[fpg.stage].wave[fpg.wave];
    if (fpg.form_i >= wv->n) return;
    if (fpg.form_gap > 0) { fpg.form_gap--; return; }
    const FplForm *f = &FPL_FORMS[wv->forms[fpg.form_i]];
    int tag = fpg.form_i + 1;
    while (fpg.spawn_i < f->n && f->s[fpg.spawn_i].t <= fpg.form_t) {
        const FplSpawn *sp = &f->s[fpg.spawn_i];
        int i = fpl_spawn_foe(sp->kind, sp->col, sp->row, sp->arg);
        if (i >= 0) fpg.foe[i].form = (uint8_t)tag;
        fpg.wave_spawned++;
        fpg.spawn_i++;
    }
    fpg.form_t++;
    if (fpg.spawn_i >= f->n && !form_alive(tag)) {
        fpg.form_i++;
        fpg.spawn_i = 0;
        fpg.form_t = 0;
        fpg.form_gap = FPL_FORM_GAP;
    }
}

/* ------------------------------------------------------------------ */
/* shots                                                                */

int fpl_add_eshot(int kind, float x, float y, float z, float vx, float vy, float vz, int burst) {
    for (int i = 0; i < FPL_MAX_ESHOTS; i++) {
        FplEShot *e = &fpg.es[i];
        if (e->alive) continue;
        memset(e, 0, sizeof *e);
        e->alive = 1;
        e->kind = (uint8_t)kind;
        e->burst = (uint8_t)burst;
        e->x = x; e->y = y; e->z = z;
        e->vx = vx; e->vy = vy; e->vz = vz;
        e->life = 600;
        return i;
    }
    return -1;
}

/* a shell from a foe in depth to a spot on the plane, arriving in so many frames */
void fpl_shell_at(float x, float y, float z, float tx, float ty, int frames, int burst) {
    if (frames < 1) frames = 1;
    fpl_add_eshot(ES_SHELL, x, y, z, (tx - x) / frames, (ty - y) / frames, -z / frames, burst);
}

static void plane_shot(float x, float y, float vx, float vy, float spin) {
    int i = fpl_add_eshot(ES_PLANE, x, y, 0, vx, vy, 0, BURST_NONE);
    if (i >= 0) fpg.es[i].spin = spin;
}

static void burst_at(float x, float y, int burst) {
    const float v = 0.034f;
    switch (burst) {
    case BURST_CROSS:
        plane_shot(x, y, v, 0, 0); plane_shot(x, y, -v, 0, 0);
        plane_shot(x, y, 0, v, 0); plane_shot(x, y, 0, -v, 0);
        break;
    case BURST_DIAG:
        plane_shot(x, y, v * 0.7f, v * 0.7f, 0); plane_shot(x, y, -v * 0.7f, v * 0.7f, 0);
        plane_shot(x, y, v * 0.7f, -v * 0.7f, 0); plane_shot(x, y, -v * 0.7f, -v * 0.7f, 0);
        break;
    case BURST_FORK:
        plane_shot(x, y, 0, v, 0); plane_shot(x, y, 0, -v, 0);
        break;
    case BURST_TRI:
        for (int k = 0; k < 3; k++) {
            float a = (float)k * 2.0943951f + (float)(fpg.frame_t % 7) * 0.3f;
            plane_shot(x, y, cosf(a) * 0.028f, sinf(a) * 0.028f, 0.035f);
        }
        break;
    default: break;
    }
}

void fpl_burst(float x, float y, float z, int col, int n) {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < FPL_MAX_PARTS; i++) {
            FplPart *p = &fpg.part[i];
            if (p->life > 0) continue;
            float a = rng_float(&g_rng) * 6.2831853f, sp = 0.02f + rng_float(&g_rng) * 0.05f;
            p->x = x; p->y = y; p->z = z;
            p->vx = cosf(a) * sp;
            p->vy = sinf(a) * sp;
            p->vz = (rng_float(&g_rng) - 0.5f) * 0.01f;
            p->life = 14 + rng_range(&g_rng, 0, 14);
            p->col = k % 3 == 0 ? C_WHITE : col;
            break;
        }
    }
}

bool fpl_ship_hit_circle(float x, float y, float r) {
    if (!fpg.alive || fpg.inv > 0 || fpg.god) return false; /* god (tests): things pass through */
    float dx = x - fpg.x, dy = y - fpg.y;
    return dx * dx + dy * dy < (r + FPL_SHIP_R) * (r + FPL_SHIP_R);
}

void fpl_lose_ship(int cause) {
    if (!fpg.alive || fpg.god) return;
    fpg.alive = false;
    fpg.cause = cause;
    fpg.lives--;
    fpg.deaths++;
    fpg.dead_t = FPL_DEAD_T;
    fpg.shake = 14;
    fpl_burst(fpg.x, fpg.y, 0, C_ORANGE, 24);
    fpl_sfx("fpl_crash", 0);
}

/* ------------------------------------------------------------------ */
/* foes                                                                 */

int fpl_spawn_foe(int kind, int col, int row, int arg) {
    int slot = -1;
    for (int i = 0; i < FPL_MAX_FOES; i++)
        if (!fpg.foe[i].alive) { slot = i; break; }
    if (slot < 0) return -1;
    FplFoe *f = &fpg.foe[slot];
    memset(f, 0, sizeof *f);
    f->alive = 1;
    f->kind = (uint8_t)kind;
    f->counted = FPL_FOE[kind].counted;
    f->hp = FPL_FOE[kind].hp;
    f->arg = arg;
    f->state = FS_IN;
    int c = iclamp(col, 0, 5), r = iclamp(row, 0, 3);
    f->x = fpl_lane_x(c);
    f->y = fpl_lane_y(r);
    f->z = 1.0f;
    f->vz = -FPL_FOE[kind].vz;
    f->dir = (col < 0 || col > 5) ? (col < 0 ? 1 : -1) : (arg & 1 ? -1 : 1);
    switch (kind) {
    case EK_CLAPPER: case EK_TENOR: case EK_TREBLE: case EK_DODGER:
        if (col < 0 || col > 5) {
            /* a crosser: on the plane at one end, after a warning */
            f->x = col < 0 ? -3.5f : 3.5f;
            f->z = 0;
            f->state = FS_HOLD;
            f->t = -WARN_T;
            f->arg = 9;
        }
        break;
    case EK_LOOKOUT:
        f->x = col < 0 ? -3.5f : 3.5f;
        f->z = 0;
        f->t = -WARN_T;
        f->state = FS_IN;
        break;
    case EK_BOURDON:
        /* in from a side, in depth, to the lane arg (its outline marks it) */
        f->tx = fpl_lane_x(iclamp(arg, 0, 5));
        f->ty = fpl_lane_y(r);
        f->x = col < 0 ? -5.5f : 5.5f;
        f->z = 0.5f;
        f->t = -WARN_T;
        break;
    case EK_PENDULUM: case EK_SPITE:
        f->vy = (arg & 2) ? -0.045f : 0.045f;
        break;
    case EK_MOTE: case EK_NIBBLER:
        f->z = 0;
        f->state = FS_HOLD;
        f->t = -30; /* it shimmers into being, harmless, first */
        break;
    case EK_WISP:
        f->x = col < 0 ? -3.5f : 3.5f;
        f->z = 0;
        f->state = FS_HOLD;
        f->t = -WARN_T / 2;
        f->ty = f->y;
        break;
    case EK_FLARE:
        f->y = -2.4f;
        f->z = 0;
        f->state = FS_HOLD;
        f->t = -WARN_T;
        break;
    default: break;
    }
    return slot;
}

void fpl_kill_foe(int i, bool shot) {
    FplFoe *f = &fpg.foe[i];
    if (!f->alive) return;
    f->alive = 0;
    if (shot) {
        fpl_burst(f->x, f->y, f->z, FPL_FOE[f->kind].col, 10);
        fpl_sfx("fpl_pop", 3);
        if (f->counted) fpg.wave_kills++;
        if (f->kind == EK_SPITE) {
            /* its spite: a cross, on the plane where it is or where it was heading */
            if (f->z <= FPL_PLANE_Z) burst_at(f->x, f->y, BURST_CROSS);
            else fpl_shell_at(f->x, f->y, f->z, f->x, f->y, 40, BURST_CROSS);
        }
    } else if (f->counted) {
        fpg.wave_escaped++;
    }
}

void fpl_hurt_foe(int i, int dmg) {
    FplFoe *f = &fpg.foe[i];
    if (!f->alive || f->hp <= 0) return;
    f->hp -= dmg;
    f->flash = 6;
    if (f->hp <= 0) { fpl_kill_foe(i, true); return; }
    fpl_sfx("fpl_tick", 2);
    if (f->kind == EK_DODGER) {
        /* it slips a lane to the side */
        int c = fpl_col(f->x) + f->dir;
        if (c < 0 || c > 5) { f->dir = -f->dir; c = fpl_col(f->x) + f->dir; }
        f->tx = fpl_lane_x(c);
        f->sub = 1;
    }
}

static void foe_leave(FplFoe *f) {
    f->state = FS_OUT;
    f->t = 0;
}

static void foe_gone(int i) { fpl_kill_foe(i, false); }

static void foe_update(int i) {
    FplFoe *f = &fpg.foe[i];
    int k = f->kind;
    f->t++;
    if (f->flash > 0) f->flash--;
    if (fpg.still) return;
    switch (k) {
    case EK_CLAPPER: case EK_TENOR: case EK_TREBLE: case EK_DODGER:
        if (f->state == FS_HOLD && f->arg == 9) {
            /* a crosser goes end to end along its row */
            if (f->t < 0) break;
            f->x += f->dir * 0.032f;
            if ((f->dir > 0 && f->x > 3.6f) || (f->dir < 0 && f->x < -3.6f)) foe_gone(i);
            break;
        }
        if (k == EK_DODGER && f->sub) {
            f->x = fapproach(f->x, f->tx, 0.06f);
            if (f->x == f->tx) f->sub = 0;
        }
        if (f->state == FS_IN) {
            f->z += f->vz;
            if (f->z <= 0) { f->z = 0; f->state = FS_HOLD; f->t = 0; f->ty = f->y; }
        } else if (f->state == FS_HOLD) {
            f->y = f->ty + sinf(f->t * 0.08f) * 0.05f;
            if (f->t >= HOLD_T) foe_leave(f);
        } else {
            f->z -= 0.02f;
            if (f->z < -0.14f) foe_gone(i);
        }
        break;
    case EK_PENDULUM: case EK_SPITE:
        f->y += f->vy;
        if (f->y > 1.5f) { f->y = 1.5f; f->vy = -f->vy; }
        if (f->y < -1.5f) { f->y = -1.5f; f->vy = -f->vy; }
        if (f->state == FS_IN) {
            f->z += f->vz;
            if (f->z <= 0) { f->z = 0; f->state = FS_HOLD; f->t = 0; }
        } else if (f->state == FS_HOLD) {
            if (f->t >= 150) foe_leave(f);
        } else {
            f->z -= 0.02f;
            if (f->z < -0.14f) foe_gone(i);
        }
        break;
    case EK_CROSSHEAD: case EK_FORKER: case EK_BROODER:
        if (f->state == FS_IN) {
            f->z += f->vz;
            if (f->z <= FAR_HOLD_Z) { f->z = FAR_HOLD_Z; f->state = FS_HOLD; f->t = 0; f->tx = f->x; f->ty = f->y; f->fire_t = 70; }
        } else if (f->state == FS_HOLD) {
            f->x = f->tx + sinf(f->t * 0.03f) * 0.12f;
            if (--f->fire_t <= 0 && fpg.alive) {
                if (k == EK_BROODER) {
                    /* a mote spat at where the ship is */
                    fpl_shell_at(f->x, f->y, f->z, fpg.x, fpg.y, 75, BURST_NONE);
                    f->fire_t = 80;
                } else {
                    fpl_shell_at(f->x, f->y, f->z, fpg.x, fpg.y, 60, k == EK_CROSSHEAD ? BURST_CROSS : BURST_FORK);
                    f->fire_t = 150;
                }
                fpl_sfx("fpl_efire", 6);
            }
            if (f->t >= FAR_HOLD_T) foe_leave(f);
        } else {
            f->z += 0.012f;
            if (f->z > 1.15f) foe_gone(i);
        }
        break;
    case EK_SALLY: case EK_QUICKSALLY: {
        float home = k == EK_SALLY ? 0.006f : 0.009f;
        if (fpg.alive) {
            f->x = fapproach(f->x, fpg.x, home);
            f->y = fapproach(f->y, fpg.y, home);
        }
        if (k == EK_QUICKSALLY) f->x += sinf(f->t * 0.25f) * 0.03f;
        f->z += f->vz;
        if (f->z <= 0) {
            /* it reaches the plane and bursts in three */
            if (fpl_ship_hit_circle(f->x, f->y, 0.3f)) fpl_lose_ship(CAUSE_FOE + k);
            burst_at(f->x, f->y, BURST_TRI);
            fpl_burst(f->x, f->y, 0, C_CYAN, 6);
            fpl_sfx("fpl_efire", 4);
            foe_gone(i);
        }
        break;
    }
    case EK_LOOKOUT: {
        float edge = f->dir > 0 ? -2.9f : 2.9f; /* dir: the way it fires */
        if (f->t < 0) break;
        if (f->state == FS_IN) {
            f->x = fapproach(f->x, edge, 0.05f);
            if (f->x == edge) { f->state = FS_HOLD; f->t = 0; }
        } else if (f->state == FS_HOLD) {
            static const int SHOTS[6] = {30, 42, 54, 110, 122, 134};
            for (int s = 0; s < 6; s++)
                if (f->t == SHOTS[s]) { plane_shot(f->x, f->y, f->dir * 0.06f, 0, 0); fpl_sfx("fpl_efire", 3); }
            if (f->t >= 175) foe_leave(f);
        } else {
            f->x -= f->dir * 0.05f;
            if (f->x < -3.6f || f->x > 3.6f) foe_gone(i);
        }
        break;
    }
    case EK_MOTE:
        if (f->t < 0) break;
        if (f->sub <= 0) {
            /* straight at where the ship is now */
            f->tx = fpg.x;
            f->ty = fpg.y;
            float dx = f->tx - f->x, dy = f->ty - f->y, d = sqrtf(dx * dx + dy * dy);
            if (d < 0.01f) d = 0.01f;
            f->vx = dx / d * 0.024f;
            f->vy = dy / d * 0.024f;
            f->sub = (int)(d / 0.024f) + 20;
        }
        if (f->sub > 20) { f->x += f->vx; f->y += f->vy; }
        f->sub--;
        if (f->t >= 380) foe_gone(i);
        break;
    case EK_NIBBLER: {
        if (f->t < 0) break;
        float dx = fpg.x - f->x, dy = fpg.y - f->y, d = sqrtf(dx * dx + dy * dy);
        if (d > 0.01f && fpg.alive) { f->vx += dx / d * 0.0022f; f->vy += dy / d * 0.0022f; }
        float sp = sqrtf(f->vx * f->vx + f->vy * f->vy);
        if (sp > 0.034f) { f->vx *= 0.034f / sp; f->vy *= 0.034f / sp; }
        f->x += f->vx;
        f->y += f->vy;
        if (f->t >= 420) foe_gone(i);
        break;
    }
    case EK_BOURDON:
        if (f->t < 0) break;
        if (f->state == FS_IN) {
            f->x = fapproach(f->x, f->tx, 0.07f);
            if (f->x == f->tx) {
                f->z += f->vz;
                if (f->z <= 0) { f->z = 0; f->state = FS_HOLD; f->t = 0; fpl_sfx("fpl_bong", 0); fpg.shake = 6; }
            }
        } else if (f->state == FS_HOLD) {
            if (f->t >= 170) foe_leave(f);
        } else {
            f->z += 0.012f;
            if (f->z > 1.1f) foe_gone(i);
        }
        break;
    case EK_CALTROP:
        f->z += f->vz;
        if (f->z < -0.15f) foe_gone(i);
        break;
    case EK_WISP:
        if (f->t < 0) break;
        f->x += f->dir * 0.03f;
        f->y = f->ty + sinf(f->t * 0.09f) * 0.3f;
        if (f->x < -3.6f || f->x > 3.6f) foe_gone(i);
        break;
    case EK_FLARE:
        if (f->t < 0) break;
        f->y += 0.03f;
        if (f->y > 2.4f) foe_gone(i);
        break;
    default: break;
    }
    if (!f->alive) return;
    /* touching the ship: anything on the plane (and not leaving or still arriving) */
    if (f->t >= 0 && f->state != FS_OUT && f->z <= FPL_PLANE_Z && f->z >= -0.04f) {
        if (fpl_ship_hit_circle(f->x, f->y, foe_r(k) * 0.85f)) fpl_lose_ship(CAUSE_FOE + k);
    }
}

void fpl_foes_update(void) {
    for (int i = 0; i < FPL_MAX_FOES; i++)
        if (fpg.foe[i].alive) foe_update(i);
}

/* ------------------------------------------------------------------ */
/* the ship and its guns                                                */

static int add_pshot(void) {
    for (int i = 0; i < FPL_MAX_PSHOTS; i++)
        if (!fpg.ps[i].alive) { memset(&fpg.ps[i], 0, sizeof fpg.ps[i]); fpg.ps[i].alive = 1; return i; }
    return -1;
}

static void fire_forward(void) {
    int i = add_pshot();
    if (i < 0) return;
    FplPShot *s = &fpg.ps[i];
    s->side = 0;
    s->x = fpg.x;
    s->y = fpg.y;
    s->z = 0;
    s->lane_c = fpl_col(fpg.x);
    s->lane_r = fpl_row(fpg.y);
    fpg.fired_fwd++;
    fpl_sfx("fpl_shot", 2);
}

static void fire_side(void) {
    int i = add_pshot();
    if (i < 0) return;
    FplPShot *s = &fpg.ps[i];
    s->side = 1;
    s->x = fpg.x + fpg.side_dx * 0.2f;
    s->y = fpg.y + fpg.side_dy * 0.2f;
    s->vx = fpg.side_dx * FPL_SIDE_V;
    s->vy = fpg.side_dy * FPL_SIDE_V;
    fpg.fired_side++;
    fpg.last_side = i;
    fpl_sfx("fpl_side", 2);
}

void fpl_ship_update(uint8_t in) {
    uint8_t pressed = in & ~fpg.prev_in;
    fpg.prev_in = in;
    if (fpg.inv > 0) fpg.inv--;
    if (!fpg.alive) {
        if (fpg.dead_t > 0) fpg.dead_t--;
        if (fpg.dead_t == 0 && fpg.lives > 0) ship_spawn();
        return;
    }
    int dx = ((in & BTN_RIGHT) ? 1 : 0) - ((in & BTN_LEFT) ? 1 : 0);
    int dy = ((in & BTN_DOWN) ? 1 : 0) - ((in & BTN_UP) ? 1 : 0);
    float sp = FPL_SHIP_SPEED * ((dx && dy) ? 0.7071f : 1.0f);
    fpg.x = fclamp(fpg.x + dx * sp, -FPL_SHIP_MX, FPL_SHIP_MX);
    fpg.y = fclamp(fpg.y + dy * sp, -FPL_SHIP_MY, FPL_SHIP_MY);
    if (dx || dy) {
        /* the last way it moved, one axis: the side across the screen wins */
        fpg.face_x = dx;
        fpg.face_y = dx ? 0 : dy;
    }
    if (fpg.fwd_cd > 0) fpg.fwd_cd--;
    if (fpg.side_cd > 0) fpg.side_cd--;
    if (fpg.hugs) return; /* HUGS-ONLY: no guns at all */
    if (in & BTN_A) {
        if ((pressed & BTN_A) || !fpg.side_locked) {
            /* the side blaster points away from the way the ship is moving,
             * and keeps that direction for as long as A stays down */
            if (dx) { fpg.side_dx = -dx; fpg.side_dy = 0; }
            else if (dy) { fpg.side_dx = 0; fpg.side_dy = -dy; }
            else if (fpg.face_x || fpg.face_y) { fpg.side_dx = -fpg.face_x; fpg.side_dy = -fpg.face_y; }
            fpg.side_locked = true;
            fpg.side_cd = 0;
        }
        if (fpg.side_cd == 0) { fire_side(); fpg.side_cd = FPL_SIDE_CD; }
    } else {
        fpg.side_locked = false;
        /* both buttons: the side blaster wins */
        if (in & BTN_B) {
            if (pressed & BTN_B) fpg.fwd_cd = 0;
            if (fpg.fwd_cd == 0) { fire_forward(); fpg.fwd_cd = FPL_FWD_CD; }
        }
    }
}

void fpl_bonus_shot(FplPShot *s, float z0); /* fpl_bonus.c */

void fpl_shots_update(void) {
    for (int i = 0; i < FPL_MAX_PSHOTS; i++) {
        FplPShot *s = &fpg.ps[i];
        if (!s->alive) continue;
        s->t++;
        if (s->side) {
            s->x += s->vx;
            s->y += s->vy;
            if (s->x < -3.4f || s->x > 3.4f || s->y < -2.4f || s->y > 2.4f) { s->alive = 0; continue; }
            for (int k = 0; k < FPL_MAX_FOES && s->alive; k++) {
                FplFoe *f = &fpg.foe[k];
                if (!f->alive || f->t < 0 || f->z > FPL_PLANE_Z || f->state == FS_OUT) continue;
                float dx = f->x - s->x, dy = f->y - s->y, r = FPL_SIDE_HIT + foe_r(f->kind) * 0.5f;
                if (dx * dx + dy * dy < r * r) {
                    s->alive = 0;
                    if (FPL_FOE[f->kind].hp == 0) { fpl_sfx("fpl_tink", 3); continue; }
                    fpl_hurt_foe(k, 1);
                }
            }
            if (s->alive && fpg.boss.on && fpl_boss_side_shot(s)) s->alive = 0;
            if (s->alive && (fpg.state == PS_BONUS)) fpl_bonus_shot(s, 0);
            continue;
        }
        /* forward: into the distance down its lane */
        float z0 = s->z;
        s->z += FPL_FWD_VZ;
        float lx = fpl_lane_x(s->lane_c), ly = fpl_lane_y(s->lane_r);
        float a = fclamp(s->z * 6.0f, 0, 1);
        s->x = s->x + (lx - s->x) * a * 0.5f;
        s->y = s->y + (ly - s->y) * a * 0.5f;
        if (s->z > 1.05f) { s->alive = 0; continue; }
        /* the nearest foe in the lane between the last step and this one */
        int best = -1;
        float bz = 9;
        for (int k = 0; k < FPL_MAX_FOES; k++) {
            FplFoe *f = &fpg.foe[k];
            if (!f->alive || f->t < 0 || f->z <= FPL_PLANE_Z) continue;
            if (f->z < z0 - 0.02f || f->z > s->z + 0.02f) continue;
            if (fabsf(f->x - lx) > FPL_FWD_HIT || fabsf(f->y - ly) > FPL_FWD_HIT) continue;
            if (f->z < bz) { bz = f->z; best = k; }
        }
        if (best >= 0) {
            s->alive = 0;
            if (FPL_FOE[fpg.foe[best].kind].hp == 0) { fpl_sfx("fpl_tink", 3); continue; }
            fpl_hurt_foe(best, 1);
            continue;
        }
        if (fpg.boss.on && !fpg.boss.dead && z0 < FPL_BOSS_Z && s->z >= FPL_BOSS_Z) {
            if (fpl_boss_shot(s)) { s->alive = 0; continue; }
        }
        if (fpg.state == PS_BONUS) fpl_bonus_shot(s, z0);
    }
}

void fpl_eshots_update(void) {
    for (int i = 0; i < FPL_MAX_ESHOTS; i++) {
        FplEShot *e = &fpg.es[i];
        if (!e->alive) continue;
        e->t++;
        if (fpg.still) continue;
        if (e->kind == ES_PLANE) {
            if (e->spin != 0) {
                float c = cosf(e->spin), s = sinf(e->spin);
                float vx = e->vx * c - e->vy * s, vy = e->vx * s + e->vy * c;
                e->vx = vx * 1.004f;
                e->vy = vy * 1.004f;
                e->spin *= 0.985f;
            }
            e->x += e->vx;
            e->y += e->vy;
            if (e->x < -3.4f || e->x > 3.4f || e->y < -2.4f || e->y > 2.4f || e->t > e->life) { e->alive = 0; continue; }
            if (fpl_ship_hit_circle(e->x, e->y, 0.1f)) { e->alive = 0; fpl_lose_ship(CAUSE_SHOT); }
            continue;
        }
        /* a shell in depth */
        e->x += e->vx;
        e->y += e->vy;
        e->z += e->vz;
        if (e->z <= 0) {
            e->alive = 0;
            if (fpl_ship_hit_circle(e->x, e->y, e->burst ? 0.14f : 0.18f)) fpl_lose_ship(CAUSE_SHOT + 1);
            if (e->burst) burst_at(e->x, e->y, e->burst);
        }
    }
}

void fpl_parts_update(void) {
    for (int i = 0; i < FPL_MAX_PARTS; i++) {
        FplPart *p = &fpg.part[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->z += p->vz;
        p->vx *= 0.93f;
        p->vy *= 0.93f;
    }
}

void fpl_play_update(uint8_t in) {
    fpg.stage_t++;
    if (fpg.radio) fpg.radio_t++;
    fpl_ship_update(in);
    run_wave();
    fpl_foes_update();
    if (fpg.boss.on) fpl_boss_update();
    fpl_shots_update();
    fpl_eshots_update();
    fpl_parts_update();
    if (fpg.shake > 0) fpg.shake--;
}
