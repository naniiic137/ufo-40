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

#endif
