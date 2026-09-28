/* WET PAINT - pixel art (palette-letter strings, see gfx.h). All drawn for
 * UFO 40. Cars and foes are drawn facing right; the other three headings are
 * made here by turning the drawings. */
#include "wetpaint.h"

Sprite wp_car[WS_CAR_COUNT][4];
Sprite wp_spr[WS_COUNT];

/* Bo's blue kart: wheels, the body, Bo's yellow helmet and goggles */
static const char BO[] =
    ".kkk..kkk."
    ".kkkBBkkk."
    "kBBBBBBBuk"
    "kBuyyBBBBk"
    "kBywwyBBIk"
    "kBywwyBBIk"
    "kBuyyBBBBk"
    "kBBBBBBBuk"
    ".kkkBBkkk."
    ".kkk..kkk.";
/* Foxy Fuchsia's hot rod: the same size, ears on the helmet */
static const char FOXY[] =
    ".kkk..kkk."
    ".kkkPPkkk."
    "kPPPPPPPKk"
    "kPoooPPPPk"
    "okowwoPPIk"
    "okowwoPPIk"
    "kPoooPPPPk"
    "kPPPPPPPKk"
    ".kkkPPkkk."
    ".kkk..kkk.";
/* a smudger: a pink kart pushing a paint roller */
static const char ROLLER[] =
    "..kk.kk..."
    "..kk.kk.kk"
    ".kKKKKKkgk"
    "kKPPPKKkPk"
    "kKPwkPKkPk"
    "kKPwkPKkPk"
    "kKPPPKKkPk"
    ".kKKKKKkgk"
    "..kk.kk.kk"
    "..kk.kk...";
/* a sloshing paint tanker: a round tank behind a small cab */
static const char TANKER[] =
    ".kkkkkkk.."
    "kPKKKKKPkk"
    "kKPPPPPKkI"
    "kKPKKKPKkI"
    "kKPKKKPKkk"
    "kKPKKKPKkk"
    "kKPPPPPKkI"
    "kPKKKKKPkI"
    ".kkkkkkkkk"
    "..k..k.k..";
/* a little gloop */
static const char GLOOP[] =
    ".........."
    "...kkkk..."
    "..kKKKKk.."
    ".kKPPKKKk."
    ".kPwkPwkk."
    ".kPwkPwkPk"
    ".kPPPPPPk."
    "..kPPPPk.."
    "...kkkk..."
    "..........";
/* the mother gloop, white and quick */
static const char BIGGLOOP[] =
    "...kkkk..."
    "..kwwwwk.."
    ".kwwlwwwk."
    "kwwwwwwwwk"
    "kwlwkwwkwk"
    "kwlwkwwkwk"
    "kwwwwwwwwk"
    ".kwlllwwk."
    "..kwwwwkwk"
    "...kkkk.k.";
/* a popper: a cart with a paint bomb and its fuse */
static const char POPPER_FULL[] =
    ".kk...kk.."
    ".kk.kkkk.."
    "kkkkPPPPk."
    "kgkPKwPPPk"
    "kgkPwPPPPk"
    "kgkPPPPPPk"
    "kgkPPPPPPk"
    "kkkkPPPPky"
    ".kk.kkkkoa"
    ".kk...kk..";
/* a hedgehog: a pink car behind a plough of spikes */
static const char HEDGEHOG[] =
    ".kkk..kk.."
    ".kkkkkkkkw"
    "kPPPPPPkwl"
    "kPKKKKPkkw"
    "kPKwwKPkwl"
    "kPKwwKPkwl"
    "kPKKKKPkkw"
    "kPPPPPPkwl"
    ".kkkkkkkkw"
    ".kkk..kk..";
/* a conker with its spines, and without */
static const char CONKER[] =
    "....w....."
    ".w.kkk.w.."
    "..kbeebk.."
    ".kbeteebk."
    "wkbewkebkw"
    ".kbewkebk."
    ".kbeeeebk."
    "..kbeebk.."
    ".w.kkk.w.."
    "....w.....";
static const char CONKER_BARE[] =
    ".........."
    "...kkkk..."
    "..kbeebk.."
    ".kbeteebk."
    ".kbewkebk."
    ".kbewkebk."
    ".kbeeeebk."
    "..kbeebk.."
    "...kkkk..."
    "..........";
/* a jelly: a wobbling purple cube that throws fast cars */
static const char JELLY[] =
    ".........."
    ".kkkkkkkk."
    "kVKVVVVVVk"
    "kVKVVVVVVk"
    "kVVwkVwkVk"
    "kVVwkVwkVk"
    "kVVVVVVVVk"
    "kpVVVVVVpk"
    ".kpppppppk"
    "..kkkkkkk.";
/* the helper: a little wind-up kart */
static const char HELPER[] =
    ".........."
    "..kk..kk.."
    ".kBBBBBBk."
    ".kBwwBBuk."
    ".kBwwBBuk."
    ".kBBBBBBk."
    "..kk..kk.."
    "....y....."
    "...yay...."
    "..........";
/* the duster: a toy biplane, two propeller frames */
static const char DUSTER1[] =
    "...kk....."
    "..kPPk...."
    "kkkPPkkkk."
    "kKKKKKKKKw"
    "kPPwwPPPkl"
    "kPPwwPPPkl"
    "kKKKKKKKKw"
    "kkkPPkkkk."
    "..kPPk...."
    "...kk.....";
static const char DUSTER2[] =
    "...kk....."
    "..kPPk...."
    "kkkPPkkkkl"
    "kKKKKKKKk."
    "kPPwwPPPkw"
    "kPPwwPPPkw"
    "kKKKKKKKk."
    "kkkPPkkkkl"
    "..kPPk...."
    "...kk.....";

/* power-up icons: yellow, as they sit on the floor */
static const char SPRINKLER[] =
    "..kkk..."
    ".kyyyk.k"
    ".kakyky."
    "kyyyyyk."
    "kyaaayk."
    "kyyyyyk."
    "kyaaayk."
    ".kkkkk..";
static const char TACK[] =
    ".kkkkkk."
    "kyyyyyyk"
    ".kyaayk."
    "..kyyk.."
    ".kyyyyk."
    "kkkkkkkk"
    "...kk..."
    "...kk...";
static const char HELPER_ICON[] =
    "........"
    ".kk..kk."
    "kyyyyyyk"
    "kywyyaak"
    "kywyyaak"
    "kyyyyyyk"
    ".kk..kk."
    "........";
static const char FREEZE[] =
    "..kkkk.."
    ".kIIIyk."
    ".kIIyyk."
    ".kIyyyk."
    ".kyyyak."
    "..kkkk.."
    "...kt..."
    "...kt...";

/* cutscene portraits */
static const char BO_BIG[] =
    "......kkkkkkkk......"
    "....kkBBBBBBBBkk...."
    "...kBBBuuuuuuBBBk..."
    "..kBBuuuuuuuuuuBBk.."
    "..kBkkkkkkkkkkkkBk.."
    ".kkkIIIIkkkkIIIIkkk."
    ".kykIwwIkyykIwwIkyk."
    ".kykIIIIkyykIIIIkyk."
    "..kkkkkktttkkkkkkk.."
    "..ktttttttttttttk..."
    "..kttkkttttkkttk...."
    "..ktttttttttttttk..."
    "...kttttrrrtttk....."
    "....kttttttttk......"
    ".....kkkkkkkk......."
    "....kBBBwwBBBk......"
    "...kBBBBwwBBBBk....."
    "..kBBuBBwwBBuBBk...."
    "..kBBuBBwwBBuBBk...."
    "..kBBBBByyBBBBBk...."
    "..ktkBBBBBBBBktk...."
    "..kkkkkkkkkkkkkk....";
static const char FOXY_BIG[] =
    "..kk............kk.."
    ".kook..........kook."
    ".koook.kkkkkk.kooook"
    ".kooookooooooooookk."
    "..kooooooooooooook.."
    "..koookkooookkoook.."
    ".koookykooookykooook"
    ".koooookooooooooook."
    ".kwwwwoooookooowwwk."
    "..kwwwwwwkkkwwwwwk.."
    "...kwwwwwwwwwwwwk..."
    "....kkwwwwwwwwkk...."
    "......kkkkkkkk......"
    ".....kPPPwwPPPk....."
    "....kPPPPwwPPPPk...."
    "...kPPKPPwwPPKPPk..."
    "...kPPKPPwwPPKPPk..."
    "...kPPPPPyyPPPPPk..."
    "...koPPPPPPPPPPok..."
    "...kkkkkkkkkkkkkk...";
static const char MARSHAL_BIG[] =
    "......kkkkkkkk......"
    ".....kkkkkkkkkk....."
    "....kkkkkkkkkkkk...."
    "...kkkkkkkkkkkkkkk.."
    "..kkkkkkkkkkkkkkkkk."
    "...kttttttttttttk..."
    "..kttkkttttttkkttk.."
    "..kttkwkttttkwkttk.."
    "..kttttttttttttttk.."
    "..kttttteeeetttttk.."
    "..ktttteekkeettttk.."
    "..kttttteeeetttttk.."
    "..kkttwwwwwwwwttkk.."
    "...kktwwwwwwwwtkk..."
    ".....kkkkkkkkkk....."
    "....kNNNNrrNNNNk...."
    "...kNNNNNrrNNNNNk..."
    "..kNNyNNNNNNNNyNNk.."
    "..kNNNNNNNNNNNNNNk.."
    "..kkkkkkkkkkkkkkkk..";
static const char HEAD[] =
    ".kkkkk."
    "kBuuuBk"
    "kkkkkkk"
    "kIwkIwk"
    "kttttk."
    ".kttk.."
    "..kk...";
static const char MEDAL[] =
    "kBBk..kBBk.."
    "kBBBkkBBBk.."
    ".kBBBBBBk..."
    "..kBBBBk...."
    "...kkkk....."
    "..kyyyyk...."
    ".kyaaaayk..."
    "kyaywwayak.."
    "kyayyyyayk.."
    "kyaaaaaayk.."
    ".kyaaaayk..."
    "..kyyyyk...."
    "...kkkk.....";
/* a crumpled kart, dizzy for a moment */
static const char CRUMPLE[] =
    "..kk..k..."
    ".kkkkBBk.."
    "kBBkBBBBk."
    "kBuBBkBBuk"
    ".kBywwBBk."
    "kBBwwyBkk."
    "kkBBBBkBBk"
    ".kBBkBBBk."
    "..kkBBkkk."
    "...k..kk..";

typedef struct { int id, w, h; const char *px; } Art;
static const Art CARS[] = {
    {WS_BO, 10, 10, BO},
    {WS_FOXY, 10, 10, FOXY},
    {WS_ROLLER, 10, 10, ROLLER},
    {WS_TANKER, 10, 10, TANKER},
    {WS_GLOOP, 10, 10, GLOOP},
    {WS_BIGGLOOP, 10, 10, BIGGLOOP},
    {WS_POPPER, 10, 10, POPPER_FULL},
    {WS_HEDGEHOG, 10, 10, HEDGEHOG},
    {WS_CONKER, 10, 10, CONKER},
    {WS_CONKER_BARE, 10, 10, CONKER_BARE},
    {WS_JELLY, 10, 10, JELLY},
    {WS_HELPER, 10, 10, HELPER},
    {WS_DUSTER1, 10, 10, DUSTER1},
    {WS_DUSTER2, 10, 10, DUSTER2},
};
static const Art ART[] = {
    {WS_SPRINKLER, 8, 8, SPRINKLER},
    {WS_TACK, 8, 8, TACK},
    {WS_HELPER_ICON, 8, 8, HELPER_ICON},
    {WS_FREEZE, 8, 8, FREEZE},
    {WS_BO_BIG, 20, 22, BO_BIG},
    {WS_FOXY_BIG, 20, 20, FOXY_BIG},
    {WS_MARSHAL_BIG, 20, 20, MARSHAL_BIG},
    {WS_HEAD, 7, 7, HEAD},
    {WS_MEDAL, 12, 13, MEDAL},
    {WS_CRUMPLE, 10, 10, CRUMPLE},
};

/* Turn a right-facing drawing to face down, left or up. */
static void turned(const char *src, int w, int h, int dir, char *dst, int *ow, int *oh) {
    *ow = (dir & 1) ? h : w;
    *oh = (dir & 1) ? w : h;
    for (int y = 0; y < *oh; y++)
        for (int x = 0; x < *ow; x++) {
            int sx, sy;
            switch (dir) {
            case WP_DOWN: sx = y; sy = h - 1 - x; break;
            case WP_LEFT: sx = w - 1 - x; sy = y; break;
            case WP_UP: sx = w - 1 - y; sy = x; break;
            default: sx = x; sy = y; break;
            }
            dst[y * *ow + x] = src[sy * w + sx];
        }
    dst[*ow * *oh] = 0;
}

void wp_art_load(void) {
    static int loaded;
    if (loaded) return;
    for (int i = 0; i < ARRAY_LEN(CARS); i++)
        for (int d = 0; d < 4; d++) {
            char buf[16 * 16 + 1];
            int w, h;
            turned(CARS[i].px, CARS[i].w, CARS[i].h, d, buf, &w, &h);
            spr_make(&wp_car[CARS[i].id][d], w, h, buf);
        }
    for (int i = 0; i < ARRAY_LEN(ART); i++) spr_make(&wp_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
    loaded = 1;
}
