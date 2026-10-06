/* FULL PEAL - shared declarations. Cartridge 49 of UFO 40.
 * A tribute to Campanella 3 (UFO 50 #49); see docs/games/49-full-peal.md.
 *
 * Ansel flies the Tinkler into the screen, out to the bell-planet Knell,
 * with his sister Clary on the radio. A faux-3D rail shooter: the ship
 * moves freely on a flat 6 x 4 plane facing the screen, with no gravity
 * and no walls. B fires the forward gun down the lane the ship is in, at
 * whatever is still in the distance; A fires the side blaster along the
 * plane, away from the way the ship is moving (held, it keeps its
 * direction), at whatever has reached the plane. Five stages of four
 * fixed waves and a boss. Each wave is graded by the share of its foes
 * shot down (0-100); a 100 or a 0 in a stage opens a balloon round after
 * its boss, where 50 points buy a continue. Final score: the twenty grades
 * plus 100 for each continue left.
 *
 * A second controller holding A opens Clary's console: fifty micro-games
 * of four colours in the little monitor in the cockpit, playable whatever
 * the main game is doing, each with its own high score.
 *
 *   fpl.c         the screens, the run, records, goals and the test hooks
 *   fpl_play.c    the rules: the ship, both guns, foes, shots, grading
 *   fpl_waves.c   the twenty waves (formations) and the stage table
 *   fpl_boss.c    the five bosses
 *   fpl_bonus.c   the balloon round
 *   fpl_micro.c   Clary's console: the menu and micro-games 1-25
 *   fpl_micro2.c  micro-games 26-50
 *   fpl_bot.c     the demo pilot (tests)
 *   fpl_draw.c    the view, the cockpit HUD and the screens
 *   fpl_art.c     sprites        fpl_audio.c   music and sounds */
#ifndef FPL_H
#define FPL_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the view ---------------------------------------------------------- */
#define FPL_HUD_H 46          /* the cockpit strip across the top */
#define FPL_VX 160            /* the vanishing point */
#define FPL_VY 113
#define FPL_CELL_W 44         /* one lane at the ship's plane, in pixels */
#define FPL_CELL_H 28
#define FPL_ZK 3.5f           /* depth z (0 = the ship's plane, 1 = far off) scales by 1 / (1 + ZK z) */
#define FPL_COLS 6
#define FPL_ROWS 4
#define FPL_PX 3.0f           /* the plane: x -3..3, y -2..2 */
#define FPL_PY 2.0f

static inline float fpl_k(float z) { return 1.0f / (1.0f + FPL_ZK * (z < -0.15f ? -0.15f : z)); }
static inline float fpl_sx(float x, float z) { return FPL_VX + x * FPL_CELL_W * fpl_k(z); }
static inline float fpl_sy(float y, float z) { return FPL_VY + y * FPL_CELL_H * fpl_k(z); }
static inline int fpl_col(float x) { int c = (int)floorf(x + FPL_PX); return c < 0 ? 0 : c > 5 ? 5 : c; }
static inline int fpl_row(float y) { int r = (int)floorf(y + FPL_PY); return r < 0 ? 0 : r > 3 ? 3 : r; }
static inline float fpl_lane_x(int c) { return (float)c - 2.5f; }
static inline float fpl_lane_y(int r) { return (float)r - 1.5f; }

/* ---- the ship ------------------------------------------------------------ */
#define FPL_SHIP_SPEED 0.06f  /* plane units a frame */
#define FPL_SHIP_MX 2.75f     /* how far the ship can go from the middle */
#define FPL_SHIP_MY 1.75f
#define FPL_EDGE_WARN 0.45f   /* the orange edge marks show this close to an edge */
#define FPL_SHIP_R 0.26f      /* hit radius */
#define FPL_FWD_CD 7          /* frames between forward shots while B is held */
#define FPL_SIDE_CD 7         /* ... and side shots while A is held */
#define FPL_FWD_VZ 0.055f     /* a forward shot's speed into the screen */
#define FPL_SIDE_V 0.16f      /* a side shot's speed along the plane */
#define FPL_FWD_HIT 0.78f     /* a forward shot hits within this of its lane's middle (generous) */
#define FPL_SIDE_HIT 0.42f
#define FPL_PLANE_Z 0.035f    /* a foe nearer than this is on the plane: only the side blaster reaches it */
#define FPL_LIVES 3
#define FPL_CONTINUES 2       /* three credits in all: the third loss of every life is the end */
#define FPL_MAX_CONTINUES 9
#define FPL_DEAD_T 50         /* frames from a loss to the next ship */
#define FPL_INV_T 120         /* the new ship blinks, safe, this long */

/* ---- foes ------------------------------------------------------------------ */
enum {
    EK_CLAPPER,    /* comes in down its lane and holds on the plane; some cross the plane end to end */
    EK_TENOR,      /* the same, three hits */
    EK_TREBLE,     /* the same, much quicker */
    EK_DODGER,     /* slow, two hits, slips a lane sideways when hit */
    EK_BOURDON,    /* a great bell that drops in from a side onto a marked lane: can't be hurt */
    EK_CROSSHEAD,  /* keeps its distance and lobs a shell that bursts in a cross on the plane */
    EK_FORKER,     /* keeps its distance; its shell splits up and down on the plane */
    EK_PENDULUM,   /* swings top to bottom on a fixed path as it comes */
    EK_SPITE,      /* a pendulum that bursts in a cross when shot */
    EK_SALLY,      /* a rope-worm that homes a little and bursts in three on reaching the plane */
    EK_QUICKSALLY, /* the same, faster, wriggling */
    EK_LOOKOUT,    /* turns up at an end of the plane and fires along it, three and three, then goes */
    EK_MOTE,       /* appears on the plane and drifts straight at where you were */
    EK_NIBBLER,    /* appears on the plane and chases, two hits */
    EK_CALTROP,    /* comes down its lane and rams through the plane: can't be hurt */
    EK_BROODER,    /* keeps its distance and spits motes */
    /* the bosses' company: never counted in a grade */
    EK_WISP,       /* Queen Sordina's: crosses the plane, one hit */
    EK_FLARE,      /* the Inkwell's: drops down a column of the plane, one hit */
    EK_COUNT
};

typedef struct {
    const char *name;
    int hp;            /* 0 = can't be hurt */
    float vz;          /* depth a frame on the way in */
    uint8_t counted;   /* counts toward the wave's grade */
    uint8_t col;       /* main colour (radar, sparks) */
} FplFoeDef;
extern const FplFoeDef FPL_FOE[EK_COUNT];

enum { FS_IN, FS_HOLD, FS_OUT };  /* coming in, on the plane (or keeping its distance), leaving */
typedef struct {
    uint8_t alive, kind, state, counted, form;
    float x, y, z, vx, vy, vz;
    float tx, ty;      /* a target or anchor */
    int hp, t, sub, arg, flash, fire_t;
    int dir;
} FplFoe;

/* shots: player shots, and foes' shots in depth (shells) or on the plane */
enum { ES_PLANE, ES_SHELL };
enum { BURST_NONE, BURST_CROSS, BURST_FORK, BURST_TRI, BURST_DIAG };
typedef struct {
    uint8_t alive, kind, burst, big;
    float x, y, z, vx, vy, vz;
    float spin;
    int t, life;
} FplEShot;

typedef struct {
    uint8_t alive, side;
    float x, y, z, vx, vy;
    int lane_c, lane_r;
    int t;
} FplPShot;

typedef struct { float x, y, z, vx, vy, vz; int life, col; } FplPart;

#define FPL_MAX_FOES 48
#define FPL_MAX_ESHOTS 160
#define FPL_MAX_PSHOTS 32
#define FPL_MAX_PARTS 200

/* ---- waves --------------------------------------------------------------------- */
/* A formation: foes released at fixed times. col/row pick a lane; col -1
 * or 6 brings it in from the left or right end of the plane instead. */
typedef struct {
    int16_t t;
    uint8_t kind;
    int8_t col, row;
    int8_t arg;
} FplSpawn;
typedef struct {
    const char *name;
    const FplSpawn *s;
    int n;
} FplForm;
extern const FplForm FPL_FORMS[];
extern const int FPL_FORM_COUNT;

typedef struct {
    const uint8_t *forms;  /* formation numbers, in order */
    int n;
} FplWave;

#define FPL_STAGES 5
#define FPL_WAVES 4
typedef struct {
    char letter;
    const char *name;
    const char *radio;     /* Clary's message as the stage starts */
    FplWave wave[FPL_WAVES];
    int boss;
    uint8_t sky, sky2;     /* backdrop colours */
} FplStage;
extern const FplStage FPL_STAGE[FPL_STAGES];
int fpl_wave_total(int stage, int wave);  /* foes that count toward its grade */
int fpl_wave_foes(int stage, int wave);   /* every foe in it */
int fpl_wave_forms(int stage, int wave);

/* ---- bosses -------------------------------------------------------------------- */
enum { BOSS_GLOAMEYE, BOSS_KNUCKLEBELL, BOSS_INKWELL, BOSS_SHELLBACK, BOSS_SORDINA, BOSS_COUNT };
extern const char *const FPL_BOSS_NAME[BOSS_COUNT];
extern const int FPL_BOSS_HP[BOSS_COUNT];
#define FPL_BOSS_Z 0.2f
typedef struct {
    bool on, dead;
    int kind, t, phase, hp, maxhp, flash, dead_t, fire_t, sub;
    float x, y, cx, cy;
    float ang;
    bool angry;            /* past half health */
    /* Knucklebell's fists: x on the plane, their speed */
    float fx[2], fvx[2];
    int fist_t[2];
    float fy;
    /* Sordina */
    bool mouth;            /* open */
    bool shut_by_hit;      /* this opening ended with a hit (no caltrops) */
    int mouth_t, heal_t;
    bool healed, clary;    /* she has healed once; Clary has come */
    float clx, cly;        /* Clary's ship */
    int clary_t, clary_fire;
    int hits;              /* forward shots that landed on the weak point (tests) */
    int blocked;           /* ... and that the body stopped */
    int fired, last_fan;   /* volleys fired (crosses, fans), the last fan's size (tests) */
    int period;            /* frames between volleys, as last set (tests) */
} FplBoss;
#define FPL_BOSS_FALL_T 150

/* ---- the balloon round ------------------------------------------------------------ */
#define FPL_BONUS_T (30 * 60)
#define FPL_BONUS_NEED 50
#define FPL_BALLOONS 45
#define FPL_BALLOON_ORANGE0 4     /* orange balloons with no perfect wave */
#define FPL_BALLOON_ORANGE_PER 4  /* more for each 100 in the stage */
typedef struct {
    uint8_t alive, orange, popped;
    float x, y, z, vy;
    int t;
} FplBalloon;

/* ---- Clary's console (the micro-games) ----------------------------------------------- */
#define FPL_MICROS 50
#define FPL_MON_X 117         /* the monitor's screen, inside its frame */
#define FPL_MON_Y 4
#define FPL_MON_W 86
#define FPL_MON_H 38
#define FPL_HOLD_OPEN 120     /* frames P2 holds A to open it */
#define FPL_MICRO_MAX 99999u
enum { MC_OFF, MC_MENU, MC_PLAY, MC_OVER };
typedef struct {
    int state, sel, t, hold;
    int game;              /* 0..49 */
    uint32_t score;
    int over_t;
    bool fresh_best;
    /* the running micro-game's own state: a shared scratch area */
    Rng rng;
    int gt;                /* frames into the game */
    int ivar[64];
    float fvar[64];
    int8_t grid[24][12];
    int ex[40], ey[40], es[40], et[40];
    float fx[40], fy[40], fvx[40], fvy[40];
    int n;
} FplMicro;
extern FplMicro fpm;
typedef struct {
    const char *name;
    void (*start)(void);
    void (*update)(uint8_t held, uint8_t pressed);
    void (*draw)(int ox, int oy);
} FplMicroDef;
extern const FplMicroDef FPL_MICRO_A[25];  /* games 1-25 (fpl_micro.c) */
const FplMicroDef *fpl_micro_def(int game);
void fpl_micro_reset(void);
void fpl_micro_update(uint8_t held2, uint8_t pressed2, bool allowed);
void fpl_micro_draw(int x, int y);       /* the console's screen, top-left at x, y */
bool fpl_micro_showing(void);
void fpl_micro_start(int game);          /* tests */
uint32_t fpl_micro_total(void);
/* helpers the games share (fpl_micro.c) */
void fpm_over(void);
void fpm_add(uint32_t pts);
void fpm_px(int ox, int oy, int x, int y, int col);
void fpm_rect(int ox, int oy, int x, int y, int w, int h, int col);
void fpm_text(int ox, int oy, const char *s, int x, int y, int col);
int fpm_rand(int lo, int hi);
#define MC_K C_INK
#define MC_W C_WHITE
#define MC_Y C_YELLOW
#define MC_R C_RED
/* micro-game buttons: the low bits of held/pressed */
#define MB_UP BTN_UP
#define MB_DOWN BTN_DOWN
#define MB_LEFT BTN_LEFT
#define MB_RIGHT BTN_RIGHT
#define MB_A BTN_A
#define MB_B BTN_B
#define MB_FIRE (BTN_A | BTN_B)
#define MB_DIRS (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)
extern const FplMicroDef FPL_MICRO_B[25];  /* games 26-50 (fpl_micro2.c) */
/* the play field under the console's top line */
#define FPM_W 86
#define FPM_H 32
#define FPM_TOP 6
/* engines shared by more than one game (fpl_micro.c) */
void fpm_chase(int *x, int *y, int tx, int ty);          /* a grid step toward a target */
void fpm_boss_start(int kind);                            /* the three OGRE games */
void fpm_boss_update(uint8_t held, uint8_t pressed);
void fpm_boss_draw(int ox, int oy);
void fpm_mines_start(bool warp);                          /* TIPTOE and WARPTOE */
void fpm_mines_update(uint8_t held, uint8_t pressed);
void fpm_mines_draw(int ox, int oy);
void fpm_tilt_start(bool rows);                           /* SLOPE and LANES */
void fpm_tilt_update(uint8_t held, uint8_t pressed);
void fpm_tilt_draw(int ox, int oy);
void fpm_paddle_ball(float *x, float *y, float *vx, float *vy, float px, int pw, int py);

/* ---- the game's states ------------------------------------------------------------- */
enum {
    PS_TITLE, PS_CODE, PS_RADIO, PS_WAVE, PS_GRADE, PS_BOSS, PS_BOSS_DOWN,
    PS_BONUS_IN, PS_BONUS, PS_BONUS_END, PS_ENDING, PS_TALLY, PS_OVER, PS_RESTART
};
#define FPL_RADIO_T 240
#define FPL_GRADE_T 150
#define FPL_FORM_GAP 30

/* The cartridge save: records and the console's high scores (a run is not
 * saved, as in the original). */
typedef struct {
    uint32_t magic;
    uint16_t best;           /* best final score */
    uint16_t top[FPL_STAGES];/* best total of a stage's four grades, A-E */
    uint16_t runs, wins, cherries, bonuses;
    uint8_t furthest;        /* the furthest stage reached, 1..5 (6: past the last boss) */
    uint8_t perfects;        /* most 100s in one run */
    uint8_t pad[2];
    uint32_t micro[FPL_MICROS];
} FplSave;

typedef struct {
    int state, state_t, frame_t;
    int sel;                 /* title menu */
    bool hugs;               /* the HUGS-ONLY code: no guns, no records or goals */
    char code[9];
    int code_pos;
    const char *code_msg;
    int code_msg_t;
    /* the run */
    int stage, wave;         /* 0..4, 0..3 */
    int grade[FPL_STAGES][FPL_WAVES];   /* -1 = not flown yet */
    int lives, continues;
    int perfect_stage;       /* 100s in this stage */
    bool bonus_due;          /* a 100 or a 0 in this stage */
    int deaths, credits_lost, perfects;
    bool won;
    int final_score;
    /* the ship */
    bool alive;
    float x, y;
    int face_x, face_y;      /* the last way the ship moved (opposite is where the blaster fires) */
    int side_dx, side_dy;    /* where the blaster is firing (locked while A is held) */
    bool side_locked;
    int fwd_cd, side_cd, dead_t, inv;
    int fired_fwd, fired_side, last_side;
    uint8_t prev_in;
    /* the wave */
    int form_i, form_t, form_gap, spawn_i;
    int wave_total, wave_kills, wave_escaped, wave_spawned;
    int stage_t;
    int last_grade;
    bool owl, meta;          /* the owl for a 100; the message for exactly 50 on the first wave */
    /* the bonus round */
    FplBalloon bal[FPL_BALLOONS];
    int bonus_pts, bonus_t, bonus_spawned, bonus_orange;
    bool bonus_won;
    int bonuses_won;
    /* the world */
    FplFoe foe[FPL_MAX_FOES];
    FplEShot es[FPL_MAX_ESHOTS];
    FplPShot ps[FPL_MAX_PSHOTS];
    FplPart part[FPL_MAX_PARTS];
    FplBoss boss;
    float scroll;            /* how far we've flown (the backdrop) */
    int shake;
    /* radio: Clary's lines on the console */
    const char *radio;
    int radio_t;
    /* testing */
    bool god, still, sandbox; /* sandbox: no wave runs or ends (tests) */
    int cause;
} FplGame;
extern FplGame fpg;
extern FplSave fpsv;

/* fpl_play.c */
void fpl_new_run(bool hugs);
void fpl_start_stage(int s);
void fpl_start_wave(int w);
void fpl_play_update(uint8_t in);
bool fpl_wave_done(void);
int fpl_wave_grade(void);
void fpl_ship_update(uint8_t in);
void fpl_shots_update(void);
void fpl_foes_update(void);
void fpl_eshots_update(void);
void fpl_parts_update(void);
int fpl_spawn_foe(int kind, int col, int row, int arg);
void fpl_kill_foe(int i, bool shot);
void fpl_hurt_foe(int i, int dmg);
void fpl_lose_ship(int cause);
int fpl_add_eshot(int kind, float x, float y, float z, float vx, float vy, float vz, int burst);
void fpl_shell_at(float x, float y, float z, float tx, float ty, int frames, int burst);
void fpl_burst(float x, float y, float z, int col, int n);
void fpl_clear_world(void);
void fpl_sfx(const char *name, int gap);
bool fpl_ship_hit_circle(float x, float y, float r);
void fpl_radio(const char *s);
int fpl_count_foes(int kind);
enum { CAUSE_SHOT = 0, CAUSE_FOE = 100, CAUSE_FIST = 200, CAUSE_BOSS, CAUSE_TEST };

/* fpl_boss.c */
void fpl_boss_start(int kind);
void fpl_boss_update(void);
/* a forward shot reaching the boss's depth: true if the boss stopped it */
bool fpl_boss_shot(FplPShot *s);
bool fpl_boss_side_shot(FplPShot *s);
void fpl_boss_weak(float *x, float *y, float *r); /* the weak point now (r 0 = shut) */
bool fpl_boss_weak_open(void);
int fpl_boss_noptions(void);
void fpl_boss_option(int k, float *x, float *y);
#define FPL_FIST_HW 0.42f
#define FPL_FIST_HH 0.55f
void fpl_bonus_shot(FplPShot *s, float z0);
void fpl_save_micro(int game, uint32_t score);

/* fpl_bonus.c */
void fpl_bonus_start(void);
void fpl_bonus_update(uint8_t in);
bool fpl_bonus_over(void);

/* fpl_bot.c */
int fpl_bot_buttons(void);
extern int fpl_bot_dbg[4];

/* fpl_draw.c */
void fpl_draw(void);
void fpl_draw_label(int x, int y, int w, int h, int t);
void fpl_blit(const Sprite *s, float cx, float cy, float scale, int flags, const uint8_t *remap, int solid);

/* art and sound */
enum {
    SP_SHIP,
    SP_CLAPPER, SP_TENOR, SP_TREBLE, SP_DODGER, SP_BOURDON, SP_CROSSHEAD, SP_FORKER,
    SP_PENDULUM, SP_SALLY, SP_SALLY_SEG, SP_LOOKOUT, SP_MOTE, SP_NIBBLER, SP_NIBBLER2,
    SP_CALTROP, SP_BROODER, SP_WISP, SP_FLARE,
    SP_BALLOON, SP_OWL,
    SP_GLOAMEYE, SP_KNUCKLE, SP_FIST, SP_INKWELL, SP_SHELLBACK, SP_SORDINA,
    SP_ORB,
    SP_COUNT
};
extern Sprite fpl_spr[SP_COUNT];
void fpl_art_load(void);
void fpl_audio_load(void);
extern int FPL_MUS_TITLE, FPL_MUS_STAGE[FPL_STAGES], FPL_MUS_BOSS, FPL_MUS_QUEEN, FPL_MUS_BONUS, FPL_MUS_RADIO,
    FPL_MUS_GRADE, FPL_MUS_PERFECT, FPL_MUS_OVER, FPL_MUS_ENDING, FPL_MUS_TALLY;

#endif
