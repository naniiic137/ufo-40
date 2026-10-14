/* SHOWSTOPPER - shared declarations. Cartridge 43 of UFO 40.
 * A tribute to Elfazar's Hat (UFO 50 #43); see docs/games/43-showstopper.md.
 *
 * Clover, a show rabbit with a stolen wand, breaks out of Morrow the
 * Marvellous's travelling show and goes after him to free the other
 * animals. A top-down run-and-gun: walk eight ways, hold A to keep firing
 * one way while you move any way, B dashes (safe while it lasts, and over
 * pits). Foes and crates drop cards; every third card is a hand: a pair
 * gives a colour's small power, three of a kind its big one, three
 * different nothing. Powers last until a life is lost. Five stages, hidden
 * star passes that pay for the Hop (a bonus round that pays lives), and a
 * last form behind the last form for a run with no continue.
 *
 * Every position is fixed point (SST_FX units a pixel), every angle comes
 * from a table and every rule uses whole numbers, so a run plays the same
 * on every platform, and the demo player can try moves in a copy of it. */
#ifndef SST_H
#define SST_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include <ctype.h>

/* ---- fixed point and angles ---------------------------------------------- */
#define SST_FX 256
#define SST_PX(v) ((v) >> 8)
#define SST_ANG 64
extern const int16_t SST_SIN[SST_ANG];
static inline int sst_sin(int a) { return SST_SIN[a & 63]; }
static inline int sst_cos(int a) { return SST_SIN[(a + 16) & 63]; }
int sst_dir_to(int dx, int dy);  /* nearest table angle to (dx, dy); 0 = right, 16 = down */
int sst_dist(int dx, int dy);    /* whole-number length */
int sst_isqrt(int v);

/* the eight ways: 0 right, 1 down-right, 2 down ... 6 up, 7 up-right */
extern const int8_t SST_DX8[8], SST_DY8[8];
int sst_dir8_of(uint32_t held);  /* -1 when the pad is still */

/* ---- the screen ------------------------------------------------------------ */
#define SST_TILE 16
#define SST_COLS 20
#define SST_HUD 12                         /* the strip along the top */
#define SST_VIEW_H (SCREEN_H - SST_HUD)    /* 168 */

/* ---- the rules (numbers are ours where no source gives them) -------------- */
#define SST_STAGES 5
#define SST_START_HP 3
#define SST_MAX_HP 4
#define SST_START_LIVES 3                  /* a credit: three lives */
#define SST_MAX_LIVES 9
#define SST_WALK 320                       /* 1.25 px a frame */
#define SST_WALK_DIAG 226
#define SST_DASH_T 14                      /* the dash: 14 frames ... */
#define SST_DASH_V 896                     /* ... at 3.5 px a frame */
#define SST_DASH_CD 10                     /* frames before the next */
#define SST_INV_HIT 90                     /* safe after a hit */
#define SST_INV_POP 120                    /* safe after the bubble pops */
#define SST_BUBBLE_T 300                   /* the bubble's time to pick a spot */
#define SST_SHOT_V 1280                    /* 5 px a frame */
#define SST_SHOT_CD 5                      /* a fresh press fires at most this often */
#define SST_HOLD_FIRST 12                  /* held: the first burst ... */
#define SST_BURST_GAP 6                    /* ... three shots this far apart ... */
#define SST_BURST_EVERY 40                 /* ... one burst this often */
#define SST_MAX_SHOTS 14
#define SST_STUN 75                        /* a dash through a foe stuns it */
#define SST_PASS_HITS 5                    /* hits to bring a star pass out */
#define SST_PASSES 9                       /* in the whole game */
#define SST_CARD_LIFE 540
#define SST_SHIELD_HP 3
#define SST_BOSS_INTRO 90                 /* frames a boss shows itself before it fights */

/* card colours (and the powers they make) */
enum { CC_NONE, CC_LIFE, CC_SHOT, CC_OPTION, CC_BOMB, CC_SHIELD, CC_COUNT };
/* what the last hand did */
enum { HAND_NOTHING, HAND_A, HAND_B };

/* ---- the map ---------------------------------------------------------------- */
/* Tiles, one character each in a stage's rows (20 a row):
 *   .  floor              ,  path / cobbles      ~  ice (slides)
 *   #  wall               T  tree / pillar       R  roof (snow falls below)
 *   o  pit                w  water (a pit)       =  bridge (floor)
 *   W  window wall (a bat waits behind it)       H  wall with a gun / torch
 * Things, on floor unless said (the tile under them is floor):
 *   P  where Clover starts        c  crate       b  barrel     $  hidden spot
 *   g  gumdrop    L  lantern      j  jester      s  snowball   v  bat (on W)
 *   G  wall gun (on H)            k  bomber (stands on a roof)  n  snowman
 *   r  rocketeer  h  copter       u  turtle      K  harlequin  z  skull jet (on H)
 *   f  emberling  B  book train (comes from the side)    q  page imp
 *   C  racecar (drives across)    t  torch (on H)
 *   1 2  mini-bosses   9  the boss   7  the chase starts   8  the chase stops
 *   |  the second half starts on this row (music) */
typedef struct {
    const char *name;        /* "FROSTBELL LANE" */
    const char *half_b;      /* the second half's name, or NULL */
    uint8_t kind;            /* SK_* */
    uint8_t theme;
    const char *const *rows; /* top row first */
    int nrows;
    uint8_t boss1, boss2, boss9, chase; /* BK_* behind the markers */
    uint8_t passes;          /* hidden spots in the stage */
} SstStage;
enum { SK_SCROLL, SK_TRAIN, SK_LIFT };
enum { TH_SNOW, TH_TRAIN, TH_TOWN, TH_PALACE, TH_TOWER };
extern const SstStage SST_STAGE[SST_STAGES];

enum { T_FLOOR, T_PATH, T_ICE, T_WALL, T_TREE, T_ROOF, T_PIT, T_WATER, T_BRIDGE, T_WINDOW, T_GUNWALL, T_ROOFTOP };
int sst_tile_at(int tx, int ty);       /* T_*; outside the map: wall at the sides, floor above/below */
bool sst_tile_solid(int t);
bool sst_tile_pit(int t);
int sst_map_rows(void);
/* the demo player's guide: steps to the top of the stage from a tile */
int sst_guide(int tx, int ty);

/* ---- foes ---------------------------------------------------------------------- */
enum {
    FK_NONE,
    FK_CRATE, FK_BARREL, FK_SECRET,
    FK_GUMDROP, FK_LANTERN, FK_JESTER, FK_BAT, FK_GUN, FK_SNOWBALL,
    FK_GOURD, FK_PUFFER, FK_TOYTRAIN,
    FK_BOMBER, FK_SNOWMAN, FK_ROCKETEER, FK_COPTER, FK_GUN3,
    FK_TURTLE, FK_HARLEQUIN, FK_SKULLJET, FK_EMBER, FK_BOOK, FK_IMP, FK_RACECAR, FK_TORCH,
    FK_MOUSE,    /* the Snow Engine's homing critter */
    FK_SKULL,    /* Old Wicks's hatchlings */
    FK_EGG,
    FK_WISP,     /* Old Wicks's torches' roaming fire */
    FK_SWOOP,    /* Morrow's bats */
    FK_SPIKE,    /* the Doppels' spike blocks */
    FK_COUNT
};
typedef struct {
    const char *name;
    int8_t hp;
    int8_t hw, hh;           /* half box, px */
    uint8_t flags;
} SstFoeDef;
enum {
    FF_STUN = 1,     /* a dash stuns it */
    FF_SOLID = 2,    /* blocks Clover (crates) */
    FF_WALL = 4,     /* sits in a wall; no touching harm */
    FF_NOHURT = 8,   /* touching it is harmless */
    FF_FLY = 16,     /* over pits and walls */
    FF_ALWAYS = 32,  /* always drops a card */
    FF_NOCARD = 64   /* never drops one */
};
extern const SstFoeDef SST_FOE[FK_COUNT];

typedef struct {
    uint8_t alive, kind, stun, size;
    int x, y, vx, vy;        /* FX */
    int hp, t, a, b, c, d;   /* timers and the kind's own numbers */
    int hurt;                /* flash */
    int dir;
    int ord;                 /* a hidden spot's pass number */
} SstFoe;
#define SST_MAX_FOES 72

/* enemy shots */
enum {
    EB_ROUND, EB_STAR, EB_FIRE, EB_BIG,
    EB_MINE,     /* sits, can be shot (defused), bursts after its fuse */
    EB_LOB,      /* an arc to a spot (a = frames, then a small blast) */
    EB_ROCKET,   /* comes down on its red X (a = frames) */
    EB_HELIX,    /* the gourd cloaks' wavy pairs */
    EB_JET,      /* a skull's flame (lives a while, long) */
    EB_HOMING,
    EB_COUNT
};
typedef struct {
    uint8_t alive, kind;
    int x, y, vx, vy;        /* FX */
    int t, a, b;
} SstBullet;
#define SST_MAX_BULLETS 200

/* Clover's shots and her helpers' */
typedef struct {
    uint8_t alive, big, from_opt;
    int x, y, vx, vy, dmg;
} SstShot;

typedef struct { uint8_t alive, col; int x, y, t; } SstCard;
#define SST_MAX_CARDS 16

typedef struct { uint8_t alive; int x, y, t; } SstPass;
#define SST_MAX_PASSES 4

/* things on the ground */
enum { HZ_SLICK, HZ_SLIME, HZ_BLAST, HZ_MARK, HZ_SNOW, HZ_PBLAST };
typedef struct { uint8_t alive, kind; int x, y, t, r; } SstHaz;
#define SST_MAX_HAZ 64

typedef struct { uint8_t alive; int x, y, t; } SstBomb;
#define SST_MAX_BOMBS 8

/* ---- bosses -------------------------------------------------------------------- */
enum {
    BK_NONE,
    BK_ENGINE,     /* I mini: the Snow Engine */
    BK_GOGGLE,     /* I boss: Granny Goggle */
    BK_BOILER,     /* III mini: the Boiler, up the winding road */
    BK_DOPPELS,    /* III boss: the Doppels, two with one bar */
    BK_WICKS,      /* IV mini: Old Wicks */
    BK_RIBBON,     /* IV mini: the Ribbon */
    BK_MORROW,     /* IV boss: Morrow the Marvellous */
    BK_MAW,        /* V: the Morrowmaw */
    BK_HEART,      /* V, no continues: the Heartstack */
    BK_COUNT
};
typedef struct {
    uint8_t alive, kind, shut;
    int x, y, vx, vy;
    int hp, t, a, b;
    int8_t hw, hh;
} SstPart;
#define SST_MAX_PARTS 28
typedef struct {
    uint8_t alive, kind, phase, dying;
    int x, y, vx, vy;
    int hp, maxhp;
    int t, pt, a, b, c;
    int hurt, die_t;
    SstPart part[SST_MAX_PARTS];
    int nparts;
} SstBoss;
extern const char *const SST_BOSS_NAME[BK_COUNT];
extern const int SST_BOSS_HP[BK_COUNT];

/* ---- Clover --------------------------------------------------------------------- */
enum { PS_WALK, PS_DASH, PS_FALL, PS_DOWN, PS_BUBBLE, PS_GONE };
typedef struct {
    int x, y, vx, vy;        /* FX */
    int state, st;
    int face;                /* 0..7 */
    int move_dir;            /* the last way she moved */
    bool firing;             /* A held: the facing is locked */
    int hold_t, shot_cd, volley_first;
    int hp, max_hp, inv;
    int dash_t, dash_cd, dash_dir, dash_bombs;
    int safe_x, safe_y;      /* the last firm ground */
    int bub_t;
    uint8_t pow[CC_COUNT];   /* 0 none, 1 = A, 2 = B */
    int shield[5];           /* each spike ball's strength */
    int shield_ang;
    int opt_x, opt_y, opt_cd;
    int trail_x[16], trail_y[16], trail_i;
    uint8_t hand[3];
    int nhand;
    int hand_t, hand_res, hand_col;  /* the last hand, for the HUD */
    uint8_t shown[3];
    int step;
} SstPlayer;

/* ---- the Hop (the bonus round) ----------------------------------------------------- */
#define HOP_LANES 7
#define HOP_ROWS 24                  /* rows kept in the ring of the board */
enum { HT_HOLE, HT_TILE, HT_SPIKE, HT_MOUTH };
typedef struct {
    uint8_t tile[HOP_ROWS][HOP_LANES];
    uint8_t coin[HOP_ROWS][HOP_LANES];   /* 0 none, 1 a coin, 5 a big one */
    uint8_t phase[HOP_ROWS][HOP_LANES];  /* where a spike or mouth is in its cycle */
    int gen_row;                         /* rows made so far */
    int safe;                            /* the lane kept safe in the last row made */
    int lane, row;                       /* where the blob is (or lands) */
    int from_lane, from_row;
    int dir, next_dir;                   /* 0 on, 1 right lane, 2 back, 3 left lane */
    int hop_t;                           /* frames into this hop */
    int tries, coins, lives_won, next_mark;
    int state, st;                       /* HB_* */
    int cam;                             /* rows*16 the view has advanced */
    bool practice;
    int made_it;                         /* reached the far end */
    Rng rng;
} SstHop;
enum { HB_READY, HB_HOP, HB_FALL, HB_BITTEN, HB_DONE };
#define HOP_HOP_T 20
#define HOP_LENGTH 150               /* rows to the far end (the free ride has none) */
#define HOP_MARKS 9
extern const int SST_HOP_MARK[HOP_MARKS];

/* ---- the game ---------------------------------------------------------------------- */
enum {
    SS_TITLE, SS_STORY, SS_MAP, SS_PLAY, SS_CLEAR, SS_OFFER, SS_HOP, SS_HOPEND,
    SS_CONTINUE, SS_OVER, SS_ENDING, SS_CREDITS
};

typedef struct {
    uint32_t magic;
    uint16_t runs, wins, good_wins;
    uint16_t best_stage;        /* the furthest stage reached */
    uint16_t most_passes;       /* Passes Found: the most in one run */
    uint16_t hop_best;          /* the most coins in one Hop */
    uint16_t lives_won;         /* lives won in the Hop, ever */
    uint8_t free_ride;          /* the secret hand was played: practice the Hop */
    uint8_t pad[3];
} SstSave;

typedef struct {
    int x, y, kind, ord;
} SstSpawn;

typedef struct {
    int state, state_t, frame_t;
    int menu;                  /* the title's row */
    int offer_sel, cont_sel, cont_t;
    /* the run */
    int stage;                 /* 0..4 */
    int lives, continues;
    int passes;                /* held, not yet spent */
    uint16_t found;            /* which of the nine hidden spots are found */
    int passes_run;            /* found this run */
    bool won, good, practice;
    /* the stage */
    int cam_y, cam_lock, chase_stop;   /* px; lock -1 = none */
    int chase_acc;
    bool chasing;
    int next_spawn;
    int half;                  /* 0 or 1 */
    int wave, wave_t, wave_left;       /* the train and the lift */
    int lift_y;                /* the lift's climb (drawing) */
    int clear_t;               /* frames since the last boss fell */
    bool stage_done;
    int boss_seen;             /* bit per marker that has woken */
    bool heart_next;           /* the Morrowmaw fell on a clean run */
    SstPlayer pl;
    SstFoe foe[SST_MAX_FOES];
    SstBullet eb[SST_MAX_BULLETS];
    SstShot ps[SST_MAX_SHOTS + 8];
    SstCard card[SST_MAX_CARDS];
    SstPass pass[SST_MAX_PASSES];
    SstHaz hz[SST_MAX_HAZ];
    SstBomb bomb[SST_MAX_BOMBS];
    SstBoss boss;
    SstHop hop;
    Rng rng;
    uint32_t held, prev;       /* the buttons this frame and the last */
    int shake;
    /* the secret hand */
    int code_t;
    /* tests */
    bool god, frozen;
    int hits_taken, falls, kills, cards_got, hands_a, hands_b, hands_none;
    int shots_fired, dashes, stuns;
} SstGame;

extern SstGame sst_g;
extern SstSave sst_sv;
extern bool sst_sim;           /* the demo player is trying a move: no sound, no goals */

/* sst.c */
void sst_sfx(const char *name);
void sst_save_now(void);

/* sst_play.c: a stage being played */
void sst_start_stage(int stage);
void sst_play_step(uint32_t held);      /* one frame with these buttons */
void sst_player_hurt(int dmg);
void sst_lose_life(bool fell);
void sst_give_card(int col);
void sst_apply_hand(const uint8_t h[3]);
int sst_hand_result(const uint8_t h[3], int *col);
void sst_clear_powers(void);
int sst_add_bullet(int kind, int x, int y, int vx, int vy);
int sst_aim_bullet(int kind, int x, int y, int speed, int ang_ofs);
void sst_ring(int kind, int x, int y, int n, int speed, int ang0);
int sst_add_haz(int kind, int x, int y, int t, int r);
int sst_spawn_foe(int kind, int x, int y);
void sst_drop_card(int x, int y);
bool sst_foe_hit(int i, int dmg);
void sst_kill_foe(int i);
int sst_player_ang(int x, int y);       /* the angle from (x, y) to Clover */
bool sst_box_solid(int x, int y, int hw, int hh);   /* FX centre, px half sizes */
bool sst_on_screen(int x, int y, int margin);
int sst_live_foes(void);
void sst_blast(int x, int y, int r, int dmg, bool player_side);
void sst_boom(int x, int y, int r);

/* sst_foes.c */
void sst_foe_init(SstFoe *f);
void sst_foes_update(void);

/* sst_boss.c */
void sst_boss_start(int kind, int x, int y);
void sst_boss_update(void);
bool sst_boss_shot(int x, int y, int hw, int dmg);   /* a shot at (x, y) FX: true if it hit something of the boss */
bool sst_boss_touch(int x, int y, int hw);           /* Clover's box touches the boss */
int sst_boss_weak(int *x, int *y);                   /* where the demo player should aim */

/* sst_stages.c */
void sst_stage_spawns(int stage, const SstSpawn **list, int *n);
int sst_stage_pass_base(int stage);                  /* the first pass number in a stage */
void sst_train_waves(int wave, int *kinds, int *n, int max);
int sst_train_wave_count(void);
int sst_lift_wave_count(void);
void sst_lift_waves(int wave, int *kinds, int *n, int max);

/* sst_hop.c */
void sst_hop_start(int tries, bool practice);
void sst_hop_step(uint32_t held);
int sst_hop_tile(int row, int lane);
int sst_hop_coin(int row, int lane);
bool sst_hop_deadly(int row, int lane);

/* sst_bot.c */
int sst_bot_buttons(void);
extern int sst_bot_why;

/* sst_draw.c */
void sst_draw(void);
void sst_draw_label(int x, int y, int w, int h, int t);
int sst_layout_audit(void);
extern const char *const SST_STORY[];
extern const int SST_STORY_N;

/* sst_art.c */
enum {
    SP_CLOVER_D0, SP_CLOVER_D1, SP_CLOVER_U0, SP_CLOVER_U1, SP_CLOVER_S0, SP_CLOVER_S1, SP_CLOVER_DASH,
    SP_BUBBLE, SP_OPT_A, SP_OPT_B, SP_SPIKEBALL,
    SP_CARD0,                                   /* + colour - 1 */
    SP_CARD_BACK = SP_CARD0 + 5,
    SP_PASS, SP_HEART, SP_HEART_E, SP_LIFE,
    SP_CRATE, SP_BARREL,
    SP_GUMDROP, SP_LANTERN, SP_JESTER, SP_BAT, SP_SNOWBALL, SP_GOURD, SP_PUFFER, SP_TOYTRAIN,
    SP_BOMBER, SP_SNOWMAN, SP_ROCKETEER, SP_COPTER, SP_TURTLE, SP_HARLEQUIN, SP_SKULLJET,
    SP_EMBER, SP_BOOK, SP_IMP, SP_RACECAR, SP_MOUSE, SP_SKULL, SP_EGG, SP_SWOOP, SP_SPIKE,
    SP_SECRET_SNOW, SP_SECRET_TOWN, SP_SECRET_PALACE, SP_SECRET_TRAIN,
    SP_BLOB, SP_BLOB2, SP_COIN, SP_BIGCOIN, SP_GLOVE, SP_TORCHF, SP_BOMB,
    SP_COUNT
};
extern Sprite sst_spr[SP_COUNT];
void sst_art_load(void);
int sst_art_check(void);

/* sst_audio.c */
void sst_audio_load(void);
extern int SST_MUS_TITLE, SST_MUS_STORY, SST_MUS_MAP, SST_MUS_S1, SST_MUS_S2, SST_MUS_S3A, SST_MUS_S3B,
    SST_MUS_S4A, SST_MUS_S4B, SST_MUS_S5, SST_MUS_BOSS, SST_MUS_MAW, SST_MUS_HEART, SST_MUS_HOP, SST_MUS_CLEAR,
    SST_MUS_OVER, SST_MUS_BAD, SST_MUS_GOOD, SST_MUS_CREDITS;

#endif
