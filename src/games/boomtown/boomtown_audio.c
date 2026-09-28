/* BOOMTOWN - original music (UFO-MML) and sound effects. */
#include "boomtown.h"

int BM_MUS_TITLE = -1, BM_MUS_NIGHT, BM_MUS_KING, BM_MUS_HOLD, BM_MUS_OVER, BM_MUS_END;

/* one bar of off-beat chord stabs */
#define OFF4(ins, n) ins " r8 " n "8 r8 " n "8 r8 " n "8 r8 " n "8 "
#define OFF2(ins, n) ins " r8 " n "8 r8 " n "8 "
/* one bar of a broken chord in eighths */
#define ARP(a, b, c) a "8 " b "8 " c "8 " b "8 " a "8 " b "8 " c "8 " b "8 "

/* "Lantern Lane" - the title. G major, 8 bars.
 * G | Em | C | D | G | Em | Am D | G */
static const char LANE_LEAD[] =
    "@1 v11 q6"
    "| o5 g4 b8 o6 d8 o5 g4 b4 | o5 e4 g8 b8 e4 g4 | o5 c8 e8 g8 o6 c8 o5 b4 g4 | o5 a4 f+8 a8 o6 d2"
    "| o6 d8 c8 o5 b8 a8 g4 d4 | o5 e8 f+8 g8 a8 b4 g4 | o5 a8 b8 o6 c8 o5 a8 f+4 a4 | o5 g2. r4";
static const char LANE_CHORDS[] =
    "v6 q3 "
    OFF4("@16", "o4g") OFF4("@17", "o4e") OFF4("@16", "o4c") OFF4("@16", "o4d")
    OFF4("@16", "o4g") OFF4("@17", "o4e") OFF2("@17", "o4a") OFF2("@16", "o4d") OFF4("@16", "o4g");
static const char LANE_BASS[] =
    "@6 v14 q5"
    "| o2 g4 o3 d4 o2 g4 o3 d4 | o2 e4 b4 e4 b4 | o2 c4 g4 c4 g4 | o2 d4 a4 d4 a4"
    "| o2 g4 o3 d4 o2 g4 o3 d4 | o2 e4 b4 e4 b4 | o2 a4 o3 e4 o2 d4 a4 | o2 g4 o3 d4 o2 g2";
static const char LANE_DRUMS[] = "[@13 v11 o2 c8 @9 v5 o8 c8 @11 v9 o6 c8 @9 v5 o8 c8]16";

/* "Midnight Market" - thinking music while the fireworks go down.
 * A minor, 8 bars: Am | F | C | G | Am | F | Dm E | Am */
static const char NIGHT_LEAD[] =
    "@5 v10 q7"
    "| o5 a4. e8 a4 b4 | o6 c4. o5 b8 a4 f4 | o5 g4. e8 g4 o6 c4 | o5 b2 r4 g4"
    "| o5 a4. o6 c8 e4 d4 | o6 c4. o5 a8 f4 a4 | o5 d4 f4 e4 g+4 | o5 a2. r4";
static const char NIGHT_ARP[] =
    "@2 v5 q5 "
    ARP("o4a", "o5c", "o5e") ARP("o4f", "o4a", "o5c") ARP("o4g", "o5c", "o5e") ARP("o4g", "o4b", "o5d")
    ARP("o4a", "o5c", "o5e") ARP("o4f", "o4a", "o5c")
    "o4d8 o4f8 o4a8 o4f8 o4e8 o4g+8 o4b8 o4g+8 "
    ARP("o4a", "o5c", "o5e");
static const char NIGHT_BASS[] =
    "@6 v13 q7 | o2 a1 | o2 f1 | o3 c1 | o2 g1 | o2 a1 | o2 f1 | o2 d2 e2 | o2 a1";
static const char NIGHT_TICK[] = "[@9 v3 o8 c4 @9 v2 o8 c4]16";

/* "The Bog King" - the last night. D minor, 4 bars, driving.
 * Dm | Bb | C | A */
static const char KING_LEAD[] =
    "@1 v11 q5"
    "| o5 d8 d8 f8 d8 a8 d8 g8 f8 | o5 f8 f8 b-8 f8 o6 d8 o5 f8 b-8 a8"
    "| o5 e8 e8 g8 e8 o6 c8 o5 e8 g8 e8 | o5 c+8 e8 a8 c+8 e4 a4";
static const char KING_STABS[] =
    "v7 q3"
    "| @17 o4 d8 r8 d8 r8 d8 d8 r8 d8 | @16 o3 b-8 r8 b-8 r8 b-8 b-8 r8 b-8"
    "| @16 o4 c8 r8 c8 r8 c8 c8 r8 c8 | @16 o3 a8 r8 a8 r8 a8 a8 r8 a8";
static const char KING_BASS[] =
    "@6 v15 q5"
    "| o2 d8 d8 o3 d8 o2 d8 d8 d8 o3 d8 o2 d8 | o1 b-8 b-8 o2 b-8 o1 b-8 b-8 b-8 o2 b-8 o1 b-8"
    "| o2 c8 c8 o3 c8 o2 c8 c8 c8 o3 c8 o2 c8 | o1 a8 a8 o2 a8 o1 a8 a8 a8 o2 a8 o1 a8";
static const char KING_DRUMS[] = "[@13 v13 o2 c8 @9 v6 o8 c8 @11 v11 o6 c8 @9 v6 o8 c8]8";

/* "Sky Full of Sparks" - the ending. C major, 8 bars.
 * C | Am | F | G | C | Am | F G | C */
static const char END_LEAD[] =
    "@23 v12 q7"
    "| o5 e4 g4 o6 c4. o5 b8 | o5 a4 o6 c4 e4 d4 | o6 c4 o5 a4 f4 a4 | o5 g2. r4"
    "| o5 e4 g4 o6 c4 e4 | o6 e4. d8 c4 o5 a4 | o5 a4 o6 c4 o5 b4 o6 d4 | o6 c2. r4";
static const char END_STRUM[] =
    "v7 q4 "
    OFF4("@16", "o4c") OFF4("@17", "o4a") OFF4("@16", "o4f") OFF4("@16", "o4g")
    OFF4("@16", "o4c") OFF4("@17", "o4a") OFF2("@16", "o4f") OFF2("@16", "o4g") OFF4("@16", "o4c");
static const char END_BASS[] =
    "@6 v14 q6"
    "| o2 c4 g4 c4 g4 | o2 a4 o3 e4 o2 a4 o3 e4 | o2 f4 o3 c4 o2 f4 o3 c4 | o2 g4 o3 d4 o2 g4 o3 d4"
    "| o2 c4 g4 c4 g4 | o2 a4 o3 e4 o2 a4 o3 e4 | o2 f4 o3 c4 o2 g4 o3 d4 | o2 c4 g4 c2";
static const char END_DRUMS[] = "[@13 v10 o2 c4 @11 v8 o6 c4]16";

/* the town holds for another night */
static const char HOLD_P1[] = "@39 v12 o5 l16 g b o6 d g8. r16 @14 o6 d8 g4";
static const char HOLD_P2[] = "@2 v8 o5 l16 d g b o6 d8. r16 o5 b8 o6 d4";
static const char HOLD_TRI[] = "@6 v12 o3 g8 d8 g8 r8 g4";
/* overrun */
static const char OVER_P1[] = "@5 v11 o5 l8 e d c o4 a4. g+8 a2";
static const char OVER_TRI[] = "@6 v13 o2 a4 e4 a2";

void bm_audio_load(void) {
    if (BM_MUS_TITLE >= 0) return;
    BM_MUS_TITLE = song_define("bm_title", 132, true, LANE_LEAD, LANE_CHORDS, LANE_BASS, LANE_DRUMS);
    BM_MUS_NIGHT = song_define("bm_night", 92, true, NIGHT_LEAD, NIGHT_ARP, NIGHT_BASS, NIGHT_TICK);
    BM_MUS_KING = song_define("bm_king", 150, true, KING_LEAD, KING_STABS, KING_BASS, KING_DRUMS);
    BM_MUS_END = song_define("bm_end", 120, true, END_LEAD, END_STRUM, END_BASS, END_DRUMS);
    BM_MUS_HOLD = song_define("bm_hold", 150, false, HOLD_P1, HOLD_P2, HOLD_TRI, "");
    BM_MUS_OVER = song_define("bm_over", 110, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("bm_move", CH_P2, 200, "@42 v8 o6 c32");
    sfx_define("bm_pick", CH_P2, 200, "@20 v10 o5 e32 g32");
    sfx_define("bm_turn", CH_P2, 220, "@20 v9 o6 c32 e32");
    sfx_define("bm_place", CH_NOISE, 200, "@21 v10 o5 c16 @13 v8 o3 c16");
    sfx_define("bm_cancel", CH_P2, 200, "@33 v9 o5 c16");
    sfx_define("bm_nope", CH_P2, 180, "@37 v10 o3 c16 r32 c16");
    sfx_define("bm_fuse", CH_NOISE, 240, "@21 v8 o7 c32 c32 c32 c32 c32 c32");
    sfx_define("bm_bang", CH_NOISE, 160, "@34 v14 o4 c8");
    sfx_define("bm_pop", CH_P2, 220, "@33 v11 o6 c16");
    sfx_define("bm_hurt", CH_P2, 220, "@37 v10 o5 c16");
    sfx_define("bm_folk", CH_P1, 200, "@33 v10 o5 e16 c16 o4 a8");
    sfx_define("bm_whoosh", CH_P1, 200, "@32 v11 o4 c8 c8");
    sfx_define("bm_king_hit", CH_P1, 200, "@37 v12 o3 c16 o2 g16");
    sfx_define("bm_spawn", CH_P2, 220, "@33 v8 o4 g32 e32");
    sfx_define("bm_hole", CH_NOISE, 180, "@36 v10 o4 c8");
    sfx_define("bm_heal", CH_P2, 220, "@39 v9 o6 c32 e32 g32");
    sfx_define("bm_newfolk", CH_P1, 200, "@35 v11 o6 c16 e16 g8");
    sfx_define("bm_secret", CH_P1, 160, "@39 v11 o5 l16 c e g o6 c e g o7 c4");
}
