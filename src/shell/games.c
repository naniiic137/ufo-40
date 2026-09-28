/* UFO 40 - the cartridge slots. A cartridge sits in the slot with the number
 * of the UFO 50 game it pays tribute to; the empty slots are still in the
 * saucer's cargo hold. There are fifty slots, one for every UFO 50 number:
 * the name stayed 40, the library didn't. */
#include "gamedef.h"

extern const GameDef GAME_UNDERDELVE;
extern const GameDef GAME_GRUBSHIFT;
extern const GameDef GAME_ROOFCAT;
extern const GameDef GAME_WETPAINT;
extern const GameDef GAME_PETALPARADE;
extern const GameDef GAME_TINTROOP;
extern const GameDef GAME_SKYWELL;
extern const GameDef GAME_BANNERFALL;
extern const GameDef GAME_CUTLASS;
extern const GameDef GAME_FENNEC;
extern const GameDef GAME_BOOMTOWN;
extern const GameDef GAME_TINTAIL;
extern const GameDef GAME_OPENHOUSE;
extern const GameDef GAME_DUNE;
extern const GameDef GAME_LOSTLINKS;
extern const GameDef GAME_MANDIBLES;
extern const GameDef GAME_FLINTHOLD;
extern const GameDef GAME_RIMSHIRE;
extern const GameDef GAME_WOBBLE;

const GameDef *const GAMES[GAME_SLOTS] = {
    [0] = &GAME_UNDERDELVE,  /* 01 Barbuta */
    [1] = &GAME_GRUBSHIFT,   /* 02 Bug Hunter */
    [2] = &GAME_ROOFCAT,     /* 03 Ninpek */
    [3] = &GAME_WETPAINT,    /* 04 Paint Chase */
    [4] = &GAME_PETALPARADE, /* 05 Magic Garden */
    [5] = &GAME_TINTROOP,    /* 06 Mortol */
    [6] = &GAME_SKYWELL,     /* 07 Velgress */
    [8] = &GAME_BANNERFALL,  /* 09 Attactics */
    [9] = &GAME_BOOMTOWN,    /* 10 Devilition */
    [13] = &GAME_CUTLASS,    /* 14 Bushido Ball */
    [14] = &GAME_FENNEC,     /* 15 Block Koala */
    [15] = &GAME_TINTAIL,    /* 16 Camouflage */
    [17] = &GAME_LOSTLINKS,  /* 18 Golfaria */
    [24] = &GAME_OPENHOUSE,  /* 25 Party House */
    [27] = &GAME_DUNE,       /* 28 Rail Heist */
    [45] = &GAME_MANDIBLES,  /* 46 Combatants */
    [29] = &GAME_FLINTHOLD,  /* 30 Rock On! Island */
    [40] = &GAME_RIMSHIRE,   /* 41 Lords of Diskonia */
    [46] = &GAME_WOBBLE,     /* 47 Quibble Race */
};
