/* HOOPLA - ring nights in the Glass Pit.
 * Cartridge 36 of UFO 40, a tribute to Hyper Contender (UFO 50 #36).
 * See docs/games/36-hoopla.md. This file: the title, the options, the
 * fighter select, the tournament ladder, the draft, exhibition, the
 * endings, the demo, the save and the test hooks. A match itself is in
 * hoop_match.c and hoop_fighters.c. */
#include "hoop.h"

#define SAVE_MAGIC 0x484F5001u
#define ATTRACT_AFTER 720   /* 12 s alone on the title */
#define NOTE_RUNS 20        /* the demo's twentieth showing brings a note */

HoopGame hg;
HoopSave hsv;
Rng hoop_rng;

const HoopFighterDef HOOP_DEF[HOOP_FIGHTERS] = {
    {"TANSY", "KNIFE JUGGLER", "JUMP, AND A SPIN JUMP", "KNIFE: OFF TWO WALLS", C_RED, C_CREAM, C_BLUE, C_ICE, 3},
    {"CLAMP", "DOCK CRANE", "CLAW LINE AND SWING", "COG THAT COMES BACK", C_AMBER, C_SLATE, C_LIME, C_SLATE, 5},
    {"MOSS", "CAVE HERMIT", "TURN GRAVITY OVER", "ROCKET THAT BURSTS", C_LEAF, C_TAN, C_PURPLE, C_TAN, 6},
    {"BRISTLE", "BADGER TRAPPER", "SPRING TRAPS", "THREE QUILLS", C_GREY, C_WHITE, C_BROWN, C_CREAM, 2},
    {"PEWIT", "LAPWING GIRL", "JUMP, THEN FLAP", "HORSESHOE IN AN ARC", C_TEAL, C_WHITE, C_PINK, C_WHITE, 1},
    {"COLLIER", "PIT MINER", "CAGE LIFT", "CHARGE ON A FUSE", C_DUSK, C_YELLOW, C_MAROON, C_YELLOW, 4},
    {"ASTRA", "STAR CHAMPION", "ROCKET PACK", "RAY THAT BOUNCES", C_VIOLET, C_CYAN, C_ORANGE, C_CYAN, 0},
    {"GULP", "BULLFROG", "CHARGED LEAP", "DART, AIMED WHILE HELD", C_JADE, C_YELLOW, C_EARTH, C_YELLOW, 7},
};

static const char *const ENDINGS[HOOP_FIGHTERS] = {
    "THE PURSE PAYS FOR A TENT OF HER OWN. TANSY JUGGLES FOR NOBODY'S DEBTS NOW, AND THE CROWDS FOLLOW HER FROM TOWN TO TOWN.",
    "CLAMP BUYS BACK HIS OLD DOCK AND HANGS THE BELT FROM HIS JIB. THE GULLS HAVE NEVER SEEN A CRANE SO PLEASED WITH ITSELF.",
    "MOSS SPENDS THE WHOLE PURSE ON SOFT BOOTS, SO HE CAN WALK ON THE CAVE ROOF WITHOUT WAKING THE BATS. MONEY WELL SPENT.",
    "BRISTLE SPENDS THE PURSE ON A FORGE AND MAKES SPRING TRAPS FOR EVERY HENHOUSE IN THE VALLEY. THE FOXES HAVE TAKEN UP GARDENING.",
    "PEWIT BUYS A BRASS BELL FOR THE OLD MARSH CHAPEL AND RINGS IT HERSELF, FLAPPING ROUND THE TOWER TILL EVERY HERON IN THE REEDS IS AWAKE.",
    "COLLIER BUYS THE SEAM HE DUG FOR TWENTY YEARS, FLOODS IT AND OPENS IT AS A SWIMMING HOLE. THE FIRST DIVE IS HIS, LAMP HELMET AND ALL.",
    "ASTRA OPENS A FIGHTING SCHOOL ON A MOON NEARBY. LESSON ONE IS THE ROCKET PACK; LESSON TWO IS LANDING. MOST PUPILS LEAVE AFTER LESSON ONE.",
    "GULP HAS A STAGE BUILT ON HIS LILY PAD, WITH LAMPS AND A RED CURTAIN. EVERY NIGHT HE SINGS TO THE POND, AND THE POND CLAPS, MOSTLY SO HE'LL STOP.",
};

const char *hoop_ending_text(int kind) { return ENDINGS[iclamp(kind, 0, HOOP_FIGHTERS - 1)]; }

static bool vita_single(void) { return plat_kind() == PLAT_VITA; }

static void set_state(int s) {
    hg.state = s;
    hg.state_t = 0;
}

/* ---- the save ----------------------------------------------------------------- */

static void save_defaults(void) {
    memset(&hsv, 0, sizeof hsv);
    hsv.magic = SAVE_MAGIC;
    hsv.challenge = 1;
    hsv.to_win = HOOP_DEF_WIN;
    hsv.start_rings = HOOP_DEF_START;
    hsv.every = HOOP_DEF_EVERY;
    hsv.burn = HOOP_DEF_BURN;
    hsv.least_rematches = 0xFFFF;
}

static void load_save(void) {
    HoopSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) {
        hsv = tmp;
        hsv.challenge = (uint8_t)iclamp(hsv.challenge, 0, 2);
        hsv.to_win = (uint8_t)iclamp(hsv.to_win, 3, 9);
        hsv.start_rings = (uint8_t)iclamp(hsv.start_rings, 0, hsv.to_win - 1);
        hsv.every = (uint8_t)iclamp(hsv.every, 5, 30);
        hsv.burn = hsv.burn ? 1 : 0;
        return;
    }
    save_defaults();
}

void hoop_save_now(void) { game_save_write(game_current_index(), &hsv, (int)sizeof hsv); }

static int popcount8(int v) {
    int n = 0;
    for (int i = 0; i < 8; i++) n += (v >> i) & 1;
    return n;
}

static void goals_from_save(void) {
    if (hsv.draft_wins > 0) game_award(GOAL_BEACON);
    if (hsv.champs) game_award(GOAL_SAUCER);
    if (popcount8(hsv.champs) >= 4) game_award(GOAL_ALIEN);
}

/* ---- going places ---------------------------------------------------------------- */

static void pause_pick(int i);
static const char *const PAUSE_ITEMS[] = {"BACK TO THE TITLE"};

static void to_title(void) {
    set_state(HS_TITLE);
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    input_set_versus(false);
    hg.idle_t = 0;
    music_play(HOOP_MUS_TITLE);
}

static void pause_pick(int i) {
    (void)i;
    to_title();
}

static int skill_now(void) { return hsv.challenge == 0 ? SK_CALM : hsv.challenge == 1 ? SK_ROUGH : SK_RIOT; }

static int other_pal(int kind, int pal, int other_kind) { return kind == other_kind ? 1 - pal : 0; }

static void start_match(const int *kinds, const int *pals, const int *ctrls, int n) {
    hoop_match_begin(kinds, pals, ctrls, n, skill_now());
    set_state(HS_MATCH);
    game_set_pausable(hg.mode != -1);
    game_pause_items(1, PAUSE_ITEMS, pause_pick);
    int songs[3] = {HOOP_MUS_FIGHT1, HOOP_MUS_FIGHT2, HOOP_MUS_FIGHT3};
    int song = songs[hg.m.arena % 3];
    if (hg.mode == MODE_TOURNEY && hg.round == HOOP_FIGHTERS - 1) song = HOOP_MUS_MIRROR;
    music_play(song);
    if (hsv.matches < 65535 && hg.state != HS_ATTRACT) hsv.matches++;
}

/* the tournament: the eight, the mirror match last */
static void tourney_begin(int kind, int pal) {
    hg.me = kind;
    hg.me_pal = pal;
    int n = 0;
    for (int k = 0; k < HOOP_FIGHTERS; k++)
        if (k != kind) hg.order[n++] = k;
    for (int i = n - 1; i > 0; i--) {
        int j = rng_range(&hoop_rng, 0, i);
        int t = hg.order[i];
        hg.order[i] = hg.order[j];
        hg.order[j] = t;
    }
    hg.order[HOOP_FIGHTERS - 1] = kind;
    hg.round = 0;
    hg.rematches = 0;
    memset(hg.used_rematch, 0, sizeof hg.used_rematch);
    hsv.used |= (uint8_t)(1 << kind);
    hoop_save_now();
    set_state(HS_LADDER);
    music_play(HOOP_MUS_SELECT);
}

static void tourney_vs(void) {
    int opp = hg.order[hg.round];
    hg.vs_a = hg.me;
    hg.vs_b = opp;
    hg.extra = -1;
    if (hsv.challenge == 2) hg.extra = rng_range(&hoop_rng, 0, HOOP_FIGHTERS - 1);
    set_state(HS_VS);
}

static void tourney_match(void) {
    int opp = hg.order[hg.round];
    int kinds[3] = {hg.me, opp, hg.extra};
    int pals[3] = {hg.me_pal, opp == hg.me ? 1 - hg.me_pal : 0, 0};
    if (hg.extra >= 0) pals[2] = hg.extra == hg.me ? 1 - hg.me_pal : (hg.extra == opp ? 1 - pals[1] : 0);
    int ctrls[3] = {CTRL_P1, CTRL_CPU, CTRL_CPU};
    start_match(kinds, pals, ctrls, hg.extra >= 0 ? 3 : 2);
}

static void draft_begin(void) {
    /* seven of the eight, at random */
    int all[HOOP_FIGHTERS];
    for (int k = 0; k < HOOP_FIGHTERS; k++) all[k] = k;
    for (int i = HOOP_FIGHTERS - 1; i > 0; i--) {
        int j = rng_range(&hoop_rng, 0, i);
        int t = all[i];
        all[i] = all[j];
        all[j] = t;
    }
    hg.npool = 7;
    for (int i = 0; i < 7; i++) hg.pool[i] = all[i];
    /* in fighter order on the board */
    for (int i = 0; i < 7; i++)
        for (int j = i + 1; j < 7; j++)
            if (hg.pool[j] < hg.pool[i]) { int t = hg.pool[i]; hg.pool[i] = hg.pool[j]; hg.pool[j] = t; }
    memset(hg.taken, 0, sizeof hg.taken);
    hg.nteam[0] = hg.nteam[1] = 0;
    hg.draft_turn = 0;
    hg.draft_cur = 0;
    hg.score[0] = hg.score[1] = 0;
    hg.series_used[0] = hg.series_used[1] = 0;
    set_state(HS_DRAFT);
    music_play(HOOP_MUS_DRAFT);
}

/* the snake: P1, P2, P2, P1, P1, P2 */
static const int DRAFT_SIDE[6] = {0, 1, 1, 0, 0, 1};

static void shuffle_queue(int side) {
    for (int i = 0; i < 3; i++) hg.queue[side][i] = i;
    for (int i = 2; i > 0; i--) {
        int j = rng_range(&hoop_rng, 0, i);
        int t = hg.queue[side][i];
        hg.queue[side][i] = hg.queue[side][j];
        hg.queue[side][j] = t;
    }
    hg.qi[side] = 0;
}

static void draft_take(int slot) {
    int side = DRAFT_SIDE[hg.draft_turn];
    hg.taken[slot] = true;
    hg.team[side][hg.nteam[side]++] = hg.pool[slot];
    hg.draft_turn++;
    sfx_play_name("hoop_pick");
    if (hg.draft_turn >= 6) {
        if (hg.players == 1) hsv.used |= (uint8_t)(1 << hg.team[0][0] | 1 << hg.team[0][1] | 1 << hg.team[0][2]);
        shuffle_queue(0);
        shuffle_queue(1);
        set_state(HS_SERIES);
    } else {
        /* the cursor moves to the first free slot */
        for (int i = 0; i < 7; i++)
            if (!hg.taken[(hg.draft_cur + i) % 7]) { hg.draft_cur = (hg.draft_cur + i) % 7; break; }
    }
}

/* the CPU's pick: the best on its list that's left (a careless one on CALM) */
static int cpu_draft_pick(void) {
    int best = -1, bt = 99;
    for (int i = 0; i < 7; i++) {
        if (hg.taken[i]) continue;
        int t = HOOP_DEF[hg.pool[i]].tier + (hsv.challenge == 0 ? rng_range(&hoop_rng, 0, 6) : rng_range(&hoop_rng, 0, 2));
        if (t < bt) { bt = t; best = i; }
    }
    return best;
}

static int series_fighter(int side) { return hg.team[side][hg.queue[side][hg.qi[side]]]; }

static void series_match(void) {
    int a = series_fighter(0), b = series_fighter(1);
    hg.series_used[0] |= 1 << a;
    hg.series_used[1] |= 1 << b;
    int kinds[2] = {a, b};
    int pals[2] = {0, other_pal(b, 0, a)};
    int ctrls[2] = {CTRL_P1, hg.players == 2 ? CTRL_P2 : CTRL_CPU};
    start_match(kinds, pals, ctrls, 2);
}

static void exhib_match(void) {
    int kinds[2] = {hg.pick[0], hg.pick[1]};
    int pals[2] = {hg.pal[0], hg.pal[1]};
    if (kinds[0] == kinds[1] && pals[0] == pals[1]) pals[1] = 1 - pals[0];
    int ctrls[2] = {CTRL_P1, CTRL_P2};
    start_match(kinds, pals, ctrls, 2);
}

static void attract_begin(void) {
    int a = rng_range(&hoop_rng, 0, HOOP_FIGHTERS - 1);
    int b = rng_range(&hoop_rng, 0, HOOP_FIGHTERS - 2);
    if (b >= a) b++;
    int kinds[2] = {a, b}, pals[2] = {0, 0}, ctrls[2] = {CTRL_CPU, CTRL_CPU};
    hg.mode = -1;
    set_state(HS_ATTRACT);
    hoop_match_begin(kinds, pals, ctrls, 2, SK_ROUGH);
    music_play(HOOP_MUS_FIGHT1);
    if (hsv.attract_runs < 65535) hsv.attract_runs++;
    hg.attract_note = hsv.attract_runs == NOTE_RUNS;
    hoop_save_now();
}

/* ---- the end of a match --------------------------------------------------------------- */

static void tourney_won(void) {
    bool counts = hoop_ring_rule_default() && !hg.old_rules;
    if (hsv.tourney_wins < 65535) hsv.tourney_wins++;
    if (counts) {
        hsv.champs |= (uint8_t)(1 << hg.me);
        if (hg.me_pal) hsv.champs_alt |= (uint8_t)(1 << hg.me);
        if (hg.rematches < hsv.least_rematches) hsv.least_rematches = (uint16_t)hg.rematches;
        game_award(GOAL_SAUCER);
        if (popcount8(hsv.champs) >= 4) game_award(GOAL_ALIEN);
    }
    hoop_save_now();
    hg.end_kind = hg.me;
    hg.end_pal = hg.me_pal;
    hg.end_note = hg.me == HF_GULP && hg.me_pal == 1;
    set_state(HS_ENDING);
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    music_play(HOOP_MUS_ENDING);
}

static void match_done(void) {
    int w = hg.m.winner;
    input_consume();
    if (hg.mode == MODE_TOURNEY) {
        if (w == 0) {
            hg.round++;
            if (hg.round >= HOOP_FIGHTERS) {
                tourney_won();
                return;
            }
            set_state(HS_LADDER);
            hg.won_last = true;
            music_play(HOOP_MUS_SELECT);
        } else {
            hg.won_last = false;
            if (!hg.used_rematch[hg.round]) {
                set_state(HS_REMATCH);
                hg.rematch_sel = 0;
            } else {
                set_state(HS_OUT);
            }
            music_play(HOOP_MUS_LOSE);
            game_set_pausable(false);
        }
    } else if (hg.mode == MODE_DRAFT) {
        int side = w == 0 ? 0 : 1;
        hg.score[side]++;
        hg.last_winner = side;
        for (int s = 0; s < 2; s++) {
            hg.qi[s]++;
            if (hg.qi[s] >= 3) shuffle_queue(s);
        }
        if (hg.score[side] >= 5) {
            if (side == 0 && hg.players == 1 && hoop_ring_rule_default() && !hg.old_rules) {
                if (hsv.draft_wins < 65535) hsv.draft_wins++;
                game_award(GOAL_BEACON);
            }
            hoop_save_now();
            set_state(HS_RESULT);
            music_play(side == 0 || hg.players == 2 ? HOOP_MUS_CHAMP : HOOP_MUS_LOSE);
            game_set_pausable(false);
        } else {
            set_state(HS_SERIES);
            music_play(HOOP_MUS_DRAFT);
        }
    } else {
        /* exhibition: back to the select */
        hg.picked[0] = hg.picked[1] = false;
        set_state(HS_SELECT);
        music_play(HOOP_MUS_SELECT);
    }
    hoop_save_now();
}

/* ---- menus --------------------------------------------------------------------------- */

static int any_press(void) {
    return btnp(BTN_A) || btnp(BTN_START) || (hg.players == 2 && btnp2(BTN_A));
}

/* the old rules: DOWN UP DOWN UP LEFT LEFT RIGHT RIGHT on the title */
static const int OLD_CODE[8] = {BTN_DOWN, BTN_UP, BTN_DOWN, BTN_UP, BTN_LEFT, BTN_LEFT, BTN_RIGHT, BTN_RIGHT};

static void title_update(void) {
    game_set_pausable(false);
    static const int DIRS[4] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT};
    for (int d = 0; d < 4; d++) {
        if (!btnp(DIRS[d])) continue;
        hg.idle_t = 0;
        if (DIRS[d] == OLD_CODE[hg.code_i]) hg.code_i++;
        else hg.code_i = DIRS[d] == OLD_CODE[0] ? 1 : 0;
        if (hg.code_i == 8) {
            hg.old_rules = !hg.old_rules;
            hg.code_i = 0;
            sfx_play_name(hg.old_rules ? "hoop_oldon" : "hoop_oldoff");
        }
    }
    if (btn_repeat(BTN_UP)) { hg.title_sel = (hg.title_sel + 2) % 3; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { hg.title_sel = (hg.title_sel + 1) % 3; sfx_play_name("ui_move"); }
    if (input_held()) hg.idle_t = 0;
    if (++hg.idle_t > ATTRACT_AFTER) { attract_begin(); return; }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if ((btnp(BTN_A) || btnp(BTN_START)) && hg.state_t > 8) {
        if (hg.title_sel == 1 && vita_single()) { sfx_play_name("ui_error"); return; }
        sfx_play_name("ui_ok");
        if (hg.title_sel == 2) {
            set_state(HS_OPTIONS);
            hg.opt_sel = 0;
            return;
        }
        hg.players = hg.title_sel == 0 ? 1 : 2;
        hg.mode_sel = 0;
        set_state(HS_MODE);
        input_set_versus(hg.players == 2);
    }
}

static void mode_update(void) {
    if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { hg.mode_sel ^= 1; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { sfx_play_name("ui_back"); to_title(); return; }
    if (btnp(BTN_A) || btnp(BTN_START)) {
        sfx_play_name("ui_ok");
        if (hg.mode_sel == 1) {
            hg.mode = MODE_DRAFT;
            draft_begin();
        } else {
            hg.mode = hg.players == 1 ? MODE_TOURNEY : MODE_EXHIB;
            hg.picked[0] = hg.picked[1] = false;
            hg.cur[1] = HOOP_FIGHTERS - 1;
            set_state(HS_SELECT);
            music_play(HOOP_MUS_SELECT);
        }
    }
}

enum { OPT_CHALLENGE, OPT_ANTIGRAV, OPT_WIN, OPT_START, OPT_EVERY, OPT_BURN, OPT_STANDARD, OPT_BACK, OPT_COUNT };

static void options_update(void) {
    if (btn_repeat(BTN_UP)) { hg.opt_sel = (hg.opt_sel + OPT_COUNT - 1) % OPT_COUNT; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { hg.opt_sel = (hg.opt_sel + 1) % OPT_COUNT; sfx_play_name("ui_move"); }
    int d = btn_repeat(BTN_RIGHT) ? 1 : btn_repeat(BTN_LEFT) ? -1 : 0;
    if (btnp(BTN_A) && hg.opt_sel < OPT_STANDARD) d = 1;
    if (d) {
        switch (hg.opt_sel) {
        case OPT_CHALLENGE: hsv.challenge = (uint8_t)((hsv.challenge + 3 + d) % 3); break;
        case OPT_ANTIGRAV: hsv.antigrav ^= 1; break;
        case OPT_WIN:
            hsv.to_win = (uint8_t)(3 + (hsv.to_win - 3 + 7 + d) % 7);
            if (hsv.start_rings >= hsv.to_win) hsv.start_rings = (uint8_t)(hsv.to_win - 1);
            break;
        case OPT_START: hsv.start_rings = (uint8_t)((hsv.start_rings + 4 + d) % 4); if (hsv.start_rings >= hsv.to_win) hsv.start_rings = 0; break;
        case OPT_EVERY: hsv.every = (uint8_t)(5 + ((hsv.every - 5) / 5 * 5 + 30 + d * 5) % 30); break;
        case OPT_BURN: hsv.burn ^= 1; break;
        default: d = 0; break;
        }
        if (d) sfx_play_name("ui_move");
    }
    if (btnp(BTN_A) && hg.opt_sel == OPT_STANDARD) {
        hsv.to_win = HOOP_DEF_WIN;
        hsv.start_rings = HOOP_DEF_START;
        hsv.every = HOOP_DEF_EVERY;
        hsv.burn = HOOP_DEF_BURN;
        sfx_play_name("ui_ok");
    }
    if (btnp(BTN_B) || (btnp(BTN_A) && hg.opt_sel == OPT_BACK)) {
        sfx_play_name("ui_back");
        hoop_save_now();
        to_title();
    }
}

/* one player's cursor on the select row */
static void select_side(int s, bool (*p)(int), bool (*h)(int), bool (*rep)(int)) {
    if (hg.picked[s]) {
        if (p(BTN_B)) { hg.picked[s] = false; sfx_play_name("ui_back"); }
        return;
    }
    if (rep(BTN_LEFT)) { hg.cur[s] = (hg.cur[s] + HOOP_FIGHTERS - 1) % HOOP_FIGHTERS; sfx_play_name("ui_move"); }
    if (rep(BTN_RIGHT)) { hg.cur[s] = (hg.cur[s] + 1) % HOOP_FIGHTERS; sfx_play_name("ui_move"); }
    /* UP or DOWN swaps the colours when let go; with B pressed meanwhile it
     * was the random pick instead, and the colours stay */
    bool ud = h(BTN_UP) || h(BTN_DOWN);
    if (ud && !hg.ud_held[s]) hg.ud_clean[s] = true;
    if (!ud && hg.ud_held[s] && hg.ud_clean[s]) { hg.pal[s] ^= 1; sfx_play_name("ui_move"); }
    hg.ud_held[s] = ud;
    if (p(BTN_B) && ud) {
        /* UP or DOWN with B: anyone at random */
        hg.ud_clean[s] = false;
        hg.cur[s] = rng_range(&hoop_rng, 0, HOOP_FIGHTERS - 1);
        hg.pick[s] = hg.cur[s];
        hg.picked[s] = true;
        sfx_play_name("hoop_pick");
        return;
    }
    if (p(BTN_A)) {
        hg.pick[s] = hg.cur[s];
        hg.picked[s] = true;
        sfx_play_name("hoop_pick");
    }
}

static bool p1p(int b) { return btnp(b); }
static bool p1h(int b) { return btn(b); }
static bool p1r(int b) { return btn_repeat(b); }
static bool p2p(int b) { return btnp2(b); }
static bool p2h(int b) { return btn2(b); }
static bool p2r(int b) { return btn_repeat2(b); }

static void select_update(void) {
    bool two = hg.mode == MODE_EXHIB;
    if (!hg.picked[0] && btnp(BTN_B) && !btn(BTN_UP) && !btn(BTN_DOWN)) {
        sfx_play_name("ui_back");
        set_state(HS_MODE);
        return;
    }
    select_side(0, p1p, p1h, p1r);
    if (two) select_side(1, p2p, p2h, p2r);
    if (hg.picked[0] && (!two || hg.picked[1]) && hg.state_t > 4) {
        input_consume();
        if (two) {
            hg.vs_a = hg.pick[0];
            hg.vs_b = hg.pick[1];
            hg.extra = -1;
            set_state(HS_VS);
        } else {
            tourney_begin(hg.pick[0], hg.pal[0]);
        }
    }
}

static void draft_update(void) {
    int side = DRAFT_SIDE[hg.draft_turn];
    bool cpu = side == 1 && hg.players == 1;
    if (cpu) {
        if (hg.state_t > 40) {
            int pick = cpu_draft_pick();
            hg.draft_cur = pick;
            draft_take(pick);
            hg.state_t = 0;
        }
        return;
    }
    bool (*p)(int) = side == 0 ? p1p : p2p;
    bool (*r)(int) = side == 0 ? p1r : p2r;
    int dir = r(BTN_RIGHT) ? 1 : r(BTN_LEFT) ? -1 : 0;
    if (dir) {
        for (int i = 1; i <= 7; i++) {
            int c = (hg.draft_cur + dir * i + 14) % 7;
            if (!hg.taken[c]) { hg.draft_cur = c; break; }
        }
        sfx_play_name("ui_move");
    }
    if (p(BTN_A) && !hg.taken[hg.draft_cur] && hg.state_t > 6) {
        draft_take(hg.draft_cur);
        hg.state_t = 0;
    }
    if (btnp(BTN_B) && hg.draft_turn == 0) { sfx_play_name("ui_back"); to_title(); }
}

static void hoop_update(void) {
    hg.state_t++;
    hg.frame_t++;
    switch (hg.state) {
    case HS_TITLE: title_update(); break;
    case HS_MODE: game_set_pausable(false); mode_update(); break;
    case HS_OPTIONS: game_set_pausable(false); options_update(); break;
    case HS_SELECT: game_set_pausable(false); select_update(); break;
    case HS_LADDER:
        game_set_pausable(false);
        if (hg.state_t > 20 && any_press()) { input_consume(); tourney_vs(); }
        break;
    case HS_VS:
        game_set_pausable(false);
        if (hg.state_t > 100 || (hg.state_t > 30 && any_press())) {
            input_consume();
            if (hg.mode == MODE_TOURNEY) tourney_match();
            else if (hg.mode == MODE_DRAFT) series_match();
            else exhib_match();
        }
        break;
    case HS_MATCH:
        game_set_pausable(true);
        hoop_match_update();
        if (hoop_match_over()) match_done();
        else if (hg.m.winner >= 0 && hg.m.win_t == 1) {
            bool mine = hg.mode == MODE_EXHIB || hg.mode == MODE_DRAFT ? true : hg.m.winner == 0;
            if (hg.mode == MODE_DRAFT && hg.players == 1) mine = hg.m.winner == 0;
            music_play(mine ? HOOP_MUS_WIN : HOOP_MUS_LOSE);
        }
        break;
    case HS_REMATCH:
        if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN) || btn_repeat(BTN_LEFT) || btn_repeat(BTN_RIGHT)) {
            hg.rematch_sel ^= 1;
            sfx_play_name("ui_move");
        }
        if (hg.state_t > 30 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            if (hg.rematch_sel == 0) {
                hg.used_rematch[hg.round] = true;
                hg.rematches++;
                sfx_play_name("ui_ok");
                tourney_vs();
                music_play(HOOP_MUS_SELECT);
            } else {
                to_title();
            }
        }
        break;
    case HS_OUT:
    case HS_RESULT:
        if (hg.state_t > 60 && any_press()) { input_consume(); to_title(); }
        break;
    case HS_DRAFT: game_set_pausable(false); draft_update(); break;
    case HS_SERIES:
        game_set_pausable(false);
        if (hg.state_t > 30 && any_press()) {
            input_consume();
            hg.vs_a = series_fighter(0);
            hg.vs_b = series_fighter(1);
            hg.extra = -1;
            set_state(HS_VS);
        }
        if (btnp(BTN_B) && hg.score[0] + hg.score[1] == 0 && hg.state_t > 30) to_title();
        break;
    case HS_ENDING:
        if (hg.state_t > 120 && any_press()) {
            input_consume();
            set_state(HS_CREDITS);
            music_play(HOOP_MUS_CREDITS);
        }
        break;
    case HS_CREDITS:
        if (btn(BTN_A)) hg.state_t += 3;
        if (hg.state_t > 1500) { input_consume(); to_title(); }
        break;
    case HS_ATTRACT:
        game_set_pausable(false);
        hoop_match_update();
        if (input_held() || hoop_match_over() || hg.state_t > 60 * 60) {
            input_consume();
            if (hg.attract_note) {
                hg.attract_note = false;
                set_state(HS_NOTE);
                music_play(HOOP_MUS_TITLE);
            } else {
                to_title();
            }
        }
        break;
    case HS_NOTE:
        if (hg.state_t > 60 && any_press()) { input_consume(); to_title(); }
        break;
    }
}

/* ---- the cartridge ------------------------------------------------------------------- */

static void hoop_load(void) {
    hoop_art_load();
    hoop_audio_load();
}

static void hoop_start(void) {
    load_save();
    memset(&hg, 0, sizeof hg);
    rng_seed(&hoop_rng, (uint64_t)rng_next(&g_rng) + 36u);
    hg.cur[1] = HOOP_FIGHTERS - 1;
    hg.mode = MODE_TOURNEY;
    goals_from_save();
    to_title();
}

static void hoop_quit(void) {
    input_set_versus(false);
    hoop_save_now();
}

static int hoop_query(const char *key, int *out) {
    const Match *m = &hg.m;
    if (!strcmp(key, "bot")) { *out = hoop_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = hg.state; return 1; }
    if (!strcmp(key, "mode")) { *out = hg.mode; return 1; }
    if (!strcmp(key, "players")) { *out = hg.players; return 1; }
    if (!strcmp(key, "title_sel")) { *out = hg.title_sel; return 1; }
    if (!strcmp(key, "opt_sel")) { *out = hg.opt_sel; return 1; }
    if (!strcmp(key, "round")) { *out = hg.round; return 1; }
    if (!strcmp(key, "rematches")) { *out = hg.rematches; return 1; }
    if (!strcmp(key, "me")) { *out = hg.me; return 1; }
    if (!strcmp(key, "me_pal")) { *out = hg.me_pal; return 1; }
    if (!strncmp(key, "order", 5) && isdigit((unsigned char)key[5])) { *out = hg.order[atoi(key + 5) & 7]; return 1; }
    if (!strcmp(key, "cur")) { *out = hg.cur[0]; return 1; }
    if (!strcmp(key, "cur2")) { *out = hg.cur[1]; return 1; }
    if (!strcmp(key, "pal")) { *out = hg.pal[0]; return 1; }
    if (!strcmp(key, "picked")) { *out = hg.picked[0] + 2 * hg.picked[1]; return 1; }
    if (!strcmp(key, "pick")) { *out = hg.pick[0]; return 1; }
    if (!strcmp(key, "extra")) { *out = hg.extra; return 1; }
    if (!strcmp(key, "old_rules")) { *out = hg.old_rules; return 1; }
    if (!strcmp(key, "draft_turn")) { *out = hg.draft_turn; return 1; }
    if (!strcmp(key, "npool")) { *out = hg.npool; return 1; }
    if (!strncmp(key, "pool", 4) && isdigit((unsigned char)key[4])) { *out = hg.pool[atoi(key + 4) % 7]; return 1; }
    if (!strncmp(key, "team", 4) && isdigit((unsigned char)key[4])) {
        /* teamS_I */
        int s = key[4] - '0', i = key[5] == '_' ? atoi(key + 6) : 0;
        *out = s < 2 && i < hg.nteam[s] ? hg.team[s][i] : -1;
        return 1;
    }
    if (!strcmp(key, "nteam0")) { *out = hg.nteam[0]; return 1; }
    if (!strcmp(key, "nteam1")) { *out = hg.nteam[1]; return 1; }
    if (!strcmp(key, "used0")) { *out = hg.series_used[0]; return 1; }
    if (!strcmp(key, "used1")) { *out = hg.series_used[1]; return 1; }
    if (!strcmp(key, "team_mask0") || !strcmp(key, "team_mask1")) {
        int s = key[9] - '0', v = 0;
        for (int i = 0; i < hg.nteam[s]; i++) v |= 1 << hg.team[s][i];
        *out = v;
        return 1;
    }
    if (!strcmp(key, "series_ok")) {
        /* after each round of three, both sides have fielded all three */
        int v[2] = {0, 0};
        for (int s = 0; s < 2; s++)
            for (int i = 0; i < hg.nteam[s]; i++) v[s] |= 1 << hg.team[s][i];
        *out = hg.series_used[0] == v[0] && hg.series_used[1] == v[1];
        return 1;
    }
    if (!strcmp(key, "order_ok")) {
        /* the ladder holds all eight, each once, with the mirror match last */
        int seen = 0;
        for (int i = 0; i < HOOP_FIGHTERS - 1; i++) seen |= 1 << hg.order[i];
        *out = seen == (0xFF & ~(1 << hg.me)) && hg.order[HOOP_FIGHTERS - 1] == hg.me;
        return 1;
    }
    if (!strcmp(key, "pool_ok")) {
        /* seven different fighters in the draft pool */
        int seen = 0;
        for (int i = 0; i < 7; i++) seen |= 1 << hg.pool[i];
        int n = 0;
        for (int k = 0; k < 8; k++) n += (seen >> k) & 1;
        *out = n == 7;
        return 1;
    }
    if (!strcmp(key, "score0")) { *out = hg.score[0]; return 1; }
    if (!strcmp(key, "score1")) { *out = hg.score[1]; return 1; }
    if (!strcmp(key, "vs_a")) { *out = hg.vs_a; return 1; }
    if (!strcmp(key, "vs_b")) { *out = hg.vs_b; return 1; }
    if (!strcmp(key, "end_kind")) { *out = hg.end_kind; return 1; }
    if (!strcmp(key, "end_note")) { *out = hg.end_note; return 1; }
    if (!strcmp(key, "art_bad")) { *out = hoop_art_check(); return 1; }
    if (!strcmp(key, "vita_single")) { *out = vita_single(); return 1; }
    if (!strcmp(key, "versus")) { *out = input_versus(); return 1; }
    /* the match */
    if (!strcmp(key, "arena")) { *out = m->arena; return 1; }
    if (!strcmp(key, "arena_pal")) { *out = m->pal; return 1; }
    if (!strcmp(key, "fighters")) { *out = m->n; return 1; }
    if (!strcmp(key, "winner")) { *out = m->winner; return 1; }
    if (!strcmp(key, "ready")) { *out = m->ready_t; return 1; }
    if (!strcmp(key, "mt")) { *out = m->t; return 1; }
    if (!strcmp(key, "spawn_t")) { *out = m->spawn_t; return 1; }
    if (!strcmp(key, "to_win")) { *out = m->to_win; return 1; }
    if (!strcmp(key, "rings_out")) {
        int n = 0;
        for (int i = 0; i < HOOP_MAX_RINGS; i++) n += m->ring[i].alive;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "loose")) {
        int n = 0;
        for (int i = 0; i < HOOP_MAX_RINGS; i++) n += m->ring[i].alive && m->ring[i].state == RG_LOOSE;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "fresh")) {
        int n = 0;
        for (int i = 0; i < HOOP_MAX_RINGS; i++) n += m->ring[i].alive && m->ring[i].state == RG_FRESH;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "burning")) {
        int n = 0;
        for (int i = 0; i < HOOP_MAX_RINGS; i++) n += m->ring[i].alive && m->ring[i].state == RG_FRESH && m->ring[i].burn_t > 0;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "ledges")) { *out = m->nledges; return 1; }
    if (!strcmp(key, "mines")) {
        int n = 0;
        for (int i = 0; i < HOOP_MAX_MINES; i++) n += m->mine[i].alive;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "armed")) {
        int n = 0;
        for (int i = 0; i < HOOP_MAX_MINES; i++) n += m->mine[i].alive && m->mine[i].arm_t >= HOOP_MINE_ARM;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "blasts")) {
        int n = 0;
        for (int i = 0; i < HOOP_MAX_BLASTS; i++) n += m->blast[i].alive;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "shots", 5)) {
        /* shots: every shot; shotsK: shots of kind K */
        int k = isdigit((unsigned char)key[5]) ? atoi(key + 5) : -1, n = 0;
        for (int i = 0; i < HOOP_MAX_SHOTS; i++) n += m->shot[i].alive && (k < 0 || m->shot[i].kind == k);
        *out = n;
        return 1;
    }
    if (!strncmp(key, "shot_", 5)) {
        /* the first live shot's x, y, vx, vy, owner, bounces, held, stuck, kind */
        const Shot *s = NULL;
        for (int i = 0; i < HOOP_MAX_SHOTS; i++)
            if (m->shot[i].alive) { s = &m->shot[i]; break; }
        const char *u = key + 5;
        if (!s) { *out = -9999; return 1; }
        if (!strcmp(u, "x")) { *out = HPX(s->x); return 1; }
        if (!strcmp(u, "y")) { *out = HPX(s->y); return 1; }
        if (!strcmp(u, "vx")) { *out = s->vx; return 1; }
        if (!strcmp(u, "vy")) { *out = s->vy; return 1; }
        if (!strcmp(u, "owner")) { *out = s->owner; return 1; }
        if (!strcmp(u, "bounces")) { *out = s->bounces; return 1; }
        if (!strcmp(u, "held")) { *out = s->held; return 1; }
        if (!strcmp(u, "stuck")) { *out = s->stuck; return 1; }
        if (!strcmp(u, "kind")) { *out = s->kind; return 1; }
        if (!strcmp(u, "fuse")) { *out = s->fuse; return 1; }
        return 0;
    }
    /* fighters: f0_x, f1_rings ... */
    if (key[0] == 'f' && isdigit((unsigned char)key[1]) && key[2] == '_') {
        int i = key[1] - '0';
        if (i >= HOOP_MAX_IN) return 0;
        const Fighter *f = &m->f[i];
        const char *u = key + 3;
        if (!strcmp(u, "x")) { *out = HPX(f->x); return 1; }
        if (!strcmp(u, "y")) { *out = HPX(f->y); return 1; }
        if (!strcmp(u, "vx")) { *out = f->vx; return 1; }
        if (!strcmp(u, "vy")) { *out = f->vy; return 1; }
        if (!strcmp(u, "kind")) { *out = f->kind; return 1; }
        if (!strcmp(u, "pal")) { *out = f->pal; return 1; }
        if (!strcmp(u, "rings")) { *out = f->rings; return 1; }
        if (!strcmp(u, "hp")) { *out = f->hp; return 1; }
        if (!strcmp(u, "on")) { *out = f->on; return 1; }
        if (!strcmp(u, "g")) { *out = f->g; return 1; }
        if (!strcmp(u, "face")) { *out = f->face; return 1; }
        if (!strcmp(u, "ground")) { *out = f->ground; return 1; }
        if (!strcmp(u, "hurt")) { *out = f->hurt_t; return 1; }
        if (!strcmp(u, "inv")) { *out = f->inv_t; return 1; }
        if (!strcmp(u, "dizzy")) { *out = f->dizzy_t; return 1; }
        if (!strcmp(u, "melee")) { *out = hoop_melee_on(f); return 1; }
        if (!strcmp(u, "blocking")) { *out = f->blocking; return 1; }
        if (!strcmp(u, "jumps")) { *out = f->jumps; return 1; }
        if (!strcmp(u, "spin")) { *out = f->spin_t; return 1; }
        if (!strcmp(u, "claw")) { *out = f->claw; return 1; }
        if (!strcmp(u, "swing")) { *out = f->swing_t; return 1; }
        if (!strcmp(u, "lift")) { *out = f->lift; return 1; }
        if (!strcmp(u, "held_bomb")) { *out = f->held_bomb; return 1; }
        if (!strcmp(u, "charge")) { *out = f->charge_t; return 1; }
        if (!strcmp(u, "glow")) { *out = f->glow_jump; return 1; }
        if (!strcmp(u, "aiming")) { *out = f->aiming; return 1; }
        if (!strcmp(u, "aim")) { *out = hoop_gulp_angle(f); return 1; }
        if (!strcmp(u, "lock")) { *out = f->lock_t; return 1; }
        if (!strcmp(u, "hover")) { *out = f->hover_t; return 1; }
        if (!strcmp(u, "thrust")) { *out = f->thrust_t; return 1; }
        if (!strcmp(u, "goal")) { *out = m->brain[i].goal; return 1; }
        if (!strcmp(u, "hits")) { *out = f->hits_dealt; return 1; }
        if (!strcmp(u, "taken")) { *out = f->rings_taken; return 1; }
        return 0;
    }
    /* the save */
    if (!strcmp(key, "save_champs")) { *out = hsv.champs; return 1; }
    if (!strcmp(key, "save_champs_alt")) { *out = hsv.champs_alt; return 1; }
    if (!strcmp(key, "save_tourney_wins")) { *out = hsv.tourney_wins; return 1; }
    if (!strcmp(key, "save_draft_wins")) { *out = hsv.draft_wins; return 1; }
    if (!strcmp(key, "save_least")) { *out = hsv.least_rematches == 0xFFFF ? -1 : hsv.least_rematches; return 1; }
    if (!strcmp(key, "save_used")) { *out = hsv.used; return 1; }
    if (!strcmp(key, "save_attract")) { *out = hsv.attract_runs; return 1; }
    if (!strcmp(key, "save_challenge")) { *out = hsv.challenge; return 1; }
    if (!strcmp(key, "save_antigrav")) { *out = hsv.antigrav; return 1; }
    if (!strcmp(key, "save_to_win")) { *out = hsv.to_win; return 1; }
    if (!strcmp(key, "save_start")) { *out = hsv.start_rings; return 1; }
    if (!strcmp(key, "save_every")) { *out = hsv.every; return 1; }
    if (!strcmp(key, "save_burn")) { *out = hsv.burn; return 1; }
    if (!strcmp(key, "default_rules")) { *out = hoop_ring_rule_default(); return 1; }
    return 0;
}

static int hoop_cheat(const char *cmd) {
    Match *m = &hg.m;
    int a, b, c, d;
    if (sscanf(cmd, "match %d %d %d", &a, &b, &c) == 3) {
        /* a two-fighter match in arena c: P1 is a, a still CPU target is b */
        int kinds[2] = {a & 7, b & 7}, pals[2] = {0, a == b}, ctrls[2] = {CTRL_P1, CTRL_CPU};
        hg.mode = MODE_EXHIB;
        hg.players = 1;
        hoop_match_begin(kinds, pals, ctrls, 2, SK_ROUGH);
        if (c >= 0 && c < HOOP_ARENAS) { m->arena = c; m->t = 0; hoop_ledges_place(m); }
        set_state(HS_MATCH);
        game_set_pausable(true);
        return 1;
    }
    if (sscanf(cmd, "versus %d %d", &a, &b) == 2) {
        /* a two-player match */
        int kinds[2] = {a & 7, b & 7}, pals[2] = {0, a == b}, ctrls[2] = {CTRL_P1, CTRL_P2};
        hg.mode = MODE_EXHIB;
        hg.players = 2;
        input_set_versus(true);
        hoop_match_begin(kinds, pals, ctrls, 2, SK_ROUGH);
        set_state(HS_MATCH);
        return 1;
    }
    if (sscanf(cmd, "cpu %d", &a) == 1) { m->f[1].ctrl = a ? CTRL_CPU : CTRL_P2; return 1; }
    if (!strcmp(cmd, "cpuboth")) {
        /* CPU against CPU */
        m->f[0].ctrl = CTRL_CPU;
        hoop_brain_init(&m->brain[0], SK_ROUGH, 0);
        return 1;
    }
    if (!strcmp(cmd, "dummy")) { m->f[1].ctrl = CTRL_P2; return 1; } /* nobody on P2: it stands still */
    if (!strcmp(cmd, "go")) { m->ready_t = 0; return 1; }
    if (!strcmp(cmd, "noring")) {
        /* no hoops in play and none coming for a long while */
        for (int i = 0; i < HOOP_MAX_RINGS; i++) m->ring[i].alive = 0;
        m->spawn_t = 1 << 28;
        return 1;
    }
    if (!strcmp(cmd, "spawnring")) { m->spawn_t = 0; return 1; }
    if (sscanf(cmd, "ring %d %d %d", &a, &b, &c) == 3) {
        /* a hoop at (a, b): c = 0 cool, 1 on fire, 2 loose and still */
        for (int i = 0; i < HOOP_MAX_RINGS; i++) {
            Ring *g = &m->ring[i];
            if (g->alive) continue;
            memset(g, 0, sizeof *g);
            g->alive = 1;
            g->claw = -1;
            g->x = a * HQ;
            g->y = b * HQ;
            g->state = c == 2 ? RG_LOOSE : RG_FRESH;
            g->burn_t = c == 1 ? 150 : 0;
            break;
        }
        return 1;
    }
    if (sscanf(cmd, "pos %d %d %d", &a, &b, &c) == 3) {
        /* fighter a's feet at (b, c) */
        Fighter *f = &m->f[iclamp(a, 0, HOOP_MAX_IN - 1)];
        f->x = (b - HOOP_FW / 2) * HQ;
        f->y = (c - HOOP_FH) * HQ;
        f->vx = f->vy = 0;
        f->ground = GND_AIR;
        return 1;
    }
    if (sscanf(cmd, "beside %d %d", &a, &b) == 2) {
        /* fighter a just to the left of fighter b, facing it */
        Fighter *f = &m->f[iclamp(a, 0, 2)], *g = &m->f[iclamp(b, 0, 2)];
        f->x = g->x - (HOOP_FW + 2) * HQ;
        f->y = g->y;
        f->vx = f->vy = 0;
        f->face = 1;
        f->ground = GND_AIR;
        return 1;
    }
    if (sscanf(cmd, "rings %d %d", &a, &b) == 2) { m->f[iclamp(a, 0, 2)].rings = b; return 1; }
    if (sscanf(cmd, "face %d %d", &a, &b) == 2) { m->f[iclamp(a, 0, 2)].face = b < 0 ? -1 : 1; return 1; }
    if (sscanf(cmd, "dizzy %d %d", &a, &b) == 2) { m->f[iclamp(a, 0, 2)].dizzy_t = b; return 1; }
    if (sscanf(cmd, "inv %d %d", &a, &b) == 2) { m->f[iclamp(a, 0, 2)].inv_t = b; return 1; }
    if (sscanf(cmd, "arena %d", &a) == 1) { m->arena = iclamp(a, 0, HOOP_ARENAS - 1); hoop_ledges_place(m); return 1; }
    if (!strcmp(cmd, "noledges")) { m->nledges = 0; m->test_ledges = true; return 1; }
    if (sscanf(cmd, "ledge %d %d %d", &a, &b, &c) == 3) {
        /* a still ledge of the test's own */
        m->test_ledges = true;
        if (m->nledges < HOOP_MAX_LEDGES) m->ledge[m->nledges++] = (Ledge){a, b, c, 0, 0};
        return 1;
    }
    int vx, vy;
    if (sscanf(cmd, "shot %d %d %d %d %d %d", &a, &b, &c, &d, &vx, &vy) == 6) {
        /* a shot of kind a, thrown by b, at (c, d) moving (vx, vy) in 1/256 px */
        hoop_add_shot(a, b, c * HQ, d * HQ, vx, vy);
        return 1;
    }
    if (sscanf(cmd, "challenge %d", &a) == 1) { hsv.challenge = (uint8_t)iclamp(a, 0, 2); return 1; }
    if (sscanf(cmd, "towin %d", &a) == 1) { hsv.to_win = (uint8_t)iclamp(a, 3, 9); return 1; }
    if (sscanf(cmd, "every %d", &a) == 1) { hsv.every = (uint8_t)iclamp(a, 5, 30); return 1; }
    if (sscanf(cmd, "antigrav %d", &a) == 1) { hsv.antigrav = a ? 1 : 0; return 1; }
    if (sscanf(cmd, "oldrules %d", &a) == 1) { hg.old_rules = a != 0; return 1; }
    if (sscanf(cmd, "botplan %d", &a) == 1) { hg.bot_plan = a & 7; return 1; }
    if (sscanf(cmd, "botdraft %d", &a) == 1) { hg.bot_draft = a; return 1; }
    if (sscanf(cmd, "tourney %d %d", &a, &b) == 2) {
        /* straight to the ladder with fighter a (colours b) */
        hg.mode = MODE_TOURNEY;
        hg.players = 1;
        tourney_begin(a & 7, b & 1);
        return 1;
    }
    if (sscanf(cmd, "round %d", &a) == 1) { hg.round = iclamp(a, 0, HOOP_FIGHTERS - 1); return 1; }
    if (!strcmp(cmd, "winmatch")) {
        /* P1 takes the match on the spot */
        m->f[0].rings = m->to_win;
        m->winner = 0;
        m->win_t = 1000;
        return 1;
    }
    if (!strcmp(cmd, "losematch")) {
        m->f[1].rings = m->to_win;
        m->winner = 1;
        m->win_t = 1000;
        return 1;
    }
    if (sscanf(cmd, "attract %d", &a) == 1) { hsv.attract_runs = (uint16_t)a; return 1; }
    if (!strcmp(cmd, "idle")) { hg.idle_t = ATTRACT_AFTER; return 1; }
    return 0;
}

const GameDef GAME_HOOPLA = {
    "hoopla",
    "HOOPLA",
    "1988",
    "PLATFORM BRAWLER",
    "HOLD FIVE HOOPS AT ONCE TO WIN. EIGHT FIGHTERS, EIGHT WAYS TO MOVE.",
    {"TAKE A DRAFT BATTLE ON STANDARD HOOPS", "CHAMPION OF THE RING NIGHTS ON STANDARD HOOPS",
     "BE CHAMPION AS FOUR DIFFERENT FIGHTERS"},
    "D-PAD\tWALK; HOLD DOWN TO BLOCK\n"
    GLYPH_A "\tYOUR WEAPON\n"
    "UP+" GLYPH_A "\tTHE OTHER SHOT\n"
    "DOWN+" GLYPH_A "\tLUNGE (BEATS SHOTS)\n"
    GLYPH_B "\tYOUR OWN WAY OF MOVING\n"
    "DOWN+" GLYPH_B "\tDROP THROUGH A LEDGE\n"
    "START\tPAUSE",
    C_TEAL, C_YELLOW,
    hoop_load, hoop_start, hoop_update, hoop_draw, hoop_quit, hoop_draw_label, hoop_query, hoop_cheat,
    "HYPER CONTENDER", 36,
    NULL,
};
