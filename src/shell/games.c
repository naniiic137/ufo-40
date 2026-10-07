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
extern const GameDef GAME_SKIDKIDS;
extern const GameDef GAME_SOUNDINGS;
extern const GameDef GAME_DUSKLING;
extern const GameDef GAME_DUNE;
extern const GameDef GAME_DOTDASH;
extern const GameDef GAME_LOSTLINKS;
extern const GameDef GAME_MANDIBLES;
extern const GameDef GAME_FLINTHOLD;
extern const GameDef GAME_TILTSHOT;
extern const GameDef GAME_BUZZBOLT;
extern const GameDef GAME_HOMESPUN;
extern const GameDef GAME_RIMSHIRE;
extern const GameDef GAME_WOBBLE;
extern const GameDef GAME_CHIME;
extern const GameDef GAME_HATTRICK;
extern const GameDef GAME_DRIFTLINE;
extern const GameDef GAME_BELLHOP;
extern const GameDef GAME_TURNIP;
extern const GameDef GAME_SHUTTERBUG;
extern const GameDef GAME_TUSKWIND;
extern const GameDef GAME_FORLORN;
extern const GameDef GAME_DUKESUP;
extern const GameDef GAME_FULLPEAL;
extern const GameDef GAME_BRAVADO;
extern const GameDef GAME_CLARION;
extern const GameDef GAME_HOOPLA;
extern const GameDef GAME_RESTLESS;

const GameDef *const GAMES[GAME_SLOTS] = {
    [0] = &GAME_UNDERDELVE,  /* 01 Barbuta */
    [1] = &GAME_GRUBSHIFT,   /* 02 Bug Hunter */
    [2] = &GAME_ROOFCAT,     /* 03 Ninpek */
    [3] = &GAME_WETPAINT,    /* 04 Paint Chase */
    [4] = &GAME_PETALPARADE, /* 05 Magic Garden */
    [5] = &GAME_TINTROOP,    /* 06 Mortol */
    [6] = &GAME_SKYWELL,     /* 07 Velgress */
    [8] = &GAME_BANNERFALL,  /* 09 Attactics */
    [12] = &GAME_DUSKLING,   /* 13 Mooncat */
    [9] = &GAME_BOOMTOWN,    /* 10 Devilition */
    [10] = &GAME_HATTRICK,   /* 11 Kick Club */
    [13] = &GAME_CUTLASS,    /* 14 Bushido Ball */
    [14] = &GAME_FENNEC,     /* 15 Block Koala */
    [15] = &GAME_TINTAIL,    /* 16 Camouflage */
    [16] = &GAME_BELLHOP,    /* 17 Campanella */
    [17] = &GAME_LOSTLINKS,  /* 18 Golfaria */
    [18] = &GAME_CHIME,      /* 19 The Big Bell Race */
    [20] = &GAME_TUSKWIND,   /* 21 Waldorf's Journey */
    [22] = &GAME_TURNIP,     /* 23 Onion Delivery */
    [23] = &GAME_SHUTTERBUG, /* 24 Caramel Caramel */
    [24] = &GAME_OPENHOUSE,  /* 25 Party House */
    [25] = &GAME_SKIDKIDS,   /* 26 Hot Foot */
    [26] = &GAME_SOUNDINGS,  /* 27 Divers */
    [27] = &GAME_DUNE,       /* 28 Rail Heist */
    [44] = &GAME_DOTDASH,    /* 45 Mini & Max */
    [45] = &GAME_MANDIBLES,  /* 46 Combatants */
    [29] = &GAME_FLINTHOLD,  /* 30 Rock On! Island */
    [30] = &GAME_TILTSHOT,   /* 31 Pingolf */
    [31] = &GAME_FORLORN,    /* 32 Mortol II */
    [32] = &GAME_DUKESUP,    /* 33 Fist Hell */
    [33] = &GAME_BRAVADO,    /* 34 Overbold */
    [34] = &GAME_CLARION,    /* 35 Campanella 2 */
    [35] = &GAME_HOOPLA,     /* 36 Hyper Contender */
    [37] = &GAME_RESTLESS,   /* 38 Rakshasa */
    [38] = &GAME_BUZZBOLT,   /* 39 Star Waspir */
    [43] = &GAME_HOMESPUN,   /* 44 Pilot Quest */
    [40] = &GAME_RIMSHIRE,   /* 41 Lords of Diskonia */
    [46] = &GAME_WOBBLE,     /* 47 Quibble Race */
    [47] = &GAME_DRIFTLINE,  /* 48 Seaside Drive */
    [48] = &GAME_FULLPEAL,   /* 49 Campanella 3 */
};
