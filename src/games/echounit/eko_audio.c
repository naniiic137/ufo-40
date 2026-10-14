/* ECHO UNIT - original music (UFO-MML) and sound effects. Every looping tune
 * is built from whole bars (the macros below each make exactly one bar), so
 * the four channels loop together. Saturday-morning action: driving bass,
 * bright brass, a theme for each city. */
#include "eko.h"

int EKO_MUS_TITLE = -1, EKO_MUS_STORY, EKO_MUS_HQ, EKO_MUS_BRIEF, EKO_MUS_DOCKS, EKO_MUS_ANDES, EKO_MUS_CAIRO,
    EKO_MUS_ISTANBUL, EKO_MUS_RESCUE, EKO_MUS_BOSS, EKO_MUS_CALDERA, EKO_MUS_LAB, EKO_MUS_TUNNEL, EKO_MUS_FINAL,
    EKO_MUS_CLEAR, EKO_MUS_CAPTURED, EKO_MUS_OVER, EKO_MUS_ENDING, EKO_MUS_LIFT;

/* one bar each */
#define Q4(a, b, c, d) a "4 " b "4 " c "4 " d "4 "
#define E8(a, b, c, d, e, f, g, h) a "8 " b "8 " c "8 " d "8 " e "8 " f "8 " g "8 " h "8 "
#define EE(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define H2(a, b) a "2 " b "2 "
#define W1(a) a "1 "
#define DQ(a, b, c) a "4. " b "8 " c "2 "
#define OCT(n) "[o2 " n "8 o3 " n "8]4 "
/* drums, one bar */
#define BEAT "@13 v12 o2 c8 @9 v4 o8 c8 @11 v9 o6 c8 @9 v4 o8 c8 @13 v12 o2 c8 @13 v9 o2 c8 @11 v9 o6 c8 @9 v5 o8 c8 "
#define DRIVE "@13 v12 o2 c8 @9 v5 o8 c8 @11 v9 o6 c8 @9 v5 o8 c16 c16 @13 v12 o2 c8 @9 v5 o8 c8 @11 v9 o6 c8 @11 v6 o6 c8 "
#define SOFT "@9 v4 o8 c4 @11 v5 o6 c4 @9 v4 o8 c4 @11 v5 o6 c4 "
#define TICK "@21 v4 o7 c8 @9 v2 o8 c8 @21 v3 o7 c8 @9 v2 o8 c8 @21 v4 o7 c8 @9 v2 o8 c8 @21 v3 o7 c8 @9 v2 o8 c8 "
#define MARCH "@11 v9 o6 c8 @11 v5 o6 c16 c16 @11 v9 o6 c8 @9 v4 o8 c8 @13 v11 o2 c4 @11 v9 o6 c4 "

/* "Echo Unit" - the title, A minor, brass over a pumping bass */
static const char TITLE_LEAD[] =
    "@23 v12 q7 "
    Q4("o5 a", "o6 c", "e", "d") DQ("o6 c", "o5 b", "a")
    Q4("o5 g", "a", "b", "o6 e") W1("o6 e")
    Q4("o5 a", "o6 c", "e", "g") DQ("o6 f", "e", "d")
    Q4("o6 c", "o5 b", "a", "g") W1("o5 a");
static const char TITLE_HARM[] =
    "@22 v7 q6 "
    H2("o4 e", "e") H2("o4 e", "e") H2("o4 d", "d") H2("o4 e", "g+")
    H2("o4 e", "e") H2("o4 f", "f") H2("o4 e", "d") H2("o4 e", "e");
static const char TITLE_BASS[] =
    "@6 v13 q6 " OCT("a") OCT("a") OCT("g") OCT("e") OCT("a") OCT("f") OCT("g") OCT("a");
static const char TITLE_DRUM[] = "[" DRIVE "]8";

/* "Hanging in the Belfry" - the story, slow and low */
static const char STORY_LEAD[] =
    "@5 v10 q7 " DQ("o5 e", "d", "c") H2("o4 b", "o5 c") DQ("o5 d", "c", "o4 b") W1("o4 a")
    DQ("o5 e", "f", "g") H2("o5 a", "g") DQ("o5 f", "e", "d") W1("o5 e");
static const char STORY_PAD[] = "@4 v5 q8 " W1("o4 a") W1("o4 e") W1("o4 f") W1("o4 a") W1("o4 c") W1("o4 f") W1("o4 d") W1("o4 e");
static const char STORY_BASS[] = "@6 v11 q8 " W1("o2 a") W1("o2 e") W1("o2 d") W1("o2 a") W1("o2 c") W1("o2 f") W1("o2 d") W1("o2 e");
static const char STORY_DRUM[] = "[" SOFT "]8";

/* "Sound the Roost" - the briefing room board, D major, jaunty */
static const char HQ_LEAD[] =
    "@14 v10 q6 " E8("o5 d", "f+", "a", "f+", "b", "a", "f+", "d") Q4("o5 e", "f+", "e", "r")
    E8("o5 d", "f+", "a", "o6 d", "c+", "o5 b", "a", "f+") Q4("o5 g", "f+", "e", "r")
    E8("o5 b", "a", "g", "f+", "g", "a", "b", "o6 c+") Q4("o6 d", "c+", "o5 b", "a")
    E8("o5 g", "f+", "e", "d", "e", "f+", "g", "e") W1("o5 d");
static const char HQ_ARP[] = "@16 v5 q4 " EE("o4 d") EE("o4 a") EE("o4 d") EE("o4 e") EE("o4 g") EE("o4 d") EE("o4 a") EE("o4 d");
static const char HQ_BASS[] = "@6 v12 q5 " Q4("o2 d", "a", "d", "a") Q4("o2 a", "e", "a", "e") Q4("o2 d", "a", "d", "a")
    Q4("o2 a", "e", "a", "e") Q4("o2 g", "d", "g", "d") Q4("o2 d", "a", "d", "a") Q4("o2 a", "e", "a", "e") Q4("o2 d", "a", "d", "d");
static const char HQ_DRUM[] = "[" BEAT "]8";

/* "Colonel Hush" - the briefing, over a ticking clock */
static const char BRIEF_LEAD[] =
    "@5 v9 q6 " Q4("o5 e", "r", "e", "f") H2("o5 g", "e") Q4("o5 d", "r", "d", "e") H2("o5 f", "d")
    Q4("o5 c", "r", "c", "d") H2("o5 e", "c") Q4("o4 b", "o5 c", "d", "e") W1("o4 b");
static const char BRIEF_PAD[] = "@4 v4 q8 " W1("o4 a") W1("o4 a") W1("o4 g") W1("o4 g") W1("o4 f") W1("o4 f") W1("o4 e") W1("o4 e");
static const char BRIEF_BASS[] = "@6 v10 q4 " EE("o2 a") EE("o2 a") EE("o2 g") EE("o2 g") EE("o2 f") EE("o2 f") EE("o2 e") EE("o2 e");
static const char BRIEF_DRUM[] = "[" TICK "]8";

/* "Dockside Dust-Up" - Marseille, E minor rock */
static const char DOCKS_LEAD[] =
    "@1 v11 q6 " E8("o5 e", "e", "g", "e", "a", "e", "b", "a") Q4("o5 g", "e", "d", "e")
    E8("o5 e", "e", "g", "e", "a", "b", "o6 d", "c") Q4("o5 b", "a", "g", "a")
    E8("o5 b", "b", "o6 d", "o5 b", "o6 e", "d", "c", "o5 b") Q4("o5 a", "g", "a", "b")
    E8("o5 g", "a", "b", "o6 d", "e", "d", "o5 b", "a") W1("o5 e");
static const char DOCKS_RIFF[] = "@3 v7 q5 " EE("o4 e") EE("o4 e") EE("o4 c") EE("o4 d") EE("o4 e") EE("o4 g") EE("o4 d") EE("o4 e");
static const char DOCKS_BASS[] = "@6 v13 q5 " OCT("e") OCT("e") OCT("c") OCT("d") OCT("e") OCT("g") OCT("d") OCT("e");
static const char DOCKS_DRUM[] = "[" DRIVE "]8";

/* "Going Up" - the crane lift, pushing up a semitone */
static const char LIFT_LEAD[] =
    "@23 v11 q7 " Q4("o5 e", "g", "b", "a") H2("o5 g", "e") Q4("o5 f", "a", "o6 c", "o5 b") H2("o5 a", "f")
    Q4("o5 f+", "a", "o6 c+", "o5 b") H2("o5 a", "f+") Q4("o5 g", "b", "o6 d", "e") W1("o6 e");
static const char LIFT_ARP[] = "@17 v5 q4 " EE("o4 e") EE("o4 e") EE("o4 f") EE("o4 f") EE("o4 f+") EE("o4 f+") EE("o4 g") EE("o4 g");
static const char LIFT_BASS[] = "@6 v13 q5 " OCT("e") OCT("e") OCT("f") OCT("f") OCT("f+") OCT("f+") OCT("g") OCT("g");
static const char LIFT_DRUM[] = "[" DRIVE "]8";

/* "Thin Air" - the Andes, a mountain melody in G mixolydian */
static const char ANDES_LEAD[] =
    "@14 v11 q7 " Q4("o5 g", "a", "b", "o6 d") DQ("o6 e", "d", "o5 b") Q4("o5 a", "g", "f", "a") W1("o5 g")
    Q4("o5 b", "o6 c", "d", "f") DQ("o6 e", "d", "c") Q4("o5 b", "a", "f", "a") W1("o5 g");
static const char ANDES_ARP[] = "@2 v6 q4 " EE("o4 g") EE("o4 g") EE("o4 f") EE("o4 g") EE("o4 b") EE("o4 c") EE("o4 f") EE("o4 g");
static const char ANDES_BASS[] = "@6 v12 q6 " Q4("o2 g", "o3 d", "o2 g", "o3 d") Q4("o2 g", "o3 d", "o2 g", "o3 d")
    Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 g", "o3 d", "o2 g", "o3 d") Q4("o2 b-", "o3 f", "o2 b-", "o3 f")
    Q4("o2 c", "g", "o3 c", "o2 g") Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 g", "o3 d", "o2 g", "o2 g");
static const char ANDES_DRUM[] = "[" MARCH "]8";

/* "Sand and Throttle" - Cairo, D phrygian at speed */
static const char CAIRO_LEAD[] =
    "@1 v11 q6 " E8("o5 d", "e-", "f+", "g", "a", "g", "f+", "e-") Q4("o5 d", "a", "d", "r")
    E8("o5 d", "e-", "f+", "a", "b-", "a", "g", "f+") Q4("o5 g", "f+", "e-", "r")
    E8("o5 a", "b-", "o6 c", "d", "e-", "d", "c", "o5 b-") Q4("o5 a", "g", "f+", "e-")
    E8("o5 d", "f+", "a", "o6 d", "c", "o5 b-", "a", "f+") W1("o5 d");
static const char CAIRO_ARP[] = "@18 v5 q3 " EE("o4 d") EE("o4 d") EE("o4 d") EE("o4 e-") EE("o4 f+") EE("o4 g") EE("o4 e-") EE("o4 d");
static const char CAIRO_BASS[] = "@6 v13 q4 " OCT("d") OCT("d") OCT("d") OCT("e-") OCT("f+") OCT("g") OCT("e-") OCT("d");
static const char CAIRO_DRUM[] = "[" DRIVE "]8";

/* "Silent Halls" - Istanbul, sneaking in B minor */
static const char IST_LEAD[] =
    "@5 v9 q5 " Q4("o4 b", "r", "o5 d", "r") Q4("o5 c+", "r", "o4 b", "r") Q4("o4 a", "r", "o5 c+", "r") H2("o4 b", "r")
    Q4("o5 d", "r", "f+", "r") Q4("o5 e", "r", "d", "r") Q4("o5 c+", "r", "o4 a+", "r") H2("o4 b", "r");
static const char IST_PAD[] = "@4 v4 q8 " W1("o4 b") W1("o4 b") W1("o4 a") W1("o4 b") W1("o4 g") W1("o4 g") W1("o4 f+") W1("o4 f+");
static const char IST_BASS[] = "@6 v11 q3 " EE("o2 b") EE("o2 b") EE("o2 a") EE("o2 b") EE("o2 g") EE("o2 g") EE("o2 f+") EE("o2 f+");
static const char IST_DRUM[] = "[" TICK "]8";

/* "Cell Block" - the jailbreak, a plodding puzzle */
static const char RES_LEAD[] =
    "@15 v9 q4 " Q4("o5 c", "e-", "g", "e-") Q4("o5 f", "a-", "g", "r") Q4("o5 c", "e-", "g", "b-") Q4("o5 a-", "g", "f", "r")
    Q4("o5 e-", "f", "g", "a-") Q4("o5 b-", "a-", "g", "f") Q4("o5 e-", "d", "c", "d") W1("o5 c");
static const char RES_PAD[] = "@4 v4 q8 " W1("o4 c") W1("o4 f") W1("o4 c") W1("o4 f") W1("o4 e-") W1("o4 e-") W1("o4 g") W1("o4 c");
static const char RES_BASS[] = "@6 v11 q5 " Q4("o2 c", "g", "c", "g") Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 c", "g", "c", "g")
    Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 a-", "o3 e-", "o2 a-", "o3 e-") Q4("o2 b-", "o3 f", "o2 b-", "o3 f")
    Q4("o2 g", "o3 d", "o2 g", "o3 d") Q4("o2 c", "g", "c", "c");
static const char RES_DRUM[] = "[" SOFT "]8";

/* "Big Bad" - every boss in the four cities */
static const char BOSS_LEAD[] =
    "@1 v12 q6 " E8("o5 c", "c", "e-", "c", "f+", "c", "g", "f+") E8("o5 e-", "c", "o4 b-", "o5 c", "e-", "f", "f+", "g")
    E8("o5 c", "c", "e-", "c", "f+", "c", "g", "b-") Q4("o5 a-", "g", "f+", "e-")
    E8("o5 a-", "a-", "o6 c", "o5 a-", "o6 d", "c", "o5 b-", "a-") E8("o5 g", "g", "b-", "g", "o6 c", "o5 b-", "a-", "g")
    E8("o5 f+", "g", "a-", "b-", "o6 c", "d", "e-", "f+") W1("o6 g");
static const char BOSS_STAB[] = "@17 v6 q3 " EE("o4 c") EE("o4 c") EE("o4 c") EE("o4 e-") EE("o4 a-") EE("o4 g") EE("o4 f+") EE("o4 g");
static const char BOSS_BASS[] = "@6 v14 q4 " OCT("c") OCT("c") OCT("c") OCT("e-") OCT("a-") OCT("g") OCT("f+") OCT("g");
static const char BOSS_DRUM[] = "[" DRIVE "]8";

/* "Into the Caldera" - the finale's halls, low and hot */
static const char CAL_LEAD[] =
    "@19 v11 q6 " Q4("o4 e", "f", "e", "r") Q4("o4 g", "f", "e", "r") Q4("o4 e", "f", "a", "g") H2("o4 f", "e")
    Q4("o5 c", "o4 b", "a", "r") Q4("o4 b", "a", "g+", "r") Q4("o4 a", "g+", "f", "e") W1("o4 e");
static const char CAL_PAD[] = "@4 v5 q8 " W1("o4 e") W1("o4 e") W1("o4 f") W1("o4 e") W1("o4 a") W1("o4 g+") W1("o4 f") W1("o4 e");
static const char CAL_BASS[] = "@6 v12 q4 " EE("o2 e") EE("o2 e") EE("o2 f") EE("o2 e") EE("o2 a") EE("o2 g+") EE("o2 f") EE("o2 e");
static const char CAL_DRUM[] = "[" MARCH "]8";

/* "The Secret Lab" - Flint's climb, bubbling */
static const char LAB_LEAD[] =
    "@14 v10 q5 " E8("o5 a", "o6 c", "e", "c", "o5 a", "o6 c", "e", "a") Q4("o6 g", "e", "c", "o5 a")
    E8("o5 f", "a", "o6 c", "o5 a", "f", "a", "o6 c", "f") Q4("o6 e", "c", "o5 a", "f")
    E8("o5 d", "f", "a", "f", "d", "f", "a", "o6 d") Q4("o6 c", "o5 b", "a", "g+")
    E8("o5 e", "g+", "b", "o6 e", "d", "c", "o5 b", "g+") W1("o5 a");
static const char LAB_ARP[] = "@20 v4 q3 " EE("o6 a") EE("o6 a") EE("o6 f") EE("o6 f") EE("o6 d") EE("o6 d") EE("o6 e") EE("o6 e");
static const char LAB_BASS[] = "@6 v13 q5 " OCT("a") OCT("a") OCT("f") OCT("f") OCT("d") OCT("d") OCT("e") OCT("e");
static const char LAB_DRUM[] = "[" BEAT "]8";

/* "Lava Run" - Dart's tunnel */
static const char TUN_LEAD[] =
    "@1 v11 q6 " E8("o5 e", "g", "a", "b", "a", "g", "e", "d") Q4("o5 e", "r", "b", "r")
    E8("o5 e", "g", "a", "b", "o6 d", "c", "o5 b", "a") Q4("o5 g", "r", "e", "r")
    E8("o5 c", "e", "g", "a", "g", "e", "c", "o4 b") Q4("o5 c", "r", "g", "r")
    E8("o5 d", "f+", "a", "b", "a", "f+", "d", "f+") W1("o5 e");
static const char TUN_ARP[] = "@18 v5 q3 " EE("o4 e") EE("o4 e") EE("o4 e") EE("o4 e") EE("o4 c") EE("o4 c") EE("o4 d") EE("o4 e");
static const char TUN_BASS[] = "@6 v13 q4 " OCT("e") OCT("e") OCT("e") OCT("e") OCT("c") OCT("c") OCT("d") OCT("e");
static const char TUN_DRUM[] = "[" DRIVE "]8";

/* "Madame Coil" - the last fight */
static const char FIN_LEAD[] =
    "@23 v12 q7 " Q4("o5 d", "f", "a", "o6 d") DQ("o6 c+", "o5 b-", "a") Q4("o5 g", "b-", "o6 d", "g") DQ("o6 f", "e", "c+")
    Q4("o6 d", "f", "e", "d") DQ("o6 c+", "o5 b-", "g") Q4("o5 a", "b-", "o6 c+", "e") W1("o6 d");
static const char FIN_STAB[] = "@16 v6 q3 " EE("o4 d") EE("o4 b-") EE("o4 g") EE("o4 a") EE("o4 d") EE("o4 b-") EE("o4 e") EE("o4 d");
static const char FIN_BASS[] = "@6 v14 q4 " OCT("d") OCT("b-") OCT("g") OCT("a") OCT("d") OCT("b-") OCT("a") OCT("d");
static const char FIN_DRUM[] = "[" DRIVE "]8";

/* "Mango Feast" - the ending */
static const char END_LEAD[] =
    "@14 v11 q7 " Q4("o5 c", "e", "g", "o6 c") DQ("o5 b", "a", "g") Q4("o5 f", "a", "o6 c", "f") DQ("o6 e", "d", "c")
    Q4("o5 a", "o6 c", "e", "a") DQ("o6 g", "f", "e") Q4("o6 d", "c", "o5 b", "o6 d") W1("o6 c");
static const char END_PAD[] = "@22 v6 q7 " H2("o4 e", "g") H2("o4 e", "e") H2("o4 f", "a") H2("o4 g", "g") H2("o4 a", "e")
    H2("o4 e", "c") H2("o4 f", "g") W1("o4 e");
static const char END_BASS[] = "@6 v12 q6 " Q4("o2 c", "g", "c", "g") Q4("o2 e", "b", "e", "b") Q4("o2 f", "o3 c", "o2 f", "o3 c")
    Q4("o2 g", "o3 d", "o2 g", "o3 d") Q4("o2 a", "o3 e", "o2 a", "o3 e") Q4("o2 e", "b", "e", "b")
    Q4("o2 f", "o3 c", "o2 g", "o3 d") Q4("o2 c", "g", "c", "c");
static const char END_DRUM[] = "[" BEAT "]8";

/* jingles */
static const char CLEAR_P1[] = "@23 v13 q7 o5 l16 c e g o6 c8 o5 g16 o6 c4. e4 r4";
static const char CLEAR_P2[] = "@22 v8 q7 o4 l16 e g o5 c e8 c16 e4. g4 r4";
static const char CLEAR_TRI[] = "@6 v12 q7 o2 c8 g8 o3 c4. c4 r4";
static const char CAPT_P1[] = "@5 v12 q7 o5 e8 d8 c8 o4 b8 a4 g+4 a2";
static const char CAPT_TRI[] = "@6 v12 q8 o2 a4 e4 a2 a2";
static const char OVER_P1[] = "@5 v12 q7 o5 a4 g4 f4 e4 d2 c+2 d1";
static const char OVER_TRI[] = "@6 v12 q8 o2 d2 a2 b-2 a2 d1";

void eko_audio_load(void) {
    if (EKO_MUS_TITLE >= 0) return;
    EKO_MUS_TITLE = song_define("eko_title", 138, true, TITLE_LEAD, TITLE_HARM, TITLE_BASS, TITLE_DRUM);
    EKO_MUS_STORY = song_define("eko_story", 84, true, STORY_LEAD, STORY_PAD, STORY_BASS, STORY_DRUM);
    EKO_MUS_HQ = song_define("eko_hq", 126, true, HQ_LEAD, HQ_ARP, HQ_BASS, HQ_DRUM);
    EKO_MUS_BRIEF = song_define("eko_brief", 100, true, BRIEF_LEAD, BRIEF_PAD, BRIEF_BASS, BRIEF_DRUM);
    EKO_MUS_DOCKS = song_define("eko_docks", 148, true, DOCKS_LEAD, DOCKS_RIFF, DOCKS_BASS, DOCKS_DRUM);
    EKO_MUS_LIFT = song_define("eko_lift", 152, true, LIFT_LEAD, LIFT_ARP, LIFT_BASS, LIFT_DRUM);
    EKO_MUS_ANDES = song_define("eko_andes", 132, true, ANDES_LEAD, ANDES_ARP, ANDES_BASS, ANDES_DRUM);
    EKO_MUS_CAIRO = song_define("eko_cairo", 160, true, CAIRO_LEAD, CAIRO_ARP, CAIRO_BASS, CAIRO_DRUM);
    EKO_MUS_ISTANBUL = song_define("eko_istanbul", 104, true, IST_LEAD, IST_PAD, IST_BASS, IST_DRUM);
    EKO_MUS_RESCUE = song_define("eko_rescue", 96, true, RES_LEAD, RES_PAD, RES_BASS, RES_DRUM);
    EKO_MUS_BOSS = song_define("eko_boss", 156, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    EKO_MUS_CALDERA = song_define("eko_caldera", 112, true, CAL_LEAD, CAL_PAD, CAL_BASS, CAL_DRUM);
    EKO_MUS_LAB = song_define("eko_lab", 140, true, LAB_LEAD, LAB_ARP, LAB_BASS, LAB_DRUM);
    EKO_MUS_TUNNEL = song_define("eko_tunnel", 164, true, TUN_LEAD, TUN_ARP, TUN_BASS, TUN_DRUM);
    EKO_MUS_FINAL = song_define("eko_final", 150, true, FIN_LEAD, FIN_STAB, FIN_BASS, FIN_DRUM);
    EKO_MUS_ENDING = song_define("eko_ending", 110, true, END_LEAD, END_PAD, END_BASS, END_DRUM);
    EKO_MUS_CLEAR = song_define("eko_clear", 140, false, CLEAR_P1, CLEAR_P2, CLEAR_TRI, "");
    EKO_MUS_CAPTURED = song_define("eko_captured", 110, false, CAPT_P1, "", CAPT_TRI, "");
    EKO_MUS_OVER = song_define("eko_over", 90, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("eko_swing", CH_NOISE, 240, "@36 v7 o6 c32");
    sfx_define("eko_lunge", CH_NOISE, 200, "@36 v10 o4 c8");
    sfx_define("eko_charged", CH_P2, 240, "@39 v8 o6 c32 e32 g32");
    sfx_define("eko_hit", CH_NOISE, 240, "@21 v9 o6 c32");
    sfx_define("eko_ko", CH_NOISE, 200, "@34 v9 o4 c16");
    sfx_define("eko_jump", CH_P1, 240, "@32 v8 o4 c32 g32");
    sfx_define("eko_throw", CH_NOISE, 220, "@36 v8 o5 c16");
    sfx_define("eko_lift", CH_P2, 240, "@38 v8 o3 c16");
    sfx_define("eko_clang", CH_NOISE, 240, "@40 v8 o6 c16");
    sfx_define("eko_toss", CH_NOISE, 240, "@36 v6 o5 c32");
    sfx_define("eko_boom", CH_NOISE, 140, "@34 v13 o3 c4");
    sfx_define("eko_shot", CH_NOISE, 240, "@21 v7 o7 c32");
    sfx_define("eko_gun", CH_NOISE, 240, "@40 v9 o6 c32");
    sfx_define("eko_pew", CH_P2, 240, "@20 v6 o6 c32 g32");
    sfx_define("eko_shotgun", CH_NOISE, 200, "@40 v11 o5 c16");
    sfx_define("eko_tink", CH_P2, 240, "@42 v8 o7 c32");
    sfx_define("eko_fall", CH_P1, 160, "@33 v10 o5 c4");
    sfx_define("eko_down", CH_P1, 140, "@33 v12 o5 c8 o4 a8 f8 d4");
    sfx_define("eko_ouch", CH_P1, 220, "@37 v10 o5 c16 o4 g16");
    sfx_define("eko_crash", CH_NOISE, 120, "@34 v14 o2 c2");
    sfx_define("eko_bossdie", CH_NOISE, 90, "@34 v15 o2 c2");
    sfx_define("eko_bossin", CH_P1, 140, "@33 v13 o3 c4 o2 g4");
    sfx_define("eko_thud", CH_NOISE, 160, "@41 v13 o2 c8");
    sfx_define("eko_drill", CH_NOISE, 200, "@40 v8 o3 c8 c8");
    sfx_define("eko_zap", CH_P2, 220, "@37 v8 o6 c16 o5 c16");
    sfx_define("eko_beep", CH_P2, 240, "@42 v10 o6 c16 r16 c16");
    sfx_define("eko_door", CH_P1, 200, "@38 v10 o4 c16 g16 o5 c16");
    sfx_define("eko_secret", CH_P1, 200, "@39 v12 o6 l32 c d e f g a b o7 c");
    sfx_define("eko_mango", CH_P1, 220, "@35 v11 o6 c16 e16 g16");
    sfx_define("eko_1up", CH_P1, 200, "@39 v12 o5 l16 c e g o6 c e g");
    sfx_define("eko_key", CH_P1, 200, "@35 v12 o6 e16 b16 o7 e16");
    sfx_define("eko_roll", CH_NOISE, 220, "@36 v7 o4 c16");
    sfx_define("eko_missile", CH_NOISE, 200, "@36 v8 o3 c8");
    sfx_define("eko_hop", CH_P1, 240, "@32 v8 o4 e32 b32");
    sfx_define("eko_land", CH_NOISE, 240, "@13 v8 o3 c32");
    sfx_define("eko_rev", CH_P2, 160, "@37 v10 o3 c8 e8 g8");
    sfx_define("eko_dart", CH_P2, 240, "@20 v5 o7 c32");
    sfx_define("eko_zzz", CH_P2, 200, "@15 v6 o6 g16 e16 c16");
    sfx_define("eko_flash", CH_NOISE, 160, "@12 v12 o6 c4");
    sfx_define("eko_spotted", CH_P1, 200, "@15 v13 o6 c16 o7 c8");
    sfx_define("eko_alarm", CH_P1, 160, "@1 v12 o6 c8 o5 a8 o6 c8 o5 a8");
    sfx_define("eko_bonus", CH_P1, 220, "@35 v10 o6 g16 o7 c16");
    sfx_define("eko_pickup", CH_P1, 240, "@35 v10 o6 c32 g32");
    sfx_define("eko_select", CH_P2, 240, "@42 v8 o6 g32");
    sfx_define("eko_step", CH_NOISE, 240, "@9 v5 o7 c32");
    sfx_define("eko_whistle", CH_P1, 200, "@14 v10 o6 e16 g16 o7 c8");
    sfx_define("eko_free", CH_P1, 160, "@39 v13 o5 l16 c e g o6 c4");
}
