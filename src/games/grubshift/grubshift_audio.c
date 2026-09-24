/* GRUB SHIFT - original music (UFO-MML) and sound effects. */
#include "grubshift.h"

int GS_MUS_SHIFT = -1, GS_MUS_TITLE, GS_MUS_WIN, GS_MUS_LOSE, GS_MUS_NIGHT;

/* "Night Shift" - an E dorian funk groove.
 * Em7 A7 Em7 A7 | Cmaj7 Bm7 Am7 B7  (x2) */
static const char SHIFT_LEAD[] =
    "@0 v12 q6"
    "| o4 b8 o5 d8 e8 r8 d16 e16 r16 d16 o4 b8 a8 | o4 g8 a8 r8 o5 c+16 d16 e4 r4"
    "| o5 g8 f+8 e8 d8 e8 r8 o4 b8 r8 | o5 c+8 d8 e8 g8 f+4 e4"
    "| o5 e4. d8 c8 o4 b8 g8 r8 | o4 f+8 a8 b8 o5 d8 ^4 c+8 o4 b8"
    "| o4 a8 o5 c8 e8 g8 ^8 f+8 e8 d8 | o5 d+4 f+4 b4 r4"
    "| o4 b8 o5 d8 e8 r8 d16 e16 r16 d16 o4 b8 a8 | o4 g8 a8 r8 o5 c+16 d16 e4 r4"
    "| o5 g8 f+8 e8 d8 e8 r8 o4 b8 r8 | o5 c+8 d8 e8 g8 f+4 e4"
    "| o5 e4. d8 c8 o4 b8 g8 r8 | o4 f+8 a8 b8 o5 d8 ^4 c+8 o4 b8"
    "| o4 a8 o5 c8 e8 g8 ^8 f+8 e8 d8 | o5 f+8 a8 b8 o6 d+8 ^2";
#define STAB(ins, oct, note) "r8 " ins " " oct " " note "16 r16 r8 " note "8 r16 " note "16 r8 " note "8 r8 "
static const char SHIFT_COMP[] =
    "v7 q4 [" STAB("@17", "o4", "e") STAB("@16", "o4", "a") STAB("@17", "o4", "e") STAB("@16", "o4", "a")
    STAB("@16", "o4", "c") STAB("@17", "o4", "b") STAB("@17", "o4", "a") STAB("@16", "o4", "b") "]2";
static const char SHIFT_BASS[] =
    "@6 v15 q5 ["
    "o2 e8 r16 e16 o3 e8 o2 r16 e16 d8 e8 g8 a8 "
    "o2 a8 r16 a16 o3 a8 o2 r16 a16 g8 a8 o3 c+8 o2 b8 "
    "o2 e8 r16 e16 o3 e8 o2 r16 e16 d8 e8 g8 a8 "
    "o2 a8 r16 a16 o3 a8 o2 r16 a16 g8 a8 o3 c+8 o2 b8 "
    "o2 c8 r16 c16 o3 c8 o2 r16 c16 o1 b8 o2 c8 e8 g8 "
    "o2 b8 r16 b16 o3 b8 o2 r16 b16 a8 b8 o3 d8 f+8 "
    "o2 a8 r16 a16 o3 a8 o2 r16 a16 g8 a8 o3 c8 e8 "
    "o2 b8 r16 b16 o3 b8 o2 r16 b16 o3 d+8 f+8 a8 f+8 ]2";
#define FUNK "o2@13v12c16 o8@9v4c16 o8@9v6c16 o8@9v4c16 o6@11v11c16 o8@9v4c16 o2@13v10c16 o8@9v4c16 " \
             "o8@9v6c16 o8@9v4c16 o2@13v11c16 o8@9v4c16 o6@11v11c16 o8@9v4c16 o8@9v6c16 o8@10v6c16 "
#define FUNKF "o2@13v12c16 o8@9v4c16 o8@9v6c16 o8@9v4c16 o6@11v11c16 o8@9v4c16 o2@13v10c16 o8@9v4c16 " \
              "o6@11v8c16 o6@11v9c16 o6@11v10c16 o6@11v11c16 o6@11v12c16 o6@11v12c16 o8@10v8c8 "
static const char SHIFT_DRUMS[] = "[" FUNK FUNK FUNK FUNK FUNK FUNK FUNK FUNKF "]2";

/* "Briefing" - a calm title tune over the dome. G major. G D/F# Em C */
static const char BRIEF_LEAD[] =
    "@15 v11 q7 l8"
    "| o5 d4 g4 f+4 d4 | o5 e4. d c4 o4 b4 | o4 b4 o5 e4 d4 o4 b4 | o5 c4. o4 b a2"
    "| o5 d4 g4 f+4 d4 | o5 e4. f+ g4 a4 | o5 b4 a4 g4 e4 | o5 d2 r2";
static const char BRIEF_ARP[] =
    "@2 v6 q5 l8"
    "| o4 g b o5 d o4 b g b o5 d o4 b | o4 f+ a o5 d o4 a f+ a o5 d o4 a | o4 e g b g e g b g | o4 c e g e c e g e"
    "| o4 g b o5 d o4 b g b o5 d o4 b | o4 f+ a o5 d o4 a f+ a o5 d o4 a | o4 e g b g e g b g | o4 d f+ a f+ d f+ a f+";
static const char BRIEF_BASS[] =
    "@6 v13 q7 l2"
    "| o2 g g | o2 f+ f+ | o2 e e | o2 c c | o2 g g | o2 f+ f+ | o2 e e | o2 d d";

/* jingles */
static const char NIGHT_P1[] = "@15 v11 o6 l8 e b o7 e d+ o6 b4 g+4 e2";
static const char NIGHT_TRI[] = "@7 v13 o3 l2 e e e";
static const char WIN_P1[] = "@16 v12 o5 l8 e g+ b o6 e4. d+8 e8 f+8 g+2";
static const char WIN_P2[] = "@22 v8 o5 l8 b o6 e g+ b4. a8 b8 o7 c+8 d+2";
static const char WIN_TRI[] = "@6 v14 o2 l4 e b o3 e o2 b e2";
static const char LOSE_P1[] = "@5 v11 o4 l8 e d c o3 b4. a8 g+2";
static const char LOSE_TRI[] = "@6 v13 o2 l4 e d c o1 b2";

void gs_audio_load(void) {
    if (GS_MUS_SHIFT >= 0) return;
    GS_MUS_SHIFT = song_define("gs_shift", 100, true, SHIFT_LEAD, SHIFT_COMP, SHIFT_BASS, SHIFT_DRUMS);
    GS_MUS_TITLE = song_define("gs_brief", 84, true, BRIEF_LEAD, BRIEF_ARP, BRIEF_BASS, "");
    GS_MUS_NIGHT = song_define("gs_night", 120, false, NIGHT_P1, "", NIGHT_TRI, "");
    GS_MUS_WIN = song_define("gs_win", 140, false, WIN_P1, WIN_P2, WIN_TRI, "@12 v8 o6 c2");
    GS_MUS_LOSE = song_define("gs_lose", 90, false, LOSE_P1, "", LOSE_TRI, "");

    sfx_define("gs_move", CH_P2, 240, "@42 v8 o5 c32 e32");
    sfx_define("gs_hop", CH_P2, 240, "@32 v10 o5 c16");
    sfx_define("gs_zap", CH_P2, 240, "@33 v11 o7 c16");
    sfx_define("gs_toss", CH_TRI, 200, "@38 v13 o4 c16");
    sfx_define("gs_boom", CH_NOISE, 140, "@34 v15 o3 c4");
    sfx_define("gs_kill", CH_NOISE, 220, "@21 v11 o6 c16 @13 v11 o3 c16");
    sfx_define("gs_hit", CH_P2, 240, "@15 v11 o6 c32 r32 c32");
    sfx_define("gs_pod", CH_P2, 220, "@35 v11 o6 e16 g16");
    sfx_define("gs_buy", CH_P2, 220, "@39 v11 o5 c16 e16 g16 o6 c16");
    sfx_define("gs_egg", CH_NOISE, 220, "@21 v12 o5 c8");
    sfx_define("gs_spark", CH_P2, 240, "@20 v10 o7 c32 g32");
    sfx_define("gs_hole", CH_NOISE, 160, "@41 v12 o3 c8");
    sfx_define("gs_raise", CH_TRI, 200, "@38 v12 o3 c8");
    sfx_define("gs_die", CH_P1, 160, "@33 v13 o5 c8 o4 g8 e8 c4");
    sfx_define("gs_hatch", CH_P1, 140, "@37 v13 o3 c8 c+8 d4");
    sfx_define("gs_nope", CH_P2, 200, "@37 v10 o3 c16");
}
