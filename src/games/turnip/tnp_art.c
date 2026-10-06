/* TURNIP TRUCK - pixel art (palette-letter strings, see gfx.h), all drawn
 * for UFO 40. Vehicles face east and are turned at draw time. The city's
 * cells, the beet, the saucers and the weather are drawn in tnp_draw.c. */
#include "tnp.h"

Sprite tnp_spr[SP_COUNT];

/* Granny Root's truck: the cargo box with a turnip on the roof, the cab in front */
static const char TRUCK[] =
    "..kk......kk...."
    ".kkkkkkkkkkkkkk."
    ".kwwwwwwwwkVVVkk"
    ".kwwwjzwwwkVCCVk"
    ".kwwPVVPwwkVCCVk"
    ".kwwPVVPwwkVCCVk"
    ".kwwwwwwwwkVCCVk"
    ".kwwwwwwwwkVVVkk"
    ".kkkkkkkkkkkkkk."
    "..kk......kk....";

/* a town car: r = body (recoloured), v = its roof (a lighter shade) */
static const char CAR[] =
    ".kk......kk.."
    "krrrrrrrrrrrk"
    "krCrvvvvrCrry"
    "krCrvvvvrCrrk"
    "krCrvvvvrCrry"
    "krrrrrrrrrrrk"
    ".kk......kk..";

/* the Radish Ring's getaway car: black, with a radish on the bonnet */
static const char GANG[] =
    ".kk......kk.."
    "knnnnnnnnnzzk"
    "kndndddnndKKy"
    "kndndddnndKKk"
    "kndndddnndKKy"
    "knnnnnnnnnnnk"
    ".kk......kk..";

/* the police: white, a light bar (r and u swap as it flashes) */
static const char POLICE[] =
    ".kk......kk.."
    "kwwwwwwwwwwwk"
    "kwCwNrrNwCwwy"
    "kwCwNNNNwCwwk"
    "kwCwNuuNwCwwy"
    "kwwwwwwwwwwwk"
    ".kk......kk..";

static const char MUSH1[] =
    ".eeeee."
    "ecceece"
    "eeeceee"
    ".khhkh."
    "..hhh.."
    ".hhhhh."
    "..h.h.."
    ".kk.kk.";
static const char MUSH2[] =
    ".eeeee."
    "ecceece"
    "eeeceee"
    ".hkhhk."
    "..hhh.."
    ".hhhhh."
    ".h...h."
    "kk...kk";

/* a time crate: a clock painted on it */
static const char CRATE[] =
    "kkkkkkkkkk"
    "keeeeeeeek"
    "keewwwweek"
    "kewwkwwwek"
    "kewwkwwwek"
    "kewwkkwwek"
    "kewwwwwwek"
    "keewwwweek"
    "keeeeeeeek"
    "kkkkkkkkkk";

static const char BALLOON[] =
    "..rrr.."
    ".rwrrr."
    "rwrrrrr"
    "rrrrrrr"
    ".rrrrr."
    "..rrr.."
    "...k..."
    "...k..."
    "...k...";

/* one gas canister from above */
static const char CANS[] =
    "..kkkk.."
    ".kaoook."
    "kaoyyook"
    "koykkyok"
    "koykkyok"
    "kooyyook"
    ".kooook."
    "..kkkk..";

static const char HEART[] =
    ".rr.rr."
    "rwrrrrr"
    "rrrrrrr"
    ".rrrrr."
    "..rrr.."
    "...r...";
static const char HEART_OFF[] =
    ".dd.dd."
    "dsdddd."
    "ddddddd"
    ".ddddd."
    "..ddd.."
    "...d...";

/* Zib, the driver: one big eye, two antennae, Granny Root's cap badge */
static const char ZIB[] =
    "...k........k..."
    "..kKk......kKk.."
    "...k........k..."
    "....k......k...."
    ".....kkkkkk....."
    "...kkVVVVVVkk..."
    "..kVVVwwwwVVVk.."
    "..kVVwwwwwwVVk.."
    ".kVVVwwkkwwVVVk."
    ".kVVVwwkkwwVVVk."
    ".kVVVVwwwwVVVVk."
    ".kVVVVVVVVVVVVk."
    "..kVVkkkkkkVVk.."
    "..kVVVPPPPVVVk.."
    "...kkVVVVVVkk..."
    ".....kkkkkk.....";

/* Glenda Glorp of Channel 9 */
static const char ANCHOR[] =
    "......kkkk......"
    ".....kKKKKk....."
    "....kKKKKKKk...."
    "...kkkkkkkkkk..."
    "..kzzzzzzzzzzk.."
    "..kzwwzzzzwwzk.."
    "..kzwkzzzzwkzk.."
    "..kzzzzzzzzzzk.."
    "..kzzzzrrzzzzk.."
    "...kzzzzzzzzk..."
    "....kkzzzzkk...."
    "...kBBkzzkBBk..."
    "..kBBBBkkBBBBk.."
    ".kBBBBBwwBBBBBk."
    ".kBBBBBwwBBBBBk."
    ".kkkkkkkkkkkkkk.";

static const char TURNIP[] =
    "..z...z.."
    "...zjz..."
    "....j...."
    "..kkkkk.."
    ".kVPVVVk."
    "kVVVVVVVk"
    "kwwwwwwwk"
    ".kwwwwwk."
    "..kwwwk.."
    "....k....";

void tnp_art_load(void) {
    static bool done;
    if (done) return;
    done = true;
    spr_make(&tnp_spr[SP_TRUCK], 16, 10, TRUCK);
    spr_make(&tnp_spr[SP_CAR], 13, 7, CAR);
    spr_make(&tnp_spr[SP_GANG], 13, 7, GANG);
    spr_make(&tnp_spr[SP_POLICE], 13, 7, POLICE);
    spr_make(&tnp_spr[SP_MUSH1], 7, 8, MUSH1);
    spr_make(&tnp_spr[SP_MUSH2], 7, 8, MUSH2);
    spr_make(&tnp_spr[SP_CRATE], 10, 10, CRATE);
    spr_make(&tnp_spr[SP_BALLOON], 7, 9, BALLOON);
    spr_make(&tnp_spr[SP_CANS], 8, 8, CANS);
    spr_make(&tnp_spr[SP_HEART], 7, 6, HEART);
    spr_make(&tnp_spr[SP_HEART_OFF], 7, 6, HEART_OFF);
    spr_make(&tnp_spr[SP_ZIB], 16, 16, ZIB);
    spr_make(&tnp_spr[SP_ANCHOR], 16, 16, ANCHOR);
    spr_make(&tnp_spr[SP_TURNIP], 9, 10, TURNIP);
}

/* A sprite turned by ang (0 = as drawn, facing east) about its middle at cx, cy. */
void tnp_draw_rot(const Sprite *s, float cx, float cy, float ang, const uint8_t *remap, int solid) {
    float ca = tnp_cos(ang), sa = tnp_sin(ang);
    float hw = s->w * 0.5f, hh = s->h * 0.5f;
    int r = (int)ceilf(sqrtf(hw * hw + hh * hh)) + 1;
    int x0 = (int)floorf(cx) - r, y0 = (int)floorf(cy) - r;
    for (int y = y0; y <= y0 + 2 * r; y++)
        for (int x = x0; x <= x0 + 2 * r; x++) {
            float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy;
            float sx = dx * ca + dy * sa + hw, sy = -dx * sa + dy * ca + hh;
            if (sx < 0 || sy < 0) continue;
            int ix = (int)sx, iy = (int)sy;
            if (ix >= s->w || iy >= s->h) continue;
            uint8_t c = s->px[iy * s->w + ix];
            if (c == TRANSPARENT) continue;
            gfx_pset(x, y, solid >= 0 ? solid : remap ? remap[c] : c);
        }
}
