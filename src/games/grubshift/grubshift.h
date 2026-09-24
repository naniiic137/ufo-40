/* GRUB SHIFT - shared declarations. Cartridge 02 of UFO 40.
 * A tribute to Bug Hunter (UFO 50 #2); see docs/games/02-grub-shift.md. */
#ifndef GRUBSHIFT_H
#define GRUBSHIFT_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define GW 6      /* the dome floor is six tiles wide */
#define GH 5      /* and five tiles deep */
#define SLOTS 7
#define OFFERS 8
#define QUOTA 30

/* Grubs come in three colours. Each colour grows into one of two species,
 * and which one alternates from contract to contract. */
enum { PAIR_GOLD = 0, PAIR_LEAF, PAIR_SKY, PAIR_COUNT };
enum {
    SP_NONE = 0,
    SP_SPARK,   /* gold: sprays sparks two tiles each way when popped      */
    SP_HIVE,    /* gold: keeps a drone; can't be hurt while it's beside it */
    SP_MOUND,   /* leaf: raises its own tile, lowers the planters around   */
    SP_SOUR,    /* leaf: turns fizz pods next to it sour                   */
    SP_SHELL,   /* sky:  one extra hit against attacks and blasts          */
    SP_BURROW,  /* sky:  leaves a sinkhole when popped                     */
    SP_DRONE,   /* a hivebug's drone (not a grub; doesn't count)           */
    SP_COUNT
};
/* bug levels */
enum { LV_LARVA = 0, LV_ADULT, LV_QUEEN, LV_EGG };

int gs_pair_of(int species);

/* tools ("chips") */
enum {
    /* moves */
    CH_ROLL, CH_DASH, CH_STREAK, CH_RUSH, CH_SKIP, CH_LEAP, CH_VAULT, CH_WARP, CH_PERCH, CH_DIVE, CH_HUSTLE,
    /* attacks */
    CH_ZAP, CH_ARC, CH_BEAM, CH_FLARE, CH_TOSS, CH_LOB, CH_MORTAR, CH_DETONATE, CH_QUAKE, CH_PULSE, CH_HAIL,
    CH_CRACK, CH_TRACK,
    /* specials */
    CH_SEED, CH_SHIFT, CH_OVERDRIVE, CH_GATHER, CH_RELOAD, CH_REFUEL, CH_BOOST, CH_DIG, CH_VOLATILE,
    CH_DEVOLVE, CH_SPRAY, CH_RESTOCK, CH_RECHARGE, CH_FLIP, CH_HOVER, CH_SIGHT, CH_SHOO,
    TOOL_COUNT,
    TOOL_NONE = 0xFF
};
enum { KIND_MOVE, KIND_ATTACK, KIND_SPECIAL };

typedef struct ChipInfo {
    const char *name;
    uint8_t kind;
    uint8_t cost;
    uint8_t two_step; /* needs two targets (SHIFT, SPRAY) */
    const char *desc;
} ChipInfo;
extern const ChipInfo CHIPS[TOOL_COUNT];

enum { ST_PLAYING = 0, ST_WON, ST_DEAD, ST_HATCHED, ST_MISSED };

/* day-long effects */
enum { FX_OVERDRIVE = 1, FX_BOOST = 2, FX_VOLATILE = 4, FX_HOVER = 8, FX_SIGHT = 16 };

typedef struct Board {
    uint8_t elev[GH][GW];
    uint8_t hole[GH][GW];
    uint8_t pods[GH][GW];  /* fizz pods (energy) */
    uint8_t sour[GH][GW];  /* sour pods (anti-energy) */
    uint8_t spray[GH][GW]; /* sprayed tiles: no grub spawns, a pushed grub dies */
    uint8_t bsp[GH][GW];   /* species (0 = none) */
    uint8_t blv[GH][GW];   /* level */
    uint8_t bhp[GH][GW];   /* hit points */
    int8_t px, py;
    uint8_t chips[SLOTS];
    uint8_t spent[SLOTS];
    uint8_t shop[OFFERS];
    uint16_t energy;
    uint8_t day, days;
    uint16_t kills, quota;
    uint8_t pair_sp[PAIR_COUNT]; /* the species each colour grows into this contract */
    uint8_t evolvers[2];         /* the two colours that grow this contract */
    uint8_t spawn_n, pods_n;
    uint8_t status;
    uint8_t contract;
    uint8_t fx;                  /* FX_* until the shift ends */
    uint8_t pad;
    Rng rng;
} Board;

/* Visual events produced by the rules, replayed by the renderer. */
enum { EV_MOVE, EV_HIT, EV_BOOM, EV_KILL, EV_SPARK, EV_PUSH, EV_STOMP, EV_HOLE, EV_POD, EV_EGG, EV_DIE,
       EV_RAISE, EV_BUY, EV_SHOT, EV_SOUR, EV_SPRAY, EV_DRONE, EV_GROW, EV_REFRESH };
typedef struct Event {
    uint8_t type;
    int8_t x, y, x2, y2;
    uint8_t a;
} Event;
typedef struct Events {
    Event ev[200];
    int n;
} Events;

/* rules (grubshift_logic.c) */
int gs_days_for(int contract);
void gs_new_contract(Board *b, int contract, int prev_first_species, uint64_t seed);
void gs_targets(const Board *b, int chip, uint8_t out[GH][GW]);
/* second target of a two-step tool, given the first */
void gs_targets2(const Board *b, int chip, int fx, int fy, uint8_t out[GH][GW]);
bool gs_apply(Board *b, int slot, int tx, int ty, int tx2, int ty2, Events *ev);
bool gs_buy(Board *b, int offer, int slot, Events *ev);
void gs_rest(Board *b, Events *ev);
int gs_count_bugs(const Board *b, int level);
bool gs_bot_turn(Board *b, Events *ev);  /* one greedy action; false when it wants to rest */
void gs_bot_contract(Board *b);          /* play the whole contract instantly */

/* art & audio */
enum {
    GS_TILLY1, GS_TILLY2, GS_TILLY_HOP, GS_TILLY_DEAD,
    GS_POD1, GS_POD2, GS_SOUR1, GS_SOUR2,
    GS_LARVA_GOLD, GS_LARVA_LEAF, GS_LARVA_SKY,
    GS_ADULT_SPARK, GS_ADULT_HIVE, GS_ADULT_MOUND, GS_ADULT_SOUR, GS_ADULT_SHELL, GS_ADULT_BURROW,
    GS_DRONE1, GS_DRONE2,
    GS_EGG, GS_EGG2, GS_CROWN,
    GS_ICON_MOVE, GS_ICON_ATTACK, GS_ICON_SPECIAL,
    GS_SPRITE_COUNT
};
extern Sprite gs_spr[GS_SPRITE_COUNT];
void gs_art_load(void);
void gs_audio_load(void);
extern int GS_MUS_SHIFT, GS_MUS_TITLE, GS_MUS_WIN, GS_MUS_LOSE, GS_MUS_NIGHT;

#endif
