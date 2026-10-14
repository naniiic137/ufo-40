/* LIMPET - shared declarations. Cartridge 20 of UFO 40.
 * A tribute to Warptank (UFO 50 #20); see docs/games/20-limpet.md.
 *
 * Spindle Station has been overrun by the Gristle, which came up through
 * the cargo pods from the Underneath. Limpet, the last crew tank, sticks to
 * whatever hull it is on: it rolls along floors, walls and ceilings (ramps
 * carry it round corners), can't jump or fall, fires straight out from its
 * surface and warps in a straight line to the opposite surface. 26 decks
 * and a secret 27th are entered from pods in the station's hub.
 *
 * The tank lives on a grid: a cell and a normal (the way its cannon points).
 * Everything that flies uses whole numbers in 1/256 of a pixel, so a run
 * plays the same on every platform. */
#ifndef LMP_H
#define LMP_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include <ctype.h>

/* ---- directions: 45-degree steps, clockwise from up -------------------- */
enum { DR_U, DR_UR, DR_R, DR_DR, DR_D, DR_DL, DR_L, DR_UL };
extern const int LMP_DX[8], LMP_DY[8];
static inline bool lmp_axis(int n) { return (n & 1) == 0; }

/* ---- the grid ------------------------------------------------------------ */
#define LMP_T 10                 /* a tile, pixels */
#define LMP_FX 256               /* one pixel, fixed point */
#define LMP_MW 128               /* the largest map (the hub) */
#define LMP_MH 64
#define LMP_HUD 10               /* the status strip on top */
#define LMP_VIEW_H (SCREEN_H - LMP_HUD)

/* ---- rules ----------------------------------------------------------------- */
#define LMP_STEP_T 7             /* frames to roll one tile */
#define LMP_RAMP_T 5             /* frames for each half of a ramp */
#define LMP_WARP_LAG 10          /* frames after a warp before the next move */
#define LMP_WARP_COOL 14
#define LMP_BURST 3              /* a held trigger fires three */
#define LMP_BURST_GAP 9          /* frames between the shots of a held burst */
#define LMP_MASH_GAP 5           /* the quickest a fresh press can fire again */
#define LMP_MAX_SHOTS 6
#define LMP_SHOT_SPEED (4 * LMP_FX)
#define LMP_BLAST_T 14           /* the warp's landing flares */
#define LMP_DEAD_T 50

#define LMP_SECTORS 27           /* 26 decks + the secret one */
#define LMP_NORMAL_DECKS 26
#define LMP_HONEY_TOTAL 24       /* 23 decks + the Dreadnought */
#define LMP_DREAD_NEED 22        /* decks cleared before the Dreadnought's pod opens */
enum { LV_TURNCOAT = 27, LV_DREAD = 28, LV_HUB = 29, LMP_LEVELS = 30 };
enum { LK_NORMAL, LK_SURVIVAL, LK_SECRET, LK_TURNCOAT, LK_DREAD, LK_HUB };

/* things a level places that aren't single tiles */
enum { TH_NONE, TH_MOVER, TH_WAVE };
typedef struct {
    uint8_t type;
    int x, y, w, h;              /* cells */
    int dir, dist, period;       /* a mover: way, cells, frames a cell */
    int lethal;                  /* a crusher */
} LmpThing;

typedef struct {
    const char *name;
    int kind;
    const char *rows[LMP_MH + 1];
    const char *route;           /* the demo player's plan (see lmp_bot.c) */
    const LmpThing *things;
    int nthings;
    const char *const *waves;    /* survival: the foes of each wave */
    int nwaves;
    int music;                   /* 0..3 deck tunes */
} LmpLevel;
extern const LmpLevel *const LMP_LEVEL[LMP_LEVELS];

/* the hub's pods, gates, people and paint bays, in the order the map holds them */
typedef struct { int level; } LmpPod;
enum { GATE_CLEARED, GATE_TURNCOAT, GATE_DREAD, GATE_HONEY };
typedef struct { int x, y, w, h, cond, need; const char *what; } LmpGate;
typedef struct { const char *name; const char *lines[4]; } LmpNpc;
typedef struct { int x0, x1, y0, y1, zone; } LmpZone;
extern const LmpPod LMP_HUB_POD[];
extern const int LMP_HUB_NPODS;
extern const LmpGate LMP_HUB_GATE[];
extern const int LMP_HUB_NGATES;
extern const LmpNpc LMP_HUB_NPC[];
extern const int LMP_HUB_NNPCS;
extern const LmpZone LMP_HUB_ZONE[];
extern const int LMP_HUB_NZONES;
enum { ZN_CONCOURSE, ZN_CANTEEN, ZN_GRISTLE, ZN_UPPER, ZN_COUNT };
extern const char *const LMP_ZONE_NAME[ZN_COUNT];
extern const char *const LMP_SPEAK_NAME;

/* ---- foes -------------------------------------------------------------------- */
enum {
    FO_NONE,
    FO_TURRET,    /* fires along its line when the tank is on it, then cools */
    FO_LASER,     /* wall laser: a quick beam on a beat */
    FO_DRONE,     /* hovering drone: slow beams that bounce */
    FO_EYE,       /* ogler: floats, sends rings at the tank */
    FO_CRAWLER,   /* tank-sized gristle crawler: rolls along surfaces */
    FO_SAC,       /* gristle sac: lets out specks */
    FO_SPECK,     /* drifts toward the tank */
    FO_WORM,      /* cable worm: runs a corridor */
    FO_SWITCHWORM,/* a worm whose head flips the swap blocks */
    FO_BALL,      /* spiked ball: rolls and bounces, can't be hurt */
    FO_FLAME,     /* flame nozzle: breathes on a beat, can't be hurt */
    FO_TURNCOAT,  /* the mid-game boss */
    FO_DREAD,     /* the final boss */
    FO_KINDS
};
extern const char *const LMP_FOE_NAME[FO_KINDS];
extern const int LMP_FOE_HP[FO_KINDS];

typedef struct {
    uint8_t kind, alive;
    int16_t hp;
    int x, y;                    /* centre, fixed point */
    int vx, vy;
    int cx, cy, n;               /* grid foes: cell and normal */
    int ox, oy, on, mt;          /* grid foes moving: from, and frames into the step */
    int t, st, state, dir, a, b;
    int flash;
    int w, h;                    /* hitbox, pixels */
    int spawn;                   /* the index in the level's foe list, -1 none */
    int16_t trail[24][2];        /* a worm's body (pixels) */
} LmpFoe;

enum { ES_BULLET, ES_LASER, ES_BEAM, ES_RING, ES_SHELL, ES_BOLT };
typedef struct {
    uint8_t kind, alive;
    int x, y, vx, vy;            /* fixed point */
    int t, life, bounces;
} LmpEShot;

typedef struct {
    uint8_t alive;
    int x, y, vx, vy, n;
} LmpShot;

typedef struct { int x, y, vx, vy, life, col; } LmpPart;

typedef struct {
    int x, y, w, h;              /* cells, top-left of the body */
    int dir, dist, period, lethal;
    int pos, sign, t, wait;      /* cells travelled, +1/-1, frames into the cell */
} LmpMover;

/* the tank */
typedef struct {
    int x, y, n;                 /* where it is (logical) */
    int pn;                      /* path: the states it rolls through */
    int px[8], py[8], pnn[8];
    int pt, plen;                /* frames into the path, total */
    int sx, sy, sn;              /* where the path started */
    int sense;                   /* +1 clockwise, -1 the other way, 0 still */
    int run_btn;                 /* the pad button that began this run */
    int lag;                     /* frames before it can move again (after a warp) */
    int warp_cool;
    int burst, burst_t, fire_gap;
    int blast_t, blast_x, blast_y, blast_n;
    int warp_from_x, warp_from_y, warp_fx_t;
    int fizz_t;
    int hop_t;                   /* a hopper pad's fling, counting down */
    bool portal_lock;
    int colour;
} LmpTank;

/* what a checkpoint keeps */
typedef struct {
    char map[LMP_MH][LMP_MW + 1];
    LmpMover mover[12];
    int nmover;
    int parity, honey_shown;
    int x, y, n;
    bool set;
} LmpCheck;

#define LMP_MAX_FOES 48
#define LMP_MAX_ESHOTS 64
#define LMP_MAX_PARTS 120
#define LMP_MAX_MOVERS 12
#define LMP_MAX_SPAWNS 64

typedef struct { uint8_t kind; int x, y, n; } LmpSpawn;

/* ---- states ------------------------------------------------------------------ */
enum {
    GS_TITLE, GS_STORY, GS_PLAY, GS_DEAD, GS_PROMPT, GS_CLEAR, GS_ENDING, GS_CHERRY, GS_TALK
};

/* the cartridge save */
typedef struct {
    uint32_t magic;
    uint32_t cleared;            /* bit per deck (0..26) */
    uint32_t honey;              /* bit per honey: decks 0..25, bit 26 the Dreadnought's */
    uint32_t best[LMP_SECTORS];  /* best clear time per deck, frames; 0 none */
    uint8_t turncoat, dread, secret, started;
    uint8_t colour, colours;     /* the tank's paint; bays found (bits) */
    uint8_t last_pod;            /* where the tank comes out in the hub */
    uint8_t ends_seen;
    uint16_t deaths, wins;
} LmpSave;

typedef struct {
    int state, state_t, frame_t, menu;
    Rng rng;
    /* the level */
    int level, kind;
    int mw, mh;
    char map[LMP_MH][LMP_MW + 1];
    int cam_x, cam_y;
    int parity;
    int honey_x, honey_y;        /* the hidden honey, -1 none */
    bool honey_shown, honey_held;
    int bulb_out;                /* the red bulb is shot: the pod is open */
    int timed_t;                 /* a timed door's frames left open */
    int timer;                   /* frames in this deck */
    int wave, wave_t;            /* survival */
    bool boss_dead;
    int boss_t;
    int exit_x, exit_y;
    LmpMover mover[LMP_MAX_MOVERS];
    int nmover;
    LmpSpawn spawn[LMP_MAX_SPAWNS];
    int nspawn;
    LmpCheck check;
    /* things */
    LmpTank tk;
    LmpShot shot[LMP_MAX_SHOTS];
    LmpFoe foe[LMP_MAX_FOES];
    LmpEShot es[LMP_MAX_ESHOTS];
    LmpPart part[LMP_MAX_PARTS];
    int shake;
    /* the hub */
    int talk_npc, talk_t;
    int pod_here;                /* the pod the tank is at, -1 none */
    char msg[48];
    int msg_t;
    int clear_level, clear_time, clear_best, clear_honey, clear_new;
    int dead_t;
    int last_hurt;
    /* tests */
    bool god;
    int hits;
    bool tally_hits;
    int deaths_here;
    int sim_deaths;
} LmpGame;

extern LmpGame lg;
extern LmpSave lgs;

/* The demo player looks ahead by running the real game on a copy of lg
 * with made-up buttons: while it does, the game reads these instead of the
 * pad and makes no sound. */
extern bool lmp_simulating;
extern uint32_t lmp_sim_cur, lmp_sim_prev;
static inline bool lmp_btn(int m) { return lmp_simulating ? (lmp_sim_cur & (uint32_t)m) != 0 : btn(m); }
static inline bool lmp_btnp(int m) {
    return lmp_simulating ? ((lmp_sim_cur & ~lmp_sim_prev) & (uint32_t)m) != 0 : btnp(m);
}
void lmp_sfx(const char *name);
void lmp_music(int id);

/* lmp.c */
void lmp_play_step(void);
void lmp_say(const char *m);
void lmp_save_now(void);
int lmp_cleared_count(void);
int lmp_honey_count(void);
void lmp_level_cleared(void);
void lmp_tank_died(int cause);

/* lmp_world.c: tiles, the map, checkpoints, movers */
void lmp_load_level(int level);
char lmp_tile(int x, int y);
bool lmp_solid(int x, int y);              /* blocks a tank, a crawler and a warp */
bool lmp_face_solid(int x, int y, int dir);/* cell (x,y)'s face toward dir is a surface */
bool lmp_open(int x, int y);               /* a tank can be in the cell */
bool lmp_shot_blocked(int x, int y);
bool lmp_is_spikes(int x, int y);
bool lmp_is_crusher(int x, int y);
int lmp_tri_normal(char c);                /* the open side of a ramp tile, -1 not one */
bool lmp_warp_pass(int x, int y, int dir); /* a warp travelling dir goes through */
void lmp_shoot_tile(int x, int y);
void lmp_world_update(void);
void lmp_take_checkpoint(void);
void lmp_restore_checkpoint(void);
int lmp_mover_at(int x, int y);
void lmp_open_exit(void);
int lmp_count_tiles(char c);

/* lmp_tank.c: the surface graph, rolling, warping, shooting */
bool lmp_state_ok(int x, int y, int n);
int lmp_surface_step(int x, int y, int n, int sense, int *ox, int *oy, int *on);
int lmp_roll_path(int x, int y, int n, int sense, int *px, int *py, int *pn, int max);
int lmp_warp_target(int x, int y, int n, int *ox, int *oy, int *on);
void lmp_tank_update(void);
void lmp_tank_place(int x, int y, int n);
void lmp_tank_touch(void);
void lmp_tank_centre(int *x, int *y);       /* pixels */
void lmp_shots_update(void);
bool lmp_tank_moving(void);
void lmp_kill_tank(int cause);
void lmp_burst(int x, int y, int col, int n, int sp);
void lmp_parts_update(void);

/* lmp_foes.c */
int lmp_spawn_foe(int kind, int cx, int cy, int n);
void lmp_foes_update(void);
void lmp_hurt_foe(int i, int dmg);
int lmp_add_eshot(int kind, int x, int y, int vx, int vy);
void lmp_eshots_update(void);
int lmp_foes_alive(void);
int lmp_count_foes(int kind);
void lmp_boss_start(void);
void lmp_survival_update(void);
int lmp_flame_reach(const LmpFoe *f);       /* cells a nozzle's flame covers now, 0 off */

/* lmp_bot.c */
int lmp_bot_buttons(void);
extern int lmp_bot_honey;                   /* 1: the demo player hunts every honey */
int lmp_bfs_reach(int tx, int ty, int tn);  /* steps from the tank to a state, -1 none */

/* lmp_draw.c */
void lmp_draw(void);
void lmp_draw_label(int x, int y, int w, int h, int t);
int lmp_layout_audit(void);

/* art and sound */
enum {
    SPR_TANK, SPR_TANK_D, /* base art, then rotated copies */
    SPR_TURRET, SPR_LASER, SPR_DRONE0, SPR_DRONE1, SPR_EYE0, SPR_EYE1, SPR_CRAWLER0, SPR_CRAWLER1,
    SPR_SAC0, SPR_SAC1, SPR_SPECK0, SPR_SPECK1, SPR_WORM_HEAD, SPR_WORM_BODY, SPR_BALL, SPR_NOZZLE,
    SPR_HONEY, SPR_BEE, SPR_POD, SPR_NPC0, SPR_NPC1, SPR_NPC2, SPR_NPC3, SPR_NPC4, SPR_TURNCOAT,
    SPR_COUNT
};
extern Sprite lmp_spr[SPR_COUNT];
extern Sprite lmp_tank_spr[5][8];          /* paint x normal (4: the Turncoat) */
extern Sprite lmp_rot[SPR_COUNT][4];       /* square sprites turned to each axis normal */
void lmp_art_load(void);
int lmp_art_check(void);
void lmp_audio_load(void);
extern int LMP_MUS_TITLE, LMP_MUS_CONCOURSE, LMP_MUS_CANTEEN, LMP_MUS_GRISTLE, LMP_MUS_UPPER, LMP_MUS_DECK[4],
    LMP_MUS_SURVIVAL, LMP_MUS_TURNCOAT, LMP_MUS_DREAD, LMP_MUS_COMB, LMP_MUS_CLEAR, LMP_MUS_DEATH,
    LMP_MUS_ENDING, LMP_MUS_STORY, LMP_MUS_HONEY;

#endif
