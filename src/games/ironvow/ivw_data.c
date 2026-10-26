/* IRONVOW - every number: items, swings, foes, spells and their glyphs. */
#include "ivw.h"

const int IVW_DX[4] = {0, 1, 0, -1};
const int IVW_DY[4] = {-1, 0, 1, 0};

const char *const IVW_BIOME_NAME[BIO_COUNT] = {"THE GAOL", "THE WARREN", "THE STARVAULT"};

/* windup, active, recover frames; stamina a swing costs (before MIGHT) */
const IvwSwing IVW_SWING[6] = {
    {4, 3, 6, 5},    /* 0 very fast: the foil */
    {6, 3, 8, 7},    /* 1 fast: the dirk */
    {9, 4, 11, 11},  /* 2 middling */
    {12, 4, 13, 14}, /* 3 slow */
    {15, 5, 16, 18}, /* 4 heavy */
    {18, 5, 18, 22}, /* 5 very heavy */
};

#define W(n, v, r, d, s, f, desc) {n, IK_WEAPON, v, r, d, s, f, desc}
#define A(n, v, g, slow, cost, desc) {n, IK_ARMOUR, v, 0, g, slow, cost, desc}
#define S(n, v, blocks, pct, f, desc) {n, IK_SHIELD, v, 0, blocks, pct, f, desc}
#define P(n, v, amt, desc) {n, IK_POTION, v, 0, amt, 0, 0, desc}
#define M(n, v, amt, desc) {n, IK_PART, v, 0, amt, 0, 0, desc}
#define X(n, k, v, desc) {n, k, v, 0, 0, 0, 0, desc}
const IvwItem IVW_ITEM[IT_COUNT] = {
    [IT_NONE] = X("", IK_SPECIAL, 0, ""),
    [IT_DIRK] = W("DIRK", 0, 0, 5, 1, 0, "A SHORT BLADE. QUICK, AND LITTLE ELSE."),
    [IT_FOIL] = W("FOIL", 2, 3, 6, 0, 0, "LIGHT AND VERY QUICK. CHEAP TO SWING."),
    [IT_CUDGEL] = W("CUDGEL", 2, 5, 8, 2, WF_STUN, "A KNOTTED CLUB. IT CAN STUN."),
    [IT_TUSKMAUL] = W("TUSK MAUL", 1, 5, 8, 2, WF_STUN, "A TUSKER'S CLUB. IT CAN STUN."),
    [IT_ARMING] = W("ARMING SWORD", 4, 6, 11, 2, 0, "A KNIGHT'S PLAIN SWORD."),
    [IT_STAR] = W("MORNINGSTAR", 7, 8, 13, 3, WF_STUN, "A SPIKED HEAD ON A HAFT. IT CAN STUN."),
    [IT_GLAIVE] = W("GLAIVE", 7, 8, 12, 3, WF_REACH, "A LONG BLADE ON A POLE: IT REACHES THE BACK ROW."),
    [IT_BROAD] = W("BROADBLADE", 7, 8, 12, 3, WF_SWEEP, "A WIDE BLADE THAT SWEEPS THE FRONT ROW."),
    [IT_MAUL] = W("MAUL", 9, 10, 17, 4, WF_STUN, "A HAMMER FOR WALLS. IT CAN STUN."),
    [IT_AXE] = W("GREATAXE", 10, 12, 19, 4, WF_SWEEP, "A HUGE AXE THAT SWEEPS THE FRONT ROW."),
    [IT_OATH] = W("OATHBLADE", 10, 15, 22, 5, WF_REACH, "A GREAT TWO-HANDED SWORD. IT REACHES THE BACK ROW."),
    [IT_DAWN] = W("DAWNBRAND", 14, 5, 10, 2, WF_CHARGE | WF_BOLT,
                  "HOLD " GLYPH_A " TO CHARGE A BLOW NO GUARD STOPS. HALE, IT CASTS LIGHT."),
    [IT_RAGS] = A("RAGS", 0, 0, 0, 0, "WHAT THE GAOLERS LEFT YOU."),
    [IT_TUNIC] = A("TUNIC", 1, 5, 0, 0, "PLAIN WOOL."),
    [IT_COWL] = A("COWL", 1, 7, 0, 0, "A HOODED CLOAK. MANA COMES BACK A LITTLE FASTER."),
    [IT_JERKIN] = A("JERKIN", 1, 12, 0, 0, "BOILED LEATHER."),
    [IT_PADDED] = A("PADDED COAT", 2, 18, 0, 0, "QUILTED LAYERS."),
    [IT_BRONZE] = A("BRONZE MAIL", 4, 25, 15, 2, "HEAVY: SLOWER FEET, DEARER SWINGS."),
    [IT_IRON] = A("IRON MAIL", 6, 32, 20, 3, "HEAVY: SLOWER FEET, DEARER SWINGS."),
    [IT_STEEL] = A("STEEL MAIL", 7, 40, 25, 4, "HEAVY: SLOWER FEET, DEARER SWINGS."),
    [IT_MOULD] = A("MOULD CLOAK", 1, 10, 0, 0, "SPORES BITE WHAT HITS YOU. SPEND STAMINA TO WIN BACK LOST HEALTH."),
    [IT_BUCKLER] = S("PLANK BUCKLER", 0, 5, 100, 0, "A FEW BLOCKS A FIGHT, THEN IT GIVES."),
    [IT_OAK] = S("OAK SHIELD", 1, 8, 90, 0, "A ROUND OAK SHIELD."),
    [IT_BOAR] = S("BOAR SHIELD", 3, 12, 75, 0, "A STOUT SHIELD WITH A BOAR'S HEAD."),
    [IT_WYRM] = S("WYRM SHIELD", 9, 18, 60, 0, "SCALED AND HEAVY. IT HOLDS A LONG TIME."),
    [IT_WRAITH] = S("WRAITH SHIELD", 11, 14, 70, 2, "COLD TO THE TOUCH. EACH BLOCK GIVES MANA."),
    [IT_SKINKB] = S("SKINK BRACERS", 2, 0, 0, 1 | 4, "NO GUARD. QUICKER FEET; BLOWS POISON."),
    [IT_COILB] = S("COILER BRACERS", 4, 0, 0, 1 | 8, "NO GUARD. QUICKER FEET; HOLD " GLYPH_A " FOR A BLOW NO GUARD STOPS."),
    [IT_RED] = P("RED TONIC", 1, 40, "HEALS A GREAT DEAL."),
    [IT_BLACK] = P("BLACK TONIC", 1, 900, "HARDER BLOWS FOR A WHILE."),
    [IT_BLUE] = P("BLUE TONIC", 1, 30, "STAMINA AND MANA BACK."),
    [IT_GREEN] = P("GREEN TONIC", 1, 900, "HASTE FOR A WHILE."),
    [IT_YELLOW] = P("YELLOW TONIC", 1, 240, "REFLEX: THE NEXT FIGHT'S FOES START STUNNED."),
    [IT_GREY] = P("GREY TONIC", 1, 900, "STURDY: A STRONGER GUARD FOR A WHILE."),
    [IT_OOZESKULL] = M("OOZE SKULL", 0, 300, "READY IT: THROWN AT THE NEXT FIGHT, IT BOUNCES AND STUNS."),
    [IT_HEART] = M("BAT HEART", 0, 15, "READY IT: A SMALL HEAL AT THE NEXT FIGHT."),
    [IT_TUSK] = M("TUSK", 2, 12, "READY IT: STABS A FOE AT THE NEXT FIGHT."),
    [IT_BONESKULL] = M("BONE SKULL", 1, 600, "READY IT: LIKE AN OOZE SKULL, BUT LASTS LONGER."),
    [IT_BONE] = M("BONE", 0, 3, "USE IT: NEARBY FLAMES FLEE."),
    [IT_HEAD] = M("CHITIN HEAD", 1, 8, "USE IT: LEAP TO A TILE YOU HAVE SEEN, CLOSE BY."),
    [IT_SAC] = M("VENOM SAC", 1, 400, "READY IT: POISONS A FOE AT THE NEXT FIGHT."),
    [IT_BALL] = M("JUGGLER'S BALL", 1, 9999, "READY IT: BOUNCES ALL FIGHT LONG."),
    [IT_COIN] = X("AMBER COIN", IK_SPECIAL, 0, "FLIP IT FOR LUCK, GOOD OR BAD. WELLS LIKE COINS."),
    [IT_EGG] = X("GRUB EGG", IK_SPECIAL, 1, "SOMETHING MOVES INSIDE."),
    [IT_GRUB] = X("FIGHTING GRUB", IK_SPECIAL, 1, "IT JOINS YOUR FIGHTS, BOUNCING."),
    [IT_BEETLE] = X("WAR BEETLE", IK_SPECIAL, 1, "READY IT: IT FIGHTS IN YOUR PLACE. " GLYPH_B " CALLS IT BACK."),
    [IT_CINDER] = X("CINDER STONE", IK_KEY, 0, "WARM. IT BELONGS IN A PILLAR."),
    [IT_MOSS] = X("MOSS STONE", IK_KEY, 0, "DAMP. IT BELONGS IN A PILLAR."),
    [IT_FROST] = X("FROST STONE", IK_KEY, 0, "COLD. IT BELONGS IN A PILLAR."),
    [IT_IRONKEY] = X("IRON KEY", IK_KEY, 0, "IT OPENS ONE LOCKED DOOR."),
};

/*            name          hp  dmg st win act rec spd rng ess flags                                    drops */
const IvwFoeDef IVW_FOE[FK_COUNT] = {
    [FK_OOZE] = {"OOZE", 16, 5, 8, 40, 6, 40, 8, 16, 10, 0, IT_OOZESKULL, 0},
    [FK_GNAW] = {"GNAWWING", 13, 5, 6, 30, 6, 46, 22, 14, 15, FF_FLYER, IT_HEART, 0},
    [FK_LIDMAW] = {"LIDMAW", 32, 6, 6, 50, 6, 40, 8, 16, 30, FF_GUARD | FF_NOPOISON, 0, 0},
    [FK_TUSKER] = {"TUSKER", 30, 9, 18, 36, 6, 44, 12, 18, 20, FF_GUARD, IT_TUSK, IT_TUSKMAUL},
    [FK_PEDLAR] = {"PEDLAR", 90, 8, 10, 32, 6, 36, 20, 16, 150, FF_GUARD | FF_FLYER | FF_CASTER, 0, 0},
    [FK_BONE] = {"BONEGUARD", 36, 10, 12, 40, 7, 40, 12, 34, 22, FF_GUARD | FF_NOPOISON, IT_BONESKULL, IT_BONE},
    [FK_SKINK] = {"SKINK", 32, 8, 10, 32, 6, 40, 14, 16, 20, FF_GUARD, IT_SAC, IT_SKINKB},
    [FK_EGG] = {"EGG", 34, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0},
    [FK_GRUB] = {"GRUB", 15, 3, 3, 12, 4, 12, 10, 14, 25, 0, 0, 0},
    [FK_CHITIN] = {"CHITIN", 28, 8, 10, 30, 6, 40, 14, 16, 20, FF_GUARD, IT_HEAD, 0},
    [FK_PUFF] = {"PUFFBALL", 20, 4, 4, 40, 6, 40, 10, 14, 20, FF_FLYER | FF_NOPOISON | FF_CASTER, 0, 0},
    [FK_COILER] = {"COILER", 42, 10, 12, 34, 7, 42, 14, 20, 20, FF_GUARD, IT_COILB, 0},
    [FK_PUPPET] = {"PUPPET", 18, 7, 6, 30, 6, 44, 20, 14, 20, FF_FLYER | FF_NOPOISON, 0, 0},
    [FK_MUMMER] = {"MUMMER", 38, 7, 8, 40, 6, 40, 14, 14, 45, FF_GUARD | FF_FLYER | FF_CASTER, IT_BALL, 0},
    [FK_GRASPER] = {"GRASPER", 34, 9, 10, 38, 6, 40, 30, 16, 20, FF_GUARD | FF_FLYER | FF_NOPOISON | FF_NOBURN, 0, 0},
    [FK_SILVER] = {"SILVER CHITIN", 46, 11, 12, 22, 6, 30, 22, 18, 30, FF_GUARD | FF_NOPOISON, 0, 0},
    [FK_GUARD] = {"REGENT'S GUARD", 22, 8, 10, 34, 6, 40, 14, 16, 0, FF_GUARD, 0, 0},
    [FK_WARDEN] = {"WRAITH WARDEN", 170, 10, 14, 34, 7, 40, 18, 18, 0, FF_GUARD | FF_FLYER | FF_BOSS | FF_NOPOISON, 0, 0},
    [FK_MATRON] = {"COMB MATRON", 130, 10, 10, 50, 6, 40, 0, 20, 0, FF_BOSS | FF_NOPOISON, 0, 0},
    [FK_CLAW] = {"CLAW", 55, 9, 12, 34, 7, 44, 0, 22, 0, FF_GUARD | FF_BOSS | FF_NOPOISON, 0, 0},
    [FK_REGENT] = {"PIT REGENT", 340, 12, 16, 36, 8, 40, 14, 22, 0,
                   FF_GUARD | FF_BOSS | FF_NOPOISON | FF_NOBURN | FF_NOFREEZE, 0, 0},
    [FK_STONE] = {"STONE", 40, 8, 8, 60, 6, 40, 16, 12, 0, FF_FLYER | FF_BOSS | FF_NOPOISON | FF_NOBURN | FF_NOFREEZE | FF_CASTER, 0, 0},
    [FK_BEETLE] = {"WILD BEETLE", 50, 11, 12, 24, 6, 32, 20, 18, 30, FF_GUARD | FF_NOPOISON, 0, 0},
};

/* Our own glyphs (not the original's): the strokes, and their mirror image
 * (LEFT and RIGHT swapped) works as well. ivw_15 checks they never clash. */
const IvwSpell IVW_SPELL[SP_COUNT] = {
    [SP_CHART] = {"CHART", "LDR", 4, "SHOWS WHAT YOU HAVE SEEN NEARBY."},
    [SP_OPEN] = {"OPEN", "URRD", 6, "A WALL WITH A DOOR'S OUTLINE BECOMES A DOOR."},
    [SP_MEND] = {"MEND", "RDRUR", 8, "HEALS A LITTLE."},
    [SP_LANCE] = {"LANCE", "UDU", 4, "A THRUST NO GUARD STOPS, DOWN THE WHOLE LINE."},
    [SP_KINDLE] = {"KINDLE", "URUR", 6, "SINGES AND SLOWS FOES AHEAD. LIGHTS TORCHES."},
    [SP_GALE] = {"GALE", "RURDRUR", 12, "BLOWS FOES AHEAD OUT OF THE FIGHT, OR FLAMES DOWN A CORRIDOR."},
    [SP_VEIL] = {"VEIL", "UURRDDLL", 14, "UNSEEN: WALK THROUGH FLAMES. FOES CANNOT GUARD."},
    [SP_MIRE] = {"MIRE", "DLDR", 8, "SLOWS EVERY FOE AHEAD."},
    [SP_RESTORE] = {"RESTORE", "RDRDRURU", 18, "HEALS A GREAT DEAL."},
    [SP_RIME] = {"RIME", "DDRRUULLD", 16, "FREEZES EVERY FOE: NO GUARD, NO BLOWS."},
    [SP_FLOAT] = {"FLOAT", "DRURDRUR", 12, "TEN TURNS OVER PITS, SLIME AND LAVA."},
    [SP_FLARE] = {"FLARE", "URDRURDRUR", 14, "FIRE AHEAD. CAST ON A FLAME, IT BURNS BEFORE THE FIGHT."},
    [SP_QUICKEN] = {"QUICKEN", "RRRU", 8, "MUCH QUICKER FEET IN A FIGHT."},
    [SP_THUNDER] = {"THUNDER", "RDLDRDLD", 22, "STRIKES EVERY FOE."},
    [SP_HUSH] = {"HUSH", "LLDRRDLL", 10, "NO FOE MAY CAST."},
    [SP_TWIN] = {"TWIN", "RRDLLDRRU", 18, "A SECOND KNIGHT STRIKES WITH YOU."},
    [SP_WARD] = {"WARD", "RDLUU", 12, "A SHELL THAT TAKES BLOWS, EVEN UNBLOCKABLE ONES."},
};

static void mirror(const char *s, char *out) {
    int i = 0;
    for (; s[i]; i++) out[i] = s[i] == 'L' ? 'R' : s[i] == 'R' ? 'L' : s[i];
    out[i] = 0;
}

int ivw_glyph_spell(const char *strokes) {
    if (!strokes || !strokes[0]) return -1;
    char m[IVW_GLYPH_MAX + 2];
    if (strlen(strokes) > IVW_GLYPH_MAX) return -1;
    mirror(strokes, m);
    for (int s = 0; s < SP_COUNT; s++)
        if (!strcmp(IVW_SPELL[s].glyph, strokes) || !strcmp(IVW_SPELL[s].glyph, m)) return s;
    return -1;
}

/* the Gaol's gem panels: 0 red, 1 green, 2 blue, as they start */
const uint8_t IVW_GEMSET[4][9] = {
    {0, 0, 1, 1, 0, 2, 0, 1, 0},   /* five red, three green, one blue */
    {1, 1, 1, 0, 0, 2, 0, 2, 0},   /* four red, three green, two blue */
    {2, 0, 0, 2, 0, 1, 1, 1, 1},   /* three red, four green, two blue */
    {0, 0, 0, 0, 1, 1, 2, 2, 2},
};

const char *const IVW_MA_NAME[MA_COUNT] = {
    [MA_INV] = "PACK", [MA_PASS] = "PASS TURN", [MA_OPEN] = "OPEN DOOR", [MA_CLOSE] = "CLOSE DOOR",
    [MA_UNLOCK] = "UNLOCK DOOR", [MA_LEVER] = "PULL LEVER", [MA_DOUSE] = "DOUSE TORCH", [MA_POKE] = "POKE FUNGUS",
    [MA_SMOTHER] = "SMOTHER FUNGUS", [MA_LANTERN] = "REACH INTO LIGHT", [MA_GEMS] = "TOUCH GEMS",
    [MA_SWITCH] = "PRESS SWITCH", [MA_STUDY] = "STUDY GLYPH", [MA_CHEST] = "OPEN CHEST", [MA_DRINK] = "DRINK",
    [MA_WELL_ITEM] = "THROW IN...", [MA_SIT] = "SIT ON THRONE", [MA_TRADE] = "TRADE", [MA_STEAL] = "STEAL",
    [MA_TALK] = "TALK", [MA_GIVE] = "GIVE...", [MA_ATTACK] = "ATTACK", [MA_SOCKET] = "SET STONES",
    [MA_PICKUP] = "PICK UP...", [MA_SEARCH] = "SEARCH ROOM",
};
