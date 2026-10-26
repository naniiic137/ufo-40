/* IRONVOW - original music (UFO-MML) and sound effects. Every looping tune
 * is eight bars a channel, so the channels loop together: the title, one
 * tune for each part of the keep, the throne, the bosses, the Regent, the
 * ending, and a short jingle when the vow breaks. */
#include "ivw.h"

int IVW_MUS_TITLE = -1, IVW_MUS_GAOL, IVW_MUS_WARREN, IVW_MUS_STAR, IVW_MUS_THRONE, IVW_MUS_BOSS, IVW_MUS_ENDING,
    IVW_MUS_OVER, IVW_MUS_REGENT;

/* a bar of four quarters; a bar of eighths on one note */
#define Q4(a, b, c, d) a "4 " b "4 " c "4 " d "4 "
#define E8(n) n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 " n "8 "
/* half a bar of a slow march; half a bar of a quick beat */
#define MARCH "@13 v11 o2 c4 @11 v8 o6 c8 @9 v4 o8 c8 "
#define QUICK "@13 v12 o2 c8 @9 v5 o8 c8 @11 v10 o6 c8 @9 v5 o8 c8 "
#define DRIVE "@13 v12 o2 c8 @9 v5 o8 c8 @13 v10 o2 c8 @11 v10 o6 c8 "

/* "Ironvow" - the title, D minor, stately */
static const char TITLE_LEAD[] =
    "@14 v11 q7 | o4 d4. e8 f4 a4 | g4 f4 e2 | f4. g8 a4 o5 d4 | c2 o4 a2"
    "| b-4. a8 g4 f4 | e4 f4 g2 | a4 g4 f4 e4 | d1";
static const char TITLE_ARP[] =
    "v6 q5 o3 @17 " E8("d") "@16 " E8("c") "@17 " E8("d") "@16 " E8("f") "@17 " E8("g") "@16 " E8("c") "@16 " E8("a")
    "@17 " E8("d");
static const char TITLE_BASS[] =
    "@6 v13 q6 " Q4("o2 d", "d", "o3 d", "o2 d") Q4("o2 c", "c", "o3 c", "o2 c") Q4("o2 d", "d", "o3 d", "o2 d")
    Q4("o2 f", "f", "o3 f", "o2 f") Q4("o2 g", "g", "o3 g", "o2 g") Q4("o2 c", "c", "o3 c", "o2 c")
    Q4("o2 a", "a", "o3 a", "o2 a") Q4("o2 d", "d", "o3 d", "o2 d");
static const char TITLE_DRUM[] = "[" MARCH MARCH "]8";

/* "The Gaol" - the first three floors, A minor, dripping */
static const char GAOL_LEAD[] =
    "@5 v10 q7 | o4 a2 e4 a4 | g2. e4 | f2 e4 d4 | e1 | a2 o5 c4 o4 b4 | a2. g4 | f4 e4 d4 c4 | o3 b2 o4 e2";
static const char GAOL_DRIP[] =
    "@15 v4 q4 o6 e8 r4. r2 | c8 r4. r2 | d8 r4. r2 | o5 b8 r4. r2 | o6 e8 r4. r2 | a8 r4. r2 | f8 r4. r2"
    "| o5 g+8 r4. r2";
static const char GAOL_BASS[] = "@6 v12 q8 o2 a1 e1 d1 e1 a1 a1 d1 e1";
static const char GAOL_SOFT[] = "[@9 v2 o8 c2]16";

/* "The Warren" - floors 3 and 4, E, uneasy */
static const char WARREN_LEAD[] =
    "@1 v10 q6 | o4 e8 f8 e4 r4 b4 | o5 c4 o4 b8 a8 b2 | e8 f8 e4 r4 o5 c4 | o4 b2. r4"
    "| e8 f8 g4 f4 e4 | d4 e8 f8 e2 | o5 c4 o4 b4 a4 f4 | e1";
static const char WARREN_TICK[] = "@20 v5 q4 o5 [e16 f16 r8 r4 r2]8";
static const char WARREN_BASS[] =
    "@6 v12 q5 " E8("o2 e") E8("o2 c") E8("o2 a") E8("o2 b") E8("o2 e") E8("o2 d") E8("o2 a") E8("o2 e");
static const char WARREN_DRUM[] = "[@13 v10 o2 c8 r8 @9 v4 o8 c8 r8 @11 v7 o6 c8 r8 @9 v4 o8 c8 @9 v3 o8 c8]8";

/* "The Starvault" - floors 5 and 6, F sharp minor, glittering */
static const char STAR_LEAD[] =
    "@14 v10 q7 | o5 c+4. o4 a8 f+4 a4 | b4 a4 g+2 | a4. b8 o5 c+4 e4 | d2 c+2"
    "| o4 b4. a8 g+4 e4 | f+4 g+4 a2 | b4 a4 g+4 e+4 | f+1";
static const char STAR_ARP[] =
    "v6 q5 o3 @17 " E8("f+") "@16 " E8("e") "@16 " E8("d") "@17 " E8("b") "@17 " E8("b") "@16 " E8("d")
    "@16 " E8("c+") "@17 " E8("f+");
static const char STAR_BASS[] =
    "@6 v12 q6 " Q4("o2 f+", "f+", "o3 f+", "o2 f+") Q4("o2 e", "e", "o3 e", "o2 e") Q4("o2 d", "d", "o3 d", "o2 d")
    Q4("o2 b", "b", "o3 b", "o2 b") Q4("o2 b", "b", "o3 b", "o2 b") Q4("o2 d", "d", "o3 d", "o2 d")
    Q4("o2 c+", "c+", "o3 c+", "o2 c+") Q4("o2 f+", "f+", "o3 f+", "o2 f+");
static const char STAR_DRUM[] = "[" QUICK MARCH "]8";

/* "The Empty Throne" - C, slow and grand */
static const char THRONE_LEAD[] =
    "@23 v11 q7 | o4 c2 e4 g4 | o5 c2. o4 b4 | a2 f4 a4 | g1 | f2 a4 o5 c4 | o4 b2 g4 b4 | o5 c4 o4 g4 e4 d4 | c1";
static const char THRONE_PAD[] = "@4 v5 q8 o3 e1 e1 f1 g1 a1 g1 g1 e1";
static const char THRONE_BASS[] = "@6 v12 q8 o2 c1 c1 f1 g1 f1 g1 c1 c1";
static const char THRONE_DRUM[] = "[@12 v4 o4 c1 @9 v2 o8 c1]4";

/* "Steel on Steel" - the Warden and the Matron, E minor, fast */
static const char BOSS_LEAD[] =
    "@1 v12 q6 | o5 e8 e8 e8 d8 e4 g4 | f+8 e8 d8 e8 o4 b4 r4 | o5 e8 e8 e8 d8 e4 a4 | g8 f+8 e8 f+8 d+2"
    "| e8 g8 b8 o6 e8 d4 c4 | o5 b8 a8 g8 a8 f+2 | g4 f+4 e4 d+4 | e2 r2";
static const char BOSS_STAB[] =
    "@3 v7 q4 o4 [r8 b8]4 [r8 a8]4 [r8 b8]4 [r8 a8]4 [r8 g8]4 [r8 f+8]4 [r8 g8]4 [r8 b8]4";
static const char BOSS_BASS[] =
    "@6 v13 q5 " E8("o2 e") E8("o2 d") E8("o2 e") E8("o2 b") E8("o2 c") E8("o2 d") E8("o2 a") E8("o2 b");
static const char BOSS_DRUM[] = "[" QUICK "]16";

/* "The Pit Regent" - the last fight and the climb out, C minor */
static const char REGENT_LEAD[] =
    "@14 v12 q7 | o4 c4 e-4 g4 o5 c4 | o4 b2 g2 | a-4 g4 f4 e-4 | d2 g2 | c4 e-4 g4 o5 e-4 | d4 c4 o4 b4 g4"
    "| a-4 f4 d4 b4 | o5 c1";
static const char REGENT_ARP[] =
    "v6 q5 o3 @17 " E8("c") "@16 " E8("g") "@16 " E8("a-") "@16 " E8("g") "@17 " E8("c") "@16 " E8("g")
    "@17 " E8("f") "@16 " E8("g");
static const char REGENT_BASS[] =
    "@6 v13 q6 " Q4("o2 c", "c", "o3 c", "o2 c") Q4("o2 g", "g", "o3 g", "o2 g") Q4("o2 a-", "a-", "o3 a-", "o2 a-")
    Q4("o2 g", "g", "o3 g", "o2 g") Q4("o2 c", "c", "o3 c", "o2 c") Q4("o2 g", "g", "o3 g", "o2 g")
    Q4("o2 f", "f", "o3 f", "o2 f") Q4("o2 g", "g", "o3 g", "o2 g");
static const char REGENT_DRUM[] = "[" DRIVE "]16";

/* "Daylight" - the ending, G */
static const char END_LEAD[] =
    "@5 v11 q7 | o4 g4 b4 o5 d4 g4 | f+4 e4 d2 | c4 e4 d4 o4 b4 | a2. r4 | g4 b4 o5 d4 e4 | d4 c4 o4 b4 a4"
    "| g4 a4 b4 a4 | g1";
static const char END_ARP[] =
    "v6 q5 o3 @16 " E8("g") "@16 " E8("d") "@16 " E8("c") "@16 " E8("d") "@16 " E8("g") "@16 " E8("c")
    "@16 " E8("d") "@16 " E8("g");
static const char END_BASS[] = "@6 v12 q8 o2 g1 d1 c1 d1 g1 c1 d1 g1";
static const char END_DRUM[] = "[@9 v3 o8 c4 @11 v5 o6 c4 @9 v3 o8 c4 @11 v5 o6 c4]8";

/* "The Vow Is Broken" - a short fall */
static const char OVER_P1[] = "@5 v12 q7 o4 a4 g4 f4 e4 d2. r4";
static const char OVER_TRI[] = "@6 v12 q8 o2 a2 f2 d1";

void ivw_audio_load(void) {
    if (IVW_MUS_TITLE >= 0) return;
    IVW_MUS_TITLE = song_define("ivw_title", 100, true, TITLE_LEAD, TITLE_ARP, TITLE_BASS, TITLE_DRUM);
    IVW_MUS_GAOL = song_define("ivw_gaol", 92, true, GAOL_LEAD, GAOL_DRIP, GAOL_BASS, GAOL_SOFT);
    IVW_MUS_WARREN = song_define("ivw_warren", 110, true, WARREN_LEAD, WARREN_TICK, WARREN_BASS, WARREN_DRUM);
    IVW_MUS_STAR = song_define("ivw_star", 120, true, STAR_LEAD, STAR_ARP, STAR_BASS, STAR_DRUM);
    IVW_MUS_THRONE = song_define("ivw_throne", 80, true, THRONE_LEAD, THRONE_PAD, THRONE_BASS, THRONE_DRUM);
    IVW_MUS_BOSS = song_define("ivw_boss", 160, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUM);
    IVW_MUS_REGENT = song_define("ivw_regent", 150, true, REGENT_LEAD, REGENT_ARP, REGENT_BASS, REGENT_DRUM);
    IVW_MUS_ENDING = song_define("ivw_ending", 96, true, END_LEAD, END_ARP, END_BASS, END_DRUM);
    IVW_MUS_OVER = song_define("ivw_over", 80, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("ivw_step", CH_NOISE, 240, "@13 v5 o3 c32");
    sfx_define("ivw_bump", CH_NOISE, 200, "@41 v8 o3 c16");
    sfx_define("ivw_hurt", CH_P1, 200, "@37 v12 o4 c16 o3 g16");
    sfx_define("ivw_lever", CH_P2, 200, "@42 v10 o4 c32 r32 o3 g16");
    sfx_define("ivw_rumble", CH_NOISE, 120, "@34 v10 o2 c4");
    sfx_define("ivw_chest", CH_P2, 200, "@35 v10 o5 c16 e16 g16");
    sfx_define("ivw_door", CH_NOISE, 200, "@36 v8 o4 c16 @41 v6 o3 c16");
    sfx_define("ivw_unlock", CH_P1, 200, "@39 v11 o5 l32 c g o6 c e g");
    sfx_define("ivw_douse", CH_NOISE, 200, "@10 v7 o7 c8");
    sfx_define("ivw_spore", CH_NOISE, 160, "@10 v9 o6 c8 @9 v6 o7 c8");
    sfx_define("ivw_lantern", CH_P2, 200, "@15 v9 o6 c16 g16");
    sfx_define("ivw_slime", CH_NOISE, 160, "@10 v9 o4 c8");
    sfx_define("ivw_drink", CH_P1, 200, "@38 v9 o4 c16 e16 g16");
    sfx_define("ivw_coin", CH_P2, 220, "@35 v11 o6 c16 g16");
    sfx_define("ivw_warp", CH_P1, 160, "@38 v12 o4 l32 c e g o5 c e g o6 c");
    sfx_define("ivw_gasp", CH_P1, 180, "@33 v10 o4 c8");
    sfx_define("ivw_ward", CH_P2, 220, "@15 v9 o6 c16 o7 c16");
    sfx_define("ivw_block", CH_NOISE, 240, "@21 v10 o5 c16");
    sfx_define("ivw_clink", CH_P2, 240, "@42 v9 o6 c32 r32 o6 g32");
    sfx_define("ivw_hit", CH_NOISE, 240, "@13 v12 o3 c16");
    sfx_define("ivw_swish", CH_NOISE, 240, "@36 v6 o6 c16");
    sfx_define("ivw_kill", CH_NOISE, 160, "@34 v11 o3 c8");
    sfx_define("ivw_fight", CH_P1, 180, "@1 v12 o4 l16 e g b o5 e");
    sfx_define("ivw_win", CH_P1, 160, "@39 v12 o5 l16 c e g o6 c4");
    sfx_define("ivw_roll", CH_NOISE, 240, "@36 v8 o5 c16 c16");
    sfx_define("ivw_thunder", CH_NOISE, 100, "@40 v14 o2 c2");
    sfx_define("ivw_cast", CH_P2, 200, "@38 v8 o4 c16 g16");
    sfx_define("ivw_stroke", CH_P2, 240, "@20 v6 o6 c32");
    sfx_define("ivw_fizzle", CH_NOISE, 200, "@10 v8 o5 c8");
    sfx_define("ivw_spell", CH_P2, 200, "@39 v11 o5 l32 c e g b o6 d");
    sfx_define("ivw_kindle", CH_NOISE, 200, "@10 v9 o6 c8");
    sfx_define("ivw_equip", CH_NOISE, 240, "@21 v8 o4 c16");
    sfx_define("ivw_gem", CH_P2, 240, "@35 v9 o6 e16");
    sfx_define("ivw_point", CH_P2, 220, "@39 v10 o6 c16 e16");
    sfx_define("ivw_stairs", CH_NOISE, 160, "@13 v8 o3 c16 r16 c16 r16 c16");
}
