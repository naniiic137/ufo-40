/* BANNERFALL - shared declarations. Cartridge 09 of UFO 40.
 * A tribute to Attactics (UFO 50 #9); see docs/games/09-bannerfall.md. */
#ifndef BANNERFALL_H
#define BANNERFALL_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define BF_ROWS 6
#define BF_COLS 8
#define BF_ZONE 4        /* columns each side can rearrange */
#define BF_POOL_MAX 12
#define BF_LEVELS 24
#define BF_NO_FLAGS 255  /* survival: the enemy keep has no flags */

enum { SIDE_L = 0, SIDE_R = 1 }; /* the player / player 1 is always the left army */

enum {
    U_NONE = 0,
    U_FOOT,   /* Footman: column guard            */
    U_BOW,    /* Bowman: shoots down its row      */
    U_WARD,   /* Warden: big shield, 5 HP         */
    U_RIDER,  /* Rider: moves twice               */
    U_PIKE,   /* Pikeman: hits two tiles ahead    */
    U_SHADE,  /* Shade: knives up and down        */
    U_POWDER, /* Powderman: explodes on death     */
    U_CHAMP,  /* Champion: pike + rider, promoted */
    U_TYPES
};

typedef struct UnitInfo {
    const char *name;
    uint8_t hp;
    const char *line1, *line2; /* two short description lines */
} UnitInfo;
extern const UnitInfo BF_UNITS[U_TYPES];

typedef struct Unit {
    uint8_t type, side, hp, promo;
    uint16_t id;
} Unit;

typedef struct Army {
    uint8_t flags;             /* flags left (BF_NO_FLAGS = none to lose) */
    uint8_t start_flags;
    uint8_t pool[BF_POOL_MAX];
    uint8_t pool_n;
    uint8_t heroes;            /* champions from promotions */
    uint8_t promos;            /* promotions toward the next champion (0..4) */
    uint8_t taken;             /* enemy flags taken: extra spawns */
    uint8_t hero_due;
    uint16_t handicap;         /* extra units, percent */
    uint16_t acc;              /* handicap running total */
} Army;

enum { BF_PLAYING = 0, BF_WIN_L, BF_WIN_R, BF_DRAW };
enum { MODE_CAMPAIGN = 0, MODE_RANKED, MODE_SURVIVAL, MODE_VERSUS };

typedef struct Board {
    Unit g[BF_ROWS][BF_COLS];
    Army a[2];
    uint16_t turn, next_id;
    uint8_t matched;
    uint8_t mode;
    uint8_t status;
    uint8_t pad;
    uint16_t keep_hits[2];     /* times each side marched into the enemy keep */
    uint32_t score;            /* survival */
    Rng rng;
} Board;

/* Events recorded by the rules and replayed by the renderer. */
enum {
    BE_SPAWN, BE_KNIFE, BE_ARROW, BE_MELEE, BE_REACH, BE_DAMAGE, BE_DEATH, BE_PROMOTE,
    BE_BLAST, BE_MOVE, BE_CLASH, BE_KEEP, BE_REAPPEAR, BE_BLOCK
};
enum { PH_KNIVES = 0, PH_ATTACK, PH_MOVE, PH_KEEP, PH_MOVE2, PH_COUNT };
typedef struct BEvent {
    uint8_t type, phase;
    int8_t x, y, x2, y2;
    uint8_t a, b;              /* damage / side / unit type, depending on type */
    uint16_t id;
} BEvent;
typedef struct BEvents {
    BEvent ev[400];
    int n;
} BEvents;

/* levels */
typedef struct LevelDef {
    const char *name;
    uint8_t flags_l, flags_r;
    const char *pool_l, *pool_r; /* unit letters: F B W R P S X C */
    uint16_t handicap;
    uint8_t heroes;              /* champions from promotions for the player */
    const char *tip;             /* shown when a battle brings in something new, or NULL */
} LevelDef;
extern const LevelDef BF_LEVELS_DEF[BF_LEVELS];

/* rules (bannerfall_logic.c) */
void bf_pool_from_letters(Army *a, const char *letters);
void bf_setup(Board *b, int mode, uint64_t seed);
void bf_setup_level(Board *b, int level, uint64_t seed);
void bf_setup_ranked(Board *b, int rank, uint64_t seed);
void bf_setup_survival(Board *b, uint64_t seed);
void bf_spawn(Board *b, BEvents *ev);
void bf_resolve(Board *b, BEvents *ev, Board snaps[PH_COUNT + 1]);
bool bf_in_zone(int side, int x);
/* a unit standing in an unbroken column of 3+ of its own side (the footman's guard) */
bool bf_in_column(const Board *b, int x, int y);
/* Drag a unit one tile; returns true and updates *x,*y when it moved. */
bool bf_drag(Board *b, int side, int *x, int *y, int dx, int dy, int pickup_col);
int bf_count(const Board *b, int side, int type); /* type U_NONE = all */
int bf_timer_counts(const Board *b);              /* 9, 6 or 4 */
int bf_rank_change(int flags_l, int flags_r, int status);
int bf_survival_stage(int turn);                  /* 0..4 colour shift */

/* art & audio */
enum {
    BS_FOOT1, BS_FOOT2, BS_BOW1, BS_BOW2, BS_WARD1, BS_WARD2, BS_RIDER1, BS_RIDER2,
    BS_PIKE1, BS_PIKE2, BS_SHADE1, BS_SHADE2, BS_POWDER1, BS_POWDER2, BS_CHAMP1, BS_CHAMP2,
    BS_WARDX1, BS_WARDX2, /* a warden whose shield has broken (3 HP or less) */
    BS_ARROW, BS_KNIFE, BS_STAR, BS_FLAG1, BS_FLAG2, BS_FLAG_DOWN, BS_TUFT, BS_FLOWER,
    BS_HAND, BS_HAND_GRAB,
    BS_SPRITE_COUNT
};
extern Sprite bf_spr[BS_SPRITE_COUNT];
extern uint8_t BF_TEAM_MAP[2][PAL_COUNT]; /* palette remaps: gold army / violet army */
void bf_art_load(void);
void bf_audio_load(void);
extern int BF_MUS_TITLE, BF_MUS_BATTLE, BF_MUS_WIN, BF_MUS_LOSE, BF_MUS_CAMPAIGN;

#endif
