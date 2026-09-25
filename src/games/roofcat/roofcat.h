/* ROOFCAT - shared declarations. Cartridge 03 of UFO 40.
 * A tribute to Ninpek (UFO 50 #3); see docs/games/03-roofcat.md. */
#ifndef ROOFCAT_H
#define ROOFCAT_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define RC_ROWS 10
#define RC_CHUNK_W 20
#define RC_TILE 16
#define RC_HUD 20
#define RC_AREAS 4
#define RC_AREA_CHUNKS 12
/* one continuous town: four areas of twelve screens, then the harbour arena */
#define RC_WORLD_CHUNKS (RC_AREAS * RC_AREA_CHUNKS + 1)

/* level chunks: 10 rows x 20 columns, see roofcat_levels.c for the legend */
typedef struct Chunk {
    const char *name;
    const char *rows[RC_ROWS];
} Chunk;

extern const Chunk RC_WORLD[RC_WORLD_CHUNKS];
extern const char *const RC_AREA_NAME[RC_AREAS];

enum {
    R_CAT_RUN1, R_CAT_RUN2, R_CAT_RUN3, R_CAT_RUN4, R_CAT_JUMP, R_CAT_SPIN, R_CAT_FALL, R_CAT_THROW,
    R_CAT_SPIRIT1, R_CAT_SPIRIT2, R_CAT_HURT, R_CAT_IDLE,
    R_STAR1, R_STAR2, R_WISP_SHOT,
    R_PIGEON1, R_PIGEON2, R_PIGEON_ROLL, R_GULL1, R_GULL2, R_CROW1, R_CROW2, R_GECKO1, R_GECKO2,
    R_LAMP, R_SPIKE, R_FFISH1, R_FFISH2, R_WASP1, R_WASP2, R_SNAIL1, R_SNAIL2, R_TOAD1, R_TOAD2,
    R_PELICAN1, R_PELICAN2, R_JAR1, R_JAR2, R_CRACKER1, R_CRACKER2, R_PUFFER1, R_PUFFER2,
    R_SPIDER1, R_SPIDER2, R_MAGPIE1, R_MAGPIE2, R_FLASHER1, R_FLASHER2, R_SHEET1, R_SHEET2,
    R_MOTH1, R_MOTH2,
    R_PEBBLE, R_SEED, R_BUBBLE, R_BOMB, R_ORB,
    R_FISH, R_DATE, R_FIG, R_TEA, R_CROWN, R_LETTER, R_LANTERN, R_CATNIP, R_HEART,
    R_CRAB1, R_CRAB2, R_CLAW,
    R_PARCEL, R_CAT_HEAD,
    R_SPRITE_COUNT
};
extern Sprite rc_spr[R_SPRITE_COUNT];
void rc_art_load(void);
void rc_audio_load(void);
extern int RC_MUS_AREA[RC_AREAS], RC_MUS_BOSS, RC_MUS_OVER, RC_MUS_END, RC_MUS_TITLE;

#endif
