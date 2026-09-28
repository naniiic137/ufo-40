/* DOT & DASH - pixel art (palette-letter strings, see gfx.h), all drawn for
 * UFO 40, plus tile textures and bosses drawn with shapes. */
#include "dotdash.h"

Sprite dd_spr[S_COUNT];
uint8_t dd_tilepx[T_COUNT][4][64];

/* ---- Dot ------------------------------------------------------------- */
static const char DOT_STAND[] =
    "...yy..."
    "..eeey.."
    ".eeeee.."
    ".ehhkh.."
    ".ehhhh.."
    "..ehh..."
    "..qqq..."
    ".hqqqq.."
    ".hqqqqh."
    "..qqqq.."
    ".qqqqqq."
    "..h..h.."
    "..h..h.."
    ".kk.kk..";
static const char DOT_WALK1[] =
    "...yy..."
    "..eeey.."
    ".eeeee.."
    ".ehhkh.."
    ".ehhhh.."
    "..ehh..."
    "..qqq..."
    ".hqqqqh."
    "..qqqq.."
    "..qqqq.."
    ".qqqqqq."
    ".h...h.."
    "h....h.."
    "k....kk.";
static const char DOT_WALK2[] =
    "...yy..."
    "..eeey.."
    ".eeeee.."
    ".ehhkh.."
    ".ehhhh.."
    "..ehh..."
    "..qqq..."
    "..qqqh.."
    ".hqqqq.."
    "..qqqq.."
    ".qqqqqq."
    "...hh..."
    "...h.h.."
    "..kk.k..";
static const char DOT_JUMP[] =
    "...yy..."
    "..eeey.."
    ".eeeee.."
    ".ehhkh.h"
    ".ehhhhh."
    "h.ehh..."
    ".hqqq..."
    "..qqqq.."
    "..qqqq.."
    ".qqqqqq."
    "..h..h.."
    ".h....h."
    ".k.....k"
    "........";
static const char DOT_CROUCH[] =
    "........"
    "........"
    "........"
    "...yy..."
    "..eeey.."
    ".eeeee.."
    ".ehhkh.."
    ".ehhhh.."
    "..qqq..."
    ".hqqqqh."
    ".qqqqqq."
    "qqqqqqqq"
    ".hh..hh."
    ".kk..kk.";
static const char DOT_LIFT[] =
    ".h....h."
    ".h.yy.h."
    ".heeeyh."
    ".eeeeeh."
    ".ehhkh.."
    ".ehhhh.."
    "..ehh..."
    "..qqq..."
    "..qqqq.."
    "..qqqq.."
    ".qqqqqq."
    "..h..h.."
    "..h..h.."
    ".kk.kk..";
static const char DOT_HURT[] =
    "...yy..."
    "..eeey.."
    ".eeeee.."
    ".ehkhk.."
    ".ehhhh.."
    "..ekk..."
    "h.qqq.h."
    ".hqqqqh."
    "..qqqq.."
    "..qqqq.."
    ".qqqqqq."
    "..h..h.."
    "..h..h.."
    ".kk.kk..";
static const char DOT_KICK[] =
    "...yy..."
    "..eeey.."
    ".eeeee.."
    ".ehhkh.."
    ".ehhhh.."
    "..ehh..."
    "..qqq..."
    ".hqqqq.."
    "..qqqqh."
    "..qqqq.."
    ".qqqqqq."
    "..h..hhk"
    "..h....."
    ".kk.....";

/* ---- Dash --------------------------------------------------------------- */
static const char DASH1[] =
    "........ee.."
    "........ecce"
    "t......cckck"
    ".t.cccccccc."
    "..cceecccc.."
    "..cccccccc.."
    "..c.c..c.c.."
    "..k.k..k.k..";
static const char DASH2[] =
    "........ee.."
    "t.......ecce"
    ".t.....cckck"
    "..ccccccccc."
    "..cceecccc.."
    "..cccccccc.."
    "...cc..cc..."
    "...kk..kk...";
static const char DASH_SIT[] =
    "......ee.."
    "......ecce"
    ".....cckck"
    "t...cccc.."
    ".t.cceecc."
    "..ccccccc."
    "..ccccccc."
    "..cc..c.c."
    "..kk..k.k.";
static const char DASH_OUCH[] =
    "........ee.."
    "........ecce"
    "t......ckkkk"
    ".t.cccccccc."
    "..cceecccc.."
    "..cccccccc.."
    "..c.c..c.c.."
    "..k.k..k.k..";
static const char DASH_WINGS[] =
    "....wwwww.......ee.."
    "...wlllllw......ecce"
    "t...wllllw.....cckck"
    ".t...wwww..cccccccc."
    "..........cceecccc.."
    "..........cccccccc.."
    "..........c.c..c.c.."
    "..........k.k..k.k.."
    "....................";

/* ---- glints and prizes -------------------------------------------------- */
static const char GLINT1[] = "..y..""..yw.""yyyyy"".yyy.""..y..";
static const char GLINT5[] =
    "...y..."
    "..yyy.."
    ".yywyy."
    "yyyyyya"
    ".yyyya."
    "..yya.."
    "...a...";
static const char GLINT50[] =
    "....w...."
    "...yyy..."
    "..ywyyy.."
    ".yyyyyyy."
    "yywyyyyaa"
    ".yyyyyaa."
    "..yyyaa.."
    "...yaa..."
    "....a....";
static const char HEART[] =
    ".rr.rr."
    "rwrrrrr"
    "rrrrrrr"
    ".rrrrr."
    "..rrr.."
    "...r...";
static const char HALF[] =
    ".rr...."
    "rwrr..."
    "rrrr..."
    ".rrr..."
    "..rr..."
    "...r...";
static const char GIFT[] =
    "..y..y.."
    "...yy..."
    "PPPyyPPP"
    "PKPyyPKP"
    "PPPyyPPP"
    "PPPyyPPP"
    "PKPyyPKP"
    "PPPyyPPP";
static const char EGG[] =
    "..cc.."
    ".cwcc."
    "cwccci"
    "cccici"
    "ccicci"
    ".cccc."
    "..cc..";

/* ---- things to carry (8x8) --------------------------------------------- */
static const char *const OBJ[O_KINDS] = {
    [O_PEBBLE] = "........""..ggg..."".glllg..""gllgggs.""gglgggs.""gggggss."".ssssss.""........",
    [O_CRACKER] = "......y.""....yo..""...o...."".rrrrr..""rrwrrrr.""rrrrrrr.""rrrrrrr."".vvvvv..",
    [O_DART] = "BBBBBBBB""BuuuuuuB""BuuwuuuB""BwwwwwuB""BuuwuuuB""BuuuuuuB""BBBBBBBB""NNNNNNNN",
    [O_HEARTBOX] = "tttttttt""tbbbbbbt""tbrbrbbt""tbrrrbbt""tbbrbbbt""tbbbbbbt""tttttttt""eeeeeeee",
    [O_GIFTBOX] = "..y..y..""...yy...""PPPyyPPP""PKPyyPKP""PPPyyPPP""PPPyyPPP""PKPyyPKP""PPPyyPPP",
    [O_BOOMER] = "oooooooo""oaaaaaao""oaywwyao""oayyywao""oawwwyao""oaaaaaao""oooooooo""mmmmmmmm",
    [O_ROLLER] = "jjjjjjjj""jzzzzzzj""jzwwwwzj""jzwzzwzj""jzwwzwzj""jzzzzwzj""jjjjjjjj""ffffffff",
    [O_GLUE] = ".iiiiii.""iiwiiiii""iiiiiwii""iiiiiiii""iziiiziz""zzzzzzzz"".zzzzzz.""..f..f..",
    [O_HOURGLASS] = "bbbbbbbb"".IyyyyI.""..IyyI..""...II...""...II...""..IyyI..""bbbbbbbb""........",
    [O_QUAKE] = "eeeeeeee""ebbkbbbe""ebbkkbbe""ebbbkbbe""ebkkbbbe""ebbkbbbe""eeeeeeee""nnnnnnnn",
    [O_VENOM] = "pppppppp""pVwwwwVp""pVwkkwVp""pVwwwwVp""pVVwwVVp""pVwVVwVp""pppppppp""nnnnnnnn",
    [O_MOLDFRUIT] = "....f...""...fz...""..pppp..""..pVpVp.""..pppVp.""..pVpppp""...pppp.""........",
    [O_SPORE] = "........""..KKKK..""..KwKK..""..KKKK..""...cc...""...cc...""..cccc..""........",
    [O_PUPA] = "...bb...""..bttb..""..btbb..""..bbtb..""..btbb..""..bbtb..""...bb...""....b...",
    [O_HARD] = "NNNNNNNN""NBBBBBBN""NBuBBBBN""NBBBBBBN""NBBBBuBN""NBBBBBBN""NNNNNNNN""kkkkkkkk",
    [O_AXE] = "..sss...""..slls..""..slss..""...sb...""...b....""...b....""...b....""...e....",
    [O_POPGUN] = "........""..ssssss"".slllllk""sssssss.""..bb....""..bb....""..ee....""........",
    [O_SPECS] = "........""........""kk....kk""kIIkkIIk""kIIk.kIIk""kkk..kkk""........""........",
    [O_CATFOOD] = "..gggg..""lllllll.""lrrrrrl.""lrwwwrl.""lrrrrrl.""lllllll.""..gggg..""........",
    [O_GEAR] = "..g.g...""gggggg..""gglllgg.""glsslg..""gglslgg.""gglllgg."".gggg...""..g.g...",
    [O_JAR] = "..ssss..""..llll.."".l....l.""l......l""l......l""l......l"".llllll.""........",
    [O_JARWATER] = "..ssss..""..llll.."".l....l.""lIIIIIIl""lBBBBBBl""lBBBBBBl"".llllll.""........",
    [O_JARSLIME] = "..ssss..""..llll.."".l....l.""lKKKKKKl""lPPPPPPl""lPPPPPPl"".llllll.""........",
    [O_JARHONEY] = "..ssss..""..llll.."".l....l.""lyyyyyyl""laaaaaal""laaaaaal"".llllll.""........",
    [O_JARACID] = "..ssss..""..llll.."".l....l.""liiiiiil""lzzzzzzl""lzzzzzzl"".llllll.""........",
    [O_LETTER] = "........""wwwwwwww""wlwwwwlw""wwlwwlww""wwwllwww""wwwwwwww""wwwwwwww""........",
    [O_ARTIFACT] = "..ssss..""..gggg..""sgloglgs""sggoggys""sglgolgs"".sggggs.""..ssss..""........",
    [O_SCROLL] = "........""cyyyyyyc""ctttttyc""ctwwwtyc""cttttty.""cyyyyyyc""........""........",
    [O_BABY] = "...y....""...o....""..cwc...""..cwcc..""..ckck..""..cccc..""..cccc..""...cc...",
    [O_EGG] = "...ww...""..wwww..""..wcww..""..wwwc..""..cwww..""..wwww..""...ww...""........",
    [O_TWIG] = "......z.""......zi""....zzz."".zzzz...""zz..z...""....i...""........""........",
    [O_CRATE] = "bbbbbbbb""btttttbb""bbtbbtbb""bbbttbbb""bbtbbtbb""btttttbb""bbbbbbbb""eeeeeeee",
    [O_TABLET] = ".gggggg.""gllllllg""glsslslg""gllllllg""glslsslg""gllllllg"".gggggg.""........",
    [O_BLUEEYE] = "........""..BBBB..""..BwkB..""..BkkB..""..BBBB..""........""........""........",
    [O_SEED] = "........""...b....""..bhb...""..bhhb..""..bbhb..""...bb...""........""........",
    [O_REDEGG] = "...rr...""..rrrr..""..rwrr..""..rrrm..""..mrrr..""..rrrr..""...rr...""........",
    [O_THREAD] = "........""tttttt..""BBBBBB..""BuBuBu..""BBBBBB..""tttttt..""........""........",
    [O_BLUEDUST] = "........""..uuu..."".uBuBu..""uBuuBuu."".uuBuu..""..uuu...""........""........",
    [O_DRINK] = "...ss...""...cc...""..iiii..""..izzi..""..iKKi..""..izzi..""..iiii..""........",
    [O_BIGBANG] = "...y....""..y.....""rrrrrrr.""rwwrrrrr""rrrrrrrr""rryyrryy""rrrrrrr."".vvvvv..",
    [O_KEY] = "........"".yy.....""y..yyyyy"".yy..y.y""........""........""........""........",
    [O_CINDER] = "llllllll""lsslssll""llllllll""sslsslls""llllllll""lssllssl""llllllll""ssssssss",
};

/* ---- creatures (two frames each) ----------------------------------------- */
typedef struct FoeArt { int w, h; const char *a, *b; } FoeArt;
static const FoeArt FOEART[F_KINDS] = {
    [F_ANT] = {8, 6, "......k.""..k..kr.""rrrkrrr.""rrrrrrk.""k.k.k...""........",
                     "......k.""..k..kr.""rrrkrrr.""rrrrrrk."".k.k.k..""........"},
    [F_LANCER] = {8, 8, ".....s..""....ks..""..k.krr.""rrrkrrrs""rrrrrrks""rrrr....""k.k.k...""........",
                        ".....s..""....ks..""..k.krr.""rrrkrrrs""rrrrrrks""rrrr....("".k.k.k..""........"},
    [F_AXEANT] = {8, 7, "...ss...""...sb...""....bk..""eeekeee.""eeeeeek.""k.k.k...""........",
                        "..ss....""..sb....""...bk...""eeekeee.""eeeeeek."".k.k.k..""........"},
    [F_MOTH] = {10, 7, "l.......l.""ll.....ll.""lll.k.lll.""ggggtgggg.""lll...lll.""ll.....ll.""..........",
                       "..........""l.......l.""lll.k.lll.""ggggtgggg.""lll...lll.""..........""l.......l."},
    [F_BUZZER] = {8, 6, "..II....""..IIII..""kkkkkkr.""nkkkkkk.""..k.k...""........",
                        "........""..IIII..""kkkkkkr.""nkkkkkk.""..IIk...""........"},
    [F_FLUTTER] = {10, 8, "yy......yy""yyy....yyy""yayy.kyyay"".yyyyyyyy.""..yy.yyy..""..........""..........""..........",
                          "..........""yy......yy""yayy.kyyay"".yyyyyyyy.""yyy....yyy"".........."".........."".........."},
    [F_BUMBLE] = {8, 7, "...II...""..IIII..""kyykyyk.""ykyykyyk""kyykyyk."".k...k..""........",
                        "........""..IIII..""kyykyyk.""ykyykyyk""kyykyyk.""..k.k...""........"},
    [F_SPRING] = {7, 6, "....k..""..jjjj.""jjjjjkj"".jjjjj.""j.j.j..""j....j.",
                        "....k..""..jjjj.""jjjjjkj"".jjjjj.""..j.j..""..j.j.."},
    [F_NIP] = {6, 6, "..bb..""bbbbkb""bbbbbb"".bbbb.""b.b.b.""......",
                     "..bb..""bbbbkb""bbbbbb"".bbbb."".b.b.b""......"},
    [F_MITE] = {8, 6, "..gggg..""glgkkglg""gggggggg"".gggggg.""g.g..g.g""........",
                      "..gggg..""glgkkglg""gggggggg"".gggggg."".g.gg.g.""........"},
    [F_PAPERFISH] = {10, 5, "l.....llg.""llllllllkg""gllllllll."".l.l.l.l..""..........",
                            "......llg.""lllllllllkg""llllllll.."".l.l.l.l..""..........", },
    [F_GERM] = {8, 8, "i..i..i."".iiiiii.""iizizii.""izkizki."".iiiiii.""iiizzii."".iiiiii.""i..i..i.",
                      ".i..i..i""iiiiiii."".iziziii"".izkizki""iiiiiii."".iizzii.""iiiiii.."".i..i..."},
    [F_GERM2] = {8, 8, "V..V..V."".VVVVVV.""VVpVpVV.""VpkVpkV."".VVVVVV.""VVVppVV."".VVVVVV.""V..V..V.",
                       ".V..V..V""VVVVVVV."".VpVpVVV"".VpkVpkV""VVVVVVV."".VVppVV.""VVVVVV.."".V..V..."},
    [F_WIGGLER] = {10, 4, ".jj....jj.""jjjjjjjjjk"".jj.jj.jj.""..........",
                          "jj..jj..jj""jjjjjjjjjk""..jj..jj..""..........", },
    [F_GULP] = {12, 9, "....KKKK....""..KKKKKKKK..("".KKwKKKKwKK.""KKkKKKKKkKKK""KKKKKKKKKKKK""KKKPPPPPKKKK"".KKKKKKKKKK.""..KKKKKKKK..""............",
                       "............""...KKKKKK...""..KKwKKKwK..("".KKkKKKKkKK.""KKKKKKKKKKKK""KKKPPPPKKKKK""KKKKKKKKKKKK"".KKKKKKKKKK.""..KK....KK.."},
    [F_POD] = {8, 8, "..zzzz..("".zfzzfz.""zzzkkzzz""zzkyykzz""zzzkkzzz"".zzzzzz.""..ffff..""..ffff..",
                     "..zzzz..("".zfzzfz.""zzzkkzzz""zzkyykzz""zzzkkzzz"".zzzzzz.""..ffff..""..ffff.."},
    [F_TINMOUSE] = {10, 10, ".gg....gg.""glsg..gslg"".ggggggg..""..glgklgg.""..gglllg.k"".rgggggg..""ggsgggsg..""..gggggg..""..g....g..""..kk...kk.",
                            ".gg....gg.""glsg..gslg"".ggggggg..""..glgklgg.""..gglllg.k"".rgggggg..""ggsgggsg..""..gggggg...(""...g..g...""...kk.kk.."},
    [F_TINMAGE] = {10, 14, "....V.....""...VVV....""..VVVVV...""..VyVVV...("".VVVVVVV..""..gg.gg...(""..glkgl...""..gggggg..""..VVVVVV..("".VVVVVVVV.""VVVVVVVVVy"".VVVVVVV..""..VVVVVV..""...V..V...",
                           "....V.....""...VVV....""..VVVVV...""..VVVyV...("".VVVVVVV..""..gg.gg...(""..glkgl...""..gggggg..""..VVVVVV..(""yVVVVVVVV.""VVVVVVVVV."".VVVVVVV..""..VVVVVV..""...V..V..."},
    [F_WAXGUARD] = {8, 12, "...o....""...y....""..cwc...""..ckck..""..cccc..""rrcccc..""r.cccc..""..cccc..""..cccc..""..rrrr..""..c..c..""..c..c..",
                           "...y....""...o....""..cwc...""..ckck..""..cccc..""..ccccrr""..cccc.r""..cccc..""..cccc..""..rrrr..""..c..c..""...cc..."},
    [F_SPARK] = {6, 6, "y.y..y"".yyyy.""yywyy."".yyyyy""y.yy.y""..y...",
                       "..y.y.""yyyyy."".ywyyy""yyyyy."".y.yy.""y....y"},
    [F_BEETLE] = {10, 7, "...NNNN...""..NBBuBN..""kNBBBBBNk.""kNBuBBBNkk""kNNBBBNNk."".k.k.k.k..""..........",
                         "...NNNN...""..NBBuBN..""kNBBBBBNk.""kNBuBBBNkk""kNNBBBNNk.""k.k.k.k...""........."},
    [F_HOPPER] = {8, 8, "......k.""....zzzz""..zzzzkz""zzzzzzz.""z.zz.z..""z..z.z..(""z...z...""........",
                        "......k.""....zzzz""..zzzzkz""zzzzzzz.""z.zz.z..""z..z.z..(""z...z...""........"},
    [F_CLIMBER] = {8, 6, "..VVVV.."".VpVVpV.""VVVVVVVk"".VVVVVV.""v.v.v.v.""........",
                         "..VVVV.."".VpVVpV.""VVVVVVVk"".VVVVVV."".v.v.v.v""........"},
    [F_SPITTER] = {8, 8, "........""..oooo.."".oookoo.""ooooooor""oooo...."".oooooor""..o..o..""........",
                         "........""..oooo.."".oookoo.""oooooooo""oooooooo"".oooooo.""..o..o..""........"},
    [F_OCTO] = {10, 8, "...oooo...""..oooooo.."".oowoowoo."".ookooko..""..oooooo.."".o.o.o.o..""o.o.o.o.o.""..........",
                       "...oooo...""..oooooo.."".oowoowoo."".ookooko..""..oooooo..""..o.o.o.o."".o.o.o.o.o"".........."},
    [F_FACE] = {8, 8, "..tttt.."".ttttt..""tkttt...""ttttt...""trrtt...""tttt...."".tt.....""........",
                      "..tttt.."".ttttt..""tkttt...""ttttt...""ttrrt...""tttt...."".t.t....""........"},
    [F_SCAMPER] = {7, 7, "...y...""..yyy.."".yywyy.""yyyyyyy"".yyyyy.""..y.y.."".k...k.",
                         "...y...""..yyy.."".yywyy.""yyyyyyy"".yyyyy.""..y.y..""..k.k.."},
    [F_FERRY] = {16, 6, "..II......II...."".IIII....IIII...""kkkkkkkkkkkkkkr.""nkkkkkkkkkkkkkk.""..k.k....k.k....""................",
                        "................"".IIII....IIII...""kkkkkkkkkkkkkkr.""nkkkkkkkkkkkkkk.""..k.k....k.k....""................"},
    [F_TANK] = {12, 8, "....ggg.....""...gggggssss""..ggggggg..."".ggggggggg..""gggggggggggg""kgkgkgkgkgkg"".kkkkkkkkkk.""............",
                       "....ggg.....""...gggggssss""..ggggggg..."".ggggggggg..""gggggggggggg""gkgkgkgkgkgk"".kkkkkkkkkk.""............"},
    [F_PLANE] = {14, 6, "wwwwww........"".wwwwwwwww....""..wlwwwwwwwwww"".wwwwwwwww....""wwwwww........""..............",
                        ".wwwww........"".wwwwwwwww....""..wlwwwwwwwwww"".wwwwwwwww...."".wwwww........"".............."},
};

/* ---- people: species templates, then colours per person ---------------- */
enum { TP_OLD, TP_PROF, TP_BEETLE, TP_MOTH, TP_SPIDER, TP_GLOW, TP_BUNNY, TP_WORM, TP_MOSS, TP_SHREW, TP_WAX, TP_SPORE, TP_CAT, TP_TIN, TP_NIB, TP_HOP, TP_COUNT };
static const char *const TPL[TP_COUNT] = {
    [TP_OLD] =
    "..llll.."".llllll.""..hhhh.."".hkhhk.."".hhhhh..""..hhh...""..vvvv.."".vvvvvv.""hvvvvvvh"".vvvvvv."".vvvvvv."".vvvvvv..""..h..h..""..k..k..",
    [TP_PROF] =
    "........""..hhhh.."".hhhhhh."".hkkhkkh"".hhhhhh."".wwwwww.""..wwww..""..BBBB.."".hBBBBh.""..BBBB..""..BBBB..""..BBBB..""..N..N..""..k..k..",
    [TP_BEETLE] =
    "........""........""...kk...""..NBBN.."".NBuBBN.""NBBBBBBN""NBuBBBBN""NBBBBBBN""NBBBBuBN"".NBBBBN.""..NNNN..""..k..k..""..k..k..""........",
    [TP_MOTH] =
    "k......k"".k....k."".lllll..""llklkll."".lllll..""..lll...""llgggll.""lgggggll""llgggll.""lgggggl."".lgggl..""..ggg...""..g.g...""..k.k...",
    [TP_SPIDER] =
    "........""........""........""..nnnn.."".nnnnnn.""nnwnnwnn""nnnnnnnn""nnnnnnnn""k.nnnn.k""k.k..k.k"".k.k.k.k""k.k..k.k""........""........",
    [TP_GLOW] =
    "...yy...""..ywwy..""..ywwy..""...yy...""...ss...""..y..y..""...yy...""..y..y..""...yy...""..y..y..""...yy...""...ss...""..ssss..""........",
    [TP_BUNNY] =
    "..l..l..""..l..l..""..llll..("".llllll.""llklklll""llllllll""lllKKlll"".llllll.""llllllll""llllllll""llllllll"".llllll.""..l..l..""........",
    [TP_WORM] =
    "........""........""........""...ttt..""..tktkt."".tttttt."".ttKtt..""..ttt...""...tttt.""....tttt""...tttt.""..tttt..""..ttttt.""...ttt..",
    [TP_MOSS] =
    "..z..z..(""..zzzz.."".zzzzzz."".zfzzfz."".jkjjkj."".jjjjjj.""..jjjj..""..ffff.."".jffffj."".jffffj.""..ffff..""..ffff..""..j..j..""..j..j..",
    [TP_SHREW] =
    "........""..g..g..""..gggg..(""gggggggk""gggkgggk"".gggggKk""..gggg..""..gwwg..("".gwwwwg."".gwwwwg.""..gggg..""..gggg..""..g..g..""..k..k..",
    [TP_WAX] =
    "...y....""...o....""...k....""..cccc..""..ckck..""..cccc..""..cccc..("".ccccc..""..cccc..""..cccc..""..cccc..""..cccc..("".aaaaaa.""........",
    [TP_SPORE] =
    "........("".rrrrrr.""rrwrrwrr""rrrrrrrr""rwrrrrwr"".cccccc.""..cccc..""..ckcc..""..cccc..""..cccc..""..cccc..""..cccc..""..c..c..""..c..c..",
    [TP_CAT] =
    "..y.y.y.""..yyyyy."".a.....a"".aa...aa"".aaaaaaa"".akaaaka"".aaaKaaa""..aaaaa.""..aaaaa.""vaaaaaaa""vaaaaaaa""vvaaaaaa""..a.a.a.""..k.k.k.",
    [TP_TIN] =
    "........"".gg..gg.""glsggslg"".gggggg.""..gkgk..""..ggggg.""...gg...""..rrrr.."".grrrrg.""g.rrrr.g""..rrrr..""..gggg..""..g..g..""..k..k..",
    [TP_NIB] =
    "..ssss.."".ssssss.""ss.ww.ss""ss.kk.ss""ssssssss"".ssssss.""..ssss..""...ss...""..ssss.."".ssssss.""ssssssss""..ssss..""..s..s..""..k..k..",
    [TP_HOP] =
    "......k.""......k.""....zzz.""...zzkz.""..zzzzz."".zzzzz..""zzzzzz..""z.zzz...""z.z.z...""z.z..z..""z.z..z..""..z..z..""........""........",
};
typedef struct NpcLook { int tp; char f1, t1, f2, t2; } NpcLook;
static const NpcLook LOOK[N_KINDS] = {
    [N_GRANNY] = {TP_OLD, 'v', 'p', 0, 0}, [N_CRUMB] = {TP_PROF, 0, 0, 0, 0}, [N_VOLT] = {TP_BEETLE, 'B', 'j', 'u', 'i'},
    [N_PILGRIM] = {TP_MOTH, 'g', 't', 0, 0}, [N_SILK] = {TP_SPIDER, 'n', 'p', 0, 0}, [N_FILAMENT] = {TP_GLOW, 0, 0, 0, 0},
    [N_TOCK] = {TP_SPIDER, 'n', 'e', 'w', 'y'}, [N_MOTHERFLUFF] = {TP_BUNNY, 'l', 'g', 0, 0}, [N_TUFTY] = {TP_BUNNY, 0, 0, 0, 0},
    [N_PUFFIN] = {TP_BUNNY, 'l', 'I', 0, 0}, [N_INKY] = {TP_WORM, 't', 'K', 0, 0}, [N_COWPOKE] = {TP_SPIDER, 'n', 'b', 0, 0},
    [N_WEEPY] = {TP_SPIDER, 'n', 'N', 'w', 'u'}, [N_SOLDIER] = {TP_MOSS, 'f', 'e', 0, 0}, [N_SHREW] = {TP_SHREW, 0, 0, 0, 0},
    [N_SPINNER] = {TP_SPIDER, 'n', 's', 'w', 'o'},
    [N_TAPER] = {TP_WAX, 'a', 'b', 0, 0}, [N_TALLOW] = {TP_WAX, 'c', 'l', 'a', 'p'}, [N_SNUFF] = {TP_WAX, 'a', 'q', 0, 0},
    [N_SMUDGE] = {TP_WAX, 'c', 's', 'a', 'k'},
    [N_FERN] = {TP_MOSS, 'f', 'V', 0, 0}, [N_SPRIG] = {TP_MOSS, 'f', 'a', 0, 0}, [N_LICHEN] = {TP_MOSS, 'f', 'K', 0, 0},
    [N_MASON] = {TP_MOSS, 'f', 's', 0, 0}, [N_BRACKEN] = {TP_MOSS, 'f', 'b', 'z', 'i'},
    [N_CLOD] = {TP_MOSS, 'f', 'e', 'z', 'b'}, [N_SORREL] = {TP_MOSS, 'f', 'r', 0, 0}, [N_PEAT] = {TP_MOSS, 'f', 'N', 0, 0},
    [N_SMITH] = {TP_MOSS, 'f', 'k', 'j', 'g'},
    [N_WRIGGLA] = {TP_WORM, 't', 'a', 0, 0}, [N_KNOT] = {TP_WORM, 't', 'h', 0, 0}, [N_BORER] = {TP_WORM, 't', 'c', 0, 0},
    [N_OLDCAP] = {TP_SPORE, 'r', 'p', 0, 0}, [N_ONEEYE] = {TP_SPORE, 'r', 'B', 0, 0}, [N_SEEDKEEPER] = {TP_SPORE, 'r', 'f', 0, 0},
    [N_GILL] = {TP_SPORE, 'r', 'o', 0, 0}, [N_MOREL] = {TP_SPORE, 'r', 'e', 'w', 'y'},
    [N_HIGHLUMEN] = {TP_MOTH, 'g', 'y', 'l', 'w'}, [N_WARDEN] = {TP_MOTH, 'g', 'N', 0, 0},
    [N_TABITHA] = {TP_CAT, 0, 0, 0, 0}, [N_MAGE] = {TP_TIN, 'r', 'V', 0, 0}, [N_KNIGHT] = {TP_TIN, 'r', 's', 0, 0},
    [N_PELL] = {TP_WAX, 'a', 'u', 0, 0}, [N_RATCHET] = {TP_TIN, 'r', 'w', 0, 0},
    [N_NIB] = {TP_NIB, 0, 0, 0, 0}, [N_NATIVE] = {TP_MOSS, 0, 0, 0, 0}, [N_HOPPERSAGE] = {TP_HOP, 'z', 'q', 0, 0},
    [N_BUBBLE] = {TP_MOSS, 0, 0, 0, 0}, [N_WAXLING] = {TP_WAX, 0, 0, 0, 0},
    [N_BEE] = {TP_MOTH, 'g', 'y', 'l', 'k'}, [N_FACE] = {TP_NIB, 's', 't', 0, 0}, [N_EXILE] = {TP_MOTH, 'g', 'd', 0, 0},
    [N_DUMPER] = {TP_MOTH, 'g', 'a', 0, 0}, [N_EXILE2] = {TP_SPORE, 'r', 'n', 0, 0}, [N_HISTORIAN] = {TP_MOSS, 'f', 't', 0, 0},
    [N_PILOT] = {TP_BEETLE, 'B', 'e', 'u', 'y'}, [N_GLOW] = {TP_GLOW, 0, 0, 0, 0},
};

/* ---- odds and ends ------------------------------------------------------- */
static const char DOOR[] =
    "..eeee..""..eeee..("".ebbbbe.""ebbbbbbe""ebbeebbe""ebbeebbe""ebbbbbbe""ebbbbyye""ebbbbbbe""ebbeebbe""ebbeebbe""ebbbbbbe""ebbbbbbe""ebbbbbbe""ebbbbbbe""eeeeeeee";
static const char STAND[] = "tttttttttt"".bbbbbbbb.""..bbbbbb.."".eeeeeeee.";
static const char PEA[] = ".rr.""rwrr""rrrr"".rr.";
static const char SPELL[] = ".V..""VPVV"".VPV""..V.";
static const char BOLT[] = ".s..""sgss""sss."".s..";
static const char DROP[] = ".I.""III""IuI"".I.";
static const char SPROCKET_HEAD[] =
    "....gggg......""...gllllg.....""..glgggglg....""..gggggggg....(""..gkkgkkgg....""..gggggggg....""..gskksskg....""...gggggg.....""....rrrr......""..............""..............""..............""..............""..............";

/* ------------------------------------------------------------------ */
/* building sprites                                                      */

static void make_scaled(Sprite *dst, const char *src, int w, int h, int nw, int nh) {
    char *buf = (char *)malloc((size_t)(nw * nh + 1));
    for (int y = 0; y < nh; y++)
        for (int x = 0; x < nw; x++) buf[y * nw + x] = src[(y * h / nh) * w + (x * w / nw)];
    buf[nw * nh] = 0;
    spr_make(dst, nw, nh, buf);
    free(buf);
}

/* strings above may carry a stray '(' for readability of long rows: drop them */
static void make_clean(Sprite *dst, const char *src, int w, int h) {
    char *buf = (char *)malloc((size_t)(w * h + 1));
    int n = 0;
    for (const char *p = src; *p && n < w * h; p++)
        if (*p != '(') buf[n++] = *p;
    while (n < w * h) buf[n++] = '.';
    buf[n] = 0;
    spr_make(dst, w, h, buf);
    free(buf);
}

static Sprite npc_spr[N_KINDS];

static void build_npcs(void) {
    for (int n = 0; n < N_KINDS; n++) {
        const NpcLook *L = &LOOK[n];
        char buf[8 * 14 + 1];
        int k = 0;
        for (const char *p = TPL[L->tp]; *p && k < 8 * 14; p++) {
            if (*p == '(') continue;
            char c = *p;
            if (L->f1 && c == L->f1) c = L->t1;
            else if (L->f2 && c == L->f2) c = L->t2;
            buf[k++] = c;
        }
        while (k < 8 * 14) buf[k++] = '.';
        buf[k] = 0;
        spr_make(&npc_spr[n], 8, 14, buf);
    }
}

static void build_tiles(void) {
    for (int t = 0; t < T_COUNT; t++) {
        const TileInfo *I = &DD_TILE[t];
        for (int v = 0; v < 4; v++) {
            uint8_t *p = dd_tilepx[t][v];
            for (int y = 0; y < 8; y++)
                for (int x = 0; x < 8; x++) {
                    uint32_t h = dd_hash((uint32_t)(t * 64 + v), (uint32_t)(y * 8 + x));
                    uint8_t c = I->base;
                    switch (t) {
                    case T_WOOD: case T_WOODDK: case T_PLANK: case T_PENCIL: case T_CRACK:
                        if ((y + v) % 4 == 0 && h % 5) c = I->lo;
                        else if (h % 23 == 0) c = I->hi;
                        if (t == T_CRACK && (x == y || x == 7 - y / 2)) c = C_INK;
                        if (t == T_PENCIL && y == 7) c = I->lo;
                        break;
                    case T_WALL: case T_STONE: case T_CERAMIC: case T_BONE:
                        if (h % 9 == 0) c = I->lo;
                        else if (h % 13 == 0) c = I->hi;
                        if (t == T_CERAMIC && x == 3 + v && y > 4) c = I->lo;
                        break;
                    case T_SOIL: case T_ROOT:
                        if (h % 6 == 0) c = I->lo;
                        else if (h % 17 == 0) c = I->hi;
                        break;
                    case T_FABRIC: case T_FABRIC2:
                        c = ((x / 2 + y / 2) % 2) ? I->base : I->lo;
                        if ((x + y) % 4 == 0) c = I->hi;
                        break;
                    case T_BOOKR: case T_BOOKB: case T_BOOKG: case T_BOOKY: case T_BOOKW:
                        if (x == 0 || x == 7) c = I->lo;
                        else if (y == 2 || y == 5) c = v % 2 ? C_YELLOW : I->hi;
                        break;
                    case T_METAL: case T_BRASS: case T_GEAR:
                        if (x == 0 || y == 7) c = I->lo;
                        if ((x == 1 || x == 6) && (y == 1 || y == 6)) c = I->hi;
                        if (t == T_GEAR && (x + y) % 3 == 0) c = I->hi;
                        break;
                    case T_GLASS: case T_SHADE:
                        if (x - y == 2 || x - y == 3) c = I->hi;
                        else if (h % 11 == 0) c = I->lo;
                        break;
                    case T_DUST:
                        if (h % 3 == 0) c = I->hi;
                        else if (h % 5 == 0) c = I->lo;
                        break;
                    case T_CARD: case T_CARD2:
                        if (y % 4 == 3) c = I->lo;
                        break;
                    case T_FUR:
                        if ((x + v) % 3 == 0) c = I->lo;
                        else if (h % 7 == 0) c = I->hi;
                        break;
                    case T_CELL: case T_CELL2: {
                        int dx = x - 3 - (v & 1), dy = y - 3 - (v >> 1);
                        int d = dx * dx + dy * dy;
                        c = d < 4 ? I->hi : d < 9 ? I->base : I->lo;
                        break;
                    }
                    case T_WAX:
                        if (y > 4 && x == 2 + v) c = I->hi;
                        else if (h % 19 == 0) c = I->lo;
                        break;
                    case T_MOSS: case T_GLUE:
                        if (h % 4 == 0) c = I->hi;
                        else if (h % 7 == 0) c = I->lo;
                        break;
                    case T_LEAF:
                        c = y == 0 ? I->hi : y < 3 ? ((x + y) % 5 ? I->base : I->lo) : TRANSPARENT;
                        if (y == 2 && (x == 0 || x == 7)) c = TRANSPARENT;
                        break;
                    case T_THREAD: case T_STRAND:
                        c = y == 0 ? I->hi : y == 1 ? I->lo : TRANSPARENT;
                        break;
                    case T_LEDGE:
                        c = y == 0 ? I->hi : y < 3 ? (y == 2 ? I->lo : I->base) : TRANSPARENT;
                        break;
                    case T_SHROOM:
                        c = y < 4 ? ((x + y * 2) % 5 == 0 ? I->hi : I->base) : (x >= 3 && x <= 4) ? C_CREAM : TRANSPARENT;
                        if (y == 0 && (x == 0 || x == 7)) c = TRANSPARENT;
                        break;
                    case T_THORN: {
                        int tip = 7 - (x % 4 < 2 ? x % 4 : 3 - x % 4) * 3;
                        c = y >= tip ? (y == tip ? I->hi : I->base) : TRANSPARENT;
                        break;
                    }
                    case T_GOO: case T_SLIME:
                        c = y < 2 ? TRANSPARENT : y == 2 ? I->hi : (h % 9 == 0 ? I->lo : I->base);
                        break;
                    default:
                        if (h % 11 == 0) c = I->lo;
                        break;
                    }
                    p[y * 8 + x] = c;
                }
        }
    }
}

void dd_art_load(void) {
    static int loaded;
    if (loaded) return;
    loaded = 1;
    spr_make(&dd_spr[S_DOT_STAND], 8, 14, DOT_STAND);
    spr_make(&dd_spr[S_DOT_WALK1], 8, 14, DOT_WALK1);
    spr_make(&dd_spr[S_DOT_WALK2], 8, 14, DOT_WALK2);
    spr_make(&dd_spr[S_DOT_JUMP], 8, 14, DOT_JUMP);
    spr_make(&dd_spr[S_DOT_CROUCH], 8, 14, DOT_CROUCH);
    spr_make(&dd_spr[S_DOT_LIFT], 8, 14, DOT_LIFT);
    spr_make(&dd_spr[S_DOT_HURT], 8, 14, DOT_HURT);
    spr_make(&dd_spr[S_DOT_KICK], 8, 14, DOT_KICK);
    make_scaled(&dd_spr[S_DOTBIG_STAND], DOT_STAND, 8, 14, 10, 24);
    make_scaled(&dd_spr[S_DOTBIG_WALK1], DOT_WALK1, 8, 14, 10, 24);
    make_scaled(&dd_spr[S_DOTBIG_WALK2], DOT_WALK2, 8, 14, 10, 24);
    make_scaled(&dd_spr[S_DOTBIG_JUMP], DOT_JUMP, 8, 14, 10, 24);
    make_scaled(&dd_spr[S_DOTTINY_STAND], DOT_STAND, 8, 14, 4, 7);
    make_scaled(&dd_spr[S_DOTTINY_WALK1], DOT_WALK1, 8, 14, 4, 7);
    make_scaled(&dd_spr[S_DOTTINY_WALK2], DOT_WALK2, 8, 14, 4, 7);
    spr_make(&dd_spr[S_DASH1], 12, 8, DASH1);
    spr_make(&dd_spr[S_DASH2], 12, 8, DASH2);
    spr_make(&dd_spr[S_DASH_SIT], 10, 9, DASH_SIT);
    spr_make(&dd_spr[S_DASH_OUCH], 12, 8, DASH_OUCH);
    spr_make(&dd_spr[S_DASH_WINGS], 20, 9, DASH_WINGS);
    make_scaled(&dd_spr[S_DASHBIG], DASH1, 12, 8, 16, 11);
    spr_make(&dd_spr[S_GLINT1], 5, 5, GLINT1);
    spr_make(&dd_spr[S_GLINT5], 7, 7, GLINT5);
    spr_make(&dd_spr[S_GLINT50], 9, 9, GLINT50);
    spr_make(&dd_spr[S_HEART], 7, 6, HEART);
    spr_make(&dd_spr[S_HALF], 7, 6, HALF);
    spr_make(&dd_spr[S_GIFT], 8, 8, GIFT);
    spr_make(&dd_spr[S_EGG], 6, 7, EGG);
    for (int o = 0; o < O_KINDS; o++) make_clean(&dd_spr[S_OBJ0 + o], OBJ[o] ? OBJ[o] : "", 8, 8);
    for (int f = 0; f < F_KINDS; f++) {
        const FoeArt *A = &FOEART[f];
        if (!A->a) { make_clean(&dd_spr[S_FOE0 + f * 2], "", 8, 8); make_clean(&dd_spr[S_FOE0 + f * 2 + 1], "", 8, 8); continue; }
        make_clean(&dd_spr[S_FOE0 + f * 2], A->a, A->w, A->h);
        make_clean(&dd_spr[S_FOE0 + f * 2 + 1], A->b, A->w, A->h);
    }
    build_npcs();
    make_clean(&dd_spr[S_DOOR], DOOR, 8, 16);
    spr_make(&dd_spr[S_STAND], 10, 4, STAND);
    spr_make(&dd_spr[S_PEA], 4, 4, PEA);
    spr_make(&dd_spr[S_SPELL], 4, 4, SPELL);
    spr_make(&dd_spr[S_BOLT], 4, 4, BOLT);
    spr_make(&dd_spr[S_DROP], 3, 4, DROP);
    make_clean(&dd_spr[S_SPROCKET_HEAD], SPROCKET_HEAD, 14, 14);
    build_tiles();
}

/* ------------------------------------------------------------------ */
/* drawing helpers used by dotdash_draw.c                                */

void dd_draw_npc(int sub, int x, int y, int flip, int t) {
    const Sprite *s = &npc_spr[sub];
    int bob = (t / 30 + sub) % 2;
    if (sub == N_PUFFIN) {
        /* a big sick dust bunny */
        spr_draw_scaled(s, x - 4, y - 14, 2, flip ? 0 : SPR_FLIPX);
        if (!dd_flag(FL_PUFF_CLEAN) && (t / 10) % 3 == 0) gfx_pset(x + 2 + (t / 7) % 6, y - 10, C_INK);
        return;
    }
    if (sub == N_TABITHA) {
        spr_draw_scaled(s, x - 4, y - 14, 2, flip ? 0 : SPR_FLIPX);
        return;
    }
    if ((sub == N_FILAMENT || sub == N_GLOW) && (t / 4) % 2) { spr_draw_ex(s, x, y - bob, 0, NULL, C_WHITE); return; }
    if (sub == N_GLOW) gfx_dither_circle(x + 4, y + 4, 10, C_YELLOW, 6);
    spr_draw(s, x, y + (bob && sub != N_NATIVE ? 0 : 0), flip ? 0 : SPR_FLIPX);
    if (sub == N_CRUMB && dd_flag(FL_SPECS_GIVEN)) gfx_hline(x + 2, x + 6, y + 3, C_INK);
    if (sub == N_COWPOKE) { gfx_hline(x, x + 7, y + 3, C_BROWN); gfx_rect(x + 2, y, 4, 3, C_BROWN); }
    if (sub == N_INKY) { gfx_pset(x + 3, y + 4, C_INK); gfx_pset(x + 5, y + 4, C_INK); gfx_hline(x + 3, x + 5, y + 3, C_BLUE); }
    if (sub == N_WRIGGLA) { gfx_rect(x + 3, y + 1, 3, 2, C_YELLOW); gfx_pset(x + 4, y, C_YELLOW); }
    if (sub == N_BORER || sub == N_MOREL) gfx_rect(x + 2, y - 2, 4, 3, C_WHITE);
    if (sub == N_MAGE) { gfx_rect(x + 2, y - 2, 4, 3, C_VIOLET); gfx_pset(x + 3, y - 3, C_VIOLET); }
    if (sub == N_KNIGHT) gfx_rect(x + 3, y - 2, 2, 3, C_RED);
    if (sub == N_TOCK) gfx_circb(x + 4, y + 7, 5, C_YELLOW);
    if (sub == N_PELL) { gfx_vline(x - 1, y, y + 13, C_SLATE); gfx_vline(x + 8, y, y + 13, C_SLATE); }
    if (sub == N_WEEPY && (t / 12) % 2) gfx_pset(x + 2, y + 7, C_SKY);
    if (sub == N_PILGRIM || sub == N_HIGHLUMEN) if ((t / 8) % 2) gfx_pset(x + 4, y - 1, C_YELLOW);
}

/* a thing at full size: the same picture, twice as big */
void dd_draw_obj_big(int sub, int x, int y) {
    if (sub < 0 || sub >= O_KINDS) return;
    spr_draw_scaled(&dd_spr[S_OBJ0 + sub], x, y, 2, 0);
}

void dd_draw_obj(int sub, int x, int y, int param) {
    if (sub < 0 || sub >= O_KINDS) return;
    const Sprite *s = &dd_spr[S_OBJ0 + sub];
    if (sub == O_LETTER && param == 1) { spr_draw_ex(s, x, y, 0, NULL, -1); gfx_rect(x + 3, y + 3, 2, 2, C_PINK); return; }
    if (sub == O_PEBBLE && param == -2) { gfx_rect(x + 2, y + 4, 3, 3, C_GREY); return; }
    spr_draw(s, x, y, 0);
}

void dd_draw_boss(const Ent *e, int x, int y, int t) {
    bool flash = e->hurt_t > 0 && (t % 4) < 2;
    switch (e->sub) {
    case F_ROTIFER: {
        int c = flash ? C_WHITE : C_CYAN;
        gfx_dither_circle(x + 16, y + 12, 12, c, 12);
        gfx_circb(x + 16, y + 12, 12, C_TEAL);
        /* the whirling crown */
        for (int k = 0; k < 8; k++) {
            float a = (float)k * 0.785f + (float)t * 0.2f;
            gfx_pset(x + 10 + (int)(cosf(a) * 5), y + 2 + (int)(sinf(a) * 2), C_WHITE);
            gfx_pset(x + 22 + (int)(cosf(-a) * 5), y + 2 + (int)(sinf(-a) * 2), C_WHITE);
        }
        gfx_rect(x + 12, y + 16, 8, 4, C_TEAL);
        if (e->state == 1) {
            gfx_circ(x + 16, y + 11, 5, C_INK);
            gfx_rect(x + 14, y + 11, 4, 8 + (t / 6) % 3, C_PINK);
        } else {
            gfx_pset(x + 12, y + 9, C_INK);
            gfx_pset(x + 20, y + 9, C_INK);
        }
        break;
    }
    case F_EARWIG: {
        int c = flash ? C_WHITE : C_EARTH;
        int d = e->dir ? 1 : -1, hx = e->dir ? x + 34 : x + 6;
        for (int k = 0; k < 5; k++) gfx_rect(x + 4 + k * 6, y + 4, 7, 10, k % 2 ? c : C_BROWN);
        gfx_rect(x + 3, y + 5, 34, 2, C_TAN);
        gfx_circ(hx, y + 8, 5, c);
        gfx_pset(hx + d * 2, y + 6, C_RED);
        int px = e->dir ? x : x + 38;
        gfx_line(px, y + 6, px - d * 6, y + 1, C_BROWN);
        gfx_line(px, y + 11, px - d * 6, y + 15, C_BROWN);
        for (int k = 0; k < 6; k++) gfx_vline(x + 6 + k * 6, y + 14, y + 15 + (t / 5 + k) % 2, C_NIGHT);
        if (e->cmd) text_draw("?", hx - 2, y - 10, C_YELLOW);
        break;
    }
    case F_SPROCKET: {
        int c = flash ? C_WHITE : C_GREY;
        gfx_rect(x + 8, y + 32, 6, 16, C_SLATE);
        gfx_rect(x + 18, y + 32, 6, 16, C_SLATE);
        gfx_rect(x + 4, y + 16, 24, 18, c);
        gfx_rect(x + 12, y + 20, 8, 8, C_RED);
        gfx_rect(x, y + 18, 4, 12, C_SLATE);
        gfx_rect(x + 28, y + 18, 4, 12, C_SLATE);
        if (e->state == 0) {
            spr_draw(&dd_spr[S_SPROCKET_HEAD], x + 9, y + 2, 0);
        } else {
            gfx_rect(x + 13, y + 15, 6, 2, C_INK); /* the neck, open */
            float a = (float)e->st / 300.0f * 6.283f;
            int hx = x + 9 + (int)(sinf(a) * 90), hy = y - 20 - (int)(fabsf(sinf(a)) * 20);
            spr_draw(&dd_spr[S_SPROCKET_HEAD], hx, hy, 0);
        }
        gfx_hline(x + 4, x + 27, y + 16, C_LIGHT);
        break;
    }
    }
}
