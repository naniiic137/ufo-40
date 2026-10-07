/* HOOPLA - original music (UFO-MML) and sound effects. Every looping tune
 * is eight bars a channel, so the channels loop together. */
#include "hoop.h"

int HOOP_MUS_TITLE = -1, HOOP_MUS_SELECT, HOOP_MUS_FIGHT1, HOOP_MUS_FIGHT2, HOOP_MUS_FIGHT3, HOOP_MUS_MIRROR,
    HOOP_MUS_DRAFT, HOOP_MUS_WIN, HOOP_MUS_LOSE, HOOP_MUS_ENDING, HOOP_MUS_CREDITS, HOOP_MUS_CHAMP;

/* one bar each: quarter notes, eighths, off-beat chords, sixteenth arpeggios */
#define Q4(a, b, c, d) a "4 " b "4 " c "4 " d "4 "
#define EI(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define OFFB(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
#define ST16(n) "[" n "16 r16]8 "
#define OCT(n) "[o2 " n "8 o3 " n "8]4 "
#define OCT1(n) "[o1 " n "8 o2 " n "8]4 "
/* half a bar of kick, hat, snare, hat */
#define DR2 "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
#define DRQ "@13 v12 o2 c16 @9 v5 o8 c16 @11 v10 o6 c16 @9 v5 o8 c16 "

/* "Ring Night" - the title, A minor */
static const char TITLE_LEAD[] =
    "@1 v11 q6"
    "| o5 a8 a8 o6 c8 o5 a8 e4 g4 | o5 f8 f8 a8 f8 e4 d4 | o5 c8 d8 e8 g8 a4 g8 e8 | o5 d2 e2"
    "| o5 a8 a8 o6 c8 e8 d4 c4 | o5 b8 a8 g8 b8 a4 e4 | o5 f8 g8 a8 b8 o6 c8 d8 e8 c8 | o5 a2 r2";
static const char TITLE_COMP[] =
    "v6 q3 " OFFB("@17 o4", "a") OFFB("@16 o4", "f") OFFB("@16 o4", "c") OFFB("@16 o4", "g")
    OFFB("@17 o4", "a") OFFB("@16 o4", "e") OFFB("@16 o4", "f") OFFB("@17 o4", "a");
static const char TITLE_BASS[] =
    "@6 v13 q6 " Q4("o2 a", "o3 e", "o2 a", "o3 e") Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 c", "g", "c", "g")
    Q4("o2 g", "o3 d", "o2 g", "o3 d") Q4("o2 a", "o3 e", "o2 a", "o3 e") Q4("o2 e", "b", "e", "b")
    Q4("o2 f", "o3 c", "o2 f", "o3 c") Q4("o2 a", "o3 e", "o2 a", "o3 e");
static const char TITLE_DRUM[] = "[" DR2 DR2 "]8";

/* "Pick Your Corner" - the select and the ladder, C major */
static const char SEL_LEAD[] =
    "@14 v11 q7"
    "| o5 e4 g4 c4 g4 | o5 f4 a4 g2 | o5 e8 f8 g8 a8 g4 e4 | o5 d2 g2"
    "| o5 e4 g4 o6 c4 o5 b4 | o5 a4 f4 d4 f4 | o5 g8 a8 g8 f8 e4 d4 | o5 c2 r2";
static const char SEL_COMP[] =
    "v6 q3 " OFFB("@16 o4", "c") OFFB("@16 o4", "f") OFFB("@16 o4", "c") OFFB("@16 o3", "g")
    OFFB("@16 o4", "c") OFFB("@16 o4", "f") OFFB("@16 o3", "g") OFFB("@16 o4", "c");
static const char SEL_BASS[] =
    "@6 v12 q6 " Q4("o2 c", "e", "g", "e") Q4("o2 f", "a", "o3 c", "o2 a") Q4("o2 c", "e", "g", "e")
    Q4("o2 g", "b", "o3 d", "o2 b") Q4("o2 c", "e", "g", "e") Q4("o2 f", "a", "o3 c", "o2 a")
    Q4("o2 g", "b", "o3 d", "o2 b") Q4("o2 c", "e", "g", "e");
static const char SEL_DRUM[] = "[@9 v4 o8 c8 @9 v3 o8 c8 @11 v6 o6 c8 @9 v3 o8 c8]16";

/* "Glass Pit Brawl" - a match, E minor */
static const char F1_LEAD[] =
    "@0 v11 q5"
    "| o5 b8 r8 b8 a8 g8 e8 g8 a8 | o5 b8 r8 b8 o6 d8 e4 d4 | o6 c8 o5 b8 a8 g8 a8 b8 a8 g8 | o5 f+4 d4 e2"
    "| o5 b8 r8 b8 a8 g8 e8 g8 b8 | o6 e8 d8 c8 o5 b8 a4 g4 | o5 a8 g8 f+8 e8 f+8 g8 a8 f+8 | o5 e2 r2";
static const char F1_ARP[] =
    "@2 v5 q5 " ARP("o4 e", "g", "b", "g") ARP("o4 e", "g", "b", "g") ARP("o4 c", "e", "g", "e")
    ARP("o4 d", "f+", "a", "f+") ARP("o4 e", "g", "b", "g") ARP("o4 c", "e", "g", "e")
    ARP("o4 d", "f+", "a", "f+") ARP("o4 e", "g", "b", "g");
static const char F1_BASS[] =
    "@6 v14 q4 " EI("o2 e") EI("o2 e") EI("o2 c") EI("o2 d") EI("o2 e") EI("o2 c") EI("o2 d") EI("o2 e");
static const char F1_DRUM[] = "[" DR2 DR2 "]8";

/* "Hoops in the Air" - a match, G major */
static const char F2_LEAD[] =
    "@1 v11 q6"
    "| o5 g8 b8 o6 d8 o5 b8 o6 c8 o5 a8 f+8 d8 | o5 g8 b8 o6 d8 g8 f+4 d4 | o6 e8 d8 c8 o5 b8 a8 b8 o6 c8 o5 a8 | o5 b4 a4 g2"
    "| o5 g8 b8 o6 d8 o5 b8 o6 e8 d8 c8 o5 b8 | o5 a8 b8 o6 c8 d8 e4 c4 | o6 d8 c8 o5 b8 a8 g8 f+8 e8 f+8 | o5 g2 r2";
static const char F2_STAB[] =
    "v6 q2 @16 o4 " ST16("g") "@16 o4 " ST16("d") "@16 o4 " ST16("c") "@16 o4 " ST16("d")
    "@16 o4 " ST16("g") "@17 o4 " ST16("a") "@16 o4 " ST16("d") "@16 o4 " ST16("g");
static const char F2_BASS[] =
    "@6 v14 q4 " OCT("g") OCT("d") OCT("c") OCT("d") OCT("g") OCT("a") OCT("d") OCT("g");
static const char F2_DRUM[] = "[@13 v11 o2 c8 @9 v4 o8 c16 c16 @11 v9 o6 c8 @9 v4 o8 c16 c16]16";

/* "Ledge to Ledge" - a match, D minor */
static const char F3_LEAD[] =
    "@0 v11 q5"
    "| o5 d8 f8 a8 o6 d8 c8 o5 a8 f8 a8 | o5 g8 b-8 o6 d8 o5 b-8 a4 f4 | o5 e8 g8 b-8 o6 c+8 d8 c+8 o5 b-8 g8 | o5 a2 a4 r4"
    "| o5 d8 f8 a8 o6 d8 f8 e8 d8 c8 | o5 b-8 a8 g8 f8 g4 a4 | o5 b-8 a8 g8 e8 f8 e8 d8 c+8 | o5 d2 r2";
static const char F3_ARP[] =
    "@2 v5 q5 " ARP("o4 d", "f", "a", "f") ARP("o4 g", "b-", "o5 d", "o4 b-") ARP("o4 e", "g", "b-", "g")
    ARP("o4 c+", "e", "a", "e") ARP("o4 d", "f", "a", "f") ARP("o4 b-", "o5 d", "f", "d")
    ARP("o4 c+", "e", "a", "e") ARP("o4 d", "f", "a", "f");
static const char F3_BASS[] =
    "@6 v14 q4 " OCT("d") OCT("g") OCT1("a") OCT1("a") OCT("d") OCT1("b-") OCT1("a") OCT("d");
static const char F3_DRUM[] = "[" DRQ DRQ DRQ DRQ "]8";

/* "The Mirror Match" - the last match of a tournament, B minor */
static const char MIR_LEAD[] =
    "@1 v12 q6"
    "| o5 b8 f+8 b8 o6 d8 c+8 o5 b8 a+8 f+8 | o5 g8 b8 o6 d8 e8 d4 c+4 | o5 b8 f+8 b8 o6 d8 f+8 e8 d8 c+8 | o5 b4 a+4 b2"
    "| o6 d8 c+8 o5 b8 a8 g8 f+8 e8 d8 | o5 e8 f+8 g8 a8 b4 g4 | o5 a+8 b8 o6 c+8 d8 e8 d8 c+8 o5 a+8 | o5 b2 r2";
static const char MIR_ARP[] =
    "@2 v5 q5 " ARP("o4 b", "o5 d", "f+", "d") ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 b", "o5 d", "f+", "d")
    ARP("o4 f+", "a+", "o5 c+", "o4 a+") ARP("o4 b", "o5 d", "f+", "d") ARP("o4 e", "g", "b", "g")
    ARP("o4 f+", "a+", "o5 c+", "o4 a+") ARP("o4 b", "o5 d", "f+", "d");
static const char MIR_BASS[] =
    "@6 v15 q4 " EI("o2 b") EI("o2 g") EI("o2 b") EI("o2 f+") EI("o2 b") EI("o2 e") EI("o2 f+") EI("o2 b");
static const char MIR_DRUM[] = "[@13 v12 o2 c16 @9 v5 o8 c16 @11 v10 o6 c16 @9 v5 o8 c16]32";

/* "Snake Draft" - the draft and the series, F major */
static const char DR_LEAD[] =
    "@14 v11 q7"
    "| o5 f4 a8 g8 f4 c4 | o5 d4 f8 e8 d2 | o5 b-4 a8 g8 a4 f4 | o5 g2 c2"
    "| o5 f4 a8 o6 c8 d4 c4 | o5 b-4 g8 a8 b-4 o6 d4 | o6 c8 o5 b-8 a8 g8 f4 e4 | o5 f2 r2";
static const char DR_COMP[] =
    "v6 q3 " OFFB("@16 o4", "f") OFFB("@17 o4", "d") OFFB("@16 o3", "b-") OFFB("@16 o4", "c")
    OFFB("@16 o4", "f") OFFB("@16 o3", "b-") OFFB("@16 o4", "c") OFFB("@16 o4", "f");
static const char DR_BASS[] =
    "@6 v12 q6 " Q4("o2 f", "a", "o3 c", "o2 a") Q4("o2 d", "f", "a", "f") Q4("o1 b-", "o2 d", "f", "d")
    Q4("o2 c", "e", "g", "e") Q4("o2 f", "a", "o3 c", "o2 a") Q4("o1 b-", "o2 d", "f", "d")
    Q4("o2 c", "e", "g", "e") Q4("o2 f", "a", "o3 c", "o2 a");
static const char DR_DRUM[] = "[@13 v10 o2 c8 @9 v4 o8 c8 @11 v8 o6 c8 @9 v4 o8 c8]16";

/* "Purse in Hand" - the ending, C major */
static const char END_LEAD[] =
    "@14 v11 q7"
    "| o5 g4 e8 g8 o6 c2 | o5 b4 a8 g8 a2 | o5 f4 a8 o6 c8 e4 d4 | o6 c2 o5 g2"
    "| o5 a4 o6 c8 e8 g4 e4 | o6 f4 e8 d8 c4 o5 a4 | o5 g8 a8 b8 o6 c8 d4 o5 b4 | o6 c2 r2";
static const char END_COMP[] =
    "v6 q3 " OFFB("@16 o4", "c") OFFB("@16 o3", "g") OFFB("@16 o4", "f") OFFB("@16 o4", "c")
    OFFB("@17 o4", "a") OFFB("@16 o4", "f") OFFB("@16 o3", "g") OFFB("@16 o4", "c");
static const char END_BASS[] =
    "@6 v12 q6 " Q4("o2 c", "g", "e", "g") Q4("o2 g", "d", "b", "d") Q4("o2 f", "c", "a", "c") Q4("o2 c", "g", "e", "g")
    Q4("o2 a", "e", "c", "e") Q4("o2 f", "c", "a", "c") Q4("o2 g", "d", "b", "d") Q4("o2 c", "g", "e", "g");
static const char END_DRUM[] = "[@13 v10 o2 c4 @11 v8 o6 c4]16";

/* "Under the Lamps" - the credits, G major */
static const char CRED_LEAD[] =
    "@5 v11 q7"
    "| o5 d4 g4 b4 a8 g8 | o5 e4 g4 a2 | o5 c4 e4 g4 f+8 e8 | o5 d2 d2"
    "| o5 d4 g4 b4 o6 d4 | o6 e4 d8 c8 o5 b4 a4 | o5 g8 a8 b8 o6 c8 d4 f+4 | o6 g2 r2";
static const char CRED_ARP[] =
    "@2 v4 q5 " ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 e", "g", "b", "g") ARP("o4 c", "e", "g", "e")
    ARP("o4 d", "f+", "a", "f+") ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 c", "e", "g", "e")
    ARP("o4 d", "f+", "a", "f+") ARP("o4 g", "b", "o5 d", "o4 b");
static const char CRED_BASS[] =
    "@6 v12 q6 " Q4("o2 g", "d", "g", "d") Q4("o2 e", "b", "e", "b") Q4("o2 c", "g", "c", "g") Q4("o2 d", "a", "d", "a")
    Q4("o2 g", "d", "g", "d") Q4("o2 c", "g", "c", "g") Q4("o2 d", "a", "d", "a") Q4("o2 g", "d", "g", "d");
static const char CRED_DRUM[] = "[@9 v3 o8 c8 @9 v2 o8 c8]32";

/* jingles */
static const char WIN_P1[] = "@39 v12 o5 l16 e g b o6 e8 o5 b16 o6 e16 g4 b4";
static const char WIN_P2[] = "@20 v8 o5 l16 g b o6 e g8 e16 g16 b4 o7 e4";
static const char WIN_TRI[] = "@6 v12 o3 e8 b8 o4 e4 o3 e4";
static const char WIN_NOISE[] = "@11 v9 o6 c8 c8 @12 v8 o5 c2";
static const char LOSE_P1[] = "@0 v11 o5 e8 d+8 d8 c+4 c4 o4 b2";
static const char LOSE_TRI[] = "@6 v12 o2 e4 d+4 d4 c+2";
static const char CHAMP_P1[] = "@39 v12 o5 l16 c e g o6 c8 o5 g16 o6 c16 e8 g8 o7 c4. r8 o6 g8 o7 c2";
static const char CHAMP_P2[] = "@20 v8 o5 l16 e g o6 c e8 c16 e16 g8 o7 c8 e4. r8 o6 b8 o7 e2";
static const char CHAMP_TRI[] = "@6 v12 o3 c8 g8 o4 c4 o3 g4 c4. r8 g8 c2";
static const char CHAMP_NOISE[] = "@11 v9 o6 c8 c8 c8 c8 @12 v9 o5 c2 r8 @12 v8 o5 c2";

void hoop_audio_load(void) {
    if (HOOP_MUS_TITLE >= 0) return;
    HOOP_MUS_TITLE = song_define("hoop_title", 140, true, TITLE_LEAD, TITLE_COMP, TITLE_BASS, TITLE_DRUM);
    HOOP_MUS_SELECT = song_define("hoop_select", 120, true, SEL_LEAD, SEL_COMP, SEL_BASS, SEL_DRUM);
    HOOP_MUS_FIGHT1 = song_define("hoop_fight1", 156, true, F1_LEAD, F1_ARP, F1_BASS, F1_DRUM);
    HOOP_MUS_FIGHT2 = song_define("hoop_fight2", 160, true, F2_LEAD, F2_STAB, F2_BASS, F2_DRUM);
    HOOP_MUS_FIGHT3 = song_define("hoop_fight3", 150, true, F3_LEAD, F3_ARP, F3_BASS, F3_DRUM);
    HOOP_MUS_MIRROR = song_define("hoop_mirror", 168, true, MIR_LEAD, MIR_ARP, MIR_BASS, MIR_DRUM);
    HOOP_MUS_DRAFT = song_define("hoop_draft", 116, true, DR_LEAD, DR_COMP, DR_BASS, DR_DRUM);
    HOOP_MUS_ENDING = song_define("hoop_ending", 104, true, END_LEAD, END_COMP, END_BASS, END_DRUM);
    HOOP_MUS_CREDITS = song_define("hoop_credits", 126, true, CRED_LEAD, CRED_ARP, CRED_BASS, CRED_DRUM);
    HOOP_MUS_WIN = song_define("hoop_win", 150, false, WIN_P1, WIN_P2, WIN_TRI, WIN_NOISE);
    HOOP_MUS_LOSE = song_define("hoop_lose", 100, false, LOSE_P1, "", LOSE_TRI, "");
    HOOP_MUS_CHAMP = song_define("hoop_champ", 140, false, CHAMP_P1, CHAMP_P2, CHAMP_TRI, CHAMP_NOISE);

    sfx_define("hoop_spawn", CH_P1, 200, "@15 v10 o6 c16 e16 g16");
    sfx_define("hoop_ring", CH_P1, 240, "@35 v12 o6 e16 b16");
    sfx_define("hoop_hit", CH_NOISE, 200, "@40 v12 o4 c8");
    sfx_define("hoop_hit2", CH_NOISE, 160, "@40 v14 o3 c8 o4 c8");
    sfx_define("hoop_block", CH_P1, 220, "@15 v12 o6 g16 o5 g16");
    sfx_define("hoop_boom", CH_NOISE, 140, "@34 v14 o3 c4");
    sfx_define("hoop_go", CH_P1, 160, "@15 v13 o6 c8 r16 o7 c4");
    sfx_define("hoop_jump", CH_P2, 240, "@32 v9 o4 c16");
    sfx_define("hoop_flap", CH_P2, 240, "@36 v7 o5 c32");
    sfx_define("hoop_claw", CH_P2, 240, "@38 v9 o4 c16 g16");
    sfx_define("hoop_bite", CH_NOISE, 220, "@21 v11 o6 c16");
    sfx_define("hoop_flip", CH_P2, 220, "@38 v9 o5 c16 o4 c16");
    sfx_define("hoop_trap", CH_P2, 220, "@42 v9 o5 c32 r32 c32");
    sfx_define("hoop_arm", CH_P2, 240, "@20 v7 o6 g32");
    sfx_define("hoop_spring", CH_P1, 220, "@32 v12 o4 c16 g16 o5 c16");
    sfx_define("hoop_snap", CH_NOISE, 200, "@40 v13 o5 c16");
    sfx_define("hoop_lift", CH_P2, 200, "@41 v9 o3 c8");
    sfx_define("hoop_ding", CH_P2, 220, "@15 v9 o6 e16");
    sfx_define("hoop_jet", CH_NOISE, 240, "@36 v6 o6 c32");
    sfx_define("hoop_glow", CH_P1, 200, "@39 v10 o5 c16 e16 g16");
    sfx_define("hoop_lunge", CH_NOISE, 240, "@36 v9 o5 c16");
    sfx_define("hoop_throw", CH_P2, 240, "@36 v7 o6 c32");
    sfx_define("hoop_cog", CH_P2, 240, "@21 v8 o6 c32 c32");
    sfx_define("hoop_rocket", CH_NOISE, 220, "@36 v9 o4 c8");
    sfx_define("hoop_quill", CH_P2, 240, "@20 v6 o7 c32 c32 c32");
    sfx_define("hoop_ray", CH_P2, 240, "@33 v9 o6 c16");
    sfx_define("hoop_ting", CH_P2, 240, "@15 v7 o7 c32");
    sfx_define("hoop_thunk", CH_NOISE, 220, "@13 v10 o3 c16");
    sfx_define("hoop_land", CH_NOISE, 240, "@9 v3 o6 c32");
    sfx_define("hoop_pick", CH_P1, 220, "@35 v12 o5 g16 o6 c16 e16");
    sfx_define("hoop_oldon", CH_P1, 160, "@37 v12 o4 c8 o3 g8 c4");
    sfx_define("hoop_oldoff", CH_P1, 160, "@35 v12 o5 c8 g8 o6 c4");
}
