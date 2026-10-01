/* CHIME CIRCUIT - original music (UFO-MML) and sound effects, written for
 * UFO 40. Every looping tune is eight bars of 4/4 on each channel. */
#include "chime.h"

int CHM_MUS_TITLE = -1, CHM_MUS_PIT, CHM_MUS_RACE1, CHM_MUS_RACE2, CHM_MUS_RACE3, CHM_MUS_FINALE, CHM_MUS_CUP,
    CHM_MUS_HOME, CHM_MUS_FLAG, CHM_MUS_ALSO;

#define BEAT "o2@13v12c8 o8@9v5c8 o6@11v10c8 o8@9v5c8 "

/* "Chime Circuit" - the title, rung out on bells in D.
 * D G A D | Bm G A D */
static const char TITLE_LEAD[] =
    "@15 v12 q6"
    "| o5 d8 f+8 a8 o6 d8 o5 a8 f+8 d4 | o5 g8 b8 o6 d8 g8 f+8 d8 o5 b4"
    "| o5 a8 o6 c+8 e8 a8 g8 e8 c+4 | o5 d4 f+4 a2"
    "| o5 b8 o6 d8 f+8 b8 a8 f+8 d4 | o5 g8 b8 o6 d8 g8 f+8 e8 d4"
    "| o5 e8 a8 o6 c+8 e8 d8 c+8 o5 a4 | o6 d2 o5 a4 f+4";
static const char TITLE_ARP[] =
    "v6 q3 @16 o4 [r8 d8]4 [r8 g8]4 [r8 a8]4 [r8 d8]4 @17 [r8 b8]4 @16 [r8 g8]4 [r8 a8]4 [r8 d8]4";
static const char TITLE_BASS[] =
    "@6 v14 q5 [o2 d4 a4]2 [o2 g4 o3 d4]2 [o2 a4 o3 e4]2 [o2 d4 a4]2"
    " [o2 b4 o3 f+4]2 [o2 g4 o3 d4]2 [o2 a4 o3 e4]2 o2 d4 a4 d4 r4";
static const char TITLE_DRUMS[] = "[" BEAT "]16";

/* "Pit Lane" - the hangar, the cards and the results: an easy stroll in F.
 * F Dm Gm C | F Bb C F */
static const char PIT_LEAD[] =
    "@23 v11 q6"
    "| o5 c4 a8 g8 f4 c4 | o5 d4 f8 a8 o6 d4 c4 | o5 b-4 a8 g8 d4 g4 | o5 c4 e8 g8 o6 c2"
    "| o5 a4 o6 c8 d8 c4 o5 a4 | o5 b-4 o6 d8 f8 d4 o5 b-4 | o5 g8 a8 b-8 o6 c8 d4 e4 | o6 f2 c4 r4";
static const char PIT_COMP[] =
    "v6 q2 @16 o4 [r4 f4]2 @17 [r4 d4]2 [r4 g4]2 @16 [r4 c4]2 [r4 f4]2 o3 [r4 b-4]2 o4 [r4 c4]2 [r4 f4]2";
static const char PIT_BASS[] =
    "@6 v14 q6 o2 f4 a4 o3 c4 o2 a4 | o2 d4 f4 a4 f4 | o2 g4 b-4 o3 d4 o2 b-4 | o2 c4 e4 g4 e4"
    "| o2 f4 a4 o3 c4 o2 a4 | o2 b-4 o3 d4 f4 d4 | o2 c4 e4 g4 o3 c4 | o2 f4 o3 c4 o2 f4 r4";
static const char PIT_DRUMS[] =
    "[o2@13v10c4 o8@9v5c8. o8@9v4c16 o6@11v8c4 o8@9v5c8. o8@9v4c16 ]8";

/* "Full Thrust" - a race in A: the tutorial loop, the first figure eight and
 * the crooked mile. A F#m D E | A F#m D-E A */
static const char R1_LEAD[] =
    "@1 v12 q5"
    "| o5 a8 o6 c+8 e8 a8 g+8 e8 c+8 e8 | o6 f+8 e8 c+8 o5 a8 f+8 a8 o6 c+8 f+8"
    "| o6 d8 c+8 o5 a8 f+8 d8 f+8 a8 o6 d8 | o5 b8 o6 d8 e8 g+8 b4 e4"
    "| o6 a8 g+8 e8 c+8 o5 a8 o6 c+8 e8 a8 | o6 f+8 a8 f+8 c+8 o5 a8 o6 c+8 f+8 a8"
    "| o6 d8 f+8 a8 f+8 e8 g+8 b8 g+8 | o6 a4 e4 a4 r4";
static const char R1_COMP[] =
    "v6 q2 @16 o4 [a16 a16 a8]4 @17 [f+16 f+16 f+8]4 @16 [d16 d16 d8]4 [e16 e16 e8]4"
    " [a16 a16 a8]4 @17 [f+16 f+16 f+8]4 @16 [d16 d16 d8]2 [e16 e16 e8]2 [a16 a16 a8]4";
static const char R1_BASS[] =
    "@6 v15 q4 [o2 a8 o3 a8]4 [o2 f+8 o3 f+8]4 [o2 d8 o3 d8]4 [o2 e8 o3 e8]4"
    " [o2 a8 o3 a8]4 [o2 f+8 o3 f+8]4 [o2 d8 o3 d8]2 [o2 e8 o3 e8]2 [o2 a8 o3 a8]4";
static const char R1_DRUMS[] =
    "[o2@13v12c8 o8@9v6c16 o8@9v6c16 o6@11v10c8 o8@9v6c16 o8@9v6c16 ]16";

/* "Updraft" - a race in D minor: the dip, the fork.
 * Dm Bb C A | Dm Bb Gm-A Dm */
static const char R2_LEAD[] =
    "@14 v12 q6"
    "| o5 d4 f8 a8 o6 d4 c8 o5 a8 | o5 b-4 o6 d8 f8 d4 o5 b-8 f8 | o5 g4 c8 e8 g4 o6 c8 o5 g8 | o5 a2 c+4 e4"
    "| o5 d8 e8 f8 g8 a4 o6 d4 | o6 d8 c8 o5 b-8 a8 b-4 f4 | o5 g8 a8 b-8 o6 c8 d4 c+4 | o6 d2 o5 a4 r4";
static const char R2_COMP[] =
    "v6 q3 @17 o4 [r8 d8]4 @16 o3 [r8 b-8]4 o4 [r8 c8]4 [r8 a8]4 @17 [r8 d8]4 @16 o3 [r8 b-8]4"
    " @17 o4 [r8 g8]2 @16 [r8 a8]2 @17 [r8 d8]4";
static const char R2_BASS[] =
    "@6 v15 q5 [o2 d8 a8 o3 d8 o2 a8]2 [o2 b-8 o3 f8 b-8 f8]2 [o3 c8 g8 o4 c8 o3 g8]2 [o2 a8 o3 e8 a8 e8]2"
    " [o2 d8 a8 o3 d8 o2 a8]2 [o2 b-8 o3 f8 b-8 f8]2 o2 g8 o3 d8 g8 d8 o2 a8 o3 e8 a8 e8 [o2 d8 a8 o3 d8 o2 a8]2";
static const char R2_DRUMS[] = "[o2@13v12c8 o8@9v6c8 o6@11v10c8 o8@9v6c16 o2@13v9c16 ]16";

/* "Pinball Pack" - a race in C that bounces: the needle, the flue.
 * C Am F G | C Am F-G C */
static const char R3_LEAD[] =
    "@3 v12 q6"
    "| o5 c8 e8 r8 g8 r8 o6 c8 o5 g8 e8 | o5 a8 o6 c8 r8 e8 r8 a8 e8 c8"
    "| o5 f8 a8 r8 o6 c8 r8 f8 c8 o5 a8 | o5 g8 b8 r8 o6 d8 r8 g8 f8 d8"
    "| o6 e8 d8 c8 o5 g8 a8 g8 e8 g8 | o5 a8 g8 e8 c8 d8 e8 a4"
    "| o5 f8 a8 o6 c8 f8 d8 o5 b8 g8 b8 | o6 c4 o5 g4 o6 c4 r4";
static const char R3_COMP[] =
    "v6 q2 @16 o4 [c8 c16 c16]4 @17 [a8 a16 a16]4 @16 [f8 f16 f16]4 [g8 g16 g16]4"
    " [c8 c16 c16]4 @17 [a8 a16 a16]4 @16 [f8 f16 f16]2 [g8 g16 g16]2 [c8 c16 c16]4";
static const char R3_BASS[] =
    "@6 v15 q4 [o2 c8 c8 o3 c8 o2 c8]2 [o2 a8 a8 o3 a8 o2 a8]2 [o2 f8 f8 o3 f8 o2 f8]2 [o2 g8 g8 o3 g8 o2 g8]2"
    " [o2 c8 c8 o3 c8 o2 c8]2 [o2 a8 a8 o3 a8 o2 a8]2 o2 f8 f8 o3 f8 o2 f8 g8 g8 o3 g8 o2 g8"
    " [o2 c8 c8 o3 c8 o2 c8]2";
static const char R3_DRUMS[] = "[o2@13v12c8 o6@11v10c8 o8@9v6c8 o6@11v9c8 ]16";

/* "Grand Octave" - the last race, in E minor.
 * Em C D B | Em C Am-B Em */
static const char FIN_LEAD[] =
    "@23 v13 q6"
    "| o5 e4 g8 b8 o6 e4 d8 o5 b8 | o5 c4 e8 g8 o6 c4 o5 b8 g8 | o5 d4 f+8 a8 o6 d4 c8 o5 a8 | o5 b2 d+4 f+4"
    "| o5 e8 f+8 g8 a8 b4 o6 e4 | o6 e8 d8 c8 o5 b8 a4 g4 | o5 a8 b8 o6 c8 d8 e4 d+4 | o6 e2 o5 b4 r4";
static const char FIN_COMP[] =
    "v6 q3 @17 o4 [r8 e8]4 @16 [r8 c8]4 [r8 d8]4 o3 [r8 b8]4 @17 o4 [r8 e8]4 @16 [r8 c8]4"
    " @17 [r8 a8]2 @16 o3 [r8 b8]2 @17 o4 [r8 e8]4";
static const char FIN_BASS[] =
    "@6 v15 q4 [o2 e8 b8]4 [o2 c8 g8]4 [o2 d8 a8]4 [o2 b8 o3 f+8]4 [o2 e8 b8]4 [o2 c8 g8]4"
    " [o2 a8 o3 e8]2 [o2 b8 o3 f+8]2 [o2 e8 b8]4";
static const char FIN_DRUMS[] = "[o2@13v12c8 o8@9v6c16 o8@9v6c16 o6@11v11c8 o2@13v9c8 ]16";

/* "The Cup" - the champion's fanfare in B flat.
 * Bb Eb F Bb | Gm Eb F Bb */
static const char CUP_LEAD[] =
    "@23 v13 q7"
    "| o5 f4 b-4 o6 d4 f4 | o6 e-4. d8 c4 o5 b-4 | o5 a4 o6 c4 f4 e-4 | o6 d2 o5 b-2"
    "| o5 g4 b-4 o6 d4 g4 | o6 g4. f8 e-4 c4 | o6 c4 d4 e-4 c4 | o5 b-2 r2";
static const char CUP_COMP[] =
    "v6 q4 @16 o4 [b-8 b-8]4 [e-8 e-8]4 [f8 f8]4 [b-8 b-8]4 @17 [g8 g8]4 @16 [e-8 e-8]4 [f8 f8]4 [b-8 b-8]4";
static const char CUP_BASS[] =
    "@6 v14 q6 o2 b-4 o3 f4 o2 b-4 o3 f4 | o2 e-4 b-4 e-4 b-4 | o2 f4 o3 c4 o2 f4 o3 c4 | o2 b-4 o3 f4 d4 o2 b-4"
    "| o2 g4 o3 d4 o2 g4 o3 d4 | o2 e-4 b-4 e-4 b-4 | o2 f4 o3 c4 o2 f4 a4 | o2 b-4 o3 f4 o2 b-2";
static const char CUP_DRUMS[] = "[o2@13v12c4 o6@11v10c8 o6@11v8c8 o2@13v11c4 o6@11v10c4 ]8";

/* "Homeward" - the endings and the credits, in G.
 * G Em C D | G Em Am-D G */
static const char HOME_LEAD[] =
    "@5 v11 q7"
    "| o5 d4 g4 b4 a8 g8 | o5 e4 g4 b2 | o5 c4 e4 g4 f+8 e8 | o5 d2. r4"
    "| o5 b4 o6 d4 c4 o5 b4 | o5 g4 b4 e2 | o5 a4 o6 c4 o5 f+4 a4 | o5 g2. r4";
static const char HOME_COMP[] =
    "v5 q4 @22 o4 [g8 b8 o5 d8 o4 b8]2 [e8 g8 b8 g8]2 [c8 e8 g8 e8]2 [d8 f+8 a8 f+8]2"
    " [g8 b8 o5 d8 o4 b8]2 [e8 g8 b8 g8]2 c8 e8 a8 e8 d8 f+8 a8 f+8 [g8 b8 o5 d8 o4 b8]2";
static const char HOME_BASS[] =
    "@6 v13 q7 o2 g2 d2 | o2 e2 b2 | o2 c2 g2 | o2 d2 a2 | o2 g2 d2 | o2 e2 b2 | o2 a2 d2 | o2 g1";
static const char HOME_DRUMS[] = "[o8@9v4c8 o8@9v3c8 o6@11v5c8 o8@9v3c8 ]16";

/* "Chequered Flag" and "Also Ran" - a race won, and one that wasn't */
static const char FLAG_P1[] = "@39 v12 o5 g16 b16 o6 d16 g16 d16 g16 b8 o7 d4 r8";
static const char FLAG_P2[] = "@2 v8 o4 g16 b16 o5 d16 g16 d16 g16 b8 o6 d4 r8";
static const char FLAG_TRI[] = "@6 v14 o3 g8 d8 g8 b8 g4";
static const char ALSO_P1[] = "@5 v11 o5 e8 d8 c8 o4 b8 a4 g+2";
static const char ALSO_TRI[] = "@6 v13 o3 a4 e4 a2";

void chm_audio_load(void) {
    if (CHM_MUS_TITLE >= 0) return;
    CHM_MUS_TITLE = song_define("chm_title", 150, true, TITLE_LEAD, TITLE_ARP, TITLE_BASS, TITLE_DRUMS);
    CHM_MUS_PIT = song_define("chm_pit", 112, true, PIT_LEAD, PIT_COMP, PIT_BASS, PIT_DRUMS);
    CHM_MUS_RACE1 = song_define("chm_race1", 176, true, R1_LEAD, R1_COMP, R1_BASS, R1_DRUMS);
    CHM_MUS_RACE2 = song_define("chm_race2", 168, true, R2_LEAD, R2_COMP, R2_BASS, R2_DRUMS);
    CHM_MUS_RACE3 = song_define("chm_race3", 180, true, R3_LEAD, R3_COMP, R3_BASS, R3_DRUMS);
    CHM_MUS_FINALE = song_define("chm_finale", 172, true, FIN_LEAD, FIN_COMP, FIN_BASS, FIN_DRUMS);
    CHM_MUS_CUP = song_define("chm_cup", 120, true, CUP_LEAD, CUP_COMP, CUP_BASS, CUP_DRUMS);
    CHM_MUS_HOME = song_define("chm_home", 100, true, HOME_LEAD, HOME_COMP, HOME_BASS, HOME_DRUMS);
    CHM_MUS_FLAG = song_define("chm_flag", 150, false, FLAG_P1, FLAG_P2, FLAG_TRI, "");
    CHM_MUS_ALSO = song_define("chm_also", 110, false, ALSO_P1, "", ALSO_TRI, "");

    sfx_define("chm_beep", CH_P2, 200, "@42 v11 o5 a8");
    sfx_define("chm_go", CH_P2, 200, "@20 v12 o6 a4");
    sfx_define("chm_wall", CH_NOISE, 200, "@13 v13 o4 c16 @36 v8 o5 c16");
    sfx_define("chm_slash", CH_NOISE, 220, "@36 v11 o6 c16");
    sfx_define("chm_knock", CH_P2, 200, "@37 v12 o4 g16 c16");
    sfx_define("chm_wreck", CH_NOISE, 140, "@34 v15 o3 c4");
    sfx_define("chm_launch", CH_P2, 220, "@32 v11 o4 c16 g16 >c16");
    sfx_define("chm_pick", CH_P2, 220, "@39 v12 o6 c16 e16 g16");
    sfx_define("chm_warn", CH_P2, 200, "@35 v9 o6 e16 r16 e16");
    sfx_define("chm_boost", CH_P2, 240, "@32 v10 o5 c32 g32 >c32");
    sfx_define("chm_shot", CH_NOISE, 240, "@21 v7 o6 c32");
    sfx_define("chm_blast", CH_NOISE, 160, "@34 v13 o4 c8");
    sfx_define("chm_lap", CH_P2, 200, "@35 v11 o6 c16 g16");
    sfx_define("chm_last", CH_P2, 200, "@15 v12 o6 c16 c16 g8");
    sfx_define("chm_bump", CH_P2, 220, "@42 v8 o3 c32");
    sfx_define("chm_fire", CH_NOISE, 200, "@36 v10 o4 c16 @34 v8 o4 c16");
    sfx_define("chm_move", CH_P2, 220, "@42 v9 o6 c32");
}
