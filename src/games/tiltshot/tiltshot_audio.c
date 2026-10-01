/* TILTSHOT - original music (UFO-MML) and sound effects. Every song is
 * eight bars played twice (or once, for the short ones), each channel the
 * same length so they loop together. */
#include "tiltshot.h"

int TSH_MUS_TITLE = -1, TSH_MUS_FRONT, TSH_MUS_BACK, TSH_MUS_LAST, TSH_MUS_BOARD, TSH_MUS_CHAMP, TSH_MUS_CUP, TSH_MUS_ACE,
    TSH_MUS_OVER, TSH_MUS_RUNNERUP;

/* a bar of four chord stabs, a bar of eight, root and fifth in the bass */
#define ST4(ins, oct, n) ins " " oct " " n "4 " n "4 " n "4 " n "4 "
#define ST8(ins, oct, n) ins " " oct " " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define OFF4(ins, oct, n) ins " " oct " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define RF4(r, f) r "4 " f "4 " r "4 " f "4 "
#define R8(lo, hi) lo "8 " lo "8 " hi "8 " lo "8 " lo "8 " lo "8 " hi "8 " lo "8 "

/* "Comet Classic" - the title. C major, bright.  C Am F G | C Am Dm-G C */
static const char TITLE_LEAD[] =
    "@23 v12 q7 [o5 e8 g8 o6 c4 o5 b8 o6 c8 d4 | o6 e4 d8 c8 o5 a4 o6 c4 | o6 c8 d8 c8 o5 a8 f4 a4 | o5 g2 r4 g8 a8"
    "| o5 e8 g8 o6 c4 o5 b8 o6 c8 e4 | o6 d4 c8 o5 b8 a4 o6 c4 | o5 d8 f8 a8 f8 g8 f8 e8 d8 | o5 c2. r4 ]2";
static const char TITLE_HARM[] =
    "v7 q5 [" ST4("@16", "o4", "c") ST4("@17", "o4", "a") ST4("@16", "o4", "f") ST4("@16", "o4", "g")
    ST4("@16", "o4", "c") ST4("@17", "o4", "a") "@17 o4 d4 d4 @16 g4 g4 " ST4("@16", "o4", "c") "]2";
static const char TITLE_BASS[] =
    "@6 v15 q6 [" RF4("o2 c", "o2 g") RF4("o2 a", "o3 e") RF4("o2 f", "o3 c") RF4("o2 g", "o3 d")
    RF4("o2 c", "o2 g") RF4("o2 a", "o3 e") "o2 d4 a4 g4 d4 " RF4("o2 c", "o2 g") "]2";
#define DRUM_POP "o2@13v12c8 o8@9v6c8 o6@11v10c8 o8@9v6c8 o2@13v12c8 o2@13v9c8 o6@11v10c8 o8@9v6c8 "
static const char TITLE_DRUMS[] = "[" DRUM_POP "]16";

/* "Front Nine" - the first nine holes. F major, strolling.  F Dm Bb C | F Dm Gm-C F */
static const char FRONT_LEAD[] =
    "@14 v11 q6 [o5 a4 o6 c8 o5 a8 g4 f4 | o5 d4 f8 e8 d4 r4 | o5 d8 f8 b-4 a8 g8 f4 | o5 e4 g4 c2"
    "| o5 a4 o6 c8 o5 a8 o6 f4 e4 | o6 d4 c8 o5 a8 f4 d4 | o5 g8 b-8 o6 d8 c8 o5 b-8 a8 g4 | o5 f2. r4 ]2";
static const char FRONT_HARM[] =
    "v6 q4 [" OFF4("@16", "o4", "f") OFF4("@17", "o4", "d") OFF4("@16", "o3", "b-") OFF4("@16", "o4", "c")
    OFF4("@16", "o4", "f") OFF4("@17", "o4", "d") "@17 o3 r8 g8 r8 g8 @16 o4 r8 c8 r8 c8 " OFF4("@16", "o4", "f") "]2";
static const char FRONT_BASS[] =
    "@6 v14 q6 [" RF4("o2 f", "o3 c") RF4("o2 d", "o2 a") RF4("o2 b-", "o3 f") RF4("o2 c", "o2 g")
    RF4("o2 f", "o3 c") RF4("o2 d", "o2 a") "o2 g4 d4 c4 g4 " RF4("o2 f", "o3 c") "]2";
#define DRUM_STROLL "o2@13v11c4 o6@11v8c4 o2@13v10c8 o2@13v8c8 o6@11v9c4 "
static const char FRONT_DRUMS[] = "[" DRUM_STROLL "]16";

/* "Back Nine" - holes ten to seventeen. A minor, driving.  Am F G Em | Am F Dm-E Am */
static const char BACK_LEAD[] =
    "@1 v11 q6 [o5 a8 o6 c8 e8 d8 c8 o5 b8 a4 | o5 f4 a8 g8 f4 e4 | o5 d8 g8 b8 o6 d8 c8 o5 b8 g4 | o5 e2 r4 e8 g8"
    "| o5 a8 o6 c8 e8 a8 g8 e8 c4 | o6 c8 o5 a8 f8 a8 o6 c4 o5 a4 | o5 d4 f4 e4 g+4 | o5 a2. r4 ]2";
static const char BACK_HARM[] =
    "v6 q3 [" ST8("@17", "o4", "a") ST8("@16", "o4", "f") ST8("@16", "o4", "g") ST8("@17", "o4", "e")
    ST8("@17", "o4", "a") ST8("@16", "o4", "f") "@17 o4 d8 d8 d8 d8 @16 e8 e8 e8 e8 " ST8("@17", "o4", "a") "]2";
static const char BACK_BASS[] =
    "@6 v14 q5 [" R8("o2 a", "o3 a") R8("o2 f", "o3 f") R8("o2 g", "o3 g") R8("o2 e", "o3 e")
    R8("o2 a", "o3 a") R8("o2 f", "o3 f") "o2 d8 d8 o3 d8 o2 d8 e8 e8 o3 e8 o2 e8 " R8("o2 a", "o3 a") "]2";
#define DRUM_DRIVE "o2@13v12c8 o8@9v6c8 o6@11v10c8 o8@9v6c8 o2@13v12c8 o8@9v6c8 o6@11v10c8 o6@11v7c16 o6@11v9c16 "
static const char BACK_DRUMS[] = "[" DRUM_DRIVE "]16";

/* "Eighteenth Hole" - the last hole. D minor, tense.  Dm Bb C A | Dm Bb Gm-A Dm */
static const char LAST_LEAD[] =
    "@23 v12 q6 [o5 d8 f8 a8 o6 d8 c8 o5 a8 f4 | o5 b-4 a8 g8 f4 d4 | o5 e8 g8 o6 c8 e8 d8 c8 o5 g4 | o5 a2 o6 c+4 e4"
    "| o6 d8 c8 o5 a8 f8 d8 f8 a4 | o5 b-8 o6 d8 f8 d8 c8 o5 b-8 a4 | o5 g4 b-4 a4 o6 c+4 | o6 d2. r4 ]2";
static const char LAST_HARM[] =
    "v6 q3 [" ST8("@17", "o4", "d") ST8("@16", "o3", "b-") ST8("@16", "o4", "c") ST8("@16", "o3", "a")
    ST8("@17", "o4", "d") ST8("@16", "o3", "b-") "@17 o3 g8 g8 g8 g8 @16 a8 a8 a8 a8 " ST8("@17", "o4", "d") "]2";
static const char LAST_BASS[] =
    "@6 v15 q5 [" R8("o2 d", "o3 d") R8("o2 b-", "o3 b-") R8("o2 c", "o3 c") R8("o2 a", "o3 a")
    R8("o2 d", "o3 d") R8("o2 b-", "o3 b-") "o2 g8 g8 o3 g8 o2 g8 a8 a8 o3 a8 o2 a8 " R8("o2 d", "o3 d") "]2";
#define DRUM_TENSE "o2@13v12c8 o2@13v10c8 o6@11v11c8 o2@13v10c8 o2@13v12c8 o2@13v10c8 o6@11v11c8 o6@11v9c8 "
static const char LAST_DRUMS[] = "[" DRUM_TENSE "]16";

/* "The Leaderboard" - between holes. G major, easy.  G Em C D | G Em Am-D G */
static const char BOARD_LEAD[] =
    "@5 v11 q7 [o5 b4 a8 g8 d4 g4 | o5 e4 g8 a8 b2 | o6 c4 o5 b8 a8 g4 e4 | o5 f+2. r4"
    "| o5 b4 o6 d8 c8 o5 b4 g4 | o5 e4 f+8 g8 b4 a4 | o5 a4 o6 c4 o5 f+4 a4 | o5 g2. r4 ]2";
static const char BOARD_HARM[] =
    "@4 v6 q8 [o4 b1 | o4 g1 | o4 g1 | o4 a1 | o4 b1 | o4 g1 | o4 e2 f+2 | o4 d1 ]2";
static const char BOARD_BASS[] =
    "@6 v13 q6 [o2 g2 d2 | o2 e2 b2 | o2 c2 g2 | o2 d2 a2 | o2 g2 d2 | o2 e2 b2 | o2 a2 d2 | o2 g2 g2 ]2";
#define DRUM_EASY "o6@11v6c4 o8@9v4c4 o2@13v8c4 o8@9v4c4 "
static const char BOARD_DRUMS[] = "[" DRUM_EASY "]16";

/* "Champion of the Comet" - the win and the credits. C major, brass.  C F G C | Am F G C */
static const char CHAMP_LEAD[] =
    "@23 v13 q7 [o5 c4 e4 g4 o6 c4 | o5 a4. g8 f4 a4 | o5 g4 f4 e4 d4 | o5 e2. r4"
    "| o5 a4 o6 c4 e4 c4 | o6 d4. c8 o5 a4 f4 | o5 g4 a4 b4 o6 d4 | o6 c2. r4 ]2";
static const char CHAMP_HARM[] =
    "v7 q5 [" ST4("@16", "o4", "c") ST4("@16", "o4", "f") ST4("@16", "o4", "g") ST4("@16", "o4", "c")
    ST4("@17", "o4", "a") ST4("@16", "o4", "f") ST4("@16", "o4", "g") ST4("@16", "o4", "c") "]2";
static const char CHAMP_BASS[] =
    "@6 v15 q6 [" RF4("o2 c", "o2 g") RF4("o2 f", "o3 c") RF4("o2 g", "o3 d") RF4("o2 c", "o2 g")
    RF4("o2 a", "o3 e") RF4("o2 f", "o3 c") RF4("o2 g", "o3 d") RF4("o2 c", "o2 g") "]2";
#define DRUM_MARCH "o2@13v12c4 o6@11v10c4 o2@13v12c4 o6@11v9c8 o6@11v11c8 "
static const char CHAMP_DRUMS[] = "[" DRUM_MARCH "]16";

/* jingles */
static const char CUP_P1[] = "@20 v12 o5 l16 c e g o6 c8";
static const char CUP_P2[] = "@20 v8 o5 l16 e g o6 c e8";
static const char ACE_P1[] = "@1 v12 o5 l16 c e g o6 c e g o7 c8 r8 o6 g8 o7 c2";
static const char ACE_P2[] = "@16 v8 o4 l8 c c c c r8 o4 g8 o5 c2";
static const char ACE_TRI[] = "@6 v14 o2 l8 c g c g r8 o2 g8 o3 c2";
static const char OVER_P1[] = "@5 v11 o5 l8 e d c o4 g4";
static const char OVER_TRI[] = "@7 v11 o2 l4 c o1 g";
static const char RUNNER_P1[] = "@5 v11 o5 l8 g e c o4 a4. g8 a2";
static const char RUNNER_TRI[] = "@6 v12 o2 l4 c o1 a f g2";

void tsh_audio_load(void) {
    if (TSH_MUS_TITLE >= 0) return;
    TSH_MUS_TITLE = song_define("tsh_title", 132, true, TITLE_LEAD, TITLE_HARM, TITLE_BASS, TITLE_DRUMS);
    TSH_MUS_FRONT = song_define("tsh_front", 112, true, FRONT_LEAD, FRONT_HARM, FRONT_BASS, FRONT_DRUMS);
    TSH_MUS_BACK = song_define("tsh_back", 126, true, BACK_LEAD, BACK_HARM, BACK_BASS, BACK_DRUMS);
    TSH_MUS_LAST = song_define("tsh_last", 138, true, LAST_LEAD, LAST_HARM, LAST_BASS, LAST_DRUMS);
    TSH_MUS_BOARD = song_define("tsh_board", 96, true, BOARD_LEAD, BOARD_HARM, BOARD_BASS, BOARD_DRUMS);
    TSH_MUS_CHAMP = song_define("tsh_champ", 116, true, CHAMP_LEAD, CHAMP_HARM, CHAMP_BASS, CHAMP_DRUMS);
    TSH_MUS_CUP = song_define("tsh_cup", 150, false, CUP_P1, CUP_P2, "@7 v12 o3 c8", "");
    TSH_MUS_ACE = song_define("tsh_ace", 140, false, ACE_P1, ACE_P2, ACE_TRI, "@12 v8 o6 c2");
    TSH_MUS_OVER = song_define("tsh_over", 120, false, OVER_P1, "", OVER_TRI, "");
    TSH_MUS_RUNNERUP = song_define("tsh_runnerup", 100, false, RUNNER_P1, "", RUNNER_TRI, "");

    sfx_define("tsh_aim", CH_P2, 240, "@42 v6 o7 c32");
    sfx_define("tsh_tick", CH_P2, 240, "@35 v7 o6 c32");
    sfx_define("tsh_full", CH_P2, 240, "@39 v10 o6 c16 g16");
    sfx_define("tsh_warn", CH_P1, 200, "@0 v12 o6 c16 r16 c16 r16 c16 r16 c16");
    sfx_define("tsh_boom", CH_NOISE, 120, "@34 v15 o3 c2");
    sfx_define("tsh_swing", CH_NOISE, 240, "@36 v10 o7 c16");
    sfx_define("tsh_slam", CH_P2, 220, "@33 v12 o6 c16 o5 c16");
    sfx_define("tsh_bounce", CH_TRI, 240, "@8 v10 o3 c32");
    sfx_define("tsh_bumper", CH_P2, 240, "@15 v12 o6 e32 g32 o7 c16");
    sfx_define("tsh_spring", CH_P1, 220, "@32 v12 o4 c16 g16");
    sfx_define("tsh_junk", CH_NOISE, 220, "@13 v12 o4 c16 @21 v9 o6 c16");
    sfx_define("tsh_mover", CH_NOISE, 200, "@40 v13 o5 c8");
    sfx_define("tsh_skip", CH_P2, 240, "@35 v10 o7 c32 o6 g32");
    sfx_define("tsh_splash", CH_NOISE, 160, "@10 v13 o5 c8 @9 v8 o7 c8");
    sfx_define("tsh_pit", CH_P1, 180, "@33 v11 o5 c8 o4 c8");
    sfx_define("tsh_sand", CH_NOISE, 240, "@9 v9 o4 c16");
    sfx_define("tsh_fire", CH_P1, 200, "@32 v13 o4 c16 o5 c16 o6 c8");
    sfx_define("tsh_cup", CH_P2, 220, "@15 v12 o6 c16 e16 g8");
    sfx_define("tsh_secret", CH_P1, 140, "@5 v11 o3 c4 c+4 d2");
    sfx_define("tsh_code", CH_P1, 160, "@39 v12 o5 l16 c e g o6 c e g o7 c4");
}
