/* HOOPLA - shared declarations. Cartridge 36 of UFO 40.
 * A tribute to Hyper Contender (UFO 50 #36); see docs/games/36-hoopla.md.
 *
 * A one-screen, one-on-one platform brawl in the Glass Pit. Nobody has
 * health: you win by holding five hoops at once. Everyone starts with one,
 * a new hoop drops in every 15 seconds (too hot to touch at first), and
 * every hit knocks a hoop out of you (two if you were dizzy) to bounce
 * round the pit for whoever grabs it. Eight fighters, eight ways to move.
 *
 * Everything in play is whole numbers: positions and speeds in 1/256 of a
 * pixel, angles from a fixed table, so a match plays the same on every
 * machine. */
#ifndef HOOP_H
#define HOOP_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include <ctype.h>

/* ---- fixed point --------------------------------------------------------- */
#define HQ 256                  /* one pixel */
#define HPX(v) ((v) >> 8)       /* to whole pixels (floor) */

/* ---- the pit: one screen ------------------------------------------------- */
#define HOOP_AL 8               /* the walls' inner faces */
#define HOOP_AR 312
#define HOOP_AT 10              /* the ceiling's face */
#define HOOP_AB 152             /* the floor's face */
#define HOOP_PLAT_H 4           /* a ledge's thickness */
#define HOOP_FW 10              /* a fighter's box */
#define HOOP_FH 14

/* ---- the rules ----------------------------------------------------------- */
#define HOOP_FIGHTERS 8
#define HOOP_MAX_IN 3           /* fighters in one match (two, or three on RIOT) */
#define HOOP_GRAV 56            /* 0.22 px per frame per frame */
#define HOOP_MAXFALL 1024
#define HOOP_WALK 320
#define HOOP_HURT_T 36          /* knocked back, no control */
#define HOOP_INV_T 44           /* safe after a hit (hoops can still be grabbed): only just
                                 * longer than the stun, so a hit timed to its end stun-locks */
#define HOOP_DIZZY_T 80         /* a melee that hit a block: dizzy this long */
#define HOOP_MELEE_T 14         /* the lunge's active frames */
#define HOOP_MELEE_CD 34
#define HOOP_NOGRAB_T 14        /* a knocked-out hoop can't be caught at once */
#define HOOP_MAX_SHOTS 40
#define HOOP_MAX_MINES 12
#define HOOP_MAX_BLASTS 8
#define HOOP_MAX_RINGS 24
#define HOOP_MAX_PARTS 120
#define HOOP_MINE_ARM 60        /* Bristle's traps arm in 1 s */
#define HOOP_BOMB_FUSE 120      /* Collier's charges go off after 2 s */
#define HOOP_GLOW_T 60          /* Gulp glows after holding 1 s */
#define HOOP_DART_LOCK 120      /* a stuck fast dart: no darts for 2 s */
#define HOOP_OLD_HP 4           /* the old rules: four health, no hoops */

/* ring options (the defaults are what the goals need) */
#define HOOP_DEF_WIN 5
#define HOOP_DEF_START 1
#define HOOP_DEF_EVERY 15
#define HOOP_DEF_BURN 1

/* ---- the fighters ---------------------------------------------------------- */
enum {
    HF_TANSY,    /* double jump; a knife that bounces twice off walls */
    HF_CLAMP,    /* claw line; a spinning cog that comes back */
    HF_MOSS,     /* turns his own gravity over; a rocket that curves with it */
    HF_BRISTLE,  /* spring traps; a spray of three quills */
    HF_PEWIT,    /* wings; a horseshoe thrown in an arc */
    HF_COLLIER,  /* a cage lift; powder charges on a fuse */
    HF_ASTRA,    /* rocket pack; a ray that bounces once */
    HF_GULP,     /* a charged leap; a dart aimed while held */
};
typedef struct {
    const char *name;
    const char *title;         /* a few words under the name */
    const char *move;          /* what B does */
    const char *weapon;        /* what A does */
    uint8_t col_main, col_trim;   /* the first colours ... */
    uint8_t alt_main, alt_trim;   /* ... and the second ones */
    int tier;                  /* the CPU's draft order: lower picks first */
} HoopFighterDef;
extern const HoopFighterDef HOOP_DEF[HOOP_FIGHTERS];

/* the buttons a fighter reads each frame (human or CPU) */
enum { HP_L = 1, HP_R = 2, HP_U = 4, HP_D = 8, HP_A = 16, HP_B = 32 };
enum { CTRL_P1, CTRL_P2, CTRL_CPU };

enum { GND_AIR = -1, GND_FLOOR = 100, GND_CEIL = 101 };

typedef struct {
    bool on;
    int kind, pal;              /* fighter and colours (0 first, 1 second) */
    int ctrl;
    int x, y, vx, vy;           /* the box's top left, 1/256 px */
    int g;                      /* +1 falls down, -1 falls up (Moss) */
    int face;                   /* +1 right, -1 left */
    int ground;                 /* GND_* or a ledge's index */
    int drop_t;                 /* falling through ledges */
    int rings, hp;
    int hurt_t, inv_t, dizzy_t;
    int melee_t, melee_cd, melee_hit;
    int fire_cd;
    bool blocking;
    int held, prev;             /* HP_* this frame and last */
    /* Tansy */
    int jumps, spin_t, jump_hold;
    /* Clamp: the claw */
    int claw;                   /* 0 none, 1 flying, 2 caught */
    int cx, cy, cvx, cvy, rope; /* claw position and speed; the line's length */
    int swing_t, swing_dir;     /* the first arc hurts */
    int claw_ring;              /* a hoop on the claw (-1 none) */
    int claw_ledge;             /* the ledge the claw bit (-1 the ceiling) */
    int claw_dist;              /* how far it has flown */
    /* Pewit */
    int hover_t;
    /* Collier */
    int lift;                   /* 0, -1 going up, +1 going down */
    int lift_skip;              /* the ledge he started from */
    int held_bomb;              /* the charge in his hand (-1 none) */
    /* Astra */
    int thrust_t;
    /* Gulp */
    int charge_t, glow_jump, aim_t, aiming, lock_t;
    /* stats and looks */
    int step, anim_t, act_t;
    int hits_dealt, rings_taken;
} Fighter;

/* things in flight */
enum {
    SH_KNIFE, SH_SHORTKNIFE, SH_COG, SH_ROCKET, SH_DUD, SH_QUILL, SH_SHOE,
    SH_BOMB, SH_RAY, SH_DART, SH_FASTDART, SH_KINDS
};
typedef struct {
    uint8_t alive, kind, owner, bounces, stuck, held, arc, home;
    int x, y, vx, vy;           /* the middle, 1/256 px */
    int t, fuse;
    int prio;                   /* which shot wins when two meet */
    bool self_ok;               /* can hurt its owner now */
} Shot;

typedef struct { uint8_t alive, owner, landed; int x, y, vy, arm_t; } Mine;
typedef struct { uint8_t alive, owner, hurts_owner; int x, y, r, t, done; } Blast;

/* a hoop: loose (bouncing), or fresh (still, maybe on fire) */
enum { RG_FRESH, RG_LOOSE };
typedef struct {
    uint8_t alive, state;
    int x, y, vx, vy;           /* the middle, 1/256 px */
    int burn_t, nograb_t, t;
    int claw;                   /* held on a claw by this fighter (-1 none) */
} Ring;

typedef struct { int x, y, vx, vy, life, col; } Part;

/* ---- arenas ---------------------------------------------------------------- */
#define HOOP_ARENAS 12
#define HOOP_MAX_LEDGES 10
#define HOOP_SPOTS 6
enum { MV_NONE, MV_SIDE, MV_UPDOWN };
typedef struct { int16_t x, y, w; uint8_t mv; int16_t range, period; uint8_t phase; } LedgeDef;
typedef struct {
    const char *name;
    LedgeDef ledge[HOOP_MAX_LEDGES];
    int nledges;
    int16_t spot[HOOP_SPOTS][2];     /* where fresh hoops can appear */
    int16_t start[HOOP_MAX_IN][2];   /* fighters' feet at the start */
} ArenaDef;
extern const ArenaDef HOOP_ARENA[HOOP_ARENAS];
#define HOOP_PALS 6
extern const uint8_t HOOP_PAL[HOOP_PALS][5]; /* sky, far, ledge, ledge trim, floor */
typedef struct { int x, y, w, dx, dy; } Ledge; /* where it is now (px) and its last move */

/* ---- the CPU's brain, one per CPU fighter ---------------------------------- */
enum { SK_CALM, SK_ROUGH, SK_RIOT, SK_ACE };
typedef struct {
    int skill;
    int react;                  /* frames between fresh looks */
    int think_t;
    int tx, ty;                 /* where it's going, px */
    int goal;                   /* what it's doing (tests) */
    int hold_b, hold_a;         /* frames to keep holding */
    int tap_t;
    int foe;                    /* who it's after */
    int wait_t;
    int mistake_t;              /* Moss under a CPU: muddles his gravity */
    int block_t;
    int want_aim;
    int stuck_t, last_x, last_y;
    int detour;
    int last;                   /* what it held last frame */
    int mistake_flip;           /* the muddled turn-over still to press */
    int wp, wpx, wpy;           /* a stepping stone on the way up */
} Brain;

/* ---- the match ---------------------------------------------------------------- */
typedef struct {
    int n;                      /* fighters in it */
    Fighter f[HOOP_MAX_IN];
    Brain brain[HOOP_MAX_IN];
    Shot shot[HOOP_MAX_SHOTS];
    Mine mine[HOOP_MAX_MINES];
    Blast blast[HOOP_MAX_BLASTS];
    Ring ring[HOOP_MAX_RINGS];
    Part part[HOOP_MAX_PARTS];
    int arena, pal;
    Ledge ledge[HOOP_MAX_LEDGES];
    int nledges;
    int t;                      /* frames since GO */
    int spawn_t;                /* frames to the next hoop */
    int winner;                 /* -1 still going */
    int win_t;
    int ready_t;                /* the READY count before GO */
    bool old_rules;
    bool test_ledges;           /* tests: ledges set by hand stay put */
    /* the rules in force */
    int to_win, start_rings, every, burn;
    int shake;
    Rng rng;
} Match;

/* ---- the cartridge's save ---------------------------------------------------- */
typedef struct {
    uint32_t magic;
    uint8_t challenge;          /* 0 calm, 1 rough, 2 riot */
    uint8_t antigrav;           /* 1: Moss's left and right swap when he's upside down */
    uint8_t to_win, start_rings, every, burn;
    uint8_t champs;             /* fighters who won a tournament on standard hoops */
    uint8_t champs_alt;         /* ... in their second colours */
    uint8_t used;               /* fighters ever picked by player 1 */
    uint8_t pad0;
    uint16_t tourney_wins, draft_wins, matches, attract_runs;
    uint16_t least_rematches;   /* 0xFFFF: never won */
    uint16_t pad1;
} HoopSave;

/* ---- the whole game ---------------------------------------------------------- */
enum {
    HS_TITLE, HS_MODE, HS_OPTIONS, HS_SELECT, HS_LADDER, HS_VS, HS_MATCH, HS_RESULT,
    HS_REMATCH, HS_OUT, HS_DRAFT, HS_SERIES, HS_ENDING, HS_CREDITS, HS_ATTRACT, HS_NOTE
};
enum { MODE_TOURNEY, MODE_DRAFT, MODE_EXHIB };

typedef struct {
    int state, state_t, frame_t;
    int title_sel, mode_sel, opt_sel;
    int players;                /* 1 or 2 */
    int mode;
    /* select */
    int cur[2], pal[2];
    bool ud_held[2], ud_clean[2]; /* UP/DOWN: colours swap on letting go, unless B came too */
    bool picked[2];
    int pick[2];
    /* tournament */
    int me, me_pal;
    int order[HOOP_FIGHTERS];   /* the eight opponents, the mirror last */
    int round;                  /* 0..7 */
    bool used_rematch[HOOP_FIGHTERS];
    int rematches;
    int extra;                  /* RIOT: the second opponent this match (-1 none) */
    int rematch_sel;
    /* draft */
    int pool[7], npool;
    bool taken[7];
    int team[2][3], nteam[2];
    int draft_turn;             /* 0..5 */
    int draft_cur;
    int score[2];
    int queue[2][3], qi[2];     /* each side's fighting order this round of three */
    int last_winner;
    int series_used[2];         /* tests: who each side has fielded this series */
    /* the current match */
    Match m;
    int vs_a, vs_b;             /* the fighters on the VS card */
    bool won_last;
    /* the ending */
    int end_kind, end_pal;
    bool end_note;              /* the special note after Gulp's second colours */
    /* title extras */
    int idle_t;
    int code_i;                 /* the old rules code */
    bool old_rules;
    bool attract_note;
    int bot_t;                  /* tests: the demo player's menu timer */
    int bot_plan;               /* tests: which fighter the demo player takes next */
    int bot_draft;              /* tests: play draft battles instead */
} HoopGame;

extern HoopGame hg;
extern HoopSave hsv;

/* hoop_math.c */
extern const int16_t HOOP_COS[72], HOOP_SIN[72]; /* 5 degree steps, x256 */
int hoop_isqrt(int64_t v);
int hoop_tri(int t, int period, int range);       /* 0..range..0 */
int hoop_angle_of(int dx, int dy);                /* table index, 0..71 */

/* hoop_arenas.c */
void hoop_ledges_update(Match *m);
void hoop_ledges_place(Match *m);

/* hoop_match.c */
void hoop_match_begin(const int *kinds, const int *pals, const int *ctrls, int n, int skill);
void hoop_match_update(void);
bool hoop_match_over(void);
void hoop_hurt(int victim, int attacker, bool melee);
int hoop_add_shot(int kind, int owner, int x, int y, int vx, int vy);
void hoop_add_blast(int owner, int x, int y, int r, bool hurts_owner);
void hoop_drop_rings(int who, int n);
int hoop_spawn_ring(void);
bool hoop_on_ledge(const Fighter *f);
bool hoop_touch(const Fighter *f, int x, int y, int w, int h);
int hoop_feet_y(const Fighter *f);
void hoop_burst(int x, int y, int col, int n);
void hoop_sfx(const char *name);
int hoop_ring_rule_default(void);
bool hoop_melee_on(const Fighter *f);
void hoop_take_ring(int who, int r);

/* hoop_fighters.c: each fighter's B, A, up+A and down+A */
void hoop_fighter_act(int i);
void hoop_fighter_physics(int i);
void hoop_shots_update(void);
void hoop_mines_update(void);
int hoop_gulp_angle(const Fighter *f);

/* hoop_cpu.c */
void hoop_brain_init(Brain *b, int skill, int who);
int hoop_brain_think(int who);    /* HP_* bits for fighter who */

/* hoop_bot.c: the demo player for the tests */
int hoop_bot_buttons(void);

/* hoop_draw.c */
void hoop_draw(void);
void hoop_draw_label(int x, int y, int w, int h, int t);
void hoop_draw_fighter_big(int kind, int pal, int x, int y, int scale, int t);

/* hoop.c */
const char *hoop_ending_text(int kind);
void hoop_save_now(void);

/* art and sound */
enum {
    HSP_STAND, HSP_STEP, HSP_AIR, HSP_ACT, HSP_FRAMES
};
#define HSP_FIGHTER(k, fr) ((k) * HSP_FRAMES + (fr))
enum {
    HSP_KNIFE = HOOP_FIGHTERS * HSP_FRAMES, HSP_COG, HSP_COG2, HSP_ROCKET, HSP_QUILL, HSP_SHOE,
    HSP_BOMB, HSP_DART, HSP_MINE, HSP_MINE_ARMED, HSP_CLAW, HSP_RING, HSP_RING2, HSP_FIRE1, HSP_FIRE2,
    HSP_STAR, HSP_CAGE, HSP_COUNT
};
extern Sprite hoop_spr[HSP_COUNT];
void hoop_art_load(void);
int hoop_art_check(void);
void hoop_remap(int kind, int pal, uint8_t *map);
void hoop_audio_load(void);
extern int HOOP_MUS_TITLE, HOOP_MUS_SELECT, HOOP_MUS_FIGHT1, HOOP_MUS_FIGHT2, HOOP_MUS_FIGHT3, HOOP_MUS_MIRROR,
    HOOP_MUS_DRAFT, HOOP_MUS_WIN, HOOP_MUS_LOSE, HOOP_MUS_ENDING, HOOP_MUS_CREDITS, HOOP_MUS_CHAMP;

#endif
