/* SKID KIDS - two-on-two beanbag dodgeball in the school gym.
 * Cartridge 26 of UFO 40, a tribute to Hot Foot (UFO 50 #26).
 * See docs/games/26-skid-kids.md. The match rules live in
 * skidkids_match.c, the CPU in skidkids_ai.c, the court's drawing in
 * skidkids_draw.c; this file is the menus, the draft, the tournament, the
 * codes, the ending and the test hooks. */
#include "skidkids.h"
#include <ctype.h>
#include <stddef.h>

enum {
    S_TITLE, S_TEAMTYPE, S_DRAFT, S_BUILD, S_COACH, S_BRACKET, S_TALK, S_MATCH, S_RESULT, S_GAMEOVER,
    S_ENDING, S_CREDITS, S_RECORDS, S_CODES, S_RULES, S_DEMO
};
enum { MODE_1P, MODE_COOP, MODE_VS };
#define TITLE_ITEMS 5
#define DEMO_IDLE 720 /* frames on the title before the demo plays */
#define ROUNDS 6

/* ---- the save: the menu stats and a few totals -------------------------------- */
typedef struct Save {
    uint32_t magic;
    uint16_t used, won_with; /* athletes used, athletes won with (a bit per kid) */
    uint8_t best_wins, cups, blowout_cups, pad0;
    uint32_t matches_won;
} Save;
#define SAVE_MAGIC 0x534B4401u

/* ---- the codes (our own, entered from the title's CODES screen) ---------------- */
enum { CODE_ALLSTARS = 1, CODE_SWAPONLY = 2, CODE_HORNETS = 4, CODE_GYMRULES = 8, CODE_DUELRULE = 16 };
static const struct { const char *code; int bit; const char *says; } CODES[] = {
    {"ALLSTARS", CODE_ALLSTARS, "THE KANGAROO AND THE ROBOT JOIN BUILD TEAM!"},
    {"SWAPONLY", CODE_SWAPONLY, "B ONLY SWAPS KIDS. A DOES THE REST."},
    {"HORNETS!", CODE_HORNETS, "2P CO-OP: THE KANGAROO AND THE ROBOT!"},
    {"GYMRULES", CODE_GYMRULES, "HOUSE RULES FOR 1P AND CO-OP."},
    {"DUELRULE", CODE_DUELRULE, "HOUSE RULES FOR VERSUS."},
};
#define CODE_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!"

typedef struct Line {
    int who;  /* a roster kid, or -1 the Coach, -2 nobody (a caption) */
    int team; /* the bib it wears */
    const char *text;
} Line;

typedef struct Part {
    float x, y, vx, vy;
    int life, col;
} Part;

static Save sv;
static Match M;
static Rng seeds;
static int state, state_t, frame_t, title_sel, type_sel, mode, shake;
static bool build_mode, sheet_mode;
static unsigned codes;
static char code_buf[9];
static int code_pos, code_msg_t;
static const char *code_msg;
static Rules rules_tour, rules_duel;
static int rules_sel, rules_which;
/* team selection */
static int pool[5], owner[5], draft_phase, dcur[2], cpu_t, n_took[2];
static int team_a[2], team_b[2], leftover = -1;
static int bcur[2], bpick[2][2], bn[2];
/* the tournament */
static int opp[ROUNDS][2], round_i, wins, result_win, result_blowout, result_close;
static int round_score[ROUNDS][2]; /* each match's final score */
static bool all_blowouts, coach_seen;
static int coach_page;
/* the talk before a match */
static Line talk[10];
static int n_talk, talk_i;
static bool talk_special;
/* the ending and credits */
static int ending_i;
/* the demo and the secrets */
static int idle_t, p2_up_t;
static bool note_shown, nines_shown;
static uint32_t freeze_mask, pin_mask; /* tests: kids with no input at all; kids that can't walk */
static Part parts[96];

static bool vita_single(void) { return plat_kind() == PLAT_VITA; }
static bool codes_on(void) { return codes != 0; }

static void part_add(float x, float y, float vx, float vy, int life, int col) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) {
            parts[i] = (Part){x, y, vx, vy, life, col};
            return;
        }
}

/* ------------------------------------------------------------------ */
/* save and goals (a code turns both off until the cartridge is left)  */

static void save_now(void) {
    if (codes_on()) return;
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else {
        memset(&sv, 0, sizeof sv);
        sv.magic = SAVE_MAGIC;
    }
    sv.used &= 0x0FFF;
    sv.won_with &= 0x0FFF;
}

static void award(int bit) {
    if (!codes_on()) game_award(bit);
}

static int bits12(unsigned v) {
    int n = 0;
    for (int i = 0; i < SKID_KIDS; i++) n += (v >> i) & 1;
    return n;
}

/* ------------------------------------------------------------------ */
/* flow                                                                 */

static uint64_t new_seed(void) { return rng_next(&seeds) ^ ((uint64_t)rng_next(&seeds) << 32); }

static void set_state(int s) {
    state = s;
    state_t = 0;
}

static void go_title(void) {
    set_state(S_TITLE);
    idle_t = 0;
    input_set_versus(false);
    game_set_pausable(false);
    music_play(SKID_MUS_TITLE);
}

static void shuffle(int *a, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rng_range(&seeds, 0, i);
        int t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

static void go_teamtype(int m) {
    mode = m;
    type_sel = 0;
    set_state(S_TEAMTYPE);
    input_set_versus(mode != MODE_1P);
    game_set_pausable(false);
    music_play(SKID_MUS_PICK);
}

static void start_draft(void) {
    int all[SKID_KIDS];
    for (int i = 0; i < SKID_KIDS; i++) all[i] = i;
    shuffle(all, SKID_KIDS);
    for (int i = 0; i < 5; i++) {
        pool[i] = all[i];
        owner[i] = -1;
    }
    draft_phase = 0;
    n_took[0] = n_took[1] = 0;
    dcur[0] = 0;
    dcur[1] = 4;
    cpu_t = 0;
    leftover = -1;
    p2_up_t = 0;
    note_shown = false;
    build_mode = false;
    set_state(S_DRAFT);
}

static void start_build(void) {
    bcur[0] = 0;
    bcur[1] = 1;
    bn[0] = bn[1] = 0;
    leftover = -1;
    build_mode = true;
    set_state(S_BUILD);
}

static int build_slots(void) { return (codes & CODE_ALLSTARS) ? SKID_ROSTER : SKID_KIDS; }

/* The six matches. From a draft, the CPU's two picks are the first team you
 * meet, the seven kids who weren't in the draft fill matches 2 to 4 and the
 * kangaroo's side, and the kid nobody picked comes back with a robot. */
static void setup_tournament(void) {
    int rest[SKID_ROSTER], n = 0;
    if (!build_mode) {
        for (int k = 0; k < SKID_KIDS; k++) {
            bool in_pool = false;
            for (int i = 0; i < 5; i++) in_pool |= pool[i] == k;
            if (!in_pool) rest[n++] = k;
        }
        shuffle(rest, n);
        opp[0][0] = team_b[0];
        opp[0][1] = team_b[1];
        for (int r = 1; r < 4; r++) {
            opp[r][0] = rest[(r - 1) * 2];
            opp[r][1] = rest[(r - 1) * 2 + 1];
        }
        opp[4][0] = rest[6];
    } else {
        for (int k = 0; k < SKID_KIDS; k++)
            if (k != team_a[0] && k != team_a[1]) rest[n++] = k;
        shuffle(rest, n);
        leftover = rest[0];
        for (int r = 0; r < 4; r++) {
            opp[r][0] = rest[1 + r * 2];
            opp[r][1] = rest[2 + r * 2];
        }
        opp[4][0] = rest[9];
    }
    opp[4][1] = K_BOOMER;
    opp[5][0] = leftover;
    opp[5][1] = K_BENCHBOT;
    round_i = 0;
    wins = 0;
    all_blowouts = true;
    input_set_versus(mode == MODE_COOP);
    if (!coach_seen) {
        coach_page = 0;
        set_state(S_COACH);
        music_play(SKID_MUS_PICK);
    } else {
        set_state(S_BRACKET);
        music_play(SKID_MUS_BRACKET);
    }
}

static void add_line(int who, int team, const char *text) {
    if (n_talk < ARRAY_LEN(talk)) talk[n_talk++] = (Line){who, team, text};
}

static bool has(const int *t, int who) { return t[0] == who || t[1] == who; }

/* Who says what before a match: the other side's lead, some pairs'
 * exchanges, and your kids when the other side is odd. */
static void build_talk(void) {
    n_talk = 0;
    talk_i = 0;
    talk_special = false;
    const int *them = mode == MODE_VS ? team_b : opp[round_i];
    int lead = them[0];
    if (mode != MODE_VS && round_i == 5) {
        add_line(leftover, 1, "REMEMBER ME? YOU LEFT ME ON THE BENCH. SO I BUILT A FRIEND.");
        add_line(K_BENCHBOT, 1, SKID_KID[K_BENCHBOT].line);
        if (has(team_a, K_MILO) && has(team_a, K_SPARKY)) {
            /* a secret: the robot is built from their science fair projects */
            add_line(K_SPARKY, 0, "WAIT. IS THAT MY SCIENCE FAIR VOLCANO IN ITS CHEST?");
            add_line(K_MILO, 0, "AND MY SOLAR CALCULATOR!");
            add_line(K_BENCHBOT, 1, "THANK YOU FOR YOUR DONATIONS.");
            talk_special = true;
        } else {
            add_line(team_a[0], 0, "WHERE DID YOU GET A ROBOT?!");
        }
        return;
    }
    if (mode != MODE_VS && round_i == 4) {
        add_line(them[0], 1, "HE CAME ON THE FIELD TRIP AND NEVER WENT HOME.");
        add_line(K_BOOMER, 1, SKID_KID[K_BOOMER].line);
        add_line(team_a[1], 0, "IS THAT... A KANGAROO?");
        return;
    }
    add_line(lead, 1, SKID_KID[lead].line);
    if (lead < SKID_KIDS) {
        /* some pairs have words for each other */
        if (has(team_a, K_SID) && has(them, K_BUZZY)) {
            add_line(K_BUZZY, 1, "COACH SAYS YOU HAVE TO STAY ON YOUR SIDE, SID.");
            add_line(K_SID, 0, "COACH SAYS A LOT OF THINGS.");
        } else if (has(team_a, K_ROXIE) && has(them, K_KIKI)) {
            add_line(K_KIKI, 1, "TWO BAGS, ROXIE. TWO.");
            add_line(K_ROXIE, 0, "TWO FEET. FASTER ONES.");
        } else if (has(team_a, K_MOOSE) && has(them, K_PIPPA)) {
            add_line(K_PIPPA, 1, "GO ON, KNOCK ME OVER. I'LL JUST GET UP.");
            add_line(K_MOOSE, 0, "...THAT'S ANNOYING.");
        } else if (has(team_a, K_NELL) && has(them, K_NOODLE)) {
            add_line(K_NOODLE, 1, "NELL! HALF YOUR STARS? FOR OLD TIMES' SAKE?");
            add_line(K_NELL, 0, "ONLY FOR MY PARTNER, NOODLE.");
        } else if (has(them, K_BUZZY)) {
            add_line(team_a[0], 0, "WHY DOES BUZZY GET ALL THE JUICE BOXES?");
        }
    }
}

static int match_music(void) {
    if (state == S_DEMO) return SKID_MUS_MATCH1;
    if (mode == MODE_VS) return SKID_MUS_MATCH2;
    if (round_i == 4) return SKID_MUS_ROO;
    if (round_i == 5) return SKID_MUS_BOT;
    return round_i % 2 ? SKID_MUS_MATCH2 : SKID_MUS_MATCH1;
}

static void start_talk(void) {
    build_talk();
    set_state(S_TALK);
    input_set_versus(mode != MODE_1P);
}

static void mark_used(int who) {
    if (codes_on()) return;
    if (who >= 0 && who < SKID_KIDS) sv.used |= (uint16_t)(1u << who);
}

static void start_match(void) {
    int who[4];
    Rules r;
    skid_rules_default(&r);
    if (mode == MODE_VS && (codes & CODE_DUELRULE)) r = rules_duel;
    if (mode != MODE_VS && (codes & CODE_GYMRULES)) r = rules_tour;
    who[0] = team_a[0];
    who[1] = team_a[1];
    if (mode == MODE_VS) {
        who[2] = team_b[0];
        who[3] = team_b[1];
    } else {
        who[2] = opp[round_i][0];
        who[3] = opp[round_i][1];
    }
    skid_match_init(&M, who, &r, new_seed());
    M.player[0] = CTRL_P1;
    M.player[1] = mode == MODE_VS ? CTRL_P2 : CTRL_AI;
    M.coop = mode == MODE_COOP;
    M.swap_only = (codes & CODE_SWAPONLY) != 0;
    M.level[1] = round_i;
    mark_used(team_a[0]);
    mark_used(team_a[1]);
    if (mode == MODE_VS) {
        mark_used(team_b[0]);
        mark_used(team_b[1]);
    }
    memset(parts, 0, sizeof parts);
    freeze_mask = pin_mask = 0;
    set_state(S_MATCH);
    input_set_versus(mode != MODE_1P);
    game_set_pausable(true);
    music_play(match_music());
}

static void start_demo(void) {
    int all[SKID_KIDS];
    for (int i = 0; i < SKID_KIDS; i++) all[i] = i;
    shuffle(all, SKID_KIDS);
    Rules r;
    skid_rules_default(&r);
    skid_match_init(&M, all, &r, new_seed());
    M.player[0] = M.player[1] = CTRL_AI;
    M.endless = true;
    M.level[0] = M.level[1] = 3;
    nines_shown = false;
    memset(parts, 0, sizeof parts);
    set_state(S_DEMO);
    game_set_pausable(false);
    music_play(SKID_MUS_MATCH1);
}

static void finish_match(void) {
    result_win = M.winner == 0;
    int loser = M.score[1 - M.winner];
    result_blowout = loser <= M.rules.goal - 6;
    result_close = loser >= M.rules.goal - 2;
    game_set_pausable(false);
    set_state(S_RESULT);
    bool keep = !codes_on(); /* a code leaves the records (and the goals) alone */
    if (mode != MODE_VS) {
        round_score[round_i][0] = M.score[0];
        round_score[round_i][1] = M.score[1];
    }
    if (mode != MODE_VS && result_win) {
        wins++;
        if (!result_blowout) all_blowouts = false;
        if (keep) {
            sv.matches_won++;
            if (wins > sv.best_wins) sv.best_wins = (uint8_t)wins;
        }
        if (wins >= 3) award(GOAL_BEACON);
        if (round_i == ROUNDS - 1) {
            if (keep && sv.cups < 255) sv.cups++;
            for (int i = 0; i < 2 && keep; i++)
                if (team_a[i] < SKID_KIDS) sv.won_with |= (uint16_t)(1u << team_a[i]);
            award(GOAL_SAUCER);
            if (all_blowouts) {
                if (keep && sv.blowout_cups < 255) sv.blowout_cups++;
                award(GOAL_ALIEN);
            }
        }
    }
    save_now();
    music_restart(result_win || mode == MODE_VS ? SKID_MUS_WIN : SKID_MUS_LOSE);
}

/* ------------------------------------------------------------------ */
/* input                                                                */

static Pad human_pad(int who) {
    Pad p;
    memset(&p, 0, sizeof p);
    bool (*h)(int) = who ? btn2 : btn;
    bool (*pr)(int) = who ? btnp2 : btnp;
    p.dx = (int8_t)(h(BTN_RIGHT) - h(BTN_LEFT));
    p.dy = (int8_t)(h(BTN_DOWN) - h(BTN_UP));
    p.a = h(BTN_A);
    p.ap = pr(BTN_A);
    p.b = h(BTN_B);
    p.bp = pr(BTN_B);
    p.br = btnr(who ? BTN_B << BTN_P2_SHIFT : BTN_B);
    return p;
}

static void fx_from_match(void) {
    uint32_t e = M.ev;
    M.ev = 0;
    if (!e) return;
    float x = M.ev_x, y = M.ev_y;
    if (e & EV_THROW) sfx_play_name("skid_throw");
    if (e & EV_PASS) sfx_play_name("skid_pass");
    if (e & EV_WALL) sfx_play_name("skid_wall");
    if (e & EV_PUSH) sfx_play_name("skid_push");
    if (e & EV_BOOM) {
        sfx_play_name("skid_boom");
        shake = imax(shake, 10);
        for (int i = 0; i < 16; i++) {
            float a = i * 0.3927f;
            part_add(x, y - 3, cosf(a) * 2.0f, sinf(a) * 1.3f - 0.4f, 16, i % 2 ? C_ORANGE : C_YELLOW);
        }
    }
    if (e & EV_STOMP) { sfx_play_name("skid_stomp"); shake = imax(shake, 20); }
    if (e & EV_GUST) sfx_play_name("skid_gust");
    if (e & EV_CLANK) sfx_play_name("skid_clank");
    if (e & EV_SPLASH) {
        sfx_play_name("skid_splash");
        for (int i = 0; i < 8; i++) part_add(x, y - 2, (i - 3.5f) * 0.4f, -1.2f - (i % 3) * 0.3f, 14, C_ICE);
    }
    if (e & EV_HIT) {
        sfx_play_name("skid_hit");
        shake = imax(shake, 3);
        for (int i = 0; i < 6; i++) part_add(x, y - 10, (i - 2.5f) * 0.5f, -1.0f - (i % 2) * 0.5f, 12, C_WHITE);
    } else if (e & EV_DOWN) sfx_play_name("skid_down");
    if (e & EV_SLIP) sfx_play_name("skid_slip");
    if (e & EV_SPECIAL) sfx_play_name("skid_special");
    if (e & EV_MOVE) sfx_play_name("skid_move");
    if (e & EV_REEL) sfx_play_name("skid_reel");
    if (e & EV_HALFSTAR) {
        sfx_play_name("skid_half");
        for (int i = 0; i < 4; i++) part_add(x + (i - 1.5f) * 3, y - 18, 0, -0.6f, 14, C_YELLOW);
    } else if (e & EV_DRINK) sfx_play_name("skid_drink");
    else if (e & EV_PICK) sfx_play_name("skid_pick");
    else if (e & EV_SWAP) sfx_play_name("skid_swap");
    else if (e & EV_CANCEL) sfx_play_name("skid_cancel");
    else if (e & EV_FORCED) sfx_play_name("skid_forced");
    else if (e & EV_JUMP) sfx_play_name("skid_jump");
    if (e & EV_COACH) sfx_play_name("skid_coach");
    if (e & EV_WHISTLE) sfx_play_name("skid_whistle");
}

static bool bot_a_prev, bot_b_prev, bot_b_queue, bot_a_queue;

static void run_match(bool endless) {
    Pad pads[4];
    for (int i = 0; i < 4; i++) {
        int team = i / 2;
        if (!endless && M.coop && team == 0) pads[i] = human_pad(i);
        else if (!endless && M.player[team] != CTRL_AI && M.ctrl[team] == i) pads[i] = human_pad(M.player[team] == CTRL_P2);
        else skid_ai_pad(&M, i, &pads[i]);
        if (freeze_mask & (1u << i)) memset(&pads[i], 0, sizeof pads[i]);
        if (pin_mask & (1u << i)) pads[i].dx = pads[i].dy = 0;
    }
    skid_match_update(&M, pads);
    fx_from_match();
}

/* ------------------------------------------------------------------ */
/* the screens' logic                                                   */

static void update_title(void) {
    game_set_pausable(false);
    if (code_msg_t > 0) code_msg_t--;
    if (input_held() & 0xFF) idle_t = 0;
    else if (++idle_t >= DEMO_IDLE) {
        start_demo();
        return;
    }
    if (btn_repeat(BTN_UP)) { title_sel = (title_sel + TITLE_ITEMS - 1) % TITLE_ITEMS; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { title_sel = (title_sel + 1) % TITLE_ITEMS; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if (state_t < 10 || !(btnp(BTN_A) || btnp(BTN_START))) return;
    if ((title_sel == 1 || title_sel == 2) && vita_single()) { sfx_play_name("ui_error"); return; }
    sfx_play_name("ui_ok");
    switch (title_sel) {
    case 0: go_teamtype(MODE_1P); break;
    case 1:
        if (codes & CODE_HORNETS) {
            /* the code's team: the kangaroo and the robot, straight in */
            mode = MODE_COOP;
            build_mode = true;
            team_a[0] = K_BOOMER;
            team_a[1] = K_BENCHBOT;
            music_play(SKID_MUS_PICK);
            setup_tournament();
        } else {
            go_teamtype(MODE_COOP);
        }
        break;
    case 2: go_teamtype(MODE_VS); break;
    case 3: set_state(S_RECORDS); break;
    default:
        memset(code_buf, 'A', 8);
        code_buf[8] = 0;
        code_pos = 0;
        code_msg = NULL;
        code_msg_t = 0;
        set_state(S_CODES);
        break;
    }
}

static void update_teamtype(void) {
    if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { type_sel ^= 1; sfx_play_name("ui_move"); }
    if (btnp(BTN_B) && state_t > 6) { sfx_play_name("ui_back"); go_title(); return; }
    if (state_t > 6 && (btnp(BTN_A) || btnp(BTN_START))) {
        sfx_play_name("ui_ok");
        if (type_sel == 0) start_draft();
        else start_build();
    }
}

static int draft_left(void) {
    int n = 0;
    for (int i = 0; i < 5; i++) n += owner[i] < 0;
    return n;
}

static void draft_move(int *cur, int d) {
    for (int k = 0; k < 5; k++) {
        *cur = (*cur + d + 5) % 5;
        if (owner[*cur] < 0) return;
    }
}

/* the CPU's pick: the kid it likes best of those left */
static int cpu_pick(void) {
    static const int8_t LIKES[SKID_KIDS] = {6, 5, 6, 7, 7, 8, 7, 8, 6, 7, 5, 9};
    int best = -1, bs = -1;
    for (int i = 0; i < 5; i++) {
        if (owner[i] >= 0) continue;
        int s = LIKES[pool[i]] * 10 + rng_range(&seeds, 0, 12);
        if (s > bs) { bs = s; best = i; }
    }
    return best;
}

/* a pick: the teams fill in pick order (in co-op, player 1's kid first) */
static void draft_take(int i, int who_owns) {
    owner[i] = who_owns;
    if (n_took[who_owns] < 2) {
        if (who_owns == 0) team_a[n_took[0]++] = pool[i];
        else team_b[n_took[1]++] = pool[i];
    }
    sfx_play_name("ui_ok");
}

static void update_draft(void) {
    /* a secret: player 2 holding UP here for thirty seconds finds a note */
    if (mode == MODE_VS && btn2(BTN_UP)) {
        if (++p2_up_t >= 1800 && !note_shown) {
            note_shown = true;
            sfx_play_name("ui_toast");
        }
    } else {
        p2_up_t = 0;
    }
    if (btnp(BTN_B) && draft_phase == 0 && state_t > 6) { sfx_play_name("ui_back"); go_teamtype(mode); return; }
    if (draft_phase == 3) {
        if (state_t == 1) sfx_play_name("skid_cry");
        if (state_t > 40 && (btnp(BTN_A) || btnp(BTN_START) || state_t > 200)) {
            for (int i = 0; i < 5; i++)
                if (owner[i] < 0) leftover = pool[i];
            if (mode == MODE_VS) start_talk();
            else setup_tournament();
        }
        return;
    }
    /* who chooses now: 0 player 1, 1 player 2, 2 the CPU */
    int chooser = draft_phase == 0 ? 0 : draft_phase == 1 ? (mode == MODE_VS ? 1 : 2) : (mode == MODE_COOP ? 1 : 0);
    if (chooser == 2) {
        if (++cpu_t % 40 == 0) {
            int i = cpu_pick();
            draft_take(i, 1);
            if (draft_left() == 2) { draft_phase = 2; state_t = 0; draft_move(&dcur[mode == MODE_COOP ? 1 : 0], 0); }
        }
        return;
    }
    bool (*rep)(int) = chooser ? btn_repeat2 : btn_repeat;
    bool (*pr)(int) = chooser ? btnp2 : btnp;
    int *cur = &dcur[chooser];
    if (owner[*cur] >= 0) draft_move(cur, 1);
    int d = rep(BTN_RIGHT) - rep(BTN_LEFT);
    if (d) { draft_move(cur, d); sfx_play_name("ui_move"); }
    if (state_t > 8 && pr(BTN_A)) {
        int side = draft_phase == 1 ? 1 : 0; /* phase 1 is the other team's picks */
        draft_take(*cur, side);
        if (draft_phase == 0) {
            draft_phase = 1;
            state_t = 0;
            cpu_t = 0;
            draft_move(&dcur[1], 0);
        } else if (draft_phase == 1 && draft_left() == 2) {
            draft_phase = 2;
            state_t = 0;
        } else if (draft_phase == 2) {
            draft_phase = 3;
            state_t = 0;
        }
    }
}

/* each kid is one kid: nobody can be on both teams, or picked twice */
static bool build_taken(int who) {
    for (int p = 0; p < 2; p++)
        for (int k = 0; k < bn[p]; k++)
            if (bpick[p][k] == who) return true;
    return false;
}

static void update_build(void) {
    int slots = build_slots();
    int players = mode == MODE_1P ? 1 : 2;
    int need = mode == MODE_COOP ? 1 : 2;
    for (int p = 0; p < players; p++) {
        bool (*rep)(int) = p ? btn_repeat2 : btn_repeat;
        bool (*pr)(int) = p ? btnp2 : btnp;
        if (bn[p] < need) {
            int c = bcur[p] % 7, r = bcur[p] / 7, rows = (slots + 6) / 7;
            int dx = rep(BTN_RIGHT) - rep(BTN_LEFT), dy = rep(BTN_DOWN) - rep(BTN_UP);
            if (dx || dy) {
                c = (c + dx + 7) % 7;
                r = (r + dy + rows) % rows;
                int n = r * 7 + c;
                if (n >= slots) n = slots - 1;
                bcur[p] = n;
                sfx_play_name("ui_move");
            }
            if (state_t > 8 && pr(BTN_A)) {
                if (build_taken(bcur[p])) sfx_play_name("ui_error");
                else {
                    bpick[p][bn[p]++] = bcur[p];
                    sfx_play_name("ui_ok");
                }
            }
        }
        if (state_t > 8 && pr(BTN_B)) {
            if (bn[p] > 0) { bn[p]--; sfx_play_name("ui_back"); }
            else if (p == 0) { sfx_play_name("ui_back"); go_teamtype(mode); return; }
        }
    }
    bool done = bn[0] >= need && (players == 1 || bn[1] >= need);
    if (!done) return;
    if (mode == MODE_1P) {
        team_a[0] = bpick[0][0];
        team_a[1] = bpick[0][1];
    } else if (mode == MODE_COOP) {
        team_a[0] = bpick[0][0];
        team_a[1] = bpick[1][0];
    } else {
        team_a[0] = bpick[0][0];
        team_a[1] = bpick[0][1];
        team_b[0] = bpick[1][0];
        team_b[1] = bpick[1][1];
    }
    if (mode == MODE_VS) start_talk();
    else setup_tournament();
}

#define COACH_PAGES 6
static const char *const COACH_TALK[COACH_PAGES] = {
    "LISTEN UP! TWO ON TWO, BEANBAGS ONLY. HIT A KID ON THE OTHER TEAM WITH A SLIDING BAG AND YOUR TEAM GETS A POINT. FIRST TO 15.",
    "TAP " GLYPH_B " BY A BAG TO GRAB IT. HOLD " GLYPH_B " TO WIND UP, AIM WITH THE PAD, LET GO TO THROW. A FULL WIND-UP KNOCKS THEM FLAT.",
    "A QUICK TAP-AND-LET-GO JUST TOSSES IT UP BY YOUR FEET. TAP " GLYPH_B " WITH A BAG TO PASS. TAP " GLYPH_B " WITH NOTHING NEAR TO SWAP KIDS.",
    GLYPH_A " JUMPS, AND YOUR PARTNER JUMPS WITH YOU. JUMP A SLIDING BAG FOR HALF A STAR. MY JUICE BOXES ARE HALF A STAR TOO.",
    "GOT A STAR? A FULL WIND-UP IS YOUR SPECIAL THROW. " GLYPH_A " AGAIN IN THE AIR IS YOUR SPECIAL MOVE. HOLD A BAG TOO LONG AND IT THROWS ITSELF.",
    "SIX MATCHES. LOSE ONE AND YOU'RE ON THE BENCH. NOW HUSTLE!",
};

static void update_coach(void) {
    if (btnp(BTN_START) && state_t > 10) coach_page = COACH_PAGES;
    else if (state_t > 10 && btnp(BTN_A)) {
        coach_page++;
        state_t = 1;
        sfx_play_name("ui_move");
    }
    if (coach_page >= COACH_PAGES) {
        coach_seen = true;
        set_state(S_BRACKET);
        music_play(SKID_MUS_BRACKET);
    }
}

static void update_result(void) {
    if (state_t < 60 || !(btnp(BTN_A) || btnp(BTN_START))) return;
    sfx_play_name("ui_ok");
    if (mode == MODE_VS) {
        go_teamtype(MODE_VS);
        return;
    }
    if (!result_win) {
        /* one loss and it's over */
        set_state(S_GAMEOVER);
        music_restart(SKID_MUS_OVER);
        return;
    }
    round_i++;
    if (round_i >= ROUNDS) {
        ending_i = 0;
        set_state(S_ENDING);
        music_play(SKID_MUS_CHAMPS);
    } else {
        set_state(S_BRACKET);
        music_play(SKID_MUS_BRACKET);
    }
}

#define ENDING_LINES 6
static int ending_count(void) { return all_blowouts ? ENDING_LINES : ENDING_LINES - 1; }

static void update_codes(void) {
    if (code_msg_t > 0) code_msg_t--;
    int d = btn_repeat(BTN_RIGHT) - btn_repeat(BTN_LEFT);
    if (d) { code_pos = (code_pos + d + 8) % 8; sfx_play_name("ui_move"); }
    int v = btn_repeat(BTN_UP) - btn_repeat(BTN_DOWN);
    if (v) {
        const char *set = CODE_CHARS;
        int n = (int)strlen(set);
        const char *p = strchr(set, code_buf[code_pos]);
        int i = p ? (int)(p - set) : 0;
        code_buf[code_pos] = set[(i + v + n) % n];
        sfx_play_name("ui_move");
    }
    if (btnp(BTN_B) && state_t > 6) { sfx_play_name("ui_back"); go_title(); return; }
    if (state_t > 6 && (btnp(BTN_A) || btnp(BTN_START))) {
        code_msg = "NO SUCH CODE.";
        for (int i = 0; i < ARRAY_LEN(CODES); i++)
            if (!strcmp(code_buf, CODES[i].code)) {
                codes |= (unsigned)CODES[i].bit;
                code_msg = CODES[i].says;
                if (CODES[i].bit == CODE_GYMRULES || CODES[i].bit == CODE_DUELRULE) {
                    rules_which = CODES[i].bit == CODE_DUELRULE;
                    rules_sel = 0;
                    sfx_play_name("ui_toast");
                    set_state(S_RULES);
                    return;
                }
            }
        code_msg_t = 150;
        sfx_play_name(strcmp(code_msg, "NO SUCH CODE.") ? "ui_toast" : "ui_error");
    }
}

#define RULE_ROWS 6
static void update_rules(void) {
    Rules *r = rules_which ? &rules_duel : &rules_tour;
    if (btn_repeat(BTN_UP)) { rules_sel = (rules_sel + RULE_ROWS - 1) % RULE_ROWS; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { rules_sel = (rules_sel + 1) % RULE_ROWS; sfx_play_name("ui_move"); }
    int d = btn_repeat(BTN_RIGHT) - btn_repeat(BTN_LEFT);
    if (d) {
        switch (rules_sel) {
        case 0: r->goal = iclamp(r->goal + d, 5, 30); break;
        case 1: r->max_bags = iclamp(r->max_bags + d, 1, 7); break;
        case 2: r->cost = r->cost == 1 ? 2 : 1; break;
        case 3: r->mate_ai ^= 1; break;
        case 4: r->mate_jump ^= 1; break;
        default: break;
        }
        sfx_play_name("ui_move");
    }
    if ((btnp(BTN_B) && state_t > 6) || (state_t > 6 && (btnp(BTN_A) || btnp(BTN_START)) && rules_sel == RULE_ROWS - 1)) {
        sfx_play_name("ui_back");
        go_title();
    }
}

static void skid_update(void) {
    frame_t++;
    state_t++;
    if (shake > 0) shake--;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vy += 0.08f;
    }
    switch (state) {
    case S_TITLE: update_title(); break;
    case S_TEAMTYPE: update_teamtype(); break;
    case S_DRAFT: update_draft(); break;
    case S_BUILD: update_build(); break;
    case S_COACH: update_coach(); break;
    case S_BRACKET:
        if (state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); start_talk(); }
        break;
    case S_TALK:
        if (state_t > 12 && (btnp(BTN_A) || btnp(BTN_START) || btnp2(BTN_A))) {
            talk_i++;
            state_t = 1;
            if (talk_i >= n_talk) {
                input_consume();
                start_match();
            } else {
                sfx_play_name("ui_move");
            }
        }
        break;
    case S_MATCH:
        run_match(false);
        if (M.state == MS_OVER && M.state_t > 150) finish_match();
        break;
    case S_RESULT: update_result(); break;
    case S_GAMEOVER:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); go_title(); }
        break;
    case S_ENDING:
        if (state_t > 30 && (btnp(BTN_A) || btnp(BTN_START))) {
            ending_i++;
            state_t = 1;
            if (ending_i >= ending_count()) {
                set_state(S_CREDITS);
                music_play(SKID_MUS_CREDITS);
            }
        }
        break;
    case S_CREDITS:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START)) && state_t > 1250) { sfx_play_name("ui_ok"); go_title(); }
        else if (btn(BTN_A)) state_t += 3; /* hold A to hurry the credits along */
        break;
    case S_RECORDS:
        if (state_t > 8 && (btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_START))) { sfx_play_name("ui_back"); go_title(); }
        break;
    case S_CODES: update_codes(); break;
    case S_RULES: update_rules(); break;
    case S_DEMO:
        run_match(true);
        if (M.score[0] >= 999 && M.score[1] >= 999) nines_shown = true;
        if (state_t > 10 && (input_held() & 0xFF)) {
            input_consume();
            go_title();
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing the screens                                                  */

static const uint8_t GRAD_GOLD[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
static const uint8_t GRAD_RED[] = {C_PINK, C_RED, C_WINE};

static void draw_parts(void) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life > 0) gfx_rect((int)parts[i].x, (int)parts[i].y, 2, 2, parts[i].col);
}

/* the gym with an empty floor, for the menus */
static void draw_backdrop(void) {
    gfx_cls(C_AMBER);
    skid_draw_gym(frame_t, 0, state != S_TITLE);
    gfx_rect(0, SKID_TOP - 3, SCREEN_W, SCREEN_H, C_AMBER);
    for (int y = SKID_TOP - 3, row = 0; y < SCREEN_H; y += 6, row++) {
        gfx_hline(0, SCREEN_W - 1, y, C_TAN);
        for (int x = (row * 37) % 70; x < SCREEN_W; x += 70) gfx_vline(x, y + 1, y + 5, C_TAN);
    }
}

static void draw_title(void) {
    draw_backdrop();
    ui_fancy_center("SKID KIDS", 160, 20, 3, GRAD_GOLD, 4, C_INK, C_MAROON);
    text_center("SKID IT. JUMP IT. WIN IT.", 160, 48, C_INK);
    int bob = (frame_t / 16) % 2;
    skid_draw_kid_at(K_NOODLE, 0, (frame_t / 40) % 2 ? KP_THROW : KP_WIND, 50, 150 + bob, 0, 2, false);
    skid_draw_kid_at(K_PIPPA, 1, (frame_t / 20) % 3 == 0 ? KP_JUMP : KP_IDLE0, 270, 150 - ((frame_t / 20) % 3 == 0 ? 6 : 0), 1, 2, false);
    int bx = 80 + (frame_t * 2) % 160;
    gfx_dither(bx - 3, 147, 7, 3, C_BROWN, 10);
    spr_draw(&skid_spr[SS_BAG], bx - 3, 143, 0);
    static const char *items[TITLE_ITEMS] = {"1 PLAYER", "2P CO-OP", "2P VERSUS", "RECORDS", "CODES"};
    ui_panel(106, 60, 108, 72, C_NIGHT, C_AMBER);
    for (int i = 0; i < TITLE_ITEMS; i++) {
        int y = 66 + i * 13;
        bool s = i == title_sel;
        int col = s ? C_WHITE : C_GREY;
        if ((i == 1 || i == 2) && vita_single()) col = s ? C_GREY : C_SLATE;
        text_center(items[i], 160, y, col);
        if (s) ui_cursor(160 - text_width(items[i]) / 2 - 10, y, frame_t);
    }
    if ((title_sel == 1 || title_sel == 2) && vita_single()) {
        gfx_rect(80, 136, 160, 9, C_INK);
        tiny_center("NEEDS TWO CONTROLLERS", 160, 138, C_LIGHT);
    }
    if (codes & CODE_HORNETS && title_sel == 1) tiny_center("THE KANGAROO AND THE ROBOT", 160, 138, C_INK);
    if (codes_on()) {
        gfx_rect(60, 168, 200, 9, C_INK);
        tiny_center("CODES ON: NO SAVING, NO GOALS", 160, 170, C_PINK);
    }
}

static void draw_teamtype(void) {
    draw_backdrop();
    static const char *const HEAD[3] = {"1 PLAYER", "2P CO-OP", "2P VERSUS"};
    ui_fancy_center(HEAD[mode], 160, 8, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    static const char *const NAME[2] = {"DRAFT TEAM", "BUILD TEAM"};
    static const char *const WHAT[2] = {
        "FIVE KIDS COME OFF THE BENCH. YOU PICK ONE, THE OTHER SIDE PICKS TWO, YOU PICK ONE OF THE LAST TWO.",
        "PICK ANY TWO KIDS YOU LIKE.",
    };
    for (int i = 0; i < 2; i++) {
        int y = 52 + i * 52;
        bool s = i == type_sel;
        ui_panel(40, y, 240, 44, C_NIGHT, s ? C_YELLOW : C_DUSK);
        text_draw(NAME[i], 56, y + 6, s ? C_WHITE : C_GREY);
        text_wrap(WHAT[i], 56, y + 18, 214, s ? C_LIGHT : C_SLATE, 8);
        if (s) ui_cursor(46, y + 6, frame_t);
    }
}

static void draw_kid_info(int who, int x, int y, int w, int col) {
    const KidDef *d = &SKID_KID[who];
    text_draw(d->name, x, y, col);
    tiny_draw(d->power, x, y + 10, C_LIGHT);
    char buf[48];
    snprintf(buf, sizeof buf, "THROW: %s", SKID_THROW_NAME[d->throw_kind]);
    tiny_draw(buf, x, y + 18, C_YELLOW);
    snprintf(buf, sizeof buf, "MOVE: %s", SKID_MOVE_NAME[d->move_kind]);
    tiny_draw(buf, x, y + 25, C_CYAN);
    (void)w;
}

static void draw_draft(void) {
    draw_backdrop();
    static const char *const PROMPT[4] = {"PICK ONE", "THE OTHER SIDE PICKS TWO", "PICK ONE MORE", "NOBODY PICKED..."};
    const char *prompt = PROMPT[draft_phase];
    if (draft_phase == 1 && mode == MODE_VS) prompt = "PLAYER 2 PICKS TWO";
    if (draft_phase == 2 && mode == MODE_COOP) prompt = "PLAYER 2 PICKS ONE";
    ui_fancy_center(prompt, 160, 4, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    for (int i = 0; i < 5; i++) {
        int x = 10 + i * 61, y = 26;
        int who = pool[i];
        bool cur0 = draft_phase != 3 && dcur[0] == i && owner[i] < 0 && (draft_phase == 0 || (draft_phase == 2 && mode != MODE_COOP));
        bool cur1 = draft_phase != 3 && dcur[1] == i && owner[i] < 0 &&
                    ((draft_phase == 1 && mode == MODE_VS) || (draft_phase == 2 && mode == MODE_COOP));
        int border = owner[i] == 0 ? C_RED : owner[i] == 1 ? C_BLUE : cur0 ? C_YELLOW : cur1 ? C_MAGENTA : C_DUSK;
        ui_panel(x, y, 58, 74, C_NIGHT, border);
        bool cry = draft_phase == 3 && owner[i] < 0;
        int team = owner[i] == 1 ? 1 : 0;
        int pose = cry ? KP_HURT : owner[i] >= 0 ? KP_WIN : ((frame_t / 24 + i) % 2 ? KP_IDLE1 : KP_IDLE0);
        gfx_clip(x + 1, y + 1, 56, 72);
        skid_draw_kid_at(who, team, pose, x + 29, y + 50, owner[i] == 1, 2, cry);
        gfx_noclip();
        tiny_center(SKID_KID[who].name, x + 29, y + 54, C_WHITE);
        const char *tag = owner[i] == 0 ? (mode == MODE_VS ? "PLAYER 1" : "YOURS") : owner[i] == 1 ? (mode == MODE_VS ? "PLAYER 2" : "THEIRS")
                          : cry ? "LEFT OVER" : "";
        tiny_center(tag, x + 29, y + 63, owner[i] == 0 ? C_PINK : owner[i] == 1 ? C_SKY : C_GREY);
        if (cur0) tiny_draw("1P", x + 3, y + 3, C_YELLOW);
        if (cur1) tiny_draw("2P", x + 46, y + 3, C_MAGENTA);
    }
    int show = draft_phase == 1 && mode == MODE_VS ? dcur[1] : draft_phase == 2 && mode == MODE_COOP ? dcur[1] : dcur[0];
    if (draft_phase == 3) {
        for (int i = 0; i < 5; i++)
            if (owner[i] < 0) show = i;
    }
    ui_panel(10, 106, 300, 40, C_NIGHT, C_AMBER);
    draw_kid_info(pool[show], 20, 111, 280, C_YELLOW);
    if (draft_phase == 3 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 158, C_INK);
    if (note_shown) {
        /* the note under the bleachers */
        ui_panel(60, 40, 200, 60, C_WHITE, C_INK);
        spr_draw_scaled(&skid_spr[SS_NOTE], 70, 50, 2, 0);
        text_wrap("FOUND UNDER THE BLEACHERS: \"PICK ME NEXT TIME. PLEASE?\"", 100, 48, 152, C_INK, 9);
    }
}

static void draw_build(void) {
    draw_backdrop();
    int slots = build_slots();
    int need = mode == MODE_COOP ? 1 : 2;
    const char *head = mode == MODE_COOP ? "EACH PICK ONE" : mode == MODE_VS ? "EACH PICK TWO" : "PICK TWO";
    ui_fancy_center(head, 160, 4, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    for (int n = 0; n < slots; n++) {
        int x = 6 + (n % 7) * 44, y = 24 + (n / 7) * 50;
        int team = -1;
        for (int p = 0; p < 2; p++)
            for (int k = 0; k < bn[p]; k++)
                if (bpick[p][k] == n) team = p;
        bool c0 = bcur[0] == n && bn[0] < need, c1 = mode != MODE_1P && bcur[1] == n && bn[1] < need;
        int border = team == 0 ? C_RED : team == 1 ? (mode == MODE_COOP ? C_RED : C_BLUE) : c0 ? C_YELLOW : c1 ? C_MAGENTA : C_DUSK;
        ui_panel(x, y, 42, 48, C_NIGHT, border);
        int bib = team == 1 && mode == MODE_VS ? 1 : 0;
        gfx_clip(x + 1, y + 1, 40, 46);
        skid_draw_kid_at(n, bib, team >= 0 ? KP_WIN : KP_IDLE0, x + 21, y + 38, 0, n >= SKID_KIDS ? 1 : 2, false);
        gfx_noclip();
        tiny_center(SKID_KID[n].name, x + 21, y + 40, C_WHITE);
        if (c0) tiny_draw("1P", x + 3, y + 3, C_YELLOW);
        if (c1) tiny_draw("2P", x + 30, y + 3, C_MAGENTA);
    }
    if (mode == MODE_1P) {
        ui_panel(10, 128, 300, 42, C_NIGHT, C_AMBER);
        draw_kid_info(bcur[0], 20, 133, 280, C_YELLOW);
    } else {
        ui_panel(6, 128, 152, 42, C_NIGHT, C_YELLOW);
        ui_panel(162, 128, 152, 42, C_NIGHT, C_MAGENTA);
        draw_kid_info(bcur[0], 12, 133, 140, C_YELLOW);
        draw_kid_info(bcur[1], 168, 133, 140, C_MAGENTA);
    }
}

static void draw_coach(void) {
    draw_backdrop();
    spr_draw_scaled(&skid_spr[(frame_t / 20) % 2 ? SS_COACH : SS_COACH_WHISTLE], 20, 60, 4, 0);
    ui_panel(92, 50, 220, 90, C_WHITE, C_INK);
    gfx_line(92, 90, 84, 96, C_INK);
    text_draw("COACH", 100, 56, C_MAROON);
    if (coach_page < COACH_PAGES) text_wrap(COACH_TALK[coach_page], 100, 68, 204, C_INK, 9);
    char buf[16];
    snprintf(buf, sizeof buf, "%d/%d", coach_page + 1, COACH_PAGES);
    tiny_draw(buf, 290, 132, C_GREY);
    if ((state_t / 20) % 2) text_center(GLYPH_A " NEXT   START SKIP", 202, 146, C_INK);
}

static void draw_team_pair(const int *t, int team, int x, int y, int scale, int pose) {
    skid_draw_kid_at(t[0], team, pose, x, y, team == 1, scale, false);
    skid_draw_kid_at(t[1], team, pose, x + 14 * scale, y + 4 * scale, team == 1, scale, false);
}

static void draw_bracket(void) {
    draw_backdrop();
    ui_fancy_center("THE GYM CLASS CUP", 160, 2, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    ui_panel(6, 24, 78, 150, C_NIGHT, C_RED);
    draw_team_pair(team_a, 0, 30, 100, 2, KP_IDLE0);
    text_center(SKID_KID[team_a[0]].name, 45, 116, C_PINK);
    text_center(SKID_KID[team_a[1]].name, 45, 128, C_PINK);
    char buf[32];
    snprintf(buf, sizeof buf, "WON %d", wins);
    text_center(buf, 45, 150, C_YELLOW);
    for (int r = 0; r < ROUNDS; r++) {
        int y = 24 + r * 25;
        bool done = r < round_i, next = r == round_i;
        ui_panel(92, y, 222, 23, C_NIGHT, next ? C_YELLOW : done ? C_DUSK : C_SLATE);
        snprintf(buf, sizeof buf, "MATCH %d", r + 1);
        tiny_draw(buf, 98, y + 9, next ? C_YELLOW : C_GREY);
        if (r <= round_i) {
            for (int k = 0; k < 2; k++) {
                int who = opp[r][k];
                skid_draw_kid_at(who, 1, KP_IDLE0, 148 + k * 78, y + 21, 1, 1, false);
                text_draw(SKID_KID[who].name, 160 + k * 78, y + 8, done ? C_SLATE : C_WHITE);
            }
            if (done) gfx_line(138, y + 11, 300, y + 11, C_RED);
        } else {
            text_draw(r == ROUNDS - 1 ? "? ? ?" : "?", 160, y + 8, C_SLATE);
        }
        if (next) ui_cursor(84, y + 8, frame_t);
    }
    if (state_t > 20 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 45, 162, C_WHITE);
}

static void draw_talk(void) {
    draw_backdrop();
    const int *them = mode == MODE_VS ? team_b : opp[round_i];
    draw_team_pair(team_a, 0, 50, 124, 2, KP_IDLE0);
    draw_team_pair(them, 1, 240, 124, 2, KP_IDLE0);
    char buf[32];
    if (mode == MODE_VS) snprintf(buf, sizeof buf, "PLAYER 1 VS PLAYER 2");
    else snprintf(buf, sizeof buf, "MATCH %d OF %d", round_i + 1, ROUNDS);
    ui_fancy_center(buf, 160, 52, 1, GRAD_GOLD, 4, C_INK, -1);
    ui_fancy_center("VS", 160, 90, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    if (talk_i < n_talk) {
        const Line *l = &talk[talk_i];
        ui_panel(8, 136, 304, 40, C_WHITE, l->team == 0 ? C_RED : C_BLUE);
        const char *name = l->who >= 0 ? SKID_KID[l->who].name : "COACH";
        text_draw(name, 16, 140, l->team == 0 ? C_RED : C_BLUE);
        text_wrap(l->text, 16, 151, 288, C_INK, 9);
        /* a mark over the one speaking */
        const int *pair = l->team == 0 ? team_a : them;
        int sx = (l->team == 0 ? 50 : 240) + (pair[1] == l->who && pair[0] != l->who ? 28 : 0);
        int sy = 124 - 46 + (pair[1] == l->who && pair[0] != l->who ? 8 : 0) - (frame_t / 8) % 2;
        if (l->who >= 0) for (int r = 0; r < 3; r++) gfx_hline(sx - 2 + r, sx + 2 - r, sy + r, l->team == 0 ? C_RED : C_BLUE);
    }
}

static void draw_result(void) {
    skid_draw_court(&M, frame_t, 0, -1, -1);
    gfx_dither(0, 14, SCREEN_W, 166, C_INK, 9);
    int border = result_win || mode == MODE_VS ? C_YELLOW : C_RED;
    ui_panel(60, 36, 200, 100, C_NIGHT, border);
    char buf[48];
    if (mode == MODE_VS) snprintf(buf, sizeof buf, "PLAYER %d WINS", M.winner + 1);
    else snprintf(buf, sizeof buf, result_win ? "YOU WIN!" : "YOU LOSE");
    ui_fancy_center(buf, 160, 44, 2, result_win || mode == MODE_VS ? GRAD_GOLD : GRAD_RED, 3, C_INK, C_INK);
    snprintf(buf, sizeof buf, "%d - %d", M.score[0], M.score[1]);
    text_center(buf, 160, 68, C_WHITE);
    if (result_blowout) text_center("BLOWOUT!", 160, 82, C_ORANGE);
    else if (result_close) text_center("CLOSE GAME!", 160, 82, C_CYAN);
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 120, C_WHITE);
}

static void draw_gameover(void) {
    draw_backdrop();
    /* the bench */
    gfx_rect(90, 132, 140, 6, C_BROWN);
    gfx_rect(96, 138, 4, 14, C_EARTH);
    gfx_rect(220, 138, 4, 14, C_EARTH);
    skid_draw_kid_at(team_a[0], 0, KP_IDLE0, 140, 140, 0, 2, true);
    skid_draw_kid_at(team_a[1], 0, KP_IDLE0, 180, 140, 1, 2, true);
    ui_fancy_center("GAME OVER", 160, 22, 3, GRAD_RED, 3, C_INK, C_INK);
    text_center("ONE LOSS AND YOU'RE ON THE BENCH.", 160, 54, C_INK);
    char buf[40];
    snprintf(buf, sizeof buf, "MATCHES WON: %d", wins);
    text_center(buf, 160, 66, C_MAROON);
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 166, C_INK);
}

static void draw_ending(void) {
    draw_backdrop();
    int bob = (frame_t / 12) % 2;
    /* the robot, sat down for good, and the kid who built it */
    skid_draw_kid_at(K_BENCHBOT, 1, KP_DOWN, 250, 150, 1, 2, false);
    if (ending_i < 1 && (frame_t / 6) % 2) gfx_rect(262, 118, 3, 3, C_YELLOW);
    int lo = leftover >= 0 ? leftover : K_NOODLE;
    skid_draw_kid_at(lo, ending_i >= 3 ? 0 : 1, ending_i >= 3 ? KP_WIN : KP_IDLE0, 200, 150, 1, 2, ending_i >= 1 && ending_i < 3);
    skid_draw_kid_at(team_a[0], 0, KP_WIN, 70, 150 - bob, 0, 2, false);
    skid_draw_kid_at(team_a[1], 0, KP_WIN, 110, 150 - (1 - bob), 0, 2, false);
    spr_draw_scaled(&skid_spr[SS_TROPHY], 78, 70 - bob * 2, 2, 0);
    for (int i = 0; i < 18; i++) {
        int x = (i * 53 + frame_t) % SCREEN_W, y = (i * 29 + frame_t * (1 + i % 3)) % 110;
        gfx_rect(x, y, 2, 3, i % 3 == 0 ? C_RED : i % 3 == 1 ? C_YELLOW : C_BLUE);
    }
    static const char *const WHO[ENDING_LINES] = {"", "", "", "", "COACH", "COACH"};
    static const char *const TEXT[ENDING_LINES] = {
        "THE ROBOT SITS DOWN WITH A CLUNK. ITS LIGHT GOES OUT.",
        "...I JUST WANTED SOMEBODY TO PICK ME.",
        "NEXT GYM CLASS YOU'RE ON OUR TEAM. FIRST PICK. PROMISE.",
        "REALLY? CAN BENCHBOT COME TOO?",
        "THE GYM CLASS CUP GOES TO YOU TWO! NOW HIT THE SHOWERS!",
        "SIX BLOWOUTS IN A ROW. I'M GETTING YOU A BIGGER CUP.",
    };
    if (ending_i >= ending_count()) return;
    const char *who = WHO[ending_i];
    if (ending_i == 1 || ending_i == 3) who = SKID_KID[lo].name;
    if (ending_i == 2) who = SKID_KID[team_a[0]].name;
    ui_panel(8, 4, 304, 44, C_WHITE, C_INK);
    if (*who) text_draw(who, 16, 8, C_MAROON);
    text_wrap(TEXT[ending_i], 16, *who ? 19 : 12, 288, C_INK, 9);
}

#define CREDIT_LINES 24
static const char *const CREDITS[CREDIT_LINES] = {
    "SKID KIDS",
    "BEAMDOWN SOFTWORKS 1986",
    "",
    "THE KIDS",
    "NOODLE  PIPPA  HOPS  MILO",
    "SPARKY  NELL  KIKI  ROXIE",
    "TOBY  SID  BUZZY  MOOSE",
    "",
    "THE PARTNERS",
    "BOOMER THE KANGAROO",
    "BENCHBOT",
    "",
    "THE COACH",
    "AS HIMSELF",
    "",
    "GAME, PICTURES AND TUNES",
    "THE BEAMDOWN GYM CLUB",
    "",
    "NO KID WAS LEFT ON THE BENCH",
    "IN THE MAKING OF THIS GAME",
    "",
    "THANKS FOR PLAYING!",
    "",
    "CODE: ALLS-TARS",
};

static void draw_credits(void) {
    gfx_cls(C_NIGHT);
    int y0 = 190 - state_t / 3;
    for (int i = 0; i < CREDIT_LINES; i++) {
        int y = y0 + i * 14;
        if (i == CREDIT_LINES - 1) y = imax(y, 110);
        if (y < -10 || y > 184) continue;
        int col = i == 0 ? C_YELLOW : i == CREDIT_LINES - 1 ? C_PINK : (CREDITS[i][0] && i > 0 && CREDITS[i - 1][0] == 0) ? C_AMBER : C_LIGHT;
        text_center(CREDITS[i], 160, y, col);
    }
    int last = y0 + (CREDIT_LINES - 1) * 14;
    if (last <= 110) {
        tiny_center("ENTER IT UNDER CODES FOR EVERYONE IN BUILD TEAM", 160, 126, C_GREY);
        skid_draw_kid_at(K_BOOMER, 0, KP_IDLE0, 130, 170, 0, 1, false);
        skid_draw_kid_at(K_BENCHBOT, 1, KP_IDLE0, 190, 170, 1, 1, false);
    }
}

static void draw_records(void) {
    draw_backdrop();
    ui_fancy_center("RECORDS", 160, 2, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    for (int i = 0; i < SKID_KIDS; i++) {
        int x = 8 + (i % 6) * 51, y = 22 + (i / 6) * 64;
        bool used = (sv.used >> i) & 1, won = (sv.won_with >> i) & 1;
        ui_panel(x, y, 49, 62, C_NIGHT, won ? C_YELLOW : used ? C_LIGHT : C_DUSK);
        gfx_clip(x + 1, y + 1, 47, 60);
        if (used) skid_draw_kid_at(i, 0, won ? KP_WIN : KP_IDLE0, x + 24, y + 42, 0, 2, false);
        else spr_draw_ex(&skid_kid_spr[i][0][KP_IDLE0], x + 18, y + 22, 0, NULL, C_DUSK);
        gfx_noclip();
        tiny_center(SKID_KID[i].name, x + 24, y + 44, used ? C_LIGHT : C_SLATE);
        tiny_draw("USED", x + 3, y + 52, C_GREY);
        tiny_draw(used ? "Y" : "-", x + 21, y + 52, used ? C_LIME : C_SLATE);
        tiny_draw("WON", x + 27, y + 52, C_GREY);
        tiny_draw(won ? "Y" : "-", x + 41, y + 52, won ? C_YELLOW : C_SLATE);
    }
    char buf[40];
    ui_panel(40, 152, 240, 26, C_NIGHT, C_AMBER);
    snprintf(buf, sizeof buf, "ATHLETES USED  %d/%d", bits12(sv.used), SKID_KIDS);
    text_center(buf, 160, 155, C_WHITE);
    snprintf(buf, sizeof buf, "ATHLETES WON WITH  %d/%d", bits12(sv.won_with), SKID_KIDS);
    text_center(buf, 160, 166, C_YELLOW);
}

static void draw_codes(void) {
    draw_backdrop();
    ui_fancy_center("CODES", 160, 6, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    ui_panel(40, 34, 240, 64, C_NIGHT, C_AMBER);
    for (int i = 0; i < 8; i++) {
        int x = 72 + i * 20 + (i >= 4 ? 16 : 0), y = 50;
        bool s = i == code_pos;
        gfx_rect(x - 2, y - 3, 14, 15, s ? C_DUSK : C_INK);
        char c[2] = {code_buf[i], 0};
        text_draw(c, x + 2, y, s ? C_YELLOW : C_WHITE);
        if (s) {
            text_draw(GLYPH_UP, x + 1, y - 12, C_GREY);
            text_draw(GLYPH_DOWN, x + 1, y + 14, C_GREY);
        }
    }
    text_draw("-", 155, 50, C_GREY);
    text_center(GLYPH_DPAD " SPELL   " GLYPH_A " ENTER   " GLYPH_B " BACK", 160, 84, C_GREY);
    if (code_msg && code_msg_t > 0) {
        ui_panel(20, 104, 280, 20, C_WHITE, C_INK);
        text_center(code_msg, 160, 110, C_INK);
    }
    int y = 132;
    tiny_center("CODES ON:", 160, y, C_INK);
    char on[80] = "";
    for (int i = 0; i < ARRAY_LEN(CODES); i++)
        if (codes & (unsigned)CODES[i].bit) {
            char c[12];
            snprintf(c, sizeof c, "%.4s-%.4s ", CODES[i].code, CODES[i].code + 4);
            strncat(on, c, sizeof on - strlen(on) - 1);
        }
    tiny_center(on[0] ? on : "NONE", 160, y + 8, C_MAROON);
    if (codes_on()) tiny_center("NO SAVING OR GOALS UNTIL YOU LEAVE THE CARTRIDGE", 160, y + 20, C_MAROON);
}

static void draw_rules(void) {
    draw_backdrop();
    const Rules *r = rules_which ? &rules_duel : &rules_tour;
    ui_fancy_center(rules_which ? "VERSUS RULES" : "HOUSE RULES", 160, 6, 2, GRAD_GOLD, 4, C_INK, C_MAROON);
    ui_panel(30, 32, 260, 108, C_NIGHT, C_AMBER);
    static const char *const NAME[RULE_ROWS] = {"POINTS TO WIN", "BAGS ON COURT", "STAR COST", "PARTNER", "JUMPING", "DONE"};
    char val[RULE_ROWS][24];
    snprintf(val[0], 24, "%d", r->goal);
    snprintf(val[1], 24, "%d", r->max_bags);
    snprintf(val[2], 24, "%d", r->cost);
    snprintf(val[3], 24, "%s", r->mate_ai ? "PLAYS ALONE" : "FETCHES BAGS");
    snprintf(val[4], 24, "%s", r->mate_jump ? "ONE AT A TIME" : "TOGETHER");
    val[5][0] = 0;
    for (int i = 0; i < RULE_ROWS; i++) {
        int y = 42 + i * 16;
        bool s = i == rules_sel;
        text_draw(NAME[i], 50, y, s ? C_WHITE : C_GREY);
        if (i < RULE_ROWS - 1) {
            char buf[40];
            snprintf(buf, sizeof buf, GLYPH_LEFT " %.23s " GLYPH_RIGHT, val[i]);
            text_draw(buf, 162, y, s ? C_YELLOW : C_SLATE);
        }
        if (s) ui_cursor(40, y, frame_t);
    }
}

static void draw_demo(void) {
    skid_draw_court(&M, frame_t, shake, -1, -1);
    draw_parts();
    if ((frame_t / 30) % 2) {
        gfx_rect(128, 164, 64, 11, C_INK);
        tiny_center("DEMO", 160, 167, C_YELLOW);
    }
    if (nines_shown) {
        /* a secret: the demo left to run up to 999 each */
        ui_panel(40, 60, 240, 50, C_WHITE, C_INK);
        text_draw("COACH", 50, 66, C_MAROON);
        text_wrap("OKAY, OKAY. NINE HUNDRED AND NINETY-NINE EACH. EVERYBODY HIT THE SHOWERS. AND SOMEBODY TURN THIS THING OFF.", 50, 78, 222, C_INK, 9);
    }
}

/* the sprite sheet (tests): seven kids a page, every pose */
static int sheet_page;
static void draw_sheet(void) {
    gfx_cls(C_AMBER);
    for (int r = 0; r < 7; r++) {
        int k = sheet_page * 7 + r;
        for (int p = 0; p < KP_COUNT; p++) {
            const Sprite *s = &skid_kid_spr[k][r % 2][p];
            spr_draw(s, 2 + p * 26, 2 + r * 25 + (25 - s->h) - 2, 0);
        }
    }
    spr_draw(&skid_spr[SS_COACH], 292, 4, 0);
    spr_draw(&skid_spr[SS_COACH_THROW], 292, 30, 0);
    spr_draw(&skid_spr[SS_COACH_WHISTLE], 292, 56, 0);
    for (int i = SS_BAG; i < SS_SPRITE_COUNT; i++) spr_draw(&skid_spr[i], 290 + ((i - SS_BAG) % 2) * 14, 84 + ((i - SS_BAG) / 2) * 16, 0);
}

static void skid_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_TEAMTYPE: draw_teamtype(); break;
    case S_DRAFT: draw_draft(); break;
    case S_BUILD: draw_build(); break;
    case S_COACH: draw_coach(); break;
    case S_BRACKET: draw_bracket(); break;
    case S_TALK: draw_talk(); break;
    case S_MATCH: {
        int p1 = M.coop ? 0 : M.ctrl[0];
        int p2 = M.coop ? 1 : M.player[1] == CTRL_P2 ? M.ctrl[1] : -1;
        skid_draw_court(&M, frame_t, shake, p1, p2);
        draw_parts();
        if (M.state == MS_READY) ui_fancy_center(M.state_t < 30 ? "READY..." : "GO!", 160, 80, 2, GRAD_GOLD, 3, C_INK, C_BROWN);
        if (M.state == MS_OVER) {
            gfx_dither(0, 70, SCREEN_W, 36, C_INK, 10);
            const char *t = mode == MODE_VS ? (M.winner == 0 ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!") : M.winner == 0 ? "YOU WIN!" : "YOU LOSE";
            ui_fancy_center(t, 160, 80, 2, M.winner == 0 || mode == MODE_VS ? GRAD_GOLD : GRAD_RED, 3, C_INK, C_INK);
        }
        break;
    }
    case S_RESULT: draw_result(); break;
    case S_GAMEOVER: draw_gameover(); break;
    case S_ENDING: draw_ending(); break;
    case S_CREDITS: draw_credits(); break;
    case S_RECORDS: draw_records(); break;
    case S_CODES: draw_codes(); break;
    case S_RULES: draw_rules(); break;
    case S_DEMO: draw_demo(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void skid_load(void) {
    skid_art_load();
    skid_audio_load();
}

static void skid_start(void) {
    rng_seed(&seeds, g_rng.state ^ 0x5C1Dull);
    load_save();
    memset(&M, 0, sizeof M);
    memset(parts, 0, sizeof parts);
    codes = 0; /* codes last until the cartridge is left */
    skid_rules_default(&rules_tour);
    skid_rules_default(&rules_duel);
    coach_seen = false;
    sheet_mode = false;
    title_sel = 0;
    leftover = -1;
    team_a[0] = K_NOODLE;
    team_a[1] = K_PIPPA;
    go_title();
}

static void skid_quit(void) {
    input_set_versus(false);
    save_now();
}

static void skid_label(int x, int y, int w, int h, int t) {
    for (int yy = 0; yy < h; yy++) gfx_hline(x, x + w - 1, y + yy, yy < 18 ? C_CREAM : C_AMBER);
    gfx_rect(x, y + 14, w, 2, C_MAROON);
    for (int yy = y + 20; yy < y + h; yy += 6) gfx_hline(x, x + w - 1, yy, C_TAN);
    gfx_rect(x + w / 2 - 1, y + 18, 2, h - 18, C_RED);
    int k = (t / 20) % 2;
    skid_draw_kid_at(K_SPARKY, 0, k ? KP_THROW : KP_WIND, x + 24, y + 50, 0, 1, false);
    skid_draw_kid_at(K_KIKI, 1, (t / 16) % 3 == 0 ? KP_JUMP : KP_IDLE0, x + w - 26, y + 50 - ((t / 16) % 3 == 0 ? 5 : 0), 1, 1, false);
    int bx = x + 40 + (t * 2) % (w - 80);
    gfx_dither(bx - 3, y + 50, 7, 2, C_BROWN, 10);
    spr_draw(&skid_spr[SS_BAG], bx - 3, y + 45, 0);
}

/* The demo player: player 1's buttons for the runner's "bot" query. It
 * plays the kid it drives the way the best rival does, pressing real
 * buttons; a press that follows a held button waits a frame. */
static int bot_buttons(void) {
    if (state != S_MATCH || M.state != MS_PLAY) {
        bot_a_prev = bot_b_prev = bot_a_queue = bot_b_queue = false;
        return (state_t / 8) % 2 ? BTN_A : 0;
    }
    Pad pd;
    skid_bot_pad(&M, M.ctrl[0], &pd);
    int mask = 0;
    if (pd.dx > 0) mask |= BTN_RIGHT;
    if (pd.dx < 0) mask |= BTN_LEFT;
    if (pd.dy > 0) mask |= BTN_DOWN;
    if (pd.dy < 0) mask |= BTN_UP;
    bool a = false, b = pd.b;
    if (pd.ap) {
        if (bot_a_prev) bot_a_queue = true;
        else a = true;
    } else if (bot_a_queue) {
        a = true;
        bot_a_queue = false;
    }
    if (pd.bp) {
        if (bot_b_prev) { b = false; bot_b_queue = true; }
        else b = true;
    } else if (bot_b_queue) {
        b = true;
        bot_b_queue = false;
    }
    if (pd.br) b = false;
    bot_a_prev = a;
    bot_b_prev = b;
    return mask | (a ? BTN_A : 0) | (b ? BTN_B : 0);
}

static int count_bags(int st) {
    int n = 0;
    for (int b = 0; b < SKID_MAX_BAGS; b++) n += M.bag[b].state == st;
    return n;
}

static int count_items(int kind) {
    int n = 0;
    for (int i = 0; i < SKID_MAX_ITEMS; i++) n += M.it[i].kind == kind;
    return n;
}

/* The tournament's shape holds: the kangaroo partners match 5, the robot
 * and the left-over kid are match 6, no kid turns up twice, and after a
 * draft the CPU's two picks are match 1 and every one of the twelve kids
 * has a place. */
static int bracket_ok(void) {
    if (opp[4][1] != K_BOOMER || opp[5][1] != K_BENCHBOT || opp[5][0] != leftover || leftover < 0) return 0;
    int seen[SKID_ROSTER] = {0};
    int slots[12] = {team_a[0], team_a[1], opp[0][0], opp[0][1], opp[1][0], opp[1][1], opp[2][0], opp[2][1],
                     opp[3][0], opp[3][1], opp[4][0], opp[5][0]};
    for (int i = 0; i < 12; i++) {
        if (slots[i] < 0 || slots[i] >= SKID_ROSTER) return 0;
        if (++seen[slots[i]] > 1) return 0;
    }
    if (!build_mode) {
        if (opp[0][0] != team_b[0] || opp[0][1] != team_b[1]) return 0;
        for (int k = 0; k < SKID_KIDS; k++)
            if (seen[k] != 1) return 0;
    }
    return 1;
}

static int skid_query(const char *key, int *out) {
    if (!strcmp(key, "bot")) { *out = bot_buttons(); return 1; }
    if (!strcmp(key, "bracket_ok")) { *out = bracket_ok(); return 1; }
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "mode")) { *out = mode; return 1; }
    if (!strcmp(key, "round")) { *out = round_i; return 1; }
    if (!strcmp(key, "wins")) { *out = wins; return 1; }
    if (!strcmp(key, "blowouts")) { *out = all_blowouts; return 1; }
    if (!strcmp(key, "result_win")) { *out = result_win; return 1; }
    if (!strcmp(key, "result_blowout")) { *out = result_blowout; return 1; }
    if (!strcmp(key, "result_close")) { *out = result_close; return 1; }
    if (!strcmp(key, "codes")) { *out = (int)codes; return 1; }
    if (!strcmp(key, "leftover")) { *out = leftover; return 1; }
    if (!strcmp(key, "phase")) { *out = draft_phase; return 1; }
    if (!strcmp(key, "cups")) { *out = sv.cups; return 1; }
    if (!strcmp(key, "best")) { *out = sv.best_wins; return 1; }
    if (!strcmp(key, "used")) { *out = sv.used; return 1; }
    if (!strcmp(key, "won_with")) { *out = sv.won_with; return 1; }
    if (!strcmp(key, "used_n")) { *out = bits12(sv.used); return 1; }
    if (!strcmp(key, "won_n")) { *out = bits12(sv.won_with); return 1; }
    if (!strcmp(key, "talk_n")) { *out = n_talk; return 1; }
    if (!strcmp(key, "talk_i")) { *out = talk_i; return 1; }
    if (!strcmp(key, "talk_special")) { *out = talk_special; return 1; }
    if (!strcmp(key, "coach_seen")) { *out = coach_seen; return 1; }
    if (!strcmp(key, "coach_page")) { *out = coach_page; return 1; }
    if (!strcmp(key, "ending")) { *out = ending_i; return 1; }
    if (!strcmp(key, "nines")) { *out = nines_shown; return 1; }
    if (!strcmp(key, "note")) { *out = note_shown; return 1; }
    if (!strcmp(key, "art_bad")) { *out = skid_art_bad; return 1; }
    if (!strcmp(key, "music")) { *out = music_playing(); return 1; }
    if (!strcmp(key, "versus")) { *out = input_versus(); return 1; }
    if (!strcmp(key, "title_sel")) { *out = title_sel; return 1; }
    if (!strcmp(key, "build_slots")) { *out = build_slots(); return 1; }
    if (!strcmp(key, "mstate")) { *out = M.state; return 1; }
    if (!strcmp(key, "winner")) { *out = M.winner; return 1; }
    if (!strcmp(key, "score0")) { *out = M.score[0]; return 1; }
    if (!strcmp(key, "score1")) { *out = M.score[1]; return 1; }
    if (!strcmp(key, "ctrl0")) { *out = M.ctrl[0]; return 1; }
    if (!strcmp(key, "ctrl1")) { *out = M.ctrl[1]; return 1; }
    if (!strcmp(key, "goal")) { *out = M.rules.goal; return 1; }
    if (!strcmp(key, "max_bags")) { *out = M.rules.max_bags; return 1; }
    if (!strcmp(key, "cost")) { *out = M.rules.cost; return 1; }
    if (!strcmp(key, "mate_ai")) { *out = M.rules.mate_ai; return 1; }
    if (!strcmp(key, "mate_jump")) { *out = M.rules.mate_jump; return 1; }
    if (!strcmp(key, "swap_only")) { *out = M.swap_only; return 1; }
    if (!strcmp(key, "favored")) { *out = M.favored; return 1; }
    if (!strcmp(key, "level")) { *out = M.level[1]; return 1; }
    if (!strcmp(key, "bags")) { *out = skid_bags_total(&M); return 1; }
    if (!strcmp(key, "loose")) { *out = count_bags(BG_LOOSE); return 1; }
    if (!strcmp(key, "loose_l") || !strcmp(key, "loose_r")) {
        int n = 0;
        for (int b = 0; b < SKID_MAX_BAGS; b++)
            if (M.bag[b].state == BG_LOOSE && (key[6] == 'l' ? M.bag[b].x < SKID_MID : M.bag[b].x > SKID_MID)) n++;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "sliding")) { *out = count_bags(BG_SLIDE) + count_bags(BG_AIR); return 1; }
    if (!strcmp(key, "passing")) { *out = count_bags(BG_PASS); return 1; }
    if (!strcmp(key, "reeling")) { *out = count_bags(BG_REEL); return 1; }
    if (!strcmp(key, "juice")) { *out = count_items(IT_JUICE); return 1; }
    if (!strcmp(key, "puddles")) { *out = count_items(IT_PUDDLE); return 1; }
    if (!strcmp(key, "marbles")) { *out = count_items(IT_MARBLE); return 1; }
    if (!strcmp(key, "balloons")) { *out = count_items(IT_BALLOON); return 1; }
    if (!strcmp(key, "waves")) { int n = 0; for (int w = 0; w < SKID_MAX_WAVES; w++) n += M.wave[w].live; *out = n; return 1; }
    if (!strcmp(key, "sweep")) { *out = M.sweep.live; return 1; }
    if (!strcmp(key, "sweep_x")) { *out = (int)M.sweep.x; return 1; }
    if (!strcmp(key, "gust")) { *out = M.gust_t; return 1; }
    if (!strcmp(key, "quake")) { *out = M.quake_t; return 1; }
    if (!strcmp(key, "coach_items")) { *out = M.coach_items; return 1; }
    if (!strcmp(key, "bark")) { *out = M.bark_t > 0 ? M.bark : 0; return 1; }
    static const struct { const char *k; int off; } TALLY[] = {
        {"n_throw", offsetof(Match, n_throw)}, {"n_hit", offsetof(Match, n_hit)}, {"n_down", offsetof(Match, n_down)},
        {"n_half", offsetof(Match, n_half)}, {"n_pick", offsetof(Match, n_pick)}, {"n_pass", offsetof(Match, n_pass)},
        {"n_swap", offsetof(Match, n_swap)}, {"n_sthrow", offsetof(Match, n_special_throw)},
        {"n_smove", offsetof(Match, n_special_move)}, {"n_forced", offsetof(Match, n_forced)},
        {"n_cancel", offsetof(Match, n_cancel)}, {"n_cbag", offsetof(Match, n_coach_bag)},
        {"n_cjuice", offsetof(Match, n_coach_juice)}, {"n_cballoon", offsetof(Match, n_coach_balloon)},
        {"n_drink", offsetof(Match, n_drink)}, {"n_slip", offsetof(Match, n_slip)}, {"n_push", offsetof(Match, n_push)},
        {"n_boom", offsetof(Match, n_boom)}, {"n_crushed", offsetof(Match, n_crushed)}, {"n_reel", offsetof(Match, n_reel)},
        {"n_toss", offsetof(Match, n_toss)}, {"n_jump", offsetof(Match, n_jump)},
        {"when0", offsetof(Match, hit_when)}, {"when1", offsetof(Match, hit_when) + sizeof(int)},
        {"when2", offsetof(Match, hit_when) + 2 * sizeof(int)}, {"when3", offsetof(Match, hit_when) + 3 * sizeof(int)},
        {"hits0", offsetof(Match, hits_on)}, {"hits1", offsetof(Match, hits_on) + sizeof(int)},
        {"hits2", offsetof(Match, hits_on) + 2 * sizeof(int)}, {"hits3", offsetof(Match, hits_on) + 3 * sizeof(int)},
    };
    for (int i = 0; i < ARRAY_LEN(TALLY); i++)
        if (!strcmp(key, TALLY[i].k)) {
            memcpy(out, (const char *)&M + TALLY[i].off, sizeof(int));
            return 1;
        }
    /* per kid: kx0 ky0 kz0 kst0 stars0 held0 who0 hold0 charge0 timer0 face0 aimx0 aimy0 */
    static const char *const PK[] = {"kx", "ky", "kz", "kst", "stars", "held", "who", "hold", "charge", "timer", "face", "aimx", "aimy"};
    for (int k = 0; k < ARRAY_LEN(PK); k++) {
        size_t n = strlen(PK[k]);
        if (!strncmp(key, PK[k], n) && isdigit((unsigned char)key[n]) && !key[n + 1]) {
            int i = key[n] - '0';
            if (i > 3) return 0;
            const Kid *kd = &M.k[i];
            int v[] = {(int)kd->x, (int)kd->y, (int)kd->z, kd->state, kd->stars, kd->held, kd->who, kd->hold_t,
                       kd->charge, kd->timer, kd->face, kd->aimx, kd->aimy};
            *out = v[k];
            return 1;
        }
    }
    /* per bag: bagst0 bagx0 bagy0 bagkind0 bagteam0 (bags 0-11) */
    static const char *const PB[] = {"bagst", "bagx", "bagy", "bagkind", "bagteam", "bagz"};
    for (int k = 0; k < ARRAY_LEN(PB); k++) {
        size_t n = strlen(PB[k]);
        if (!strncmp(key, PB[k], n) && isdigit((unsigned char)key[n])) {
            int b = atoi(key + n);
            if (b < 0 || b >= SKID_MAX_BAGS) return 0;
            const Bag *g = &M.bag[b];
            int v[] = {g->state, (int)g->x, (int)g->y, g->kind, g->team, (int)g->z};
            *out = v[k];
            return 1;
        }
    }
    /* the tournament's teams: opp<round>_<0|1>, pool0-4, teama0-1, teamb0-1 */
    if (!strncmp(key, "opp", 3) && isdigit((unsigned char)key[3]) && key[4] == '_') {
        int r = key[3] - '0', s = key[5] - '0';
        if (r < 0 || r >= ROUNDS || s < 0 || s > 1) return 0;
        *out = opp[r][s];
        return 1;
    }
    if (!strncmp(key, "rs", 2) && isdigit((unsigned char)key[2]) && key[3] == '_') {
        *out = round_score[iclamp(key[2] - '0', 0, ROUNDS - 1)][key[4] == '1'];
        return 1;
    }
    if (!strncmp(key, "pool", 4) && isdigit((unsigned char)key[4])) { *out = pool[iclamp(key[4] - '0', 0, 4)]; return 1; }
    if (!strncmp(key, "owner", 5) && isdigit((unsigned char)key[5])) { *out = owner[iclamp(key[5] - '0', 0, 4)]; return 1; }
    if (!strncmp(key, "teama", 5) && isdigit((unsigned char)key[5])) { *out = team_a[key[5] == '1']; return 1; }
    if (!strncmp(key, "teamb", 5) && isdigit((unsigned char)key[5])) { *out = team_b[key[5] == '1']; return 1; }
    return 0;
}

static void cheat_match(int m, const int *who, int level) {
    mode = m;
    team_a[0] = who[0];
    team_a[1] = who[1];
    team_b[0] = who[2];
    team_b[1] = who[3];
    opp[0][0] = who[2];
    opp[0][1] = who[3];
    round_i = iclamp(level, 0, 5);
    opp[round_i][0] = who[2];
    opp[round_i][1] = who[3];
    start_match();
}

static int skid_cheat(const char *cmd) {
    int a, b, c, d, e;
    float fx, fy, fvx, fvy;
    int w[4];
    if (sscanf(cmd, "match %d %d %d %d %d", &w[0], &w[1], &w[2], &w[3], &e) >= 4) {
        if (sscanf(cmd, "match %d %d %d %d %d", &w[0], &w[1], &w[2], &w[3], &e) < 5) e = 0;
        cheat_match(MODE_1P, w, e);
        return 1;
    }
    if (sscanf(cmd, "coop %d %d %d %d", &w[0], &w[1], &w[2], &w[3]) == 4) { cheat_match(MODE_COOP, w, 0); return 1; }
    if (sscanf(cmd, "versus %d %d %d %d", &w[0], &w[1], &w[2], &w[3]) == 4) { cheat_match(MODE_VS, w, 0); return 1; }
    if (sscanf(cmd, "tourney %d %d", &a, &b) == 2) {
        /* a 1P tournament with a built team */
        mode = MODE_1P;
        build_mode = true;
        team_a[0] = a;
        team_a[1] = b;
        setup_tournament();
        return 1;
    }
    if (sscanf(cmd, "cooptourney %d %d", &a, &b) == 2) {
        mode = MODE_COOP;
        build_mode = true;
        team_a[0] = a;
        team_a[1] = b;
        setup_tournament();
        return 1;
    }
    if (sscanf(cmd, "opp %d %d %d", &a, &b, &c) == 3) { opp[iclamp(a, 0, 5)][0] = b; opp[iclamp(a, 0, 5)][1] = c; return 1; }
    if (sscanf(cmd, "round %d", &a) == 1) {
        round_i = iclamp(a, 0, 5);
        wins = round_i;
        set_state(S_BRACKET);
        music_play(SKID_MUS_BRACKET);
        return 1;
    }
    if (sscanf(cmd, "win %d", &a) == 1) {
        /* end the match now: team a reaches the goal */
        M.score[a & 1] = M.rules.goal - 1;
        M.state = MS_PLAY;
        M.score[a & 1]++;
        M.state = MS_OVER;
        M.state_t = 0;
        M.winner = a & 1;
        for (int i = 0; i < 4; i++) M.k[i].state = M.k[i].team == (unsigned)(a & 1) ? KS_WIN : KS_SAD;
        return 1;
    }
    if (sscanf(cmd, "score %d %d", &a, &b) == 2) { M.score[0] = a; M.score[1] = b; return 1; }
    if (sscanf(cmd, "kid %d %d %d", &a, &b, &c) == 3) { M.k[a & 3].x = (float)b; M.k[a & 3].y = (float)c; return 1; }
    if (sscanf(cmd, "stars %d %d", &a, &b) == 2) { M.k[a & 3].stars = b; return 1; }
    if (sscanf(cmd, "give %d %d", &a, &b) == 2) {
        for (int n = 0; n < b; n++) {
            int g = skid_bag_add(&M);
            if (g < 0) break;
            M.bag[g].state = BG_HELD;
            M.bag[g].owner = (int8_t)(a & 3);
            M.bag[g].team = (int8_t)M.k[a & 3].team;
            M.k[a & 3].held++;
        }
        M.k[a & 3].hold_t = 0;
        return 1;
    }
    if (sscanf(cmd, "bag %f %f %f %f %d %d", &fx, &fy, &fvx, &fvy, &a, &b) >= 5) {
        if (sscanf(cmd, "bag %f %f %f %f %d %d", &fx, &fy, &fvx, &fvy, &a, &b) < 6) b = 0;
        int g = skid_bag_add(&M);
        if (g < 0) return 1;
        Bag *bg = &M.bag[g];
        bg->state = BG_SLIDE;
        bg->x = fx; bg->y = fy; bg->vx = fvx; bg->vy = fvy; bg->z = 2;
        bg->team = (int8_t)a;
        bg->heavy = b != 0;
        return 1;
    }
    if (sscanf(cmd, "loose %f %f", &fx, &fy) == 2) {
        int g = skid_bag_add(&M);
        if (g >= 0) { M.bag[g].x = fx; M.bag[g].y = fy; }
        return 1;
    }
    if (sscanf(cmd, "juice %f %f", &fx, &fy) == 2) {
        int n = skid_item_add(&M, IT_JUICE);
        if (n >= 0) { M.it[n].x = fx; M.it[n].y = fy; M.it[n].life = 1500; }
        return 1;
    }
    if (sscanf(cmd, "puddle %f %f", &fx, &fy) == 2) {
        int n = skid_item_add(&M, IT_PUDDLE);
        if (n >= 0) { M.it[n].x = fx; M.it[n].y = fy; M.it[n].r = 13; M.it[n].life = 600; }
        return 1;
    }
    if (sscanf(cmd, "marble %f %f", &fx, &fy) == 2) {
        int n = skid_item_add(&M, IT_MARBLE);
        if (n >= 0) { M.it[n].x = fx; M.it[n].y = fy; M.it[n].r = 4; M.it[n].life = 1200; }
        return 1;
    }
    if (sscanf(cmd, "coachthrow %d %f %f", &a, &fx, &fy) == 3) { skid_coach_throw(&M, a, fx, fy); return 1; }
    if (sscanf(cmd, "coach %d", &a) == 1) { M.coach_off = a == 0; return 1; }
    if (!strcmp(cmd, "clearbags")) {
        for (int g = 0; g < SKID_MAX_BAGS; g++) M.bag[g].state = BG_GONE;
        for (int i = 0; i < 4; i++) M.k[i].held = 0;
        return 1;
    }
    if (!strcmp(cmd, "play")) { M.state = MS_PLAY; M.state_t = 0; return 1; }
    if (sscanf(cmd, "freeze %d", &a) == 1) { freeze_mask |= 1u << (a & 3); return 1; }
    if (sscanf(cmd, "thaw %d", &a) == 1) { freeze_mask &= ~(1u << (a & 3)); return 1; }
    if (sscanf(cmd, "pin %d", &a) == 1) { pin_mask |= 1u << (a & 3); return 1; }
    if (sscanf(cmd, "level %d", &a) == 1) { M.level[1] = a; return 1; }
    if (sscanf(cmd, "ctrl %d %d", &a, &b) == 2) { M.ctrl[a & 1] = b; return 1; }
    if (sscanf(cmd, "hold %d %d", &a, &b) == 2) { M.k[a & 3].hold_t = b; return 1; }
    if (sscanf(cmd, "rules %d %d %d %d %d", &a, &b, &c, &d, &e) == 5) {
        M.rules.goal = a; M.rules.max_bags = b; M.rules.cost = c; M.rules.mate_ai = d; M.rules.mate_jump = e;
        return 1;
    }
    if (!strncmp(cmd, "code ", 5)) {
        for (int i = 0; i < ARRAY_LEN(CODES); i++)
            if (!strcmp(cmd + 5, CODES[i].code)) codes |= (unsigned)CODES[i].bit;
        return 1;
    }
    if (!strcmp(cmd, "demo")) { start_demo(); return 1; }
    if (!strcmp(cmd, "idle")) { idle_t = DEMO_IDLE - 5; return 1; }
    if (sscanf(cmd, "sheet %d", &a) == 1) { sheet_mode = true; sheet_page = a & 1; return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    if (!strcmp(cmd, "save")) { save_now(); return 1; }
    return 0;
}

const GameDef GAME_SKIDKIDS = {
    "skidkids",
    "SKID KIDS",
    "1986",
    "SPORTS",
    "TWO ON TWO IN THE GYM: SKID BAGS AT THEM, JUMP THEIRS FOR STARS. FIRST TO 15.",
    {"WIN 3 MATCHES IN ONE TOURNAMENT", "WIN THE TOURNAMENT", "WIN IT WITH A BLOWOUT IN EVERY MATCH"},
    "D-PAD\tRUN (AIM WHILE WINDING UP)\n"
    GLYPH_B "\tPICK UP / PASS / SWAP KIDS\n"
    "HOLD " GLYPH_B "\tWIND UP, LET GO TO THROW\n"
    GLYPH_A "\tJUMP (THE WHOLE TEAM)\n"
    GLYPH_A " IN THE AIR\tSPECIAL MOVE (1 STAR)\n"
    GLYPH_A " WINDING UP\tCALL THE THROW OFF\n"
    "START\tPAUSE\n"
    "2P KEYS: WASD F G / ARROWS K L",
    C_AMBER, C_RED,
    skid_load, skid_start, skid_update, skid_draw, skid_quit, skid_label, skid_query, skid_cheat,
    "HOT FOOT", 26,
    NULL,
};
