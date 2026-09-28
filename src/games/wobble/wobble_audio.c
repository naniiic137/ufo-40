/* WOBBLE DERBY - original music (UFO-MML) and sound effects, written for UFO 40. */
#include "wobble.h"

int WB_MUS_TITLE = -1, WB_MUS_PADDOCK, WB_MUS_RACE, WB_MUS_NEWS, WB_MUS_FINAL, WB_MUS_WIN, WB_MUS_LOSE, WB_MUS_BELL;

#define BEAT "o2@13v12c8 o8@9v5c8 o6@11v10c8 o8@9v5c8 "

/* "Crater Downs" - the title: a bouncy two-step in F. F Bb C F | Dm Gm C F */
static const char TITLE_LEAD[] =
    "@1 v12 q6"
    "| o5 c8 f8 a8 f8 o6 c4 o5 a4 | o5 b-8 o6 d8 f8 d8 o5 b-4 f4"
    "| o5 g8 o6 c8 e8 c8 o5 g8 e8 c4 | o5 f8 a8 o6 c8 f8 e8 c8 o5 a4"
    "| o5 d8 f8 a8 o6 d8 c8 o5 a8 f4 | o5 g8 b-8 o6 d8 g8 f8 d8 o5 b-4"
    "| o6 c8 o5 b-8 a8 g8 e8 g8 b-8 o6 c8 | o6 f4 c4 o5 f4 r4";
static const char TITLE_OFF[] =
    "v6 q3 @16 o4 [r8 f8]4 [r8 b-8]4 o5 [r8 c8]4 o4 [r8 f8]4"
    " @17 o4 [r8 d8]4 [r8 g8]4 @16 o5 [r8 c8]4 o4 [r8 f8]4";
static const char TITLE_BASS[] =
    "@6 v14 q5 [o2 f4 o3 c4]2 [o2 b-4 o3 f4]2 [o3 c4 g4]2 [o2 f4 o3 c4]2"
    " [o2 d4 a4]2 [o2 g4 o3 d4]2 [o3 c4 o2 g4]2 o2 f4 o3 c4 o2 f4 r4";
static const char TITLE_DRUMS[] = "[" BEAT "]16";

/* "The Tote Board" - the paddock: a shady swing in D minor.
 * Dm Gm A7 Dm | Bb Gm A Dm */
static const char PAD_LEAD[] =
    "@23 v11 q6"
    "| o5 d8. f16 a8. f16 e8. d16 c+4 | o5 d8. g16 b-8. a16 g4 d4"
    "| o5 c+8. e16 g8. f16 e8. c+16 o4 a4 | o5 d4 f8. e16 d2"
    "| o5 f8. b-16 o6 d8. c16 o5 b-4 f4 | o5 g8. b-16 a8. g16 f8. e16 d4"
    "| o5 e8. g16 c+8. e16 a4 g4 | o5 f8. e16 d8. c+16 d2";
static const char PAD_COMP[] =
    "v6 q2 @17 o4 [r4 d4]2 [r4 g4]2 @16 [r4 a4]2 @17 [r4 d4]2"
    " @16 o3 [r4 b-4]2 @17 o4 [r4 g4]2 @16 [r4 a4]2 @17 [r4 d4]2";
static const char PAD_BASS[] =
    "@6 v14 q6 o2 d4 f4 a4 f4 | o2 g4 b-4 o3 d4 o2 b-4 | o2 a4 o3 c+4 e4 c+4 | o2 d4 a4 f4 d4"
    "| o2 b-4 o3 d4 f4 d4 | o2 g4 b-4 o3 d4 o2 b-4 | o2 a4 o3 c+4 e4 o2 a4 | o2 d4 f4 a4 d4";
static const char PAD_DRUMS[] =
    "[o2@13v10c4 o8@9v5c8. o8@9v4c16 o6@11v8c4 o8@9v5c8. o8@9v4c16 ]8";

/* "Post Time" - the race: a gallop in G. G C D G | Em C D G */
static const char RACE_LEAD[] =
    "@14 v12 q5"
    "| o5 g8 b8 o6 d8 g8 f+8 d8 o5 b8 g8 | o5 e8 g8 o6 c8 e8 d8 c8 o5 g8 e8"
    "| o5 f+8 a8 o6 d8 f+8 e8 d8 o5 a8 f+8 | o5 g4 b4 o6 d4 g4"
    "| o5 e8 g8 b8 o6 e8 d8 o5 b8 g8 e8 | o5 c8 e8 g8 o6 c8 o5 b8 g8 e8 c8"
    "| o5 d8 f+8 a8 o6 d8 c8 o5 a8 f+8 a8 | o5 g8 a8 b8 o6 d8 g4 r4";
static const char RACE_COMP[] =
    "v6 q2 @16 o4 [g8 g16 g16]4 [c8 c16 c16]4 [d8 d16 d16]4 [g8 g16 g16]4"
    " @17 [e8 e16 e16]4 @16 [c8 c16 c16]4 [d8 d16 d16]4 [g8 g16 g16]4";
static const char RACE_BASS[] =
    "@6 v15 q4 [o2 g8 o3 d8]4 [o2 c8 g8]4 [o2 d8 a8]4 [o2 g8 o3 d8]4"
    " [o2 e8 b8]4 [o2 c8 g8]4 [o2 d8 a8]4 [o2 g8 o3 d8]4";
static const char RACE_DRUMS[] =
    "[o2@13v12c8 o8@9v6c16 o8@9v6c16 o6@11v10c8 o8@9v6c16 o8@9v6c16 ]16";

/* "The Big Payout" - the final standings, in C. C Am F G | C Am Dm-G C */
static const char FIN_LEAD[] =
    "@23 v12 q6"
    "| o5 e4 g4 o6 c4 o5 g4 | o5 a4 o6 c4 e4 c4 | o6 f4 e4 d4 c4 | o5 b4 a4 g2"
    "| o5 e8 f8 g8 a8 g4 e4 | o5 a8 b8 o6 c8 d8 e4 c4 | o6 d4 f4 e4 d4 | o6 c2 r2";
static const char FIN_COMP[] =
    "v6 q3 @16 o4 [r8 c8]4 @17 o3 [r8 a8]4 @16 o4 [r8 f8]4 [r8 g8]4"
    " [r8 c8]4 @17 o3 [r8 a8]4 o4 [r8 d8]2 @16 [r8 g8]2 [r8 c8]4";
static const char FIN_BASS[] =
    "@6 v14 q5 o2 c4 g4 c4 g4 | o2 a4 o3 e4 o2 a4 o3 e4 | o2 f4 o3 c4 o2 f4 o3 c4 | o2 g4 o3 d4 o2 g4 o3 d4"
    "| o2 c4 g4 c4 g4 | o2 a4 o3 e4 o2 a4 o3 e4 | o2 d4 a4 g4 d4 | o2 c4 g4 c2";
static const char FIN_DRUMS[] = "[" BEAT "]16";

/* "Wobble Wire" - the news sting */
static const char NEWS_P1[] = "@15 v12 o5 c16 e16 g16 o6 c16 o5 g16 o6 c16 e16 g16 e4 c4 | o5 g8 o6 c8 e8 g8 c2";
static const char NEWS_P2[] = "@2 v8 o4 c16 e16 g16 o5 c16 o4 g16 o5 c16 e16 g16 c4 o4 g4 | o4 e8 g8 o5 c8 e8 o4 g2";
static const char NEWS_TRI[] = "@7 v14 o2 c4 g4 c4 g4 | o2 c4 g4 o3 c2";

/* "Photo Finish" and "Torn Ticket" - a winning and a losing slip */
static const char WIN_P1[] = "@39 v12 o5 c16 e16 g16 o6 c16 o5 g16 o6 c16 e8 g4 c2";
static const char WIN_TRI[] = "@6 v14 o3 c8 g8 c8 e8 c2";
static const char LOSE_P1[] = "@5 v11 o5 g8 f+8 f8 e8 d+4 d2";
static const char LOSE_TRI[] = "@6 v13 o2 g4 f+4 f4 e2";

/* "Off They Go" - the starter's bugle */
static const char BELL_P1[] = "@23 v12 o5 g16 r16 o6 c16 r16 e16 r16 g8 e16 r16 g4 ^8";
static const char BELL_TRI[] = "@6 v12 o3 c8 r8 c8 r8 c4 r4";

void wb_audio_load(void) {
    if (WB_MUS_TITLE >= 0) return;
    WB_MUS_TITLE = song_define("wb_title", 150, true, TITLE_LEAD, TITLE_OFF, TITLE_BASS, TITLE_DRUMS);
    WB_MUS_PADDOCK = song_define("wb_paddock", 112, true, PAD_LEAD, PAD_COMP, PAD_BASS, PAD_DRUMS);
    WB_MUS_RACE = song_define("wb_race", 184, true, RACE_LEAD, RACE_COMP, RACE_BASS, RACE_DRUMS);
    WB_MUS_FINAL = song_define("wb_final", 120, true, FIN_LEAD, FIN_COMP, FIN_BASS, FIN_DRUMS);
    WB_MUS_NEWS = song_define("wb_news", 132, false, NEWS_P1, NEWS_P2, NEWS_TRI, "");
    WB_MUS_WIN = song_define("wb_win", 150, false, WIN_P1, "", WIN_TRI, "");
    WB_MUS_LOSE = song_define("wb_lose", 110, false, LOSE_P1, "", LOSE_TRI, "");
    WB_MUS_BELL = song_define("wb_bell", 140, false, BELL_P1, "", BELL_TRI, "");

    sfx_define("wb_move", CH_P2, 220, "@42 v9 o6 c32");
    sfx_define("wb_coin", CH_P2, 240, "@35 v12 o6 e16 b16");
    sfx_define("wb_trip", CH_P2, 200, "@37 v12 o4 c8 <g8");
    sfx_define("wb_boom", CH_NOISE, 140, "@34 v15 o3 c4");
    sfx_define("wb_gun", CH_NOISE, 200, "@40 v14 o5 c8");
    sfx_define("wb_cheer", CH_NOISE, 120, "@10 v8 o7 c8 c8 c8 c8");
    sfx_define("wb_fine", CH_P2, 180, "@37 v11 o3 c16 r32 c16 r32 c8");
    sfx_define("wb_train", CH_P2, 240, "@32 v10 o4 c16 g16 >c16");
    sfx_define("wb_shady", CH_P2, 160, "@5 v10 o4 e16 d+16 d16 c+8");
    sfx_define("wb_splat", CH_NOISE, 200, "@36 v10 o5 c16 @41 v10 o3 c16");
}
