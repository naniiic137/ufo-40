/* CUTLASS CUP - original music (UFO-MML) and sound effects.
 * Like the game it pays tribute to, matches are played without music:
 * only a jingle when a point is won. */
#include "cutlass.h"

int CC_MUS_TITLE = -1, CC_MUS_SELECT, CC_MUS_BRACKET, CC_MUS_POINT, CC_MUS_WIN, CC_MUS_LOSE, CC_MUS_CUP;

/* "Heave and Hoist" - the title, a swinging shanty in D minor.
 * Dm Dm C C | Bb A Dm A, twice (the second time home to D). */
static const char TITLE_LEAD[] =
    "@1 v12 q6 o5"
    "| d6 f12 a6 f12 d4 a4 | o6 d6 c12 o5 a6 f12 e4 d4 | e6 g12 o6 c6 o5 g12 e4 c4 | d6 e12 f6 e12 d4 c4"
    "| d6 f12 b-6 f12 d4 f4 | c+6 e12 a6 g12 f4 e4 | d6 f12 a6 o6 d12 f4 d4 | o5 e6 f12 e6 d12 c+2"
    "| d6 f12 a6 f12 d4 a4 | o6 d6 c12 o5 a6 f12 e4 d4 | e6 g12 o6 c6 o5 g12 e4 c4 | d6 e12 f6 e12 d4 c4"
    "| d6 f12 b-6 f12 d4 f4 | c+6 e12 a6 g12 f4 e4 | d6 f12 a6 o6 d12 f4 d4 | o5 f6 e12 d6 c+12 d2";
#define STRUM(ins, oct, n) ins " " oct " r12 " n "12 " n "12 r12 " n "12 " n "12 r12 " n "12 " n "12 r12 " n "12 " n "12 "
static const char TITLE_HARM[] =
    "v6 q4 [" STRUM("@17", "o4", "d") STRUM("@17", "o4", "d") STRUM("@16", "o4", "c") STRUM("@16", "o4", "c")
    STRUM("@16", "o3", "b-") STRUM("@16", "o3", "a") STRUM("@17", "o4", "d") STRUM("@16", "o3", "a") "]2";
#define ROOT5(r, f) "o2 " r "4 " f "4 " r "4 " f "4 "
static const char TITLE_BASS[] =
    "@6 v15 q5 [" ROOT5("d", "a") ROOT5("d", "a") ROOT5("c", "g") ROOT5("c", "g") ROOT5("b-", "f") ROOT5("a", "e")
    ROOT5("d", "a") ROOT5("a", "e") "]2";
#define SWAY6 "o2@13v12c4 o8@9v5c12 c12 c12 o6@11v10c4 o8@9v5c12 c12 o8@10v7c12 "
static const char TITLE_DRUMS[] = "[" SWAY6 "]16";

/* "Choose Your Crew" - bright and quick, D major. D G A D | Bm G A A */
static const char SELECT_LEAD[] =
    "@14 v11 q6 o5"
    "| f+8 a8 o6 d8 o5 a8 f+8 a8 d4 | g8 b8 o6 d8 o5 b8 g4 b4 | a8 o6 c+8 e8 c+8 o5 a4 e4 | f+2 d4 r4"
    "| f+8 b8 o6 d8 f+8 e8 d8 o5 b4 | g8 b8 o6 d8 g8 f+8 e8 d4 | c+8 d8 e8 f+8 e4 c+4 | o5 a2 r2";
#define PLK(n1, n2, n3) "o4 " n1 "8 " n2 "8 " n3 "8 " n2 "8 " n1 "8 " n2 "8 " n3 "8 " n2 "8 "
static const char SELECT_HARM[] =
    "@3 v6 q5 " PLK("d", "f+", "a") PLK("d", "g", "b") PLK("c+", "e", "a") PLK("d", "f+", "a")
    PLK("d", "f+", "b") PLK("d", "g", "b") PLK("c+", "e", "a") PLK("c+", "e", "a");
static const char SELECT_BASS[] =
    "@6 v14 q6 o2 d4 d4 a4 a4 | g4 g4 d4 d4 | a4 a4 e4 e4 | d4 a4 d4 d4"
    "| b4 b4 f+4 f+4 | g4 g4 d4 d4 | a4 a4 e4 e4 | a4 e4 a4 a4";
#define DRQ "o2@13v11c8 o8@9v5c8 o6@11v10c8 o8@9v5c8 "
static const char SELECT_DRUMS[] = "[" DRQ "]16";

/* "The Road to the Cup" - the bracket, G major, proud. G C D G | Em C D D */
static const char BRACKET_LEAD[] =
    "@23 v11 q7 o4"
    "| g4. a8 b4 o5 d4 | e4. d8 c4 o4 b4 | a4 b8 o5 c8 d4 o4 a4 | b2. r4"
    "| b4. o5 c8 d4 e4 | e4 g4 e4 c4 | d4. c8 o4 b4 a4 | a2. r4";
static const char BRACKET_HARM[] =
    "@4 v6 q8 o4 [b1 | o5 c1 | o4 a1 | b1 | g1 | g1 | f+1 | f+1 ]1";
static const char BRACKET_BASS[] =
    "@6 v13 q6 o2 g2 d2 | c2 g2 | d2 a2 | g2 d2 | e2 b2 | c2 g2 | d2 a2 | d2 d2";
static const char BRACKET_DRUMS[] = "[o2@13v10c4 o6@11v7c4 o2@13v9c4 o6@11v8c8 o6@11v6c8]8";

/* "Raise the Cup" - the ending, D major, triumphant */
static const char CUP_LEAD[] =
    "@16 v12 q7 o5"
    "| d4 f+4 a4 o6 d4 | c+4. o5 b8 a2 | b4 a4 g4 f+4 | e2. r4"
    "| d4 f+4 a4 o6 d4 | e4. d8 c+4 e4 | d4 o5 a4 f+4 a4 | d2. r4";
static const char CUP_HARM[] =
    "@22 v8 q6 o4 [a8 f+8 d8 f+8]2 | [a8 e8 c+8 e8]2 | [b8 g8 d8 g8]2 | [a8 e8 c+8 e8]2"
    "| [a8 f+8 d8 f+8]2 | [a8 e8 c+8 e8]2 | [a8 f+8 d8 f+8]2 | [a8 f+8 d8 f+8]2";
static const char CUP_BASS[] = "@6 v14 q6 o2 d2 a2 | a2 e2 | g2 d2 | a2 e2 | d2 a2 | a2 e2 | d2 a2 | d2 d2";
static const char CUP_DRUMS[] = "[o2@13v12c4 o6@11v9c4 o2@13v12c8 c8 o6@11v10c4]8";

/* jingles */
static const char POINT_P1[] = "@20 v12 o5 l16 d f+ a o6 d8";
static const char POINT_P2[] = "@20 v8 o5 l16 a o6 d f+ a8";
static const char WIN_P1[] = "@16 v12 o5 l8 d f+ a o6 d4. c+8 d8 e8 f+2";
static const char WIN_P2[] = "@22 v8 o5 l8 a o6 d f+ a4. a8 a8 b8 o7 d2";
static const char WIN_TRI[] = "@6 v14 o2 l4 d a o3 d o2 a d2";
static const char LOSE_P1[] = "@5 v11 o5 l8 a g f e4. d8 c+2";
static const char LOSE_TRI[] = "@6 v13 o2 l4 d c o1 b- a2";

void cc_audio_load(void) {
    if (CC_MUS_TITLE >= 0) return;
    CC_MUS_TITLE = song_define("cc_title", 126, true, TITLE_LEAD, TITLE_HARM, TITLE_BASS, TITLE_DRUMS);
    CC_MUS_SELECT = song_define("cc_select", 140, true, SELECT_LEAD, SELECT_HARM, SELECT_BASS, SELECT_DRUMS);
    CC_MUS_BRACKET = song_define("cc_bracket", 104, true, BRACKET_LEAD, BRACKET_HARM, BRACKET_BASS, BRACKET_DRUMS);
    CC_MUS_CUP = song_define("cc_cup", 112, true, CUP_LEAD, CUP_HARM, CUP_BASS, CUP_DRUMS);
    CC_MUS_POINT = song_define("cc_point", 150, false, POINT_P1, POINT_P2, "@7 v12 o3 d8", "");
    CC_MUS_WIN = song_define("cc_win", 140, false, WIN_P1, WIN_P2, WIN_TRI, "@12 v8 o6 c2");
    CC_MUS_LOSE = song_define("cc_lose", 96, false, LOSE_P1, "", LOSE_TRI, "");

    sfx_define("cc_strike", CH_NOISE, 240, "@21 v12 o6 c16");
    sfx_define("cc_sweet", CH_P2, 240, "@39 v12 o6 c16 g16");
    sfx_define("cc_super", CH_P1, 200, "@32 v13 o4 c16 g16 o5 c16 g8");
    sfx_define("cc_wall", CH_TRI, 240, "@8 v12 o3 c16");
    sfx_define("cc_body", CH_NOISE, 220, "@13 v12 o3 c16 @37 v8 o4 c16");
    sfx_define("cc_roll", CH_NOISE, 240, "@36 v7 o6 c16");
    sfx_define("cc_swing", CH_NOISE, 240, "@36 v6 o8 c32");
    sfx_define("cc_second", CH_P2, 240, "@33 v10 o6 c16");
    sfx_define("cc_stun", CH_P2, 200, "@37 v10 o4 c16 o3 g16");
    sfx_define("cc_blast", CH_NOISE, 140, "@34 v15 o3 c4");
    sfx_define("cc_catch", CH_P2, 220, "@15 v11 o5 c16 c16");
    sfx_define("cc_reflect", CH_P2, 240, "@32 v11 o5 c16 o6 c16");
    sfx_define("cc_foul", CH_P2, 180, "@15 v12 o6 e8 c8");
    sfx_define("cc_serve", CH_P2, 200, "@15 v10 o6 g16 o7 c16");
    sfx_define("cc_meter", CH_P2, 240, "@35 v9 o6 e32 g32");
    sfx_define("cc_splash", CH_NOISE, 160, "@10 v12 o5 c8 @9 v8 o7 c8");
    sfx_define("cc_blink", CH_P2, 240, "@38 v9 o6 c32 o7 c32");
    sfx_define("cc_whistle", CH_P1, 200, "@0 v12 o7 c8 r16 c4");
}
