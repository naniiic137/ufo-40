/* CUTLASS CUP - pixel art (palette-letter strings, see gfx.h).
 * Fighters are composed at load time from shared body poses (clothes
 * recoloured per fighter) and a head per fighter; weapons are drawn live. */
#include "cutlass.h"

Sprite cc_spr[CS_SPRITE_COUNT];
Sprite cc_fighter_spr[CC_FIGHTERS][POSE_COUNT];

/* bodies, 12 x 12, facing right: r/m shirt, y sash, N trousers, h hands */
static const char BODY_IDLE[] =
    "...kkkkk...."
    "..krrrrrk..."
    ".krrrrrrrk.."
    ".kmrrrrrrhk."
    ".kmrrrrrmkk."
    "..kyyyyyyk.."
    "..kNNNNNNk.."
    "..kNNkkNNk.."
    "..kNk..kNk.."
    "..kNk..kNk.."
    "..kbk..kbk.."
    ".kbbk..kbbk.";
static const char BODY_RUN0[] =
    "...kkkkk...."
    "..krrrrrk..."
    ".krrrrrrrk.."
    ".kmrrrrrrhk."
    ".kmrrrrrmkk."
    "..kyyyyyyk.."
    "..kNNNNNNk.."
    ".kNNk.kNNk.."
    ".kNk...kNk.."
    "kNk.....kNk."
    "kbk.....kbk."
    "kbbk....kbbk";
static const char BODY_RUN1[] =
    "...kkkkk...."
    "..krrrrrk..."
    ".krrrrrrrk.."
    ".kmrrrrrrhk."
    ".kmrrrrrmkk."
    "..kyyyyyyk.."
    "..kNNNNNNk.."
    "...kNNNNk..."
    "...kNkNk...."
    "...kNkNk...."
    "...kbkbk...."
    "..kbbkbbk...";
static const char BODY_WIND[] =
    "kh.kkkkk...."
    "khkrrrrrk..."
    ".krrrrrrrk.."
    ".kmrrrrrrk.."
    ".kmrrrrrmk.."
    "..kyyyyyyk.."
    "..kNNNNNNk.."
    "..kNNkkNNk.."
    ".kNk...kNk.."
    ".kNk...kNk.."
    ".kbk...kbk.."
    "kbbk...kbbk.";
static const char BODY_STRIKE[] =
    "...kkkkk...."
    "..krrrrrk..."
    ".krrrrrrrkkk"
    ".kmrrrrrrhhk"
    ".kmrrrrrmkk."
    "..kyyyyyyk.."
    "..kNNNNNNk.."
    ".kNNk.kNNk.."
    "kNk....kNk.."
    "kNk....kNk.."
    "kbk....kbk.."
    "kbbk...kbbk.";
static const char BODY_ROLL[] =
    "............"
    "............"
    "...kkkkkk..."
    "..krrrrrrk.."
    ".krrrmmrrrk."
    ".krmyyyymrk."
    ".kryNNNNyrk."
    ".krNNNNNNrk."
    "..kNbbbbNk.."
    "...kkkkkk..."
    "............"
    "............";
static const char BODY_CATCH[] =
    "...kkkkk...."
    "..krrrrrk..."
    ".krrrrrrrkkk"
    ".kmrrrrrhhhk"
    ".kmrrrrrhhhk"
    "..kyyyyyykk."
    "..kNNNNNNk.."
    ".kNNk.kNNk.."
    "kNk....kNk.."
    "kNk....kNk.."
    "kbk....kbk.."
    "kbbk...kbbk.";
static const char BODY_WIN[] =
    "kh.kkkkk.hk."
    "khkrrrrrkhk."
    ".krrrrrrrk.."
    ".kmrrrrrrk.."
    ".kmrrrrrmk.."
    "..kyyyyyyk.."
    "..kNNNNNNk.."
    "..kNNkkNNk.."
    "..kNk..kNk.."
    "..kNk..kNk.."
    "..kbk..kbk.."
    ".kbbk..kbbk.";

/* heads, 10 x 10, facing right */
static const char HEAD_HAMDI[] =
    "..kkkkkk.."
    ".krrrrrrk."
    "krrryrrrrk"
    "rkkkkkkkkk"
    "rkhhhhhhhk"
    ".khhhhhkhk"
    ".khhhhhhhk"
    ".khhhhkkhk"
    "..keeeeek."
    "...kkkkk..";
static const char HEAD_LEILA[] =
    "..kkkkkk.."
    ".kjjjjjjk."
    "kjjjjjjjjk"
    "kjjhhhhhjk"
    "kjhhhhhhhk"
    "kjhhhhhkhk"
    "kjkhhhhhhk"
    "jk.khhkkhk"
    "j..khhhhk."
    "....kkkk..";
static const char HEAD_NOUR[] =
    "...kkkk..."
    "..kaeeak.."
    "kkaaaaaakk"
    "kaaaaaaaak"
    ".kkkkkkkk."
    ".khhhhhkhk"
    ".khhhhhhhk"
    ".khhhhkkhk"
    "..khhhhhk."
    "...kkkkk..";
static const char HEAD_KARIM[] =
    "...kkkk..."
    "..krrrrk.."
    ".kkrrrrk.."
    "kk.krrrrk."
    "k.kkkkkkk."
    "..khhhhkhk"
    "..khhhhhhk"
    "..kbbbbbhk"
    "...khhhhk."
    "....kkkk..";
static const char HEAD_ZINA[] =
    "..kkkkkk.."
    ".kddddddk."
    "kddddddddk"
    "kddddddddk"
    "kdnnnnnnnk"
    "kdnnnnwnwk"
    "kdnnnnnnnk"
    ".kddddddk."
    "..kddddk.."
    "...kkkk...";
static const char HEAD_OMAR[] =
    "..kkkkkk.."
    ".klwwwwlk."
    "klwwlwwwlk"
    "klllwllllk"
    "kkkkkkkkkk"
    "khhhhhhkhk"
    "khhhhhhhhk"
    "kbbbbbbbhk"
    ".khhhhhhk."
    "..kkkkkk..";

static const char BALL[] =
    ".kkkk."
    "kcwcck"
    "kwccek"
    "kcceek"
    "kceetk"
    ".kkkk.";
static const char COIN1[] =
    ".yy."
    "yaay"
    "yaoy"
    ".yy.";
static const char COIN2[] =
    ".ya."
    ".ya."
    ".ao."
    ".ao.";
static const char URCHIN[] =
    "p..p..p"
    ".pVVVp."
    ".VPVVV."
    "pVVVVVp"
    ".VVVVV."
    ".pVVVp."
    "p..p..p";
static const char DART[] =
    "....k."
    "PlllwK"
    "....k.";
static const char BOMB[] =
    "....y.."
    "...kw.."
    "..kbk.."
    ".ktttk."
    "ktetttk"
    "ktttttk"
    ".ktttk."
    "..kkk..";
static const char DECOY[] =
    "..kkkk.."
    ".kttttk."
    "kbbbbbbk"
    "kttettk."
    "kttttttk"
    "kbbbbbbk"
    "kttttttk"
    "kteetttk"
    ".kbbbbk."
    "..kkkk..";
static const char JUDGE1[] =
    "....kkkkkk......"
    "...kNNNNNNk....."
    "..kNNNNyNNNk...."
    "..kkkkkkkkkk...."
    "...khhhkhhk....."
    "...khhhhhhk....."
    "..kwwwhhwwwk...."
    "..kwwwwwwwwk...."
    "..kkwwwwwwkk...."
    ".kNNkwwwwkNNk..."
    "kNNNNkkkkNNNNk.."
    "kNyNNNNNNNNyNk.."
    "kNNNNNNNNNNNNk.."
    "kNNNNNNNNNNNNk..";
static const char JUDGE2[] =
    "....kkkkkk...kk."
    "...kNNNNNNk.khk."
    "..kNNNNyNNNkkhk."
    "..kkkkkkkkkkkNk."
    "...khhhkhhk.kNk."
    "...khhhhhhk.kNk."
    "..kwwwhhwwwkkNk."
    "..kwwwwwwwwkNNk."
    "..kkwwwwwwkkNk.."
    ".kNNkwwwwkNNk..."
    "kNNNNkkkkNNNNk.."
    "kNyNNNNNNNNyNk.."
    "kNNNNNNNNNNNNk.."
    "kNNNNNNNNNNNNk..";
static const char GULL1[] =
    "..kk....."
    ".kwwk...."
    "kowwwkkk."
    ".kwwwwgk."
    "..kkkkk..";
static const char GULL2[] =
    "..kk....."
    ".kwwk...."
    "kowwwkkk."
    ".kwwwwgk."
    "..k.k.k..";
static const char CUP[] =
    "..kkkkkkkkkk.."
    "kkkyywyyyyakkk"
    "k.kyywyyyyak.k"
    "k.kyyyyyyyak.k"
    ".kkyyyyyyyakk."
    "...kyyyyyak..."
    "....kyyyak...."
    ".....kyak....."
    ".....kyak....."
    "....kaaaak...."
    "...kkkkkkkk..."
    "...kooooook..."
    "..kaaaaaaaak.."
    "..kkkkkkkkkk..";

typedef struct Art {
    int id, w, h;
    const char *px;
} Art;

static const Art ART[] = {
    {CS_BODY_IDLE0, 12, 12, BODY_IDLE}, {CS_BODY_IDLE1, 12, 12, BODY_IDLE}, {CS_BODY_RUN0, 12, 12, BODY_RUN0},
    {CS_BODY_RUN1, 12, 12, BODY_RUN1}, {CS_BODY_WIND, 12, 12, BODY_WIND}, {CS_BODY_STRIKE, 12, 12, BODY_STRIKE},
    {CS_BODY_ROLL, 12, 12, BODY_ROLL}, {CS_BODY_STUN, 12, 12, BODY_IDLE}, {CS_BODY_CATCH, 12, 12, BODY_CATCH},
    {CS_BODY_WIN, 12, 12, BODY_WIN},
    {CS_HEAD0, 10, 10, HEAD_HAMDI}, {CS_HEAD1, 10, 10, HEAD_LEILA}, {CS_HEAD2, 10, 10, HEAD_NOUR},
    {CS_HEAD3, 10, 10, HEAD_KARIM}, {CS_HEAD4, 10, 10, HEAD_ZINA}, {CS_HEAD5, 10, 10, HEAD_OMAR},
    {CS_BALL, 6, 6, BALL}, {CS_COIN1, 4, 4, COIN1}, {CS_COIN2, 4, 4, COIN2}, {CS_URCHIN, 7, 7, URCHIN},
    {CS_DART, 6, 3, DART}, {CS_BOMB, 7, 8, BOMB}, {CS_DECOY, 8, 10, DECOY}, {CS_JUDGE1, 16, 14, JUDGE1},
    {CS_JUDGE2, 16, 14, JUDGE2}, {CS_GULL1, 9, 5, GULL1}, {CS_GULL2, 9, 5, GULL2}, {CS_CUP, 14, 14, CUP},
};

/* where the head sits on each pose (canvas 16 x 22, body at 2,10) */
static const int8_t HEAD_OFS[POSE_COUNT][2] = {
    {3, 1}, {3, 2}, {3, 1}, {3, 1}, {2, 1}, {4, 1}, {3, 5}, {2, 3}, {4, 1}, {3, 1},
};
static const int BODY_OF_POSE[POSE_COUNT] = {
    CS_BODY_IDLE0, CS_BODY_IDLE1, CS_BODY_RUN0, CS_BODY_RUN1, CS_BODY_WIND, CS_BODY_STRIKE,
    CS_BODY_ROLL, CS_BODY_STUN, CS_BODY_CATCH, CS_BODY_WIN,
};

static void compose(int f, int pose) {
    const FighterDef *d = &CC_FIGHTER[f];
    enum { W = 16, H = 22 };
    uint8_t *px = (uint8_t *)malloc(W * H);
    if (!px) return;
    memset(px, TRANSPARENT, W * H);
    const Sprite *body = &cc_spr[BODY_OF_POSE[pose]];
    int by = 10 + (pose == POSE_IDLE1 ? 0 : 0);
    for (int y = 0; y < body->h; y++)
        for (int x = 0; x < body->w; x++) {
            uint8_t c = body->px[y * body->w + x];
            if (c == TRANSPARENT) continue;
            if (c == C_RED) c = d->shirt;
            else if (c == C_MAROON) c = d->shirt_dark;
            else if (c == C_NAVY) c = d->pants;
            else if (c == C_YELLOW) c = d->sash;
            px[(by + y) * W + 2 + x] = c;
        }
    const Sprite *head = &cc_spr[CS_HEAD0 + f];
    int hx = HEAD_OFS[pose][0], hy = HEAD_OFS[pose][1];
    for (int y = 0; y < head->h; y++)
        for (int x = 0; x < head->w; x++) {
            uint8_t c = head->px[y * head->w + x];
            if (c != TRANSPARENT && hx + x < W && hy + y < H) px[(hy + y) * W + hx + x] = c;
        }
    Sprite *s = &cc_fighter_spr[f][pose];
    s->w = W;
    s->h = H;
    s->px = px;
}

void cc_art_load(void) {
    static int loaded;
    if (loaded) return;
    for (int i = 0; i < ARRAY_LEN(ART); i++) {
        int len = (int)strlen(ART[i].px);
        if (len != ART[i].w * ART[i].h) fprintf(stderr, "cutlass art %d: expected %d got %d\n", ART[i].id, ART[i].w * ART[i].h, len);
        spr_make(&cc_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
    }
    for (int f = 0; f < CC_FIGHTERS; f++)
        for (int p = 0; p < POSE_COUNT; p++) compose(f, p);
    loaded = 1;
}
