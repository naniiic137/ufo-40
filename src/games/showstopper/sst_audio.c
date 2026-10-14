/* SHOWSTOPPER - original music (UFO-MML) and sound effects. All composed
 * for UFO 40: a tune for each stage (two for the two-half ones), the map,
 * the bosses, the last two forms, the Hop, and the endings. */
#include "sst.h"

int SST_MUS_TITLE = -1, SST_MUS_STORY, SST_MUS_MAP, SST_MUS_S1, SST_MUS_S2, SST_MUS_S3A, SST_MUS_S3B, SST_MUS_S4A,
    SST_MUS_S4B, SST_MUS_S5, SST_MUS_BOSS, SST_MUS_MAW, SST_MUS_HEART, SST_MUS_HOP, SST_MUS_CLEAR, SST_MUS_OVER,
    SST_MUS_BAD, SST_MUS_GOOD, SST_MUS_CREDITS;

/* a bar of off-beat chord stabs */
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
/* a bar of oom-pah bass */
#define OOM(lo, hi) "o2 " lo "4 o3 " hi "4 o2 " lo "4 o3 " hi "4 "
/* a bar of sixteenth arpeggio on four notes */
#define AR4(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
/* a bar of eighth arpeggio on three notes */
#define ARP8(a, b, c) a "8 " b "8 " c "8 " b "8 " a "8 " b "8 " c "8 " b "8 "
/* a bar of bass in octaves, and of quarter notes */
#define OCT8(n) "[o2 " n "8 o3 " n "8]4 "
#define Q4(n) n "4 " n "4 " n "4 " n "4 "
/* a 3/4 bar: the waltz's two stabs */
#define WZ(ins, n) ins " r4 " n "4 " n "4 "

#define MARCH "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
#define TRAIN_DR "[@13 v12 o2 c8 @9 v5 o8 c16 c16 @11 v10 o6 c8 @9 v5 o8 c16 c16]16"

/* "Showstopper" - the title, F major */
static const char TITLE_LEAD[] =
    "@1 v11 q6 | o5 c4 f8 a8 o6 c4 o5 a8 f8 | o5 g4 a8 b-8 o6 c4 r4 | o5 d4 g8 b-8 o6 d4 c8 o5 b-8 | o5 a4 g8 f8 g2"
    "| o5 c4 f8 a8 o6 c4 o5 a8 f8 | o5 b-4 o6 d8 c8 o5 b-4 a4 | o5 g8 a8 b-8 g8 e4 g4 | o5 f2 r2";
static const char TITLE_OFF[] =
    "v6 q3 " OFF("@16 o4", "f") OFF("@16 o4", "c") OFF("@16 o3", "b-") OFF("@16 o4", "c") OFF("@16 o4", "f")
    OFF("@16 o3", "b-") OFF("@16 o4", "c") OFF("@16 o4", "f");
static const char TITLE_BASS[] =
    "@6 v14 q5 " OOM("f", "c") OOM("c", "g") OOM("b-", "f") OOM("c", "g") OOM("f", "c") OOM("b-", "f") OOM("c", "g") OOM("f", "c");
static const char TITLE_DRUM[] = "[" MARCH MARCH "]8";

/* "The Cage Door" - the story, D minor */
static const char STORY_LEAD[] =
    "@14 v10 q7 | o5 d2 f4 e4 | o5 d2. r4 | o5 a2 g4 f4 | o5 e1 | o5 d2 f4 a4 | o5 b-2 a4 g4 | o5 f4 e4 c+4 e4 | o5 d1";
static const char STORY_ARP[] =
    "@2 v4 q5 " ARP8("o4 d", "o4 f", "o4 a") ARP8("o4 d", "o4 f", "o4 a") ARP8("o4 f", "o4 a", "o5 c")
    ARP8("o4 e", "o4 a", "o5 c+") ARP8("o4 d", "o4 f", "o4 a") ARP8("o4 d", "o4 f", "o4 b-") ARP8("o4 e", "o4 a", "o5 c+")
    ARP8("o4 d", "o4 f", "o4 a");
static const char STORY_BASS[] = "@6 v11 q8 o2 d1 o2 d1 o2 f1 o2 a1 o2 d1 o2 b-1 o2 g2 o2 a2 o2 d1";
static const char STORY_TICK[] = "[@9 v2 o8 c4]32";

/* "The Route" - the map, C major */
static const char MAP_LEAD[] =
    "@15 v10 q7 | o5 e8 g8 o6 c8 o5 g8 a4 g4 | o5 f8 e8 d8 e8 c2 | o5 e8 g8 o6 c8 e8 d4 c4 | o5 b8 a8 g8 b8 o6 c2"
    "| o5 a8 o6 c8 f8 c8 o5 a4 g4 | o5 f8 a8 g8 f8 e4 c4 | o5 d8 e8 f8 g8 a8 b8 o6 c8 d8 | o6 c2 r2";
static const char MAP_OFF[] =
    "v5 q3 " OFF("@16 o4", "c") OFF("@16 o4", "c") OFF("@17 o4", "a") OFF("@16 o4", "g") OFF("@16 o4", "f")
    OFF("@17 o4", "d") OFF("@16 o4", "g") OFF("@16 o4", "c");
static const char MAP_BASS[] =
    "@6 v12 q6 o2 c2 o2 g2 o2 f2 o3 c2 o2 a2 o2 e2 o2 g2 o2 g2 o2 f2 o2 f2 o2 d2 o2 c2 o2 g2 o2 g2 o2 c2 o2 c2";
static const char MAP_TICK[] = "[@21 v3 o7 c8 @9 v2 o8 c8]32";

/* "Frostbell Lane" - stage I, G major */
#define SLEIGH "@13 v11 o2 c8 @21 v4 o7 c16 c16 @11 v9 o6 c8 @21 v4 o7 c16 c16 "
static const char S1_LEAD[] =
    "@1 v11 q6 | o5 d8 g8 b8 g8 o6 d4 o5 b4 | o5 c8 e8 a8 e8 o6 c4 o5 a4 | o5 d8 f+8 a8 f+8 o6 d4 c8 o5 a8 | o5 b8 a8 g8 f+8 g2"
    "| o5 d8 g8 b8 o6 d8 g4 f+8 e8 | o6 e8 d8 c8 o5 b8 o6 c4 o5 a4 | o5 b8 o6 c8 d8 e8 d8 c8 o5 b8 a8 | o5 g2 r2"
    "| o5 e4 g8 e8 b4 a4 | o5 a8 g8 f+8 e8 f+2 | o5 g4 b8 g8 o6 d4 c4 | o5 b8 a8 b8 o6 c8 d2"
    "| o6 e8 d8 c8 o5 b8 o6 c8 o5 a8 f+8 a8 | o5 g8 f+8 e8 d8 e4 f+4 | o5 g8 a8 b8 o6 c8 d8 e8 f+8 a8 | o6 g2 r2";
static const char S1_ARP[] =
    "@2 v5 q5 " AR4("o4 g", "b", "o5 d", "o4 b") AR4("o4 a", "o5 c", "e", "c") AR4("o4 f+", "a", "o5 d", "o4 a")
    AR4("o4 g", "b", "o5 d", "o4 b") AR4("o4 g", "b", "o5 d", "o4 b") AR4("o4 e", "g", "o5 c", "o4 g")
    AR4("o4 f+", "a", "o5 d", "o4 a") AR4("o4 g", "b", "o5 d", "o4 b") AR4("o4 e", "g", "b", "g")
    AR4("o4 f+", "a", "o5 d", "o4 a") AR4("o4 g", "b", "o5 d", "o4 b") AR4("o4 f+", "a", "o5 d", "o4 a")
    AR4("o4 e", "g", "o5 c", "o4 g") AR4("o4 e", "g", "b", "g") AR4("o4 g", "b", "o5 d", "o4 b") AR4("o4 g", "b", "o5 d", "o4 b");
static const char S1_BASS[] =
    "@6 v13 q5 " Q4("o2 g") Q4("o2 a") Q4("o2 d") Q4("o2 g") Q4("o2 g") Q4("o2 c") Q4("o2 d") Q4("o2 g") Q4("o2 e")
    Q4("o2 d") Q4("o2 g") Q4("o2 d") Q4("o2 c") Q4("o2 e") Q4("o2 g") Q4("o2 g");
static const char S1_DRUM[] = "[" SLEIGH SLEIGH "]16";

/* "The Night Freight" - stage II, A minor */
static const char S2_LEAD[] =
    "@0 v11 q5 | o5 a8 a8 o6 c8 o5 a8 e8 a8 g8 e8 | o5 f8 f8 a8 f8 c8 f8 e8 c8 | o5 g8 g8 b8 g8 d8 g8 f8 d8"
    "| o5 e8 g+8 b8 o6 d8 e4 o5 b4 | o5 a8 a8 o6 c8 e8 d8 c8 o5 b8 a8 | o5 f8 a8 o6 c8 f8 e8 d8 c8 o5 a8"
    "| o5 d8 f8 a8 o6 d8 c8 o5 b8 g+8 e8 | o5 a2 e4 r4";
static const char S2_ARP[] =
    "@2 v5 q5 " AR4("o4 a", "o5 c", "e", "c") AR4("o4 f", "a", "o5 c", "o4 a") AR4("o4 g", "b", "o5 d", "o4 b")
    AR4("o4 e", "g+", "b", "g+") AR4("o4 a", "o5 c", "e", "c") AR4("o4 f", "a", "o5 c", "o4 a") AR4("o4 d", "f", "a", "f")
    AR4("o4 e", "g+", "b", "g+");
static const char S2_BASS[] = "@6 v14 q4 " OCT8("a") OCT8("f") OCT8("g") OCT8("e") OCT8("a") OCT8("f") OCT8("d") OCT8("e");

/* "Lanternbury" - stage III, the town, D major */
static const char S3A_LEAD[] =
    "@14 v11 q7 | o5 f+4 a8 f+8 d4 e8 f+8 | o5 g4 b8 g8 e4 f+8 g8 | o5 a4 o6 d8 c+8 o5 b4 a4 | o5 g8 f+8 e8 d8 e2"
    "| o5 f+4 a8 f+8 d4 e8 f+8 | o5 g4 b8 o6 d8 e4 d8 c+8 | o5 b8 a8 g8 e8 f+4 e4 | o5 d2 r2";
static const char S3A_OFF[] =
    "v6 q3 " OFF("@16 o4", "d") OFF("@16 o4", "g") OFF("@16 o4", "d") OFF("@16 o4", "a") OFF("@16 o4", "d")
    OFF("@16 o4", "g") OFF("@16 o4", "a") OFF("@16 o4", "d");
static const char S3A_BASS[] =
    "@6 v13 q6 o2 d2 o2 a2 o2 g2 o3 d2 o2 d2 o2 a2 o2 a2 o3 e2 o2 d2 o2 a2 o2 g2 o3 d2 o2 a2 o3 e2 o2 d2 o2 a2";
static const char S3A_DRUM[] = "[@13 v11 o2 c4 @11 v9 o6 c8 @9 v4 o8 c8 @13 v8 o2 c8 @13 v11 o2 c8 @11 v9 o6 c4]8";

/* "The Canal Side" - stage III, the second half, B minor */
static const char S3B_LEAD[] =
    "@5 v11 q7 | o5 b4 o6 d8 c+8 o5 b4 f+4 | o5 g4 a8 b8 a4 e4 | o5 f+4 a8 b8 o6 c+4 o5 a4 | o5 b8 a8 g8 f+8 e2"
    "| o5 b4 o6 d8 e8 f+4 e8 d8 | o6 c+4 o5 b8 a8 g4 f+4 | o5 e8 f+8 g8 a8 b8 a8 g8 f+8 | o5 b2 r2";
static const char S3B_ARP[] =
    "@2 v5 q5 " ARP8("o4 b", "o5 d", "o5 f+") ARP8("o4 e", "o4 g", "o4 b") ARP8("o4 a", "o5 c+", "o5 e")
    ARP8("o4 f+", "o4 a+", "o5 c+") ARP8("o4 b", "o5 d", "o5 f+") ARP8("o4 a", "o5 c+", "o5 e") ARP8("o4 e", "o4 g", "o4 b")
    ARP8("o4 b", "o5 d", "o5 f+");
static const char S3B_BASS[] = "@6 v13 q7 o2 b1 o2 e1 o2 a1 o2 f+1 o2 b1 o2 a1 o2 e1 o2 b1";
static const char S3B_DRUM[] = "[@13 v10 o2 c4 @11 v8 o6 c4 @13 v10 o2 c8 @13 v7 o2 c8 @11 v8 o6 c4]8";

/* "The Glass Palace" - stage IV, a waltz in E minor */
static const char S4A_LEAD[] =
    "@15 v10 q7 | o5 e4 g4 b4 | o6 e2 d4 | o6 c4 o5 b4 a4 | o5 b2. | o5 a4 o6 c4 e4 | o6 d2 c4 | o5 b4 a4 f+4 | o5 g2."
    "| o5 e4 g4 b4 | o6 e4 f+4 g4 | o6 f+4 e4 d+4 | o6 e2.";
static const char S4A_WZ[] =
    "v5 q3 " WZ("@17 o4", "e") WZ("@17 o4", "e") WZ("@17 o4", "a") WZ("@16 o4", "b") WZ("@17 o4", "a") WZ("@16 o4", "d")
    WZ("@16 o4", "b") WZ("@17 o4", "e") WZ("@17 o4", "e") WZ("@17 o4", "e") WZ("@16 o4", "b") WZ("@17 o4", "e");
static const char S4A_BASS[] =
    "@6 v12 q7 o2 e2. o2 e2. o2 a2. o2 b2. o2 a2. o2 d2. o2 b2. o2 e2. o2 e2. o2 e2. o2 b2. o2 e2.";
static const char S4A_DRUM[] = "[@13 v9 o2 c4 @9 v3 o8 c4 @9 v3 o8 c4]12";

/* "The Upper Halls" - stage IV, the second half, C minor */
static const char S4B_LEAD[] =
    "@1 v11 q6 | o5 c8 e-8 g8 o6 c8 o5 b4 g4 | o5 a-8 g8 f8 e-8 d4 g4 | o5 c8 e-8 g8 o6 c8 d8 e-8 d8 c8 | o5 b8 a-8 g8 f8 g2"
    "| o6 c8 o5 b8 a-8 g8 a-4 f4 | o5 g8 f8 e-8 d8 e-4 c4 | o5 d8 e-8 f8 g8 a-8 b8 o6 c8 d8 | o6 c2 r2";
static const char S4B_ARP[] =
    "@2 v5 q5 " AR4("o4 c", "e-", "g", "e-") AR4("o4 c", "e-", "a-", "e-") AR4("o4 c", "e-", "g", "e-")
    AR4("o4 d", "g", "b", "g") AR4("o4 c", "f", "a-", "f") AR4("o4 c", "e-", "g", "e-") AR4("o4 d", "g", "b", "g")
    AR4("o4 c", "e-", "g", "e-");
static const char S4B_BASS[] = "@6 v14 q4 " OCT8("c") OCT8("a-") OCT8("c") OCT8("g") OCT8("f") OCT8("c") OCT8("g") OCT8("c");

/* "Going Up" - stage V, the lift, A minor */
static const char S5_LEAD[] =
    "@23 v11 q7 | o4 a4 o5 c4 e4 a4 | o5 g+2 e2 | o5 f4 a4 o6 c4 f4 | o6 e2 c2 | o5 d4 f4 a4 o6 d4 | o6 c4 o5 b4 a4 g+4"
    "| o5 a4 b4 o6 c4 d4 | o6 e2 o5 e2";
static const char S5_ARP[] =
    "@2 v5 q5 " AR4("o4 a", "o5 c", "e", "c") AR4("o4 e", "g+", "b", "g+") AR4("o4 f", "a", "o5 c", "o4 a")
    AR4("o4 e", "g", "o5 c", "o4 g") AR4("o4 d", "f", "a", "f") AR4("o4 e", "g+", "b", "g+") AR4("o4 a", "o5 c", "e", "c")
    AR4("o4 e", "g+", "b", "g+");
static const char S5_BASS[] = "@6 v14 q4 " OCT8("a") OCT8("e") OCT8("f") OCT8("c") OCT8("d") OCT8("e") OCT8("a") OCT8("e");

/* "Big Top Brawl" - the bosses, E minor */
#define BOSS_DR "[@13 v12 o2 c8 @11 v10 o6 c8 @13 v12 o2 c8 @11 v10 o6 c8]16"
static const char BOSS_LEAD[] =
    "@0 v12 q5 | o5 e8 e8 g8 e8 b8 e8 a8 g8 | o5 f+8 f+8 a8 f+8 o6 c8 o5 b8 a8 f+8 | o5 e8 e8 g8 e8 b8 e8 o6 d8 c8"
    "| o5 b8 g8 f+8 d+8 e4 b4 | o6 e8 d8 c8 o5 b8 a8 g8 f+8 e8 | o5 c8 e8 a8 g8 f+8 e8 d+8 f+8"
    "| o5 g8 a8 b8 o6 c8 d8 c8 o5 b8 a8 | o5 b4 f+4 e2";
static const char BOSS_ARP[] =
    "@2 v5 q5 " AR4("o4 e", "g", "b", "g") AR4("o4 d", "f+", "a", "f+") AR4("o4 e", "g", "b", "g")
    AR4("o4 d+", "f+", "b", "f+") AR4("o4 e", "g", "o5 c", "o4 g") AR4("o4 a", "o5 c", "e", "c")
    AR4("o4 g", "b", "o5 d", "o4 b") AR4("o4 d+", "f+", "b", "f+");
static const char BOSS_BASS[] = "@6 v14 q4 " OCT8("e") OCT8("d") OCT8("e") OCT8("b") OCT8("c") OCT8("a") OCT8("g") OCT8("b");

/* "The Morrowmaw" - G minor */
static const char MAW_LEAD[] =
    "@23 v12 q7 | o5 g4 b-4 a4 f+4 | o5 g2 d2 | o5 e-4 g4 f+4 d4 | o5 g2. r4 | o6 d4 c4 o5 b-4 a4 | o5 b-4 a4 g4 f+4"
    "| o5 g8 a8 b-8 o6 c8 d8 e-8 d8 c8 | o5 b-4 a4 g2";
static const char MAW_ARP[] =
    "@2 v5 q5 " AR4("o4 g", "b-", "o5 d", "o4 b-") AR4("o4 g", "b-", "o5 d", "o4 b-") AR4("o4 g", "b-", "o5 e-", "o4 b-")
    AR4("o4 f+", "a", "o5 d", "o4 a") AR4("o4 g", "b-", "o5 d", "o4 b-") AR4("o4 g", "o5 c", "e-", "c")
    AR4("o4 f+", "a", "o5 d", "o4 a") AR4("o4 g", "b-", "o5 d", "o4 b-");
static const char MAW_BASS[] = "@6 v14 q4 " OCT8("g") OCT8("g") OCT8("e-") OCT8("d") OCT8("g") OCT8("c") OCT8("d") OCT8("g");

/* "The Heartstack" - F minor */
static const char HEART_LEAD[] =
    "@14 v12 q7 | o5 f4 a-4 o6 c4 o5 a-4 | o5 b-4 o6 d-4 f4 e4 | o6 f4 e-4 d-4 c4 | o5 b-2 o6 c2 | o5 a-4 o6 c4 f4 c4"
    "| o6 d-4 c4 o5 b-4 a-4 | o5 g4 a-4 b-4 o6 c4 | o6 f2 r2";
static const char HEART_ARP[] =
    "@2 v5 q5 " AR4("o4 f", "a-", "o5 c", "o4 a-") AR4("o4 f", "a-", "o5 d-", "o4 a-") AR4("o4 f", "b-", "o5 d-", "o4 b-")
    AR4("o4 e", "g", "o5 c", "o4 g") AR4("o4 f", "a-", "o5 c", "o4 a-") AR4("o4 f", "a-", "o5 d-", "o4 a-")
    AR4("o4 e", "g", "o5 c", "o4 g") AR4("o4 f", "a-", "o5 c", "o4 a-");
static const char HEART_BASS[] = "@6 v14 q4 " OCT8("f") OCT8("d-") OCT8("b-") OCT8("c") OCT8("f") OCT8("d-") OCT8("c") OCT8("f");

/* "Jelly's Hop" - the bonus round, C major */
static const char HOP_LEAD[] =
    "@3 v11 q5 | o5 c8 e8 g8 e8 c8 e8 g8 o6 c8 | o5 b8 g8 d8 g8 b8 g8 o6 d8 o5 b8 | o5 a8 f8 c8 f8 a8 o6 c8 o5 a8 f8"
    "| o5 g8 e8 g8 b8 o6 c4 r4 | o5 e8 g8 o6 c8 e8 d8 c8 o5 b8 a8 | o5 g8 f8 e8 d8 c8 d8 e8 f8"
    "| o5 g8 a8 b8 g8 o6 c8 d8 e8 g8 | o6 c4 o5 g4 c4 r4";
static const char HOP_OFF[] =
    "v6 q3 " OFF("@16 o4", "c") OFF("@16 o4", "g") OFF("@16 o4", "f") OFF("@16 o4", "c") OFF("@16 o4", "c")
    OFF("@16 o4", "g") OFF("@16 o4", "g") OFF("@16 o4", "c");
static const char HOP_BASS[] =
    "@7 v13 q5 " OOM("c", "g") OOM("g", "d") OOM("f", "c") OOM("c", "g") OOM("c", "g") OOM("g", "d") OOM("g", "d") OOM("c", "g");
static const char HOP_DRUM[] = "[@13 v10 o2 c8 @21 v4 o7 c8 @11 v8 o6 c8 @21 v4 o7 c8]16";

/* jingles */
static const char CLEAR_P1[] = "@1 v12 o5 c8 e8 g8 o6 c4 o5 g8 o6 c2";
static const char CLEAR_P2[] = "@2 v7 o4 e8 g8 o5 c8 e4 c8 e2";
static const char CLEAR_TRI[] = "@6 v14 o2 c4 g4 o3 c2";
static const char CLEAR_NOISE[] = "@12 v8 o5 c2 r2";
static const char OVER_P1[] = "@14 v11 o5 e4 d4 c4 o4 b4 a1";
static const char OVER_TRI[] = "@6 v12 o2 a2 e2 a1";

/* "An Empty Cage" - the ending with continues, A minor */
static const char BAD_LEAD[] =
    "@5 v10 q7 | o5 e2 d4 c4 | o4 b2 a2 | o5 c2 o4 b4 a4 | o4 g+1 | o5 a2 g4 f4 | o5 e2 d2 | o5 c4 o4 b4 a4 g+4 | o4 a1";
static const char BAD_ARP[] =
    "@2 v4 q5 " ARP8("o3 a", "o4 c", "o4 e") ARP8("o3 g+", "o3 b", "o4 e") ARP8("o3 a", "o4 c", "o4 e")
    ARP8("o3 g+", "o3 b", "o4 e") ARP8("o3 a", "o4 c", "o4 f") ARP8("o3 a", "o4 c", "o4 e") ARP8("o3 g+", "o3 b", "o4 e")
    ARP8("o3 a", "o4 c", "o4 e");
static const char BAD_BASS[] = "@6 v11 q8 o2 a1 o2 e1 o2 a1 o2 e1 o2 f1 o2 a1 o2 e1 o2 a1";
static const char BAD_TICK[] = "[@9 v2 o8 c4]32";

/* "Encore" - the true ending, F major */
static const char GOOD_LEAD[] =
    "@14 v11 q7 | o5 f4 a4 o6 c4 f4 | o6 e4 d4 c2 | o5 b-4 o6 d4 f4 d4 | o6 c2. r4 | o5 a4 o6 c4 f4 a4 | o6 g4 f4 e4 d4"
    "| o6 c4 o5 b-4 a4 g4 | o5 f2 r2";
static const char GOOD_OFF[] =
    "v6 q3 " OFF("@16 o4", "f") OFF("@16 o4", "c") OFF("@16 o3", "b-") OFF("@16 o4", "c") OFF("@16 o4", "f")
    OFF("@16 o4", "c") OFF("@16 o4", "c") OFF("@16 o4", "f");
static const char GOOD_BASS[] =
    "@6 v13 q5 " OOM("f", "c") OOM("c", "g") OOM("b-", "f") OOM("c", "g") OOM("f", "c") OOM("c", "g") OOM("c", "g") OOM("f", "c");
static const char GOOD_DRUM[] = "[@13 v8 o2 c4 @11 v6 o6 c4]16";

/* "Take a Bow" - the credits, G major */
static const char CRED_LEAD[] =
    "@0 v11 q6 | o5 g8 b8 o6 d8 o5 b8 o6 c4 o5 a4 | o5 f+8 a8 o6 d8 c8 o5 b4 g4 | o5 e8 g8 b8 g8 a4 f+4"
    "| o5 g8 a8 b8 o6 c8 d2 | o6 e8 d8 c8 o5 b8 a8 b8 o6 c8 d8 | o6 e4 c4 o5 a4 f+4 | o5 g8 b8 a8 f+8 e4 d4 | o5 g2 r2";
static const char CRED_ARP[] =
    "@2 v5 q5 " AR4("o4 g", "b", "o5 d", "o4 b") AR4("o4 f+", "a", "o5 d", "o4 a") AR4("o4 e", "g", "b", "g")
    AR4("o4 f+", "a", "o5 d", "o4 a") AR4("o4 e", "g", "o5 c", "o4 g") AR4("o4 a", "o5 c", "e", "c")
    AR4("o4 f+", "a", "o5 d", "o4 a") AR4("o4 g", "b", "o5 d", "o4 b");
static const char CRED_BASS[] =
    "@6 v12 q5 " Q4("o2 g") Q4("o2 d") Q4("o2 e") Q4("o2 d") Q4("o2 c") Q4("o2 a") Q4("o2 d") Q4("o2 g");
static const char CRED_DRUM[] = "[@13 v10 o2 c4 @11 v8 o6 c4]16";

void sst_audio_load(void) {
    if (SST_MUS_TITLE >= 0) return;
    SST_MUS_TITLE = song_define("sst_title", 136, true, TITLE_LEAD, TITLE_OFF, TITLE_BASS, TITLE_DRUM);
    SST_MUS_STORY = song_define("sst_story", 80, true, STORY_LEAD, STORY_ARP, STORY_BASS, STORY_TICK);
    SST_MUS_MAP = song_define("sst_map", 112, true, MAP_LEAD, MAP_OFF, MAP_BASS, MAP_TICK);
    SST_MUS_S1 = song_define("sst_s1", 128, true, S1_LEAD, S1_ARP, S1_BASS, S1_DRUM);
    SST_MUS_S2 = song_define("sst_s2", 150, true, S2_LEAD, S2_ARP, S2_BASS, TRAIN_DR);
    SST_MUS_S3A = song_define("sst_s3a", 120, true, S3A_LEAD, S3A_OFF, S3A_BASS, S3A_DRUM);
    SST_MUS_S3B = song_define("sst_s3b", 108, true, S3B_LEAD, S3B_ARP, S3B_BASS, S3B_DRUM);
    SST_MUS_S4A = song_define("sst_s4a", 150, true, S4A_LEAD, S4A_WZ, S4A_BASS, S4A_DRUM);
    SST_MUS_S4B = song_define("sst_s4b", 132, true, S4B_LEAD, S4B_ARP, S4B_BASS, TRAIN_DR);
    SST_MUS_S5 = song_define("sst_s5", 140, true, S5_LEAD, S5_ARP, S5_BASS, TRAIN_DR);
    SST_MUS_BOSS = song_define("sst_boss", 160, true, BOSS_LEAD, BOSS_ARP, BOSS_BASS, BOSS_DR);
    SST_MUS_MAW = song_define("sst_maw", 138, true, MAW_LEAD, MAW_ARP, MAW_BASS, BOSS_DR);
    SST_MUS_HEART = song_define("sst_heart", 150, true, HEART_LEAD, HEART_ARP, HEART_BASS, BOSS_DR);
    SST_MUS_HOP = song_define("sst_hop", 126, true, HOP_LEAD, HOP_OFF, HOP_BASS, HOP_DRUM);
    SST_MUS_CLEAR = song_define("sst_clear", 150, false, CLEAR_P1, CLEAR_P2, CLEAR_TRI, CLEAR_NOISE);
    SST_MUS_OVER = song_define("sst_over", 100, false, OVER_P1, "", OVER_TRI, "");
    SST_MUS_BAD = song_define("sst_bad", 80, true, BAD_LEAD, BAD_ARP, BAD_BASS, BAD_TICK);
    SST_MUS_GOOD = song_define("sst_good", 112, true, GOOD_LEAD, GOOD_OFF, GOOD_BASS, GOOD_DRUM);
    SST_MUS_CREDITS = song_define("sst_credits", 120, true, CRED_LEAD, CRED_ARP, CRED_BASS, CRED_DRUM);

    sfx_define("sst_shot", CH_P2, 240, "@20 v5 o6 e32");
    sfx_define("sst_dash", CH_NOISE, 220, "@36 v9 o6 c16");
    sfx_define("sst_hit", CH_NOISE, 240, "@21 v6 o6 c32");
    sfx_define("sst_tink", CH_P1, 240, "@15 v10 o7 e32 o7 b32");
    sfx_define("sst_found", CH_P1, 200, "@39 v12 o6 l32 c e g o7 c e g");
    sfx_define("sst_pop", CH_NOISE, 220, "@21 v9 o5 c16");
    sfx_define("sst_crate", CH_NOISE, 180, "@40 v10 o5 c8");
    sfx_define("sst_card", CH_P1, 240, "@35 v10 o6 a16 o7 c16");
    sfx_define("sst_power", CH_P1, 200, "@39 v12 o5 l16 c e g o6 c");
    sfx_define("sst_big", CH_P1, 220, "@39 v13 o5 l32 c d e f g a b o6 c d e f g o7 c8");
    sfx_define("sst_dud", CH_P1, 180, "@37 v10 o4 e8 c8");
    sfx_define("sst_ouch", CH_P1, 200, "@37 v12 o5 c16 o4 g16");
    sfx_define("sst_lose", CH_P1, 140, "@33 v13 o5 g8 e8 c8 o4 g4");
    sfx_define("sst_bubble", CH_P2, 200, "@38 v10 o5 c16 g16 o6 c16");
    sfx_define("sst_fall", CH_P1, 150, "@33 v12 o6 c8 o5 g8 e8 c8 o4 g8");
    sfx_define("sst_defuse", CH_P2, 240, "@42 v9 o6 g32 o7 c32");
    sfx_define("sst_ballgone", CH_P2, 220, "@33 v9 o6 c16 o5 c16");
    sfx_define("sst_boom", CH_NOISE, 140, "@34 v13 o4 c4");
    sfx_define("sst_pass", CH_P1, 180, "@39 v13 o6 l16 c e g o7 c8 o6 g8 o7 c4");
    sfx_define("sst_puff", CH_NOISE, 200, "@36 v7 o5 c8");
    sfx_define("sst_flap", CH_NOISE, 240, "@36 v6 o7 c32 r32 c32");
    sfx_define("sst_wave", CH_P2, 220, "@38 v7 o5 c16 e16");
    sfx_define("sst_rush", CH_NOISE, 200, "@36 v9 o4 c8");
    sfx_define("sst_stars", CH_P2, 220, "@39 v9 o6 c32 e32 g32");
    sfx_define("sst_launch", CH_P2, 200, "@32 v11 o4 c8 g8");
    sfx_define("sst_ring", CH_P2, 220, "@15 v8 o5 g16 o6 c16");
    sfx_define("sst_drop", CH_P2, 220, "@41 v10 o4 c8");
    sfx_define("sst_jet", CH_NOISE, 160, "@34 v10 o6 c4");
    sfx_define("sst_hatch", CH_P2, 220, "@42 v9 o6 c32 o5 g32");
    sfx_define("sst_bossin", CH_P1, 160, "@23 v12 o3 c8 g8 o4 c4");
    sfx_define("sst_mouse", CH_P2, 240, "@20 v8 o7 c32 c32 c32");
    sfx_define("sst_fire", CH_NOISE, 220, "@36 v8 o6 c16");
    sfx_define("sst_cough", CH_NOISE, 200, "@13 v10 o3 c16");
    sfx_define("sst_spikes", CH_P2, 200, "@40 v9 o5 c16 c16");
    sfx_define("sst_blink", CH_P2, 220, "@39 v9 o6 c32 o5 c32 o6 c32");
    sfx_define("sst_egg", CH_P2, 220, "@35 v8 o5 g16");
    sfx_define("sst_shut", CH_P1, 200, "@37 v11 o5 c16 o4 c16");
    sfx_define("sst_heart", CH_P1, 140, "@23 v13 o3 c8 e-8 g8 o4 c4");
    sfx_define("sst_bosshit", CH_NOISE, 240, "@21 v8 o5 c32");
    sfx_define("sst_bossdie", CH_NOISE, 100, "@34 v15 o3 c1");
    sfx_define("sst_chomp", CH_P1, 180, "@37 v12 o4 c16 o3 g16 c8");
    sfx_define("sst_coin", CH_P1, 240, "@35 v10 o6 e16 b16");
    sfx_define("sst_bigcoin", CH_P1, 220, "@35 v12 o6 c16 e16 g16 o7 c16");
    sfx_define("sst_oneup", CH_P1, 180, "@39 v13 o5 l16 e g o6 e c d g8");
}
