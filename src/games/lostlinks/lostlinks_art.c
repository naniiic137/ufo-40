/* LOST LINKS - pixel art (palette-letter strings, see gfx.h) and the tiles
 * of the world, all drawn for UFO 40. */
#include "lostlinks.h"

Sprite lnk_spr[LS_COUNT];

static const char BALL[] =
    "..kkk.."
    ".kwwwk."
    "kwwlwwk"
    "kwwwwlk"
    "klwwwwk"
    ".kwwlk."
    "..kkk..";
static const char SHADOW[] =
    ".kkkkk."
    "kkkkkkk"
    ".kkkkk.";
/* an iron, floating: grip, shaft, a bright head */
static const char IRON[] =
    "..........c."
    ".........kkc"
    "........kbbk"
    ".......kbbk."
    "......kgk..."
    ".....kgk...."
    "....kgk....."
    "...kgk......"
    "..kgk......."
    ".kllk......."
    "klwwlk......"
    "kllwwlk.c..."
    ".kkllkk....."
    "...kk......."
    ;
/* a scorecrow: a brass-and-iron crow with a red lamp for an eye */
static const char CROW1[] =
    "....kkk....."
    "...kdddk...."
    "..kdrddkaa.."
    "..kddddkka.."
    ".kddsdddk..."
    "kddssssddk.."
    "kdssssssdk.."
    ".kdddddddk.."
    "..kkkkkkk..."
    "...g...g...."
    "..kg..kg....";
static const char CROW2[] =
    "k.........k."
    "dk..kkk..kd."
    "ddkkdrdkkdd."
    ".dddddddddaa"
    "..kddsdddkka"
    "..kdssssdk.."
    "...kssssk..."
    "....kkkk...."
    ".....g.g...."
    "....kg.kg..."
    "............";
static const char CROW_DEAD[] =
    "............"
    "..k...kk...."
    ".kdk.kddkr.."
    "kddskdssdk.a"
    "kkkkkkkkkkk.";
/* a slicer: a mowing-machine gnat with a needle */
static const char SLICER1[] =
    "I...I...."
    "IIkIIk..."
    ".kIIkI..."
    "..kggk..."
    ".kgrrgk.."
    ".kgggggkr"
    "..kgggk.."
    "...kkk..."
    "..k...k..";
static const char SLICER2[] =
    "........."
    "........."
    ".IIkIIk.."
    "IIkggkI.."
    ".kgrrgk.."
    ".kgggggkr"
    "..kgggk.."
    "...kkk..."
    "...k.k...";
/* a lark in a bush: mostly hidden, now and then it looks out */
static const char LARK1[] =
    "...ffff..."
    "..fjjjjf.."
    ".fjzjjjjf."
    "fjjjjjzjjf"
    "fjzjjjjjjf"
    ".fjjjzjjf."
    "..ffffff.."
    "....bb....";
static const char LARK2[] =
    "...ffffk.."
    "..fjjjkeka"
    ".fjzjjkekk"
    "fjjjjjkeef"
    "fjzjjjjkef"
    ".fjjjzjjf."
    "..ffffff.."
    "....bb....";
/* an albatross, wings up and down */
static const char ALBA1[] =
    "w..............w"
    "lw............wl"
    ".lw..........wl."
    "..lww......wwl.."
    "...llww..wwll..."
    ".....lwwwwl....."
    "......wwww......"
    ".....wwwwwwa...."
    ".....lwwwwkaa..."
    "......lwwl......"
    ".......ll......."
    "......a..a......"
    "................"
    "................"
    "................"
    "................";
static const char ALBA2[] =
    "................"
    "................"
    "................"
    "................"
    "......wwww......"
    ".....wwwwwwa...."
    ".lwwwlwwwwkaa..."
    "lwwlllwwwwlllwwl"
    "wll...lwwl...llw"
    "w......ll......w"
    "......a..a......"
    "................"
    "................"
    "................"
    "................"
    "................";
/* a pin: the flagstick and its flag (red, yellow once touched) */
static const char PIN[] =
    ".krrrrk..."
    ".krrrrrrk."
    ".krrrrrrrk"
    ".krrrrrrk."
    ".krrrrk..."
    ".kkkk....."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    "kkkk......";
static const char PIN_LIT[] =
    ".kyyyyk..."
    ".kyyyyyyk."
    ".kyaayyyyk"
    ".kyyyyyyk."
    ".kyyyyk..."
    ".kkkk....."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    ".wk......."
    "kkkk......";
/* a stray ball: a cream ball with two eyes */
static const char STRAY[] =
    "..kkkk.."
    ".kcccck."
    "kcckckck"
    "kcckckck"
    "kccccccc"
    "kcchhhck"
    ".kchhck."
    "..kkkk..";
/* the moss folk, who live below: a purple hood and a pale face */
static const char FOLK[] =
    "...kkkk..."
    "..kVVVVk.."
    ".kVpppVVk."
    ".kVhhhhVk."
    ".kVhkhkVk."
    ".kVhhhhVk."
    "..kVVVVk.."
    ".kpVVVVpk."
    "kpVVVVVVpk"
    "kpVVVVVVpk"
    "kpVVVVVVpk"
    ".kpppppk.."
    "..kk.kk...";
/* the Sage: an old ball with a long white beard */
static const char SAGE[] =
    "...kkkk..."
    "..klllllk."
    ".klwkwklk."
    ".klllllllk"
    ".klwwwwwlk"
    "..kwwwwwk."
    "...kwwwk.."
    "....kwk..."
    ".....k....";
/* the four abilities, each a glowing orb */
static const char ORB_HAMMER[] =
    "...kkkkk...."
    "..koooook..."
    ".koakkkaok.."
    "koakkkkkaok."
    "koookkkooook"
    "kooookooooak"
    "kooookoooook"
    "kooookoooook"
    ".koookoooak."
    "..koooooak.."
    "...kkkkkk..."
    "............";
static const char ORB_BACKSPIN[] =
    "...kkkkk...."
    "..kKKKKKk..."
    ".kKwwwwKKk.."
    "kKwKKKKwKKk."
    "kKwKwwKKwKk."
    "kKwKwKKKwKk."
    "kKKwKKKwKKk."
    "kKKKwwwKKKk."
    ".kKKKKKKKk.."
    "..kKKKKKk..."
    "...kkkkk...."
    "............";
static const char ORB_TREAD[] =
    "...kkkkk...."
    "..keeeeek..."
    ".kebebebek.."
    "kebebebebek."
    "keeeeeeeeek."
    "kebebebebek."
    "keeeeeeeeek."
    "kebebebebek."
    ".kebebebek.."
    "..keeeeek..."
    "...kkkkk...."
    "............";
static const char ORB_SKIPPER[] =
    "...kkkkk...."
    "..kuuuuuk..."
    ".kuuuuuuuk.."
    "kuuwuuuwuuk."
    "kuwuwuwuwuk."
    "kuuuuwuuuuk."
    "kuuuuuuuuuk."
    "kuuwuuuwuuk."
    ".kwuwuwuwk.."
    "..kuuuuuk..."
    "...kkkkk...."
    "............";
/* a piece of the Star Pin */
static const char PIECE[] =
    ".....k......"
    "....kyk....."
    "....kyk....."
    "...kyyyk...."
    "kkkkyyyykkkk"
    "kyyyyayyyyyk"
    ".kyyayayyyk."
    "..kyyyyyyk.."
    "..kyyyyyyk.."
    ".kyyk..kyyk."
    ".kyk....kyk."
    ".kk......kk.";
/* the altar: a block of old stone with a star socket */
static const char ALTAR[] =
    "..kkkkkkkkkkkk.."
    ".kllllllllllllk."
    "klggggggggggggsk"
    "klgggkkkkgggggsk"
    "klggkddddkggggsk"
    "klgggkddkgggggsk"
    "klggggkkggggggsk"
    "kssssssssssssssk"
    ".kgggggggggggsk."
    ".kgsgsgsgsgsgsk."
    ".kgggggggggggsk."
    ".kssssssssssssk."
    "kkkkkkkkkkkkkkkk"
    "................";
static const char PLATE[] =
    "................"
    "..kkkkkkkkkkkk.."
    ".kssssssssssssk."
    ".ksddddddddddsk."
    ".ksdnnnnnnnndsk."
    ".ksdnddddddndsk."
    ".ksdndnnnndndsk."
    ".ksdndnddndndsk."
    ".ksdndnddndndsk."
    ".ksdndnnnndndsk."
    ".ksdnddddddndsk."
    ".ksdnnnnnnnndsk."
    ".ksddddddddddsk."
    ".kssssssssssssk."
    "..kkkkkkkkkkkk.."
    "................";
static const char PLATE_LIT[] =
    "................"
    "..kkkkkkkkkkkk.."
    ".kIIIIIIIIIIIIk."
    ".kICCCCCCCCCCIk."
    ".kICuuuuuuuuCIk."
    ".kICuCCCCCCuCIk."
    ".kICuCwwwwCuCIk."
    ".kICuCwIIwCuCIk."
    ".kICuCwIIwCuCIk."
    ".kICuCwwwwCuCIk."
    ".kICuCCCCCCuCIk."
    ".kICuuuuuuuuCIk."
    ".kICCCCCCCCCCIk."
    ".kIIIIIIIIIIIIk."
    "..kkkkkkkkkkkk.."
    "................";
/* a flying saucer, nose down in the sand */
static const char SAUCER[] =
    "..........kkkkk................."
    "........kkCCCCCkk..............."
    ".......kCIIICCCCCk.............."
    "......kCIwICCCCCCk.............."
    "....kkkkkkkkkkkkkkkkk..........."
    "..kkggllllllllllllgggkk........."
    ".kgllwlllyllllylllllggskk......."
    "kgllllllllllllllllllllgssk......"
    "kssggggggggggggggggggssskk......"
    ".kkssssssssssssssssssskkeee....."
    "...kkkkkkkkkkkkkkkkkkkeeeeee...."
    "......eeeeeeeeeeeeeeeeeeeeee...."
    "....eeeeehheeeeeeeeehheeeeee...."
    "..eeeeeeeeeeeeeeeeeeeeeeeeeee..."
    "................................"
    "................................";
/* the odd tree: the wrong colour, as if painted over */
static const char TREE_ODD[] =
    "......kkkkkk........"
    "....kkVVVVVVkk......"
    "...kVVPPVVVVVVk....."
    "..kVPPPVVVVVVVVk...."
    ".kVVPVVVVVVpVVVVk..."
    ".kVVVVVVVVVVVVVVk..."
    "kVVVVVVVpVVVVVVVVk.."
    "kVVVpVVVVVVVVVVpVk.."
    "kVVVVVVVVVVVVVVVVk.."
    ".kVVVVVVVVVpVVVVk..."
    ".kpVVVVpVVVVVVVpk..."
    "..kppVVVVVVVVppk...."
    "...kkppppppppkk....."
    ".....kkkbbkkk......."
    ".......kbbk........."
    ".......kbbk........."
    ".......kbbk........."
    "......kbbbbk........"
    "......kkkkkk........"
    "...................."
    "...................."
    "....................";
/* the Brass Badger: its head out of a burrow, standing, and broken */
static const char BADGER_HEAD[] =
    "................................"
    "........kk............kk........"
    ".......kaak..........kaak......."
    ".......kaaakkkkkkkkkkaaak......."
    "......kaawwwwkkkkkkwwwwaak......"
    ".....kaawwwwkaaaaaakwwwwaak....."
    "....kaawwwwkaayyyyaakwwwwaak...."
    "....kaawwwkaayyyyyyaakwwwaak...."
    "...kaaaakkaayrryyrryaakkaaaak..."
    "...kaaaaaaayrrrrrrrryaaaaaaak..."
    "...kaaaaaaayyyyyyyyyyaaaaaaak..."
    "...kaaataaaayyykkyyyaaaataaak..."
    "....kaaatayyyykkkkyyyyataaak...."
    ".....kaaaayyyyyyyyyyyyaaaak....."
    "......kkaaaayyyyyyyyaaaakk......"
    "........kkkaaaaaaaaaakkk........"
    "......bbbbkkkkkkkkkkkkbbbb......"
    "....bbeebbbbbbbbbbbbbbbeebb....."
    "...beeeebeeeebbbeeeebeeeeb......"
    "....bbbbbbbbbbbbbbbbbbbbb......."
    ;
static const char BADGER_STAND[] =
    "........kk............kk........"
    ".......kaak..........kaak......."
    ".......kaaakkkkkkkkkkaaak......."
    "......kaawwwwkkkkkkwwwwaak......"
    ".....kaawwwwkaaaaaakwwwwaak....."
    "....kaawwwwkaayyyyaakwwwwaak...."
    "....kaawwwkaayyyyyyaakwwwaak...."
    "...kaaaakkaayrryyrryaakkaaaak..."
    "...kaaaaaaayrrrrrrrryaaaaaaak..."
    "...kaaaaaaayyyyyyyyyyaaaaaaak..."
    "....kaaaaaaayyykkyyyaaaaaaak...."
    ".....kaaaayyyykkkkyyyyaaaak....."
    "......kkaaaayyyyyyyyaaaakk......"
    "..kk....kkkaaaaaaaaaakkk....kk.."
    ".kggk..kaaaaggggggggaaaak..kggk."
    ".kgsgkkaaaagssssssssgaaaakkgsgk."
    "..kgsgaaaaagsyyyyyysgaaaaagsgk.."
    "...kgaaaaaagsyrrrrysgaaaaaagk..."
    "....kaaaaaagsyrrrrysgaaaaaak...."
    "....kaaaaaagsyyyyyysgaaaaaak...."
    "....kaaaaaagssssssssgaaaaaak...."
    "....kaaaaaaaggggggggaaaaaaak...."
    ".....kaaaaaaaaaaaaaaaaaaaak....."
    "......kaaaaaaaaaaaaaaaaaak......"
    ".......kkaaaakkkkkkaaaakk......."
    ".........kaaak....kaaak........."
    "........kaaaak....kaaaak........"
    "........kkkkk......kkkkk........"
    "......bbbbbbbbbbbbbbbbbbbb......"
    "....bbeebbbbbbbbbbbbbbbbeebb...."
    "...beeeebeeeebbbbbeeeebeeeeb...."
    "....bbbbbbbbbbbbbbbbbbbbbbb....."
    ;
static const char BADGER_DOWN[] =
    "................................"
    "................................"
    "................................"
    "..........k....k......k........."
    "........kk.k..k.kk...k.k........"
    ".......kaak..kaaak..kaak........"
    "......kaaakkkaaaakkkaaak........"
    ".....kaagggaaaatttaaaaaakk......"
    "....kaaagkgaaatkktaaaggaaak....."
    "...kaaaaggaaattkttaagkgaaaak...."
    "...kaataaaaatttktttaaggaataak..."
    "..kaaaaaaaaaaaaaaaaaaaaaaaaak..."
    "..kkaaaataaaaataaaaaataaaaakk..."
    "....kkkkkkkkkkkkkkkkkkkkkkk....."
    "......bbbbbbbbbbbbbbbbbbbb......"
    "....bbeebbbbbbbbbbbbbbbbeebb...."
    "...beeeebeeeebbbbbeeeebeeeeb...."
    "....bbbbbbbbbbbbbbbbbbbbbbb....."
    "................................"
    "................................";
static const char CROSSHAIR[] =
    ".......r......."
    ".......r......."
    ".....rrrrr....."
    "....r..r..r...."
    "...r...r...r..."
    "..r....r....r.."
    "..r.........r.."
    "rrrrrr...rrrrrr"
    "..r.........r.."
    "..r....r....r.."
    "...r...r...r..."
    "....r..r..r...."
    ".....rrrrr....."
    ".......r......."
    ".......r.......";
/* Dimple, up close (the title and the end) */
static const char DIMPLE_BIG[] =
    ".....kkkkkk....."
    "...kkwwwwwwkk..."
    "..kwwwwwwwwwwk.."
    ".kwwlwwwwwlwwwk."
    ".kwwwwwwwwwwwwk."
    "kwwwwwlwwwwwwlwk"
    "kwlwwwwwwlwwwwwk"
    "kwwwwwwwwwwwwwwk"
    "kwwwwlwwwwwwlwwk"
    "kwwlwwwwlwwwwwwk"
    "klwwwwwwwwwwwwlk"
    ".klwwwlwwwwlwlk."
    ".kllwwwwwwwwllk."
    "..kllllwwllllk.."
    "...kkllllllkk..."
    ".....kkkkkk.....";

void lnk_art_load(void) {
    static bool done;
    if (done) return;
    done = true;
    spr_make(&lnk_spr[LS_BALL], 7, 7, BALL);
    spr_make(&lnk_spr[LS_BALL_SHADOW], 7, 3, SHADOW);
    spr_make(&lnk_spr[LS_IRON], 12, 14, IRON);
    spr_make(&lnk_spr[LS_CROW1], 12, 11, CROW1);
    spr_make(&lnk_spr[LS_CROW2], 12, 11, CROW2);
    spr_make(&lnk_spr[LS_CROW_DEAD], 12, 5, CROW_DEAD);
    spr_make(&lnk_spr[LS_SLICER1], 9, 9, SLICER1);
    spr_make(&lnk_spr[LS_SLICER2], 9, 9, SLICER2);
    spr_make(&lnk_spr[LS_LARK1], 10, 8, LARK1);
    spr_make(&lnk_spr[LS_LARK2], 10, 8, LARK2);
    spr_make(&lnk_spr[LS_ALBA1], 16, 16, ALBA1);
    spr_make(&lnk_spr[LS_ALBA2], 16, 16, ALBA2);
    spr_make(&lnk_spr[LS_PIN], 10, 17, PIN);
    spr_make(&lnk_spr[LS_PIN_LIT], 10, 17, PIN_LIT);
    spr_make(&lnk_spr[LS_STRAY], 8, 8, STRAY);
    spr_make(&lnk_spr[LS_FOLK], 10, 13, FOLK);
    spr_make(&lnk_spr[LS_SAGE], 10, 9, SAGE);
    spr_make(&lnk_spr[LS_AB_HAMMER], 12, 12, ORB_HAMMER);
    spr_make(&lnk_spr[LS_AB_BACKSPIN], 12, 12, ORB_BACKSPIN);
    spr_make(&lnk_spr[LS_AB_TREAD], 12, 12, ORB_TREAD);
    spr_make(&lnk_spr[LS_AB_SKIPPER], 12, 12, ORB_SKIPPER);
    spr_make(&lnk_spr[LS_PIECE], 12, 12, PIECE);
    spr_make(&lnk_spr[LS_ALTAR], 16, 14, ALTAR);
    spr_make(&lnk_spr[LS_PLATE], 16, 16, PLATE);
    spr_make(&lnk_spr[LS_PLATE_LIT], 16, 16, PLATE_LIT);
    spr_make(&lnk_spr[LS_SAUCER], 32, 16, SAUCER);
    spr_make(&lnk_spr[LS_TREE_ODD], 20, 22, TREE_ODD);
    spr_make(&lnk_spr[LS_BADGER_HEAD], 32, 20, BADGER_HEAD);
    spr_make(&lnk_spr[LS_BADGER_STAND], 32, 32, BADGER_STAND);
    spr_make(&lnk_spr[LS_BADGER_DOWN], 32, 20, BADGER_DOWN);
    spr_make(&lnk_spr[LS_CROSSHAIR], 15, 15, CROSSHAIR);
    spr_make(&lnk_spr[LS_DIMPLE_BIG], 16, 16, DIMPLE_BIG);
}

/* ---- the tiles ------------------------------------------------------------ */

static uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}

static char at(uint8_t (*tiles)[LNK_MH][LNK_MW], int l, int x, int y) {
    if (x < 0 || y < 0 || x >= LNK_MW || y >= LNK_MH) return '#';
    return (char)tiles[l][y][x];
}

static bool wallish(char c) { return c == '#' || c == 'T' || c == 'H'; }

static void ground(int l, char ch, int tx, int ty, int x, int y) {
    uint32_t h = hash2(tx, ty);
    if (l == LNK_OVER) {
        switch (ch) {
        case ',':
            gfx_rect(x, y, 16, 16, C_FOREST);
            for (int i = 0; i < 6; i++) {
                int px = (int)((h >> (i * 5)) & 15), py = (int)((h >> (i * 3 + 7)) & 15);
                gfx_vline(x + px, y + py - 1, y + py, C_JADE);
            }
            return;
        case ':':
            gfx_rect(x, y, 16, 16, C_LEAF);
            if (tx % 2) gfx_dither(x, y, 16, 16, C_LIME, 2);
            return;
        case 's':
            gfx_rect(x, y, 16, 16, C_EARTH);
            for (int i = 0; i < 5; i++) gfx_pset(x + (int)((h >> (i * 5)) & 15), y + (int)((h >> (i * 4 + 3)) & 15), C_HIDE);
            gfx_pset(x + (int)((h >> 20) & 15), y + (int)((h >> 24) & 15), C_TAN);
            return;
        default:
            gfx_rect(x, y, 16, 16, C_JADE);
            if ((tx / 2) % 2) gfx_dither(x, y, 16, 16, C_LEAF, 2);
            if ((h & 7) == 0) gfx_pset(x + (int)((h >> 8) & 15), y + (int)((h >> 12) & 15), C_FOREST);
            return;
        }
    }
    switch (ch) {
    case ',':
        gfx_rect(x, y, 16, 16, C_PURPLE);
        for (int i = 0; i < 6; i++) {
            int px = (int)((h >> (i * 5)) & 15), py = (int)((h >> (i * 3 + 7)) & 15);
            gfx_vline(x + px, y + py - 1, y + py, C_VIOLET);
        }
        return;
    case ':':
        gfx_rect(x, y, 16, 16, C_SLATE);
        gfx_dither(x, y, 16, 16, C_DUSK, 3);
        return;
    case 's':
        gfx_rect(x, y, 16, 16, C_TAN);
        for (int i = 0; i < 5; i++) gfx_pset(x + (int)((h >> (i * 5)) & 15), y + (int)((h >> (i * 4 + 3)) & 15), C_EARTH);
        return;
    default:
        gfx_rect(x, y, 16, 16, C_DUSK);
        for (int i = 0; i < 4; i++) gfx_pset(x + (int)((h >> (i * 6)) & 15), y + (int)((h >> (i * 4 + 5)) & 15), C_NIGHT);
        return;
    }
}

/* a little arrow down the slope */
static void slope_mark(int x, int y, int k, int col, bool big) {
    static const signed char DX[10] = {0, -1, 0, 1, -1, 0, 1, -1, 0, 1};
    static const signed char DY[10] = {0, 1, 1, 1, 0, 0, 0, -1, -1, -1};
    int dx = DX[k], dy = DY[k], cx = x + 8, cy = y + 8;
    int len = big ? 5 : 3;
    gfx_line(cx - dx * len, cy - dy * len, cx + dx * len, cy + dy * len, col);
    /* the head */
    int hx = cx + dx * len, hy = cy + dy * len;
    gfx_line(hx, hy, hx - dx * 2 - dy * 2, hy - dy * 2 + dx * 2, col);
    gfx_line(hx, hy, hx - dx * 2 + dy * 2, hy - dy * 2 - dx * 2, col);
}

void lnk_draw_tile(int layer, char ch, int tx, int ty, int x, int y, int t, bool pan, uint8_t (*tiles)[LNK_MH][LNK_MW]) {
    bool over = layer == LNK_OVER;
    uint32_t h = hash2(tx, ty);
    char up = at(tiles, layer, tx, ty - 1), down = at(tiles, layer, tx, ty + 1);
    switch (ch) {
    case '#':
        if (over) {
            gfx_rect(x, y, 16, 16, C_SLATE);
            if (!wallish(up)) { gfx_rect(x, y, 16, 3, C_GREY); gfx_hline(x, x + 15, y, C_LIGHT); }
            if (!wallish(down)) { gfx_rect(x, y + 12, 16, 4, C_DUSK); gfx_hline(x, x + 15, y + 15, C_NIGHT); }
            if (h & 1) gfx_line(x + 4, y + 5, x + 8, y + 9, C_DUSK);
            if (h & 2) gfx_line(x + 11, y + 4, x + 13, y + 8, C_DUSK);
        } else {
            gfx_rect(x, y, 16, 16, C_NIGHT);
            if (!wallish(down)) { gfx_rect(x, y + 12, 16, 4, C_INK); }
            if (!wallish(up)) gfx_hline(x, x + 15, y, C_DUSK);
            if (h & 1) gfx_line(x + 3, y + 3, x + 7, y + 8, C_INK);
            if (h & 4) gfx_pset(x + 11, y + 6, C_DUSK);
        }
        return;
    case 'T':
        ground(layer, '.', tx, ty, x, y);
        gfx_rect(x + 7, y + 10, 3, 5, C_BROWN);
        gfx_circ(x + 8, y + 7, 7, C_FOREST);
        gfx_circ(x + 7, y + 6, 5, C_JADE);
        gfx_circ(x + 6, y + 4, 2, C_LEAF);
        gfx_pset(x + 10, y + 8, C_FOREST);
        return;
    case 'H':
        gfx_rect(x, y, 16, 16, over ? C_LIGHT : C_GREY);
        for (int r = 0; r < 4; r++) {
            gfx_hline(x, x + 15, y + r * 4 + 3, over ? C_GREY : C_SLATE);
            int off = (r % 2) * 4;
            gfx_vline(x + off + 2, y + r * 4, y + r * 4 + 2, over ? C_GREY : C_SLATE);
            gfx_vline(x + off + 10, y + r * 4, y + r * 4 + 2, over ? C_GREY : C_SLATE);
        }
        if (!wallish(down)) gfx_rect(x, y + 13, 16, 3, C_SLATE);
        return;
    case 'X':
        ground(layer, '.', tx, ty, x, y);
        gfx_rect(x + 1, y + 1, 14, 14, C_ORANGE);
        gfx_hline(x + 1, x + 14, y + 1, C_AMBER);
        gfx_vline(x + 1, y + 1, y + 14, C_AMBER);
        gfx_rect(x + 1, y + 13, 14, 2, C_BROWN);
        gfx_line(x + 4, y + 3, x + 7, y + 8, C_BROWN);
        gfx_line(x + 7, y + 8, x + 5, y + 12, C_BROWN);
        gfx_line(x + 7, y + 8, x + 12, y + 6, C_BROWN);
        gfx_rectb(x, y, 16, 16, C_INK);
        return;
    case '~': {
        int deep = over ? C_BLUE : C_NAVY, lite = over ? C_SKY : C_BLUE;
        gfx_rect(x, y, 16, 16, deep);
        int ph = (t / 12 + tx * 3 + ty * 5) % 16;
        gfx_hline(x + ph / 2, x + ph / 2 + 4, y + 5, lite);
        gfx_hline(x + (ph + 8) % 16 / 2 + 6, x + (ph + 8) % 16 / 2 + 9, y + 11, lite);
        if (up != '~' && !wallish(up)) gfx_hline(x, x + 15, y, over ? C_CYAN : C_SKY);
        return;
    }
    case 'r':
        ground(layer, (char)(over ? '.' : '.'), tx, ty, x, y);
        gfx_rect(x, y + 6, 16, 4, C_GREY);
        gfx_hline(x, x + 15, y + 6, C_LIGHT);
        gfx_hline(x, x + 15, y + 10, C_DUSK);
        gfx_rect(x + 2, y + 4, 3, 8, C_LIGHT);
        gfx_rect(x + 11, y + 4, 3, 8, C_LIGHT);
        gfx_vline(x + 4, y + 4, y + 11, C_SLATE);
        gfx_vline(x + 13, y + 4, y + 11, C_SLATE);
        return;
    case 'v':
        ground(layer, '.', tx, ty, x, y);
        gfx_rect(x, y + 12, 16, 4, over ? C_FOREST : C_NIGHT);
        gfx_hline(x, x + 15, y + 11, over ? C_LEAF : C_SLATE);
        return;
    case 'u':
        ground(layer, '.', tx, ty, x, y);
        gfx_circ(x + 8, y + 8, 5, over ? C_FOREST : C_NIGHT);
        gfx_circ(x + 8, y + 9, 3, over ? C_BROWN : C_INK);
        return;
    case 'o': {
        char base = up == ':' || down == ':' ? ':' : '.';
        ground(layer, base, tx, ty, x, y);
        if (over) {
            gfx_circ(x + 8, y + 8, 5, C_FOREST);
            gfx_circ(x + 8, y + 8, 4, C_INK);
            gfx_hline(x + 5, x + 11, y + 12, C_LIME);
        } else {
            /* light falls down the shaft from the hole above */
            gfx_dither_circle(x + 8, y + 8, 7, C_CREAM, 4 + (t / 20) % 2);
            gfx_circ(x + 8, y + 8, 4, C_YELLOW);
            gfx_circ(x + 8, y + 8, 2, C_WHITE);
        }
        return;
    }
    case 'O':
        ground(layer, '.', tx, ty, x, y);
        if (!over) { gfx_circ(x + 8, y + 8, 4, C_YELLOW); gfx_circ(x + 8, y + 8, 2, C_WHITE); }
        else { gfx_pset(x + 6, y + 7, C_FOREST); gfx_pset(x + 10, y + 9, C_FOREST); } /* barely there */
        return;
    case '=':
        gfx_rect(x, y, 16, 16, C_TAN);
        for (int i = 0; i < 4; i++) gfx_hline(x, x + 15, y + i * 4 + 3, C_BROWN);
        return;
    case 'f':
        ground(layer, '.', tx, ty, x, y);
        for (int i = 0; i < 4; i++) {
            int px = x + 2 + (int)((h >> (i * 6)) % 12), py = y + 2 + (int)((h >> (i * 5 + 2)) % 12);
            gfx_pset(px, py, i % 3 == 0 ? C_PINK : i % 3 == 1 ? C_YELLOW : C_WHITE);
            gfx_pset(px, py + 1, C_FOREST);
        }
        return;
    case '\'':
        ground(layer, '.', tx, ty, x, y);
        gfx_line(x + 5, y + 12, x + 7, y + 4, C_CYAN);
        gfx_line(x + 9, y + 12, x + 11, y + 7, C_ICE);
        return;
    case 'g':
        gfx_rect(x, y, 16, 16, C_INK);
        for (int i = 0; i < 4; i++) gfx_rect(x + 1 + i * 4, y, 2, 16, C_GREY);
        gfx_hline(x, x + 15, y + 3, C_SLATE);
        gfx_hline(x, x + 15, y + 12, C_SLATE);
        return;
    case 'k':
        gfx_rect(x, y, 16, 16, C_SLATE);
        gfx_rectb(x, y, 16, 16, C_INK);
        gfx_pset(x + 8, y + 5, C_YELLOW);
        gfx_hline(x + 6, x + 10, y + 7, C_YELLOW);
        gfx_pset(x + 7, y + 9, C_YELLOW);
        gfx_pset(x + 9, y + 9, C_YELLOW);
        return;
    default: break;
    }
    if (ch >= '1' && ch <= '9' && ch != '5') {
        ground(layer, '.', tx, ty, x, y);
        int k = ch - '0';
        /* shade: the ground falls away, darker downhill */
        slope_mark(x, y, k, pan ? C_YELLOW : (over ? C_FOREST : C_NIGHT), pan);
        return;
    }
    ground(layer, ch, tx, ty, x, y);
}
