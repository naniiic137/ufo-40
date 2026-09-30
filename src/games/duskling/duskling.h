/* DUSKLING - shared declarations. Cartridge 13 of UFO 40,
 * a tribute to Mooncat (UFO 50 #13). See docs/games/13-duskling.md. */
#ifndef DUSKLING_H
#define DUSKLING_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define DK_TS 10          /* tile size in pixels */
#define DK_MAXW 256       /* widest room, in tiles */
#define DK_MAXH 40        /* tallest room, in tiles */
#define DK_ROOMS 42

/* Rooms by index: the main way (M0 the dayling's walk, M1-M21), the
 * amber way (A1-A7), the rose way (R1-R8) and the warp pockets (X1-X5). */
enum {
    RM_M0 = 0, RM_M1, RM_M2, RM_M3, RM_M4, RM_M5, RM_M6, RM_M7, RM_M8, RM_M9, RM_M10,
    RM_M11, RM_M12, RM_M13, RM_M14, RM_M15, RM_M16, RM_M17, RM_M18, RM_M19, RM_M20, RM_M21,
    RM_A1, RM_A2, RM_A3, RM_A4, RM_A5, RM_A6, RM_A7,
    RM_R1, RM_R2, RM_R3, RM_R4, RM_R5, RM_R6, RM_R7, RM_R8,
    RM_X1, RM_X2, RM_X3, RM_X4, RM_X5
};

enum { BI_DUSK, BI_WOOD, BI_MERE, BI_STEPS, BI_WORKS, BI_CAVES, BI_HEIGHTS, BI_POCKET, BI_COUNT };
enum { EGG_WHITE = 0, EGG_AMBER, EGG_ROSE };

typedef struct DKRoom {
    const char *name;
    uint8_t biome;
    int8_t next;             /* where the way out ('>') leads, -1 = none */
    int8_t warp_room[3];     /* where warps '1' '2' '3' lead */
    char warp_at[3];         /* ... and where they set you down: 'S', '@' or '&' */
    const char *const *rows; /* NULL-terminated, all the same width */
} DKRoom;

extern const DKRoom DK_ROOM[DK_ROOMS];

/* Tile legend (see duskling_rooms.c):
 *  ' ' air          '#' ground         '=' cloud / pink ledge (one way)
 *  '^' thorns       '~' water          'X' ground that is not there (the dusk wood)
 *  '?' hidden block (a stone face nearby shows it)
 *  '%' hidden spring block (shows when touched)
 *  'I' stone face (two jumps over it wake it)
 *  'b' blue bells   'o' orange poppies   'k' bones   (background)
 *  'm' mushroom cap (solid; a slam on it springs you high)
 *  '*' a perch for the fleeing Hermit (never drawn)
 *  '1' '2' '3' warps (never drawn)     'S' '@' '&' ways in  '>' way out
 *  'D' boss door (opens when the boss falls)                'Q' the egg
 *  't' the mole's old tent (background)
 *  foes: 'w' prickle   'v' wasp (across)  'y' wasp (up and down)
 *        'p' thistledown (harmless)   'F' frog (a spring)
 *        'M' gulper (a two-tile mouth: a ledge while shut)
 *        'e' ceiling eye   'r' pebble beetle (knocked flying by a slam)
 *        'n' spear newt   'c' crow with a pebble (harmless)   'x' fish
 *  bosses: 'W' the Brass Warden   'H' the Ember Hermit   'G' the Old Badger
 *          'h' the Ember Hermit on the main way, who only runs off */

/* ------------------------------------------------------------------ */
/* the world: everything a room needs from one frame to the next.
 * It is plain data, so the route finder can copy it about. */

#define DK_PLAYERS 2
#define DK_FOES 48
#define DK_SHOTS 24
#define DK_MUT 128

enum { SIDE_L = 1, SIDE_R = 2 };
enum { ACT_NONE, ACT_JUMP, ACT_SLIDE, ACT_SPRINT, ACT_POUND, ACT_BOUNCE };
#define DK_SLAM_HANG 7    /* frames the slam hangs in the air, with a "!" */

typedef struct DKPlayer {
    float x, y, vx, vy;
    int32_t press_t[3], last_press_t[3], release_t[3]; /* by side (1, 2) */
    int16_t jump_hold, coyote, act_t, dead_t, stun, anim, over_face;
    int8_t face, jump_dir;
    uint8_t alive, ground, act, held, prev, jump_side, spun, first, in_water, ride, pounded, hb_streak;
    int8_t on_foe;
} DKPlayer;

enum {
    F_PRICKLE, F_WASP, F_WASPV, F_PUFF, F_FROG, F_GULPER, F_EYE, F_BEETLE, F_NEWT, F_CROW, F_FISH, F_KINDS
};
typedef struct DKFoe {
    float x, y, vx, vy, x0, y0;
    int16_t t, phase, timer;
    uint8_t kind, alive, state;
    int8_t dir;
} DKFoe;

enum { SH_SPEAR, SH_FIRE, SH_SPARK, SH_BLAST };
typedef struct DKShot {
    float x, y, vx, vy;
    uint8_t kind, alive;
    int16_t t;
} DKShot;

enum { BOSS_NONE, BOSS_WARDEN, BOSS_HERMIT, BOSS_BADGER };
typedef struct DKBoss {
    float x, y, vx, vy;
    int16_t hp, t, flash, state, cycle, perch, spoke;
    uint8_t kind, alive, solid, ground;
    int8_t dir;
} DKBoss;

typedef struct DKWorld {
    int32_t t;                /* frames since the room was entered */
    int16_t room, freeze;
    uint8_t nplayers, arrive, boss_down;
    DKPlayer P[DK_PLAYERS];
    DKFoe foe[DK_FOES];
    DKShot shot[DK_SHOTS];
    DKBoss boss;
    uint8_t mut[DK_MUT];      /* hidden blocks shown, faces woken, doors open */
    int16_t head_bounces;     /* two players: bounces on each other's heads */
    uint8_t socks;            /* ... three in a row: the line at the end */
} DKWorld;

extern DKWorld dk_w;

/* what a step of the world brings about */
enum { EV_NONE, EV_EXIT, EV_WARP1, EV_WARP2, EV_WARP3, EV_EGG, EV_REBIRTH };

/* world (duskling_world.c) */
void dk_room_load(int room, int arrive, int nplayers);   /* enter a room afresh */
void dk_room_reset(void);                                 /* start the room over (a death) */
int dk_world_step(uint32_t pad1, uint32_t pad2);          /* one frame; pads are BTN_* masks */
char dk_tile(int tx, int ty);                             /* what is drawn / felt there */
char dk_tile_raw(int tx, int ty);
int dk_room_w(void);
int dk_room_h(void);
bool dk_hidden_shown(int tx, int ty);
bool dk_wet(int tx, int ty);
bool dk_face_awake(int tx, int ty);
int dk_face_passes(int tx, int ty);  /* 0, 1 (an eye glows) or 2 (awake) */
bool dk_solid_at(int tx, int ty);
int dk_foe_count(int kind);
void dk_player_hitbox(int i, float *x, float *y, float *w, float *h);
extern bool dk_sim_quiet;  /* the route finder is trying things: no sound */
extern int dk_last_event;
/* sound and dust requests from the world (the game draws/plays them) */
void dk_fx(const char *sfx, float x, float y, int col, int n);

/* the route finder (scripts only): prints the button presses as script lines */
int dk_solve(int room, int arrive, int target, int max_nodes, FILE *out);
/* the same through waypoint tiles in order (n pairs of tile x, y) */
int dk_solve_here(int target, int max_nodes, FILE *out); /* from the world as it stands */
int dk_solve_via(int room, int target, int max_nodes, const int *wp, int n, FILE *out);
void dk_solve_tile(int tx, int ty); /* the next solve aims at this tile instead */
extern int dk_solve_ok;   /* the last solve: 1 found, 0 not */
extern int dk_nerf;
enum { NERF_FACES = 1, NERF_FROGS = 2, NERF_SPRINGS = 4, NERF_EYES = 8 };
enum { TG_EXIT, TG_WARP1, TG_WARP2, TG_WARP3, TG_EGG };

/* how forgiving one obstacle is (tests only): a steady player tries every
 * mix of wait, take-off distance and jump hold from a spot (see dk_window) */
typedef struct DKWinSpec {
    int room;
    int sx, sy;          /* where the duskling stands to start (as "cheat pos") */
    int edge;            /* the first jump's edge in px, or -1: the stop ahead */
    int gx0, gx1, gfeet; /* the goal: centre x in gx0..gx1, feet at or above
                            gfeet (left of the start: it goes left);
                            gx0 < 0: the way out (or a warp) */
    int period, wstep;   /* the waits tried: 0, wstep, ... below period */
    int slam;            /* its jumps from pink ledges end in a slam */
} DKWinSpec;
typedef struct DKWinResult {
    int n, ok;           /* inputs tried, and those that got there */
    int pct;             /* ok * 100 / n */
    int clock;           /* % of the waits at which some input gets there */
    int worst;           /* % of the inputs that get there at the worst wait */
    int tw, xw, hw;      /* the best input's windows: frames of wait, px of
                            take-off distance (of a jump), frames of hold */
} DKWinResult;
int dk_window(const DKWinSpec *s, DKWinResult *r, FILE *out);

/* art */
enum {
    S_PIM1, S_PIM2, S_PIM3, S_PIM_JUMP, S_PIM_FALL, S_PIM_ROLL1, S_PIM_ROLL2, S_PIM_POUND, S_PIM_SWIM,
    S_ORB, S_PRICKLE1, S_PRICKLE2, S_WASP1, S_WASP2, S_PUFF, S_FROG1, S_FROG2, S_EYE, S_EYE_OPEN,
    S_BEETLE1, S_BEETLE2, S_BEETLE_BALL, S_NEWT1, S_NEWT2, S_SPEAR, S_CROW1, S_CROW2, S_FISH1, S_FISH2,
    S_FACE, S_FACE_AWAKE, S_BELL, S_POPPY, S_EGG, S_EGG_CRACK, S_TENT,
    S_WARDEN1, S_WARDEN2, S_HERMIT1, S_HERMIT2, S_BADGER1, S_BADGER2, S_BADGER_JUMP,
    S_FIRE1, S_FIRE2, S_SPARK, S_TREE, S_MOTH, S_HELM, S_BANG, S_BONES, S_MUSH, S_PRICKLE_FLIP,
    S_COUNT
};
extern Sprite dk_spr[S_COUNT];
void dk_art_load(void);

/* audio */
void dk_audio_load(void);
extern int DK_MUS_TITLE, DK_MUS_DUSK, DK_MUS_WOOD, DK_MUS_MERE, DK_MUS_STEPS, DK_MUS_WORKS, DK_MUS_CAVES,
    DK_MUS_HEIGHTS, DK_MUS_POCKET, DK_MUS_BOSS, DK_MUS_EGG, DK_MUS_WAKE;

#endif
