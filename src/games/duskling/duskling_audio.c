/* DUSKLING - original music (UFO-MML) and sound effects. */
#include "duskling.h"

int DK_MUS_TITLE = -1, DK_MUS_DUSK, DK_MUS_WOOD, DK_MUS_MERE, DK_MUS_STEPS, DK_MUS_WORKS, DK_MUS_CAVES,
    DK_MUS_HEIGHTS, DK_MUS_POCKET, DK_MUS_BOSS, DK_MUS_EGG, DK_MUS_WAKE;

/* "Dusk Bell" - the title, D minor, 4/4 */
static const char TITLE_BELL[] =
    "@15 v11 q7"
    "| o5 d4 o4 a4 f4 a4 | g4 b-4 a2 | f4 a4 o5 d4 c4 | o4 a2. r4"
    "| b-4 a4 g4 f4 | e4 f4 g2 | a4 g4 f4 e4 | d2. r4";
static const char TITLE_PAD[] = "@4 v5 q8 o4 f1 g1 f1 e1 d1 e1 c+1 d1";
static const char TITLE_BASS[] = "@6 v13 q6 o2 d2 a2 g2 d2 d2 a2 a2 e2 g2 d2 c2 g2 a2 e2 d2 a2";
static const char TITLE_TICK[] = "[@9 v3 o8 c4 c4 @21 v2 o7 c4 @9 v3 o8 c4]8";

/* "Last Light" - the dayling's walk, C major, slow */
#define ARP(a, b, c) "o4 " a "8 " b "8 " c "8 o5 " a "8 o4 " c "8 " b "8 " a "8 " b "8 "
static const char DUSK_LEAD[] =
    "@5 v10 q7"
    "| o5 e2 g4 e4 | d2. r4 | c2 e4 g4 | a2. r4"
    "| g2 e4 c4 | d2 e4 d4 | c1 | r1";
static const char DUSK_ARP[] = "@2 v4 q5 " ARP("c", "e", "g") ARP("g", "b", "d") ARP("a", "c", "e") ARP("f", "a", "c")
    ARP("c", "e", "g") ARP("g", "b", "d") ARP("c", "e", "g") ARP("c", "e", "g");
static const char DUSK_BASS[] = "@6 v12 q7 o2 c1 g1 a1 f1 c1 g1 c1 c1";
static const char DUSK_AIR[] = "[@21 v2 o7 c2 @9 v2 o8 c2]8";

/* "Hush Wood" - the forest, A minor, a quirky walk */
#define STAB(ins, n) "r8 " ins " " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define WALK(lo, hi) lo "4 " hi "4 " lo "4 " hi "4 "
static const char WOOD_LEAD[] =
    "@1 v11 q6"
    "| o4 a8 r8 o5 c8 e8 d4 c4 | o4 b8 r8 g8 b8 a2 | o4 a8 r8 o5 c8 e8 g4 e4 | d8 e8 d8 c8 o4 b2"
    "| o4 a8 r8 o5 c8 e8 d4 c4 | o4 b8 r8 g8 b8 a2 | o5 c8 d8 e8 f8 g4 e4 | d8 c8 o4 b8 g+8 a2"
    "| o5 e4 e8 f8 g4 f8 e8 | d4 d8 e8 f4 e8 d8 | c4 c8 d8 e4 d8 c8 | o4 b4 g4 b4 o5 d4"
    "| o5 e4 e8 f8 g4 a8 g8 | f4 e8 d8 c4 o4 b8 a8 | g+8 a8 b8 o5 c8 d4 e4 | o4 a2 r2";
static const char WOOD_STAB[] =
    "v5 q3 o4 "
    STAB("@17", "a") STAB("@16", "g") STAB("@17", "a") STAB("@16", "g")
    STAB("@17", "a") STAB("@16", "g") STAB("@16", "c") STAB("@16", "e")
    STAB("@16", "c") STAB("@17", "d") STAB("@17", "a") STAB("@16", "e")
    STAB("@16", "c") STAB("@16", "f") STAB("@16", "e") STAB("@17", "a");
static const char WOOD_BASS[] =
    "@6 v14 q5 "
    WALK("o2 a", "o3 e") WALK("o2 g", "o3 d") WALK("o2 a", "o3 e") WALK("o2 g", "o3 d")
    WALK("o2 a", "o3 e") WALK("o2 g", "o3 d") WALK("o3 c", "o3 g") WALK("o2 e", "o2 b")
    WALK("o3 c", "o3 g") WALK("o2 d", "o2 a") WALK("o2 a", "o3 e") WALK("o2 e", "o2 b")
    WALK("o3 c", "o3 g") WALK("o2 f", "o3 c") WALK("o2 e", "o2 b") WALK("o2 a", "o3 e");
static const char WOOD_DRUM[] = "[@13 v11 o2 c8 @9 v4 o8 c8 @11 v7 o6 c8 @9 v4 o8 c8 @13 v9 o2 c8 @13 v7 o2 c8 @11 v7 o6 c8 @9 v4 o8 c8]16";

/* "Sunken Mere" - under the water, E minor, 3/4 */
#define ARP3(a, b, c) "o4 " a "8 " b "8 " c "8 o5 " a "8 o4 " c "8 " b "8 "
static const char MERE_LEAD[] =
    "@14 v10 q7"
    "| o5 e2 g4 | f+2 d4 | e2 b4 | o4 b2. | o5 c2 e4 | d2 o4 b4 | a2 f+4 | g2."
    "| o5 e2 g4 | a2 b4 | o6 c2 o5 b4 | a2. | g4 f+4 e4 | d4 e4 f+4 | e2. | r2.";
static const char MERE_ARP[] =
    "@2 v4 q5 " ARP3("e", "g", "b") ARP3("d", "f+", "a") ARP3("e", "g", "b") ARP3("b", "d+", "f+")
    ARP3("c", "e", "g") ARP3("g", "b", "d") ARP3("d", "f+", "a") ARP3("g", "b", "d")
    ARP3("e", "g", "b") ARP3("a", "c", "e") ARP3("c", "e", "g") ARP3("b", "d+", "f+")
    ARP3("e", "g", "b") ARP3("d", "f+", "a") ARP3("e", "g", "b") ARP3("e", "g", "b");
static const char MERE_BASS[] = "@6 v12 q7 o2 e2. d2. e2. b2. c2. g2. d2. g2. e2. a2. c2. b2. e2. d2. e2. e2.";
static const char MERE_DRIP[] = "[@9 v2 o8 c4 r4 @21 v3 o7 c4]16";

/* "Old Steps" - the ruins, D minor, a slow procession */
static const char STEPS_LEAD[] =
    "@23 v11 q7"
    "| o4 d4 f4 g4 a4 | o5 c4 o4 a4 g2 | f4 g4 a4 o5 c4 | d2. r4"
    "| o5 d4 c4 o4 a4 g4 | f4 g4 a2 | g4 f4 e4 c4 | d2. r4";
static const char STEPS_PAD[] = "@4 v5 q8 o3 a1 o4 c1 o3 a1 a1 b-1 a1 g1 a1";
static const char STEPS_BASS[] = "@6 v13 q6 o2 d2 a2 f2 o3 c2 o2 d2 a2 d2 a2 b-2 f2 f2 o3 c2 o2 c2 g2 d2 a2";
static const char STEPS_DRUM[] = "[@11 v6 o6 c4 @9 v3 o8 c8 c8 @13 v8 o2 c4 @9 v3 o8 c4]8";

/* "Humming Works" - the machines, C minor, fast */
#define PULSE(lo, hi) "[" lo "8 " hi "8]4 "
static const char WORKS_LEAD[] =
    "@3 v10 q5"
    "| o5 c8 c8 e-8 c8 g8 c8 f8 e-8 | d8 d8 f8 d8 a-8 d8 g8 f8 | e-8 e-8 g8 e-8 o6 c8 o5 e-8 b-8 a-8 | g4 f4 e-4 d4"
    "| o5 c8 c8 e-8 c8 g8 c8 f8 e-8 | d8 d8 f8 d8 a-8 d8 g8 f8 | e-8 f8 g8 a-8 b-8 o6 c8 d8 e-8 | c2 o5 g2";
static const char WORKS_COMP[] =
    "v5 q3 [@17 o4 c8 r8]4 [@16 o3 b-8 r8]4 [@16 o3 a-8 r8]4 [@16 o3 g8 r8]4"
    " [@17 o4 c8 r8]4 [@16 o3 b-8 r8]4 [@16 o3 a-8 r8]4 [@16 o3 g8 r8]4";
static const char WORKS_BASS[] =
    "@6 v14 q4 " PULSE("o2 c", "o3 c") PULSE("o2 b-", "o3 b-") PULSE("o2 a-", "o3 a-") PULSE("o2 g", "o3 g")
    PULSE("o2 c", "o3 c") PULSE("o2 b-", "o3 b-") PULSE("o2 a-", "o3 a-") PULSE("o2 g", "o3 g");
static const char WORKS_DRUM[] = "[@13 v12 o2 c8 @9 v5 o8 c16 c16 @11 v10 o6 c8 @21 v4 o7 c8]16";

/* "Ember Caves" - the amber way, A phrygian */
static const char CAVES_LEAD[] =
    "@5 v11 q7"
    "| o4 a4 b-4 a4 g4 | f4 e4 f2 | a4 b-4 o5 c4 d4 | e2. r4"
    "| o5 f4 e4 d4 c4 | o4 b-4 a4 g2 | f4 g4 a4 b-4 | a2. r4";
static const char CAVES_PAD[] = "@4 v5 q8 o3 e1 f1 e1 e1 f1 d1 d1 e1";
static const char CAVES_BASS[] = "@6 v14 q6 o2 a2 a2 b-2 b-2 a2 a2 a2 e2 d2 d2 b-2 g2 f2 g2 a2 a2";
static const char CAVES_DRUM[] = "[@13 v11 o2 c4 @11 v5 o6 c4 @13 v9 o2 c8 c8 @11 v6 o6 c4]8";

/* "Windy Heights" - the rose way, G major, bright */
static const char HEIGHTS_LEAD[] =
    "@1 v11 q6"
    "| o5 d4 g4 b4 a8 g8 | a4 d4 d2 | e4 g4 o6 c4 o5 b8 a8 | b2. r4"
    "| o6 c4 o5 b4 a4 g4 | a4 b4 o6 d2 | o5 e4 f+4 g4 a4 | g2. r4";
static const char HEIGHTS_ARP[] = "@2 v4 q5 " ARP("g", "b", "d") ARP("d", "f+", "a") ARP("c", "e", "g") ARP("g", "b", "d")
    ARP("c", "e", "g") ARP("d", "f+", "a") ARP("a", "c", "e") ARP("g", "b", "d");
static const char HEIGHTS_BASS[] = "@6 v13 q6 o2 g2 o3 d2 o2 d2 a2 o3 c2 g2 o2 g2 o3 d2 o3 c2 g2 o2 d2 a2 o2 a2 d2 g2 o3 d2";
static const char HEIGHTS_WIND[] = "[@10 v3 o7 c2 @21 v2 o6 c4 c4]8";

/* "Hidden Places" - the warp pockets, whole tones */
static const char POCKET_BELL[] =
    "@15 v10 q7"
    "| o5 c4 d4 e4 f+4 | g+2 f+2 | e4 d4 c4 o4 b-4 | o5 c1"
    "| o5 e4 f+4 g+4 b-4 | o6 c2 o5 b-2 | g+4 f+4 e4 d4 | c1";
static const char POCKET_PAD[] = "@4 v5 q8 o4 e1 d1 c1 e1 g+1 f+1 e1 c1";
static const char POCKET_BASS[] = "@6 v11 q7 o3 c1 o2 b-1 g+1 o3 c1 o2 e1 f+1 g+1 o3 c1";
static const char POCKET_TICK[] = "[@21 v2 o7 c8 r8 r4 r2]8";

/* "The Warden" - every boss, E minor, driving */
#define EIGHT(n) "[" n "8]8 "
static const char BOSS_LEAD[] =
    "@1 v11 q6"
    "| o5 e8 e8 b8 e8 o6 c8 o5 e8 b8 a8 | g8 g8 o6 d8 o5 g8 o6 e8 o5 g8 o6 d8 c8"
    "| o5 b8 b8 o6 f+8 o5 b8 o6 g8 o5 b8 o6 f+8 e8 | d+4 f+4 o5 b2"
    "| o5 e8 e8 b8 e8 o6 c8 o5 e8 b8 a8 | g8 a8 b8 o6 c8 d8 e8 f+8 g8"
    "| f+8 e8 d+8 e8 f+4 d+4 | e2 r2";
static const char BOSS_STAB[] =
    "v6 q3 o4 " STAB("@17", "e") STAB("@16", "g") STAB("@16", "b") STAB("@16", "b")
    STAB("@17", "e") STAB("@16", "c") STAB("@16", "b") STAB("@17", "e");
static const char BOSS_BASS[] = "@6 v14 q5 " EIGHT("o2 e") EIGHT("o2 g") EIGHT("o2 b") EIGHT("o2 b") EIGHT("o2 e") EIGHT("o3 c") EIGHT("o2 b") EIGHT("o2 e");
static const char BOSS_DRUM[] = "[@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8]16";

/* "The Egg" - the ending, C major */
static const char EGG_LEAD[] =
    "@14 v10 q7"
    "| o5 e4 d4 c4 d4 | e4 g4 e2 | d4 c4 o4 a4 o5 c4 | d2. r4"
    "| o5 e4 d4 c4 d4 | e4 g4 o6 c2 | o5 b4 a4 g4 d4 | c2. r4";
static const char EGG_ARP[] = "@2 v4 q5 " ARP("c", "e", "g") ARP("e", "g", "b") ARP("a", "c", "e") ARP("g", "b", "d")
    ARP("c", "e", "g") ARP("a", "c", "e") ARP("g", "b", "d") ARP("c", "e", "g");
static const char EGG_BASS[] = "@6 v12 q7 o2 c1 e1 f1 g1 c1 a1 g1 c1";
static const char EGG_TICK[] = "[@9 v2 o8 c4 c4 c4 c4]8";

/* the waking: a short rising jingle */
static const char WAKE_P1[] = "@15 v10 o5 l8 c e g b o6 c4 e4 g2";
static const char WAKE_P2[] = "@4 v6 o4 l2 c e g1";
static const char WAKE_TRI[] = "@6 v12 o2 c1 c1";

void dk_audio_load(void) {
    if (DK_MUS_TITLE >= 0) return;
    DK_MUS_TITLE = song_define("dk_title", 92, true, TITLE_BELL, TITLE_PAD, TITLE_BASS, TITLE_TICK);
    DK_MUS_DUSK = song_define("dk_dusk", 80, true, DUSK_LEAD, DUSK_ARP, DUSK_BASS, DUSK_AIR);
    DK_MUS_WOOD = song_define("dk_wood", 120, true, WOOD_LEAD, WOOD_STAB, WOOD_BASS, WOOD_DRUM);
    DK_MUS_MERE = song_define("dk_mere", 84, true, MERE_LEAD, MERE_ARP, MERE_BASS, MERE_DRIP);
    DK_MUS_STEPS = song_define("dk_steps", 100, true, STEPS_LEAD, STEPS_PAD, STEPS_BASS, STEPS_DRUM);
    DK_MUS_WORKS = song_define("dk_works", 138, true, WORKS_LEAD, WORKS_COMP, WORKS_BASS, WORKS_DRUM);
    DK_MUS_CAVES = song_define("dk_caves", 110, true, CAVES_LEAD, CAVES_PAD, CAVES_BASS, CAVES_DRUM);
    DK_MUS_HEIGHTS = song_define("dk_heights", 132, true, HEIGHTS_LEAD, HEIGHTS_ARP, HEIGHTS_BASS, HEIGHTS_WIND);
    DK_MUS_POCKET = song_define("dk_pocket", 76, true, POCKET_BELL, POCKET_PAD, POCKET_BASS, POCKET_TICK);
    DK_MUS_BOSS = song_define("dk_boss", 156, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    DK_MUS_EGG = song_define("dk_egg", 88, true, EGG_LEAD, EGG_ARP, EGG_BASS, EGG_TICK);
    DK_MUS_WAKE = song_define("dk_wake", 100, false, WAKE_P1, WAKE_P2, WAKE_TRI, "");

    sfx_define("dk_hop", CH_P2, 240, "@32 v7 o5 g16");
    sfx_define("dk_jump", CH_P2, 240, "@32 v9 o5 c16");
    sfx_define("dk_land", CH_NOISE, 240, "@13 v5 o3 c16");
    sfx_define("dk_slide", CH_NOISE, 220, "@36 v8 o6 c8");
    sfx_define("dk_sprint", CH_P2, 240, "@20 v6 o6 c32 e32");
    sfx_define("dk_spin", CH_P2, 240, "@39 v8 o6 c16 g16");
    sfx_define("dk_slam", CH_P2, 240, "@33 v9 o6 c16");
    sfx_define("dk_pound", CH_NOISE, 220, "@13 v12 o2 c8");
    sfx_define("dk_thump", CH_TRI, 220, "@41 v12 o3 c8");
    sfx_define("dk_eye", CH_P1, 220, "@20 v9 o6 e32 c32 e32 c32");
    sfx_define("dk_clank", CH_NOISE, 220, "@40 v10 o5 c16");
    sfx_define("dk_reveal", CH_P1, 200, "@39 v9 o5 l32 c e g o6 c e");
    sfx_define("dk_spring", CH_P2, 240, "@32 v10 o4 c16 o5 c16");
    sfx_define("dk_boing", CH_P2, 240, "@38 v11 o4 c16");
    sfx_define("dk_knock", CH_NOISE, 220, "@21 v10 o5 c16 @13 v8 o3 c16");
    sfx_define("dk_stomp", CH_NOISE, 220, "@34 v11 o4 c16");
    sfx_define("dk_die", CH_P1, 200, "@33 v11 o5 e16 c16 o4 g16 e8");
    sfx_define("dk_swim", CH_P2, 240, "@36 v5 o6 c16");
    sfx_define("dk_splash", CH_NOISE, 200, "@36 v10 o5 c16 @10 v6 o7 c8");
    sfx_define("dk_spear", CH_P2, 240, "@36 v8 o7 c16");
    sfx_define("dk_fire", CH_NOISE, 200, "@36 v10 o6 c8");
    sfx_define("dk_zap", CH_P2, 240, "@20 v10 o7 c32 o6 g32 e32 c32");
    sfx_define("dk_fade", CH_P2, 200, "@5 v6 o5 g16 e16 c16");
    sfx_define("dk_blast", CH_NOISE, 160, "@34 v12 o3 c8");
    sfx_define("dk_bosshit", CH_NOISE, 160, "@34 v15 o4 c8 @40 v12 o5 c8");
    sfx_define("dk_bossdown", CH_P1, 160, "@39 v12 o5 l16 c e g o6 c e g o7 c4");
    sfx_define("dk_door", CH_P2, 220, "@35 v6 o5 g32");
    sfx_define("dk_warp", CH_P1, 180, "@39 v11 o5 l32 c d e f+ g+ b- o6 c d e f+ g+ b- o7 c8");
}
