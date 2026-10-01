/* BUZZBOLT - pixel art (palette-letter strings, see gfx.h). All drawn for
 * UFO 40. The bosses, spore-rocks and walls are drawn in buzzbolt_draw.c. */
#include "buzzbolt.h"

Sprite bzz_spr[SP_COUNT];

/* the Hive Wing's three ships, 15 x 13 */
static const char LACEWING[] =
    ".......w......."
    "......wIw......"
    "......IuI......"
    "..l...IuI...l.."
    ".lCl.kIuIk.lCl."
    "lCIClkuBuklCICl"
    "lCICCluBulCCICl"
    ".lCIClIuIlCICl."
    "..lCCkIuIkCCl.."
    "...lIk.u.kIl..."
    "....l.kuk.l...."
    "......IuI......"
    ".......C.......";
static const char SHIELDBUG[] =
    "......kkk......"
    "....kkjijkk...."
    "...kjziiizjk..."
    "..kjzzfifzzjk.."
    ".kjzfzzjzzfzjk."
    ".kzfjzjwjzjfzk."
    "kjzfjzjkjzjfzjk"
    "kzzfjzjkjzjfzzk"
    "kjzffzjkjzffzjk"
    ".kjzfzjkjzfzjk."
    "..kjzfjkjfzjk.."
    "...kjjfkfjjk..."
    "....kk.y.kk....";
static const char FIREFLY[] =
    ".......k......."
    "......kvk......"
    ".....kvrvk....."
    "..k..krark..k.."
    ".kok.krark.kok."
    "kooakvrarvkaook"
    "kaoorrvavrrooak"
    ".kaorvryrvroak."
    "..kkkvayavkkk.."
    ".....kyayk....."
    "......yay......"
    ".......y......."
    "......y.y......";

/* options, 7 x 7 */
static const char OPT_LACE[] =
    "...w..."
    "..IuI.."
    ".lCuCl."
    "lCIuICl"
    ".lCBCl."
    "..lul.."
    "...l...";
static const char OPT_SHIELD[] =
    "..kkk.."
    ".kjzjk."
    "kjzizjk"
    "kzjwjzk"
    "kjzizjk"
    ".kjzjk."
    "..kkk..";
static const char OPT_FIRE[] =
    "...k..."
    "..kok.."
    ".korok."
    "koryrok"
    ".korok."
    "..kak.."
    "...y...";

/* the Blight's bugs */
static const char GNAT1[] =
    "wl.....lw"
    ".wl.k.lw."
    "..lkgkl.."
    "..kgrgk.."
    "...kgk..."
    "..k.k.k.."
    ".........";
static const char GNAT2[] =
    "........."
    "....k...."
    "..kkgkk.."
    "wlkgrgklw"
    "wl.kgk.lw"
    "..k.k.k.."
    ".........";
static const char MIDGE1[] =
    "l.....l"
    ".l.k.l."
    "..kPk.."
    "..PKP.."
    "...P..."
    "..k.k..";
static const char MIDGE2[] =
    "......."
    "...k..."
    "lkkPkkl"
    "l.PKP.l"
    "...P..."
    "..k.k..";
static const char WHIRL1[] =
    "..........."
    "..........."
    "hht.....thh"
    ".hhtt.tthh."
    "....tkt...."
    "...kbebk..."
    "...kebek..."
    "....kbk...."
    ".....k....."
    "..........."
    "...........";
static const char WHIRL2[] =
    "...h......."
    "...hh......"
    "....ht....."
    ".....tt...."
    "....tkt...."
    "...kbebk..."
    "...kebek..."
    "....kbk...."
    ".....ktt..."
    "......thh.."
    "........h..";
static const char CRICKET[] =
    "k...........k"
    ".k.........k."
    "..k..kkk..k.."
    "...kkfjfkk..."
    "....kjrjk...."
    "..kkfjzjfkk.."
    ".k.kfzjzfk.k."
    "k..kfjzjfk..k"
    "...kffjffk..."
    "..k.kfffk.k.."
    ".k...kfk...k."
    "k.....k.....k"
    ".............";
static const char BLISTER[] =
    "...kkkkk..."
    "..kmmmmmk.."
    ".kvrryrrvk."
    "kvryarrrrvk"
    "kvrrrrryavk"
    "kvrarrrrrvk"
    "kvyarrryrvk"
    ".kvrrrrrvk."
    "..kvvvvvk.."
    ".k.kkkkk.k."
    "k.........k";
static const char GOLD1[] =
    "Il.......lI"
    ".Il.kkk.lI."
    "..lkyyykl.."
    "..kyaycyk.."
    "..kayyyak.."
    "...kaaak..."
    "...kyayk..."
    "....kak...."
    "....k.k....";
static const char GOLD2[] =
    "..........."
    "....kkk...."
    "...kyyyk..."
    "IlkyaycyklI"
    ".lkayyyakl."
    "...kaaak..."
    "...kyayk..."
    "....kak...."
    "....k.k....";
/* the Ironback, 27 x 19, drawn upside down (it faces the ship) */
static const char IRONBACK[] =
    "......k.............k......"
    ".....klk...........klk....."
    "......klk.........klk......"
    ".......kgkkkkkkkkkgk......."
    "......kgsgggsgsgggsgk......"
    ".....kgsdsgggrgggsdsgk....."
    "..k.kgsdddsgsksgsdddsgk.k.."
    ".k.kgsdsdsdsgkgsdsdsdsgk.k."
    "k..kgsdddddsgkgsdddddsgk..k"
    "...kgsdsdsdsgkgsdsdsdsgk..."
    "..kkgsdddddsgkgsdddddsgkk.."
    ".k.kgsdsdsdsgkgsdsdsdsgk.k."
    "k..kgsdddddsgkgsdddddsgk..k"
    "...kgssdddssgkgssdddssgk..."
    "....kgssdssgkkkgssdssgk...."
    "......kggsggkkkggsggk......"
    ".......kkkkk...kkkkk......."
    "......k.k.k.....k.k.k......"
    ".....k..k...........k..k...";
/* the firefly's dragonfly, 21 x 9 */
static const char DRAGONFLY[] =
    "..IIIII.....IIIII...."
    ".IlllllI...IlllllI..."
    "..IlllllI.IlllllI...."
    "kKPKPKPKPKPPPPPkkk..."
    ".kkkkkkkkkPPwPPkwk..."
    "..IlllllI.IlllllI...."
    ".IlllllI...IlllllI..."
    "..IIIII.....IIIII...."
    ".....................";
static const char BOMB[] =
    "..kkk.."
    ".kBuBk."
    "kBuIuBk"
    "kuIwIuk"
    "kBuIuBk"
    ".kBuBk."
    "..kkk..";

void bzz_art_load(void) {
    if (bzz_spr[SP_LACEWING].px) return;
    static const struct { int id, w, h; const char *px; } ART[] = {
        {SP_LACEWING, 15, 13, LACEWING}, {SP_SHIELDBUG, 15, 13, SHIELDBUG}, {SP_FIREFLY, 15, 13, FIREFLY},
        {SP_OPT_LACE, 7, 7, OPT_LACE}, {SP_OPT_SHIELD, 7, 7, OPT_SHIELD}, {SP_OPT_FIRE, 7, 7, OPT_FIRE},
        {SP_GNAT1, 9, 7, GNAT1}, {SP_GNAT2, 9, 7, GNAT2}, {SP_MIDGE1, 7, 6, MIDGE1}, {SP_MIDGE2, 7, 6, MIDGE2},
        {SP_WHIRL1, 11, 11, WHIRL1}, {SP_WHIRL2, 11, 11, WHIRL2}, {SP_CRICKET, 13, 13, CRICKET},
        {SP_BLISTER, 11, 11, BLISTER}, {SP_GOLD1, 11, 9, GOLD1}, {SP_GOLD2, 11, 9, GOLD2},
        {SP_IRONBACK, 27, 19, IRONBACK}, {SP_DRAGONFLY, 21, 9, DRAGONFLY},
        {SP_BOMB, 7, 7, BOMB},
    };
    for (int i = 0; i < ARRAY_LEN(ART); i++) spr_make(&bzz_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
}
