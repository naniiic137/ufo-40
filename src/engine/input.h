/* UFO 40 - input model: D-pad, A, B, START, SELECT. */
#ifndef UFO_INPUT_H
#define UFO_INPUT_H

#include <stdint.h>
#include <stdbool.h>

enum {
    BTN_UP = 1 << 0,
    BTN_DOWN = 1 << 1,
    BTN_LEFT = 1 << 2,
    BTN_RIGHT = 1 << 3,
    BTN_A = 1 << 4,
    BTN_B = 1 << 5,
    BTN_START = 1 << 6,
    BTN_SELECT = 1 << 7,
    BTN_ANY = 0xFF
};

/* Player 2 (local versus) uses the same button bits shifted into bits 8..15
 * of the raw mask. Only games with a 2-player mode read them. */
#define BTN_P2_SHIFT 8

/* Platform: set the raw button mask before each engine update. */
void input_set_raw(uint32_t mask);
/* Engine: latch raw state into current/previous (called once per tick). */
void input_update(void);

bool btn(int mask);        /* held */
bool btnp(int mask);       /* pressed this tick */
bool btnr(int mask);       /* released this tick */
bool btn_repeat(int mask); /* pressed, with auto-repeat while held (menus) */
uint32_t input_held(void);
/* Swallow the current presses (e.g. after a scene change). */
void input_consume(void);

/* Player 2 versions of the above (mask uses the normal BTN_* values). */
bool btn2(int mask);
bool btnp2(int mask);
bool btn_repeat2(int mask);

/* A game turns versus mode on while a local 2-player match runs. The PC
 * platform then splits the keyboard in two halves and gives the second
 * gamepad to player 2. Off by default; the shell turns it off in the library. */
void input_set_versus(bool on);
bool input_versus(void);

#endif
