/* TILTSHOT - the eighteen holes of the Comet Classic: names, pars, looks,
 * and the things that move. The ground itself is drawn in
 * tools/tiltshot/holes.txt (tiltshot_maps.c). Pars follow the original's
 * card hole by hole (61 in all); every layout is our own. */
#include "tiltshot.h"

/* kind, centre x, centre y, half width, half height, frames a round, phase */
static const TshMoverDef MV2[] = {
    {MV_BLIMP, 250, 64, 70, 0, 300, 0},
    {MV_BLIMP, 450, 88, 60, 0, 240, 100},
};
static const TshMoverDef MV4[] = {
    {MV_SPARK, 250, 60, 28, 28, 180, 0},
    {MV_SPARK, 340, 40, 22, 22, 150, 60},
};
static const TshMoverDef MV9[] = {
    {MV_HOPPER, 180, 76, 0, 22, 90, 0},
    {MV_HOPPER, 224, 70, 0, 26, 120, 40},
};
static const TshMoverDef MV12[] = {
    {MV_FISH, 192, 124, 60, 50, 160, 0},
    {MV_FISH, 488, 124, 60, 56, 180, 70},
    {MV_FISH, 784, 124, 56, 50, 150, 20},
};
static const TshMoverDef MV16[] = {
    {MV_KITE, 500, 70, 20, 30, 200, 0},
};
static const TshMoverDef MV18[] = {
    {MV_FISH, 220, 124, 60, 48, 170, 0},
    {MV_FISH, 360, 124, 60, 52, 190, 80},
    {MV_KITE, 900, 40, 30, 16, 220, 0},
    {MV_FISH, 1330, 124, 70, 54, 160, 40},
    {MV_BLIMP, 1560, 60, 50, 0, 260, 0},
};

/* spring lines, tilted down to the right so they throw the ball on */
static const TshSpringDef SP10[] = {
    {140, 122, 220, 134},
    {320, 118, 400, 132},
};

#define N(a) a, (uint8_t)ARRAY_LEN(a)

const TshHoleDef TSH_HOLE[TSH_HOLES] = {
    {"OPENING TEE", 3, TH_MEADOW, NULL, 0, NULL, 0},
    {"STONE SKIP", 3, TH_BEACH, N(MV2), NULL, 0},
    {"BOUNCE HOUSE", 3, TH_FAIR, NULL, 0, NULL, 0},
    {"SKYLARK", 4, TH_DUSK, N(MV4), NULL, 0},
    {"THE CHUTE", 4, TH_MEADOW, NULL, 0, NULL, 0},
    {"THREE STOREYS", 4, TH_FAIR, NULL, 0, NULL, 0},
    {"DUNE STEPS", 3, TH_DUSK, NULL, 0, NULL, 0},
    {"FROST TUNNEL", 4, TH_ICE, NULL, 0, NULL, 0},
    {"TOSS-UP", 1, TH_FAIR, N(MV9), NULL, 0},
    {"DOUBLE SPRING", 3, TH_NIGHT, NULL, 0, N(SP10)},
    {"PEBBLE", 2, TH_MEADOW, NULL, 0, NULL, 0},
    {"ATOLL", 4, TH_BEACH, N(MV12), NULL, 0},
    {"BOULDER", 3, TH_DUSK, NULL, 0, NULL, 0},
    {"BUMPER ALLEY", 4, TH_FAIR, NULL, 0, NULL, 0},
    {"THE HIGH SHELF", 4, TH_NIGHT, NULL, 0, NULL, 0},
    {"NERVE", 3, TH_MEADOW, N(MV16), NULL, 0},
    {"THE PAGODA", 3, TH_TEMPLE, NULL, 0, NULL, 0},
    {"LAST ORBIT", 6, TH_NIGHT, N(MV18), NULL, 0},
};
