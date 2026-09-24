/* GRUB SHIFT - shared declarations. Cartridge 02 of UFO 40. */
#ifndef GRUBSHIFT_H
#define GRUBSHIFT_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define GW 8
#define GH 6
#define SLOTS 7
#define OFFERS 8

/* species */
enum { SP_NONE = 0, SP_SPARK, SP_SHELL, SP_BURROW, SP_MOUND, SP_COUNT };
/* bug levels */
enum { LV_LARVA = 0, LV_ADULT, LV_QUEEN, LV_EGG };

/* tools ("chips") */
enum {
    CH_ROLL, CH_DASH, CH_STREAK, CH_SKIP, CH_LEAP, CH_VAULT, CH_WARP,
    CH_ZAP, CH_ARC, CH_BEAM, CH_TOSS, CH_LOB, CH_MORTAR, CH_PULSE, CH_DETONATE,
    CH_SEED, CH_RAISE, CH_RECHARGE, CH_REBOOT, CH_DEVOLVE,
    TOOL_COUNT,
    TOOL_NONE = 0xFF
};
enum { KIND_MOVE, KIND_ATTACK, KIND_SPECIAL };

typedef struct ChipInfo {
    const char *name;
    uint8_t kind;
    uint8_t cost;
    const char *desc;
} ChipInfo;
extern const ChipInfo CHIPS[TOOL_COUNT];

enum { ST_PLAYING = 0, ST_WON, ST_DEAD, ST_HATCHED, ST_MISSED };

typedef struct Board {
    uint8_t elev[GH][GW];
    uint8_t hole[GH][GW];
    uint8_t pods[GH][GW];
    uint8_t bsp[GH][GW]; /* species (0 = none) */
    uint8_t blv[GH][GW]; /* level */
    uint8_t bhp[GH][GW]; /* hit points */
    int8_t px, py;
    uint8_t chips[SLOTS];
    uint8_t spent[SLOTS];
    uint8_t shop[OFFERS];
    uint16_t energy;
    uint8_t day, days;
    uint16_t kills, quota;
    uint8_t species[3];
    uint8_t evolvers[2];
    uint8_t spawn_n, pods_n;
    uint8_t status;
    uint8_t best_combo; /* most kills from one action */
    uint8_t contract;
    uint8_t pad;
    Rng rng;
} Board;

/* Visual events produced by the rules, replayed by the renderer. */
enum { EV_MOVE, EV_HIT, EV_BOOM, EV_KILL, EV_SPARK, EV_PUSH, EV_STOMP, EV_HOLE, EV_POD, EV_EGG, EV_DIE,
       EV_RAISE, EV_BUY, EV_SHOT };
typedef struct Event {
    uint8_t type;
    int8_t x, y, x2, y2;
    uint8_t a;
} Event;
typedef struct Events {
    Event ev[160];
    int n;
} Events;

/* rules (grubshift_logic.c) */
void gs_new_contract(Board *b, int contract, uint64_t seed);
void gs_targets(const Board *b, int chip, uint8_t out[GH][GW]);
bool gs_apply(Board *b, int slot, int tx, int ty, Events *ev);
bool gs_buy(Board *b, int offer, int slot, Events *ev);
void gs_rest(Board *b, Events *ev);
int gs_count_bugs(const Board *b, int level);
bool gs_bot_turn(Board *b, Events *ev);  /* one greedy action; false when it wants to rest */
void gs_bot_contract(Board *b);          /* play the whole contract instantly */

/* art & audio */
enum {
    GS_TILLY1, GS_TILLY2, GS_TILLY_HOP, GS_TILLY_DEAD,
    GS_POD1, GS_POD2,
    GS_LARVA_SPARK, GS_LARVA_SHELL, GS_LARVA_BURROW, GS_LARVA_MOUND,
    GS_ADULT_SPARK, GS_ADULT_SHELL, GS_ADULT_BURROW, GS_ADULT_MOUND,
    GS_QUEEN_SPARK, GS_QUEEN_SHELL, GS_QUEEN_BURROW, GS_QUEEN_MOUND,
    GS_EGG, GS_EGG2, GS_CROWN,
    GS_ICON_MOVE, GS_ICON_ATTACK, GS_ICON_SPECIAL,
    GS_SPRITE_COUNT
};
extern Sprite gs_spr[GS_SPRITE_COUNT];
void gs_art_load(void);
void gs_audio_load(void);
extern int GS_MUS_SHIFT, GS_MUS_TITLE, GS_MUS_WIN, GS_MUS_LOSE, GS_MUS_NIGHT;

#endif
