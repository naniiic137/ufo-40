/* PETAL PARADE - shared declarations. Cartridge 05 of UFO 40,
 * a tribute to Magic Garden (UFO 50 #5). See docs/games/05-petal-parade.md. */
#ifndef PETALPARADE_H
#define PETALPARADE_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define PP_N 12        /* the garden is 12 x 12 tiles */
#define PP_TILE 12
#define PP_GOAL 200    /* pups to save */

enum {
    P_LINA_DOWN1, P_LINA_DOWN2, P_LINA_UP1, P_LINA_UP2, P_LINA_SIDE1, P_LINA_SIDE2, P_LINA_HOP, P_LINA_FALL,
    P_PUP1, P_PUP2, P_PUP_TRAIL, P_BRAMBLE1, P_BRAMBLE2, P_BRAMBLE_DAZED,
    P_TOAD1, P_TOAD2, P_JAR, P_WITCH, P_WITCH_CACKLE, P_LINA_BIG, P_HEART, P_PETAL,
    P_SPRITE_COUNT
};
extern Sprite pp_spr[P_SPRITE_COUNT];
void pp_art_load(void);
void pp_audio_load(void);
extern int PP_MUS_GARDEN, PP_MUS_RUSH, PP_MUS_TITLE, PP_MUS_OVER, PP_MUS_END;

#endif
