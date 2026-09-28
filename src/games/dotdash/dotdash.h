/* DOT & DASH - shared declarations. Cartridge 45 of UFO 40, a tribute to
 * Mini & Max (UFO 50 #45). See docs/games/45-dot-and-dash.md.
 *
 * One room at four sizes. The full-size lumber room (S0) and the small
 * world (S1) are the same tile map: every cell of the room drawn at 2 px is
 * an 8-px tile once Dot is small. Every tile Dot stands on holds a strip of
 * the micro world (S2), generated from the material and the position, the
 * same for everyone. The fourth size (S3, tiny) is Dot herself made smaller
 * inside the micro world, small enough to fit through one-tile gaps. */
#ifndef DOTDASH_H
#define DOTDASH_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define DD_TS 8            /* tile size at S1..S3 */
#define DD_TS0 2           /* the same room at full size */
#define ROOM_W 160
#define ROOM_H 90
#define CHUNK_W 40         /* one parent tile becomes a chunk this big */
#define CHUNK_H 32
#define STRIP_MAX 48       /* chunks kept around the way in */
#define LV_MAXW (STRIP_MAX * CHUNK_W)
#define LV_MAXH 96
#define DD_MAX_ENTS 200
#define DD_MAX_PARTS 160
#define DEPTH_MAX 4

enum { SC_FULL, SC_SMALL, SC_MICRO, SC_DEEP }; /* SC_DEEP: the tiny body inside a micro level */

/* ---- tiles --------------------------------------------------------- */
enum {
    T_AIR,
    /* solids */
    T_WOOD, T_WOODDK, T_PLANK, T_WALL, T_CERAMIC, T_SOIL, T_FABRIC, T_FABRIC2,
    T_BOOKR, T_BOOKB, T_BOOKG, T_BOOKY, T_BOOKW, T_METAL, T_BRASS, T_GLASS, T_DUST,
    T_CARD, T_CARD2, T_FUR, T_CELL, T_CELL2, T_STONE, T_PENCIL, T_ROOT, T_SHADE, T_WAX,
    T_MOSS, T_CRACK, T_GLUE, T_GEAR, T_BONE,
    /* one-way ledges */
    T_LEAF, T_THREAD, T_LEDGE, T_SHROOM, T_STRAND,
    /* hazards (not solid) */
    T_THORN, T_GOO, T_SLIME,
    T_COUNT
};
enum { TF_SOLID = 1, TF_ONEWAY = 2, TF_HURT = 4, TF_GOO = 8, TF_BOUNCE = 16, TF_BOMB = 32 };

/* materials: what a tile is made of, and so the world inside it */
enum {
    M_NONE, M_WOOD, M_WALL, M_CERAMIC, M_SOIL, M_FABRIC, M_PAPER, M_METAL, M_GLASS,
    M_DUST, M_CARD, M_FUR, M_CELL, M_STONE, M_PLANT, M_WAX, M_COUNT
};

typedef struct TileInfo {
    uint8_t flags, mat, base, hi, lo;
} TileInfo;
extern const TileInfo DD_TILE[T_COUNT];

/* back-wall decoration (drawn only) */
enum {
    BG_NONE, BG_PAPER, BG_DOOR, BG_OUTLET, BG_FRIED, BG_CORD, BG_STALK, BG_CLOCK, BG_FRAME,
    BG_BRACKET, BG_CAVITY, BG_BOX, BG_DARK, BG_BULB, BG_HOLLOW, BG_WINDOW, BG_TRACK
};

/* ---- levels -------------------------------------------------------- */
enum { LV_AREA, LV_STRIP, LV_SPECIAL };

/* S1 areas: the room and the hand-made places inside things */
enum {
    AR_ROOM, AR_THIMBLE, AR_CAVITY, AR_HOLLOW, AR_SIEGE, AR_LAIR, AR_CLOCKWORKS, AR_THRONE, AR_SHAFT, AR_TRAIN,
    AR_COUNT
};
/* hand-made micro places entered by shrinking on a creature or a seam */
enum {
    SP_DASHFUR, SP_PUFFFUR, SP_SPROCKET, SP_KEYHOLE, SP_SUMMIT, SP_SHREWHEAD, SP_COCKPIT, SP_COUNT
};

typedef struct LevelDesc {
    uint8_t scale;   /* SC_SMALL.. */
    uint8_t kind;    /* LV_* */
    uint8_t id;      /* area or special id */
    int16_t row;     /* strips: the parent row stood on */
    int16_t x0, n;   /* strips: parent x of chunk 0, chunk count */
    uint32_t key;    /* strips: the parent level's key (chunk keys hash from it) */
    int16_t pabs;    /* strips: the parent level's absolute x of its tile 0 */
} LevelDesc;

typedef struct Level {
    LevelDesc d;
    int w, h;
    uint8_t *t;      /* tiles */
    uint8_t *bg;     /* back decoration */
    uint32_t key;    /* identity for chunk keys below this level */
    uint8_t biome[STRIP_MAX]; /* strips: material of each chunk */
    uint32_t ckey[STRIP_MAX]; /* strips: key of each chunk */
    int8_t town[STRIP_MAX];   /* strips: hand-made place in this chunk (-1 = generated) */
    int8_t town_part[STRIP_MAX]; /* which chunk of a multi-chunk place */
    uint8_t special[STRIP_MAX]; /* strips: generated cave kind (1 = dangerous) */
    int8_t cave_x0[STRIP_MAX], cave_x1[STRIP_MAX], cave_floor[STRIP_MAX]; /* local cave chamber, -1 none */
    int8_t gap_x[STRIP_MAX], gap_y[STRIP_MAX]; /* a nook behind a one-tile gap: the tile inside, -1 none */
    uint8_t *surf;   /* strips: surface row of each column */
} Level;

static inline int lv_tile(const Level *L, int x, int y) {
    if (x < 0 || x >= L->w) return T_WALL;
    if (y < 0) return T_AIR;
    if (y >= L->h) return T_WALL;
    return L->t[y * L->w + x];
}
static inline void lv_set(Level *L, int x, int y, int t) {
    if (x >= 0 && x < L->w && y >= 0 && y < L->h) L->t[y * L->w + x] = (uint8_t)t;
}

/* ---- entities ------------------------------------------------------ */
enum { EK_NONE, EK_DASH, EK_NPC, EK_FOE, EK_OBJ, EK_PICK, EK_SHOT, EK_DOOR, EK_STAND, EK_DRIP };

/* carryable things */
enum {
    O_PEBBLE, O_CRACKER, O_DART, O_HEARTBOX, O_GIFTBOX, O_BOOMER, O_ROLLER, O_GLUE, O_HOURGLASS,
    O_QUAKE, O_VENOM, O_MOLDFRUIT, O_SPORE, O_PUPA, O_HARD, O_AXE, O_POPGUN,
    /* quest things */
    O_SPECS, O_CATFOOD, O_GEAR, O_JAR, O_JARWATER, O_JARSLIME, O_JARHONEY, O_JARACID, O_LETTER,
    O_ARTIFACT, O_SCROLL, O_BABY, O_EGG, O_TWIG, O_CRATE, O_TABLET, O_BLUEEYE, O_SEED, O_REDEGG,
    O_THREAD, O_BLUEDUST, O_DRINK, O_BIGBANG, O_KEY, O_CINDER,
    O_KINDS
};

/* creatures */
enum {
    F_ANT, F_LANCER, F_AXEANT, F_MOTH, F_BUZZER, F_FLUTTER, F_BUMBLE, F_SPRING, F_NIP, F_MITE,
    F_PAPERFISH, F_GERM, F_GERM2, F_WIGGLER, F_GULP, F_POD, F_TINMOUSE, F_TINMAGE, F_WAXGUARD,
    F_SPARK, F_BEETLE, F_HOPPER,
    F_CLIMBER, F_SPITTER, F_OCTO, F_FACE, F_SCAMPER, F_FERRY, F_TANK, F_PLANE,
    F_ROTIFER, F_EARWIG, F_SPROCKET,
    F_KINDS
};
#define F_FIRST_BOSS F_ROTIFER

/* pickups */
enum { P_GLINT1, P_GLINT5, P_GLINT50, P_HEART, P_UPGRADE, P_BABYDUMMY };

/* upgrades: 23 abilities, then 8 heart buttons and 8 pep eggs = 39 */
enum {
    U_MITT1, U_MITT2, U_TONIC1, U_TONIC2, U_BANGLE, U_SATCHEL1, U_SATCHEL2, U_BEAN1, U_BEAN2,
    U_FEATHER, U_CLOGS1, U_CLOGS2, U_MUSK, U_BUZZ1, U_BUZZ2, U_FIZZ, U_TOP1, U_TOP2, U_WHISTLE,
    U_STEW, U_SHELL, U_PLATE, U_WINGS,
    U_ABILITIES,
    U_HEART0 = U_ABILITIES,           /* 8 heart buttons: 6 whole, 2 halves */
    U_EGG0 = U_HEART0 + 8,            /* 8 pep eggs */
    U_TOTAL = U_EGG0 + 8              /* 39 */
};
extern const char *const DD_UPNAME[U_ABILITIES];
extern const char *const DD_UPDESC[U_ABILITIES];

/* people */
enum {
    N_GRANNY, N_CRUMB, N_VOLT, N_PILGRIM, N_SILK, N_FILAMENT, N_TOCK, N_MOTHERFLUFF, N_TUFTY,
    N_PUFFIN, N_INKY, N_COWPOKE, N_WEEPY, N_SOLDIER, N_SHREW, N_SPINNER,
    N_TAPER, N_TALLOW, N_SNUFF, N_SMUDGE,
    N_FERN, N_SPRIG, N_LICHEN, N_MASON, N_BRACKEN,
    N_CLOD, N_SORREL, N_PEAT, N_SMITH,
    N_WRIGGLA, N_KNOT, N_BORER,
    N_OLDCAP, N_ONEEYE, N_SEEDKEEPER, N_GILL, N_MOREL,
    N_HIGHLUMEN, N_WARDEN,
    N_TABITHA, N_MAGE, N_KNIGHT, N_PELL, N_RATCHET,
    N_NIB, N_NATIVE, N_HOPPERSAGE, N_BUBBLE, N_WAXLING,
    N_BEE, N_FACE, N_EXILE, N_DUMPER, N_EXILE2, N_HISTORIAN, N_PILOT, N_GLOW,
    N_KINDS
};

typedef struct Ent {
    uint8_t kind, sub, alive, dir;
    float x, y, vx, vy;
    int16_t w, h;
    int16_t hp, t, state, st;
    int32_t param, param2;
    uint32_t pid;      /* persistence id (0 = none) */
    uint8_t ground, held, stun, poison;
    uint8_t cmd, flip, frozen, pad;
    int16_t home_x, home_y;
    int16_t hurt_t, fuse;
} Ent;

extern Ent dd_ent[DD_MAX_ENTS];

/* ---- quest flags --------------------------------------------------- */
enum {
    FL_INTRO, FL_MET_GRANNY, FL_SPECS_GIVEN, FL_TONIC_GIVEN, FL_MITT_SOLD, FL_DASH_CLEAN,
    FL_PUFF_CLEAN, FL_PUFF_PAID, FL_BLUEDUST_DONE, FL_INKY_DONE, FL_SNARL_DONE, FL_SCROLL_DONE,
    FL_GERM2_DONE, FL_PELL_FREED, FL_PELL_GUARDED, FL_SILK_DONE, FL_EGG_DONE, FL_TWIG_DONE,
    FL_CRATE_DONE, FL_LETTER1, FL_LETTER2, FL_LETTER_DONE, FL_SMITH_DONE, FL_GEAR_DONE,
    FL_OUTPOST, FL_ROTIFER_ASKED, FL_ROTIFER_DEAD, FL_ROTIFER_PAID, FL_WATER_GIVEN, FL_WATER_PAID,
    FL_TABLET_GIVEN, FL_OLDCAP_DONE, FL_BLUEEYE_DONE, FL_MOREL_DONE, FL_SEED_DONE, FL_REDEGG_DONE,
    FL_COWPOKE_DONE, FL_WEEPY_DONE, FL_SHREW_FREE, FL_SHREW_PAID, FL_MAGE_BEATEN, FL_POWER_OFF,
    FL_EARWIG_DEAD, FL_WINGS_DONE, FL_LUMEN, FL_LAMP_OFF, FL_FILAMENT_ASKED, FL_BIGBANG_DONE,
    FL_THRONE_OPEN, FL_CATFOOD_DONE, FL_MET_QUEEN, FL_PAID_QUEEN, FL_SPROCKET_DEAD, FL_ESCAPED,
    FL_MET_NIB, FL_BALANCE, FL_TRUE_END, FL_SIEGE_DEAD, FL_SHRINE, FL_WORM_FRIENDS, FL_PUFF_MET,
    FL_TUFTY_ASKED, FL_BUBBLES_ASKED, FL_STICKY_ASKED, FL_GLUE_DONE, FL_HOPPER_SEEN, FL_VOLT_MET,
    FL_KNIGHT_MET, FL_FILAMENT_PAID, FL_TOCK_MET, FL_GRATE_OPEN, FL_GRASS_SEEN, FL_SEEN_END,
    FL_BEE_DONE, FL_FACE_DONE, FL_EXILE_DONE, FL_EXILE2_DONE, FL_DUMP_TOLD, FL_HISTORIAN, FL_PILGRIM,
    FL_KEY_TAKEN, FL_SCAMPER, FL_GLOW_MET, FL_COUNT
};

/* ---- the saved game -------------------------------------------------- */
#define DD_COLLECT_MAX 6000
typedef struct DDSave {
    uint32_t magic;
    uint32_t ups;          /* abilities (U_MITT1..U_WINGS) */
    uint8_t hearts_got;    /* heart buttons taken (bits) */
    uint8_t eggs_got;      /* pep eggs taken (bits) */
    uint8_t started, depth;
    int32_t glints;        /* held */
    int32_t glints_total;  /* ever picked up */
    int32_t clock_min;     /* the room clock, minutes past noon */
    uint8_t flags[(FL_COUNT + 7) / 8];
    uint8_t counts[16];    /* quest counters */
    uint8_t satchel[2];    /* stored objects (O_* + 1, 0 = empty) */
    uint8_t carry;         /* O_* + 1 held when saved */
    int16_t carry_param;
    int16_t satchel_param[2];
    uint8_t dash_away;
    int16_t px, py;        /* position in the deepest level */
    int16_t fx, fy;        /* position at full size (tiles of the room) */
    LevelDesc stack[DEPTH_MAX];
    uint16_t n_collect;
    uint32_t collect[DD_COLLECT_MAX];
    uint8_t bigs_found;
    uint8_t full;          /* Dot is at full size */
    uint8_t pad[2];
    uint32_t towns_seen;   /* towns and landmarks Dot has walked into (bits) */
    uint32_t caves_done;   /* big glints taken, two bits a dangerous cave */
} DDSave;

enum { QC_BABIES, QC_BUBBLES, QC_GLUE, QC_TWIGS, QC_NIPS, QC_MITES, QC_PAPERFISH, QC_BABY_PAID, QC_BUBBLE_PAID, QC_GLUE_PAID };

extern DDSave dd_sv;

static inline bool dd_flag(int f) { return (dd_sv.flags[f >> 3] >> (f & 7)) & 1; }
static inline void dd_set(int f) { dd_sv.flags[f >> 3] |= (uint8_t)(1u << (f & 7)); }
static inline bool dd_has(int u) { return (dd_sv.ups >> u) & 1; }

/* ---- the player ------------------------------------------------------ */
typedef struct Body {
    float x, y, vx, vy;
    uint8_t ground, jumping, facing, crouch;
    int16_t w, h;
    int16_t coyote, peak_y, landed_fall;
    uint8_t drop_t, sprint, bounced, pad;
} Body;

typedef struct Phys {
    int ts;            /* tile size in px */
    float walk, sprint, accel, air, grav, maxfall, jump, jump_hi;
} Phys;
extern const Phys DD_PHYS_FULL, DD_PHYS_SMALL;

/* ---- world (dotdash_world.c) ---------------------------------------- */
extern Level dd_lv;            /* the level Dot is in */
extern int dd_scale;
extern LevelDesc dd_stack[DEPTH_MAX];
extern int dd_depth;           /* index into dd_stack of the current level */

void dd_world_init(void);
void dd_build(Level *out, const LevelDesc *d, const Level *parent);
/* run of exposed ground on a row of a level containing x; false if none */
bool dd_strip_run(const Level *L, int row, int x, int *x0, int *n);
uint32_t dd_hash(uint32_t a, uint32_t b);
int dd_level_material(const Level *L, int x, int y);
void dd_spawn_level(void);     /* entities for dd_lv */
const char *dd_place_name(void);
int dd_music_for_level(void);
bool dd_collected(uint32_t pid);
void dd_collect(uint32_t pid);
/* named places for tests and the demo player: fills tile x,y; false if not in this level */
bool dd_find_place(const char *name, int *tx, int *ty);
int dd_chunk_of(int px);       /* strip chunk index under a pixel x */
const char *dd_biome_name(int mat);
int dd_town_at(const Level *L, int tx);
/* the hand-made area maps */
typedef struct AreaMap {
    const char *name;
    int w, h;
    const char *const *rows;
    int anchor_x, anchor_y;    /* where Dot stands in the room when she grows */
    int music;
} AreaMap;
extern const AreaMap DD_AREA[AR_COUNT];
extern const AreaMap DD_SPECIAL[SP_COUNT];

/* ---- play (dotdash_play.c) ------------------------------------------ */
extern Body dd_p;
extern int dd_hp, dd_hp_max;   /* in half hearts */
extern int dd_carry;           /* entity index held, -1 none */
extern int dd_shrink_t, dd_grow_t;
extern int dd_hurt_t, dd_dead_t, dd_frozen_t, dd_flash_t;
extern float dd_cam_x, dd_cam_y;
extern int dd_dash;            /* entity index of Dash, -1 = not here */
extern int dd_trans, dd_trans_dir, dd_trans_kind;
extern int dd_sniff_x, dd_sniff_y;
extern bool dd_no_foes, dd_god;

void dd_play_enter_level(int how);
void dd_play_update(void);
void dd_play_draw(void);
void dd_body_step(Body *b, uint32_t in, uint32_t prev, const Level *L, const Phys *ph, bool bangle, int bean);
bool dd_body_blocked(const Level *L, int ts, float x, float y, float w, float h);
const Phys *dd_phys(void);
int dd_add_ent(int kind, int sub, float x, float y);
int dd_add_obj(int sub, float x, float y);
int dd_add_foe(int sub, float x, float y);
int dd_add_pick(int sub, float x, float y, int param);
void dd_hurt(int halves, float from_x);
void dd_give_glints(int n);
void dd_give_upgrade(int u);
int dd_resolve_upgrade(int u);   /* the next level or piece a spot of kind u gives, -1 full */
void dd_award_upgrade(int u);    /* give it, or glints when full */
int dd_pep(void);
void dd_part(float x, float y, float vx, float vy, int life, int col);
void dd_burst(float x, float y, int col, int n);
void dd_popup(float x, float y, const char *s, int col);
void dd_change_scale(int dir);   /* -1 grow, +1 shrink (on what Dot stands on) */
void dd_goto_area(int area, int tx, int ty);
void dd_kill_foe(int i);
void dd_throw_dash_away(void);
int dd_obj_home_count(int sub);
void dd_foe_size(int sub, int *w, int *h);
bool dd_foe_is_bug(int sub);
bool dd_foe_is_germ(int sub);
extern int dd_lamp_dark;
void dd_after_death_room(void);
void dd_autosave(void);

/* more play and world services */
void dd_push_level(const LevelDesc *d);
void dd_pop_level(void);
void dd_replace_level(const LevelDesc *d);
const Level *dd_parent_level(void);
int dd_door_target(int id, int *area, int *tx, int *ty);
bool dd_door_is(int id, const char *what);
bool dd_obj_exists(int sub, int param);
void dd_restore_carry_after_load(void);
void dd_spawn_dash_now(void);
void dd_save_now(void);
int dd_riding(void);
int dd_lift_t(void);
int dd_kick_t(void);
int dd_throw_t(void);
int dd_freeze_t(void);
void dd_parts_draw(int cx, int cy);
void dd_set_message(const char *s);
const char *dd_message(void);
void dd_reset_play_state(void);
bool dd_save_pending(void);
int dd_hp_max_now(void);
int dd_upgrade_count(void);
int dd_shake(void);
int dd_town_id(const char *name);
void dd_town_where(int k, int *area, int *row, int *x);
int dd_town_count(void);
const char *dd_town_name(int k);
int dd_extra_count(void);
void dd_extra(int k, int *area, int *row, int *x, int *type, int *param);
bool dd_sniff_spot(int area, int row, int x); /* a room tile whose micro strip holds something worth finding */
int dd_town_mark(int k, int *area, int *row, int *x); /* a town Dot has seen: 1 and where, else 0 */
void dd_note_town(void);       /* remember the town Dot stands in */
bool dd_in_danger_cave(void);

/* ---- people (dotdash_npc.c) ----------------------------------------- */
void dd_talk(int ent);              /* the player pressed up at an NPC */
void dd_say(const char *who, const char *text);
void dd_say_more(const char *text);
bool dd_dialog_active(void);
void dd_dialog_update(void);
void dd_dialog_draw(void);
const char *dd_npc_name(int sub);
void dd_buy(int ent);
void dd_shop_items(int npc, int *items3, int *prices3);
int dd_item_upgrade(int item);      /* stand items: >=100 means O_ object, else upgrade */
void dd_native_hint(int ent, char *buf, int n);
void dd_npc_touch(int ent);         /* NPCs that react to what you carry */
void dd_on_boss_dead(int sub);
void dd_on_kill(int sub, int ent);
bool dd_npc_visible(int sub);
extern int dd_answer;               /* yes/no question result (1 yes, 0 no, -1 open) */
void dd_ask(const char *who, const char *text, void (*done)(int yes));

/* ---- game (dotdash.c) ----------------------------------------------- */
enum { ST_TITLE, ST_INTRO, ST_PLAY, ST_ENDING, ST_CREDITS, ST_BAG };
extern int dd_state, dd_state_t;
extern uint32_t dd_in, dd_in_prev; /* buttons this frame (real presses) */
extern uint32_t dd_ticks;         /* update frames since boot */
void dd_hud_track(void);
void dd_start_ending(bool true_end);
void dd_check_goals(void);
extern bool dd_quiet;              /* tests: no autosave writes */

/* ---- demo player (dotdash_bot.c) ------------------------------------- */
void dd_bot_goto(int tx, int ty);
void dd_bot_clear(void);
uint32_t dd_bot_buttons(void);
extern int dd_bot_state;            /* 0 idle, 1 walking, 2 arrived, 3 stuck */
int dd_bot_plan_len(void);
int dd_reach_count(int tx, int ty); /* tiles reachable from a stand (tests) */
bool dd_bot_can_reach(int fx, int fy, int tx, int ty);

/* ---- art and audio ---------------------------------------------------- */
enum {
    S_DOT_STAND, S_DOT_WALK1, S_DOT_WALK2, S_DOT_JUMP, S_DOT_CROUCH, S_DOT_LIFT, S_DOT_HURT, S_DOT_KICK,
    S_DOTBIG_STAND, S_DOTBIG_WALK1, S_DOTBIG_WALK2, S_DOTBIG_JUMP,
    S_DASH1, S_DASH2, S_DASH_SIT, S_DASH_OUCH, S_DASH_WINGS, S_DASHBIG,
    S_GLINT1, S_GLINT5, S_GLINT50, S_HEART, S_HALF, S_GIFT, S_EGG,
    S_OBJ0,                                  /* one per O_ kind */
    S_FOE0 = S_OBJ0 + O_KINDS,               /* two frames per F_ kind */
    S_NPC0 = S_FOE0 + F_KINDS * 2,           /* one per N_ kind */
    S_DOOR = S_NPC0 + N_KINDS, S_STAND, S_PEA, S_SPELL, S_BOLT, S_DROP, S_OWL, S_TRAIN, S_THIMBLE,
    S_SPROCKET_HEAD, S_ICON_UP, S_DOTTINY_STAND, S_DOTTINY_WALK1, S_DOTTINY_WALK2,
    S_COUNT
};
extern Sprite dd_spr[S_COUNT];
void dd_art_load(void);
void dd_audio_load(void);
enum { MU_TITLE, MU_ROOM, MU_SMALL, MU_MICRO, MU_DEEP, MU_TOWN, MU_WALLS, MU_BOSS, MU_LATCH, MU_ENDING, MU_TRUE, MU_SIEGE,
       MU_MICRO2, MU_MICRO3, MU_CAVE, MU_COUNT };
extern int DD_MUS[MU_COUNT];
extern int DD_JINGLE_UP, DD_JINGLE_DEAD;

#endif
