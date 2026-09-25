/* PETAL PARADE - pixel art (palette-letter strings, see gfx.h). All drawn for UFO 40. */
#include "petalparade.h"

Sprite pp_spr[P_SPRITE_COUNT];

static const char LINA_DOWN1[] =
    "...kkkkkk..."
    "..kyyyyyyk.."
    ".kyrrrrrryk."
    "kaayyyyyyaak"
    ".kkbhhhhbkk."
    "..kbkhhkbk.."
    "..kbhKKhbk.."
    "..kkuwwukk.."
    ".kuuwwwwuuk."
    ".khuwwwwuhk."
    "..kuuuuuuk.."
    "..kbk...kk..";
static const char LINA_DOWN2[] =
    "...kkkkkk..."
    "..kyyyyyyk.."
    ".kyrrrrrryk."
    "kaayyyyyyaak"
    ".kkbhhhhbkk."
    "..kbkhhkbk.."
    "..kbhKKhbk.."
    "..kkuwwukk.."
    ".kuuwwwwuuk."
    ".khuwwwwuhk."
    "..kuuuuuuk.."
    "..kk...kbk..";
static const char LINA_UP1[] =
    "...kkkkkk..."
    "..kyyyyyyk.."
    ".kyyyyyyyyk."
    "kaayrrrryaak"
    ".kkbbbbbbkk."
    "..kbbbbbbk.."
    "..kbbttbbk.."
    "..kkuttukk.."
    ".kuuuuuuuuk."
    ".khuuwwuuhk."
    "..kuuuuuuk.."
    "..kbk...kk..";
static const char LINA_UP2[] =
    "...kkkkkk..."
    "..kyyyyyyk.."
    ".kyyyyyyyyk."
    "kaayrrrryaak"
    ".kkbbbbbbkk."
    "..kbbbbbbk.."
    "..kbbttbbk.."
    "..kkuttukk.."
    ".kuuuuuuuuk."
    ".khuuwwuuhk."
    "..kuuuuuuk.."
    "..kk...kbk..";
static const char LINA_SIDE1[] =
    "...kkkkk...."
    "..kyyyyyk..."
    "..kyrrrrrk.."
    "kaayyyyyyaak"
    ".kkbbhhhhkk."
    "..kbbhhkhk.."
    "..kbbhhhhk.."
    "...kkuwwkk.."
    "..kuuuwwhk.."
    "..kuuuwwwk.."
    "..kuuuuuk..."
    "..kbk.kbk...";
static const char LINA_SIDE2[] =
    "...kkkkk...."
    "..kyyyyyk..."
    "..kyrrrrrk.."
    "kaayyyyyyaak"
    ".kkbbhhhhkk."
    "..kbbhhkhk.."
    "..kbbhhhhk.."
    "...kkuwwkk.."
    "..kuuuwwk..."
    "..khuuwwwk.."
    "..kuuuuuk..."
    "...kbbk.....";
static const char LINA_HOP[] =
    "...kkkkkk..."
    "..kyyyyyyk.."
    ".kyrrrrrryk."
    "kaayyyyyyaak"
    "hkkbhhhhbkkh"
    "ukkbkhhkbkku"
    "uk.kbhhbk.ku"
    ".kkkuwwukkk."
    "..kuuwwwuk.."
    "..kuuuuuuk.."
    "..kbkkkkbk.."
    "............";
static const char LINA_FALL[] =
    "............"
    "............"
    "............"
    "..kkk......."
    ".kyyyk......"
    "kyrrryk....."
    "kaayyyakkk.."
    ".kkhhkkuuwk."
    "..khkhkuuwwk"
    "..kkhhkuuuuk"
    "...kkkkbkbk."
    "............";
static const char PUP1[] =
    ".kk....kk."
    "kPKk..kKPk"
    "kPKkkkkKPk"
    ".kwwwwwwk."
    "kwwkwwkwwk"
    "kwwwkkwwwk"
    "kKwwwwwwKk"
    ".kwccccwk."
    ".kwwkkwwk."
    "..kk..kk..";
static const char PUP2[] =
    "kk......kk"
    "kPKk..kKPk"
    ".kKkkkkKk."
    ".kwwwwwwk."
    "kwwkwwkwwk"
    "kwwwkkwwwk"
    "kKwwwwwwKk"
    ".kwccccwk."
    ".kwwkkwwk."
    ".kk....kk.";
static const char PUP_TRAIL[] =
    ".kk....kk."
    "kPKk..kKPk"
    "kPKkkkkKPk"
    ".kwwwwwwk."
    "kwkwwwwkwk"
    "kwwwkkwwwk"
    "kKwwkkwwKk"
    ".krrrrrrk."
    ".kwwkkwwk."
    "..kk..kk..";
static const char BRAMBLE1[] =
    "k........k"
    "vk.k..k.kv"
    "kvkvkkvkvk"
    ".kvvvvvvk."
    "kvyyvvyyvk"
    "jvkyvvykvj"
    "kvvvvvvvvk"
    ".vkwkkwkv."
    "kmvkvvkvmk"
    ".k.k..k.k.";
static const char BRAMBLE2[] =
    ".k......k."
    "kvk.kk.kvk"
    ".kvkvvkvk."
    "kvvvvvvvvk"
    "vvyyvvyyvv"
    "kvkyvvykvk"
    "jvvvvvvvvj"
    "kvkwkkwkvk"
    ".mvvkkvvm."
    "k.k....k.k";
static const char BRAMBLE_DAZED[] =
    "k........k"
    "vk.k..k.kv"
    "kvkvkkvkvk"
    ".kvvvvvvk."
    "kvkkvvkkvk"
    "jvvvvvvvvj"
    "kvvvkkvvvk"
    ".vvkvvkvv."
    "kmvvvvvvmk"
    ".k.k..k.k.";
static const char TOAD1[] =
    "....kkkk...."
    "..kkVVVVkk.."
    ".kVVcVVVVVk."
    "kVccVVVVcVVk"
    "kVVVVVcVVVVk"
    "kpVVVVVVVVpk"
    ".kppppppppk."
    "..kkkkkkkk.."
    "...kcccck..."
    "...kckkck..."
    "...kccclk..."
    "..kkkkkkkk..";
static const char TOAD2[] =
    "....kkkk...."
    "..kkVVVVkk.."
    ".kVVVVVcVVk."
    "kVcVVVVVVcck"
    "kVVVcVVVVVVk"
    "kpVVVVVVVVpk"
    ".kppppppppk."
    "..kkkkkkkk.."
    "...kcccck..."
    "...kckkck..."
    "...kcclck..."
    "..kkkkkkkk..";
static const char JAR[] =
    "...kkkk..."
    "...kbbk..."
    "...kttk..."
    "..kkkkkk.."
    ".kwllllgk."
    ".kwrrrrvk."
    "kwrrrrrrvk"
    "kwrwrrrrvk"
    "kwrrrrrrvk"
    ".kvrrrrvk."
    "..kkkkkk..";
static const char WITCH[] =
    "..............kk........"
    ".............kVk........"
    "............kVpk........"
    "...........kVppk........"
    "..........kVVppk........"
    ".........kVVpppk.kk....."
    "........kVVppppkkzik...."
    ".......kVVpppppkzjk....."
    "......kKKKKKKKKKKk......"
    "..kkkkVVVVVVVVVVVVkkkk.."
    ".kVVVVpppppppppppppVVVk."
    "..kkkkllgkhhhhkgllkkkk.."
    "....kllkhhkhhkhhkllk...."
    "....kglkhhhheehhkglk...."
    "....kglkhhkkkkhhkglk...."
    ".....kgkkhhhhkkgk......."
    "......kkVVkhhkVVkk......"
    ".....kVVVVVKKVVVVVk....."
    "....kVVpVVVVVVVVpVVk...."
    "...khVpVVVVVVVVVVpVhk..."
    "...kkVppVVVVVVVVppVkk..."
    "....kVpppVVVVVVpppVk...."
    "....kVppppVVVVppppVk...."
    "...kVVppppppppppppVVk..."
    "...kkkkkkkkkkkkkkkkkk..."
    ".....kbk......kbk.......";
static const char WITCH_CACKLE[] =
    "..............kk........"
    ".............kVk........"
    "............kVpk........"
    "...........kVppk........"
    "..........kVVppk........"
    ".........kVVpppk.kk....."
    "........kVVppppkkzik...."
    ".......kVVpppppkzjk....."
    "......kKKKKKKKKKKk......"
    "..kkkkVVVVVVVVVVVVkkkk.."
    ".kVVVVpppppppppppppVVVk."
    "..kkkkllgkhhhhkgllkkkk.."
    "....kllkhhyhhyhhkllk...."
    "....kglkhhhheehhkglk...."
    "....kglkhkrrrrkhkglk...."
    ".....kgkkhkkkkhkgk......"
    "..hk..kkVVkhhkVVkk..kh.."
    "..kVk.kVVVVKKVVVVVk.kVk."
    "...kVkVVpVVVVVVVVpVkVk.."
    "....kVVpVVVVVVVVVVpVk..."
    "....kVppVVVVVVVVppVk...."
    "....kVpppVVVVVVpppVk...."
    "....kVppppVVVVppppVk...."
    "...kVVppppppppppppVVk..."
    "...kkkkkkkkkkkkkkkkkk..."
    ".....kbk......kbk.......";
static const char LINA_BIG[] =
    "........kkkkkkkk........"
    "......kkyyyyyyyykk......"
    ".....kyyyyaayyyyyyk....."
    "....kyyayyyyyyyayyyk...."
    "....krrrrrrrrrrrrrrk...."
    "..kkyyyyyyyyyyyyyyyykk.."
    ".kaayyayyyyyyyyyyayyaak."
    "..kkkkkkkkkkkkkkkkkkkk.."
    "....kbbbhhhhhhhhbbbk...."
    "...kbbbhhhhhhhhhhbbbk..."
    "...kbbhhkkhhhhkkhhbbk..."
    "...kbbhhkwhhhhkwhhbbk..."
    "...kbbhhkkhhhhkkhhbbk..."
    "...kbbKKhhhhhhhhKKbbk..."
    "....kbbhhhhkkhhhhbbk...."
    "....kbbbhhhhhhhhbbbk...."
    "...ktbkkkhhhhhhkkkbtk..."
    "...ktbkuuukhhkuuukbtk..."
    "....kkuuuwwwwwwuuukk...."
    "...kuuuuwwwwwwwwuuuuk..."
    "..khuuuuwwwKKwwwuuuuhk.."
    "..khkuuuwwwwwwwwuuukhk.."
    "...kkuuuwwwwwwwwuuukk..."
    "....kuuuwwwwwwwwuuuk...."
    "....kuuuuwwwwwwuuuuk...."
    "....kuuuuuuuuuuuuuuk...."
    ".....kkkhhkkkkhhkkk....."
    ".....kbbbk....kbbbk.....";
static const char HEART[] =
    ".kk.kk."
    "kKKkKrk"
    "kKKKKrk"
    ".kKKrk."
    "..kKk.."
    "...k...";
static const char PETAL[] =
    ".KK.."
    "KPPK."
    ".KPPK"
    "..KK.";

typedef struct { int id, w, h; const char *px; } Art;
static const Art ART[] = {
    {P_LINA_DOWN1, 12, 12, LINA_DOWN1},
    {P_LINA_DOWN2, 12, 12, LINA_DOWN2},
    {P_LINA_UP1, 12, 12, LINA_UP1},
    {P_LINA_UP2, 12, 12, LINA_UP2},
    {P_LINA_SIDE1, 12, 12, LINA_SIDE1},
    {P_LINA_SIDE2, 12, 12, LINA_SIDE2},
    {P_LINA_HOP, 12, 12, LINA_HOP},
    {P_LINA_FALL, 12, 12, LINA_FALL},
    {P_PUP1, 10, 10, PUP1},
    {P_PUP2, 10, 10, PUP2},
    {P_PUP_TRAIL, 10, 10, PUP_TRAIL},
    {P_BRAMBLE1, 10, 10, BRAMBLE1},
    {P_BRAMBLE2, 10, 10, BRAMBLE2},
    {P_BRAMBLE_DAZED, 10, 10, BRAMBLE_DAZED},
    {P_TOAD1, 12, 12, TOAD1},
    {P_TOAD2, 12, 12, TOAD2},
    {P_JAR, 10, 11, JAR},
    {P_WITCH, 24, 26, WITCH},
    {P_WITCH_CACKLE, 24, 26, WITCH_CACKLE},
    {P_LINA_BIG, 24, 28, LINA_BIG},
    {P_HEART, 7, 6, HEART},
    {P_PETAL, 5, 4, PETAL},
};

void pp_art_load(void) {
    static int loaded;
    if (loaded) return;
    for (int i = 0; i < ARRAY_LEN(ART); i++) spr_make(&pp_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
    loaded = 1;
}
