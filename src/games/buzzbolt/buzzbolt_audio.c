/* BUZZBOLT - original music (UFO-MML) and sound effects. */
#include "buzzbolt.h"

int BZZ_MUS_TITLE = -1, BZZ_MUS_SELECT, BZZ_MUS_WAVE[BZZ_WAVES], BZZ_MUS_BOSS, BZZ_MUS_FINAL, BZZ_MUS_CLEAR,
    BZZ_MUS_OVER, BZZ_MUS_ENDING, BZZ_MUS_CREDITS, BZZ_MUS_NAME;

/* eight eighths of one note: a bar of driving bass */
#define EI(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define EI4(n) n "8 " n "8 " n "8 " n "8 "
#define DRIVE "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
/* a bar of sixteenth-note arpeggio on four notes */
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
#define ARPH(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]2 "
/* a bar of off-beat chord stabs, and half a bar */
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define HOFF(ins, n) ins " r8 " n "8 r8 " n "8 "
/* a bar of sixteenth stabs, and half a bar */
#define ST16(n) "[" n "16 r16]8 "
#define ST16H(n) "[" n "16 r16]4 "

/* "Buzzbolt" - the title, D major */
static const char TITLE_LEAD[] =
    "@1 v11 q6"
    "| o5 a8 f+8 d8 f+8 a4 o6 d4 | o6 c+8 o5 b8 f+8 d8 b4 a4"
    "| o5 g8 b8 o6 d8 o5 b8 o6 e4 d4 | o6 c+8 o5 a8 e8 c+8 a2"
    "| o5 a8 f+8 d8 f+8 a4 o6 d4 | o6 f+8 e8 d8 c+8 d4 o5 b4"
    "| o5 b8 o6 c+8 d8 e8 f+8 e8 c+8 o5 a8 | o6 d2 r2";
static const char TITLE_ARP[] =
    "@2 v5 q5 " ARP("o4 d", "f+", "a", "o5 d") ARP("o4 b", "o5 d", "f+", "d") ARP("o4 g", "b", "o5 d", "o4 b")
    ARP("o4 a", "o5 c+", "e", "c+") ARP("o4 d", "f+", "a", "o5 d") ARP("o4 b", "o5 d", "f+", "d")
    ARPH("o4 g", "b", "o5 d", "o4 b") ARPH("o4 a", "o5 c+", "e", "c+") ARP("o4 d", "f+", "a", "o5 d");
static const char TITLE_BASS[] =
    "@6 v14 q5 " EI("o2 d") EI("o1 b") EI("o1 g") EI("o1 a") EI("o2 d") EI("o1 b") EI4("o1 g") EI4("o1 a") EI("o2 d");
static const char TITLE_DRUM[] = "[" DRIVE DRIVE "]8";

/* "Choose Your Wing" - the ship select, G major */
static const char SEL_LEAD[] = "@14 v10 q7 | o5 d4 g4 b4 a4 | o5 g2 e2 | o5 c4 e4 g4 f+4 | o5 d1";
static const char SEL_ARP[] =
    "@2 v5 q5 [o4 g8 b8 o5 d8 o4 b8]2 [o4 e8 g8 b8 g8]2 [o4 c8 e8 g8 e8]2 [o4 d8 f+8 a8 f+8]2";
static const char SEL_BASS[] = "@6 v12 q7 o2 g2 g2 e2 e2 c2 c2 d2 d2";
static const char SEL_TICK[] = "[@9 v3 o8 c8 @21 v3 o7 c8]16";

/* "Outer Meadow" - wave 1, E minor */
static const char W1_LEAD[] =
    "@1 v11 q6"
    "| o5 e8 g8 b8 g8 o6 e4 d8 o5 b8 | o5 c8 e8 g8 e8 o6 c4 o5 b8 g8"
    "| o5 d8 f+8 a8 f+8 o6 d4 c8 o5 a8 | o5 b8 a8 g8 f+8 d+4 f+4"
    "| o5 e8 g8 b8 o6 e8 g4 f+8 e8 | o6 e8 d8 c8 o5 b8 o6 c4 o5 g4"
    "| o5 a8 o6 c8 e8 d8 c8 o5 b8 a8 f+8 | o5 e2 r4 b4";
static const char W1_OFF[] =
    "v6 q3 " OFF("@17 o4", "e") OFF("@16 o4", "c") OFF("@16 o4", "d") OFF("@16 o3", "b")
    OFF("@17 o4", "e") OFF("@16 o4", "c") HOFF("@17 o4", "a") HOFF("@16 o3", "b") OFF("@17 o4", "e");
static const char W1_BASS[] =
    "@6 v14 q4 " EI("o2 e") EI("o2 c") EI("o2 d") EI("o1 b") EI("o2 e") EI("o2 c") EI4("o1 a") EI4("o1 b") EI("o2 e");
static const char W1_DRUM[] = "[" DRIVE DRIVE "]8";

/* "The Thicket" - wave 2, A minor */
#define OCT(n) "[o2 " n "8 o3 " n "8]4 "
#define OCTH(n) "[o2 " n "8 o3 " n "8]2 "
static const char W2_LEAD[] =
    "@0 v11 q5"
    "| o5 a8 r8 a8 o6 c8 r8 o5 a8 g8 e8 | o5 f8 r8 f8 a8 r8 f8 e8 c8"
    "| o5 g8 r8 g8 b8 r8 o6 d8 c8 o5 b8 | o5 e4 g4 b4 g4"
    "| o5 a8 r8 a8 o6 c8 r8 e8 d8 c8 | o6 c8 r8 o5 a8 f8 r8 a8 o6 c8 f8"
    "| o6 d8 c8 o5 a8 f8 e8 g+8 b8 o6 d8 | o6 c4 o5 a4 a2";
static const char W2_ARP[] =
    "@2 v5 q5 " ARP("o4 a", "o5 c", "e", "c") ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 g", "b", "o5 d", "o4 b")
    ARP("o4 e", "g", "b", "g") ARP("o4 a", "o5 c", "e", "c") ARP("o4 f", "a", "o5 c", "o4 a")
    ARPH("o4 d", "f", "a", "f") ARPH("o4 e", "g+", "b", "g+") ARP("o4 a", "o5 c", "e", "c");
static const char W2_BASS[] =
    "@6 v14 q4 " OCT("a") OCT("f") OCT("g") OCT("e") OCT("a") OCT("f") OCTH("d") OCTH("e") OCT("a");
static const char W2_DRUM[] = "[@13 v11 o2 c8 @9 v4 o8 c16 c16 @11 v9 o6 c8 @9 v4 o8 c16 c16]16";

/* "The Rot" - wave 3, D minor */
static const char W3_LEAD[] =
    "@1 v11 q5"
    "| o5 d8 d8 f8 d8 a8 d8 g+8 a8 | o5 d8 d8 f8 d8 o6 c8 o5 a8 f8 e8"
    "| o5 b-8 b-8 o6 d8 o5 b-8 f8 b-8 a8 g8 | o5 a8 c+8 e8 a8 g8 f8 e8 c+8"
    "| o5 d8 d8 f8 d8 a8 d8 g+8 a8 | o5 g8 b-8 o6 d8 o5 b-8 o6 g8 f8 e8 d8"
    "| o6 d8 c8 o5 b-8 a8 g8 f8 e8 c+8 | o5 d2 r2";
static const char W3_STAB[] =
    "v6 q2 @17 o4 " ST16("d") ST16("d") "@16 o3 " ST16("b-") "@16 o3 " ST16("a") "@17 o4 " ST16("d")
    "@17 o3 " ST16("g") "@16 o3 " ST16H("b-") "@16 o3 " ST16H("a") "@17 o4 " ST16("d");
static const char W3_BASS[] =
    "@6 v14 q4 " EI("o2 d") EI("o2 d") EI("o1 b-") EI("o1 a") EI("o2 d") EI("o1 g") EI4("o1 b-") EI4("o1 a") EI("o2 d");
static const char W3_DRUM[] = "[@13 v12 o2 c16 @9 v5 o8 c16 @11 v10 o6 c16 @9 v5 o8 c16]32";

/* "The Walls" - wave 4, C minor, a march */
#define MARCH(ins, n) ins " " n "4 r4 " n "4 " n "8 " n "8 "
#define HMARCH(ins, n) ins " " n "4 " n "8 " n "8 "
#define Q(a, b) a "4 " b "4 " a "4 " b "4 "
static const char W4_LEAD[] =
    "@23 v11 q6"
    "| o5 c4 e-8 g8 o6 c4 o5 b-8 g8 | o5 a-4 c8 e-8 a-4 g8 f8"
    "| o5 b-4 d8 f8 b-4 a-8 g8 | o5 g4 b8 o6 d8 f4 e-8 d8"
    "| o6 c4 o5 g8 e-8 c4 e-8 g8 | o5 a-4 o6 c8 e-8 d-4 c8 o5 a-8"
    "| o5 f8 a-8 o6 c8 o5 a-8 g8 b8 o6 d8 f8 | o6 e-4 d4 c2";
static const char W4_CHORD[] =
    "v6 q4 " MARCH("@17 o4", "c") MARCH("@16 o3", "a-") MARCH("@16 o3", "b-") MARCH("@16 o3", "g")
    MARCH("@17 o4", "c") MARCH("@16 o3", "a-") HMARCH("@17 o3", "f") HMARCH("@16 o3", "g") MARCH("@17 o4", "c");
static const char W4_BASS[] =
    "@6 v14 q5 " Q("o2 c", "o2 g") Q("o1 a-", "o2 e-") Q("o1 b-", "o2 f") Q("o1 g", "o2 d") Q("o2 c", "o2 g")
    Q("o1 a-", "o2 e-") "o1 f4 o2 c4 o1 g4 o2 d4 " Q("o2 c", "o2 g");
static const char W4_DRUM[] = "[@13 v12 o2 c8 @11 v7 o6 c16 c16 @11 v11 o6 c8 @9 v4 o8 c8]16";

/* "The Big Ones" - the bosses, E minor */
static const char BOSS_LEAD[] =
    "@1 v12 q5"
    "| o5 e8 e8 g8 e8 b8 e8 a+8 b8 | o5 e8 e8 g8 e8 o6 c8 o5 b8 a8 g8"
    "| o5 c8 c8 e8 c8 g8 c8 f+8 g8 | o5 d8 f+8 a8 o6 d8 c8 o5 a8 f+8 d8"
    "| o5 e8 e8 g8 e8 b8 e8 a+8 b8 | o5 e8 g8 b8 o6 e8 g8 f+8 e8 d8"
    "| o6 c8 o5 b8 a8 g8 f+8 d+8 f+8 a8 | o5 b4 o6 e4 o5 e2";
static const char BOSS_STAB[] =
    "v6 q2 @17 o4 " ST16("e") ST16("e") "@16 o4 " ST16("c") "@16 o4 " ST16("d") "@17 o4 " ST16("e") ST16("e")
    "@16 o4 " ST16H("c") "@16 o3 " ST16H("b") "@17 o4 " ST16("e");
static const char BOSS_BASS[] =
    "@6 v14 q4 " EI("o2 e") EI("o2 e") EI("o2 c") EI("o2 d") EI("o2 e") EI("o2 e") EI4("o2 c") EI4("o1 b") EI("o2 e");
static const char BOSS_DRUM[] = "[@13 v12 o2 c16 @9 v5 o8 c16 @11 v10 o6 c16 @9 v5 o8 c16]32";

/* "The Sporeheart" - wave 5, B minor */
static const char FIN_LEAD[] =
    "@14 v12 q7"
    "| o5 b4. a8 b4 o6 d4 | o6 d4. c+8 o5 b4 g4 | o5 e4. f+8 g4 b4 | o5 a+4 o6 c+4 e4 c+4"
    "| o5 b4. a8 b4 o6 f+4 | o6 g4. f+8 e4 d4 | o6 e4 d4 c+4 o5 a+4 | o5 b1";
static const char FIN_ARP[] =
    "@2 v5 q5 " ARP("o4 b", "o5 d", "f+", "d") ARP("o4 g", "b", "o5 d", "o4 b") ARP("o4 e", "g", "b", "g")
    ARP("o4 f+", "a+", "o5 c+", "o4 a+") ARP("o4 b", "o5 d", "f+", "d") ARP("o4 g", "b", "o5 d", "o4 b")
    ARPH("o4 e", "g", "b", "g") ARPH("o4 f+", "a+", "o5 c+", "o4 a+") ARP("o4 b", "o5 d", "f+", "d");
static const char FIN_BASS[] =
    "@6 v14 q4 " EI("o1 b") EI("o1 g") EI("o2 e") EI("o1 f+") EI("o1 b") EI("o1 g") EI4("o2 e") EI4("o1 f+") EI("o1 b");
static const char FIN_DRUM[] = "[@13 v12 o2 c8 @9 v5 o8 c8 @11 v11 o6 c8 @13 v10 o2 c8]16";

/* jingles */
static const char CLEAR_P1[] = "@23 v12 o5 l16 g o6 c e g8 e8 g4";
static const char CLEAR_TRI[] = "@6 v13 o3 c8 g8 o4 c4 c4";
static const char CLEAR_NOISE[] = "@12 v8 o5 c2";
static const char OVER_P1[] = "@5 v11 o5 l8 b a g f+ e d+ e2";
static const char OVER_TRI[] = "@6 v13 o3 l4 e b o2 e2";

/* "Bloom" - the ending, F major */
static const char END_LEAD[] =
    "@14 v11 q7"
    "| o5 a4 o6 c4 f4 a4 | o6 g2 e4 c4 | o6 d4 f4 a4 o7 d4 | o7 c2 o6 b-4 a4"
    "| o6 a4 o7 c4 f4 e4 | o7 d2 c4 o6 b-4 | o6 a4 g4 f4 e4 | o6 f1";
static const char END_ARP[] =
    "@22 v6 q6 [o4 f8 a8 o5 c8 o4 a8]2 [o4 e8 g8 o5 c8 o4 g8]2 [o4 d8 f8 a8 f8]2 [o4 d8 f8 b-8 f8]2"
    " [o4 f8 a8 o5 c8 o4 a8]2 [o4 e8 g8 o5 c8 o4 g8]2 o4 d8 f8 b-8 f8 o4 e8 g8 o5 c8 o4 g8 [o4 f8 a8 o5 c8 o4 a8]2";
static const char END_BASS[] = "@6 v12 q7 o2 f1 c1 d1 o1 b-1 o2 f1 c1 o1 b-2 o2 c2 f1";
static const char END_SOFT[] = "[@9 v3 o8 c4 @21 v3 o7 c4]16";

/* "Flying Home" - the credits, G major */
static const char CRED_LEAD[] =
    "@0 v11 q6"
    "| o5 b8 o6 d8 g8 d8 o5 b4 a4 | o5 a8 o6 d8 f+8 d8 o5 a4 f+4"
    "| o5 g8 b8 o6 e8 d8 o5 b4 g4 | o5 e8 g8 o6 c8 o5 b8 a2"
    "| o5 b8 o6 d8 g8 a8 b4 a4 | o6 a8 f+8 d8 f+8 a4 g4"
    "| o6 e8 d8 c8 o5 b8 a8 b8 o6 c8 d8 | o6 g2 r2";
static const char CRED_ARP[] =
    "@2 v5 q5 [o4 g8 b8 o5 d8 o4 b8]2 [o4 f+8 a8 o5 d8 o4 a8]2 [o4 e8 g8 b8 g8]2 [o4 e8 g8 o5 c8 o4 g8]2"
    " [o4 g8 b8 o5 d8 o4 b8]2 [o4 f+8 a8 o5 d8 o4 a8]2 o4 e8 g8 o5 c8 o4 g8 o4 f+8 a8 o5 d8 o4 a8 [o4 g8 b8 o5 d8 o4 b8]2";
static const char CRED_BASS[] =
    "@6 v12 q5 " EI("o2 g") EI("o2 d") EI("o2 e") EI("o2 c") EI("o2 g") EI("o2 d") EI4("o2 c") EI4("o2 d") EI("o2 g");
static const char CRED_DRUM[] = "[@13 v10 o2 c4 @11 v8 o6 c4]16";

/* "Sign Your Name" - the high-score entry, C major */
static const char NAME_LEAD[] =
    "@15 v10 q7 | o5 e8 g8 o6 c8 o5 g8 a4 g4 | o5 f8 a8 o6 c8 o5 a8 g2 | o5 e8 g8 o6 c8 e8 d4 c4 | o5 b8 o6 d8 g8 d8 c2";
static const char NAME_BASS[] = "@6 v12 q7 o3 c2 o2 g2 f2 g2 o3 c2 o2 a2 g2 o3 c2";
static const char NAME_TICK[] = "[@21 v3 o7 c8 @9 v2 o8 c8]16";

void bzz_audio_load(void) {
    if (BZZ_MUS_TITLE >= 0) return;
    BZZ_MUS_TITLE = song_define("bzz_title", 144, true, TITLE_LEAD, TITLE_ARP, TITLE_BASS, TITLE_DRUM);
    BZZ_MUS_SELECT = song_define("bzz_select", 120, true, SEL_LEAD, SEL_ARP, SEL_BASS, SEL_TICK);
    BZZ_MUS_WAVE[0] = song_define("bzz_meadow", 150, true, W1_LEAD, W1_OFF, W1_BASS, W1_DRUM);
    BZZ_MUS_WAVE[1] = song_define("bzz_thicket", 138, true, W2_LEAD, W2_ARP, W2_BASS, W2_DRUM);
    BZZ_MUS_WAVE[2] = song_define("bzz_rot", 132, true, W3_LEAD, W3_STAB, W3_BASS, W3_DRUM);
    BZZ_MUS_WAVE[3] = song_define("bzz_walls", 144, true, W4_LEAD, W4_CHORD, W4_BASS, W4_DRUM);
    BZZ_MUS_BOSS = song_define("bzz_boss", 160, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    BZZ_MUS_FINAL = song_define("bzz_heart", 150, true, FIN_LEAD, FIN_ARP, FIN_BASS, FIN_DRUM);
    BZZ_MUS_WAVE[4] = BZZ_MUS_FINAL; /* the last wave is nothing but its boss */
    BZZ_MUS_CLEAR = song_define("bzz_clear", 150, false, CLEAR_P1, "", CLEAR_TRI, CLEAR_NOISE);
    BZZ_MUS_OVER = song_define("bzz_over", 110, false, OVER_P1, "", OVER_TRI, "");
    BZZ_MUS_ENDING = song_define("bzz_bloom", 100, true, END_LEAD, END_ARP, END_BASS, END_SOFT);
    BZZ_MUS_CREDITS = song_define("bzz_home", 120, true, CRED_LEAD, CRED_ARP, CRED_BASS, CRED_DRUM);
    BZZ_MUS_NAME = song_define("bzz_name", 112, true, NAME_LEAD, "", NAME_BASS, NAME_TICK);

    sfx_define("bzz_shot", CH_P2, 240, "@20 v5 o6 a32");
    sfx_define("bzz_shot2", CH_P2, 240, "@20 v6 o5 e32 a32");
    sfx_define("bzz_lance", CH_P2, 200, "@38 v12 o4 c16 g16 o5 c8");
    sfx_define("bzz_pop", CH_NOISE, 240, "@21 v8 o6 c32");
    sfx_define("bzz_bigboom", CH_NOISE, 120, "@34 v15 o3 c2");
    sfx_define("bzz_die", CH_P1, 160, "@33 v13 o5 c8 o4 g8 e8 c4");
    sfx_define("bzz_letb", CH_P1, 240, "@35 v10 o6 c16 e16");
    sfx_define("bzz_letz", CH_P1, 240, "@35 v10 o6 g16 b16");
    sfx_define("bzz_mult", CH_P1, 220, "@39 v12 o5 l32 c e g o6 c e g");
    sfx_define("bzz_option", CH_P1, 200, "@39 v11 o5 l16 e g b o6 e");
    sfx_define("bzz_special", CH_P1, 240, "@39 v12 o5 l32 c d e f g a b o6 c d e f g");
    sfx_define("bzz_wrong", CH_P1, 180, "@37 v12 o4 c8 o3 f+8");
    sfx_define("bzz_extend", CH_P1, 180, "@39 v13 o5 l16 c e g o6 c8 e8 g4");
    sfx_define("bzz_optlost", CH_P2, 200, "@33 v9 o5 c16 o4 c16");
    sfx_define("bzz_bomb", CH_P2, 220, "@32 v10 o4 c16 g16");
    sfx_define("bzz_blast", CH_NOISE, 140, "@34 v14 o4 c4");
    sfx_define("bzz_efire", CH_NOISE, 240, "@36 v6 o7 c32");
    sfx_define("bzz_efire2", CH_NOISE, 220, "@36 v8 o6 c16");
    sfx_define("bzz_homing", CH_P2, 220, "@38 v7 o5 c16 e16");
    sfx_define("bzz_ring", CH_P2, 220, "@15 v8 o5 g16 o6 c16");
    sfx_define("bzz_puff", CH_NOISE, 200, "@36 v8 o5 c8");
    sfx_define("bzz_launch", CH_P1, 200, "@32 v12 o4 c8 g8 o5 c4");
}
