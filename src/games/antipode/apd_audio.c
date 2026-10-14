/* ANTIPODE - original music (UFO-MML) and sound effects. Every looping tune
 * is eight bars a channel, so the channels loop together. A tune for each
 * sector, one for the cradle, one for the fights and one for the last. */
#include "apd.h"

int APD_MUS_TITLE = -1, APD_MUS_INTRO, APD_MUS_GANTRY, APD_MUS_BRINE, APD_MUS_COIL, APD_MUS_CROWN, APD_MUS_POD,
    APD_MUS_BOSS, APD_MUS_FINAL, APD_MUS_ENDING, APD_MUS_CREDITS, APD_MUS_ITEM, APD_MUS_OVER;

/* a bar of four quarters; a bar of eighths on one note; a bar of sixteenth arpeggios */
#define Q4(a, b, c, d) a "4 " b "4 " c "4 " d "4 "
#define E8(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
/* half a bar of rock drums; half a bar of busy drums */
#define ROCK "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
#define BUSY "@13 v12 o2 c8 @9 v5 o8 c16 c16 @11 v10 o6 c8 @9 v5 o8 c16 c16 "
#define SOFT "@13 v9 o2 c4 @9 v3 o8 c4 @11 v6 o6 c4 @9 v3 o8 c4 "

/* "Antipode" - the title, A minor */
static const char TITLE_LEAD[] =
    "@1 v12 q7 | o4 a4 o5 c4 e4 a4 | g4. f8 e4 d4 | e4 c4 o4 a4 b4 | o5 c2 o4 b2"
    "| o4 a4 o5 c4 e4 a4 | b4. a8 g4 e4 | f4 e4 d4 o4 b4 | o4 a1";
static const char TITLE_ARP[] =
    "v6 q5 o3 @17 " E8("a") "@16 " E8("f") "@17 " E8("a") "@16 " E8("g") "@17 " E8("a") "@16 " E8("e") "@17 " E8("d")
    "@17 " E8("a");
static const char TITLE_BASS[] =
    "@6 v13 q6 " Q4("o2 a", "a", "o3 a", "o2 a") Q4("o2 f", "f", "o3 f", "o2 f") Q4("o2 a", "a", "o3 a", "o2 a")
    Q4("o2 g", "g", "o3 g", "o2 g") Q4("o2 a", "a", "o3 a", "o2 a") Q4("o2 e", "e", "o3 e", "o2 e")
    Q4("o2 d", "d", "o3 d", "o2 d") Q4("o2 a", "a", "o3 a", "o2 a");
static const char TITLE_DRUM[] = "[" ROCK ROCK "]8";

/* "Wake in the Cradle" - the story, slow */
static const char INTRO_LEAD[] =
    "@5 v10 q7 | o4 e2 a2 | g2. e4 | f2 e4 d4 | e1 | e2 a2 | o5 c2. o4 b4 | a4 g4 f4 d4 | e1";
static const char INTRO_PAD[] = "@4 v5 q8 o3 a1 g1 f1 e1 a1 a1 f1 e1";
static const char INTRO_BASS[] = "@6 v11 q8 o2 a1 e1 d1 e1 a1 a1 d1 e1";
static const char INTRO_SOFT[] = "[@9 v2 o8 c2]16";

/* "Gantry Shift" - the Gantry, E minor, driving */
static const char GANTRY_LEAD[] =
    "@14 v11 q6 | o5 e8 e8 b4 a8 g8 f+4 | g8 a8 b4 e4 d4 | c4 b8 a8 b4 g4 | f+2 r4 o4 b4"
    "| o5 e8 e8 b4 a8 g8 f+4 | g8 a8 b4 o6 d4 c4 | o5 b4 a8 g8 f+4 d+4 | e1";
static const char GANTRY_ARP[] =
    "v6 q5 o3 @17 " E8("e") "@16 " E8("c") "@17 " E8("a") "@16 " E8("b") "@17 " E8("e") "@16 " E8("g") "@16 " E8("b")
    "@17 " E8("e");
static const char GANTRY_BASS[] =
    "@6 v13 q5 " E8("o2 e") E8("o2 c") E8("o2 a") E8("o2 b") E8("o2 e") E8("o2 g") E8("o2 b") E8("o2 e");
static const char GANTRY_DRUM[] = "[" ROCK ROCK "]8";

/* "Brine" - the Brine, D dorian, dripping */
static const char BRINE_LEAD[] =
    "@5 v10 q7 | o4 d4. f8 a4 g4 | f4 e4 d2 | o4 c4. e8 g4 f4 | e2 r2 | o4 d4. f8 a4 o5 c4 | o4 b4 a4 g2"
    "| f4 e4 c4 e4 | d1";
static const char BRINE_DRIP[] =
    "@15 v4 q4 o6 a8 r4. r2 | f8 r4. r2 | g8 r4. r2 | e8 r4. r2 | a8 r4. r2 | b8 r4. r2 | g8 r4. r2 | d8 r4. r2";
static const char BRINE_BASS[] =
    "@6 v12 q7 o2 d2 a2 d2 a2 c2 g2 c2 g2 d2 a2 g2 d2 c2 g2 d2 a2";
static const char BRINE_DRUM[] = "[" SOFT "]8";

/* "The Coil" - the Coil, A minor, crackling arpeggios */
static const char COIL_LEAD[] =
    "@1 v11 q6 | o5 a8 r8 a8 g8 a4 o6 c4 | o5 b8 r8 b8 a8 g4 e4 | f8 r8 f8 e8 f4 a4 | g8 r8 g8 f8 e4 d4"
    "| a8 r8 a8 g8 a4 o6 c4 | d8 r8 d8 c8 o5 b4 g4 | a4 g4 f4 e4 | a1";
static const char COIL_ARP[] =
    "@2 v5 q4 " ARP("o4 a", "o5 c", "e", "c") ARP("o4 e", "g", "b", "g") ARP("o4 f", "a", "o5 c", "o4 a")
    ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 a", "o5 c", "e", "c") ARP("o4 d", "f", "a", "f")
    ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 a", "o5 c", "e", "c");
static const char COIL_BASS[] =
    "@6 v13 q5 " E8("o2 a") E8("o2 e") E8("o2 f") E8("o2 g") E8("o2 a") E8("o2 d") E8("o2 f") E8("o2 a");
static const char COIL_DRUM[] = "[" BUSY BUSY "]8";

/* "The Crown" - the last sector, D minor, slow and heavy */
static const char CROWN_LEAD[] =
    "@23 v11 q7 | o4 d2 f2 | e2 a2 | b-2 a4 g4 | a1 | d2 f2 | g2 b-2 | a4 g4 f4 e4 | d1";
static const char CROWN_PAD[] = "@4 v5 q8 o4 f1 e1 d1 c+1 f1 g1 e1 f1";
static const char CROWN_BASS[] = "@6 v12 q8 o2 d1 a1 g1 a1 d1 g1 a1 d1";
static const char CROWN_DRUM[] = "[@13 v12 o2 c8 c8 r4 r2]8";

/* "Cradle" - the cradle's screens, F major */
static const char POD_LEAD[] =
    "@15 v9 q6 | o5 c4 a4 g4 f4 | e2 g2 | f4 d4 c4 o4 a4 | b-2. r4 | o5 c4 a4 g4 f4 | e4 g4 o6 c2"
    "| o5 b-4 a4 g4 e4 | f1";
static const char POD_PAD[] = "@4 v5 q8 o4 a1 g1 f1 f1 a1 g1 e1 a1";
static const char POD_BASS[] = "@6 v11 q8 o2 f1 c1 d1 o1 b-1 o2 f1 c1 c1 f1";
static const char POD_SOFT[] = "[@9 v2 o8 c2]16";

/* "Hostile Unit" - the fights, E phrygian */
static const char BOSS_LEAD[] =
    "@1 v12 q6 | o5 e8 f8 e8 d8 e4 b4 | a8 g8 f8 g8 e2 | e8 f8 e8 d8 e4 o6 c4 | o5 b8 a8 g8 a8 b2"
    "| e8 f8 e8 d8 e4 b4 | o6 c8 d8 c8 o5 b8 a2 | g8 a8 b8 a8 g8 f8 e8 d8 | e1";
static const char BOSS_STAB[] =
    "v6 q4 o3 @17 " E8("e") "@16 " E8("f") "@17 " E8("e") "@16 " E8("g") "@17 " E8("e") "@17 " E8("a")
    "@16 " E8("g") "@17 " E8("e");
static const char BOSS_BASS[] =
    "@6 v13 q5 " E8("o2 e") E8("o2 f") E8("o2 e") E8("o2 g") E8("o2 e") E8("o2 a") E8("o2 g") E8("o2 e");
static const char BOSS_DRUM[] = "[" BUSY BUSY "]8";

/* "Halloran" - the last fight, C minor */
static const char FINAL_LEAD[] =
    "@14 v12 q6 | o4 c4 e-4 g4 o5 c4 | o4 b4 g4 f4 d4 | e-4 g4 o5 c4 e-4 | d2 o4 b2"
    "| o4 c4 e-4 g4 o5 c4 | d4 e-4 f4 g4 | a-4 g4 f4 d4 | c1";
static const char FINAL_ARP[] =
    "@2 v5 q4 " ARP("o4 c", "e-", "g", "e-") ARP("o3 b", "o4 d", "g", "d") ARP("o4 c", "e-", "g", "e-")
    ARP("o3 b", "o4 d", "g", "d") ARP("o4 c", "e-", "g", "e-") ARP("o4 b-", "d", "f", "d")
    ARP("o4 a-", "c", "f", "c") ARP("o4 c", "e-", "g", "e-");
static const char FINAL_BASS[] =
    "@6 v13 q5 " E8("o2 c") E8("o2 g") E8("o2 c") E8("o2 g") E8("o2 c") E8("o2 b-") E8("o2 a-") E8("o2 c");
static const char FINAL_DRUM[] = "[" BUSY BUSY "]8";

/* "Visor Up" - the ending, G major */
static const char END_LEAD[] =
    "@14 v11 q7 | o4 g4 b4 o5 d4 g4 | f+4 e4 d2 | c4 e4 d4 o4 b4 | a2. r4 | g4 b4 o5 d4 g4 | a4 b4 o6 c2"
    "| o5 b4 a4 g4 f+4 | g1";
static const char END_PAD[] = "@22 v6 q8 o4 b1 a1 g1 f+1 b1 o5 c1 o4 a1 b1";
static const char END_BASS[] = "@6 v12 q7 o2 g2 d2 d2 a2 c2 g2 d2 a2 g2 d2 c2 e2 d2 f+2 g2 d2";
static const char END_DRUM[] = "[" SOFT "]8";

/* "Tallow Station" - the credits, D major */
static const char CRED_LEAD[] =
    "@1 v11 q7 | o5 d4 f+8 a8 o6 d4 c+4 | o5 b4 a4 f+2 | g4 b8 o6 d8 c+4 o5 a4 | b2 a2"
    "| d4 f+8 a8 o6 d4 e4 | f+4 e4 d4 c+4 | o5 b4 a4 g4 e4 | d1";
static const char CRED_ARP[] =
    "v6 q5 o3 @16 " E8("d") "@17 " E8("b") "@16 " E8("g") "@16 " E8("a") "@16 " E8("d") "@16 " E8("a")
    "@16 " E8("g") "@16 " E8("d");
static const char CRED_BASS[] =
    "@6 v12 q5 " E8("o2 d") E8("o2 b") E8("o2 g") E8("o2 a") E8("o2 d") E8("o2 a") E8("o2 g") E8("o2 d");
static const char CRED_DRUM[] = "[" ROCK ROCK "]8";

/* jingles */
static const char ITEM_P1[] = "@1 v12 o5 l16 c e g o6 c4 o5 g8 o6 c4";
static const char ITEM_P2[] = "@4 v7 o4 l16 e g o5 c e4 c8 e4";
static const char ITEM_TRI[] = "@6 v12 o3 c4 r16 r16 g8 c4";
static const char OVER_P1[] = "@5 v11 o5 e4 d4 c4 o4 b4 a1";
static const char OVER_TRI[] = "@6 v12 o2 a2 e2 a1";

void apd_audio_load(void) {
    if (APD_MUS_TITLE >= 0) return;
    APD_MUS_TITLE = song_define("apd_title", 120, true, TITLE_LEAD, TITLE_ARP, TITLE_BASS, TITLE_DRUM);
    APD_MUS_INTRO = song_define("apd_intro", 80, true, INTRO_LEAD, INTRO_PAD, INTRO_BASS, INTRO_SOFT);
    APD_MUS_GANTRY = song_define("apd_gantry", 132, true, GANTRY_LEAD, GANTRY_ARP, GANTRY_BASS, GANTRY_DRUM);
    APD_MUS_BRINE = song_define("apd_brine", 100, true, BRINE_LEAD, BRINE_DRIP, BRINE_BASS, BRINE_DRUM);
    APD_MUS_COIL = song_define("apd_coil", 140, true, COIL_LEAD, COIL_ARP, COIL_BASS, COIL_DRUM);
    APD_MUS_CROWN = song_define("apd_crown", 110, true, CROWN_LEAD, CROWN_PAD, CROWN_BASS, CROWN_DRUM);
    APD_MUS_POD = song_define("apd_pod", 90, true, POD_LEAD, POD_PAD, POD_BASS, POD_SOFT);
    APD_MUS_BOSS = song_define("apd_boss", 160, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    APD_MUS_FINAL = song_define("apd_final", 150, true, FINAL_LEAD, FINAL_ARP, FINAL_BASS, FINAL_DRUM);
    APD_MUS_ENDING = song_define("apd_ending", 96, true, END_LEAD, END_PAD, END_BASS, END_DRUM);
    APD_MUS_CREDITS = song_define("apd_credits", 112, true, CRED_LEAD, CRED_ARP, CRED_BASS, CRED_DRUM);
    APD_MUS_ITEM = song_define("apd_item", 140, false, ITEM_P1, ITEM_P2, ITEM_TRI, "");
    APD_MUS_OVER = song_define("apd_over", 80, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("apd_shot", CH_P2, 240, "@20 v6 o6 c32 o5 g32");
    sfx_define("apd_hotshot", CH_P2, 240, "@20 v7 o5 g32 d32");
    sfx_define("apd_jump", CH_P1, 240, "@32 v8 o4 c32 g32");
    sfx_define("apd_flip", CH_P1, 240, "@38 v10 o5 c32 o4 c32 o5 c32");
    sfx_define("apd_land", CH_NOISE, 240, "@13 v7 o3 c32");
    sfx_define("apd_stomp", CH_NOISE, 160, "@41 v13 o2 c8");
    sfx_define("apd_hurt", CH_P1, 200, "@37 v12 o4 c16 o3 g16");
    sfx_define("apd_hit", CH_NOISE, 240, "@21 v8 o6 c32");
    sfx_define("apd_tink", CH_P2, 240, "@42 v8 o7 c32");
    sfx_define("apd_pop", CH_NOISE, 200, "@34 v9 o4 c16");
    sfx_define("apd_boom", CH_NOISE, 140, "@34 v12 o3 c4");
    sfx_define("apd_bigboom", CH_NOISE, 90, "@34 v15 o2 c2");
    sfx_define("apd_die", CH_P1, 140, "@33 v13 o5 c8 o4 a8 f8 d4");
    sfx_define("apd_respawn", CH_P1, 180, "@38 v12 o4 c16 g16 o5 c16 g16");
    sfx_define("apd_item", CH_P1, 200, "@39 v12 o6 l32 c e g o7 c");
    sfx_define("apd_save", CH_P1, 180, "@15 v11 o5 c16 e16 g16 o6 c8");
    sfx_define("apd_pod", CH_P2, 200, "@4 v9 o4 c16 e16 g16");
    sfx_define("apd_slot", CH_P2, 240, "@42 v10 o5 c32 r32 g32");
    sfx_define("apd_term", CH_P2, 240, "@20 v8 o6 c32 e32 c32 e32");
    sfx_define("apd_tele", CH_P1, 160, "@38 v12 o4 l32 c e g o5 c e g o6 c");
    sfx_define("apd_door", CH_NOISE, 220, "@36 v8 o5 c16");
    sfx_define("apd_deny", CH_P2, 180, "@37 v10 o3 c16 r32 c16");
    sfx_define("apd_unlock", CH_P1, 200, "@39 v11 o5 l32 c g o6 c e g");
    sfx_define("apd_click", CH_P2, 240, "@42 v10 o5 c32 r32 c32");
    sfx_define("apd_crumble", CH_NOISE, 160, "@34 v10 o3 c8");
    sfx_define("apd_sizzle", CH_NOISE, 240, "@9 v7 o7 c16");
    sfx_define("apd_spit", CH_P2, 220, "@37 v7 o4 g16");
    sfx_define("apd_star", CH_P2, 220, "@39 v7 o6 c16 g16");
    sfx_define("apd_cannon", CH_NOISE, 200, "@34 v9 o4 c16");
    sfx_define("apd_rumble", CH_NOISE, 120, "@34 v10 o2 c4");
    sfx_define("apd_splash", CH_NOISE, 160, "@10 v10 o5 c8");
    sfx_define("apd_zap", CH_NOISE, 200, "@40 v11 o6 c8");
    sfx_define("apd_beam", CH_P2, 220, "@37 v9 o6 c16 o5 a16");
    sfx_define("apd_ring", CH_P2, 200, "@39 v8 o6 e16 g16");
    sfx_define("apd_shield", CH_P2, 200, "@15 v9 o6 c16 o7 c16");
    sfx_define("apd_alarm", CH_P1, 140, "@15 v12 o5 c8 r16 c8 r16 c8");
}
