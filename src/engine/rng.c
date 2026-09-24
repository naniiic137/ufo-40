#include "rng.h"

Rng g_rng = {0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL};

void rng_seed(Rng *r, uint64_t seed) {
    r->state = 0;
    r->inc = (seed << 1u) | 1u;
    rng_next(r);
    r->state += 0x9E3779B97F4A7C15ULL ^ seed;
    rng_next(r);
}

uint32_t rng_next(Rng *r) {
    uint64_t old = r->state;
    r->state = old * 6364136223846793005ULL + r->inc;
    uint32_t xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
    uint32_t rot = (uint32_t)(old >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

int rng_range(Rng *r, int lo, int hi) {
    if (hi <= lo) return lo;
    uint32_t span = (uint32_t)(hi - lo + 1);
    return lo + (int)(rng_next(r) % span);
}

int rng_chance(Rng *r, int percent) { return (int)(rng_next(r) % 100u) < percent; }

float rng_float(Rng *r) { return (float)(rng_next(r) >> 8) / 16777216.0f; }
