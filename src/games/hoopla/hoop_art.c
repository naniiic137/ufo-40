/* HOOPLA - pixel art (palette-letter strings, see gfx.h), all drawn for
 * UFO 40. Each fighter is 12 x 16: a top half of its own and one of three
 * pairs of legs. Clothes are drawn in stand-in colours (P main, p its
 * shade, K trim) that hoop_remap turns into the fighter's first or second
 * colours. */
#include "hoop.h"

Sprite hoop_spr[HSP_COUNT];

#define TOP_ROWS 11
#define LEG_ROWS 5

static const char LEGS_STAND[] =
    "...kPPPPk..."
    "...kPkkPk..."
    "...kP..Pk..."
    "...kk..kk..."
    "..kkk..kkk..";
static const char LEGS_STEP[] =
    "...kPPPPk..."
    "..kPkkkPk..."
    "..kP...Pk..."
    ".kk.....kk.."
    ".kkk....kkk.";
static const char LEGS_AIR[] =
    "...kPPPPk..."
    "..kPPkkPPk.."
    "..kkk..kkk.."
    "............"
    "............";

/* the eight tops: standing, then with the weapon out */
static const char *const TOPS[HOOP_FIGHTERS][2] = {
    { /* TANSY, a juggler in a tasselled cap */
     "....kkk....."
     "...kPPPkkk.."
     "..kPPPPPkKk."
     "..kyyyyyk.k."
     "..kyttkttk.."
     "...ktttttk.."
     "....kkkkk..."
     "...kPPKPPk.."
     "..ktkPKPktk."
     "...kPPPPPk.."
     "...kKKKKk...",
     "....kkk....."
     "...kPPPkkk.."
     "..kPPPPPkKk."
     "..kyyyyyk.k."
     "..kyttkttk.."
     "...ktttttk.."
     "....kkkkk..."
     "...kPPKPkttl"
     "..ktkPKPk..."
     "...kPPPPPk.."
     "...kKKKKk..."},
    { /* CLAMP, a dock crane with one eye */
     "......k....."
     ".....kgk...."
     "..kkkkkkkk.."
     "..kPPPPPPk.."
     "..kPkwwkPk.."
     "..kPPPPPPk.."
     "..kkkKKkkk.."
     ".kgkPPPPkgk."
     ".kgkPKKPkgk."
     ".kk.kPPk.kk."
     "....kKKk....",
     "......k....."
     ".....kgk...."
     "..kkkkkkkk.."
     "..kPPPPPPk.."
     "..kPkwwkPk.."
     "..kPPPPPPk.."
     "..kkkKKkkk.."
     ".kgkPPPPkggk"
     ".kk.kPKKPk.."
     "....kPPk...."
     "....kKKk...."},
    { /* MOSS, a shaggy cave hermit */
     "...kkkkkk..."
     "..kPPPPPPk.."
     ".kPPPPPPPPk."
     ".kPKKKKKKPk."
     ".kPKkKKkKPk."
     ".kPPKKKKPPk."
     ".kPPPPPPPPk."
     "kPPpPPPPpPPk"
     "kPkpPPPPpkPk"
     "kk.kPPPPk.kk"
     "...kpppPk...",
     "...kkkkkk..."
     "..kPPPPPPk.."
     ".kPPPPPPPPk."
     ".kPKKKKKKPk."
     ".kPKkKKkKPk."
     ".kPPKKKKPPk."
     ".kPPPPPPPPk."
     "kPPpPPPPpggg"
     "kPkpPPPPpkk."
     "kk.kPPPPk..."
     "...kpppPk..."},
    { /* BRISTLE, a badger who sets traps */
     "...k....k..."
     "..kPk..kPk.."
     "..kPkkkkPk.."
     "..kKkKKkKk.."
     "..kKKKKKKk.."
     "...kKkkKk..."
     "....kkkk...."
     "..kPPPPPPk.."
     ".kPkPPPPkPk."
     ".kk.PPPP.kk."
     "....kPPk....",
     "...k....k..."
     "..kPk..kPk.."
     "..kPkkkkPk.."
     "..kKkKKkKk.."
     "..kKKKKKKk.."
     "...kKkkKk..."
     "....kkkk...."
     "..kPPPPPkhww"
     ".kPkPPPPk..."
     ".kk.PPPP...."
     "....kPPk...."},
    { /* PEWIT, a lapwing girl with a crest and wings */
     "......kk...."
     ".....kPk...."
     "....knnnk..."
     "...knnnnnk.."
     "...kntktk..."
     "...kttttk..."
     "....kkkk...."
     "kKKkkPPkkKKk"
     ".kKKkPPkKKk."
     "..kk.PPPk..."
     "....kKKk....",
     "......kk...."
     ".....kPk...."
     "....knnnk..."
     "...knnnnnk.."
     "...kntktk..."
     "...kttttk..."
     "....kkkk...."
     "kKKkkPPkktg."
     ".kKKkPPk...."
     "..kk.PPPk..."
     "....kKKk...."},
    { /* COLLIER, a miner with a lamp on his helmet */
     "....kkkk...."
     "...kPPPPk..."
     "..kPPKKPPk.."
     "..kkkkkkkk.."
     "...kttkttk.."
     "...ktttttk.."
     "...keeeeek.."
     "..kPPKKPPk.."
     ".ktkPPPPktk."
     ".kk.PPPP.kk."
     "...kPPPPk...",
     "....kkkk...."
     "...kPPPPk..."
     "..kPPKKPPk.."
     "..kkkkkkkk.."
     "...kttkttk.."
     "...ktttttk.."
     "...keeeeek.."
     "..kPPKKPPkok"
     ".ktkPPPPk..."
     ".kk.PPPP...."
     "...kPPPPk..."},
    { /* ASTRA, a champion from another star, pack on her back */
     "..k......k.."
     "...k....k..."
     "...kkkkkk..."
     "..kKKKKKKk.."
     "..kKwKKwKk.."
     "..kKKKKKKk.."
     "...kkkkkk..."
     ".kgkPPPPk..."
     ".kgkPKKPktk."
     ".kk.PPPP.kk."
     "...kPPPPk...",
     "..k......k.."
     "...k....k..."
     "...kkkkkk..."
     "..kKKKKKKk.."
     "..kKwKKwKk.."
     "..kKKKKKKk.."
     "...kkkkkk..."
     ".kgkPPPPkttK"
     ".kgkPKKPk..."
     ".kk.PPPP...."
     "...kPPPPk..."},
    { /* GULP, a bullfrog */
     "..kkk..kkk.."
     ".kwwkkkkwwk."
     ".kwkPPPPkwk."
     ".kPPPPPPPPk."
     ".kPPPPPPPPk."
     ".kPkkkkkkPk."
     "..kKKKKKKk.."
     ".kPPKKKKPPk."
     "kPPkKKKKkPPk"
     "kk.kPPPPk.kk"
     "...kPPPPk...",
     "..kkk..kkk.."
     ".kwwkkkkwwk."
     ".kwkPPPPkwk."
     ".kPPPPPPPPk."
     ".kPPPPPPPPk."
     ".kPkkkkkkPk."
     "..kKKKKKKk.."
     ".kPPKKKKPkgl"
     "kPPkKKKKk..."
     "kk.kPPPPk..."
     "...kPPPPk..."},
};

static const char KNIFE[] =
    "......k."
    "bbkllllw"
    "......k.";
static const char COG[] =
    "..k..k.."
    ".kggggk."
    "kggkkggk"
    ".gk..kg."
    ".gk..kg."
    "kggkkggk"
    ".kggggk."
    "..k..k..";
static const char COG2[] =
    ".k.gg.k."
    "kgggggg."
    ".gkkkkg."
    "ggk..kgg"
    "ggk..kgg"
    ".gkkkkg."
    ".ggggggk"
    ".k.gg.k.";
static const char ROCKET[] =
    "..kkkk.."
    "okggggwk"
    "okggggwk"
    "..kkkk..";
static const char QUILL[] =
    "...w"
    "..w."
    ".w.."
    "k...";
static const char SHOE[] =
    ".kkkkk."
    "kgk.kgk"
    "kgk.kgk"
    "kgk.kgk"
    "kgk.kgk"
    "kk...kk";
static const char BOMB[] =
    "...oy."
    "..kk.."
    ".knnk."
    "knnngk"
    "knnnnk"
    ".kkkk.";
static const char DART[] =
    "k......"
    "rkggggw"
    "k......";
static const char MINE[] =
    "..kkkk.."
    ".kggggk."
    "kgkgkgkg"
    "kkkkkkkk";
static const char MINE_ARMED[] =
    "..krrk.."
    ".kggggk."
    "kgkgkgkg"
    "kkkkkkkk";
static const char CLAW[] =
    "k....k"
    "gk..kg"
    ".gkkg."
    "..gg.."
    "..kk.."
    "......";
static const char RING[] =
    "...kkkk..."
    "..kyyyyk.."
    ".kyk..kyk."
    "kyk....kyk"
    "kyk....kyk"
    "kyk....kyk"
    "kyk....kyk"
    ".kyk..kyk."
    "..kyyyyk.."
    "...kkkk...";
static const char RING2[] =
    "...kkkk..."
    "..kwyyyk.."
    ".kwk..kyk."
    "kwk....kyk"
    "kyk....kak"
    "kyk....kak"
    "kyk....kak"
    ".kyk..kak."
    "..kaaaak.."
    "...kkkk...";
static const char FIRE1[] =
    "..r...o...r."
    ".ror.oyo.ro."
    "royyoyyyoyor"
    ".oyyyyyyyyo."
    "..oyy..yyo.."
    "...o....o...";
static const char FIRE2[] =
    "....r...o..."
    "..o.oro.oyo."
    ".royoyyoyyor"
    "royyyyyyyyo."
    ".oyyo..oyyo."
    "..o......o..";
static const char STAR[] =
    "..y.."
    ".yyy."
    "yyyyy"
    ".yyy."
    "..y..";
static const char CAGE[] =
    "kgkkkkkkkkkkgk"
    "kggggggggggggk"
    "k............k";

void hoop_art_load(void) {
    static bool done;
    if (done) return;
    done = true;
    char buf[12 * 16 + 1];
    for (int k = 0; k < HOOP_FIGHTERS; k++) {
        for (int fr = 0; fr < HSP_FRAMES; fr++) {
            const char *top = TOPS[k][fr == HSP_ACT ? 1 : 0];
            const char *legs = fr == HSP_STEP ? LEGS_STEP : fr == HSP_AIR ? LEGS_AIR : LEGS_STAND;
            memcpy(buf, top, 12 * TOP_ROWS);
            memcpy(buf + 12 * TOP_ROWS, legs, 12 * LEG_ROWS);
            buf[12 * 16] = 0;
            spr_make(&hoop_spr[HSP_FIGHTER(k, fr)], 12, 16, buf);
        }
    }
    spr_make(&hoop_spr[HSP_KNIFE], 8, 3, KNIFE);
    spr_make(&hoop_spr[HSP_COG], 8, 8, COG);
    spr_make(&hoop_spr[HSP_COG2], 8, 8, COG2);
    spr_make(&hoop_spr[HSP_ROCKET], 8, 4, ROCKET);
    spr_make(&hoop_spr[HSP_QUILL], 4, 4, QUILL);
    spr_make(&hoop_spr[HSP_SHOE], 7, 6, SHOE);
    spr_make(&hoop_spr[HSP_BOMB], 6, 6, BOMB);
    spr_make(&hoop_spr[HSP_DART], 7, 3, DART);
    spr_make(&hoop_spr[HSP_MINE], 8, 4, MINE);
    spr_make(&hoop_spr[HSP_MINE_ARMED], 8, 4, MINE_ARMED);
    spr_make(&hoop_spr[HSP_CLAW], 6, 6, CLAW);
    spr_make(&hoop_spr[HSP_RING], 10, 10, RING);
    spr_make(&hoop_spr[HSP_RING2], 10, 10, RING2);
    spr_make(&hoop_spr[HSP_FIRE1], 12, 6, FIRE1);
    spr_make(&hoop_spr[HSP_FIRE2], 12, 6, FIRE2);
    spr_make(&hoop_spr[HSP_STAR], 5, 5, STAR);
    spr_make(&hoop_spr[HSP_CAGE], 14, 3, CAGE);
}

/* tests: sprites whose strings aren't the size they claim (should be 0) */
int hoop_art_check(void) {
    int bad = 0;
    for (int k = 0; k < HOOP_FIGHTERS; k++)
        for (int f = 0; f < 2; f++) bad += strlen(TOPS[k][f]) != 12 * TOP_ROWS;
    bad += strlen(LEGS_STAND) != 12 * LEG_ROWS;
    bad += strlen(LEGS_STEP) != 12 * LEG_ROWS;
    bad += strlen(LEGS_AIR) != 12 * LEG_ROWS;
    bad += strlen(RING) != 100 || strlen(RING2) != 100 || strlen(COG) != 64 || strlen(COG2) != 64;
    bad += strlen(FIRE1) != 72 || strlen(FIRE2) != 72 || strlen(CAGE) != 42 || strlen(MINE) != 32;
    return bad;
}

/* the stand-in colours to a fighter's first (pal 0) or second (pal 1) ones */
void hoop_remap(int kind, int pal, uint8_t *map) {
    const HoopFighterDef *d = &HOOP_DEF[iclamp(kind, 0, HOOP_FIGHTERS - 1)];
    int main = pal ? d->alt_main : d->col_main, trim = pal ? d->alt_trim : d->col_trim;
    pal_identity(map);
    map[C_MAGENTA] = (uint8_t)main;
    map[C_PURPLE] = PAL_DARKER[main];
    map[C_PINK] = (uint8_t)trim;
}
