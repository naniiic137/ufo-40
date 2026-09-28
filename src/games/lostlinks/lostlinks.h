/* LOST LINKS - shared declarations. Cartridge 18 of UFO 40, a tribute to
 * Golfaria (UFO 50 #18). See docs/games/18-lost-links.md.
 *
 * Dimple, a golf ball with a spark in it, wakes under the old links. Every
 * move is a stroke, and strokes are also Dimple's health: run out and it
 * wakes again at the last pin it touched. */
#ifndef LOSTLINKS_H
#define LOSTLINKS_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the world ------------------------------------------------------------ */

#define LNK_T 16                 /* a tile is 16 x 16 pixels */
#define LNK_ZW 20                /* a zone (one screen, about) is 20 x 12 tiles */
#define LNK_ZH 12
#define LNK_ZX 8                 /* 8 x 6 zones a layer */
#define LNK_ZY 6
#define LNK_MW (LNK_ZW * LNK_ZX) /* 160 x 72 tiles a layer */
#define LNK_MH (LNK_ZH * LNK_ZY)
enum { LNK_OVER, LNK_UNDER, LNK_LAYERS };

/* The two layers as drawn in lostlinks_world.c (made from the text files in
 * tools/lostlinks by mkworld.sh). Terrain:
 *   .  fairway (overworld) / cave floor (underground)
 *   ,  rough / moss: more grip        :  green: less grip
 *   s  sand: stops a ball dead, only a chip gets out (until the Dune Tread)
 *   ~  water: a ball that stops in it or lands in it sinks (until the Skipper)
 *   #  rock        T  tree        H  wall (buildings, the clubhouse)
 *   X  cracked block: the Hammerhead breaks it
 *   r  low rail: only an airborne ball (a chip or a hop) gets over it
 *   v  ledge: a ball may drop over it going south, never climb back north
 *   u  divot: a slow ball settles in it and must chip out
 *   o  hole: drops a slow ball to the other layer, at the same place
 *   O  a hole hidden in a patch of bushes (the same, drawn as a bush)
 *   1-9 (not 5)  slope, falling the way the digit points on a keypad
 *   b  bush: the ball crashes through it, slowed (it grows back next run)
 *   f  flowers (fairway)      '  cave crystals (floor)
 *   g  the altar gate: shut until the Star Pin is whole
 *   k  the den door: shut until the four sanctum plates are lit
 * Things (the tile under them is floor):
 *   W  where Dimple wakes     F  pin (checkpoint)     I  iron
 *   C  scorecrow              S  stray ball           N  somebody to talk to
 *   L  lark (hidden)          E  albatross            B  slicer
 *   h j d q  the Hammerhead, Backspin, Dune Tread, Skipper
 *   Q  a piece of the Star Pin   Z  the altar         P  a sanctum plate
 *   M  the odd tree           U  the crashed saucer   G  the Brass Badger's den
 */
extern const char *const LNK_MAP[LNK_LAYERS][LNK_MH];
extern const char *const LNK_ZONE_NAME[LNK_LAYERS][LNK_ZY][LNK_ZX];

/* ---- things in the world -------------------------------------------------- */

enum {
    EK_START, EK_PIN, EK_IRON, EK_CROW, EK_STRAY, EK_NPC, EK_LARK, EK_ALBA, EK_SLICER,
    EK_ABILITY, EK_PIECE, EK_ALTAR, EK_PLATE, EK_TREE, EK_SAUCER, EK_DEN, EK_KINDS
};
enum { AB_HAMMER, AB_BACKSPIN, AB_TREAD, AB_SKIPPER, AB_COUNT };
#define LNK_PINS 10
#define LNK_IRONS 20
#define LNK_CROWS 10
#define LNK_STRAYS 8
#define LNK_NPCS 11
#define LNK_PIECES 4
#define LNK_PLATES 4
#define LNK_MAX_THINGS 200

typedef struct LnkThing {
    uint8_t kind, layer, id; /* id counts things of a kind in map order (abilities: AB_*) */
    int16_t tx, ty;
} LnkThing;
extern LnkThing lnk_things[LNK_MAX_THINGS];
extern int lnk_nthings;
void lnk_world_scan(void);  /* parse the markers once */
const LnkThing *lnk_find(int kind, int id);
int lnk_count(int kind);
bool lnk_is_marker(char ch);
/* the ground as drawn, with the things lifted off it */
void lnk_tiles_reset(uint8_t (*t)[LNK_MH][LNK_MW]);

/* words (lostlinks_text.c) */
extern const char *const LNK_PIN_NAME[LNK_PINS];
extern const char *const LNK_NPC_NAME[LNK_NPCS];
extern const char *const LNK_NPC_LINE[LNK_NPCS][2];
extern const char *const LNK_STRAY_NAME[LNK_STRAYS];
extern const char *const LNK_STRAY_LINE[LNK_STRAYS];
extern const char *const LNK_CROW_TAPE[LNK_CROWS];
extern const char *const LNK_ABILITY_NAME[AB_COUNT];
extern const char *const LNK_ABILITY_LINE[AB_COUNT];
extern const char *const LNK_INTRO[];
extern const char *const LNK_ENDING[];
extern const char *const LNK_ENDING_ALL[];
extern const char *const LNK_TREE_LINE;
extern const char *const LNK_SAUCER_LINE;
extern const char *const LNK_ALTAR_LINE[3];

/* ---- the ball (lostlinks_ball.c) ------------------------------------------ */

#define LNK_R 3                  /* ball radius in pixels */
#define LNK_LEVELS 12            /* power levels on the meter */
#define LNK_DIRS 64              /* aim directions */
enum { LIE_GROUND, LIE_SAND, LIE_DIVOT, LIE_CUP };
enum { EV_NONE, EV_REST, EV_SINK, EV_HOLE, EV_BLOCK, EV_BOUNCE, EV_LAND };

typedef struct LnkBall {
    float x, y, vx, vy, z, vz;
    float sx, sy;                /* where this stroke was hit from */
    uint8_t layer, slayer, lie, slie;
    bool moving, hopped, lipped, braked;
    int slow_t, roll_t;
    int16_t hx, hy;              /* the hole it dropped into (EV_HOLE) */
    int16_t bx, by;              /* the block it broke (EV_BLOCK) */
    uint8_t nbroken;             /* planning runs: blocks broken so far (the map stays as it is) */
    uint8_t broken[6][3];        /* layer, x, y */
} LnkBall;

/* what the ball's owner can do, and the ground it rolls on */
typedef struct LnkCtx {
    uint8_t (*tiles)[LNK_MH][LNK_MW]; /* [layer][y][x] */
    uint8_t abilities;           /* 1 << AB_* */
    bool probe;                  /* a planning run: blocks are not broken for real */
    uint8_t opened;              /* 1 = the altar gate, 2 = the den door */
} LnkCtx;

char lnk_tile(const LnkCtx *c, int layer, int tx, int ty);
bool lnk_solid_char(char ch);
/* start a stroke: direction 0..63 (0 = east, clockwise), power 1..12 */
void lnk_hit(LnkBall *b, const LnkCtx *c, int dir, int level);
/* one 1/60 s step; returns an EV_* */
int lnk_ball_step(LnkBall *b, const LnkCtx *c);
/* the pad's hop or brake while rolling (Dune Tread / Backspin) */
bool lnk_hop(LnkBall *b, const LnkCtx *c);
bool lnk_brake(LnkBall *b, const LnkCtx *c);
bool lnk_on_sand(const LnkBall *b, const LnkCtx *c);
/* is this stroke a chip (sand, divot or the cup)? a free one (the cup)? */
bool lnk_is_chip(const LnkBall *b, const LnkCtx *c);
float lnk_dir_x(int dir);
float lnk_dir_y(int dir);
float lnk_roll_dist(int level);  /* flat-fairway distance of a roll, in pixels */
float lnk_chip_dist(int level);
/* slope pull of a tile char (x, y), 0 if flat */
void lnk_slope(char ch, float *ax, float *ay);

/* ---- probes and the demo player (lostlinks_probe.c) ----------------------- */

/* How far every tile is from a goal, walking the ground the way a ball can
 * go with these abilities (holes join the layers, a chip from sand or a cup
 * clears water and rails). Used to steer the demo player and to check the
 * world: what can be reached with which abilities. */
#define LNK_FAR 0xFFFF
typedef struct LnkFlow {
    uint16_t d[LNK_LAYERS][LNK_MH][LNK_MW];
} LnkFlow;
void lnk_flow_to(LnkFlow *f, const LnkCtx *c, int layer, int tx, int ty);
void lnk_flow_from(LnkFlow *f, const LnkCtx *c, int layer, int tx, int ty);

/* One stroke tried out in advance with the real rules (the creatures left
 * out). The best stroke towards a thing at (px, py) on a layer, steered by
 * the flow: returns false if nothing gets any nearer. */
typedef struct LnkShot {
    int dir, level;
    int hop_at, brake_at;    /* roll frame to press A (hop) / B (brake), -1 none */
    int score;
} LnkShot;
bool lnk_plan_shot(const LnkBall *b, const LnkCtx *c, const LnkFlow *f, int layer, float px, float py, float rad,
                   LnkShot *out);
/* run one stroke to its end; touched is set if it passes within rad of (px, py) */
int lnk_sim(LnkBall *b, const LnkCtx *c, int dir, int level, int hop_at, int brake_at, int layer, float px, float py,
            float rad, bool *touched);

/* ---- art and sound -------------------------------------------------------- */

enum {
    LS_BALL, LS_BALL_SHADOW, LS_IRON, LS_CROW1, LS_CROW2, LS_CROW_DEAD, LS_SLICER1, LS_SLICER2,
    LS_LARK1, LS_LARK2, LS_ALBA1, LS_ALBA2, LS_PIN, LS_PIN_LIT, LS_STRAY, LS_FOLK, LS_SAGE,
    LS_AB_HAMMER, LS_AB_BACKSPIN, LS_AB_TREAD, LS_AB_SKIPPER, LS_PIECE, LS_ALTAR, LS_PLATE, LS_PLATE_LIT,
    LS_SAUCER, LS_TREE_ODD, LS_BADGER_HEAD, LS_BADGER_STAND, LS_BADGER_DOWN, LS_CROSSHAIR, LS_DIMPLE_BIG,
    LS_COUNT
};
extern Sprite lnk_spr[LS_COUNT];
void lnk_art_load(void);
/* draw one tile of the world at screen (x, y) */
void lnk_draw_tile(int layer, char ch, int tx, int ty, int x, int y, int t, bool pan,
                   uint8_t (*tiles)[LNK_MH][LNK_MW]);

void lnk_audio_load(void);
extern int LNK_MUS_TITLE, LNK_MUS_LINKS, LNK_MUS_CAVES, LNK_MUS_SANDS, LNK_MUS_FEN, LNK_MUS_RUINS,
    LNK_MUS_BADGER, LNK_MUS_END, LNK_MUS_ITEM, LNK_MUS_OUT, LNK_MUS_TAPE;

#endif
