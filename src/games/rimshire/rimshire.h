/* RIMSHIRE - shared declarations. Cartridge 41 of UFO 40, a tribute to
 * Lords of Diskonia (UFO 50 #41). See docs/games/41-rimshire.md.
 *
 * Two layers, as in the original: a board of roads where the Brass and Plum
 * banners take turns drawing their routes (rimshire_map.c), and flick
 * battles on a walled field where disks are aimed, charged and launched,
 * bouncing off walls and each other (rimshire_battle.c). */
#ifndef RIMSHIRE_H
#define RIMSHIRE_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the disks -------------------------------------------------------------- */
enum {
    K_SQUIRE, K_WARDEN, K_FERRET, K_BRUTE, K_SLINGER, K_TOADKIN, K_OOZE, K_HEXER,
    K_MENHIR, K_ADDER, K_FRIAR, K_PIPER, K_LEECH, K_DELVER, K_WYRM, K_EMPRESS,
    K_COUNT
};
enum { SZ_SMALL, SZ_MID, SZ_LARGE };
/* what a disk's projectile does */
enum { RG_NONE, RG_HIT, RG_STUN, RG_EMBERS, RG_HEAL, RG_STAR };
/* what its melee does beyond damage */
enum { FX_NONE, FX_STUN, FX_POISON };

typedef struct RshKind {
    const char *name;
    char letter;          /* for the scenario tables */
    int size, hp, cost;   /* cost 0: never for hire */
    int melee, moves;     /* damage of each melee move, and how many */
    int ranged, rkind;    /* the projectile: its damage (or heal) and what it does */
    int charge;           /* most power pips for a move */
    int pcharge;          /* ... and for its projectile */
    int fx;               /* FX_* on melee hits */
    int aqua, anchored, drain, triple;
    const char *blurb;
} RshKind;
extern const RshKind RSH_KIND[K_COUNT];
int rsh_kind_of_letter(char c);
int rsh_kind_value(int k);   /* rough worth, for the computer's choices */

/* ---- skills (from the tomes) ------------------------------------------------- */
enum {
    SK_COMMAND = 1,     /* choose from five disks, not three */
    SK_STOCKPILE = 2,   /* battles start with 5 shards, not 2 */
    SK_SCOUTING = 4,    /* three moves on the board, not two */
    SK_HAGGLING = 8,    /* 4 coins each time you pass an inn */
    SK_PROSPECT = 16,   /* seams pay 2 a turn */
    SK_SIGHTLINE = 32,  /* the long aim line with its bounces */
    SK_HOBNAILS = 64,   /* sand does not slow your disks */
    SK_REMEDY = 128     /* every disk 1 more hit point in battle */
};
#define RSH_SKILLS 8
extern const char *const RSH_SKILL_NAME[RSH_SKILLS];
extern const char *const RSH_SKILL_TEXT[RSH_SKILLS];

/* ---- the board ---------------------------------------------------------------- */
#define RSH_MW 16          /* nodes across, at most */
#define RSH_MH 8           /* nodes down, at most */
#define RSH_NODE_GAP 20    /* pixels between nodes on screen */
enum { N_NONE, N_PLAIN, N_BASE0, N_BASE1, N_INN, N_TOME, N_SEAM, N_CHEST };
enum { T_GRASS, T_STONE, T_SAND, T_WATER };
#define RSH_ARMY 8         /* disks in the field ... */
#define RSH_RESERVE 6      /* ... and in reserve */
#define RSH_TRAIL 96

typedef struct RshScenario {
    const char *name;
    const char *intro;         /* one line on the card */
    int w, h;
    char border;               /* terrain beyond the edge */
    const char *rows[15];      /* 2h-1 strings of 2w-1 characters */
    int coins[2];
    const char *army[2], *reserve[2]; /* kind letters */
    uint16_t pool;             /* kinds the inns offer (bits) */
    uint16_t fresh;            /* kinds shown as new on the card */
    uint8_t castle;            /* bit 0: Brass's base is a castle, bit 1: Plum's */
    uint8_t cpu;               /* the Plum banner's sharpness 0..10 */
    uint8_t first;             /* who moves first */
    uint8_t battle_coins;      /* extra coins on the fields (no map gold) */
    uint32_t seed;
} RshScenario;
#define RSH_SCENARIOS 10
extern const RshScenario RSH_SCEN[RSH_SCENARIOS];

typedef struct RshSide {
    int nx, ny;                /* the banner's node */
    int bx, by;                /* home */
    int castle;
    int coins, skills;
    uint8_t army[RSH_ARMY]; int n_army;
    uint8_t reserve[RSH_RESERVE]; int n_reserve;
    int trail_n;
    int8_t tx[RSH_TRAIL], ty[RSH_TRAIL]; /* the route from home, home first */
    int cpu;                   /* -1 a player, else the computer's sharpness */
    int wins, losses;
} RshSide;

/* war states */
enum { W_PLAN, W_WALK, W_INN, W_TOME, W_BATTLE, W_OVER, W_CPU, W_SWAP };

typedef struct RshWar {
    int scen;                  /* 0..9, or -1 for a streak or versus war */
    int w, h;
    char border;
    uint8_t node[RSH_MH][RSH_MW];
    uint8_t er[RSH_MH][RSH_MW];   /* a road to the right */
    uint8_t ed[RSH_MH][RSH_MW];   /* a road down */
    uint8_t tile[RSH_MH + 1][RSH_MW + 1]; /* tile (x,y) lies up-left of node (x,y) */
    uint8_t owner[RSH_MH][RSH_MW];        /* seams: 0 nobody, 1 Brass, 2 Plum */
    uint8_t visits[RSH_MH][RSH_MW];       /* inns: 3 visits and it closes */
    uint8_t inn_offer[RSH_MH][RSH_MW][3]; /* each inn's three disks, the same all war */
    uint8_t seam_left[RSH_MH][RSH_MW];    /* seams: turns of pay left (10) */
    uint16_t pool;
    uint8_t battle_coins;
    RshSide s[2];
    int turn;                  /* whose turn */
    int first_move;            /* the war's first turn: one move */
    int turns;                 /* turns taken, both sides */
    int moves;                 /* moves this turn */
    int moves_used;
    int plan_n;                /* planned steps (d-pad) */
    int8_t plan_x[4], plan_y[4];
    int walk_i, walk_t;        /* walking the confirmed plan */
    int state, state_t;
    uint8_t offer[3];          /* the inn's three disks */
    int swap_kind;             /* a disk bought with a full field army: who goes to the reserve */
    uint8_t tome[3];           /* the tome's three skills (bit index) */
    int menu_sel;
    int winner;                /* W_OVER: 0, 1, or 2 for a stalemate */
    int bx, by;                /* where the battle is */
    int attacker;
    int regroup;               /* the battle is a base's second stand */
    int home_hold;             /* frames B has been held (held long enough: home) */
    char msg[40];              /* the location window: what the last place did */
    int msg_t;
    Rng rng;
} RshWar;
extern RshWar rw;

void rsh_war_start(const RshScenario *sc, int scen, int cpu0, int cpu1);
void rsh_war_streak(uint32_t seed, int streak, int versus);
bool rsh_node_ok(int x, int y);
bool rsh_road(int x0, int y0, int x1, int y1);
int rsh_step_ok(int side, int fx, int fy, int dir, int *nx, int *ny); /* 0 no, 1 yes, 2 battle, 3 home */
bool rsh_in_trail(int side, int x, int y);
int rsh_try_step(int dir);      /* the d-pad: one space now; 0 if the road is closed */
bool rsh_can_step(void);        /* the side to move has somewhere to go */
int rsh_step_kind(int dir);     /* what a step that way would be: 0 none, 1 a step, 2 a battle, 3 home */
void rsh_go_home(int side);     /* B: back to base, the turn ends */
void rsh_swap_pick(int i);      /* send field disk i to the reserve (W_SWAP); -1 cancels */
void rsh_war_update(void);      /* walking, the computer's turns */
void rsh_inn_buy(int i);
void rsh_inn_leave(void);
void rsh_tome_pick(int i);
int rsh_army_size(int side);
int rsh_strength(int side);
void rsh_battle_over(int winner); /* called when the field is decided */
int rsh_cpu_map_plan(int side, int8_t *px, int8_t *py); /* steps chosen, or -1: go home */
void rsh_end_turn(void);
extern const int RSH_DX[4], RSH_DY[4];

/* ---- the battlefield ------------------------------------------------------------ */
#define RSH_AW 384         /* the field, in pixels */
#define RSH_AH 256
#define RSH_CELL 8
#define RSH_CW (RSH_AW / RSH_CELL)
#define RSH_CH (RSH_AH / RSH_CELL)
#define RSH_VIEW_Y 16      /* the field's window on screen */
#define RSH_VIEW_H 164
#define RSH_MAXD 24        /* disks (and the one projectile) */
#define RSH_MAXO 28        /* pickups, springs, piles, embers */
#define RSH_PIP_FRAMES 12  /* frames per power pip */
#define RSH_RESET_FRAMES 180 /* held at full with no other input: the shot resets */
#define RSH_HOME_HOLD 20   /* frames B is held on the board to retreat home */
#define RSH_SEAM_TURNS 10  /* a seam pays this many times, then it is worked out */
#define RSH_STALE_TURNS 10 /* five full rounds with nothing touched: a stalemate */
#define RSH_MAX_STARS 5
#define RSH_FOG_ROUND 5    /* the haze starts closing once both sides have had this many turns */
#define RSH_FOG_STEP 14    /* ... this far each round */
#define RSH_FOG_MAX 96

typedef struct RshDisk {
    uint8_t on, side, kind, proj;
    int8_t hp, maxhp;
    uint8_t stun, poison, stars, slot;
    float x, y, vx, vy, r, m;
} RshDisk;

enum { O_COIN, O_SHARD, O_TONIC, O_WELL, O_PILE, O_CLUSTER, O_EMBER, O_TREE };
typedef struct RshObj {
    uint8_t on, kind, left, pad;
    float x, y, r;
} RshObj;

/* the part of a battle that a shot changes: the computer copies it to try shots */
typedef struct RshPhys {
    RshDisk d[RSH_MAXD];
    RshObj o[RSH_MAXO];
    uint8_t contact[RSH_MAXD][RSH_MAXD];
    uint8_t ocontact[RSH_MAXD][RSH_MAXO];
    int skills[2];
    int shards[2], coins[2];
    int chain_side;            /* -1: no shot in flight */
    int chain_dmg, chain_fx, chain_kind, chain_disk, chain_ranged;
    int combo, best_combo;
    int hits, kills[2], water_deaths, heals, stars_given;
    int touches;               /* contacts and pickups: a battle with none for five rounds is a stalemate */
    int moving;                /* anything still moving */
    int frames;
    uint8_t sim;               /* a trial: no events */
    int n_ev;
    struct { uint8_t kind; int16_t x, y, a; } ev[24];
} RshPhys;

enum { BE_HIT, BE_KILL, BE_WALL, BE_SPLASH, BE_PICK, BE_HEAL, BE_STAR, BE_EMBER, BE_STRIKE, BE_POISON, BE_STUN };

enum { B_WIN0, B_WIN1, B_BOTH, B_STALE }; /* how a battle ended (rb.winner) */

/* battle phases */
enum { B_INTRO, B_SELECT, B_AIM, B_ROLL, B_RAIM, B_END, B_OVER, B_THINK };

typedef struct RshBattle {
    uint8_t cell[RSH_CH][RSH_CW];
    uint8_t quad[4];           /* the four corner tiles of the board node */
    RshPhys p;
    uint8_t queue[2][RSH_MAXD];
    int qn[2];
    int side;                  /* whose turn */
    int phase, phase_t;
    int sel;                   /* the chosen disk */
    int cursor;                /* in the choosable part of the queue */
    int move_i, moves;         /* melee moves taken / allowed this turn */
    int ranged;                /* a projectile still to come */
    int angle;                 /* 0..255, 0 = right, 64 = down */
    int pips, charging, charge_t, still_t;
    int no_room;               /* the projectile can't fit that way */
    int round, turns[2];
    int fog;                   /* the haze's reach in from every edge */
    int winner;                /* -1 still on, 0/1, 2 both gone */
    int cam_x, cam_y, cam_free;
    int cpu[2];                /* -1 a player */
    int fog_deaths;
    int quiet, touch_mark;     /* turns in a row with nothing touched */
    int attacker;
    int nwall;                 /* the cliffs of mountain corners: line walls */
    float wall[16][4];
    int stun_pick;             /* the chosen disk was stunned when picked */
    int over_t;
    int proj;                  /* index of the projectile disk, or -1 */
    int skip_ranged;           /* last projectile was skipped (tests) */
    int launches;
    int stars_at_pick;         /* stars the chosen disk had when picked */
    /* the computer's plan */
    int ai_disk, ai_angle, ai_pips, ai_skip, ai_key, ai_ready;
} RshBattle;
extern RshBattle rb;

void rsh_battle_start(int attacker, uint32_t seed);
void rsh_battle_step(uint32_t held, uint32_t pressed, uint32_t released);
int rsh_terrain_at(float x, float y);
void rsh_phys_step(RshPhys *p);
int rsh_eligible(int side, int *out); /* choosable disks, in queue order */
void rsh_launch(RshPhys *p, int disk, int angle, int pips, int ranged);
bool rsh_proj_room(const RshPhys *p, int disk, int angle);
float rsh_pip_speed(int pips);
int rsh_disks_left(int side);
void rsh_fog_rect(int *x0, int *y0, int *x1, int *y1);
int rsh_in_fog(float x, float y);
int rsh_touches_fog(const RshDisk *d); /* any part of the disk in the haze */
/* the computer: a shot for this side's turn (bot = the player's demo) */
void rsh_ai_begin(int side, int bot);
int rsh_ai_work(int budget);   /* 1 when decided */
extern int rsh_ai_busy;
/* the buttons the computer (or the demo player) holds this frame; prev is
 * what it held last frame, so presses are fresh */
uint32_t rsh_battle_buttons(int side, int bot, uint32_t prev);
int rsh_aim_dir_for(int from, int to); /* the d-pad bits that turn the aim toward 'to' */
int rsh_count_trace(int angle, int pips, int16_t *xs, int16_t *ys, uint8_t *kinds, int max); /* the aim line */

/* ---- art (rimshire_art.c) -------------------------------------------------------- */
enum {
    RS_EMB0 = 0,               /* + kind: the emblem on each disk */
    RS_TOKEN0 = K_COUNT, RS_TOKEN1,
    RS_BASE, RS_CASTLE, RS_INN, RS_TOME, RS_SEAM, RS_CHEST,
    RS_COIN, RS_SHARD, RS_TONIC, RS_WELL, RS_PILE, RS_CLUSTER, RS_EMBER0, RS_EMBER1,
    RS_STAR, RS_SKULL, RS_BOLT, RS_DROP, RS_TREE, RS_SHARD2,
    RS_LORD0, RS_LORD1,        /* the two lords, for the title and cards */
    RS_COUNT
};
extern Sprite rsh_spr[RS_COUNT];
extern int rsh_art_bad;
void rsh_art_load(void);
void rsh_draw_disk(const RshDisk *d, int x, int y, int t);
void rsh_draw_disk_icon(int kind, int side, int cx, int cy, int t);
extern const uint8_t RSH_SIDE_COL[2][3]; /* rim, face, dark */

/* ---- audio ----------------------------------------------------------------------- */
extern int RSH_MUS_TITLE, RSH_MUS_MAP, RSH_MUS_BATTLE, RSH_MUS_BATTLE2, RSH_MUS_EMPRESS, RSH_MUS_END,
    RSH_MUS_WIN, RSH_MUS_LOSE, RSH_MUS_WAR_WON, RSH_MUS_WAR_LOST;
void rsh_audio_load(void);

/* ---- streak maps (rimshire_levels.c) --------------------------------------------- */
void rsh_make_streak_map(RshWar *w, Rng *r);

#endif
