/* BELLHOP - shared declarations. Cartridge 17 of UFO 40.
 * A tribute to Campanella (UFO 50 #17); see docs/games/17-bellhop.md.
 *
 * Ansel flies the Tinkler, the chime ship of CHIME CIRCUIT, through fifty
 * one-screen stages in five worlds to Lady Hush's citadel, and stops for
 * tea on the way. The ship flies on CHIME CIRCUIT's flight model
 * (../chime/chime_flight.c) with its own numbers and a fuel tank.
 *
 *   bellhop_stage.c   the rules of a stage: flight, fuel, the slash, crashes,
 *                     every enemy and stage part, tea, coins, circlers,
 *                     warps and the crystal rooms
 *   bellhop_boss.c    the five bosses
 *   bellhop_stages.c  the fifty stage layouts and the demo pilot's routes
 *   bellhop_bot.c     the demo pilot: a planner that flies the real rules
 *   bellhop.c         the screens, the run, lives and score, saving, goals,
 *                     the code screen and the test hooks
 *   bellhop_art.c     pixel art         bellhop_audio.c   music and sounds */
#ifndef BELLHOP_H
#define BELLHOP_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include "../chime/chime_flight.h"

#define BHP_TW 40              /* a stage is 40 x 21 tiles of 8 px */
#define BHP_TH 21
#define BHP_T 8
#define BHP_OY 12              /* the stage's top on screen (the HUD is above) */
#define BHP_WORLDS 5
#define BHP_PER_WORLD 10
#define BHP_STAGES 50
#define BHP_CUPS 40            /* one cup of tea on every regular stage */
#define BHP_ENTS 72
#define BHP_SHOTS 64
#define BHP_FX 12

/* numbers (frames at 60 a second, 1/256 px) */
#define BHP_FUEL_MAX 600       /* frames of thrust in a full tank */
#define BHP_SLASH_T 12         /* a slash lasts this long ... */
#define BHP_SLASH_ACTIVE 8     /* ... and hits in its first frames */
#define BHP_SLASH_CD 18        /* frames from one slash to the next */
#define BHP_SLASH_REACH 18     /* px beyond the ship's middle */
#define BHP_DEAD_T 70          /* a crash, then back to the bubble */
#define BHP_CLEAR_T 40         /* into the exit ring */
#define BHP_WARP_SHOW 300      /* a warp sparkle shows this long after the stage loads */
#define BHP_COIN_T 300         /* the five coins wait this long */
#define BHP_ROUND_T 480        /* a crystal round */
#define BHP_OWL_T 3600         /* a minute on the first stage */
#define BHP_EXTEND 1000        /* a ship every 1,000 points */
#define BHP_START_SHIPS 3
#define BHP_MAX_SHIPS 99
#define BHP_GIFT_SHIPS 15

/* stage kinds: X-5 is a crystal room, X-10 a boss */
enum { BHK_STAGE, BHK_BONUS, BHK_BOSS };
int bhp_kind(int stage);
/* the regular stage's cup number, 0..39 (-1 for a crystal room or a boss) */
int bhp_cup_index(int stage);
/* "A-1" */
const char *bhp_stage_label(int stage);

/* tiles */
enum {
    BTL_AIR, BTL_WALL, BTL_WALL2, BTL_LIQUID, BTL_BLOCK, BTL_GLASS, BTL_CRACK,
    BTL_GATE1, BTL_GATE1_OPEN, BTL_GATE2, BTL_GATE2_OPEN, BTL_LEVER1, BTL_LEVER2,
    BTL_CANNON_L, BTL_CANNON_R, BTL_CANNON_U, BTL_CANNON_D, BTL_THORN, BTL_PRESS
};
bool bhp_tile_solid(int t);

/* things on a stage */
enum {
    BEK_NONE,
    /* enemies */
    BEK_MOTH, BEK_MITE, BEK_WASP, BEK_CRAWLER, BEK_TURRET, BEK_GHOST, BEK_DRONE,
    /* parts */
    BEK_BUBBLE, BEK_BOMB, BEK_FIREBAR, BEK_PLATE_H, BEK_PLATE_V, BEK_FUEL,
    BEK_BIGCOIN, BEK_COIN, BEK_CIRCLER, BEK_CRYSTAL,
    /* boss parts */
    BEK_BUCKET, BEK_APPLE, BEK_SPRINKLER, BEK_LAMP, BEK_GUM, BEK_CHUNK, BEK_SPIKE,
    BEK_COUNT
};

typedef struct BhpEnt {
    uint8_t kind, on;
    uint8_t hp, flag;          /* flag: per kind (a lit fuse, a lit node set, red/blue ...) */
    int32_t x, y, vx, vy;      /* middle, 1/256 px */
    int16_t hx, hy;            /* home, px */
    int16_t a, b;              /* per kind */
    uint16_t t;
    uint8_t id;                /* its place in the stage's list, for what stays taken */
    uint8_t size;              /* gumballs: 2 giant, 1 middling, 0 small */
    uint8_t hitno;             /* the slash that last struck it */
} BhpEnt;

enum { BSH_PELLET, BSH_BALL, BSH_JET, BSH_RETURN };
typedef struct BhpShot {
    int32_t x, y, vx, vy;
    uint8_t kind, on;
    uint16_t life;
} BhpShot;

/* what happened this frame, for sounds and sparks */
enum {
    BEV_SLASH = 1 << 0, BEV_HIT = 1 << 1, BEV_BREAK = 1 << 2, BEV_KILL = 1 << 3, BEV_CRASH = 1 << 4,
    BEV_CUP_SHOW = 1 << 5, BEV_CUP = 1 << 6, BEV_COIN = 1 << 7, BEV_BIGCOIN = 1 << 8, BEV_NODE = 1 << 9,
    BEV_CIRCLER = 1 << 10, BEV_FUEL = 1 << 11, BEV_LEVER = 1 << 12, BEV_BLAST = 1 << 13, BEV_SHOOT = 1 << 14,
    BEV_CLEAR = 1 << 15, BEV_WARP = 1 << 16, BEV_POP = 1 << 17, BEV_CRYSTAL = 1 << 18, BEV_ROUND = 1 << 19,
    BEV_BOSS_HIT = 1 << 20, BEV_BOSS_DOWN = 1 << 21, BEV_EXIT_OPEN = 1 << 22, BEV_DUNK = 1 << 23,
    BEV_FUSE = 1 << 24, BEV_DRY = 1 << 25
};
typedef struct BhpFx { int16_t x, y; uint8_t kind; } BhpFx;

/* the ship's mode */
enum { BSM_BUBBLE, BSM_FLY, BSM_DEAD, BSM_CLEAR, BSM_WARP };

typedef struct BhpBoss {
    uint8_t kind;              /* 0 none, 1..5 by world */
    uint8_t phase, hp, down;
    int32_t ang, spin;         /* wheels: 1/65536 turns, and a frame */
    int32_t x, y, vx, vy;      /* Lady Hush's ship, 1/256 px */
    uint16_t t, t2;
    uint8_t active;            /* the bucket or wheel that is open */
    uint8_t round, left;       /* gumball rounds */
    uint16_t down_t;
    uint8_t lobs;
} BhpBoss;

typedef struct BhpStage {
    uint8_t idx, kind;
    uint8_t tile[BHP_TH][BHP_TW];
    uint16_t ver;              /* goes up when a tile changes */
    ChmFlight f;
    uint8_t mode;
    uint8_t ctl;               /* this frame's controls (CHF_* plus BHP_CTL_SLASH) */
    int16_t fuel;
    uint16_t slash_t, slash_cd;
    uint8_t slash_no;          /* counts slashes, so a slash strikes a thing once */
    uint32_t t;                /* frames since the stage loaded (the warp clock) */
    uint32_t life_t;           /* frames in this life */
    uint16_t mode_t;
    /* tea: 0 hidden, 1 out, 2 taken (stays through a crash) */
    uint8_t cup;
    int16_t cup_x, cup_y;
    int16_t trig_c, trig_r;    /* the hidden trigger's tile (-1: none) */
    int16_t start_x, start_y;
    int16_t exit_x, exit_y;
    uint8_t exit_open;
    int16_t warp_x, warp_y;
    int8_t warp_to;
    int16_t coin_x[5], coin_y[5];
    uint8_t ncoin_spots, coins_got;
    uint16_t coin_t;
    uint8_t lever_cd;
    uint8_t ncan;              /* the cannons, by tile */
    int8_t can_c[12], can_r[12];
    uint64_t taken;            /* ids of circlers and big coins already had (stay through a crash) */
    BhpEnt e[BHP_ENTS];
    int ne;
    BhpShot shot[BHP_SHOTS];
    BhpBoss boss;
    /* crystal rooms */
    uint8_t round, crystals_left, bonus_done;
    uint16_t round_t;
    uint8_t crystal_pts;       /* 200, or 100 with the code */
    /* this frame's news (the run takes the points and fuel use) */
    uint32_t ev;
    int pts;
    int thrust_frames;
    BhpFx fx[BHP_FX];
    uint8_t nfx;
    uint8_t dead_cause;
    Rng rng;
} BhpStage;

/* the controls for one frame: the flight bits, and the slash (a press) */
#define BHP_CTL_SLASH 8

extern const ChmFlightTune BHP_TUNE;

/* ---- stages (bellhop_stages.c) ------------------------------------------ */
typedef struct BhpStageDef {
    const char *name;
    const char *rows[BHP_TH];
    int8_t warp_to;            /* -1, or the stage a warp sparkle sends you to */
    const char *route;         /* the demo pilot's way through (see bellhop_bot.c) */
} BhpStageDef;
extern const BhpStageDef BHP_STAGE[BHP_STAGES];
extern const char *const BHP_WORLD_NAME[BHP_WORLDS];

/* ---- rules (bellhop_stage.c) -------------------------------------------- */
/* load stage idx afresh (a new visit: tea and taken things forgotten) */
void bhp_stage_load(BhpStage *s, int idx, int crystal_pts, uint64_t seed);
/* back to the bubble after a crash: everything as it was, except the warp
 * clock, the tea and the circlers and big coins already had */
void bhp_stage_respawn(BhpStage *s);
void bhp_stage_step(BhpStage *s, unsigned ctl);
bool bhp_solid(const void *ctx, int px, int py);
bool bhp_ship_box_hits(const BhpStage *s, int x0, int y0, int x1, int y1); /* px box vs the ship */
void bhp_kill_ship(BhpStage *s, int cause);
void bhp_add_fx(BhpStage *s, int x, int y, int kind);
int bhp_ent_add(BhpStage *s, int kind, int x, int y);
void bhp_ent_settle(BhpStage *s, BhpEnt *e); /* a crawler takes hold of its surface */
BhpShot *bhp_shot_add(BhpStage *s, int kind, int32_t x, int32_t y, int32_t vx, int32_t vy);
void bhp_set_tile(BhpStage *s, int c, int r, int t);
int bhp_tile_at(const BhpStage *s, int px, int py); /* BTL_WALL off the stage */
bool bhp_ent_deadly(const BhpEnt *e);
void bhp_ent_box(const BhpEnt *e, int *x0, int *y0, int *x1, int *y1);
/* the slash's box this frame (false: not striking) */
bool bhp_slash_box(const BhpStage *s, int *x0, int *y0, int *x1, int *y1);
void bhp_refuel(BhpStage *s, int amount);
int bhp_isqrt(int v);
int bhp_sin(int a); /* a in 1/256 turns; -127..127 */
int bhp_cos(int a);
/* aim a speed at (tx, ty) from (x, y), all 1/256 px */
void bhp_aim(int32_t x, int32_t y, int32_t tx, int32_t ty, int32_t speed, int32_t *vx, int32_t *vy);

/* ---- bosses (bellhop_boss.c) ----------------------------------------------- */
void bhp_boss_init(BhpStage *s);
void bhp_boss_step(BhpStage *s);
/* the slash struck boss part e */
void bhp_boss_ent_slash(BhpStage *s, BhpEnt *e);
/* does the slash box strike the boss itself (Lady Hush's ship)? */
bool bhp_boss_slash(BhpStage *s, int x0, int y0, int x1, int y1);
bool bhp_boss_hits_ship(const BhpStage *s);
int bhp_boss_progress(const BhpStage *s); /* goes up as the boss is worn down (the planner) */
void bhp_boss_finish(BhpStage *s);        /* tests: beat it now */
/* boss drawing helpers' geometry */
#define BHP_MILL_N 6
#define BHP_COG_N 5
void bhp_mill_bucket_pos(const BhpStage *s, int k, int *x, int *y);
void bhp_cog_lamp_pos(const BhpStage *s, int wheel, int k, int *x, int *y);
extern const int BHP_COG_X[2], BHP_COG_Y, BHP_COG_R, BHP_LAMP_R, BHP_MILL_X, BHP_MILL_Y, BHP_MILL_R;
extern const int BHP_CHUTE_X0, BHP_CHUTE_X1, BHP_CHUTE_Y0, BHP_CHUTE_Y1;
extern const int BHP_LID_X, BHP_LID_Y;

/* ---- the demo pilot (bellhop_bot.c) ---------------------------------------- */
typedef struct BhpBot {
    uint8_t stage;             /* the stage this memory is for (255: none) */
    uint8_t step;              /* where it is on the route */
    uint16_t ver;
    int16_t tx, ty;            /* the field's target, px */
    uint8_t plan, plan_left;
    uint8_t want_warp;         /* take a warp sparkle when it shows */
    uint8_t prev_slash;
    uint16_t stuck;
    uint16_t field[42][80];
} BhpBot;
void bhp_bot_reset(BhpBot *b, bool warps);
unsigned bhp_bot(const BhpStage *s, BhpBot *b);
int bhp_bot_route_len(int stage);
void bhp_bot_dump(const BhpBot *b);
extern int bhp_bot_debug;
extern int bhp_bot_rounds;

/* ---- art & audio ------------------------------------------------------- */
void bhp_art_load(void);
bool bhp_art_ok(void);
void bhp_draw_stage(const BhpStage *s, int t, bool owl);
void bhp_draw_world_backdrop(int world, int t);
void bhp_draw_cup(int x, int y, bool full);
void bhp_draw_cup_big(int x, int y, bool full);
void bhp_draw_ship(int x, int y, int face, bool flame, int t);
void bhp_draw_lady(int x, int y, int scale);
void bhp_draw_ansel(int x, int y, int scale);

void bhp_audio_load(void);
extern int BHP_MUS_TITLE, BHP_MUS_WORLD[BHP_WORLDS], BHP_MUS_BONUS, BHP_MUS_BOSS, BHP_MUS_LADY,
    BHP_MUS_END, BHP_MUS_CLEAR, BHP_MUS_OVER, BHP_MUS_TEA;

#endif
