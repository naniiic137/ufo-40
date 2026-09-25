/* OPEN HOUSE - summer parties in a whitewashed house by the sea.
 * Cartridge 25 of UFO 40, a tribute to Party House (UFO 50 #25).
 * See docs/games/25-open-house.md. The rules live in openhouse_logic.c;
 * this file is the menus, the party and shop screens, saving and the flow. */
#include "openhouse.h"

enum {
    S_TITLE, S_MODE, S_SCEN, S_INTRO, S_TURN, S_PARTY, S_TARGET, S_FETCH, S_PEEK, S_CONFIRM,
    S_BUST, S_BAN, S_RESULT, S_SHOP, S_WIN, S_LOSE, S_LIST
};

typedef struct Save {
    uint32_t magic;
    uint8_t won;          /* a bit per set scenario, bit 5 for any Random win */
    uint8_t streak, best_streak, has_run, run_phase;
    uint8_t pad[3];
    uint16_t random_wins, parties;
    PhGame run;
} Save;
#define SAVE_MAGIC 0x50480001u

static Save sv;
static PhGame G;
static int state, state_t, frame_t, back_state;
static int cur = 100, sel, tcur, fcur, menu_sel, scen_sel, shop_cur, mode_players = 1;
static uint8_t fetch_list[G_COUNT];
static int nfetch;
static int arrive[PH_MAX_CARDS];   /* frame each guest walked in, for the walk from the door */
static int info_card = -1;         /* what the info panel describes */
static int msg_t;
static const char *msg;

#define SLOT_W 26
#define SLOT_H 28
#define GRID_X 8
#define GRID_Y 22
#define PANEL_X 220
#define CUR_DOOR 100
#define CUR_END 101
#define CUR_BOOK 102

/* ------------------------------------------------------------------ */
/* save & goals                                                         */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    if (G.players == 1 && sv.has_run) sv.run = G;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    static Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else { memset(&sv, 0, sizeof sv); sv.magic = SAVE_MAGIC; }
}

static bool unlocked(int scen) {
    if (scen == 0) return true;
    return (sv.won >> (scen - 1)) & 1;
}

static void check_goals(void) {
    if (sv.won) game_award(GOAL_BEACON);
    if ((sv.won & 31) == 31) game_award(GOAL_SAUCER);
    if (sv.best_streak >= 5) game_award(GOAL_ALIEN);
}

/* ------------------------------------------------------------------ */
/* flow                                                                 */

static void say(const char *m) { msg = m; msg_t = 90; }

static void begin_party(void) {
    state = G.players == 2 ? S_TURN : S_PARTY;
    state_t = 0;
    cur = CUR_DOOR;
    info_card = -1;
    memset(arrive, 0, sizeof arrive);
    game_set_pausable(true);
    if (state == S_PARTY) music_play(PH_MUS_PARTY);
    else music_restart(PH_MUS_NIGHT);
    if (G.players == 1) { sv.run_phase = 0; save_now(); }
}

static void new_run(int scen, int players) {
    uint64_t seed = (uint64_t)rng_next(&g_rng) << 32 | rng_next(&g_rng);
    if (players == 1) {
        /* walking out on a Random run in progress ends the streak */
        if (sv.has_run && sv.run.scen == PH_RANDOM && !sv.run.done) sv.streak = 0;
        sv.has_run = 1;
    }
    ph_new(&G, scen, players, seed);
    state = S_INTRO;
    state_t = 0;
    music_play(PH_MUS_TITLE);
    if (players == 1) save_now();
}

static void on_scenario_over(void) {
    if (G.players == 1) {
        bool won = G.winner == 1;
        if (won) {
            if (G.scen < PH_SCENARIOS) sv.won |= (uint8_t)(1 << G.scen);
            else {
                sv.won |= 32;
                if (sv.random_wins < 60000) sv.random_wins++;
                if (sv.streak < 255) sv.streak++;
                if (sv.streak > sv.best_streak) sv.best_streak = sv.streak;
            }
        } else if (G.scen == PH_RANDOM) {
            sv.streak = 0;
        }
        sv.has_run = 0;
        check_goals();
        save_now();
    }
    state = G.winner ? S_WIN : S_LOSE;
    state_t = 0;
    music_restart(G.winner ? PH_MUS_WIN : PH_MUS_LOSE);
}

/* the party is over and paid (or shut down and a guest banned) */
static void after_party(void) {
    if (sv.parties < 60000) sv.parties++;
    if (G.done) { on_scenario_over(); return; } /* four stars: won */
    PhPlayer *p = ph_me(&G);
    if (p->night >= PH_NIGHTS) {
        ph_next_turn(&G);
        if (G.done) { on_scenario_over(); return; }
        begin_party();
        return;
    }
    state = S_SHOP;
    state_t = 0;
    shop_cur = 0;
    music_play(PH_MUS_SHOP);
    if (G.players == 1) { sv.run_phase = 1; save_now(); }
}

static void leave_shop(void) {
    ph_next_turn(&G);
    if (G.done) { on_scenario_over(); return; }
    begin_party();
}

/* ------------------------------------------------------------------ */
/* the party screen: input                                              */

static int slot_x(int i) { return GRID_X + (i % PH_ROW) * SLOT_W; }
static int slot_y(int i) { return GRID_Y + (i / PH_ROW) * SLOT_H; }

static void note_arrivals(void) {
    for (int i = 0; i < G.party.nlast; i++) arrive[G.party.last[i]] = frame_t + i * 6;
    bool star = false, wild = false;
    for (int i = 0; i < G.party.nlast; i++) {
        int c = G.party.last[i];
        star |= (PH_GUESTS[ph_me(&G)->card[c].type].traits & T_STAR) != 0;
        wild |= ph_is_wild(&G, c) != 0;
    }
    sfx_play_name(star ? "ph_star" : wild ? "ph_trouble" : "ph_enter");
    G.party.nlast = 0;
}

static void after_change(int was_trouble) {
    PhParty *pa = &G.party;
    if (pa->over == PO_POLICE || pa->over == PO_FIRE) {
        state = S_BUST;
        state_t = 0;
        music_restart(PH_MUS_BUST);
        return;
    }
    if (ph_trouble(&G) >= 2 && was_trouble < 2) sfx_play_name("ph_warn");
    if (cur < CUR_DOOR && cur >= pa->n) cur = pa->n ? pa->n - 1 : CUR_DOOR;
    if (ph_should_end(&G)) {
        ph_end_party(&G);
        state = S_RESULT;
        state_t = 0;
        sfx_play_name("ph_cash");
    }
}

static void open_door(void) {
    int t = ph_trouble(&G);
    PhPlayer *p = ph_me(&G);
    if (G.party.n >= p->cap) { sfx_play_name("ph_no"); say("THE HOUSE IS FULL"); return; }
    sfx_play_name("ph_knock");
    if (!ph_open_door(&G)) { sfx_play_name("ph_no"); say("NOBODY LEFT TO INVITE"); return; }
    note_arrivals();
    after_change(t);
}

static void use_action(int slot) {
    PhPlayer *p = ph_me(&G);
    int card = G.party.house[slot];
    int act = PH_GUESTS[p->card[card].type].action;
    if (!ph_can_act(&G, slot)) { sfx_play_name("ph_no"); return; }
    sel = slot;
    int t = ph_trouble(&G);
    switch (act) {
    case A_BOOT: case A_PHOTO: case A_STYLE: case A_MAGIC: case A_CUPID:
        state = S_TARGET;
        state_t = 0;
        for (tcur = 0; tcur < G.party.n && !ph_target_ok(&G, sel, tcur); tcur++) {}
        sfx_play_name("ui_ok");
        return;
    case A_FETCH:
        nfetch = 0;
        for (int ty = 0; ty < G_COUNT; ty++)
            if (ph_fetch_ok(&G, ty)) fetch_list[nfetch++] = (uint8_t)ty;
        fcur = 0;
        state = S_FETCH;
        state_t = 0;
        sfx_play_name("ui_ok");
        return;
    case A_PEEK:
        if (ph_act(&G, slot, 0)) { state = S_PEEK; state_t = 0; menu_sel = G.party.n < p->cap ? 0 : 1; sfx_play_name("ph_knock"); }
        return;
    default:
        if (ph_act(&G, slot, 0)) {
            sfx_play_name(act == A_RESHUFFLE ? "ph_shuffle" : "ph_act");
            if (act == A_GREET) note_arrivals();
            if (act == A_RESHUFFLE) cur = CUR_DOOR;
            after_change(t);
        }
        return;
    }
}

static void move_grid_cursor(int *c, int n, bool allow_panel) {
    if (*c >= CUR_DOOR) {
        int lim = allow_panel ? CUR_BOOK : CUR_END;
        if (btn_repeat(BTN_UP)) { *c = *c == CUR_DOOR ? lim : *c - 1; sfx_play_name("ph_move"); }
        if (btn_repeat(BTN_DOWN)) { *c = *c == lim ? CUR_DOOR : *c + 1; sfx_play_name("ph_move"); }
        if (btn_repeat(BTN_LEFT) && n > 0) { *c = imin(n - 1, PH_ROW - 1); sfx_play_name("ph_move"); }
        return;
    }
    int c0 = *c;
    if (btn_repeat(BTN_LEFT) && *c % PH_ROW > 0) (*c)--;
    if (btn_repeat(BTN_RIGHT)) {
        if (*c % PH_ROW < PH_ROW - 1 && *c + 1 < n) (*c)++;
        else if (allow_panel) *c = CUR_DOOR;
    }
    if (btn_repeat(BTN_UP) && *c < CUR_DOOR && *c >= PH_ROW) *c -= PH_ROW;
    if (btn_repeat(BTN_DOWN) && *c < CUR_DOOR && *c + PH_ROW < n) *c += PH_ROW;
    if (*c != c0) sfx_play_name("ph_move");
}

static void update_party(void) {
    PhParty *pa = &G.party;
    move_grid_cursor(&cur, pa->n, true);
    info_card = cur < CUR_DOOR && cur < pa->n ? pa->house[cur] : (cur == CUR_DOOR && pa->peek >= 0 ? pa->peek : -1);
    if (!btnp(BTN_A)) return;
    if (cur == CUR_DOOR) open_door();
    else if (cur == CUR_END) { state = S_CONFIRM; menu_sel = 1; sfx_play_name("ui_ok"); }
    else if (cur == CUR_BOOK) { back_state = S_PARTY; state = S_LIST; state_t = 0; fcur = 0; sfx_play_name("ui_ok"); }
    else use_action(cur);
}

static void update_target(void) {
    int n = G.party.n;
    int c0 = tcur;
    move_grid_cursor(&tcur, n, false);
    if (tcur != c0) info_card = G.party.house[tcur];
    if (btnp(BTN_B)) { state = S_PARTY; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A)) {
        int t = ph_trouble(&G);
        int act = PH_GUESTS[ph_me(&G)->card[G.party.house[sel]].type].action;
        if (!ph_target_ok(&G, sel, tcur)) { sfx_play_name("ph_no"); return; }
        if (ph_act(&G, sel, tcur)) {
            state = S_PARTY;
            sfx_play_name(act == A_BOOT || act == A_CUPID ? "ph_boot" : act == A_PHOTO ? "ph_cash" : "ph_act");
            if (act == A_MAGIC) note_arrivals();
            after_change(t);
        }
    }
}

static void update_fetch(void) {
    if (btn_repeat(BTN_UP) && fcur > 0) { fcur--; sfx_play_name("ph_move"); }
    if (btn_repeat(BTN_DOWN) && fcur < nfetch - 1) { fcur++; sfx_play_name("ph_move"); }
    if (btnp(BTN_B)) { state = S_PARTY; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A) && nfetch > 0) {
        int t = ph_trouble(&G);
        if (ph_act(&G, sel, fetch_list[fcur])) {
            state = S_PARTY;
            note_arrivals();
            after_change(t);
        }
    }
}

static void update_peek(void) {
    if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { menu_sel ^= 1; sfx_play_name("ph_move"); }
    if (btnp(BTN_B)) { state = S_PARTY; cur = CUR_DOOR; sfx_play_name("ui_back"); return; } /* they wait at the door */
    if (btnp(BTN_A)) {
        int t = ph_trouble(&G);
        if (menu_sel == 0) {
            if (G.party.n >= ph_me(&G)->cap) { sfx_play_name("ph_no"); return; }
            state = S_PARTY;
            if (ph_peek_decide(&G, true)) { note_arrivals(); after_change(t); }
        } else {
            ph_peek_decide(&G, false);
            sfx_play_name("ph_boot");
            state = S_PARTY;
        }
    }
}

static void update_confirm(void) {
    if (btn_repeat(BTN_LEFT) || btn_repeat(BTN_RIGHT) || btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { menu_sel ^= 1; sfx_play_name("ph_move"); }
    if (btnp(BTN_B)) { state = S_PARTY; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A)) {
        if (menu_sel == 0) {
            ph_end_party(&G);
            state = S_RESULT;
            state_t = 0;
            sfx_play_name("ph_cash");
        } else {
            state = S_PARTY;
            sfx_play_name("ui_back");
        }
    }
}

static void update_ban(void) {
    int n = G.party.n;
    move_grid_cursor(&tcur, n, false);
    if (n == 0) { after_party(); return; }
    if (btnp(BTN_A)) {
        ph_ban(&G, G.party.house[tcur]);
        sfx_play_name("ph_boot");
        after_party();
    }
}

/* ------------------------------------------------------------------ */
/* the shop                                                             */

#define SHOP_COLS 5 /* the guests, then SPACE, then NEXT PARTY */

static void update_shop(void) {
    int c0 = shop_cur;
    if (shop_cur < G.npool) {
        if (btn_repeat(BTN_LEFT) && shop_cur % SHOP_COLS > 0) shop_cur--;
        if (btn_repeat(BTN_RIGHT) && shop_cur % SHOP_COLS < SHOP_COLS - 1 && shop_cur + 1 < G.npool) shop_cur++;
        if (btn_repeat(BTN_UP) && shop_cur >= SHOP_COLS) shop_cur -= SHOP_COLS;
        if (btn_repeat(BTN_DOWN)) { if (shop_cur + SHOP_COLS < G.npool) shop_cur += SHOP_COLS; else shop_cur = G.npool; }
    } else {
        /* left to right: SPACE, GUEST BOOK, NEXT PARTY */
        const int order[3] = {G.npool, G.npool + 2, G.npool + 1};
        int k = 0;
        while (k < 2 && order[k] != shop_cur) k++;
        if (btn_repeat(BTN_LEFT) && k > 0) shop_cur = order[k - 1];
        if (btn_repeat(BTN_RIGHT) && k < 2) shop_cur = order[k + 1];
        if (btn_repeat(BTN_UP)) shop_cur = imax(0, G.npool - 1 - (G.npool - 1) % SHOP_COLS);
    }
    if (shop_cur != c0) sfx_play_name("ph_move");
    if (!btnp(BTN_A)) return;
    if (shop_cur == G.npool + 2) { back_state = S_SHOP; state = S_LIST; state_t = 0; fcur = 0; sfx_play_name("ui_ok"); return; }
    if (shop_cur < G.npool) {
        int ty = G.pool[shop_cur];
        if (ph_buy(&G, ty)) sfx_play_name(PH_GUESTS[ty].traits & T_STAR ? "ph_star" : "ph_buy");
        else { sfx_play_name("ph_no"); say(!(PH_GUESTS[ty].traits & T_STAR) && G.bought[ty] >= PH_STOCK ? "SOLD OUT" : "NOT ENOUGH FAME"); }
    } else if (shop_cur == G.npool) {
        if (ph_expand(&G)) sfx_play_name("ph_build");
        else { sfx_play_name("ph_no"); say(ph_me(&G)->cap >= PH_MAX_HOUSE ? "THE HOUSE CAN'T GROW" : "NOT ENOUGH CASH"); }
    } else {
        sfx_play_name("ui_ok");
        leave_shop();
    }
}

/* ------------------------------------------------------------------ */
/* menus                                                                */

static void update_title(void) {
    game_set_pausable(false);
    if (btnp(BTN_B) && state_t > 10) { game_exit_to_library(); return; }
    if (state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) {
        sfx_play_name("ui_ok");
        input_consume();
        state = S_MODE;
        state_t = 0;
        menu_sel = sv.has_run ? 0 : 1;
    }
}

static void update_mode(void) {
    int n = 3;
    if (btn_repeat(BTN_UP)) { menu_sel = (menu_sel + n - 1) % n; if (menu_sel == 0 && !sv.has_run) menu_sel = n - 1; sfx_play_name("ph_move"); }
    if (btn_repeat(BTN_DOWN)) { menu_sel = (menu_sel + 1) % n; if (menu_sel == 0 && !sv.has_run) menu_sel = 1; sfx_play_name("ph_move"); }
    if (btnp(BTN_B)) { state = S_TITLE; state_t = 0; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A)) {
        sfx_play_name("ui_ok");
        if (menu_sel == 0 && sv.has_run) {
            G = sv.run;
            if (sv.run_phase == 1) { state = S_SHOP; state_t = 0; shop_cur = 0; music_play(PH_MUS_SHOP); }
            else { state = S_PARTY; state_t = 0; cur = CUR_DOOR; music_play(PH_MUS_PARTY); game_set_pausable(true); }
            return;
        }
        mode_players = menu_sel == 2 ? 2 : 1;
        state = S_SCEN;
        state_t = 0;
        scen_sel = 0;
        for (int i = 0; i <= PH_RANDOM; i++)
            if (unlocked(i) && !((sv.won >> i) & 1)) { scen_sel = i; break; }
    }
}

static void update_scen(void) {
    if (btn_repeat(BTN_UP)) { scen_sel = (scen_sel + PH_RANDOM) % (PH_RANDOM + 1); sfx_play_name("ph_move"); }
    if (btn_repeat(BTN_DOWN)) { scen_sel = (scen_sel + 1) % (PH_RANDOM + 1); sfx_play_name("ph_move"); }
    if (btnp(BTN_B)) { state = S_MODE; state_t = 0; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A)) {
        if (!unlocked(scen_sel)) { sfx_play_name("ph_no"); say(scen_sel == PH_RANDOM ? "WIN SCENARIO 5 FIRST" : "WIN THE ONE BEFORE"); return; }
        sfx_play_name("ui_ok");
        new_run(scen_sel, mode_players);
    }
}

/* ------------------------------------------------------------------ */
/* update                                                               */

static void oh_update(void) {
    frame_t++;
    state_t++;
    if (msg_t > 0) msg_t--;
    switch (state) {
    case S_TITLE: update_title(); break;
    case S_MODE: update_mode(); break;
    case S_SCEN: update_scen(); break;
    case S_INTRO:
        if (state_t > 20 && btnp(BTN_A)) { sfx_play_name("ui_ok"); begin_party(); }
        if (btnp(BTN_B)) { state = S_SCEN; state_t = 0; }
        break;
    case S_TURN:
        if (state_t > 30 && btnp(BTN_A)) { state = S_PARTY; state_t = 0; music_play(PH_MUS_PARTY); }
        break;
    case S_PARTY: update_party(); break;
    case S_TARGET: update_target(); break;
    case S_FETCH: update_fetch(); break;
    case S_PEEK: update_peek(); break;
    case S_CONFIRM: update_confirm(); break;
    case S_BUST:
        if (state_t > 100 && btnp(BTN_A)) { state = S_BAN; state_t = 0; tcur = 0; music_play(PH_MUS_PARTY); }
        break;
    case S_BAN: update_ban(); break;
    case S_RESULT:
        if (state_t > 40 && btnp(BTN_A)) { sfx_play_name("ui_ok"); after_party(); }
        break;
    case S_SHOP: update_shop(); break;
    case S_WIN: case S_LOSE:
        if (state_t > 90 && btnp(BTN_A)) { state = S_SCEN; state_t = 0; music_play(PH_MUS_TITLE); game_set_pausable(true); }
        break;
    case S_LIST:
        if (btn_repeat(BTN_UP) && fcur > 0) fcur--;
        if (btn_repeat(BTN_DOWN)) fcur++;
        if (btnp(BTN_B) || btnp(BTN_A)) { state = back_state; sfx_play_name("ui_back"); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing helpers                                                      */

static void draw_num(int v, int x, int y, int col, const char *pre) {
    char b[16];
    snprintf(b, sizeof b, "%s%d", pre, v);
    tiny_draw(b, x, y, col);
}

/* the tiny font has no line breaks of its own */
static void tiny_lines(const char *s, int x, int y, int col) {
    char line[64];
    while (*s) {
        int n = 0;
        while (s[n] && s[n] != '\n' && n < 63) n++;
        memcpy(line, s, (size_t)n);
        line[n] = 0;
        tiny_draw(line, x, y, col);
        y += 6;
        s += n;
        if (*s == '\n') s++;
    }
}

static void icon_pop(int x, int y) {
    gfx_rect(x + 1, y, 5, 7, C_YELLOW);
    gfx_rect(x, y + 1, 7, 5, C_YELLOW);
    gfx_pset(x + 3, y + 3, C_AMBER);
    gfx_pset(x + 2, y + 2, C_CREAM);
}

static void icon_cash(int x, int y) {
    gfx_rect(x, y + 1, 7, 5, C_JADE);
    gfx_rectb(x, y + 1, 7, 5, C_FOREST);
    gfx_pset(x + 3, y + 3, C_LIME);
}

static void icon_star(int x, int y, int col) { text_draw(GLYPH_STAR, x, y, col); }

static void icon_trouble(int x, int y, int col) {
    gfx_line(x, y, x + 4, y + 4, col);
    gfx_line(x + 4, y, x, y + 4, col);
    gfx_line(x + 1, y, x + 5, y + 4, col);
    gfx_line(x + 5, y, x + 1, y + 4, col);
}

static const char *const SECRET_NAMES[8] = {"UNCLE BERT", "AUNT MAVIS", "OLD GUS", "MISS PENNY",
                                            "COUSIN TOM", "COUSIN MAE", "COUSIN NED", "COUSIN IVY"};

static const char *card_name(int card) {
    const PhCard *c = &ph_me(&G)->card[card];
    if (c->name) return SECRET_NAMES[(c->name - 1) % 8];
    return PH_GUESTS[c->type].name;
}

/* a guest in a house slot */
static void draw_slot(int i, bool hi, bool target_ok, bool dim) {
    PhParty *pa = &G.party;
    int x = slot_x(i), y = slot_y(i);
    int card = pa->house[i];
    PhCard *c = &ph_me(&G)->card[card];
    const PhGuest *t = &PH_GUESTS[c->type];
    /* walking in from the door */
    int dt = frame_t - arrive[card];
    int ox = 0, oy = 0;
    if (dt >= 0 && dt < 14 && arrive[card]) {
        float f = 1.0f - dt / 14.0f;
        ox = (int)((PANEL_X + 30 - x) * f);
        oy = (int)((40 - y) * f);
    } else if (dt < 0) {
        return; /* not in yet */
    }
    bool wild = ph_is_wild(&G, card);
    int rug = t->traits & T_STAR ? C_AMBER : wild ? C_WINE : C_DUSK;
    gfx_rect(x + 2, y + 18, SLOT_W - 4, 8, rug);
    gfx_hline(x + 3, x + SLOT_W - 4, y + 17, rug);
    if (hi) gfx_rectb(x, y - 1, SLOT_W, SLOT_H, (frame_t / 8) % 2 ? C_WHITE : C_YELLOW);
    else if (target_ok) gfx_rectb(x, y - 1, SLOT_W, SLOT_H, C_SKY);
    int bob = (frame_t / 16 + i) % 4 == 0 ? 1 : 0;
    if (dim) spr_draw_ex(&ph_spr[c->type], x + 5 + ox, y + 1 + oy - bob, 0, NULL, C_SLATE);
    else spr_draw(&ph_spr[c->type], x + 5 + ox, y + 1 + oy - bob, 0);
    if (ox || oy) return;
    int v = ph_value_pop(&G, card), m = ph_value_cash(&G, card);
    if (v) draw_num(v, x + 3, y + 19, v > 0 ? C_YELLOW : C_ORANGE, v > 0 ? "+" : "");
    char mb[16];
    if (m > 0) snprintf(mb, sizeof mb, "$%d", m);
    else snprintf(mb, sizeof mb, "-$%d", -m);
    if (m) tiny_draw(mb, x + SLOT_W - 2 - tiny_width(mb), y + 19, m > 0 ? C_LIME : C_ORANGE);
    if (wild) icon_trouble(x + 1, y + 1, (frame_t / 10) % 2 ? C_RED : C_ORANGE);
    if (t->traits & T_STAR) icon_star(x + 19, y, C_YELLOW);
    if (t->traits & T_PEACE) gfx_circb(x + 22, y + 12, 2, C_CYAN);
    if (t->action != A_NONE) {
        /* the action badge: bright while it can still be used */
        bool ok = ph_can_act(&G, i);
        gfx_circ(x + 3, y + 12, 2, ok ? C_WHITE : C_DUSK);
        if (ok) gfx_pset(x + 3, y + 12, (frame_t / 12) % 2 ? C_YELLOW : C_ORANGE);
    }
}

static void draw_room(void) {
    PhPlayer *p = ph_me(&G);
    int w = PANEL_X - 4;
    /* the back wall: whitewash, a tiled dado and a window on the sea */
    gfx_rect(0, 16, w, 150, C_LIGHT);
    gfx_rect(0, 16, w, 4, C_WHITE);
    for (int x = 0; x < w; x += 8) {
        gfx_rect(x, 20, 8, 2, (x / 8) % 2 ? C_BLUE : C_SKY);
    }
    /* the floor: warm terracotta and a blue-and-white rug under the guests */
    gfx_rect(0, 22, w, 144, C_TAN);
    for (int y = 22; y < 166; y += 8)
        for (int x = ((y - 22) / 8) % 2 * 8; x < w; x += 16) gfx_rect(x, y, 8, 8, C_EARTH);
    int rows = (p->cap + PH_ROW - 1) / PH_ROW;
    int fh = rows * SLOT_H;
    gfx_rect(GRID_X - 5, GRID_Y - 3, PH_ROW * SLOT_W + 10, fh + 4, C_NAVY);
    gfx_rectb(GRID_X - 5, GRID_Y - 3, PH_ROW * SLOT_W + 10, fh + 4, C_CREAM);
    gfx_rectb(GRID_X - 3, GRID_Y - 1, PH_ROW * SLOT_W + 6, fh, C_BLUE);
    for (int x = GRID_X; x < GRID_X + PH_ROW * SLOT_W; x += 6) { gfx_pset(x, GRID_Y - 3, C_WHITE); gfx_pset(x + 3, GRID_Y + fh, C_WHITE); }
    /* the free spaces on the rug */
    for (int i = G.party.n; i < p->cap; i++) {
        int x = slot_x(i), y = slot_y(i);
        gfx_dither(x + 5, y + 17, SLOT_W - 10, 7, C_BLUE, 8);
        gfx_rectb(x + 4, y + 16, SLOT_W - 8, 9, C_BLUE);
    }
    /* below the rug: the rest of the room, a sofa and a lemon tree */
    int below = GRID_Y + fh + 6;
    if (below < 150) {
        gfx_rect(12, below + 4, 60, 12, C_WINE);
        gfx_rect(12, below, 60, 6, C_RED);
        gfx_rect(8, below + 2, 6, 14, C_WINE);
        gfx_rect(70, below + 2, 6, 14, C_WINE);
        gfx_rect(170, below + 8, 14, 10, C_ORANGE);
        gfx_circ(177, below + 2, 9, C_FOREST);
        gfx_circ(176, below, 7, C_JADE);
        gfx_pset(173, below - 1, C_YELLOW);
        gfx_pset(180, below + 3, C_YELLOW);
        gfx_pset(177, below - 4, C_YELLOW);
    }
    /* paper lanterns strung across the ceiling */
    for (int i = 0; i < 9; i++) {
        int x = 10 + i * 24, sw = (frame_t / 20 + i) % 2;
        gfx_rect(x - 1 + sw, 16, 4, 4, i % 3 == 0 ? C_ORANGE : i % 3 == 1 ? C_AMBER : C_PINK);
    }
}

static void draw_topbar(void) {
    PhPlayer *p = ph_me(&G);
    gfx_rect(0, 0, SCREEN_W, 15, C_INK);
    gfx_hline(0, SCREEN_W - 1, 15, C_DUSK);
    char b[32];
    if (G.players == 2) snprintf(b, sizeof b, "P%d  NIGHT %d/%d", G.turn + 1, p->night, PH_NIGHTS);
    else snprintf(b, sizeof b, "NIGHT %d/%d", p->night, PH_NIGHTS);
    text_draw(b, 4, 4, C_CREAM);
    icon_pop(96, 4);
    snprintf(b, sizeof b, "%d", p->pop);
    text_draw(b, 106, 4, C_YELLOW);
    icon_cash(140, 4);
    snprintf(b, sizeof b, "%d", p->cash);
    text_draw(b, 150, 4, C_LIME);
    /* the neighbour's window lights up at two rowdies: one more brings the police */
    bool lamp = ph_trouble(&G) >= 2;
    gfx_rect(300, 2, 12, 11, lamp ? C_YELLOW : C_NIGHT);
    gfx_rectb(300, 2, 12, 11, C_SLATE);
    gfx_vline(306, 3, 12, C_SLATE);
}

static void draw_info(int card, int x, int y, int w) {
    if (card < 0) return;
    const PhCard *c = &ph_me(&G)->card[card];
    const PhGuest *t = &PH_GUESTS[c->type];
    ui_panel(x, y, w, 60, C_NIGHT, t->traits & T_STAR ? C_AMBER : C_DUSK);
    tiny_draw(card_name(card), x + 4, y + 4, t->traits & T_STAR ? C_YELLOW : C_WHITE);
    int yy = y + 12;
    int v = G.party.where[card] == W_HOUSE ? ph_value_pop(&G, card) : t->pop + c->bonus;
    int m = G.party.where[card] == W_HOUSE ? ph_value_cash(&G, card) : t->cash;
    icon_pop(x + 4, yy);
    draw_num(v, x + 13, yy + 1, C_YELLOW, v > 0 ? "+" : "");
    icon_cash(x + 34, yy);
    draw_num(m, x + 43, yy + 1, C_LIME, m > 0 ? "+" : "");
    if (ph_is_wild(&G, card) || (t->traits & T_TROUBLE)) icon_trouble(x + w - 12, yy + 1, C_RED);
    tiny_lines(t->does[0] ? t->does : "NO SPECIAL TALENT.", x + 4, yy + 10, C_LIGHT);
    tiny_lines(t->flavour, x + 4, y + 44, C_SLATE);
}

static void draw_panel(void) {
    PhParty *pa = &G.party;
    int x = PANEL_X;
    gfx_rect(x - 4, 16, SCREEN_W - x + 4, 150, C_NIGHT);
    /* the door, in its arch */
    int dx = x + 22, dy = 22;
    gfx_rect(dx - 6, dy - 2, 44, 40, C_LIGHT);
    gfx_rect(dx - 4, dy, 40, 38, C_WHITE);
    bool open = pa->peek >= 0 || (frame_t - arrive[pa->n ? pa->house[pa->n - 1] : 0]) < 10;
    if (open) {
        gfx_rect(dx + 4, dy + 4, 24, 32, C_NAVY);
        for (int i = 0; i < 6; i++) gfx_pset(dx + 6 + (i * 7) % 20, dy + 6 + (i * 5) % 12, C_WHITE);
        if (pa->peek >= 0) spr_draw(&ph_spr[ph_me(&G)->card[pa->peek].type], dx + 8, dy + 18, 0);
    } else {
        spr_draw_scaled(&ph_spr[PS_DOOR], dx, dy + 4, 2, 0);
    }
    bool hi_door = state == S_PARTY && cur == CUR_DOOR, hi_end = state == S_PARTY && cur == CUR_END;
    bool hi_book = state == S_PARTY && cur == CUR_BOOK;
    int by = 62;
    gfx_rect(x, by, 92, 12, hi_door ? C_JADE : C_DUSK);
    text_center(pa->peek >= 0 ? "LET THEM IN" : "OPEN THE DOOR", x + 46, by + 3, hi_door ? C_WHITE : C_GREY);
    gfx_rect(x, by + 14, 92, 12, hi_end ? C_WINE : C_DUSK);
    text_center("END THE PARTY", x + 46, by + 17, hi_end ? C_WHITE : C_GREY);
    gfx_rect(x, by + 28, 92, 12, hi_book ? C_NAVY : C_DUSK);
    text_center("GUEST BOOK", x + 46, by + 31, hi_book ? C_WHITE : C_GREY);
    if (hi_door || hi_end || hi_book) ui_cursor(x - 8, (hi_door ? by : hi_end ? by + 14 : by + 28) + 3, frame_t);
    int shown = info_card;
    if (state == S_TARGET || state == S_BAN) shown = tcur < pa->n ? pa->house[tcur] : -1;
    draw_info(shown, x - 2, 106, 98);
}

static void draw_party_screen(void) {
    gfx_cls(C_INK);
    draw_room();
    PhParty *pa = &G.party;
    for (int i = 0; i < pa->n; i++) {
        bool hi = (state == S_PARTY && cur == i) || ((state == S_TARGET || state == S_BAN) && tcur == i);
        bool tok = state == S_TARGET && ph_target_ok(&G, sel, i);
        bool dim = state == S_TARGET && !tok;
        draw_slot(i, hi, tok, dim);
        if (state == S_TARGET && i == sel) gfx_rectb(slot_x(i) + 1, slot_y(i), SLOT_W - 2, SLOT_H - 2, C_LIME);
        /* the matchmaker's pair */
        if (state == S_TARGET && hi && PH_GUESTS[ph_me(&G)->card[pa->house[sel]].type].action == A_CUPID && tok)
            gfx_rectb(slot_x(i + 1), slot_y(i + 1) - 1, SLOT_W, SLOT_H, C_PINK);
    }
    draw_panel();
    draw_topbar();
    /* the bottom line: only the question after a shutdown */
    gfx_rect(0, 166, SCREEN_W, 14, C_INK);
    if (state == S_BAN) text_draw("WHO TAKES THE BLAME? THEY MISS THE NEXT PARTY.", 6, 169, C_GREY);
    if (msg_t > 0 && msg) {
        ui_panel(60, 70, 150, 20, C_NIGHT, C_ORANGE);
        text_center(msg, 135, 76, C_CREAM);
    }
}

static void draw_fetch(void) {
    draw_party_screen();
    gfx_darken_rect(0, 16, PANEL_X - 4, 150, 2);
    ui_panel(24, 26, 170, 136, C_NIGHT, C_JADE);
    text_center("WHO SHOULD COME?", 109, 32, C_LIME);
    int top = imax(0, fcur - 9);
    for (int i = top; i < nfetch && i < top + 10; i++) {
        int y = 46 + (i - top) * 11;
        int ty = fetch_list[i];
        if (i == fcur) gfx_rect(30, y - 2, 158, 11, C_DUSK);
        spr_draw(&ph_spr[ty], 34, y - 4, 0);
        text_draw(PH_GUESTS[ty].name, 54, y, i == fcur ? C_WHITE : C_GREY);
        char b[8];
        snprintf(b, sizeof b, "X%d", ph_count_type(&G, ty, W_POOL));
        tiny_draw(b, 172, y + 1, C_SLATE);
    }
}

static void draw_peek(void) {
    draw_party_screen();
    PhParty *pa = &G.party;
    if (pa->peek < 0) return;
    int ty = ph_me(&G)->card[pa->peek].type;
    ui_panel(30, 40, 180, 90, C_NIGHT, C_SKY);
    text_center("AT THE DOOR...", 120, 46, C_SKY);
    spr_draw_scaled(&ph_spr[ty], 60, 60, 2, 0);
    text_draw(card_name(pa->peek), 98, 62, C_WHITE);
    if (PH_GUESTS[ty].traits & (T_TROUBLE | T_MOON)) tiny_draw(PH_GUESTS[ty].traits & T_MOON ? "MOODY TONIGHT?" : "RUCKUS!", 98, 74, C_RED);
    if (PH_GUESTS[ty].traits & T_STAR) tiny_draw("A STAR GUEST!", 98, 74, C_YELLOW);
    bool room = pa->n < ph_me(&G)->cap;
    text_draw("LET THEM IN", 110, 96, menu_sel == 0 ? (room ? C_WHITE : C_SLATE) : C_GREY);
    text_draw("TURN THEM AWAY", 110, 108, menu_sel == 1 ? C_WHITE : C_GREY);
    ui_cursor(100, menu_sel ? 108 : 96, frame_t);
}

static void draw_confirm(void) {
    draw_party_screen();
    ui_panel(70, 64, 130, 46, C_NIGHT, C_WINE);
    text_center("END THE PARTY?", 135, 70, C_CREAM);
    text_draw("YES", 104, 88, menu_sel == 0 ? C_WHITE : C_SLATE);
    text_draw("NO", 150, 88, menu_sel == 1 ? C_WHITE : C_SLATE);
    ui_cursor(menu_sel == 0 ? 96 : 142, 88, frame_t);
}

static void draw_bust(void) {
    draw_party_screen();
    bool police = G.party.over == PO_POLICE;
    int t = state_t;
    int c = (t / 6) % 2 ? (police ? C_BLUE : C_RED) : (police ? C_RED : C_ORANGE);
    gfx_darken_rect(0, 16, SCREEN_W, 150, 1);
    gfx_rect(0, 16, SCREEN_W, 3, c);
    gfx_rect(0, 163, SCREEN_W, 3, c);
    ui_panel(40, 46, 190, 50, C_NIGHT, c);
    static const uint8_t g1[] = {C_WHITE, C_SKY, C_BLUE}, g2[] = {C_WHITE, C_YELLOW, C_RED};
    ui_fancy_center(police ? "THE POLICE!" : "FIRE MARSHAL!", 135, 52, 2, police ? g1 : g2, 3, C_INK, C_NIGHT);
    tiny_center(police ? "THREE ROWDIES. NO PAY TONIGHT." : "TOO MANY GUESTS. NO PAY TONIGHT.", 135, 78, C_CREAM);
    int vx = -40 + imin(t * 4, 128);
    gfx_rect(0, 130, PANEL_X - 4, 24, C_NIGHT);
    gfx_hline(0, PANEL_X - 5, 153, C_SLATE);
    spr_draw_scaled(&ph_spr[police ? PS_POLICE : PS_FIRE], vx, 132, 2, 0);
    if ((t / 6) % 2) gfx_rect(vx + 8, 128, 8, 3, police ? C_SKY : C_YELLOW);
    if (t > 100 && (t / 20) % 2) text_center("PRESS " GLYPH_A, 135, 110, C_WHITE);
}

static void draw_result(void) {
    draw_party_screen();
    PhParty *pa = &G.party;
    ui_panel(40, 40, 150, 92, C_NIGHT, C_YELLOW);
    static const uint8_t g1[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center("WHAT A NIGHT!", 115, 46, 1, g1, 3, C_INK, -1);
    int t = imin(state_t, 30);
    char b[48];
    icon_pop(58, 66);
    snprintf(b, sizeof b, "FAME %+d", pa->end_pop * t / 30);
    text_draw(b, 70, 66, C_YELLOW);
    icon_cash(58, 80);
    snprintf(b, sizeof b, "CASH %+d", pa->end_cash * t / 30);
    text_draw(b, 70, 80, C_LIME);
    if (pa->penalty) { snprintf(b, sizeof b, "UNPAID GUESTS: -%d FAME", pa->penalty); tiny_draw(b, 58, 95, C_ORANGE); }
    if (state_t > 40 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 115, 116, C_WHITE);
}

/* a guest card for the shop and the lists */
static void draw_card(int ty, int x, int y, bool hi, bool can) {
    const PhGuest *t = &PH_GUESTS[ty];
    bool star = t->traits & T_STAR;
    int bg = star ? C_BROWN : C_NIGHT;
    gfx_rect(x, y, 40, 40, bg);
    gfx_rectb(x, y, 40, 40, hi ? ((frame_t / 8) % 2 ? C_WHITE : C_YELLOW) : star ? C_AMBER : C_DUSK);
    spr_draw(&ph_spr[ty], x + 12, y + 4, 0);
    if (star) icon_star(x + 30, y + 2, C_YELLOW);
    if (t->traits & (T_TROUBLE | T_MOON)) icon_trouble(x + 3, y + 3, C_RED);
    char b[8];
    snprintf(b, sizeof b, "%d", t->cost);
    icon_pop(x + 3, y + 23);
    text_draw(b, x + 12, y + 23, can ? C_YELLOW : C_SLATE);
    if (!star) {
        int left = PH_STOCK - G.bought[ty];
        for (int k = 0; k < PH_STOCK; k++) gfx_rect(x + 5 + k * 8, y + 34, 5, 3, k < left ? C_LIME : C_DUSK);
    } else {
        tiny_draw("STAR", x + 12, y + 33, C_AMBER);
    }
}

static void draw_shop(void) {
    gfx_cls(C_NIGHT);
    /* a market street at dusk */
    for (int y = 16; y < 180; y += 4) gfx_dither(0, y, SCREEN_W, 2, C_DUSK, 2);
    PhPlayer *p = ph_me(&G);
    gfx_rect(0, 0, SCREEN_W, 15, C_INK);
    gfx_hline(0, SCREEN_W - 1, 15, C_DUSK);
    char b[48];
    if (G.players == 2) snprintf(b, sizeof b, "P%d  THE SHOP", G.turn + 1);
    else snprintf(b, sizeof b, "THE SHOP");
    text_draw(b, 4, 4, C_CREAM);
    snprintf(b, sizeof b, "NIGHT %d/%d", p->night, PH_NIGHTS);
    tiny_draw(b, 74, 6, C_GREY);
    icon_pop(130, 4);
    snprintf(b, sizeof b, "%d", p->pop);
    text_draw(b, 140, 4, C_YELLOW);
    icon_cash(176, 4);
    snprintf(b, sizeof b, "%d", p->cash);
    text_draw(b, 186, 4, C_LIME);
    snprintf(b, sizeof b, "HOUSE %d", p->cap);
    text_draw(b, 222, 4, C_SKY);
    for (int i = 0; i < G.npool; i++) {
        int x = 6 + (i % SHOP_COLS) * 43, y = 20 + (i / SHOP_COLS) * 43;
        draw_card(G.pool[i], x, y, shop_cur == i, ph_can_buy(&G, G.pool[i]));
    }
    /* space, the guest book and the next party */
    bool hs = shop_cur == G.npool, hn = shop_cur == G.npool + 1, hb = shop_cur == G.npool + 2;
    int cost = ph_expand_cost(p);
    gfx_rect(6, 152, 70, 20, hs ? C_TEAL : C_INK);
    gfx_rectb(6, 152, 70, 20, hs ? C_CYAN : C_DUSK);
    snprintf(b, sizeof b, "+1 SPACE $%d", cost);
    text_center(p->cap >= PH_MAX_HOUSE ? "FULL SIZE" : b, 41, 158, p->cash >= cost && p->cap < PH_MAX_HOUSE ? C_WHITE : C_SLATE);
    gfx_rect(80, 152, 66, 20, hb ? C_NAVY : C_INK);
    gfx_rectb(80, 152, 66, 20, hb ? C_SKY : C_DUSK);
    text_center("GUEST BOOK", 113, 158, hb ? C_WHITE : C_GREY);
    gfx_rect(150, 152, 68, 20, hn ? C_JADE : C_INK);
    gfx_rectb(150, 152, 68, 20, hn ? C_LIME : C_DUSK);
    text_center("NEXT PARTY", 184, 158, hn ? C_WHITE : C_GREY);
    /* the card on the counter */
    if (shop_cur < G.npool) {
        int ty = G.pool[shop_cur];
        const PhGuest *t = &PH_GUESTS[ty];
        ui_panel(222, 20, 94, 152, C_INK, t->traits & T_STAR ? C_AMBER : C_DUSK);
        spr_draw_scaled(&ph_spr[ty], 253, 26, 2, 0);
        tiny_draw(t->name, 226, 62, t->traits & T_STAR ? C_YELLOW : C_WHITE);
        icon_pop(226, 72);
        draw_num(t->pop, 235, 73, C_YELLOW, t->pop > 0 ? "+" : "");
        icon_cash(256, 72);
        draw_num(t->cash, 265, 73, C_LIME, t->cash > 0 ? "+" : "");
        if (t->traits & T_DRUM) tiny_draw("SEE BELOW", 226, 82, C_GREY);
        tiny_lines(t->does[0] ? t->does : "NO SPECIAL TALENT.", 226, 90, C_LIGHT);
        tiny_lines(t->flavour, 226, 128, C_SLATE);
    } else {
        ui_panel(222, 20, 94, 152, C_INK, C_DUSK);
    }
    if (msg_t > 0 && msg) {
        ui_panel(40, 80, 150, 20, C_NIGHT, C_ORANGE);
        text_center(msg, 115, 86, C_CREAM);
    }
}

static void draw_list(void) {
    gfx_cls(C_NIGHT);
    PhPlayer *p = ph_me(&G);
    text_center("THE GUEST BOOK", 160, 4, C_CREAM);
    tiny_draw("GUEST", 20, 16, C_SLATE);
    tiny_draw("HERE", 180, 16, C_SLATE);
    tiny_draw("TO COME", 214, 16, C_SLATE);
    tiny_draw("OUT", 262, 16, C_SLATE);
    int rows = 0, shown = 0;
    int top = fcur;
    for (int ty = 0; ty < G_COUNT; ty++) {
        int total = 0;
        for (int i = 0; i < p->ncards; i++) total += p->card[i].type == ty;
        if (!total) continue;
        if (rows++ < top) continue;
        if (shown >= 13) break;
        int y = 26 + shown * 11;
        spr_draw_ex(&ph_spr[ty], 2, y - 4, 0, NULL, -1);
        char b[16];
        text_draw(PH_GUESTS[ty].name, 20, y, PH_GUESTS[ty].traits & T_STAR ? C_YELLOW : C_LIGHT);
        snprintf(b, sizeof b, "%d", ph_count_type(&G, ty, W_HOUSE)); text_draw(b, 184, y, C_WHITE);
        snprintf(b, sizeof b, "%d", ph_count_type(&G, ty, W_POOL)); text_draw(b, 222, y, C_LIME);
        snprintf(b, sizeof b, "%d", ph_count_type(&G, ty, W_OUT)); text_draw(b, 264, y, C_ORANGE);
        shown++;
    }
    if (fcur > 0 && fcur >= rows) fcur = rows - 1;
}

/* ------------------------------------------------------------------ */
/* title, menus, win and loss                                           */

static void draw_villa(int t, int base) {
    /* the night sea, the house and its lit windows */
    gfx_cls(C_NIGHT);
    for (int i = 0; i < 40; i++) gfx_pset((i * 71) % 320, (i * 37) % 70, (t / 30 + i) % 5 ? C_GREY : C_WHITE);
    gfx_circ(270, 26, 10, C_CREAM);
    gfx_circ(274, 24, 9, C_NIGHT);
    gfx_rect(0, base, SCREEN_W, SCREEN_H - base, C_NAVY);
    for (int y = base + 2; y < SCREEN_H; y += 5) gfx_hline((t / 3 + y * 7) % 40, (t / 3 + y * 7) % 40 + 12, y, C_BLUE);
    int hx = 70, hy = base - 70;
    gfx_rect(hx, hy + 14, 180, 70, C_LIGHT);
    gfx_rect(hx - 6, hy + 8, 192, 8, C_WHITE);
    gfx_rect(hx + 60, hy - 6, 60, 16, C_WHITE);
    gfx_circ(hx + 90, hy - 6, 14, C_WHITE);
    gfx_rect(hx + 88, hy - 26, 4, 8, C_WHITE);
    for (int i = 0; i < 5; i++) {
        int wx = hx + 10 + i * 34, wy = hy + 26;
        bool lit = (t / 40 + i * 3) % 7 != 0;
        gfx_rect(wx, wy, 18, 24, lit ? C_YELLOW : C_DUSK);
        gfx_circ(wx + 9, wy, 9, lit ? C_YELLOW : C_DUSK);
        if (lit) {
            /* dancers in the windows */
            int s = (t / 12 + i) % 2;
            gfx_rect(wx + 5 + s, wy + 8, 4, 10, C_BROWN);
            gfx_circ(wx + 7 + s, wy + 6, 2, C_BROWN);
            gfx_rect(wx + 11 - s, wy + 10, 3, 8, C_BROWN);
        }
        gfx_rect(wx - 1, wy + 24, 20, 2, C_BLUE);
    }
    gfx_rect(hx + 82, hy + 56, 16, 28, C_BLUE);
    gfx_circ(hx + 90, hy + 56, 8, C_BLUE);
    /* lantern strings */
    for (int i = 0; i < 12; i++) {
        int lx = hx - 4 + i * 17, ly = hy + 12 + (i % 2) * 3;
        gfx_rect(lx, ly, 3, 4, (t / 15 + i) % 3 ? C_ORANGE : C_AMBER);
    }
}

static void draw_title(void) {
    draw_villa(frame_t, 150);
    /* fireworks */
    for (int k = 0; k < 3; k++) {
        int ph = (frame_t + k * 50) % 150;
        if (ph > 60) continue;
        int cx = 60 + k * 100, cy = 40 + (k % 2) * 10;
        for (int i = 0; i < 12; i++) {
            float a = i / 12.0f * 6.2832f;
            gfx_pset(cx + (int)(cosf(a) * ph * 0.5f), cy + (int)(sinf(a) * ph * 0.5f) + ph / 8, k == 0 ? C_PINK : k == 1 ? C_CYAN : C_YELLOW);
        }
    }
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_center("OPEN HOUSE", 160, 8, 3, grad, 4, C_INK, C_WINE);
    tiny_center("FOUR STARS UNDER ONE ROOF", 160, 36, C_CREAM);
    if (state_t > 20 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 160, C_WHITE);
}

static void draw_mode(void) {
    draw_villa(frame_t, 150);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(90, 40, 140, 80, C_NIGHT, C_AMBER);
    static const char *const M[3] = {"CONTINUE", "1 PLAYER", "2 PLAYERS"};
    for (int i = 0; i < 3; i++) {
        int y = 52 + i * 16;
        bool off = i == 0 && !sv.has_run;
        if (i == menu_sel) gfx_rect(96, y - 3, 128, 13, C_DUSK);
        text_draw(M[i], 116, y, off ? C_DUSK : i == menu_sel ? C_WHITE : C_GREY);
        if (i == menu_sel) ui_cursor(104, y, frame_t);
    }
    char b[48];
    snprintf(b, sizeof b, "STREAK %d  " GLYPH_DOT "  BEST %d", sv.streak, sv.best_streak);
    tiny_center(b, 160, 130, C_CREAM);
}

static const char *scen_name(int i) { return i == PH_RANDOM ? "RANDOM GUEST LIST" : PH_SCEN[i].name; }

static void draw_scen(void) {
    draw_villa(frame_t, 150);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(8, 10, 304, 160, C_NIGHT, C_AMBER);
    text_draw(mode_players == 2 ? "2 PLAYERS: CHOOSE A GUEST LIST" : "CHOOSE A GUEST LIST", 16, 16, C_CREAM);
    for (int i = 0; i <= PH_RANDOM; i++) {
        int y = 32 + i * 14;
        bool open = unlocked(i), won = (sv.won >> i) & 1;
        if (i == scen_sel) gfx_rect(14, y - 3, 150, 13, C_DUSK);
        char b[40];
        if (i < PH_RANDOM) snprintf(b, sizeof b, "%d  %s", i + 1, scen_name(i));
        else snprintf(b, sizeof b, "?  %s", scen_name(i));
        text_draw(open ? b : "   " GLYPH_LOCK " LOCKED", 26, y, !open ? C_DUSK : i == scen_sel ? C_WHITE : C_GREY);
        if (won) icon_star(154, y, C_YELLOW);
        if (i == scen_sel) ui_cursor(16, y, frame_t);
    }
    /* the chosen list's stars */
    int s = scen_sel;
    if (unlocked(s)) {
        if (s < PH_SCENARIOS) {
            tiny_lines(PH_SCEN[s].blurb, 176, 32, C_LIGHT);
            int k = 0;
            for (int i = 0; i < PH_SCEN[s].n; i++) {
                int ty = PH_SCEN[s].pool[i];
                if (!(PH_GUESTS[ty].traits & T_STAR)) continue;
                spr_draw_scaled(&ph_spr[ty], 180 + k * 60, 60, 2, 0);
                tiny_draw(PH_GUESTS[ty].name, 176 + k * 60, 96, C_YELLOW);
                k++;
            }

        } else {

            char b[40];
            snprintf(b, sizeof b, "STREAK %d  BEST %d", sv.streak, sv.best_streak);
            tiny_draw(b, 176, 76, C_CREAM);
        }
    } else {

    }
    if (msg_t > 0 && msg) text_draw(msg, 16, 150, C_ORANGE);
}

static void draw_intro(void) {
    draw_villa(frame_t, 150);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(10, 10, 300, 160, C_NIGHT, C_AMBER);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center(scen_name(G.scen), 160, 16, 1, grad, 3, C_INK, -1);
    tiny_center(G.scen < PH_SCENARIOS ? "THE GUESTS YOU CAN INVITE THIS SUMMER" : "TONIGHT'S RANDOM GUEST LIST", 160, 32, C_GREY);
    for (int i = 0; i < G.npool; i++) {
        int ty = G.pool[i];
        int x = 22 + (i % 8) * 36, y = 44 + (i / 8) * 40;
        spr_draw(&ph_spr[ty], x + 6, y, 0);
        if (PH_GUESTS[ty].traits & T_STAR) icon_star(x + 20, y - 2, C_YELLOW);
        char b[8];
        snprintf(b, sizeof b, "%d", PH_GUESTS[ty].cost);
        tiny_center(b, x + 14, y + 18, C_YELLOW);
    }
    if (state_t > 20 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 154, C_WHITE);
}

static void draw_turn(void) {
    draw_villa(frame_t, 150);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 1);
    ui_panel(70, 30, 180, 90, C_NIGHT, G.turn ? C_MAGENTA : C_SKY);
    static const uint8_t g1[] = {C_WHITE, C_CYAN, C_SKY}, g2[] = {C_WHITE, C_PINK, C_MAGENTA};
    char b[64];
    snprintf(b, sizeof b, "PLAYER %d", G.turn + 1);
    ui_fancy_center(b, 160, 36, 2, G.turn ? g2 : g1, 3, C_INK, C_NIGHT);
    snprintf(b, sizeof b, "NIGHT %d OF %d", ph_me(&G)->night, PH_NIGHTS);
    text_center(b, 160, 60, C_CREAM);
    for (int p = 0; p < 2; p++) {
        snprintf(b, sizeof b, "P%d  POP %d  $%d  HOUSE %d", p + 1, G.pl[p].pop, G.pl[p].cash, G.pl[p].cap);
        tiny_center(b, 160, 76 + p * 9, p == G.turn ? C_WHITE : C_SLATE);
    }
    if (state_t > 30 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 102, C_WHITE);
}

static void draw_end(void) {
    bool win = state == S_WIN;
    if (win) {
        draw_villa(frame_t, 150);
        for (int k = 0; k < 5; k++) {
            int ph = (frame_t * 2 + k * 37) % 120;
            if (ph > 70) continue;
            int cx = 30 + k * 64, cy = 30 + (k % 3) * 12;
            for (int i = 0; i < 16; i++) {
                float a = i / 16.0f * 6.2832f;
                gfx_rect(cx + (int)(cosf(a) * ph * 0.6f), cy + (int)(sinf(a) * ph * 0.6f) + ph / 6, 2, 2, (k + i) % 3 == 0 ? C_YELLOW : k % 2 ? C_PINK : C_CYAN);
            }
        }
    } else {
        draw_villa(0, 150);
        gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 3);
    }
    ui_panel(40, 60, 240, 70, C_NIGHT, win ? C_YELLOW : C_SLATE);
    static const uint8_t g1[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER}, g2[] = {C_LIGHT, C_GREY, C_SLATE};
    char b[48];
    if (G.players == 2) snprintf(b, sizeof b, G.winner ? "PLAYER %d WINS!" : "NOBODY WINS", G.winner);
    else snprintf(b, sizeof b, win ? "FOUR STARS!" : "SUMMER'S OVER");
    ui_fancy_center(b, 160, 68, 2, win ? g1 : g2, win ? 4 : 3, C_INK, win ? C_WINE : C_NIGHT);
    if (win) {
        /* the four stars on the terrace */
        int k = 0;
        PhPlayer *p = ph_me(&G);
        for (int i = 0; i < G.party.n && k < 4; i++) {
            int ty = p->card[G.party.house[i]].type;
            if (!(PH_GUESTS[ty].traits & T_STAR)) continue;
            spr_draw(&ph_spr[ty], 116 + k * 22, 92 - ((frame_t / 8 + k) % 2), 0);
            k++;
        }
        if (G.players == 1 && G.scen == PH_RANDOM) { snprintf(b, sizeof b, "STREAK %d", sv.streak); tiny_center(b, 160, 116, C_CREAM); }
    } else {
        tiny_center(G.players == 2 ? "BOTH HOUSES RAN OUT OF NIGHTS." : "25 NIGHTS AND NEVER FOUR STARS AT ONCE.", 160, 100, C_GREY);
        if (G.scen == PH_RANDOM && G.players == 1) tiny_center("THE STREAK STARTS AGAIN.", 160, 110, C_SLATE);
    }
    if (state_t > 90 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 146, C_WHITE);
}

static bool sheet_mode;

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    for (int i = 0; i < G_COUNT; i++) {
        int x = 4 + (i % 12) * 26, y = 4 + (i / 12) * 26;
        spr_draw(&ph_spr[i], x, y, 0);
    }
    spr_draw(&ph_spr[PS_DOOR], 4, 120, 0);
    spr_draw(&ph_spr[PS_LAMP], 30, 120, 0);
    spr_draw(&ph_spr[PS_POLICE], 50, 120, 0);
    spr_draw(&ph_spr[PS_FIRE], 80, 120, 0);
}

static void oh_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_MODE: draw_mode(); break;
    case S_SCEN: draw_scen(); break;
    case S_INTRO: draw_intro(); break;
    case S_TURN: draw_turn(); break;
    case S_PARTY: case S_TARGET: case S_BAN: draw_party_screen(); break;
    case S_FETCH: draw_fetch(); break;
    case S_PEEK: draw_peek(); break;
    case S_CONFIRM: draw_confirm(); break;
    case S_BUST: draw_bust(); break;
    case S_RESULT: draw_result(); break;
    case S_SHOP: draw_shop(); break;
    case S_WIN: case S_LOSE: draw_end(); break;
    case S_LIST: draw_list(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void oh_load(void) {
    ph_art_load();
    ph_audio_load();
}

static void oh_start(void) {
    load_save();
    memset(&G, 0, sizeof G);
    state = S_TITLE;
    state_t = 0;
    sheet_mode = false;
    game_set_pausable(false);
    music_play(PH_MUS_TITLE);
}

static void oh_quit(void) {
    if (G.players == 1 && sv.has_run && !G.done && G.pl[0].ncards) sv.run = G;
    save_now();
}

static void oh_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_NIGHT);
    for (int i = 0; i < 16; i++) gfx_pset(x + (i * 37) % w, y + (i * 13) % 24, (t / 30 + i) % 4 ? C_GREY : C_WHITE);
    gfx_rect(x, y + 50, w, 14, C_NAVY);
    gfx_rect(x + 20, y + 24, 102, 30, C_LIGHT);
    gfx_rect(x + 16, y + 20, 110, 5, C_WHITE);
    for (int i = 0; i < 4; i++) {
        bool lit = (t / 30 + i) % 5 != 0;
        gfx_rect(x + 26 + i * 24, y + 30, 12, 16, lit ? C_YELLOW : C_DUSK);
    }
    static const int GUESTS[4] = {G_PILOT, G_PUNK, G_GRANNY, G_GOAT};
    for (int i = 0; i < 4; i++) spr_draw(&ph_spr[GUESTS[i]], x + 24 + i * 24, y + 30 - ((t / 10 + i) % 2), 0);
    for (int i = 0; i < 8; i++) gfx_rect(x + 18 + i * 14, y + 19 + (i % 2) * 2, 3, 4, (t / 15 + i) % 2 ? C_ORANGE : C_AMBER);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_text("OPEN HOUSE", x + 4, y + 3, 1, grad, 3, C_INK, -1);
}

static int oh_query(const char *key, int *out) {
    PhPlayer *p = ph_me(&G);
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "scen")) { *out = G.scen + 1; return 1; }
    if (!strcmp(key, "night")) { *out = p->night; return 1; }
    if (!strcmp(key, "pop")) { *out = p->pop; return 1; }
    if (!strcmp(key, "cash")) { *out = p->cash; return 1; }
    if (!strcmp(key, "cap")) { *out = p->cap; return 1; }
    if (!strcmp(key, "n")) { *out = G.party.n; return 1; }
    if (!strcmp(key, "over")) { *out = G.party.over; return 1; }
    if (!strcmp(key, "trouble")) { *out = ph_trouble(&G); return 1; }
    if (!strcmp(key, "stars")) { *out = ph_stars(&G); return 1; }
    if (!strcmp(key, "cur")) { *out = cur; return 1; }
    if (!strcmp(key, "cards")) { *out = p->ncards; return 1; }
    if (!strcmp(key, "peek")) { *out = G.party.peek; return 1; }
    if (!strcmp(key, "banned")) { *out = p->banned; return 1; }
    if (!strcmp(key, "turn")) { *out = G.turn + 1; return 1; }
    if (!strcmp(key, "winner")) { *out = G.winner; return 1; }
    if (!strcmp(key, "done")) { *out = G.done; return 1; }
    if (!strcmp(key, "won")) { *out = sv.won; return 1; }
    if (!strcmp(key, "streak")) { *out = sv.streak; return 1; }
    if (!strcmp(key, "best_streak")) { *out = sv.best_streak; return 1; }
    if (!strcmp(key, "has_run")) { *out = sv.has_run; return 1; }
    if (!strcmp(key, "end_pop")) { *out = G.party.end_pop; return 1; }
    if (!strcmp(key, "end_cash")) { *out = G.party.end_cash; return 1; }
    if (!strcmp(key, "penalty")) { *out = G.party.penalty; return 1; }
    if (!strcmp(key, "got_pop")) { *out = G.party.got_pop; return 1; }
    if (!strcmp(key, "got_cash")) { *out = G.party.got_cash; return 1; }
    if (!strcmp(key, "collected")) { *out = G.party.got_pop + G.party.got_cash; return 1; }
    if (!strncmp(key, "wout", 4)) { *out = ph_count_type(&G, atoi(key + 4), W_OUT); return 1; }
    if (!strncmp(key, "wpool", 5)) { *out = ph_count_type(&G, atoi(key + 5), W_POOL); return 1; }
    if (!strcmp(key, "poolstars")) {
        int n = 0;
        for (int i = 0; i < G.npool; i++) n += (PH_GUESTS[G.pool[i]].traits & T_STAR) != 0;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "npool")) { *out = G.npool; return 1; }
    if (!strcmp(key, "players")) { *out = G.players; return 1; }
    if (!strncmp(key, "slot", 4)) { int i = atoi(key + 4); *out = i < G.party.n ? p->card[G.party.house[i]].type : -1; return 1; }
    if (!strncmp(key, "vpop", 4)) { int i = atoi(key + 4); *out = i < G.party.n ? ph_value_pop(&G, G.party.house[i]) : 0; return 1; }
    if (!strncmp(key, "vcash", 5)) { int i = atoi(key + 5); *out = i < G.party.n ? ph_value_cash(&G, G.party.house[i]) : 0; return 1; }
    if (!strncmp(key, "canact", 6)) { *out = ph_can_act(&G, atoi(key + 6)); return 1; }
    if (!strncmp(key, "have", 4)) {
        int ty = atoi(key + 4), n = 0;
        for (int i = 0; i < p->ncards; i++) n += p->card[i].type == ty;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "bought", 6)) { *out = G.bought[atoi(key + 6) % G_COUNT]; return 1; }
    if (!strncmp(key, "pool", 4)) { int i = atoi(key + 4); *out = i < G.npool ? G.pool[i] : -1; return 1; }
    if (!strncmp(key, "named", 5)) {
        int n = 0;
        for (int i = 0; i < p->ncards; i++) n += p->card[i].name != 0;
        *out = n;
        return 1;
    }
    return 0;
}

/* cheats build exact parties for the tests: "door TYPE TYPE ..." lets
 * those guests in, one knock each, in that order */
static int take_card_of(int ty) {
    PhPlayer *p = ph_me(&G);
    for (int i = 0; i < p->ncards; i++)
        if (p->card[i].type == ty && G.party.where[i] == W_POOL && i != G.party.peek) return i;
    return -1;
}

static int oh_cheat(const char *cmd) {
    int a, b;
    PhPlayer *p = ph_me(&G);
    if (sscanf(cmd, "new %d %d", &a, &b) == 2) {
        ph_new(&G, a - 1, b, 1234 + (uint64_t)a);
        if (b == 1) { sv.has_run = 1; }
        state = S_PARTY; state_t = 0; cur = CUR_DOOR;
        game_set_pausable(true);
        return 1;
    }
    if (sscanf(cmd, "give %d %d", &a, &b) == 2) {
        for (int i = 0; i < b && p->ncards < PH_MAX_CARDS; i++) {
            memset(&p->card[p->ncards], 0, sizeof p->card[0]);
            p->card[p->ncards++].type = (uint8_t)a;
        }
        return 1;
    }
    if (sscanf(cmd, "pop %d", &a) == 1) { p->pop = (int16_t)a; return 1; }
    if (sscanf(cmd, "cash %d", &a) == 1) { p->cash = (int16_t)a; return 1; }
    if (sscanf(cmd, "cap %d", &a) == 1) { p->cap = (uint8_t)a; return 1; }
    if (sscanf(cmd, "night %d", &a) == 1) { p->night = (uint8_t)a; return 1; }
    if (!strncmp(cmd, "door ", 5)) {
        /* let in exactly these guest types, one knock each */
        const char *s = cmd + 5;
        while (*s) {
            while (*s == ' ') s++;
            if (!*s) break;
            int ty = atoi(s);
            while (*s && *s != ' ') s++;
            int c = take_card_of(ty);
            if (c < 0 || G.party.over) continue;
            G.party.peek = (int16_t)c;
            int t = ph_trouble(&G);
            if (ph_open_door(&G)) { note_arrivals(); after_change(t); }
            if (G.party.over) break;
        }
        return 1;
    }
    if (sscanf(cmd, "act %d %d", &a, &b) == 2) {
        int t = ph_trouble(&G);
        if (ph_act(&G, a, b)) { if (G.party.nlast) note_arrivals(); if (state == S_PARTY || state == S_TARGET) after_change(t); }
        return 1;
    }
    if (sscanf(cmd, "shop %d", &a) == 1) { ph_buy(&G, a); return 1; }
    if (sscanf(cmd, "decide %d", &a) == 1) {
        int t = ph_trouble(&G);
        if (ph_peek_decide(&G, a != 0) && a) { note_arrivals(); after_change(t); }
        return 1;
    }
    if (!strcmp(cmd, "expand")) { ph_expand(&G); return 1; }
    if (!strcmp(cmd, "end")) { ph_end_party(&G); state = S_RESULT; state_t = 0; return 1; }
    if (!strcmp(cmd, "next")) { after_party(); return 1; }
    if (!strcmp(cmd, "leave")) { leave_shop(); return 1; }
    if (sscanf(cmd, "ban %d", &a) == 1) { ph_ban(&G, G.party.house[a]); after_party(); return 1; }
    if (sscanf(cmd, "won %d", &a) == 1) { sv.won = (uint8_t)a; save_now(); return 1; }
    if (sscanf(cmd, "streak %d", &a) == 1) { sv.streak = (uint8_t)a; if (a > sv.best_streak) sv.best_streak = (uint8_t)a; save_now(); return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    if (!strcmp(cmd, "menu")) { state = S_SCEN; state_t = 0; mode_players = 1; return 1; }
    return 0;
}

const GameDef GAME_OPENHOUSE = {
    "openhouse",
    "OPEN HOUSE",
    "1986",
    "STRATEGY",
    "THROW 25 SUMMER PARTIES AND GET FOUR STAR GUESTS UNDER ONE ROOF.",
    {"WIN A GUEST LIST", "WIN ALL FIVE GUEST LISTS", "WIN 5 RANDOM LISTS IN A ROW"},
    "D-PAD\tMOVE THE CURSOR\n"
    GLYPH_A "\tOPEN THE DOOR / USE /\n\tCHOOSE / BUY\n"
    GLYPH_B "\tCANCEL / BACK\n"
    "START\tPAUSE",
    C_WINE, C_YELLOW,
    oh_load, oh_start, oh_update, oh_draw, oh_quit, oh_label, oh_query, oh_cheat,
    "PARTY HOUSE", 25,
};
