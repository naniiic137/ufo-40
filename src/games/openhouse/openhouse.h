/* OPEN HOUSE - shared declarations. Cartridge 25 of UFO 40.
 * A tribute to Party House (UFO 50 #25); see docs/games/25-open-house.md. */
#ifndef OPENHOUSE_H
#define OPENHOUSE_H

#ifndef PH_NO_SHELL
#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#else
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../engine/rng.h"
#endif

#define PH_MAX_CARDS 160
#define PH_MAX_HOUSE 34
#define PH_START_HOUSE 5
#define PH_ROW 8            /* guests stand in rows of eight; side by side means the same row */
#define PH_NIGHTS 25
#define PH_STOCK 4          /* each non-star guest can be bought four times a scenario */
#define PH_POOL_MAX 16
#define PH_SCENARIOS 5
#define PH_RANDOM 5         /* scenario index of the Random Scenario */
#define PH_NONE 255

/* the guests, in the order of the table in the design doc */
enum {
    G_NEIGHBOUR, G_COUSIN, G_ROWDY,
    G_CABBIE, G_SLEUTH, G_SURFER, G_KITTEN, G_BOUNCER, G_STRONGMAN, G_PARROT, G_DOORMAN,
    G_FIREWORKER, G_GUIDE, G_DRUMMER, G_SOCIALITE, G_IDOL, G_COATCHECK, G_STORYTELLER,
    G_PAPARAZZO, G_PASTRY, G_MERCHANT, G_GRANNY, G_BOOKWORM, G_TAILOR, G_BARISTA, G_POET,
    G_UPSTART, G_BANDLEADER, G_USHER, G_FORTUNE, G_MATCHMAKER, G_SAGE, G_MOONCHILD,
    G_GOAT, G_PUNK, G_SMUGGLER, G_CARDSHARK,
    G_PILOT, G_TYCOON, G_WISHFISH, G_SERPENT, G_CYCLOPS, G_PHOENIX, G_SHADOW, G_SPHINX, G_CHAMPION,
    G_COUNT
};
#define G_FIRST_BUYABLE G_CABBIE   /* the non-star guests a random pool picks from: CABBIE..CARDSHARK */
#define G_LAST_BUYABLE G_CARDSHARK
#define G_FIRST_STAR G_PILOT

/* actions (once a party) */
enum { A_NONE, A_FETCH, A_BOOT, A_PEEK, A_RESHUFFLE, A_PHOTO, A_STYLE, A_CHEER, A_GREET, A_MAGIC, A_CUPID, A_CALM };
/* passive traits */
enum {
    T_STAR = 1, T_TROUBLE = 2, T_PEACE = 4, T_BRING1 = 8, T_BRING2 = 16, T_DRUM = 32, T_STORY = 64,
    T_GRANNY = 128, T_BOOK = 256, T_BARISTA = 512, T_POET = 1024, T_UPSTART = 2048, T_MOON = 4096
};

typedef struct PhGuest {
    const char *name;
    int8_t cost, pop, cash;  /* cost < 0: can't be bought */
    uint8_t action;
    uint16_t traits;
    const char *does;        /* the rules, in our words */
    const char *flavour;     /* a line of our own */
} PhGuest;
extern const PhGuest PH_GUESTS[G_COUNT];

typedef struct PhScenario {
    const char *name, *blurb;
    uint8_t n;
    uint8_t pool[PH_POOL_MAX]; /* the guests the shop sells, besides neighbours and cousins */
} PhScenario;
extern const PhScenario PH_SCEN[PH_SCENARIOS];

typedef struct PhCard {
    uint8_t type;
    int8_t bonus;        /* the tailor's lasting +1s */
    uint8_t visits;      /* times it has come to a party: the upstart's value, the moon child's mood */
    uint8_t name;        /* 1+ : one of the secret real names */
} PhCard;

typedef struct PhPlayer {
    int16_t pop, cash;
    uint8_t cap, expansions;
    uint8_t night;       /* parties thrown so far */
    uint8_t banned;      /* card index + 1 kept out of the next party, 0 none */
    uint8_t won, lost;
    uint8_t ncards;
    PhCard card[PH_MAX_CARDS];
} PhPlayer;

enum { PO_RUNNING = 0, PO_ENDED, PO_POLICE, PO_FIRE, PO_WON };
enum { W_POOL = 0, W_HOUSE, W_OUT };

typedef struct PhParty {
    uint8_t n;
    uint8_t house[PH_MAX_HOUSE];     /* card indices, in the order they arrived */
    uint8_t where[PH_MAX_CARDS];     /* W_* */
    uint8_t used[PH_MAX_CARDS];      /* action spent this party */
    uint8_t calm[PH_MAX_CARDS];      /* the sage took its TROUBLE! away */
    uint8_t wild[PH_MAX_CARDS];      /* TROUBLE! this visit */
    int16_t peek;                    /* the guest waiting at the door, if someone looked; -1 */
    uint8_t over;                    /* PO_* */
    int16_t got_pop, got_cash;       /* collected during the party (paparazzo, usher) */
    int16_t end_pop, end_cash, penalty; /* the final tally */
    uint8_t warned;                  /* the neighbour's lamp went on */
    uint8_t nlast, last[PH_MAX_HOUSE]; /* who came in with the last knock */
} PhParty;

typedef struct PhGame {
    uint8_t scen;                    /* 0..4 set, PH_RANDOM */
    uint8_t npool;
    uint8_t pool[PH_POOL_MAX];       /* the shop: every guest on sale */
    uint8_t bought[G_COUNT];         /* shared stock in 2P */
    uint8_t players, turn;
    uint8_t winner;                  /* 0 none, 1 or 2 */
    uint8_t done;                    /* the scenario is over */
    Rng rng;
    PhPlayer pl[2];
    PhParty party;
} PhGame;

/* setting up */
void ph_new(PhGame *g, int scen, int players, uint64_t seed);
void ph_start_party(PhGame *g);
/* the party: each returns true if something happened */
int ph_draw(PhGame *g);                  /* a random card still in the rolodex, or -1 */
bool ph_open_door(PhGame *g);
bool ph_can_act(const PhGame *g, int slot);
bool ph_target_ok(const PhGame *g, int slot, int target);   /* for boot, photo, style, magic, cupid (left of a pair) */
bool ph_act(PhGame *g, int slot, int target);               /* target: a slot, or a guest type for a fetch */
bool ph_peek_decide(PhGame *g, bool admit);
bool ph_fetch_ok(const PhGame *g, int type);
void ph_end_party(PhGame *g);
bool ph_should_end(const PhGame *g);     /* full and nothing left to do */
void ph_ban(PhGame *g, int card);        /* after a shutdown */
void ph_next_turn(PhGame *g);            /* the next party (the other player in 2P) */
/* the shop */
int ph_expand_cost(const PhPlayer *p);
bool ph_can_buy(const PhGame *g, int type);
bool ph_buy(PhGame *g, int type);
bool ph_expand(PhGame *g);
/* reading the party */
int ph_trouble(const PhGame *g);         /* after peacemakers */
int ph_trouble_status(const PhGame *g);  /* every TROUBLE! guest (poet, barista) */
int ph_stars(const PhGame *g);
bool ph_is_wild(const PhGame *g, int card);
int ph_value_pop(const PhGame *g, int card);
int ph_value_cash(const PhGame *g, int card);
int ph_count_type(const PhGame *g, int type, int where);
static inline PhPlayer *ph_me(PhGame *g) { return &g->pl[g->turn]; }
static inline const PhPlayer *ph_me_c(const PhGame *g) { return &g->pl[g->turn]; }

#ifndef PH_NO_SHELL
/* art & audio */
enum { PS_DOOR = G_COUNT, PS_LAMP, PS_POLICE, PS_FIRE, PS_COUNT };
extern Sprite ph_spr[PS_COUNT];
void ph_art_load(void);
void ph_audio_load(void);
extern int PH_MUS_TITLE, PH_MUS_PARTY, PH_MUS_SHOP, PH_MUS_WIN, PH_MUS_LOSE, PH_MUS_BUST, PH_MUS_NIGHT;
#endif

#endif
