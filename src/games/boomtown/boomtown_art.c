/* BOOMTOWN - pixel art (palette-letter strings, see gfx.h). All drawn for UFO 40. */
#include "boomtown.h"

Sprite bm_spr[BS_SPRITE_COUNT];

/* ---- Mossbury's folk: four looks, 12 x 14 --------------------------- */
static const char FOLK1[] = /* the lamplighter in his cap */
    "...kkkkk...."
    "..kbbbbbk..."
    ".kbbbbbbbk.."
    "..khhhhhk..."
    "..khkhkhk..."
    "..khhhhhk..."
    "...kkkkk...."
    "..ktttttk..."
    ".kttttttkyk."
    ".ktkttttkak."
    ".ktkttttkk.."
    "..ktttttk..."
    "..kbk.kbk..."
    "..kk...kk...";
static const char FOLK2[] = /* the baker in her bonnet */
    "...kkkkkk..."
    "..kcccccck.."
    ".kcchhhhcck."
    ".kchkhhkhck."
    ".kchhhhhhck."
    "..kkhhhhkk.."
    "..kVVVVVVk.."
    ".kVVVVVVVVk."
    ".kpVVVVVVpk."
    "..kppppppk.."
    ".kppppppppk."
    ".kppppppppk."
    "..kkkkkkkk.."
    "...kk..kk...";
static const char FOLK3[] = /* old Mr Pell with his stick */
    "............"
    "...kkkkk...."
    "..khhhhhk..."
    "..khkhkhk..."
    "..klllllk..."
    "..kllllk...."
    "..kkkkkk...."
    ".kfffffffk.."
    ".kfkfffkfk.."
    ".kfkfffkfkb."
    "..kfffffk.b."
    "..kfffffk.b."
    "..kbk.kbk.b."
    "..kk...kk.k.";
static const char FOLK4[] = /* a child in a red cap */
    "............"
    "............"
    "...kkkkk...."
    "..krrrrrk..."
    ".krrrrrrrk.."
    "..khhhhhk..."
    "..khkhkhk..."
    "..khhhhhk..."
    "...kkkkk...."
    "..kuuuuuk..."
    ".kuuuuuuuk.."
    "..kuuuuuk..."
    "..kek.kek..."
    "..kk...kk...";

/* ---- bogles --------------------------------------------------------- */
static const char SMALL1[] =
    "............"
    "............"
    "..k......k.."
    ".kzk....kzk."
    ".kzzkkkkzzk."
    "kzzzzzzzzzzk"
    "kzykzzzzykzk"
    "kzzzzzzzzzzk"
    "kzzkwkkwkzzk"
    ".kzzzzzzzzk."
    "..kjjjjjjk.."
    "...kk..kk...";
static const char SMALL2[] =
    "............"
    "............"
    "............"
    "..k......k.."
    ".kzkkkkkkzk."
    "kzzzzzzzzzzk"
    "kzykzzzzykzk"
    "kzzzzzzzzzzk"
    "kzzkwkkwkzzk"
    "kzzzzzzzzzzk"
    ".kjjjjjjjjk."
    "..kk....kk..";
static const char BIG1[] =
    "..k........k.."
    ".kck......kck."
    ".kcck....kcck."
    "..kVVkkkkVVk.."
    ".kVVVVVVVVVVk."
    "kVVVVVVVVVVVVk"
    "kVwwVVVVVVwwVk"
    "kVwkVVVVVVwkVk"
    "kVVVVVVVVVVVVk"
    "kVVkkkkkkkkVVk"
    "kVVkwVwwVwkVVk"
    ".kVVkkkkkkVVk."
    "..kppppppppk.."
    "..kkk....kkk..";
static const char BIG2[] =
    "..k........k.."
    ".kck......kck."
    ".kcck....kcck."
    "..kVVkkkkVVk.."
    ".kVVVVVVVVVVk."
    "kVVVVVVVVVVVVk"
    "kVwwVVVVVVwwVk"
    "kVwkVVVVVVwkVk"
    "kVVVVVVVVVVVVk"
    "kVVVVVVVVVVVVk"
    "kVVVkkkkkkVVVk"
    ".kVVVVVVVVVVk."
    "..kppppppppk.."
    ".kkk......kkk.";
static const char OLD1[] =
    "....kkkkkk...."
    "...kqqqqqqk..."
    "..kqzqqqqzqk.."
    ".kqqqqqqqqqqk."
    ".kqykqqqqykqk."
    ".kqqqqqqqqqqk."
    ".kqllllllllqk."
    "kqqllllllllqqk"
    "kqqqllllllqqqk"
    "kqzqqllllqqzqk"
    "kqqqqqllqqqqqk"
    ".kqqqqqqqqqqk."
    "..kfffkkfffk.."
    "..kkkk..kkkk..";
static const char OLD2[] =
    "....kkkkkk...."
    "...kqqqqqqk..."
    "..kqzqqqqzqk.."
    ".kqqqqqqqqqqk."
    ".kqkkqqqqkkqk."
    ".kqqqqqqqqqqk."
    ".kqllllllllqk."
    "kqqllllllllqqk"
    "kqqqllllllqqqk"
    "kqzqqllllqqzqk"
    "kqqqqqllqqqqqk"
    ".kqqqqqqqqqqk."
    "..kfffkkfffk.."
    "..kkkk..kkkk..";

/* ---- Hazel, the fireworks maker, 16 x 24 ---------------------------- */
static const char HAZEL1[] =
    ".....kkkkk......"
    "....kbbbbbk....."
    "...kbbbbbbbk...."
    "...kCCkbkCCk...."
    "...kCCkbkCCk...."
    "...khhhhhhhk...."
    "...khkhhhkhk...."
    "...khhhKhhhk...."
    "....kkhhhkk....."
    "...krrrrrrrk...."
    "..krrkcccckrrk.."
    "..khkcccccckhk.."
    "..khkcccccckhk.."
    "...kkcccccckk..."
    "....kcccccck...."
    "....kcceecck...."
    "....kcccccck...."
    "....kBBBBBBk...."
    "....kBBkkBBk...."
    "....kBBkkBBk...."
    "....kbbk.kbbk..."
    "...kbbbk.kbbbk.."
    "...kkkkk.kkkkk.."
    "................";
static const char HAZEL2[] =
    ".....kkkkk......"
    "....kbbbbbk....."
    "...kbbbbbbbk...."
    "...kCCkbkCCk...."
    "...kCCkbkCCk...."
    "...khhhhhhhk...."
    "...khhhhhhhk...."
    "...khhhKhhhk...."
    "....kkhhhkk....."
    "...krrrrrrrk...."
    "..krrkcccckrrk.."
    "..khkcccccckhk.."
    "..khkcccccckhk.."
    "...kkcccccckk..."
    "....kcccccck...."
    "....kcceecck...."
    "....kcccccck...."
    "....kBBBBBBk...."
    "....kBBkkBBk...."
    "....kBBkkBBk...."
    "....kbbk.kbbk..."
    "...kbbbk.kbbbk.."
    "...kkkkk.kkkkk.."
    "................";
static const char HAZEL_CHEER[] =
    ".....kkkkk......"
    "....kbbbbbk....."
    "...kbbbbbbbk...."
    "...kCCkbkCCk...."
    "...kCCkbkCCk...."
    "...khhhhhhhk...."
    "h..khkhhhkhk..h."
    "k..khhhKhhhk..k."
    "khk.kkhhhkk.khk."
    "khkkrrrrrrrkkhk."
    ".kkrrkcccckrrkk."
    "...kkcccccckk..."
    "...kkcccccckk..."
    "...kkcccccckk..."
    "....kcccccck...."
    "....kcceecck...."
    "....kcccccck...."
    "....kBBBBBBk...."
    "....kBBkkBBk...."
    "....kBBkkBBk...."
    "....kbbk.kbbk..."
    "...kbbbk.kbbbk.."
    "...kkkkk.kkkkk.."
    "................";

/* ---- the fireworks, 12 x 12, drawn facing up and turned in code ------ */
static const char P_STARBURST[] =
    "......t....."
    ".....t......"
    ".....t......"
    "....kkkkk..."
    "...krrrrrk.."
    "..krrwrrrrk."
    "..krwrrrrrk."
    "..kaaaaaaak."
    "..krrrrrrrk."
    "..krrrrrrrk."
    "...krrrrrk.."
    "....kkkkk...";
static const char P_CANDLE[] =
    "....kkkk...."
    "....kkkk...."
    "...kBBBBk..."
    "...kwwwwk..."
    "...kBBBBk..."
    "...kBBBBk..."
    "...kwwwwk..."
    "...kBBBBk..."
    "...kBBBBk..."
    "..kkbbbbkk.."
    "..kbbbbbbk.."
    "..kkkkkkkk..";
static const char P_ROCKET[] =
    ".....kk....."
    "....krrk...."
    "....krrk...."
    "...krwrrk..."
    "...krrrrk..."
    "...kyyyyk..."
    "...krrrrk..."
    "..kkrrrrkk.."
    "..krkbbkrk.."
    "..kk.bb.kk.."
    ".....bb....."
    ".....bb.....";
static const char P_PERCH[] =
    "...kkkkkk..."
    "..krrrrrrk.."
    ".krwwwwwwrk."
    ".krwrrrrwrk."
    ".krwrkkrwrk."
    ".krwrkkrwrk."
    ".krwrrrrwrk."
    ".krwwwwwwrk."
    "..krrrrrrk.."
    "...kkbbkk..."
    ".....bb....."
    "...kkbbkk...";
static const char P_BANGER[] =
    "............"
    "....kkkk...."
    "....kook...."
    "....kook...."
    ".kkkkyykkkk."
    ".kooyyyyook."
    ".kooyyyyook."
    ".kkkkyykkkk."
    "....kook...."
    "....kook...."
    "....kkkk...."
    "............";
static const char P_PINWHEEL[] =
    "............"
    ".kk......kk."
    ".kPk....kPk."
    "..kPk..kPk.."
    "...kPkkPk..."
    "....kyyk...."
    "....kyyk...."
    "...kVkkVk..."
    "..kVk..kVk.."
    ".kVk....kVk."
    ".kk......kk."
    "............";
static const char P_JACK[] =
    "............"
    "....kkkk...."
    "...kiiiik..."
    "...kkkkkk..."
    "...kyyyyk..."
    "...kkkkkk..."
    "...kiiiik..."
    "...kkkkkk..."
    "...kyyyyk..."
    "...kkkkkk..."
    "..kbbbbbbk.."
    "..kkkkkkkk..";
static const char P_TWIN[] =
    "....kkkk...."
    "...kkddkk..."
    "...kjjjjk..."
    "...kwwwwk..."
    "...kjjjjk..."
    "...kjjjjk..."
    "...kjjjjk..."
    "...kjjjjk..."
    "...kwwwwk..."
    "...kjjjjk..."
    "...kkddkk..."
    "....kkkk....";
static const char P_FOUNTAIN[] =
    "............"
    "............"
    ".....kk....."
    "....kyyk...."
    "....kook...."
    "...kooook..."
    "...kKKKKk..."
    "..kooooook.."
    "..kKKKKKKk.."
    ".kooooooook."
    ".kkkkkkkkkk."
    "............";

static const char *piece_map(int kind) {
    switch (kind) {
    case BM_STARBURST: return P_STARBURST;
    case BM_CANDLE: return P_CANDLE;
    case BM_ROCKET: return P_ROCKET;
    case BM_PERCH: return P_PERCH;
    case BM_BANGER: return P_BANGER;
    case BM_PINWHEEL: return P_PINWHEEL;
    case BM_JACK: return P_JACK;
    case BM_TWIN: return P_TWIN;
    case BM_FOUNTAIN: return P_FOUNTAIN;
    default: return NULL;
    }
}

/* The fuse tip of each firework (in its 12 x 12 map, facing up). */
static void fuse_tip(int kind, int *fx, int *fy) {
    switch (kind) {
    case BM_STARBURST: *fx = 6; *fy = 0; break;
    case BM_BANGER: *fx = 6; *fy = 0; break;
    default: *fx = -1; *fy = -1; break;
    }
}

void bm_draw_piece(int x, int y, int kind, int dir, int col, int t) {
    const char *m = piece_map(kind);
    if (!m) return;
    int d = bm_turns(kind) ? dir & 3 : 0;
    for (int oy = 0; oy < 12; oy++)
        for (int ox = 0; ox < 12; ox++) {
            int sx, sy; /* the source pixel for this screen pixel */
            switch (d) {
            case 1: sx = oy; sy = 11 - ox; break;      /* facing right */
            case 2: sx = 11 - ox; sy = 11 - oy; break; /* facing down */
            case 3: sx = 11 - oy; sy = ox; break;      /* facing left */
            default: sx = ox; sy = oy; break;
            }
            char ch = m[sy * 12 + sx];
            if (ch == '.') continue;
            int c = pal_char_index(ch);
            if (col == 2 && c == C_RED) c = C_LEAF;   /* the green skyrocket */
            if (col == 2 && c == C_WINE) c = C_FOREST;
            gfx_pset(x + 2 + ox, y + 2 + oy, c);
        }
    int fx, fy;
    fuse_tip(kind, &fx, &fy);
    if (fx >= 0 && (t / 4) % 3) gfx_pset(x + 2 + fx, y + 2 + fy - ((t / 4) % 2), (t / 8) % 2 ? C_YELLOW : C_WHITE);
}

static void make(int id, int w, int h, const char *data) {
    if ((int)strlen(data) != w * h) {
        /* a mistyped sprite shows up as a magenta block rather than garbage */
        static char bad[16 * 24 + 1];
        memset(bad, 'P', sizeof bad - 1);
        bad[w * h < (int)sizeof bad - 1 ? w * h : (int)sizeof bad - 1] = 0;
        spr_make(&bm_spr[id], w, h, bad);
        return;
    }
    spr_make(&bm_spr[id], w, h, data);
}

bool bm_art_ok(void) {
    static const struct { const char *s; int n; } ALL[] = {
        {FOLK1, 168}, {FOLK2, 168}, {FOLK3, 168}, {FOLK4, 168},
        {SMALL1, 144}, {SMALL2, 144}, {BIG1, 196}, {BIG2, 196}, {OLD1, 196}, {OLD2, 196},
        {HAZEL1, 384}, {HAZEL2, 384}, {HAZEL_CHEER, 384},
        {P_STARBURST, 144}, {P_CANDLE, 144}, {P_ROCKET, 144}, {P_PERCH, 144}, {P_BANGER, 144},
        {P_PINWHEEL, 144}, {P_JACK, 144}, {P_TWIN, 144}, {P_FOUNTAIN, 144},
    };
    for (int i = 0; i < ARRAY_LEN(ALL); i++)
        if ((int)strlen(ALL[i].s) != ALL[i].n) return false;
    return true;
}

void bm_art_load(void) {
    if (bm_spr[BS_FOLK1].px) return;
    make(BS_FOLK1, 12, 14, FOLK1);
    make(BS_FOLK2, 12, 14, FOLK2);
    make(BS_FOLK3, 12, 14, FOLK3);
    make(BS_FOLK4, 12, 14, FOLK4);
    make(BS_SMALL1, 12, 12, SMALL1);
    make(BS_SMALL2, 12, 12, SMALL2);
    make(BS_BIG1, 14, 14, BIG1);
    make(BS_BIG2, 14, 14, BIG2);
    make(BS_OLD1, 14, 14, OLD1);
    make(BS_OLD2, 14, 14, OLD2);
    make(BS_HAZEL1, 16, 24, HAZEL1);
    make(BS_HAZEL2, 16, 24, HAZEL2);
    make(BS_HAZEL_CHEER, 16, 24, HAZEL_CHEER);
}
