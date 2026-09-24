#include "input.h"

static uint32_t raw, cur, prev, blocked;
static int repeat_timer[8];

void input_set_raw(uint32_t mask) { raw = mask; }

void input_update(void) {
    prev = cur;
    cur = raw;
    /* a consumed button stays blocked until released */
    blocked &= cur;
    for (int i = 0; i < 8; i++) {
        if (cur & (1u << i)) repeat_timer[i]++;
        else repeat_timer[i] = 0;
    }
}

bool btn(int mask) { return (cur & ~blocked & (uint32_t)mask) != 0; }
bool btnp(int mask) { return ((cur & ~prev) & ~blocked & (uint32_t)mask) != 0; }
bool btnr(int mask) { return ((~cur & prev) & (uint32_t)mask) != 0; }
uint32_t input_held(void) { return cur & ~blocked; }

bool btn_repeat(int mask) {
    for (int i = 0; i < 8; i++) {
        if (!(mask & (1 << i))) continue;
        if (blocked & (1u << i)) continue;
        int t = repeat_timer[i];
        if (t == 1) return true;
        if (t > 18 && ((t - 18) % 5) == 0) return true;
    }
    return false;
}

void input_consume(void) { blocked |= cur; }
