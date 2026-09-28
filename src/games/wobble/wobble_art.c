/* WOBBLE DERBY - pixel art (palette-letter strings, see gfx.h). All drawn
 * for UFO 40: the wobblers, the sixteen punters, the track's regulars and
 * the bits of junk that end up on the track. */
#include "wobble.h"

Sprite wb_spr[WA_COUNT];
Sprite wb_face[WB_CHARS];
static bool art_ok = true;

/* A wobbler, facing right: 'r' is the body and 'o' the antenna bobble;
 * both take the wobbler's own colours. */
static const char RUN1[] =
    "..........k....."
    ".........kok...."
    "..........k....."
    ".....kkkkkkk...."
    "...kkrrrrrrrkk.."
    "..krrrrrrrwwwrk."
    ".krrrrrrrwwwkkk."
    ".krrrrrrrwwwkkrk"
    ".krrrrrrrrwwwrrk"
    ".krrrrrrrrrrrkk."
    "..krrrrrrrrrrk.."
    "...kkkkkkkkkk..."
    "...kk.....kk...."
    "..kkk....kkk....";
static const char RUN2[] =
    ".........k......"
    "........kok....."
    ".........k......"
    ".....kkkkkkk...."
    "...kkrrrrrrrkk.."
    "..krrrrrrrwwwrk."
    ".krrrrrrrwwwkkk."
    ".krrrrrrrwwwkkrk"
    ".krrrrrrrrwwwrrk"
    ".krrrrrrrrrrrkk."
    "..krrrrrrrrrrk.."
    "...kkkkkkkkkk..."
    ".....kk.kk......"
    "....kkk.kkk.....";
static const char IDLE[] =
    "..........k....."
    ".........kok...."
    "..........k....."
    ".....kkkkkkk...."
    "...kkrrrrrrrkk.."
    "..krrrrrrrwwwrk."
    ".krrrrrrrwwkwwk."
    ".krrrrrrrwwkwwrk"
    ".krrrrrrrrwwwrrk"
    ".krrrrrrrrrrkrk."
    "..krrrrrrrrrrk.."
    "...kkkkkkkkkk..."
    "....kk....kk...."
    "...kkk...kkk....";
static const char DEAD[] =
    "................"
    "................"
    "................"
    "................"
    "................"
    "......kkkkkkk..."
    "....kkrrrrrrrkk."
    "..kkrrrrrrrkwkrk"
    ".krrrrrrrrrrkwrk"
    ".krrrrrrrrrkwkrk"
    ".krrrrrrrrrrrrk."
    "..kkkkkkkkkkkk.."
    "..kk.kk..kk.kk.."
    ".kok............";

/* junk and the like */
static const char PEEL[] =
    "..yy.."
    ".yaay."
    "yay.ay"
    "a....a";
static const char CAN[] =
    ".kkkk."
    "krrrgk"
    "krwwrk"
    "krrrrk"
    ".kkkk.";
static const char BOTTLE[] =
    "..jj.."
    "..jj.."
    ".jzzj."
    ".jzij."
    ".jzzj."
    ".jjjj.";
static const char METEOR[] =
    "....oa.."
    "..oaya.."
    ".oayyao."
    "kbyyyyao"
    "kbbyyao."
    "kbbbao.."
    ".kbbk..."
    "..kk....";
static const char TRUCK[] =
    "......................kk"
    ".kkkkkkkkkkkkkkkkk...kgk"
    "kzzzzzzzzzzzzzzzzk.kkgk."
    "kzfzfzfzfzfzfzfzzkkuuuk."
    "kzzzzzzzzzzzzzzzzkkuuuuk"
    "kzfzfzfzfzfzfzfzzkkggggk"
    "kzzzzzzzzzzzzzzzzkkggggk"
    "kkkkkkkkkkkkkkkkkkkkkkkk"
    "..kkk..........kkk.kkk.."
    ".kgggk........kgggkgggk."
    "..kkk..........kkk.kkk..";
static const char STONE[] =
    "..kkkk.."
    ".kglglk."
    "kgggkggk"
    "kgkkkkgk"
    "kgggkggk"
    "kgggkggk"
    "kgggggsk"
    "kssssssk"
    "kkkkkkkk";

/* the fixer's jobs, 8 x 8 icons */
static const char PEP[] =
    "...oo..."
    "..oyyo.."
    ".oyyyyo."
    ".oyccyo."
    "otccccto"
    "ottccttb"
    ".bttttb."
    "..bbbb..";
static const char NIGHT[] =
    "...kk..."
    "..kssk.."
    "..kVVk.."
    ".kVVVVk."
    "kVwVVwVk"
    "kVVwwVVk"
    "kVwVVwVk"
    ".kkkkkk.";
static const char FIZZ[] =
    "..kkkk.."
    ".kCwCCk."
    ".kCCCCk."
    "kkkkkkkk"
    "kPPPPPPk"
    "kPKPPKPk"
    ".kPPPPk."
    "..kkkk..";
static const char NOBBLE[] =
    ".....kk."
    "....kbbk"
    "...kbbk."
    "..kbbk.."
    ".kbbk..."
    "kgbk...."
    "kggk...."
    ".kk.....";
static const char MINDER[] =
    ".kkkkkk."
    "kBuuuuBk"
    "kBuyyuBk"
    "kByyyyBk"
    "kBuyyuBk"
    ".kBuuBk."
    "..kBBk.."
    "...kk...";

/* ---- faces, 16 x 16 (drawn at twice the size) ------------------------ */
static const char F_GLOOB[] =
    "................"
    "......kkkk......"
    "....kkiiiikk...."
    "...kiiiiiiiik..."
    "..kiiwwiiiwwik.."
    "..kiwkwiiwkwik.."
    ".kiiwwwiiwwwiik."
    ".kiiiiiiiiiiiik."
    ".kiiiiiiiiiiiik."
    ".kiiikiiiiikiik."
    ".kiiiikkkkkiiik."
    ".kziiiiiiiiiizk."
    ".kzzziiiiiizzzk."
    "..kzzzzzzzzzzk.."
    "...kkkkkkkkkk..."
    "................";
static const char F_VEXA[] =
    "..k..........k.."
    "..Vk........kV.."
    "...Vk......kV..."
    "....kkkkkkkk...."
    "...kppppppppk..."
    "..kppppppppppk.."
    "..kkkkkpkkkkkk.."
    "..kCwCkpkCwCkk.."
    "..kkkkkpkkkkkk.."
    "..kppppppppppk.."
    "..kppppKKppppk.."
    "...kppKKKKppk..."
    "....kkppppkk...."
    "...kVVkkkkVVk..."
    "..kVVVVVVVVVVk.."
    "..kVVVVVVVVVVk..";
static const char F_ZORP[] =
    "................"
    "....kkkkkkkk...."
    "...koooooooook.."
    "..koooooooooook."
    "..koooowwwwoook."
    ".kooowwwwwwwook."
    ".kooowwkkwwwook."
    ".kooowwkkwwwook."
    ".koooowwwwooook."
    ".kooooooooooook."
    ".koookwwwwkoook."
    "..koookkkkoook.."
    "..kaoooooooak..."
    "...kaaaaaaaak..."
    "..kyyykkkkyyyk.."
    "..kyyyyyyyyyyk..";
static const char F_OOZ[] =
    "................"
    ".....kkkkkk....."
    "...kkKKKKKKkk..."
    "..kKKKKKKKKKKk.."
    "..kKKwwKKwwKKk.."
    ".kKKwkwKKwkwKKk."
    ".kKKKKKKKKKKKKk."
    ".kKKKKPPPPKKKKk."
    ".kKKKKKPPKKKKKk."
    ".kKKKKKKKKKKKKk."
    "..kKwKwKwKwKKk.."
    "..kKKKKKKKKKKk.."
    "..kPPPPPPPPPPk.."
    ".kPPPPPPPPPPPPk."
    ".kPPPPPPPPPPPPk."
    ".kkkkkkkkkkkkkk.";
static const char F_K7[] =
    ".......k........"
    ".......y........"
    ".......k........"
    "...kkkkkkkkkk..."
    "...kggggggggk..."
    "...kgkkkkkkgk..."
    "...kgkCkkCkgk..."
    "...kgkkkkkkgk..."
    "...kggggggggk..."
    "...kgkykykygk..."
    "...kggggggggk..."
    "...kkkkkkkkkk..."
    ".....kssssk....."
    "...kkssssssk...."
    "..ksssssssssk..."
    "..ksssssssssk...";
static const char F_NIBBS[] =
    "................"
    "................"
    "................"
    "......kkkk......"
    ".....kggggk....."
    "....kggggggk...."
    "...kgwwggwwgk..."
    "...kgkwggkwgk..."
    "...kggggggggk..."
    "...kgggkkgggk..."
    "....kggggggk...."
    ".....kkkkkk....."
    ".....kddddk....."
    "....kddddddk...."
    "...kddddddddk..."
    "...kddddddddk...";
static const char F_FUZZ[] =
    "....kkkkkkkk...."
    "...kkkkkkkkkk..."
    "...kkkkkkkkkk..."
    "..kkkkkkkkkkkkk."
    "..khhhhhhhhhhk.."
    ".khhhhhhhhhhhhk."
    ".khhwwhhhkkkhhk."
    ".khhwkhhkyykkhk."
    ".khhhhhhhkkkhhk."
    ".khhhhhhhhhhhhk."
    ".khheeeeeeeehhk."
    ".kheeeeeeeeeehk."
    "..keeekkkkeeek.."
    "...keeeeeeeek..."
    "..kkwwwkkwwwkk.."
    ".kNNNNwwwwNNNNk.";
static const char F_QUILLA[] =
    "......k.k.k....."
    ".....kCkCkCk...."
    "....kCCCCCCCk..."
    "...kCCCCCCCCCk.."
    "..kCCwwwCCCCCk.."
    "..kCwkkwCCCCCk.."
    "..kCwkkwCCaaaak."
    "..kCCwwwCaaaaaak"
    "..kCCCCCCCaaaak."
    "..kCCCCCCCCkkk.."
    "...kCCCCCCCCk..."
    "...kIIIIIIIIk..."
    "..kIIIIIIIIIIk.."
    "..kIIICCCCIIIk.."
    ".kIIICCCCCCIIIk."
    ".kIICCCCCCCCIIk.";
static const char F_MO[] =
    "....kkkkkkkk...."
    "...kaaaaaaaak..."
    "..kaaayyyyaaak.."
    "..kkkkkkkkkkkk.."
    "..keeeeeeeeeek.."
    ".keeeeeeeeeeeek."
    ".keekkeeeekkeek."
    ".keeeeeeeeeeeek."
    ".keeeeeKKeeeeek."
    ".keeeeKKKKeeeek."
    ".keeeeewweeeeek."
    "..keeeeeeeeeek.."
    "...kkeeeeeekk..."
    "..kbbkkkkkkbbk.."
    ".kbbbbbbbbbbbbk."
    ".kbbbbbbbbbbbbk.";
static const char F_TILLY[] =
    "......kkk......."
    "......kak......."
    ".......k........"
    "...kkkkkkkkkk..."
    "..kllllllllllk.."
    "..klkkkkkkkklk.."
    "..klkuCkkuCklk.."
    "..klkkkkkkkklk.."
    "..kllllllllllk.."
    "..klllkkkklllk.."
    "..kllllllllllk.."
    "...kkkkkkkkkk..."
    "....kaaaaaak...."
    "..kkaakkkkaakk.."
    ".kaaaaaaaaaaaak."
    ".kaaaaaaaaaaaak.";
static const char F_PEPPER[] =
    "..kk........kk.."
    "..kok......kok.."
    "..kook....kook.."
    "..koookkkkoook.."
    "..kBBBBBBBBBBk.."
    ".kBBBBBBBBBBBBkk"
    ".kooooooooooook."
    ".koowkoooowkook."
    ".koowkoooowkook."
    ".kooooooooooook."
    ".kccccoKKocccck."
    "..kcccckkcccck.."
    "...kccccccccck.."
    "...kkkkkkkkkk..."
    "..kBBBBrrBBBBk.."
    ".kBBBBBrrBBBBBk.";
static const char F_FOXY[] =
    ".kk..........kk."
    ".kok........kok."
    ".kook......kook."
    ".koookkkkkkoook."
    "..kPPPPPPPPPPk.."
    ".kPPKKKKKKKKPPk."
    ".kPkkkkkkkkkkPk."
    ".kkwwkookkwwkkk."
    ".koooooooooooook"
    ".kooooooooooook."
    "..kccccookcccck."
    "..kcccccoocccck."
    "...kccccccccck.."
    "....kkkkkkkkk..."
    "...kPPPPPPPPPk.."
    "..kPPPPPPPPPPPk.";
static const char F_POSY[] =
    "................"
    "......kkkk......"
    "....kkttttkk...."
    "...kttttttttk..."
    ".kkttttttttttkk."
    "kttttKKKKKKttttk"
    ".kkkkkkkkkkkkkk."
    "...kccccccccck.."
    "...kcwkcccwkck.."
    "...kcccccccccck."
    "...kcKcccccKcck."
    "...kccccrrcccck."
    "....kcccccccck.."
    ".....kkkkkkkk..."
    "....kzzzKzzzzk.."
    "...kzzzzzzzzzzk.";
static const char F_KIP[] =
    "....kkkkkkkk...."
    "...kaaaaaaaak..."
    "..kaaaaaaaaaak.."
    "..kaakkkkkkaak.."
    "..kakCCkkCCkak.."
    "..kakCIkkCIkak.."
    "..kakkkkkkkkak.."
    "..kttttttttttk.."
    "..kttttttttttk.."
    "..kttttkkttttk.."
    "..kttttttttttk.."
    "...kttttttttk..."
    "....kkkkkkkk...."
    "...kggoggoggk..."
    "..kgggggggggk..."
    "..kgggggggggk...";
static const char F_TWIG[] =
    "................"
    "....kkkkk......."
    "...kzzzzzkk....."
    "..kzzjzzzzzkk..."
    "..kzzzzzzzzzzk.."
    ".kzzzzzzzzkkzzk."
    ".kzzzzzzzkwwkzk."
    ".kzjzzzzzkwkkzk."
    ".kzzzzzzzzkkzzk."
    ".kzzzzzzzzzzzzzk"
    ".kiiiiiiiiiiiiik"
    "..kiiiiiiiiiiik."
    "...kkkkkkkkkkk.."
    "..kjzzzzzzzzjk.."
    ".kzzjzzzzzzjzzk."
    ".kzzzzzzzzzzzzk.";
static const char F_WADE[] =
    "....kkkkkkkk...."
    "...kbbbbbbbbk..."
    "...kbbbbbbbbk..."
    ".kkkkkkkkkkkkkk."
    "kbbbbbbbbbbbbbbk"
    ".kkttttttttttkk."
    "..kttttttttttk.."
    "..ktwkttttwktk.."
    "..kttttttttttk.."
    "..kttttbbttttk.."
    "..kttbbbbbbttk.."
    "..krrrrrrrrrrk.."
    "..krrwrrrrwrrkk."
    "...krrrrrrrrkrrk"
    "..kttttttttttkk."
    ".kttttttttttttk.";
/* the track's regulars */
static const char F_BOOTH[] =
    "........k......."
    ".......kyk......"
    "........k......."
    "..kkkkkkkkkkkk.."
    "..kssssssssssk.."
    "..ksnnnnnnnnsk.."
    "..ksnCCnnCCnsk.."
    "..ksnCCnnCCnsk.."
    "..ksnnnnnnnnsk.."
    "..ksnnCCCCnnsk.."
    "..ksnnnnnnnnsk.."
    "..kssssssssssk.."
    "..kskrkykgkssk.."
    "..kssssssssssk.."
    "..kkkkkkkkkkkk.."
    "...kk......kk...";
static const char F_ALLEY[] =
    "................"
    "................"
    ".....kkkkkk....."
    "....kddddddk...."
    "...kddddddddk..."
    ".kkkkkkkkkkkkkk."
    "kddddddddddddddk"
    ".kkjjjjjjjjjjkk."
    "..kjjkkjjkkjjk.."
    "..kjjjyjjjyjjk.."
    "..kjjjjjjjjjjk.."
    "..kjgjgjgjgjjktk"
    "..kjjjkkkkjjjktk"
    "...kjjjjjjjjk.k."
    "..kddddddddddk.."
    ".kddddddddddddk.";
static const char F_LENDER[] =
    "................"
    "....kkkkkk......"
    "...kyyyyyykk...."
    "..kyyyyyyyyyk..."
    "..kyywwyyyyyyk.."
    ".kyywkkwyyyyyk.."
    ".kyywkkwyyyyyyk."
    ".kyyywwyyyyyyyk."
    ".kfyyyyyyyyyyyk."
    ".kfffyyyyyyrrrk."
    ".kfkfkfkfkfrrk.."
    "..kfffffffffk..."
    "..kfyfyfyfyfk..."
    "...kfffffffk...."
    "..kfffffffffk..."
    ".kfffffffffffk..";
static const char F_STABLE[] =
    "....kkkkkkkk...."
    "....kkkkkkkk...."
    "....kkkkkkkk...."
    "....kkrrrrkk...."
    "..kkkkkkkkkkkk.."
    "...kuuuuuuuuk..."
    "..kuuuuuuuuuuk.."
    "..kuukkuuuwwuk.."
    "..kuuwkuuwkwuk.."
    "..kuuuuuuuwwuk.."
    "..kuuuukkuuuuk.."
    "...kuuuuuuuuk..."
    "....kkkkkkkk...."
    "...kkwwkkwwkk..."
    "..kkkkwrrwkkkk.."
    ".kkkkkkwwkkkkkk.";
static const char F_COACH[] =
    "................"
    "...kkkkkkkkkk..."
    "..kBBBBBBBBBBk.."
    ".kBBBBBBBBBBBBkk"
    ".kkkkkkkkkkkkkkB"
    "..kbbbbbbbbbbk.."
    ".kbbwwwbbwwwbbk."
    ".kbwwkwbbwkwwbk."
    ".kbbwwwbbwwwbbk."
    ".kbbbbbaabbbbbk."
    ".kbbbbbaabbbbbk."
    "..kbbcccccbbbk.."
    "..kbcccccccbbk.."
    "...kccccccccky.."
    "..kBBBBBBBBBBky."
    ".kBBBBBBBBBBBBk.";
static const char F_ANCHOR[] =
    "..k..........k.."
    "...k........k..."
    "....k......k...."
    ".kkkkkkkkkkkkkk."
    ".ksssssssssssssk"
    ".ksNNNNNNNNNNsk."
    ".ksNwwNNNNwwNsk."
    ".ksNwkNNNNwkNsk."
    ".ksNNNNNNNNNNsk."
    ".ksNNNKKKKNNNsk."
    ".ksNNNNNNNNNNsk."
    ".kssssssssssssk."
    ".kkkkkkkkkkkkkk."
    "...kPPwwwwPPk..."
    "..kPPPPwrPPPPk.."
    ".kPPPPPwrPPPPPk.";

static const char *const FACES[WB_CHARS] = {
    F_GLOOB, F_VEXA, F_ZORP, F_OOZ, F_K7, F_NIBBS, F_FUZZ, F_QUILLA,
    F_MO, F_TILLY, F_PEPPER, F_FOXY, F_POSY, F_KIP, F_TWIG, F_WADE,
};

/* every drawing is checked for its size; a short one is padded, never read past */
static void mk(Sprite *s, int w, int h, const char *data) {
    int n = (int)strlen(data);
    if (n == w * h) { spr_make(s, w, h, data); return; }
    art_ok = false;
    char *buf = (char *)malloc((size_t)w * h + 1);
    for (int i = 0; i < w * h; i++) buf[i] = i < n ? data[i] : '.';
    buf[w * h] = 0;
    spr_make(s, w, h, buf);
    free(buf);
}

bool wb_art_ok(void) { return art_ok; }

void wb_art_load(void) {
    if (wb_spr[WA_RUN1].px) return;
    mk(&wb_spr[WA_RUN1], 16, 14, RUN1);
    mk(&wb_spr[WA_RUN2], 16, 14, RUN2);
    mk(&wb_spr[WA_TRIP], 16, 14, RUN1);
    mk(&wb_spr[WA_DEAD], 16, 14, DEAD);
    mk(&wb_spr[WA_IDLE], 16, 14, IDLE);
    mk(&wb_spr[WA_PEEL], 6, 4, PEEL);
    mk(&wb_spr[WA_CAN], 6, 5, CAN);
    mk(&wb_spr[WA_BOTTLE], 6, 6, BOTTLE);
    mk(&wb_spr[WA_METEOR], 8, 8, METEOR);
    mk(&wb_spr[WA_TRUCK], 24, 11, TRUCK);
    mk(&wb_spr[WA_STONE], 8, 9, STONE);
    mk(&wb_spr[WA_BOOTH], 16, 16, F_BOOTH);
    mk(&wb_spr[WA_ALLEY], 16, 16, F_ALLEY);
    mk(&wb_spr[WA_LENDER], 16, 16, F_LENDER);
    mk(&wb_spr[WA_STABLE], 16, 16, F_STABLE);
    mk(&wb_spr[WA_COACH], 16, 16, F_COACH);
    mk(&wb_spr[WA_ANCHOR], 16, 16, F_ANCHOR);
    mk(&wb_spr[WA_PEP], 8, 8, PEP);
    mk(&wb_spr[WA_NIGHTSHADE], 8, 8, NIGHT);
    mk(&wb_spr[WA_FIZZ], 8, 8, FIZZ);
    mk(&wb_spr[WA_NOBBLE], 8, 8, NOBBLE);
    mk(&wb_spr[WA_MINDER], 8, 8, MINDER);
    for (int i = 0; i < WB_CHARS; i++) mk(&wb_face[i], 16, 16, FACES[i]);
}

int wb_body_col(int racer) { return WB_DEF[racer % WB_RACERS].body; }

/* markings, in sprite space: where the pattern colour goes if the body is
 * there (so they follow the drawing, whatever the frame) */
static bool mark_at(int pattern, int x, int y) {
    switch (pattern) {
    case 1: return ((x * 5 + y * 3) % 7 == 0) && y >= 4 && y <= 10;           /* spots */
    case 2: return (x % 3 == 0) && y >= 4 && y <= 10 && x < 9;                 /* stripes */
    case 3: return y >= 5 && y <= 7 && x >= 6;                                  /* a mask */
    case 4: return y >= 4 && y <= 5 && x >= 4 && x <= 9;                        /* a tuft */
    default: return false;
    }
}

void wb_draw_racer(int racer, int frame, int x, int y, int flags) {
    const WbDef *d = &WB_DEF[racer % WB_RACERS];
    uint8_t map[256];
    pal_identity(map);
    pal_swap(map, C_RED, d->body);
    pal_swap(map, C_ORANGE, d->mark);
    /* dark bodies get a lighter outline so they read on the track */
    if (d->body == C_NAVY || d->body == C_SLATE || d->body == C_WINE) pal_swap(map, C_INK, C_NIGHT);
    const Sprite *s = &wb_spr[frame];
    if (frame == WA_TRIP) flags ^= SPR_FLIPY;
    spr_draw_ex(s, x, y, flags, map, -1);
    if (!d->pattern) return;
    int w = s->w, h = s->h;
    for (int sy = 0; sy < h; sy++)
        for (int sx = 0; sx < w; sx++) {
            int px = (flags & SPR_FLIPX) ? w - 1 - sx : sx;
            int py = (flags & SPR_FLIPY) ? h - 1 - sy : sy;
            if (s->px[py * w + px] != C_RED || !mark_at(d->pattern, px, py)) continue;
            gfx_pset(x + sx, y + sy, d->mark);
        }
}

void wb_draw_face(int chr, int x, int y, int scale) {
    const Sprite *s = &wb_face[((chr % WB_CHARS) + WB_CHARS) % WB_CHARS];
    if (scale <= 1) spr_draw(s, x, y, 0);
    else spr_draw_scaled(s, x, y, scale, 0);
}
