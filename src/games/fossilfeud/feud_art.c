/* FOSSIL FEUD - pixel art (palette-letter rows, see gfx.h). All drawn for
 * UFO 40: the five Ancestors' bones, the five kinds of bird (in the Rust
 * Roost's red; the Slate Flock wears them in blue by a palette swap), the
 * buildings and the resources. The ground, the totems and the battlefield
 * are drawn in feud_draw.c. */
#include "feud.h"

Sprite feud_spr[FSP_COUNT];
static int art_bad;

#define ROWS(name) name, ARRAY_LEN(name)

/* ---- the Ancestors: old bones, 20 x 14 ---------------------------------------- */
static const char *const DINO_JAW[] = {
    "......kkkkkkkk......",
    "....kkccccccccckk...",
    "...kcccckkcccccccck.",
    "..kccccckkccccccccck",
    "..kcccccccccccccccck",
    "..kccckkkkkkkkkcccck",
    "...kckwkwkwkwkwkcck.",
    "...kk..........kkk..",
    "...kckwkwkwkwkwck...",
    "....kcccccccccccck..",
    ".....kkkkkkkkkkkk...",
    "......kck....kck....",
    "......kk......kk....",
    "....................",
};
static const char *const DINO_PLATE[] = {
    "......k...k...k.....",
    ".....kok.kok.kok....",
    "....kooookoooookk...",
    "...kcccccccccccccck.",
    "..kccckcccccccccccck",
    ".kccccccccccccccccck",
    "kwcccccccccccccccck.",
    "kkcccccccccccccckk..",
    ".kkccck.kcck.kcck...",
    "...kcck..kck..kck...",
    "...kcck..kck..kck...",
    "...kkkk..kkk..kkk...",
    "....................",
    "....................",
};
static const char *const DINO_NECK[] = {
    "..kkk...............",
    ".kcckk..............",
    ".kwccck.............",
    "..kkcck.............",
    "....kcck............",
    ".....kcck...........",
    "......kcckkkkkk.....",
    ".......kcccccccckk..",
    "......kccccccccccck.",
    "......kcccccccccccck",
    ".......kcck..kcck.kk",
    ".......kcck..kcck...",
    ".......kkkk..kkkk...",
    "....................",
};
static const char *const DINO_CREST[] = {
    "kkk.................",
    "kcckk...............",
    ".kccckk.............",
    "..kkcwck............",
    "....kccckkkk........",
    "...kccccccccckkkk...",
    "..kcccccccccccccckk.",
    ".kcckkkkkkkkkkccccck",
    "kcck..........kkkkk.",
    "kck.................",
    "kk..................",
    "....................",
    "....................",
    "....................",
};
static const char *const DINO_HORN[] = {
    "..kk.kk.............",
    ".kpk.kpk............",
    "kppkkkpkkkk.........",
    "kpcccccccppk........",
    ".kcwcccccccpk.......",
    "kcccccccccccpkkkkk..",
    "kcckkcccccccccccccck",
    ".kk.kcccccccccccccck",
    ".....kccck.kcck.kck.",
    ".....kccck.kcck.kck.",
    ".....kkkkk.kkkk.kkk.",
    "....................",
    "....................",
    "....................",
};

/* ---- the birds, 9 x 9, facing right: walking, then striking -------------------- */
static const char *const ST0[] = {
    "...kkk...", "..krrrk..", "..krwkaak", "..krrrkk.", ".krrrrk..",
    "krvrrrrk.", "krvvrrrk.", ".kkaakk..", ".ka..ak..",
};
static const char *const ST1[] = {
    ".........", "....kkk..", "...krrrk.", "...krwkaa", "..krrrrkk",
    ".krvrrrk.", "krvvrrrk.", ".kkaakk..", "ka....ak.",
};
static const char *const LA0[] = {
    "..kk.....", ".krrk....", ".krwkaaa.", "..krk....", ".krrrkll.",
    "krrrrklwl", "krvvrklwl", ".kkkk.kl.", "..k.k....",
};
static const char *const LA1[] = {
    "..kk.....", ".krrk....", ".krwkaaaa", "..krk.ll.", ".krrrrklw",
    "krrrrrklw", "krvvrk.l.", ".kkkk....", ".k...k...",
};
static const char *const SL0[] = {
    ".........", "...kkk...", "..kwwwk..", "..kwkwaa.", "..kwwwk..",
    ".krrrrrk.", "krrrrrrbk", ".kkrrkkb.", "..ka.ak..",
};
static const char *const SL1[] = {
    "......b..", "...kkkb..", "..kwwwkb.", "..kwkwaa.", "..kwwwk..",
    ".krrrrrk.", "krrrrrrk.", ".kkrrkk..", "..ka.ak..",
};
static const char *const MY0[] = {
    "..k...k..", "..kkkkk..", ".kpwkwpk.", ".kppappkb", ".krrrrrkb",
    "krrvrrrkb", "krrvrrrkb", ".krrrrk.b", "..kk.kk.b",
};
static const char *const MY1[] = {
    "..k...k.y", "..kkkkkby", ".kpwkwpkb", ".kppappkb", ".krrrrrk.",
    "krrvrrrk.", "krrvrrrk.", ".krrrrk..", "..kk.kk..",
};
static const char *const SW0[] = {
    ".........", "kk.....kk", "krk...krk", ".krkkkrk.", "..krwkaa.",
    "...krk...", "....k....", ".........", ".........",
};
static const char *const SW1[] = {
    ".........", ".........", "...kkk...", "..krwkaa.", ".krrrrrk.",
    "krk...krk", "kk.....kk", ".........", "...b.....",
};

/* ---- 5 x 5 marks for the board ---------------------------------------------------- */
static const char *const MI_ST[] = {".kkk.", "krwka", ".krk.", "krrrk", ".a.a."};
static const char *const MI_LA[] = {".kk.l", "krwal", ".krkl", "krrkl", ".k.k."};
static const char *const MI_SL[] = {".kkk.", "kwkwa", ".kwk.", "krrrk", ".a.a."};
static const char *const MI_MY[] = {"k...k", "kpkpk", ".kak.", "krrrk", ".k.k."};
static const char *const MI_SW[] = {"k...k", "rk.kr", ".krk.", "..k..", "....."};

/* ---- buildings, 7 x 7 ---------------------------------------------------------------- */
static const char *const B_GRAN[] = {"..kkk..", ".kbbbk.", "kbbbbbk", "ktttttk", "ktykytk", "ktttttk", "kkkkkkk"};
static const char *const B_ROOST[] = {"k.....k", "kkbbbkk", "kbbbbbk", ".kbbbk.", "..b.b..", "..b.b..", ".kk.kk."};
static const char *const B_SHRINE[] = {"...k...", "..kpk..", ".kpppk.", "kpppppk", ".kpwpk.", ".kpkpk.", "kkkkkkk"};
static const char *const B_BATH[] = {".......", "kkkkkkk", "kuuIuuk", ".kuuuk.", "..klk..", "..klk..", ".kkkkk."};

/* ---- resources, 5 x 5 ------------------------------------------------------------------ */
static const char *const R_GRAIN[] = {".y.y.", "yayay", ".yay.", "..e..", "..e.."};
static const char *const R_TOIL[] = {".kk..", "kcwka", "kcck.", ".kck.", ".k.k."};
static const char *const R_RELIC[] = {"kk...", "kckk.", ".kcck", "..kck", "...kk"};

/* ---- small marks ------------------------------------------------------------------------- */
static const char *const TOTEM[] = {"kkkk..", "kPPPk.", "kPwPPk", "kPPPk.", "kkkk..", "k.....", "k.....", "k.....", "kk...."};
static const char *const SNOW[] = {"...I...", ".I.I.I.", "..III..", "IIIwIII", "..III..", ".I.I.I.", "...I..."};
static const char *const EYE[] = {".kkkkk.", "kwwkwwk", "kwkkkwk", "kwwkwwk", ".kkkkk."};
static const char *const STAR[] = {"..y..", ".yyy.", "yyyyy", ".yay.", "y...y"};
static const char *const STAR_OFF[] = {"..s..", ".s.s.", "s...s", ".s.s.", "s...s"};

/* ---- the title's two birds, 16 x 16 ---------------------------------------------------- */
static const char *const BIG_RED[] = {
    "......kkkk......",
    ".....krrrrk.....",
    "....krrrrrrk....",
    "....krwkrrrkk...",
    "....krkkrrkaak..",
    "....krrrrrkaaak.",
    "....kvrrrrkkkk..",
    "...kvvrrrrrk....",
    "..kvvvrrrrrrk...",
    ".kvvvvrrrrrrrk..",
    "kvvvvvrrrrrrrk..",
    "kvvvvrrrrrrrk...",
    ".kvvrrrrrrrk....",
    "..kkkrrrrkk.....",
    "....kak.kak.....",
    "...kaak.kaak....",
};
static const char *const BIG_BLUE[] = {
    "......kkkk......",
    ".....kBBBBk.....",
    "....kBBBBBBk....",
    "...kkBBBkwBk....",
    "..kaakBBkkBk....",
    ".kaaakBBBBBk....",
    "..kkkkBBBBNk....",
    "....kBBBBBNNk...",
    "...kBBBBBBNNNk..",
    "..kBBBBBBBNNNNk.",
    "..kBBBBBBBNNNNNk",
    "...kBBBBBBBNNNNk",
    "....kBBBBBBBNNk.",
    ".....kkBBBBkkk..",
    ".....kak.kak....",
    "....kaak.kaak...",
};

static void make(int id, const char *const *rows, int nrows, int w) {
    static char buf[64 * 64 + 1];
    int h = nrows;
    for (int r = 0; r < h; r++) {
        if ((int)strlen(rows[r]) != w) art_bad++;
        for (int c = 0; c < w; c++) buf[r * w + c] = c < (int)strlen(rows[r]) ? rows[r][c] : '.';
    }
    buf[w * h] = 0;
    spr_make(&feud_spr[id], w, h, buf);
}

void feud_art_load(void) {
    if (feud_spr[FSP_DINO0].px) return;
    make(FSP_DINO0 + 0, ROWS(DINO_JAW), 20);
    make(FSP_DINO0 + 1, ROWS(DINO_PLATE), 20);
    make(FSP_DINO0 + 2, ROWS(DINO_NECK), 20);
    make(FSP_DINO0 + 3, ROWS(DINO_CREST), 20);
    make(FSP_DINO0 + 4, ROWS(DINO_HORN), 20);
    const char *const *u[FU_KINDS][2] = {{ST0, ST1}, {LA0, LA1}, {SL0, SL1}, {MY0, MY1}, {SW0, SW1}};
    for (int k = 0; k < FU_KINDS; k++)
        for (int f = 0; f < 2; f++) make(FSP_UNIT0 + k * 2 + f, u[k][f], 9, 9);
    const char *const *mi[FU_KINDS] = {MI_ST, MI_LA, MI_SL, MI_MY, MI_SW};
    for (int k = 0; k < FU_KINDS; k++) make(FSP_MINI0 + k, mi[k], 5, 5);
    make(FSP_BLD0 + FB_GRANARY, ROWS(B_GRAN), 7);
    make(FSP_BLD0 + FB_ROOST, ROWS(B_ROOST), 7);
    make(FSP_BLD0 + FB_SHRINE, ROWS(B_SHRINE), 7);
    make(FSP_BLD0 + FB_BATH, ROWS(B_BATH), 7);
    make(FSP_RES0 + FR_GRAIN, ROWS(R_GRAIN), 5);
    make(FSP_RES0 + FR_TOILERS, ROWS(R_TOIL), 5);
    make(FSP_RES0 + FR_RELICS, ROWS(R_RELIC), 5);
    make(FSP_TOTEM, ROWS(TOTEM), 6);
    make(FSP_SNOW, ROWS(SNOW), 7);
    make(FSP_EYE, ROWS(EYE), 7);
    make(FSP_STAR, ROWS(STAR), 5);
    make(FSP_STAR_OFF, ROWS(STAR_OFF), 5);
    make(FSP_BIRD_BIG, ROWS(BIG_RED), 16);
    make(FSP_BIRD_BIG2, ROWS(BIG_BLUE), 16);
}

int feud_art_check(void) { return art_bad; }
