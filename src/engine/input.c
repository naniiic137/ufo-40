#include "input.h"

#define NBITS 16 /* player 1 in bits 0..7, player 2 in bits 8..15 */

static uint32_t raw, cur, prev, blocked;
static int repeat_timer[NBITS];
static bool versus;

void input_set_raw(uint32_t mask) { raw = mask; }

void input_update(void) {
    prev = cur;
    cur = raw;
    /* a consumed button stays blocked until released */
    blocked &= cur;
    for (int i = 0; i < NBITS; i++) {
        if (cur & (1u << i)) repeat_timer[i]++;
        else repeat_timer[i] = 0;
    }
}

bool btn(int mask) { return (cur & ~blocked & (uint32_t)mask) != 0; }
bool btnp(int mask) { return ((cur & ~prev) & ~blocked & (uint32_t)mask) != 0; }
bool btnr(int mask) { return ((~cur & prev) & (uint32_t)mask) != 0; }
uint32_t input_held(void) { return cur & ~blocked; }

bool btn_repeat(int mask) {
    for (int i = 0; i < NBITS; i++) {
        if (!((uint32_t)mask & (1u << i))) continue;
        if (blocked & (1u << i)) continue;
        int t = repeat_timer[i];
        if (t == 1) return true;
        if (t > 18 && ((t - 18) % 5) == 0) return true;
    }
    return false;
}

void input_consume(void) { blocked |= cur; }

bool btn2(int mask) { return btn(mask << BTN_P2_SHIFT); }
bool btnp2(int mask) { return btnp(mask << BTN_P2_SHIFT); }
bool btn_repeat2(int mask) { return btn_repeat(mask << BTN_P2_SHIFT); }

void input_set_versus(bool on) { versus = on; }
bool input_versus(void) { return versus; }
