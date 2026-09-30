/* TILTSHOT - shared declarations. Cartridge 31 of UFO 40, a tribute to
 * Pingolf (UFO 50 #31). See docs/games/31-tiltshot.md.
 *
 * Side-on golf on pinball courses: aim, hold A to fill the meter, let go to
 * swing, and tap A once in the air to slam the ball down. Eighteen fixed
 * holes, eight golfers on the board, lowest total against par wins the
 * Comet Classic. */
#ifndef TILTSHOT_H
#define TILTSHOT_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the course ------------------------------------------------------------ */

#define TSH_T 8                       /* a tile is 8 x 8 pixels */
#define TSH_ROWS 20                   /* every hole is 20 tiles tall */
#define TSH_VIEW_H (TSH_ROWS * TSH_T) /* 160: the play field; the display sits under it */
#define TSH_MAXCOLS 256
#define TSH_HOLES 18
#define TSH_PAR_TOTAL 61

/* Tiles, as drawn in tools/tiltshot/holes.txt (made into tiltshot_maps.c by
 * mkmaps.sh):
 *   .  air           #  turf (grass on top)     X  stone        I  ice block
 *   s  sand          T  spring pad (bouncy)     R  the red block (a secret)
 *   /  45 degrees up to the right              \  45 degrees down to the right
 *   1 2  a gentle rise (low half, high half)   3 4  a gentle fall (high, low)
 *   q  ceiling, solid top-left                 p  ceiling, solid top-right
 *   ~  water         U  the cup                 S  the tee
 *   O  big bumper    o  small bumper            !  peg
 *   C  crate   K  cone   N  churn   Y  bucket   (junk: breaks, slows a little)
 * Anything off the bottom of the map is a pit. */
extern const char *const *const TSH_MAP[TSH_HOLES];

enum { TH_MEADOW, TH_DUSK, TH_NIGHT, TH_ICE, TH_BEACH, TH_TEMPLE, TH_FAIR, TH_COUNT };

/* moving hazards: orange, break at a touch and nearly stop the ball */
enum { MV_BLIMP, MV_HOPPER, MV_KITE, MV_FISH, MV_SPARK, MV_KINDS };
typedef struct TshMoverDef {
    uint8_t kind;
    int16_t cx, cy;   /* centre of its path, in pixels */
    int16_t ax, ay;   /* half widths of the path */
    uint16_t period;  /* frames for one round */
    uint16_t phase;   /* frames into the round at the hole's start */
} TshMoverDef;

/* a spring line at any angle (both sides bounce) */
typedef struct TshSpringDef {
    int16_t x0, y0, x1, y1;
} TshSpringDef;

typedef struct TshHoleDef {
    const char *name;
    uint8_t par, theme;
    const TshMoverDef *movers;
    uint8_t nmovers;
    const TshSpringDef *springs;
    uint8_t nsprings;
} TshHoleDef;
extern const TshHoleDef TSH_HOLE[TSH_HOLES];

/* surfaces */
enum { TM_TURF, TM_ICE, TM_SAND, TM_SPRING, TM_BUMPER, TM_PEG };

#define TSH_MAXSEG 1400
#define TSH_MAXBUCKET (TSH_MAXCOLS / 2 + 2)
#define TSH_MAXBLIST 12000
#define TSH_MAXCIRC 48
#define TSH_MAXTRASH 32
#define TSH_MAXMOVER 16

typedef struct TshSeg {
    float x0, y0, x1, y1;
    float nx, ny;         /* the outside of the surface */
    float minx, maxx, miny, maxy;
    uint8_t mat, two_sided;
} TshSeg;

enum { TC_BIG, TC_SMALL, TC_PEG };
typedef struct TshCirc {
    float x, y, r;
    uint8_t kind;
} TshCirc;

enum { TJ_CRATE, TJ_CONE, TJ_CHURN, TJ_BUCKET, TJ_KINDS };
typedef struct TshJunk {
    float x, y;
    uint8_t kind;
} TshJunk;

typedef struct TshCourse {
    int hole, cols, w;
    uint8_t theme, par;
    char tile[TSH_ROWS][TSH_MAXCOLS];
    TshSeg seg[TSH_MAXSEG];
    int nseg;
    uint16_t bstart[TSH_MAXBUCKET + 1]; /* 16-pixel columns of the course */
    uint16_t blist[TSH_MAXBLIST];
    int nbucket;
    TshCirc circ[TSH_MAXCIRC];
    int ncirc;
    TshJunk junk[TSH_MAXTRASH];
    int njunk;
    TshMoverDef mover[TSH_MAXMOVER];
    int nmover;
    float tee_x, tee_y;
    int cup_col, cup_row;
    float cup_x, cup_y; /* the middle of the cup's mouth */
    int red_col, red_row;
} TshCourse;

void tsh_course_build(TshCourse *c, int hole);
char tsh_tile(const TshCourse *c, int col, int row);
bool tsh_tile_solid(char ch);
/* the solid part of a tile, pixel by pixel (for drawing) */
bool tsh_tile_px(char ch, int px, int py);
/* where a mover is at clock t; false while it is out of sight (a fish under water) */
bool tsh_mover_pos(const TshMoverDef *m, int t, float *x, float *y);

/* ---- one golfer's play on a hole ------------------------------------------- */

#define TSH_R 3.0f          /* the ball's radius */
#define TSH_AIMS 37         /* aim 0 points flat right ... 36 flat left, 5 degrees a step */
#define TSH_AIM_START 9     /* 45 degrees up and to the right */
#define TSH_FILL 60         /* frames of A to fill the meter */
#define TSH_GRACE 24        /* frames at full before the warning */
#define TSH_WARN 48         /* frames of warning, then the golfer blows up */
#define TSH_BOOM_T 80       /* frames until the next try after a blow-up */
#define TSH_LOST_T 70       /* frames before a lost ball is put back */

enum { TP_AIM, TP_CHARGE, TP_FLIGHT, TP_LOST, TP_BOOM, TP_HOLED };
enum { TL_WATER = 1, TL_PIT };

/* buttons as the play sees them (held this frame) */
enum { TB_LEFT = 1, TB_RIGHT = 2, TB_A = 4, TB_LOOK = 8 };

/* events for the presentation, cleared by it */
enum {
    FXT_SWING = 1 << 0, FXT_SLAM = 1 << 1, FXT_BOUNCE = 1 << 2, FXT_BUMPER = 1 << 3, FXT_SPRING = 1 << 4,
    FXT_JUNK = 1 << 5, FXT_MOVER = 1 << 6, FXT_SPLASH = 1 << 7, FXT_SKIP = 1 << 8, FXT_PIT = 1 << 9,
    FXT_SAND = 1 << 10, FXT_CUP = 1 << 11, FXT_FIRE = 1 << 12, FXT_WARN = 1 << 13, FXT_BOOM = 1 << 14,
    FXT_SECRET = 1 << 15, FXT_REST = 1 << 16, FXT_AIM = 1 << 17, FXT_FULL = 1 << 18, FXT_BACK = 1 << 19
};

typedef struct TshPlay {
    float x, y, vx, vy;   /* the ball */
    float sx, sy;         /* where the last stroke was hit from */
    int phase, phase_t;
    int aim, meter, max_t;
    int strokes;
    int clock;            /* frames on this hole: the movers' time */
    int fly_t, air_t, rest_t, fire_t;
    bool slam_used, slammed, secret, holed_in;
    uint8_t ground_mat, lost_why, skips, prev;
    int rep_t;            /* frames LEFT or RIGHT has been held */
    uint32_t junk_gone, mover_gone;
    /* for the presentation */
    uint32_t fx;
    float fx_x, fx_y;
    int fx_circ;          /* the bumper last struck */
    float fx_power;       /* how hard the last bounce was */
    int max_fly_t;
} TshPlay;

void tsh_play_begin(TshPlay *p, const TshCourse *c);
void tsh_play_step(TshPlay *p, const TshCourse *c, uint8_t buttons);
float tsh_aim_dx(int aim);
float tsh_aim_dy(int aim);
float tsh_shot_speed(int meter);
/* the dotted guide's points (launch direction only) */
int tsh_distance(const TshPlay *p, const TshCourse *c); /* to the cup, in yards */

/* ---- the demo player, the solver and the stored routes (tiltshot_bot.c) ---- */

typedef struct TshShot {
    int8_t aim;   /* -1 ends a route */
    int8_t power; /* meter frames */
    int16_t slam; /* frames after the swing to slam, -1 none */
} TshShot;

#define TSH_ROUTE_MAX 12
extern const TshShot TSH_ROUTE[TSH_HOLES][TSH_ROUTE_MAX];
/* the buttons that play shot s from the state p */
uint8_t tsh_bot_buttons(const TshPlay *p, const TshShot *s);
/* play one shot to its end on a copy; returns the phase it ends in */
int tsh_try_shot(TshPlay *p, const TshCourse *c, const TshShot *s);
/* search for a way round the hole from `start` (NULL: the tee); returns the
 * number of strokes found, 0 if none */
int tsh_solve(const TshCourse *c, const TshPlay *start, TshShot *out, int max, int beam, bool log);
/* strokes on a hole by a player who knows the course but whose swing is a
 * little off every time (a yardstick for the designs) */
int tsh_steady_play(const TshCourse *c, uint64_t seed, int *lost_out);

/* ---- the tournament (tiltshot_tour.c) ---------------------------------------- */

#define TSH_FIELD 8
#define TSH_GOLFERS 7       /* five on the tee, two more by the code */
#define TSH_RIVALS 12       /* the CPU names the field is drawn from */
extern const char *const TSH_GOLFER_NAME[TSH_GOLFERS];
extern const char *const TSH_RIVAL_NAME[TSH_RIVALS];

typedef struct TshEntrant {
    int8_t human;          /* 0 = CPU, 1 or 2 = player */
    uint8_t golfer, coat;  /* humans: golfer and colourway; CPUs: rival index */
    uint8_t score[TSH_HOLES]; /* strokes, 0 = not played yet */
} TshEntrant;

typedef struct TshTour {
    TshEntrant e[TSH_FIELD];
    int humans;
    int hole;              /* 0..17 */
    Rng rng;
} TshTour;

void tsh_tour_new(TshTour *t, int humans, const int *golfer, const int *coat, uint64_t seed);
int tsh_tour_total(const TshTour *t, int i, int holes); /* strokes against par over the first holes */
int tsh_tour_strokes(const TshTour *t, int i, int holes);
/* the standings after `holes` holes: order[] gets entrant indices, place[] 1-based (ties share) */
void tsh_tour_rank(const TshTour *t, int holes, int *order, int *place);
int tsh_tour_place(const TshTour *t, int i, int holes);
/* CPU rounds: per-hole scores whose totals follow the original's spread */
void tsh_cpu_round(Rng *r, uint8_t out[][TSH_HOLES], int ncpu);
const char *tsh_score_word(int strokes, int par);

/* ---- art and sound ------------------------------------------------------------ */

enum {
    TS_BALL, TS_FLAG1, TS_FLAG2, TS_CRATE, TS_CONE, TS_CHURN, TS_BUCKET,
    TS_BLIMP1, TS_BLIMP2, TS_HOPPER1, TS_HOPPER2, TS_KITE1, TS_KITE2, TS_FISH1, TS_FISH2, TS_SPARK1, TS_SPARK2,
    TS_TROPHY, TS_COMET,
    TS_COUNT
};
extern Sprite tsh_spr[TS_COUNT];

/* golfers: a head per golfer, a shared body; two colourways each */
enum { GP_STAND, GP_BACK1, GP_BACK2, GP_BACK3, GP_FOLLOW, GP_CHEER, GP_POSES };
void tsh_art_load(void);
/* the course (tiltshot_draw.c) */
void tsh_terrain_build(const TshCourse *c);
void tsh_draw_sky(int theme, int cam, int t);
void tsh_draw_course(const TshCourse *c, const TshPlay *p, int cam, int t, const uint8_t *flash);
void tsh_draw_outline(const TshCourse *c, int x, int y, int scale);
void tsh_draw_mover(const TshMoverDef *m, float x, float y, int t);
void tsh_draw_golfer(int golfer, int coat, int pose, int x, int y, bool flip, bool red);
void tsh_draw_head(int golfer, int coat, int x, int y);
const char *tsh_coat_name(int golfer, int coat);

void tsh_audio_load(void);
extern int TSH_MUS_TITLE, TSH_MUS_FRONT, TSH_MUS_BACK, TSH_MUS_LAST, TSH_MUS_BOARD, TSH_MUS_CHAMP,
    TSH_MUS_CUP, TSH_MUS_ACE, TSH_MUS_OVER, TSH_MUS_RUNNERUP;

#endif
