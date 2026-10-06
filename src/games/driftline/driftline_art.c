/* DRIFTLINE - pixel art (palette-letter strings, see gfx.h). All drawn for
 * UFO 40. The bosses, the hoops, the scenery and the saucer are drawn in
 * driftline_draw.c. */
#include "driftline.h"

Sprite dfl_spr[SP_COUNT];

/* Lou's red convertible, 24 x 12, facing right; the gun is drawn on top */
static const char CAR[] =
    "...........kkk.........."
    "..........kbbbk........."
    "..........kcsck....k...."
    "....kkkkkkkcckkkkkkIk..."
    "...krrrrrrrrrrrrrrrrrk.."
    "..krwwrrrrrrrrrrrrrrrrk."
    "..krrrrrrrrrrrrrrrrrrrrk"
    ".kmrrrrrrrrrrrrrrrrrrrak"
    ".kmmmmkkkmmmmmmmkkkmmmmk"
    "..kkkkgggkkkkkkkgggkkkk."
    ".....kgsgk.....kgsgk...."
    "......kkk.......kkk.....";

/* a road hog, 24 x 11 */
static const char HOG[] =
    "......kkkkkkkkkk........"
    ".....kuuukyyykuuuk......"
    "....kuuuukyyykuuuuk....."
    "..kkkkkkkkkkkkkkkkkkkk.."
    ".kyyyyyyyyyyyyyyyyyyyyk."
    "kyywyyyyykkyyyyyyyyyyyyk"
    "kaaaaaaaakkaaaaaaaaaaaak"
    "kkkkkkkkkkkkkkkkkkkkkkkk"
    ".kkgggkkkkkkkkkkkgggkk.."
    "...kgsgk.......kgsgk...."
    "....kkk.........kkk.....";

/* the kite (a biplane), 16 x 10, two propeller frames */
static const char KITE1[] =
    "..ooooooooooo..."
    "......k..k......"
    ".k...kkooook...."
    "kok.kooooooookw."
    "kooooooooooooowk"
    "kok.kkooooookkw."
    ".k...kk....k...."
    "......k..k......"
    "..ooooooooooo..."
    "................";
static const char KITE2[] =
    "..ooooooooooo..."
    "......k..k....w."
    ".k...kkooook..w."
    "kok.kooooooookk."
    "kooooooooooooook"
    "kok.kkooooookkk."
    ".k...kk....k..w."
    "......k..k....w."
    "..ooooooooooo..."
    "................";

/* a buzzer (a little drone), 10 x 10 */
static const char BUZZER[] =
    "kkkk..kkkk"
    "...k..k..."
    "...kkkk..."
    "..kssssk.."
    ".ksrrrrsk."
    ".ksryyrsk."
    ".ksrrrrsk."
    "..kssssk.."
    "...kggk..."
    "....kk....";

/* a rotor (helicopter), 22 x 12, two frames */
static const char ROTOR1[] =
    "kkkkkkkkkkkkkkkkkk...."
    "........kk............"
    "......kkkkkkk........."
    "....kooooooooak......."
    "...kuuoooooooaakkkkkkk"
    "..kuuuuoooooooooooook."
    "..kuuuuoooooooak....k."
    "...kooooooooaak......."
    "....kkkkkkkkkk........"
    ".....k......k........."
    "...kkkkkkkkkkkkk......"
    "......................";
static const char ROTOR2[] =
    "......kkkkkkkkkkkkkkkk"
    "........kk............"
    "......kkkkkkk.......k."
    "....kooooooooak.....k."
    "...kuuoooooooaakkkkkkk"
    "..kuuuuoooooooooooook."
    "..kuuuuoooooooak....k."
    "...kooooooooaak......."
    "....kkkkkkkkkk........"
    ".....k......k........."
    "...kkkkkkkkkkkkk......"
    "......................";

/* a shard (the glass square), 12 x 12 */
static const char SHARD[] =
    "kkkkkkkkkkkk"
    "kwiiiiiiiijk"
    "kiwiiiiiijjk"
    "kiiziiiijzjk"
    "kiiizzzjjzjk"
    "kiiiz..jzzjk"
    "kiiiz..jzzjk"
    "kiiijjjjzzjk"
    "kiijzzzzzjjk"
    "kijzzzzzzzjk"
    "kjjjjjjjjjfk"
    "kkkkkkkkkkkk";

/* a prism, 16 x 20 */
static const char PRISM[] =
    ".......kk......."
    "......kwIk......"
    ".....kwIICk....."
    ".....kIICCk....."
    "....kwIICCuk...."
    "....kIIICCuk...."
    "...kwIIICCuuk..."
    "...kIIICCCuuk..."
    "..kwIIICCCuuuk.."
    "..kIIIICCCuuuk.."
    ".kwIIIICCCuuuBk."
    "kIIIIICCCCuuuBBk"
    ".kIIIICCCuuuBBk."
    "..kIIICCCuuBBk.."
    "...kIICCuuBBk..."
    "....kICCuBBk...."
    ".....kCCuBk....."
    "......kCBk......"
    ".......kk......."
    "................";

/* a tumbler (the spinning cube), 14 x 14, two frames */
static const char TUMBLER1[] =
    "kkkkkkkkkkkkkk"
    "kPPPPPPPPPPPVk"
    "kPKKPPPPPPPPVk"
    "kPKPPPPPPPPPVk"
    "kPPPPkkkkPPPVk"
    "kPPPPkVVkPPPVk"
    "kPPPPkVVkPPPVk"
    "kPPPPkkkkPPPVk"
    "kPPPPPPPPPPPVk"
    "kPPPPPPPPPPPVk"
    "kPPPPPPPPPPPVk"
    "kPVVVVVVVVVVVk"
    "kVVVVVVVVVVVVk"
    "kkkkkkkkkkkkkk";
static const char TUMBLER2[] =
    "......kk......"
    ".....kPPk....."
    "....kPKPPk...."
    "...kPKPPPPk..."
    "..kPPPPPPPPk.."
    ".kPPPPkkPPPVk."
    "kPPPPkVVkPPPVk"
    "kPPPPkVVkPPPVk"
    ".kPPPPkkPPPVk."
    "..kPPPPPPPVk.."
    "...kPPPPPVk..."
    "....kPPPVk...."
    ".....kVVk....."
    "......kk......";

/* a skull, 14 x 14 */
static const char SKULL[] =
    "....kkkkkk...."
    "..kkrrrrrrkk.."
    ".krrKrrrrrrrk."
    ".krKrrrrrrrrk."
    "krrrrrrrrrrrrk"
    "krrkkkrrkkkrrk"
    "krkkykrrkykkrk"
    "krkkkkrrkkkkrk"
    "krrkkrrrrkkrrk"
    ".krrrrkkrrrrk."
    "..kmrrrrrrmk.."
    "..kmwkwkwkmk.."
    "...kmkmkmkk..."
    "....kkkkkk....";

/* a sheet (the ghost), 16 x 18, two frames */
static const char SHEET1[] =
    ".....kkkkkk....."
    "...kkwwwwwwkk..."
    "..kwwwwwwwwwwk.."
    ".kwwwwwwwwwwwwk."
    ".kwwkkwwwwkkwwk."
    "kwwwkkwwwwkkwwwk"
    "kwwwwwwwwwwwwwwk"
    "kwwwwwkkkkwwwwwk"
    "kwwwwwkkkkwwwwwk"
    "kwwwwwwwwwwwwwwk"
    "kllwwwwwwwwwwwlk"
    "kllwwwwwwwwwwllk"
    "kllllwwwwwwwlllk"
    "klllllllllllllk."
    "kllkllllkllllk.."
    "klk.kllk.klllk.."
    "kk...kk...kllk.."
    "...........kk...";
static const char SHEET2[] =
    ".....kkkkkk....."
    "...kkwwwwwwkk..."
    "..kwwwwwwwwwwk.."
    ".kwwwwwwwwwwwwk."
    ".kwwkkwwwwkkwwk."
    "kwwwkkwwwwkkwwwk"
    "kwwwwwwwwwwwwwwk"
    "kwwwwwwkkwwwwwwk"
    "kwwwwwwkkwwwwwwk"
    "kwwwwwwwwwwwwwwk"
    "klwwwwwwwwwwwwlk"
    "kllwwwwwwwwwwllk"
    "klllwwwwwwwwlllk"
    ".kllllllllllllk."
    "..kllllkllllkllk"
    "..klllk.klllk.kk"
    "..kllk...kkk...."
    "...kk...........";

/* a snapper (the chaser), 16 x 16, mouth shut and open */
static const char SNAPPER[] =
    ".....kkkkkk....."
    "...kkBBBBBBkk..."
    "..kBBuuBBBBBBk.."
    ".kBuuBBBBBBBBBk."
    ".kBuBBkkBBBBBBk."
    "kBBBBkwykBBBBBBk"
    "kBBBBkkkkBBBBBBk"
    "kBBBBBBBBBBBBBBk"
    "kBBBBBBBBBBBBBBk"
    "kBBwkwkwkwkwBBBk"
    "kBBkwkwkwkwkBBBk"
    ".kBBBBBBBBBBBBk."
    ".kBBBBBBBBBBBNk."
    "..kNBBBBBBBBNk.."
    "...kkNNNNNNkk..."
    ".....kkkkkk.....";
static const char SNAPPER2[] =
    ".....kkkkkk....."
    "...kkBBBBBBkk..."
    "..kBBuuBBBBBBk.."
    ".kBuuBBBBBBBBBk."
    ".kBuBBkkBBBBBBk."
    "kBBBBkwykBBBBBBk"
    "kBBBBkkkkBBBBBBk"
    "kBBBBBBBBkkkkkkk"
    "kBBBBBBkwvwvwvwk"
    "kBBBBBkvvvvvvvk."
    "kBBBBBkwvwvwvwk."
    ".kBBBBBkkkkkkkk."
    ".kBBBBBBBBBBBNk."
    "..kNBBBBBBBBNk.."
    "...kkNNNNNNkk..."
    ".....kkkkkk.....";

/* a slab (the falling stone face), 24 x 24 */
static const char SLAB[] =
    "kkkkkkkkkkkkkkkkkkkkkkkk"
    "kllllllllllllllllllllggk"
    "klggggggggggggggggggggsk"
    "klgsgggggggggggggggsggsk"
    "klggggggggggggggggggggsk"
    "klggkkkkkgggggkkkkkgggsk"
    "klggkwwwkgggggkwwwkgggsk"
    "klggkwkwkgggggkwkwkgggsk"
    "klggkkkkkgggggkkkkkgggsk"
    "klggggggggggggggggggggsk"
    "klggggggggkkgggggggggssk"
    "klgggggggggkkggggggggssk"
    "klggggggggggkgggggggggsk"
    "klgggggggggggggggggggssk"
    "klgggkkkkkkkkkkkkkggggsk"
    "klgggkwwkwwkwwkwwkggggsk"
    "klgggkkkkkkkkkkkkkggggsk"
    "klggggggggggggggggggggsk"
    "klgsgggggggggggggggsggsk"
    "klggggggggggggggggggggsk"
    "klggggggggggggggggggggsk"
    "kgsssssssssssssssssssssk"
    "kkkkkkkkkkkkkkkkkkkkkkkk"
    "........................";

/* a jelly, 12 x 14, two frames */
static const char JELLY1[] =
    "...kkkkkk..."
    "..kKKKKKKk.."
    ".kKwKKKKKKk."
    "kKwKKKKKKKKk"
    "kKKKKKKKKKKk"
    "kPPPPPPPPPPk"
    ".kkkkkkkkkk."
    "..K.K..K.K.."
    "..K.K..K.K.."
    ".K..K..K..K."
    ".K...K.K..K."
    "..K..K..K.K."
    "..K.K...K..K"
    "...K....K..."
    ;
static const char JELLY2[] =
    "...kkkkkk..."
    "..kKKKKKKk.."
    ".kKwKKKKKKk."
    "kKwKKKKKKKKk"
    "kKKKKKKKKKKk"
    "kPPPPPPPPPPk"
    ".kkkkkkkkkk."
    "..K.K..K.K.."
    ".K..K..K..K."
    ".K...K.K..K."
    "..K..K..K.K."
    "..K.K...K..K"
    "...K....K..."
    "...K.....K..";

/* a dartfish, 14 x 8, facing right */
static const char DART[] =
    "........kk...."
    "k.....kkyyk..."
    "kk..kkyyyyykk."
    "kykkyyyyyyywyk"
    "kykkyyyyyyyyak"
    "kk..kkaaaaakk."
    "k.....kkaak..."
    "........kk....";

/* a squirt (the octopus), 18 x 16, two frames */
static const char SQUIRT1[] =
    "......kkkkkk......"
    "....kkoooooookk..."
    "...koooooooooook.."
    "..kooKoooooooooak."
    "..koKoooooooooook."
    ".koooookkooookkook"
    ".koooookwkookwkook"
    ".koooookkooookkook"
    ".kooooooooooooooak"
    "..koooooooooooaak."
    "...kkooooooooakk.."
    "..kokkoookooakkok."
    ".kok.koak.kook.kok"
    ".ko..kok..koak..ko"
    "kok.kok...kok...ko"
    "kk..kk.....kk...kk";
static const char SQUIRT2[] =
    "......kkkkkk......"
    "....kkoooooookk..."
    "...koooooooooook.."
    "..kooKoooooooooak."
    "..koKoooooooooook."
    ".koooookkooookkook"
    ".koooookwkookwkook"
    ".koooookkooookkook"
    ".kooooooooooooooak"
    "..koooooooooooaak."
    "...kkooooooooakk.."
    "...kokoookooakok.."
    "...koko.kook.kok.."
    "....kok.koak.ko..."
    "....kok..kok.ko..."
    ".....kk...kk.kk...";

/* a shark, 32 x 14, facing right */
static const char SHARK[] =
    "...............kk..............."
    "..............ksk..............."
    ".............kssk..............."
    "k...........ksssk..............."
    "kk.......kkksssssskkkkk........."
    "ksk...kkksssssssssssssskkk......"
    "kssk.kssssssssssssssssssssskk..."
    "ksssksssssssssssssssssssskwssk.."
    "kssssssssssssssssssssssssskkssk."
    "kssk.ksswwwwwwwwwwwwwwwwwwkwkwsk"
    "ksk...kkwwwwwwwwwwwwwwwwkkwkwk.."
    "kk.......kkkwwwwwwwwwkkk..kkk..."
    "k...........kkkkkkkkk..........."
    "................................";

/* a palm tree for the roadside, 16 x 32 */
static const char PALM[] =
    "....ff....ff...."
    "..ffzzf..fzzff.."
    ".fzzzzzffzzzzzf."
    "fzzf..fzzf..fzzf"
    "fzf..fzbbzf..fzf"
    "ff..fzzbbzzf..ff"
    "f..fzzf.bfzzf..f"
    "...fzf..b..fzf.."
    "...ff...bb..ff.."
    "........bb......"
    "........bb......"
    ".......bb......."
    ".......bb......."
    ".......bt......."
    ".......bb......."
    ".......bb......."
    "......bb........"
    "......bt........"
    "......bb........"
    "......bb........"
    "......bb........"
    "......bt........"
    "......bb........"
    "......bb........"
    ".......bb......."
    ".......bt......."
    ".......bb......."
    ".......bb......."
    ".......bb......."
    "......bbbb......"
    ".....bbbbbb....."
    "................";

/* the coin in the bonus stage, 10 x 10 */
static const char COIN[] =
    "...kkkk..."
    ".kkyyyykk."
    ".kywwyyak."
    "kyywyyyaak"
    "kyyyykyaak"
    "kyyyykyaak"
    "kyyyyyyaak"
    ".kyyyyaak."
    ".kkaaaakk."
    "...kkkk...";

/* a street lamp, 8 x 30 */
static const char LAMP[] =
    "kkkkkk.."
    "kyyyyyk."
    ".kkkkkgk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    "......gk"
    ".....sss"
    "........";

/* the second car: the same body in teal */
static void recolour(char *s, const char *from, const char *to) {
    for (char *p = s; *p; p++) {
        const char *f = strchr(from, *p);
        if (f) *p = to[f - from];
    }
}

void dfl_art_load(void) {
    static bool done = false;
    if (done) return;
    done = true;
    static char car2[sizeof CAR], wreck[sizeof HOG];
    memcpy(car2, CAR, sizeof CAR);
    recolour(car2, "rmwb", "CqIy");
    memcpy(wreck, HOG, sizeof HOG);
    recolour(wreck, "yauw", "esdg");
    spr_make(&dfl_spr[SP_CAR], 24, 12, CAR);
    spr_make(&dfl_spr[SP_CAR2], 24, 12, car2);
    spr_make(&dfl_spr[SP_HOG], 24, 11, HOG);
    spr_make(&dfl_spr[SP_WRECK], 24, 11, wreck);
    spr_make(&dfl_spr[SP_KITE1], 16, 10, KITE1);
    spr_make(&dfl_spr[SP_KITE2], 16, 10, KITE2);
    spr_make(&dfl_spr[SP_BUZZER], 10, 10, BUZZER);
    spr_make(&dfl_spr[SP_ROTOR1], 22, 12, ROTOR1);
    spr_make(&dfl_spr[SP_ROTOR2], 22, 12, ROTOR2);
    spr_make(&dfl_spr[SP_SHARD], 12, 12, SHARD);
    spr_make(&dfl_spr[SP_PRISM], 16, 20, PRISM);
    spr_make(&dfl_spr[SP_TUMBLER1], 14, 14, TUMBLER1);
    spr_make(&dfl_spr[SP_TUMBLER2], 14, 14, TUMBLER2);
    spr_make(&dfl_spr[SP_SKULL], 14, 14, SKULL);
    spr_make(&dfl_spr[SP_SHEET1], 16, 18, SHEET1);
    spr_make(&dfl_spr[SP_SHEET2], 16, 18, SHEET2);
    spr_make(&dfl_spr[SP_SNAPPER], 16, 16, SNAPPER);
    spr_make(&dfl_spr[SP_SNAPPER2], 16, 16, SNAPPER2);
    spr_make(&dfl_spr[SP_SLAB], 24, 24, SLAB);
    spr_make(&dfl_spr[SP_JELLY1], 12, 14, JELLY1);
    spr_make(&dfl_spr[SP_JELLY2], 12, 14, JELLY2);
    spr_make(&dfl_spr[SP_DART], 14, 8, DART);
    spr_make(&dfl_spr[SP_SQUIRT1], 18, 16, SQUIRT1);
    spr_make(&dfl_spr[SP_SQUIRT2], 18, 16, SQUIRT2);
    spr_make(&dfl_spr[SP_SHARK], 32, 14, SHARK);
    spr_make(&dfl_spr[SP_PALM], 16, 32, PALM);
    spr_make(&dfl_spr[SP_COIN], 10, 10, COIN);
    spr_make(&dfl_spr[SP_LAMP], 8, 30, LAMP);
}
