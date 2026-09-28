/* BOOMTOWN - shared declarations. Cartridge 10 of UFO 40, a tribute to
 * Devilition (UFO 50 #10). See docs/games/10-boomtown.md. */
#ifndef BOOMTOWN_H
#define BOOMTOWN_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define BM_W 10           /* the market square is 10 x 8 tiles */
#define BM_H 8
#define BM_ROUNDS 10
#define BM_ROW 3          /* pieces on offer at once */
#define BM_START_PIECES 15
#define BM_START_FOLK 2
#define BM_BOSS_HP 10
#define BM_CHAIN_STEP 14  /* frames between one firework going off and the next it lights */
#define BM_ROCKET_FLIGHT 34 /* frames a skyrocket takes to come down on its perch */

/* the fireworks (Devilition's pieces, our own names) */
enum {
    BM_NONE = 0,
    BM_STARBURST, /* tier 1: the 8 tiles around it            (bomb)     */
    BM_CANDLE,    /* tier 1: a straight line to the edge, turns (cannon)   */
    BM_ROCKET,    /* tier 1: flies up and lands on its perch    (rocket)   */
    BM_PERCH,     /* the rocket's landing post: 8 tiles round   (pad)      */
    BM_BANGER,    /* tier 2: the 4 tiles beside it              (plus)     */
    BM_PINWHEEL,  /* tier 2: the 4 tiles on its corners         (cross)    */
    BM_JACK,      /* tier 2: 2 tiles away, 4 ways               (toad)     */
    BM_TWIN,      /* tier 3: in front and behind, turns         (snake)    */
    BM_FOUNTAIN,  /* tier 3: the 3 tiles in front, turns        (strawman) */
    BM_KINDS
};

/* what stands on a tile */
enum { BO_EMPTY = 0, BO_HOLE, BO_FOLK, BO_BOGLE, BO_PIECE, BO_KING };
/* bogles (Devilition's demons) */
enum { BG_SMALL = 0, BG_BIG, BG_OLD };
/* facing: up, right, down, left */
enum { BD_UP = 0, BD_RIGHT, BD_DOWN, BD_LEFT };
/* round results */
enum { BR_HOLDS = 0, BR_CLEARED, BR_OVERRUN, BR_WON, BR_LOST };

typedef struct BmCell {
    uint8_t occ;    /* BO_* */
    uint8_t kind;   /* piece kind, or bogle kind */
    uint8_t dir;    /* facing of turning pieces */
    uint8_t hp;     /* a bogle's hits left */
    uint8_t col;    /* rocket / perch colour: 1 red, 2 green */
    uint8_t look;   /* art variant */
} BmCell;

typedef struct BmGame {
    BmCell c[BM_H][BM_W];
    uint8_t round;          /* 1..10 */
    int16_t hand;           /* pieces left to place, the three on offer included */
    uint8_t row[BM_ROW];    /* the three on offer (0 = empty) */
    uint8_t row_col[BM_ROW];/* a rocket's colour on offer */
    uint8_t bag[6];         /* tiers of the bag being dealt */
    uint8_t bag_i;          /* next in the bag (6 = a new bag) */
    uint8_t perch_owed;     /* a rocket's perch still to place (its colour) */
    uint8_t king_hp;
    uint8_t folk_most;      /* the most folk this run */
    Rng rng;
} BmGame;

/* one firework waiting to go off during a chain */
typedef struct BmFuse { int8_t x, y; uint8_t landing; uint16_t t; } BmFuse;

typedef struct BmChain {
    BmFuse q[128];
    int n;
    uint16_t now;
    int8_t lit[BM_H][BM_W];  /* already queued */
    int8_t hitmap[BM_H][BM_W]; /* every tile an attack reached */
    uint8_t fired, killed, folk_lost, king_hits, hits;
} BmChain;

/* what a chain did this frame, for the effects */
typedef struct BmFx {
    void (*fire)(int x, int y, int kind, int dir, int col);
    void (*hit)(int x, int y, int what, int dead); /* what: BO_* on the tile */
    void (*launch)(int x, int y, int col, int to_x, int to_y, int has_perch);
} BmFx;

/* rules (boomtown_logic.c) */
void bm_new_game(BmGame *g, uint64_t seed);
void bm_round_begin(BmGame *g);            /* next round: hole, bogles, pieces */
int bm_pieces_added(int round);
int bm_spawn_count(int round, int kind);
int bm_attack(const BmGame *g, int x, int y, int kind, int dir, int8_t out[][2]); /* tiles hit */
bool bm_turns(int kind);
int bm_tier(int kind);
bool bm_free(const BmGame *g, int x, int y);
bool bm_place(BmGame *g, int slot, int x, int y, int dir); /* true: its perch must follow */
bool bm_place_perch(BmGame *g, int x, int y);
void bm_fill_row(BmGame *g);
int bm_row_count(const BmGame *g);
void bm_chain_start(BmGame *g, BmChain *ch, int x, int y);
bool bm_chain_tick(BmGame *g, BmChain *ch, const BmFx *fx); /* false once it is over */
void bm_chain_run(BmGame *g, BmChain *ch);  /* the whole chain at once */
int bm_round_end(BmGame *g);               /* BR_* */
int bm_count(const BmGame *g, int occ);
int bm_pieces_on_board(const BmGame *g);
bool bm_colour_free(const BmGame *g, int col);
int bm_score(const BmGame *g, uint32_t frames, int *time_bonus);
int bm_time_bonus(uint32_t frames);

/* art and audio */
enum {
    BS_FOLK1, BS_FOLK2, BS_FOLK3, BS_FOLK4,
    BS_SMALL1, BS_SMALL2, BS_BIG1, BS_BIG2, BS_OLD1, BS_OLD2,
    BS_HAZEL1, BS_HAZEL2, BS_HAZEL_CHEER,
    BS_SPRITE_COUNT
};
extern Sprite bm_spr[BS_SPRITE_COUNT];
void bm_art_load(void);
bool bm_art_ok(void); /* tests: every sprite string is the right size */
/* a firework drawn into a 16 x 16 tile at (x,y) */
void bm_draw_piece(int x, int y, int kind, int dir, int col, int t);
void bm_audio_load(void);
extern int BM_MUS_TITLE, BM_MUS_NIGHT, BM_MUS_KING, BM_MUS_HOLD, BM_MUS_OVER, BM_MUS_END;

#endif
