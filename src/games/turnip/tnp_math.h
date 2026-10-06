/* TURNIP TRUCK's own sine, cosine and arctangent.
 *
 * The C library's sinf/cosf/atan2f differ in the last bit between Windows,
 * Linux, the web and the Vita, and a week of driving magnifies that until the
 * demo driver takes another road. These use only + - * / and floorf, which give
 * the same bits everywhere, so a seed plays the same week on every platform.
 * Accuracy is about 1e-6, far below anything that shows on screen. */
#ifndef TNP_MATH_H
#define TNP_MATH_H

#include <math.h>

#define TNP_PI 3.14159265358979f

/* sin on [-pi/2, pi/2]: odd Taylor series to x^11 */
static inline float tnp_sin_core(float x) {
    float x2 = x * x;
    return x * (1.0f + x2 * (-1.0f / 6.0f + x2 * (1.0f / 120.0f + x2 * (-1.0f / 5040.0f +
                x2 * (1.0f / 362880.0f + x2 * (-1.0f / 39916800.0f))))));
}

static inline float tnp_sin(float x) {
    /* to [-pi, pi], then fold to [-pi/2, pi/2] using sin(pi - x) = sin(x) */
    float k = floorf(x / (2.0f * TNP_PI) + 0.5f);
    x -= k * (2.0f * TNP_PI);
    if (x > TNP_PI * 0.5f) x = TNP_PI - x;
    else if (x < -TNP_PI * 0.5f) x = -TNP_PI - x;
    return tnp_sin_core(x);
}

static inline float tnp_cos(float x) { return tnp_sin(x + TNP_PI * 0.5f); }

/* atan on [0, 1]: odd minimax polynomial (error ~1e-6) */
static inline float tnp_atan01(float t) {
    float t2 = t * t;
    return t * (0.99997726f + t2 * (-0.33262347f + t2 * (0.19354346f + t2 * (-0.11643287f +
                t2 * (0.05265332f + t2 * (-0.01172120f))))));
}

static inline float tnp_atan2(float y, float x) {
    float ax = fabsf(x), ay = fabsf(y);
    if (ax == 0.0f && ay == 0.0f) return 0.0f;
    float a = ax >= ay ? tnp_atan01(ay / ax) : TNP_PI * 0.5f - tnp_atan01(ax / ay);
    if (x < 0.0f) a = TNP_PI - a;
    return y < 0.0f ? -a : a;
}

#endif
