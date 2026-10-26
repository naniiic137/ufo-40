/* HELLSPUR - original music (UFO-MML) and sound effects. Every looping tune
 * is built from whole bars (each macro below is exactly one bar), so the
 * four channels loop together. Prairie shuffles, saloon rags, a hymn, and
 * something worse for the pit. */
#include "spur.h"

int SPUR_MUS_TITLE = -1, SPUR_MUS_SALOON, SPUR_MUS_TOWN, SPUR_MUS_WORLD, SPUR_MUS_BATTLE, SPUR_MUS_BOSS, SPUR_MUS_CAVE,
    SPUR_MUS_DREAM, SPUR_MUS_HELL, SPUR_MUS_CITADEL, SPUR_MUS_CHURCH, SPUR_MUS_RIDE, SPUR_MUS_FLY, SPUR_MUS_WIN,
    SPUR_MUS_LOSE, SPUR_MUS_ENDING, SPUR_MUS_FINAL, SPUR_MUS_GHOST, SPUR_MUS_SHOP, SPUR_MUS_LEVEL;

/* one bar each */
#define Q4(a, b, c, d) a "4 " b "4 " c "4 " d "4 "
#define E8(a, b, c, d, e, f, g, h) a "8 " b "8 " c "8 " d "8 " e "8 " f "8 " g "8 " h "8 "
#define EE(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define H2(a, b) a "2 " b "2 "
#define W1(a) a "1 "
#define DQ(a, b, c) a "4. " b "8 " c "2 "
/* a shuffle: long-short pairs, four to the bar */
#define SH(a, b, c, d, e, f, g, h) a "6 " b "12 " c "6 " d "12 " e "6 " f "12 " g "6 " h "12 "
/* a walking bass in shuffle: root, fifth, octave, fifth */
#define WALK(r, f) "[" r "6 " f "12 ]2 " r "6 " r "12 " f "6 " f "12 "
#define OOMPAH(lo, hi) lo "4 " hi "4 " lo "4 " hi "4 "
/* drums, one bar */
#define TRAIN "@13 v11 o2 c6 @9 v4 o8 c12 @11 v8 o6 c6 @9 v4 o8 c12 @13 v11 o2 c6 @9 v4 o8 c12 @11 v8 o6 c6 @9 v5 o8 c12 "
#define GALLOP "@13 v11 o2 c8 @9 v5 o8 c16 c16 @11 v9 o6 c8 @9 v5 o8 c16 c16 @13 v11 o2 c8 @9 v5 o8 c16 c16 @11 v9 o6 c8 @9 v5 o8 c16 c16 "
#define STOMP "@13 v12 o2 c4 @11 v9 o6 c4 @13 v12 o2 c8 c8 @11 v9 o6 c4 "
#define BRUSH "@9 v3 o8 c4 @9 v5 o8 c4 @9 v3 o8 c4 @9 v5 o8 c4 "
#define DRIVE "@13 v12 o2 c8 @9 v5 o8 c8 @11 v9 o6 c8 @9 v5 o8 c8 @13 v12 o2 c8 @13 v9 o2 c8 @11 v9 o6 c8 @11 v6 o6 c8 "
#define TOLL "@12 v6 o3 c1 "
#define HEART "@13 v10 o2 c8 r8 @13 v7 o2 c8 r8 r2 "

/* "Hellspur" - the title, D minor, a lone harmonica over a slow train */
static const char TITLE_LEAD[] =
    "@5 v12 q7 " DQ("o5 d", "f", "a") Q4("o5 g", "f", "e", "c") DQ("o5 d", "e", "f") W1("o5 a")
    DQ("o6 d", "c", "o5 a") Q4("o5 b-", "a", "g", "e") DQ("o5 f", "e", "c+") W1("o5 d");
static const char TITLE_HARM[] =
    "@4 v6 q8 " H2("o4 a", "f") H2("o4 g", "e") H2("o4 f", "a") H2("o4 a", "c+")
    H2("o4 a", "f") H2("o4 g", "e") H2("o4 a", "a") W1("o4 a");
static const char TITLE_BASS[] =
    "@6 v13 q6 " WALK("o2 d", "a") WALK("o2 c", "g") WALK("o2 b-", "f") WALK("o2 a", "e")
    WALK("o2 d", "a") WALK("o2 g", "d") WALK("o2 a", "e") WALK("o2 d", "a");
static const char TITLE_DRUM[] = "[" TRAIN "]8";

/* "The Golden Garter" - the saloon, a rag on an old upright */
static const char SAL_LEAD[] =
    "@3 v11 q5 " E8("o5 e", "g", "o6 c", "e", "d", "c", "o5 a", "g") E8("o5 a", "g", "e", "c", "d", "e", "g", "r")
    E8("o5 f", "a", "o6 c", "f", "e", "d", "c", "o5 a") E8("o5 g", "b", "o6 d", "g", "f", "d", "o5 b", "g")
    E8("o5 e", "g", "o6 c", "e", "d", "c", "o5 a", "g") E8("o5 a", "o6 c", "e", "a", "g", "e", "c", "o5 a")
    E8("o5 g", "a", "b", "o6 c", "d", "e", "f", "d") Q4("o6 c", "e", "c", "r");
static const char SAL_COMP[] =
    "@16 v6 q4 " OOMPAH("o3 c", "o4 c") OOMPAH("o3 a", "o4 a") OOMPAH("o3 f", "o4 f") OOMPAH("o3 g", "o4 g")
    OOMPAH("o3 c", "o4 c") OOMPAH("o3 a", "o4 a") OOMPAH("o3 g", "o4 g") OOMPAH("o3 c", "o4 c");
static const char SAL_BASS[] =
    "@7 v12 q5 " Q4("o2 c", "g", "c", "g") Q4("o2 a", "e", "a", "e") Q4("o2 f", "o3 c", "o2 f", "o3 c")
    Q4("o2 g", "d", "g", "d") Q4("o2 c", "g", "c", "g") Q4("o2 a", "e", "a", "e") Q4("o2 g", "d", "g", "b")
    Q4("o3 c", "o2 g", "e", "c");
static const char SAL_DRUM[] = "[" BRUSH "]8";

/* "Main Street" - the towns, G major, an easy shuffle */
static const char TOWN_LEAD[] =
    "@14 v11 q6 " SH("o5 d", "b", "o6 d", "o5 b", "g", "a", "b", "r") SH("o5 c", "e", "g", "e", "a", "g", "e", "r")
    SH("o5 d", "b", "o6 d", "e", "d", "b", "g", "e") DQ("o5 d", "e", "f+")
    SH("o5 g", "a", "b", "g", "o6 c", "o5 b", "a", "g") SH("o5 a", "b", "o6 c", "o5 a", "o6 d", "c", "o5 b", "a")
    SH("o5 g", "f+", "e", "d", "e", "f+", "g", "a") W1("o5 g");
static const char TOWN_COMP[] =
    "@22 v6 q5 " SH("o4 g", "b", "g", "b", "g", "b", "g", "b") SH("o4 e", "g", "e", "g", "e", "g", "e", "g")
    SH("o4 g", "b", "g", "b", "g", "b", "g", "b") SH("o4 f+", "a", "f+", "a", "f+", "a", "f+", "a")
    SH("o4 g", "b", "g", "b", "g", "b", "g", "b") SH("o4 f+", "a", "f+", "a", "f+", "a", "f+", "a")
    SH("o4 e", "g", "e", "g", "d", "f+", "d", "f+") SH("o4 d", "g", "d", "g", "d", "g", "d", "g");
static const char TOWN_BASS[] =
    "@6 v13 q5 " WALK("o2 g", "d") WALK("o2 c", "g") WALK("o2 g", "d") WALK("o2 d", "a")
    WALK("o2 g", "d") WALK("o2 d", "a") WALK("o2 c", "g") WALK("o2 g", "d");
static const char TOWN_DRUM[] = "[" TRAIN "]8";

/* "Open Country" - the Territory, E minor, wide and lonely */
static const char WORLD_LEAD[] =
    "@1 v11 q7 " DQ("o5 e", "g", "b") Q4("o5 a", "g", "f+", "d") DQ("o5 e", "f+", "g") W1("o5 b")
    DQ("o6 e", "d", "o5 b") Q4("o5 a", "b", "o6 c", "o5 a") DQ("o5 g", "f+", "d") W1("o5 e")
    DQ("o5 g", "a", "b") Q4("o6 d", "c", "o5 b", "a") DQ("o5 g", "a", "b") W1("o6 e")
    DQ("o6 d", "c", "o5 b") Q4("o5 a", "g", "f+", "g") DQ("o5 f+", "e", "d+") W1("o5 e");
static const char WORLD_HARM[] =
    "@4 v5 q8 " W1("o4 b") W1("o4 a") W1("o4 g") W1("o4 b") W1("o4 g") W1("o4 a") W1("o4 b") W1("o4 g")
    W1("o4 b") W1("o4 a") W1("o4 g") W1("o4 b") W1("o4 g") W1("o4 e") W1("o4 b") W1("o4 g");
static const char WORLD_BASS[] =
    "@6 v12 q6 " Q4("o2 e", "b", "e", "b") Q4("o2 d", "a", "d", "a") Q4("o2 c", "g", "c", "g") Q4("o2 b", "f+", "b", "f+")
    Q4("o2 e", "b", "e", "b") Q4("o2 a", "e", "a", "e") Q4("o2 b", "f+", "b", "f+") Q4("o2 e", "b", "e", "b")
    Q4("o2 g", "d", "g", "d") Q4("o2 d", "a", "d", "a") Q4("o2 c", "g", "c", "g") Q4("o2 e", "b", "e", "b")
    Q4("o2 a", "e", "a", "e") Q4("o2 d", "a", "d", "a") Q4("o2 b", "f+", "b", "f+") Q4("o2 e", "b", "e", "b");
static const char WORLD_DRUM[] = "[" BRUSH BRUSH TRAIN BRUSH "]4";

/* "Wisps!" - every fight, A minor, all gallop */
static const char BAT_LEAD[] =
    "@1 v12 q6 " E8("o5 a", "a", "o6 c", "o5 a", "o6 e", "o5 a", "o6 d", "c") E8("o5 b", "g", "e", "g", "b", "o6 d", "c", "o5 b")
    E8("o5 a", "a", "o6 c", "o5 a", "o6 e", "f", "e", "d") Q4("o6 c", "o5 b", "a", "g+")
    E8("o5 f", "f", "a", "f", "o6 c", "o5 a", "o6 d", "c") E8("o5 e", "e", "g", "e", "b", "g", "o6 c", "o5 b")
    E8("o5 d", "f", "a", "o6 d", "c", "o5 b", "a", "g+") W1("o5 a");
static const char BAT_STAB[] = "@17 v6 q3 " EE("o4 a") EE("o4 g") EE("o4 a") EE("o4 e") EE("o4 f") EE("o4 e") EE("o4 d") EE("o4 e");
static const char BAT_BASS[] =
    "@6 v14 q4 " "[o2 a8 o3 a8]4 " "[o2 g8 o3 g8]4 " "[o2 a8 o3 a8]4 " "[o2 e8 o3 e8]4 "
    "[o2 f8 o3 f8]4 " "[o2 c8 o3 c8]4 " "[o2 d8 o3 d8]4 " "[o2 e8 o3 e8]4 ";
static const char BAT_DRUM[] = "[" GALLOP "]8";

/* "Something Big" - the bosses, chromatic and heavy */
static const char BOSS_LEAD[] =
    "@23 v12 q7 " Q4("o5 d", "e-", "d", "c+") H2("o5 d", "a") Q4("o5 g", "f", "e-", "d") H2("o5 c+", "o4 a")
    Q4("o5 d", "f", "a", "o6 d") Q4("o6 c+", "o5 b-", "a", "g") Q4("o5 f", "e", "d", "c+") W1("o5 d");
static const char BOSS_STAB[] = "@16 v6 q3 " EE("o4 d") EE("o4 d") EE("o4 e-") EE("o4 c+") EE("o4 d") EE("o4 g") EE("o4 b-") EE("o4 a");
static const char BOSS_BASS[] =
    "@6 v14 q4 " "[o2 d8 o2 d8 o3 d8 o2 d8]2 " "[o2 d8 o2 d8 o3 d8 o2 d8]2 " "[o2 e-8 o2 e-8 o3 e-8 o2 e-8]2 "
    "[o2 c+8 o2 c+8 o3 c+8 o2 c+8]2 " "[o2 d8 o2 d8 o3 d8 o2 d8]2 " "[o2 g8 o2 g8 o3 g8 o2 g8]2 "
    "[o2 b-8 o2 b-8 o3 b-8 o2 b-8]2 " "[o2 a8 o2 a8 o3 a8 o2 a8]2 ";
static const char BOSS_DRUM[] = "[" DRIVE "]8";

/* "Down the Shaft" - caves and mines, sparse and dripping */
static const char CAVE_LEAD[] =
    "@15 v9 q4 " Q4("o5 e", "r", "b", "r") Q4("o5 a", "r", "g", "r") Q4("o5 e", "r", "f", "r") H2("o5 e", "r")
    Q4("o5 c", "r", "d", "r") Q4("o5 e", "r", "g", "r") Q4("o5 f", "r", "d+", "r") H2("o5 e", "r");
static const char CAVE_PAD[] = "@4 v4 q8 " W1("o3 e") W1("o3 e") W1("o3 f") W1("o3 e") W1("o3 a") W1("o3 g") W1("o3 f") W1("o3 e");
static const char CAVE_BASS[] = "@6 v11 q3 " EE("o2 e") EE("o2 e") EE("o2 f") EE("o2 e") EE("o2 a") EE("o2 g") EE("o2 f") EE("o2 e");
static const char CAVE_DRUM[] = "[@21 v3 o7 c4 r4 @21 v2 o7 c8 r8 r4]8";

/* "The Dreaming" - the spirit country, floating whole tones */
static const char DREAM_LEAD[] =
    "@14 v10 q8 " DQ("o5 c", "d", "e") H2("o5 f+", "g+") DQ("o5 a+", "g+", "f+") W1("o5 e")
    DQ("o5 d", "e", "f+") H2("o5 g+", "a+") DQ("o6 c", "o5 a+", "g+") W1("o5 f+");
static const char DREAM_ARP[] = "@18 v4 q3 " EE("o4 c") EE("o4 d") EE("o4 e") EE("o4 c") EE("o4 d") EE("o4 e") EE("o4 f+") EE("o4 d");
static const char DREAM_BASS[] = "@19 v10 q8 " W1("o3 c") W1("o3 d") W1("o3 e") W1("o3 c") W1("o3 d") W1("o3 e") W1("o3 f+") W1("o3 d");
static const char DREAM_DRUM[] = "[@21 v2 o8 c2 @21 v3 o7 c2]8";

/* "The Pit" - Hell and the Bone Tower */
static const char HELL_LEAD[] =
    "@5 v10 q6 " Q4("o4 b", "o5 c", "o4 b", "r") Q4("o4 b", "o5 d", "c", "r") Q4("o4 b", "o5 c", "e-", "d") H2("o5 c", "o4 b")
    Q4("o5 f", "e", "d", "r") Q4("o5 e-", "d", "c", "r") Q4("o4 b", "a", "g+", "a") W1("o4 b");
static const char HELL_PAD[] = "@4 v5 q8 " W1("o3 b") W1("o3 b") W1("o4 c") W1("o3 b") W1("o4 d") W1("o4 c") W1("o3 g+") W1("o3 b");
static const char HELL_BASS[] = "@6 v13 q4 " "[o2 b8 r8 o2 b8 r8 o2 c8 r8 o2 b8 r8]8";
static const char HELL_DRUM[] = "[" HEART "]8";

/* "The Governor's House" - Hellspur itself, a crooked march */
static const char CIT_LEAD[] =
    "@23 v11 q6 " Q4("o5 e", "e", "f", "e") H2("o5 d+", "e") Q4("o5 g", "g", "a-", "g") H2("o5 f+", "g")
    Q4("o5 b", "a", "g", "f+") Q4("o5 e", "d+", "e", "f+") Q4("o5 g", "f+", "e", "d+") W1("o5 e");
static const char CIT_STAB[] = "@17 v6 q3 " Q4("o4 e", "r", "e", "r") Q4("o4 e", "r", "e", "r") Q4("o4 c", "r", "c", "r") Q4("o4 b", "r", "b", "r")
    Q4("o4 e", "r", "e", "r") Q4("o4 a", "r", "a", "r") Q4("o4 b", "r", "b", "r") Q4("o4 e", "r", "e", "r");
static const char CIT_BASS[] = "@6 v13 q5 " Q4("o2 e", "b", "e", "b") Q4("o2 e", "b", "e", "b") Q4("o2 c", "g", "c", "g") Q4("o2 b", "f+", "b", "f+")
    Q4("o2 e", "b", "e", "b") Q4("o2 a", "e", "a", "e") Q4("o2 b", "f+", "b", "f+") Q4("o2 e", "b", "e", "b");
static const char CIT_DRUM[] = "[" STOMP "]8";

/* "Old Hundred and One" - the chapels and missions, a hymn */
static const char CH_LEAD[] =
    "@14 v10 q8 " H2("o5 g", "g") H2("o5 f+", "e") H2("o5 d", "g") W1("o5 a")
    H2("o5 b", "b") H2("o5 a", "g") H2("o5 a", "f+") W1("o5 g");
static const char CH_ALTO[] = "@22 v6 q8 " H2("o5 d", "d") H2("o5 d", "c") H2("o4 b", "o5 d") W1("o5 d")
    H2("o5 g", "g") H2("o5 f+", "e") H2("o5 e", "d") W1("o5 d");
static const char CH_BASS[] = "@19 v9 q8 " H2("o3 g", "g") H2("o3 d", "a") H2("o3 b", "g") W1("o3 d") H2("o3 g", "e") H2("o3 d", "e")
    H2("o3 c", "d") W1("o3 g");
static const char CH_DRUM[] = "[" TOLL "]8";

/* "Hired Horses" - riding, a fast gallop in D */
static const char RIDE_LEAD[] =
    "@1 v11 q5 " E8("o5 d", "f+", "a", "f+", "d", "f+", "a", "o6 d") E8("o6 c+", "o5 a", "e", "a", "c+", "e", "a", "o6 c+")
    E8("o5 b", "o6 d", "f+", "d", "o5 b", "o6 d", "f+", "b") E8("o6 a", "f+", "e", "c+", "o5 a", "f+", "e", "c+")
    E8("o5 d", "f+", "a", "f+", "g", "b", "o6 d", "o5 b") E8("o5 a", "o6 c+", "e", "c+", "d", "f+", "a", "f+")
    E8("o6 g", "f+", "e", "d", "c+", "o5 b", "a", "g") W1("o5 f+");
static const char RIDE_COMP[] = "@16 v5 q3 " EE("o4 d") EE("o4 a") EE("o4 b") EE("o4 a") EE("o4 d") EE("o4 a") EE("o4 g") EE("o4 d");
static const char RIDE_BASS[] = "@6 v13 q4 " "[o2 d8 o3 d8]4 [o2 a8 o3 a8]4 [o2 b8 o3 b8]4 [o2 a8 o3 a8]4 [o2 d8 o3 d8]4 [o2 a8 o3 a8]4 [o2 g8 o3 g8]4 [o2 d8 o3 d8]4";
static const char RIDE_DRUM[] = "[" GALLOP "]8";

/* "Nightmares" - on the black wings */
static const char FLY_LEAD[] =
    "@23 v11 q7 " DQ("o5 a", "o6 c", "e") H2("o6 d", "c") DQ("o5 b", "o6 c", "d") W1("o6 e")
    DQ("o6 f", "e", "d") H2("o6 c", "o5 b") DQ("o5 a", "b", "g+") W1("o5 a");
static const char FLY_ARP[] = "@18 v5 q3 " EE("o4 a") EE("o4 f") EE("o4 g") EE("o4 e") EE("o4 d") EE("o4 a") EE("o4 e") EE("o4 a");
static const char FLY_BASS[] = "@19 v11 q7 " W1("o2 a") W1("o2 f") W1("o2 g") W1("o2 e") W1("o2 d") W1("o2 a") W1("o2 e") W1("o2 a");
static const char FLY_DRUM[] = "[" BRUSH "]8";

/* "Ghost Town" - Hollow Gap and Hope Springs under the devils, a dead waltz */
static const char GHOST_LEAD[] = "@15 v9 q6 [o5 e4 g4 b4 a4 g4 e4 f+2. e4 d+4 e4 b2. r4 o4 b4 o5 c4 e2. d+2.]2";
static const char GHOST_PAD[] = "@4 v4 q8 [o4 e2. e2. d+2. e2. g2. c2. a2. b2.]2";
static const char GHOST_BASS[] = "@6 v10 q4 [o2 e4 r4 r4 o2 e4 r4 r4 o2 b4 r4 r4 o2 e4 r4 r4 o2 g4 r4 r4 o2 c4 r4 r4 o2 a4 r4 r4 o2 b4 r4 r4]2";
static const char GHOST_DRUM[] = "[@9 v3 o8 c4 r4 r4]16";

/* "Buy Something" - the counters */
static const char SHOP_LEAD[] =
    "@3 v11 q4 " E8("o5 c", "e", "g", "e", "a", "g", "e", "c") E8("o5 d", "f", "a", "f", "b", "a", "f", "d")
    E8("o5 e", "g", "o6 c", "o5 g", "a", "f", "d", "o4 b") Q4("o5 c", "e", "c", "r");
static const char SHOP_COMP[] = "@16 v5 q3 " OOMPAH("o3 c", "o4 e") OOMPAH("o3 d", "o4 f") OOMPAH("o3 e", "o4 g") OOMPAH("o3 c", "o4 c");
static const char SHOP_BASS[] = "@7 v12 q5 " Q4("o2 c", "g", "c", "g") Q4("o2 d", "a", "d", "a") Q4("o2 e", "g", "f", "g") Q4("o2 c", "g", "c", "r");
static const char SHOP_DRUM[] = "[" BRUSH "]4";

/* "The Last Hand" - Zenith and Nadir */
static const char FIN_LEAD[] =
    "@23 v12 q7 " Q4("o5 d", "a", "o6 d", "c") DQ("o5 b-", "a", "g") Q4("o5 f", "a", "o6 c", "e") DQ("o6 d", "c+", "o5 a")
    Q4("o6 d", "f", "e", "d") DQ("o6 c", "o5 b-", "a") Q4("o5 g", "b-", "a", "c+") W1("o5 d")
    Q4("o5 a", "b-", "o6 c", "d") Q4("o6 e-", "d", "c", "o5 b-") Q4("o5 a", "g", "f", "e") H2("o5 f", "a")
    Q4("o6 d", "c", "o5 b-", "a") Q4("o5 b-", "a", "g", "f") Q4("o5 e", "f", "g", "c+") W1("o5 d");
static const char FIN_STAB[] = "@17 v6 q3 [" EE("o4 d") EE("o4 g") EE("o4 f") EE("o4 a") EE("o4 d") EE("o4 b-") EE("o4 g") EE("o4 a") "]2";
static const char FIN_BASS[] = "@6 v14 q4 [[o2 d8 o3 d8]4 [o2 g8 o3 g8]4 [o2 f8 o3 f8]4 [o2 a8 o3 a8]4 [o2 b-8 o3 b-8]4 [o2 g8 o3 g8]4 [o2 a8 o3 a8]4 [o2 d8 o3 d8]4]2";
static const char FIN_DRUM[] = "[" DRIVE "]16";

/* "Sun Comes Up Yellow" - the ending */
static const char END_LEAD[] =
    "@14 v11 q7 " DQ("o5 g", "a", "b") Q4("o6 d", "c", "o5 b", "a") DQ("o5 g", "e", "d") W1("o5 g")
    DQ("o5 b", "o6 c", "d") Q4("o6 e", "d", "c", "o5 b") DQ("o5 a", "g", "f+") W1("o5 g");
static const char END_COMP[] = "@22 v6 q6 " SH("o4 g", "b", "g", "b", "g", "b", "g", "b") SH("o4 e", "g", "e", "g", "e", "g", "e", "g")
    SH("o4 c", "e", "c", "e", "d", "f+", "d", "f+") SH("o4 g", "b", "g", "b", "g", "b", "g", "b")
    SH("o4 g", "b", "g", "b", "g", "b", "g", "b") SH("o4 c", "e", "c", "e", "c", "e", "c", "e")
    SH("o4 d", "f+", "d", "f+", "d", "f+", "d", "f+") SH("o4 g", "b", "g", "b", "g", "b", "g", "b");
static const char END_BASS[] = "@6 v12 q6 " WALK("o2 g", "d") WALK("o2 e", "b") WALK("o2 c", "g") WALK("o2 g", "d")
    WALK("o2 g", "d") WALK("o2 c", "g") WALK("o2 d", "a") WALK("o2 g", "d");
static const char END_DRUM[] = "[" TRAIN "]8";

/* jingles */
static const char WIN_P1[] = "@1 v12 q6 o5 l12 a o6 c+ e a8 r8 e8 a2";
static const char WIN_P2[] = "@22 v8 q6 o5 l12 e a o6 c+ e8 r8 c+8 e2";
static const char WIN_TRI[] = "@6 v12 q6 o2 a8 r8 e8 r8 a2";
static const char LOSE_P1[] = "@5 v12 q7 o5 a4 g4 f4 e4 d2 c+2 d1";
static const char LOSE_TRI[] = "@6 v12 q8 o2 d2 a2 b-2 a2 d1";
static const char LEVEL_P1[] = "@39 v12 q7 o5 l16 g b o6 d g d g4";
static const char LEVEL_TRI[] = "@6 v12 q7 o3 g8 d8 g4";

void spur_audio_load(void) {
    if (SPUR_MUS_TITLE >= 0) return;
    SPUR_MUS_TITLE = song_define("spur_title", 92, true, TITLE_LEAD, TITLE_HARM, TITLE_BASS, TITLE_DRUM);
    SPUR_MUS_SALOON = song_define("spur_saloon", 132, true, SAL_LEAD, SAL_COMP, SAL_BASS, SAL_DRUM);
    SPUR_MUS_TOWN = song_define("spur_town", 112, true, TOWN_LEAD, TOWN_COMP, TOWN_BASS, TOWN_DRUM);
    SPUR_MUS_WORLD = song_define("spur_world", 104, true, WORLD_LEAD, WORLD_HARM, WORLD_BASS, WORLD_DRUM);
    SPUR_MUS_BATTLE = song_define("spur_battle", 152, true, BAT_LEAD, BAT_STAB, BAT_BASS, BAT_DRUM);
    SPUR_MUS_BOSS = song_define("spur_boss", 138, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    SPUR_MUS_CAVE = song_define("spur_cave", 90, true, CAVE_LEAD, CAVE_PAD, CAVE_BASS, CAVE_DRUM);
    SPUR_MUS_DREAM = song_define("spur_dream", 78, true, DREAM_LEAD, DREAM_ARP, DREAM_BASS, DREAM_DRUM);
    SPUR_MUS_HELL = song_define("spur_hell", 96, true, HELL_LEAD, HELL_PAD, HELL_BASS, HELL_DRUM);
    SPUR_MUS_CITADEL = song_define("spur_citadel", 108, true, CIT_LEAD, CIT_STAB, CIT_BASS, CIT_DRUM);
    SPUR_MUS_CHURCH = song_define("spur_church", 72, true, CH_LEAD, CH_ALTO, CH_BASS, CH_DRUM);
    SPUR_MUS_RIDE = song_define("spur_ride", 160, true, RIDE_LEAD, RIDE_COMP, RIDE_BASS, RIDE_DRUM);
    SPUR_MUS_FLY = song_define("spur_fly", 116, true, FLY_LEAD, FLY_ARP, FLY_BASS, FLY_DRUM);
    SPUR_MUS_GHOST = song_define("spur_ghost", 100, true, GHOST_LEAD, GHOST_PAD, GHOST_BASS, GHOST_DRUM);
    SPUR_MUS_SHOP = song_define("spur_shop", 128, true, SHOP_LEAD, SHOP_COMP, SHOP_BASS, SHOP_DRUM);
    SPUR_MUS_FINAL = song_define("spur_final", 148, true, FIN_LEAD, FIN_STAB, FIN_BASS, FIN_DRUM);
    SPUR_MUS_ENDING = song_define("spur_ending", 100, true, END_LEAD, END_COMP, END_BASS, END_DRUM);
    SPUR_MUS_WIN = song_define("spur_win", 140, false, WIN_P1, WIN_P2, WIN_TRI, "");
    SPUR_MUS_LOSE = song_define("spur_lose", 90, false, LOSE_P1, "", LOSE_TRI, "");
    SPUR_MUS_LEVEL = song_define("spur_level", 150, false, LEVEL_P1, "", LEVEL_TRI, "");

    sfx_define("spur_wisp", CH_P2, 200, "@14 v6 o6 c32 g32 o7 c32");
    sfx_define("spur_encounter", CH_NOISE, 160, "@36 v12 o4 c8 o5 c8");
    sfx_define("spur_ambush", CH_P1, 180, "@37 v12 o5 c16 o4 g16 c8");
    sfx_define("spur_hit", CH_NOISE, 240, "@21 v10 o5 c16");
    sfx_define("spur_crit", CH_P1, 220, "@39 v13 o6 c32 g32 o7 c16");
    sfx_define("spur_miss", CH_P2, 240, "@20 v6 o4 c32");
    sfx_define("spur_heavy", CH_NOISE, 240, "@40 v9 o5 c32");
    sfx_define("spur_kill", CH_NOISE, 160, "@34 v11 o4 c8");
    sfx_define("spur_ouch", CH_P1, 220, "@37 v10 o5 c16 o4 g16");
    sfx_define("spur_fall", CH_P1, 140, "@33 v12 o5 c8 o4 a8 f8 d4");
    sfx_define("spur_fire", CH_NOISE, 140, "@34 v12 o3 c4");
    sfx_define("spur_heal", CH_P2, 200, "@39 v10 o5 c16 e16 g16 o6 c16");
    sfx_define("spur_skill", CH_P2, 220, "@15 v8 o6 c16 o5 g16");
    sfx_define("spur_learn", CH_P1, 180, "@39 v12 o5 l16 c e g o6 c e g o7 c8");
    sfx_define("spur_raise", CH_P1, 160, "@39 v12 o4 l16 g o5 c e g o6 c4");
    sfx_define("spur_run", CH_NOISE, 200, "@36 v8 o4 c16 c16 c16");
    sfx_define("spur_reload", CH_NOISE, 220, "@40 v8 o6 c32 r32 o5 c16");
    sfx_define("spur_menu", CH_P2, 220, "@42 v9 o6 e32 g32");
    sfx_define("spur_tick", CH_P2, 240, "@42 v7 o6 c32");
    sfx_define("spur_ok", CH_P2, 220, "@20 v10 o5 c32 g32 o6 c16");
    sfx_define("spur_back", CH_P2, 220, "@20 v9 o5 g32 d32 o4 g16");
    sfx_define("spur_no", CH_P2, 200, "@37 v10 o3 c16 r32 c16");
    sfx_define("spur_buy", CH_P1, 240, "@35 v11 o6 c16 g16 o7 c16");
    sfx_define("spur_equip", CH_NOISE, 220, "@21 v8 o6 c32 c32");
    sfx_define("spur_drop", CH_NOISE, 220, "@13 v8 o3 c16");
    sfx_define("spur_chest", CH_P1, 200, "@35 v11 o5 c16 e16 g16 o6 c8");
    sfx_define("spur_key", CH_P1, 180, "@39 v12 o6 c16 e16 g16 o7 c8");
    sfx_define("spur_door", CH_NOISE, 200, "@41 v10 o3 c8");
    sfx_define("spur_stairs", CH_P2, 200, "@38 v9 o4 c16 e16 g16");
    sfx_define("spur_warp", CH_P1, 160, "@39 v11 o5 l32 c d e f g a b o6 c d e f g");
    sfx_define("spur_mount", CH_NOISE, 200, "@36 v9 o4 c16 o5 c16");
    sfx_define("spur_dismount", CH_NOISE, 200, "@13 v8 o3 c16");
    sfx_define("spur_wings", CH_NOISE, 160, "@36 v10 o3 c8 o4 c8");
    sfx_define("spur_train", CH_P1, 140, "@1 v11 o5 a4 f+4 a2");
    sfx_define("spur_boom", CH_NOISE, 120, "@34 v15 o2 c2");
    sfx_define("spur_bell", CH_P1, 140, "@15 v13 o6 a4 o7 c+4 e2");
}
