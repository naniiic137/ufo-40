/* BANNERFALL - pixel art (palette-letter strings, see gfx.h).
 * Everything faces right and wears the Marigold Guard's gold; the Thistle
 * Host is the same art flipped and remapped to violet (BF_TEAM_MAP). */
#include "bannerfall.h"

Sprite bf_spr[BS_SPRITE_COUNT];
uint8_t BF_TEAM_MAP[2][PAL_COUNT];

static const char FOOT1[] =
    ".............kk."
    "......kkkkk.klk."
    ".....kllwllkklk."
    "....kllllllgklk."
    "....kgggggggklk."
    ".....khhhkhkklk."
    ".....khhhhhkklk."
    "....kkoooookkak."
    "...kaaooooookhk."
    "...kaooooooorkk."
    "...kakoooooork.."
    "....kkooooorkk.."
    ".....kbbkbbk...."
    ".....kbk.kbk...."
    ".....kbk.kbk...."
    "....kkkk.kkkk...";
static const char FOOT2[] =
    "................"
    ".............kk."
    "......kkkkk.klk."
    ".....kllwllkklk."
    "....kllllllgklk."
    "....kgggggggklk."
    ".....khhhkhkklk."
    ".....khhhhhkklk."
    "....kkoooookkak."
    "...kaaooooookhk."
    "...kaooooooorkk."
    "...kakoooooork.."
    "....kkooooorkk.."
    "....kbk..kbk...."
    "....kbk...kbk..."
    "...kkkk...kkkk..";

static const char BOW1[] =
    "................"
    "......kkkk...k.."
    ".....kooook.ltk."
    "....kooaoookl.tk"
    "....kohhkhokl.tk"
    "....kohhhhokl.tk"
    ".....kooook.l.tk"
    "....kkooookkl.tk"
    "...kaooooohhhhtk"
    "...kaooooookl.tk"
    "...kakoooorkl.tk"
    "....kkooorkkltk."
    ".....kbbkbbk.tk."
    ".....kbk.kbk.k.."
    ".....kbk.kbk...."
    "....kkkk.kkkk...";
static const char BOW2[] =
    "................"
    "......kkkk...k.."
    ".....kooook.ltk."
    "....kooaoookl.tk"
    "....kohhkhokl.tk"
    "....kohhhhokl.tk"
    ".....kooook.l.tk"
    "....kkooookkl.tk"
    "...kaooooohhhhtk"
    "...kaooooookl.tk"
    "...kakoooorkl.tk"
    "....kkooorkkltk."
    ".....kbbkbbk.tk."
    "....kbk..kbk.k.."
    "....kbk...kbk..."
    "...kkkk...kkkk..";

static const char WARD1[] =
    "................"
    "....kkkkk......."
    "...kllwllk......"
    "...klllllkkkkkk."
    "...kgggggkllllk."
    "....khhkhkllllk."
    "....khhhhklaalk."
    "...kkooookaooak."
    "..kaoooookaooak."
    "..kaoooooklaalk."
    "..kakooookllllk."
    "...kkooorkglllk."
    "....kbbkbkggglk."
    "....kbk.kggggk.."
    "....kbk.kkkkkk.."
    "...kkkk.........";
static const char WARD2[] =
    "................"
    "................"
    "....kkkkk......."
    "...kllwllk......"
    "...klllllkkkkkk."
    "...kgggggkllllk."
    "....khhkhkllllk."
    "....khhhhklaalk."
    "...kkooookaooak."
    "..kaoooookaooak."
    "..kaoooooklaalk."
    "..kakooookllllk."
    "...kkooorkglllk."
    "...kbk.bkggggk.."
    "...kbk..kkkkkk.."
    "..kkkk..........";

/* the warden once its shield has broken: a plain soldier with a short blade */
static const char WARDX1[] =
    "................"
    "....kkkkk......."
    "...kllwllk......"
    "...klllllk......"
    "...kgggggk...k.."
    "....khhkhk..klk."
    "....khhhhk..klk."
    "...kkooookk.klk."
    "..kaoooooaakhk.."
    "..kaoooooookk..."
    "..kakoooork....."
    "...kkooorkk....."
    "....kbbkbk......"
    "....kbk.kbk....."
    "....kbk.kbk....."
    "...kkkk.kkkk....";
static const char WARDX2[] =
    "................"
    "................"
    "....kkkkk......."
    "...kllwllk......"
    "...klllllk......"
    "...kgggggk...k.."
    "....khhkhk..klk."
    "....khhhhk..klk."
    "...kkooookk.klk."
    "..kaoooooaakhk.."
    "..kaoooooookk..."
    "..kakoooork....."
    "...kkooorkk....."
    "...kbk.bk......."
    "...kbk..kbk....."
    "..kkkk..kkkk....";

static const char RIDER1[] =
    "...................."
    ".......kkkk........."
    "......kllwlk....kk.."
    "......kggggk...kbbk."
    ".......khkhk..kbtttk"
    ".......khhhk.kbttktk"
    "......kkoookkkbttttk"
    ".....kaoooorkbtttkk."
    ".....kaoohorkttttk.."
    "..kkkkaoooorkttttk.."
    ".kbttkkkookktttttk.."
    "kbtttttkbbkttttttk.."
    "kb.ktttttttttttk...."
    "...kbk.kbk..kbk.kbk."
    "...kbk.kbk..kbk.kbk."
    "...kkk.kkk..kkk.kkk.";
static const char RIDER2[] =
    "...................."
    ".......kkkk........."
    "......kllwlk....kk.."
    "......kggggk...kbbk."
    ".......khkhk..kbtttk"
    ".......khhhk.kbttktk"
    "......kkoookkkbttttk"
    ".....kaoooorkbtttkk."
    ".....kaoohorkttttk.."
    "..kkkkaoooorkttttk.."
    ".kbttkkkookktttttk.."
    "kbtttttkbbkttttttk.."
    "kb.ktttttttttttk...."
    "..kbk..kbk...kbk.kbk"
    ".kbk...kbk....kbk.kb"
    ".kk....kk......kk.kk";

static const char PIKE1[] =
    "................"
    ".....kkkkk......"
    "....kllwllk....."
    "...kkkkkkkkk...."
    ".....khhkhk....."
    ".....khhhhk....."
    "....kkooookk...k"
    "tttkaooooohhttlw"
    "...kaooooork...k"
    "...kakoooork...."
    "....kkooorkk...."
    ".....kooook....."
    ".....kbbkbbk...."
    ".....kbk.kbk...."
    ".....kbk.kbk...."
    "....kkkk.kkkk...";
static const char PIKE2[] =
    "................"
    "................"
    ".....kkkkk......"
    "....kllwllk....."
    "...kkkkkkkkk...."
    ".....khhkhk....."
    ".....khhhhk....."
    "....kkooookk...k"
    "tttkaooooohhttlw"
    "...kaooooork...k"
    "...kakoooork...."
    "....kkooorkk...."
    ".....kbbkbbk...."
    "....kbk..kbk...."
    "....kbk...kbk..."
    "...kkkk...kkkk..";

static const char SHADE1[] =
    "................"
    "......kkkk......"
    ".....kddddk....."
    "....kddddddk...."
    "....kdwkkwdk...."
    "....kdnnnndk...."
    "....kkdddddk...."
    "...kddooooddk..."
    "..kdddddooddkhlk"
    "..kdddddddddk.k."
    "..kddddddddk...."
    "...kdddddddk...."
    "....kddkddk....."
    "....knk.knk....."
    "....knk.knk....."
    "...kkkk.kkkk....";
static const char SHADE2[] =
    "................"
    "................"
    "......kkkk......"
    ".....kddddk....."
    "....kddddddk...."
    "....kdwkkwdk...."
    "....kdnnnndk...."
    "....kkdddddk...."
    "...kddooooddk..."
    "..kdddddooddkhlk"
    "..kdddddddddk.k."
    "..kddddddddk...."
    "...kdddddddk...."
    "...knk..knk....."
    "...knk...knk...."
    "..kkkk...kkkk...";

static const char POWDER1[] =
    "..w............."
    ".cwc............"
    "..k...kkkk......"
    ".kkkkkooaok....."
    "kttbtkoooook...."
    "kttbtkhhkhk....."
    "kbbbbkhhhhk....."
    "kttbtkaooooohk.."
    "kttbtkaooooohhk."
    "kttbtkaokooork.."
    "kbbbbkkoooork..."
    ".kkkkkkooork...."
    "......kbbkbbk..."
    "......kbk.kbk..."
    "......kbk.kbk..."
    ".....kkkk.kkkk..";
static const char POWDER2[] =
    ".c.............."
    "..w............."
    "..k...kkkk......"
    ".kkkkkooaok....."
    "kttbtkoooook...."
    "kttbtkhhkhk....."
    "kbbbbkhhhhk....."
    "kttbtkaooooohk.."
    "kttbtkaooooohhk."
    "kttbtkaokooork.."
    "kbbbbkkoooork..."
    ".kkkkkkooork...."
    "......kbbkbbk..."
    ".....kbk..kbk..."
    ".....kbk...kbk.."
    "....kkkk...kkkk.";

static const char CHAMP1[] =
    "........rr.........."
    ".......kkkkr........"
    "......kaawak....kk.."
    "......kooook...kggk."
    ".......khkhk..kglllk"
    ".......khhhkttttttlw"
    "......kkaaakkkgllllk"
    ".....kaoooorkglllkk."
    ".....kaoohorkllllk.."
    "..rrrkaoooorkllllk.."
    ".kgllkkkookklllllk.."
    "kglllllkbbkllllllk.."
    "kg.klllllllllllk...."
    "...kgk.kgk..kgk.kgk."
    "...kgk.kgk..kgk.kgk."
    "...kkk.kkk..kkk.kkk.";
static const char CHAMP2[] =
    ".......rr..........."
    ".......kkkkr........"
    "......kaawak....kk.."
    "......kooook...kggk."
    ".......khkhk..kglllk"
    ".......khhhkttttttlw"
    "......kkaaakkkgllllk"
    ".....kaoooorkglllkk."
    ".....kaoohorkllllk.."
    ".rrrrkaoooorkllllk.."
    ".kgllkkkookklllllk.."
    "kglllllkbbkllllllk.."
    "kg.klllllllllllk...."
    "..kgk..kgk...kgk.kgk"
    ".kgk...kgk....kgk.kg"
    ".kk....kk......kk.kk";

static const char ARROW[] =
    "c......"
    "cttttlw"
    "c......";
static const char KNIFE[] =
    ".w."
    ".l."
    ".l."
    "kyk"
    ".b."
    ".b.";
static const char STAR[] =
    "y...y"
    "yy.yy"
    ".yyy.";
static const char FLAG1[] =
    "kk........"
    "tkkkkkkkk."
    "taaoooook."
    "taooooook."
    "tooocoook."
    "toocccook."
    "tooocoork."
    "toooooork."
    "tkkkkkkkk."
    "t........."
    "t........."
    "b.........";
static const char FLAG2[] =
    "kk........"
    "tkkkk....."
    "taaookkkk."
    "taooooook."
    "tooocoook."
    "tooccoook."
    "toooccook."
    "toooooork."
    "tkkkkoork."
    "t....kkkk."
    "t........."
    "b.........";
static const char FLAG_DOWN[] =
    ".........."
    ".........."
    "kk........"
    "tk........"
    "tak......."
    "toak......"
    "tooak....."
    "toook....."
    "torok....."
    "tork......"
    "tk........"
    "b.........";
static const char TUFT[] =
    ".i.i."
    "zizjz"
    "jzjjj";
static const char FLOWER[] =
    ".c."
    "cyc"
    ".c.";
static const char HAND[] =
    "..kk...."
    ".kwwk..."
    ".kwwk..."
    ".kwwkkk."
    "kkwwwlwk"
    "kwwwwwwk"
    ".kwwwwk."
    "..kkkk..";
static const char HAND_GRAB[] =
    "........"
    "........"
    "..kkkk.."
    ".kwlwlk."
    "kwkwkwwk"
    "kwwwwwwk"
    ".kwwwwk."
    "..kkkk..";

typedef struct Art {
    int id, w, h;
    const char *px;
} Art;

static const Art ART[] = {
    {BS_FOOT1, 16, 16, FOOT1}, {BS_FOOT2, 16, 16, FOOT2}, {BS_BOW1, 16, 16, BOW1}, {BS_BOW2, 16, 16, BOW2},
    {BS_WARD1, 16, 16, WARD1}, {BS_WARD2, 16, 16, WARD2}, {BS_RIDER1, 20, 16, RIDER1}, {BS_RIDER2, 20, 16, RIDER2},
    {BS_PIKE1, 16, 16, PIKE1}, {BS_PIKE2, 16, 16, PIKE2}, {BS_SHADE1, 16, 16, SHADE1}, {BS_SHADE2, 16, 16, SHADE2},
    {BS_POWDER1, 16, 16, POWDER1}, {BS_POWDER2, 16, 16, POWDER2}, {BS_CHAMP1, 20, 16, CHAMP1}, {BS_CHAMP2, 20, 16, CHAMP2},
    {BS_WARDX1, 16, 16, WARDX1}, {BS_WARDX2, 16, 16, WARDX2},
    {BS_ARROW, 7, 3, ARROW}, {BS_KNIFE, 3, 6, KNIFE}, {BS_STAR, 5, 3, STAR},
    {BS_FLAG1, 10, 12, FLAG1}, {BS_FLAG2, 10, 12, FLAG2}, {BS_FLAG_DOWN, 10, 12, FLAG_DOWN},
    {BS_TUFT, 5, 3, TUFT}, {BS_FLOWER, 3, 3, FLOWER}, {BS_HAND, 8, 8, HAND}, {BS_HAND_GRAB, 8, 8, HAND_GRAB},
};

void bf_art_load(void) {
    static int loaded;
    if (loaded) return;
    for (int i = 0; i < ARRAY_LEN(ART); i++) {
        int len = (int)strlen(ART[i].px);
        if (len != ART[i].w * ART[i].h) fprintf(stderr, "bannerfall art %d: expected %d got %d\n", ART[i].id, ART[i].w * ART[i].h, len);
        spr_make(&bf_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
    }
    /* the Thistle Host wears violet */
    for (int s = 0; s < 2; s++) pal_identity(BF_TEAM_MAP[s]);
    pal_swap(BF_TEAM_MAP[1], C_AMBER, C_MAGENTA);
    pal_swap(BF_TEAM_MAP[1], C_ORANGE, C_VIOLET);
    pal_swap(BF_TEAM_MAP[1], C_RED, C_PURPLE);
    loaded = 1;
}
