/* TURNIP TRUCK - original music (UFO-MML) and sound effects. Like its
 * original: a newscast sting each morning, a tune for the working day, a
 * faster one for overtime, and on the rainy day only the rain (until the
 * overtime tune comes in). Plus a title tune, an ending and four jingles. */
#include "tnp.h"

int TNP_MUS_TITLE = -1, TNP_MUS_NEWS, TNP_MUS_DAY, TNP_MUS_OVERTIME, TNP_MUS_RAIN, TNP_MUS_CLEAR, TNP_MUS_FAIL,
    TNP_MUS_END, TNP_MUS_FIRED;

/* a bar of bass: root and octave, eighths */
#define OCT(n) "[o2 " n "8 o3 " n "8]4 "
/* a bar of bass in sixteenths */
#define OCT16(n) "[o2 " n "16 o3 " n "16]8 "
/* a bar of off-beat chord stabs */
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define BEAT "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
#define BEAT16 "@13 v12 o2 c16 @9 v5 o8 c16 @11 v10 o6 c16 @9 v5 o8 c16 "

/* "Turnip Truck" - the title, F major, 8 bars */
static const char TITLE_LEAD[] =
    "@0 v11 q6"
    "| o5 f8 a8 o6 c8 o5 a8 b-8 a8 g8 f8 | o5 g8 b-8 o6 d8 o5 b-8 a4 c4"
    "| o5 f8 a8 o6 c8 f8 e8 d8 c8 o5 b-8 | o5 a8 g8 f8 e8 f4 r4"
    "| o6 c8 c8 d8 c8 o5 a8 f8 g8 a8 | o5 b-8 b-8 o6 c8 o5 b-8 g8 e8 f8 g8"
    "| o5 a8 o6 c8 f8 c8 d8 o5 b-8 g8 e8 | o5 f4 c4 f4 r4";
static const char TITLE_OFF[] =
    "v6 q3 " OFF("@16 o4", "f") OFF("@17 o4", "g") OFF("@16 o4", "f") OFF("@16 o4", "c")
    OFF("@16 o4", "f") OFF("@16 o4", "c") OFF("@16 o4", "f") OFF("@16 o4", "f");
static const char TITLE_BASS[] =
    "@6 v14 q4 " OCT("f") OCT("g") OCT("f") OCT("c") OCT("f") OCT("c") OCT("f") OCT("f");
static const char TITLE_DRUM[] = "[" BEAT BEAT "]8";

/* "News at Nine" - the morning newscast's sting */
static const char NEWS_P1[] = "@1 v12 q6 o5 c8 r16 c16 g4 r8 c8 r16 c16 a4 r8 o5 g8 f8 e8 d8 c4 o6 c4 r2";
static const char NEWS_P2[] = "@2 v7 q5 o4 e8 r16 e16 b4 r8 e8 r16 e16 o5 c4 r8 o4 b8 a8 g8 f8 e4 o5 e4 r2";
static const char NEWS_TRI[] = "@6 v14 o3 c4 r8 c8 c4 r8 o2 a8 f4 g4 o3 c4 o2 c4 r2";
static const char NEWS_NOISE[] = "@11 v10 o6 c16 c16 c16 c16 c4 r8 @11 c16 c16 c4 r8 @13 v12 o2 c4 c4 @12 v12 o5 c2 r2";

/* "On the Clock" - the working day, G major, 16 bars */
static const char DAY_LEAD[] =
    "@1 v11 q6"
    "| o5 g8 b8 o6 d8 o5 b8 g4 d4 | o5 a8 b8 o6 c8 o5 b8 a4 g4"
    "| o6 c8 c8 e8 c8 o5 g4 e4 | o5 f+8 g8 a8 b8 o6 c4 d4"
    "| o5 g8 b8 o6 d8 g8 f+4 d4 | o5 e8 g8 b8 o6 e8 d4 o5 b4"
    "| o5 c8 e8 g8 o6 c8 o5 b8 a8 g8 e8 | o5 d8 f+8 a8 o6 c8 o5 b4 a4"
    "| o6 d4 o5 b8 o6 d8 e4 d4 | o6 c4 o5 a8 o6 c8 d4 c4"
    "| o5 b8 o6 c8 d8 e8 d8 c8 o5 b8 a8 | o5 a8 b8 o6 c8 d8 e4 f+4"
    "| o6 g4 d8 o5 b8 g4 b4 | o6 c4 o5 a8 f+8 d4 f+4"
    "| o5 g8 a8 b8 o6 c8 d8 e8 f+8 d8 | o6 g2 r2";
static const char DAY_OFF[] =
    "v6 q3 " OFF("@16 o4", "g") OFF("@16 o4", "g") OFF("@16 o4", "c") OFF("@16 o4", "d")
    OFF("@16 o4", "g") OFF("@17 o4", "e") OFF("@16 o4", "c") OFF("@16 o4", "d")
    OFF("@16 o4", "g") OFF("@16 o4", "c") OFF("@17 o4", "e") OFF("@16 o4", "d")
    OFF("@16 o4", "g") OFF("@16 o4", "d") OFF("@16 o4", "g") OFF("@16 o4", "g");
static const char DAY_BASS[] =
    "@6 v14 q4 " OCT("g") OCT("g") OCT("c") OCT("d") OCT("g") OCT("e") OCT("c") OCT("d")
    OCT("g") OCT("c") OCT("e") OCT("d") OCT("g") OCT("d") OCT("g") OCT("g");
static const char DAY_DRUM[] = "[" BEAT BEAT "]16";

/* "After Hours" - overtime, E minor, 8 bars, quicker */
static const char OVER_LEAD[] =
    "@0 v12 q5"
    "| o5 e8 e8 g8 e8 b8 e8 a8 g8 | o5 f+8 f+8 a8 f+8 o6 c8 o5 b8 a8 f+8"
    "| o5 g8 g8 b8 g8 o6 e8 d8 c8 o5 b8 | o5 a8 b8 o6 c8 d8 e4 d+4"
    "| o6 e8 e8 d8 e8 g8 e8 d8 o5 b8 | o6 c8 c8 o5 b8 o6 c8 e8 c8 o5 b8 a8"
    "| o5 b8 o6 c8 d8 e8 f+8 g8 a8 f+8 | o6 e4 o5 b4 e4 r4";
static const char OVER_STAB[] =
    "v6 q2 " OFF("@17 o4", "e") OFF("@16 o4", "d") OFF("@17 o4", "e") OFF("@16 o3", "b")
    OFF("@17 o4", "e") OFF("@17 o4", "a") OFF("@16 o4", "d") OFF("@17 o4", "e");
static const char OVER_BASS[] =
    "@6 v14 q4 " OCT16("e") OCT16("d") OCT16("e") OCT16("b") OCT16("e") OCT16("a") OCT16("d") OCT16("e");
static const char OVER_DRUM[] = "[" BEAT16 BEAT16 BEAT16 BEAT16 "]8";

/* "Rain on the Roof" - the downpour: only the rain */
static const char RAIN_NOISE[] =
    "[@9 v4 o8 c16 v3 c16 v5 c16 v2 c16 v4 c16 v3 c16 v5 c16 v3 c16 "
    "v4 c16 v2 c16 v4 c16 v3 c16 v5 c16 v2 c16 v4 c16 v3 c16]7 "
    "@12 v7 o3 c1";

/* "Clocked Out", "Written Off" and "Pink Slip" - the jingles */
static const char CLEAR_P1[] = "@1 v12 o5 l16 g b o6 d g8 d8 g4 r4";
static const char CLEAR_TRI[] = "@6 v14 o3 l16 g r g r d8 o2 b8 o3 g4 r4";
static const char CLEAR_NOISE[] = "@11 v9 o6 c16 c16 c16 c16 c8 c8 @12 v10 o5 c4 r4";
static const char FAIL_P1[] = "@14 v11 o5 e8 d8 c8 o4 b8 a4 r8 a8 g+2";
static const char FAIL_TRI[] = "@6 v13 o3 c4 o2 g4 a4 r8 f8 e2";
static const char FIRED_P1[] = "@20 v10 o6 c8 r8 c8 r8 @14 v11 o5 c4 o4 b4 b-4 a4 a-2 g2";
static const char FIRED_TRI[] = "r2 @6 v13 o3 c4 o2 b4 b-4 a4 a-2 g2";

/* "Sunday Off" - the ending, C major */
static const char END_LEAD[] =
    "@14 v11 q7"
    "| o5 e4 g4 o6 c4 o5 g4 | o5 a4 o6 c4 o5 g2 | o5 f4 a4 g4 e4 | o5 d2. r4"
    "| o5 e4 g4 o6 c4 e4 | o6 d4 c4 o5 a4 b4 | o6 c4 o5 g4 f4 d4 | o5 c2. r4";
static const char END_ARP[] =
    "@2 v5 q5 [o4 c8 e8 g8 e8]2 [o4 f8 a8 o5 c8 o4 a8]2 [o4 f8 a8 o5 c8 o4 a8]2 [o4 g8 b8 o5 d8 o4 b8]2"
    " [o4 c8 e8 g8 e8]2 [o4 g8 b8 o5 d8 o4 b8]2 [o4 c8 e8 g8 e8]2 [o4 c8 e8 g8 e8]2";
static const char END_BASS[] = "@6 v13 q6 o2 c2 g2 f2 c2 f2 c2 g2 d2 c2 g2 g2 d2 c2 g2 c2 c2";
static const char END_DRUM[] = "[@13 v10 o2 c4 @11 v8 o6 c4 @13 v9 o2 c8 c8 @11 v8 o6 c4]8";

void tnp_audio_load(void) {
    if (TNP_MUS_TITLE >= 0) return;
    TNP_MUS_TITLE = song_define("tnp_title", 132, true, TITLE_LEAD, TITLE_OFF, TITLE_BASS, TITLE_DRUM);
    TNP_MUS_NEWS = song_define("tnp_news", 120, false, NEWS_P1, NEWS_P2, NEWS_TRI, NEWS_NOISE);
    TNP_MUS_DAY = song_define("tnp_day", 150, true, DAY_LEAD, DAY_OFF, DAY_BASS, DAY_DRUM);
    TNP_MUS_OVERTIME = song_define("tnp_overtime", 168, true, OVER_LEAD, OVER_STAB, OVER_BASS, OVER_DRUM);
    TNP_MUS_RAIN = song_define("tnp_rain", 120, true, "", "", "", RAIN_NOISE);
    TNP_MUS_CLEAR = song_define("tnp_clear", 150, false, CLEAR_P1, "", CLEAR_TRI, CLEAR_NOISE);
    TNP_MUS_FAIL = song_define("tnp_fail", 110, false, FAIL_P1, "", FAIL_TRI, "");
    TNP_MUS_END = song_define("tnp_end", 100, true, END_LEAD, END_ARP, END_BASS, END_DRUM);
    TNP_MUS_FIRED = song_define("tnp_fired", 96, false, FIRED_P1, "", FIRED_TRI, "");

    sfx_define("tnp_bump", CH_NOISE, 240, "@13 v11 o3 c16");
    sfx_define("tnp_crash", CH_NOISE, 200, "@34 v14 o4 c8");
    sfx_define("tnp_smash", CH_NOISE, 200, "@40 v14 o4 c8 @34 v12 o3 c8");
    sfx_define("tnp_hurt", CH_P1, 180, "@37 v13 o5 c8 o4 g8");
    sfx_define("tnp_wreck", CH_NOISE, 120, "@34 v15 o3 c2");
    sfx_define("tnp_heal", CH_P1, 220, "@39 v11 o5 l32 c e g o6 c");
    sfx_define("tnp_skid", CH_NOISE, 200, "@36 v10 o7 c8 c8 c8");
    sfx_define("tnp_jink", CH_NOISE, 240, "@36 v7 o6 c32");
    sfx_define("tnp_ramp", CH_P2, 220, "@32 v10 o4 c16 g16 o5 c16");
    sfx_define("tnp_land", CH_P2, 240, "@41 v10 o3 c16");
    sfx_define("tnp_splash", CH_NOISE, 160, "@10 v13 o5 c8 @9 v8 o6 c8");
    sfx_define("tnp_crate", CH_P1, 220, "@35 v11 o6 c16 e16 g16 o7 c8");
    sfx_define("tnp_deliver", CH_P1, 200, "@39 v12 o5 l16 g o6 c e g8");
    sfx_define("tnp_quota", CH_P1, 180, "@39 v13 o5 l16 c e g o6 c e g o7 c4");
    sfx_define("tnp_boom", CH_NOISE, 140, "@34 v14 o3 c4");
    sfx_define("tnp_beet", CH_P2, 240, "@33 v8 o3 c16 o2 g16");
    sfx_define("tnp_squish", CH_NOISE, 240, "@21 v8 o4 c32 @13 v6 o3 c32");
    sfx_define("tnp_zap", CH_P2, 240, "@33 v9 o6 c16 o5 c16");
    sfx_define("tnp_bang", CH_NOISE, 240, "@13 v12 o5 c32 @11 v8 o6 c32");
    sfx_define("tnp_tick", CH_P2, 240, "@42 v9 o6 c32");
    sfx_define("tnp_horn", CH_P2, 180, "@3 v11 o4 e16 r32 e8");
}
