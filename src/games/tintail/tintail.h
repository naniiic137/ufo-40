/* TINTAIL - shared declarations. Cartridge 16 of UFO 40.
 * A tribute to Camouflage (UFO 50 #16); see docs/games/16-tintail.md. */
#ifndef TINTAIL_H
#define TINTAIL_H

#ifndef TT_NO_SHELL
#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#else
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#endif

#define TT_W 20
#define TT_H 10
#define TT_LEVELS 15
#define TT_MAX_TOADS 10
#define TT_MAX_STORKS 6
#define TT_MAX_SWITCH 4
#define TT_PATH_MAX 48
#define TT_BEAT 8          /* frames per beat */
#define TT_CAMO_BEATS 2    /* changing colour takes two beats */
#define TT_NONE 255

/* colours a tile can have (and the chameleon can take) */
enum { TC_NONE = 0, TC_GRASS, TC_SAND, TC_SWAMP, TC_ROCK, TC_COUNT }; /* dry grass wears the sand colour */

/* terrain kinds */
enum {
    TT_GRASS = 0, TT_SAND, TT_SWAMP, TT_ROCK, TT_SWITCHGRASS, /* walkable ground */
    TT_SEA, TT_PALM, TT_BOULDER, TT_BUSH,                      /* solid */
    TT_LOG_H, TT_LOG_V, TT_HOLE
};

enum { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT };

/* switch-grass state: the level's start mix, all wet or all dry */
enum { SW_START = 0, SW_WET, SW_DRY };

typedef struct TtStork {
    uint8_t len, speed;               /* moves in the loop, beats per step */
    uint8_t px[TT_PATH_MAX], py[TT_PATH_MAX], dir[TT_PATH_MAX]; /* position before move i, and move i */
} TtStork;

/* the fixed part of a level */
typedef struct TtLevel {
    uint8_t kind[TT_H][TT_W];
    uint8_t wet0[TT_H][TT_W];         /* switch grass: 1 = starts wet */
    uint8_t start_x, start_y, hole_x, hole_y;
    uint8_t nfruit, fruit_x[2], fruit_y[2];
    uint8_t baby_x, baby_y;           /* TT_NONE: no hatchling */
    uint8_t ntoad, toad_x[TT_MAX_TOADS], toad_y[TT_MAX_TOADS], toad_dir[TT_MAX_TOADS];
    uint8_t nstork;
    TtStork stork[TT_MAX_STORKS];
    uint8_t nrain, rain_x[TT_MAX_SWITCH], rain_y[TT_MAX_SWITCH];
    uint8_t nsun, sun_x[TT_MAX_SWITCH], sun_y[TT_MAX_SWITCH];
    int period;                       /* beats until every stork is back where it began */
} TtLevel;

/* everything that changes */
typedef struct TtState {
    uint32_t beat;
    uint8_t x, y, face;
    uint8_t px, py;                   /* the tile the chameleon was on before its last step */
    uint8_t camo, camo_t;             /* colour; beats left of a colour change */
    uint8_t baby;                     /* 0 waiting, 1 following */
    uint8_t bx, by;
    uint8_t fruit;                    /* bit per fruit */
    uint8_t sw;                       /* SW_* */
    uint8_t dead, won;
    uint8_t eaten_by, eater;          /* 0 toad, 1 stork, 2 falcon; predator index */
    uint8_t baby_eaten;
} TtState;

/* actions, one per beat */
enum { ACT_WAIT = 0, ACT_UP, ACT_RIGHT, ACT_DOWN, ACT_LEFT, ACT_CAMO };

/* what happened in a beat, for sounds and effects */
enum { TE_STEP = 1, TE_BUMP = 2, TE_CAMO_START = 4, TE_CAMO_DONE = 8, TE_FRUIT = 16, TE_BABY = 32,
       TE_RAIN = 64, TE_SUN = 128, TE_WIN = 256, TE_EATEN = 512, TE_REFUSED = 1024, TE_LOG = 2048 };

/* rules (tintail_logic.c) */
typedef struct TtLevelDef {
    const char *name;
    const char *ground[TT_H];   /* terrain */
    const char *things[TT_H];   /* chameleon, fruit, hatchling, toads, switches */
    const char *storks[TT_MAX_STORKS]; /* "x y speed MOVES" */
} TtLevelDef;

int tt_parse(const TtLevelDef *def, TtLevel *lv, TtState *st); /* 0 on success */
void tt_reset(const TtLevel *lv, TtState *st);
int tt_colour(const TtLevel *lv, const TtState *st, int x, int y);
bool tt_walkable(const TtLevel *lv, int x, int y);
void tt_stork_at(const TtLevel *lv, int i, uint32_t beat, int *x, int *y, int *dir);
/* danger[y][x] = predator id + 1 (toads 1..10, storks 11..16), 0 if safe */
void tt_danger(const TtLevel *lv, const TtState *st, uint32_t beat, uint8_t danger[TT_H][TT_W]);
bool tt_hidden(const TtLevel *lv, const TtState *st, int who); /* 0 chameleon, 1 hatchling */
bool tt_can_camo(const TtLevel *lv, const TtState *st);
int tt_step(const TtLevel *lv, TtState *st, int act); /* one beat; returns TE_* flags */
int tt_collected(const TtState *st);                   /* 0..3 */

/* The solver: fewest beats to escape. need_all: with both fruit and the
 * hatchling. Writes the actions (one char per beat: . U R D L C) into out
 * (NUL-terminated) and returns the number of beats, or -1. */
int tt_solve(const TtLevel *lv, bool need_all, char *out, int out_max);

extern const TtLevelDef TT_LEVEL_DEFS[TT_LEVELS];

#ifndef TT_NO_SHELL
/* art & audio */
enum {
    TS_KAMA_R, TS_KAMA_R2, TS_KAMA_U, TS_KAMA_U2, TS_KAMA_D, TS_KAMA_D2,
    TS_BABY_R, TS_BABY_U, TS_BABY_D,
    TS_TOAD_R, TS_TOAD_U, TS_TOAD_D, TS_TOAD_BLINK, TS_TOAD_BLINK_U, TS_TOAD_BLINK_D,
    TS_STORK_R, TS_STORK_R2, TS_STORK_U, TS_STORK_U2, TS_STORK_D, TS_STORK_D2,
    TS_FALCON, TS_FALCON2, TS_PEAR, TS_PALM, TS_BOULDER, TS_BUSH, TS_RAIN, TS_SUN,
    TS_HERO, TS_SPRITE_COUNT
};
extern Sprite tt_spr[TS_SPRITE_COUNT];
void tt_art_load(void);
void tt_audio_load(void);
extern int TT_MUS_MAP, TT_MUS_LEVEL, TT_MUS_GATE, TT_MUS_WIN, TT_MUS_EATEN, TT_MUS_END, TT_MUS_TITLE;
#endif

#endif
