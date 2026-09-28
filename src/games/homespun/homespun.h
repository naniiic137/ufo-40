/* HOMESPUN - shared declarations. Cartridge 44 of UFO 40, a tribute to
 * Pilot Quest (UFO 50 #44). See docs/games/44-homespun.md.
 *
 * Wick the courier pilot has crashed the Tumbleweed on the moor-moon
 * Oddmoor. At camp she knocks glints out of the Glowstone with her yo-yo,
 * plants glintbuds with Old Burl, crafts bars, builds huts, anvils, bins
 * and a smokehouse, and researches with Dr. Orrery. Every strip of jerky
 * buys two minutes out in the Wilds, where the clock is also her health.
 * The camp keeps working in real time while the console is on. */
#ifndef HOMESPUN_H
#define HOMESPUN_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define HS_SLOT 43          /* library slot index (UFO 50 #44) */

/* ---- the field ------------------------------------------------------------ */
#define HS_TW 20            /* a room is 20 x 10 tiles of 16 px: one screen */
#define HS_TH 10
#define HS_T 16
#define HS_OY 20            /* the field's top on screen (the HUD is above) */
#define HS_FW (HS_TW * HS_T)
#define HS_FH (HS_TH * HS_T)
#define HS_U 16             /* positions are in 1/16 pixel */

enum { D_RIGHT, D_DOWN, D_LEFT, D_UP };
extern const int HS_DX[4], HS_DY[4];

/* ---- the world -------------------------------------------------------------- */
#define HS_OW_W 8           /* the Wilds: 8 x 6 screens */
#define HS_OW_H 6
#define HS_SCREENS (HS_OW_W * HS_OW_H)
#define HS_START_SCREEN (5 * HS_OW_W + 3) /* the screen above camp */
#define HS_CAVES 11
#define HS_DUNS 3
#define HS_DROOMS 12
#define HS_PADS 5           /* hopstones: 0 is camp's, 1-4 in the Wilds */
#define HS_VSPOTS 7         /* places a noodler may stand (4 of them do) */

/* room ids: 0..47 the Wilds, then caves, then dungeon rooms, then camp */
#define HS_R_CAVE0 HS_SCREENS
#define HS_R_DUN0 (HS_R_CAVE0 + HS_CAVES)
#define HS_R_BASE (HS_R_DUN0 + HS_DUNS * HS_DROOMS)
#define HS_ROOMS (HS_R_BASE + 1)

enum { AR_BASE, AR_OVER, AR_CAVE, AR_DUN };

/* biomes of the Wilds */
enum { B_MEADOW, B_MARSH, B_WOODS, B_DUNES, B_CRAGS, B_ASH, B_CAVE, B_DUN, B_CAMP, B_COUNT };

/* what a cave holds (shuffled per save) */
enum { K_DUN1, K_DUN2, K_DUN3, K_SIS, K_GAMBLE, K_HUSH, K_TANGER, K_LOOT3, K_LOOT1, K_LOOT2, K_MEAT };

/* tiles */
enum {
    T_FLOOR, T_WALL, T_WATER, T_LAVA, T_BLOCK, T_CAVE, T_CRACK, T_SHUT,
    T_EXIT, T_BRIDGE, T_PATH, T_DOOR, T_COUNT
};

typedef struct HsSpawn { uint8_t kind, tx, ty; } HsSpawn;
typedef struct HsThingSpawn { uint8_t kind, tx, ty, arg; } HsThingSpawn;

#define HS_MAX_SPAWN 8
#define HS_MAX_THINGSP 8

/* The world: one layout every save shares, plus what the save's seed
 * draws (the barriers, what each cave holds, the gear's chest, the
 * noodlers). Tiles are made again each time a screen is entered. */
typedef struct HsWorld {
    uint32_t seed;
    uint8_t biome[HS_SCREENS];
    uint8_t tier[HS_SCREENS];     /* 0 low, 1 normal, 2 high */
    uint8_t open[HS_SCREENS];     /* bit per direction: always open */
    uint8_t block[HS_SCREENS];    /* bit per direction: a barrier this save */
    uint8_t gear_screen;          /* the chest spot that holds the gear this save */
    int8_t cave_of[HS_SCREENS];   /* cave index on this screen, -1 none */
    uint8_t cave_x[HS_SCREENS];   /* its mouth's tile x (top wall) */
    uint8_t cave_kind[HS_CAVES];
    uint8_t cave_screen[HS_CAVES];
    int8_t pad_of[HS_SCREENS];    /* hopstone index on this screen, -1 none */
    uint8_t pad_screen[HS_PADS];
    uint8_t ruins, lair;          /* the Volthog's screen and Mother Loom's */
    uint8_t lair_dir;             /* direction from the ruins to the lair */
    uint8_t vendor_at[HS_VSPOTS]; /* noodler kind at each spot, 0xFF none */
} HsWorld;
extern HsWorld hw;

/* ---- the save ------------------------------------------------------------------ */

enum { RS_METAL, RS_FERT, RS_BIGBIN, RS_PILL, RS_STAR, RS_FUEL, RS_COUNT };
enum { RES_GLINT, RES_BAR, RES_JERKY, RES_DATA, RES_THREAD, RES_COUNT };
enum { ST_HASTE, ST_HIDE, ST_HUNGER }; /* the standing stones (after an escape) */

#define HS_PLANTS 6
#define HS_ANVILS 6
#define HS_HUTS 2
#define HS_BINS 2
#define HS_SMOKE_MAX 6

typedef struct HsSave {
    uint32_t magic;
    uint32_t seed;               /* the world */
    uint8_t started, loop, marks, spent_marks;
    uint8_t stone[3];            /* HASTE, HIDE, HUNGER: 0..5 */
    uint8_t escapes, idol_escapes, intro_seen;
    int32_t res[RES_COUNT];      /* camp stores */
    uint8_t plants, smoke_built, smoke_stock, anvils; /* anvils: bit per plot built */
    uint8_t huts[HS_HUTS];       /* rooms: 0 (not built) .. 3 */
    uint8_t bins[HS_BINS];       /* 0 none, 1 built, 2 raised */
    uint8_t anvil_hand;          /* bit per anvil with a hand at it */
    uint8_t lab_hands, lab_fixed, research;
    uint8_t loom_home, parts, idol, letter, pipe, weapon, pads; /* parts: bit per dungeon */
    uint8_t pads_woken;          /* how many wild hopstones are awake (sets the next price) */
    uint16_t odds;               /* the Wilds' coins, carried always, lost when time runs out */
    uint32_t acc_plant, acc_smoke, acc_anvil, acc_lab, acc_loom; /* ms toward the next unit */
    uint32_t trips, fadeouts, hits;
    uint32_t alive_s;            /* seconds the console has been on with this save */
    /* a trip in progress */
    uint8_t on_trip, room, carry, mawbo_room;  /* carry: bits 0-2 parts, 3 gear, 4 idol */
    int16_t px, py;
    int32_t time_f;              /* frames left */
    int32_t gain[RES_COUNT];     /* found this trip (lost if time runs out) */
    uint8_t mawbo_wins, volt_down, face, pad0;
    int16_t mawbo_hp, pad1;
    uint16_t dead[HS_ROOMS];     /* foes beaten this trip, by spawn index */
    uint16_t opened[HS_ROOMS];   /* pots and chests used this trip */
    uint32_t excursion;          /* trips begun (seeds each trip's dice) */
} HsSave;
extern HsSave sv;

/* ---- the economy (homespun_base.c) ---------------------------------------------- */

#define GLINTS_PER_BAR 1000
#define JERKY_PRICE 500        /* glints */
#define JERKY_SECONDS 120
#define SMOKE_BUILD 1          /* bars */
#define SMOKE_MS 120000        /* one strip of jerky smoked (reading) */
#define PLANT_MS 2000          /* a glintbud: 1 glint every 2 s ... */
#define PLANT_FERT_MS 1500     /* ... every 1.5 s with Fertilizer */
#define ANVIL_MS 120000        /* a hand at an anvil: a bar every 2 min ... */
#define ANVIL_PILL_MS 30000    /* ... two a minute with the Pep Pill */
#define LAB_MS 60000           /* a hand at the Thinker: 1 data a minute */
#define LOOM_MS 30000          /* Mother Loom: a spool every 30 s ... (reading) */
#define LOOM_HAND_MS 4000      /* ... 4 s sooner for each idle hand ... */
#define LOOM_MIN_MS 8000       /* ... but no quicker than 8 s (reading) */
#define HUT_COST0 5
#define HUT_COST1 20
#define HUT_COST2 100
#define ANVIL_COST 5
#define BIN_COST 50
#define BIN_RAISE 200
#define TRADE_BARS 100
#define TRADE_DATA 20

typedef struct HsResearch {
    const char *name, *what;
    int bars, data, thread;
} HsResearch;
extern const HsResearch HS_RESEARCH[RS_COUNT];

int hs_plant_cost(int n);         /* glints for the n-th glintbud (0-based) */
int hs_cap(int res);              /* what camp and bag can hold */
int hs_bin_level(void);
int hs_hands(void);               /* hands the huts hold */
int hs_free_hands(void);
int hs_anvil_hands(void);
int hs_yoyo_dmg(void);
int hs_hit_cost_s(int base_s);    /* after the HIDE stone */
int hs_jerky_seconds(void);       /* after the HUNGER stone */
int hs_loom_ms(void);             /* how long a spool of thread takes now */
/* Real time passing: ms of production for everything at camp. */
void hs_produce(uint32_t ms);
bool hs_pay(int bars, int glints, int data, int thread); /* false: not enough (nothing taken) */
void hs_add(int res, int n);      /* into camp stores, up to the cap */

/* ---- menus and talk ------------------------------------------------------------- */

#define HS_MENU_MAX 8
typedef struct HsMenuLine {
    char label[28], cost[20];
    uint8_t act, arg, ok;
} HsMenuLine;
typedef struct HsMenu {
    int n, sel, open;
    char title[28];
    HsMenuLine line[HS_MENU_MAX];
} HsMenu;
extern HsMenu hm;
void hs_menu_clear(const char *title);
void hs_menu_add(const char *label, const char *cost, int act, int arg, bool ok);

/* menu actions */
enum {
    M_CLOSE, M_PLANT, M_CRAFT, M_SMOKE_BUILD, M_JERKY, M_HUT, M_ANVIL_BUILD, M_ANVIL_HAND,
    M_ANVIL_FREE, M_BIN, M_LAB_ADD, M_LAB_SUB, M_RESEARCH, M_TRADE, M_LAUNCH, M_STONE,
    M_WAKE, M_HOP, M_BUY, M_BET, M_NOTHING
};

/* camp spots (the things at camp you can face and press A at) */
enum {
    P_BURL, P_STONE, P_BENCH, P_SHIP, P_TOLLY, P_SMOKE, P_HUT1, P_HUT2, P_BIN1, P_BIN2,
    P_ANVIL1, P_ANVIL2, P_ANVIL3, P_ANVIL4, P_ANVIL5, P_ANVIL6, P_LAB, P_ORRERY,
    P_NEST, P_STONE_HASTE, P_STONE_HIDE, P_STONE_HUNGER, P_PAD, P_COUNT
};
typedef struct HsSpot { int x, y, w, h; uint8_t solid; } HsSpot;
extern const HsSpot HS_SPOT[P_COUNT];
bool hs_spot_shown(int p);
/* Open the menu (or a line of talk) for a camp spot. */
void hs_base_menu(int p);
/* Do a menu line's action at camp; returns true if it did something. */
bool hs_base_act(int act, int arg);

/* ---- foes ------------------------------------------------------------------------ */

enum {
    E_SPINNER, E_GRUBLET,                                                    /* low */
    E_SPLOSH, E_SKITTER, E_STINGLE, E_GNATTER, E_SPITBLOOM, E_GRUMMLE,        /* normal */
    E_LOBBER, E_SHELLBACK, E_DRIPLET,
    E_CLACKER, E_REDCLACKER, E_OOZER, E_PUFFCAP, E_ZIPSNAKE, E_BROODER,       /* high */
    E_TREADLE, E_LEECHLING,
    E_GRUMMKING, E_THORNMOTHER, E_ZIZZIK, E_VOLTHOG, E_LOOM, E_MAWBO,        /* bosses */
    E_KINDS
};
#define E_FIRST_BOSS E_GRUMMKING
typedef struct HsFoeDef {
    const char *name;
    int hp;       /* yo-yo hits at 1 damage */
    int tier;     /* 0 low, 1 normal, 2 high, 3 boss */
    int hit_s;    /* seconds lost when it touches Wick */
    int speed;    /* 1/16 px a frame */
    int size;     /* body box, px */
} HsFoeDef;
extern const HsFoeDef HS_FOE[E_KINDS];

/* things standing in a room */
enum {
    TH_POT, TH_CHEST, TH_PAD, TH_NPC, TH_VENDOR, TH_GEAR, TH_PART, TH_CACHE, TH_TABLE, TH_KINDS
};
/* who (TH_NPC arg) */
enum { N_KIT, N_SHUFFLE, N_HUSH, N_TANGER, N_HERMIT };
/* noodler wares */
enum { V_DATA, V_THREAD, V_TONIC3, V_TONIC2, V_KINDS };

/* pickups */
enum { IT_GLINT, IT_BAR, IT_JERKY, IT_ODD, IT_DATA, IT_THREAD, IT_TONIC, IT_BIGTONIC, IT_GEAR, IT_PART, IT_IDOL, IT_KINDS };


#define TONIC_S 100            /* a tonic a noodler sells */
#define TONIC_BIG_S 100        /* a boss's tonic */

/* ---- the running sim (homespun_wild.c) ---------------------------------------------- */

#define HS_MAX_MOBS 24
#define HS_MAX_SHOTS 64
#define HS_MAX_ITEMS 48
#define HS_MAX_THINGS 16

typedef struct HsMob {
    uint8_t on, kind, spawn;     /* spawn: index for the trip's beaten mask (0xFF none) */
    int32_t x, y;                /* centre, 1/16 px */
    int32_t vx, vy;
    int16_t hp, t, st, stun, flash, cool, cool2;
    int8_t dx, dy, face, up;     /* up: burrowers and fish above ground */
    int32_t tx, ty;              /* a target point */
} HsMob;

enum { SH_PELLET, SH_ROCK, SH_SEED, SH_SPORE, SH_WEB, SH_BOLT, SH_BOMB, SH_BLAST, SH_SLUG, SH_SHELL };
typedef struct HsShot {
    uint8_t on, kind, mine;      /* mine: Wick's */
    int32_t x, y, vx, vy;
    int16_t life, fuse;
} HsShot;

typedef struct HsItem {
    uint8_t on, kind;
    int16_t amount, t;
    int32_t x, y;
} HsItem;

typedef struct HsFx { uint8_t on, kind; int16_t t, x, y; } HsFx; /* puffs and sparks, pixels */
enum { FX_PUFF, FX_SPARK, FX_BOOM, FX_HOP };

typedef struct HsThing {
    uint8_t on, kind, arg, idx;  /* idx: bit in the trip's opened mask */
    uint8_t tx, ty, open;
} HsThing;

typedef struct HsSim {
    int area, room;
    uint8_t tile[HS_TH][HS_TW];
    uint8_t biome;
    /* Wick */
    int32_t px, py;
    int dirx, diry;              /* facing, -1..1 each */
    int walk_t, inv, knock_t, kx, ky;
    int yo_t;                    /* 0: yo-yo home; else frames into a throw */
    int32_t yo_x, yo_y;
    uint32_t yo_hit;             /* mobs struck this throw */
    uint8_t yo_stone;            /* the Glowstone struck this throw */
    int shoot_cool, muzzle_t;
    uint32_t bump_hit;
    /* the room */
    HsMob mob[HS_MAX_MOBS];
    HsShot shot[HS_MAX_SHOTS];
    HsItem item[HS_MAX_ITEMS];
    HsThing thing[HS_MAX_THINGS];
    HsFx fxs[24];
    int boss;                    /* index of the boss mob in the room, -1 none */
    int shut;                    /* the room's shutters are closed */
    int frame;
    /* messages */
    char msg[96];
    int msg_t;
    int events;                  /* bits for the cartridge: EV_* */
    int last_hit_s;
    uint32_t shot_seen;          /* kinds of shot fired in this room (for tests) */
} HsSim;
extern HsSim hs;

enum {
    EV_FADED = 1, EV_HOME = 2, EV_GATE = 4, EV_TALK = 8, EV_BOSSDOWN = 16, EV_LOOM = 32,
    EV_MAWBO_FLED = 64, EV_IDOL = 128, EV_HURT = 256, EV_PICK = 512
};

/* player speed and weapons */
#define WICK_SPEED 20          /* 1.25 px a frame */
#define YOYO_OUT 10            /* frames out, and as many back */
#define YOYO_REACH 40          /* px */
#define PIPE_COOL 12
#define PIPE_SPEED 80          /* 5 px a frame */
#define HIT_INV 90             /* a second and a half safe after a hit */

void hs_world_make(uint32_t seed);
/* The roads a trip finds: bit per direction, open. */
uint8_t hs_exits(int screen);
bool hs_edge_open(int screen, int dir);
int hs_neighbour(int screen, int dir);
/* Build a room's tiles and spawn lists (from the seed; the trip's masks skip what is gone). */
void hs_room_tiles(int room, uint8_t tile[HS_TH][HS_TW], int *biome);
int hs_room_spawns(int room, HsSpawn *sp, int max);
int hs_room_things(int room, HsThingSpawn *ts, int max);
int hs_room_area(int room);
/* dungeon rooms: which exists next door (-1 none) */
int hs_dun_neighbour(int room, int dir);
int hs_dun_of(int room);
bool hs_dun_boss_room(int room);
const char *hs_room_name(int room);
/* world checks for the tests: 0 if sound, else a code */
int hs_world_check(uint32_t seed);
int hs_dun_check(void);
int hs_dun_cave(int d);           /* the cave mouth that leads into dungeon d */
int hs_world_variety(int seeds); /* bits: what changes from save to save and trip to trip */
void hs_kill_all(void);
const char *hs_dun_name(int d);
const char *hs_cave_name(int kind);

/* the sim */
void hs_sim_seed(uint32_t seed);
void hs_sim_enter(int room, int32_t px, int32_t py);
void hs_sim_update(uint32_t pad, uint32_t press);
bool hs_solid_px(int x, int y);  /* field pixels */
int hs_front_spot(void);         /* the camp spot Wick faces, -1 none */
int hs_front_thing(void);        /* the thing Wick faces in the Wilds, -1 none */
void hs_open_chest(int i);
void hs_spawn_drip(int32_t x, int32_t y);   /* a driplet that drops nothing */
void hs_env_loot(int rolls, bool dungeon, int32_t x, int32_t y);
void hs_loot_stats(int tier, int n, int out[4]);   /* tests: the foe tables, rolled n times */
void hs_env_stats(bool dungeon, int n, int out[5]); /* tests: pots' and chests' table */
int hs_add_mob(int kind, int tx, int ty, int spawn);
bool hs_mob_tell(const HsMob *m); /* a leap or dive is about to come */
void hs_drop(int kind, int amount, int32_t x, int32_t y);
void hs_say(const char *s);
void hs_bank_trip(void);         /* home: everything found joins camp */
void hs_lose_trip(void);         /* time out: everything found is gone */
void hs_start_trip(int room, int32_t px, int32_t py);
int hs_total(int res);           /* camp plus the bag */
bool hs_spend_glint(void);       /* a thunderpipe shot */
bool hs_mawbo_here(void);
int hs_parts_count(void);

bool hs_box_free(int cx, int cy, int half); /* field pixels: can Wick's box stand here */
extern int hs_hurt_by[16];

/* the cartridge's flow, for the demo player */
enum { S_TITLE, S_INTRO, S_PLAY, S_FADED, S_HOME, S_LAUNCH, S_CREDITS };
int hs_game_state(void);
bool hs_talk_open(void);
int hs_gamble_state(void);

/* ---- bot (homespun_bot.c) ------------------------------------------------------------- */
extern bool hs_bot_cherry;       /* the demo player also goes for the Alien */
extern bool hs_bot_arena;        /* it only fights what is in the room (tests) */
void hs_bot_reset(void);
int hs_bot_buttons(void);
extern int hs_bot_goal;          /* what the demo player is after (for tests) */
extern int hs_bot_idle;          /* it is waiting at camp with nothing to do */

/* ---- art and audio -------------------------------------------------------------------- */
enum {
    HS_WICK_D0, HS_WICK_D1, HS_WICK_U0, HS_WICK_U1, HS_WICK_S0, HS_WICK_S1, HS_WICK_THROW,
    HS_FOE0, /* one sprite per foe kind (bosses are 24x24) */
    HS_FOE_B = HS_FOE0 + E_KINDS, /* a second frame for each foe */
    HS_NPC0 = HS_FOE_B + E_KINDS, /* Kit, Shuffle, Hush, Tanger, Hermit */
    HS_TOLLY = HS_NPC0 + 5, HS_GRISTLE, HS_ORRERY, HS_HAND, HS_NOODLER, HS_BURL,
    HS_ICON0, /* 8x8 icons: one per pickup kind */
    HS_ICON_PIPE = HS_ICON0 + IT_KINDS, HS_ICON_YOYO, HS_ICON_LETTER,
    HS_SPR_COUNT
};
extern Sprite hs_spr[HS_SPR_COUNT];
extern int hs_art_bad;
void hs_art_load(void);
void hs_draw_tile(int t, int biome, int x, int y, int tx, int ty, int room);
void hs_draw_room(void);
void hs_draw_hud(void);
void hs_draw_base(int t);
void hs_draw_menu(void);
void hs_draw_label(int x, int y, int w, int h, int t);
void hs_draw_icon(int kind, int x, int y);

extern int HS_MUS_TITLE, HS_MUS_CAMP, HS_MUS_WILDS, HS_MUS_DEEP, HS_MUS_CAVE, HS_MUS_BOSS, HS_MUS_MAWBO,
    HS_MUS_END, HS_MUS_HOME, HS_MUS_FADE;
void hs_audio_load(void);

#endif
