/* CLARION CALL - shared declarations. Cartridge 35 of UFO 40.
 * A tribute to Campanella 2 (UFO 50 #35); see docs/games/35-clarion-call.md.
 *
 * Ansel flew the Tinkler to the Carillon, the great bell-tower station, and
 * went quiet. His sister Clary flies the Clarion after him: nine areas in
 * four of the station's seven regions, all generated but the last, with the
 * ship flying on CHIME CIRCUIT's flight model (../chime/chime_flight.c) and
 * Clary on foot through the doors.
 *
 *   clc_gen.c     the generators: every area's map, the caves and the rooms,
 *                 the fixed Crown and the escape shaft
 *   clc_world.c   the rules of a map: the ship, landing, fuel and coins,
 *                 the slash and the ship's gear, Clary on foot, notes, the
 *                 gold-door dash, every map enemy
 *   clc_sub.c     the rules behind a door: caves, rooms, shops, the Lobber,
 *                 the Hush Engines, Lady Hush and Grandsire Tock
 *   clc_items.c   the gear and what it costs; the walker both maps share
 *   clc.c         the screens, the run, the station map, saving, goals,
 *                 the code screen and the test hooks
 *   clc_bot.c     the demo player: a planner that plays the real rules
 *   clc_draw.c    drawing  clc_art.c sprites  clc_audio.c music and sounds */
#ifndef CLC_H
#define CLC_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include "../chime/chime_flight.h"

/* ---- the station ------------------------------------------------------ */
enum { RG_CELLARS, RG_ARBOR, RG_ICE, RG_GULLET, RG_COG, RG_CLOISTER, RG_SPIRE, RG_COUNT };
extern const char *const CLC_REGION_NAME[RG_COUNT];
/* the stage of the run a region belongs to: 0 the Cellars, 1 the Arboretum
 * or the Icehouse, 2 the Gullet, Cogtown or the Cloister, 3 the Spire */
int clc_region_tier(int region);

/* ---- numbers (frames at 60 a second, 1/256 px) ------------------------- */
#define CLC_HP_START 16        /* half points: 8 on the bar */
#define CLC_FUEL_START 800
#define CLC_NOTES_NEEDED 10
#define CLC_NOTES_PLACED 14
#define CLC_TIMER 1800         /* the dash: "60" counting double = 30 s */
#define CLC_RING_PAUSE 120     /* a clock ring stops the dash this long */
#define CLC_ESCAPE_T 5940      /* 99 s up the escape shaft */
#define CLC_BURN_FRAMES 1      /* frames of thrust that burn one unit of fuel */
#define CLC_COIN_BURN 6        /* with the tank dry, a coin for this many frames of thrust */
#define CLC_SHIP_INV 50
#define CLC_FOOT_INV 60
#define CLC_WALL_HURT 256      /* the ship hits a wall faster than this: 1 point */
#define CLC_LAND_VY 192        /* a landing slower than this (and level) is safe ... */
#define CLC_LAND_VX 160
#define CLC_BAD_LAND 320       /* ... faster than this down is a bad landing */
#define CLC_FALL_DEATH 12      /* on foot outside: a drop of more than a tile and a half kills */

/* ---- gear: the sixteen upgrades (bits of ClcPlayer.gear) --------------- */
enum {
    G_TWIN, G_SEEKER, G_BOUNCE, G_SIPHON, G_SPIT,          /* the ship */
    G_FEATHER, G_FAN, G_GHOST, G_CREEPER, G_BIGBANG, G_THIMBLE, /* Clary */
    G_CHARM, G_MAGNET, G_DOWSER,                            /* the maps */
    G_PURSE, G_PLATE,                                       /* both */
    G_COUNT
};
/* every item a pickup, a chest or a shop can hold */
enum {
    IT_HEARTPIN = G_COUNT, IT_SPARETANK, IT_TOFFEE, IT_FLASK, IT_DRUM, IT_SACK, IT_KEY, IT_SHEET, IT_COUNT
};
extern const int16_t CLC_PRICE[IT_COUNT];
extern const char *const CLC_ITEM_NAME[IT_COUNT]; /* for the docs, the tests and the credits: the game shows icons */

typedef struct ClcPlayer {
    int16_t hp, hpmax;         /* half points */
    int16_t fuel, fuelmax;
    int32_t coins;
    uint32_t gear;             /* 1 << G_* */
    uint8_t key;               /* the yellow key, one door's worth */
    uint8_t burn;              /* thrust frames toward the next coin burnt */
    uint8_t dead;
    uint8_t drip;              /* thrust frames toward the next unit of fuel */
} ClcPlayer;
bool clc_has(const ClcPlayer *p, int g);
/* damage in half points (armour halves it); returns true if it landed */
void clc_hurt(ClcPlayer *p, int half);
void clc_heal(ClcPlayer *p, int half);
void clc_add_fuel(ClcPlayer *p, int n);
/* coins from a kill or a block, with the purse's half again (rounded up) */
int clc_coin_drop(const ClcPlayer *p, int n);
/* give an item to the player (bought, found or chosen) */
void clc_give(ClcPlayer *p, int item);

/* ---- the walker: Clary on her feet, on a map (scale 1) or behind a door
 * (scale 2, the closer view) ------------------------------------------- */
typedef bool (*ClcSolidFn)(const void *ctx, int px, int py, bool feet); /* feet: thin floors count */
typedef bool (*ClcLadderFn)(const void *ctx, int px, int py);
typedef struct ClcWalker {
    int32_t x, y, vx, vy;      /* x the middle, y the feet */
    int8_t face;
    uint8_t ground, ladder, crouch, jumping, drop_t;
    uint8_t aim;               /* 0 ahead, 1 up, 2 down */
    int32_t fall_from;         /* the feet's height when she last stood */
    uint8_t shot_cd, inv;
    uint8_t slip;              /* ice */
    uint8_t landed;            /* set the frame she lands: the drop, in px */
    int16_t drop;
} ClcWalker;
typedef struct ClcWalkTune {
    int hw, h;                 /* half width, height (px) */
    int32_t walk, accel, gravity, jump, jump_cut, max_fall, climb;
} ClcWalkTune;
extern const ClcWalkTune CLC_WALK_MAP, CLC_WALK_SUB;
enum { WK_FEATHER = 1, WK_ICE = 2 };
/* one frame of Clary: buttons held and pressed (BTN_*), flags WK_* */
void clc_walk(ClcWalker *w, const ClcWalkTune *t, unsigned held, unsigned pressed, unsigned flags,
              ClcSolidFn solid, ClcLadderFn ladder, const void *ctx);
bool clc_walker_blocked(const ClcWalkTune *t, ClcSolidFn solid, const void *ctx, int32_t x, int32_t y);

/* ---- entities ---------------------------------------------------------- */
enum {
    EK_NONE,
    /* map enemies */
    EK_FLITTER, EK_CREEPER, EK_GRUB, EK_SNAP, EK_FIREDRONE, EK_WINDDRONE, EK_LEECH, EK_EYE, EK_ACID,
    EK_BLOB, EK_SENTRY, EK_SNAKE, EK_CRAB, EK_GHOST, EK_JETFIRE, EK_BOOMER, EK_WORM, EK_CHASER,
    EK_BIGBELLY, EK_POD, EK_LATE, EK_SEG,
    /* map things */
    EK_NOTE, EK_FAKENOTE, EK_PLUM, EK_COIN, EK_RING, EK_FUELCAN, EK_ARROW,
    /* behind doors: enemies */
    EK_STINGER, EK_DROPPER, EK_LOUSE, EK_JET, EK_BRUTE, EK_RINGWORM, EK_GOOPER, EK_AXER, EK_AIRBOT,
    EK_TROOPER, EK_COCOON, EK_BEE, EK_SPEWER, EK_WHEEL, EK_GHORBNEST, EK_GHORB, EK_SKULL, EK_GWORM,
    EK_FACE, EK_FLAME,
    /* behind doors: things */
    EK_BARREL, EK_CHEST, EK_ITEM, EK_NPC, EK_SWITCH,
    /* bosses and their parts */
    EK_LOBBER, EK_BOMB, EK_ENGINE, EK_HUSH, EK_HUSHGOOP, EK_TOCK, EK_TOCKHEAD, EK_MISSILE,
    EK_COUNT
};
typedef struct ClcEnt {
    uint8_t kind, on;
    int16_t hp;
    int32_t x, y, vx, vy;      /* middle, 1/256 px */
    int16_t hx, hy;            /* home, px */
    int16_t a, b, c;           /* per kind */
    uint16_t t;
    int8_t dir;
    uint8_t flag;              /* per kind */
    uint8_t hitno;             /* the last slash that struck it */
    uint8_t link;              /* a segment's leader */
} ClcEnt;

/* shots, both sides */
enum {
    SH_PISTOL, SH_FAN, SH_BIG, SH_FRAG, SH_SEEKER, SH_BEAM, SH_SPIT,  /* Clary's and the ship's */
    SH_PELLET, SH_FIREBALL, SH_ACID, SH_BOOMERANG, SH_RING, SH_ROCK, SH_BOMBLET, SH_ENERGY, SH_FLAMELET
};
#define SH_FOE SH_PELLET
typedef struct ClcShot {
    int32_t x, y, vx, vy;
    uint8_t kind, on, dmg, bounces;
    uint16_t life;
    int16_t target;
} ClcShot;

typedef struct ClcFx { int16_t x, y; uint8_t kind; } ClcFx;
enum { FX_BOOM = 1, FX_POP, FX_SPARK, FX_COINS, FX_DUST, FX_FUEL, FX_HIT, FX_NOTE };

/* news from a step, for the sounds and the run */
enum {
    CEV_SLASH = 1 << 0, CEV_HIT = 1 << 1, CEV_KILL = 1 << 2, CEV_HURT = 1 << 3, CEV_DIE = 1 << 4,
    CEV_NOTE = 1 << 5, CEV_COIN = 1 << 6, CEV_FUEL = 1 << 7, CEV_LAND = 1 << 8, CEV_BADLAND = 1 << 9,
    CEV_BUMP = 1 << 10, CEV_SHOOT = 1 << 11, CEV_DOOR = 1 << 12, CEV_BOARD = 1 << 13, CEV_TIMER = 1 << 14,
    CEV_RING = 1 << 15, CEV_LATE = 1 << 16, CEV_BREAK = 1 << 17, CEV_BLAST = 1 << 18, CEV_BUY = 1 << 19,
    CEV_ITEM = 1 << 20, CEV_BOSS_HIT = 1 << 21, CEV_BOSS_DOWN = 1 << 22, CEV_EXIT = 1 << 23, CEV_TALK = 1 << 24,
    CEV_DRY = 1 << 25, CEV_CLANG = 1 << 26, CEV_FOESHOT = 1 << 27, CEV_OPEN = 1 << 28, CEV_ESCAPED = 1 << 29
};

/* ---- the maps ----------------------------------------------------------- */
#define CLC_T 8                /* a map tile */
#define CLC_MW 128
#define CLC_MH 64
#define CLC_CW 16              /* the generator's cells */
#define CLC_CH 12
#define CLC_GW 8
#define CLC_GH 5
#define CLC_ENTS 200
#define CLC_SHOTS 96
#define CLC_DOORS 16
#define CLC_FX 16

enum { MT_AIR, MT_ROCK, MT_ROCK2, MT_COIN, MT_LADDER, MT_MAGNET_PULL, MT_MAGNET_PUSH, MT_GATE, MT_GATE_OPEN, MT_FLAME };
bool clc_map_solid_tile(int t);

/* doors: how they look, and what is behind them */
enum { DT_RED, DT_GREEN, DT_BLUE, DT_YELLOW, DT_GOLD, DT_SECRET };
enum {
    RM_CAVE, RM_GOLDCAVE, RM_NPC, RM_TRIAL, RM_CURSED, RM_HEALTH, RM_SHOP, RM_MEGA, RM_CHARMSHOP,
    RM_SAGE, RM_EMPTY, RM_LIGHTS, RM_MAGNETS, RM_BOON, RM_ARMOR, RM_SHORTCUT, RM_HUSH, RM_TOCK, RM_COUNT
};
typedef struct ClcDoor {
    uint8_t type, room;
    uint8_t used;              /* the free fuel given, the chest opened, the trial won ... */
    int8_t item;               /* a chest's item (-1: decided when opened) */
    int16_t c, r;              /* the door's lower tile */
    int16_t pad_c;             /* the ship lands with its middle here (tile column) */
    uint8_t hidden;            /* the gold door until ten notes; the secret door */
    uint8_t seed;
} ClcDoor;

enum { WK_GEN, WK_CROWN, WK_ESCAPE };
enum { SM_PILOT, SM_PARKED, SM_GONE };

typedef struct ClcWorld {
    uint8_t region, area, kind;
    uint8_t w, h;
    uint8_t tile[CLC_MH][CLC_MW];
    uint16_t ver;
    ClcDoor door[CLC_DOORS];
    int nd;
    int16_t spawn_c, spawn_r;  /* the ship's first pad: its middle's column, the floor row under it */
    int16_t exit_y;            /* the escape shaft's top (px) */
    /* the ship */
    ChmFlight f;
    uint8_t ship;              /* SM_* */
    uint8_t ship_inv, slash_t, slash_cd, slash_no, slash_side;
    uint8_t leeches, landed, crashed, dive;
    uint8_t air_t;             /* frames off the ground (a ship must be up before it can land) */
    int16_t landing_icon;      /* the floor row under the ship that would take a landing (-1 none) */
    /* Clary */
    ClcWalker cl;
    uint8_t on_foot;
    uint8_t charm_jump;        /* a mid-air jump calls the ship */
    /* notes and the dash */
    uint8_t notes, timer_on, late_on;
    int16_t timer, ring_pause;
    uint8_t gold_door;         /* its index */
    uint8_t dark, magnets_off, sentries_off;
    ClcEnt e[CLC_ENTS];
    int ne;
    ClcShot shot[CLC_SHOTS];
    int16_t ring_c[10], ring_r[10];
    uint8_t nrings;
    /* this frame's news */
    uint32_t ev;
    int8_t enter;              /* a door entered this frame (-1 none) */
    ClcFx fx[CLC_FX];
    uint8_t nfx;
    uint8_t prev;              /* last frame's buttons */
    uint8_t sim;               /* a look-ahead copy: no lasting changes outside it */
    uint8_t kills;
    uint8_t hurt_by;           /* what hurt the player last (a kind, or 100 + a shot kind) */
    uint32_t t;
    Rng rng;
} ClcWorld;

/* the generators (clc_gen.c) */
typedef struct ClcAreaSpec {
    uint8_t region, area;
    uint8_t sage;              /* a sage's door goes in */
    uint8_t final_sage;        /* this is the Spire's sage */
    uint8_t secret;            /* the Crown's secret door shows */
    uint32_t seed;
} ClcAreaSpec;
void clc_gen_world(ClcWorld *w, const ClcAreaSpec *spec);
void clc_gen_crown(ClcWorld *w, bool secret);
void clc_gen_escape(ClcWorld *w);
/* every area's checks: reachable notes, doors and pads; 0 = sound */
int clc_check_world(const ClcWorld *w, char *why, int n);
/* a ship cell (its middle at tile corner c, r) with room for the ship */
bool clc_ship_cell_free(const ClcWorld *w, int c, int r);
/* the door's floor row and the pad (the ship's middle) in px */
void clc_door_pad(const ClcDoor *d, int *x, int *y);

/* the rules of a map (clc_world.c) */
void clc_world_start(ClcWorld *w, ClcPlayer *p, bool on_foot);
void clc_world_step(ClcWorld *w, ClcPlayer *p, unsigned buttons);
bool clc_map_solid(const void *ctx, int px, int py);
int clc_map_tile(const ClcWorld *w, int px, int py);
void clc_world_return(ClcWorld *w, int door); /* back out of a door, on foot */
void clc_world_notes_done(ClcWorld *w, ClcPlayer *p); /* tests: the tenth note */
int clc_ent_add(ClcWorld *w, int kind, int x, int y);
void clc_set_tile(ClcWorld *w, int c, int r, int t);
int clc_isqrt(int v);
int clc_sin(int a);
int clc_cos(int a);
void clc_aim(int32_t x, int32_t y, int32_t tx, int32_t ty, int32_t speed, int32_t *vx, int32_t *vy);
extern const ChmFlightTune CLC_TUNE;

/* ---- behind the doors (closer view) ---------------------------------------- */
#define CLC_ST 16              /* a sub tile */
#define CLC_SW 152
#define CLC_SH 10
#define CLC_SOY 20             /* the play area's top on screen (the HUD is above) */
#define CLC_SENTS 96
#define CLC_SSHOTS 64
enum { ST_AIR, ST_WALL, ST_WALL2, ST_THIN, ST_BLOCK, ST_ACID, ST_ICE, ST_SPIKE, ST_DOOR, ST_EXIT };
bool clc_sub_solid_tile(int t, bool feet);

typedef struct ClcSub {
    uint8_t room, region, area;
    uint8_t w;                 /* tiles */
    uint8_t tile[CLC_SH][CLC_SW];
    uint16_t ver;
    ClcWalker cl;
    int32_t cam;               /* the view's left edge (1/256 px) */
    int16_t door_c, exit_c;    /* the way in, the way out (tile columns; -1 none) */
    uint8_t exit_open, locked;
    uint8_t arena;             /* the camera stops at the arena */
    int16_t arena_c;
    uint8_t bought;            /* one purchase a visit */
    uint8_t chest_item;
    uint8_t done;              /* the trial won, the boss down ... */
    uint8_t boss_kind;
    int16_t boss_hp, boss_max;
    uint8_t phase;
    uint16_t phase_t;
    uint8_t lit_switch;
    uint8_t talk;              /* an NPC's line is up */
    int8_t line;
    uint8_t npc_shots;         /* shots into the dozing tortoise */
    uint8_t mega;              /* the shop is the mega store */
    uint8_t ice;
    uint8_t leave;             /* set when Clary goes out: 1 back, 2 on (next area) */
    uint8_t picked;            /* the boon: chosen */
    ClcEnt e[CLC_SENTS];
    int ne;
    ClcShot shot[CLC_SSHOTS];
    uint32_t ev;
    ClcFx fx[CLC_FX];
    uint8_t nfx;
    uint8_t prev, sim;
    int16_t got_item;          /* an item taken this frame (-1) */
    int16_t gave_fuel;
    uint32_t t;
    Rng rng;
} ClcSub;

typedef struct ClcSubSpec {
    uint8_t room, region, area;
    int8_t item;               /* the chest's item; -1 for the region's draw */
    uint8_t lobber;            /* the gold cave ends at the Lobber and an engine */
    uint8_t engine;
    uint8_t used;              /* the NPC has given its fuel already */
    int8_t hint;               /* the NPC's line, or the sage's region */
    uint8_t charm_price;
    uint32_t seed;
    uint32_t owned;            /* gear already had (shops skip it) */
} ClcSubSpec;
void clc_gen_sub(ClcSub *s, const ClcSubSpec *spec);
/* the red cave's chest in a region's area (0 or 1): an item, or -1 for a draw */
int clc_chest_item(int region, int area);
int clc_pieces_bad(void);
void clc_sub_step(ClcSub *s, ClcPlayer *p, unsigned buttons);
bool clc_sub_solid(const void *ctx, int px, int py, bool feet);
int clc_sub_ent_add(ClcSub *s, int kind, int x, int y);
void clc_sub_boss_down(ClcSub *s); /* tests: beat the boss now */
int clc_sub_tile(const ClcSub *s, int px, int py);
extern const char *const CLC_NPC_LINE[];
extern const int CLC_NPC_LINES;

/* ---- the demo player (clc_bot.c) ---------------------------------------- */
enum { BOT_GOLD, BOT_CHERRY, BOT_BAD };
typedef struct ClcBotView {
    /* what the run tells the bot */
    int mode;                  /* 0 map, 1 behind a door */
    ClcWorld *w;
    ClcSub *s;
    ClcPlayer *p;
    int goal;                  /* BOT_* */
    int chain;                 /* the region the next sage waits in (-1 none) */
    int scrolls;
    bool want_charm;
} ClcBotView;
void clc_bot_reset(void);
unsigned clc_bot(const ClcBotView *v);
/* which door the bot is heading for, for the tests (-1) */
int clc_bot_target(void);
extern int clc_bot_debug;

/* ---- drawing, art and sound ---------------------------------------------- */
void clc_art_load(void);
bool clc_art_ok(void);
void clc_draw_world(const ClcWorld *w, const ClcPlayer *p, int t);
void clc_draw_sub(const ClcSub *s, const ClcPlayer *p, int t);
/* the clock reads timer / per (30: the dash's double-time "60"; 60: the
 * escape's seconds) */
void clc_draw_hud(const ClcPlayer *p, int notes, int timer, bool timer_on, int per, int t);
void clc_draw_ship(int x, int y, int face, bool flame, int t);
void clc_draw_clary_small(int x, int y, int face, int frame);
void clc_draw_clary(int x, int y, int face, int frame, int aim);
void clc_draw_item(int item, int x, int y, int t);
void clc_draw_face(int who, int x, int y, int scale); /* 0 Clary, 1 Ansel, 2 Lady Hush, 3 Tock */
void clc_draw_station(int region_sel, const uint8_t *visited, int cur, int t);
void clc_draw_backdrop(int region, int t);
void clc_fx_feed(const ClcFx *fx, int n);
void clc_fx_clear(void);
extern const uint8_t CLC_REGION_COL[RG_COUNT][2];

/* the hand-drawn sprites (clc_art.c) */
enum {
    CS_CLARY_S0, CS_CLARY_S1, CS_CLARY_SJ, CS_CLARY_B0, CS_CLARY_B1, CS_CLARY_BJ, CS_CLARY_BC,
    CS_NOTE, CS_TOCK, CS_SKULL, CS_LOBBER, CS_TORTOISE, CS_SEXTON, CS_VILLAGER, CS_KEEPER,
    CS_FLITTER, CS_GRUB, CS_JELLY, CS_BRUTE, CS_STINGER, CS_TROOPER, CS_FACE, CS_LOUSE,
    CS_COUNT
};
void clc_spr(int id, int x, int y, int flags);
void clc_spr_ex(int id, int x, int y, int flags, const uint8_t *map);
int clc_spr_w(int id);
int clc_spr_h(int id);

void clc_audio_load(void);
extern int CLC_MUS_TITLE, CLC_MUS_REGION[RG_COUNT], CLC_MUS_CAVE, CLC_MUS_DASH, CLC_MUS_BOSS, CLC_MUS_HUSH,
    CLC_MUS_TOCK, CLC_MUS_ESCAPE, CLC_MUS_SHOP, CLC_MUS_MAP, CLC_MUS_END, CLC_MUS_TRUE, CLC_MUS_SAD,
    CLC_MUS_OVER, CLC_MUS_CLEAR;

#endif
