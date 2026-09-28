/* HOMESPUN - original music (UFO-MML) and sound effects, written for UFO 40.
 * Every looping tune is eight bars of 4/4 on each channel. */
#include "homespun.h"

int HS_MUS_TITLE = -1, HS_MUS_CAMP, HS_MUS_WILDS, HS_MUS_DEEP, HS_MUS_CAVE, HS_MUS_BOSS, HS_MUS_MAWBO, HS_MUS_END,
    HS_MUS_HOME, HS_MUS_FADE;

/* an off-beat chord bar: rest, chord, four times */
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define Q4(a, b) a "4 " b "4 " a "4 " b "4 "
#define QX(a, b) a "4 >" b "4< " a "4 >" b "4< "
#define E8(a) a "8 " a "8 >" a "8< " a "8 " a "8 " a "8 >" a "8< " a "8 "
#define BEAT "o2@13v12c8 o8@9v6c8 o6@11v10c8 o8@9v6c8 "
#define SOFT "o2@13v9c4 o8@9v4c8 o8@9v3c8 o6@11v6c4 o8@9v4c8 o8@9v3c8 "
#define DRIVE "o2@13v13c8 o8@9v7c8 o6@11v11c8 o2@13v10c8 o2@13v12c8 o8@9v7c8 o6@11v11c8 o8@10v8c8 "

/* "Homespun" - the title: C Am F G, twice through the tune */
static const char TITLE_LEAD[] =
    "@14 v11 q6"
    "| o5 e4 g8 e8 c4 e4 | d4 c8 d8 e2 | f4 a8 f8 c4 f4 | g4. f8 e4 d4"
    "| e8 f8 g8 a8 g4 e4 | a4 g8 e8 c4 e4 | f8 e8 d8 c8 d4 f4 | e4 d4 c2";
static const char TITLE_OFF[] =
    "v6 q3 o4 [" OFF("@16", "c") OFF("@17", "a") OFF("@16", "f") OFF("@16", "g") "]2";
static const char TITLE_BASS[] =
    "@6 v14 q5 [o3 " Q4("c", "g") "o2 " QX("a", "e") "o2 " QX("f", "c") "o2 " QX("g", "d") "]2";
static const char TITLE_DRUMS[] = "[" BEAT BEAT "]8";

/* "Glowstone Camp" - camp: an easy walk in F. F C Dm Bb | F C Bb C */
static const char CAMP_LEAD[] =
    "@3 v10 q5"
    "| o5 c8 f8 a8 f8 c4 f4 | e8 g8 o6 c8 o5 g8 e4 c4 | d8 f8 a8 f8 d4 a4 | b-4 a8 g8 f2"
    "| o5 a8 a8 g8 f8 a4 o6 c4 | o5 g4 e8 c8 e4 g4 | f8 g8 a8 b-8 a4 f4 | g4 e4 f2";
static const char CAMP_OFF[] =
    "v5 q3 o4 " OFF("@16", "f") OFF("@16", "c") OFF("@17", "d") OFF("@16", "b-")
    OFF("@16", "f") OFF("@16", "c") OFF("@16", "b-") OFF("@16", "c");
static const char CAMP_BASS[] =
    "@7 v13 q4 o2 " QX("f", "c") "o2 " QX("c", "g") "o2 " QX("d", "a") "o2 " QX("b-", "f")
    "o2 " QX("f", "c") "o2 " QX("c", "g") "o2 " QX("b-", "f") "o2 " QX("c", "g");
static const char CAMP_DRUMS[] = "[" SOFT "]8";

/* "Into the Wilds" - the Wilds: A minor, on the move. Am F C G | Am F G E */
static const char WILD_LEAD[] =
    "@1 v11 q5"
    "| o5 a4 e8 a8 o6 c4 o5 b4 | a8 g8 f8 e8 f4 c4 | e4 g8 o6 c8 e4 d4 | o5 d4 b8 g8 d2"
    "| o5 a8 b8 o6 c8 d8 e4 c4 | o5 a8 b8 o6 c8 o5 a8 f4 a4 | g8 a8 b8 o6 d8 c4 o5 b4 | g+4 b4 e2";
static const char WILD_OFF[] =
    "v6 q3 o4 " OFF("@17", "a") OFF("@16", "f") OFF("@16", "c") OFF("@16", "g")
    OFF("@17", "a") OFF("@16", "f") OFF("@16", "g") OFF("@16", "e");
static const char WILD_BASS[] =
    "@6 v14 q4 o2 " E8("a") "o2 " E8("f") "o2 " E8("c") "o2 " E8("g") "o2 " E8("a") "o2 " E8("f") "o2 " E8("g") "o2 " E8("e");
static const char WILD_DRUMS[] = "[" BEAT BEAT "]8";

/* "Deeper Down" - dungeons and the high Wilds: D minor, slow. Dm Dm Bb A | Dm Gm A A */
static const char DEEP_LEAD[] =
    "@5 v11 q6"
    "| o4 d4 f4 a4 f4 | g4 f8 e8 d2 | b-4 a4 g4 f4 | e4 c+4 e4 a4"
    "| o5 d4 c8 o4 a8 f4 d4 | g4 b-4 o5 d4 o4 b-4 | a4 g8 f8 e4 c+4 | d2 r2";
static const char DEEP_PAD[] = "@4 v6 q8 o3 f1 f1 f1 e1 f1 g1 e1 e1";
static const char DEEP_BASS[] = "@6 v13 q6 o2 d2 a2 d2 a2 b-2 f2 a2 e2 d2 a2 g2 d2 a2 e2 a2 e2";
static const char DEEP_DRUMS[] = "[o2@13v10c4 r4 o6@11v7c4 r4]8";

/* "Dripping Caves" - caves: E minor, a drip here and there */
static const char CAVE_LEAD[] =
    "@15 v10 q6"
    "| o5 e4 r8 b8 r4 g4 | f+4 r4 e2 | c4 r8 g8 r4 e4 | d+4 r4 o4 b2"
    "| o5 e4 g4 b4 o6 e4 | d4 c4 o5 b2 | a4 f+4 d+4 c4 | e1";
static const char CAVE_DRIP[] =
    "@20 v5 [r4 o7 c16 r16 r8 r2 | r2 r8 o6 g16 r16 r4]4";
static const char CAVE_BASS[] = "@7 v12 q8 o2 e1 e1 c1 o1 b1 o2 e1 g1 o1 b1 o2 e1";

/* "Guardian" - bosses: C minor, hard and fast. Cm Ab Bb G, twice */
static const char BOSS_LEAD[] =
    "@23 v12 q5"
    "| o5 c8 c8 e-8 g8 o6 c4 o5 g4 | a-8 g8 f8 e-8 f4 c4 | b-8 a-8 g8 f8 g4 d4 | g8 f8 e-8 d8 o4 b4 g4"
    "| o5 c8 e-8 g8 o6 c8 e-4 d4 | c8 o5 b-8 a-8 g8 a-4 f4 | b-8 o6 c8 d8 o5 b-8 g4 f4 | e-4 d4 c2";
static const char BOSS_OFF[] =
    "v6 q3 o4 [" OFF("@17", "c") OFF("@16", "a-") OFF("@16", "b-") OFF("@16", "g") "]2";
static const char BOSS_BASS[] =
    "@6 v14 q4 [o2 " E8("c") "o2 " E8("a-") "o2 " E8("b-") "o2 " E8("g") "]2";
static const char BOSS_DRUMS[] = "[" DRIVE "]8";

/* "Mawbo" - the six fights: E minor that won't sit still */
static const char MAWBO_LEAD[] =
    "@1 v12 q5"
    "| o5 e8 f8 e8 d+8 e4 b4 | o6 c8 o5 b8 a+8 b8 g4 e4 | f8 g8 f8 e8 f4 o6 c4 | o5 b8 a8 g8 f+8 e2"
    "| o6 e8 f8 e8 d+8 e4 o5 b4 | o6 c8 d8 e8 f8 g4 e4 | f8 e8 d+8 c8 o5 b4 a+4 | b2 e2";
static const char MAWBO_OFF[] =
    "v6 q3 o4 [" OFF("@17", "e") OFF("@17", "e") OFF("@16", "f") OFF("@17", "e") "]2";
static const char MAWBO_BASS[] = "@6 v14 q4 [o2 e8 e8 f8 e8 b8 e8 a+8 b8]8";
static const char MAWBO_DRUMS[] = "[" DRIVE "]8";

/* "The Long Way Home" - the ending: G major. G D Em C | G D C D */
static const char END_LEAD[] =
    "@14 v11 q6"
    "| o5 d4 g4 b4. a8 | a4 f+4 d2 | e4 g8 b8 o6 d4 o5 b4 | c4 e8 g8 e2"
    "| o5 d8 g8 b8 o6 d8 g4 f+4 | e8 d8 c+8 d8 o5 a2 | g8 a8 b8 o6 c8 e4 d4 | o5 b4 a4 g2";
static const char END_OFF[] =
    "v5 q3 o4 " OFF("@22", "g") OFF("@22", "d") OFF("@22", "e") OFF("@22", "c")
    OFF("@22", "g") OFF("@22", "d") OFF("@22", "c") OFF("@22", "d");
static const char END_BASS[] =
    "@7 v13 q5 o2 " QX("g", "d") "o2 " QX("d", "a") "o2 " QX("e", "b") "o2 " QX("c", "g")
    "o2 " QX("g", "d") "o2 " QX("d", "a") "o2 " QX("c", "g") "o2 " QX("d", "a");
static const char END_DRUMS[] = "[" SOFT "]8";

/* jingles */
static const char HOME_P1[] = "@39 v12 o5 l16 c e g >c8 <g16 >c16 e4";
static const char HOME_P2[] = "@2 v8 o4 l16 e g >c e8 c16 e16 g4";
static const char HOME_TRI[] = "@7 v13 o3 c8 g8 c4 c4";
static const char FADE_P1[] = "@5 v11 o5 e8 d+8 d8 c+8 c2";
static const char FADE_TRI[] = "@6 v12 o2 a4 g+4 g2";

void hs_audio_load(void) {
    if (HS_MUS_TITLE >= 0) return;
    HS_MUS_TITLE = song_define("hs_title", 112, true, TITLE_LEAD, TITLE_OFF, TITLE_BASS, TITLE_DRUMS);
    HS_MUS_CAMP = song_define("hs_camp", 100, true, CAMP_LEAD, CAMP_OFF, CAMP_BASS, CAMP_DRUMS);
    HS_MUS_WILDS = song_define("hs_wilds", 144, true, WILD_LEAD, WILD_OFF, WILD_BASS, WILD_DRUMS);
    HS_MUS_DEEP = song_define("hs_deep", 116, true, DEEP_LEAD, DEEP_PAD, DEEP_BASS, DEEP_DRUMS);
    HS_MUS_CAVE = song_define("hs_cave", 88, true, CAVE_LEAD, CAVE_DRIP, CAVE_BASS, "");
    HS_MUS_BOSS = song_define("hs_boss", 168, true, BOSS_LEAD, BOSS_OFF, BOSS_BASS, BOSS_DRUMS);
    HS_MUS_MAWBO = song_define("hs_mawbo", 176, true, MAWBO_LEAD, MAWBO_OFF, MAWBO_BASS, MAWBO_DRUMS);
    HS_MUS_END = song_define("hs_end", 92, true, END_LEAD, END_OFF, END_BASS, END_DRUMS);
    HS_MUS_HOME = song_define("hs_home", 150, false, HOME_P1, HOME_P2, HOME_TRI, "");
    HS_MUS_FADE = song_define("hs_fade", 90, false, FADE_P1, "", FADE_TRI, "");

    sfx_define("hs_yoyo", CH_NOISE, 240, "@36 v7 o7 c32");
    sfx_define("hs_hit", CH_NOISE, 240, "@9 v9 o6 c32");
    sfx_define("hs_pop", CH_NOISE, 200, "@36 v11 o6 c16 @34 v8 o5 c16");
    sfx_define("hs_bossdown", CH_NOISE, 100, "@34 v15 o3 c4");
    sfx_define("hs_stone", CH_P2, 240, "@15 v10 o7 c32 g32");
    sfx_define("hs_glint", CH_P2, 240, "@35 v8 o7 e32");
    sfx_define("hs_bar", CH_P2, 240, "@35 v10 o6 c32 g32");
    sfx_define("hs_odd", CH_P2, 240, "@35 v10 o6 b32 >e32");
    sfx_define("hs_tonic", CH_P2, 200, "@39 v10 o5 l32 c e g >c");
    sfx_define("hs_part", CH_P1, 160, "@39 v12 o5 l16 c e g >c e g >c8");
    sfx_define("hs_ouch", CH_P2, 200, "@37 v12 o5 e16 c16 <g8");
    sfx_define("hs_pot", CH_NOISE, 220, "@40 v10 o6 c16");
    sfx_define("hs_crack", CH_NOISE, 160, "@34 v13 o4 c8");
    sfx_define("hs_pipe", CH_NOISE, 200, "@34 v12 o5 c16");
    sfx_define("hs_click", CH_P2, 240, "@42 v8 o4 c32");
    sfx_define("hs_leap", CH_P2, 200, "@32 v10 o3 c8");
    sfx_define("hs_slam", CH_NOISE, 120, "@34 v15 o2 c8");
    sfx_define("hs_spit", CH_NOISE, 240, "@36 v8 o6 c32");
    sfx_define("hs_dive", CH_P2, 200, "@33 v10 o6 c8");
    sfx_define("hs_zap", CH_P2, 240, "@20 v10 o7 c32 <c32 >c32");
    sfx_define("hs_boom", CH_NOISE, 160, "@34 v13 o3 c8");
    sfx_define("hs_puff", CH_NOISE, 200, "@36 v8 o5 c16");
    sfx_define("hs_chest", CH_P2, 200, "@35 v11 o5 c16 e16 g16 >c8");
    sfx_define("hs_door", CH_P2, 200, "@41 v10 o3 c8");
    sfx_define("hs_step", CH_P2, 240, "@42 v5 o5 c32");
    sfx_define("hs_shut", CH_NOISE, 160, "@40 v12 o4 c8");
    sfx_define("hs_talk", CH_P2, 240, "@20 v8 o6 c32 e32");
    sfx_define("hs_gate", CH_P1, 160, "@23 v12 o4 g8 >c8 e4");
    sfx_define("hs_hop", CH_P2, 200, "@38 v12 o4 c16 g16 >c16 g16");
    sfx_define("hs_shuffle", CH_P2, 240, "@42 v8 o6 l32 c e c e");
    sfx_define("hs_nope", CH_P2, 200, "@37 v10 o3 c16 r32 c16");
    sfx_define("hs_buy", CH_P2, 240, "@35 v11 o6 c16 <g16 >c16");
}
