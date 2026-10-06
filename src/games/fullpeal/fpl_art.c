/* FULL PEAL - pixel art, all drawn for UFO 40 (palette letters, see gfx.h).
 * Most things here face the camera and are symmetrical, so each is drawn
 * as its left half and mirrored when the cartridge loads. The faces of
 * Ansel and Clary are CHIME CIRCUIT's (chime_art.c). */
#include "fpl.h"
#include "../chime/chime.h"

Sprite fpl_spr[SP_COUNT];

/* the Tinkler from behind, 24 x 18: the bell, its window, its rim, the clapper's glow */
static const char SHIP[] =
    "..........kk"
    "........kkyy"
    ".......kyyyc"
    "......kyyycw"
    "......kyyycc"
    ".....kayyyyy"
    ".....kayyyyy"
    "....kaayyyyy"
    "....kaayyyyy"
    "...kaaayyyyy"
    "..kaaaaayyyy"
    ".kooaaaaaaaa"
    "kooooaaaaaaa"
    "kbbbbbbbbbbb"
    ".kmmmmmmmmmm"
    "..kkkkkkkkkm"
    "........kkCI"
    ".........kCI";

/* clapper, 16 x 14: a red clapper ball on its ring */
static const char CLAPPER[] =
    "......kk"
    ".....k.."
    "......kk"
    "....kkrr"
    "...krrrr"
    "..krwkrr"
    ".krrkkrr"
    ".krrrrrr"
    "krrrrrrr"
    "krvrrrrr"
    ".krvrrrr"
    ".kvvvrrv"
    "..kkvvvv"
    "....kkkk";

/* tenor, 18 x 16: a heavy green bell with a band */
static const char TENOR[] =
    "......kkk"
    ".....kjjj"
    "....kjjzz"
    "...kjjzzz"
    "...kjjzzz"
    "..kjjwkzz"
    "..kjjkkzz"
    "..kjjzzzz"
    ".kjjjzzzz"
    ".kfffffff"
    ".kjjjjzzz"
    "kjjjjjjzz"
    "kjjjjjjjj"
    "kffffffff"
    ".kkkkkkkk"
    "......kk.";

/* treble, 12 x 10: a small quick silver diamond */
static const char TREBLE[] =
    ".....k"
    "....kl"
    "...klw"
    "..kllw"
    ".klkwl"
    "kllkll"
    ".kllll"
    "..kgll"
    "...kgg"
    "....kk";

/* dodger, 16 x 14: a blue box with shifty eyes */
static const char DODGER[] =
    "kkkkkkkk"
    "kBBBBBBB"
    "kBuuuuuu"
    "kBukkkuu"
    "kBuwwkuu"
    "kBuwkkuu"
    "kBuuuuuu"
    "kBuuuuuu"
    "kBuuukkk"
    "kBuuuuuu"
    "kBuuuuuu"
    "kBBBBBBB"
    "kNNNNNNN"
    "kkkkkkkk";

/* bourdon, 26 x 24: a great grey bell; nothing hurts it */
static const char BOURDON[] =
    "...........kk"
    ".........kkgg"
    "........kglll"
    ".......kglllw"
    ".......kglllw"
    "......kgglllw"
    "......kggllll"
    ".....kgggllll"
    ".....kgggllll"
    ".....kgggllll"
    "....kggggllll"
    "....kssssssss"
    "....kggggllll"
    "...kgggggllll"
    "...kgggggglll"
    "..kgggggggggl"
    "..kgggggggggg"
    ".kggggggggggg"
    "kssssssssssss"
    "kgggggggggggg"
    ".kssssssssssd"
    "..kkkkkkkkkkk"
    "..........kss"
    "...........kk";

/* crosshead, 16 x 16: a violet gunner with a cross on its face */
static const char CROSSHEAD[] =
    "....kkkk"
    "...kVVVV"
    "..kVVVVV"
    ".kVVPPVV"
    ".kVPPwkk"
    "kVVPPkkw"
    "kVVVVVVw"
    "kVVVVkkw"
    "kVVVVkww"
    "kVVVVVVw"
    "kVVPPkkw"
    ".kVPPwkk"
    ".kVVPPVV"
    "..kVVVVV"
    "...kpppp"
    "....kkkk";

/* forker, 16 x 16: a blue two-pronged gunner */
static const char FORKER[] =
    ".kk....."
    "kBBk...."
    "kBuk...."
    "kBuk...."
    "kBuukkkk"
    "kBuuBBBB"
    ".kBuuuuu"
    ".kBuwkuu"
    ".kBukkuu"
    ".kBuuuuu"
    "..kBuuuu"
    "..kBBBBB"
    "...kNNNN"
    "....kkkk"
    "......kC"
    ".......k";

/* pendulum, 14 x 14: a gold box swinging a weight */
static const char PENDULUM[] =
    "kkkkkkk"
    "kyyyyyy"
    "kyaaaaa"
    "kyakkaa"
    "kyawkaa"
    "kyaaaaa"
    "kyaaaak"
    "kyyyyyk"
    "kkkkkkk"
    "......k"
    "......k"
    ".....ko"
    "....kao"
    ".....kk";

/* sally, the rope-worm's head 14 x 12, and a segment 10 x 10 */
static const char SALLY[] =
    "...kkkk"
    "..kCCCC"
    ".kCCCCC"
    "kCCwkCC"
    "kCCkkCC"
    "kCCCCCC"
    "kuCCCCC"
    "kuCCCkk"
    ".kuCCkr"
    "..kuuCC"
    "...kkkk"
    "......k";
static const char SALLY_SEG[] =
    "..kkk"
    ".kuCC"
    "kuCCC"
    "kuCCI"
    "kuCCI"
    "kuCCC"
    "kuuCC"
    ".kuuu"
    "..kkk"
    ".....";

/* lookout, 16 x 12: an orange gun-pod */
static const char LOOKOUT[] =
    "....kkkk"
    "...koooo"
    "..kooaaa"
    ".kooawkk"
    "kooaakkk"
    "kooaaaaa"
    "koooooaa"
    "kmmooooo"
    ".kmmmmmm"
    "..kkkkkk"
    "...kk..k"
    "....k..k";

/* mote, 10 x 10 */
static const char MOTE[] =
    "..kkk"
    ".kKKK"
    "kKKwK"
    "kKKKK"
    "kPKKK"
    "kPKKk"
    "kPPKK"
    ".kPPP"
    "..kkk"
    ".....";

/* nibbler, 14 x 12, two frames (jaws) */
static const char NIBBLER[] =
    "..kkkkk"
    ".kPPPPP"
    "kPPPwkP"
    "kPPPkkP"
    "kPPPPPP"
    "kPPkkkk"
    "kPkwkwk"
    "kPk...."
    "kPkwkwk"
    "kPPkkkk"
    ".kVVVVV"
    "..kkkkk";
static const char NIBBLER2[] =
    "..kkkkk"
    ".kPPPPP"
    "kPPPwkP"
    "kPPPkkP"
    "kPPPPPP"
    "kPPPPPP"
    "kPPkkkk"
    "kPkwkwk"
    "kPPkkkk"
    "kPPPPPP"
    ".kVVVVV"
    "..kkkkk";

/* caltrop, 16 x 16: a spiked ball */
static const char CALTROP[] =
    ".......k"
    "......kl"
    ".k....kg"
    ".klk.kgg"
    "..kgkggs"
    "...kgggg"
    "..kggsgg"
    "kkggggsg"
    "lgggsggg"
    "kkgggggs"
    "..kgsggg"
    "...kgggg"
    "..kgkggs"
    ".klk.kgg"
    ".k....kg"
    ".......k";

/* brooder, 18 x 16: a lime sac full of motes */
static const char BROODER[] =
    "......kkk"
    "....kkiii"
    "...kiiiii"
    "..kiizzzi"
    ".kiizKKzi"
    ".kizKKKzi"
    "kiizKKzzi"
    "kiiizzwkk"
    "kiiiiikkk"
    "kizziiiii"
    "kizKKziii"
    ".kzKKziii"
    ".kizziiii"
    "..kjjjjjj"
    "...kkkkkk"
    ".....k..k";

/* wisp, 12 x 12: Queen Sordina's pink skull-moths */
static const char WISP[] =
    "k....."
    "Kk..kk"
    "KKkkKK"
    ".KKKKK"
    ".KKkkK"
    ".KKkkK"
    "..KKKK"
    "...KKK"
    "...KkK"
    "...KKK"
    "....kk"
    "......";

/* flare, 8 x 14: the Inkwell's, nose down */
static const char FLARE[] =
    "...k"
    "..kr"
    "..ka"
    "..ky"
    "..ky"
    "..kl"
    ".kll"
    ".kll"
    ".kll"
    ".kll"
    "kkll"
    ".kll"
    "..kl"
    "...k";

/* a balloon, 12 x 16 */
static const char BALLOON[] =
    "...kkk"
    "..krrr"
    ".krrrr"
    "krrwwr"
    "krrwrr"
    "krrrrr"
    "krrrrr"
    "kvrrrr"
    ".kvrrr"
    ".kvvrr"
    "..kvvv"
    "...kkk"
    ".....k"
    "......"
    ".....k"
    "......";

/* the owl that turns up on a perfect wave, 24 x 22 */
static const char OWL[] =
    "..kk........"
    "..ktk......."
    "..kttkkkkkkk"
    "..kttttttttt"
    ".kttkkkktttt"
    ".ktkccccktkk"
    ".ktkcckkckcc"
    ".ktkcckkckcc"
    ".ktkccccktkk"
    ".kttkkkkkkya"
    ".kttteeeekka"
    "kttteeeeeekk"
    "ktteehehheee"
    "ktteeeeeeeee"
    "ktteehehheee"
    "ktteeeeeeeee"
    "kttteeeeeeee"
    ".kttteeeeeee"
    "..kktttttttt"
    "....kkkkkkkk"
    "...kaaka..ka"
    "............";

/* an orb (the bosses' options), 12 x 12 */
static const char ORB[] =
    "....kk"
    "..kkPP"
    ".kPPKK"
    ".kPKwK"
    "kPPKKK"
    "kPPPKK"
    "kpPPPP"
    "kpPPPP"
    ".kpPPP"
    ".kppPP"
    "..kkpp"
    "....kk";

/* ---- the bosses, drawn at twice their depth's scale ---------------- */
/* the Gloameye, 48 x 40: a dark iron bell with one great eye */
static const char GLOAMEYE[] =
    "....................kkkk"
    "..................kkdddd"
    "................kkdddsss"
    "..............kkddssssss"
    ".............kddsssssggg"
    "............kdsssssggggg"
    "...........kdssssggggggg"
    "..........kdsssgggkkkkkk"
    ".........kdsssggkkPPPPPP"
    ".........kdssggkPPPKKKKK"
    "........kdsssgkPPKKKKKKK"
    "........kdssggkPKKKKwwKK"
    ".......kdsssgkPKKKKkkkkw"
    ".......kdsssgkPKKKkkkkkk"
    ".......kdssggkPKKKkkkkkk"
    "......kdsssggkPKKKkkkkkk"
    "......kdsssggkPPKKKkkkkk"
    "......kdsssggkPPKKKKKkkk"
    ".....kdssssggkVPPKKKKKKK"
    ".....kdssssgggkVPPPKKKKK"
    ".....kdssssgggkkVVPPPPPP"
    "....kddsssssggggkkkVVVVV"
    "....kddssssssgggggkkkkkk"
    "...kdddssssssggggggggggg"
    "...kdddsssssssgggggggggg"
    "..kddddsssssssssgggggggg"
    "..kddddssssssssssggggggg"
    ".kdddddssssssssssssggggg"
    ".kdddddsssssssssssssssss"
    "kddddddsssssssssssssssss"
    "knnnnnnnnnnnnnnnnnnnnnnn"
    "kddddddddddddddddddddddd"
    "knnnnnnnnnnnnnnnnnnnnnnn"
    ".kkkkkkkkkkkkkkkkkkkkkkk"
    "..................kkkkkk"
    "...................kssss"
    "....................kkss"
    ".....................kgg"
    ".....................kgg"
    "......................kk";

/* Knucklebell, 56 x 40: a riveted bell-head with a big round nose */
static const char KNUCKLE[] =
    "..........................kk"
    "........................kkss"
    "......................kkssgg"
    "....................kkssgggg"
    "..................kkssgggggg"
    "................kkssgggggggg"
    "..............kkssgggggggggg"
    ".............kssgggggglggggg"
    "............ksggggggggglgggg"
    "...........ksgggwgggggggllgg"
    "..........ksggggggggggggglll"
    "..........ksgggggkkkkkkggggg"
    ".........ksgggggkCCCCCkkgggg"
    ".........ksggggkCIICCCCkgggg"
    "........ksgggggkCIwCCCCkgggg"
    "........ksgggggkCCCCCCCkgggg"
    "........ksgggggkuCCCCCukgggg"
    ".......ksggggggkkuuuuukkkggg"
    ".......ksggggggggkkkkkgggggg"
    ".......ksggggggggggggggggggg"
    "......ksgggggggggggggggKKKKK"
    "......ksgggggggggggggKKPPPPP"
    "......ksggggggggggggKPPwwPPP"
    ".....ksggggggggggggKPPwPPPPP"
    ".....ksggggggggggggKPPPPPPPP"
    ".....kssgggggggggggKVPPPPPPP"
    "....ksssgggggggggggkKVVPPPPP"
    "....ksssggggggggggggkkKVVVVV"
    "...kssssggggggggggggggkkkkkk"
    "...kssssgggkgggggkgggggggggg"
    "..ksssssgggkgggggkgggggggggg"
    "..ksssssgggkgggggkgggggggggg"
    ".kssssssgggggggggggggggggggg"
    ".kdddddddddddddddddddddddddd"
    "ksssssssssssssssssssssssssss"
    "kddddddddddddddddddddddddddd"
    ".kkkkkkkkkkkkkkkkkkkkkkkkkkk"
    ".......kss........kss......."
    ".......kss........kss......."
    "........kk.........kk.......";

/* a fist, 24 x 24 */
static const char FIST[] =
    "....kkkkkkkk"
    "...kggkggkgg"
    "..kgllkgllkg"
    "..kgllkgllkg"
    "..kgllkgllkg"
    "..kgglkgglkg"
    ".kkkkkkkkkkk"
    "kggggggggggg"
    "kgllllllllll"
    "kgllllllllll"
    "kgllllllllll"
    "kgglllllllll"
    "kgggllllllll"
    "kggggggggggg"
    "kssssssssggg"
    ".kssssssssss"
    "..kkkkkkkkkk"
    "...kddddddd."
    "...kddddddd."
    "...kdsdsdsd."
    "...kddddddd."
    "...kddddddd."
    "....kkkkkkk."
    "............";

/* the Inkwell, 48 x 44: a squat ink pot with a squid in it */
static const char INKWELL[] =
    "................kkkkkkkk"
    "..............kkVVVVVVVV"
    ".............kVVVVPPPPPP"
    "............kVVVPPPPPPPP"
    "...........kVVPPPPPPPPPP"
    "...........kVPPPPwwkPPPP"
    "..........kVPPPPPwkkkPPP"
    "..........kVPPPPPPkkkPPP"
    "..........kVPPPPPPPPPPPP"
    "..........kVPPPPPPPPPPPP"
    "..........kVVPPPPPPPkkkk"
    "...........kVVPPPPPPPPPP"
    "............kkVVVVVVVVVV"
    "..........kkkkkkkkkkkkkk"
    ".........knnnnnnnnnnnnnn"
    "........kNNNNNNNNNNNNNNN"
    ".......kNBBBBBBBBBBBBBBB"
    "......kNBBBuuuuuuuuuuuuu"
    ".....kNBBuuuuuuuuuuuuuuu"
    "....kNBBuuuuuuuuuuuuuuuu"
    "...kNBBuuuuuuCCCCuuuuuuu"
    "...kNBuuuuuuCIIICuuuuuuu"
    "..kNBBuuuuuuCIwICuuuuuuu"
    "..kNBuuuuuuuCIIICuuuuuuu"
    "..kNBuuuuuuuuCCCuuuuuuuu"
    ".kNBBuuuuuuuuuuuuuuuuuuu"
    ".kNBuuuuuuuuuuuuuuuuuuuu"
    ".kNBuuuuuuuuuuuuuuuuuuuu"
    "kNBBuuuuuuuuuuuuuuuuuuuu"
    "kNBuuuuuuuuuuuuuuuuuuuuu"
    "kNBBuuuuuuuuuuuuuuuuuuuu"
    "kNNBBuuuuuuuuuuuuuuuuuuu"
    "kNNBBBuuuuuuuuuuuuuuuuuu"
    ".kNNBBBBuuuuuuuuuuuuuuuu"
    ".kNNNBBBBBBuuuuuuuuuuuuu"
    "..kNNNNBBBBBBBBBBBBBBBBB"
    "...kkNNNNNNNNNNNNNNNNNNN"
    ".....kkkkkkkkkkkkkkkkkkk"
    "......kVk...kVk...kVk..k"
    "......kVk...kVk...kVk..k"
    ".....kVk....kVk...kVk..k"
    ".....kVk...kVk.....kVk.k"
    "....kVk....kVk.....kVk.."
    "....kk.....kk.......kk..";

/* Shellback, 64 x 32: a wide domed shell with a beak */
static const char SHELLBACK[] =
    "......................kkkkkkkkkk"
    "..................kkkkjjjjjjjjjj"
    "...............kkkjjjjjzzzzzzzzz"
    ".............kkjjjjzzzzzzzzzzzzz"
    "...........kkjjjzzzzzkkkkkkzzzzz"
    "..........kjjjzzzzzkkiiiiiikkzzz"
    ".........kjjzzzzzzkiiiiiiiiikzzz"
    "........kjjzzzzzzkiiiiiiiiiiikzz"
    ".......kjjzzzzzzkiiikkkkkiiiikzz"
    "......kjjzzzzzzzkiikzzzzzkiiikzz"
    ".....kjjzzzzzzzzkiikzzzzzkiiikzz"
    "....kjjzzzzzzzzzzkiikkkkkiiikzzz"
    "...kjjzzzzzzzzzzzkkiiiiiiiikkzzz"
    "..kjjzzzzzzzzzzzzzzkkkkkkkkzzzzz"
    ".kjjzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
    "kjjzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
    "kfffffffffffffffffffffffffffffff"
    "kfjjfjjfjjfjjfjjfjjfjjfjjfjjfjjf"
    "kfffffffffffffffffffffffffffffff"
    ".kqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    "..kkkkkkkkkkkkkkkkkqqqqqqqqqqqqq"
    "...................kqqqqqqqqqqqq"
    "....................kqqqkkkkkkkk"
    ".....................kqkaaaaaaaa"
    "......................kkaaaawwaa"
    "......................kaaaaawkaa"
    "......................kaaaaaaaaa"
    "......................kkaaaaaaaa"
    ".......................koaaaaaaa"
    "........................kooooooo"
    ".........................kkkkkkk"
    "................................";

/* Queen Sordina, 56 x 48: a crowned bell with a muffled face; and with
 * her mouth open */
static const char SORDINA[] =
    "..............k....k....k..."
    ".............kyk..kyk..kyk.."
    ".............kyyk.kyyk.kyyk."
    ".............kyyykkyyykkyyyk"
    ".............kaaaaaaaaaaaaaa"
    ".............kkkkkkkkkkkkkkk"
    "...........kkVVVVVVVVVVVVVVV"
    ".........kkVVVVVPPPPPPPPPPPP"
    "........kVVVVPPPPPPPPPPPPPPP"
    ".......kVVVPPPPPPPPPPPPPPPPP"
    "......kVVPPPPPPPPPPPPPPPPPPP"
    "......kVPPPPPPkkkkkPPPPPPPPP"
    ".....kVPPPPPPkwwwKkPPPPPPPPP"
    ".....kVPPPPPPkwkkKkPPPPPPPPP"
    ".....kVPPPPPPPkkkkPPPPPPPPPP"
    "....kVPPPPPPPPPPPPPPPPPPPPPP"
    "....kVPPPPPPPPPPPPPPPPPPPPPP"
    "....kVPPPPPPPPPPPPPPPPPPPPPP"
    "...kVPPPPPPPPPPPPPPPPPPPPPPP"
    "...kVPPPPPPPPPPPPPPPkkkkkkkk"
    "...kVPPPPPPPPPPPPPPkgggggggg"
    "...kVPPPPPPPPPPPPPkgllllllll"
    "..kVPPPPPPPPPPPPPPkgllllllll"
    "..kVPPPPPPPPPPPPPPkggggggggg"
    "..kVPPPPPPPPPPPPPPPkkkkkkkkk"
    "..kVPPPPPPPPPPPPPPPPPPPPPPPP"
    ".kVPPPPPPPPPPPPPPPPPPPPPPPPP"
    ".kVPPPPPPPPPPPPPPPPPPPPPPPPP"
    ".kVPPPPPPPPPPPPPPPPPPPPPPPPP"
    "kVVPPPPPPPPPPPPPPPPPPPPPPPPP"
    "kVVPPPPPPPPPPPPPPPPPPPPPPPPP"
    "kppppppppppppppppppppppppppp"
    "kaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "kayayayayayayayayayayayayaya"
    "kaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    ".kpppppppppppppppppppppppppp"
    "..kkkkkkkkkkkkkkkkkkkkkkkkkk"
    "...........kVVk.......kVVk.."
    "...........kVVk.......kVVk.."
    "...........kVVk.......kVVk.."
    "...........kVk.........kVk.."
    "...........kVk.........kVk.."
    "..........kVk...........kVk."
    "..........kVk...........kVk."
    "..........kk.............kk."
    "............................"
    "............................"
    "............................";

/* make a sprite from its left half, mirrored; short rows are padded */
static void mirror_make(Sprite *s, int hw, int h, const char *half) {
    int w = hw * 2;
    char *full = malloc((size_t)(w * h + 1));
    if (!full) return;
    int len = (int)strlen(half), at = 0;
    /* rows may be shorter than hw if written so: walk the string by rows of hw */
    for (int r = 0; r < h; r++)
        for (int c = 0; c < hw; c++) {
            char ch = at < len ? half[r * hw + c] : '.';
            if (r * hw + c >= len) ch = '.';
            full[r * w + c] = ch;
            full[r * w + (w - 1 - c)] = ch;
        }
    (void)at;
    full[w * h] = 0;
    spr_make(s, w, h, full);
    free(full);
}

void fpl_art_load(void) {
    if (fpl_spr[SP_SHIP].px) return;
    chm_art_load(); /* Ansel's and Clary's faces */
    mirror_make(&fpl_spr[SP_SHIP], 12, 18, SHIP);
    mirror_make(&fpl_spr[SP_CLAPPER], 8, 14, CLAPPER);
    mirror_make(&fpl_spr[SP_TENOR], 9, 16, TENOR);
    mirror_make(&fpl_spr[SP_TREBLE], 6, 10, TREBLE);
    mirror_make(&fpl_spr[SP_DODGER], 8, 14, DODGER);
    mirror_make(&fpl_spr[SP_BOURDON], 13, 24, BOURDON);
    mirror_make(&fpl_spr[SP_CROSSHEAD], 8, 16, CROSSHEAD);
    mirror_make(&fpl_spr[SP_FORKER], 8, 16, FORKER);
    mirror_make(&fpl_spr[SP_PENDULUM], 7, 14, PENDULUM);
    mirror_make(&fpl_spr[SP_SALLY], 7, 12, SALLY);
    mirror_make(&fpl_spr[SP_SALLY_SEG], 5, 10, SALLY_SEG);
    mirror_make(&fpl_spr[SP_LOOKOUT], 8, 12, LOOKOUT);
    mirror_make(&fpl_spr[SP_MOTE], 5, 10, MOTE);
    mirror_make(&fpl_spr[SP_NIBBLER], 7, 12, NIBBLER);
    mirror_make(&fpl_spr[SP_NIBBLER2], 7, 12, NIBBLER2);
    mirror_make(&fpl_spr[SP_CALTROP], 8, 16, CALTROP);
    mirror_make(&fpl_spr[SP_BROODER], 9, 16, BROODER);
    mirror_make(&fpl_spr[SP_WISP], 6, 12, WISP);
    mirror_make(&fpl_spr[SP_FLARE], 4, 14, FLARE);
    mirror_make(&fpl_spr[SP_BALLOON], 6, 16, BALLOON);
    mirror_make(&fpl_spr[SP_OWL], 12, 22, OWL);
    mirror_make(&fpl_spr[SP_ORB], 6, 12, ORB);
    mirror_make(&fpl_spr[SP_GLOAMEYE], 24, 40, GLOAMEYE);
    mirror_make(&fpl_spr[SP_KNUCKLE], 28, 40, KNUCKLE);
    mirror_make(&fpl_spr[SP_FIST], 12, 24, FIST);
    mirror_make(&fpl_spr[SP_INKWELL], 24, 44, INKWELL);
    mirror_make(&fpl_spr[SP_SHELLBACK], 32, 32, SHELLBACK);
    mirror_make(&fpl_spr[SP_SORDINA], 28, 48, SORDINA);
}
