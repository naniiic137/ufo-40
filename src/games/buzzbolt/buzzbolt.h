/* BUZZBOLT - shared declarations. Cartridge 39 of UFO 40.
 * A tribute to Star Waspir (UFO 50 #39); see docs/games/39-buzzbolt.md.
 *
 * A fast, one-hit vertical shooter over the whole wide screen. Tap A for a
 * wide spread at full speed, hold it for focused fire and a slower ship.
 * Every kill drops a letter, always in the order B, Z, Z, B, Z, Z...; each
 * three letters caught make a word: BZZ multiplies the score, ZZZ brings an
 * option, BBB and ZBB are the ship's own specials, and any other word puts
 * the multiplier back to x1. Five waves, each ending in a boss. */
#ifndef BUZZBOLT_H
#define BUZZBOLT_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the field: the whole screen ---------------------------------------- */
#define BZZ_W SCREEN_W
#define BZZ_H SCREEN_H
#define BZZ_HUD_H 10        /* the strip along the top */
#define BZZ_MIN_X 7.0f      /* how far the ship can go */
#define BZZ_MAX_X 313.0f
#define BZZ_MIN_Y 20.0f
#define BZZ_MAX_Y 172.0f

/* ---- the ship --------------------------------------------------------------- */
#define BZZ_FAST 2.0f       /* not firing, or tapping: px a frame */
#define BZZ_SLOW 0.85f      /* fire held: focused, and much slower */
#define BZZ_HOLD_T 10       /* held this long, a press turns into focused fire */
#define BZZ_SPREAD_WIN 18   /* a tap keeps the spread going this long */
#define BZZ_CHARGE_MIN 20   /* the lacewing's lance needs this much charge */
#define BZZ_CHARGE_MAX 150
#define BZZ_HIT_R 2.0f      /* the ship's hitbox, a radius */
#define BZZ_PICK_R 9        /* how near a letter must come to be caught */
#define BZZ_RESPAWN_T 60    /* frames between a loss and the next ship */
#define BZZ_INV_T 120       /* the new ship blinks, safe, this long */
#define BZZ_START_SHIPS 2   /* the ship in play and one spare */

enum { BZ_LACEWING, BZ_SHIELDBUG, BZ_FIREFLY, BZ_SHIPS };

/* ---- letters and words ----------------------------------------------------- */
enum { LT_NONE, LT_B, LT_Z };
enum { W_NONE, W_BZZ, W_ZZZ, W_BBB, W_ZBB, W_WRONG };
#define BZZ_MAX_OPTIONS 2
#define BZZ_MAX_BOMBS 3

/* ---- foes -------------------------------------------------------------------- */
enum {
    EK_GNAT,      /* the basic bug: groups that fire and break away */
    EK_IRONBACK,  /* the big beetle: wave 1's boss pair, wave 3's midbosses */
    EK_MIDGE,     /* popcorn */
    EK_WHIRLER,   /* small spinning seed-copter */
    EK_CRICKET,   /* tougher; fires homing shots */
    EK_BLOATFLY,  /* wave 2's boss */
    EK_BLISTER,   /* red, dense patterns */
    EK_BIGPUFF,   /* a drifting spore-rock, bursts into two */
    EK_PUFF,      /* a small one */
    EK_TICK,      /* wave 3's boss: rings that bounce off the bottom */
    EK_ROTWALL,   /* a wall block; gates switch on and off */
    EK_GOLDBUG,   /* wave 4's golden swarm */
    EK_SCYTHE,    /* wave 4's boss, with... */
    EK_DUSTWING,  /* ...its partner */
    EK_SPOREHEART,/* wave 5's final boss, worth nothing */
    EK_COUNT
};
enum { ROLE_FOE, ROLE_MIDBOSS, ROLE_BOSS };

typedef struct {
    const char *name;
    int points, hp;
    int8_t hw, hh;          /* hitbox half sizes */
} FoeDef;
extern const FoeDef BZZ_FOE[EK_COUNT];

/* formations (how a spawn moves) */
enum {
    FM_DIVE,    /* down from the top, fire, break away to the side */
    FM_COLUMN,  /* a straight line down the screen */
    FM_SIDE,    /* across from one side, weaving */
    FM_ARC,     /* fan out along an arc at the top, hold, leave upward */
    FM_HOVER,   /* fly to a spot, hover and fire, then leave */
    FM_DRIFT,   /* drift down slowly */
    FM_ZIG,     /* zigzag down */
    FM_STREAM,  /* a long loop across the top (the golden swarm) */
    FM_WALL,    /* a row of wall blocks scrolling down */
    FM_BOSS,    /* the wave's boss */
    FM_MIDBOSS,
    FM_PEST     /* wave 5's harrying gnat */
};

typedef struct {
    int16_t t;       /* frames from the start of the wave */
    uint8_t kind, form;
    int16_t x, y;    /* where (formation-specific) */
    int8_t n;        /* how many */
    int8_t dir;      /* -1 / 1: which way it breaks or enters */
    int8_t shots;    /* how many times each fires (0 = never) */
    uint16_t arg;    /* formation-specific: a wall's gap bits, a spacing */
    uint8_t fan;     /* a gnat's or midge's volley: this many shots in a small aimed fan */
} Spawn;

typedef struct {
    const char *name;
    const Spawn *spawns;
    int count;
    int boss_t;      /* frames from the start until the boss comes */
    int par_s;       /* the time bonus counts down from this */
} WaveDef;
#define BZZ_WAVES 5
extern const WaveDef BZZ_WAVE[BZZ_WAVES];

typedef struct {
    float x, y, vx, vy;
    uint8_t kind, form, role, alive;
    uint8_t leaving, state;   /* state: a wall block is 1 on, 0 off */
    int8_t hw, hh;            /* hitbox half sizes */
    int8_t dir, shots;
    int hp, maxhp, t, life, flash;
    int idx;                  /* place in its group */
    int fire_t;               /* next shot, in frames of t */
    int phase, pt, sub;       /* bosses */
    float ax, ay;             /* anchor / target */
    int arg, value;           /* value: points (base) */
    int lance;                /* the last lance that hit it */
    int touch_t;              /* an option's ramming cooldown */
    int fan;                  /* shots in each of its volleys (gnats, midges) */
} Foe;

/* enemy shots */
enum { ES_AIMED, ES_HOMING, ES_RING, ES_BIG, ES_SPORE };
typedef struct {
    float x, y, vx, vy, r;
    uint8_t kind, alive, bounced;
    int t;
} EShot;

/* the player's shots */
enum { PS_BOLT, PS_WAVE, PS_CRESCENT, PS_LANCE, PS_ROCKET, PS_ALLY };
typedef struct {
    float x, y, vx, vy, w, h;
    uint8_t kind, alive;
    int dmg, t, lance, target;
    float wob;
} PShot;

typedef struct { float x, y, vx, vy; uint8_t alive, letter; int t; } Letter;
typedef struct { float x, y; uint8_t alive; int hp, maxhp, fire_cd, target, touch_cd; } Option;
typedef struct { float x, y, vy; uint8_t alive; int t; } Bomb;
typedef struct { float x, y, r; int t; uint8_t alive; } Blast;
typedef struct { float x, y, vx, vy; int life, col; uint8_t kind; } Part;

#define BZZ_MAX_FOES 120
#define BZZ_MAX_ESHOTS 420
#define BZZ_MAX_PSHOTS 160
#define BZZ_MAX_LETTERS 48
#define BZZ_MAX_PARTS 320
#define BZZ_MAX_BLASTS 6

/* the game's states */
enum {
    BS_TITLE, BS_SELECT, BS_BANNER, BS_PLAY, BS_CLEAR, BS_OVER, BS_ENDING, BS_CREDITS, BS_NAME, BS_SCORES,
    BS_INTRO   /* the opening before the title */
};
#define BZZ_INTRO_T 640

#define BZZ_HISCORES 8
typedef struct {
    char name[4];
    uint32_t score;
    uint8_t ship, wave, won, pad;
} HiScore;

/* The cartridge save: the high-score table and the tracked stats. */
typedef struct {
    uint32_t magic;
    uint16_t best_mult, runs;
    uint32_t best[BZ_SHIPS];      /* each ship's best score */
    HiScore hs[BZZ_HISCORES];
    uint16_t wins, cherries;
    uint8_t best_wave, pad[3];
} BzzSave;

/* ---- the whole game's state (buzzbolt_play.c) ------------------------------ */
typedef struct {
    int state, state_t, frame_t;
    int ship;
    int sel;                          /* the ship select cursor */
    /* the run */
    int wave;                         /* 0..4 */
    uint32_t score;
    int mult, best_mult, lives, extends, deaths;
    uint8_t word[3], shown[3];        /* shown: the word just spelt, for the HUD */
    int nword;
    int drop_idx;                     /* letters dropped this run */
    int last_word, word_t;            /* the HUD's flash of the last word */
    int wave_t;                       /* frames since the wave began */
    int spawn_i;                      /* the next spawn in the wave's list */
    bool boss_out, boss_dead;
    int boss_count;                   /* bosses still alive */
    int clear_t;                      /* frames since the last boss fell */
    int time_bonus, bonus_shown, bonus_given;
    int kills, kills_wave, letters_caught;
    int escaped;                      /* foes that left unkilled */
    bool beacon_given;
    /* the ship */
    float px, py;
    bool alive;
    int dead_t, inv, fire_hold, spread_win, spread_cd, focus_cd, charge, volley;
    bool focused;
    int laser_t, laser_len;           /* the firefly's laser: frames on, reach */
    int laser_tick;
    /* the letters' gifts */
    Option opt[BZZ_MAX_OPTIONS];
    int nopt;
    bool orb[2];
    int orb_hp[2], orb_cd[2];         /* bumps into foes an orb can still take */
    float orb_a;
    int power_t;                      /* shieldbug: powered-up frames left */
    int bombs;                        /* firefly: bombs trailing behind */
    Bomb bomb[BZZ_MAX_BOMBS];
    Blast blast[BZZ_MAX_BLASTS];
    struct { bool on; float x, y; int t; } hive;       /* lacewing's big ally */
    struct { bool on; float x, y; int t, fired; } fly; /* firefly's dragonfly */
    int lance_id;
    /* the world */
    Foe foe[BZZ_MAX_FOES];
    EShot es[BZZ_MAX_ESHOTS];
    PShot ps[BZZ_MAX_PSHOTS];
    Letter lt[BZZ_MAX_LETTERS];
    Part part[BZZ_MAX_PARTS];
    float scroll;
    int shake, flash;
    int pest_t;                       /* wave 5: the harrying gnat's return */
    int es_sum[BZZ_WAVES], es_frames[BZZ_WAVES]; /* enemy shots on screen, for the tests */
    /* name entry */
    char name[4];
    int name_pos, name_rank;
    bool won;
    /* testing */
    bool god, sheet;
    int scores_page;
} BzzGame;
extern BzzGame bz;
extern BzzSave bzs;

/* buzzbolt_play.c */
void bzz_new_run(int ship);
void bzz_start_wave(int w);
void bzz_play_update(void);
void bzz_kill_ship(void);
void bzz_add_score(uint32_t pts);
void bzz_catch_letter(int letter);
int bzz_word_of(const uint8_t *w);
int bzz_next_drop(void);
int bzz_foe_value(const Foe *e);
void bzz_hurt_foe(int i, int dmg);
void bzz_kill_foe(int i);
void bzz_boss_gone(void);   /* a boss beaten, or (wave 1's pair only) flown off */
int bzz_add_eshot(int kind, float x, float y, float vx, float vy);
void bzz_aimed(float x, float y, float speed, int n, float spread_deg);
void bzz_ring(float x, float y, float speed, int n, float rot, int kind);
void bzz_eshot_step(EShot *s, float px, float py);
void bzz_burst(float x, float y, int col, int n, float sp);
/* a sound that fires often: played at most once every gap frames */
void bzz_sfx(const char *name, int gap);
void bzz_save_now(void);
void bzz_clear_world(void);
void bzz_launch_bomb(void);
void bzz_word_effect(int w);

/* buzzbolt_foes.c */
int bzz_spawn_foe(int kind, int form, float x, float y, int idx, int dir, int shots, int arg);
void bzz_run_spawns(void);
void bzz_foes_update(void);
void bzz_spawn_boss(int wave);
float bzz_aim(float x, float y);

/* buzzbolt_bot.c: the demo player, for the tests */
int bzz_bot_buttons(void);
extern int bzz_bot_ship;

/* buzzbolt_draw.c */
void bzz_draw(void);
void bzz_draw_label(int x, int y, int w, int h, int t);

/* art and sound */
enum {
    SP_LACEWING, SP_SHIELDBUG, SP_FIREFLY, SP_OPT_LACE, SP_OPT_SHIELD, SP_OPT_FIRE,
    SP_GNAT1, SP_GNAT2, SP_MIDGE1, SP_MIDGE2, SP_WHIRL1, SP_WHIRL2, SP_CRICKET, SP_BLISTER,
    SP_GOLD1, SP_GOLD2, SP_IRONBACK, SP_DRAGONFLY, SP_BOMB,
    SP_COUNT
};
extern Sprite bzz_spr[SP_COUNT];
void bzz_art_load(void);
void bzz_audio_load(void);
extern int BZZ_MUS_TITLE, BZZ_MUS_SELECT, BZZ_MUS_WAVE[BZZ_WAVES], BZZ_MUS_BOSS, BZZ_MUS_FINAL,
    BZZ_MUS_CLEAR, BZZ_MUS_OVER, BZZ_MUS_ENDING, BZZ_MUS_CREDITS, BZZ_MUS_NAME;

extern const char *const BZZ_SHIP_NAME[BZ_SHIPS];

#endif
