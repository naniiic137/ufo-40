/* DUNE EXPRESS - original music (UFO-MML) and sound effects. */
#include "dune.h"

int DX_MUS_TITLE = -1, DX_MUS_QUIET, DX_MUS_TENSE, DX_MUS_WIN, DX_MUS_LOSE, DX_MUS_MAP, DX_MUS_END;

/* "Salt Line" - the title. D phrygian, a train rhythm under a long melody.
 * Dm | Eb | Dm | C | Dm | Eb | C | Dm */
static const char T_LEAD[] =
    "@14 v11 q7 ["
    "| o5 d4. e-8 f4 a4 | o5 g4. f8 e-2 | o5 d4 f8 e-8 d4 c4 | o5 d2. r4"
    "| o5 a4. b-8 a4 g4 | o5 f4. g8 e-4 d4 | o5 c4 e-8 d8 c4 o4 b-4 | o5 d2. r4 ]2";
static const char T_HARM[] =
    "@17 v5 q4 l8 ["
    "o4 d a d a d a d a | o4 e- b- e- b- e- b- e- b- | o4 d a d a d a d a | o4 c g c g c g c g"
    "| o4 d a d a d a d a | o4 e- b- e- b- e- b- e- b- | o4 c g c g c g c g | o4 d a d a d a d a ]2";
static const char T_BASS[] =
    "@6 v13 q5 [o2 d4 d8 d8 d4 d8 d8 | o2 e-4 e-8 e-8 e-4 e-8 e-8 | o2 d4 d8 d8 d4 d8 d8 | o2 c4 c8 c8 c4 c8 c8"
    "| o2 d4 d8 d8 d4 d8 d8 | o2 e-4 e-8 e-8 e-4 e-8 e-8 | o2 c4 c8 c8 c4 c8 c8 | o2 d4 d8 d8 d4 d8 d8 ]2";
#define CHUG "o2@13v11c8 o8@9v4c16 o8@9v3c16 o6@11v7c8 o8@9v4c16 o8@9v3c16 o2@13v10c8 o8@9v4c16 o8@9v3c16 o6@11v7c8 o8@9v5c8 "
static const char T_DRUMS[] = "[" CHUG "]16";

/* "Quiet Carriage" - sneaking. A minor, soft plucks and brushes. */
static const char Q_LEAD[] =
    "@3 v9 q4 ["
    "| o5 a8 r8 r4 e8 r8 r4 | o5 g8 r8 f8 r8 e4 r4 | o5 a8 r8 r4 o6 c8 r8 r4 | o5 b8 r8 g+8 r8 e4 r4 ]2";
static const char Q_HARM[] =
    "@5 v4 q3 ["
    "| r4 o4 e4 r4 e4 | r4 o4 d4 r4 c4 | r4 o4 e4 r4 a4 | r4 o4 d4 r4 b4 ]2";
static const char Q_BASS[] =
    "@7 v12 q4 [o2 a4 r4 e4 r4 | o2 d4 r4 a4 r4 | o2 a4 r4 e4 r4 | o2 e4 r4 g+4 r4 ]2";
static const char Q_DRUMS[] = "[o8@9v3c8 r8 o8@9v2c8 r8 o8@9v3c8 r8 o8@9v2c8 o8@9v2c8]8";

/* "Guards on the Move" - turns. D minor, driving. Dm | Bb | C | A */
static const char X_LEAD[] =
    "@1 v11 q6 ["
    "| o5 d8 d8 f8 d8 a8 d8 f8 d8 | o5 d8 d8 f8 d8 b-8 a8 g8 f8 | o5 e8 e8 g8 e8 o6 c8 o5 e8 g8 e8 | o5 c+8 e8 a8 e8 c+4 e4"
    "| o6 d8 c8 o5 a8 f8 d4 f4 | o5 b-8 a8 f8 d8 f4 b-4 | o5 g8 e8 c8 e8 g4 o6 c4 | o5 a4 g8 f8 e4 c+4 ]2";
static const char X_ARP[] =
    "@18 v5 q4 l16 [[o4 d f a f]4 | [o3 b- o4 d f d]4 | [o4 c e g e]4 | [o4 c+ e a e]4 ]4";
static const char X_BASS[] =
    "@7 v14 q5 l8 [[o2 d o3 d]4 | [o2 b- o3 b-]4 | [o2 c o3 c]4 | [o2 a o3 a]4 ]4";
#define RUSH "o2@13v12c8 o8@9v5c8 o6@11v10c8 o8@9v5c8 o2@13v12c8 o2@13v9c8 o6@11v10c8 o6@11v6c16 o6@11v8c16 "
static const char X_DRUMS[] = "[" RUSH "]16";

/* "The Line" - the mission map. F major, a steady trot. */
static const char M_LEAD[] =
    "@14 v10 q7 | o5 c4 f4 a4 g8 f8 | o5 g4 e4 c2 | o5 d4 f4 b-4 a8 g8 | o5 a2. r4"
    "| o5 c4 f4 a4 o6 c4 | o5 b-4 g4 e4 c4 | o5 d4 e4 f4 g4 | o5 f2. r4";
static const char M_BASS[] =
    "@7 v12 q5 o2 f4 c4 f4 c4 | c4 g4 c4 g4 | b-4 f4 b-4 f4 | f4 c4 f4 c4 | f4 c4 f4 c4 | c4 g4 c4 g4 | b-4 f4 c4 g4 | f4 c4 f4 r4";
static const char M_DRUMS[] = "[o2@13v9c8 o8@9v3c8 o6@11v6c8 o8@9v3c8]16";

/* "Coastward" - the ending. G major, gentle. */
static const char E_LEAD[] =
    "@14 v11 q7 | o5 d4 g4 b4 a8 g8 | o5 a4. g8 e2 | o5 c4 e4 g4 f+8 e8 | o5 d2. r4"
    "| o5 d4 g4 b4 o6 d4 | o6 c4 o5 a4 f+4 d4 | o5 e4 f+4 g4 a4 | o5 g1";
static const char E_ARP[] =
    "@22 v6 q6 l8 o4 [g b o5 d o4 b]2 | [c e g e]2 | [c e g e]2 | [d f+ a f+]2 | [g b o5 d o4 b]2 | [a o5 c e c o4]2 | [c e g e]2 | [g b o5 d o4 b]2";
static const char E_BASS[] = "@6 v12 q6 o2 g2 d2 | c2 g2 | c2 g2 | d2 a2 | g2 d2 | a2 e2 | c2 d2 | g1";

void dx_audio_load(void) {
    if (DX_MUS_TITLE >= 0) return;
    DX_MUS_TITLE = song_define("dx_saltline", 104, true, T_LEAD, T_HARM, T_BASS, T_DRUMS);
    DX_MUS_QUIET = song_define("dx_quiet", 92, true, Q_LEAD, Q_HARM, Q_BASS, Q_DRUMS);
    DX_MUS_TENSE = song_define("dx_guards", 132, true, X_LEAD, X_ARP, X_BASS, X_DRUMS);
    DX_MUS_MAP = song_define("dx_theline", 100, true, M_LEAD, "", M_BASS, M_DRUMS);
    DX_MUS_END = song_define("dx_coastward", 88, true, E_LEAD, E_ARP, E_BASS, "");
    DX_MUS_WIN = song_define("dx_getaway", 150, false, "@39 v12 o5 l16 d f+ a o6 d f+ a o7 d4", "@16 v8 o5 l8 d a o6 d4.",
                             "@6 v13 o2 l8 d a o3 d4.", "@12 v8 o6 c2");
    DX_MUS_LOSE = song_define("dx_caught", 110, false, "@37 v11 o5 l8 a g+ g f+4. f2", "@5 v6 o4 l8 e d+ d c+4. c2",
                              "@6 v12 o2 l8 a g+ g f+4. f2", "");

    sfx_define("dx_shot", CH_NOISE, 200, "@34 v13 o6 c16 @10 v8 o7 c8");
    sfx_define("dx_boom", CH_NOISE, 120, "@34 v15 o3 c4");
    sfx_define("dx_punch", CH_NOISE, 220, "@13 v12 o4 c16");
    sfx_define("dx_coin", CH_P1, 220, "@35 v10 o6 e16 b16");
    sfx_define("dx_stun", CH_P2, 220, "@39 v9 o6 c32 e32 g32 e32 c32");
    sfx_define("dx_break", CH_NOISE, 200, "@36 v11 o5 c16 @9 v8 o7 c16");
    sfx_define("dx_gate", CH_P2, 160, "@37 v11 o3 c8 o2 g8");
    sfx_define("dx_alert", CH_P1, 180, "@1 v12 o6 e16 r32 e8");
    sfx_define("dx_turn", CH_P2, 200, "@42 v10 o6 g16 o5 g16");
    sfx_define("dx_jump", CH_P2, 240, "@32 v7 o5 c16");
}
