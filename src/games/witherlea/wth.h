/* WITHERLEA - shared declarations. Cartridge 42 of UFO 40.
 * A tribute to Night Manor (UFO 50 #42); see docs/games/42-witherlea.md.
 *
 * You wake locked in the cold store of Witherlea, a house in the fens. One
 * picture a room, a slow cursor on the pad, things to look at, take, use
 * and put together, and the Keeper (a man in a beekeeper's veil with a
 * billhook) who comes when too long goes by or when you make a noise.
 * Hide, run, or strike him; caught, you wake in the cold store with
 * everything you had.
 *
 *   wth_data.c   the rooms, every hotspot, the items, the diary, the words
 *   wth_logic.c  clicks: looking, taking, using, combining, the exits
 *   wth_keeper.c the Keeper: the clocks, the chase, hiding, stuns, capture
 *   wth.c        the screens (warning, title, the house, the bag, the
 *                diary, endings and credits), saving, goals, test hooks
 *   wth_bot.c    the demo player (real button presses)
 *   wth_draw.c   the room pictures   wth_art.c sprites   wth_audio.c music
 *
 * Gameplay uses integers only (the cursor in 1/256 px). */
#ifndef WTH_H
#define WTH_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the screen ------------------------------------------------------- */
#define WTH_PX 8           /* the room picture */
#define WTH_PY 6
#define WTH_PW 304
#define WTH_PH 128
#define WTH_TY 138         /* the text panel */
#define WTH_TLINES 4

/* ---- rooms: 28 rooms and the two parts of the maze --------------------- */
enum {
    R_CELL, R_PASSAGE, R_SICKROOM, R_HALL, R_PARLOUR, R_DINING, R_KITCHEN, R_SCULLERY, R_SMOKING,
    R_WASHROOM, R_GUEST, R_GLASSHOUSE, R_GARAGE, R_POND, R_SHED, R_GARDEN, R_DRIVE, R_COURTYARD,
    R_MAZE, R_HEART, R_LANDING, R_MASTER, R_BALCONY, R_STUDY, R_BATH, R_NURSERY, R_CHILDREN,
    R_ATTIC, R_CELLAR, R_SPORE, R_COUNT
};
extern const char *const WTH_ROOM_NAME[R_COUNT];
bool wth_room_dark(int r);          /* upstairs and the cellar */
bool wth_room_outside(int r);

/* ---- items: 54 kinds ----------------------------------------------------- */
enum {
    I_NONE,
    I_SHOEHORN, I_MUG, I_WIRE, I_NEWSPAPER, I_DIARY, I_PENNIES, I_MATCHES, I_KNIFE, I_SODA, I_TONGS,
    I_TINKEY, I_CUPHOOK, I_COPPERKEY, I_GEAR, I_TWINE, I_OIL, I_TORCH, I_PARAFFIN, I_BAR, I_BATTERIES,
    I_CANE, I_LONGHOOK, I_HYMN, I_SHEDKEY, I_SPANNER, I_SCREWDRIVER, I_SHEARS, I_SPADE, I_PEWTERKEY,
    I_MOTOR, I_CARKEYS, I_PIG, I_BRASSKEY, I_SAW, I_GARNET, I_RING, I_JADE, I_SNUFF, I_CLOVE, I_BORAX,
    I_LENS, I_COMBO, I_OPAL, I_DOLL, I_SILVERKEY, I_PASSWORD, I_GLASSES, I_DIRECTIONS, I_CROSSBOW,
    I_STEELWIRE, I_AMBER, I_BOLT, I_KEEPERKEY, I_BREW,
    I_COUNT
};
#define WTH_ITEM_KINDS (I_COUNT - 1) /* 54 */
typedef struct WthItem {
    const char *name;
    const char *desc;               /* the bag's line about it */
} WthItem;
extern const WthItem WTH_ITEM[I_COUNT];
const char *wth_item_name(int it);  /* with its state (a loaded crossbow) */

/* ---- flags: the house as you have changed it -------------------------------- */
#define WTH_FLAG_LIST(X) \
    X(F_STRAW) X(F_VENT_OPEN) X(F_VENT_WIRE) X(F_VENT_PAPER) X(F_CELL_OPEN) X(F_LEFT_CELL) X(F_MUG_TAKEN) \
    X(F_COAT) X(F_TORCH_TAKEN) X(F_CANE_TAKEN) X(F_CLOCK_WOUND) X(F_PIG_TAKEN) X(F_FIRE_SOAKED) \
    X(F_FIRE_LIT) X(F_GARNET_TAKEN) X(F_HYMN_OUT) X(F_TONGS_TAKEN) X(F_KNIFE_TAKEN) X(F_MATCHES_TAKEN) \
    X(F_BAR_TAKEN) X(F_SODA_TAKEN) X(F_SNUFF_TAKEN) X(F_BATT_TAKEN) X(F_BATH_DRAINED) X(F_TINKEY_TAKEN) \
    X(F_BORAX_TAKEN) X(F_BOX_OPEN) X(F_OIL_TAKEN) X(F_HOOK_TAKEN) X(F_SPANNER_TAKEN) X(F_TWINE_TAKEN) \
    X(F_PARAFFIN_TAKEN) X(F_MOTOR_FITTED) X(F_RAILS_OILED) X(F_GARAGE_OPEN) X(F_POND_DONE) X(F_SHED_OPEN) \
    X(F_MOTOR_TAKEN) X(F_SCREW_TAKEN) X(F_SHEARS_TAKEN) X(F_SAW_TAKEN) X(F_SPADE_TAKEN) X(F_PEWTER_TAKEN) \
    X(F_GRAVE_DUG) X(F_IVY_CUT) X(F_ROPE_CUT) X(F_AMBER_TAKEN) X(F_FOUNTAIN_DRAINED) X(F_JADE_TAKEN) \
    X(F_GATE_OPEN) X(F_TORCH_LOADED) X(F_STAIRS_LIT) X(F_MASTER_OPEN) X(F_LENS_TAKEN) X(F_SAFE_OPEN) \
    X(F_OPAL_TAKEN) X(F_CASE_BROKEN) X(F_RING_TAKEN) X(F_RECIPE) X(F_MIRROR_BROKEN) X(F_COMBO_TAKEN) \
    X(F_CLOVE_TAKEN) X(F_BOXMUSIC) X(F_GLASSES_TAKEN) X(F_WALL_READ) X(F_DOLL_TAKEN) X(F_DOLLHOUSE_READ) \
    X(F_HATCH_DOWN) X(F_TRUNK_OPEN) X(F_HARMONIUM_PLAYED) X(F_WIRE2_TAKEN) X(F_BOW_LOADED) X(F_KEEPER_DEAD) \
    X(F_STORM_OPEN) X(F_MECH_GEAR) X(F_MECH_WIRE) X(F_MECH_SCREW) X(F_MECH_OIL) X(F_IRONDOOR_OPEN) \
    X(F_BOLT_TAKEN) X(F_HIDE_SCREEN_GONE) X(F_HIDE_ALCOVE_GONE) X(F_HIDE_BED_GONE) X(F_HIDE_CHEST_GONE) \
    X(F_STUN_KNIFE) X(F_STUN_BAR) X(F_EVER_CAUGHT) X(F_GIFT) X(F_GOLD_DONE) X(F_CHERRY_DONE) X(F_BEST_DONE) \
    X(F_OIL_USED1) X(F_HEART_SEEN) X(F_WARNED)
#define X(f) f,
enum { WTH_FLAG_LIST(X) F_COUNT };
#undef X
#define WTH_FLAG_BYTES 16

/* ---- hotspots ------------------------------------------------------------ */
enum { HS_THING, HS_EXIT, HS_HIDE };
enum { DIR_UP, DIR_LEFT, DIR_RIGHT, DIR_DOWN };
typedef struct WthSpot {
    uint8_t room, kind;
    int16_t x, y, w, h;             /* screen pixels, inside the picture */
    uint8_t dest, dir;              /* exits */
    const char *look;               /* what A says with nothing in hand (NULL: wth_click decides) */
} WthSpot;
extern const WthSpot WTH_SPOT[];
extern const int WTH_SPOTS;
/* spot ids are their index in WTH_SPOT; the names are in wth_data.c's enum */
#include "wth_spots.h"
bool wth_spot_live(int s);          /* shown and clickable as the house stands now */
int wth_spot_at(int x, int y);      /* the live spot under a point in the current room, -1 */
int wth_exit_dest(int s);           /* where an exit leads now, -1 = not passable */

/* ---- the diary: 17 pages ---------------------------------------------------- */
#define WTH_PAGES 17
typedef struct WthPage { const char *date; const char *text; } WthPage;
extern const WthPage WTH_PAGE[WTH_PAGES];    /* in date order */
extern const uint8_t WTH_PAGE_SPOT[WTH_PAGES]; /* the spot each page lies at */

/* ---- the maze ----------------------------------------------------------------- */
#define WTH_MAZE_STEPS 6
extern const uint8_t WTH_MAZE_WAY[WTH_MAZE_STEPS]; /* DIR_LEFT, DIR_UP (straight on), DIR_RIGHT */
/* the hares' eyes, left to right */
extern const uint8_t WTH_HARE_GEM[4];

/* ---- the save: everything about the house ------------------------------------- */
#define WTH_BAG 30
#define WTH_FLOOR 3
typedef struct WthSave {
    uint32_t magic;
    uint16_t version;
    uint8_t started, room;
    uint8_t bag[WTH_BAG];               /* item kinds, 0 = an empty slot */
    uint8_t flag[WTH_FLAG_BYTES];
    uint32_t pages;                     /* bit n: page n (date order) found */
    uint8_t floor[R_COUNT][WTH_FLOOR];  /* things lying on a room's floor */
    uint8_t hare[4];                    /* gems set in the hares' eyes */
    uint8_t mug;                        /* bits: snuff, clove oil, borax */
    uint8_t maze;                       /* junctions passed */
    uint8_t body_room;                  /* where the Keeper lies (once he's dead) */
    uint8_t pad[3];
    int16_t cx, cy;                  /* the cursor, px */
    int32_t spawn_left;                 /* frames until the Keeper comes by himself */
    int32_t interval;                   /* ... and the wait after each escape */
    uint16_t caught, escapes, stuns;
    uint16_t golds, cherries, bests;
    uint32_t frames;                    /* time played in the house */
} WthSave;
#define WTH_MAGIC 0x57544801u
#define WTH_VERSION 1

/* ---- the Keeper ------------------------------------------------------------------- */
enum { E_NONE, E_COMING, E_HERE, E_FOLLOW, E_STUN, E_HIDING, E_CAUGHT };
#define WTH_NOISE_T 150          /* 2.5 s after a noise */
#define WTH_MAZE_NOISE_T 390     /* 6.5 s after a wrong turn in the maze */
#define WTH_CATCH_T 240          /* in your room: this long to get out */
#define WTH_FOLLOW_T 210         /* he reaches the next room */
#define WTH_STUN_T 240
#define WTH_OUTRUN 4             /* rooms in a row */
#define WTH_FIRST_WAIT (90 * 60) /* the first timed visit */
#define WTH_WAIT_STEP (30 * 60)  /* added after each escape */
#define WTH_WAIT_MAX (180 * 60)
#define WTH_CAUGHT_T 100         /* the swing, then black */
/* the breathing game */
#define WTH_BREATHS 8
#define WTH_MISSES 3
#define WTH_BAR_W 160
typedef struct WthMini {
    uint8_t n, miss, hit;           /* prompts done, missed, hit */
    int32_t pos, vel;               /* the marker, 1/256 px across the bar */
    int16_t zone, zw;               /* the safe square */
    int16_t passes;                 /* edge bounces this prompt (two full sweeps = a miss) */
    int16_t flash, flash_ok;        /* the last result, for drawing */
    int16_t pause;                  /* between prompts */
} WthMini;
typedef struct WthKeeper {
    uint8_t state;
    int32_t t;                      /* the countdown of the state */
    uint8_t chain;                  /* rooms put behind you */
    uint8_t room;                   /* where he is (E_HERE, E_STUN) */
    int8_t hide_spot;
    uint8_t noise_maze;
    int16_t arrive;                 /* frames since he came into the room (drawing) */
    WthMini mini;
} WthKeeper;
int wth_hide_one_use_flag(int spot); /* F_HIDE_*_GONE, or -1 for the two that last */

/* ---- the one state ------------------------------------------------------------------- */
enum { SC_WARNING, SC_TITLE, SC_INTRO, SC_PLAY, SC_BAG, SC_DIARY, SC_WAKE, SC_ENDING, SC_CREDITS, SC_STINGER };
enum { END_NONE, END_CAR, END_CELLAR };
#define WTH_MSG_PAGES 8
typedef struct WthState {
    WthSave s;
    WthKeeper k;
    Rng rng;
    int scene, scene_t;
    uint32_t frame;
    int32_t cx, cy;                 /* cursor, 1/256 px */
    int held;                       /* the item held as the cursor (0) */
    int held_slot;
    int bag_sel;
    int diary_sel;
    int title_sel, title_erase;
    /* the text panel */
    char msg[WTH_MSG_PAGES][WTH_TLINES][UI_WRAP_LEN];
    int msg_n, msg_page;
    /* changing rooms */
    int trans_t, trans_to;
    int ending;                     /* END_* */
    uint8_t ending_best, ending_caught;
    int shake_x, shake_y;           /* the scared cursor's tremble */
    uint8_t bot_on, bot_plan;
    uint8_t noise_count;            /* noises made (tests) */
    int last_noise;
} WthState;
extern WthState wth;

static inline bool wth_flag(int f) { return (wth.s.flag[f >> 3] >> (f & 7)) & 1; }
static inline void wth_set(int f) { wth.s.flag[f >> 3] |= (uint8_t)(1 << (f & 7)); }
static inline void wth_clear(int f) { wth.s.flag[f >> 3] &= (uint8_t)~(1 << (f & 7)); }

/* ---- shared rules ------------------------------------------------------------------------ */
/* the bag */
bool wth_has(int it);
int wth_slot_of(int it);
int wth_give(int it);               /* into the first empty slot; returns the slot or -1 */
void wth_take_away(int it);         /* leaves a hole */
int wth_bag_count(void);
int wth_page_count(void);
/* the text panel */
void wth_say(const char *text);     /* queued; pages of four lines */
void wth_sayf(const char *fmt, ...);
bool wth_talking(void);
/* clicks (wth_logic.c) */
void wth_click(int spot, int held);           /* A on a spot, with the item held (0) */
void wth_combine(int a, int b);                /* one item on another in the bag */
void wth_goto_room(int room);                  /* the exit's far side (after the fade) */
void wth_start_exit(int dest);                 /* the fade */
void wth_noise(bool maze);                     /* a loud thing: the Keeper comes */
/* the Keeper (wth_keeper.c) */
void wth_keeper_reset(void);
void wth_keeper_step(bool in_transition);
void wth_keeper_room_changed(int from, int to);
void wth_keeper_click(int held);               /* A on him */
bool wth_keeper_visible(void);                 /* drawn in the room now */
void wth_keeper_rect(int *x, int *y, int *w, int *h);
bool wth_scared(void);
void wth_hide_begin(int spot);
void wth_mini_step(bool press);
void wth_caught_now(void);
void wth_escaped(void);
/* screens (wth.c) */
void wth_goto(int scene);
void wth_save_now(void);
void wth_award(int bit);
void wth_end_game(int ending);
/* the demo player (wth_bot.c) */
enum { BOT_GOLD, BOT_CHERRY, BOT_CHERRY_CAUGHT };
void wth_bot_reset(int plan);
unsigned wth_bot(void);
extern int wth_bot_step;
extern int wth_bot_debug;
/* drawing */
void wth_draw_room(int room);
void wth_draw_play(void);
void wth_draw_bag(void);
void wth_draw_diary(void);
void wth_draw_screen(void);         /* warning, title, intro, wake, endings, credits */
void wth_draw_label(int x, int y, int w, int h, int t);
void wth_draw_item(int it, int x, int y);
void wth_draw_keeper(int x, int y, int scale, int t, int pose);
int wth_layout_audit(void);
/* art and sound */
void wth_art_load(void);
void wth_audio_load(void);
int wth_art_bad(void);
extern int WTH_MUS_TITLE, WTH_MUS_HOUSE, WTH_MUS_UPSTAIRS, WTH_MUS_GARDEN, WTH_MUS_CHASE, WTH_MUS_HIDE,
    WTH_MUS_CELLAR, WTH_MUS_MAZE, WTH_MUS_ENDING, WTH_MUS_DAWN, WTH_MUS_CAUGHT, WTH_MUS_STING;
void wth_room_music(void);
/* small helpers */
int wth_rand(int lo, int hi);

#endif
