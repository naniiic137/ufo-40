/* FLINTHOLD - shared declarations. Cartridge 30 of UFO 40, a tribute to
 * Rock On! Island (UFO 50 #30). See docs/games/30-flinthold.md. */
#ifndef FLINTHOLD_H
#define FLINTHOLD_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define FH_W 20            /* a stage is 20 x 10 tiles, one screen */
#define FH_H 10
#define FH_T 16            /* tile size in pixels */
#define FH_OY 20           /* the field's top on screen (the HUD is above) */
#define FH_U 16            /* positions are in 1/16 pixel */
#define FH_TU (FH_T * FH_U)

#define FH_STAGES 10       /* ten stages ... */
#define FH_VILLAGES 3      /* ... and three villages, each a short defence */
#define FH_LEVELS (FH_STAGES + FH_VILLAGES)
#define FH_FINAL 9         /* the last stage: the Four Lords */
#define FH_V_HOME 10       /* Hearthome, where Pim starts */
#define FH_V_CAMP 11       /* the Scale Camp: the second village (hidden) */
#define FH_V_ISLE 12       /* Far Isle (hidden) */

#define FH_CAVE_HP 30
#define FH_MEAT_MAX 99
#define FH_START_MEAT 30
#define FH_WAVE_MEAT 20    /* per spawn point, after every wave */

enum { FH_RIGHT, FH_DOWN, FH_LEFT, FH_UP };
extern const int FH_DX[4], FH_DY[4];

/* ---- stage data (flinthold_levels.c) ------------------------------------ */

/* A stage as drawn. Legend:
 *  .  grass (build here, unless it is next to the path)   ,  grass, flowers
 *  #  thicket (nobody walks through; blocks shots but arrows)
 *  b  bush (dig it out for 5 meat)   r  boulder (dig it out for 10 meat)
 *  ~  water    C  the cave    h  a hut (villages)    1-4  a villager
 * The path is not drawn in the rows: it is laid by the routes. */
#define FH_MAX_ROUTES 4
#define FH_MAX_WAY 24
typedef struct FhRoute {
    int n;
    int8_t x[FH_MAX_WAY], y[FH_MAX_WAY]; /* corners, tile coordinates; the last is the cave */
} FhRoute;

typedef struct FhStage {
    const char *name;
    int theme;             /* 0 meadow, 1 fern, 2 ash, 3 oasis, 4 jungle, 5 cliffs, 6 volcano, 7 village */
    int boss_song;         /* which battle tune */
    const char *rows[FH_H];
    int n_routes;
    FhRoute route[FH_MAX_ROUTES];
    /* waves separated by '|', groups by spaces: COUNT KIND [@ROUTE | *]
     * kinds: n nipper, r redback, f fangcat, c clubtail, g glider, q gnat,
     *        M shagtusk, J Old Jaw, P Lord Plate, G Lord Gale, S Lord Snap */
    const char *waves;
} FhStage;
extern const FhStage FH_STAGE[FH_LEVELS];

/* a villager's words (villages) */
typedef struct FhTalk { const char *who; const char *line; } FhTalk;
extern const FhTalk FH_TALK[FH_VILLAGES][4];

/* ---- foes ------------------------------------------------------------------ */
enum {
    E_NIPPER,   /* baby: 50 hp */
    E_REDBACK,  /* red: 80 hp, light armour */
    E_FANGCAT,  /* sabre cat: 40 hp, twice as fast */
    E_CLUBTAIL, /* armoured: 120 hp, 80 % armour */
    E_GLIDER,   /* flies straight to the cave, slow */
    E_GNAT,     /* flies; goes for Pim when she is near */
    E_SHAGTUSK, /* boss: 2000 */
    E_JAW,      /* boss: 3000, 30 to the cave */
    E_PLATE,    /* boss: 4000, all armour */
    E_GALE,     /* boss: flies */
    E_SNAP,     /* boss: quick */
    E_KINDS
};
typedef struct FhFoeDef {
    const char *name;
    int hp;               /* whole hit points */
    int res_normal, res_fire, res_arrow; /* per cent */
    int speed;            /* 1/64 px per frame */
    int meat, cave;
    int flies, boss;
} FhFoeDef;
extern const FhFoeDef FH_FOE[E_KINDS];

/* ---- towers ---------------------------------------------------------------- */
enum {
    U_THROWER,  /* the plain one: a bone every 5/6 s */
    U_SPEAR, U_BARB, U_BOW,
    U_SLING, U_HURLER, U_BOULDER,
    U_TORCH, U_BLAZE, U_PITCH,
    U_KINDS
};
typedef struct FhUnitDef {
    const char *name;
    int cost;             /* to hire (thrower) or to upgrade into */
    int from;             /* what it upgrades from (-1 for the thrower) */
    int dmg;              /* per hit */
    int every;            /* frames between attacks */
    int range;            /* tiles */
    int dtype;
    int shot_speed;       /* 1/16 px per frame */
} FhUnitDef;
extern const FhUnitDef FH_UNIT[U_KINDS];
enum { D_NORMAL, D_FIRE, D_ARROW, D_PIERCE };

/* ---- the stage simulation (flinthold_logic.c) ------------------------------ */

enum { O_NONE, O_UNIT, O_HEN, O_FIRE, O_NPC };
enum { TL_GRASS, TL_FLOWER, TL_PATH, TL_THICKET, TL_BUSH, TL_ROCK, TL_WATER, TL_CAVE, TL_HUT };
enum { PH_BUILD, PH_BATTLE, PH_WON, PH_LOST };
enum { Z_OK, Z_STUN, Z_GONE };

#define FH_MAX_UNITS 64
#define FH_MAX_FOES 96
#define FH_MAX_SHOTS 160
#define FH_MAX_ROLLS 16
#define FH_MAX_WAVES 20
#define FH_MAX_GROUPS 12
#define FH_MAX_EV 32
#define FH_MENU_MAX 6

typedef struct FhGroup { int8_t kind, route, count; } FhGroup;
typedef struct FhWave { int n; FhGroup g[FH_MAX_GROUPS]; } FhWave;

typedef struct FhUnit {
    uint8_t on, kind, x, y;
    int16_t cool;
    int16_t spent;        /* meat put into it (selling gives half back) */
    int16_t aim_x, aim_y; /* last target, for drawing (pixels) */
    int16_t throw_t;      /* frames since the last throw, for drawing */
} FhUnit;

typedef struct FhFoe {
    uint8_t on, kind, route;
    int32_t hp;           /* tenths of a hit point */
    int32_t dist;         /* along the route (1/16 px); fliers: along the flight */
    int32_t x, y;         /* 1/16 px */
    int16_t slow_t, tar_t, flash_t;
    uint8_t tarred, sub;  /* sub: 1/64 px carried over */
    int8_t face;          /* for drawing: -1 left, 1 right */
    int32_t fx0, fy0;     /* fliers: where they set off */
} FhFoe;

enum { SH_BONE, SH_SPEAR, SH_BARB, SH_ARROW, SH_ROCK, SH_FIRE, SH_EMBER, SH_TAR, SH_PIM };
typedef struct FhShot {
    uint8_t on, kind, dtype;
    int16_t target;       /* foe index to home on (-1: straight) */
    int32_t x, y, vx, vy; /* 1/16 px */
    int32_t dmg;          /* tenths */
    int16_t life;
    int16_t slow;         /* frames of slow it gives (barb) or tar */
    uint8_t follow;       /* arrows: frames of follow-through left after a hit */
    uint8_t hit_n;
    uint8_t hits[8];
} FhShot;

typedef struct FhRoll {
    uint8_t on;
    int32_t x, y, vx, vy;
    int32_t left;         /* 1/16 px of rolling left once on the path */
    int32_t budget;       /* total 1/16 px it may still roll */
    uint8_t on_path;
    int32_t dmg;          /* tenths per frame */
} FhRoll;

enum { EV_KILL, EV_HIT, EV_CAVE, EV_PIM_HIT, EV_BUILD, EV_SELL, EV_DIG, EV_NOPE, EV_WAVE_END, EV_COOK,
       EV_THROW, EV_SPAWN, EV_UPGRADE, EV_WON, EV_LOST, EV_MEAT };
typedef struct FhEvent { uint8_t kind; int16_t x, y; int16_t a; } FhEvent;

/* menu entries (what A offers on the faced tile) */
enum { M_THROWER, M_HEN, M_FIRE, M_DIG, M_UPGRADE, M_SELL, M_TALK, M_FIGHT, M_THROW_UP, M_WEAPON_UP, M_CLOSE };
typedef struct FhMenuItem { uint8_t act, arg; int16_t cost; uint8_t ok; } FhMenuItem;

typedef struct FhSim {
    int level;
    uint8_t tile[FH_H][FH_W];
    uint8_t obj[FH_H][FH_W];      /* O_* */
    uint8_t oarg[FH_H][FH_W];     /* unit index, hen cooking (0 raw, 1 half, 2 done), npc id */
    uint8_t near_path[FH_H][FH_W];
    int n_spawns;
    int cave_x, cave_y;
    /* routes compiled to pixel corners (1/16 px) and their lengths */
    int n_routes;
    int32_t rx[FH_MAX_ROUTES][FH_MAX_WAY], ry[FH_MAX_ROUTES][FH_MAX_WAY];
    int rn[FH_MAX_ROUTES];
    int32_t rlen[FH_MAX_ROUTES];

    int n_waves, wave;            /* wave = index of the next (or current) wave */
    FhWave waves[FH_MAX_WAVES];
    int phase, phase_t;
    /* the running wave: which group is coming out and how many are left */
    int grp, grp_left, spawn_t;

    int meat, cave_hp, cave_hits, wasted;
    int kills, spent_upgrades, most_units, most_hens;

    /* Pim */
    int32_t px, py;
    int face, zstate, zt, throw_cool, throw_lv, weapon_lv, throwing, walk_t;

    /* the A menu */
    int menu_open, menu_sel, menu_n, menu_tx, menu_ty;
    FhMenuItem menu[FH_MENU_MAX];
    int talk_npc, talk_t;         /* a villager speaking (-1 none) */

    FhUnit unit[FH_MAX_UNITS];
    FhFoe foe[FH_MAX_FOES];
    FhShot shot[FH_MAX_SHOTS];
    FhRoll roll[FH_MAX_ROLLS];

    FhEvent ev[FH_MAX_EV];
    int n_ev;
    int frame;
    int god;                      /* tests: the cave takes no damage */
    int no_spawn;                 /* tests: waves bring nobody */
    int hold_foes;                /* tests: foes stand where they are */
} FhSim;
extern FhSim fh;

void fh_sim_start(int level);
/* one frame: held buttons and the ones pressed this frame */
void fh_sim_step(uint32_t held, uint32_t pressed);
int fh_faced(int *tx, int *ty);   /* the tile Pim is facing; 0 if off the field */
void fh_menu_build(int tx, int ty);
int fh_menu_do(int i);            /* carry out menu item i; 1 if done */
bool fh_buildable(int x, int y);
bool fh_walkable(int x, int y);
bool fh_blocks_shot(int x, int y);
int fh_fires_near(int x, int y);
int fh_unit_count(void);
int fh_hen_count(void);
int fh_foe_count(void);
int fh_add_foe(int kind, int route);
void fh_foe_pos(const FhFoe *f, int *x, int *y); /* pixels on the field */
int fh_foe_pos_at(int route, int32_t dist, int32_t *x, int32_t *y);
int fh_next_wave_kinds(uint8_t kinds[E_KINDS]);
int fh_throw_every(void);
int fh_throw_range(void);         /* tiles */
int fh_weapon_dmg(void);
int fh_sell_value(int x, int y);
const char *fh_menu_label(const FhMenuItem *m, char *buf, int n);

/* the demo player: returns the buttons to hold this frame */
int fh_bot_buttons(void);
void fh_bot_reset(void);
extern int fh_bot_step;           /* how far through its plan it is */

/* ---- art and audio ------------------------------------------------------- */
enum {
    FS_PIM_D0, FS_PIM_D1, FS_PIM_U0, FS_PIM_U1, FS_PIM_S0, FS_PIM_S1, FS_PIM_THROW_D, FS_PIM_THROW_U, FS_PIM_THROW_S,
    FS_PIM_HURT,
    FS_THROWER, FS_SPEAR, FS_BARB, FS_BOW, FS_SLING, FS_HURLER, FS_BOULDER, FS_TORCH, FS_BLAZE, FS_PITCH,
    FS_HEN, FS_FIRE0, FS_FIRE1,
    FS_NIPPER0, FS_NIPPER1, FS_REDBACK0, FS_REDBACK1, FS_FANGCAT0, FS_FANGCAT1, FS_CLUBTAIL0, FS_CLUBTAIL1,
    FS_GLIDER0, FS_GLIDER1, FS_GNAT0, FS_GNAT1,
    FS_SHAGTUSK0, FS_SHAGTUSK1, FS_JAW0, FS_JAW1, FS_PLATE0, FS_PLATE1, FS_GALE0, FS_GALE1, FS_SNAP0, FS_SNAP1,
    FS_ELDER, FS_BABY, FS_HENWIFE, FS_SCALEFOLK, FS_TUSKLING,
    FS_MEAT, FS_HEART, FS_PIM_BIG, FS_CAVE,
    FS_COUNT
};
extern Sprite fh_spr[FS_COUNT];
extern int fh_art_bad;
void fh_art_load(void);
void fh_audio_load(void);
extern int FH_MUS_TITLE, FH_MUS_MAP, FH_MUS_VILLAGE, FH_MUS_BATTLE1, FH_MUS_BATTLE2, FH_MUS_BATTLE3, FH_MUS_LORDS,
    FH_MUS_END, FH_MUS_WAVE, FH_MUS_CLEAR, FH_MUS_FELL;

#endif
