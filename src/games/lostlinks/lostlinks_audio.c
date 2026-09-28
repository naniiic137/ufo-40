/* LOST LINKS - original music (UFO-MML) and sound effects, written for
 * UFO 40. Every looping tune is whole bars of 4/4 on every channel. */
#include "lostlinks.h"

int LNK_MUS_TITLE = -1, LNK_MUS_LINKS, LNK_MUS_CAVES, LNK_MUS_SANDS, LNK_MUS_FEN, LNK_MUS_RUINS, LNK_MUS_BADGER,
    LNK_MUS_END, LNK_MUS_ITEM, LNK_MUS_OUT, LNK_MUS_TAPE;

#define HAT4 "o2@13v9c4 o8@9v4c4 o6@11v6c4 o8@9v4c4 "

/* "Lost Links" - the title: a slow walk in F. F C F-Dm C | F Bb-C F-C F */
static const char TITLE_LEAD[] =
    "@14 v11 q6"
    "| o5 c4 f8 a8 o6 c4. o5 a8 | o5 g4 e8 f8 g2 | o5 a4 o6 c8 d8 c4 o5 a4 | o5 g2. r4"
    "| o5 f4 a8 o6 c8 f4 e8 d8 | o6 c4 o5 b-8 a8 g2 | o5 a8 b-8 o6 c8 d8 c4 o5 e4 | o5 f2. r4";
static const char TITLE_HARM[] =
    "v6 q4 @16 o4 f2 f2 | c2 c2 | f2 @17 d2 | @16 c2 c2 | f2 f2 | b-2 c2 | f2 c2 | f1";
static const char TITLE_BASS[] =
    "@6 v13 q5 | o3 f4 c4 f4 c4 | o3 c4 o2 g4 o3 c4 o2 g4 | o3 f4 c4 d4 a4 | o3 c4 o2 g4 o3 c4 o2 g4"
    "| o3 f4 c4 f4 c4 | o2 b-4 f4 o3 c4 o2 g4 | o3 f4 c4 c4 o2 g4 | o3 f2 c2";
static const char TITLE_DRUMS[] = "[" HAT4 "]8";

/* "Fairway Breeze" - the links above: G major, two eight-bar halves */
static const char LINKS_LEAD[] =
    "@1 v11 q6"
    "| o5 g8 b8 o6 d8 o5 b8 g4 d4 | o5 f+8 a8 o6 d8 c8 o5 b4 a4 | o5 e8 g8 b8 g8 e4 b4 | o5 c8 e8 g8 b8 a2"
    "| o5 g8 b8 o6 d8 g8 f+4 d4 | o6 e8 d8 c8 o5 a8 f+4 d4 | o5 e8 f+8 g8 a8 b8 o6 c8 d8 e8 | o6 d2 r2"
    "| o6 e4 d8 c8 o5 b4 g4 | o5 a4 b8 o6 c8 d2 | o6 e8 f+8 g8 e8 d4 o5 b4 | o6 c4 o5 a4 f+2"
    "| o5 g8 a8 b8 o6 c8 d4 g4 | o6 f+8 e8 d8 c8 o5 b4 a4 | o5 g8 b8 a8 f+8 d4 f+4 | o5 g2. r4";
static const char LINKS_HARM[] =
    "v6 q4 @16 o4 g2 g2 | d2 d2 | @17 e2 e2 | @16 c2 d2 | g2 g2 | c2 d2 | c2 d2 | d2 d2"
    "| c2 g2 | d2 d2 | c2 g2 | @17 a2 @16 d2 | g2 g2 | d2 d2 | c2 d2 | g2 g2";
static const char LINKS_BASS[] =
    "@6 v14 q5 | o2 g4 o3 d4 o2 g4 o3 d4 | o2 d4 a4 d4 a4 | o2 e4 b4 e4 b4 | o2 c4 g4 d4 a4"
    "| o2 g4 o3 d4 o2 g4 o3 d4 | o2 c4 g4 d4 a4 | o2 c4 g4 d4 a4 | o2 d4 a4 d4 f+4"
    "| o2 c4 g4 g4 o3 d4 | o2 d4 a4 d4 a4 | o2 c4 g4 g4 o3 d4 | o2 a4 o3 e4 o2 d4 a4"
    "| o2 g4 o3 d4 o2 g4 o3 d4 | o2 d4 a4 d4 a4 | o2 c4 g4 d4 a4 | o2 g4 o3 d4 o2 g2";
static const char LINKS_DRUMS[] = "[o2@13v11c8 o8@9v5c8 o6@11v9c8 o8@9v5c8 ]32";

/* "Under the Links" - the caves: slow and dripping, A minor */
static const char CAVES_LEAD[] =
    "@5 v10 q5"
    "| o4 a4 r8 o5 c8 e4 r4 | o5 d4. c8 o4 b2 | o4 a4 r8 o5 e8 a4 g4 | o5 f2 e2"
    "| o5 d4 r8 f8 a4 g8 f8 | o5 e2. r4 | o5 c8 d8 e8 c8 o4 b4 g+4 | o4 a1";
static const char CAVES_PAD[] = "@4 v5 q8 o3 a1 | g1 | f1 | e1 | d1 | a1 | f1 | e1";
static const char CAVES_BASS[] = "@6 v12 q6 o2 a2 a2 | g2 g2 | f2 f2 | e2 e2 | d2 d2 | a2 a2 | f2 f2 | e2 e2";
static const char CAVES_DRUMS[] = "[o7@21v4c4 r4 o8@9v3c8 r8 r4 ]8";

/* "Bunker Heat" - the dunes: D with a flat second, dry and swaying */
static const char SANDS_LEAD[] =
    "@1 v11 q5"
    "| o5 d8 e-8 f+8 g8 a4 g8 f+8 | o5 e-8 d8 e-8 f+8 d2 | o5 a8 b-8 a8 g8 f+8 e-8 d4 | o5 e-4 f+4 d2"
    "| o6 d8 c8 o5 b-8 a8 g4 a8 b-8 | o5 a8 g8 f+8 e-8 d4 f+4 | o5 g8 f+8 g8 a8 b-4 a4 | o5 d2. r4";
static const char SANDS_OFF[] =
    "v5 q3 @17 o4 d2 d2 | @16 e-2 @17 d2 | d2 g2 | @16 e-2 @17 d2 | g2 g2 | d2 d2 | g2 @16 e-2 | @17 d1";
static const char SANDS_BASS[] =
    "@6 v14 q5 o2 d4 a4 d4 a4 | e-4 b-4 d4 a4 | d4 a4 g4 o3 d4 | o2 e-4 b-4 d4 a4"
    "| g4 o3 d4 o2 g4 o3 d4 | o2 d4 a4 d4 a4 | g4 o3 d4 o2 e-4 b-4 | d2 d2";
static const char SANDS_DRUMS[] = "[o2@13v11c8 o6@11v6c16 o6@11v6c16 o8@9v5c8 o6@11v9c8 ]16";

/* "Heron Fen" - the water: bells over a lydian lilt in C */
static const char FEN_LEAD[] =
    "@15 v11 q7"
    "| o5 e8 g8 b8 o6 d8 c4 o5 g4 | o5 f+8 a8 o6 c8 e8 d2 | o6 e8 d8 c8 o5 b8 a4 g4 | o5 f+2 g2"
    "| o5 e8 g8 o6 c8 e8 g4 e4 | o6 f+8 e8 d8 c8 o5 b2 | o5 a8 b8 o6 c8 d8 e4 d4 | o6 c2. r4";
static const char FEN_PAD[] = "@4 v5 q8 o4 e1 | f+1 | e1 | f+1 | g1 | g1 | a1 | g1";
static const char FEN_BASS[] = "@6 v12 q6 o3 c2 g2 | d2 a2 | o2 a2 o3 e2 | d2 a2 | c2 g2 | e2 b2 | f2 o4 c2 | o3 c1";
static const char FEN_DRUMS[] = "[o8@9v3c8 o8@9v2c8 o6@11v4c8 o8@9v2c8 ]16";

/* "The Old Clubhouse" - the ruins and the sanctum: stately D minor */
static const char RUINS_LEAD[] =
    "@23 v11 q6"
    "| o4 d4 a4 o5 d4. c8 | o4 b-2 a2 | o4 g4 b-4 o5 d4 e4 | o5 f2. e4"
    "| o5 d4 f4 a4 g8 f8 | o5 e2 c+2 | o5 d8 e8 f8 g8 a4 c+4 | o5 d1";
static const char RUINS_HARM[] = "@22 v6 q6 o4 f2 f2 | d2 c+2 | d2 g2 | a2 a2 | f2 d2 | c+2 e2 | f2 e2 | f1";
static const char RUINS_BASS[] = "@6 v13 q6 o2 d2 d2 | g2 a2 | g2 b-2 | a2 a2 | d2 d2 | a2 a2 | d2 a2 | d1";
static const char RUINS_DRUMS[] = "[o2@13v10c4 r4 o6@11v7c4 r4 ]8";

/* "Brass Badger" - the fight: E minor at a gallop */
static const char BADGER_LEAD[] =
    "@1 v12 q5"
    "| o5 e8 e8 g8 e8 b8 e8 a8 g8 | o5 f+8 f+8 a8 f+8 o6 c8 o5 b8 a8 f+8 | o5 e8 e8 g8 e8 b8 e8 o6 e8 d8"
    "| o6 c8 o5 b8 a8 g8 f+4 d+4 | o6 e4 d8 e8 g4 f+8 e8 | o6 d4 c8 d8 f+4 e8 d8"
    "| o6 c8 o5 b8 a8 b8 o6 c8 d8 e8 f+8 | o6 g4 f+4 e2";
static const char BADGER_OFF[] =
    "v6 q3 @17 o4 e8 r8 e8 r8 e8 r8 e8 r8 | @16 d8 r8 d8 r8 d8 r8 d8 r8 | @17 e8 r8 e8 r8 e8 r8 e8 r8"
    "| @16 c8 r8 c8 r8 o3 b8 r8 b8 r8 | @17 o4 e8 r8 e8 r8 e8 r8 e8 r8 | @16 d8 r8 d8 r8 d8 r8 d8 r8"
    "| c8 r8 c8 r8 d8 r8 d8 r8 | @17 e8 r8 e8 r8 e8 r8 e8 r8";
static const char BADGER_BASS[] =
    "@6 v15 q4 [o2 e8 e8 b8 e8]2 [d8 d8 a8 d8]2 [e8 e8 b8 e8]2 c8 c8 g8 c8 o1 b8 b8 o2 f+8 o1 b8"
    "[o2 e8 e8 b8 e8]2 [d8 d8 a8 d8]2 c8 c8 g8 c8 d8 d8 a8 d8 [e8 e8 b8 e8]2";
static const char BADGER_DRUMS[] = "[o2@13v13c8 o8@9v6c8 o6@11v11c8 o8@9v6c8 ]16";

/* "Green Again" - the ending */
static const char END_LEAD[] =
    "@14 v12 q7"
    "| o5 e4 g4 o6 c4 e4 | o6 d4. c8 o5 b2 | o5 a4 o6 c4 f4 a4 | o6 g2. r4"
    "| o6 e4 d4 c4 e4 | o6 f4 e4 d4 f4 | o6 e4 d8 c8 o5 b4 o6 d4 | o6 c2. r4";
static const char END_HARM[] = "v6 q5 @16 o4 c2 c2 | g2 g2 | f2 f2 | c2 c2 | @17 a2 a2 | @16 f2 f2 | g2 g2 | c1";
static const char END_BASS[] = "@6 v13 q6 o3 c2 o2 g2 | g2 d2 | f2 o3 c2 | o3 c2 o2 g2 | a2 e2 | d2 f2 | g2 g2 | o3 c1";
static const char END_DRUMS[] = "[o2@13v8c4 o8@9v4c4 o6@11v6c4 o8@9v4c4 ]8";

/* jingles */
static const char ITEM_P1[] = "@39 v12 o5 l16 g b o6 d g8 d16 g16 b4";
static const char ITEM_TRI[] = "@6 v13 o3 l8 g o4 d g4";
static const char OUT_P1[] = "@5 v11 o5 l8 d c o4 b- a g4. r8";
static const char OUT_TRI[] = "@6 v12 o3 l4 g d o2 g4";
static const char TAPE_P1[] = "@20 v9 o6 l32 c g c g c g r16 @15 v10 o5 e8 c8 o4 g4";
static const char TAPE_TRI[] = "@7 v11 o3 l8 r4 c g c4";

void lnk_audio_load(void) {
    if (LNK_MUS_TITLE >= 0) return;
    LNK_MUS_TITLE = song_define("lnk_title", 96, true, TITLE_LEAD, TITLE_HARM, TITLE_BASS, TITLE_DRUMS);
    LNK_MUS_LINKS = song_define("lnk_links", 144, true, LINKS_LEAD, LINKS_HARM, LINKS_BASS, LINKS_DRUMS);
    LNK_MUS_CAVES = song_define("lnk_caves", 92, true, CAVES_LEAD, CAVES_PAD, CAVES_BASS, CAVES_DRUMS);
    LNK_MUS_SANDS = song_define("lnk_sands", 126, true, SANDS_LEAD, SANDS_OFF, SANDS_BASS, SANDS_DRUMS);
    LNK_MUS_FEN = song_define("lnk_fen", 108, true, FEN_LEAD, FEN_PAD, FEN_BASS, FEN_DRUMS);
    LNK_MUS_RUINS = song_define("lnk_ruins", 84, true, RUINS_LEAD, RUINS_HARM, RUINS_BASS, RUINS_DRUMS);
    LNK_MUS_BADGER = song_define("lnk_badger", 164, true, BADGER_LEAD, BADGER_OFF, BADGER_BASS, BADGER_DRUMS);
    LNK_MUS_END = song_define("lnk_end", 100, true, END_LEAD, END_HARM, END_BASS, END_DRUMS);
    LNK_MUS_ITEM = song_define("lnk_item", 150, false, ITEM_P1, "", ITEM_TRI, "");
    LNK_MUS_OUT = song_define("lnk_out", 110, false, OUT_P1, "", OUT_TRI, "");
    LNK_MUS_TAPE = song_define("lnk_tape", 130, false, TAPE_P1, "", TAPE_TRI, "");

    sfx_define("lnk_hit", CH_NOISE, 240, "@36 v12 o6 c16");
    sfx_define("lnk_chip", CH_NOISE, 240, "@36 v10 o5 c16 @9 v6 o7 c16");
    sfx_define("lnk_bounce", CH_P2, 240, "@42 v8 o5 g32");
    sfx_define("lnk_land", CH_NOISE, 240, "@9 v7 o5 c32");
    sfx_define("lnk_cup", CH_P2, 200, "@33 v11 o5 c16 <c16");
    sfx_define("lnk_splash", CH_NOISE, 160, "@10 v11 o6 c8");
    sfx_define("lnk_smash", CH_NOISE, 180, "@40 v13 o4 c8");
    sfx_define("lnk_bush", CH_NOISE, 220, "@36 v10 o5 c16 o6 c16");
    sfx_define("lnk_pin", CH_P2, 240, "@39 v11 o5 l32 e g o6 c e");
    sfx_define("lnk_talk", CH_P2, 240, "@42 v9 o5 c32 e32");
    sfx_define("lnk_plate", CH_P2, 240, "@15 v11 o6 c16 g16");
    sfx_define("lnk_help", CH_P1, 220, "@39 v11 o5 l16 c e g o6 c");
    sfx_define("lnk_flap", CH_NOISE, 240, "@36 v8 o7 c32 r32 c32");
    sfx_define("lnk_caw", CH_P2, 200, "@37 v10 o6 c16 o5 a16");
    sfx_define("lnk_crash", CH_NOISE, 180, "@40 v12 o5 c8");
    sfx_define("lnk_bonk", CH_P2, 240, "@35 v11 o4 g32 c32");
    sfx_define("lnk_sting", CH_P2, 240, "@33 v11 o6 c16");
    sfx_define("lnk_bird", CH_P1, 240, "@32 v11 o6 c16 e16");
    sfx_define("lnk_clang", CH_NOISE, 200, "@40 v14 o6 c8");
    sfx_define("lnk_aim", CH_P2, 240, "@20 v8 o6 c32 r32 c32");
    sfx_define("lnk_boom", CH_NOISE, 160, "@34 v14 o3 c4");
    sfx_define("lnk_dig", CH_NOISE, 200, "@13 v10 o3 c8 c8");
    sfx_define("lnk_tick", CH_P2, 240, "@42 v6 o6 c32");
    sfx_define("lnk_cancel", CH_P2, 240, "@33 v8 o5 c32");
    sfx_define("lnk_charge", CH_P2, 240, "@38 v7 o4 c16");
    sfx_define("lnk_hop", CH_P1, 240, "@32 v10 o5 c16");
    sfx_define("lnk_brake", CH_NOISE, 240, "@36 v9 o4 c16");
}
