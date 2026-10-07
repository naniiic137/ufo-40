/* SOUNDINGS - original music (UFO-MML) and sound effects, written for UFO 40.
 * Like the cartridge it pays tribute to, the caves are silent: music plays
 * at the title and the raft, in fights, in the red deep, in the still green
 * room nobody can explain, and at the end. Every looping tune is eight bars
 * of 4/4 on every channel. */
#include "sdg.h"

int SDG_MUS_TITLE = -1, SDG_MUS_RAFT, SDG_MUS_BATTLE, SDG_MUS_WARDEN, SDG_MUS_GLOAM, SDG_MUS_DEEP, SDG_MUS_STILL,
    SDG_MUS_WIN, SDG_MUS_WIPE, SDG_MUS_END, SDG_MUS_LEVEL;

/* "Soundings" - the title: a slow line let down into D minor */
static const char TITLE_P1[] =
    "@5 v10 q7 o5 d4 f4 a2 | g4 f8 e8 d2 | c4 e4 g2 | f4. e8 d2"
    "| d4 f4 a4 o6 d4 | c4 o5 a4 b-2 | a4 g4 f4 e4 | d1";
static const char TITLE_P2[] = "@4 v5 q8 o4 a1 | g1 | g1 | a1 | a1 | f1 | e1 | f1";
static const char TITLE_TRI[] = "@6 v12 q7 o2 d2 a2 | g2 d2 | c2 g2 | d2 a2 | d2 a2 | f2 >c2< | a2 e2 | d1";
static const char TITLE_NOI[] = "[o8@9v3c4 r4 o8@9v2c4 r4]8";

/* "The Raft" - the base: a lazy swell in F */
static const char RAFT_P1[] =
    "@14 v10 q6 o5 c4 f4 a4 f4 | g4. a8 g2 | a4 o6 c4 o5 b-4 a4 | g2 r2"
    "| f4 a4 o6 c4 d4 | c4. o5 b-8 a2 | g4 b-4 a4 g4 | f2 r2";
static const char RAFT_P2[] =
    "@16 v5 q3 o4 r4 a4 r4 a4 | r4 g4 r4 g4 | r4 a4 r4 a4 | r4 g4 r4 g4"
    "| r4 a4 r4 a4 | r4 a4 r4 a4 | r4 g4 r4 g4 | r4 a4 r4 a4";
static const char RAFT_TRI[] = "@6 v12 q6 o2 f2 >c2< | c2 g2 | f2 >c2< | c2 g2 | f2 a2 | f2 >c2< | c2 g2 | f2 >c2<";
static const char RAFT_NOI[] = "[o3@13v6c4 o8@9v3c4 o8@9v3c8 o8@9v2c8 o8@9v3c4]8";

/* "Something in the Dark" - every fight: E minor, quick */
static const char BAT_P1[] =
    "@1 v12 q5 o5 e8 g8 b8 o6 e8 d8 o5 b8 g8 a8 | b4 g4 e2 | c8 e8 g8 o6 c8 o5 b8 g8 e8 g8 | f+4 a4 b2"
    "| o6 e8 d8 c8 o5 b8 a8 g8 f+8 g8 | a4 f+4 d2 | e8 f+8 g8 a8 b8 o6 c8 d8 d+8 | e2 o5 b2";
static const char BAT_P2[] =
    "@17 v6 q3 o4 [r8 e8]4 [r8 e8]4 @16 [r8 c8]4 [r8 <b8>]4 @17 [r8 e8]4 @16 [r8 d8]4 @17 [r8 e8]4 @16 [r8 <b8>]4";
static const char BAT_TRI[] =
    "@6 v15 q4 [o2 e8 e8 b8 e8]2 [o2 e8 e8 b8 e8]2 [o2 c8 c8 g8 c8]2 [o2 b8 b8 >f+8< b8]2"
    "[o2 e8 e8 b8 e8]2 [o2 d8 d8 a8 d8]2 [o2 e8 e8 b8 e8]2 [o2 b8 b8 >f+8< b8]2";
static const char BAT_NOI[] = "[o2@13v13c8 o8@9v6c8 o4@11v10c8 o8@9v6c8 o2@13v12c8 o2@13v9c8 o4@11v10c8 o8@9v6c8]8";

/* "The Abbot" - the shrine's keeper: C minor, stately */
static const char ABB_P1[] =
    "@23 v12 q6 o4 g4 o5 c4 e-4. d8 | c4 o4 b4 g2 | a-4 o5 c4 f4. e-8 | d2 o4 b2"
    "| o5 e-4 g4 o6 c4 o5 b-8 a-8 | g4 f4 e-4 d4 | e-8 f8 g8 a-8 b4 o6 d4 | c2. r4";
static const char ABB_P2[] =
    "@17 v6 q3 o4 [r8 c8]4 @16 [r8 <g8>]4 @17 [r8 f8]4 @16 [r8 <g8>]4 @17 [r8 c8]4 [r8 c8]4 @16 [r8 <a-8>]2 [r8 <g8>]2 @17 [r8 c8]4";
static const char ABB_TRI[] =
    "@6 v14 q5 o2 c4 c4 g4 c4 | o2 g4 g4 d4 g4 | o2 f4 f4 >c4< f4 | o2 g4 g4 d4 <g4> | o2 c4 c4 g4 c4"
    "| o2 c4 e-4 g4 e-4 | o2 a-4 a-4 g4 g4 | o2 c2 c2";
static const char ABB_NOI[] = "[o2@13v14c4 o4@11v10c8 o2@13v8c8 o2@13v12c4 o4@11v12c4]8";

/* "The Gloamheart" - the last fight: F-sharp minor, driving */
static const char GLM_P1[] =
    "@14 v12 q5 o5 f+8 f+8 a8 f+8 o6 c+8 o5 a8 f+8 e8 | f+4 c+4 f+2 | d8 d8 f+8 d8 a8 f+8 d8 c+8 | c+4 g+4 c+2"
    "| f+8 g+8 a8 b8 o6 c+8 d8 c+8 o5 b8 | a4 f+4 d4 b4 | c+8 d8 e8 f+8 g+8 a8 b8 o6 c+8 | o5 f+2 c+2";
static const char GLM_P2[] =
    "@17 v6 q3 o4 [r8 f+8]4 [r8 f+8]4 @16 [r8 d8]4 [r8 c+8]4 @17 [r8 f+8]4 @16 [r8 d8]4 [r8 c+8]4 @17 [r8 f+8]4";
static const char GLM_TRI[] =
    "@6 v15 q4 [o2 f+8 f+8 >c+8< f+8]2 [o2 f+8 f+8 >c+8< f+8]2 [o2 d8 d8 a8 d8]2 [o2 c+8 c+8 g+8 c+8]2"
    "[o2 f+8 f+8 >c+8< f+8]2 [o2 d8 d8 a8 d8]2 [o2 c+8 c+8 g+8 c+8]2 [o2 f+8 f+8 >c+8< f+8]2";
static const char GLM_NOI[] =
    "[o2@13v14c8 o8@9v6c8 o4@11v12c8 o2@13v10c8 o2@13v14c8 o4@11v9c16 o4@11v9c16 o4@11v12c8 o8@10v7c8]8";

/* "The Red Deep" - the last region: a slow heartbeat under a thin line */
static const char DEEP_P1[] =
    "@5 v8 q7 o4 r2 a4. g+8 | a1 | r2 o5 c4. o4 b8 | o5 c1 | r2 e4. d8 | c2 o4 b2 | a1 | r1";
static const char DEEP_P2[] = "@4 v5 q8 o3 a1 | f1 | a1 | e1 | a1 | f1 | e1 | a1";
static const char DEEP_TRI[] = "@6 v13 q3 [o2 a8 a8 r4 r2]8";
static const char DEEP_NOI[] = "[o2@13v10c8 o2@13v7c8 r4 r2]8";

/* "The Still Room" - the green room: bells hanging in the water */
static const char STILL_P1[] =
    "@15 v9 q8 o5 e4 g4 b4 o6 d4 | c+2 o5 a2 | f+4 a4 o6 c+4 e4 | d1"
    "| o5 e4 g4 b4 o6 d4 | e2 f+2 | g4 f+4 e4 c+4 | d1";
static const char STILL_P2[] =
    "@22 v5 q6 o4 [b8 g8 e8 g8]2 [a8 e8 c+8 e8]2 [a8 f+8 c+8 f+8]2 [a8 f+8 d8 f+8]2"
    "[b8 g8 e8 g8]2 [a8 f+8 c+8 f+8]2 [b8 g8 e8 g8]2 [a8 f+8 d8 f+8]2";
static const char STILL_TRI[] = "@6 v11 q8 o2 e1 | a1 | f+1 | d1 | e1 | f+1 | e1 | d1";
static const char STILL_NOI[] = "[o8@9v2c2 o8@9v2c2]8";

/* "Toward the Light" - the ending */
static const char END_P1[] =
    "@23 v11 q7 o5 d4 f+4 a4 o6 d4 | c+4. o5 b8 a2 | g4 b4 o6 d4 g4 | f+2. r4"
    "| e4 f+4 g4 e4 | d4 c+4 o5 b4 a4 | g4 f+8 e8 f+4 a4 | d2. r4";
static const char END_P2[] =
    "@22 v6 q6 o4 [a8 f+8 d8 f+8]2 [a8 e8 c+8 e8]2 [b8 g8 d8 g8]2 [a8 f+8 d8 f+8]2"
    "[b8 g8 e8 g8]2 [a8 f+8 d8 f+8]2 [b8 g8 e8 g8]2 [a8 f+8 d8 f+8]2";
static const char END_TRI[] = "@6 v13 q6 o2 d2 a2 | a2 e2 | g2 >d2< | d2 a2 | e2 b2 | d2 f+2 | e2 a2 | d2 a2";
static const char END_NOI[] = "[o3@13v7c4 o8@9v3c4 o4@11v5c4 o8@9v3c4]8";

/* jingles */
static const char WIN_P1[] = "@39 v11 o5 l16 e g b >e8. <b16 >e4";
static const char WIN_TRI[] = "@6 v12 o3 l8 e b >e4";
static const char WIPE_P1[] = "@5 v10 o4 l4 a f e d2";
static const char WIPE_TRI[] = "@6 v12 o2 l2 d <a";
static const char LEVEL_P1[] = "@39 v10 o5 l32 d f+ a >d f+ a >d8";

void sdg_audio_load(void) {
    if (SDG_MUS_TITLE >= 0) return;
    SDG_MUS_TITLE = song_define("sdg_title", 84, true, TITLE_P1, TITLE_P2, TITLE_TRI, TITLE_NOI);
    SDG_MUS_RAFT = song_define("sdg_raft", 92, true, RAFT_P1, RAFT_P2, RAFT_TRI, RAFT_NOI);
    SDG_MUS_BATTLE = song_define("sdg_battle", 150, true, BAT_P1, BAT_P2, BAT_TRI, BAT_NOI);
    SDG_MUS_WARDEN = song_define("sdg_abbot", 126, true, ABB_P1, ABB_P2, ABB_TRI, ABB_NOI);
    SDG_MUS_GLOAM = song_define("sdg_gloam", 150, true, GLM_P1, GLM_P2, GLM_TRI, GLM_NOI);
    SDG_MUS_DEEP = song_define("sdg_deep", 66, true, DEEP_P1, DEEP_P2, DEEP_TRI, DEEP_NOI);
    SDG_MUS_STILL = song_define("sdg_still", 72, true, STILL_P1, STILL_P2, STILL_TRI, STILL_NOI);
    SDG_MUS_END = song_define("sdg_end", 84, true, END_P1, END_P2, END_TRI, END_NOI);
    SDG_MUS_WIN = song_define("sdg_win", 140, false, WIN_P1, "", WIN_TRI, "");
    SDG_MUS_WIPE = song_define("sdg_wipe", 90, false, WIPE_P1, "", WIPE_TRI, "");
    SDG_MUS_LEVEL = song_define("sdg_level", 150, false, LEVEL_P1, "", "", "");

    sfx_define("sdg_tick", CH_P2, 240, "@42 v8 o6 c32");
    sfx_define("sdg_nope", CH_P2, 200, "@37 v10 o3 c16 r32 c16");
    sfx_define("sdg_buy", CH_P2, 240, "@35 v11 o6 c16 g16 >c16");
    sfx_define("sdg_chest", CH_P2, 220, "@39 v11 o5 l32 c e g >c e");
    sfx_define("sdg_lever", CH_NOISE, 200, "@40 v10 o4 c8");
    sfx_define("sdg_boom", CH_NOISE, 120, "@34 v15 o3 c4");
    sfx_define("sdg_heal", CH_P2, 240, "@39 v9 o5 l32 g >c e g");
    sfx_define("sdg_ink", CH_P2, 200, "@33 v9 o5 c8 <g8");
    sfx_define("sdg_head", CH_P2, 180, "@15 v11 o5 l16 e g >c e4");
    sfx_define("sdg_pew", CH_P2, 240, "@33 v6 o6 c32");
    sfx_define("sdg_sting", CH_P2, 220, "@37 v10 o4 e16 c16");
    sfx_define("sdg_burst", CH_NOISE, 200, "@36 v11 o6 c8");
    sfx_define("sdg_fight", CH_P2, 200, "@32 v11 o4 l32 c e g >c e g");
    sfx_define("sdg_splash", CH_NOISE, 160, "@36 v9 o5 c8 @9 v5 o7 c16");
    sfx_define("sdg_hit", CH_NOISE, 240, "@9 v10 o6 c16");
    sfx_define("sdg_weak", CH_P2, 240, "@39 v11 o6 l32 c g >c");
    sfx_define("sdg_ouch", CH_P2, 200, "@37 v11 o4 e16 c16 <g16");
    sfx_define("sdg_miss", CH_NOISE, 240, "@36 v6 o7 c32");
}
