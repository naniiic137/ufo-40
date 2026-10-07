/* DUKES UP - shared declarations. Cartridge 33 of UFO 40.
 * A tribute to Fist Hell (UFO 50 #33); see docs/games/33-dukes-up.md.
 *
 * A belt-scroll brawler. Pick one of four fighters (POWER, RECOV, TOUGH and
 * THROW from 1 to 3), then punch through five nights of ghouls, each with a
 * boss, on one health bar that never refills by itself. Between nights a
 * corner shop sells a full heal or +1 to a stat. Dying offers a continue that
 * restarts the night (cash and stats kept), as often as you like, but only a
 * run with no continue saves Gran. Walk left at the very start of night 1 and
 * you find the gym: endless waves.
 *
 * Every position is fixed point (DKU_FX units a pixel) and every rule uses
 * integer arithmetic only, so a run plays the same on every platform. */
#ifndef DKU_H
#define DKU_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include <ctype.h>

/* ---- units and the screen ------------------------------------------------- */
#define DKU_FX 16
#define DKU_TOP 22           /* the HUD strip */
#define DKU_FLOOR0 108       /* the back edge of the walkable floor (screen y) */
#define DKU_FLOOR1 172       /* the front edge */
#define DKU_FLOORMID 140

static inline int dku_px(int v) { return v >= 0 ? v / DKU_FX : -((-v + DKU_FX - 1) / DKU_FX); }
static inline int dku_fx(int px) { return px * DKU_FX; }

/* ---- the rules (numbers are ours where no source gives them) ------------- */
#define DKU_NIGHTS 5
#define DKU_MAX_HP 100
#define DKU_PRICE_STAT 20    /* +1 POWER, RECOV or TOUGH */
#define DKU_PRICE_HEAL 5     /* back to full health */
#define DKU_SPIN_COST 6      /* only when the spin hits someone */
#define DKU_COMBO_RESET 18   /* frames without a punch and the string starts over */
#define DKU_STUN 28          /* every hit stuns this long */
#define DKU_PSTUN 18         /* a fighter's stun */
#define DKU_REACH_UP 10      /* an attack reaches this far up the floor ... */
#define DKU_REACH_DOWN 3     /* ... and only this far down it */
#define DKU_DBL_TAP 12       /* frames for a double tap */
#define DKU_GYM_GIFT 9       /* waves to survive for the beacon */

/* the stats */
enum { DK_POWER, DK_RECOV, DK_TOUGH, DK_THROW, DK_NSTATS };
/* the four fighters */
enum { DK_ROOK, DK_PIP, DK_MACK, DK_DOLLY, DK_NFIGHTERS };

typedef struct {
    const char *name, *tag, *special;
    uint8_t stat[DK_NSTATS];
    uint8_t skin, hair, top, top2, legs, shoe;
} DkuFighter;
extern const DkuFighter DKU_FIGHTERS[DK_NFIGHTERS];

/* ---- actors: fighters, ghouls, bosses, dogs, passers-by, the saucer ------- */
enum {
    AK_FIGHTER,
    AK_SHAMBLER, /* green fodder; flanks */
    AK_TORCH,    /* fat; lobs firebottles, no melee, goes off after it dies */
    AK_CROW,     /* crow-masked; throws cleavers */
    AK_RAMMER,   /* rushes across the screen */
    AK_HOWLER,   /* pounces; springs up after a knockdown */
    AK_TUSKER,   /* big; guards while idle; body slam */
    AK_SLUDGER,  /* giant slug; spits a poison cloud */
    AK_VISITOR,  /* tall grey; blinks, slow ray, strong combo */
    AK_BULWARK,  /* armoured, masked; guards until the guard breaks */
    AK_GIGGLER,  /* glass cannon; slaps; springs back up */
    AK_SKIPPER,  /* leaps out of the water; fast jab */
    AK_FEELER,   /* a feeler in the planks; stays put */
    AK_UNDERTOW, /* night 4's boss, out in the water */
    AK_GRIST,    /* night 5's boss, two forms */
    AK_DOG,      /* a friend once its leash is broken */
    AK_PASSER,   /* a passer-by */
    AK_SAUCER,   /* flies over; $10 inside */
    AK_COUNT
};

typedef struct {
    const char *name;
    int16_t hp;
    int16_t speed;   /* FX a frame */
    int8_t hw, h;    /* half width and height (px) of the body */
    int8_t reach;    /* melee reach in front (px from the middle) */
    int8_t dmg;      /* its melee hit */
    int8_t windup;   /* frames of warning before it */
    int8_t cash;     /* nothing: there is no score */
} DkuKind;
extern const DkuKind DKU_KINDS[AK_COUNT];

/* actor states */
enum {
    AS_FREE,      /* standing or walking */
    AS_ATTACK,    /* an attack (atk) under way */
    AS_HURT,      /* stunned */
    AS_AIR,       /* jumping (players) */
    AS_DOWN,      /* knocked flying, then lying, then up */
    AS_GRABBED,   /* held by someone */
    AS_GRAB,      /* holding someone */
    AS_THROWN,    /* flying from a throw */
    AS_DODGE,
    AS_SPIN,
    AS_DEAD,      /* dying: knocked away and fading */
    AS_FALL,      /* into a pit */
    AS_DORMANT,   /* feeding, sitting, dancing or asleep until disturbed */
    AS_RISE,      /* out of the ground / the water / from above */
    AS_WINDUP,    /* an enemy's warning */
    AS_RUSH,      /* the rammer's charge, the howler's pounce */
    AS_BLINK,     /* the visitor's blink */
    AS_GUARD,     /* blocking */
    AS_SLAM,      /* the tusker's / Grist's body slam */
    AS_SUPLEX,    /* the fighter's jump with an enemy held */
    AS_ENTER,     /* walking on from off the screen */
    AS_CHANGE,    /* Grist changing form */
    AS_LEASHED,   /* a dog still tied up */
    AS_GONE
};

/* dormant modes */
enum { DM_NONE, DM_FEED, DM_SIT, DM_DANCE, DM_SLEEP };
/* how an enemy comes on */
enum { FROM_LEFT, FROM_RIGHT, FROM_AT, FROM_GROUND, FROM_ABOVE, FROM_WATER, FROM_DOOR, FROM_ROOF };

/* attacks */
enum {
    AT_NONE,
    AT_JAB, AT_KICK, AT_CHARGED, AT_DASH, AT_FLYKICK, AT_SPIN, AT_PUMMEL, AT_SWING, AT_SAW,
    AT_E_HIT,    /* an enemy's ordinary melee hit (damage from its kind) */
    AT_E_RUSH, AT_E_POUNCE, AT_E_SLAM, AT_E_ELBOW, AT_E_BITE,
    AT_COUNT
};
enum { AF_KNOCK = 1, AF_REFLECT = 2, AF_SWEEP = 4, AF_AIR = 8, AF_GUARDBREAK = 16, AF_MOVE = 32 };
typedef struct {
    int8_t startup, active, recover;
    int8_t reach, back;    /* px in front of / behind the middle */
    int8_t zhi;            /* reaches targets up to this high (px) over the attacker's feet */
    int8_t dmg;            /* added to POWER for a fighter's attack */
    uint8_t flags;
} DkuAtk;
extern const DkuAtk DKU_ATK[AT_COUNT];

#define DKU_MAX_HITS 12

typedef struct {
    uint8_t alive, kind, team; /* team 0 the fighters and their dogs, 1 the ghouls, 2 nobody's */
    uint8_t boss, mode, from;
    int x, y, z;               /* FX; y is depth on the floor, z height */
    int vx, vy, vz;
    int face;                  /* -1 or +1 */
    int hp, maxhp;
    int state, st;             /* the state and frames in it */
    int stun;                  /* frames of stun left (AS_HURT) */
    int atk, atk_t;            /* the attack and its frame */
    uint8_t hits[DKU_MAX_HITS];
    int nhits;
    int partner;               /* who we hold / who holds us (-1) */
    int carry;                 /* the item carried (-1) */
    int ready;                 /* frames until a picked-up item is ready */
    int ammo;
    /* enemies */
    int ai_t, cool, side, tx, ty, wake;
    int guard_hits;            /* blows taken on the guard */
    int blinks;                /* the visitor's blinks left */
    int timer2;
    int down_t;                /* how long it lies */
    int last_hitter;
    int thrown_by;             /* who threw it (-1) */
    int thrown_dmg;
    bool exploding;            /* the torch-belly after it dies */
    bool hit_boss;             /* a body flying into the Undertow (once) */
    /* fighters */
    int player;                /* 0 or 1 for a fighter, -1 otherwise */
    int combo, combo_t;        /* the punch string and frames since the last */
    int charge_t;              /* A held */
    bool charged;
    int grab_charge;           /* holding a button in a grab */
    int tapdir, tap_t;         /* double taps */
    bool running;
    int dodge_dir;
    int vert_t;                /* frames moving up or down (resets the string) */
    bool air_jump;             /* PIP: a kick landed, one more jump */
    bool air_kicked;
    int fly_t;
    int hurt_t;                /* flash */
    int spin_paid;
    int land_t;
    int jab_t;                 /* for the bot and the tests: frames since a jab started */
    int step;                  /* walk animation */
    int anim;
} Actor;

#define DKU_MAX_ACTORS 72
#define DKU_P0 0               /* actors 0 and 1 are the fighters */

/* ---- items on the floor ---------------------------------------------------- */
enum {
    IT_NONE,
    IT_APPLE, IT_SANDWICH, IT_DRUMSTICK, IT_ROAST,     /* food */
    IT_COIN, IT_NOTE, IT_RING,                         /* cash: $1, $5, $10 */
    IT_PLANK, IT_CHAIN, IT_PIPE, IT_ARM,               /* swung */
    IT_SCATTER, IT_SAW,                                /* the scattergun (6 shots), the chainsaw */
    IT_BIN, IT_HEAD, IT_CLEAVER, IT_BOTTLE,            /* thrown */
    IT_COUNT
};
typedef struct { const char *name; uint8_t food_pct, cash; uint8_t kind; } DkuItemDef;
enum { IK_FOOD, IK_CASH, IK_SWING, IK_GUN, IK_SAW, IK_THROW };
extern const DkuItemDef DKU_ITEMS[IT_COUNT];

typedef struct {
    uint8_t alive, kind;
    int x, y, z, vx, vz;
    int ammo;
    int t;
} Item;
#define DKU_MAX_ITEMS 48

/* ---- props: things to break -------------------------------------------- */
enum { PR_BIN, PR_BOX, PR_CRATE, PR_STUMP, PR_SIGN, PR_VASE, PR_JUNK, PR_TUFT, PR_POST, PR_COUNT };
typedef struct {
    uint8_t alive, kind, content, content2;
    int x, y;                  /* FX, the foot */
    int hp, shake;
} Prop;
#define DKU_MAX_PROPS 40

/* ---- flying things --------------------------------------------------------- */
enum { SH_BOTTLE, SH_CLEAVER, SH_RAY, SH_PELLET, SH_ITEM, SH_BOMB, SH_GAS, SH_SPIT };
typedef struct {
    uint8_t alive, kind, team, arg;
    int x, y, z, vx, vy, vz;
    int t, dmg, owner;
    bool reflected;
} Shot;
#define DKU_MAX_SHOTS 40

/* ---- hazards ------------------------------------------------------------- */
enum {
    HZ_PIT,        /* a hole: anyone whose feet go in is gone */
    HZ_MINE,       /* a bump in the dirt */
    HZ_LAMP,       /* a chandelier and its shadow */
    HZ_FIRE,       /* a patch of fire (timed) */
    HZ_FIREWALL,   /* a burning barricade (stays) */
    HZ_CLOUD,      /* the slug's poison or Grist's gas */
    HZ_CAR,        /* the car that crashes across */
    HZ_THRESHER,   /* night 3's runaway machine */
    HZ_WATER,      /* where skippers jump from (a pit too) */
    HZ_BEAM,       /* the visitors' beam that lifts you to their ship */
    HZ_DOOR,       /* a section's way on (drawn) */
    HZ_COUNT
};
typedef struct {
    uint8_t alive, kind;
    int x, y, w, h;            /* px, world: x along the street, y on the floor */
    int t, arg, arg2;
    uint8_t hit[(DKU_MAX_ACTORS + 7) / 8]; /* who it has already hit */
} Hazard;
#define DKU_MAX_HAZARDS 40

typedef struct { int x, y, z, vx, vy, vz; int life, col; } Part;
#define DKU_MAX_PARTS 160

/* ---- the nights, written as scripts ---------------------------------------- */
enum {
    EV_SPAWN,      /* a = kind, b = FROM_*, c = dormant mode; x, y (x only for FROM_AT etc.) */
    EV_PROP,       /* a = PR_*, b = content, c = second content */
    EV_ITEM,       /* a = IT_* lying loose */
    EV_LOCK,       /* the camera stops at `at` until nobody is left */
    EV_PIT,        /* x, y, w, h */
    EV_WATER,      /* x, y, w, h */
    EV_MINE,
    EV_LAMP,       /* a chandelier at x, y; b = what it drops */
    EV_FIREWALL,   /* x, y, w, h */
    EV_CAR,        /* the car, in lane y */
    EV_THRESHER,   /* starts behind the camera; stops at x */
    EV_SAUCER,
    EV_DOG,        /* a dog tied to a post at x, y */
    EV_PASSER,     /* a passer-by runs in from the right and is cut down; b = what he drops */
    EV_BOSS,       /* a = kind; x, y */
    EV_WAVE,       /* the lift: a = wave, b = kind, c = how many */
    EV_BEAM,       /* the beam at x */
    EV_STREAM,     /* enemies keep coming during a boss: a = kind, b = every (frames/10), c = most at once */
    EV_DOOR        /* the way on, at x */
};
typedef struct {
    int16_t at;
    uint8_t op, a, b, c;
    int16_t x, y, w, h;
} DkuEvt;

enum { SEC_WALK, SEC_BOSS, SEC_LIFT, SEC_GYM };
enum {
    TH_STREET, TH_CLUB, TH_CANAL, TH_FERRY, TH_WAXWORKS, TH_ORCHARD, TH_GRAVES, TH_SHIP,
    TH_PROM, TH_JETTY, TH_DUNES, TH_HOTEL, TH_LIFT, TH_PENTHOUSE, TH_GYM, TH_COUNT
};
typedef struct {
    const char *name;
    uint8_t theme, kind;
    int16_t len;            /* px */
    const DkuEvt *ev;
    int nev;
} DkuSection;
typedef struct {
    const char *name, *place;
    const DkuSection *sec;
    int nsec;
} DkuNight;
extern const DkuNight DKU_NIGHT[DKU_NIGHTS];
extern const DkuSection DKU_GYM;

/* survival: one wave's line-up */
void dku_gym_wave(int wave, uint8_t *kinds, int *n, int max);

/* ---- game states ----------------------------------------------------------- */
enum {
    DS_TITLE, DS_STORY, DS_SELECT, DS_CARD, DS_PLAY, DS_CLEAR, DS_SHOP, DS_CONTINUE, DS_OVER,
    DS_ENDING, DS_CREDITS, DS_GYMOVER
};

/* the cartridge save: no runs, only records */
typedef struct {
    uint32_t magic;
    uint16_t runs, wins, good_wins;
    uint16_t gym_best1, gym_best2;   /* most waves beaten in the gym */
    uint16_t best_night;             /* the furthest night reached */
    uint16_t fewest_continues;       /* in a won run (0xFFFF none yet) */
    uint32_t total_kos;
} DkuSave;

typedef struct {
    int pick;               /* fighter */
    int cash_unused;
    uint8_t stat[DK_NSTATS];
} DkuPlayerRun;

/* ---- the whole game's state ------------------------------------------------ */
typedef struct {
    int state, state_t, frame_t;
    int menu;                 /* title: 0 one player, 1 two players */
    int players;
    int sel[2];               /* the select screen's cursors */
    bool picked[2];
    /* the run */
    DkuPlayerRun pr[2];
    int cash;                 /* one purse */
    int night;                /* 0..4 */
    int continues;
    bool gym;                 /* in the gym (survival) */
    int gym_wave, gym_break, gym_spawned, gym_left;
    uint8_t gym_queue[24];
    int gym_nq;
    int kos;
    bool won;
    /* the night being played */
    int sec;                  /* section */
    int cam, cam_lock;        /* camera x (px); a lock (-1 none) */
    int next_ev;
    bool ev_done[96];
    int sec_t;
    int go_t;                 /* the GO sign */
    int fade;                 /* section change */
    int boss;                 /* the boss actor (-1) */
    bool boss_down;
    int lift_wave, lift_t;
    int stream_kind, stream_every, stream_max, stream_t;
    int dog_t;                /* the final fight's clock for the dog pack */
    bool dogs_came;
    int start_hp[2];          /* health at the start of the night (for a continue) */
    int thresher_stop;
    bool night_done;          /* the boss is down (or the gym is lost) */
    int gym_push;             /* frames pushing left at the very start */
    int exit_t;               /* frames since the way on opened */
    int dead_t;               /* frames since the last fighter fell */
    Actor a[DKU_MAX_ACTORS];
    Item it[DKU_MAX_ITEMS];
    Prop pr_[DKU_MAX_PROPS];
    Shot sh[DKU_MAX_SHOTS];
    Hazard hz[DKU_MAX_HAZARDS];
    Part part[DKU_MAX_PARTS];
    int shake;
    Rng rng;
    Rng fx;                   /* sparks and dust only: never touches play */
    /* the shop */
    int shop_sel, shop_who;
    const char *shop_msg;
    int shop_msg_t;
    /* the continue screen */
    int cont_sel, cont_t, tip;
    /* the clear screen */
    int clear_cash;
    /* testing */
    bool god, frozen;         /* frozen: enemies don't think */
    int last_dmg, last_hit_kind;
    int dmg_by[32];           /* tests: damage taken by source (kind + 10; -1 blasts, -2 fire, -3 the car,
                                 -4 the thresher, -5 falls, 19 the spin's own cost) */
    int kick_kills;           /* kills that came with the kick (tests) */
    int bot_t;
} DkuGame;

extern DkuGame dku_g;
extern DkuSave dku_sv;

/* dku.c */
void dku_save_now(void);
void dku_sfx(const char *name);
void dku_award_gym(void);

/* dku_world.c: the nights, the camera, spawning, items, props, hazards */
void dku_new_run(int players);
void dku_start_night(int night);
void dku_start_section(int sec);
void dku_start_gym(void);
void dku_world_update(void);
int dku_spawn(int kind, int from, int x, int y, int mode);
int dku_drop_item(int kind, int x, int y);
void dku_explode(int x, int y, int r, int dmg, int src_team, bool hurts_all);
void dku_burst(int x, int y, int z, int col, int n);
bool dku_in_pit(int x, int y);
int dku_hazard_at(int kind, int x, int y);
int dku_enemies_alive(void);
int dku_enemies_awake(void);
void dku_clear_enemies(void);
int dku_floor_lo(void);
int dku_floor_hi(void);
int dku_add_shot(int kind, int team, int x, int y, int z, int vx, int vy, int vz, int dmg, int owner);
void dku_add_hazard(int kind, int x, int y, int w, int h, int t, int arg);
bool dku_section_cleared(void);
int dku_view_left(void);
int dku_view_right(void);

/* dku_play.c: the fighters and the rules of a blow */
void dku_fighters_update(void);
void dku_actor_physics(Actor *a);
void dku_set_state(Actor *a, int s);
void dku_start_attack(Actor *a, int atk);
void dku_attack_update(Actor *a, int idx);
bool dku_hit(int target, int attacker, int atk, int dmg, int dir, uint32_t flags);
void dku_hurt_fighter(int i, int dmg, bool reducible, int dir, bool knock, int src);
void dku_knock(Actor *t, int dir, int power);
void dku_kill(int i, int dir);
int dku_stat(int player, int stat);
int dku_fighter_dmg(const Actor *a, int atk);
void dku_reflect_in(const Actor *a, int idx);
bool dku_in_reach(const Actor *a, const Actor *t, int reach, int back, int zhi);
void dku_throw_actor(int holder, int dir, bool charged);
void dku_fighter_input(int i, uint32_t *held, uint32_t *press, uint32_t *rel);
int dku_fighters_alive(void);

/* dku_foes.c */
void dku_foes_update(void);
void dku_foe_think(int i);
void dku_boss_dead(int i);

/* dku_bot.c: the demo player for the tests */
int dku_bot_buttons(void);
extern int dku_bot_plan;      /* 0 story, 1 the gym */

/* dku_draw.c */
#define DKU_NTIPS 7
extern const char *const DKU_TIPS[DKU_NTIPS];
void dku_draw(void);
void dku_draw_label(int x, int y, int w, int h, int t);
int dku_layout_audit(void);

/* dku_art.c */
enum {
    SPR_HEAD0, SPR_HEAD1, SPR_HEAD2, SPR_HEAD3,   /* the fighters' heads (portraits) */
    SPR_ITEM0,                                    /* items, in IT_ order */
    SPR_PROP0 = SPR_ITEM0 + IT_COUNT,             /* props, in PR_ order */
    SPR_COUNT = SPR_PROP0 + PR_COUNT
};
extern Sprite dku_spr[SPR_COUNT];
void dku_art_load(void);
void dku_draw_actor(const Actor *a, int sx, int sy);
void dku_draw_fighter_big(int f, int x, int y, int t);

/* dku_audio.c */
void dku_audio_load(void);
extern int DKU_MUS_TITLE, DKU_MUS_SELECT, DKU_MUS_N1, DKU_MUS_N2, DKU_MUS_N3, DKU_MUS_N4, DKU_MUS_N5,
    DKU_MUS_BOSS, DKU_MUS_GRIST, DKU_MUS_SHOP, DKU_MUS_GYM, DKU_MUS_CLEAR, DKU_MUS_DOWN, DKU_MUS_GOOD,
    DKU_MUS_BAD, DKU_MUS_LIFT;

#endif
