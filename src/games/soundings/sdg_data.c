/* SOUNDINGS - every number in the game: items and the shop, the creatures
 * and the groups they swim in, levels, chests and the notes on the walls.
 * Where the original's numbers are known they are kept (docs/games/
 * 27-soundings.md lists which are ours). */
#include "sdg.h"

const char *const SDG_EL_NAME[EL_COUNT] = {"", "REEF", "ZAP", "OOZE"};
const uint8_t SDG_EL_COL[EL_COUNT] = {C_LIGHT, C_RED, C_YELLOW, C_LIME};
const char *const SDG_RELIC_NAME[RL_COUNT] = {"FOAM", "GUM", "PEBBLE", "COG", "SPIRAL", "BONE", "SUN DISC", "MOON DISC"};
const char *const SDG_SHOP_PAGE[SHOP_PAGES] = {"POLEARMS", "HAMMERS", "SHIELDS", "TONICS", "KEY ITEMS"};

#define NR {-1, -1}, {0, 0}
#define R1(a, n) {a, -1}, {n, 0}
#define R2(a, n, b, m) {a, b}, {n, m}

/*  name          kind      el       pow  uses def grd heal  price  relics                         max flags        shop */
const SdgItem SDG_ITEM[IT_COUNT] = {
    [IT_NONE] = {"", K_PASSIVE, EL_NONE, 0, 0, 0, 0, 0, 0, NR, 0, 0, -1},
    [IT_POKER_R] = {"POKER", K_POLE, EL_REEF, 200, 25, 0, 0, 0, 100, NR, 6, 0, SHOP_POLE},
    [IT_POKER_Z] = {"POKER", K_POLE, EL_ZAP, 200, 25, 0, 0, 0, 100, NR, 6, 0, SHOP_POLE},
    [IT_POKER_O] = {"POKER", K_POLE, EL_OOZE, 200, 25, 0, 0, 0, 100, NR, 6, 0, SHOP_POLE},
    [IT_GAFF_R] = {"GAFF", K_POLE, EL_REEF, 300, 25, 0, 0, 0, 600, R1(RL_SPIRAL, 3), 6, 0, SHOP_POLE},
    [IT_GAFF_Z] = {"GAFF", K_POLE, EL_ZAP, 300, 25, 0, 0, 0, 600, R1(RL_PEBBLE, 3), 6, 0, SHOP_POLE},
    [IT_GAFF_O] = {"GAFF", K_POLE, EL_OOZE, 300, 25, 0, 0, 0, 600, R1(RL_GUM, 2), 6, 0, SHOP_POLE},
    [IT_HARPOON_R] = {"HARPOON", K_POLE, EL_REEF, 400, 25, 0, 0, 0, 1500, R2(RL_SPIRAL, 2, RL_COG, 2), 6, 0, SHOP_POLE},
    [IT_HARPOON_Z] = {"HARPOON", K_POLE, EL_ZAP, 400, 25, 0, 0, 0, 1500, R1(RL_BONE, 3), 6, 0, SHOP_POLE},
    [IT_HARPOON_O] = {"HARPOON", K_POLE, EL_OOZE, 400, 25, 0, 0, 0, 1500, R1(RL_GUM, 6), 6, 0, SHOP_POLE},
    [IT_CLUB_R] = {"CLUB", K_HAMMER, EL_REEF, 150, 15, 0, 20, 0, 150, NR, 6, 0, SHOP_HAMMER},
    [IT_CLUB_Z] = {"CLUB", K_HAMMER, EL_ZAP, 150, 15, 0, 20, 0, 150, NR, 6, 0, SHOP_HAMMER},
    [IT_CLUB_O] = {"CLUB", K_HAMMER, EL_OOZE, 150, 15, 0, 20, 0, 150, NR, 6, 0, SHOP_HAMMER},
    [IT_MAUL_R] = {"MAUL", K_HAMMER, EL_REEF, 300, 15, 0, 45, 0, 800, R1(RL_COG, 3), 6, 0, SHOP_HAMMER},
    [IT_MAUL_Z] = {"MAUL", K_HAMMER, EL_ZAP, 300, 15, 0, 45, 0, 800, R2(RL_COG, 1, RL_BONE, 1), 6, 0, SHOP_HAMMER},
    [IT_MAUL_O] = {"MAUL", K_HAMMER, EL_OOZE, 300, 15, 0, 45, 0, 800, R2(RL_COG, 1, RL_GUM, 1), 6, 0, SHOP_HAMMER},
    [IT_PLATE_R] = {"PLATE", K_SHIELD, EL_REEF, 150, 20, 20, 0, 0, 150, NR, 6, 0, SHOP_SHIELD},
    [IT_PLATE_Z] = {"PLATE", K_SHIELD, EL_ZAP, 150, 20, 20, 0, 0, 150, NR, 6, 0, SHOP_SHIELD},
    [IT_PLATE_O] = {"PLATE", K_SHIELD, EL_OOZE, 150, 20, 20, 0, 0, 150, NR, 6, 0, SHOP_SHIELD},
    [IT_TARGE_R] = {"TARGE", K_SHIELD, EL_REEF, 250, 20, 45, 0, 0, 600, R2(RL_FOAM, 1, RL_SPIRAL, 2), 6, 0, SHOP_SHIELD},
    [IT_TARGE_Z] = {"TARGE", K_SHIELD, EL_ZAP, 250, 20, 45, 0, 0, 600, R2(RL_FOAM, 1, RL_PEBBLE, 2), 6, 0, SHOP_SHIELD},
    [IT_TARGE_O] = {"TARGE", K_SHIELD, EL_OOZE, 250, 20, 45, 0, 0, 600, R2(RL_FOAM, 1, RL_GUM, 2), 6, 0, SHOP_SHIELD},
    [IT_BULWARK_R] = {"BULWARK", K_SHIELD, EL_REEF, 350, 20, 60, 0, 0, 1200, R1(RL_COG, 2), 6, 0, SHOP_SHIELD},
    [IT_BULWARK_Z] = {"BULWARK", K_SHIELD, EL_ZAP, 350, 20, 60, 0, 0, 1200, R1(RL_BONE, 2), 6, 0, SHOP_SHIELD},
    [IT_BULWARK_O] = {"BULWARK", K_SHIELD, EL_OOZE, 350, 20, 60, 0, 0, 1200, R1(RL_GUM, 4), 6, 0, SHOP_SHIELD},
    [IT_LEECH] = {"LEECH CLUB", K_HAMMER, EL_OOZE, 250, 15, 0, 45, 0, 0, NR, 1, IF_LEECH | IF_UNIQUE, -1},
    [IT_SPINE] = {"SPINE TARGE", K_SHIELD, EL_REEF, 250, 10, 50, 0, 0, 0, NR, 1, IF_THORNS | IF_UNIQUE, -1},
    [IT_LAMP] = {"LAMP ROD", K_POLE, EL_ZAP, 350, 15, 0, 0, 350, 0, NR, 1, IF_HOLY | IF_UNIQUE, -1},
    [IT_SMALL] = {"SIP", K_POTION, EL_NONE, 0, 4, 0, 0, 500, 200, NR, 6, 0, SHOP_POTION},
    [IT_MEDIUM] = {"DRAUGHT", K_POTION, EL_NONE, 0, 8, 0, 0, 500, 1000, R1(RL_FOAM, 3), 6, 0, SHOP_POTION},
    [IT_LARGE] = {"FLAGON", K_POTION, EL_NONE, 0, 12, 0, 0, 500, 0, NR, 1, IF_UNIQUE, -1},
    [IT_HOLY] = {"HALLOW", K_POTION, EL_NONE, 0, 8, 0, 0, 2500, 2000, R1(RL_MOON, 1), 1, 0, SHOP_POTION},
    [IT_EVIL] = {"BITTERS", K_POTION, EL_NONE, 0, 4, 0, 0, 1000, 400, NR, 6, 0, SHOP_POTION},
    [IT_EGG] = {"EGG", K_EGG, EL_NONE, 0, 1, 0, 0, 0, 400, R1(RL_SPIRAL, 1), 6, 0, SHOP_POTION},
    [IT_BOMB] = {"CHARGE", K_BOMB, EL_NONE, 0, 1, 0, 0, 0, 500, R1(RL_PEBBLE, 1), 3, 0, SHOP_KEY},
    [IT_MIST] = {"INK SAC", K_MIST, EL_NONE, 0, 3, 0, 0, 0, 1000, R1(RL_SUN, 1), 1, 0, SHOP_KEY},
    [IT_GODBLOOD] = {"OLD BLOOD", K_PASSIVE, EL_NONE, 0, 0, 0, 0, 0, 800, NR, 1, 0, SHOP_KEY},
    [IT_FLIPPERS] = {"FINS", K_PASSIVE, EL_NONE, 0, 0, 0, 0, 0, 0, NR, 1, IF_UNIQUE, -1},
};

int sdg_shop_list(int page, uint8_t *out, int max) {
    int n = 0;
    for (int it = 1; it < IT_COUNT && n < max; it++)
        if (SDG_ITEM[it].shop == page) out[n++] = (uint8_t)it;
    return n;
}

bool sdg_is_weapon(int it) {
    int k = SDG_ITEM[it].kind;
    return it > IT_NONE && it < IT_COUNT && (k == K_POLE || k == K_HAMMER || k == K_SHIELD);
}

bool sdg_battle_usable(int it) {
    if (it <= IT_NONE || it >= IT_COUNT) return false;
    return SDG_ITEM[it].kind != K_PASSIVE;
}

/* ---- creatures ------------------------------------------------------------ */
/*  name         hp    xp    gold  weak     lo   hi   drop        flags                                   heal spr */
const SdgFoeDef SDG_FOE[EN_COUNT] = {
    [EN_FIZZLE] = {"FIZZLE", 300, 10, 20, EL_REEF, 118, 151, RL_PEBBLE, FF_IDLE, 0, SP_FIZZLE},
    [EN_PRICKLE] = {"NEEDLER", 200, 30, 30, EL_OOZE, 98, 182, RL_SPIRAL, FF_THORNS, 0, SP_PRICKLE},
    [EN_FROND] = {"FROND", 600, 20, 15, EL_REEF, 154, 162, RL_FOAM, FF_HEAL, 300, SP_FROND},
    [EN_NIPPER] = {"NIPPER", 600, 80, 30, EL_ZAP, 200, 205, RL_FOAM, FF_MAYDOUBLE | FF_LEECH | FF_STARTHURT, 0, SP_NIPPER},
    [EN_GLOB] = {"GLOB", 400, 22, 40, EL_ZAP, 160, 190, RL_GUM, 0, 0, SP_GLOB},
    [EN_SMOG] = {"SMOG EEL", 350, 20, 25, EL_ZAP, 138, 146, -1, FF_IDLE, 0, SP_SMOG},
    [EN_CLAMPER] = {"CLAMPER", 800, 50, 40, EL_OOZE, 235, 260, RL_SPIRAL, FF_TELEGRAPH | FF_GUARD, 0, SP_CLAMPER},
    [EN_TINFIN] = {"TINFIN", 700, 100, 30, EL_OOZE, 225, 239, RL_COG, FF_DOUBLE, 0, SP_TINFIN},
    [EN_WHORL] = {"WHORL", 1000, 100, 30, EL_OOZE, 329, 429, -1, FF_HIDE, 0, SP_WHORL},
    [EN_JELLY] = {"GLIMMER", 500, 150, 100, EL_REEF, 321, 324, -1, FF_SHIFT, 0, SP_JELLY},
    [EN_GROPER] = {"GROPER", 1000, 300, 50, EL_ZAP, 373, 482, RL_GUM, 0, 0, SP_GROPER},
    [EN_BURRNUT] = {"BURRNUT", 580, 100, 200, EL_OOZE, 192, 206, RL_COG, FF_THORNS, 0, SP_BURRNUT},
    [EN_STILTER] = {"STILTER", 500, 120, 100, EL_OOZE, 328, 399, -1, 0, 0, SP_STILTER},
    [EN_LOUSE] = {"SLATER", 700, 300, 300, EL_OOZE, 444, 500, -1, FF_TELEGRAPH, 0, SP_LOUSE},
    [EN_HAUNT] = {"HAUNT", 1200, 500, 50, EL_REEF, 183, 207, RL_BONE, FF_HEAL | FF_REVIVE, 400, SP_HAUNT},
    [EN_SQUID] = {"DUSKSQUID", 1500, 1200, 600, EL_OOZE, 540, 570, RL_COG, FF_LOWEST, 0, SP_SQUID},
    [EN_GRINFISH] = {"GRINFISH", 1200, 800, 1000, EL_OOZE, 410, 440, -1, FF_GUARD, 0, SP_GRINFISH},
    [EN_WORM] = {"MAWWORM", 1500, 1500, 1000, EL_ZAP, 640, 665, -1, FF_MAYDOUBLE | FF_LEECH, 0, SP_WORM},
    [EN_WARDEN] = {"ABBOT", 5000, 1040, 1030, EL_OOZE, 392, 416, RL_SUN, FF_MAYDOUBLE | FF_HEAL | FF_REVIVE | FF_BOSS, 800, SP_WARDEN},
    [EN_EYE] = {"GLOAM EYE", 4000, 0, 0, EL_ZAP, 250, 300, -1, FF_HEAL | FF_BOSS, 600, SP_EYE},
    [EN_ARM] = {"GLOAM ARM", 5000, 0, 0, EL_REEF, 430, 470, -1, FF_TELEGRAPH | FF_GUARD | FF_BOSS, 0, SP_ARM},
};

/* ---- levels ---------------------------------------------------------------- */
const int32_t SDG_XP_AT[SDG_MAX_LEVEL + 1] = {0, 0, 50, 150, 400, 900, 1800, 4600, 8800, 15500, 26000, 42000, 65000, 98000, 145000, 210000, 300000};
const int16_t SDG_HP_AT[SDG_MAX_LEVEL + 1] = {500, 500, 640, 780, 920, 1060, 1200, 1340, 1480, 1620, 1760, 1900, 2050, 2200, 2300, 2400, 2500};

/* 60 % at level 1, 150 % at 10, 210 % at 16 */
int sdg_level_mul(int level) { return 50 + 10 * iclamp(level, 1, SDG_MAX_LEVEL); }

/* ---- chests, in the order the map is read (row by row) -------------------- */
const SdgChest SDG_CHEST[SDG_CHESTS] = {
    {CH_PEARL, IT_LEECH},      /* the hollow's top corner */
    {CH_RELIC, RL_GUM},        /* the shelf, by the surface */
    {CH_PEARL, IT_SMALL},      /* gumwell's west end */
    {CH_GOLD, 100},            /* the shelf's floor, west */
    {CH_PEARL, IT_FLIPPERS},   /* behind the soft wall on the way round */
    {CH_PEARL, IT_GAFF_Z},     /* under the shelf's cracked floor */
    {CH_PEARL, IT_GAFF_R},     /* the shrine's high nook */
    {CH_GOLD, 500},            /* gumwell, behind cracked rock */
    {CH_GOLD, 500},            /* under the hollow's cracked floor */
    {CH_RELIC, RL_BONE},       /* the shrine's east nook */
    {CH_GOLD, 500},            /* the shrine's west hall, unseen */
    {CH_PEARL, IT_SPINE},      /* the sunk vessel's room */
    {CH_GOLD, 1500},           /* lantern cave, unseen */
    {CH_PEARL, IT_LARGE},      /* lantern cave's west end */
    {CH_RELIC, RL_MOON},       /* the stilt pit's floor */
    {CH_PEARL, IT_LAMP},       /* before the last door, unseen */
    {CH_GOLD, 2000},           /* the red deep's west pocket */
};

const char *const SDG_LORE_TEXT[SDG_LORES][2] = {
    {"A SMALL SKULL, WORN SMOOTH.", "SOMETHING WITH GILLS ONCE WORE IT."},
    {"AN OLD ANCHOR, SUNK TO ITS SHANK.", "IT WILL NOT BUDGE."},
    {"A STONE FIGURE WITH A FISH'S HEAD", "KEEPS WATCH OVER NOTHING."},
    {"ITS TWIN. ONE STONE HAND", "POINTS DOWN. DEEPER."},
    {"A ROUND VESSEL UNDER BARNACLES.", "A FADED NUMBER ON ITS SIDE: 40."},
    {"A RIBCAGE AS LONG AS THE RAFT.", "WHAT WAS IT?"},
    {"THE WALL IS WARM.", "IT IS BREATHING."},
};

const char *const SDG_HEAD_NAME[SDG_HEADS] = {"A DUSKLING FIGURE", "A MINER'S HELMET", "A STONE AXOLOTL HEAD"};
