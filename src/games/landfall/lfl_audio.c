/* LANDFALL - original music (UFO-MML) and sound effects. Every looping
 * tune is eight bars a channel, so the channels loop together. */
#include "lfl.h"

int LFL_MUS_TITLE = -1, LFL_MUS_LAND, LFL_MUS_PLANET, LFL_MUS_DUNGEON, LFL_MUS_LIFTOFF, LFL_MUS_ENDING, LFL_MUS_OVER,
    LFL_MUS_CHART;

#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
#define DR(a, b) a "2 " b "2 "

/* "Landfall" - the title, A dorian over a slow pulse */
static const char TITLE_LEAD[] =
    "@14 v11 q7"
    "| o5 a4. e8 a4 b4 | o6 c4 o5 b8 a8 g4 e4 | o5 f+4. g8 a4 b4 | o5 a2 r2"
    "| o5 a4. e8 a4 b4 | o6 c4 d8 e8 d4 c4 | o5 b4 a8 g8 f+4 g4 | o5 a1";
static const char TITLE_ARP[] =
    "@2 v5 q4 " ARP("o4 a", "o5 c", "e", "c") ARP("o4 g", "o5 c", "e", "c") ARP("o4 f+", "a", "o5 d", "o4 a")
    ARP("o4 a", "o5 c", "e", "c") ARP("o4 a", "o5 c", "e", "c") ARP("o4 f", "a", "o5 c", "o4 a")
    ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 a", "o5 c", "e", "c");
static const char TITLE_BASS[] =
    "@6 v12 q7 " DR("o2 a", "o3 e") DR("o3 c", "g") DR("o2 d", "a") DR("o2 a", "o3 e") DR("o2 a", "o3 e")
    DR("o2 f", "o3 c") DR("o2 g", "o3 d") DR("o2 a", "o3 e");
static const char TITLE_DRUM[] = "[@13 v10 o2 c4 @9 v4 o8 c8 c8 @11 v7 o6 c4 @9 v4 o8 c8 c8]8";

/* "Strange Ground" - the planet, E phrygian, sparse and wary */
static const char PLANET_LEAD[] =
    "@5 v10 q7"
    "| o5 e4 r8 f8 g4 r4 | o5 b-4 a8 g8 f2 | o5 e4 r8 g8 b4 r4 | o6 c4 o5 b8 a8 g2"
    "| o5 e4 r8 f8 g4 a4 | o5 b-4 o6 c8 d8 e4 d4 | o6 c4 o5 b-8 a8 g4 f4 | o5 e1";
static const char PLANET_PAD[] = "@4 v4 q8 o3 e1 f1 e1 c1 e1 g1 f1 e1";
static const char PLANET_BASS[] =
    "@6 v11 q5 o2 e4 r4 e4 r4 f4 r4 f4 r4 e4 r4 e4 r4 c4 r4 c4 r4 e4 r4 e4 r4 g4 r4 g4 r4 f4 r4 f4 r4 e4 r4 e4 r4";
static const char PLANET_DRUM[] = "[@9 v2 o8 c4 @9 v1 o8 c4 @11 v3 o6 c4 @9 v1 o8 c4]8";

/* "Under Yombra" - the dungeon, C minor ostinato */
static const char DUN_LEAD[] =
    "@1 v10 q6"
    "| o4 c8 e-8 g8 c8 e-8 g8 a-4 | o4 g8 f8 e-8 d8 c2 | o4 c8 e-8 g8 c8 e-8 g8 b-4 | o4 a-8 g8 f8 e-8 d2"
    "| o4 g4 a-8 g8 f4 e-4 | o4 f4 g8 f8 e-4 d4 | o4 e-8 d8 c8 d8 e-4 g4 | o4 c1";
static const char DUN_OST[] =
    "@3 v6 q4 o3 [c8 g8]4 [c8 g8]4 [c8 g8]4 [c8 g8]4 [o2 a-8 o3 e-8]4 [f8 o4 c8 o3]4 [g8 o4 d8 o3]4 [c8 g8]4";
static const char DUN_BASS[] = "@6 v12 q8 o2 c1 c1 c1 g1 a-1 f1 g1 c1";
static const char DUN_DRUM[] = "[@13 v11 o2 c8 @9 v4 o8 c8 @11 v8 o6 c8 @9 v4 o8 c8]16";

/* "The Way Home" - the ending, C major, warm */
static const char END_LEAD[] =
    "@23 v11 q7"
    "| o5 e4 g4 o6 c4 o5 b4 | o5 a4 g4 e2 | o5 f4 a4 o6 d4 c4 | o5 b2 g2"
    "| o5 e4 g4 o6 c4 e4 | o6 d4 c4 o5 a4 o6 c4 | o5 b4 o6 c4 d4 o5 b4 | o6 c1";
static const char END_CHORD[] = "@16 v5 q8 o4 c1 @17 a1 @16 f1 g1 c1 @17 d1 @16 g1 c1";
static const char END_BASS[] = "@6 v11 q7 o2 c2 g2 a2 e2 f2 o3 c2 o2 g2 d2 c2 g2 d2 a2 g2 d2 c1";
static const char END_DRUM[] = "[@13 v8 o2 c4 @9 v3 o8 c4 @11 v5 o6 c4 @9 v3 o8 c4]8";

/* jingles */
static const char LAND_P1[] = "@15 v12 o5 c8 e8 g8 o6 c4 r8 o5 g8 o6 c2";
static const char LAND_TRI[] = "@6 v11 o3 c2 g4 r8 g8 c2";
static const char CHART_P1[] = "@39 v12 o5 l16 c e g o6 c e g o7 c4";
static const char CHART_P2[] = "@15 v8 o5 e8 g8 o6 c4 r8";
static const char LIFT_P1[] = "@14 v12 o4 c8 e8 g8 o5 c8 e8 g8 o6 c2";
static const char LIFT_NOISE[] = "@34 v10 o3 c1";
static const char OVER_P1[] = "@5 v11 o5 a4 g4 f4 e4 d2 c+2 d1";
static const char OVER_TRI[] = "@6 v10 o3 d1 a1 d1";

static bool loaded;

void lfl_audio_load(void) {
    if (loaded) return;
    loaded = true;
    LFL_MUS_TITLE = song_define("lfl_title", 112, true, TITLE_LEAD, TITLE_ARP, TITLE_BASS, TITLE_DRUM);
    LFL_MUS_PLANET = song_define("lfl_planet", 96, true, PLANET_LEAD, PLANET_PAD, PLANET_BASS, PLANET_DRUM);
    LFL_MUS_DUNGEON = song_define("lfl_dungeon", 120, true, DUN_LEAD, DUN_OST, DUN_BASS, DUN_DRUM);
    LFL_MUS_ENDING = song_define("lfl_ending", 100, true, END_LEAD, END_CHORD, END_BASS, END_DRUM);
    LFL_MUS_LAND = song_define("lfl_land", 140, false, LAND_P1, "", LAND_TRI, "");
    LFL_MUS_CHART = song_define("lfl_chart", 150, false, CHART_P1, CHART_P2, "", "");
    LFL_MUS_LIFTOFF = song_define("lfl_liftoff", 130, false, LIFT_P1, "", "", LIFT_NOISE);
    LFL_MUS_OVER = song_define("lfl_over", 80, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("lfl_hurt", CH_P1, 200, "@37 v12 o5 c16 o4 g16");
    sfx_define("lfl_die", CH_P1, 140, "@33 v13 o5 c8 o4 a8 f8 d4");
    sfx_define("lfl_res", CH_P1, 240, "@35 v9 o6 e32 b32");
    sfx_define("lfl_eat", CH_P2, 220, "@20 v8 o4 c32 e32 c32");
    sfx_define("lfl_chart", CH_P2, 200, "@39 v11 o6 c16 g16");
    sfx_define("lfl_tool", CH_P1, 200, "@35 v11 o5 c16 g16 o6 c16");
    sfx_define("lfl_cube", CH_P1, 180, "@39 v12 o5 l16 c e g o6 c e");
    sfx_define("lfl_bundle", CH_P1, 220, "@35 v10 o6 c32 e32 g32 o7 c32");
    sfx_define("lfl_spit", CH_P2, 220, "@37 v6 o5 g16");
    sfx_define("lfl_crunch", CH_NOISE, 200, "@40 v10 o5 c8");
    sfx_define("lfl_pop", CH_NOISE, 220, "@34 v8 o5 c16");
    sfx_define("lfl_tink", CH_P2, 240, "@42 v8 o7 c32");
    sfx_define("lfl_hit", CH_NOISE, 240, "@21 v8 o6 c32");
    sfx_define("lfl_morph", CH_P2, 200, "@39 v9 o6 c32 g32 o7 c32");
    sfx_define("lfl_boom", CH_NOISE, 140, "@34 v13 o3 c4");
    sfx_define("lfl_secret", CH_P1, 200, "@39 v12 o6 l32 c d e f g a b o7 c");
    sfx_define("lfl_nope", CH_P2, 200, "@37 v9 o3 c8 r16 c8");
    sfx_define("lfl_rivet", CH_P2, 240, "@20 v6 o6 c32 g32");
    sfx_define("lfl_disc", CH_NOISE, 220, "@36 v7 o5 c16");
    sfx_define("lfl_hatch", CH_P2, 220, "@38 v8 o5 c16 g16");
    sfx_define("lfl_pod", CH_P2, 220, "@42 v8 o4 c16");
    sfx_define("lfl_thump", CH_NOISE, 120, "@41 v15 o2 c4");
    sfx_define("lfl_print", CH_P2, 220, "@42 v8 o5 c32 r32 c32 e32");
    sfx_define("lfl_crystal", CH_P1, 240, "@15 v10 o7 c32 e32");
    sfx_define("lfl_bounce", CH_P2, 240, "@42 v6 o6 g32");
    sfx_define("lfl_shrink", CH_P1, 200, "@33 v11 o6 c16 o5 c16");
    sfx_define("lfl_grow", CH_P1, 200, "@32 v11 o5 c16 o6 c16");
    sfx_define("lfl_shards", CH_NOISE, 200, "@40 v12 o6 c8");
    sfx_define("lfl_buzz", CH_P2, 160, "@20 v6 o6 c8 c+8 c8");
    sfx_define("lfl_stairs", CH_P1, 200, "@33 v10 o5 l16 g e c");
    sfx_define("lfl_warp", CH_P1, 200, "@38 v11 o4 c16 g16 o5 c16 g16");
    sfx_define("lfl_drop", CH_P2, 220, "@42 v8 o4 g16 c16");
    sfx_define("lfl_swap", CH_P2, 240, "@42 v7 o5 c32 e32");
    sfx_define("lfl_talk", CH_P2, 240, "@20 v7 o6 c32 d32 c32");
    sfx_define("lfl_trade", CH_P1, 200, "@35 v11 o5 c16 e16 g16 o6 c16");
    sfx_define("lfl_liftoff", CH_NOISE, 120, "@34 v12 o2 c2");
}
