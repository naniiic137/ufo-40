/* DUNE EXPRESS - shared declarations. Cartridge 28 of UFO 40.
 * A tribute to Rail Heist (UFO 50 #28); see docs/games/28-dune-express.md. */
#ifndef DUNE_H
#define DUNE_H

#ifndef DX_NO_SHELL
#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#else
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../../engine/rng.h"
#endif

#define DX_T 16                 /* tile size in pixels */
#define DX_H 10                 /* rows in every train */
#define DX_MAX_W 200            /* columns */
#define DX_MISSIONS 20
#define DX_MAX_ACTORS 24
#define DX_MAX_OBJS 96
#define DX_MAX_SHOTS 16
#define DX_MAX_CARS 16
#define DX_TURN_FRAMES 600      /* the outlaws' turn: 10 seconds */
#define DX_LAW_FRAMES 210       /* the lawmen's turn: 3.5 seconds */
#define DX_STUN 180             /* 3 seconds */
#define DX_DRAW 130             /* loading and drawing the gun: just over 2 s */
#define DX_QUICKDRAW 60
#define DX_MAX_AMMO 6
#define DX_DEATH_Y (9 * DX_T)   /* below the undercarriage: off the train */
#define DX_TILE_HP 3            /* punches a plank wall, floor or roof takes */
#define DX_ARMOR_HP 30          /* and a plate of armor */
#define DX_GUN_SHOTS 6          /* a crank gun: two bursts of three */
#define DX_END_FRAMES 70        /* a failed heist: the fall plays out first */

/* tiles */
enum {
    TL_AIR = 0, TL_ROOF, TL_WALL, TL_FLOOR, TL_ARMOR, TL_GATE, TL_GATE_OPEN, TL_LADDER, TL_SHELF,
    TL_WHEELS, TL_COUPLING, TL_CELL
};

/* actors */
enum { AK_OUTLAW, AK_GUARD, AK_GOVERNOR };
enum { OUT_WADE, OUT_HUSH, OUT_PEARL };
enum { LS_GUARD, LS_PATROL, LS_ALERT };

typedef struct DxActor {
    uint8_t kind, who, alive, face;     /* face: 0 left, 1 right */
    float x, y, vx, vy;                 /* x, y: top-left of a 12 x 30 box (12 x 14 ducking) */
    uint8_t ground, duck, climb, hidden;/* hidden: inside a barrel */
    int16_t carry;                      /* object index, -1 */
    int16_t stun;
    uint8_t state, armed;               /* lawmen */
    int16_t gun;                        /* outlaws: 0 holstered, >0 loading, -1 drawn */
    uint8_t ammo, jumps;
    uint8_t charm, fist, boots, blast, quick; /* power-ups */
    uint8_t escaped;                    /* reached the camel */
    uint8_t punch_t, anim;
    uint8_t carries_default;            /* a lawman who starts carrying something */
    int16_t throw_cd;
    uint8_t rolling, jump_hold, dying;  /* dying: a shot body flying off the train */
    uint8_t has_tgt;                    /* lawmen: a place to search (a noise, a sighting) */
    float tgt_x, tgt_y;                 /* tgt_y: the feet */
    int16_t push_t, look_t;
} DxActor;

/* things */
enum {
    OB_NONE = 0, OB_BARREL, OB_CRATE, OB_ANVIL, OB_DYNAMITE, OB_GOOSE, OB_RAM, OB_GUN, OB_LEVER,
    OB_LOOT, OB_COIN, OB_AMMO, OB_POWER, OB_CAMEL
};
enum { PW_NONE = 0, PW_ROUNDS, PW_BLAST, PW_BOOTS, PW_CHARM, PW_FIST, PW_QUICK };

typedef struct DxObj {
    uint8_t type, alive, face;
    float x, y, vx, vy;
    uint8_t w, h;
    int16_t held;           /* actor index, -1 */
    int16_t thrower;        /* actor index + 1 while flying from a throw, 0 otherwise */
    uint8_t content;        /* PW_* hidden in a barrel or crate; for loot: its id */
    uint8_t marked;         /* loot: 1 shows as a strongbox, 0 looks like a plain crate */
    int16_t timer;          /* dynamite fuse, gun flash, ram charge */
    int8_t ammo;            /* the crank gun's bullets */
    uint8_t friendly;       /* crank gun turned on the lawmen */
    uint8_t loot_id;        /* coins remember whose box they came from */
    uint8_t suspect;        /* a barrel a lawman saw roll: he'll shoot it once alert */
    uint8_t burst;          /* crank gun: shots left in this burst */
    int8_t roll;            /* an occupied barrel rolling: -1, 0, 1 */
} DxObj;

typedef struct DxShot {
    uint8_t alive, blast;
    int8_t owner;           /* actor index, or -2 for a crank gun */
    float x, y, vx;
    uint8_t hostile;        /* 1: fired at outlaws */
    uint8_t tracer;         /* only a streak of light: the hit was already dealt */
    float range;            /* a tracer's length */
} DxShot;

enum { PH_FREE = 0, PH_PLAYER, PH_LAW };
enum { DX_PLAYING = 0, DX_WON, DX_LOST };
enum { WHY_NONE = 0, WHY_SHOT, WHY_FELL, WHY_CRUSHED, WHY_BLAST, WHY_TRAMPLED, WHY_TIME, WHY_GUN };
enum { OBJ_LOOT = 0, OBJ_RESCUE, OBJ_AMMO, OBJ_GOVERNOR };

typedef struct DxMission {
    const char *name;
    uint8_t outlaws;        /* 1 or 2 */
    uint8_t who[2];         /* OUT_* */
    uint8_t objective, need;
    int16_t seconds, star_seconds; /* the master timer and the time star */
    uint8_t start_ammo;
    const char *brief;
    const char *tiles[DX_H];
    const char *things[DX_H];
} DxMission;
extern const DxMission DX_MISSIONS_DEF[DX_MISSIONS];

typedef struct DxInput {
    uint8_t left, right, up, down, a, b, a_pressed, b_pressed, down_pressed;
} DxInput;

typedef struct DxWorld {
    int w;
    uint8_t tile[DX_H][DX_MAX_W];
    uint8_t car_of[DX_MAX_W];       /* car index per column, 255 in a gap */
    uint8_t ncars;
    int16_t car_x0[DX_MAX_CARS], car_x1[DX_MAX_CARS];
    DxActor a[DX_MAX_ACTORS];
    int na;
    DxObj o[DX_MAX_OBJS];
    int no;
    DxShot s[DX_MAX_SHOTS];
    int ctrl;                       /* the actor the player controls */
    uint8_t phase;
    int32_t turn_t, master;         /* frames left */
    int32_t elapsed;                /* frames of master time used */
    uint8_t objective, need, loot, mission;
    uint8_t result, why;
    uint8_t kills, lawmen_total, governor_dead;
    uint8_t loot_coins[8];          /* coins picked up, per box */
    uint8_t loot_done[8];
    uint8_t turns;                  /* lawmen turns so far (2P) */
    uint8_t versus;                 /* 2P: player 2 drives one lawman a turn */
    uint8_t law_ctrl;               /* the lawman player 2 drives */
    uint8_t coins;                  /* 2P: coins the outlaws hold */
    int16_t ev_shot, ev_boom, ev_punch, ev_coin, ev_stun, ev_break, ev_gate, ev_alert, ev_turn; /* sounds */
    uint8_t rescue_open;            /* mission 13: the cell was opened */
    uint8_t dmg[DX_H][DX_MAX_W];    /* punches a tile has taken */
    uint8_t tflash[DX_H][DX_MAX_W]; /* a struck tile flashes white */
    int16_t end_t;                  /* frames until a failed heist ends */
    Rng rng;
} DxWorld;

/* the rules (dune_logic.c) */
int dx_load(DxWorld *w, const DxMission *m, int mission);
void dx_step(DxWorld *w, const DxInput *in, const DxInput *in2);
bool dx_solid(const DxWorld *w, int tx, int ty);
int dx_tile(const DxWorld *w, int tx, int ty);
int dx_car_at(const DxWorld *w, float x);
bool dx_sees(const DxWorld *w, int lawman, int target);
int dx_active_lawmen(const DxWorld *w);
void dx_swap_outlaw(DxWorld *w);
int dx_stars(const DxWorld *w);     /* bit 0 angel, 1 devil, 2 time */
void dx_versus_train(DxWorld *w, uint64_t seed);

#ifndef DX_NO_SHELL
/* art & audio */
enum {
    DS_STAND, DS_WALK1, DS_WALK2, DS_DUCK, DS_ROLL, DS_JUMP, DS_CLIMB, DS_PUNCH, DS_GUN, DS_DOWN, DS_FRAMES
};
enum {
    DO_BARREL, DO_CRATE, DO_ANVIL, DO_DYNAMITE, DO_GOOSE, DO_GOOSE2, DO_RAM, DO_GUN, DO_LEVER, DO_LOOT, DO_COIN,
    DO_AMMO, DO_POWER, DO_CAMEL, DO_CAMEL2, DO_HAT_W, DO_HAT_H, DO_HAT_P, DO_HAT_G, DO_HAT_GOV, DO_ICONS, DO_COUNT
};
extern Sprite dx_body[DS_FRAMES];
extern Sprite dx_obj[DO_COUNT];
extern Sprite dx_icon[8];
void dx_art_load(void);
void dx_audio_load(void);
extern int DX_MUS_TITLE, DX_MUS_QUIET, DX_MUS_TENSE, DX_MUS_WIN, DX_MUS_LOSE, DX_MUS_MAP, DX_MUS_END;
#endif

#endif
