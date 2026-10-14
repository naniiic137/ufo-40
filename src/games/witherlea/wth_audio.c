/* WITHERLEA - original music (UFO-MML) and sounds. The house is nearly
 * silent: a low drone and a bell now and then; the chase is the only fast
 * tune. Every looping tune is eight bars a channel, so they loop together. */
#include "wth.h"

int WTH_MUS_TITLE = -1, WTH_MUS_HOUSE, WTH_MUS_UPSTAIRS, WTH_MUS_GARDEN, WTH_MUS_CHASE, WTH_MUS_HIDE, WTH_MUS_CELLAR,
    WTH_MUS_MAZE, WTH_MUS_ENDING, WTH_MUS_DAWN, WTH_MUS_CAUGHT, WTH_MUS_STING;

/* "Witherlea" - the title, A minor, slow */
static const char TITLE_LEAD[] =
    "@5 v10 q7 | o5 e2 d4 c4 | o4 b2. a4 | o5 c4 d4 e4 f4 | o5 e1 | o5 a2 g4 f4 | o5 e2. d4 | o5 c4 o4 b4 a4 g+4 | o4 a1";
static const char TITLE_PAD[] = "@4 v5 q8 o4 a1 g+1 a1 a1 f1 e1 g+1 e1";
static const char TITLE_BASS[] = "@6 v11 q8 o2 a1 e1 a1 a1 d1 a1 e1 a1";
static const char TITLE_WIND[] = "[@9 v2 o8 c2]16";

/* "The Quiet House" - hardly music at all: a drone, a bell, a creak */
static const char HOUSE_BELL[] = "@15 v6 o6 r1 r1 e4 r2. r1 r1 c4 r2. r1 o5 b2 r2";
static const char HOUSE_DRONE[] = "@4 v3 q8 o3 [a1]4 [f1]4";
static const char HOUSE_BASS[] = "@6 v7 q8 o2 [a1]4 [f1]4";
static const char HOUSE_CREAK[] = "[r1]7 @36 v3 o3 c2 r2";

/* "Lights Out" - upstairs, B minor, a heartbeat under it */
static const char UP_LEAD[] = "@5 v8 q7 | o5 f+2 g2 | o5 f+1 | o5 e2 f+2 | o5 d1 | o5 f+2 g2 | o5 a2 g4 f+4 | o5 e2 d2 | o5 c+1";
static const char UP_PAD[] = "@4 v4 q8 o4 [b1]2 [g1]2 [b1]2 [f+1]2";
static const char UP_BASS[] = "@6 v9 q7 o2 [b2 f+2]2 [g2 d2]2 [b2 f+2]2 [f+2 c+2]2";
static const char UP_BEAT[] = "[@13 v6 o2 c8 r8 @13 v4 o2 c8 r8 r2]8";

/* "The Yew and the Well" - outside, an owl over the wind */
static const char GDN_OWL[] = "@19 v6 q6 o5 r1 r2 d4 r4 o4 a2 r2 r1 r1 r2 o5 d4 r4 o4 a2 r2 r1";
static const char GDN_PAD[] = "@4 v3 q8 o4 [d1 c1]4";
static const char GDN_BASS[] = "@6 v7 q8 o2 [d1 c1]4";
static const char GDN_WIND[] = "[@12 v2 o3 c1]8";

/* "The Keeper" - the chase, E minor, fast */
static const char CH_LEAD[] =
    "@1 v12 q5 | o5 e8 e8 g8 e8 a8 e8 b8 a8 | o5 g8 f+8 e8 d+8 e4 r4 | o5 e8 e8 g8 e8 o6 c8 o5 b8 a8 g8 | o5 f+8 g8 a8 f+8 d+4 r4"
    "| o5 e8 e8 g8 e8 a8 e8 b8 a8 | o5 g8 a8 b8 o6 c8 o5 b4 r4 | o6 e8 d8 c8 o5 b8 a8 g8 f+8 d+8 | o5 e2 r2";
static const char CH_STAB[] =
    "@17 v6 q2 [o4 e16 r16]8 [o4 e16 r16]8 [o4 e16 r16]8 [o3 b16 r16]8 [o4 e16 r16]8 [o4 c16 r16]8 [o3 a16 r16]8 [o3 b16 r16]8";
static const char CH_BASS[] = "@6 v13 q4 [o2 e8]8 [o2 e8]8 [o2 e8]8 [o1 b8]8 [o2 e8]8 [o2 c8]8 [o1 a8]8 [o1 b8]8";
static const char CH_DRUM[] = "[@13 v12 o2 c8 @11 v9 o6 c8 @13 v10 o2 c8 @11 v10 o6 c8]16";

/* "Hold Your Breath" - hiding: a thin whine and your own heart */
static const char HIDE_WHINE[] = "@4 v2 q8 o5 [b1]8";
static const char HIDE_HEART[] = "@8 v12 [o2 c16 r16 c8 r4 r2]8";

/* "Under the House" - the cellar, half-steps grinding */
static const char CEL_LEAD[] = "@5 v7 q7 o4 [c2 c+2 c1]4";
static const char CEL_PAD[] = "@4 v3 q8 o3 [f+1 g1]4";
static const char CEL_BASS[] = "@6 v10 q8 o2 [c1]8";
static const char CEL_DRIP[] = "[@21 v2 o7 c4 r2. r1]4";

/* "The Hedges" - the maze, D minor, turning round on itself */
static const char MAZE_ARP[] =
    "@2 v6 q4 [o4 d16 f16 a16 f16]8 [o3 b-16 o4 d16 f16 d16]8 [o3 g16 b-16 o4 d16 o3 b-16]8 [o3 a16 o4 c+16 e16 c+16]8";
static const char MAZE_LEAD[] = "@5 v9 q7 | o5 a2 g4 f4 | o5 e2 d2 | o5 f2 e4 d4 | o5 c+1 | o5 d2 f4 g4 | o5 a2 b-2 | o5 a4 g4 f4 e4 | o5 d1";
static const char MAZE_BASS[] = "@6 v10 q8 o2 [d1]2 o1 [b-1]2 [g1]2 [a1]2";
static const char MAZE_TICK[] = "[@9 v2 o8 c4]32";

/* "The Lane Home" - the ending, C major */
static const char END_LEAD[] =
    "@14 v10 q7 | o5 e4 g4 c4 d4 | o5 e2. g4 | o5 f4 e4 d4 c4 | o5 d2. r4 | o5 e4 g4 o6 c4 o5 b4 | o5 a2. g4 | o5 f4 e4 d4 o4 b4 | o5 c1";
static const char END_PAD[] = "@4 v4 q8 o4 e1 e1 a1 g1 e1 e1 f1 e1";
static const char END_BASS[] = "@6 v11 q6 o2 c2 g2 c2 g2 f2 o3 c2 o2 g2 d2 c2 g2 a2 e2 f2 g2 c1";
static const char END_DRUM[] = "[@9 v3 o8 c8 r8 @11 v4 o6 c8 r8]16";

/* "Daybreak" - the best ending, F major, bright */
static const char DAWN_LEAD[] =
    "@23 v11 q7 | o5 c4 f4 a4 o6 c4 | o5 b-2 a2 | o5 g4 a4 b-4 o6 d4 | o6 c1 | o5 a4 o6 c4 f4 e4 | o6 d2 c2 | o5 b-4 a4 g4 e4 | o5 f1";
static const char DAWN_COMP[] =
    "@16 v5 q3 o4 [f8 r8]4 [b-8 r8]4 [c8 r8]4 [f8 r8]4 [f8 r8]4 [b-8 r8]4 [c8 r8]4 [f8 r8]4";
static const char DAWN_BASS[] = "@6 v12 q7 o2 f1 b-1 o3 c1 o2 f1 f1 b-1 o3 c1 o2 f1";
static const char DAWN_DRUM[] = "[@13 v10 o2 c8 @9 v4 o8 c8 @11 v8 o6 c8 @9 v4 o8 c8]16";

/* jingles */
static const char CAUGHT_P1[] = "@33 v13 o5 c4 o4 g4 e-4 c2";
static const char CAUGHT_TRI[] = "@41 v12 o3 c2.";
static const char CAUGHT_NOISE[] = "@34 v12 o3 c2";
static const char STING_P1[] = "@37 v14 o6 c+16 c16 c+16 c16 c+2";
static const char STING_P2[] = "@37 v12 o5 g16 f+16 g16 f+16 g2";
static const char STING_NOISE[] = "@12 v12 o4 c2";

void wth_audio_load(void) {
    if (WTH_MUS_TITLE >= 0) return;
    WTH_MUS_TITLE = song_define("wth_title", 70, true, TITLE_LEAD, TITLE_PAD, TITLE_BASS, TITLE_WIND);
    WTH_MUS_HOUSE = song_define("wth_house", 60, true, HOUSE_BELL, HOUSE_DRONE, HOUSE_BASS, HOUSE_CREAK);
    WTH_MUS_UPSTAIRS = song_define("wth_upstairs", 64, true, UP_LEAD, UP_PAD, UP_BASS, UP_BEAT);
    WTH_MUS_GARDEN = song_define("wth_garden", 66, true, GDN_OWL, GDN_PAD, GDN_BASS, GDN_WIND);
    WTH_MUS_CHASE = song_define("wth_chase", 168, true, CH_LEAD, CH_STAB, CH_BASS, CH_DRUM);
    WTH_MUS_HIDE = song_define("wth_hide", 76, true, HIDE_WHINE, "", HIDE_HEART, "");
    WTH_MUS_CELLAR = song_define("wth_cellar", 56, true, CEL_LEAD, CEL_PAD, CEL_BASS, CEL_DRIP);
    WTH_MUS_MAZE = song_define("wth_maze", 92, true, MAZE_ARP, MAZE_LEAD, MAZE_BASS, MAZE_TICK);
    WTH_MUS_ENDING = song_define("wth_ending", 84, true, END_LEAD, END_PAD, END_BASS, END_DRUM);
    WTH_MUS_DAWN = song_define("wth_dawn", 112, true, DAWN_LEAD, DAWN_COMP, DAWN_BASS, DAWN_DRUM);
    WTH_MUS_CAUGHT = song_define("wth_caught", 90, false, CAUGHT_P1, "", CAUGHT_TRI, CAUGHT_NOISE);
    WTH_MUS_STING = song_define("wth_sting", 120, false, STING_P1, STING_P2, "", STING_NOISE);

    sfx_define("wth_take", CH_P2, 220, "@35 v8 o6 c32 g32");
    sfx_define("wth_page", CH_NOISE, 200, "@36 v5 o7 c16");
    sfx_define("wth_tick", CH_P2, 240, "@42 v7 o6 c32");
    sfx_define("wth_bag", CH_NOISE, 220, "@36 v5 o5 c32 c32");
    sfx_define("wth_click", CH_NOISE, 240, "@21 v8 o6 c32");
    sfx_define("wth_door", CH_P2, 160, "@37 v8 o3 c8 o2 g16");
    sfx_define("wth_clank", CH_NOISE, 200, "@40 v10 o5 c8");
    sfx_define("wth_chime", CH_P1, 150, "@15 v12 o5 c4 c4 c4 c4 c4 c4 c4 c4");
    sfx_define("wth_fire", CH_NOISE, 160, "@34 v9 o4 c4");
    sfx_define("wth_static", CH_NOISE, 120, "@10 v13 o7 c2");
    sfx_define("wth_orch", CH_P1, 200, "@16 v12 o5 c8 e8 g8 >c8 <g8 e8 c4");
    sfx_define("wth_drain", CH_NOISE, 120, "@36 v8 o3 c2");
    sfx_define("wth_flush", CH_NOISE, 120, "@34 v11 o5 c2");
    sfx_define("wth_glass", CH_NOISE, 160, "@40 v14 o7 c4");
    sfx_define("wth_rumble", CH_NOISE, 100, "@34 v12 o2 c1");
    sfx_define("wth_engine", CH_NOISE, 100, "@34 v13 o2 c2 c2");
    sfx_define("wth_dig", CH_NOISE, 180, "@36 v8 o4 c8 r8 c8");
    sfx_define("wth_grind", CH_NOISE, 100, "@40 v9 o3 c1");
    sfx_define("wth_creak", CH_P2, 120, "@38 v7 o3 c4");
    sfx_define("wth_thud", CH_TRI, 160, "@41 v14 o3 c4");
    sfx_define("wth_musicbox", CH_P1, 180, "@15 v11 o6 e8 g8 b8 >e8 <b8 g8 e8 r8 e8 g8 b8 >e8");
    sfx_define("wth_organ", CH_P1, 120, "@23 v13 o4 c2 e2 g2");
    sfx_define("wth_smash", CH_NOISE, 200, "@40 v11 o6 c8");
    sfx_define("wth_fizz", CH_NOISE, 160, "@10 v6 o8 c4");
    sfx_define("wth_step", CH_NOISE, 240, "@13 v5 o3 c32");
    sfx_define("wth_bell", CH_P2, 200, "@15 v10 o7 c16 e16 c16 e16");
    sfx_define("wth_sting", CH_P1, 200, "@37 v14 o6 c+16 c16 c+8");
    sfx_define("wth_swing", CH_NOISE, 200, "@36 v14 o5 c8 @34 v14 o3 c4");
    sfx_define("wth_hide", CH_P2, 160, "@33 v7 o4 c8");
    sfx_define("wth_found", CH_P1, 200, "@37 v14 o5 c16 o4 c8");
    sfx_define("wth_breath", CH_NOISE, 200, "@36 v4 o6 c16");
    sfx_define("wth_gasp", CH_NOISE, 200, "@36 v10 o5 c8");
    sfx_define("wth_shot", CH_NOISE, 200, "@36 v12 o6 c16 @41 v14 o3 c8");
    sfx_define("wth_hit", CH_NOISE, 220, "@34 v11 o4 c16");
    sfx_define("wth_block", CH_NOISE, 220, "@40 v13 o6 c16");
}
