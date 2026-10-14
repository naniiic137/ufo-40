/* ECHO UNIT - pixel art (palette-letter strings, see gfx.h), all drawn for
 * UFO 40: the four bats of Echo Unit and Colonel Hush, V.I.P.E.R.'s lizard
 * troops, Scrapjaw, Dr. Talpa, Ramrod and Vultra, Hog Wild, Cipher and
 * Madame Coil. Frames are put together from shared blocks of rows (a body
 * and its legs, say); vehicles, scenery and the gunship are drawn in
 * eko_draw.c. */
#include "eko.h"

Sprite eko_spr[SP_COUNT];
Sprite eko_spr_half[SP_COUNT], eko_spr_3q[SP_COUNT];

/* ---- FLINT, 16 x 24, facing right: a big brown bat in a red bandana ---- */
static const char *const FL_TOP[16] = {
    "..k......k......",
    ".kbk....kbk.....",
    ".kbhk..khbk.....",
    ".kbhbkkbhbk.....",
    "..krrrrrrrkr....",
    "..kbtttttwk.r...",
    "..kbttkttkk.....",
    "...kttttttk.....",
    "....kktttk......",
    "...kbbbbbbk.....",
    "..kebbhhbbbk....",
    ".kebbhhhhbbtk...",
    ".kebbhhhhbtk....",
    "..kebbhhbbk.....",
    "...kbbbbbbk.....",
    "...kbbbbbbk.....",
};
static const char *const FL_TOP_PUNCH[16] = {
    "..k......k......",
    ".kbk....kbk.....",
    ".kbhk..khbk.....",
    ".kbhbkkbhbk.....",
    "..krrrrrrrkr....",
    "..kbtttttwk.r...",
    "..kbttkttkk.....",
    "...kttttttk.....",
    "....kktttk......",
    "...kbbbbbbkkkk..",
    "..kebbhhbbbbbtk.",
    ".kebbhhhhbbbbttk",
    ".kebbhhhhbkkkkk.",
    "..kebbhhbbk.....",
    "...kbbbbbbk.....",
    "...kbbbbbbk.....",
};
static const char *const FL_LEGS[5][8] = {
    {"...kbbkkbbk.....", "...kbk..kbk.....", "...kbk..kbk.....", "...kbk..kbk.....",
     "...kbk..kbk.....", "..kkbk..kbkk....", "..kkkk..kkkk....", "................"},
    {"...kbbkkbbk.....", "...kbk...kbk....", "..kbk.....kbk...", "..kbk.....kbk...",
     ".kbk.......kbk..", "kkbk.......kbkk.", "kkkk.......kkkk.", "................"},
    {"...kbbkkbbk.....", "....kbkkbk......", "....kbkbk.......", "....kbkbk.......",
     "....kbkbk.......", "...kkbkbkk......", "...kkkkkkk......", "................"},
    /* the kick: the front leg out */
    {"...kbbbbbbkkkk..", "...kbk..kbbbbbk.", "...kbk...kkkkkk.", "...kbk..........",
     "...kbk..........", "..kkbk..........", "..kkkk..........", "................"},
    /* in the air: knees up */
    {"...kbbkkbbk.....", "...kbbkkbbk.....", "....kbk.kbk.....", "....kkk.kkk.....",
     "................", "................", "................", "................"},
};

/* ---- V.I.P.E.R. lizard trooper, 16 x 24, facing right ---- */
static const char *const TR_TOP[16] = {
    "................",
    "....kkkkk.......",
    "...kNNNNNk......",
    "..kNNNNNNNk.....",
    "..kgggggggkk....",
    "..kzzzzzzwkzk...",
    "..kzzzzzzkkzzk..",
    "...kzzzzzzzzzk..",
    "....kzzziiiik...",
    "...kNNNNNNk.....",
    "..kNNNgNNNNk....",
    ".kzNNNgNNNzzk...",
    ".kzNNNgNNNzk....",
    "..kNNNgNNNk.....",
    "...kssssssk.....",
    "...kNNNNNNk.....",
};
static const char *const TR_TOP_AIM[16] = {
    "................",
    "....kkkkk.......",
    "...kNNNNNk......",
    "..kNNNNNNNk.....",
    "..kgggggggkk....",
    "..kzzzzzzwkzk...",
    "..kzzzzzzkkzzk..",
    "...kzzzzzzzzzk..",
    "....kzzziiiik...",
    "...kNNNNNNkkkkkk",
    "..kNNNgNNzzsssss",
    ".kzNNNgNNzkkkkkk",
    ".kzNNNgNNNk.....",
    "..kNNNgNNNk.....",
    "...kssssssk.....",
    "...kNNNNNNk.....",
};
static const char *const TR_LEGS[2][8] = {
    {"...kNNkkNNk.....", "...kNk..kNk.....", "...kNk..kNk.....", "...kzk..kzk.....",
     "...kzk..kzk.....", "..kkzk..kzkk....", "..kkkk..kkkk....", "................"},
    {"...kNNkkNNk.....", "...kNk...kNk....", "..kNk.....kNk...", "..kzk.....kzk...",
     ".kzk.......kzk..", "kkzk.......kzkk.", "kkkk.......kkkk.", "................"},
};

/* ---- VESPER from behind, 16 x 24: a grey bat with goggles and a long rifle ---- */
static const char *const VE_TOP[14] = {
    "...k.......k....",
    "..kgk.....kgk...",
    "..kglk...klgk...",
    "..kgllkkkllgk...",
    "...kgggggggk....",
    "...kCCkkkCCk....",
    "...kgggggggk....",
    "....kgggggk.....",
    "..kkksssssskk...",
    ".kdgsssssssgdk..",
    "kddgsssssssgddk.",
    "kdd.ksssssk.ddk.",
    ".k..ksssssk..k..",
    "....ksssssk.....",
};
static const char *const VE_LEGS[3][10] = {
    {"....kssksssk....", "....ksk.ksk.....", "....ksk.ksk.....", "....ksk.ksk.....", "....ksk.ksk.....",
     "...kksk.kskk....", "...kkkk.kkkk....", "................", "................", "................"},
    {"....kssksssk....", "...ksk...ksk....", "...ksk...ksk....", "..ksk.....ksk...", "..ksk.....ksk...",
     ".kksk.....kskk..", ".kkkk.....kkkk..", "................", "................", "................"},
    {"....kssksssk....", "....ksk.ksk.....", "....ksk.ksk.....", "....ksk.ksk.....", "....ksk.ksk.....",
     "...kksk.kskk....", "...kkkk.kkkk....", "................", "................", "................"},
};
/* the rifle raised to the shoulder */
static const char *const VE_FIRE_TOP[14] = {
    "...k.......k....",
    "..kgk.....kgk...",
    "..kglk...klgk...",
    "..kgllkkkllgk...",
    "...kgggggggk....",
    "...kCCkkkCCk....",
    "...kgggggggkk...",
    "....kgggggkak...",
    "..kkksssssska...",
    ".kdgsssssssak...",
    "kddgssssssskk...",
    "kdd.ksssssk.....",
    ".k..ksssssk.....",
    "....ksssssk.....",
};
static const char VE_ROLL[] =
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "......kkkk......"
    "....kkggggkk...."
    "...kgggggggggk.."
    "..kgdgggCCgggk.."
    "..kgddgggggsgk.."
    ".kgdddgggsssgk.."
    ".kgdddggsssggk.."
    ".kggddgsssgggk.."
    "..kggggssggggk.."
    "..kgggggggggk..."
    "...kkgggggkk...."
    ".....kkkkk......"
    "................"
    "................"
    "................";

/* ---- the gallery's troopers face her: 16 x 24 ---- */
static const char *const GT_TOP[16] = {
    "................",
    ".....kkkkkk.....",
    "....kNNNNNNk....",
    "...kNNNNNNNNk...",
    "...kggggggggk...",
    "...kzwkzzkwzk...",
    "...kzzzzzzzzk...",
    "....kziiiizk....",
    ".....kzzzzk.....",
    "...kkNNNNNNkk...",
    "..kzNNNggNNNzk..",
    "..kzNNNggNNNzk..",
    "..kzkNNggNNkzk..",
    "..kk.kssssk.kk..",
    ".....kNNNNk.....",
    ".....kNNNNk.....",
};
static const char *const GT_TOP_AIM[16] = {
    "................",
    ".....kkkkkk.....",
    "....kNNNNNNk....",
    "...kNNNNNNNNk...",
    "...kggggggggk...",
    "...kzwkzzkwzk...",
    "...kzzzzzzzzk...",
    "....kziiiizk....",
    ".....kzzzzk.....",
    "...kkNNskNNkk...",
    "..kzNNNsNNNNzk..",
    "..kzNNskkNNNzk..",
    "..kzkzzrrzzkzk..",
    "..kk.kssssk.kk..",
    ".....kNNNNk.....",
    ".....kNNNNk.....",
};
static const char *const GT_LEGS[2][8] = {
    {".....kNkkNk.....", ".....kNkkNk.....", ".....kNkkNk.....", ".....kzkkzk.....",
     ".....kzkkzk.....", "....kkzkkzkk....", "....kkkkkkkk....", "................"},
    {"....kNk..kNk....", "...kNk....kNk...", "...kNk....kNk...", "..kzk......kzk..",
     "..kzk......kzk..", ".kkzk......kzkk.", ".kkkk......kkkk.", "................"},
};

/* ---- DART on the hover-bike, 24 x 16, riding right ---- */
static const char BIKE1[] =
    "......k.....k..........."
    ".....kok...kok.........."
    ".....kaok.koak.........."
    "......kooooook.........."
    "......kooBBwok.kk......."
    ".......kooooookk........"
    "......kkkoook.kk........"
    ".....krrrkkk.kssk......."
    "...kkrrrrrrrrrssskk....."
    "..krrrrrrrrrrrrrrrrk...."
    ".krrgggggggggggrrrrrkk.."
    ".kkkkkkkkkkkkkkkkkkkkCk."
    "..kCCk..........kCCk...."
    "...kk.....u......kk....."
    "........u...u..........."
    "........................";
static const char BIKE2[] =
    "......k.....k..........."
    ".....kok...kok.........."
    ".....kaok.koak.........."
    "......kooooook.........."
    "......kooBBwok.kk......."
    ".......kooooookk........"
    "......kkkoook.kk........"
    ".....krrrkkk.kssk......."
    "...kkrrrrrrrrrssskk....."
    "..krrrrrrrrrrrrrrrrk...."
    ".krrgggggggggggrrrrrkk.."
    ".kkkkkkkkkkkkkkkkkkkkCk."
    "..kCCk..........kCCk...."
    "...kk.....I......kk....."
    ".......u....u..........."
    "........................";

/* ---- SABLE from above, 16 x 16, facing right: black bat, purple scarf ---- */
static const char SABLE_R[] =
    "................"
    "....kk...kk....."
    "...kdnk.kdnk...."
    "....kkkkkkk....."
    "..kknnnnnnnkk..."
    ".kddnnnnnnwkdk.."
    ".kdnnppppnnkdk.."
    ".kdnnppppnnnkk.."
    ".kdnnppppnnnkk.."
    ".kdnnppppnnkdk.."
    ".kddnnnnnnwkdk.."
    "..kknnnnnnnkk..."
    "....kkkkkkk....."
    "...kdnk.kdnk...."
    "....kk...kk....."
    "................";
static const char SABLE_R2[] =
    "................"
    "....kk...kk....."
    "...kdnk.kdnk...."
    "....kkkkkkk....."
    "..kknnnnnnnkk..."
    "kddnnnnnnnwkdk.."
    "kdnnnppppnnkdk.."
    ".kdnnppppnnnkk.."
    ".kdnnppppnnnkk.."
    "kdnnnppppnnkdk.."
    "kddnnnnnnnwkdk.."
    "..kknnnnnnnkk..."
    "....kkkkkkk....."
    "...kdnk.kdnk...."
    "....kk...kk....."
    "................";
/* a lizard guard from above, facing right */
static const char GUARD_R[] =
    "................"
    "................"
    "....kkkkk......."
    "...kNNNNNk......"
    "..kzNNNNNNkk...."
    ".kzzNNggNNzzk..."
    ".kzNNNggNNNwzk.."
    ".kzNNNggNNNkzkss"
    ".kzNNNggNNNkzkss"
    ".kzNNNggNNNwzk.."
    ".kzzNNggNNzzk..."
    "..kzNNNNNNkk...."
    "...kNNNNNk......"
    "....kkkkk......."
    "................"
    "................";
static const char GUARD_R2[] =
    "................"
    "................"
    "....kkkkk......."
    "...kNNNNNk......"
    "..kzNNNNNNkk...."
    "kzzzNNggNNzzk..."
    ".kzNNNggNNNwzk.."
    ".kzNNNggNNNkzkss"
    ".kzNNNggNNNkzkss"
    ".kzNNNggNNNwzk.."
    "kzzzNNggNNzzk..."
    "..kzNNNNNNkk...."
    "...kNNNNNk......"
    "....kkkkk......."
    "................"
    "................";

/* ---- bosses, 32 x 32 ---- */
static const char SCRAPJAW[] =
    "................................"
    "................................"
    "..........kkkkkkkkk............."
    "........kkfffffffffkk..........."
    ".......kffffffffffwkfkkkkkkkk..."
    "......kfffffffffffkkffffffffffk."
    "......kfzfzfzfffffffffffffffffk."
    "......kffffffffffffffwkwkwkwkwk."
    ".......kffffffffffffkkwkwkwkwk.."
    "........kkfffffffffffffffffffk.."
    "..........kkkkfffffffffffkkkk..."
    ".........kaaakkkkkkkkkkk........"
    "........kaaaaaaakgggggk........."
    ".......kfaaaaaaakgsssgk........."
    "......kffaaaaaaakgggggkkkk......"
    ".....kfffaaaaaaakkkkkkksssk....."
    ".....kffkaaaaaaaaaakffk.ssk....."
    "....kffk.kaaaaaaaaakffk..sk....."
    "....kfk..kaaaaaaaaakfk...sk....."
    "....kk...kaaaaaaaaakk....sk....."
    ".........kffffffffffk...ksk....."
    ".........kffffkkffffk..kssk....."
    ".........kfffk.kfffk..kssk......"
    "........kfffk...kfffk.kk........"
    "........kfffk...kfffk..........."
    "........kfffk...kfffk..........."
    ".......kffffk...kffffk.........."
    ".......kkkkkk...kkkkkk.........."
    "................................"
    "................................"
    "................................"
    "................................";
static const char TALPA[] =
    "................................"
    "................................"
    "............kkkkkkk............."
    "..........kkIIIIIIIkk..........."
    ".........kIIuuuuuuuIIk.........."
    "........kIuuukkkkuuuuIk........."
    "........kIukhhhhhhkuuIk........."
    "........kIkhhwkhwkhkuIk........."
    "........kIkhhhhhhhhkuIk........."
    "........kIukhhKKhhkuuIk........."
    ".........kIuukkkkkuuIk.........."
    "..........kkkkkkkkkkk..........."
    "........kkgggggggggggkk........."
    ".......kgggsssssssssgggk........"
    "......kgggsrrsssrrssggggk......."
    "......kggssssssssssssgggkkkkk..."
    "......kggsssssssssssssggkllllk.."
    "......kggggggggggggggggkllllllk."
    ".......kkggggggggggggkkllllllk.."
    ".........kgggkkkkgggk..kllllk..."
    ".........kgggk..kgggk...kkkk...."
    "........kggggk..kggggk.........."
    "........kssssk..kssssk.........."
    ".......kssssssk.kssssssk........"
    ".......kkkkkkk..kkkkkkkk........"
    "................................"
    "................................"
    "................................"
    "................................"
    "................................"
    "................................"
    "................................";
static const char RAMROD[] =
    "................................"
    "..........kkk......kkk.........."
    ".........kcckk....kkcck........."
    "........kcck.kkkkkk.kcck........"
    "........kck.kccccccck.kck......."
    ".........k.kccccccccck.k........"
    "...........kckwkkwkcck.........."
    "...........kccccccccck.........."
    "............kctttttck..........."
    "............kckktkkck..........."
    "..........kkkkccccckkkk........."
    ".........kssssNNNNNNssssk......."
    "........kccsNNNNNNNNNsscck......"
    ".......kcccNNNNggNNNNNscccck...."
    ".......kcckNNNNggNNNNNkksssssk.."
    ".......kcckNNNNggNNNNNkssssssssk"
    ".......kcckNNNNggNNNNNkksssssk.."
    "........kkkNNNNNNNNNNNk.kkkkk..."
    "..........kssssssssssk.........."
    "..........kNNNNNNNNNNk.........."
    "..........kNNNNkkNNNNk.........."
    "..........kNNNk..kNNNk.........."
    "..........kcck....kcck.........."
    "..........kcck....kcck.........."
    ".........kkcck....kcckk........."
    ".........kkkkk....kkkkk........."
    "................................"
    "................................"
    "................................"
    "................................"
    "................................"
    "................................";
static const char VULTRA[] =
    "................................"
    "................................"
    "..............kkkk.............."
    ".............kwwwwk............."
    "............kwwkwwwk............"
    "............kwwwwwaak..........."
    ".............kwwwwk............."
    "kk..........kkeeeekk..........kk"
    "keekk.....kkeeeeeeeekk.....kkeek"
    ".keeeekkkkeeeeeeeeeeeekkkkeeeek."
    "..keeeeeeeeeeesssseeeeeeeeeeek.."
    "...kkeeeeeeeessssssseeeeeeekk..."
    ".....kkkeeeeeesrrsseeeeeekk....."
    "........kkkkeesssseekkkkk......."
    "............keeeeeek............"
    "............kaak.kaak..........."
    "............kak...kak..........."
    "................................"
    "................................"
    "................................"
    "................................"
    "................................"
    "................................"
    "................................";
static const char HOG[] =
    "....k......k................"
    "...khk....khk..............."
    "...khkkkkkkhk..............."
    "..khhhhhhhhhhk.............."
    ".khhhwkhhhhhhhk............."
    ".khhhkkhhhhhhhhkk..........."
    "kKKhhhhhhhhhhhhhk..........."
    "kKkKhhhhhhhhhhhk............"
    ".kKKwwhhhhhhhhk..kkk........"
    "..kkwkhhhhhhkkkkksssk......."
    "....kkkbbbbbbbbbbbsskk......"
    "...kbbbbbbbbbbbbbbbbbbk....."
    "..kbbbbmmmmmmmmmbbbbbbbk...."
    ".kssssssssssssssssssssssk..."
    "kssgggggggggggggggggggssk..."
    "kkkkkkkkkkkkkkkkkkkkkkkkCk.."
    ".kkkkk..............kkkkk..."
    "kgssgk..............kgssgk.."
    "kgssgk..............kgssgk.."
    ".kkkk................kkkk...";

/* ---- faces for the briefing room, 20 x 20 ---- */
static const char FACE_FLINT[] =
    "..kk..........kk...."
    ".kbbk........kbbk..."
    ".kbhbk......kbhbk..."
    ".kbhhbkkkkkkbhhbk..."
    "..kbhbbbbbbbbhbk...."
    "..krrrrrrrrrrrrk...."
    "..kbbttttttttbbkr..."
    ".kbbttwkttwkttbbkr.."
    ".kbtttkkttkkttttbk.."
    ".kbtttttttttttttbk.."
    ".kbttttkkkktttttbk.."
    "..kbttkwkkwkttttk..."
    "..kbbttkkkkttbbk...."
    "...kbbbttttbbbk....."
    "....kkbbbbbbkk......"
    "...kebbbhhbbbek....."
    "..keebbhhhhbbeek...."
    ".keeebbhhhhbbeeek..."
    ".keeebbhhhhbbeeek..."
    ".kkkkkkkkkkkkkkkk...";
static const char FACE_VESPER[] =
    "..kk..........kk...."
    ".kggk........kggk..."
    ".kglgk......kglgk..."
    ".kgllgkkkkkkgllgk..."
    "..kglggggggggglgk..."
    "..kggggggggggggk...."
    "..kssssssssssssk...."
    ".kgsCCCCsssCCCCsgk.."
    ".kgsCwCCsssCwCCsgk.."
    ".kgsssssgggssssgk..."
    ".kggggggggggggggk..."
    "..kggggkkkkggggk...."
    "..kgggggggggggk....."
    "...kgggggggggk......"
    "....kkggggggkk......"
    "...kdddggggdddk....."
    "..kdddsssssdddddk..."
    ".kddddsssssddddddk.."
    ".kddddsssssddddddk.."
    ".kkkkkkkkkkkkkkkkk..";
static const char FACE_DART[] =
    "..kk..........kk...."
    ".kook........kook..."
    ".koaok......koaok..."
    ".koaaokkkkkkoaaok..."
    "..kBBBBBBBBBBBBk...."
    ".kBBBBBBBBBBBBBBk..."
    ".kBwBBBBBBBBBBBBk..."
    ".kkkkkkkkkkkkkkkk..."
    ".koooowkoooowkook..."
    ".kooookkooookkook..."
    ".koooooooooooooook.."
    "..koooookkkooooook.."
    "..kooookwwkooook...."
    "...kooookkoooook...."
    "....kkoooooookk....."
    "...krrrrrrrrrrrk...."
    "..krrrrgggggrrrrk..."
    ".krrrrgggggggrrrrk.."
    ".krrrrgggggggrrrrk.."
    ".kkkkkkkkkkkkkkkkk..";
static const char FACE_SABLE[] =
    "..kk..........kk...."
    ".kdnk........kdnk..."
    ".kdnnk......kdnnk..."
    ".kdnnnkkkkkknnndk..."
    "..kdnnnnnnnnnndk...."
    "..knnnnnnnnnnnnk...."
    ".knnnnnnnnnnnnnnk..."
    ".knnnwwknnnwwknnk..."
    ".knnnkwknnnkwknnk..."
    ".knnnnnnnnnnnnnnk..."
    ".knnnnnnkknnnnnnk..."
    "..knnnnkddknnnnk...."
    "..knnnnnkknnnnk....."
    "...kppppppppppk....."
    "..kppppppppppppk...."
    "..kpppkkkkkkpppk...."
    "..knnnnnnnnnnnnk...."
    ".knnnnnnnnnnnnnnk..."
    ".knnnnnnnnnnnnnnk..."
    ".kkkkkkkkkkkkkkkk...";
static const char FACE_HUSH[] =
    "..kk..........kk...."
    ".kllk........kllk..."
    ".klwlk......klwlk..."
    ".klwwlkkkkkklwwlk..."
    "..klllllllllllk....."
    "..klllllllllllk....."
    ".kllkkkklllkkkkk...."
    ".kllkCCkkklkCCk....."
    ".kllkkkklllkkkk....."
    ".klllllllllllllk...."
    ".kllwwwwwwwwwwlk...."
    "..klwwwkkkwwwwk....."
    "..klwwwwwwwwwk......"
    "...kllwwwwwlk......."
    "....kkllllkk........"
    "...kfffffffffk......"
    "..kffaaffffaaffk...."
    ".kfffaafffffaafffk.."
    ".kffffffaaafffffffk."
    ".kkkkkkkkkkkkkkkkkk.";
static const char FACE_COIL[] =
    "........kkkk........"
    "......kkPPPPkk......"
    ".....kPPPPPPPPk....."
    "....kPPkkPPkkPPk...."
    "...kPPPPPPPPPPPPk..."
    "...kPPyykPPkyyPPk..."
    "...kPPykkPPkkyPPk..."
    "...kPPPPPPPPPPPPk..."
    "....kPPPPrrPPPPk...."
    ".....kPPPrrPPPk....."
    "......kkPrrPkk......"
    ".......kkrrkk......."
    "......kaaaaaak......"
    ".....kaPPPPPPak....."
    "....kPPPPPPPPPPk...."
    "...kPPPPaaaaPPPPk..."
    "..kPPPPPPPPPPPPPPk.."
    ".kPPPPPPPPPPPPPPPPk."
    ".kPPPPPPPPPPPPPPPPk."
    ".kkkkkkkkkkkkkkkkkk.";

/* ---- small things ---- */
static const char BARREL[] =
    "..kkkkkkkk.."
    ".kbbbbbbbbk."
    "kkkkkkkkkkkk"
    "kbbbebbbebbk"
    "kbbbebbbebbk"
    "kkkkkkkkkkkk"
    "kbbbebbbebbk"
    "kbbbebbbebbk"
    "kkkkkkkkkkkk"
    ".kbbbbbbbbk."
    "..kkkkkkkk.."
    "............";
static const char MANGO[] =
    "....kfk."
    "...kfzk."
    "..kkkk.."
    ".kayyak."
    "kaayyyok"
    "kooyyook"
    "kooaoook"
    ".kooook."
    "..kkkk..";
static const char LIFE[] =
    "k......k"
    "kk....kk"
    "krk..krk"
    "krrkkrrk"
    "krrrrrrk"
    ".krwrrk."
    ".krrrrk."
    "..krrk.."
    "...kk...";
static const char KEY[] =
    ".kkk...."
    "kyyyk..."
    "kykyk..."
    "kyyykkkk"
    ".kkkyyyk"
    "....kyky"
    "....k.kk"
    "........";
static const char CONE[] =
    "...kk..."
    "...ko..."
    "..kook.."
    "..kwwk.."
    ".kooook."
    ".kwwwwk."
    "kooooook"
    "kkkkkkkk";
static const char HURDLE[] =
    "kkkkkkkkkkkk"
    "kwwrrwwrrwwk"
    "krrwwrrwwrrk"
    "kkkkkkkkkkkk"
    ".ks......sk."
    ".ks......sk."
    ".ks......sk."
    ".ks......sk."
    ".ks......sk."
    ".ks......sk."
    "kkkk....kkkk"
    "kkkk....kkkk";
static const char BRUNNER[] =
    "...kkkk..."
    "..kNNNNk.."
    "..kggggk.."
    ".kzwzzzk.."
    "..kzziik.."
    "..kNNNk..."
    ".kzNgNNk.."
    "kzkNgNkzk."
    "..kNNNk..."
    "..kNkNk..."
    ".kNk.kNk.."
    ".kzk.kzk.."
    "kkk...kkk."
    ".........."
    ".........."
    "..........";
static const char BIKER[] =
    "...........kkkk........."
    "..........kNNNNk........"
    "..........kggggk........"
    ".........kzzzwzk........"
    "..........kiizzk........"
    ".......kkkkNNNNkk......."
    "......kssssNNNNNk......."
    ".......kkkNNNNkk........"
    "...kkkkkkkkkkkkkkkkkk..."
    "..kNNNNNNNNNNNNNNNNNNk.."
    ".kggggggggggggggggggggk."
    ".kkkkkkkkkkkkkkkkkkkkkk."
    "..kCCk..........kCCk...."
    "...kk............kk....."
    "........................"
    "........................";
static const char MINE[] =
    "...kk..."
    ".k.kk.k."
    "..kssk.."
    "kkssrskk"
    "kkssssk."
    "..kssk.."
    ".k.kk.k."
    "...kk...";
static const char CAMERA[] =
    "kkkkkkkk"
    "kssssssk"
    "ksgggssk"
    "ksgrgssk"
    "ksgggssk"
    "kssssssk"
    ".kkkkkk."
    "...kk...";
static const char CIPHER[] =
    "................"
    ".....kkkkk......"
    "....kjjjjjk....."
    "...kjjwkjjjk...."
    "..kjjjkkjjjjk..."
    "..kjjjjjjjjjk..."
    "..kjjIIIIIIjk..."
    "...kjICCCCIk...."
    "...kjICCCCIk...."
    "..kjjIIIIIIjk..."
    "..kjjjjjjjjjk..."
    ".kjjk.kjk.kjjk.."
    ".kjk..kjk..kjk.."
    "..k...kjjk..k..."
    ".......kjjk....."
    "........kk......";
static const char IT_DART[] =
    "........"
    ".kk....."
    ".kwk...."
    "..kwkkk."
    "...kwppk"
    "....kppk"
    "....kkk."
    "........";
static const char IT_AMMO[] =
    ".kkkkkk."
    "kaaaaaak"
    "kakkkkak"
    "kakyykak"
    "kakyykak"
    "kakkkkak"
    "kaaaaaak"
    ".kkkkkk.";
static const char IT_STUN[] =
    "...kk..."
    "..kssk.."
    ".kwwwwk."
    "kwwyywwk"
    "kwwyywwk"
    "kwwwwwwk"
    ".kwwwwk."
    "..kkkk..";
static const char IT_GREN[] =
    "...kk..."
    "..kssk.."
    ".kffffk."
    "kfzfzffk"
    "kffffffk"
    "kfzfzffk"
    ".kffffk."
    "..kkkk..";
static const char IT_CARD[] =
    "kkkkkkkk"
    "kwwwwwwk"
    "kwkkkkwk"
    "kwwwwwwk"
    "kwkkwwwk"
    "kwwwwwwk"
    "kkkkkkkk"
    "........";

/* ---- building frames from row blocks ---------------------------------------------- */

static char buf[64 * 64 + 1];
static int blen;

static void rows(const char *const *r, int n) {
    for (int i = 0; i < n; i++) {
        int l = (int)strlen(r[i]);
        if (blen + l < (int)sizeof buf) { memcpy(buf + blen, r[i], (size_t)l); blen += l; }
    }
    buf[blen] = 0;
}
static void blank(int w, int n) {
    for (int i = 0; i < w * n && blen < (int)sizeof buf - 1; i++) buf[blen++] = '.';
    buf[blen] = 0;
}
static int art_bad;
static void make(int id, int w, int h) {
    if (blen != w * h) { art_bad++; blen = 0; return; }
    spr_make(&eko_spr[id], w, h, buf);
    blen = 0;
}
static void make_str(int id, int w, int h, const char *s) {
    if ((int)strlen(s) != w * h) { art_bad++; return; }
    spr_make(&eko_spr[id], w, h, s);
}

/* a sprite turned a quarter clockwise, k times */
static void rot90(Sprite *d, const Sprite *s) {
    d->w = s->h;
    d->h = s->w;
    d->px = (uint8_t *)malloc((size_t)(d->w * d->h));
    if (!d->px) return;
    for (int y = 0; y < s->h; y++)
        for (int x = 0; x < s->w; x++) d->px[x * d->w + (s->h - 1 - y)] = s->px[y * s->w + x];
}
static void rotate(int dst, int src, int k) {
    Sprite cur = eko_spr[src];
    for (int r = 0; r < k; r++) {
        Sprite n;
        rot90(&n, &cur);
        if (!n.px) return;
        if (r > 0) free(cur.px);
        cur = n;
    }
    eko_spr[dst] = cur;
}

/* a smaller copy (num / den of the size), nearest pixel */
static void shrink(Sprite *d, const Sprite *s, int num, int den) {
    d->w = (int16_t)imax(1, s->w * num / den);
    d->h = (int16_t)imax(1, s->h * num / den);
    d->px = (uint8_t *)malloc((size_t)(d->w * d->h));
    if (!d->px) return;
    for (int y = 0; y < d->h; y++)
        for (int x = 0; x < d->w; x++) d->px[y * d->w + x] = s->px[(y * den / num) * s->w + x * den / num];
}

void eko_art_load(void) {
    static bool done;
    if (done) return;
    done = true;
    /* FLINT */
    rows(FL_TOP, 16); rows(FL_LEGS[0], 8); make(SP_FLINT_STAND, 16, 24);
    rows(FL_TOP, 16); rows(FL_LEGS[1], 8); make(SP_FLINT_WALK1, 16, 24);
    rows(FL_TOP, 16); rows(FL_LEGS[2], 8); make(SP_FLINT_WALK2, 16, 24);
    rows(FL_TOP_PUNCH, 16); rows(FL_LEGS[0], 8); make(SP_FLINT_PUNCH, 16, 24);
    rows(FL_TOP, 16); rows(FL_LEGS[3], 8); make(SP_FLINT_KICK, 16, 24);
    blank(16, 9); rows(FL_TOP, 15); make(SP_FLINT_DUCK, 16, 24);
    blank(16, 9); rows(FL_TOP_PUNCH, 15); make(SP_FLINT_LOW, 16, 24);
    rows(FL_TOP, 16); rows(FL_LEGS[4], 8); make(SP_FLINT_JUMP, 16, 24);
    rows(FL_TOP_PUNCH, 16); rows(FL_LEGS[3], 8); make(SP_FLINT_AIRKICK, 16, 24);
    rows(FL_TOP_PUNCH, 16); rows(FL_LEGS[1], 8); make(SP_FLINT_CHARGE, 16, 24);
    rows(FL_TOP, 16); rows(FL_LEGS[0], 8); make(SP_FLINT_CARRY, 16, 24);
    rows(FL_TOP, 16); rows(FL_LEGS[4], 8); make(SP_FLINT_HURT, 16, 24);
    /* troopers */
    rows(TR_TOP, 16); rows(TR_LEGS[0], 8); make(SP_TROOP_STAND, 16, 24);
    rows(TR_TOP, 16); rows(TR_LEGS[1], 8); make(SP_TROOP_WALK, 16, 24);
    rows(TR_TOP_AIM, 16); rows(TR_LEGS[0], 8); make(SP_TROOP_PUNCH, 16, 24);
    rows(TR_TOP_AIM, 16); rows(TR_LEGS[0], 8); make(SP_TROOP_AIM, 16, 24);
    rows(TR_TOP, 16); rows(TR_LEGS[0], 8); make(SP_GREN, 16, 24);
    rows(TR_TOP_AIM, 16); rows(TR_LEGS[1], 8); make(SP_GREN_THROW, 16, 24);
    rows(TR_TOP, 16); rows(TR_LEGS[0], 8); make(SP_JET1, 16, 24);
    rows(TR_TOP, 16); rows(TR_LEGS[1], 8); make(SP_JET2, 16, 24);
    rows(TR_TOP, 16); rows(TR_LEGS[0], 8); make(SP_KEYMAN, 16, 24);
    make_str(SP_BARREL, 12, 12, BARREL);
    make_str(SP_MANGO, 8, 9, MANGO);
    make_str(SP_LIFE, 8, 9, LIFE);
    make_str(SP_KEY, 8, 8, KEY);
    make_str(SP_SCRAPJAW, 32, 32, SCRAPJAW);
    make_str(SP_SCRAPJAW2, 32, 32, SCRAPJAW);
    make_str(SP_TALPA, 32, 32, TALPA);
    make_str(SP_TALPA2, 32, 32, TALPA);
    /* VESPER and the gallery */
    rows(VE_TOP, 14); rows(VE_LEGS[0], 10); make(SP_VESPER, 16, 24);
    rows(VE_TOP, 14); rows(VE_LEGS[1], 10); make(SP_VESPER_MOVE, 16, 24);
    rows(VE_FIRE_TOP, 14); rows(VE_LEGS[2], 10); make(SP_VESPER_FIRE, 16, 24);
    make_str(SP_VESPER_ROLL, 16, 24, VE_ROLL);
    rows(GT_TOP, 16); rows(GT_LEGS[0], 8); make(SP_GTROOP, 16, 24);
    rows(GT_TOP_AIM, 16); rows(GT_LEGS[0], 8); make(SP_GTROOP_AIM, 16, 24);
    rows(GT_TOP, 16); rows(GT_LEGS[1], 8); make(SP_GRUNNER, 16, 24);
    rows(GT_TOP_AIM, 16); rows(GT_LEGS[1], 8); make(SP_GMORTAR, 16, 24);
    make_str(SP_RAMROD, 32, 32, RAMROD);
    make_str(SP_RAMROD2, 32, 32, RAMROD);
    make_str(SP_VULTRA, 32, 24, VULTRA);
    make_str(SP_VULTRA2, 32, 24, VULTRA);
    make_str(SP_COIL, 20, 20, FACE_COIL);
    /* DART */
    make_str(SP_BIKE1, 24, 16, BIKE1);
    make_str(SP_BIKE2, 24, 16, BIKE2);
    make_str(SP_CONE, 8, 8, CONE);
    make_str(SP_HURDLE, 12, 12, HURDLE);
    make_str(SP_BRUNNER, 10, 16, BRUNNER);
    make_str(SP_BIKER, 24, 16, BIKER);
    make_str(SP_HOG, 28, 20, HOG);
    make_str(SP_HOG2, 28, 20, HOG);
    make_str(SP_MINE, 8, 8, MINE);
    /* SABLE: right, then turned for down, left (a mirror would do, but the
     * scarf should stay on her chest) and up */
    make_str(SP_SABLE_R, 16, 16, SABLE_R);
    make_str(SP_SABLE_R2, 16, 16, SABLE_R2);
    rotate(SP_SABLE_D, SP_SABLE_R, 1);
    rotate(SP_SABLE_L, SP_SABLE_R, 2);
    rotate(SP_SABLE_U, SP_SABLE_R, 3);
    rotate(SP_SABLE_D2, SP_SABLE_R2, 1);
    rotate(SP_SABLE_L2, SP_SABLE_R2, 2);
    rotate(SP_SABLE_U2, SP_SABLE_R2, 3);
    make_str(SP_GUARD_R, 16, 16, GUARD_R);
    make_str(SP_GUARD_R2, 16, 16, GUARD_R2);
    rotate(SP_GUARD_D, SP_GUARD_R, 1);
    rotate(SP_GUARD_L, SP_GUARD_R, 2);
    rotate(SP_GUARD_U, SP_GUARD_R, 3);
    rotate(SP_GUARD_D2, SP_GUARD_R2, 1);
    rotate(SP_GUARD_L2, SP_GUARD_R2, 2);
    rotate(SP_GUARD_U2, SP_GUARD_R2, 3);
    make_str(SP_CAMERA, 8, 8, CAMERA);
    make_str(SP_CIPHER, 16, 16, CIPHER);
    make_str(SP_CIPHER2, 16, 16, CIPHER);
    make_str(SP_IT_DART, 8, 8, IT_DART);
    make_str(SP_IT_AMMO, 8, 8, IT_AMMO);
    make_str(SP_IT_STUN, 8, 8, IT_STUN);
    make_str(SP_IT_GREN, 8, 8, IT_GREN);
    make_str(SP_IT_CARD, 8, 8, IT_CARD);
    make_str(SP_FACE_FLINT, 20, 20, FACE_FLINT);
    make_str(SP_FACE_VESPER, 20, 20, FACE_VESPER);
    make_str(SP_FACE_DART, 20, 20, FACE_DART);
    make_str(SP_FACE_SABLE, 20, 20, FACE_SABLE);
    make_str(SP_FACE_HUSH, 20, 20, FACE_HUSH);
    make_str(SP_FACE_COIL, 20, 20, FACE_COIL);
    /* the jailbreak's pieces: the top-down bat, dressed per bat at draw time */
    make_str(SP_RS_FLINT, 16, 16, SABLE_R);
    make_str(SP_RS_VESPER, 16, 16, SABLE_R);
    make_str(SP_RS_DART, 16, 16, SABLE_R);
    make_str(SP_RS_SABLE, 16, 16, SABLE_R);
    make_str(SP_RS_GUARD, 16, 16, GUARD_R);
    make_str(SP_RS_GUARD2, 16, 16, GUARD_R2);
    make_str(SP_RS_CRATE, 8, 8, IT_AMMO);
    make_str(SP_RS_CELL, 8, 8, IT_CARD);
    /* the gallery's far and middle rows */
    static const int GAL[] = {SP_GTROOP, SP_GTROOP_AIM, SP_GRUNNER, SP_GMORTAR};
    for (int i = 0; i < ARRAY_LEN(GAL); i++) {
        shrink(&eko_spr_half[GAL[i]], &eko_spr[GAL[i]], 1, 2);
        shrink(&eko_spr_3q[GAL[i]], &eko_spr[GAL[i]], 3, 4);
    }
}

int eko_art_check(void) {
    int bad = art_bad;
    for (int i = 0; i < SP_COUNT; i++) bad += eko_spr[i].px == NULL;
    return bad;
}
