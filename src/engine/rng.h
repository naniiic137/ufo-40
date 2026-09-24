/* UFO 40 - deterministic RNG (PCG32). */
#ifndef UFO_RNG_H
#define UFO_RNG_H

#include <stdint.h>

typedef struct Rng {
    uint64_t state;
    uint64_t inc;
} Rng;

void rng_seed(Rng *r, uint64_t seed);
uint32_t rng_next(Rng *r);
int rng_range(Rng *r, int lo, int hi); /* inclusive lo..hi */
int rng_chance(Rng *r, int percent);
float rng_float(Rng *r);                /* [0,1) */

extern Rng g_rng; /* shared engine RNG, seeded by the platform */

#endif
