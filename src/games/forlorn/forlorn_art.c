/* FORLORN HOPE - pixel art (palette-letter strings, see gfx.h). All drawn
 * for UFO 40. The volunteers share one body drawn with stand-in letters
 * (H hat, T tunic, S skin, L legs) that each trade colours its own way. */
#include "forlorn.h"

/* ------------------------------------------------------------------ */
/* the volunteers                                                       */

static const char BODY[4][81] = {
    /* stand */
    "...kkk.."
    "..kHHHk."
    ".kHHHHHk"
    "..kSkSk."
    "..kSSSk."
    ".kkTTTkk"
    ".STTTTTS"
    "..kTTTk."
    "..kL.Lk."
    "..kk.kk.",
    /* step */
    "...kkk.."
    "..kHHHk."
    ".kHHHHHk"
    "..kSkSk."
    "..kSSSk."
    ".kkTTTkk"
    ".STTTTTS"
    "..kTTTk."
    ".kL...Lk"
    ".kk...kk",
    /* stride */
    "...kkk.."
    "..kHHHk."
    ".kHHHHHk"
    "..kSkSk."
    "..kSSSk."
    ".kkTTTkk"
    "..STTTS."
    "..kTTTk."
    "...kLk.."
    "...kkk..",
    /* jump */
    "...kkk.."
    "..kHHHk."
    ".kHHHHHk"
    "S.kSkSkS"
    "k.kSSSkk"
    ".kkTTTk."
    "..kTTTk."
    "..kTTTk."
    ".kL..kL."
    ".k....k.",
};

/* hat, tunic, skin, legs for each trade */
static const char TRADE_COL[FRC_COUNT][4] = {
    {'g', 'b', 'h', 'e'}, /* mason: a grey hood, a leather apron */
    {'f', 'z', 't', 'e'}, /* hunter: green */
    {'r', 'o', 'h', 'b'}, /* runner: a red scarf */
    {'a', 'B', 't', 'N'}, /* tinker: goggles and blue overalls */
    {'s', 'd', 'h', 'n'}, /* sapper: soot and a dark coat */
};

static Sprite body[FRC_COUNT][4];

/* ------------------------------------------------------------------ */
/* the foes                                                             */

static const char EYE_OPEN[] =
    ".........."
    "..kkkkkk.."
    ".kwwwwwwk."
    "kwwwrrwwwk"
    "kwwrkkrwwk"
    "kwwrkkrwwk"
    "kwwwrrwwwk"
    ".kwwwwwwk."
    "..kkkkkk.."
    "..........";
static const char EYE_SHUT[] =
    ".........."
    ".........."
    "..kkkkkk.."
    ".k......k."
    "kkkkkkkkkk"
    ".kvvvvvvk."
    "..kkkkkk.."
    ".........."
    ".........."
    "..........";
static const char OOZLE1[] =
    "..jjjj.."
    ".jziizj."
    "jzikzikj"
    "jzzzzzzj"
    ".jjjjjj.";
static const char OOZLE2[] =
    "........"
    "..jjjj.."
    ".jzikzij"
    "jzzzzzzj"
    "jjjjjjjj";
static const char MIDGE1[] =
    "I.II.I"
    ".IkkI."
    "..kk.."
    ".kVVk."
    "..kk.."
    "......";
static const char MIDGE2[] =
    "......"
    "IIkkII"
    "..kk.."
    ".kVVk."
    "..kk.."
    "......";
static const char HORNET1[] =
    "..II.II."
    "...IIII."
    ".kyykyk."
    "kkyykykr"
    ".kyykyk."
    "..k..k.."
    "........";
static const char HORNET2[] =
    "........"
    ".IIIIII."
    ".kyykyk."
    "kkyykykr"
    ".kyykyk."
    "..k..k.."
    "........";
static const char SHELL1[] =
    "....kkkk.."
    "...kgllgk."
    "..kglsslgk"
    "..kgsgglgk"
    "k.kkgllgk."
    "kkttkkkkk."
    "kttttttttk"
    ".kkkkkkkk.";
static const char SHELL2[] =
    "....kkkk.."
    "...kgllgk."
    "..kglsslgk"
    "k.kgsgglgk"
    "kkkkgllgk."
    "ktttkkkkk."
    ".ktttttttk"
    "..kkkkkkk.";
static const char HATCH1[] =
    "..kkkk...."
    ".kjjjjk..."
    "kjwwkjjk.."
    "kjwkkjjk.."
    ".kjjjjk.ss"
    "..kbbk.kgs"
    ".kbbbbkbk."
    "kjkbbkjk.."
    "..kbbk...."
    "..kjkjk..."
    ".kjk.kjk.."
    ".kk...kk..";
static const char HATCH2[] =
    "..kkkk..ss"
    ".kjjjjk.gs"
    "kjwwkjjkk."
    "kjwkkjjkb."
    ".kjjjjkb.."
    "..kbbkk..."
    ".kbbbbk..."
    "kjkbbkjk.."
    "..kbbk...."
    "..kjkjk..."
    ".kjk.kjk.."
    ".kk...kk..";
static const char SQUAWK1[] =
    "..kkkk.."
    ".kyyyyk."
    "kokyyyyk"
    "kkkwkyyk"
    ".kyyyyyk"
    ".kyyyyk."
    "..kaak.."
    "..k..k..";
static const char SQUAWK2[] =
    "..kkkk.."
    ".kyyyyk."
    "kokyyyyk"
    "kkkwkyyk"
    "kyyyyyyk"
    "kyyyyyk."
    ".kaaak.."
    ".k...k..";
static const char TUSK1[] =
    "....kkkkkkkk...."
    "..kkbbbbbbbbkk.."
    ".kbbbbbebebbbbk."
    "kwkbbbbbbbbbbbbk"
    "kwbbrbbbbbbbbbbk"
    ".kbbbbbbbbbbbbk."
    "..ktbkbbbbkbtk.."
    "..kbk.kbbk.kbk.."
    "..kk...kk...kk.."
    "................";
static const char TUSK2[] =
    "....kkkkkkkk...."
    "..kkbbbbbbbbkk.."
    ".kbbbbbebebbbbk."
    "kwkbbbbbbbbbbbbk"
    "kwbbrbbbbbbbbbbk"
    ".kbbbbbbbbbbbbk."
    "..kbtkbbbbktbk.."
    "...kbk.kk.kbk..."
    "...kk..kk..kk..."
    "................";
static const char IDOL1[] =
    "..kkkkkkkk.."
    ".kgggggggggk"
    "kglllgglllgk"
    "kgkkkggkkkgk"
    "kglrlgglrlgk"
    "kgggggggggsk"
    "kgggkkkkggsk"
    "kggkvvvvkgsk"
    "kggkvvvvkgsk"
    "kgggkkkkggsk"
    ".kgssssssssk"
    "..kkkkkkkkk.";
static const char IDOL2[] =
    "..kkkkkkkk.."
    ".kgggggggggk"
    "kglllgglllgk"
    "kgkkkggkkkgk"
    "kglrlgglrlgk"
    "kgggggggggsk"
    "kggkkkkkkgsk"
    "kgkvvvvvvkgk"
    "kgkvvIIvvkgk"
    "kggkkkkkkgsk"
    ".kgssssssssk"
    "..kkkkkkkkk.";
static const char DRAKE1[] =
    "........kk......"
    ".....kkkqqk....."
    "..kkkqqqqqqk...."
    ".kjqqqqqqqqqk.kk"
    "kwkjqqqqjjqqkkqk"
    "kkrjqqqjjjjqqqk."
    ".kkkjqqjjjjqqk.."
    "...kjjqqqqqqk..."
    "....kqqkkqqk...."
    "....kqk..kqk...."
    "...kkk..kkk....."
    "................";
static const char DRAKE2[] =
    "................"
    ".....kkk.k......"
    "..kkkqqqkqk....."
    ".kjqqqqqqqqqk..."
    "kwkjqqqqjjqqk.kk"
    "kkrjqqqjjjjqqkqk"
    "okkkjqqjjjjqqqk."
    "o..kjjqqqqqqk..."
    "....kqqkkqqk...."
    "....kqk..kqk...."
    "...kkk..kkk....."
    "................";
static const char BELL1[] =
    "....kkkk...."
    "...ktcctk..."
    "..ktcttctk.."
    ".ktcttttctk."
    ".kcttccttck."
    "ktttcttcttk."
    "kcttttttttck"
    "kttcttttcttk"
    "kcttttttttck"
    "kttcttttcttk"
    ".ktttkktttk."
    ".kcttkkttck."
    "..kkkkkkkk.."
    "............";
static const char KNIGHT1[] =
    "...kkkk..."
    "..kssssk.."
    "..kskkskk."
    "..ksrrsk.."
    "..kssssk.."
    ".kkooookk."
    "kgkoaookgk"
    "kgkooaokgk"
    "kgkoooookk"
    ".kkkkkkk.."
    "..koook..."
    "..kokok..."
    ".kok.kok.."
    ".kk...kk..";
static const char KNIGHT2[] =
    "...kkkk..."
    "..kssssk.l"
    "..kskkskkl"
    "..ksrrskl."
    "..ksssskl."
    ".kkooookk."
    ".kkoaookk."
    ".kkooaok.."
    ".kkoooook."
    ".kkkkkkk.."
    "..koook..."
    "..kokok..."
    ".kok.kok.."
    ".kk...kk..";

typedef struct { const char *d; int w, h; } SprDef;
enum {
    SP_EYE_OPEN, SP_EYE_SHUT, SP_OOZLE1, SP_OOZLE2, SP_MIDGE1, SP_MIDGE2, SP_HORNET1, SP_HORNET2,
    SP_SHELL1, SP_SHELL2, SP_HATCH1, SP_HATCH2, SP_SQUAWK1, SP_SQUAWK2, SP_TUSK1, SP_TUSK2,
    SP_IDOL1, SP_IDOL2, SP_DRAKE1, SP_DRAKE2, SP_BELL1, SP_KNIGHT1, SP_KNIGHT2, SP_COUNT
};
static const SprDef DEFS[SP_COUNT] = {
    {EYE_OPEN, 10, 10}, {EYE_SHUT, 10, 10}, {OOZLE1, 8, 5}, {OOZLE2, 8, 5}, {MIDGE1, 6, 6}, {MIDGE2, 6, 6},
    {HORNET1, 8, 7}, {HORNET2, 8, 7}, {SHELL1, 10, 8}, {SHELL2, 10, 8}, {HATCH1, 10, 12}, {HATCH2, 10, 12},
    {SQUAWK1, 8, 8}, {SQUAWK2, 8, 8}, {TUSK1, 16, 10}, {TUSK2, 16, 10}, {IDOL1, 12, 12}, {IDOL2, 12, 12},
    {DRAKE1, 16, 12}, {DRAKE2, 16, 12}, {BELL1, 12, 14}, {KNIGHT1, 10, 14}, {KNIGHT2, 10, 14},
};
static Sprite spr[SP_COUNT];
static bool loaded, art_ok;

void frl_art_load(void) {
    if (loaded) return;
    loaded = true;
    art_ok = true;
    for (int c = 0; c < FRC_COUNT; c++)
        for (int f = 0; f < 4; f++) {
            char buf[81];
            memcpy(buf, BODY[f], 81);
            for (int i = 0; i < 80; i++) {
                switch (buf[i]) {
                case 'H': buf[i] = TRADE_COL[c][0]; break;
                case 'T': buf[i] = TRADE_COL[c][1]; break;
                case 'S': buf[i] = TRADE_COL[c][2]; break;
                case 'L': buf[i] = TRADE_COL[c][3]; break;
                default: break;
                }
            }
            if ((int)strlen(BODY[f]) != 80) art_ok = false;
            spr_make(&body[c][f], 8, 10, buf);
        }
    for (int i = 0; i < SP_COUNT; i++) {
        if ((int)strlen(DEFS[i].d) != DEFS[i].w * DEFS[i].h) art_ok = false;
        spr_make(&spr[i], DEFS[i].w, DEFS[i].h, DEFS[i].d);
    }
}

bool frl_art_ok(void) { return art_ok; }

/* ------------------------------------------------------------------ */
/* drawing helpers                                                      */

/* the trade's tool, in the hand on the facing side */
static void draw_tool(int cls, int x, int y, int face, int frame, bool swing) {
    int hx = face > 0 ? x + 7 : x;  /* the hand */
    int hy = y + 6;
    switch (cls) {
    case FRC_MASON:
        if (swing) {
            /* the mallet comes down in front */
            int mx = face > 0 ? x + 9 : x - 5;
            gfx_rect(mx, y + 3, 4, 3, C_GREY);
            gfx_rectb(mx, y + 3, 4, 3, C_INK);
            gfx_hline(face > 0 ? x + 7 : x - 1, face > 0 ? x + 9 : x + 1, y + 6, C_BROWN);
        } else {
            gfx_vline(hx, hy - 4, hy, C_BROWN);
            gfx_rect(hx - 1, hy - 6, 3, 3, C_GREY);
            gfx_pset(hx - 1, hy - 6, C_LIGHT);
        }
        break;
    case FRC_HUNTER: {
        int x0 = face > 0 ? x + 4 : x - 4, x1 = face > 0 ? x + 12 : x + 4;
        gfx_hline(x0, x1, hy - 1, C_INK);
        gfx_hline(face > 0 ? x0 : x1 - 2, face > 0 ? x0 + 2 : x1, hy, C_BROWN);
        break;
    }
    case FRC_RUNNER:
        /* the scarf streams out behind */
        gfx_hline(face > 0 ? x - 2 : x + 8, face > 0 ? x + 1 : x + 10, y + 4 + (frame & 1), C_RED);
        break;
    case FRC_TINKER:
        gfx_vline(hx, hy - 3, hy, C_LIGHT);
        gfx_pset(hx - 1, hy - 4, C_LIGHT);
        gfx_pset(hx + 1, hy - 4, C_LIGHT);
        gfx_pset(x + 3, y + 3, C_CYAN); /* goggles */
        break;
    case FRC_SAPPER: {
        /* the keg on the back, its fuse lit */
        int kx = face > 0 ? x - 3 : x + 7;
        gfx_rect(kx, y + 4, 4, 5, C_BROWN);
        gfx_hline(kx, kx + 3, y + 5, C_INK);
        gfx_hline(kx, kx + 3, y + 7, C_INK);
        gfx_pset(kx + 1, y + 3, C_GREY);
        gfx_pset(kx + 1 + (frame & 1), y + 2, (frame & 2) ? C_YELLOW : C_ORANGE);
        break;
    }
    default: break;
    }
}

void frl_draw_unit_at(int cls, int x, int y, int face, int frame, bool flash, int player) {
    int f = frame & 3;
    int flags = face < 0 ? SPR_FLIPX : 0;
    if (flash) {
        static uint8_t white[256];
        static bool init;
        if (!init) { for (int i = 0; i < 256; i++) white[i] = C_WHITE; white[C_INK] = C_INK; init = true; }
        spr_draw_ex(&body[cls][f], x, y, flags, white, -1);
    } else {
        spr_draw(&body[cls][f], x, y, flags);
    }
    draw_tool(cls, x, y, face, frame, false);
    if (player == 1) {
        gfx_pset(x + 3, y - 2, C_CYAN);
        gfx_pset(x + 4, y - 2, C_CYAN);
        gfx_pset(x + 3, y - 3, C_ICE);
    }
}

/* a volunteer drawn as the world sees them (with the swing) */
void frl_draw_unit_full(int cls, int x, int y, int face, int frame, bool flash, int player, bool swing) {
    if (swing) {
        /* the mallet's down stroke replaces the one held up */
        int f = frame & 3;
        spr_draw(&body[cls][f], x, y, face < 0 ? SPR_FLIPX : 0);
        draw_tool(cls, x, y, face, frame, true);
        if (flash) frl_draw_unit_at(cls, x, y, face, frame, flash, player);
        return;
    }
    frl_draw_unit_at(cls, x, y, face, frame, flash, player);
}

void frl_draw_heart(int x, int y, int t, int scale) {
    /* a dark red heart in a cage of thorns, beating */
    int beat = (t / 6) % 8 < 2 ? 1 : 0;
    int cx = x + 10 * scale, cy = y + 10 * scale;
    int r = (5 + beat) * scale;
    gfx_circ(cx - r / 2 - scale, cy - 2 * scale, r, C_WINE);
    gfx_circ(cx + r / 2 + scale, cy - 2 * scale, r, C_WINE);
    for (int k = 0; k < 8 * scale; k++) gfx_hline(cx - (8 - k / scale) * scale - beat, cx + (8 - k / scale) * scale + beat, cy + k, C_WINE);
    gfx_circ(cx - r / 2 - scale, cy - 3 * scale, r / 2, C_RED);
    gfx_pset(cx - r / 2 - 2 * scale, cy - 5 * scale, C_PINK);
    /* thorns */
    for (int k = 0; k < 6; k++) {
        int a = k * 43 + t;
        int tx = cx + (int)(9.0f * scale * cosf((float)a * 0.0245f));
        int ty = cy + (int)(8.0f * scale * sinf((float)a * 0.0245f));
        gfx_line(cx, cy, tx, ty, C_INK);
        gfx_pset(tx, ty, C_LIME);
    }
    gfx_line(cx - 9 * scale, cy - 6 * scale, cx + 9 * scale, cy + 6 * scale, C_FOREST);
    gfx_line(cx - 9 * scale, cy + 6 * scale, cx + 9 * scale, cy - 6 * scale, C_FOREST);
}

static void draw_big(const FrlFoe *f, int x, int y, int t) {
    int col_flash = f->flash && (f->flash & 2);
    switch (f->kind) {
    case FK_BLOATER: {
        int pulse = (t / 10) & 1;
        gfx_circ(x + 10, y + 11, 9 + pulse, C_INK);
        gfx_circ(x + 10, y + 11, 8 + pulse, col_flash ? C_WHITE : C_VIOLET);
        gfx_circ(x + 8, y + 8, 3, C_PINK);
        gfx_circ(x + 6, y + 10, 2, C_WHITE);
        gfx_circ(x + 13, y + 10, 2, C_WHITE);
        gfx_pset(x + 6, y + 10, C_INK);
        gfx_pset(x + 13, y + 10, C_INK);
        gfx_rect(x + 7, y + 14, 6, 2 + pulse, C_INK);
        for (int k = 0; k < 5; k++) gfx_pset(x + 2 + k * 4, y + 19, C_PURPLE);
        break;
    }
    case FK_BROODHEN: {
        bool l = f->face < 0;
        int sx = l ? 1 : -1, bx = x + 9;
        gfx_circ(bx, y + 11, 7, C_INK);
        gfx_circ(bx, y + 11, 6, col_flash ? C_WHITE : C_CREAM);
        gfx_circ(bx - sx * 6, y + 5, 4, C_INK);
        gfx_circ(bx - sx * 6, y + 5, 3, col_flash ? C_WHITE : C_CREAM);
        gfx_rect(bx - sx * 7 - 1, y, 3, 2, C_RED);         /* comb */
        gfx_pset(bx - sx * 7, y + 4, C_INK);               /* eye */
        gfx_rect(l ? bx - 13 : bx + 10, y + 5, 3, 2, C_AMBER); /* beak */
        gfx_pset(bx - sx * 6, y + 8, C_RED);
        gfx_line(bx + sx * 4, y + 6, bx + sx * 8, y + 3, C_GREY);  /* tail */
        gfx_line(bx + sx * 5, y + 8, bx + sx * 9, y + 6, C_GREY);
        gfx_vline(bx - 2, y + 16, y + 17, C_AMBER);
        gfx_vline(bx + 2, y + 16, y + 17, C_AMBER);
        break;
    }
    case FK_HORNHEAD: {
        bool l = f->face < 0;
        gfx_rect(x + 3, y + 8, 12, 10, C_INK);
        gfx_rect(x + 4, y + 9, 10, 8, col_flash ? C_WHITE : C_BROWN);
        gfx_rect(x + 5, y + 2, 8, 7, C_INK);
        gfx_rect(x + 6, y + 3, 6, 5, col_flash ? C_WHITE : C_EARTH);
        gfx_line(x + 5, y + 3, x + 1, y, C_CREAM);       /* horns */
        gfx_line(x + 12, y + 3, x + 16, y, C_CREAM);
        gfx_pset(l ? x + 7 : x + 10, y + 5, C_RED);
        gfx_rect(x + 7, y + 7, 4, 2, C_TAN);
        gfx_rect(x + 4, y + 18, 4, 4, C_INK);
        gfx_rect(x + 10, y + 18, 4, 4, C_INK);
        gfx_rect(x + 1, y + 10, 2, 6, C_EARTH);
        gfx_rect(x + 15, y + 10, 2, 6, C_EARTH);
        break;
    }
    case FK_STINGBACK: {
        bool l = f->face < 0;
        int hx = l ? x + 2 : x + 14;
        for (int k = 0; k < 4; k++) {
            int sx = l ? x + 6 + k * 4 : x + 12 - k * 4;
            gfx_rect(sx, y + 7, 5, 6, C_INK);
            gfx_rect(sx + 1, y + 8, 3, 4, col_flash ? C_WHITE : C_RED);
        }
        /* the tail over the back */
        int tx = l ? x + 20 : x + 1;
        gfx_line(tx, y + 8, tx, y + 2, C_MAROON);
        gfx_line(tx, y + 2, l ? tx - 6 : tx + 6, y, C_MAROON);
        gfx_pset(l ? tx - 7 : tx + 7, y + 1, C_YELLOW);
        /* claws */
        gfx_rect(hx, y + 6, 5, 4, C_INK);
        gfx_rect(hx + 1, y + 7, 3, 2, C_ORANGE);
        gfx_pset(l ? hx - 1 : hx + 5, y + 6, C_ORANGE);
        gfx_pset(l ? hx + 1 : hx + 3, y + 10, C_YELLOW);
        for (int k = 0; k < 3; k++) gfx_pset(x + 7 + k * 4, y + 13, C_INK);
        break;
    }
    case FK_GULPER: {
        /* the head of something long, mouth open, just over its shaft */
        int o = (t / 12) & 1;
        gfx_rect(x, y + 2, 20, 8, C_INK);
        gfx_rect(x + 1, y + 3, 18, 7, col_flash ? C_WHITE : C_RED);
        gfx_rect(x + 4, y + 4 + o, 12, 4 - o, C_INK);
        for (int k = 0; k < 4; k++) gfx_pset(x + 5 + k * 3, y + 4 + o, C_WHITE);
        gfx_pset(x + 2, y + 1, C_YELLOW);
        gfx_pset(x + 17, y + 1, C_YELLOW);
        gfx_pset(x + 2, y, C_INK);
        gfx_pset(x + 17, y, C_INK);
        break;
    }
    case FK_HEART:
        frl_draw_heart(x, y, t + f->hx, 1);
        if (col_flash) gfx_circb(x + 10, y + 10, 10, C_WHITE);
        break;
    default: break;
    }
}

void frl_draw_foe(const FrlFoe *f, int x, int y, int t) {
    int a = (t / 10) & 1;
    int flags = f->face > 0 ? SPR_FLIPX : 0;
    int s = -1;
    switch (f->kind) {
    case FK_WALLEYE: s = f->state ? SP_EYE_OPEN : SP_EYE_SHUT; flags = 0; break;
    case FK_OOZLE: s = a ? SP_OOZLE2 : SP_OOZLE1; break;
    case FK_MIDGE: s = (t / 3) & 1 ? SP_MIDGE2 : SP_MIDGE1; break;
    case FK_HORNET: s = (t / 2) & 1 ? SP_HORNET2 : SP_HORNET1; break;
    case FK_SHELLBACK: s = a ? SP_SHELL2 : SP_SHELL1; break;
    case FK_HATCHET: s = f->state ? SP_HATCH2 : SP_HATCH1; break;
    case FK_SQUAWKER: s = f->ground ? SP_SQUAWK1 : SP_SQUAWK2; break;
    case FK_TUSKER: s = (f->state == 1 ? (t / 3) & 1 : a) ? SP_TUSK2 : SP_TUSK1; break;
    case FK_IDOL: s = f->state ? SP_IDOL2 : SP_IDOL1; break;
    case FK_DRAKE: s = f->t2 >= 116 && f->t2 < 160 ? SP_DRAKE2 : SP_DRAKE1; break;
    case FK_BELL: s = SP_BELL1; flags = 0; break;
    case FK_KNIGHT: s = f->state ? SP_KNIGHT2 : SP_KNIGHT1; break;
    default: draw_big(f, x, y, t); return;
    }
    const Sprite *sp = &spr[s];
    int dx = x + (f->w - sp->w) / 2, dy = y + f->h - sp->h;
    if (f->kind == FK_BELL || f->kind == FK_WALLEYE) dy = y;
    if (f->flash && (f->flash & 2)) {
        static uint8_t white[256];
        static bool init;
        if (!init) { for (int i = 0; i < 256; i++) white[i] = C_WHITE; init = true; }
        spr_draw_ex(sp, dx, dy, flags, white, -1);
    } else {
        spr_draw(sp, dx, dy, flags);
    }
    if (f->kind == FK_BELL) {
        gfx_vline(dx + 6, dy - 3, dy - 1, C_TAN);
        int hole = (t / 8) % 3;
        gfx_pset(dx + 5 + hole, dy + 11, C_YELLOW);
    }
}

/* the card for a trade at the troop's door */
void frl_draw_class_card(int cls, int x, int y, bool sel, bool allowed, int t) {
    ui_panel(x, y, 52, 50, sel ? C_NIGHT : C_INK, sel ? C_YELLOW : (allowed ? C_SLATE : C_NIGHT));
    int fx = x + 22, fy = y + 8;
    int frame = sel ? (t / 8) % 3 : 0;
    if (!allowed) {
        spr_draw_ex(&body[cls][0], fx, fy, 0, NULL, C_SLATE);
    } else {
        frl_draw_unit_at(cls, fx, fy, 1, frame, false, 0);
    }
    text_center(FRL_CLASS_NAME[cls], x + 26, y + 22, sel ? C_YELLOW : allowed ? C_LIGHT : C_SLATE);
    static const char *const ATK[FRC_COUNT] = {"MALLET 2", "MUSKET 1", "KNIVES 1", "SPANNER 2", "NO ATTACK"};
    static const char *const GIFT[FRC_COUNT] = {"STONE", "POUCH", "WAYSTONE", "CHUTE", "BLAST 15"};
    tiny_center(ATK[cls], x + 26, y + 32, allowed ? C_GREY : C_SLATE);
    tiny_center(GIFT[cls], x + 26, y + 40, allowed ? (sel ? C_CREAM : C_GREY) : C_SLATE);
}

/* Thornkeep from afar: the title, the label and the ending */
void frl_draw_keep(int x, int y, int t, bool fallen) {
    if (!fallen) {
        gfx_rect(x + 10, y + 20, 60, 40, C_INK);
        gfx_rect(x, y + 8, 14, 52, C_INK);
        gfx_rect(x + 66, y + 4, 14, 56, C_INK);
        gfx_rect(x + 32, y, 16, 60, C_INK);
        for (int k = 0; k < 4; k++) {
            gfx_rect(x + k * 4, y + 6, 2, 2, C_INK);
            gfx_rect(x + 66 + k * 4, y + 2, 2, 2, C_INK);
            gfx_rect(x + 32 + k * 4, y - 2, 2, 2, C_INK);
        }
        /* lit windows, and the beat of the hearts inside */
        int beat = (t / 6) % 8 < 2;
        gfx_rect(x + 38, y + 14, 4, 6, beat ? C_RED : C_WINE);
        gfx_rect(x + 5, y + 20, 3, 4, C_AMBER);
        gfx_rect(x + 71, y + 16, 3, 4, C_AMBER);
        gfx_rect(x + 20, y + 32, 3, 4, C_WINE);
        gfx_rect(x + 56, y + 32, 3, 4, C_WINE);
        /* thorns climbing it */
        for (int k = 0; k < 9; k++) {
            int bx = x + 4 + k * 9;
            gfx_line(bx, y + 60, bx + 3, y + 40 - (k * 7) % 18, C_FOREST);
            gfx_pset(bx + 2, y + 48 - (k * 5) % 12, C_LIME);
        }
    } else {
        /* rubble, and statues in front of it */
        for (int k = 0; k < 12; k++) gfx_rect(x + k * 7, y + 48 + (k * 5) % 7, 8, 12, k & 1 ? C_SLATE : C_DUSK);
        gfx_rect(x + 32, y + 36, 10, 24, C_DUSK);
    }
}
