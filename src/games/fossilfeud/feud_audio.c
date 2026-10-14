/* FOSSIL FEUD - original music (UFO-MML) and sound effects. Every looping
 * tune is eight bars a channel, so the channels loop together. */
#include "feud.h"

int FEUD_MUS_TITLE = -1, FEUD_MUS_MAP, FEUD_MUS_RIVAL, FEUD_MUS_BATTLE, FEUD_MUS_ORDEAL, FEUD_MUS_WIN, FEUD_MUS_LOSE,
    FEUD_MUS_ENDING, FEUD_MUS_STORY;

#define Q4(a, b, c, d) a "4 " b "4 " c "4 " d "4 "
#define E8(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
#define HAND "@13 v9 o2 c8 @9 v3 o8 c8 @11 v6 o6 c8 @9 v3 o8 c8 "

/* "Old Bones" - the title, A minor, stately */
static const char TITLE_LEAD[] =
    "@14 v11 q7"
    "| o5 a4. g8 e4 d4 | o5 e2 c4 o4 a4 | o5 c4. d8 e4 g4 | o5 a2. r4"
    "| o5 a4. b8 o6 c4 o5 b4 | o5 a4 g4 e4 d4 | o5 c4 d8 e8 d4 o4 b4 | o4 a1";
static const char TITLE_PAD[] = "@4 v5 q8 o4 a1 f1 e1 e1 a1 f1 g1 a1";
static const char TITLE_BASS[] =
    "@6 v12 q6 " Q4("o2 a", "e", "a", "e") Q4("o2 f", "c", "f", "c") Q4("o2 c", "g", "c", "g") Q4("o2 e", "b", "e", "b")
    Q4("o2 a", "e", "a", "e") Q4("o2 f", "c", "f", "c") Q4("o2 g", "d", "g", "d") Q4("o2 a", "e", "a", "e");
static const char TITLE_DRUM[] = "[@13 v10 o2 c4 @9 v4 o8 c8 c8 @11 v7 o6 c4 @9 v4 o8 c4]8";

/* "Under the Ground" - the story, slow */
static const char STORY_LEAD[] =
    "@5 v10 q7"
    "| o5 e2 d4 c4 | o4 b2. a4 | o5 c4 d4 e4 g4 | o5 e1"
    "| o5 a2 g4 e4 | o5 f2. e4 | o5 d4 c4 o4 b4 g+4 | o4 a1";
static const char STORY_PAD[] = "@4 v5 q8 o4 a1 e1 c1 e1 a1 d1 e1 a1";
static const char STORY_BASS[] = "@6 v11 q8 o2 a1 e1 a1 c1 f1 d1 e1 a1";
static const char STORY_SOFT[] = "[@9 v2 o8 c2]16";

/* "The Totem Fields" - your turn on the board, D dorian, walking */
static const char MAP_LEAD[] =
    "@1 v10 q6"
    "| o5 d8 e8 f8 a8 g4 f8 e8 | o5 d4 a4 o6 c4 o5 a4 | o5 g8 a8 b8 g8 e4 d8 e8 | o5 f2 e2"
    "| o5 d8 e8 f8 a8 g4 f8 e8 | o5 d4 a4 o6 c4 d4 | o6 c8 o5 a8 g8 e8 f4 e8 c8 | o5 d1";
static const char MAP_COMP[] =
    "@17 v6 q3 [o4 d8 r8]4 [o4 c8 r8]4 [o4 e8 r8]4 [o4 d8 r8]4 [o4 d8 r8]4 [o4 c8 r8]4 [o3 a8 r8]4 [o4 d8 r8]4";
static const char MAP_BASS[] =
    "@6 v12 q6 " Q4("o2 d", "a", "d", "a") Q4("o2 c", "g", "c", "g") Q4("o2 e", "b", "g", "b") Q4("o2 d", "a", "c", "a")
    Q4("o2 d", "a", "d", "a") Q4("o2 c", "g", "c", "g") Q4("o2 a", "e", "c", "e") Q4("o2 d", "a", "d", "a");
static const char MAP_DRUM[] = "[" HAND HAND "]8";

/* "Slate on the Wind" - their turn, E phrygian, uneasy */
static const char RIVAL_LEAD[] =
    "@5 v10 q7"
    "| o4 e4 f4 g4 f4 | o4 e2 r4 b4 | o5 c4 o4 b4 a4 g4 | o4 f2 e2"
    "| o4 e4 f8 g8 a4 g4 | o4 f4 e4 d4 f4 | o4 e4 f4 g4 b4 | o5 c4 o4 b4 e2";
static const char RIVAL_PAD[] = "@4 v5 q8 o3 [e1 f1]4";
static const char RIVAL_BASS[] = "@6 v12 q7 o2 [e8 e8 e8 e8 f8 f8 e8 e8]8";
static const char RIVAL_DRUM[] = "[@13 v9 o2 c4 r4 @11 v6 o6 c4 r4]8";

/* "Talon and Beak" - battle, C minor, fast */
static const char BATTLE_LEAD[] =
    "@0 v12 q5"
    "| o5 c8 c8 e-8 g8 c8 c8 b-8 g8 | o5 a-8 g8 f8 e-8 f8 g8 c4 | o5 c8 c8 e-8 g8 o6 c8 o5 b-8 a-8 g8 | o5 f8 e-8 d8 e-8 c4 r4"
    "| o5 g8 g8 a-8 b-8 o6 c8 o5 b-8 a-8 g8 | o5 f8 f8 g8 a-8 g4 e-4 | o5 d8 e-8 f8 g8 a-8 g8 f8 d8 | o5 c2 r2";
static const char BATTLE_STAB[] =
    "@16 v6 q2 [o4 c16 r16]8 [o3 a-16 r16]8 [o4 c16 r16]8 [o3 g16 r16]8 [o4 c16 r16]8 [o3 f16 r16]8 [o3 g16 r16]8 [o4 c16 r16]8";
static const char BATTLE_BASS[] =
    "@6 v14 q4 " E8("o2 c") E8("o1 a-") E8("o2 c") E8("o1 g") E8("o2 c") E8("o1 f") E8("o1 g") E8("o2 c");
static const char BATTLE_DRUM[] = "[@13 v12 o2 c8 @11 v9 o6 c8 @13 v10 o2 c8 @11 v10 o6 c8]16";

/* "The Ordeals" - the ordeals' board, B minor, a slow march */
static const char ORD_LEAD[] =
    "@14 v11 q7"
    "| o4 b4. a8 f+4 d4 | o4 e4 f+4 b2 | o5 c+4 d4 e4 f+4 | o5 e2 d4 c+4"
    "| o4 b4. a8 f+4 d4 | o4 g4 f+4 e4 c+4 | o4 d4 e4 f+4 a+4 | o4 b1";
static const char ORD_ARP[] =
    "@2 v5 q4 " ARP("o4 b", "o5 d", "f+", "d") ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 a", "o5 c+", "e", "c+")
    ARP("o4 f+", "a+", "o5 c+", "o4 a+") ARP("o4 b", "o5 d", "f+", "d") ARP("o4 e", "g", "b", "g")
    ARP("o4 f+", "a+", "o5 c+", "o4 a+") ARP("o4 b", "o5 d", "f+", "d");
static const char ORD_BASS[] =
    "@6 v12 q6 " Q4("o2 b", "f+", "b", "f+") Q4("o2 g", "d", "g", "d") Q4("o2 a", "e", "a", "e") Q4("o2 f+", "c+", "f+", "c+")
    Q4("o2 b", "f+", "b", "f+") Q4("o2 e", "b", "e", "b") Q4("o2 f+", "c+", "f+", "a+") Q4("o2 b", "f+", "b", "f+");
static const char ORD_DRUM[] = "[@13 v11 o2 c8 @11 v6 o6 c16 c16 @11 v9 o6 c8 @9 v3 o8 c8]16";

/* "What the Bones Remember" - the ending, F major, quiet */
static const char END_LEAD[] =
    "@14 v10 q7"
    "| o5 c4 f4 a4 g8 f8 | o5 g2 e4 c4 | o5 d4 f4 b-4 a8 g8 | o5 a2. r4"
    "| o5 c4 f4 a4 o6 c4 | o5 b-4 a4 g4 e4 | o5 f4 g8 a8 g4 e4 | o5 f1";
static const char END_COMP[] = "@22 v5 q6 [o4 f8 a8 o5 c8 o4 a8]2 [o4 e8 g8 o5 c8 o4 g8]2 [o4 d8 f8 b-8 f8]2 [o4 f8 a8 o5 c8 o4 a8]2"
                               " [o4 f8 a8 o5 c8 o4 a8]2 [o4 g8 b-8 o5 d8 o4 b-8]2 [o4 c8 e8 g8 e8]2 [o4 f8 a8 o5 c8 o4 a8]2";
static const char END_BASS[] = "@6 v11 q8 o2 f1 c1 b-1 f1 f1 g1 c1 f1";
static const char END_DRUM[] = "[@9 v2 o8 c4 @11 v3 o6 c4 @9 v2 o8 c4 @9 v2 o8 c4]8";

/* jingles */
static const char WIN_P1[] = "@1 v12 o5 l8 c e g o6 c4 o5 g8 o6 c8 e2";
static const char WIN_P2[] = "@4 v7 o4 l8 g o5 c e g4 e8 g8 c2";
static const char WIN_TRI[] = "@6 v12 o2 c4 g4 o3 c4 o2 g4 c2";
static const char WIN_NOISE[] = "@13 v10 o2 c4 c4 c4 c4 @12 v8 o5 c2";
static const char LOSE_P1[] = "@5 v11 o5 e4 d4 c4 o4 b4 a1";
static const char LOSE_TRI[] = "@6 v12 o2 a2 g2 f1";

void feud_audio_load(void) {
    if (FEUD_MUS_TITLE >= 0) return;
    FEUD_MUS_TITLE = song_define("fd_title", 96, true, TITLE_LEAD, TITLE_PAD, TITLE_BASS, TITLE_DRUM);
    FEUD_MUS_STORY = song_define("fd_story", 72, true, STORY_LEAD, STORY_PAD, STORY_BASS, STORY_SOFT);
    FEUD_MUS_MAP = song_define("fd_map", 104, true, MAP_LEAD, MAP_COMP, MAP_BASS, MAP_DRUM);
    FEUD_MUS_RIVAL = song_define("fd_rival", 100, true, RIVAL_LEAD, RIVAL_PAD, RIVAL_BASS, RIVAL_DRUM);
    FEUD_MUS_BATTLE = song_define("fd_battle", 150, true, BATTLE_LEAD, BATTLE_STAB, BATTLE_BASS, BATTLE_DRUM);
    FEUD_MUS_ORDEAL = song_define("fd_ordeal", 112, true, ORD_LEAD, ORD_ARP, ORD_BASS, ORD_DRUM);
    FEUD_MUS_ENDING = song_define("fd_ending", 88, true, END_LEAD, END_COMP, END_BASS, END_DRUM);
    FEUD_MUS_WIN = song_define("fd_win", 132, false, WIN_P1, WIN_P2, WIN_TRI, WIN_NOISE);
    FEUD_MUS_LOSE = song_define("fd_lose", 80, false, LOSE_P1, "", LOSE_TRI, "");

    sfx_define("fd_pray", CH_P1, 180, "@39 v11 o5 l16 a o6 c e a");
    sfx_define("fd_bless", CH_P1, 160, "@39 v13 o5 l16 c e g o6 c e g o7 c8");
    sfx_define("fd_gather", CH_P1, 220, "@35 v10 o6 c32 e32 g32");
    sfx_define("fd_buy", CH_P2, 240, "@35 v9 o6 e32 b32");
    sfx_define("fd_build", CH_NOISE, 200, "@13 v10 o3 c16 r32 c16");
    sfx_define("fd_step", CH_P2, 240, "@20 v8 o5 c32 g32");
    sfx_define("fd_clash", CH_NOISE, 140, "@40 v12 o5 c4");
    sfx_define("fd_peck", CH_NOISE, 240, "@21 v6 o6 c32");
    sfx_define("fd_sling", CH_NOISE, 240, "@36 v5 o6 c32");
    sfx_define("fd_zap", CH_P2, 220, "@39 v6 o6 c32 g32");
    sfx_define("fd_drop", CH_P2, 220, "@33 v6 o6 c16");
    sfx_define("fd_bomb", CH_NOISE, 200, "@34 v9 o4 c16");
    sfx_define("fd_tink", CH_P2, 240, "@42 v8 o7 c32");
    sfx_define("fd_die", CH_P1, 200, "@33 v8 o5 c16");
    sfx_define("fd_wreck", CH_NOISE, 140, "@34 v12 o3 c4");
    sfx_define("fd_alarm", CH_P1, 200, "@37 v10 o5 c16 r32 c16");
    sfx_define("fd_take", CH_P1, 160, "@39 v12 o5 l16 c e g o6 c4");
    sfx_define("fd_hold", CH_P1, 160, "@15 v11 o5 g8 c4");
    sfx_define("fd_wind", CH_NOISE, 160, "@36 v8 o4 c4");
    sfx_define("fd_quake", CH_NOISE, 120, "@34 v13 o2 c2");
    sfx_define("fd_hail", CH_P2, 200, "@15 v9 o7 c32 e32 c32 e32 c32");
    sfx_define("fd_thunder", CH_NOISE, 120, "@40 v15 o4 c4 @34 v10 o2 c4");
    sfx_define("fd_sight", CH_P1, 200, "@38 v10 o5 c16 g16 o6 c16");
    sfx_define("fd_hatch", CH_P1, 220, "@35 v10 o6 c32 r32 c32 e16");
    sfx_define("fd_secret", CH_P1, 120, "@15 v12 o4 a8 e8 o5 c8 o4 a4");
}
