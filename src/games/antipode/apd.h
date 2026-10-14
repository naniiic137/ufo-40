/* ANTIPODE - shared declarations. Cartridge 29 of UFO 40.
 * A tribute to Vainger (UFO 50 #29); see docs/games/29-antipode.md.
 *
 * The ANTIPODE, a gravity-flipping battle suit, wakes in its cradle on
 * Tallow Station to find the station's systems turned against it. Pressing
 * jump again in the air flips gravity, as often as you like; the ceiling is a
 * real floor. Three sectors (the Gantry, the Brine, the Coil), four ciphers
 * to open the Crown, three pass levels, four mods in three systems (FLIP,
 * FRAME, GUN), armor 75 to 200, two to five spare bodies, and the map only in
 * a cradle.
 *
 * Everything that moves uses whole numbers (1/256 of a pixel). The whole run
 * lives in one struct with no pointers (ag), so the demo player can copy it
 * and look ahead. */
#ifndef APD_H
#define APD_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include <ctype.h>

/* ---- fixed point -------------------------------------------------------- */
#define APD_FX 256
#define APD_PX(v) ((v) >> 8)
/* a 32-step circle, 256 = 1 */
extern const int16_t APD_CIRC[32];
static inline int apd_sin32(int a) { return APD_CIRC[a & 31]; }
static inline int apd_cos32(int a) { return APD_CIRC[(a + 8) & 31]; }

/* ---- the world ------------------------------------------------------------ */
#define APD_TILE 16
#define APD_CW 20                 /* a cell (one screen) is 20 x 10 tiles */
#define APD_CH 10
#define APD_CPW (APD_CW * APD_TILE) /* 320 */
#define APD_CPH (APD_CH * APD_TILE) /* 160 */
#define APD_HUD 20                /* the play view sits under a 20-px bar */
#define APD_RMAX 3                /* a room is at most 3 cells a side ... */
#define APD_RTW (APD_CW * APD_RMAX)
#define APD_RTH (APD_CH * APD_RMAX)
#define APD_GW 9                  /* a sector's grid of cells */
#define APD_GH 7

enum { SEC_GANTRY, SEC_BRINE, SEC_COIL, SEC_CROWN, APD_SECTORS };
extern const char *const APD_SECTOR_NAME[APD_SECTORS];

#define APD_MAX_ROOMS 112
#define APD_MAX_DOORS 192
#define APD_MAX_ITEMS 40
#define APD_MAX_PODS 16
#define APD_MAX_TERMS 24

/* tiles */
enum {
    T_AIR, T_WALL, T_SPUP, T_SPDN, T_PLAT, T_ACID, T_CRACK, T_BARREL, T_DOOR, T_LOCK, T_SHUT, T_PROP,
    T_FAKE, T_ZAP, T_GREY
};
/* what sits on an edge between two rooms */
enum {
    E_WALL, E_DOOR, E_PASS1, E_PASS2, E_PASS3, E_BOSS, E_NANO, E_BARREL, E_CRACK, E_ZAP, E_TARGET, E_HIDDEN
};

typedef struct {
    char id;
    uint8_t sector, gx, gy, w, h; /* bounding cells; rooms are rectangles */
    int16_t spec;                 /* index in APD_SPECS */
    int16_t first_item, nitems;   /* its items in the item list */
} ApdRoom;

typedef struct {
    uint8_t sector, type, vertical;
    uint8_t ax, ay;               /* cell a; b is right of it (or below) */
    int16_t ra, rb;               /* rooms */
} ApdDoor;

/* the room table: one line a room */
typedef struct {
    uint8_t sector;
    char id;
    const char *tpl;              /* a template letter per cell, row by row */
    uint16_t flags;
    const char *things;           /* what's in it (see apd_world.c) */
    const char *name;
} ApdSpec;
enum { RF_HEAT = 1, RF_SPIKES = 2, RF_ACID = 4, RF_DARK = 8, RF_ARENA = 16, RF_START = 32 };

/* items */
enum { IT_ARMOR, IT_SPARE, IT_DAMPER, IT_MOD };
typedef struct { uint8_t kind, var; int16_t room; } ApdItemDef;
#define APD_ARMOR_CELLS 25
#define APD_SPARE_PICKS 3
#define APD_DAMPERS 2
#define APD_UPGRADES (APD_ARMOR_CELLS + APD_SPARE_PICKS + APD_DAMPERS + 4)

extern const ApdSpec APD_SPECS[];
extern const int APD_NSPECS;
extern ApdRoom apd_rooms[APD_MAX_ROOMS];
extern int apd_nrooms;
extern ApdDoor apd_doors[APD_MAX_DOORS];
extern int apd_ndoors;
extern ApdItemDef apd_items[APD_MAX_ITEMS];
extern int apd_nitems;
extern int16_t apd_cell[APD_SECTORS][APD_GH][APD_GW]; /* room index, -1 none */
extern int apd_npods, apd_nterms;
void apd_world_init(void);
int apd_room_find(int sector, char id);
int apd_door_at(int sector, int ax, int ay, int vertical); /* -1 none */

/* ---- mods ------------------------------------------------------------------- */
enum { MOD_EMBER, MOD_TWIN, MOD_VOLT, MOD_MASS, APD_MODS };
enum { SYS_OFF, SYS_FLIP, SYS_FRAME, SYS_GUN, APD_SYS };
#define MOD_NONE 255              /* not found yet */
extern const char *const APD_MOD_NAME[APD_MODS];
extern const char *const APD_SYS_NAME[APD_SYS];
extern const char *const APD_EFFECT_NAME[APD_MODS][APD_SYS];

/* ---- rules ---------------------------------------------------------------------- */
#define APD_ARMOR0 75
#define APD_ARMOR_STEP 5
#define APD_SPARES0 2
#define APD_WALK 384              /* 1.5 px a frame */
#define APD_GRAV 64               /* 0.25 px a frame per frame */
#define APD_FALL 1152             /* 4.5 px a frame at most */
#define APD_JUMP 1376             /* about 58 px high with the button held */
#define APD_JUMP_CUT 384          /* let go early: the rise is cut to this */
#define APD_STOMP 2048            /* the anvil drop, 8 px a frame */
#define APD_PW 10
#define APD_PH 20
#define APD_PH_DUCK 13
#define APD_FIRE_EVERY 8
#define APD_SHOT_SPEED 1280       /* 5 px a frame */
#define APD_SHOT_LIFE 22          /* 110 px; REACH doubles it */
#define APD_SHOT_DMG 5
#define APD_MERCY 60
#define APD_SPIKE_DMG 100
#define APD_FLIP_DMG 50           /* STORMBALL and ANVIL */
#define APD_BOSS_REGAIN 250
#define APD_THUNDER_FLIPS 3
#define APD_SHADOW_LIFE 240

/* ---- foes ------------------------------------------------------------------------ */
enum {
    FO_NONE,
    FO_TREAD,     /* grey treadle: patrols a floor or a ceiling */
    FO_TREAD_R,   /* red: tougher, quicker */
    FO_TREAD_G,   /* gold: tougher still, quicker */
    FO_SPITTER,   /* a turret of slow energy balls */
    FO_STAREYE,   /* the big red eye: star bursts */
    FO_HIVE,      /* the blue hivebox: lets out mites */
    FO_MITE,
    FO_SKIP,      /* skipjack: jumps from floor to ceiling and back */
    FO_SKIMMER,   /* flies in a wave */
    FO_SENTRY,    /* the sentry orb: all gone, the nano-wall opens */
    /* bosses and their parts */
    FO_SCRAPPER,  /* mini, the Gantry */
    FO_WARDEN,    /* the Gantry: cipher 1 */
    FO_BRACKISH,  /* the Brine: cipher 2 */
    FO_LAMPREY,   /* mini, the Coil */
    FO_LAMP_SEG,
    FO_COILHEART, /* the Coil: cipher 3 */
    FO_NODE,
    FO_OBVERSE,   /* the Brine, north-east: cipher 4 */
    FO_HALLORAN,  /* the Crown: three phases */
    FO_HAL_LEG,
    FO_HAL_JOINT,
    FO_KINDS
};
extern const char *const APD_FOE_NAME[FO_KINDS];
#define APD_IS_BOSS(k) ((k) >= FO_SCRAPPER)
enum { BS_SCRAPPER, BS_WARDEN, BS_BRACKISH, BS_LAMPREY, BS_COILHEART, BS_OBVERSE, BS_HALLORAN, APD_BOSSES };
extern const char *const APD_BOSS_NAME[APD_BOSSES];
extern const int APD_BOSS_HP[APD_BOSSES];
#define APD_HAL_PHASES 3
extern const int APD_HAL_HP[APD_HAL_PHASES];

typedef struct {
    uint8_t kind, alive, up, flags;  /* up: walks on the ceiling */
    int16_t hp, maxhp;
    int x, y, vx, vy;                 /* centre, fixed point */
    int16_t w, h;
    int t, state, a, b, c, dir;
    int16_t owner;                    /* a part passes its harm to this foe (-1 none) */
    int16_t flash;
    uint16_t hit_flip;                /* the flip that last hit it (STORMBALL / ANVIL hit once a flip) */
    int16_t latch_t;
} ApdFoe;
enum { FF_SHIELD = 1, FF_NOHIT = 2, FF_REFLECT = 4, FF_HIDDEN = 8 };

enum { ES_BALL, ES_STAR, ES_BOLT, ES_BUBBLE, ES_BEAM, ES_ORB, ES_RING, ES_LIGHTNING, ES_SHELL, ES_SPARK };
typedef struct {
    uint8_t kind, alive;
    int16_t dmg;
    int x, y, vx, vy, grav;
    int16_t w, h;
    int life, t;
} ApdEShot;

enum { PS_GUN, PS_WAVE, PS_DRONE, PS_SHADOW };
typedef struct {
    uint8_t kind, alive, zap, hot;
    int x, y, vx, vy;
    int life, dmg;
} ApdShot;

/* things in a room */
enum { TH_POD, TH_TERM, TH_TELE, TH_ITEM, TH_BALL, TH_TARGET, TH_BUTTON, TH_EMITTER };
typedef struct {
    uint8_t kind, var, alive, flag;
    int16_t tx, ty;                   /* tile (its middle) */
    int16_t id;                       /* item / pod / terminal / door / link */
    int t;
} ApdThing;

typedef struct { int x, y, vx, vy, life, col; } ApdPart;

#define APD_MAX_FOES 40
#define APD_MAX_ESHOTS 64
#define APD_MAX_SHOTS 32
#define APD_MAX_THINGS 24
#define APD_MAX_PARTS 96

typedef struct {
    int x, y, vx, vy;                 /* centre, fixed point */
    int grav;                         /* +1 falls down, -1 falls up */
    int face;
    bool ground, crouch, stomp, jumping;
    int aim;                          /* 0 ahead, 1 over the head, 2 under the feet */
    int fire_t;
    bool firing;                      /* A held: the facing is locked */
    int inv, armor;
    int air_flips, thunder_t;
    uint16_t flip_id;
    int anim, heat_t, acid_t, leech;
    int entry_x, entry_y, entry_grav, entry_face;
    int shots_fired, flips;
} ApdPlayer;

typedef struct { bool on; int x, y, face, aim, grav, t; } ApdShadow;
typedef struct { bool on; int x, y, ang, latch, t; } ApdDrone;

/* ---- states ---------------------------------------------------------------------- */
enum {
    AS_TITLE, AS_CODE, AS_INTRO, AS_PLAY, AS_POD, AS_TEXT, AS_TELE, AS_DYING, AS_OVER, AS_ENDING, AS_CREDITS
};

/* everything a run keeps (and the save holds) */
typedef struct {
    uint8_t item_got[APD_MAX_ITEMS];
    uint8_t gate_open[APD_MAX_DOORS];
    uint8_t visited[APD_MAX_ROOMS];
    uint8_t boss_dead[APD_BOSSES];
    uint8_t mod_sys[APD_MODS];        /* SYS_* or MOD_NONE */
    uint8_t dampers, spares_max, cells, pass, ciphers, secret_open;
    uint8_t code_on, won;
    int16_t pod;                      /* the cradle last saved at (-1: the start) */
    int16_t room;                     /* its room */
    uint32_t deaths, frames;
} ApdRun;

typedef struct {
    uint32_t magic;
    uint8_t has_run;
    ApdRun run;
    /* records */
    uint16_t best_upgrades, best_explore, wins, cherries;
    uint32_t times_died;
    uint8_t gift;
} ApdSave;

typedef struct {
    int state, state_t, frame_t, menu;
    ApdRun run;
    int spares;                       /* spare bodies left now */
    /* the room */
    int room, rw, rh;                 /* room index, size in tiles */
    uint8_t tile[APD_RTH][APD_RTW];
    uint8_t tdoor[APD_RTH][APD_RTW];  /* door index + 1 on door tiles */
    int cam_x, cam_y;
    ApdPlayer pl;
    ApdShadow shadow;
    ApdDrone drone;
    ApdFoe foe[APD_MAX_FOES];
    ApdEShot es[APD_MAX_ESHOTS];
    ApdShot shot[APD_MAX_SHOTS];
    ApdThing th[APD_MAX_THINGS];
    int nth;
    ApdPart part[APD_MAX_PARTS];
    int shake;
    int pod_zone;                     /* the cradle he stands by (-1) */
    int msg_t;
    char msg[40];
    /* a boss fight */
    int boss, boss_foe, boss_phase, boss_t;
    bool boss_on, boss_lock;
    int boss_dead_t;
    int lightning_t;                  /* the Crown's last phase: shots are deleted */
    /* buttons of the secret room */
    int button_t[2];
    /* the cradle menu */
    int pod_page, pod_mod, pod_sector;
    /* terminal text */
    int text_id, text_page;
    /* teleport */
    int tele_to, tele_t;
    /* the code screen */
    char code[9];
    int code_pos;
    bool code_armed;
    /* ending */
    bool cherry_run;
    /* tests */
    bool god, no_foes;
    int hits_taken, last_hurt;
    int bot_t;
} ApdGame;

extern ApdGame ag;
extern ApdSave ags;

/* The demo player looks ahead by running the real game on a copy of ag with
 * made-up buttons: while it does, the game reads these instead of the pad,
 * and makes no sound and saves nothing. */
extern bool apd_simulating;
extern uint32_t apd_sim_cur, apd_sim_prev;
static inline bool apd_btn(int m) { return apd_simulating ? (apd_sim_cur & (uint32_t)m) != 0 : btn(m); }
static inline bool apd_btnp(int m) {
    return apd_simulating ? ((apd_sim_cur & ~apd_sim_prev) & (uint32_t)m) != 0 : btnp(m);
}
void apd_sfx(const char *name);
void apd_music(int id);

/* apd.c */
void apd_say(const char *m);
void apd_save_now(void);
void apd_play_step(void);
void apd_enter_pod(void);
void apd_read_terminal(int id);
void apd_start_teleport(int link);
void apd_lose_body(void);
int apd_explore_pct(void);
int apd_items_got(void);
int apd_items_pct(void);
int apd_safe_slots(void);
int apd_sys_count(int sys);
bool apd_has(int mod, int sys);
bool apd_overload(int sys);
void apd_pickup(int item);
void apd_pod_save(int pod);

/* apd_world.c: building a room, tiles */
void apd_build_room(int room);
bool apd_solid_tile(int tx, int ty, int grav, int dir_y);
int apd_tile_at(int px, int py);
bool apd_box_solid(int cx, int cy, int w, int h, int grav, int vy); /* pixels, centre */
void apd_open_gate(int door);
void apd_lock_boss_doors(bool lock);
void apd_open_nano(void);
int apd_room_cell_x(int px);
int apd_room_cell_y(int py);
const char *apd_room_name(int room);
int apd_room_pod(int room);       /* the room's cradle id or -1 */
void apd_room_dump(int room);     /* tests: the room's tiles as text */

/* apd_play.c: the suit, its shots, the mods, harm */
void apd_player_update(void);
void apd_player_hurt(int dmg, int src, bool spikes);
void apd_shots_update(void);
void apd_hazards_update(void);
void apd_things_update(void);
void apd_camera(bool snap);
void apd_burst(int x, int y, int col, int n, int sp);
void apd_room_change_check(void);
int apd_shot_range(void);
void apd_respawn_here(void);

/* apd_foes.c */
int apd_spawn_foe(int kind, int x, int y, bool up);
void apd_foes_update(void);
void apd_hurt_foe(int i, int dmg, bool from_player);
int apd_add_eshot(int kind, int x, int y, int vx, int vy, int dmg);
void apd_eshots_update(void);
int apd_foes_alive(void);
int apd_count_foes(int kind);

/* apd_boss.c */
void apd_boss_spawn(int boss);    /* when he walks in */
void apd_boss_update(int i);
void apd_boss_hurt(int i, int dmg);
void apd_boss_regain(void);
int apd_boss_hp(void);
int apd_boss_maxhp(void);
void apd_boss_check_trigger(void);

/* apd_bot.c: the demo player */
int apd_bot_buttons(void);
extern int apd_bot_goal;           /* the goal it is working on */
extern int apd_bot_plan;           /* 0: the whole game, 100% */

/* apd_draw.c */
void apd_draw(void);
void apd_draw_label(int x, int y, int w, int h, int t);
int apd_layout_audit(void);

/* apd_text.c: the words */
extern const char *const APD_INTRO[];
extern const int APD_NINTRO;
extern const char *const APD_LOG[][6];
extern const int APD_NLOGS;
extern const char *const APD_END_GOLD[];
extern const int APD_NEND_GOLD;
extern const char *const APD_END_CHERRY[];
extern const int APD_NEND_CHERRY;
extern const char *const APD_CREDITS[];
extern const int APD_NCREDITS;
#define APD_CODE "LOAD-KITS"

/* art and sound */
enum {
    SPR_SUIT0, SPR_SUIT1, SPR_SUIT2, SPR_SUIT_JUMP, SPR_SUIT_DUCK, SPR_SUIT_UP,
    SPR_TREAD0, SPR_TREAD1, SPR_SPITTER, SPR_HIVE, SPR_SKIP, SPR_SKIMMER,
    SPR_ARMOR, SPR_SPARE, SPR_DAMPER, SPR_MOD, SPR_POD, SPR_TERM,
    SPR_COUNT
};
extern Sprite apd_spr[SPR_COUNT];
void apd_art_load(void);
int apd_art_check(void);
void apd_audio_load(void);
extern int APD_MUS_TITLE, APD_MUS_INTRO, APD_MUS_GANTRY, APD_MUS_BRINE, APD_MUS_COIL, APD_MUS_CROWN, APD_MUS_POD,
    APD_MUS_BOSS, APD_MUS_FINAL, APD_MUS_ENDING, APD_MUS_CREDITS, APD_MUS_ITEM, APD_MUS_OVER;

#endif
