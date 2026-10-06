/* TURNIP TRUCK - shared declarations. Cartridge 23 of UFO 40.
 * A tribute to Onion Delivery (UFO 50 #23); see docs/games/23-turnip-truck.md.
 *
 * Zib drives Granny Root's turnip truck round Blipton, a city that wraps in
 * every direction, under a fixed north-up camera with steering that turns
 * the truck to its own left and right. Each workday: five timed deliveries
 * to named places, then back to the depot before the clock runs out; extra
 * deliveries are overtime. Three hearts that come back at top speed, tries
 * shared by the whole week, and from Tuesday one chaos event a day. Seven
 * days to the weekend. */
#ifndef TNP_H
#define TNP_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the city ------------------------------------------------------------------ */
#define TNP_CELL 48            /* one map cell, px */
#define TNP_MAP 32             /* cells a side */
#define TNP_WORLD (TNP_CELL * TNP_MAP) /* 1536 px, wrapping both ways */
#define TNP_HUD_W 64           /* the dashboard on the left */
#define TNP_VIEW_W (SCREEN_W - TNP_HUD_W)
#define TNP_VIEW_H SCREEN_H

/* cell types */
enum {
    CT_ROAD, CT_BUILD, CT_HQ, CT_GRASS, CT_TREE, CT_BRINE, CT_ROCK, CT_FENCE, CT_LOT, CT_BRIDGE,
    CT_COUNT
};
/* things painted on or standing in a road cell */
enum { FT_NONE, FT_DROP, FT_PIPE, FT_CRATE, FT_CANS, FT_RAMP };
enum { DIR_E, DIR_S, DIR_W, DIR_N };

typedef struct {
    uint8_t type, feat, ramp_dir, district;
} TnpCell;

extern TnpCell tnp_map[TNP_MAP][TNP_MAP];
extern const char *const TNP_MAP_ROWS[TNP_MAP];

#define TNP_DESTS 33
typedef struct { const char *name; int8_t cx, cy; } TnpDest;
extern const TnpDest TNP_DEST[TNP_DESTS];
#define TNP_DISTRICTS 9
extern const char *const TNP_DISTRICT_NAME[TNP_DISTRICTS];
/* the depot: its door is the middle of the top edge of the street cell below it */
#define TNP_HQ_DOOR_X (14 * TNP_CELL)
#define TNP_HQ_DOOR_Y (10 * TNP_CELL)
#define TNP_LOT_X0 28          /* the fenced lot (the secret): cells x 28-31, y 1-4 */
#define TNP_LOT_Y0 1

#define TNP_MAX_CRATES 16
#define TNP_MAX_CANS 8
#define TNP_MAX_PIPES 16
#define TNP_MAX_DROPS 16
extern int tnp_n_crates, tnp_n_cans, tnp_n_pipes, tnp_n_drops;
extern int8_t tnp_crate_c[TNP_MAX_CRATES][2], tnp_cans_c[TNP_MAX_CANS][2], tnp_pipe_c[TNP_MAX_PIPES][2],
    tnp_drop_c[TNP_MAX_DROPS][2];

void tnp_city_load(void);
const TnpCell *tnp_cell(int cx, int cy);           /* wraps */
const TnpCell *tnp_cell_at(float x, float y);      /* world px, wraps */
bool tnp_solid_type(int type);
bool tnp_drivable(int cx, int cy);                 /* the truck can be there (brine counts as not) */
bool tnp_roadlike(int cx, int cy);                 /* traffic drives there */
float tnp_wrap(float v);                           /* into [0, WORLD) */
float tnp_wrapd(float d);                          /* into [-WORLD/2, WORLD/2) */
int tnp_wrapc(int c);
float tnp_dist(float ax, float ay, float bx, float by);
float tnp_cx(int c);                               /* a cell's centre in px */
int tnp_district_at(float x, float y);

/* ---- feel (60 frames a second; px and radians) -------------------------------- */
#define TNP_TOP 2.6f           /* top speed forward, px a frame */
#define TNP_ACC 0.055f
#define TNP_COAST 0.012f       /* speed lost a frame with no pedal */
#define TNP_BRAKE 0.12f
#define TNP_REV_TOP 0.9f       /* reverse: much slower */
#define TNP_REV_ACC 0.03f
#define TNP_TURN 0.055f        /* the wheel at full lock, at speed */
#define TNP_TURN_FULL 1.2f     /* below this speed the truck turns less (no turns from a stand) */
#define TNP_GAS_TURN 0.75f     /* turning with the gas down at speed: wider */
#define TNP_GRIP 0.86f         /* sideways speed kept a frame: slippery */
#define TNP_DRIFT_GRIP 0.985f
#define TNP_DRIFT_TURN 2.1f
#define TNP_DRIFT_MIN 1.4f     /* A+B and a turn above this speed: a powerslide */
#define TNP_DRIFT_LIMIT 45     /* held longer: a spin-out */
#define TNP_PIVOT 0.04f        /* A+B with a turn while (nearly) stopped: turn on the spot */
#define TNP_TAP 8              /* a turn let go within this many frames is a sidestep */
#define TNP_JINK 1.4f          /* the sidestep's sideways push */
#define TNP_SPIN_T 100         /* a spin-out: wind-up, then the spin attack, then it eases */
#define TNP_SPIN_ATTACK0 40
#define TNP_SPIN_ATTACK1 80
#define TNP_R 5.0f             /* the truck's body against walls */
#define TNP_CAR_R 6.0f         /* ... and against cars */
#define TNP_CRASH_MIN 0.6f     /* cars meeting slower than this only nudge */
#define TNP_BOUNCE 0.35f
#define TNP_HEARTS 3
#define TNP_INV_T 60
#define TNP_REGEN_SPEED (TNP_TOP * 0.92f)
#define TNP_REGEN_T 150        /* frames at top speed, unhurt, for a heart */
#define TNP_RAMP_MIN 1.6f
#define TNP_ROCK_DRAG 0.955f
#define TNP_ROCK_GAS 0.35f
#define TNP_DEST_R 16.0f       /* a delivery circle */
#define TNP_HQ_R 11.0f         /* the depot's half circle: smaller */

/* ---- the job ------------------------------------------------------------------- */
#define TNP_DAYS 7
#define TNP_QUOTA 5
#define TNP_START_TIME 45      /* seconds on the clock each morning */
#define TNP_ADD_EARLY 8        /* deliveries 1-4 */
#define TNP_ADD_FIFTH 30
#define TNP_ADD_CRATE 12
#define TNP_TRIES 3            /* spare tries for the whole week: the 4th failure ends it */
#define TNP_CHERRY 50

enum { EV_NONE, EV_BRINE, EV_BEET, EV_MUSH, EV_RADISH, EV_RAIN, EV_MOON, EV_COUNT };
extern const char *const TNP_EVENT_NAME[EV_COUNT];

/* ---- things on the road ---------------------------------------------------------- */
enum { CK_TRAFFIC, CK_GANG, CK_POLICE };
typedef struct {
    float x, y, speed, nudge, ang;
    int16_t dir, ncx, ncy, kind, col, wreck_t, fire_t, partner, bump_t;
    uint8_t alive;
} TnpCar;

enum { MK_BEET, MK_MUSH, MK_SAUCER_S, MK_SAUCER_M, MK_SAUCER_L };
typedef struct {
    float x, y, vx, vy;
    int16_t kind, state, t, fire_t, dead_t;
    uint8_t alive;
} TnpMob;

enum { SK_BULLET, SK_BOMB };
typedef struct {
    float x, y, vx, vy;
    int16_t kind, t;
    uint8_t alive;
} TnpShot;

typedef struct { float x, y; int16_t t, life; uint8_t alive; } TnpPuddle;
typedef struct { float x, y, vx, vy; int16_t life, col; } TnpPart;

#define TNP_MAX_CARS 20
#define TNP_MAX_MOBS 48
#define TNP_MAX_SHOTS 24
#define TNP_MAX_PUDDLES 48
#define TNP_MAX_PARTS 64

/* the truck */
enum { TS_DRIVE, TS_SINK, TS_WRECK, TS_BOOM };
typedef struct {
    float x, y, vx, vy, ang, spin;
    float tap_ang;
    int16_t state, state_t, hearts, inv, regen_t, drift_t, spin_t, air_t, air_max, tap_t, tap_dir;
    int16_t hit_wall_t;
    uint16_t prev;
    uint8_t flip_reverse, swarmed;
} TnpTruck;

/* one workday being played (copied whole by nothing: the bot reads it) */
enum { DP_PLAY, DP_DONE, DP_FAIL };
enum { FAIL_NONE, FAIL_TIME, FAIL_WRECK };
typedef struct {
    int day;                 /* 0..6 */
    int event;
    int phase, fail_reason;
    int frame;
    int time_f;              /* frames left on the clock */
    int delivered;           /* today */
    int dest, prev_dest, dest_t;
    int crates_broken, hits, falls;
    int hit_by[4];           /* hearts lost to cars, brine, canisters, the chaos */
    int last_add;            /* the seconds the last delivery or crate added */
    int add_t;               /* how long to show it */
    int practice, god;       /* the practice code; tests */
    int died_in_lot;         /* the try ended in the fenced lot */
    int swarm_t, beet_sound_t;
    int squashed;            /* mushmen run over */
    int spawned, spawn_bad;  /* traffic that appeared; how many of those ahead of you going your way */
    Rng rng;
    TnpTruck tr;
    TnpCar car[TNP_MAX_CARS];
    TnpMob mob[TNP_MAX_MOBS];
    TnpShot shot[TNP_MAX_SHOTS];
    TnpPuddle pud[TNP_MAX_PUDDLES];
    TnpPart part[TNP_MAX_PARTS];
    uint8_t crate_ok[TNP_MAX_CRATES], cans_ok[TNP_MAX_CANS];
    uint8_t ev_delivered, ev_clock_out, ev_fail, ev_heal, ev_hurt; /* for sound and the screens */
    uint8_t no_traffic, no_events; /* tests */
} TnpDay;

extern TnpDay tnp;
extern bool tnp_quiet;       /* no sound (the fairness runs) */

/* tnp_drive.c */
void tnp_day_begin(TnpDay *d, int day, int event, uint64_t seed, int practice, int flip);
void tnp_step(TnpDay *d, uint16_t pad);
void tnp_pick_dest(TnpDay *d);
float tnp_speed(const TnpTruck *t);
float tnp_forward(const TnpTruck *t);
void tnp_hurt(TnpDay *d, int why);
void tnp_burst(TnpDay *d, float x, float y, int col, int n, float sp);
void tnp_sfx(const char *name);
bool tnp_in_hq_zone(float x, float y);
void tnp_dest_pos(int i, float *x, float *y);
bool tnp_spin_attack(const TnpTruck *t);
int tnp_seconds(const TnpDay *d);
void tnp_traffic_update(TnpDay *d);
int tnp_car_spawn(TnpDay *d, int kind, int cx, int cy, int dir);
void tnp_truck_respawn(TnpDay *d);
bool tnp_spawn_spot(TnpDay *d, int *cx, int *cy, int *dir, float dmin, float dmax);
void tnp_spinout(TnpTruck *t, int dir);

/* tnp_events.c: the six chaos days */
void tnp_events_begin(TnpDay *d);
void tnp_events_update(TnpDay *d);
/* a brine spray or wave pushing at x, y (0 = none) */
void tnp_brine_push(const TnpDay *d, float x, float y, float *fx, float *fy);
bool tnp_pipe_spraying(const TnpDay *d, int pipe, int dir);
int tnp_wave_band(const TnpDay *d, int lane, float *x0, float *x1);
#define TNP_WAVES 3
extern const int TNP_WAVE_ROW[TNP_WAVES];

/* tnp_bot.c: the demo player */
uint16_t tnp_bot_buttons(TnpDay *d);
void tnp_bot_reset(void);
int tnp_path_len(int sx, int sy, int tx, int ty);  /* cells, -1 if no way */

/* tnp_draw.c */
void tnp_draw_play(const TnpDay *d);
void tnp_draw_city(const TnpDay *d, float camx, float camy, int vx, int vy, int vw, int vh);
void tnp_draw_truck(float sx, float sy, float ang, int lift, int flash);
void tnp_draw_label(int x, int y, int w, int h, int t);
void tnp_draw_zib(int x, int y, int scale, int t, int mood);
void tnp_draw_anchor(int x, int y, int t, bool talking);
extern int tnp_hud_tries, tnp_hud_week, tnp_hud_best;

/* tnp_art.c */
enum {
    SP_TRUCK, SP_CAR, SP_GANG, SP_POLICE, SP_MUSH1, SP_MUSH2, SP_CRATE, SP_BALLOON, SP_CANS, SP_HEART, SP_HEART_OFF,
    SP_ZIB, SP_ANCHOR, SP_TURNIP,
    SP_COUNT
};
extern Sprite tnp_spr[SP_COUNT];
void tnp_art_load(void);
void tnp_draw_rot(const Sprite *s, float cx, float cy, float ang, const uint8_t *remap, int solid);

/* tnp_audio.c */
void tnp_audio_load(void);
extern int TNP_MUS_TITLE, TNP_MUS_NEWS, TNP_MUS_DAY, TNP_MUS_OVERTIME, TNP_MUS_RAIN, TNP_MUS_CLEAR, TNP_MUS_FAIL,
    TNP_MUS_END, TNP_MUS_FIRED;

#endif
