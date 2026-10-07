/* RESTLESS - shared declarations. Cartridge 38 of UFO 40.
 * A tribute to Rakshasa (UFO 50 #38); see docs/games/38-restless.md.
 *
 * Old Gaunt, chief of Mossfold, was killed when the Hollow Host burned his
 * village. He will not stay dead. A side-on platformer in three stages of
 * two halves, one hit kills, and there are no lives: every death sends his
 * wisp into the Low Glow to gather pieces of his soul past as many
 * guardians (1, 3, 6, 9, then 12). Each death also makes the world harder
 * and its treasure richer.
 *
 * Everything that moves uses whole numbers (1/256 of a pixel), and every
 * angle comes from a table, so a seed plays the same on every platform. */
#ifndef RSL_H
#define RSL_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include <ctype.h>

/* ---- fixed point and angles ---------------------------------------------- */
#define RSL_FX 256                    /* one pixel */
#define RSL_PX(v) ((v) >> 8)          /* fixed -> whole pixels (floor) */
#define RSL_ANG 64                    /* a full turn */
extern const int16_t RSL_SIN[RSL_ANG];
static inline int rsl_sin(int a) { return RSL_SIN[a & 63]; }
static inline int rsl_cos(int a) { return RSL_SIN[(a + 16) & 63]; }
int rsl_isqrt(int v);
int rsl_dir_to(int dx, int dy);       /* the table angle nearest to (dx, dy) */
int rsl_dist(int dx, int dy);         /* whole-number length */

/* ---- the map ---------------------------------------------------------------- */
#define RSL_TILE 16
#define RSL_SW 20                     /* a screen: 20 x 12 tiles */
#define RSL_SH 12
#define RSL_MAX_SX 16                 /* a half is at most 16 screens wide ... */
#define RSL_MAX_SY 4                  /* ... and 4 high */
#define RSL_MW (RSL_SW * RSL_MAX_SX)
#define RSL_MH (RSL_SH * RSL_MAX_SY)

typedef struct {
    char code;                        /* the letter in the half's layout */
    const char *rows[RSL_SH];
} RslScreen;

/* what a hidden spot gives, in the order the spots come in a half */
typedef struct { uint8_t item, count; } RslSecret;

typedef struct {
    const char *name;                 /* "THE GREENWOOD GATE" */
    int stage, half;                  /* 1..3, 0 = A, 1 = B */
    const char *layout[RSL_MAX_SY + 1]; /* rows of screen letters, '.' = rock, NULL ends */
    const RslScreen *screens;
    int nscreens;
    const RslSecret *secrets;
    int nsecrets;
    const RslScreen *pits[3];         /* the rooms its pits drop you into, left to right */
    int treasure_pit;                 /* index of the treasure room in pits[], -1 none */
    int boss;                         /* BOSS_* fought at the end */
    int lackey_every;                 /* frames between lackeys at no deaths (0 none) */
} RslHalf;

#define RSL_HALVES 6
extern const RslHalf RSL_HALF[RSL_HALVES];

/* ---- rules -------------------------------------------------------------------- */
#define RSL_HALF_TIME (150 * 60)      /* 150 s on the clock for each half */
#define RSL_CLOCK_ADD (30 * 60)
#define RSL_LATE_TIME (20 * 60)       /* revive with 20 s or less: back to 20 */
#define RSL_BONUS_EVERY 5000          /* the next kill after each 5,000 drops a gift */
#define RSL_CHERRY_SCORE 50000
#define RSL_GIFT_REVIVALS 3
#define RSL_WALK 224                  /* 0.875 px a frame, one speed */
#define RSL_JUMP_VY (-1024)           /* one fixed jump, 40 px high */
#define RSL_GRAV 51
#define RSL_MAX_FALL 1280
#define RSL_LAND_LAG 14               /* frozen this long on every landing */
#define RSL_CHARGE_T 40               /* hold the attack this long for a charged shot */
#define RSL_REVIVE_INV 150
#define RSL_SHIELD_INV 90             /* after the owlet takes a hit */
#define RSL_PW 10                     /* Gaunt's body */
#define RSL_PH 22
#define RSL_PH_DUCK 13
#define RSL_SPIRIT_PICK_INV 40        /* safe frames after each soul piece */
#define RSL_HIGH_SCORES 5

/* soul pieces (and guardians) by deaths: 1, 3, 6, 9, 12 */
int rsl_pieces_for(int deaths);

/* ---- weapons ------------------------------------------------------------------ */
enum { WP_STAFF, WP_SEEKER, WP_EMBER, WP_SCATTER, WP_COUNT };
extern const char *const RSL_WEAPON_NAME[WP_COUNT];

/* ---- items -------------------------------------------------------------------- */
enum {
    IT_NONE,
    IT_POT,       /* 100 */
    IT_URN,       /* 200 */
    IT_JAR,       /* 500 (gold jar) */
    IT_COIN,      /* 100 */
    IT_IDOL,      /* 1000 (gold hand) */
    IT_BEETLE,    /* 1000 (gold beetle) */
    IT_CLOCK,     /* +30 s */
    IT_LILY,      /* clears the screen of foes, no drops */
    IT_CHARM,     /* summons a storm cloud worth 1000 */
    IT_EGG,       /* the owlet */
    IT_BELL,      /* calls Grandmother Ash: one death fewer */
    IT_WHEEL,     /* three weapons circling: take one */
    IT_WEAPON,    /* one weapon of a wheel (var = WP_*) */
    IT_ASH,       /* Grandmother Ash rising */
    IT_KINDS
};
int rsl_item_value(int kind);

/* ---- foes --------------------------------------------------------------------- */
enum {
    FO_NONE,
    FO_LACKEY,    /* Fool 6: red blade, green spear thrower */
    FO_BLOOM,     /* Plant 42: three-shot volleys */
    FO_SPITTER,   /* Spewer 20: stars thrown up that rain down */
    FO_HIVE,      /* Nest 24: lets out gnats */
    FO_GNAT,      /* Bug 1 */
    FO_WISP,      /* Siner 6: fours in a wave; all four drop treasure */
    FO_TUMBLER,   /* Monkey 6: drops in, rolls and hops */
    FO_TOAD,      /* Big Frog 10: out of holes, tongue */
    FO_BOULDER,   /* Bouncing Rock 24 */
    FO_CRAB,      /* Patroid 1: stunned, gets up again */
    FO_LEAPER,    /* Fish 1 */
    FO_WEEPER,    /* Dripper 20: hurt only with its eye open */
    FO_SHADE,     /* Karda 6: quick, jumps up, drops through */
    FO_CROW,      /* Spirit 42: volleys, knocked back by shots */
    FO_FIREMAW,   /* Snip 42: volleys of shootable fireballs */
    FO_SHIELD,    /* Tossa 1: hides behind a shield */
    FO_HULK,      /* Brute 80: walks, blocks */
    FO_CLOCKFIRE, /* Time Demon 5 */
    FO_CLOUD,     /* Cloud 42: the charm's summons */
    /* bosses */
    FO_KEEPER,    /* Troll 80 (120 blue) */
    FO_RATTLE,    /* Borkus 150 */
    FO_HEAD,      /* one of Triye's heads, 60 */
    FO_GUST,      /* Winger 180 */
    FO_GASH,      /* Slasher 220 */
    FO_RAMMER,    /* Goruk 150 */
    FO_KING,      /* the Lord 300 */
    FO_SKULL,     /* the Lord's drone */
    FO_KINDS
};
extern const char *const RSL_FOE_NAME[FO_KINDS];
extern const int RSL_FOE_HP[FO_KINDS];
#define RSL_IS_BOSS(k) ((k) >= FO_KEEPER)

enum { BOSS_NONE, BOSS_KEEPER, BOSS_RATTLE, BOSS_TONGUE, BOSS_PAIR, BOSS_RAMMER, BOSS_KING };

typedef struct {
    uint8_t kind, alive, flags, var;  /* var: lackey colour, foe variant */
    int16_t hp, maxhp;
    int x, y, vx, vy;                 /* centre, fixed point */
    int w, h;                         /* hitbox, pixels */
    int t, st, state, dir;            /* dir: +1 right, -1 left */
    int spawn;                        /* the map spawn that made it (-1 none) */
    int flash, inv;
    int a, b, c;                      /* behaviour scratch */
    int ox, oy;                       /* where it was last frame */
} RslFoe;
enum { FF_HARMLESS = 1, FF_GROUNDED = 2, FF_NODROP = 4, FF_GREEN = 8, FF_BLUE = 16, FF_PIT = 32 };

/* foe shots and hazards that hurt */
enum { ES_BALL, ES_SPEAR, ES_STAR, ES_FIRE, ES_TEAR, ES_ROCK, ES_BOMB, ES_FLAME, ES_BOLT, ES_TONGUE, ES_TRAP,
       ES_BLAZE, ES_BLADE, ES_STONE };
typedef struct {
    uint8_t kind, alive, shootable;
    int16_t hp;
    int x, y, vx, vy, grav;
    int w, h, t, life;
    int owner;                        /* the foe it belongs to (blades, tongues) */
} RslEShot;

/* Gaunt's shots */
typedef struct {
    uint8_t kind, alive, charged, split;
    int x, y, vx, vy;
    int dmg, life, t, ang, phase, hover, r;
} RslShot;

typedef struct {
    uint8_t kind, alive, var, landed;
    int x, y, vx, vy;
    int t, value;
    int cx, cy;                       /* a wheel's middle */
    int wheel;                        /* the wheel it belongs to (0 none) */
} RslItem;

typedef struct { int x, y, vx, vy, life, col; } RslPart;

/* things in the map */
enum {
    SP_FOE, SP_TORCH, SP_SECRET, SP_LIFT, SP_ROCKFALL, SP_FIREWALL, SP_LAUNCHER, SP_BUTTON, SP_BOSS, SP_START
};
typedef struct {
    uint8_t type, foe, var, used;     /* used: a torch lit out, a secret found, a foe killed */
    int tx, ty;
    int t, active, child;             /* active: in view; child: the foe it made */
    int a, b;                         /* a lift's travel, a wall's facing, a torch's contents */
} RslSpawn;
enum { TORCH_RANDOM, TORCH_WHEEL, TORCH_EGG };

typedef struct {
    int x, y, px, py;                 /* fixed point, top-left; previous */
    int x0, y0, x1, y1;               /* travel ends */
    int w;                            /* pixels */
    int t, period, vertical;
} RslLift;

#define RSL_MAX_FOES 64
#define RSL_MAX_ESHOTS 96
#define RSL_MAX_SHOTS 24
#define RSL_MAX_ITEMS 64
#define RSL_MAX_PARTS 160
#define RSL_MAX_SPAWNS 256
#define RSL_MAX_LIFTS 12
#define RSL_MAX_PIECES 12
#define RSL_MAX_GUARDS 16

/* ---- the game's states ------------------------------------------------------ */
enum {
    RS_TITLE, RS_STORY, RS_PLAY, RS_DYING, RS_SPIRIT, RS_REVIVE, RS_TALLY, RS_OVER, RS_NAME, RS_ENDING,
    RS_SCORES, RS_CARD
};

typedef struct {
    int x, y, vx, vy;                 /* fixed point; x the middle, y the feet */
    int face;                         /* +1 / -1 */
    bool ground, duck, dropping;
    int air_t, lag, inv, anim, fire_t;
    int charge;                       /* frames the attack has been held */
    bool charging;
    int weapon;
    int lift;                         /* the lift stood on, -1 none */
    int safe_x, safe_y;               /* the last place he stood */
    int jump_dir;                     /* the run carried into a jump */
    int shots_fired;
} RslPlayer;

typedef struct {
    bool on;
    int x, y;                         /* fixed point */
    int t, state, flag_t;
    int target;                       /* the secret it points at (-1 none) */
} RslOwlet;

/* the Low Glow */
typedef struct { int x, y; bool taken; } RslPiece;
typedef struct {
    int x, y, vx, vy;                 /* fixed point */
    int piece;                        /* the piece it guards (-1: a blue flame) */
    int ang, r, speed, t, lunge;
} RslGuard;
typedef struct {
    int x, y;                         /* the wisp, fixed point */
    int n, got;
    RslPiece piece[RSL_MAX_PIECES];
    RslGuard guard[RSL_MAX_GUARDS];
    int nguards;
    int inv, t, done_t;
    bool failed;
} RslSpirit;

typedef struct { uint32_t score; char name[4]; uint8_t stage, won; } RslScoreRow;

/* the cartridge save: records only (a run is never saved) */
typedef struct {
    uint32_t magic;
    RslScoreRow top[RSL_HIGH_SCORES];
    uint16_t runs, wins, most_deaths, most_revivals;
    uint32_t best_won_score;
    uint8_t name[4];                  /* the last initials entered */
} RslSave;

typedef struct {
    int state, state_t, frame_t;
    int menu;
    Rng rng;
    /* the run */
    int half;                         /* 0..5 */
    uint32_t score;
    int deaths, revivals, kills;
    int time;                         /* frames left on the clock */
    int bonus_next;                   /* the next 5,000 mark */
    int bonus_pending;                /* marks passed, waiting for a kill (count) */
    bool won;
    /* the half */
    int mw, mh;                       /* map size in tiles */
    char map[RSL_MH][RSL_MW + 1];
    RslSpawn spawn[RSL_MAX_SPAWNS];
    int nspawn;
    RslLift lift[RSL_MAX_LIFTS];
    int nlift;
    int cam_x, cam_y;                 /* pixels */
    int lock_x, lock_y;               /* a locked camera (boss), -1 none */
    int boss_spawn;
    bool boss_on, boss_dead;
    int boss_done_t;
    int boss_kind;
    int lackey_t, wisp_t, demon_t, quiet_t;
    int section_x, section_y;         /* where a pit sends you back to (pixels, feet) */
    int pit;                          /* in a pit room: its index, -1 none */
    int pit_done_t;
    int secrets_found, half_secrets;
    char msg[48];
    int msg_t;
    /* things */
    RslPlayer pl;
    RslOwlet owl;
    RslFoe foe[RSL_MAX_FOES];
    RslEShot es[RSL_MAX_ESHOTS];
    RslShot shot[RSL_MAX_SHOTS];
    RslItem item[RSL_MAX_ITEMS];
    RslPart part[RSL_MAX_PARTS];
    int shake;
    /* the Low Glow */
    RslSpirit sp;
    /* the tally between halves */
    int tally_bonus, tally_secs;
    /* name entry */
    int name_pos, name_slot;
    char name[4];
    int scores_from;                  /* RS_SCORES returns to the title */
    /* testing */
    bool god, no_spawn, no_timer;
    int last_hurt;                    /* what killed him last (foe kind, 100+ shot kind) */
    int bot_t;
    /* small bookkeeping that belongs to the run (so a copy of rg is the whole game) */
    int flock_kills[16], flock_n;
    int lackeys_made, greens_made;    /* the ground's lackeys this half (tests) */
    int wheel_id;
    int boss_floor_y;
} RslGame;

extern RslGame rg;
extern RslSave rgs;

/* The demo player looks ahead by running the real game on a copy of rg
 * with made-up buttons: while it does, the game reads these instead of the
 * pad, and makes no sound. */
extern bool rsl_simulating;
extern uint32_t rsl_sim_cur, rsl_sim_prev;
static inline bool rsl_btn(int m) { return rsl_simulating ? (rsl_sim_cur & (uint32_t)m) != 0 : btn(m); }
static inline bool rsl_btnp(int m) {
    return rsl_simulating ? ((rsl_sim_cur & ~rsl_sim_prev) & (uint32_t)m) != 0 : btnp(m);
}
void rsl_music(int id);
void rsl_music_stop(void);

/* rsl.c */
void rsl_play_step(void);             /* one frame of play (the demo player's look-ahead uses it) */
void rsl_add_score(int pts);
void rsl_say(const char *m);
void rsl_save_now(void);
void rsl_award_check(void);

/* rsl_world.c: the map, Gaunt, his shots, lifts, hazards */
void rsl_load_half(int h);
void rsl_world_update(void);
bool rsl_solid_px(int x, int y);      /* rock or brick at a pixel */
bool rsl_oneway_px(int x, int y);
bool rsl_floor_px(int x, int y);      /* anything to stand on */
int rsl_ground_below(int x, int y);   /* the first floor at or below y (pixels), -1 none */
char rsl_tile(int tx, int ty);
void rsl_player_hurt(int src);
void rsl_kill_player(int src);
void rsl_clear_screen(bool drops);
void rsl_camera_update(bool snap);
bool rsl_in_view(int x, int y, int margin);
int rsl_shots_alive(int kind);
void rsl_fire(bool charged);
void rsl_shots_update(void);
void rsl_enter_pit(int idx);
void rsl_leave_pit(void);
int rsl_lift_under(int x, int y);
void rsl_burst(int x, int y, int col, int n, int sp);

/* rsl_foes.c */
int rsl_spawn_foe(int kind, int x, int y, int var);
void rsl_foes_update(void);
void rsl_hurt_foe(int i, int dmg, int push);
void rsl_kill_foe(int i, bool drops);
int rsl_add_eshot(int kind, int x, int y, int vx, int vy);
void rsl_eshots_update(void);
void rsl_spawners_update(void);
int rsl_count_foes(int kind);
int rsl_foes_alive(bool bosses);

/* rsl_boss.c */
void rsl_boss_start(void);
void rsl_boss_update(int i);
void rsl_boss_dead(int i);
int rsl_boss_hp_total(void);

/* rsl_items.c */
int rsl_drop_item(int kind, int x, int y, int var);
void rsl_items_update(void);
void rsl_light_torch(int s);
void rsl_find_secret(int s);
void rsl_owlet_update(void);
void rsl_bonus_drop(int foe_kind, int x, int y);
void rsl_kill_drop(int foe_kind, int x, int y, int flags);

/* rsl_spirit.c */
void rsl_spirit_start(void);
void rsl_spirit_update(void);
void rsl_guard_step(RslGuard *g, const RslSpirit *s, int deaths);

/* rsl_bot.c: the demo player, for the tests */
int rsl_bot_buttons(void);
extern int rsl_bot_plan;              /* 0 plays to win, 1 farms deaths early for the cherry */

/* rsl_draw.c */
void rsl_draw(void);
void rsl_draw_label(int x, int y, int w, int h, int t);
int rsl_layout_audit(void);

/* art and sound */
enum {
    SPR_GAUNT0, SPR_GAUNT1, SPR_GAUNT2, SPR_GAUNT_JUMP, SPR_GAUNT_DUCK, SPR_GAUNT_DEAD,
    SPR_WISP, SPR_PIECE, SPR_GUARD, SPR_FLAME,
    SPR_LACKEY0, SPR_LACKEY1, SPR_BLOOM, SPR_SPITTER, SPR_HIVE, SPR_GNAT, SPR_WISPFOE, SPR_TUMBLER,
    SPR_TOAD, SPR_BOULDER, SPR_CRAB, SPR_CRAB_FLIP, SPR_LEAPER, SPR_WEEPER_SHUT, SPR_WEEPER_OPEN,
    SPR_SHADE, SPR_CROW0, SPR_CROW1, SPR_FIREMAW, SPR_SHIELDLING, SPR_SHIELD, SPR_HULK0, SPR_HULK1,
    SPR_CLOCKFIRE, SPR_CLOUD,
    SPR_KEEPER, SPR_RATTLE, SPR_HEAD, SPR_GUST, SPR_GASH, SPR_RAMMER, SPR_SKULL,
    SPR_TORCH0, SPR_TORCH1, SPR_TORCH_OUT,
    SPR_POT, SPR_URN, SPR_JAR, SPR_COIN, SPR_IDOL, SPR_BEETLE, SPR_CLOCK, SPR_LILY, SPR_CHARM, SPR_EGG,
    SPR_BELL, SPR_ASH, SPR_WP_SEEKER, SPR_WP_EMBER, SPR_WP_SCATTER, SPR_WP_STAFF,
    SPR_OWLET0, SPR_OWLET1, SPR_FLAG, SPR_SKULL_ICON,
    SPR_COUNT
};
extern Sprite rsl_spr[SPR_COUNT];
void rsl_art_load(void);
void rsl_audio_load(void);
extern int RSL_MUS_TITLE, RSL_MUS_STORY, RSL_MUS_S1A, RSL_MUS_S1B, RSL_MUS_S2A, RSL_MUS_S2B, RSL_MUS_S3A,
    RSL_MUS_S3B, RSL_MUS_MIDBOSS, RSL_MUS_BOSS, RSL_MUS_CLEAR, RSL_MUS_SPECIAL, RSL_MUS_SPIRIT, RSL_MUS_OVER,
    RSL_MUS_ENDING, RSL_MUS_RISE, RSL_MUS_SCORES;
void rsl_sfx(const char *name);

#endif
