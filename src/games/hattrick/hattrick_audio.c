/* HAT TRICK - original music (UFO-MML) and sound effects. Four tunes, as
 * the original has (a short opening, the match, the boss, the ending), and
 * three jingles. */
#include "hattrick.h"

int HTK_MUS_WHISTLE = -1, HTK_MUS_MATCH, HTK_MUS_FINAL, HTK_MUS_TROPHY, HTK_MUS_CLEAR, HTK_MUS_OVER,
    HTK_MUS_SHOOTOUT, HTK_MUS_TITLE;

/* a bar of bass: root and octave, eighths */
#define OCT(n) "[o2 " n "8 o3 " n "8]4 "
/* a bar of off-beat chord stabs */
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
/* a bar of sixteenth stabs */
#define ST16(n) "[" n "16 r16]8 "
#define BEAT "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
#define BEAT16 "@13 v12 o2 c16 @9 v5 o8 c16 @11 v10 o6 c16 @9 v5 o8 c16 "

/* "Whistle Blows" - the opening, a referee's whistle and a fanfare */
static const char WHISTLE_P1[] = "@20 v10 o7 c16 r16 c4 r8 @1 v12 o5 g8 o6 c8 e8 g4 e8 g8 o7 c2";
static const char WHISTLE_P2[] = "r2 @2 v7 o5 e8 g8 o6 c8 e4 c8 e8 g2";
static const char WHISTLE_TRI[] = "r2 @6 v14 o3 c8 r8 c8 r8 o2 g8 r8 o3 c2";
static const char WHISTLE_NOISE[] = "r2 @13 v12 o2 c8 @11 v10 o6 c8 @13 v12 o2 c8 @11 v10 o6 c8 @11 c16 c16 c16 c16 @12 v12 o5 c2";

/* "Saturday League" - the match, C major, 16 bars */
static const char MATCH_LEAD[] =
    "@1 v11 q6"
    "| o5 c8 e8 g8 e8 a4 g8 e8 | o5 f8 a8 g8 f8 e4 c4"
    "| o5 d8 f8 a8 f8 b4 a8 f8 | o5 g8 a8 g8 f8 e4 g4"
    "| o5 c8 e8 g8 e8 o6 c4 o5 b8 a8 | o5 a8 o6 c8 o5 b8 a8 g4 e4"
    "| o5 f8 e8 d8 e8 f8 g8 a8 b8 | o6 c2 r4 o5 g4"
    "| o5 a8 a8 g8 a8 o6 c4 o5 a4 | o5 g8 g8 e8 g8 o6 c4 o5 g4"
    "| o5 f8 f8 e8 f8 a4 f4 | o5 e8 e8 d8 e8 g4 r4"
    "| o5 a8 b8 o6 c8 d8 e4 d8 c8 | o5 b8 o6 c8 d8 o5 b8 g4 e4"
    "| o5 f8 a8 g8 f8 e8 g8 f8 d8 | o5 c2 r2";
static const char MATCH_OFF[] =
    "v6 q3 " OFF("@16 o4", "c") OFF("@16 o4", "f") OFF("@17 o4", "d") OFF("@16 o4", "g")
    OFF("@16 o4", "c") OFF("@17 o4", "a") OFF("@16 o4", "f") OFF("@16 o4", "c")
    OFF("@16 o4", "f") OFF("@16 o4", "c") OFF("@17 o4", "d") OFF("@16 o4", "c")
    OFF("@17 o4", "a") OFF("@16 o4", "g") OFF("@16 o4", "f") OFF("@16 o4", "c");
static const char MATCH_BASS[] =
    "@6 v14 q4 " OCT("c") OCT("f") OCT("d") OCT("g") OCT("c") OCT("a") OCT("f") OCT("c")
    OCT("f") OCT("c") OCT("d") OCT("c") OCT("a") OCT("g") OCT("f") OCT("c");
static const char MATCH_DRUM[] = "[" BEAT BEAT "]16";

/* "Cup Final" - the bosses, A minor */
static const char FINAL_LEAD[] =
    "@1 v12 q5"
    "| o5 a8 a8 o6 c8 o5 a8 e8 a8 g+8 a8 | o5 a8 a8 o6 d8 c8 o5 b8 a8 g8 e8"
    "| o5 f8 f8 a8 f8 o6 c8 o5 f8 e8 f8 | o5 e8 g+8 b8 o6 e8 d8 c8 o5 b8 g+8"
    "| o5 a8 a8 o6 c8 o5 a8 e8 a8 g+8 a8 | o6 c8 d8 e8 c8 o5 a8 o6 c8 o5 b8 a8"
    "| o5 f8 a8 o6 c8 d8 e8 d8 c8 o5 b8 | o5 a4 e4 a2";
static const char FINAL_STAB[] =
    "v6 q2 @17 o4 " ST16("a") ST16("a") "@16 o4 " ST16("f") "@16 o4 " ST16("e") "@17 o4 " ST16("a") ST16("a")
    "@16 o4 " ST16("f") "@17 o4 " ST16("a");
static const char FINAL_BASS[] =
    "@6 v14 q4 " OCT("a") OCT("a") OCT("f") OCT("e") OCT("a") OCT("a") OCT("f") OCT("a");
static const char FINAL_DRUM[] = "[" BEAT16 BEAT16 BEAT16 BEAT16 "]8";

/* "Lap of Honour" - the ending, F major */
static const char TROPHY_LEAD[] =
    "@14 v11 q7"
    "| o5 f4 a4 o6 c4 o5 a4 | o5 b-4 o6 d4 c2 | o5 a4 g4 f4 a4 | o5 g2 c2"
    "| o5 f4 a4 o6 c4 f4 | o6 e4 d4 c4 o5 b-4 | o5 a4 g4 f4 g4 | o5 f2. r4";
static const char TROPHY_ARP[] =
    "@2 v5 q5 [o4 f8 a8 o5 c8 o4 a8]2 [o4 b-8 o5 d8 f8 d8]2 [o4 f8 a8 o5 c8 o4 a8]2 [o4 g8 b-8 o5 c8 o4 b-8]2"
    " [o4 f8 a8 o5 c8 o4 a8]2 [o4 c8 e8 g8 e8]2 [o4 b-8 o5 d8 f8 d8]1 [o4 g8 b-8 o5 c8 o4 b-8]1 [o4 f8 a8 o5 c8 o4 a8]2";
static const char TROPHY_BASS[] = "@6 v13 q6 o2 f2 c2 b-2 f2 f2 d2 c2 c2 f2 c2 c2 o1 b-2 b-2 o2 c2 f2 f2";
static const char TROPHY_DRUM[] = "[@13 v10 o2 c4 @11 v8 o6 c4 @13 v9 o2 c8 c8 @11 v8 o6 c4]8";

/* "Pitch Cleared" and "Final Whistle" - the jingles */
static const char CLEAR_P1[] = "@1 v12 o5 l16 c e g o6 c8 o5 g8 o6 e8 d8 c4";
static const char CLEAR_TRI[] = "@6 v14 o3 l16 c r c r o2 g8 b8 o3 c4";
static const char CLEAR_NOISE[] = "@11 v9 o6 c16 c16 c16 c16 c8 c8 @12 v10 o5 c4";
static const char OVER_P1[] = "@20 v10 o7 c4 r8 c2 @14 v11 o5 e4 d4 c4 o4 b8 a8 g2";
static const char OVER_TRI[] = "r2 r4 r8 @6 v13 o3 c4 o2 g4 e4 d8 c8 o1 g2";

/* "Shootout" - the versus pitch, D major, 8 bars */
static const char SHOOT_LEAD[] =
    "@0 v11 q5"
    "| o5 d8 f+8 a8 f+8 o6 d4 o5 a4 | o5 b8 a8 g8 f+8 e4 a4 | o5 g8 b8 o6 d8 o5 b8 o6 e4 d4"
    "| o6 c+8 o5 a8 b8 o6 c+8 d4 o5 a4 | o5 d8 f+8 a8 f+8 o6 d4 f+4 | o6 e8 d8 c+8 o5 b8 a4 f+4"
    "| o5 g8 a8 b8 o6 c+8 d8 e8 f+8 e8 | o6 d2 r2";
static const char SHOOT_OFF[] =
    "v6 q3 " OFF("@16 o4", "d") OFF("@16 o4", "a") OFF("@16 o4", "g") OFF("@16 o4", "a")
    OFF("@16 o4", "d") OFF("@16 o4", "a") OFF("@16 o4", "g") OFF("@16 o4", "d");
static const char SHOOT_BASS[] =
    "@6 v14 q4 " OCT("d") OCT("a") OCT("g") OCT("a") OCT("d") OCT("a") OCT("g") OCT("d");
static const char SHOOT_DRUM[] = "[" BEAT BEAT "]8";

void htk_audio_load(void) {
    if (HTK_MUS_WHISTLE >= 0) return;
    HTK_MUS_WHISTLE = song_define("htk_whistle", 150, false, WHISTLE_P1, WHISTLE_P2, WHISTLE_TRI, WHISTLE_NOISE);
    HTK_MUS_MATCH = song_define("htk_match", 152, true, MATCH_LEAD, MATCH_OFF, MATCH_BASS, MATCH_DRUM);
    HTK_MUS_FINAL = song_define("htk_final", 164, true, FINAL_LEAD, FINAL_STAB, FINAL_BASS, FINAL_DRUM);
    HTK_MUS_TROPHY = song_define("htk_trophy", 112, true, TROPHY_LEAD, TROPHY_ARP, TROPHY_BASS, TROPHY_DRUM);
    HTK_MUS_CLEAR = song_define("htk_clear", 150, false, CLEAR_P1, "", CLEAR_TRI, CLEAR_NOISE);
    HTK_MUS_OVER = song_define("htk_over", 120, false, OVER_P1, "", OVER_TRI, "");
    HTK_MUS_SHOOTOUT = song_define("htk_shootout", 144, true, SHOOT_LEAD, SHOOT_OFF, SHOOT_BASS, SHOOT_DRUM);
    HTK_MUS_TITLE = HTK_MUS_WHISTLE;

    sfx_define("htk_jump", CH_P1, 240, "@32 v9 o4 c16 g16");
    sfx_define("htk_kick", CH_NOISE, 240, "@13 v13 o3 c16 @36 v8 o6 c32");
    sfx_define("htk_drive", CH_NOISE, 200, "@13 v15 o2 c16 @34 v10 o5 c8");
    sfx_define("htk_touch", CH_NOISE, 240, "@13 v10 o3 c16");
    sfx_define("htk_trap", CH_P2, 240, "@20 v7 o4 g32 o5 c32");
    sfx_define("htk_slide", CH_NOISE, 200, "@36 v9 o5 c8");
    sfx_define("htk_bounce", CH_NOISE, 240, "@13 v7 o3 c32");
    sfx_define("htk_bonk", CH_NOISE, 240, "@21 v7 o5 c32");
    sfx_define("htk_dark", CH_P2, 220, "@33 v8 o5 e16 o4 b16");
    sfx_define("htk_kill", CH_P1, 240, "@35 v11 o6 c16 g16");
    sfx_define("htk_kill4", CH_P1, 240, "@39 v12 o6 c16 e16 g16 o7 c16");
    sfx_define("htk_land", CH_P2, 240, "@41 v8 o3 c16");
    sfx_define("htk_eat", CH_P1, 240, "@35 v10 o6 e16 a16");
    sfx_define("htk_dessert", CH_P1, 220, "@39 v12 o6 l32 c e g o7 c e g");
    sfx_define("htk_secret", CH_P1, 200, "@39 v12 o5 l16 g o6 c e g");
    sfx_define("htk_extend", CH_P1, 180, "@39 v13 o5 l16 c e g o6 c8 e8 g4");
    sfx_define("htk_hurt", CH_P1, 160, "@33 v13 o5 c8 o4 g8 e8 c4");
    sfx_define("htk_whistle", CH_P1, 200, "@20 v11 o7 c16 r16 c16 r16 c4");
    sfx_define("htk_clear", CH_P2, 200, "@20 v10 o7 c4 r16 c8");
    sfx_define("htk_balloon", CH_P2, 200, "@38 v9 o5 c16 e16 g16 o6 c8");
    sfx_define("htk_throw", CH_NOISE, 220, "@36 v6 o6 c16");
    sfx_define("htk_bosshit", CH_P2, 220, "@37 v12 o4 c16 o3 g16");
    sfx_define("htk_bossdown", CH_NOISE, 120, "@34 v15 o3 c2");
    sfx_define("htk_bosshop", CH_P2, 200, "@41 v11 o2 c8");
    sfx_define("htk_goal", CH_P1, 180, "@39 v13 o5 l16 c e g o6 c e g o7 c4");
}
