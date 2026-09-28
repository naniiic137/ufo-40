/* WET PAINT - shared declarations. Cartridge 04 of UFO 40, a tribute to
 * Paint Chase (UFO 50 #4). See docs/games/04-wet-paint.md. */
#ifndef WETPAINT_H
#define WETPAINT_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define WP_W 26          /* a course is 26 x 13 tiles, one screen */
#define WP_H 13
#define WP_TILE 12
#define WP_OX 4          /* the field's top left on screen */
#define WP_OY 22
#define WP_COURSES 26    /* 25 courses and the final showdown */
#define WP_FINAL 25      /* index of the final showdown */
#define WP_UNIT 16       /* positions are in 1/16 pixel */
#define WP_TU (WP_TILE * WP_UNIT) /* one tile in position units */

/* A course as drawn in wetpaint_courses.c. Legend:
 *  #  wall                    .  floor
 *  P  your start              Q  the rival's (and player 2's) start
 *  R L U D  a garage facing right, left, up, down (a wall that sends out foes)
 *  > < ^ v  a boost arrow     1 2 3 4  sprinkler, tack shooter, helper, freeze pop
 *  o  bumper                  x  thorn hedge (breaks when you hit it)
 *  T t  swing barrier, shut / open at the start     !  the lever that swings them
 *  A B  bollards that rise and sink in turn (A up first)
 *  ( ) m w  conveyor belt running left, right, up, down */
typedef struct WpCourse {
    const char *name;
    int secs;            /* the course clock */
    int goal;            /* per cent of the floor that must be blue */
    int group;           /* foes per garage load */
    const char *deck;    /* foe kinds the garages send, in turn */
    const char *rows[WP_H];
} WpCourse;
extern const WpCourse WP_COURSE[WP_COURSES];

/* foe kinds (letters in a deck) */
enum {
    F_ROLLER,  /* r: the plain pink kart */
    F_DUSTER,  /* p: a toy biplane, flies over walls, sprays in 4 ways at the end */
    F_TANKER,  /* k: a paint tanker, twice as fast */
    F_GLOOP,   /* s: a little pink gloop */
    F_BIGGLOOP,/* M: the white mother gloop, fast, lays little ones */
    F_POPPER,  /* g: a slow paint bomb cart that bursts in the end */
    F_HEDGEHOG,/* t: a spiked plough, only hit from the side or behind */
    F_CONKER,  /* c: a spiky ball that fires its spines when you line up */
    F_JELLY,   /* b: can't be hit at boost speed */
    F_KINDS
};

enum { WP_RIGHT, WP_DOWN, WP_LEFT, WP_UP };
enum { PAINT_NONE, PAINT_BLUE, PAINT_PINK };

/* sprites (cars face right; the art file makes the other three turns) */
enum {
    WS_BO, WS_FOXY, WS_ROLLER, WS_TANKER, WS_GLOOP, WS_BIGGLOOP, WS_POPPER, WS_HEDGEHOG,
    WS_CONKER, WS_CONKER_BARE, WS_JELLY, WS_HELPER, WS_DUSTER1, WS_DUSTER2,
    WS_CAR_COUNT,
    WS_SPRINKLER = WS_CAR_COUNT, WS_TACK, WS_HELPER_ICON, WS_FREEZE,
    WS_BO_BIG, WS_FOXY_BIG, WS_MARSHAL_BIG, WS_HEAD, WS_MEDAL, WS_CRUMPLE,
    WS_COUNT
};
/* the car sprites in 4 directions: wp_car[sprite][dir] */
extern Sprite wp_car[WS_CAR_COUNT][4];
extern Sprite wp_spr[WS_COUNT];
void wp_art_load(void);
void wp_audio_load(void);
extern int WP_MUS_TITLE, WP_MUS_RACE1, WP_MUS_RACE2, WP_MUS_RACE3, WP_MUS_FINAL, WP_MUS_CUT, WP_MUS_END,
    WP_MUS_CLEAR, WP_MUS_MISS, WP_MUS_OVER;

/* ---- the course simulation (wetpaint_logic.c) ---------------------------- */

#define WP_MAX_FOES 24
#define WP_MAX_SHOTS 64
#define WP_MAX_DRONES 6

enum { TK_FLOOR, TK_WALL, TK_GARAGE, TK_BUMPER, TK_THORN, TK_SWING, TK_BOLLARD, TK_LEVER, TK_ARROW, TK_BELT };
enum { ITEM_NONE, ITEM_SPRINKLER, ITEM_TACK, ITEM_HELPER, ITEM_FREEZE };
enum { SH_PAINT, SH_TACK, SH_SPINE, SH_SPRAY };
enum { MODE_SOLO, MODE_VERSUS };

typedef struct WpTile {
    uint8_t kind, dir, item;
    uint8_t alt;   /* swing barrier: open at the start; bollard: the B set */
} WpTile;

/* Anything that drives along the lanes: tile, heading, and how far it has
 * gone from that tile's centre towards the next one. */
typedef struct WpMover {
    int8_t tx, ty, dir;
    int16_t prog;  /* 0 .. WP_TU-1 */
} WpMover;

typedef struct WpCar {
    uint8_t on, team, human;
    WpMover m;
    int8_t want;        /* a turn waiting for the next tile centre */
    uint8_t stopped;    /* halted against a wall */
    int16_t speed;      /* position units per frame */
    int16_t boost_t;
    uint8_t boost_lv;
    int16_t stun_t, inv_t;
    uint8_t spin;       /* spun out by a jelly: slides to the wall painting pink */
    uint8_t gun;        /* ITEM_SPRINKLER or ITEM_TACK */
    int16_t gun_t, gun_cd;
    int8_t last_tx, last_ty;
    uint8_t braking;
    uint32_t hold;      /* buttons held this frame */
} WpCar;

typedef struct WpFoe {
    uint8_t on, kind;
    WpMover m;
    int32_t fx, fy;     /* dusters fly freely: centre in position units */
    int8_t plan;        /* hedgehog: the turn its lights show */
    int16_t age, t, boost_t, turn_t;
    uint8_t armed, slow;
    int8_t garage;
    int8_t last_tx, last_ty;
} WpFoe;

typedef struct WpShot {
    uint8_t on, kind, team;
    int8_t dir;
    int8_t owner;       /* the foe that fired a spine */
    int8_t lx, ly;      /* the tile it is over */
    uint8_t hops;       /* tiles entered since it was fired */
    int32_t x, y;
} WpShot;

typedef struct WpDrone {
    uint8_t on, team;
    WpMover m;
    int16_t inv_t;
    int8_t last_tx, last_ty;
} WpDrone;

typedef struct WpGarage {
    int8_t x, y, dir;
    uint8_t phase, left;
    int16_t t;
    uint8_t alive;
} WpGarage;

typedef struct WpSim {
    int course, mode;
    WpTile tile[WP_H][WP_W];
    uint8_t paint[WP_H][WP_W];
    uint8_t thorn[WP_H][WP_W];      /* a thorn hedge still standing */
    int total;                       /* paintable tiles */
    uint8_t swing_open;              /* the lever's state */
    uint8_t bollard_phase;
    int16_t bollard_t;
    int32_t frames_left;             /* the course clock */
    int16_t freeze_t;                /* a freeze pop is running */
    uint8_t freeze_team;             /* the side that is frozen */
    WpCar car[2];                    /* 0: you (blue); 1: the rival or player 2 (pink) */
    WpFoe foe[WP_MAX_FOES];
    WpShot shot[WP_MAX_SHOTS];
    WpDrone drone[WP_MAX_DRONES];
    WpGarage garage[8];
    int n_garages;
    int deck_i;
    int group;
    const char *deck;
    uint8_t secret;                  /* parked in the corner of course 1 while the pink took over */
    uint8_t no_foes, no_clock;       /* test switches */
    uint8_t foes_still;              /* test switch: foes hold their place */
    int stuns;                       /* times your car was stunned */
    uint32_t frame;
    int kills;
    int shake;
    Rng rng;
    /* events for the drawing side (bursts, sounds): a small ring */
    struct { uint8_t kind; int16_t x, y; } ev[32];
    int n_ev;
} WpSim;

extern WpSim wp;

enum { EV_KILL, EV_POP, EV_STUN, EV_ITEM, EV_BOOST, EV_BUMP, EV_THORN, EV_SPRAY, EV_LEVER };

void wp_sim_start(int course, int mode, uint64_t seed);
void wp_sim_step(void);
bool wp_solid(int x, int y);
int wp_count(int team);          /* tiles of one colour */
int wp_percent(int team);
bool wp_car_boosted(const WpCar *c);
int wp_bot_buttons(int who);     /* the demo driver's buttons for car 0 or 1 */
int wp_foe_count(void);
int wp_foe_count_kind(int kind);
int wp_add_foe(int kind, int tx, int ty, int dir, int garage); /* index or -1 */
void wp_mover_pos(const WpMover *m, int32_t *x, int32_t *y); /* centre in units */
int wp_opposite(int d);
extern const int WP_DX[4], WP_DY[4];

#endif
