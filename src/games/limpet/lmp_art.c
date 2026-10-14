/* LIMPET - pixel art (palette-letter strings, see gfx.h). All drawn for
 * UFO 40: Limpet the hull tank in its four paints, the Turncoat, the
 * station's guns, the Gristle's creatures, honey, bees and the station
 * folk. The hull, the pods and the Dreadnought are drawn in lmp_draw.c. */
#include "lmp.h"

Sprite lmp_spr[SPR_COUNT];
Sprite lmp_tank_spr[5][8];
Sprite lmp_rot[SPR_COUNT][4];

/* Limpet, cannon up: teal hull, amber stripe, grey treads */
static const char TANK[] =
    "....kk...."
    "...kwlk..."
    "...kwlk..."
    "..kqqqqk.."
    ".kqjjjjqk."
    "kqqqaaqqqk"
    "kqqqqqqqqk"
    "kkkkkkkkkk"
    "kgkgkgkgkk"
    ".kkkkkkkk.";
/* on a ramp, cannon up and to the right */
static const char TANK_D[] =
    "........kk"
    ".......kwk"
    "...kk.kwk."
    "..kqqkwk.."
    ".kqjjqk..."
    "kqqjaqqk.."
    "kgqqqqqqk."
    ".kgqqqqk.."
    "..kgkqk..."
    "...kgk....";

static const char TURRET[] =
    "....kk...."
    "....kk...."
    "...kggk..."
    "..kslgsk.."
    ".ksslrssk."
    ".ksssssk.."
    "kssssssssk"
    "kddddddddk"
    "kkkkkkkkkk"
    "..........";
static const char LASER[] =
    "...kkkk..."
    "...kCuk..."
    "..kkuukk.."
    ".kssuussk."
    ".kslggssk."
    "ksssssssk."
    "kddddddddk"
    "kkkkkkkkkk"
    "..........";
static const char DRONE0[] =
    "k........k"
    "kgk....kgk"
    ".kVkkkkVk."
    "..kpVVpk.."
    ".kpPwwPpk."
    ".kppkkppk."
    "..kpppk..."
    "...kkk....";
static const char DRONE1[] =
    "..........";
static const char EYE0[] =
    "..kkkkk.."
    ".kooooak."
    "koowwwook"
    "kowNNwaok"
    "kowNkwaok"
    "koowwwook"
    "kooaaaook"
    ".koooook."
    "..kkkkk..";
static const char EYE1[] =
    "..kkkkk.."
    ".kooooak."
    "koooooook"
    "kkkkkkkkk"
    "koooooook"
    "kooaaaook"
    "kooooooak"
    ".koooook."
    "..kkkkk..";
static const char CRAWLER0[] =
    "..k....k.."
    "...k..k..."
    "..kKKKKk.."
    ".kKPKKPKk."
    "kKKKKKKKKk"
    "kPKvKKvKPk"
    "kKKKKKKKKk"
    ".kvkvvkvk."
    "k.k.k.k.k."
    "..........";
static const char CRAWLER1[] =
    "..k....k.."
    "..k....k.."
    "..kKKKKk.."
    ".kKPKKPKk."
    "kKKKKKKKKk"
    "kPKvKKvKPk"
    "kKKKKKKKKk"
    ".kvkvvkvk."
    ".k.k.k.k.k"
    "..........";
static const char SAC0[] =
    "...kkkk..."
    "..kPKKPk.."
    ".kPKwKKPk."
    "kPKKKKKKPk"
    "kPKKvvKKPk"
    "kPKvkkvKPk"
    "kPKKvvKKPk"
    ".kPPKKPPk."
    "kvvvvvvvvk"
    "kkkkkkkkkk";
static const char SAC1[] =
    "..kkkkkk.."
    ".kPKKKKPk."
    "kPKwKKKKPk"
    "kPKKKKKKPk"
    "kPKvvvvKPk"
    "kPKvkkvKPk"
    "kPKvvvvKPk"
    "kPPKKKKPPk"
    "kvvvvvvvvk"
    "kkkkkkkkkk";
static const char SPECK0[] =
    "k.kk.k"
    ".kKPk."
    "kKwKPk"
    "kKKKPk"
    ".kPPk."
    "..kk..";
static const char SPECK1[] =
    "..kk.."
    "kkKPkk"
    "kKwKPk"
    "kKKKPk"
    ".kPPk."
    "..kk..";
static const char WORM_HEAD[] =
    ".kkkkkk."
    "kgllllgk"
    "kgrkkrgk"
    "kggggggk"
    "kgsggsgk"
    "kgggggk."
    ".kkkkk.."
    "........";
static const char WORM_BODY[] =
    "........"
    ".kkkkkk."
    "kgsgsgsk"
    "ksgsgsgk"
    "kgsgsgsk"
    ".kkkkkk."
    "........"
    "........";
static const char BALL[] =
    "....k...."
    "..k.k.k.."
    "...kgk..."
    ".kkglgkk."
    "kkglwlgkk"
    ".kkgsgkk."
    "...ksk..."
    "..k.k.k.."
    "....k....";
static const char NOZZLE_SQ[] =
    "...kkkk..."
    "...kook..."
    "..kssssk.."
    "..kslgsk.."
    ".ksssssskk"
    "kddoooddk."
    "kdddddddk."
    "kddddddddk"
    "kkkkkkkkkk"
    "..........";
static const char HONEY[] =
    ".kkkkkk."
    ".kwwwwk."
    "kkkkkkkk"
    "kaywaaak"
    "kaywoaak"
    "kayaoaak"
    "kaaaooak"
    "koaaaaok"
    ".kkkkkk.";
static const char BEE[] =
    ".wk.w"
    "kykyk"
    "kykyk"
    ".kkk.";
static const char POD[] = "k";
/* the station folk, 8 x 12 */
static const char NPC0[] = /* Rolo, the railway man: cap, overalls */
    "..kkkk.."
    ".kBBBBk."
    "kBBBBBBk"
    ".kttttk."
    ".ktktkt."
    ".kttttk."
    "..kttk.."
    ".kBlBBk."
    "kBBlBBBk"
    "kBBBBBBk"
    ".kBkkBk."
    ".kkk.kkk";
static const char NPC1[] = /* Mother Brindle, the cook */
    "..kwwk.."
    ".kwwwwk."
    ".kwwwwk."
    ".khhhhk."
    ".khkhkh."
    ".khhhhk."
    "..khhk.."
    ".kwwwwk."
    "kwwrwwwk"
    "kwwwwwwk"
    ".kwkkwk."
    ".kkk.kkk";
static const char NPC2[] = /* Gus, the crane man */
    "..kkkk.."
    ".kyyyyk."
    "kyyyyyyk"
    ".kbbbbk."
    ".kbkbkb."
    ".kbbbbk."
    "..kbbk.."
    ".koyooko"
    "koooyook"
    "kooooook"
    ".kokkok."
    ".kkk.kkk";
static const char NPC3[] = /* Wren, the beekeeper: a veiled hat */
    "kkkkkkkk"
    ".kccccck"
    ".kcgcgck"
    ".kcccck."
    ".ktktkt."
    ".kttttk."
    "..kttk.."
    ".kccccck"
    "kcccacck"
    "kcccccck"
    ".kckkck."
    ".kkk.kkk";
static const char NPC4[] = /* the night clerk */
    "..kkkk.."
    ".kssssk."
    ".kssssk."
    ".klllk.."
    ".klklk.."
    ".kllllk."
    "..kllk.."
    ".kNNNNk."
    "kNNwNNNk"
    "kNNwNNNk"
    ".kNkkNk."
    ".kkk.kkk";

typedef struct { int id, w, h; const char *px; } Art;
static const Art ART[] = {
    {SPR_TANK, 10, 10, TANK}, {SPR_TANK_D, 10, 10, TANK_D},
    {SPR_TURRET, 10, 10, TURRET}, {SPR_LASER, 10, 9, LASER}, {SPR_DRONE0, 10, 8, DRONE0},
    {SPR_DRONE1, 10, 1, DRONE1}, {SPR_EYE0, 9, 9, EYE0}, {SPR_EYE1, 9, 9, EYE1},
    {SPR_CRAWLER0, 10, 10, CRAWLER0}, {SPR_CRAWLER1, 10, 10, CRAWLER1}, {SPR_SAC0, 10, 10, SAC0},
    {SPR_SAC1, 10, 10, SAC1}, {SPR_SPECK0, 6, 6, SPECK0}, {SPR_SPECK1, 6, 6, SPECK1},
    {SPR_WORM_HEAD, 8, 8, WORM_HEAD}, {SPR_WORM_BODY, 8, 8, WORM_BODY}, {SPR_BALL, 9, 9, BALL},
    {SPR_NOZZLE, 10, 10, NOZZLE_SQ}, {SPR_HONEY, 8, 9, HONEY}, {SPR_BEE, 5, 4, BEE}, {SPR_POD, 1, 1, POD},
    {SPR_NPC0, 8, 12, NPC0}, {SPR_NPC1, 8, 12, NPC1}, {SPR_NPC2, 8, 12, NPC2}, {SPR_NPC3, 8, 12, NPC3},
    {SPR_NPC4, 8, 12, NPC4}, {SPR_TURNCOAT, 10, 10, TANK},
};

int lmp_art_check(void) {
    /* tests: every sprite string has exactly w * h pixels */
    int bad = 0;
    for (int i = 0; i < ARRAY_LEN(ART); i++)
        if ((int)strlen(ART[i].px) != ART[i].w * ART[i].h) {
            bad++;
            fprintf(stderr, "lmp art %d: %d chars for %dx%d\n", ART[i].id, (int)strlen(ART[i].px), ART[i].w, ART[i].h);
        }
    return bad;
}

/* turn a square picture a quarter clockwise, r times */
static void rotate(char *dst, const char *src, int n, int r) {
    char a[400], b[400];
    memcpy(a, src, (size_t)(n * n));
    for (int k = 0; k < r; k++) {
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++) b[y * n + x] = a[(n - 1 - x) * n + y];
        memcpy(a, b, (size_t)(n * n));
    }
    memcpy(dst, a, (size_t)(n * n));
    dst[n * n] = 0;
}

/* the four paints (and the Turncoat's): hull, highlight, stripe */
static const char PAINT[5][3] = {
    {'q', 'j', 'a'}, /* station teal */
    {'P', 'K', 'w'}, /* the canteen's pink */
    {'f', 'z', 'y'}, /* the gristle wing's green */
    {'r', 'o', 'y'}, /* the upper ring's red */
    {'v', 'm', 'r'}, /* the Turncoat */
};

static void repaint(char *s, int p) {
    for (char *c = s; *c; c++) {
        if (*c == 'q') *c = PAINT[p][0];
        else if (*c == 'j') *c = PAINT[p][1];
        else if (*c == 'a') *c = PAINT[p][2];
    }
}

void lmp_art_load(void) {
    static bool done;
    if (done) return;
    done = true;
    for (int i = 0; i < ARRAY_LEN(ART); i++)
        if ((int)strlen(ART[i].px) == ART[i].w * ART[i].h) spr_make(&lmp_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
    char buf[401];
    /* the tank: axis normals from the upright picture, diagonals from the slanted one */
    for (int p = 0; p < 5; p++)
        for (int n = 0; n < 8; n++) {
            rotate(buf, lmp_axis(n) ? TANK : TANK_D, 10, n / 2);
            repaint(buf, p);
            spr_make(&lmp_tank_spr[p][n], 10, 10, buf);
        }
    for (int i = 0; i < ARRAY_LEN(ART); i++) {
        if (ART[i].w != ART[i].h || ART[i].w > 20) continue;
        if ((int)strlen(ART[i].px) != ART[i].w * ART[i].h) continue;
        for (int r = 0; r < 4; r++) {
            rotate(buf, ART[i].px, ART[i].w, r);
            spr_make(&lmp_rot[ART[i].id][r], ART[i].w, ART[i].w, buf);
        }
    }
}
