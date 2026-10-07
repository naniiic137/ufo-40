/* RESTLESS - original music (UFO-MML) and sound effects. Every looping
 * tune is eight (or sixteen) bars a channel, so the channels loop
 * together. Mostly modal: drones under a singing lead. */
#include "rsl.h"

int RSL_MUS_TITLE = -1, RSL_MUS_STORY, RSL_MUS_S1A, RSL_MUS_S1B, RSL_MUS_S2A, RSL_MUS_S2B, RSL_MUS_S3A,
    RSL_MUS_S3B, RSL_MUS_MIDBOSS, RSL_MUS_BOSS, RSL_MUS_CLEAR, RSL_MUS_SPECIAL, RSL_MUS_SPIRIT, RSL_MUS_OVER,
    RSL_MUS_ENDING, RSL_MUS_RISE, RSL_MUS_SCORES;

/* a bar of four quarter notes; a bar of eighths on one note; a bar of a
 * root-fifth drone in halves */
#define Q4(a, b, c, d) a "4 " b "4 " c "4 " d "4 "
#define E8(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define DR(a, b) a "2 " b "2 "
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
/* half a bar of soft hand drums */
#define HAND "@13 v9 o2 c8 @9 v3 o8 c8 @11 v6 o6 c8 @9 v3 o8 c8 "
#define HAND2 "@13 v10 o2 c8 @13 v7 o2 c8 @11 v7 o6 c8 @9 v4 o8 c16 c16 "
#define BIRD "@9 v2 o8 c4 @20 v3 o7 e32 g32 r16 r8 r4 @9 v2 o8 c4 "

/* "Restless" - the title, D with a raised third over a flat second */
static const char TITLE_LEAD[] =
    "@14 v11 q7"
    "| o5 d4 e-8 f+8 g4 f+8 e-8 | o5 d2 r4 a4 | o5 b-4 a8 g8 f+4 e-8 f+8 | o5 g2 f+4 r4"
    "| o5 d4 e-8 f+8 a4 g8 f+8 | o5 b-4 a8 b-8 o6 c4 o5 b-4 | o5 a8 g8 f+8 e-8 f+4 e-4 | o5 d1";
static const char TITLE_DRONE[] =
    "@4 v5 q8 " DR("o4 d", "o4 a") DR("o4 d", "o4 a") DR("o4 e-", "o4 b-") DR("o4 d", "o4 a")
    DR("o4 d", "o4 a") DR("o4 g", "o4 b-") DR("o4 e-", "o4 a") DR("o4 d", "o4 a");
static const char TITLE_BASS[] =
    "@6 v12 q6 " Q4("o2 d", "a", "o3 d", "o2 a") Q4("o2 d", "a", "o3 d", "o2 a") Q4("o2 e-", "b-", "o3 e-", "o2 b-")
    Q4("o2 d", "a", "o3 d", "o2 a") Q4("o2 d", "a", "o3 d", "o2 a") Q4("o2 g", "o3 d", "g", "d")
    Q4("o2 e-", "b-", "o3 e-", "o2 b-") Q4("o2 d", "a", "o3 d", "o2 a");
static const char TITLE_DRUM[] = "[" HAND HAND "]8";

/* "Mossfold Burns" - the story, D minor lament */
static const char STORY_LEAD[] =
    "@5 v10 q7"
    "| o5 a2 g4 f4 | o5 e2. d4 | o5 f4 e4 d4 c+4 | o5 d1"
    "| o5 d4 f4 a4 o6 d4 | o6 c2 o5 b-4 a4 | o5 g4 f4 e4 c+4 | o5 d1";
static const char STORY_PAD[] =
    "@4 v5 q8 o4 [d1]2 [b-1]1 [a1]1 [d1]1 [f1]1 [g1]1 [a1]1";
static const char STORY_BASS[] = "@6 v11 q8 o2 d1 d1 g1 d1 d1 f1 a1 d1";
static const char STORY_SOFT[] = "[@9 v2 o8 c2]16";

/* "Greenwood Gate" - stage 1A: no tune yet, only birds and leaves over a
 * low hum; the music proper starts at the first fight */
static const char S1A_LEAD[] =
    "@20 v3 [r2 o7 e32 g32 e32 g32 r8 r4 | r1 | r4 o7 c32 r32 e32 r32 r8 r2 | r1]2";
static const char S1A_ECHO[] = "@4 v2 q8 [o3 a1 o3 e1]4";
static const char S1A_BASS[] = "";
static const char S1A_DRUM[] = "[" BIRD "]8";

/* "The Old Shrines" - stage 1B, E with a flat second */
static const char S1B_LEAD[] =
    "@14 v10 q7"
    "| o5 e4 f8 g+8 a4 g+8 f8 | o5 e2 b4 a4 | o5 g+4 a8 b8 o6 c4 o5 b8 a8 | o5 g+2 e2"
    "| o5 e4 f8 e8 g+4 a4 | o5 b4 o6 c8 d8 e4 d4 | o6 c8 o5 b8 a8 g+8 f4 g+4 | o5 e1";
static const char S1B_ARP[] =
    "@2 v5 q4 " ARP("o4 e", "g+", "b", "g+") ARP("o4 e", "g+", "b", "g+") ARP("o4 f", "a", "o5 c", "o4 a")
    ARP("o4 e", "g+", "b", "g+") ARP("o4 e", "g+", "b", "g+") ARP("o4 g+", "b", "o5 e", "o4 b")
    ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 e", "g+", "b", "g+");
static const char S1B_BASS[] =
    "@6 v12 q7 " DR("o2 e", "o2 b") DR("o2 e", "o2 b") DR("o2 f", "o3 c") DR("o2 e", "o2 b")
    DR("o2 e", "o2 b") DR("o2 g+", "o2 b") DR("o2 f", "o3 c") DR("o2 e", "o2 b");
static const char S1B_DRUM[] = "[" HAND HAND2 "]8";

/* "Toad Warrens" - stage 2A, C minor, dripping */
static const char S2A_LEAD[] =
    "@19 v11 q6"
    "| o4 c4 r8 e-8 g4 r8 f8 | o4 e-4 d4 c2 | o4 g4 r8 a-8 b4 r8 a-8 | o4 g2 r2"
    "| o4 c4 r8 e-8 g4 a-8 g8 | o4 f4 e-8 f8 g4 o5 c4 | o4 b4 a-8 g8 f4 d4 | o4 c2 r2";
static const char S2A_DRIP[] =
    "@15 v4 q4 [o6 g8 r4. r2]2 [o6 e-8 r4. r2]2 [o6 g8 r4. r2]2 [o6 c8 r4. r8 o6 g8 r4]2";
static const char S2A_BASS[] =
    "@6 v13 q5 " E8("o2 c") E8("o2 c") E8("o1 a-") E8("o1 g") E8("o2 c") E8("o1 f") E8("o1 g") E8("o2 c");
static const char S2A_DRUM[] = "[@13 v10 o2 c4 @9 v2 o8 c4 @11 v5 o6 c4 @9 v2 o8 c8 @9 v4 o7 c8]8";

/* "The Sunken Court" - stage 2B, G harmonic minor, a march */
static const char S2B_LEAD[] =
    "@1 v11 q6"
    "| o5 g4. a8 b-4 a4 | o5 g8 f+8 g8 a8 d2 | o5 e-4. f+8 g4 a4 | o5 b-8 a8 g8 f+8 g2"
    "| o5 d4. e-8 f+4 g4 | o5 a4 b-8 a8 g4 f+4 | o5 e-8 d8 c8 e-8 d4 f+4 | o5 g1";
static const char S2B_COMP[] =
    "@17 v6 q3 [o4 g8 r8]4 [o4 d8 r8]4 [o4 c8 r8]4 [o4 g8 r8]4 [o4 d8 r8]4 [o4 d8 r8]4 [o4 c8 r8]4 [o4 g8 r8]4";
static const char S2B_BASS[] =
    "@6 v13 q5 " Q4("o2 g", "d", "g", "d") Q4("o2 d", "a", "d", "f+") Q4("o2 c", "g", "c", "g")
    Q4("o2 g", "d", "g", "d") Q4("o2 d", "a", "d", "a") Q4("o2 d", "f+", "a", "f+")
    Q4("o2 c", "e-", "d", "f+") Q4("o2 g", "d", "g", "d");
static const char S2B_DRUM[] = "[@13 v11 o2 c8 @11 v6 o6 c16 c16 @11 v9 o6 c8 @9 v3 o8 c8]16";

/* "The High Falls" - stage 3A, B minor, falling water */
static const char S3A_LEAD[] =
    "@14 v11 q7"
    "| o5 f+2 e4 d4 | o5 c+4 d4 e2 | o5 f+4 a4 b4 o6 c+4 | o6 d2 c+4 o5 a4"
    "| o5 b2 a4 g4 | o5 f+4 g4 a2 | o5 g4 f+4 e4 c+4 | o5 b1";
static const char S3A_ARP[] =
    "@3 v5 q4 " ARP("o5 b", "f+", "d", "o4 b") ARP("o5 a", "f+", "c+", "o4 a") ARP("o5 b", "f+", "d", "o4 b")
    ARP("o5 a", "f+", "d", "o4 a") ARP("o5 g", "e", "d", "o4 b") ARP("o5 a", "f+", "d", "o4 a")
    ARP("o5 g", "e", "c+", "o4 a") ARP("o5 b", "f+", "d", "o4 b");
static const char S3A_BASS[] =
    "@7 v12 q6 " Q4("o2 b", "f+", "b", "f+") Q4("o2 a", "e", "a", "e") Q4("o2 b", "f+", "b", "f+") Q4("o2 d", "a", "d", "a")
    Q4("o2 g", "d", "g", "d") Q4("o2 d", "a", "d", "a") Q4("o2 e", "b", "f+", "o3 c+") Q4("o2 b", "f+", "b", "f+");
static const char S3A_DRUM[] = "[@21 v3 o7 c16 c16 c16 c16 @11 v6 o6 c8 @21 v3 o7 c16 c16]16";

/* "The Hollow Crown" - stage 3B, D minor, hurried */
static const char S3B_LEAD[] =
    "@0 v11 q5"
    "| o5 d8 d8 f8 d8 a8 d8 g8 f8 | o5 e8 e8 g8 e8 o6 c8 o5 e8 b-8 a8 | o5 f8 f8 a8 f8 o6 d8 o5 f8 o6 c8 o5 b-8 | o5 a8 g8 f8 e8 c+4 e4"
    "| o5 d8 d8 f8 d8 a8 d8 o6 d8 c8 | o5 b-8 a8 g8 b-8 a8 g8 f8 a8 | o5 g8 f8 e8 g8 f8 e8 d8 c+8 | o5 d2 r2";
static const char S3B_STAB[] =
    "@16 v6 q2 [o4 d16 r16]8 [o4 c16 r16]8 [o3 b-16 r16]8 [o3 a16 r16]8 [o4 d16 r16]8 [o3 g16 r16]8 [o3 a16 r16]8 [o4 d16 r16]8";
static const char S3B_BASS[] =
    "@6 v14 q4 " E8("o2 d") E8("o2 c") E8("o1 b-") E8("o1 a") E8("o2 d") E8("o1 g") E8("o1 a") E8("o2 d");
static const char S3B_DRUM[] = "[@13 v12 o2 c8 @9 v4 o8 c8 @11 v10 o6 c8 @9 v4 o8 c16 c16]16";

/* "Something in the Way" - the first fight of each stage, E minor ostinato */
static const char MID_LEAD[] =
    "@1 v12 q5"
    "| o5 e8 g8 b8 g8 o6 c8 o5 b8 a8 g8 | o5 f+8 a8 o6 c8 o5 a8 b8 a8 g8 f+8 | o5 e8 g8 b8 o6 e8 d8 c8 o5 b8 a8 | o5 b4 a4 g4 f+4"
    "| o5 e8 g8 b8 g8 o6 c8 o5 b8 a8 g8 | o5 a8 o6 c8 e8 c8 d8 c8 o5 b8 a8 | o5 g8 f+8 e8 f+8 g8 a8 b8 o6 d+8 | o6 e2 r2";
static const char MID_ARP[] =
    "@2 v5 q4 " ARP("o4 e", "g", "b", "g") ARP("o4 d", "f+", "a", "f+") ARP("o4 c", "e", "g", "e")
    ARP("o3 b", "o4 d+", "f+", "d+") ARP("o4 e", "g", "b", "g") ARP("o4 a", "o5 c", "e", "c")
    ARP("o3 b", "o4 d+", "f+", "d+") ARP("o4 e", "g", "b", "g");
static const char MID_BASS[] =
    "@6 v14 q4 " E8("o2 e") E8("o2 d") E8("o2 c") E8("o1 b") E8("o2 e") E8("o2 a") E8("o1 b") E8("o2 e");
static const char MID_DRUM[] = "[@13 v12 o2 c8 @11 v9 o6 c8 @13 v10 o2 c8 @11 v10 o6 c8]16";

/* "The Hollow Host" - the end-of-stage fights, C with a flat second */
static const char BOSS_LEAD[] =
    "@0 v12 q5"
    "| o4 c8 c8 o5 c8 o4 c8 d-8 o5 c8 e8 f8 | o5 g8 g8 f8 e8 f8 e8 d-8 o4 b-8 | o4 a-8 a-8 o5 a-8 o4 a-8 g8 o5 a-8 g8 f8 | o5 e8 d-8 c8 d-8 o4 b-8 g8 b-8 o5 d-8"
    "| o4 c8 c8 o5 c8 o4 c8 d-8 o5 c8 e8 f8 | o5 g8 a-8 b-8 o6 c8 o5 b-8 a-8 g8 f8 | o5 e8 f8 g8 a-8 g8 f8 e8 d-8 | o5 c2 r2";
static const char BOSS_STAB[] =
    "@17 v7 q2 [o4 c16 r16]8 [o4 d-16 r16]8 [o3 a-16 r16]8 [o3 b-16 r16]8 [o4 c16 r16]8 [o4 d-16 r16]8 [o4 e16 r16]8 [o4 c16 r16]8";
static const char BOSS_BASS[] =
    "@6 v15 q4 " E8("o2 c") E8("o2 d-") E8("o1 a-") E8("o1 b-") E8("o2 c") E8("o2 d-") E8("o2 e") E8("o2 c");
static const char BOSS_DRUM[] = "[@13 v13 o2 c8 @11 v10 o6 c16 @13 v10 o2 c16 @13 v12 o2 c8 @11 v11 o6 c8]16";

/* "Hidden Hoard" - the treasure room, F major, glittering */
static const char SPEC_LEAD[] =
    "@15 v10 q6"
    "| o6 c8 a8 f8 a8 o6 c4 o5 a4 | o5 g8 b-8 o6 d8 b-8 c2 | o5 a8 o6 c8 f8 c8 d4 c4 | o5 b-8 a8 g8 e8 f2"
    "| o6 c8 a8 f8 a8 o6 c4 d4 | o6 e8 d8 c8 o5 b-8 a4 g4 | o5 f8 g8 a8 b-8 o6 c8 d8 e8 g8 | o6 f2 r2";
static const char SPEC_PAD[] = "@4 v5 q8 o4 [f1]1 [g1]1 [f1]1 [c1]1 [f1]1 [c1]1 [b-1]1 [f1]1";
static const char SPEC_BASS[] =
    "@7 v11 q5 " Q4("o2 f", "o3 c", "f", "c") Q4("o2 g", "o3 d", "g", "d") Q4("o2 f", "o3 c", "f", "c")
    Q4("o2 c", "g", "o3 c", "o2 g") Q4("o2 f", "o3 c", "f", "c") Q4("o2 c", "g", "o3 c", "o2 g")
    Q4("o2 b-", "o3 f", "b-", "f") Q4("o2 f", "o3 c", "f", "c");
static const char SPEC_DRUM[] = "[@21 v4 o7 c8 @9 v2 o8 c8]32";

/* "The Low Glow" - the soul round, hanging on the whole-tone scale */
static const char GLOW_LEAD[] =
    "@14 v10 q8"
    "| o5 c2 d2 | o5 e2 f+2 | o5 g+2 f+4 e4 | o5 d1"
    "| o5 e2 g+2 | o5 a+2 g+4 f+4 | o5 e2 d4 c4 | o4 a+1";
static const char GLOW_ARP[] =
    "@3 v4 q3 [o5 c16 e16 g+16 o6 c16 o5 g+16 e16 c16 e16]8 [o5 d16 f+16 a+16 o6 d16 o5 a+16 f+16 d16 f+16]8";
static const char GLOW_BASS[] = "@6 v10 q8 o2 c1 c1 d1 d1 e1 e1 d1 c1";
static const char GLOW_DRUM[] = "[@9 v2 o8 c4 @36 v2 o6 c4 r2]8";

/* "Laid to Rest" - the ending, D major */
static const char END_LEAD[] =
    "@14 v11 q7"
    "| o5 d4 f+4 a4 o6 d4 | o6 c+4 o5 b4 a2 | o5 g4 b4 o6 d4 g4 | o6 f+2 e2"
    "| o6 d4 c+8 o5 b8 a4 f+4 | o5 g4 a8 b8 o6 c+4 e4 | o6 d8 c+8 o5 b8 a8 g4 e4 | o5 d1";
static const char END_COMP[] =
    "@22 v5 q5 [o4 f+8 a8]4 [o4 e8 a8]4 [o4 g8 b8]4 [o4 a8 o5 c+8]4 [o4 f+8 a8]4 [o4 e8 g8]4 [o4 d8 g8]4 [o4 f+8 a8]4";
static const char END_BASS[] =
    "@6 v12 q6 " Q4("o2 d", "a", "f+", "a") Q4("o2 a", "e", "c+", "e") Q4("o2 g", "d", "b", "d") Q4("o2 a", "e", "c+", "e")
    Q4("o2 b", "f+", "d", "f+") Q4("o2 e", "b", "g", "b") Q4("o2 a", "e", "a", "e") Q4("o2 d", "a", "f+", "a");
static const char END_DRUM[] = "[" HAND HAND "]8";

/* "Names in the Ash" - the high scores, A minor */
static const char SC_LEAD[] =
    "@2 v9 q5"
    "| o5 a8 e8 a8 b8 o6 c4 o5 b4 | o5 a8 g8 e8 g8 a2 | o5 f8 a8 o6 c8 d8 e4 d4 | o6 c8 o5 b8 a8 g+8 a2";
static const char SC_BASS[] = "@7 v11 q5 " Q4("o2 a", "e", "a", "e") Q4("o2 g", "d", "g", "d") Q4("o2 f", "o3 c", "f", "c") Q4("o2 e", "b", "e", "b");
static const char SC_DRUM[] = "[" HAND "]8";

/* jingles */
static const char CLEAR_P1[] = "@1 v12 o5 l8 d f+ a o6 d4 o5 a8 o6 d8 f+2";
static const char CLEAR_P2[] = "@4 v7 o4 l8 a o5 d f+ a4 f+8 a8 d2";
static const char CLEAR_TRI[] = "@6 v12 o2 d4 a4 o3 d4 o2 a4 d2";
static const char CLEAR_NOISE[] = "@13 v10 o2 c4 c4 c4 c4 @12 v8 o5 c2";
static const char OVER_P1[] = "@5 v11 o5 a4 g4 f4 e4 d1";
static const char OVER_TRI[] = "@6 v12 o2 d2 c2 o1 b-1";
static const char RISE_P1[] = "@15 v12 o5 l16 d f+ a o6 d f+ a o7 d4 r8";
static const char RISE_P2[] = "@39 v8 o6 l16 a o7 d f+ a o6 a o7 d f+4 r8";

void rsl_audio_load(void) {
    if (RSL_MUS_TITLE >= 0) return;
    RSL_MUS_TITLE = song_define("rsl_title", 96, true, TITLE_LEAD, TITLE_DRONE, TITLE_BASS, TITLE_DRUM);
    RSL_MUS_STORY = song_define("rsl_story", 72, true, STORY_LEAD, STORY_PAD, STORY_BASS, STORY_SOFT);
    RSL_MUS_S1A = song_define("rsl_s1a", 112, true, S1A_LEAD, S1A_ECHO, S1A_BASS, S1A_DRUM);
    RSL_MUS_S1B = song_define("rsl_s1b", 104, true, S1B_LEAD, S1B_ARP, S1B_BASS, S1B_DRUM);
    RSL_MUS_S2A = song_define("rsl_s2a", 96, true, S2A_LEAD, S2A_DRIP, S2A_BASS, S2A_DRUM);
    RSL_MUS_S2B = song_define("rsl_s2b", 116, true, S2B_LEAD, S2B_COMP, S2B_BASS, S2B_DRUM);
    RSL_MUS_S3A = song_define("rsl_s3a", 108, true, S3A_LEAD, S3A_ARP, S3A_BASS, S3A_DRUM);
    RSL_MUS_S3B = song_define("rsl_s3b", 140, true, S3B_LEAD, S3B_STAB, S3B_BASS, S3B_DRUM);
    RSL_MUS_MIDBOSS = song_define("rsl_midboss", 150, true, MID_LEAD, MID_ARP, MID_BASS, MID_DRUM);
    RSL_MUS_BOSS = song_define("rsl_boss", 146, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    RSL_MUS_SPECIAL = song_define("rsl_special", 100, true, SPEC_LEAD, SPEC_PAD, SPEC_BASS, SPEC_DRUM);
    RSL_MUS_SPIRIT = song_define("rsl_spirit", 84, true, GLOW_LEAD, GLOW_ARP, GLOW_BASS, GLOW_DRUM);
    RSL_MUS_ENDING = song_define("rsl_ending", 92, true, END_LEAD, END_COMP, END_BASS, END_DRUM);
    RSL_MUS_SCORES = song_define("rsl_scores", 110, true, SC_LEAD, "", SC_BASS, SC_DRUM);
    RSL_MUS_CLEAR = song_define("rsl_clear", 132, false, CLEAR_P1, CLEAR_P2, CLEAR_TRI, CLEAR_NOISE);
    RSL_MUS_OVER = song_define("rsl_over", 80, false, OVER_P1, "", OVER_TRI, "");
    RSL_MUS_RISE = song_define("rsl_rise", 120, false, RISE_P1, RISE_P2, "", "");

    sfx_define("rsl_shot", CH_P2, 240, "@20 v6 o5 a32 e32");
    sfx_define("rsl_bigshot", CH_P2, 220, "@32 v10 o4 c16 g16");
    sfx_define("rsl_charged", CH_P2, 240, "@39 v7 o6 c32 e32 g32");
    sfx_define("rsl_jump", CH_P1, 240, "@32 v8 o4 c32 g32");
    sfx_define("rsl_land", CH_NOISE, 240, "@13 v8 o3 c32");
    sfx_define("rsl_hit", CH_NOISE, 240, "@21 v8 o6 c32");
    sfx_define("rsl_tink", CH_P2, 240, "@42 v8 o7 c32");
    sfx_define("rsl_pop", CH_NOISE, 200, "@34 v9 o4 c16");
    sfx_define("rsl_bossdie", CH_NOISE, 90, "@34 v15 o2 c2");
    sfx_define("rsl_die", CH_P1, 140, "@33 v13 o5 c8 o4 a8 f8 d4");
    sfx_define("rsl_flag", CH_P1, 200, "@37 v10 o5 c16 o4 g16");
    sfx_define("rsl_fall", CH_P1, 160, "@33 v10 o5 c4");
    sfx_define("rsl_swish", CH_NOISE, 240, "@36 v8 o6 c16");
    sfx_define("rsl_throw", CH_NOISE, 240, "@36 v6 o5 c32");
    sfx_define("rsl_spit", CH_P2, 220, "@37 v7 o4 g16");
    sfx_define("rsl_fireball", CH_NOISE, 220, "@36 v7 o4 c16");
    sfx_define("rsl_tongue", CH_P2, 240, "@38 v8 o4 c16");
    sfx_define("rsl_drip", CH_P2, 240, "@15 v6 o7 c32");
    sfx_define("rsl_zap", CH_NOISE, 200, "@40 v11 o6 c8");
    sfx_define("rsl_whoosh", CH_NOISE, 200, "@36 v7 o4 c8");
    sfx_define("rsl_wisps", CH_P2, 220, "@39 v6 o6 e16 g16");
    sfx_define("rsl_boom", CH_NOISE, 140, "@34 v13 o3 c4");
    sfx_define("rsl_rumble", CH_NOISE, 160, "@34 v8 o2 c8");
    sfx_define("rsl_roar", CH_NOISE, 160, "@34 v9 o3 c4");
    sfx_define("rsl_click", CH_P2, 240, "@42 v10 o5 c32 r32 c32");
    sfx_define("rsl_torch", CH_NOISE, 200, "@36 v9 o5 c8");
    sfx_define("rsl_coin", CH_P1, 240, "@35 v10 o6 e32 b32");
    sfx_define("rsl_bigcoin", CH_P1, 220, "@35 v12 o6 c32 e32 g32 o7 c16");
    sfx_define("rsl_clock", CH_P1, 220, "@15 v10 o6 c16 g16 c16");
    sfx_define("rsl_lily", CH_P1, 200, "@39 v11 o5 l32 c e g o6 c e g");
    sfx_define("rsl_hatch", CH_P1, 220, "@39 v10 o6 c16 e16 g16");
    sfx_define("rsl_bell", CH_P1, 160, "@15 v13 o6 d8 a8 o7 d4");
    sfx_define("rsl_bless", CH_P1, 180, "@39 v12 o5 l16 d f+ a o6 d");
    sfx_define("rsl_power", CH_P1, 200, "@35 v12 o5 c16 g16 o6 c16 e16");
    sfx_define("rsl_wheel", CH_P2, 220, "@15 v8 o6 c16 e16");
    sfx_define("rsl_secret", CH_P1, 200, "@39 v12 o6 l32 c d e f g a b o7 c");
    sfx_define("rsl_gift", CH_P1, 200, "@39 v11 o6 l32 e g o7 c e");
    sfx_define("rsl_piece", CH_P1, 240, "@35 v10 o6 a32 o7 e32");
    sfx_define("rsl_whole", CH_P1, 180, "@39 v12 o6 l16 d f+ a o7 d");
    sfx_define("rsl_final", CH_P1, 120, "@33 v13 o4 c4 o3 c2");
    sfx_define("rsl_revive", CH_P1, 180, "@38 v12 o4 c16 g16 o5 c16 g16");
    sfx_define("rsl_bossin", CH_P1, 140, "@33 v13 o3 c4 o2 g4");
    sfx_define("rsl_victory", CH_P1, 160, "@39 v13 o5 l16 c e g o6 c4");
    sfx_define("rsl_thud", CH_NOISE, 160, "@41 v13 o2 c8");
    sfx_define("rsl_screech", CH_P2, 200, "@37 v10 o6 c8 o5 a8");
    sfx_define("rsl_belch", CH_NOISE, 180, "@34 v8 o2 c8");
    sfx_define("rsl_timeup", CH_P1, 160, "@15 v12 o5 c8 r16 c8 r16 c8");
    sfx_define("rsl_tick", CH_P2, 240, "@42 v7 o6 g32");
}
