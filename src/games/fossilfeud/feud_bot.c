/* FOSSIL FEUD - the demo player for the tests. It thinks with the same
 * planner as the Slate Flock (at its best, and it peeks through the fog,
 * which a person can't), then works the menus with real button presses:
 * the cursor to the tile, the menu row, the head counts, A. Every press is
 * let go the next frame, so a held button never blocks the next one.
 * Through the campaign it takes the first scenario not yet won. */
#include "feud.h"

int feud_bot_goal_type;
static FeudStep goal;
static bool have_goal;
static int pray_turn = -1, pray_pick;
static Rng bot_rng;
static bool released;

static int cursor_to(int target) {
    int c = fg.cur % FD_COLS, r = fg.cur / FD_COLS;
    int tc = target % FD_COLS, tr = target / FD_COLS;
    if (tc < c) return BTN_LEFT;
    if (tc > c) return BTN_RIGHT;
    if (tr < r) return BTN_UP;
    if (tr > r) return BTN_DOWN;
    return 0;
}

static int list_to(int sel, int target, int up, int down) {
    if (sel < target) return down;
    if (sel > target) return up;
    return 0;
}

static int split_buttons(void) {
    /* get every row to the planned count, then go */
    for (int k = 0; k < FU_KINDS; k++) {
        if (!feud_movable(&fg.m, fg.src, k) || fg.split[k] == goal.counts[k]) continue;
        if (fg.split_sel != k) {
            /* rows are the kinds present, in order */
            return k > fg.split_sel ? BTN_DOWN : BTN_UP;
        }
        return fg.split[k] < goal.counts[k] ? BTN_RIGHT : BTN_LEFT;
    }
    return BTN_A;
}

static int act_buttons(void) {
    FeudMatch *m = &fg.m;
    int kind = FEUD_ACT[fg.act_id].kind;
    if (fg.au == AU_CONFIRM) return BTN_A;
    if (!have_goal) {
        have_goal = feud_ai_next(m, 0, fg.act_id, FD_SKILL_BOT, &goal);
        feud_bot_goal_type = have_goal ? goal.type : 0;
    }
    if (!have_goal) {
        /* nothing more worth doing: finish the action */
        return BTN_B;
    }
    int b;
    switch (goal.type) {
    case ST_ENLIST:
    case ST_BUILD:
        if (fg.au == AU_MAP) return (b = cursor_to(goal.tile)) ? b : BTN_A;
        if (fg.au == AU_MENU) {
            if (fg.cur != goal.tile) return BTN_B;
            if ((b = list_to(fg.msel, goal.idx, BTN_UP, BTN_DOWN))) return b;
            have_goal = false;
            return BTN_A;
        }
        return BTN_B;
    case ST_TRADE:
        if (fg.au != AU_MENU) return BTN_B;
        if ((b = list_to(fg.msel, goal.idx, BTN_UP, BTN_DOWN))) return b;
        have_goal = false;
        return BTN_A;
    case ST_MOVE:
        if (fg.au == AU_MAP) return (b = cursor_to(goal.tile)) ? b : BTN_A;
        if (fg.au == AU_SPLIT) return split_buttons();
        if (fg.au == AU_DEST) {
            if ((b = cursor_to(goal.tile2))) return b;
            have_goal = false;
            return BTN_A;
        }
        return BTN_B;
    case ST_OMEN: {
        const FeudOffer *o;
        feud_offers_of(m, 0, fg.act_id, &o);
        int om = o[goal.idx].what;
        if (fg.au == AU_MENU) return (b = list_to(fg.msel, goal.idx, BTN_UP, BTN_DOWN)) ? b : BTN_A;
        if (fg.au == AU_ANC) {
            if ((b = list_to(fg.anc_sel, goal.anc, BTN_LEFT, BTN_RIGHT))) return b;
            have_goal = false;
            return BTN_A;
        }
        if (fg.au == AU_TARGET) {
            if ((b = cursor_to(goal.tile))) return b;
            if (om != OM_UPDRAFT) have_goal = false;
            return BTN_A;
        }
        if (fg.au == AU_SPLIT) return split_buttons();
        if (fg.au == AU_DEST) {
            if ((b = cursor_to(goal.tile2))) return b;
            have_goal = false;
            return BTN_A;
        }
        return BTN_B;
    }
    default: break;
    }
    (void)kind;
    have_goal = false;
    return 0;
}

static int match_buttons(void) {
    FeudMatch *m = &fg.m;
    switch (fg.mp) {
    case MP_BANNER: return m->side == 0 && fg.mp_t > 14 ? BTN_A : 0;
    case MP_PRAY: {
        if (pray_turn != m->turn) {
            pray_turn = m->turn;
            pray_pick = feud_ai_pick_anc(m, 0, FD_SKILL_BOT, &bot_rng);
            have_goal = false;
        }
        int idx = 0;
        for (int i = 0; i < m->noffer; i++)
            if (m->offer[i] == pray_pick) idx = i;
        int b = list_to(fg.pray_sel, idx, BTN_LEFT, BTN_RIGHT);
        return b ? b : fg.mp_t > 12 ? BTN_A : 0;
    }
    case MP_BLESS: {
        int act = feud_ai_pick_bless(m, 0, fg.bless_anc), idx = 0;
        for (int j = 0; j < 3; j++)
            if (m->deck[fg.bless_anc][j] == act) idx = j;
        int b = list_to(fg.bless_sel, idx, BTN_UP, BTN_DOWN);
        return b ? b : fg.mp_t > 12 ? BTN_A : 0;
    }
    case MP_ACT: return act_buttons();
    default: return 0;
    }
}

static int battle_buttons(void) {
    FeudBattle *b = &fg.b;
    if (b->result != BR_NONE) return b->end_t > 22 ? BTN_A : 0;
    static FeudBattle copy;
    copy = *b;
    copy.order[0] = b->order[0];
    feud_battle_ai_order(&copy, 0);
    if (copy.order[0] < b->order[0]) return BTN_LEFT;
    if (copy.order[0] > b->order[0]) return BTN_RIGHT;
    return 0;
}

static int next_scenario(void) {
    for (int s = 0; s < FD_SCEN; s++)
        if ((s == 0 || (fsv.beaten >> (s - 1) & 1)) && !(fsv.beaten >> s & 1)) return s;
    return 0;
}

static int want(void) {
    switch (fg.state) {
    case FS_TITLE: return fg.title_sel != 0 ? BTN_UP : fg.state_t > 10 ? BTN_A : 0;
    case FS_CAMPAIGN: {
        int b = list_to(fg.camp_sel, next_scenario(), BTN_UP, BTN_DOWN);
        return b ? b : fg.state_t > 10 ? BTN_A : 0;
    }
    case FS_CUSTOM: return fg.cust_sel != 4 ? BTN_DOWN : BTN_A;
    case FS_RECORDS: return BTN_B;
    case FS_STORY: return fg.state_t > 22 ? BTN_A : 0;
    case FS_RESULT: return fg.state_t > 92 ? BTN_A : 0;
    case FS_ENDING: return BTN_A;
    case FS_BATTLE: return battle_buttons();
    case FS_MATCH: return match_buttons();
    default: return 0;
    }
}

int feud_bot_buttons(void) {
    static bool seeded;
    if (!seeded) { rng_seed(&bot_rng, 1212); seeded = true; }
    /* let go every other frame, so each press is a fresh one */
    if (!released) { released = true; return 0; }
    int b = want();
    if (b) released = false;
    return b;
}
