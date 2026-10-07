/* HOOPLA - the demo player for the tests. It works the menus with real
 * presses (letting go between them, so each counts as fresh) and plays
 * its fighter with the CPU's brain on its sharpest setting. Plan: which
 * fighter it takes into the tournament (hg.bot_plan); hg.bot_draft makes it
 * play draft battles instead. */
#include "hoop.h"

/* a fresh press every other frame */
static int pulse(int bit) { return (hg.bot_t++ & 1) ? bit : 0; }

static int toward(int cur, int want, int n) {
    if (cur == want) return 0;
    int right = (want - cur + n) % n;
    return right <= n / 2 ? BTN_RIGHT : BTN_LEFT;
}

static int match_buttons(void) {
    Match *m = &hg.m;
    if (m->winner >= 0 || m->ready_t > 0 || m->f[0].ctrl != CTRL_P1) return 0;
    int b = hoop_brain_think(0), out = 0;
    if (b & HP_L) out |= BTN_LEFT;
    if (b & HP_R) out |= BTN_RIGHT;
    if (b & HP_U) out |= BTN_UP;
    if (b & HP_D) out |= BTN_DOWN;
    if (b & HP_A) out |= BTN_A;
    if (b & HP_B) out |= BTN_B;
    return out;
}

int hoop_bot_buttons(void) {
    switch (hg.state) {
    case HS_TITLE:
        if (hg.title_sel != 0) return pulse(BTN_UP);
        return hg.state_t > 10 ? pulse(BTN_A) : 0;
    case HS_MODE: {
        int want = hg.bot_draft ? 1 : 0;
        if (hg.mode_sel != want) return pulse(BTN_DOWN);
        return pulse(BTN_A);
    }
    case HS_OPTIONS:
        return pulse(BTN_B);
    case HS_SELECT:
        if (hg.picked[0]) return 0;
        if (hg.pal[0] != 0) return pulse(BTN_UP);
        if (hg.cur[0] != hg.bot_plan) return pulse(toward(hg.cur[0], hg.bot_plan, HOOP_FIGHTERS));
        return pulse(BTN_A);
    case HS_DRAFT: {
        static const int DRAFT_SIDE[6] = {0, 1, 1, 0, 0, 1};
        if (hg.draft_turn >= 6 || DRAFT_SIDE[hg.draft_turn] != 0 || hg.state_t < 8) return 0;
        /* the best that's left, by the same list the CPU uses */
        int want = -1, bt = 99;
        for (int i = 0; i < 7; i++)
            if (!hg.taken[i] && HOOP_DEF[hg.pool[i]].tier < bt) { bt = HOOP_DEF[hg.pool[i]].tier; want = i; }
        if (want < 0) return 0;
        if (hg.draft_cur != want) return pulse(toward(hg.draft_cur, want, 7));
        return pulse(BTN_A);
    }
    case HS_MATCH:
        return match_buttons();
    case HS_REMATCH:
        if (hg.rematch_sel != 0) return pulse(BTN_UP);
        return hg.state_t > 32 ? pulse(BTN_A) : 0;
    case HS_LADDER:
    case HS_VS:
    case HS_SERIES:
    case HS_RESULT:
    case HS_OUT:
    case HS_ENDING:
    case HS_NOTE:
        return hg.state_t > 20 ? pulse(BTN_A) : 0;
    case HS_CREDITS:
        return BTN_A;
    default:
        return 0;
    }
}
