/* ROOFCAT - shared declarations. Cartridge 03 of UFO 40. */
#ifndef ROOFCAT_H
#define ROOFCAT_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define RC_ROWS 10
#define RC_CHUNK_W 20
#define RC_TILE 16
#define RC_HUD 20
#define RC_MAX_CHUNKS 14
#define RC_STAGES 4

/* level chunks: 10 rows x 20 columns, see roofcat_levels.c for the legend */
typedef struct Chunk {
    const char *name;
    const char *rows[RC_ROWS];
} Chunk;

enum { BOSS_PIGEON, BOSS_POTS, BOSS_CRAB, BOSS_MAGPIE };

typedef struct StageDef {
    const char *name;
    const char *subtitle;
    uint8_t theme;           /* 0 whitewash, 1 souk, 2 harbour, 3 fort */
    uint8_t n_chunks;
    uint8_t chunks[RC_MAX_CHUNKS]; /* indexes into RC_CHUNKS; last one is the arena */
    uint8_t letter_chunk, letter_x, letter_y;
    uint8_t boss;
    int music;               /* index into stage music table */
} StageDef;

extern const Chunk RC_CHUNKS[];
extern const int RC_CHUNK_COUNT;
extern const StageDef RC_STAGES_DEF[RC_STAGES];

enum {
    R_CAT_RUN1, R_CAT_RUN2, R_CAT_RUN3, R_CAT_RUN4, R_CAT_JUMP, R_CAT_SPIN, R_CAT_FALL, R_CAT_THROW,
    R_CAT_SPIRIT1, R_CAT_SPIRIT2, R_CAT_HURT, R_CAT_IDLE,
    R_STAR1, R_STAR2, R_WISP_SHOT,
    R_PIGEON1, R_PIGEON2, R_GULL1, R_GULL2, R_RAT1, R_RAT2, R_SLINGER1, R_SLINGER2,
    R_MIRAGE1, R_MIRAGE2, R_BOBBER1, R_BOBBER2, R_VENT1, R_VENT2, R_MAGPIE1, R_MAGPIE2,
    R_PEBBLE, R_SEED, R_BUBBLE, R_BOMB, R_SHARD,
    R_FISH, R_DATE, R_ORANGE, R_POMEGRANATE, R_BREAD, R_LETTER, R_LANTERN, R_CATNIP, R_HEART,
    R_BOSS_PIGEON1, R_BOSS_PIGEON2, R_POT, R_POT_EYES, R_CRAB1, R_CRAB2, R_MKING1, R_MKING2,
    R_PARCEL, R_CAT_HEAD, R_POW,
    R_SPRITE_COUNT
};
extern Sprite rc_spr[R_SPRITE_COUNT];
void rc_art_load(void);
void rc_audio_load(void);
extern int RC_MUS_STAGE[RC_STAGES], RC_MUS_BOSS, RC_MUS_CLEAR, RC_MUS_OVER, RC_MUS_END, RC_MUS_TITLE;

#endif
