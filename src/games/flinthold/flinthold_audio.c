/* FLINTHOLD - original music (UFO-MML) and sound effects, written for UFO 40.
 * Like the cartridge it pays tribute to, the build phase has no music: the
 * tunes start when the horn sounds. */
#include "flinthold.h"

int FH_MUS_TITLE = -1, FH_MUS_MAP, FH_MUS_VILLAGE, FH_MUS_BATTLE1, FH_MUS_BATTLE2, FH_MUS_BATTLE3, FH_MUS_LORDS,
    FH_MUS_END, FH_MUS_WAVE, FH_MUS_CLEAR, FH_MUS_FELL;

/* an off-beat chord bar: rest, chord, four times */
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define Q4(a, b) a "4 " b "4 " a "4 " b "4 "
#define QX(a, b) a "4 >" b "4< " a "4 >" b "4< " /* the fifth an octave up */
#define TOM "o2@13v13c8 o4@11v6c8 o3@13v10c8 o8@9v5c8 "
#define TOM2 "o2@13v13c8 o2@13v9c8 o4@11v9c8 o8@9v6c8 "

/* "Flinthold" - the title: a stomping march in D minor. Dm C F C | Dm F C Dm */
static const char TITLE_LEAD[] =
    "@1 v12 q6"
    "| o5 d4 a4 f8 e8 d4 | c4 e8 f8 g2 | a4 o6 c4 o5 a8 g8 f4 | e8 f8 g8 e8 d2"
    "| o5 d8 d8 a4 o6 c4 d4 | o6 c8 o5 a8 g8 f8 e4 c4 | d8 f8 a8 o6 d8 c8 o5 a8 g8 e8 | d2. r4";
static const char TITLE_OFF[] =
    "v6 q3 o4 " OFF("@17", "d") OFF("@16", "c") OFF("@16", "f") OFF("@16", "c")
    OFF("@17", "d") OFF("@16", "f") OFF("@16", "c") OFF("@17", "d");
static const char TITLE_BASS[] =
    "@6 v14 q5 | o3 " Q4("d", "a") "| o3 " Q4("c", "g") "| o2 " QX("f", "c") "| o3 " Q4("c", "g")
    "| o3 " Q4("d", "a") "| o2 " QX("f", "c") "| o3 " Q4("c", "g") "| o3 d4 a4 d2";
static const char TITLE_DRUMS[] = "[" TOM "]16";

/* "The Island" - the map: an easy stroll in G. G Em C D, twice */
static const char MAP_LEAD[] =
    "@14 v10 q5"
    "| o5 g4 b8 o6 d8 r8 o5 b8 g4 | e4 g8 b8 r8 g8 e4 | c4 e8 g8 r8 e8 c4 | d8 f+8 a8 f+8 d2"
    "| o5 g8 a8 b8 o6 d8 c8 o5 b8 a8 g8 | e8 f+8 g8 b8 a4 e4 | c8 d8 e8 g8 f+8 e8 d8 c8 | o5 d2 g2";
static const char MAP_OFF[] =
    "v5 q3 o4 [" OFF("@16", "g") OFF("@17", "e") OFF("@16", "c") OFF("@16", "d") "]2";
static const char MAP_BASS[] =
    "@7 v13 q4 [o2 " QX("g", "d") "o2 " Q4("e", "b") "o2 " Q4("c", "g") "o2 " Q4("d", "a") "]2";
static const char MAP_DRUMS[] = "[o3@13v8c4 o8@9v4c8 o8@9v3c8 o4@11v5c4 o8@9v4c8 o8@9v3c8]8";

/* "Hearth Song" - the villages: a gentle waltz feel in F, 4/4 by the bar */
static const char VIL_LEAD[] =
    "@3 v10 q5"
    "| o5 f4. a8 o6 c4 o5 a4 | b-4. g8 e4 c4 | a4. f8 c4 f4 | g8 a8 b-8 a8 g2"
    "| o5 f4. a8 o6 c4 f4 | e4. d8 c4 o5 b-4 | a8 g8 f8 a8 g4 e4 | f2. r4";
static const char VIL_OFF[] =
    "v5 q3 o4 " OFF("@16", "f") OFF("@17", "g") OFF("@16", "f") OFF("@16", "c")
    OFF("@16", "f") OFF("@16", "c") OFF("@16", "c") OFF("@16", "f");
static const char VIL_BASS[] =
    "@6 v13 q6 | o2 f2 >c2< | o2 g2 >d2< | o2 f2 >c2< | o2 c2 g2"
    "| o2 f2 >c2< | o2 c2 g2 | o2 c2 e2 | o2 f2 >c2<";
static const char VIL_DRUMS[] = "[o6@11v4c4 o8@9v3c8 o8@9v3c8 o3@13v6c4 o8@9v3c4]8";

/* "Horns at Dawn" - battles, stages 1-3: A minor, drums in front */
static const char B1_LEAD[] =
    "@23 v12 q5"
    "| o5 a8 a8 o6 c8 e8 d8 c8 o5 b8 g8 | a4 e4 a2 | f8 f8 a8 o6 c8 o5 b8 a8 g8 f8 | e4 g+4 b2"
    "| o6 c8 o5 b8 a8 g8 a8 b8 o6 c8 d8 | e4 d4 c4 o5 a4 | f8 g8 a8 f8 e8 f8 g+8 b8 | a2 r2";
static const char B1_OFF[] =
    "v6 q3 o4 " OFF("@17", "a") OFF("@17", "a") OFF("@16", "f") OFF("@16", "e")
    OFF("@17", "a") OFF("@16", "c") OFF("@16", "f") OFF("@16", "e");
static const char B1_BASS[] =
    "@6 v15 q4 [o2 a8 a8 >e8< a8]2 [o2 a8 a8 >e8< a8]2 [o2 f8 f8 >c8< f8]2 [o2 e8 e8 b8 e8]2"
    "[o2 a8 a8 >e8< a8]2 [o3 c8 c8 g8 c8]2 [o2 f8 f8 >c8< f8]2 [o2 e8 e8 b8 e8]2";
static const char B1_DRUMS[] = "[" TOM TOM2 "]8";

/* "Ash and Fern" - battles, stages 4-6: E minor, quicker */
static const char B2_LEAD[] =
    "@1 v11 q5"
    "| o5 e8 g8 b8 g8 e8 g8 b8 o6 e8 | d8 c8 o5 b8 a8 b4 g4 | c8 e8 g8 e8 c8 e8 g8 o6 c8 | o5 b8 a8 g8 f+8 g4 b4"
    "| o6 e4 d8 c8 o5 b4 a8 g8 | a8 b8 o6 c8 d8 e4 c4 | o5 d8 f+8 a8 o6 d8 c8 o5 a8 f+8 d8 | e2 r2";
static const char B2_OFF[] =
    "v6 q3 o4 " OFF("@17", "e") OFF("@16", "g") OFF("@16", "c") OFF("@16", "g")
    OFF("@17", "e") OFF("@17", "a") OFF("@16", "d") OFF("@17", "e");
static const char B2_BASS[] =
    "@6 v15 q4 [o2 e8 e8 b8 e8]2 [o2 g8 g8 >d8< g8]2 [o2 c8 c8 g8 c8]2 [o2 g8 g8 >d8< g8]2"
    "[o2 e8 e8 b8 e8]2 [o2 a8 a8 >e8< a8]2 [o2 d8 d8 a8 d8]2 [o2 e8 e8 b8 e8]2";
static const char B2_DRUMS[] = "[o2@13v13c8 o8@9v6c8 o4@11v10c8 o2@13v9c16 o2@13v9c16 o2@13v12c8 o8@9v6c8 o4@11v10c8 o8@10v6c8 ]8";

/* "Vines and Wings" - battles, stages 7-9: G minor, restless */
static const char B3_LEAD[] =
    "@14 v12 q5"
    "| o5 g8 b-8 o6 d8 g8 f8 d8 o5 b-8 o6 d8 | c8 e-8 g8 e-8 c4 o5 a4 | b-8 o6 d8 f8 d8 o5 b-8 a8 g8 f8 | f+4 a4 o6 d2"
    "| o6 g4 f8 e-8 d4 c8 o5 b-8 | a8 b-8 o6 c8 d8 e-4 c4 | o5 b-8 a8 g8 f+8 g8 a8 b-8 o6 c8 | d4 o5 g4 r2";
static const char B3_OFF[] =
    "v6 q3 o4 " OFF("@17", "g") OFF("@17", "c") OFF("@16", "b-") OFF("@16", "d")
    OFF("@17", "g") OFF("@17", "c") OFF("@16", "e-") OFF("@16", "d");
static const char B3_BASS[] =
    "@6 v15 q4 [o2 g8 g8 >d8< g8]2 [o2 c8 c8 g8 c8]2 [o2 b-8 b-8 >f8< b-8]2 [o2 d8 d8 a8 d8]2"
    "[o2 g8 g8 >d8< g8]2 [o2 c8 c8 g8 c8]2 [o2 e-8 e-8 b-8 e-8]2 [o2 d8 d8 a8 d8]2";
static const char B3_DRUMS[] = "[" TOM2 "o2@13v13c8 o4@11v9c16 o4@11v7c16 o4@11v11c8 o8@10v7c8 ]8";

/* "The Four Lords" - the last stage: C minor, heavy */
static const char LORDS_LEAD[] =
    "@23 v12 q6"
    "| o5 c4 e-4 g4 o6 c4 | o5 b-8 a-8 g4 f4 e-4 | a-4 o6 c4 e-4 d8 c8 | o5 b4 g4 d4 g4"
    "| o6 c8 d8 e-8 d8 c8 o5 b-8 a-8 g8 | a-4 f4 d4 f4 | g8 a-8 b8 o6 c8 d8 e-8 f8 d8 | c2 o5 g2";
static const char LORDS_OFF[] =
    "v6 q3 o4 " OFF("@17", "c") OFF("@17", "f") OFF("@16", "a-") OFF("@16", "g")
    OFF("@17", "c") OFF("@17", "f") OFF("@16", "g") OFF("@17", "c");
static const char LORDS_BASS[] =
    "@6 v15 q5 [o2 c8 c8 g8 c8]2 [o2 f8 f8 >c8< f8]2 [o2 a-8 a-8 >e-8< a-8]2 [o2 g8 g8 >d8< g8]2"
    "[o2 c8 c8 g8 c8]2 [o2 f8 f8 >c8< f8]2 [o2 g8 g8 >d8< g8]2 [o2 c8 c8 g8 c8]2";
static const char LORDS_DRUMS[] = "[o2@13v14c8 o2@13v10c8 o4@11v12c8 o8@9v6c8 o2@13v14c8 o8@9v6c8 o4@11v12c8 o4@11v8c16 o4@11v8c16 ]8";

/* "Embers Home" - the ending */
static const char END_LEAD[] =
    "@16 v11 q7"
    "| o5 f4 a4 o6 c4 f4 | e4. d8 c2 | o5 b-4 o6 d4 f4 d4 | c2. r4"
    "| o5 a4 o6 c4 f4 a4 | g4 f4 e4 d4 | c4 o5 b-8 a8 g4 o6 c4 | f2. r4";
static const char END_HARM[] =
    "@22 v7 q6 o4 [a8 f8 c8 f8]2 [g8 e8 c8 e8]2 [b-8 f8 d8 f8]2 [a8 f8 c8 f8]2"
    "[a8 f8 c8 f8]2 [b-8 g8 d8 g8]2 [g8 e8 c8 e8]2 [a8 f8 c8 f8]2";
static const char END_BASS[] = "@6 v14 q6 | o2 f2 >c2< | o2 c2 g2 | o2 b-2 >f2< | o2 f2 >c2< | o2 f2 a2 | o2 b-2 g2 | o2 c2 >c2< | o2 f2 >c2<";
static const char END_DRUMS[] = "[o3@13v9c4 o4@11v6c4 o3@13v8c4 o4@11v7c4]8";

/* jingles */
static const char HELD_P1[] = "@39 v11 o5 l16 d f a >d8";
static const char HELD_TRI[] = "@6 v12 o3 l8 d a >d";
static const char CLEAR_P1[] = "@23 v12 o5 l8 d f a >d4 <a8 >d8 f2";
static const char CLEAR_P2[] = "@5 v8 o4 l8 a >d f a4 f8 a8 >d2";
static const char CLEAR_TRI[] = "@6 v13 o3 l4 d a d2";
static const char FELL_P1[] = "@5 v11 o5 l8 a g f e d4 c+4 d2";
static const char FELL_TRI[] = "@6 v13 o3 l4 d <a d2.";

void fh_audio_load(void) {
    if (FH_MUS_TITLE >= 0) return;
    FH_MUS_TITLE = song_define("fh_title", 132, true, TITLE_LEAD, TITLE_OFF, TITLE_BASS, TITLE_DRUMS);
    FH_MUS_MAP = song_define("fh_map", 112, true, MAP_LEAD, MAP_OFF, MAP_BASS, MAP_DRUMS);
    FH_MUS_VILLAGE = song_define("fh_village", 104, true, VIL_LEAD, VIL_OFF, VIL_BASS, VIL_DRUMS);
    FH_MUS_BATTLE1 = song_define("fh_battle1", 144, true, B1_LEAD, B1_OFF, B1_BASS, B1_DRUMS);
    FH_MUS_BATTLE2 = song_define("fh_battle2", 156, true, B2_LEAD, B2_OFF, B2_BASS, B2_DRUMS);
    FH_MUS_BATTLE3 = song_define("fh_battle3", 162, true, B3_LEAD, B3_OFF, B3_BASS, B3_DRUMS);
    FH_MUS_LORDS = song_define("fh_lords", 138, true, LORDS_LEAD, LORDS_OFF, LORDS_BASS, LORDS_DRUMS);
    FH_MUS_END = song_define("fh_end", 100, true, END_LEAD, END_HARM, END_BASS, END_DRUMS);
    FH_MUS_WAVE = song_define("fh_wave", 150, false, HELD_P1, "", HELD_TRI, "");
    FH_MUS_CLEAR = song_define("fh_clear", 140, false, CLEAR_P1, CLEAR_P2, CLEAR_TRI, "");
    FH_MUS_FELL = song_define("fh_fell", 100, false, FELL_P1, "", FELL_TRI, "");

    sfx_define("fh_throw", CH_NOISE, 240, "@36 v7 o7 c32");
    sfx_define("fh_hit", CH_NOISE, 240, "@9 v8 o6 c32");
    sfx_define("fh_kill", CH_NOISE, 200, "@36 v11 o6 c16 @34 v9 o4 c16");
    sfx_define("fh_boss_kill", CH_NOISE, 120, "@34 v15 o3 c4");
    sfx_define("fh_cave", CH_P2, 180, "@37 v12 o3 c8 <g8");
    sfx_define("fh_ouch", CH_P2, 200, "@37 v12 o5 e16 c16 <g8");
    sfx_define("fh_build", CH_P2, 240, "@35 v11 o5 c16 g16");
    sfx_define("fh_sell", CH_P2, 240, "@35 v11 o6 c16 <g16 e16");
    sfx_define("fh_dig", CH_NOISE, 200, "@40 v11 o5 c8");
    sfx_define("fh_nope", CH_P2, 200, "@37 v10 o3 c16 r32 c16");
    sfx_define("fh_up", CH_P2, 240, "@39 v11 o5 l32 c e g >c e g");
    sfx_define("fh_horn", CH_P1, 120, "@23 v13 o4 d4 ^8 a8 >d4");
    sfx_define("fh_cook", CH_P2, 240, "@39 v9 o6 l32 c g >c");
    sfx_define("fh_talk", CH_P2, 240, "@20 v8 o6 c32 e32");
    sfx_define("fh_step", CH_P2, 240, "@42 v6 o5 c32");
}
