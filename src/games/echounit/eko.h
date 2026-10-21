/* ECHO UNIT - shared declarations. Cartridge 50 of UFO 40.
 * A tribute to Cyber Owls (UFO 50 #50); see docs/games/50-echo-unit.md.
 *
 * Four cyber-enhanced bats, four genres. FLINT brawls through the Marseille
 * docks, VESPER holds a shooting gallery in the Andes, DART rides a
 * hover-bike down the Cairo ring road and SABLE sneaks through a V.I.P.E.R.
 * listening post in Istanbul. Every mission gives two lives; lose both and
 * that bat is captured, its mission locked until another bat wins a short,
 * random, turn-based jailbreak. With all four missions done the team storms
 * the Caldera, V.I.P.E.R.'s volcano base, where every style comes back.
 *
 * Everything that moves uses whole numbers (1/256 of a pixel) and every
 * angle comes from a table, so a seed plays the same on every platform. */
#ifndef EKO_H
#define EKO_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- fixed point and angles ---------------------------------------------- */
#define EKO_FX 256
#define EKO_PX(v) ((v) >> 8)
#define EKO_ANG 64
extern const int16_t EKO_SIN[EKO_ANG];
static inline int eko_sin(int a) { return EKO_SIN[a & 63]; }
static inline int eko_cos(int a) { return EKO_SIN[(a + 16) & 63]; }
int eko_dir_to(int dx, int dy);   /* table angle (0 = right, 16 = down) nearest (dx, dy) */
int eko_isqrt(int v);
int eko_dist(int dx, int dy);
int eko_angdiff(int a, int b);    /* |a - b| on the circle, 0..32 */

/* ---- buttons --------------------------------------------------------------- */
/* Owner's layout: the main action of every bat is on A (Z / Space / Cross);
 * Cyber Owls has it on button 1. The card's CONTROLS page can swap them. */
#define EKO_MAIN BTN_A
#define EKO_ALT BTN_B
#define EKO_DIRS (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)

/* ---- the team ---------------------------------------------------------------- */
enum { OWL_FLINT, OWL_VESPER, OWL_DART, OWL_SABLE, OWL_COUNT };
extern const char *const EKO_OWL_NAME[OWL_COUNT];
extern const int EKO_OWL_HP[OWL_COUNT];      /* health bar: 8, 6, 6, 4 pips */
extern const int EKO_OWL_INV[OWL_COUNT];     /* safe frames after a hit; Sable's are short */
#define EKO_LIVES 2                          /* every mission starts with two */
#define EKO_MAX_LIVES 9
#define EKO_LIFE_EVERY 800                   /* an extra life every 800 points of a mission */
#define EKO_MANGO_HP 4

enum { MI_MARSEILLE, MI_ANDES, MI_CAIRO, MI_ISTANBUL, MI_CALDERA, MI_COUNT };
extern const char *const EKO_MISSION_NAME[MI_COUNT];

/* the playable pieces a mission is made of */
enum { MODE_NONE, MODE_BRAWL, MODE_GALLERY, MODE_BIKE, MODE_SNEAK, MODE_RESCUE };
typedef struct {
    uint8_t mode, owl, data;  /* data: the map / script / course */
    const char *title;        /* the card shown before it */
} EkoSection;
#define EKO_MAX_SECTIONS 6
typedef struct {
    const char *name;
    int nsec;
    EkoSection sec[EKO_MAX_SECTIONS];
} EkoMission;
extern const EkoMission EKO_MISSION[MI_COUNT];

/* ---- shared bits ----------------------------------------------------------------- */
typedef struct { int x, y, vx, vy, life, col; } EkoPart;
#define EKO_MAX_PARTS 160

/* ==== FLINT: the brawler ============================================================ */
#define BR_TILE 16
#define BR_SW 20                  /* a screen chunk: 20 x 10 tiles */
#define BR_SH 10
#define BR_TOP 20                 /* the play field starts under the HUD */
#define BR_MAX_CW 12              /* a map is at most 12 chunks wide ... */
#define BR_MAX_CH 11              /* ... and 11 high (the lab climb) */
#define BR_MW (BR_SW * BR_MAX_CW)
#define BR_MH (BR_SH * BR_MAX_CH)

typedef struct { char code; const char *rows[BR_SH]; } EkoChunk;

enum { BM_DOCKS, BM_YARDS, BM_LIFT, BM_LAB, BM_COUNT };
typedef struct {
    const char *name;
    const char *layout[BR_MAX_CH + 1]; /* rows of chunk letters, top first, NULL ends */
    int lift;                          /* the lift ride: waves, then the boss on the roof */
    int boss;                          /* BRB_* at the 'B' mark (or after the ride) */
} EkoBrawlMap;
extern const EkoBrawlMap EKO_BMAP[BM_COUNT];
extern const EkoChunk EKO_BCHUNK[];
extern const int EKO_BCHUNKS;

/* the lift ride's waves: at frame t, n foes of a kind drop in */
typedef struct { int16_t t; uint8_t kind, n; int16_t x; } EkoWave;
extern const EkoWave EKO_LIFT_WAVES[];
extern const int EKO_LIFT_NWAVES;
#define BR_LIFT_T 2700            /* frames the lift climbs before the roof */

/* physics (1/256 px a frame) */
#define BR_WALK 320               /* 1.25 px a frame */
#define BR_CARRY_WALK 224
#define BR_GRAV 64
#define BR_JUMP_VY (-1152)        /* one fixed jump: 40 px, a tap or a hold the same */
#define BR_MAX_FALL 1536
#define BR_PW 12
#define BR_PH 24
#define BR_PH_DUCK 15
#define BR_CHARGE_T 40            /* hold A, standing still, this long: he flashes, charged */
#define BR_KI_T 24                /* the ki blast flies this many frames ... */
#define BR_KI_VX 1024             /* ... at 4 px a frame: 96 px, a short way */
#define BR_KI_POSE 14             /* frames of the throwing pose ... */
#define BR_KI_LUNGE 512           /* ... in which he lunges forward 2 px a frame (28 px; off a ledge, too) */
#define BR_WARN_T 45              /* a hazard's "!" shows this long before it comes */
#define BR_RESPAWN_T 240          /* more troopers come every 4 s ... */
#define BR_RESPAWN_CAP 3          /* ... while fewer than this are about */
#define BR_COMBO_WINDOW 22

enum { BRA_NONE, BRA_PUNCH1, BRA_PUNCH2, BRA_KICK, BRA_LOW, BRA_AIRKICK, BRA_KI, BRA_THROW };

enum { BF_NONE, BF_SOLDIER, BF_GRENADIER, BF_JETPACK, BF_KEYMAN, BF_RAT, BF_SCRAPJAW, BF_TALPA, BF_KINDS };
/* map marks beyond the foes: a barrel, a mango, an extra life, a gull, a
 * hazard that drops, a hook that sweeps */
enum { SPK_BARREL = 20, SPK_MANGO, SPK_LIFE, SPK_GULL, SPK_DROP, SPK_HOOK };
extern const int EKO_BF_HP[BF_KINDS];
extern const int EKO_BF_POINTS[BF_KINDS];
enum { BRB_NONE, BRB_SCRAPJAW, BRB_TALPA };

typedef struct {
    uint8_t alive, kind, state, onground;
    int x, y, vx, vy;            /* feet centre */
    int hp, face, t, cd, flash, kb, chain, spawn;
    int a, b;                    /* kind-specific */
    int hitsn;                   /* the attack that last hit it */
    int last_gx;                 /* last x it stood on ground (a key dropped by a fall lands there) */
} EkoBFoe;
#define BR_MAX_FOES 24

/* GULL: one sitting on a bollard (state 1: frightened off); DROP: a crane load
 * or a lump of slag, "!" first, then it falls; HOOK: a crane hook, "!" first,
 * then it sweeps across at chest height. A grenade in state 1 was kicked back. */
enum { BO_BARREL, BO_MANGO, BO_LIFE, BO_KEY, BO_GRENADE, BO_BULLET, BO_FBULLET, BO_FLAME, BO_KI, BO_GULL, BO_DROP, BO_HOOK };
typedef struct {
    uint8_t alive, kind, state;  /* barrel: 0 rest, 1 carried, 2 thrown, 3 bouncing */
    int x, y, vx, vy, t, chain, hits, last_foe;
} EkoBObj;
#define BR_MAX_OBJS 48

typedef struct {
    int x, y, vx, vy;            /* feet centre */
    int face, ground, duck;
    int act, act_t, combo_ok, charge, carry; /* carry: object index + 1 */
    int inv, hurt_t, dead_t;
    int atk_sn;                  /* serial of the attack in progress */
    int safe_x, safe_y;
    bool key;
} EkoBPlayer;

typedef struct {
    int map;
    int mw, mh;                  /* map size in tiles */
    char tile[BR_MH][BR_MW];
    int8_t thp[BR_MH][BR_MW];    /* crates and cracked walls: hits left */
    int cam_x, cam_y, lock_x;    /* lock_x: the boss arena's left edge (-1 none) */
    EkoBPlayer p;
    EkoBFoe foe[BR_MAX_FOES];
    EkoBObj obj[BR_MAX_OBJS];
    int nspawn;
    struct { int16_t tx, ty; uint8_t kind, used; } spawn[96];
    int chain_next;
    int chain_kills[16];
    int lift_t, lift_wave, lift_on, lift_y;
    int boss_on, boss_dead, boss_t, boss_i, done_t;
    int wall_open_t;             /* the key in the lock: the wall blows */
    int key_lost;                /* the key carrier went down off the screen: no key, ever (the original's bug) */
    int kills, secrets;
    int tile_sn;                 /* the blow that last knocked a crate or wall */
    int dmg_dealt;               /* for the demo player */
    int respawn_t, respawns;     /* the troopers who keep coming */
    int calm;                    /* tests: 1 no troopers coming, 2 no hazards */
    int gulls, gulls_hit;        /* this part's gulls, and those frightened off */
    int hazards;                 /* hazards set off */
} EkoBrawl;

/* ==== VESPER: the shooting gallery ===================================================== */
#define GA_ROWS 3
extern const int GA_ROW_Y[GA_ROWS];   /* far, middle, near: where their feet are */
extern const int GA_ROW_SCALE[GA_ROWS]; /* sizes in 1/4: 2, 3, 4 */
#define GA_PY 172                 /* Vesper's feet */
#define GA_MINX 16
#define GA_MAXX 304
#define GA_WALK 384               /* 1.5 px a frame */
#define GA_AIM 640                /* crosshair speed when locked: 2.5 px a frame */
#define GA_AIM_Y 512
#define GA_CROSS_MINY 34
#define GA_CROSS_MAXY 150
#define GA_LOCK_T 8               /* A held this long: locked in place */
#define GA_TAP_CD 5               /* frames between shots when tapping ... */
#define GA_HOLD_CD 7              /* ... and when holding: mashing fires only a little faster */
#define GA_ROLL_T 22
#define GA_ROLL_VX 768
#define GA_COMBO_T 150            /* frames the combo meter lasts after a kill */
#define GA_COMBO_MAX 9
#define GA_REACH 96               /* the sight stays within this far of her, either side */
#define GA_GRAB 12                /* standing this close to a thing on the ground, a tap of A takes it */
#define GA_FUSE_T 30              /* a grenade lies this long before it goes off */
#define GA_GRAB_FUSE 16           /* picked up at the last moment, it still gives her this long */
#define GA_THROW_T 16             /* frames a grenade thrown back takes to reach the sight */
#define GA_THROW_R 28             /* and its blast */

enum { GE_NONE, GE_RIFLE, GE_RUNNER, GE_MORTAR, GE_JEEP, GE_CHOPPER, GE_BARREL, GE_RAMROD, GE_VULTRA,
       GE_POD, GE_CANNON, GE_COIL, GE_MISSILE, GE_KINDS };
extern const int EKO_GE_HP[GE_KINDS];
extern const int EKO_GE_POINTS[GE_KINDS];

typedef struct {
    uint8_t alive, kind, row, state;
    int x, y;                    /* x px*256; y: feet (rows) or centre (sky) */
    int vx, vy, hp, t, cd, flash, cover, dir, a, b;
} EkoGFoe;
#define GA_MAX_FOES 32

typedef struct {
    uint8_t alive, kind;         /* 0 bullet, 1 grenade (lands, then a fuse), 2 bomb, 3 mango, 4 her grenade thrown back */
    int sx, sy, tx, ty, t, dur;  /* from (sx, sy) to (tx, ty) over dur frames, px */
} EkoGShot;
#define GA_MAX_SHOTS 40

typedef struct { int16_t t; uint8_t kind, row; int16_t x; int8_t dir; } EkoGSpawn;
enum { GS_ANDES, GS_FINALE, GS_COUNT };
typedef struct {
    const char *name;
    const EkoGSpawn *spawns;
    int n;
    int boss_t;                  /* frames until the boss */
    int covers[GA_ROWS][6];      /* cover x positions on each row (0 = none) */
} EkoGScript;
extern const EkoGScript EKO_GSCRIPT[GS_COUNT];

typedef struct {
    int script, t, spawn_i;
    int px, cx, cy;              /* Vesper's x and the crosshair (px*256) */
    int held_t, cd, roll_t, roll_dir, roll_cd, face;
    int inv, dead_t;
    int combo, meter, best_combo;
    int shots_fired, hits;
    int last_shot_x, last_shot_y, flash_t;
    EkoGFoe foe[GA_MAX_FOES];
    EkoGShot shot[GA_MAX_SHOTS];
    int boss_on, boss_dead, done_t, barrel_t;
    int kills;
    int carry;                   /* a live grenade in her hands: frames of fuse left (0 none) */
    int grabs, throws;
} EkoGallery;

/* ==== DART: the hover-bike ============================================================ */
#define BK_ROAD_TOP 92            /* the road's far edge (y of the bike's wheels) */
#define BK_ROAD_BOT 170
#define BK_MINX 20
#define BK_MAXX 230
#define BK_SPEED 384              /* 1.5 px a frame, four ways */
#define BK_SCROLL 3               /* the road, px a frame */
#define BK_JUMP_VZ 640            /* a jump: 30 px up, about 45 frames */
#define BK_GRAV_Z 28
#define BK_TAP_CD 4
#define BK_HOLD_CD 6              /* held, only a little slower than tapping (the heat is the reason to tap) */
#define BK_HEAT_MAX 300
#define BK_HEAT_DELAY 10          /* held this long, the gun starts heating */
#define BK_HEAT_UP 3
#define BK_HEAT_DOWN 2
#define BK_GLOW_T 30              /* boiled over, the bike glows red and the gun holds this long; still held, the next shot blows it up */

enum { OB_NONE, OB_CONE, OB_HURDLE, OB_RUNNER, OB_CAR, OB_BIKER, OB_GTRUCK, OB_SEMI, OB_MINE, OB_FIREJET,
       OB_ROCK, OB_MANGO, OB_LIFE, OB_CARD, OB_HOG,
       /* the Caldera's flight */
       OB_DRONE, OB_TURRET, OB_PILLAR, OB_MISSILE, OB_RATTLE, OB_KINDS };
/* the flight over the lava: the bike flies anywhere in this band, no jump */
#define BK_FLY_TOP 34
#define BK_FLY_BOT 162
extern const int EKO_OB_HP[OB_KINDS];
extern const int EKO_OB_POINTS[OB_KINDS];

typedef struct {
    uint8_t alive, kind, state;
    int x, y, z;                 /* world x (px*256), road y (px*256), height */
    int vx, vy, vz, hp, t, cd, flash, a, b;
} EkoKObj;
#define BK_MAX_OBJS 48
typedef struct { uint8_t alive, foe; int x, y, vx, vy; } EkoKShot;
#define BK_MAX_SHOTS 40

typedef struct { int32_t at; uint8_t kind; int16_t y; int8_t n; int16_t gap; int8_t arg; } EkoKSpawn;
enum { BC_CAIRO, BC_FLIGHT, BC_COUNT };
typedef struct {
    const char *name;
    const EkoKSpawn *spawns;
    int n;
    int length;                  /* px of road (or sky) before the boss */
    int boss;                    /* OB_HOG on the road, OB_RATTLE in the air */
    int fly;                     /* 1: a flight, the bike off the road (no jump) */
} EkoKCourse;
extern const EkoKCourse EKO_KCOURSE[BC_COUNT];

typedef struct {
    int course;
    int dist;                    /* px of road gone by */
    int spawn_i;
    int x, y, z, vz;             /* bike: screen x, road y, height (all *256) */
    int cd, held_t, heat, overheat;
    int hot, hot_held;           /* glowing red; and fire held without a break since it started */
    int inv, dead_t;
    EkoKObj obj[BK_MAX_OBJS];
    EkoKShot shot[BK_MAX_SHOTS];
    int boss_on, boss_dead, done_t, card;
    int kills, shots_fired, jumps;
} EkoBike;

/* ==== SABLE: the stealth mission ============================================================ */
#define SN_TILE 16
#define SN_RW 20                  /* a room: 20 x 10 tiles */
#define SN_RH 10
#define SN_TOP 20
#define SN_MAX_RX 7
#define SN_MAX_RY 5
#define SN_MW (SN_RW * SN_MAX_RX)
#define SN_MH (SN_RH * SN_MAX_RY)
#define SN_WALK 256               /* 1 px a frame, four ways */
#define SN_PW 10
#define SN_VIEW 84                /* how far a guard sees */
#define SN_VIEW_ANG 5             /* half the cone, in table steps (28 degrees) */
#define SN_SLEEP_T 600
#define SN_STUN_T 300
#define SN_LOOK_T 150
#define SN_ROOM_BONUS 100
#define SN_NOISE_COST 50
#define SN_AMMO_MAX 20            /* she carries up to twenty of each */
#define SN_NEAR 30                /* closer than this (out of sight), a guard comes to look where she was */
#define SN_HUNT_SP 224            /* a hunting guard: 0.875 px a frame, slower than her */
#define SN_HUNT_TURN 60           /* ... and it looks round only once a second */
#define SN_HUNT_FIRE 50
#define SN_THROW_T 18             /* frames a lobbed grenade flies (over anything) */
#define SN_ROBOT_RUN 768          /* a spider robot's charge: 3 px a frame, straight on */

enum { SM_ISTANBUL, SM_CALDERA, SM_COUNT };
typedef struct { char code; const char *rows[SN_RH]; } EkoRoom;
typedef struct {
    const char *name;
    const char *layout[SN_MAX_RY + 1];
    /* which rooms open into which: "A-B" a doorway, "L-M:R" a door that
     * wants the red card (G green, R red, U blue). Every other edge is wall. */
    const char *links;
} EkoSneakMap;
extern const EkoSneakMap EKO_SMAP[SM_COUNT];
extern const EkoRoom EKO_ROOM[];
extern const int EKO_ROOMS;

enum { WP_DART, WP_PISTOL, WP_STUN, WP_GRENADE, WP_COUNT };
extern const char *const EKO_WP_NAME[WP_COUNT];
enum { CARD_GREEN = 1, CARD_RED = 2, CARD_BLUE = 4 };
enum { GD_PATROL, GD_LOOK, GD_INVESTIGATE, GD_RETURN, GD_ALERT, GD_SLEEP, GD_STUN, GD_DEAD };
enum { GK_WALKER, GK_TURNER, GK_CAMERA, GK_LIGHT, GK_CIPHER };

typedef struct {
    uint8_t alive, kind, state, room;
    int x, y;                    /* px*256 */
    int hx, hy, hdir;            /* post: where it stands and which way */
    int dir;                     /* 0 right 1 down 2 left 3 up (cameras: table angle) */
    int t, cd, tx, ty, flash, hp, a, b;
    int aim_t;                   /* hunting: frames before it looks round again (until then it keeps one way) */
} EkoSGuard;
#define SN_MAX_GUARDS 120

typedef struct { uint8_t alive, foe; int x, y, vx, vy, t, kind; } EkoSShot; /* kind: 0 bullet 1 dart 2 stun 3 grenade */
#define SN_MAX_SHOTS 24

typedef struct {
    int map;
    int mw, mh;                  /* in tiles */
    char tile[SN_MH][SN_MW];
    int x, y, dir, walk_t;       /* Sable, px*256 */
    int room_x, room_y;          /* the room on screen */
    int inv, dead_t, hurt_t;
    int ammo[WP_COUNT], weapon;
    int cards;
    uint8_t room_seen[SN_MAX_RY][SN_MAX_RX];  /* 1 entered, 2 bonus paid */
    int room_spotted, room_noise;            /* this visit */
    EkoSGuard g[SN_MAX_GUARDS];
    int ng;
    EkoSShot shot[SN_MAX_SHOTS];
    int alarm;                   /* frames of the room-wide alarm (a camera or a light) */
    int boss_dead, done_t, zone, relay;      /* relay: reached the handover point */
    int spotted_total, rooms_clean, noise_total;
    int start_x, start_y;
    int entry_x, entry_y;        /* where she came into this room (a lost life starts her there) */
    struct { int16_t tx, ty; uint8_t kind, taken; } item[64];
    int nitem;
    int boss_g;                  /* Cipher's guard slot (-1 none) */
    int used_relays;
    int lights_out, cams_out;    /* fuse boxes blown; cameras broken */
    int ff, wakeups, robot_kos;  /* guards hit by guards' shots; sleepers woken by friends; guards a robot ran down */
} EkoSneak;

/* ==== the jailbreak (rescue): turn-based ====================================================== */
#define RS_W 15                   /* the grid, right of the bat's panel: all the screen holds */
#define RS_H 10
#define RS_TILE 16
#define RS_PANEL_W 76             /* the left side: the bat, its health, its two skills */
#define RS_X0 78
#define RS_Y0 20
#define RS_MAX_GUARDS 10
#define RS_MAX_SHOTS 24
#define RS_MAX_GRENS 6
#define RS_SHOT_SPEED 3           /* tiles a guard's shot flies each turn */
#define RS_SLEEP_T 3              /* a darted guard wakes three turns later */
#define RS_CLOAK_T 5              /* Sable's cloak lasts at most five turns ... */
#define RS_CLOAK_CD 4             /* ... and recharges for the turns it was on, four at most */
#define RS_CARD_HP 2              /* the keycard stops two shots */
/* the guards, each kind in its own colour:
 *   GUN     (green)  walks a beat; seen, chases and fires a shot after each move
 *   KNIFE   (green)  walks a beat; seen, closes in every turn and stabs
 *   SENTRY  (red)    stands still; seen, shoots at the bat along any open line
 *   GREN    (orange) walks a beat; seen, backs off and lobs grenades (they hurt its own side too)
 *   JET     (purple) turns on the spot; seen, shoots once, then jumps to two tiles off the bat
 *   GATLING (grey)   turns on the spot; seen, spins up a turn, then fires four turns the same way */
enum { RK_GUN, RK_KNIFE, RK_SENTRY, RK_GREN, RK_JET, RK_GATLING, RK_COUNT };
/* POST on its beat or watch; ALERT onto the bat; SEEK going to look at a
 * noise; SLEEP darted; SPIN and FIRE the gatling's wind-up and volley */
enum { RGS_POST, RGS_ALERT, RGS_SEEK, RGS_SLEEP, RGS_SPIN, RGS_FIRE };
typedef struct { uint8_t alive, kind, state, hp; int8_t x, y, dir, sleep, tx, ty, hx, hy, hdir, t, cd, flash; } EkoRGuard;
typedef struct { uint8_t alive, age; int8_t x, y, dir; } EkoRShot;   /* a guard's shot in flight */
typedef struct { uint8_t alive; int8_t x, y, fuse; } EkoRGren;       /* a grenade on the floor, fuse burning */
extern const int EKO_RS_HP[OWL_COUNT];      /* 3, 2, 4, 2 */
extern const int EKO_RS_CD[OWL_COUNT][2];   /* turns each skill takes to come back */
extern const char *const EKO_RS_SKILL[OWL_COUNT][2];
extern const int EKO_RK_HP[RK_COUNT];       /* 2, the gatling 4 */
typedef struct {
    char tile[RS_H][RS_W];       /* '#' wall '.' floor 'o' crate 'C' cell door 'K' keycard */
    int ox, oy, dir, hp;
    int cd[2];                   /* turns until skill A / B is ready again (0: ready) */
    int owl, captive;
    bool card, won, lost, brace;
    int cloak, cloak_used;       /* Sable's cloak: turns left on it, turns it has been on */
    int card_hp;                 /* the keycard's two health (only Vesper's shots go through it once spent) */
    EkoRGuard g[RS_MAX_GUARDS];
    EkoRShot shot[RS_MAX_SHOTS];
    EkoRGren gren[RS_MAX_GRENS];
    int ng, turns, kos, noise_t, noise_x, noise_y;
    int shot_t, shot_x, shot_y, shot_x2, shot_y2; /* the bat's last shot or dash, for the screen */
    int boom_t, boom_x, boom_y;                   /* the last grenade blast, for the screen */
    int gshots, ff, hits, grens; /* guard shots fired; guards downed by their own side; hits on the bat; grenades thrown */
    int strikes, stabs;          /* the bat's close blows (stepping into a guard); knife guards' stabs */
    int cell_x, cell_y, key_x, key_y, start_x, start_y;
    int rooms, ndoor;            /* how the jail was rolled: 1-3 rooms, the doorways between them */
    int8_t door_x[6], door_y[6];
    int msg_t;
    int seed;
} EkoRescue;
int eko_rescue_gen(EkoRescue *r, uint32_t seed, int owl, int captive);
int eko_rescue_check(const EkoRescue *r);  /* 0 = a fair layout */
void eko_rescue_turn(EkoRescue *r, int action); /* RA_* */
enum { RA_NONE, RA_RIGHT, RA_DOWN, RA_LEFT, RA_UP, RA_A, RA_B };
int eko_rescue_sees(const EkoRescue *r, int gi);

/* ==== the whole run ========================================================================= */
enum {
    ES_TITLE, ES_STORY, ES_HQ, ES_PICK_RESCUER, ES_BRIEF, ES_CARD, ES_PLAY, ES_CLEAR, ES_CAPTURED,
    ES_RESCUE_WON, ES_RESCUE_LOST, ES_OVER, ES_ENDING
};

typedef struct {
    int lives, hp;
    bool captured, cleared;
} EkoOwlState;

typedef struct {
    uint32_t magic;
    uint32_t best[3];            /* best scores: Marseille, the Andes, Cairo */
    uint16_t rescues;            /* successful rescues */
    uint16_t runs, wins, cherries;
    uint8_t pad[4];
} EkoSave;

typedef struct {
    int state, state_t, frame_t;
    int menu, hq_sel, pick_sel;
    Rng rng;
    /* the run */
    EkoOwlState owl[OWL_COUNT];
    int lives_lost;              /* every emptied health bar of the run */
    int mission, sec, cur_owl, mode;
    int score;                   /* this mission's points */
    int next_life;
    int run_score;
    int rescue_for, rescue_by;
    bool won, in_finale;
    int finale_cards;
    int clear_bonus;
    /* input for the cartridge's own logic (so the demo player's look-ahead
     * can run it with buttons of its choosing) */
    uint16_t in, prev;
    /* the pieces */
    EkoBrawl br;
    EkoGallery ga;
    EkoBike bk;
    EkoSneak sn;
    EkoRescue rs;
    EkoPart part[EKO_MAX_PARTS];
    int shake;
    char msg[48];
    int msg_t;
    /* tests */
    bool god;
    int hits_taken;
} EkoGame;
extern EkoGame eg;
extern EkoSave egs;
extern bool eko_simulating;
extern bool eko_sim_done;

/* input helpers for game logic */
static inline bool ek_btn(int m) { return (eg.in & m) != 0; }
static inline bool ek_btnp(int m) { return (eg.in & ~eg.prev & m) != 0; }
static inline bool ek_btnr(int m) { return (~eg.in & eg.prev & m) != 0; }

/* eko.c */
void eko_sfx(const char *name);
void eko_add_score(int pts);
void eko_say(const char *m);
void eko_burst(int x, int y, int col, int n);
void eko_hurt(int dmg);          /* the current bat takes a hit */
void eko_life_lost(void);        /* the bar is empty: the piece's death is over */
void eko_section_done(void);
int eko_hp(void);
void eko_set_hp(int hp);
void eko_save_now(void);
void eko_step_play(void);        /* one frame of the current piece with eg.in */
void eko_parts_update(void);

/* eko_brawl.c */
void eko_brawl_start(int map);
void eko_brawl_update(void);
void eko_brawl_respawn(void);
bool eko_brawl_solid(int tx, int ty);
int eko_brawl_spawn_foe(int kind, int x, int y);
int eko_brawl_add_obj(int kind, int x, int y, int vx, int vy);
void eko_brawl_boss_start(void);

/* eko_gallery.c */
void eko_gallery_start(int script);
void eko_gallery_update(void);
int eko_gallery_spawn(int kind, int row, int x, int dir);
void eko_gallery_boss_start(void);
void eko_gallery_fire(int x, int y);

/* eko_bike.c */
void eko_bike_start(int course);
void eko_bike_update(void);
int eko_bike_spawn(int kind, int wx, int y, int arg);
void eko_bike_boss_start(void);

/* eko_sneak.c */
void eko_sneak_start(int map, int zone);
void eko_sneak_resume(int zone);
void eko_sneak_update(void);
void eko_sneak_respawn(void);
bool eko_sneak_solid_px(int x, int y);
bool eko_sneak_wall(int tx, int ty);
bool eko_sneak_los(int x0, int y0, int x1, int y1);
int eko_sneak_guard_sees(int gi);
void eko_sneak_enter_room(int rx, int ry);
void eko_sneak_noise(int x, int y);

/* eko_bot.c: the demo player */
int eko_bot_buttons(void);
extern int eko_bot_cherry;     /* 1: never take a risk that could cost a life */
int eko_rescue_bot_action(const EkoRescue *r);

/* eko_draw.c */
void eko_draw(void);
void eko_draw_label(int x, int y, int w, int h, int t);
int eko_layout_audit(void);

/* art and sound */
enum {
    SP_FLINT_STAND, SP_FLINT_WALK1, SP_FLINT_WALK2, SP_FLINT_PUNCH, SP_FLINT_KICK, SP_FLINT_DUCK,
    SP_FLINT_LOW, SP_FLINT_JUMP, SP_FLINT_AIRKICK, SP_FLINT_CHARGE, SP_FLINT_CARRY, SP_FLINT_HURT,
    SP_TROOP_STAND, SP_TROOP_WALK, SP_TROOP_PUNCH, SP_TROOP_AIM, SP_GREN, SP_GREN_THROW, SP_JET1, SP_JET2,
    SP_KEYMAN, SP_BARREL, SP_MANGO, SP_LIFE, SP_KEY, SP_SCRAPJAW, SP_SCRAPJAW2, SP_TALPA, SP_TALPA2,
    SP_VESPER, SP_VESPER_MOVE, SP_VESPER_FIRE, SP_VESPER_ROLL, SP_GTROOP, SP_GTROOP_AIM, SP_GRUNNER, SP_GMORTAR,
    SP_RAMROD, SP_RAMROD2, SP_VULTRA, SP_VULTRA2, SP_COIL,
    SP_BIKE1, SP_BIKE2, SP_CONE, SP_HURDLE, SP_BRUNNER, SP_BIKER, SP_HOG, SP_HOG2, SP_MINE,
    SP_SABLE_R, SP_SABLE_D, SP_SABLE_L, SP_SABLE_U, SP_SABLE_R2, SP_SABLE_D2, SP_SABLE_L2, SP_SABLE_U2,
    SP_GUARD_R, SP_GUARD_D, SP_GUARD_L, SP_GUARD_U, SP_GUARD_R2, SP_GUARD_D2, SP_GUARD_L2, SP_GUARD_U2,
    SP_CAMERA, SP_CIPHER, SP_CIPHER2, SP_IT_DART, SP_IT_AMMO, SP_IT_STUN, SP_IT_GREN, SP_IT_CARD,
    SP_FACE_FLINT, SP_FACE_VESPER, SP_FACE_DART, SP_FACE_SABLE, SP_FACE_HUSH, SP_FACE_COIL,
    SP_RS_FLINT, SP_RS_VESPER, SP_RS_DART, SP_RS_SABLE, SP_RS_GUARD, SP_RS_GUARD2, SP_RS_CRATE, SP_RS_CELL,
    SP_COUNT
};
extern Sprite eko_spr[SP_COUNT];
/* the gallery's foes at the far and middle rows, made once from the near ones */
extern Sprite eko_spr_half[SP_COUNT], eko_spr_3q[SP_COUNT];
void eko_art_load(void);
int eko_art_check(void);
void eko_audio_load(void);
extern int EKO_MUS_TITLE, EKO_MUS_STORY, EKO_MUS_HQ, EKO_MUS_BRIEF, EKO_MUS_DOCKS, EKO_MUS_ANDES, EKO_MUS_CAIRO,
    EKO_MUS_ISTANBUL, EKO_MUS_RESCUE, EKO_MUS_BOSS, EKO_MUS_CALDERA, EKO_MUS_LAB, EKO_MUS_FLIGHT, EKO_MUS_FINAL,
    EKO_MUS_CLEAR, EKO_MUS_CAPTURED, EKO_MUS_OVER, EKO_MUS_ENDING, EKO_MUS_LIFT;

/* words */
extern const char *const EKO_STORY[];
extern const int EKO_STORY_LINES;
extern const char *const EKO_BRIEF[MI_COUNT][4];
extern const char *const EKO_ENDING[];
extern const int EKO_ENDING_LINES;

#endif
