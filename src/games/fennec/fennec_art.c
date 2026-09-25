/* FENNEC FOUNTAIN - pixel art (palette-letter strings, see gfx.h).
 * Blocks and tiles are drawn in code (fennec.c); the creatures are here. */
#include "fennec.h"

Sprite fn_spr[FS_SPRITE_COUNT];

/* Fen the fennec: big ears, sandy fur (e/h), cream belly (c), dark eyes */
static const char FEN_D[] =
    "..k........k...."
    ".kek......kek..."
    ".kehk....khek..."
    ".kehhkkkkkhhek.."
    "..keeeeeeeeek..."
    "..keekeeeekeek.."
    "..kechhhhhhcek.."
    "...kchhkkhhck..."
    "....kkcccckk...."
    "...keeccccek...."
    "..keeecccceek..."
    "..keeecccceek..."
    "...keeeeeeek...."
    "...kek..kek....."
    "...kk....kk....."
    "................";
static const char FEN_D2[] =
    "..k........k...."
    ".kek......kek..."
    ".kehk....khek..."
    ".kehhkkkkkhhek.."
    "..keeeeeeeeek..."
    "..keekeeeekeek.."
    "..kechhhhhhcek.."
    "...kchhkkhhck..."
    "....kkcccckk...."
    "...keeccccek...."
    "..keeecccceek..."
    "..keeecccceek..."
    "...keeeeeeek...."
    "....kek.kek....."
    "....kk...kk....."
    "................";
static const char FEN_U[] =
    "..k........k...."
    ".kek......kek..."
    ".kehk....khek..."
    ".kehhkkkkkhhek.."
    "..keeeeeeeeek..."
    "..keeeeeeeeek..."
    "..keeeeeeeeek..."
    "...keeeeeeek...."
    "....kkeeeekk...."
    "...keeeeeeek...."
    "..keeeeeeeeek..."
    "..keeeeeeeeek..."
    "...keeeeeeekhk.."
    "...kek..kek.khk."
    "...kk....kk..k.."
    "................";
static const char FEN_U2[] =
    "..k........k...."
    ".kek......kek..."
    ".kehk....khek..."
    ".kehhkkkkkhhek.."
    "..keeeeeeeeek..."
    "..keeeeeeeeek..."
    "..keeeeeeeeek..."
    "...keeeeeeek...."
    "....kkeeeekk...."
    "...keeeeeeek...."
    "..keeeeeeeeek..."
    "..keeeeeeeeek..."
    "...keeeeeeekhk.."
    "....kek.kek.khk."
    "....kk...kk..k.."
    "................";
static const char FEN_S[] =
    ".....k....k....."
    "....kek..kek...."
    "....kehkkhek...."
    "....kehhhhek...."
    "...keeeeeeeek..."
    "...keeeeekeek..."
    "...kchhhhhhcek.."
    "....kchhhhhhhkk."
    ".....kkkcccckk.."
    "..khkeeecccek..."
    ".kheeeeeecccek.."
    ".kheeeeeeeeeek.."
    "..kkeeeeeeeek..."
    "....kek..kek...."
    "....kk....kk...."
    "................";
static const char FEN_S2[] =
    ".....k....k....."
    "....kek..kek...."
    "....kehkkhek...."
    "....kehhhhek...."
    "...keeeeeeeek..."
    "...keeeeekeek..."
    "...kchhhhhhcek.."
    "....kchhhhhhhkk."
    ".....kkkcccckk.."
    "..khkeeecccek..."
    ".kheeeeeecccek.."
    ".kheeeeeeeeeek.."
    "..kkeeeeeeeek..."
    "...kek...kek...."
    "...kk.....kk...."
    "................";
static const char FEN_PUSH[] =
    "......k....k...."
    ".....kek..kek..."
    ".....kehkkhek..."
    ".....kehhhhek..."
    "....keeeeeeeek.."
    "....keeeeekeek.."
    "....kchhhhhhcek."
    ".....kchhhhhhhkk"
    "..khkkkkcccckhk."
    ".kheeeeeecccekhk"
    ".kheeeeeecccek.."
    "..kkeeeeeeeeek.."
    "....keeeeeeek..."
    "...kek...kek...."
    "...kk.....kk...."
    "................";
/* Zizi, Fen's sister: a pink scarf */
static const char ZIZI[] =
    "..k........k...."
    ".kek......kek..."
    ".kehk....khek..."
    ".kehhkkkkkhhek.."
    "..keeeeeeeeek..."
    "..keekeeeekeek.."
    "..kechhhhhhcek.."
    "...kchhkkhhck..."
    "...kKKKKKKKKk..."
    "..kKPPPPPPPPKk.."
    "..keeecccceek..."
    "..keeecccceek..."
    "...keeeeeeek...."
    "...kek..kek....."
    "...kk....kk....."
    "................";
/* Grand Vizier Humph, a camel in a turban */
static const char HUMPH[] =
    "....kkkkkk......"
    "...kwwlwwwk....."
    "..kwwwlwwwwk...."
    "..kkkkkkkkkk...."
    "...kttttttk....."
    "..kttktttttk...."
    "..ktttttttttkk.."
    "...ktttttteeek.."
    "....kkkkktteek.."
    "...kttttttkkk..."
    "..kttVVVVttk...."
    ".kttVVyyVVttk..."
    ".kttVVVVVVttk..."
    "..kttttttttk...."
    "..ktk....ktk...."
    "..kk......kk....";
static const char GECKO1[] =
    "................"
    "......kkk......."
    ".....kzzzk......"
    ".....kzkzk......"
    "..k..kzzzk..k..."
    ".kzk.kiiik.kzk.."
    "..kzkzzzzzkzk..."
    "....kzzizzk....."
    "....kzziizk....."
    "..kzkzzzzzkzk..."
    ".kzk.kzzzk.kzk.."
    "..k...kzk...k..."
    "......kzk......."
    ".......kzk......"
    "........kk......"
    "................";
static const char GECKO2[] =
    "................"
    "......kkk......."
    ".....kzzzk......"
    ".....kzkzk......"
    "....kkzzzkk....."
    "...kzkiiikzk...."
    "..kzkzzzzzkzk..."
    "....kzzizzk....."
    "....kzziizk....."
    "...kkzzzzzkk...."
    "..kzkkzzzkkzk..."
    "..k...kzk...k..."
    "......kzk......."
    "......kzk......."
    ".....kk........."
    "................";
static const char STONE[] =
    "................"
    ".....kkkkkk....."
    "....kglllllk...."
    "...kglkkkklgk..."
    "..kgklCIICklgk.."
    "..kglCIwICClgk.."
    "..kglCIICCClgk.."
    "..kglCCCCuClgk.."
    "..kgklCCuuklgk.."
    "...kglkkkklgk..."
    "...kggllllggk..."
    "....kssssssk...."
    ".....kkkkkk....."
    "................"
    "................"
    "................";
static const char SPRING_DRY[] =
    "................"
    ".....kkkkkk....."
    "...kkllllllkk..."
    "..klgggggggglk.."
    ".klgkbbbbbbkglk."
    ".klgbebbebbbglk."
    ".klgbbebebbbglk."
    ".klgbbbebbebglk."
    ".klgbebbbbbbglk."
    ".klgkbbbbbbkglk."
    "..kggggggggggk.."
    "...kkggggggkk..."
    ".....kkkkkk....."
    "................"
    "................"
    "................";
static const char SPRING_WET[] =
    ".......I........"
    ".....kkIkkk....."
    "...kkllClllkk..."
    "..klggICIgggglk."
    ".klgkBuCuBBkglk."
    ".klgBuCIuuBBglk."
    ".klgBBuuCuuBglk."
    ".klgBuuCuuIBglk."
    ".klgBBuuuuBBglk."
    ".klgkBBBBBBkglk."
    "..kggggggggggk.."
    "...kkggggggkk..."
    ".....kkkkkk....."
    "................"
    "................"
    "................";
static const char DROP[] =
    "..k..."
    ".kIk.."
    ".kCk.."
    "kCICk."
    "kCuCk."
    ".kkk..";
static const char PALM[] =
    ".....kkk..kkk..."
    "...kkzzzkkzzzkk."
    "..kzzjjzzzzjjzzk"
    ".kzjjk.kzzk.kjjk"
    ".kjk...kttk...k."
    "..k....kttk....."
    ".......kttk....."
    "......ktttk....."
    "......kttk......"
    "......kttk......"
    ".....ktttk......"
    ".....kttk......."
    ".....kttk......."
    "....ktttk......."
    "...kkeeekk......"
    "..kkkkkkkkk.....";
static const char DOOR[] =
    "....kkkkkkkk...."
    "..kktttttttkk..."
    ".kttkkkkkkkttk.."
    ".ktk.......ktk.."
    "ktk.........ktk."
    "ktk.........ktk."
    "ktk.........ktk."
    "ktk.........ktk."
    "ktk.........ktk."
    "ktk.........ktk."
    "ktk.........ktk."
    "ktk.........ktk."
    "ktk.........ktk."
    "ktk.........ktk."
    "kkk.........kkk."
    "................";
static const char LOCK[] =
    "..kkk..."
    ".k...k.."
    ".k...k.."
    "kyyyyyk."
    "kyakayk."
    "kyykyyk."
    "kaaaaak."
    ".kkkkk..";
static const char CURSOR[] =
    "yyy..........yyy"
    "y..............y"
    "y..............y"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "y..............y"
    "y..............y"
    "yyy..........yyy";

typedef struct Art {
    int id, w, h;
    const char *px;
} Art;

static const Art ART[] = {
    {FS_FEN_D, 16, 16, FEN_D}, {FS_FEN_D2, 16, 16, FEN_D2}, {FS_FEN_U, 16, 16, FEN_U}, {FS_FEN_U2, 16, 16, FEN_U2},
    {FS_FEN_S, 16, 16, FEN_S}, {FS_FEN_S2, 16, 16, FEN_S2}, {FS_FEN_PUSH, 16, 16, FEN_PUSH}, {FS_ZIZI, 16, 16, ZIZI},
    {FS_HUMPH, 16, 16, HUMPH}, {FS_GECKO1, 16, 16, GECKO1}, {FS_GECKO2, 16, 16, GECKO2}, {FS_STONE, 16, 16, STONE},
    {FS_SPRING_DRY, 16, 16, SPRING_DRY}, {FS_SPRING_WET, 16, 16, SPRING_WET}, {FS_DROP, 6, 6, DROP},
    {FS_PALM, 16, 16, PALM}, {FS_DOOR, 16, 16, DOOR}, {FS_LOCK, 8, 8, LOCK}, {FS_CURSOR, 16, 16, CURSOR},
};

void fn_art_load(void) {
    static int loaded;
    if (loaded) return;
    for (int i = 0; i < ARRAY_LEN(ART); i++) {
        int len = (int)strlen(ART[i].px);
        if (len != ART[i].w * ART[i].h) fprintf(stderr, "fennec art %d: expected %d got %d\n", ART[i].id, ART[i].w * ART[i].h, len);
        spr_make(&fn_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
    }
    loaded = 1;
}
