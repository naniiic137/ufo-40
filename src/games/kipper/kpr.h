/* KIPPER - shared declarations. Cartridge 22 of UFO 40.
 * A tribute to Porgy (UFO 50 #22); see docs/games/22-kipper.md.
 *
 * Dr. Juno Marlow takes her little yellow sub KIPPER down into Gullwater
 * Sound, where the sea creatures have turned mean. Fuel is health and time:
 * swimming, dashing and every knock burn it, and an empty tank means a tow
 * back to the dock with the dive's finds lost. Anything picked up trails
 * behind the sub (three at most) and only counts once it is home at a dock.
 * Three areas stacked by depth, two docks on the surface, 20 fuel tanks, 20
 * torpedo boosters, 20 eggs for Auntie Roe, 10 pieces of equipment (2 to 4
 * fitted at once), six big creatures and Xolloth at the bottom.
 *
 * The whole game lives in one struct with no pointers (kg), so the demo
 * player can copy it. Positions are floats in pixels. */
#ifndef KPR_H_INCLUDED
#define KPR_H_INCLUDED

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include <ctype.h>

/* ---- the sea (kpr_sea.c, written by tools/kipper/make_sea.py) ------------ */
#define KPR_W 160
#define KPR_H 150
#define KPR_TILE 16
#define KPR_PW (KPR_W * KPR_TILE)
#define KPR_PH (KPR_H * KPR_TILE)
#define KPR_SKY 3                  /* rows of sky over the water line */
#define KPR_AREA2 50               /* the first row of each lower area */
#define KPR_AREA3 100
#define KPR_NITEMS 70
#define KPR_NDOCKS 2
#define KPR_NVENTS 10
#define KPR_NEYES 14
#define KPR_BOSSES 6
#define KPR_HUD_W 64               /* the console on the left */
#define KPR_VIEW_W (SCREEN_W - KPR_HUD_W)
#define KPR_VIEW_H SCREEN_H

typedef struct { int16_t x, y; } KprPoint;
typedef struct { int16_t x0, y0, x1, y1; } KprRect;
typedef struct { const KprPoint *pts; int n; } KprRoute;

enum { IK_TANK, IK_BOOSTER, IK_EGG, IK_EQUIP };
typedef struct { uint8_t kind, var; int16_t x, y; } KprItemDef;

enum {
    FK_POLYP, FK_FILCHER, FK_GLIMMER, FK_GREENLING, FK_SHELLBACK, FK_KITERAY, FK_BLUEBELL, FK_REDCAP,
    FK_LANTERNMAW, FK_RISER, FK_PIPEHORSE, FK_BLOATER, FK_STILTCRAB, FK_SUNSTAR, FK_BILLFISH, FK_BURR,
    FK_SNAPWORM, FK_FIZZWORM, FK_KINDS
};
typedef struct { uint8_t kind; int16_t x, y, a, b; } KprFoeDef;

extern const char *const KPR_ROWS[KPR_H];
extern const KprItemDef KPR_ITEMS[KPR_NITEMS];
extern const KprFoeDef KPR_FOES[];
extern const int KPR_NFOES;
extern const KprPoint KPR_DOCKS[KPR_NDOCKS];
extern const KprPoint KPR_ROE;
extern const KprPoint KPR_VENTS[KPR_NVENTS];
extern const KprPoint KPR_EYES[KPR_NEYES];
extern const KprRoute KPR_ROUTES[KPR_BOSSES];
extern const KprRect KPR_ARENA_WHORL, KPR_ARENA_XOLLOTH;

/* ---- equipment ------------------------------------------------------------- */
enum {
    EQ_BUSTER, EQ_DRILL, EQ_BOOSTER, EQ_CHARGE, EQ_MISSILE, EQ_TARGET, EQ_SPOT, EQ_RADAR, EQ_EFFICIENT, EQ_ARMOR,
    KPR_EQUIP
};
extern const char *const KPR_EQ_NAME[KPR_EQUIP];
extern const char *const KPR_EQ_LINE[KPR_EQUIP];

/* ---- bosses --------------------------------------------------------------- */
enum { BS_GNASHER, BS_INKWELL, BS_LONGJAW, BS_WHORL, BS_NIGHTCAPE, BS_XOLLOTH };
extern const char *const KPR_BOSS_NAME[KPR_BOSSES];
extern const int KPR_BOSS_HP[KPR_BOSSES];
extern const char *const KPR_FOE_NAME[FK_KINDS];

/* ---- the rules -------------------------------------------------------------- */
#define KPR_BAR 360                /* fuel in a bar: frames of swimming */
#define KPR_BARS0 4                /* bars at the start; +1 a tank */
#define KPR_HIT 180                /* a knock costs half a bar */
#define KPR_DASH_COST KPR_HIT      /* a dash: half a bar, as much as a knock */
#define KPR_BOOST_COST 2           /* a frame of the long thruster: cheaper than swimming per px */
#define KPR_SPEED 1.0f             /* swimming, px a frame */
#define KPR_ACCEL 0.08f
#define KPR_DRAG 0.06f
#define KPR_DASH_SPEED 3.2f
#define KPR_DASH_T 12
#define KPR_BOOST_SPEED 2.6f
#define KPR_MERCY 60
#define KPR_TRAIL 3
#define KPR_SW 12                  /* the sub's box */
#define KPR_SH 10
#define KPR_FIRE_HOLD 14           /* held fire: a torpedo every 14 frames */
#define KPR_FIRE_TAP 7             /* tapping: as fast as every 7 */
#define KPR_TORP_SPEED 3.0f
#define KPR_MISSILE_HOLD 40
#define KPR_CHARGE_HOLD 20
#define KPR_CHARGES 2
#define KPR_PLATES 3
#define KPR_RUB 40                 /* frames of rubbing open a cracked wall */
#define KPR_CRACK_HITS 3
#define KPR_FOE_DROP 12            /* % a pacified creature leaves fuel */

int kpr_torp_damage(void);
int kpr_torp_range(void);          /* px */

/* ---- dynamic things ------------------------------------------------------- */
enum { TR_ITEM, TR_CHARGE };
typedef struct { uint8_t kind; int16_t id; } KprTrailObj;

/* where an item is during a dive */
enum { IW_HOME, IW_TRAIL, IW_LOOSE, IW_HELD, IW_GONE };

typedef struct {
    uint8_t kind, alive, pacified, flash;
    int16_t hp, maxhp, def;           /* def: its line in KPR_FOES (-1: spawned) */
    float x, y, vx, vy, hx, hy;       /* hx, hy: home */
    int t, state, dir, a, b;
    int16_t carry, owner;             /* a filcher's stolen item / its polyp */
    int16_t child;                    /* a polyp's filcher */
} KprFoe;

enum { SH_TORP, SH_BUSTER, SH_BUBBLE, SH_MISSILE };
typedef struct {
    uint8_t kind, alive, homing;
    float x, y, vx, vy;
    int life, dmg;
} KprShot;

enum { ES_PELLET, ES_CLOUD, ES_INK, ES_SPINE, ES_SPORE, ES_BUBBLE };
typedef struct {
    uint8_t kind, alive;
    float x, y, vx, vy;
    int life;
} KprEShot;

typedef struct { uint8_t alive; float x, y, vy; int fuse; } KprBomb;
typedef struct { uint8_t alive; float x, y, vx, vy; } KprPlate;
typedef struct { float x, y, vx, vy; int life, col; } KprPart;

typedef struct {
    uint8_t on;                       /* in the sea now */
    float x, y, vx, vy;
    int hp;
    int route, state, t, dir, flash;
    float ax, ay, aang;               /* an aim point / angle (laser, lunges) */
    float sx[10], sy[10];             /* a body's segments (the eel) */
    int seen_t;                       /* chasing the sub since */
    int warn_t;                       /* last rumble */
    int dying;                        /* the chain of blasts as it flees */
    float bx[40], by[40];             /* breadcrumbs back to its route */
    int nb;
    float tx, ty;                     /* a tentacle's root, or the laser's aim */
    int tent_t, beam_t;
} KprBoss;

/* ---- what is banked at a dock (and saved) -------------------------------- */
enum { IS_HOME, IS_TAKEN, IS_CARRIED };   /* IS_CARRIED: an egg trailing at the dock */
#define KPR_MAX_BRK 160
typedef struct {
    uint8_t item[KPR_NITEMS];
    uint8_t broken[KPR_MAX_BRK / 8];
    uint16_t have, on;                /* equipment found / fitted */
    int16_t boss_hp[KPR_BOSSES];
    uint8_t boss_dead[KPR_BOSSES];
    uint8_t lights, tanks, boosters, eggs_back;
    uint8_t dock, docks_known;        /* the dock last used; bit per dock reached */
    uint8_t spare[2];
    uint32_t frames, towed;
} KprBank;

typedef struct {
    uint32_t magic;
    uint8_t has_run, gift;
    uint16_t wins, cherries;
    KprBank bank;
    /* records (the title's stats) */
    uint8_t best_tanks, best_boosters, best_eggs, spare;
} KprSave;

/* ---- states ------------------------------------------------------------ */
enum { KS_TITLE, KS_INTRO, KS_PLAY, KS_DOCK, KS_TOWED, KS_ROE, KS_ENDING, KS_CREDITS };

#define KPR_MAX_FOES 128
#define KPR_MAX_SHOTS 24
#define KPR_MAX_ESHOTS 64
#define KPR_MAX_BOMBS 4
#define KPR_MAX_PARTS 96
#define KPR_HIST 64

typedef struct {
    int state, state_t, frame_t, menu;
    KprBank cur;                      /* the dive in progress */
    /* the sea as it is now */
    uint8_t tile[KPR_H][KPR_W];
    uint8_t gates;                    /* lit switches, a bit a gate */
    uint8_t crack[KPR_MAX_BRK];       /* hits and rubbing on cracked walls */
    /* the sub */
    float x, y, vx, vy;
    int face, fuel, fuel_frac, inv, dash_t;
    float dash_dx, dash_dy;
    bool boosting, firing, moving;
    int fire_t, fire_hold, b_hold, b_still, rub_t, area, last_dir_x, last_dir_y;
    bool missile_ready, charge_ready;
    int plates;
    KprPlate plate[KPR_PLATES];
    /* the trail */
    KprTrailObj trail[KPR_TRAIL];
    int trail_n;
    float hx[KPR_HIST], hy[KPR_HIST];
    int hist_i;
    uint8_t where[KPR_NITEMS];
    float ix[KPR_NITEMS], iy[KPR_NITEMS];
    /* creatures, shots */
    KprFoe foe[KPR_MAX_FOES];
    KprShot shot[KPR_MAX_SHOTS];
    KprEShot es[KPR_MAX_ESHOTS];
    KprBomb bomb[KPR_MAX_BOMBS];
    KprPart part[KPR_MAX_PARTS];
    KprBoss boss[KPR_BOSSES];
    float fuel_drop_x[8], fuel_drop_y[8];
    int fuel_drop_t[8];
    float comp_ang;
    int cam_x, cam_y, shake;
    int fight;                        /* the boss fought now (-1) */
    /* messages */
    int msg_t;
    char msg[48];
    uint32_t said;                    /* lines said once a dive */
    /* the dock */
    int dock_phase, dock_cur, dock_lines;
    char dock_line[8][40];
    /* Auntie Roe */
    int roe_page, roe_given;
    /* the ending */
    bool cherry_run;
    int towed_t;
    int idle_t, demo_t;               /* the title's wait for its demo */
    /* tests */
    bool god, no_foes, calm;
    int hits_taken, last_cost, dives;
} KprGame;

extern KprGame kg;
extern KprSave ksv;

/* The demo player runs the game on copies of kg with made-up buttons: then
 * the game reads these instead of the pad, and makes no sound and saves
 * nothing. */
extern bool kpr_simulating;
extern bool kpr_demo;                 /* the title's attract demo: the demo player swims */
extern uint32_t kpr_sim_cur, kpr_sim_prev;
static inline bool kpr_virtual(void) { return kpr_simulating || kpr_demo; }
static inline bool kpr_btn(int m) { return kpr_virtual() ? (kpr_sim_cur & (uint32_t)m) != 0 : btn(m); }
static inline bool kpr_btnp(int m) {
    return kpr_virtual() ? ((kpr_sim_cur & ~kpr_sim_prev) & (uint32_t)m) != 0 : btnp(m);
}
static inline bool kpr_btnr(int m) {
    return kpr_virtual() ? ((~kpr_sim_cur & kpr_sim_prev) & (uint32_t)m) != 0 : btnr(m);
}
void kpr_sfx(const char *name);
void kpr_music(int id);

/* kpr.c */
void kpr_say(const char *m);
void kpr_say_once(int bit, const char *m);
void kpr_save_now(void);
void kpr_play_step(void);
bool kpr_on(int eq);              /* fitted now */
int kpr_slots(void);
int kpr_bars_max(void);
int kpr_completion(void);         /* whole per cent */
int kpr_parts_done(void);         /* of 75 */
void kpr_dock_at(int d);
void kpr_run_dry(void);
void kpr_launch(int d);
void kpr_reset_creatures(void);
void kpr_area_music(void);
void kpr_boss_defeated(int b);
void kpr_final_defeated(void);
void kpr_meet_roe(void);

/* kpr_world.c */
void kpr_world_init(void);
void kpr_build_tiles(void);       /* from the rows and the banked breaks */
int kpr_tile(int tx, int ty);
bool kpr_solid_tile(int tx, int ty);
bool kpr_box_solid(float cx, float cy, int w, int h);
bool kpr_shot_solid(float x, float y);
int kpr_brk_index(int tx, int ty); /* -1: not a breakable */
int kpr_nbrk(void);
bool kpr_brk_banked(int i);
void kpr_break_tile(int tx, int ty);
void kpr_blast(float x, float y, int radius);   /* breaks blocks and sealed walls */
int kpr_area_of(float y);
int kpr_item_at_tile(int tx, int ty);

/* kpr_play.c */
void kpr_sub_update(void);
void kpr_sub_hurt(float fx, float fy, int cost);
void kpr_burn(int units);
void kpr_shots_update(void);
void kpr_items_update(void);
void kpr_trail_update(void);
void kpr_trail_add(int kind, int id);
int kpr_trail_drop_last(void);    /* knocked off: returns the trail slot kind or -1 */
int kpr_trail_count(int kind);
void kpr_bombs_update(void);
void kpr_vents_update(void);
bool kpr_vent_on(int v);
void kpr_burst(float x, float y, int col, int n, float sp);
bool kpr_drilling(void);          /* the Auger and a dash or boost: creatures in the way are hit */
void kpr_camera(bool snap);
void kpr_steal_from_trail(int foe);
void kpr_drop_item_loose(int item, float x, float y);
void kpr_trail_get(int slot, float *x, float *y);
void kpr_reset_history(void);
void kpr_companion_pos(int k, float *x, float *y);
int kpr_companions(void);

/* kpr_foes.c */
void kpr_foes_reset(void);
void kpr_foes_update(void);
void kpr_hurt_foe(int i, int dmg, bool buster);
int kpr_add_eshot(int kind, float x, float y, float vx, float vy);
void kpr_eshots_update(void);
int kpr_spawn_foe(int kind, float x, float y);
int kpr_foes_awake(void);
int kpr_count_foes(int kind, bool pacified);
void kpr_foe_size(int kind, int *w, int *h);

/* kpr_boss.c */
void kpr_bosses_reset(void);      /* from the banked state */
void kpr_bosses_update(void);
void kpr_boss_hurt(int b, int dmg);
bool kpr_boss_box(int b, float *x, float *y, int *w, int *h);
bool kpr_boss_vulnerable(int b);

/* kpr_bot.c */
int kpr_bot_buttons(void);
int kpr_bot_idle(void);
void kpr_bot_reset(void);
extern int kpr_bot_goal, kpr_bot_log;

/* kpr_draw.c */
void kpr_draw(void);
void kpr_draw_label(int x, int y, int w, int h, int t);
int kpr_layout_audit(void);

/* kpr_text.c */
extern const char *const KPR_INTRO[];
extern const int KPR_NINTRO;
extern const char *const KPR_END_GOLD[];
extern const int KPR_NEND_GOLD;
extern const char *const KPR_END_CHERRY[];
extern const int KPR_NEND_CHERRY;
extern const char *const KPR_CREDITS[];
extern const int KPR_NCREDITS;
extern const char *const KPR_ROE_HELLO[];
extern const int KPR_NROE_HELLO;
extern const char *const KPR_ROE_THANKS[];
extern const int KPR_NROE_THANKS;
extern const char *const KPR_ROE_FRIEND[];
extern const int KPR_NROE_FRIEND;

/* art and sound */
enum {
    SPR_SUB0, SPR_SUB1, SPR_TANK, SPR_BOOST, SPR_EGG, SPR_CHARGE, SPR_LIGHT, SPR_PLATE,
    SPR_ROE, SPR_FISH_BLUE, SPR_FISH_RED,
    SPR_POLYP, SPR_FILCHER, SPR_GLIMMER, SPR_GREENLING, SPR_SHELLBACK, SPR_SHELL, SPR_KITERAY, SPR_JELLY,
    SPR_LANTERN, SPR_RISER, SPR_PIPEHORSE, SPR_BLOATER, SPR_STILT, SPR_SUNSTAR, SPR_BILLFISH,
    SPR_BURR, SPR_BURR_ANGRY, SPR_WORM, SPR_PACIFIED,
    SPR_EQ0, SPR_EQ_LAST = SPR_EQ0 + KPR_EQUIP - 1,
    SPR_COUNT
};
extern Sprite kpr_spr[SPR_COUNT];
void kpr_art_load(void);
int kpr_art_check(void);
void kpr_audio_load(void);
extern int KPR_MUS_TITLE, KPR_MUS_INTRO, KPR_MUS_KELP, KPR_MUS_ARCADE, KPR_MUS_BLACK, KPR_MUS_DOCK, KPR_MUS_BOSS,
    KPR_MUS_FINAL, KPR_MUS_ENDING, KPR_MUS_ITEM, KPR_MUS_TOWED;

#endif
