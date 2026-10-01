/* HAT TRICK - shared declarations. Cartridge 11 of UFO 40.
 * A tribute to Kick Club (UFO 50 #11); see docs/games/11-hat-trick.md.
 *
 * A one-screen action platformer where the only weapon is one ball. Run
 * and jump about a screen that wraps wherever it has no wall, pick the ball
 * up by walking into it, and kick it (tap B), drive it (hold B, let go) or
 * aim it with the d-pad into the creatures that took over the pitches. With
 * no ball, B slides along the ground or heads it in the air. A lit ball
 * keeps the chain going and each kill in a chain drops better food; the
 * food only appears once the body lands. Four worlds of nine screens and a
 * boss, 40 in all, a 20-count clock and the Timekeeper in extra time. */
#ifndef HATTRICK_H
#define HATTRICK_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the field ------------------------------------------------------------ */
#define HTK_TOP 12            /* the field's top on screen, under the HUD */
#define HTK_W 320
#define HTK_H 168
#define HTK_T 8               /* tile size */
#define HTK_COLS 40
#define HTK_ROWS 21
#define HTK_HALF 20           /* the screens are written as their left half */
#define HTK_WORLDS 4
#define HTK_PER_WORLD 10
#define HTK_LEVELS (HTK_WORLDS * HTK_PER_WORLD)

enum { T_EMPTY, T_SOLID, T_LEDGE, T_GOAL };

/* ---- timing and feel (60 frames a second) -------------------------------------- */
#define HTK_TICK 150          /* one count of the clock: 2.5 s */
#define HTK_COUNTS 20         /* the clock starts at 20 */
#define HTK_BONUS 100         /* points for each count left */
#define HTK_RUN 1.25f         /* run speed, px a frame */
#define HTK_GROUND_ACC 0.35f
#define HTK_AIR_ACC 0.03f     /* stiff: a jump's course is mostly set at take-off */
#define HTK_GRAV 0.2f
#define HTK_JUMP (-4.0f)      /* one fixed height, held or tapped: about 38 px */
#define HTK_MAXFALL 4.0f
#define HTK_PW 6              /* half width of a kid: 12 x 19, a big target */
#define HTK_PH 19
#define HTK_PH_LOW 10         /* crouched or sliding */
#define HTK_SLIDE_T 22
#define HTK_SLIDE_V 2.6f
#define HTK_HEAD_T 14
#define HTK_CHARGE_T 30       /* B held this long on the ground: a driven shot */
#define HTK_KICK_CD 16        /* a kicked ball can't be picked straight up again */
#define HTK_DEATH_T 80
#define HTK_INV_T 120
#define HTK_START_SPARE 2     /* three lives in all */

#define HTK_BALL_R 3
#define HTK_BALL_GRAV 0.16f
#define HTK_KILL_SPEED 1.0f   /* a ball slower than this is at rest: it kills nothing */
#define HTK_DARK_BOUNCES 3    /* a loose ball goes dark on its third bounce ... */
#define HTK_DARK_REST 60      /* ... or after a second on the ground */
#define HTK_DARK_HOLD 180     /* carried this long without a kick, it goes dark too */
#define HTK_POWER_T 90        /* a driven shot hits bosses three times as hard */

#define HTK_COLLECT_T 240     /* after the last kill: time to pick up the food */
#define HTK_EXIT_T 110        /* the balloons lift the kids to the next screen */
#define HTK_INTRO_T 90

/* ---- creatures -------------------------------------------------------------- */
enum {
    FK_SPIKER, FK_FRISBEE, FK_BEACHBALL,  /* world 1: Sandy Court */
    FK_PIN, FK_BOWLER, FK_SPINNER,        /* world 2: Lucky Lanes */
    FK_BUOY, FK_POLO, FK_DUCKY,           /* world 3: the Lido */
    FK_PROP, FK_GLOVE, FK_BULLDOG,        /* world 4: the Big Stadium */
    FK_KINGSPIKER, FK_KINGPIN, FK_LIFEGUARD, FK_TOWER, FK_CAPTAIN,
    FK_TIMEKEEPER,
    FK_COUNT
};
#define FK_REGULAR 12
extern const char *const HTK_FOE_NAME[FK_COUNT];

typedef struct {
    float x, y, vx, vy;       /* centre */
    float ax, ay, ang;        /* anchor, angle (orbits, routes) */
    float rx0, ry0, rx1, ry1; /* a route round a platform, or the arena */
    int16_t kind, dir, vdir, state, hp, maxhp, flash, hw, hh, hops;
    int t, fire_t, wait;
    uint8_t alive, ground, alt, away;
} HtkFoe;

enum { SH_SERVE, SH_BOWL, SH_SQUIRT, SH_RUGBY, SH_SPADE, SH_CAN, SH_RING, SH_COUNT };
typedef struct {
    float x, y, vx, vy;
    int16_t kind, t, bounces;
    uint8_t alive;
} HtkShot;

/* foods: four for a chain, four desserts */
enum { IT_POPCORN, IT_PRETZEL, IT_TACO, IT_DRUMSTICK, IT_LOLLY, IT_COOKIE, IT_DONUT, IT_PARFAIT, IT_COUNT };
extern const int HTK_ITEM_VALUE[IT_COUNT];
extern const char *const HTK_ITEM_NAME[IT_COUNT];

/* a body falling (it turns into its food when it lands), or a food */
typedef struct {
    float x, y, vx, vy;
    int16_t body;             /* the creature it was (-1: a dessert, or landed) */
    int16_t item, t, fall_t;
    uint8_t alive, falling;
} HtkItem;

typedef struct { float x, y, vx, vy; int16_t life, col; } HtkPart;
typedef struct { float x, y; int16_t t, value; } HtkPop;

typedef struct {
    float x, y, vx, vy;       /* x centre, y the feet */
    int16_t facing, spare, ch, slide_t, head_t, kick_t, charge, dead_t, inv, anim;
    int16_t start_x, start_y, start_face, next_ext, desserts, kills;
    int16_t hit_by;           /* what took the last life: a creature, or 100 + a shot */
    int16_t deaths;
    uint8_t on, alive, out, ground, crouch, charging, lifted;
    uint16_t prev;            /* last frame's buttons */
    uint32_t score;
} HtkPlayer;

typedef struct {
    float x, y, vx, vy;
    int16_t carrier, last, bounces, rest_t, hold_t, power_t, spin;
    int16_t nopick[2];
    uint8_t lit, ground;
} HtkBall;

/* ---- one screen being played (copied whole by the demo player's look-ahead) ---- */
#define HTK_MAX_FOES 40
#define HTK_MAX_SHOTS 48
#define HTK_MAX_ITEMS 48
#define HTK_MAX_PARTS 64
#define HTK_MAX_POPS 8

enum { MODE_1P, MODE_COOP, MODE_VS };
enum { LS_INTRO, LS_PLAY, LS_CLEAR, LS_EXIT, LS_GOAL };

typedef struct {
    uint8_t tile[HTK_ROWS][HTK_COLS];
    int level;                /* 0..39, or -1 for the versus pitch */
    int mode;
    int sub, sub_t, frame;
    int clock;                /* frames left on the clock */
    int overtime, extra_t;    /* extra_t: the EXTRA TIME banner */
    int combo;                /* kills in the chain while the ball stays lit */
    int bonus;                /* the time bonus just given */
    int dessert_tx, dessert_ty, dessert_found;
    int boss_kind;            /* the screen's boss (-1 none) */
    int vs_target, vs_score[2], vs_scorer;
    int clear_wait;
    int kills;
    Rng rng;
    HtkPlayer pl[2];
    HtkBall ball;
    HtkFoe foe[HTK_MAX_FOES];
    HtkShot shot[HTK_MAX_SHOTS];
    HtkItem item[HTK_MAX_ITEMS];
    HtkPart part[HTK_MAX_PARTS];
    HtkPop pop[HTK_MAX_POPS];
    uint8_t ev_dead, ev_exit, ev_over, ev_vswin; /* for the screens around the play */
    uint8_t god, still;   /* tests: no deaths; creatures hold still */
} HtkPlay;

extern HtkPlay htk;
extern bool htk_sim;          /* the demo player is looking ahead: no sound */

/* ---- the screens ------------------------------------------------------------ */
typedef struct {
    const char *name;
    const char *const *rows;  /* HTK_ROWS strings of HTK_HALF characters */
} HtkLevelDef;
extern const HtkLevelDef HTK_LEVEL[HTK_LEVELS];
extern const HtkLevelDef HTK_VS_PITCH;
extern const char *const HTK_WORLD_NAME[HTK_WORLDS];
/* the Morse in the Lido boss's lane ropes (our own message) */
extern const char HTK_MORSE_MSG[];

/* hattrick_play.c */
void htk_load_level(HtkPlay *g, int level);
void htk_step(HtkPlay *g, uint16_t pad0, uint16_t pad1);
int htk_tile(const HtkPlay *g, float x, float y);
float htk_wrapdx(float d);
float htk_wrapdy(float d);
float htk_wrapx(float x);
float htk_find_floor(const HtkPlay *g, float xl, float xr, float oldy, float newy, bool kid);
bool htk_box_solid(const HtkPlay *g, float x0, float y0, float x1, float y1, bool kid);
void htk_kill_player(HtkPlay *g, int i);
void htk_add_score(HtkPlay *g, int i, int pts);
int htk_foes_left(const HtkPlay *g);
int htk_spawn_foe(HtkPlay *g, int kind, float x, float y, int dir, int alt);
int htk_add_shot(HtkPlay *g, int kind, float x, float y, float vx, float vy);
void htk_burst(HtkPlay *g, float x, float y, int col, int n, float sp);
void htk_sfx(const char *name);
int htk_counts_left(const HtkPlay *g);
float htk_ball_speed(const HtkPlay *g);

/* hattrick_foes.c */
void htk_foe_setup(HtkPlay *g, HtkFoe *f);
void htk_foes_update(HtkPlay *g);
void htk_shots_update(HtkPlay *g);
void htk_boss_hit(HtkPlay *g, int i, int dmg);
void htk_spawn_body(HtkPlay *g, float x, float y, int body, int item, float vx);
int htk_nearest_player(const HtkPlay *g, float x, float y);

/* hattrick_bot.c: the demo player, for the tests */
uint16_t htk_bot_buttons(HtkPlay *g, int who);
void htk_bot_reset(void);
int htk_bot_nav(const HtkPlay *g, int who, int what);
void htk_reach_check(const HtkPlay *g, int *unreach, int *dead);
extern int htk_reach_log;
extern int htk_bot_width;     /* candidates a decision (fewer: a weaker player) */
/* fairness: of n tries by a modest player at screen L, how many lose no life */
int htk_fair_runs(int level, int n, int width, int *worst_first_t);
extern int htk_fair_killer[128]; /* what took the life in the tries that lost one */

/* hattrick_draw.c */
void htk_draw_play(const HtkPlay *g);
void htk_draw_label(int x, int y, int w, int h, int t);
void htk_draw_backdrop(int world, int t);
void htk_draw_kid(int ch, int x, int y, int pose, int facing, int t);
void htk_draw_foe_sprite(int kind, int x, int y, int flip, int t, bool dead);
void htk_draw_item(int item, int x, int y);
int htk_morse_layout(int *len, int *gap, int max);

/* art and sound */
enum {
    SP_KID_STAND, SP_KID_RUN1, SP_KID_RUN2, SP_KID_JUMP, SP_KID_KICK, SP_KID_SLIDE, SP_KID_HEAD,
    SP_KID_CROUCH, SP_KID_DOWN,
    SP_SPIKER, SP_FRISBEE, SP_BEACHBALL, SP_PIN, SP_BOWLER, SP_SPINNER, SP_BUOY, SP_POLO, SP_DUCKY,
    SP_PROP, SP_GLOVE, SP_BULLDOG, SP_TIMEKEEPER, SP_TOWER,
    SP_POPCORN, SP_PRETZEL, SP_TACO, SP_DRUMSTICK, SP_LOLLY, SP_COOKIE, SP_DONUT, SP_PARFAIT,
    SP_BALL, SP_BALL_DARK, SP_BALLOON,
    SP_COUNT
};
extern Sprite htk_spr[SP_COUNT];
enum { KID_TEDDY, KID_MAE };
extern const char *const HTK_KID_NAME[2];
void htk_kid_remap(int ch, uint8_t *map);
void htk_art_load(void);
void htk_audio_load(void);
extern int HTK_MUS_WHISTLE, HTK_MUS_MATCH, HTK_MUS_FINAL, HTK_MUS_TROPHY, HTK_MUS_CLEAR, HTK_MUS_OVER,
    HTK_MUS_SHOOTOUT, HTK_MUS_TITLE;

#endif
