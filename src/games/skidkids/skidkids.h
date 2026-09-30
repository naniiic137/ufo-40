/* SKID KIDS - shared declarations. Cartridge 26 of UFO 40.
 * A tribute to Hot Foot (UFO 50 #26); see docs/games/26-skid-kids.md.
 *
 * Top-down two-on-two beanbag dodgeball in a school gym: beanbags skid
 * along the floor, you jump them for half-stars, and stars pay for each
 * kid's special throw (a full wind-up) or special move (a second jump in
 * the air). First to 15 hits. */
#ifndef SKIDKIDS_H
#define SKIDKIDS_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the court (floor coordinates: x, y; heights go in z) -------------- */
#define SKID_LEFT 12   /* the left back wall's face */
#define SKID_RIGHT 308 /* the right back wall's face */
#define SKID_TOP 46    /* the far edge of the floor */
#define SKID_BOT 174   /* the near edge */
#define SKID_MID 160   /* the centre line */
#define SKID_COACH_X 160
#define SKID_COACH_Y 42

#define SKID_MAX_BAGS 12   /* every bag on the court, held ones too */
#define SKID_MAX_ITEMS 32  /* juice boxes, puddles, marbles, balloons */
#define SKID_MAX_WAVES 4

/* ---- the roster ---------------------------------------------------------- */
enum {
    K_NOODLE, K_PIPPA, K_HOPS, K_MILO, K_SPARKY, K_NELL,
    K_KIKI, K_ROXIE, K_TOBY, K_SID, K_BUZZY, K_MOOSE,
    K_BOOMER,   /* the kangaroo (match 5's partner) */
    K_BENCHBOT, /* the robot (match 6's partner) */
    SKID_ROSTER
};
#define SKID_KIDS 12

/* passives */
enum {
    P_WAVY = 1 << 0,        /* throws snake along */
    P_QUICKUP = 1 << 1,     /* stands up faster */
    P_HIGHJUMP = 1 << 2,    /* jumps higher and faster */
    P_LOB = 1 << 3,         /* throws fly high and far */
    P_QUICKCHARGE = 1 << 4, /* winds up in half the time */
    P_SHARE = 1 << 5,       /* half-stars go to the partner too */
    P_TWOBAGS = 1 << 6,     /* carries two bags */
    P_FAST = 1 << 7,        /* runs faster */
    P_NOSLIP = 1 << 8,      /* never slips (and so the CPU needn't steer round puddles) */
    P_QUICKGRAB = 1 << 9,   /* picks things up faster */
    P_CROSS = 1 << 10,      /* may cross the centre line */
    P_FAVORED = 1 << 11,    /* the Coach's favourite */
    P_PUSH = 1 << 12,       /* running into a rival knocks them down */
    P_BIGPUSH = 1 << 13     /* that push beats any other push */
};

/* special throws (a full wind-up with a star) */
enum { ST_COMET, ST_RIPPLE, ST_YOYO, ST_POPPER, ST_MARBLE, ST_HOMING, ST_COUNT };
/* special moves (a second jump in the air with a star) */
enum { SA_STOMP, SA_GUST, SA_REEL, SA_SPLASH, SA_POUCH, SA_SWEEPER, SA_COUNT };

typedef struct KidDef {
    const char *name;
    const char *power;          /* the passive, for the cards */
    const char *line;           /* said when leading the other team */
    uint16_t passives;
    uint8_t throw_kind, move_kind;
    uint8_t skin, hair, shorts, shoes; /* palette colours */
} KidDef;
extern const KidDef SKID_KID[SKID_ROSTER];
extern const char *const SKID_THROW_NAME[ST_COUNT];
extern const char *const SKID_MOVE_NAME[SA_COUNT];

/* ---- a match -------------------------------------------------------------- */

/* what drives a kid */
enum { CTRL_AI, CTRL_P1, CTRL_P2 };
/* how an AI kid plays */
enum { AI_RIVAL, AI_MATE_BAGS, AI_MATE_FULL, AI_IDLE };

/* kid states */
enum { KS_STAND, KS_PICKUP, KS_CHARGE, KS_THROW, KS_HURT, KS_DOWN, KS_WIN, KS_SAD, KS_CRY };

/* a virtual pad: people and the CPU drive kids through the same thing */
typedef struct Pad {
    int8_t dx, dy;
    bool a, ap;      /* A held / pressed this frame (jump) */
    bool b, bp, br;  /* B held / pressed / released */
} Pad;

typedef struct Kid {
    uint8_t who, team, slot;
    uint8_t ai;             /* AI_* when an AI drives it */
    uint16_t pas;           /* passives */
    uint8_t state;
    float x, y, z, vz;
    int8_t face;            /* +1 faces right */
    int8_t aimx, aimy;      /* the throw's direction */
    int t;                  /* frames in this state */
    int stars;              /* in halves, 0..6 */
    int held;               /* bags in hand */
    int hold_t;             /* frames holding (the forced throw) */
    int charge;             /* frames B has been held with a bag */
    bool armed;             /* B went down holding a bag: a tap or a wind-up? */
    int timer;              /* down / hurt / pick-up frames left */
    int pick_kind, pick_i;  /* what is being picked up: 1 bag, 2 item */
    bool air_move;          /* the special move was used this jump */
    bool jumping;
    int slip_cd;            /* no slipping again for a while after getting up */
    int since_up;           /* frames since last on its feet again */
    bool moved;             /* walked this frame */
    bool walking;           /* the pad held to walk this frame, blocked or not (the push) */
    int anim;
    int queued_reel;        /* the pouch grab's second pull */
    /* the AI's working memory */
    int ai_t, ai_wait, ai_goal, ai_target, ai_lock;
    float ai_tx, ai_ty;
    int ai_charge_to;       /* wind-up frames it means to reach */
    int ai_seen;            /* the bag it saw coming */
    int ai_react;           /* frames until it reacts to it */
    int ai_jump_roll;
    int ai_detour;          /* going round a puddle: the way it turns (+1/-1), 0 none */
} Kid;

/* bag states */
enum { BG_GONE, BG_LOOSE, BG_SLIDE, BG_AIR, BG_HELD, BG_PASS, BG_REEL, BG_WAVE, BG_COACH };
/* bag kinds (a normal bag, or a special throw riding on it) */
enum { BK_NORMAL, BK_COMET, BK_YOYO, BK_POPPER, BK_MARBLE, BK_HOMING };

typedef struct Bag {
    uint8_t state, kind;
    int8_t team;            /* the team that threw it (-1 the Coach) */
    int8_t owner;           /* the kid holding it, or the thrower */
    int8_t target;          /* pass or pull: the kid it flies to */
    float x, y, z, vx, vy, vz;
    float wphase;           /* the wavy throw's wiggle */
    float wamp;
    bool heavy;             /* knocks down (a full wind-up or a special) */
    bool toss;              /* a light toss: hits in the air knock down */
    bool dead;              /* has hit: harmless until it stops */
    bool wavy;
    bool lob;               /* an arcing throw */
    uint8_t hitmask;        /* kids it has hit this flight */
    uint8_t jumpmask;       /* kids that got half a star over it */
    int age;
    int phase;              /* yo-yo out / back; popper fuse */
    int fuse;
    float radius;           /* hit radius */
    float friction;
} Bag;

enum { IT_NONE, IT_JUICE, IT_PUDDLE, IT_MARBLE, IT_BALLOON };
typedef struct Item {
    uint8_t kind;
    int8_t team;            /* a balloon's thrower's team (-1 the Coach) */
    bool flying;
    float x, y, z, vx, vy, vz;
    float r;
    int life, age;
    bool from_coach;
} Item;

typedef struct Wave {
    bool live;
    int8_t team, dir;
    float x, y, half;
    uint8_t hitmask, jumpmask;
    int bag;                /* the bag riding on it */
} Wave;

typedef struct Sweeper {
    bool live;
    int8_t team, dir;       /* the robot's team; the way it slides */
    float x;
    uint8_t hitmask;
    int swept[SKID_MAX_BAGS];
    int n_swept;
} Sweeper;

typedef struct Rules {
    int goal;        /* points to win (15) */
    int max_bags;    /* bags on the court at most (5) */
    int cost;        /* stars a special costs (1) */
    int mate_ai;     /* 0 fetches bags only, 1 plays by itself */
    int mate_jump;   /* 0 the whole team jumps together, 1 each on its own */
} Rules;

enum { MS_READY, MS_PLAY, MS_OVER };

/* event bits for sounds and effects */
enum {
    EV_THROW = 1 << 0, EV_HIT = 1 << 1, EV_DOWN = 1 << 2, EV_JUMP = 1 << 3, EV_HALFSTAR = 1 << 4,
    EV_PICK = 1 << 5, EV_PASS = 1 << 6, EV_SWAP = 1 << 7, EV_SPECIAL = 1 << 8, EV_MOVE = 1 << 9,
    EV_BOOM = 1 << 10, EV_WALL = 1 << 11, EV_COACH = 1 << 12, EV_SLIP = 1 << 13, EV_SPLASH = 1 << 14,
    EV_WHISTLE = 1 << 15, EV_DRINK = 1 << 16, EV_STOMP = 1 << 17, EV_GUST = 1 << 18, EV_REEL = 1 << 19,
    EV_PUSH = 1 << 20, EV_CANCEL = 1 << 21, EV_FORCED = 1 << 22, EV_STAR = 1 << 23, EV_CLANK = 1 << 24
};

/* the Coach's barks */
enum {
    BARK_NONE, BARK_PLAY, BARK_HEADS, BARK_JUMP, BARK_OUCH, BARK_HUSTLE, BARK_FAIR, BARK_DRINK,
    BARK_WHOA, BARK_GAME, BARK_COUNT
};

typedef struct Match {
    Kid k[4];                /* 0-1 the left team, 2-3 the right */
    Bag bag[SKID_MAX_BAGS];
    Item it[SKID_MAX_ITEMS];
    Wave wave[SKID_MAX_WAVES];
    Sweeper sweep;
    Rules rules;
    int score[2];
    uint8_t player[2];       /* who plays each team: CTRL_AI (the CPU), CTRL_P1, CTRL_P2 */
    int ctrl[2];             /* the kid each team's player drives */
    bool coop;               /* each player drives one kid of the left team */
    bool swap_only;          /* the SWAP-ONLY code: B only swaps kids */
    bool endless;            /* the attract demo: no winner */
    int level[2];            /* each team's CPU skill (0-5, 6 = the demo player) */
    int state, state_t, winner;
    int frame;
    /* the Coach */
    int coach_t, coach_next, coach_throw_t, coach_items;
    int bark, bark_t;
    bool coach_off;          /* tests: no items */
    int favored;             /* the team the Coach favours (-1 none) */
    /* effects on the whole court */
    int gust_t, gust_team;
    int quake_t;
    Rng rng;
    /* tallies (for the tests and the result card) */
    int n_throw, n_hit, n_down, n_half, n_pick, n_pass, n_swap, n_special_throw, n_special_move;
    int n_forced, n_cancel, n_coach_bag, n_coach_juice, n_coach_balloon, n_drink, n_slip, n_push;
    int n_boom, n_crushed, n_reel, n_toss, n_jump;
    int hits_on[4];          /* points scored on each kid */
    int hit_when[4];         /* points on a kid standing, in the air, busy, down */
    /* events for sounds and effects, cleared by the presentation */
    uint32_t ev;
    float ev_x, ev_y;
} Match;

/* match rules (skidkids_match.c) */
void skid_rules_default(Rules *r);
void skid_match_init(Match *m, const int who[4], const Rules *r, uint64_t seed);
void skid_match_update(Match *m, const Pad pads[4]);
void skid_kid_bounds(const Match *m, int i, float *x0, float *x1);
int skid_bags_total(const Match *m);
int skid_bag_add(Match *m);
int skid_item_add(Match *m, int kind);
bool skid_airborne(const Kid *k);
int skid_charge_full(const Kid *k);
int skid_pick_time(const Kid *k);
int skid_down_time(const Kid *k);
int skid_capacity(const Kid *k);
void skid_knockdown(Match *m, int i, int frames);
void skid_coach_throw(Match *m, int kind, float tx, float ty);
#define SKID_TAP 10     /* B held this long with a bag starts a wind-up */
#define SKID_TOSS 20    /* a wind-up let go before this is the light toss, for every kid */
#define SKID_FORCED 300 /* holding a bag this long throws it for you */

/* the CPU (skidkids_ai.c) */
void skid_ai_pad(Match *m, int i, Pad *out);
void skid_bot_pad(Match *m, int i, Pad *out); /* the demo player */

/* art & audio */
enum {
    SS_COACH, SS_COACH_THROW, SS_COACH_WHISTLE,
    SS_BAG, SS_JUICE, SS_MARBLE, SS_BALLOON, SS_STAR, SS_HALFSTAR, SS_NOSTAR, SS_TROPHY, SS_NOTE,
    SS_SPRITE_COUNT
};
/* every kid in every pose, in each team's bib, built at load time */
enum { KP_IDLE0, KP_IDLE1, KP_RUN0, KP_RUN1, KP_WIND, KP_THROW, KP_PICK, KP_JUMP, KP_HURT, KP_WIN, KP_DOWN, KP_COUNT };
extern Sprite skid_spr[SS_SPRITE_COUNT];
extern Sprite skid_kid_spr[SKID_ROSTER][2][KP_COUNT];
extern int skid_art_bad; /* sprite strings of the wrong length (tests) */
void skid_art_load(void);
void skid_audio_load(void);
extern int SKID_MUS_TITLE, SKID_MUS_PICK, SKID_MUS_BRACKET, SKID_MUS_MATCH1, SKID_MUS_MATCH2, SKID_MUS_ROO, SKID_MUS_BOT,
    SKID_MUS_WIN, SKID_MUS_LOSE, SKID_MUS_CHAMPS, SKID_MUS_OVER, SKID_MUS_CREDITS;

/* drawing (skidkids_draw.c) */
/* a kid with its feet at (x, y); cry adds tears */
void skid_draw_kid_at(int who, int team, int pose, int x, int y, int flip, int scale, bool cry);
int skid_kid_pose(const Kid *k, int frame);
void skid_draw_court(const Match *m, int frame, int shake, int p1_kid, int p2_kid);
void skid_draw_gym(int frame, int y0, bool plain);
void skid_draw_hud(const Match *m, int frame);
extern const char *const SKID_BARK_TEXT[BARK_COUNT];

#endif
