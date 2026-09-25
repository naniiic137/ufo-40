/* TINTAIL - original music (UFO-MML) and sound effects. */
#include "tintail.h"

int TN_MUS_MAP = -1, TN_MUS_LEVEL, TN_MUS_GATE, TN_MUS_WIN, TN_MUS_EATEN, TN_MUS_END, TN_MUS_TITLE;

/* "Salt Island" - the title and the island map. G major, easy-going.
 * G | Am | Em | D | C | Am | D | G */
static const char MAP_LEAD[] =
    "@14 v11 q7 ["
    "| o4 g4 b8 o5 d8 ^4 c8 o4 b8 | o4 a4 o5 c8 e8 ^4 d8 c8 | o4 b4 o5 d8 g8 ^4 f+8 e8 | o5 d2. r4"
    "| o5 e4 g8 e8 d4 o4 b4 | o5 c4 e8 c8 o4 b4 g4 | o4 a4 b8 o5 c8 d4 f+4 | o5 g2. r4 ]2";
static const char MAP_ARP[] =
    "@2 v5 q5 l8 ["
    "o4 g b o5 d o4 b g b o5 d o4 b | o4 a o5 c e c o4 a o5 c e c | o4 e g b g e g b g | o4 d f+ a f+ d f+ a f+"
    "| o4 c e g e c e g e | o4 a o5 c e c o4 a o5 c e c | o4 d f+ a f+ d f+ a f+ | o4 g b o5 d o4 b g b o5 d o4 b ]2";
static const char MAP_BASS[] =
    "@6 v13 q6 [o2 g2 o3 d2 | o2 a2 o3 e2 | o2 e2 b2 | o2 d2 a2 | o2 c2 g2 | o2 a2 o3 e2 | o2 d2 a2 | o2 g2 o3 d2 ]2";
#define ISLE "o2@13v10c8 o8@9v4c8 o8@9v5c8 o6@11v7c8 o2@13v9c8 o8@9v4c8 o6@11v8c8 o8@9v5c8 "
static const char MAP_DRUMS[] = "[" ISLE "]16";

/* "Tiptoe" - inside a level. E minor, sneaking pizzicato.
 * Em | Em | C | B7 | Am | Em | C | B */
static const char LV_LEAD[] =
    "@3 v10 q4 ["
    "| o5 e8 r8 g8 r8 b8 r8 g8 r8 | o5 a8 r8 g8 f+8 e4 r4 | o5 e8 r8 g8 r8 o6 c8 r8 o5 b8 r8 | o5 a8 g8 f+8 d+8 e4 r4"
    "| o5 c8 r8 e8 r8 a8 r8 g8 r8 | o5 f+8 e8 d8 e8 b4 r4 | o6 c8 r8 o5 b8 r8 a8 r8 g8 r8 | o5 f+4 d+4 o4 b4 r4 ]2";
static const char LV_HARM[] =
    "@5 v5 q3 ["
    "| r4 o4 b4 r4 b4 | r4 o4 b4 r4 b4 | r4 o4 g4 r4 g4 | r4 o4 a4 r4 f+4"
    "| r4 o4 a4 r4 a4 | r4 o4 g4 r4 g4 | r4 o4 g4 r4 e4 | r4 o4 f+4 r4 d+4 ]2";
static const char LV_BASS[] =
    "@7 v13 q4 ["
    "| o2 e4 b4 o3 e4 o2 b4 | o2 e4 b4 g4 b4 | o2 c4 g4 o3 c4 o2 g4 | o2 b4 o3 d+4 f+4 o2 b4"
    "| o2 a4 o3 e4 a4 e4 | o2 e4 b4 g4 b4 | o2 c4 g4 e4 g4 | o2 b4 f+4 d+4 f+4 ]2";
#define TIP "o8@9v3c8 r8 o8@9v5c8 r8 o8@9v3c8 r8 o6@11v4c8 r8 "
static const char LV_DRUMS[] = "[" TIP "]16";

/* "The Sun Gate" - the last level. D minor, the drums of the temple.
 * Dm | C | Bb | A | Dm | F | Gm | A */
static const char GATE_LEAD[] =
    "@1 v11 q7 ["
    "| o4 d4. f8 a4 o5 d4 | o5 c4. o4 b-8 a4 g4 | o4 b-4 a8 g8 f4 d4 | o4 e2 c+2"
    "| o4 d4 f4 a4 o5 d8 e8 | o5 f4. e8 d4 c4 | o4 b-4 o5 d4 g4 f8 e8 | o5 c+2 o4 a2 ]2";
static const char GATE_HARM[] =
    "@17 v6 q4 l8 ["
    "o4 r d r d r d r d | o4 r c r c r c r c | o3 r b- r b- r b- r b- | o3 r a r a r a r a"
    "| o4 r d r d r d r d | o4 r f r f r f r f | o3 r g r g r g r g | o3 r a r a r a r a ]2";
static const char GATE_BASS[] =
    "@6 v14 q5 [o2 d4 d8 d8 a4 d4 | o2 c4 c8 c8 g4 c4 | o1 b-4 b-8 b-8 o2 f4 o1 b-4 | o1 a4 a8 a8 o2 e4 o1 a4"
    "| o2 d4 d8 d8 a4 d4 | o2 f4 f8 f8 o3 c4 o2 f4 | o2 g4 g8 g8 d4 g4 | o1 a4 a8 a8 o2 e4 c+4 ]2";
#define DRUMB "o2@13v12c8 o2@13v8c8 o6@11v9c8 o8@9v4c8 o2@13v12c8 o6@11v6c16 c16 o6@11v10c8 o8@9v5c8 "
static const char GATE_DRUMS[] = "[" DRUMB "]16";

/* "First Light" - the ending. C major, slow and warm. */
static const char END_LEAD[] =
    "@14 v11 q7"
    "| o5 e4 g4 o6 c4 o5 b8 a8 | o5 g2 e4 c4 | o5 d4 f4 a4 g8 f8 | o5 e2. r4"
    "| o5 c4 e4 a4 g8 f8 | o5 e4. d8 c4 o4 a4 | o4 b4 o5 d4 g4 f4 | o5 e1";
static const char END_ARP[] =
    "@22 v6 q6 l8 o4 [c e g e]2 | [c e g e]2 | [d f a f]2 | [c e g e]2"
    "| [c e a e]2 | [c e a e]2 | [d g b g]2 | [c e g e]2";
static const char END_BASS[] = "@6 v13 q6 o2 c2 g2 | e2 g2 | d2 a2 | c2 g2 | a2 e2 | a2 e2 | g2 d2 | c1";

void tn_audio_load(void) {
    if (TN_MUS_MAP >= 0) return;
    TN_MUS_MAP = song_define("tn_island", 96, true, MAP_LEAD, MAP_ARP, MAP_BASS, MAP_DRUMS);
    TN_MUS_TITLE = TN_MUS_MAP;
    TN_MUS_LEVEL = song_define("tn_tiptoe", 112, true, LV_LEAD, LV_HARM, LV_BASS, LV_DRUMS);
    TN_MUS_GATE = song_define("tn_sungate", 104, true, GATE_LEAD, GATE_HARM, GATE_BASS, GATE_DRUMS);
    TN_MUS_END = song_define("tn_firstlight", 84, true, END_LEAD, END_ARP, END_BASS, "");
    TN_MUS_WIN = song_define("tn_escape", 150, false, "@39 v12 o5 l16 g b o6 d g b o7 d4", "@16 v8 o5 l8 g o6 d g4.",
                             "@6 v13 o2 l8 g o3 d g4.", "");
    TN_MUS_EATEN = song_define("tn_eaten", 120, false, "@37 v11 o5 l8 e d+ d c+4.", "@5 v6 o4 l8 g f+ f e4.",
                               "@6 v12 o2 l8 e d+ d c+4.", "");

    sfx_define("tn_step", CH_NOISE, 240, "@9 v3 o8 c32");
    sfx_define("tn_bump", CH_P2, 200, "@37 v7 o3 c16");
    sfx_define("tn_camo", CH_P2, 200, "@39 v9 o5 c32 e32 g32 o6 c32 e32");
    sfx_define("tn_hidden", CH_P2, 220, "@35 v8 o6 g32 o7 c16");
    sfx_define("tn_no", CH_P2, 200, "@42 v8 o4 c16 r32 c16");
    sfx_define("tn_pear", CH_P1, 200, "@35 v11 o6 c16 e16 g16");
    sfx_define("tn_baby", CH_P1, 200, "@39 v10 o6 e16 g16 o7 c8");
    sfx_define("tn_rain", CH_NOISE, 120, "@10 v8 o7 c4 @9 v5 o8 c4");
    sfx_define("tn_sun", CH_P1, 160, "@16 v10 o5 c8 e8 g8 o6 c8");
    sfx_define("tn_log", CH_TRI, 200, "@41 v12 o3 c16");
    sfx_define("tn_spot", CH_P1, 180, "@1 v12 o6 c8 o5 c8");
    sfx_define("tn_gulp", CH_NOISE, 160, "@34 v12 o4 c8");
    sfx_define("tn_swoop", CH_NOISE, 100, "@36 v10 o5 c4");
    sfx_define("tn_undo", CH_P2, 240, "@33 v8 o6 c32 o5 g32 e32");
    sfx_define("tn_node", CH_P2, 220, "@42 v8 o6 e32");
}
