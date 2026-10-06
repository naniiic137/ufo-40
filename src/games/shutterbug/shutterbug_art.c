/* SHUTTERBUG - pixel art (palette-letter strings, see gfx.h). All drawn for
 * UFO 40. The bosses, comets, lasers, letters and scenery are drawn in
 * shutterbug_draw.c. */
#include "shutterbug.h"

Sprite shb_spr[SP_COUNT];

/* Poppy, the puffer-blimp with a camera for a nose, 18 x 11. The same
 * drawing is recoloured for her bare look, and for Sprig. */
static const char SHIP[] =
    "......kkkkkkk....."
    "....kkrrrrrrrkk..."
    "k..krrrrrrrrwwrk.."
    "kk.krroorrrrwkwrkk"
    "korkrroorrrrwwwrks"
    "korkrrrrrrrrrrrrkI"
    "kk.krrrrrrrrrrrrks"
    "k...krrrrrrrrrrk.."
    ".....kkrrrrrrkk..."
    ".......kkkkkk....."
    ".........kk.......";

static const char PUFF[] =
    "...l.w.l...."
    ".w.lwwwl.w.."
    "..lwwIwwwl.."
    "wlwwIIIwwlw."
    ".lwwIwIwwl.."
    "..lwwwwwl.w."
    ".w..lwl....."
    "....ltl....."
    ".....t......"
    ".....t......";
static const char MINT[] =
    "...kkkkk..."
    "..kiwiwik.."
    ".kwizwziwk."
    "kiwzwiwzwik"
    "kwzwkwkwzwk"
    "kizwwiwwzik"
    "kwzwkkkwzwk"
    "kiwzwiwzwik"
    ".kwizwziwk."
    "..kiwiwik.."
    "...kkkkk...";
static const char TURRET[] =
    "....kkkk...."
    "...kllllk..."
    "..klwlllgk.."
    "kkklllllgkkk"
    "kslllllllgsk"
    "kssgggggggsk"
    ".kssssssssk."
    "..kkkkkkkk..";
static const char WALLBOMB[] =
    "..kkkkkk.."
    ".kPKKKKPk."
    "kPKwKKwKPk"
    "kPKkKKkKPk"
    "kPKKKKKKPk"
    "kPKKPPKKPk"
    "kPPKKKKPPk"
    ".kPPPPPPk."
    "..kkkkkk.."
    ".k.k..k.k.";
static const char CHOMPER[] =
    ".....kk....."
    "....kVVk...."
    "..kkVppVkk.."
    ".kVpppppVVk."
    "kVpwkppwkpVk"
    "kVppppppppVk"
    "kVpkwkwkwpVk"
    "kVpk.....pVk"
    "kVpkwkwkwpVk"
    ".kVppppppVk."
    "..kkVVVVkk.."
    "....kkkk....";
static const char CUBE[] =
    "kkkkkkkkkk"
    "kwwwwwwwlk"
    "kwcwwwwclk"
    "kwkwwwwklk"
    "kwwwwwwwlk"
    "kwwkwwkwlk"
    "kwwwkkwwlk"
    "kwwwwwwwlk"
    "kllllllllk"
    "kkkkkkkkkk";
static const char TOAST[] =
    "..kkkkkkkk.."
    ".kbbbbbbbbk."
    "kbttttttttbk"
    "kbtaaaaaatbk"
    "kbtakaakatbk"
    "kbtaaaaaatbk"
    "kbtaakkaatbk"
    "kbtaaaaaatbk"
    "kbtaaaaaatbk"
    "kbttttttttbk"
    ".kbbbbbbbbk."
    "..kk....kk..";
static const char SWIRL[] =
    "..kkkkkk.."
    ".kPwPPwPk."
    "kPwPwwPwPk"
    "kwPwPPwPwk"
    "kPwPkkPwPk"
    "kwPwkkwPwk"
    "kPwPwwPwPk"
    "kwPPwwPPwk"
    ".kPwPPwPk."
    "..kkkkkk..";
static const char ERUPTER[] =
    "...kkkkkk..."
    "..kuCCCCuk.."
    ".kuCIIIICuk."
    "kuCIwkIwkCuk"
    "kuCIIIIIICuk"
    "kuCCIkkICCuk"
    "kuuCCCCCCuuk"
    ".kuuCCCCuuk."
    "..kkuuuukk.."
    ".k.kk..kk.k."
    "k..k....k..k"
    "............";
static const char BURSTER[] =
    ".....k......"
    "..k.kKk.k..."
    "...kKPKk...."
    ".kkKPPPKkk.."
    "kKPPwPPwPPKk"
    ".kKPkPPkPKk."
    "kKPPPPPPPPKk"
    ".kkKPPPPKkk."
    "...kKPPKk..."
    "..k.kKKk.k.."
    ".....kk....."
    "............";
static const char DART[] =
    "....kkkkkk....."
    "kkkkggllllkkk.."
    "korrgglllllwwk."
    "korrgglllllwwwk"
    "kkkkggllllkkk.."
    "....kkkkkk.....";
static const char HOPPER[] =
    "...kkkkkk..."
    "..kyyyyyyk.."
    ".kyywyyyyak."
    "kyywwyyyyyak"
    "kyyyykyykyak"
    "kyyyyyyyyyak"
    "kyyyykkkyyak"
    "kayyyyyyyaak"
    ".kaayyyyaak."
    "..kaaaaaak.."
    "...kkkkkk..."
    "............";
static const char GHOST[] =
    "...kkkkkk..."
    "..kIwwwwIk.."
    ".kIwwwwwwIk."
    "kIwwkwwkwwIk"
    "kIwwkwwkwwIk"
    "kIwwwwwwwwIk"
    "kIwwwkkwwwIk"
    "kIwwwwwwwwIk"
    "kIwwwwwwwwIk"
    "kIwIwwIwwIIk"
    ".kIkIIkIIkk."
    "..k.kk.kk...";
static const char CART[] =
    ".kkkkkkkkkkkk."
    "kvvvvvvvvvvvvk"
    "kvmIIvvvvIImvk"
    "kvmIkvvvvIkmvk"
    "kvmmmmmmmmmmvk"
    "kvvvvvvvvvvvvk"
    "kkkkkkkkkkkkkk"
    ".kgk......kgk."
    "kgsgk....kgsgk"
    ".kgk......kgk.";
static const char RAILBOMB[] =
    "....kkkkkkkk...."
    "..kksggggggskk.."
    ".kssgllllllgssk."
    "kssglrwllrwlgssk"
    "kssglrkllrklgssk"
    "kssgllllllllgssk"
    "kssggllkkllggssk"
    ".kssgggggggggsk."
    "..kkkkkkkkkkkk.."
    ".kgk.kgk.kgk.kgk"
    "kgsgkgsgkgsgkgsg"
    ".kgk.kgk.kgk.kgk";
static const char SILO[] =
    "....kkkk...."
    "...kggggk..."
    "...kgkkgk..."
    "..kgsooskk.."
    "..kgsoosgk.."
    ".kgggsssggk."
    ".kgllllllgk."
    "kgllgggglllk"
    "kgllgkkglllk"
    "kggggggggggk"
    "kssssssssssk"
    "kkkkkkkkkkkk";
static const char ROCKET[] =
    ".kkkk."
    "kyooak"
    "kyooak"
    ".kkkk.";
static const char NIP[] =
    "......kkk."
    ".....kizik"
    "k...kizwkk"
    "kk.kiiiiik"
    ".kkiziziik"
    "..kiiiiik."
    "..kzkkzk.."
    "..kk..kk..";
static const char KITE[] =
    "k............."
    "kBk.......kk.."
    "kuBk.....kuuk."
    ".kuBkkkkkuwkk."
    "..kuuBBBuuuuyk"
    "...kkuBBuuukk."
    ".....kkkkkk..."
    "......k..k....";
static const char GEN[] =
    "..kkkkkkkkkkkk.."
    ".kssssssssssssk."
    "kslllllllllllsk."
    "ksllCCCCCCCCllsk"
    "kslCIIIIIIIICIsk"
    "kslCIwwwwwwICIsk"
    "kslCIIIIIIIICIsk"
    "ksllCCCCCCCCllsk"
    "kslllllllllllssk"
    "ksgggggggggggggk"
    "kssssssssssssssk"
    ".kgkgkgkgkgkgkk."
    "..kkkkkkkkkkkk.."
    "................";
static const char CRYSTAL[] =
    "..kk.."
    ".kKKk."
    "kKwKPk"
    "kKKPPk"
    "kKPPPk"
    ".kPPk."
    ".kssk."
    "..kk..";
static const char WRENCH[] =
    ".kk...kk."
    "kllk.kllk"
    "klk...klk"
    ".klkkklk."
    "..klllk.."
    "...klk..."
    "...klk..."
    "...kllk.."
    "....kk...";

/* a copy of src with each 'from' letter swapped for its 'to' */
static char *recolour(const char *src, const char *from, const char *to) {
    size_t n = strlen(src);
    char *s = malloc(n + 1);
    if (!s) return NULL;
    memcpy(s, src, n + 1);
    for (size_t i = 0; i < n; i++)
        for (size_t k = 0; from[k]; k++)
            if (s[i] == from[k]) { s[i] = to[k]; break; }
    return s;
}

static void make_swapped(int id, int w, int h, const char *src, const char *from, const char *to) {
    char *s = recolour(src, from, to);
    if (s) {
        spr_make(&shb_spr[id], w, h, s);
        free(s);
    }
}

void shb_art_load(void) {
    if (shb_spr[SP_POPPY].px) return;
    spr_make(&shb_spr[SP_POPPY], 18, 11, SHIP);
    make_swapped(SP_POPPY_BARE, 18, 11, SHIP, "ro", "Bu");   /* the armour gone: blue underneath */
    make_swapped(SP_SPRIG, 18, 11, SHIP, "ro", "zi");
    make_swapped(SP_SPRIG_BARE, 18, 11, SHIP, "ro", "ac");
    spr_make(&shb_spr[SP_PUFF], 12, 10, PUFF);
    spr_make(&shb_spr[SP_MINT], 11, 11, MINT);
    spr_make(&shb_spr[SP_TURRET], 12, 8, TURRET);
    make_swapped(SP_TURRET_RED, 12, 8, TURRET, "lwgs", "rovm");
    spr_make(&shb_spr[SP_WALLBOMB], 10, 10, WALLBOMB);
    spr_make(&shb_spr[SP_CHOMPER], 12, 12, CHOMPER);
    spr_make(&shb_spr[SP_CUBE], 10, 10, CUBE);
    make_swapped(SP_CUBE_RED, 10, 10, CUBE, "wcl", "rov");
    spr_make(&shb_spr[SP_TOAST], 12, 12, TOAST);
    spr_make(&shb_spr[SP_SWIRL], 10, 10, SWIRL);
    spr_make(&shb_spr[SP_ERUPTER], 12, 12, ERUPTER);
    spr_make(&shb_spr[SP_BURSTER], 12, 12, BURSTER);
    spr_make(&shb_spr[SP_DART], 15, 6, DART);
    spr_make(&shb_spr[SP_HOPPER], 12, 12, HOPPER);
    spr_make(&shb_spr[SP_GHOST], 12, 12, GHOST);
    spr_make(&shb_spr[SP_CART], 14, 10, CART);
    spr_make(&shb_spr[SP_RAILBOMB], 16, 12, RAILBOMB);
    make_swapped(SP_RAILBOMB_RED, 16, 12, RAILBOMB, "glsr", "orvy");
    spr_make(&shb_spr[SP_SILO], 12, 12, SILO);
    spr_make(&shb_spr[SP_ROCKET], 6, 4, ROCKET);
    spr_make(&shb_spr[SP_NIP], 10, 8, NIP);
    spr_make(&shb_spr[SP_KITE], 14, 8, KITE);
    spr_make(&shb_spr[SP_GEN], 16, 14, GEN);
    spr_make(&shb_spr[SP_CRYSTAL], 6, 8, CRYSTAL);
    spr_make(&shb_spr[SP_WRENCH], 9, 9, WRENCH);
}
