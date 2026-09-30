/* SKID KIDS - original music (UFO-MML) and sound effects. */
#include "skidkids.h"

int SKID_MUS_TITLE = -1, SKID_MUS_PICK, SKID_MUS_BRACKET, SKID_MUS_MATCH1, SKID_MUS_MATCH2, SKID_MUS_ROO, SKID_MUS_BOT,
    SKID_MUS_WIN, SKID_MUS_LOSE, SKID_MUS_CHAMPS, SKID_MUS_OVER, SKID_MUS_CREDITS;

/* shared patterns: each makes one 4/4 bar */
#define STAB(ins, oct, n) ins " " oct " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define BB(a, b) a "4 " b "4 " a "4 " b "4 "
#define B8(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define RF8(r, f) r "8 " f "8 " r "8 " f "8 " r "8 " f "8 " r "8 " f "8 "
#define PLK(a, b, c) "o4 " a "8 " b "8 " c "8 " b "8 " a "8 " b "8 " c "8 " b "8 "
#define BEAT "o2@13v12c8 o8@9v5c8 o6@11v10c8 o8@9v5c8 o2@13v12c8 o2@13v10c8 o6@11v10c8 o8@9v6c8 "
#define BEAT2 "o2@13v12c8 o8@9v5c16 c16 o6@11v10c8 o8@9v5c8 o2@13v12c8 o8@9v5c8 o6@11v10c8 o6@11v7c8 "
#define FILL "o2@13v12c8 o8@9v5c8 o6@11v10c8 o8@9v5c8 o6@11v9c16 c16 o6@11v11c16 c16 o6@11v12c8 o8@10v8c8 "

/* "Skid Kids" - the title. F major, bouncy. */
static const char TITLE_LEAD[] =
    "@1 v12 q6"
    "| o4 f8 a8 o5 c8 o4 a8 f4 c4 | o4 d8 f8 b-8 f8 d4 f4 | o4 e8 g8 o5 c8 o4 g8 e8 g8 o5 c4 | o4 a4 o5 c8 o4 a8 f2"
    "| o4 d8 f8 a8 f8 d4 a4 | o4 b-8 a8 g8 f8 d4 f4 | o4 e8 f8 g8 a8 b-4 o5 c4 | o5 c2 r4 o4 c4"
    "| o4 f8 a8 o5 c8 o4 a8 f4 c4 | o4 d8 f8 b-8 o5 d8 c4 o4 b-4 | o4 a8 g8 f8 e8 g4 o5 c4 | o4 d8 e8 f8 a8 o5 d4 c4"
    "| o4 b-8 a8 b-8 o5 d8 c4 o4 b-4 | o4 a8 g8 a8 o5 c8 o4 g4 e4 | o4 f8 a8 o5 c8 f8 e4 c4 | o5 f2 r2";
static const char TITLE_HARM[] =
    "v6 q4 " STAB("@16", "o4", "f") STAB("@16", "o3", "b-") STAB("@16", "o4", "c") STAB("@16", "o4", "f")
    STAB("@17", "o4", "d") STAB("@16", "o3", "b-") STAB("@16", "o4", "c") STAB("@16", "o4", "c")
    STAB("@16", "o4", "f") STAB("@16", "o3", "b-") STAB("@16", "o4", "f") STAB("@17", "o4", "d")
    STAB("@16", "o3", "b-") STAB("@16", "o4", "c") STAB("@16", "o4", "f") STAB("@16", "o4", "f");
static const char TITLE_BASS[] =
    "@6 v15 q5 " BB("o2f", "o3c") BB("o2b-", "o3f") BB("o2c", "o2g") BB("o2f", "o3c") BB("o2d", "o2a")
    BB("o2b-", "o3f") BB("o2c", "o2g") BB("o2c", "o2g") BB("o2f", "o3c") BB("o2b-", "o3f") BB("o2f", "o3c")
    BB("o2d", "o2a") BB("o2b-", "o3f") BB("o2c", "o2g") BB("o2f", "o3c") BB("o2f", "o3c");
static const char TITLE_DRUMS[] = "[" BEAT "]7 " FILL "[" BEAT "]7 " FILL;

/* "Pick Me, Pick Me" - choosing teams. C major, skipping along. */
static const char PICK_LEAD[] =
    "@3 v12 q6"
    "| o5 c8 e8 g8 e8 c8 e8 g4 | o5 a8 g8 f8 e8 d4 o4 g4 | o5 c8 e8 g8 o6 c8 o5 b8 a8 g4 | o5 f8 e8 d8 e8 c2"
    "| o5 e8 f8 g8 a8 g8 f8 e4 | o5 d8 e8 f8 g8 f8 e8 d4 | o5 c8 d8 e8 g8 a8 g8 e8 d8 | o5 c4 o4 g4 o5 c4 r4";
static const char PICK_HARM[] =
    "@3 v6 q5 " PLK("c", "e", "g") PLK("c", "f", "a") PLK("c", "e", "g") PLK("d", "g", "b")
    PLK("c", "e", "g") PLK("d", "g", "b") PLK("c", "e", "g") PLK("c", "e", "g");
static const char PICK_BASS[] =
    "@6 v14 q6 " BB("o2c", "o2g") BB("o2f", "o3c") BB("o2c", "o2g") BB("o2g", "o3d") BB("o2c", "o2g")
    BB("o2g", "o3d") BB("o2c", "o2g") BB("o2c", "o2g");
#define DRL "o2@13v11c8 o8@9v5c8 o6@11v9c8 o8@9v5c8 "
static const char PICK_DRUMS[] = "[" DRL "]16";

/* "The Big Board" - the tournament ladder. G major, a march. */
static const char BRACKET_LEAD[] =
    "@23 v11 q7"
    "| o4 g4 b8 o5 d8 g4 d4 | o5 e4 d8 c8 o4 b4 a4 | o4 a4 b8 o5 c8 d4 o4 a4 | o4 b2 g4 r4"
    "| o5 c4 d8 e8 d4 c4 | o4 b4 o5 c8 d8 o4 g4 b4 | o4 a4 g8 a8 b4 o5 c4 | o5 d2 o4 g2";
static const char BRACKET_HARM[] = "@4 v6 q8 o4 d1 e1 f+1 d1 e1 d1 f+1 d1";
static const char BRACKET_BASS[] =
    "@6 v13 q6 " BB("o2g", "o3d") BB("o2c", "o2g") BB("o2d", "o2a") BB("o2g", "o3d") BB("o2c", "o2g")
    BB("o2g", "o3d") BB("o2d", "o2a") BB("o2g", "o3d");
#define MARCH "o2@13v10c4 o6@11v7c8 o6@11v6c8 o2@13v9c4 o6@11v8c4 "
static const char BRACKET_DRUMS[] = "[" MARCH "]8";

/* "Gym Class" - a match. A minor, driving. */
static const char MATCH1_LEAD[] =
    "@1 v12 q6"
    "| o5 a8 a8 o6 c8 o5 a8 e4 a4 | o5 f8 f8 a8 f8 c4 f4 | o5 g8 g8 b8 g8 d4 g4 | o5 a8 b8 o6 c8 d8 e4 o5 a4"
    "| o6 e8 d8 c8 o5 b8 a4 e4 | o5 f8 g8 a8 o6 c8 o5 a4 f4 | o5 g8 a8 b8 o6 d8 o5 b4 g4 | o5 g+8 a8 b8 g+8 e2"
    "| o6 c8 o5 a8 f8 a8 o6 c4 f4 | o6 d8 o5 b8 g8 b8 o6 d4 g4 | o6 e8 d8 c8 d8 e4 a4 | o6 a8 g8 e8 d8 c4 o5 a4"
    "| o5 f8 a8 o6 c8 f8 e8 d8 c4 | o5 b8 o6 c8 d8 g8 f8 e8 d4 | o5 b8 o6 c8 d8 e8 o5 g+4 b4 | o5 e2 r4 e4";
static const char MATCH1_HARM[] =
    "v6 q4 " STAB("@17", "o4", "a") STAB("@16", "o4", "f") STAB("@16", "o4", "g") STAB("@17", "o4", "a")
    STAB("@17", "o4", "a") STAB("@16", "o4", "f") STAB("@16", "o4", "g") STAB("@16", "o4", "e")
    STAB("@16", "o4", "f") STAB("@16", "o4", "g") STAB("@17", "o4", "a") STAB("@17", "o4", "a")
    STAB("@16", "o4", "f") STAB("@16", "o4", "g") STAB("@16", "o4", "e") STAB("@16", "o4", "e");
static const char MATCH1_BASS[] =
    "@6 v14 q5 " B8("o2a") B8("o2f") B8("o2g") B8("o2a") B8("o2a") B8("o2f") B8("o2g") B8("o2e")
    B8("o2f") B8("o2g") B8("o2a") B8("o2a") B8("o2f") B8("o2g") B8("o2e") B8("o2e");
static const char MATCH1_DRUMS[] = "[" BEAT "]15 " FILL;

/* "Recess Rally" - a match. D major, quick. */
static const char MATCH2_LEAD[] =
    "@14 v12 q6"
    "| o5 d8 f+8 a8 f+8 d4 a4 | o5 g8 b8 o6 d8 o5 b8 g4 d4 | o5 a8 o6 c+8 e8 c+8 o5 a4 e4 | o5 f+4 e8 d8 f+2"
    "| o5 b8 a8 f+8 d8 b4 f+4 | o5 g8 f+8 g8 b8 o6 d4 o5 b4 | o5 a8 b8 o6 c+8 d8 e4 c+4 | o6 e2 r4 o5 a4"
    "| o6 d8 c+8 o5 b8 a8 f+4 a4 | o5 b8 a8 g8 f+8 g4 b4 | o5 a8 g8 f+8 e8 a4 o6 c+4 | o6 d8 c+8 o5 b8 f+8 b2"
    "| o5 g8 a8 b8 o6 d8 e4 d4 | o6 c+8 o5 b8 a8 g8 f+4 e4 | o5 d8 f+8 a8 o6 d8 f+4 e4 | o6 d2 r2";
static const char MATCH2_HARM[] =
    "v6 q4 " STAB("@16", "o4", "d") STAB("@16", "o3", "g") STAB("@16", "o3", "a") STAB("@16", "o4", "d")
    STAB("@17", "o3", "b") STAB("@16", "o3", "g") STAB("@16", "o3", "a") STAB("@16", "o3", "a")
    STAB("@16", "o4", "d") STAB("@16", "o3", "g") STAB("@16", "o3", "a") STAB("@17", "o3", "b")
    STAB("@16", "o3", "g") STAB("@16", "o3", "a") STAB("@16", "o4", "d") STAB("@16", "o4", "d");
static const char MATCH2_BASS[] =
    "@6 v14 q5 " RF8("o2d", "o2a") RF8("o2g", "o3d") RF8("o2a", "o3e") RF8("o2d", "o2a") RF8("o2b", "o3f+")
    RF8("o2g", "o3d") RF8("o2a", "o3e") RF8("o2a", "o3e") RF8("o2d", "o2a") RF8("o2g", "o3d") RF8("o2a", "o3e")
    RF8("o2b", "o3f+") RF8("o2g", "o3d") RF8("o2a", "o3e") RF8("o2d", "o2a") RF8("o2d", "o2a");
static const char MATCH2_DRUMS[] = "[" BEAT2 "]15 " FILL;

/* "Field Trip" - the kangaroo's match. E major, all hops. */
#define ROO_A                                                                                                       \
    "| o4 e8 b8 e8 b8 g+8 b8 o5 e4 | o4 a8 o5 c+8 o4 a8 o5 c+8 e8 c+8 o4 a4 | o4 b8 o5 d+8 f+8 d+8 o4 b8 o5 d+8 f+4" \
    "| o5 e4 o4 b8 g+8 e2 | o5 c+8 e8 g+8 e8 c+8 e8 g+4 | o5 a8 g+8 f+8 e8 c+4 o4 a4"                              \
    "| o4 b8 o5 c+8 d+8 f+8 e8 d+8 c+4 "
static const char ROO_LEAD[] = "@18 v11 q6 " ROO_A "| o4 b2 r4 b4 " ROO_A "| o5 e2 r2";
#define ROO_H STAB("@16", "o4", "e") STAB("@16", "o3", "a") STAB("@16", "o3", "b") STAB("@16", "o4", "e") \
    STAB("@17", "o4", "c+") STAB("@16", "o3", "a") STAB("@16", "o3", "b") STAB("@16", "o3", "b")
static const char ROO_HARM[] = "v6 q4 " ROO_H ROO_H;
#define OCTH(n) "o2 " n "8 r8 o3 " n "8 r8 o2 " n "8 r8 o3 " n "8 r8 "
#define ROO_B OCTH("e") OCTH("a") OCTH("b") OCTH("e") OCTH("c+") OCTH("a") OCTH("b") OCTH("b")
static const char ROO_BASS[] = "@6 v14 q5 " ROO_B ROO_B;
static const char ROO_DRUMS[] = "[" BEAT "]7 " FILL "[" BEAT "]7 " FILL;

/* "Benchbot" - the robot's match. C minor, clockwork. */
static const char BOT_LEAD[] =
    "@2 v12 q5"
    "| o5 c8 c8 e-8 c8 g8 c8 e-8 c8 | o5 c8 c8 e-8 c8 g8 f8 e-8 d8 | o5 c8 c8 e-8 c8 a-8 c8 e-8 c8"
    "| o5 d8 d8 f8 d8 b-8 a-8 g8 f8 | o5 c8 c8 e-8 c8 g8 c8 e-8 c8 | o5 c8 c8 e-8 c8 g8 f8 e-8 d8"
    "| o5 c8 c8 e-8 c8 a-8 c8 e-8 c8 | o5 d8 d8 g8 d8 b8 g8 f8 d8 | o5 c8 c8 f8 c8 a-8 f8 e-8 c8"
    "| o5 d8 d8 g8 d8 b8 a-8 g8 f8 | o5 e-8 g8 o6 c8 o5 g8 e-8 g8 o6 c4 | o5 e-8 a-8 o6 c8 o5 a-8 e-8 a-8 o6 c4"
    "| o5 f8 a-8 o6 c8 f8 e-8 c8 o5 a-4 | o5 g8 b8 o6 d8 g8 f8 d8 o5 b4 | o6 c8 o5 g8 e-8 c8 g8 e-8 c4"
    "| o5 d8 g8 b8 o6 d8 f4 d4";
static const char BOT_HARM[] =
    "v6 q4 " STAB("@17", "o4", "c") STAB("@17", "o4", "c") STAB("@16", "o3", "a-") STAB("@16", "o3", "b-")
    STAB("@17", "o4", "c") STAB("@17", "o4", "c") STAB("@16", "o3", "a-") STAB("@16", "o3", "g")
    STAB("@17", "o3", "f") STAB("@16", "o3", "g") STAB("@17", "o4", "c") STAB("@16", "o3", "a-")
    STAB("@17", "o3", "f") STAB("@16", "o3", "g") STAB("@17", "o4", "c") STAB("@16", "o3", "g");
static const char BOT_BASS[] =
    "@6 v14 q5 " B8("o2c") B8("o2c") B8("o1a-") B8("o1b-") B8("o2c") B8("o2c") B8("o1a-") B8("o1g")
    B8("o1f") B8("o1g") B8("o2c") B8("o1a-") B8("o1f") B8("o1g") B8("o2c") B8("o1g");
#define CLOCK "o2@13v12c8 o8@21v6c8 o6@11v10c8 o8@21v6c8 o2@13v12c8 o8@21v6c8 o6@11v10c8 o8@21v7c8 "
static const char BOT_DRUMS[] = "[" CLOCK "]16";

/* "Gym Champions" - the ending. C major, proud. */
static const char CHAMPS_LEAD[] =
    "@23 v12 q7"
    "| o5 c4 e4 g4 o6 c4 | o6 c4. o5 a8 f2 | o5 g4 b4 o6 d4 f4 | o6 e2. r4"
    "| o5 a4 o6 c4 e4 a4 | o6 a4. g8 f4 c4 | o6 d4 e4 f4 d4 | o6 c2. r4";
static const char CHAMPS_HARM[] =
    "@22 v7 q6 " PLK("c", "e", "g") PLK("c", "f", "a") PLK("d", "g", "b") PLK("c", "e", "g") PLK("c", "e", "a")
    PLK("c", "f", "a") PLK("d", "g", "b") PLK("c", "e", "g");
static const char CHAMPS_BASS[] =
    "@6 v14 q6 o2 c2 g2 | o2 f2 o3 c2 | o2 g2 o3 d2 | o2 c2 g2 | o2 a2 o3 e2 | o2 f2 o3 c2 | o2 g2 o3 d2 | o2 c2 g2";
static const char CHAMPS_DRUMS[] = "[o2@13v12c4 o6@11v9c4 o2@13v12c8 c8 o6@11v10c4]8";

/* "Hit the Showers" - the credits. F major, easy does it. */
static const char CREDITS_LEAD[] =
    "@14 v11 q7"
    "| o5 c4 a4 g8 f8 e8 f8 | o5 e4 c4 e8 g8 a4 | o5 f4 d4 f8 b-8 a8 g8 | o5 g2 e4 c4"
    "| o5 c4 a4 g8 f8 e8 f8 | o5 d4 f4 a4 o6 d4 | o6 c4 o5 b-8 a8 g4 e4 | o5 f2. r4";
static const char CREDITS_HARM[] = "@4 v6 q8 o4 a1 | o5 c1 | o4 d1 | e1 | a1 | f1 | d2 e2 | c1";
static const char CREDITS_BASS[] =
    "@6 v13 q6 o2 f2 o3 c2 | o2 a2 o3 e2 | o2 b-2 o3 f2 | o2 c2 g2 | o2 f2 o3 c2 | o2 d2 a2 | o2 b-2 c2 | o2 f2 o3 c2";
static const char CREDITS_DRUMS[] = "[o2@13v9c4 o8@9v4c8 o8@9v4c8 o6@11v7c4 o8@9v4c8 o8@9v4c8]8";

/* jingles */
static const char WIN_P1[] = "@16 v12 o5 l8 c e g o6 c4. o5 b8 o6 c8 d8 e2";
static const char WIN_P2[] = "@22 v8 o5 l8 e g o6 c e4. d8 e8 f8 g2";
static const char WIN_TRI[] = "@6 v14 o3 l4 c g o4 c o3 g c2";
static const char LOSE_P1[] = "@5 v11 o5 l8 e d c o4 b4. a8 g+2";
static const char LOSE_TRI[] = "@6 v13 o2 l4 a g f e2";
static const char OVER_P1[] = "@5 v11 o4 l8 g f e d4. c8 o3 b2 r4 o4 c2";
static const char OVER_TRI[] = "@6 v13 o2 l4 c o1 b a g o2 c1";

void skid_audio_load(void) {
    if (SKID_MUS_TITLE >= 0) return;
    SKID_MUS_TITLE = song_define("skid_title", 150, true, TITLE_LEAD, TITLE_HARM, TITLE_BASS, TITLE_DRUMS);
    SKID_MUS_PICK = song_define("skid_pick", 132, true, PICK_LEAD, PICK_HARM, PICK_BASS, PICK_DRUMS);
    SKID_MUS_BRACKET = song_define("skid_bracket", 110, true, BRACKET_LEAD, BRACKET_HARM, BRACKET_BASS, BRACKET_DRUMS);
    SKID_MUS_MATCH1 = song_define("skid_gym", 144, true, MATCH1_LEAD, MATCH1_HARM, MATCH1_BASS, MATCH1_DRUMS);
    SKID_MUS_MATCH2 = song_define("skid_recess", 150, true, MATCH2_LEAD, MATCH2_HARM, MATCH2_BASS, MATCH2_DRUMS);
    SKID_MUS_ROO = song_define("skid_roo", 160, true, ROO_LEAD, ROO_HARM, ROO_BASS, ROO_DRUMS);
    SKID_MUS_BOT = song_define("skid_bot", 138, true, BOT_LEAD, BOT_HARM, BOT_BASS, BOT_DRUMS);
    SKID_MUS_CHAMPS = song_define("skid_champs", 120, true, CHAMPS_LEAD, CHAMPS_HARM, CHAMPS_BASS, CHAMPS_DRUMS);
    SKID_MUS_CREDITS = song_define("skid_credits", 100, true, CREDITS_LEAD, CREDITS_HARM, CREDITS_BASS, CREDITS_DRUMS);
    SKID_MUS_WIN = song_define("skid_win", 140, false, WIN_P1, WIN_P2, WIN_TRI, "@12 v8 o6 c2");
    SKID_MUS_LOSE = song_define("skid_lose", 96, false, LOSE_P1, "", LOSE_TRI, "");
    SKID_MUS_OVER = song_define("skid_over", 90, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("skid_throw", CH_NOISE, 240, "@36 v10 o6 c16");
    sfx_define("skid_hit", CH_P2, 220, "@37 v12 o4 c16 o3 g16");
    sfx_define("skid_down", CH_TRI, 200, "@41 v13 o3 c8");
    sfx_define("skid_jump", CH_P2, 240, "@32 v8 o5 c16");
    sfx_define("skid_half", CH_P2, 240, "@39 v10 o6 c16 e16");
    sfx_define("skid_pick", CH_P2, 240, "@35 v9 o5 g32 o6 c32");
    sfx_define("skid_pass", CH_NOISE, 240, "@36 v7 o7 c16");
    sfx_define("skid_swap", CH_P2, 240, "@42 v9 o6 e32 c32");
    sfx_define("skid_special", CH_P1, 200, "@32 v13 o4 c16 g16 o5 c16 g8");
    sfx_define("skid_move", CH_P1, 200, "@39 v12 o5 c16 g16 o6 c8");
    sfx_define("skid_boom", CH_NOISE, 140, "@34 v15 o3 c4");
    sfx_define("skid_wall", CH_TRI, 240, "@8 v11 o3 c16");
    sfx_define("skid_coach", CH_P2, 200, "@15 v9 o6 c16 o5 g16");
    sfx_define("skid_whistle", CH_P1, 200, "@0 v12 o7 c8 r16 c4");
    sfx_define("skid_slip", CH_P2, 200, "@33 v11 o6 c8");
    sfx_define("skid_splash", CH_NOISE, 160, "@10 v12 o5 c8 @9 v8 o7 c8");
    sfx_define("skid_drink", CH_P2, 240, "@35 v10 o5 c16 e16 g16");
    sfx_define("skid_stomp", CH_NOISE, 160, "@34 v14 o2 c8");
    sfx_define("skid_gust", CH_NOISE, 120, "@36 v10 o5 c4");
    sfx_define("skid_reel", CH_P2, 240, "@32 v10 o5 c16 g16 o6 c16");
    sfx_define("skid_push", CH_NOISE, 220, "@13 v12 o4 c16");
    sfx_define("skid_cancel", CH_P2, 240, "@33 v8 o5 c16");
    sfx_define("skid_forced", CH_P2, 220, "@15 v10 o6 c16 c16");
    sfx_define("skid_clank", CH_NOISE, 180, "@40 v12 o5 c8");
    sfx_define("skid_cry", CH_P2, 160, "@5 v9 o5 e8 d8 c4");
    sfx_define("skid_star", CH_P2, 240, "@39 v11 o6 c16 e16 g16 o7 c16");
}
