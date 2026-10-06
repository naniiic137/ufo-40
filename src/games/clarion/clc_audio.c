/* CLARION CALL - original music (UFO-MML) and sound effects, written for
 * UFO 40. Every looping tune is eight bars of 4/4 on each channel. */
#include "clc.h"

int CLC_MUS_TITLE = -1, CLC_MUS_REGION[RG_COUNT], CLC_MUS_CAVE, CLC_MUS_DASH, CLC_MUS_BOSS, CLC_MUS_HUSH, CLC_MUS_TOCK,
    CLC_MUS_ESCAPE, CLC_MUS_SHOP, CLC_MUS_MAP, CLC_MUS_END, CLC_MUS_TRUE, CLC_MUS_SAD, CLC_MUS_OVER, CLC_MUS_CLEAR;

/* "Clarion Call" - the title: a fanfare rung on bells, in Bb. Bb Gm Eb F | Bb Eb F Bb */
static const char TITLE_LEAD[] =
    "@15 v12 q6"
    "| o5 b-8 o6 d8 f8 b-8 a8 f8 d8 f8 | g8 b-8 o7 d8 c8 o6 b-8 a8 g8 d8"
    "| e-8 g8 b-8 o7 e-8 d8 c8 o6 b-8 g8 | f4 a4 o7 c4 o6 a4"
    "| b-8 o7 d8 f8 d8 c8 o6 b-8 a8 b-8 | g8 b-8 o7 e-8 d8 c8 o6 b-8 g8 e-8"
    "| f8 a8 o7 c8 e-8 d8 c8 o6 a8 f8 | b-2 f4 b-4";
static const char TITLE_ARP[] =
    "v6 q3 @16 o4 [r8 b-8]4 @17 [r8 g8]4 @16 [r8 e-8]4 [r8 f8]4 [r8 b-8]4 [r8 e-8]4 [r8 f8]4 [r8 b-8]4";
static const char TITLE_BASS[] =
    "@6 v14 q5 [o2 b-4 o3 f4]2 [o2 g4 o3 d4]2 [o2 e-4 b-4]2 [o2 f4 o3 c4]2 [o2 b-4 o3 f4]2 [o2 e-4 b-4]2 [o2 f4 o3 c4]2 [o2 b-4 o3 f4]2";
static const char TITLE_DRUMS[] = "[o2@13v11c8 o8@9v5c8 o6@11v9c8 o8@9v5c8 ]16";

/* "Down in the Cellars" - the Cellars, a careful creep in E minor. Em C Am B */
static const char CELL_LEAD[] =
    "@5 v12 q6"
    "| o5 e4 g8 f+8 e4 b4 | c4 e8 d8 c4 g4 | a4 o6 c8 o5 b8 a4 e4 | f+2 d+4 f+4"
    "| e4 g8 a8 b4 o6 e4 | o5 c8 d8 e8 g8 f+4 e4 | a8 g8 f+8 e8 f+4 d+4 | e2. r4";
static const char CELL_COMP[] =
    "v6 q2 @17 o4 [e16 r16 b8]4 @16 [c16 r16 g8]4 @17 [a16 r16 e8]4 @16 [b16 r16 f+8]4"
    " @17 [e16 r16 b8]4 @16 [c16 r16 g8]4 @17 [a16 r16 e8]2 @16 [b16 r16 f+8]2 @17 [e16 r16 b8]4";
static const char CELL_BASS[] =
    "@6 v14 q4 [o2 e8 r8 e8 r8]2 [o2 c8 r8 c8 r8]2 [o2 a8 r8 a8 r8]2 [o2 b8 r8 b8 r8]2"
    " [o2 e8 r8 e8 r8]2 [o2 c8 r8 c8 r8]2 o2 a8 r8 a8 r8 b8 r8 b8 r8 [o2 e8 r8 e8 r8]2";
static const char CELL_DRUMS[] = "[o2@13v11c4 o8@9v4c8 o8@9v3c8 o6@11v8c4 o8@9v4c8 o8@9v3c8 ]8";

/* "Under the Boughs" - the Arboretum, green and climbing, in G. G D Em C */
static const char ARBOR_LEAD[] =
    "@14 v12 q6"
    "| o5 g8 a8 b8 o6 d8 c8 o5 b8 a8 g8 | f+8 g8 a8 b8 a4 d4 | e8 f+8 g8 b8 a8 g8 f+8 e8 | c4 e4 g4 e4"
    "| g8 a8 b8 o6 d8 e8 d8 c8 o5 b8 | a8 b8 o6 c8 d8 o5 a4 f+4 | g8 f+8 e8 d8 e8 g8 c8 e8 | d4 b4 g2";
static const char ARBOR_COMP[] =
    "v6 q4 @22 o4 [g8 b8 o5 d8 o4 b8]2 [f+8 a8 o5 d8 o4 a8]2 [e8 g8 b8 g8]2 [e8 g8 o5 c8 o4 g8]2"
    " [g8 b8 o5 d8 o4 b8]2 [f+8 a8 o5 d8 o4 a8]2 [e8 g8 o5 c8 o4 g8]2 [g8 b8 o5 d8 o4 b8]2";
static const char ARBOR_BASS[] =
    "@6 v14 q6 [o2 g4 o3 d4]2 [o2 d4 a4]2 [o2 e4 b4]2 [o2 c4 g4]2 [o2 g4 o3 d4]2 [o2 d4 a4]2 [o2 c4 g4]2 [o2 g4 o3 d4]2";
static const char ARBOR_DRUMS[] = "[o2@13v10c8 o8@9v5c8 o8@10v6c8 o8@9v5c8 o6@11v9c8 o8@9v5c8 o8@10v6c8 o8@9v5c8 ]8";

/* "Thin Ice" - the Icehouse, glassy and quick, in D. D Bm G A */
static const char ICE_LEAD[] =
    "@18 v11 q4"
    "| o6 d8 f+8 a8 f+8 d8 f+8 a8 o7 d8 | o6 b8 d8 f+8 d8 o5 b8 o6 d8 f+8 b8 | g8 b8 o7 d8 o6 b8 g8 b8 o7 d8 g8 | e8 c+8 o6 a8 e8 c+8 e8 a8 o7 c+8"
    "| o6 d8 f+8 a8 o7 d8 c+8 o6 a8 f+8 a8 | b8 a8 f+8 d8 b8 o7 d8 f+8 d8 | o6 g8 a8 b8 o7 d8 e8 d8 c+8 o6 a8 | d2 r2";
static const char ICE_COMP[] = "v6 q2 @16 o4 [d8 d8]4 @17 [b8 b8]4 @16 [g8 g8]4 [a8 a8]4 [d8 d8]4 @17 [b8 b8]4 @16 [g8 g8]2 [a8 a8]2 [d8 d8]4";
static const char ICE_BASS[] =
    "@6 v14 q4 [o2 d8 o3 d8]4 [o2 b8 o3 b8]4 [o2 g8 o3 g8]4 [o2 a8 o3 a8]4 [o2 d8 o3 d8]4 [o2 b8 o3 b8]4 [o2 g8 o3 g8]2 [o2 a8 o3 a8]2 [o2 d8 o3 d8]4";
static const char ICE_DRUMS[] = "[o2@13v11c8 o8@21v6c8 o6@11v9c8 o8@21v6c8 ]16";

/* "Something's Digesting" - the Gullet, a wet waltz in 4 in F minor. Fm Db C Fm */
static const char GUT_LEAD[] =
    "@1 v12 q5"
    "| o5 f4 a-8 g8 f8 e8 f4 | d-4 f8 e-8 d-8 c8 d-4 | c4 e8 f8 g4 b-4 | a-2 g4 c4"
    "| f4 a-8 b-8 o6 c4 o5 a-4 | b-8 a-8 g8 f8 d-4 f4 | e8 f8 g8 a-8 g8 f8 e8 c8 | f2. r4";
static const char GUT_COMP[] =
    "v6 q2 @17 o4 [f16 f16 r8]4 @16 [d-16 d-16 r8]4 @16 [c16 c16 r8]4 @17 [f16 f16 r8]4"
    " @17 [f16 f16 r8]4 @16 [d-16 d-16 r8]4 [c16 c16 r8]4 @17 [f16 f16 r8]4";
static const char GUT_BASS[] =
    "@6 v15 q3 [o2 f8 f8 o3 c8 o2 f8]2 [o2 d-8 d-8 a-8 d-8]2 [o2 c8 c8 g8 c8]2 [o2 f8 f8 o3 c8 o2 f8]2"
    " [o2 f8 f8 o3 c8 o2 f8]2 [o2 d-8 d-8 a-8 d-8]2 [o2 c8 c8 g8 c8]2 [o2 f8 f8 o3 c8 o2 f8]2";
static const char GUT_DRUMS[] = "[o2@13v12c8 o2@13v8c8 o6@11v8c8 o8@9v4c8 ]16";

/* "Cogtown Shift" - Cogtown, clockwork and brass, in A minor. Am F G E */
static const char COG_LEAD[] =
    "@23 v12 q5"
    "| o5 a8 a8 o6 c8 o5 a8 e8 a8 o6 c8 e8 | f8 f8 a8 f8 c8 f8 a8 o7 c8 | o6 g8 g8 b8 g8 d8 g8 b8 o7 d8 | o6 e4 g+4 b4 g+4"
    "| a8 o7 c8 o6 b8 a8 g8 e8 f8 g8 | a8 f8 e8 d8 c8 o5 a8 o6 c8 f8 | g8 b8 a8 g8 f8 d8 e8 g+8 | a2 e4 a4";
static const char COG_COMP[] =
    "v6 q2 @21 o4 [a8 r8]4 [f8 r8]4 [g8 r8]4 [e8 r8]4 [a8 r8]4 [f8 r8]4 [g8 r8]2 [e8 r8]2 [a8 r8]4";
static const char COG_BASS[] =
    "@6 v15 q3 [o2 a8 o3 a8 o2 a8 o3 a8]2 [o2 f8 o3 f8 o2 f8 o3 f8]2 [o2 g8 o3 g8 o2 g8 o3 g8]2 [o2 e8 o3 e8 o2 e8 o3 e8]2"
    " [o2 a8 o3 a8 o2 a8 o3 a8]2 [o2 f8 o3 f8 o2 f8 o3 f8]2 o2 g8 o3 g8 o2 g8 o3 g8 o2 e8 o3 e8 o2 e8 o3 e8 [o2 a8 o3 a8 o2 a8 o3 a8]2";
static const char COG_DRUMS[] = "[o8@21v8c8 o6@11v9c8 o8@21v6c8 o2@13v11c8 ]16";

/* "Candles Out" - the Cloister, hushed, in C minor. Cm Ab Fm G */
static const char CLOI_LEAD[] =
    "@19 v12 q7"
    "| o5 c2 e-4 g4 | a-2 g4 f4 | f2 a-4 o6 c4 | o5 b2 g2"
    "| c2 e-4 g4 | a-4 g4 f4 e-4 | d4 f4 e-4 d4 | c1";
static const char CLOI_COMP[] =
    "v5 q4 @4 o4 [c8 e-8 g8 e-8]2 [c8 e-8 a-8 e-8]2 [c8 f8 a-8 f8]2 [d8 g8 b8 g8]2"
    " [c8 e-8 g8 e-8]2 [c8 e-8 a-8 e-8]2 [d8 f8 a-8 f8]1 [d8 g8 b8 g8]1 [c8 e-8 g8 e-8]2";
static const char CLOI_BASS[] = "@6 v13 q7 o2 c1 | a-1 | f1 | g1 | c1 | a-1 | f2 g2 | c1";
static const char CLOI_DRUMS[] = "[o8@9v3c4 o8@9v2c4 o6@11v4c4 o8@9v2c4 ]8";

/* "Top of the Spire" - the Spire, everything at once, in E minor. Em D C B */
static const char SPIRE_LEAD[] =
    "@14 v12 q6"
    "| o5 e8 b8 o6 e8 d8 o5 b8 a8 g8 b8 | f+8 a8 o6 d8 c+8 o5 a8 g8 f+8 a8 | e8 g8 o6 c8 o5 b8 g8 f+8 e8 g8 | d+4 f+4 b4 a4"
    "| e8 b8 o6 e8 f+8 g8 f+8 e8 d8 | c8 d8 e8 d8 c8 o5 b8 a8 b8 | o6 c8 o5 b8 a8 g8 f+8 g8 a8 f+8 | e2 b4 e4";
static const char SPIRE_COMP[] =
    "v6 q2 @17 o4 [e16 e16 e8]4 @16 [d16 d16 d8]4 [c16 c16 c8]4 [b16 b16 b8]4"
    " @17 [e16 e16 e8]4 @16 [c16 c16 c8]4 [a16 a16 a8]2 [b16 b16 b8]2 @17 [e16 e16 e8]4";
static const char SPIRE_BASS[] =
    "@6 v15 q3 [o2 e8 e8 o3 e8 o2 e8]2 [o2 d8 d8 o3 d8 o2 d8]2 [o2 c8 c8 o3 c8 o2 c8]2 [o2 b8 b8 o3 b8 o2 b8]2"
    " [o2 e8 e8 o3 e8 o2 e8]2 [o2 c8 c8 o3 c8 o2 c8]2 o2 a8 a8 o3 a8 o2 a8 b8 b8 o3 b8 o2 b8 [o2 e8 e8 o3 e8 o2 e8]2";
static const char SPIRE_DRUMS[] = "[o2@13v13c8 o8@9v6c16 o8@9v6c16 o6@11v11c8 o2@13v10c16 o2@13v8c16 ]16";

/* "Underfoot" - behind the doors, a quick march in G minor. Gm Eb F D */
static const char CAVE_LEAD[] =
    "@3 v12 q5"
    "| o5 g8 r8 b-8 r8 o6 d8 r8 o5 b-8 a8 | g8 r8 e-8 r8 g8 r8 b-8 r8 | a8 r8 f8 r8 a8 r8 o6 c8 r8 | o5 f+4 a4 o6 d4 o5 a4"
    "| g8 a8 b-8 o6 c8 d8 c8 o5 b-8 a8 | b-8 r8 g8 r8 e-8 r8 g8 r8 | f8 g8 a8 b-8 a8 g8 f+8 a8 | g2 d4 g4";
static const char CAVE_COMP[] = "v6 q2 @17 o4 [g8 g8]4 @16 [e-8 e-8]4 [f8 f8]4 [d8 d8]4 @17 [g8 g8]4 @16 [e-8 e-8]4 [f8 f8]2 [d8 d8]2 @17 [g8 g8]4";
static const char CAVE_BASS[] =
    "@6 v15 q4 [o2 g8 o3 g8]4 [o2 e-8 o3 e-8]4 [o2 f8 o3 f8]4 [o2 d8 o3 d8]4 [o2 g8 o3 g8]4 [o2 e-8 o3 e-8]4 [o2 f8 o3 f8]2 [o2 d8 o3 d8]2 [o2 g8 o3 g8]4";
static const char CAVE_DRUMS[] = "[o2@13v12c8 o8@9v6c8 o6@11v10c8 o8@9v6c8 ]16";

/* "The Gold Door" - the dash, all out, in C minor */
static const char DASH_LEAD[] =
    "@1 v12 q4"
    "| o6 c16 c16 e-16 c16 g16 c16 e-16 g16 o7 c8 o6 b-8 g8 e-8 | f16 f16 a-16 f16 o7 c16 o6 f16 a-16 o7 c16 f8 e-8 c8 o6 a-8"
    "| g16 g16 b16 g16 o7 d16 o6 g16 b16 o7 d16 g8 f8 d8 o6 b8 | o7 c4 o6 g4 e-4 c4"
    "| c16 c16 e-16 c16 g16 c16 e-16 g16 o7 c8 d8 e-8 d8 | c8 o6 b-8 a-8 g8 f8 e-8 d8 c8 | o5 b8 o6 d8 f8 g8 a-8 g8 f8 d8 | c2 g4 c4";
static const char DASH_COMP[] = "v6 q2 @17 o4 [c16 c16 c8]4 @17 [f16 f16 f8]4 @16 [g16 g16 g8]4 @17 [c16 c16 c8]4 [c16 c16 c8]4 [f16 f16 f8]4 @16 [g16 g16 g8]4 @17 [c16 c16 c8]4";
static const char DASH_BASS[] =
    "@6 v15 q3 [o2 c8 o3 c8]4 [o2 f8 o3 f8]4 [o2 g8 o3 g8]4 [o2 c8 o3 c8]4 [o2 c8 o3 c8]4 [o2 f8 o3 f8]4 [o2 g8 o3 g8]4 [o2 c8 o3 c8]4";
static const char DASH_DRUMS[] = "[o2@13v13c8 o6@11v10c8 o2@13v11c8 o6@11v10c16 o6@11v8c16 ]16";

/* "The Lobber" - the mini-boss, heavy and lopsided, in D minor */
static const char BOSS_LEAD[] =
    "@23 v12 q5"
    "| o5 d8 d8 f8 d8 a8 d8 g+8 a8 | d8 d8 f8 d8 o6 c8 o5 b-8 a8 g8 | f8 f8 a8 f8 o6 d8 c8 o5 b-8 a8 | g8 a8 b-8 o6 c8 c+4 o5 a4"
    "| d8 d8 f8 d8 a8 d8 g+8 a8 | d8 d8 f8 d8 o6 c8 o5 b-8 a8 g8 | b-8 b-8 o6 d8 o5 b-8 a8 g8 f8 e8 | d2 a4 d4";
static const char BOSS_COMP[] = "v6 q2 @17 o4 [d16 d16 d8]4 [d16 d16 d8]4 [d16 d16 d8]4 @16 [a16 a16 a8]4 @17 [d16 d16 d8]4 [d16 d16 d8]4 @16 [b-16 b-16 b-8]2 [a16 a16 a8]2 @17 [d16 d16 d8]4";
static const char BOSS_BASS[] = "@6 v15 q3 [[o2 d8 o3 d8]4]3 [o2 a8 o3 a8]4 [[o2 d8 o3 d8]4]2 [o2 b-8 o3 b-8]2 [o2 a8 o3 a8]2 [o2 d8 o3 d8]4";
static const char BOSS_DRUMS[] = "[o2@13v13c8 o2@13v9c8 o6@11v11c8 o8@9v6c8 ]16";

/* "Lady Hush Returns" - her theme from BELLHOP's days, darker, in B minor */
static const char HUSH_LEAD[] =
    "@14 v12 q6"
    "| o5 b8 o6 f+8 b8 f+8 g8 f+8 e8 f+8 | d8 c+8 o5 b8 o6 c+8 d4 f+4 | e8 d8 c+8 d8 e4 g4 | f+8 e8 d8 c+8 o5 a+4 o6 c+4"
    "| o5 b8 o6 f+8 b8 o7 d8 c+8 o6 b8 a+8 b8 | g8 a8 b8 g8 e4 b4 | a+8 b8 o7 c+8 d8 c+8 o6 b8 a+8 f+8 | b2 r4 f+4";
static const char HUSH_COMP[] = "v6 q2 @17 o4 [b16 b16 b8]4 [b16 b16 b8]4 [e16 e16 e8]4 @16 [f+16 f+16 f+8]4 @17 [b16 b16 b8]4 @16 [g16 g16 g8]4 [f+16 f+16 f+8]4 @17 [b16 b16 b8]4";
static const char HUSH_BASS[] =
    "@6 v15 q3 [o2 b8 b8 o3 b8 o2 b8]4 [o2 e8 e8 o3 e8 o2 e8]2 [o2 f+8 f+8 o3 f+8 o2 f+8]2"
    " [o2 b8 b8 o3 b8 o2 b8]2 [o2 g8 g8 o3 g8 o2 g8]2 [o2 f+8 f+8 o3 f+8 o2 f+8]2 [o2 b8 b8 o3 b8 o2 b8]2";
static const char HUSH_DRUMS[] = "[o2@13v13c8 o8@9v6c16 o8@9v6c16 o6@11v11c8 o2@13v10c16 o2@13v8c16 ]16";

/* "Grandsire Tock" - the true last fight: a ticking in F# minor */
static const char TOCK_LEAD[] =
    "@1 v12 q4"
    "| o5 f+8 c+8 f+8 a8 g+8 c+8 g+8 b8 | a8 f+8 a8 o6 c+8 o5 b8 g+8 e8 g+8 | f+8 c+8 f+8 a8 b8 o6 c+8 d8 c+8 | o5 b4 g+4 e+4 c+4"
    "| f+8 a8 o6 c+8 f+8 e+8 c+8 o5 g+8 b8 | a8 o6 c+8 d8 c+8 o5 b8 a8 g+8 f+8 | d8 e8 f+8 g+8 a8 b8 o6 c+8 o5 e+8 | f+2 c+4 f+4";
static const char TOCK_COMP[] = "v6 q2 @21 o4 [f+8 r8]4 [d8 r8]4 [f+8 r8]4 [c+8 r8]4 [f+8 r8]4 [d8 r8]4 [d8 r8]2 [c+8 r8]2 [f+8 r8]4";
static const char TOCK_BASS[] =
    "@6 v15 q3 [o2 f+8 o3 f+8]4 [o2 d8 o3 d8]4 [o2 f+8 o3 f+8]4 [o2 c+8 o3 c+8]4 [o2 f+8 o3 f+8]4 [o2 d8 o3 d8]4 [o2 d8 o3 d8]2 [o2 c+8 o3 c+8]2 [o2 f+8 o3 f+8]4";
static const char TOCK_DRUMS[] = "[o8@21v9c8 o8@21v5c8 o2@13v12c8 o8@21v5c8 ]16";

/* "Ninety-Nine Seconds" - the escape, up and out, in A */
static const char ESC_LEAD[] =
    "@1 v12 q4"
    "| o5 a8 o6 c+8 e8 a8 g+8 e8 c+8 e8 | f+8 a8 o7 c+8 o6 b8 a8 f+8 d8 f+8 | e8 g+8 b8 o7 e8 d8 o6 b8 g+8 b8 | a4 e4 c+4 e4"
    "| a8 b8 o7 c+8 d8 e8 d8 c+8 o6 b8 | o7 d8 c+8 o6 b8 a8 f+8 a8 o7 d8 f+8 | e8 d8 c+8 o6 b8 a8 g+8 f+8 g+8 | a2 e4 a4";
static const char ESC_COMP[] = "v6 q2 @16 o4 [a16 a16 a8]4 [d16 d16 d8]4 [e16 e16 e8]4 [a16 a16 a8]4 [a16 a16 a8]4 [d16 d16 d8]4 [e16 e16 e8]4 [a16 a16 a8]4";
static const char ESC_BASS[] =
    "@6 v15 q3 [o2 a8 o3 a8]4 [o2 d8 o3 d8]4 [o2 e8 o3 e8]4 [o2 a8 o3 a8]4 [o2 a8 o3 a8]4 [o2 d8 o3 d8]4 [o2 e8 o3 e8]4 [o2 a8 o3 a8]4";
static const char ESC_DRUMS[] = "[o2@13v13c8 o6@11v10c8 o2@13v11c8 o6@11v10c8 ]16";

/* "One per Customer" - the shops, a shuffle in F */
static const char SHOP_LEAD[] =
    "@0 v12 q6"
    "| o5 f8 a8 o6 c4 o5 a8 f8 c4 | d8 f8 b-4 a8 g8 f4 | e8 g8 b-8 o6 c8 d8 c8 o5 b-8 g8 | a2 r4 c4"
    "| f8 a8 o6 c4 d8 c8 o5 a4 | b-8 a8 g8 f8 d4 f4 | g8 a8 b-8 g8 e8 g8 c8 e8 | f2. r4";
static const char SHOP_COMP[] = "v6 q4 @22 o4 [f8 a8 o5 c8 o4 a8]2 [d8 f8 b-8 f8]2 [e8 g8 b-8 g8]2 [f8 a8 o5 c8 o4 a8]2 [f8 a8 o5 c8 o4 a8]2 [d8 f8 b-8 f8]2 [e8 g8 o5 c8 o4 g8]2 [f8 a8 o5 c8 o4 a8]2";
static const char SHOP_BASS[] = "@6 v14 q6 [o2 f4 o3 c4]2 [o2 b-4 o3 f4]2 [o2 c4 g4]2 [o2 f4 o3 c4]2 [o2 f4 o3 c4]2 [o2 b-4 o3 f4]2 [o2 c4 g4]2 [o2 f4 o3 c4]2";
static const char SHOP_DRUMS[] = "[o2@13v10c8 o8@9v5c16 o8@9v4c16 o6@11v8c8 o8@9v5c8 ]16";

/* "The Station Map" - choosing the way, slow, in D */
static const char MAP_LEAD[] =
    "@15 v11 q7"
    "| o5 d4 f+4 a4 o6 d4 | c+2 o5 a2 | b4 o6 d4 f+4 e4 | d2 c+2"
    "| o5 g4 b4 o6 d4 g4 | f+2 d2 | e4 c+4 o5 a4 b4 | o6 d1";
static const char MAP_COMP[] = "v5 q4 @4 o4 [d8 f+8 a8 f+8]2 [c+8 e8 a8 e8]2 [d8 g8 b8 g8]2 [c+8 e8 a8 e8]2 [d8 g8 b8 g8]2 [d8 f+8 a8 f+8]2 [c+8 e8 a8 e8]2 [d8 f+8 a8 f+8]2";
static const char MAP_BASS[] = "@6 v13 q7 o2 d1 | a1 | g1 | a1 | g1 | d1 | a1 | d1";
static const char MAP_DRUMS[] = "[o8@9v3c4 o8@9v2c4 o8@9v3c4 o8@9v2c4 ]8";

/* "Home with Ansel" - the gold ending, warm, in G */
static const char END_LEAD[] =
    "@15 v12 q7"
    "| o5 b4 o6 d4 g4 b4 | a4 f+4 d2 | e4 g4 b4 o7 d4 | c2 o6 b2"
    "| a4 c4 e4 g4 | f+4 a4 d4 f+4 | g4 b4 a4 f+4 | g1";
static const char END_COMP[] = "v5 q4 @22 o4 [g8 b8 o5 d8 o4 b8]2 [d8 f+8 a8 f+8]2 [e8 g8 b8 g8]2 [c8 e8 g8 e8]2 [a8 c8 e8 c8]2 [d8 f+8 a8 f+8]2 [c8 e8 a8 e8]2 [g8 b8 o5 d8 o4 b8]2";
static const char END_BASS[] = "@6 v13 q7 o2 g2 d2 | d2 a2 | e2 b2 | c2 g2 | a2 e2 | d2 a2 | c2 d2 | g1";
static const char END_DRUMS[] = "[o8@9v4c8 o8@9v3c8 o6@11v5c8 o8@9v3c8 ]16";

/* "The Bells Ring Again" - the true ending, full peal, in C */
static const char TRUE_LEAD[] =
    "@15 v12 q6"
    "| o6 c8 e8 g8 o7 c8 o6 b8 g8 e8 g8 | a8 o7 c8 e8 c8 o6 b8 a8 g8 f8 | e8 g8 o7 c8 e8 d8 c8 o6 b8 a8 | g4 b4 o7 d4 o6 b4"
    "| o7 c8 o6 b8 a8 g8 f8 e8 d8 c8 | f8 a8 o7 c8 o6 a8 g8 b8 o7 d8 o6 b8 | o7 c8 e8 d8 c8 o6 b8 g8 a8 b8 | o7 c2 g4 c4";
static const char TRUE_COMP[] = "v6 q3 @16 o4 [r8 c8]4 [r8 f8]4 [r8 c8]4 [r8 g8]4 [r8 c8]4 [r8 f8]2 [r8 g8]2 [r8 c8]2 [r8 g8]2 [r8 c8]4";
static const char TRUE_BASS[] = "@6 v14 q5 [o2 c4 g4]2 [o2 f4 o3 c4]2 [o2 c4 g4]2 [o2 g4 o3 d4]2 [o2 c4 g4]2 o2 f4 o3 c4 o2 g4 o3 d4 o2 c4 g4 o2 g4 o3 d4 [o2 c4 g4]2";
static const char TRUE_DRUMS[] = "[o2@13v11c8 o8@9v5c8 o6@11v9c8 o8@9v5c8 ]16";

/* "Home Alone" - the bad ending, wistful, in A minor */
static const char SAD_LEAD[] =
    "@5 v11 q7"
    "| o5 a2 o6 c4 o5 b4 | a2 e2 | f2 a4 g4 | e1"
    "| d2 f4 e4 | c2 o4 a2 | b2 o5 d4 c4 | o4 a1";
static const char SAD_COMP[] = "v5 q4 @4 o4 [a8 o5 c8 e8 c8]2 [o4 a8 o5 c8 e8 c8]2 [o4 f8 a8 o5 c8 o4 a8]2 [e8 g+8 b8 g+8]2 [d8 f8 a8 f8]2 [c8 e8 a8 e8]2 [d8 g+8 b8 g+8]2 [c8 e8 a8 e8]2";
static const char SAD_BASS[] = "@6 v12 q7 o2 a1 | a1 | f1 | e1 | d1 | a1 | e1 | a1";
static const char SAD_DRUMS[] = "[o8@9v2c4 o8@9v2c4 o8@9v2c4 o8@9v2c4 ]8";

/* jingles: an engine broken, the run lost */
static const char CLEAR_P1[] = "@39 v12 o5 c16 e16 g16 o6 c16 e8 g8 o7 c4";
static const char CLEAR_TRI[] = "@6 v13 o3 c8 g8 o4 c4 r8";
static const char OVER_P1[] = "@5 v11 o5 e8 d8 c8 o4 b8 a4 e2";
static const char OVER_TRI[] = "@6 v13 o3 a4 e4 o2 a2";

void clc_audio_load(void) {
    if (CLC_MUS_TITLE >= 0) return;
    CLC_MUS_TITLE = song_define("clc_title", 140, true, TITLE_LEAD, TITLE_ARP, TITLE_BASS, TITLE_DRUMS);
    CLC_MUS_REGION[RG_CELLARS] = song_define("clc_cellars", 112, true, CELL_LEAD, CELL_COMP, CELL_BASS, CELL_DRUMS);
    CLC_MUS_REGION[RG_ARBOR] = song_define("clc_arbor", 136, true, ARBOR_LEAD, ARBOR_COMP, ARBOR_BASS, ARBOR_DRUMS);
    CLC_MUS_REGION[RG_ICE] = song_define("clc_ice", 150, true, ICE_LEAD, ICE_COMP, ICE_BASS, ICE_DRUMS);
    CLC_MUS_REGION[RG_GULLET] = song_define("clc_gullet", 118, true, GUT_LEAD, GUT_COMP, GUT_BASS, GUT_DRUMS);
    CLC_MUS_REGION[RG_COG] = song_define("clc_cog", 144, true, COG_LEAD, COG_COMP, COG_BASS, COG_DRUMS);
    CLC_MUS_REGION[RG_CLOISTER] = song_define("clc_cloister", 84, true, CLOI_LEAD, CLOI_COMP, CLOI_BASS, CLOI_DRUMS);
    CLC_MUS_REGION[RG_SPIRE] = song_define("clc_spire", 156, true, SPIRE_LEAD, SPIRE_COMP, SPIRE_BASS, SPIRE_DRUMS);
    CLC_MUS_CAVE = song_define("clc_cave", 150, true, CAVE_LEAD, CAVE_COMP, CAVE_BASS, CAVE_DRUMS);
    CLC_MUS_DASH = song_define("clc_dash", 176, true, DASH_LEAD, DASH_COMP, DASH_BASS, DASH_DRUMS);
    CLC_MUS_BOSS = song_define("clc_boss", 160, true, BOSS_LEAD, BOSS_COMP, BOSS_BASS, BOSS_DRUMS);
    CLC_MUS_HUSH = song_define("clc_hush", 168, true, HUSH_LEAD, HUSH_COMP, HUSH_BASS, HUSH_DRUMS);
    CLC_MUS_TOCK = song_define("clc_tock", 172, true, TOCK_LEAD, TOCK_COMP, TOCK_BASS, TOCK_DRUMS);
    CLC_MUS_ESCAPE = song_define("clc_escape", 180, true, ESC_LEAD, ESC_COMP, ESC_BASS, ESC_DRUMS);
    CLC_MUS_SHOP = song_define("clc_shop", 120, true, SHOP_LEAD, SHOP_COMP, SHOP_BASS, SHOP_DRUMS);
    CLC_MUS_MAP = song_define("clc_map", 90, true, MAP_LEAD, MAP_COMP, MAP_BASS, MAP_DRUMS);
    CLC_MUS_END = song_define("clc_end", 96, true, END_LEAD, END_COMP, END_BASS, END_DRUMS);
    CLC_MUS_TRUE = song_define("clc_true", 132, true, TRUE_LEAD, TRUE_COMP, TRUE_BASS, TRUE_DRUMS);
    CLC_MUS_SAD = song_define("clc_sad", 80, true, SAD_LEAD, SAD_COMP, SAD_BASS, SAD_DRUMS);
    CLC_MUS_CLEAR = song_define("clc_clear", 150, false, CLEAR_P1, "", CLEAR_TRI, "");
    CLC_MUS_OVER = song_define("clc_over", 100, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("clc_slash", CH_NOISE, 220, "@36 v11 o6 c16");
    sfx_define("clc_shoot", CH_NOISE, 240, "@21 v7 o7 c32");
    sfx_define("clc_hit", CH_P2, 240, "@37 v9 o5 c32");
    sfx_define("clc_kill", CH_P2, 220, "@33 v11 o5 g16 c16");
    sfx_define("clc_hurt", CH_P2, 200, "@37 v13 o4 c8");
    sfx_define("clc_bump", CH_NOISE, 200, "@40 v10 o4 c16");
    sfx_define("clc_die", CH_NOISE, 120, "@34 v15 o3 c2");
    sfx_define("clc_blast", CH_NOISE, 150, "@34 v14 o4 c8");
    sfx_define("clc_note", CH_P2, 200, "@39 v12 o6 e16 g16 b16 o7 e8");
    sfx_define("clc_coin", CH_P2, 240, "@35 v11 o6 e16 b16");
    sfx_define("clc_ring", CH_P2, 220, "@39 v12 o5 c16 g16 o6 c16 g16");
    sfx_define("clc_item", CH_P2, 180, "@15 v12 o6 c16 e16 g16 o7 c8");
    sfx_define("clc_land", CH_TRI, 200, "@41 v12 o3 c16");
    sfx_define("clc_board", CH_P2, 220, "@32 v10 o4 c16 g16 o5 c16");
    sfx_define("clc_door", CH_P2, 200, "@42 v10 o4 c16 e16");
    sfx_define("clc_clang", CH_NOISE, 200, "@21 v10 o5 c16");
    sfx_define("clc_break", CH_NOISE, 200, "@40 v11 o5 c16");
    sfx_define("clc_bosshit", CH_NOISE, 200, "@40 v12 o4 c16");
    sfx_define("clc_talk", CH_P2, 240, "@42 v8 o6 c32 e32 c32");
    sfx_define("clc_open", CH_P2, 180, "@39 v12 o5 c16 g16 o6 c16 g16");
    sfx_define("clc_timer", CH_P1, 160, "@1 v13 o6 c8 r16 c8 r16 o7 c4");
    sfx_define("clc_late", CH_P1, 120, "@37 v13 o3 c4 o2 g4");
    sfx_define("clc_foeshot", CH_NOISE, 240, "@21 v6 o6 c32");
    sfx_define("clc_dry", CH_P2, 220, "@42 v7 o3 c32");
    sfx_define("clc_move", CH_P2, 220, "@42 v9 o6 c32");
}
