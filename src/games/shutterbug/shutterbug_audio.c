/* SHUTTERBUG - original music (UFO-MML) and sound effects. */
#include "shutterbug.h"

int SHB_MUS_TITLE = -1, SHB_MUS_STORY, SHB_MUS_STAGE[SHB_STAGES], SHB_MUS_BOSS, SHB_MUS_TRUE, SHB_MUS_CLEAR,
    SHB_MUS_LOST, SHB_MUS_OVER, SHB_MUS_ENDING, SHB_MUS_CREDITS;

/* a bar of eighth-note bass, and half a bar */
#define EI(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
#define EI4(n) n "8 " n "8 " n "8 " n "8 "
#define DRIVE "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
/* a bar of sixteenth-note arpeggio on four notes, and half a bar */
#define ARP(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]4 "
#define ARPH(a, b, c, d) "[" a "16 " b "16 " c "16 " d "16]2 "
/* off-beat chord stabs: a bar, half a bar */
#define OFF(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define HOFF(ins, n) ins " r8 " n "8 r8 " n "8 "
/* sixteenth stabs: a bar, half a bar */
#define ST16(n) "[" n "16 r16]8 "
#define ST16H(n) "[" n "16 r16]4 "
/* a tom-tom bass bar: root, root, fifth, root */
#define TB(r, f) r "4 " r "8 " r "8 " f "4 " r "4 "

/* "Say Cheese" - the title, F major */
static const char TITLE_LEAD[] =
    "@1 v11 q6"
    "| o5 c8 f8 a8 o6 c8 o5 a4 f4 | o5 g8 a8 b-8 a8 g4 e4 | o5 f8 a8 o6 c8 d8 c4 o5 a4 | o5 g2 r4 c4"
    "| o5 f8 a8 o6 c8 f8 e4 d4 | o6 c8 o5 b-8 a8 g8 a4 f4 | o5 d8 f8 a8 o6 c8 o5 b-4 g4 | o5 f2. r4";
static const char TITLE_ARP[] =
    "@2 v5 q5 " ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 e", "g", "o5 c", "o4 g") ARP("o4 f", "a", "o5 c", "o4 a")
    ARP("o4 e", "g", "o5 c", "o4 g") ARP("o4 f", "a", "o5 c", "o4 a") ARP("o4 f", "a", "o5 c", "o4 a")
    ARPH("o4 f", "b-", "o5 d", "o4 b-") ARPH("o4 e", "g", "o5 c", "o4 g") ARP("o4 f", "a", "o5 c", "o4 a");
static const char TITLE_BASS[] =
    "@6 v14 q5 " EI("o2 f") EI("o2 c") EI("o2 f") EI("o2 c") EI("o2 f") EI("o2 f") EI4("o1 b-") EI4("o2 c") EI("o2 f");
static const char TITLE_DRUM[] = "[" DRIVE DRIVE "]8";

/* "New Camera" - the story, C major */
static const char STORY_LEAD[] =
    "@14 v10 q7 | o5 e4 g4 o6 c4 o5 b4 | o5 a2 g2 | o5 f4 a4 o6 c4 o5 a4 | o5 g1"
    "| o5 e4 g4 o6 c4 e4 | o6 d2 c4 o5 a4 | o5 f4 e4 d4 g4 | o5 c1";
static const char STORY_ARP[] =
    "@22 v6 q6 [o4 c8 e8 g8 e8]2 [o4 c8 f8 a8 f8]2 [o4 c8 f8 a8 f8]2 [o4 d8 g8 b8 g8]2 [o4 c8 e8 g8 e8]2"
    " [o4 d8 f8 a8 f8]2 o4 c8 f8 a8 f8 o4 d8 g8 b8 g8 [o4 c8 e8 g8 e8]2";
static const char STORY_BASS[] = "@6 v12 q7 o2 c1 f1 f1 g1 c1 d1 f2 g2 c1";
static const char STORY_SOFT[] = "[@9 v3 o8 c4 @21 v3 o7 c4]16";

/* "Home Orbit" - the prologue, G major, a lounge tune */
static const char HOME_LEAD[] =
    "@5 v11 q7 | o5 b4. a8 g4 d4 | o5 e4 g4 b2 | o5 a4. g8 f+4 d4 | o5 e2 d2"
    "| o5 b4. a8 g4 b4 | o6 d4 c4 o5 b4 a4 | o5 g4 a4 b4 f+4 | o5 g1";
static const char HOME_COMP[] =
    "v6 q3 " OFF("@16 o4", "g") OFF("@16 o4", "c") OFF("@16 o4", "d") OFF("@16 o4", "c") OFF("@16 o4", "g")
    OFF("@16 o4", "d") HOFF("@16 o4", "c") HOFF("@16 o4", "d") OFF("@16 o4", "g");
static const char HOME_BASS[] =
    "@7 v14 q6 | o2 g4 b4 o3 d4 o2 b4 | o2 c4 e4 g4 e4 | o2 d4 f+4 a4 f+4 | o2 c4 e4 g4 e4"
    "| o2 g4 b4 o3 d4 o2 b4 | o2 d4 f+4 a4 f+4 | o2 c4 e4 d4 f+4 | o2 g4 d4 g2";
static const char HOME_DRUM[] = "[@9 v4 o8 c8 @9 v2 o8 c16 c16 @11 v6 o6 c8 @9 v3 o8 c8]16";

/* "Teatime Planet" - stage 1, D major */
static const char TEA_LEAD[] =
    "@1 v11 q6"
    "| o5 d8 f+8 a8 f+8 o6 d4 c+8 o5 a8 | o5 b8 a8 g8 f+8 e4 c+4 | o5 d8 f+8 a8 o6 d8 e8 d8 c+8 o5 b8 | o5 a2 r4 a4"
    "| o5 g8 b8 o6 d8 o5 b8 o6 g4 f+8 e8 | o6 f+8 e8 d8 c+8 o5 b4 a4 | o5 g8 f+8 e8 g8 f+8 e8 d8 c+8 | o5 d2 r2";
static const char TEA_ARP[] =
    "@2 v5 q5 " ARP("o4 d", "f+", "a", "f+") ARP("o4 c+", "e", "a", "e") ARP("o4 d", "f+", "a", "f+")
    ARP("o4 c+", "e", "a", "e") ARP("o4 d", "g", "b", "g") ARP("o4 d", "f+", "a", "f+") ARPH("o4 e", "g", "b", "g")
    ARPH("o4 c+", "e", "a", "e") ARP("o4 d", "f+", "a", "f+");
static const char TEA_BASS[] =
    "@6 v14 q4 " EI("o2 d") EI("o2 a") EI("o2 d") EI("o2 a") EI("o2 g") EI("o2 d") EI4("o2 e") EI4("o2 a") EI("o2 d");
static const char TEA_DRUM[] = "[" DRIVE DRIVE "]8";

/* "Comet Rain" - stages 2 and 4, E minor */
static const char COMET_LEAD[] =
    "@0 v11 q5"
    "| o5 e8 e8 g8 e8 b8 e8 a8 g8 | o5 f+8 f+8 a8 f+8 o6 c8 o5 b8 a8 f+8 | o5 g8 g8 b8 g8 o6 e8 d8 c8 o5 b8"
    "| o5 a8 b8 o6 c8 d8 o5 b4 a4 | o5 e8 e8 g8 e8 b8 e8 o6 d8 c8 | o5 b8 a8 g8 a8 b4 o6 e4"
    "| o6 d8 c8 o5 b8 a8 g8 f+8 d+8 f+8 | o5 e2 r2";
static const char COMET_STAB[] =
    "v6 q2 @17 o4 " ST16("e") "@16 o4 " ST16("d") "@16 o4 " ST16("c") "@17 o4 " ST16H("a") "@16 o3 " ST16H("b")
    "@17 o4 " ST16("e") "@16 o4 " ST16("c") "@16 o3 " ST16("b") "@17 o4 " ST16("e");
static const char COMET_BASS[] =
    "@6 v14 q4 " EI("o2 e") EI("o2 d") EI("o2 c") EI4("o2 a") EI4("o2 b") EI("o2 e") EI("o2 c") EI("o2 b") EI("o2 e");
static const char COMET_DRUM[] = "[@13 v12 o2 c16 @9 v5 o8 c16 @11 v10 o6 c16 @9 v5 o8 c16]32";

/* "Gloom Planet" - stage 3, A minor */
static const char GLOOM_LEAD[] =
    "@5 v11 q7 | o5 a4. b8 o6 c4 o5 b4 | o5 a4 e4 g+2 | o5 f4. e8 d4 f4 | o5 e1"
    "| o5 a4. b8 o6 c4 d4 | o6 e4 d4 c4 o5 b4 | o5 a4 g+4 b4 e4 | o5 a1";
static const char GLOOM_ARP[] =
    "@2 v4 q5 " ARP("o4 a", "o5 c", "e", "c") ARP("o4 e", "g+", "b", "g+") ARP("o4 d", "f", "a", "f")
    ARP("o4 e", "g+", "b", "g+") ARP("o4 a", "o5 c", "e", "c") ARP("o4 e", "a", "o5 c", "o4 a") ARPH("o4 d", "f", "a", "f")
    ARPH("o4 e", "g+", "b", "g+") ARP("o4 a", "o5 c", "e", "c");
static const char GLOOM_BASS[] =
    "@6 v13 q7 o2 a2 e2 | o2 e2 b2 | o2 d2 a2 | o2 e2 g+2 | o2 a2 e2 | o2 c2 g2 | o2 d2 e2 | o2 a2 e2";
static const char GLOOM_DRUM[] = "[@13 v10 o2 c4 @9 v3 o8 c8 c8 @11 v7 o6 c4 @9 v3 o8 c8 c8]8";

/* "Fossil Planet" - stage 5, C minor */
static const char FOSSIL_LEAD[] =
    "@23 v11 q6"
    "| o5 c4 e-8 f8 g4 b-8 g8 | o5 f8 e-8 c8 e-8 f4 g4 | o5 a-4 g8 f8 e-4 c8 e-8 | o5 d8 c8 o4 b-8 o5 d8 c2"
    "| o5 c4 e-8 f8 g4 o6 c8 o5 b-8 | o5 a-8 g8 f8 a-8 g4 e-4 | o5 f8 g8 a-8 b-8 g8 f8 e-8 d8 | o5 c2 r2";
static const char FOSSIL_CHORD[] =
    "v6 q3 " OFF("@17 o4", "c") OFF("@17 o4", "c") OFF("@16 o3", "a-") OFF("@16 o3", "g") OFF("@17 o4", "c")
    OFF("@16 o3", "a-") HOFF("@16 o3", "b-") HOFF("@16 o3", "g") OFF("@17 o4", "c");
static const char FOSSIL_BASS[] =
    "@6 v14 q5 " TB("o2 c", "o2 g") TB("o2 c", "o2 g") TB("o1 a-", "o2 e-") TB("o1 g", "o2 d") TB("o2 c", "o2 g")
    TB("o1 a-", "o2 e-") "o1 b-4 b-8 b-8 o1 g4 g4 " TB("o2 c", "o2 g");
static const char FOSSIL_DRUM[] =
    "[@13 v12 o2 c8 @11 v6 o5 c8 @13 v9 o2 c8 @11 v6 o5 c8 @13 v12 o2 c8 @13 v8 o3 c8 @11 v11 o6 c8 @9 v4 o8 c8]8";

/* "Flash Point" - the bosses, E minor */
static const char BOSS_LEAD[] =
    "@1 v12 q5"
    "| o5 e8 b8 o6 e8 o5 b8 o6 d8 o5 b8 a8 g8 | o5 f+8 g8 a8 b8 o6 c4 o5 b4 | o5 e8 b8 o6 e8 o5 b8 o6 f+8 e8 d8 c8"
    "| o5 b2 r4 b4 | o6 c8 o5 b8 a8 g8 a8 b8 o6 c8 d8 | o6 e8 d8 c8 o5 b8 a4 g4"
    "| o5 f+8 g8 a8 f+8 d+8 e8 f+8 d+8 | o5 e2 r2";
static const char BOSS_STAB[] =
    "v6 q2 @17 o4 " ST16("e") "@16 o4 " ST16("d") "@17 o4 " ST16("e") "@16 o3 " ST16("b") "@16 o4 " ST16("c")
    "@16 o4 " ST16("c") "@16 o3 " ST16("b") "@17 o4 " ST16("e");
static const char BOSS_BASS[] =
    "@6 v14 q4 " EI("o2 e") EI("o2 d") EI("o2 e") EI("o1 b") EI("o2 c") EI("o2 c") EI("o1 b") EI("o2 e");
static const char BOSS_DRUM[] = "[@13 v12 o2 c16 @9 v5 o8 c16 @11 v10 o6 c16 @9 v5 o8 c16]32";

/* "The Kaleidoscope" - the true boss, B minor */
static const char LENS_LEAD[] =
    "@14 v12 q7 | o5 b4 o6 d4 f+4 e8 d8 | o6 c+4 o5 a4 f+2 | o5 g4 b4 o6 d4 c+8 o5 b8 | o5 a+2 f+2"
    "| o5 b4 o6 d4 f+4 a4 | o6 g4 f+4 e4 d4 | o6 e4 c+4 o5 a+4 f+4 | o5 b1";
static const char LENS_ARP[] =
    "@2 v5 q5 " ARP("o4 b", "o5 d", "f+", "d") ARP("o4 f+", "a", "o5 c+", "o4 a") ARP("o4 g", "b", "o5 d", "o4 b")
    ARP("o4 f+", "a+", "o5 c+", "o4 a+") ARP("o4 b", "o5 d", "f+", "d") ARP("o4 g", "b", "o5 e", "o4 b")
    ARP("o4 f+", "a+", "o5 c+", "o4 a+") ARP("o4 b", "o5 d", "f+", "d");
static const char LENS_BASS[] =
    "@6 v14 q4 " EI("o1 b") EI("o1 f+") EI("o1 g") EI("o1 f+") EI("o1 b") EI("o1 g") EI("o1 f+") EI("o1 b");
static const char LENS_DRUM[] = "[@13 v12 o2 c8 @9 v5 o8 c8 @11 v11 o6 c8 @13 v10 o2 c8]16";

/* jingles */
static const char CLEAR_P1[] = "@23 v12 o5 l16 d f+ a o6 d8 o5 a8 o6 d4";
static const char CLEAR_TRI[] = "@6 v13 o3 d8 a8 o4 d4 d4";
static const char CLEAR_NOISE[] = "@12 v8 o5 c2";
static const char LOST_P1[] = "@5 v11 o5 l8 a g f+ e d4 r4";
static const char LOST_TRI[] = "@6 v13 o3 l4 d o2 a o2 d";
static const char OVER_P1[] = "@5 v11 o5 l8 e d c o4 b a g+ a2";
static const char OVER_TRI[] = "@6 v13 o3 l4 a e o2 a2";

/* "Postcards Home" - the ending, F major */
static const char END_LEAD[] =
    "@14 v11 q7 | o5 a4 o6 c4 f4 e4 | o6 d2 c2 | o5 b-4 o6 d4 f4 d4 | o6 c1"
    "| o5 a4 o6 c4 f4 a4 | o6 g2 f4 e4 | o6 d4 c4 o5 b-4 o6 e4 | o6 f1";
static const char END_ARP[] =
    "@22 v6 q6 [o4 f8 a8 o5 c8 o4 a8]2 [o4 f8 b-8 o5 d8 o4 b-8]2 [o4 f8 b-8 o5 d8 o4 b-8]2 [o4 e8 g8 o5 c8 o4 g8]2"
    " [o4 f8 a8 o5 c8 o4 a8]2 [o4 e8 g8 o5 c8 o4 g8]2 o4 f8 b-8 o5 d8 o4 b-8 o4 e8 g8 o5 c8 o4 g8 [o4 f8 a8 o5 c8 o4 a8]2";
static const char END_BASS[] = "@6 v12 q7 o2 f1 o1 b-1 o1 b-1 o2 c1 o2 f1 o2 c1 o1 b-2 o2 c2 o2 f1";
static const char END_SOFT[] = "[@9 v3 o8 c4 @21 v3 o7 c4]16";

/* "The Album" - the credits, G major */
static const char CRED_LEAD[] =
    "@0 v11 q6"
    "| o5 g8 b8 o6 d8 o5 b8 o6 c4 o5 a4 | o5 b8 o6 d8 g8 d8 e4 d4 | o6 c8 o5 b8 a8 g8 f+4 a4 | o5 g8 f+8 e8 f+8 d2"
    "| o5 g8 b8 o6 d8 g8 f+4 e4 | o6 d8 c8 o5 b8 a8 b4 g4 | o5 e8 f+8 g8 a8 b8 a8 g8 f+8 | o5 g2 r2";
static const char CRED_ARP[] =
    "@2 v5 q5 [o4 g8 b8 o5 d8 o4 b8]2 [o4 g8 b8 o5 d8 o4 b8]2 [o4 e8 a8 o5 c8 o4 a8]2 [o4 d8 f+8 a8 f+8]2"
    " [o4 g8 b8 o5 d8 o4 b8]2 [o4 d8 f+8 a8 f+8]2 o4 e8 g8 o5 c8 o4 g8 o4 d8 f+8 a8 f+8 [o4 g8 b8 o5 d8 o4 b8]2";
static const char CRED_BASS[] =
    "@6 v12 q5 " EI("o2 g") EI("o2 g") EI("o2 a") EI("o2 d") EI("o2 g") EI("o2 d") EI4("o2 c") EI4("o2 d") EI("o2 g");
static const char CRED_DRUM[] = "[@13 v10 o2 c4 @11 v8 o6 c4]16";

void shb_audio_load(void) {
    if (SHB_MUS_TITLE >= 0) return;
    SHB_MUS_TITLE = song_define("shb_title", 132, true, TITLE_LEAD, TITLE_ARP, TITLE_BASS, TITLE_DRUM);
    SHB_MUS_STORY = song_define("shb_story", 100, true, STORY_LEAD, STORY_ARP, STORY_BASS, STORY_SOFT);
    SHB_MUS_STAGE[0] = song_define("shb_home", 116, true, HOME_LEAD, HOME_COMP, HOME_BASS, HOME_DRUM);
    SHB_MUS_STAGE[1] = song_define("shb_teatime", 144, true, TEA_LEAD, TEA_ARP, TEA_BASS, TEA_DRUM);
    SHB_MUS_STAGE[2] = song_define("shb_comet", 150, true, COMET_LEAD, COMET_STAB, COMET_BASS, COMET_DRUM);
    SHB_MUS_STAGE[3] = song_define("shb_gloom", 120, true, GLOOM_LEAD, GLOOM_ARP, GLOOM_BASS, GLOOM_DRUM);
    SHB_MUS_STAGE[4] = SHB_MUS_STAGE[2]; /* both Comet Rains share their tune */
    SHB_MUS_STAGE[5] = song_define("shb_fossil", 140, true, FOSSIL_LEAD, FOSSIL_CHORD, FOSSIL_BASS, FOSSIL_DRUM);
    SHB_MUS_BOSS = song_define("shb_boss", 160, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    SHB_MUS_TRUE = song_define("shb_lens", 150, true, LENS_LEAD, LENS_ARP, LENS_BASS, LENS_DRUM);
    SHB_MUS_STAGE[6] = SHB_MUS_TRUE;
    SHB_MUS_CLEAR = song_define("shb_clear", 150, false, CLEAR_P1, "", CLEAR_TRI, CLEAR_NOISE);
    SHB_MUS_LOST = song_define("shb_lost", 120, false, LOST_P1, "", LOST_TRI, "");
    SHB_MUS_OVER = song_define("shb_over", 110, false, OVER_P1, "", OVER_TRI, "");
    SHB_MUS_ENDING = song_define("shb_ending", 100, true, END_LEAD, END_ARP, END_BASS, END_SOFT);
    SHB_MUS_CREDITS = song_define("shb_credits", 124, true, CRED_LEAD, CRED_ARP, CRED_BASS, CRED_DRUM);

    sfx_define("shb_shot", CH_P2, 240, "@20 v4 o6 e32");
    sfx_define("shb_shot2", CH_P2, 240, "@20 v4 o6 a32");
    sfx_define("shb_ready", CH_P2, 220, "@39 v9 o6 c32 g32");
    sfx_define("shb_rings", CH_P2, 200, "@38 v12 o4 g16 o5 d16 g8");
    sfx_define("shb_snap", CH_NOISE, 200, "@40 v13 o6 c16 @36 v10 o7 c16");
    sfx_define("shb_empty", CH_P2, 200, "@42 v6 o4 c32");
    sfx_define("shb_pop", CH_NOISE, 240, "@21 v8 o6 c32");
    sfx_define("shb_pop2", CH_P2, 220, "@32 v7 o5 c32 e32");
    sfx_define("shb_bigboom", CH_NOISE, 120, "@34 v15 o3 c2");
    sfx_define("shb_die", CH_P1, 160, "@33 v13 o5 c8 o4 g8 e8 c4");
    sfx_define("shb_armour", CH_P1, 200, "@37 v12 o4 g16 c16");
    sfx_define("shb_extend", CH_P1, 180, "@39 v13 o5 l16 c e g o6 c8 e8 g4");
    sfx_define("shb_bulb", CH_P1, 240, "@35 v10 o6 e16 b16");
    sfx_define("shb_wrench", CH_P1, 200, "@39 v12 o5 l16 g b o6 d g");
    sfx_define("shb_repair", CH_P1, 220, "@35 v11 o5 c16 g16 o6 c16");
    sfx_define("shb_orb", CH_P1, 240, "@35 v9 o6 g16 o7 c16");
    sfx_define("shb_clink", CH_P2, 240, "@20 v7 o7 c32");
    sfx_define("shb_crumble", CH_NOISE, 200, "@36 v8 o5 c16");
    sfx_define("shb_letter", CH_P1, 180, "@39 v13 o5 l32 c e g o6 c e g o7 c4");
    sfx_define("shb_secret", CH_P1, 200, "@39 v11 o6 l32 c d e f g a b o7 c");
    sfx_define("shb_cling", CH_P2, 220, "@32 v8 o4 c16 e16");
    sfx_define("shb_burst", CH_NOISE, 200, "@36 v10 o5 c8");
    sfx_define("shb_launch", CH_NOISE, 200, "@36 v9 o6 c16 o5 c16");
    sfx_define("shb_drop", CH_P2, 220, "@33 v7 o6 c16");
    sfx_define("shb_hum", CH_P2, 200, "@15 v6 o3 c8");
    sfx_define("shb_volley", CH_NOISE, 220, "@36 v8 o6 c16");
    sfx_define("shb_bonk", CH_NOISE, 200, "@13 v12 o2 c8");
    sfx_define("shb_croak", CH_P2, 180, "@37 v10 o3 c16 o2 g16");
    sfx_define("shb_spark", CH_P2, 220, "@38 v9 o5 c16 g16");
    sfx_define("shb_roar", CH_NOISE, 160, "@34 v12 o4 c4");
    sfx_define("shb_chute", CH_NOISE, 220, "@13 v9 o3 c16");
    sfx_define("shb_unwind", CH_P1, 200, "@39 v11 o6 l32 c o5 g e c o4 g e c");
    sfx_define("shb_lid", CH_P1, 220, "@32 v11 o5 c16 g16");
}
