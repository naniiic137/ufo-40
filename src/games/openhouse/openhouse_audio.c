/* OPEN HOUSE - original music (UFO-MML) and sound effects. */
#include "openhouse.h"

int PH_MUS_TITLE = -1, PH_MUS_PARTY, PH_MUS_SHOP, PH_MUS_WIN, PH_MUS_LOSE, PH_MUS_BUST, PH_MUS_NIGHT;

/* "Lanterns on the Terrace" - the title and menus. F major, a slow sway.
 * F | Dm | Gm | C | F | Bb | C | F */
static const char T_LEAD[] =
    "@14 v11 q7 ["
    "| o5 c4. a8 g4 f4 | o5 a4. f8 d4 c4 | o5 d4 f8 a8 g4 f4 | o5 e2. r4"
    "| o5 c4. a8 o6 c4 o5 a4 | o5 b-4. a8 g4 f4 | o5 g4 a8 b-8 a4 g4 | o5 f2. r4 ]2";
static const char T_COMP[] =
    "v6 q4 l8 ["
    "@16 o4 r f r f r f r f | @17 o4 r d r d r d r d | @17 o4 r g r g r g r g | @16 o4 r c r c r c r c"
    "| @16 o4 r f r f r f r f | @16 o3 r b- r b- r b- r b- | @16 o4 r c r c r c r c | @16 o4 r f r f r f r f ]2";
static const char T_BASS[] =
    "@6 v13 q6 [o2 f4 r8 c8 f4 a4 | o2 d4 r8 a8 d4 f4 | o2 g4 r8 d8 g4 b-4 | o2 c4 r8 g8 c4 e4"
    "| o2 f4 r8 c8 f4 a4 | o2 b-4 r8 f8 b-4 o3 d4 | o2 c4 r8 g8 c4 e4 | o2 f4 r8 c8 f4 r4 ]2";
#define SWAY "o8@9v4c8 o8@9v3c8 o6@11v6c8 o8@9v3c8 o8@9v4c8 o8@9v3c8 o6@11v6c8 o8@9v5c8 "
static const char T_DRUMS[] = "[" SWAY "]16";

/* "House Groove" - the party. A minor, four on the floor.
 * Am | Am | F | G | Am | Am | Dm | E */
static const char P_LEAD[] =
    "@1 v10 q6 ["
    "| o5 e8 e8 r8 a8 r8 g8 e8 d8 | o5 e4 r8 c8 d8 e8 r4 | o5 f8 f8 r8 a8 r8 g8 f8 e8 | o5 d4 r8 b8 o6 c8 d8 r4"
    "| o6 e8 e8 r8 d8 c8 o5 b8 a8 g8 | o5 a4 r8 e8 g8 a8 r4 | o5 f8 a8 o6 d8 c8 o5 a8 f8 d8 f8 | o5 e2 g+4 b4 ]2";
static const char P_ARP[] =
    "@18 v5 q4 l16 ["
    "[o4 a o5 c e c]4 | [o4 a o5 c e c]4 | [o4 f a o5 c o4 a]4 | [o4 g b o5 d o4 b]4"
    "| [o4 a o5 c e c]4 | [o4 a o5 c e c]4 | [o4 d f a f]4 | [o4 e g+ b g+]4 ]2";
static const char P_BASS[] =
    "@7 v14 q5 l8 [[o2 a o3 a]4 | [o2 a o3 a]4 | [o2 f o3 f]4 | [o2 g o3 g]4 | [o2 a o3 a]4 | [o2 a o3 a]4 | [o2 d o3 d]4 | [o2 e o3 e]4 ]2";
#define FLOOR "o2@13v12c8 o8@9v5c8 o6@11v10c8 o8@9v5c8 o2@13v12c8 o8@10v6c8 o6@11v10c8 o8@9v5c16 o8@9v4c16 "
static const char P_DRUMS[] = "[" FLOOR "]16";

/* "Market Street" - the shop. C major, bouncy. C | Am | F | G (twice) */
static const char S_LEAD[] =
    "@3 v11 q5 l8 ["
    "| o5 c e g e o6 c4 o5 g4 | o5 a o6 c e c o5 a4 e4 | o5 f a o6 c o5 a f4 c4 | o5 g b o6 d o5 b g4 r4 ]2";
static const char S_BASS[] =
    "@7 v13 q5 [o2 c4 g4 c4 g4 | o2 a4 e4 a4 e4 | o2 f4 o3 c4 o2 f4 o3 c4 | o2 g4 d4 g4 d4 ]2";
static const char S_DRUMS[] = "[o2@13v10c8 o8@9v4c8 o6@11v8c8 o8@9v4c8]16";

void ph_audio_load(void) {
    if (PH_MUS_TITLE >= 0) return;
    PH_MUS_TITLE = song_define("ph_lanterns", 92, true, T_LEAD, T_COMP, T_BASS, T_DRUMS);
    PH_MUS_PARTY = song_define("ph_groove", 124, true, P_LEAD, P_ARP, P_BASS, P_DRUMS);
    PH_MUS_SHOP = song_define("ph_market", 112, true, S_LEAD, "", S_BASS, S_DRUMS);
    PH_MUS_WIN = song_define("ph_fourstars", 140, false,
                             "@14 v12 o5 l8 c e g o6 c4 o5 g8 o6 c8 e8 g8 c2",
                             "@16 v8 o4 l4 c e g o5 c2", "@6 v13 o2 l4 c g c g c2", "@12 v8 o6 c1");
    PH_MUS_LOSE = song_define("ph_lastnight", 90, false, "@5 v11 o5 l4 e d c o4 b2. a2",
                              "@17 v6 o4 l4 a a g e2. e2", "@6 v12 o2 l4 a a e e2. a2", "");
    PH_MUS_BUST = song_define("ph_siren", 150, false, "@1 v11 o6 l8 c o5 g o6 c o5 g o6 c o5 g4.", "", "@6 v12 o2 l8 c c c c c c4.", "@12 v9 o6 c2");
    PH_MUS_NIGHT = song_define("ph_dusk", 110, false, "@15 v10 o5 l8 c f a o6 c4 o5 a4 f2", "@22 v6 o4 l4 f a o5 c2.",
                               "@6 v12 o2 l4 f c f2.", "");

    sfx_define("ph_knock", CH_NOISE, 200, "@13 v12 o3 c16 r16 c16");
    sfx_define("ph_enter", CH_P2, 220, "@35 v9 o5 c16 g16");
    sfx_define("ph_star", CH_P1, 200, "@39 v11 o6 c16 e16 g16 o7 c8");
    sfx_define("ph_trouble", CH_P2, 200, "@37 v10 o4 c16 o3 g16");
    sfx_define("ph_warn", CH_P1, 180, "@20 v11 o6 e8 r16 e8");
    sfx_define("ph_act", CH_P2, 220, "@39 v9 o5 e16 a16");
    sfx_define("ph_boot", CH_NOISE, 200, "@36 v10 o5 c8");
    sfx_define("ph_cash", CH_P1, 220, "@35 v11 o6 e16 o7 c8");
    sfx_define("ph_pop", CH_P2, 220, "@20 v9 o6 c32 g32");
    sfx_define("ph_buy", CH_P1, 200, "@35 v11 o5 g16 o6 c16 e8");
    sfx_define("ph_build", CH_NOISE, 160, "@13 v12 o3 c16 r16 @13 o3 c16 r16 @21 v9 o6 c8");
    sfx_define("ph_no", CH_P2, 180, "@37 v10 o3 c16 r32 c16");
    sfx_define("ph_shuffle", CH_NOISE, 200, "@36 v8 o6 c16 c16 c16");
    sfx_define("ph_move", CH_P2, 240, "@42 v7 o6 d32");
}
