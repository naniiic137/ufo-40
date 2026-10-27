/* HELLSPUR - the tables: the eight strangers, their skills and how they
 * learn them, Sage's animals, every item and price, the bestiary, the troops
 * and where they roam, the shops and what each town offers. The numbers
 * follow the original's curves (see the design doc); the names are ours. */
#include "spur.h"

/* ---- tiles ----------------------------------------------------------------- */
#define W TF_WALK
#define F TF_FLY
#define L TF_LAND
#define E TF_ENTER
#define S TF_SEARCH
const uint8_t SPUR_TILE_FLAGS[T_COUNT] = {
    [T_SEA] = F | TF_WATER, [T_RIVER] = F | TF_WATER, [T_GRASS] = W | F | L, [T_PRAIRIE] = W | F | L,
    [T_SAND] = W | F | L, [T_DIRT] = W | F | L, [T_SCRUB] = W | F | L, [T_FOREST] = W | F, [T_PINE] = W | F,
    [T_CACTUS] = W | F | L, [T_HILL] = W | F | L, [T_MOUNT] = F, [T_PEAK] = TF_HIGH, [T_SWAMP] = W | F | L,
    [T_ROAD] = W | F | L, [T_BRIDGE] = W | F, [T_FERRY] = F | S, [T_LAVA] = F, [T_ASH] = W | F | L,
    [T_BONES] = W | F | L, [T_CLOUD] = W | F | L, [T_MESA] = F, [T_SALT] = W | F | L, [T_DEADTREE] = W | F,
    [T_FLOWERS] = W | F | L,
    [T_TOWN] = W | F | E, [T_FORT] = W | F | E, [T_MISSION] = W | F | E, [T_HOUSE] = W | F | E,
    [T_CAVE] = W | F | E, [T_MINE] = W | F | E, [T_TOWER] = W | F | E, [T_GATE] = W | F | E,
    [T_SPIKE] = F | S, [T_SPIKE_GONE] = W | F | L, [T_CITADEL] = W | F | E, [T_SHRINE] = W | F | E,
    [T_SALOONSKY] = W | F | E, [T_SMITHY] = W | F | E, [T_RUIN] = W | F | E,
    [T_ANGELS] = F | S, [T_RUBBLE] = S, [T_FIREWALL] = S, [T_BARRIER] = TF_HIGH | S, [T_LOCKED] = S,
    [T_FLOOR] = W, [T_STONE] = W, [T_WALL] = 0, [T_ROCK] = 0, [T_ROOF] = 0, [T_BWALL] = 0, [T_DOOR] = W | E,
    [T_WINDOW] = 0, [T_COUNTER] = S, [T_FENCE] = 0, [T_WELL] = S, [T_BARREL] = S, [T_CRATE] = S, [T_UP] = W | E,
    [T_DOWN] = W | E, [T_EXIT] = W | E, [T_CHEST] = S, [T_BED] = S, [T_TABLE] = S, [T_RAIL] = W, [T_ORE] = S,
    [T_HERB] = W | S, [T_WATER] = 0, [T_BARS] = 0, [T_ALTAR] = S, [T_TORCH] = 0, [T_GRAVE] = S, [T_BURNING] = 0,
    [T_PEW] = 0, [T_RUG] = W, [T_BONEWALL] = 0, [T_DREAMFLOOR] = W, [T_HELLFLOOR] = W, [T_HELLWALL] = 0,
    [T_GLASS] = 0, [T_SIGN] = S, [T_BOOKS] = S, [T_PIANO] = S, [T_TRACKS] = W, [T_STOOL] = W,
    [T_TREE] = F, [T_HIVE] = F | S, [T_WINCH] = S, [T_LIFT] = W | E, [T_PORTAL] = W | E, [T_CAULDRON] = S,
    [T_SECRET] = W,
};
#undef W
#undef F
#undef L
#undef E
#undef S

/* ---- the eight strangers ------------------------------------------------------------ */
/*                       HP  SP ATK DEF SPD EVD RES VIT                                 */
const SpurHeroDef SPUR_HERO[H_COUNT] = {
    {"TUCKER", "COWHAND",
     {"A BROAD COWHAND WITH A BROKEN NOSE.", "HE HUMS TO THE CATTLE NOBODY ELSE CAN SEE."},
     {32, 4, 7, 4, 6, 4, 3, 8}, {70, 5, 20, 10, 8, 3, 6, 12}, 0, 0},
    {"SAGE", "SPIRITWALKER",
     {"A WOMAN WITH FEATHERS BRAIDED IN HER HAIR.", "THE SMOKE BENDS AWAY FROM HER, POLITELY."},
     {24, 14, 5, 3, 6, 6, 8, 7}, {50, 28, 14, 8, 9, 5, 14, 11}, 1, 1},
    {"HOLLIS", "SURGEON",
     {"HIS LEFT SLEEVE IS PINNED UP AT THE ELBOW.", "HE TAKES THE PULSE OF A MAN ALREADY GONE."},
     {24, 12, 4, 3, 5, 5, 9, 6}, {48, 22, 10, 8, 8, 4, 16, 9}, 2, 2},
    {"QUILL", "SCHOLAR",
     {"INK ON HIS CUFFS AND A SPLIT LIP HEALING.", "HE SKETCHES THE FLAMES AS THEY CLIMB."},
     {27, 9, 6, 4, 7, 6, 6, 7}, {58, 16, 16, 9, 10, 5, 10, 10}, 3, 3},
    {"VERA", "OUTLAW",
     {"A WHITE SCAR RUNS FROM HER THUMB TO HER WRIST.", "SHE IS SIZING UP EVERYONE'S POCKETS."},
     {25, 8, 7, 3, 4, 10, 5, 6}, {52, 14, 19, 8, 6, 8, 8, 9}, 4, 4},
    {"HATTIE", "HOMESTEADER",
     {"FLOUR TO THE ELBOWS AND A SHELL BELT ON.", "SHE IS COUNTING COINS UNDER HER BREATH."},
     {31, 8, 7, 4, 5, 3, 4, 8}, {66, 14, 19, 10, 7, 3, 7, 12}, 5, 5},
    {"CORA", "SHOWGIRL",
     {"SEQUINS ON HER SLEEVES CATCH THE FIRELIGHT.", "HER HEELS TAP OUT A TUNE NOBODY IS PLAYING."},
     {20, 13, 5, 2, 9, 10, 9, 5}, {40, 26, 13, 6, 12, 8, 14, 7}, 6, 6},
    {"BISCUIT", "HOUND",
     {"A SCRUFFY HOUND WITH A NOTCHED EAR, GROWLING", "AT THE FIRE AS IF IT OWED HIM SUPPER."},
     {18, 8, 9, 2, 10, 10, 8, 5}, {38, 6, 22, 7, 12, 7, 12, 8}, 7, 7},
};

const char *const SPUR_PASSIVE_NAME[PS_COUNT] = {"MELEE", "PISTOL", "RIFLE", "SHOTGUN", "HEAVY", "SPIRIT",
                                                 "DOCTOR", "SECOND WIND", "TWO GUNS", "ONE ARM", "DOG"};
const char *const SPUR_PASSIVE_DESC[PS_COUNT] = {
    "FISTS AND BLADES HIT HARDER.", "CAN USE PISTOLS. EACH LEVEL HITS HARDER.",
    "CAN USE RIFLES. EACH LEVEL HITS HARDER.", "CAN USE SHOTGUNS. EACH LEVEL HITS HARDER.",
    "CAN USE HEAVY GUNS. EACH LEVEL HITS HARDER.", "SKILLS AND BOWS WORK BETTER.",
    "MEDICINE GOES FURTHER: SOME ITEMS LEAVE HALF BEHIND.", "NOW AND THEN SHRUGS OFF A KILLING BLOW.",
    "CAN HOLD TWO OF THE SAME ONE-HANDED WEAPON.", "ONE HAND FREE, SO NOTHING THAT TAKES TWO.",
    "TAKES NO ORDERS. FIGHTS HIS OWN WAY.",
};

/* ---- skills (SP cost as the original's) ------------------------------------------------ */
#define B SKU_BATTLE
#define FD SKU_FIELD
const SpurSkillDef SPUR_SKILL[SK_COUNT] = {
    [SK_NONE] = {"", "", 0, 0, 0, 0},
    [SK_REEL] = {"REEL", "A JIG THAT PATCHES UP THE WHOLE POSSE A LITTLE.", 14, SKT_ALLALLY, B | FD, 26},
    [SK_SHIMMY] = {"SHIMMY", "THE WHOLE POSSE DODGES BETTER.", 20, SKT_ALLALLY, B, 8},
    [SK_MINUET] = {"MINUET", "A SLOW DANCE THAT MENDS THE WHOLE POSSE WELL.", 30, SKT_ALLALLY, B | FD, 70},
    [SK_CANCAN] = {"CANCAN", "THE WHOLE POSSE HITS HARDER.", 32, SKT_ALLALLY, B, 8},
    [SK_LIFT] = {"LIFT", "PICKS A FOE'S POCKET.", 4, SKT_ENEMY, B, 0},
    [SK_LIELOW] = {"LIE LOW", "HARDER TO HIT FOR THE REST OF THE FIGHT.", 16, SKT_SELF, B, 15},
    [SK_DEADSHOT] = {"DEAD SHOT", "MAY KILL A FOE OUTRIGHT.", 13, SKT_ENEMY, B, 40},
    [SK_RUMMAGE] = {"RUMMAGE", "SEARCH A GLINTING PATCH OF PLANTS.", 8, SKT_FIELD, FD, 0},
    [SK_PROSPECT] = {"PROSPECT", "DIG AT A GLINTING SEAM OF ROCK.", 24, SKT_FIELD, FD, 0},
    [SK_MEND] = {"MEND", "HEALS ONE. AWAY FROM A FIGHT, CAN WAKE THE FALLEN.", 4, SKT_ALLY, B | FD, 30},
    [SK_SUTURE] = {"SUTURE", "HEALS ONE A GOOD DEAL. CAN WAKE THE FALLEN.", 12, SKT_ALLY, B | FD, 90},
    [SK_PURGE] = {"PURGE", "DRAWS THE POISON OUT OF ONE.", 6, SKT_ALLY, B | FD, 0},
    [SK_SPLINT] = {"SPLINT", "ONE OF THE POSSE TAKES LESS DAMAGE.", 10, SKT_ALLY, B, 8},
    [SK_RAISE] = {"RAISE", "BRINGS ONE BACK FROM THE DEAD.", 30, SKT_DEADALLY, B | FD, 0},
    [SK_PAYOFF] = {"PAY OFF", "PAYS A FOE TO LEAVE. NO PAY, NO SPOILS.", 0, SKT_ENEMY, B, 0},
    [SK_COVER] = {"COVER", "TAKES THE BLOWS MEANT FOR THE OTHERS.", 8, SKT_SELF, B, 0},
    [SK_STEADY] = {"STEADY", "A STEADY HAND: THE RED OF THE BAR GROWS.", 10, SKT_SELF, B, 0},
    [SK_OBSERVE] = {"OBSERVE", "WATCHES A FOE TO LEARN ITS TRICK.", 2, SKT_ENEMY, B, 0},
    [SK_RAGE] = {"RAGE", "SWINGS ON HIS OWN FOR A WHILE, HARDER.", 6, SKT_SELF, B, 4},
    [SK_BRACE] = {"BRACE", "TAKES LESS DAMAGE.", 7, SKT_SELF, B, 8},
    [SK_SIPHON] = {"SIPHON", "DRINKS A FOE'S LIFE, PAST ITS ARMOUR.", 9, SKT_ENEMY, B, 40},
    [SK_JAB] = {"JAB", "A BLOW STRAIGHT PAST ARMOUR.", 9, SKT_ENEMY, B, 50},
    [SK_UPPERCUT] = {"UPPERCUT", "KNOCKS A FOE SILLY.", 14, SKT_ENEMY, B, 0},
    [SK_HAYMAKER] = {"HAYMAKER", "A WILD SWING AT EVERY FOE.", 15, SKT_ALLENEMY, B, 100},
    [SK_KEENEYE] = {"KEEN EYE", "ONE OF THE POSSE GETS A WIDER RED.", 5, SKT_ALLY, B, 0},
    [SK_BITE] = {"BITE", "BITS STRAIGHT THROUGH ARMOUR.", 8, SKT_ENEMY, B, 60},
    [SK_DUSTSTORM] = {"DUST STORM", "GRIT IN EVERY FOE'S EYES, PAST ARMOUR.", 8, SKT_ALLENEMY, B, 30},
    [SK_HOWL] = {"YIP", "ONE OF THE POSSE HITS HARDER.", 10, SKT_ALLY, B, 8},
    [SK_TROT] = {"TROT", "THE WHOLE POSSE MOVES QUICKER.", 10, SKT_ALLALLY, B, 4},
    [SK_RENEW] = {"RENEW", "FREES STIFF LIMBS AND HEALS THE POSSE.", 10, SKT_ALLALLY, B | FD, 30},
    [SK_BLENDIN] = {"BLEND IN", "FEWER WISPS COME FOR A WHILE.", 12, SKT_FIELD, FD, 0},
    [SK_SNARL] = {"SNARL", "ONE FOE HITS SOFTER.", 15, SKT_ENEMY, B, 0},
    [SK_FLUTTER] = {"FLUTTER", "ONE OF THE POSSE DODGES BETTER.", 15, SKT_ALLY, B, 10},
    [SK_BUCK] = {"BUCK", "A HARD KICK AT ONE FOE.", 18, SKT_ENEMY, B, 220},
    [SK_SNARE] = {"SNARE", "TIES A FOE UP FOR A WHILE.", 18, SKT_ENEMY, B, 0},
    [SK_SWOOP] = {"SWOOP", "TALONS ACROSS EVERY FOE.", 28, SKT_ALLENEMY, B, 200},
    [SK_MAUL] = {"MAUL", "TEARS ONE FOE APART, PAST ARMOUR.", 50, SKT_ENEMY, B, 600},
    [SK_SHELL] = {"", "", 0, 0, 0, 0},
};
#undef B
#undef FD

#define P(lv, ps) {lv, ps, 0}
#define K(lv, sk) {lv, -1, sk}
const SpurLearn SPUR_LEARN[H_COUNT][SPUR_MAX_LEARN] = {
    /* TUCKER */ {P(1, PS_PISTOL), P(1, PS_SHOTGUN), P(1, PS_RIFLE), P(5, PS_PISTOL), P(10, PS_RIFLE), P(16, PS_HEAVY),
                  P(23, PS_SHOTGUN), P(31, PS_RIFLE), P(40, PS_HEAVY), K(50, SK_BRACE)},
    /* SAGE */ {P(1, PS_MELEE), P(5, PS_PISTOL), P(12, PS_SPIRIT), P(19, PS_SPIRIT), P(26, PS_MELEE), P(33, PS_SPIRIT),
                P(40, PS_SPIRIT), P(50, PS_SPIRIT)},
    /* HOLLIS */ {P(1, PS_ONEARM), P(1, PS_PISTOL), P(1, PS_DOC), K(4, SK_SPLINT), P(8, PS_DOC), P(12, PS_PISTOL),
                  K(16, SK_MEND), P(20, PS_DOC), K(26, SK_PURGE), K(32, SK_SUTURE), P(38, PS_PISTOL), P(44, PS_SPIRIT),
                  P(50, PS_DOC)},
    /* QUILL */ {P(1, PS_MELEE), P(1, PS_PISTOL), K(3, SK_OBSERVE), P(9, PS_SPIRIT), P(16, PS_MELEE), P(24, PS_SPIRIT),
                 P(33, PS_DUAL), P(43, PS_MELEE), P(50, PS_SPIRIT)},
    /* VERA */ {P(1, PS_PISTOL), P(1, PS_RIFLE), K(2, SK_LIFT), P(10, PS_RIFLE), K(18, SK_LIELOW), P(26, PS_RIFLE),
                K(34, SK_DEADSHOT), P(42, PS_RIFLE), P(50, PS_RIFLE)},
    /* HATTIE */ {P(1, PS_PISTOL), P(1, PS_SHOTGUN), P(6, PS_PISTOL), K(10, SK_PAYOFF), P(12, PS_HEAVY),
                  P(18, PS_SHOTGUN), P(24, PS_SHOTGUN), P(30, PS_HEAVY), P(36, PS_HEAVY), P(42, PS_SHOTGUN),
                  P(50, PS_HEAVY)},
    /* CORA */ {P(1, PS_PISTOL), P(1, PS_DUAL), K(5, SK_REEL), P(8, PS_SPIRIT), K(12, SK_SHIMMY), P(18, PS_DOC),
                K(24, SK_MINUET), P(30, PS_PISTOL), P(36, PS_SPIRIT), P(42, PS_DOC), P(50, PS_PISTOL)},
    /* BISCUIT */ {P(1, PS_DOG), P(1, PS_MELEE), K(3, SK_RUMMAGE), P(5, PS_MELEE), P(10, PS_SURVIVOR), P(15, PS_MELEE),
                   P(20, PS_MELEE), P(30, PS_SURVIVOR), P(40, PS_SURVIVOR), P(50, PS_MELEE)},
};
#undef P
#undef K

/* Sage's ten animals, each with a choice of two. Where they wait is ours. */
const SpurAnimal SPUR_ANIMAL[SPUR_ANIMALS] = {
    {"JACKRABBIT", SK_MEND, SK_RUMMAGE, M_WORLD, 0, 0},
    {"OWL", SK_FLUTTER, SK_SIPHON, M_DRYWASH, 0, 0},
    {"KIT FOX", SK_HOWL, SK_BITE, M_WORLD, 0, 0},
    {"ROADRUNNER", SK_KEENEYE, SK_DUSTSTORM, M_NOTCH, 0, 0},
    {"ARMADILLO", SK_SPLINT, SK_SNARL, M_WORLD, 0, 0},
    {"BURRO", SK_TROT, SK_BUCK, M_DOLORES, 0, 0},
    {"HORNED TOAD", SK_RENEW, SK_BLENDIN, M_ROOSTMINE, 0, 0},
    {"SCORPION", SK_PURGE, SK_SNARE, M_SNAKE1, 0, 0},
    {"COUGAR", SK_SUTURE, SK_MAUL, M_WORLD, 0, 0},
    {"CONDOR", SK_RAISE, SK_SWOOP, M_WORLD, 0, 0},
};

/* ---- items --------------------------------------------------------------------------- */
#define U(n, pr, pw, use, half, hd, fl) {n, IK_USE, WC_NONE, fl, pr, pw, 0, use, half, hd, 0}
#define WPN(n, wc, pr, atk, hits, fl) {n, IK_WEAPON, wc, fl, pr, atk, hits, IU_NONE, -1, 0, 0}
#define NK(n, pr, evd, res, fl) {n, IK_NECK, WC_NONE, fl, pr, evd, 0, IU_NONE, -1, 0, res}
#define BD(n, pr, def, fl) {n, IK_BODY, WC_NONE, fl, pr, def, 0, IU_NONE, -1, 0, 0}
#define FT(n, pr, spd, fl) {n, IK_FEET, WC_NONE, fl, pr, spd, 0, IU_NONE, -1, 0, 0}
#define ORE(n, sell) {n, IK_ORE, WC_NONE, 0, sell * 2, 0, 0, IU_NONE, -1, 0, 0}
#define BI (IF_BATTLE)
#define FI (IF_FIELD)
#define TH IF_TWOHAND
const SpurItemDef SPUR_ITEM[IT_COUNT] = {
    [IT_NONE] = {"", 0, 0, 0, 0, 0, 0, 0, -1, 0, 0},
    [IT_BANDAGE] = U("BANDAGE", 40, 30, IU_HEAL, IT_HALFBANDAGE, 1, BI | FI),
    [IT_HALFBANDAGE] = U("HALF BANDAGE", 0, 30, IU_HEAL, -1, 0, BI | FI),
    [IT_HERB] = U("HERB", 20, 50, IU_HEAL, IT_HALFHERB, 1, BI | FI),
    [IT_HALFHERB] = U("HALF HERB", 0, 50, IU_HEAL, -1, 0, BI | FI),
    [IT_SALVE] = U("SALVE", 120, 120, IU_HEAL, IT_HALFSALVE, 2, BI | FI),
    [IT_HALFSALVE] = U("HALF SALVE", 0, 120, IU_HEAL, -1, 0, BI | FI),
    [IT_MEDCHEST] = U("MED CHEST", 600, 450, IU_HEAL, IT_HALFCHEST, 3, BI | FI),
    [IT_HALFCHEST] = U("HALF CHEST", 0, 450, IU_HEAL, IT_SALVE, 3, BI | FI),
    [IT_SNAKEROOT] = U("SNAKEROOT", 60, 0, IU_CURE, IT_HALFROOT, 2, BI | FI),
    [IT_HALFROOT] = U("HALF ROOT", 0, 0, IU_CURE, -1, 0, BI | FI),
    [IT_MINT] = U("MINT", 30, 20, IU_SP, -1, 0, BI | FI),
    [IT_GINGERPOP] = U("GINGER POP", 120, 40, IU_SP, -1, 0, BI | FI),
    [IT_CACTUSFIG] = U("CACTUS FIG", 60, 30, IU_SP, -1, 0, BI | FI),
    [IT_BLACKBREW] = U("BLACK BREW", 1000, 999, IU_SPALL, -1, 0, BI | FI),
    [IT_HOLYOIL] = U("HOLY OIL", 1200, 1, IU_REVIVE, -1, 0, BI | FI),
    [IT_CAMPKIT] = U("CAMP KIT", 200, 50, IU_CAMP, -1, 0, FI),
    [IT_LARIAT] = U("LARIAT", 60, 0, IU_STUN, -1, 0, BI),
    [IT_HALOPIN] = U("HALO PIN", 500, 0, IU_WARP, -1, 0, FI),
    [IT_SMUDGE] = U("SMUDGE", 200, 250, IU_WARD, -1, 0, FI),
    [IT_BEEFBONE] = U("BEEF BONE", 50000, 0, IU_TREAT_D, -1, 0, FI),
    [IT_SOUPBONE] = U("SOUP BONE", 50000, 0, IU_TREAT_A, -1, 0, FI),
    [IT_WISHBONE] = U("WISHBONE", 50000, 0, IU_TREAT_E, -1, 0, FI),
    [IT_STEADYBOOK] = U("STEADY BOOK", 5000, 0, IU_LESSON_STEADY, -1, 0, FI),
    [IT_PROSPECTBOOK] = U("DIGGER BOOK", 8000, 0, IU_LESSON_PROSPECT, -1, 0, FI),
    /* pistols: ATK, hits a turn */
    [IT_POCKET] = WPN("POCKET GUN", WC_PISTOL, 50, 6, 1, 0),
    [IT_DUCKFOOT] = WPN("DUCKFOOT", WC_PISTOL, 800, 8, 2, 0),
    [IT_BUNTLINE] = WPN("BUNTLINE", WC_PISTOL, 8000, 16, 2, 0),
    [IT_SILVERBELLE] = WPN("SILVERBELLE", WC_PISTOL, 0, 24, 2, IF_UNIQUE),
    [IT_LAWMAN] = WPN("LAWMAN", WC_PISTOL, 30000, 38, 3, 0),
    [IT_DEVILSOWN] = WPN("DEVIL'S OWN", WC_PISTOL, 0, 66, 3, IF_UNIQUE),
    [IT_SCATTER] = WPN("SCATTERGUN", WC_SHOTGUN, 400, 18, 1, TH),
    [IT_QUAD] = WPN("QUAD BARREL", WC_SHOTGUN, 6000, 30, 1, TH),
    [IT_BIGBORE] = WPN("BIG BORE", WC_SHOTGUN, 35000, 68, 1, TH),
    [IT_ELEPHANT] = WPN("ELEPHANT", WC_SHOTGUN, 45000, 88, 1, TH),
    [IT_LONGTOM] = WPN("LONG TOM", WC_RIFLE, 1200, 20, 1, TH),
    [IT_CAVALRY] = WPN("CAVALRY", WC_RIFLE, 12000, 40, 1, TH),
    [IT_PLAINSMAN] = WPN("PLAINSMAN", WC_RIFLE, 24000, 64, 1, TH),
    [IT_NEEDLEGUN] = WPN("NEEDLE GUN", WC_RIFLE, 0, 111, 1, TH | IF_RELOAD | IF_UNIQUE),
    [IT_DOOMSDAY] = WPN("DOOMSDAY", WC_RIFLE, 0, 200, 1, TH | IF_UNIQUE),
    [IT_CRANKGUN] = WPN("CRANK GUN", WC_HEAVY, 10000, 37, 8, TH | IF_RELOAD),
    [IT_GRINDER] = WPN("GRINDER", WC_HEAVY, 20000, 51, 8, TH | IF_RELOAD),
    [IT_THRESHER] = WPN("THRESHER", WC_HEAVY, 0, 76, 8, TH | IF_RELOAD | IF_UNIQUE),
    [IT_HELLFIRE] = WPN("HELLFIRE", WC_HEAVY, 80000, 100, 8, TH | IF_RELOAD),
    [IT_CLEAVER] = WPN("CLEAVER", WC_MELEE, 160, 8, 1, 0),
    [IT_BOWIE] = WPN("BOWIE", WC_MELEE, 2000, 14, 1, 0),
    [IT_BLACKGLASS] = WPN("BLACKGLASS", WC_MELEE, 16000, 49, 1, 0),
    [IT_WIDOWMAKER] = WPN("WIDOWMAKER", WC_MELEE, 50000, 72, 1, 0),
    [IT_SAPLING] = WPN("SAPLING BOW", WC_BOW, 0, 12, 1, TH | IF_UNIQUE),
    [IT_HORNBOW] = WPN("HORN BOW", WC_BOW, 0, 52, 1, TH | IF_UNIQUE),
    [IT_THUNDERBOW] = WPN("THUNDER BOW", WC_BOW, 0, 80, 1, TH | IF_UNIQUE),
    /* necks: EVD, RES */
    [IT_KERCHIEF] = NK("KERCHIEF", 30, 2, 0, 0),
    [IT_BOLOTIE] = NK("BOLO TIE", 100, 4, 0, 0),
    [IT_LUCKYCOIN] = NK("LUCKY COIN", 600, 8, 0, 0),
    [IT_TOOTHNECK] = NK("TOOTH CHAIN", 16000, 12, 0, 0),
    [IT_STARSAPPHIRE] = NK("STAR STONE", 0, 20, 0, IF_UNIQUE),
    [IT_WARD] = NK("WARD", 0, 6, 30, IF_UNIQUE),
    [IT_BEADS] = NK("SPIRIT BEADS", 0, 10, 0, IF_UNIQUE | IF_SPIRIT),
    [IT_STEADYEYE] = NK("STEADY EYE", 0, 4, 0, IF_UNIQUE | IF_STEADY),
    /* bodies: DEF */
    [IT_COTTON] = BD("LINEN", 20, 1, IF_LIGHT),
    [IT_WOOL] = BD("FLANNEL", 40, 2, IF_LIGHT),
    [IT_BUCKSKIN] = BD("BUCKSKIN", 1600, 8, IF_LIGHT),
    [IT_SILK] = BD("CHIFFON", 6000, 16, IF_LIGHT),
    [IT_SATIN] = BD("SATIN", 0, 24, IF_LIGHT | IF_UNIQUE),
    [IT_VEST] = BD("VEST", 30, 4, 0),
    [IT_WOOLCOAT] = BD("WOOL COAT", 150, 6, 0),
    [IT_LEATHER] = BD("CHAPS", 1300, 12, 0),
    [IT_SERAPE] = BD("SERAPE", 8000, 20, 0),
    [IT_DUSTER] = BD("LONGCOAT", 24000, 32, 0),
    [IT_HELLHIDE] = BD("HELLHIDE", 60000, 48, IF_TUCKER),
    /* feet: SPD */
    [IT_WORNBOOTS] = FT("OLD BOOTS", 25, 2, 0),
    [IT_RIDING] = FT("RIDING", 90, 4, 0),
    [IT_SNAKESKIN] = FT("SNAKESKIN", 200, 8, 0),
    [IT_OSTRICH] = FT("OSTRICH", 900, 10, 0),
    [IT_GATOR] = FT("SWAMP BOOTS", 0, 12, IF_UNIQUE),
    [IT_MOCCASIN] = FT("BEADED SOLES", 0, 16, IF_UNIQUE),
    /* ores: sold for */
    [IT_IRONORE] = ORE("IRON ORE", 100), [IT_SILVER] = ORE("SILVER", 200), [IT_GOLD] = ORE("GOLD", 400),
    [IT_TOPAZ] = ORE("TOPAZ", 600), [IT_EMERALD] = ORE("EMERALD", 1200), [IT_RUBY] = ORE("RUBY", 3000),
    [IT_DIAMOND] = ORE("DIAMOND", 10000), [IT_HELLITE] = ORE("HELLITE", 50000),
};
#undef U
#undef WPN
#undef NK
#undef BD
#undef FT
#undef ORE
#undef BI
#undef FI
#undef TH

const char *const SPUR_KEYITEM_NAME[KI_COUNT] = {"FERRY CRANK", "HONEYCOMB", "LETTER", "SILVER BELL", "CELLAR KEY",
                                                 "PLUNGER", "STORM FEATHER", "FIRE CHARM", "IRON SPUR", "CLAW BAR",
                                                 "NIGHTMARES"};
const char *const SPUR_KEYITEM_DESC[KI_COUNT] = {
    "THE HANDLE OFF THE MULE CROSSING FERRY.", "DRIPPING WILD HONEY. SOMEONE HAS A SWEET TOOTH.",
    "A BANK LETTER, SEALED, FOR A HATTIE.", "IT RINGS TRUE. FALSE THINGS HATE IT.",
    "A BIG IRON KEY FROM UNDER THE MISSION.", "THE PLUNGER FOR A CHARGE OF DYNAMITE.",
    "A BLUE FEATHER THAT CRACKLES.", "WARM TO HOLD. FIRE WILL LET YOU PASS.",
    "IT FITS THE HEEL OF THE BLACK GATE.", "FOR PULLING VERY BIG NAILS.",
    "FOUR BLACK HORSES WITH WINGS. MOUNT FROM THE MENU.",
};

/* ---- the bestiary ------------------------------------------------------------------------ */
#define ACT(k, w, p) {k, w, p}
#define HIT ACT(EA_HIT, 10, 0)
const SpurEnemyDef SPUR_ENEMY[] = {
    {"", 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, {HIT}, 0, -1, -1, 0},
    /* the Sweetgrass */
    {"DUST RAT", A_RAT, 0, 0, 12, 6, 0, 5, 5, 0, 4, 4, {HIT}, 0, -1, -1, 0},                                   /* 1 */
    {"THORNBALL", A_WEED, 0, 0, 18, 5, 1, 8, 10, 0, 6, 3, {HIT}, 0, -1, -1, 0},
    {"BUZZARD", A_BIRD, 0, 0, 27, 8, 1, 9, 15, 2, 8, 6, {HIT}, 0, -1, -1, EF_FLIER},
    {"FOOTPAD", A_BANDIT, 0, 0, 34, 9, 2, 7, 8, 2, 10, 12, {HIT, ACT(EA_STEAL, 3, 5)}, SK_LIFT, IT_BANDAGE, IT_WORNBOOTS, 0},
    {"PUCK", A_PUCK, 0, 0, 40, 10, 3, 8, 10, 4, 20, 13, {HIT}, SK_PROSPECT, -1, IT_IRONORE, 0},
    {"FLITTER", A_BAT, 0, 0, 25, 7, 0, 12, 20, 2, 12, 4, {HIT}, SK_SIPHON, -1, -1, EF_FLIER},
    {"DIGGER", A_DIGGER, 0, 0, 89, 14, 6, 6, 5, 4, 70, 23, {HIT, ACT(EA_MULTI, 3, 2)}, SK_HAYMAKER, IT_IRONORE, IT_SILVER, 0},
    {"MILDEW", A_MOLD, 0, 0, 10, 4, 0, 2, 0, 0, 10, 0, {HIT, ACT(EA_POISON, 4, 0)}, 0, -1, -1, 0},
    {"MANYLEGS", A_CENTI, 0, 1, 160, 18, 8, 8, 5, 6, 320, 189, {HIT, ACT(EA_MULTI, 4, 2), ACT(EA_POISON, 3, 0)}, SK_JAB, -1, -1, 0},
    /* the Bottoms */
    {"SCRUB DOG", A_DOG, 0, 0, 30, 12, 2, 12, 15, 3, 35, 21, {HIT}, 0, -1, -1, 0},                               /* 10 */
    {"RABID CUR", A_DOG, 1, 0, 89, 16, 3, 14, 15, 4, 103, 21, {HIT, ACT(EA_POISON, 4, 0)}, 0, -1, -1, 0},
    {"ROAD AGENT", A_BANDIT, 1, 0, 45, 14, 4, 9, 10, 4, 80, 66, {HIT, ACT(EA_MULTI, 2, 2)}, SK_LIFT, IT_BANDAGE, IT_DUCKFOOT, 0},
    {"MUDLUMP", A_MOLD, 1, 0, 49, 12, 2, 3, 0, 2, 44, 2, {HIT}, 0, -1, -1, 0},
    {"FUZZBACK", A_SPIDER, 0, 0, 98, 15, 5, 10, 10, 4, 77, 37, {HIT, ACT(EA_POISON, 5, 0)}, 0, -1, -1, 0},
    {"BUZZTAIL", A_SNAKE, 0, 0, 90, 18, 4, 13, 15, 5, 187, 187, {HIT, ACT(EA_POISON, 5, 0)}, SK_JAB, IT_SNAKEROOT, IT_SNAKESKIN, 0},
    {"IMP", A_PUCK, 1, 0, 57, 15, 4, 11, 15, 6, 49, 49, {HIT, ACT(EA_FIRE, 3, 12)}, SK_RAGE, -1, -1, 0},
    {"SHAMBLER", A_GHOUL, 0, 0, 60, 16, 6, 4, 0, 6, 92, 56, {HIT}, SK_BRACE, -1, -1, EF_UNDEAD},
    {"EMBER HOUND", A_DOG, 2, 0, 337, 22, 6, 14, 15, 8, 366, 66, {HIT, ACT(EA_FIRE, 4, 20)}, SK_HAYMAKER, -1, -1, 0},
    {"TURNKEY", A_TROOPER, 0, 1, 155, 22, 10, 7, 5, 8, 186, 250, {HIT, ACT(EA_PARALYZE, 3, 0)}, SK_UPPERCUT, -1, -1, 0},
    /* the High Mesa */
    {"CHUPACABRA", A_GOAT, 0, 0, 114, 26, 8, 12, 10, 8, 192, 192, {HIT, ACT(EA_DRAIN, 4, 20)}, 0, -1, -1, 0}, /* 20 */
    {"MUSK APE", A_APE, 0, 1, 293, 34, 10, 7, 5, 8, 308, 120, {HIT, ACT(EA_WEAKEN, 2, 0)}, 0, -1, -1, 0},
    {"HIGH TALON", A_BIRD, 1, 0, 61, 24, 4, 16, 25, 6, 78, 34, {HIT}, 0, -1, -1, EF_FLIER},
    {"ANTLER HARE", A_HARE, 0, 0, 92, 22, 6, 18, 30, 8, 100, 100, {HIT, ACT(EA_MULTI, 3, 2)}, 0, -1, -1, 0},
    {"SAND SPOUT", A_WEED, 1, 0, 50, 20, 4, 16, 30, 6, 73, 10, {HIT}, 0, -1, -1, 0},
    {"TWISTER", A_WEED, 2, 1, 240, 44, 10, 18, 30, 10, 290, 36, {HIT, ACT(EA_ALL, 4, 0)}, 0, -1, -1, 0},
    {"NIGHT RIDER", A_RIDER, 0, 1, 195, 39, 14, 14, 10, 10, 410, 310, {HIT, ACT(EA_MULTI, 4, 2)}, 0, -1, -1, 0},
    {"JITTERBEAN", A_BEAN, 0, 0, 30, 9, 250, 40, 60, 99, 12000, 0, {ACT(EA_IDLE, 4, 0), ACT(EA_FLEE, 6, 0)}, SK_LIELOW, -1, -1, EF_NOSTUN},
    {"BUFFALO MAN", A_BISON, 0, 1, 800, 44, 20, 9, 5, 12, 1800, 1800, {HIT, ACT(EA_ALL, 3, 0)}, SK_UPPERCUT, -1, -1, 0},
    {"DESPERADO", A_BANDIT, 2, 0, 105, 28, 10, 12, 15, 8, 300, 199, {HIT, ACT(EA_MULTI, 3, 2)}, SK_LIFT, IT_SALVE, IT_BUNTLINE, 0},
    {"NECK BITER", A_BAT, 1, 0, 90, 23, 6, 16, 25, 8, 201, 44, {HIT, ACT(EA_DRAIN, 5, 20)}, SK_SIPHON, -1, -1, EF_FLIER},  /* 30 */
    {"POX RAT", A_RAT, 1, 0, 73, 21, 6, 12, 10, 6, 130, 98, {HIT, ACT(EA_POISON, 5, 0)}, 0, -1, -1, 0},
    {"ROCK FIST", A_GOLEM, 0, 1, 118, 26, 40, 4, 0, 10, 400, 0, {HIT}, SK_BRACE, -1, -1, EF_STONE},
    {"HORNED IMP", A_PUCK, 2, 0, 105, 26, 10, 12, 15, 10, 164, 164, {HIT, ACT(EA_FIRE, 4, 24)}, SK_RAGE, -1, -1, 0},
    {"BLACK DOG", A_DOG, 3, 1, 390, 42, 16, 16, 15, 12, 1808, 601, {HIT, ACT(EA_MULTI, 3, 2)}, 0, -1, -1, 0},
    {"SWAMP HAG", A_HAG, 0, 1, 300, 35, 12, 9, 10, 14, 510, 1600, {HIT, ACT(EA_PARALYZE, 3, 0), ACT(EA_DRAIN, 3, 30)}, 0, -1, -1, 0},
    {"LAMP MOTH", A_MOTH, 0, 1, 225, 32, 8, 13, 20, 10, 700, 20, {HIT, ACT(EA_PARALYZE, 4, 0)}, 0, -1, -1, EF_FLIER},
    {"BONE CRAWLER", A_CENTI, 1, 1, 525, 49, 20, 10, 5, 14, 3200, 189, {HIT, ACT(EA_MULTI, 4, 3)}, 0, -1, -1, EF_UNDEAD},
    {"WITCH OWL", A_OWL, 0, 0, 210, 35, 10, 15, 25, 14, 600, 124, {HIT, ACT(EA_PARALYZE, 3, 0)}, 0, -1, -1, EF_FLIER},
    {"GRAVE SLAB", A_GOLEM, 1, 1, 450, 44, 60, 4, 0, 16, 1981, 0, {HIT, ACT(EA_ALL, 2, 0)}, SK_BRACE, -1, -1, EF_STONE},
    {"BOG VIPER", A_SNAKE, 1, 0, 300, 46, 14, 14, 15, 12, 1136, 900, {HIT, ACT(EA_POISON, 5, 0)}, 0, IT_SNAKEROOT, -1, 0}, /* 40 */
    {"TOADSTOOL", A_MOLD, 2, 0, 112, 26, 6, 6, 0, 10, 300, 0, {HIT, ACT(EA_POISONALL, 3, 0), ACT(EA_PARALYZE, 2, 0)}, 0, -1, -1, 0},
    {"BLOODPOOL", A_MOLD, 3, 1, 300, 39, 20, 5, 0, 14, 400, 0, {HIT, ACT(EA_DRAIN, 3, 40)}, 0, -1, -1, 0},
    /* the Burnt Hills */
    {"MORTAR IMP", A_CANNON, 0, 1, 525, 62, 30, 6, 0, 16, 2780, 490, {HIT, ACT(EA_FIRE, 6, 60)}, 0, -1, -1, 0},
    {"ASH RIDER", A_RIDER, 1, 1, 450, 56, 22, 16, 10, 16, 915, 605, {HIT, ACT(EA_MULTI, 4, 2)}, 0, -1, -1, 0},
    {"QUICKDRAW", A_BANDIT, 3, 0, 337, 55, 18, 20, 20, 14, 544, 362, {HIT, ACT(EA_MULTI, 5, 3)}, SK_LIFT, IT_SALVE, IT_LAWMAN, 0},
    {"GIBBET", A_GHOUL, 1, 0, 375, 51, 18, 10, 10, 16, 440, 800, {HIT, ACT(EA_DEATH, 2, 0)}, SK_UPPERCUT, -1, -1, EF_UNDEAD},
    {"GATOR", A_GATOR, 0, 1, 675, 62, 26, 8, 5, 16, 2808, 2400, {HIT, ACT(EA_MULTI, 3, 2)}, 0, IT_GATOR, IT_GATOR, 0},
    {"GAWKER", A_EYE, 0, 0, 375, 44, 18, 12, 15, 18, 600, 1100, {HIT, ACT(EA_PARALYZE, 4, 0)}, 0, -1, -1, EF_FLIER},
    /* the Dreaming */
    {"RAGWING", A_BIRD, 2, 0, 225, 44, 12, 18, 25, 16, 500, 201, {HIT, ACT(EA_MULTI, 3, 2)}, 0, -1, -1, EF_FLIER},
    {"WILD PUCK", A_PUCK, 3, 0, 337, 46, 16, 14, 20, 18, 598, 800, {HIT, ACT(EA_FIRE, 4, 40)}, SK_PROSPECT, -1, IT_GOLD, 0}, /* 50 */
    {"GLOOM", A_GHOST, 0, 0, 262, 49, 10, 16, 40, 20, 606, 0, {HIT, ACT(EA_DRAIN, 3, 40)}, SK_LIELOW, -1, -1, EF_UNDEAD},
    {"FAMILIAR", A_HARE, 1, 0, 285, 48, 14, 20, 30, 20, 650, 300, {HIT, ACT(EA_PARALYZE, 2, 0)}, 0, -1, -1, 0},
    {"MASK DANCER", A_MONK, 0, 0, 525, 58, 20, 18, 25, 20, 1736, 900, {HIT, ACT(EA_MULTI, 4, 2)}, SK_RAGE, -1, -1, 0},
    {"BISON LORD", A_BISON, 1, 1, 1200, 70, 30, 10, 5, 20, 2800, 2800, {HIT, ACT(EA_ALL, 3, 0)}, 0, -1, -1, 0},
    {"MOON HARE", A_HARE, 2, 0, 1500, 79, 40, 30, 60, 30, 24000, 0, {HIT, ACT(EA_FLEE, 3, 0)}, 0, -1, -1, 0},
    /* Hell and the Bone Tower */
    {"CINDERKIN", A_TROOPER, 1, 1, 1050, 84, 36, 14, 10, 24, 3996, 2664, {HIT, ACT(EA_FIRE, 4, 80)}, 0, -1, -1, 0},
    {"OUTCAST", A_TROOPER, 2, 0, 750, 77, 30, 16, 15, 24, 999, 1666, {HIT, ACT(EA_MULTI, 3, 2)}, 0, -1, -1, 0},
    {"TEMPTER", A_MONK, 1, 0, 600, 70, 26, 18, 25, 24, 701, 701, {HIT, ACT(EA_DRAIN, 4, 60), ACT(EA_WEAKEN, 2, 0)}, 0, -1, -1, 0},
    {"DREAD EYE", A_EYE, 1, 0, 562, 72, 24, 14, 15, 24, 651, 500, {HIT, ACT(EA_PARALYZE, 4, 0)}, 0, -1, -1, EF_FLIER},
    {"PLAGUE MAN", A_GHOUL, 2, 0, 675, 67, 24, 10, 5, 24, 971, 890, {HIT, ACT(EA_POISONALL, 4, 0)}, 0, -1, -1, EF_UNDEAD}, /* 60 */
    {"SOOT GALE", A_WEED, 3, 1, 525, 70, 20, 22, 40, 24, 1578, 160, {HIT, ACT(EA_POISONALL, 3, 0), ACT(EA_ALL, 3, 0)}, 0, -1, -1, 0},
    {"DEMON KNIGHT", A_RIDER, 2, 1, 975, 88, 40, 16, 10, 26, 1230, 1100, {HIT, ACT(EA_MULTI, 4, 2)}, SK_UPPERCUT, -1, -1, 0},
    {"DEMON ELITE", A_TROOPER, 3, 0, 825, 84, 38, 16, 15, 26, 801, 700, {HIT, ACT(EA_MULTI, 3, 2)}, 0, -1, -1, 0},
    {"SPIKE HOUND", A_DOG, 4, 0, 675, 81, 26, 22, 20, 24, 960, 0, {HIT, ACT(EA_MULTI, 4, 2)}, 0, -1, IT_SOUPBONE, 0},
    {"HELL JAY", A_BIRD, 3, 0, 525, 74, 20, 22, 30, 24, 599, 238, {HIT, ACT(EA_MULTI, 3, 2)}, 0, -1, -1, EF_FLIER},
    {"SKULL MOTH", A_MOTH, 1, 1, 675, 76, 20, 16, 25, 26, 1000, 233, {HIT, ACT(EA_PARALYZE, 4, 0)}, 0, -1, -1, EF_FLIER},
    {"GRIT WHIRL", A_WEED, 4, 0, 375, 62, 16, 18, 30, 24, 504, 10, {HIT, ACT(EA_ALL, 3, 0)}, 0, -1, -1, 0},
    {"LONGLEG", A_SPIDER, 1, 1, 750, 79, 26, 14, 15, 26, 1101, 400, {HIT, ACT(EA_POISON, 5, 0), ACT(EA_PARALYZE, 2, 0)}, 0, -1, -1, 0},
    {"HEADLESS", A_GHOUL, 3, 0, 675, 81, 26, 12, 10, 26, 651, 500, {HIT, ACT(EA_MULTI, 3, 2)}, 0, -1, -1, EF_UNDEAD},
    {"DRIFTER", A_GHOST, 1, 0, 750, 84, 20, 18, 35, 28, 1156, 300, {HIT, ACT(EA_DRAIN, 4, 80)}, 0, -1, -1, EF_UNDEAD}, /* 70 */
    {"PALE THING", A_GHOST, 2, 1, 900, 88, 24, 18, 35, 30, 1313, 0, {HIT, ACT(EA_DEATH, 2, 0)}, 0, -1, -1, EF_UNDEAD},
    /* the citadel of Hellspur */
    {"HAUNT", A_GHOST, 3, 0, 1050, 97, 26, 20, 40, 30, 1560, 0, {HIT, ACT(EA_MULTI, 5, 3)}, SK_LIELOW, -1, -1, EF_UNDEAD},
    {"BLACK ADDER", A_SNAKE, 2, 0, 1200, 104, 32, 20, 20, 30, 2900, 1280, {HIT, ACT(EA_POISON, 4, 0)}, 0, -1, -1, 0},
    {"CUTTHROAT", A_MONK, 2, 0, 975, 99, 30, 24, 30, 32, 2020, 1010, {HIT, ACT(EA_DEATH, 4, 0)}, 0, -1, -1, 0},
    {"ASH PRIOR", A_MONK, 3, 1, 1800, 106, 40, 18, 20, 34, 6100, 1800, {HIT, ACT(EA_ALL, 3, 0), ACT(EA_HEALALL, 2, 400)}, 0, -1, -1, 0},
    {"RAVAGER", A_APE, 1, 1, 1950, 123, 44, 14, 10, 32, 5200, 5201, {HIT, ACT(EA_MULTI, 3, 2)}, 0, -1, -1, 0},
    {"GRAVE BULL", A_BISON, 2, 1, 2700, 132, 50, 12, 5, 36, 10000, 10000, {HIT, ACT(EA_ALL, 3, 0), ACT(EA_DEATH, 1, 0)}, SK_DEADSHOT, IT_DIAMOND, IT_HELLITE, 0},
    /* bosses and the story's named fights */
    {"PUCK KING", A_PUCKKING, 0, 2, 312, 18, 6, 9, 5, 6, 400, 333, {HIT, ACT(EA_MULTI, 3, 2)}, SK_PROSPECT, IT_SILVER, IT_GOLD, EF_BOSS | EF_NORUN},
    {"THE SCOURGE", A_SCOURGE, 0, 2, 692, 30, 12, 10, 5, 10, 2360, 874, {HIT, ACT(EA_ALL, 3, 0), ACT(EA_PARALYZE, 2, 0)}, SK_SIPHON, -1, IT_MEDCHEST, EF_BOSS | EF_NORUN}, /* 79 */
    {"BRUTUS", A_BRUTUS, 0, 2, 2400, 52, 40, 8, 0, 20, 6666, 5, {HIT, ACT(EA_MULTI, 3, 2), ACT(EA_ALL, 2, 0)}, 0, -1, -1, EF_BOSS | EF_NORUN}, /* 80 */
    {"TALLYMAN", A_TALLYMAN, 0, 2, 500, 26, 10, 10, 5, 10, 1026, 454, {HIT, ACT(EA_CALL, 2, 16), ACT(EA_WEAKEN, 2, 0)}, 0, -1, -1, EF_BOSS | EF_NORUN},
    {"JACKAL JACK", A_JACK, 0, 2, 1038, 40, 14, 16, 15, 12, 2408, 2000, {HIT, ACT(EA_MULTI, 4, 3), ACT(EA_STEAL, 2, 100)}, 0, -1, -1, EF_BOSS | EF_NORUN},
    {"SLICK MAGPIE", A_MAGPIE, 0, 2, 600, 36, 10, 20, 25, 12, 544, 362, {HIT, ACT(EA_MULTI, 4, 2)}, 0, IT_SALVE, IT_LAWMAN, EF_BOSS | EF_NORUN},
    {"THE MIMIC", A_MIMIC, 0, 2, 1500, 50, 16, 12, 10, 16, 3333, 3333, {HIT, ACT(EA_DRAIN, 3, 40), ACT(EA_ALL, 2, 0)}, SK_SIPHON, -1, -1, EF_BOSS | EF_NORUN},
    {"BLACK MOLLY", A_MOLLY, 0, 2, 1200, 46, 14, 16, 20, 14, 2256, 1630, {HIT, ACT(EA_MULTI, 4, 2)}, 0, IT_BLACKBREW, IT_BLACKBREW, EF_BOSS | EF_NORUN},
    {"DEMON TROOPER", A_TROOPER, 0, 0, 217, 23, 12, 10, 5, 10, 300, 150, {HIT}, 0, -1, -1, EF_NORUN},
    {"PIT CAPTAIN", A_CAPTAIN, 0, 2, 468, 40, 16, 12, 10, 12, 786, 725, {HIT, ACT(EA_ALL, 3, 0)}, 0, -1, -1, EF_BOSS | EF_NORUN},
    {"STORM CROW", A_STORMCROW, 0, 2, 2000, 60, 18, 18, 20, 16, 2000, 0, {HIT, ACT(EA_ALL, 4, 0), ACT(EA_PARALYZE, 2, 0)}, SK_SIPHON, -1, -1, EF_BOSS | EF_NORUN | EF_FLIER},
    {"CINDER WORM", A_WORM, 0, 2, 2400, 72, 30, 10, 5, 20, 4000, 3200, {HIT, ACT(EA_FIRE, 3, 60)}, 0, -1, -1, EF_BOSS | EF_NORUN},
    {"SAGUARO MOTHER", A_MOTHER, 0, 2, 3200, 70, 24, 10, 0, 24, 4000, 0, {HIT, ACT(EA_HEALALL, 2, 300), ACT(EA_ALL, 3, 0)}, 0, IT_CACTUSFIG, IT_CACTUSFIG, EF_BOSS | EF_NORUN}, /* 90 */
    {"LEFT ARM", A_ARM, 0, 1, 1200, 64, 20, 12, 5, 20, 1000, 0, {HIT, ACT(EA_PARALYZE, 2, 0)}, 0, -1, -1, EF_BOSS | EF_NORUN},
    {"RIGHT ARM", A_ARM, 1, 1, 1200, 72, 20, 12, 5, 20, 1000, 0, {HIT, ACT(EA_MULTI, 2, 2)}, 0, -1, -1, EF_BOSS | EF_NORUN},
    {"BLIGHT", A_BLIGHT, 0, 2, 3600, 90, 30, 14, 10, 26, 3256, 1300, {HIT, ACT(EA_POISONALL, 4, 0), ACT(EA_ALL, 2, 0)}, 0, -1, -1, EF_BOSS | EF_NORUN},
    {"CARNAGE", A_CARNAGE, 0, 2, 4000, 110, 36, 16, 10, 26, 2907, 8100, {HIT, ACT(EA_MULTI, 4, 3), ACT(EA_ALL, 2, 0)}, SK_RAGE, IT_GRINDER, IT_THRESHER, EF_BOSS | EF_NORUN},
    {"HUNGER", A_HUNGER, 0, 2, 4160, 100, 32, 14, 15, 28, 4250, 0, {HIT, ACT(EA_DRAIN, 4, 120), ACT(EA_WEAKEN, 2, 0)}, SK_SIPHON, IT_HALFHERB, IT_HALFSALVE, EF_BOSS | EF_NORUN},
    {"REAPER", A_REAPER, 0, 2, 4800, 120, 36, 18, 15, 30, 8000, 1800, {HIT, ACT(EA_DEATH, 3, 0), ACT(EA_ALL, 2, 0)}, SK_DEADSHOT, IT_DIAMOND, IT_HELLITE, EF_BOSS | EF_NORUN},
    {"JUGGERNAUT", A_JUGGERNAUT, 0, 2, 24000, 200, 80, 20, 0, 50, 80000, 80000, {HIT, ACT(EA_ALL, 4, 0), ACT(EA_DELETE, 2, 0), ACT(EA_MULTI, 2, 3)}, 0, -1, -1, EF_BOSS | EF_NORUN | EF_NOBRIBE},
    {"THE GOVERNOR", A_GOVERNOR, 0, 2, 4000, 120, 30, 14, 10, 30, 0, 0, {HIT, ACT(EA_ALL, 3, 0)}, 0, -1, -1, EF_BOSS | EF_NORUN | EF_NOBRIBE},
    {"NADIR", A_NADIR, 0, 2, 6000, 140, 40, 20, 10, 40, 0, 0, {HIT, ACT(EA_DEATH, 4, 0), ACT(EA_ALL, 3, 0), ACT(EA_MULTI, 2, 2)}, 0, IT_BLACKBREW, IT_BLACKBREW, EF_BOSS | EF_NORUN}, /* 98 */
    {"ZENITH", A_ZENITH, 0, 2, 8000, 130, 40, 16, 10, 40, 0, 0, {HIT, ACT(EA_HEALALL, 2, 500), ACT(EA_ALL, 3, 0), ACT(EA_DEATH, 2, 0)}, 0, IT_BLACKBREW, IT_BLACKBREW, EF_BOSS | EF_NORUN | EF_NOBRIBE},
};
const int SPUR_ENEMIES = ARRAY_LEN(SPUR_ENEMY);
#undef HIT
#undef ACT

int spur_enemy_find(const char *name) {
    for (int i = 1; i < SPUR_ENEMIES; i++)
        if (!strcmp(SPUR_ENEMY[i].name, name)) return i;
    return 0;
}

/* ---- troops ------------------------------------------------------------------------------ */
#define R(...) {{__VA_ARGS__}, 1, 3, TRF_RANDOMN}
#define RN(lo, hi, ...) {{__VA_ARGS__}, lo, hi, TRF_RANDOMN}
#define FX(...) {{__VA_ARGS__}, 0, 0, 0}
#define BOSS(...) {{__VA_ARGS__}, 0, 0, TRF_BOSS | TRF_NORUN}
const SpurTroop SPUR_TROOP[] = {
    FX(0),
    /* 1.. the Sweetgrass */
    RN(1, 3, 1, 1, 2), RN(1, 3, 1, 2, 3), RN(1, 2, 3, 4), RN(1, 3, 4, 5, 1), RN(1, 2, 5, 2),
    /* 6.. Dry Wash */
    RN(2, 4, 6, 8, 1), RN(1, 3, 6, 5), FX(9), RN(2, 4, 8, 8, 6),
    /* 10.. Sixpenny */
    RN(2, 4, 6, 8), RN(1, 2, 7, 6), RN(1, 3, 5, 7), FX(7, 7),
    /* 14.. the Bottoms */
    RN(1, 3, 10, 10, 11), RN(1, 3, 12, 4, 10), RN(2, 4, 13, 13, 3), RN(1, 2, 15, 14), RN(1, 3, 11, 12),
    /* 19.. the Deep Pen */
    RN(2, 4, 16, 16, 17), RN(1, 3, 17, 14, 13), RN(1, 2, 18, 16), RN(1, 2, 19, 12), RN(2, 3, 8, 16, 17),
    /* 24.. Widow's Notch */
    RN(2, 3, 10, 11, 22), RN(1, 3, 12, 15, 3), FX(21), RN(1, 2, 32, 22),
    /* 28.. the High Mesa */
    RN(1, 3, 20, 24, 22), RN(1, 2, 21, 23), RN(2, 4, 24, 24, 23), RN(1, 2, 26, 20), FX(27), FX(28), RN(1, 2, 28, 21),
    /* 35.. the crypt */
    RN(2, 4, 6, 30, 8), RN(1, 2, 18, 14), RN(2, 3, 16, 41, 30),
    /* 38.. the Roost mine */
    RN(1, 3, 20, 31, 30), RN(1, 2, 32, 15), RN(2, 3, 33, 16, 31), FX(9),
    /* 42.. Hollow Gap and its mine */
    RN(1, 2, 34, 20), RN(2, 3, 41, 33, 21), RN(1, 2, 35, 36), RN(1, 2, 37, 34), RN(2, 4, 36, 36, 41),
    /* 47.. Smugglers' Cut */
    RN(1, 3, 38, 29, 22), RN(1, 2, 39, 32), RN(1, 2, 40, 15), RN(2, 3, 29, 12, 38),
    /* 51.. the Burnt Hills */
    RN(1, 2, 43, 39), RN(1, 3, 44, 45), RN(1, 2, 25, 46), RN(1, 3, 48, 45, 26),
    /* 55.. the Snake Tunnels */
    RN(1, 3, 40, 15, 42), RN(2, 4, 41, 30, 42), RN(1, 2, 35, 37), RN(2, 6, 47, 40), RN(2, 3, 9, 37),
    /* 60.. the Dreaming */
    RN(1, 3, 49, 50, 52), RN(1, 3, 51, 51, 53), RN(1, 2, 53, 49), FX(54), FX(55),
    /* 65.. Hell */
    RN(1, 2, 56, 57), RN(1, 3, 58, 59, 34), RN(1, 3, 57, 59),
    /* 68.. the Bone Tower */
    RN(2, 4, 31, 60, 61), RN(1, 3, 59, 60, 41), RN(1, 3, 63, 62, 64), RN(1, 2, 43, 56, 58), RN(2, 4, 67, 66, 36),
    RN(1, 3, 68, 37, 65), RN(1, 3, 51, 70, 69), RN(1, 3, 71, 44, 46),
    /* 76.. the citadel */
    {{74, 72}, 2, 6, TRF_TWOPOOL}, RN(1, 3, 72, 51, 65), RN(1, 3, 59, 57, 64), RN(1, 3, 70, 73), RN(1, 2, 75, 76), FX(77),
    /* 82.. bosses */
    BOSS(78, 5, 5),           /* 82 the Puck King and two pucks */
    BOSS(79),                 /* 83 the Scourge */
    BOSS(80),                 /* 84 Brutus */
    BOSS(81, 16, 16, 16, 16), /* 85 the Tallyman and four imps */
    BOSS(82),                 /* 86 Jackal Jack */
    BOSS(83),                 /* 87 Slick Magpie */
    BOSS(84),                 /* 88 the Mimic */
    BOSS(85, 29, 29),         /* 89 Black Molly and two outlaws */
    BOSS(87, 86, 86, 86, 86, 86), /* 90 the siege: a captain and five troopers */
    BOSS(88),                 /* 91 the Storm Crow */
    BOSS(89),                 /* 92 the Cinder Worm */
    BOSS(90, 91, 92),         /* 93 the Saguaro Mother and her arms */
    BOSS(93),                 /* 94 Blight */
    BOSS(94),                 /* 95 Carnage */
    BOSS(95),                 /* 96 Hunger */
    BOSS(96),                 /* 97 the Reaper */
    BOSS(97),                 /* 98 the Juggernaut */
    BOSS(98),                 /* 99 the Governor */
    BOSS(100, 99),            /* 100 Zenith and Nadir */
    /* 101.. fixed ambushes */
    FX(4, 4, 5), FX(7, 7, 6), FX(19, 12, 12), FX(17, 17, 18), FX(29, 29, 21), FX(36, 34, 33), FX(39, 39),
    FX(47, 47, 47), FX(45, 45, 44), FX(63, 62, 62), FX(70, 69, 71), FX(77, 75), FX(76, 74, 74), FX(28, 28),
    FX(40, 40, 40, 40), FX(18, 18, 30),
    /* 117.. the late overworld */
    RN(1, 3, 56, 44, 45), RN(1, 2, 39, 57, 43), RN(1, 3, 56, 57, 44),
};
const int SPUR_TROOPS = ARRAY_LEN(SPUR_TROOP);
#undef R
#undef RN
#undef FX
#undef BOSS

/* ---- zones -------------------------------------------------------------------------------- */
const SpurZone SPUR_ZONE[] = {
    /* name, wisps per 100 steps, troops, a rare troop and its chance in 100 */
    {"SWEETGRASS", 7, {1, 2, 3, 1, 2, 4, 5, 0}, 0, 0},
    {"DRY WASH", 9, {6, 7, 9, 6, 7, 9, 6, 8}, 0, 0},
    {"SIXPENNY", 9, {10, 11, 12, 10, 12, 13, 0}, 0, 0},
    {"BOTTOMS", 8, {14, 15, 16, 17, 18, 0}, 0, 0},
    {"DEEP PEN", 9, {19, 20, 21, 22, 23, 0}, 0, 0},
    {"NOTCH", 9, {24, 25, 26, 27, 24, 0}, 0, 0},
    {"HIGH MESA", 8, {28, 29, 30, 31, 28, 29, 0}, 32, 2},
    {"THE WOODS", 8, {34, 33, 34, 0}, 0, 0},
    {"CRYPT", 9, {35, 36, 37, 0}, 0, 0},
    {"ROOST MINE", 9, {38, 39, 40, 41, 38, 0}, 0, 0},
    {"HOLLOW GAP", 8, {42, 43, 28, 31, 0}, 32, 2},
    {"HOLLOW MINE", 9, {43, 44, 45, 46, 42, 0}, 0, 0},
    {"THE CUT", 9, {47, 48, 49, 50, 0}, 0, 0},
    {"BURNT HILLS", 8, {51, 52, 53, 54, 0}, 0, 0},
    {"SNAKE TUNNELS", 9, {55, 56, 57, 59, 55, 0}, 0, 0},
    {"THE DREAMING", 9, {60, 61, 62, 60, 61, 0}, 63, 4},
    {"SCORCHED REACH", 8, {65, 66, 67, 0}, 0, 0},
    {"TOWER ONE", 9, {68, 69, 0}, 0, 0},
    {"TOWER TWO", 9, {70, 71, 0}, 0, 0},
    {"TOWER THREE", 9, {72, 73, 0}, 0, 0},
    {"TOWER FOUR", 9, {74, 75, 0}, 0, 0},
    {"HELLSPUR", 9, {77, 78, 79, 80, 0}, 76, 9},
    {"OUTSKIRTS", 8, {117, 118, 52, 66, 0}, 0, 0},
    {"THE BLUFF", 8, {117, 118, 119, 0}, 98, 12},
    {"ALLIGATOR RUN", 0, {58, 0}, 0, 0},
    {"PEN EDGE", 8, {28, 29, 31, 0}, 0, 0},
    {"DREAMING DEEP", 9, {61, 62, 60, 0}, 64, 2},
};
const int SPUR_ZONES = ARRAY_LEN(SPUR_ZONE);

/* ---- shops and towns ---------------------------------------------------------------------- */
const SpurShop SPUR_SHOP[] = {
    {"", {0}},
    {"GENERAL STORE", {IT_BANDAGE, IT_POCKET, IT_VEST, IT_WORNBOOTS}},                                         /* 1 Gladwell */
    {"GENERAL STORE", {IT_BANDAGE, IT_SNAKEROOT, IT_LARIAT, IT_CLEAVER, IT_KERCHIEF, IT_WOOLCOAT, IT_COTTON, IT_RIDING}}, /* 2 */
    {"GUNSMITH", {IT_POCKET, IT_SCATTER}},                                                                      /* 3 Meridian */
    {"GENERAL STORE", {IT_BANDAGE, IT_SNAKEROOT, IT_LARIAT, IT_CAMPKIT, IT_BOLOTIE, IT_LEATHER, IT_WOOL, IT_SNAKESKIN}}, /* 4 Hope */
    {"GUNSMITH", {IT_DUCKFOOT, IT_SCATTER, IT_LONGTOM}},                                                        /* 5 */
    {"GENERAL STORE", {IT_BANDAGE, IT_SALVE, IT_SNAKEROOT, IT_LARIAT, IT_BOWIE, IT_SERAPE, IT_BUCKSKIN, IT_OSTRICH}}, /* 6 Steepleton */
    {"QUARTERMASTER", {IT_SALVE, IT_SNAKEROOT, IT_CAMPKIT, IT_HOLYOIL, IT_SMUDGE, IT_LARIAT, IT_CRANKGUN, IT_LUCKYCOIN}}, /* 7 Fort */
    {"TRADING POST", {IT_SALVE, IT_SNAKEROOT, IT_LARIAT, IT_CAMPKIT, IT_BUNTLINE, IT_QUAD, IT_CAVALRY, IT_CRANKGUN}}, /* 8 Dolores */
    {"GENERAL STORE", {IT_SALVE, IT_MEDCHEST, IT_SNAKEROOT, IT_CAMPKIT, IT_HOLYOIL, IT_SMUDGE, IT_DUSTER, IT_SILK}}, /* 9 Last Chance */
    {"GUNSMITH", {IT_LAWMAN, IT_BIGBORE, IT_PLAINSMAN, IT_GRINDER}},                                            /* 10 */
    {"BONE MARKET", {IT_SALVE, IT_MEDCHEST, IT_SNAKEROOT, IT_LARIAT, IT_TOOTHNECK, IT_HELLHIDE}},               /* 11 Ashmarket */
    {"HELL'S ARMORY", {IT_BLACKGLASS, IT_WIDOWMAKER, IT_ELEPHANT, IT_HELLFIRE}},                                /* 12 */
    {"THE KENNEL", {IT_BEEFBONE, IT_SOUPBONE, IT_WISHBONE}},                                                   /* 13 Hermitage */
    {"SKY BAR", {IT_BLACKBREW, IT_GINGERPOP}},                                                                  /* 14 */
    {"MOLLY'S BAR", {IT_BLACKBREW}},                                                                            /* 15 */
    {"THE OLD PROSPECTOR", {IT_PROSPECTBOOK}},                                                                  /* 16 Hollow Gap */
    {"SHERIFF'S DESK", {IT_STEADYBOOK}},                                                                        /* 17 Steepleton */
    {"SHERIFF'S DESK", {IT_HALOPIN}},                                                                           /* 18 Last Chance */
    {"GENERAL STORE", {IT_SALVE, IT_SNAKEROOT, IT_CAMPKIT, IT_MINT}},                                           /* 19 Hollow Gap past */
};
const int SPUR_SHOPS = ARRAY_LEN(SPUR_SHOP);

/*                          hotel chapel bank saloon stables shopA shopB stableprice */
const SpurTownDef SPUR_TOWN[SPUR_TOWNS] = {
    /* GLADWELL */ {10, 1, 0, 0, 0, 1, 0, 0},
    /* MERIDIAN */ {20, 1, 1, 1, 1, 2, 3, 100},
    /* ST CLARE */ {0, 1, 0, 0, 0, 0, 0, 0},
    /* HOPE SPRINGS */ {60, 1, 0, 1, 0, 4, 5, 0},
    /* STEEPLETON */ {120, 1, 0, 0, 1, 6, 17, 200},
    /* FORT PATIENCE */ {0, 1, 0, 0, 0, 7, 0, 0},
    /* DOLORES */ {200, 1, 0, 1, 0, 8, 0, 0},
    /* HOLLOW GAP */ {0, 0, 0, 0, 0, 19, 16, 0},
    /* LAST CHANCE */ {500, 1, 1, 1, 1, 9, 10, 500},
    /* ASHMARKET */ {1000, 0, 0, 0, 0, 11, 12, 0},
    /* HERMITAGE */ {0, 1, 0, 0, 0, 13, 0, 0},
};

/* ---- numbers about the posse --------------------------------------------------------------- */

/* the total XP to reach a level: 5(L-1)^2 + (L-1)^4 / 16 (49 levels to 50) */
int spur_xp_for(int level) {
    if (level <= 1) return 0;
    long n = level - 1;
    return (int)(5 * n * n + n * n * n * n / 16);
}

int spur_hero_at(int slot) {
    if (slot < 0 || slot >= 4) return -1;
    int h = sg.s.order[slot];
    return h < H_COUNT ? h : -1;
}

bool spur_alive(int h) { return h >= 0 && h < H_COUNT && sg.s.hero[h].hp > 0; }

int spur_posse_alive(void) {
    int n = 0;
    for (int i = 0; i < 4; i++) n += spur_alive(spur_hero_at(i));
    return n;
}

int spur_passive(int h, int p) {
    int lv = sg.s.hero[h].level, n = 0;
    for (int i = 0; i < SPUR_MAX_LEARN; i++) {
        const SpurLearn *l = &SPUR_LEARN[h][i];
        if (l->level == 0) break;
        if (l->level <= lv && l->passive == p && l->skill == 0) n++;
    }
    n += sg.s.hero[h].passive_bonus[p];
    return imin(n, 5);
}

int spur_base_stat(int h, int st) {
    const SpurHeroDef *d = &SPUR_HERO[h];
    int lv = sg.s.hero[h].level;
    int v = d->base[st] + d->grow[st] * (lv - 1) / 10;
    v += sg.s.hero[h].treat[st];
    return v;
}

int spur_stat(int h, int st) {
    int v = spur_base_stat(h, st);
    const SpurHero *hr = &sg.s.hero[h];
    for (int s = 0; s < EQ_COUNT; s++) {
        int it = hr->equip[s];
        if (!it) continue;
        const SpurItemDef *d = &SPUR_ITEM[it];
        if (d->kind == IK_NECK) {
            if (st == ST_EVD) v += d->power;
            if (st == ST_RES) v += d->res;
            if (st == ST_SP && (d->flags & IF_SPIRIT)) v += 50;
        }
        if (d->kind == IK_BODY && st == ST_DEF) v += d->power;
        if (d->kind == IK_FEET && st == ST_SPD) v += d->power;
    }
    if (st == ST_EVD) v = imin(v, 60); /* the cap */
    return v;
}

int spur_maxhp(int h) { return imin(999, spur_stat(h, ST_HP)); }
int spur_maxsp(int h) { return imin(999, spur_stat(h, ST_SP)); }

bool spur_has_skill(int h, int sk) {
    if (sk <= 0 || sk >= SK_COUNT) return false;
    const SpurHero *hr = &sg.s.hero[h];
    if (sk < 32) { if (hr->skills_lo & (1u << sk)) return true; }
    else if (hr->skills_hi & (1u << (sk - 32))) return true;
    for (int i = 0; i < SPUR_MAX_LEARN; i++) {
        const SpurLearn *l = &SPUR_LEARN[h][i];
        if (l->level == 0) break;
        if (l->skill == sk && l->level <= hr->level) return true;
    }
    return false;
}

void spur_learn_skill(int h, int sk) {
    SpurHero *hr = &sg.s.hero[h];
    if (sk < 32) hr->skills_lo |= 1u << sk;
    else hr->skills_hi |= 1u << (sk - 32);
}

/* the menu order is the order the skill table lists them */
int spur_skill_list(int h, int *out) {
    int n = 0;
    for (int sk = 1; sk < SK_COUNT; sk++)
        if (SPUR_SKILL[sk].name[0] && spur_has_skill(h, sk)) out[n++] = sk;
    return n;
}

int spur_skill_count(int h) {
    int tmp[SK_COUNT];
    return spur_skill_list(h, tmp);
}

static int class_passive(int wc) {
    switch (wc) {
    case WC_MELEE: return PS_MELEE;
    case WC_PISTOL: return PS_PISTOL;
    case WC_RIFLE: return PS_RIFLE;
    case WC_SHOTGUN: return PS_SHOTGUN;
    case WC_HEAVY: return PS_HEAVY;
    case WC_BOW: return PS_SPIRIT;
    default: return -1;
    }
}

bool spur_can_equip(int h, int it, int slot) {
    if (it <= 0 || it >= IT_COUNT) return false;
    const SpurItemDef *d = &SPUR_ITEM[it];
    bool dog = h == H_BISCUIT;
    switch (d->kind) {
    case IK_WEAPON: {
        if (dog) return false;
        if (slot != EQ_RHAND && slot != EQ_LHAND) return false;
        if ((d->flags & IF_TWOHAND) && spur_passive(h, PS_ONEARM)) return false;
        if (d->wclass == WC_BOW) return !spur_passive(h, PS_ONEARM);
        int p = class_passive(d->wclass);
        if (p >= 0 && spur_passive(h, p) < 1) return false;
        if (slot == EQ_LHAND) {
            /* the second hand: two of the same one-handed class, with TWO GUNS */
            if (d->flags & IF_TWOHAND) return false;
            if (!spur_passive(h, PS_DUAL)) return false;
            int r = sg.s.hero[h].equip[EQ_RHAND];
            return r && SPUR_ITEM[r].wclass == d->wclass && !(SPUR_ITEM[r].flags & IF_TWOHAND);
        }
        return true;
    }
    case IK_NECK: return slot == EQ_NECK;
    case IK_BODY:
        if (dog || slot != EQ_BODY) return false;
        if ((d->flags & IF_TUCKER) && h != H_TUCKER) return false;
        if (h == H_CORA && !(d->flags & IF_LIGHT)) return false;
        return true;
    case IK_FEET: return !dog && slot == EQ_FEET;
    default: return false;
    }
}

int spur_weapon_class(int h) {
    int r = sg.s.hero[h].equip[EQ_RHAND];
    return r ? SPUR_ITEM[r].wclass : WC_MELEE;
}

/* the swing's strength: half the hero's own ATK plus the weapon's ATK,
 * which each level of the weapon's skill raises by half */
int spur_attack_power(int h) {
    int a = spur_stat(h, ST_ATK) / 2;
    int r = sg.s.hero[h].equip[EQ_RHAND];
    if (!r) {
        /* bare hands, or the hound's jaws, which grow with him */
        int m = imax(1, spur_passive(h, PS_MELEE));
        int base = h == H_BISCUIT ? spur_stat(h, ST_ATK) / 2 + 4 : 2;
        return a + base * (m + 1) / 2;
    }
    const SpurItemDef *d = &SPUR_ITEM[r];
    int p = class_passive(d->wclass);
    int lv = p >= 0 ? imax(1, spur_passive(h, p)) : 1;
    return a + d->power * (lv + 1) / 2;
}

void spur_hero_init(int h) {
    SpurHero *hr = &sg.s.hero[h];
    memset(hr, 0, sizeof *hr);
    hr->level = 1;
    hr->hp = (int16_t)spur_maxhp(h);
    hr->sp = (int16_t)spur_maxsp(h);
    /* what each stranger walked into the saloon with */
    static const uint8_t KIT[H_COUNT][5] = {
        {IT_POCKET, 0, 0, IT_VEST, 0},     {0, 0, 0, IT_COTTON, 0},       {IT_POCKET, 0, 0, IT_VEST, 0},
        {0, 0, 0, IT_VEST, 0},             {IT_POCKET, 0, 0, IT_VEST, 0}, {IT_POCKET, 0, 0, IT_VEST, 0},
        {IT_POCKET, 0, 0, IT_COTTON, 0},   {0, 0, IT_KERCHIEF, 0, 0},
    };
    for (int s = 0; s < EQ_COUNT; s++) hr->equip[s] = KIT[h][s];
    hr->hp = (int16_t)spur_maxhp(h);
    hr->sp = (int16_t)spur_maxsp(h);
}

/* apply any level ups the XP has earned; the message names the new things */
int spur_level_up_check(int h, char *msg, int n) {
    SpurHero *hr = &sg.s.hero[h];
    int ups = 0;
    if (msg && n) msg[0] = 0;
    while (hr->level < 50 && hr->xp >= spur_xp_for(hr->level + 1)) {
        int oldhp = spur_maxhp(h), oldsp = spur_maxsp(h);
        hr->level++;
        ups++;
        if (hr->hp > 0) {
            hr->hp = (int16_t)imin(spur_maxhp(h), hr->hp + spur_maxhp(h) - oldhp);
            hr->sp = (int16_t)imin(spur_maxsp(h), hr->sp + spur_maxsp(h) - oldsp);
        }
        for (int i = 0; i < SPUR_MAX_LEARN && msg; i++) {
            const SpurLearn *l = &SPUR_LEARN[h][i];
            if (l->level != hr->level) continue;
            const char *what = l->skill ? SPUR_SKILL[l->skill].name : SPUR_PASSIVE_NAME[l->passive];
            if ((int)strlen(msg) + (int)strlen(what) + 3 < n) {
                if (msg[0]) strcat(msg, ", ");
                strcat(msg, what);
            }
        }
    }
    return ups;
}

const char *spur_item_name(int it) { return it > 0 && it < IT_COUNT ? SPUR_ITEM[it].name : ""; }

int spur_bag_count(void) {
    int n = 0;
    for (int i = 0; i < SPUR_BAG; i++) n += sg.s.bag[i] != 0;
    return n;
}

int spur_bag_add(int it) {
    for (int i = 0; i < SPUR_BAG; i++)
        if (!sg.s.bag[i]) { sg.s.bag[i] = (uint8_t)it; return i; }
    return -1;
}

/* the bag closes up the gap */
void spur_bag_remove(int slot) {
    if (slot < 0 || slot >= SPUR_BAG) return;
    for (int i = slot; i < SPUR_BAG - 1; i++) sg.s.bag[i] = sg.s.bag[i + 1];
    sg.s.bag[SPUR_BAG - 1] = 0;
}

int spur_bag_find(int it) {
    for (int i = 0; i < SPUR_BAG; i++)
        if (sg.s.bag[i] == it) return i;
    return -1;
}

int spur_doc_level(void) {
    int best = 0;
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (spur_alive(h)) best = imax(best, spur_passive(h, PS_DOC));
    }
    return best;
}

int spur_sell_price(int it) { return it > 0 && it < IT_COUNT ? SPUR_ITEM[it].price / 2 : 0; }
