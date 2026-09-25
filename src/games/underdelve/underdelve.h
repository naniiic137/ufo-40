/* UNDERDELVE - shared declarations. Cartridge 01 of UFO 40,
 * a tribute to Barbuta (UFO 50 #1). See docs/games/01-underdelve.md. */
#ifndef UNDERDELVE_H
#define UNDERDELVE_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define UD_MAP_W 8
#define UD_MAP_H 8
#define UD_ROOM_W 20
#define UD_ROOM_H 10
#define UD_TILE 16
#define UD_HUD_H 20

enum {
    UD_ITEM_POT = 1 << 0,     /* drips bounce off it */
    UD_ITEM_FORK = 1 << 1,    /* the pick pops crystal bubbles */
    UD_ITEM_CRANK = 1 << 2,   /* the mine lifts run */
    UD_ITEM_GLOVES = 1 << 3,  /* faster climbing */
    UD_ITEM_TALLY = 1 << 4,   /* the Deep Gate */
    UD_ITEM_CANARY = 1 << 5,  /* deals with the wisps at the end */
    UD_ITEM_SPARKER = 1 << 6, /* weapon: fires sparks */
    UD_ITEM_HUNGRY = 1 << 7,  /* weapon: double damage */
    UD_ITEM_BOOT = 1 << 8,    /* an old boot. That's all it is. */
    UD_ITEM_LANTERN = 1 << 9, /* shop only */
};
/* a chest holding ore rather than an item */
#define UD_MONEY 0x8000

typedef struct LiftDef {
    int8_t tx, ty;  /* resting tile position (left tile, top surface at tile top) */
    int8_t w;       /* width in tiles */
    int8_t axis;    /* 0 = moves right, 1 = moves up */
    int8_t range;   /* travel in tiles */
    uint8_t period; /* frames per tile of travel */
} LiftDef;

typedef struct RoomDef {
    const char *name;
    const char *rows[UD_ROOM_H];
    uint16_t shop_item[3];  /* pedestals '1' '2' '3' */
    uint16_t shop_price[3];
    uint16_t chest[2];      /* chests 'c' and 'C': an item bit or UD_MONEY | ore */
    uint8_t safe;           /* the Gloom never comes here */
    LiftDef lifts[2];
    uint8_t n_lifts;
    uint8_t keeper;         /* shopkeeper look: 0 toad, 1 owl, 2 lizard */
} RoomDef;

extern const RoomDef UD_ROOMS[UD_MAP_H][UD_MAP_W];

/* ---- sprites ---- */
enum {
    /* hero */
    S_MO_IDLE, S_MO_BLINK, S_MO_WALK1, S_MO_WALK2, S_MO_JUMP, S_MO_FALL,
    S_MO_CLIMB1, S_MO_CLIMB2, S_MO_SWING1, S_MO_SWING2, S_MO_HURT,
    S_PICK_UP, S_PICK_FWD, S_SPARKER_FWD, S_HUNGRY_FWD,
    /* tiles (neutral palette, remapped per zone) */
    S_T_ROCK, S_T_ROCK2, S_T_BRICK, S_T_PLANK, S_T_LADDER, S_T_ROPE, S_T_SPIKES,
    S_T_CRYSTAL, S_T_DRIP, S_T_OOZE1, S_T_OOZE2, S_T_OOZEBODY, S_T_GATE, S_T_SEAL,
    S_T_BEAM, S_T_TORCH1, S_T_TORCH2, S_T_MUSH, S_T_CRYS_DECOR, S_T_BGROCK,
    /* things */
    S_NUGGET, S_GEM1, S_GEM2, S_CHEST, S_CHEST_OPEN, S_CAGE, S_CAGE_OPEN,
    S_SUNSTONE, S_PEDESTAL, S_LIFT,
    S_KEEPER_TOAD1, S_KEEPER_TOAD2, S_KEEPER_OWL1, S_KEEPER_OWL2, S_KEEPER_LIZ1, S_KEEPER_LIZ2,
    S_VENDOR1, S_VENDOR2, S_SMITH1, S_SMITH2, S_LEVER_UP, S_LEVER_DOWN, S_SHRINE, S_SHRINE_OPEN,
    S_PUSH,
    /* item icons 12x12 */
    S_I_POT, S_I_FORK, S_I_CRANK, S_I_GLOVES, S_I_TALLY, S_I_CANARY, S_I_SPARKER, S_I_HUNGRY, S_I_LANTERN,
    S_I_BOOT,
    /* enemies */
    S_MOTH1, S_MOTH2, S_TOAD1, S_TOAD2, S_GRUB1, S_GRUB2, S_SPIT1, S_SPIT2,
    S_SWOOP_HANG, S_SWOOP1, S_SWOOP2, S_SACK, S_SACK_HOP, S_BEETLE1, S_BEETLE2,
    S_WISP1, S_WISP2, S_GLOOM1, S_GLOOM2, S_CANARY1, S_CANARY2,
    /* boss */
    S_BOSS_SHUT, S_BOSS_OPEN, S_BOSS_HURT,
    /* projectiles & fx */
    S_STONE, S_SHARD, S_DRIP_FALL, S_SPARK, S_BOLT, S_COIN, S_PUFF1, S_PUFF2, S_PUFF3, S_AXE,
    S_LANTERN_HUD, S_LANTERN_HUD_OFF,
    S_DWELLER1, S_DWELLER2, /* a glow-worm who gives a hint */
    S_COUNT
};

extern Sprite ud_spr[S_COUNT];
void ud_art_load(void);
void ud_audio_load(void);

extern int UD_MUS_MINE, UD_MUS_DEEP, UD_MUS_BOSS, UD_MUS_WIN, UD_MUS_TITLE, UD_MUS_OVER;

#endif
