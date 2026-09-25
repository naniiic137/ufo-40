/* CUTLASS CUP - shared declarations. Cartridge 14 of UFO 40.
 * A tribute to Bushido Ball (UFO 50 #14); see docs/games/14-cutlass-cup.md. */
#ifndef CUTLASS_H
#define CUTLASS_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* the court (ground coordinates; the ball also has a height z) */
#define CC_TOP 34          /* top rail's foot */
#define CC_BOT 168         /* bottom rail's foot */
#define CC_MID ((CC_TOP + CC_BOT) / 2)
#define CC_CENTER 160
#define CC_CIRCLE_L 58
#define CC_CIRCLE_R 262
#define CC_REACH 48        /* how far past the centre a player may go */
#define CC_MAX_PLAYERS 4
#define CC_MAX_PROJ 24
#define CC_MAX_FAKES 4
#define CC_FIGHTERS 6

enum { F_HAMDI, F_LEILA, F_NOUR, F_KARIM, F_ZINA, F_OMAR };

typedef struct FighterDef {
    const char *name, *title;
    const char *second, *super; /* gimmick names */
    uint8_t speed, control, power;
    uint8_t reach_x, reach_y;
    uint8_t shirt, shirt_dark, pants, sash; /* palette colours */
} FighterDef;
extern const FighterDef CC_FIGHTER[CC_FIGHTERS];

/* player states */
enum { PS_IDLE, PS_RUN, PS_SWING, PS_ROLL, PS_STUN, PS_CATCH, PS_CHARGE, PS_WIN, PS_LOSE };
/* swing kinds */
enum { SW_NORMAL, SW_SUPER, SW_THRUST };

/* a virtual pad: humans and the CPU drive players through the same thing */
typedef struct Pad {
    int8_t dx, dy;
    bool roll;               /* pressed this frame */
    bool strike, held, released;
} Pad;

typedef struct Player {
    uint8_t fighter, team, cpu, input; /* input: 0 = player 1, 1 = player 2 */
    float x, y, kx, ky;       /* position, knockback */
    int8_t face;              /* +1 faces right */
    uint8_t state;
    int t;                    /* time in state */
    int stun;
    int swing_kind, swing_aim; /* aim: -1 up, 0 straight, 1 down, 2 lob */
    bool swing_hit, roll_boost, curve;
    int8_t roll_dx, roll_dy, hold_dy;
    int roll_cd;
    int meter;                /* half bars, 0..6 */
    int strikes;              /* weapon strikes, two per half bar */
    int tap_t;                /* frames since the last strike press */
    int charge;
    int mash;
    float home_y;
    /* the CPU's thinking */
    int ai_delay, ai_cool, ai_plan, ai_charge_t;
    float ai_ty, ai_tx, ai_err;
    bool ai_hold;
} Player;

/* ball kinds: normal, or a super shot */
enum { BK_NORMAL, BK_COMET, BK_WALL, BK_MIRAGE, BK_WHIRL, BK_DJINN, BK_PLUNGE };

typedef struct Ball {
    float x, y, z, vx, vy, vz;
    float curve;              /* vy change per frame */
    float speed;
    uint8_t kind, level, phase, live;
    int8_t team;              /* last team to strike it, -1 = the judge */
    int8_t dir;               /* +1 heading right */
    int8_t hitter;            /* player index that last struck it */
    int t;                    /* frames since it was struck */
    int no_body;              /* frames the hitter's own body is ignored */
    float ax, ay, ang;        /* whirl centre and angle */
    int8_t turn;              /* whirl / wall: chosen way */
    int caught_by;            /* -1, or the player holding it */
} Ball;

/* projectiles and traps */
enum { PR_NONE, PR_COIN, PR_URCHIN, PR_HOOK, PR_SLASH, PR_DART, PR_BOMB, PR_DECOY, PR_BLAST, PR_FAKE };
typedef struct Proj {
    uint8_t kind, team, owner, live;
    float x, y, z, vx, vy, vz;
    int t, life;
    float range, dist;
    bool reflected, hooked, full_range;
} Proj;

enum { MS_READY, MS_SERVE, MS_PLAY, MS_POINT, MS_OVER };
enum { SPEED_NORMAL, SPEED_FAST, SPEED_HYPER };
enum { FOUL_STALL = 1, FOUL_WEAPON, FOUL_SERVE };

typedef struct Match {
    Player pl[CC_MAX_PLAYERS];
    int np;
    Ball ball;
    Ball fake[CC_MAX_FAKES];
    Proj pr[CC_MAX_PROJ];
    int score[2], fouls[2];
    int goal, time_limit, time_left; /* frames, 0 = no limit */
    bool laws;
    int speed_mode;
    float spd;                /* speed multiplier */
    int state, state_t;
    int receiver_team;
    bool serve_live;
    int stall_t[2];
    int winner;               /* -1 while playing */
    int point_team, point_why; /* who scored and why (0 ball, FOUL_*) */
    int last_foul_team, last_foul_kind, foul_flash;
    bool sudden;
    int ai_level;             /* 0..4 */
    Rng rng;
    int frame;
    /* events for sounds and effects, cleared by the presentation each frame */
    uint32_t fx;
    float fx_x, fx_y;
} Match;

/* effect bits in Match.fx */
enum {
    FX_STRIKE = 1 << 0, FX_SUPER = 1 << 1, FX_BODY = 1 << 2, FX_WALL = 1 << 3, FX_POINT = 1 << 4,
    FX_FOUL = 1 << 5, FX_SECOND = 1 << 6, FX_STUN = 1 << 7, FX_BLAST = 1 << 8, FX_CATCH = 1 << 9,
    FX_REFLECT = 1 << 10, FX_SERVE = 1 << 11, FX_ROLL = 1 << 12, FX_METER = 1 << 13, FX_SWEET = 1 << 14,
    FX_SPLASH = 1 << 15, FX_BLINK = 1 << 16, FX_SLIP = 1 << 17
};

/* match rules (cutlass_match.c) */
void cc_match_init(Match *m, const int *fighters, int np, const bool *cpu, int goal, int time_min, bool laws,
                   int speed_mode, int ai_level, uint64_t seed);
void cc_match_update(Match *m, const Pad pads[CC_MAX_PLAYERS]);
void cc_ai_pad(Match *m, int i, Pad *out);
int cc_team_size(const Match *m, int team);
float cc_min_x(const Match *m, int i);
float cc_max_x(const Match *m, int i);

/* art & audio */
enum {
    CS_BODY_IDLE0, CS_BODY_IDLE1, CS_BODY_RUN0, CS_BODY_RUN1, CS_BODY_WIND, CS_BODY_STRIKE,
    CS_BODY_ROLL, CS_BODY_STUN, CS_BODY_CATCH, CS_BODY_WIN,
    CS_HEAD0, CS_HEAD1, CS_HEAD2, CS_HEAD3, CS_HEAD4, CS_HEAD5,
    CS_BALL, CS_COIN1, CS_COIN2, CS_URCHIN, CS_DART, CS_BOMB, CS_DECOY, CS_JUDGE1, CS_JUDGE2,
    CS_GULL1, CS_GULL2, CS_CUP,
    CS_SPRITE_COUNT
};
enum { POSE_IDLE0, POSE_IDLE1, POSE_RUN0, POSE_RUN1, POSE_WIND, POSE_STRIKE, POSE_ROLL, POSE_STUN, POSE_CATCH, POSE_WIN, POSE_COUNT };
extern Sprite cc_spr[CS_SPRITE_COUNT];
extern Sprite cc_fighter_spr[CC_FIGHTERS][POSE_COUNT]; /* bodies with heads, per fighter */
void cc_art_load(void);
void cc_audio_load(void);
extern int CC_MUS_TITLE, CC_MUS_SELECT, CC_MUS_BRACKET, CC_MUS_POINT, CC_MUS_WIN, CC_MUS_LOSE, CC_MUS_CUP;

#endif
