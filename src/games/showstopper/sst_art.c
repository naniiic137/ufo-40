/* SHOWSTOPPER - pixel art (palette-letter strings, see gfx.h). All drawn
 * for UFO 40. The bosses and the map's tiles are drawn in sst_draw.c. */
#include "sst.h"

Sprite sst_spr[SP_COUNT];

/* Clover, 14 x 16: a white show rabbit with a wine scarf and a wand */
static const char CLOVER_D0[] =
    "...kk....kk..."
    "..kwk....kwk.."
    "..kKk....kKk.."
    "..kKk....kKk.."
    "..kwk....kwk.."
    "...kwkkkkwk..."
    "..kwwwwwwwwk.."
    ".kwwkwwwwkwwk."
    ".kwwwwKKwwwwk."
    ".kwwwwwwwwwwk."
    "..kvvvvvvvvk.."
    ".kwkvvvvvvkwky"
    ".kwwkwwwwkwwbk"
    "..kwwwwwwwwkb."
    "..kwwk..kwwk.."
    "...kk....kk...";
static const char CLOVER_D1[] =
    "...kk....kk..."
    "..kwk....kwk.."
    "..kKk....kKk.."
    "..kKk....kKk.."
    "..kwk....kwk.."
    "...kwkkkkwk..."
    "..kwwwwwwwwk.."
    ".kwwkwwwwkwwk."
    ".kwwwwKKwwwwk."
    ".kwwwwwwwwwwk."
    "..kvvvvvvvvk.."
    ".kwkvvvvvvkwky"
    ".kwwkwwwwkwwbk"
    "..kwwwwwwwwkb."
    "...kwwk.kwwk.."
    "....kk...kk...";
static const char CLOVER_U0[] =
    "...kk....kk..."
    "..kwk....kwk.."
    "..kwk....kwk.."
    "..kwk....kwk.."
    "..kwk....kwk.."
    "...kwkkkkwk..."
    "..kwwwwwwwwk.."
    ".kwwwwwwwwwwk."
    ".kwwwwwwwwwwk."
    ".kwwwwwwwwwwk."
    "..kvvvvvvvvk.."
    "ykwkwwwwwwkwk."
    "kbwwwwwwwwwwk."
    ".bkwwwwlwwwk.."
    "..kwwk..kwwk.."
    "...kk....kk...";
static const char CLOVER_U1[] =
    "...kk....kk..."
    "..kwk....kwk.."
    "..kwk....kwk.."
    "..kwk....kwk.."
    "..kwk....kwk.."
    "...kwkkkkwk..."
    "..kwwwwwwwwk.."
    ".kwwwwwwwwwwk."
    ".kwwwwwwwwwwk."
    ".kwwwwwwwwwwk."
    "..kvvvvvvvvk.."
    "ykwkwwwwwwkwk."
    "kbwwwwwwwwwwk."
    ".bkwwwwlwwwk.."
    "..kwwk.kwwk..."
    "...kk...kk....";
static const char CLOVER_S0[] =
    "....kk........"
    "...kwk........"
    "...kKwk......."
    "....kKwk......"
    ".....kwk......"
    "....kkwwkk...."
    "...kwwwwwwk..."
    "..kwwwwwkwwk.."
    "..kwwwwwwwwKk."
    "..kwwwwwwwwk.."
    "...kvvvvvvk..."
    "..kwvvvvvvkbby"
    ".kwwwwwwwkk..."
    ".kwwwwwwwwk..."
    "..kwwk.kwwk..."
    "...kk...kk....";
static const char CLOVER_S1[] =
    "....kk........"
    "...kwk........"
    "...kKwk......."
    "....kKwk......"
    ".....kwk......"
    "....kkwwkk...."
    "...kwwwwwwk..."
    "..kwwwwwkwwk.."
    "..kwwwwwwwwKk."
    "..kwwwwwwwwk.."
    "...kvvvvvvk..."
    "..kwvvvvvvkbby"
    ".kwwwwwwwkk..."
    ".kwwwwwwwwk..."
    "...kwwkkwwk..."
    "....kk..kk....";
static const char CLOVER_DASH[] =
    ".............."
    "..........kk.."
    "........kkwk.."
    "......kkwKk..."
    "....kkwwkk...."
    "...kwwwwwwk..."
    "..kwwwwwkwwk.."
    ".kwwwwwwwwwKk."
    ".kwwwwwwwwwk.."
    "lkvvvvvvvvk..."
    "l.kvvvvvvkbby."
    "l.kwwwwwwwk..."
    "..kwwwwwwwk..."
    "...kkkkkkk...."
    ".............."
    "..............";

/* the bubble she comes back in, 18 x 18 */
static const char BUBBLE[] =
    "......IIIIII......"
    "....II......II...."
    "...I..........I..."
    "..I...ww.......I.."
    ".I...ww.........I."
    ".I..ww..........I."
    "I...w............I"
    "I................I"
    "I................I"
    "I................I"
    "I................I"
    "I................I"
    ".I..............I."
    ".I..............I."
    "..I............I.."
    "...I..........I..."
    "....II......II...."
    "......IIIIII......";

/* the helpers: the one-eyed blob and the stubby round one, 9 x 9 */
static const char OPT_A[] =
    "...kkk..."
    "..koook.."
    ".kowwwok."
    "koowkwook"
    "kooowoook"
    "koooooook"
    ".kaoooak."
    "..kaaak.."
    "...kkk...";
static const char OPT_B[] =
    "..kkkkk.."
    ".kooooak."
    "kowkowkok"
    "koKkoKkok"
    "kooooooak"
    "koorrrook"
    "kaoooooak"
    ".kaaaaak."
    "..kkkkk..";
static const char SPIKEBALL[] =
    "...C..."
    "..kCk.."
    ".kuBuk."
    "CuBwBuC"
    ".kuBuk."
    "..kCk.."
    "...C...";

/* cards, 9 x 11: a face for each colour */
static const char CARD_LIFE[] =
    "kkkkkkkkk"
    "kccccccck"
    "kcpp.ppck"
    "kcpppppck"
    "kcpppppck"
    "kccpppcck"
    "kcccpccck"
    "kcccpccck"
    "kccpppcck"
    "kccccccck"
    "kkkkkkkkk";
static const char CARD_SHOT[] =
    "kkkkkkkkk"
    "kccccccck"
    "kccjjjcck"
    "kcjjcccck"
    "kcjccccck"
    "kcjccccck"
    "kcjccccck"
    "kcjjcccck"
    "kccjjjcck"
    "kccccccck"
    "kkkkkkkkk";
static const char CARD_OPTION[] =
    "kkkkkkkkk"
    "kccccccck"
    "kcccoccck"
    "kcccoccck"
    "kcooooock"
    "kcccoccck"
    "kcccoccck"
    "kcccoccck"
    "kcccoccck"
    "kccccccck"
    "kkkkkkkkk";
static const char CARD_BOMB[] =
    "kkkkkkkkk"
    "kccccccck"
    "kccrrrcck"
    "kcrcccrck"
    "krcwkwcrk"
    "krcckccrk"
    "kcrcccrck"
    "kccrrrcck"
    "kccccccck"
    "kccccccck"
    "kkkkkkkkk";
static const char CARD_SHIELD[] =
    "kkkkkkkkk"
    "kccccccck"
    "kcccBccck"
    "kcccBccck"
    "kBBBBBBBk"
    "kccBBBcck"
    "kccBcBcck"
    "kcBcccBck"
    "kccccccck"
    "kccccccck"
    "kkkkkkkkk";
static const char CARD_BACK[] =
    "kkkkkkkkk"
    "kvvvvvvvk"
    "kvyvvvyvk"
    "kvvvyvvvk"
    "kvvyvyvvk"
    "kvyvvvyvk"
    "kvvyvyvvk"
    "kvvvyvvvk"
    "kvyvvvyvk"
    "kvvvvvvvk"
    "kkkkkkkkk";

/* a star pass, 11 x 9 */
static const char PASS[] =
    "kkkkkkkkkkk"
    "kyyyyyyyyyk"
    "kya.ayyayyk"
    "kyyyaaayyyk"
    "kay.aaa.yak"
    "kyyaayaayyk"
    "kya.ayyayyk"
    "kyyyyyyyyyk"
    "kkkkkkkkkkk";
static const char HEART[] =
    ".kk.kk."
    "krrkrrk"
    "krwrrrk"
    "krrrrrk"
    ".krrrk."
    "..krk.."
    "...k...";
static const char HEART_E[] =
    ".kk.kk."
    "kddkddk"
    "kdddddk"
    "kdddddk"
    ".kdddk."
    "..kdk.."
    "...k...";
static const char LIFE[] =
    ".kk.kk."
    ".kwkwk."
    ".kKkKk."
    "kkwwwkk"
    "kwkwkwk"
    "kwwKwwk"
    ".kkkkk.";

static const char CRATE[] =
    "kkkkkkkkkkkkkk"
    "kbtttttttttttk"
    "ktbeeeeeeeebtk"
    "ktebeeeeeebetk"
    "kteebeeeebeetk"
    "kteeebeebeeetk"
    "kteeeebbeeeetk"
    "kteeeebbeeeetk"
    "kteeebeebeeetk"
    "kteebeeeebeetk"
    "ktebeeeeeebetk"
    "ktbeeeeeeeebtk"
    "kbtttttttttttk"
    "kkkkkkkkkkkkkk";
static const char BARREL[] =
    "...kkkkkk..."
    "..kbtttttk.."
    ".kbtbbbbttk."
    ".kgggggggggk"
    "kbtbbbbbbttk"
    "kbtbbbbbbttk"
    "kbtbbbbbbttk"
    "kbtbbbbbbttk"
    "kgggggggggk."
    ".kbtbbbbttk."
    "..kbtttttk.."
    "...kkkkkk...";

/* ---- the show's runaways ---- */
static const char GUMDROP[] =
    "....kkkk...."
    "..kkiiiikk.."
    ".kiiiiiiiik."
    ".kiwkiiwkik."
    "kiiwkiiwkiik"
    "kiiiiiiiiiik"
    "kziiikkiiizk"
    "kzziiiiiizzk"
    ".kzzzzzzzzk."
    "..kkkkkkkk..";
static const char LANTERN[] =
    "...kkkk..."
    "....kk...."
    "..kkkkkk.."
    ".kayyyyak."
    ".kyywwyyk."
    ".kyywwyyk."
    ".kayyyyak."
    ".kaoyyoak."
    "..kkkkkk.."
    "...kbbk..."
    "...kbbk..."
    "...kbbk..."
    "..kbbbbk..";
static const char JESTER[] =
    ".kk....kk..."
    "kPPk..krrk.."
    ".kPPkkrrk..."
    "..kPwwwk...."
    "..kwkwkwk..."
    "..kwwrwwk..."
    "...kwwwk...."
    "..kPPkrrk..."
    ".kPPPkrrrk.."
    ".kPPPkrrrk.."
    "..kPk.krk..."
    "..kk...kk...";
static const char BAT[] =
    "k....kk....k"
    "kpk.kppk.kpk"
    "kppkpwpwkppk"
    ".kpppppppppk"
    "..kppkkppk.."
    "...kk..kk...";
static const char SNOWBALL[] =
    "...kkkk..."
    "..kwwwwk.."
    ".kwwwwwIk."
    "kwkwwkwwIk"
    "kwwwwwwwIk"
    "kwwwkkwwIk"
    ".kwwwwwIk."
    "..kIIIIk.."
    "...kkkk...";
static const char GOURD[] =
    "....kfk....."
    "...kkkkk...."
    "..koooook..."
    ".koyokyook.."
    ".kooookoook."
    ".koykkkyok.."
    "..koooook..."
    ".knnnnnnnk.."
    "knnnnnnnnnk."
    "knnnnnnnnnk."
    ".knnknnnk..."
    "..kk.kk....."
    ;
static const char PUFFER[] =
    "...k.kk.k..."
    "..kakaakak.."
    ".kaaaaaaaak."
    "kaawkaawkaak"
    "kaaaaaaaaaak"
    "kaaaarraaaak"
    ".kaaaaaaaak."
    "..kakaakak.."
    "...k.kk.k...";
static const char TOYTRAIN[] =
    "...kk.............."
    "...kgk..kkkkkkkk..."
    "..kkkkkkkrrrrrrrk.."
    ".kBBBBBBkrwwrwwrk.."
    "kBBBBBBBBrrrrrrrrk."
    "kBBBBBBBBBBBBBBBBBk"
    ".kkgkkkkgkkkkgkkk.."
    "..kk....kk...kk....";
static const char BOMBER[] =
    "...kkkk...."
    "..kbbbbk..."
    ".kbbbbbbk.."
    ".kttkttk..."
    ".kttttttk.."
    "..ktmmtk..."
    ".kgggggk.k."
    "kggggggkkrk"
    "kgkggggk.k."
    ".kgggggk..."
    "..kk.kk....";
static const char SNOWMAN[] =
    "....kkkk......"
    "...kwwwwk....."
    "..kwkwwkwk...."
    "..kwwwwwoook.."
    "..kwwwwwwk...."
    "...kwwwwk....."
    "..krrrrrrk...."
    ".kwwwwwwwwk..."
    "kwwwwkwwwwwk.."
    "kwwwwwwwwwwk.."
    "kwwwwkwwwwwk.."
    "kIwwwwwwwwIk.."
    ".kIIIIIIIIk..."
    "..kkkkkkkk....";
static const char ROCKETEER[] =
    "..kkkk......"
    ".kffffk....."
    ".ktkktk.kkk."
    ".kttttk.krrk"
    "..kffk.kgggk"
    ".kffffkgggk."
    "kffffffggk.."
    "kfkffffk...."
    ".kffffk....."
    ".kffffk....."
    "..kk.kk.....";
static const char COPTER[] =
    "..kkkkk..."
    ".kVVVVVk.."
    "kVwkVwkVk."
    "kVVVVVVVk."
    "kVVPPPVVk."
    ".kVVVVVk.."
    "..kkkkk...";
static const char TURTLE[] =
    "...kkkkkk...."
    "..kjjfjjfk..."
    ".kjfjjjjfjkk."
    "kjjjfjjfjjjhk"
    "kfjjjjjjjjfwk"
    "kjjfjjjjfjjhk"
    ".kjjjfjjjjkk."
    "..khkkkkhk..."
    "..kk....kk...";
static const char HARLEQUIN[] =
    ".kk....kk..."
    "kNNk..kPPk.."
    ".kNNkkPPk..."
    "..kwwwwwk..."
    "..kwkwkwk..."
    "..kwPwPwk..."
    "...kwwwk...."
    "..kNPkNPk..."
    ".kNPNkPNPk.."
    ".kPNPkNPNk.."
    ".kNPNkPNPk.."
    "..kNk.kPk..."
    "..kk...kk...";
static const char SKULLJET[] =
    "..kkkkkkkk...."
    ".kllllllllk..."
    "kllwwllwwllk.."
    "kllkkllkklllk."
    "klllllllllllk."
    ".kllllkllllk.."
    "..klklklklk..."
    "...kkkkkkk....";
static const char EMBER[] =
    "....r...."
    "...rar..."
    "..raya.r."
    ".rayyyar."
    "rayywyyar"
    "rayykyyar"
    ".rayyyar."
    "..raaar.."
    "...rrr...";
static const char BOOK[] =
    "kkkkkkkkkk"
    "kVVVVVVVwk"
    "kVyyyyVVwk"
    "kVVVVVVVwk"
    "kVyyyVVVwk"
    "kVVVVVVVwk"
    "kkkkkkkkkk";
static const char IMP[] =
    ".k......k."
    "kck....kck"
    "kcckkkkcck"
    ".kcccccck."
    ".kckcckck."
    ".kccrrcck."
    "..kcccck.."
    "...kkkk...";
static const char RACECAR[] =
    "...kkkkkkkk....."
    "..kiiiiiiiik...."
    ".kizzzuuzzzik..."
    "kiizzuuuuzziikk."
    "kiiiiiiiiiiiiiik"
    "kzkkzzzzzzzkkzzk"
    ".kgk.......kgk.."
    "..k.........k...";
static const char MOUSE[] =
    "..kk......"
    ".kggk.kkk."
    ".kgggkgggk"
    "kgggggggwk"
    "kggggggggk"
    ".kkggggkk."
    "..k.kk.yk.";
static const char SKULL[] =
    "..kkkkk..."
    ".klllllk.."
    "kllllllk.."
    "klkklkklk."
    "kllllllk.."
    ".klkklk..."
    "..kkkk....";
static const char EGG[] =
    "...kk..."
    "..kllk.."
    ".kllwlk."
    ".klllpk."
    "klllplk."
    "kllpllk."
    ".kllllk."
    "..kkkk..";
static const char SWOOP[] =
    "k....kk....k"
    "kvk.kvvk.kvk"
    "kvvkvyvykvvk"
    ".kvvvvvvvvvk"
    "..kvvkkvvk.."
    "...kk..kk...";
static const char SPIKE[] =
    "...k....k...."
    "..klk..klk..."
    "kkkkkkkkkkkkk"
    ".kssssssssk.."
    "kslllllllslk."
    ".kslsssslsk.."
    "kslsllllslsk."
    ".kslsssslsk.."
    "kslllllllslk."
    ".kssssssssk.."
    "kkkkkkkkkkkkk"
    "..klk..klk..."
    "...k....k....";

/* hidden spots: something a little out of place */
static const char SECRET_SNOW[] =
    "...kkkk..."
    "..kwwwwk.."
    "..kwkkwk.."
    "...kwwk..."
    "..kwwwwk.."
    ".kwwwwwwk."
    ".kwwwwwwk."
    "..kkkkkk..";
static const char SECRET_TOWN[] =
    "..kkkkkk.."
    ".kmrrrrmk."
    "..kmmmmk.."
    "..kzzzzk.."
    ".kzizziik."
    "..kzzzzk.."
    "..kbbbbk.."
    "..kkkkkk..";
static const char SECRET_PALACE[] =
    "....kk...."
    "...kyyk..."
    "..kyccyk.."
    "...kyyk..."
    "...kaak..."
    "..kaaaak.."
    ".kaaaaaak."
    ".kkkkkkkk.";
static const char SECRET_TRAIN[] =
    "..kkkkk..."
    ".kgggggk.."
    ".kgsssgk.."
    ".kgsssgk.."
    ".kgggggk.."
    "..kgggk..."
    "..kgkgk..."
    "..kk.kk...";

/* Jelly, the Hop's blob, 12 x 10, and the Hop's coins */
static const char BLOB[] =
    "....kkkk...."
    "..kkKKKKkk.."
    ".kKKwKKwKKk."
    ".kKKkKKkKKk."
    "kKKKKKKKKKKk"
    "kKKKPPPPKKKk"
    "kPKKKKKKKKPk"
    "kPPKKKKKKPPk"
    ".kPPPPPPPPk."
    "..kkkkkkkk..";
static const char BLOB2[] =
    "............"
    "....kkkk...."
    "..kkKKKKkk.."
    ".kKKwKKwKKk."
    "kKKKkKKkKKKk"
    "kKKKKKKKKKKk"
    "kPKKPPPPKKPk"
    "kPPKKKKKKPPk"
    "kkPPPPPPPPkk"
    ".kkkkkkkkkk.";
static const char COIN[] =
    ".kkk."
    "kyayk"
    "kyayk"
    "kyayk"
    ".kkk.";
static const char BIGCOIN[] =
    "..kkk.."
    ".kyyyk."
    "kyywayk"
    "kyyaayk"
    "kyyaayk"
    ".kyyyk."
    "..kkk..";
static const char GLOVE[] =
    "..kk.kk...."
    ".kwwkwwkk.."
    ".kwwkwwkwk."
    "kkwwwwwwwk."
    "kwkwwwwwwwk"
    "kwwwwwwwwwk"
    ".kwwwwwwwk."
    "..kvvvvvk.."
    "..kkkkkkk..";
static const char TORCHF[] =
    "...r...."
    "..rar..."
    ".rayar.."
    ".rywyr.."
    "..ryr..."
    ".kbbbk.."
    "..kbk..."
    "..kbk...";
static const char BOMB[] =
    "....ky.."
    "...kk..."
    "..kkkk.."
    ".knnnnk."
    "knwnnnnk"
    "knnnnnnk"
    ".knnnnk."
    "..kkkk..";

typedef struct { int id, w, h; const char *px; } ArtDef;
static const ArtDef ART[] = {
    {SP_CLOVER_D0, 14, 16, CLOVER_D0}, {SP_CLOVER_D1, 14, 16, CLOVER_D1}, {SP_CLOVER_U0, 14, 16, CLOVER_U0},
    {SP_CLOVER_U1, 14, 16, CLOVER_U1}, {SP_CLOVER_S0, 14, 16, CLOVER_S0}, {SP_CLOVER_S1, 14, 16, CLOVER_S1},
    {SP_CLOVER_DASH, 14, 16, CLOVER_DASH}, {SP_BUBBLE, 18, 18, BUBBLE}, {SP_OPT_A, 9, 9, OPT_A}, {SP_OPT_B, 9, 9, OPT_B},
    {SP_SPIKEBALL, 7, 7, SPIKEBALL}, {SP_CARD0, 9, 11, CARD_LIFE}, {SP_CARD0 + 1, 9, 11, CARD_SHOT},
    {SP_CARD0 + 2, 9, 11, CARD_OPTION}, {SP_CARD0 + 3, 9, 11, CARD_BOMB}, {SP_CARD0 + 4, 9, 11, CARD_SHIELD},
    {SP_CARD_BACK, 9, 11, CARD_BACK}, {SP_PASS, 11, 9, PASS}, {SP_HEART, 7, 7, HEART}, {SP_HEART_E, 7, 7, HEART_E},
    {SP_LIFE, 7, 7, LIFE}, {SP_CRATE, 14, 14, CRATE}, {SP_BARREL, 12, 12, BARREL}, {SP_GUMDROP, 12, 10, GUMDROP},
    {SP_LANTERN, 10, 13, LANTERN}, {SP_JESTER, 12, 12, JESTER}, {SP_BAT, 12, 6, BAT}, {SP_SNOWBALL, 10, 9, SNOWBALL},
    {SP_GOURD, 12, 12, GOURD}, {SP_PUFFER, 12, 9, PUFFER}, {SP_TOYTRAIN, 19, 8, TOYTRAIN}, {SP_BOMBER, 11, 11, BOMBER},
    {SP_SNOWMAN, 14, 14, SNOWMAN}, {SP_ROCKETEER, 12, 11, ROCKETEER}, {SP_COPTER, 10, 7, COPTER},
    {SP_TURTLE, 13, 9, TURTLE}, {SP_HARLEQUIN, 12, 13, HARLEQUIN}, {SP_SKULLJET, 14, 8, SKULLJET}, {SP_EMBER, 9, 9, EMBER},
    {SP_BOOK, 10, 7, BOOK}, {SP_IMP, 10, 8, IMP}, {SP_RACECAR, 16, 8, RACECAR}, {SP_MOUSE, 10, 7, MOUSE},
    {SP_SKULL, 10, 7, SKULL}, {SP_EGG, 8, 8, EGG}, {SP_SWOOP, 12, 6, SWOOP}, {SP_SPIKE, 13, 13, SPIKE},
    {SP_SECRET_SNOW, 10, 8, SECRET_SNOW}, {SP_SECRET_TOWN, 10, 8, SECRET_TOWN}, {SP_SECRET_PALACE, 10, 8, SECRET_PALACE},
    {SP_SECRET_TRAIN, 10, 8, SECRET_TRAIN}, {SP_BLOB, 12, 10, BLOB}, {SP_BLOB2, 12, 10, BLOB2}, {SP_COIN, 5, 5, COIN},
    {SP_BIGCOIN, 7, 7, BIGCOIN}, {SP_GLOVE, 11, 9, GLOVE}, {SP_TORCHF, 8, 8, TORCHF}, {SP_BOMB, 8, 8, BOMB},
};

int sst_art_check(void) {
    /* tests: every sprite string has exactly w * h pixels, and every id is drawn */
    int bad = 0;
    bool seen[SP_COUNT] = {false};
    for (int i = 0; i < ARRAY_LEN(ART); i++) {
        if ((int)strlen(ART[i].px) != ART[i].w * ART[i].h) bad++;
        for (const char *c = ART[i].px; *c; c++)
            if (*c != '.' && pal_char_index(*c) < 0) { bad++; break; }
        seen[ART[i].id] = true;
    }
    for (int i = 0; i < SP_COUNT; i++) bad += !seen[i];
    return bad;
}

void sst_art_load(void) {
    static bool done;
    if (done) return;
    done = true;
    for (int i = 0; i < ARRAY_LEN(ART); i++)
        if ((int)strlen(ART[i].px) == ART[i].w * ART[i].h) spr_make(&sst_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
}
