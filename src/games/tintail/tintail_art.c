/* TINTAIL - pixel art (palette-letter strings, see gfx.h). Creatures are
 * drawn facing right; the up and down poses are rotated at load time.
 * The ground tiles are drawn in code (tintail.c). */
#include "tintail.h"

Sprite tn_spr[TS_SPRITE_COUNT];

/* Twig the chameleon, from above: casque head (right), eye turret, splayed
 * feet, and the curled tail. w/l/g are the skin ramp that camouflage repaints. */
static const char TWIG_R[] =
    "................"
    "................"
    "......kk..kk...."
    ".....kgk.kgk...."
    "....kkwkkkwkkkk."
    "...kwwwwwwwwkcck"
    "..kwlwwlwwlwwkkk"
    ".kwwwwwwwwwwwwwk"
    ".kwlwwlwwlwwwkkk"
    "kwkkwwwwwwwwkcck"
    "kwk.kkwkkkwkkkk."
    "kwk..kgk.kgk...."
    "kwwk.kk..kk....."
    ".kwwk..........."
    "..kk............"
    "................";
static const char TWIG_R2[] =
    "................"
    "................"
    ".....kk....kk..."
    "....kgk...kgk..."
    "....kkwkkkwkkkk."
    "...kwwwwwwwwkcck"
    "..kwlwwlwwlwwkkk"
    ".kwwwwwwwwwwwwwk"
    ".kwlwwlwwlwwwkkk"
    "kwkkwwwwwwwwkcck"
    "kwk.kkwkkkwkkkk."
    "kwk...kgk.kgk..."
    "kwwk...kk..kk..."
    ".kwwk..........."
    "..kk............"
    "................";

/* a hatchling: the same shape, small */
static const char BABY_R[] =
    ".........."
    "...k...k.."
    "..kkkkkkk."
    ".kwlwlwwck"
    "kwwwwwwwkk"
    "kwkkkkkkk."
    "kk.k...k.."
    ".........."
    ".........."
    "..........";

/* the toad: squat, warty, crimson, a yellow eye on each side */
static const char TOAD_R[] =
    "................"
    "................"
    "...kk......kkk.."
    "..kmok....kykyk."
    "..kmrokkkkkyyyk."
    "...kmoooooookkk."
    "..kmrorrorrrrrk."
    ".kmrrrrrrrrrrrrk"
    ".kmrmrrrrmrrrcck"
    ".kmrrrrrrrrrrrrk"
    "..kmrrmrrrmrrrk."
    "...kmrrrrrrrkkk."
    "..kmrmkkkkkyyyk."
    "..kmrk....kykyk."
    "...kk......kkk.."
    "................";
static const char TOAD_BLINK[] =
    "................"
    "................"
    "...kk......kkk.."
    "..kmok....kkkkk."
    "..kmrokkkkkkkkk."
    "...kmoooooookkk."
    "..kmrorrorrrrrk."
    ".kmrrrrrrrrrrrrk"
    ".kmrmrrrrmrrrcck"
    ".kmrrrrrrrrrrrrk"
    "..kmrrmrrrmrrrk."
    "...kmrrrrrrrkkk."
    "..kmrmkkkkkkkkk."
    "..kmrk....kkkkk."
    "...kk......kkk.."
    "................";

/* the stork from above: white wings with black tips, neck and a long red bill */
static const char STORK_R[] =
    "................"
    ".kk............."
    "kdkk............"
    "kddkk..........."
    ".kdwwkk........."
    "..kwwwwkk......."
    "..kwwlwwwkk..kk."
    ".kwwwwlwwwwkkwwk"
    ".kwwwwlwwwwwwwrr"
    ".kwwwwlwwwwkkwwk"
    "..kwwlwwwkk..kk."
    "..kwwwwkk......."
    ".kdwwkk........."
    "kddkk..........."
    "kdkk............"
    ".kk.............";
static const char STORK_R2[] =
    "................"
    "................"
    ".kk............."
    "kdkkk..........."
    "kddwwkk........."
    ".kdwwwwkk......."
    "..kwwlwwwkk..kk."
    ".kwwwwlwwwwkkwwk"
    ".kwwwwlwwwwwwwrr"
    ".kwwwwlwwwwkkwwk"
    "..kwwlwwwkk..kk."
    ".kdwwwwkk......."
    "kddwwkk........."
    "kdkkk..........."
    ".kk............."
    "................";

/* the falcon, stooping: 24 x 16 */
static const char FALCON[] =
    "........................"
    "..kk...................."
    ".kbbk..................."
    ".kbtbk.................."
    "..kbttbk.........kk....."
    "...kbtttbkk....kkcck...."
    "....kbtttttkkkkcccyyk..."
    ".....kbttttttccccckyyk.."
    "....kbtttttbbccckkk.kk.."
    "...kbtttbbkkkkkk........"
    "..kbtbbkk..............."
    ".kbbkk.................."
    "..kk...................."
    "........................"
    "........................"
    "........................";
static const char FALCON2[] =
    "........................"
    "........................"
    "........................"
    "........................"
    "..kkkk...........kk....."
    ".kbtttkkk......kkcck...."
    "..kbttttbkkkkkkcccyyk..."
    "...kbttttttttccccckyyk.."
    "..kbttttttbbbccckkk.kk.."
    ".kbtttbbkkkkkkkk........"
    "..kkkk.................."
    "........................"
    "........................"
    "........................"
    "........................"
    "........................";

/* a prickly pear */
static const char PEAR[] =
    "...zz.z..."
    "....zzi..."
    "...kkkkk.."
    "..kPrPrPk."
    ".kPrKyrPrk"
    ".krPrPrPrk"
    ".kPyrPrKrk"
    ".krPrPyrPk"
    ".kPrPrPrPk"
    "..kPrPrPk."
    "...kkkkk..";

/* a date palm from above */
static const char PALM[] =
    "..k..........k.."
    ".kzk........kzk."
    "..kzk..kk..kzk.."
    "...kzkkzzkkzk..."
    "....kjzizzjk...."
    ".kkkkjjzzjjkkkk."
    "kzzzzjjbbjjzzzzk"
    "kiizzjbttbjzziik"
    ".kkkkjjbbjjkkkk."
    "....kjzzzzjk...."
    "...kzkkzzkkzk..."
    "..kzk..kk..kzk.."
    ".kzk...kk...kzk."
    "..k....kk....k.."
    "................"
    "................";

static const char BOULDER[] =
    "................"
    "....kkkkkk......"
    "..kkllllgggkk..."
    ".kllwlllgggggk.."
    ".klwllllgggggskk"
    "klllllggggggsssk"
    "kllllgggggsgsssk"
    "klllgggggggsssk."
    "kgllgggsgggsssk."
    "kggggggssssssssk"
    ".kgggsgsssssssk."
    ".ksgsssssssssk.."
    "..kksssssssskk.."
    "....kkkkkkkk...."
    "................"
    "................";

/* a thorn bush */
static const char BUSH[] =
    "................"
    "....k.kkk.k....."
    "...kfkjzjkfk.k.."
    "..kfjjzzjjfkfk.."
    ".kfjzjjfjzjjfk.."
    "kfjjrjfjjjzjjfk."
    "kfzjjjfjzjjrjfk."
    ".kfjjzjjjfjjjjfk"
    "kfjjfjjrjjzjjfk."
    "kfjzjjfjjjjfjjfk"
    ".kfjjjzjfjzjjfk."
    "..kfjfjjjjjfjk.."
    "...kkfqfqfqkk..."
    ".....kkkkkk....."
    "................"
    "................";

/* Twig from the side, for the title and the label: 40 x 24 */
static const char HERO[] =
    "........................................"
    "...........................kkkk........."
    "..........................kwwwwk........"
    "..................kkkkkkkkwwlwwwk......."
    "...............kkkwwwwwwwwwwwwwwwk......"
    ".............kkwwwlwwwlwwwwwwkkkkwk....."
    "...........kkwwwwwwwwwwwwwwwwkccccck...."
    "..........kwwwwlwwwlwwwwlwwwkcccckcck..."
    ".........kwwwwwwwwwwwwwwwwwwkccckkcck..."
    "........kwwwlwwwlwwwwlwwwwwwwkccccck...."
    "........kwwwwwwwwwwwwwwwwwwwwwkkkkkwwk.."
    ".......kwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwk."
    ".......kwwwwwwwwwwwwwwwwwwwwwwkkkkkkkk.."
    ".......kwwwlllllllllllllllwwwwwwwwk....."
    "...kkk.kwwllkkkkllllllkkkkkllwwwkkk....."
    "..kwwwkkwllk..klllllk....kllkkkk........"
    ".kwwkwwkklk...klllk.k...kllk............"
    ".kwk.kwwklk...kllkk.....kllk............"
    ".kwk..kwkk...klklkl....klklkl..........."
    ".kwwk.kwk..bbkbkbkbbbbbbkbkbkbbbbbbbbb.."
    "..kwwkkwk.bttttttttttttttttttttttttttb.."
    "...kwwwk..bbbbbbbbbbbbbbbbbbbbbbbbbbbbb."
    "....kkk................................."
    "........................................";

/* the rain and sun pads: a flat stone with its sign */
static const char RAINPAD[] =
    "................"
    "..kkkkkkkkkkkk.."
    ".kssssssssssssk."
    ".ksllkkkkkllssk."
    ".kslkllwllkllsk."
    ".ksklwwwwwlklsk."
    ".kkwwwwwwwwwkksk"
    ".kllwllllwwllksk"
    ".kkkkkkkkkkkksk."
    ".ksuslsuslsuslk."
    ".ksusssussusssk."
    ".kssussussussk.."
    ".ksssssssssssk.."
    "..kkkkkkkkkkk..."
    "................"
    "................";
static const char SUNPAD[] =
    "................"
    "..kkkkkkkkkkkk.."
    ".ksssssyssssssk."
    ".kssyssysssyssk."
    ".ksssykkkkyssssk"
    ".kssskyyyyksssk."
    ".ksyykyaayykyysk"
    ".kssskyaaayksssk"
    ".kssskyyyyksssk."
    ".ksssykkkkyssssk"
    ".kssyssysssyssk."
    ".ksssssyssssssk."
    ".kssssssssssssk."
    "..kkkkkkkkkkkk.."
    "................"
    "................";

/* rotate a square pixel string: dir 0 = the same, 1 = head up, 2 = head down */
static void make_rot(Sprite *s, const char *src, int n, int rot) {
    char buf[16 * 16 + 1];
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            int sx = x, sy = y;
            if (rot == 1) { sx = n - 1 - y; sy = x; }       /* counter-clockwise: right -> up */
            else if (rot == 2) { sx = y; sy = n - 1 - x; }  /* clockwise: right -> down */
            buf[y * n + x] = src[sy * n + sx];
        }
    buf[n * n] = 0;
    spr_make(s, n, n, buf);
}

void tn_art_load(void) {
    if (tn_spr[TS_PEAR].px) return;
    make_rot(&tn_spr[TS_TWIG_R], TWIG_R, 16, 0);
    make_rot(&tn_spr[TS_TWIG_R2], TWIG_R2, 16, 0);
    make_rot(&tn_spr[TS_TWIG_U], TWIG_R, 16, 1);
    make_rot(&tn_spr[TS_TWIG_U2], TWIG_R2, 16, 1);
    make_rot(&tn_spr[TS_TWIG_D], TWIG_R, 16, 2);
    make_rot(&tn_spr[TS_TWIG_D2], TWIG_R2, 16, 2);
    make_rot(&tn_spr[TS_BABY_R], BABY_R, 10, 0);
    make_rot(&tn_spr[TS_BABY_U], BABY_R, 10, 1);
    make_rot(&tn_spr[TS_BABY_D], BABY_R, 10, 2);
    make_rot(&tn_spr[TS_TOAD_R], TOAD_R, 16, 0);
    make_rot(&tn_spr[TS_TOAD_U], TOAD_R, 16, 1);
    make_rot(&tn_spr[TS_TOAD_D], TOAD_R, 16, 2);
    make_rot(&tn_spr[TS_TOAD_BLINK], TOAD_BLINK, 16, 0);
    make_rot(&tn_spr[TS_TOAD_BLINK_U], TOAD_BLINK, 16, 1);
    make_rot(&tn_spr[TS_TOAD_BLINK_D], TOAD_BLINK, 16, 2);
    make_rot(&tn_spr[TS_STORK_R], STORK_R, 16, 0);
    make_rot(&tn_spr[TS_STORK_R2], STORK_R2, 16, 0);
    make_rot(&tn_spr[TS_STORK_U], STORK_R, 16, 1);
    make_rot(&tn_spr[TS_STORK_U2], STORK_R2, 16, 1);
    make_rot(&tn_spr[TS_STORK_D], STORK_R, 16, 2);
    make_rot(&tn_spr[TS_STORK_D2], STORK_R2, 16, 2);
    spr_make(&tn_spr[TS_FALCON], 24, 16, FALCON);
    spr_make(&tn_spr[TS_FALCON2], 24, 16, FALCON2);
    spr_make(&tn_spr[TS_PEAR], 10, 11, PEAR);
    spr_make(&tn_spr[TS_HERO], 40, 24, HERO);
    spr_make(&tn_spr[TS_PALM], 16, 16, PALM);
    spr_make(&tn_spr[TS_BOULDER], 16, 16, BOULDER);
    spr_make(&tn_spr[TS_BUSH], 16, 16, BUSH);
    spr_make(&tn_spr[TS_RAIN], 16, 16, RAINPAD);
    spr_make(&tn_spr[TS_SUN], 16, 16, SUNPAD);
}
