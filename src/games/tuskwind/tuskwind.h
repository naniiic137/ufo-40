/* TUSKWIND - shared declarations. Cartridge 21 of UFO 40.
 * A tribute to Waldorf's Journey (UFO 50 #21); see docs/games/21-tuskwind.md.
 *
 * Burl the walrus is asleep, and in his dream he crosses a chain of
 * floating islets towards a door at the far end. Every jump is aimed with a
 * cursor and charged before he leaves the ground; in the air only his
 * flippers can steer him, and they run on a stamina bar that only sprats
 * fill. Shells are money and score. Terns from six lighthouses carry him
 * out of the sea. The map is new every run. 1P journey; 2P versus brawl. */
#ifndef TUSKWIND_H
#define TUSKWIND_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- the screen ---------------------------------------------------------- */
#define TKW_HUD_H 12          /* the strip across the top */
#define TKW_VIEW_H (SCREEN_H - TKW_HUD_H)

/* ---- the body (Burl, and each brawler) ------------------------------------ */
#define TKW_HW 6              /* half the body's width */
#define TKW_BH 12             /* the body's height above its feet */
#define TKW_G 0.11f           /* gravity, px a frame a frame */
#define TKW_G_KITE 0.035f     /* under the kite */
#define TKW_VMIN 1.2f         /* launch speed at no charge */
#define TKW_VMAX 4.6f         /* ... and at a full charge */
#define TKW_CHARGE_T 60       /* frames to a full charge (it then holds) */
#define TKW_AIM_MIN (-45.0f)  /* the cursor's arc, degrees above flat, facing */
#define TKW_AIM_MAX 90.0f
#define TKW_AIM_STEP 1.5f     /* degrees a frame while UP or DOWN is held */
#define TKW_AIM_START 45.0f
#define TKW_WALK 0.6f         /* the scoot */
#define TKW_LUNGE_DECEL 0.07f /* a flat jump slides along the ground */
#define TKW_FLAP 0.22f        /* flipper thrust */
#define TKW_FLAP_RISE 1.8f    /* flapping never climbs faster than this */
#define TKW_FLAP_MAXVX 3.0f   /* nor pushes sideways past this */
#define TKW_FALL_MAX 5.0f
#define TKW_STAMINA 480       /* the bar, in half frames of flapping (240 frames) */
#define TKW_SPRAT 120         /* a sprat: a quarter of the bar */
#define TKW_WIND 0.014f       /* the wind's push in the air */
#define TKW_WIND_FIRST 2400   /* the first natural wind dies out after this */
#define TKW_FOAM_T 72         /* a foam islet holds this long once stood on */
#define TKW_FOAM_BACK 300     /* ... and forms again this long after */
#define TKW_MAGNET 30         /* pickups drift in from this far */
#define TKW_SPIN_FUEL 360     /* the spinner's own bar */
#define TKW_SPIN_SPEED 1.4f

/* ---- the world ----------------------------------------------------------- */
#define TKW_MAP_W 8000        /* the journey, start to door */
#define TKW_MAP_H 360
#define TKW_SEA_Y 344         /* below this the sea has you */
#define TKW_MANTA_Y 40        /* the band the mantas glide in */
#define TKW_HALL_W 540
#define TKW_HALL_EDGE 236     /* the floor's west end, over the chasm */
#define TKW_HALL_DOOR 360     /* where he comes in */
#define TKW_HALL_SHAFT 456    /* the middle of the shaft up through the vault */
#define TKW_HALL_ELDER 512
#define TKW_BRAWL_W SCREEN_W
#define TKW_BRAWL_H SCREEN_H
#define TKW_BRAWL_SEA 172

#define TKW_MAX_PLATS 340
#define TKW_MAX_THINGS 420
#define TKW_MAX_MANTAS 12
#define TKW_BUCKET_W 64
#define TKW_BUCKETS (TKW_MAP_W / TKW_BUCKET_W + 2)
#define TKW_BUCKET_N 16

enum { PK_LEDGE, PK_ROCK, PK_FOAM };   /* one-way, solid, crumbling */

typedef struct {
    int16_t x, y, w, h;   /* top-left corner and size */
    uint8_t kind;
    uint8_t route;        /* on the chain the generator laid from start to door */
    uint8_t gone;         /* a foam islet that has crumbled */
    uint8_t door;         /* the last islet: the door down to the hall */
    int16_t timer;        /* foam: frames since first stood on (-1 = not yet) */
    int16_t back;         /* foam: frames until it forms again */
    int16_t brawl_t;      /* brawl: frames of crumbling left (0 = solid) */
} TkwPlat;

enum {
    TH_COCKLE, TH_WHELK, TH_KEY, TH_SPRAT, TH_SPIRAL,  /* pickups */
    TH_CHEST, TH_LIGHTHOUSE, TH_SHOP, TH_SIGN, TH_BELL, TH_RAM,
    TH_PERCH,      /* the hall: a tern that came along, perched */
    TH_ELDER,      /* the hall: the old one */
    TH_SECRET,     /* the hidden sign in the top-left corner */
    TH_KINDS
};
#define TKW_IS_PICKUP(k) ((k) <= TH_SPIRAL)

/* a ram's moods */
enum { RAM_IDLE, RAM_ALERT, RAM_CHARGE, RAM_REST, RAM_FALL };

typedef struct {
    uint8_t kind, alive, state;
    int16_t plat;          /* the islet it stands on (-1 = in the air) */
    float x, y, vx, vy;    /* x = middle, y = bottom */
    int t, arg;            /* sign: its number; shop: its two wares; chest: opened */
    int dir;
} TkwThing;

typedef struct {
    float cx, y0, span, phase, speed;
    uint8_t alive;
    float x, y, px;        /* this frame and the last */
} TkwManta;
#define TKW_MANTA_HW 12
#define TKW_MANTA_HH 4

typedef struct {
    int w, h, sea_y;
    bool wrap;             /* the brawl screen wraps left to right */
    int nplat, nth, nmanta;
    TkwPlat plat[TKW_MAX_PLATS];
    TkwThing th[TKW_MAX_THINGS];
    TkwManta manta[TKW_MAX_MANTAS];
    int start_plat, goal_plat;
    int start_x, goal_x;   /* progress runs from 0 % here to 100 % there */
    int nroute;
    int16_t bucket[TKW_BUCKETS][TKW_BUCKET_N];
    uint8_t bucket_n[TKW_BUCKETS];
} TkwWorld;
extern TkwWorld tkw_w;

/* ---- a body in flight (shared by the game and the demo player's planner) - */
enum { BM_GROUND, BM_AIR, BM_LUNGE, BM_RIDE };
typedef struct {
    float x, y, vx, vy;
    int plat;              /* standing on (BM_GROUND / BM_LUNGE), else -1 */
    int ride;              /* the manta under it (BM_RIDE), else -1 */
    int mode;
    int stamina;
    bool kite;             /* low gravity until it lands */
    int ignore;            /* the islet it just dropped through (a downward jump) */
    int landed;            /* the islet it landed on this frame, -1 = none */
    bool bumped;           /* hit a rock this frame */
} TkwBody;

/* one frame in the air: dx, dy = the flap's direction (0, 0 = no flap) */
void tkw_body_air(TkwBody *b, const TkwWorld *w, float fdx, float fdy, bool flap, float wind);
/* launch from the ground at an aim (degrees above flat) and a charge (0..60) */
void tkw_body_launch(TkwBody *b, const TkwWorld *w, int face, float aim, int charge);
/* a frame on the ground: walking dir (-1, 0, 1) and the flat-jump slide */
void tkw_body_ground(TkwBody *b, const TkwWorld *w, int dir);
/* flapping costs two units a frame, one under the kite */
#define TKW_FLAP_COST(kite) ((kite) ? 1 : 2)
float tkw_launch_speed(int charge);
bool tkw_plat_solid(const TkwWorld *w, int i);
float tkw_plat_top(const TkwWorld *w, int i);
void tkw_buckets_build(TkwWorld *w);
void tkw_bucket_add(TkwWorld *w, int i);

/* ---- items --------------------------------------------------------------- */
enum { IT_BOBBER, IT_SPYGLASS, IT_GRAPNEL, IT_KITE, IT_SPINNER, IT_TIN, IT_COUNT };
extern const char *const TKW_ITEM_NAME[IT_COUNT];
extern const int TKW_ITEM_COST[IT_COUNT];
#define TKW_ITEM_MAX 9

/* ---- what Burl is doing -------------------------------------------------- */
enum {
    PS_PLAY,        /* walking, aiming, charging, flying */
    PS_MENU,        /* B held: the item row */
    PS_THROW,       /* aiming and charging the bobber */
    PS_BALL,        /* watching the bobber fly */
    PS_LOOK,        /* the spyglass */
    PS_ROPE,        /* the grapnel's line going out */
    PS_HAUL,        /* being hauled along it */
    PS_SHOP,        /* at the duchess's stall */
    PS_RESCUE,      /* a tern carries him back */
    PS_SINK,        /* into the sea with no tern: he wakes */
    PS_DOOR         /* through the door */
};

typedef struct {
    TkwBody b;
    int face;              /* 1 right, -1 left */
    float aim;
    int charge;            /* frames held */
    bool charging;
    bool flap_ok;          /* A has been let go since the launch */
    bool flapping;
    int ps, ps_t;
    int takeoff;           /* the islet of the last jump (the terns bring him back there) */
    float takeoff_x;
    int menu_sel;          /* 0 = the cross (cancel), 1.. = items owned */
    int red_line;          /* the bobber's charge, shown on the meter (-1 none) */
    bool spinner;
    int spin_fuel;
    /* the bobber */
    TkwBody ball;
    int ball_charge, ball_t;
    /* the grapnel */
    float rope_x, rope_y, rope_dx, rope_dy, rope_len;
    int rope_plat;
    float look_x, look_y;  /* the spyglass's view */
    int anim;
    int shop_sel, shop_cool;
    int shop_th;
    int hurt_t;            /* knocked: a moment with no control */
} TkwHero;

/* ---- the brawl ----------------------------------------------------------- */
#define TKW_BRAWL_STAMINA 320
typedef struct {
    TkwBody b;
    int face, charge;
    float aim;
    bool charging, flap_ok, flapping, out;
    int hurt_t, wins, anim;
} TkwBrawler;

/* ---- the game's states --------------------------------------------------- */
enum {
    TS_TITLE, TS_INTRO, TS_JOURNEY, TS_HALL, TS_TALK, TS_ENDING, TS_CREDITS, TS_WAKE,
    TS_BRAWL_SETUP, TS_BRAWL, TS_BRAWL_ROUND, TS_BRAWL_OVER
};

/* the cartridge save: records only (a journey is not saved, as in the original) */
typedef struct {
    uint32_t magic;
    uint32_t signs;        /* bit i: sign i has been read (bit 22: the hidden one) */
    uint16_t runs, wins, cherries;
    uint8_t most_terns, most_chests;
    uint8_t best_pct, best_shells;
    uint8_t pad[2];
} TkwSave;

#define TKW_SIGNS 22
#define TKW_TERNS 6
#define TKW_SHELLS_FLOAT 36
#define TKW_CHERRY_SHELLS 50

typedef struct {
    int state, state_t, frame_t;
    int sel;                 /* title menu */
    uint64_t seed;           /* the map's seed */
    uint64_t seed_next;      /* tests: the next journey's seed (0 = any) */
    Rng rng;                 /* the run's own dice (winds, wares, brawl maps) */
    uint16_t prev_in;        /* last frame's buttons */
    /* the run */
    TkwHero h;
    int shells, keys, terns, terns_used, terns_found, chests, spent;
    int inv[IT_COUNT];
    int pct, max_pct;        /* progress, as last landed / best this run */
    int signs_run;           /* signs read this run (bits) */
    int sign_near;           /* the sign Burl is next to (-1) */
    int wind;                /* -1 left, 0 calm, 1 right */
    int wind_t;              /* frames left of a dying wind (0 = it stays) */
    bool wind40, wind80;     /* the natural winds have come */
    int msg_t;
    const char *msg;
    float cam_x, cam_y;
    bool gift;
    bool in_hall;
    int hall_terns;          /* terns that came to the hall */
    int talk_page, talk_t;
    bool cherry_end;
    bool secret_read;
    int rescues;
    int journey_frames;
    /* the brawl */
    int rounds;              /* best of 1, 3, 5, 7 or 9 */
    int round_no, round_winner, crumble_t;
    TkwBrawler br[2];
    int sprat_t;
    /* the effects */
    int shake;
    float rain[40][2];
    /* testing */
    bool god;                /* the sea throws him back up */
    bool bot_collect;        /* the demo player goes after shells */
    bool bot_brawl;          /* the demo player plays the brawl's player 2 */
} TkwGame;
extern TkwGame tkg;
extern TkwSave tks;

/* tuskwind_map.c */
void tkw_gen_journey(uint64_t seed);
void tkw_gen_hall(int terns);
void tkw_gen_brawl(uint64_t seed);
int tkw_add_plat(TkwWorld *w, int x, int y, int wd, int kind);
int tkw_add_thing(TkwWorld *w, int kind, float x, float y, int plat);
int tkw_progress_at(float x);
/* can a jump (calm, no flapping) get from islet a to islet b? */
bool tkw_can_reach(const TkwWorld *w, int a, int b);
void tkw_manta_pos(const TkwManta *m, int t, float *x, float *y);
int tkw_count_things(int kind);

/* tuskwind_play.c */
void tkw_new_journey(uint64_t seed);
void tkw_journey_update(uint16_t in);
void tkw_enter_hall(void);
void tkw_hero_place(int plat, float x);
void tkw_on_land(int plat);
void tkw_add_shells(int n);
void tkw_set_wind(int dir, int frames);
void tkw_sfx(const char *name);
void tkw_say(const char *msg);
bool tkw_can_use(int item);
int tkw_menu_items(int *list);    /* the item row: the items held, in order */
void tkw_camera(bool snap);

/* tuskwind_brawl.c */
void tkw_brawl_start_match(void);
void tkw_brawl_start_round(void);
void tkw_brawl_update(uint16_t in0, uint16_t in1);
int tkw_brawl_bot(void);

/* tuskwind_bot.c: the demo player, for the tests */
int tkw_bot_buttons(void);
void tkw_bot_reset(void);
extern int tkw_bot_plans, tkw_bot_fails;

/* tuskwind_draw.c */
void tkw_draw(void);
void tkw_draw_label(int x, int y, int w, int h, int t);

/* tuskwind_text.c */
extern const char *const TKW_SIGN_TEXT[TKW_SIGNS];
extern const char *const TKW_SECRET_TEXT;
#define TKW_TALK_PAGES_MAX 12
extern const char *const TKW_TALK_GOLD[];
extern const char *const TKW_TALK_CHERRY[];
extern const char *const TKW_INTRO[];
extern const char *const TKW_CREDITS[];
int tkw_talk_pages(bool cherry);
const char *tkw_talk_page(bool cherry, int i, char *buf, int n);

/* art and sound */
enum {
    SP_BURL, SP_BURL_WALK, SP_BURL_CROUCH, SP_BURL_FLAP1, SP_BURL_FLAP2, SP_BURL_SLEEP, SP_BURL_FALL,
    SP_COCKLE, SP_WHELK, SP_KEY, SP_SPRAT, SP_SPIRAL, SP_CHEST, SP_CHEST_OPEN,
    SP_LIGHTHOUSE, SP_TERN1, SP_TERN2, SP_DUCHESS, SP_BELL, SP_BELL_DONE,
    SP_RAM, SP_RAM_RUN, SP_MANTA1, SP_MANTA2, SP_ELDER,
    SP_I_BOBBER, SP_I_SPYGLASS, SP_I_GRAPNEL, SP_I_KITE, SP_I_SPINNER, SP_I_TIN, SP_I_CROSS,
    SP_BALL, SP_KITE_BIG, SP_SPINNER_BIG,
    SP_COUNT
};
extern Sprite tkw_spr[SP_COUNT];
void tkw_art_load(void);
void tkw_audio_load(void);
extern int TKW_MUS_DREAM, TKW_MUS_HALL, TKW_MUS_ENDING, TKW_MUS_BRAWL, TKW_MUS_WAKE, TKW_MUS_ROUND;

#endif
