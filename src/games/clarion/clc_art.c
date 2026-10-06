/* CLARION CALL - pixel art, all drawn for UFO 40. The Clarion is CHIME
 * CIRCUIT's chime ship in Clary's colours (chime_art.c), the ship she
 * races there; Ansel's and Clary's faces are hers and his from that
 * cartridge, and Lady Hush is BELLHOP's. */
#include "clc.h"
#include "../chime/chime.h"
#include "../bellhop/bellhop.h"

static bool art_ok = true;

static Sprite spr[CS_COUNT];

/* Clary, small (on a map): 7 x 9 */
static const char CLARY_S0[] =
    "..KKK.."
    ".KKKKK."
    ".Kcckc."
    "..ccc.."
    ".PPPPc."
    ".cPPP.."
    "..ppp.."
    "..t.t.."
    ".kk.kk.";
static const char CLARY_S1[] =
    "..KKK.."
    ".KKKKK."
    ".Kcckc."
    "..ccc.."
    ".PPPPc."
    ".cPPP.."
    "..ppp.."
    ".t...t."
    "kk...kk";
static const char CLARY_SJ[] =
    ".KKKK.."
    "KKKKKK."
    ".Kcckc."
    "c.ccc.c"
    ".PPPPP."
    "..PPP.."
    "..ppp.."
    ".t..t.."
    ".k..k..";
/* Clary, close up (behind a door): 12 x 16 */
static const char CLARY_B0[] =
    "....KKKK...."
    "...KKKKKKK.."
    "..KKKKKKKKK."
    "..KKccccKK.."
    "..KccckcK..."
    "...ccccccK.."
    "....cKKc...."
    "..PPPPPPPP.."
    ".cPPwPPPPPc."
    ".c.PPPPPP.c."
    "...PPPPPP..."
    "...pppppp..."
    "...tt..tt..."
    "...tt..tt..."
    "...bb..bb..."
    "..kkk..kkk..";
static const char CLARY_B1[] =
    "....KKKK...."
    "...KKKKKKK.."
    "..KKKKKKKKK."
    "..KKccccKK.."
    "..KccckcK..."
    "...ccccccK.."
    "....cKKc...."
    "..PPPPPPPP.."
    ".cPPwPPPPPc."
    ".c.PPPPPP.c."
    "...PPPPPP..."
    "...pppppp..."
    "..tt....tt.."
    "..tt....tt.."
    ".bb......bb."
    "kkk......kkk";
static const char CLARY_BJ[] =
    "...KKKKK...."
    "..KKKKKKKKK."
    ".KKKKKKKKK.."
    "..KKccccKK.."
    "..KccckcK..."
    "c..cccccc..c"
    ".c..cKKc..c."
    "..PPPPPPPP.."
    "...PPwPPPP.."
    "...PPPPPP..."
    "...PPPPPP..."
    "...pppppp..."
    "..tt...tt..."
    ".tt.....tt.."
    ".bb.....bb.."
    "kk.......kk.";
static const char CLARY_BC[] =
    "............"
    "............"
    "............"
    "............"
    "............"
    "............"
    "....KKKK...."
    "...KKKKKKK.."
    "..KKccccKK.."
    "..KccckcK..."
    "...cccccc..."
    "..PPPPPPPP.."
    ".cPPwPPPPPc."
    "..pppppppp.."
    "..tttbbttt.."
    ".kkkk..kkkk.";
/* a note: the station's lost tunes, 8 x 10 */
static const char NOTE[] =
    "...kkkk."
    "...kVVVk"
    "...kVkVk"
    "...kV.kk"
    "...kV..."
    "...kV..."
    ".kkkV..."
    "kVVVVk.."
    "kVPVVk.."
    ".kkkk...";
/* Grandsire Tock's face: 16 x 16 */
static const char TOCK[] =
    "....kkkkkkkk...."
    "..kkaaaaaaaakk.."
    ".kaayyyyyyyyaak."
    ".kayywwwwwwyyak."
    "kayywkkwkkwwyyak"
    "kaywwwkwkwwwwyak"
    "kaywkwwkwwwkwyak"
    "kaywwwwkkkwwwyak"
    "kaywkwwwwwwkwyak"
    "kaywwwwkwwwwwyak"
    ".kayywwwwwwyyak."
    ".kaayyyyyyyyaak."
    "..kkaaaaaaaakk.."
    "...kkvvvvvvkk..."
    "..kvvvkkkkvvvk.."
    "..kkk......kkk..";
/* the cave skull that follows: 18 x 18 */
static const char SKULL[] =
    ".....kkkkkkkk....."
    "...kkllllllllkk..."
    "..klllwwwwwwllllk."
    ".kllwwwwwwwwwwwllk"
    ".klwwwwwwwwwwwwwlk"
    "klwwkkkwwwwkkkwwlk"
    "klwkkrkkwwkkrkkwlk"
    "klwkkkkkwwkkkkkwlk"
    "klwwkkkwwwwkkkwwlk"
    "klwwwwwwkkwwwwwwlk"
    ".klwwwwkkkkwwwwlk."
    "..klwwwwwwwwwwlk.."
    "...klwkwkwkwkwlk.."
    "...klwkwkwkwkwlk.."
    "....klllllllllk..."
    ".....kkkkkkkkk...."
    ".................."
    "..................";
/* the Lobber: 24 x 36 */
static const char LOBBER[] =
    "........kkkkkkkk........"
    "......kkqqqqqqqqkk......"
    ".....kqqqqqqqqqqqqk....."
    "....kqqjjjjjjjjjjqqk...."
    "....kqjjwwjjjjwwjjqk...."
    "...kqqjjwkjjjjwkjjqqk..."
    "...kqqjjjjjjjjjjjjqqk..."
    "...kqqjjjkkkkkkjjjqqk..."
    "...kqqjjkrrrrrrkjjqqk..."
    "....kqqjjkkkkkkjjqqk...."
    ".....kqqqqqqqqqqqqk....."
    "......kkkqqqqqqkkk......"
    "....kkfffkkkkkkfffkk...."
    "..kkffffffffffffffffkk.."
    ".kfffjjfffffffffjjfffk.."
    "kffjjjjfffffffffjjjjffk."
    "kffjjfffffffffffffjjffk."
    "kfffffkfffffffffkffffffk"
    ".kfffkkffffffffffkkfffk."
    "..kkk.kfffffffffffk.kkk."
    "......kfffffffffffk....."
    "......kffffkkffffffk...."
    "......kfffk..kffffk....."
    ".....kfffk....kfffk....."
    ".....kfffk....kfffk....."
    ".....kfffk....kfffk....."
    "....kffffk....kffffk...."
    "....kkkkkk....kkkkkk...."
    "........................"
    "........................"
    "........................"
    "........................"
    "........................"
    "........................"
    "........................"
    "........................";
/* the dozing tortoise: 16 x 12 */
static const char TORTOISE[] =
    "....kkkkkkk....."
    "...kfjfjfjfk...."
    "..kjfjfjfjfjk..."
    ".kfjfjfjfjfjfk.."
    ".kjfjfjfjfjfjkkk"
    "kfjfjfjfjfjfjkzk"
    "kkkkkkkkkkkkkkzk"
    "kzzk.kzzk.kzzzkk"
    "kzzk.kzzk.kzzz.k"
    "kkk..kkk...kkk.."
    "................"
    "................";
/* a sexton: 14 x 24 */
static const char SEXTON[] =
    "....kkkkkk...."
    "...kNNNNNNk..."
    "..kNNNNNNNNk.."
    "..kNcccccNk..."
    "..kNcwkwcNk..."
    "...kcccccck..."
    "...klllllk...."
    "..kNNlllNNk..."
    ".kNNNNlNNNNk.."
    ".kNNNNNNNNNNk."
    "kNNNcNNNNcNNNk"
    "kNNkcNNNNckNNk"
    "kNNkkNNNNkkNNk"
    ".kNNNNNNNNNNk."
    ".kNNNNNNNNNNk."
    ".kNNNNNNNNNNk."
    ".kNNNNNNNNNNk."
    ".kNNNNNNNNNNk."
    "..kNNNNNNNNk.."
    "..kNNNNNNNNk.."
    "..kNNNNNNNNk.."
    "..kkkkkkkkkk.."
    ".............."
    "..............";
/* a friendly station hand: 14 x 24 */
static const char VILLAGER[] =
    ".............."
    "....kkkkkk...."
    "...kooooook..."
    "..kooooooook.."
    "..kocccccok..."
    "..kcckcckck..."
    "...kcccccck..."
    "....kcckk....."
    "...kBBBBBBk..."
    "..kBBBBBBBBk.."
    ".kcBBBwBBBBck."
    ".kcBBBBBBBBck."
    "..kBBBBBBBBk.."
    "..kBBBBBBBBk.."
    "..kNNNNNNNNk.."
    "..kNNNkkNNNk.."
    "..kNNk..kNNk.."
    "..kNNk..kNNk.."
    "..kNNk..kNNk.."
    "..kbbk..kbbk.."
    ".kkkkk..kkkkk."
    ".............."
    ".............."
    "..............";
/* the shopkeeper: 14 x 24 */
static const char KEEPER[] =
    ".............."
    "...kkkkkkkk..."
    "..kaaaaaaaak.."
    "...kkkkkkkk..."
    "...kcccccck..."
    "...kckcckck..."
    "...kcccccck..."
    "...kchhhhck..."
    "..kVVVVVVVVk.."
    ".kVVwVVVVwVVk."
    ".kVVVVVVVVVVk."
    "kcVVVVVVVVVVck"
    "kcVVVVVVVVVVck"
    ".kVVVVVVVVVVk."
    "..kpppppppppk."
    "..kpppppppppk."
    "..kppk...kppk."
    "..kppk...kppk."
    "..kbbk...kbbk."
    ".kkkkk...kkkkk"
    ".............."
    ".............."
    ".............."
    "..............";
/* a map flitter: 10 x 8 */
static const char FLITTER[] =
    "k........k"
    "kdk....kdk"
    "kddkkkkddk"
    ".kdwkkwdk."
    ".kddkkddk."
    "..kddddk.."
    "...k..k..."
    "..........";
/* a grub: 18 x 8 */
static const char GRUB[] =
    "....kkkkkkkkkk...."
    "..kkttttttttttkk.."
    ".kttcttcttcttcttk."
    "kttcttcttcttcttkkk"
    "ktttttttttttttkwkk"
    ".kttttttttttttkkk."
    "..kkkkkkkkkkkkk..."
    "..................";
/* a latecomer: 12 x 12 */
static const char JELLY[] =
    "...kkkkkk..."
    "..kPPPPPPk.."
    ".kPKKPPPPPk."
    "kPKwkPPkwKPk"
    "kPPkkPPkkPPk"
    "kPPPPPPPPPPk"
    ".kPPkkkkPPk."
    ".kPk.kk.kPk."
    ".kPk.kPk.Pk."
    "..kPkkPk.k.."
    "...k..kPk..."
    "........k...";
/* a brute: 20 x 26 */
static const char BRUTE[] =
    "......kkkkkkkk......"
    "....kkIIIIIIIIkk...."
    "...kIIIIIIIIIIIIk..."
    "..kIIIkkIIIIkkIIIk.."
    "..kIIkwkIIIIkwkIIk.."
    "..kIIIIIIIIIIIIIIk.."
    "..kIIIIkkkkkkIIIIk.."
    "...kIIIkwwwwkIIIk..."
    "....kkIIIIIIIIkk...."
    "..kkuuuuuuuuuuuukk.."
    ".kuuuuuuuuuuuuuuuuk."
    "kuuIuuuuuuuuuuuuIuuk"
    "kuIIuuuuuuuuuuuuIIuk"
    "kuIIuuuuuuuuuuuuIIuk"
    "kuIkuuuuuuuuuuuukIuk"
    "kIIkuuuuuuuuuuuukIIk"
    "kIIkuuuuuuuuuuuukIIk"
    ".kk.kuuuuuuuuuuk.kk."
    "....kuuuuuuuuuuk...."
    "....kuuuukkuuuuk...."
    "....kuuuk..kuuuk...."
    "....kuuuk..kuuuk...."
    "....kuuuk..kuuuk...."
    "...kIIIIk..kIIIIk..."
    "...kkkkkk..kkkkkk..."
    "....................";
/* a stinger: 14 x 12 */
static const char STINGER[] =
    "..........kk.."
    ".........kaak."
    "..........kak."
    "...kkkkkkkkak."
    "..kaaakaaakk.."
    ".kaawaaaaaak.."
    "kaaakaaakaaak."
    "kaaaaaaaaaaak."
    ".kkkkkkkkkkk.."
    ".k.k.k.k.k.k.."
    "k.k.k.k.k.k.k."
    "..............";
/* a trooper: 12 x 18 */
static const char TROOPER[] =
    "...kkkkkk..."
    "..kssssssk.."
    "..ksrrrrsk.."
    "..ksssssk..."
    "...kkkkkk..."
    "..kgggggggk."
    ".kgggkgggggk"
    ".kggkkkkkkkk"
    ".kgggkgggk.."
    "..kgggggk..."
    "..ksssssk..."
    "..kssksssk.."
    "..ksk.ksk..."
    "..ksk.ksk..."
    "..ksk.ksk..."
    ".kssk.kssk.."
    ".kkkk.kkkk.."
    "............";
/* a cursed face: 20 x 20 */
static const char FACE[] =
    "......kkkkkkkk......"
    "....kkVVVVVVVVkk...."
    "...kVVVVVVVVVVVVk..."
    "..kVVVVVVVVVVVVVVk.."
    ".kVVkkkVVVVVVkkkVVk."
    ".kVkwwwkVVVVkwwwkVk."
    "kVVkwkwkVVVVkwkwkVVk"
    "kVVkwwwkVVVVkwwwkVVk"
    "kVVVkkkVVVVVVkkkVVVk"
    "kVVVVVVVVVVVVVVVVVVk"
    "kVVVVVkkkkkkkkVVVVVk"
    "kVVVVkrrrrrrrrkVVVVk"
    "kVVVVkrwkwkwkrkVVVVk"
    ".kVVVkrrrrrrrrkVVVk."
    ".kVVVVkkkkkkkkVVVVk."
    "..kVVVVVVVVVVVVVVk.."
    "...kVVVVVVVVVVVVk..."
    "....kkVVVVVVVVkk...."
    "......kkkkkkkk......"
    "....................";
/* a louse: 10 x 8 */
static const char LOUSE[] =
    "..kkkkkk.."
    ".kzzzzzzk."
    "kzwkzzzzzk"
    "kzkkzzzizk"
    "kzzzzzzzzk"
    ".kkkkkkkk."
    "k.k.k.k.k."
    "..........";

static void make(Sprite *s, int w, int h, const char *data) {
    if ((int)strlen(data) != w * h) {
        art_ok = false;
        fprintf(stderr, "clarion art: a %dx%d sprite has %d pixels\n", w, h, (int)strlen(data));
    }
    spr_make(s, w, h, data);
}

bool clc_art_ok(void) { return art_ok; }

void clc_art_load(void) {
    bhp_art_load(); /* (chime art too): the Clarion, the faces, Lady Hush */
    if (spr[CS_CLARY_S0].px) return;
    make(&spr[CS_CLARY_S0], 7, 9, CLARY_S0);
    make(&spr[CS_CLARY_S1], 7, 9, CLARY_S1);
    make(&spr[CS_CLARY_SJ], 7, 9, CLARY_SJ);
    make(&spr[CS_CLARY_B0], 12, 16, CLARY_B0);
    make(&spr[CS_CLARY_B1], 12, 16, CLARY_B1);
    make(&spr[CS_CLARY_BJ], 12, 16, CLARY_BJ);
    make(&spr[CS_CLARY_BC], 12, 16, CLARY_BC);
    make(&spr[CS_NOTE], 8, 10, NOTE);
    make(&spr[CS_TOCK], 16, 16, TOCK);
    make(&spr[CS_SKULL], 18, 18, SKULL);
    make(&spr[CS_LOBBER], 24, 36, LOBBER);
    make(&spr[CS_TORTOISE], 16, 12, TORTOISE);
    make(&spr[CS_SEXTON], 14, 24, SEXTON);
    make(&spr[CS_VILLAGER], 14, 24, VILLAGER);
    make(&spr[CS_KEEPER], 14, 24, KEEPER);
    make(&spr[CS_FLITTER], 10, 8, FLITTER);
    make(&spr[CS_GRUB], 18, 8, GRUB);
    make(&spr[CS_JELLY], 12, 12, JELLY);
    make(&spr[CS_BRUTE], 20, 26, BRUTE);
    make(&spr[CS_STINGER], 14, 12, STINGER);
    make(&spr[CS_TROOPER], 12, 18, TROOPER);
    make(&spr[CS_FACE], 20, 20, FACE);
    make(&spr[CS_LOUSE], 10, 8, LOUSE);
}

const Sprite *clc_sprite(int id) { return id >= 0 && id < CS_COUNT ? &spr[id] : NULL; }

/* ---- characters ------------------------------------------------------------------- */

void clc_draw_ship(int x, int y, int face, bool flame, int t) { chm_draw_ship(1, x, y, face, flame, 0, t); }

void clc_draw_clary_small(int x, int y, int face, int frame) {
    /* x the middle, y the feet */
    int id = frame == 2 ? CS_CLARY_SJ : frame == 1 ? CS_CLARY_S1 : CS_CLARY_S0;
    spr_draw(&spr[id], x - 3, y - 9, face < 0 ? SPR_FLIPX : 0);
}

void clc_draw_clary(int x, int y, int face, int frame, int aim) {
    /* frame: 0 stand, 1 step, 2 air, 3 crouch; aim 0 ahead, 1 up, 2 down */
    int id = frame == 3 ? CS_CLARY_BC : frame == 2 ? CS_CLARY_BJ : frame == 1 ? CS_CLARY_B1 : CS_CLARY_B0;
    spr_draw(&spr[id], x - 6, y - 16, face < 0 ? SPR_FLIPX : 0);
    /* her blaster */
    int gy = frame == 3 ? y - 5 : y - 9;
    if (aim == 1) { gfx_rect(x + face * 2 - 1, y - 20, 2, 5, C_SLATE); gfx_pset(x + face * 2, y - 21, C_LIGHT); }
    else if (aim == 2) { gfx_rect(x - 1, y - 5, 2, 5, C_SLATE); }
    else { gfx_rect(face > 0 ? x + 4 : x - 8, gy, 5, 2, C_SLATE); gfx_pset(face > 0 ? x + 8 : x - 8, gy, C_LIGHT); }
}

void clc_draw_face(int who, int x, int y, int scale) {
    switch (who) {
    case 0: chm_draw_face(1, x, y, scale); break;
    case 1: chm_draw_face(0, x, y, scale); break;
    case 2: bhp_draw_lady(x, y, scale); break;
    default: spr_draw_scaled(&spr[CS_TOCK], x, y, scale, 0); break;
    }
}

/* ---- items: little icons, no names ------------------------------------------------- */

static void frame_box(int x, int y, int c) {
    gfx_rect(x - 7, y - 7, 14, 14, C_INK);
    gfx_rectb(x - 7, y - 7, 14, 14, c);
}

void clc_draw_item(int item, int x, int y, int t) {
    /* x, y the middle */
    int c1 = C_WHITE;
    if (item < G_SIPHON + 1) c1 = C_CYAN;            /* the ship's */
    else if (item <= G_THIMBLE) c1 = C_PINK;          /* Clary's */
    else if (item <= G_DOWSER) c1 = C_LIME;           /* the maps' */
    else if (item <= G_PLATE) c1 = C_YELLOW;
    else c1 = C_LIGHT;
    frame_box(x, y, (t / 8) & 1 ? c1 : PAL_DARKER[c1]);
    switch (item) {
    case G_TWIN: gfx_line(x - 5, y - 3, x - 2, y + 3, C_WHITE); gfx_line(x + 5, y - 3, x + 2, y + 3, C_WHITE); gfx_rect(x - 1, y - 1, 2, 2, C_CYAN); break;
    case G_SEEKER: gfx_circ(x, y, 3, C_AMBER); gfx_pset(x + 4, y - 3, C_WHITE); gfx_pset(x + 5, y - 4, C_ORANGE); break;
    case G_BOUNCE: gfx_line(x - 5, y + 4, x - 1, y - 4, C_RED); gfx_line(x - 1, y - 4, x + 4, y + 4, C_RED); break;
    case G_SIPHON: gfx_rect(x - 3, y - 4, 6, 8, C_LEAF); gfx_rect(x - 1, y - 6, 2, 2, C_LIME); gfx_hline(x - 2, x + 1, y, C_WHITE); break;
    case G_SPIT: gfx_rect(x - 5, y - 1, 6, 3, C_SLATE); gfx_circ(x + 3, y, 1, C_YELLOW); break;
    case G_FEATHER: gfx_line(x - 4, y + 4, x + 4, y - 4, C_WHITE); gfx_line(x - 2, y + 3, x + 3, y - 2, C_ICE); gfx_pset(x - 3, y, C_ICE); break;
    case G_FAN: gfx_line(x - 4, y, x + 4, y, C_PINK); gfx_line(x - 4, y, x + 3, y - 4, C_PINK); gfx_line(x - 4, y, x + 3, y + 4, C_PINK); break;
    case G_GHOST: gfx_circ(x, y - 1, 3, C_ICE); gfx_rect(x - 3, y, 7, 3, C_ICE); gfx_pset(x - 1, y - 2, C_INK); gfx_pset(x + 1, y - 2, C_INK); break;
    case G_CREEPER: gfx_rect(x - 4, y + 1, 8, 3, C_ORANGE); gfx_pset(x - 3, y + 4, C_INK); gfx_pset(x + 2, y + 4, C_INK); gfx_pset(x + 3, y, C_YELLOW); break;
    case G_BIGBANG: gfx_circ(x, y, 4, C_RED); gfx_circ(x, y, 2, C_YELLOW); break;
    case G_THIMBLE: gfx_rect(x - 3, y - 2, 6, 6, C_LIGHT); gfx_hline(x - 2, x + 1, y - 3, C_LIGHT); gfx_pset(x - 1, y, C_GREY); gfx_pset(x + 1, y + 2, C_GREY); break;
    case G_CHARM: gfx_circb(x, y + 1, 3, C_PINK); gfx_rect(x - 1, y - 5, 2, 3, C_YELLOW); gfx_pset(x, y + 1, C_MAGENTA); break;
    case G_MAGNET: gfx_rect(x - 4, y - 4, 2, 7, C_RED); gfx_rect(x + 2, y - 4, 2, 7, C_RED); gfx_hline(x - 4, x + 3, y + 3, C_RED); gfx_rect(x - 4, y - 4, 2, 2, C_WHITE); gfx_rect(x + 2, y - 4, 2, 2, C_WHITE); break;
    case G_DOWSER: gfx_circb(x, y, 4, C_LIME); gfx_line(x, y, x + 3, y - 3, C_RED); gfx_pset(x, y, C_WHITE); break;
    case G_PURSE: gfx_circ(x, y + 1, 4, C_BROWN); gfx_rect(x - 1, y - 4, 2, 2, C_TAN); gfx_pset(x, y + 1, C_YELLOW); break;
    case G_PLATE: gfx_rect(x - 4, y - 4, 8, 8, C_GREY); gfx_rectb(x - 4, y - 4, 8, 8, C_LIGHT); gfx_vline(x, y - 3, y + 3, C_SLATE); break;
    case IT_HEARTPIN: gfx_circ(x - 2, y - 1, 2, C_RED); gfx_circ(x + 2, y - 1, 2, C_RED); gfx_line(x - 4, y, x, y + 4, C_RED); gfx_line(x + 4, y, x, y + 4, C_RED); gfx_rect(x - 2, y, 5, 3, C_RED); break;
    case IT_SPARETANK: gfx_rect(x - 3, y - 4, 7, 9, C_LEAF); gfx_rect(x - 1, y - 6, 3, 2, C_GREY); gfx_vline(x, y - 2, y + 2, C_WHITE); gfx_hline(x - 2, x + 2, y, C_WHITE); break;
    case IT_TOFFEE: gfx_rect(x - 3, y - 2, 7, 5, C_AMBER); gfx_line(x - 6, y - 2, x - 4, y, C_CREAM); gfx_line(x + 6, y - 2, x + 4, y, C_CREAM); break;
    case IT_FLASK: gfx_rect(x - 2, y - 2, 5, 6, C_LIME); gfx_rect(x - 1, y - 5, 3, 3, C_GREY); break;
    case IT_DRUM: gfx_rect(x - 4, y - 4, 9, 9, C_LEAF); gfx_hline(x - 4, x + 4, y - 1, C_FOREST); gfx_hline(x - 4, x + 4, y + 2, C_FOREST); break;
    case IT_SACK: gfx_circ(x, y + 1, 4, C_TAN); text_draw(GLYPH_COIN, x - 3, y - 3, C_YELLOW); break;
    case IT_KEY: gfx_circb(x - 2, y, 2, C_YELLOW); gfx_hline(x, x + 4, y, C_YELLOW); gfx_vline(x + 3, y, y + 2, C_YELLOW); break;
    case IT_SHEET: gfx_rect(x - 4, y - 5, 8, 10, C_CREAM); for (int k = 0; k < 4; k++) gfx_hline(x - 3, x + 2, y - 3 + k * 2, C_BROWN); break;
    default: break;
    }
}

/* ---- sprites for the drawing code ----------------------------------------------------- */

void clc_spr(int id, int x, int y, int flags) {
    if (id >= 0 && id < CS_COUNT) spr_draw(&spr[id], x, y, flags);
}

void clc_spr_ex(int id, int x, int y, int flags, const uint8_t *map) {
    if (id >= 0 && id < CS_COUNT) spr_draw_ex(&spr[id], x, y, flags, map, -1);
}

int clc_spr_w(int id) { return id >= 0 && id < CS_COUNT ? spr[id].w : 0; }
int clc_spr_h(int id) { return id >= 0 && id < CS_COUNT ? spr[id].h : 0; }
