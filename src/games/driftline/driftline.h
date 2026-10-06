/* DRIFTLINE - shared declarations. Cartridge 48 of UFO 40.
 * A tribute to Seaside Drive (UFO 50 #48); see docs/games/48-driftline.md.
 *
 * A red convertible on the coast road, locked to the bottom of the screen
 * while the world scrolls by. Driving moves you and swings the gun: B
 * fires a stream up into a 90-degree cone, A fires side pairs along the
 * road. The charge meter (grey, green, red) sets the damage; it drains
 * slowly and only fills while you drive left. One hit loses a car. Four
 * stages, day to sunset to night to the open sea, each with a boss; a
 * stage cleared without losing a car leads to a bonus stage of blocks,
 * whose coin is worth two more cars. 1P, or 2P together. */
#ifndef DRIFTLINE_H
#define DRIFTLINE_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the screen ---------------------------------------------------------- */
#define DFL_HUD_H 10          /* the score strip along the top */
#define DFL_CAR_TOP 150       /* the car's top edge (where its gun is) */
#define DFL_ROAD_Y 162        /* where its wheels touch */
#define DFL_ROAD_TOP 146      /* the road's far edge */
#define DFL_ROAD_BOT 168      /* the road's near edge; the meter strip below */
#define DFL_MIN_X 12.0f
#define DFL_MAX_X 308.0f
#define DFL_SCROLL 1.0f       /* the world going by, px a frame */

/* ---- the car --------------------------------------------------------------- */
#define DFL_SPEED 2.0f        /* top speed, px a frame */
#define DFL_ACCEL 0.25f
#define DFL_FRICTION 0.2f
#define DFL_HW 9              /* hitbox half width */
#define DFL_HIT_TOP 153       /* hitbox: from here down to DFL_ROAD_Y */
#define DFL_AIM_MAX 45.0f     /* the gun swings within 45 degrees of straight up */
#define DFL_AIM_SWING 2.0f    /* degrees a frame while steering */
#define DFL_AIM_CENTRE 3.0f   /* degrees a frame back to straight up while UP is held */
#define DFL_MAIN_CD 5         /* frames between main-gun shots */
#define DFL_SIDE_CD 8         /* frames between side pairs */
#define DFL_SHOT_SPEED 6.0f
#define DFL_RESPAWN_T 70      /* frames from a loss to the next car */
#define DFL_INV_T 120         /* the new car blinks, safe, this long */
#define DFL_START_SPARE 3     /* cars in reserve at the start (four in all) */

/* ---- the charge meter --------------------------------------------------------- */
#define DFL_METER_MAX 720
#define DFL_TIER1 240         /* grey below this */
#define DFL_TIER2 480         /* green below this, red from here */
#define DFL_FILL 12           /* a frame of drifting left */
#define DFL_DRAIN 1           /* a frame of anything else */
#define DFL_DRIFT_VX -0.3f    /* moving left at least this fast counts as a drift */
extern const int DFL_TIER_DMG[3]; /* 1, 2, 4 */

/* ---- foes -------------------------------------------------------------------- */
enum {
    /* stage 1, Harbour Road */
    FK_KITE,      /* biplane: loiters, fires if left alone */
    FK_BUZZER,    /* small drone: bursts if left alone */
    FK_HOG,       /* a car on the road; a hit leaves... */
    FK_WRECK,     /* ...a burning wreck, which is what scores */
    FK_ROTOR,     /* helicopter */
    /* stage 2, Sundown Strip */
    FK_SHARD,     /* square: shoot it and it bursts into shrapnel */
    FK_PRISM,     /* diamond: a beam that walks inward; hits push it back */
    FK_HOOP,      /* bouncing ring */
    FK_TUMBLER,   /* spinning cube on the road: side gun pops it up */
    /* stage 3, Moonlit Mile */
    FK_SKULL,     /* floats at you */
    FK_SHEET,     /* ghost: fires at 45 degrees down */
    FK_SNAPPER,   /* chaser: blue, purple, then it bursts */
    FK_SLAB,      /* stone face that drops onto the road */
    FK_SAUCER,    /* the Beamdown saucer, passing through */
    /* stage 4, Open Water */
    FK_JELLY,
    FK_DARTFISH,  /* zigzags down to the road, then chases */
    FK_SQUIRT,    /* octopus: scoots and fires three behind */
    FK_SHARK,     /* leaps over the road */
    /* boss leftovers */
    FK_PLANET,    /* an orbiter knocked off the Orrery: bounces, juggled by shots */
    FK_COUNT
};

typedef struct {
    const char *name;
    int points, hp;
    int8_t hw, hh;     /* hitbox half sizes */
    uint8_t ground;    /* drives on the road: only the side guns reach it */
} DflFoeDef;
extern const DflFoeDef DFL_FOE[FK_COUNT];

typedef struct {
    uint8_t alive, kind, ground, state, still; /* still: a test dummy that does nothing */
    float x, y, vx, vy;
    float ax, ay;          /* an anchor */
    int hp, t, sub, dir, arg, flash;
    int fire_t;
} DflFoe;

/* a stage's spawn list: each entry brings n foes of a kind, gap frames apart */
typedef struct {
    int16_t t;       /* frames from the start of the stage */
    uint8_t kind;
    int16_t x, y;    /* where (kind-specific) */
    int8_t n, dir;   /* how many; -1 / 1: from which side, or which way */
    int16_t gap;     /* frames between them */
    int16_t arg;     /* kind-specific */
} DflSpawn;

typedef struct {
    const char *name;
    const DflSpawn *spawns;
    int count;
    int boss_t;      /* frames from the start until the boss comes */
} DflStage;
#define DFL_STAGES 4
extern const DflStage DFL_STAGE[DFL_STAGES];

/* stage 4's swells: a whale breaches out at sea, its wave rolls in and
 * breaks over a stretch of road */
typedef struct { int16_t t, x; } DflSwellCue;
extern const DflSwellCue DFL_SWELLS[];
extern const int DFL_SWELL_COUNT;
#define DFL_SWELL_T 100       /* frames from the breach to the break */
#define DFL_SWELL_HIT 24      /* the frames the break covers the road */
#define DFL_SWELL_HW 22       /* half the stretch it covers */
#define DFL_SWELL_LOOP 9000   /* the swells' list runs this long, then again */

/* the bonus stages' blocks: 12 columns, up to 7 rows */
#define DFL_BCOLS 12
#define DFL_BROWS 7
#define DFL_BLOCK_W 24
#define DFL_BLOCK_H 9
#define DFL_BLOCK_X0 16
#define DFL_BLOCK_Y0 22
#define DFL_BLOCK_HP 12
extern const char *const DFL_BONUS_MAP[3][DFL_BROWS];

/* ---- shots ----------------------------------------------------------------------- */
enum { ES_PELLET, ES_SHRAPNEL, ES_BOMB, ES_BUBBLE, ES_FLARE };
typedef struct {
    float x, y, vx, vy, r, g;
    uint8_t alive, kind;
    int t;
} DflEShot;

typedef struct {
    float x, y, vx, vy;
    uint8_t alive, side, owner;
    int dmg;
} DflPShot;

typedef struct { float x, y, vx, vy; int life, col; } DflPart;
typedef struct { int x, t; uint8_t alive; } DflSwell;

#define DFL_MAX_FOES 64
#define DFL_MAX_ESHOTS 200
#define DFL_MAX_PSHOTS 96
#define DFL_MAX_PARTS 240
#define DFL_MAX_SWELLS 6

/* ---- the bosses ------------------------------------------------------------------ */
enum { BOSS_ZEPHYR, BOSS_ORRERY, BOSS_MOON, BOSS_CRAB };
extern const int DFL_BOSS_POINTS[DFL_STAGES];
#define DFL_TURRET_HP 80
#define DFL_ORRERY_HP 600
#define DFL_ORBITER_HP 10
#define DFL_MOON_HP 800
#define DFL_CRAB_HP 800
typedef struct {
    bool on, dead;
    int kind, t, phase, pt, hp, maxhp, flash, dead_t;
    float x, y, vx;
    /* the airship's turrets, the orrery's orbiters, the crab's legs */
    int part_hp[6];
    int part_t[6];       /* a leg's telegraph and slam clock */
    float orbit_a;
    /* the moon's hand */
    float hx, hy;
    int hand_state, hand_t, attack;
    bool revealed;
} DflBoss;

/* ---- the cars ------------------------------------------------------------------------ */
typedef struct {
    bool on;         /* this player is in the game */
    bool alive;
    float x, vx, aim;
    int face;        /* -1 / 1: which way the side guns fire with the pad neutral */
    int meter;
    int main_cd, side_cd;
    int dead_t, inv;
    int enter;       /* frames left driving in from the left edge (controls off) */
    uint16_t prev;   /* last frame's buttons */
    bool drifting;
    int drift_frames;
    int col;         /* body colour */
} DflCar;

/* ---- the game's states --------------------------------------------------------------- */
enum { DS_TITLE, DS_PLAY, DS_CLEAR, DS_BONUS, DS_BONUS_END, DS_OVER, DS_ENDING, DS_CREDITS };
#define DFL_BANNER_T 150      /* the stage's name is shown this long at its start */
#define DFL_CLEAR_T 240       /* the stage clear screen */
#define DFL_BOSS_FALL_T 150   /* the boss's last explosions */

/* The cartridge save: records only (a run is not saved, as in the original). */
typedef struct {
    uint32_t magic;
    uint32_t best;          /* best score */
    uint16_t runs, wins;
    uint16_t coins, cherries;
    uint8_t most_cars;      /* the "most lives at once" record */
    uint8_t best_stage;     /* furthest stage reached */
    uint8_t pad[2];
} DflSave;

typedef struct {
    int state, state_t, frame_t;
    int sel;                 /* title menu */
    int players;             /* 1 or 2 */
    /* the run */
    int stage;               /* 0..3 */
    uint32_t score;
    int spare;               /* cars in reserve, shared in 2P */
    int most_cars;
    int lost_stage;          /* cars lost in this stage */
    int deaths;
    int coins;
    int bonus_played;
    bool won;
    DflCar car[2];
    /* the stage */
    int stage_t, spawn_i;
    float scroll;
    int swell_i, swell_loop;
    DflSwell swell[DFL_MAX_SWELLS];
    DflBoss boss;
    int clear_t;
    int kills, kills_stage, foes_stage, escaped;
    int stage_points;        /* points from foes this stage */
    /* the bonus stage */
    uint8_t block[DFL_BROWS][DFL_BCOLS];   /* hit points left, 0 = gone */
    int blocks_left, blocks_total, blocks_broken, blocks_shot;
    uint32_t bonus_points;
    float bx, by, bvx, bvy;
    bool ball_live, bonus_won, coin_out;
    float coin_y;
    /* the world */
    DflFoe foe[DFL_MAX_FOES];
    DflEShot es[DFL_MAX_ESHOTS];
    DflPShot ps[DFL_MAX_PSHOTS];
    DflPart part[DFL_MAX_PARTS];
    int shake;
    /* testing */
    bool god, bot2, boss_still;
    int last_hit_dmg;
    int cause;              /* what took the last car (CAUSE_*) */
    int msg_t;
    const char *msg;
} DflGame;
extern DflGame dfg;
extern DflSave dfs;

/* driftline_play.c */
void dfl_new_run(int players);
void dfl_start_stage(int s);
void dfl_play_update(uint16_t in0, uint16_t in1);
void dfl_bonus_start(void);
void dfl_bonus_update(uint16_t in0, uint16_t in1);
void dfl_car_spawn(int p, bool from_left);
/* what took the car: a shot (CAUSE_SHOT + its kind), a foe touched
 * (CAUSE_FOE + its kind), or one of these */
enum { CAUSE_SHOT = 0, CAUSE_FOE = 100, CAUSE_BEAM = 200, CAUSE_SWELL, CAUSE_FIST, CAUSE_LEG, CAUSE_TEST };
void dfl_kill_car(int p, int cause);
void dfl_add_score(uint32_t pts);
int dfl_tier(int meter);
void dfl_clear_world(bool keep_boss);
void dfl_burst(float x, float y, int col, int n, float sp);
int dfl_add_eshot(int kind, float x, float y, float vx, float vy);
void dfl_aimed(float x, float y, float speed, int n, float spread_deg);
void dfl_ring(float x, float y, float speed, int n, float rot);
int dfl_nearest_car(float x);
void dfl_sfx(const char *name, int gap);
bool dfl_car_hit_rect(int p, float x0, float y0, float x1, float y1);
bool dfl_car_hit_circle(int p, float x, float y, float r);
void dfl_save_now(void);
int dfl_cars_alive(void);
void dfl_note_cars(void);

/* driftline_foes.c */
int dfl_spawn_foe(int kind, float x, float y, int dir, int arg);
#define DFL_ADD (-7777)   /* arg of a foe a boss brings along: worth nothing */
void dfl_run_spawns(void);
void dfl_foes_update(void);
void dfl_hurt_foe(int i, int dmg, bool side, float svx);
void dfl_kill_foe(int i, bool scored);
void dfl_swells_update(void);
bool dfl_swell_breaking(int i);
void dfl_boss_start(int stage);
void dfl_boss_update(void);
/* a player shot against the boss: true if it was stopped */
bool dfl_boss_shot(DflPShot *s);
/* the moon's fist sweeps low along this curve */
#define DFL_FIST_HW 13
#define DFL_FIST_HH 10
float dfl_fist_y(float x);
/* the crab's legs: a warning, the drop, a stay on the road, the lift */
#define DFL_LEG_WARN 50
#define DFL_LEG_DROP 8
#define DFL_LEG_STAY 26
#define DFL_LEG_LIFT 20
#define DFL_LEG_TOP_Y 96.0f
float dfl_leg_x(int k);
float dfl_leg_foot(int k);
/* the parts of the boss that hurt the cars, for the demo player:
 * rectangles x0,y0,x1,y1 (up to max) */
int dfl_boss_hazards(float *rects, int max);
uint32_t dfl_stage_points(int s);   /* every foe in a stage's list, killed */
int dfl_stage_foes(int s);

/* driftline_bot.c: the demo player, for the tests */
int dfl_bot_buttons(int p);

/* driftline_draw.c */
void dfl_draw(void);
void dfl_draw_label(int x, int y, int w, int h, int t);

/* art and sound */
enum {
    SP_CAR, SP_CAR2, SP_HOG, SP_WRECK, SP_KITE1, SP_KITE2, SP_BUZZER, SP_ROTOR1, SP_ROTOR2,
    SP_SHARD, SP_PRISM, SP_TUMBLER1, SP_TUMBLER2,
    SP_SKULL, SP_SHEET1, SP_SHEET2, SP_SNAPPER, SP_SNAPPER2, SP_SLAB,
    SP_JELLY1, SP_JELLY2, SP_DART, SP_SQUIRT1, SP_SQUIRT2, SP_SHARK, SP_PALM, SP_COIN, SP_LAMP,
    SP_COUNT
};
extern Sprite dfl_spr[SP_COUNT];
void dfl_art_load(void);
void dfl_audio_load(void);
extern int DFL_MUS_TITLE, DFL_MUS_STAGE[DFL_STAGES], DFL_MUS_BOSS, DFL_MUS_FINAL, DFL_MUS_BONUS, DFL_MUS_CLEAR,
    DFL_MUS_OVER, DFL_MUS_ENDING, DFL_MUS_CREDITS, DFL_MUS_COIN;

extern const char *const DFL_STAGE_TIME[DFL_STAGES];

#endif
