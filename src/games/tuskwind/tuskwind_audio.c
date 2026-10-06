/* TUSKWIND - original music (UFO-MML) and sound effects, all written for
 * UFO 40. Four tunes, as the original has four (the dream, the hall, the
 * ending and the brawl), and two short jingles. Every looping song's
 * channels are a whole number of bars. */
#include "tuskwind.h"

int TKW_MUS_DREAM = -1, TKW_MUS_HALL, TKW_MUS_ENDING, TKW_MUS_BRAWL, TKW_MUS_WAKE, TKW_MUS_ROUND;

/* a bar of eighth-note arpeggio on four notes */
#define ARP8(a, b, c, d) "[" a "8 " b "8 " c "8 " d "8]2 "
/* a bar of eighths on one bass note */
#define EI(n) "[" n "8]8 "
/* a bar of off-beat chord stabs */
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
/* half a bar of kick, hat, snare, hat */
#define DR "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
/* a bar of soft hats */
#define HUSH "@9 v3 o8 c4 @9 v2 o8 c8 c8 @9 v3 o8 c4 @9 v2 o8 c8 c8 "

/* "Driftsleep" - the dream, D major, a slow lullaby across the islets */
static const char DREAM_LEAD[] =
    "@5 v11 q7 o5"
    "| f+4. e8 d4 a4 | b2 a4 f+4 | g4. f+8 e4 d4 | e2. r4"
    "| f+4. g8 a4 >d4< | c+4 a4 f+2 | g4 b4 >d4 c+8< b8 | a2. r4"
    "| b4. a8 f+4 d4 | g4 a4 b4 >d4< | a4. g8 f+4 e4 | f+2 e4 r4"
    "| g4 f+4 e4 d4 | e4 f+4 g4 c+4 | d2. r4 | r1";
static const char DREAM_ARP[] =
    "@2 v5 q5 " ARP8("o4 d", "f+", "a", "f+") ARP8("o4 b", "o5 d", "f+", "d") ARP8("o4 g", "b", "o5 d", "o4 b")
    ARP8("o4 a", "o5 c+", "e", "c+") ARP8("o4 d", "f+", "a", "f+") ARP8("o4 f+", "a", "o5 c+", "o4 a")
    ARP8("o4 g", "b", "o5 d", "o4 b") ARP8("o4 a", "o5 c+", "e", "c+") ARP8("o4 b", "o5 d", "f+", "d")
    ARP8("o4 g", "b", "o5 d", "o4 b") ARP8("o4 d", "f+", "a", "f+") ARP8("o4 a", "o5 c+", "e", "c+")
    ARP8("o4 g", "b", "o5 d", "o4 b") ARP8("o4 a", "o5 c+", "e", "c+") ARP8("o4 d", "f+", "a", "f+")
    ARP8("o4 d", "a", "o5 d", "o4 a");
static const char DREAM_BASS[] =
    "@6 v13 q6"
    "| o2 d2 a2 | o1 b2 o2 f+2 | o1 g2 o2 d2 | o1 a2 o2 e2"
    "| o2 d2 a2 | o2 f+2 c+2 | o1 g2 o2 d2 | o1 a2 o2 e2"
    "| o1 b2 o2 f+2 | o1 g2 o2 d2 | o2 d2 a2 | o1 a2 o2 e2"
    "| o1 g2 o2 d2 | o1 a2 o2 c+2 | o2 d2 a2 | o2 d1";
static const char DREAM_HUSH[] = "[" HUSH "]16";

/* "The Deep Hall" - at the dream's end, A minor, still and echoing */
static const char HALL_LEAD[] =
    "@4 v10 q8 o4"
    "| a2 >c2< | b2 g+2 | a2 e2 | f2. e4 | d2 f2 | e2 a2 | g+2 b2 | a1";
static const char HALL_PAD[] =
    "v6 q8 | @17 o3 a1 | @16 o3 e1 | @17 o3 a1 | @16 o3 f1 | @17 o3 d1 | @17 o3 a1 | @16 o3 e1 | @17 o3 a1";
static const char HALL_BASS[] = "@6 v12 q8 o2 a1 e1 a1 f1 d1 a1 e1 a1";
static const char HALL_DRIP[] = "[r2 @21 v3 o7 c8 r8 r4]8";

/* "Waking Tide" - the endings, F major */
static const char END_LEAD[] =
    "@14 v11 q7 o5"
    "| a4 g4 f4 c4 | d2 c2 | b-4 a4 g4 f4 | g2. r4"
    "| a4 >c4 f4 e4 | d2 c4< a4 | g4 a4 b-4 g4 | f1";
static const char END_ARP[] =
    "@2 v5 q5 " ARP8("o4 f", "a", "o5 c", "o4 a") ARP8("o4 d", "f", "a", "f") ARP8("o3 b-", "o4 d", "f", "d")
    ARP8("o4 c", "e", "g", "e") ARP8("o4 f", "a", "o5 c", "o4 a") ARP8("o4 d", "f", "a", "f")
    ARP8("o4 c", "e", "g", "e") ARP8("o4 f", "a", "o5 c", "o4 a");
static const char END_BASS[] = "@6 v12 q7 o2 f2 c2 d2 o1 a2 b-2 o2 f2 c2 o1 g2 o2 f2 c2 d2 o1 a2 o2 c2 o1 g2 o2 f1";
static const char END_HUSH[] = "[" HUSH "]8";

/* "Floe Brawl" - the brawl, E minor, quick */
static const char BRAWL_LEAD[] =
    "@1 v11 q6 o5 ["
    "| e8 g8 b8 e8 g8 b8 >e4< | d8 f+8 a8 d8 f+8 a8 >d4< | c8 e8 g8 c8 e8 g8 >c8< b8 | a8 g8 f+8 e8 d+4 o4 b4 o5"
    "| e8 e8 g8 e8 b8 a8 g8 f+8 | g8 a8 b8 >d8 e4< b4 | >c8< b8 a8 g8 f+8 g8 a8 f+8 | e2 r4 o4 b4 o5 ]2";
static const char BRAWL_STAB[] =
    "v6 q2 [" OFF("@17 o4", "e") OFF("@16 o4", "d") OFF("@16 o4", "c") OFF("@16 o3", "b")
    OFF("@17 o4", "e") OFF("@16 o4", "g") OFF("@17 o4", "a") OFF("@17 o4", "e") "]2";
static const char BRAWL_BASS[] =
    "@6 v14 q4 [" EI("o2 e") EI("o2 d") EI("o2 c") EI("o1 b") EI("o2 e") EI("o2 g") EI("o2 a") EI("o2 e") "]2";
static const char BRAWL_DRUM[] = "[" DR DR "]16";

/* jingles */
static const char WAKE_P1[] = "@5 v11 o5 a4 f+4 d4 o4 a4 b2 a2";
static const char WAKE_TRI[] = "@6 v12 o2 d2 o1 a2 g2 a2";
static const char ROUND_P1[] = "@1 v12 o5 l16 e g b >e8< b8 >e4 r8 d+8 e2";
static const char ROUND_TRI[] = "@6 v12 o2 e8 b8 e8 g8 b4 r8 b8 e2";
static const char ROUND_NOISE[] = "@13 v12 o2 c8 r8 @11 v10 o6 c8 r8 @12 v10 o5 c2";

void tkw_audio_load(void) {
    if (TKW_MUS_DREAM >= 0) return;
    TKW_MUS_DREAM = song_define("tkw_dream", 92, true, DREAM_LEAD, DREAM_ARP, DREAM_BASS, DREAM_HUSH);
    TKW_MUS_HALL = song_define("tkw_hall", 66, true, HALL_LEAD, HALL_PAD, HALL_BASS, HALL_DRIP);
    TKW_MUS_ENDING = song_define("tkw_ending", 84, true, END_LEAD, END_ARP, END_BASS, END_HUSH);
    TKW_MUS_BRAWL = song_define("tkw_brawl", 150, true, BRAWL_LEAD, BRAWL_STAB, BRAWL_BASS, BRAWL_DRUM);
    TKW_MUS_WAKE = song_define("tkw_wake", 90, false, WAKE_P1, "", WAKE_TRI, "");
    TKW_MUS_ROUND = song_define("tkw_round", 150, false, ROUND_P1, "", ROUND_TRI, ROUND_NOISE);

    sfx_define("tkw_start", CH_P1, 200, "@32 v12 o4 d8 a8 o5 d4");
    sfx_define("tkw_charge0", CH_P2, 240, "@42 v6 o5 c32");
    sfx_define("tkw_jump", CH_P1, 220, "@32 v10 o4 g16 o5 c16");
    sfx_define("tkw_lunge", CH_NOISE, 200, "@36 v7 o6 c8");
    sfx_define("tkw_flap", CH_NOISE, 240, "@36 v4 o7 c32");
    sfx_define("tkw_land", CH_NOISE, 220, "@13 v8 o2 c16");
    sfx_define("tkw_scoot", CH_NOISE, 240, "@9 v3 o6 c32");
    sfx_define("tkw_bump", CH_P2, 200, "@37 v10 o4 c16 o3 g16");
    sfx_define("tkw_splash", CH_NOISE, 160, "@10 v11 o5 c4");
    sfx_define("tkw_shell", CH_P2, 240, "@35 v9 o6 e32 b32");
    sfx_define("tkw_whelk", CH_P2, 220, "@39 v10 o5 l32 c e g o6 c");
    sfx_define("tkw_key", CH_P2, 220, "@15 v10 o6 c16 g16");
    sfx_define("tkw_sprat", CH_P2, 220, "@38 v10 o4 c16 g16");
    sfx_define("tkw_alert", CH_P2, 200, "@20 v10 o6 c16 r32 c16");
    sfx_define("tkw_charge", CH_NOISE, 160, "@36 v9 o4 c4");
    sfx_define("tkw_crumble", CH_NOISE, 180, "@21 v7 o5 c8");
    sfx_define("tkw_sign", CH_P2, 200, "@15 v8 o5 a16 o6 e16");
    sfx_define("tkw_bell", CH_P1, 150, "@15 v12 o6 c4 o5 g4");
    sfx_define("tkw_wind", CH_NOISE, 100, "@36 v8 o4 c2");
    sfx_define("tkw_tern", CH_P2, 220, "@39 v10 o6 l32 e g e g");
    sfx_define("tkw_shopbell", CH_P2, 200, "@15 v10 o6 e16 c16");
    sfx_define("tkw_buy", CH_P1, 220, "@35 v11 o6 c16 e16 g8");
    sfx_define("tkw_poof", CH_NOISE, 200, "@36 v8 o5 c8");
    sfx_define("tkw_chest", CH_P1, 200, "@39 v12 o5 l16 c e g o6 c e4");
    sfx_define("tkw_door", CH_P1, 160, "@33 v10 o4 g8 e8 c4");
    sfx_define("tkw_nope", CH_P2, 180, "@37 v10 o3 c16 r32 c16");
    sfx_define("tkw_rope", CH_NOISE, 220, "@36 v8 o7 c8");
    sfx_define("tkw_hook", CH_P2, 220, "@21 v9 o6 c16");
    sfx_define("tkw_item", CH_P1, 220, "@39 v10 o5 l32 g o6 c e g");
    sfx_define("tkw_throw", CH_P2, 220, "@32 v8 o5 c16");
    sfx_define("tkw_ballbump", CH_P2, 240, "@20 v6 o5 g32");
    sfx_define("tkw_pop", CH_NOISE, 200, "@34 v10 o5 c8");
}
