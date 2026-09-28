/* DOT & DASH - original music (UFO-MML) and sound effects. Every channel of
 * a looping tune is the same number of bars. */
#include "dotdash.h"

int DD_MUS[MU_COUNT] = {-1};
int DD_JINGLE_UP = -1, DD_JINGLE_DEAD = -1;

#define B4(r, f) r "4 " f "4 " r "4 " f "4 "

/* "Dot & Dash" - the title, C major */
static const char TITLE_L[] =
    "@14 v12 q6 o5 e8 g8 o6 c4 o5 b8 g8 e4 | o5 f8 a8 o6 c4 o5 a8 f8 d4 | o5 e8 g8 o6 c8 e8 d4 c4 | o5 b8 o6 c8 d8 o5 b8 g2"
    "| o5 a8 o6 c8 e4 d8 c8 o5 a4 | o5 g8 b8 o6 d4 c8 o5 b8 g4 | o5 f8 e8 d8 e8 f4 g4 | o5 c2 r2";
static const char TITLE_P[] = "@16 v6 q7 o4 c1 f1 c1 g1 @17 a1 @16 g1 f1 c1";
static const char TITLE_T[] = "@6 v13 q5 " B4("o3 c", "o2 g") B4("o3 f", "o3 c") B4("o3 c", "o2 g") B4("o2 g", "o3 d")
                              B4("o3 a", "o3 e") B4("o2 g", "o3 d") B4("o3 f", "o3 c") B4("o3 c", "o2 g");
static const char TITLE_N[] = "[@9 v5 o8 c8 @11 v7 o6 c8]32";

/* "Full Size" - the room at full size, a music box in F */
static const char ROOM_L[] =
    "@15 v10 q7 o5 f4 a4 o6 c4 o5 a4 | o5 g4 b-4 o6 d2 | o6 c4 o5 a4 f4 a4 | o5 g2. r4"
    "| o5 a4 o6 c4 f4 e4 | o6 d4 c4 o5 b-4 g4 | o5 a4 g4 e4 g4 | o5 f2. r4";
static const char ROOM_P[] = "@4 v5 q8 o4 f1 g1 f1 c1 f1 b-1 c1 f1";
static const char ROOM_T[] = "@6 v12 q6 o3 f2 c2 o2 g2 o3 d2 f2 c2 c2 o2 g2 o3 f2 a2 o2 b-2 o3 f2 c2 o2 g2 o3 f1";
static const char ROOM_N[] = "[@21 v2 o7 c4]32";

/* "Button High" - small in the big room, G major */
#define BB8(r, f) r "8 " f "8 " r "8 " f "8 " r "8 " f "8 " r "8 " f "8 "
static const char SMALL_L[] =
    "@1 v11 q5 o5 g8 b8 o6 d8 o5 b8 g4 d4 | o5 e8 g8 b8 g8 e4 c4 | o5 d8 f+8 a8 f+8 d4 a4 | o5 g8 a8 b8 a8 g2"
    "| o5 g8 b8 o6 d8 g8 f+4 e4 | o6 d8 c8 o5 b8 a8 g4 e4 | o5 c8 e8 d8 c8 o4 b4 a4 | o4 g2 r2"
    "| o5 b4 o6 c8 d8 e4 d4 | o6 c4 o5 b8 a8 b2 | o5 a4 b8 o6 c8 d4 c4 | o5 b4 a8 g8 a2"
    "| o5 b4 o6 c8 d8 e4 g4 | o6 f+4 e8 d8 e4 c4 | o5 b8 o6 c8 d8 o5 a8 b4 f+4 | o5 g2 r2";
static const char SMALL_P[] = "@16 v5 q7 o4 g1 c1 d1 g1 g1 d1 c1 g1 @17 e1 @16 c1 @17 a1 @16 d1 @17 e1 @16 d1 g1 g1";
static const char SMALL_T[] = "@6 v13 q4 " BB8("o2 g", "o3 d") BB8("o3 c", "o3 g") BB8("o2 d", "o2 a") BB8("o2 g", "o3 d")
                              BB8("o2 g", "o3 d") BB8("o2 d", "o2 a") BB8("o3 c", "o3 g") BB8("o2 g", "o3 d")
                              BB8("o2 e", "o2 b") BB8("o3 c", "o3 g") BB8("o2 a", "o3 e") BB8("o2 d", "o2 a")
                              BB8("o2 e", "o2 b") BB8("o2 d", "o2 a") BB8("o2 g", "o3 d") BB8("o2 g", "o3 d");
static const char SMALL_N[] = "[@13 v10 o2 c8 @9 v5 o8 c8 @11 v8 o6 c8 @9 v5 o8 c8]32";

/* "Speck Country" - the micro world, A minor */
static const char MICRO_L[] =
    "@5 v11 q6 o5 a8 r8 e8 a8 o6 c4 o5 b4 | o5 a8 g8 e8 d8 e2 | o5 f8 r8 a8 o6 c8 d4 c4 | o5 b8 a8 g+8 a8 b2"
    "| o5 a8 r8 e8 a8 o6 c4 e4 | o6 d8 c8 o5 b8 a8 g4 e4 | o5 f8 e8 d8 c8 o4 b4 o5 e4 | o5 a2 r2";
static const char MICRO_P[] =
    "@2 v5 q4 [o4 a8 o5 c8 e8 o4 a8]2 [o4 g8 b8 o5 e8 o4 g8]2 [o4 f8 a8 o5 c8 o4 f8]2 [o4 e8 g+8 b8 o4 e8]2"
    "[o4 a8 o5 c8 e8 o4 a8]2 [o4 g8 b8 o5 d8 o4 g8]2 [o4 f8 a8 o5 d8 o4 f8]2 [o4 e8 g+8 b8 o4 e8]2";
static const char MICRO_T[] =
    "@6 v13 q5 o3 a4 r4 a4 e4 o3 g4 r4 g4 d4 o3 f4 r4 f4 c4 o3 e4 r4 e4 o2 b4"
    " o3 a4 r4 a4 e4 o3 g4 r4 g4 d4 o3 f4 r4 d4 f4 o3 e2 a2";
static const char MICRO_N[] = "[@9 v4 o8 c16 c16 @21 v3 o7 c8]32";

/* "The Motes" - the deep world, D minor */
static const char DEEP_L[] =
    "@5 v10 q7 o4 d4. f8 a2 | o4 g4. b-8 o5 d2 | o4 a4. o5 c8 e4 d4 | o5 c+2. r4"
    "| o5 d4. f8 a4 g4 | o5 f4 e4 d4 c4 | o4 b-4 a4 g4 e4 | o4 d2. r4";
static const char DEEP_P[] = "@4 v5 q8 o3 d1 g1 a1 a1 d1 d1 g1 d1";
static const char DEEP_T[] = "@6 v12 q7 o2 d1 g1 a1 a1 b-1 f1 g1 d1";
static const char DEEP_N[] = "[@10 v2 o8 c4 r4 @21 v2 o6 c4 r4]8";

/* "Moss and Loam" - the micro world in soil and leaf, E minor */
static const char MICRO2_L[] =
    "@1 v11 q5 o5 e8 g8 a8 b8 a4 g4 | o5 e8 d8 e8 g8 a2 | o5 b8 o6 d8 e8 d8 o5 b4 a4 | o5 g8 a8 b8 a8 e2"
    "| o5 e8 g8 a8 b8 o6 d4 e4 | o6 d8 o5 b8 a8 g8 a4 e4 | o5 d8 e8 g8 a8 b4 a4 | o5 e2 r2";
static const char MICRO2_P[] = "@16 v5 q7 o4 e1 a1 b1 e1 e1 d1 g1 e1";
static const char MICRO2_T[] = "@6 v13 q5 " B4("o3 e", "o2 b") B4("o2 a", "o3 e") B4("o2 b", "o3 f+") B4("o3 e", "o2 b")
                               B4("o3 e", "o2 b") B4("o3 d", "o2 a") B4("o2 g", "o3 d") B4("o3 e", "o2 b");
static const char MICRO2_N[] = "[@9 v4 o8 c8 @11 v6 o6 c8]32";

/* "Tin and Glass" - the micro world in metal, glass and dust, C minor */
static const char MICRO3_L[] =
    "@5 v10 q5 o5 c8 r8 c8 e-8 g4 f4 | o5 e-8 d8 c8 d8 e-2 | o5 f8 r8 f8 a-8 o6 c4 o5 b-4 | o5 a-8 g8 f8 g8 g2"
    "| o5 c8 r8 c8 e-8 g4 o6 c4 | o5 b-8 a-8 g8 f8 e-4 c4 | o5 d8 e-8 f8 d8 g4 o4 b4 | o5 c2 r2";
static const char MICRO3_P[] =
    "@2 v5 q4 [o4 c8 e-8 g8 e-8]4 [o4 f8 a-8 o5 c8 o4 a-8]2 [o4 g8 b8 o5 d8 o4 b8]2"
    "[o4 c8 e-8 g8 e-8]4 [o4 g8 b8 o5 d8 o4 b8]2 [o4 c8 e-8 g8 o5 c8]2";
static const char MICRO3_T[] = "@6 v13 q5 " B4("o3 c", "o2 g") B4("o2 e-", "o2 b-") B4("o2 f", "o3 c") B4("o2 g", "o3 d")
                               B4("o3 c", "o2 g") B4("o2 a-", "o3 e-") B4("o2 g", "o3 d") B4("o3 c", "o2 g");
static const char MICRO3_N[] = "[@10 v3 o8 c16 c16 @21 v4 o7 c8]32";

/* "The Dangerous Caves" - F sharp minor, never quite still */
static const char CAVE_L[] =
    "@5 v11 q4 o5 f+8 f+8 r8 f+8 a4 g+4 | o5 f+8 e8 f+8 c+8 d2 | o5 f+8 f+8 r8 a8 b4 a4 | o5 g+8 f+8 e8 g+8 f+2"
    "| o5 f+8 f+8 r8 f+8 o6 c+4 o5 b4 | o5 a8 g+8 f+8 e8 d4 c+4 | o5 d8 e8 f+8 d8 e4 o4 b4 | o5 f+2 r2";
static const char CAVE_P[] = "@16 v5 q7 o4 f+1 d1 f+1 c+1 f+1 d1 b1 f+1";
static const char CAVE_T[] = "@6 v13 q5 " B4("o2 f+", "o3 c+") B4("o2 d", "o2 a") B4("o2 f+", "o3 c+") B4("o2 c+", "o2 g+")
                             B4("o2 f+", "o3 c+") B4("o2 d", "o2 a") B4("o2 b", "o3 f+") B4("o2 f+", "o3 c+");
static const char CAVE_N[] = "[@13 v9 o2 c8 @9 v4 o8 c8 @9 v4 o8 c8 @9 v4 o8 c8]16";

/* "Little Market" - the towns, D major */
static const char TOWN_L[] =
    "@0 v11 q5 o5 d8 f+8 a8 f+8 d8 f+8 a4 | o5 b8 a8 g8 f+8 e2 | o5 c+8 e8 a8 e8 c+8 e8 a4 | o5 g8 f+8 e8 c+8 d2"
    "| o5 f+8 a8 o6 d8 o5 a8 f+8 a8 o6 d4 | o6 e8 d8 c+8 o5 b8 a4 f+4 | o5 g8 f+8 e8 d8 c+8 d8 e4 | o5 d2 r2";
static const char TOWN_P[] = "@22 v6 q3 [o4 f+8 a8]4 [o4 g8 b8]4 [o4 e8 a8]4 [o4 f+8 a8]4 [o4 f+8 a8]4 [o4 g8 b8]4 [o4 e8 g8]4 [o4 f+8 a8]4";
static const char TOWN_T[] = "@6 v13 q5 " B4("o3 d", "o3 a") B4("o3 g", "o3 d") B4("o3 a", "o3 e") B4("o3 d", "o3 a")
                             B4("o3 d", "o3 a") B4("o3 g", "o3 d") B4("o3 a", "o3 e") "o3 d2 d2";
static const char TOWN_N[] = "[@11 v7 o6 c8 @9 v4 o8 c8]32";

/* "Between the Walls" - E minor */
static const char WALLS_L[] =
    "@3 v10 q4 o4 e8 e8 g8 e8 a8 e8 b8 a8 | o4 g8 g8 b8 g8 o5 d8 c8 o4 b8 a8 | o4 e8 e8 g8 e8 a8 e8 b8 o5 c8 | o5 d8 c8 o4 b8 a8 g4 f+4"
    "| o4 e8 e8 g8 e8 a8 e8 b8 a8 | o5 c8 o4 b8 a8 g8 f+8 g8 a8 f+8 | o4 g8 f+8 e8 d8 e8 f+8 g8 b8 | o4 e2 r2";
static const char WALLS_P[] = "@17 v5 q8 o4 e1 g1 e1 b1 e1 a1 g1 e1";
static const char WALLS_T[] = "@6 v14 q4 [o2 e8 o3 e8]4 [o2 g8 o3 g8]4 [o2 e8 o3 e8]4 [o2 b8 o3 b8]4 [o2 e8 o3 e8]4 [o2 a8 o3 a8]4 [o2 g8 o3 g8]4 [o2 e8 o3 e8]4";
static const char WALLS_N[] = "[@13 v11 o2 c8 @9 v4 o8 c8 @11 v9 o6 c8 @9 v4 o8 c8]16";

/* "Big Trouble" - the bosses, C minor */
static const char BOSS_L[] =
    "@1 v12 q4 o5 c8 c8 e-8 c8 g8 c8 f+8 g8 | o5 a-8 g8 f8 e-8 f4 d4 | o5 c8 c8 e-8 c8 g8 c8 b-8 g8 | o5 a-8 b-8 o6 c8 d8 e-4 d4"
    "| o6 c8 o5 b-8 a-8 g8 a-8 g8 f8 e-8 | o5 f8 e-8 d8 c8 d4 o4 b4 | o5 c8 d8 e-8 f8 g8 a-8 b8 o6 c8 | o6 c4 o5 g4 c2";
static const char BOSS_P[] = "@18 v6 q4 [o4 c8 c8]4 [o3 a-8 a-8]4 [o4 c8 c8]4 [o3 a-8 b-8]4 [o3 a-8 a-8]4 [o3 f8 g8]4 [o4 c8 c8]4 [o3 g8 g8]4";
static const char BOSS_T[] = "@6 v14 q4 [o2 c8 o3 c8]4 [o2 a-8 o3 a-8]4 [o2 c8 o3 c8]4 [o2 a-8 b-8]4 [o2 a-8 o3 a-8]4 [o2 f8 g8]4 [o2 c8 o3 c8]4 [o2 g8 o3 g8]4";
static const char BOSS_N[] = "[@13 v12 o2 c8 @11 v10 o6 c8]32";

/* "Latchtown" - the cat queen's town, a sly march in B flat */
static const char LATCH_L[] =
    "@23 v11 q6 o4 b-4 o5 d8 f8 b-4 f4 | o5 g8 f8 e-8 d8 c2 | o5 d4 f8 a8 o6 c4 o5 a4 | o5 b-8 a8 g8 f8 f2"
    "| o5 f4 b-8 o6 d8 f4 d4 | o6 e-8 d8 c8 o5 b-8 a4 f4 | o5 g8 a8 b-8 o6 c8 d4 c4 | o5 b-2 r2";
static const char LATCH_P[] = "@16 v6 q3 o4 [b-8 r8]4 [e-8 r8]4 [f8 r8]4 [b-8 r8]4 [b-8 r8]4 [e-8 r8]4 [f8 r8]4 [b-8 r8]4";
static const char LATCH_T[] = "@6 v13 q5 " B4("o2 b-", "o2 f") B4("o2 e-", "o2 b-") B4("o2 f", "o3 c") B4("o2 b-", "o2 f")
                              B4("o2 b-", "o2 f") B4("o2 e-", "o2 b-") B4("o2 f", "o3 c") "o2 b-2 b-2";
static const char LATCH_N[] = "[@11 v9 o6 c16 v6 c16 v9 c8 @13 v10 o2 c8 @9 v4 o8 c8]16";

/* "Siege!" - the board game, G minor */
static const char SIEGE_L[] =
    "@23 v12 q5 o4 g8 g8 b-8 o5 d8 g4 d4 | o5 e-8 d8 c8 o4 b-8 a2 | o4 g8 g8 b-8 o5 d8 g4 a4 | o5 b-8 a8 g8 f+8 g2"
    "| o5 d8 d8 f8 a8 o6 d4 c4 | o5 b-8 a8 g8 f8 e-4 d4 | o5 c8 d8 e-8 c8 d4 f+4 | o5 g2 r2";
static const char SIEGE_P[] = "@17 v5 q6 o4 g1 @16 e-1 @17 g1 g1 @16 f1 e-1 @17 c1 g1";
static const char SIEGE_T[] = "@6 v14 q4 [o2 g8 o3 d8]4 [o2 e-8 b-8]4 [o2 g8 o3 d8]4 [o2 d8 a8]4 [o2 f8 o3 c8]4 [o2 e-8 b-8]4 [o2 c8 g8]4 [o2 g8 o3 d8]4";
static const char SIEGE_N[] = "[@11 v10 o6 c16 c16 c8 @13 v12 o2 c8 @11 v9 o6 c8]16";

/* "The Party" - the way out, C major */
static const char END_L[] =
    "@14 v12 q6 o5 c8 e8 g8 o6 c8 o5 b4 g4 | o5 a8 o6 c8 f8 e8 d2 | o5 b8 o6 d8 g8 f8 e4 c4 | o5 a8 b8 o6 c8 d8 e2"
    "| o6 f8 e8 d8 c8 o5 b4 g4 | o5 a8 g8 f8 e8 d4 g4 | o6 c8 o5 b8 a8 g8 f8 e8 d8 g8 | o6 c2 r2";
static const char END_P[] = "@22 v6 q4 [o4 e8 g8]4 [o4 f8 a8]4 [o4 g8 b8]4 [o4 e8 a8]4 [o4 f8 a8]4 [o4 d8 f8]4 [o4 e8 g8]4 [o4 e8 g8]4";
static const char END_T[] = "@6 v13 q5 " B4("o3 c", "o3 g") B4("o3 f", "o3 c") B4("o3 g", "o3 d") B4("o3 a", "o3 e")
                            B4("o3 f", "o3 c") "o3 d4 a4 g4 d4 " B4("o3 c", "o3 g") "o3 c2 c2";
static const char END_N[] = "[@13 v10 o2 c8 @9 v5 o8 c8 @11 v9 o6 c8 @9 v5 o8 c8]16";

/* "In Balance" - the true ending, F major */
static const char TRUE_L[] =
    "@14 v11 q7 o5 c4 f4 a4 g8 f8 | o5 g4 b-4 o6 d2 | o6 c4 o5 a4 f4 g8 a8 | o5 g2. c4"
    "| o5 d4 f4 b-4 a8 g8 | o5 a4 o6 c4 f2 | o6 e4 d4 c4 o5 b-4 | o5 a2 f2";
static const char TRUE_P[] = "@4 v6 q8 o4 f1 g1 a1 c1 b-1 f1 c1 f1";
static const char TRUE_T[] = "@6 v12 q7 o3 f2 c2 o2 g2 o3 d2 f2 a2 c2 o2 g2 b-2 o3 f2 f2 c2 c2 e2 f1";
static const char TRUE_N[] = "[@21 v2 o7 c4 @10 v2 o8 c4]16";

/* jingles */
static const char UP_P1[] = "@23 v12 o5 l16 c e g o6 c8 o5 g16 o6 c4";
static const char UP_P2[] = "@22 v8 o4 l16 g o5 c e g8 e16 g4";
static const char UP_T[] = "@6 v13 o3 c8 g8 c4";
static const char DEAD_P1[] = "@5 v11 o5 l8 e d c o4 g4 c4";
static const char DEAD_T[] = "@6 v12 o3 c8 o2 g8 c4";

void dd_audio_load(void) {
    if (DD_MUS[MU_TITLE] >= 0 && DD_JINGLE_UP >= 0) return;
    DD_MUS[MU_TITLE] = song_define("dd_title", 108, true, TITLE_L, TITLE_P, TITLE_T, TITLE_N);
    DD_MUS[MU_ROOM] = song_define("dd_room", 84, true, ROOM_L, ROOM_P, ROOM_T, ROOM_N);
    DD_MUS[MU_SMALL] = song_define("dd_small", 120, true, SMALL_L, SMALL_P, SMALL_T, SMALL_N);
    DD_MUS[MU_MICRO] = song_define("dd_micro", 112, true, MICRO_L, MICRO_P, MICRO_T, MICRO_N);
    DD_MUS[MU_DEEP] = song_define("dd_deep", 90, true, DEEP_L, DEEP_P, DEEP_T, DEEP_N);
    DD_MUS[MU_TOWN] = song_define("dd_town", 126, true, TOWN_L, TOWN_P, TOWN_T, TOWN_N);
    DD_MUS[MU_WALLS] = song_define("dd_walls", 100, true, WALLS_L, WALLS_P, WALLS_T, WALLS_N);
    DD_MUS[MU_BOSS] = song_define("dd_boss", 150, true, BOSS_L, BOSS_P, BOSS_T, BOSS_N);
    DD_MUS[MU_LATCH] = song_define("dd_latch", 116, true, LATCH_L, LATCH_P, LATCH_T, LATCH_N);
    DD_MUS[MU_SIEGE] = song_define("dd_siege", 140, true, SIEGE_L, SIEGE_P, SIEGE_T, SIEGE_N);
    DD_MUS[MU_ENDING] = song_define("dd_ending", 120, true, END_L, END_P, END_T, END_N);
    DD_MUS[MU_TRUE] = song_define("dd_true", 96, true, TRUE_L, TRUE_P, TRUE_T, TRUE_N);
    DD_MUS[MU_MICRO2] = song_define("dd_micro2", 104, true, MICRO2_L, MICRO2_P, MICRO2_T, MICRO2_N);
    DD_MUS[MU_MICRO3] = song_define("dd_micro3", 116, true, MICRO3_L, MICRO3_P, MICRO3_T, MICRO3_N);
    DD_MUS[MU_CAVE] = song_define("dd_cave", 132, true, CAVE_L, CAVE_P, CAVE_T, CAVE_N);
    DD_JINGLE_UP = song_define("dd_upgrade", 150, false, UP_P1, UP_P2, UP_T, "");
    DD_JINGLE_DEAD = song_define("dd_back", 110, false, DEAD_P1, "", DEAD_T, "");

    sfx_define("dd_jump", CH_P2, 240, "@32 v8 o5 c16");
    sfx_define("dd_boing", CH_P2, 220, "@38 v11 o4 c8 g8");
    sfx_define("dd_block", CH_P2, 220, "@37 v8 o4 c16");
    sfx_define("dd_hurt", CH_P1, 200, "@37 v12 o5 c16 o4 g16");
    sfx_define("dd_die", CH_P1, 160, "@33 v12 o5 g8 e8 c4");
    sfx_define("dd_pop", CH_NOISE, 220, "@36 v10 o6 c16");
    sfx_define("dd_hit", CH_NOISE, 220, "@40 v10 o5 c16");
    sfx_define("dd_shoot", CH_P2, 240, "@20 v8 o6 c32 o5 g32");
    sfx_define("dd_spell", CH_P2, 220, "@39 v9 o6 c16 e16");
    sfx_define("dd_thud", CH_NOISE, 200, "@13 v13 o2 c8");
    sfx_define("dd_boom", CH_NOISE, 140, "@34 v15 o3 c4");
    sfx_define("dd_break", CH_NOISE, 220, "@40 v11 o5 c16 @36 v8 o6 c16");
    sfx_define("dd_freeze", CH_P2, 200, "@39 v10 o6 c16 g16 o7 c8");
    sfx_define("dd_splat", CH_NOISE, 200, "@36 v10 o4 c16 c16");
    sfx_define("dd_lift", CH_P2, 240, "@38 v9 o4 c16 g16");
    sfx_define("dd_throw", CH_NOISE, 240, "@36 v10 o6 c16");
    sfx_define("dd_drop", CH_TRI, 220, "@41 v10 o3 c16");
    sfx_define("dd_kick", CH_NOISE, 240, "@13 v12 o3 c16");
    sfx_define("dd_stow", CH_P2, 220, "@35 v9 o6 e16 c16");
    sfx_define("dd_shrink", CH_P1, 200, "@33 v11 o6 c16 g16 e16 c16 o5 g16 e16 c8");
    sfx_define("dd_grow", CH_P1, 200, "@32 v11 o5 c16 e16 g16 o6 c16 e16 g16 o7 c8");
    sfx_define("dd_door", CH_TRI, 200, "@38 v11 o3 g16 o4 d16");
    sfx_define("dd_charge", CH_P2, 240, "@42 v5 o6 c32");
    sfx_define("dd_glint", CH_P2, 240, "@35 v8 o7 e32");
    sfx_define("dd_glint5", CH_P2, 240, "@35 v9 o6 b32 o7 e16");
    sfx_define("dd_big", CH_P1, 200, "@39 v12 o6 l16 c e g o7 c8");
    sfx_define("dd_heart", CH_P2, 220, "@35 v10 o6 c16 e16 g16");
    sfx_define("dd_flip", CH_P2, 240, "@32 v9 o6 c16 g16");
    sfx_define("dd_command", CH_P2, 220, "@20 v9 o6 g16 o7 c16");
    sfx_define("dd_flap", CH_NOISE, 200, "@36 v8 o6 c16 r16 c16");
    sfx_define("dd_whistle", CH_P1, 200, "@0 v11 o7 c8 o6 g16 o7 e8");
    sfx_define("dd_yelp", CH_P2, 220, "@33 v11 o6 e16 o5 b16");
    sfx_define("dd_fill", CH_P2, 220, "@38 v9 o5 c16 e16 g16");
    sfx_define("dd_stomp", CH_NOISE, 220, "@13 v12 o3 c16 @40 v8 o5 c16");
    sfx_define("dd_give", CH_P2, 220, "@35 v10 o6 g16 o7 c16");
    sfx_define("dd_blip", CH_P2, 240, "@42 v4 o6 g32");
    sfx_define("dd_gulp", CH_TRI, 200, "@41 v11 o3 g16 o2 c8");
    sfx_define("dd_sniff", CH_NOISE, 240, "@36 v6 o7 c32 r32 c32 r32 c32");
}
