/* FULL PEAL - original music (UFO-MML) and sound effects, all written for
 * UFO 40. The title tune is built from rung changes: rounds (the scale
 * straight down), then queens and tittums, the orders bell-ringers call.
 * Every looping song's channels are a whole number of the same bars. */
#include "fpl.h"

int FPL_MUS_TITLE = -1, FPL_MUS_STAGE[FPL_STAGES], FPL_MUS_BOSS, FPL_MUS_QUEEN, FPL_MUS_BONUS, FPL_MUS_RADIO,
    FPL_MUS_GRADE, FPL_MUS_PERFECT, FPL_MUS_OVER, FPL_MUS_ENDING, FPL_MUS_TALLY;

/* a bar of eighths on one note */
#define EI(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
/* a bar of octave-jumping eighths */
#define OCT(lo, hi) "[" lo "8 " hi "8]4 "
/* a bar of sixteenth arpeggio on four notes */
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
/* a bar of off-beat chord stabs */
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
/* half a bar of kick, hat, snare, hat */
#define KH "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
/* a bar of backbeat */
#define BEAT "@13 v12 o2 c4 @11 v10 o6 c4 @13 v12 o2 c8 @13 v9 o2 c8 @11 v10 o6 c4 "
/* a bar of sixteenth hats with kick and snare */
#define BUSY "@13 v12 o2 c16 @9 v4 o8 c16 c16 c16 @11 v10 o6 c16 @9 v4 o8 c16 c16 c16 " \
             "@13 v12 o2 c16 @9 v4 o8 c16 @13 v10 o2 c16 @9 v4 o8 c16 @11 v10 o6 c16 @9 v4 o8 c16 @11 v7 o6 c16 c16 "

/* "Full Peal" - the title, D major: rounds, queens, tittums */
static const char TITLE_LEAD[] =
    "@15 v12 q6"
    "| o6 d8 c+8 o5 b8 a8 g8 f+8 e8 d8 | o6 d8 o5 b8 g8 e8 o6 c+8 o5 a8 f+8 d8"
    "| o6 d8 o5 g8 o6 c+8 o5 f+8 b8 e8 a8 d8 | o5 a4 d4 f+4 a4"
    "| o6 d8 c+8 o5 b8 a8 g8 f+8 e8 d8 | o5 e8 g8 b8 o6 d8 c+8 o5 a8 f+8 a8"
    "| o5 g8 b8 o6 d8 o5 b8 a8 o6 c+8 e8 c+8 | o6 d2. r4";
static const char TITLE_COMP[] =
    "v6 q3 " OFF("@16 o4", "d") OFF("@16 o4", "g") OFF("@16 o4", "a") OFF("@16 o4", "d")
    OFF("@16 o4", "d") OFF("@17 o4", "e") OFF("@16 o4", "a") OFF("@16 o4", "d");
static const char TITLE_BASS[] =
    "@6 v14 q5 " EI("o2 d") EI("o2 g") EI("o2 a") EI("o2 d") EI("o2 d") EI("o2 e") EI("o2 a") EI("o2 d");
static const char TITLE_DRUM[] = "[" KH KH "]8";

/* "Outer Belfry" - stage A, G major, setting out */
static const char SA_LEAD[] =
    "@1 v11 q6"
    "| o5 g8 b8 o6 d8 o5 b8 o6 c4 o5 a4 | o5 b8 o6 d8 g8 d8 e4 d4"
    "| o6 c8 o5 b8 a8 g8 f+8 g8 a8 f+8 | o5 g2 d4 r4"
    "| o5 g8 b8 o6 d8 g8 f+4 e4 | o6 e8 d8 c8 o5 b8 a4 o6 c4"
    "| o5 b8 a8 g8 f+8 e8 f+8 g8 a8 | o5 g2. r4";
static const char SA_COMP[] =
    "v6 q3 " OFF("@16 o4", "g") OFF("@16 o4", "d") OFF("@16 o4", "c") OFF("@16 o4", "g")
    OFF("@16 o4", "g") OFF("@17 o4", "a") OFF("@16 o4", "d") OFF("@16 o4", "g");
static const char SA_BASS[] =
    "@6 v14 q5 " EI("o2 g") EI("o2 d") EI("o2 c") EI("o2 g") EI("o2 g") EI("o2 a") EI("o2 d") EI("o2 g");
static const char SA_DRUM[] = "[" KH KH "]8";

/* "Tin Nebula" - stage B, E minor, drifting through foil clouds */
static const char SB_LEAD[] =
    "@14 v11 q7"
    "| o5 e4 g4 b4 o6 e4 | o6 d4 c8 o5 b8 a2 | o5 g4 b4 o6 d4 c4 | o5 b2 f+2"
    "| o5 e4 g4 b4 o6 e4 | o6 f+4 e8 d8 c2 | o5 b4 a8 g8 f+4 a4 | o5 e1";
static const char SB_ARP[] =
    "@2 v5 q5 " ARP("o4 e", "g", "b", "g") ARP("o4 a", "o5 c", "e", "c") ARP("o4 g", "b", "o5 d", "o4 b")
    ARP("o4 b", "o5 d+", "f+", "d+") ARP("o4 e", "g", "b", "g") ARP("o4 c", "e", "g", "e")
    ARP("o4 b", "o5 d+", "f+", "d+") ARP("o4 e", "g", "b", "g");
static const char SB_BASS[] =
    "@6 v14 q4 " OCT("o2 e", "o3 e") OCT("o2 a", "o3 a") OCT("o2 g", "o3 g") OCT("o2 b", "o3 b")
    OCT("o2 e", "o3 e") OCT("o2 c", "o3 c") OCT("o2 b", "o3 b") OCT("o2 e", "o3 e");
static const char SB_DRUM[] = "[" BEAT "]8";

/* "The Murk" - stage C, C minor, something growing out there */
static const char SC_LEAD[] =
    "@5 v11 q7"
    "| o5 c2 e-4 g4 | o5 a-2. g4 | o5 f2 a-4 o6 c4 | o5 b2 g2"
    "| o5 c2 e-4 g4 | o6 c2 o5 b-4 a-4 | o5 g4 f4 e-4 d4 | o5 c1";
static const char SC_PLUCK[] =
    "@3 v6 q3 [o4 c8 g8 e-8 g8]2 [o4 c8 a-8 e-8 a-8]2 [o4 c8 a-8 f8 a-8]2 [o4 d8 g8 b8 g8]2"
    " [o4 c8 g8 e-8 g8]2 [o4 c8 a-8 e-8 a-8]2 [o4 d8 g8 b8 g8]2 [o4 c8 g8 e-8 g8]2";
static const char SC_BASS[] =
    "@6 v14 q6 o2 c2 c2 o1 a-2 a-2 o2 f2 f2 o1 g2 g2 o2 c2 c2 o1 a-2 a-2 o1 g2 g2 o2 c2 c2";
static const char SC_DRUM[] = "[@13 v11 o2 c4 @9 v4 o8 c8 c8 @11 v9 o6 c4 @9 v4 o8 c4]8";

/* "Tollgate" - stage D, A minor, the run at the gate */
static const char SD_LEAD[] =
    "@0 v11 q5 ["
    "| o5 a8 a8 o6 c8 o5 a8 o6 e8 o5 a8 o6 d8 c8 | o5 b8 b8 o6 d8 o5 b8 o6 f8 e8 d8 o5 b8"
    "| o5 a8 a8 o6 c8 o5 a8 o6 e8 o5 a8 o6 g8 e8 | o6 f8 e8 d8 c8 o5 b4 g+4 ]2";
static const char SD_STAB[] =
    "v6 q2 [" OFF("@17 o4", "a") OFF("@16 o4", "g") OFF("@17 o4", "a") OFF("@16 o4", "e") "]2";
static const char SD_BASS[] = "@6 v14 q4 [" EI("o2 a") EI("o2 g") EI("o2 a") EI("o2 e") "]2";
static const char SD_DRUM[] = "[" BUSY "]8";

/* "Knell" - stage E, D minor, the bell-planet itself */
static const char SE_LEAD[] =
    "@23 v11 q6"
    "| o5 d4 f4 a4 o6 d4 | o6 c+2 o5 a2 | o5 b-4 a4 g4 f4 | o5 e2 a2"
    "| o5 d4 f4 a4 o6 f4 | o6 e4 d4 c+4 o5 a4 | o5 b-4 g4 e4 c+4 | o5 d1";
static const char SE_TOLL[] = "[@15 v9 o4 d2 r2 o3 a2 r2]4";
static const char SE_BASS[] =
    "@6 v14 q4 " OCT("o2 d", "o3 d") OCT("o2 a", "o3 a") OCT("o2 g", "o3 g") OCT("o2 a", "o3 a")
    OCT("o2 d", "o3 d") OCT("o2 a", "o3 a") OCT("o2 g", "o3 g") OCT("o2 d", "o3 d");
static const char SE_DRUM[] = "[" BEAT "]8";

/* "Big Bells" - the bosses of stages A to D, F minor */
static const char BOSS_LEAD[] =
    "@0 v11 q5 ["
    "| o5 f8 f8 a-8 f8 o6 c8 o5 f8 b-8 a-8 | o5 f8 f8 a-8 f8 o6 d-8 c8 o5 b-8 a-8"
    "| o5 e-8 e-8 g8 e-8 b-8 a-8 g8 f8 | o5 g8 f8 e8 f8 g4 c4 ]2";
static const char BOSS_STAB[] =
    "v6 q2 [" OFF("@17 o4", "f") OFF("@16 o4", "d-") OFF("@16 o4", "e-") OFF("@16 o4", "c") "]2";
static const char BOSS_BASS[] = "@6 v14 q4 [" EI("o2 f") EI("o2 d-") EI("o2 e-") EI("o2 c") "]2";
static const char BOSS_DRUM[] = "[" BUSY "]8";

/* "Queen Sordina" - the last boss, B-flat minor */
static const char QUEEN_LEAD[] =
    "@1 v11 q6"
    "| o5 b-8 o6 d-8 f8 b-8 ^4 a8 f8 | o6 g-8 b-8 o7 d-8 o6 b-8 g-4 f4"
    "| o6 e-8 g-8 b-8 g-8 e-4 d-4 | o6 c8 d-8 e-8 f8 a4 c4"
    "| o5 b-8 o6 d-8 f8 b-8 ^4 o7 c8 d-8 | o7 c8 o6 b-8 a8 g-8 f4 e-4"
    "| o6 d-8 e-8 f8 g-8 f4 e-4 | o6 f2 a4 o7 c4";
static const char QUEEN_ARP[] =
    "@2 v5 q5 " ARP("o4 b-", "o5 d-", "f", "d-") ARP("o4 g-", "b-", "o5 d-", "o4 b-") ARP("o4 e-", "g-", "b-", "g-")
    ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 b-", "o5 d-", "f", "d-") ARP("o4 g-", "b-", "o5 d-", "o4 b-")
    ARP("o4 e-", "g-", "b-", "g-") ARP("o4 f", "a", "o5 c", "o4 a");
static const char QUEEN_BASS[] =
    "@6 v14 q4 " OCT("o1 b-", "o2 b-") OCT("o1 g-", "o2 g-") OCT("o1 e-", "o2 e-") OCT("o1 f", "o2 f")
    OCT("o1 b-", "o2 b-") OCT("o1 g-", "o2 g-") OCT("o1 e-", "o2 e-") OCT("o1 f", "o2 f");
static const char QUEEN_DRUM[] = "[" BUSY "]8";

/* "Balloon Round" - C major, bouncing */
static const char BON_LEAD[] =
    "@15 v11 q5"
    "| o5 c8 e8 g8 o6 c8 o5 g8 e8 c4 | o5 d8 f8 a8 o6 d8 o5 a8 f8 d4"
    "| o5 e8 g8 b8 o6 e8 o5 b8 g8 e4 | o5 f8 a8 o6 c8 f8 e8 d8 c4"
    "| o5 c8 e8 g8 o6 c8 o5 g8 e8 c4 | o5 a8 o6 c8 e8 a8 g8 e8 c4"
    "| o5 f8 a8 o6 c8 o5 a8 g8 b8 o6 d4 | o6 c2 r2";
static const char BON_ARP[] =
    "@3 v6 q4 " ARP("o4 c", "e", "g", "e") ARP("o4 d", "f", "a", "f") ARP("o4 e", "g", "b", "g")
    ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 c", "e", "g", "e") ARP("o4 a", "o5 c", "e", "c")
    ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 c", "e", "g", "e");
static const char BON_BASS[] =
    "@7 v14 q4 " OCT("o2 c", "o3 c") OCT("o2 d", "o3 d") OCT("o2 e", "o3 e") OCT("o2 f", "o3 f")
    OCT("o2 c", "o3 c") OCT("o2 a", "o3 a") OCT("o2 f", "o3 f") OCT("o2 c", "o3 c");
static const char BON_DRUM[] = "[" KH KH "]8";

/* "Ring Out" - the ending, D major */
static const char END_LEAD[] =
    "@14 v10 q7"
    "| o5 f+2 a4 o6 d4 | o6 c+2 o5 a2 | o5 b2 o6 d4 c+4 | o5 a1"
    "| o5 g2 b4 o6 e4 | o6 d2 c+4 o5 b4 | o5 a4 g4 f+4 e4 | o5 d1";
static const char END_ARP[] =
    "@4 v5 q6 " ARP("o4 d", "f+", "a", "f+") ARP("o4 a", "o5 c+", "e", "c+") ARP("o4 b", "o5 d", "f+", "d")
    ARP("o4 a", "o5 c+", "e", "c+") ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 d", "f+", "a", "f+")
    ARP("o4 a", "o5 c+", "e", "c+") ARP("o4 d", "f+", "a", "f+");
static const char END_BASS[] = "@6 v13 q7 o2 d2 d2 o1 a2 a2 o1 b2 b2 o1 a2 a2 o1 g2 g2 o2 d2 d2 o1 a2 a2 o2 d2 d2";
static const char END_SOFT[] = "[@9 v3 o8 c8]64";

/* "The Final Count" - G major */
static const char TAL_LEAD[] =
    "@15 v10 q6"
    "| o5 g4 b4 o6 d4 g4 | o6 f+4 d4 o5 a4 f+4 | o5 e4 g4 b4 o6 e4 | o6 d1"
    "| o5 c4 e4 g4 o6 c4 | o5 b4 d4 g4 b4 | o5 a4 o6 c4 o5 f+4 a4 | o5 g1";
static const char TAL_COMP[] =
    "v6 q3 " OFF("@16 o4", "g") OFF("@16 o4", "d") OFF("@17 o4", "e") OFF("@16 o4", "d")
    OFF("@16 o4", "c") OFF("@16 o4", "g") OFF("@16 o4", "d") OFF("@16 o4", "g");
static const char TAL_BASS[] =
    "@6 v13 q5 " EI("o2 g") EI("o2 d") EI("o2 e") EI("o2 d") EI("o2 c") EI("o2 g") EI("o2 d") EI("o2 g");
static const char TAL_DRUM[] = "[@13 v10 o2 c4 @11 v8 o6 c4]16";

/* jingles */
static const char RADIO_P1[] = "@39 v10 o5 l16 a o6 d f+ a r8 o5 a o6 d f+ a r8 r4 @15 v9 o6 d4 o5 a4 o6 d2";
static const char RADIO_TRI[] = "@6 v12 o3 d4 r4 d4 r4 d4 a4 d2";
static const char RADIO_NOISE[] = "@21 v4 o7 [c16 r16]8 r2 @9 v3 o8 [c8]8";
static const char GRADE_P1[] = "@1 v12 o5 l16 d f+ a o6 d4 r8 o5 a8 o6 d4";
static const char GRADE_TRI[] = "@6 v12 o3 d8 r8 a8 r8 d4";
static const char PERF_P1[] = "@39 v12 o6 l16 d f+ a o7 d o6 f+ a o7 d f+ a4 r8 o6 a8 o7 d2";
static const char PERF_P2[] = "@15 v9 o5 l16 a o6 d f+ a o5 a o6 d f+ a f+4 r8 d8 f+2";
static const char PERF_TRI[] = "@6 v13 o3 d8 a8 d8 a8 d4 r8 a8 d2";
static const char PERF_NOISE[] = "@13 v12 o2 c8 r8 @11 v10 o6 c8 r8 @12 v11 o5 c2";
static const char OVER_P1[] = "@0 v11 o5 a8 g8 f8 e8 d4 c+4 d2";
static const char OVER_TRI[] = "@6 v12 o2 d4 a4 d4 a4 d2";

void fpl_audio_load(void) {
    if (FPL_MUS_TITLE >= 0) return;
    FPL_MUS_TITLE = song_define("fpl_title", 132, true, TITLE_LEAD, TITLE_COMP, TITLE_BASS, TITLE_DRUM);
    FPL_MUS_STAGE[0] = song_define("fpl_belfry", 140, true, SA_LEAD, SA_COMP, SA_BASS, SA_DRUM);
    FPL_MUS_STAGE[1] = song_define("fpl_nebula", 136, true, SB_LEAD, SB_ARP, SB_BASS, SB_DRUM);
    FPL_MUS_STAGE[2] = song_define("fpl_murk", 120, true, SC_LEAD, SC_PLUCK, SC_BASS, SC_DRUM);
    FPL_MUS_STAGE[3] = song_define("fpl_tollgate", 150, true, SD_LEAD, SD_STAB, SD_BASS, SD_DRUM);
    FPL_MUS_STAGE[4] = song_define("fpl_knell", 144, true, SE_LEAD, SE_TOLL, SE_BASS, SE_DRUM);
    FPL_MUS_BOSS = song_define("fpl_boss", 156, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    FPL_MUS_QUEEN = song_define("fpl_queen", 150, true, QUEEN_LEAD, QUEEN_ARP, QUEEN_BASS, QUEEN_DRUM);
    FPL_MUS_BONUS = song_define("fpl_balloons", 160, true, BON_LEAD, BON_ARP, BON_BASS, BON_DRUM);
    FPL_MUS_ENDING = song_define("fpl_ending", 92, true, END_LEAD, END_ARP, END_BASS, END_SOFT);
    FPL_MUS_TALLY = song_define("fpl_tally", 110, true, TAL_LEAD, TAL_COMP, TAL_BASS, TAL_DRUM);
    FPL_MUS_RADIO = song_define("fpl_radio", 120, false, RADIO_P1, "", RADIO_TRI, RADIO_NOISE);
    FPL_MUS_GRADE = song_define("fpl_grade", 150, false, GRADE_P1, "", GRADE_TRI, "");
    FPL_MUS_PERFECT = song_define("fpl_perfect", 150, false, PERF_P1, PERF_P2, PERF_TRI, PERF_NOISE);
    FPL_MUS_OVER = song_define("fpl_over", 100, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("fpl_shot", CH_P2, 240, "@20 v4 o6 a32");
    sfx_define("fpl_side", CH_P2, 240, "@20 v4 o5 e32 a32");
    sfx_define("fpl_pop", CH_NOISE, 200, "@34 v9 o5 c8");
    sfx_define("fpl_pop3", CH_P1, 220, "@39 v11 o6 l32 c e g o7 c");
    sfx_define("fpl_tick", CH_NOISE, 240, "@21 v6 o7 c32");
    sfx_define("fpl_tink", CH_P2, 240, "@15 v6 o7 c32");
    sfx_define("fpl_crash", CH_P1, 160, "@33 v13 o5 c8 o4 g8 e8 c4");
    sfx_define("fpl_efire", CH_NOISE, 240, "@36 v5 o7 c32");
    sfx_define("fpl_bong", CH_P1, 120, "@15 v12 o3 c2");
    sfx_define("fpl_bhit", CH_NOISE, 240, "@21 v8 o5 c32");
    sfx_define("fpl_angry", CH_P1, 160, "@37 v11 o3 c8 o2 g8");
    sfx_define("fpl_bigboom", CH_NOISE, 100, "@34 v15 o3 c1");
    sfx_define("fpl_boom", CH_NOISE, 160, "@34 v12 o4 c4");
    sfx_define("fpl_clap", CH_NOISE, 200, "@40 v12 o4 c8");
    sfx_define("fpl_launch", CH_NOISE, 200, "@36 v8 o5 c8");
    sfx_define("fpl_heal", CH_P1, 200, "@38 v10 o4 l16 c e g o5 c");
    sfx_define("fpl_mouth", CH_P2, 180, "@33 v9 o4 c8");
    sfx_define("fpl_spit", CH_NOISE, 200, "@36 v10 o6 c8");
    sfx_define("fpl_clary", CH_P1, 200, "@39 v12 o5 l16 e g o6 c e");
    sfx_define("fpl_extra", CH_P1, 200, "@39 v13 o6 l16 c e g o7 c4");
    sfx_define("fpl_start", CH_P1, 200, "@32 v12 o4 c8 g8 o5 c4");
    /* Clary's console */
    sfx_define("fpm_blip", CH_P2, 240, "@42 v7 o6 c32");
    sfx_define("fpm_point", CH_P2, 240, "@35 v8 o6 e16");
    sfx_define("fpm_over", CH_P2, 180, "@33 v10 o4 c8 o3 g8");
    sfx_define("fpm_open", CH_P2, 220, "@39 v9 o5 l32 c e g o6 c");
}
