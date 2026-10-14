/* LIMPET - original music (UFO-MML) and sound effects. Every looping tune
 * is eight bars a channel, so the channels loop together. Each part of the
 * station's hub has its own tune (the concourse, the canteen's swing, the
 * Gristle wing's drone, the upper ring), the decks share four, and the
 * survival decks, the two bosses and the secret deck have their own. */
#include "lmp.h"

int LMP_MUS_TITLE = -1, LMP_MUS_CONCOURSE, LMP_MUS_CANTEEN, LMP_MUS_GRISTLE, LMP_MUS_UPPER, LMP_MUS_DECK[4],
    LMP_MUS_SURVIVAL, LMP_MUS_TURNCOAT, LMP_MUS_DREAD, LMP_MUS_COMB, LMP_MUS_CLEAR, LMP_MUS_DEATH,
    LMP_MUS_ENDING, LMP_MUS_STORY, LMP_MUS_HONEY;

#define Q4(a, b, c, d) a "4 " b "4 " c "4 " d "4 "
#define E8(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
/* one bar each */
#define HATS "[@9 v4 o8 c8]8 "
#define KICKSN "@13 v11 o2 c8 @9 v4 o8 c8 @11 v8 o6 c8 @9 v4 o8 c8 @13 v11 o2 c8 @9 v4 o8 c8 @11 v8 o6 c8 @9 v4 o8 c8 "
#define SWING "@13 v10 o2 c4 @9 v4 o8 c8. c16 @11 v7 o6 c4 @9 v4 o8 c8. c16 "
#define MARCH "@13 v12 o2 c8 c8 @11 v9 o6 c4 @13 v12 o2 c8 @9 v4 o8 c8 @11 v9 o6 c4 "
#define SPARSE "@9 v3 o8 c4 r4 @21 v4 o7 c4 r4 "
#define HEAVY "@13 v13 o2 c8 c8 @34 v6 o3 c4 @13 v13 o2 c8 @9 v4 o8 c8 @11 v10 o6 c4 "
#define HUM "@13 v8 o2 c4 r4 r4 @21 v3 o7 c8 c8 "

/* "Limpet" - the title, C minor, a little hull tank's march */
static const char TITLE_LEAD[] =
    "@1 v11 q6"
    "| o5 c4 e-8 g8 o6 c4 o5 b-8 g8 | o5 a-4 g8 f8 g2 | o5 e-4 f8 g8 a-4 g8 f8 | o5 g2 r4 g4"
    "| o5 c4 e-8 g8 o6 c4 d8 e-8 | o6 d4 c8 o5 b-8 o6 c2 | o5 a-8 g8 f8 e-8 d4 f4 | o5 c1";
static const char TITLE_ARP[] =
    "@2 v5 q4 " ARP("o4 c", "e-", "g", "e-") ARP("o4 f", "a-", "o5 c", "o4 a-") ARP("o4 e-", "g", "b-", "g")
    ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 c", "e-", "g", "e-") ARP("o4 a-", "o5 c", "e-", "c")
    ARP("o4 f", "a-", "o5 c", "o4 a-") ARP("o4 c", "e-", "g", "o5 c");
static const char TITLE_BASS[] =
    "@6 v12 q6 " Q4("o2 c", "g", "o3 c", "o2 g") Q4("o2 f", "o3 c", "f", "c") Q4("o2 e-", "b-", "o3 e-", "o2 b-")
    Q4("o2 g", "o3 d", "g", "d") Q4("o2 c", "g", "o3 c", "o2 g") Q4("o2 a-", "o3 e-", "a-", "e-")
    Q4("o2 f", "o3 c", "o2 g", "o3 d") Q4("o2 c", "g", "o3 c", "o2 c");
static const char TITLE_DRUM[] = "[" MARCH "]8";

/* "The Crew Fled" - the story, A minor */
static const char STORY_LEAD[] =
    "@5 v10 q7"
    "| o5 e2 d4 c4 | o4 b2. a4 | o5 c4 d4 e4 g4 | o5 a1 | o5 g2 f4 e4 | o5 d2. c4 | o4 b4 o5 c4 d4 o4 b4 | o4 a1";
static const char STORY_PAD[] = "@4 v5 q8 o4 a1 g1 f1 e1 c1 d1 e1 a1";
static const char STORY_BASS[] = "@6 v11 q8 o2 a1 g1 f1 a1 c1 d1 e1 a1";
static const char STORY_SOFT[] = "[@9 v2 o8 c2]16";

/* "Concourse" - the hub's first rooms, F major, easy going */
static const char CON_LEAD[] =
    "@14 v10 q7"
    "| o5 f4 a8 o6 c8 o5 b-4 a4 | o5 g4 a8 b-8 a2 | o5 d4 f8 a8 g4 f4 | o5 e2. r4"
    "| o5 f4 a8 o6 c8 d4 c4 | o5 b-4 a8 g8 a2 | o5 g8 a8 b-8 g8 e4 g4 | o5 f1";
static const char CON_PAD[] = "@4 v5 q8 o4 a1 c1 f1 g1 a1 d1 e1 f1";
static const char CON_BASS[] =
    "@6 v12 q6 " Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 c", "g", "c", "g") Q4("o2 d", "a", "d", "a")
    Q4("o2 c", "g", "c", "e") Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 b-", "o3 f", "o2 b-", "o3 f")
    Q4("o2 c", "g", "c", "g") Q4("o2 f", "o3 c", "o2 f", "f");
static const char CON_DRUM[] = "[" HATS "]8";

/* "Canteen Swing" - the cafe zone, B-flat, a lounge band */
static const char CAN_LEAD[] =
    "@23 v11 q6"
    "| o5 b-8. o6 d16 f4 e-8. d16 c4 | o5 b-4 a8. b-16 o6 c2 | o5 g8. a16 b-8. o6 c16 d4 c4 | o5 b-2 r2"
    "| o5 b-8. o6 d16 f4 g8. f16 e-4 | o6 d4 c8. o5 b-16 a2 | o5 g8. b-16 o6 d8. c16 o5 a4 f4 | o5 b-1";
static const char CAN_COMP[] =
    "@16 v5 q3 [o4 b-8 r8]4 [o4 e-8 r8]4 [o4 g8 r8]4 [o4 b-8 r8]4 [o4 b-8 r8]4 [o4 f8 r8]4 [o4 e-8 r8]4 [o4 b-8 r8]4";
static const char CAN_BASS[] =
    "@7 v12 q5 " Q4("o2 b-", "o3 d", "f", "a-") Q4("o2 e-", "g", "b-", "a") Q4("o2 g", "b-", "o3 d", "c")
    Q4("o2 b-", "a", "a-", "f") Q4("o2 b-", "o3 d", "f", "e-") Q4("o3 d", "c", "o2 b-", "a")
    Q4("o2 e-", "f", "g", "f") Q4("o2 b-", "f", "d", "f");
static const char CAN_DRUM[] = "[" SWING "]8";

/* "The Gristle Wing" - the infested rooms, E with a flat second, slow */
static const char GRI_LEAD[] =
    "@5 v10 q8"
    "| o4 e2 f2 | o4 e4 g4 f2 | o4 e2 b-2 | o4 a1 | o4 e2 f2 | o4 g4 a4 b-2 | o4 a4 g4 f4 e4 | o4 e1";
static const char GRI_DRONE[] = "@4 v4 q8 [o3 e1]8";
static const char GRI_BASS[] = "@6 v12 q8 o2 [e2 f2]8";
static const char GRI_DRUM[] = "[" HUM "]8";

/* "Upper Ring" - the last rooms, D dorian, driving */
static const char UPP_LEAD[] =
    "@1 v11 q6"
    "| o5 d8 e8 f8 a8 g4 f8 e8 | o5 d4 a4 o6 c4 o5 a4 | o5 g8 a8 b8 o6 c8 d4 c8 o5 b8 | o5 a2 r2"
    "| o5 d8 e8 f8 a8 o6 c4 o5 b8 a8 | o5 g4 f8 g8 a2 | o5 f8 e8 d8 e8 f4 e4 | o5 d1";
static const char UPP_ARP[] =
    "@3 v5 q4 " ARP("o4 d", "f", "a", "f") ARP("o4 d", "f", "a", "o5 c") ARP("o4 g", "b", "o5 d", "o4 b")
    ARP("o4 a", "o5 c", "e", "c") ARP("o4 d", "f", "a", "f") ARP("o4 g", "b", "o5 d", "o4 b")
    ARP("o4 c", "e", "g", "e") ARP("o4 d", "f", "a", "o5 d");
static const char UPP_BASS[] =
    "@6 v13 q5 " E8("o2 d") E8("o2 d") E8("o2 g") E8("o2 a") E8("o2 d") E8("o2 g") E8("o2 c") E8("o2 d");
static const char UPP_DRUM[] = "[" MARCH "]8";

/* "Hull Crawl" - decks, A minor */
static const char D0_LEAD[] =
    "@14 v11 q7"
    "| o5 a4 o6 c8 e8 d4 c4 | o5 b4 g8 b8 a2 | o5 f4 a8 o6 c8 o5 b4 a4 | o5 g+2 e2"
    "| o5 a4 o6 c8 e8 g4 f4 | o6 e4 d8 c8 d2 | o6 c8 o5 b8 a8 g+8 a4 b4 | o5 a1";
static const char D0_ARP[] =
    "@2 v5 q4 " ARP("o4 a", "o5 c", "e", "c") ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 f", "a", "o5 c", "o4 a")
    ARP("o4 e", "g+", "b", "g+") ARP("o4 a", "o5 c", "e", "c") ARP("o4 d", "f", "a", "f")
    ARP("o4 e", "g+", "b", "g+") ARP("o4 a", "o5 c", "e", "a");
static const char D0_BASS[] =
    "@6 v12 q6 " Q4("o2 a", "o3 e", "o2 a", "o3 e") Q4("o2 g", "o3 d", "o2 g", "o3 d") Q4("o2 f", "o3 c", "o2 f", "o3 c")
    Q4("o2 e", "b", "e", "g+") Q4("o2 a", "o3 e", "o2 a", "o3 e") Q4("o2 d", "a", "d", "a")
    Q4("o2 e", "b", "e", "b") Q4("o2 a", "o3 e", "o2 a", "a");
static const char D0_DRUM[] = "[" KICKSN "]8";

/* "Pressure" - decks, E minor, staccato */
static const char D1_LEAD[] =
    "@3 v11 q5"
    "| o5 e8 e8 g8 e8 b8 e8 a8 g8 | o5 f+8 f+8 a8 f+8 o6 c8 o5 f+8 b8 a8 | o5 g8 g8 b8 g8 o6 e8 o5 g8 o6 d8 c8 | o5 b4 a4 g4 f+4"
    "| o5 e8 e8 g8 e8 b8 e8 a8 g8 | o5 c8 c8 e8 c8 a8 c8 g8 f+8 | o5 d8 f+8 a8 o6 c8 o5 b8 a8 g8 f+8 | o5 e2 r2";
static const char D1_PAD[] = "@4 v4 q8 o4 g1 a1 b1 b1 g1 e1 f+1 e1";
static const char D1_BASS[] =
    "@6 v13 q4 " E8("o2 e") E8("o2 d") E8("o2 c") E8("o2 b") E8("o2 e") E8("o2 c") E8("o2 d") E8("o2 e");
static const char D1_DRUM[] = "[" MARCH "]8";

/* "Cold Iron" - decks, D minor, slow */
static const char D2_LEAD[] =
    "@19 v12 q7"
    "| o5 d2 f4 e4 | o5 d4 c4 o4 a2 | o4 b-2 o5 d4 c4 | o4 a1 | o5 d2 f4 a4 | o5 g4 f4 e2 | o5 f4 e4 d4 c+4 | o5 d1";
static const char D2_PAD[] = "@4 v5 q8 o4 f1 f1 d1 c+1 f1 b-1 a1 f1";
static const char D2_BASS[] =
    "@6 v11 q7 " Q4("o2 d", "a", "d", "a") Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 b-", "o3 f", "o2 b-", "o3 f")
    Q4("o2 a", "o3 e", "o2 a", "o3 e") Q4("o2 d", "a", "d", "a") Q4("o2 g", "o3 d", "o2 g", "o3 d")
    Q4("o2 a", "o3 e", "o2 a", "o3 c+") Q4("o2 d", "a", "d", "d");
static const char D2_DRUM[] = "[@13 v10 o2 c4 r4 @11 v6 o6 c4 r4]8";

/* "Deep Decks" - decks, G minor */
static const char D3_LEAD[] =
    "@0 v11 q6"
    "| o5 g4 b-8 o6 d8 c4 o5 b-4 | o5 a4 f8 a8 g2 | o5 e-4 g8 b-8 a4 g4 | o5 f+2 d2"
    "| o5 g4 b-8 o6 d8 f4 e-4 | o6 d4 c8 o5 b-8 o6 c2 | o5 b-8 a8 g8 f+8 g4 a4 | o5 g1";
static const char D3_ARP[] =
    "@17 v5 q3 [o4 g8 r8]4 [o4 f8 r8]4 [o4 e-8 r8]4 [o4 d8 r8]4 [o4 g8 r8]4 [o4 c8 r8]4 [o4 d8 r8]4 [o4 g8 r8]4";
static const char D3_BASS[] =
    "@6 v12 q5 " Q4("o2 g", "o3 d", "o2 g", "o3 d") Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 e-", "b-", "e-", "b-")
    Q4("o2 d", "a", "d", "f+") Q4("o2 g", "o3 d", "o2 g", "o3 d") Q4("o2 c", "g", "c", "g")
    Q4("o2 d", "a", "d", "a") Q4("o2 g", "o3 d", "o2 g", "g");
static const char D3_DRUM[] = "[" KICKSN "]8";

/* "Hold the Line" - the survival decks, C minor, fast */
static const char SUR_LEAD[] =
    "@1 v11 q5"
    "[| o5 c8 c8 e-8 c8 g8 c8 f8 e-8 | o5 d8 d8 f8 d8 a-8 d8 g8 f8 | o5 e-8 e-8 g8 e-8 b-8 e-8 a-8 g8 | o5 f4 e-4 d4 o4 b4]2";
static const char SUR_ARP[] = "@18 v5 q3 [o4 c8 r8 c8 r8 d8 r8 e-8 r8 | o4 f8 r8 f8 r8 g8 r8 b8 r8]4";
static const char SUR_BASS[] = "@6 v13 q4 [" E8("o2 c") E8("o2 b-") E8("o2 a-") E8("o2 g") "]2";
static const char SUR_DRUM[] = "[" MARCH "]8";

/* "Turncoat" - the mid-game boss, F-sharp minor */
static const char TUR_LEAD[] =
    "@1 v12 q5"
    "[| o5 f+8 a8 o6 c+8 o5 a8 f+8 a8 o6 c+8 e8 | o6 d8 c+8 o5 b8 a8 b4 g+4 | o5 f+8 a8 o6 c+8 o5 a8 o6 d8 c+8 o5 b8 a8 | o5 g+2 f2]2";
static const char TUR_ARP[] = "@17 v5 q3 [[o4 f+16 r16]8 [o4 e16 r16]8 [o4 d16 r16]8 [o4 c+16 r16]8]2";
static const char TUR_BASS[] = "@6 v13 q4 [" E8("o2 f+") E8("o2 b") E8("o2 d") E8("o2 c+") "]2";
static const char TUR_DRUM[] = "[" HEAVY "]8";

/* "Dreadnought" - the final boss, C with a flat second, heavy brass */
static const char DRE_LEAD[] =
    "@23 v12 q7"
    "| o4 c4 d-4 c4 g4 | o4 a-4 g4 f4 d-4 | o4 c4 d-4 e-4 f4 | o4 g2 d-2 | o4 c4 d-4 c4 b-4 | o4 a-4 b-4 o5 c4 d-4 | o5 c4 o4 b-4 a-4 g4 | o4 c1";
static const char DRE_STAB[] =
    "@16 v6 q2 [o4 c16 r16]8 [o4 d-16 r16]8 [o4 c16 r16]8 [o3 g16 r16]8 [o4 c16 r16]8 [o3 a-16 r16]8 [o3 g16 r16]8 [o4 c16 r16]8";
static const char DRE_BASS[] =
    "@6 v14 q4 " E8("o2 c") E8("o2 d-") E8("o2 c") E8("o1 g") E8("o2 c") E8("o1 a-") E8("o1 g") E8("o2 c");
static const char DRE_DRUM[] = "[" HEAVY "]8";

/* "The Comb" - the secret deck, A major, strange and sweet */
static const char COMB_LEAD[] =
    "@15 v11 q6"
    "| o6 e4 c+8 o5 a8 b4 o6 c+4 | o6 d4 o5 b8 g+8 a2 | o6 e4 f+8 g+8 a4 g+4 | o6 f+2 e2"
    "| o6 c+4 e8 a8 g+4 f+4 | o6 e4 d8 c+8 o5 b2 | o5 a8 b8 o6 c+8 d8 e4 g+4 | o6 a1";
static const char COMB_ARP[] =
    "@39 v4 q4 " ARP("o5 a", "o6 c+", "e", "c+") ARP("o5 b", "o6 d", "e", "d") ARP("o5 a", "o6 c+", "e", "c+")
    ARP("o5 b", "o6 d", "f+", "d") ARP("o5 a", "o6 c+", "e", "c+") ARP("o5 g+", "b", "o6 e", "o5 b")
    ARP("o5 f+", "a", "o6 d", "o5 a") ARP("o5 a", "o6 c+", "e", "a");
static const char COMB_BASS[] =
    "@7 v11 q6 " Q4("o2 a", "o3 e", "o2 a", "o3 e") Q4("o2 e", "b", "e", "b") Q4("o2 a", "o3 e", "o2 a", "o3 e")
    Q4("o2 d", "a", "e", "b") Q4("o2 a", "o3 e", "o2 a", "o3 e") Q4("o2 e", "b", "e", "g+")
    Q4("o2 d", "a", "e", "b") Q4("o2 a", "o3 e", "o2 a", "a");
static const char COMB_DRUM[] = "[" SPARSE "]8";

/* "Quiet Orbit" - the endings, C major */
static const char END_LEAD[] =
    "@14 v10 q7"
    "| o5 e4 g8 o6 c8 o5 b4 g4 | o5 a4 f8 a8 g2 | o5 e4 g8 o6 c8 e4 d4 | o6 c2 o5 g2"
    "| o5 f4 a8 o6 c8 o5 b4 a4 | o5 g4 e8 g8 f2 | o5 d8 e8 f8 g8 a4 b4 | o6 c1";
static const char END_ARP[] =
    "@2 v4 q4 " ARP("o4 c", "e", "g", "e") ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 c", "e", "g", "e")
    ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 c", "e", "g", "e")
    ARP("o4 d", "f", "a", "f") ARP("o4 c", "e", "g", "o5 c");
static const char END_BASS[] =
    "@6 v11 q7 " Q4("o2 c", "g", "c", "g") Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 c", "g", "c", "g")
    Q4("o2 g", "o3 d", "o2 g", "o3 d") Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 c", "g", "c", "g")
    Q4("o2 d", "a", "g", "b") Q4("o2 c", "g", "c", "c");
static const char END_DRUM[] = "[@9 v3 o8 c8 r8 c8 r8 @11 v4 o6 c8 r8 @9 v3 o8 c8 r8]8";

/* jingles */
static const char CLEAR_P1[] = "@1 v12 o5 l8 c e g o6 c4 o5 g8 o6 c8 e2";
static const char CLEAR_P2[] = "@4 v7 o4 l8 g o5 c e g4 e8 g8 c2";
static const char CLEAR_TRI[] = "@6 v12 o2 c4 g4 o3 c4 o2 g4 c2";
static const char CLEAR_NOISE[] = "@13 v10 o2 c4 c4 c4 c4 @12 v8 o5 c2";
static const char DEATH_P1[] = "@5 v11 o5 e4 d4 c4 o4 b4 a1";
static const char DEATH_TRI[] = "@6 v12 o2 a2 g2 e1";
static const char HONEY_P1[] = "@15 v12 o6 l16 c e g o7 c e4 r8";
static const char HONEY_P2[] = "@39 v8 o6 l16 e g o7 c e g4 r8";

void lmp_audio_load(void) {
    if (LMP_MUS_TITLE >= 0) return;
    LMP_MUS_TITLE = song_define("lmp_title", 120, true, TITLE_LEAD, TITLE_ARP, TITLE_BASS, TITLE_DRUM);
    LMP_MUS_STORY = song_define("lmp_story", 80, true, STORY_LEAD, STORY_PAD, STORY_BASS, STORY_SOFT);
    LMP_MUS_CONCOURSE = song_define("lmp_concourse", 104, true, CON_LEAD, CON_PAD, CON_BASS, CON_DRUM);
    LMP_MUS_CANTEEN = song_define("lmp_canteen", 132, true, CAN_LEAD, CAN_COMP, CAN_BASS, CAN_DRUM);
    LMP_MUS_GRISTLE = song_define("lmp_gristle", 84, true, GRI_LEAD, GRI_DRONE, GRI_BASS, GRI_DRUM);
    LMP_MUS_UPPER = song_define("lmp_upper", 140, true, UPP_LEAD, UPP_ARP, UPP_BASS, UPP_DRUM);
    LMP_MUS_DECK[0] = song_define("lmp_deck_a", 120, true, D0_LEAD, D0_ARP, D0_BASS, D0_DRUM);
    LMP_MUS_DECK[1] = song_define("lmp_deck_b", 150, true, D1_LEAD, D1_PAD, D1_BASS, D1_DRUM);
    LMP_MUS_DECK[2] = song_define("lmp_deck_c", 96, true, D2_LEAD, D2_PAD, D2_BASS, D2_DRUM);
    LMP_MUS_DECK[3] = song_define("lmp_deck_d", 126, true, D3_LEAD, D3_ARP, D3_BASS, D3_DRUM);
    LMP_MUS_SURVIVAL = song_define("lmp_survival", 160, true, SUR_LEAD, SUR_ARP, SUR_BASS, SUR_DRUM);
    LMP_MUS_TURNCOAT = song_define("lmp_turncoat", 150, true, TUR_LEAD, TUR_ARP, TUR_BASS, TUR_DRUM);
    LMP_MUS_DREAD = song_define("lmp_dread", 120, true, DRE_LEAD, DRE_STAB, DRE_BASS, DRE_DRUM);
    LMP_MUS_COMB = song_define("lmp_comb", 100, true, COMB_LEAD, COMB_ARP, COMB_BASS, COMB_DRUM);
    LMP_MUS_ENDING = song_define("lmp_ending", 92, true, END_LEAD, END_ARP, END_BASS, END_DRUM);
    LMP_MUS_CLEAR = song_define("lmp_clear", 132, false, CLEAR_P1, CLEAR_P2, CLEAR_TRI, CLEAR_NOISE);
    LMP_MUS_DEATH = song_define("lmp_death", 110, false, DEATH_P1, "", DEATH_TRI, "");
    LMP_MUS_HONEY = song_define("lmp_honey", 120, false, HONEY_P1, HONEY_P2, "", "");

    sfx_define("lmp_shot", CH_P2, 240, "@20 v6 o6 c32 o5 g32");
    sfx_define("lmp_warp", CH_P1, 240, "@38 v11 o4 c32 g32 o5 c32 g32 o6 c16");
    sfx_define("lmp_fizzle", CH_NOISE, 240, "@21 v6 o6 c32 r32 c32");
    sfx_define("lmp_hop", CH_P1, 240, "@32 v9 o4 c32 g32");
    sfx_define("lmp_push", CH_NOISE, 220, "@13 v8 o3 c16");
    sfx_define("lmp_break", CH_NOISE, 200, "@34 v9 o4 c16");
    sfx_define("lmp_solid", CH_P2, 240, "@15 v9 o6 g32 o7 c32");
    sfx_define("lmp_swap", CH_P2, 220, "@42 v9 o5 c32 g32 c32");
    sfx_define("lmp_door", CH_NOISE, 160, "@40 v9 o4 c8");
    sfx_define("lmp_tick", CH_P2, 240, "@42 v7 o6 g32");
    sfx_define("lmp_tink", CH_P2, 240, "@42 v6 o7 c32");
    sfx_define("lmp_check", CH_P1, 200, "@35 v10 o6 c16 g16");
    sfx_define("lmp_reveal", CH_P1, 200, "@39 v11 o6 l32 c d e f g a b o7 c");
    sfx_define("lmp_honey", CH_P1, 200, "@39 v12 o6 l32 e g o7 c e");
    sfx_define("lmp_portal", CH_P1, 220, "@38 v10 o5 c32 e32 g32 o6 c32");
    sfx_define("lmp_hit", CH_NOISE, 240, "@21 v8 o6 c32");
    sfx_define("lmp_pop", CH_NOISE, 200, "@34 v9 o4 c16");
    sfx_define("lmp_aim", CH_P2, 240, "@42 v8 o6 c32 r32 c32");
    sfx_define("lmp_turret", CH_P2, 220, "@37 v8 o4 g16");
    sfx_define("lmp_laser", CH_P2, 240, "@33 v9 o7 c16");
    sfx_define("lmp_flame", CH_NOISE, 160, "@36 v9 o3 c4");
    sfx_define("lmp_beam", CH_P2, 220, "@39 v7 o6 e16 c16");
    sfx_define("lmp_ring", CH_P2, 220, "@15 v8 o6 c16 e16");
    sfx_define("lmp_spawn", CH_NOISE, 200, "@36 v6 o5 c16");
    sfx_define("lmp_boom", CH_NOISE, 140, "@34 v13 o3 c4");
    sfx_define("lmp_charge", CH_P1, 180, "@38 v10 o3 c8 g8 o4 c8");
    sfx_define("lmp_shell", CH_NOISE, 160, "@34 v12 o2 c8");
    sfx_define("lmp_corehit", CH_P1, 240, "@37 v11 o5 c16 o4 c16");
    sfx_define("lmp_bossdie", CH_NOISE, 90, "@34 v15 o2 c2");
    sfx_define("lmp_wave", CH_P1, 180, "@33 v10 o5 c8 o4 g8");
    sfx_define("lmp_paint", CH_P1, 200, "@39 v10 o5 l32 c e g o6 c");
    sfx_define("lmp_enter", CH_P1, 200, "@38 v11 o4 c16 g16 o5 c16 g16");
    sfx_define("lmp_open", CH_P1, 180, "@15 v12 o5 g16 o6 c16 e16 g8");
    sfx_define("lmp_clear", CH_P1, 160, "@39 v13 o5 l16 c e g o6 c4");
    sfx_define("lmp_die", CH_P1, 140, "@33 v13 o5 c8 o4 a8 f8 d4");
    sfx_define("lmp_talk", CH_P2, 240, "@42 v5 o6 c32");
}
