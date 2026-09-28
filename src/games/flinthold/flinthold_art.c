/* FLINTHOLD - pixel art (palette-letter strings, see gfx.h), all drawn for
 * UFO 40. Walkers face right; the game flips them. The hunters are one body
 * with the thing each kind holds laid over it at load time. */
#include "flinthold.h"

Sprite fh_spr[FS_COUNT];
int fh_art_bad; /* sprites whose string is the wrong size (a test checks 0) */

/* ---- Pim, young chief of the Hearth Clan ---------------------------------- */
static const char PIM_D0[] =
    "......kkkk......"
    ".....kbbbbk.kk.."
    "....kbbbbbbkwwk."
    "...kbbbbbbbbkk.."
    "...kbbhhhhbbk..."
    "...kbhkhhkhbk..."
    "...kbhhhhhhbk..."
    "....khhrrhhk...."
    "....kkhhhhkk...."
    "...kkoooooookk.."
    "..khkoboooaokhk."
    "..khkooaoboakhk."
    "...kkoooooookk.."
    "....kboaoobk...."
    "....khk..khk...."
    "....kk....kk....";
static const char PIM_D1_LEGS[] =
    "...khk....khk..."
    "...kk......kk...";
static const char PIM_U0[] =
    "......kkkk......"
    ".....kbbbbk.kk.."
    "....kbbbbbbkwwk."
    "...kbbbbbbbbkk.."
    "...kbbbbbbbbk..."
    "...kbbbbbbbbk..."
    "...kbbbbbbbbk..."
    "....kbbbbbbk...."
    "....kkhhhhkk...."
    "...kkoooooookk.."
    "..khkoooaoookhk."
    "..khkobooaookhk."
    "...kkoooooookk.."
    "....kboaoobk...."
    "....khk..khk...."
    "....kk....kk....";
static const char PIM_S0[] =
    "......kkkk......"
    "....kkbbbbk....."
    "...kwkbbbbbk...."
    "..kwwkbbbbbbk..."
    "...kkbbbbhhhk..."
    "....kbbbhhkhk..."
    "....kbbbhhhhhk.."
    ".....kbbhhhrk..."
    ".....kkhhhhk...."
    "....kkoooook...."
    "...kooboohhk...."
    "...koaoookhhk..."
    "...kkooooookk..."
    "....kboaobk....."
    "....khk.khk....."
    "....kk..kk......";
static const char PIM_S1_LEGS[] =
    "...khk...khk...."
    "...kk.....kk....";
static const char PIM_THROW_S[] =
    "......kkkk......"
    "....kkbbbbk....."
    "...kwkbbbbbk...."
    "..kwwkbbbbbbk..."
    "...kkbbbbhhhk..."
    "....kbbbhhkhk..."
    "....kbbbhhhhhk.."
    ".....kbbhhhrk.kk"
    ".....kkhhhhkkhk."
    "....kkooooookk.."
    "...kooboooak...."
    "...koaoooook...."
    "...kkooooookk..."
    "....kboaobk....."
    "...khk...khk...."
    "...kk.....kk....";

/* ---- the hunters: one body, and what each kind holds ----------------------- */
static const char BODY[] =
    "................"
    "....kkkkk......."
    "...kbbbbbk......"
    "..kbbrrrbbk....."
    "..kbhhhhhbk....."
    "..khkhhkhhk....."
    "..khhhhhhhk....."
    "..kbbhhhbbk....."
    "...kbbbbbk......"
    "..kkttttkkk....."
    ".khkteetekhk...."
    ".khktteetkhk...."
    "..kkttttttk....."
    "...ktk.ktk......"
    "...khk.khk......"
    "...kk...kk......";
/* 5 x 16, laid over the body at x = 11 */
static const char IT_BONE[] =
    "....." "....." "....." "....." "....." "....." "....." "....."
    ".kk.." "kwwk." ".kwwk" "..kk." "....." "....." "....." ".....";
static const char IT_SPEAR[] =
    "..k.." ".kwk." ".klk." "..t.." "..t.." "..t.." "..t.." "..t.."
    "..t.." "..t.." "..t.." "..t.." "..t.." "..t.." "..t.." ".....";
static const char IT_BARB[] =
    "..k.." ".kwk." "krlrk" ".rtr." "..t.." "..t.." "..t.." "..t.."
    "..t.." "..t.." "..t.." "..t.." "..t.." "..t.." "..t.." ".....";
static const char IT_BOW[] =
    "....." "..kk." ".lkbk" ".l.kb" ".l..b" ".l..b" ".l..b" ".l..b"
    ".l..b" ".l.kb" ".lkbk" "..kk." "....." "....." "....." ".....";
static const char IT_SLING[] =
    "....." "....." "....." "....." "....." "....." "..kk." ".kggk"
    ".kglk" "..kk." "..e.." ".e..." "....." "....." "....." ".....";
static const char IT_HURLER[] =
    "....." "....." "....." "....." "....." ".kkk." "kgggk" "kglgk"
    "kgggk" ".kkk." "..e.." "..e.." ".kk.." "kgsk." ".kk.." ".....";
static const char IT_BOULDER[] =
    "....." "....." "....." "....." "....." "....." "....." "....."
    "....." ".kkk." "kgglk" "kgggk" "kgggk" "ksggk" ".kkk." ".....";
static const char IT_TORCH[] =
    "....." "....." "....." "..r.." ".rar." ".aya." "..k.." "..t.."
    "..t.." "..t.." "..t.." "..t.." "....." "....." "....." ".....";
static const char IT_BLAZE[] =
    "....." "..r.." ".rar." "rayar" "ryyyr" ".aya." ".kbk." "..t.."
    "..t.." "..t.." "..t.." "..t.." "....." "....." "....." ".....";
static const char IT_PITCH[] =
    "....." "....." "....." "....." "....." "....." "....." "k...k"
    "kkkkk" "kndnk" "knnnk" ".kkk." "..n.." "....." "..n.." ".....";

/* ---- hens and fire pits --------------------------------------------------- */
static const char HEN[] =
    "................"
    "................"
    "......kk........"
    ".....krrk......."
    "....kwwwwk......"
    "...kwwkwwwk....."
    "..kaawwwwwk....."
    "...kkwwwwwwk..kk"
    ".....kwwwwwwkkwk"
    "....kwwwlwwwwwwk"
    "....kwwlllwwwwk."
    "....kwwwwwwwwk.."
    ".....kwwwwwwk..."
    "......kkkkkk...."
    ".......a..a....."
    "......aa.aa.....";
static const char FIRE0[] =
    "................"
    "................"
    "................"
    "........r......."
    ".......rar......"
    "......raar......"
    ".....raayar....."
    ".....rayyar....."
    "......ayya......"
    "....ktbayabtk..."
    "...ktbbtbbbtk..."
    "..kgktbbbbtkgk.."
    "..kgggkkkkggk..."
    "...kgggggggk...."
    "................"
    "................";
static const char FIRE1[] =
    "................"
    "................"
    "................"
    "................"
    ".......r.r......"
    "......rarar....."
    ".....rayyar....."
    "......ayyar....."
    "......ayya......"
    "....ktbayabtk..."
    "...ktbbtbbbtk..."
    "..kgktbbbbtkgk.."
    "..kgggkkkkggk..."
    "...kgggggggk...."
    "................"
    "................";

/* ---- the island's beasts (all face right) ---------------------------------- */
static const char NIPPER0[] =
    "................" "................" "................" "................"
    "................" "................"
    "........kkk....."
    ".......kzzzk...."
    ".......kzwkzk..."
    "..k....kzzzzzk.."
    ".kzk..kjzzzkk..."
    ".kzzkkzzzzzk...."
    "..kzzzzzzzjk...."
    "...kjzzzzjk....."
    "....kzk.kzk....."
    "....kk..kk......";
static const char NIPPER1_LEGS[] =
    "...kzk...kzk...."
    "...kk.....kk....";
static const char REDBACK0[] =
    "................" "................"
    "..........kkkk.."
    ".........krrrrk."
    ".........krwkrrk"
    ".........krrrrrk"
    "..k.....krrrkkk."
    ".krk...krrrrk..."
    ".krrk.krrrrrrk.."
    "..krrkrrmrrrrk.."
    "...krrrrrrrrrk.."
    "....krrrrrrmk..."
    ".....kmrrrmk...."
    ".....krk.krk...."
    ".....krk.krk...."
    ".....kk..kk.....";
static const char REDBACK1_LEGS[] =
    "....krk...krk..."
    "....krk...krk..."
    "....kk....kk....";
static const char FANGCAT0[] =
    "................" "................" "................" "................"
    "................" "................"
    "............kk.."
    "..........kkaak."
    "..k......kaawak."
    ".kak.....kaaaakk"
    "..kakkkkkabaakwk"
    "...kabaabaaak.w."
    "...kaabaabaak..."
    "...kak.kak.kak.."
    "...kk..kk..kk..."
    "................";
static const char FANGCAT1_LEGS[] =
    "..kak..kak..kak."
    "..kk...kk...kk..";
static const char CLUBTAIL0[] =
    "................" "................" "................" "................"
    "................" "................"
    ".....kkkkkk....."
    "...kkgsgsgskk..."
    "..kgsgsgsgsgk.kk"
    "kkkgggggggggkkek"
    "kgktttttttttttek"
    "kkk.kttttttttk.."
    "....ktk..ktk...."
    "....kk...kk....."
    "................" "................";
static const char CLUBTAIL1_LEGS[] =
    "...ktk....ktk..."
    "...kk.....kk....";
static const char GLIDER0[] =
    "................" "................" "................"
    "Vk............kV"
    ".VVk........kVV."
    "..VVVk....kVVV.."
    "...kVVVkkVVVk..."
    "....kVVpVVkkkk.."
    ".....kpppkwaaak."
    "......kkk..kk..."
    "................" "................" "................" "................" "................" "................";
static const char GLIDER1[] =
    "................" "................" "................" "................" "................"
    "....kkk..kkk...."
    "..kkVVVkkVVVkk.."
    ".kVVVVVpVVVVkkk."
    "kVVk.kpppkwaaak."
    "Vk....kkk..kk..."
    "................" "................" "................" "................" "................" "................";
static const char GNAT0[] =
    "................" "................" "................" "................" "................"
    ".....II.II......"
    ".....IIkII......"
    "......kpkrr....."
    ".....kppk..r...."
    "....kpk.k......."
    "...k.k.........."
    "................" "................" "................" "................" "................";
static const char GNAT1[] =
    "................" "................" "................" "................" "................" "................"
    ".......k........"
    "....IIkpkrr....."
    "....IkppkI.r...."
    "....kpk.k......."
    "...k.k.........."
    "................" "................" "................" "................" "................";
static const char SHAG0[] =
    ".........kkkkkk........."
    ".......kkbtbbtbbkk......"
    ".....kkbtbbtbbtbbtkk...."
    "....kbbtbbtbbtbbtbbtk..."
    "...kbtbbtbbtbbtbbtbbtbk."
    "..kbbtbbtbbtbbtbbtbkwbk."
    "..kbtbbtbbtbbtbbtbbbbbk."
    ".kbbtbbtbbtbbtbbtbbtbbck"
    ".kbtbbtbbtbbtbbtbbtkbbcc"
    ".kbbtbbtbbtbbtbbtbbkbkcc"
    ".kbtbbtbbtbbtbbtbbk.bk.c"
    "..kbbtbbtbbtbbtbbk..bk.."
    "..kbtbbtbbtbbtbbbk..kbk."
    "...kbbtkkkkkkbbtk...kk.."
    "...kbbtk....kbbtk......."
    "...kbbtk....kbbtk......."
    "...kbbbk....kbbbk......."
    "...kkkkk....kkkkk.......";
static const char SHAG1_LEGS[] =
    "....kbbtk..kbbtk........"
    "....kbbtk..kbbtk........"
    "....kbbbk..kbbbk........"
    "....kkkkk..kkkkk........";

/* ---- villagers ------------------------------------------------------------- */
static const char BABY[] =
    "................" "................" "................" "................"
    "................" "................" "................"
    "......kkkk......"
    ".....kbbbbk....."
    ".....khkkhk....."
    ".....khhhhk....."
    "....kkaaaakk...."
    "...khkaaaakhk..."
    "....kkaaaakk...."
    ".....kk..kk....."
    "................";

/* ---- bits ------------------------------------------------------------------ */
static const char MEAT[] =
    "...kkk.."
    "..kotok."
    ".kooooak"
    ".kooaotk"
    "..koook."
    "..kwkk.."
    ".kwk...."
    ".kk.....";
static const char HEART[] =
    ".kk.kk."
    "krrkrrk"
    "krrrrrk"
    ".krrrk."
    "..krk.."
    "...k...";
static const char CAVE[] =
    ".........kkkkkk........."
    ".......kkgglllgkk......."
    ".....kkgggllgggsskk....."
    "....kgggglgggggsssgk...."
    "...kgglggggggsgggssgk..."
    "..kggggggsgggggggsssgk.."
    "..kgglgggkkkkkkggsssgk.."
    ".kggggggkkkkkkkkggssssk."
    ".kgglggkkkkkkkkkkgsssgk."
    "kggggggkkkkkkkkkkkggsssk"
    "kgsgggkkkkkkkkkkkkgsssgk"
    "kgggsgkkkkkkkkkkkkggsssk"
    "kssggskkkkkkkkkkkksgsssk"
    "ksssgskkkkkkkkkkkksssssk"
    "kkssssbkkkkkkkkkkbsssskk"
    "kkkkkkkkkkkkkkkkkkkkkkkk";

static void make(int id, int w, int h, const char *s) {
    if ((int)strlen(s) != w * h) { fh_art_bad++; fprintf(stderr, "flinthold: sprite %d is %d chars, not %d\n", id, (int)strlen(s), w * h); return; }
    spr_make(&fh_spr[id], w, h, s);
}

/* a copy of base with its last rows swapped for other legs */
static void make_legs(int id, int w, int h, const char *base, const char *legs) {
    static char buf[24 * 24 + 1];
    int n = (int)strlen(legs) / w;
    if ((int)strlen(base) != w * h || (int)strlen(legs) != n * w) { fh_art_bad++; return; }
    memcpy(buf, base, (size_t)(w * h));
    memcpy(buf + (h - n) * w, legs, (size_t)(n * w));
    buf[w * h] = 0;
    make(id, w, h, buf);
}

/* the hunter body with its line's headband and what it holds */
static void make_hunter(int id, const char *item, char band, bool tier2) {
    static char buf[16 * 16 + 1];
    if ((int)strlen(item) != 5 * 16) { fh_art_bad++; return; }
    memcpy(buf, BODY, 256);
    buf[256] = 0;
    for (int i = 0; i < 256; i++) {
        if (buf[i] == 'r') buf[i] = band;
        if (tier2 && buf[i] == 'e') buf[i] = 'w'; /* a bone necklace for the second rung */
    }
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 5; x++) {
            char c = item[y * 5 + x];
            if (c != '.') buf[y * 16 + 11 + x] = c;
        }
    make(id, 16, 16, buf);
}

void fh_art_load(void) {
    if (fh_spr[FS_PIM_D0].px) return;
    make(FS_PIM_D0, 16, 16, PIM_D0);
    make_legs(FS_PIM_D1, 16, 16, PIM_D0, PIM_D1_LEGS);
    make(FS_PIM_U0, 16, 16, PIM_U0);
    make_legs(FS_PIM_U1, 16, 16, PIM_U0, PIM_D1_LEGS);
    make(FS_PIM_S0, 16, 16, PIM_S0);
    make_legs(FS_PIM_S1, 16, 16, PIM_S0, PIM_S1_LEGS);
    make(FS_PIM_THROW_D, 16, 16, PIM_D0);
    make(FS_PIM_THROW_U, 16, 16, PIM_U0);
    make(FS_PIM_THROW_S, 16, 16, PIM_THROW_S);
    make(FS_PIM_HURT, 16, 16, PIM_D0);
    make_hunter(FS_THROWER, IT_BONE, 'e', false);
    make_hunter(FS_SPEAR, IT_SPEAR, 'r', false);
    make_hunter(FS_BARB, IT_BARB, 'r', true);
    make_hunter(FS_BOW, IT_BOW, 'r', true);
    make_hunter(FS_SLING, IT_SLING, 'g', false);
    make_hunter(FS_HURLER, IT_HURLER, 'g', true);
    make_hunter(FS_BOULDER, IT_BOULDER, 'g', true);
    make_hunter(FS_TORCH, IT_TORCH, 'o', false);
    make_hunter(FS_BLAZE, IT_BLAZE, 'o', true);
    make_hunter(FS_PITCH, IT_PITCH, 'o', true);
    make(FS_HEN, 16, 16, HEN);
    make(FS_FIRE0, 16, 16, FIRE0);
    make(FS_FIRE1, 16, 16, FIRE1);
    make(FS_NIPPER0, 16, 16, NIPPER0);
    make_legs(FS_NIPPER1, 16, 16, NIPPER0, NIPPER1_LEGS);
    make(FS_REDBACK0, 16, 16, REDBACK0);
    make_legs(FS_REDBACK1, 16, 16, REDBACK0, REDBACK1_LEGS);
    make(FS_FANGCAT0, 16, 16, FANGCAT0);
    {
        /* its legs sit one row up from the bottom */
        static char buf[257];
        memcpy(buf, FANGCAT0, 256);
        buf[256] = 0;
        memcpy(buf + 13 * 16, FANGCAT1_LEGS, 32);
        make(FS_FANGCAT1, 16, 16, buf);
    }
    make(FS_CLUBTAIL0, 16, 16, CLUBTAIL0);
    {
        static char buf[257];
        memcpy(buf, CLUBTAIL0, 256);
        buf[256] = 0;
        memcpy(buf + 12 * 16, CLUBTAIL1_LEGS, 32);
        make(FS_CLUBTAIL1, 16, 16, buf);
    }
    make(FS_GLIDER0, 16, 16, GLIDER0);
    make(FS_GLIDER1, 16, 16, GLIDER1);
    make(FS_GNAT0, 16, 16, GNAT0);
    make(FS_GNAT1, 16, 16, GNAT1);
    make(FS_SHAGTUSK0, 24, 18, SHAG0);
    make_legs(FS_SHAGTUSK1, 24, 18, SHAG0, SHAG1_LEGS);
    /* the Four Lords are drawn twice size from these, with crowns */
    make(FS_JAW0, 16, 16, REDBACK0);
    make_legs(FS_JAW1, 16, 16, REDBACK0, REDBACK1_LEGS);
    make(FS_PLATE0, 16, 16, CLUBTAIL0);
    {
        static char buf[257];
        memcpy(buf, CLUBTAIL0, 256);
        buf[256] = 0;
        memcpy(buf + 12 * 16, CLUBTAIL1_LEGS, 32);
        make(FS_PLATE1, 16, 16, buf);
    }
    make(FS_GALE0, 16, 16, GLIDER0);
    make(FS_GALE1, 16, 16, GLIDER1);
    make(FS_SNAP0, 16, 16, NIPPER0);
    make_legs(FS_SNAP1, 16, 16, NIPPER0, NIPPER1_LEGS);
    make_hunter(FS_ELDER, IT_SPEAR, 'w', true);
    make(FS_BABY, 16, 16, BABY);
    make(FS_HENWIFE, 16, 16, PIM_D0);
    make(FS_SCALEFOLK, 16, 16, NIPPER0);
    make(FS_TUSKLING, 24, 18, SHAG0);
    make(FS_MEAT, 8, 8, MEAT);
    make(FS_HEART, 7, 6, HEART);
    make(FS_PIM_BIG, 16, 16, PIM_D0);
    make(FS_CAVE, 24, 16, CAVE);
}
