/* BRAVADO - original music (UFO-MML) and sound effects. Every looping tune
 * is eight bars a channel, so the channels loop together. */
#include "bravado.h"

int BRV_MUS_TITLE = -1, BRV_MUS_SHOP, BRV_MUS_FIGHT1, BRV_MUS_FIGHT2, BRV_MUS_LAST, BRV_MUS_BOSS, BRV_MUS_WIN,
    BRV_MUS_OVER, BRV_MUS_ENDING, BRV_MUS_STORY;

/* one bar of quarter notes, of eighths, of off-beat chord stabs */
#define Q4(a, b, c, d) a "4 " b "4 " c "4 " d "4 "
#define EI(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define OFFB(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
#define ST16(n) "[" n "16 r16]8 "
#define OCT(n) "[o2 " n "8 o3 " n "8]4 "
#define OCT1(n) "[o1 " n "8 o2 " n "8]4 "
/* half a bar of kick, hat, snare, hat */
#define DR2 "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "

/* "All In" - the title, C minor */
static const char TITLE_LEAD[] =
    "@1 v11 q6"
    "| o5 c8 e-8 g8 b-8 a-4 g4 | o5 f8 a-8 o6 c8 o5 a-8 g2 | o5 e-8 g8 b-8 o6 d8 c4 o5 b-4 | o5 g8 f8 e-8 d8 c2"
    "| o5 c8 e-8 g8 o6 c8 e-4 d4 | o6 c8 o5 a-8 f8 a-8 g4 f4 | o5 e-8 f8 g8 a-8 b-8 a-8 g8 f8 | o5 e-2 c2";
static const char TITLE_COMP[] =
    "v6 q3 " OFFB("@17 o4", "c") OFFB("@17 o4", "f") OFFB("@16 o4", "e-") OFFB("@16 o4", "g")
    OFFB("@17 o4", "c") OFFB("@17 o4", "f") OFFB("@16 o4", "e-") OFFB("@17 o4", "c");
static const char TITLE_BASS[] =
    "@6 v13 q6 " Q4("o2 c", "g", "c", "g") Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 e-", "b-", "e-", "b-")
    Q4("o2 g", "o3 d", "o2 g", "b") Q4("o2 c", "g", "c", "g") Q4("o2 f", "o3 c", "o2 f", "o3 c")
    Q4("o2 e-", "b-", "e-", "b-") Q4("o2 c", "g", "c", "g");
static const char TITLE_DRUM[] = "[" DR2 DR2 "]8";

/* "Down on My Luck" - the story, A minor */
static const char STORY_LEAD[] =
    "@5 v10 q7"
    "| o5 e4 a4 g8 e8 d4 | o5 c4 e4 d2 | o5 e4 a4 o6 c8 o5 b8 a4 | o5 g+2 e2"
    "| o5 a4 o6 c4 o5 b8 a8 g4 | o5 f4 a4 g2 | o5 e8 f8 e8 d8 c4 o4 b4 | o5 a1";
static const char STORY_ARP[] =
    "@2 v5 q5 [o4 a8 o5 c8 e8 c8]2 [o4 f8 a8 o5 c8 o4 a8]2 [o4 a8 o5 c8 e8 c8]2 [o4 e8 g+8 b8 g+8]2"
    " [o4 a8 o5 c8 e8 c8]2 [o4 f8 a8 o5 c8 o4 a8]2 o4 d8 f8 a8 f8 o4 e8 g+8 b8 g+8 [o4 a8 o5 c8 e8 c8]2";
static const char STORY_BASS[] = "@6 v12 q7 o2 a2 a2 f2 f2 a2 a2 e2 e2 a2 a2 f2 f2 d2 e2 a1";
static const char STORY_SOFT[] = "[@9 v2 o8 c4]32";

/* "House Money" - the shop, F major */
static const char SHOP_LEAD[] =
    "@14 v11 q7"
    "| o5 a4 g8 f8 c4 f4 | o5 g4 f8 e8 d2 | o5 b-4 a8 g8 e4 g4 | o5 f2 c2"
    "| o5 a4 o6 c8 o5 a8 f4 a4 | o5 g4 b-8 g8 e4 c4 | o5 d8 e8 f8 g8 a8 g8 e8 c8 | o5 f2 r2";
static const char SHOP_COMP[] =
    "v6 q3 " OFFB("@16 o4", "f") OFFB("@17 o4", "d") OFFB("@17 o3", "g") OFFB("@16 o4", "f")
    OFFB("@16 o4", "f") OFFB("@16 o4", "c") OFFB("@16 o3", "b-") OFFB("@16 o4", "f");
static const char SHOP_BASS[] =
    "@6 v12 q6 " Q4("o2 f", "a", "o3 c", "o2 a") Q4("o2 d", "f", "a", "f") Q4("o2 g", "b-", "o3 d", "o2 b-")
    Q4("o2 f", "a", "o3 c", "o2 a") Q4("o2 f", "a", "o3 c", "o2 a") Q4("o2 c", "e", "g", "e")
    Q4("o1 b-", "o2 d", "f", "d") Q4("o2 f", "a", "o3 c", "o2 a");
static const char SHOP_DRUM[] = "[@9 v4 o8 c8 @9 v3 o8 c8 @11 v6 o6 c8 @9 v3 o8 c8]16";

/* "The Glass Pit" - fights 1 to 3, E minor */
static const char F1_LEAD[] =
    "@1 v11 q6"
    "| o5 e8 e8 g8 e8 b8 e8 a8 g8 | o5 f+8 f+8 a8 f+8 o6 d8 o5 f+8 b8 a8 | o5 g8 g8 b8 g8 o6 e8 o5 g8 o6 d8 c8 | o5 b4 a4 g4 f+4"
    "| o5 e8 e8 g8 e8 b8 e8 o6 c8 o5 b8 | o5 a8 a8 o6 c8 o5 a8 o6 e8 o5 a8 o6 d8 c8 | o5 b8 a8 g8 f+8 g8 a8 b8 o6 d+8 | o6 e2 r2";
static const char F1_ARP[] =
    "@2 v5 q5 " ARP("o4 e", "g", "b", "g") ARP("o4 d", "f+", "a", "f+") ARP("o4 c", "e", "g", "e")
    ARP("o3 b", "o4 d+", "f+", "d+") ARP("o4 e", "g", "b", "g") ARP("o4 a", "o5 c", "e", "c")
    ARP("o3 b", "o4 d+", "f+", "d+") ARP("o4 e", "g", "b", "g");
static const char F1_BASS[] =
    "@6 v14 q4 " EI("o2 e") EI("o2 d") EI("o2 c") EI("o1 b") EI("o2 e") EI("o2 a") EI("o1 b") EI("o2 e");
static const char F1_DRUM[] = "[" DR2 DR2 "]8";

/* "Double or Nothing" - fights 4 to 7, D minor */
static const char F2_LEAD[] =
    "@0 v11 q5"
    "| o5 d8 r8 d8 f8 a8 r8 g8 f8 | o5 e8 r8 e8 g8 o6 c8 r8 o5 b-8 a8 | o5 f8 r8 f8 a8 o6 d8 c8 o5 b-8 a8 | o5 g8 a8 b-8 a8 g8 f8 e8 c+8"
    "| o5 d8 r8 d8 f8 a8 r8 o6 d8 c8 | o5 b-8 r8 b-8 o6 d8 f8 r8 e8 d8 | o6 c8 o5 b-8 a8 g8 f8 e8 f8 g8 | o5 a8 c+8 e8 a8 d4 r4";
static const char F2_STAB[] =
    "v6 q2 @17 o4 " ST16("d") "@16 o4 " ST16("c") "@16 o3 " ST16("b-") "@16 o3 " ST16("a")
    "@17 o4 " ST16("d") "@16 o3 " ST16("b-") "@16 o4 " ST16("c") "@16 o3 " ST16("a");
static const char F2_BASS[] =
    "@6 v14 q4 " OCT("d") OCT("c") OCT1("b-") OCT1("a") OCT("d") OCT1("b-") OCT("c") OCT1("a");
static const char F2_DRUM[] = "[@13 v11 o2 c8 @9 v4 o8 c16 c16 @11 v9 o6 c8 @9 v4 o8 c16 c16]16";

/* "Full House" - the last fight, G minor */
static const char LAST_LEAD[] =
    "@1 v12 q6"
    "| o5 g8 b-8 o6 d8 o5 g8 o6 f8 d8 e-8 d8 | o6 c8 o5 a8 f8 a8 o6 e-8 c8 d8 c8 | o5 b-8 g8 e-8 g8 o6 d8 o5 b-8 o6 c8 o5 b-8 | o5 a8 f+8 d8 f+8 a4 o6 d4"
    "| o6 g8 f8 e-8 d8 c8 o5 b-8 a8 g8 | o5 f8 a8 o6 c8 e-8 d8 c8 o5 b-8 a8 | o5 b-8 o6 c8 d8 e-8 f8 e-8 d8 c8 | o5 b-4 a4 g2";
static const char LAST_ARP[] =
    "@2 v5 q5 " ARP("o4 g", "b-", "o5 d", "o4 b-") ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 e-", "g", "b-", "g")
    ARP("o4 d", "f+", "a", "f+") ARP("o4 c", "e-", "g", "e-") ARP("o4 f", "a", "o5 c", "o4 a")
    ARP("o4 e-", "g", "b-", "g") ARP("o4 d", "f+", "a", "f+");
static const char LAST_BASS[] =
    "@6 v14 q4 " EI("o2 g") EI("o2 f") EI("o2 e-") EI("o2 d") EI("o2 c") EI("o2 f") EI("o2 e-") EI("o2 d");
static const char LAST_DRUM[] = "[@13 v12 o2 c16 @9 v5 o8 c16 @11 v10 o6 c16 @9 v5 o8 c16]32";

/* "The Pit Boss" - C minor, heavy */
static const char BOSS_LEAD[] =
    "@0 v12 q5"
    "| o4 c8 c8 o5 c8 o4 c8 b-8 o5 c8 e-8 f8 | o5 g8 g8 f8 e-8 f8 e-8 d8 o4 b-8 | o4 a-8 a-8 o5 a-8 o4 a-8 g8 o5 a-8 g8 f8 | o5 e-8 d8 c8 d8 o4 b8 g8 b8 o5 d8"
    "| o4 c8 c8 o5 c8 o4 c8 b-8 o5 c8 e-8 f8 | o5 g8 a-8 b-8 o6 c8 o5 b-8 a-8 g8 f8 | o4 a-8 a-8 o5 a-8 o4 a-8 g8 o5 a-8 g8 f8 | o5 e-8 d8 c8 o4 b8 o5 c2";
static const char BOSS_STAB[] =
    "v7 q2 @17 o4 " ST16("c") "@16 o4 " ST16("e-") "@16 o3 " ST16("a-") "@16 o3 " ST16("g")
    "@17 o4 " ST16("c") "@16 o4 " ST16("e-") "@16 o3 " ST16("a-") "@16 o3 " ST16("g");
static const char BOSS_BASS[] =
    "@6 v15 q4 " EI("o2 c") EI("o2 e-") EI("o1 a-") EI("o1 g") EI("o2 c") EI("o2 e-") EI("o1 a-") EI("o1 g");
static const char BOSS_DRUM[] = "[@13 v13 o2 c8 @11 v10 o6 c16 @13 v10 o2 c16 @13 v12 o2 c8 @11 v11 o6 c8]16";

/* "Cashing Out" - the ending, C major */
static const char END_LEAD[] =
    "@14 v11 q7"
    "| o5 c8 d8 e8 g8 o6 c4 o5 b4 | o5 a8 g8 a8 o6 c8 o5 g2 | o5 f8 e8 f8 a8 o6 d4 c4 | o5 b8 a8 b8 o6 d8 c2"
    "| o6 e8 d8 c8 o5 a8 o6 d8 c8 o5 b8 g8 | o5 a8 b8 o6 c8 d8 e4 d4 | o6 c8 o5 b8 a8 g8 f8 e8 d8 e8 | o5 c2 r2";
static const char END_COMP[] =
    "v6 q3 " OFFB("@16 o4", "c") OFFB("@17 o4", "a") OFFB("@17 o4", "d") OFFB("@16 o4", "g")
    OFFB("@17 o4", "a") OFFB("@16 o4", "f") OFFB("@16 o4", "g") OFFB("@16 o4", "c");
static const char END_BASS[] =
    "@6 v12 q6 " Q4("o2 c", "g", "e", "g") Q4("o2 a", "e", "c", "e") Q4("o2 d", "a", "f", "a") Q4("o2 g", "d", "b", "d")
    Q4("o2 a", "e", "c", "e") Q4("o2 f", "c", "a", "c") Q4("o2 g", "d", "b", "d") Q4("o2 c", "g", "e", "g");
static const char END_DRUM[] = "[@13 v10 o2 c4 @11 v8 o6 c4]16";

/* jingles */
static const char WIN_P1[] = "@39 v12 o5 l16 c e g o6 c8 o5 g16 o6 c16 e4 g4";
static const char WIN_P2[] = "@20 v8 o5 l16 e g o6 c e8 c16 e16 g4 o7 c4";
static const char WIN_TRI[] = "@6 v12 o3 c8 g8 o4 c4 o3 c4";
static const char WIN_NOISE[] = "@11 v9 o6 c8 c8 @12 v8 o5 c2";
static const char OVER_P1[] = "@0 v11 o5 g8 f+8 f8 e4 e-4 d2";
static const char OVER_TRI[] = "@6 v12 o2 g4 f+4 f4 e2";

void brv_audio_load(void) {
    if (BRV_MUS_TITLE >= 0) return;
    BRV_MUS_TITLE = song_define("brv_title", 132, true, TITLE_LEAD, TITLE_COMP, TITLE_BASS, TITLE_DRUM);
    BRV_MUS_STORY = song_define("brv_story", 88, true, STORY_LEAD, STORY_ARP, STORY_BASS, STORY_SOFT);
    BRV_MUS_SHOP = song_define("brv_shop", 112, true, SHOP_LEAD, SHOP_COMP, SHOP_BASS, SHOP_DRUM);
    BRV_MUS_FIGHT1 = song_define("brv_fight1", 150, true, F1_LEAD, F1_ARP, F1_BASS, F1_DRUM);
    BRV_MUS_FIGHT2 = song_define("brv_fight2", 160, true, F2_LEAD, F2_STAB, F2_BASS, F2_DRUM);
    BRV_MUS_LAST = song_define("brv_last", 168, true, LAST_LEAD, LAST_ARP, LAST_BASS, LAST_DRUM);
    BRV_MUS_BOSS = song_define("brv_boss", 150, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    BRV_MUS_ENDING = song_define("brv_ending", 120, true, END_LEAD, END_COMP, END_BASS, END_DRUM);
    BRV_MUS_WIN = song_define("brv_win", 150, false, WIN_P1, WIN_P2, WIN_TRI, WIN_NOISE);
    BRV_MUS_OVER = song_define("brv_over", 100, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("brv_shot", CH_P2, 240, "@20 v5 o6 e32");
    sfx_define("brv_pop", CH_NOISE, 240, "@21 v8 o5 c32 c32");
    sfx_define("brv_boom", CH_NOISE, 140, "@34 v14 o3 c4");
    sfx_define("brv_bossdie", CH_NOISE, 100, "@34 v15 o2 c2");
    sfx_define("brv_tick", CH_P2, 240, "@42 v6 o5 g32");
    sfx_define("brv_guard", CH_P1, 200, "@15 v11 o6 c16 g16");
    sfx_define("brv_sizzle", CH_NOISE, 220, "@36 v8 o7 c16");
    sfx_define("brv_hurt", CH_P1, 200, "@37 v12 o4 e8");
    sfx_define("brv_die", CH_P1, 160, "@33 v13 o5 c8 o4 g8 e8 c4");
    sfx_define("brv_drop", CH_P2, 220, "@41 v10 o4 c16");
    sfx_define("brv_click", CH_P2, 240, "@42 v10 o6 c32 r32 c32");
    sfx_define("brv_dash", CH_P1, 220, "@32 v12 o4 c16 g16 o5 c16");
    sfx_define("brv_heal", CH_P1, 220, "@39 v11 o5 l32 c e g o6 c");
    sfx_define("brv_bonk", CH_NOISE, 200, "@40 v13 o4 c8");
    sfx_define("brv_warp", CH_P2, 220, "@38 v7 o4 c16 g16");
    sfx_define("brv_bossin", CH_P1, 140, "@33 v13 o3 c4 o2 g4");
    sfx_define("brv_medin", CH_P2, 220, "@35 v9 o6 e16 g16");
    sfx_define("brv_tone", CH_P1, 200, "@15 v12 o6 e8 r16 e8");
    sfx_define("brv_sprout", CH_NOISE, 200, "@36 v8 o5 c8");
    sfx_define("brv_efire", CH_NOISE, 240, "@36 v6 o6 c32");
    sfx_define("brv_keg", CH_P2, 220, "@37 v8 o5 c16 c16");
    sfx_define("brv_raise", CH_P1, 220, "@35 v11 o5 c16 g16 o6 c16");
    sfx_define("brv_bell", CH_P1, 180, "@15 v12 o6 c8 o5 g8 o6 c4");
    sfx_define("brv_buy", CH_P1, 220, "@35 v12 o6 c16 e16");
}
