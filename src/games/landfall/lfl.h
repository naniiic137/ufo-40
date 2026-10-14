/* LANDFALL - cartridge 08 of UFO 40, a tribute to Planet Zoldath (UFO 50 #08).
 * See docs/games/08-landfall.md. Shared types for every lfl_*.c file.
 *
 * A planet is built fresh for every run: a grid of flip-screen rooms (7 x 4
 * or 6 x 5) with a ring of side rooms behind gates, and a dungeon. All
 * gameplay maths is integer: positions in 1/256 px (LFL_FX). */
#ifndef LFL_H
#define LFL_H

#include <ctype.h>
#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define LFL_FX 256
#define LFL_PX(v) ((v) >> 8)
#define LFL_TILE 16
#define LFL_RW 18 /* room width in tiles */
#define LFL_RH 9  /* room height in tiles */
#define LFL_ROOM_PW (LFL_RW * LFL_TILE) /* 288 */
#define LFL_ROOM_PH (LFL_RH * LFL_TILE) /* 144 */
#define LFL_VIEW_X 16 /* the room sits here on screen: a tile of each side neighbour shows */
#define LFL_VIEW_Y 16
#define LFL_PLAY_H 160 /* the HUD is below */

/* the planet */
#define LFL_GMAX_W 9 /* grid plus the ring of side rooms */
#define LFL_GMAX_H 7
#define LFL_OVER_ROOMS (LFL_GMAX_W * LFL_GMAX_H)
#define LFL_DUN_W 6
#define LFL_DUN_H 6
#define LFL_DUN_ROOMS 6 /* rooms in the dungeon's line */
#define LFL_MAX_ROOMS (LFL_OVER_ROOMS + LFL_DUN_W * LFL_DUN_H)
#define LFL_MAX_SPAWNS 14
#define LFL_MAX_THINGS 40
#define LFL_MAX_MOBS 20
#define LFL_MAX_SHOTS 24
#define LFL_MAX_DROPS 32
#define LFL_MAX_BOMBS 4
#define LFL_MAX_BLASTS 8
#define LFL_MAX_FLESH 24
#define LFL_MAX_GATES 8
#define LFL_RES_CAP 63
#define LFL_HP_START 3
#define LFL_HP_MAX 7
#define LFL_CHERRY_FRAMES (20 * 60 * 60)

enum { AREA_OVER = 0, AREA_DUN = 1 };
enum { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT };

/* tiles */
enum {
    T_FLOOR = 0, T_WOOD, T_STONE, T_ROCK, T_PLANT, T_HUT, T_FOLIAGE, T_CRYSTAL, T_FLESH, T_TECH,
    T_TECHDOWN, T_SWAMP, T_WARP, T_PIT, T_HOLE, T_LOG, T_STAIRS_DN, T_STAIRS_UP, T_SECRET, T_SHIP,
    T_VOID, T_COUNT
};

/* the eight tools */
enum { TL_NONE = -1, TL_DISC = 0, TL_RIVET, TL_HATCHER, TL_PODS, TL_THUMPER, TL_BOOTS, TL_CHOPS, TL_TALKBOX, TL_COUNT };
/* the four resources (their rarity is rolled per planet) */
enum { RS_BEAD = 0, RS_BURR, RS_FERN, RS_SPORE, RS_COUNT };
enum { TIER_COMMON = 0, TIER_LESS, TIER_MORE, TIER_RARE };

/* gate kinds: what the obstacle is (and so the tool that opens it) */
enum { GK_SWAMP = 0, GK_PIT, GK_HOLE, GK_CRYSTAL, GK_FOLIAGE, GK_TECH, GK_SNAIL, GK_COUNT };
/* what a gate guards */
enum { GR_MAP = 0, GR_DUNGEON, GR_ITEM, GR_RARE, GR_DUN };

/* secrets: the tool that opens the hidden wall */
enum { SK_THUMPER = 0, SK_DISC, SK_PODS, SK_RIVET, SK_SMALL, SK_COUNT };

/* room kinds */
enum { RK_NONE = 0, RK_STONE, RK_FIELD, RK_SHIP, RK_VILLAGE, RK_SIDE, RK_DUN };

/* spawn types (things that come back when the room resets) */
enum { SPW_MOB = 1, SPW_RES, SPW_SNAIL };

/* persistent things */
enum { TH_TOOL = 1, TH_CUBE, TH_MAP, TH_BUNDLE };

/* creature movement / attack / death rolls */
enum { MV_STILL = 0, MV_WANDER, MV_CHASE };
enum { AT_NONE = 0, AT_FOUR, AT_AIM };
enum { DE_NONE = 0, DE_EXPLODE, DE_SWAMP };

/* species slots on a planet: 0-1 friendly, 2-5 hostile, then the fixed ones */
#define SP_FRIEND_A 0 /* speaks plainly */
#define SP_FRIEND_B 1 /* speaks in glyphs */
#define SP_HOSTILE0 2
#define SP_SNAIL 6
#define SP_GNAT 7
#define SP_COUNT 8
#define LFL_LOOKS 10

/* mob flags */
enum { MF_FRIENDLY = 1, MF_ANGRY = 2, MF_GIVER = 4, MF_MORPHED = 8, MF_SHELL = 16, MF_GATE = 32, MF_LURED = 64, MF_MUTE = 128 };

/* shots */
enum { SH_RIVET = 1, SH_DISC, SH_HATCH, SH_FOE, SH_SHARD };
/* room-only drops */
enum { DR_RES = 1, DR_CHOP };

/* trades */
enum { TR_RES = 0, TR_ITEM, TR_MAP };

typedef struct LflSpawn {
    uint8_t type, sub, tx, ty, taken;
} LflSpawn;

typedef struct LflRoom {
    uint8_t exists, kind, area, gx, gy;
    uint8_t open;              /* bit per DIR_*: a way through that side */
    uint8_t op_lo[4], op_hi[4]; /* the opening's tiles along that side */
    uint8_t t[LFL_RH][LFL_RW];
    uint8_t tmpl;       /* which interior template (for tests / hints) */
    uint8_t village;    /* index into villages + 1, 0 = none */
    uint8_t dirty;      /* something was taken: reset when far away */
    uint8_t seen;
    int8_t secret_x, secret_y, secret_key; /* -1 = no secret here */
    int8_t dun_step;    /* order in the dungeon's line */
    LflSpawn spawn[LFL_MAX_SPAWNS];
    uint8_t nspawn;
} LflRoom;

typedef struct LflThing {
    uint8_t alive, kind, sub, n;
    uint8_t room;
    int16_t x, y; /* px, room-local, top-left of a 12 x 12 box */
    int16_t cd;   /* pickup cooldown after a drop */
} LflThing;

typedef struct LflGate {
    uint8_t kind, role, room, used;
    int8_t tx, ty, w, h;   /* obstacle tiles */
    int8_t side;            /* the side of the room the passage leaves by */
    int32_t sx, sy;         /* the snail (fx) */
    uint8_t snail;          /* 0 at home, 1 lured away for good, 2 a shell */
    uint8_t snail_face;
    uint8_t prize_thing;    /* thing index behind it, 255 = none */
    uint8_t snail_room;     /* the room the snail stands in, at the mouth of the way */
} LflGate;

typedef struct LflTrade {
    uint8_t kind, room, pay_res, pay_n, get_res, get_n, item, done, glyph;
} LflTrade;

typedef struct LflVillage {
    uint8_t room, glyph;
    int8_t trade;     /* -1 = the hint-giver */
    int8_t gx, gy;    /* the giver's tile */
} LflVillage;

typedef struct LflSpecies {
    uint8_t look, friendly, glyph;
    uint8_t move, attack, hp, dmg, death;
    int8_t drop;      /* resource, or -1 = a chop (meat) */
    uint8_t fire_every; /* frames / 4 */
    uint8_t speed;    /* 1/256 px a frame */
} LflSpecies;

typedef struct LflPlanet {
    uint32_t seed;
    uint8_t gw, gh;           /* the grid, without the ring */
    uint8_t ship_room;
    uint8_t ship_tx, ship_ty;
    uint8_t tier_of[RS_COUNT];
    uint8_t res_of_tier[4];
    uint8_t cost_res[TL_COUNT]; /* for pods, thumper, chops; 255 = free */
    uint8_t ground_col, ground_dark, wall_col, plant_col; /* the planet's colours */
    LflSpecies sp[SP_COUNT];
    LflRoom room[LFL_MAX_ROOMS];
    LflThing thing[LFL_MAX_THINGS];
    int nthing;
    LflGate gate[LFL_MAX_GATES];
    int ngate;
    LflTrade trade[6];
    LflVillage village[7];
    int nvillage;
    uint8_t item_where[TL_COUNT]; /* 0 open, 1 behind a gate, 2 traded */
    uint8_t dun_room[LFL_DUN_ROOMS];
    uint8_t dun_entry_room;       /* the side room with the stairs */
    uint8_t ncubes;
    uint8_t secret_room;          /* 255 = none */
    uint8_t secret_key;
    int8_t warp_room[2], warp_x[2], warp_y[2]; /* the teleporting swamp pair */
    uint8_t mute_room;            /* the glyph speaker who stays untranslated */
    uint8_t gen_tries;
} LflPlanet;

typedef struct LflPlayer {
    int32_t x, y;   /* fx, room-local, hitbox top-left */
    int face;
    int hp, hpmax;
    int inv;
    int8_t hold[2];
    int act;
    int shrink_t, grow_wait;
    int a_held, b_held, b_used;
    int fire_cd;
    int lock_face;
    int walk_t;
    int hurt_flash;
    int swamp_hurt;
} LflPlayer;

typedef struct LflMob {
    uint8_t alive, sp, flags;
    int32_t x, y; /* fx, hitbox top-left */
    int w, h;
    int hp, t, fire_t, dir, face, morph_t, hurt_t, talk_t;
    int16_t spawn, gate, line;
} LflMob;

typedef struct LflShot {
    uint8_t alive, kind, returning;
    int32_t x, y, vx, vy;
    int life, bounces, dmg;
    int8_t carry_drop, carry_thing;
} LflShot;

typedef struct LflDrop {
    uint8_t alive, kind, sub, cooked;
    int32_t x, y;
    int16_t spawn, t, cd;
} LflDrop;

typedef struct LflBomb {
    uint8_t alive;
    int32_t x, y;
    int t;
} LflBomb;

typedef struct LflBlast {
    uint8_t alive, kind; /* 0 pod/creature blast, 1 shard burst */
    int x, y, t, r;
} LflBlast;

typedef struct LflFlesh {
    uint8_t alive, tx, ty;
    int t;
} LflFlesh;

/* game states */
enum { LS_TITLE = 0, LS_LANDING, LS_PLAY, LS_TALK, LS_SCROLL, LS_DYING, LS_OVER, LS_LIFTOFF, LS_ENDING };

typedef struct LflGame {
    int state, state_t, frame_t;
    LflPlayer pl;
    int cur;             /* current room */
    int prev_room;
    uint32_t run_t;      /* the in-game timer, frames */
    int res[RS_COUNT];
    int res_flash[RS_COUNT];
    int maps;            /* chart pieces held */
    uint8_t map_from;    /* bits: 1 gate, 2 trade, 4 dungeon */
    int cubes_found;
    uint8_t tools_found; /* bits */
    int items_found;
    bool won;
    LflMob mob[LFL_MAX_MOBS];
    LflShot shot[LFL_MAX_SHOTS];
    LflDrop drop[LFL_MAX_DROPS];
    LflBomb bomb[LFL_MAX_BOMBS];
    LflBlast blast[LFL_MAX_BLASTS];
    LflFlesh flesh[LFL_MAX_FLESH];
    int pulse_flash;
    /* the screen scroll between rooms */
    int scroll_dir, scroll_t, scroll_to;
    /* talking */
    char talk[3][64];
    int talk_lines, talk_trade, talk_glyph, talk_t, talk_kind;
    int nope_t;
    char msg[48];
    int msg_t;
    /* tests and the demo player */
    bool god, no_mobs, no_timer;
    int hits_taken, kills, gate_opened;
    int last_hurt;
    int deaths;
    int warps_taken;
    int mosquitos;
    int menu;
    int warp_lock, stairs_lock, axis, ship_msg;
    int joke_t;
} LflGame;

typedef struct LflSave {
    uint32_t magic;
    uint16_t runs, wins, deaths;
    uint8_t most_items, most_energy;
    uint8_t found_map;     /* the gift: a chart piece found */
    uint8_t pad[3];
    uint32_t best_time;    /* frames, 0 = none */
} LflSave;

extern LflPlanet lp;
extern LflGame lg;
extern LflSave lsv;

/* lfl_gen.c */
void lfl_generate(uint32_t seed);
int lfl_room_index(int area, int gx, int gy);
int lfl_neighbor(int room, int dir); /* -1 = none */
bool lfl_winnable(void);             /* the progression check */
int lfl_gen_audit(void);             /* 0 = every rule holds */
int lfl_tile(int room, int tx, int ty);
const char *lfl_hint_line(int i);
const char *const *lfl_tool_names(void);
const char *lfl_res_name(int r);
int lfl_trade_price(int kind, int tier);

/* lfl_play.c */
void lfl_enter_room(int room, bool reset_far);
void lfl_play_update(void);
void lfl_scroll_update(void);
bool lfl_solid_for_player(int tile, bool shrunk);
bool lfl_tile_solid_at(int px, int py, bool shrunk);
void lfl_hurt_player(int dmg, int why);
extern int lfl_death_cause[8]; /* tests: what each death was */
void lfl_heal(int n);
int lfl_add_res(int r, int n);
void lfl_give_tool_drop(int tool, int x, int y);
int lfl_count_mobs(int flags_any, int flags_none);
void lfl_room_reset(int room);
void lfl_explode(int x, int y, int r, int dmg_player, int dmg_mob, bool from_creature);
int lfl_spawn_mob(int sp, int x, int y, int spawn);
void lfl_open_secret(void);
bool lfl_gate_open(int g);
bool lfl_near_ship(void);
int lfl_talk_target(void);
void lfl_say(const char *m);
void lfl_sfx(const char *name);

/* lfl.c */
void lfl_start_talk(int mob);
void lfl_win(void);
void lfl_take_map(int from);

/* lfl_draw.c */
void lfl_draw(void);
void lfl_draw_label(int x, int y, int w, int h, int t);
void lfl_glyph_text(const char *s, int x, int y, int col);

/* lfl_art.c */
enum {
    SPR_WICK_D0, SPR_WICK_D1, SPR_WICK_U0, SPR_WICK_U1, SPR_WICK_R0, SPR_WICK_R1, SPR_WICK_SMALL,
    SPR_SHIP, SPR_SHIP_FLAME,
    SPR_LOOK0, /* 10 looks x 2 frames */
    SPR_SNAIL = SPR_LOOK0 + LFL_LOOKS * 2, SPR_SNAIL2, SPR_SHELL, SPR_GNAT0, SPR_GNAT1, SPR_CHICK0, SPR_CHICK1,
    SPR_TOOL0, /* 8 tools */
    SPR_CUBE = SPR_TOOL0 + TL_COUNT, SPR_MAP, SPR_RES0, /* 4 resources */
    SPR_CHOP = SPR_RES0 + RS_COUNT, SPR_CHOP_COOKED, SPR_BUNDLE,
    SPR_RIVET, SPR_DISC0, SPR_DISC1, SPR_HATCH, SPR_FOESHOT, SPR_SHARD, SPR_POD0, SPR_POD1,
    SPR_TALK, SPR_COUNT
};
extern Sprite lfl_spr[SPR_COUNT];
void lfl_art_load(void);
int lfl_art_check(void);
int lfl_layout_audit(void);

/* lfl_audio.c */
extern int LFL_MUS_TITLE, LFL_MUS_LAND, LFL_MUS_PLANET, LFL_MUS_DUNGEON, LFL_MUS_LIFTOFF, LFL_MUS_ENDING,
    LFL_MUS_OVER, LFL_MUS_CHART;
void lfl_audio_load(void);

/* lfl_bot.c */
int lfl_bot_buttons(void);
void lfl_bot_reset(void);
extern int lfl_bot_task, lfl_bot_tx, lfl_bot_ty;

#endif
