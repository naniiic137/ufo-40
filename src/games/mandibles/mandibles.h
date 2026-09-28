/* MANDIBLES - shared declarations. Cartridge 46 of UFO 40.
 * A tribute to Combatants (UFO 50 #46); see docs/games/46-mandibles.md.
 * Symbol prefix: mnd_ / MND_ (tests: mnd_*.ufs). */
#ifndef MANDIBLES_H
#define MANDIBLES_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- controls, in one place --------------------------------------------
 * CONFIRMED (the owner read the original's own on-screen prompts, "hold Z:
 * command menu", "E: spit"): Button 1 held opens the command menu, Button 2
 * spits, the d-pad moves. On UFO 40 those are A, B and the d-pad. */
#define MND_BTN_ORDER BTN_A       /* hold: the command menu */
#define MND_BTN_SPIT BTN_B        /* spit (hold to keep spitting) */
#define MND_GLYPH_ORDER GLYPH_A
#define MND_GLYPH_SPIT GLYPH_B
/* UNCONFIRMED (our reading; no source shows the menu itself): the command
 * menu is a list, up/down move the cursor, letting go of A gives the order
 * under it, and a quick tap gives the last order again. The queen's
 * worker/soldier switch is the list's last line, there only while your queen
 * is inside your shout radius. Beads are picked up by walking over them and
 * handed over by touching your queen, with no button. */
#define MND_MENU_UP BTN_UP
#define MND_MENU_DOWN BTN_DOWN
enum { MI_FOLLOW = 0, MI_SOLDIER_FOLLOW, MI_HALT, MI_INSTINCT, MI_QUEEN, MI_COUNT };

/* ---- world ---------------------------------------------------------------- */
#define MND_TILE 8
#define MND_FP 16 /* sub-pixels per pixel */
#define MND_MAXW 100
#define MND_MAXH 64
#define MND_MAX_UNITS 180
#define MND_MAX_SHOTS 120
#define MND_MAX_BEADS 200
#define MND_MISSIONS 12      /* the campaign */
#define MND_BONUS 12         /* index of the hidden bonus mission */
#define MND_VS_MAPS 3
#define MND_ALL_MAPS (MND_MISSIONS + 1 + MND_VS_MAPS)

enum { MND_BLUE = 0, MND_RED = 1, MND_WILD = 2 };
enum { MK_NONE = 0, MK_PLAYER, MK_WORKER, MK_SOLDIER, MK_QUEEN, MK_SPIDER };
enum { ORD_INSTINCT = 0, ORD_FOLLOW, ORD_HOLD };
enum { PROD_WORKER = 0, PROD_SOLDIER = 1 };
enum { TL_GROUND = 0, TL_GRASS, TL_ROCK, TL_WATER, TL_PEBBLE, TL_ROOT };
enum { MND_PLAYING = 0, MND_WON, MND_LOST }; /* status, from blue's side (versus: WON = blue won) */

typedef struct MndUnit {
    uint8_t kind, side, order, carry;
    int8_t face;          /* 0..7: E SE S SW W NW N NE */
    int8_t lock;          /* red soldier: locked spit direction, -1 none */
    int8_t step_dir;
    uint8_t prod;         /* queen: PROD_* */
    int16_t hp, maxhp;
    int32_t x, y;         /* centre, sub-pixels */
    int16_t step, pause;  /* frames left in this step / in the pause after it */
    int16_t cool, hurt, bite, order_age, lost, anim;
    int16_t target;       /* unit index, -1 none */
    int16_t goal_bead;    /* bead a red ant is heading for, -1 none */
    int16_t ox, oy;       /* follow: where to stand, relative to the leader (px) */
    int16_t store, gest;  /* queen: beads in store, gestation left */
    uint8_t cycle, spawn_kind;
    int32_t memx, memy;   /* spider: last place a victim was seen */
    int16_t mem_t, wander_t;
    int16_t ack;          /* "!" over an ant that just took an order */
    uint16_t id;
} MndUnit;

/* The command menu of one player (0 blue, 1 red in versus). */
typedef struct MndMenu {
    bool open;        /* the order button is held */
    int8_t sel;       /* the line under the cursor (kept between openings) */
    int8_t flash;     /* the last order given, for the HUD */
    int16_t flash_t;
    int16_t open_t;
} MndMenu;

typedef struct MndShot {
    uint8_t on, side;
    int8_t dir;
    int16_t life;
    int32_t x, y;
} MndShot;

typedef struct MndBead {
    uint8_t on;
    int16_t x, y; /* pixel centre */
} MndBead;

/* What happened this frame (sounds, flashes, tests). */
typedef struct MndEvents {
    uint16_t spits, hits, deaths, melee, bites, pickups, deliveries, spawns, respawns, spider_slain;
    uint16_t blue_spider_kill, orders, menu_moves;
} MndEvents;

typedef struct MndWorld {
    int w, h;
    uint8_t tile[MND_MAXH][MND_MAXW];
    MndUnit u[MND_MAX_UNITS];
    int n_units;
    MndShot shot[MND_MAX_SHOTS];
    MndBead bead[MND_MAX_BEADS];
    int n_beads;
    int player[2];        /* unit index of each side's player ant, -1 when dead */
    int dead_t[2];        /* frames a side's player has been dead */
    MndMenu menu[2];
    uint8_t map;          /* which field (index into MND_MAPS) */
    uint8_t versus;
    uint8_t status;
    uint8_t red_mistake;  /* percent of red steps that go astray */
    uint8_t fire_held[2];
    int32_t frame;
    uint16_t next_id;
    /* totals for the result screen and the tests */
    uint16_t kills[2], losses[2], delivered[2], spawned[2], respawns[2], spiders_slain, blue_spider_kills;
    MndEvents ev;
    Rng rng;
} MndWorld;

/* One player's buttons for a frame. */
typedef struct MndPad {
    bool up, down, left, right;
    bool menu_up, menu_down;      /* pressed this frame (menu cursor) */
    bool order_held, order_pressed;
    bool spit_held;
} MndPad;

/* ---- maps ------------------------------------------------------------------ */
typedef struct MndMap {
    const char *name;
    const char *brief;          /* the general's briefing (campaign) */
    const char *const *rows;    /* the layout, see mandibles_maps.c */
    uint8_t blue_store, red_store;
    const char *red_prod;       /* the red queens' spawn cycle: W worker, S soldier */
    int16_t map_x, map_y;       /* node on the campaign map (px) */
    int8_t next[3];             /* missions this one opens (1-based, 0 none) */
} MndMap;
extern const MndMap MND_MAPS[MND_ALL_MAPS];

/* ---- rules (mandibles_logic.c) --------------------------------------------- */
extern MndWorld mnd_w;
void mnd_load_map(MndWorld *w, int map, uint64_t seed, bool versus);
void mnd_step(MndWorld *w, const MndPad pads[2]);
bool mnd_menu_line_ok(const MndWorld *w, int side, int line); /* the queen line needs her in earshot */
bool mnd_solid(const MndWorld *w, int px, int py);   /* pixel point blocks walking */
bool mnd_opaque(const MndWorld *w, int px, int py);  /* pixel point blocks sight and spit */
bool mnd_los(const MndWorld *w, int x0, int y0, int x1, int y1);
int mnd_octant(int dx, int dy);
int mnd_count(const MndWorld *w, int side, int kind); /* kind MK_NONE: every ant but spiders */
int mnd_queen_of(const MndWorld *w, int side);       /* first living queen, -1 */
int mnd_add_unit(MndWorld *w, int kind, int side, int px, int py);
void mnd_shout(MndWorld *w, int side, int order, bool soldiers_only);
bool mnd_toggle_queen(MndWorld *w, int side);
int mnd_beads_left(const MndWorld *w);
extern const int8_t MND_DX[8], MND_DY[8];
/* numbers the presentation and the bot share */
#define MND_SHOUT_R 40      /* order radius, px (five tiles) */
#define MND_SPIT_RANGE 56   /* how far a spit flies, px */
#define MND_RESPAWN_T 90
#define MND_STARVE_T 600    /* dead this long with an empty queen: the mission is lost */

/* ---- demo player (mandibles_bot.c) ---------------------------------------- */
void mnd_bot_reset(void);
int mnd_bot_buttons(const MndWorld *w, int side);
int mnd_bot_debug(int what); /* 0: its target, 1: which rule it followed last */

/* ---- art & audio ----------------------------------------------------------- */
enum {
    MS_SOL_E1, MS_SOL_E2, MS_SOL_D1, MS_SOL_D2, MS_SOL_S1, MS_SOL_S2, /* soldier: east, south-east, south */
    MS_WRK_E1, MS_WRK_E2, MS_WRK_D1, MS_WRK_D2, MS_WRK_S1, MS_WRK_S2, /* worker */
    MS_QUEEN1, MS_QUEEN2,
    MS_SPIDER1, MS_SPIDER2,
    MS_BEAD, MS_SPIT, MS_CROWN,
    MS_ROCK1, MS_ROCK2, MS_TUFT, MS_PEBBLE, MS_ROOT,
    MS_GENERAL, MS_SPRITE_COUNT
};
extern Sprite mnd_spr[MS_SPRITE_COUNT];
extern uint8_t MND_TEAM[3][PAL_COUNT]; /* palette maps: blue, red, the player's own ant */
void mnd_art_load(void);
void mnd_audio_load(void);
extern int MND_MUS_TITLE, MND_MUS_FIELD, MND_MUS_BATTLE, MND_MUS_MAP, MND_MUS_WIN, MND_MUS_LOSE, MND_MUS_CAPITAL;

#endif
