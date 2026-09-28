/* WET PAINT - original music (UFO-MML) and sound effects, written for UFO 40. */
#include "wetpaint.h"

int WP_MUS_TITLE = -1, WP_MUS_RACE1, WP_MUS_RACE2, WP_MUS_RACE3, WP_MUS_FINAL, WP_MUS_CUT, WP_MUS_END,
    WP_MUS_CLEAR, WP_MUS_MISS, WP_MUS_OVER;

#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define ROOT5(r, f) r "4 " f "4 " r "4 " f "4 "
#define BEAT "o2@13v12c8 o8@9v5c8 o6@11v10c8 o8@9v5c8 "
#define BEAT2 "o2@13v12c8 o2@13v9c8 o6@11v10c8 o8@9v6c8 "

/* "Wet Paint" - the title: a sunny shuffle in C. C G F C | C F-G C-G C */
static const char TITLE_LEAD[] =
    "@1 v12 q6"
    "| o5 e8 g8 o6 c8 o5 g8 e8 g8 o6 c4 | o6 d8 c8 o5 b8 a8 g2"
    "| o5 f8 a8 o6 c8 o5 a8 f8 a8 o6 d4 | o6 c8 o5 b8 a8 b8 o6 c2"
    "| o5 e8 g8 o6 c8 o5 g8 e8 g8 o6 e4 | o6 f8 e8 d8 c8 o5 a4 b4"
    "| o6 c8 o5 g8 e8 g8 f8 d8 o4 b8 o5 d8 | o5 c2 r2";
static const char TITLE_OFF[] =
    "v6 q3 o4 " OFF("@16", "c") OFF("@16", "g") OFF("@16", "f") OFF("@16", "c")
    OFF("@16", "c") OFF("@16", "f") OFF("@16", "c") OFF("@16", "c");
static const char TITLE_BASS[] =
    "@6 v14 q5 | o3 c4 o2 g4 o3 c4 o2 g4 | o2 " ROOT5("g", "d") "| o2 f4 o3 c4 o2 f4 o3 c4 | o3 c4 o2 g4 o3 c4 o2 g4"
    "| o3 c4 o2 g4 o3 c4 o2 g4 | o2 f4 o3 c4 o2 g4 d4 | o3 c4 o2 g4 g4 d4 | o3 c4 o2 g4 o3 c2";
static const char TITLE_DRUMS[] = "[" BEAT "]16";

/* "First Coat" - courses 1 to 10: a bright run in G. G C D G | Em C D D, twice */
static const char RACE1_LEAD[] =
    "@14 v11 q6"
    "| o5 g8 b8 o6 d8 o5 b8 g8 b8 o6 d8 g8 | o6 e8 d8 c8 o5 b8 a4 g4"
    "| o5 f+8 a8 o6 d8 c8 o5 b8 a8 g8 f+8 | o5 g4 b4 o6 d2"
    "| o5 e8 g8 b8 g8 e8 g8 b8 o6 e8 | o6 e8 d8 c8 o5 b8 a8 g8 e4"
    "| o5 f+8 g8 a8 b8 o6 c8 d8 e8 f+8 | o6 g4 f+4 d2"
    "| o6 g8 f+8 e8 d8 o5 b4 o6 d4 | o6 c8 o5 b8 a8 g8 e4 g4"
    "| o5 a8 b8 o6 c8 d8 e8 d8 c8 o5 a8 | o5 b4 g4 d2"
    "| o5 e8 f+8 g8 a8 b8 a8 g8 e8 | o5 c8 d8 e8 g8 o6 c4 o5 e4"
    "| o5 d8 e8 f+8 a8 o6 d8 c8 o5 a8 f+8 | o5 g2 r4 d4";
static const char RACE1_OFF[] =
    "v6 q3 o4 [" OFF("@16", "g") OFF("@16", "c") OFF("@16", "d") OFF("@16", "g")
    OFF("@17", "e") OFF("@16", "c") OFF("@16", "d") OFF("@16", "d") "]2";
static const char RACE1_BASS[] =
    "@6 v14 q5 [o2 " ROOT5("g", "d") "o2 " ROOT5("c", "g") "o2 " ROOT5("d", "a") "o2 " ROOT5("g", "d")
    "o2 " ROOT5("e", "b") "o2 " ROOT5("c", "g") "o2 " ROOT5("d", "a") "o2 d4 a4 d4 f+4 ]2";
static const char RACE1_DRUMS[] = "[" BEAT "]32";

/* "Second Coat" - courses 11 to 20: pushier, in A minor. Am F G E | Am F G Am */
static const char RACE2_LEAD[] =
    "@1 v11 q5"
    "| o5 a8 o6 c8 e8 c8 o5 a8 o6 c8 e4 | o6 f8 e8 d8 c8 o5 a4 f4"
    "| o5 g8 b8 o6 d8 o5 b8 g8 b8 o6 d4 | o5 g+8 b8 o6 e8 d8 o5 b4 g+4"
    "| o5 a8 o6 c8 e8 a8 g8 e8 c8 e8 | o6 f8 a8 o7 c8 o6 a8 f4 d4"
    "| o6 d8 e8 f8 d8 o5 b8 o6 d8 g8 f8 | o6 e4 c4 o5 a2";
static const char RACE2_OFF[] =
    "v6 q3 o4 " OFF("@17", "a") OFF("@16", "f") OFF("@16", "g") OFF("@16", "e")
    OFF("@17", "a") OFF("@16", "f") OFF("@16", "g") OFF("@17", "a");
static const char RACE2_BASS[] =
    "@6 v15 q4 [o2 a8 a8 o3 e8 o2 a8]2 [o2 f8 f8 o3 c8 o2 f8]2 [o2 g8 g8 o3 d8 o2 g8]2 [o2 e8 e8 b8 e8]2"
    "[o2 a8 a8 o3 e8 o2 a8]2 [o2 f8 f8 o3 c8 o2 f8]2 [o2 g8 g8 o3 d8 o2 g8]2 [o2 a8 a8 o3 e8 o2 a8]2";
static const char RACE2_DRUMS[] = "[" BEAT2 "]16";

/* "Top Coat" - courses 21 to 25: the last lap, D minor, driving */
static const char RACE3_LEAD[] =
    "@14 v12 q5"
    "| o5 d8 f8 a8 o6 d8 c8 o5 a8 f8 a8 | o5 b-8 o6 d8 f8 d8 o5 b-4 g4"
    "| o5 c8 e8 g8 o6 c8 o5 b-8 g8 e8 g8 | o5 a8 c+8 e8 a8 g8 e8 c+4"
    "| o6 d8 c8 o5 a8 f8 d8 f8 a4 | o5 b-8 a8 g8 f8 e8 f8 g4"
    "| o5 a8 b-8 a8 g8 f8 e8 d8 c+8 | o5 d4 a4 o6 d4 r4";
static const char RACE3_OFF[] =
    "v6 q3 o4 " OFF("@17", "d") OFF("@16", "b-") OFF("@16", "c") OFF("@16", "a")
    OFF("@17", "d") OFF("@16", "b-") OFF("@16", "a") OFF("@17", "d");
static const char RACE3_BASS[] =
    "@6 v15 q4 [o2 d8 d8 a8 d8]2 [o2 b-8 b-8 o3 f8 o2 b-8]2 [o2 c8 c8 g8 c8]2 [o2 a8 a8 o3 e8 o2 a8]2"
    "[o2 d8 d8 a8 d8]2 [o2 b-8 b-8 o3 f8 o2 b-8]2 [o2 a8 a8 o3 e8 o2 a8]2 [o2 d8 d8 a8 d8]2";
static const char RACE3_DRUMS[] = "[o2@13v12c8 o6@11v8c16 o6@11v6c16 o6@11v11c8 o8@9v6c8 ]16";

/* "Showdown" - the final against Foxy: E minor, two cars, one wall */
static const char FINAL_LEAD[] =
    "@23 v12 q6"
    "| o5 e8 g8 b8 o6 e8 d8 o5 b8 g8 b8 | o5 c8 e8 g8 o6 c8 o5 b8 g8 e4"
    "| o5 d8 f+8 a8 o6 d8 c8 o5 a8 f+8 a8 | o5 b8 o6 d+8 f+8 b8 a8 f+8 d+4"
    "| o6 e4 d8 e8 g4 f+8 e8 | o6 c4 o5 b8 o6 c8 e4 d8 c8"
    "| o5 a8 b8 o6 c8 d8 e8 f+8 g8 a8 | o6 b4 f+4 e2";
static const char FINAL_OFF[] =
    "v6 q3 o4 " OFF("@17", "e") OFF("@16", "c") OFF("@16", "d") OFF("@16", "b")
    OFF("@17", "e") OFF("@16", "c") OFF("@17", "a") OFF("@16", "b");
static const char FINAL_BASS[] =
    "@6 v15 q4 [o2 e8 e8 b8 e8]2 [o2 c8 c8 g8 c8]2 [o2 d8 d8 a8 d8]2 [o2 b8 b8 o3 f+8 o2 b8]2"
    "[o2 e8 e8 b8 e8]2 [o2 c8 c8 g8 c8]2 [o2 a8 a8 o3 e8 o2 a8]2 [o2 b8 b8 o3 f+8 o2 b8]2";
static const char FINAL_DRUMS[] = "[o2@13v13c8 o8@9v6c8 o6@11v11c8 o2@13v10c8 o2@13v12c8 o8@9v6c8 o6@11v11c8 o8@10v7c8 ]8";

/* "Intermission" - the cutscenes: a cheeky walk in F */
static const char CUT_LEAD[] =
    "@3 v10 q4"
    "| o5 f4 a8 o6 c8 r8 o5 a8 f4 | o5 g4 b-8 o6 d8 r8 o5 b-8 g4"
    "| o5 a4 o6 c8 f8 r8 c8 o5 a4 | o5 g8 a8 b-8 g8 f2";
static const char CUT_BASS[] = "@7 v13 q6 | o3 f4 c4 f4 c4 | o3 c4 o2 g4 o3 c4 o2 g4 | o3 f4 c4 d4 a4 | o3 c4 o2 g4 o3 f2";
static const char CUT_DRUMS[] = "[o6@11v6c4 o8@9v4c8 c8]8";

/* "Platinum" - the ending */
static const char END_LEAD[] =
    "@16 v12 q7"
    "| o5 c4 e4 g4 o6 c4 | o5 b4. a8 g2 | o5 a4 o6 c4 f4 a4 | o6 g2. r4"
    "| o6 e4 d4 c4 e4 | o6 f4 e4 d4 f4 | o6 e4 d8 c8 o5 b4 o6 d4 | o6 c2. r4";
static const char END_HARM[] =
    "@22 v7 q6 o4 [g8 e8 c8 e8]2 [g8 d8 o3 b8 o4 d8]2 [a8 f8 c8 f8]2 [g8 e8 c8 e8]2"
    "[g8 e8 c8 e8]2 [a8 f8 d8 f8]2 [g8 f8 d8 f8]2 [g8 e8 c8 e8]2";
static const char END_BASS[] = "@6 v14 q6 | o3 c2 o2 g2 | o2 g2 d2 | o2 f2 o3 c2 | o3 c2 o2 g2 | o3 c2 o2 a2 | o2 d2 f2 | o2 g2 g2 | o3 c2 o2 g2";
static const char END_DRUMS[] = "[o2@13v10c4 o6@11v7c4 o2@13v9c4 o6@11v8c4]8";

/* jingles */
static const char CLEAR_P1[] = "@39 v12 o5 l16 c e g >c8 <g16 >c16 e4";
static const char CLEAR_TRI[] = "@6 v13 o3 l8 c g >c4";
static const char MISS_P1[] = "@5 v11 o5 l8 e d c <b a4. r8";
static const char MISS_TRI[] = "@6 v13 o3 l4 a e <a4";
static const char OVER_P1[] = "@5 v11 o5 l8 c <b- a- g4 r8 g8 c2";
static const char OVER_TRI[] = "@6 v13 o3 l4 c <g c2";

void wp_audio_load(void) {
    if (WP_MUS_TITLE >= 0) return;
    WP_MUS_TITLE = song_define("wp_title", 150, true, TITLE_LEAD, TITLE_OFF, TITLE_BASS, TITLE_DRUMS);
    WP_MUS_RACE1 = song_define("wp_race1", 160, true, RACE1_LEAD, RACE1_OFF, RACE1_BASS, RACE1_DRUMS);
    WP_MUS_RACE2 = song_define("wp_race2", 168, true, RACE2_LEAD, RACE2_OFF, RACE2_BASS, RACE2_DRUMS);
    WP_MUS_RACE3 = song_define("wp_race3", 176, true, RACE3_LEAD, RACE3_OFF, RACE3_BASS, RACE3_DRUMS);
    WP_MUS_FINAL = song_define("wp_final", 172, true, FINAL_LEAD, FINAL_OFF, FINAL_BASS, FINAL_DRUMS);
    WP_MUS_CUT = song_define("wp_cut", 120, true, CUT_LEAD, "", CUT_BASS, CUT_DRUMS);
    WP_MUS_END = song_define("wp_end", 120, true, END_LEAD, END_HARM, END_BASS, END_DRUMS);
    WP_MUS_CLEAR = song_define("wp_clear", 150, false, CLEAR_P1, "", CLEAR_TRI, "");
    WP_MUS_MISS = song_define("wp_miss", 120, false, MISS_P1, "", MISS_TRI, "");
    WP_MUS_OVER = song_define("wp_over", 110, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("wp_kill", CH_NOISE, 220, "@36 v12 o6 c16 @34 v10 o4 c16");
    sfx_define("wp_pop", CH_NOISE, 140, "@34 v15 o3 c4");
    sfx_define("wp_stun", CH_P2, 200, "@37 v12 o5 c8 <g8");
    sfx_define("wp_item", CH_P2, 240, "@39 v11 o5 l32 c e g >c e g");
    sfx_define("wp_boost", CH_P2, 240, "@32 v10 o4 c16 g16");
    sfx_define("wp_bump", CH_P2, 240, "@35 v11 o6 c32 <c32");
    sfx_define("wp_thorn", CH_NOISE, 200, "@40 v12 o5 c8");
    sfx_define("wp_spray", CH_NOISE, 240, "@36 v8 o7 c16");
    sfx_define("wp_lever", CH_P2, 240, "@42 v11 o5 c32 g32");
    sfx_define("wp_tick", CH_P2, 240, "@42 v9 o6 c32");
    sfx_define("wp_flash", CH_P2, 240, "@20 v7 o6 e32");
    sfx_define("wp_life", CH_P1, 200, "@39 v12 o6 l16 c e g >c");
}
