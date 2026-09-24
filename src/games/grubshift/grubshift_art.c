/* GRUB SHIFT - pixel art (palette-letter strings, see gfx.h). */
#include "grubshift.h"

Sprite gs_spr[GS_SPRITE_COUNT];

static const char TILLY1[] =
    ".......zj......."
    "......zijz......"
    ".......jf......."
    ".......kk......."
    "....kkkkkkkk...."
    "...kllllllllk..."
    "..klwwwwwwwwlk.."
    "..klwNNNNNNwlk.."
    "..klwNCkkCNwlk.."
    "..klwNCkkCNwlk.."
    "..klwwwwwwwwlk.."
    "..kgllllllllgk.."
    "..kkkkkkkkkkkk.."
    ".kdsdsdsdsdsdsk."
    ".kdkdkdkdkdkdkk."
    "..kkkkkkkkkkkk..";
static const char TILLY2[] =
    "......zj........"
    ".....zijz......."
    "......jf........"
    ".......kk......."
    "....kkkkkkkk...."
    "...kllllllllk..."
    "..klwwwwwwwwlk.."
    "..klwNNNNNNwlk.."
    "..klwNkkCCNwlk.."
    "..klwNkkCCNwlk.."
    "..klwwwwwwwwlk.."
    "..kgllllllllgk.."
    "..kkkkkkkkkkkk.."
    ".ksdsdsdsdsdsdk."
    ".kkdkdkdkdkdkdk."
    "..kkkkkkkkkkkk..";
static const char TILLY_HOP[] =
    ".......zj......."
    "......zijz......"
    ".......jf......."
    ".......kk......."
    "....kkkkkkkk...."
    "...kllllllllk..."
    "..klwwwwwwwwlk.."
    "..klwNNNNNNwlk.."
    "..klwNCCCCNwlk.."
    "..klwNkkkkNwlk.."
    "..klwwwwwwwwlk.."
    "..kgllllllllgk.."
    "..kkkkkkkkkkkk.."
    "..kdsdsdsdsdsk.."
    "...kkkkkkkkkk..."
    "................";
static const char TILLY_DEAD[] =
    "................"
    "................"
    "...........k...."
    "....kkkkkkkjk..."
    "...kllllllkzk..."
    "..klwwwwwwwkk..."
    "..klwNNNNNNwlk.."
    "..klwNrNNrNwlk.."
    "..klwNNrrNNwlk.."
    "..klwNrNNrNwlk.."
    "..klwwwwwwwwlk.."
    "..kgllllllllgk.."
    "..kkkkkkkkkkkk.."
    ".kdsdsdsdsdsdsk."
    ".kdkdkdkdkdkdkk."
    "..kkkkkkkkkkkk..";
static const char POD1[] =
    "..kkkk.."
    ".kgllgk."
    "kkkkkkkk"
    "kizzzzjk"
    "kiizzzjk"
    "kizwzzjk"
    "kizzzzjk"
    "kizzzjjk"
    "kkkkkkkk"
    ".kgssgk."
    "..kkkk..";
static const char POD2[] =
    "..kkkk.."
    ".kgllgk."
    "kkkkkkkk"
    "kiiiizjk"
    "kiiwiizk"
    "kiwwwizk"
    "kiiwiizk"
    "kiiiizjk"
    "kkkkkkkk"
    ".kgssgk."
    "..kkkk..";

/* larva base, recoloured per species: l light, g mid, s dark */
static const char LARVA[] =
    "............"
    "...kkkkkk..."
    "..klglglgkk."
    ".klglglglgwk"
    ".kglglglgkkk"
    ".ksgsgsgsk.."
    "..kkkkkkk..."
    "............";

static const char ADULT_SPARK[] =
    "...k......k....."
    "....k....k......"
    "...kykkkkyk....."
    "..kyyaaaayyk...."
    ".kyawkyywkayk..."
    ".kyakkyykkayk..."
    ".kaaaaaaaaaak..."
    "koaoaoaoaoaook.."
    "kaoaoaoaoaoaok.."
    ".kkaaaaaaaakk..."
    ".k.k.k..k.k.k..."
    "k..k..k..k..k...";
static const char ADULT_SHELL[] =
    "................"
    "......kkkk......"
    "....kkCCCCkk...."
    "...kCIICCCuuk..."
    "..kCIICCCuuuBk.."
    "..kCICCCuuuBBk.."
    ".kkuCCCuuuBBkkk."
    "kwkkuuuuuBBkkwk."
    "kkkkkkkkkkkkkkk."
    ".kNk.kNk.kNk.kNk"
    ".k...k...k...k.."
    "................";
static const char ADULT_BURROW[] =
    "................"
    "...kk.....kk...."
    "..ktk.....ktk..."
    "..kekkkkkkkek..."
    ".kteeeeeeeeetk.."
    ".ktewkeeekwetk.."
    "kkteekeeekeetkk."
    "kttteeeeeeetttk."
    "khhtteeeeettthk."
    ".kkkttttttttkk.."
    ".khk.kbk.kbk.khk"
    "..k...k...k...k.";
static const char ADULT_MOUND[] =
    "................"
    ".....kkkkk......"
    "...kkzzizzkk...."
    "..kzziizzzzjk..."
    ".kzwkzzzzwkzjk.."
    ".kzkkzzzzkkzjk.."
    ".kjzzzzzzzzjjk.."
    "kfjjzzkkzzjjjfk."
    "kffjjjjjjjjjffk."
    ".kkffffffffkkk.."
    "..kfk.kfk.kfk..."
    "...k...k...k....";
static const char EGG[] =
    "....kkkk...."
    "...kcccck..."
    "..kccvccck.."
    ".kcccccvcck."
    ".kcvccccccK."
    ".kccccvcccK."
    ".kccvcccvcK."
    ".kcccccccKk."
    "..kKcccKKk.."
    "...kkkkkk...";
static const char EGG2[] =
    "....kkkk...."
    "...kcccck..."
    "..kccvccck.."
    ".kcccccvcck."
    ".kcvccrcccK."
    ".kcccrrcccK."
    ".kccvcrcvcK."
    ".kcccccccKk."
    "..kKcccKKk.."
    "...kkkkkk...";
static const char CROWN[] =
    "k.k.k.k."
    "kykakyk."
    "kyyyyyk."
    "kaaraak."
    "kkkkkkk.";
static const char ICON_MOVE[] =
    "..k....."
    "..kk...."
    "kkkzk..."
    "kzzzzk.."
    "kkkzk..."
    "..kk...."
    "..k.....";
static const char ICON_ATTACK[] =
    "...k...."
    "..kok..."
    ".koyok.."
    "koywyok."
    ".koyok.."
    "..kok..."
    "...k....";
static const char ICON_SPECIAL[] =
    "..kkk..."
    ".kCIIk.."
    "kCIwICk."
    "kuCICuk."
    ".kuuuk.."
    "..kBk..."
    "...k....";

typedef struct { int id, w, h; const char *px; } Art;
static const Art ART[] = {
    {GS_TILLY1, 16, 16, TILLY1}, {GS_TILLY2, 16, 16, TILLY2}, {GS_TILLY_HOP, 16, 16, TILLY_HOP},
    {GS_TILLY_DEAD, 16, 16, TILLY_DEAD}, {GS_POD1, 8, 11, POD1}, {GS_POD2, 8, 11, POD2},
    {GS_ADULT_SPARK, 16, 12, ADULT_SPARK}, {GS_ADULT_SHELL, 16, 12, ADULT_SHELL},
    {GS_ADULT_BURROW, 16, 12, ADULT_BURROW}, {GS_ADULT_MOUND, 16, 12, ADULT_MOUND},
    {GS_EGG, 12, 10, EGG}, {GS_EGG2, 12, 10, EGG2}, {GS_CROWN, 8, 5, CROWN},
    {GS_ICON_MOVE, 8, 7, ICON_MOVE}, {GS_ICON_ATTACK, 8, 7, ICON_ATTACK}, {GS_ICON_SPECIAL, 8, 7, ICON_SPECIAL},
};

static void make_larva(int id, char l, char g, char s) {
    char buf[sizeof LARVA];
    memcpy(buf, LARVA, sizeof LARVA);
    for (size_t i = 0; i < sizeof LARVA - 1; i++) {
        if (buf[i] == 'l') buf[i] = l;
        else if (buf[i] == 'g') buf[i] = g;
        else if (buf[i] == 's') buf[i] = s;
    }
    spr_make(&gs_spr[id], 12, 8, buf);
}

void gs_art_load(void) {
    static int loaded;
    if (loaded) return;
    for (int i = 0; i < ARRAY_LEN(ART); i++) {
        int len = (int)strlen(ART[i].px);
        if (len != ART[i].w * ART[i].h) fprintf(stderr, "grubshift art %d: expected %d got %d\n", ART[i].id, ART[i].w * ART[i].h, len);
        spr_make(&gs_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
    }
    make_larva(GS_LARVA_SPARK, 'c', 'y', 'a');
    make_larva(GS_LARVA_SHELL, 'I', 'C', 'u');
    make_larva(GS_LARVA_BURROW, 'h', 'e', 't');
    make_larva(GS_LARVA_MOUND, 'i', 'z', 'j');
    /* queens share the adult body; the renderer adds the crown and glow */
    gs_spr[GS_QUEEN_SPARK] = gs_spr[GS_ADULT_SPARK];
    gs_spr[GS_QUEEN_SHELL] = gs_spr[GS_ADULT_SHELL];
    gs_spr[GS_QUEEN_BURROW] = gs_spr[GS_ADULT_BURROW];
    gs_spr[GS_QUEEN_MOUND] = gs_spr[GS_ADULT_MOUND];
    loaded = 1;
}
