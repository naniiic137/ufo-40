/* ROOFCAT - original music (UFO-MML) and sound effects. */
#include "roofcat.h"

int RC_MUS_AREA[RC_AREAS] = {-1, -1, -1, -1};
int RC_MUS_BOSS = -1, RC_MUS_OVER, RC_MUS_END, RC_MUS_TITLE;

/* ---- stage 1: "Rooftop Run" - C major, bouncy. C G Am F | C G F G */
static const char S1_LEAD[] =
    "@1 v12 q6"
    "| o5 c8 e8 g8 e8 c8 e8 g4 | o5 d8 g8 b8 g8 d4 r4 | o5 e8 a8 o6 c8 o5 a8 e8 c8 e4 | o5 f8 a8 o6 c8 o5 a8 f4. r8"
    "| o5 g4 e8 g8 o6 c4 o5 g4 | o5 b4 a8 g8 d4 g4 | o5 a8 g8 f8 e8 d8 c8 d8 e8 | o5 d2 g4 r4"
    "| o6 c8 o5 g8 e8 g8 o6 c8 e8 d8 c8 | o5 b8 g8 d8 g8 b8 o6 d8 c8 o5 b8 | o5 a8 e8 c8 e8 a8 o6 c8 o5 b8 a8 | o5 f4 a4 o6 c4 o5 a4"
    "| o5 g8 f8 e8 d8 c4 e4 | o5 d8 e8 f8 g8 a4 b4 | o6 c4. o5 a8 f4 a4 | o5 g2 b4 o6 d4";
#define OFF(ins, oct, n) "r8 " ins " " oct " " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
static const char S1_HARM[] =
    "v6 q4 [" OFF("@16", "o4", "c") OFF("@16", "o3", "g") OFF("@17", "o3", "a") OFF("@16", "o3", "f")
    OFF("@16", "o4", "c") OFF("@16", "o3", "g") OFF("@16", "o3", "f") OFF("@16", "o3", "g") "]2";
#define BOUNCE(n) "o2 " n "8 o3 " n "8 o2 " n "8 o3 " n "8 o2 " n "8 o3 " n "8 o2 " n "8 o3 " n "8 "
static const char S1_BASS[] =
    "@6 v15 q5 [" BOUNCE("c") BOUNCE("g") BOUNCE("a") BOUNCE("f") BOUNCE("c") BOUNCE("g") BOUNCE("f") BOUNCE("g") "]2";
#define S1_DR "o2@13v12c8 o8@9v5c8 o6@11v10c8 o8@9v5c8 o2@13v12c8 o8@9v5c8 o6@11v10c8 o8@9v5c8 "
#define S1_DF "o2@13v12c8 o8@9v5c8 o6@11v10c8 o8@9v5c8 o6@11v9c16 c16 o6@11v11c16 c16 o6@11v12c8 o8@10v7c8 "
static const char S1_DRUMS[] = "[" S1_DR S1_DR S1_DR S1_DF "]4";

/* ---- stage 2: "Spice Market" - D Hijaz, maqsum rhythm */
static const char S2_LEAD[] =
    "@14 v12 q7"
    "| o5 d8 e-8 f+8 g8 a4 g8 f+8 | o5 e-8 f+8 e-8 d8 c4 d4 | o5 d8 f+8 a8 b-8 a8 g8 f+8 e-8 | o5 d2 r4 a8 g8"
    "| o5 a8 b-8 a8 g8 f+8 g8 a4 | o5 b-8 a8 g8 f+8 e-4 f+4 | o5 g8 f+8 e-8 d8 e-8 f+8 e-8 d8 | o5 d2. r4"
    "| o6 d16 e-16 d8 c8 o5 b-8 a4 f+8 g8 | o5 a8 b-8 o6 c8 d8 e-4 d4 | o6 d8 o5 a8 f+8 d8 e-8 f+8 g8 a8 | o5 b-4 a4 g4 f+4"
    "| o5 g16 a16 g8 f+8 e-8 d4 f+4 | o5 e-8 f+8 g8 a8 b-4 a4 | o5 g8 f+8 e-8 f+8 g4 f+8 e-8 | o5 d2 r2";
#define ARP4(a, b, c) "o4 " a " " b " o5 " c " o4 " b " " a " " b " o5 " c " o4 " b " "
static const char S2_HARM[] =
    "@3 v7 q5 l8 [" ARP4("d", "a", "d") ARP4("c", "g", "c") ARP4("d", "a", "d") ARP4("g", "b-", "d")
    ARP4("d", "a", "d") ARP4("e-", "b-", "e-") ARP4("d", "a", "d") ARP4("d", "a", "d") "]2";
#define MAQ_B(n) "o2 " n "8 r16 " n "16 r8 " n "8 o1 a8 r8 o2 " n "8 r8 "
static const char S2_BASS[] =
    "@6 v15 q5 [" MAQ_B("d") MAQ_B("c") MAQ_B("d") MAQ_B("g") MAQ_B("d") MAQ_B("e-") MAQ_B("d") MAQ_B("d") "]2";
#define MAQ "o2@13v12c8 o7@21v9c8 r8 o7@21v9c8 o2@13v12c8 r8 o7@21v9c8 o8@9v5c8 "
static const char S2_DRUMS[] = "[" MAQ "]16";

/* ---- stage 3: "Harbour Breeze" - A minor, lilting triplets */
static const char S3_LEAD[] =
    "@5 v12 q7"
    "| o5 e4 a12 g12 e12 c4 e4 | o5 f4 a12 g12 f12 c4. r8 | o5 g4 o6 c12 o5 b12 g12 e4 g4 | o5 d4. g8 b4 a4"
    "| o5 a4 o6 c12 o5 b12 a12 e4 a4 | o5 a4 g12 f12 e12 f4 c4 | o5 b4 g+12 a12 b12 o6 d4 c4 | o5 b2 g+4 e4";
static const char S3_HARM[] =
    "@2 v6 q5 [o4 a12 o5 c12 e12]4 [o4 f12 a12 o5 c12]4 [o4 c12 e12 g12]4 [o3 g12 b12 o4 d12]4"
    " [o4 a12 o5 c12 e12]4 [o4 f12 a12 o5 c12]4 [o3 e12 g+12 b12]4 [o3 e12 g+12 b12]4";
#define LILT(r, f) "o2 " r "4. " r "8 o3 " f "4 o2 " r "4 "
static const char S3_BASS[] =
    "@6 v14 q6 " LILT("a", "e") LILT("f", "c") LILT("c", "g") LILT("g", "d") LILT("a", "e") LILT("f", "c") LILT("e", "b") LILT("e", "b");
#define SWAY "o2@13v10c4 o8@9v4c12 c12 c12 o6@11v8c4 o8@9v4c12 c12 c12 "
static const char S3_DRUMS[] = "[" SWAY "]8";

/* ---- stage 4: "Fort at Midnight" - E minor, driving. Em C D B | Em C Am B */
static const char S4_LEAD[] =
    "@1 v13 q6"
    "| o5 e8 e16 g16 b8 e8 d8 e8 g8 a8 | o5 g8 g16 f+16 e8 c8 e8 g8 o6 c8 o5 b8"
    "| o5 a8 a16 g16 f+8 d8 f+8 a8 o6 d8 c8 | o5 b4 a8 g8 f+4 d+4"
    "| o5 e8 e16 g16 b8 e8 d8 e8 g8 a8 | o5 g8 a8 b8 o6 c8 d8 e8 d8 c8"
    "| o5 b8 a8 g8 e8 a4 c4 | o5 b8 o6 d+8 f+8 b8 ^2";
#define STAB5(oct, n) oct " " n "8 " n "8 r8 " n "8 r8 " n "8 " n "8 r8 "
static const char S4_HARM[] =
    "@22 v8 q3 " STAB5("o4", "e") STAB5("o4", "c") STAB5("o4", "d") STAB5("o3", "b") STAB5("o4", "e") STAB5("o4", "c")
    STAB5("o3", "a") STAB5("o3", "b");
#define GALLOP(n) "o2 " n "8 " n "8 o3 " n "8 o2 " n "8 " n "8 " n "8 o3 " n "8 o2 " n "8 "
static const char S4_BASS[] =
    "@6 v15 q5 " GALLOP("e") GALLOP("c") GALLOP("d") GALLOP("b") GALLOP("e") GALLOP("c") GALLOP("a") GALLOP("b");
#define S4_DR "o2@13v13c8 o8@9v6c8 o6@11v12c8 o8@9v6c8 o2@13v13c8 o2@13v11c8 o6@11v12c8 o8@9v6c8 "
#define S4_DF "o2@13v13c8 o8@9v6c8 o6@11v12c8 o8@9v6c8 o6@11v10c16 c16 o6@11v12c16 c16 o6@11v13c8 o8@10v9c8 "
static const char S4_DRUMS[] = "[" S4_DR S4_DR S4_DR S4_DF "]2";

/* ---- boss: "Magpie Mayhem" - G minor, frantic */
static const char BOSS_LEAD[] =
    "@1 v13 q6 [o5 g16 b-16 o6 d16 o5 b-16 g8 d8 g8 a8 b-8 a8 "
    "o5 g16 b-16 o6 e-16 o5 b-16 g8 e-8 g8 b-8 o6 e-8 d8 "
    "o5 a16 o6 c16 f16 c16 o5 a8 f8 a8 o6 c8 f8 e-8 "
    "o6 d8 c8 o5 b-8 a8 f+4 d4]2";
#define BSTAB(ins, n) ins " o4 " n "8 r8 " n "8 " n "8 r8 " n "8 r8 " n "8 "
static const char BOSS_HARM[] = "v8 q3 [" BSTAB("@17", "g") BSTAB("@16", "e-") BSTAB("@16", "f") BSTAB("@16", "d") "]2";
static const char BOSS_BASS[] = "@6 v15 q5 l8 [[o2 g o3 g]4 [o2 e- o3 e-]4 [o2 f o3 f]4 [o2 d o3 d]4]2";
static const char BOSS_DRUMS[] = "[o2@13v13c8 o8@9v6c8 o6@11v12c8 o2@13v12c8 o2@13v13c8 o8@9v6c8 o6@11v12c8 o8@10v8c8]8";

/* ---- jingles */
static const char OVER_P1[] = "@5 v12 o5 l8 c o4 b a g4 f4 e2.";
static const char OVER_TRI[] = "@6 v13 o2 l4 a g f e a2";

void rc_audio_load(void) {
    if (RC_MUS_BOSS >= 0) return;
    RC_MUS_AREA[0] = song_define("rc_rooftops", 150, true, S1_LEAD, S1_HARM, S1_BASS, S1_DRUMS);
    RC_MUS_AREA[1] = song_define("rc_souk", 138, true, S2_LEAD, S2_HARM, S2_BASS, S2_DRUMS);
    RC_MUS_AREA[2] = song_define("rc_fort", 164, true, S4_LEAD, S4_HARM, S4_BASS, S4_DRUMS);
    RC_MUS_AREA[3] = song_define("rc_harbour", 126, true, S3_LEAD, S3_HARM, S3_BASS, S3_DRUMS);
    RC_MUS_BOSS = song_define("rc_boss", 172, true, BOSS_LEAD, BOSS_HARM, BOSS_BASS, BOSS_DRUMS);
    RC_MUS_OVER = song_define("rc_over", 100, false, OVER_P1, "", OVER_TRI, "");
    RC_MUS_END = song_define("rc_ending", 112, true, S1_LEAD, S1_HARM, S1_BASS, S1_DRUMS);
    RC_MUS_TITLE = RC_MUS_AREA[0];

    sfx_define("rc_jump", CH_P2, 240, "@32 v10 o5 c16");
    sfx_define("rc_jump2", CH_P2, 240, "@32 v10 o6 c16");
    sfx_define("rc_throw", CH_NOISE, 240, "@36 v7 o7 c32");
    sfx_define("rc_hit", CH_NOISE, 220, "@21 v11 o6 c16 @13 v11 o3 c16");
    sfx_define("rc_coin", CH_P2, 220, "@35 v10 o6 c16 g16");
    sfx_define("rc_food", CH_P2, 220, "@39 v10 o5 e16 g16 o6 c8");
    sfx_define("rc_power", CH_P1, 180, "@16 v12 o5 c16 e16 g16 o6 c8");
    sfx_define("rc_oneup", CH_P1, 160, "@39 v12 o5 c8 e8 g8 o6 c8 e4");
    sfx_define("rc_die", CH_P1, 160, "@33 v13 o5 g8 e8 c4");
    sfx_define("rc_spirit", CH_P2, 120, "@4 v9 o4 e4 b4");
    sfx_define("rc_revive", CH_P2, 200, "@38 v11 o4 c16 g16 o5 c8");
    sfx_define("rc_boom", CH_NOISE, 140, "@34 v14 o3 c4");
    sfx_define("rc_bosshit", CH_P2, 240, "@37 v11 o4 c16");
    sfx_define("rc_shoot", CH_NOISE, 240, "@36 v8 o5 c32");
    sfx_define("rc_letter", CH_P1, 160, "@15 v12 o6 e8 g8 b8 o7 e4");
    sfx_define("rc_secret", CH_P1, 180, "@39 v11 o6 c16 e16 g16 o7 c16 e8");
    sfx_define("rc_splash", CH_NOISE, 200, "@36 v9 o4 c16 @21 v7 o6 c16");
    sfx_define("rc_fuse", CH_NOISE, 240, "@21 v7 o8 c32 c32 c32 c32 c32 c32");
    sfx_define("rc_beam", CH_P2, 240, "@33 v11 o7 c16 o6 c16");
}
