/* SHUTTERBUG - shared declarations. Cartridge 24 of UFO 40.
 * A tribute to Caramel Caramel (UFO 50 #24); see docs/games/24-shutterbug.md.
 *
 * Poppy, a little red puffer-blimp, has a new camera and a holiday ticket.
 * A side-scrolling shooter with no power-ups: hold B for an automatic gun
 * that also charges four bouncing rings (let go to throw them), tap A to
 * take a photo a third of the screen ahead. A photo stuns what it catches:
 * stunned foes take double damage, are worth double, blow up their
 * neighbours and never strike back. Two hits a life, no lives to start
 * with, lives only from points, and a lost life starts the stage again.
 * A tutorial and five stages; photograph the hidden U, F and O for the
 * true last boss. 1 player, or 2 in co-op (Sprig joins). */
#ifndef SHUTTERBUG_H
#define SHUTTERBUG_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the screen and the world ---------------------------------------------- */
#define SHB_PF_Y 12           /* the playfield starts under the HUD strip */
#define SHB_PF_H 168          /* 21 rows of 8 */
#define SHB_TILE 8
#define SHB_VIEW_ROWS 21
#define SHB_MAX_COLS 720
#define SHB_MAX_ROWS 42

/* tiles */
enum { TL_EMPTY, TL_ROCK, TL_CAVE, TL_BREAK, TL_RAIL };
#define tile_solid(t) ((t) == TL_ROCK || (t) == TL_CAVE || (t) == TL_BREAK)

/* ---- the ship --------------------------------------------------------------- */
#define SHB_SPEED 1.3f        /* px a frame, eight ways; there are no speed-ups */
#define SHB_BOX_W 6           /* half sizes of the body that bumps into rock */
#define SHB_BOX_H 4
#define SHB_HIT_R 2.5f        /* what bullets and foes must touch */
#define SHB_FIRE_GAP 7        /* the gun: a shot every 7 frames while B is held */
#define SHB_SHOT_SPEED 6.0f
#define SHB_CHARGE_T 80       /* B held this long: the two dots light, rings ready */
#define SHB_RING_DMG 11       /* a close volley takes a big bite out of a boss */
#define SHB_RING_SPEED 3.2f
#define SHB_RING_LIFE 96
#define SHB_RING_BOUNCES 3
#define SHB_FLASH_MAX 1320    /* the camera's meter; it must be full to snap */
#define SHB_FLASH_RATE 1      /* refills by itself, slowly: in 22 seconds */
#define SHB_CRYSTAL 660       /* a bulb is half the meter: two fill it */
#define SHB_CURSOR_DX 107     /* the photo's centre: a third of the screen ahead */
#define SHB_PHOTO_W 64
#define SHB_PHOTO_H 52
#define SHB_STUN_T 300        /* a photographed foe stays stunned this long */
#define SHB_BOSS_STUN_T 180
#define SHB_BLAST_R 28        /* a stunned foe's death blast */
#define SHB_BLAST_DMG 6
#define SHB_INV_T 100         /* after the armour goes */
#define SHB_DEAD_T 150

#define SHB_EXTENDS 8
extern const uint32_t SHB_EXTEND_AT[SHB_EXTENDS];

enum { PL_POPPY, PL_SPRIG };

typedef struct {
    float x, y;               /* world, the centre */
    bool on;                  /* in this game (Sprig only in co-op) */
    bool alive;
    bool armour;              /* the first hit takes it; the second the ship */
    int inv, dead_t;
    int fire_cd, hold_t;      /* hold_t: frames B has been held */
    bool firing;              /* B held this frame */
    int flash;                /* the camera's meter */
    int snap_t;               /* the white frame of the last photo */
    float snap_x, snap_y;
    int ghost;                /* pushed into rock: passes through until free */
    int vx, vy;               /* the pad, for drawing */
} Ship;

/* ---- foes and everything else that lives in the world -------------------- */
enum {
    K_NONE,
    /* everywhere */
    K_PUFF,      /* the tutorial's drifting seed */
    K_MINT,      /* green: a parting shot when killed, unless photographed */
    K_TURRET,    /* on a wall or ceiling; falls when photographed */
    K_WALLBOMB,  /* clings to rock, then flies at you */
    K_CHOMPER,   /* hangs from the ceiling, lunges down; falls when photographed */
    /* Teatime Planet */
    K_CUBE,      /* sugar cubes in a wave; the red one is the repair foe */
    K_TOAST,     /* walks the floor or ceiling, pops crumbs that bounce */
    /* Comet Rain */
    K_SWIRL,     /* loops in a formation */
    K_ERUPTER,   /* bursts out of the top or bottom edge, after a warning */
    K_ROCKBIG,   /* a big comet: breaks into three */
    K_ROCK,
    K_BURSTER,   /* bursts into a ring of shots if left alive */
    K_DART,      /* a missile from the left, after a "!" */
    K_HOPPER,    /* bounces between the top and bottom edges */
    /* Gloom Planet */
    K_GHOST,     /* fades in and out; can't be hurt while faded */
    K_CART,      /* coach of a ghost train: one photo stuns the whole train */
    K_RAILBOMB,  /* tough cave crawler on rails; the red one is the repair foe */
    K_SILO,      /* launches homing rockets */
    K_ROCKET,
    /* Fossil Planet */
    K_NIP,       /* green lizard that leaps at you */
    K_KITE,      /* drops darts that stab upward when they land */
    K_LASER,     /* a laser gate: on, off, on; shooting it draws fire */
    K_GEN,       /* a laser generator: all must go before the king comes */
    /* props: not hurt by shots, harmless to touch */
    K_MOVER,     /* a moving block of rock: a photo freezes it */
    K_SECRET,    /* something odd in the scenery: a photo shakes out orbs */
    K_LETTER,    /* U, F or O, hidden in the scenery */
    K_ORB,       /* a score orb: shoot it */
    K_SIGN,      /* the tutorial's signposts */
    /* bosses and their parts */
    K_SCONE,     /* Teatime mid-boss: Madame Scone */
    K_TEAPOT,    /* Teatime boss: the Teapot */
    K_CROAK,     /* Gloom mid-boss: Old Croak (optional) */
    K_SIGNAL,    /* Gloom boss: the Signalman */
    K_JAW,       /* Fossil boss: King Thunderjaw */
    K_CHUTE,     /* the king's bone chutes, top and bottom */
    K_KALEI,     /* the true boss: the Kaleidoscope */
    K_SHARD,     /* one of its glass shards */
    K_COUNT
};

enum { ROLE_FOE, ROLE_MID, ROLE_BOSS, ROLE_PROP, ROLE_PART };
enum { F_RED = 1, F_CRYSTAL = 2, F_GREEN = 4, F_CEIL = 8, F_TOP = 16, F_LAST = 32 };

typedef struct {
    const char *name;
    int hp, points;
    int8_t hw, hh;
    uint8_t role;
    uint8_t mount;            /* 1: sits on rock, falls when photographed */
} FoeDef;
extern const FoeDef SHB_FOE[K_COUNT];

typedef struct {
    float x, y, vx, vy;
    uint8_t kind, alive, role, flags;
    int8_t hw, hh;
    int hp, maxhp, t, flash;
    int stun, photo;          /* stunned frames left; which photo caught it */
    int group, idx;           /* a train or formation, and the place in it */
    int state, st, phase, arg, arg2;
    float ax, ay;
    bool falling;             /* knocked off its rock by a photo */
    bool open;                /* a boss's weak point is showing */
    bool frozen;              /* a mover held by a photo */
    bool still;               /* tests: held in place, never moves or fires */
    int value;
} Foe;

/* enemy shots */
enum { ES_SHOT, ES_BOUNCE, ES_FALL, ES_ARROW, ES_CRAWL, ES_RETAL, ES_BIG, ES_CRUMB };
typedef struct {
    float x, y, vx, vy, r;
    uint8_t kind, alive;
    int t, bounces, hp, arg;
} EShot;

/* the player's shots */
enum { PS_GUN, PS_RING };
typedef struct {
    float x, y, vx, vy;
    uint8_t kind, alive, owner;
    int t, bounces, dmg;
} PShot;

enum { PK_CRYSTAL, PK_WRENCH };
typedef struct { float x, y, vx, vy; uint8_t kind, alive; int t; } Pickup;
typedef struct { float x, y, vx, vy; int life, col; } Part;
typedef struct { float x, y; int t, r; uint8_t alive; } Blast;

#define SHB_MAX_FOES 140
#define SHB_MAX_ESHOTS 160
#define SHB_MAX_PSHOTS 96
#define SHB_MAX_PICKUPS 24
#define SHB_MAX_PARTS 260
#define SHB_MAX_BLASTS 12

/* ---- stages ---------------------------------------------------------------- */
enum { TH_SPACE, TH_TEA, TH_GLOOM, TH_FOSSIL, TH_LENS };
enum { HOLD_NONE, HOLD_BIG, HOLD_GENS, HOLD_FOREVER /* tests only */ };

typedef struct { int16_t col; int8_t ceil, floor; uint8_t ramp; } TerrKey; /* ramp: 1 slide, 2 indoors */
typedef struct { int16_t col, row, w, h; uint8_t tile; } TerrRect;
typedef struct {
    int16_t x, y;             /* world px: it comes once the view's right edge nears x */
    uint8_t kind, n;
    int16_t gap;              /* frames between members, or px for placed ones */
    uint8_t flags;
    int16_t arg;
} SpawnDef;
typedef struct { int16_t x; int16_t ylo, yhi; } BandDef;  /* the view's top, world px */
typedef struct { int16_t x; uint8_t speed; uint8_t hold; } ScrollDef; /* speed in quarter px */

typedef struct {
    const char *name, *sub;
    int theme, rows;
    int end_x;                /* the view stops here; the stage ends when its boss falls */
    const TerrKey *keys; int nkeys;
    const TerrRect *rects; int nrects;
    const SpawnDef *spawns; int nspawns;
    const BandDef *bands; int nbands;
    const ScrollDef *scroll; int nscroll;
} StageDef;

#define SHB_STAGES 7          /* 0 the prologue, 1-5 the five, 6 the true boss */
extern const StageDef SHB_STAGE[SHB_STAGES];

/* ---- the save -------------------------------------------------------------- */
typedef struct {
    uint32_t magic;
    uint32_t best;            /* the best score: the highest it ever showed */
    uint16_t runs, wins, trues, pad;
    uint8_t most_types, most_secrets, best_stage, pad2;
    uint32_t kinds_ever;      /* bit per foe kind ever photographed (low 32) */
    uint32_t kinds_ever2;
} ShbSave;

/* ---- the game's states ----------------------------------------------------- */
enum {
    SS_TITLE, SS_STORY, SS_BANNER, SS_PLAY, SS_LOST, SS_OVER, SS_CLEAR, SS_ENDING, SS_CREDITS
};

typedef struct {
    int state, state_t, frame_t;
    int menu;                 /* the title's cursor: 1 PLAYER / 2 PLAYERS */
    int players;
    int stage;
    /* the run */
    uint32_t score;           /* the shown score: back to 0 when a life is lost */
    uint32_t best;            /* the highest it showed this run: the final score */
    uint32_t total;           /* every point earned this run: the extends count it */
    int extends, lives, deaths;
    uint8_t letters;          /* bits: 1 U, 2 F, 4 O; kept through lost lives */
    int orbs_shot;            /* score orbs shot this life: each pays more */
    uint32_t kinds;           /* foe kinds photographed this run */
    uint32_t kinds2;
    int secrets;              /* secret photos this run */
    bool won, true_won;
    bool beacon_given;
    bool zero_prologue;       /* the prologue ended with no points */
    /* the stage */
    float cam_x, cam_y;
    int scroll_i;
    int hold;                 /* the view waits for: HOLD_* */
    bool big_seen;
    int spawn_i;
    int stage_t;
    bool boss_dead;
    int clear_t;
    int tip, tip_t;           /* the tutorial's current tip */
    /* the ships */
    Ship ship[2];
    /* the world */
    Foe foe[SHB_MAX_FOES];
    EShot es[SHB_MAX_ESHOTS];
    PShot ps[SHB_MAX_PSHOTS];
    Pickup pk[SHB_MAX_PICKUPS];
    Part part[SHB_MAX_PARTS];
    Blast blast[SHB_MAX_BLASTS];
    int photo_seq, group_seq;
    int shake, flash;
    int warn_t; float warn_y; /* Comet Rain B's "!" */
    int msg_t; const char *msg;
    /* testing */
    bool god;
    int photos, rings_thrown, crystals_got, wrenches, retaliations;
    int big_kills;
    int drips, drip_bulbs, drips_in_stun; /* the Teapot's room */
} ShbGame;
extern ShbGame sb;
extern ShbSave sbs;
extern uint8_t shb_tile[SHB_MAX_COLS][SHB_MAX_ROWS];
extern uint8_t shb_bhp[SHB_MAX_COLS][SHB_MAX_ROWS];
extern uint8_t shb_cave[SHB_MAX_COLS];   /* 1: a cave, with rock behind */
extern int shb_cols, shb_rows;

/* shutterbug_play.c */
void shb_new_run(int players);
void shb_start_stage(int s);
void shb_play_update(void);
void shb_add_score(uint32_t pts);
void shb_hurt_ship(int p);
void shb_hurt_foe(int i, int dmg, bool ring);
void shb_kill_foe(int i, bool by_blast);
void shb_take_photo(int p);
int shb_add_eshot(int kind, float x, float y, float vx, float vy);
void shb_aimed(float x, float y, float speed, int n, float spread_deg);
void shb_ring(float x, float y, float speed, int n, float rot, int kind);
void shb_burst(float x, float y, int col, int n, float sp);
void shb_drop(int kind, float x, float y);
void shb_sfx(const char *name, int gap);
void shb_save_now(void);
void shb_clear_world(void);
bool shb_solid_at(float x, float y);
int shb_tile_at(float x, float y);
int shb_near_ship(float x, float y);   /* the nearest live ship, or -1 */
float shb_aim(float x, float y);       /* the angle to the nearest ship */
void shb_stage_won(void);
uint32_t shb_next_extend(void);
void shb_eshot_step(EShot *s);

/* shutterbug_foes.c */
extern int shb_nmover;        /* the moving blocks, which count as rock */
extern int shb_mover[8];
int shb_spawn(int kind, float x, float y, int flags, int arg);
void shb_run_spawns(void);
void shb_foes_update(void);
void shb_foe_photographed(int i, int photo);
bool shb_foe_hittable(const Foe *e);
bool shb_foe_harmful(const Foe *e);

/* shutterbug_bosses.c */
void shb_boss_init(Foe *e);
void shb_boss_update(int i);
bool shb_boss_photo(int i, int photo);
bool shb_boss_hit(int i, int dmg, float x, float y);   /* false: the shot is blocked */
void shb_boss_dead(int i);

/* shutterbug_stages.c */
void shb_build_terrain(int s);
float shb_band_lo(float x);
float shb_band_hi(float x);

/* shutterbug_bot.c: the demo player, for the tests */
int shb_bot_buttons(void);
extern bool shb_bot_letters;   /* go out of its way for U, F and O */

/* shutterbug_draw.c */
void shb_draw(void);
void shb_draw_label(int x, int y, int w, int h, int t);
int shb_credits_len(void);         /* the credits' height in pixels */
bool shb_kind_photographed(int kind);
extern const uint8_t SHB_CAST[];   /* the credits' roll call, in the order they turn up */
extern const int SHB_CAST_N;

/* art and sound */
enum {
    SP_POPPY, SP_POPPY_BARE, SP_SPRIG, SP_SPRIG_BARE,
    SP_PUFF, SP_MINT, SP_TURRET, SP_WALLBOMB, SP_CHOMPER, SP_CUBE, SP_CUBE_RED, SP_TOAST,
    SP_SWIRL, SP_ERUPTER, SP_BURSTER, SP_DART, SP_HOPPER,
    SP_GHOST, SP_CART, SP_RAILBOMB, SP_RAILBOMB_RED, SP_SILO, SP_ROCKET,
    SP_NIP, SP_KITE, SP_GEN, SP_TURRET_RED,
    SP_CRYSTAL, SP_WRENCH,
    SP_COUNT
};
extern Sprite shb_spr[SP_COUNT];
void shb_art_load(void);
void shb_audio_load(void);
extern int SHB_MUS_TITLE, SHB_MUS_STORY, SHB_MUS_STAGE[SHB_STAGES], SHB_MUS_BOSS, SHB_MUS_TRUE, SHB_MUS_CLEAR,
    SHB_MUS_LOST, SHB_MUS_OVER, SHB_MUS_ENDING, SHB_MUS_CREDITS;

#endif
