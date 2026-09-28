/* WOBBLE DERBY - shared declarations. Cartridge 47 of UFO 40.
 * A tribute to Quibble Race (UFO 50 #47); see docs/games/47-wobble-derby.md. */
#ifndef WOBBLE_H
#define WOBBLE_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define WB_RACERS 26         /* the wobblers in the book */
#define WB_PLAYERS 3         /* three punters: you and two rivals */
#define WB_FIELD 3           /* three wobblers a race */
#define WB_PRERACES 20       /* races run before the game, for the form book */
#define WB_MAX_SPONSOR 3     /* wobblers one punter can sponsor */
#define WB_START_CASH 1000
#define WB_SPONSOR_BONUS 500 /* paid to a sponsor whenever its wobbler wins */
#define WB_TRAIN_COST 50
#define WB_TRAIN_GAIN 2      /* speed points a session adds for good */
#define WB_TRAINS_A_ROUND 2  /* sessions one punter can book a round */
#define WB_NOBBLE_LOSS 6     /* speed a nobble takes away for good */
#define WB_PEP_GAIN 10       /* speed a pep snack adds for one race */
#define WB_INTEREST 15       /* per cent a round, compound */
#define WB_NONE 255
#define WB_CHARS 16          /* punters to play as (looks only) */
#define WB_TRACK_PX 272      /* start line to finish line, in pixels */
#define WB_TRACK (WB_TRACK_PX * 256)
#define WB_TRIP_TICKS 60
#define WB_MAX_LITTER 10
#define WB_MAX_METEORS 6
#define WB_NEWS_MAX 12

/* how many races a game lasts (the setup screen's choices) */
#define WB_LENGTHS 4
extern const uint8_t WB_LENGTH[WB_LENGTHS];

/* a wobbler as written in the book (wobble_logic.c) */
typedef struct WbDef {
    const char *name;
    uint8_t stab_lo, stab_hi; /* clumsiness 1 (sure-footed) .. 10 (a disaster) */
    uint8_t spd_lo, spd_hi;   /* the speeds it can run a race at */
    uint8_t body, mark, pattern; /* colours and markings (wobble_art.c) */
} WbDef;
extern const WbDef WB_DEF[WB_RACERS];

/* a punter's face and name (looks only) */
typedef struct WbChar {
    const char *name;
    const char *from;         /* a line on the setup screen */
} WbChar;
extern const WbChar WB_CHAR[WB_CHARS];

/* the fixer's jobs */
enum { J_NONE, J_PEP, J_PEEL, J_NOBBLE, J_FIZZ, J_NIGHTSHADE, J_MINDER, J_COUNT };
extern const int WB_JOB_COST[J_COUNT];
extern const char *const WB_JOB_NAME[J_COUNT];

/* rare race-day events */
enum { EV_NONE, EV_METEORS, EV_SPILL, EV_SMOG };

/* why a wobbler left the book */
enum { OUT_NO, OUT_METEOR, OUT_COLLAPSED, OUT_RETIRED };

typedef struct WbRacer {
    int8_t mod;          /* speed gained (coach) or lost (nobbles, age) */
    uint8_t wins, races;
    uint8_t out;         /* OUT_* */
    uint8_t sponsor;     /* punter index + 1, 0 none */
    uint8_t trained;     /* booked at the coach this round */
} WbRacer;

typedef struct WbPlayer {
    uint8_t human, chr;
    int32_t cash, debt;
    uint8_t bet_on;      /* lane 0..2, WB_NONE for no bet */
    int32_t bet;
    uint8_t job, job_lane;
    uint8_t tips;        /* lanes looked up at the tip booth this round, a bit each */
    uint8_t trains;      /* coach sessions booked this round */
    int32_t won, bonus, fine, interest; /* this round, for the payout sheet */
    uint8_t sponsor_wins;/* times a wobbler it sponsors has won */
} WbPlayer;

typedef struct WbLane {
    int32_t x;           /* 1/256 px from the start line */
    int16_t form;        /* speed this race */
    uint8_t stab;        /* clumsiness this race */
    uint8_t pep, fizz, nobbled, minded;
    uint16_t poison_at;  /* tick it collapses, 0 none */
    int16_t trip;        /* ticks left on the ground */
    int16_t fizz_f;      /* the pills' current pace, per cent */
    uint8_t dead, done;
    uint16_t finish_t;
    uint8_t nlitter;
    uint16_t litter[WB_MAX_LITTER]; /* pixel positions of litter on this lane */
    uint16_t litter_hit;            /* a bit per piece already passed */
    uint8_t trips;
} WbLane;

typedef struct WbMeteor {
    uint8_t lane;
    int16_t x;           /* where it lands, px */
    uint16_t land;       /* tick it lands */
} WbMeteor;

typedef struct WbRace {
    uint8_t field[WB_FIELD]; /* racer index in each lane */
    uint8_t odds[WB_FIELD];  /* X : 1 */
    uint8_t event;
    WbLane lane[WB_FIELD];
    uint8_t nmet;
    WbMeteor met[WB_MAX_METEORS];
    uint16_t t;
    uint8_t winner;          /* lane, WB_NONE */
    uint8_t over;
    uint8_t started;
    uint16_t over_t;
} WbRace;

/* news items (the broadcast after each race) */
enum {
    NW_WIN, NW_NOWIN, NW_TRIPS, NW_METEOR_DEAD, NW_COLLAPSED, NW_FINE, NW_SPONSOR, NW_RETIRED,
    NW_EVENT, NW_SECRET, NW_PLACES
};
typedef struct WbNews { uint8_t kind, a, b; int32_t v; } WbNews;

typedef struct WbGame {
    uint8_t nraces, round;   /* round counts from 0 */
    uint8_t turn;            /* the punter whose turn it is */
    uint8_t done;
    uint8_t humans;
    WbPlayer pl[WB_PLAYERS];
    WbRacer r[WB_RACERS];
    WbRace race;
    uint8_t nnews;
    WbNews news[WB_NEWS_MAX];
    uint8_t secret;          /* someone bet the magic number this round */
    Rng rng;
} WbGame;

/* ---- rules (wobble_logic.c) ------------------------------------------- */
void wb_new(WbGame *g, int nraces, int humans, const uint8_t *chars, uint64_t seed);
void wb_start_round(WbGame *g);          /* deal the field, the odds and the weather */
int wb_cap(const WbGame *g);             /* the round's biggest bet, 0 = no limit */
int wb_cap_of(int round, int nraces);
bool wb_last_round(const WbGame *g);
int wb_tip_fee(const WbGame *g);
int wb_sponsor_fee(const WbGame *g, int racer);
int wb_loan_size(const WbGame *g);
int wb_spd_lo(const WbGame *g, int racer);
int wb_spd_hi(const WbGame *g, int racer);
bool wb_active(const WbGame *g, int racer); /* still racing */
int wb_sponsored_count(const WbGame *g, int p);
/* a punter's moves; each returns true if it was allowed */
bool wb_bet(WbGame *g, int p, int lane, int amount); /* amount 0 takes the bet back */
bool wb_tip(WbGame *g, int p, int lane);
bool wb_job(WbGame *g, int p, int job, int lane);
bool wb_borrow(WbGame *g, int p);
bool wb_repay(WbGame *g, int p);
bool wb_sponsor(WbGame *g, int p, int racer);
bool wb_train(WbGame *g, int p, int racer);
void wb_cpu_turn(WbGame *g, int p);
/* the race */
void wb_race_begin(WbGame *g);           /* applies the jobs; fines are taken here */
void wb_race_step(WbGame *g);            /* one tick (1/60 s) */
void wb_race_run(WbGame *g);             /* to the end at once */
void wb_race_settle(WbGame *g);          /* pay out, record, age; writes the news */
bool wb_next_round(WbGame *g);           /* false when that was the last race */
int wb_final(const WbGame *g, int p);    /* cash after the debt is taken */
int wb_rank(const WbGame *g, int p);     /* 1 = first (ties share) */
/* odds from a record */
int wb_odds_for(const WbGame *g, const uint8_t *field, int lane);
/* the demo player's pick (and the CPUs' judgement): win chance in 1/1000 */
void wb_estimate(const WbGame *g, int *pmil, bool know_stats, uint32_t seed);

/* ---- art & audio ------------------------------------------------------- */
enum {
    WA_RUN1, WA_RUN2, WA_TRIP, WA_DEAD, WA_IDLE, WA_BODY_COUNT,
    WA_PEEL = WA_BODY_COUNT, WA_CAN, WA_BOTTLE, WA_METEOR, WA_TRUCK, WA_STONE,
    WA_BOOTH, WA_ALLEY, WA_LENDER, WA_STABLE, WA_COACH, WA_ANCHOR,
    WA_PEP, WA_NIGHTSHADE, WA_FIZZ, WA_NOBBLE, WA_MINDER,
    WA_COUNT
};
extern Sprite wb_spr[WA_COUNT];
extern Sprite wb_face[WB_CHARS];
void wb_art_load(void);
bool wb_art_ok(void);          /* every drawing had the right size */
/* a wobbler: frame WA_RUN1..WA_IDLE, x,y top left */
void wb_draw_racer(int racer, int frame, int x, int y, int flags);
void wb_draw_face(int chr, int x, int y, int scale);
int wb_body_col(int racer);
void wb_audio_load(void);
extern int WB_MUS_TITLE, WB_MUS_PADDOCK, WB_MUS_RACE, WB_MUS_NEWS, WB_MUS_FINAL, WB_MUS_WIN, WB_MUS_LOSE,
    WB_MUS_BELL;

#endif
