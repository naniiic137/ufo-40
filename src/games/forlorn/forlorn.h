/* FORLORN HOPE - shared declarations. Cartridge 32 of UFO 40.
 * A tribute to Mortol II (UFO 50 #32); see docs/games/32-forlorn-hope.md.
 *
 * The folk of Holloway have ninety-nine volunteers and one fixed map: the
 * camp, the Old Yew, the undercroft, the caves, the deep, the tower and
 * Thornkeep, where four thorn hearts beat. Each volunteer picks a trade at
 * the troop's door, and whatever they do stays done: foes stay hurt or
 * dead, keys stay found, doors stay open, and the stones, pouches,
 * waystones and chutes they leave behind when they give themselves up
 * stay where they were left.
 *
 *   forlorn_world.c   the rules: the volunteer, the five trades, giving
 *                     yourself up, keys, doors, plates, drains, the map
 *   forlorn_foes.c    the seventeen kinds of foe and the thorn hearts
 *   forlorn_map.c     the map (written by tools/forlorn/make_map.py)
 *   forlorn_bot.c     the demo player: a route, volunteer by volunteer
 *   forlorn.c         the screens, the run, the records, the goals, the
 *                     code screen, two players and the test hooks
 *   forlorn_draw.c    drawing the map, the troop and the foes
 *   forlorn_art.c     pixel art        forlorn_audio.c   music and sounds */
#ifndef FORLORN_H
#define FORLORN_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define FRL_MW 160             /* the map: 160 x 80 tiles of 10 px */
#define FRL_MH 80
#define FRL_T 10
#define FRL_OY 12              /* the map's top on screen (the HUD is above) */
#define FRL_VH (SCREEN_H - FRL_OY)
#define FRL_ONE 256            /* positions and speeds are in 1/256 px */

/* numbers (frames at 60 a second, 1/256 px) */
#define FRL_LIVES 99           /* volunteers in reserve when a run begins */
#define FRL_CHERRY 50          /* win having lost fewer than this many */
#define FRL_UW 6               /* a volunteer's box */
#define FRL_UH 9
#define FRL_RUN 256
#define FRL_ACC_GROUND 32
#define FRL_DEC_GROUND 64
#define FRL_ACC_AIR 16
#define FRL_GRAV 56
#define FRL_JUMP 870
#define FRL_FALL_MAX 1024
#define FRL_COYOTE 2
#define FRL_CHARGE 45          /* hold B this long and the volunteer flashes */
#define FRL_ARMED 10           /* held this long, a death sets the trade's gift off */
#define FRL_PIPE_LEN 5         /* a chute runs this many cells down */
#define FRL_BLAST_R 24         /* the sapper's blast, px */
#define FRL_BLAST_DMG 15
#define FRL_DEAD_T 50
#define FRL_WARP_T 18

enum { FRC_MASON, FRC_HUNTER, FRC_RUNNER, FRC_TINKER, FRC_SAPPER, FRC_COUNT };
extern const char *const FRL_CLASS_NAME[FRC_COUNT];
extern const int FRL_AMMO[FRC_COUNT];   /* 0: needs none (mason) or has no attack (sapper) */
extern const int FRL_DAMAGE[FRC_COUNT];

/* tiles */
enum {
    FT_AIR, FT_ROCK, FT_BRICK, FT_WOOD, FT_LEAVES, FT_TRUNK, FT_SPIKES, FT_LOOSE, FT_SEAL,
    FT_STONE,                  /* a mason's stone */
    FT_DOOR, FT_DOOR_OPEN,
    FT_BLOCK1, FT_BLOCK2, FT_BLOCK3, FT_BLOCK4, /* raised while plate 1-4 is held */
    FT_COMB,                   /* a midge comb (solid) */
    FT_GULP,                   /* the gulper's body (solid) */
    FT_COUNT
};

/* foes */
enum {
    FK_NONE,
    FK_WALLEYE, FK_OOZLE, FK_MIDGE, FK_HORNET, FK_SHELLBACK, FK_HATCHET, FK_SQUAWKER,
    FK_TUSKER, FK_IDOL, FK_DRAKE, FK_BELL, FK_KNIGHT, FK_BLOATER, FK_BROODHEN,
    FK_HORNHEAD, FK_STINGBACK, FK_GULPER, FK_HEART,
    FK_COUNT
};
extern const char *const FRL_FOE_NAME[FK_COUNT];
extern const int FRL_FOE_HP[FK_COUNT]; /* 0: can't be hurt (the gulper) */

typedef struct FrlFoe {
    uint8_t kind, on, minion;  /* minion: came out of a drain, comb or bell */
    int8_t face;
    int16_t hp;
    int16_t w, h;              /* box, px */
    int32_t x, y, vx, vy;      /* box top-left, 1/256 px */
    int16_t hx, hy;            /* home, px */
    int16_t owner;             /* the spawner a minion came from (-1 none) */
    uint16_t t, t2;
    uint8_t state, flash, ground;
    uint8_t hit_no;            /* the sword swing that last struck it */
    uint8_t kids;              /* minions out (a bell) */
} FrlFoe;

enum { FS_PELLET, FS_AXE, FS_BUBBLE, FS_CRESCENT, FS_BULLET, FS_STAR, FS_WRENCH };
typedef struct FrlShot {
    uint8_t kind, on, mine;    /* mine: the volunteer's */
    int32_t x, y, vx, vy;      /* middle, 1/256 px */
    int16_t life;
    int16_t t;
} FrlShot;

typedef struct FrlDrain { int16_t tx, ty; uint8_t ceiling, kids; uint16_t t; uint8_t near; } FrlDrain;
typedef struct FrlComb { int16_t tx, ty; uint8_t kids; uint16_t t; } FrlComb;
typedef struct FrlPlate { int16_t tx, ty; uint8_t down, ever; } FrlPlate;
typedef struct FrlKey { int16_t tx, ty; uint8_t taken; } FrlKey;
typedef struct FrlPipe { int16_t tx, ty, len; } FrlPipe;
typedef struct FrlPouch { int16_t x, y; } FrlPouch;          /* px, its middle bottom */

enum { FUM_WALK, FUM_PIPE, FUM_WARP };
typedef struct FrlUnit {
    uint8_t cls, player, mode;
    int32_t x, y, vx, vy;      /* box top-left, 1/256 px */
    int8_t face;
    uint8_t ground, coyote, air_jump, jump_held;
    int16_t charge;            /* frames B has been held (0: not held) */
    uint8_t atk_t, atk_cd, swing_no;
    int16_t ammo;
    uint16_t mode_t;
    int32_t tx, ty;            /* where a chute or a waystone takes it, 1/256 px */
    uint32_t life_t;
} FrlUnit;

/* the run's phase */
enum { FWP_SELECT, FWP_PLAY, FWP_DEAD, FWP_WON, FWP_OVER };

/* what happened this frame, for sounds and sparks */
enum {
    FEV_JUMP = 1 << 0, FEV_SWING = 1 << 1, FEV_SHOOT = 1 << 2, FEV_HIT = 1 << 3, FEV_KILL = 1 << 4,
    FEV_DIE = 1 << 5, FEV_STONE = 1 << 6, FEV_POUCH = 1 << 7, FEV_WAY = 1 << 8, FEV_PIPE = 1 << 9,
    FEV_BLAST = 1 << 10, FEV_KEY = 1 << 11, FEV_DOOR = 1 << 12, FEV_PLATE = 1 << 13, FEV_PLATE_UP = 1 << 14,
    FEV_WARP = 1 << 15, FEV_CHUTE = 1 << 16, FEV_AMMO = 1 << 17, FEV_HEART = 1 << 18, FEV_EATEN = 1 << 19,
    FEV_READY = 1 << 20, FEV_FOE_SHOT = 1 << 21, FEV_TING = 1 << 22, FEV_WIN = 1 << 23, FEV_SPAWN = 1 << 24,
    FEV_BREAK = 1 << 25, FEV_DOUBLE = 1 << 26, FEV_EMPTY = 1 << 27
};
typedef struct FrlFx { int16_t x, y; uint8_t kind; } FrlFx;
enum { FFX_BLAST, FFX_DIE, FFX_HIT, FFX_KILL, FFX_DUST, FFX_KEY, FFX_STONE, FFX_WARP, FFX_BREAK, FFX_HEART };

/* how a volunteer was lost */
enum { FCAUSE_GAVE, FCAUSE_SPIKES, FCAUSE_FOE, FCAUSE_SHOT, FCAUSE_EATEN, FCAUSE_TEST };

#define FRL_FOES 160
#define FRL_SHOTS 96
#define FRL_DRAINS 16
#define FRL_COMBS 4
#define FRL_PLATES 4
#define FRL_KEYS 8
#define FRL_PIPES 48
#define FRL_POUCHES 48
#define FRL_FX 16

typedef struct FrlWorld {
    uint8_t tile[FRL_MH][FRL_MW];
    FrlFoe foe[FRL_FOES];
    int nfoe;                  /* placed foes come first; minions after */
    int nplaced;
    FrlShot shot[FRL_SHOTS];
    FrlDrain drain[FRL_DRAINS];
    int ndrain;
    FrlComb comb[FRL_COMBS];
    int ncomb;
    FrlPlate plate[FRL_PLATES];
    FrlKey key[FRL_KEYS];
    int nkey;
    FrlPipe pipe[FRL_PIPES];
    int npipe;
    FrlPouch pouch[FRL_POUCHES];
    int npouch;
    int stones;
    uint8_t way_on;
    int16_t way_x, way_y;      /* the waystone's middle, px */
    int16_t base_x, base_y;    /* the troop's door: its middle bottom, px */
    int16_t pad_x, pad_y;      /* the waystone pad at the base: middle bottom, px */
    FrlUnit u;
    uint8_t phase;
    uint16_t phase_t;
    int lives;                 /* in reserve (the counter over the door) */
    int lost;                  /* volunteers lost this run */
    int units;                 /* volunteers sent out this run */
    int keys;                  /* carried (any key fits any door) */
    int doors, switches;       /* doors unlocked, plates pressed this run */
    int hearts_left;
    int sel;                   /* the trade under the cursor at the door */
    uint8_t allowed;           /* trades on offer (a bit each; the code limits them) */
    uint8_t players;           /* 1 or 2: in co-op the volunteers alternate */
    uint8_t last_cause;
    uint8_t stone_ev;
    int16_t cam_x, cam_y;      /* px */
    uint32_t t;
    uint32_t ev;
    FrlFx fx[FRL_FX];
    int nfx;
    uint16_t shake;
    uint32_t ctl, ctl_prev;    /* this frame's buttons and the last frame's */
} FrlWorld;

extern FrlWorld frl_w;

/* ---- the map (forlorn_map.c) -------------------------------------------- */
extern const char *const FRL_MAP[FRL_MH];

/* ---- rules (forlorn_world.c) -------------------------------------------- */
void frl_world_new(FrlWorld *w, int players, uint8_t allowed);
void frl_world_step(FrlWorld *w, uint32_t pad1, uint32_t pad2);
bool frl_solid(const FrlWorld *w, int tx, int ty);
bool frl_box_solid(const FrlWorld *w, int px, int py, int bw, int bh);
int frl_tile(const FrlWorld *w, int tx, int ty);
void frl_add_fx(FrlWorld *w, int x, int y, int kind);
void frl_unit_die(FrlWorld *w, int cause);
void frl_choose(FrlWorld *w, int cls);     /* a volunteer of that trade steps out */
void frl_give_up(FrlWorld *w);             /* the gift now, as if B were let go flashing */
bool frl_plate_down(const FrlWorld *w, int i);
int frl_unit_cx(const FrlWorld *w);        /* the volunteer's middle, px */
int frl_unit_cy(const FrlWorld *w);
bool frl_unit_on_pad(const FrlWorld *w);
bool frl_unit_at_way(const FrlWorld *w);
bool frl_way_red(const FrlWorld *w);       /* a foe waits by the waystone */
int frl_pipe_at(const FrlWorld *w, int px, int py, int bw, int bh);
void frl_blast(FrlWorld *w, int cx, int cy);
void frl_hurt_foe(FrlWorld *w, int i, int dmg);
FrlShot *frl_shot_add(FrlWorld *w, int kind, int32_t x, int32_t y, int32_t vx, int32_t vy, int life, bool mine);
int frl_isqrt(int v);
void frl_aim(int32_t x, int32_t y, int32_t tx, int32_t ty, int32_t speed, int32_t *vx, int32_t *vy);

/* ---- foes (forlorn_foes.c) ------------------------------------------------- */
int frl_foe_add(FrlWorld *w, int kind, int px, int py);  /* box top-left, px */
void frl_foe_size(int kind, int *bw, int *bh);
void frl_foes_step(FrlWorld *w);
void frl_spawners_step(FrlWorld *w);
void frl_clear_minions(FrlWorld *w);
bool frl_foe_active(const FrlWorld *w, const FrlFoe *f);
bool frl_foe_deadly(const FrlFoe *f);

/* ---- the demo player (forlorn_bot.c) --------------------------------------- */
typedef struct FrlBot {
    int unit;                  /* which volunteer of the route */
    int step;                  /* the command in its plan */
    int t;                     /* frames in this command */
    int dir;                   /* held direction: -1, 0, 1 */
    bool hold_b;
    bool stuck;
    int stuck_unit, stuck_step;
    uint32_t prev;
} FrlBot;
void frl_bot_reset(FrlBot *b);
uint32_t frl_bot(const FrlWorld *w, FrlBot *b);   /* the buttons for this frame */
int frl_bot_plans(void);
void frl_bot_route(int r);           /* 0 the demo route, 1 the castle way */
extern int frl_bot_debug;

/* ---- art & audio ------------------------------------------------------- */
void frl_art_load(void);
bool frl_art_ok(void);
void frl_draw_world(const FrlWorld *w, int t);
void frl_draw_unit_at(int cls, int x, int y, int face, int frame, bool flash, int player);
void frl_draw_unit_full(int cls, int x, int y, int face, int frame, bool flash, int player, bool swing);
void frl_draw_foe(const FrlFoe *f, int x, int y, int t);
void frl_draw_class_card(int cls, int x, int y, bool sel, bool allowed, int t);
void frl_draw_heart(int x, int y, int t, int scale);
void frl_draw_keep(int x, int y, int t, bool fallen);

void frl_audio_load(void);
extern int FRL_MUS_TITLE, FRL_MUS_CLASS[FRC_COUNT], FRL_MUS_END, FRL_MUS_OVER, FRL_MUS_SELECT;

#endif
