/* DUSKLING - pixel art (palette-letter strings, see gfx.h). All drawn for UFO 40. */
#include "duskling.h"

Sprite dk_spr[S_COUNT];

/* the duskling: a pink teardrop with a leaf on top, 9x10 */
static const char PIM1[] =
    "....zi..."
    "....z...."
    "..KKKK..."
    ".KKKKKKK."
    "KKKKKwwK."
    "KKKKKwkK."
    "PKKKKKKKK"
    "PPKKKKKP."
    ".PPPPPP.."
    "..k...k..";
static const char PIM2[] =
    "....zi..."
    "....z...."
    "..KKKK..."
    ".KKKKKKK."
    "KKKKKwwK."
    "KKKKKwkK."
    "PKKKKKKKK"
    "PPKKKKKP."
    ".PPPPPP.."
    ".k.....k.";
static const char PIM3[] =
    "...zi...."
    "....z...."
    "..KKKK..."
    ".KKKKKKK."
    "KKKKKwwK."
    "KKKKKwkK."
    "PKKKKKKKK"
    "PPKKKKKP."
    ".PPPPPP.."
    "...kk....";
static const char PIM_JUMP[] =
    "...zi...."
    "..KKKK..."
    ".KKKKKK.."
    "KKKKKwwK."
    "KKKKKwkK."
    "KKKKKKKKK"
    "PKKKKKKK."
    ".PPKKKP.."
    "..PPPP..."
    "..k..k...";
static const char PIM_FALL[] =
    "....z.i.."
    "....z...."
    ".KKKKKK.."
    "KKKKKwwK."
    "KKKKKwkKK"
    "PKKKKKKKK"
    "PPKKKKKP."
    ".PPPPPPP."
    ".k.....k."
    ".........";
static const char PIM_ROLL1[] =
    "........."
    "..KKKKK.."
    ".KKzKKKK."
    "KKKiKKwKK"
    "KKKKKKkKK"
    "KPKKKKKKK"
    "KPPKKKKPK"
    ".PPPPPPP."
    "..PPPPP.."
    ".........";
static const char PIM_ROLL2[] =
    "........."
    "..PPPPP.."
    ".PPKKKPP."
    "PKKKKKKPP"
    "KKkKKKKKK"
    "KKwKKKiKK"
    ".KKKKzKK."
    ".KKKKKKK."
    "..KKKKK.."
    ".........";
static const char PIM_POUND[] =
    "l..zi...l"
    "l..z....l"
    "..KKKK..."
    ".KKKKKK.."
    "KKKKkwkK."
    "KKKKKKKKK"
    "PKKKKKKKK"
    "PPKKKKKPP"
    ".PPPPPPP."
    "kk.....kk";
static const char PIM_SWIM[] =
    "........."
    ".....zi.."
    "...KKKK.."
    ".KKKKKwK."
    "KKKKKKkKK"
    "PKKKKKKKK"
    "kPKKKKKP."
    "..PPPPP.."
    "........."
    ".........";

static const char ORB[] =
    "..yyy.."
    ".ycccy."
    "ycwwwcy"
    "ycwwwcy"
    "ycwwwcy"
    ".ycccy."
    "..yyy..";

/* foes */
static const char PRICKLE1[] =
    "...g.g...."
    "..gsgsg.g."
    ".gsgsgsgs."
    "gsgsgsgsgs"
    "sgsgsgssts"
    "gsgsgsstkt"
    "sgsgsgtttt"
    ".tttttttt."
    ".h.h..h.h."
    "..........";
static const char PRICKLE2[] =
    "....g.g..."
    "..gsgsgsg."
    ".gsgsgsgs."
    "gsgsgsgsgs"
    "sgsgsgssts"
    "gsgsgsstkt"
    "sgsgsgtttt"
    ".tttttttt."
    "..h.h.h.h."
    "..........";
static const char WASP1[] =
    "..II.II..."
    "..IIIII..."
    "...II....."
    ".kaakak..."
    "kaakaakaw."
    "kaakaakkk."
    ".kaakak.k."
    "...k......"
    "..........";
static const char WASP2[] =
    ".........."
    ".........."
    "...II....."
    ".kaakak..."
    "kaakaakaw."
    "kaakaakkk."
    ".kaakak.k."
    ".IIIII...."
    "..II.II...";
static const char PUFF[] =
    "..l.l.l..."
    ".l.www.l.."
    "l.wwwww.l."
    ".wwwcwww.."
    "l.wwwww.l."
    ".l.www.l.."
    "..l.t.l..."
    "....t....."
    "....t....."
    "..........";
static const char FROG1[] =
    ".........."
    ".........."
    ".........."
    ".wk....kw."
    ".jjj..jjj."
    "jjjjjjjjjj"
    "jjzzzzzzjj"
    "jjjzzzzjjj"
    "fj.jjjj.jf"
    "ff......ff";
static const char FROG2[] =
    ".wk....kw."
    ".jjj..jjj."
    "jjjjjjjjjj"
    "jjzzzzzzjj"
    "jjjzzzzjjj"
    ".jjjjjjjj."
    ".j.j..j.j."
    "fj......jf"
    "f........f"
    "..........";
static const char EYE[] =
    "..gggggg.."
    ".gsssssg.."
    "gsllllllsg"
    "slwwwwwwls"
    "slwkkkkwls"
    "slwkrrkwls"
    "slwwwwwwls"
    "gsllllllsg"
    ".gssssssg."
    "..gggggg..";
static const char EYE_OPEN[] =
    "..gggggg.."
    ".gsssssg.."
    "gsllllllsg"
    "slwwwwwwls"
    "slwkrrkwls"
    "slwrrrrwls"
    "slwkrrkwls"
    "gswwwwwwsg"
    ".gssssssg."
    "..gggggg..";
static const char BEETLE1[] =
    ".........."
    ".........."
    "...tttt..."
    "..tctcttk."
    ".tctctctkk"
    "tctctctttk"
    "hhhhhhhhhh"
    ".h..h..h.."
    ".........."
    "..........";
static const char BEETLE2[] =
    ".........."
    ".........."
    "...tttt..."
    "..tctcttk."
    ".tctctctkk"
    "tctctctttk"
    "hhhhhhhhhh"
    "..h..h..h."
    ".........."
    "..........";
static const char BEETLE_BALL[] =
    "...tttt..."
    ".ttctctt.."
    "ttctctcttt"
    "tctctctctt"
    "ctctctcttc"
    "tctctctctt"
    "ttctctcttt"
    ".ttctctth."
    "..hhhhhh.."
    "..........";
static const char NEWT1[] =
    "...zzz...."
    "..zwkzz..."
    "..zzzzz..."
    "...zjz...."
    "..zjjjz.l."
    ".zjjjjjzl."
    "..zjjjz.l."
    "..zj.jz.l."
    "..z...zzl."
    ".zz....l..";
static const char NEWT2[] =
    "...zzz...."
    "..zwkzz..."
    "..zzzzz..."
    "...zjz.l.."
    "..zjjjzl.."
    ".zjjjjjl.."
    "..zjjjz..."
    "..zj.jz..."
    "..z...z..."
    ".zz....zz.";
static const char SPEAR[] =
    "lllllllllg"
    "..........";
static const char CROW1[] =
    ".........."
    "...kkk...."
    "..kkwkk..."
    ".gkkkkkaa."
    "..kkkkk..."
    ".kkkkkkk.."
    "kkkkkkkk.."
    ".kkkkkk..."
    "...a.a...."
    "..aa.aa...";
static const char CROW2[] =
    "k........k"
    "kk.kkk..kk"
    ".kkkwkkkk."
    ".gkkkkkaa."
    "..kkkkk..."
    "..kkkkk..."
    "...kkkk..."
    "....kk...."
    "...a.a...."
    "..aa.aa...";
static const char FISH1[] =
    ".........."
    ".........."
    "o...ooo..."
    "oo.ooooww."
    ".ooooookw."
    "oo.oooooo."
    "o...ooo..."
    ".........."
    ".........."
    "..........";
static const char FISH2[] =
    ".........."
    ".........."
    "....ooo..."
    "o..ooooww."
    "oooooookw."
    "o..oooooo."
    "....ooo..."
    ".........."
    ".........."
    "..........";

/* the world */
static const char FACE[] =
    "..gggg...."
    ".gsssss..."
    ".gkssks..."
    ".gsssss..."
    ".gssgss..."
    ".gskkks..."
    "..gssss..."
    "..gsss...."
    ".gsssss..."
    "gsssssss..";
static const char FACE_AWAKE[] =
    "..gggg...."
    ".gsssss..."
    ".gCssCs..."
    ".gsssss..."
    ".gssgss..."
    ".gskkks..."
    "..gssss..."
    "..gsss...."
    ".gsssss..."
    "gsssssss..";
static const char BELL[] =
    "..B....."
    ".BuB..B."
    ".BuB.BuB"
    "..j..BuB"
    "..j...j."
    "..j..j.."
    "...jj..."
    "...j....";
static const char POPPY[] =
    ".oo....."
    "oyoo..oo"
    ".oo..oyo"
    "..j...o."
    "..j..j.."
    "...jj..."
    "...j...."
    "...j....";
static const char EGG[] =
    "....wwww...."
    "...wwwwww..."
    "..wwwwwwww.."
    "..wwwwwlww.."
    ".wwwwwwwlww."
    ".wwlwwwwwww."
    ".wwwwwwwwww."
    "wwwwwwwlwwww"
    "wwwwwwwwwwww"
    "wlwwwwwwwwlw"
    "wwwwwwwwwwww"
    ".wwwwwwwwwl."
    "..wlwwwwww.."
    "...gggggg...";
static const char EGG_CRACK[] =
    "............"
    "............"
    "............"
    "............"
    "..w.w..w.w.."
    ".wkwkwwkwkw."
    ".wwwkwwkwww."
    "wwwwwwwlwwww"
    "wwwwwwwwwwww"
    "wlwwwwwwwwlw"
    "wwwwwwwwwwww"
    ".wwwwwwwwwl."
    "..wlwwwwww.."
    "...gggggg...";
static const char TENT[] =
    ".........k.........."
    "........kek........."
    ".......kebek........"
    "......kebbbek......."
    ".....kebbbbbek......"
    "....kebbbkbbbek....."
    "...kebbbkkkbbbek...."
    "..kebbbkkkkkbbbek..."
    ".kebbbkkkakkkbbbek.."
    "kebbbbkkkkkkkbbbbek."
    "eeeeeeeeeeeeeeeeeee."
    "....................";

/* bosses */
static const char WARDEN1[] =
    "......gggggg......"
    ".....gllllllg....."
    "....gllCCCCllg...."
    "....glCwwwwClg...."
    "....glCwkkwClg...."
    "....gllCCCCllg...."
    ".....gllllllg....."
    "...gggsssssssggg.."
    "..glllgsssssglllg."
    ".glsslgaaaaaglssl."
    ".gls.lgsssssgl.sl."
    ".gls.lgaaaaagl.sl."
    ".gls.lgsssssgl.sl."
    ".gyy.lgggggggl.yy."
    "..y..gssssssgg..y."
    "....gsss..sssg...."
    "....gss....ssg...."
    "....gss....ssg...."
    "...gsss....sssg..."
    "...gggg....gggg...";
static const char WARDEN2[] =
    "......gggggg......"
    ".....gllllllg....."
    "....gllCCCCllg...."
    "....glCwwwwClg...."
    "....glCwkkwClg...."
    "....gllCCCCllg...."
    ".....gllllllg....."
    "...gggsssssssggg.."
    "..glllgsssssglllg."
    ".glsslgaaaaaglssl."
    ".gls.lgsssssgl.sl."
    ".gls.lgaaaaagl.sl."
    ".gls.lgsssssgl.sl."
    ".gyy.lgggggggl.yy."
    "..y..gssssssgg..y."
    ".....gss..ssg....."
    "....gss....ssg...."
    "...gss......ssg..."
    "..gsss......sssg.."
    "..gggg......gggg..";
static const char HERMIT1[] =
    "......vvv......."
    ".....vVVVv......"
    "....vVVVVVv....."
    "...vVVkkkVVv...."
    "...vVkykykVv...."
    "...vVkkkkkVv...."
    "..vVVVkkkVVVv..."
    ".vVVVVVVVVVVVv.."
    ".vVVcVVVVVcVVv.."
    "vVVVcVVVVVcVVVv."
    "vVVVVVVVVVVVVVv."
    "vVVVVVVVVVVVVVv."
    ".vVVVVVVVVVVVv.."
    ".vVVVVVVVVVVVv.."
    "..vvVVVVVVVvv..."
    "....vvvvvvv.....";
static const char HERMIT2[] =
    "......vvv......."
    ".....vVVVv......"
    "....vVVVVVv....."
    "...vVVkkkVVv...."
    "...vVkykykVv...."
    "...vVkkkkkVv...."
    "o.vVVVkkkVVVv..o"
    "ovVVVVVVVVVVVvoa"
    "avVVcVVVVVcVVva."
    ".VVVcVVVVVcVVV.."
    "vVVVVVVVVVVVVVv."
    "vVVVVVVVVVVVVVv."
    ".vVVVVVVVVVVVv.."
    ".vVVVVVVVVVVVv.."
    "..vvVVVVVVVvv..."
    "....vvvvvvv.....";
static const char BADGER1[] =
    "........................"
    "..................kk...."
    "................kkwwk..."
    "..gggggggggggg.kwwkwwk.."
    ".glllllllllllggkwwwwwkkk"
    "glllllllllllllgkkwwwkkkk"
    "gllgggggggggglgkkkkkkkk."
    "glgssssssssssgggkkkkk..."
    "ggsssssssssssssgg......."
    "gssssssssssssssssg......"
    "gssssssssssssssssg......"
    "gssssssssssssssssg......"
    ".gssssssssssssssg......."
    "..ggsgggggggsggg........"
    "..kkk.kk...kk.kkk......."
    "..kk..kk...kk..kk......."
    ".kkk.kkk..kkk.kkk......."
    "........................";
static const char BADGER2[] =
    "........................"
    "..................kk...."
    "................kkwwk..."
    "..gggggggggggg.kwwkwwk.."
    ".glllllllllllggkwwwwwkkk"
    "glllllllllllllgkkwwwkkkk"
    "gllgggggggggglgkkkkkkkk."
    "glgssssssssssgggkkkkk..."
    "ggsssssssssssssgg......."
    "gssssssssssssssssg......"
    "gssssssssssssssssg......"
    "gssssssssssssssssg......"
    ".gssssssssssssssg......."
    "..ggsgggggggsggg........"
    "...kkk.kk.kk.kkk........"
    "...kk..kk.kk..kk........"
    "..kkk.kkk.kkk.kkk......."
    "........................";
static const char BADGER_JUMP[] =
    "..................kk...."
    "................kkwwk..."
    "..gggggggggggg.kwwkwwk.."
    ".glllllllllllggkwwwwwkkk"
    "glllllllllllllgkkwwwkkkk"
    "gllgggggggggglgkkkkkkkk."
    "glgssssssssssgggkkkkk..."
    "ggsssssssssssssgg......."
    "gssssssssssssssssg......"
    "gssssssssssssssssg......"
    "gssssssssssssssssg......"
    ".gssssssssssssssg......."
    "..ggsgggggggsggg........"
    ".kkk..........kkk......."
    "kkk............kkk......"
    "kk..............kk......"
    "........................"
    "........................";
static const char FIRE1[] =
    "..y..."
    ".yay.."
    "yaoay."
    "aoroa."
    ".aoa.."
    "..a...";
static const char FIRE2[] =
    "...y.."
    "..yay."
    ".yaoay"
    ".aoroa"
    "..aoa."
    "...a..";
static const char SPARK[] =
    "..C..."
    ".CwC.."
    "CwwwCI"
    ".CwC.."
    "..C.I."
    "......";
static const char TREE[] =
    "..ff.."
    ".ffff."
    "ffffff"
    ".ffff."
    "..b..."
    "..b...";
static const char MOTH[] =
    ".ll......ll."
    "lcll....llcl"
    "lccll..llccl"
    "lccclttlcccl"
    ".llclttlcll."
    "..lllttlll.."
    "..l..tt..l.."
    "......k.....";
static const char HELM[] =
    "...gggg..."
    "..glllgg.."
    ".gllllllg."
    ".glkkkklg."
    ".gkk.kkkg."
    ".glkkkklg."
    "..gggggg.."
    ".....l...."
    "....l.l..."
    "...l...l..";

typedef struct { int id, w, h; const char *data; } Def;
static const Def DEFS[] = {
    {S_PIM1, 9, 10, PIM1}, {S_PIM2, 9, 10, PIM2}, {S_PIM3, 9, 10, PIM3},
    {S_PIM_JUMP, 9, 10, PIM_JUMP}, {S_PIM_FALL, 9, 10, PIM_FALL},
    {S_PIM_ROLL1, 9, 10, PIM_ROLL1}, {S_PIM_ROLL2, 9, 10, PIM_ROLL2},
    {S_PIM_POUND, 9, 10, PIM_POUND}, {S_PIM_SWIM, 9, 10, PIM_SWIM},
    {S_ORB, 7, 7, ORB},
    {S_PRICKLE1, 10, 10, PRICKLE1}, {S_PRICKLE2, 10, 10, PRICKLE2},
    {S_WASP1, 10, 9, WASP1}, {S_WASP2, 10, 9, WASP2},
    {S_PUFF, 10, 10, PUFF}, {S_FROG1, 10, 10, FROG1}, {S_FROG2, 10, 10, FROG2},
    {S_EYE, 10, 10, EYE}, {S_EYE_OPEN, 10, 10, EYE_OPEN},
    {S_BEETLE1, 10, 10, BEETLE1}, {S_BEETLE2, 10, 10, BEETLE2}, {S_BEETLE_BALL, 10, 10, BEETLE_BALL},
    {S_NEWT1, 10, 10, NEWT1}, {S_NEWT2, 10, 10, NEWT2}, {S_SPEAR, 10, 2, SPEAR},
    {S_CROW1, 10, 10, CROW1}, {S_CROW2, 10, 10, CROW2}, {S_FISH1, 10, 10, FISH1}, {S_FISH2, 10, 10, FISH2},
    {S_FACE, 10, 10, FACE}, {S_FACE_AWAKE, 10, 10, FACE_AWAKE}, {S_BELL, 8, 8, BELL}, {S_POPPY, 8, 8, POPPY},
    {S_EGG, 12, 14, EGG}, {S_EGG_CRACK, 12, 14, EGG_CRACK}, {S_TENT, 20, 12, TENT},
    {S_WARDEN1, 18, 20, WARDEN1}, {S_WARDEN2, 18, 20, WARDEN2},
    {S_HERMIT1, 16, 16, HERMIT1}, {S_HERMIT2, 16, 16, HERMIT2},
    {S_BADGER1, 24, 18, BADGER1}, {S_BADGER2, 24, 18, BADGER2}, {S_BADGER_JUMP, 24, 18, BADGER_JUMP},
    {S_FIRE1, 6, 6, FIRE1}, {S_FIRE2, 6, 6, FIRE2}, {S_SPARK, 6, 6, SPARK}, {S_TREE, 6, 6, TREE},
    {S_MOTH, 12, 8, MOTH}, {S_HELM, 10, 10, HELM},
};

void dk_art_load(void) {
    if (dk_spr[S_PIM1].px) return;
    for (int i = 0; i < ARRAY_LEN(DEFS); i++) {
        const Def *d = &DEFS[i];
        if ((int)strlen(d->data) != d->w * d->h) {
            fprintf(stderr, "duskling sprite %d is %d chars, not %dx%d\n", d->id, (int)strlen(d->data), d->w, d->h);
            continue;
        }
        spr_make(&dk_spr[d->id], d->w, d->h, d->data);
    }
}
