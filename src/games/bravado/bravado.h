/* BRAVADO - shared declarations. Cartridge 34 of UFO 40.
 * A tribute to Overbold (UFO 50 #34); see docs/games/34-bravado.md.
 *
 * One fixed arena, eight fights. Between fights a shop sells sixteen kinds
 * of gear, and every RAISE puts the next prize up by 100 and adds one more
 * random pack of monsters to it. Fight 1 is a fixed 100; fight 8 is always
 * full (12 packs and the Pit Boss) and pays 3,200. You start with 6 health
 * and most things hit for 6. Hold fire to keep shooting one way while you
 * move; your own bombs hurt you too. */
#ifndef BRAVADO_H
#define BRAVADO_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include <ctype.h>

/* ---- the arena: one fixed screen ---------------------------------------- */
#define BRV_AX0 8           /* the floor inside the walls */
#define BRV_AY0 14
#define BRV_AX1 312
#define BRV_AY1 166
#define BRV_CX 160          /* the middle, where every fight starts */
#define BRV_CY 90
#define BRV_PAD_R 9         /* the corner pads' radius */

/* ---- rules ---------------------------------------------------------------- */
#define BRV_ROUNDS 8
#define BRV_MAX_GROUPS 16   /* RAISE stops at 16 packs ... */
#define BRV_STEP 100        /* ... 100 a pack, so 1,600 */
#define BRV_LAST_GROUPS 12  /* the last fight: 12 packs and the boss */
#define BRV_LAST_PRIZE 3200
#define BRV_RATE_CAP 900    /* the spawn rate stops rising at this prize */
#define BRV_ON_SCREEN 12    /* monsters on the floor at once (lava aside) */
#define BRV_START_HP 6
#define BRV_HIT 6           /* what most things take off */
#define BRV_KEG_HIT 8       /* a powder keg's blast */
#define BRV_LAVA_EVERY 30   /* lava takes 1 health this often: 2.5 s from 6 */
#define BRV_MED_HEAL 6
#define BRV_MED_CAP 40      /* medkits heal past the most health, up to this */
#define BRV_INV_T 60        /* safe frames after a hit */
#define BRV_FUSE 150        /* a bomb goes off 2.5 s after it is dropped */
#define BRV_BLAST_R 30
#define BRV_KEG_R 26
#define BRV_FIZZ_R 20
#define BRV_TAP_WIN 15      /* frames for the second tap of a double tap */
#define BRV_DASH_T 10
#define BRV_DASH_SPEED 5.0f
#define BRV_DASH_CD 24
#define BRV_DRONE_LAG 24    /* the drone does what you did this many frames ago */
#define BRV_DRONE_UP 10     /* it hovers this far over where you were */
#define BRV_BOSS_HP 400
#define BRV_PEEP_WINDUP 170 /* a peeper's eye glows this long before its one shot */
#define BRV_DASH_BOSS 100   /* a dash takes this off the boss: four kill it */
#define BRV_PLAYER_SPEED 1.0f

/* the eight ways: 0 east, then clockwise (screen y grows downwards) */
extern const int8_t BRV_DX[8], BRV_DY[8];

/* ---- the gear: a 4 x 4 grid ----------------------------------------------- */
enum {
    GR_HEART,    /* +4 most health, four tiers */
    GR_MEDKIT,   /* a medkit every 30 s / 15 s */
    GR_BLASTGD,  /* shrug off 1 / 2 blasts a fight */
    GR_SHOTGD,   /* shrug off 1 / 2 shots a fight */
    GR_TRIGGER,  /* faster / even faster fire */
    GR_HEAVY,    /* stronger / even stronger shots */
    GR_FAN,      /* 2 / 3 shots at once */
    GR_RICO,     /* shots bounce off 1 / 2 walls */
    GR_KICK,     /* shots push harder */
    GR_BUDDY,    /* a drone that does what you did; then twice its health */
    GR_FIREWALK, /* cross 1 / 2 lava pools a fight */
    GR_DASH,     /* double tap B: a dash that kills */
    GR_BOMBBAG,  /* 3 more bombs, four tiers */
    GR_CLICKER,  /* B sets your bombs off */
    GR_NAILS,    /* bombs throw nails / more and harder */
    GR_BAIT,     /* monsters go for your bombs */
    GR_COUNT
};
typedef struct {
    const char *name;
    int tiers;
    int price[4];
    const char *desc[4];
} GearDef;
extern const GearDef BRV_GEAR[GR_COUNT];

/* ---- monsters ----------------------------------------------------------------- */
enum {
    MK_MITE,     /* weakest; one shot; chases; 9 a pack */
    MK_GASBAG,   /* floats or follows; splits in two when shot; 9 a pack */
    MK_BRUTE,    /* slow, tough, chases */
    MK_KEG,      /* charges along the eight ways; blows up when it dies */
    MK_STILTER,  /* walks about and shoots with no warning */
    MK_PEEPER,   /* sprouts under you, fires slow shots, sinks, comes back */
    MK_SLAG,     /* not a monster: three more lava pools */
    MK_KINDS,    /* the seven pack kinds */
    MK_BOSS = MK_KINDS,  /* the Pit Boss, last fight only */
    MK_FIZZER,   /* the boss's little runners that blow up */
    MK_ALL
};
extern const char *const BRV_KIND_NAME[MK_ALL];
extern const int BRV_KIND_HP[MK_ALL];

typedef struct {
    float x, y, vx, vy;
    uint8_t kind, alive, size, state;
    int hp, t, st, inv, flash;
    int dir;
    float tx, ty;
    int fire_t;
    int warp;      /* frames still materialising on a pad (harmless) */
    int dash_id;   /* the last dash that hit it (the boss) */
    uint8_t still; /* tests: a monster that stands and never shoots */
} Foe;

enum { ES_PLAIN, ES_SLOW, ES_BOSS };
typedef struct { float x, y, vx, vy; uint8_t alive, kind; int t; } EShot;

enum { PS_GUN, PS_NAIL, PS_DRONE };
typedef struct {
    float x, y, vx, vy;
    uint8_t alive, kind, bounces, owner;
    int dmg, t;
} PShot;

typedef struct { float x, y; int fuse, age; uint8_t alive, owner, drone; } Bomb;
typedef struct { float x, y, r; int t; int dmg; uint8_t alive; } Blast;
#define BRV_BLAST_LINGER 12 /* a blast keeps hurting this many frames */
typedef struct { float x, y, rx, ry; uint8_t alive, cooled; } Pool;
typedef struct { float x, y; int t; uint8_t alive; } Medkit;
typedef struct { float x, y, vx, vy; int life, col; } Part;

#define BRV_MAX_FOES 96
#define BRV_MAX_ESHOTS 96
#define BRV_MAX_PSHOTS 160
#define BRV_MAX_BOMBS 24
#define BRV_MAX_BLASTS 16
#define BRV_MAX_POOLS 24
#define BRV_MAX_MEDS 4
#define BRV_MAX_PARTS 220
#define BRV_MAX_QUEUE 300

/* the drone's memory of what its owner did */
typedef struct { float x, y; uint8_t face, shot, bomb; } Echo;

typedef struct {
    bool on, down;           /* in this run / knocked out this fight */
    float x, y;
    int face;                /* 0..7 */
    bool locked;             /* fire held: the facing stays put */
    int hp, maxhp, inv, hurt_t;
    int cd;                  /* frames until the next shot */
    int lava_t;              /* frames until lava bites again */
    int bombs;               /* left this fight */
    int blast_sh, shot_sh, walks;  /* charges left this fight */
    int tap_t;               /* bomb button: frames since the first tap */
    int fire_tap_t;          /* the fire-button reading of the dash */
    int dash_t, dash_cd, dash_face, dash_id;
    int dashes;
    int fired;               /* presses and auto-fire that shot (tests) */
    bool moving;
    int step;                /* walking animation */
    /* the drone */
    bool drone_on;
    float dx, dy;
    int drone_face, drone_hp, drone_inv, drone_cd;
    Echo echo[BRV_DRONE_LAG];
    int echo_i, echo_n;
    bool shot_now, bomb_now; /* this frame, for the drone's memory */
} Player;

/* a pack in the next fight's lineup */
typedef struct { uint8_t kind, count; } Group;

/* the game's states */
enum {
    BS_TITLE, BS_STORY, BS_BANNER, BS_FIGHT, BS_WON, BS_SHOP, BS_OVER, BS_ENDING, BS_CREDITS
};

/* the cartridge save: no runs are saved, only the records */
typedef struct {
    uint32_t magic;
    uint16_t runs, wins;
    uint16_t most_upgrades, most_kills;
    uint16_t best_fight;     /* the biggest prize won in one fight */
    uint16_t best_round;     /* the furthest fight reached */
    uint32_t best_cash;      /* the most cash at the end of a won run */
    uint32_t total_kills;
} BrvSave;

/* ---- the whole game's state ------------------------------------------------ */
typedef struct {
    int state, state_t, frame_t;
    int menu;                 /* title: 0 one player, 1 two players */
    int players;              /* 1 or 2 */
    /* the run */
    int round;                /* 0..7 */
    int cash;
    int prize;
    Group lineup[BRV_MAX_GROUPS];
    int ngroups;
    int upgrades, kills, hiked_bought;
    bool hike_note;           /* the barker's line, shown once a run */
    uint8_t gear[GR_COUNT];
    int sale, hike;           /* the shop's two changes this visit (-1 none) */
    int last_bought;          /* for the shop's flash */
    /* the shop */
    int shop_row;             /* 0 the buttons, 1 the gear grid */
    int shop_btn;             /* 0 SPEND, 1 RAISE, 2 FIGHT */
    int shop_sel;             /* 0..15 in the grid */
    int shop_msg_t;
    const char *shop_msg;
    char shop_buf[48];        /* a message made up on the spot */
    int said_kind;            /* the pack the last raise added (tests) */
    int raise_flash;
    /* the fight */
    Player p[2];
    Foe foe[BRV_MAX_FOES];
    EShot es[BRV_MAX_ESHOTS];
    PShot ps[BRV_MAX_PSHOTS];
    Bomb bomb[BRV_MAX_BOMBS];
    Blast blast[BRV_MAX_BLASTS];
    Pool pool[BRV_MAX_POOLS];
    Medkit med[BRV_MAX_MEDS];
    Part part[BRV_MAX_PARTS];
    uint8_t queue[BRV_MAX_QUEUE];
    int nqueue, qi;           /* the spawn queue and how far along it */
    int spawn_t;
    int med_t;
    int fight_t;
    int dash_counter;
    bool boss_out, boss_dead;
    int shake;
    int peeper_tone_t;        /* the sprouting tone, for the HUD and tests */
    int fight_kills;
    int won_t;
    bool won;                 /* the run */
    /* testing */
    bool god, no_spawn;
    int hurt_src;             /* what hit last: 100+monster, 200+shot, 300 blast, 400 lava */
    int dash_reading;         /* 0: fire twice on the move (TV Tropes); 1: B twice (the wiki; tests only) */
    int bot_t;
} BrvGame;

extern BrvGame bv;
extern BrvSave bvs;

/* bravado_rules.c: the shop, the lineup and the prices (no drawing) */
int brv_price(int item);              /* this visit's price of the next tier (0 = sold out) */
int brv_base_price(int item, int tier);
bool brv_maxed(int item);
bool brv_buy(int item);               /* true if bought */
void brv_shop_changes(void);          /* pick this visit's sale and hike */
void brv_new_lineup(void);            /* the next fight's base lineup */
bool brv_raise(void);                 /* +100 and one more pack; false at the cap */
void brv_random_group(Group *g);
int brv_group_count(int kind);
int brv_lineup_monsters(void);
int brv_spawn_interval(int prize);
int brv_total_gear_cost(void);
int brv_stock_bombs(void);            /* bombs a fight with this gear */
int brv_max_health(void);

/* bravado_play.c */
void brv_new_run(int players);
void brv_start_fight(void);
void brv_fight_update(void);
void brv_hurt_player(int who, int dmg, int how);
enum { HURT_HIT, HURT_SHOT, HURT_BLAST, HURT_LAVA };
int brv_spawn_foe(int kind, float x, float y);
void brv_kill_foe(int i, int how);
enum { KILL_SHOT, KILL_BLAST, KILL_DASH, KILL_QUIET };
void brv_explode(float x, float y, float r, int dmg_player, bool hurts_player, int boss_dmg);
int brv_drop_bomb(int who, float x, float y, bool drone);
void brv_add_pools(int n);
int brv_alive_foes(bool counted_only);
int brv_enemies_left(void);
bool brv_in_lava(float x, float y);
int brv_pool_at(float x, float y);
void brv_clear_fight(void);
void brv_sfx(const char *name, int gap);
void brv_save_now(void);
int brv_add_eshot(int kind, float x, float y, float vx, float vy);
void brv_burst(float x, float y, int col, int n, float sp);

/* bravado_foes.c */
void brv_foes_update(void);
void brv_foe_init(Foe *f);

/* bravado_bot.c: the demo player, for the tests */
int brv_bot_buttons(void);
extern int brv_bot_plan;              /* 0 plays for the win, 1 for the cherry */

/* bravado_draw.c */
void brv_draw(void);
void brv_draw_label(int x, int y, int w, int h, int t);
int brv_shop_audit(void);

/* art and sound */
enum {
    SP_DICE_D, SP_DICE_U, SP_DICE_S, SP_DICE_D2, SP_DICE_U2, SP_DICE_S2,
    SP_MITE1, SP_MITE2, SP_GAS0, SP_GAS1, SP_GAS2, SP_BRUTE1, SP_BRUTE2, SP_KEG,
    SP_STILT1, SP_STILT2, SP_PEEP, SP_PEEP_BUD, SP_FIZZ, SP_BOMB, SP_MEDKIT, SP_DRONE,
    SP_GEAR0,                 /* sixteen gear icons, in GR_ order */
    SP_SLAG_ICON = SP_GEAR0 + GR_COUNT,
    SP_COUNT
};
extern Sprite brv_spr[SP_COUNT];
void brv_art_load(void);
void brv_audio_load(void);
extern int BRV_MUS_TITLE, BRV_MUS_SHOP, BRV_MUS_FIGHT1, BRV_MUS_FIGHT2, BRV_MUS_LAST, BRV_MUS_BOSS,
    BRV_MUS_WIN, BRV_MUS_OVER, BRV_MUS_ENDING, BRV_MUS_STORY;

#endif
