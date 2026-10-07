/* SOUNDINGS - pixel art, drawn for UFO 40 (palette-letter strings, see
 * gfx.h): the diving axolotl, every creature (16 x 16, shown twice the size
 * in a fight; the Abbot and the Gloamheart's parts 32 x 32), chests, levers,
 * the three heads, the divers' faces, the raft's three pictures and the 8 x 8
 * icons of every item and relic. Item icons are drawn in light grey and
 * tinted with the item's element. */
#include "sdg.h"

enum { IC_POLE, IC_HAMMER, IC_SHIELD, IC_POTION, IC_EGG, IC_BOMB, IC_MIST, IC_BLOOD, IC_FINS,
       IC_FOAM, IC_GUM, IC_PEBBLE, IC_COG, IC_SPIRAL, IC_BONE, IC_SUN, IC_MOON, IC_COUNT };

typedef struct SprDef { int id, w, h; const char *px; } SprDef;

static const char SP_DIVER_PX[] =
    "................"
    "...........P.P.."
    "............PKP."
    "..K......KKKKKKP"
    ".KKK..KKKKKKKkKK"
    "KKPKKKKKKKKKKKKw"
    ".KPPKKKKKKKKKKK."
    "..K.PKKcccccKK.."
    ".....PK..P..PK.."
    ".....P...P...P.."
    "................"
    "................";
static const char SP_DIVER2_PX[] =
    "................"
    "...........P.P.."
    "............PKP."
    ".........KKKKKKP"
    "K.....KKKKKKKkKK"
    "KKKKKKKKKKKKKKKw"
    "KKPPKKKKKKKKKKK."
    ".K..PKKcccccKK.."
    "....PK...PK..P.."
    "...P......P....."
    "................"
    "................";
static const char SP_FIZZLE_PX[] =
    "................"
    "................"
    "................"
    ".........y......"
    "........bbbb...."
    ".......bbbbbb..."
    "..y...bbkbbbbb.."
    ".bbbbbbbbbbbbby."
    "bbtbbbbbbbbbbbb."
    ".btttbbbtttbb..."
    "..b..tttbb......"
    ".y......y......."
    "................"
    "................"
    "................"
    "................";
static const char SP_PRICKLE_PX[] =
    "................"
    ".......V........"
    "...V...V...V...."
    "....V..V..V....."
    ".....VpppV......"
    "..V.ppPPppp..V.."
    "...VpPPPPPpV...."
    "VVVpPPkPkPPpVVV."
    "...VpPPPPPpV...."
    "..V.ppPPPpp..V.."
    ".....VpppV......"
    "....V..V..V....."
    "...V...V...V...."
    ".......V........"
    "................"
    "................";
static const char SP_FROND_PX[] =
    "................"
    "..K..K....K..K.."
    "...K..K..K..K..."
    ".K..K.K..K.K..K."
    "..K..KKKKKK..K.."
    "...KKPKKKKPKK..."
    "....KPPKKPPK...."
    ".....PPPPPP....."
    "......PvvP......"
    "......PvvP......"
    "......vvvv......"
    ".....vvvvvv....."
    "....vvvmmvvv...."
    "....mmmmmmmm...."
    "................"
    "................";
static const char SP_NIPPER_PX[] =
    "................"
    "................"
    "................"
    "......ggg......."
    "....gggggggg...."
    "..ggggggggiggg.."
    ".gg..gggggggkgg."
    "ggg.ggggggggggw."
    "gggggggggggrww.."
    ".gg..grrrrrrrw.."
    "..g...rrrrrr...."
    "......g.g......."
    "................"
    "................"
    "................"
    "................";
static const char SP_GLOB_PX[] =
    "................"
    "................"
    "................"
    "................"
    "......zzzz......"
    "....zziiiizz...."
    "...ziiiiiiiiz..."
    "..ziiwkiiwkiiz.."
    "..ziiikiiikiiz.."
    ".zziiiiiiiiiizz."
    ".ziiiiifffiiiiz."
    ".zziiiiiiiiiizz."
    "..zzzziiiizzzz.."
    "....zzzzzzzz...."
    "................"
    "................";
static const char SP_SMOG_PX[] =
    "................"
    "................"
    ".......lll......"
    ".....lgggggl...."
    "...lggggggggl..."
    "..lgggkgggggggl."
    ".lggggggggggglll"
    "lggsgggsgggg.l.."
    ".lsssssssssl...."
    "..l..ss.ss......"
    "....s..s..s....."
    "...s..s..s......"
    "................"
    "................"
    "................"
    "................";
static const char SP_CLAMPER_PX[] =
    "................"
    "..rr........rr.."
    ".r..r......r..r."
    ".rrrr......rrrr."
    "..rr...ww...rr.."
    "...r...kk...r..."
    "....rrrrrrrr...."
    "...rroooooorr..."
    "..rroooooooorr.."
    "..roooooooooor.."
    "..rrooooooaorr.."
    "...rrrrrrrrrr..."
    "..r.r.r..r.r.r.."
    ".r..r..r..r..r.."
    "................"
    "................";
static const char SP_TINFIN_PX[] =
    "................"
    "................"
    ".......s........"
    "......sss......."
    "...sssllllss...."
    "..sllllllllls..."
    ".slllgllllkllsy."
    "sslglllgllllll.."
    ".slllllllllllsy."
    "..sgggggggggs..."
    "...ssssssss....."
    "......s........."
    ".....ss........."
    "................"
    "................"
    "................";
static const char SP_WHORL_PX[] =
    "................"
    "................"
    "................"
    ".....hhhhh......"
    "...hhtttttth...."
    "..httbbbbbtth..."
    "..htbbtttbbth..."
    "..htbtbbbtbth..."
    "..htbtbtbtbth..."
    "..httbbbtttth..."
    "...hhhtttthh.kk."
    ".cccchhhhhcccck."
    "cccccccccccccc.."
    ".cccccccccccc..."
    "................"
    "................";
static const char SP_SHELL_IN_PX[] =
    "................"
    "................"
    "................"
    ".....hhhhh......"
    "...hhtttttth...."
    "..httbbbbbtth..."
    "..htbbtttbbth..."
    "..htbtbbbtbth..."
    "..htbtbtbtbth..."
    "..httbbbtttth..."
    "...hhhtttthh...."
    "....hhhhhhh....."
    "................"
    "................"
    "................"
    "................";
static const char SP_JELLY_PX[] =
    "................"
    ".....yyyyyy....."
    "...yyccccccyy..."
    "..yccwccccccyy.."
    ".yccwwccccccccy."
    ".yccccckcckccy.."
    ".yyyyyyyyyyyyyy."
    "..y.y.y..y.y.y.."
    "..y..y.y.y..y..."
    "...y.y..y..y.y.."
    "..y..y..y..y...."
    "...y..y..y..y..."
    "..y..y....y....."
    "................"
    "................"
    "................";
static const char SP_GROPER_PX[] =
    "................"
    ".....fffff......"
    "....fjjjjjf....."
    "...fjjwkjjjf...."
    ".f.fjjjjjjjf.f.."
    ".ff.fjjjjjf.ff.."
    "..ffjjzzzjjff..."
    "....fjzzzjf....."
    "...ffjjjjjff...."
    "..ff.fjjjf.ff..."
    ".....fj.jf......"
    "....ff...ff....."
    "...fff...fff...."
    "................"
    "................"
    "................";
static const char SP_BURRNUT_PX[] =
    "................"
    ".......b........"
    "...b...b...b...."
    "....b.ebe.b....."
    ".....eeeee......"
    "..b.eehheee.b..."
    "...eehhhhhee...."
    "bbeehhkhkhheebb."
    "...eehhhhhhe...."
    "..b.eehhhee.b..."
    ".....eeeee......"
    "....b.ebe.b....."
    "...b...b...b...."
    ".......b........"
    "................"
    "................";
static const char SP_STILTER_PX[] =
    "................"
    "................"
    "......ddd......."
    ".....dvvvd......"
    "....dvkvkvd....."
    "....dvvvvvd....."
    "..d..dvvvd..d..."
    ".d.d..ddd..d.d.."
    "d...d.d.d.d...d."
    "d....d..d.d...d."
    ".d...d.d..d..d.."
    ".d..d..d...d.d.."
    "..d.d..d...d...."
    "...d...d....d..."
    "................"
    "................";
static const char SP_LOUSE_PX[] =
    "................"
    "................"
    "................"
    "................"
    "....sssssss....."
    "...sgggggggs...."
    "..sgsgsgsgsgs..."
    ".sggsggsggsggs.."
    ".sgsgsgsgsgsgks."
    "sggsggsggsggggs."
    ".sssssssssssss.."
    "..s.s.s.s.s.s..."
    ".s.s.s.s.s.s...."
    "................"
    "................"
    "................";
static const char SP_HAUNT_PX[] =
    "................"
    "................"
    ".....IIIII......"
    "...IIwwwwwII...."
    "..IwwwwwwwwwI..."
    "..IwwkwwwkwwI..."
    ".IwwwkwwwkwwwI.."
    ".IwwwwwwwwwwwIC."
    ".IwwwwPPPwwwwI.."
    "..IwwwwwwwwwwI.."
    "..IIwwIwwIwwI..."
    "...I.I..IIwI...."
    ".......C..I....."
    ".........C......"
    "................"
    "................";
static const char SP_SQUID_PX[] =
    "................"
    ".......VV......."
    "......VppV......"
    ".....VppppV....."
    "....VppppppV...."
    "....VpkppkpV...."
    "....VppppppV...."
    "....VppppppV...."
    ".....VVVVVV....."
    "....V.V.V.V....."
    "...V..V..V.V...."
    "...V.V..V..V...."
    "..V..V..V...V..."
    "..V.V...V...V..."
    "................"
    "................";
static const char SP_GRINFISH_PX[] =
    "................"
    "................"
    ".......NN......."
    ".....NNBBNN....."
    "...NNBBBBBBNN..."
    ".NNBBBBBBBkBBN.."
    "NBBBBBBBBBBBBBN."
    ".NBBBBBwkwkwkwN."
    "NBBBBBBwwwwwwN.."
    ".NBBBBBBBBBBN..."
    "...NNBBBBBNN...."
    ".....NNNNN......"
    "................"
    "................"
    "................"
    "................";
static const char SP_WORM_PX[] =
    "................"
    "....mmmmmm......"
    "...mrrrrrrm....."
    "..mrrkrrkrrm...."
    "..mrrrrrrrrrm..."
    "..mrwwwwwwwrm..."
    "..mrkwkwkwkrm..."
    "..mrrrrrrrrm...."
    "...mrrrrrrm....."
    "....mrrrrm......"
    "....mrrrrm......"
    "....mvvvvm......"
    "....mvvvvm......"
    "....mrrrrm......"
    "....mvvvvm......"
    "....mrrrrm......";
static const char SP_WARDEN_PX[] =
    "................................"
    ".........r..r....r..r..........."
    "........rr.rr....rr.rr.........."
    "........rrrrrrrrrrrrrr.........."
    ".......sssssssssssssssss........"
    "......sglllllllllllllllgs......."
    ".....sglllllllllllllllllgs......"
    ".....sgllwwwwllllllllllllgs....."
    ".....sglwwwwwwllllllllllgs......"
    "....sglwwwkkkwwlllllllllgs......"
    "....sglwwkkkkkwwllllllllgs......"
    "....sglwwwkkkwwllllllllllgs....."
    "....sgllwwwwwwllllllllllgs......"
    "....sglllwwwwlllllllllllgs......"
    "....sglllllllllllllllllgs......."
    "....sglllggggggggggglllgs......."
    "....sgllgkkkkkkkkkkkglllgs......"
    "....sgllgkwkwkwkwkwkglllgs......"
    "....sglllggggggggggglllgs......."
    ".....sglllllllllllllllgs........"
    "....qssgggllllllllgggssq........"
    "...qjqssssgggggggssssqjq........"
    "..qjjjq..ssssssssss.qjjjq......."
    "..qjjq....sglllgs....qjjq......."
    "...qq....sglllllgs....qq........"
    "........sglllllllgs............."
    ".......sglllllllllgs............"
    "......sglllllllllllgs..........."
    ".....sssssssssssssssss.........."
    "....ddddddddddddddddddd........."
    "................................"
    "................................";
static const char SP_EYE_PX[] =
    "................................"
    "................................"
    ".........mmmmmmmmmmmm..........."
    ".......mmvvvvvvvvvvvvmm........."
    "......mvvrrrrrrrrrrrrvvm........"
    ".....mvrrwwwwwwwwwwwwrrvm......."
    "....mvrwwwwwwwwwwwwwwwwrvm......"
    "....mvrwwwwwwwwwwwwwwwwwrvm....."
    "...mvrwwwwwwwiiiiiiwwwwwrvm....."
    "...mvrwwwwwwiiiiiiiiwwwwwrvm...."
    "...mvrwwwwwiiiikkkiiiwwwwrvm...."
    "..mvrwwwwwwiiikkkkkiiiwwwwrvm..."
    "..mvrwwwwwwiikkkkkkkiiwwwwrvm..."
    "..mvrwwwwwwiikkkwkkkiiwwwwrvm..."
    "..mvrwwwwwwiikkkkkkkiiwwwwrvm..."
    "..mvrwwwwwwiiikkkkkiiiwwwwrvm..."
    "...mvrwwwwwiiiikkkiiiwwwwrvm...."
    "...mvrwwwwwwiiiiiiiiwwwwwrvm...."
    "...mvrwwwwwwwiiiiiiwwwwwrvm....."
    "....mvrwwwwwwwwwwwwwwwwwrvm....."
    "....mvrrwwwwwwwwwwwwwwwrrvm....."
    ".....mvvrrwwwwwwwwwwwrrvvm......"
    "......mmvvrrrrrrrrrrrvvmm......."
    "........mmvvvvvvvvvvvmm........."
    "..........mmmmmmmmmmm..........."
    "............m.m.m.m............."
    "...........m..m..m..m..........."
    "................................"
    "................................"
    "................................"
    "................................"
    "................................";
static const char SP_ARM_PX[] =
    ".................mmmm..........."
    "................mrrrrm.........."
    "...............mrrwwrm.........."
    "...............mrwkkwm.........."
    "..............mrrwkkwm.........."
    "..............mrrrwwrm.........."
    ".............mrrrrrrm..........."
    ".............mrrKrrrm..........."
    "............mrrrrKrrm..........."
    "............mrrrrrrm............"
    "...........mrrKrrrrm............"
    "...........mrrrrKrrm............"
    "..........mrrrrrrrm............."
    "..........mrrKrrrrm............."
    ".........mrrrrrKrrm............."
    ".........mrrrrrrrm.............."
    "........mrrrKrrrrm.............."
    "........mrrrrrKrrm.............."
    ".......mrrrrrrrrm..............."
    ".......mrrrKrrrrm..............."
    "......mrrrrrrKrrm..............."
    "......mrrrrrrrrm................"
    ".....mrrrKrrrrrm................"
    ".....mrrrrrrKrrm................"
    "....mrrrrrrrrrm................."
    "....mrrrKrrrrrm................."
    "...mrrrrrrrKrrm................."
    "...mrrrrrrrrrm.................."
    "..mvvvvvvvvvvm.................."
    "..mmmmmmmmmmmm.................."
    "................................"
    "................................";
static const char SP_CHEST_PX[] =
    "............"
    "..bbbbbbbb.."
    ".btttttttb.."
    ".bttttttttb."
    "bbbbbbbbbbbb"
    "baaaaykaaaab"
    "bttttyytttb."
    "bttttttttttb"
    "bbbbbbbbbbbb"
    "............";
static const char SP_CHEST_OPEN_PX[] =
    ".bbbbbbbbbb."
    "bttttttttttb"
    "bbbbbbbbbbbb"
    "............"
    "bbbbbbbbbbbb"
    "bkkkkkkkkkkb"
    "bttttyytttb."
    "bttttttttttb"
    "bbbbbbbbbbbb"
    "............";
static const char SP_LEVER_PX[] =
    "......gg"
    ".....gwg"
    "....g.g."
    "...g...."
    "..g....."
    ".g......"
    ".g......"
    "sssssss."
    "sgggggs."
    "sgsgsgs."
    "sssssss."
    "........";
static const char SP_LEVER_ON_PX[] =
    "gg......"
    "gwg....."
    ".g.g...."
    "....g..."
    ".....g.."
    "......g."
    "......g."
    "sssssss."
    "sgggggs."
    "sgsgsgs."
    "sssssss."
    "........";
static const char SP_HEAD0_PX[] =
    "............"
    "..P......P.."
    ".PKP....PKP."
    ".PKKPPPPKKP."
    ".PKKKKKKKKP."
    "PKKkKKKKkKKP"
    "PKKKKKKKKKKP"
    "PKKKKccKKKKP"
    ".PKKKKKKKKP."
    "..PPKKKKPP.."
    "....PPPP...."
    "............";
static const char SP_HEAD1_PX[] =
    "............"
    "....yyyy...."
    "...yywwyy..."
    "..yyywwyyy.."
    "..yyyyyyyyy."
    ".yyyyyyyyyy."
    ".ayyyyyyyya."
    "aaaaaaaaaaaa"
    ".aaaaaaaaaa."
    "............"
    "............"
    "............";
static const char SP_HEAD2_PX[] =
    "............"
    ".g.g....g.g."
    "..gsg..gsg.."
    ".gsllllllsg."
    ".slllllllls."
    "slllkllklll."
    "sllllllllll."
    "slllgggglls."
    ".sllllllls.."
    "..ssssssss.."
    "............"
    "............";
static const char SP_FACE0_PX[] =
    ".f.f....f.f."
    "..fjf..fjf.."
    "..fjjjjjjf.."
    ".fjjjjjjjjf."
    "fjjwkjjwkjjf"
    "fjjkkjjkkjjf"
    "fjjjjjjjjjjf"
    ".fjjjzzjjjf."
    ".fjjzzzzjjf."
    "..ffjjjjff.."
    "....ffff...."
    "............";
static const char SP_FACE1_PX[] =
    ".a.a....a.a."
    "..aya..aya.."
    "..ayyyyyya.."
    ".ayyyyyyyya."
    "ayywkyywkyya"
    "ayykkyykkyya"
    "ayyyyyyyyyya"
    ".ayyyccyyya."
    ".ayyccccyya."
    "..aayyyyaa.."
    "....aaaa...."
    "............";
static const char SP_FACE2_PX[] =
    ".P.P....P.P."
    "..PKP..PKP.."
    "..PKKKKKKP.."
    ".PKKKKKKKKP."
    "PKKwkKKwkKKP"
    "PKKkkKKkkKKP"
    "PKKKKKKKKKKP"
    ".PKKKccKKKP."
    ".PKKccccKKP."
    "..PPKKKKPP.."
    "....PPPP...."
    "............";
static const char SP_ICON_SHOP_PX[] =
    "........................"
    "...........bb..........."
    ".........bbttbb........."
    ".......bbttttttbb......."
    ".....bbttttttttttbb....."
    "...bbttttttttttttttbb..."
    "..bbbbbbbbbbbbbbbbbbbb.."
    "....eeeeeeeeeeeeeeee...."
    "....eaaaeeeeeeeeaaae...."
    "....eayaeebbbbeeayae...."
    "....eaaaeebttbeeaaae...."
    "....eeeeeebttbeeeeee...."
    "....eeeeeebttbeeeeee...."
    "....eeeeeebtkbeeeeee...."
    "....eeeeeebttbeeeeee...."
    "....eeeeeebttbeeeeee...."
    "..bbbbbbbbbbbbbbbbbbbb.."
    "..btbtbtbtbtbtbtbtbtbb.."
    "..bbbbbbbbbbbbbbbbbbbb.."
    "...b..b...........b..b.."
    "...b..b...........b..b.."
    "........................"
    "........................"
    "........................";
static const char SP_ICON_KIT_PX[] =
    "........................"
    "...........ll..........."
    "..........lwwl.........."
    "..........lwwl.........."
    "..........llll.........."
    "...........ll..........."
    ".......lllllllllll......"
    "...........ll..........."
    "...........ll..........."
    "...........ll..........."
    "...........ll..........."
    "...........ll..........."
    "...........ll..........."
    "...........ll..........."
    "..ll.......ll.......ll.."
    "..lll......ll......lll.."
    "...lll.....ll.....lll..."
    "....llll...ll...llll...."
    ".....lllllllllllll......"
    ".......lllllllll........"
    ".........lllll.........."
    "........................"
    "........................"
    "........................";
static const char SP_ICON_DIVE_PX[] =
    "........................"
    "........................"
    "..ss..............ss...."
    "..sgssssssssssssssgs...."
    "..ss..............ss...."
    "..sgssssssssssssssgs...."
    "..ss..............ss...."
    "..sgssssssssssssssgs...."
    "..ss.........P.P..ss...."
    "..sgsssssss..PKP..gs...."
    "..ss........KKKKKKss...."
    "..sgsssss.KKKKKkKKgs...."
    "..ss....KKPKKKKKKKws...."
    "uuuuuuuuuuuuuuuuuuuuuuuu"
    "uIIuuuuuuIIIuuuuuuuIIuuu"
    "BBBBBBBBBBBBBBBBBBBBBBBB"
    "BBBuuBBBBBBBBBBuuBBBBBBB"
    "BBBBBBBBBBBBBBBBBBBBBBBB"
    "NNNNNNNNNNNNNNNNNNNNNNNN"
    "NNNNNNNNNNNNNNNNNNNNNNNN"
    "NNNNNNNNNNNNNNNNNNNNNNNN"
    "NNNNNNNNNNNNNNNNNNNNNNNN"
    "........................"
    "........................";
static const char IC_POLE_PX[] =
    ".......l"
    "......ll"
    ".....ll."
    "....ll.."
    "...bb..."
    "..bb...."
    ".bb....."
    "bb......";
static const char IC_HAMMER_PX[] =
    "....lll."
    "...lllll"
    "...lllll"
    "....lbl."
    "...bb..."
    "..bb...."
    ".bb....."
    "bb......";
static const char IC_SHIELD_PX[] =
    ".llllll."
    "llwlllll"
    "llllllll"
    "llllllll"
    ".llllll."
    ".llllll."
    "..llll.."
    "...ll...";
static const char IC_POTION_PX[] =
    "...cc..."
    "...bb..."
    "..wllw.."
    ".llllll."
    ".lrrrrl."
    ".lrrrrl."
    ".lrrrrl."
    "..llll..";
static const char IC_EGG_PX[] =
    "...cc..."
    "..cwcc.."
    ".cwcccc."
    ".cccccc."
    ".ccccct."
    ".cccctt."
    "..cttt.."
    "........";
static const char IC_BOMB_PX[] =
    ".....ya."
    "....b..."
    "..ssbs.."
    ".sggggs."
    ".sgwggs."
    ".sggggs."
    ".sggggs."
    "..ssss..";
static const char IC_MIST_PX[] =
    "........"
    "..VVVV.."
    ".VppppV."
    "VpkppkpV"
    "VppppppV"
    ".VppppV."
    "..V..V.."
    ".V..V...";
static const char IC_BLOOD_PX[] =
    "...ww..."
    "...ll..."
    "..wllw.."
    ".lrrrrl."
    ".rmrrrr."
    ".rrrrmr."
    ".lrrrrl."
    "..llll..";
static const char IC_FINS_PX[] =
    "........"
    ".B....B."
    "BuB..BuB"
    "BuuB.BuB"
    "BuuuBuuB"
    ".BuuuuB."
    "..BBBB.."
    "........";
static const char IC_FOAM_PX[] =
    "..CC...."
    ".CwIC.C."
    ".CIIC.wC"
    "..CC..CC"
    "....CC.."
    "...CwIC."
    "...CIIC."
    "....CC..";
static const char IC_GUM_PX[] =
    "........"
    ".P....P."
    "PKP..PKP"
    ".PKKKKP."
    ".PKwKKP."
    ".PKKKKP."
    "PKP..PKP"
    ".P....P.";
static const char IC_PEBBLE_PX[] =
    "........"
    "..sss..."
    ".sgggs.."
    "sggwggs."
    "sgggggs."
    "sgggsgs."
    ".sssss.."
    "........";
static const char IC_COG_PX[] =
    "...ss..."
    ".s.gg.s."
    "..gggg.."
    "sggkkggs"
    "sggkkggs"
    "..gggg.."
    ".s.gg.s."
    "...ss...";
static const char IC_SPIRAL_PX[] =
    "...oo..."
    "..oaao.."
    ".oaoaao."
    ".oaooao."
    ".oaaoao."
    "..oaaao."
    "...ooo.."
    "........";
static const char IC_BONE_PX[] =
    "cc....cc"
    "cwc..cwc"
    "..c..c.."
    "...cc..."
    "...cc..."
    "..c..c.."
    "cwc..cwc"
    "cc....cc";
static const char IC_SUN_PX[] =
    "...yy..."
    ".y.aa.y."
    "..aaaa.."
    "yaaykaay"
    "yaakyaay"
    "..aaaa.."
    ".y.aa.y."
    "...yy...";
static const char IC_MOON_PX[] =
    "..IIII.."
    ".IwwIII."
    "IwwII..."
    "IwII...."
    "IwII...."
    "IwwII..."
    ".IwwIII."
    "..IIII..";
static const SprDef DEFS[] = {
    {SP_DIVER, 16, 12, SP_DIVER_PX},
    {SP_DIVER2, 16, 12, SP_DIVER2_PX},
    {SP_FIZZLE, 16, 16, SP_FIZZLE_PX},
    {SP_PRICKLE, 16, 16, SP_PRICKLE_PX},
    {SP_FROND, 16, 16, SP_FROND_PX},
    {SP_NIPPER, 16, 16, SP_NIPPER_PX},
    {SP_GLOB, 16, 16, SP_GLOB_PX},
    {SP_SMOG, 16, 16, SP_SMOG_PX},
    {SP_CLAMPER, 16, 16, SP_CLAMPER_PX},
    {SP_TINFIN, 16, 16, SP_TINFIN_PX},
    {SP_WHORL, 16, 16, SP_WHORL_PX},
    {SP_SHELL_IN, 16, 16, SP_SHELL_IN_PX},
    {SP_JELLY, 16, 16, SP_JELLY_PX},
    {SP_GROPER, 16, 16, SP_GROPER_PX},
    {SP_BURRNUT, 16, 16, SP_BURRNUT_PX},
    {SP_STILTER, 16, 16, SP_STILTER_PX},
    {SP_LOUSE, 16, 16, SP_LOUSE_PX},
    {SP_HAUNT, 16, 16, SP_HAUNT_PX},
    {SP_SQUID, 16, 16, SP_SQUID_PX},
    {SP_GRINFISH, 16, 16, SP_GRINFISH_PX},
    {SP_WORM, 16, 16, SP_WORM_PX},
    {SP_WARDEN, 32, 32, SP_WARDEN_PX},
    {SP_EYE, 32, 32, SP_EYE_PX},
    {SP_ARM, 32, 32, SP_ARM_PX},
    {SP_CHEST, 12, 10, SP_CHEST_PX},
    {SP_CHEST_OPEN, 12, 10, SP_CHEST_OPEN_PX},
    {SP_LEVER, 8, 12, SP_LEVER_PX},
    {SP_LEVER_ON, 8, 12, SP_LEVER_ON_PX},
    {SP_HEAD0, 12, 12, SP_HEAD0_PX},
    {SP_HEAD1, 12, 12, SP_HEAD1_PX},
    {SP_HEAD2, 12, 12, SP_HEAD2_PX},
    {SP_FACE0, 12, 12, SP_FACE0_PX},
    {SP_FACE1, 12, 12, SP_FACE1_PX},
    {SP_FACE2, 12, 12, SP_FACE2_PX},
    {SP_ICON_SHOP, 24, 24, SP_ICON_SHOP_PX},
    {SP_ICON_KIT, 24, 24, SP_ICON_KIT_PX},
    {SP_ICON_DIVE, 24, 24, SP_ICON_DIVE_PX},
};
static const SprDef ICONS[] = {
    {IC_POLE, 8, 8, IC_POLE_PX},
    {IC_HAMMER, 8, 8, IC_HAMMER_PX},
    {IC_SHIELD, 8, 8, IC_SHIELD_PX},
    {IC_POTION, 8, 8, IC_POTION_PX},
    {IC_EGG, 8, 8, IC_EGG_PX},
    {IC_BOMB, 8, 8, IC_BOMB_PX},
    {IC_MIST, 8, 8, IC_MIST_PX},
    {IC_BLOOD, 8, 8, IC_BLOOD_PX},
    {IC_FINS, 8, 8, IC_FINS_PX},
    {IC_FOAM, 8, 8, IC_FOAM_PX},
    {IC_GUM, 8, 8, IC_GUM_PX},
    {IC_PEBBLE, 8, 8, IC_PEBBLE_PX},
    {IC_COG, 8, 8, IC_COG_PX},
    {IC_SPIRAL, 8, 8, IC_SPIRAL_PX},
    {IC_BONE, 8, 8, IC_BONE_PX},
    {IC_SUN, 8, 8, IC_SUN_PX},
    {IC_MOON, 8, 8, IC_MOON_PX},
};

static Sprite spr[SP_COUNT];
static Sprite icon[IC_COUNT];
static int bad;

void sdg_art_load(void) {
    if (spr[SP_DIVER].px) return;
    bad = 0;
    for (int i = 0; i < ARRAY_LEN(DEFS); i++) {
        if ((int)strlen(DEFS[i].px) != DEFS[i].w * DEFS[i].h) { bad++; continue; }
        spr_make(&spr[DEFS[i].id], DEFS[i].w, DEFS[i].h, DEFS[i].px);
    }
    for (int i = 0; i < ARRAY_LEN(ICONS); i++) {
        if ((int)strlen(ICONS[i].px) != ICONS[i].w * ICONS[i].h) { bad++; continue; }
        spr_make(&icon[ICONS[i].id], ICONS[i].w, ICONS[i].h, ICONS[i].px);
    }
    for (int i = 0; i < SP_COUNT; i++)
        if (!spr[i].px) bad++;
    for (int i = 0; i < IC_COUNT; i++)
        if (!icon[i].px) bad++;
}

int sdg_art_bad(void) { return bad; }
const Sprite *sdg_sprite(int id) { return &spr[iclamp(id, 0, SP_COUNT - 1)]; }

void sdg_draw_item_icon(int it, int x, int y) {
    if (it <= IT_NONE || it >= IT_COUNT) return;
    const SdgItem *I = &SDG_ITEM[it];
    int ic = IC_POLE;
    switch (I->kind) {
    case K_POLE: ic = IC_POLE; break;
    case K_HAMMER: ic = IC_HAMMER; break;
    case K_SHIELD: ic = IC_SHIELD; break;
    case K_POTION: ic = it == IT_EVIL ? IC_BLOOD : IC_POTION; break;
    case K_EGG: ic = IC_EGG; break;
    case K_BOMB: ic = IC_BOMB; break;
    case K_MIST: ic = IC_MIST; break;
    default: ic = it == IT_FLIPPERS ? IC_FINS : IC_BLOOD; break;
    }
    uint8_t map[256];
    pal_identity(map);
    if (I->el != EL_NONE) {
        pal_swap(map, C_LIGHT, SDG_EL_COL[I->el]);
        pal_swap(map, C_WHITE, C_WHITE);
    }
    if (it == IT_HOLY) pal_swap(map, C_RED, C_YELLOW);
    if (it == IT_MEDIUM) pal_swap(map, C_RED, C_CYAN);
    if (it == IT_LARGE) pal_swap(map, C_RED, C_LIME);
    if (it == IT_LEECH) pal_swap(map, C_BROWN, C_WINE);
    if (it == IT_LAMP) pal_swap(map, C_BROWN, C_WHITE);
    spr_draw_ex(&icon[ic], x, y, 0, map, -1);
}

void sdg_draw_relic_icon(int r, int x, int y) {
    if (r < 0 || r >= RL_COUNT) return;
    spr_draw(&icon[IC_FOAM + r], x, y, 0);
}

void sdg_draw_head_icon(int h, int x, int y) {
    if (h < 0 || h >= SDG_HEADS) return;
    spr_draw(&spr[SP_HEAD0 + h], x, y, 0);
}
