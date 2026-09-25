/* TIN TROOP - original music (UFO-MML) and sound effects: marches for a toy army. */
#include "tintroop.h"

int TT_MUS_WORLD[4] = {-1, -1, -1, -1};
int TT_MUS_BOSS, TT_MUS_TITLE, TT_MUS_MAP, TT_MUS_CLEAR, TT_MUS_FAIL, TT_MUS_END;

/* a snare-drum bar in sixteenths: ta-ta-TUM, boom, TUM, ta-ta-TUM */
#define MARCH "@11 v10 o6 c16 v7 c16 v10 c8 @13 v11 o2 c8 @9 v4 o8 c8 @11 v10 o6 c8 @9 v4 o8 c8 @11 v8 o6 c16 v6 c16 v10 c8 "
#define STAB4(ins, n) "r4 " ins " " n "4 r4 " n "4 "
#define OOM4(r, f) r "4 r4 " f "4 r4 "

/* "Nursery March" - C major, 4/4 */
static const char W1_LEAD[] =
    "@1 v11 q6"
    "| o5 c8. c16 e8. e16 g4 e4 | o5 f8. f16 a8. a16 g2 | o5 e8. e16 g8. g16 o6 c4 o5 g4 | o5 f8 e8 d8 e8 c2"
    "| o5 c8. c16 e8. e16 g4 e4 | o5 f8. f16 a8. a16 g2 | o5 a8 g8 f8 e8 d4 g4 | o5 c2 r2"
    "| o5 e4 e8 f8 g4 e4 | o5 d4 d8 e8 f4 d4 | o5 c4 c8 d8 e4 g4 | o5 a8 b8 o6 c8 d8 o5 b4 g4"
    "| o5 e4 e8 f8 g4 e4 | o5 d4 d8 e8 f4 d4 | o5 e8 g8 o6 c8 e8 d4 o5 b4 | o6 c2 r2";
static const char W1_STAB[] =
    "v6 q3 o4 "
    STAB4("@16", "c") STAB4("@16", "f") STAB4("@16", "c") STAB4("@16", "g")
    STAB4("@16", "c") STAB4("@16", "f") STAB4("@16", "g") STAB4("@16", "c")
    STAB4("@16", "c") STAB4("@16", "g") STAB4("@16", "c") STAB4("@16", "g")
    STAB4("@16", "c") STAB4("@16", "g") STAB4("@16", "g") STAB4("@16", "c");
static const char W1_BASS[] =
    "@6 v14 q5 "
    OOM4("o3 c", "o2 g") OOM4("o2 f", "o3 c") OOM4("o3 c", "o2 g") OOM4("o2 g", "o2 d")
    OOM4("o3 c", "o2 g") OOM4("o2 f", "o3 c") OOM4("o2 g", "o2 d") OOM4("o3 c", "o2 g")
    OOM4("o3 c", "o2 g") OOM4("o2 g", "o2 d") OOM4("o3 c", "o2 g") OOM4("o2 g", "o2 d")
    OOM4("o3 c", "o2 g") OOM4("o2 g", "o2 d") OOM4("o2 g", "o2 d") OOM4("o3 c", "o2 g");
static const char W1_DRUM[] = "[" MARCH "]16";

/* "Bathwater" - D minor, a slow 3/4 */
#define DM "o4 d8 f8 a8 o5 d8 o4 a8 f8 "
#define GM "o4 g8 b-8 o5 d8 g8 d8 o4 b-8 "
#define FM "o4 f8 a8 o5 c8 f8 c8 o4 a8 "
#define AM "o4 a8 o5 c+8 e8 a8 e8 c+8 "
#define BB "o4 b-8 o5 d8 f8 b-8 f8 d8 "
#define CM "o4 c8 e8 g8 o5 c8 o4 g8 e8 "
static const char W2_LEAD[] =
    "@5 v11 q7"
    "| o5 d4 f4 a4 | o6 d2 c4 | o5 b-4 a4 g4 | o5 a2."
    "| o5 g4 b-4 o6 d4 | o6 c2 o5 a4 | o5 f4 e4 d4 | o5 e2."
    "| o5 d4 f4 a4 | o6 d2 e4 | o6 f4 e4 d4 | o6 c+2."
    "| o6 d4 c4 o5 b-4 | o5 a4 g4 e4 | o5 f4 e4 c+4 | o5 d2.";
static const char W2_ARP[] = "@2 v5 q5 " DM DM GM DM GM FM DM AM DM BB DM AM BB CM AM DM;
static const char W2_BASS[] =
    "@6 v13 q7 o3 d2. d2. o2 g2. o3 d2. o2 g2. f2. o3 d2. o2 a2. o3 d2. o2 b-2. o3 d2. o2 a2. b-2. o3 c2. o2 a2. o3 d2.";
static const char W2_DRIP[] = "[@9 v3 o8 c4 r4 @21 v4 o7 c4]16";

/* "Kitchen Galop" - G major, 2/4 */
#define AFT(n) "r8 " n "8 r8 " n "8 "
static const char W3_LEAD[] =
    "@1 v11 q5"
    "| o5 g8 b8 o6 d8 o5 b8 | o6 c8 o5 a8 f+8 d8 | o5 g8 b8 o6 d8 g8 | o6 f+4 d4"
    "| o6 e8 c8 o5 a8 o6 c8 | o6 d8 o5 b8 g8 b8 | o6 c8 o5 a8 f+8 a8 | o5 g4 r4"
    "| o6 d16 e16 d16 c16 o5 b8 g8 | o6 c16 d16 c16 o5 b16 a8 f+8 | o5 g8 a8 b8 o6 c8 | o6 d4 o5 d4"
    "| o5 g8 b8 o6 d8 g8 | o6 f+8 e8 d8 c8 | o5 b8 a8 g8 f+8 | o5 g4 r4";
static const char W3_AFT[] =
    "@16 v6 q3 o4 "
    AFT("g") AFT("d") AFT("g") AFT("d") AFT("c") AFT("g") AFT("d") AFT("g")
    AFT("g") AFT("d") AFT("g") AFT("d") AFT("g") AFT("d") AFT("d") AFT("g");
#define GAL(r, f) r "8 r8 " f "8 r8 "
static const char W3_BASS[] =
    "@6 v14 q5 "
    GAL("o2 g", "o3 d") GAL("o2 d", "o2 a") GAL("o2 g", "o3 d") GAL("o2 d", "o2 a")
    GAL("o3 c", "o2 g") GAL("o2 g", "o3 d") GAL("o2 d", "o2 a") GAL("o2 g", "o3 d")
    GAL("o2 g", "o3 d") GAL("o2 d", "o2 a") GAL("o2 g", "o3 d") GAL("o2 d", "o2 a")
    GAL("o2 g", "o3 d") GAL("o2 d", "o2 a") GAL("o2 d", "o2 a") GAL("o2 g", "o3 d");
static const char W3_DRUM[] = "[@13 v11 o2 c8 @11 v8 o6 c8 @13 v10 o2 c8 @11 v8 o6 c16 c16]16";

/* "The Toy Chest" - A minor music box, 4/4 */
static const char W4_BELL[] =
    "@15 v10 q7"
    "| o5 a4 o6 c4 e4 c4 | o5 b4 g+4 e2 | o5 a4 o6 c4 e4 a4 | o6 g+2. r4"
    "| o6 f4 e4 d4 c4 | o5 b4 o6 c4 d2 | o6 c4 o5 b4 a4 g+4 | o5 a2. r4";
static const char W4_PAD[] = "@4 v5 q8 o4 e1 e1 e1 e1 f1 g1 e1 e1";
static const char W4_BASS[] = "@6 v12 q7 o2 a1 e1 a1 e1 f1 g1 e1 a1";
static const char W4_TICK[] = "[@21 v3 o7 c4 @21 v2 o6 c4]16";

/* "Jack of the Chest" - E minor, driving */
#define ST2(n) n "16 r16 " n "16 r16 "
#define BAR16(ins, n) ins " " ST2(n) ST2(n) ST2(n) ST2(n)
static const char BOSS_LEAD[] =
    "@1 v11 q6"
    "| o5 e8 e8 g8 e8 b8 e8 a8 g8 | o5 f+8 f+8 a8 f+8 o6 c8 o5 b8 a8 f+8"
    "| o5 e8 e8 g8 e8 b8 e8 o6 d8 c8 | o5 b4 a8 g8 f+4 d+4"
    "| o5 g8 g8 b8 g8 o6 d8 o5 g8 o6 c8 o5 b8 | o5 a8 a8 o6 c8 o5 a8 o6 e8 o5 a8 o6 d8 c8"
    "| o5 b8 o6 c8 d8 e8 f+8 e8 d8 c8 | o5 b4 d+4 e2";
static const char BOSS_STAB[] =
    "v6 q3 o4 " BAR16("@17", "e") BAR16("@16", "d") BAR16("@17", "e") BAR16("@16", "b")
    BAR16("@16", "g") BAR16("@17", "a") BAR16("@16", "c") BAR16("@16", "b");
#define EI(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
static const char BOSS_BASS[] = "@6 v14 q5 " EI("o2 e") EI("o2 d") EI("o2 e") EI("o2 b") EI("o2 g") EI("o2 a") EI("o3 c") EI("o2 b");
static const char BOSS_DRUM[] = "[@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 @13 v12 o2 c8 @13 v10 o2 c8 @11 v10 o6 c8 @9 v5 o8 c8]8";

/* "Tin Troop Fanfare" - the title, C major */
static const char TITLE_LEAD[] =
    "@23 v12 q7"
    "| o4 g8. g16 o5 c4 e4 g4 | o5 f8. e16 d8. c16 d2 | o4 g8. g16 o5 c4 e4 g4 | o5 a8. g16 f8. e16 g2"
    "| o5 a4 f4 o6 c4 o5 a4 | o5 g4 e4 c2 | o5 d8. d16 e8. f16 g4 b4 | o6 c2 r2";
static const char TITLE_STAB[] =
    "v6 q3 o4 " STAB4("@16", "c") STAB4("@16", "g") STAB4("@16", "c") STAB4("@16", "f")
    STAB4("@16", "f") STAB4("@16", "c") STAB4("@16", "g") STAB4("@16", "c");
static const char TITLE_BASS[] =
    "@6 v14 q5 " OOM4("o3 c", "o2 g") OOM4("o2 g", "o2 d") OOM4("o3 c", "o2 g") OOM4("o2 f", "o3 c")
    OOM4("o2 f", "o3 c") OOM4("o3 c", "o2 g") OOM4("o2 g", "o2 d") OOM4("o3 c", "o2 g");
static const char TITLE_DRUM[] = "[" MARCH "]8";

/* "Barracks" - the map */
static const char MAP_LEAD[] = "@2 v9 q6 | o5 c4 e4 g4 e4 | o5 f4 a4 g2 | o5 e4 g4 o6 c4 o5 g4 | o5 f4 d4 c2";
static const char MAP_BASS[] = "@6 v12 q6 | o3 c1 | o2 f2 g2 | o3 c1 | o2 g2 o3 c2";
static const char MAP_DRUM[] = "[@11 v6 o6 c16 v4 c16 v5 c8 @9 v3 o8 c8 c8 @11 v5 o6 c8 @9 v3 o8 c8 c8 c8]4";

/* "The Troop Comes Home" - the ending, F major */
static const char END_LEAD[] =
    "@23 v12 q7"
    "| o5 c4 f4 a4 f4 | o5 g4 b-4 a2 | o5 f4 a4 o6 c4 o5 a4 | o5 g2. r4"
    "| o5 b-4 g4 o6 d4 o5 b-4 | o5 a4 f4 o6 c2 | o5 g4 a4 b-4 e4 | o5 f2 r2";
static const char END_STAB[] =
    "v6 q3 o4 " STAB4("@16", "f") STAB4("@16", "c") STAB4("@16", "f") STAB4("@16", "c")
    STAB4("@16", "b-") STAB4("@16", "f") STAB4("@16", "c") STAB4("@16", "f");
static const char END_BASS[] =
    "@6 v14 q5 " OOM4("o2 f", "o3 c") OOM4("o3 c", "o2 g") OOM4("o2 f", "o3 c") OOM4("o3 c", "o2 g")
    OOM4("o2 b-", "o3 f") OOM4("o2 f", "o3 c") OOM4("o3 c", "o2 g") OOM4("o2 f", "o3 c");
static const char END_DRUM[] = "[" MARCH "]8";

/* jingles */
static const char CLEAR_P1[] = "@23 v12 o5 l8 c e g o6 c4 o5 g8 o6 c2";
static const char CLEAR_P2[] = "@22 v8 o4 l8 g o5 c e g4 e8 g2";
static const char CLEAR_TRI[] = "@6 v14 o3 l4 c o2 g o3 c2";
static const char CLEAR_NOISE[] = "@11 v9 o6 c16 c16 c8 c8 c8 @12 v8 o6 c2";
static const char FAIL_P1[] = "@5 v11 o5 l8 g f e d4 c8 o4 g2";
static const char FAIL_TRI[] = "@6 v13 o3 l4 c o2 b- a g2";

void tt_audio_load(void) {
    if (TT_MUS_WORLD[0] >= 0) return;
    TT_MUS_WORLD[0] = song_define("tt_nursery", 120, true, W1_LEAD, W1_STAB, W1_BASS, W1_DRUM);
    TT_MUS_WORLD[1] = song_define("tt_bath", 96, true, W2_LEAD, W2_ARP, W2_BASS, W2_DRIP);
    TT_MUS_WORLD[2] = song_define("tt_kitchen", 168, true, W3_LEAD, W3_AFT, W3_BASS, W3_DRUM);
    TT_MUS_WORLD[3] = song_define("tt_chest", 84, true, W4_BELL, W4_PAD, W4_BASS, W4_TICK);
    TT_MUS_BOSS = song_define("tt_jack", 152, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    TT_MUS_TITLE = song_define("tt_title", 116, true, TITLE_LEAD, TITLE_STAB, TITLE_BASS, TITLE_DRUM);
    TT_MUS_MAP = song_define("tt_map", 100, true, MAP_LEAD, "", MAP_BASS, MAP_DRUM);
    TT_MUS_END = song_define("tt_home", 112, true, END_LEAD, END_STAB, END_BASS, END_DRUM);
    TT_MUS_CLEAR = song_define("tt_clear", 140, false, CLEAR_P1, CLEAR_P2, CLEAR_TRI, CLEAR_NOISE);
    TT_MUS_FAIL = song_define("tt_fail", 100, false, FAIL_P1, "", FAIL_TRI, "");

    sfx_define("tt_deploy", CH_P2, 200, "@15 v8 o6 g16 e16");
    sfx_define("tt_jump", CH_P2, 240, "@32 v9 o5 c16");
    sfx_define("tt_land", CH_NOISE, 240, "@13 v7 o3 c16");
    sfx_define("tt_lance", CH_P2, 240, "@36 v11 o7 c8");
    sfx_define("tt_thunk", CH_NOISE, 200, "@41 v12 o4 c16 @21 v8 o6 c16");
    sfx_define("tt_pop", CH_NOISE, 140, "@34 v15 o3 c4");
    sfx_define("tt_stone", CH_TRI, 200, "@41 v13 o3 c8");
    sfx_define("tt_thud", CH_NOISE, 220, "@13 v12 o2 c8");
    sfx_define("tt_crash", CH_NOISE, 220, "@40 v11 o5 c16");
    sfx_define("tt_die", CH_P1, 200, "@33 v11 o5 e16 c16 o4 g8");
    sfx_define("tt_burnout", CH_NOISE, 160, "@36 v10 o6 c8 @34 v8 o5 c8");
    sfx_define("tt_foe", CH_P2, 220, "@37 v10 o5 c16 o4 g16");
    sfx_define("tt_life", CH_P1, 220, "@39 v11 o5 l16 c e g o6 c");
    sfx_define("tt_splash", CH_NOISE, 200, "@36 v10 o5 c16 @10 v6 o7 c8");
    sfx_define("tt_ignite", CH_NOISE, 200, "@36 v11 o7 c16 c16");
    sfx_define("tt_seed", CH_P2, 220, "@35 v9 o6 c16 d16");
    sfx_define("tt_spit", CH_P2, 240, "@32 v7 o4 g16");
    sfx_define("tt_fire", CH_NOISE, 200, "@36 v9 o6 c8");
    sfx_define("tt_gate", CH_TRI, 200, "@38 v12 o3 c16 g16");
    sfx_define("tt_gate_shut", CH_TRI, 200, "@41 v12 o3 g16 c16");
    sfx_define("tt_snort", CH_NOISE, 220, "@36 v9 o4 c16 c16");
    sfx_define("tt_clang", CH_P2, 220, "@40 v12 o6 c16");
    sfx_define("tt_spawn", CH_P2, 220, "@33 v8 o5 c16");
    sfx_define("tt_jack", CH_P1, 180, "@3 v11 o5 c16 e16 g16 o6 c8");
    sfx_define("tt_orb", CH_P2, 220, "@20 v9 o6 e16 c16");
    sfx_define("tt_bosshit", CH_NOISE, 160, "@34 v15 o4 c8 @40 v12 o5 c8");
    sfx_define("tt_gulp", CH_TRI, 200, "@41 v13 o4 c8 o3 c8");
    sfx_define("tt_door", CH_P2, 200, "@35 v9 o5 g16 o6 d16");
    sfx_define("tt_intro", CH_P1, 180, "@23 v11 o5 l16 g o6 c e g8");
}
