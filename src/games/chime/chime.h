/* CHIME CIRCUIT - shared declarations. Cartridge 19 of UFO 40.
 * A tribute to The Big Bell Race (UFO 50 #19); see docs/games/19-chime-circuit.md.
 *
 *   chime_flight.c  the chime-ship flight model (on its own, for later cartridges)
 *   chime_tracks.c  the eight tracks, and the map, route and distance fields built from one
 *   chime_race.c    the race rules: damage, slashes, laps, pickups, weapons, places, points
 *   chime_ai.c      the CPU pilots and the demo pilot the tests use
 *   chime.c         the screens, the tournament, saving, the code screen and test hooks
 *   chime_art.c     pixel art          chime_audio.c   music and sounds */
#ifndef CHIME_H
#define CHIME_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#include "chime_flight.h"

#define CHM_TW 40            /* a track is 40 x 22 tiles of 8 px */
#define CHM_TH 22
#define CHM_TILE 8
#define CHM_OY 2             /* the track's top on screen */
#define CHM_NW 80            /* the distance fields: 4 px cells */
#define CHM_NH 44
#define CHM_NC 4
#define CHM_TRACKS 8
#define CHM_RACES 8          /* races in the tournament, one per track, in order */
#define CHM_LAPS 8
#define CHM_SHIPS 6
#define CHM_PILOTS 6
#define CHM_HP 3
#define CHM_ZONES 13         /* the start line, checkpoints 1-9, and x, y, z for the CPUs' lines */
#define CHM_ZONE_S 0
#define CHM_ROUTES 3
#define CHM_ROUTE_LEN 12
#define CHM_STATIONS 4
#define CHM_BULLETS 48
#define CHM_MINES 32
#define CHM_FIRES 12
#define CHM_PARTS 128
#define CHM_FAR 65535

/* times, in frames */
#define CHM_COUNTDOWN 120    /* 3, 2, 1 ... */
#define CHM_MERCY 45         /* no more damage for this long after a hit */
#define CHM_LAUNCH_MERCY 60
#define CHM_WRECK_T 80       /* a wrecked ship's replacement launches after this */
#define CHM_SLASH_T 14
#define CHM_SLASH_CD 24
#define CHM_SLASH_ACTIVE 8   /* the first frames of a slash hit */
#define CHM_STUN 16          /* a knocked ship can't steer for this long (it can still thrust) */
#define CHM_SUPER_STUN 26
#define CHM_HP_SHOW 60       /* the damage meter shows over a ship this long after a change */
#define CHM_HUNT_REST 240    /* a hunter's knock lands: it races on for this long before hunting again */
#define CHM_END_WAIT 120     /* after the last player finishes */

/* the controls a pilot gives in one frame: the flight bits plus a slash */
#define CHM_CTL_SLASH 8

extern const int CHM_POINTS[CHM_SHIPS]; /* 9 7 5 3 2 1 */

/* the five pickups */
enum { PW_NONE, PW_BULLETS, PW_MINES, PW_FIREBALLS, PW_SUPER, PW_PAYLOAD, PW_COUNT };
extern const char *const CHM_POWER_NAME[PW_COUNT];
extern const uint16_t CHM_POWER_T[PW_COUNT];

/* tile shapes: which part of the tile is solid */
enum { SH_AIR, SH_FULL, SH_TL, SH_TR, SH_BL, SH_BR };

/* ---- tracks (chime_tracks.c) ------------------------------------------- */
typedef struct ChmTrackDef {
    const char *name;
    const char *where;       /* a line for the race card */
    const char *const *rows; /* CHM_TH rows of CHM_TW */
    int8_t dir;              /* +1: ships leave the grid going right */
    uint8_t theme;
    uint8_t cpu_routes;      /* the CPUs pick among the first this many routes */
    uint8_t bot_route;       /* the demo pilot's line */
    uint8_t cpu_pace;        /* per cent: the CPUs' speed limits on this track */
    const char *routes[CHM_ROUTES]; /* zones in order, from the line: "S123", "Sx1234" */
} ChmTrackDef;
extern const ChmTrackDef CHM_TRACK[CHM_TRACKS];

typedef struct ChmMap {
    uint8_t track;
    int8_t dir;
    uint8_t shape[CHM_TH][CHM_TW];
    int8_t zone[CHM_TH][CHM_TW];   /* -1: none */
    int8_t boost[CHM_TH][CHM_TW];  /* 0 none, 1 right, 2 left, 3 up, 4 down */
    uint8_t station[CHM_TH][CHM_TW];
    uint8_t ncp;                   /* checkpoints 1..ncp, then the line */
    bool has_zone[CHM_ZONES];
    int16_t zx[CHM_ZONES], zy[CHM_ZONES];    /* each zone's middle, px */
    int launch_c, launch_r;
    int32_t spawn_x, spawn_y;      /* where a ship launches from, 1/256 px */
    uint8_t nroutes;
    uint8_t route[CHM_ROUTES][CHM_ROUTE_LEN];
    uint8_t route_len[CHM_ROUTES];
    uint8_t nav[CHM_NH][CHM_NW];   /* 1: a ship's centre fits here with room to spare */
    uint8_t clear[CHM_NH][CHM_NW]; /* cells to the nearest wall */
    uint16_t field[CHM_ZONES][CHM_NH * CHM_NW]; /* distance to each zone along the track */
    uint16_t gap[CHM_ZONES][CHM_ZONES];          /* field[b] at zone a's middle */
    bool ok;                       /* the map passed its checks */
} ChmMap;
extern ChmMap chm_map;

void chm_map_build(ChmMap *m, int track);
bool chm_solid(const void *ctx, int px, int py);
int chm_zone_at(const ChmMap *m, int px, int py);    /* -1 */
int chm_boost_at(const ChmMap *m, int px, int py);
uint16_t chm_field_at(const ChmMap *m, int zone, int32_t x, int32_t y);
int chm_zone_char(int z);                             /* 'S', '1'.., 'x' */
int chm_route_length(const ChmMap *m, int route);     /* field units round a route */
void chm_grid_pos(const ChmMap *m, int slot, int32_t *x, int32_t *y); /* slot 0: the front */

/* ---- the race (chime_race.c) --------------------------------------------- */
typedef struct ChmShip {
    ChmFlight f;
    uint8_t pilot;
    uint8_t human;           /* 0 a CPU, 1 player one, 2 player two */
    int8_t hp;
    uint8_t laps, cp;        /* laps done; the checkpoint wanted next (ncp + 1: the line) */
    uint8_t alive, finished, place, lost;
    uint8_t parked;          /* tests: taken out of the race altogether */
    uint8_t idle;            /* tests: a CPU that gives no controls */
    uint16_t wreck_t, mercy, stun, slash_t, slash_cd, boost_t;
    uint16_t hp_show;        /* the damage meter pops up while this runs */
    uint8_t slash_hits;      /* a bit for each ship this slash has knocked */
    uint8_t power;
    uint16_t power_t, power_tick;
    int32_t ball_x, ball_y;  /* the payload on its chain */
    uint16_t orbit;          /* the fireballs' angle, 1/256 turns */
    uint8_t route, rz;       /* the pilot's line and the zone it heads for */
    uint8_t ctl, act;        /* this frame's controls; the planner's last pick */
    uint8_t aggro, prey;     /* CPU mood for the race (AG_*) and the ship it hunts */
    uint16_t plan_t;
    uint16_t rest_t;         /* a hunter that has just knocked its prey lets it go this long */
    int16_t skill;           /* a CPU's speed limit, 1/256 px a frame */
    uint8_t grid;
    uint32_t lap_t0, finish_t;
    uint16_t last_lap;
    uint16_t wall_hits, wrecks, knocks;
} ChmShip;

enum { AG_CALM, AG_JOSTLE, AG_HUNT };
/* the CPUs' temper for a whole race */
enum { TEMPER_CALM, TEMPER_MIXED, TEMPER_MEAN };

typedef struct ChmStation {
    int16_t x, y;
    uint8_t state, kind;     /* state: 0 empty, 1 the "!!" warning, 2 a pickup waiting */
    uint16_t t;
} ChmStation;

typedef struct ChmShot { int32_t x, y, vx, vy; uint8_t owner, life, on; } ChmShot;
typedef struct ChmMine { int16_t x, y; uint8_t owner, on; uint16_t t; } ChmMine;
typedef struct ChmFire { int16_t x, y; uint8_t on, owner; uint16_t t; } ChmFire; /* owner 255: nobody's */
typedef struct ChmPart { int16_t x, y, vx, vy; uint8_t life, col, kind, on; } ChmPart; /* 1/16 px */

enum { RP_COUNTDOWN, RP_RUN, RP_OVER };

/* things that happened this frame, for the sounds (chime.c) */
enum {
    EV_WALL = 1 << 0, EV_SLASH = 1 << 1, EV_KNOCK = 1 << 2, EV_WRECK = 1 << 3, EV_LAUNCH = 1 << 4,
    EV_PICK = 1 << 5, EV_WARN = 1 << 6, EV_BOOST = 1 << 7, EV_SHOT = 1 << 8, EV_BLAST = 1 << 9,
    EV_LAP = 1 << 10, EV_FINISH = 1 << 11, EV_BUMP = 1 << 12, EV_HURT = 1 << 13, EV_BEEP = 1 << 14,
    EV_GO = 1 << 15, EV_MINE = 1 << 16, EV_FIRE = 1 << 17, EV_LASTLAP = 1 << 18
};

typedef struct ChmRace {
    uint8_t track, humans, fast_loot;
    uint8_t phase;
    uint8_t temper;          /* TEMPER_*: drawn once per race */
    uint16_t count_t, over_t;
    uint32_t t;              /* frames since GO */
    ChmShip s[CHM_SHIPS];
    uint8_t nst;
    ChmStation st[CHM_STATIONS];
    ChmShot shot[CHM_BULLETS];
    ChmMine mine[CHM_MINES];
    ChmFire fire[CHM_FIRES];
    ChmPart part[CHM_PARTS];
    uint8_t nfinished;
    uint8_t order[CHM_SHIPS];/* the running order, ship indexes */
    uint32_t ev, ev_human;   /* EV_* this frame (anyone / a player's ship) */
    Rng rng;
} ChmRace;

/* The tournament: ship 0 is player one; in 2P ship 1 is player two. */
typedef struct ChmCup {
    uint8_t humans;
    uint8_t race;                      /* the next race, 0..7 (8: all run) */
    uint8_t pilot[CHM_SHIPS];
    uint8_t points[CHM_SHIPS];
    uint8_t place[CHM_RACES][CHM_SHIPS];/* 1..6 */
    uint8_t grid[CHM_SHIPS];           /* the ship in each grid slot, pole first */
    uint8_t fast_loot;
} ChmCup;

void chm_cup_new(ChmCup *c, int humans, const uint8_t *pilots, Rng *rng);
void chm_cup_score(ChmCup *c, const ChmRace *r); /* points, places and the next grid */
int chm_cup_rank(const ChmCup *c, int ship);       /* 1 = leads (ties broken, see the doc) */
int chm_cup_leader(const ChmCup *c);
int chm_cup_wins(const ChmCup *c, int ship);       /* races won */

void chm_race_begin(ChmRace *r, const ChmCup *c, int track, uint64_t seed);
/* one frame; ctl[0], ctl[1] are the two players' controls */
void chm_race_step(ChmRace *r, const unsigned ctl[2]);
uint32_t chm_progress(const ChmRace *r, int ship);  /* for the running order */
void chm_race_hurt(ChmRace *r, int ship);
void chm_race_wreck(ChmRace *r, int ship);
void chm_race_power(ChmRace *r, int ship, int kind);
int chm_ship_by_human(const ChmRace *r, int human); /* -1 */
int chm_sin(int a);  /* a in 1/256 turns; -127..127 */
int chm_cos(int a);
void chm_station_fill(ChmRace *r, int station, int kind);
void chm_race_fire(ChmRace *r, int x, int y);        /* a fire on the course (tests) */

/* ---- pilots (chime_ai.c) ---------------------------------------------- */
/* the controls a CPU pilot (or the demo pilot, bot = true) gives this frame */
unsigned chm_ai(ChmRace *r, int ship, bool bot);
void chm_ai_moods(ChmRace *r);                      /* the CPUs' temper and moods for the race */

/* ---- art & audio ------------------------------------------------------- */
typedef struct ChmPilot {
    const char *name;
    const char *ship;        /* the ship's name */
    const char *line;        /* who they are, for the pilot select */
    uint8_t body, trim;      /* ship colours */
} ChmPilot;
extern const ChmPilot CHM_PILOT[CHM_PILOTS];

enum {
    CA_SHIP, CA_ICON_BULLETS, CA_ICON_MINES, CA_ICON_FIRE, CA_ICON_SUPER, CA_ICON_PAYLOAD, CA_MINE,
    CA_FIREBALL, CA_FIRE1, CA_FIRE2, CA_BALL, CA_ARROW, CA_ARROW_UP, CA_HATCH, CA_CUP, CA_FLAG, CA_COUNT
};
extern Sprite chm_spr[CA_COUNT];
extern Sprite chm_face[CHM_PILOTS];
void chm_art_load(void);
bool chm_art_ok(void);
/* tilt: -1, 0, +1, the ship leans that way (thrust and steer together) */
void chm_draw_ship(int pilot, int x, int y, int face, bool flame, int tilt, int t);
void chm_draw_face(int pilot, int x, int y, int scale);
/* the track: the sky and scenery behind, then the walls, into a 320 x 180 buffer */
void chm_render_track(const ChmMap *m, uint8_t *px);
void chm_draw_mini_track(const ChmMap *m, int x, int y); /* half size: 160 x 88 */

void chm_audio_load(void);
extern int CHM_MUS_TITLE, CHM_MUS_PIT, CHM_MUS_RACE1, CHM_MUS_RACE2, CHM_MUS_RACE3, CHM_MUS_FINALE,
    CHM_MUS_CUP, CHM_MUS_HOME, CHM_MUS_FLAG, CHM_MUS_ALSO;

#endif
