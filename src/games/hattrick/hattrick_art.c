/* HAT TRICK - pixel art (palette-letter strings, see gfx.h). All drawn for
 * UFO 40. The kids are one set of frames in placeholder colours (h/e skin,
 * b hair, j/f shirt, w/l shorts), recoloured for Teddy and Mae. The tiles,
 * backdrops, the chair and the bosses' extras are drawn in hattrick_draw.c. */
#include "hattrick.h"

Sprite htk_spr[SP_COUNT];
const char *const HTK_KID_NAME[2] = {"TEDDY", "MAE"};

/* ---- the kids, 14 x 20, facing right, feet on the bottom row ------------------ */
static const char KID_STAND[] =
    ".....kkkkk...."
    "....kbbbbbk..."
    "...kbbbbbbbk.."
    "...kbbbhhhhk.."
    "...kbbhhkhhk.."
    "...kbhhhkhhk.."
    "....khhhhhek.."
    ".....keeekk..."
    "....kjjjjjjk.."
    "...kjjwwjjjjk."
    "...kjjjjjjfjk."
    "...khkjjjjkhk."
    "....k.kffk.k.."
    "....kwwwwwk..."
    "....kwwlwwk..."
    "....kwk.kwk..."
    "....khk.khk..."
    "....kwk.kwk..."
    "...kkkk.kkkk.."
    "..kkkkk.kkkkk.";
static const char KID_RUN1[] =
    ".....kkkkk...."
    "....kbbbbbk..."
    "...kbbbbbbbk.."
    "...kbbbhhhhk.."
    "...kbbhhkhhk.."
    "...kbhhhkhhk.."
    "....khhhhhek.."
    ".....keeekk..."
    "...kkjjjjjjk.."
    "..khjjwwjjjjk."
    "...kkjjjjjfjkk"
    "....kjjjjjjkhk"
    "....kkfffkk.k."
    "....kwwwwwk..."
    "...kwwwlwwwk.."
    "..kwk....kwk.."
    ".khk......khk."
    ".kwk......kwkk"
    "kkk........kkk"
    "kk............";
static const char KID_RUN2[] =
    ".....kkkkk...."
    "....kbbbbbk..."
    "...kbbbbbbbk.."
    "...kbbbhhhhk.."
    "...kbbhhkhhk.."
    "...kbhhhkhhk.."
    "....khhhhhek.."
    ".....keeekk..."
    "....kjjjjjjk.."
    "...kjjwwjjjjk."
    "...kjjjjjjfjk."
    "...khkjjjjkhk."
    "....k.kffk.k.."
    "....kwwwwwk..."
    "....kwwlwwk..."
    ".....kwkwk...."
    ".....khkhk...."
    ".....kwkwk...."
    "....kkkkkkk..."
    "....kkkkkkkk..";
static const char KID_JUMP[] =
    ".....kkkkk...."
    "....kbbbbbk..."
    "...kbbbbbbbk.."
    "...kbbbhhhhk.."
    "..kkbbhhkhhk.."
    ".khkbhhhkhhkk."
    ".khkkhhhhhekhk"
    "..kk.keeekk.kk"
    "....kjjjjjjk.."
    "...kjjwwjjjjk."
    "...kjjjjjjfjk."
    "...kkjjjjjjkk."
    "....kkfffkk..."
    "....kwwwwwk..."
    "...kwwwlwwwk.."
    "...kwkk.kkwk.."
    "..khkk...khk.."
    "..kwk....kwk.."
    "..kkk....kkkk."
    "..kk.....kkk..";
static const char KID_KICK[] =
    ".....kkkkk...."
    "....kbbbbbk..."
    "...kbbbbbbbk.."
    "...kbbbhhhhk.."
    "...kbbhhkhhk.."
    "...kbhhhkhhk.."
    "....khhhhhek.."
    ".....keeekk..."
    "..kkkjjjjjjk.."
    ".khkjjwwjjjjk."
    "..kkjjjjjjfjk."
    "....kjjjjjkhk."
    "....kkfffkkk.."
    "....kwwwwwkkk."
    "....kwwlwwwwkk"
    "....kwk..kwwhk"
    "....khk...kkwk"
    "....kwk....kkk"
    "...kkkk......."
    "..kkkkk.......";
static const char KID_SLIDE[] =
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    "...kkkkk......"
    "..kbbbbbk....."
    ".kbbbhhhhk...."
    ".kbbhhkhhk...."
    ".kkhhhhhek...."
    "kjjkkeekjjjjkk"
    "kjjwwjjjjfkwwk"
    ".kkjjjjfkwwlwk"
    "..kkkkkkkkkhkk"
    "...........kkk";
static const char KID_HEAD[] =
    "....kkkkkk...."
    "...kbbbbbbk..."
    "..kbbbhhhhhk.."
    "..kbbhhhkhhk.."
    "..kbhhhhkhhk.."
    "..kbhhhhhhek.."
    "...khhhhkkk..."
    "kk..kkeek..kk."
    "khk.kjjjjk.khk"
    ".khkjjwwjjkhk."
    "..kkjjjjjjkk.."
    "...kjjjjjfjk.."
    "....kkfffkk..."
    "....kwwwwwk..."
    "....kwwlwwk..."
    "....kwk.kwk..."
    "....khk.khk..."
    "....kwk.kwk..."
    "...kkkk.kkkk.."
    "..kkkkk.kkkkk.";
static const char KID_CROUCH[] =
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".....kkkkk...."
    "....kbbbbbk..."
    "...kbbbhhhhk.."
    "...kbbhhkhhk.."
    "...kbhhhhhek.."
    "...kkjkeekjkk."
    "..khjjwwjjjjhk"
    "...kjjjjjjfjk."
    "...kwwwwwwwwk."
    "..kkkkk.kkkkk."
    "..kkkkk.kkkkk.";
static const char KID_DOWN[] =
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    ".............."
    "........kkkkk."
    ".......kbbbbbk"
    "kk.kkkkkhkhkbk"
    "kwkkjjjjhhhhbk"
    "kwwwwjjjhkhkbk"
    "khkkjjjjkeeek."
    "kkk.kkkkkkkk.."
    "..............";

/* ---- the creatures, 16 x 16 unless noted, facing right -------------------------- */
static const char SPIKER[] =
    "....kkkkkkk....."
    "...kwwyywwwk...."
    "..kwwwyyywwwk..."
    ".kyywwwyywwyyk.."
    ".kwyyywwwyyywk.."
    "kwwwkkwwwkkwwwk."
    "kwwkwkwwkwkwwwk."
    "kywwkwyywkwwyyk."
    "kyywwwyyywwwyyk."
    ".kwwwkkkkkwwwk.."
    ".kwywkrrrkwywk.."
    "..kwwwkkkwwwk..."
    "...kkkkkkkkk...."
    "....kok..kok...."
    "...kook..kook..."
    "...kkkk..kkkk...";
static const char FRISBEE[] =
    "................"
    "................"
    "................"
    "................"
    ".....kkkkkk....."
    "...kkrrrrrrkk..."
    "..krrrooooorrk.."
    ".krrokwrrkwrrrk."
    ".krrokkrrkkoorrk"
    "krrrrrrrrrrrrrk."
    ".kvvrrrrrrrrvvk."
    "..kkvvvvvvvvkk.."
    "....kkkkkkkk...."
    "................"
    "................"
    "................";
static const char BEACHBALL[] =
    "................"
    ".....kkkkkk....."
    "...kkrrwwBBkk..."
    "..krrrrwwBBBBk.."
    ".krrrrwwwwBBBBk."
    ".krrkwrwwBkwBBk."
    "kyrrkkrwwBkkBByk"
    "kyyrrrwwwwBBByyk"
    "kyyyrwwwwwwByyyk"
    "kyyyykkkkkkyyyyk"
    ".kyyykrrrrkyyyk."
    ".kwyyykkkkyyywk."
    "..kwwyyyyyyywk.."
    "...kkwwwwwwkk..."
    ".....kkkkkk....."
    "................";
static const char PIN[] =
    "......kkkk......"
    ".....kwwwwk....."
    ".....kwkwkk....."
    ".....kwwwwk....."
    "......krrk......"
    "......kwwk......"
    ".....kwwwwk....."
    "....kwwwwwlk...."
    "....kwrrrrlk...."
    "....kwwwwwlk...."
    "....kwwwwwlk...."
    "....kwwwwwlk...."
    ".....kwwwlk....."
    ".....kkkkkk....."
    "....kok..kok...."
    "....kkk..kkk....";
static const char BOWLER[] =
    "................"
    ".....kkkkkk....."
    "....kbbbbbbk...."
    "....kbbbbbbk...."
    "..kkkkkkkkkkkk.."
    "...kVVVVVVVVk..."
    "..kVVkwVVkwVVk.."
    "..kVVkkVVkkVVk.."
    ".kVVVVVVVVVVVVk."
    ".kVVVkkkkkkVVVk."
    ".kVpVVVVVVVVpVk."
    "..kVppVVVVppVk.."
    "...kVVppppVVk..."
    "....kkkkkkkk...."
    "....kak..kak...."
    "...kkkk..kkkk...";
static const char SPINNER[] =
    "................"
    ".....kkkkkk....."
    "...kkddddddkk..."
    "..kdddddddsddk.."
    ".kdddsdddddsddk."
    ".kddkkddddkkddk."
    "kdddkwkddkwkdddk"
    "kddddkddddkddddk"
    "kddddddddddddddk"
    "kdddddkkkkdddddk"
    "kddddkrrrrkddddk"
    ".kddddkkkkddddk."
    ".kndddddddddddk."
    "..knnddddddnnk.."
    "...kknnnnnnkk..."
    ".....kkkkkk.....";
static const char BUOY[] =
    "................"
    ".......kk......."
    "......kwwk......"
    "......kwwk......"
    "....kkkkkkkk...."
    "...krrrrrrrrk..."
    "..kwwwwwwwwwwk.."
    "..kwwkwwwwkwwk.."
    "..krrkkrrkkrrk.."
    "..krrrrrrrrrrk.."
    "..kwwwkkkkwwwk.."
    "..kwwwwwwwwwwk.."
    "...krrrrrrrrk..."
    "..kCCkkkkkkCCk.."
    ".kCuCCCCCCCCuCk."
    "..kkkkkkkkkkkk..";
static const char POLO[] =
    "................"
    "....kkkkkkk....."
    "...kBBBBBBBk...."
    "..kBBwBBBBBBk..."
    "..kBkkhhhhkBk..."
    "..kBkhhkhhkhk..."
    "..kBkhhkhhkhk..."
    "..kwkhhhhhekk..."
    "...kkkeeeekk...."
    "....kBBBBBBkk..."
    "...khBBwwBBkhk.."
    "..kCCkBBBBkCCk.."
    ".kCuCCCCCCCCuCk."
    "kCuuCuCCCuCCuuCk"
    ".kCCCCuCCCCuCCk."
    "..kkkkkkkkkkkk..";
static const char DUCKY[] =
    "................"
    "................"
    "................"
    "................"
    "........kkkk...."
    ".......kyyyyk..."
    "......kyyykyyk.."
    "......kyyyyyykkk"
    "..k...kyyyyykaak"
    ".kyk...kyyyyykkk"
    ".kyykkkyyyyyyk.."
    ".kyyyyyyyyyayyk."
    ".kyyyyyyyyaayyk."
    "..kayyyyyyyyyk.."
    "...kkaaaaaaakk.."
    ".....kkkkkkk....";
static const char PROP[] =
    "....kkkkkkk....."
    "...kvvvvvvvk...."
    "..kvvvvvvvvvk..."
    "..kvhhhhhhhvk..."
    "..kvhkhhhkhhk..."
    "..kvhkhhhkhhk..."
    "..kkhhhhhhhek..."
    ".kkkkeeeeekkkk.."
    "kvvvvwvvvvvvvvk."
    "kvhvvwwvvvvvhvk."
    "kvhkvvwvvvvkhvk."
    ".kk.kvvvvvvk.kk."
    "....kwwwwwwk...."
    "....kwwkkwwk...."
    "...kkwk..kwkk..."
    "...kkkk..kkkk...";
static const char GLOVE[] =
    "................"
    "....kkkkkk......"
    "...krrrrrrkk...."
    "..krrrrrrrrrk..."
    ".krrrrrrrrrrrk.."
    ".krrrrwrrkrrrrk."
    "kkkrrrrrrkrrrrk."
    "krrkrrrrrrrrrrk."
    "krrrkrrrrrrrrvk."
    ".krrrrrrrrrrvk.."
    "..kvrrrrrrrvk..."
    "...kkvvvvvkk...."
    "...kwwwwwwk....."
    "...kwlwlwlk....."
    "...kkkkkkkk....."
    "................";
static const char BULLDOG[] =
    "................"
    "..kk.......kk..."
    ".kbtk.....ktbk.."
    ".kttkkkkkkkttk.."
    "..kttttttttttk.."
    ".ktttttttttttk.."
    ".ktthhttthhttk.."
    "kttkkktttkkkttk."
    "ktttttkkktttttk."
    "kthhttkkkttthhk."
    "khhhhkhhhkhhhhk."
    "khwkhhhhhhhkwhk."
    ".khwwwwwwwwwhk.."
    "..kkhhhhhhhkk..."
    "...kkkkkkkkk...."
    "................";
static const char TIMEKEEPER[] =
    "....kkkkkk......"
    "...kIIIIIIk....."
    "..kIIIIIIIIk...."
    "..kIIkIIkIIk...."
    "..kIIkIIkIIk...."
    "..kIIIIIIIIk...."
    "..kIIIkkIIIk..kk"
    "..kkkkkkkkkk.kwk"
    ".klklklklklk.kwk"
    ".kllklklklkkkkk."
    ".klklklklklk...."
    ".kllklklklk....."
    "..kIIIIIIIk....."
    "..kIIkIkIIk....."
    "..kIk.k.kIk....."
    "...k.....k......";

/* ---- the food, 8 x 8 ------------------------------------------------------------ */
static const char POPCORN[] =
    ".kckck.."
    "kcycyck."
    "kcycccyk"
    "krwrwrwk"
    "krwrwrwk"
    ".krwrwk."
    ".krwrwk."
    "..kkkk..";
static const char PRETZEL[] =
    "..kkkk.."
    ".kttttk."
    "ktkttktk"
    "ktkttktk"
    ".ktkktk."
    "kttkkttk"
    "ktk..ktk"
    ".kk..kk.";
static const char TACO[] =
    "........"
    "..kkkk.."
    ".kzrzrk."
    "kzrzrzrk"
    "kaaaaaak"
    "kyyyyyyk"
    ".kyyyyk."
    "..kkkk..";
static const char DRUMSTICK[] =
    "..kkk..."
    ".keeek.."
    "keettek."
    "keetttk."
    ".kettk.."
    "..kkwk.."
    "...kwwk."
    "...kkkk.";
static const char LOLLY[] =
    ".kkkk..."
    "kPKPKk.."
    "kKPKPk.."
    "kPKPKk.."
    ".kkkk..."
    "...kwk.."
    "....kwk."
    ".....kk.";
static const char COOKIE[] =
    "..kkkk.."
    ".keeeek."
    "kebeeeek"
    "keeeebek"
    "kebeeeek"
    "keeebeek"
    ".keeeek."
    "..kkkk..";
static const char DONUT[] =
    "..kkkk.."
    ".kKKKKk."
    "kKyKKuKk"
    "kKKkkKKk"
    "kKKkkKuk"
    "keKyKKek"
    ".keeeek."
    "..kkkk..";
static const char PARFAIT[] =
    ".kk.rk.."
    "kwwkkwk."
    "kccccck."
    "kKKKKKk."
    "kccccck."
    ".kVVVk.."
    "..kwk..."
    ".kkkkk..";

/* ---- the ball (lit and dark), 7 x 7, and a balloon ------------------------------- */
static const char BALL[] =
    "..kkk.."
    ".kwwwk."
    "kwkwwwk"
    "kwwwkwk"
    "kwkwwwk"
    ".kwwwk."
    "..kkk..";
static const char BALL_DARK[] =
    "..kkk.."
    ".ksssk."
    "kskssdk"
    "ksssksk"
    "kskssdk"
    ".kssdk."
    "..kkk..";
static const char BALLOON[] =
    ".kkkkk."
    "krrrrrk"
    "krwrrrk"
    "krrrrrk"
    "krrrrvk"
    ".kvvvk."
    "..kkk.."
    "...l..."
    "...l...";

void htk_kid_remap(int ch, uint8_t *map) {
    pal_identity(map);
    if (ch == KID_TEDDY) {
        /* Teddy: freckles, a ginger mop, green and white hoops */
        pal_swap(map, C_BROWN, C_ORANGE);
    } else {
        /* Mae: dark skin, black braids, a yellow shirt, navy shorts */
        pal_swap(map, C_HIDE, C_TAN);
        pal_swap(map, C_EARTH, C_BROWN);
        pal_swap(map, C_BROWN, C_INK);
        pal_swap(map, C_JADE, C_YELLOW);
        pal_swap(map, C_FOREST, C_AMBER);
        pal_swap(map, C_WHITE, C_NAVY);
        pal_swap(map, C_LIGHT, C_NIGHT);
    }
}

void htk_art_load(void) {
    static bool done;
    if (done) return;
    done = true;
    const char *kids[] = {KID_STAND, KID_RUN1, KID_RUN2, KID_JUMP, KID_KICK, KID_SLIDE, KID_HEAD, KID_CROUCH, KID_DOWN};
    for (int i = 0; i < ARRAY_LEN(kids); i++) spr_make(&htk_spr[SP_KID_STAND + i], 14, 20, kids[i]);
    const char *foes[] = {SPIKER, FRISBEE, BEACHBALL, PIN, BOWLER, SPINNER, BUOY, POLO, DUCKY, PROP, GLOVE, BULLDOG, TIMEKEEPER};
    for (int i = 0; i < ARRAY_LEN(foes); i++) spr_make(&htk_spr[SP_SPIKER + i], 16, 16, foes[i]);
    const char *items[] = {POPCORN, PRETZEL, TACO, DRUMSTICK, LOLLY, COOKIE, DONUT, PARFAIT};
    for (int i = 0; i < ARRAY_LEN(items); i++) spr_make(&htk_spr[SP_POPCORN + i], 8, 8, items[i]);
    spr_make(&htk_spr[SP_BALL], 7, 7, BALL);
    spr_make(&htk_spr[SP_BALL_DARK], 7, 7, BALL_DARK);
    spr_make(&htk_spr[SP_BALLOON], 7, 9, BALLOON);
}
