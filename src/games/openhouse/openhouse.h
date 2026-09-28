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
#define PH_POOL_MAX 48      /* room for the whole roster (a custom list may sell everyone) */
#define PH_POOL_MAX_V3 24   /* the pool size in saves before the custom list */
#define PH_POOL_MAX_V1 16   /* the pool size in saves before OPEN ALL NIGHT */
#define PH_SCENARIOS 5
#define PH_RANDOM 5         /* scenario index of the Random Scenario */
#define PH_ENDLESS 6        /* OPEN ALL NIGHT, the owner's endless mode (and its big mix) */
#define PH_CUSTOM 7         /* the custom list (the owner's): the player picks the shop */
#define PH_BASES 8          /* OPEN ALL NIGHT plays any of: five lists, Random, the big mix, the custom list */
#define PH_BASES_V3 7       /* the bests kept in saves before the custom list */
#define PH_CUSTOM_MIN 6     /* the custom list needs six guests besides the locked ones... */
#define PH_CUSTOM_MIN_STARS 1 /* ...and a star, or it can't be won */
#define PH_ENDLESS_STARS 4  /* its shop: four stars, fourteen other guests (plus neighbours, cousins) */
#define PH_ENDLESS_GUESTS 14
#define PH_ENDLESS_STRIKES 3 /* the third shutdown closes the house for good */
#define PH_ENDLESS_FRESH 5  /* every fifth night, three new faces in the shop */
#define PH_NONE 255
#define PH_POP_CAP 65       /* Party House caps fame at 65 ([V]) */
#define PH_CASH_CAP 30      /* and cash at $30 ([S]) */

/* the guests, in the order of the table in the design doc */
enum {
    G_NEIGHBOUR, G_COUSIN, G_ROWDY,
    G_CABBIE, G_SLEUTH, G_SURFER, G_KITTEN, G_BOUNCER, G_STRONGMAN, G_PARROT, G_DOORMAN,
    G_FIREWORKER, G_GUIDE, G_DRUMMER, G_SOCIALITE, G_IDOL, G_COATCHECK, G_STORYTELLER,
    G_PAPARAZZO, G_PASTRY, G_MERCHANT, G_GRANNY, G_BOOKWORM, G_TAILOR, G_BARISTA, G_POET,
    G_UPSTART, G_BANDLEADER, G_USHER, G_FORTUNE, G_MATCHMAKER, G_SAGE, G_MOONCHILD,
    G_GOAT, G_PUNK, G_SMUGGLER, G_CARDSHARK,
    G_PILOT, G_TYCOON, G_WISHFISH, G_SERPENT, G_CYCLOPS, G_PHOENIX, G_SHADOW, G_SPHINX, G_CHAMPION,
    /* the owner's own guests (not Party House's), after the original roster
     * so saved guest books keep their numbers */
    G_CROONER, G_ALBATROSS,
    G_COUNT
};
#define G_FIRST_BUYABLE G_CABBIE   /* the non-star guests a random pool picks from: CABBIE..CARDSHARK */
#define G_LAST_BUYABLE G_CARDSHARK /* (plus the owner's guests, where his table sells them) */
#define G_FIRST_STAR G_PILOT
#define G_LAST_STAR G_CHAMPION
#define G_FIRST_OWNER G_CROONER
#define PH_GUESTS_V2 46             /* the roster in saves made before the owner's guests */

/* actions (once a party) */
enum { A_NONE, A_FETCH, A_BOOT, A_PEEK, A_RESHUFFLE, A_PHOTO, A_STYLE, A_CHEER, A_GREET, A_MAGIC, A_CUPID, A_CALM, A_ENCORE };
/* passive traits */
enum {
    T_STAR = 1, T_TROUBLE = 2, T_PEACE = 4, T_BRING1 = 8, T_BRING2 = 16, T_DRUM = 32, T_STORY = 64,
    T_GRANNY = 128, T_BOOK = 256, T_BARISTA = 512, T_POET = 1024, T_UPSTART = 2048, T_MOON = 4096,
    T_JINX = 8192   /* the owner's: everyone who comes in after him tonight is RUCKUS! */
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
    uint8_t pool[PH_POOL_MAX_V3]; /* the guests the shop sells, besides neighbours and cousins */
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
    uint8_t jinx;                    /* an albatross came in tonight (this byte was padding,
                                        so the saved layout keeps its size) */
    int16_t peek;                    /* the guest waiting at the door, if someone looked; -1 */
    uint8_t over;                    /* PO_* */
    uint8_t overflow;                /* card + 1 who couldn't get in when the fire marshal came, 0 none
                                        (this byte was padding, so the saved layout keeps its size) */
    int16_t got_pop, got_cash;       /* collected during the party (paparazzo, usher) */
    int16_t end_pop, end_cash, penalty; /* the final tally */
    uint8_t warned;                  /* the neighbour's lamp went on */
    uint8_t nlast, last[PH_MAX_HOUSE]; /* who came in with the last knock */
} PhParty;

typedef struct PhGame {
    uint8_t scen;                    /* 0..4 set, PH_RANDOM, PH_ENDLESS, PH_CUSTOM */
    uint8_t npool;
    uint8_t pool[PH_POOL_MAX];       /* the shop: every guest on sale */
    uint8_t bought[G_COUNT];         /* shared stock in 2P */
    uint8_t players, turn;
    uint8_t winner;                  /* 0 none, 1 or 2 */
    uint8_t done;                    /* the scenario is over */
    Rng rng;
    PhPlayer pl[2];
    PhParty party;
    /* OPEN ALL NIGHT (unused by the other lists) */
    uint8_t goal;                    /* stars the next star party needs */
    uint8_t star_party;              /* the party that just ended was one */
    uint16_t score;                  /* star parties thrown */
    uint16_t nights;                 /* parties thrown (no last night, so past 255) */
    uint8_t strikes;                 /* shutdowns so far */
    uint16_t top_pop;                /* the most fame held at once this run */
    uint8_t base;                    /* the list it plays: 0..4, PH_RANDOM, PH_ENDLESS (the big mix) or PH_CUSTOM */
} PhGame;

/* setting up */
void ph_new(PhGame *g, int scen, int players, uint64_t seed);
void ph_new_endless(PhGame *g, int base, uint64_t seed); /* OPEN ALL NIGHT on list base */
/* the custom list (the owner's): the shop sells neighbours, cousins and the
 * n chosen guests; endless plays it as OPEN ALL NIGHT */
void ph_new_custom(PhGame *g, const uint8_t *types, int n, bool endless, int players, uint64_t seed);
bool ph_custom_locked(int type);         /* in every run, so always in a custom list: neighbour, cousin, rowdy mate */
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
bool ph_should_end(const PhGame *g);     /* full (or nobody left to come) and nothing left to do */
int ph_pay_order(const PhGame *g, int cash, uint8_t unpaid[PH_MAX_HOUSE]); /* who can't be paid; the penalty */
int ph_add_pop(int have, int v);         /* fame after pay: up to the cap (never taking away what's over it), never below 0 */
int ph_add_cash(int have, int v);        /* cash after pay, up to the cap */
int ph_night_limit(const PhGame *g);     /* the last night: 25 (OPEN ALL NIGHT has none) */
bool ph_endless_strike(PhGame *g);       /* OPEN ALL NIGHT: a shutdown counts; true when it closes the house */
int ph_goal(const PhGame *g);            /* stars the winning (or star) party needs */
bool ph_endless_refresh(PhGame *g);      /* OPEN ALL NIGHT's shop turns over */
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
bool ph_list_sells(int list, int type);  /* can the list's shop hold this guest (0..4, PH_RANDOM, PH_ENDLESS) */
static inline PhPlayer *ph_me(PhGame *g) { return &g->pl[g->turn]; }
static inline const PhPlayer *ph_me_c(const PhGame *g) { return &g->pl[g->turn]; }

#ifndef PH_NO_SHELL
/* art & audio */
enum { PS_DOOR = G_COUNT, PS_LAMP, PS_POLICE, PS_FIRE, PS_COUNT };
extern Sprite ph_spr[PS_COUNT];
void ph_art_load(void);
/* the icon set (the owner's): tiny badges drawn with a dark outline. The
 * first ones are the ability families, one per guest (ph_ability_icon). */
enum {
    PI_NONE, PI_FETCH, PI_BRING, PI_BOOT, PI_PEEK, PI_SHUFFLE, PI_SCORE, PI_STYLE, PI_REFRESH,
    PI_SWAP, PI_CALM, PI_FAMEUP, PI_CASHUP, PI_MOON, PI_CURSE, PI_ENCORE,
    PI_ABILITIES,                    /* the number of ability families (PI_NONE included) */
    PI_STAR = PI_ABILITIES, PI_FAME, PI_INF, PI_CROWN, PI_COUNT
};
extern const uint8_t PH_ICON_COL[PI_COUNT];  /* each icon's own colour */
extern const char *const PH_ICON_NAME[PI_ABILITIES]; /* the legend's words */
int ph_ability_icon(int type);       /* the guest's ability family, PI_NONE for none */
void ph_icon(int id, int x, int y, int col);  /* col < 0: the icon's own colour */
int ph_icon_w(int id);
void ph_audio_load(void);
extern int PH_MUS_TITLE, PH_MUS_PARTY, PH_MUS_SHOP, PH_MUS_WIN, PH_MUS_LOSE, PH_MUS_BUST, PH_MUS_NIGHT;
#endif

#endif
