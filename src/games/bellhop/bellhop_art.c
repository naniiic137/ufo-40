/* BELLHOP - pixel art, all drawn for UFO 40. The Tinkler itself is CHIME
 * CIRCUIT's chime ship (chime_art.c), the same ship Ansel races there. */
#include "bellhop.h"
#include "../chime/chime.h"

static bool art_ok = true;

enum {
    SP_MOTH1, SP_MOTH2, SP_MITE, SP_WASP1, SP_WASP2, SP_CRAWL1, SP_CRAWL2, SP_TURRET, SP_GHOST, SP_DRONE,
    SP_BOMB, SP_FUEL, SP_CUP, SP_CUP_EMPTY, SP_BUCKET, SP_APPLE, SP_SPRINKLER, SP_SPIKE, SP_HUSH, SP_OWL,
    SP_LADY, SP_COUNT
};
static Sprite spr[SP_COUNT];

static const char MOTH1[] =
    "kk......kk"
    "kVk.kk.kVk"
    "kVVkcckVVk"
    "kVVkcckVVk"
    ".kVkcckVk."
    "..kkcckk.."
    "...kkkk..."
    "....kk....";
static const char MOTH2[] =
    ".........."
    ".........."
    "kkkkkkkkkk"
    "kVVVccVVVk"
    ".kkkcckkk."
    "...kcck..."
    "...kkkk..."
    "....kk....";
static const char MITE[] =
    "..k....k.."
    "...k..k..."
    "..kkkkkk.."
    ".krrrrrrk."
    "krrwkkwrrk"
    "krrkkkkrrk"
    "krrrrrrrrk"
    ".krmrrmrk."
    "..kkkkkk.."
    ".k.k..k.k.";
static const char WASP1[] =
    ".ww..ww..."
    "wIIwwIIw.."
    ".wwkkww..."
    "..kyyyk..."
    ".kykkykk.."
    "kyyyyyyyk."
    "kkkykkykkk"
    ".kyyyyyk.."
    "..kkkkk.k."
    "........k.";
static const char WASP2[] =
    ".........."
    "wwwwkwwww."
    ".wIIkIIw.."
    "..kyyyk..."
    ".kykkykk.."
    "kyyyyyyyk."
    "kkkykkykkk"
    ".kyyyyyk.."
    "..kkkkk.k."
    "........k.";
static const char CRAWL1[] =
    "..kkkkkk.."
    ".kzzzzzzk."
    "kzwkzzzzzk"
    "kzkkzzizzk"
    ".kizkizik."
    "..k.k.k...";
static const char CRAWL2[] =
    "..kkkkkk.."
    ".kzzzzzzk."
    "kzwkzzzzzk"
    "kzkkzzizzk"
    ".kizkizik."
    "...k.k.k..";
static const char TURRET[] =
    "..kkkk.."
    ".kgllgk."
    "kglkklgk"
    "kgkrrkgk"
    "kgkrrkgk"
    "kglkklgk"
    ".kgggg.k"
    "kkkkkkkk";
static const char GHOST[] =
    "...kkkk..."
    "..kIIIIk.."
    ".kIIIIIIk."
    "kIkkIIkkIk"
    "kIwkIIwkIk"
    "kIIIIIIIIk"
    "kIIIKKIIIk"
    "kIIIIIIIIk"
    "kIkIIkIIkk"
    ".k.kk.kk..";
static const char DRONE[] =
    "kkkk..kkkk"
    "..kkkkkk.."
    ".kssssssk."
    "kslPPPPlsk"
    "ksPkPPkPsk"
    ".kssssssk."
    "..kkrrkk.."
    "....kk....";
static const char BOMB[] =
    "....ka.."
    "...k.k.."
    "..kkkk.."
    ".knnnnk."
    "knnwnnnk"
    "knnnnnnk"
    ".knnnnk."
    "..kkkk..";
static const char FUEL[] =
    "..kkkk.."
    ".kgggk.."
    "kkkkkkkk"
    "krrrrrrk"
    "krwwwwrk"
    "krwrrrrk"
    "krwwwrrk"
    "krwrrrrk"
    "krrrrrrk"
    "kkkkkkkk";
static const char CUP[] =
    "...w.w...."
    "....w.w..."
    ".kkkkkkk.."
    ".kwbbbwkkk"
    ".kwwwwwk.k"
    ".kwwwwwkkk"
    "..kwwwk..."
    "...kkk....";
static const char CUP_FULL[] =
    "........"
    "........"
    "kkkkkkk."
    "kwwwwwkk"
    "kwwwwwk."
    "kwwwwwkk"
    ".kwwwk.."
    "..kkk...";
static const char BUCKET[] =
    "..kkkkkkkk.."
    ".kbbbbbbbbk."
    "kttttttttttk"
    "kbkbbbbbbkbk"
    "kbbbbbbbbbbk"
    ".kbtttttbbk."
    ".kbbbbbbbbk."
    ".kbbbbbbbbk."
    "..kttttttk.."
    "...kkkkkk...";
static const char APPLE[] =
    ".....kk......"
    "....kfk.kk..."
    "...kkfkkzzk.."
    "..krrkrrkzk.."
    ".krrwrrrrrk.."
    "krrwwrrrrrrk."
    "krrwrrrrrrrk."
    "krrrrrrrrrmk."
    "krrrrrrrrrmk."
    ".krrrrrrrmk.."
    "..kmrrrmmk..."
    "...kkkkkk....";
static const char SPRINKLER[] =
    "...kkkk..."
    "...kCCk..."
    "..kkuukk.."
    "..kuuuuk.."
    ".kuuuuuuk."
    ".kuwuuuuk."
    "kuuuuuuuuk"
    "kuuuuuuuuk"
    "kkkkkkkkkk"
    "kssssssssk";
static const char SPIKE[] =
    "...rr..."
    ".r.rr.r."
    "..rkkr.."
    "rrkwkkrr"
    "rrkkkkrr"
    "..rkkr.."
    ".r.rr.r."
    "...rr...";
static const char HUSH[] =
    "...........kkkkkk..........."
    ".........kkpppppkk.........."
    "........kppVVVVVppk........."
    ".......kpVVkkkkVVVpk........"
    ".......kpVkIIIIkVVpk........"
    "......kppVkIwIIkVVppk......."
    "...kkkkpppVkkkkVVpppkkkk...."
    ".kkpppppppppppppppppppppkk.."
    "kppVVVVVVVVVVVVVVVVVVVVVVppk"
    "kpVnnnVVnnnVVnnnVVnnnVVnnVpk"
    "kppVVVVVVVVVVVVVVVVVVVVVVppk"
    ".kkpppppppppppppppppppppkk.."
    "...kkkkkkkkkkkkkkkkkkkkk...."
    ".....kkk..kkk....kkk..kkk..."
    "......k....k......k....k...."
    "............................";
static const char OWL[] =
    "..k........k.."
    "..kk......kk.."
    "..kbkkkkkkbk.."
    ".kbbbbbbbbbbk."
    "kbbccbbbbccbbk"
    "kbcwkcbbcwkcbk"
    "kbcwkcbbcwkcbk"
    "kbbccbaabccbbk"
    ".kbbbbaabbbbk."
    "..kbbbbbbbbk.."
    "...kkkkkkkk...";
/* Lady Hush, for the story and the ending: a face behind a veil */
static const char LADY[] =
    "....kkkkkkkk...."
    "...kppppppppk..."
    "..kppVVVVVVppk.."
    ".kpVVkkkkkkVVpk."
    ".kpVkhhhhhhkVpk."
    "kppkhhhhhhhhkppk"
    "kpkhhkkhhkkhhkpk"
    "kpkhhwkhhwkhhkpk"
    "kpkIIIIIIIIIIkpk"
    "kpkIIIIIIIIIIkpk"
    "kpkIIIIIIIIIIkpk"
    ".kpkIIIIIIIIkpk."
    ".kppkIIIIIIkppk."
    "..kppkkkkkkppk.."
    "...kpppppppppk.."
    "....kkkkkkkkk...";

static void make(Sprite *s, int w, int h, const char *data) {
    if ((int)strlen(data) != w * h) art_ok = false;
    spr_make(s, w, h, data);
}

bool bhp_art_ok(void) { return art_ok; }

void bhp_art_load(void) {
    chm_art_load();
    if (spr[SP_MOTH1].px) return;
    make(&spr[SP_MOTH1], 10, 8, MOTH1);
    make(&spr[SP_MOTH2], 10, 8, MOTH2);
    make(&spr[SP_MITE], 10, 10, MITE);
    make(&spr[SP_WASP1], 10, 10, WASP1);
    make(&spr[SP_WASP2], 10, 10, WASP2);
    make(&spr[SP_CRAWL1], 10, 6, CRAWL1);
    make(&spr[SP_CRAWL2], 10, 6, CRAWL2);
    make(&spr[SP_TURRET], 8, 8, TURRET);
    make(&spr[SP_GHOST], 10, 10, GHOST);
    make(&spr[SP_DRONE], 10, 8, DRONE);
    make(&spr[SP_BOMB], 8, 8, BOMB);
    make(&spr[SP_FUEL], 8, 10, FUEL);
    make(&spr[SP_CUP], 10, 8, CUP);
    make(&spr[SP_CUP_EMPTY], 8, 8, CUP_FULL);
    make(&spr[SP_BUCKET], 12, 10, BUCKET);
    make(&spr[SP_APPLE], 13, 12, APPLE);
    make(&spr[SP_SPRINKLER], 10, 10, SPRINKLER);
    make(&spr[SP_SPIKE], 8, 8, SPIKE);
    make(&spr[SP_HUSH], 28, 16, HUSH);
    make(&spr[SP_OWL], 14, 11, OWL);
    make(&spr[SP_LADY], 16, 16, LADY);
}

void bhp_draw_ship(int x, int y, int face, bool flame, int t) { chm_draw_ship(0, x, y, face, flame, 0, t); }
void bhp_draw_lady(int x, int y, int scale) { spr_draw_scaled(&spr[SP_LADY], x, y, scale, 0); }
void bhp_draw_ansel(int x, int y, int scale) { chm_draw_face(0, x, y, scale); }

void bhp_draw_cup(int x, int y, bool full) {
    if (full) spr_draw(&spr[SP_CUP], x, y, 0);
    else {
        uint8_t map[256];
        pal_identity(map);
        map[C_WHITE] = C_SLATE;
        map[C_BROWN] = C_SLATE;
        map[C_INK] = C_DUSK;
        spr_draw_ex(&spr[SP_CUP_EMPTY], x, y, 0, map, -1);
    }
}

/* ---- the worlds ------------------------------------------------------------------ */

typedef struct Theme {
    uint8_t sky[3];
    uint8_t fill, dark, light, rim;
    uint8_t far, liquid, liquid2;
} Theme;

static const Theme THEME[BHP_WORLDS] = {
    /* Millbrook: stone walls with moss, a millpond */
    {{C_SKY, C_CYAN, C_ICE}, C_GREY, C_SLATE, C_LIGHT, C_LEAF, C_JADE, C_BLUE, C_SKY},
    /* the Orchard: earth banks under leaves, a brook */
    {{C_CREAM, C_AMBER, C_ORANGE}, C_BROWN, C_EARTH, C_TAN, C_LIME, C_FOREST, C_TEAL, C_JADE},
    /* the Clockworks: brass plate, oil */
    {{C_INK, C_NIGHT, C_DUSK}, C_AMBER, C_BROWN, C_YELLOW, C_CREAM, C_SLATE, C_INK, C_DUSK},
    /* the Sugarworks: candy walls, syrup */
    {{C_PINK, C_CREAM, C_WHITE}, C_MAGENTA, C_PURPLE, C_PINK, C_WHITE, C_PINK, C_AMBER, C_YELLOW},
    /* the Hush Citadel: slate and violet, ink */
    {{C_INK, C_NAVY, C_NIGHT}, C_SLATE, C_NIGHT, C_GREY, C_VIOLET, C_DUSK, C_PURPLE, C_VIOLET},
};

static void sky(int w, int t) {
    const Theme *th = &THEME[w];
    gfx_rect(0, 0, SCREEN_W, 60, th->sky[0]);
    gfx_rect(0, 60, SCREEN_W, 60, th->sky[1]);
    gfx_rect(0, 120, SCREEN_W, 60, th->sky[2]);
    gfx_dither(0, 50, SCREEN_W, 10, th->sky[1], 8);
    gfx_dither(0, 110, SCREEN_W, 10, th->sky[2], 8);
    switch (w) {
    case 0: /* far hills and a mill */
        gfx_circ(60, 40, 12, C_WHITE);
        for (int x = 0; x < SCREEN_W; x++) {
            int h = 130 + bhp_sin(x * 2) / 12 + bhp_sin(x / 2 + 40) / 9;
            gfx_vline(x, h, SCREEN_H, th->far);
        }
        gfx_rect(240, 104, 18, 30, C_TAN);
        for (int k = 0; k < 4; k++) {
            int a = t / 4 + k * 64;
            gfx_line(249, 104, 249 + bhp_cos(a) * 18 / 127, 104 + bhp_sin(a) * 18 / 127, C_BROWN);
        }
        break;
    case 1: /* apple trees */
        for (int i = 0; i < 8; i++) {
            int x = 20 + i * 42, y = 118 + (i * 13) % 18;
            gfx_rect(x - 2, y, 4, SCREEN_H - y, C_EARTH);
            gfx_circ(x, y - 4, 15, th->far);
            gfx_dither_circle(x - 4, y - 8, 7, C_LEAF, 6);
            gfx_pset(x - 6, y - 2, C_RED);
            gfx_pset(x + 5, y - 9, C_RED);
            gfx_pset(x + 2, y + 3, C_RED);
        }
        break;
    case 2: /* gears turning in the dark */
        for (int i = 0; i < 6; i++) {
            int cx = 30 + i * 55, cy = 60 + (i * 37) % 80, r = 14 + (i * 7) % 12;
            gfx_circb(cx, cy, r, th->far);
            gfx_circb(cx, cy, r / 3, th->far);
            for (int k = 0; k < 8; k++) {
                int a = (i & 1 ? t : -t) / 3 + k * 32;
                gfx_rect(cx + bhp_cos(a) * (r + 2) / 127 - 1, cy + bhp_sin(a) * (r + 2) / 127 - 1, 3, 3, th->far);
            }
        }
        break;
    case 3: /* candy stripes and lollipops */
        for (int x = 0; x < SCREEN_W; x += 24) gfx_dither(x, 0, 12, SCREEN_H, C_PINK, 3);
        for (int i = 0; i < 6; i++) {
            int x = 26 + i * 54, y = 120 + (i * 11) % 16;
            gfx_rect(x - 1, y, 2, SCREEN_H - y, C_WHITE);
            gfx_circ(x, y - 8, 9, i & 1 ? C_PINK : C_LIME);
            gfx_circb(x, y - 8, 5, C_WHITE);
        }
        break;
    default: /* the citadel's towers under the stars */
        for (int i = 0; i < 70; i++) gfx_pset((i * 89 + 7) % 320, (i * 61 + 3) % 120, i % 6 ? C_SLATE : C_WHITE);
        for (int i = 0; i < 7; i++) {
            int x = 8 + i * 46, top = 70 + (i * 29) % 50;
            gfx_rect(x, top, 16, SCREEN_H - top, th->far);
            for (int k = 0; k < 4; k++) gfx_rect(x + k * 4, top - 3, 2, 3, th->far);
            gfx_pset(x + 7, top + 10, (t / 30 + i) % 3 ? C_YELLOW : C_AMBER);
        }
        break;
    }
}

void bhp_draw_world_backdrop(int world, int t) { sky(iclamp(world, 0, BHP_WORLDS - 1), t); }

/* the stage's scenery, kept until a tile changes */
static uint8_t bg_px[SCREEN_W * SCREEN_H];
static int bg_stage = -1, bg_ver = -1, bg_t = -1;

static bool solid_px(const BhpStage *s, int x, int y) {
    int t = bhp_tile_at(s, x, y);
    return t == BTL_WALL || t == BTL_WALL2 || t == BTL_PRESS || t == BTL_CRACK;
}

static void render_bg(const BhpStage *s, int t) {
    int w = s->idx / BHP_PER_WORLD;
    const Theme *th = &THEME[w];
    Surface surf = {SCREEN_W, SCREEN_H, bg_px};
    gfx_set_target(&surf);
    gfx_camera(0, 0);
    gfx_noclip();
    sky(w, t);
    for (int y = BHP_OY; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            if (!solid_px(s, x, y)) continue;
            int tl = bhp_tile_at(s, x, y);
            bool up = y > BHP_OY && !solid_px(s, x, y - 1), up2 = y > BHP_OY + 1 && !solid_px(s, x, y - 2);
            bool dn = y + 1 < SCREEN_H && !solid_px(s, x, y + 1);
            bool lf = x > 0 && !solid_px(s, x - 1, y), rt = x < SCREEN_W - 1 && !solid_px(s, x + 1, y);
            int c = th->fill;
            if (tl == BTL_WALL2) c = ((x / 4 + y / 4) & 1) ? th->dark : th->fill;
            if (tl == BTL_PRESS) c = ((x + y) / 3) % 4 ? C_BROWN : C_EARTH;
            if (((x * 7 + y * 13) % 17) == 0 || ((x + y * 3) % 23) == 0) c = th->dark;
            if (tl == BTL_CRACK) c = ((x * 3 + y * 5) % 7) == 0 ? C_INK : th->dark;
            if (lf || rt || dn) c = th->dark;
            if (up2) c = th->light;
            if (up) c = th->rim;
            gfx_pset(x, y, c);
        }
    gfx_set_target(NULL);
    bg_stage = s->idx;
    bg_ver = s->ver;
    bg_t = t;
}

/* ---- things ---------------------------------------------------------------------- */

static int PX(int32_t v) { return (int)(v >> 8); }

static void draw_tiles_live(const BhpStage *s, int t) {
    const Theme *th = &THEME[s->idx / BHP_PER_WORLD];
    for (int r = 0; r < BHP_TH; r++)
        for (int c = 0; c < BHP_TW; c++) {
            int tl = s->tile[r][c], x = c * BHP_T, y = BHP_OY + r * BHP_T;
            switch (tl) {
            case BTL_LIQUID: {
                bool top = r == 0 || s->tile[r - 1][c] != BTL_LIQUID;
                gfx_rect(x, y, 8, 8, th->liquid);
                if (top) {
                    int k = (t / 8 + c) & 3;
                    gfx_hline(x, x + 7, y, th->liquid2);
                    gfx_pset(x + k * 2, y + 1, th->liquid2);
                }
                break;
            }
            case BTL_BLOCK:
                gfx_rect(x, y, 8, 8, C_TAN);
                gfx_rectb(x, y, 8, 8, C_BROWN);
                gfx_hline(x + 1, x + 6, y + 1, C_CREAM);
                gfx_pset(x + 3, y + 4, C_BROWN);
                gfx_pset(x + 4, y + 3, C_BROWN);
                break;
            case BTL_GLASS:
                gfx_rect(x, y, 8, 8, C_ICE);
                gfx_rectb(x, y, 8, 8, C_SKY);
                gfx_line(x + 1, y + 5, x + 5, y + 1, C_WHITE);
                break;
            case BTL_GATE1: case BTL_GATE2: {
                int col = tl == BTL_GATE1 ? C_ORANGE : C_CYAN;
                gfx_rect(x, y, 8, 8, C_INK);
                gfx_rect(x + 1, y, 2, 8, col);
                gfx_rect(x + 5, y, 2, 8, col);
                break;
            }
            case BTL_GATE1_OPEN: case BTL_GATE2_OPEN: {
                int col = tl == BTL_GATE1_OPEN ? C_ORANGE : C_CYAN;
                gfx_pset(x + 1, y + 3, col);
                gfx_pset(x + 6, y + 4, col);
                break;
            }
            case BTL_LEVER1: case BTL_LEVER2: {
                int col = tl == BTL_LEVER1 ? C_ORANGE : C_CYAN;
                gfx_rect(x, y + 5, 8, 3, C_SLATE);
                gfx_rectb(x, y + 5, 8, 3, C_INK);
                /* the handle leans the way its gates are */
                bool open = false;
                for (int rr = 0; rr < BHP_TH && !open; rr++)
                    for (int cc = 0; cc < BHP_TW; cc++)
                        if (s->tile[rr][cc] == (tl == BTL_LEVER1 ? BTL_GATE1_OPEN : BTL_GATE2_OPEN)) { open = true; break; }
                gfx_line(x + 4, y + 5, x + (open ? 7 : 1), y + 1, C_INK);
                gfx_rect(x + (open ? 6 : 0), y, 2, 2, col);
                break;
            }
            case BTL_CANNON_L: case BTL_CANNON_R: case BTL_CANNON_U: case BTL_CANNON_D:
                gfx_rect(x, y, 8, 8, C_NIGHT);
                gfx_circ(x + 4, y + 4, 3, C_SLATE);
                gfx_circ(x + 4 + (tl == BTL_CANNON_R) * 3 - (tl == BTL_CANNON_L) * 3, y + 4 + (tl == BTL_CANNON_D) * 3 - (tl == BTL_CANNON_U) * 3, 1, C_INK);
                gfx_rectb(x, y, 8, 8, C_INK);
                break;
            case BTL_THORN:
                gfx_rect(x + 3, y, 2, 8, C_FOREST);
                gfx_pset(x + 1, y + 2, C_LIME);
                gfx_pset(x + 6, y + 5, C_LIME);
                gfx_pset(x + 2, y + 6, C_LIME);
                gfx_pset(x + 5, y + 1, C_LIME);
                break;
            default: break;
            }
        }
}

static void draw_ent(const BhpStage *s, const BhpEnt *e, int t) {
    int x = PX(e->x), y = PX(e->y);
    int f = (t / 6 + e->id) & 1;
    switch (e->kind) {
    case BEK_MOTH: spr_draw(&spr[f ? SP_MOTH1 : SP_MOTH2], x - 5, y - 4, e->vx < 0 ? SPR_FLIPX : 0); break;
    case BEK_MITE: spr_draw(&spr[SP_MITE], x - 5, y - 5, f ? SPR_FLIPX : 0); break;
    case BEK_WASP: spr_draw(&spr[f ? SP_WASP1 : SP_WASP2], x - 5, y - 5, PX(s->f.x) < x ? SPR_FLIPX : 0); break;
    case BEK_CRAWLER: spr_draw(&spr[f ? SP_CRAWL1 : SP_CRAWL2], x - 5, y - 3, (e->vx < 0 ? SPR_FLIPX : 0) | (e->flag ? SPR_FLIPY : 0)); break;
    case BEK_TURRET: spr_draw(&spr[SP_TURRET], x - 4, y - 4, 0); break;
    case BEK_GHOST:
        if ((t + e->id) % 3) spr_draw(&spr[SP_GHOST], x - 5, y - 5, bhp_cos(e->t) < 0 ? SPR_FLIPX : 0);
        else spr_draw_outline(&spr[SP_GHOST], x - 5, y - 5, 0, C_WHITE);
        break;
    case BEK_DRONE: spr_draw(&spr[SP_DRONE], x - 5, y - 4, (t / 3) & 1 ? SPR_FLIPX : 0); break;
    case BEK_BUBBLE:
        gfx_circb(x, y, 5, C_ICE);
        gfx_pset(x - 2, y - 2, C_WHITE);
        gfx_pset(x - 1, y - 3, C_WHITE);
        gfx_dither_circle(x, y, 4, C_CYAN, 3);
        break;
    case BEK_BOMB:
        spr_draw(&spr[SP_BOMB], x - 4, y - 4, 0);
        if (e->flag && (t / (e->t < 20 ? 2 : 5)) & 1) { gfx_pset(x + 1, y - 4, C_YELLOW); gfx_pset(x + 2, y - 5, C_ORANGE); gfx_circb(x, y, 5, C_RED); }
        break;
    case BEK_FIREBAR:
        gfx_circ(x, y, 3, C_SLATE);
        gfx_circb(x, y, 3, C_INK);
        for (int k = 0; k < 4; k++) {
            int r = 8 + k * 8;
            int px = x + bhp_cos(e->a) * r / 127, py = y + bhp_sin(e->a) * r / 127;
            gfx_circ(px, py, 3, (t / 3 + k) & 1 ? C_ORANGE : C_YELLOW);
            gfx_pset(px, py, C_WHITE);
        }
        break;
    case BEK_PLATE_H:
        gfx_rect(x - 12, y - 3, 24, 6, C_INK);
        gfx_rect(x - 11, y - 2, 22, 4, C_RED);
        gfx_hline(x - 11, x + 10, y - 2, C_PINK);
        for (int k = -10; k < 10; k += 4) gfx_pset(x + k, y + 1, C_MAROON);
        break;
    case BEK_PLATE_V:
        gfx_rect(x - 3, y - 12, 6, 24, C_INK);
        gfx_rect(x - 2, y - 11, 4, 22, C_RED);
        gfx_vline(x - 2, y - 11, y + 10, C_PINK);
        break;
    case BEK_FUEL: spr_draw(&spr[SP_FUEL], x - 4, y - 5, 0); break;
    case BEK_BIGCOIN:
        gfx_circ(x, y, 5, C_AMBER);
        gfx_circb(x, y, 5, C_INK);
        gfx_vline(x - 1 + ((t / 8) & 1), y - 3, y + 2, C_YELLOW);
        gfx_pset(x - 2, y - 3, C_CREAM);
        break;
    case BEK_COIN:
        if (s->coin_t > 60 || (t / 3) & 1) {
            gfx_rect(x - 2, y - 3, 4 - ((t / 6) & 1) * 2 + 1, 6, C_YELLOW);
            gfx_rectb(x - 2, y - 3, 4 - ((t / 6) & 1) * 2 + 1, 6, C_AMBER);
        }
        break;
    case BEK_CIRCLER: {
        gfx_circ(x, y, 6, C_PURPLE);
        gfx_circb(x, y, 6, C_INK);
        gfx_circ(x, y, 2, (t / 10) & 1 ? C_PINK : C_MAGENTA);
        static const int8_t N[4][2] = {{0, -12}, {12, 0}, {0, 12}, {-12, 0}};
        for (int k = 0; k < 4; k++) {
            bool lit = e->flag & (1 << k);
            gfx_rect(x + N[k][0] - 1, y + N[k][1] - 1, 3, 3, lit ? C_YELLOW : C_DUSK);
            if (lit) gfx_pset(x + N[k][0], y + N[k][1], C_WHITE);
        }
        break;
    }
    case BEK_CRYSTAL:
        for (int k = 0; k < 4; k++) gfx_hline(x - k, x + k, y - 4 + k, (t / 4 + k) & 1 ? C_CYAN : C_ICE);
        for (int k = 0; k < 4; k++) gfx_hline(x - 3 + k, x + 3 - k, y + k, C_SKY);
        gfx_pset(x - 1, y - 2, C_WHITE);
        break;
    case BEK_BUCKET: {
        uint8_t map[256];
        pal_identity(map);
        if (e->hp == 1) { map[C_BROWN] = C_MAROON; map[C_TAN] = C_RED; }
        if (s->boss.active == e->a && s->boss.t >= 30 && s->boss.t <= 125 && (t / 3) & 1) map[C_TAN] = C_YELLOW;
        spr_draw_ex(&spr[SP_BUCKET], x - 6, y - 5, 0, map, -1);
        break;
    }
    case BEK_APPLE: spr_draw(&spr[SP_APPLE], x - 6, y - 6, 0); break;
    case BEK_SPRINKLER: spr_draw(&spr[SP_SPRINKLER], x - 5, y - 5, 0); break;
    case BEK_LAMP:
        gfx_circ(x, y, 4, C_INK);
        gfx_circ(x, y, 3, e->flag == 1 ? ((t / 5) & 1 ? C_YELLOW : C_WHITE) : C_SLATE);
        if (e->flag == 1) gfx_pset(x - 1, y - 1, C_WHITE);
        break;
    case BEK_GUM: {
        int r = e->size == 2 ? 11 : e->size == 1 ? 7 : 5;
        static const uint8_t COL[5] = {C_RED, C_LIME, C_YELLOW, C_SKY, C_PINK};
        int col = COL[e->id % 5];
        gfx_circ(x, y, r, C_INK);
        gfx_circ(x, y, r - 1, col);
        gfx_circ(x - r / 3, y - r / 3, imax(1, r / 4), C_WHITE);
        break;
    }
    case BEK_CHUNK:
        gfx_rect(x - 2, y - 2, 4, 4, C_PINK);
        gfx_pset(x - 1, y - 1, C_WHITE);
        break;
    case BEK_SPIKE: {
        uint8_t map[256];
        pal_identity(map);
        if (e->flag == 1) map[C_RED] = C_SKY;
        spr_draw_ex(&spr[SP_SPIKE], x - 4, y - 4, (t / 4) & 1 ? SPR_FLIPX : 0, map, -1);
        break;
    }
    default: break;
    }
}

static void draw_boss(const BhpStage *s, int t) {
    const BhpBoss *b = &s->boss;
    switch (b->kind) {
    case 1: {
        if (b->down && b->down_t > 60) break;
        /* the wheel: rim, spokes, hub */
        gfx_rect(BHP_MILL_X - 3, BHP_MILL_Y, 6, SCREEN_H - BHP_MILL_Y, C_BROWN);
        gfx_circb(BHP_MILL_X, BHP_MILL_Y, BHP_MILL_R, C_BROWN);
        gfx_circb(BHP_MILL_X, BHP_MILL_Y, BHP_MILL_R - 1, C_EARTH);
        for (int k = 0; k < BHP_MILL_N * 2; k++) {
            int a = (int)((b->ang >> 8) + k * 128 / BHP_MILL_N) & 255;
            gfx_line(BHP_MILL_X, BHP_MILL_Y, BHP_MILL_X + bhp_cos(a) * BHP_MILL_R / 127, BHP_MILL_Y + bhp_sin(a) * BHP_MILL_R / 127, C_TAN);
        }
        gfx_circ(BHP_MILL_X, BHP_MILL_Y, 9, C_SLATE);
        gfx_circb(BHP_MILL_X, BHP_MILL_Y, 9, C_INK);
        gfx_circ(BHP_MILL_X, BHP_MILL_Y, 3, C_INK);
        break;
    }
    case 2: {
        /* the chute's mouth glows */
        int c = (t / 8) & 1 ? C_YELLOW : C_AMBER;
        gfx_rect(BHP_CHUTE_X0, BHP_CHUTE_Y0, 2, BHP_CHUTE_Y1 - BHP_CHUTE_Y0 + 1, c);
        for (int k = 0; k < 3; k++) gfx_rect(282 - k * 10, 66, 6, 6, k < b->phase ? C_LIME : C_INK);
        break;
    }
    case 3:
        for (int w = 0; w < 2; w++) {
            if (b->down && b->down_t > 60) break;
            int cx = BHP_COG_X[w], cy = BHP_COG_Y;
            int col = b->active == w && !b->down ? C_AMBER : C_BROWN;
            gfx_circ(cx, cy, BHP_COG_R, col);
            for (int k = 0; k < 10; k++) {
                int a = (int)(((w ? -b->ang : b->ang) >> 8) + k * 26) & 255;
                gfx_rect(cx + bhp_cos(a) * (BHP_COG_R + 1) / 127 - 2, cy + bhp_sin(a) * (BHP_COG_R + 1) / 127 - 2, 5, 5, col);
            }
            gfx_circb(cx, cy, BHP_COG_R, C_INK);
            gfx_circ(cx, cy, 6, C_INK);
            gfx_circ(cx, cy, 3, C_YELLOW);
            /* the dead lamps, grey on the rim */
            for (int k = 0; k < BHP_COG_N; k++) {
                bool alive = false;
                for (int i = 0; i < s->ne; i++)
                    if (s->e[i].on && s->e[i].kind == BEK_LAMP && s->e[i].a == w && s->e[i].b == k) alive = true;
                if (!alive && !b->down) {
                    int x, y;
                    bhp_cog_lamp_pos(s, w, k, &x, &y);
                    gfx_circ(x, y, 3, C_GREY);
                    gfx_circb(x, y, 3, C_SLATE);
                }
            }
        }
        break;
    case 4: {
        /* the globe over the machine, and its lid */
        int x = BHP_LID_X, y = BHP_LID_Y;
        gfx_dither_circle(x, y + 22, 26, C_ICE, 6);
        gfx_circb(x, y + 22, 26, C_WHITE);
        for (int k = 0; k < 9; k++) gfx_circ(x - 16 + (k * 13) % 32, y + 18 + (k * 7) % 22, 4, k % 3 ? C_PINK : C_LIME);
        bool open = !b->down && b->left && b->t > 30;
        gfx_rect(x - 14, y - (open ? 6 : 3), 28, 4, C_RED);
        gfx_rectb(x - 14, y - (open ? 6 : 3), 28, 4, C_INK);
        gfx_rect(x - 3, y - (open ? 9 : 6), 6, 3, C_MAROON);
        break;
    }
    case 5:
        if (b->down && b->down_t > 60) break;
        spr_draw(&spr[SP_HUSH], PX(b->x) - 14, PX(b->y) - 8, 0);
        if (!b->down)
            for (int k = 0; k < b->hp; k++) gfx_rect(PX(b->x) - 12 + k * 3, PX(b->y) - 12, 2, 2, C_MAGENTA);
        break;
    }
}

static void draw_slash(const BhpStage *s, int x, int y, int t) {
    if (!s->slash_t || s->mode != BSM_FLY) return;
    int f = s->f.face >= 0 ? 1 : -1, age = BHP_SLASH_T - s->slash_t;
    static const int8_t CS[9][2] = {{0, -10}, {4, -9}, {7, -7}, {9, -4}, {10, 0}, {9, 4}, {7, 7}, {4, 9}, {0, 10}};
    int shown = imin(9, 3 + age * 2);
    for (int k = 0; k < shown; k++) {
        int px = x + f * (CS[k][0] * 13 / 10 + 4), py = y + CS[k][1] * 9 / 10;
        gfx_rect(px - 1, py - 1, 2, 2, age < 5 ? C_WHITE : C_ICE);
    }
    (void)t;
}

void bhp_draw_stage(const BhpStage *s, int t, bool owl) {
    int w = s->idx / BHP_PER_WORLD;
    /* the sky moves a little (mill sails, gears): redraw it now and then */
    if (bg_stage != s->idx || bg_ver != s->ver || ((w == 0 || w == 2) && t - bg_t >= 8) || t < bg_t) render_bg(s, t);
    memcpy(g_screen.px, bg_px, sizeof bg_px);
    gfx_camera(0, 0);
    gfx_noclip();
    if (owl) {
        /* someone has been watching all along */
        gfx_rect(286, 140, 18, 16, C_INK);
        spr_draw(&spr[SP_OWL], 288, 142, 0);
    }
    draw_tiles_live(s, t);
    /* the exit ring: a blue circle round a flashing red cross */
    if (s->exit_open || s->kind != BHK_STAGE) {
        int x = s->exit_x, y = s->exit_y;
        if (s->exit_open) {
            gfx_circb(x, y, 7, C_BLUE);
            gfx_circb(x, y, 6, (t / 6) & 1 ? C_SKY : C_BLUE);
            int c = (t / 8) & 1 ? C_RED : C_PINK;
            gfx_hline(x - 3, x + 3, y, c);
            gfx_vline(x, y - 3, y + 3, c);
        } else {
            gfx_dither_circle(x, y, 6, C_SLATE, 4);
        }
    }
    if (s->cup == 1) bhp_draw_cup(s->cup_x - 5, s->cup_y - 5 + ((t / 12) & 1), true);
    if (s->warp_to >= 0 && s->t < BHP_WARP_SHOW) {
        /* a little sparkle */
        int x = s->warp_x, y = s->warp_y, k = (t / 4) % 4;
        int c = k & 1 ? C_WHITE : C_YELLOW;
        gfx_pset(x, y, C_WHITE);
        gfx_pset(x - 1 - (k & 1), y, c);
        gfx_pset(x + 1 + (k & 1), y, c);
        gfx_pset(x, y - 1 - (k >> 1), c);
        gfx_pset(x, y + 1 + (k >> 1), c);
    }
    draw_boss(s, t);
    for (int i = 0; i < s->ne; i++)
        if (s->e[i].on) draw_ent(s, &s->e[i], t);
    for (int k = 0; k < BHP_SHOTS; k++) {
        const BhpShot *b = &s->shot[k];
        if (!b->on) continue;
        int x = PX(b->x), y = PX(b->y);
        switch (b->kind) {
        case BSH_BALL: gfx_circ(x, y, 3, C_INK); gfx_pset(x - 1, y - 1, C_GREY); break;
        case BSH_JET: gfx_rect(x - 2, y - 2, 4, 4, C_SKY); gfx_pset(x, y, C_WHITE); break;
        case BSH_RETURN: gfx_circ(x, y, 4, C_SKY); gfx_circb(x, y, 4, C_WHITE); break;
        default: gfx_rect(x - 2, y - 2, 4, 4, (t / 3) & 1 ? C_ORANGE : C_YELLOW); gfx_pset(x, y, C_WHITE); break;
        }
    }
    /* the ship */
    int x = PX(s->f.x), y = PX(s->f.y);
    if (s->mode == BSM_BUBBLE || s->mode == BSM_FLY) {
        bool flame = (s->ctl & CHF_THRUST) && s->fuel > 0 && s->mode == BSM_FLY;
        bhp_draw_ship(x, y, s->f.face, flame, t);
        draw_slash(s, x, y, t);
        if (s->mode == BSM_BUBBLE) {
            gfx_circb(x, y + 1, 10, (t / 10) & 1 ? C_WHITE : C_ICE);
            gfx_pset(x - 5, y - 5, C_WHITE);
            gfx_pset(x - 6, y - 3, C_WHITE);
        }
    } else if (s->mode == BSM_CLEAR || s->mode == BSM_WARP) {
        /* drawn into the ring (or the sparkle) */
        int k = s->mode_t, tx = s->mode == BSM_CLEAR ? s->exit_x : s->warp_x, ty = s->mode == BSM_CLEAR ? s->exit_y : s->warp_y;
        int px = x + (tx - x) * imin(k, 20) / 20, py = y + (ty - y) * imin(k, 20) / 20;
        if (k < 24) bhp_draw_ship(px, py, (k / 3) & 1 ? 1 : -1, false, t);
        for (int r = 0; r < 3; r++) gfx_circb(tx, ty, (k * 2 + r * 6) % 24, r & 1 ? C_WHITE : C_SKY);
    }
}
