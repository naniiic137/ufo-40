/* FORLORN HOPE - original music (UFO-MML) and sound effects. The run has
 * an intro theme and a tune for each trade, so each volunteer marches out
 * to their own; all of it written for UFO 40. */
#include "forlorn.h"

int FRL_MUS_TITLE = -1, FRL_MUS_CLASS[FRC_COUNT] = {-1, -1, -1, -1, -1}, FRL_MUS_END = -1, FRL_MUS_OVER = -1,
    FRL_MUS_SELECT = -1;

/* "The Forlorn Hope" - A minor, a slow muster, 8 bars */
static const char TITLE_LEAD[] =
    "@14 v11 q7"
    "| o4 a4 b4 o5 c4 e4 | o5 d2 c4 o4 b4 | o4 a4 g4 f4 e4 | o4 e2. r4"
    "| o4 f4 a4 o5 c4 f4 | o5 e2 d4 c4 | o4 b4 g+4 a4 b4 | o4 a2. r4";
static const char TITLE_PAD[] = "@4 v6 q8 o4 e1 f1 c1 o3 b1 o4 c1 c1 o3 b2 o4 e2 o4 e1";
static const char TITLE_BASS[] =
    "@6 v14 q7 o2 a2 e2 d2 a2 f2 c2 e2 o1 b2 o2 f2 c2 a2 e2 e2 e2 a2 a2";
static const char TITLE_DRUM[] = "[@13 v10 o2 c4 @9 v3 o8 c4 @11 v6 o6 c4 @9 v3 o8 c4]8";

/* "Stone and Mortar" (the mason) - D minor, a heavy march */
static const char MASON_LEAD[] =
    "@1 v11 q6"
    "| o4 d4 f4 a4 f4 | o4 g4 e4 c+2 | o4 d4 f4 a4 o5 d4 | o5 c4 o4 a4 f2"
    "| o4 b-4 a4 g4 f4 | o4 e4 g4 a2 | o4 d4 e4 f4 g4 | o4 a2 d2";
static const char MASON_COMP[] =
    "v6 q4 o3"
    "| @17 d4 d4 d4 d4 | @16 a4 a4 a4 a4 | @17 d4 d4 d4 d4 | @16 f4 f4 f4 f4"
    "| @17 g4 g4 g4 g4 | @16 a4 a4 a4 a4 | @17 d4 d4 g4 g4 | @16 a4 a4 @17 d4 d4";
static const char MASON_BASS[] =
    "@6 v14 q5"
    "| o2 d4 a4 d4 a4 | o2 a4 e4 a4 e4 | o2 d4 a4 d4 a4 | o2 f4 c4 f4 c4"
    "| o2 g4 d4 g4 d4 | o2 a4 e4 a4 e4 | o2 d4 a4 g4 d4 | o2 a4 e4 d4 r4";
#define MASON_BAR "@13 v11 o2 c8 @11 v8 o6 c16 c16 @13 v10 o2 c8 @11 v9 o6 c8 @13 v11 o2 c8 @11 v8 o6 c16 c16 @11 v10 o6 c8 @9 v4 o8 c8 "
static const char MASON_DRUM[] = "[" MASON_BAR "]8";

/* "Powder and Shot" (the hunter) - E minor, at a gallop */
static const char HUNTER_LEAD[] =
    "@0 v11 q6"
    "| o4 e12 e12 g12 b4 a12 g12 f+12 e4 | o4 d12 d12 f+12 a4 g12 f+12 e12 d4"
    "| o4 e12 e12 g12 b4 o5 d12 c12 o4 b12 a4 | o4 b4 g4 e2"
    "| o4 c12 c12 e12 g4 f+12 e12 d12 c4 | o4 d12 d12 f+12 a4 b12 a12 g12 f+4"
    "| o4 e12 g12 b12 o5 e4 d12 c12 o4 b12 a4 | o4 b2 e2";
#define GAL(n) "[" n "12 r12 " n "12]4 "
static const char HUNTER_COMP[] =
    "v6 q4 o3 @17 " GAL("e") "@16 " GAL("d") "@17 " GAL("e") "@17 " GAL("e")
    "@16 " GAL("c") "@16 " GAL("d") "@17 " GAL("e") "@16 " GAL("b");
static const char HUNTER_BASS[] =
    "@6 v14 q5 o2 e4 b4 e4 b4 d4 a4 d4 a4 e4 b4 e4 b4 e4 b4 e4 g4"
    " c4 g4 c4 g4 d4 a4 d4 a4 e4 b4 e4 b4 o1 b4 o2 f+4 e2";
static const char HUNTER_DRUM[] = "[@13 v10 o2 c12 @9 v4 o8 c12 @11 v8 o6 c12]32";

/* "Light Feet" (the runner) - G minor, quick */
static const char RUNNER_LEAD[] =
    "@1 v10 q5"
    "| o5 g8 f8 d8 b-8 o4 g8 b-8 o5 d8 g8 | o5 f8 e-8 c8 a8 o4 f8 a8 o5 c8 f8"
    "| o5 e-8 d8 o4 b-8 o5 g8 o4 e-8 g8 b-8 o5 e-8 | o5 d8 f+8 a8 f+8 d4 r4"
    "| o5 g8 a8 b-8 a8 g8 f8 e-8 d8 | o5 e-8 f8 g8 f8 e-8 d8 c8 o4 b-8"
    "| o4 a8 b-8 o5 c8 d8 e-8 f+8 g8 a8 | o5 g4 d4 o4 g2";
#define PL(a, b, c) "[o4 " a "16 " b "16 " c "16 o4 " b "16]4 "
static const char RUNNER_COMP[] =
    "@2 v5 q5 " PL("g", "b-", "o5 d") PL("f", "a", "o5 c") PL("e-", "g", "b-") PL("d", "f+", "a")
    PL("g", "b-", "o5 d") PL("e-", "g", "o5 c") PL("d", "f+", "a") PL("g", "b-", "o5 d");
static const char RUNNER_BASS[] =
    "@6 v14 q4 [o2 g8 o3 g8]4 [o2 f8 o3 f8]4 [o2 e-8 o3 e-8]4 [o2 d8 o3 d8]4"
    " [o2 g8 o3 g8]4 [o2 e-8 o3 e-8]4 [o2 d8 o3 d8]4 [o2 g8 o3 g8]4";
static const char RUNNER_DRUM[] = "[@9 v5 o8 c16 c16 @11 v9 o6 c8 @9 v5 o8 c16 c16 @13 v10 o2 c8]16";

/* "Gears and Grit" (the tinker) - C minor, clockwork */
static const char TINKER_LEAD[] =
    "@3 v11 q5"
    "| o4 c8 e-8 g8 e-8 c8 e-8 g8 o5 c8 | o4 b-8 a-8 g8 f8 e-8 d8 c8 d8"
    "| o4 e-8 g8 o5 c8 o4 g8 e-8 g8 o5 c8 e-8 | o5 d4 o4 b4 g2"
    "| o4 a-8 o5 c8 e-8 c8 o4 a-8 o5 c8 e-8 a-8 | o5 g8 f8 e-8 d8 c8 o4 b8 a-8 g8"
    "| o4 f8 a-8 o5 c8 f8 e-8 d8 c8 o4 b8 | o5 c2 o4 g4 c4";
#define TK(n) "[o5 " n "16 r16 o5 " n "16 r16]4 "
static const char TINKER_COMP[] =
    "@20 v5 q3 " TK("c") TK("c") TK("e-") TK("d") TK("c") TK("c") TK("d") TK("c");
static const char TINKER_BASS[] =
    "@6 v14 q4 [o2 c8 c8 g8 c8]2 [o2 g8 g8 d8 g8]2 [o2 c8 c8 g8 c8]2 [o2 g8 g8 d8 g8]2"
    " [o2 a-8 a-8 e-8 a-8]2 [o2 g8 g8 d8 g8]2 [o2 f8 f8 c8 f8]2 [o2 c8 c8 g8 c8]2";
static const char TINKER_DRUM[] =
    "[@21 v6 o7 c16 @21 v3 o7 c16 @11 v8 o6 c8 @21 v6 o7 c16 @21 v3 o7 c16 @13 v10 o2 c8]16";

/* "Short Fuse" (the sapper) - B minor, tight and ticking */
static const char SAPPER_LEAD[] =
    "@14 v11 q7"
    "| o4 b2 o5 d4 c+4 | o4 b4 a4 f+2 | o4 g4 a4 b4 o5 d4 | o5 c+2. r4"
    "| o4 b2 o5 f+4 e4 | o5 d4 c+4 o4 b2 | o4 a+4 b4 o5 c+4 d4 | o5 c+2 o4 f+2";
static const char SAPPER_COMP[] =
    "@3 v5 q3 [o4 b8]8 [o4 b8]8 [o4 g8]8 [o4 f+8]8 [o4 b8]8 [o4 g8]8 [o4 f+8]8 [o4 f+8]8";
static const char SAPPER_BASS[] =
    "@6 v14 q4 [o2 b8 r8]4 [o2 b8 r8]4 [o2 g8 r8]4 [o2 f+8 r8]4 [o2 b8 r8]4 [o2 g8 r8]4 [o2 f+8 r8]4 [o2 f+8 r8]4";
static const char SAPPER_DRUM[] = "[@21 v5 o8 c16 @21 v3 o8 c16 @21 v4 o8 c16 @21 v3 o8 c16 @13 v9 o2 c8 @21 v3 o8 c16 c16]16";

/* "The Last Name on the Roll" - the run is over */
static const char OVER_LEAD[] = "@14 v11 q7 o4 e4 d4 c4 o3 b4 a1";
static const char OVER_PAD[] = "@4 v6 q8 o3 c1 o2 a1";
static const char OVER_BASS[] = "@6 v12 q8 o2 a1 o1 a1";
static const char OVER_DRUM[] = "@12 v5 o5 c1 r1";

/* "Statues in the Square" - the ending: C major at last */
static const char END_LEAD[] =
    "@15 v11 q7"
    "| o5 c4 e4 g4 e4 | o5 f4 a4 g2 | o5 e4 d4 c4 o4 a4 | o4 g2. r4"
    "| o4 a4 o5 c4 e4 a4 | o5 g4 f4 e4 d4 | o5 c4 d4 e4 g4 | o5 c2. r4";
static const char END_PAD[] = "@4 v6 q8 o4 e1 f1 e1 d1 e1 d1 e1 e1";
static const char END_BASS[] = "@6 v13 q7 o2 c2 g2 f2 c2 a2 e2 g2 d2 a2 e2 f2 g2 c2 g2 c1";
static const char END_DRUM[] = "[@9 v2 o8 c4 @11 v4 o6 c4 @9 v2 o8 c4 @11 v4 o6 c4]8";

void frl_audio_load(void) {
    if (FRL_MUS_TITLE >= 0) return;
    FRL_MUS_TITLE = song_define("frl_title", 84, true, TITLE_LEAD, TITLE_PAD, TITLE_BASS, TITLE_DRUM);
    FRL_MUS_CLASS[FRC_MASON] = song_define("frl_mason", 100, true, MASON_LEAD, MASON_COMP, MASON_BASS, MASON_DRUM);
    FRL_MUS_CLASS[FRC_HUNTER] = song_define("frl_hunter", 120, true, HUNTER_LEAD, HUNTER_COMP, HUNTER_BASS, HUNTER_DRUM);
    FRL_MUS_CLASS[FRC_RUNNER] = song_define("frl_runner", 150, true, RUNNER_LEAD, RUNNER_COMP, RUNNER_BASS, RUNNER_DRUM);
    FRL_MUS_CLASS[FRC_TINKER] = song_define("frl_tinker", 112, true, TINKER_LEAD, TINKER_COMP, TINKER_BASS, TINKER_DRUM);
    FRL_MUS_CLASS[FRC_SAPPER] = song_define("frl_sapper", 136, true, SAPPER_LEAD, SAPPER_COMP, SAPPER_BASS, SAPPER_DRUM);
    FRL_MUS_OVER = song_define("frl_over", 70, false, OVER_LEAD, OVER_PAD, OVER_BASS, OVER_DRUM);
    FRL_MUS_END = song_define("frl_end", 80, true, END_LEAD, END_PAD, END_BASS, END_DRUM);
    FRL_MUS_SELECT = FRL_MUS_TITLE;

    sfx_define("frl_jump", CH_P2, 200, "@32 v9 o4 c32 g32");
    sfx_define("frl_double", CH_P2, 220, "@32 v9 o5 c32 e32 g32");
    sfx_define("frl_swing", CH_NOISE, 200, "@36 v10 o5 c16");
    sfx_define("frl_musket", CH_NOISE, 200, "@34 v12 o4 c16");
    sfx_define("frl_throw", CH_P2, 220, "@36 v8 o6 c32 @33 v7 o6 c32");
    sfx_define("frl_empty", CH_P2, 200, "@21 v6 o7 c32 r32 c32");
    sfx_define("frl_hit", CH_P2, 200, "@37 v10 o4 c32 o3 g32");
    sfx_define("frl_kill", CH_P2, 180, "@33 v11 o5 g32 e32 c32 o4 g16");
    sfx_define("frl_die", CH_P1, 140, "@33 v12 o4 e16 c16 o3 a16 e8");
    sfx_define("frl_eaten", CH_P1, 120, "@41 v13 o3 c8 o2 g8 c4");
    sfx_define("frl_ready", CH_P2, 220, "@39 v8 o6 c32 g32");
    sfx_define("frl_stone", CH_NOISE, 150, "@13 v13 o2 c8 @40 v8 o4 c8");
    sfx_define("frl_pouch", CH_P2, 180, "@35 v10 o5 e16 g16 o6 c8");
    sfx_define("frl_way", CH_P1, 160, "@39 v10 o5 c16 g16 o6 c16 g8");
    sfx_define("frl_pipe", CH_P1, 150, "@38 v10 o3 c16 g16 o4 c8");
    sfx_define("frl_blast", CH_NOISE, 120, "@34 v15 o3 c4");
    sfx_define("frl_break", CH_NOISE, 160, "@40 v11 o4 c16 @13 v10 o2 c16");
    sfx_define("frl_key", CH_P1, 180, "@35 v11 o6 c16 e16 g16 o7 c8");
    sfx_define("frl_door", CH_NOISE, 140, "@40 v10 o3 c8 @13 v9 o2 c8");
    sfx_define("frl_plate", CH_P1, 160, "@15 v11 o4 c16 g16 o5 c8");
    sfx_define("frl_plate_up", CH_P1, 160, "@15 v9 o5 c16 o4 g16 c8");
    sfx_define("frl_warp", CH_P1, 200, "@39 v9 o5 c32 e32 g32 o6 c32 e32 g32");
    sfx_define("frl_chute", CH_P1, 180, "@33 v10 o5 c16 o4 g16 e16 c16");
    sfx_define("frl_ammo", CH_P2, 200, "@35 v10 o6 c32 e32 g32");
    sfx_define("frl_heart", CH_NOISE, 100, "@12 v15 o3 c2");
    sfx_define("frl_foe_shot", CH_P2, 220, "@20 v5 o5 c32 o4 a32");
    sfx_define("frl_ting", CH_P2, 220, "@35 v9 o7 c32");
    sfx_define("frl_spawn", CH_P2, 200, "@20 v4 o3 c32 e32");
    sfx_define("frl_move", CH_P2, 220, "@42 v9 o6 c32");
    sfx_define("frl_pick", CH_P1, 180, "@20 v11 o5 c32 g32 o6 c16");
    sfx_define("frl_win", CH_P1, 140, "@39 v12 o5 c8 e8 g8 o6 c4");
}
