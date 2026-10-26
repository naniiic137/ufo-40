/* HELLSPUR - shared declarations. Cartridge 40 of UFO 40.
 * A tribute to Grimstone (UFO 50 #40); see docs/games/40-hellspur.md.
 *
 * The Territory of Calico has been dragged down into Hell. In the saloon at
 * Gladwell a widow gathered seven strangers and a dog; the saloon burned,
 * and the angel who came for them had time to carry out only four. Those
 * four are the posse. A full-size Western JRPG: a top-down world of towns,
 * dungeons and mines, visible hellfire wisps that step when you step and
 * chase the conga line, side-on battles where every plain attack is a press
 * on a timing bar (yellow hits, red is a sure critical), Teeth for money
 * and a bank that pays interest after every battle.
 *
 * Everything is whole numbers and table driven, and every random roll comes
 * from the cartridge's own seeded generator, so a run plays the same on
 * every platform. */
#ifndef SPUR_H
#define SPUR_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

/* ---- buttons ------------------------------------------------------------- */
/* Owner's rule: the main action (talk, confirm, the timing press) is on A
 * (Z / Space / Cross). Grimstone has it on button 2, which UFO 40 calls A,
 * so nothing moves; B opens the menu and cancels, as in the original. */
#define SPUR_OK BTN_A
#define SPUR_BACK BTN_B
#define SPUR_DIRS (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)

/* The title's demo plays itself: while it runs, the cartridge reads the demo
 * player's buttons instead of the pad. Every btn/btnp/btn_repeat/
 * input_consume in the cartridge goes through these. */
bool spur_btn(int m);
bool spur_btnp(int m);
bool spur_btn_repeat(int m);
void spur_consume(void);
void spur_input_frame(void);
#define btn(m) spur_btn(m)
#define btnp(m) spur_btnp(m)
#define btn_repeat(m) spur_btn_repeat(m)
#define input_consume() spur_consume()

/* ---- the screen ---------------------------------------------------------- */
#define TS 16                     /* a tile is 16 x 16 pixels */
#define VIEW_W 20                 /* tiles across the screen */
#define VIEW_H 12
#define SPUR_MAX_W 128            /* the biggest map (the Territory) */
#define SPUR_MAX_H 96

/* ---- tiles ----------------------------------------------------------------- */
enum {
    /* outdoors */
    T_SEA, T_RIVER, T_GRASS, T_PRAIRIE, T_SAND, T_DIRT, T_SCRUB, T_FOREST, T_PINE, T_CACTUS,
    T_HILL, T_MOUNT, T_PEAK, T_SWAMP, T_ROAD, T_BRIDGE, T_FERRY, T_LAVA, T_ASH, T_BONES,
    T_CLOUD, T_MESA, T_SALT, T_DEADTREE, T_FLOWERS,
    /* places on the big maps (walk onto them to go in) */
    T_TOWN, T_FORT, T_MISSION, T_HOUSE, T_CAVE, T_MINE, T_TOWER, T_GATE, T_SPIKE, T_SPIKE_GONE,
    T_CITADEL, T_SHRINE, T_SALOONSKY, T_SMITHY, T_RUIN,
    /* blocks the story clears */
    T_ANGELS, T_RUBBLE, T_FIREWALL, T_BARRIER, T_LOCKED,
    /* indoors and in town */
    T_FLOOR, T_STONE, T_WALL, T_ROCK, T_ROOF, T_BWALL, T_DOOR, T_WINDOW, T_COUNTER, T_FENCE,
    T_WELL, T_BARREL, T_CRATE, T_UP, T_DOWN, T_EXIT, T_CHEST, T_BED, T_TABLE, T_RAIL,
    T_ORE, T_HERB, T_WATER, T_BARS, T_ALTAR, T_TORCH, T_GRAVE, T_BURNING, T_PEW, T_RUG,
    T_BONEWALL, T_DREAMFLOOR, T_HELLFLOOR, T_HELLWALL, T_GLASS, T_SIGN, T_BOOKS, T_PIANO, T_TRACKS, T_STOOL,
    T_TREE, T_HIVE, T_WINCH, T_LIFT, T_PORTAL, T_CAULDRON,
    T_COUNT
};

enum { TF_WALK = 1, TF_FLY = 2, TF_LAND = 4, TF_ENTER = 8, TF_SEARCH = 16, TF_WATER = 32, TF_HIGH = 64 };
extern const uint8_t SPUR_TILE_FLAGS[T_COUNT];
static inline bool spur_tile_walk(int t) { return t >= 0 && t < T_COUNT && (SPUR_TILE_FLAGS[t] & TF_WALK) != 0; }

/* ---- maps -------------------------------------------------------------------- */
enum {
    M_WORLD, M_HELL, M_SALOON,
    M_GLADWELL, M_MERIDIAN, M_STCLARE, M_HOPE, M_STEEPLE, M_FORT, M_DOLORES, M_HOLLOW, M_LASTCHANCE,
    M_ASHMARKET, M_HERMITAGE,
    M_PIKESHACK, M_ROOST, M_THORNS, M_WIDOW, M_SHRINE, M_SKYBAR, M_SMITH,
    M_DRYWASH, M_SIXPENNY, M_PEN1, M_PEN2, M_NOTCH, M_CRYPT, M_ROOSTMINE, M_HOLLOWMINE, M_CUT,
    M_SNAKE1, M_SNAKE2, M_DREAM1, M_DREAM2, M_TOWER1, M_TOWER2, M_TOWER3, M_TOWER4,
    M_SPUR1, M_SPUR2, M_SPUR3,
    M_COUNT
};
enum { MK_WORLD, MK_TOWN, MK_PLACE, MK_DUNGEON };
#define SPUR_FIRST_TOWN M_GLADWELL
#define SPUR_TOWNS 11              /* Gladwell ... the Hermitage */
#define SPUR_FIRST_DUNGEON M_DRYWASH
#define SPUR_DUNGEON_MAPS (M_COUNT - M_DRYWASH)

/* what lives on a map */
typedef struct { int8_t x, y; uint8_t map; int8_t tx, ty; } SpurLink;   /* stairs, doors, edges */
typedef struct {
    int8_t x, y;
    uint8_t look;         /* SPUR_LOOK_* */
    uint8_t pal;          /* colour set */
    uint8_t dir;          /* 0 down 1 up 2 left 3 right */
    uint8_t wander;       /* walks about */
    uint16_t talk;        /* TALK_* */
    int16_t show_if, hide_if; /* flag (or -1) */
} SpurNpc;
typedef struct { int8_t x, y; uint16_t id; int16_t item; int32_t teeth; uint8_t hidden; } SpurChest;
typedef struct { int8_t x, y; uint8_t troop; int16_t flag; } SpurAmbush;      /* flag -1: every time */
typedef struct { int8_t x, y; uint8_t kind, table; } SpurSpot;               /* forage / ore */
typedef struct { int8_t x, y; uint8_t service; uint8_t arg; } SpurDoor;      /* a town door */
enum { SV_NONE, SV_HOTEL, SV_CHAPEL, SV_SHOP_A, SV_SHOP_B, SV_BANK, SV_SALOON, SV_TRAIN, SV_STABLES, SV_SHERIFF,
       SV_LOCKED, SV_STORY };
enum { SPOT_FORAGE, SPOT_ORE };

#define SPUR_MAX_NPC 24
#define SPUR_MAX_LINK 40
#define SPUR_MAX_CHEST 16
#define SPUR_MAX_AMBUSH 8
#define SPUR_MAX_SPOT 6
#define SPUR_MAX_DOOR 16

typedef struct {
    int id, kind, w, h;
    uint8_t tile[SPUR_MAX_H][SPUR_MAX_W];
    int nnpc, nlink, nchest, nambush, nspot;
    SpurNpc npc[SPUR_MAX_NPC];
    SpurLink link[SPUR_MAX_LINK];
    SpurChest chest[SPUR_MAX_CHEST];
    SpurAmbush ambush[SPUR_MAX_AMBUSH];
    SpurSpot spot[SPUR_MAX_SPOT];
    int ndoor;
    SpurDoor door[SPUR_MAX_DOOR];
    int zone;             /* encounter zone for the whole map (dungeons, towns: -1 none) */
    int music;
    int start_x, start_y; /* where you arrive from outside */
    int back_x, back_y;   /* the far end: where you arrive coming back down/up */
} SpurMap;

/* fixed facts about each map (name, music, where it sits on the big map) */
typedef struct {
    const char *name;
    uint8_t kind;
    uint8_t parent;       /* the big map it opens off (M_WORLD / M_HELL) */
    int8_t px, py;        /* its door there */
    uint8_t station;      /* train stop number + 1 (0 none) */
} SpurMapInfo;
extern const SpurMapInfo SPUR_MAPINFO[M_COUNT];

/* ---- encounter zones ------------------------------------------------------------ */
#define SPUR_ZONE_TROOPS 8
typedef struct {
    const char *name;
    uint8_t rate;             /* wisps per 100 steps, roughly */
    uint8_t troop[SPUR_ZONE_TROOPS]; /* troop ids, 0 ends */
    uint8_t rare, rare_pct;   /* a rare troop and its chance in 100 */
} SpurZone;
extern const SpurZone SPUR_ZONE[];
extern const int SPUR_ZONES;
int spur_world_zone(int map, int x, int y); /* the zone at a place on a big map */

/* ---- heroes ------------------------------------------------------------------------ */
enum { H_TUCKER, H_SAGE, H_HOLLIS, H_QUILL, H_VERA, H_HATTIE, H_CORA, H_BISCUIT, H_COUNT };
enum { ST_HP, ST_SP, ST_ATK, ST_DEF, ST_SPD, ST_EVD, ST_RES, ST_VIT, ST_COUNT };
typedef struct {
    const char *name, *klass;
    const char *pick_line[2];    /* what the angel sees in the burning saloon */
    int16_t base[ST_COUNT];      /* level 1 */
    int16_t grow[ST_COUNT];      /* per level, in tenths */
    uint8_t look, pal;
} SpurHeroDef;
extern const SpurHeroDef SPUR_HERO[H_COUNT];

/* passives (levels 0..5) */
enum { PS_MELEE, PS_PISTOL, PS_RIFLE, PS_SHOTGUN, PS_HEAVY, PS_SPIRIT, PS_DOC, PS_SURVIVOR, PS_DUAL, PS_ONEARM, PS_DOG,
       PS_COUNT };
extern const char *const SPUR_PASSIVE_NAME[PS_COUNT];
extern const char *const SPUR_PASSIVE_DESC[PS_COUNT];

/* active skills */
enum {
    SK_NONE,
    /* Cora's dances */
    SK_REEL, SK_SHIMMY, SK_MINUET, SK_CANCAN,
    /* outlaw tricks */
    SK_LIFT, SK_LIELOW, SK_DEADSHOT,
    /* field work */
    SK_RUMMAGE, SK_PROSPECT,
    /* doctoring */
    SK_MEND, SK_SUTURE, SK_PURGE, SK_SPLINT, SK_RAISE,
    /* the rest */
    SK_PAYOFF, SK_COVER, SK_STEADY,
    /* Quill's */
    SK_OBSERVE, SK_RAGE, SK_BRACE, SK_SIPHON, SK_JAB, SK_UPPERCUT, SK_HAYMAKER,
    /* Sage's, from the animals */
    SK_KEENEYE, SK_BITE, SK_DUSTSTORM, SK_HOWL, SK_TROT, SK_RENEW, SK_BLENDIN, SK_SNARL, SK_FLUTTER, SK_BUCK,
    SK_SNARE, SK_SWOOP, SK_MAUL, SK_SHELL,
    SK_COUNT
};
enum { SKT_ENEMY, SKT_ALLENEMY, SKT_ALLY, SKT_ALLALLY, SKT_SELF, SKT_DEADALLY, SKT_FIELD };
enum { SKU_BATTLE = 1, SKU_FIELD = 2 };
typedef struct {
    const char *name, *desc;
    uint8_t sp, target, use;
    int16_t power;
} SpurSkillDef;
extern const SpurSkillDef SPUR_SKILL[SK_COUNT];

/* what a hero learns by level: a passive step or a skill */
typedef struct { uint8_t level; int8_t passive; uint8_t skill; } SpurLearn;
#define SPUR_MAX_LEARN 14
extern const SpurLearn SPUR_LEARN[H_COUNT][SPUR_MAX_LEARN];

/* Sage's ten animals: each offers a choice of two */
#define SPUR_ANIMALS 10
typedef struct { const char *name; uint8_t a, b; uint8_t map; int8_t x, y; } SpurAnimal;
extern const SpurAnimal SPUR_ANIMAL[SPUR_ANIMALS];
#define SPUR_QUILL_MAX 10          /* Quill's skill list holds ten, OBSERVE among them */

/* ---- items ------------------------------------------------------------------------- */
enum { IK_NONE, IK_USE, IK_WEAPON, IK_NECK, IK_BODY, IK_FEET, IK_ORE, IK_TREAT, IK_LESSON };
enum { WC_NONE, WC_MELEE, WC_PISTOL, WC_RIFLE, WC_SHOTGUN, WC_HEAVY, WC_BOW };
enum {
    IF_TWOHAND = 1, IF_RELOAD = 2, IF_UNIQUE = 4, IF_LIGHT = 8, IF_TUCKER = 16, IF_FIELD = 32, IF_BATTLE = 64,
    IF_STEADY = 128, IF_SPIRIT = 256
};
enum { /* what a usable item does */
    IU_NONE, IU_HEAL, IU_SP, IU_SPALL, IU_REVIVE, IU_CURE, IU_CAMP, IU_STUN, IU_WARP, IU_WARD, IU_TREAT_D, IU_TREAT_A,
    IU_TREAT_E, IU_LESSON_STEADY, IU_LESSON_PROSPECT
};
typedef struct {
    const char *name;
    uint8_t kind, wclass;
    uint16_t flags;
    int32_t price;            /* 0 = not sold; sells for half */
    int16_t power;            /* ATK, DEF, EVD, SPD or the amount it heals */
    uint8_t hits;             /* weapons: hits a turn */
    uint8_t use;              /* IU_* */
    int16_t half;             /* the item it leaves behind with the doctor's skill (-1) */
    uint8_t half_doc;         /* the DOC level that makes it do so */
    int8_t res;               /* necklaces: RES bonus; +SP for the spirit beads */
} SpurItemDef;

enum {
    IT_NONE,
    /* things to use */
    IT_BANDAGE, IT_HALFBANDAGE, IT_HERB, IT_HALFHERB, IT_SALVE, IT_HALFSALVE, IT_MEDCHEST, IT_HALFCHEST,
    IT_SNAKEROOT, IT_HALFROOT, IT_MINT, IT_GINGERPOP, IT_CACTUSFIG, IT_BLACKBREW, IT_HOLYOIL, IT_CAMPKIT,
    IT_LARIAT, IT_HALOPIN, IT_SMUDGE, IT_BEEFBONE, IT_SOUPBONE, IT_WISHBONE, IT_STEADYBOOK, IT_PROSPECTBOOK,
    /* pistols */
    IT_POCKET, IT_DUCKFOOT, IT_BUNTLINE, IT_SILVERBELLE, IT_LAWMAN, IT_DEVILSOWN,
    /* shotguns */
    IT_SCATTER, IT_QUAD, IT_BIGBORE, IT_ELEPHANT,
    /* rifles */
    IT_LONGTOM, IT_CAVALRY, IT_PLAINSMAN, IT_NEEDLEGUN, IT_DOOMSDAY,
    /* heavy */
    IT_CRANKGUN, IT_GRINDER, IT_THRESHER, IT_HELLFIRE,
    /* melee */
    IT_CLEAVER, IT_BOWIE, IT_BLACKGLASS, IT_WIDOWMAKER,
    /* bows */
    IT_SAPLING, IT_HORNBOW, IT_THUNDERBOW,
    /* necks */
    IT_KERCHIEF, IT_BOLOTIE, IT_LUCKYCOIN, IT_TOOTHNECK, IT_STARSAPPHIRE, IT_WARD, IT_BEADS, IT_STEADYEYE,
    /* body: light, then heavy */
    IT_COTTON, IT_WOOL, IT_BUCKSKIN, IT_SILK, IT_SATIN,
    IT_VEST, IT_WOOLCOAT, IT_LEATHER, IT_SERAPE, IT_DUSTER, IT_HELLHIDE,
    /* feet */
    IT_WORNBOOTS, IT_RIDING, IT_SNAKESKIN, IT_OSTRICH, IT_GATOR, IT_MOCCASIN,
    /* ores */
    IT_IRONORE, IT_SILVER, IT_GOLD, IT_TOPAZ, IT_EMERALD, IT_RUBY, IT_DIAMOND, IT_HELLITE,
    IT_COUNT
};
extern const SpurItemDef SPUR_ITEM[IT_COUNT];

/* key items: a bit each */
enum {
    KI_CRANK, KI_HONEYCOMB, KI_LETTER, KI_BELL, KI_CELLARKEY, KI_PLUNGER, KI_STORMFEATHER, KI_FIRECHARM,
    KI_IRONSPUR, KI_CLAWBAR, KI_NIGHTMARES, KI_COUNT
};
extern const char *const SPUR_KEYITEM_NAME[KI_COUNT];
extern const char *const SPUR_KEYITEM_DESC[KI_COUNT];

#define SPUR_BAG 20               /* two pages of ten */

/* ---- enemies ------------------------------------------------------------------------- */
enum { EA_HIT, EA_MULTI, EA_ALL, EA_POISON, EA_PARALYZE, EA_DEATH, EA_DRAIN, EA_GUARD, EA_HEALALL, EA_FIRE,
       EA_CALL, EA_WEAKEN, EA_POISONALL, EA_STEAL, EA_FLEE, EA_DELETE, EA_IDLE };
typedef struct { uint8_t kind, weight; int16_t power; } SpurAct;
typedef struct {
    const char *name;
    uint8_t art, pal, size;      /* sprite family, colour set, 0 small 1 big 2 huge */
    int32_t hp;
    int16_t atk, def, spd, evd, res;
    int32_t xp, teeth;
    SpurAct act[4];
    uint8_t observe;             /* the skill Quill can learn from it (0 none) */
    int16_t lift_common, lift_rare; /* what Vera can lift */
    uint8_t flags;
} SpurEnemyDef;
/* enemy art families (spur_art.c); the bosses have their own */
enum { A_RAT, A_WEED, A_BIRD, A_BAT, A_BANDIT, A_PUCK, A_DIGGER, A_MOLD, A_DOG, A_SPIDER, A_SNAKE, A_GHOUL, A_GOLEM,
       A_HARE, A_GOAT, A_APE, A_RIDER, A_TROOPER, A_BISON, A_MOTH, A_CENTI, A_GATOR, A_HAG, A_EYE, A_OWL, A_CANNON,
       A_GHOST, A_BEAN, A_MONK,
       /* bosses */
       A_PUCKKING, A_SCOURGE, A_BRUTUS, A_TALLYMAN, A_JACK, A_MAGPIE, A_MIMIC, A_MOLLY, A_CAPTAIN, A_STORMCROW, A_WORM,
       A_MOTHER, A_ARM, A_BLIGHT, A_CARNAGE, A_HUNGER, A_REAPER, A_JUGGERNAUT, A_GOVERNOR, A_NADIR, A_ZENITH };

enum { EF_BOSS = 1, EF_NORUN = 2, EF_UNDEAD = 4, EF_FLIER = 8, EF_NOBRIBE = 16, EF_NOSTUN = 32, EF_STONE = 64 };
extern const SpurEnemyDef SPUR_ENEMY[];
extern const int SPUR_ENEMIES;
enum { /* the bosses and the ones the story names */
    EN_NONE = 0
};
int spur_enemy_find(const char *name);

/* troops: up to six */
#define SPUR_TROOP_MAX 6
typedef struct { uint8_t e[SPUR_TROOP_MAX]; uint8_t min, max; uint8_t flags; } SpurTroop;
enum { TRF_BOSS = 1, TRF_RANDOMN = 2, TRF_NORUN = 4 };
extern const SpurTroop SPUR_TROOP[];
extern const int SPUR_TROOPS;

/* ---- shops ------------------------------------------------------------------------------- */
#define SPUR_SHOP_ITEMS 10
typedef struct { const char *name; uint8_t item[SPUR_SHOP_ITEMS]; } SpurShop;
extern const SpurShop SPUR_SHOP[];
extern const int SPUR_SHOPS;
/* what each town offers */
typedef struct {
    int16_t hotel;            /* price, 0 free, -1 none */
    int8_t chapel, bank, saloon, stables, shop_a, shop_b;
    int16_t stable_price;
} SpurTownDef;
extern const SpurTownDef SPUR_TOWN[SPUR_TOWNS];

/* ---- story flags --------------------------------------------------------------------------- */
enum {
    F_POSSE, F_WREN, F_PIKE_FOUND, F_PIKE_PAID, F_PUCKKING, F_FERRY, F_BRUTUS_GONE, F_BRUTUS_BEATEN,
    F_HOPE_FREE, F_SCOURGE, F_AMBROSE, F_JACK, F_FORT, F_MAGPIE, F_CANCAN, F_MOLLY, F_MOLLY_PAID, F_BELL,
    F_MIMIC, F_CRYPT_KEY, F_ROOST_DOOR, F_HOLLOW_PAST, F_PLUNGER, F_CUT_OPEN, F_STORMCROW, F_STORM_TRADED,
    F_FIRECHARM, F_WORM, F_MOTHER, F_GATE_OPEN, F_BLIGHT, F_CARNAGE, F_HUNGER, F_REAPER, F_CLAWBAR,
    F_SPIKE1, F_SPIKE2, F_SPIKE3, F_GOVERNOR, F_WON,
    F_LETTER, F_LETTER_PAID, F_SOLDIER_RAISE, F_SOLDIER_DEAD, F_QUILL_SW, F_TUCKER_COVER, F_SAGE_BEADS,
    F_SMITH_DONE, F_JUGGERNAUT, F_SKYBAR, F_LACKEY_GIFT, F_HIVE, F_HONEY_GIVEN, F_GIL_GAVE, F_STEADY_SOLD,
    F_ANIMAL0,                    /* ... ten in a row */
    F_AMBUSH0 = F_ANIMAL0 + SPUR_ANIMALS, /* fixed ambushes, 48 of them */
    F_MISC0 = F_AMBUSH0 + 48,     /* odds and ends: hidden finds, one-off talks */
    F_COUNT = F_MISC0 + 64
};
#define SPUR_FLAG_BYTES ((F_COUNT + 7) / 8)
#define SPUR_CHESTS 672             /* sixteen a map */
#define SPUR_CHEST_BYTES (SPUR_CHESTS / 8)

/* ---- the posse ------------------------------------------------------------------------------- */
enum { SS_POISON = 1, SS_PARALYZED = 2 };
typedef struct {
    uint8_t in;               /* saved in the saloon */
    uint8_t level;
    int32_t xp;
    int16_t hp, sp;           /* current */
    uint8_t status;
    uint8_t equip[5];         /* right hand, left hand, neck, body, feet */
    uint32_t skills_lo, skills_hi; /* learned skills, a bit each (SK_*) */
    uint8_t passive_bonus[PS_COUNT]; /* passives from teachers (on top of levels) */
    int16_t treat[ST_COUNT];  /* bones eaten */
    uint8_t reload;           /* the gun must be reloaded */
    uint8_t observed;         /* Quill: skills learned by observing */
} SpurHero;
enum { EQ_RHAND, EQ_LHAND, EQ_NECK, EQ_BODY, EQ_FEET, EQ_COUNT };

/* ---- the save ----------------------------------------------------------------------------- */
#define SPUR_SAVE_MAGIC 0x53505231u
typedef struct {
    uint32_t magic;
    uint8_t valid;            /* a game to continue */
    uint8_t order[4];         /* hero ids, 0xFF empty */
    uint8_t nposse;
    SpurHero hero[H_COUNT];
    uint8_t bag[SPUR_BAG];
    uint32_t keyitems;
    int32_t teeth, bank;
    uint8_t flags[SPUR_FLAG_BYTES];
    uint8_t chests[SPUR_CHEST_BYTES];
    uint8_t map;              /* where the game was saved (a town) */
    int8_t x, y;
    uint8_t chapel;           /* the last chapel town (a map id) */
    uint8_t horses;           /* hired horses waiting (0 none) */
    uint8_t stations;         /* train stops opened, a bit each */
    int32_t battles, deaths;  /* stats: battles fought, posse members fallen */
    int32_t steps;
    uint32_t play_secs;
    /* records kept across games */
    int32_t best_battles, wins, cherries, max_level;
    uint8_t wolfpack;         /* never set: the original's all-dogs code is not in UFO 40 */
    uint8_t spare[15];
} SpurSave;

/* ---- battle ----------------------------------------------------------------------------- */
#define SPUR_FOES 6
enum { CMD_ATTACK, CMD_SKILL, CMD_ITEM, CMD_DEFEND, CMD_RUN, CMD_COUNT };
typedef struct {
    uint8_t cmd, target, skill, item_slot;
    bool done;
} SpurOrder;
typedef struct {
    int16_t e;                /* enemy id, -1 empty */
    int32_t hp, maxhp;
    int8_t buff_atk, buff_def;
    uint8_t stun;             /* lariat / uppercut: out until struck */
    uint8_t tied;             /* snare: turns left */
    uint8_t weak;             /* snarl: ATK down steps */
    uint8_t lifted;           /* Vera has lifted from it */
    uint8_t observed;         /* Quill has learned all he can */
    int16_t x, y;             /* where it stands */
    uint8_t flash, dying, fled;
    uint8_t guard;
} SpurFoe;
typedef struct {
    int8_t atk, def, evd, spd, crit; /* stacking buffs */
    uint8_t defend, rage, covering, lielow;
    uint8_t steady;           /* the red zone is wide */
} SpurBuff;

/* the timing bar */
#define BAR_X 96
#define BAR_W 128
#define BAR_MAXZ 8
typedef struct { int16_t a, red, b; uint8_t result; } SpurZoneBar; /* yellow a..red, red red..b */
enum { ZR_PENDING, ZR_MISS, ZR_HIT, ZR_CRIT };

enum { /* battle phases */
    BP_INTRO, BP_COMMAND, BP_TARGET, BP_SKILLMENU, BP_ITEMMENU, BP_ALLYTARGET, BP_RUN, BP_ACT, BP_BAR, BP_MSG,
    BP_WIN, BP_LEVELUP, BP_LOSE, BP_FLED, BP_DONE
};
typedef struct {
    int troop;
    int nfoe;
    SpurFoe foe[SPUR_FOES];
    SpurOrder order[4];
    SpurBuff buff[4];
    int phase, phase_t;
    int cur;                  /* posse slot being ordered */
    int sel, sub_sel, sub_top;
    int target_sel;
    /* the turn */
    int queue[12], nqueue, qi; /* actor ids: 0..3 posse slots, 10.. foes */
    int actor;
    char msg[2][64];
    int msg_t;
    /* the bar */
    int nz;
    SpurZoneBar z[BAR_MAXZ];
    int bar_x;                /* marker, in 1/16 px */
    int bar_speed;
    int bar_kind;             /* WC_* of the swing */
    int bar_target;
    int bar_heavy_t, bar_shots, bar_held;
    int bar_done_t;
    int bar_auto;             /* dog / rage: the game presses for you */
    int bar_auto_at;
    int bar_delta;            /* how far the marker moved last frame */
    bool boss, norun, wiped, won, fled;
    int xp_gain, teeth_gain;
    int lift_item;
    int levelups[4];
    char lvl_msg[4][48];
    int ambush_flag;
    int shake, flash;
    int turn;
    int final_kill;           /* the final fight: which god fell first */
    int run_tries;
} SpurBattle;

/* ---- the field ------------------------------------------------------------------------------- */
#define SPUR_TRAIL 4
#define SPUR_WISPS 5
typedef struct { int16_t x, y; uint8_t alive; uint8_t wait; int16_t ox, oy; uint8_t t; } SpurWisp;
typedef struct { int16_t x, y, dir; } SpurPos;

/* menus / dialog boxes */
enum { UI_NONE, UI_TEXT, UI_MENU, UI_SHOP, UI_HOTEL, UI_CHAPEL, UI_BANK, UI_STABLE, UI_TRAIN, UI_SALOON,
       UI_YESNO, UI_ANIMAL, UI_PICK, UI_NUMBER };
#define SPUR_TEXT_MAX 8
typedef struct {
    int kind;
    const char *lines[SPUR_TEXT_MAX * 3];
    char buf[SPUR_TEXT_MAX * 3][64];
    int nlines, page, t;
    int after;                /* an event to run when the text closes */
    int yes_event, no_event;
    int sel, sel2, top, mode, shop, num, num_max;
} SpurUi;

/* field menu pages */
enum { FM_ROOT, FM_ITEM, FM_ITEM_DO, FM_ITEM_WHO, FM_SKILL_WHO, FM_SKILL, FM_SKILL_TGT, FM_EQUIP_WHO, FM_EQUIP,
       FM_EQUIP_PICK, FM_STATS_WHO, FM_STATS, FM_ORDER, FM_ORDER2, FM_KEYS };

/* ---- the whole game ---------------------------------------------------------------------- */
enum { GS_TITLE, GS_INTRO, GS_FIELD, GS_BATTLE, GS_ENDING, GS_OVER, GS_DEMO };
typedef struct {
    int state, state_t;
    Rng rng;
    SpurSave s;               /* the game being played (saved as is) */
    SpurMap map;              /* the map on screen */
    int map_id;
    /* the field */
    SpurPos pos[SPUR_TRAIL];  /* 0 the leader, then the followers */
    SpurPos prev[SPUR_TRAIL]; /* where they stood before this step (for drawing) */
    int flash_poison;
    int move_t, move_dir, moving;
    int step_frames;
    int face;
    SpurWisp wisp[SPUR_WISPS];
    int step_count, ward_steps, poison_steps;
    int mounted;              /* 1 on hired horses, 2 on the nightmares */
    int horse_step;
    int npc_x[SPUR_MAX_NPC], npc_y[SPUR_MAX_NPC], npc_dir[SPUR_MAX_NPC], npc_t[SPUR_MAX_NPC];
    int cam_x, cam_y;
    SpurUi ui;
    int menu_page, menu_sel, menu_who, menu_slot, menu_top, menu_sel2;
    int menu_skill;
    int fade;
    int pending_event;
    int pending_battle, pending_flag;
    int intro_page;
    int title_sel;
    int msg_t; char msg[48];
    /* the opening: the burning saloon */
    uint8_t saved[H_COUNT];
    int nsaved;
    /* battle */
    SpurBattle b;
    int return_music;
    /* the ending */
    int end_t;
    bool cherry_run;
    int god;                  /* tests: nothing hurts the posse */
    int demo;                 /* the title's demo is playing */
    int demo_t;
    int last_battle_won;
    int ani;
    int map_t;                /* frames since this map was entered */
    int no_spawn;             /* tests: no new wisps */
} SpurGame;

extern SpurGame sg;
extern SpurSave spur_records;   /* the save file: the records live on between games */
extern bool spur_sim;           /* sounds off (the bot's look-ahead) */

/* ---- spur.c ---------------------------------------------------------------------------- */
void spur_sfx(const char *name);
void spur_toast(const char *m);
bool spur_flag(int f);
void spur_set_flag(int f);
void spur_clear_flag(int f);
bool spur_chest_open(int id);
void spur_set_chest(int id);
bool spur_key(int k);
void spur_give_key(int k);
void spur_take_key(int k);
void spur_save_game(void);
void spur_write_file(void);
void spur_set_state(int s);
void spur_award_level_goal(void);
int spur_rand(int lo, int hi);

/* ---- spur_data.c ----------------------------------------------------------------------- */
int spur_xp_for(int level);                 /* total XP to reach a level */
int spur_stat(int h, int st);               /* with equipment, bones and level */
int spur_base_stat(int h, int st);
int spur_maxhp(int h);
int spur_maxsp(int h);
int spur_passive(int h, int p);             /* level of a passive */
bool spur_has_skill(int h, int sk);
void spur_learn_skill(int h, int sk);
int spur_skill_count(int h);
int spur_skill_list(int h, int *out);       /* skills in the menu order */
bool spur_can_equip(int h, int item, int slot);
int spur_weapon_class(int h);               /* of what is in hand */
int spur_attack_power(int h);               /* ATK used by the plain attack */
int spur_level_up_check(int h, char *msg, int n); /* apply level ups; returns how many */
void spur_hero_init(int h);
const char *spur_item_name(int it);
int spur_bag_count(void);
int spur_bag_add(int it);                   /* slot or -1 when full */
void spur_bag_remove(int slot);
int spur_bag_find(int it);
int spur_posse_alive(void);
int spur_hero_at(int slot);                 /* hero in posse slot, -1 */
bool spur_alive(int h);
int spur_doc_level(void);                   /* best DOC among the living */
int spur_sell_price(int it);
bool spur_slot_is_weapon(int slot);

/* ---- spur_world.c ----------------------------------------------------------------------- */
void spur_world_build(SpurMap *m, int id);  /* lays out a map */
int spur_tile_at(const SpurMap *m, int x, int y);
bool spur_walkable(int x, int y, bool flying);
const char *spur_map_name(int id);
int spur_map_check(int id);                 /* tests: problems found in a map */
int spur_chest_total(void);
int spur_poi_at(int map, int x, int y);     /* the map a big-map tile opens */

/* ---- spur_field.c ----------------------------------------------------------------------- */
void spur_enter_map(int id, int x, int y, int dir);
void spur_field_update(void);
void spur_field_after_battle(bool won);
void spur_place_party(int x, int y, int dir);
void spur_clear_wisps(void);
bool spur_in_town(void);
int spur_facing_x(void);
int spur_facing_y(void);
int spur_npc_at(int x, int y);
bool spur_npc_visible(int i);
void spur_start_battle(int troop, int flag);
void spur_wipe_return(void);
void spur_respawn_at_chapel(void);
void spur_set_mount(int m);
void spur_spawn_wisp_at(int x, int y);
void spur_refresh_map(void);
void spur_leave_town(void);
void spur_open_chest(int i);
void spur_search_tile(int x, int y, int t);
uint64_t spur_world_reach(int sx, int sy); /* bit per map: the doors walkable to from there */
int spur_dun_index(int map);

/* ---- spur_menu.c ------------------------------------------------------------------------- */
void spur_say(const char *text);            /* a text box (lines split by '\n' and pages by '\f') */
void spur_say_then(const char *text, int event);
void spur_ask(const char *text, int yes_event, int no_event);
void spur_open_door(int service, int arg);
void spur_pick(const char *title, const char *const *opts, int n, int mode);
void spur_number(const char *title, int max, int mode);
void spur_menu_draw(void);
void spur_draw_posse_panel(int x, int y, int w);
void spur_open_field_menu(void);
void spur_ui_update(void);
bool spur_ui_open(void);
void spur_use_item_field(int slot, int who);
bool spur_use_skill_field(int who, int sk, int target);
int spur_heal_amount(int it);
int spur_skill_heal(int h, int sk);
bool spur_menu_event(int ev);
void spur_hotel_rest(int price);

/* ---- spur_battle.c ------------------------------------------------------------------------ */
void spur_battle_begin(int troop, int flag);
void spur_battle_update(void);
int spur_bar_zone_kind(int h);
int spur_damage_to_foe(int h, int foe, int result, int extra_pct);
void spur_battle_end(void);
void spur_battle_draw(void);
int spur_foe_alive(int i);
int spur_foes_left(void);
void spur_foe_kill(int i);

/* ---- spur_story.c ------------------------------------------------------------------------- */
void spur_talk(int npc);
void spur_event(int ev);
void spur_step_event(int x, int y);         /* stepping on a tile of a map */
void spur_enter_event(int map);
bool spur_event_door(int map);          /* walking into a place on a big map: false = held back */
bool spur_story_door(int service, int arg); /* a door the story answers */
void spur_story_pick(int mode, int choice);  /* a choice made in a story list */
void spur_story_number(int mode, int value);
bool spur_battle_won_event(int troop);
const char *spur_ending_line(int k);
extern const char *const SPUR_INTRO[];
extern const int SPUR_INTRO_PAGES;
const char *spur_objective(void);
void spur_saloon_exit(void);

/* ---- spur_draw.c / spur_art.c -------------------------------------------------------------- */
void spur_draw(void);
void spur_draw_label(int x, int y, int w, int h, int t);
void spur_art_load(void);
int spur_art_check(void);
int spur_layout_audit(void);
void spur_draw_tile(int t, int x, int y, int mx, int my);
void spur_draw_person(int look, int pal, int dir, int frame, int x, int y);
void spur_draw_hero(int h, int dir, int frame, int x, int y, bool dead);
void spur_draw_hero_big(int h, int pose, int x, int y, bool dead); /* 32 x 32, facing right */
void spur_draw_horse(int x, int y, int frame, bool flying, int dir);
void spur_draw_wisp(int x, int y, int t);
void spur_draw_enemy(int e, int x, int y, int flash);
int spur_enemy_w(int e);
int spur_enemy_h(int e);

enum { /* people on the maps */
    LOOK_MAN, LOOK_WOMAN, LOOK_KID, LOOK_OLD, LOOK_SHERIFF, LOOK_PRIEST, LOOK_KEEPER, LOOK_DEMON, LOOK_ANGEL,
    LOOK_SOLDIER, LOOK_OUTLAW, LOOK_DOG, LOOK_HORSE, LOOK_ANIMAL, LOOK_BOSS, LOOK_GHOST, LOOK_NUN, LOOK_MINER,
    LOOK_BARKEEP, LOOK_COFFIN, LOOK_SIGNPOST, LOOK_COUNT
};

/* ---- spur_audio.c ------------------------------------------------------------------------- */
extern int SPUR_MUS_TITLE, SPUR_MUS_SALOON, SPUR_MUS_TOWN, SPUR_MUS_WORLD, SPUR_MUS_BATTLE, SPUR_MUS_BOSS,
    SPUR_MUS_CAVE, SPUR_MUS_DREAM, SPUR_MUS_HELL, SPUR_MUS_CITADEL, SPUR_MUS_CHURCH, SPUR_MUS_RIDE, SPUR_MUS_FLY,
    SPUR_MUS_WIN, SPUR_MUS_LOSE, SPUR_MUS_ENDING, SPUR_MUS_FINAL, SPUR_MUS_GHOST, SPUR_MUS_SHOP, SPUR_MUS_LEVEL;
void spur_audio_load(void);

/* ---- spur_bot.c ---------------------------------------------------------------------------- */
int spur_bot_buttons(void);
extern int spur_bot_debug;
extern int spur_bot_goal;     /* tests: stop after this story step (-1 none) */
int spur_bot_step(void);      /* which story step the bot is working on */
void spur_bot_pick(const int *h, int n); /* the strangers the bot carries out */

#endif
