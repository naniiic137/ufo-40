/* DUKES UP - pixel art, all drawn for UFO 40. Things on the floor are
 * palette-letter strings (see gfx.h); people and ghouls are posed figures
 * built from limbs, so every one of them can punch, kick, fall and get up
 * in every direction without a sheet of frames. */
#include "dku.h"

Sprite dku_spr[SPR_COUNT];

/* ---- things on the floor (10 x 8) ----------------------------------------- */
static const char IT_APPLE_S[] =
    "....kk...."
    "...kfk...."
    "..kkrkkk.."
    ".krrwrrrk."
    ".krwrrrrk."
    ".krrrrrrk."
    "..krrrrk.."
    "...kkkk...";
static const char IT_SANDWICH_S[] =
    ".........."
    "..kkkkkk.."
    ".kttttttk."
    "kzzizzizzk"
    "kryrryrryk"
    "kttttttttk"
    ".kkkkkkkk."
    "..........";
static const char IT_DRUMSTICK_S[] =
    ".........."
    "....kkkk.."
    "...kbbbbk."
    "..kbbtbbk."
    ".kkbbbbk.."
    "kwkkbkk..."
    "kwwk......"
    ".kk.......";
static const char IT_ROAST_S[] =
    "..k....k.."
    ".kwk..kwk."
    "..kkkkkk.."
    ".kbbtbbbk."
    "kbbbbbtbbk"
    "kbbtbbbbbk"
    "kwwwwwwwwk"
    ".kkkkkkkk.";
static const char IT_COIN_S[] =
    ".........."
    ".........."
    "....kkk..."
    "...kyyak.."
    "..kkkkkkk."
    ".kyyakyyak"
    ".kkkkkkkk."
    "..........";
static const char IT_NOTE_S[] =
    ".........."
    ".........."
    "kkkkkkkkk."
    "kjjjjjjjk."
    "kjikkkijk."
    "kjjjjjjjk."
    "kkkkkkkkk."
    "..........";
static const char IT_RING_S[] =
    "....kk...."
    "...kCCk..."
    "..kkkkkk.."
    ".kyk..kyk."
    ".kak..kak."
    ".kyykkyyk."
    "..kaaaak.."
    "...kkkk...";
static const char IT_PLANK_S[] =
    ".........."
    ".........."
    ".........."
    "kkkkkkkkkk"
    "kbtbbbbtbk"
    "kbbbbtbbbk"
    "kkkkkkkkkk"
    "..........";
static const char IT_CHAIN_S[] =
    ".........."
    ".........."
    ".kk.kk.kk."
    "kglkglkglk"
    "kllkllkllk"
    ".kk.kk.kk."
    ".........."
    "..........";
static const char IT_PIPE_S[] =
    ".........."
    ".........."
    ".........."
    "kkkkkkkkkk"
    "klllllllgk"
    "kggggggggk"
    "kkkkkkkkkk"
    "..........";
static const char IT_ARM_S[] =
    ".........."
    ".........."
    "kk........"
    "kjkkkkkk.."
    "kizzzzzzkk"
    "kjzzzzzzzk"
    "kkkkkkkkk."
    "..........";
static const char IT_SCATTER_S[] =
    ".........."
    ".........."
    "kkkkkkkk.."
    "kssssssgkk"
    "kkkkbbbbbk"
    "....kbbkk."
    "....kkk..."
    "..........";
static const char IT_SAW_S[] =
    ".........."
    "..kkkkkk.."
    ".kgggggk.."
    "kroorkgkkk"
    "krrrrkllgk"
    "kkkkkkgggk"
    "..k..kkkk."
    "..........";
static const char IT_BIN_S[] =
    ".kkkkkkkk."
    "kslllllssk"
    "kkkkkkkkkk"
    ".kgslgsgk."
    ".kgslgsgk."
    ".kgslgsgk."
    ".kgslgsgk."
    ".kkkkkkkk.";
static const char IT_HEAD_S[] =
    ".........."
    "...kkkk..."
    "..kjzzjk.."
    ".kzkzzkzk."
    ".kzzzzzzk."
    ".kzkkkkzk."
    "..kjjjjk.."
    "...kkkk...";
static const char IT_CLEAVER_S[] =
    ".........."
    "kkkkkk...."
    "kllllgk..."
    "kllllgkkkk"
    "klllllkbbk"
    "kkkkkkkkkk"
    ".........."
    "..........";
static const char IT_BOTTLE_S[] =
    "....yo...."
    "....or...."
    "....kk...."
    "...kfk...."
    "..kfjfk..."
    "..kjjjk..."
    "..kjjjk..."
    "...kkk....";

/* ---- props --------------------------------------------------------------- */
static const char PR_BIN_S[] =
    "..kkkkkkkk.."
    ".klllllllsk."
    ".kkkkkkkkkk."
    "..kgslgsgk.."
    "..kgslgsgk.."
    "..kgslgsgk.."
    "..kgslgsgk.."
    "..kgslgsgk.."
    "..kgslgsgk.."
    "..kkkkkkkk..";
static const char PR_BOX_S[] =
    "............"
    "............"
    "kkkkkkkkkkkk"
    "kttttttttttk"
    "ktttbbbbtttk"
    "kttttttttttk"
    "kbbbbbbbbbbk"
    "kttttttttttk"
    "kttttttttttk"
    "kkkkkkkkkkkk";
static const char PR_CRATE_S[] =
    "kkkkkkkkkkkk"
    "kbtttttttttk"
    "kbkkkkkkkkbk"
    "kbtbtttttbbk"
    "kbttbtttbtbk"
    "kbtttbtbttbk"
    "kbttttbtttbk"
    "kbtttbtbttbk"
    "kbkkkkkkkkbk"
    "kkkkkkkkkkkk";
static const char PR_STUMP_S[] =
    "............"
    "............"
    "..kkkkkkkk.."
    ".kttbtttbtk."
    ".ktbbbtttbk."
    ".kkttbbttkk."
    ".kbbebbbebk."
    ".kbebbbebbk."
    "kbbbbebbbbbk"
    "kkkkkkkkkkkk";
static const char PR_SIGN_S[] =
    "kkkkkkkkkkkk"
    "kwwwwwwwwwwk"
    "kwrrwrwrrrwk"
    "kwwwwwwwwwwk"
    "kwrrrwrrwrwk"
    "kkkkkkkkkkkk"
    ".....kk....."
    ".....kk....."
    ".....kk....."
    "....kkkk....";
static const char PR_VASE_S[] =
    "....kkkk...."
    "...kNNNNk..."
    "....kBBk...."
    "...kBBBBk..."
    "..kBIIBBBk.."
    "..kBIBBBBk.."
    "..kBBBBBBk.."
    "..kNBBBBNk.."
    "...kNNNNk..."
    "....kkkk....";
static const char PR_JUNK_S[] =
    "............"
    "............"
    "............"
    "....kkk....."
    "..kkgsgkkk.."
    ".kbbkskbtbk."
    "kttbbkksgbbk"
    "kgsttbbkbbsk"
    "kbbgsstbbttk"
    "kkkkkkkkkkkk";
static const char PR_TUFT_S[] =
    "............"
    "............"
    "............"
    "............"
    "..k...k..k.."
    ".kzk.kzkkzk."
    ".kzkkzjkzjk."
    "kzjzkzjzzjzk"
    "kjzjjzjjjzjk"
    "kkkkkkkkkkkk";
static const char PR_POST_S[] =
    "....kkkk...."
    "....kbtk...."
    "....kbtk...."
    "....kbtk...."
    "....kbtk...."
    "....kbtk...."
    "....kbtk...."
    "....kbtk...."
    "....kbtk...."
    "...kkkkkk...";

void dku_art_load(void) {
    if (dku_spr[SPR_ITEM0 + IT_APPLE].px) return;
    static const char *const ITEMS[IT_COUNT] = {
        [IT_APPLE] = IT_APPLE_S, [IT_SANDWICH] = IT_SANDWICH_S, [IT_DRUMSTICK] = IT_DRUMSTICK_S, [IT_ROAST] = IT_ROAST_S,
        [IT_COIN] = IT_COIN_S, [IT_NOTE] = IT_NOTE_S, [IT_RING] = IT_RING_S, [IT_PLANK] = IT_PLANK_S,
        [IT_CHAIN] = IT_CHAIN_S, [IT_PIPE] = IT_PIPE_S, [IT_ARM] = IT_ARM_S, [IT_SCATTER] = IT_SCATTER_S,
        [IT_SAW] = IT_SAW_S, [IT_BIN] = IT_BIN_S, [IT_HEAD] = IT_HEAD_S, [IT_CLEAVER] = IT_CLEAVER_S,
        [IT_BOTTLE] = IT_BOTTLE_S,
    };
    static const char *const PROPS[PR_COUNT] = {PR_BIN_S, PR_BOX_S, PR_CRATE_S, PR_STUMP_S, PR_SIGN_S,
                                                PR_VASE_S, PR_JUNK_S, PR_TUFT_S, PR_POST_S};
    for (int i = 1; i < IT_COUNT; i++) spr_make(&dku_spr[SPR_ITEM0 + i], 10, 8, ITEMS[i]);
    for (int i = 0; i < PR_COUNT; i++) spr_make(&dku_spr[SPR_PROP0 + i], 12, 10, PROPS[i]);
}

/* ---- posed figures ------------------------------------------------------- */

/* a line with a square brush */
static void thick_line(int x0, int y0, int x1, int y1, int w, int c) {
    int dx = iabs(x1 - x0), dy = -iabs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int o = w / 2;
    for (;;) {
        gfx_rect(x0 - o, y0 - o, w, w, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

typedef struct {
    int8_t fhx, fhy, bhx, bhy;   /* hands, from the shoulder */
    int8_t ffx, ffy, bfx, bfy;   /* feet, from the hip */
    int8_t lean, drop;           /* shoulders forward, hips lower */
} Pose;

enum {
    PO_STAND, PO_WALK1, PO_WALK2, PO_JAB, PO_JAB2, PO_KICK, PO_WIND, PO_HURT, PO_JUMP, PO_FLYKICK,
    PO_SPIN1, PO_SPIN2, PO_GRAB, PO_THROW, PO_SWING, PO_CROUCH, PO_SIT, PO_ZOMBIE1, PO_ZOMBIE2, PO_GUARD,
    PO_DANCE1, PO_DANCE2, PO_RUN1, PO_RUN2, PO_ARMSUP, PO_COUNT
};
static const Pose POSES[PO_COUNT] = {
    [PO_STAND] = {3, 9, -2, 9, 2, 11, -2, 11, 0, 0},
    [PO_WALK1] = {4, 8, -3, 8, 4, 11, -3, 11, 0, 0},
    [PO_WALK2] = {-1, 9, 3, 8, -2, 11, 4, 11, 0, 0},
    [PO_JAB] = {11, 1, 3, 4, 3, 11, -3, 11, 1, 0},
    [PO_JAB2] = {3, 4, 11, 1, 3, 11, -3, 11, 1, 0},
    [PO_KICK] = {-2, 5, -5, 6, 11, 0, -2, 11, -2, 0},
    [PO_WIND] = {-6, 3, -3, 6, 3, 10, -4, 10, -1, 1},
    [PO_HURT] = {-5, -3, -7, 2, 2, 11, -4, 11, -3, 0},
    [PO_JUMP] = {4, -4, -4, -3, 4, 6, -3, 8, 0, 0},
    [PO_FLYKICK] = {-3, 3, -6, 2, 12, 2, -4, 7, -2, 0},
    [PO_SPIN1] = {11, -1, -11, -1, 3, 11, -3, 11, 0, 0},
    [PO_SPIN2] = {7, 4, -7, 4, 2, 11, -2, 11, 0, 0},
    [PO_GRAB] = {9, 4, 8, 6, 4, 11, -3, 11, 1, 0},
    [PO_THROW] = {9, -5, 6, -3, 4, 11, -4, 11, 1, 0},
    [PO_SWING] = {10, -3, 5, 2, 4, 11, -4, 11, 1, 0},
    [PO_CROUCH] = {7, 9, 5, 10, 4, 5, -3, 5, 4, 5},
    [PO_SIT] = {4, 6, -2, 7, 8, 1, 7, 2, 0, 3},
    [PO_ZOMBIE1] = {10, 3, 8, 4, 4, 11, -3, 11, 1, 0},
    [PO_ZOMBIE2] = {10, 4, 8, 3, -2, 11, 4, 11, 1, 0},
    [PO_GUARD] = {6, 0, 5, 2, 3, 11, -3, 11, 0, 1},
    [PO_DANCE1] = {5, -8, -3, 8, 4, 11, -3, 11, 0, 0},
    [PO_DANCE2] = {3, 8, -5, -8, -2, 11, 4, 11, 0, 0},
    [PO_RUN1] = {6, 3, -6, 4, 6, 10, -6, 9, 2, 0},
    [PO_RUN2] = {-4, 4, 5, 3, -5, 9, 6, 10, 2, 0},
    [PO_ARMSUP] = {5, -10, -4, -10, 3, 11, -3, 11, 0, 0},
};

/* what a figure looks like */
typedef struct {
    uint8_t skin, hair, top, top2, legs, shoe;
    uint8_t s;           /* size in eighths */
    uint8_t wide;        /* torso width */
    uint8_t head;        /* head style */
} Look;
enum { HD_HUMAN, HD_LONG, HD_SHAMBLER, HD_CROW, HD_RAM, HD_BOAR, HD_GREY, HD_HELM, HD_JESTER, HD_FISH, HD_ALDERMAN, HD_MUTANT, HD_PIGTAILS, HD_BUN, HD_CAP };

static Look look_of(const Actor *a) {
    Look l = {C_CREAM, C_BROWN, C_BLUE, C_SKY, C_SLATE, C_INK, 8, 7, HD_HUMAN};
    switch (a->kind) {
    case AK_FIGHTER: {
        const DkuFighter *f = &DKU_FIGHTERS[dku_g.pr[a->player].pick];
        l.skin = f->skin; l.hair = f->hair; l.top = f->top; l.top2 = f->top2; l.legs = f->legs; l.shoe = f->shoe;
        static const uint8_t HEADS[DK_NFIGHTERS] = {HD_CAP, HD_PIGTAILS, HD_HUMAN, HD_BUN};
        l.head = HEADS[dku_g.pr[a->player].pick];
        if (dku_g.pr[a->player].pick == DK_ROOK) { l.wide = 8; l.s = 9; }
        if (dku_g.pr[a->player].pick == DK_MACK) l.wide = 8;
        break;
    }
    case AK_SHAMBLER: l = (Look){C_LEAF, C_FOREST, C_GREY, C_SLATE, C_EARTH, C_INK, 8, 7, HD_SHAMBLER}; break;
    case AK_TORCH: l = (Look){C_SKY, C_NAVY, C_BLUE, C_NAVY, C_NAVY, C_INK, 9, 13, HD_SHAMBLER}; break;
    case AK_CROW: l = (Look){C_CREAM, C_INK, C_NIGHT, C_DUSK, C_INK, C_INK, 8, 7, HD_CROW}; break;
    case AK_RAMMER: l = (Look){C_VIOLET, C_CREAM, C_PURPLE, C_VIOLET, C_PURPLE, C_INK, 10, 11, HD_RAM}; break;
    case AK_TUSKER: l = (Look){C_PINK, C_MAROON, C_BROWN, C_TAN, C_EARTH, C_INK, 11, 13, HD_BOAR}; break;
    case AK_VISITOR: l = (Look){C_GREY, C_INK, C_LIGHT, C_GREY, C_GREY, C_SLATE, 10, 5, HD_GREY}; break;
    case AK_BULWARK: l = (Look){C_HIDE, C_INK, C_SLATE, C_GREY, C_DUSK, C_INK, 9, 11, HD_HELM}; break;
    case AK_GIGGLER: l = (Look){C_WHITE, C_RED, C_YELLOW, C_RED, C_RED, C_YELLOW, 8, 7, HD_JESTER}; break;
    case AK_SKIPPER: l = (Look){C_TEAL, C_JADE, C_JADE, C_TEAL, C_TEAL, C_TEAL, 7, 7, HD_FISH}; break;
    case AK_GRIST:
        if (a->mode == 2) l = (Look){C_LIME, C_PURPLE, C_MAGENTA, C_PURPLE, C_FOREST, C_INK, 12, 15, HD_MUTANT};
        else l = (Look){C_TAN, C_INK, C_INK, C_RED, C_INK, C_INK, 10, 10, HD_ALDERMAN};
        break;
    case AK_PASSER: {
        static const uint8_t TOPS[4] = {C_ORANGE, C_BLUE, C_JADE, C_PINK};
        l = (Look){C_TAN, C_BROWN, TOPS[(a->y >> 4) & 3], C_WHITE, C_SLATE, C_INK, 8, 7, HD_HUMAN};
        break;
    }
    default: break;
    }
    return l;
}

static void draw_head(const Look *l, int hx, int hy, int f, int pass, int sc) {
    /* hx, hy: the middle of the head; f facing; pass 0 = outline, 1 = colour */
    int w = 3 * sc / 8, h = sc / 2;
    int o = pass == 0 ? 1 : 0;
    int c = pass == 0 ? C_INK : l->skin;
    switch (l->head) {
    case HD_CROW:
        gfx_rect(hx - w - o, hy - h - o, 2 * w + 1 + 2 * o, 2 * h + 1 + 2 * o, pass ? C_INK : C_INK);
        if (pass) {
            gfx_rect(hx - w + 1, hy - h + 1, 2 * w - 1, 2 * h - 1, C_NIGHT);
            gfx_pset(hx + f * 1, hy - 1, C_RED);
        }
        /* the beak */
        thick_line(hx + f * w, hy, hx + f * (w + 5), hy + 2, pass ? 2 : 4, pass ? C_AMBER : C_INK);
        return;
    case HD_GREY:
        gfx_circ(hx, hy - 1, w + 1 + o, c);
        if (pass) {
            gfx_rect(hx + f * 1 - (f < 0 ? 2 : 0), hy - 2, 3, 3, C_INK);
            gfx_pset(hx + f * 2, hy - 2, C_WHITE);
        }
        return;
    case HD_FISH:
        gfx_rect(hx - w - o, hy - h - o, 2 * w + 3 + 2 * o, 2 * h + 1 + 2 * o, c);
        if (pass) {
            gfx_pset(hx + f * 2, hy - 1, C_YELLOW);
            gfx_pset(hx + f * 2, hy, C_INK);
            gfx_hline(hx + f * 1, hx + f * (w + 1), hy + 2, C_INK);
            gfx_pset(hx - f * w, hy - h - 1, C_JADE);
            gfx_pset(hx - f * (w - 1), hy - h - 2, C_JADE);
        }
        return;
    default: break;
    }
    gfx_rect(hx - w - o, hy - h - o, 2 * w + 1 + 2 * o, 2 * h + 1 + 2 * o, c);
    if (pass == 0) {
        if (l->head == HD_RAM) { gfx_circ(hx - f * 2, hy - h, 4, C_INK); }
        if (l->head == HD_JESTER) { thick_line(hx - 4, hy - h, hx - 7, hy - h - 6, 4, C_INK); thick_line(hx + 4, hy - h, hx + 7, hy - h - 6, 4, C_INK); }
        if (l->head == HD_ALDERMAN) gfx_rect(hx - w - 1, hy - h - 7, 2 * w + 3, 8, C_INK);
        if (l->head == HD_BOAR) gfx_rect(hx + f * w - 1, hy - 1, 5, 5, C_INK);
        return;
    }
    /* hair and faces */
    int eye = hx + f * (w - 2);
    switch (l->head) {
    case HD_HUMAN: case HD_PIGTAILS: case HD_BUN: case HD_CAP:
        gfx_rect(hx - w, hy - h, 2 * w + 1, 2, l->hair);
        gfx_rect(hx - f * w - (f > 0 ? 0 : 1), hy - h, 2, h + 1, l->hair);
        if (l->head == HD_PIGTAILS) { gfx_rect(hx - f * (w + 2), hy - 1, 2, 4, l->hair); }
        if (l->head == HD_BUN) { gfx_circ(hx - f * (w - 1), hy - h - 1, 2, l->hair); }
        if (l->head == HD_CAP) { gfx_rect(hx - w, hy - h - 1, 2 * w + 1, 3, l->top2); gfx_rect(hx + f * w - (f > 0 ? 0 : 3), hy - h + 1, 4, 1, l->top2); }
        gfx_pset(eye, hy - 1, C_INK);
        break;
    case HD_SHAMBLER:
        gfx_rect(hx - w, hy - h, 2 * w + 1, 1, l->hair);
        gfx_pset(eye, hy - 1, C_RED);
        gfx_hline(hx + f, hx + f * (w - 1), hy + 2, C_INK);
        gfx_pset(hx - f * 1, hy + h - 1, C_RED);
        break;
    case HD_RAM:
        gfx_circb(hx - f * 2, hy - h, 3, l->hair);
        gfx_pset(eye, hy - 1, C_YELLOW);
        gfx_rect(hx + f * w, hy + 1, 1, 2, C_INK);
        break;
    case HD_BOAR:
        gfx_rect(hx + f * w, hy, 3 * f > 0 ? 3 : 3, 3, C_PINK);
        gfx_pset(hx + f * (w + 1), hy + 1, C_INK);
        gfx_pset(hx + f * (w + 1), hy + 3, C_WHITE);
        gfx_pset(hx + f * (w + 2), hy + 3, C_WHITE);
        gfx_pset(eye - f, hy - 2, C_INK);
        gfx_rect(hx - f * w, hy - h - 1, 2, 2, l->hair);
        break;
    case HD_HELM:
        gfx_rect(hx - w, hy - h, 2 * w + 1, h + 1, C_GREY);
        gfx_hline(hx - w, hx + w, hy - h, C_LIGHT);
        gfx_rect(hx - w, hy, 2 * w + 1, 2, C_SLATE);
        gfx_pset(eye, hy - 1, C_RED);
        gfx_pset(eye - f * 2, hy - 1, C_RED);
        break;
    case HD_JESTER:
        thick_line(hx - 4, hy - h, hx - 7, hy - h - 5, 2, C_RED);
        thick_line(hx + 4, hy - h, hx + 7, hy - h - 5, 2, C_PURPLE);
        gfx_pset(hx - 7, hy - h - 6, C_YELLOW);
        gfx_pset(hx + 7, hy - h - 6, C_YELLOW);
        gfx_pset(eye, hy - 1, C_INK);
        gfx_pset(hx + f * w, hy + 1, C_RED);
        gfx_hline(hx, hx + f * (w - 1), hy + 3, C_RED);
        break;
    case HD_ALDERMAN:
        gfx_rect(hx - w, hy - h - 6, 2 * w + 1, 7, C_INK);
        gfx_hline(hx - w - 1, hx + w + 1, hy - h, C_INK);
        gfx_hline(hx - w + 1, hx + w - 1, hy - h - 2, C_RED);
        gfx_pset(eye, hy - 1, C_INK);
        gfx_hline(hx + f, hx + f * (w - 1), hy + 2, C_GREY);
        break;
    case HD_MUTANT:
        gfx_pset(eye, hy - 2, C_YELLOW);
        gfx_pset(eye - f * 3, hy - 2, C_YELLOW);
        gfx_hline(hx - w + 1, hx + w - 1, hy + 3, C_INK);
        for (int k = -w + 2; k < w; k += 2) gfx_pset(hx + k, hy + 3, C_WHITE);
        gfx_rect(hx - w, hy - h, 2 * w + 1, 2, C_PURPLE);
        break;
    case HD_LONG: default: break;
    }
}

static void draw_figure(const Actor *a, const Look *l, int sx, int sy, int pose, bool lying, int flash) {
    const Pose *p = &POSES[pose];
    int f = a->face;
    int sc = l->s;
    if (lying) {
        /* flat on the floor */
        for (int pass = 0; pass < 2; pass++) {
            int t = pass ? 0 : 2;
            int y = sy - 3;
            int hx = sx - f * (9 * sc / 8), fx = sx + f * (10 * sc / 8);
            thick_line(sx - f * 3, y, fx, y + 1, 3 + t, pass ? l->legs : C_INK);
            thick_line(sx - f * 3, y - 1, sx + f * 1, y - 2, 3 + t, pass ? l->legs : C_INK);
            gfx_rect(imin(hx, sx - f * 3) - (pass ? 0 : 1), y - 3 - (pass ? 0 : 1), iabs(sx - f * 3 - hx) + 1 + (pass ? 0 : 2),
                     5 + (pass ? 0 : 2), pass ? l->top : C_INK);
            Look hl = *l;
            if (pass) draw_head(&hl, hx - f * 3, y - 1, f, 1, sc);
            else draw_head(&hl, hx - f * 3, y - 1, f, 0, sc);
        }
        (void)flash;
        return;
    }
    int leg = 11 * sc / 8, body = 9 * sc / 8;
    int hipx = sx, hipy = sy - leg + p->drop;
    int shx = sx + f * p->lean, shy = hipy - body;
    int hw = l->wide / 2;
    int limb = sc >= 10 ? 3 : 2;
    for (int pass = 0; pass < 2; pass++) {
        int t = pass ? 0 : 2;
        int ink = flash ? flash : C_INK;
        /* legs */
        int ffx = hipx + f * p->ffx * sc / 8, ffy = hipy + p->ffy * sc / 8;
        int bfx = hipx + f * p->bfx * sc / 8, bfy = hipy + p->bfy * sc / 8;
        thick_line(hipx - f * 1, hipy, bfx, bfy, limb + t, pass ? PAL_DARKER[l->legs] : ink);
        thick_line(hipx + f * 1, hipy, ffx, ffy, limb + t, pass ? l->legs : ink);
        if (pass) {
            gfx_rect(bfx - 1 + (f > 0 ? 0 : -1), bfy - 1, 3, 2, l->shoe);
            gfx_rect(ffx - 1 + (f > 0 ? 0 : -1), ffy - 1, 3, 2, l->shoe);
        }
        /* the back arm */
        int bhx = shx + f * p->bhx * sc / 8, bhy = shy + 2 + p->bhy * sc / 8;
        thick_line(shx - f * 2, shy + 2, bhx, bhy, limb + t, pass ? PAL_DARKER[l->skin] : ink);
        /* the body */
        int top = shy, bot = hipy + 1;
        gfx_rect(imin(shx, hipx) - hw - (pass ? 0 : 1), top - (pass ? 0 : 1), hw * 2 + iabs(shx - hipx) + 1 + (pass ? 0 : 2),
                 bot - top + 1 + (pass ? 0 : 2), pass ? l->top : ink);
        if (pass) {
            gfx_hline(imin(shx, hipx) - hw, imax(shx, hipx) + hw, bot - 1, l->top2);
            if (a->kind == AK_FIGHTER || a->kind == AK_GRIST) gfx_vline(shx + f * (hw - 2), top + 1, bot - 2, l->top2);
        }
        /* the head */
        int hh = sc / 2 + 1;
        draw_head(l, shx + f * 1, top - hh, f, pass, sc);
        /* the front arm */
        int fhx = shx + f * p->fhx * sc / 8, fhy = shy + 2 + p->fhy * sc / 8;
        thick_line(shx + f * 1, shy + 2, fhx, fhy, limb + t, pass ? l->skin : ink);
        if (pass) gfx_rect(fhx - 1, fhy - 1, 3, 3, a->kind == AK_FIGHTER && dku_g.pr[a->player].pick == DK_MACK ? C_RED : l->skin);
    }
}

/* ---- the non-humans --------------------------------------------------------- */

static void draw_beast(const Actor *a, int sx, int sy, bool dog) {
    int f = a->face;
    int body = dog ? C_TAN : C_EARTH, dark = dog ? C_BROWN : C_NIGHT;
    int len = dog ? 9 : 12, hgt = dog ? 7 : 9;
    int st = (a->step / 5) & 1;
    bool down = a->state == AS_DOWN || a->state == AS_DEAD;
    bool leap = a->state == AS_RUSH || a->state == AS_WINDUP;
    int by = sy - hgt;
    if (down) by = sy - 4;
    for (int pass = 0; pass < 2; pass++) {
        int o = pass ? 0 : 1;
        int c = pass ? body : C_INK;
        if (!down) {
            for (int k = -1; k <= 1; k += 2) {
                int lx = sx + k * (len - 3);
                int sw = (st ^ (k > 0)) ? 2 : -2;
                if (leap) sw = k * f * 3;
                thick_line(lx, by + 3, lx + sw, sy - 1, 2 + o * 2, pass ? dark : C_INK);
            }
        }
        gfx_rect(sx - len - o, by - o + (a->state == AS_WINDUP ? 2 : 0), 2 * len + 1 + 2 * o, 6 + 2 * o, c);
        int hx = sx + f * (len + 2), hy = by - 3 + (a->state == AS_WINDUP ? 3 : 0);
        gfx_rect(hx - 3 - o, hy - o, 7 + 2 * o, 6 + 2 * o, c);
        /* ears and tail */
        gfx_rect(hx - f * 2 - o, hy - 3 - o, 2 + 2 * o, 3 + o, pass ? dark : C_INK);
        thick_line(sx - f * len, by + 1, sx - f * (len + 5), by - 3 + (dog ? (a->step & 4 ? 0 : 2) : 0), 2 + 2 * o, pass ? dark : C_INK);
        if (pass) {
            gfx_pset(hx + f * 1, hy + 1, dog ? C_INK : C_YELLOW);
            gfx_pset(hx + f * 3, hy + 3, C_INK);
            if (!dog) gfx_hline(hx + f * 1, hx + f * 3, hy + 4, C_WHITE);
            if (dog) gfx_rect(hx - f * 3, hy + 5, 2, 2, C_RED); /* a collar */
        }
    }
}

static void draw_sludger(const Actor *a, int sx, int sy) {
    int f = a->face;
    int pulse = (a->step / 8) & 1;
    for (int pass = 0; pass < 2; pass++) {
        int o = pass ? 0 : 1;
        gfx_rect(sx - 14 - o, sy - 9 - o, 26 + 2 * o, 9 + 2 * o, pass ? C_LIME : C_INK);
        gfx_circ(sx + f * 8, sy - 10, 7 + o, pass ? C_LIME : C_INK);
        thick_line(sx + f * 10, sy - 14, sx + f * 13, sy - 22 + pulse, 2 + 2 * o, pass ? C_LEAF : C_INK);
        thick_line(sx + f * 6, sy - 15, sx + f * 6, sy - 23 + pulse, 2 + 2 * o, pass ? C_LEAF : C_INK);
    }
    gfx_pset(sx + f * 13, sy - 23 + pulse, C_INK);
    gfx_pset(sx + f * 6, sy - 24 + pulse, C_INK);
    gfx_hline(sx - 12, sx + 10, sy - 2, C_LEAF);
    for (int k = -10; k < 8; k += 5) gfx_pset(sx + k, sy - 6, C_YELLOW);
    if (a->state == AS_WINDUP) gfx_circ(sx + f * 14, sy - 10, 2 + (a->st / 6) % 2, C_JADE);
}

static void draw_feeler(const Actor *a, int sx, int sy) {
    int sway = a->state == AS_WINDUP ? -a->face * 4 : ((a->st / 10) & 1) * 2 - 1;
    if (a->state == AS_ATTACK) sway = a->face * 10;
    for (int pass = 0; pass < 2; pass++) {
        int w = pass ? 5 : 7;
        int c = pass ? C_PURPLE : C_INK;
        thick_line(sx, sy, sx + sway / 2, sy - 14, w, c);
        thick_line(sx + sway / 2, sy - 14, sx + sway, sy - 26, w - 1, c);
    }
    for (int k = 0; k < 4; k++) gfx_pset(sx + sway * k / 4 + 1, sy - 4 - k * 6, C_PINK);
    gfx_hline(sx - 6, sx + 6, sy, C_NIGHT);
}

static void draw_undertow(const Actor *a, int sx, int sy) {
    int t = dku_g.frame_t;
    int bob = (t / 20) & 1;
    int y = sy - 34 + bob;
    for (int k = 0; k < 3; k++) {
        int tx = sx - 22 + k * 8, sway = ((t / 12 + k) & 3) - 1;
        thick_line(tx, y + 30, tx - 6 + sway * 3, y + 10, 6, C_INK);
        thick_line(tx, y + 30, tx - 6 + sway * 3, y + 10, 4, C_VIOLET);
    }
    gfx_circ(sx, y, 21, C_INK);
    gfx_circ(sx, y, 20, a->hurt_t ? C_WHITE : C_PURPLE);
    gfx_circ(sx - 4, y - 6, 12, C_VIOLET);
    for (int e = -1; e <= 1; e += 2) {
        gfx_circ(sx - 8 + e * 6, y + 2, 4, C_YELLOW);
        gfx_rect(sx - 8 + e * 6 - 1, y + 1, 2, 3, C_INK);
    }
    for (int k = -14; k <= 14; k += 7) gfx_pset(sx + k / 2, y + 14, C_PINK);
    gfx_dither(sx - 26, sy - 4, 52, 6, C_SKY, 6);
}

static void draw_saucer(const Actor *a, int sx, int sy) {
    int z = dku_px(a->z);
    gfx_dither(sx - 9, sy - 1, 18, 3, C_INK, 8);
    ui_saucer(sx - 11, sy - z - 8, dku_g.frame_t, 1);
}

/* ---- picking a pose ------------------------------------------------------------ */

static int pose_for(const Actor *a, bool *lying) {
    *lying = false;
    bool walking = (int)(dku_g.frame_t - a->anim) < 3;
    int wf = (a->step / 7) & 1;
    bool zombie = a->kind == AK_SHAMBLER;
    switch (a->state) {
    case AS_FREE: case AS_ENTER:
        if (a->kind == AK_FIGHTER && a->running) return wf ? PO_RUN1 : PO_RUN2;
        if (a->kind == AK_FIGHTER && a->carry >= 0) return walking ? (wf ? PO_WALK1 : PO_WALK2) : PO_GRAB;
        if (a->kind == AK_FIGHTER && a->charge_t >= 12) return PO_WIND;
        if (walking || a->state == AS_ENTER) return zombie ? (wf ? PO_ZOMBIE1 : PO_ZOMBIE2) : (wf ? PO_WALK1 : PO_WALK2);
        return zombie ? PO_ZOMBIE1 : PO_STAND;
    case AS_ATTACK: {
        if (a->atk == AT_NONE) return PO_THROW;
        const DkuAtk *d = &DKU_ATK[a->atk];
        if (a->atk_t <= d->startup) return a->atk == AT_KICK ? PO_WIND : PO_WIND;
        switch (a->atk) {
        case AT_JAB: case AT_PUMMEL: return (a->combo & 1) ? PO_JAB : PO_JAB2;
        case AT_KICK: case AT_DASH: return PO_KICK;
        case AT_CHARGED: return PO_JAB;
        case AT_SWING: case AT_SAW: return PO_SWING;
        case AT_E_SLAM: return PO_CROUCH;
        default: return a->kind == AK_TUSKER || a->kind == AK_RAMMER ? PO_JAB : PO_JAB;
        }
    }
    case AS_WINDUP: return a->kind == AK_TORCH || a->kind == AK_CROW || a->kind == AK_GRIST ? PO_THROW : PO_WIND;
    case AS_HURT: case AS_GRABBED: return PO_HURT;
    case AS_AIR: return a->atk == AT_FLYKICK && a->atk_t > 2 ? PO_FLYKICK : PO_JUMP;
    case AS_SUPLEX: case AS_SLAM: return PO_ARMSUP;
    case AS_SPIN: return ((a->atk_t / 3) & 1) ? PO_SPIN1 : PO_SPIN2;
    case AS_GRAB: return PO_GRAB;
    case AS_DODGE: return PO_CROUCH;
    case AS_GUARD: return PO_GUARD;
    case AS_RUSH:
        if (a->kind == AK_RAMMER) return PO_RUN1;
        return PO_FLYKICK;
    case AS_DORMANT:
        if (a->mode == DM_FEED) return PO_CROUCH;
        if (a->mode == DM_SIT || a->mode == DM_SLEEP) return PO_SIT;
        if (a->mode == DM_DANCE) return ((dku_g.frame_t / 16 + (int)(a->x >> 6)) & 1) ? PO_DANCE1 : PO_DANCE2;
        return PO_STAND;
    case AS_DOWN:
        if (a->z > 0) return PO_HURT;
        if (a->st > a->down_t) return PO_CROUCH;
        *lying = true;
        return PO_STAND;
    case AS_DEAD: case AS_THROWN:
        if (a->state == AS_THROWN) return PO_HURT;
        *lying = a->z == 0;
        return PO_HURT;
    case AS_CHANGE: return (dku_g.frame_t / 4) & 1 ? PO_ARMSUP : PO_CROUCH;
    case AS_RISE: return a->from == FROM_GROUND ? PO_ARMSUP : PO_JUMP;
    default: return PO_STAND;
    }
}

/* sx, sy: the feet on the screen (the floor point under the actor) */
void dku_draw_actor(const Actor *a, int sx, int sy) {
    int z = dku_px(a->z);
    if (a->state == AS_FALL) {
        if (a->st > 24) return;
        sy += a->st;
    }
    if (a->state == AS_DEAD && a->st > 30 && (a->st & 2)) return;
    if (a->state == AS_BLINK && a->st >= 4 && a->st < 14) {
        if (a->st & 1) gfx_vline(sx, sy - 34, sy, C_CYAN);
        return;
    }
    /* the shadow */
    if (a->kind != AK_SAUCER && a->kind != AK_UNDERTOW && a->state != AS_FALL)
        gfx_dither(sx - 7, sy - 1, 14, 3, C_INK, 8);
    switch (a->kind) {
    case AK_HOWLER: case AK_DOG:
        if (a->state == AS_LEASHED) { thick_line(sx - 10, sy - 6, sx - 2, sy - 4, 1, C_BROWN); }
        draw_beast(a, sx, sy - z, a->kind == AK_DOG);
        return;
    case AK_SLUDGER: draw_sludger(a, sx, sy - z); return;
    case AK_FEELER: draw_feeler(a, sx, sy); return;
    case AK_UNDERTOW: draw_undertow(a, sx, sy); return;
    case AK_SAUCER: draw_saucer(a, sx, sy); return;
    default: break;
    }
    if (a->state == AS_DORMANT && a->mode == DM_FEED) {
        /* what it's eating: a passer-by who didn't make it */
        int bx = sx + a->face * 10;
        gfx_rect(bx - 9, sy - 5, 18, 5, C_INK);
        gfx_rect(bx - 8, sy - 4, 12, 3, C_SLATE);
        gfx_rect(bx + 4, sy - 5, 4, 4, C_TAN);
        gfx_pset(bx - 2, sy - 3, C_RED);
        gfx_pset(bx + 1, sy - 2, C_MAROON);
    }
    Look l = look_of(a);
    bool lying;
    int pose = pose_for(a, &lying);
    int flash = 0;
    if (a->hurt_t > 0 && (a->hurt_t & 2)) flash = C_WHITE;
    if (a->kind == AK_FIGHTER && a->charged && (dku_g.frame_t & 4)) flash = C_YELLOW;
    if (a->kind == AK_FIGHTER && a->state == AS_GRAB && dku_g.pr[a->player].pick == DK_DOLLY && a->grab_charge >= 20 && (dku_g.frame_t & 4)) flash = C_ORANGE;
    if (a->state == AS_CHANGE && (dku_g.frame_t & 2)) flash = C_LIME;
    if (a->state == AS_RISE && a->from == FROM_GROUND) {
        /* clawing up out of the ground */
        int shown = 34 * a->st / 40;
        gfx_clip(sx - 20, sy - shown, 40, shown);
        draw_figure(a, &l, sx, sy - z + 34 - shown, pose, false, 0);
        gfx_noclip();
        gfx_dither(sx - 8, sy - 2, 16, 3, C_EARTH, 12);
        return;
    }
    if (a->kind == AK_GRIST && a->mode == 2 && !lying) {
        /* the changed alderman: extra arms */
        thick_line(sx - a->face * 4, sy - z - 26, sx - a->face * 14, sy - z - 14 + ((dku_g.frame_t / 8) & 1) * 3, 5, C_INK);
        thick_line(sx - a->face * 4, sy - z - 26, sx - a->face * 14, sy - z - 14 + ((dku_g.frame_t / 8) & 1) * 3, 3, C_LIME);
    }
    draw_figure(a, &l, sx, sy - z, pose, lying, flash);
    if (a->kind == AK_TORCH && !lying) {
        /* a lit bottle in hand */
        int hx = sx + a->face * 6, hy = sy - z - 22;
        gfx_rect(hx - 1, hy, 3, 5, C_JADE);
        gfx_pset(hx, hy - 1 - ((dku_g.frame_t / 4) & 1), C_ORANGE);
    }
    if (a->kind == AK_CROW && !lying && a->state != AS_WINDUP) {
        int hx = sx + a->face * 8, hy = sy - z - 15;
        gfx_rect(hx, hy, 3, 4, C_LIGHT);
    }
    if (a->kind == AK_BULWARK && a->state == AS_GUARD) {
        gfx_rect(sx + a->face * 7 - 2, sy - z - 28, 5, 18, C_INK);
        gfx_rect(sx + a->face * 7 - 1, sy - z - 27, 3, 16, C_GREY);
    }
    if (a->kind == AK_TUSKER && a->state == AS_GUARD) {
        gfx_rect(sx + a->face * 8 - 2, sy - z - 30, 4, 12, C_PINK);
    }
    if (a->state == AS_DORMANT && a->mode == DM_SLEEP && ((dku_g.frame_t / 30) & 1)) tiny_draw("Z", sx + 4, sy - 40, C_WHITE);
    if (a->kind == AK_VISITOR && a->state == AS_WINDUP) gfx_circ(sx + a->face * 10, sy - z - 18, 1 + (a->st / 4) % 3, C_CYAN);
    /* what a fighter carries */
    if (a->kind == AK_FIGHTER && a->carry >= 0 && !lying) {
        int k = dku_g.it[a->carry].kind;
        int hx = sx + a->face * 6 - 5, hy = sy - z - 24;
        if (a->state == AS_ATTACK) hx += a->face * 6;
        spr_draw(&dku_spr[SPR_ITEM0 + k], hx, hy, a->face < 0 ? SPR_FLIPX : 0);
    }
}

/* the select screen's big fighter: a standing figure at double size */
/* x: the middle, y: the feet */
void dku_draw_fighter_big(int f, int x, int y, int t) {
    static uint8_t buf[64 * 64];
    Surface s = {64, 64, buf};
    memset(buf, TRANSPARENT, sizeof buf);
    Surface *old = gfx_get_target();
    gfx_set_target(&s);
    Actor a;
    memset(&a, 0, sizeof a);
    a.kind = AK_FIGHTER;
    a.player = 0;
    a.face = x > SCREEN_W / 2 + 40 ? -1 : 1;
    a.state = AS_FREE;
    int keep = dku_g.pr[0].pick;
    dku_g.pr[0].pick = f;
    Look l = look_of(&a);
    draw_figure(&a, &l, 32, 60, (t / 20) & 1 ? PO_STAND : PO_GUARD, false, 0);
    dku_g.pr[0].pick = keep;
    gfx_set_target(old);
    Sprite sp = {64, 64, buf};
    spr_draw_scaled(&sp, x - 64, y - 128, 2, 0);
}
