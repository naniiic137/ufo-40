/* ANTIPODE - pixel art (palette-letter strings, see gfx.h). All drawn for
 * UFO 40: the AP-2 suit in navy with a pink visor, the station's machines,
 * the pick-ups, the cradle and the terminal. The round things (star-eyes,
 * orbs, spike balls), the bosses and the tiles are drawn in apd_draw.c. */
#include "apd.h"

Sprite apd_spr[SPR_COUNT];

/* the suit, 12 x 20, facing right */
#define SUIT_HEAD \
    "...kkkkk...." \
    "..kNBBBBk..." \
    ".kNBBBBBBk.." \
    ".kNBKKKKKk.." \
    ".kNBKwKKKk.." \
    ".kNBBBBBBk.." \
    "..kNNNNNk..."
#define SUIT_BODY \
    ".kNBBBBBBkk." \
    "kNBBuBBBBggk" \
    "kNBBuBBBBkkk" \
    "kNBBBBBBBk.." \
    ".kNBBBBBNk.." \
    ".kNNNNNNNk.."
static const char SUIT0[] = SUIT_HEAD SUIT_BODY
    "..kBBkBBk..."
    "..kBBkBBk..."
    "..kBNkBNk..."
    "..kNNkNNk..."
    ".kBBk.kBBk.."
    ".kNNk.kNNk.."
    ".kkkk.kkkk..";
static const char SUIT1[] = SUIT_HEAD SUIT_BODY
    "..kBBkBBk..."
    ".kBBk.kBBk.."
    ".kBNk..kBNk."
    "kNNk....kNNk"
    "kBBk....kBBk"
    "kNNk....kNNk"
    "kkkk....kkkk";
static const char SUIT2[] = SUIT_HEAD SUIT_BODY
    "..kBBkBBk..."
    "...kBBBk...."
    "...kBNBk...."
    "...kNNNk...."
    "..kBBkBBk..."
    "..kNNkNNk..."
    "..kkkkkkk...";
static const char SUIT_JUMP[] = SUIT_HEAD SUIT_BODY
    "..kBBBBBk..."
    ".kBBNNNBBk.."
    ".kNNk.kNNk.."
    ".kkk...kkk.."
    "............"
    "............"
    "............";
static const char SUIT_DUCK[] =
    "............"
    "............"
    "............"
    "............"
    "............"
    "............"
    "...kkkkk...."
    "..kNBBBBk..."
    ".kNBKKKKKk.."
    ".kNBKwKKKk.."
    ".kNBBBBBBk.."
    ".kNBBBBBBkk."
    "kNBBuBBBBggk"
    "kNBBuBBBBkkk"
    "kNBBBBBBBk.."
    ".kNNNNNNNk.."
    "kBBkBBBkBBk."
    "kNNkNNNkNNk."
    "kNNkkkkkNNk."
    "kkkk...kkkk.";
static const char SUIT_UP[] =
    "...kkkkk.kk."
    "..kNBBBBkggk"
    ".kNBBBBBBkgk"
    ".kNBKKKKKkBk"
    ".kNBKwKKKkBk"
    ".kNBBBBBBkBk"
    "..kNNNNNkBk."
    ".kNBBBBBBBk."
    "kNBBuBBBBBk."
    "kNBBuBBBBk.."
    "kNBBBBBBBk.."
    ".kNBBBBBNk.."
    ".kNNNNNNNk.."
    "..kBBkBBk..."
    "..kBBkBBk..."
    "..kBNkBNk..."
    "..kNNkNNk..."
    ".kBBk.kBBk.."
    ".kNNk.kNNk.."
    ".kkkk.kkkk..";

/* a treadle, 14 x 12 */
#define TREAD_TOP \
    "....kkkkkk...." \
    "...kllllllk..." \
    "..klgrrrrglk.." \
    "..klgrwwrglk.." \
    "..kgggggggggk." \
    ".kssssssssssk." \
    "kddddddddddddk"
static const char TREAD0[] = TREAD_TOP
    "kdkkdkkdkkdkdk"
    "kdgkgkgkgkgkdk"
    "kddddddddddddk"
    ".kkkkkkkkkkkk."
    "..............";
static const char TREAD1[] = TREAD_TOP
    "kkdkkdkkdkkdkk"
    "kgkgkgkgkgkgdk"
    "kddddddddddddk"
    ".kkkkkkkkkkkk."
    "..............";

/* a spitter turret, 14 x 10 */
static const char SPITTER[] =
    "......kk......"
    ".....kggk....."
    "....kgrrgk...."
    "...kgrwwrgk..."
    "..kgggggggggk."
    ".kssssssssssk."
    "kssgssssssgssk"
    "kddddddddddddk"
    "kdkdkdkdkdkdkk"
    ".kkkkkkkkkkkk.";

/* a hivebox, 18 x 14 */
static const char HIVE[] =
    "..kkkkkkkkkkkkkk.."
    ".kBBBBBBBBBBBBBBk."
    "kBuuuuuuuuuuuuuuBk"
    "kBukkkukkkukkkuuBk"
    "kBukNkukNkukNkuuBk"
    "kBukkkukkkukkkuuBk"
    "kBuuuuuuuuuuuuuuBk"
    "kBNNNNNNNNNNNNNNBk"
    "kBukkkukkkukkkuuBk"
    "kBukNkukNkukNkuuBk"
    "kBukkkukkkukkkuuBk"
    "kBuuuuuuuuuuuuuuBk"
    ".kBBBBBBBBBBBBBBk."
    "..kkkkkkkkkkkkkk..";

/* a skipjack, 12 x 12 */
static const char SKIP[] =
    "...kkkkkk..."
    "..kVVVVVVk.."
    ".kVPPwPPPVk."
    ".kVPPkPPPVk."
    ".kVVVVVVVVk."
    "..kpppppppk."
    "..kkkkkkkk.."
    "...kg..gk..."
    "..kg....gk.."
    "...kg..gk..."
    "..kg....gk.."
    ".kkk....kkk.";

/* a skimmer, 14 x 8 */
static const char SKIMMER[] =
    ".....kkkk....."
    "....kCCCCk...."
    "...kCwCCCCk..."
    ".kkkkkkkkkkkk."
    "kqqqqqqqqqqqqk"
    "kqkqqkqqkqqkqk"
    ".kkkkkkkkkkkk."
    "...k.k..k.k...";

/* pick-ups, 12 x 12 */
static const char ARMOR[] =
    "....kkkk...."
    "...kCCCCk..."
    "..kCIIICCk.."
    ".kCIwICCCCk."
    "kCCIICCCCCCk"
    "kCCCCCuuCCCk"
    "kCCCCCuuCCCk"
    "kCCCCuuuuCCk"
    ".kCCCCCCCCk."
    "..kCCCCCCk.."
    "...kCCCCk..."
    "....kkkk....";
static const char SPARE[] =
    "...kkkkkk..."
    "..kNBBBBBk.."
    ".kNBBBBBBBk."
    ".kNBKKKKKBk."
    ".kNBKwKKKBk."
    ".kNBKKKKKBk."
    ".kNBBBBBBBk."
    "..kNNNNNNk.."
    ".kBBBBBBBBk."
    "kBBBBuuBBBBk"
    "kNNNNNNNNNNk"
    "kkkkkkkkkkkk";
static const char DAMPER[] =
    "..kkkkkkkk.."
    "..kssssssk.."
    "...kyyyyk..."
    "..kyakkayk.."
    "...kyyyyk..."
    "..kyakkayk.."
    "...kyyyyk..."
    "..kyakkayk.."
    "...kyyyyk..."
    "..kssssssk.."
    "..kkkkkkkk.."
    "............";
static const char MODCHIP[] =
    ".kkkkkkkkkk."
    "kggggggggggk"
    "kgoooooooogk"
    "kgoaaaaaaogk"
    "kgoayyyyaogk"
    "kgoaywwyaogk"
    "kgoaywwyaogk"
    "kgoayyyyaogk"
    "kgoaaaaaaogk"
    "kgoooooooogk"
    "kggggggggggk"
    ".kgkgkgkgkk.";

/* the cradle, 24 x 32 */
#define GLASS1 "..kpVkIICCCCCCCCCCkVpk.."
#define GLASS2 "..kpVkICCCCCCCCCCCkVpk.."
#define GLASS3 "..kpVkCCCCCCCCCCCCkVpk.."
static const char POD[] =
    "....kkkkkkkkkkkkkkkk...."
    "...kpVVVVVVVVVVVVVVpk..."
    "..kpVVkkkkkkkkkkkkVVpk.."
    GLASS1 GLASS2 GLASS3 GLASS3 GLASS1 GLASS2 GLASS3 GLASS3 GLASS1 GLASS2 GLASS3 GLASS3
    GLASS1 GLASS2 GLASS3 GLASS3 GLASS1 GLASS2 GLASS3 GLASS3 GLASS1 GLASS2 GLASS3 GLASS3
    "..kpVVkkkkkkkkkkkkVVpk.."
    ".kppppppppppppppppppppk."
    "kpVVVVVVVVVVVVVVVVVVVVpk"
    "kpVPVPVPVPVPVPVPVPVPVVpk"
    "kkkkkkkkkkkkkkkkkkkkkkkk";

/* a terminal, 14 x 16 */
static const char TERM[] =
    "kkkkkkkkkkkkkk"
    "kssssssssssssk"
    "kskkkkkkkkkksk"
    "kskjjjjjjjjksk"
    "kskjzzjjjjjksk"
    "kskjjjzzzjjksk"
    "kskjzzzjjjjksk"
    "kskkkkkkkkkksk"
    "kssssssssssssk"
    "kkkkkkkkkkkkkk"
    "...kgggggggk.."
    "..kgsgsgsgsgk."
    "..kgggggggggk."
    "....kssssk...."
    "....kssssk...."
    "..kkkkkkkkkk..";

static const struct { int id, w, h; const char *px; } ART[] = {
    {SPR_SUIT0, 12, 20, SUIT0},     {SPR_SUIT1, 12, 20, SUIT1},   {SPR_SUIT2, 12, 20, SUIT2},
    {SPR_SUIT_JUMP, 12, 20, SUIT_JUMP}, {SPR_SUIT_DUCK, 12, 20, SUIT_DUCK}, {SPR_SUIT_UP, 12, 20, SUIT_UP},
    {SPR_TREAD0, 14, 12, TREAD0},   {SPR_TREAD1, 14, 12, TREAD1}, {SPR_SPITTER, 14, 10, SPITTER},
    {SPR_HIVE, 18, 14, HIVE},       {SPR_SKIP, 12, 12, SKIP},     {SPR_SKIMMER, 14, 8, SKIMMER},
    {SPR_ARMOR, 12, 12, ARMOR},     {SPR_SPARE, 12, 12, SPARE},   {SPR_DAMPER, 12, 12, DAMPER},
    {SPR_MOD, 12, 12, MODCHIP},     {SPR_POD, 24, 32, POD},       {SPR_TERM, 14, 16, TERM},
};

int apd_art_check(void) {
    /* tests: every sprite string has exactly w * h pixels */
    int bad = 0;
    for (int i = 0; i < ARRAY_LEN(ART); i++)
        if ((int)strlen(ART[i].px) != ART[i].w * ART[i].h) {
            bad++;
            fprintf(stderr, "apd art %d: %d pixels, want %d\n", ART[i].id, (int)strlen(ART[i].px), ART[i].w * ART[i].h);
        }
    return bad;
}

void apd_art_load(void) {
    static bool done;
    if (done) return;
    done = true;
    for (int i = 0; i < ARRAY_LEN(ART); i++)
        if ((int)strlen(ART[i].px) == ART[i].w * ART[i].h) spr_make(&apd_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
}
