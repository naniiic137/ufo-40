/* UFO 40 - the cartridge slots. A cartridge sits in the slot with the number
 * of the UFO 50 game it pays tribute to; the empty slots are still in the
 * saucer's cargo hold. Slots 41-50 don't exist: UFO 40 stops at 40. */
#include "gamedef.h"

extern const GameDef GAME_UNDERDELVE;
extern const GameDef GAME_GRUBSHIFT;
extern const GameDef GAME_ROOFCAT;
extern const GameDef GAME_PETALPARADE;

const GameDef *const GAMES[GAME_SLOTS] = {
    [0] = &GAME_UNDERDELVE,  /* 01 Barbuta */
    [1] = &GAME_GRUBSHIFT,   /* 02 Bug Hunter */
    [2] = &GAME_ROOFCAT,     /* 03 Ninpek */
    [4] = &GAME_PETALPARADE, /* 05 Magic Garden */
};
