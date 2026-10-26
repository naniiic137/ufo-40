/* KIPPER - original music (UFO-MML) and sound effects. Every looping tune is
 * eight bars a channel, so the channels loop together: a theme for each of
 * the three areas, one for the docks, one for the big creatures, one for
 * Xolloth, the title, the story and the ending. */
#include "kpr.h"

int KPR_MUS_TITLE = -1, KPR_MUS_INTRO, KPR_MUS_KELP, KPR_MUS_ARCADE, KPR_MUS_BLACK, KPR_MUS_DOCK, KPR_MUS_BOSS,
    KPR_MUS_FINAL, KPR_MUS_ENDING, KPR_MUS_ITEM, KPR_MUS_TOWED;

#define Q4(a, b, c, d) a "4 " b "4 " c "4 " d "4 "
#define E8(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
#define ROCK "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
#define BUSY "@13 v12 o2 c8 @9 v5 o8 c16 c16 @11 v10 o6 c8 @9 v5 o8 c16 c16 "
#define SOFT "@13 v9 o2 c4 @9 v3 o8 c4 @11 v6 o6 c4 @9 v3 o8 c4 "

/* "Kipper" - the title, C major, jaunty */
static const char TITLE_LEAD[] =
    "@1 v12 q6 o5 | c4 e8 g8 e4 c4 | d4 f8 a8 g2 | e4 g8 o6 c8 o5 b4 g4 | a2 g2"
    "| c4 e8 g8 e4 c4 | d4 f8 a8 b4 g4 | o6 c4 o5 g4 e4 d4 | c1";
static const char TITLE_ARP[] =
    "v6 q5 o3 @16 " E8("c") "@17 " E8("d") "@16 " E8("c") "@16 " E8("f") "@16 " E8("c") "@16 " E8("g")
    "@16 " E8("c") "@16 " E8("c");
static const char TITLE_BASS[] =
    "@6 v13 q6 " Q4("o2 c", "o2 g", "o2 c", "o2 g") Q4("o2 d", "o2 a", "o2 d", "o2 a") Q4("o2 c", "o2 g", "o2 c", "o2 g")
    Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 c", "o2 g", "o2 c", "o2 g") Q4("o2 g", "o3 d", "o2 g", "o3 d")
    Q4("o2 c", "o2 g", "o2 c", "o2 g") Q4("o2 c", "o2 g", "o2 c", "o2 c");
static const char TITLE_DRUM[] = "[" ROCK ROCK "]8";

/* "Casting Off" - the story, slow */
static const char INTRO_LEAD[] = "@5 v10 q7 o4 | e2 g2 | a2. g4 | f2 e4 d4 | e1 | e2 g2 | o5 c2. o4 b4 | a4 g4 f4 d4 | c1";
static const char INTRO_PAD[] = "@4 v5 q8 o3 c1 a1 f1 e1 c1 a1 f1 g1";
static const char INTRO_BASS[] = "@6 v11 q8 o2 c1 f1 d1 e1 c1 f1 g1 c1";
static const char INTRO_SOFT[] = "[@9 v2 o8 c2]16";

/* "The Kelpwood" - the first area, F major, swaying */
static const char KELP_LEAD[] =
    "@14 v11 q6 o5 | f4 a8 g8 f4 c4 | d4 f8 e8 d2 | c4 e8 g8 b-4 a4 | g2. r4"
    "| f4 a8 g8 f4 c4 | d4 f8 a8 o6 c4 o5 b-4 | a4 g4 e4 g4 | f1";
static const char KELP_ARP[] =
    "v6 q5 o3 @16 " E8("f") "@17 " E8("d") "@16 " E8("c") "@16 " E8("c") "@16 " E8("f") "@16 " E8("b-")
    "@16 " E8("c") "@16 " E8("f");
static const char KELP_BASS[] =
    "@6 v13 q5 " E8("o2 f") E8("o2 d") E8("o2 c") E8("o2 c") E8("o2 f") E8("o2 b-") E8("o2 c") E8("o2 f");
static const char KELP_DRUM[] = "[" SOFT "]8";

/* "Drowned Arcade" - the ruins, D dorian */
static const char ARCADE_LEAD[] =
    "@5 v10 q7 o4 | a4. g8 f4 e4 | d4 f4 a2 | g4. f8 e4 c4 | d2 e2 | a4. g8 f4 a4 | o5 c4 o4 b4 a2"
    "| g4 f4 e4 c4 | d1";
static const char ARCADE_DRIP[] =
    "@15 v4 q4 o6 d8 r4. r2 f8 r4. r2 e8 r4. r2 c8 r4. r2 d8 r4. r2 a8 r4. r2 g8 r4. r2 d8 r4. r2";
static const char ARCADE_BASS[] = "@6 v12 q7 o2 d2 o1 a2 o2 d2 f2 c2 o1 g2 o2 c2 e2 d2 o1 a2 o2 f2 a2 c2 e2 d2 d2";
static const char ARCADE_DRUM[] = "[@13 v9 o2 c4 r4 @11 v5 o6 c4 r4]8";

/* "Blackwater" - the dark, A minor, almost nothing */
static const char BLACK_LEAD[] = "@19 v9 q8 o4 | a1 | g2 e2 | f1 | e2 r2 | a1 | o5 c2 o4 b2 | g+1 | a1";
static const char BLACK_PING[] = "@15 v3 q3 o6 [e8 r8 r4 r2 r1]4";
static const char BLACK_BASS[] = "@6 v10 q8 o1 a1 a1 f1 e1 a1 a1 e1 a1";
static const char BLACK_HUSH[] = "[@10 v2 o4 c1]8";

/* "Dry Dock" - the docks, G major */
static const char DOCK_LEAD[] =
    "@15 v9 q6 o5 | d4 b4 a4 g4 | f+2 a2 | g4 e4 d4 b4 | a2. r4 | d4 b4 a4 g4 | f+4 a4 o6 d2"
    "| o5 c4 b4 a4 f+4 | g1";
static const char DOCK_PAD[] = "@4 v5 q8 o4 b1 a1 g1 f+1 b1 a1 a1 b1";
static const char DOCK_BASS[] = "@6 v11 q8 o2 g1 d1 e1 d1 g1 d1 d1 g1";
static const char DOCK_SOFT[] = "[@9 v2 o8 c2]16";

/* "Big Fish" - the big creatures, E minor */
static const char BOSS_LEAD[] =
    "@1 v12 q6 o5 | e8 e8 g8 e8 b4 a4 | g8 f+8 e8 d8 e2 | e8 e8 g8 e8 o6 c4 o5 b4 | a8 g8 f+8 g8 a2"
    "| e8 e8 g8 e8 b4 a4 | o6 d8 c8 o5 b8 a8 g2 | f+8 g8 a8 g8 f+8 e8 d+8 f+8 | e1";
static const char BOSS_ARP[] =
    "v6 q4 o3 @17 " E8("e") "@16 " E8("c") "@17 " E8("e") "@17 " E8("a") "@17 " E8("e") "@16 " E8("d")
    "@16 " E8("b") "@17 " E8("e");
static const char BOSS_BASS[] =
    "@6 v13 q5 " E8("o2 e") E8("o2 c") E8("o2 e") E8("o2 a") E8("o2 e") E8("o2 d") E8("o2 b") E8("o2 e");
static const char BOSS_DRUM[] = "[" BUSY BUSY "]8";

/* "Xolloth" - the last fight, F minor */
static const char FINAL_LEAD[] =
    "@14 v12 q6 o4 | f4 a-4 o5 c4 f4 | e4 c4 o4 b-4 g4 | a-4 o5 c4 f4 a-4 | g2 e2 | f4 e-4 d-4 c4"
    "| o4 b-4 a-4 g4 e4 | f4 g4 a-4 b-4 | o5 c2 o4 f2";
static const char FINAL_ARP[] =
    "@2 v5 q4 " ARP("o4 f", "a-", "o5 c", "o4 a-") ARP("o4 e", "g", "b-", "g") ARP("o4 f", "a-", "o5 c", "o4 a-")
    ARP("o4 e", "g", "b-", "g") ARP("o4 d-", "f", "a-", "f") ARP("o4 c", "e", "g", "e") ARP("o4 d-", "f", "a-", "f")
    ARP("o4 f", "a-", "o5 c", "o4 a-");
static const char FINAL_BASS[] =
    "@6 v13 q5 " E8("o2 f") E8("o2 c") E8("o2 f") E8("o2 c") E8("o2 d-") E8("o2 c") E8("o2 d-") E8("o2 f");
static const char FINAL_DRUM[] = "[" BUSY BUSY "]8";

/* "Slack Tide" - the ending, C major */
static const char END_LEAD[] =
    "@14 v11 q7 o4 | g4 o5 c4 e4 g4 | f4 e4 d2 | c4 e4 d4 o4 b4 | a2. r4 | g4 o5 c4 e4 g4 | a4 g4 o6 c2"
    "| o5 b4 a4 g4 d4 | c1";
static const char END_PAD[] = "@22 v6 q8 o4 e1 f1 e1 f1 e1 f1 d1 e1";
static const char END_BASS[] = "@6 v12 q7 o2 c2 g2 f2 c2 c2 g2 f2 a2 c2 g2 f2 a2 g2 d2 c2 c2";
static const char END_DRUM[] = "[" SOFT "]8";

/* jingles */
static const char ITEM_P1[] = "@39 v11 o5 l16 e g o6 c e8 d8 c4";
static const char ITEM_P2[] = "@4 v6 o4 l16 c e g o5 c8 o4 b8 o5 c4";
static const char ITEM_TRI[] = "@6 v12 o3 c8 r8 g8 f8 c4";
static const char TOWED_P1[] = "@5 v11 o5 c4 o4 b4 a4 g+4 a1";
static const char TOWED_TRI[] = "@6 v12 o2 a2 e2 a1";

void kpr_audio_load(void) {
    if (KPR_MUS_TITLE >= 0) return;
    KPR_MUS_TITLE = song_define("kpr_title", 120, true, TITLE_LEAD, TITLE_ARP, TITLE_BASS, TITLE_DRUM);
    KPR_MUS_INTRO = song_define("kpr_intro", 80, true, INTRO_LEAD, INTRO_PAD, INTRO_BASS, INTRO_SOFT);
    KPR_MUS_KELP = song_define("kpr_kelp", 112, true, KELP_LEAD, KELP_ARP, KELP_BASS, KELP_DRUM);
    KPR_MUS_ARCADE = song_define("kpr_arcade", 100, true, ARCADE_LEAD, ARCADE_DRIP, ARCADE_BASS, ARCADE_DRUM);
    KPR_MUS_BLACK = song_define("kpr_black", 72, true, BLACK_LEAD, BLACK_PING, BLACK_BASS, BLACK_HUSH);
    KPR_MUS_DOCK = song_define("kpr_dock", 96, true, DOCK_LEAD, DOCK_PAD, DOCK_BASS, DOCK_SOFT);
    KPR_MUS_BOSS = song_define("kpr_boss", 156, true, BOSS_LEAD, BOSS_ARP, BOSS_BASS, BOSS_DRUM);
    KPR_MUS_FINAL = song_define("kpr_final", 140, true, FINAL_LEAD, FINAL_ARP, FINAL_BASS, FINAL_DRUM);
    KPR_MUS_ENDING = song_define("kpr_ending", 88, true, END_LEAD, END_PAD, END_BASS, END_DRUM);
    KPR_MUS_ITEM = song_define("kpr_item", 140, false, ITEM_P1, ITEM_P2, ITEM_TRI, "");
    KPR_MUS_TOWED = song_define("kpr_towed", 80, false, TOWED_P1, "", TOWED_TRI, "");

    sfx_define("kpr_torp", CH_P2, 240, "@20 v6 o5 c32 o4 g32");
    sfx_define("kpr_buster", CH_P2, 220, "@37 v9 o4 c32 o3 g32");
    sfx_define("kpr_dash", CH_NOISE, 220, "@36 v9 o5 c16");
    sfx_define("kpr_hurt", CH_P1, 200, "@37 v12 o4 c16 o3 g16");
    sfx_define("kpr_clank", CH_NOISE, 220, "@40 v10 o6 c16");
    sfx_define("kpr_knock", CH_P2, 220, "@33 v9 o5 c16");
    sfx_define("kpr_steal", CH_P2, 220, "@39 v9 o6 c32 o5 g32 e32");
    sfx_define("kpr_hit", CH_NOISE, 240, "@21 v8 o6 c32");
    sfx_define("kpr_tink", CH_P2, 240, "@42 v8 o7 c32");
    sfx_define("kpr_crack", CH_NOISE, 200, "@34 v8 o4 c16");
    sfx_define("kpr_switch", CH_P1, 200, "@39 v11 o5 l32 c g o6 c e");
    sfx_define("kpr_boom", CH_NOISE, 140, "@34 v12 o3 c4");
    sfx_define("kpr_bigboom", CH_NOISE, 90, "@34 v15 o2 c2");
    sfx_define("kpr_drop", CH_P2, 200, "@33 v9 o4 c16 o3 g16");
    sfx_define("kpr_pick", CH_P1, 220, "@39 v10 o6 l32 c e g");
    sfx_define("kpr_fuel", CH_P1, 200, "@35 v10 o6 c16 g16");
    sfx_define("kpr_drill", CH_NOISE, 240, "@21 v9 o5 c32 c32");
    sfx_define("kpr_bump", CH_NOISE, 200, "@13 v8 o3 c16");
    sfx_define("kpr_ready", CH_P2, 220, "@15 v8 o6 c32 e32");
    sfx_define("kpr_missile", CH_NOISE, 200, "@36 v11 o4 c8");
    sfx_define("kpr_spit", CH_P2, 220, "@37 v6 o4 g16");
    sfx_define("kpr_calm", CH_P1, 200, "@38 v9 o5 c16 g16");
    sfx_define("kpr_snap", CH_P2, 220, "@37 v9 o5 c32 o4 c32");
    sfx_define("kpr_whoosh", CH_NOISE, 200, "@36 v8 o6 c16");
    sfx_define("kpr_puff", CH_NOISE, 180, "@10 v10 o5 c8");
    sfx_define("kpr_bosshit", CH_NOISE, 240, "@21 v10 o5 c32");
    sfx_define("kpr_rumble", CH_NOISE, 120, "@34 v10 o2 c4");
    sfx_define("kpr_spines", CH_P2, 200, "@39 v8 o6 e16 g16");
    sfx_define("kpr_laser", CH_P1, 160, "@38 v11 o6 c4");
    sfx_define("kpr_spin", CH_NOISE, 160, "@36 v10 o4 c8 c8");
    sfx_define("kpr_light", CH_P1, 180, "@39 v12 o5 l32 c e g o6 c e g o7 c");
    sfx_define("kpr_dock", CH_P1, 180, "@15 v11 o5 c16 e16 g16 o6 c8");
    sfx_define("kpr_dry", CH_P1, 140, "@33 v12 o5 c8 o4 a8 f8 d4");
    sfx_define("kpr_fit", CH_P2, 240, "@42 v10 o5 c32 r32 g32");
    sfx_define("kpr_unfit", CH_P2, 240, "@42 v9 o5 g32 r32 c32");
    sfx_define("kpr_launch", CH_NOISE, 160, "@10 v10 o5 c8");
    sfx_define("kpr_roe", CH_P1, 200, "@15 v10 o5 e16 g16 o6 c8");
}
