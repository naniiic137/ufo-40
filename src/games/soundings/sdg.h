/* SOUNDINGS - shared declarations. Cartridge 27 of UFO 40.
 * A tribute to Divers (UFO 50 #27); see docs/games/27-soundings.md.
 *
 * Moss, Reed and Skip, three axolotl siblings, dive from a raft into the
 * dark under the sea: one hand-built cave of eight regions, a raft that is
 * the only light, the only shop, the only place to change gear and the only
 * place the log is written. Touching a creature starts a turn-based fight.
 *
 *   sdg_data.c    every number: items, the shop, creatures, groups, levels,
 *                 chests, notes
 *   sdg_map.c     the hand-built map and what stands on it
 *   sdg_battle.c  the fights: orders, the round, damage, the creatures' ways
 *   sdg_dive.c    swimming: the dark, creatures, shots, chests, levers,
 *                 bombs, the mist orb, the item menu
 *   sdg.c         the screens (title, raft, shop, kit, ending), saving,
 *                 goals and the test hooks
 *   sdg_bot.c     the demo player (plays with button presses)
 *   sdg_draw.c    drawing   sdg_art.c sprites   sdg_audio.c music and sounds
 *
 * Gameplay and the demo player use integers only, so a run plays the same
 * on every platform. */
#ifndef SDG_H
#define SDG_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- elements: REEF beats ZAP, ZAP beats OOZE, OOZE beats REEF ---------- */
enum { EL_NONE, EL_REEF, EL_ZAP, EL_OOZE, EL_COUNT };
extern const char *const SDG_EL_NAME[EL_COUNT];
extern const uint8_t SDG_EL_COL[EL_COUNT];

/* ---- relics ------------------------------------------------------------ */
enum { RL_FOAM, RL_GUM, RL_PEBBLE, RL_COG, RL_SPIRAL, RL_BONE, RL_SUN, RL_MOON, RL_COUNT };
#define SDG_RELIC_MAX 9
extern const char *const SDG_RELIC_NAME[RL_COUNT];

/* ---- items --------------------------------------------------------------- */
enum { K_POLE, K_HAMMER, K_SHIELD, K_POTION, K_EGG, K_BOMB, K_MIST, K_PASSIVE };
enum {
    IT_NONE,
    IT_POKER_R, IT_POKER_Z, IT_POKER_O,
    IT_GAFF_R, IT_GAFF_Z, IT_GAFF_O,
    IT_HARPOON_R, IT_HARPOON_Z, IT_HARPOON_O,
    IT_CLUB_R, IT_CLUB_Z, IT_CLUB_O,
    IT_MAUL_R, IT_MAUL_Z, IT_MAUL_O,
    IT_PLATE_R, IT_PLATE_Z, IT_PLATE_O,
    IT_TARGE_R, IT_TARGE_Z, IT_TARGE_O,
    IT_BULWARK_R, IT_BULWARK_Z, IT_BULWARK_O,
    IT_LEECH, IT_SPINE, IT_LAMP,
    IT_SMALL, IT_MEDIUM, IT_LARGE, IT_HOLY, IT_EVIL, IT_EGG,
    IT_BOMB, IT_MIST, IT_GODBLOOD, IT_FLIPPERS,
    IT_COUNT
};
enum { IF_LEECH = 1, IF_THORNS = 2, IF_HOLY = 4, IF_UNIQUE = 8 };
typedef struct SdgItem {
    const char *name;
    uint8_t kind, el;
    int16_t power;             /* weapons: attack power */
    uint8_t uses;              /* per dive; 0 = passive */
    uint8_t def;               /* shields: % taken off every hit while held */
    uint8_t guard;             /* hammers: defend like a shield of this cut (poles 0) */
    int16_t heal;              /* potions */
    int16_t price;             /* gold; 0 = never sold */
    int8_t rel[2];             /* relic costs: kind (-1 none) ... */
    uint8_t reln[2];           /* ... and how many */
    uint8_t max;               /* most you can own */
    uint8_t flags;
    int8_t shop;               /* shop page, -1 found only */
} SdgItem;
extern const SdgItem SDG_ITEM[IT_COUNT];
enum { SHOP_POLE, SHOP_HAMMER, SHOP_SHIELD, SHOP_POTION, SHOP_KEY, SHOP_PAGES };
extern const char *const SDG_SHOP_PAGE[SHOP_PAGES];
/* the items on a shop page, in order; returns how many */
int sdg_shop_list(int page, uint8_t *out, int max);
bool sdg_is_weapon(int it);
bool sdg_battle_usable(int it);   /* can be picked in a fight */

/* ---- creatures (in a fight) ---------------------------------------------- */
enum {
    EN_FIZZLE, EN_PRICKLE, EN_FROND, EN_NIPPER, EN_GLOB, EN_SMOG, EN_CLAMPER, EN_TINFIN, EN_WHORL,
    EN_JELLY, EN_GROPER, EN_BURRNUT, EN_STILTER, EN_LOUSE, EN_HAUNT, EN_SQUID, EN_GRINFISH, EN_WORM,
    EN_WARDEN, EN_EYE, EN_ARM,
    EN_COUNT
};
#define EN_REGULAR 18
enum {
    FF_THORNS = 1 << 0, FF_HEAL = 1 << 1, FF_REVIVE = 1 << 2, FF_DOUBLE = 1 << 3, FF_MAYDOUBLE = 1 << 4,
    FF_LEECH = 1 << 5, FF_TELEGRAPH = 1 << 6, FF_GUARD = 1 << 7, FF_HIDE = 1 << 8, FF_IDLE = 1 << 9,
    FF_LOWEST = 1 << 10, FF_STARTHURT = 1 << 11, FF_BOSS = 1 << 12, FF_SHIFT = 1 << 13
};
typedef struct SdgFoeDef {
    const char *name;
    int16_t hp;
    int16_t xp, gold;
    uint8_t weak;              /* EL_*; the jelly shifts (FF_SHIFT) */
    int16_t lo, hi;            /* one hit */
    int8_t drop;               /* RL_* or -1 */
    uint16_t flags;
    int16_t heal;              /* what a heal gives */
    uint8_t spr;               /* SP_* */
} SdgFoeDef;
extern const SdgFoeDef SDG_FOE[EN_COUNT];

/* ---- levels ---------------------------------------------------------------- */
#define SDG_MAX_LEVEL 16
extern const int32_t SDG_XP_AT[SDG_MAX_LEVEL + 1]; /* total XP to reach level n (index n) */
extern const int16_t SDG_HP_AT[SDG_MAX_LEVEL + 1];
/* damage scale in % for a level */
int sdg_level_mul(int level);

/* ---- the map ------------------------------------------------------------- */
#define SDG_T 16
#define SDG_MW 96
#define SDG_MH 80
#define SDG_SURF_ROW 4          /* the first row of water */
#define SDG_SURF_Y (SDG_SURF_ROW * SDG_T)
#define SDG_SURF_C0 40          /* the open water under the raft */
#define SDG_SURF_C1 56
extern const char *const SDG_MAP[SDG_MH];
enum { RG_SHELF, RG_GUMWELL, RG_HOLLOW, RG_SHRINE, RG_LANTERN, RG_STILT, RG_STILL, RG_DEEP, RG_COUNT };
extern const char *const SDG_REGION_NAME[RG_COUNT];
int sdg_region_at(int c, int r);

/* what the map holds, found by scanning it once (sdg_map_index) */
#define SDG_CHESTS 17
#define SDG_HEADS 3
#define SDG_LORES 7
#define SDG_WALLS 3
#define SDG_MOBS 72
enum { DOOR_WEST, DOOR_SHORT, DOOR_WARDEN, DOOR_FINAL, DOOR_COUNT };
enum { LV_WEST, LV_SHORT, LV_FINAL_W, LV_FINAL_E, LV_COUNT };
typedef struct SdgPlace { int16_t c, r; } SdgPlace;
typedef struct SdgMapIndex {
    SdgPlace chest[SDG_CHESTS];
    uint8_t chest_hidden[SDG_CHESTS];
    SdgPlace head[SDG_HEADS];
    SdgPlace lore[SDG_LORES];
    SdgPlace lever[LV_COUNT];
    SdgPlace warden, gloam;
    int nchest, nhead, nlore, nwall;
    int8_t wall_of[SDG_MH][SDG_MW]; /* bomb wall group per cell, -1 */
    struct { uint8_t letter; int16_t c, r; } spawn[SDG_MOBS];
    int nspawn;
} SdgMapIndex;
extern SdgMapIndex sdg_mi;
void sdg_map_index(void);

/* chest contents */
enum { CH_GOLD, CH_RELIC, CH_PEARL };
typedef struct SdgChest { uint8_t kind; int16_t n; } SdgChest; /* gold amount, relic kind, item */
extern const SdgChest SDG_CHEST[SDG_CHESTS];
extern const char *const SDG_LORE_TEXT[SDG_LORES][2];
extern const char *const SDG_HEAD_NAME[SDG_HEADS];

/* ---- progress: the whole state of the game (live), and the copy written
 * at the last surfacing that a wipe goes back to ---------------------------- */
#define SDG_PEARLS 12
typedef struct SdgProg {
    uint32_t magic;
    uint16_t version;
    uint8_t level;
    uint8_t started;
    int32_t xp;
    int32_t gold;
    uint8_t relic[RL_COUNT];       /* banked */
    uint8_t carry[RL_COUNT];       /* found on this dive */
    uint8_t owned[IT_COUNT];
    uint8_t equip[3][2];
    uint8_t uses[3][2];
    int16_t hp[3];
    uint8_t pearl[SDG_PEARLS];     /* items found as pearls on this dive */
    uint8_t npearl;
    uint32_t chests;               /* bits */
    uint8_t doors;                 /* DOOR_* bits */
    uint8_t levers;                /* LV_* bits */
    uint8_t walls;                 /* bits */
    uint8_t heads;                 /* bits */
    int16_t deepest;
    uint16_t doors_open, chests_open; /* the title's stats */
    uint16_t wins, cherry_wins, dives;
    uint8_t gift;
    uint8_t pad[3];
} SdgProg;
#define SDG_MAGIC 0x53444701u
#define SDG_VERSION 1

/* ---- the dive -------------------------------------------------------------- */
enum {
    MK_FIZZLE, MK_PRICKLE, MK_FROND, MK_NEST, MK_SCHOOL, MK_GLOB, MK_SMOGVENT, MK_CLAMPVENT, MK_TINFIN,
    MK_WHORL, MK_JELLY, MK_GROPER, MK_BURRNUT, MK_STILTER, MK_LOUSE, MK_HAUNT, MK_SQUID, MK_GRINFISH,
    MK_WORM, MK_WARDEN, MK_GLOAM, MK_COUNT
};
enum { MS_HOME, MS_ACTIVE, MS_GONE };
typedef struct SdgMob {
    uint8_t kind, state, spawn;    /* spawn: its index in sdg_mi.spawn */
    uint8_t region;
    int32_t x, y;                  /* 1/256 px, the middle */
    int32_t hx, hy;                /* home, px */
    int32_t vx, vy;
    int16_t t, t2;
    int8_t dir;
    uint8_t out;                   /* a vent's creature is out, a worm reaching */
    uint32_t gone_at;
} SdgMob;
typedef struct SdgShot { int32_t x, y, vx, vy; uint8_t on; uint16_t life; } SdgShot;
#define SDG_SHOTS 24

typedef struct SdgDive {
    int32_t x, y;                  /* the leader, 1/256 px (the middle) */
    int8_t face;
    uint8_t left_surface;
    uint16_t inv;                  /* frames before a creature can start a fight again */
    int32_t mist;                  /* metres of mist left (0 = off), 1/256 */
    SdgMob mob[SDG_MOBS];
    int nmob;
    SdgShot shot[SDG_SHOTS];
    uint32_t t;
    int16_t haunt_t;
    int8_t near_chest, near_lever, near_lore, near_head;
    int8_t hit_flash;              /* a shot struck the party */
    uint8_t moving;
} SdgDive;
#define SDG_SPEED 192              /* 0.75 px a frame */
#define SDG_HW 5                   /* the leader's half width ... */
#define SDG_HH 4                   /* ... and half height */
#define SDG_LIGHT 54               /* the light around the diver (px) */
#define SDG_MIST_M 3000            /* the mist orb's metres */
#define SDG_SHOT_DMG 40

/* ---- the fight ------------------------------------------------------------- */
#define SDG_FOES 4
typedef struct SdgFoe {
    uint8_t kind, alive;
    int16_t hp, maxhp;
    uint8_t weak;
    int8_t mark;                   /* telegraphed at this diver (-1) */
    int8_t guard;                  /* shielding this foe (-1) */
    uint8_t hidden;
    uint8_t nodrop;
    int8_t flash;                  /* drawing: hit flash frames */
    uint8_t weakhit;               /* drawing: the weak-spot sparkle */
} SdgFoe;
enum { ACT_NONE, ACT_ITEM, ACT_PASS };
enum { MODE_ATTACK, MODE_DEFEND, MODE_HEAL, MODE_USE };
typedef struct SdgOrder { uint8_t act, slot, mode; int8_t target; } SdgOrder;
typedef struct SdgMsg { char text[48]; } SdgMsg;
#define SDG_MSGS 16
enum { BP_INTRO, BP_ORDER, BP_TARGET, BP_RESOLVE, BP_WIN, BP_LOSE, BP_FLED, BP_DONE };
typedef struct SdgBattle {
    SdgFoe foe[SDG_FOES];
    int nfoe;
    uint8_t boss;                  /* 0, 1 the Warden, 2 the Gloamheart */
    int mob;                       /* the creature that started it (-1) */
    uint8_t region;
    uint8_t phase;
    int cur;                       /* the diver being ordered */
    int menu;                      /* 0 left item, 1 right item, 2 pass, 3 run */
    int tmode, tsel;               /* target screen: mode, chosen foe or diver */
    SdgOrder ord[3];
    int8_t guarding[3];            /* this round: diver d defends diver guarding[d] (-1) */
    uint8_t guard_kind[3];         /* K_* of what d defends with */
    int actor;                     /* resolving: 0-2 divers, 3.. foes */
    int sub;                       /* a foe's second attack */
    SdgMsg msg[SDG_MSGS];
    int nmsg, msg_t;
    int round;
    int32_t xp, gold;
    int8_t drop;
    int8_t shake[3];               /* drawing: a diver hit */
    uint8_t levelled;
    uint8_t wipe;
    uint8_t ran;                   /* a run was tried this round */
    /* tallies for the tests */
    int16_t thorns_taken;          /* spines that pricked a diver */
    int16_t leech_healed;          /* HP the leech club drank back */
    int16_t covered;               /* blows a diver took for another */
    int16_t last_dmg;              /* the last blow a diver took (after shields) */
    int8_t last_target;            /* who took it */
    int8_t last_heavy;             /* it was a telegraphed blow */
    int16_t heals, revives;        /* creatures mending and bringing back */
} SdgBattle;
#define SDG_MSG_T 50               /* frames a line stays ... */
#define SDG_MSG_MIN 20             /* ... and A moves on after this many */
#define SDG_ESCAPE 25              /* % */
#define SDG_HIT 90                 /* % */
#define SDG_WEAK_NUM 3              /* a weak spot takes half again */
#define SDG_WEAK_DEN 2
#define SDG_DROP 10                /* % a relic drops */

/* ---- the one shared state ---------------------------------------------------- */
enum { SC_TITLE, SC_RAFT, SC_SHOP, SC_KIT, SC_DIVE, SC_MENU, SC_BATTLE, SC_WIPE, SC_SURFACE, SC_ENDING, SC_CODE };
#define SDG_CODE "MOONWAKE"        /* shown as MOON-WAKE: a new log with three eggs */
#define SDG_GOLD_MAX 9999
typedef struct SdgState {
    SdgProg prog, saved;
    SdgDive dive;
    SdgBattle bat;
    Rng rng;
    int scene, scene_t;
    uint32_t frame;
    /* menus */
    int title_sel, title_erase;
    int raft_sel;
    int shop_page, shop_sel;
    int kit_col, kit_sel, kit_held, kit_from, kit_scroll; /* held: item (0 none); from: -1 storage or d*2+s */
    int menu_sel, menu_target, menu_pick;  /* the dive's item menu */
    const char *note1, *note2;     /* a line on the dive screen (lore, chest) */
    int note_t;
    char notebuf[2][48];
    /* surfacing news */
    char surf[6][40];
    int nsurf;
    int ending, ending_t;
    char code[9];
    int code_pos, code_msg_t;
    uint8_t code_on;               /* the code is on: no saving, no goals, until the library */
    uint8_t bot_on, bot_goal;
    uint8_t snd_ev;
} SdgState;
extern SdgState sdg;

/* ---- the rules shared between files --------------------------------------- */
/* the effective level of a diver (Godblood +1, an Evil Potion -1) */
int sdg_diver_level(const SdgProg *p, int d);
int sdg_diver_maxhp(const SdgProg *p, int d);
/* what is left of a blow after the shields a diver holds, in thousandths */
int sdg_shield_mul(const SdgProg *p, int d);
int sdg_cap_gold(int32_t g);
bool sdg_holds(const SdgProg *p, int d, int it);
bool sdg_party_holds(const SdgProg *p, int it);   /* anyone, alive or not */
int sdg_alive_count(const SdgProg *p);
int sdg_stored(const SdgProg *p, int it);   /* owned and not in a hand */
bool sdg_can_buy(const SdgProg *p, int it);
bool sdg_buy(SdgProg *p, int it);
void sdg_refill(SdgProg *p);                /* every use back, HP full, revived */
void sdg_add_xp(SdgProg *p, int xp);        /* level-ups; returns nothing */
int sdg_relic_total(const SdgProg *p, int r);
void sdg_gain_relic(SdgProg *p, int r);
const char *sdg_diver_name(int d);
int sdg_rand(int lo, int hi);               /* the game's own dice */
int sdg_chance(int pct);
int sdg_isqrt(int v);

/* the dive (sdg_dive.c) */
void sdg_dive_begin(void);                  /* from the raft */
void sdg_dive_reset_mobs(void);
void sdg_dive_step(unsigned held, unsigned pressed);
bool sdg_solid_px(int px, int py);          /* a pixel of rock (doors and walls as they stand) */
bool sdg_cell_open(int c, int r);           /* water a diver can be in */
int sdg_depth(void);                        /* metres below the surface */
void sdg_use_item_dive(int d, int s, int target); /* the item menu's A */
bool sdg_dive_usable(int it);
void sdg_bomb_blast(int px, int py);
void sdg_open_door(int door);
/* news from a dive step (sdg.c reacts) */
enum { DV_NONE, DV_BATTLE, DV_SURFACE };
extern int sdg_dive_news;
extern int sdg_dive_news_mob;

/* the fight (sdg_battle.c) */
void sdg_battle_start(int mob, const uint8_t *kinds, int n, int boss);
void sdg_battle_step(unsigned pressed);
/* the round's arithmetic, open for the tests */
int sdg_attack_damage(int power, int level, int el, int weak, int roll_pct);
int sdg_reduce(int dmg, int pct);
bool sdg_order_valid(int d, int slot);
void sdg_group_for(int mobkind, int region, uint8_t *kinds, int *n); /* rolls a group */

/* the screens (sdg.c) */
void sdg_goto(int scene);
void sdg_surface(void);                     /* back at the raft: heal, bank, open pearls, save */
void sdg_wipe(void);                        /* back to the last surfacing */
void sdg_win_final(void);
void sdg_note(const char *a, const char *b);

/* the demo player (sdg_bot.c) */
enum { BOTG_GOLD, BOTG_CHERRY };
void sdg_bot_reset(void);
unsigned sdg_bot(void);
extern int sdg_bot_debug;
extern int sdg_bot_task;

/* drawing (sdg_draw.c), art (sdg_art.c), sound (sdg_audio.c) */
void sdg_draw_title(void);
void sdg_draw_raft(void);
void sdg_draw_shop(void);
void sdg_draw_kit(void);
void sdg_draw_dive(void);
void sdg_draw_menu(void);
void sdg_draw_battle(void);
void sdg_draw_wipe(void);
void sdg_draw_surface(void);
void sdg_draw_ending(void);
void sdg_draw_code(void);
void sdg_draw_label(int x, int y, int w, int h, int t);
void sdg_draw_item_icon(int it, int x, int y);
void sdg_draw_relic_icon(int r, int x, int y);
void sdg_draw_head_icon(int h, int x, int y);

enum {
    SP_DIVER, SP_DIVER2, SP_FIZZLE, SP_PRICKLE, SP_FROND, SP_NIPPER, SP_GLOB, SP_SMOG, SP_CLAMPER, SP_TINFIN,
    SP_WHORL, SP_JELLY, SP_GROPER, SP_BURRNUT, SP_STILTER, SP_LOUSE, SP_HAUNT, SP_SQUID, SP_GRINFISH,
    SP_WORM, SP_WARDEN, SP_EYE, SP_ARM, SP_CHEST, SP_CHEST_OPEN, SP_LEVER, SP_LEVER_ON, SP_HEAD0, SP_HEAD1,
    SP_HEAD2, SP_FACE0, SP_FACE1, SP_FACE2, SP_ICON_SHOP, SP_ICON_KIT, SP_ICON_DIVE, SP_SHELL_IN,
    SP_COUNT
};
void sdg_art_load(void);
int sdg_art_bad(void);
const Sprite *sdg_sprite(int id);

void sdg_audio_load(void);
extern int SDG_MUS_TITLE, SDG_MUS_RAFT, SDG_MUS_BATTLE, SDG_MUS_WARDEN, SDG_MUS_GLOAM, SDG_MUS_DEEP,
    SDG_MUS_STILL, SDG_MUS_WIN, SDG_MUS_WIPE, SDG_MUS_END, SDG_MUS_LEVEL;

#endif
