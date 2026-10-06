/* DRIFTLINE - original music (UFO-MML) and sound effects, all written for
 * UFO 40. Every looping song's channels are a whole number of bars. */
#include "driftline.h"

int DFL_MUS_TITLE = -1, DFL_MUS_STAGE[DFL_STAGES], DFL_MUS_BOSS, DFL_MUS_FINAL, DFL_MUS_BONUS, DFL_MUS_CLEAR,
    DFL_MUS_OVER, DFL_MUS_ENDING, DFL_MUS_CREDITS, DFL_MUS_COIN;

/* one bar of eighth notes on one bass note, and half a bar */
#define EI(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define EI4(n) n "8 " n "8 " n "8 " n "8 "
/* octave-jumping eighths, a bar */
#define OCT(lo, hi) "[" lo "8 " hi "8]4 "
/* a bar of sixteenth-note arpeggio on four notes */
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
/* a bar of off-beat chord stabs */
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
/* half a bar of kick, hat, snare, hat */
#define DR "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
/* a bar of backbeat on quarters */
#define BEAT "@13 v12 o2 c4 @11 v10 o6 c4 @13 v12 o2 c4 @11 v10 o6 c4 "
/* a bar of sixteenth hats with a kick and a snare */
#define BUSY "@13 v12 o2 c16 @9 v4 o8 c16 c16 c16 @11 v10 o6 c16 @9 v4 o8 c16 c16 c16 " \
             "@13 v12 o2 c16 @9 v4 o8 c16 @13 v10 o2 c16 @9 v4 o8 c16 @11 v10 o6 c16 @9 v4 o8 c16 c16 c16 "

/* "Driftline" - the title, F major */
static const char TITLE_LEAD[] =
    "@1 v11 q6"
    "| o5 c8 f8 a8 o6 c8 ^4 o5 a4 | o5 d8 f8 a8 o6 d8 ^4 c4 | o5 b-8 a8 g8 f8 d4 f4 | o5 g2 e4 c4"
    "| o5 c8 f8 a8 o6 c8 ^4 f4 | o6 e8 d8 c8 o5 a8 f4 d4 | o5 g8 a8 b-8 o6 d8 c4 o5 b-4 | o5 a2 g4 r4";
static const char TITLE_ARP[] =
    "@2 v5 q5 " ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 d", "f", "a", "f") ARP("o3 b-", "o4 d", "f", "d")
    ARP("o4 c", "e", "g", "e") ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 d", "f", "a", "f")
    ARP("o4 g", "b-", "o5 d", "o4 b-") ARP("o4 c", "e", "g", "e");
static const char TITLE_BASS[] =
    "@6 v14 q5 " EI("o2 f") EI("o2 d") EI("o1 b-") EI("o2 c") EI("o2 f") EI("o2 d") EI("o1 g") EI("o2 c");
static const char TITLE_DRUM[] = "[" DR DR "]8";

/* "Harbour Road" - stage 1, C major, an easy morning */
static const char S1_LEAD[] =
    "@14 v11 q6"
    "| o5 e4 g8 e8 c4 e4 | o5 a4 g8 e8 c2 | o5 f4 a8 g8 f4 c4 | o5 d2 g4 r4"
    "| o5 e4 g8 o6 c8 ^4 o5 b8 a8 | o5 a4 e8 g8 a4 o6 c4 | o6 d8 c8 o5 a8 f8 d4 a4 | o5 g2. r4"
    "| o5 a4 o6 c4 o5 a4 f4 | o5 b4 o6 d4 o5 b4 g4 | o5 g8 a8 b8 o6 e8 ^2 | o6 e8 d8 c8 o5 b8 a2"
    "| o5 a8 b8 o6 c8 d8 c4 o5 a4 | o5 b8 o6 c8 d8 e8 d4 o5 b4 | o6 c4 o5 g4 e4 g4 | o6 c2 r2";
static const char S1_COMP[] =
    "v6 q3 " OFF("@16 o4", "c") OFF("@17 o4", "a") OFF("@16 o4", "f") OFF("@16 o4", "g")
    OFF("@16 o4", "c") OFF("@17 o4", "a") OFF("@17 o4", "d") OFF("@16 o4", "g")
    OFF("@16 o4", "f") OFF("@16 o4", "g") OFF("@17 o4", "e") OFF("@17 o4", "a")
    OFF("@16 o4", "f") OFF("@16 o4", "g") OFF("@16 o4", "c") OFF("@16 o4", "c");
static const char S1_BASS[] =
    "@6 v14 q5"
    "| o2 c4 g4 c4 g4 | o1 a4 o2 e4 o1 a4 o2 e4 | o2 f4 c4 f4 c4 | o2 g4 d4 g4 d4"
    "| o2 c4 g4 c4 g4 | o1 a4 o2 e4 o1 a4 o2 e4 | o2 d4 a4 d4 a4 | o2 g4 d4 g4 b4"
    "| o2 f4 c4 f4 c4 | o2 g4 d4 g4 d4 | o2 e4 b4 e4 b4 | o1 a4 o2 e4 o1 a4 o2 e4"
    "| o2 f4 c4 f4 c4 | o2 g4 d4 g4 d4 | o2 c4 g4 c4 g4 | o2 c4 g4 c2";
static const char S1_DRUM[] = "[" DR DR "]16";

/* "Sundown Strip" - stage 2, A minor, the purple hour */
static const char S2_LEAD[] =
    "@14 v11 q7"
    "| o5 e2 a4 b4 | o6 c2 o5 a2 | o5 g2 e4 g4 | o5 d1"
    "| o5 e2 a4 o6 c4 | o6 d4 c4 o5 a2 | o5 g4 a4 b4 o6 d4 | o5 b1";
static const char S2_ARP[] =
    "@4 v6 q6 " ARP("o4 a", "o5 c", "e", "c") ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 c", "e", "g", "e")
    ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 a", "o5 c", "e", "c") ARP("o4 f", "a", "o5 c", "o4 a")
    ARP("o4 c", "e", "g", "e") ARP("o4 e", "g+", "b", "g+");
static const char S2_BASS[] =
    "@6 v14 q4 " OCT("o2 a", "o3 a") OCT("o2 f", "o3 f") OCT("o2 c", "o3 c") OCT("o2 g", "o3 g")
    OCT("o2 a", "o3 a") OCT("o2 f", "o3 f") OCT("o2 c", "o3 c") OCT("o2 e", "o3 e");
static const char S2_DRUM[] = "[" BEAT "]8";

/* "Moonlit Mile" - stage 3, D minor, past the graveyard */
static const char S3_LEAD[] =
    "@1 v11 q5"
    "| o5 d8 f8 a8 d8 c+8 d8 f8 a8 | o5 b-4 a8 g8 f4 d4 | o5 g8 b-8 o6 d8 o5 b-8 a8 g8 f8 e8 | o5 e2 c+4 r4"
    "| o5 d8 f8 a8 o6 d8 c+8 d8 f8 d8 | o6 d4 c8 o5 b-8 a4 f4 | o5 g8 f8 e8 g8 b-8 a8 g8 e8 | o5 d2 r2";
static const char S3_PLUCK[] =
    "@3 v6 q3 [o4 d8 a8 f8 a8]2 [o4 d8 b-8 f8 b-8]2 [o4 d8 b-8 g8 b-8]2 [o4 c+8 a8 e8 a8]2"
    " [o4 d8 a8 f8 a8]2 [o4 d8 b-8 f8 b-8]2 [o4 e8 b-8 g8 b-8]2 [o4 c+8 a8 e8 a8]2";
static const char S3_BASS[] =
    "@6 v14 q6"
    "| o2 d4 f4 a4 f4 | o1 b-4 o2 d4 f4 d4 | o1 g4 b-4 o2 d4 o1 b-4 | o1 a4 o2 c+4 e4 c+4"
    "| o2 d4 f4 a4 f4 | o1 b-4 o2 d4 f4 d4 | o1 g4 b-4 o2 e4 g4 | o1 a4 o2 e4 o1 a2";
static const char S3_DRUM[] = "[@13 v11 o2 c8 @9 v4 o8 c8 @9 v4 o8 c8 @11 v9 o6 c8]16";

/* "Open Water" - stage 4, D major, across the causeway */
static const char S4_LEAD[] =
    "@1 v11 q6"
    "| o5 f+8 a8 o6 d8 e8 f+4 e8 d8 | o5 a4 o6 c+8 e8 a4 e4 | o6 d8 c+8 o5 b8 a8 f+4 b4 | o5 g8 a8 b8 o6 d8 g4 f+8 e8"
    "| o6 f+8 e8 d8 c+8 d4 f+4 | o6 e8 d8 c+8 o5 b8 a4 o6 c+4 | o5 b8 o6 c+8 d8 e8 d4 o5 b4 | o5 a2 o6 c+4 e4";
static const char S4_ARP[] =
    "@2 v5 q5 " ARP("o4 d", "f+", "a", "f+") ARP("o4 a", "o5 c+", "e", "c+") ARP("o4 b", "o5 d", "f+", "d")
    ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 d", "f+", "a", "f+") ARP("o4 a", "o5 c+", "e", "c+")
    ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 a", "o5 c+", "e", "c+");
static const char S4_BASS[] =
    "@6 v14 q4 " EI("o2 d") EI("o1 a") EI("o1 b") EI("o1 g") EI("o2 d") EI("o1 a") EI("o1 g") EI("o1 a");
static const char S4_DRUM[] = "[" BUSY "]8";

/* "Trouble on the Coast" - the bosses of stages 1 to 3, E minor */
static const char BOSS_LEAD[] =
    "@0 v11 q5 ["
    "| o5 e8 e8 g8 e8 b8 e8 a8 g8 | o5 e8 e8 g8 e8 o6 c8 o5 b8 a8 g8"
    "| o5 f+8 f+8 a8 f+8 o6 d8 c8 o5 b8 a8 | o5 b8 a8 g8 f+8 d+4 f+4 ]2";
static const char BOSS_STAB[] =
    "v6 q2 [" OFF("@17 o4", "e") OFF("@16 o4", "c") OFF("@16 o4", "d") OFF("@16 o3", "b") "]2";
static const char BOSS_BASS[] = "@6 v14 q4 [" EI("o2 e") EI("o2 c") EI("o2 d") EI("o1 b") "]2";
static const char BOSS_DRUM[] = "[" BUSY "]8";

/* "High Tide" - the last boss, C minor */
static const char FIN_LEAD[] =
    "@1 v11 q6"
    "| o5 c8 e-8 g8 o6 c8 ^4 o5 b8 g8 | o5 a-8 o6 c8 e-8 c8 o5 a-4 g4 | o5 b-8 o6 d8 f8 d8 o5 b-4 a-4 | o5 g8 f8 e-8 d8 o4 b4 o5 d4"
    "| o5 c8 e-8 g8 o6 c8 e-4 d8 c8 | o5 a-8 b-8 o6 c8 e-8 f4 e-4 | o6 d8 c8 o5 a-8 f8 a-4 g4 | o5 g2 b4 o6 d4";
static const char FIN_ARP[] =
    "@2 v5 q5 " ARP("o4 c", "e-", "g", "e-") ARP("o4 a-", "o5 c", "e-", "c") ARP("o4 b-", "o5 d", "f", "d")
    ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 c", "e-", "g", "e-") ARP("o4 a-", "o5 c", "e-", "c")
    ARP("o4 f", "a-", "o5 c", "o4 a-") ARP("o4 g", "b", "o5 d", "o4 b");
static const char FIN_BASS[] =
    "@6 v14 q4 " OCT("o2 c", "o3 c") OCT("o1 a-", "o2 a-") OCT("o1 b-", "o2 b-") OCT("o1 g", "o2 g")
    OCT("o2 c", "o3 c") OCT("o1 a-", "o2 a-") OCT("o2 f", "o3 f") OCT("o1 g", "o2 g");
static const char FIN_DRUM[] = "[" BUSY "]8";

/* "Bubble and Bounce" - the bonus stages, G major */
static const char BON_LEAD[] =
    "@15 v11 q6"
    "| o5 g8 b8 o6 d8 o5 b8 g4 d4 | o5 e8 g8 b8 g8 e4 o4 b4 | o5 c8 e8 g8 e8 a4 g4 | o5 f+8 g8 a8 b8 a4 d4"
    "| o5 g8 b8 o6 d8 g8 f+4 e4 | o6 e8 d8 c8 o5 b8 g4 e4 | o5 c8 e8 a8 g8 f+4 a4 | o5 g2 r2";
static const char BON_ARP[] =
    "@3 v6 q4 " ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 e", "g", "b", "g") ARP("o4 c", "e", "g", "e")
    ARP("o4 d", "f+", "a", "f+") ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 e", "g", "b", "g")
    ARP("o4 c", "e", "a", "e") ARP("o4 g", "b", "o5 d", "o4 b");
static const char BON_BASS[] =
    "@7 v14 q4 " OCT("o2 g", "o3 g") OCT("o2 e", "o3 e") OCT("o2 c", "o3 c") OCT("o2 d", "o3 d")
    OCT("o2 g", "o3 g") OCT("o2 e", "o3 e") OCT("o2 a", "o3 a") OCT("o2 g", "o3 g");
static const char BON_DRUM[] = "[" DR DR "]8";

/* "The Last Mile" - the ending, F major */
static const char END_LEAD[] =
    "@14 v11 q7"
    "| o5 a2 g4 f4 | o5 g2. c4 | o5 d4 f4 a4 o6 d4 | o6 c2 o5 b-2"
    "| o5 a4 o6 c4 f4 e4 | o6 d2 c4 o5 b-4 | o5 a4 g4 f4 d4 | o5 f1";
static const char END_ARP[] =
    "@4 v5 q6 " ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 e", "g", "o5 c", "o4 g") ARP("o4 d", "f", "a", "f")
    ARP("o3 b-", "o4 d", "f", "d") ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 c", "e", "g", "e")
    ARP("o3 b-", "o4 d", "f", "d") ARP("o4 f", "a", "o5 c", "o4 a");
static const char END_BASS[] = "@6 v13 q7 o2 f2 f2 e2 e2 d2 d2 o1 b-2 b-2 o2 f2 f2 c2 c2 o1 b-2 b-2 o2 f2 f2";
static const char END_SOFT[] = "[@9 v3 o8 c8]64";

/* "Tail Lights" - the credits, C major */
static const char CRED_LEAD[] =
    "@0 v11 q6"
    "| o5 e8 g8 o6 c8 o5 g8 e4 c4 | o5 d8 g8 b8 g8 d4 o4 b4 | o5 c8 e8 a8 e8 c4 o4 a4 | o5 f8 a8 o6 c8 o5 a8 f2"
    "| o5 e8 g8 o6 c8 e8 d4 c4 | o5 b8 o6 d8 g8 d8 o5 b4 g4 | o5 a8 o6 c8 f8 c8 o5 a4 f4 | o5 g4 a4 b4 o6 c4";
static const char CRED_ARP[] =
    "@2 v5 q5 [o4 c8 e8 g8 e8]2 [o3 b8 o4 d8 g8 d8]2 [o4 c8 e8 a8 e8]2 [o4 c8 f8 a8 f8]2"
    " [o4 c8 e8 g8 e8]2 [o3 b8 o4 d8 g8 d8]2 [o4 c8 f8 a8 f8]2 [o3 b8 o4 d8 g8 d8]2";
static const char CRED_BASS[] =
    "@6 v12 q5 " EI("o2 c") EI("o1 g") EI("o1 a") EI("o2 f") EI("o2 c") EI("o1 g") EI("o2 f") EI("o1 g");
static const char CRED_DRUM[] = "[@13 v10 o2 c4 @11 v8 o6 c4]16";

/* jingles */
static const char CLEAR_P1[] = "@1 v12 o5 l16 c e g o6 c8 o5 g8 o6 e4 d8 e8 c2";
static const char CLEAR_TRI[] = "@6 v13 o3 c8 g8 c8 e8 g4 f8 g8 c2";
static const char CLEAR_NOISE[] = "@13 v12 o2 c8 r8 @11 v10 o6 c8 r8 @12 v11 o5 c2";
static const char OVER_P1[] = "@0 v11 o5 e8 d8 c8 o4 b8 a4 g+4 a2";
static const char OVER_TRI[] = "@6 v12 o2 a4 e4 a4 e4 a2";
static const char COIN_P1[] = "@39 v12 o5 l16 c e g o6 c e g o7 c4 r8 o6 g8 o7 c2";
static const char COIN_TRI[] = "@6 v12 o3 c8 e8 g8 o4 c8 e4 r8 c8 c2";

void dfl_audio_load(void) {
    if (DFL_MUS_TITLE >= 0) return;
    DFL_MUS_TITLE = song_define("dfl_title", 140, true, TITLE_LEAD, TITLE_ARP, TITLE_BASS, TITLE_DRUM);
    DFL_MUS_STAGE[0] = song_define("dfl_harbour", 132, true, S1_LEAD, S1_COMP, S1_BASS, S1_DRUM);
    DFL_MUS_STAGE[1] = song_define("dfl_sundown", 116, true, S2_LEAD, S2_ARP, S2_BASS, S2_DRUM);
    DFL_MUS_STAGE[2] = song_define("dfl_moonlit", 126, true, S3_LEAD, S3_PLUCK, S3_BASS, S3_DRUM);
    DFL_MUS_STAGE[3] = song_define("dfl_openwater", 144, true, S4_LEAD, S4_ARP, S4_BASS, S4_DRUM);
    DFL_MUS_BOSS = song_define("dfl_boss", 156, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    DFL_MUS_FINAL = song_define("dfl_hightide", 150, true, FIN_LEAD, FIN_ARP, FIN_BASS, FIN_DRUM);
    DFL_MUS_BONUS = song_define("dfl_bonus", 150, true, BON_LEAD, BON_ARP, BON_BASS, BON_DRUM);
    DFL_MUS_ENDING = song_define("dfl_ending", 96, true, END_LEAD, END_ARP, END_BASS, END_SOFT);
    DFL_MUS_CREDITS = song_define("dfl_credits", 120, true, CRED_LEAD, CRED_ARP, CRED_BASS, CRED_DRUM);
    DFL_MUS_CLEAR = song_define("dfl_clear", 150, false, CLEAR_P1, "", CLEAR_TRI, CLEAR_NOISE);
    DFL_MUS_OVER = song_define("dfl_over", 100, false, OVER_P1, "", OVER_TRI, "");
    DFL_MUS_COIN = song_define("dfl_coin", 160, false, COIN_P1, "", COIN_TRI, "");

    sfx_define("dfl_shot", CH_P2, 240, "@20 v4 o6 e32");
    sfx_define("dfl_side", CH_P2, 240, "@20 v4 o5 b32 e32");
    sfx_define("dfl_hit", CH_NOISE, 240, "@21 v6 o7 c32");
    sfx_define("dfl_pop2", CH_NOISE, 200, "@34 v10 o5 c8");
    sfx_define("dfl_boom", CH_NOISE, 160, "@34 v13 o4 c4");
    sfx_define("dfl_bigboom", CH_NOISE, 100, "@34 v15 o3 c1");
    sfx_define("dfl_crash", CH_P1, 160, "@33 v13 o5 c8 o4 g8 e8 c4");
    sfx_define("dfl_drift", CH_NOISE, 240, "@36 v4 o8 c16");
    sfx_define("dfl_tier", CH_P1, 220, "@39 v10 o5 l32 c e g o6 c");
    sfx_define("dfl_efire", CH_NOISE, 240, "@36 v6 o7 c32");
    sfx_define("dfl_beam", CH_P2, 160, "@38 v10 o3 c4 g4");
    sfx_define("dfl_bounce", CH_P2, 220, "@20 v7 o5 c32 g32");
    sfx_define("dfl_paddle", CH_P2, 220, "@15 v10 o5 g16 o6 c16");
    sfx_define("dfl_block", CH_P1, 240, "@35 v10 o6 e16 b16");
    sfx_define("dfl_chip", CH_NOISE, 240, "@21 v5 o6 c32");
    sfx_define("dfl_pop", CH_P1, 200, "@32 v12 o5 c16 g16 o6 c8");
    sfx_define("dfl_coin", CH_P1, 200, "@39 v13 o6 l16 c e g o7 c4");
    sfx_define("dfl_lost", CH_P1, 150, "@33 v11 o5 g8 e8 c4");
    sfx_define("dfl_clink", CH_P2, 240, "@15 v8 o7 c32 g32");
    sfx_define("dfl_crunch", CH_NOISE, 200, "@40 v11 o4 c8");
    sfx_define("dfl_thud", CH_NOISE, 200, "@13 v10 o2 c16");
    sfx_define("dfl_snap", CH_P2, 220, "@37 v9 o4 c16 o5 c16");
    sfx_define("dfl_slam", CH_NOISE, 140, "@34 v14 o2 c4");
    sfx_define("dfl_whoosh", CH_NOISE, 160, "@36 v9 o5 c4");
    sfx_define("dfl_splash", CH_NOISE, 180, "@10 v9 o6 c8");
    sfx_define("dfl_breach", CH_P2, 160, "@41 v9 o3 c8 o2 g8");
    sfx_define("dfl_wave", CH_NOISE, 120, "@12 v13 o5 c2");
    sfx_define("dfl_bubble", CH_P2, 220, "@38 v8 o5 c16 g16");
    sfx_define("dfl_start", CH_P1, 200, "@32 v12 o4 c8 g8 o5 c4");
}
