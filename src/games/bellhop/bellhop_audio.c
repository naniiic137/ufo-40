/* BELLHOP - original music (UFO-MML) and sound effects, written for UFO 40.
 * Every looping tune is eight bars of 4/4 on each channel. */
#include "bellhop.h"

int BHP_MUS_TITLE = -1, BHP_MUS_WORLD[BHP_WORLDS], BHP_MUS_BONUS, BHP_MUS_BOSS, BHP_MUS_LADY, BHP_MUS_END,
    BHP_MUS_CLEAR, BHP_MUS_OVER, BHP_MUS_TEA;

/* "Bellhop" - the title, rung on bells in G. G Am G C | G C D G */
static const char TITLE_LEAD[] =
    "@15 v12 q6"
    "| o6 g8 b8 o7 d8 g8 f+8 d8 o6 b8 g8 | a8 o7 c8 e8 a8 g8 e8 c8 o6 a8"
    "| b8 o7 d8 g8 b8 a8 g8 f+8 d8 | e4 d4 c4 o6 b4"
    "| o6 g8 b8 o7 d8 g8 b8 a8 g8 d8 | e8 g8 o8 c8 o7 b8 a8 g8 e8 c8"
    "| d8 f+8 a8 o8 c8 o7 b8 a8 f+8 d8 | g2 d4 o6 g4";
static const char TITLE_ARP[] =
    "v6 q3 @16 o4 [r8 g8]4 @17 [r8 a8]4 @16 [r8 g8]4 [r8 c8]4 [r8 g8]4 [r8 c8]4 [r8 d8]4 [r8 g8]4";
static const char TITLE_BASS[] =
    "@6 v14 q5 [o2 g4 o3 d4]2 [o2 a4 o3 e4]2 [o2 g4 o3 d4]2 [o3 c4 g4]2 [o2 g4 o3 d4]2 [o3 c4 g4]2 [o2 d4 a4]2 [o2 g4 o3 d4]2";
static const char TITLE_DRUMS[] = "[o2@13v11c8 o8@9v5c8 o6@11v9c8 o8@9v5c8 ]16";

/* "Millbrook" - world A, a stroll by the millpond in F. F C F C | F Bb C F */
static const char MILL_LEAD[] =
    "@14 v12 q6"
    "| o5 c4 f8 g8 a4 f4 | g8 a8 b-8 a8 g4 c4 | a4 o6 c8 d8 c4 o5 a4 | g2. r4"
    "| f4 a8 b-8 o6 c4 o5 a4 | b-8 o6 c8 d8 c8 o5 b-4 g4 | a8 g8 f8 e8 g4 e4 | f2. r4";
static const char MILL_COMP[] =
    "v6 q3 @22 o4 [f8 a8 o5 c8 o4 a8]2 [e8 g8 o5 c8 o4 g8]2 [f8 a8 o5 c8 o4 a8]2 [e8 g8 o5 c8 o4 g8]2"
    " [f8 a8 o5 c8 o4 a8]2 [d8 f8 b-8 f8]2 [e8 g8 o5 c8 o4 g8]2 [f8 a8 o5 c8 o4 a8]2";
static const char MILL_BASS[] =
    "@6 v14 q6 [o2 f4 o3 c4]2 [o2 c4 g4]2 [o2 f4 o3 c4]2 [o2 c4 g4]2 [o2 f4 o3 c4]2 [o2 b-4 o3 f4]2 [o2 c4 g4]2 [o2 f4 o3 c4]2";
static const char MILL_DRUMS[] = "[o2@13v10c4 o8@9v5c8 o8@9v4c8 o6@11v8c4 o8@9v5c8 o8@9v4c8 ]8";

/* "Orchard Hop" - world B, a skip through the trees in D. D Em D G-A | D A G D */
static const char ORCH_LEAD[] =
    "@3 v12 q6"
    "| o5 d8 r8 f+8 a8 r8 f+8 d8 r8 | e8 r8 g8 b8 r8 g8 e8 r8 | f+8 a8 o6 d8 r8 c+8 r8 o5 a8 r8 | b8 a8 g8 f+8 e4 r4"
    "| d8 r8 f+8 a8 r8 o6 d8 f+8 r8 | e8 d8 c+8 o5 b8 a4 g4 | f+8 a8 g8 b8 a8 o6 c+8 e8 c+8 | d4 o5 a4 d4 r4";
static const char ORCH_COMP[] =
    "v6 q2 o4 @16 [d8 d8]4 @17 [e8 e8]4 @16 [d8 d8]4 [g8 g8]2 [a8 a8]2 [d8 d8]4 [a8 a8]4 [g8 g8]4 [d8 d8]4";
static const char ORCH_BASS[] =
    "@6 v15 q4 [o2 d8 o3 d8]4 [o2 e8 o3 e8]4 [o2 d8 o3 d8]4 [o2 g8 o3 g8]2 [o2 a8 o3 a8]2"
    " [o2 d8 o3 d8]4 [o2 a8 o3 a8]4 [o2 g8 o3 g8]4 [o2 d8 o3 d8]4";
static const char ORCH_DRUMS[] = "[o2@13v12c8 o8@9v6c8 o6@11v10c8 o8@9v6c8 ]16";

/* "Tick-Tock Works" - world C, wound up in A minor. Am E Am E | F G E Am */
static const char CLOCK_LEAD[] =
    "@2 v12 q5"
    "| o5 a8 e8 a8 o6 c8 o5 b8 e8 b8 o6 d8 | c8 o5 a8 o6 c8 e8 d8 o5 b8 g+8 e8"
    "| a8 e8 a8 o6 c8 o5 b8 e8 b8 o6 d8 | e4 d4 c4 o5 b4"
    "| f8 a8 o6 c8 f8 e8 c8 o5 a8 f8 | g8 b8 o6 d8 g8 f8 d8 o5 b8 g8"
    "| e8 a8 o6 c8 e8 d8 o5 b8 g+8 b8 | a2 e4 a4";
static const char CLOCK_COMP[] =
    "v6 q2 @17 o4 [a8 r8]4 @16 [e8 r8]4 @17 [a8 r8]4 @16 [e8 r8]4 [f8 r8]4 [g8 r8]4 [e8 r8]4 @17 [a8 r8]4";
static const char CLOCK_BASS[] =
    "@6 v15 q3 [o2 a8 a8 o3 a8 o2 a8]2 [o2 e8 e8 o3 e8 o2 e8]2 [o2 a8 a8 o3 a8 o2 a8]2 [o2 e8 e8 o3 e8 o2 e8]2"
    " [o2 f8 f8 o3 f8 o2 f8]2 [o2 g8 g8 o3 g8 o2 g8]2 [o2 e8 e8 o3 e8 o2 e8]2 [o2 a8 a8 o3 a8 o2 a8]2";
static const char CLOCK_DRUMS[] = "[o8@21v8c8 o6@11v9c8 o8@21v6c8 o2@13v11c8 ]16";

/* "Sugarworks" - world D, sweet and round in C. C Dm C G | Am G F C */
static const char SUGAR_LEAD[] =
    "@0 v12 q6"
    "| o5 e8 g8 o6 c4 o5 b8 a8 g4 | f8 a8 o6 d4 c8 o5 b8 a4 | g8 e8 g8 o6 c8 e8 d8 c8 o5 a8 | g2 r4 g4"
    "| o6 c8 d8 e4 d8 c8 o5 a4 | b8 o6 c8 d4 c8 o5 b8 g4 | a8 b8 o6 c8 d8 e8 g8 f8 d8 | c2. r4";
static const char SUGAR_COMP[] =
    "v6 q4 @22 o4 [c8 e8 g8 e8]2 [d8 f8 a8 f8]2 [c8 e8 g8 e8]2 [o3 b8 o4 d8 g8 d8]2"
    " [c8 e8 a8 e8]2 [o3 b8 o4 d8 g8 d8]2 [c8 f8 a8 f8]2 [c8 e8 g8 e8]2";
static const char SUGAR_BASS[] =
    "@6 v14 q6 [o2 c4 g4]2 [o2 d4 a4]2 [o2 c4 g4]2 [o2 g4 o3 d4]2 [o2 a4 o3 e4]2 [o2 g4 o3 d4]2 [o2 f4 o3 c4]2 [o2 c4 g4]2";
static const char SUGAR_DRUMS[] = "[o2@13v10c8 o8@9v5c8 o8@10v6c8 o8@9v5c8 o6@11v9c8 o8@9v5c8 o8@10v6c8 o8@9v5c8 ]8";

/* "Hush Citadel" - world E, on tiptoe in D minor. Dm A Dm A | Dm Bb Gm-A Dm */
static const char HUSH_LEAD[] =
    "@1 v12 q5"
    "| o5 d4 f8 e8 d8 c+8 d4 | a4 g8 f8 e8 d8 e4 | f4 a8 g8 f8 e8 f8 g8 | a2 g4 c+4"
    "| d4 f8 e8 d8 c+8 d4 | b-4 a8 g8 f8 e8 f4 | g8 a8 b-8 a8 g8 f8 e8 c+8 | d2. r4";
static const char HUSH_COMP[] =
    "v6 q2 @17 o4 [d16 d16 d8]4 @16 [a16 a16 a8]4 @17 [d16 d16 d8]4 @16 [a16 a16 a8]4"
    " @17 [d16 d16 d8]4 @16 o3 [b-16 b-16 b-8]4 o4 @17 [g16 g16 g8]2 @16 [a16 a16 a8]2 @17 [d16 d16 d8]4";
static const char HUSH_BASS[] =
    "@6 v15 q4 [o2 d8 a8 o3 d8 o2 a8]2 [o2 a8 o3 e8 a8 e8]2 [o2 d8 a8 o3 d8 o2 a8]2 [o2 a8 o3 e8 a8 e8]2"
    " [o2 d8 a8 o3 d8 o2 a8]2 [o2 b-8 o3 f8 b-8 f8]2 o2 g8 o3 d8 g8 d8 o2 a8 o3 e8 a8 e8 [o2 d8 a8 o3 d8 o2 a8]2";
static const char HUSH_DRUMS[] = "[o2@13v12c8 o8@9v6c16 o8@9v6c16 o6@11v10c8 o2@13v9c8 ]16";

/* "Crystal Room" - the bonus rooms, all sparkle in E. E F#m G#m A-B (twice) */
static const char BONUS_LEAD[] =
    "@18 v11 q4 [o5 e8 g+8 b8 o6 e8 o5 b8 g+8 e8 g+8 | f+8 a8 o6 c+8 f+8 c+8 o5 a8 f+8 a8"
    "| g+8 b8 o6 e8 g+8 e8 o5 b8 g+8 b8 | a8 o6 c+8 e8 a8 g+8 e8 o5 b8 g+8 ]2";
static const char BONUS_COMP[] = "v6 q2 [@16 o4 [e8 e8]4 @17 [f+8 f+8]4 [g+8 g+8]4 @16 [a8 a8]2 [b8 b8]2]2";
static const char BONUS_BASS[] =
    "@6 v14 q4 [[o2 e8 o3 e8]4 [o2 f+8 o3 f+8]4 [o2 g+8 o3 g+8]4 [o2 a8 o3 a8]2 [o2 b8 o3 b8]2]2";
static const char BONUS_DRUMS[] = "[o2@13v12c8 o8@9v6c8 o6@11v10c8 o8@9v6c8 ]16";

/* "Big Machinery" - the first four bosses, grinding in C minor */
static const char BOSS_LEAD[] =
    "@23 v12 q5"
    "| o5 c8 c8 e-8 c8 g8 c8 f+8 g8 | c8 c8 e-8 c8 b-8 a-8 g8 f8 | e-8 e-8 g8 e-8 o6 c8 o5 b-8 a-8 g8 | f8 g8 a-8 b-8 b4 g4"
    "| c8 c8 e-8 c8 g8 c8 f+8 g8 | c8 c8 e-8 c8 b-8 a-8 g8 f8 | a-8 a-8 o6 c8 o5 a-8 g8 f8 e-8 d8 | c2 g4 c4";
static const char BOSS_COMP[] =
    "v6 q2 @17 o4 [c16 c16 c8]4 [c16 c16 c8]4 [c16 c16 c8]4 @16 [f16 f16 f8]2 [g16 g16 g8]2"
    " @17 [c16 c16 c8]4 [c16 c16 c8]4 @16 [a-16 a-16 a-8]2 [g16 g16 g8]2 @17 [c16 c16 c8]4";
static const char BOSS_BASS[] =
    "@6 v15 q3 [[o2 c8 o3 c8]4]3 [o2 f8 o3 f8]2 [o2 g8 o3 g8]2 [[o2 c8 o3 c8]4]2 [o2 a-8 o3 a-8]2 [o2 g8 o3 g8]2 [o2 c8 o3 c8]4";
static const char BOSS_DRUMS[] = "[o2@13v13c8 o2@13v9c8 o6@11v11c8 o8@9v6c8 ]16";

/* "Lady Hush" - the last fight, in D minor */
static const char LADY_LEAD[] =
    "@14 v12 q6"
    "| o5 d8 a8 o6 d8 o5 a8 b-8 a8 g8 a8 | f8 e8 d8 e8 f4 a4 | g8 f8 e8 f8 g4 b-4 | a8 g8 f8 e8 c+4 e4"
    "| d8 a8 o6 d8 f8 e8 d8 c+8 d8 | o5 b-8 o6 c8 d8 o5 b-8 g4 o6 d4 | c+8 d8 e8 f8 e8 d8 c+8 o5 a8 | d2 r4 a4";
static const char LADY_COMP[] =
    "v6 q2 @17 o4 [d16 d16 d8]4 [d16 d16 d8]4 [g16 g16 g8]4 @16 [a16 a16 a8]4"
    " @17 [d16 d16 d8]4 @16 o3 [b-16 b-16 b-8]4 o4 [a16 a16 a8]4 @17 [d16 d16 d8]4";
static const char LADY_BASS[] =
    "@6 v15 q3 [o2 d8 d8 o3 d8 o2 d8]4 [o2 g8 g8 o3 g8 o2 g8]2 [o2 a8 a8 o3 a8 o2 a8]2"
    " [o2 d8 d8 o3 d8 o2 d8]2 [o2 b-8 b-8 o3 b-8 o2 b-8]2 [o2 a8 a8 o3 a8 o2 a8]2 [o2 d8 d8 o3 d8 o2 d8]2";
static const char LADY_DRUMS[] = "[o2@13v13c8 o8@9v6c16 o8@9v6c16 o6@11v11c8 o2@13v10c16 o2@13v8c16 ]16";

/* "The Bells Ring" - the ending and the credits, slow, in G */
static const char END_LEAD[] =
    "@15 v12 q7"
    "| o5 g4 b4 o6 d4 g4 | f+4 d4 o5 a2 | b4 o6 d4 g4 b4 | a2 g2"
    "| e4 g4 b4 a4 | g4 f+4 e4 d4 | c4 e4 d4 f+4 | g1";
static const char END_COMP[] =
    "v5 q4 @22 o4 [g8 b8 o5 d8 o4 b8]2 [d8 f+8 a8 f+8]2 [g8 b8 o5 d8 o4 b8]2 [d8 f+8 a8 f+8]2"
    " [c8 e8 g8 e8]2 [o3 b8 o4 d8 g8 d8]2 [c8 e8 a8 e8]2 [g8 b8 o5 d8 o4 b8]2";
static const char END_BASS[] =
    "@6 v13 q7 o2 g2 d2 | o2 d2 a2 | o2 g2 d2 | o2 d2 o3 d2 | o2 c2 g2 | o2 e2 b2 | o2 a2 d2 | o2 g1";
static const char END_DRUMS[] = "[o8@9v4c8 o8@9v3c8 o6@11v5c8 o8@9v3c8 ]16";

/* jingles: a stage cleared, the last ship lost, a world's tea card */
static const char CLEAR_P1[] = "@39 v12 o5 c16 e16 g16 o6 c16 e8 g8";
static const char CLEAR_TRI[] = "@6 v13 o3 c8 g8 o4 c8";
static const char OVER_P1[] = "@5 v11 o5 e8 d8 c8 o4 g4 c2";
static const char OVER_TRI[] = "@6 v13 o3 c4 o2 g4 c2";
static const char TEA_P1[] = "@15 v12 o5 g8 b8 o6 d8 g4 r8 d8 g2";
static const char TEA_P2[] = "@2 v7 o4 g8 b8 o5 d8 g4 r8 d8 g2";
static const char TEA_TRI[] = "@6 v13 o3 g4 d4 g2";

void bhp_audio_load(void) {
    if (BHP_MUS_TITLE >= 0) return;
    BHP_MUS_TITLE = song_define("bhp_title", 140, true, TITLE_LEAD, TITLE_ARP, TITLE_BASS, TITLE_DRUMS);
    BHP_MUS_WORLD[0] = song_define("bhp_mill", 128, true, MILL_LEAD, MILL_COMP, MILL_BASS, MILL_DRUMS);
    BHP_MUS_WORLD[1] = song_define("bhp_orchard", 150, true, ORCH_LEAD, ORCH_COMP, ORCH_BASS, ORCH_DRUMS);
    BHP_MUS_WORLD[2] = song_define("bhp_clock", 140, true, CLOCK_LEAD, CLOCK_COMP, CLOCK_BASS, CLOCK_DRUMS);
    BHP_MUS_WORLD[3] = song_define("bhp_sugar", 128, true, SUGAR_LEAD, SUGAR_COMP, SUGAR_BASS, SUGAR_DRUMS);
    BHP_MUS_WORLD[4] = song_define("bhp_hush", 150, true, HUSH_LEAD, HUSH_COMP, HUSH_BASS, HUSH_DRUMS);
    BHP_MUS_BONUS = song_define("bhp_bonus", 168, true, BONUS_LEAD, BONUS_COMP, BONUS_BASS, BONUS_DRUMS);
    BHP_MUS_BOSS = song_define("bhp_boss", 164, true, BOSS_LEAD, BOSS_COMP, BOSS_BASS, BOSS_DRUMS);
    BHP_MUS_LADY = song_define("bhp_lady", 172, true, LADY_LEAD, LADY_COMP, LADY_BASS, LADY_DRUMS);
    BHP_MUS_END = song_define("bhp_end", 96, true, END_LEAD, END_COMP, END_BASS, END_DRUMS);
    BHP_MUS_CLEAR = song_define("bhp_clear", 150, false, CLEAR_P1, "", CLEAR_TRI, "");
    BHP_MUS_OVER = song_define("bhp_over", 110, false, OVER_P1, "", OVER_TRI, "");
    BHP_MUS_TEA = song_define("bhp_tea", 120, false, TEA_P1, TEA_P2, TEA_TRI, "");

    sfx_define("bhp_slash", CH_NOISE, 220, "@36 v11 o6 c16");
    sfx_define("bhp_crash", CH_NOISE, 140, "@34 v15 o3 c4");
    sfx_define("bhp_break", CH_NOISE, 200, "@40 v11 o5 c16");
    sfx_define("bhp_kill", CH_P2, 220, "@33 v11 o5 g16 c16");
    sfx_define("bhp_show", CH_P2, 200, "@39 v11 o6 e16 g16 o7 c16");
    sfx_define("bhp_cup", CH_P2, 180, "@15 v12 o6 c16 e16 g16 o7 c8");
    sfx_define("bhp_coin", CH_P2, 240, "@35 v11 o6 e16 b16");
    sfx_define("bhp_bigcoin", CH_P2, 200, "@39 v12 o5 c16 g16 o6 c16");
    sfx_define("bhp_node", CH_P2, 240, "@35 v9 o6 g32");
    sfx_define("bhp_circler", CH_P2, 200, "@39 v12 o6 c16 e16 g16 o7 c16");
    sfx_define("bhp_fuel", CH_P2, 200, "@32 v10 o4 c16 g16 o5 c16");
    sfx_define("bhp_lever", CH_NOISE, 200, "@21 v12 o5 c16 @40 v8 o4 c16");
    sfx_define("bhp_blast", CH_NOISE, 150, "@34 v14 o4 c8");
    sfx_define("bhp_shoot", CH_NOISE, 240, "@21 v7 o6 c32");
    sfx_define("bhp_pop", CH_P2, 220, "@32 v10 o5 c16 g16");
    sfx_define("bhp_crystal", CH_P2, 240, "@39 v11 o6 b16 o7 e16");
    sfx_define("bhp_round", CH_P2, 200, "@39 v10 o5 e16 g+16 b16");
    sfx_define("bhp_hit", CH_P2, 220, "@37 v11 o4 g16");
    sfx_define("bhp_bosshit", CH_NOISE, 180, "@40 v13 o4 c8");
    sfx_define("bhp_open", CH_P2, 180, "@39 v12 o5 c16 g16 o6 c16 g16");
    sfx_define("bhp_warp", CH_P2, 220, "@32 v11 o4 c16 g16 o5 c16 g16 o6 c16");
    sfx_define("bhp_oneup", CH_P1, 180, "@39 v12 o6 c16 e16 g16 o7 c16 e16 g8");
    sfx_define("bhp_dry", CH_P2, 220, "@42 v7 o3 c32");
    sfx_define("bhp_move", CH_P2, 220, "@42 v9 o6 c32");
    sfx_define("bhp_fuse", CH_NOISE, 240, "@21 v8 o7 c32 c32");
}
