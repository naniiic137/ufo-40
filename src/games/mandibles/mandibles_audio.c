/* MANDIBLES - original music (UFO-MML) and sound effects. */
#include "mandibles.h"

int MND_MUS_TITLE = -1, MND_MUS_FIELD, MND_MUS_BATTLE, MND_MUS_MAP, MND_MUS_WIN, MND_MUS_LOSE, MND_MUS_CAPITAL;

/* chord comp: an off-beat stab on every other eighth */
#define STAB(ins, oct, n) "[" ins " " oct " r8 " n "8]4 "

/* "Bluebell Colony" - the title, G minor, a small proud march.
 * Gm Cm Gm D | Eb Bb D Gm */
static const char TITLE_LEAD[] =
    "@23 v12 q6 ["
    "| o5 g4 b-8 a8 g4 d4 | o5 e-4 d8 c8 d2 | o5 g4 b-8 o6 c8 d4 o5 b-4 | o5 a4 g8 f+8 g2"
    "| o5 b-4 o6 c8 d8 e-4 d4 | o6 c4 o5 b-8 a8 b-2 | o5 a4 g8 f+8 g4 d4 | o5 g1 ]2";
static const char TITLE_COMP[] =
    "v6 q4 [" STAB("@17", "o4", "g") STAB("@17", "o4", "c") STAB("@17", "o4", "g") STAB("@16", "o4", "d")
    STAB("@16", "o4", "e-") STAB("@16", "o4", "b-") STAB("@16", "o4", "d") STAB("@17", "o4", "g") "]2";
#define RF4(r, f) "o2 " r "4 " f "4 " r "4 " f "4 "
static const char TITLE_BASS[] =
    "@6 v15 q5 [" RF4("g", "d") RF4("c", "g") RF4("g", "d") RF4("d", "a") RF4("e-", "b-") RF4("b-", "f")
    RF4("d", "a") RF4("g", "d") "]2";
#define MARCH "o2@13v12c8 o6@11v8c16 c16 o6@11v11c8 o6@11v7c8 o2@13v12c8 o6@11v8c16 c16 o6@11v11c8 o8@9v5c8 "
#define MARCHF "o2@13v12c8 o6@11v8c16 c16 o6@11v11c8 o6@11v7c8 o6@11v9c16 c16 o6@11v10c16 c16 o6@11v12c8 o8@10v7c8 "
static const char TITLE_DRUMS[] = "[" MARCH MARCH MARCH MARCHF "]4";

/* "Molasses March" - every mission, D minor, plodding on purpose.
 * Dm Gm C A | Bb Gm A Dm */
static const char FIELD_LEAD[] =
    "@5 v11 q7 ["
    "| o5 d4. e8 f4 a4 | o5 g4 f8 e8 d2 | o5 c4. d8 e4 g4 | o5 f4 e8 d8 c+2"
    "| o5 d4 a4 b-4 a4 | o5 g4. f8 e4 c4 | o5 d4 e4 f4 e8 c+8 | o5 d2. r4 ]2";
static const char FIELD_COMP[] =
    "v6 q3 [" STAB("@17", "o4", "d") STAB("@17", "o4", "g") STAB("@16", "o4", "c") STAB("@16", "o4", "a")
    STAB("@16", "o4", "b-") STAB("@17", "o4", "g") STAB("@16", "o4", "a") STAB("@17", "o4", "d") "]2";
#define PLOD(r, f) "o2 " r "4 r8 " r "8 " f "4 r8 " r "8 "
static const char FIELD_BASS[] =
    "@6 v15 q6 [" PLOD("d", "a") PLOD("g", "d") PLOD("c", "g") PLOD("a", "e") PLOD("b-", "f") PLOD("g", "d")
    PLOD("a", "e") PLOD("d", "a") "]2";
#define TRUDGE "o2@13v12c4 o6@11v7c4 o2@13v12c8 o2@13v9c8 o6@11v8c4 "
static const char FIELD_DRUMS[] = "[" TRUDGE "]16";

/* "Orders From Above" - the campaign map, B-flat major, unhurried.
 * Bb Gm Eb F | Bb Cm F Bb */
static const char MAP_LEAD[] =
    "@14 v11 q7 ["
    "| o5 d4 f8 b-8 o6 d4 c4 | o5 b-4 a8 g8 f2 | o5 e-4 g8 b-8 o6 c4 o5 b-4 | o5 a4 g8 a8 f2"
    "| o5 d4 f8 b-8 o6 d4 f4 | o6 e-4 d8 c8 o5 b-4 g4 | o5 f4 a8 o6 c8 e-4 c4 | o5 b-2. r4 ]2";
static const char MAP_COMP[] =
    "v5 q4 [" STAB("@16", "o4", "b-") STAB("@17", "o4", "g") STAB("@16", "o4", "e-") STAB("@16", "o4", "f")
    STAB("@16", "o4", "b-") STAB("@17", "o4", "c") STAB("@16", "o4", "f") STAB("@16", "o4", "b-") "]2";
#define HALF(r, f) "o2 " r "2 " f "2 "
static const char MAP_BASS[] =
    "@6 v13 q7 [" HALF("b-", "f") HALF("g", "d") HALF("e-", "b-") HALF("f", "c") HALF("b-", "f") HALF("c", "g")
    HALF("f", "c") HALF("b-", "f") "]2";
#define SOFT "o8@9v4c4 o6@11v5c4 o8@9v4c8 c8 o6@11v5c4 "
static const char MAP_DRUMS[] = "[" SOFT "]16";

/* "The Old Halls" - the capital and the pointless hill, E minor, driving.
 * Em Am Em C | C B Em B */
static const char CAP_LEAD[] =
    "@1 v12 q6 ["
    "| o5 e8 e8 b8 e8 o6 d8 e8 o5 b8 g8 | o5 a8 a8 o6 c8 o5 a8 g8 f+8 e4 | o5 e8 e8 b8 e8 o6 d8 e8 g8 f+8"
    "| o6 e8 d8 c8 o5 b8 a4 b4 | o6 c4 o5 b8 a8 g4 a4 | o5 b4 a8 g8 f+4 d+4 | o5 e8 g8 b8 o6 e8 d8 c8 o5 b8 a8"
    "| o5 b2 e2 ]2";
static const char CAP_COMP[] =
    "v6 q3 [" STAB("@17", "o4", "e") STAB("@17", "o4", "a") STAB("@17", "o4", "e") STAB("@16", "o4", "c")
    STAB("@16", "o4", "c") STAB("@16", "o3", "b") STAB("@17", "o4", "e") STAB("@16", "o3", "b") "]2";
#define DRIVE(r) "o2 " r "8 " r "8 o3 " r "8 o2 " r "8 " r "8 " r "8 o3 " r "8 o2 " r "8 "
static const char CAP_BASS[] =
    "@6 v15 q5 [" DRIVE("e") DRIVE("a") DRIVE("e") DRIVE("c") DRIVE("c") DRIVE("b") DRIVE("e") DRIVE("b") "]2";
#define BEAT "o2@13v13c8 o8@9v6c8 o6@11v12c8 o8@9v6c8 o2@13v13c8 o2@13v11c8 o6@11v12c8 o8@9v6c8 "
#define BEATF "o2@13v13c8 o8@9v6c8 o6@11v12c8 o8@9v6c8 o6@11v10c16 c16 o6@11v12c16 c16 o6@11v13c8 o8@10v9c8 "
static const char CAP_DRUMS[] = "[" BEAT BEAT BEAT BEATF "]4";

/* "Picnic War" - 2P versus, A minor, quick.
 * Am G F E | Am G F Am */
static const char VS_LEAD[] =
    "@14 v12 q5 ["
    "| o5 a8 o6 c8 e8 a8 g8 e8 c8 o5 a8 | o5 g8 b8 o6 d8 g8 f8 d8 o5 b8 g8 | o5 f8 a8 o6 c8 f8 e8 c8 o5 a8 f8"
    "| o5 e8 g+8 b8 o6 e8 d4 o5 b4 | o6 a4 g8 e8 c4 e4 | o6 g4 f8 d8 o5 b4 o6 d4 | o6 f4 e8 d8 c8 o5 b8 a8 g+8"
    "| o5 a2 r2 ]2";
static const char VS_COMP[] =
    "v6 q3 [" STAB("@17", "o4", "a") STAB("@16", "o4", "g") STAB("@16", "o4", "f") STAB("@16", "o4", "e")
    STAB("@17", "o4", "a") STAB("@16", "o4", "g") STAB("@16", "o4", "f") STAB("@17", "o4", "a") "]2";
#define OCT(r) "o2 " r "8 o3 " r "8 o2 " r "8 o3 " r "8 o2 " r "8 o3 " r "8 o2 " r "8 o3 " r "8 "
static const char VS_BASS[] =
    "@6 v15 q5 [" OCT("a") OCT("g") OCT("f") OCT("e") OCT("a") OCT("g") OCT("f") OCT("a") "]2";
static const char VS_DRUMS[] = "[" BEAT BEAT BEAT BEATF "]4";

/* jingles */
static const char WIN_P1[] = "@16 v12 o5 l8 g b o6 d g4. d8 g2";
static const char WIN_P2[] = "@22 v8 o5 l8 d g b o6 d4. o5 b8 o6 d2";
static const char WIN_TRI[] = "@6 v14 o2 l4 g o3 d g o2 g8 o3 d8 g2";
static const char WIN_NOISE[] = "o6@11v10c16 c16 c16 c16 o6@11v12c4 r4 o5@12v9c2";
static const char LOSE_P1[] = "@5 v11 o4 l8 a g f e4. d8 c+2";
static const char LOSE_TRI[] = "@6 v13 o2 l4 a g f e2";

void mnd_audio_load(void) {
    if (MND_MUS_TITLE >= 0) return;
    MND_MUS_TITLE = song_define("mnd_title", 112, true, TITLE_LEAD, TITLE_COMP, TITLE_BASS, TITLE_DRUMS);
    MND_MUS_MAP = song_define("mnd_map", 88, true, MAP_LEAD, MAP_COMP, MAP_BASS, MAP_DRUMS);
    MND_MUS_FIELD = song_define("mnd_field", 96, true, FIELD_LEAD, FIELD_COMP, FIELD_BASS, FIELD_DRUMS);
    MND_MUS_CAPITAL = song_define("mnd_capital", 124, true, CAP_LEAD, CAP_COMP, CAP_BASS, CAP_DRUMS);
    MND_MUS_BATTLE = song_define("mnd_versus", 140, true, VS_LEAD, VS_COMP, VS_BASS, VS_DRUMS);
    MND_MUS_WIN = song_define("mnd_win", 150, false, WIN_P1, WIN_P2, WIN_TRI, WIN_NOISE);
    MND_MUS_LOSE = song_define("mnd_lose", 96, false, LOSE_P1, "", LOSE_TRI, "");

    sfx_define("mnd_spit", CH_NOISE, 240, "@36 v8 o7 c32");
    sfx_define("mnd_hit", CH_P2, 240, "@37 v9 o4 c32");
    sfx_define("mnd_die", CH_NOISE, 200, "@21 v10 o5 c16 @13 v10 o3 c16");
    sfx_define("mnd_crunch", CH_NOISE, 200, "@40 v12 o5 c16");
    sfx_define("mnd_bead", CH_P2, 220, "@35 v9 o6 e32 a32");
    sfx_define("mnd_feed", CH_P2, 200, "@39 v10 o5 c16 g16");
    sfx_define("mnd_hatch", CH_TRI, 220, "@38 v10 o4 c16");
    sfx_define("mnd_reborn", CH_P1, 180, "@16 v11 o5 c16 e16 g8");
    sfx_define("mnd_shout", CH_P2, 220, "@20 v11 o5 g32 o6 c16");
    sfx_define("mnd_menu", CH_P2, 220, "@42 v8 o6 c32");
    sfx_define("mnd_open", CH_P2, 220, "@42 v8 o5 a32");
    sfx_define("mnd_bite", CH_NOISE, 160, "@34 v13 o4 c8");
    sfx_define("mnd_spider_die", CH_P1, 150, "@33 v12 o5 c8 o4 g8 c4");
    sfx_define("mnd_ouch", CH_P1, 180, "@37 v11 o4 e16 c8");
}
