/* TILTSHOT - the eighteen holes of the Comet Classic: names, pars, looks,
 * and the things that move. The ground itself is drawn in
 * tools/tiltshot/holes.txt (tiltshot_maps.c). Pars follow the original's
 * card hole by hole (61 in all); every layout is our own. */
#include "tiltshot.h"

/* kind, centre x, centre y, half width, half height, frames a round, phase
 * (a lantern: pivot x, pivot y, swing, chain) */
static const TshMoverDef MV2[] = {
    {MV_BLIMP, 200, 76, 60, 0, 240, 0},
    {MV_BLIMP, 470, 100, 50, 0, 180, 90},
};
static const TshMoverDef MV3[] = {
    {MV_KITE, 180, 110, 18, 10, 150, 0},
    {MV_KITE, 260, 96, 16, 12, 180, 60},
};
static const TshMoverDef MV4[] = {
    {MV_SPARK, 272, 56, 30, 30, 200, 0},
    {MV_SPARK, 460, 40, 20, 20, 240, 100},
    {MV_SPARK, 680, 70, 24, 24, 160, 40},
};
static const TshMoverDef MV5[] = {
    {MV_SPARK, 600, 68, 24, 6, 120, 0},
    {MV_SPARK, 740, 68, 20, 6, 144, 60},
};
static const TshMoverDef MV6[] = {
    {MV_SPARK, 300, 60, 26, 26, 220, 0},
    {MV_SPARK, 640, 76, 30, 10, 180, 60},
    {MV_SPARK, 840, 40, 20, 20, 150, 30},
};
static const TshMoverDef MV7[] = {
    {MV_ROLLER, 760, 123, 40, 0, 200, 0},
};
static const TshMoverDef MV9[] = {
    {MV_HOPPER, 150, 15, 0, 18, 90, 0},
    {MV_HOPPER, 205, 12, 0, 18, 120, 30},
    {MV_HOPPER, 260, 22, 0, 18, 150, 70},
    {MV_HOPPER, 315, 50, 0, 18, 180, 20},
};
static const TshMoverDef MV10[] = {
    {MV_SWING, 268, 0, 44, 76, 160, 0},
};
static const TshMoverDef MV12[] = {
    {MV_FISH, 200, 124, 70, 56, 160, 0},
    {MV_FISH, 512, 124, 64, 60, 180, 70},
    {MV_FISH, 816, 124, 60, 52, 150, 20},
    {MV_FISH, 1136, 124, 16, 30, 120, 40},
};
static const TshMoverDef MV13[] = {
    {MV_ROLLER, 448, 115, 24, 0, 110, 0},
    {MV_ROLLER, 536, 115, 24, 0, 130, 50},
};
static const TshMoverDef MV15[] = {
    {MV_BLIMP, 680, 72, 40, 0, 200, 0},
};
static const TshMoverDef MV16[] = {
    {MV_SWING, 500, 0, 40, 70, 160, 0},
};
static const TshMoverDef MV18[] = {
    {MV_FISH, 200, 124, 60, 48, 170, 0},
    {MV_FISH, 300, 124, 50, 52, 190, 80},
    {MV_FISH, 1000, 124, 50, 54, 160, 40},
    {MV_SWING, 1116, 80, 14, 18, 120, 0},
    {MV_ROLLER, 1400, 59, 70, 0, 240, 0},
};

/* spring lines, tilted down to the right so they throw the ball on */
static const TshSpringDef SP3[] = {
    {120, 122, 200, 132},
    {232, 126, 344, 137},
};
static const TshSpringDef SP10[] = {
    {140, 122, 220, 135},
    {320, 118, 400, 132},
};

#define N(a) a, (uint8_t)ARRAY_LEN(a)

const TshHoleDef TSH_HOLE[TSH_HOLES] = {
    {"OPENING TEE", 3, TH_MEADOW, NULL, 0, NULL, 0},
    {"STONE SKIP", 3, TH_BEACH, N(MV2), NULL, 0},
    {"BOUNCE HOUSE", 3, TH_FAIR, N(MV3), N(SP3)},
    {"SKYLARK", 4, TH_DUSK, N(MV4), NULL, 0},
    {"THE CHUTE", 4, TH_MEADOW, N(MV5), NULL, 0},
    {"THREE STOREYS", 4, TH_FAIR, N(MV6), NULL, 0},
    {"DUNE STEPS", 3, TH_DUSK, N(MV7), NULL, 0},
    {"FROST TUNNEL", 4, TH_ICE, NULL, 0, NULL, 0},
    {"TOSS-UP", 1, TH_FAIR, N(MV9), NULL, 0},
    {"DOUBLE SPRING", 3, TH_NIGHT, N(MV10), N(SP10)},
    {"PEBBLE", 2, TH_MEADOW, NULL, 0, NULL, 0},
    {"ATOLL", 4, TH_BEACH, N(MV12), NULL, 0},
    {"BOULDER", 3, TH_DUSK, N(MV13), NULL, 0},
    {"BUMPER ALLEY", 4, TH_FAIR, NULL, 0, NULL, 0},
    {"THE HIGH SHELF", 4, TH_NIGHT, N(MV15), NULL, 0},
    {"NERVE", 3, TH_MEADOW, N(MV16), NULL, 0},
    {"THE PAGODA", 3, TH_TEMPLE, NULL, 0, NULL, 0},
    {"LAST ORBIT", 6, TH_NIGHT, N(MV18), NULL, 0},
};
