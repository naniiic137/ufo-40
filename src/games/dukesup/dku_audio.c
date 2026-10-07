/* DUKES UP - original music (UFO-MML) and sound effects. Every looping
 * tune is eight bars a channel, so the channels loop together. */
#include "dku.h"

int DKU_MUS_TITLE = -1, DKU_MUS_SELECT, DKU_MUS_N1, DKU_MUS_N2, DKU_MUS_N3, DKU_MUS_N4, DKU_MUS_N5, DKU_MUS_BOSS,
    DKU_MUS_GRIST, DKU_MUS_SHOP, DKU_MUS_GYM, DKU_MUS_CLEAR, DKU_MUS_DOWN, DKU_MUS_GOOD, DKU_MUS_BAD, DKU_MUS_LIFT;

/* one bar of quarter notes, of eighths, of off-beat stabs */
#define Q4(a, b, c, d) a "4 " b "4 " c "4 " d "4 "
#define EI(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define OFFB(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
#define ST16(n) "[" n "16 r16]8 "
#define OCT(n) "[o2 " n "8 o3 " n "8]4 "
#define OCT1(n) "[o1 " n "8 o2 " n "8]4 "
/* half a bar of kick, hat, snare, hat */
#define DR2 "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
#define DRB "@13 v12 o2 c8 @13 v9 o2 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
#define SHUF "@13 v11 o2 c8. @9 v5 o8 c16 @11 v9 o6 c8. @9 v5 o8 c16 "

/* "Dukes Up" - the title, A minor */
static const char TITLE_LEAD[] =
    "@1 v11 q6"
    "| o5 a8 a8 o6 c8 o5 a8 g8 a8 e4 | o5 f8 f8 a8 f8 e8 d8 e4 | o5 a8 a8 o6 c8 d8 e8 d8 c8 o5 b8 | o5 a2 r2"
    "| o6 e8 e8 d8 c8 o5 b8 o6 c8 d4 | o6 c8 o5 b8 a8 g8 f8 g8 a4 | o5 e8 f8 g8 a8 b8 o6 c8 d8 e8 | o6 e4 o5 e4 a2";
static const char TITLE_COMP[] =
    "v6 q3 " OFFB("@17 o4", "a") OFFB("@16 o4", "f") OFFB("@17 o4", "a") OFFB("@16 o4", "e")
    OFFB("@16 o4", "c") OFFB("@16 o4", "f") OFFB("@17 o4", "g") OFFB("@17 o4", "a");
static const char TITLE_BASS[] =
    "@6 v14 q4 " EI("o2 a") EI("o2 f") EI("o2 a") EI("o2 e") EI("o2 c") EI("o2 f") EI("o2 g") EI("o2 a");
static const char TITLE_DRUM[] = "[" DR2 DR2 "]7 " DRB DRB;

/* "Pick Your Corner" - the select screen, D minor, a strut */
static const char SEL_LEAD[] =
    "@14 v10 q5"
    "| o5 d4 f8 a8 r8 g8 f4 | o5 e4 g8 b-8 r8 a8 g4 | o5 f4 a8 o6 d8 r8 c8 o5 a4 | o5 g8 f8 e8 d8 c+2"
    "| o5 d4 f8 a8 r8 g8 f4 | o5 b-4 a8 g8 r8 f8 e4 | o5 f8 g8 a8 b-8 a8 g8 f8 e8 | o5 d2 r2";
static const char SEL_COMP[] =
    "v5 q3 " OFFB("@16 o4", "d") OFFB("@16 o4", "c") OFFB("@16 o4", "d") OFFB("@17 o4", "a")
    OFFB("@16 o4", "d") OFFB("@16 o3", "b-") OFFB("@17 o4", "c") OFFB("@16 o4", "d");
static const char SEL_BASS[] =
    "@6 v13 q5 " Q4("o2 d", "a", "d", "a") Q4("o2 c", "g", "c", "g") Q4("o1 b-", "o2 f", "o1 b-", "o2 f")
    Q4("o1 a", "o2 e", "o1 a", "o2 c+") Q4("o2 d", "a", "d", "a") Q4("o1 g", "o2 d", "o1 g", "o2 d")
    Q4("o1 a", "o2 e", "o1 a", "o2 e") Q4("o2 d", "a", "d", "a");
static const char SEL_DRUM[] = "[" SHUF SHUF "]8";

/* "Market Street Midnight" - night 1, E minor */
static const char N1_LEAD[] =
    "@0 v11 q5"
    "| o5 e8 r8 e8 g8 b8 a8 g8 e8 | o5 d8 r8 d8 f+8 a8 g8 f+8 d8 | o5 c8 r8 e8 g8 o6 c8 o5 b8 a8 g8 | o5 f+4 b4 a4 f+4"
    "| o5 e8 r8 e8 g8 b8 o6 d8 e8 d8 | o6 c8 o5 b8 a8 g8 a8 b8 o6 c8 d8 | o5 b8 a8 g8 f+8 g8 a8 b8 o6 d+8 | o6 e2 o5 b4 r4";
static const char N1_ARP[] =
    "@2 v5 q5 " ARP("o4 e", "g", "b", "g") ARP("o4 d", "f+", "a", "f+") ARP("o4 c", "e", "g", "e")
    ARP("o3 b", "o4 d+", "f+", "d+") ARP("o4 e", "g", "b", "g") ARP("o4 a", "o5 c", "e", "c")
    ARP("o3 b", "o4 d+", "f+", "d+") ARP("o4 e", "g", "b", "g");
static const char N1_BASS[] =
    "@6 v14 q4 " OCT("e") OCT("d") OCT("c") OCT1("b") OCT("e") OCT("a") OCT1("b") OCT("e");
static const char N1_DRUM[] = "[" DR2 DR2 "]8";

/* "Last Ferry" - night 2, G minor, rolling */
static const char N2_LEAD[] =
    "@1 v10 q6"
    "| o5 g4. b-8 a4 g4 | o5 f4. a8 g4 f4 | o5 e-4. g8 b-4 o6 d4 | o6 c2 o5 a2"
    "| o5 g4. b-8 o6 d4 c4 | o5 b-4 a8 g8 f4 e-4 | o5 d8 e-8 f8 g8 a8 b-8 o6 c8 d8 | o5 g2 r2";
static const char N2_COMP[] =
    "v5 q4 @2 [o4 g8 b-8 o5 d8 o4 b-8]2 [o4 f8 a8 o5 c8 o4 a8]2 [o4 e-8 g8 b-8 g8]2 [o4 d8 f+8 a8 f+8]2"
    " [o4 g8 b-8 o5 d8 o4 b-8]2 [o4 e-8 g8 b-8 g8]2 [o4 d8 f+8 a8 f+8]2 [o4 g8 b-8 o5 d8 o4 b-8]2";
static const char N2_BASS[] =
    "@6 v13 q5 " Q4("o2 g", "d", "g", "d") Q4("o2 f", "c", "f", "c") Q4("o2 e-", "b-", "e-", "b-")
    Q4("o2 d", "a", "d", "f+") Q4("o2 g", "d", "g", "d") Q4("o2 e-", "b-", "e-", "b-")
    Q4("o2 d", "a", "d", "a") Q4("o2 g", "d", "g", "d");
static const char N2_DRUM[] = "[" SHUF SHUF "]8";

/* "The Orchard at Night" - night 3, C minor, uneasy */
static const char N3_LEAD[] =
    "@5 v10 q7"
    "| o5 c4 e-4 g4 f+4 | o5 g2 e-2 | o5 a-4 g4 f4 e-4 | o5 d2 b2"
    "| o5 c4 e-4 g4 o6 c4 | o5 b2 a-2 | o5 g4 f4 e-4 d4 | o5 c1";
static const char N3_COMP[] =
    "v6 q2 @17 o4 " ST16("c") "@16 o4 " ST16("c") "@16 o3 " ST16("a-") "@16 o3 " ST16("g")
    "@17 o4 " ST16("c") "@16 o3 " ST16("a-") "@16 o3 " ST16("g") "@17 o4 " ST16("c");
static const char N3_BASS[] =
    "@6 v14 q3 " EI("o2 c") EI("o2 c") EI("o1 a-") EI("o1 g") EI("o2 c") EI("o1 a-") EI("o1 g") EI("o2 c");
static const char N3_DRUM[] = "[@13 v12 o2 c4 @9 v4 o8 c8 @9 v4 o8 c8 @11 v9 o6 c4 @9 v4 o8 c4]8";

/* "Promenade" - night 4, F major, a seaside swagger */
static const char N4_LEAD[] =
    "@14 v11 q6"
    "| o5 c8 f8 a8 o6 c8 r8 o5 a8 f4 | o5 b-8 a8 g8 f8 g4 c4 | o5 d8 g8 b-8 o6 d8 r8 c8 o5 b-4 | o5 a8 g8 f8 e8 f4 r4"
    "| o5 c8 f8 a8 o6 c8 d8 c8 o5 a4 | o5 b-8 o6 c8 d8 e8 f4 c4 | o5 b-8 a8 g8 f8 e8 d8 c8 e8 | o5 f2 r2";
static const char N4_COMP[] =
    "v6 q3 " OFFB("@16 o4", "f") OFFB("@16 o4", "c") OFFB("@17 o4", "g") OFFB("@16 o4", "f")
    OFFB("@16 o4", "f") OFFB("@16 o3", "b-") OFFB("@17 o4", "c") OFFB("@16 o4", "f");
static const char N4_BASS[] =
    "@6 v13 q5 " Q4("o2 f", "a", "o3 c", "o2 a") Q4("o2 c", "e", "g", "e") Q4("o2 g", "b-", "o3 d", "o2 b-")
    Q4("o2 c", "e", "f", "c") Q4("o2 f", "a", "o3 c", "o2 a") Q4("o1 b-", "o2 d", "f", "d")
    Q4("o2 c", "e", "g", "e") Q4("o2 f", "c", "f", "c");
static const char N4_DRUM[] = "[" DR2 DR2 "]8";

/* "The Grand Hotel" - night 5, D minor, fast */
static const char N5_LEAD[] =
    "@0 v11 q5"
    "| o5 d8 d8 a8 d8 o6 c8 d8 o5 a8 f8 | o5 g8 g8 o6 d8 o5 g8 f8 g8 e8 c+8 | o5 d8 d8 a8 d8 o6 c8 d8 f8 e8 | o6 d8 c8 o5 a8 f8 e4 c+4"
    "| o5 b-8 b-8 o6 f8 o5 b-8 a8 b-8 g8 f8 | o5 g8 a8 b-8 o6 c8 d8 c8 o5 b-8 a8 | o5 f8 e8 d8 e8 f8 g8 a8 c+8 | o5 d2 r2";
static const char N5_STAB[] =
    "v6 q2 @17 o4 " ST16("d") "@16 o4 " ST16("c") "@17 o4 " ST16("d") "@16 o3 " ST16("a")
    "@16 o3 " ST16("b-") "@16 o4 " ST16("c") "@16 o3 " ST16("a") "@17 o4 " ST16("d");
static const char N5_BASS[] =
    "@6 v14 q4 " OCT("d") OCT("c") OCT("d") OCT1("a") OCT1("b-") OCT("c") OCT1("a") OCT("d");
static const char N5_DRUM[] = "[@13 v11 o2 c8 @9 v4 o8 c16 c16 @11 v9 o6 c8 @9 v4 o8 c16 c16]16";

/* "Heavyweight" - the bosses, E minor */
static const char BOSS_LEAD[] =
    "@0 v12 q5"
    "| o4 e8 e8 o5 e8 o4 e8 d8 o5 e8 g8 a8 | o5 b8 b8 a8 g8 a8 g8 f+8 d8 | o4 c8 c8 o5 c8 o4 c8 b8 o5 c8 b8 a8 | o5 g8 f+8 e8 f+8 d+8 o4 b8 o5 d+8 f+8"
    "| o4 e8 e8 o5 e8 o4 e8 d8 o5 e8 g8 a8 | o5 b8 o6 c8 d8 e8 d8 c8 o5 b8 a8 | o4 c8 c8 o5 c8 o4 c8 b8 o5 c8 b8 a8 | o5 g8 f+8 e8 d+8 e2";
static const char BOSS_STAB[] =
    "v7 q2 @17 o4 " ST16("e") "@16 o4 " ST16("g") "@16 o4 " ST16("c") "@16 o3 " ST16("b")
    "@17 o4 " ST16("e") "@16 o4 " ST16("g") "@16 o4 " ST16("c") "@16 o3 " ST16("b");
static const char BOSS_BASS[] =
    "@6 v15 q4 " EI("o2 e") EI("o2 g") EI("o2 c") EI("o1 b") EI("o2 e") EI("o2 g") EI("o2 c") EI("o1 b");
static const char BOSS_DRUM[] = "[@13 v13 o2 c8 @11 v10 o6 c16 @13 v10 o2 c16 @13 v12 o2 c8 @11 v11 o6 c8]16";

/* "Alderman Grist" - the last fight, C minor */
static const char GRIST_LEAD[] =
    "@1 v12 q6"
    "| o5 c8 e-8 g8 c8 o6 c8 o5 b8 a-8 g8 | o5 a-8 f8 c8 f8 a-8 g8 f8 e-8 | o5 g8 b8 o6 d8 o5 g8 o6 f8 e-8 d8 o5 b8 | o6 c4 o5 g4 e-4 c4"
    "| o5 c8 e-8 g8 c8 o6 e-8 d8 c8 o5 b8 | o5 a-8 g8 f8 e-8 f8 g8 a-8 b8 | o6 c8 d8 e-8 f8 g8 f8 e-8 d8 | o6 c2 r2";
static const char GRIST_ARP[] =
    "@2 v5 q5 " ARP("o4 c", "e-", "g", "e-") ARP("o4 f", "a-", "o5 c", "o4 a-") ARP("o4 g", "b", "o5 d", "o4 b")
    ARP("o4 c", "e-", "g", "e-") ARP("o4 c", "e-", "g", "e-") ARP("o4 f", "a-", "o5 c", "o4 a-")
    ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 c", "e-", "g", "e-");
static const char GRIST_BASS[] =
    "@6 v15 q4 " OCT("c") OCT1("f") OCT1("g") OCT("c") OCT("c") OCT1("f") OCT1("g") OCT("c");
static const char GRIST_DRUM[] = "[@13 v12 o2 c16 @9 v5 o8 c16 @11 v10 o6 c16 @9 v5 o8 c16]32";

/* "The Corner Shop" - between nights, B-flat major, easy */
static const char SHOP_LEAD[] =
    "@14 v10 q7"
    "| o5 f4 d8 b-8 r4 c4 | o5 d4 e-8 f8 g2 | o5 f4 d8 b-8 r4 a4 | o5 g8 f8 e-8 d8 c2"
    "| o5 b-4 o6 d8 c8 o5 b-4 f4 | o5 g4 a8 b-8 o6 c2 | o5 b-8 a8 g8 f8 e-8 d8 c8 e-8 | o5 d2 b-2";
static const char SHOP_COMP[] =
    "v5 q3 " OFFB("@16 o4", "d") OFFB("@16 o4", "e-") OFFB("@16 o4", "d") OFFB("@17 o4", "c")
    OFFB("@16 o4", "d") OFFB("@16 o4", "e-") OFFB("@17 o4", "c") OFFB("@16 o4", "d");
static const char SHOP_BASS[] =
    "@6 v12 q6 " Q4("o2 b-", "f", "b-", "f") Q4("o2 e-", "b-", "e-", "b-") Q4("o2 b-", "f", "b-", "f")
    Q4("o2 f", "c", "f", "a") Q4("o2 b-", "f", "b-", "f") Q4("o2 e-", "b-", "e-", "b-")
    Q4("o2 f", "c", "f", "c") Q4("o2 b-", "f", "b-", "f");
static const char SHOP_DRUM[] = "[@9 v4 o8 c8 @9 v3 o8 c8 @11 v6 o6 c8 @9 v3 o8 c8]16";

/* "Home and Guest" - the gym, A major, a pep band */
static const char GYM_LEAD[] =
    "@14 v11 q5"
    "| o5 a8 a8 a8 o6 c+8 e4 c+4 | o5 b8 b8 b8 o6 d8 e4 d4 | o6 c+8 c+8 c+8 e8 a4 e4 | o6 d8 c+8 o5 b8 a8 b4 e4"
    "| o5 a8 a8 a8 o6 c+8 e4 a4 | o6 f+8 e8 d8 c+8 d4 o5 b4 | o5 a8 b8 o6 c+8 d8 e8 f+8 g+8 a8 | o6 a4 e4 a2";
static const char GYM_COMP[] =
    "v6 q3 " OFFB("@16 o4", "c+") OFFB("@16 o4", "d") OFFB("@16 o4", "c+") OFFB("@17 o4", "e")
    OFFB("@16 o4", "c+") OFFB("@16 o4", "d") OFFB("@17 o4", "e") OFFB("@16 o4", "c+");
static const char GYM_BASS[] =
    "@6 v14 q4 " Q4("o2 a", "e", "a", "e") Q4("o2 b", "f+", "b", "e") Q4("o2 a", "e", "a", "e")
    Q4("o2 d", "a", "e", "b") Q4("o2 a", "e", "a", "e") Q4("o2 d", "a", "b", "f+")
    Q4("o2 e", "b", "e", "g+") Q4("o2 a", "e", "a", "a");
static const char GYM_DRUM[] = "[" DRB DR2 "]8";

/* "Going Up" - the service lift, F minor, tense */
static const char LIFT_LEAD[] =
    "@5 v10 q6"
    "| o5 f4 a-4 o6 c4 o5 b4 | o6 c2 o5 a-2 | o5 g4 b-4 o6 d-4 c4 | o5 b-2 g2"
    "| o5 f4 a-4 o6 c4 e4 | o6 f2 d-2 | o6 c4 o5 b-4 a-4 g4 | o5 f2 e2";
static const char LIFT_COMP[] =
    "v6 q2 @17 o4 " ST16("f") "@16 o4 " ST16("f") "@16 o4 " ST16("e-") "@16 o4 " ST16("e-")
    "@17 o4 " ST16("f") "@16 o4 " ST16("d-") "@16 o4 " ST16("c") "@16 o4 " ST16("c");
static const char LIFT_BASS[] =
    "@6 v13 q3 " EI("o2 f") EI("o2 f") EI("o2 e-") EI("o2 e-") EI("o2 f") EI("o2 d-") EI("o2 c") EI("o2 c");
static const char LIFT_DRUM[] = "[@13 v11 o2 c4 @11 v8 o6 c4 @13 v11 o2 c8 @13 v8 o2 c8 @11 v8 o6 c4]8";

/* "Gran for Alderman" - the good ending, C major */
static const char GOOD_LEAD[] =
    "@14 v11 q7"
    "| o5 e8 f8 g8 o6 c8 e4 d4 | o6 c8 o5 b8 a8 g8 a2 | o5 f8 g8 a8 o6 d8 f4 e4 | o6 d8 c8 o5 b8 a8 g2"
    "| o5 e8 f8 g8 o6 c8 e4 g4 | o6 f8 e8 d8 c8 d4 a4 | o6 g8 f8 e8 d8 c8 o5 b8 a8 b8 | o6 c2 r2";
static const char GOOD_COMP[] =
    "v6 q3 " OFFB("@16 o4", "e") OFFB("@17 o4", "a") OFFB("@16 o4", "f") OFFB("@16 o4", "g")
    OFFB("@16 o4", "e") OFFB("@17 o4", "f") OFFB("@16 o4", "g") OFFB("@16 o4", "e");
static const char GOOD_BASS[] =
    "@6 v12 q6 " Q4("o2 c", "g", "e", "g") Q4("o2 a", "e", "c", "e") Q4("o2 d", "a", "f", "a")
    Q4("o2 g", "d", "b", "d") Q4("o2 c", "g", "e", "g") Q4("o2 f", "c", "a", "c")
    Q4("o2 g", "d", "b", "d") Q4("o2 c", "g", "e", "g");
static const char GOOD_DRUM[] = "[@13 v10 o2 c4 @11 v8 o6 c4]16";

/* "Too Late" - the bad ending, A minor, slow */
static const char BAD_LEAD[] =
    "@5 v10 q7"
    "| o5 a2 g4 e4 | o5 f2 e4 d4 | o5 e2 c4 o4 a4 | o4 b1"
    "| o5 a2 o6 c4 o5 b4 | o5 a2 g4 f4 | o5 e4 d4 c4 o4 b4 | o4 a1";
static const char BAD_ARP[] =
    "@2 v4 q5 [o4 a8 o5 c8 e8 c8]2 [o4 f8 a8 o5 c8 o4 a8]2 [o4 c8 e8 a8 e8]2 [o4 e8 g+8 b8 g+8]2"
    " [o4 a8 o5 c8 e8 c8]2 [o4 f8 a8 o5 c8 o4 a8]2 [o4 d8 f8 a8 f8]2 [o4 a8 o5 c8 e8 c8]2";
static const char BAD_BASS[] = "@6 v11 q7 o2 a1 f1 c1 e1 a1 f1 d1 a1";
static const char BAD_SOFT[] = "[@9 v2 o8 c4]32";

/* jingles */
static const char CLEAR_P1[] = "@39 v12 o5 l16 e g o6 c8 o5 g16 o6 c16 e8 d16 e16 g4 c4";
static const char CLEAR_P2[] = "@20 v8 o5 l16 c e g8 e16 g16 o6 c8 o5 b16 o6 c16 e4 o5 g4";
static const char CLEAR_TRI[] = "@6 v12 o3 c8 g8 o4 c8 o3 g8 c4 o2 c4";
static const char CLEAR_NOISE[] = "@11 v9 o6 c8 c8 c8 c8 @12 v8 o5 c2";
static const char DOWN_P1[] = "@0 v11 o5 e8 d+8 d8 c+8 c4 o4 b8 a+8 a2";
static const char DOWN_TRI[] = "@6 v12 o2 e4 d4 c4 o1 b4 a2";

void dku_audio_load(void) {
    if (DKU_MUS_TITLE >= 0) return;
    DKU_MUS_TITLE = song_define("dku_title", 140, true, TITLE_LEAD, TITLE_COMP, TITLE_BASS, TITLE_DRUM);
    DKU_MUS_SELECT = song_define("dku_select", 116, true, SEL_LEAD, SEL_COMP, SEL_BASS, SEL_DRUM);
    DKU_MUS_N1 = song_define("dku_n1", 150, true, N1_LEAD, N1_ARP, N1_BASS, N1_DRUM);
    DKU_MUS_N2 = song_define("dku_n2", 126, true, N2_LEAD, N2_COMP, N2_BASS, N2_DRUM);
    DKU_MUS_N3 = song_define("dku_n3", 104, true, N3_LEAD, N3_COMP, N3_BASS, N3_DRUM);
    DKU_MUS_N4 = song_define("dku_n4", 132, true, N4_LEAD, N4_COMP, N4_BASS, N4_DRUM);
    DKU_MUS_N5 = song_define("dku_n5", 160, true, N5_LEAD, N5_STAB, N5_BASS, N5_DRUM);
    DKU_MUS_BOSS = song_define("dku_boss", 150, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    DKU_MUS_GRIST = song_define("dku_grist", 166, true, GRIST_LEAD, GRIST_ARP, GRIST_BASS, GRIST_DRUM);
    DKU_MUS_SHOP = song_define("dku_shop", 108, true, SHOP_LEAD, SHOP_COMP, SHOP_BASS, SHOP_DRUM);
    DKU_MUS_GYM = song_define("dku_gym", 144, true, GYM_LEAD, GYM_COMP, GYM_BASS, GYM_DRUM);
    DKU_MUS_LIFT = song_define("dku_lift", 120, true, LIFT_LEAD, LIFT_COMP, LIFT_BASS, LIFT_DRUM);
    DKU_MUS_GOOD = song_define("dku_good", 116, true, GOOD_LEAD, GOOD_COMP, GOOD_BASS, GOOD_DRUM);
    DKU_MUS_BAD = song_define("dku_bad", 76, true, BAD_LEAD, BAD_ARP, BAD_BASS, BAD_SOFT);
    DKU_MUS_CLEAR = song_define("dku_clear", 150, false, CLEAR_P1, CLEAR_P2, CLEAR_TRI, CLEAR_NOISE);
    DKU_MUS_DOWN = song_define("dku_down", 96, false, DOWN_P1, "", DOWN_TRI, "");

    sfx_define("dku_jab", CH_NOISE, 240, "@40 v9 o6 c32");
    sfx_define("dku_kick", CH_NOISE, 220, "@40 v12 o5 c16");
    sfx_define("dku_thud", CH_NOISE, 200, "@13 v11 o2 c16");
    sfx_define("dku_hurt", CH_P1, 200, "@37 v12 o4 e16 c16");
    sfx_define("dku_ko", CH_P1, 180, "@33 v12 o4 g8 e8 c8");
    sfx_define("dku_block", CH_P2, 240, "@42 v10 o6 g32 r32 g32");
    sfx_define("dku_guardbreak", CH_P2, 200, "@15 v12 o5 c16 o4 g16 c16");
    sfx_define("dku_spin", CH_P2, 240, "@32 v10 o4 c32 e32 g32 o5 c32 e32 g32");
    sfx_define("dku_charge", CH_P2, 220, "@35 v9 o5 c16 g16");
    sfx_define("dku_charged", CH_NOISE, 200, "@40 v14 o4 c8");
    sfx_define("dku_dash", CH_P1, 220, "@32 v11 o4 c16 g16");
    sfx_define("dku_jump", CH_P2, 240, "@20 v8 o5 c32 g32");
    sfx_define("dku_dodge", CH_P2, 240, "@20 v7 o4 g32 c32");
    sfx_define("dku_grab", CH_P2, 220, "@41 v10 o4 c16");
    sfx_define("dku_throw", CH_P1, 220, "@32 v10 o5 c16 o4 c16");
    sfx_define("dku_slam", CH_NOISE, 160, "@34 v14 o3 c8");
    sfx_define("dku_pickup", CH_P2, 240, "@20 v9 o5 e32 g32");
    sfx_define("dku_eat", CH_P1, 220, "@39 v11 o5 l32 c e g o6 c");
    sfx_define("dku_cash", CH_P1, 240, "@35 v11 o6 c32 g32 o7 c16");
    sfx_define("dku_buy", CH_P1, 220, "@35 v12 o6 c16 e16 g16");
    sfx_define("dku_swing", CH_NOISE, 240, "@21 v8 o6 c32 c32");
    sfx_define("dku_saw", CH_NOISE, 240, "@36 v10 o5 c16 c16 c16 c16");
    sfx_define("dku_shotgun", CH_NOISE, 160, "@34 v14 o4 c16 @21 v8 o6 c16");
    sfx_define("dku_reflect", CH_P2, 240, "@15 v12 o7 c32 g32");
    sfx_define("dku_boom", CH_NOISE, 140, "@34 v14 o3 c4");
    sfx_define("dku_toss", CH_P2, 220, "@41 v8 o5 c16");
    sfx_define("dku_ray", CH_P2, 200, "@38 v10 o6 c16 o5 c16 o4 c16");
    sfx_define("dku_spit", CH_NOISE, 220, "@36 v9 o5 c16");
    sfx_define("dku_hiss", CH_NOISE, 200, "@36 v7 o7 c8");
    sfx_define("dku_rush", CH_P1, 180, "@33 v12 o3 c8 o2 g8");
    sfx_define("dku_growl", CH_P1, 200, "@37 v11 o3 c16 c16");
    sfx_define("dku_leap", CH_P2, 220, "@20 v9 o4 c16 g16");
    sfx_define("dku_swipe", CH_NOISE, 240, "@21 v7 o5 c32");
    sfx_define("dku_blink", CH_P2, 220, "@38 v9 o6 c16 o7 c16");
    sfx_define("dku_roar", CH_P1, 140, "@33 v13 o3 c4 o2 g4");
    sfx_define("dku_bark", CH_P2, 220, "@37 v10 o5 c16 r32 c16");
    sfx_define("dku_yelp", CH_P2, 240, "@20 v9 o6 e16 c16");
    sfx_define("dku_splash", CH_NOISE, 200, "@36 v9 o6 c8");
    sfx_define("dku_horn", CH_P1, 160, "@15 v12 o4 c4 r16 c4");
    sfx_define("dku_saucer", CH_P2, 200, "@38 v8 o5 c16 e16 g16 o6 c16");
    sfx_define("dku_creak", CH_P2, 160, "@41 v9 o3 c8 c+8");
    sfx_define("dku_crash", CH_NOISE, 140, "@34 v15 o3 c4");
    sfx_define("dku_fall", CH_P1, 200, "@32 v11 o5 c16 o4 g16 e16 c8");
    sfx_define("dku_door", CH_NOISE, 200, "@13 v12 o3 c8 @40 v9 o5 c8");
    sfx_define("dku_go", CH_P1, 200, "@15 v12 o6 c16 r16 c16");
    sfx_define("dku_ding", CH_P1, 160, "@15 v12 o6 e8 c4");
    sfx_define("dku_beam", CH_P2, 180, "@38 v10 o4 c16 g16 o5 c16 g16 o6 c4");
    sfx_define("dku_whistle", CH_P1, 200, "@15 v12 o7 c8 r16 c4");
    sfx_define("dku_bell", CH_P1, 160, "@15 v12 o6 c8 o5 g8 o6 c4");
}
