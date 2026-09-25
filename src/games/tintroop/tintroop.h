/* TIN TROOP - shared declarations. Cartridge 06 of UFO 40,
 * a tribute to Mortol (UFO 50 #6). See docs/games/06-tin-troop.md. */
#ifndef TINTROOP_H
#define TINTROOP_H

#include "../../shell/gamedef.h"
#include "../../shell/ui.h"

#define TT_TS 10         /* tile size in pixels */
#define TT_ROWS 16       /* every level is 16 tiles tall */
#define TT_MAXW 200      /* and at most 200 wide */
#define TT_LEVELS 10
#define TT_START_LIVES 20

typedef struct TTLevel {
    const char *name;    /* "1-A" */
    const char *title;   /* "THE PLAYROOM FLOOR" */
    int world;           /* 1..4 */
    const char *rows[TT_ROWS];
} TTLevel;

extern const TTLevel TT_LEVEL[TT_LEVELS];

/* Tile legend (see tintroop_levels.c):
 *  ' ' air        '#' wall       'x' breakable   '^' spikes     'w' water
 *  'f' lit wick   'c' unlit wick 'P' seed pot    '[' switch A   ']' switch B
 *  '|' gate A     '!' gate B     'L' 'R' scale pans (3 wide)
 *  'Z' water pipe (pours water in)   'U' drain (lets it out)
 *  'H' launcher (sends out fish in water, dart planes in air)
 *  '<' '>' laser blocks facing left / right
 *  '1'..'9' life tag   'E' exit   'D' toy-box door   'S' start
 *  foes: 'm' mouse 'a' paper dart 'r' ram 'q' fish 'o' pill bug 'd' dragon
 *  boss: 'J' jack box (4x5)  'N' carved head (2x2) */

enum {
    T_SOLDIER1, T_SOLDIER2, T_SOLDIER3, T_SOLDIER_JUMP, T_SOLDIER_CHUTE, T_CHUTE, T_SOLDIER_SWIM,
    T_SOLDIER_CLIMB, T_ARROW, T_LODGED, T_CORPSE, T_STONE, T_BLIMP,
    T_MOUSE1, T_MOUSE2, T_DART1, T_DART2, T_RAM1, T_RAM2, T_RAM_CHARGE, T_FISH1, T_FISH2,
    T_BUG1, T_BUG2, T_DRAGON1, T_DRAGON2, T_FLAME1, T_FLAME2, T_SEED, T_POT, T_TAG,
    T_EXIT, T_DOOR, T_VINE, T_SWITCH_UP, T_SWITCH_DOWN, T_GATE,
    T_JACK_BOX, T_JACK_HEAD, T_JACK_MOUTH, T_HEAD_FLAME, T_HEAD_SEED, T_HEAD_BUG, T_ORB,
    T_PIP_BIG, T_SPRITE_COUNT
};
extern Sprite tt_spr[T_SPRITE_COUNT];
void tt_art_load(void);
void tt_audio_load(void);
extern int TT_MUS_WORLD[4], TT_MUS_BOSS, TT_MUS_TITLE, TT_MUS_MAP, TT_MUS_CLEAR, TT_MUS_FAIL, TT_MUS_END;

#endif
