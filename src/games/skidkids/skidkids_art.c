/* SKID KIDS - pixel art (palette-letter strings, see gfx.h). Every kid is
 * built at load time from a shared body pose (the bib in the team's colour,
 * the kid's own shorts, shoes and skin) and the kid's own head. */
#include "skidkids.h"

Sprite skid_spr[SS_SPRITE_COUNT];
Sprite skid_kid_spr[SKID_ROSTER][2][KP_COUNT];
int skid_art_bad;

static const char PAL_CH[] = "kndsglwmvroaycbtehqfjziNBuCIpVPK";
static char pal_ch(int c) { return PAL_CH[iclamp(c, 0, PAL_COUNT - 1)]; }

static void mk(Sprite *s, int w, int h, const char *data) {
    if ((int)strlen(data) != w * h) {
        skid_art_bad++;
        fprintf(stderr, "skidkids art: %dx%d sprite has %d characters\n", w, h, (int)strlen(data));
        char blank[64 * 64];
        memset(blank, '.', sizeof blank);
        blank[w * h] = 0;
        spr_make(s, w, h, blank);
        return;
    }
    spr_make(s, w, h, data);
}

/* ---- bodies: 12 x 12, facing right ------------------------------------
 * r bib, m bib shade, t skin, q shorts, e shoes, w socks */
#define BODY_W 12
#define BODY_H 12
static const char *const BODY[KP_COUNT - 1] = {
    /* idle 0 */
    "...kkkkkk..."
    "..krrrrrrk.."
    ".krrrrrrrrk."
    ".ktrrrrrrtk."
    ".ktmrrrrmtk."
    "..kkmmmmkk.."
    "..kqqqqqqk.."
    "..kqqkkqqk.."
    "..ktk..ktk.."
    "..kwk..kwk.."
    "..keek.keek."
    "...kk...kk..",
    /* idle 1 */
    "...kkkkkk..."
    "..krrrrrrk.."
    ".krrrrrrrrk."
    "ktrrrrrrrrtk"
    "ktkmrrrrmktk"
    "..kkmmmmkk.."
    "..kqqqqqqk.."
    "..kqqkkqqk.."
    "..ktk..ktk.."
    "..kwk..kwk.."
    "..keek.keek."
    "...kk...kk..",
    /* run 0 */
    "...kkkkkk..."
    "..krrrrrrk.."
    ".krrrrrrrrk."
    ".ktrrrrrrtk."
    "..kmrrrrmtk."
    "..kkmmmmkk.."
    "..kqqqqqqk.."
    ".kqqk.kqqk.."
    ".ktk...ktk.."
    "kwk.....kwk."
    "keek....keek"
    ".kk......kk.",
    /* run 1 */
    "...kkkkkk..."
    "..krrrrrrk.."
    ".krrrrrrrrk."
    ".ktrrrrrrtk."
    ".ktmrrrrmk.."
    "..kkmmmmkk.."
    "..kqqqqqqk.."
    "...kqqqqk..."
    "...ktkktk..."
    "...kwkkwk..."
    "...keekeek.."
    "....kk.kk...",
    /* wind-up: the arm back and up */
    "tk.kkkkkk..."
    "tkkrrrrrrk.."
    ".krrrrrrrrk."
    ".kmrrrrrrtk."
    ".kmrrrrrmtk."
    "..kkmmmmkk.."
    "..kqqqqqqk.."
    "..kqqkkqqk.."
    ".ktk...ktk.."
    ".kwk...kwk.."
    ".keek..keek."
    "..kk....kk..",
    /* throw: the arm out in front */
    "...kkkkkk..."
    "..krrrrrrk.."
    ".krrrrrrrrkk"
    ".kmrrrrrrttt"
    ".kmrrrrrmkk."
    "..kkmmmmkk.."
    "..kqqqqqqk.."
    ".kqqk.kqqk.."
    "ktk....ktk.."
    "kwk....kwk.."
    "keek...keek."
    ".kk.....kk..",
    /* pick-up: crouched (the head sits 3 lower) */
    "............"
    "............"
    "............"
    "...kkkkkk..."
    "..krrrrrrk.."
    ".krrrrrrrrk."
    ".kmrrrrrrrk."
    "..kmmmmmmkk."
    "..kqqqqqqtk."
    ".kqqkkkqqtk."
    ".ktk...ktk.."
    "keek...keek.",
    /* jump: knees up */
    "...kkkkkk..."
    "..krrrrrrk.."
    "ktrrrrrrrrtk"
    "ktrrrrrrrrtk"
    ".kmmrrrrmmk."
    "..kkmmmmkk.."
    "..kqqqqqqk.."
    ".kqqqqqqqqk."
    ".ktkkkkkktk."
    ".kwk....kwk."
    ".keek..keek."
    "..kk....kk..",
    /* hurt: knocked back */
    "k..kkkkkk..."
    "tkkrrrrrrk.."
    ".krrrrrrrrk."
    "..krrrrrrrk."
    "..kmrrrrmk.."
    "..kkmmmmkk.."
    "..kqqqqqqk.."
    "..kqqkkqqk.."
    "..ktk..ktk.."
    "..kwk..kwk.."
    ".keek.keek.."
    "..kk...kk...",
    /* win: both arms up */
    "tk.kkkkkk.kt"
    "tkkrrrrrrkkt"
    ".krrrrrrrrk."
    ".kmrrrrrrmk."
    ".kmrrrrrrmk."
    "..kkmmmmkk.."
    "..kqqqqqqk.."
    "..kqqkkqqk.."
    "..ktk..ktk.."
    "..kwk..kwk.."
    "..keek.keek."
    "...kk...kk..",
};

/* flat on the floor, 20 x 8 (V hair) */
#define DOWN_W 20
#define DOWN_H 8
static const char DOWN[] =
    "..kkkk.............."
    ".kVVVVk.kkkkkkkk...."
    "kVVttVVkrrrrrrrrkkkk"
    "kVttkttkrrrrrrrrqqqk"
    "kVttttkkmrrrrrrrqqqk"
    ".kVttk.kmmmmmmmmkwek"
    "..kkk...kkkkkkkkkkek"
    "..................kk";

/* ---- heads: 12 x 8, facing right (t skin) -------------------------------- */
#define HEAD_W 12
#define HEAD_H 8
static const char *const HEAD[SKID_KIDS] = {
    /* NOODLE: curls and freckles */
    "..kkkkkkk..."
    ".kbbkbbkbk.."
    "kbbbbbbbbbk."
    "kbbbbttttttk"
    ".kbbttttkttk"
    ".kbthtthttk."
    "..kttttkkk.."
    "...kkkkk....",
    /* PIPPA: an orange pigtail */
    "...kkkkkk..."
    "..koooooook."
    "kkoooooooook"
    "koooottttttk"
    ".kkoottttktk"
    "kokkotttttk."
    ".k..kttkkk.."
    "....kkkkk...",
    /* HOPS: a lime headband */
    "...kkkkkk..."
    "..knnnnnnk.."
    ".kiiiiiiiik."
    ".knnnttttttk"
    ".knnttttkttk"
    ".kntttttttk."
    "..kttttkkk.."
    "...kkkkk....",
    /* MILO: big glasses */
    "...kkkkkk..."
    "..kaaaaaak.."
    ".kaaaaaaaak."
    ".kaaattttttk"
    ".kaatkkwwkwk"
    ".katttkkttk."
    "..kttttkkk.."
    "...kkkkk....",
    /* SPARKY: spikes and goggles */
    ".k.k.k.k...."
    "kykykykyk..."
    ".kyyyyyyyyk."
    ".kyyoooooook"
    ".kyyttttkttk"
    ".kytttttttk."
    "..kttttkkk.."
    "...kkkkk....",
    /* NELL: a bob and a clip */
    "...kkkkkk..."
    "..kmmmmmPk.."
    ".kmmmmmmmmk."
    ".kmmmttttttk"
    ".kmmttttkttk"
    ".kmmtttttttk"
    ".kmmkttttkk."
    "..kk.kkkk...",
    /* KIKI: two buns */
    ".kk...kk...."
    "knnk.knnk..."
    "..knnnnnnk.."
    ".knnnttttttk"
    ".knnttttkttk"
    ".kntttttttk."
    "..kttttkkk.."
    "...kkkkk....",
    /* ROXIE: a pink ponytail */
    "...kkkkkk..."
    "..kKKKKKKk.."
    ".kKKKKKKKKk."
    "kKKKKttttttk"
    "kKkKttttkttk"
    "kKkkttttttk."
    ".k..kttkkk.."
    "....kkkkk...",
    /* TOBY: a cap on backwards */
    "..kkkkkkk..."
    ".kjjjjjjjk.."
    "kjjjjjjjjjk."
    "kkkbbttttttk"
    "..kbttttkttk"
    "..kttttttttk"
    "..kttttkkk.."
    "...kkkkk....",
    /* SID: a quiff and a smirk */
    "....kkkkkk.."
    "..kksssssssk"
    ".kssssssssk."
    ".ksssttttttk"
    ".ksstttkktk."
    ".kstttttttk."
    "..kttttttkk."
    "...kkkkkk...",
    /* BUZZY: the hornet hood */
    "..k....k...."
    "...kkkkkk..."
    "..kyyyyyyk.."
    ".kkkkkkkkkk."
    ".kyyttttkttk"
    ".kyttttttk.."
    ".kkkttkkk..."
    "..kkkkk.....",
    /* MOOSE: a buzz cut and a big brow */
    "..kkkkkkkk.."
    ".keeeeeeeek."
    "keeeeeeeeeek"
    "keettttttttk"
    "ketttkkktttk"
    "kettttttkttk"
    ".kttttttttk."
    "..kkkkkkkk..",
};

/* ---- the kangaroo: 18 x 22 (r bib) --------------------------------------- */
#define ROO_W 18
#define ROO_H 22
static const char ROO_STAND[] =
    "..........kk.kk..."
    ".........kak.kak.."
    ".........kaakkaak."
    "..........kaaaaak."
    ".........kaaaaakak"
    ".........kaaaaaaak"
    "..........kaaaakkk"
    "..........kaaak..."
    ".........krrrrk..."
    "........krrrrrrk.."
    "........kmrrrrrakk"
    ".......kaaccccaaak"
    ".......kaacttcak.."
    ".......kaacttcak.."
    "......kaaacccak..."
    ".....kaaaaaaaak..."
    "....kaa.kaaaaak..."
    "...kaa..kaaaak...."
    "..kaa..kaaakak...."
    ".kak...kaak.kak..."
    "kak...kaaaakkaaak."
    "kk....kkkkkkkkkkk.";
static const char ROO_HOP[] =
    "...........kk.kk.."
    "..........kak.kak."
    "..........kaakkaak"
    "...........kaaaaak"
    "..........kaaaaaka"
    "..........kaaaaaak"
    "...........kaaaakk"
    "..........krrrrk.."
    ".........krrrrrrk."
    ".........kmrrrrrak"
    "........kaaccccaak"
    "........kaacttcak."
    ".......kaaaccccak."
    "......kaaaaaaaak.."
    ".....kaa.kaaaaak.."
    "....kaa..kaaaaak.."
    "...kaa...kaaaaaak."
    "..kaa.....kaaaaaak"
    ".kak.......kkaaaak"
    "kak..........kkkk."
    "kk................"
    "..................";
static const char ROO_THROW[] =
    "..........kk.kk..."
    ".........kak.kak.."
    ".........kaakkaak."
    "..........kaaaaak."
    ".........kaaaaakak"
    ".........kaaaaaaak"
    "..........kaaaakkk"
    "..........kaaak..."
    ".........krrrrkkkk"
    "........krrrrraaak"
    "........kmrrrrkkk."
    ".......kaaccccak.."
    ".......kaacttcak.."
    ".......kaacttcak.."
    "......kaaacccak..."
    ".....kaaaaaaaak..."
    "....kaa.kaaaaak..."
    "...kaa..kaaaak...."
    "..kaa..kaaakak...."
    ".kak...kaak.kak..."
    "kak...kaaaakkaaak."
    "kk....kkkkkkkkkkk.";
#define ROO_DOWN_W 22
#define ROO_DOWN_H 9
static const char ROO_DOWN[] =
    "...............kk.kk.."
    "..............kaakaak."
    "kk.....kkkkkkkaaaaaak."
    "kak..kkrrrrrrkaaakaak."
    ".kaakkmrrrrrrkaaaaaak."
    "..kaaaaaccccaaakkkkk.."
    "...kaaaacttcaaak......"
    "....kkkaaaaaakkaaak..."
    ".......kkkkkkkkkkkk...";

/* ---- the robot: 16 x 22 (r bib) -------------------------------------------- */
#define BOT_W 16
#define BOT_H 22
static const char BOT_STAND[] =
    ".......kk......."
    ".......kyk......"
    "........k......."
    "....kkkkkkkk...."
    "...kgggggggglk.."
    "...kgkkkkkkkgk.."
    "...kgkrrrrrkgk.."
    "...kgkkkkkkkgk.."
    "...kggggggggk..."
    "..kkkkkkkkkkkk.."
    ".kslkbbbbbbkslk."
    ".kslkttttttkslk."
    ".kslkbbbbbbkslk."
    ".kslkttttttkslk."
    ".kkkkbbbbbbkkkk."
    "..kgkrrrrrrkgk.."
    "..kkkmmmmmmkkk.."
    "....kgk..kgk...."
    "....kgk..kgk...."
    "...kgggk.kgggk.."
    "...ksssk.ksssk.."
    "...kkkkk.kkkkk..";
static const char BOT_WALK[] =
    ".......kk......."
    ".......kyk......"
    "........k......."
    "....kkkkkkkk...."
    "...kgggggggglk.."
    "...kgkkkkkkkgk.."
    "...kgkrrrrrkgk.."
    "...kgkkkkkkkgk.."
    "...kggggggggk..."
    "..kkkkkkkkkkkk.."
    ".kslkbbbbbbkslk."
    ".kslkttttttkslk."
    ".kslkbbbbbbkslk."
    ".kslkttttttkslk."
    ".kkkkbbbbbbkkkk."
    "..kgkrrrrrrkgk.."
    "..kkkmmmmmmkkk.."
    "...kgk....kgk..."
    "..kgk......kgk.."
    ".kgggk....kgggk."
    ".ksssk....ksssk."
    ".kkkkk....kkkkk.";
static const char BOT_THROW[] =
    ".......kk......."
    ".......kyk......"
    "........k......."
    "....kkkkkkkk...."
    "...kgggggggglk.."
    "...kgkkkkkkkgk.."
    "...kgkrrrrrkgk.."
    "...kgkkkkkkkgk.."
    "...kggggggggk..."
    "..kkkkkkkkkkkkkk"
    ".kslkbbbbbbkslsk"
    ".kslkttttttkkkkk"
    ".kslkbbbbbbk...."
    ".kslkttttttk...."
    ".kkkkbbbbbbk...."
    "..kgkrrrrrrk...."
    "..kkkmmmmmmk...."
    "....kgk..kgk...."
    "....kgk..kgk...."
    "...kgggk.kgggk.."
    "...ksssk.ksssk.."
    "...kkkkk.kkkkk..";
#define BOT_DOWN_W 22
#define BOT_DOWN_H 9
static const char BOT_DOWN[] =
    "....kkkkkkkkkkkkk....."
    "...kbtbtbtrrrkgggk...."
    "kk.kbtbtbtrrrkgkrk.kyk"
    "kgkkbtbtbtmmmkgkrkkk.."
    "kgkkbtbtbtmmmkgkrk...."
    "kssk.kkkkkkkkkgggk...."
    "kssk.kgk...kgkkkk....."
    ".kk..kkk...kkk........"
    "......................";

/* ---- the Coach: 14 x 22, facing us --------------------------------------- */
static const char COACH[] =
    "....kkkkkk...."
    "...kNNNNNNk..."
    "..kNNNNNNNNk.."
    "..kkkkkkkkkkk."
    "...kccccccck.."
    "...kckcckcck.."
    "...kccccccck.."
    "...kbbbbbbbk.."
    "....kccccck..."
    "..kkkgggggkkk."
    ".kgggglyllggk."
    ".kgkgglylggkgk"
    ".kgkggggggkgk."
    ".kckggggggkck."
    ".kk.kggggk.kk."
    "....kssssk...."
    "....kssssk...."
    "....ksk.ksk..."
    "....ksk.ksk..."
    "....ksk.ksk..."
    "...kwwk.kwwk.."
    "...kkkk.kkkk..";
static const char COACH_THROW[] =
    "....kkkkkk..kk"
    "...kNNNNNNk.kc"
    "..kNNNNNNNNkkc"
    "..kkkkkkkkkkkc"
    "...kccccccckgk"
    "...kckcckcckgk"
    "...kccccccckgk"
    "...kbbbbbbbkgk"
    "....kccccck.gk"
    "..kkkgggggkkgk"
    ".kgggglyllgggk"
    ".kgkgglylggkk."
    ".kgkggggggk..."
    ".kckggggggk..."
    ".kk.kggggk...."
    "....kssssk...."
    "....kssssk...."
    "....ksk.ksk..."
    "....ksk.ksk..."
    "....ksk.ksk..."
    "...kwwk.kwwk.."
    "...kkkk.kkkk..";
static const char COACH_WHISTLE[] =
    "....kkkkkk...."
    "...kNNNNNNk..."
    "..kNNNNNNNNk.."
    "..kkkkkkkkkkk."
    "...kccccccck.."
    "...kckcckcck.."
    "...kccccccck.."
    "...kbbbyybbk.."
    "....kccyyck..."
    "..kkkggckgkkk."
    ".kggggckkggk.."
    ".kgkggggggkk.."
    ".kgkggggggkgk."
    ".kckggggggkck."
    ".kk.kggggk.kk."
    "....kssssk...."
    "....kssssk...."
    "....ksk.ksk..."
    "....ksk.ksk..."
    "....ksk.ksk..."
    "...kwwk.kwwk.."
    "...kkkk.kkkk..";

/* ---- things ---------------------------------------------------------------- */
static const char BAG[] =
    ".kkkk."
    "kooaok"
    "koaaok"
    "koooak"
    ".kkkk.";
static const char JUICE[] =
    "...w."
    "..w.."
    "kkkkk"
    "kiiik"
    "kyiyk"
    "kiiik"
    "kkkkk";
static const char MARBLE[] =
    ".kkk."
    "kPPIk"
    "kPPPk"
    "kVPPk"
    ".kkk.";
static const char BALLOON[] =
    ".kkkk."
    "kCIICk"
    "kCICCk"
    "kCCCCk"
    ".kCCk."
    "..kk.."
    "...k..";
static const char STAR[] =
    "..y.."
    ".yyy."
    "yyyyy"
    ".yyy."
    ".y.y.";
static const char HALFSTAR[] =
    "..y.."
    ".yyd."
    "yyydd"
    ".yyd."
    ".y.d.";
static const char NOSTAR[] =
    "..d.."
    ".ddd."
    "ddddd"
    ".ddd."
    ".d.d.";
static const char TROPHY[] =
    "..kkkkkkkk.."
    "kkyyyyyyyykk"
    "kykwyyyyyyky"
    "kykwyyyyyyky"
    ".kkyyyyyyakk"
    "...kyyyyak.."
    "....kyyak..."
    ".....kak...."
    ".....kak...."
    "....kaaak..."
    "...kbbbbbk.."
    "...kbyyybk.."
    "...kbbbbbk.."
    "...kkkkkkk..";
static const char NOTE[] =
    "kkkkkkkkkkk."
    "kwwwwwwwwwkk"
    "kwsssssswwwk"
    "kwwwwwwwwwwk"
    "kwssssswwwwk"
    "kwwwwwwwwwwk"
    "kwsssssssswk"
    "kwwwwwwwwwwk"
    "kkkkkkkkkkkk";

/* ---- building the kids ------------------------------------------------------ */

/* Copy a w x h string into a canvas at (dx, dy), mapping letters. */
static void paste(char *cv, int cw, int ch, const char *src, int w, int h, int dx, int dy, const char *from,
                  const char *to) {
    if ((int)strlen(src) != w * h) {
        skid_art_bad++;
        fprintf(stderr, "skidkids art: %dx%d part has %d characters\n", w, h, (int)strlen(src));
        return;
    }
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            char c = src[y * w + x];
            if (c == '.') continue;
            const char *p = strchr(from, c);
            if (p) c = to[p - from];
            int X = x + dx, Y = y + dy;
            if (X >= 0 && X < cw && Y >= 0 && Y < ch) cv[Y * cw + X] = c;
        }
}

static void build_kid(int who, int team) {
    const KidDef *d = &SKID_KID[who];
    char shirt = team == 0 ? 'r' : 'B', shade = team == 0 ? 'm' : 'N';
    char to_body[8] = {shirt, shade, pal_ch(d->skin), pal_ch(d->shorts), pal_ch(d->shoes), 0};
    char to_head[2] = {pal_ch(d->skin), 0};
    char to_down[8] = {shirt, shade, pal_ch(d->skin), pal_ch(d->shorts), pal_ch(d->shoes), pal_ch(d->hair), 0};
    char cv[20 * 20 + 1];
    for (int p = 0; p < KP_COUNT; p++) {
        if (p == KP_DOWN) {
            memset(cv, '.', sizeof cv);
            cv[DOWN_W * DOWN_H] = 0;
            paste(cv, DOWN_W, DOWN_H, DOWN, DOWN_W, DOWN_H, 0, 0, "rmtqeV", to_down);
            spr_make(&skid_kid_spr[who][team][p], DOWN_W, DOWN_H, cv);
            continue;
        }
        int w = 12, h = 20;
        memset(cv, '.', sizeof cv);
        cv[w * h] = 0;
        int hy = p == KP_PICK ? 3 : 0;
        paste(cv, w, h, BODY[p], BODY_W, BODY_H, 0, 8, "rmtqe", to_body);
        paste(cv, w, h, HEAD[who], HEAD_W, HEAD_H, 0, hy, "t", to_head);
        spr_make(&skid_kid_spr[who][team][p], w, h, cv);
    }
}

static void build_boss(int who, int team) {
    char to[3] = {team == 0 ? 'r' : 'B', team == 0 ? 'm' : 'N', 0};
    const char *stand, *move, *thr, *down;
    int w, h, dw, dh;
    if (who == K_BOOMER) {
        stand = ROO_STAND; move = ROO_HOP; thr = ROO_THROW; down = ROO_DOWN;
        w = ROO_W; h = ROO_H; dw = ROO_DOWN_W; dh = ROO_DOWN_H;
    } else {
        stand = BOT_STAND; move = BOT_WALK; thr = BOT_THROW; down = BOT_DOWN;
        w = BOT_W; h = BOT_H; dw = BOT_DOWN_W; dh = BOT_DOWN_H;
    }
    static const int8_t USE[KP_COUNT] = {0, 0, 0, 1, 2, 2, 1, 1, 0, 1, 3};
    char cv[24 * 24 + 1];
    for (int p = 0; p < KP_COUNT; p++) {
        const char *src = USE[p] == 0 ? stand : USE[p] == 1 ? move : USE[p] == 2 ? thr : down;
        int sw = USE[p] == 3 ? dw : w, sh = USE[p] == 3 ? dh : h;
        memset(cv, '.', sizeof cv);
        cv[sw * sh] = 0;
        paste(cv, sw, sh, src, sw, sh, 0, 0, "rm", to);
        spr_make(&skid_kid_spr[who][team][p], sw, sh, cv);
    }
}

void skid_art_load(void) {
    static bool loaded;
    if (loaded) return;
    loaded = true;
    skid_art_bad = 0;
    mk(&skid_spr[SS_COACH], 14, 22, COACH);
    mk(&skid_spr[SS_COACH_THROW], 14, 22, COACH_THROW);
    mk(&skid_spr[SS_COACH_WHISTLE], 14, 22, COACH_WHISTLE);
    mk(&skid_spr[SS_BAG], 6, 5, BAG);
    mk(&skid_spr[SS_JUICE], 5, 7, JUICE);
    mk(&skid_spr[SS_MARBLE], 5, 5, MARBLE);
    mk(&skid_spr[SS_BALLOON], 6, 7, BALLOON);
    mk(&skid_spr[SS_STAR], 5, 5, STAR);
    mk(&skid_spr[SS_HALFSTAR], 5, 5, HALFSTAR);
    mk(&skid_spr[SS_NOSTAR], 5, 5, NOSTAR);
    mk(&skid_spr[SS_TROPHY], 12, 14, TROPHY);
    mk(&skid_spr[SS_NOTE], 12, 9, NOTE);
    for (int t = 0; t < 2; t++) {
        for (int k = 0; k < SKID_KIDS; k++) build_kid(k, t);
        build_boss(K_BOOMER, t);
        build_boss(K_BENCHBOT, t);
    }
}
