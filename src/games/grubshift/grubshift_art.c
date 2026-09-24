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
static const char ADULT_HIVE[] =
    "....ll....ll...."
    "...lwwl..lwwl..."
    "....lllkklll...."
    "....kkaaaakk...."
    "...kaoaoaoaok..."
    "..kaywkaakwyak.."
    "..kaykkaakkyak.."
    "..koaoaoaoaook.."
    "...kaaaaaaaak..."
    "....kkooookk...."
    "....k.k..k.k...."
    "...k..k..k..k...";
static const char ADULT_SOUR[] =
    "................"
    ".....kkkkkk....."
    "...kkiiizzzkk..."
    "..kiiizzzzzzjk.."
    ".kiiwkzzzwkzzjk."
    ".kizkkzVzkkzzjk."
    ".kzzzzzzzzzVzjk."
    ".kjzVzzkkzzzjjk."
    "..kjjzzzzzjjjk.."
    "...kkjjjjjjkk..."
    "....k..k..k....."
    "...z....z...z...";
static const char DRONE1[] =
    "ll....ll"
    ".lwkkwl."
    "..kook.."
    ".kayyak."
    ".kyaayk."
    "..kaak.."
    "...kk..."
    "........";
static const char DRONE2[] =
    "........"
    ".lwkkwl."
    "llkookll"
    ".kayyak."
    ".kyaayk."
    "..kaak.."
    "...kk..."
    "........";
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
    {GS_ADULT_MOUND, 16, 12, ADULT_MOUND}, {GS_ADULT_HIVE, 16, 12, ADULT_HIVE},
    {GS_ADULT_SOUR, 16, 12, ADULT_SOUR}, {GS_DRONE1, 8, 8, DRONE1}, {GS_DRONE2, 8, 8, DRONE2},
    {GS_EGG, 12, 10, EGG}, {GS_EGG2, 12, 10, EGG2}, {GS_CROWN, 8, 5, CROWN},
    {GS_ICON_MOVE, 8, 7, ICON_MOVE}, {GS_ICON_ATTACK, 8, 7, ICON_ATTACK}, {GS_ICON_SPECIAL, 8, 7, ICON_SPECIAL},
};

/* Copy a sprite string, swapping palette letters: map is "fromto fromto ...". */
static void make_recolor(int id, int w, int h, const char *src, const char *map) {
    char buf[512];
    size_t n = strlen(src);
    if (n >= sizeof buf) return;
    memcpy(buf, src, n + 1);
    for (size_t i = 0; i < n; i++)
        for (const char *m = map; m[0] && m[1]; m += 2)
            if (src[i] == m[0]) { buf[i] = m[1]; break; }
    spr_make(&gs_spr[id], w, h, buf);
}

void gs_art_load(void) {
    static int loaded;
    if (loaded) return;
    for (int i = 0; i < ARRAY_LEN(ART); i++) {
        int len = (int)strlen(ART[i].px);
        if (len != ART[i].w * ART[i].h) fprintf(stderr, "grubshift art %d: expected %d got %d\n", ART[i].id, ART[i].w * ART[i].h, len);
        spr_make(&gs_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
    }
    /* larvae show their colour; what they grow into depends on the contract */
    make_recolor(GS_LARVA_GOLD, 12, 8, LARVA, "lcgysa");
    make_recolor(GS_LARVA_LEAF, 12, 8, LARVA, "lizzsj");
    make_recolor(GS_LARVA_SKY, 12, 8, LARVA, "lIgCsu");
    /* the burrower belongs to the sky colour */
    make_recolor(GS_ADULT_BURROW, 16, 12, ADULT_BURROW, "eutBhCbN");
    /* sour pods: the fizz spoiled to a bruised violet */
    make_recolor(GS_SOUR1, 8, 11, POD1, "iVzpjmwP");
    make_recolor(GS_SOUR2, 8, 11, POD2, "iVzpjmwP");
    loaded = 1;
}
