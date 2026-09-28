/* RIMSHIRE - original music (UFO-MML) and sound effects, written for
 * UFO 40. Every looping tune is eight bars of 4/4 on every channel. */
#include "rimshire.h"

int RSH_MUS_TITLE = -1, RSH_MUS_MAP, RSH_MUS_BATTLE, RSH_MUS_BATTLE2, RSH_MUS_EMPRESS, RSH_MUS_END,
    RSH_MUS_WIN, RSH_MUS_LOSE, RSH_MUS_WAR_WON, RSH_MUS_WAR_LOST;

/* an off-beat chord bar: rest, chord, four times */
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
/* four quarter notes, root and fifth */
#define Q4(a, b) a "4 " b "4 " a "4 " b "4 "
/* a bar of driving eighths: root, root, fifth, root, twice */
#define DRIVE(r, f) r "8 " r "8 " f "8 " r "8 " r "8 " r "8 " f "8 " r "8 "

/* "Rimshire" - the title: a proud little march in D. D G A D | Bm G A D */
static const char TITLE_LEAD[] =
    "@1 v12 q6"
    "| o5 d4 f+8 a8 o6 d4 o5 a4 | o5 b4 g8 b8 o6 d4 o5 b4 | o5 a4 e8 a8 o6 c+4 o5 a4 | o5 f+8 e8 d8 e8 f+2"
    "| o5 b4 f+8 b8 o6 d4 c+4 | o5 b8 a8 g8 a8 b2 | o5 a8 b8 o6 c+8 d8 e4 c+4 | o6 d2. r4";
static const char TITLE_OFF[] =
    "v6 q3 o4 " OFF("@16", "d") OFF("@16", "g") OFF("@16", "a") OFF("@16", "d")
    OFF("@17", "b") OFF("@16", "g") OFF("@16", "a") OFF("@16", "d");
static const char TITLE_BASS[] =
    "@6 v14 q5 " Q4("o3 d", "o3 a") Q4("o2 g", "o3 d") Q4("o2 a", "o3 e") Q4("o3 d", "o3 a")
    Q4("o2 b", "o3 f+") Q4("o2 g", "o3 d") Q4("o2 a", "o3 e") "o3 d4 o3 a4 o3 d2";
#define MARCH "o2@13v12c8 o8@9v5c8 o4@11v9c8 o8@9v5c8 o2@13v12c8 o2@13v8c8 o4@11v9c8 o8@9v5c8 "
static const char TITLE_DRUMS[] = "[" MARCH "]8";

/* "The Roads of Rimshire" - the board: an easy walk in G. G Em C D | G Em Am D */
static const char MAP_LEAD[] =
    "@14 v10 q5"
    "| o5 g4 b8 a8 g4 d4 | o5 e4 g8 f+8 e4 b4 | o5 c8 e8 g8 e8 c4 g4 | o5 d8 f+8 a8 f+8 d2"
    "| o5 b4 o6 d8 c8 o5 b4 g4 | o5 e8 f+8 g8 a8 b4 g4 | o5 a8 b8 o6 c8 o5 a8 e4 c4 | o5 d4 f+4 a4 d4";
static const char MAP_OFF[] =
    "v5 q3 o4 " OFF("@16", "g") OFF("@17", "e") OFF("@16", "c") OFF("@16", "d")
    OFF("@16", "g") OFF("@17", "e") OFF("@17", "a") OFF("@16", "d");
static const char MAP_BASS[] =
    "@7 v13 q4 " Q4("o2 g", "o3 d") Q4("o2 e", "o2 b") Q4("o2 c", "o2 g") Q4("o2 d", "o2 a")
    Q4("o2 g", "o3 d") Q4("o2 e", "o2 b") Q4("o2 a", "o3 e") Q4("o2 d", "o2 a");
static const char MAP_DRUMS[] = "[o3@13v8c4 o8@9v4c8 o8@9v3c8 o4@11v5c4 o8@9v4c8 o8@9v3c8]8";

/* "Flick and Fling" - battles, wars 1-5: A minor, quick. Am F G E | Am F Dm E */
static const char B1_LEAD[] =
    "@1 v11 q5"
    "| o5 a8 o6 c8 e8 a8 g8 e8 c8 e8 | o6 f8 e8 d8 c8 o5 a4 f4 | o5 g8 a8 b8 o6 d8 c8 o5 b8 g8 b8 | o5 g+4 b4 o6 e2"
    "| o6 a8 g8 e8 c8 d8 e8 c8 o5 a8 | o6 c8 o5 a8 f8 a8 o6 c4 f4 | o6 d8 c8 o5 a8 f8 d8 f8 a8 o6 d8 | o5 b4 g+4 e2";
static const char B1_OFF[] =
    "v6 q3 o4 " OFF("@17", "a") OFF("@16", "f") OFF("@16", "g") OFF("@16", "e")
    OFF("@17", "a") OFF("@16", "f") OFF("@17", "d") OFF("@16", "e");
static const char B1_BASS[] =
    "@6 v15 q4 " DRIVE("o2 a", "o3 e") DRIVE("o2 f", "o3 c") DRIVE("o2 g", "o3 d") DRIVE("o2 e", "o2 b")
    DRIVE("o2 a", "o3 e") DRIVE("o2 f", "o3 c") DRIVE("o2 d", "o2 a") DRIVE("o2 e", "o2 b");
static const char B1_DRUMS[] = "[o2@13v13c8 o8@9v6c8 o4@11v11c8 o8@9v6c8]16";

/* "Bank Shot" - battles, wars 6-9 and the streak: E minor. Em C D B | Em C Am B */
static const char B2_LEAD[] =
    "@23 v12 q5"
    "| o5 e8 g8 b8 o6 e8 d8 o5 b8 g8 b8 | o5 c8 e8 g8 o6 c8 o5 b8 g8 e8 g8 | o5 d8 f+8 a8 o6 d8 c8 o5 a8 f+8 a8 | o5 d+4 f+4 b2"
    "| o6 e4 d8 o5 b8 g4 b4 | o6 c4 o5 b8 g8 e4 g4 | o5 a8 b8 o6 c8 d8 e8 d8 c8 o5 a8 | o5 b2 d+4 f+4";
static const char B2_OFF[] =
    "v6 q3 o4 " OFF("@17", "e") OFF("@16", "c") OFF("@16", "d") OFF("@16", "b")
    OFF("@17", "e") OFF("@16", "c") OFF("@17", "a") OFF("@16", "b");
static const char B2_BASS[] =
    "@6 v15 q4 " DRIVE("o2 e", "o2 b") DRIVE("o2 c", "o2 g") DRIVE("o2 d", "o2 a") DRIVE("o2 b", "o3 f+")
    DRIVE("o2 e", "o2 b") DRIVE("o2 c", "o2 g") DRIVE("o2 a", "o3 e") DRIVE("o2 b", "o3 f+");
static const char B2_DRUMS[] = "[o2@13v13c8 o8@9v6c16 o8@9v4c16 o4@11v11c8 o8@9v6c8 o2@13v12c8 o2@13v9c8 o4@11v11c8 o8@10v6c8]8";

/* "The Plum Empress" - the last war's battles: C minor, heavy. Cm Ab Bb G | Cm Fm G G */
static const char EMP_LEAD[] =
    "@23 v12 q7"
    "| o5 c4 e-4 g4 o6 c4 | o6 c8 o5 b-8 a-4 e-4 c4 | o5 d4 f4 b-4 o6 d4 | o5 b4 g4 d4 g4"
    "| o6 e-8 d8 c8 o5 b-8 a-8 g8 f8 e-8 | o5 f4 a-4 o6 c4 f4 | o6 d8 e-8 f8 d8 o5 b4 g4 | o5 g2 b4 d4";
static const char EMP_OFF[] =
    "v6 q3 o4 " OFF("@17", "c") OFF("@16", "a-") OFF("@16", "b-") OFF("@16", "g")
    OFF("@17", "c") OFF("@17", "f") OFF("@16", "g") OFF("@16", "g");
static const char EMP_BASS[] =
    "@6 v15 q5 " DRIVE("o2 c", "o2 g") DRIVE("o2 a-", "o3 e-") DRIVE("o2 b-", "o3 f") DRIVE("o2 g", "o3 d")
    DRIVE("o2 c", "o2 g") DRIVE("o2 f", "o3 c") DRIVE("o2 g", "o3 d") DRIVE("o2 g", "o3 d");
static const char EMP_DRUMS[] = "[o2@13v14c8 o2@13v10c8 o4@11v12c8 o8@9v6c8 o2@13v14c8 o8@9v6c8 o4@11v12c8 o4@11v8c8]8";

/* "Brass at Peace" - the ending: F major, slow. F C Bb C | F Dm Bb F */
static const char END_LEAD[] =
    "@14 v11 q7"
    "| o5 f4 a4 o6 c4 f4 | o6 e4. d8 c2 | o5 b-4 o6 d4 f4 d4 | o6 c2. r4"
    "| o5 a4 o6 c4 f4 a4 | o6 a4 f4 d4 f4 | o6 d4 c4 o5 b-4 g4 | o5 f2. r4";
static const char END_HARM[] =
    "@22 v7 q6 o4 [a8 f8 c8 f8]2 [g8 e8 c8 e8]2 [b-8 f8 d8 f8]2 [g8 e8 c8 e8]2"
    "[a8 f8 c8 f8]2 [a8 f8 d8 f8]2 [b-8 f8 d8 f8]2 [a8 f8 c8 f8]2";
static const char END_BASS[] =
    "@6 v14 q6 o2 f2 o3 c2 o2 c2 o2 g2 o2 b-2 o3 f2 o2 c2 o2 g2 o2 f2 o3 c2 o2 d2 o2 a2 o2 b-2 o3 c2 o2 f2 o3 c2";
static const char END_DRUMS[] = "[o3@13v9c4 o4@11v6c4 o3@13v8c4 o4@11v7c4]8";

/* jingles */
static const char WIN_P1[] = "@1 v12 o5 l16 a o6 c+ e a8 r16 e16 a4";
static const char WIN_P2[] = "@5 v8 o5 l16 e a o6 c+ e8 r16 c+16 e4";
static const char WIN_TRI[] = "@6 v13 o3 l8 a e a4";
static const char LOSE_P1[] = "@5 v11 o5 l8 e d c o4 b4 a4";
static const char LOSE_TRI[] = "@6 v13 o3 l4 e d o2 a2";
static const char WWON_P1[] = "@23 v12 o5 l8 d f+ a o6 d4 o5 a8 o6 d8 f+2 e8 f+8 d2";
static const char WWON_P2[] = "@5 v8 o5 l8 f+ a o6 d f+4 d8 f+8 a2 g8 a8 f+2";
static const char WWON_TRI[] = "@6 v13 o3 l4 d a d a o2 a2 o3 d2";
static const char WWON_NOISE[] = "@12 v9 o5 c2 r2 r2 @12 v10 c2";
static const char WLOST_P1[] = "@5 v11 o5 l8 a g f e d4 c+4 o4 a2";
static const char WLOST_TRI[] = "@6 v13 o3 l4 d o2 a b- a d1";

void rsh_audio_load(void) {
    if (RSH_MUS_TITLE >= 0) return;
    RSH_MUS_TITLE = song_define("rsh_title", 120, true, TITLE_LEAD, TITLE_OFF, TITLE_BASS, TITLE_DRUMS);
    RSH_MUS_MAP = song_define("rsh_map", 108, true, MAP_LEAD, MAP_OFF, MAP_BASS, MAP_DRUMS);
    RSH_MUS_BATTLE = song_define("rsh_battle", 150, true, B1_LEAD, B1_OFF, B1_BASS, B1_DRUMS);
    RSH_MUS_BATTLE2 = song_define("rsh_battle2", 158, true, B2_LEAD, B2_OFF, B2_BASS, B2_DRUMS);
    RSH_MUS_EMPRESS = song_define("rsh_empress", 132, true, EMP_LEAD, EMP_OFF, EMP_BASS, EMP_DRUMS);
    RSH_MUS_END = song_define("rsh_end", 96, true, END_LEAD, END_HARM, END_BASS, END_DRUMS);
    RSH_MUS_WIN = song_define("rsh_win", 150, false, WIN_P1, WIN_P2, WIN_TRI, "");
    RSH_MUS_LOSE = song_define("rsh_lose", 110, false, LOSE_P1, "", LOSE_TRI, "");
    RSH_MUS_WAR_WON = song_define("rsh_warwon", 132, false, WWON_P1, WWON_P2, WWON_TRI, WWON_NOISE);
    RSH_MUS_WAR_LOST = song_define("rsh_warlost", 90, false, WLOST_P1, "", WLOST_TRI, "");

    sfx_define("rsh_flick", CH_NOISE, 240, "@36 v10 o6 c16");
    sfx_define("rsh_shoot", CH_P2, 240, "@32 v10 o5 c16 g16");
    sfx_define("rsh_hit", CH_NOISE, 240, "@21 v12 o5 c16 @9 v8 o6 c32");
    sfx_define("rsh_knock", CH_P2, 240, "@20 v9 o5 c32");
    sfx_define("rsh_wall", CH_P2, 240, "@42 v5 o4 c32");
    sfx_define("rsh_kill", CH_NOISE, 160, "@34 v12 o5 c8");
    sfx_define("rsh_splash", CH_NOISE, 160, "@10 v11 o7 c16 @10 v8 o6 c8");
    sfx_define("rsh_pick", CH_P2, 240, "@35 v10 o6 c16 g16");
    sfx_define("rsh_heal", CH_P2, 240, "@39 v9 o5 l32 c e g >c");
    sfx_define("rsh_star", CH_P2, 240, "@39 v10 o6 l32 e g b >e");
    sfx_define("rsh_ember", CH_NOISE, 200, "@36 v9 o5 c16");
    sfx_define("rsh_poison", CH_P2, 200, "@33 v9 o5 c16 <g16");
    sfx_define("rsh_stun", CH_P2, 240, "@2 v9 o6 l32 c <c >c <c");
    sfx_define("rsh_step", CH_P2, 240, "@42 v7 o5 c32");
    sfx_define("rsh_buy", CH_P2, 240, "@35 v11 o5 c16 g16 >c16");
    sfx_define("rsh_nope", CH_P2, 200, "@37 v10 o3 c16 r32 c16");
    sfx_define("rsh_pip", CH_P2, 240, "@42 v6 o6 c32");
    sfx_define("rsh_fog", CH_NOISE, 120, "@12 v7 o3 c4");
    sfx_define("rsh_clash", CH_P1, 150, "@23 v12 o4 d8 a8 >d4");
    sfx_define("rsh_turn", CH_P2, 240, "@20 v7 o6 c32 g32");
}
