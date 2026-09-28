/* WOBBLE DERBY - betting on wobblers at Crater Downs, with a tip booth, a
 * fixer in the alley, a lender and a coach.
 * Cartridge 47 of UFO 40, a tribute to Quibble Race (UFO 50 #47).
 * See docs/games/47-wobble-derby.md. The rules live in wobble_logic.c; this
 * file is the screens, the flow, saving, the demo player and test hooks. */
#include "wobble.h"

enum {
    S_TITLE, S_SETUP, S_BOOK, S_ROUND, S_PASS, S_PADDOCK, S_CPU, S_PARADE, S_RACE, S_NEWS, S_PAYOUT,
    S_FINAL
};
/* what the paddock's right-hand panel is doing */
enum {
    M_MAIN, M_BET_LANE, M_BET_AMT, M_TIP_LANE, M_TIP_SHOW, M_FIX_JOB, M_FIX_LANE, M_LENDER, M_SPONSOR,
    M_COACH
};
enum { MI_BET, MI_TIPS, MI_FIXER, MI_LENDER, MI_SPONSOR, MI_COACH, MI_DONE, MI_COUNT };
static const char *const MENU[MI_COUNT] = {"BET WINDOW", "TIP BOOTH", "THE FIXER", "THE LENDER", "SPONSOR", "COACH", "DONE"};

typedef struct Save {
    uint32_t magic;
    uint8_t has_run;
    uint8_t chars[WB_PLAYERS];
    uint16_t games, wins, sponsor_wins;
    int32_t best;         /* the best final purse in a game you won */
    WbGame run;
} Save;
#define SAVE_MAGIC 0x57424401u

static Save sv;
static WbGame G;
static int state, state_t, frame_t, sub, sub_t;
static int title_sel, setup_row, setup_len = 1;
static uint8_t setup_chr[WB_PLAYERS] = {0, 2, 6};
static uint8_t setup_human[WB_PLAYERS] = {1, 0, 0};
static int menu_sel, lane_sel, amt, job_sel, list_sel, list_top, lend_sel, coach_sel, tip_lane;
static int news_shown;
static const char *msg;
static int msg_t;
static bool sure_poison;
/* race presentation */
static char call_line[64];
static int call_t;
static int prev_trip[WB_FIELD], prev_dead[WB_FIELD], prev_done[WB_FIELD];
static int shake_t, leader;
static bool public_cards;   /* the CPUs' turns: no punter's own notes on the cards */

#define CARD_X 4
#define CARD_W 196
#define CARD_H 40
#define CARD_Y(l) (14 + (l) * 42)
#define PANEL_X 204
#define LANE_Y(l) (86 + (l) * 26)
#define START_X 24

/* ------------------------------------------------------------------ */
/* helpers                                                              */

static const char *money(int32_t v) {
    static char bufs[4][20];
    static int k;
    char *b = bufs[k++ & 3];
    char tmp[16];
    int32_t a = v < 0 ? -v : v;
    snprintf(tmp, sizeof tmp, "%ld", (long)a);
    int n = (int)strlen(tmp), o = 0;
    if (v < 0) b[o++] = '-';
    b[o++] = '$';
    for (int i = 0; i < n; i++) {
        b[o++] = tmp[i];
        if ((n - 1 - i) % 3 == 0 && i < n - 1) b[o++] = ',';
    }
    b[o] = 0;
    return b;
}

static const char *pname(int p) { return WB_CHAR[G.pl[p].chr % WB_CHARS].name; }
static const char *rname(int racer) { return WB_DEF[racer % WB_RACERS].name; }
static int cur_p(void) { return G.turn % WB_PLAYERS; }

static const char *clumsy_word(int racer) {
    const WbDef *d = &WB_DEF[racer];
    if (d->stab_hi - d->stab_lo > 2) return "ANYONE'S GUESS";
    switch ((d->stab_lo + 1) / 2) {
    case 1: return "SURE-FOOTED";
    case 2: return "STEADY";
    case 3: return "SO-SO";
    case 4: return "CLUMSY";
    default: return "A DISASTER";
    }
}

static void say(const char *m) { msg = m; msg_t = 120; }

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    static Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else { memset(&sv, 0, sizeof sv); sv.magic = SAVE_MAGIC; }
}

static void keep_run(void) {
    sv.run = G;
    sv.has_run = 1;
    save_now();
}

/* ------------------------------------------------------------------ */
/* flow                                                                 */

static void goto_state(int s) {
    state = s;
    state_t = 0;
    input_consume();
}

static void begin_turn(void);

static void round_card(void) {
    goto_state(S_ROUND);
    music_play(WB_MUS_PADDOCK);
    game_set_pausable(true);
}

/* the next punter in the seat order, or the parade when all have gone */
static void next_turn(void) {
    G.turn++;
    if (G.turn >= WB_PLAYERS) {
        goto_state(S_PARADE);
        return;
    }
    begin_turn();
}

static void begin_turn(void) {
    int p = cur_p();
    if (!G.pl[p].human) {
        wb_cpu_turn(&G, p);
        goto_state(S_CPU);
        return;
    }
    sub = M_MAIN;
    sub_t = 0;
    menu_sel = 0;
    if (G.humans > 1) goto_state(S_PASS);
    else goto_state(S_PADDOCK);
}

static void start_game(int nraces, int humans) {
    uint64_t seed = (uint64_t)rng_next(&g_rng) << 32 | rng_next(&g_rng);
    uint8_t chars[WB_PLAYERS];
    /* the human seats first, then the rivals */
    int k = 0;
    for (int p = 0; p < WB_PLAYERS; p++) if (setup_human[p]) chars[k++] = setup_chr[p];
    for (int p = 0; p < WB_PLAYERS; p++) if (!setup_human[p]) chars[k++] = setup_chr[p];
    memcpy(sv.chars, setup_chr, sizeof sv.chars);
    wb_new(&G, nraces, humans, chars, seed);
    keep_run();
    goto_state(S_BOOK);
    music_play(WB_MUS_PADDOCK);
    game_set_pausable(true);
}

/* the end of the meeting: the goals and the records */
static void finish_meeting(void) {
    sv.has_run = 0;
    if (sv.games < 65535) sv.games++;
    bool solo = G.humans == 1;
    if (wb_rank(&G, 0) == 1 && G.pl[0].human) {
        if (sv.wins < 65535) sv.wins++;
        if (wb_final(&G, 0) > sv.best) sv.best = wb_final(&G, 0);
        if (solo && G.nraces >= 6) game_award(GOAL_SAUCER);
        if (solo && G.nraces == 6 && wb_final(&G, 0) >= 10000) game_award(GOAL_ALIEN);
    }
    save_now();
}

static void after_payout(void) {
    if (wb_next_round(&G)) {
        keep_run();
        round_card();
        return;
    }
    finish_meeting();
    goto_state(S_FINAL);
    bool human_won = false;
    for (int p = 0; p < WB_PLAYERS; p++) human_won |= G.pl[p].human && wb_rank(&G, p) == 1;
    music_play(human_won ? WB_MUS_FINAL : WB_MUS_LOSE);
}

static void settle_race(void) {
    int before[WB_PLAYERS];
    for (int p = 0; p < WB_PLAYERS; p++) before[p] = G.pl[p].sponsor_wins;
    wb_race_settle(&G);
    for (int p = 0; p < WB_PLAYERS; p++)
        if (G.pl[p].human && G.pl[p].sponsor_wins > before[p]) {
            if (sv.sponsor_wins < 65535) sv.sponsor_wins++;
            game_award(GOAL_BEACON);
        }
    news_shown = imin(G.nnews, 4);
    goto_state(S_NEWS);
    music_restart(WB_MUS_NEWS);
}

/* ------------------------------------------------------------------ */
/* the paddock                                                          */

static int bet_room(void) {
    WbPlayer *P = &G.pl[cur_p()];
    int have = P->cash + (P->bet_on != WB_NONE ? P->bet : 0);
    int cap = wb_cap(&G);
    return cap ? imin(cap, have) : have;
}

static void lane_nav(void) {
    if (btn_repeat(BTN_UP)) { lane_sel = (lane_sel + WB_FIELD - 1) % WB_FIELD; sfx_play_name("wb_move"); }
    if (btn_repeat(BTN_DOWN)) { lane_sel = (lane_sel + 1) % WB_FIELD; sfx_play_name("wb_move"); }
}

/* the sponsor list: every wobbler still racing, and the gone ones last */
static int book_order[WB_RACERS];
static int book_n;
static void build_book(void) {
    book_n = 0;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < WB_RACERS; i++)
            if (wb_active(&G, i) == (pass == 0)) book_order[book_n++] = i;
}

static int my_sponsored(int p, int *out) {
    int n = 0;
    for (int i = 0; i < WB_RACERS; i++)
        if (G.r[i].sponsor == p + 1 && wb_active(&G, i)) out[n++] = i;
    return n;
}

static void open_sub(int s) {
    sub = s;
    sub_t = 0;
    sfx_play_name("ui_ok");
}

static void update_paddock(void) {
    int p = cur_p();
    WbPlayer *P = &G.pl[p];
    sub_t++;
    switch (sub) {
    case M_MAIN:
        if (btn_repeat(BTN_UP)) { menu_sel = (menu_sel + MI_COUNT - 1) % MI_COUNT; sfx_play_name("wb_move"); }
        if (btn_repeat(BTN_DOWN)) { menu_sel = (menu_sel + 1) % MI_COUNT; sfx_play_name("wb_move"); }
        if (!btnp(BTN_A)) break;
        switch (menu_sel) {
        case MI_BET:
            lane_sel = P->bet_on != WB_NONE ? P->bet_on : 0;
            open_sub(M_BET_LANE);
            break;
        case MI_TIPS:
            lane_sel = 0;
            open_sub(M_TIP_LANE);
            break;
        case MI_FIXER:
            if (P->job != J_NONE) { sfx_play_name("ui_error"); say("THE FIXER: ONE JOB A RACE, FRIEND."); break; }
            job_sel = 1;
            open_sub(M_FIX_JOB);
            sfx_play_name("wb_shady");
            break;
        case MI_LENDER: lend_sel = P->debt > 0 ? 1 : 0; open_sub(M_LENDER); break;
        case MI_SPONSOR: build_book(); list_sel = 0; list_top = 0; open_sub(M_SPONSOR); break;
        case MI_COACH: {
            int mine[WB_RACERS];
            if (!my_sponsored(p, mine)) { sfx_play_name("ui_error"); say("THE COACH ONLY TRAINS WOBBLERS YOU SPONSOR."); break; }
            coach_sel = 0;
            open_sub(M_COACH);
            break;
        }
        case MI_DONE:
            sfx_play_name("ui_ok");
            if (P->bet_on == WB_NONE) say("NO BET THIS RACE.");
            next_turn();
            break;
        }
        break;
    case M_BET_LANE:
        lane_nav();
        if (btnp(BTN_B)) { sub = M_MAIN; sfx_play_name("ui_back"); break; }
        if (btnp(BTN_A)) {
            int room = bet_room();
            if (room < 10) { sfx_play_name("ui_error"); say("NOT ENOUGH CASH TO BET."); break; }
            amt = P->bet_on == lane_sel ? P->bet : imin(room, 10);
            open_sub(M_BET_AMT);
        }
        break;
    case M_BET_AMT: {
        int room = bet_room();
        if (btn_repeat(BTN_RIGHT)) { amt += 10; sfx_play_name("wb_move"); }
        if (btn_repeat(BTN_LEFT)) { amt -= 10; sfx_play_name("wb_move"); }
        if (btn_repeat(BTN_UP)) { amt += 100; sfx_play_name("wb_move"); }
        if (btn_repeat(BTN_DOWN)) { amt -= 100; sfx_play_name("wb_move"); }
        amt = iclamp(amt, 0, room);
        if (btnp(BTN_B)) { sub = M_BET_LANE; sfx_play_name("ui_back"); break; }
        if (btnp(BTN_A)) {
            if (wb_bet(&G, p, lane_sel, amt)) {
                sfx_play_name("wb_coin");
                say(amt ? "BET PLACED." : "BET TAKEN BACK.");
                sub = M_MAIN;
                menu_sel = MI_DONE;
            } else sfx_play_name("ui_error");
        }
        break;
    }
    case M_TIP_LANE:
        lane_nav();
        if (btnp(BTN_B)) { sub = M_MAIN; sfx_play_name("ui_back"); break; }
        if (btnp(BTN_A)) {
            if (P->tips & (1 << lane_sel)) { tip_lane = lane_sel; open_sub(M_TIP_SHOW); break; }
            if (!wb_tip(&G, p, lane_sel)) { sfx_play_name("ui_error"); say("THE TIP BOOTH WANTS CASH UP FRONT."); break; }
            sfx_play_name("wb_coin");
            tip_lane = lane_sel;
            open_sub(M_TIP_SHOW);
        }
        break;
    case M_TIP_SHOW:
        if (sub_t > 10 && (btnp(BTN_A) || btnp(BTN_B))) { sub = M_TIP_LANE; sfx_play_name("ui_back"); }
        break;
    case M_FIX_JOB:
        if (btn_repeat(BTN_UP)) { job_sel = job_sel <= 1 ? J_COUNT - 1 : job_sel - 1; sfx_play_name("wb_move"); }
        if (btn_repeat(BTN_DOWN)) { job_sel = job_sel >= J_COUNT - 1 ? 1 : job_sel + 1; sfx_play_name("wb_move"); }
        if (btnp(BTN_B)) { sub = M_MAIN; sfx_play_name("ui_back"); break; }
        if (btnp(BTN_A)) {
            if (P->cash < WB_JOB_COST[job_sel]) { sfx_play_name("ui_error"); say("THE FIXER: CASH FIRST, FRIEND."); break; }
            lane_sel = 0;
            open_sub(M_FIX_LANE);
        }
        break;
    case M_FIX_LANE:
        lane_nav();
        if (btnp(BTN_B)) { sub = M_FIX_JOB; sfx_play_name("ui_back"); break; }
        if (btnp(BTN_A)) {
            if (wb_job(&G, p, job_sel, lane_sel)) {
                sfx_play_name("wb_shady");
                say("THE FIXER: CONSIDER IT DONE.");
                sub = M_MAIN;
            } else sfx_play_name("ui_error");
        }
        break;
    case M_LENDER:
        if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { lend_sel ^= 1; sfx_play_name("wb_move"); }
        if (btnp(BTN_B)) { sub = M_MAIN; sfx_play_name("ui_back"); break; }
        if (btnp(BTN_A)) {
            if (lend_sel == 0) {
                if (wb_borrow(&G, p)) { sfx_play_name("wb_coin"); say("THE LENDER: FIFTEEN PER CENT A RACE, DEAR."); sub = M_MAIN; }
                else { sfx_play_name("ui_error"); say("THE LENDER: PAY ME BACK FIRST, DEAR."); }
            } else {
                if (wb_repay(&G, p)) { sfx_play_name("wb_coin"); say("THE LENDER: A PLEASURE, AS ALWAYS."); sub = M_MAIN; }
                else { sfx_play_name("ui_error"); say(P->debt ? "THE LENDER: YOU CAN'T COVER IT, DEAR." : "YOU OWE NOTHING."); }
            }
        }
        break;
    case M_SPONSOR:
        if (btn_repeat(BTN_UP) && list_sel > 0) { list_sel--; sfx_play_name("wb_move"); }
        if (btn_repeat(BTN_DOWN) && list_sel < book_n - 1) { list_sel++; sfx_play_name("wb_move"); }
        if (list_sel < list_top) list_top = list_sel;
        if (list_sel >= list_top + 9) list_top = list_sel - 8;
        if (btnp(BTN_B)) { sub = M_MAIN; sfx_play_name("ui_back"); break; }
        if (btnp(BTN_A)) {
            int r = book_order[list_sel];
            if (wb_sponsor(&G, p, r)) { sfx_play_name("wb_coin"); say("A NEW NAME ON THE STABLE DOOR."); }
            else {
                sfx_play_name("ui_error");
                if (!wb_active(&G, r)) say("THAT ONE IS NO LONGER RACING.");
                else if (G.r[r].sponsor) say("THAT ONE ALREADY HAS A SPONSOR.");
                else if (wb_sponsored_count(&G, p) >= WB_MAX_SPONSOR) say("THREE IS THE MOST YOU CAN SPONSOR.");
                else say("NOT ENOUGH CASH.");
            }
        }
        break;
    case M_COACH: {
        int mine[WB_RACERS];
        int n = my_sponsored(p, mine);
        if (n == 0) { sub = M_MAIN; break; }
        if (coach_sel >= n) coach_sel = n - 1;
        if (btn_repeat(BTN_UP)) { coach_sel = (coach_sel + n - 1) % n; sfx_play_name("wb_move"); }
        if (btn_repeat(BTN_DOWN)) { coach_sel = (coach_sel + 1) % n; sfx_play_name("wb_move"); }
        if (btnp(BTN_B)) { sub = M_MAIN; sfx_play_name("ui_back"); break; }
        if (btnp(BTN_A)) {
            int r = mine[coach_sel];
            if (wb_train(&G, p, r)) { sfx_play_name("wb_train"); say("THE COACH: THAT'S THE STUFF!"); }
            else {
                sfx_play_name("ui_error");
                if (G.r[r].trained) say("ONE SESSION A RACE FOR EACH WOBBLER.");
                else if (P->trains >= WB_TRAINS_A_ROUND) say("THE COACH HAS ROOM FOR TWO A RACE.");
                else say("NOT ENOUGH CASH.");
            }
        }
        break;
    }
    }
}

/* ------------------------------------------------------------------ */
/* the race                                                             */

static void call(const char *s) {
    snprintf(call_line, sizeof call_line, "%s", s);
    call_t = 90;
}

static void start_race(void) {
    goto_state(S_RACE);
    wb_race_begin(&G);
    if (sure_poison)
        for (int p = 0; p < WB_PLAYERS; p++)
            if (G.pl[p].job == J_NIGHTSHADE && !G.race.lane[G.pl[p].job_lane].minded) G.race.lane[G.pl[p].job_lane].poison_at = 200;
    for (int l = 0; l < WB_FIELD; l++) prev_trip[l] = prev_dead[l] = prev_done[l] = 0;
    call_line[0] = 0;
    call_t = 0;
    leader = -1;
    music_stop();
    music_restart(WB_MUS_BELL);
    for (int p = 0; p < WB_PLAYERS; p++)
        if (G.pl[p].fine) { sfx_play_name("wb_fine"); break; }
}

#define COUNTDOWN 150
static void update_race(void) {
    WbRace *rc = &G.race;
    if (call_t > 0) call_t--;
    if (shake_t > 0) shake_t--;
    if (state_t == COUNTDOWN - 30) { sfx_play_name("wb_gun"); music_play(WB_MUS_RACE); }
    if (state_t < COUNTDOWN - 30) return;
    if (!rc->over) {
        wb_race_step(&G);
        char b[64];
        for (int l = 0; l < WB_FIELD; l++) {
            WbLane *L = &rc->lane[l];
            int racer = rc->field[l];
            if ((int)L->trips > prev_trip[l]) {
                prev_trip[l] = L->trips;
                snprintf(b, sizeof b, "%s GOES DOWN!", rname(racer));
                call(b);
                sfx_play_name("wb_trip");
            }
            if (L->dead && !prev_dead[l]) {
                prev_dead[l] = 1;
                snprintf(b, sizeof b, L->dead == OUT_METEOR ? "%s IS HIT!" : "%s COLLAPSES!", rname(racer));
                call(b);
                sfx_play_name(L->dead == OUT_METEOR ? "wb_boom" : "wb_splat");
            }
            if (L->done && !prev_done[l]) {
                prev_done[l] = 1;
                if (rc->winner == l) {
                    snprintf(b, sizeof b, "%s WINS!", rname(racer));
                    call(b);
                    sfx_play_name("wb_cheer");
                }
            }
        }
        for (int i = 0; i < rc->nmet; i++)
            if (rc->met[i].land == rc->t) { shake_t = 12; sfx_play_name("wb_boom"); }
        /* the caller follows the lead */
        int lead = -1;
        for (int l = 0; l < WB_FIELD; l++)
            if (!rc->lane[l].dead && (lead < 0 || rc->lane[l].x > rc->lane[lead].x)) lead = l;
        if (lead >= 0 && lead != leader && rc->winner == WB_NONE) {
            if (leader >= 0 && call_t < 30 && rc->t > 60) {
                snprintf(b, sizeof b, "%s TAKES THE LEAD!", rname(rc->field[lead]));
                call(b);
            }
            leader = lead;
        }
        if (rc->winner == WB_NONE && lead >= 0 && rc->lane[lead].x / 256 == WB_TRACK_PX - 60 && call_t < 30)
            call("INTO THE LAST STRETCH!");
        if (rc->over) {
            if (rc->winner == WB_NONE) call("NOBODY FINISHES!");
            state_t = 10000;
            music_stop();
            bool mine = false;
            for (int p = 0; p < WB_PLAYERS; p++) mine |= G.pl[p].human && G.pl[p].bet_on == rc->winner && rc->winner != WB_NONE;
            music_restart(mine ? WB_MUS_WIN : WB_MUS_LOSE);
        }
        return;
    }
    if (state_t > 10000 + 100 || (state_t > 10000 + 30 && btnp(BTN_A))) settle_race();
}

/* ------------------------------------------------------------------ */
/* menus                                                                */

static void update_title(void) {
    game_set_pausable(false);
    int n = sv.has_run ? 2 : 1;
    if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { if (n > 1) { title_sel ^= 1; sfx_play_name("wb_move"); } }
    if (title_sel >= n) title_sel = 0;
    if (btnp(BTN_B) && state_t > 10) { game_exit_to_library(); return; }
    if (state_t > 20 && btnp(BTN_A)) {
        sfx_play_name("ui_ok");
        if (sv.has_run && title_sel == 0) {
            G = sv.run;
            game_set_pausable(true);
            round_card();
            if (G.turn >= WB_PLAYERS) goto_state(S_PARADE);   /* every punter had gone */
            return;
        }
        if (sv.chars[0] || sv.chars[1] || sv.chars[2]) memcpy(setup_chr, sv.chars, sizeof setup_chr);
        if (setup_chr[0] == setup_chr[1] || setup_chr[1] == setup_chr[2] || setup_chr[0] == setup_chr[2]) {
            setup_chr[0] = 0; setup_chr[1] = 2; setup_chr[2] = 6;
        }
        setup_row = 0;
        goto_state(S_SETUP);
    }
}

static bool chr_taken(int c, int except) {
    for (int p = 0; p < WB_PLAYERS; p++) if (p != except && setup_chr[p] == c) return true;
    return false;
}

static void update_setup(void) {
    if (btn_repeat(BTN_UP)) { setup_row = (setup_row + 4) % 5; sfx_play_name("wb_move"); }
    if (btn_repeat(BTN_DOWN)) { setup_row = (setup_row + 1) % 5; sfx_play_name("wb_move"); }
    if (btnp(BTN_B)) { goto_state(S_TITLE); sfx_play_name("ui_back"); return; }
    int dx = btn_repeat(BTN_RIGHT) ? 1 : btn_repeat(BTN_LEFT) ? -1 : 0;
    if (setup_row < 3) {
        int p = setup_row;
        if (dx) {
            int c = setup_chr[p];
            do c = (c + dx + WB_CHARS) % WB_CHARS; while (chr_taken(c, p));
            setup_chr[p] = (uint8_t)c;
            sfx_play_name("wb_move");
        }
        if (btnp(BTN_A) && p > 0) { setup_human[p] ^= 1; sfx_play_name("ui_ok"); }
    } else if (setup_row == 3) {
        if (dx) { setup_len = (setup_len + dx + WB_LENGTHS) % WB_LENGTHS; sfx_play_name("wb_move"); }
    } else if (btnp(BTN_A)) {
        sfx_play_name("ui_ok");
        int humans = 0;
        for (int p = 0; p < WB_PLAYERS; p++) humans += setup_human[p];
        start_game(WB_LENGTH[setup_len], humans);
    }
}

static void wb_update(void) {
    frame_t++;
    state_t++;
    if (msg_t > 0) msg_t--;
    switch (state) {
    case S_TITLE: update_title(); break;
    case S_SETUP: update_setup(); break;
    case S_BOOK:
        if (state_t > 20 && btnp(BTN_A)) { sfx_play_name("ui_ok"); round_card(); }
        break;
    case S_ROUND:
        if (state_t > 20 && btnp(BTN_A)) { sfx_play_name("ui_ok"); begin_turn(); }
        break;
    case S_PASS:
        if (state_t > 20 && btnp(BTN_A)) { sfx_play_name("ui_ok"); goto_state(S_PADDOCK); }
        break;
    case S_PADDOCK: update_paddock(); break;
    case S_CPU:
        if (state_t > 50 || (state_t > 8 && btnp(BTN_A))) next_turn();
        break;
    case S_PARADE:
        if (state_t > 20 && btnp(BTN_A)) { sfx_play_name("ui_ok"); start_race(); }
        break;
    case S_RACE: update_race(); break;
    case S_NEWS:
        if (state_t > 15 && btnp(BTN_A)) {
            sfx_play_name("ui_ok");
            if (news_shown < G.nnews) { news_shown = imin(G.nnews, news_shown + 4); state_t = 0; }
            else { goto_state(S_PAYOUT); music_play(WB_MUS_PADDOCK); }
        }
        break;
    case S_PAYOUT:
        if (state_t > 20 && btnp(BTN_A)) { sfx_play_name("ui_ok"); after_payout(); }
        break;
    case S_FINAL:
        if (state_t > 60 && btnp(BTN_A)) { sfx_play_name("ui_ok"); goto_state(S_TITLE); music_play(WB_MUS_TITLE); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */


static void face_box(int chr, int x, int y) {
    gfx_rect(x, y, 36, 36, C_DUSK);
    gfx_rectb(x, y, 36, 36, C_INK);
    wb_draw_face(chr, x + 2, y + 2, 2);
}
static void npc_box(int spr, int x, int y) {
    gfx_rect(x, y, 36, 36, C_DUSK);
    gfx_rectb(x, y, 36, 36, C_INK);
    spr_draw_scaled(&wb_spr[spr], x + 2, y + 2, 2, 0);
}

static void draw_backdrop(int t) {
    gfx_cls(C_NIGHT);
    for (int i = 0; i < 40; i++) {
        int x = (i * 97 + 13) % 320, y = (i * 53 + 7) % 180;
        gfx_pset(x, y, (t / 20 + i) % 7 ? C_DUSK : C_GREY);
    }
}

/* ---- the racer cards ---- */
static void draw_card(int l, bool hi) {
    int p = cur_p();
    WbPlayer *P = &G.pl[p];
    int racer = G.race.field[l];
    int x = CARD_X, y = CARD_Y(l);
    ui_panel(x, y, CARD_W, CARD_H, hi ? C_DUSK : C_NIGHT, hi ? C_YELLOW : C_SLATE);
    gfx_rect(x + 3, y + 3, 10, 9, C_INK);
    char b[48];
    snprintf(b, sizeof b, "%d", l + 1);
    text_draw(b, x + 6, y + 4, C_WHITE);
    int bob = ((frame_t / 12 + l) % 2);
    wb_draw_racer(racer, (frame_t / 20 + l) % 3 == 0 ? WA_IDLE : WA_RUN1, x + 6, y + 18 - bob, 0);
    text_draw(rname(racer), x + 26, y + 3, C_WHITE);
    const WbRacer *R = &G.r[racer];
    snprintf(b, sizeof b, "WON %d OF %d", R->wins, R->races);
    tiny_draw(b, x + 26, y + 13, C_LIGHT);
    if (!public_cards && (P->tips & (1 << l))) {
        snprintf(b, sizeof b, "SPEED %d-%d  %s", wb_spd_lo(&G, racer), wb_spd_hi(&G, racer), clumsy_word(racer));
        tiny_draw(b, x + 26, y + 21, C_CYAN);
    }
    if (!public_cards && P->bet_on == l) {
        snprintf(b, sizeof b, "YOUR BET %s", money(P->bet));
        tiny_draw(b, x + 26, y + 30, C_YELLOW);
    }
    if (!public_cards && P->job != J_NONE && P->job_lane == l) {
        static const int ICON[J_COUNT] = {0, WA_PEP, WA_PEEL, WA_NOBBLE, WA_FIZZ, WA_NIGHTSHADE, WA_MINDER};
        spr_draw(&wb_spr[ICON[P->job]], x + 116, y + 29, 0);
    }
    if (R->sponsor) {
        gfx_rect(x + 128, y + 21, 18, 18, C_INK);
        wb_draw_face(G.pl[R->sponsor - 1].chr, x + 129, y + 22, 1);
    }
    snprintf(b, sizeof b, "%d:1", G.race.odds[l]);
    int w = text_width_scaled(b, 2);
    text_draw_scaled(b, x + CARD_W - 6 - w + 1, y + 11, C_INK, 2);
    text_draw_scaled(b, x + CARD_W - 6 - w, y + 10, C_YELLOW, 2);
    tiny_draw("ODDS", x + CARD_W - 22, y + 3, C_GREY);
}

static void draw_topbar(void) {
    gfx_rect(0, 0, 320, 11, C_INK);
    char b[64];
    snprintf(b, sizeof b, "RACE %d OF %d", G.round + 1, G.nraces);
    text_draw(b, 4, 2, C_WHITE);
    int cap = wb_cap(&G);
    if (cap) snprintf(b, sizeof b, "BETS UP TO %s", money(cap));
    else snprintf(b, sizeof b, "LAST RACE: NO LIMIT");
    text_center(b, 196, 2, cap ? C_AMBER : C_RED);
}

static void draw_msg_line(const char *s, int col) {
    gfx_rect(0, 170, 320, 10, C_INK);
    if (s) text_center(s, 160, 171, col);
}

static const char *JOB_DESC[J_COUNT] = {
    "",
    "A SNACK THAT PUTS A SPRING IN ITS STEP FOR ONE RACE.",
    "LITTER ON ITS LANE. CLUMSY ONES TRIP ON IT. NO MINDER STOPS IT.",
    "A KICK THAT SLOWS IT DOWN FOR THE REST OF THE MEETING.",
    "PILLS THAT SEND IT RUNNING ALL OVER THE PLACE.",
    "IT MAY NOT FINISH THE RACE. OR ANY RACE, EVER AGAIN.",
    "A MINDER KEEPS EVERY HAND OFF IT, GOOD OR BAD. MEDDLERS ARE FINED.",
};

static void draw_right_panel(void) {
    int p = cur_p();
    WbPlayer *P = &G.pl[p];
    int x = PANEL_X, y = 14;
    ui_panel(x, y, 112, 154, C_NIGHT, C_SLATE);
    char b[64];
    switch (sub) {
    case M_TIP_SHOW: {
        npc_box(WA_BOOTH, x + 4, y + 4);
        text_draw("TIP BOOTH", x + 44, y + 6, C_CYAN);
        tiny_draw("BEEP. READING...", x + 44, y + 16, C_GREY);
        int racer = G.race.field[tip_lane];
        text_draw(rname(racer), x + 6, y + 46, C_WHITE);
        snprintf(b, sizeof b, "SPEED %d-%d", wb_spd_lo(&G, racer), wb_spd_hi(&G, racer));
        text_draw(b, x + 6, y + 58, C_YELLOW);
        text_draw(clumsy_word(racer), x + 6, y + 70, C_YELLOW);
        snprintf(b, sizeof b, "(CLUMSY %d-%d OF 10)", WB_DEF[racer].stab_lo, WB_DEF[racer].stab_hi);
        tiny_draw(b, x + 6, y + 81, C_GREY);
        static const char *const FORE[4] = {
            "CLEAR SKIES FOR THIS RACE.", "I SEE METEORS FALLING ON THIS RACE!",
            "A GARBAGE TRUCK WILL SPILL ON THE TRACK!", "SMOG WILL CHOKE THIS RACE!"};
        text_wrap(FORE[G.race.event & 3], x + 6, y + 94, 100, G.race.event ? C_ORANGE : C_LIGHT, 9);
        tiny_draw(GLYPH_A " OK", x + 6, y + 144, C_GREY);
        return;
    }
    case M_FIX_JOB: case M_FIX_LANE: {
        npc_box(WA_ALLEY, x + 4, y + 4);
        text_draw("THE FIXER", x + 44, y + 6, C_LIME);
        tiny_draw("ONE JOB A RACE.", x + 44, y + 16, C_GREY);
        snprintf(b, sizeof b, "CASH %s", money(P->cash));
        tiny_draw(b, x + 44, y + 26, C_YELLOW);
        static const int ICON[J_COUNT] = {0, WA_PEP, WA_PEEL, WA_NOBBLE, WA_FIZZ, WA_NIGHTSHADE, WA_MINDER};
        for (int j = 1; j < J_COUNT; j++) {
            int yy = y + 42 + (j - 1) * 11;
            bool on = j == job_sel;
            if (on) gfx_rect(x + 3, yy - 2, 106, 11, C_DUSK);
            spr_draw(&wb_spr[ICON[j]], x + 5, yy, 0);
            text_draw(WB_JOB_NAME[j], x + 16, yy, on ? C_YELLOW : C_LIGHT);
            snprintf(b, sizeof b, "$%d", WB_JOB_COST[j]);
            tiny_draw(b, x + 108 - tiny_width(b), yy + 2, C_AMBER);
        }
        text_wrap(JOB_DESC[job_sel], x + 5, y + 110, 104, C_GREY, 8);
        if (sub == M_FIX_LANE) tiny_draw("ON WHICH WOBBLER?", x + 6, y + 144, C_YELLOW);
        return;
    }
    case M_LENDER: {
        npc_box(WA_LENDER, x + 4, y + 4);
        text_draw("THE LENDER", x + 44, y + 6, C_YELLOW);
        tiny_draw("15% A RACE,", x + 44, y + 16, C_GREY);
        tiny_draw("COMPOUNDED.", x + 44, y + 23, C_GREY);
        snprintf(b, sizeof b, "BORROW %s", money(wb_loan_size(&G)));
        text_draw(b, x + 14, y + 50, lend_sel == 0 ? C_YELLOW : C_LIGHT);
        snprintf(b, sizeof b, "REPAY %s", money(P->debt));
        text_draw(b, x + 14, y + 62, lend_sel == 1 ? C_YELLOW : C_LIGHT);
        ui_cursor(x + 6, y + 50 + lend_sel * 12, frame_t);
        text_wrap("ONE LOAN AT A TIME. WHAT YOU STILL OWE AT THE END IS TAKEN FROM YOUR PURSE.", x + 6, y + 80, 100, C_GREY, 9);
        return;
    }
    case M_COACH: {
        npc_box(WA_COACH, x + 4, y + 4);
        text_draw("THE COACH", x + 44, y + 6, C_SKY);
        snprintf(b, sizeof b, "$%d A SESSION", WB_TRAIN_COST);
        tiny_draw(b, x + 44, y + 16, C_GREY);
        snprintf(b, sizeof b, "%d BOOKED OF %d", P->trains, WB_TRAINS_A_ROUND);
        tiny_draw(b, x + 44, y + 23, C_GREY);
        int mine[WB_RACERS];
        int n = my_sponsored(p, mine);
        for (int i = 0; i < n; i++) {
            int r = mine[i], yy = y + 46 + i * 28;
            bool on = i == coach_sel;
            if (on) gfx_rect(x + 3, yy - 2, 106, 26, C_DUSK);
            wb_draw_racer(r, G.r[r].trained ? WA_RUN1 + (frame_t / 8) % 2 : WA_IDLE, x + 5, yy + 4, 0);
            text_draw(rname(r), x + 24, yy, on ? C_YELLOW : C_WHITE);
            snprintf(b, sizeof b, "SPEED %d-%d", wb_spd_lo(&G, r), wb_spd_hi(&G, r));
            tiny_draw(b, x + 24, yy + 10, C_LIGHT);
            if (G.r[r].trained) tiny_draw("TRAINED", x + 24, yy + 17, C_LIME);
        }
        return;
    }
    default: break;
    }
    face_box(P->chr, x + 4, y + 4);
    text_draw(pname(p), x + 44, y + 4, C_WHITE);
    snprintf(b, sizeof b, "CASH %s", money(P->cash));
    tiny_draw(b, x + 44, y + 15, C_YELLOW);
    snprintf(b, sizeof b, "DEBT %s", money(P->debt));
    tiny_draw(b, x + 44, y + 22, P->debt ? C_RED : C_GREY);
    snprintf(b, sizeof b, "SPONSOR %d/%d", wb_sponsored_count(&G, p), WB_MAX_SPONSOR);
    tiny_draw(b, x + 44, y + 29, C_GREY);
    for (int i = 0; i < MI_COUNT; i++) {
        int yy = y + 46 + i * 12;
        bool on = i == menu_sel && sub == M_MAIN;
        if (on) gfx_rect(x + 3, yy - 2, 106, 11, C_DUSK);
        text_draw(MENU[i], x + 16, yy, on ? C_YELLOW : sub == M_MAIN ? C_LIGHT : C_GREY);
    }
    if (sub == M_MAIN) ui_cursor(x + 6, y + 46 + menu_sel * 12, frame_t);
    /* a word on the item under the cursor */
    static const char *const HELP[MI_COUNT] = {
        "BACK ONE WOBBLER TO WIN.",
        "", "", "", "", "", "READY FOR THE OFF.",
    };
    const char *h = HELP[menu_sel];
    if (menu_sel == MI_TIPS) { snprintf(b, sizeof b, "SPEED AND FEET, %s A LOOK.", money(wb_tip_fee(&G))); h = b; }
    if (menu_sel == MI_FIXER) h = P->job ? "JOB BOOKED." : "SHADY HELP, FOR A PRICE.";
    if (menu_sel == MI_LENDER) { snprintf(b, sizeof b, "LOANS OF %s.", money(wb_loan_size(&G))); h = b; }
    if (menu_sel == MI_SPONSOR) { snprintf(b, sizeof b, "%s WHEN YOURS WINS.", money(WB_SPONSOR_BONUS)); h = b; }
    if (menu_sel == MI_COACH) h = "TRAIN YOUR SPONSORED ONES.";
    if (sub == M_MAIN) text_wrap(h, x + 6, y + 134, 102, C_GREY, 8);
    if (sub == M_BET_LANE) text_wrap("WHICH WOBBLER?", x + 6, y + 134, 102, C_YELLOW, 8);
    if (sub == M_TIP_LANE) text_wrap("LOOK UP WHICH ONE?", x + 6, y + 134, 102, C_YELLOW, 8);
}

static void draw_sponsor_list(void) {
    int p = cur_p();
    int x = CARD_X, y = 14;
    ui_panel(x, y, CARD_W, 124, C_NIGHT, C_AMBER);
    spr_draw(&wb_spr[WA_STABLE], x + 4, y + 3, 0);
    text_draw("THE STABLES", x + 24, y + 3, C_AMBER);
    char b[64];
    snprintf(b, sizeof b, "YOURS: %d/%d", wb_sponsored_count(&G, p), WB_MAX_SPONSOR);
    tiny_draw(b, x + CARD_W - 6 - tiny_width(b), y + 5, C_GREY);
    tiny_draw("WON/RAN", x + 78, y + 13, C_GREY);
    tiny_draw("FEE", x + 116, y + 13, C_GREY);
    for (int i = 0; i < 9 && list_top + i < book_n; i++) {
        int k = list_top + i, r = book_order[k], yy = y + 22 + i * 11;
        bool on = k == list_sel;
        if (on) gfx_rect(x + 2, yy - 1, CARD_W - 4, 11, C_DUSK);
        const WbRacer *R = &G.r[r];
        int col = !wb_active(&G, r) ? C_SLATE : on ? C_YELLOW : C_LIGHT;
        gfx_rect(x + 5, yy + 1, 5, 5, wb_body_col(r));
        text_draw(rname(r), x + 13, yy, col);
        snprintf(b, sizeof b, "%d/%d", R->wins, R->races);
        tiny_draw(b, x + 84, yy + 2, col);
        if (R->out == OUT_RETIRED) tiny_draw("RETIRED", x + 116, yy + 2, C_SLATE);
        else if (R->out) tiny_draw("GONE", x + 116, yy + 2, C_SLATE);
        else if (R->sponsor) {
            tiny_draw(R->sponsor == p + 1 ? "YOURS" : pname(R->sponsor - 1), x + 116, yy + 2, R->sponsor == p + 1 ? C_LIME : C_PINK);
        } else {
            tiny_draw(money(wb_sponsor_fee(&G, r)), x + 116, yy + 2, C_AMBER);
        }
    }
    if (list_top > 0) tiny_draw(GLYPH_UP, x + CARD_W - 10, y + 22, C_GREY);
    if (list_top + 9 < book_n) tiny_draw(GLYPH_DOWN, x + CARD_W - 10, y + 112, C_GREY);
}

static void draw_bet_amount(void) {
    int x = CARD_X + 20, y = CARD_Y(lane_sel) + 6;
    if (lane_sel == 2) y -= 26;
    ui_panel(x, y + 30, 156, 34, C_INK, C_YELLOW);
    char b[48];
    snprintf(b, sizeof b, "BET %s", money(amt));
    text_draw_scaled(b, x + 6, y + 34, C_YELLOW, 2);
    snprintf(b, sizeof b, GLYPH_LEFT GLYPH_RIGHT " $10  " GLYPH_UP GLYPH_DOWN " $100  MAX %s", money(bet_room()));
    tiny_draw(b, x + 6, y + 54, C_LIGHT);
}

static void draw_paddock(void) {
    draw_backdrop(frame_t);
    draw_topbar();
    if (sub == M_SPONSOR) draw_sponsor_list();
    else
        for (int l = 0; l < WB_FIELD; l++) {
            bool hi = (sub == M_BET_LANE || sub == M_BET_AMT || sub == M_TIP_LANE || sub == M_FIX_LANE) && l == lane_sel;
            draw_card(l, hi);
        }
    if (sub == M_BET_AMT) draw_bet_amount();
    draw_right_panel();
    if (msg_t > 0) draw_msg_line(msg, C_WHITE);
    else {
        const char *h = sub == M_MAIN ? GLYPH_A " CHOOSE   " GLYPH_B " -" : GLYPH_A " CHOOSE   " GLYPH_B " BACK";
        if (sub == M_BET_AMT) h = GLYPH_A " PLACE THE BET   " GLYPH_B " BACK";
        if (sub == M_SPONSOR) h = GLYPH_A " SPONSOR   " GLYPH_B " BACK   FEE: $100 + $50 A WIN";
        if (sub == M_COACH) h = GLYPH_A " TRAIN   " GLYPH_B " BACK";
        draw_msg_line(h, C_GREY);
    }
}

/* ---- the track ---- */
static void draw_stands(int t) {
    /* dusk over the crater, two moons */
    gfx_rect(0, 0, 320, 34, C_NAVY);
    gfx_dither(0, 18, 320, 16, C_PURPLE, 6);
    for (int i = 0; i < 24; i++) gfx_pset((i * 67 + 11) % 320, (i * 29) % 18, (t / 25 + i) % 5 ? C_BLUE : C_WHITE);
    gfx_circ(262, 10, 6, C_CREAM);
    gfx_circ(264, 9, 5, C_NAVY);
    gfx_circ(40, 12, 3, C_PINK);
    /* the crater rim and the stands */
    gfx_rect(0, 34, 320, 38, C_SLATE);
    for (int row = 0; row < 3; row++) {
        gfx_hline(0, 319, 40 + row * 10, C_GREY);
        for (int i = 0; i < 40; i++) {
            int x = (i * 8 + row * 3 + 2) % 320, hop = ((t / 8 + i * 7 + row) % 9) == 0 ? 2 : 0;
            static const uint8_t HEADS[6] = {C_LIME, C_PINK, C_CYAN, C_AMBER, C_VIOLET, C_ORANGE};
            int c = HEADS[(i * 5 + row * 3) % 6];
            gfx_rect(x, 42 + row * 10 - hop, 5, 5, c);
            gfx_pset(x + 1, 43 + row * 10 - hop, C_INK);
            gfx_pset(x + 3, 43 + row * 10 - hop, C_INK);
        }
    }
    gfx_rect(0, 70, 320, 4, C_WHITE);
    for (int x = 0; x < 320; x += 8) gfx_rect(x, 70, 4, 4, C_RED);
}

static void draw_track(void) {
    WbRace *rc = &G.race;
    gfx_rect(0, 74, 320, 96, C_EARTH);
    gfx_dither(0, 74, 320, 96, C_BROWN, 4);
    for (int l = 0; l <= WB_FIELD; l++) gfx_hline(0, 319, LANE_Y(l) - 12, C_TAN);
    /* the start and the checkered finish */
    gfx_vline(START_X, 74, 165, C_WHITE);
    int fx = START_X + WB_TRACK_PX;
    for (int y = 74; y < 166; y += 3)
        for (int k = 0; k < 2; k++) gfx_rect(fx + k * 3, y, 3, 3, ((y / 3 + k) % 2) ? C_WHITE : C_INK);
    for (int l = 0; l < WB_FIELD; l++) {
        char b[4];
        snprintf(b, sizeof b, "%d", l + 1);
        gfx_rect(3, LANE_Y(l) - 3, 11, 10, C_INK);
        text_draw(b, 6, LANE_Y(l) - 2, C_WHITE);
        /* litter */
        WbLane *L = &rc->lane[l];
        for (int i = 0; i < L->nlitter; i++) {
            int kind = WA_PEEL + (L->litter[i] + i) % 3;
            spr_draw(&wb_spr[kind], START_X + L->litter[i] - 3, LANE_Y(l) + 7, 0);
        }
    }
}

static void draw_race(void) {
    WbRace *rc = &G.race;
    int sx = shake_t ? ((shake_t % 4) < 2 ? 1 : -1) : 0;
    gfx_camera(sx, 0);
    draw_stands(frame_t);
    draw_track();
    bool counting = state_t < COUNTDOWN - 30;
    /* the garbage truck makes its pass before the off */
    if (rc->event == EV_SPILL && state_t < COUNTDOWN) {
        int tx = -30 + state_t * 360 / COUNTDOWN;
        spr_draw(&wb_spr[WA_TRUCK], tx, 76, 0);
    }
    for (int l = 0; l < WB_FIELD; l++) {
        WbLane *L = &rc->lane[l];
        int racer = rc->field[l];
        int px = START_X + L->x / 256 - 14, py = LANE_Y(l) - 9;
        int frame;
        if (L->dead) frame = WA_DEAD;
        else if (counting || L->done) frame = WA_IDLE;
        else if (L->trip > 0) frame = WA_TRIP;
        else frame = (rc->t / 5) % 2 ? WA_RUN1 : WA_RUN2;
        if (L->done && !L->dead) frame = (frame_t / 10) % 2 ? WA_RUN1 : WA_IDLE;
        int bob = frame == WA_RUN2 ? -1 : 0;
        if (L->fizz && !L->dead && !counting) bob += (rc->t / 3) % 3 - 1;
        if (L->trip > 0 && !L->dead) py -= (L->trip > WB_TRIP_TICKS / 2) ? (WB_TRIP_TICKS - L->trip) / 6 : L->trip / 6;
        wb_draw_racer(racer, frame, px, py + bob, 0);
        if (L->pep && !counting && !L->dead && !L->done)
            for (int k = 0; k < 3; k++) gfx_hline(px - 6 - k * 2, px - 2, py + 5 + k * 3, (rc->t / 2 + k) % 2 ? C_YELLOW : C_ORANGE);
        if (L->trip > 0 && !L->dead) {
            int a = (rc->t / 4) % 4;
            gfx_pset(px + 4 + a * 2, py - 3, C_YELLOW);
            gfx_pset(px + 11 - a * 2, py - 5, C_WHITE);
        }
        if (L->dead == OUT_METEOR) { gfx_circb(px + 8, py + 12, 7, C_INK); }
        if (L->dead) {
            gfx_circb(px + 8, py + 1, 3, C_YELLOW);
            spr_draw(&wb_spr[WA_STONE], px + 4, py - 12, 0);
        }
        if (L->minded) spr_draw(&wb_spr[WA_MINDER], 14, LANE_Y(l) + 6, 0);
    }
    /* meteors on their way down */
    for (int i = 0; i < rc->nmet; i++) {
        WbMeteor *m = &rc->met[i];
        int dt = (int)m->land - (int)rc->t;
        int mx = START_X + m->x, my = LANE_Y(m->lane) + 4;
        if (!counting && dt > 0 && dt < 50) {
            int yy = my - dt * 2, xx = mx + dt;
            for (int k = 1; k < 5; k++) gfx_pset(xx + k * 2, yy - k * 2, k % 2 ? C_ORANGE : C_YELLOW);
            spr_draw(&wb_spr[WA_METEOR], xx - 4, yy - 4, 0);
        } else if (dt <= 0 && dt > -20 && rc->started && !counting) {
            gfx_circ(mx, my, 8 + dt / 3, dt % 4 < 2 ? C_YELLOW : C_ORANGE);
        } else if (dt <= -20 && rc->started) {
            gfx_rect(mx - 5, my + 1, 11, 4, C_INK);
            gfx_rect(mx - 4, my, 9, 6, C_INK);
            gfx_hline(mx - 3, mx + 3, my + 1, C_BROWN);
        }
    }
    if (rc->event == EV_SMOG) gfx_dither(0, 30, 320, 140, C_GREY, 5 + (frame_t / 30) % 2);
    gfx_camera(0, 0);
    /* who's who */
    gfx_rect(0, 0, 320, 11, C_INK);
    for (int l = 0; l < WB_FIELD; l++) {
        char b[32];
        snprintf(b, sizeof b, "%d %s %d:1", l + 1, rname(rc->field[l]), rc->odds[l]);
        text_draw(b, 4 + l * 106, 2, rc->winner == l && rc->over ? C_YELLOW : C_WHITE);
    }
    if (state_t < COUNTDOWN) {
        int n = 3 - state_t / 40;
        char b[16];
        if (n >= 1) snprintf(b, sizeof b, "%d", n);
        else snprintf(b, sizeof b, "GO!");
        static const uint8_t grad[] = {C_WHITE, C_YELLOW, C_ORANGE};
        ui_fancy_center(b, 160, 100, 3, grad, 3, C_INK, C_WINE);
    }
    if (state_t < COUNTDOWN + 30) {
        static const char *const EVN[4] = {"", "METEOR SHOWER!", "GARBAGE ON THE TRACK!", "SMOG ALERT!"};
        if (rc->event) text_center(EVN[rc->event], 160, 16, (frame_t / 8) % 2 ? C_ORANGE : C_YELLOW);
    }
    gfx_rect(0, 170, 320, 10, C_INK);
    if (call_t > 0) text_center(call_line, 160, 171, C_YELLOW);
    else if (rc->over) text_center(GLYPH_A " THE NEWS", 160, 171, C_GREY);
    if (rc->over && rc->winner != WB_NONE) {
        char b[48];
        snprintf(b, sizeof b, "%s WINS!", rname(rc->field[rc->winner]));
        static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
        ui_fancy_center(b, 160, 36, 2, grad, 4, C_INK, C_WINE);
    }
}

/* ---- the news ---- */
static void news_text(const WbNews *n, char *b, int len) {
    switch (n->kind) {
    case NW_WIN: snprintf(b, (size_t)len, "%s WINS AT %d TO 1!", rname(n->a), (int)n->v); break;
    case NW_NOWIN: snprintf(b, (size_t)len, "NOBODY FINISHED THE RACE. EVERY BET IS LOST."); break;
    case NW_TRIPS: snprintf(b, (size_t)len, "%s FELL OVER %d TIMES.", rname(n->a), n->b); break;
    case NW_METEOR_DEAD: snprintf(b, (size_t)len, "%s WAS STRUCK BY A METEOR. REST IN PEACE.", rname(n->a)); break;
    case NW_COLLAPSED: snprintf(b, (size_t)len, "%s COLLAPSED ON THE TRACK AND NEVER GOT UP. A MYSTERY.", rname(n->a)); break;
    case NW_FINE: snprintf(b, (size_t)len, "%s WAS CAUGHT MEDDLING WITH %s. FINED %s.", pname(n->a), rname(n->b), money(n->v)); break;
    case NW_SPONSOR: snprintf(b, (size_t)len, "SPONSOR %s COLLECTS %s FOR %s'S WIN.", pname(n->a), money(n->v), rname(n->b)); break;
    case NW_RETIRED: snprintf(b, (size_t)len, "%s HAS RETIRED FROM RACING. HAPPY GRAZING!", rname(n->a)); break;
    case NW_EVENT: {
        static const char *const E[4] = {"", "METEORS RAINED ON CRATER DOWNS!", "A GARBAGE TRUCK SHED ITS LOAD ON THE TRACK.", "THICK SMOG HUNG OVER THE RACE."};
        snprintf(b, (size_t)len, "%s", E[n->a & 3]);
        break;
    }
    case NW_PLACES:
        if (n->v == 2) snprintf(b, (size_t)len, "%s CAME SECOND AND %s THIRD.", rname(n->a), rname(n->b));
        else snprintf(b, (size_t)len, "%s CAME SECOND.", rname(n->a));
        break;
    case NW_SECRET: snprintf(b, (size_t)len, "A BET OF $1990! THE WIRE TIPS ITS HAT TO WHOEVER DREW THE VERY FIRST WOBBLER."); break;
    default: b[0] = 0;
    }
}

static void draw_news(void) {
    draw_backdrop(frame_t);
    ui_panel(8, 6, 304, 160, C_INK, C_GREY);
    gfx_rect(12, 10, 296, 18, C_WINE);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_text("THE WOBBLE WIRE", 50, 12, 1, grad, 3, C_INK, -1);
    char b[64];
    snprintf(b, sizeof b, "RACE %d", G.round + 1);
    tiny_draw(b, 270, 16, C_CREAM);
    gfx_rect(16, 34, 40, 40, C_NAVY);
    spr_draw_scaled(&wb_spr[WA_ANCHOR], 20, 38, 2, 0);
    tiny_center("DOT DIAL", 36, 76, C_GREY);
    int first = news_shown > 4 ? ((news_shown - 1) / 4) * 4 : 0;
    int y = 34;
    for (int i = first; i < news_shown && i < G.nnews; i++) {
        char t[128];
        news_text(&G.news[i], t, sizeof t);
        int col = G.news[i].kind == NW_WIN ? C_YELLOW : G.news[i].kind == NW_SECRET ? C_PINK : C_WHITE;
        int lines = text_wrap(t, 64, y, 240, col, 9);
        y += lines * 9 + 5;
    }
    if (state_t > 15) tiny_draw(news_shown < G.nnews ? GLYPH_A " MORE" : GLYPH_A " THE PAY WINDOW", 240, 156, C_GREY);
}

/* ---- the pay window ---- */
static void draw_payout(void) {
    draw_backdrop(frame_t);
    draw_topbar();
    WbRace *rc = &G.race;
    char b[64];
    for (int p = 0; p < WB_PLAYERS; p++) {
        WbPlayer *P = &G.pl[p];
        int y = 16 + p * 50;
        ui_panel(4, y, 312, 46, C_NIGHT, P->human ? C_AMBER : C_SLATE);
        face_box(P->chr, 8, y + 5);
        text_draw(pname(p), 50, y + 4, P->human ? C_YELLOW : C_WHITE);
        if (P->bet_on != WB_NONE) {
            snprintf(b, sizeof b, "%s ON %s", money(P->bet), rname(rc->field[P->bet_on]));
            tiny_draw(b, 50, y + 14, C_LIGHT);
            if (P->won > 0) snprintf(b, sizeof b, "WINS! PAYS %s", money(P->won));
            else snprintf(b, sizeof b, "LOST %s", money(-P->won));
            tiny_draw(b, 160, y + 14, P->won > 0 ? C_LIME : C_RED);
        } else tiny_draw("NO BET", 50, y + 14, C_GREY);
        int yy = y + 22;
        if (P->bonus) { snprintf(b, sizeof b, "SPONSOR +%s", money(P->bonus)); tiny_draw(b, 50, yy, C_LIME); yy += 7; }
        if (P->fine) { snprintf(b, sizeof b, "FINED -%s", money(P->fine)); tiny_draw(b, 50, yy, C_RED); yy += 7; }
        if (P->interest) { snprintf(b, sizeof b, "INTEREST +%s ON THE LOAN", money(P->interest)); tiny_draw(b, 50, yy, C_ORANGE); }
        snprintf(b, sizeof b, "%s", money(P->cash));
        text_draw_scaled(b, 312 - text_width_scaled(b, 2), y + 6, C_YELLOW, 2);
        if (P->debt) { snprintf(b, sizeof b, "OWES %s", money(P->debt)); tiny_draw(b, 312 - tiny_width(b), y + 24, C_RED); }
        snprintf(b, sizeof b, "PLACE %d", wb_rank(&G, p));
        tiny_draw(b, 312 - tiny_width(b), y + 34, C_GREY);
    }
    draw_msg_line(wb_last_round(&G) ? GLYPH_A " THE FINAL COUNT" : GLYPH_A " THE NEXT RACE", C_GREY);
}

/* ---- title, setup and the rest ---- */
static void draw_title(void) {
    draw_stands(frame_t);
    gfx_rect(0, 74, 320, 106, C_EARTH);
    gfx_dither(0, 74, 320, 106, C_BROWN, 4);
    for (int l = 0; l < 3; l++) gfx_hline(0, 319, 100 + l * 26, C_TAN);
    for (int i = 0; i < 5; i++) {
        int x = ((frame_t * (3 + i % 3)) / 2 + i * 71) % 380 - 30;
        int f = (frame_t / 5 + i) % 2 ? WA_RUN1 : WA_RUN2;
        if ((frame_t / 60 + i) % 11 == 0) f = WA_TRIP;
        wb_draw_racer((i * 7 + frame_t / 380) % WB_RACERS, f, x, 106 + (i % 3) * 22, 0);
    }
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center("WOBBLE", 160, 6, 3, grad, 5, C_INK, C_WINE);
    ui_fancy_center("DERBY", 160, 32, 3, grad, 5, C_INK, C_WINE);
    gfx_rect(0, 56, 320, 9, C_INK);
    tiny_center("CRATER DOWNS " GLYPH_DOT " BETTING " GLYPH_DOT " FIXING " GLYPH_DOT " TRAINING", 160, 58, C_CREAM);
    ui_panel(100, 128, 120, sv.has_run ? 34 : 22, C_INK, C_AMBER);
    int y = 134;
    if (sv.has_run) {
        text_center("CONTINUE", 160, y, title_sel == 0 ? C_YELLOW : C_LIGHT);
        y += 12;
    }
    text_center("NEW MEETING", 160, y, (sv.has_run ? title_sel == 1 : 1) ? C_YELLOW : C_LIGHT);
    char b[64];
    snprintf(b, sizeof b, "WINS %d  " GLYPH_DOT "  BEST PURSE %s", sv.wins, money(sv.best));
    gfx_rect(0, 170, 320, 10, C_INK);
    tiny_center(b, 160, 172, C_GREY);
}

static void draw_setup(void) {
    draw_backdrop(frame_t);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center("THE MEETING", 160, 4, 2, grad, 3, C_INK, C_WINE);
    char b[64];
    for (int p = 0; p < WB_PLAYERS; p++) {
        int x = 12 + p * 102, y = 28;
        bool on = setup_row == p;
        ui_panel(x, y, 94, 88, on ? C_DUSK : C_NIGHT, on ? C_YELLOW : C_SLATE);
        snprintf(b, sizeof b, "PUNTER %d", p + 1);
        tiny_draw(b, x + 5, y + 4, C_GREY);
        tiny_draw(setup_human[p] ? "PLAYER" : "CPU", x + 89 - tiny_width(setup_human[p] ? "PLAYER" : "CPU"), y + 4, setup_human[p] ? C_LIME : C_SKY);
        face_box(setup_chr[p], x + 29, y + 12);
        if (on) { text_draw(GLYPH_LEFT, x + 16, y + 26, C_YELLOW); text_draw(GLYPH_RIGHT, x + 70, y + 26, C_YELLOW); }
        text_center(WB_CHAR[setup_chr[p]].name, x + 47, y + 52, C_WHITE);
        text_wrap(WB_CHAR[setup_chr[p]].from, x + 5, y + 63, 86, C_GREY, 8);
    }
    ui_panel(12, 122, 296, 16, setup_row == 3 ? C_DUSK : C_NIGHT, setup_row == 3 ? C_YELLOW : C_SLATE);
    snprintf(b, sizeof b, "RACES:  " GLYPH_LEFT " %d " GLYPH_RIGHT, WB_LENGTH[setup_len]);
    text_center(b, 160, 126, setup_row == 3 ? C_YELLOW : C_LIGHT);
    ui_panel(120, 142, 80, 16, setup_row == 4 ? C_DUSK : C_NIGHT, setup_row == 4 ? C_YELLOW : C_SLATE);
    text_center("TO THE TRACK", 160, 146, setup_row == 4 ? C_YELLOW : C_LIGHT);
    const char *h = setup_row == 0 ? GLYPH_LEFT GLYPH_RIGHT " CHOOSE YOUR FACE" :
                    setup_row < 3 ? GLYPH_LEFT GLYPH_RIGHT " FACE   " GLYPH_A " PLAYER OR CPU" :
                    setup_row == 3 ? GLYPH_LEFT GLYPH_RIGHT " HOW MANY RACES" : GLYPH_A " START   " GLYPH_B " BACK";
    draw_msg_line(h, C_GREY);
}

static void draw_book(void) {
    draw_backdrop(frame_t);
    ui_panel(20, 16, 280, 146, C_NIGHT, C_AMBER);
    npc_box(WA_STABLE, 30, 26);
    text_draw("WELCOME TO CRATER DOWNS", 74, 28, C_YELLOW);
    text_wrap("TWENTY RACES HAVE BEEN RUN THIS SEASON ALREADY, AND EVERY RESULT IS IN THE FORM BOOK. THE ODDS COME FROM THAT RECORD.", 74, 40, 216, C_LIGHT, 9);
    char b[80];
    snprintf(b, sizeof b, "%d RACES. %s EACH TO START. THE BIGGEST PURSE AT THE END WINS.", G.nraces, money(WB_START_CASH));
    text_wrap(b, 30, 88, 260, C_WHITE, 9);
    text_wrap("BETS PAY ONLY ON A WIN. WHAT YOU OWE IS TAKEN FROM YOUR PURSE AT THE END.", 30, 116, 260, C_GREY, 9);
    draw_msg_line(GLYPH_A " THE FIRST RACE", C_GREY);
}

static void draw_round(void) {
    draw_backdrop(frame_t);
    draw_topbar();
    for (int l = 0; l < WB_FIELD; l++) {
        int x = 20 + l * 98, y = 40;
        ui_panel(x, y, 86, 70, C_NIGHT, C_SLATE);
        int r = G.race.field[l];
        wb_draw_racer(r, (frame_t / 15 + l) % 2 ? WA_IDLE : WA_RUN1, x + 35, y + 10, 0);
        text_center(rname(r), x + 43, y + 30, C_WHITE);
        char b[32];
        snprintf(b, sizeof b, "%d:1", G.race.odds[l]);
        text_center(b, x + 43, y + 42, C_YELLOW);
        snprintf(b, sizeof b, "WON %d OF %d", G.r[r].wins, G.r[r].races);
        tiny_center(b, x + 43, y + 54, C_GREY);
    }
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    char b[32];
    snprintf(b, sizeof b, "RACE %d", G.round + 1);
    ui_fancy_center(b, 160, 16, 2, grad, 3, C_INK, C_WINE);
    text_center(wb_last_round(&G) ? "THE LAST RACE OF THE MEETING!" : "THE RUNNERS ARE IN THE PADDOCK.", 160, 116, C_LIGHT);
    /* the purses so far */
    for (int p = 0; p < WB_PLAYERS; p++) {
        int x = 20 + p * 98, y = 130;
        wb_draw_face(G.pl[p].chr, x, y + 2, 1);
        char s[32];
        text_draw(pname(p), x + 20, y + 2, G.pl[p].human ? C_YELLOW : C_WHITE);
        snprintf(s, sizeof s, "%s", money(G.pl[p].cash));
        tiny_draw(s, x + 20, y + 12, C_LIME);
        if (G.pl[p].debt) { snprintf(s, sizeof s, "OWES %s", money(G.pl[p].debt)); tiny_draw(s, x + 20, y + 19, C_RED); }
    }
    draw_msg_line(GLYPH_A " TO THE PADDOCK", C_GREY);
}

static void draw_pass(void) {
    draw_backdrop(frame_t);
    int p = cur_p();
    ui_panel(80, 40, 160, 90, C_NIGHT, C_AMBER);
    face_box(G.pl[p].chr, 142, 50);
    char b[48];
    snprintf(b, sizeof b, "PUNTER %d: %s", p + 1, pname(p));
    text_center(b, 160, 92, C_YELLOW);
    text_center("YOUR TURN. NO PEEKING,", 160, 104, C_LIGHT);
    text_center("EVERYONE ELSE.", 160, 114, C_LIGHT);
    draw_msg_line(GLYPH_A " READY", C_GREY);
}

static void draw_cpu(void) {
    draw_backdrop(frame_t);
    draw_topbar();
    public_cards = true;
    for (int l = 0; l < WB_FIELD; l++) draw_card(l, false);
    public_cards = false;
    int p = cur_p();
    int x = PANEL_X, y = 14;
    ui_panel(x, y, 112, 154, C_NIGHT, C_SLATE);
    face_box(G.pl[p].chr, x + 38, y + 20);
    text_center(pname(p), x + 56, y + 62, C_WHITE);
    static const char *const DOTS[4] = {"", ".", "..", "..."};
    char b[48];
    snprintf(b, sizeof b, "IS AT THE WINDOW%s", DOTS[(state_t / 10) % 4]);
    tiny_center(b, x + 56, y + 76, C_GREY);
    draw_msg_line(NULL, C_GREY);
}

static void draw_parade(void) {
    draw_stands(frame_t);
    draw_track();
    for (int l = 0; l < WB_FIELD; l++) wb_draw_racer(G.race.field[l], (frame_t / 12 + l) % 2 ? WA_IDLE : WA_RUN1, START_X - 14, LANE_Y(l) - 9, 0);
    ui_panel(40, 12, 240, 56, C_INK, C_AMBER);
    text_center("THE TOTE BOARD", 160, 16, C_YELLOW);
    for (int p = 0; p < WB_PLAYERS; p++) {
        char b[64];
        const WbPlayer *P = &G.pl[p];
        if (P->bet_on != WB_NONE) snprintf(b, sizeof b, "%s: %s ON %s", pname(p), money(P->bet), rname(G.race.field[P->bet_on]));
        else snprintf(b, sizeof b, "%s: NO BET", pname(p));
        wb_draw_face(P->chr, 46, 26 + p * 13, 1);
        text_draw(b, 66, 29 + p * 13, C_WHITE);
    }
    gfx_rect(0, 170, 320, 10, C_INK);
    text_center(GLYPH_A " THEY'RE UNDER STARTER'S ORDERS", 160, 171, C_GREY);
}

static void draw_final(void) {
    draw_backdrop(frame_t);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_center("THE FINAL COUNT", 160, 4, 2, grad, 4, C_INK, C_WINE);
    int order[WB_PLAYERS] = {0, 1, 2};
    for (int i = 0; i < WB_PLAYERS; i++)
        for (int j = i + 1; j < WB_PLAYERS; j++)
            if (wb_final(&G, order[j]) > wb_final(&G, order[i])) { int t = order[i]; order[i] = order[j]; order[j] = t; }
    static const int PX[3] = {142, 50, 234}, PH[3] = {64, 52, 42};
    for (int k = 0; k < WB_PLAYERS; k++) {
        int p = order[k], x = PX[k], y = 150 - PH[k];
        gfx_rect(x - 17, y, 70, PH[k], k == 0 ? C_AMBER : k == 1 ? C_LIGHT : C_TAN);
        gfx_rectb(x - 17, y, 70, PH[k], C_INK);
        char b[48];
        snprintf(b, sizeof b, "%d", wb_rank(&G, p));
        text_center(b, x + 18, y + 3, C_INK);
        face_box(G.pl[p].chr, x, y - 38 - (k == 0 ? (frame_t / 15) % 2 : 0));
        text_center(pname(p), x + 18, y + 12, C_INK);
        snprintf(b, sizeof b, "%s", money(wb_final(&G, p)));
        text_center(b, x + 18, y + 21, C_INK);
        if (G.pl[p].debt) { snprintf(b, sizeof b, "LOAN -%s", money(G.pl[p].debt)); tiny_center(b, x + 18, y + 32, C_WINE); }
    }
    int w = order[0];
    char b[64];
    snprintf(b, sizeof b, "%s TAKES THE PURSE!", pname(w));
    text_center(b, 160, 28, G.pl[w].human ? C_YELLOW : C_LIGHT);
    if (G.humans == 1 && G.nraces == 6 && wb_rank(&G, 0) == 1 && wb_final(&G, 0) >= 10000)
        text_center("THE TEN THOUSAND CLUB!", 160, 40, (frame_t / 8) % 2 ? C_PINK : C_YELLOW);
    draw_msg_line(state_t > 60 ? GLYPH_A " BACK TO THE TITLE" : NULL, C_GREY);
}

static void wb_draw(void) {
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_SETUP: draw_setup(); break;
    case S_BOOK: draw_book(); break;
    case S_ROUND: draw_round(); break;
    case S_PASS: draw_pass(); break;
    case S_PADDOCK: draw_paddock(); break;
    case S_CPU: draw_cpu(); break;
    case S_PARADE: draw_parade(); break;
    case S_RACE: draw_race(); break;
    case S_NEWS: draw_news(); break;
    case S_PAYOUT: draw_payout(); break;
    case S_FINAL: draw_final(); break;
    }
}

/* ------------------------------------------------------------------ */
/* the demo player: real button presses only                            */

static int bot_goal_lane, bot_goal_amt, bot_goal_racer;
static int bot_round = -1, bot_turn = -1;
static bool bot_want_sponsor, bot_want_loan;

static int press(int b) { return (frame_t & 1) ? b : 0; }

static int nav_menu(int want) {
    if (menu_sel == want) return press(BTN_A);
    return press(((want - menu_sel + MI_COUNT) % MI_COUNT) <= MI_COUNT / 2 ? BTN_DOWN : BTN_UP);
}
static int nav_lane(int want) {
    if (lane_sel == want) return press(BTN_A);
    return press(BTN_DOWN);
}

static int bot_plan_bet(void) {
    int p = cur_p();
    WbPlayer *P = &G.pl[p];
    int est[WB_FIELD];
    wb_estimate(&G, est, P->tips == 7, 99u + G.round);
    int pick = 0, best = -1;
    for (int l = 0; l < WB_FIELD; l++) {
        int ev = est[l] * (G.race.odds[l] + 1);
        if (ev > best) { best = ev; pick = l; }
    }
    bot_goal_lane = pick;
    int room = bet_room();
    bool last = wb_last_round(&G);
    int a;
    if (last) a = room;
    else if (best >= 1200) a = room;
    else if (best >= 900) a = room / 2;
    else a = room / 5;
    a = a / 10 * 10;
    if (a == 1990) a = 1980;
    return iclamp(a, 10, imax(10, room));
}

static int wb_bot(void) {
    switch (state) {
    case S_TITLE:
        if (state_t < 25) return 0;
        if (sv.has_run && title_sel == 0) return press(BTN_DOWN);
        return press(BTN_A);
    case S_SETUP:
        if (setup_row != 4) return press(BTN_UP);
        return press(BTN_A);
    case S_BOOK: case S_ROUND: case S_PASS: case S_PARADE: case S_PAYOUT: case S_NEWS: case S_FINAL:
        return state_t > 70 ? press(BTN_A) : 0;
    case S_CPU: case S_RACE: return 0;
    case S_PADDOCK: break;
    default: return 0;
    }
    int p = cur_p();
    WbPlayer *P = &G.pl[p];
    if (bot_round != G.round || bot_turn != p) {
        bot_round = G.round;
        bot_turn = p;
        bot_want_sponsor = G.round == 0;
        bot_want_loan = wb_last_round(&G) && P->debt == 0 && wb_rank(&G, p) > 1;
        bot_goal_racer = -1;
        bot_goal_amt = -1;
    }
    if (msg_t > 0 && sub == M_MAIN && sub_t < 3) return 0;
    int fee = wb_tip_fee(&G);
    bool tips_left = P->tips != 7 && P->cash >= fee * 3 + 60 && !wb_last_round(&G);
    switch (sub) {
    case M_MAIN:
        if (bot_want_loan) return nav_menu(MI_LENDER);
        if (P->debt > 0 && !wb_last_round(&G) && P->cash >= P->debt + 200) return nav_menu(MI_LENDER);
        if (tips_left && P->bet_on == WB_NONE) return nav_menu(MI_TIPS);
        if (bot_want_sponsor) return nav_menu(MI_SPONSOR);
        if (P->bet_on == WB_NONE && bet_room() >= 10) return nav_menu(MI_BET);
        return nav_menu(MI_DONE);
    case M_LENDER:
        if (bot_want_loan) {
            if (lend_sel != 0) return press(BTN_UP);
            int b = press(BTN_A);
            if (b) bot_want_loan = false;
            return b;
        }
        if (P->debt > 0) { if (lend_sel != 1) return press(BTN_UP); return press(BTN_A); }
        return press(BTN_B);
    case M_TIP_LANE: {
        int want = -1;
        for (int l = 0; l < WB_FIELD; l++) if (!(P->tips & (1 << l))) { want = l; break; }
        if (want < 0 || P->cash < fee) return press(BTN_B);
        return nav_lane(want);
    }
    case M_TIP_SHOW: return sub_t > 20 ? press(BTN_A) : 0;
    case M_SPONSOR: {
        if (bot_goal_racer < 0) {
            /* the fastest-looking wobbler it can afford: a good record, cheap */
            int best = -1, bs = -1;
            for (int k = 0; k < book_n; k++) {
                int r = book_order[k];
                if (!wb_active(&G, r) || G.r[r].sponsor || wb_sponsor_fee(&G, r) > P->cash - 150) continue;
                int s = (WB_DEF[r].spd_lo + WB_DEF[r].spd_hi) * 10 - WB_DEF[r].stab_hi * 12 - G.r[r].wins * 10;
                if (s > bs) { bs = s; best = k; }
            }
            if (best < 0) { bot_want_sponsor = false; return press(BTN_B); }
            bot_goal_racer = best;
        }
        if (list_sel < bot_goal_racer) return press(BTN_DOWN);
        if (list_sel > bot_goal_racer) return press(BTN_UP);
        if (G.r[book_order[list_sel]].sponsor == p + 1) { bot_want_sponsor = false; return press(BTN_B); }
        return press(BTN_A);
    }
    case M_BET_LANE:
        if (bot_goal_amt < 0) bot_goal_amt = bot_plan_bet();
        return nav_lane(bot_goal_lane);
    case M_BET_AMT:
        if (amt + 100 <= bot_goal_amt) return press(BTN_UP);
        if (amt + 10 <= bot_goal_amt) return press(BTN_RIGHT);
        if (amt > bot_goal_amt) return press(BTN_LEFT);
        return press(BTN_A);
    default: return press(BTN_B);
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void wb_load(void) {
    wb_art_load();
    wb_audio_load();
}

static void wb_start(void) {
    load_save();
    memset(&G, 0, sizeof G);
    state = S_TITLE;
    state_t = 0;
    title_sel = 0;
    sure_poison = false;
    game_set_pausable(false);
    music_play(WB_MUS_TITLE);
}

static void wb_quit(void) {
    /* a race under way is run to its end first, so walking out never undoes
     * a result; the game then waits at the next race (so a second call
     * keeps it there) */
    if (state == S_RACE) settle_race();
    if (state == S_NEWS || state == S_PAYOUT) {
        if (wb_next_round(&G)) { keep_run(); state = S_ROUND; }
        else { finish_meeting(); state = S_TITLE; }
    } else if (state >= S_ROUND && state <= S_PARADE && !G.done) {
        if (state == S_CPU) { G.turn++; state = S_ROUND; }  /* that rival has had its turn */
        keep_run();
    }
    save_now();
}

static void wb_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_NAVY);
    gfx_dither(x, y + 14, w, 12, C_PURPLE, 6);
    gfx_circ(x + w - 18, y + 9, 5, C_CREAM);
    gfx_rect(x, y + 26, w, 8, C_SLATE);
    for (int i = 0; i < 18; i++) gfx_rect(x + i * 8 + 1, y + 27 - ((t / 8 + i) % 5 == 0), 5, 4, (i % 3) ? C_LIME : C_PINK);
    gfx_rect(x, y + 34, w, 30, C_EARTH);
    gfx_dither(x, y + 34, w, 30, C_BROWN, 4);
    for (int l = 0; l < 3; l++) {
        int f = (t / 5 + l) % 2 ? WA_RUN1 : WA_RUN2;
        int racer = l == 0 ? 6 : l == 1 ? 0 : 4;
        int rx = x + ((t * (2 + l)) / 3 + l * 40) % (w + 20) - 18;
        wb_draw_racer(racer, f, rx, y + 33 + l * 7, 0);
    }
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_text("WOBBLE DERBY", x + 4, y + 3, 1, grad, 3, C_INK, -1);
}

static int wb_query(const char *key, int *out) {
    int p = 0;
    if (!strcmp(key, "bot")) { *out = wb_bot(); return 1; }
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "sub")) { *out = sub; return 1; }
    if (!strcmp(key, "round")) { *out = G.round + 1; return 1; }
    if (!strcmp(key, "nraces")) { *out = G.nraces; return 1; }
    if (!strcmp(key, "turn")) { *out = G.turn + 1; return 1; }
    if (!strcmp(key, "humans")) { *out = G.humans; return 1; }
    if (!strcmp(key, "done")) { *out = G.done; return 1; }
    if (!strcmp(key, "cap")) { *out = wb_cap(&G); return 1; }
    if (!strcmp(key, "tipfee")) { *out = wb_tip_fee(&G); return 1; }
    if (!strcmp(key, "loan")) { *out = wb_loan_size(&G); return 1; }
    if (!strcmp(key, "event")) { *out = G.race.event; return 1; }
    if (!strcmp(key, "winner")) { *out = G.race.winner == WB_NONE ? -1 : G.race.winner; return 1; }
    if (!strcmp(key, "over")) { *out = G.race.over; return 1; }
    if (!strcmp(key, "race_t")) { *out = G.race.t; return 1; }
    if (!strcmp(key, "nnews")) { *out = G.nnews; return 1; }
    if (!strcmp(key, "has_run")) { *out = sv.has_run; return 1; }
    if (!strcmp(key, "stat_wins")) { *out = sv.wins; return 1; }
    if (!strcmp(key, "stat_games")) { *out = sv.games; return 1; }
    if (!strcmp(key, "best")) { *out = sv.best; return 1; }
    if (!strcmp(key, "menu")) { *out = menu_sel; return 1; }
    if (!strcmp(key, "lane")) { *out = lane_sel; return 1; }
    if (!strcmp(key, "amt")) { *out = amt; return 1; }
    if (!strcmp(key, "art_ok")) { *out = wb_art_ok(); return 1; }
    if (!strcmp(key, "field_ok")) {
        /* three different wobblers, all still racing */
        const uint8_t *f = G.race.field;
        *out = f[0] != f[1] && f[1] != f[2] && f[0] != f[2] && wb_active(&G, f[0]) && wb_active(&G, f[1]) && wb_active(&G, f[2]);
        return 1;
    }
    if (!strcmp(key, "retired")) { int n = 0; for (int i = 0; i < WB_RACERS; i++) n += G.r[i].out == OUT_RETIRED; *out = n; return 1; }
    if (!strcmp(key, "aged")) { int n = 0; for (int i = 0; i < WB_RACERS; i++) n += G.r[i].mod < 0; *out = n; return 1; }
    if (!strcmp(key, "mod_sum")) { int n = 0; for (int i = 0; i < WB_RACERS; i++) n += G.r[i].mod; *out = n; return 1; }
    if (!strcmp(key, "active")) { int n = 0; for (int i = 0; i < WB_RACERS; i++) n += wb_active(&G, i); *out = n; return 1; }
    if (!strcmp(key, "book_races")) { int n = 0; for (int i = 0; i < WB_RACERS; i++) n += G.r[i].races; *out = n; return 1; }
    if (!strcmp(key, "book_wins")) { int n = 0; for (int i = 0; i < WB_RACERS; i++) n += G.r[i].wins; *out = n; return 1; }
    if (!strcmp(key, "setup_row")) { *out = setup_row; return 1; }
    if (!strcmp(key, "secret")) {
        int s = 0;
        for (int i = 0; i < G.nnews; i++) s |= G.news[i].kind == NW_SECRET;
        *out = s;
        return 1;
    }
    if (!strncmp(key, "news", 4)) {
        /* newsK: how many items of kind K */
        int k = atoi(key + 4), n = 0;
        for (int i = 0; i < G.nnews; i++) n += G.news[i].kind == k;
        *out = n;
        return 1;
    }
    /* per punter: cashP, debtP, betP, betonP, jobP, tipsP, finalP, rankP, fineP, bonusP, wonP */
    static const char *const PK[] = {"cash", "debt", "beton", "bet", "job", "tips", "final", "rank", "fine", "bonus", "won", "interest", "trains", "sponsored"};
    for (int k = 0; k < ARRAY_LEN(PK); k++) {
        size_t n = strlen(PK[k]);
        if (!strncmp(key, PK[k], n) && key[n] >= '0' && key[n] <= '9') {
            p = atoi(key + n) % WB_PLAYERS;
            const WbPlayer *P = &G.pl[p];
            switch (k) {
            case 0: *out = P->cash; break;
            case 1: *out = P->debt; break;
            case 2: *out = P->bet_on == WB_NONE ? -1 : P->bet_on; break;
            case 3: *out = P->bet; break;
            case 4: *out = P->job; break;
            case 5: *out = P->tips; break;
            case 6: *out = wb_final(&G, p); break;
            case 7: *out = wb_rank(&G, p); break;
            case 8: *out = P->fine; break;
            case 9: *out = P->bonus; break;
            case 10: *out = P->won; break;
            case 11: *out = P->interest; break;
            case 12: *out = P->trains; break;
            case 13: *out = wb_sponsored_count(&G, p); break;
            }
            return 1;
        }
    }
    /* per lane: fieldL, oddsL, formL, deadL, tripsL, litterL, doneL, xL */
    static const char *const LK[] = {"field", "odds", "form", "dead", "trips", "litter", "ldone", "px", "minded", "fizz", "pep", "poison"};
    for (int k = 0; k < ARRAY_LEN(LK); k++) {
        size_t n = strlen(LK[k]);
        if (!strncmp(key, LK[k], n) && key[n] >= '0' && key[n] <= '9') {
            int l = atoi(key + n) % WB_FIELD;
            const WbLane *L = &G.race.lane[l];
            switch (k) {
            case 0: *out = G.race.field[l]; break;
            case 1: *out = G.race.odds[l]; break;
            case 2: *out = L->form; break;
            case 3: *out = L->dead; break;
            case 4: *out = L->trips; break;
            case 5: *out = L->nlitter; break;
            case 6: *out = L->done; break;
            case 7: *out = L->x / 256; break;
            case 8: *out = L->minded; break;
            case 9: *out = L->fizz; break;
            case 10: *out = L->pep; break;
            case 11: *out = L->poison_at != 0; break;
            }
            return 1;
        }
    }
    /* per wobbler: outR, winsR, racesR, modR, sponsorR, lo R, hiR, feeR */
    static const char *const RK[] = {"out", "wins", "races", "mod", "sponsor", "spdlo", "spdhi", "fee", "trained"};
    for (int k = 0; k < ARRAY_LEN(RK); k++) {
        size_t n = strlen(RK[k]);
        if (!strncmp(key, RK[k], n) && key[n] >= '0' && key[n] <= '9') {
            int r = atoi(key + n) % WB_RACERS;
            switch (k) {
            case 0: *out = G.r[r].out; break;
            case 1: *out = G.r[r].wins; break;
            case 2: *out = G.r[r].races; break;
            case 3: *out = G.r[r].mod; break;
            case 4: *out = G.r[r].sponsor; break;
            case 5: *out = wb_spd_lo(&G, r); break;
            case 6: *out = wb_spd_hi(&G, r); break;
            case 7: *out = wb_sponsor_fee(&G, r); break;
            case 8: *out = G.r[r].trained; break;
            }
            return 1;
        }
    }
    if (!strcmp(key, "bands_ok")) {
        /* the book keeps the original's spread: five wobblers in each
         * clumsiness band and each speed band, and one wild card */
        int sb[6] = {0}, pb[6] = {0};
        for (int i = 0; i < WB_RACERS; i++) {
            const WbDef *d = &WB_DEF[i];
            sb[d->stab_hi - d->stab_lo > 2 ? 5 : (d->stab_lo - 1) / 2]++;
            pb[d->spd_hi - d->spd_lo > 10 ? 5 : (d->spd_lo - 35) / 5]++;
        }
        int ok = 1;
        for (int b = 0; b < 5; b++) ok &= sb[b] == 5 && pb[b] == 5;
        *out = ok && sb[5] == 1 && pb[5] == 1;
        return 1;
    }
    return 0;
}

static int wb_cheat(const char *cmd) {
    int a, b, c;
    if (sscanf(cmd, "new %d %d", &a, &b) == 2) {
        for (int p = 0; p < WB_PLAYERS; p++) setup_human[p] = p < b;
        rng_seed(&g_rng, 4747);
        start_game(a, b);
        round_card();
        return 1;
    }
    if (sscanf(cmd, "field %d %d %d", &a, &b, &c) == 3) {
        G.race.field[0] = (uint8_t)a; G.race.field[1] = (uint8_t)b; G.race.field[2] = (uint8_t)c;
        for (int l = 0; l < WB_FIELD; l++) G.race.odds[l] = (uint8_t)wb_odds_for(&G, G.race.field, l);
        return 1;
    }
    if (sscanf(cmd, "odds %d %d", &a, &b) == 2) { G.race.odds[a % WB_FIELD] = (uint8_t)b; return 1; }
    if (sscanf(cmd, "event %d", &a) == 1) { G.race.event = (uint8_t)a; return 1; }
    if (sscanf(cmd, "cash %d %d", &a, &b) == 2) { G.pl[a % WB_PLAYERS].cash = b; return 1; }
    if (sscanf(cmd, "debt %d %d", &a, &b) == 2) { G.pl[a % WB_PLAYERS].debt = b; return 1; }
    if (sscanf(cmd, "round %d", &a) == 1) { G.round = (uint8_t)(a - 1); return 1; }
    if (sscanf(cmd, "mod %d %d", &a, &b) == 2) { G.r[a % WB_RACERS].mod = (int8_t)b; return 1; }
    if (sscanf(cmd, "record %d %d %d", &a, &b, &c) == 3) { G.r[a % WB_RACERS].wins = (uint8_t)b; G.r[a % WB_RACERS].races = (uint8_t)c; return 1; }
    if (sscanf(cmd, "sponsor %d %d", &a, &b) == 2) { G.r[b % WB_RACERS].sponsor = (uint8_t)(a + 1); return 1; }
    if (sscanf(cmd, "out %d %d", &a, &b) == 2) { G.r[a % WB_RACERS].out = (uint8_t)b; return 1; }
    if (sscanf(cmd, "meteor %d %d %d", &a, &b, &c) == 3) {
        /* a meteor on lane a, at pixel b, landing at tick c */
        WbRace *rc = &G.race;
        if (rc->nmet < WB_MAX_METEORS) rc->met[rc->nmet++] = (WbMeteor){(uint8_t)a, (int16_t)b, (uint16_t)c};
        return 1;
    }
    if (!strcmp(cmd, "sure_poison")) { sure_poison = true; return 1; }
    if (!strcmp(cmd, "reodds")) {
        for (int l = 0; l < WB_FIELD; l++) G.race.odds[l] = (uint8_t)wb_odds_for(&G, G.race.field, l);
        return 1;
    }
    if (!strcmp(cmd, "autorace")) {
        /* a whole round with every seat played by the CPU, no screens */
        for (int q = 0; q < WB_PLAYERS; q++) wb_cpu_turn(&G, q);
        wb_race_settle(&G);
        wb_next_round(&G);
        return 1;
    }
    if (!strcmp(cmd, "cpu_turn")) { wb_cpu_turn(&G, cur_p()); return 1; }
    if (!strcmp(cmd, "settle")) { settle_race(); return 1; }
    if (!strcmp(cmd, "run")) { if (state != S_RACE) start_race(); wb_race_run(&G); return 1; }
    return 0;
}

const GameDef GAME_WOBBLE = {
    "wobble",
    "WOBBLE DERBY",
    "1989",
    "SIMULATION",
    "BET ON THE WOBBLERS AT CRATER DOWNS. TIPS, LOANS, SPONSORS AND DIRTY TRICKS: THE BIGGEST PURSE WINS.",
    {"SPONSOR A WOBBLER THAT WINS", "WIN A 1P MEETING OF 6+ RACES", "WIN 6 RACES WITH $10,000"},
    "D-PAD\tMOVE / SET THE BET\n"
    GLYPH_A "\tCHOOSE / CONFIRM\n"
    GLYPH_B "\tBACK\n"
    "START\tPAUSE",
    C_PURPLE, C_YELLOW,
    wb_load, wb_start, wb_update, wb_draw, wb_quit, wb_label, wb_query, wb_cheat,
    "QUIBBLE RACE", 47,
};
