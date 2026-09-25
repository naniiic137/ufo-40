/* UFO 40 - the cartridge slots. 3 of 40 are built so far. */
#include "gamedef.h"

extern const GameDef GAME_UNDERDELVE;
extern const GameDef GAME_GRUBSHIFT;
extern const GameDef GAME_ROOFCAT;
extern const GameDef GAME_BANNERFALL;
extern const GameDef GAME_CUTLASS;
extern const GameDef GAME_FENNEC;
extern const GameDef GAME_TINTAIL;

const GameDef *const GAMES[GAME_SLOTS] = {
    &GAME_UNDERDELVE,
    &GAME_GRUBSHIFT,
    &GAME_ROOFCAT,
    /* slots 4..40: still in the saucer's cargo hold */
    [8] = &GAME_BANNERFALL,
    [13] = &GAME_CUTLASS,
    [14] = &GAME_FENNEC,
    [15] = &GAME_TINTAIL,
};
