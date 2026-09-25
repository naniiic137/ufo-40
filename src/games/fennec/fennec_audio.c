/* FENNEC FOUNTAIN - original music (UFO-MML) and sound effects. */
#include "fennec.h"

int FN_MUS_HUB = -1, FN_MUS_ROOM, FN_MUS_PALACE, FN_MUS_WIN, FN_MUS_EDIT, FN_MUS_END;

/* "Dry Gardens" - the hub, E phrygian over a hand drum. Em F Em D | Em F G Em */
static const char HUB_LEAD[] =
    "@5 v11 q7 ["
    "| o5 e4 f8 g8 f4 e4 | o5 f4. e8 d4 c4 | o5 e8 f8 g8 a8 b4 a4 | o5 a4 g8 f8 e2"
    "| o5 b4 o6 c8 o5 b8 a4 g4 | o5 a4. g8 f4 e4 | o5 g8 a8 b8 o6 c8 d4 o5 b4 | o5 e2. r4 ]2";
static const char HUB_ARP[] =
    "@2 v5 q5 l8 ["
    "o4 e g b g e g b g | o4 f a o5 c o4 a f a o5 c o4 a | o4 e g b g e g b g | o4 d f+ a f+ d f+ a f+"
    "| o4 e g b g e g b g | o4 f a o5 c o4 a f a o5 c o4 a | o4 g b o5 d o4 b g b o5 d o4 b | o4 e g b g e g b g ]2";
static const char HUB_BASS[] =
    "@6 v13 q6 [o2 e2 b2 | o2 f2 o3 c2 | o2 e2 b2 | o2 d2 a2 | o2 e2 b2 | o2 f2 o3 c2 | o2 g2 o3 d2 | o2 e2 b2 ]2";
#define DUM "o2@13v10c8 r8 o7@21v7c8 o7@21v5c8 o2@13v10c8 o7@21v7c8 r8 o7@21v5c8 "
static const char HUB_DRUMS[] = "[" DUM "]16";

/* "Thinking Stones" - the rooms, A minor, slow and soft */
static const char ROOM_LEAD[] =
    "@15 v9 q8 ["
    "| o5 e2 a4 g4 | o5 f2 e4 c4 | o5 d2 g4 f4 | o5 e1"
    "| o5 c2 e4 a4 | o5 a2 g4 f4 | o5 f4 e4 d4 f4 | o5 e1 ]2";
static const char ROOM_ARP[] =
    "@2 v5 q5 l8 ["
    "o4 a o5 c e c o4 a o5 c e c | o4 f a o5 c o4 a f a o5 c o4 a | o4 g b o5 d o4 b g b o5 d o4 b | o4 e g b g e g b g"
    "| o4 a o5 c e c o4 a o5 c e c | o4 f a o5 c o4 a f a o5 c o4 a | o4 d f a f d f a f | o4 e g+ b g+ e g+ b g+ ]2";
static const char ROOM_BASS[] =
    "@6 v12 q7 [o2 a2 e2 | o2 f2 o3 c2 | o2 g2 d2 | o2 e2 b2 | o2 a2 e2 | o2 f2 o3 c2 | o2 d2 a2 | o2 e2 b2 ]2";

/* "Humph's Bath" - the palace, D minor, pompous */
static const char PALACE_LEAD[] =
    "@23 v12 q7 ["
    "| o4 d4 f4 a4 o5 d4 | o5 c4. o4 b-8 a4 g4 | o4 b-4 a4 g4 f4 | o4 a2. r4"
    "| o5 d4 f4 e4 d4 | o5 d4. c8 o4 b-4 a4 | o4 a8 b-8 o5 c+8 d8 e4 c+4 | o5 d2. r4 ]2";
#define PAH(ins, oct, n) "r8 " ins " " oct " " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
static const char PALACE_HARM[] =
    "v6 q4 [" PAH("@17", "o4", "d") PAH("@16", "o4", "c") PAH("@16", "o3", "b-") PAH("@16", "o3", "a")
    PAH("@17", "o4", "d") PAH("@17", "o4", "g") PAH("@16", "o3", "a") PAH("@17", "o4", "d") "]2";
#define OOM(r, f) "o2 " r "4 " f "4 " r "4 " f "4 "
static const char PALACE_BASS[] =
    "@6 v15 q5 [" OOM("d", "a") OOM("c", "g") OOM("b-", "f") OOM("a", "e") OOM("d", "a") OOM("g", "d") OOM("a", "e")
    OOM("d", "a") "]2";
#define MR "o2@13v12c8 o6@11v7c16 c16 o6@11v10c8 o8@9v5c8 o2@13v12c8 o2@13v9c8 o6@11v10c8 o8@9v5c8 "
static const char PALACE_DRUMS[] = "[" MR "]16";

/* "The Workshop" - the editor, C major, bouncy. C F G C | Am F G G */
static const char EDIT_LEAD[] =
    "@3 v11 q5 l8"
    "| o5 c e g e c4 g4 | o5 f a o6 c o5 a f4 c4 | o5 g b o6 d o5 b g4 d4 | o5 e g o6 c o5 g e4 c4"
    "| o5 a o6 c e c o5 a4 e4 | o5 f a o6 c o5 a f4 a4 | o5 g a b o6 c d4 o5 b4 | o5 g2 r2";
static const char EDIT_BASS[] =
    "@7 v13 q5 o2 c4 g4 c4 g4 | f4 o3 c4 o2 f4 o3 c4 | o2 g4 d4 g4 d4 | c4 g4 c4 g4"
    "| a4 e4 a4 e4 | f4 o3 c4 o2 f4 o3 c4 | o2 g4 d4 g4 d4 | g4 d4 g4 g4";
static const char EDIT_DRUMS[] = "[o2@13v10c8 o8@9v4c8 o6@11v8c8 o8@9v4c8]16";

/* "The Gardens Flow" - the ending, D major */
static const char END_LEAD[] =
    "@14 v11 q7"
    "| o5 f+4 a4 o6 d4 c+4 | o5 b4. a8 g2 | o5 e4 g4 b4 a4 | o5 f+2. r4"
    "| o5 f+4 a4 o6 d4 e4 | o6 f+4. e8 d4 o5 b4 | o5 a4 b4 o6 c+4 e4 | o6 d2. r4";
static const char END_ARP[] =
    "@22 v6 q6 l8 o4 [d f+ a f+]2 | [g b o5 d o4 b]2 | [e g b g]2 | [d f+ a f+]2"
    "| [d f+ a f+]2 | [g b o5 d o4 b]2 | [a o5 c+ e c+ o4]2 | [d f+ a f+]2";
static const char END_BASS[] = "@6 v13 q6 o2 d2 a2 | g2 d2 | e2 b2 | d2 a2 | d2 a2 | g2 d2 | a2 e2 | d2 a2";

void fn_audio_load(void) {
    if (FN_MUS_HUB >= 0) return;
    FN_MUS_HUB = song_define("fn_dry_gardens", 96, true, HUB_LEAD, HUB_ARP, HUB_BASS, HUB_DRUMS);
    FN_MUS_ROOM = song_define("fn_stones", 84, true, ROOM_LEAD, ROOM_ARP, ROOM_BASS, "");
    FN_MUS_PALACE = song_define("fn_palace", 120, true, PALACE_LEAD, PALACE_HARM, PALACE_BASS, PALACE_DRUMS);
    FN_MUS_EDIT = song_define("fn_workshop", 126, true, EDIT_LEAD, "", EDIT_BASS, EDIT_DRUMS);
    FN_MUS_END = song_define("fn_the_gardens_flow", 100, true, END_LEAD, END_ARP, END_BASS, "");
    FN_MUS_WIN = song_define("fn_spring", 150, false, "@39 v12 o5 l16 c e g o6 c e g o7 c4", "@16 v8 o5 l8 c g o6 c4.",
                             "@6 v13 o2 l8 c g o3 c4.", "");

    sfx_define("fn_step", CH_NOISE, 240, "@9 v4 o8 c32");
    sfx_define("fn_push", CH_NOISE, 220, "@13 v10 o3 c16");
    sfx_define("fn_bump", CH_P2, 200, "@37 v8 o3 c16");
    sfx_define("fn_merge", CH_P2, 220, "@39 v11 o5 c16 g16 o6 c16");
    sfx_define("fn_marble", CH_P1, 180, "@15 v12 o6 c16 e16 g16 o7 c8");
    sfx_define("fn_shrink", CH_NOISE, 200, "@41 v11 o3 c8");
    sfx_define("fn_crumble", CH_NOISE, 160, "@34 v12 o4 c8");
    sfx_define("fn_undo", CH_P2, 240, "@33 v8 o6 c32 o5 g32");
    sfx_define("fn_gecko", CH_P2, 240, "@42 v6 o7 c32");
    sfx_define("fn_door", CH_P2, 200, "@20 v10 o5 c16 e16");
    sfx_define("fn_gate", CH_P1, 180, "@16 v11 o5 c8 e8 g8 o6 c4");
    sfx_define("fn_talk", CH_P2, 240, "@42 v7 o6 e32");
    sfx_define("fn_place", CH_P2, 240, "@35 v9 o5 g32");
    sfx_define("fn_erase", CH_NOISE, 240, "@36 v7 o6 c32");
}
