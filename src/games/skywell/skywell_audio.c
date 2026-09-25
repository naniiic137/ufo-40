/* SKYWELL - original music (UFO-MML) and sound effects. */
#include "skywell.h"

int SW_MUS_LEVEL[3] = {-1, -1, -1};
int SW_MUS_BOSS, SW_MUS_TITLE, SW_MUS_SHOP, SW_MUS_OVER, SW_MUS_END;

#define EI(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define DRIVE "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 @13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "

/* "The Roots" - A minor, running */
static const char L1_LEAD[] =
    "@0 v11 q6"
    "| o5 a8 a8 o6 c8 o5 a8 e8 a8 g8 e8 | o5 f8 f8 a8 f8 c8 f8 e8 c8"
    "| o5 d8 d8 f8 d8 o4 a8 o5 d8 c8 d8 | o5 e8 g+8 b8 o6 e8 d4 o5 b4"
    "| o5 a8 a8 o6 c8 o5 a8 e8 a8 g8 e8 | o5 f8 f8 a8 f8 c8 f8 e8 c8"
    "| o5 d8 f8 a8 o6 d8 c8 o5 b8 g+8 e8 | o5 a2 r2";
#define AM16 "[o4 a16 o5 c16 e16 c16]4 "
#define FM16 "[o4 f16 a16 o5 c16 o4 a16]4 "
#define DM16 "[o4 d16 f16 a16 f16]4 "
#define EM16 "[o4 e16 g+16 b16 g+16]4 "
static const char L1_ARP[] = "@2 v5 q5 " AM16 FM16 DM16 EM16 AM16 FM16 DM16 AM16;
static const char L1_BASS[] = "@6 v14 q5 " EI("o2 a") EI("o2 f") EI("o2 d") EI("o2 e") EI("o2 a") EI("o2 f") EI("o2 d") EI("o2 a");
static const char L1_DRUM[] = "[" DRIVE "]8";

/* "The Sparkworks" - E minor, clipped and sparking */
static const char L2_LEAD[] =
    "@1 v11 q4"
    "| o5 e8 r8 e8 g8 r8 e8 d8 r8 | o5 e8 r8 e8 b8 r8 a8 g8 r8"
    "| o5 c8 r8 c8 e8 r8 c8 o4 b8 r8 | o4 b8 r8 o5 d+8 f+8 r8 a8 g8 f+8"
    "| o5 e8 r8 e8 g8 r8 e8 d8 r8 | o5 e8 r8 e8 b8 r8 a8 g8 r8"
    "| o5 c8 r8 c8 e8 r8 c8 o4 b8 r8 | o5 e2 r2";
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
static const char L2_OFF[] =
    "v6 q3 o4 " OFF("@17", "e") OFF("@17", "e") OFF("@16", "c") OFF("@16", "b")
    OFF("@17", "e") OFF("@17", "e") OFF("@16", "c") OFF("@17", "e");
static const char L2_BASS[] = "@6 v14 q4 " EI("o2 e") EI("o2 e") EI("o2 c") EI("o1 b") EI("o2 e") EI("o2 e") EI("o2 c") EI("o2 e");
static const char L2_DRUM[] = "[@13 v11 o2 c8 @9 v4 o8 c16 c16 @11 v9 o6 c8 @9 v4 o8 c16 c16]16";

/* "The Sky Reef" - D major, drifting */
static const char L3_LEAD[] =
    "@14 v11 q7"
    "| o5 f+4 a4 o6 d4 c+4 | o5 b2 a2 | o5 g4 b4 o6 e4 d4 | o6 c+1"
    "| o5 f+4 a4 o6 d4 e4 | o6 f+2 e4 d4 | o5 b4 o6 c+4 d4 e4 | o6 d1";
#define D8 "o4 d8 f+8 a8 o5 d8 o4 a8 f+8 d8 f+8 "
#define BM8 "o4 b8 o5 d8 f+8 b8 f+8 d8 o4 b8 o5 d8 "
#define G8 "o4 g8 b8 o5 d8 g8 d8 o4 b8 g8 b8 "
#define A8 "o4 a8 o5 c+8 e8 a8 e8 c+8 o4 a8 o5 c+8 "
#define FSM8 "o4 f+8 a8 o5 c+8 f+8 c+8 o4 a8 f+8 a8 "
static const char L3_ARP[] = "@2 v5 q5 " D8 BM8 G8 A8 D8 FSM8 G8 D8;
static const char L3_BASS[] = "@6 v12 q7 o3 d2 d2 o2 b2 b2 g2 g2 a2 a2 o3 d2 d2 o2 f+2 f+2 g2 a2 o3 d2 d2";
static const char L3_DRIFT[] = "[@9 v3 o8 c4 @21 v3 o7 c4 @9 v3 o8 c4 @21 v4 o7 c8 c8]8";

/* "The Eye" - C minor, the last climb */
static const char BOSS_LEAD[] =
    "@1 v12 q5"
    "| o5 c8 c8 e-8 c8 g8 c8 f+8 g8 | o5 c8 c8 e-8 c8 a-8 g8 f8 e-8"
    "| o5 d8 d8 f8 d8 a-8 d8 g8 f8 | o5 b-8 a-8 g8 f8 e-8 d8 o4 b8 g8"
    "| o5 c8 c8 e-8 c8 g8 c8 f+8 g8 | o5 c8 c8 e-8 c8 a-8 g8 f8 e-8"
    "| o5 d8 d8 f8 d8 a-8 d8 g8 f8 | o5 c2 o4 g2";
#define ST16(n) n "16 r16 " n "16 r16 " n "16 r16 " n "16 r16 " n "16 r16 " n "16 r16 " n "16 r16 " n "16 r16 "
static const char BOSS_STAB[] =
    "v6 q2 o4 @17 " ST16("c") "@16 " ST16("a-") "@17 " ST16("f") "@16 " ST16("g")
    "@17 " ST16("c") "@16 " ST16("a-") "@17 " ST16("f") "@17 " ST16("c");
static const char BOSS_BASS[] = "@6 v14 q4 " EI("o2 c") EI("o1 a-") EI("o2 f") EI("o2 g") EI("o2 c") EI("o1 a-") EI("o2 f") EI("o2 c");
static const char BOSS_DRUM[] = "[@13 v12 o2 c16 @9 v5 o8 c16 @11 v10 o6 c16 @9 v5 o8 c16]32";

/* "Skywell" - the title, G major */
static const char TITLE_BELL[] =
    "@15 v10 q7"
    "| o5 d4 g4 b4 a4 | o5 g2 d2 | o5 e4 g4 o6 c4 o5 b4 | o5 a1"
    "| o5 d4 g4 b4 o6 d4 | o6 e2 d4 c4 | o5 b4 a4 g4 f+4 | o5 g1";
#define G8B "o4 g8 b8 o5 d8 g8 d8 o4 b8 g8 b8 "
#define C8B "o4 c8 e8 g8 o5 c8 o4 g8 e8 c8 e8 "
#define D8B "o4 d8 f+8 a8 o5 d8 o4 a8 f+8 d8 f+8 "
#define E8B "o4 e8 g8 b8 o5 e8 o4 b8 g8 e8 g8 "
static const char TITLE_ARP[] = "@2 v5 q5 " G8B G8B C8B D8B G8B E8B C8B G8B;
static const char TITLE_BASS[] = "@6 v12 q7 o3 g1 g1 c1 d1 g1 e1 c2 d2 g1";

/* "The Tinker" - the shop, F major */
static const char SHOP_LEAD[] =
    "@3 v11 q6"
    "| o5 f8 a8 o6 c8 o5 a8 b-8 a8 g8 f8 | o5 e8 g8 b-8 g8 a8 g8 f8 e8"
    "| o5 d8 f8 a8 f8 g8 f8 e8 d8 | o5 c8 e8 g8 b-8 a4 f4";
static const char SHOP_BASS[] = "@6 v13 q5 | o3 f4 c4 f4 c4 | o3 c4 o2 g4 o3 c4 o2 g4 | o2 b-4 f4 b-4 f4 | o3 c4 o2 g4 o3 f2";
static const char SHOP_TICK[] = "[@21 v3 o7 c8 @9 v2 o8 c8]16";

/* "Dawn" - the ending, G major */
static const char END_LEAD[] =
    "@23 v12 q7"
    "| o5 g4 b4 o6 d4 g4 | o6 f+2 e4 d4 | o6 c4 e4 d4 c4 | o5 b2 a2"
    "| o5 g4 b4 o6 d4 g4 | o6 a2 g4 f+4 | o6 e4 c4 d4 f+4 | o6 g1";
static const char END_ARP[] = "@22 v6 q6 " G8B D8B C8B D8B G8B D8B C8B G8B;
static const char END_BASS[] = "@6 v13 q7 o3 g1 d1 c1 d1 g1 d1 c2 d2 g1";
static const char END_DRUM[] = "[@13 v9 o2 c4 @9 v4 o8 c4 @11 v7 o6 c4 @9 v4 o8 c4]8";

/* game over */
static const char OVER_P1[] = "@5 v11 o5 l8 e d c o4 b a g+ a2";
static const char OVER_TRI[] = "@6 v13 o3 l4 a e o2 a2";

void sw_audio_load(void) {
    if (SW_MUS_LEVEL[0] >= 0) return;
    SW_MUS_LEVEL[0] = song_define("sw_roots", 150, true, L1_LEAD, L1_ARP, L1_BASS, L1_DRUM);
    SW_MUS_LEVEL[1] = song_define("sw_spark", 138, true, L2_LEAD, L2_OFF, L2_BASS, L2_DRUM);
    SW_MUS_LEVEL[2] = song_define("sw_reef", 120, true, L3_LEAD, L3_ARP, L3_BASS, L3_DRIFT);
    SW_MUS_BOSS = song_define("sw_eye", 160, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    SW_MUS_TITLE = song_define("sw_title", 104, true, TITLE_BELL, TITLE_ARP, TITLE_BASS, "");
    SW_MUS_SHOP = song_define("sw_shop", 110, true, SHOP_LEAD, "", SHOP_BASS, SHOP_TICK);
    SW_MUS_END = song_define("sw_dawn", 112, true, END_LEAD, END_ARP, END_BASS, END_DRUM);
    SW_MUS_OVER = song_define("sw_over", 110, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("sw_jump", CH_P2, 240, "@32 v9 o5 c16");
    sfx_define("sw_jump2", CH_P2, 240, "@32 v9 o6 c16");
    sfx_define("sw_shoot", CH_P2, 240, "@20 v7 o6 a32");
    sfx_define("sw_hit", CH_NOISE, 240, "@21 v9 o6 c32");
    sfx_define("sw_crumble", CH_NOISE, 220, "@40 v9 o5 c16");
    sfx_define("sw_puff", CH_NOISE, 200, "@36 v9 o6 c8");
    sfx_define("sw_cog", CH_P1, 240, "@35 v10 o6 e16 b16");
    sfx_define("sw_key", CH_P1, 200, "@39 v12 o5 l16 c e g o6 c e g");
    sfx_define("sw_star", CH_P1, 220, "@39 v12 o5 l32 c d e f g a b o6 c d e f g");
    sfx_define("sw_stun", CH_P2, 220, "@37 v11 o4 c16 o3 g16");
    sfx_define("sw_die", CH_P1, 160, "@33 v13 o5 c8 o4 g8 e8 c4");
    sfx_define("sw_tick", CH_P2, 240, "@42 v8 o6 c32 r32 c32");
    sfx_define("sw_boom", CH_NOISE, 140, "@34 v14 o3 c4");
    sfx_define("sw_bat", CH_P2, 240, "@33 v8 o7 c16");
    sfx_define("sw_boing", CH_P2, 220, "@32 v10 o4 c16 g16");
    sfx_define("sw_owl", CH_P1, 180, "@5 v10 o6 e8 c8 o5 a8");
    sfx_define("sw_orb", CH_P2, 220, "@20 v8 o6 e16 c16");
    sfx_define("sw_zap", CH_NOISE, 220, "@36 v11 o7 c8");
    sfx_define("sw_clear", CH_P1, 180, "@23 v12 o5 l16 g o6 c e g8");
    sfx_define("sw_buy", CH_P1, 220, "@39 v11 o5 l16 e g o6 c");
}
