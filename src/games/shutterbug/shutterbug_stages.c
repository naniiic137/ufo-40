/* SHUTTERBUG - the seven stages: the prologue, Teatime Planet, Comet Rain A,
 * Gloom Planet, Comet Rain B, Fossil Planet and the true boss's room. Every
 * layout and wave here is our own; only the order and the kinds of places
 * follow the original (a tutorial, three planets with caves and an open
 * wave stage between each, mid-bosses on the first two planets, a boss at
 * the end of each planet). See docs/games/24-shutterbug.md.
 *
 * Terrain is a profile: each key gives, from its column on, how many rows
 * of rock hang from the top and how many rise from the bottom (ramp = slide
 * evenly to the next key); rectangles then add or cut rock. Spawns come
 * when the view's right edge nears their x. */
#include "shutterbug.h"

#define K(c, ce, fl) {c, ce, fl, 0}
#define R(c, ce, fl) {c, ce, fl, 1}
/* the same, indoors: a cave behind */
#define KC(c, ce, fl) {c, ce, fl, 2}
#define RC(c, ce, fl) {c, ce, fl, 3}
/* x, y, kind, n, gap (frames, or px for things on the rock), flags, arg */
#define S(x, y, k, n, g, f, a) {x, y, k, n, g, f, a}

/* ---- 0: the prologue: Home Orbit ------------------------------------------- */
static const TerrKey P_KEYS[] = {
    K(0, 0, 2), R(40, 0, 2), K(48, 0, 4), R(60, 0, 4), K(64, 0, 2), R(84, 0, 2), K(88, 0, 9), K(92, 0, 2),
    R(150, 0, 2), K(156, 0, 3), K(260, 0, 3),
};
static const TerrRect P_RECTS[] = {
    {88, 0, 4, 5, TL_ROCK}, /* a pillar from the top, to bump into */
};
static const SpawnDef P_SPAWNS[] = {
    S(150, 80, K_SIGN, 1, 0, 0, 0),
    S(330, 70, K_PUFF, 4, 40, 0, 0),
    S(470, 110, K_PUFF, 4, 40, 0, 0),
    S(560, 80, K_SIGN, 1, 0, 0, 1),
    S(640, 60, K_PUFF, 3, 20, 0, 0),
    S(700, 110, K_PUFF, 3, 20, 0, 0),
    S(780, 80, K_SIGN, 1, 0, 0, 2),
    S(940, 80, K_SIGN, 1, 0, 0, 3),
    S(1080, 84, K_PUFF, 5, 10, 0, 0),
    S(1180, 80, K_SIGN, 1, 0, 0, 4),
    S(1280, 70, K_PUFF, 5, 10, F_CRYSTAL, 1),
    S(1380, 80, K_SIGN, 1, 0, 0, 5),
    S(1460, 100, K_MINT, 3, 34, F_GREEN, 0),
    S(1560, 80, K_SIGN, 1, 0, 0, 6),
    S(1640, 60, K_PUFF, 3, 30, F_CRYSTAL, 0),
    S(1700, 120, K_PUFF, 3, 30, F_CRYSTAL, 0),
    S(1800, 80, K_SIGN, 1, 0, 0, 7),
};
static const BandDef P_BANDS[] = {{0, 0, 0}};
static const ScrollDef P_SCROLL[] = {{0, 2, HOLD_NONE}};

/* ---- 1: Teatime Planet (two screens tall in its scrolling caves) ---------- */
static const TerrKey T_KEYS[] = {
    /* the lawn */
    K(0, 21, 3), R(30, 21, 3), K(40, 21, 6), R(48, 21, 6), K(56, 21, 3), R(80, 21, 3), K(95, 21, 5), R(105, 21, 7),
    K(112, 21, 4), R(130, 21, 3), K(150, 21, 4), R(160, 21, 6), K(170, 21, 4),
    /* the first cave */
    RC(175, 21, 4), KC(182, 25, 5), RC(200, 25, 5), KC(206, 27, 5), RC(214, 27, 5), KC(220, 25, 7), RC(232, 25, 7),
    KC(238, 24, 4), RC(260, 24, 4), KC(266, 26, 7), RC(280, 26, 7), RC(290, 24, 4),
    /* out on the lawn again: Madame Scone */
    K(300, 21, 3), R(370, 21, 3), K(380, 21, 5), R(390, 21, 4),
    /* the second cave, with its two tall rooms */
    RC(400, 21, 3), KC(406, 24, 4), RC(420, 24, 4), KC(425, 3, 3), RC(464, 3, 3), KC(476, 24, 4), RC(495, 24, 4),
    KC(500, 3, 3), RC(557, 3, 3),
    /* the Teapot's parlour */
    K(570, 24, 3), K(640, 24, 3),
};
static const TerrRect T_RECTS[] = {
    /* the first tall room's shelves */
    {432, 15, 12, 2, TL_ROCK}, {448, 27, 12, 2, TL_ROCK}, {458, 8, 3, 12, TL_ROCK},
    {440, 33, 3, 6, TL_BREAK},
    /* the second tall room */
    {506, 20, 14, 2, TL_ROCK}, {528, 10, 3, 17, TL_ROCK}, {542, 24, 10, 2, TL_ROCK}, {536, 33, 2, 6, TL_BREAK},
    /* the parlour's ceiling has holes in it, where the drips come from */
    {580, 21, 2, 3, TL_EMPTY}, {590, 21, 2, 3, TL_EMPTY}, {600, 21, 2, 3, TL_EMPTY}, {610, 21, 2, 3, TL_EMPTY},
};
static const SpawnDef T_SPAWNS[] = {
    S(360, 230, K_CUBE, 5, 14, 0, 0),
    S(420, 300, K_TOAST, 2, 60, 0, 0),
    S(560, 270, K_CUBE, 5, 14, F_CRYSTAL, 0),
    S(640, 250, K_MINT, 3, 30, F_GREEN, 0),
    S(700, 280, K_TURRET, 1, 0, 0, 0),
    S(820, 220, K_CUBE, 5, 14, F_CRYSTAL, 0),
    S(870, 290, K_SECRET, 1, 0, 0, 0),
    S(940, 300, K_TOAST, 2, 50, 0, 0),
    S(1000, 240, K_MINT, 4, 26, F_GREEN | F_CRYSTAL, 0),
    S(1100, 280, K_TURRET, 2, 60, 0, 0),
    S(1180, 210, K_CUBE, 5, 14, 0, 0),
    S(1260, 290, K_CUBE, 5, 14, F_CRYSTAL, 0),
    S(1450, 220, K_TURRET, 1, 0, F_CEIL, 0),
    S(1500, 270, K_WALLBOMB, 2, 80, 0, 0),
    S(1560, 250, K_MINT, 3, 30, F_GREEN, 0),
    S(1640, 220, K_WALLBOMB, 2, 70, F_CEIL, 0),
    S(1720, 250, K_MOVER, 1, 0, 0, 34),
    S(1780, 240, K_CUBE, 5, 14, F_CRYSTAL, 0),
    S(1850, 280, K_TOAST, 2, 70, 0, 0),
    S(1910, 225, K_TURRET, 2, 80, F_CEIL, 0),
    S(2010, 250, K_MOVER, 1, 0, 0, 30),
    S(2080, 240, K_MINT, 4, 26, F_GREEN | F_CRYSTAL, 0),
    S(2160, 280, K_WALLBOMB, 3, 50, 0, 0),
    S(2250, 250, K_CUBE, 5, 14, 0, 0),
    S(2470, 250, K_CUBE, 5, 14, F_RED, 0),
    S(2900, 250, K_SCONE, 1, 0, 0, 0),
    S(3000, 230, K_CUBE, 5, 14, F_CRYSTAL, 0),
    S(3060, 300, K_TOAST, 2, 60, 0, 0),
    S(3140, 260, K_MINT, 3, 30, F_GREEN, 0),
    S(3260, 215, K_TURRET, 1, 0, F_CEIL, 0),
    S(3330, 260, K_CUBE, 5, 14, F_CRYSTAL, 0),
    S(3460, 120, K_CUBE, 5, 14, 0, 0),
    S(3520, 290, K_TOAST, 1, 0, 0, 0),
    S(3570, 60, K_TURRET, 1, 0, F_CEIL, 0),
    S(3620, 200, K_MINT, 3, 30, F_GREEN | F_CRYSTAL, 0),
    S(3690, 290, K_WALLBOMB, 2, 40, 0, 0),
    S(3730, 140, K_CHOMPER, 1, 0, F_CEIL, 0),
    S(3760, 190, K_SECRET, 1, 0, 0, 0),
    S(3860, 250, K_CUBE, 5, 14, F_CRYSTAL, 0),
    S(3920, 215, K_CHOMPER, 2, 60, F_CEIL, 0),
    S(4060, 100, K_MINT, 3, 30, F_GREEN, 0),
    S(4110, 290, K_TOAST, 2, 60, 0, 0),
    S(4190, 40, K_TURRET, 1, 0, F_CEIL, 0),
    S(4260, 200, K_CUBE, 5, 14, F_CRYSTAL, 0),
    S(4340, 290, K_WALLBOMB, 2, 40, 0, 0),
    S(4410, 120, K_CHOMPER, 1, 0, F_CEIL, 0),
    S(4462, 294, K_LETTER, 1, 0, 0, 0),
    S(4900, 260, K_TEAPOT, 1, 0, 0, 0),
};
static const BandDef T_BANDS[] = {{0, 168, 168}, {3390, 0, 168}, {3800, 168, 168}, {3990, 0, 168}, {4540, 168, 168}};
static const ScrollDef T_SCROLL[] = {{0, 2, HOLD_NONE}, {2600, 2, HOLD_BIG}, {4600, 0, HOLD_BIG}};

/* ---- 2 and 4: Comet Rain, out in the open ------------------------------------- */
static const TerrKey SPACE_KEYS[] = {K(0, 0, 0)};
static const SpawnDef A_SPAWNS[] = {
    S(340, 50, K_SWIRL, 6, 12, 0, 190),
    S(480, 120, K_SWIRL, 6, 12, F_TOP, 170),
    S(600, 40, K_ERUPTER, 1, 0, 0, 0),
    S(640, 90, K_ROCKBIG, 1, 0, F_CRYSTAL, 1),
    S(700, 30, K_ERUPTER, 1, 0, F_TOP, 0),
    S(760, 120, K_ROCK, 4, 30, 0, 3),
    S(840, 84, K_MINT, 4, 22, F_GREEN | F_CRYSTAL, 0),
    S(920, 60, K_BURSTER, 1, 0, 0, 230),
    S(980, 70, K_SWIRL, 6, 12, F_CRYSTAL, 200),
    S(1060, 40, K_ERUPTER, 2, 40, 0, 0),
    S(1120, 120, K_ROCKBIG, 1, 0, 0, 3),
    S(1180, 40, K_ROCKBIG, 1, 0, F_CRYSTAL, 1),
    S(1260, 110, K_BURSTER, 1, 0, 0, 250),
    S(1290, 50, K_BURSTER, 1, 0, 0, 200),
    S(1360, 30, K_ERUPTER, 2, 40, F_TOP, 0),
    S(1420, 120, K_SWIRL, 6, 12, F_TOP, 180),
    S(1500, 84, K_MINT, 5, 20, F_GREEN, 1),
    S(1580, 60, K_ROCK, 6, 18, 0, 1),
    S(1640, 40, K_ERUPTER, 3, 30, 0, 0),
    S(1720, 84, K_BURSTER, 2, 60, F_CRYSTAL, 210),
    S(1800, 50, K_SWIRL, 6, 12, 0, 200),
    S(1880, 120, K_SWIRL, 6, 12, F_TOP, 200),
    S(1960, 90, K_ROCKBIG, 2, 70, F_CRYSTAL, 2),
    S(2060, 40, K_ERUPTER, 3, 25, F_TOP, 0),
    S(2140, 84, K_MINT, 5, 18, F_GREEN | F_CRYSTAL, 1),
    S(2240, 60, K_BURSTER, 1, 0, 0, 240),
    S(2280, 110, K_BURSTER, 1, 0, 0, 200),
    S(2360, 50, K_SWIRL, 8, 10, 0, 190),
    S(2460, 30, K_ERUPTER, 4, 22, 0, 0),
    S(2520, 120, K_ROCK, 6, 16, 0, 3),
};
static const SpawnDef B_SPAWNS[] = {
    S(340, 60, K_SWIRL, 6, 12, 0, 190),
    S(420, 120, K_HOPPER, 2, 40, F_TOP, 1),
    S(520, 84, K_DART, 3, 30, 0, 0),
    S(600, 50, K_MINT, 4, 22, F_GREEN | F_CRYSTAL, 1),
    S(680, 120, K_SWIRL, 6, 12, F_TOP, 170),
    S(740, 40, K_HOPPER, 3, 40, 0, 1),
    S(820, 40, K_DART, 2, 20, 0, 0),
    S(860, 130, K_DART, 2, 20, 0, 0),
    S(920, 90, K_ROCKBIG, 1, 0, F_CRYSTAL, 2),
    S(1000, 60, K_BURSTER, 2, 50, 0, 220),
    S(1080, 84, K_SWIRL, 8, 10, F_CRYSTAL, 200),
    S(1160, 100, K_DART, 4, 16, 0, 0),
    S(1240, 30, K_HOPPER, 3, 30, F_TOP, 1),
    S(1320, 84, K_MINT, 5, 18, F_GREEN, 1),
    S(1400, 50, K_DART, 3, 24, 0, 0),
    S(1440, 120, K_DART, 3, 24, 0, 0),
    S(1520, 60, K_ROCKBIG, 2, 60, F_CRYSTAL, 1),
    S(1620, 84, K_BURSTER, 3, 40, 0, 230),
    S(1700, 40, K_SWIRL, 6, 12, 0, 200),
    S(1760, 130, K_SWIRL, 6, 12, F_TOP, 200),
    S(1840, 80, K_HOPPER, 4, 30, 0, 1),
    S(1940, 70, K_DART, 5, 14, 0, 0),
    S(2020, 84, K_MINT, 5, 16, F_GREEN | F_CRYSTAL, 1),
    S(2120, 40, K_DART, 3, 20, 0, 0),
    S(2160, 130, K_DART, 3, 20, 0, 0),
    S(2260, 60, K_SWIRL, 8, 10, 0, 190),
    S(2360, 50, K_DART, 4, 26, F_LAST, 0),
    S(2380, 120, K_DART, 4, 26, F_LAST, 0),
};
static const BandDef SPACE_BANDS[] = {{0, 0, 0}};
static const ScrollDef SPACE_SCROLL[] = {{0, 2, HOLD_NONE}};

/* ---- 3: Gloom Planet ------------------------------------------------------- */
static const TerrKey G_KEYS[] = {
    /* the haunted line */
    K(0, 0, 3), K(187, 0, 3),
    /* the first cave */
    RC(188, 0, 3), KC(194, 4, 4), RC(215, 4, 4), KC(220, 6, 4), RC(235, 6, 4), KC(240, 3, 6), RC(260, 3, 6),
    KC(266, 5, 5), RC(300, 5, 5),
    /* Old Croak's hall: a chasm in the floor, the F in the ceiling */
    K(312, 2, 3), K(340, 2, 0), K(351, 2, 3), K(370, 2, 3),
    /* the second cave */
    RC(372, 2, 3), KC(378, 4, 4), RC(400, 4, 4), KC(405, 7, 4), RC(420, 7, 4), KC(425, 4, 7), RC(445, 4, 7),
    KC(450, 5, 5), RC(480, 5, 5),
    /* the sidings, then the Signalman's yard */
    K(490, 0, 3), K(640, 0, 3),
};
static const TerrRect G_RECTS[] = {
    {0, 17, 187, 1, TL_RAIL}, {490, 17, 150, 1, TL_RAIL}, {490, 5, 70, 1, TL_RAIL}, {490, 11, 70, 1, TL_RAIL},
    {566, 5, 74, 1, TL_RAIL}, {566, 10, 74, 1, TL_RAIL}, {566, 16, 74, 1, TL_RAIL},
    {346, 0, 3, 2, TL_EMPTY},  /* the notch the F hides in */
    {226, 12, 3, 4, TL_BREAK}, {430, 9, 3, 4, TL_BREAK},
};
static const SpawnDef G_SPAWNS[] = {
    S(360, 70, K_GHOST, 2, 40, 0, 0),
    S(440, 139, K_CART, 4, 18, F_CRYSTAL, 0),
    S(560, 84, K_MINT, 4, 24, F_GREEN, 0),
    S(640, 50, K_GHOST, 3, 30, F_CRYSTAL, 0),
    S(760, 139, K_CART, 5, 18, 0, 0),
    S(840, 40, K_TURRET, 1, 0, 0, 0),
    S(880, 60, K_SECRET, 1, 0, 0, 0),
    S(940, 90, K_GHOST, 3, 30, 0, 0),
    S(1040, 139, K_CART, 6, 18, F_CRYSTAL, 0),
    S(1100, 70, K_MINT, 4, 24, F_GREEN | F_CRYSTAL, 0),
    S(1200, 50, K_GHOST, 2, 40, 0, 0),
    S(1300, 139, K_CART, 4, 18, 0, 0),
    S(1380, 84, K_GHOST, 3, 30, F_CRYSTAL, 0),
    S(1560, 120, K_RAILBOMB, 1, 0, 0, 0),
    S(1620, 40, K_SILO, 1, 0, F_CEIL, 0),
    S(1700, 84, K_MINT, 3, 28, F_GREEN, 0),
    S(1780, 120, K_SILO, 2, 70, 0, 0),
    S(1860, 50, K_RAILBOMB, 1, 0, F_CEIL, 0),
    S(1940, 90, K_WALLBOMB, 2, 60, 0, 0),
    S(2000, 84, K_GHOST, 3, 30, F_CRYSTAL, 0),
    S(2080, 50, K_WALLBOMB, 2, 60, F_CEIL, 0),
    S(2160, 120, K_RAILBOMB, 1, 0, 0, 0),
    S(2240, 84, K_MINT, 4, 22, F_GREEN | F_CRYSTAL, 0),
    S(2330, 40, K_SILO, 1, 0, F_CEIL, 0),
    S(2784, 8, K_LETTER, 1, 0, 0, 1),
    S(2900, 100, K_CROAK, 1, 0, 0, 0),
    S(3040, 84, K_GHOST, 3, 30, F_CRYSTAL, 0),
    S(3120, 120, K_RAILBOMB, 1, 0, F_RED, 0),
    S(3200, 40, K_CHOMPER, 2, 70, F_CEIL, 0),
    S(3280, 84, K_MINT, 4, 22, F_GREEN, 0),
    S(3360, 120, K_SILO, 2, 80, 0, 0),
    S(3440, 60, K_SECRET, 1, 0, 0, 0),
    S(3480, 60, K_RAILBOMB, 1, 0, F_CEIL, 0),
    S(3560, 84, K_GHOST, 4, 30, F_CRYSTAL, 0),
    S(3660, 40, K_TURRET, 2, 60, F_CEIL, 0),
    S(3760, 120, K_WALLBOMB, 3, 40, 0, 0),
    S(3840, 84, K_MINT, 4, 22, F_GREEN | F_CRYSTAL, 0),
    S(4000, 139, K_CART, 6, 18, F_CRYSTAL, 0),
    S(4060, 43, K_CART, 5, 18, 0, 0),
    S(4140, 91, K_CART, 6, 18, F_CRYSTAL, 0),
    S(4220, 84, K_GHOST, 3, 30, 0, 0),
    S(4300, 139, K_CART, 7, 18, 0, 0),
    S(4360, 43, K_CART, 6, 18, F_CRYSTAL, 0),
    S(4420, 91, K_CART, 5, 18, 0, 0),
    S(4860, 84, K_SIGNAL, 1, 0, 0, 0),
};
static const BandDef G_BANDS[] = {{0, 0, 0}};
static const ScrollDef G_SCROLL[] = {{0, 2, HOLD_NONE}, {2600, 2, HOLD_BIG}, {4560, 0, HOLD_BIG}};

/* ---- 5: Fossil Planet ------------------------------------------------------ */
static const TerrKey F_KEYS[] = {
    /* the fern valley */
    K(0, 0, 3), R(20, 0, 3), K(30, 0, 5), R(45, 0, 5), K(52, 0, 3), K(86, 0, 3),
    /* the first laser maze */
    RC(88, 0, 3), KC(92, 4, 4), RC(140, 4, 4), KC(145, 6, 2), RC(160, 6, 2), KC(165, 3, 6), RC(195, 3, 6),
    /* the valley again */
    K(200, 0, 3), R(230, 0, 3), K(240, 0, 6), R(250, 0, 6), K(255, 0, 3), K(298, 0, 3),
    /* the second laser maze */
    RC(300, 0, 3), KC(305, 5, 5), RC(330, 5, 5), KC(334, 2, 8), RC(350, 2, 8), KC(354, 8, 2), RC(370, 8, 2), KC(374, 4, 4),
    RC(410, 4, 4),
    /* the hanging gallery */
    KC(416, 3, 3), RC(490, 3, 3),
    /* the generator hall, then the king's lair */
    KC(494, 2, 2), RC(540, 2, 2), K(546, 0, 2), K(640, 0, 2),
};
static const TerrRect F_RECTS[] = {
    {120, 4, 3, 4, TL_BREAK}, {182, 12, 3, 3, TL_BREAK}, {360, 10, 3, 4, TL_BREAK},
};
static const SpawnDef F_SPAWNS[] = {
    S(300, 120, K_NIP, 2, 40, F_GREEN, 0),
    S(338, 80, K_LETTER, 1, 0, 0, 2),
    S(380, 50, K_KITE, 2, 40, 0, 0),
    S(470, 120, K_NIP, 3, 30, F_GREEN | F_CRYSTAL, 0),
    S(560, 40, K_KITE, 3, 36, F_CRYSTAL, 0),
    S(640, 120, K_NIP, 2, 50, F_GREEN, 0),
    S(800, 84, K_LASER, 1, 0, 0, 90),
    S(860, 40, K_TURRET, 1, 0, F_CEIL, 0),
    S(920, 84, K_LASER, 1, 0, 0, 90 | (60 << 8)),
    S(980, 84, K_MINT, 3, 26, F_GREEN | F_CRYSTAL, 0),
    S(1040, 84, K_LASER, 1, 0, 0, 80),
    S(1100, 120, K_TURRET, 1, 0, 0, 0),
    S(1180, 84, K_LASER, 1, 0, 0, 70 | (40 << 8)),
    S(1240, 84, K_KITE, 2, 40, F_CRYSTAL, 0),
    S(1330, 84, K_LASER, 1, 0, 0, 90),
    S(1400, 40, K_TURRET, 2, 60, F_CEIL, 0),
    S(1480, 84, K_LASER, 1, 0, 0, 80 | (30 << 8)),
    S(1560, 84, K_MINT, 3, 26, F_GREEN, 0),
    S(1680, 120, K_TURRET, 1, 0, F_RED, 0),
    S(1760, 120, K_NIP, 3, 30, F_GREEN | F_CRYSTAL, 0),
    S(1840, 40, K_KITE, 3, 36, 0, 0),
    S(1900, 80, K_SECRET, 1, 0, 0, 0),
    S(1960, 120, K_NIP, 3, 30, F_GREEN, 0),
    S(2060, 50, K_KITE, 3, 30, F_CRYSTAL, 0),
    S(2160, 84, K_MINT, 4, 22, F_GREEN, 0),
    S(2260, 120, K_NIP, 3, 30, F_GREEN | F_CRYSTAL, 0),
    S(2440, 84, K_LASER, 1, 0, 0, 90),
    S(2520, 50, K_TURRET, 1, 0, F_CEIL, 0),
    S(2580, 84, K_LASER, 1, 0, 0, 80 | (50 << 8)),
    S(2700, 84, K_MINT, 3, 24, F_GREEN | F_CRYSTAL, 0),
    S(2760, 84, K_LASER, 1, 0, 0, 70),
    S(2840, 120, K_TURRET, 1, 0, 0, 0),
    S(2900, 84, K_LASER, 1, 0, 0, 90 | (45 << 8)),
    S(2990, 84, K_KITE, 2, 40, F_CRYSTAL, 0),
    S(3060, 84, K_LASER, 1, 0, 0, 80),
    S(3140, 40, K_TURRET, 2, 60, F_CEIL, 0),
    S(3220, 84, K_LASER, 1, 0, 0, 70 | (35 << 8)),
    S(3360, 30, K_CHOMPER, 3, 50, F_CEIL, 0),
    S(3380, 130, K_NIP, 4, 36, F_GREEN, 0),
    S(3520, 84, K_MINT, 3, 26, F_GREEN | F_CRYSTAL, 0),
    S(3600, 30, K_CHOMPER, 3, 50, F_CEIL, 0),
    S(3620, 130, K_NIP, 4, 36, F_GREEN, 0),
    S(3760, 30, K_CHOMPER, 3, 46, F_CEIL, 0),
    S(3770, 130, K_NIP, 4, 30, F_GREEN | F_CRYSTAL, 0),
    S(4080, 84, K_LASER, 1, 0, 0, 90),
    S(4160, 84, K_LASER, 1, 0, 0, 90 | (90 << 8)),
    S(4200, 20, K_GEN, 2, 70, F_CEIL | F_CRYSTAL, 0),
    S(4200, 150, K_GEN, 2, 70, F_CRYSTAL, 0),
    S(4700, 84, K_JAW, 1, 0, 0, 0),
};
static const BandDef F_BANDS[] = {{0, 0, 0}};
static const ScrollDef F_SCROLL[] = {{0, 2, HOLD_NONE}, {4000, 2, HOLD_GENS}, {4400, 0, HOLD_BIG}};

/* ---- 6: the true boss -------------------------------------------------------- */
static const SpawnDef K_SPAWNS[] = {S(300, 84, K_KALEI, 1, 0, 0, 0)};
static const ScrollDef K_SCROLL[] = {{0, 0, HOLD_BIG}};

#define N(a) (int)(sizeof(a) / sizeof((a)[0]))
const StageDef SHB_STAGE[SHB_STAGES] = {
    {"PROLOGUE", "HOME ORBIT", TH_SPACE, 21, 1800, P_KEYS, N(P_KEYS), P_RECTS, N(P_RECTS), P_SPAWNS, N(P_SPAWNS),
     P_BANDS, N(P_BANDS), P_SCROLL, N(P_SCROLL)},
    {"STAGE 1", "TEATIME PLANET", TH_TEA, 42, 4600, T_KEYS, N(T_KEYS), T_RECTS, N(T_RECTS), T_SPAWNS, N(T_SPAWNS),
     T_BANDS, N(T_BANDS), T_SCROLL, N(T_SCROLL)},
    {"STAGE 2", "COMET RAIN A", TH_SPACE, 21, 2700, SPACE_KEYS, N(SPACE_KEYS), NULL, 0, A_SPAWNS, N(A_SPAWNS),
     SPACE_BANDS, N(SPACE_BANDS), SPACE_SCROLL, N(SPACE_SCROLL)},
    {"STAGE 3", "GLOOM PLANET", TH_GLOOM, 21, 4560, G_KEYS, N(G_KEYS), G_RECTS, N(G_RECTS), G_SPAWNS, N(G_SPAWNS),
     G_BANDS, N(G_BANDS), G_SCROLL, N(G_SCROLL)},
    {"STAGE 4", "COMET RAIN B", TH_SPACE, 21, 2700, SPACE_KEYS, N(SPACE_KEYS), NULL, 0, B_SPAWNS, N(B_SPAWNS),
     SPACE_BANDS, N(SPACE_BANDS), SPACE_SCROLL, N(SPACE_SCROLL)},
    {"STAGE 5", "FOSSIL PLANET", TH_FOSSIL, 21, 4400, F_KEYS, N(F_KEYS), F_RECTS, N(F_RECTS), F_SPAWNS, N(F_SPAWNS),
     F_BANDS, N(F_BANDS), F_SCROLL, N(F_SCROLL)},
    {"???", "THE LENS", TH_LENS, 21, 0, SPACE_KEYS, N(SPACE_KEYS), NULL, 0, K_SPAWNS, N(K_SPAWNS),
     SPACE_BANDS, N(SPACE_BANDS), K_SCROLL, N(K_SCROLL)},
};

/* ------------------------------------------------------------------------ */

static int band_index(float x) {
    const StageDef *st = &SHB_STAGE[sb.stage];
    int b = 0;
    for (int i = 0; i < st->nbands; i++)
        if (st->bands[i].x <= x) b = i;
    return b;
}

float shb_band_lo(float x) { return SHB_STAGE[sb.stage].bands[band_index(x)].ylo; }
float shb_band_hi(float x) { return SHB_STAGE[sb.stage].bands[band_index(x)].yhi; }

void shb_build_terrain(int s) {
    const StageDef *st = &SHB_STAGE[s];
    shb_rows = st->rows;
    shb_cols = SHB_MAX_COLS;
    memset(shb_tile, 0, sizeof shb_tile);
    memset(shb_bhp, 0, sizeof shb_bhp);
    memset(shb_cave, 0, sizeof shb_cave);
    int k = 0;
    for (int c = 0; c < shb_cols; c++) {
        while (k + 1 < st->nkeys && st->keys[k + 1].col <= c) k++;
        const TerrKey *a = &st->keys[k];
        float ce = a->ceil, fl = a->floor;
        shb_cave[c] = (a->ramp & 2) != 0;
        if ((a->ramp & 1) && k + 1 < st->nkeys) {
            const TerrKey *b = &st->keys[k + 1];
            float f = (float)(c - a->col) / (float)imax(1, b->col - a->col);
            ce = a->ceil + (b->ceil - a->ceil) * f;
            fl = a->floor + (b->floor - a->floor) * f;
        }
        int ci = (int)lroundf(ce), fi = (int)lroundf(fl);
        for (int r = 0; r < shb_rows; r++)
            if (r < ci || r >= shb_rows - fi) shb_tile[c][r] = TL_ROCK;
    }
    for (int i = 0; i < st->nrects; i++) {
        const TerrRect *r = &st->rects[i];
        for (int c = r->col; c < r->col + r->w && c < shb_cols; c++)
            for (int y = r->row; y < r->row + r->h && y < shb_rows; y++) {
                if (r->tile == TL_RAIL && shb_tile[c][y] != TL_EMPTY) continue;
                shb_tile[c][y] = r->tile;
                if (r->tile == TL_BREAK) shb_bhp[c][y] = 6;
            }
    }
}
