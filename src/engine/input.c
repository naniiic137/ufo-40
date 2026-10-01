#include "input.h"

#define NBITS 16 /* player 1 in bits 0..7, player 2 in bits 8..15 */

static uint32_t raw, cur, prev, blocked;
static uint32_t phys_cur, phys_prev; /* the buttons as pressed, before the layout */
static uint8_t remap_src[8];
static bool remapped;
static int repeat_timer[NBITS];
static bool versus;

void input_set_raw(uint32_t mask) { raw = mask; }

void input_set_remap(const uint8_t src[8]) {
    remapped = false;
    for (int i = 0; i < 8; i++) {
        remap_src[i] = src ? (uint8_t)(src[i] & 7) : (uint8_t)i;
        remapped |= remap_src[i] != i;
    }
}

static uint32_t apply_remap(uint32_t m) {
    if (!remapped) return m;
    uint32_t out = m & ~0xFFFFu;
    for (int p = 0; p < 2; p++)
        for (int i = 0; i < 8; i++)
            if (m & (1u << (remap_src[i] + p * BTN_P2_SHIFT))) out |= 1u << (i + p * BTN_P2_SHIFT);
    return out;
}

void input_update(void) {
    phys_prev = phys_cur;
    phys_cur = raw;
    /* both through today's layout: a change of layout is never a press */
    prev = apply_remap(phys_prev);
    cur = apply_remap(phys_cur);
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
uint32_t input_down(void) { return cur; }

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
