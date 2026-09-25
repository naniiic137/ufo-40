/* PETAL PARADE - original music (UFO-MML) and sound effects. */
#include "petalparade.h"

int PP_MUS_GARDEN = -1, PP_MUS_RUSH, PP_MUS_TITLE, PP_MUS_OVER, PP_MUS_END;

/* "Garden Waltz" - F major, 3/4.
 * A: F F C7 C7 C7 C7 F F | A': F F7 Bb Bbm F C7 F F | B: Dm Dm Gm Gm C7 C7 F C7 */
static const char WALTZ_LEAD[] =
    "@14 v11 q7"
    "| o5 c4 f4 a4 | o6 c2 o5 a4 | o5 g4 e4 c4 | o5 b-2 g4"
    "| o5 e4 g4 b-4 | o6 d2 c4 | o5 a4 f4 c4 | o5 f2."
    "| o5 c4 f4 a4 | o6 c4 e-4 c4 | o6 d2 o5 b-4 | o6 d-2 o5 b-4"
    "| o5 a4 o6 c4 o5 a4 | o5 g4 b-4 e4 | o5 f2. | o5 f2 r4"
    "| o5 a4 o6 d4 f4 | o6 e2 d4 | o5 b-4 o6 d4 g4 | o6 f2 d4"
    "| o6 c4 e4 g4 | o6 b-2 a4 | o6 a4 f4 c4 | o5 b-4 g4 e4";
#define PAH(ins, n) "r4 " ins " " n "4 " n "4 "
static const char WALTZ_PAH[] =
    "v6 q3 o4 "
    PAH("@16", "f") PAH("@16", "f") PAH("@16", "c") PAH("@16", "c")
    PAH("@16", "c") PAH("@16", "c") PAH("@16", "f") PAH("@16", "f")
    PAH("@16", "f") PAH("@16", "f") PAH("@16", "b-") PAH("@17", "b-")
    PAH("@16", "f") PAH("@16", "c") PAH("@16", "f") PAH("@16", "f")
    PAH("@17", "d") PAH("@17", "d") PAH("@17", "g") PAH("@17", "g")
    PAH("@16", "c") PAH("@16", "c") PAH("@16", "f") PAH("@16", "c");
static const char WALTZ_BASS[] =
    "@6 v14 q6"
    "| o3 f4 r2 | o3 c4 r2 | o3 c4 r2 | o2 g4 r2 | o3 c4 r2 | o2 g4 r2 | o3 f4 r2 | o3 c4 r2"
    "| o3 f4 r2 | o3 e-4 r2 | o2 b-4 r2 | o3 d-4 r2 | o3 c4 r2 | o3 c4 r2 | o3 f4 r2 | o3 c4 r2"
    "| o3 d4 r2 | o2 a4 r2 | o2 g4 r2 | o3 d4 r2 | o3 c4 r2 | o2 g4 r2 | o3 f4 r2 | o3 c4 r2";
static const char WALTZ_BRUSH[] = "[@9 v5 o8 c4 v3 c4 c4]24";

/* "Nectar Rush" - the same garden at a gallop. C major, 2 x 8 bars. */
static const char RUSH_LEAD[] =
    "@1 v11 q6"
    "| o5 c8 e8 g8 e8 c8 e8 g8 o6 c8 | o6 e4 d8 c8 o5 g4 e4"
    "| o5 d8 f8 b8 f8 d8 f8 b8 o6 d8 | o6 f4 e8 d8 o5 b4 g4"
    "| o5 c8 e8 g8 e8 c8 e8 g8 o6 c8 | o6 e8 f8 g4 e4 c4"
    "| o6 d8 c8 o5 b8 a8 g4 b4 | o6 c4 o5 g4 o6 c2"
    "| o5 a8 o6 c8 f8 c8 o5 a8 o6 c8 f8 a8 | o6 g4 e8 c8 o5 g4 o6 c4"
    "| o5 b8 o6 d8 g8 d8 o5 b8 o6 d8 f8 d8 | o6 e4 c4 o5 g4 o6 c4"
    "| o6 f8 e8 f8 a8 g8 f8 e8 d8 | o6 e8 d8 e8 g8 f8 e8 d8 c8"
    "| o5 b8 o6 c8 d8 e8 f8 d8 o5 b8 g8 | o5 g8 a8 b8 o6 c8 d4 o5 g4";
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
static const char RUSH_OFF[] =
    "v6 q3 o4 "
    OFF("@16", "c") OFF("@16", "c") OFF("@16", "g") OFF("@16", "g")
    OFF("@16", "c") OFF("@16", "c") OFF("@16", "g") OFF("@16", "c")
    OFF("@16", "f") OFF("@16", "c") OFF("@16", "g") OFF("@16", "c")
    OFF("@16", "f") OFF("@16", "c") OFF("@16", "g") OFF("@16", "g");
#define OOM_C "o3 c4 o2 g4 o3 c4 o2 g4 "
#define OOM_G "o2 g4 d4 g4 d4 "
#define OOM_F "o2 f4 o3 c4 o2 f4 o3 c4 "
static const char RUSH_BASS[] =
    "@6 v14 q5 "
    OOM_C OOM_C OOM_G OOM_G OOM_C OOM_C OOM_G OOM_C
    OOM_F OOM_C OOM_G OOM_C OOM_F OOM_C OOM_G OOM_G;
static const char RUSH_DRUMS[] =
    "[@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 @13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8]16";

/* "Petal Lullaby" - a music box over the title. F major, 3/4. */
static const char LULL_BELL[] =
    "@15 v10 q7"
    "| o5 a4 o6 c4 o5 f4 | o5 b-4 o6 d4 o5 b-4 | o5 a2 g4 | o5 e2."
    "| o5 a4 o6 c4 o5 f4 | o6 d4 f4 d4 | o6 c4 o5 b-4 g4 | o5 f2.";
static const char LULL_ARP[] =
    "@2 v5 q5"
    "| o4 f8 a8 o5 c8 f8 c8 o4 a8 | o4 b-8 o5 d8 f8 b-8 f8 d8 | o4 f8 a8 o5 c8 f8 c8 o4 a8 | o4 c8 e8 g8 b-8 g8 e8"
    "| o4 f8 a8 o5 c8 f8 c8 o4 a8 | o4 b-8 o5 d8 f8 b-8 f8 d8 | o4 c8 e8 g8 b-8 g8 e8 | o4 f8 a8 o5 c8 f8 c8 o4 a8";
static const char LULL_BASS[] =
    "@6 v12 q7 | o3 f2. | o2 b-2. | o3 f2. | o3 c2. | o3 f2. | o2 b-2. | o3 c2. | o3 f2.";

/* "Two Hundred Home" - the ending, brass and strums. F major, 3/4. */
static const char END_LEAD[] =
    "@23 v12 q7"
    "| o5 f4 a4 o6 c4 | o6 f2 e8 d8 | o6 d4 c4 o5 b-4 | o6 d2."
    "| o6 c4 o5 a4 f4 | o5 g4 b-4 o6 e4 | o6 f4 c4 o5 a4 | o5 f2.";
static const char END_STRUM[] =
    "v7 q4 o4 "
    PAH("@16", "f") PAH("@16", "f") PAH("@16", "b-") PAH("@16", "b-")
    PAH("@16", "f") PAH("@16", "c") PAH("@16", "f") PAH("@16", "f");
static const char END_BASS[] =
    "@6 v14 q6"
    "| o3 f4 r4 c4 | o3 f4 r4 c4 | o2 b-4 r4 o3 f4 | o2 b-4 r4 o3 f4"
    "| o3 f4 r4 c4 | o3 c4 r4 o2 g4 | o3 f4 r4 c4 | o3 f4 r4 c4";
static const char END_DRUMS[] = "[@13 v10 o2 c4 @9 v4 o8 c4 c4]8";

/* game over */
static const char OVER_P1[] = "@5 v11 o5 l8 a f c o4 a4. g8 f2";
static const char OVER_TRI[] = "@6 v13 o3 l4 f c o2 f2";

void pp_audio_load(void) {
    if (PP_MUS_GARDEN >= 0) return;
    PP_MUS_GARDEN = song_define("pp_garden", 168, true, WALTZ_LEAD, WALTZ_PAH, WALTZ_BASS, WALTZ_BRUSH);
    PP_MUS_RUSH = song_define("pp_rush", 152, true, RUSH_LEAD, RUSH_OFF, RUSH_BASS, RUSH_DRUMS);
    PP_MUS_TITLE = song_define("pp_title", 100, true, LULL_BELL, LULL_ARP, LULL_BASS, "");
    PP_MUS_END = song_define("pp_end", 132, true, END_LEAD, END_STRUM, END_BASS, END_DRUMS);
    PP_MUS_OVER = song_define("pp_over", 120, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("pp_hop", CH_P2, 240, "@32 v10 o5 c16");
    sfx_define("pp_pup", CH_P2, 240, "@35 v10 o6 c16 g16");
    sfx_define("pp_save", CH_P1, 200, "@39 v12 o5 l16 c e g >c e g >c8");
    sfx_define("pp_sour", CH_P2, 200, "@33 v11 o4 e16 c16 <g8");
    sfx_define("pp_bump", CH_NOISE, 160, "@34 v14 o4 c8");
    sfx_define("pp_drink", CH_P2, 220, "@38 v12 o4 c16 e16 g16 >c8");
    sfx_define("pp_smash", CH_NOISE, 220, "@36 v11 o6 c16 @13 v12 o3 c16");
    sfx_define("pp_jar", CH_P2, 220, "@15 v10 o6 e16 g16 >c16");
    sfx_define("pp_ripen", CH_P2, 240, "@20 v8 o7 c32 e32");
    sfx_define("pp_cackle", CH_P1, 200, "@3 v11 o6 l32 a r f r a r f r d8");
}
