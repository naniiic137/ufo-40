/* CHIME CIRCUIT - pixel art (palette-letter strings, see gfx.h), all drawn
 * for UFO 40: the chime ship, the six pilots, the pickups and weapons, and
 * the eight tracks' skies and walls. */
#include "chime.h"

Sprite chm_spr[CA_COUNT];
Sprite chm_face[CHM_PILOTS];
static bool art_ok = true;

const ChmPilot CHM_PILOT[CHM_PILOTS] = {
    {"ANSEL", "THE TINKLER", "CHIME-SHIP ACE. HAS NEVER LOST A HAT.", C_SKY, C_NAVY},
    {"CLARY", "THE CLARION", "ANSEL'S SISTER. BUILT HER SHIP TALLER.", C_PINK, C_MAGENTA},
    {"WICK", "THE TUMBLEWEED", "A COURIER FROM HOMESPUN, ON HER DAY OFF.", C_RED, C_MAROON},
    {"TANGER", "THE BOOMER", "FROM HOMESPUN'S CAVES. LIKES LOUD THINGS.", C_ORANGE, C_BROWN},
    {"KIP", "THE SALVAGE", "A SCRAP-DIVER FROM SKYWELL. BUILT HER OWN.", C_LEAF, C_FOREST},
    {"ZORP", "THE WALLET", "ONE EYE, TWO WALLETS, A THIRD ON THE RACE.", C_VIOLET, C_PURPLE},
};

/* the chime ship, facing right: 'r' is the body, 'o' the trim band */
static const char SHIP[] =
    "....kkkk...."
    "...krrrrk..."
    "..krrrrCCk.."
    "..krrrCwCk.."
    ".krrrrrCCrk."
    ".krrrrrrrrk."
    "krrrrrrrrrrk"
    "kooooooooook"
    ".kkkkkkkkkk."
    "....kaak....";

static const char ICON_BULLETS[] =
    "....aa...."
    "....yy...."
    ".........."
    "....kk...."
    "ay.kwwk.ya"
    "ay.kwwk.ya"
    "....kk...."
    ".........."
    "....yy...."
    "....aa....";
static const char ICON_MINES[] =
    "....rr...."
    ".r.kkkk.r."
    "..kggggk.."
    ".kgwgggsk."
    "rkgggggskr"
    "rkggggsskr"
    ".kgggsssk."
    "..kssssk.."
    ".r.kkkk.r."
    "....rr....";
static const char ICON_FIRE[] =
    "..oy......"
    ".oyyo....."
    ".oyyo....."
    "..oo......"
    "....kk...."
    "....kk...."
    "......oo.."
    ".....oyyo."
    ".....oyyo."
    "......yo..";
static const char ICON_SUPER[] =
    "..CCww...."
    "....Cww..."
    ".....Cww.."
    "......Cww."
    "......Cww."
    "......Cww."
    "......Cww."
    ".....Cww.."
    "....Cww..."
    "..CCww....";
static const char ICON_PAYLOAD[] =
    "k........."
    ".g........"
    "..g......."
    "...g......"
    "....g....."
    ".....kkk.."
    "....kgggk."
    "....kgwgk."
    "....kgggk."
    ".....kkk..";
static const char MINE[] =
    "...r..."
    ".rkkkr."
    ".kgwgk."
    "rkgggkr"
    ".kgggk."
    ".rkkkr."
    "...r...";
static const char FIREBALL[] =
    "..yy.."
    ".yccy."
    "yccwcy"
    "ycccoy"
    ".yooy."
    "..oo..";
static const char FIRE1[] =
    "...y...."
    "..yy..y."
    "..yoy.y."
    ".yoooyy."
    ".yoroooy"
    "yoorrooy"
    "yorrrroy"
    "yorrrroy"
    ".orrrro."
    "..oooo..";
static const char FIRE2[] =
    "....y..."
    ".y..yy.."
    ".y.yoy.."
    ".yyoooy."
    "yooorooy"
    "yoorrooy"
    "yorrrroy"
    "yorrrroy"
    ".orrrro."
    "..oooo..";
static const char BALL[] =
    "..kkk.."
    ".kgggk."
    "kgwgggk"
    "kggggsk"
    "kgggssk"
    ".kgssk."
    "..kkk..";
static const char ARROW[] =
    "...kk..."
    "...krk.."
    "kkkkrrk."
    "krrrrrrk"
    "krrrrrrk"
    "kkkkrrk."
    "...krk.."
    "...kk...";
static const char ARROW_UP[] =
    "...kk..."
    "..krrk.."
    ".krrrrk."
    "krrrrrrk"
    "kkkrrkkk"
    "..krrk.."
    "..krrk.."
    "..kkkk..";
static const char HATCH[] =
    "kkkkkkkk"
    "kyykyyky"
    "kggggggk"
    "kgsssssk"
    "kgsddssk"
    "kgsssssk"
    "kggggggk"
    "kkkkkkkk";
static const char CUP[] =
    "................"
    "..kkkkkkkkkkkk.."
    "..kcyyyyyyyyak.."
    "kkkcyyyyyyyyakkk"
    "k.kcyyyyyyyyak.k"
    "k..kcyyyyyyak..k"
    ".k.kcyyyyyyak.k."
    "..kkkcyyyyakkk.."
    ".....kcyyak....."
    "......kyak......"
    "......kyak......"
    ".....kcyyak....."
    "....kkkkkkkk...."
    "....kbtttttk...."
    "....kbbbbbbk...."
    "....kkkkkkkk....";
static const char FLAG[] =
    "k......."
    "kwwkkww."
    "kwwkkww."
    "kkkwwkk."
    "kkkwwkk."
    "k......."
    "k......."
    "k.......";

static const char *const FACES[CHM_PILOTS] = {
    /* ANSEL */
    "................"
    ".....BBBBBB....."
    "...BBuuuuuuBB..."
    "..BuuuuuuuuuuB.."
    "..BkkkkuukkkkB.."
    ".BkCCwkBBkCCwkB."
    ".BkCCCkBBkCCCkB."
    "..bkkkkhhkkkkb.."
    "..bhhhhhhhhhhb.."
    "..hhkhhhhhhkhh.."
    "..hhkhhhhhhkhh.."
    "..ehhhhehhhhhe.."
    "...hhhvvvvhhh..."
    "....ehhhhhhe...."
    "...NNNehheNNN..."
    "..NNNNNNNNNNNN..",
    /* CLARY */
    "................"
    "....PPPPPPPP...."
    "...PKKKKKKKKP..."
    "..PKKKKKKKKKKP.."
    "..PkkkkKKkkkkP.."
    ".PKkCwkKKkCwkKP."
    ".PKkkkkhhkkkkKP."
    ".PKhhhhhhhhhhKP."
    ".PKhkkhhhhkkhKP."
    ".PKhhhhhhhhhhKP."
    ".PKKhhhhhhhhKKP."
    "..PKhhhvvhhhKP.."
    "..PKKhhhhhhKKP.."
    "...PP.ehhe.PP..."
    "....VVVhhVVV...."
    "...VVVVVVVVVV...",
    /* WICK */
    "................"
    ".....tttttt....."
    "...tteeeeeett..."
    "..teeeeeeeeeet.."
    "..tkkkkeekkkkt.."
    ".tkIIwkttkIIwkt."
    ".tkkkkkhhkkkkkt."
    "..ohhhhhhhhhho.."
    "..ohkkhhhhkkho.."
    "..ohhhhhhhhhho.."
    "..oohhhhhhhhoo.."
    "...ohhhvvhhho..."
    "....hhhhhhhh...."
    "..rrrrehherrrr.."
    ".rrrrrrrrrrrrrr."
    "..rr.bbbbbbb....",
    /* TANGER */
    "................"
    "..k.........k..."
    "..ok.kkkkk.ko..."
    "...koooooooko..."
    "..kooooooooook.."
    ".koowwoooowwook."
    ".koowkoooowkook."
    ".kooooooooooook."
    ".kaooooooooooak."
    ".koookkkkkkoook."
    ".kookwwwwwwkook."
    ".koookkkkkkoook."
    "..kooooooooook.."
    "...kkooooookk..."
    "....bkkkkkkb...."
    "...bbb....bbb...",
    /* KIP */
    "................"
    ".....ffffff....."
    "...ffzzzzzzff..."
    "..fzzzzzzzzzzf.."
    "..fzzkkkkkkzzf.."
    ".fzzkaawkaawkzf."
    ".fzzkkkkkkkkzzf."
    ".fzhhhhhhhhhhzf."
    ".fzhkkhhhhkkhzf."
    ".fzhhhhhhhhhhzf."
    ".fzhthhhhhhthzf."
    "..fzhhhvvhhhzf.."
    "..fzzhhhhhhzzf.."
    "...ffzzzzzzff..."
    "...tttffffttt..."
    "..tttttttttttt..",
    /* ZORP */
    ".......k........"
    "......kPk......."
    ".......k........"
    ".....kkkkkk....."
    "...kkVVVVVVkk..."
    "..kVVVwwwwVVVk.."
    ".kVVVwwwwwwVVVk."
    ".kVVwwwkkwwwVVk."
    ".kVVwwkkkkwwVVk."
    ".kVVVwwkkwwVVVk."
    ".kVVVVwwwwVVVVk."
    ".kpVVVVVVVVVVpk."
    "..kpVVkkkkVVpk.."
    "...kppVVVVppk..."
    "....kkkkkkkk...."
    "...ppp....ppp...",
};

static void make(Sprite *s, int w, int h, const char *data) {
    if ((int)strlen(data) != w * h) art_ok = false;
    spr_make(s, w, h, data);
}

bool chm_art_ok(void) { return art_ok; }

void chm_art_load(void) {
    if (chm_spr[CA_SHIP].px) return;
    make(&chm_spr[CA_SHIP], 12, 10, SHIP);
    make(&chm_spr[CA_ICON_BULLETS], 10, 10, ICON_BULLETS);
    make(&chm_spr[CA_ICON_MINES], 10, 10, ICON_MINES);
    make(&chm_spr[CA_ICON_FIRE], 10, 10, ICON_FIRE);
    make(&chm_spr[CA_ICON_SUPER], 10, 10, ICON_SUPER);
    make(&chm_spr[CA_ICON_PAYLOAD], 10, 10, ICON_PAYLOAD);
    make(&chm_spr[CA_MINE], 7, 7, MINE);
    make(&chm_spr[CA_FIREBALL], 6, 6, FIREBALL);
    make(&chm_spr[CA_FIRE1], 8, 10, FIRE1);
    make(&chm_spr[CA_FIRE2], 8, 10, FIRE2);
    make(&chm_spr[CA_BALL], 7, 7, BALL);
    make(&chm_spr[CA_ARROW], 8, 8, ARROW);
    make(&chm_spr[CA_ARROW_UP], 8, 8, ARROW_UP);
    make(&chm_spr[CA_HATCH], 8, 8, HATCH);
    make(&chm_spr[CA_CUP], 16, 16, CUP);
    make(&chm_spr[CA_FLAG], 8, 8, FLAG);
    for (int p = 0; p < CHM_PILOTS; p++) make(&chm_face[p], 16, 16, FACES[p]);
}

/* ---- ships and pilots ------------------------------------------------------ */

void chm_draw_ship(int pilot, int x, int y, int face, bool flame, int tilt, int hp, int t) {
    const ChmPilot *p = &CHM_PILOT[pilot % CHM_PILOTS];
    uint8_t map[256];
    pal_identity(map);
    map[C_RED] = p->body;
    map[C_ORANGE] = p->trim;
    int fl = face < 0 ? SPR_FLIPX : 0;
    int sx = x - 6, sy = y - 5;
    (void)tilt;
    if (flame) {
        int k = (t >> 1) & 1;
        gfx_rect(sx + 5, sy + 10, 2, 2 + k, C_YELLOW);
        gfx_pset(sx + 4 + k, sy + 11, C_ORANGE);
        gfx_pset(sx + 7 - k, sy + 11, C_ORANGE);
        gfx_pset(sx + 5 + k, sy + 12 + k, C_ORANGE);
        gfx_pset(sx + 6 - k, sy + 13, C_RED);
    }
    spr_draw_ex(&chm_spr[CA_SHIP], sx, sy, fl, map, -1);
    /* the lamp on top shows the damage */
    if (hp > 0) {
        int c = hp >= 3 ? C_LIME : hp == 2 ? C_YELLOW : ((t >> 3) & 1 ? C_RED : C_MAROON);
        gfx_rect(sx + 5, sy + 1, 2, 1, c);
    }
}

void chm_draw_face(int pilot, int x, int y, int scale) {
    spr_draw_scaled(&chm_face[pilot % CHM_PILOTS], x, y, scale, 0);
}

/* ---- tracks -------------------------------------------------------------------- */

typedef struct Theme {
    uint8_t sky[3];                /* top, middle, bottom of the sky */
    uint8_t fill, dark, light, rim; /* the walls, and the lip on top of them */
    uint8_t far, deco;             /* the scenery */
} Theme;

static const Theme THEMES[8] = {
    /* meadow */  {{C_SKY, C_CYAN, C_ICE}, C_TAN, C_BROWN, C_EARTH, C_LEAF, C_JADE, C_LIME},
    /* canyon */  {{C_WINE, C_ORANGE, C_AMBER}, C_WINE, C_MAROON, C_RED, C_ORANGE, C_RED, C_YELLOW},
    /* crystal */ {{C_INK, C_NIGHT, C_DUSK}, C_PURPLE, C_NIGHT, C_VIOLET, C_CYAN, C_DUSK, C_MAGENTA},
    /* desert */  {{C_AMBER, C_YELLOW, C_CREAM}, C_EARTH, C_TAN, C_HIDE, C_CREAM, C_HIDE, C_ORANGE},
    /* marsh */   {{C_TEAL, C_FOREST, C_JADE}, C_FOREST, C_TEAL, C_JADE, C_LIME, C_TEAL, C_LEAF},
    /* foundry */ {{C_INK, C_MAROON, C_WINE}, C_SLATE, C_DUSK, C_GREY, C_ORANGE, C_NIGHT, C_AMBER},
    /* garden */  {{C_SKY, C_CYAN, C_ICE}, C_JADE, C_FOREST, C_LEAF, C_LIME, C_LEAF, C_PINK},
    /* night */   {{C_INK, C_NAVY, C_BLUE}, C_NAVY, C_INK, C_BLUE, C_ICE, C_JADE, C_VIOLET},
};

/* a gentle triangle wave, -127..127, for hills and dunes */
static int isin_px(int x) {
    int p = ((x % 256) + 256) % 256, tri = p < 128 ? p : 255 - p;
    return tri * 2 - 127;
}

static void sky(const Theme *th, int theme) {
    int h = SCREEN_H;
    gfx_rect(0, 0, SCREEN_W, h / 3, th->sky[0]);
    gfx_rect(0, h / 3, SCREEN_W, h / 3, th->sky[1]);
    gfx_rect(0, 2 * h / 3, SCREEN_W, h - 2 * h / 3, th->sky[2]);
    gfx_dither(0, h / 3 - 10, SCREEN_W, 10, th->sky[1], 8);
    gfx_dither(0, 2 * h / 3 - 10, SCREEN_W, 10, th->sky[2], 8);
    switch (theme) {
    case 0: /* meadow: a sun and far hills */
        gfx_circ(250, 40, 14, C_CREAM);
        gfx_circb(250, 40, 16, C_YELLOW);
        for (int x = 0; x < SCREEN_W; x++) {
            int hy = 130 + (isin_px(x) >> 2);
            gfx_vline(x, hy, SCREEN_H, th->far);
        }
        break;
    case 1: /* canyon at sunset: mesas */
        gfx_circ(80, 110, 20, C_YELLOW);
        for (int i = 0; i < 6; i++) {
            int x = i * 60 - 10, w = 30 + (i * 17) % 20, top = 100 + (i * 23) % 30;
            gfx_rect(x, top, w, SCREEN_H - top, th->far);
            gfx_rect(x - 4, top, w + 8, 4, th->far);
        }
        break;
    case 2: /* crystal cave: glints */
        for (int i = 0; i < 60; i++) gfx_pset((i * 97 + 11) % 320, (i * 53 + 5) % 180, i % 3 ? C_DUSK : C_VIOLET);
        break;
    case 3: /* desert: dunes and a big pale sun */
        gfx_circ(160, 60, 24, C_CREAM);
        for (int x = 0; x < SCREEN_W; x++) gfx_vline(x, 140 + (isin_px(x * 2) >> 3), SCREEN_H, th->far);
        break;
    case 4: /* marsh: reeds */
        for (int i = 0; i < 70; i++) {
            int x = (i * 37) % 320, top = 120 + (i * 29) % 40;
            gfx_vline(x, top, SCREEN_H, th->far);
            gfx_pset(x, top, C_TAN);
        }
        break;
    case 5: /* foundry: chimneys and embers */
        for (int i = 0; i < 7; i++) {
            int x = 10 + i * 46, top = 60 + (i * 37) % 60;
            gfx_rect(x, top, 12, SCREEN_H - top, th->far);
            gfx_rect(x - 2, top, 16, 3, th->far);
            gfx_dither(x - 4, top - 30, 20, 30, C_DUSK, 4);
        }
        for (int i = 0; i < 40; i++) gfx_pset((i * 71 + 3) % 320, (i * 41 + 9) % 180, i % 2 ? C_ORANGE : C_AMBER);
        break;
    case 6: /* garden: trees */
        for (int i = 0; i < 9; i++) {
            int x = 18 + i * 36, y = 120 + (i * 13) % 20;
            gfx_rect(x - 1, y, 3, SCREEN_H - y, C_TAN);
            gfx_circ(x, y, 12, th->far);
            gfx_dither_circle(x - 3, y - 3, 6, C_LIME, 5);
        }
        break;
    default: /* night: an aurora over the stars */
        for (int x = 0; x < SCREEN_W; x++) {
            int y = 34 + isin_px(x * 2) / 10;
            gfx_dither(x, y, 1, 7, th->far, 10);
            if ((x / 2) % 3) gfx_dither(x, y + 7, 1, 6, th->deco, 6);
        }
        for (int i = 0; i < 90; i++) gfx_pset((i * 89 + 7) % 320, (i * 61 + 3) % 150, i % 5 ? C_GREY : C_WHITE);
        break;
    }
}

void chm_render_track(const ChmMap *m, uint8_t *px) {
    const Theme *th = &THEMES[CHM_TRACK[m->track].theme % 8];
    int theme = CHM_TRACK[m->track].theme % 8;
    Surface s = {SCREEN_W, SCREEN_H, px};
    int cx = gfx_cam_x(), cy = gfx_cam_y();
    gfx_set_target(&s);
    gfx_camera(0, 0);
    gfx_noclip();
    sky(th, theme);
    /* the walls, pixel by pixel, with a lip where air meets them */
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            bool in = y < CHM_OY || y >= CHM_OY + CHM_TH * CHM_TILE || chm_solid(m, x, y);
            if (!in) continue;
            bool up = !chm_solid(m, x, y - 1) && y - 1 >= CHM_OY, up2 = y - 2 >= CHM_OY && !chm_solid(m, x, y - 2);
            bool dn = y + 1 < CHM_OY + CHM_TH * CHM_TILE && !chm_solid(m, x, y + 1);
            bool lf = x > 0 && !chm_solid(m, x - 1, y), rt = x < SCREEN_W - 1 && !chm_solid(m, x + 1, y);
            int c = th->fill;
            if (((x * 7 + y * 13) % 17) == 0 || ((x + y * 3) % 23) == 0) c = th->dark;
            if (((x * 5 + y * 11) % 29) == 0) c = th->light;
            if (up) c = th->rim;
            else if (up2) c = theme == 2 || theme == 7 ? th->light : th->rim;
            else if (dn) c = th->dark;
            else if (lf) c = th->light;
            else if (rt) c = th->dark;
            px[y * SCREEN_W + x] = (uint8_t)c;
        }
    /* grass tufts, crystals, reeds or flowers on top of the walls */
    for (int x = 2; x < SCREEN_W - 2; x += 3)
        for (int y = CHM_OY + 1; y < CHM_OY + CHM_TH * CHM_TILE; y++)
            if (chm_solid(m, x, y) && !chm_solid(m, x, y - 1) && ((x * 13 + y) % 7) < 3) {
                gfx_pset(x, y - 1, th->deco);
                if ((x * 5 + y) % 4 == 0) gfx_pset(x, y - 2, th->deco);
            }
    /* the start line: a chequered strip; the launcher under it */
    for (int r = 0; r < CHM_TH; r++)
        for (int c = 0; c < CHM_TW; c++) {
            int x0 = c * CHM_TILE, y0 = CHM_OY + r * CHM_TILE;
            if (m->zone[r][c] == CHM_ZONE_S)
                for (int yy = 0; yy < CHM_TILE; yy++)
                    for (int xx = 2; xx < 6; xx++)
                        px[(y0 + yy) * SCREEN_W + x0 + xx] = (uint8_t)((((xx - 2) >> 1) + (yy >> 1)) & 1 ? C_INK : C_WHITE);
            if (m->boost[r][c] == 1) spr_draw(&chm_spr[CA_ARROW], x0, y0, 0);
            if (m->boost[r][c] == 2) spr_draw(&chm_spr[CA_ARROW], x0, y0, SPR_FLIPX);
            if (m->boost[r][c] == 3) spr_draw(&chm_spr[CA_ARROW_UP], x0, y0, 0);
            if (m->boost[r][c] == 4) spr_draw(&chm_spr[CA_ARROW_UP], x0, y0, SPR_FLIPY);
            if (c == m->launch_c && r == m->launch_r) spr_draw(&chm_spr[CA_HATCH], x0, y0, 0);
        }
    gfx_set_target(NULL);
    gfx_camera(cx, cy);
}

void chm_draw_mini_track(const ChmMap *m, int x, int y) {
    const Theme *th = &THEMES[CHM_TRACK[m->track].theme % 8];
    gfx_rect(x, y, 160, 88, th->sky[1]);
    for (int j = 0; j < 88; j++)
        for (int i = 0; i < 160; i++) {
            int px = i * 2 + 1, py = CHM_OY + j * 2 + 1;
            int c = -1;
            if (chm_solid(m, px, py)) c = th->fill;
            else {
                int z = chm_zone_at(m, px, py);
                if (z == CHM_ZONE_S) c = (j >> 1) & 1 ? C_WHITE : C_INK;
                else if (chm_boost_at(m, px, py)) c = C_RED;
                else if (m->station[(py - CHM_OY) >> 3][px >> 3]) c = C_YELLOW;
            }
            if (c >= 0) gfx_pset(x + i, y + j, c);
        }
    gfx_rectb(x - 1, y - 1, 162, 90, C_INK);
}
