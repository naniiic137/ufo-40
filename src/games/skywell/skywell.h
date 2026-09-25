/* SKYWELL - shared declarations. Cartridge 07 of UFO 40,
 * a tribute to Velgress (UFO 50 #7). See docs/games/07-skywell.md. */
#ifndef SKYWELL_H
#define SKYWELL_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define SW_FLOORS 30      /* floors per level */
#define SW_FLOOR_H 32     /* pixels per floor */
#define SW_TILE 16
#define SW_COLS 13        /* the shaft is 13 tiles wide */
#define SW_X0 56          /* screen x of the shaft's left edge */
#define SW_X1 (SW_X0 + SW_COLS * SW_TILE)

enum {
    K_STAND, K_RUN1, K_RUN2, K_JUMP, K_FALL, K_SHOOT_UP, K_STUN, K_STAR, K_DEAD,
    K_CLOUD, K_CRATE, K_ROCK, K_STARBLOCK, K_COINBLOCK, K_MINE, K_BOMB, K_ZAPBLOCK, K_BUBBLE, K_METAL,
    K_COIN1, K_COIN2, K_KEY, K_BAT_SLEEP, K_BAT1, K_BAT2, K_MINNOW1, K_MINNOW2, K_JELLY, K_JELLY_SMALL,
    K_SQUID1, K_SQUID2, K_OWL1, K_OWL2, K_EYE, K_EYE_SHUT, K_HAND, K_HAND_OPEN, K_TICKMINE, K_SHOT, K_ORB,
    K_TINKER, K_KIP_BIG, K_GRINDER, K_SPRITE_COUNT
};
extern Sprite sw_spr[K_SPRITE_COUNT];
void sw_art_load(void);
void sw_audio_load(void);
extern int SW_MUS_LEVEL[3], SW_MUS_BOSS, SW_MUS_TITLE, SW_MUS_SHOP, SW_MUS_OVER, SW_MUS_END;

#endif
