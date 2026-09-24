/* UFO 40 engine - umbrella header. Platform independent, no dependencies. */
#ifndef UFO_ENGINE_H
#define UFO_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "gfx.h"
#include "font.h"
#include "input.h"
#include "rng.h"
#include "audio.h"
#include "save.h"
#include "scene.h"
#include "platform.h"

#define TICK_HZ 60

void engine_init(void);
void engine_update(void); /* one fixed 1/60 s step */
void engine_draw(void);
uint32_t engine_frame(void);

/* small helpers */
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int iclamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline int isign(int v) { return (v > 0) - (v < 0); }
static inline int iabs(int v) { return v < 0 ? -v : v; }
static inline float fclamp(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline float fapproach(float v, float target, float step) {
    if (v < target) return v + step > target ? target : v + step;
    return v - step < target ? target : v - step;
}
static inline bool rects_overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

#define ARRAY_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

#endif
