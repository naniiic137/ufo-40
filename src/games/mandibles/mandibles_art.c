/* MANDIBLES - pixel art (palette-letter strings, see gfx.h). All ours.
 * Ants are drawn top-down in the Bluebell Colony's blue ('B' body, 'u'
 * shine, 'N' shade); the Rust Horde is the same art remapped (MND_TEAM).
 * Three views (east, south-east, south) give all eight by flipping. */
#include "mandibles.h"

Sprite mnd_spr[MS_SPRITE_COUNT];
uint8_t MND_TEAM[3][PAL_COUNT];

static const char SOL_E1[] =
    "...k.k.."
    "..k.k..k"
    ".NNk.NNk"
    "NBuBNBuw"
    "NBBBNBBw"
    ".NNk.NNk"
    "..k.k..k"
    "...k.k..";
static const char SOL_E2[] =
    "..k.k..."
    "...k.k.k"
    ".NN.kNNk"
    "NBuBNBuw"
    "NBBBNBBw"
    ".NN.kNNk"
    "...k.k.k"
    "..k.k...";
static const char SOL_D1[] =
    "NN......"
    "NBuk.k.."
    ".BBN...."
    ".kNNBk.."
    "...kBuN."
    "..k.NBBk"
    "....k.Nw"
    "......w.";
static const char SOL_D2[] =
    "NN......"
    "NBu.k..."
    ".BBNk..."
    "..NNB.k."
    ".k.kBuN."
    "....NBB."
    "...k.kNw"
    "......w.";
static const char SOL_S1[] =
    "..NNNN.."
    "k.NBuN.k"
    ".kNBBNk."
    "...NN..."
    "k.NBuN.k"
    ".kNBBNk."
    "...ww..."
    "..w..w..";
static const char SOL_S2[] =
    "..NNNN.."
    ".kNBuNk."
    "k.NBBN.k"
    "...NN..."
    ".kNBuNk."
    "k.NBBN.k"
    "...ww..."
    "..w..w..";

static const char WRK_E1[] =
    "........"
    "..k.k..k"
    "..NkNNk."
    ".NBuNBu."
    ".NBBNBB."
    "..NkNNk."
    "..k.k..k"
    "........";
static const char WRK_E2[] =
    "........"
    "...k.k.k"
    "..N.kNk."
    ".NBuNBu."
    ".NBBNBB."
    "..N.kNk."
    "...k.k.k"
    "........";
static const char WRK_D1[] =
    "........"
    ".NN.k..."
    ".NBuk..."
    "..NNBk.."
    "..kkBuN."
    "....NBB."
    "...k.k.k"
    "........";
static const char WRK_D2[] =
    "........"
    ".NNk...."
    ".NBu.k.."
    "..NNBk.."
    "...kBuN."
    "..k.NBB."
    ".....k.k"
    "........";
static const char WRK_S1[] =
    "........"
    "..NNN..."
    ".kNBuk.."
    "..NBN..."
    ".kNBuk.."
    "..NBN..."
    ".k.k.k.."
    "........";
static const char WRK_S2[] =
    "........"
    "..NNN..."
    "k.NBu.k."
    "..NBN..."
    "k.NBu.k."
    "..NBN..."
    "..k.k..."
    "........";

static const char QUEEN1[] =
    "......NNNN......"
    ".....NBBBBN....."
    "....NBuBBBBN...."
    "....NBBBBBBN...."
    "..cc.NBBBBN.cc.."
    ".cIIc.NNNN.cIIc."
    ".cIIIcNBBNcIIIc."
    "..cIIcNBuNcIIc.."
    "...cckNBBNkcc..."
    "..k..kNNNNk..k.."
    ".k...k.NN.k...k."
    "k....kNBBNk....k"
    ".....NBuBBN....."
    ".....NBBBBN....."
    "......wNNw......"
    ".....w....w.....";
static const char QUEEN2[] =
    "......NNNN......"
    ".....NBBBBN....."
    "....NBuBBBBN...."
    "....NBBBBBBN...."
    "...c.NBBBBN.c..."
    "..cIc.NNNN.cIc.."
    ".cIIIcNBBNcIIIc."
    ".cIIIcNBuNcIIIc."
    "..cc.kNBBNk.cc.."
    ".k...kNNNNk...k."
    "..k..k.NN.k..k.."
    "...k.kNBBNk.k..."
    ".....NBuBBN....."
    ".....NBBBBN....."
    "......wNNw......"
    "......w..w......";

static const char SPIDER1[] =
    "k......kk......k"
    ".k.....kk.....k."
    "..k...k..k...k.."
    "...k.ssssss.k..."
    "kk..sdggggds..kk"
    "..kksgdddgdskk.."
    "....sdgggdds...."
    "kkkksdddddds.kkk"
    "...ksdgdgdds.k.."
    "..k.ssddddss..k."
    ".k...ssvvss....k"
    "k....svKKvs....."
    "....k.ssss.k...."
    "...k..k..k..k..."
    "..k..k....k..k.."
    ".k..k......k..k.";
static const char SPIDER2[] =
    ".k.....kk.....k."
    "k......kk......k"
    ".k....k..k....k."
    "..kk.ssssss.kk.."
    "....sdggggds...."
    "kkkksgdddgdskkkk"
    "....sdgggdds...."
    "..kksdddddds.kk."
    ".k..sdgdgdds...k"
    "k...ssddddss.k.."
    "..k..ssvvss..k.."
    ".k...svKKvs...k."
    "k...k.ssss.k...k"
    "...k..k..k..k..."
    "..k...k..k...k.."
    ".k....k..k....k.";

static const char BEAD[] =
    ".jj."
    "jiij"
    "jzij"
    ".jj.";
static const char SPIT[] =
    ".w."
    "www"
    ".w.";
static const char CROWN[] =
    "y.y.y"
    "yyyyy"
    ".aaa.";

static const char ROCK1[] =
    "..gggg.."
    ".gllggs."
    "gllgggss"
    "gggggsss"
    "gggsgsss"
    "sggssssd"
    ".sssssd."
    "..dddd..";
static const char ROCK2[] =
    "gggggggg"
    "gllggggs"
    "ggggggss"
    "gggsggss"
    "ggssgsss"
    "sgsssssd"
    "sssssssd"
    "ssdsdddd";
static const char TUFT[] =
    "........"
    ".z...z.."
    ".zj.jz.."
    "..jzj..z"
    "z..j..zj"
    "jz....j."
    ".j......"
    "........";
static const char PEBBLE[] =
    "........"
    ".hh....."
    ".ht..c.."
    "......t."
    "...hh..."
    "...tt.h."
    ".c....t."
    "........";
static const char ROOT[] =
    "bbtbbbbb"
    "btttbbtb"
    "bbbtttbb"
    "tbbbbttb"
    "ttbbbbtt"
    "btttbbbb"
    "bbbttbbt"
    "bbbbtttb";

/* General Stag, who writes the briefings */
static const char GENERAL[] =
    "................"
    "....kkkkkkkk...."
    "...kyyyyyyyyk..."
    "..kNNNNNNNNNNk.."
    "..kkkkkkkkkkkk.."
    "...kNBBBBBBNk..."
    "..kNBuwkBwkBNk.."
    "..kNBBkkBkkBNk.."
    "..kNBBBBBBBBNk.."
    "...kNBBBBBBNk..."
    "...kwwNNNNwwk..."
    "..kw.kkkkkk.wk.."
    "..kkNByayaBNkk.."
    ".kNNBBrrrrBBNNk."
    ".kNBBBBBBBBBBNk."
    ".kkkkkkkkkkkkkk.";

typedef struct ArtDef { int id, w, h; const char *px; } ArtDef;
static const ArtDef ART[] = {
    {MS_SOL_E1, 8, 8, SOL_E1}, {MS_SOL_E2, 8, 8, SOL_E2}, {MS_SOL_D1, 8, 8, SOL_D1}, {MS_SOL_D2, 8, 8, SOL_D2},
    {MS_SOL_S1, 8, 8, SOL_S1}, {MS_SOL_S2, 8, 8, SOL_S2},
    {MS_WRK_E1, 8, 8, WRK_E1}, {MS_WRK_E2, 8, 8, WRK_E2}, {MS_WRK_D1, 8, 8, WRK_D1}, {MS_WRK_D2, 8, 8, WRK_D2},
    {MS_WRK_S1, 8, 8, WRK_S1}, {MS_WRK_S2, 8, 8, WRK_S2},
    {MS_QUEEN1, 16, 16, QUEEN1}, {MS_QUEEN2, 16, 16, QUEEN2},
    {MS_SPIDER1, 16, 16, SPIDER1}, {MS_SPIDER2, 16, 16, SPIDER2},
    {MS_BEAD, 4, 4, BEAD}, {MS_SPIT, 3, 3, SPIT}, {MS_CROWN, 5, 3, CROWN},
    {MS_ROCK1, 8, 8, ROCK1}, {MS_ROCK2, 8, 8, ROCK2}, {MS_TUFT, 8, 8, TUFT}, {MS_PEBBLE, 8, 8, PEBBLE},
    {MS_ROOT, 8, 8, ROOT}, {MS_GENERAL, 16, 16, GENERAL},
};

void mnd_art_load(void) {
    static int loaded;
    if (loaded) return;
    for (int i = 0; i < ARRAY_LEN(ART); i++) {
        int len = (int)strlen(ART[i].px);
        if (len != ART[i].w * ART[i].h) fprintf(stderr, "mandibles art %d: expected %d got %d\n", ART[i].id, ART[i].w * ART[i].h, len);
        spr_make(&mnd_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
    }
    for (int s = 0; s < 3; s++) pal_identity(MND_TEAM[s]);
    /* the Rust Horde */
    pal_swap(MND_TEAM[1], C_BLUE, C_RED);
    pal_swap(MND_TEAM[1], C_SKY, C_ORANGE);
    pal_swap(MND_TEAM[1], C_NAVY, C_MAROON);
    /* your own ant: a brighter blue */
    pal_swap(MND_TEAM[2], C_BLUE, C_SKY);
    pal_swap(MND_TEAM[2], C_SKY, C_CYAN);
    loaded = 1;
}
