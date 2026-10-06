/* BRAVADO - pixel art (palette-letter strings, see gfx.h). All drawn for
 * UFO 40: Dice the fox in her flight jacket, the Glass Pit's monsters and
 * the shop's gear. The arena, the lava and the Pit Boss are drawn in
 * bravado_draw.c. */
#include "bravado.h"

Sprite brv_spr[SP_COUNT];

/* Dice, 10 x 12: front, back and side (facing right), two steps each */
static const char DICE_D[] =
    ".kk....kk."
    "kook..kook"
    "koookkoook"
    "kooooooook"
    "kowkookwok"
    "kocckkccok"
    ".kcccccck."
    "krrrkkrrrk"
    "krrryyrrrk"
    "kcrrrrrrck"
    ".kbbkkbbk."
    ".kkk..kkk.";
static const char DICE_D2[] =
    ".kk....kk."
    "kook..kook"
    "koookkoook"
    "kooooooook"
    "kowkookwok"
    "kocckkccok"
    ".kcccccck."
    "krrrkkrrrk"
    "krrryyrrrk"
    "kcrrrrrrck"
    ".kbbk.kbk."
    ".kkk...kk.";
static const char DICE_U[] =
    ".kk....kk."
    "kook..kook"
    "koookkoook"
    "kooooooook"
    "kooooooook"
    "kooooooook"
    ".kooooook."
    "krrrrrrrrk"
    "krrrrrrrrk"
    "kcrrrrrrck"
    ".kbbkkbbk."
    ".kkk..kkk.";
static const char DICE_U2[] =
    ".kk....kk."
    "kook..kook"
    "koookkoook"
    "kooooooook"
    "kooooooook"
    "kooooooook"
    ".kooooook."
    "krrrrrrrrk"
    "krrrrrrrrk"
    "kcrrrrrrck"
    ".kbk.kbbk."
    ".kk...kkk.";
static const char DICE_S[] =
    "...kk....."
    "..kook...."
    ".kooookkk."
    ".kooowkook"
    ".kooooocck"
    "..koocccck"
    "..kkcccck."
    ".krrrrrk.."
    "kotrrrrrk."
    ".kkrrrrck."
    "..kbbkbk.."
    "..kkk.kk..";
static const char DICE_S2[] =
    "...kk....."
    "..kook...."
    ".kooookkk."
    ".kooowkook"
    ".kooooocck"
    "..koocccck"
    "..kkcccck."
    ".krrrrrk.."
    "kotrrrrrk."
    ".kkrrrrck."
    ".kbk.kbbk."
    ".kk...kkk.";

/* a mite, 8 x 6 */
static const char MITE1[] =
    "..kkkk.."
    ".kooook."
    "kowkowak"
    "kooaaook"
    ".kkkkkk."
    "k.k..k.k";
static const char MITE2[] =
    "..kkkk.."
    ".kooook."
    "kowkowak"
    "kooaaook"
    ".kkkkkk."
    ".k.kk.k.";

/* gasbags: big 14 x 10, middling 10 x 8, small 6 x 5 */
static const char GAS0[] =
    "....kkkkkk...."
    "..kkVVVVVVkk.."
    ".kVVVVVVVVVVk."
    "kVVwwVVVVVVVVk"
    "kVVwkVVVVVpVVk"
    "kVVVVVVVVppVVk"
    "kpVVVVVVVVVVpk"
    ".kppVVVVVVppk."
    "..kkppppppkk.."
    "....kkkkkk....";
static const char GAS1[] =
    "...kkkk..."
    ".kkVVVVkk."
    "kVwwVVVVVk"
    "kVwkVVVpVk"
    "kpVVVVVVpk"
    ".kppVVppk."
    "..kkppkk.."
    "....kk....";
static const char GAS2[] =
    ".kkkk."
    "kVwVVk"
    "kVkVpk"
    "kpVVpk"
    ".kkkk.";

/* a brute, 12 x 14 */
static const char BRUTE1[] =
    "...kkkkkk..."
    "..kbbbbbbk.."
    ".kbbrbbrbbk."
    ".kbbbbbbbbk."
    ".kwbttttbwk."
    "..kwtkktwk.."
    ".kkbttttbkk."
    "kbbkbbbbkbbk"
    "kbbbebbebbbk"
    "kttbeeeebttk"
    ".kkbbbbbbkk."
    "..kbbkkbbk.."
    "..kbbkkbbk.."
    "..kkk..kkk..";
static const char BRUTE2[] =
    "...kkkkkk..."
    "..kbbbbbbk.."
    ".kbbrbbrbbk."
    ".kbbbbbbbbk."
    ".kwbttttbwk."
    "..kwtkktwk.."
    ".kkbttttbkk."
    "kbbkbbbbkbbk"
    "kbbbebbebbbk"
    "kttbeeeebttk"
    ".kkbbbbbbkk."
    "..kbbk.kbbk."
    ".kbbk..kbbk."
    ".kkk....kkk.";

/* a powder keg, 10 x 10 */
static const char KEG[] =
    "......yk.."
    ".....rk..."
    "..kkkkkk.."
    ".kaaaaaak."
    "kbbbbbbbbk"
    "kawkaakwak"
    "kaakaakaak"
    "kbbbbbbbbk"
    ".kaaaaaak."
    "..kkkkkk..";

/* a stilter, 12 x 14 */
static const char STILT1[] =
    "....kkk....."
    "...kuuuk...."
    "..kuwkuukkk."
    "..kuuuuuyyyk"
    "..kuuuuukkk."
    ".kBuuuuuk..."
    "kBBBuuuBk..."
    ".kBBBBBk...."
    "..kk.kk....."
    "..kl.kl....."
    "..kl..kl...."
    "..kl..kl...."
    ".kl....kl..."
    ".kk....kk...";
static const char STILT2[] =
    "....kkk....."
    "...kuuuk...."
    "..kuwkuukkk."
    "..kuuuuuyyyk"
    "..kuuuuukkk."
    ".kBuuuuuk..."
    "kBBBuuuBk..."
    ".kBBBBBk...."
    "..kk.kk....."
    "..kl.kl....."
    "..kl.kl....."
    "...klkl....."
    "...kllk....."
    "...kkkk.....";

/* a peeper up, 12 x 12, and its closed bud, 12 x 8 */
static const char PEEP[] =
    "...kkkkkk..."
    "..kiiiiiik.."
    ".kiwwwwwwik."
    "kiwwwkkwwwik"
    "kiwwkkkkwwik"
    "kiwwkkkkwwik"
    "kiwwwkkwwwik"
    ".kiwwwwwwik."
    "..kiiiiiik.."
    ".kzk.kk.kzk."
    "kzzzkffkzzzk"
    ".kkkkkkkkkk.";
static const char PEEP_BUD[] =
    "....kkkk...."
    "...kiiiik..."
    "..kiizziik.."
    "..kizzzzik.."
    "...kiiiik..."
    ".kzk.kk.kzk."
    "kzzzkffkzzzk"
    ".kkkkkkkkkk.";

/* a fizzer, 8 x 8 */
static const char FIZZ[] =
    "....wy.."
    "...ak..."
    "..kkkk.."
    ".kyyyyk."
    "kyykkyyk"
    "kyyyyyyk"
    ".kyaayk."
    "..kkkk..";

static const char BOMB[] =
    "....yr."
    "...k..."
    "..kkk.."
    ".kssgk."
    "kssslgk"
    "kssssgk"
    ".kssgk."
    "..kkk..";

static const char MEDKIT[] =
    ".kkkkkkk."
    "kwwwwwwwk"
    "kwwwrwwwk"
    "kwwrrrwwk"
    "kwwwrwwwk"
    "kwwwwwwwk"
    "klllllllk"
    ".kkkkkkk.";

static const char DRONE[] =
    "k.....k"
    ".kgggk."
    "kglCCgk"
    "kgCCCgk"
    ".kgggk."
    "..k.k..";

/* the gear, 12 x 12 each, in GR_ order */
static const char ICONS[GR_COUNT + 1][145] = {
    /* heart plate */
    "............"
    ".kkkkkkkkkk."
    "kllkkllkkllk"
    "klkrrkkrrklk"
    "klkrrrrrrklk"
    "klkrrwrrrklk"
    "klkkrrrrkklk"
    "kllkkrrkklk."
    "klllkkkklllk"
    "kggggggggggk"
    ".kkkkkkkkkk."
    "............",
    /* first aid */
    "............"
    "....kkkk...."
    "...kk..kk..."
    ".kkkkkkkkkk."
    "kwwwwrrwwwwk"
    "kwwwwrrwwwwk"
    "kwwrrrrrrwwk"
    "kwwrrrrrrwwk"
    "kwwwwrrwwwwk"
    "kwwwwrrwwwwk"
    ".kkkkkkkkkk."
    "............",
    /* blast guard */
    "............"
    ".kkkkkkkkkk."
    ".kBBBBBBBBk."
    ".kBBayBaBBk."
    ".kBaayyaaBk."
    ".kByyoyyBBk."
    ".kBBaoyaBBk."
    "..kBayaBBk.."
    "..kBBBBBBk.."
    "...kBBBBk..."
    "....kBBk...."
    ".....kk.....",
    /* shot guard */
    "............"
    ".kkkkkkkkkk."
    ".kjjjjjjjjk."
    ".kjjjkkjjjk."
    ".kjjklgkjjk."
    ".kjjklgkjjk."
    ".kjjklgkjjk."
    "..kjkkkkjk.."
    "..kjjjjjjk.."
    "...kjjjjk..."
    "....kjjk...."
    ".....kk.....",
    /* quick trigger */
    "............"
    "............"
    "..kkkkkkkkk."
    ".kgggggggglk"
    "kggllllllgk."
    "kgkkkkgkkk.."
    "kgk.kgk....."
    "kgkkgk..y.y."
    "kggk...y.y.."
    ".kk.....y.y."
    "............"
    "............",
    /* heavy rounds */
    "............"
    "....kkk....."
    "...kyyyk...."
    "..kyyyyyk..."
    "..kyywyyk..."
    "..kyywyyk..."
    "..kyyyyyk..."
    "..kaaaaak..."
    "..kbbbbbk..."
    "..kaaaaak..."
    "..kkkkkkk..."
    "............",
    /* fan fire */
    "............"
    "..kk....kk.."
    "..ky....yk.."
    "...k.kk.k..."
    "....kyyk...."
    "......k....."
    ".....k......"
    "....kgk....."
    "...kgggk...."
    "..kgggggk..."
    "..kkkkkkk..."
    "............",
    /* ricochet */
    "............"
    "k..........."
    "kk....k....."
    "k.k..kyk...."
    "k..kky.yk..."
    "k...y...yk.."
    "k........yk."
    "k.........yk"
    "k..........k"
    "kkkkkkkkkkkk"
    "kggggggggggk"
    "............",
    /* kickback */
    "............"
    "............"
    "...kkkkk...."
    "..kcccccck.."
    ".kcckckcck.."
    ".kcccccccku."
    ".kcckkkkck.u"
    ".kcccccck.u."
    "..kkcccck..."
    "...kkkkk...."
    "............"
    "............",
    /* buddy bot */
    "............"
    ".k........k."
    "..k......k.."
    "...kkkkkk..."
    "..kgggggk..."
    ".kglCCCglk.."
    ".kgCCwCCgk.."
    ".kglCCCglk.."
    "..kgggggk..."
    "...kkkkkk..."
    "...k....k..."
    "............",
    /* firewalkers */
    "...r..o....."
    "..ror.oy...."
    "..oyo.oyr..."
    "...kkkkk...."
    "...kbbbk...."
    "...kbbbk...."
    "...kbbbkkk.."
    "..kbbbbbbbk."
    "..kbbbbbbbk."
    "..kkkkkkkkk."
    "..oyoyoyoyo."
    "............",
    /* rocket dash */
    "............"
    "......kkk..."
    ".....kook..."
    "....kooook.."
    "uu..krrrk..."
    ".uukrrrrrk.."
    "uu..krrrk..."
    "..u.kbkbk..."
    "...kbk.kbk.."
    "...kk...kk.."
    "............"
    "............",
    /* bomb bag */
    "............"
    "...kkkkkk..."
    "..kttttttk.."
    "...kttttk..."
    "..kttttttk.."
    ".kttkkkkttk."
    ".ktksslkttk."
    ".ktksssktbk."
    ".kttkkkkttk."
    ".kttttttttk."
    "..kkkkkkkk.."
    "............",
    /* clicker */
    "......r....."
    "......k....."
    "......k....."
    "....kkkkk..."
    "....kgggk..."
    "....krrgk..."
    "....krrgk..."
    "....kgggk..."
    "....kglgk..."
    "....kgggk..."
    "....kkkkk..."
    "............",
    /* nail bombs */
    "............"
    ".l...l...l.."
    "..l..l..l..."
    "...kkkkk...."
    "llkssssglll."
    "..ksssslk..."
    "llksssssglll"
    "..kssssgk..."
    "...kkkkk...."
    "..l..l..l..."
    ".l...l...l.."
    "............",
    /* bait bombs */
    "............"
    "..yyyy......"
    ".yayayy....."
    ".yyyyyy....."
    ".....k......"
    "....kkk....."
    "...kssgk...."
    "..kssslgk..."
    "..ksssssk..."
    "...kssgk...."
    "....kkk....."
    "............",
    /* slag: three more pools */
    "............"
    "............"
    "....kkkk...."
    "..kkoyyokk.."
    ".kooyyyyook."
    "kroooyyooork"
    "krroooooorrk"
    ".krrrrrrrrk."
    "..kkkkkkkk.."
    "......o....."
    ".....oyo...."
    "............",
};

void brv_art_load(void) {
    static bool done;
    if (done) return;
    done = true;
    spr_make(&brv_spr[SP_DICE_D], 10, 12, DICE_D);
    spr_make(&brv_spr[SP_DICE_D2], 10, 12, DICE_D2);
    spr_make(&brv_spr[SP_DICE_U], 10, 12, DICE_U);
    spr_make(&brv_spr[SP_DICE_U2], 10, 12, DICE_U2);
    spr_make(&brv_spr[SP_DICE_S], 10, 12, DICE_S);
    spr_make(&brv_spr[SP_DICE_S2], 10, 12, DICE_S2);
    spr_make(&brv_spr[SP_MITE1], 8, 6, MITE1);
    spr_make(&brv_spr[SP_MITE2], 8, 6, MITE2);
    spr_make(&brv_spr[SP_GAS0], 14, 10, GAS0);
    spr_make(&brv_spr[SP_GAS1], 10, 8, GAS1);
    spr_make(&brv_spr[SP_GAS2], 6, 5, GAS2);
    spr_make(&brv_spr[SP_BRUTE1], 12, 14, BRUTE1);
    spr_make(&brv_spr[SP_BRUTE2], 12, 14, BRUTE2);
    spr_make(&brv_spr[SP_KEG], 10, 10, KEG);
    spr_make(&brv_spr[SP_STILT1], 12, 14, STILT1);
    spr_make(&brv_spr[SP_STILT2], 12, 14, STILT2);
    spr_make(&brv_spr[SP_PEEP], 12, 12, PEEP);
    spr_make(&brv_spr[SP_PEEP_BUD], 12, 8, PEEP_BUD);
    spr_make(&brv_spr[SP_FIZZ], 8, 8, FIZZ);
    spr_make(&brv_spr[SP_BOMB], 7, 8, BOMB);
    spr_make(&brv_spr[SP_MEDKIT], 9, 8, MEDKIT);
    spr_make(&brv_spr[SP_DRONE], 7, 6, DRONE);
    for (int i = 0; i <= GR_COUNT; i++) spr_make(&brv_spr[SP_GEAR0 + i], 12, 12, ICONS[i]);
}
