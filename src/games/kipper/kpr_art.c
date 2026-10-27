/* KIPPER - pixel art (palette-letter strings, see gfx.h), all drawn for
 * UFO 40: Kipper the yellow sub with a porthole face, the pick-ups, Auntie
 * Roe, the egg companions, every creature and the equipment icons. The
 * docks, the big creatures, the rock and the dark are drawn in kpr_draw.c.
 * Everything faces right; the draw code flips it. */
#include "kpr.h"

Sprite kpr_spr[SPR_COUNT];

static const char SUB0[] =
    "......kk........"
    "......ka........"
    "....kkkakkk....."
    "..kkyyyyyyykk..."
    ".kyyyyyyyykkkk.."
    "kayyyyyyyykwwIk."
    ".ayyyyyyyykwkIk."
    "kayyyyyyyykwwIk."
    ".kyyyyyyyykkkk.."
    "..kkyyyyyyyykk.."
    "....kkkkkkkk...."
    "................";
static const char SUB1[] =
    "......kk........"
    "......ka........"
    "....kkkakkk....."
    "..kkyyyyyyykk..."
    ".kyyyyyyyykkkk.."
    ".ayyyyyyyykwwIk."
    "kayyyyyyyykwkIk."
    ".ayyyyyyyykwwIk."
    ".kyyyyyyyykkkk.."
    "..kkyyyyyyyykk.."
    "....kkkkkkkk...."
    "................";
static const char TANK[] =
    "...kkkk..."
    "...kggk..."
    "..kkkkkk.."
    ".krrrrrrk."
    ".krwrrrrk."
    ".krwrrrrk."
    ".krrrrrrk."
    ".krrrrrrk."
    ".kmmmmmmk."
    "..kkkkkk..";
static const char BOOST[] =
    ".........."
    "k........."
    "kk.kkkkkk."
    "kokllllllk"
    "kollllllwk"
    "kokggggggk"
    "kk.kkkkkk."
    "k........."
    ".........."
    "..........";
static const char EGG[] =
    "...kkkk..."
    "..kKKKKk.."
    ".kKKwKKKk."
    ".kKwKKPKk."
    "kKKKKKKKKk"
    "kKPKKKKPKk"
    "kKKKKKKKKk"
    ".kKKKPKKk."
    "..kKKKKk.."
    "...kkkk...";
static const char CHARGE[] =
    "....kk...."
    "...kak...."
    "..kkkkk..."
    ".knnnnnk.."
    "knnwnnnnk."
    "kaaaaaaak."
    "knnnnnnnk."
    ".knnnnnk.."
    "..kkkkk..."
    "..........";
static const char LIGHT[] =
    "...yy..."
    "..yccy.."
    ".yccwcy."
    "ycwwwwcy"
    "ycwwwwcy"
    ".ycwccy."
    "..yccy.."
    "...yy...";
static const char PLATE[] =
    "kkkkkkk."
    "kllllllk"
    "klwllllk"
    "kllllgk."
    "kllllgk."
    "klgggk.."
    "kkkkk..."
    "........";
static const char ROE[] =
    "....kkkkkk......"
    "...kjjjjjjk....."
    "..kjjwkjjjjk...."
    "..kjjkkjjjjk...."
    ".kjjjjjjjjjk...."
    ".kjjKKKjjjk....."
    "..kjjjjjjjk....."
    "...kkpppkk......"
    "..kpppppppk....."
    ".kpVpppVpppk...."
    ".kppppppppk....."
    "..kpppppppk....."
    "...kqqqqqk......"
    "...kqqqqqk......"
    "....kqqqk......."
    "....kqqqk......."
    ".....kqqk......."
    "....kqkqqk......"
    "...kqk.kqqk....."
    "...kk...kkk.....";
static const char FISH_BLUE[] =
    "...kkk.."
    ".kkBBBk."
    "kBBwkBBk"
    "kBBBBBBk"
    ".kkBBBk."
    "...kkk..";
static const char FISH_RED[] =
    "...kkk.."
    ".kkrrrk."
    "krrwkrrk"
    "krrrrrrk"
    ".kkrrrk."
    "...kkk..";
static const char POLYP[] =
    "..K..K..K..K.."
    ".KPK.KP.KPK.K."
    "..KPKPKPKPKP.."
    "..PKPKPKPKPK.."
    "...PPPPPPPP..."
    "...kPPPPPPk..."
    "....kPPPPk...."
    "....kPPPPk...."
    "....kPPPPk...."
    "...kmPPPPmk..."
    "..kmmmmmmmmk.."
    "..kkkkkkkkkk..";
static const char FILCHER[] =
    "..k.kkkk.."
    ".kokowook."
    "kookowowkk"
    "kookowowwk"
    "kookowowok"
    ".kokowook."
    "..k.kkkk.."
    "..........";
static const char GLIMMER[] =
    "...kkkkkkkk..."
    "..kVVVVVVVVkk."
    ".kVpVVpVVpVVVk"
    "kVVVVVVVVVVwkk"
    "kVVVVVVVVVVkwk"
    "kVVVVVVVVVVVVk"
    ".kVpVVpVVpVVk."
    "..kkkkkkkkkVkV"
    "...........k.V"
    "..............";
static const char GREENLING[] =
    "..k.kkkkk..."
    ".kzkzzzzzkk."
    "kzzkzzzzzwkk"
    "kzzzzzzzzkwk"
    "kzzkzzzzzzzk"
    ".kzkzzzzzzk."
    "..k.kkkkkk.."
    "............";
static const char SHELLBACK[] =
    "....kkkkk....."
    "..kkttttbk...."
    ".kttbbbbtbk..."
    ".ktbttttbtk..."
    "kttbtbbtbtk..."
    "kttbtbttbtkkk."
    "kttbbttbttkrrk"
    ".kttttbttkrwrk"
    ".kkkkkkkkrrrk."
    "..krkrkrkrkk.."
    "..rk.rk.rk...."
    "..............";
static const char SHELL[] =
    "....kkkkk....."
    "..kkttttbk...."
    ".kttbbbbtbk..."
    ".ktbttttbtk..."
    "kttbtbbtbtk..."
    "kttbtbttbtk..."
    "kttbbttbttk..."
    ".kttttbttk...."
    ".kkkkkkkkk...."
    ".............."
    ".............."
    "..............";
static const char KITERAY[] =
    "........kk.........."
    ".......kBBk........."
    "kk....kBBBBk....kk.."
    "kBkk.kBBBBBBk.kkBk.."
    "kBBBkBBBBBwBBkBBBk.."
    ".kBBBBBBBBBBBBBBk..."
    "..kkBBBBBBBBBBkk...."
    "....kkkBBBBkkk......"
    ".......kkkkk........"
    "....................";
static const char JELLY[] =
    "...kkkkkk..."
    "..kuuuuuuk.."
    ".kuwuuuuuuk."
    ".kuuuuuuuuk."
    "kuuuuuuuuuuk"
    "kkkkkkkkkkkk"
    ".u.u.u.u.u.."
    ".u..u.u..u.."
    "..u.u..u.u.."
    ".u..u.u..u.."
    "..u..u...u.."
    "............";
static const char LANTERN[] =
    "..........kk......"
    "........kk.ky....."
    ".......k....yyk..."
    "....kkkkkkk..yk..."
    "..kkdddddddkk....."
    ".kdddddddddwkk...."
    "kddddddddddkwk...."
    "kddddddddddddkk..."
    "kddddkddddwkwkwk.."
    ".kddddddddkwkwkk.."
    "..kkdddddddddkk..."
    "....kkkkkkkkk....."
    ".................."
    "..................";
static const char RISER[] =
    "....kk...."
    "...kIIk..."
    "..kIIIIk.."
    ".kIIIIIIk."
    ".kIwIIwIk."
    ".kIkIIkIk."
    ".kIIIIIIk."
    ".kIIIIIIk."
    "..kIIIIk.."
    "..kIkkIk.."
    ".kIk..kIk."
    ".kI.kk.Ik."
    "kI.kIIk.Ik"
    "k..kIIk..k"
    "...kIIk..."
    "....kk....";
static const char PIPEHORSE[] =
    "...kkkk..."
    "..kaaaak.."
    ".kaawkaakk"
    "..kaaaakkk"
    "...kaak..."
    "..kaaak..."
    "..kaaaak.."
    ".kaaoaak.."
    ".kaaoaak.."
    "..kaaaak.."
    "...kaaak.."
    "....kaak.."
    "..kkaak..."
    "..kaak....";
static const char BLOATER[] =
    "............"
    "....kkkk...."
    "..kkttttkk.."
    ".kttbtttwkk."
    "kkttttttkwk."
    "kttbtttttttk"
    "kttttbtttttk"
    "kkttttttttk."
    ".kkttbtttk.."
    "..kkkkkkk..."
    "............"
    "............";
static const char STILT[] =
    "................"
    "......kkkk......"
    "....kkrrrrkk...."
    "...krrwrrwrrk..."
    "...krrkrrkrrk..."
    "....kkrrrrkk...."
    "..kk.k.kk.k.kk.."
    ".k..k..kk..k..k."
    "k..k...kk...k..k"
    "k..k..k..k..k..k"
    "k.k...k..k...k.k"
    "................";
static const char SUNSTAR[] =
    ".....kk....."
    "....kook...."
    "....kook...."
    "kkkkkookkkkk"
    "kooooooooook"
    ".kooowwoook."
    "..kooooook.."
    "..kookkook.."
    ".kook..kook."
    ".kok....kok."
    "kok......kok"
    "kk........kk";
static const char BILLFISH[] =
    "......kk.............."
    "..k..kssk............."
    ".kskkssssskkk........."
    "kssssssssswkkkkkkkkkk."
    "ksssssssssskllllllllll"
    "kssssssssssskkkkkkkkk."
    ".kskkssssskkk........."
    "..k..kk...............";
static const char BURR[] =
    "............"
    "....k..k...."
    "..k.kppk.k.."
    "...kppppk..."
    ".kkppppppkk."
    "..kpwppppk.."
    ".kkppppppkk."
    "...kppppk..."
    "..k.kppk.k.."
    "....k..k...."
    "............"
    "............";
static const char BURR_ANGRY[] =
    "k....kk....k"
    ".k...kk...k."
    "..k.kppk.k.."
    "...kppppk..."
    "kkkppppppkkk"
    "..kpwppppk.."
    "kkkppppppkkk"
    "...kppppk..."
    "..k.kppk.k.."
    ".k...kk...k."
    "k....kk....k"
    "............";
static const char WORM[] =
    "...kkkk..."
    "..kmmmmk.."
    ".kmwkkwmk."
    ".kmmmmmmk."
    ".kmkwwkmk."
    ".kmmmmmmk."
    "..kmmmmk.."
    "..kmmmmk.."
    "..kmvvmk.."
    "..kmmmmk.."
    "..kmvvmk.."
    "..kmmmmk..";
static const char PACIFIED[] =
    "..kkkk.."
    ".kuuuuk."
    "kuwuuwuk"
    "kuuuuuuk"
    ".kuuuuk."
    "..kkkk..";

/* the equipment, 12 x 12 */
static const char EQ_RAM[] =
    "............"
    "............"
    "k..........."
    "kkk.kkkkkk.."
    "kokkrrrrrrk."
    "korrrrrrrwrk"
    "korrrrrrrrrk"
    "kokkrrrrrrk."
    "kkk.kkkkkk.."
    "k..........."
    "............"
    "............";
static const char EQ_AUGER[] =
    "............"
    "kkkkk......."
    "kbbbkkk....."
    "kbllglgkk..."
    "kblglglglkk."
    "kbllglgllglk"
    "kblglglglkk."
    "kbllglgkk..."
    "kbbbkkk....."
    "kkkkk......."
    "............"
    "............";
static const char EQ_THRUST[] =
    "............"
    "....kkkk...."
    "...kggggk..."
    "...kgllgk..."
    "...kgllgk..."
    "..kkggggkk.."
    "..kssggssk.."
    "...kyyyyk..."
    "...koyyok..."
    "....koook..."
    "....kook...."
    ".....kk.....";
static const char EQ_BOMBS[] =
    "......kk...."
    ".....kak...."
    "....kkkkk..."
    "...knnnnnk.."
    "..knnwnnnnk."
    "..kaaaaaaak."
    "..knnnnnnnk."
    "..knnnnnnnk."
    "...knnnnnk.."
    "....kkkkk..."
    "............"
    "............";
static const char EQ_ROCKET[] =
    ".....kk....."
    "....kwwk...."
    "....kllk...."
    "...kllllk..."
    "...klrrlk..."
    "...kllllk..."
    "...klrrlk..."
    "..kkllllkk.."
    ".krkllllkrk."
    ".krkkkkkkrk."
    "..k.kook.k.."
    "....kook....";
static const char EQ_FINS[] =
    "....kkkk...."
    "..kkiiiikk.."
    ".kiik..kiik."
    ".kik.kk.kik."
    "kik..kk..kik"
    "kikkkiikkkik"
    "kikkkiikkkik"
    "kik..kk..kik"
    ".kik.kk.kik."
    ".kiik..kiik."
    "..kkiiiikk.."
    "....kkkk....";
static const char EQ_LAMP[] =
    "....kkkk...."
    "....k..k...."
    "...kkkkkk..."
    "..kyyyyyyk.."
    "..kycwwcyk.."
    "..kywwwwyk.."
    "..kycwwcyk.."
    "..kyyyyyyk.."
    "...kkkkkk..."
    "...kbbbbk..."
    "....kkkk...."
    "............";
static const char EQ_SONAR[] =
    "...kkkkkk..."
    "..kffffffk.."
    ".kffjjjjffk."
    "kffjffffjffk"
    "kfjffiiffjfk"
    "kfjfiwwifjfk"
    "kfjffiiffjfk"
    "kffjffffjffk"
    ".kffjjjjffk."
    "..kffffffk.."
    "...kkkkkk..."
    "............";
static const char EQ_LEAN[] =
    "............"
    "....kkkk...."
    "...kzzzzk..."
    "..kkkkkkkk.."
    "..kjjjjjjk.."
    "..kjwjjjjk.."
    "..kjjzzjjk.."
    "..kjzzzzjk.."
    "..kjjzzjjk.."
    "..kjjjjjjk.."
    "..kffffffk.."
    "..kkkkkkkk..";
static const char EQ_PLATES[] =
    "..kkkkkkkk.."
    ".kllllllllk."
    "kllwlllllllk"
    "klwllllllllk"
    "kllllllllllk"
    "klllllllllgk"
    "kllllllllggk"
    ".kllllllggk."
    "..kllllggk.."
    "...klllgk..."
    "....kkkk...."
    "............";

static const struct { const char *s; int w, h; } ART[SPR_COUNT] = {
    [SPR_SUB0] = {SUB0, 16, 12}, [SPR_SUB1] = {SUB1, 16, 12}, [SPR_TANK] = {TANK, 10, 10},
    [SPR_BOOST] = {BOOST, 10, 10}, [SPR_EGG] = {EGG, 10, 10}, [SPR_CHARGE] = {CHARGE, 10, 10},
    [SPR_LIGHT] = {LIGHT, 8, 8}, [SPR_PLATE] = {PLATE, 8, 8}, [SPR_ROE] = {ROE, 16, 20},
    [SPR_FISH_BLUE] = {FISH_BLUE, 8, 6}, [SPR_FISH_RED] = {FISH_RED, 8, 6},
    [SPR_POLYP] = {POLYP, 14, 12}, [SPR_FILCHER] = {FILCHER, 10, 8}, [SPR_GLIMMER] = {GLIMMER, 14, 10},
    [SPR_GREENLING] = {GREENLING, 12, 8}, [SPR_SHELLBACK] = {SHELLBACK, 14, 12}, [SPR_SHELL] = {SHELL, 14, 12},
    [SPR_KITERAY] = {KITERAY, 20, 10}, [SPR_JELLY] = {JELLY, 12, 12}, [SPR_LANTERN] = {LANTERN, 18, 14},
    [SPR_RISER] = {RISER, 10, 16}, [SPR_PIPEHORSE] = {PIPEHORSE, 10, 14}, [SPR_BLOATER] = {BLOATER, 12, 12},
    [SPR_STILT] = {STILT, 16, 12}, [SPR_SUNSTAR] = {SUNSTAR, 12, 12}, [SPR_BILLFISH] = {BILLFISH, 22, 8},
    [SPR_BURR] = {BURR, 12, 12}, [SPR_BURR_ANGRY] = {BURR_ANGRY, 12, 12}, [SPR_WORM] = {WORM, 10, 12},
    [SPR_PACIFIED] = {PACIFIED, 8, 6},
    [SPR_EQ0 + EQ_BUSTER] = {EQ_RAM, 12, 12}, [SPR_EQ0 + EQ_DRILL] = {EQ_AUGER, 12, 12},
    [SPR_EQ0 + EQ_BOOSTER] = {EQ_THRUST, 12, 12}, [SPR_EQ0 + EQ_CHARGE] = {EQ_BOMBS, 12, 12},
    [SPR_EQ0 + EQ_MISSILE] = {EQ_ROCKET, 12, 12}, [SPR_EQ0 + EQ_TARGET] = {EQ_FINS, 12, 12},
    [SPR_EQ0 + EQ_SPOT] = {EQ_LAMP, 12, 12}, [SPR_EQ0 + EQ_RADAR] = {EQ_SONAR, 12, 12},
    [SPR_EQ0 + EQ_EFFICIENT] = {EQ_LEAN, 12, 12}, [SPR_EQ0 + EQ_ARMOR] = {EQ_PLATES, 12, 12},
};

void kpr_art_load(void) {
    if (kpr_spr[SPR_SUB0].px) return;
    for (int i = 0; i < SPR_COUNT; i++)
        if (ART[i].s && (int)strlen(ART[i].s) == ART[i].w * ART[i].h) spr_make(&kpr_spr[i], ART[i].w, ART[i].h, ART[i].s);
}

/* tests: every sprite drawn to its stated size */
int kpr_art_check(void) {
    int bad = 0;
    for (int i = 0; i < SPR_COUNT; i++)
        if (!ART[i].s || (int)strlen(ART[i].s) != ART[i].w * ART[i].h) {
            fprintf(stderr, "kipper sprite %d: %d chars for %d x %d\n", i, ART[i].s ? (int)strlen(ART[i].s) : -1,
                    ART[i].w, ART[i].h);
            bad++;
        }
    return bad;
}
