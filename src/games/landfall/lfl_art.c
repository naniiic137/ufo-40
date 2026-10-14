/* LANDFALL - pixel art (palette-letter strings, see gfx.h), all drawn for
 * UFO 40: Wick in her flight suit, the ten kinds of creature a planet draws
 * its species from, the big shell, the gnat, the hatchling, the eight
 * tools, the energy cell, the chart piece and the four resources. The
 * ground, the ship and the shots are drawn in lfl_draw.c. */
#include "lfl.h"

Sprite lfl_spr[SPR_COUNT];

/* Wick, 12 x 14 */
static const char WICK_D0[] =
    "....kkkk...."
    "..kkwwwwkk.."
    ".kwwwwwwwwk."
    ".kwCCCCCCwk."
    ".kwCuCCuCwk."
    ".kwwCCCCwwk."
    "..kkwwwwkk.."
    ".kaakwwkaak."
    "kaaaaaaaaaak"
    "kwkaaaaaakwk"
    "kwkaaooaakwk"
    ".k.kaaaak.k."
    "...kak.kak.."
    "...kbk.kbk..";
static const char WICK_D1[] =
    "....kkkk...."
    "..kkwwwwkk.."
    ".kwwwwwwwwk."
    ".kwCCCCCCwk."
    ".kwCuCCuCwk."
    ".kwwCCCCwwk."
    "..kkwwwwkk.."
    ".kaakwwkaak."
    "kaaaaaaaaaak"
    "kwkaaaaaakwk"
    "kwkaaooaakwk"
    ".k.kaaaak.k."
    "...kak..kk.."
    "...kbk......";
static const char WICK_U0[] =
    "....kkkk...."
    "..kkwwwwkk.."
    ".kwwwwwwwwk."
    ".kwwwwwwlwk."
    ".kwwwwwwwwk."
    ".kwwwwwwwwk."
    "..kkwwwwkk.."
    ".kakggggkak."
    "kaakgllgkaak"
    "kwkkggggkkwk"
    "kwkaggggakwk"
    ".k.kaaaak.k."
    "...kak.kak.."
    "...kbk.kbk..";
static const char WICK_U1[] =
    "....kkkk...."
    "..kkwwwwkk.."
    ".kwwwwwwwwk."
    ".kwwwwwwlwk."
    ".kwwwwwwwwk."
    ".kwwwwwwwwk."
    "..kkwwwwkk.."
    ".kakggggkak."
    "kaakgllgkaak"
    "kwkkggggkkwk"
    "kwkaggggakwk"
    ".k.kaaaak.k."
    "..kk..kak..."
    "......kbk...";
static const char WICK_R0[] =
    "....kkkk...."
    "..kkwwwwkk.."
    ".kwwwwwwwwk."
    ".kwwwwCCCCk."
    ".kwwwwCuCCk."
    ".kwwwwwCCwk."
    "..kkwwwwkk.."
    "..kgkaaak..."
    ".kggkaaaak.."
    ".kggkaaaak.."
    ".kggkaoak..."
    "..kkaaaak..."
    "...kak.kak.."
    "...kbk.kbk..";
static const char WICK_R1[] =
    "....kkkk...."
    "..kkwwwwkk.."
    ".kwwwwwwwwk."
    ".kwwwwCCCCk."
    ".kwwwwCuCCk."
    ".kwwwwwCCwk."
    "..kkwwwwkk.."
    "..kgkaaak..."
    ".kggkaaaak.."
    ".kggkaaaak.."
    ".kggkaoak..."
    "..kkaaaak..."
    "....kakak..."
    "....kbkbk...";
static const char WICK_SMALL[] =
    "..kkkk.."
    ".kwCCwk."
    ".kwwwwk."
    "..kaak.."
    ".kaaaak."
    "..kaak.."
    "..k..k.."
    "........";

/* the ten looks a planet's species are drawn from, 16 x 16 */
static const char LOOKS[LFL_LOOKS][16 * 16 + 1] = {
    /* 0 puffwort: a fuzzy ball */
    "................"
    "......k.k.k....."
    "....kkikiikk...."
    "...kiiiiiiiik..."
    "..kiiziiiiziik.."
    "..kiikwkkwkiik.."
    ".kiiikwkkwkiiik."
    ".kiiiiiiiiiiiik."
    ".kiziiikkiiizik."
    ".kiiiiikkiiiiik."
    "..kiiiiiiiiiik.."
    "..kiziiiiiiziik."
    "...kiiiiiiiik..."
    "....kkiiiikk...."
    "....kzk..kzk...."
    "....kk....kk....",
    /* 1 shellback: a beetle */
    "................"
    "....k......k...."
    ".....k....k....."
    "....kkkkkkkk...."
    "...kVVVVVVVVk..."
    "..kVpVVVVVVpVk.."
    "..kVVVVkkVVVVk.."
    ".kkVVVVkkVVVVkk."
    "k.kVVpVkkVpVVk.k"
    ".kkVVVVkkVVVVkk."
    "k.kVVVVkkVVVVk.k"
    "..kVpVVkkVVpVk.."
    "...kVVVkkVVVk..."
    "....kkkkkkkk...."
    "...k..k..k..k..."
    "................",
    /* 2 stalkeye: a slug with one eye up high */
    ".......kkk......"
    "......kwwwk....."
    "......kwkwk....."
    "......kwwwk....."
    ".......kqk......"
    ".......kqk......"
    ".......kqk......"
    "......kqqqk....."
    ".....kqqqqqk...."
    "....kqjqqqjqk..."
    "...kqqqqqqqqqk.."
    "..kqqjqqqqqjqqk."
    "..kqqqqqqqqqqqk."
    ".kqqqqqqqqqqqqk."
    ".kkkkkkkkkkkkkk."
    "................",
    /* 3 drifter: a squid */
    "................"
    ".....kkkkkk....."
    "....kKKKKKKk...."
    "...kKKKKKKKKk..."
    "...kKKwKKwKKk..."
    "...kKKkKKkKKk..."
    "...kKKKKKKKKk..."
    "...kKPKKKKPKk..."
    "....kKKKKKKk...."
    "....kKkKKkKk...."
    "...kKk.kk.kKk..."
    "...kKk.kk.kKk..."
    "..kKk..kk..kKk.."
    "..kk...kk...kk.."
    "........k......."
    "................",
    /* 4 squatter: a frog */
    "................"
    "................"
    "...kkk....kkk..."
    "..kwkwk..kwkwk.."
    "..kwwwkkkkwwwk.."
    "..kkkzzzzzzkkk.."
    ".kzzzzzzzzzzzzk."
    ".kzzzzzzzzzzzzk."
    ".kzkkkkkkkkkkzk."
    ".kzzzzzzzzzzzzk."
    "..kzzttttttzzk.."
    "..kztttttttztk.."
    ".kzzkttttttkzzk."
    "kzzk.kkkkkk.kzzk"
    "kkk..........kkk"
    "................",
    /* 5 pincher: a crab */
    "................"
    ".kk..........kk."
    "krrk........krrk"
    "krk..........krk"
    ".krk...kk...krk."
    "..krkkkkkkkkrk.."
    "...krrrrrrrrk..."
    "..krrwkrrwkrrk.."
    "..krrkkrrkkrrk.."
    "..krrrrrrrrrrk.."
    ".kkrrorrrrorrkk."
    "k..krrrrrrrrk..k"
    ".k..kkkkkkkk..k."
    "..k.k......k.k.."
    "................"
    "................",
    /* 6 spiner: a walking cactus */
    "......k........."
    ".....kfk..k....."
    "....kfffkkfk...."
    "...kffwfwkfk...."
    "..kkfffffkfk.k.."
    ".kfkffkfffkkfk.."
    ".kfkfffffffkfk.."
    ".kfkffffffffk..."
    "..kkfjfffjffk..."
    "....kfffffffk..."
    "....kfjfffjfk..."
    "....kfffffffk..."
    "....kfffffffk..."
    "....kkfffffkk..."
    "....kek...kek..."
    "....kk.....kk...",
    /* 7 bobber: a jelly */
    "................"
    ".....kkkkkk....."
    "...kkIIIIIIkk..."
    "..kIIwIIIIIIIk.."
    "..kIwIIIIIIIIk.."
    ".kIIIIIkIIkIIIk."
    ".kIIIIIkIIkIIIk."
    ".kIIIIIIIIIIIIk."
    ".kuuuuuuuuuuuuk."
    "..kkuukkkkuukk.."
    "...kuk.ku.kuk..."
    "...ku..ku..uk..."
    "...k...uk...k..."
    "........k......."
    "................"
    "................",
    /* 8 burrower: a mole */
    "................"
    "................"
    ".....kkkkkk....."
    "....kbbbbbbk...."
    "...kbbbbbbbbk..."
    "..kbbkbbbbkbbk.."
    "..kbbkbbbbkbbk.."
    "..kbbbbKKbbbbk.."
    ".kbbbbKKKKbbbbk."
    ".kbbbbbkkbbbbbk."
    ".kbbttttttttbbk."
    "kkkbttttttttbkkk"
    "kwkkbbbbbbbbkkwk"
    ".k.kbbbbbbbbk.k."
    "...kkk....kkk..."
    "................",
    /* 9 clanker: a boxy thing on one wheel */
    "......k........."
    "......kk........"
    "....kkkkkkkk...."
    "...kggggggggk..."
    "...kgkkggkkgk..."
    "...kgkrggkrgk..."
    "...kggggggggk..."
    "...kgkkkkkkgk..."
    "..kkggggggggkk.."
    ".kgkllllllllkgk."
    ".kgkllllllllkgk."
    "..kkkkkkkkkkkk.."
    "......kssk......"
    ".....kskkskk...."
    "......kssk......"
    ".......kk.......",
};

/* the big shell, 32 x 32, drawn in lfl_draw.c from circles */

static const char GNAT0[] =
    ".w....w."
    "wIw..wIw"
    ".wIkkIw."
    "..kmmk.."
    "..kmrk.."
    "...kk..."
    "...k...."
    "........";
static const char GNAT1[] =
    "........"
    "........"
    "wwIkkIww"
    ".IIkmkI."
    "..kmrk.."
    "...kk..."
    "...k...."
    "........";
static const char CHICK0[] =
    "........"
    "..kkk..."
    ".kyyyk.."
    ".kykyyk."
    ".kyyyyoo"
    "..kyyyk."
    "...o.o.."
    "........";
static const char CHICK1[] =
    "........"
    "..kkk..."
    ".kyyyk.."
    ".kykyyk."
    ".kyyyyoo"
    "..kyyyk."
    "..o...o."
    "........";

/* the tools, 12 x 12 */
static const char TOOLS[TL_COUNT][12 * 12 + 1] = {
    /* disc */
    "............"
    "....kkkk...."
    "..kkoooakk.."
    ".koooaaoook."
    ".kooakkaook."
    "koaak..kaaok"
    "koaak..kaaok"
    ".kooakkaook."
    ".koooaaoook."
    "..kkoooakk.."
    "....kkkk...."
    "............",
    /* rivet gun */
    "............"
    "............"
    "..kkkkkkkkk."
    ".klllllllllk"
    ".kgggggggkkk"
    ".kgsssggk..."
    ".kgk.kgk...."
    ".kgk..kk...."
    "..kgk......."
    "..kgk......."
    "..kkk......."
    "............",
    /* hatcher: an egg-shaped ray */
    "............"
    "....kkk....."
    "...kwwwk...."
    "..kwwKwwk..."
    "..kwKwKwk..."
    "..kwwwwwk..."
    "..kwwKwwk..."
    "...kwwwkkkk."
    "....kkkKKKK."
    ".....kk.kkk."
    ".....kk....."
    "............",
    /* blast pods */
    "......r....."
    ".....ry....."
    ".....k......"
    "....kkk....."
    "..kknnnkk..."
    ".knnwnnnnk.."
    ".knwnnnnnk.."
    ".knnnnnnnk.."
    ".knnnnnnnk.."
    "..knnnnnk..."
    "...kkkkk...."
    "............",
    /* thumper: a drum */
    "............"
    "..kkkkkkkk.."
    ".kuuuuuuuuk."
    ".kBBBBBBBBk."
    ".kwBkBBkBwk."
    ".kBBBkkBBBk."
    ".kBkBBBBkBk."
    ".kBBBBBBBBk."
    ".kNNNNNNNNk."
    "..kkkkkkkk.."
    "..k......k.."
    "............",
    /* bog boots */
    "............"
    "..kkkk......"
    "..kzzk......"
    "..kzzk.kkkk."
    "..kzzk.kzzk."
    "..kzzk.kzzk."
    "..kzzzkkzzk."
    "..kzzzzkzzk."
    ".kzzzzzkzzzk"
    ".kffffkkzzzk"
    ".kkkkk.kffkk"
    ".......kkkk.",
    /* chop press */
    "............"
    ".kkkkkkkkkk."
    ".kssssssssk."
    ".ksrrrrrrsk."
    ".ksrwwrrrsk."
    ".kssssssssk."
    ".kgggggggk.."
    ".kgkkkkkgk.."
    ".kgkrrrkgk.."
    ".kgkkkkkgk.."
    ".kkkkkkkkk.."
    "............",
    /* talk box */
    "............"
    ".kkkkkkkkkk."
    ".kyyyyyyyyk."
    ".kykkkykkyk."
    ".kyyyyyyyyk."
    ".kykykkkyyk."
    ".kyyyyyyyyk."
    ".kkkkkkkkkk."
    "...kyk......"
    "...kk......."
    "............"
    "............",
};

static const char CUBE[] =
    "............"
    "...kkkkkk..."
    "..krrrrrrk.."
    ".krrwrrrrrk."
    ".krwrrrrrmk."
    ".krrrrrrrmk."
    ".krrrrrrrmk."
    ".krrrrrrmmk."
    ".kmrrrrmmmk."
    "..kmmmmmmk.."
    "...kkkkkk..."
    "............";
static const char MAPPIECE[] =
    "............"
    ".kkkkkkk...."
    ".kcccccck..."
    ".kccyccccck."
    ".kcyyycccck."
    ".kccyccNcck."
    ".kcccccNcck."
    ".kccNNNcccck"
    ".kcccccccyck"
    ".kcccccccck."
    ".kkkkkkkkk.."
    "............";
static const char BUNDLE[] =
    "............"
    ".....kk....."
    "....kttk...."
    "...kktkkk..."
    "..kbbbbbbk.."
    ".kbbtbbbbbk."
    ".kbtbbbbbbk."
    ".kbbbbbbbbk."
    ".kbbbbbbbbk."
    "..kbbbbbbk.."
    "...kkkkkk..."
    "............";

/* the resources, 8 x 8: beads, burrs, ferns, spores */
static const char RES[RS_COUNT][8 * 8 + 1] = {
    "..kkkk.."
    ".kCCCCk."
    "kCwCCCCk"
    "kCCCCuCk"
    "kCCCCuCk"
    "kCCuuuCk"
    ".kCCCCk."
    "..kkkk..",
    "k..k..k."
    ".kakaak."
    "..kaaak."
    "kaaoaaak"
    ".kaaaok."
    ".kaoaak."
    ".kakkak."
    "k..k..k.",
    "....kk.."
    "...kik.."
    "..kiizk."
    ".kiizik."
    ".kizik.."
    "kizik..."
    "kzik...."
    ".kk.....",
    ".kk..kk."
    "kKPkkPKk"
    "kPKkkKPk"
    ".kk..kk."
    ".kk..kk."
    "kKPkkKPk"
    "kPKkkPKk"
    ".kk..kk.",
};

static const char CHOP[] =
    "..kkkk.."
    ".krrrrk."
    "krrwrrrk"
    "krrrrrrk"
    "kmrrrrmk"
    ".kmmmmkk"
    "..kkkkwk"
    "......kk";
static const char CHOP_COOKED[] =
    "..kkkk.."
    ".kaaaak."
    "kaayaaak"
    "kaaaaaak"
    "koaaaaok"
    ".kooookk"
    "..kkkkwk"
    "......kk";

static const char TALK[] =
    ".kkkkkk."
    "kwwwwwwk"
    "kwkwkwwk"
    "kwwwwwwk"
    ".kkwkkk."
    "...kk..."
    "........"
    "........";

/* frame 2 of a look: the bottom rows shuffle */
static void make_step(Sprite *dst, const Sprite *src) {
    dst->w = src->w;
    dst->h = src->h;
    dst->px = (uint8_t *)malloc((size_t)src->w * src->h);
    memcpy(dst->px, src->px, (size_t)src->w * src->h);
    for (int y = src->h - 4; y < src->h; y++) {
        int sh = (y & 1) ? 1 : -1;
        for (int x = 0; x < src->w; x++) {
            int sx = x - sh;
            dst->px[y * src->w + x] = sx >= 0 && sx < src->w ? src->px[y * src->w + sx] : TRANSPARENT;
        }
    }
}

static bool loaded;

void lfl_art_load(void) {
    if (loaded) return;
    loaded = true;
    spr_make(&lfl_spr[SPR_WICK_D0], 12, 14, WICK_D0);
    spr_make(&lfl_spr[SPR_WICK_D1], 12, 14, WICK_D1);
    spr_make(&lfl_spr[SPR_WICK_U0], 12, 14, WICK_U0);
    spr_make(&lfl_spr[SPR_WICK_U1], 12, 14, WICK_U1);
    spr_make(&lfl_spr[SPR_WICK_R0], 12, 14, WICK_R0);
    spr_make(&lfl_spr[SPR_WICK_R1], 12, 14, WICK_R1);
    spr_make(&lfl_spr[SPR_WICK_SMALL], 8, 8, WICK_SMALL);
    for (int i = 0; i < LFL_LOOKS; i++) {
        spr_make(&lfl_spr[SPR_LOOK0 + i * 2], 16, 16, LOOKS[i]);
        make_step(&lfl_spr[SPR_LOOK0 + i * 2 + 1], &lfl_spr[SPR_LOOK0 + i * 2]);
    }
    spr_make(&lfl_spr[SPR_GNAT0], 8, 8, GNAT0);
    spr_make(&lfl_spr[SPR_GNAT1], 8, 8, GNAT1);
    spr_make(&lfl_spr[SPR_CHICK0], 8, 8, CHICK0);
    spr_make(&lfl_spr[SPR_CHICK1], 8, 8, CHICK1);
    for (int t = 0; t < TL_COUNT; t++) spr_make(&lfl_spr[SPR_TOOL0 + t], 12, 12, TOOLS[t]);
    spr_make(&lfl_spr[SPR_CUBE], 12, 12, CUBE);
    spr_make(&lfl_spr[SPR_MAP], 12, 12, MAPPIECE);
    spr_make(&lfl_spr[SPR_BUNDLE], 12, 12, BUNDLE);
    for (int r = 0; r < RS_COUNT; r++) spr_make(&lfl_spr[SPR_RES0 + r], 8, 8, RES[r]);
    spr_make(&lfl_spr[SPR_CHOP], 8, 8, CHOP);
    spr_make(&lfl_spr[SPR_CHOP_COOKED], 8, 8, CHOP_COOKED);
    spr_make(&lfl_spr[SPR_TALK], 8, 8, TALK);
}

/* every sprite string the right length, with only palette letters (tests) */
int lfl_art_check(void) {
    int bad = 0;
    for (int i = 0; i < LFL_LOOKS; i++) bad += strlen(LOOKS[i]) != 256;
    for (int t = 0; t < TL_COUNT; t++) bad += strlen(TOOLS[t]) != 144;
    for (int r = 0; r < RS_COUNT; r++) bad += strlen(RES[r]) != 64;
    const char *s14[] = {WICK_D0, WICK_D1, WICK_U0, WICK_U1, WICK_R0, WICK_R1};
    for (int i = 0; i < 6; i++) bad += strlen(s14[i]) != 12 * 14;
    const char *s8[] = {WICK_SMALL, GNAT0, GNAT1, CHICK0, CHICK1, CHOP, CHOP_COOKED, TALK};
    for (int i = 0; i < 8; i++) bad += strlen(s8[i]) != 64;
    const char *s12[] = {CUBE, MAPPIECE, BUNDLE};
    for (int i = 0; i < 3; i++) bad += strlen(s12[i]) != 144;
    for (int i = 0; i < LFL_LOOKS; i++)
        for (const char *p = LOOKS[i]; *p; p++) bad += *p != '.' && pal_char_index(*p) < 0;
    for (int t = 0; t < TL_COUNT; t++)
        for (const char *p = TOOLS[t]; *p; p++) bad += *p != '.' && pal_char_index(*p) < 0;
    return bad;
}
