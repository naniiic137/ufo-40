/* BANNERFALL - original music (UFO-MML) and sound effects. */
#include "bannerfall.h"

int BF_MUS_TITLE = -1, BF_MUS_BATTLE, BF_MUS_WIN, BF_MUS_LOSE, BF_MUS_CAMPAIGN;

/* "Marigold March" - fife and drum, D major.
 * A: D G D A | D G A D    B: Bm G D A | Bm G A7 D */
static const char MARCH_LEAD[] =
    "@1 v12 q6"
    "| o5 a4 f+8 a8 o6 d4 o5 a4 | o5 b4 g8 b8 o6 d4 o5 b4 | o5 a8 g8 f+8 e8 d4 f+4 | o5 e8 f+8 g8 a8 e2"
    "| o5 a4 f+8 a8 o6 d4 f+4 | o6 g4 f+8 e8 d4 o5 b4 | o5 a8 b8 o6 c+8 d8 e4 c+4 | o6 d2 r4 o5 a4"
    "| o5 b4 o6 d8 c+8 o5 b4 f+4 | o5 g4 b8 a8 g4 d4 | o5 f+4 a8 g8 f+4 d4 | o5 e8 f+8 e8 d8 c+4 e4"
    "| o5 b4 o6 d8 e8 f+4 d4 | o6 g4 f+8 e8 d4 o5 b4 | o5 a8 b8 o6 c+8 e8 g4 c+4 | o6 d4 o5 a8 f+8 d2";
#define PAH(ins, oct, n) "r8 " ins " " oct " " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
static const char MARCH_HARM[] =
    "v6 q4 " PAH("@16", "o4", "d") PAH("@16", "o4", "g") PAH("@16", "o4", "d") PAH("@16", "o4", "a")
    PAH("@16", "o4", "d") PAH("@16", "o4", "g") PAH("@16", "o4", "a") PAH("@16", "o4", "d")
    PAH("@17", "o4", "b") PAH("@16", "o4", "g") PAH("@16", "o4", "d") PAH("@16", "o4", "a")
    PAH("@17", "o4", "b") PAH("@16", "o4", "g") PAH("@16", "o4", "a") PAH("@16", "o4", "d");
#define OOM(r, f) "o2 " r "4 " f "4 " r "4 " f "4 "
static const char MARCH_BASS[] =
    "@6 v15 q5 " OOM("d", "a") OOM("g", "d") OOM("d", "a") OOM("a", "e") OOM("d", "a") OOM("g", "d") OOM("a", "e")
    OOM("d", "a") OOM("b", "f+") OOM("g", "d") OOM("d", "a") OOM("a", "e") OOM("b", "f+") OOM("g", "d")
    OOM("a", "e") OOM("d", "a");
#define ROLL "o2@13v12c8 o6@11v8c16 c16 o6@11v11c8 o6@11v7c8 o2@13v12c8 o6@11v8c16 c16 o6@11v11c8 o8@9v5c8 "
#define ROLLF "o2@13v12c8 o6@11v8c16 c16 o6@11v11c8 o6@11v7c8 o6@11v9c16 c16 o6@11v10c16 c16 o6@11v12c8 o8@10v7c8 "
static const char MARCH_DRUMS[] = "[" ROLL ROLL ROLL ROLLF "]4";

/* "Lanes of Iron" - the battle, E minor, driving.
 * Em C D B | Em C Am B (twice, the second time higher) */
static const char BATTLE_LEAD[] =
    "@14 v12 q6"
    "| o5 e8 e8 g8 e8 b4 a8 g8 | o5 e8 g8 o6 c8 o5 b8 g4 e4 | o5 f+8 a8 o6 d8 c8 o5 a4 f+4 | o5 d+8 f+8 b8 a8 f+4 d+4"
    "| o5 e8 g8 b8 o6 e8 d4 o5 b4 | o6 c8 o5 b8 a8 g8 e4 g4 | o5 a8 o6 c8 e8 d8 c4 o5 a4 | o5 b4 d+4 f+4 b4"
    "| o6 e4 d8 e8 g4 f+8 e8 | o6 e8 d8 c8 o5 b8 g4 a4 | o5 a8 b8 o6 c8 d8 f+4 d4 | o6 d+4 c8 o5 b8 a8 f+8 d+4"
    "| o5 e8 g8 b8 o6 e8 g4 e4 | o6 g8 f+8 e8 d8 c4 e4 | o6 c8 o5 b8 a8 b8 o6 c4 e4 | o5 b8 o6 d+8 f+8 b8 ^2";
#define GAL(ins, n) ins " o4 " n "8 " n "16 " n "16 r8 " n "8 " n "8 " n "16 " n "16 r8 " n "8 "
static const char BATTLE_HARM[] =
    "v7 q3 [" GAL("@17", "e") GAL("@16", "c") GAL("@16", "d") GAL("@16", "b")
    GAL("@17", "e") GAL("@16", "c") GAL("@17", "a") GAL("@16", "b") "]2";
#define B8(n) "o2 " n "8 " n "8 o3 " n "8 o2 " n "8 " n "8 " n "8 o3 " n "8 o2 " n "8 "
static const char BATTLE_BASS[] =
    "@6 v15 q5 [" B8("e") B8("c") B8("d") B8("b") B8("e") B8("c") B8("a") B8("b") "]2";
#define DRV "o2@13v13c8 o8@9v6c8 o6@11v12c8 o8@9v6c8 o2@13v13c8 o2@13v11c8 o6@11v12c8 o8@9v6c8 "
#define DRVF "o2@13v13c8 o8@9v6c8 o6@11v12c8 o8@9v6c8 o6@11v10c16 c16 o6@11v12c16 c16 o6@11v13c8 o8@10v9c8 "
static const char BATTLE_DRUMS[] = "[" DRV DRV DRV DRVF "]4";

/* "The War Table" - the campaign map, B minor, slow.
 * Bm G D A | Bm G Em F# */
static const char TABLE_LEAD[] =
    "@5 v11 q7 ["
    "| o5 f+4. e8 d4 c+4 | o5 d4 o4 b4 g2 | o5 a4. g8 f+4 e4 | o5 e2. r4"
    "| o5 f+4 b4 a4 f+4 | o5 g4. f+8 e4 d4 | o5 e4 g4 f+4 e4 | o5 c+2 o4 a+4 r4 ]2";
static const char TABLE_ARP[] =
    "@2 v6 q5 l8 ["
    "o3 b o4 d f+ d o3 b o4 d f+ d | o3 g b o4 d o3 b g b o4 d o3 b | o4 d f+ a f+ d f+ a f+ | o3 a o4 c+ e c+ o3 a o4 c+ e c+"
    "| o3 b o4 d f+ d o3 b o4 d f+ d | o3 g b o4 d o3 b g b o4 d o3 b | o3 e g b g e g b g | o3 f+ a+ o4 c+ o3 a+ f+ a+ o4 c+ o3 a+ ]2";
static const char TABLE_BASS[] =
    "@6 v13 q7 [o2 b2 f+2 | o2 g2 d2 | o2 d2 a2 | o2 a2 e2 | o2 b2 f+2 | o2 g2 d2 | o2 e2 b2 | o2 f+2 c+2 ]2";
static const char TABLE_DRUMS[] = "[o8@9v4c4 o6@11v5c4 o8@9v4c8 c8 o6@11v5c4]16";

/* jingles */
static const char WIN_P1[] = "@16 v12 o5 l8 d f+ a o6 d4. o5 a8 o6 d2";
static const char WIN_P2[] = "@22 v8 o5 l8 a o6 d f+ a4. f+8 a2";
static const char WIN_TRI[] = "@6 v14 o2 l4 d a o3 d o2 a8 o3 d8 d2";
static const char WIN_NOISE[] = "o6@11v10c16 c16 c16 c16 o6@11v12c4 r4 o5@12v9c2";
static const char LOSE_P1[] = "@5 v11 o4 l8 b a g f+4. e8 d+2";
static const char LOSE_TRI[] = "@6 v13 o2 l4 e d c o1 b2";

void bf_audio_load(void) {
    if (BF_MUS_TITLE >= 0) return;
    BF_MUS_TITLE = song_define("bf_march", 126, true, MARCH_LEAD, MARCH_HARM, MARCH_BASS, MARCH_DRUMS);
    BF_MUS_BATTLE = song_define("bf_battle", 150, true, BATTLE_LEAD, BATTLE_HARM, BATTLE_BASS, BATTLE_DRUMS);
    BF_MUS_CAMPAIGN = song_define("bf_table", 92, true, TABLE_LEAD, TABLE_ARP, TABLE_BASS, TABLE_DRUMS);
    BF_MUS_WIN = song_define("bf_win", 140, false, WIN_P1, WIN_P2, WIN_TRI, WIN_NOISE);
    BF_MUS_LOSE = song_define("bf_lose", 96, false, LOSE_P1, "", LOSE_TRI, "");

    sfx_define("bf_move", CH_P2, 200, "@42 v8 o6 c32");
    sfx_define("bf_grab", CH_P2, 220, "@20 v10 o5 e32 g32");
    sfx_define("bf_drop", CH_P2, 220, "@20 v9 o5 g32 c32");
    sfx_define("bf_drag", CH_P2, 220, "@42 v9 o5 a32");
    sfx_define("bf_bump", CH_P2, 200, "@37 v9 o3 c16");
    sfx_define("bf_tick", CH_P2, 220, "@15 v8 o6 a32");
    sfx_define("bf_go", CH_NOISE, 200, "@11 v8 o6 c32 c32 v10 c32 c32 v12 c16");
    sfx_define("bf_march", CH_NOISE, 220, "@13 v10 o2 c16 @9 v5 o8 c16");
    sfx_define("bf_clang", CH_NOISE, 220, "@21 v12 o7 c16");
    sfx_define("bf_arrow", CH_NOISE, 240, "@36 v9 o7 c16");
    sfx_define("bf_knife", CH_NOISE, 240, "@36 v8 o8 c32");
    sfx_define("bf_hit", CH_P2, 240, "@37 v10 o4 c32");
    sfx_define("bf_die", CH_NOISE, 200, "@21 v11 o5 c16 @13 v11 o3 c16");
    sfx_define("bf_blast", CH_NOISE, 140, "@34 v15 o3 c4");
    sfx_define("bf_keep", CH_P1, 170, "@33 v12 o5 c8 o4 g8 c4");
    sfx_define("bf_promote", CH_P2, 220, "@39 v11 o5 c16 e16 g16");
    sfx_define("bf_spawn", CH_TRI, 220, "@38 v10 o4 c16");
    sfx_define("bf_hero", CH_P1, 180, "@16 v12 o5 c8 e8 g8 o6 c4");
    sfx_define("bf_double", CH_P1, 200, "@15 v12 o6 c16 r16 c16 r16 e8");
}
