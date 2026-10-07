/* HOOPLA - whole-number helpers, so every match plays the same everywhere
 * (no sinf, cosf or atan2f anywhere in play). */
#include "hoop.h"

/* cos and sin of 0, 5, 10 ... 355 degrees, times 256; y grows downwards,
 * so index 54 (270 degrees) points straight up */
const int16_t HOOP_COS[72] = {
    256, 255, 252, 247, 241, 232, 222, 210, 196, 181, 165, 147, 128, 108, 88, 66, 44, 22,
    0, -22, -44, -66, -88, -108, -128, -147, -165, -181, -196, -210, -222, -232, -241, -247, -252, -255,
    -256, -255, -252, -247, -241, -232, -222, -210, -196, -181, -165, -147, -128, -108, -88, -66, -44, -22,
    0, 22, 44, 66, 88, 108, 128, 147, 165, 181, 196, 210, 222, 232, 241, 247, 252, 255};
const int16_t HOOP_SIN[72] = {
    0, 22, 44, 66, 88, 108, 128, 147, 165, 181, 196, 210, 222, 232, 241, 247, 252, 255,
    256, 255, 252, 247, 241, 232, 222, 210, 196, 181, 165, 147, 128, 108, 88, 66, 44, 22,
    0, -22, -44, -66, -88, -108, -128, -147, -165, -181, -196, -210, -222, -232, -241, -247, -252, -255,
    -256, -255, -252, -247, -241, -232, -222, -210, -196, -181, -165, -147, -128, -108, -88, -66, -44, -22};

int hoop_isqrt(int64_t v) {
    if (v <= 0) return 0;
    int64_t r = 0, bit = (int64_t)1 << 62;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (int)r;
}

int hoop_tri(int t, int period, int range) {
    if (period < 2) return 0;
    int p = t % period, half = period / 2;
    if (p < 0) p += period;
    return p < half ? p * range / half : (period - p) * range / (period - half);
}

/* the table direction closest to (dx, dy) */
int hoop_angle_of(int dx, int dy) {
    int best = 0;
    int64_t bd = INT64_MIN;
    for (int i = 0; i < 72; i++) {
        int64_t d = (int64_t)dx * HOOP_COS[i] + (int64_t)dy * HOOP_SIN[i];
        if (d > bd) { bd = d; best = i; }
    }
    return best;
}
