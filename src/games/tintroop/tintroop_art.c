/* TIN TROOP - pixel art (palette-letter strings, see gfx.h). All drawn for UFO 40. */
#include "tintroop.h"

Sprite tt_spr[T_SPRITE_COUNT];

static const char SOLDIER1[] =
    "...rr..."
    "..kNNk.."
    "..kNyk.."
    ".kkkkkk."
    "..khhk.."
    ".kBwBBk."
    ".hBBwBh."
    "..kBBk.."
    "..kNNk.."
    "..kk.kk.";
static const char SOLDIER2[] =
    "...rr..."
    "..kNNk.."
    "..kNyk.."
    ".kkkkkk."
    "..khhk.."
    ".kBwBBk."
    ".hBBwBk."
    "..kBBkh."
    ".kN..Nk."
    ".kk..kk.";
static const char SOLDIER3[] =
    "...rr..."
    "..kNNk.."
    "..kNyk.."
    ".kkkkkk."
    "..khhk.."
    ".kBwBBk."
    ".kBBwBh."
    ".hkBBk.."
    "...kNk.."
    "...kk...";
static const char SOLDIER_JUMP[] =
    "...rr..."
    "..kNNk.."
    "..kNyk.."
    ".kkkkkk."
    "h.khhk.h"
    "kkBwBBkk"
    "..BBwB.."
    "..kBBk.."
    ".kNk.kN."
    ".kk...k.";
static const char SOLDIER_CHUTE[] =
    ".l.rr.l."
    ".lkNNkl."
    ".lkNykl."
    ".kkkkkk."
    "hkkhhkkh"
    ".kBwBBk."
    "..BBwB.."
    "..kBBk.."
    "..kNNk.."
    "..kk.kk.";
static const char CHUTE[] =
    "....kkkkkkkk...."
    "..kkrrwwrrwwkk.."
    ".krrrwwwrrwwwrk."
    "krrrrwwwrrwwwrrk"
    "kkkkkkkkkkkkkkkk"
    ".l............l."
    "..l..........l.."
    "...l........l..."
    "....l......l...."
    ".....l....l.....";
static const char SOLDIER_SWIM[] =
    "...rr..."
    "..kNNk.."
    "..kNyk.."
    ".kkkkkk."
    "..khhk.."
    "hkBwBBkh"
    "..BBwB.."
    "..kBBk.."
    ".kN..Nk."
    "k......k";
static const char SOLDIER_CLIMB[] =
    "...rr..."
    "..kNNk.."
    "..kNNk.."
    ".kkkkkk."
    "h.kNNk.."
    "kkBBBBk."
    ".kBwwBk."
    "..kBBkh."
    "..kNNk.."
    "..kk.k..";
static const char ARROW[] =
    ".......kkkk..."
    "kk..kkkBBkNNk."
    "kNNkBBwBBkhhkl"
    "kNNkBBBwBkhhkw"
    "kk..kkkBBkkkk."
    "..............";
static const char LODGED[] =
    "...kkkk..."
    "kk.kBBkNNk"
    "kNkBwBkhhw"
    "kNkBBwkhhw"
    "kk.kkkkkk.";
static const char CORPSE[] =
    ".........."
    ".....kkk.."
    "kk.kkBBkrr"
    "kNkBwBBkNk"
    "kkkkkkkkkk";
static const char STONE[] =
    ".kkkkkkkk."
    "kgggllggsk"
    "kglgggggsk"
    "kggggggssk"
    "kgggkkggsk"
    "kggkssksgk"
    "kgggggggsk"
    "kgsgggggsk"
    "kssssssssk"
    ".kkkkkkkk.";
static const char BLIMP[] =
    ".......kkkkkkkkkkkk......."
    "....kkkggglllllllgggkk...."
    "..kkggglllwwwwwwlllgggkk.."
    ".kgggllllllllllllllllgggk."
    "kggrrrrrrrrrrrrrrrrrrrrggk"
    "kgglllllllllllllllllllgggk"
    ".kgggllllllllllllllllgggk."
    "..kkggggggggggggggggggkk.."
    "....kkkkkgggggggkkkkkk...."
    "........kbbbbbbbbk........"
    "........kbtbtbtbbk........"
    ".........kkkkkkkk.........";
static const char MOUSE1[] =
    "..kk......"
    ".kgglk...."
    "kgggggkkk."
    "kkgggggggk"
    ".kggggggk."
    "..k.k.kk..";
static const char MOUSE2[] =
    "..kk......"
    ".kgglk...."
    "kgggggkkk."
    "kkgggggggk"
    ".kggggggkk"
    "...kk.k...";
static const char DART1[] =
    "k........."
    "kwk......."
    "kwwlkk...."
    "kkwwwwlkkk"
    "..kkkkkk..";
static const char DART2[] =
    ".........."
    "kk........"
    "kwwlkk...."
    "kkwwwwlkkk"
    "..kkkkkk..";
static const char RAM1[] =
    ".kk......."
    "kyyk.kkkk."
    "kykkklllk."
    ".kaaklllkk"
    ".kaaalllkk"
    "..kaalllk."
    "..kaaaaak."
    "..kakkakk."
    "..kk..kk.."
    "..........";
static const char RAM2[] =
    ".kk......."
    "kyyk.kkkk."
    "kykkklllk."
    ".kaaklllkk"
    ".kaaalllkk"
    "..kaalllk."
    "..kaaaaak."
    "..kkakkak."
    "...kk..kk."
    "..........";
static const char RAM_CHARGE[] =
    ".........."
    ".kk..kkkk."
    "kyykklllk."
    "kykaaklllk"
    ".kaaaallkk"
    "..kaaalllk"
    "..kaaaaaak"
    ".kak..kak."
    ".kk....kk."
    "..........";
static const char FISH1[] =
    ".........."
    "...kkkk..."
    ".kkoyooykk"
    "kyooooookk"
    ".kkoooookk"
    "...kkkk.k.";
static const char FISH2[] =
    ".........."
    "...kkkk..."
    ".kkoyooyk."
    "kyoooooook"
    ".kkooooook"
    "...kkkk...";
static const char BUG1[] =
    "..kkkk.."
    ".kgssgk."
    "kgsgsgsk"
    "ksgsgsgk"
    "kgsgsgsk"
    ".kkkkkk."
    "..k..k..";
static const char BUG2[] =
    "..kkkk.."
    ".kgsgsk."
    "ksgsgsgk"
    "kgsgsgsk"
    "ksgsgsgk"
    ".kkkkkk."
    ".k..k...";
static const char DRAGON1[] =
    "....kk......"
    "...kjjk....."
    "..kjijjkk..."
    ".kjjjjjjjk.."
    "kwkjjjjjjk.."
    "kkkkjjjjk..."
    "...kjjzjjk.."
    "..kjjzzjjk.."
    "..kjzzzjjjk."
    "..kjjzzjjjjk"
    "..kjjjjjkkjk"
    "..kkjjjjk.kk"
    "...kjk.kjk.."
    "...kk...kk..";
static const char DRAGON2[] =
    "....kk......"
    "...kjjk....."
    "..kjijjkk..."
    ".kjjjjjjjk.."
    "kokjjjjjjk.."
    "kykkjjjjk..."
    "koo.kjjzjjk."
    "..kjjjzzjjk."
    "..kjzzzjjjk."
    "..kjjzzjjjjk"
    "..kjjjjjkkjk"
    "..kkjjjjk.kk"
    "...kjk.kjk.."
    "...kk...kk..";
static const char FLAME1[] =
    "...y...."
    "..yay..."
    "..yaoy.."
    ".yaoory."
    ".yaooay."
    "..yaay.."
    "...kk...";
static const char FLAME2[] =
    "....y..."
    "...yay.."
    "..yaoay."
    ".yaooay."
    ".yaoory."
    "..yaay.."
    "...kk...";
static const char SEED[] =
    ".zz."
    "zijz"
    "zjjz"
    ".zz.";
static const char POT[] =
    "..zj..jz.."
    ".zijzzjiz."
    "..zjzzjz.."
    "kkkkkkkkkk"
    "koooooooek"
    ".koooooek."
    ".koeoooek."
    ".kooooeek."
    "..kooeek.."
    "..kkkkkk..";
static const char TAG[] =
    ".kkkkkkkk."
    "kyyyyyyyak"
    "kyccccccak"
    "kyccccccak"
    "kyccccccak"
    "kyccccccak"
    "kyccccccak"
    "kaaaaaaaak"
    ".kkkkkkkk.";
static const char EXIT[] =
    "....kk.............."
    "....kwrrrrrk........"
    "....kwrrrrrrrk......"
    "....kwryyrrrrrk....."
    "....kwrrrrrrk......."
    "....kwrrrrk........."
    "....kw.............."
    "....kw.............."
    "....kw.............."
    "....kw.............."
    "....kw.............."
    "....kw.............."
    "....kw.............."
    "..kkkkkk............"
    "..kttttk............"
    "..kkkkkk............"
    "...................."
    "...................."
    "...................."
    "....................";
static const char DOOR[] =
    ".kkkkkkkkkk."
    "kbttttttttbk"
    "kbtbbbbbbtbk"
    "kbtbeeeebtbk"
    "kbtbeeeebtbk"
    "kbtbeeeebtbk"
    "kbtbeeyebtbk"
    "kbtbeeeebtbk"
    "kbtbeeeebtbk"
    "kbtbeeeebtbk"
    "kbtbeeeebtbk"
    "kbtbeeeebtbk"
    "kbttttttttbk"
    "kkkkkkkkkkkk"
    "............"
    "............";
static const char VINE[] =
    "..zj......"
    "..jz......"
    "...jz....."
    "...zj.....";
static const char SWITCH_UP[] =
    ".kkkkkkkk."
    "kaayyyyaak"
    "kkkkkkkkkk"
    "kbbbbbbbbk";
static const char SWITCH_DOWN[] =
    ".........."
    ".kkkkkkkk."
    "kkaayyaakk"
    "kbbbbbbbbk";
static const char GATE[] =
    "kssssssk.."
    "kglllgsk.."
    "kssssssk.."
    "kglllgsk.."
    "kssssssk.."
    "kglllgsk.."
    "kssssssk.."
    "kglllgsk.."
    "kssssssk.."
    "kglllgsk..";
static const char JACK_BOX[] =
    "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
    "kPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPk"
    "kPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPk"
    "kVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVk"
    "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbetetttetttetttetttetttetttetttettebbk"
    "kbbetetttetttetttetttetttetttetttettebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbetttttttttttttttyttttttttttttttttebbk"
    "kbbetttttttttttttttyttttttttttttttttebbk"
    "kbbettttttttttttttyaytttttttttttttttebbk"
    "kbbettttttttttttttyaytttttttttttttttebbk"
    "kbbetttttttyyyyyyyaaayyyyyyyttttttttebbk"
    "kbbetttttttttyyaaaaaaaaayyttttttttttebbk"
    "kbbetttttttttttyaaaaaaayttttttttttttebbk"
    "kbbettttttttttttyaaaaaytttttttttttttebbk"
    "kbbetttttttttttyaaayaaayttttttttttttebbk"
    "kbbettttttttttyaaytttyaaytttttttttttebbk"
    "kbbetttttttttyaytttttttyayttttttttttebbk"
    "kbbetttttttttytttttttttttyttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbetetttetttetttetttetttetttetttettebbk"
    "kbbetetttetttetttetttetttetttetttettebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kbbettttttttttttttttttttttttttttttttebbk"
    "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
    "kbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbk"
    ".kbbk..............................kbbk."
    "..kk................................kk.."
    "........................................";
static const char JACK_HEAD[] =
    ".....kkkkkkkk......."
    "...kkrrrrrrrrkk....."
    "..krryyrrrryyrrk...."
    ".krrrrrrrrrrrrrrk..."
    ".kkkkkkkkkkkkkkkk..."
    "..kwwwwwwwwwwwwk...."
    ".kwwkkwwwwwwkkwwk..."
    ".kwwkwkwwwwkwkwwk..."
    ".kwwwwwwrrwwwwwwk..."
    ".kKwwwwwrrwwwwwKk..."
    ".kKwkkkkkkkkkkwKk..."
    "..kwkrrrrrrrrkwk...."
    "...kwkkkkkkkkwk....."
    "....kkwwwwwwkk......"
    "......kkkkkk........";
static const char JACK_MOUTH[] =
    "..kkkkkk"
    ".kmmmmmk"
    "kmmrrrmk"
    "kmrrrrmk"
    "kmmrrrmk"
    ".kmmmmmk"
    "..kkkkkk";
static const char HEAD_FLAME[] =
    ".kkkkkkkkkkkkkkkkkk."
    "kttttttttttttttttttk"
    "ktkkkkkkkkkkkkkkkktk"
    "ktkrrrrrrrrrrrrrrktk"
    "ktkkkkkkkkkkkkkkkktk"
    "kthhhhhhhhhhhhhhhhtk"
    "kthhkkhhhhhhhhhhhhtk"
    "kthkwykhhhhhhhhhhhtk"
    "kthhkkhhhhhhhhhhhhtk"
    "kthhhhhhhhhhhheeehtk"
    "kthhhhhhhhhhhhhhhhtk"
    "kthkkkkkkkkkkkhhhhtk"
    "kthkoyoyoyoyokhhhhtk"
    "kthkyoyoyoyoykhhhhtk"
    "kthkkkkkkkkkkkhhhhtk"
    "kthhhhhhhhhhhhhhhhtk"
    "kttttttttttttttttttk"
    "ktbtbtbtbtbtbtbtbttk"
    "kttttttttttttttttttk"
    ".kkkkkkkkkkkkkkkkkk.";
static const char HEAD_SEED[] =
    ".kkkkkkkkkkkkkkkkkk."
    "kttttttttttttttttttk"
    "ktkkkkkkkkkkkkkkkktk"
    "ktkjjjjjjjjjjjjjjktk"
    "ktkkkkkkkkkkkkkkkktk"
    "kthhhhhhhhhhhhhhhhtk"
    "kthhkkhhhhhhhhhhhhtk"
    "kthkwzkhhhhhhhhhhhtk"
    "kthhkkhhhhhhhhhhhhtk"
    "kthhhhhhhhhhhheeehtk"
    "kthhhhhhhhhhhhhhhhtk"
    "kthkkkkkkkkkkkhhhhtk"
    "kthkzizizizizkhhhhtk"
    "kthkizizizizikhhhhtk"
    "kthkkkkkkkkkkkhhhhtk"
    "kthhhhhhhhhhhhhhhhtk"
    "kttttttttttttttttttk"
    "ktbtbtbtbtbtbtbtbttk"
    "kttttttttttttttttttk"
    ".kkkkkkkkkkkkkkkkkk.";
static const char HEAD_BUG[] =
    ".kkkkkkkkkkkkkkkkkk."
    "kttttttttttttttttttk"
    "ktkkkkkkkkkkkkkkkktk"
    "ktkssssssssssssssktk"
    "ktkkkkkkkkkkkkkkkktk"
    "kthhhhhhhhhhhhhhhhtk"
    "kthhkkhhhhhhhhhhhhtk"
    "kthkwgkhhhhhhhhhhhtk"
    "kthhkkhhhhhhhhhhhhtk"
    "kthhhhhhhhhhhheeehtk"
    "kthhhhhhhhhhhhhhhhtk"
    "kthkkkkkkkkkkkhhhhtk"
    "kthkgsgsgsgsgkhhhhtk"
    "kthksgsgsgsgskhhhhtk"
    "kthkkkkkkkkkkkhhhhtk"
    "kthhhhhhhhhhhhhhhhtk"
    "kttttttttttttttttttk"
    "ktbtbtbtbtbtbtbtbttk"
    "kttttttttttttttttttk"
    ".kkkkkkkkkkkkkkkkkk.";
static const char ORB[] =
    "..kk.."
    ".kPPk."
    "kPwKPk"
    "kPKPPk"
    ".kPPk."
    "..kk..";
static const char PIP_BIG[] =
    "........rrrr............"
    ".......rryyrr..........."
    "........rrrr............"
    ".......kkkkkkk.........."
    "......kNNNNNNNk........."
    "......kNNNNNNNk........."
    "......kNNyyyNNk........."
    "......kNNyayNNk........."
    "......kNNyyyNNk........."
    "......kNNNNNNNk........."
    ".....kkkkkkkkkkk........"
    "......khhhhhhhk........."
    "......khkhhhkhk........."
    "......khhhhhhhk........."
    "......khhbbbhhk........."
    ".......khhhhhk.........."
    ".....kkBBBBBBBkk........"
    "....kBBwBBBBBwBBk......."
    "....kBBBwBBBwBBBk......."
    "...khBBBBwBwBBBBhk......"
    "...khBByBBwBByBBhk......"
    "...kkBBBBwBwBBBBkk......"
    ".....kBBwBBBwBBk........"
    ".....kBBBBBBBBBk........"
    ".....kNNNNkNNNNk........"
    ".....kNNNNkNNNNk........"
    ".....kNNNk.kNNNk........"
    ".....kkkkk.kkkkk........";

typedef struct { int id, w, h; const char *px; } Art;
static const Art ART[] = {
    {T_SOLDIER1, 8, 10, SOLDIER1},
    {T_SOLDIER2, 8, 10, SOLDIER2},
    {T_SOLDIER3, 8, 10, SOLDIER3},
    {T_SOLDIER_JUMP, 8, 10, SOLDIER_JUMP},
    {T_SOLDIER_CHUTE, 8, 10, SOLDIER_CHUTE},
    {T_CHUTE, 16, 10, CHUTE},
    {T_SOLDIER_SWIM, 8, 10, SOLDIER_SWIM},
    {T_SOLDIER_CLIMB, 8, 10, SOLDIER_CLIMB},
    {T_ARROW, 14, 6, ARROW},
    {T_LODGED, 10, 5, LODGED},
    {T_CORPSE, 10, 5, CORPSE},
    {T_STONE, 10, 10, STONE},
    {T_BLIMP, 26, 12, BLIMP},
    {T_MOUSE1, 10, 6, MOUSE1},
    {T_MOUSE2, 10, 6, MOUSE2},
    {T_DART1, 10, 5, DART1},
    {T_DART2, 10, 5, DART2},
    {T_RAM1, 10, 10, RAM1},
    {T_RAM2, 10, 10, RAM2},
    {T_RAM_CHARGE, 10, 10, RAM_CHARGE},
    {T_FISH1, 10, 6, FISH1},
    {T_FISH2, 10, 6, FISH2},
    {T_BUG1, 8, 7, BUG1},
    {T_BUG2, 8, 7, BUG2},
    {T_DRAGON1, 12, 14, DRAGON1},
    {T_DRAGON2, 12, 14, DRAGON2},
    {T_FLAME1, 8, 7, FLAME1},
    {T_FLAME2, 8, 7, FLAME2},
    {T_SEED, 4, 4, SEED},
    {T_POT, 10, 10, POT},
    {T_TAG, 10, 9, TAG},
    {T_EXIT, 20, 20, EXIT},
    {T_DOOR, 12, 16, DOOR},
    {T_VINE, 10, 4, VINE},
    {T_SWITCH_UP, 10, 4, SWITCH_UP},
    {T_SWITCH_DOWN, 10, 4, SWITCH_DOWN},
    {T_GATE, 10, 10, GATE},
    {T_JACK_BOX, 40, 50, JACK_BOX},
    {T_JACK_HEAD, 20, 15, JACK_HEAD},
    {T_JACK_MOUTH, 8, 7, JACK_MOUTH},
    {T_HEAD_FLAME, 20, 20, HEAD_FLAME},
    {T_HEAD_SEED, 20, 20, HEAD_SEED},
    {T_HEAD_BUG, 20, 20, HEAD_BUG},
    {T_ORB, 6, 6, ORB},
    {T_PIP_BIG, 24, 28, PIP_BIG},
};

void tt_art_load(void) {
    static int loaded;
    if (loaded) return;
    for (int i = 0; i < ARRAY_LEN(ART); i++) spr_make(&tt_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
    loaded = 1;
}
