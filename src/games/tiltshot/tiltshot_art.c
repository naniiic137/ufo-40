/* TILTSHOT - pixel art (palette-letter strings, see gfx.h): the golfers'
 * heads and bodies, the junk and the orange movers, the flag and the
 * trophy. Golfers are a head per golfer on a shared body, the arms and club
 * drawn live so the swing can follow the meter. */
#include "tiltshot.h"

Sprite tsh_spr[TS_COUNT];
static Sprite heads[TSH_GOLFERS], body_spr;

/* heads, 8 x 8, facing right; P is the colourway's accent */
static const char HEAD_NOVA[] =
    "..kkkk.."
    ".kPPPPk."
    ".kPPPPPk"
    "kbkkkkkk"
    "kbhhhhk."
    "kbhhkhk."
    ".khhhhk."
    "..kkkk..";
static const char HEAD_DIGBY[] =
    "...kPk.."
    "....k..."
    ".kkkkkkk"
    ".kllllgk"
    ".klCCCCk"
    ".kllllgk"
    ".kgkgkgk"
    ".kkkkkkk";
static const char HEAD_PEACHES[] =
    ".kPk.kPk"
    "..k...k."
    ".kKKKKKk"
    "kKKKKKKk"
    "kKkKkKkk"
    "kKKKKKKk"
    ".kKKvKk."
    "..kkkk..";
static const char HEAD_TUCK[] =
    "..kkkk.."
    ".knnnnk."
    "knnwwnk."
    "knwkwwk."
    "knwwwwoo"
    "knwwwwk."
    ".knwwk.."
    "..kkk...";
static const char HEAD_MOSS[] =
    ".kk..kk."
    "kwkkkwkk"
    "kkzzzkzk"
    "kzzzzzzk"
    "kzzzzzzk"
    "kzkkkkzk"
    ".kzzzzk."
    "..kkkk..";
static const char HEAD_WICK[] =
    "..kkkk.."
    ".kcccck."
    "kcccccck"
    "kccuuuuk"
    "kcuwuuuk"
    "kccuuuck"
    ".kcccck."
    "..kkkk..";
static const char HEAD_KIP[] =
    "..kkkk.."
    ".kbbbbk."
    "kbbbbbbk"
    "kbhhhhbk"
    "kkIIkIIk"
    "khhhhhhk"
    ".khhhhk."
    "..kkkk..";

/* the body, 8 x 8: r is the shirt (the colourway), N the trousers */
static const char BODY[] =
    "..krrk.."
    ".krrrrk."
    ".krrrrk."
    ".krrrrk."
    "..kNNk.."
    "..kNNk.."
    "..kNkNk."
    ".kkk.kk.";

/* colourways: shirt, accent */
static const uint8_t COAT[TSH_GOLFERS][2][2] = {
    {{C_RED, C_RED}, {C_BLUE, C_SKY}},          /* Nova */
    {{C_TEAL, C_RED}, {C_ORANGE, C_YELLOW}},    /* Digby */
    {{C_VIOLET, C_YELLOW}, {C_JADE, C_PINK}},   /* Peaches */
    {{C_RED, C_RED}, {C_FOREST, C_LIME}},       /* Tuck */
    {{C_BROWN, C_AMBER}, {C_NAVY, C_BLUE}},     /* Moss */
    {{C_ORANGE, C_RED}, {C_SLATE, C_RED}},      /* Wick */
    {{C_BLUE, C_ORANGE}, {C_WINE, C_ORANGE}},   /* Kip */
};
static const char *const COAT_NAME[TSH_GOLFERS][2] = {
    {"RED", "BLUE"}, {"TEAL", "ORANGE"}, {"VIOLET", "JADE"}, {"RED", "GREEN"},
    {"BROWN", "NAVY"}, {"ORANGE", "SLATE"}, {"BLUE", "WINE"},
};

const char *tsh_coat_name(int golfer, int coat) { return COAT_NAME[iclamp(golfer, 0, TSH_GOLFERS - 1)][coat & 1]; }

/* junk (purple) */
static const char CRATE[] =
    "kkkkkkkk"
    "kpVVVVpk"
    "kVpVVpVk"
    "kVVppVVk"
    "kVVppVVk"
    "kVpVVpVk"
    "kpVVVVpk"
    "kkkkkkkk";
static const char CONE[] =
    "...kk..."
    "..kVVk.."
    "..kVVk.."
    "..kwwk.."
    ".kVVVVk."
    ".kwwwwk."
    "kVVVVVVk"
    "kkkkkkkk";
static const char CHURN[] =
    ".kkkkkk."
    "kVVVVVPk"
    "kppppppk"
    "kVVVVVPk"
    "kVVVVVPk"
    "kppppppk"
    "kVVVVVPk"
    ".kkkkkk.";
static const char BUCKET[] =
    ".cyccyc."
    "cycyccyc"
    "kwVwVwVk"
    "kwVwVwVk"
    ".kVwVwk."
    ".kVwVwk."
    ".kVwVwk."
    "..kkkk..";

/* movers (orange) */
static const char BLIMP1[] =
    "...kkkkkkk.."
    ".kkoooooaak."
    "kaoooooooook"
    "koooooooooak"
    ".kaaooooakk."
    "k.kkkkkkk..."
    "k...kaak...."
    "....kkkk....";
static const char BLIMP2[] =
    "...kkkkkkk.."
    ".kkoooooaak."
    ".aoooooooook"
    "koooooooooak"
    "k.kaooooakk."
    "..kkkkkkk..."
    "....kaak...."
    "....kkkk....";
static const char HOPPER1[] =
    "..kkkkk..."
    ".koooook.."
    "kowkowok.."
    "kooooook.."
    ".kaaaak..."
    "..kggk...."
    ".kgkkgk..."
    "..kggk...."
    ".kgkkgk..."
    "kkk..kkk..";
static const char HOPPER2[] =
    "..kkkkk..."
    ".koooook.."
    "kowkowok.."
    "kooooook.."
    ".kaaaak..."
    ".kgggggk.."
    "kkk...kkk.";
static const char KITE1[] =
    "....k...."
    "...kyk..."
    "..kyyok.."
    ".kyyooak."
    "kooooaaak"
    ".koaaark."
    "..karrk.."
    "...krk..."
    "....k....";
static const char FISH1[] =
    "...kkkk..."
    "kkkooook.."
    "kaoowooak."
    "koooooooak"
    "kkaaooook."
    "...kkkk...";
static const char FISH2[] =
    "...kkkk..."
    "..kooook.."
    "kaoowooak."
    "koooooooak"
    "kaaaooook."
    "kk.kkkk...";
static const char SPARK1[] =
    "....k....."
    ".k..y..k.."
    "..kyyyk..."
    ".kyooooyk."
    "kyooaaooyk"
    ".kyoaaoyk."
    "..kyooyk.."
    ".k..yy..k."
    "....k.....";
static const char SPARK2[] =
    "k...k...k."
    "...yyy...."
    "..kyooyk.."
    ".yooaaooy."
    "kyoaaaaoyk"
    ".yooaaooy."
    "..kyooyk.."
    "...yyy...."
    "k...k...k.";

/* a trundler: patrols a floor on two wheels */
static const char ROLLER1[] =
    "...kkkkkk..."
    "..koooooak.."
    ".kowwkooook."
    ".kowkkooaak."
    ".koooooooak."
    "..kaaaaaak.."
    ".kkkkkkkkkk."
    "kglgk..kglgk"
    "kgkgk..kgkgk"
    ".kkk....kkk.";
static const char ROLLER2[] =
    "...kkkkkk..."
    "..koooooak.."
    ".kowwkooook."
    ".kowkkooaak."
    ".koooooooak."
    "..kaaaaaak.."
    ".kkkkkkkkkk."
    "kgkgk..kgkgk"
    "kglgk..kglgk"
    ".kkk....kkk.";
/* a lantern on a chain */
static const char LANTERN[] =
    "...kkk..."
    "..kgggk.."
    ".kkkkkkk."
    "kooyyyook"
    "koyyyyyok"
    "koyywyyok"
    "kooyyyook"
    ".kkkkkkk."
    "..kaaak.."
    "...kkk...";

static const char FLAG1[] =
    "rrrrrr"
    "rrrrrm"
    "rrrrm."
    "rrm...";
static const char FLAG2[] =
    "rrrrm."
    "rrrrrr"
    "rrrrrm"
    "rrm...";
static const char TROPHY[] =
    "....kkkkkkkk...."
    "..kkyyyyyyyykk.."
    ".kyk.yyyyyyy.kyk"
    ".kyk.yywyyyy.kyk"
    ".kyk.yywyyyy.kyk"
    "..kkyyyyyyyykk.."
    "....kaayyyakk..."
    ".....kaayak....."
    "......kayk......"
    "......kayk......"
    ".....kkaakk....."
    "....kaaaaaak...."
    "...knnnnnnnnk..."
    "...knyyyyyynk..."
    "...knnnnnnnnk..."
    "...kkkkkkkkkk...";
static const char COMET[] =
    "..........wwkk.."
    ".....IICCwwwwk.."
    "IICCCCuuwwwwwwk."
    "..IICCuuwwwwwwk."
    ".....IICCwwwwk.."
    "..........kkk...";

void tsh_art_load(void) {
    if (body_spr.px) return;
    spr_make(&body_spr, 8, 8, BODY);
    static const char *const H[TSH_GOLFERS] = {HEAD_NOVA, HEAD_DIGBY, HEAD_PEACHES, HEAD_TUCK, HEAD_MOSS, HEAD_WICK, HEAD_KIP};
    for (int i = 0; i < TSH_GOLFERS; i++) spr_make(&heads[i], 8, 8, H[i]);
    spr_make(&tsh_spr[TS_CRATE], 8, 8, CRATE);
    spr_make(&tsh_spr[TS_CONE], 8, 8, CONE);
    spr_make(&tsh_spr[TS_CHURN], 8, 8, CHURN);
    spr_make(&tsh_spr[TS_BUCKET], 8, 8, BUCKET);
    spr_make(&tsh_spr[TS_BLIMP1], 12, 8, BLIMP1);
    spr_make(&tsh_spr[TS_BLIMP2], 12, 8, BLIMP2);
    spr_make(&tsh_spr[TS_HOPPER1], 10, 10, HOPPER1);
    spr_make(&tsh_spr[TS_HOPPER2], 10, 7, HOPPER2);
    spr_make(&tsh_spr[TS_KITE1], 9, 9, KITE1);
    spr_make(&tsh_spr[TS_KITE2], 9, 9, KITE1);
    spr_make(&tsh_spr[TS_FISH1], 10, 6, FISH1);
    spr_make(&tsh_spr[TS_FISH2], 10, 6, FISH2);
    spr_make(&tsh_spr[TS_SPARK1], 10, 9, SPARK1);
    spr_make(&tsh_spr[TS_SPARK2], 10, 9, SPARK2);
    spr_make(&tsh_spr[TS_ROLLER1], 12, 10, ROLLER1);
    spr_make(&tsh_spr[TS_ROLLER2], 12, 10, ROLLER2);
    spr_make(&tsh_spr[TS_LANTERN], 9, 10, LANTERN);
    spr_make(&tsh_spr[TS_FLAG1], 6, 4, FLAG1);
    spr_make(&tsh_spr[TS_FLAG2], 6, 4, FLAG2);
    spr_make(&tsh_spr[TS_TROPHY], 16, 16, TROPHY);
    spr_make(&tsh_spr[TS_COMET], 16, 6, COMET);
}

static void coat_map(int golfer, int coat, uint8_t *map, bool red) {
    pal_identity(map);
    golfer = iclamp(golfer, 0, TSH_GOLFERS - 1);
    map[C_RED] = COAT[golfer][coat & 1][0];
    map[C_MAGENTA] = COAT[golfer][coat & 1][1];
    if (red)
        for (int i = 0; i < PAL_COUNT; i++)
            if (i != C_INK) map[i] = (i == C_WHITE || i == C_CREAM || i == C_YELLOW || i == C_LIGHT) ? C_PINK : C_RED;
}

void tsh_draw_head(int golfer, int coat, int x, int y) {
    uint8_t map[PAL_COUNT];
    coat_map(golfer, coat, map, false);
    spr_draw_ex(&heads[iclamp(golfer, 0, TSH_GOLFERS - 1)], x, y, 0, map, -1);
}

/* feet at (x, y); the pose sets the club: STAND at address, BACK1-3 the
 * backswing, FOLLOW after the swing, CHEER arms up */
void tsh_draw_golfer(int golfer, int coat, int pose, int x, int y, bool flip, bool red) {
    uint8_t map[PAL_COUNT];
    coat_map(golfer, coat, map, red);
    int fl = flip ? SPR_FLIPX : 0, s = flip ? -1 : 1;
    spr_draw_ex(&body_spr, x - 4, y - 8, fl, map, -1);
    spr_draw_ex(&heads[iclamp(golfer, 0, TSH_GOLFERS - 1)], x - 4, y - 15, fl, map, -1);
    /* arms and club, in degrees: 0 = the club hangs straight down */
    static const int16_t ANG[GP_POSES] = {35, -60, -120, -165, 150, 180};
    int a = ANG[iclamp(pose, 0, GP_POSES - 1)];
    float r = a * 3.14159265f / 180.0f;
    int shx = x + s * 1, shy = y - 7;
    int hx = shx + (int)lroundf(s * sinf(r * 0.55f) * 4), hy = shy + (int)lroundf(cosf(r * 0.55f) * 4);
    int arm = red ? C_PINK : (golfer == 1 ? C_GREY : golfer == 2 ? C_PINK : golfer == 3 ? C_NIGHT : golfer == 4 ? C_LEAF : C_HIDE);
    if (pose == GP_CHEER) {
        gfx_line(x - 2 * s, y - 7, x - 4 * s, y - 13, arm);
        gfx_line(x + 2 * s, y - 7, x + 4 * s, y - 13, arm);
        gfx_line(x + 4 * s, y - 13, x + 7 * s, y - 20, C_LIGHT);
        return;
    }
    gfx_line(shx, shy, hx, hy, arm);
    int cx = hx + (int)lroundf(s * sinf(r) * 9), cy = hy + (int)lroundf(cosf(r) * 9);
    gfx_line(hx, hy, cx, cy, red ? C_PINK : C_LIGHT);
    gfx_rect(cx - (flip ? 1 : 0), cy - 1, 2, 2, red ? C_RED : C_GREY);
}
