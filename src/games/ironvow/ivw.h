/* IRONVOW - shared declarations. Cartridge 37 of UFO 40.
 * A tribute to Valbrace (UFO 50 #37); see docs/games/37-ironvow.md.
 *
 * A knight sworn to end the Pit Regent is caught, stripped to his rags and
 * thrown into the cellars under Blackspire Keep. Seven floors down, in
 * three parts (the Gaol, the Warren, the Starvault), walked one tile and
 * one turn at a time in first person; touch a wandering flame and the
 * fight is real time, side to side: guard, roll, strike, read the tell.
 * Spells are drawn with the pad, and the glyphs are written on the walls.
 *
 *   ivw_data.c    every number: items, foes, spells and their glyphs, pools
 *   ivw_floors.c  the seven hand-made floors and what stands on them
 *   ivw_world.c   exploring: steps, turns, flames, traps, the context menu
 *   ivw_fight.c   the fights: the knight, the foes, shots, the bosses
 *   ivw_magic.c   drawing glyphs and what each spell does
 *   ivw.c         the screens (title, menus, throne, shop...), saving,
 *                 goals, records and the test hooks
 *   ivw_bot.c     the demo player (plays with button presses)
 *   ivw_view.c    the first-person view   ivw_draw.c everything else drawn
 *   ivw_art.c     sprites                 ivw_audio.c music and sounds
 *
 * Gameplay and the demo player use integers only, so a run plays the same
 * on every platform. Only the drawing uses floats. */
#ifndef IRONVOW_IVW_H
#define IRONVOW_IVW_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- directions: 0 north (up the map), 1 east, 2 south, 3 west ---------- */
extern const int IVW_DX[4], IVW_DY[4];

/* ---- the floors ---------------------------------------------------------------- */
#define IVW_FLOORS 7
#define IVW_W 32
#define IVW_H 32
enum { BIO_GAOL, BIO_WARREN, BIO_STAR, BIO_COUNT };
extern const char *const IVW_BIOME_NAME[BIO_COUNT];

/* tiles */
enum {
    T_FLOOR, T_WALL, T_DOOR, T_DOOR_OPEN, T_LOCKED, T_GATE, T_GATE_OPEN, T_HIDDEN, T_SPIKES, T_PIT,
    T_SLIME_A, T_SLIME_B, T_LAVA, T_STAIRS, T_SHIFT_A, T_SHIFT_B, T_EXIT,
    /* walls with something on their faces */
    T_TORCH, T_FUNGUS, T_LANTERN, T_MURAL, T_LEVER, T_GEMS, T_SWITCH,
    T_COUNT
};
#define IVW_IS_DECOR(t) ((t) >= T_TORCH)

/* lever jobs (ivw_floors.c lists them, in map order, for each floor) */
enum { LV_GATE, LV_BOULDER, LV_TRAP, LV_SHIFT, LV_SLIME };
typedef struct IvwLever { uint8_t job; int8_t tx, ty; } IvwLever;

/* things standing on floor tiles */
enum {
    OB_NONE, OB_CHEST, OB_MIMIC, OB_SWEET_WELL, OB_FOUL_WELL, OB_THRONE, OB_PEDLAR, OB_KNIGHT, OB_PILLAR,
    OB_SOCKET, OB_BOSS, OB_BOULDER, OB_COUNT
};

/* a chest's content: an item, RANDOM (a roll from the floor's tier), or shards */
#define IVW_RANDOM 250
#define IVW_SHARDS 251 /* + the amount in the second byte */
typedef struct IvwChest { uint8_t item, amount; } IvwChest;

typedef struct IvwFloorDef {
    const char *name;
    uint8_t biome;
    uint8_t w, h;
    const char *const *rows;
    const IvwLever *levers; int nlevers;
    const IvwChest *chests; int nchests;  /* chests and mimics, map order */
    const uint8_t *murals; int nmurals;   /* spell of each mural, map order */
    const uint8_t *gems; int ngems;       /* each gem panel's layout set (ivw_data.c) */
    uint8_t flame_cap;                    /* map flames at once */
    uint8_t boss;                         /* BOSS_* on the boss tile */
    uint8_t search_kind;                  /* what the searched room yields last */
    int16_t throne_cost;
} IvwFloorDef;
extern const IvwFloorDef IVW_FLOOR[IVW_FLOORS];

/* ---- items -------------------------------------------------------------------------- */
enum { IK_WEAPON, IK_ARMOUR, IK_SHIELD, IK_POTION, IK_PART, IK_SPECIAL, IK_KEY };
enum {
    IT_NONE,
    /* weapons */
    IT_DIRK, IT_FOIL, IT_CUDGEL, IT_TUSKMAUL, IT_ARMING, IT_STAR, IT_GLAIVE, IT_BROAD, IT_MAUL, IT_AXE,
    IT_OATH, IT_DAWN,
    /* armour */
    IT_RAGS, IT_TUNIC, IT_COWL, IT_JERKIN, IT_PADDED, IT_BRONZE, IT_IRON, IT_STEEL, IT_MOULD,
    /* shields */
    IT_BUCKLER, IT_OAK, IT_BOAR, IT_WYRM, IT_WRAITH, IT_SKINKB, IT_COILB,
    /* tonics */
    IT_RED, IT_BLACK, IT_BLUE, IT_GREEN, IT_YELLOW, IT_GREY,
    /* monster parts (readied: they go off at the start of the next fight) */
    IT_OOZESKULL, IT_HEART, IT_TUSK, IT_BONESKULL, IT_BONE, IT_HEAD, IT_SAC, IT_BALL,
    /* special */
    IT_COIN, IT_EGG, IT_GRUB, IT_BEETLE,
    /* the three stones */
    IT_CINDER, IT_MOSS, IT_FROST,
    IT_IRONKEY,                      /* never in the pack: picked up, it is a key */
    IT_COUNT
};
typedef struct IvwItem {
    const char *name;
    uint8_t kind;
    uint8_t value;   /* the pedlar pays this many shards (buying costs twice) */
    uint8_t req;     /* weapons: MIGHT needed */
    int16_t a, b, c; /* weapon: damage, speed class, flags; armour: guard %, slowdown %; shield: blocks, stamina % */
    const char *desc;
} IvwItem;
extern const IvwItem IVW_ITEM[IT_COUNT];
/* weapon flags */
enum { WF_STUN = 1, WF_REACH = 2, WF_SWEEP = 4, WF_CHARGE = 8, WF_BOLT = 16 };
/* weapon speed classes: windup, active, recover frames and the stamina a swing costs */
typedef struct IvwSwing { uint8_t windup, active, recover, cost; } IvwSwing;
extern const IvwSwing IVW_SWING[6];

/* ---- foes ------------------------------------------------------------------------------ */
enum {
    FK_OOZE, FK_GNAW, FK_LIDMAW, FK_TUSKER, FK_PEDLAR, FK_BONE, FK_SKINK, FK_EGG, FK_GRUB, FK_CHITIN,
    FK_PUFF, FK_COILER, FK_PUPPET, FK_MUMMER, FK_GRASPER, FK_SILVER, FK_GUARD,
    FK_WARDEN, FK_MATRON, FK_CLAW, FK_REGENT, FK_STONE,
    FK_BEETLE, /* the player's own beetle gone wild */
    FK_COUNT
};
enum { FF_GUARD = 1, FF_FLYER = 2, FF_NOPOISON = 4, FF_NOBURN = 8, FF_NOFREEZE = 16, FF_CASTER = 32, FF_BOSS = 64 };
typedef struct IvwFoeDef {
    const char *name;
    int16_t hp;
    uint8_t dmg, stdmg;      /* damage, and stamina it takes when guarded */
    uint8_t windup, active, recover;
    uint8_t speed;           /* sixteenths of a pixel a frame */
    uint8_t range;           /* half width of its blow, pixels */
    uint8_t essence;
    uint8_t flags;
    uint8_t drop, drop2;     /* parts it may leave */
} IvwFoeDef;
extern const IvwFoeDef IVW_FOE[FK_COUNT];
enum { BOSS_NONE, BOSS_WARDEN, BOSS_MATRON, BOSS_REGENT };

/* ---- spells ------------------------------------------------------------------------------ */
enum {
    SP_CHART, SP_OPEN, SP_MEND, SP_LANCE, SP_KINDLE, SP_GALE, SP_VEIL, SP_MIRE, SP_RESTORE, SP_RIME,
    SP_FLOAT, SP_FLARE, SP_QUICKEN, SP_THUNDER, SP_HUSH, SP_TWIN, SP_WARD, SP_COUNT
};
typedef struct IvwSpell {
    const char *name;
    const char *glyph; /* strokes: U R D L */
    uint8_t mana;
    const char *desc;
} IvwSpell;
extern const IvwSpell IVW_SPELL[SP_COUNT];
#define IVW_GLYPH_MAX 10
/* the spell a traced glyph makes (or its mirror image), -1 = none */
int ivw_glyph_spell(const char *strokes);

/* gem panels on the Gaol's walls (3 x 3; 0 red, 1 green, 2 blue) */
extern const uint8_t IVW_GEMSET[4][9];

/* ---- the hero ----------------------------------------------------------------------------- */
#define IVW_INV_MAX 12
enum { STAT_MIGHT, STAT_GRIT, STAT_LORE };
enum { SEC_OATH = 1, SEC_COIN = 2, SEC_WISH = 4, SEC_DAWN = 8 };
typedef struct IvwHero {
    uint8_t floor;
    int8_t x, y, dir;
    int16_t hp, mp, st;              /* st: stamina in sixteenths */
    int16_t hp_bonus;                /* the Amber Knight's gifts, up to +20 */
    uint8_t stat[3];
    uint8_t inv[IVW_INV_MAX];
    uint8_t ready[IVW_INV_MAX];      /* a part readied for the next fight */
    uint8_t inv_cap;
    int8_t wpn, arm, shd;            /* inventory slots of the gear worn, -1 none */
    int32_t essence;
    int16_t shards;
    uint8_t keys;
    uint8_t poison, burn;            /* turns left */
    /* the Amber Knight */
    uint8_t knight_gone, knight_talk, knight_favour, knight_hp_gifts, knight_inv_gifts;
    uint8_t coin_given, honor, dawn_given;
    uint8_t secrets;                 /* SEC_* found this run */
    /* the torches of the Gaol, counted as each floor is left */
    uint8_t torch_lit[3], torch_all[3];
    /* the grub egg, the grub, the beetle */
    uint8_t egg_fights, grub_fights, beetle_fights;
    int16_t beetle_hp;
    uint8_t thrones_used;            /* bit per floor */
    uint8_t bosses;                  /* bit per BOSS_* beaten */
    uint8_t pad[2];
} IvwHero;

/* ---- the world of the floor being walked ------------------------------------------------------ */
#define IVW_MAX_OBJ 48
#define IVW_MAX_DROP 64
#define IVW_MAX_FLAME 8
typedef struct IvwObj { uint8_t kind, x, y, state, param, uses; } IvwObj;
typedef struct IvwDrop { uint8_t item, x, y; } IvwDrop;
typedef struct IvwFlame { uint8_t alive, x, y, burned, wait; } IvwFlame;
typedef struct IvwWorld {
    uint8_t tile[IVW_H][IVW_W];
    uint8_t st[IVW_H][IVW_W];        /* lit, pulled, solved... */
    uint8_t id[IVW_H][IVW_W];        /* lever / mural / gem panel number */
    uint8_t seen[IVW_H][IVW_W];
    IvwObj obj[IVW_MAX_OBJ]; int nobj;
    IvwDrop drop[IVW_MAX_DROP]; int ndrop;
    IvwFlame flame[IVW_MAX_FLAME];
    uint8_t spawn_x[16], spawn_y[16]; int nspawn;
    int32_t turn;
    uint8_t slime_b;                 /* 0: group A wet, 1: group B wet */
    uint8_t shift;                   /* 0 or 1 */
    uint8_t battles;                 /* battles fought on this floor (essence taper) */
    uint8_t veil, fly;               /* turns left */
    uint8_t searches;                /* the searched room: searches made */
    uint8_t boss_dead, stones_set;
    uint8_t pedlar_gone;
    uint8_t escape;                  /* the way up is open */
    uint8_t shop[4];                 /* the pedlar's stock (0 = sold) */
} IvwWorld;

/* ---- the fight ---------------------------------------------------------------------------------- */
#define IVW_ARENA_W 176
#define IVW_MAX_FOES 10
#define IVW_MAX_SHOTS 16
enum { FS_MOVE, FS_WINDUP, FS_ACTIVE, FS_RECOVER, FS_STUN, FS_CAST, FS_HATCH, FS_DEAD };
typedef struct IvwFoe {
    uint8_t kind, alive, row, state;
    int16_t x;                       /* sixteenths of a pixel */
    int16_t hp, hpmax;
    int16_t t, cd;
    int16_t stun, slow, freeze, burn, poison;
    uint8_t alt, move, strikes, blocks;
    int16_t aux;                     /* a timer of its own (hatching, row swap, a stone's height) */
    int16_t tx;                      /* where it heads */
} IvwFoe;
enum { SH_PELLET, SH_ACID, SH_BALL, SH_NEEDLE, SH_FIRE, SH_BOLT, SH_ROCK, SH_SPIKE, SH_SHIELD, SH_LIGHTNING,
       SH_CLOUD, SH_MIST, SH_BEAM, SH_MYBOLT, SH_TOSS };
typedef struct IvwShot {
    uint8_t kind, alive, from;
    int16_t x, x0, t, t_max;         /* x in sixteenths; it lands when t reaches t_max */
    uint8_t dmg, stdmg;
} IvwShot;
enum { PS_IDLE, PS_GUARD, PS_SWING, PS_DODGE, PS_STAGGER, PS_CHARGE };
typedef struct IvwFight {
    uint8_t active, boss, result;    /* result: 0 on, 1 won, 2 lost, 3 swept away */
    int16_t px;                      /* the knight, sixteenths of a pixel */
    uint8_t pstate, combo, queued, exhausted, beetle, beetle_out;
    int16_t pt;                      /* time in this state */
    int16_t down_t;                  /* frames since DOWN went down (rolls) */
    int8_t roll_dir;
    int16_t blocks_left;
    int16_t invis, haste, ward, twin, reflex, sturdy, might_up, charge, flash_hurt;
    int16_t recover_hp;              /* the mould cloak's grey health */
    int16_t silence;
    IvwFoe foe[IVW_MAX_FOES]; int nfoe;
    IvwShot shot[IVW_MAX_SHOTS];
    int32_t t;
    int16_t essence_won;
    uint8_t loot[4]; int nloot;
    uint8_t flame;                   /* the map flame that started it (255 none) */
    uint8_t search;                  /* started by a search */
    uint8_t pedlar, mimic_obj;
    int16_t ball_x, ball_v, ball_t;  /* a bouncing skull, ball or grub */
    uint8_t ball_kind;
    int16_t regent_phase;
    int16_t start_burn;
    uint8_t swaps;                   /* rows traded (tests) */
} IvwFight;

/* ---- casting --------------------------------------------------------------------------------------- */
typedef struct IvwCast {
    uint8_t on;
    char strokes[IVW_GLYPH_MAX + 1];
    uint8_t n;
    int16_t t, t_max;
    int16_t result_t;                /* showing the result */
    int8_t result;                   /* spell, -1 fizzle, -2 no mana */
} IvwCast;

/* ---- the screens ----------------------------------------------------------------------------------- */
enum {
    VS_TITLE, VS_STORY, VS_CARD, VS_EXPLORE, VS_MENU, VS_INV, VS_ITEM, VS_PICK, VS_SHOP, VS_THRONE, VS_GEMS,
    VS_WISH, VS_GLYPH, VS_MAP, VS_TALK, VS_FIGHT, VS_LOOT, VS_OVER, VS_ENDING, VS_CONFIRM, VS_GIVE, VS_TELEPORT
};

/* context menu actions */
enum {
    MA_INV, MA_PASS, MA_OPEN, MA_CLOSE, MA_UNLOCK, MA_LEVER, MA_DOUSE, MA_POKE, MA_SMOTHER, MA_LANTERN,
    MA_GEMS, MA_SWITCH, MA_STUDY, MA_CHEST, MA_DRINK, MA_WELL_ITEM, MA_SIT, MA_TRADE, MA_STEAL, MA_TALK,
    MA_GIVE, MA_ATTACK, MA_SOCKET, MA_PICKUP, MA_SEARCH, MA_COUNT
};
extern const char *const IVW_MA_NAME[MA_COUNT];

#define IVW_MSG_LINES 3
#define IVW_LOG_W 184                    /* the log's width in pixels */
typedef struct IvwGame {
    int state, state_t, ret_state;
    int frame_t;
    IvwHero hero;
    IvwHero floor_start;             /* the hero as the floor began (the save) */
    uint64_t floor_rng;
    IvwWorld w;
    IvwFight f;
    IvwCast cast;
    Rng rng;
    /* exploring: the step and turn being drawn */
    int anim_t, anim_kind;           /* 0 none, 1 step, 2 turn */
    int from_x, from_y, from_dir;
    int clock_t;
    int pass_hold;
    /* menus */
    uint8_t menu[16]; int nmenu, menu_sel;
    int inv_sel, item_sel, pick_sel, shop_sel, shop_buy, throne_pts, throne_sel, gem_sel, gem_held, wish_sel;
    uint8_t gem_board[9]; int gem_panel_x, gem_panel_y;
    uint8_t item_menu[6]; int nitem_menu;
    int pick_mode;                   /* what the item picker is for */
    int confirm_kind, confirm_sel;
    int title_sel;
    const char *talk[4]; int ntalk, talk_page;
    int glyph_view;
    int tele_x, tele_y;
    int stat_before[3];
    /* messages */
    char msg[IVW_MSG_LINES][64];
    int msg_t;
    /* the run */
    bool won, cherry_run, demo;
    int over_t;
    int ending_kind;                 /* 0 the plain ending, 1 the dawn */
    int dawn_kill;                   /* the Regent fell to the Dawnbrand */
    int easter;                      /* the wall behind the start */
    int buff;                        /* tonics for the next fight: 1 black, 2 green, 4 grey, 8 yellow */
} IvwGame;

typedef struct IvwSave {
    uint32_t magic;
    uint8_t has_run;                 /* a floor to continue from */
    uint8_t pad0[3];
    IvwHero hero;                    /* the hero as the floor began */
    uint64_t rng_state, rng_inc;
    /* the records (the title shows them) */
    uint32_t battles, deaths;
    uint8_t secrets_seen;            /* every SEC_* ever found */
    uint8_t best_floor;
    uint8_t wins, cherries;
    uint8_t gifts;
    uint8_t pad1[3];
} IvwSave;

extern IvwGame ig;
extern IvwSave igs;
extern bool ivw_simulating;          /* the demo player's look-ahead: no sound */

/* ---- ivw.c ------------------------------------------------------------------------------------------ */
void ivw_sfx(const char *name);
void ivw_say(const char *fmt, ...);
void ivw_set_state(int s);
void ivw_save_now(void);
void ivw_floor_arrive(int floor);    /* load a floor, save, show its card */
void ivw_game_over(void);
void ivw_win_escape(void);
int ivw_max_hp(const IvwHero *h);
int ivw_max_mp(const IvwHero *h);
int ivw_max_st(const IvwHero *h);    /* sixteenths */
int ivw_stat_total(const IvwHero *h);
void ivw_check_gift(void);
bool ivw_give_item(int item);        /* into the pack; false if it is full */
int ivw_count_item(int item);
int ivw_find_item(int item);
void ivw_remove_slot(int slot);
int ivw_free_slots(void);
void ivw_add_secret(int bit);

/* ---- ivw_floors.c -------------------------------------------------------------------------------------- */
void ivw_floor_load(int floor);
void ivw_floor_start(int floor, int *x, int *y, int *dir);
int ivw_floor_pool(int floor, Rng *r, uint8_t *kinds, int max);
int ivw_random_item(int floor, Rng *r);

/* ---- ivw_world.c ------------------------------------------------------------------------------------------ */
bool ivw_walkable(int x, int y);       /* the knight may stand there */
bool ivw_flame_walkable(int x, int y);
bool ivw_opaque(int x, int y);         /* blocks the view */
int ivw_tile(int x, int y);
IvwObj *ivw_obj_at(int x, int y);
void ivw_explore_update(void);
void ivw_pass_turn(void);
void ivw_build_menu(void);
void ivw_menu_do(int action);
void ivw_reveal_around(void);
void ivw_touch_tile(void);             /* what the tile the knight now stands on does */
int ivw_flame_at(int x, int y);
void ivw_start_fight_flame(int flame);
void ivw_fight_done(void);             /* back from a fight */
int ivw_drops_here(int x, int y, uint8_t *items, int max);
void ivw_drop_item(int item, int x, int y);
bool ivw_take_drop(int item, int x, int y);
int ivw_torches(int *lit);             /* the Gaol torches of this floor */
void ivw_world_message_lever(int job);
bool ivw_spikes_up(void);
void ivw_use_item_world(int slot);
void ivw_wish(int choice);
void ivw_knight_talk(void);
void ivw_knight_give(int slot);
void ivw_search_room(void);
void ivw_set_stones(void);
void ivw_pick_do(int sel);
void ivw_teleport_to(int x, int y);
void ivw_mimic_loot(int obj);
int ivw_wish_foul(void);

/* ---- ivw_fight.c ------------------------------------------------------------------------------------------ */
void ivw_fight_begin(const uint8_t *kinds, int n, int boss);
void ivw_fight_update(void);
void ivw_fight_spell(int spell);
int ivw_fight_foes_alive(void);
bool ivw_foe_aligned(const IvwFoe *e, int extra);
int ivw_weapon_item(void);
int ivw_shield_item(void);
int ivw_armour_item(void);
int ivw_player_speed(void);           /* sixteenths of a pixel a frame */

/* ---- ivw_magic.c ------------------------------------------------------------------------------------------ */
void ivw_cast_begin(void);
void ivw_cast_update(void);           /* true while drawing */
int ivw_cast_window(void);
void ivw_spell_world(int spell);

/* ---- ivw_bot.c ----------------------------------------------------------------------------------------------- */
int ivw_bot_buttons(void);
extern int ivw_bot_debug;
void ivw_bot_reset(void);

/* ---- ivw_view.c / ivw_draw.c / ivw_art.c / ivw_audio.c -------------------------------------------------------- */
void ivw_view_draw(int vx, int vy, int vw, int vh);
void ivw_draw(void);
void ivw_draw_label(int x, int y, int w, int h, int t);
void ivw_draw_glyph(const char *strokes, int n, int cx, int cy, int cell, int col, int dot_col);
int ivw_layout_audit(void);
void ivw_art_load(void);
enum {
    SPR_KNIGHT, SPR_KNIGHT_GUARD, SPR_KNIGHT_SWING, SPR_KNIGHT_ROLL, SPR_BEETLE_BACK,
    SPR_FLAME, SPR_CHEST, SPR_CHEST_OPEN, SPR_WELL, SPR_THRONE, SPR_PEDLAR, SPR_KNIGHT_NPC, SPR_PILLAR, SPR_SOCKET,
    SPR_BOULDER, SPR_STAIRS, SPR_ITEM_BAG,
    SPR_COUNT
};
extern Sprite ivw_spr[SPR_COUNT];
void ivw_draw_foe(const IvwFoe *e, int x, int y, int scale, int t);
void ivw_audio_load(void);
extern int IVW_MUS_TITLE, IVW_MUS_GAOL, IVW_MUS_WARREN, IVW_MUS_STAR, IVW_MUS_THRONE, IVW_MUS_BOSS, IVW_MUS_ENDING,
    IVW_MUS_OVER, IVW_MUS_REGENT;
void ivw_floor_music(void);

#endif
