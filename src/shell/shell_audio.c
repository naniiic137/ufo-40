/* UFO 40 - console shell music and UI sounds. All compositions original. */
#include "shell.h"

int MUS_BOOT = -1, MUS_LIBRARY = -1;

/* "Saucer Lounge" - the library theme. F major, 16 bars.
 * Progression: F  Em7  Dm7  C | Bb  Am7  Gm7  C7 (twice). */
static const char LIB_LEAD[] =
    "@14 v12 q7 o4 l8"
    "| r c f a >c4< a g | a g e d e4 r4 | r d f a >c4< a >d< | c4 e g ^2"
    "| r d f b- >d4< b- a | g a e c e4 r c | d g b- >d ^4 c< b- | a g e g ^2"
    "| r c f a >c4 f4< | >e d c< a g4 e4 | f e d e f4 a4 | g2. r4"
    "| b- a g f d4 f4 | e f g a >c4< a4 | g4. a b-4 >d4< | >c4.< b- g4 e4";

static const char LIB_COMP[] =
    "v7 q5 l8"
    "| [o4 @16 r f]4 | [o4 @17 r e]4 | [o4 @17 r d]4 | [o4 @16 r c]4"
    "| [o3 @16 r b-]4 | [o3 @17 r a]4 | [o3 @17 r g]4 | [o4 @16 r c]4"
    "| [o4 @16 r f]4 | [o4 @17 r e]4 | [o4 @17 r d]4 | [o4 @16 r c]4"
    "| [o3 @16 r b-]4 | [o3 @17 r a]4 | [o3 @17 r g]4 | [o4 @16 r c]4";

#define BASS_BAR(lo, hi, fi) "[" lo " " hi " " fi " " hi "]2 "
static const char LIB_BASS[] =
    "@6 v15 q6 l8 "
    "[" BASS_BAR("o2f", "o3f", "o3c") BASS_BAR("o2e", "o3e", "o2b") BASS_BAR("o2d", "o3d", "o2a")
    BASS_BAR("o2c", "o3c", "o2g") BASS_BAR("o2b-", "o3b-", "o3f") BASS_BAR("o2a", "o3a", "o3e")
    BASS_BAR("o2g", "o3g", "o3d") BASS_BAR("o2c", "o3c", "o2g") "]2";

#define DR_BAR "o2@13v12c8 o8@9v6c8 o6@11v10c8 o8@9v6c8 o2@13v12c8 o2@13v9c8 o6@11v10c8 o8@9v6c8 "
#define DR_FILL "o2@13v12c8 o8@9v6c8 o6@11v10c8 o8@9v6c8 o6@11v9c16c16 o6@11v11c16c16 o6@11v12c8 o8@10v8c8 "
static const char LIB_DRUMS[] = "[" DR_BAR DR_BAR DR_BAR DR_BAR DR_BAR DR_BAR DR_BAR DR_FILL "]2";

/* Boot jingle: the beam-down arpeggio and a big major landing. */
static const char BOOT_P1[] = "@2 v11 o4 l32 c e g >c e g >c e g >c e g r8 @14 v13 o5 g8 >c8 e4 ^8 d16 e16 c2^4";
static const char BOOT_P2[] = "@2 v7 o3 l32 r16 g >c e g >c e g >c e g >c r16 @5 v10 o5 e8 g8 >c4 ^8 <b16 >c16 <g2^4";
static const char BOOT_TRI[] = "@7 v15 o2 l8 c r c r c r4 @6 o2 g8 >c8 c4 ^8 g16 a16 @7 c2^4";
static const char BOOT_NOISE[] = "@21 v6 o7 l32 [c]12 r8 @11 v10 o6 c8 c8 @12 v12 o5 c2.";

void shell_audio_init(void) {
    if (MUS_LIBRARY >= 0) return;
    MUS_LIBRARY = song_define("library", 104, true, LIB_LEAD, LIB_COMP, LIB_BASS, LIB_DRUMS);
    MUS_BOOT = song_define("boot", 150, false, BOOT_P1, BOOT_P2, BOOT_TRI, BOOT_NOISE);

    sfx_define("ui_move", CH_P2, 200, "@42 v9 o6 c32");
    sfx_define("ui_ok", CH_P2, 200, "@20 v11 o5 c32 g32 >c16");
    sfx_define("ui_back", CH_P2, 200, "@20 v10 o5 g32 d32 <g16");
    sfx_define("ui_error", CH_P2, 180, "@37 v11 o3 c16 r32 c16");
    sfx_define("ui_toast", CH_P1, 160, "@39 v12 o5 l16 c e g >c e g >c4");
    sfx_define("ui_pause", CH_P2, 180, "@15 v11 o5 e16 c16 <g8");
    sfx_define("cart_insert", CH_NOISE, 150, "@21 v12 o5 c16 r16 @13 v14 o3 c8 @40 v10 o6 c4");
}
