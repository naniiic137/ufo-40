/* CHIME CIRCUIT - the chime-ship flight model, on its own.
 *
 * Side view: gravity always pulls the ship down, holding thrust pushes it
 * up, left and right steer with inertia (the ship keeps drifting when you
 * let go), and there is no fuel. Thrust held with a direction also helps
 * the ship pick up speed that way, and a ship in the middle of a slash
 * falls a little slower. Everything is in 1/256 px and frames (60 a
 * second), so a run plays the same on every platform.
 *
 * This file knows nothing about races: it only needs a "solid?" callback
 * for the scenery. The later chime-ship cartridges (17, 35 and 49) are meant
 * to fly on this same model. */
#ifndef CHIME_FLIGHT_H
#define CHIME_FLIGHT_H

#include <stdint.h>
#include <stdbool.h>

#define CHF_ONE 256 /* one pixel (or one pixel a frame) */

typedef struct ChmFlightTune {
    int32_t gravity;    /* added to vy every frame */
    int32_t thrust;     /* taken from vy every frame the thrust is held */
    int32_t accel_x;    /* steering, every frame left or right is held */
    int32_t accel_thrust_x; /* ... and while thrust is held too */
    int32_t drag_x;     /* slow-down every frame nothing steers */
    int32_t max_vx;     /* steering and gravity stop adding speed here ... */
    int32_t max_up;
    int32_t max_down;
    int32_t over_decay; /* ... and a faster ship (a boost, a knock) slows back by this a frame */
    int32_t top_speed;  /* nothing ever moves faster than this on either axis */
    int32_t bounce;     /* speed kept off a wall, in 1/256 */
    int32_t min_bounce; /* the least a wall pushes back */
    int32_t slash_drag; /* a slashing ship's downward speed is kept at this much a frame, in 1/256 */
    int half;           /* the hit box: a square this many pixels each way from the centre */
} ChmFlightTune;

/* the chime ship's own numbers */
extern const ChmFlightTune CHM_TUNE;

typedef struct ChmFlight {
    int32_t x, y;   /* centre */
    int32_t vx, vy;
    int8_t face;    /* -1 left, +1 right: the last way steered (a slash goes this way) */
} ChmFlight;

/* the controls for one frame; CHF_SLASHING: the ship is mid-slash (its
 * fall slows) */
enum { CHF_LEFT = 1, CHF_RIGHT = 2, CHF_THRUST = 4, CHF_SLASHING = 16 };
/* what a move ran into */
enum { CHF_HIT_X = 1, CHF_HIT_Y = 2 };

/* is pixel (px, py) solid scenery? */
typedef bool (*ChmSolidFn)(const void *ctx, int px, int py);

/* one frame of steering, thrust and gravity (no movement yet) */
void chm_flight_control(ChmFlight *f, const ChmFlightTune *t, unsigned ctl);
/* one frame of movement in small steps, stopping at scenery; a wall that is
 * hit bounces the ship back off it. Returns CHF_HIT_* bits. */
int chm_flight_move(ChmFlight *f, const ChmFlightTune *t, ChmSolidFn solid, const void *ctx);
/* the same movement without the bounce (for looking ahead): the ship stops
 * where it touches */
int chm_flight_probe(ChmFlight *f, const ChmFlightTune *t, ChmSolidFn solid, const void *ctx);
/* does a hit box of the given half size at (x, y) touch scenery? */
bool chm_flight_blocked(int half, ChmSolidFn solid, const void *ctx, int32_t x, int32_t y);

#endif
