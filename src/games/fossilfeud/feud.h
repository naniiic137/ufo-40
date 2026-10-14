/* FOSSIL FEUD - shared declarations. Cartridge 12 of UFO 40.
 * A tribute to Avianos (UFO 50 #12); see docs/games/12-fossil-feud.md.
 *
 * Two bird nations fight over a 5 x 4 board of tiles. Each turn you pray
 * to one of three Ancestors on offer (five in all, great old beasts whose
 * bones lie under the land); its three fixed actions then run in order.
 * Pray to one twice and it blesses one of its actions. When armies meet, a
 * battle plays out by itself and you only give the whole army orders.
 * Hold four totems through a whole round to win.
 *
 * Everything in play is whole numbers: battle positions in 1/256 px. */
#ifndef FEUD_H
#define FEUD_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include <ctype.h>

/* ---- the board ------------------------------------------------------------ */
#define FD_COLS 5
#define FD_ROWS 4
#define FD_TILES 20
#define FD_ANC 5          /* Ancestors */
#define FD_ACTS 15        /* three actions each */
#define FD_FLAGS 6        /* totems on the board */
#define FD_WIN_FLAGS 4    /* held through a round to win */
#define FD_BLD_MAX 3      /* buildings on one tile */
#define FD_STACK_MAX 99   /* of one kind on one tile */

enum { FU_STOMPER, FU_LANCER, FU_SLINGER, FU_MYSTIC, FU_SWIFT, FU_KINDS };
enum { FB_GRANARY, FB_ROOST, FB_SHRINE, FB_BATH, FB_KINDS };
enum { FT_MEADOW, FT_THICKET, FT_BONEYARD, FT_SCREE, FT_GORGE, FT_PEAK, FT_EYRIE, FT_KINDS };
enum { FR_GRAIN, FR_TOILERS, FR_RELICS, FR_KINDS };
enum { FA_GRANDJAW, FA_PLATEBACK, FA_LONGNECK, FA_SKYCREST, FA_THREEHORN };
enum { AK_GATHER, AK_ENLIST, AK_CALLUP, AK_BUILD, AK_MARCH, AK_BARTER, AK_OMEN };
enum { OM_TITHE, OM_SHIFT, OM_HAIL, OM_SIGHT, OM_UPDRAFT, OM_BOLT, OM_HATCH, OM_KINDS };
enum { MF_DIAG = 1, MF_SURPRISE = 2, MF_THROUGH = 4 };

#define FD_RES_CAP_GRAIN 99
#define FD_RES_CAP_OTHER 9

/* one thing on an action's list: a unit, a building, a trade or an omen, and its price */
typedef struct {
    uint8_t what;  /* FU_* / FB_* / OM_* ; for a trade the resource given */
    uint8_t res;   /* the resource paid (a trade: the resource got) */
    uint8_t cost;
    uint8_t gain;  /* a trade: how much is got */
} FeudOffer;

typedef struct {
    uint8_t n;
    FeudOffer off[4];
    uint8_t nf;        /* omens with the fog off */
    FeudOffer offf[3];
    /* GATHER: grain, grain a granary, toilers a bath, relics, relics a shrine
     * CALL UP: stompers a roost, lancers a roost, mystics a shrine, swifts a bath */
    int8_t g[5];
    uint8_t mflags;    /* MARCH */
} FeudLevel;

typedef struct {
    const char *name;
    uint8_t kind;      /* AK_* */
    FeudLevel lv[3];
} FeudActDef;
extern const FeudActDef FEUD_ACT[FD_ACTS];

typedef struct {
    const char *name, *beast, *say;
    uint8_t col;
} FeudAncDef;
extern const FeudAncDef FEUD_ANC[FD_ANC];

typedef struct {
    const char *name, *plural;
    int hp, shield;
    int speed;         /* 1/256 px a frame */
    int reach_lo, reach_hi;
    int every;         /* frames between attacks */
    int dmg_lo[FU_KINDS], dmg_hi[FU_KINDS]; /* against each kind; lo < 0: can't hit it */
    int worth;         /* for the score and the planners */
} FeudUnitDef;
extern const FeudUnitDef FEUD_UNIT[FU_KINDS];
extern const char *const FEUD_BLD_NAME[FB_KINDS];
extern const char *const FEUD_TERR_NAME[FT_KINDS];
extern const char *const FEUD_RES_NAME[FR_KINDS];
extern const char *const FEUD_OMEN_NAME[OM_KINDS];
extern const char *const FEUD_SIDE_NAME[2];

typedef struct {
    int8_t owner;            /* -1 nobody, 0 the Rust Roost (you), 1 the Slate Flock */
    uint8_t terr;            /* FT_* (an eyrie under a moved peak is FT_PEAK) */
    uint8_t eyrie;           /* a recruiting point, whatever the ground */
    uint8_t flag;            /* a totem */
    uint8_t ndrop, drop[2];  /* waiting for whoever claims the tile */
    uint8_t b[FB_KINDS];
    uint8_t u[FU_KINDS];     /* the owner's birds */
    uint8_t moved[FU_KINDS]; /* already moved in this march */
    uint8_t frozen;          /* turn ends left under the hailstorm */
    uint8_t seen;            /* far sight: you can see in until your turn ends */
} FeudTile;

typedef struct {
    int res[FR_KINDS];
    uint8_t favour[FD_ANC];
    uint8_t picks[FD_ANC];
    uint8_t hold;            /* held four totems when its last turn ended */
    uint8_t blessings;
} FeudSide;

typedef struct {
    FeudTile t[FD_TILES];
    FeudSide s[2];
    uint8_t star[2][FD_ACTS];
    uint8_t deck[FD_ANC][3];   /* each Ancestor's three actions, in order */
    int8_t recent[2];          /* the Ancestors prayed to in the last two turns */
    uint8_t offer[3], noffer;
    int turn;                  /* 1, 2, ... counting both sides' turns */
    int side;                  /* whose turn */
    uint8_t fog;
    uint8_t ordeal;            /* 0, or 1-5 */
    int8_t winner;             /* -1 still on */
    uint8_t shuffled;          /* ordeal 4: Move previews shown */
    Rng rng;
} FeudMatch;

typedef struct {
    uint8_t eyries, peaks, fog;
    uint8_t ordeal;
    uint64_t seed;
} FeudSetup;

/* ---- battles ------------------------------------------------------------- */
#define FB_FW 300            /* the field, px */
#define FB_FH 110
#define FB_CELL 10
#define FB_GW 30
#define FB_GH 11
#define FB_ON_FIELD 40       /* a side's birds on the field at once */
#define FB_MAX (FB_ON_FIELD * 2)
#define FB_MAX_SHOTS 64
#define FB_STILL_END 180     /* nobody hurt and nobody moving this long: the attacker withdraws */
#define FB_TIME_MAX 7200     /* two minutes: the attacker withdraws */
#define FB_BLD_HP 24

enum { FO_CHARGE, FO_STAND, FO_WRECK, FO_FLEE, FO_KINDS };
enum { BC_OPEN, BC_COVER, BC_BLOCK, BC_HIGH };
enum { BR_NONE, BR_ATK_WIN, BR_DEF_WIN, BR_ATK_BACK, BR_DEF_FLED };

typedef struct {
    uint8_t alive, side, kind, frozen;
    int16_t hp, shield;
    int x, y;                /* 1/256 px on the field */
    int cd;                  /* frames to the next attack */
    int tgt;                 /* a soldier, or -2 - building */
    int wp;                  /* waypoint cell, -1 none */
    int path_t;
    int engaged_t;           /* in a melee lately: the shield is down */
    int anim, hit_t, face;
} FeudSoldier;

typedef struct {
    uint8_t alive, kind, side;
    int x, y, tx, ty, vx, vy, t;
    int tgt;                 /* soldier index, or -2 - building */
    uint8_t blocked;         /* stops at a cover cell */
    uint8_t high;            /* loosed from high ground */
    int stop_x, stop_y;
} FeudShot;

typedef struct {
    int x, y, hp, kind;
} FeudFieldBld;

typedef struct {
    int atk, def;            /* sides */
    int src, dst;            /* tiles; src -1: from nowhere (summoned) */
    uint8_t terr;
    uint8_t grid[FB_GH][FB_GW];
    FeudSoldier s[FB_MAX];
    FeudShot sh[FB_MAX_SHOTS];
    FeudFieldBld bld[FD_BLD_MAX];
    int nbld;
    int reserve[2][FU_KINDS];
    int escaped[2][FU_KINDS];
    int lost[2][FU_KINDS];
    int order[2];
    uint8_t surprise;        /* the defenders don't stir till they are attacked */
    uint8_t woke;
    uint8_t frozen_def;
    int t, still_t;
    int result;
    int end_t;
    int hits;                /* for the tests */
    Rng rng;
    int blocked_shots, shield_saves;
} FeudBattle;

/* ---- the planner's moves ----------------------------------------------- */
enum { ST_NONE, ST_ENLIST, ST_BUILD, ST_MOVE, ST_TRADE, ST_OMEN };
typedef struct {
    uint8_t type;
    int8_t tile, tile2;
    uint8_t idx;             /* the offer on the action's list */
    uint8_t anc;             /* tithe: the Ancestor */
    uint8_t counts[FU_KINDS];
} FeudStep;

/* ---- the cartridge's screens -------------------------------------------- */
enum {
    FS_TITLE, FS_CAMPAIGN, FS_CUSTOM, FS_STORY, FS_MATCH, FS_BATTLE, FS_RESULT, FS_ENDING, FS_RECORDS
};
enum { MP_BANNER, MP_PRAY, MP_BLESS, MP_ACT, MP_CPU, MP_OVER };
enum { AU_CONFIRM, AU_MAP, AU_MENU, AU_SPLIT, AU_DEST, AU_ANC, AU_TARGET };

#define FD_SCEN 8            /* three moults and five ordeals */
#define FD_LEVELS 3

typedef struct {
    uint32_t magic;
    uint8_t beaten;          /* bit per scenario */
    uint8_t gift;
    uint8_t eyries, peaks, fog, opp;  /* the custom match */
    uint16_t picks[FD_ANC];
    uint16_t wins, losses, matches;
    uint16_t best_turns;     /* fewest turns to win a match (0 none) */
    uint8_t secret;          /* the bones spoke */
    uint8_t pad[3];
} FeudSave;

typedef struct {
    int state, state_t, frame_t;
    int title_sel, camp_sel, cust_sel;
    int scenario;            /* 0-7, or -1 a custom match */
    int ai_level;            /* 0-2 */
    FeudMatch m;
    FeudBattle b;
    int mp, mp_t;
    int act_i, act_id, au;
    int anc;                 /* the Ancestor you prayed to this turn */
    int tgt_stage;           /* shifting a peak: 0 which, 1 where */
    int cpu_anc, hl_tile;
    int pray_sel, bless_sel, anc_sel;
    int cur;                 /* the map cursor */
    int msel;
    int split[FU_KINDS], split_sel;
    int src;
    int omen, omen_cost;
    int tithe_bless;         /* a tithe brought a blessing: the Ancestor + 1 */
    int bless_anc;
    int code_i;              /* the secret: LEFT RIGHT RIGHT LEFT LEFT LEFT at the prayer */
    int secret_t;
    char msg[96];
    int msg_t;
    char note[96];           /* the last thing that happened, under the board */
    /* the Slate Flock's turn, step by step */
    int cpu_stage, cpu_wait, cpu_act, cpu_moves;
    Rng ai_rng;
    /* a battle */
    int battle_return;       /* 0 your action, 1 the Slate Flock's */
    int lock_orders;         /* tests: the Slate Flock's battle orders stay as set */
    FeudStep pend;           /* the move that started it */
    int battle_counts_src[FU_KINDS];
    int result_t, won;
    int ending_t;
    int turns_played;
    int last_frozen;         /* how many the last hailstorm froze */
    int last_bolt;           /* how many the last thunderbolt killed */
    int records_back;
} FeudGame;

extern FeudGame fg;
extern FeudSave fsv;

/* feud_rules.c */
extern int feud_last_got[FR_KINDS]; /* what the last move's claim gave */
void feud_setup_match(FeudMatch *m, const FeudSetup *su);
void feud_make_offers(FeudMatch *m);
int feud_level(const FeudMatch *m, int side, int act);
const FeudLevel *feud_lv(const FeudMatch *m, int side, int act);
int feud_offers_of(const FeudMatch *m, int side, int act, const FeudOffer **out);
bool feud_pray(FeudMatch *m, int anc);               /* true: a blessing is due */
bool feud_bless_due(const FeudMatch *m, int side, int anc);
bool feud_can_bless(const FeudMatch *m, int side, int anc);
bool feud_bless(FeudMatch *m, int side, int act);
bool feud_add_favour(FeudMatch *m, int side, int anc); /* true: a blessing is due */
bool feud_anc_maxed(const FeudMatch *m, int side, int anc);
void feud_gather(FeudMatch *m, int side, int act, int got[FR_KINDS]);
void feud_callup(FeudMatch *m, int side, int act, int got[FU_KINDS]);
bool feud_is_recruit_point(const FeudMatch *m, int side, int t);
bool feud_can_afford(const FeudMatch *m, int side, const FeudOffer *o);
bool feud_enlist(FeudMatch *m, int side, int act, int t, int idx);
bool feud_can_build_on(const FeudMatch *m, int side, int t);
bool feud_build(FeudMatch *m, int side, int act, int t, int idx);
bool feud_trade(FeudMatch *m, int side, int act, int idx);
int feud_movable(const FeudMatch *m, int t, int kind);
bool feud_can_move_from(const FeudMatch *m, int side, int t);
uint32_t feud_march_dests(const FeudMatch *m, int side, int src, int mflags);
uint32_t feud_flight_dests(const FeudMatch *m, int side, int src);
/* the move itself: 0 done (moved or claimed), 1 a battle is needed */
int feud_move(FeudMatch *m, int side, int src, int dst, const uint8_t counts[FU_KINDS]);
void feud_claim(FeudMatch *m, int side, int t, int got[FR_KINDS]);
void feud_reset_moved(FeudMatch *m);
uint32_t feud_omen_targets(const FeudMatch *m, int side, int omen);
uint32_t feud_shift_dests(const FeudMatch *m, int src);
int feud_omen_cost(const FeudMatch *m, int side, int act, int omen);
int feud_bolt(FeudMatch *m, int t);
int feud_hail(FeudMatch *m, int t);
void feud_shift(FeudMatch *m, int src, int dst);
int feud_hatch(FeudMatch *m, int side, int t);
void feud_end_turn(FeudMatch *m);
void feud_start_turn(FeudMatch *m);
int feud_flags_held(const FeudMatch *m, int side);
int feud_tiles_held(const FeudMatch *m, int side);
int feud_units_on(const FeudMatch *m, int t);
int feud_army(const FeudMatch *m, int side);
int feud_bld_count(const FeudMatch *m, int side, int kind);
int feud_tile_blds(const FeudTile *t);
bool feud_adjacent(int a, int b, bool diag);
int feud_dist(int a, int b);
void feud_add_res(FeudMatch *m, int side, int r, int n);
bool feud_gift_check(const FeudMatch *m, int side);
void feud_check_elimination(FeudMatch *m);
int feud_level_text(const FeudMatch *m, int side, int act, int lvl, char *buf, int n);
int feud_offer_text(const FeudMatch *m, int act, const FeudOffer *o, char *buf, int n);

/* feud_battle.c */
void feud_battle_begin(FeudBattle *b, const FeudMatch *m, int atk, int src, int dst, const uint8_t counts[FU_KINDS], bool surprise);
void feud_battle_step(FeudBattle *b);
void feud_battle_apply(FeudMatch *m, const FeudBattle *b, int got[FR_KINDS]);
int feud_battle_power(const FeudBattle *b, int side);
int feud_battle_count(const FeudBattle *b, int side);
void feud_battle_ai_order(FeudBattle *b, int side);
extern bool feud_quiet;     /* planning: no sounds */

/* feud_ai.c */
int feud_unit_power(int kind, const int enemy[FU_KINDS]);
int feud_tile_power(const FeudMatch *m, int t, const int enemy[FU_KINDS], bool defending);
int feud_eval(const FeudMatch *m, int side);
int feud_ai_pick_anc(const FeudMatch *m, int side, int skill, Rng *r);
int feud_ai_pick_bless(const FeudMatch *m, int side, int anc);
bool feud_ai_next(const FeudMatch *m, int side, int act, int skill, FeudStep *st);
void feud_ai_apply_est(FeudMatch *m, int side, int act, const FeudStep *st);
int feud_estimate(const FeudMatch *m, int atk, int dst, const uint8_t counts[FU_KINDS], bool surprise, int surv_a[FU_KINDS], int surv_d[FU_KINDS]);
#define FD_SKILL_BOT 3

/* feud_turn.c (the match screens: your turn, the Slate Flock's) */
void feud_match_begin(int scenario);
void feud_match_update(void);
void feud_battle_update(void);
void feud_start_action(void);
const char *feud_turn_hint(void);
uint32_t feud_valid_tiles(void);
int feud_menu_count(void);
void feud_say(const char *s);
void feud_sfx(const char *name);
void feud_battle_done(void);
void feud_finish_match(int winner);

/* feud_bot.c */
int feud_bot_buttons(void);
extern int feud_bot_goal_type;

/* feud.c */
void feud_save_now(void);
void feud_award_gift(void);

/* feud_draw.c */
void feud_draw(void);
void feud_draw_label(int x, int y, int w, int h, int t);
int feud_layout_audit(void);

/* feud_art.c */
enum {
    FSP_DINO0, FSP_DINO1, FSP_DINO2, FSP_DINO3, FSP_DINO4,
    FSP_UNIT0, /* five kinds x two frames: walk, strike */
    FSP_MINI0 = FSP_UNIT0 + FU_KINDS * 2,
    FSP_BLD0 = FSP_MINI0 + FU_KINDS,
    FSP_RES0 = FSP_BLD0 + FB_KINDS,
    FSP_TERR0 = FSP_RES0 + FR_KINDS,
    FSP_TOTEM = FSP_TERR0 + FT_KINDS,
    FSP_SNOW, FSP_EYE, FSP_STAR, FSP_STAR_OFF, FSP_BIRD_BIG, FSP_BIRD_BIG2,
    FSP_COUNT
};
extern Sprite feud_spr[FSP_COUNT];
void feud_art_load(void);
int feud_art_check(void);

/* feud_audio.c */
extern int FEUD_MUS_TITLE, FEUD_MUS_MAP, FEUD_MUS_RIVAL, FEUD_MUS_BATTLE, FEUD_MUS_ORDEAL, FEUD_MUS_WIN,
    FEUD_MUS_LOSE, FEUD_MUS_ENDING, FEUD_MUS_STORY;
void feud_audio_load(void);

#endif
