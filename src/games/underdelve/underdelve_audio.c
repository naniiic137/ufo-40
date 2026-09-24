/* UNDERDELVE - original music (UFO-MML) and sound effects. */
#include "underdelve.h"

int UD_MUS_MINE = -1, UD_MUS_DEEP, UD_MUS_BOSS, UD_MUS_WIN, UD_MUS_TITLE, UD_MUS_OVER;

/* "Lamplight Descent" - D minor, the theme of the upper mine.
 * A: Dm Bb F C | Dm Bb Gm A    B: Bb C Am Dm | Bb C A A */
static const char MINE_LEAD[] =
    "@1 v12 q7 l8"
    "| o4 d4 f a ^4 g f | f4. e d4 c4 | c4 f a ^4 g a | g2. r4"
    "| o4 d4 f a o5 d4 c o4 a | b-4. a g4 f4 | g a b- o5 d ^4 c o4 b- | a2 c+4 e4"
    "| o5 d4. c o4 b-4 a4 | g4. f e4 g4 | a4. g e4 c4 | d2. r4"
    "| o4 f g a b- o5 d4 f4 | e4. d c4 e4 | c+2 o4 a4 e4 | a2 r4 a o5 c+";
static const char MINE_ARP[] =
    "@3 v7 q5 l8"
    "| o3 d f a o4 d o3 a f d f | o2 b- o3 d f b- f d o2 b- o3 d | o3 f a o4 c f c o3 a f a | o3 c e g o4 c o3 g e c e"
    "| o3 d f a o4 d o3 a f d f | o2 b- o3 d f b- f d o2 b- o3 d | o2 g b- o3 d g d o2 b- g b- | o2 a o3 c+ e a e c+ o2 a o3 c+"
    "| o2 b- o3 d f b- f d o2 b- o3 d | o3 c e g o4 c o3 g e c e | o2 a o3 c e a e c o2 a o3 c | o3 d f a o4 d o3 a f d f"
    "| o2 b- o3 d f b- f d o2 b- o3 d | o3 c e g o4 c o3 g e c e | o2 a o3 c+ e a e c+ o2 a o3 c+ | o2 a o3 c+ e a e c+ o2 a o3 c+";
static const char MINE_BASS[] =
    "@6 v15 q6"
    "| o2 d4 d8 o3 d8 o2 d4 a4 | o2 b-4 b-8 o3 b-8 o2 b-4 o3 f4 | o2 f4 f8 o3 f8 o2 f4 o3 c4 | o2 c4 c8 o3 c8 o2 c4 g4"
    "| o2 d4 d8 o3 d8 o2 d4 a4 | o2 b-4 b-8 o3 b-8 o2 b-4 o3 f4 | o2 g4 g8 o3 g8 o2 g4 o3 d4 | o2 a4 a8 o3 a8 o2 a4 o3 e4"
    "| o2 b-4 b-8 o3 b-8 o2 b-4 o3 f4 | o2 c4 c8 o3 c8 o2 c4 g4 | o2 a4 a8 o3 a8 o2 a4 o3 e4 | o2 d4 d8 o3 d8 o2 d4 a4"
    "| o2 b-4 b-8 o3 b-8 o2 b-4 o3 f4 | o2 c4 c8 o3 c8 o2 c4 g4 | o2 a4 a8 o3 a8 o2 a4 o3 e4 | o2 a4 a8 o3 a8 o2 a4 o3 e4";
#define MINE_DR "o2@13v11c8 o8@9v5c8 o8@9v3c8 o8@9v5c8 o6@11v9c8 o8@9v5c8 o8@9v3c8 o8@9v5c8 "
#define MINE_DF "o2@13v11c8 o8@9v5c8 o8@9v3c8 o8@9v5c8 o6@11v9c8 o6@11v7c16c16 o6@11v10c8 o8@10v7c8 "
static const char MINE_DRUMS[] = "[" MINE_DR MINE_DR MINE_DR MINE_DF "]4";

/* "Ember Deep" - E phrygian, tense and pulsing. Em F Em F | Em G F E */
static const char DEEP_LEAD[] =
    "@5 v12 q7 l8"
    "| o4 e4. f e4 o3 b4 | o5 c4. o4 b a4 f4 | o4 e4. f g4 b4 | o4 a4. g f4 c4"
    "| o4 b4. o5 c o4 b4 g4 | o5 d4. e d4 o4 b4 | o5 c2 o4 a4 f4 | o4 g+2. r4";
static const char DEEP_ARP[] =
    "@2 v6 q4 l16"
    "| [o4 e g b g]4 | [o4 f a o5 c o4 a]4 | [o4 e g b g]4 | [o4 f a o5 c o4 a]4"
    "| [o4 e g b g]4 | [o4 d g b g]4 | [o4 f a o5 c o4 a]4 | [o4 e g+ b g+]4";
static const char DEEP_BASS[] =
    "@6 v15 q5 l8"
    "| [o2 e]8 | [o2 f]8 | [o2 e]8 | [o2 f]8 | [o2 e]8 | [o2 g]8 | [o2 f]8 | [o2 e]8";
#define DEEP_DR "o2@13v12c8 o8@9v5c16c16 o6@11v10c8 o8@9v5c16c16 o2@13v12c8 o8@9v5c16c16 o6@11v10c8 o8@9v5c16c16 "
static const char DEEP_DRUMS[] = "[" DEEP_DR "]8";

/* "The Old Lode" - boss battle, A minor, 156 bpm. Am Am F G | Am Am F E */
static const char BOSS_LEAD[] =
    "@1 v13 q6 l16"
    "| o5 a8 a g a8 e8 g8 a8 o6 c8 o5 b8 | o5 a8 a g a8 e8 d8 e8 c8 o4 b8"
    "| o5 c8 c o4 b o5 c8 f8 a8 o6 c8 o5 a8 f8 | o5 g8 g f g8 d8 b8 o6 d8 o5 b8 g8"
    "| o5 a8 a g a8 e8 g8 a8 o6 c8 e8 | o6 d8 c o5 b o6 c8 o5 a8 e8 a8 o6 c8 o5 a8"
    "| o5 f8 a o6 c f8 e8 d8 c8 o5 a8 f8 | o5 e8 g+ b o6 e8 d8 o5 b8 g+8 e8 d8";
static const char BOSS_STAB[] =
    "@22 v8 q3 l8"
    "| [o4 a r a a r a r a]2 | o4 f r f f r f r f | o4 g r g g r g r g"
    "| [o4 a r a a r a r a]2 | o4 f r f f r f r f | o4 e r e e r e r e";
static const char BOSS_BASS[] =
    "@6 v15 q5 l8"
    "| [o2 a o3 a]4 | [o2 a o3 a]4 | [o2 f o3 f]4 | [o2 g o3 g]4"
    "| [o2 a o3 a]4 | [o2 a o3 a]4 | [o2 f o3 f]4 | [o2 e o3 e]4";
#define BOSS_DR "o2@13v13c8 o8@9v6c8 o6@11v11c8 o8@9v6c8 o2@13v13c8 o2@13v11c8 o6@11v11c8 o8@9v6c8 "
#define BOSS_DF "o2@13v13c8 o8@9v6c8 o6@11v11c8 o8@9v6c8 o6@11v10c16c16 o6@11v12c16c16 o6@11v13c8 o8@10v9c8 "
static const char BOSS_DRUMS[] = "[" BOSS_DR BOSS_DR BOSS_DR BOSS_DF "]2";

/* "Sunstone" - victory theme, C major. C F G C | Am F G C */
static const char WIN_LEAD[] =
    "@14 v12 q7 l8"
    "| o5 c4 e g ^4 e g | o5 a4 o6 c o5 a ^4 f4 | o5 g4 b o6 d ^4 o5 b o6 d | o6 c2. r4"
    "| o5 e4 a o6 c ^4 o5 b a | o5 f4 a o6 c ^4 o5 a4 | o5 g4 f e d4 g4 | o5 c2. r4";
static const char WIN_HARM[] =
    "@22 v7 q6 l8"
    "| o4 e4 g o5 c ^4 o4 g o5 c | o4 f4 a o5 c ^4 o4 a4 | o4 g4 b o5 d ^4 o4 b o5 d | o4 e2. r4"
    "| o4 c4 e a ^4 g e | o4 c4 f a ^4 f4 | o4 b4 a g f4 b4 | o4 e2. r4";
static const char WIN_BASS[] =
    "@6 v14 q7 l4"
    "| o2 c g o3 c o2 g | o2 f o3 c f c | o2 g o3 d g d | o2 c g o3 c o2 g"
    "| o2 a o3 e a e | o2 f o3 c f c | o2 g o3 d o2 g b | o3 c o2 g o3 c r";
static const char WIN_DRUMS[] =
    "[o2@13v10c4 o6@11v8c4 o2@13v10c8 o2@13v8c8 o6@11v8c4]7 o6@11v10c8 c8 c8 c8 @12 v10 c2";

/* short jingles */
static const char OVER_P1[] = "@5 v12 q7 o4 l4 e d c o3 b a2.";
static const char OVER_TRI[] = "@6 v14 q7 o2 l4 a g f e a2.";

void ud_audio_load(void) {
    if (UD_MUS_MINE >= 0) return;
    UD_MUS_MINE = song_define("ud_mine", 116, true, MINE_LEAD, MINE_ARP, MINE_BASS, MINE_DRUMS);
    UD_MUS_DEEP = song_define("ud_deep", 132, true, DEEP_LEAD, DEEP_ARP, DEEP_BASS, DEEP_DRUMS);
    UD_MUS_BOSS = song_define("ud_boss", 156, true, BOSS_LEAD, BOSS_STAB, BOSS_BASS, BOSS_DRUMS);
    UD_MUS_WIN = song_define("ud_win", 120, true, WIN_LEAD, WIN_HARM, WIN_BASS, WIN_DRUMS);
    UD_MUS_TITLE = UD_MUS_MINE;
    UD_MUS_OVER = song_define("ud_over", 90, false, OVER_P1, "", OVER_TRI, "");

    sfx_define("ud_jump", CH_P2, 240, "@32 v10 o4 c16");
    sfx_define("ud_swing", CH_NOISE, 200, "@36 v8 o7 c16");
    sfx_define("ud_kill", CH_NOISE, 200, "@21 v12 o6 c16 @13 v12 o3 c8");
    sfx_define("ud_clink", CH_P2, 220, "@15 v11 o6 e32 r32 e32");
    sfx_define("ud_coin", CH_P2, 220, "@35 v11 o6 e16 b8");
    sfx_define("ud_gem", CH_P2, 220, "@39 v11 o6 c16 e16 g16 o7 c8");
    sfx_define("ud_die", CH_P1, 160, "@33 v13 o5 c8 o4 g8 e8 c4");
    sfx_define("ud_item", CH_P1, 150, "@16 v12 o5 c8 e8 g8 o6 c4.");
    sfx_define("ud_buy", CH_P2, 220, "@35 v11 o5 g16 o6 c16 e8");
    sfx_define("ud_nope", CH_P2, 200, "@37 v11 o3 c16 r16 c8");
    sfx_define("ud_drip", CH_NOISE, 240, "@21 v8 o7 c32");
    sfx_define("ud_clang", CH_P2, 240, "@15 v10 o6 a32 e32");
    sfx_define("ud_shatter", CH_NOISE, 200, "@40 v12 o6 c8");
    sfx_define("ud_gate", CH_NOISE, 120, "@34 v13 o3 c4");
    sfx_define("ud_bosshit", CH_P2, 220, "@37 v12 o4 c16 o3 c16");
    sfx_define("ud_slam", CH_NOISE, 160, "@34 v14 o2 c4");
    sfx_define("ud_boom", CH_NOISE, 120, "@34 v15 o2 c2");
    sfx_define("ud_gloom", CH_P2, 90, "@4 v10 o3 c+4 c4");
    sfx_define("ud_spit", CH_NOISE, 220, "@36 v9 o5 c16");
    sfx_define("ud_bolt", CH_P2, 240, "@32 v10 o6 c16");
    sfx_define("ud_peck", CH_P2, 240, "@20 v10 o7 c32 e32");
    sfx_define("ud_step", CH_NOISE, 240, "@21 v3 o5 c32");
    sfx_define("ud_land", CH_NOISE, 240, "@13 v7 o3 c32");
    sfx_define("ud_door", CH_NOISE, 160, "@41 v12 o3 c8");
}
