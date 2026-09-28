/* RIMSHIRE - pixel art (palette-letter strings, see gfx.h), all drawn for
 * UFO 40. The disks themselves are drawn round at run time in their
 * banner's colours; each wears an emblem of its kind. */
#include "rimshire.h"

Sprite rsh_spr[RS_COUNT];
int rsh_art_bad;

/* rim, face, dark: Brass and Plum */
const uint8_t RSH_SIDE_COL[2][3] = {{C_AMBER, C_CREAM, C_BROWN}, {C_VIOLET, C_PINK, C_PURPLE}};

/* ---- emblems: 7x7 small, 9x9 mid, 13x13 large ------------------------------- */
static const char E_SQUIRE[] =
    "....k...."
    "...kwk..."
    "...kwk..."
    "...kwk..."
    "...kwk..."
    ".kkkakkk."
    "...kak..."
    "...kbk..."
    "....k....";
static const char E_WARDEN[] =
    ".kkkkkkk."
    "kwwwrwwwk"
    "kwwwrwwwk"
    "krrrrrrrk"
    "kwwwrwwwk"
    "kwwwrwwwk"
    ".kwwrwwk."
    "..kwrwk.."
    "...kkk...";
static const char E_FERRET[] =
    "k.....k"
    "kk...kk"
    "khhhhhk"
    "hkhhhkh"
    "hhhhhhh"
    ".hhkhh."
    "..hhh..";
static const char E_BRUTE[] =
    ".........kk.."
    "........keek."
    ".......keeeek"
    "......keeeek."
    ".....keeeek.."
    "....keeeek..."
    "...keeeek...."
    "..kbeek......"
    ".kbbk........"
    "kbbk........."
    "kbk.........."
    ".k..........."
    ".............";
static const char E_SLINGER[] =
    "k.......k"
    "bk.....kb"
    ".bk...kb."
    "..bk.kb.."
    "...bkb..."
    "....b...."
    "....b...."
    "...kbk..."
    "...kkk...";
static const char E_TOADKIN[] =
    ".kk...kk."
    "kwkk.kkwk"
    "kkjjjjjkk"
    "kjjjjjjjk"
    "kjkjjjkjk"
    "kjjkkkjjk"
    ".kjjjjjk."
    "..kkkkk.."
    ".........";
static const char E_OOZE[] =
    "..kkk.."
    ".kiiik."
    "kiwiiik"
    "kiiiiik"
    "kikiiki"
    "kiiiiik"
    "kkkkkkk";
static const char E_HEXER[] =
    "....k...."
    "...kpk..."
    "...kpk..."
    "..kpypk.."
    "..kpppk.."
    ".kpppppk."
    "kkkkkkkkk"
    ".kppppk.."
    ".........";
static const char E_MENHIR[] =
    "..kkkkk.."
    ".kgggggk."
    ".kglgggk."
    ".kgggsgk."
    ".kgsgggk."
    ".kggglgk."
    ".kgggggk."
    "kkkkkkkkk"
    ".........";
static const char E_ADDER[] =
    ".kkkk.."
    "kjjjjk."
    "kjkkk.."
    ".kjjjk."
    "..kkkjk"
    ".kjjjjk"
    "..kkkk.";
static const char E_FRIAR[] =
    "...kkk..."
    "...kwk..."
    "...kwk..."
    "..kwwwk.."
    ".kwrrrwk."
    "kwrrwrrwk"
    "kwrwwwrwk"
    ".kwrwrwk."
    "..kkkkk..";
static const char E_PIPER[] =
    "....kkkkk"
    "....kyyyk"
    "....kk.kk"
    "....k...k"
    "....k...k"
    "..kkk.kkk"
    ".kyyykyyk"
    ".kyyykkk."
    "..kkk....";
static const char E_LEECH[] =
    "k.......k"
    "kk.....kk"
    "kvk...kvk"
    "kvvkkkvvk"
    "kvvvvvvvk"
    "kvwkvkwvk"
    ".kvvvvvk."
    "..kwkwk.."
    "...k.k...";
static const char E_DELVER[] =
    ".kkkkkkk."
    "kgggggggk"
    "kgk.b.kgk"
    ".k..b..k."
    "....b...."
    "....b...."
    "....b...."
    "....b...."
    "....k....";
static const char E_WYRM[] =
    "..k.......k.."
    ".krk.....krk."
    ".krrk...krrk."
    "..krrkkkrrk.."
    ".krrrrrrrrrk."
    "krrywrrrwyrrk"
    "krrrrrrrrrrrk"
    "krrrrrrrrrrrk"
    ".krrkkkkkrrk."
    ".krkwkwkwkrk."
    "..krrrrrrrk.."
    "...kkkkkkk..."
    ".............";
static const char E_EMPRESS[] =
    "........."
    "k...k...k"
    "kk.kyk.kk"
    "kykkyykyk"
    "kyyyyyyyk"
    "kyryyyryk"
    "kyyyyyyyk"
    "kkkkkkkkk"
    ".........";

/* ---- the board ---------------------------------------------------------------- */
static const char TOKEN[] =
    ".kkkkk..."
    "krrrrrk.."
    "krrrrrrk."
    "krrwwrrk."
    "krrrrrrk."
    "krrrrk..."
    "kbk......"
    "kbk......"
    "kbk......"
    "kbk......"
    "kbk......"
    "kkk......";
static const char BASE[] =
    "....kkk...."
    "...krrrk..."
    "..krrrrrk.."
    ".krrrkrrrk."
    "krrrkkkrrrk"
    "krrk...krrk"
    "kkkkkkkkkkk";
static const char CASTLE[] =
    "k.k.k.k.k.k"
    "krkrkrkrkrk"
    "krrrrrrrrrk"
    "krgrrrrrgrk"
    "krrrrrrrrrk"
    "krrrkkkrrrk"
    "krrk...krrk"
    "kkkkkkkkkkk";
static const char INN[] =
    "....kkk...."
    "..kkeeekk.."
    ".keeeeeeek."
    "kkkkkkkkkkk"
    ".kcckckcck."
    ".kcckyykck."
    ".kckbbkyk.."
    ".kckbbkck.."
    ".kkkkkkkk..";
static const char TOME[] =
    ".kkkkkkk."
    "kvvvvvvvk"
    "kvyyyyvvk"
    "kvvvvvvvk"
    "kvyyyvvvk"
    "kvvvvvvvk"
    "kcccccccl"
    ".kkkkkkk.";
static const char SEAM[] =
    "...kkk..."
    "..kgggk.."
    ".kgyggsk."
    "kgggsygk."
    "kgsgggggk"
    "kggyggsgk"
    ".kkkkkkk.";
static const char CHEST[] =
    ".kkkkkkk."
    "kbbbbbbbk"
    "kkkkykkkk"
    "kbbbybbbk"
    "kbbbbbbbk"
    ".kkkkkkk.";

/* ---- the field ----------------------------------------------------------------- */
static const char COIN[] =
    ".kkkkk."
    "kyyyyak"
    "kyakyak"
    "kyakyak"
    "kyyyyak"
    ".kkkkk.";
static const char SHARD[] =
    "...k..."
    "..kCk.."
    ".kCICk."
    "kCIwICk"
    ".kCICk."
    "..kCk.."
    "...k...";
static const char TONIC[] =
    "..kkk.."
    "..kbk.."
    ".kwwwk."
    "kwrrrwk"
    "krrwrrk"
    "krrrrrk"
    ".kkkkk.";
static const char WELL[] =
    "....kkkkkkk...."
    "..kkgggggggkk.."
    ".kgggggggggggk."
    ".kgkkkkkkkkkgk."
    "kgkuuuuuuuuukgk"
    "kgkuCuuuuuCukgk"
    "kgkuuuuCuuuukgk"
    "kgkuuuuuuuuukgk"
    "kgkuuCuuuuuukgk"
    "kgkuuuuuuCuukgk"
    ".kgkuuuuuuukgk."
    ".kggkkkkkkkggk."
    "..kkgggggggkk.."
    "....kkkkkkk...."
    "...............";
static const char PILE[] =
    "..............."
    "......kkk......"
    ".....kyyak....."
    "....kyakyak...."
    "...kkyyyyakk..."
    "..kyyakkkyyak.."
    "..kyakyyakyak.."
    ".kkyyyakyyyakk."
    "kyyakkyyakkyyak"
    "kyakyyakyyakyak"
    "kyyyyakyyyyakak"
    ".kkkkkkkkkkkkk."
    "..............."
    "..............."
    "...............";
static const char CLUSTER[] =
    "..............."
    "......k........"
    ".....kCk..k...."
    ".....kIk.kCk..."
    "..k..kCk.kIk..."
    ".kCk.kIkkkCk..."
    ".kIkkkCkkCIk..."
    ".kCkkCIkkIwk..."
    "..kkkIwCkkCk..."
    "...kkCICkkk...."
    "..kgggggggggk.."
    ".kgsgggsgggsgk."
    ".kkkkkkkkkkkkk."
    "..............."
    "...............";
static const char EMBER0[] =
    "...k..."
    "..kok.."
    ".koyok."
    ".kyyyok"
    "koywyok"
    ".kooyk."
    "..kkk..";
static const char EMBER1[] =
    "..k...."
    ".kok..."
    ".koyok."
    "koyyyk."
    "koywyok"
    ".kyooK."
    "..kkk..";
static const char STAR[] =
    "..y.."
    ".yyy."
    "yyyyy"
    ".yyy."
    ".y.y.";
static const char SKULL[] =
    ".kkkkk."
    "klllllk"
    "klkllkl"
    "kllllll"
    ".kllkl."
    ".klklk."
    "..kkk..";
static const char BOLT[] =
    "..yy."
    ".yy.."
    "yyyy."
    "..yy."
    ".yy.."
    ".y..."
    "y....";
static const char DROP[] =
    "..i.."
    ".iii."
    ".iii."
    "iiiii"
    "iiwii"
    "iiiii"
    ".iii.";

static const char BAG[] =
    "..kkk.."
    ".kbkbk."
    "..kbk.."
    ".ktttk."
    "kttyttk"
    "ktyyytk"
    "kttyttk"
    ".kkkkk.";

/* ---- the two lords --------------------------------------------------------------- */
static const char LORD0[] =
    "....kkkkkkkk...."
    "...kaaaaaaaak..."
    "..kaayaaaayaak.."
    "..kaaaaaaaaaak.."
    "..kkkkkkkkkkkk.."
    "..khhhhhhhhhhk.."
    "..khkkhhhhkkhk.."
    "..khhhhhhhhhhk.."
    "..khhhhbbhhhhk.."
    "..kbbbbbbbbbbk.."
    "...kbhhhhhhbk..."
    "....kkhhhhkk...."
    "...kaakkkkaak..."
    "..kaaayaayaaak.."
    ".kaaaaayyaaaaak."
    ".kkkkkkkkkkkkkk.";
static const char LORD1[] =
    "..k..k..k..k...."
    "..kkkykkykkk...."
    "..kyyyyyyyyk...."
    "..kyryyyyyryk..."
    "..kkkkkkkkkkk..."
    ".kvvhhhhhhhvvk.."
    ".kvhkkhhhkkhvk.."
    ".kvhhhhhhhhhvk.."
    ".kvhhhhrrhhhvk.."
    ".kvvhhhhhhhvvk.."
    ".kvvvkhhhkvvvk.."
    ".kvvvvkkkvvvvk.."
    "..kpppKKKpppk..."
    ".kppppKKKppppk.."
    ".kpppppKpppppk.."
    ".kkkkkkkkkkkkk..";

static void mk(int id, int w, int h, const char *data) {
    if ((int)strlen(data) != w * h) { rsh_art_bad++; return; }
    spr_make(&rsh_spr[id], w, h, data);
}

void rsh_art_load(void) {
    if (rsh_spr[RS_TOKEN0].px) return;
    rsh_art_bad = 0;
    static const char *const EMB[K_COUNT] = {E_SQUIRE, E_WARDEN, E_FERRET, E_BRUTE, E_SLINGER, E_TOADKIN, E_OOZE, E_HEXER,
                                             E_MENHIR, E_ADDER, E_FRIAR, E_PIPER, E_LEECH, E_DELVER, E_WYRM, E_EMPRESS};
    for (int k = 0; k < K_COUNT; k++) {
        int s = RSH_KIND[k].size == SZ_SMALL ? 7 : RSH_KIND[k].size == SZ_LARGE ? 13 : 9;
        mk(RS_EMB0 + k, s, s, EMB[k]);
    }
    mk(RS_TOKEN0, 9, 12, TOKEN);
    mk(RS_TOKEN1, 9, 12, TOKEN);
    mk(RS_BASE, 11, 7, BASE);
    mk(RS_CASTLE, 11, 8, CASTLE);
    mk(RS_INN, 11, 9, INN);
    mk(RS_TOME, 9, 8, TOME);
    mk(RS_SEAM, 9, 7, SEAM);
    mk(RS_CHEST, 9, 6, CHEST);
    mk(RS_COIN, 7, 6, COIN);
    mk(RS_SHARD, 7, 7, SHARD);
    mk(RS_TONIC, 7, 7, TONIC);
    mk(RS_WELL, 15, 15, WELL);
    mk(RS_PILE, 15, 15, PILE);
    mk(RS_CLUSTER, 15, 15, CLUSTER);
    mk(RS_EMBER0, 7, 7, EMBER0);
    mk(RS_EMBER1, 7, 7, EMBER1);
    mk(RS_STAR, 5, 5, STAR);
    mk(RS_SKULL, 7, 7, SKULL);
    mk(RS_BOLT, 5, 7, BOLT);
    mk(RS_DROP, 5, 7, DROP);
    mk(RS_BAG, 7, 8, BAG);
    mk(RS_LORD0, 16, 16, LORD0);
    mk(RS_LORD1, 16, 16, LORD1);
}

static const float RAD[3] = {6.0f, 8.0f, 11.0f};

/* a disk: its rim and face in the banner's colours, its emblem, and
 * what is wrong with it (poison, a stun) or right (stars) */
void rsh_draw_disk(const RshDisk *d, int x, int y, int t) {
    const uint8_t *c = RSH_SIDE_COL[d->side];
    if (d->proj) {
        gfx_circ(x, y, 3, C_INK);
        gfx_circ(x, y, 2, c[0]);
        gfx_pset(x - 1, y - 1, C_WHITE);
        return;
    }
    int r = (int)RAD[RSH_KIND[d->kind].size];
    gfx_circ(x + 1, y + 2, r, C_INK); /* shadow */
    gfx_circ(x, y, r, C_INK);
    gfx_circ(x, y, r - 1, d->poison ? C_LIME : c[0]);
    gfx_circ(x, y, r - 2, c[1]);
    gfx_circb(x, y, r - 2, c[2]);
    const Sprite *e = &rsh_spr[RS_EMB0 + d->kind];
    if (e->px) spr_draw(e, x - e->w / 2, y - e->h / 2, d->side ? SPR_FLIPX : 0);
    if (d->stun && (t / 4) % 2) spr_draw(&rsh_spr[RS_BOLT], x + r - 3, y - r - 3, 0);
    for (int s = 0; s < d->stars && s < 5; s++) spr_draw(&rsh_spr[RS_STAR], x - r + s * 5, y - r - 6, 0);
    char hp[8];
    snprintf(hp, sizeof hp, "%d", d->hp);
    int w = tiny_width(hp);
    gfx_rect(x - w / 2 - 1, y + r - 2, w + 2, 7, C_INK);
    tiny_draw(hp, x - w / 2, y + r - 1, d->poison ? C_LIME : d->hp > RSH_KIND[d->kind].hp ? C_CYAN : C_WHITE);
}

/* a disk as a small icon (queues, inns, the army row) */
void rsh_draw_disk_icon(int kind, int side, int cx, int cy, int t) {
    RshDisk d;
    memset(&d, 0, sizeof d);
    d.kind = (uint8_t)kind;
    d.side = (uint8_t)side;
    const uint8_t *c = RSH_SIDE_COL[side];
    int r = (int)RAD[RSH_KIND[kind].size];
    (void)t;
    gfx_circ(cx, cy, r, C_INK);
    gfx_circ(cx, cy, r - 1, c[0]);
    gfx_circ(cx, cy, r - 2, c[1]);
    const Sprite *e = &rsh_spr[RS_EMB0 + kind];
    if (e->px) spr_draw(e, cx - e->w / 2, cy - e->h / 2, side ? SPR_FLIPX : 0);
}
