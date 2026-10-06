/* TUSKWIND - a walrus dreams his way across a sky of islets. Cartridge 21
 * of UFO 40, a tribute to Waldorf's Journey (UFO 50 #21). See
 * docs/games/21-tuskwind.md. This file: the title, the story, the talk at
 * the end, the endings, the records and the test hooks. The journey is in
 * tuskwind_play.c, the maps in tuskwind_map.c, the brawl in
 * tuskwind_brawl.c. */
#include "tuskwind.h"

#define SAVE_MAGIC 0x544B5701u

TkwGame tkg;
TkwSave tks;

static void set_state(int s) {
    tkg.state = s;
    tkg.state_t = 0;
}

static bool vita_single(void) { return plat_kind() == PLAT_VITA; }

static void load_save(void) {
    TkwSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { tks = tmp; return; }
    memset(&tks, 0, sizeof tks);
    tks.magic = SAVE_MAGIC;
}

static void save_now(void) {
    tks.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &tks, (int)sizeof tks);
}

static void to_title(void) {
    set_state(TS_TITLE);
    game_set_pausable(false);
    input_set_versus(false);
    music_play(TKW_MUS_DREAM);
}

/* the journey's records, won or lost */
static void record_run(void) {
    if (tkg.terns_found > tks.most_terns) tks.most_terns = (uint8_t)tkg.terns_found;
    if (tkg.chests > tks.most_chests) tks.most_chests = (uint8_t)tkg.chests;
    if (tkg.max_pct > tks.best_pct) tks.best_pct = (uint8_t)tkg.max_pct;
    save_now();
}

static void start_journey(void) {
    input_consume();
    game_set_pausable(true);
    input_set_versus(false);
    uint64_t seed = tkg.seed_next ? tkg.seed_next : ((uint64_t)rng_next(&g_rng) << 32 | rng_next(&g_rng));
    tkg.seed_next = 0;
    tkw_new_journey(seed);
    tkw_bot_reset();
    if (tks.runs < 65535) tks.runs++;
    save_now();
    set_state(TS_JOURNEY);
    music_play(TKW_MUS_DREAM);
}

/* at the old one's side: the shells decide which talk it is */
static void begin_talk(void) {
    tkg.cherry_end = tkg.shells >= TKW_CHERRY_SHELLS;
    tkg.talk_page = 0;
    tkg.talk_t = 0;
    game_award(GOAL_SAUCER);
    if (tkg.cherry_end) game_award(GOAL_ALIEN);
    if (tks.wins < 65535) tks.wins++;
    if (tkg.cherry_end && tks.cherries < 65535) tks.cherries++;
    if (tkg.shells > tks.best_shells) tks.best_shells = (uint8_t)imin(255, tkg.shells);
    tkg.max_pct = 100;
    record_run();
    game_set_pausable(false);
    music_play(TKW_MUS_ENDING);
}

static void title_update(void) {
    game_set_pausable(false);
    tkg.frame_t++;
    if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { tkg.sel ^= 1; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if (tkg.state_t < 10 || !(btnp(BTN_A) || btnp(BTN_START))) return;
    if (tkg.sel == 1) {
        if (vita_single()) { sfx_play_name("ui_error"); return; }
        sfx_play_name("ui_ok");
        input_consume();
        set_state(TS_BRAWL_SETUP);
        return;
    }
    sfx_play_name("tkw_start");
    input_consume();
    tkg.talk_page = 0;
    set_state(TS_INTRO);
}

static int pages_of(const char *const *p) {
    int n = 0;
    while (p[n]) n++;
    return n;
}

static void tkw_update(void) {
    tkg.state_t++;
    uint32_t held = input_held();
    uint16_t in0 = (uint16_t)(held & 0xFF);
    switch (tkg.state) {
    case TS_TITLE: title_update(); break;
    case TS_INTRO:
        tkg.frame_t++;
        if (btnp(BTN_START)) { start_journey(); break; }
        if (tkg.state_t > 20 && btnp(BTN_A)) {
            tkg.state_t = 0;
            if (++tkg.talk_page >= pages_of(TKW_INTRO)) start_journey();
        }
        break;
    case TS_JOURNEY:
    case TS_HALL:
        game_set_pausable(true);
        tkw_journey_update(in0);
        tkg.prev_in = in0;
        if (tkg.state == TS_TALK && tkg.state_t == 0) begin_talk();
        if (tkg.state == TS_WAKE && tkg.state_t == 0) {
            record_run();
            game_set_pausable(false);
            music_play(TKW_MUS_WAKE);
        }
        break;
    case TS_TALK:
        tkg.frame_t++;
        tkg.talk_t++;
        if (tkg.talk_t > 20 && btnp(BTN_A)) {
            tkg.talk_t = 0;
            if (++tkg.talk_page >= tkw_talk_pages(tkg.cherry_end)) set_state(TS_ENDING);
        }
        break;
    case TS_ENDING:
        tkg.frame_t++;
        if (tkg.state_t >= 720 || (tkg.state_t > 240 && btnp(BTN_A))) {
            input_consume();
            set_state(TS_CREDITS);
        }
        break;
    case TS_CREDITS:
        tkg.frame_t++;
        if (btn(BTN_A)) tkg.state_t += 3; /* hold A to hurry them along */
        if (tkg.state_t >= 1200) { input_consume(); to_title(); }
        break;
    case TS_WAKE:
        tkg.frame_t++;
        if (tkg.state_t > 60 && btnp(BTN_A)) { input_consume(); to_title(); }
        break;
    case TS_BRAWL_SETUP: {
        static const int ROUNDS[5] = {1, 3, 5, 7, 9};
        int k = 0;
        while (k < 4 && ROUNDS[k] != tkg.rounds) k++;
        if (btn_repeat(BTN_LEFT) && k > 0) { tkg.rounds = ROUNDS[k - 1]; sfx_play_name("ui_move"); }
        if (btn_repeat(BTN_RIGHT) && k < 4) { tkg.rounds = ROUNDS[k + 1]; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { sfx_play_name("ui_back"); to_title(); break; }
        if (tkg.state_t > 10 && btnp(BTN_A)) {
            sfx_play_name("tkw_start");
            input_consume();
            game_set_pausable(true);
            input_set_versus(true);
            tkw_brawl_start_match();
            set_state(TS_BRAWL);
            music_play(TKW_MUS_BRAWL);
        }
        break;
    }
    case TS_BRAWL: {
        game_set_pausable(true);
        uint16_t in1 = (uint16_t)((held >> BTN_P2_SHIFT) & 0xFF);
        if (tkg.bot_brawl) in1 = (uint16_t)tkw_brawl_bot();
        tkw_brawl_update(in0, in1);
        break;
    }
    case TS_BRAWL_ROUND:
        tkg.frame_t++;
        if (tkg.state_t >= 150 || (tkg.state_t > 50 && btnp(BTN_A))) {
            int need = tkg.rounds / 2 + 1;
            input_consume();
            if (tkg.br[0].wins >= need || tkg.br[1].wins >= need) {
                set_state(TS_BRAWL_OVER);
                game_set_pausable(false);
                music_play(TKW_MUS_ROUND);
            } else {
                tkw_brawl_start_round();
                set_state(TS_BRAWL);
                music_play(TKW_MUS_BRAWL);
            }
        }
        break;
    case TS_BRAWL_OVER:
        tkg.frame_t++;
        if (tkg.state_t > 60 && btnp(BTN_A)) {
            input_consume();
            input_set_versus(false);
            set_state(TS_BRAWL_SETUP);
            game_set_pausable(false);
            music_play(TKW_MUS_DREAM);
        }
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* cartridge interface                                                      */

static void tkw_load(void) {
    tkw_art_load();
    tkw_audio_load();
}

static void tkw_start(void) {
    load_save();
    memset(&tkg, 0, sizeof tkg);
    tkg.rounds = 3;
    to_title();
}

static void tkw_quit(void) {
    input_set_versus(false);
    if (tkg.state == TS_JOURNEY || tkg.state == TS_HALL) record_run();
    else save_now();
}

static int count_signs(uint32_t mask) {
    int n = 0;
    for (int i = 0; i < TKW_SIGNS; i++) n += (mask >> i) & 1;
    return n;
}

static int first_thing(int kind) {
    for (int i = 0; i < tkw_w.nth; i++)
        if (tkw_w.th[i].alive && tkw_w.th[i].kind == kind) return i;
    return -1;
}

/* every hop on the generated route can be jumped, calm and without flapping */
static int route_ok(void) {
    int prev = -1, bad = 0;
    for (int i = 0; i < tkw_w.nplat; i++) {
        if (!tkw_w.plat[i].route) continue;
        if (prev >= 0 && !tkw_can_reach(&tkw_w, prev, i)) bad++;
        prev = i;
    }
    return bad;
}

static int tkw_query(const char *key, int *out) {
    const TkwHero *h = &tkg.h;
    const TkwBody *b = &h->b;
    if (!strcmp(key, "bot")) { *out = tkw_bot_buttons(); return 1; }
    if (!strcmp(key, "bot_plans")) { *out = tkw_bot_plans; return 1; }
    if (!strcmp(key, "bot_fails")) { *out = tkw_bot_fails; return 1; }
    if (!strncmp(key, "bot_dbg", 7) && key[7] >= '0' && key[7] <= '3') {
        extern int tkw_bot_dbg[4];
        *out = tkw_bot_dbg[key[7] - '0'];
        return 1;
    }
    if (!strcmp(key, "state")) { *out = tkg.state; return 1; }
    if (!strcmp(key, "sel")) { *out = tkg.sel; return 1; }
    if (!strcmp(key, "ps")) { *out = h->ps; return 1; }
    if (!strcmp(key, "mode")) { *out = b->mode; return 1; }
    if (!strcmp(key, "x")) { *out = (int)lroundf(b->x); return 1; }
    if (!strcmp(key, "y")) { *out = (int)lroundf(b->y); return 1; }
    if (!strcmp(key, "vx10")) { *out = (int)lroundf(b->vx * 10); return 1; }
    if (!strcmp(key, "vy10")) { *out = (int)lroundf(b->vy * 10); return 1; }
    if (!strcmp(key, "plat")) { *out = b->plat; return 1; }
    if (!strcmp(key, "ride")) { *out = b->ride; return 1; }
    if (!strcmp(key, "face")) { *out = h->face; return 1; }
    if (!strcmp(key, "aim10")) { *out = (int)lroundf(h->aim * 10); return 1; }
    if (!strcmp(key, "charge")) { *out = h->charge; return 1; }
    if (!strcmp(key, "charging")) { *out = h->charging; return 1; }
    if (!strcmp(key, "flapping")) { *out = h->flapping; return 1; }
    if (!strcmp(key, "stamina")) { *out = b->stamina; return 1; }
    if (!strcmp(key, "kite")) { *out = b->kite; return 1; }
    if (!strcmp(key, "spinner")) { *out = h->spinner; return 1; }
    if (!strcmp(key, "spin_fuel")) { *out = h->spin_fuel; return 1; }
    if (!strcmp(key, "red_line")) { *out = h->red_line; return 1; }
    if (!strcmp(key, "item_sel")) { *out = h->menu_sel; return 1; }
    if (!strcmp(key, "shop_sel")) { *out = h->shop_sel; return 1; }
    if (!strcmp(key, "takeoff")) { *out = h->takeoff; return 1; }
    if (!strcmp(key, "hurt")) { *out = h->hurt_t; return 1; }
    if (!strcmp(key, "ball_x")) { *out = (int)lroundf(h->ball.x); return 1; }
    if (!strcmp(key, "ball_mode")) { *out = h->ball.mode; return 1; }
    if (!strcmp(key, "ball_plat")) { *out = h->ball.plat; return 1; }
    if (!strcmp(key, "cam_x")) { *out = (int)lroundf(tkg.cam_x); return 1; }
    if (!strcmp(key, "cam_y")) { *out = (int)lroundf(tkg.cam_y); return 1; }
    if (!strcmp(key, "shells")) { *out = tkg.shells; return 1; }
    if (!strcmp(key, "spent")) { *out = tkg.spent; return 1; }
    if (!strcmp(key, "keys")) { *out = tkg.keys; return 1; }
    if (!strcmp(key, "terns")) { *out = tkg.terns; return 1; }
    if (!strcmp(key, "terns_used")) { *out = tkg.terns_used; return 1; }
    if (!strcmp(key, "terns_found")) { *out = tkg.terns_found; return 1; }
    if (!strcmp(key, "chests")) { *out = tkg.chests; return 1; }
    if (!strcmp(key, "pct")) { *out = tkg.pct; return 1; }
    if (!strcmp(key, "max_pct")) { *out = tkg.max_pct; return 1; }
    if (!strcmp(key, "wind")) { *out = tkg.wind; return 1; }
    if (!strcmp(key, "wind_t")) { *out = tkg.wind_t; return 1; }
    if (!strcmp(key, "sign_near")) { *out = tkg.sign_near; return 1; }
    if (!strcmp(key, "signs_run")) { *out = count_signs((uint32_t)tkg.signs_run); return 1; }
    if (!strcmp(key, "secret")) { *out = tkg.secret_read; return 1; }
    if (!strcmp(key, "in_hall")) { *out = tkg.in_hall; return 1; }
    if (!strcmp(key, "hall_terns")) { *out = tkg.hall_terns; return 1; }
    if (!strcmp(key, "cherry_end")) { *out = tkg.cherry_end; return 1; }
    if (!strcmp(key, "talk_page")) { *out = tkg.talk_page; return 1; }
    if (!strcmp(key, "rescues")) { *out = tkg.rescues; return 1; }
    if (!strcmp(key, "msg")) { *out = tkg.msg_t > 0; return 1; }
    if (!strncmp(key, "inv", 3) && key[3] >= '0' && key[3] < '0' + IT_COUNT) { *out = tkg.inv[key[3] - '0']; return 1; }
    /* every line of words fits its box (three lines of 280 px) in the font */
    if (!strcmp(key, "text_bad")) {
        const char *all[64];
        int n = 0;
        char buf[200];
        for (int i = 0; i < TKW_SIGNS; i++) all[n++] = TKW_SIGN_TEXT[i];
        all[n++] = TKW_SECRET_TEXT;
        for (int i = 0; TKW_INTRO[i]; i++) all[n++] = TKW_INTRO[i];
        for (int i = 0; TKW_TALK_CHERRY[i]; i++) all[n++] = TKW_TALK_CHERRY[i];
        for (int i = 0; TKW_TALK_GOLD[i]; i++) all[n++] = TKW_TALK_GOLD[i];
        int saved = tkg.shells, bad = 0;
        tkg.shells = 49; /* the widest number the short talk can hold */
        for (int i = 0; i < tkw_talk_pages(false); i++) all[n++] = tkw_talk_page(false, i, buf, sizeof buf);
        char l[4][UI_WRAP_LEN];
        for (int i = 0; i < n; i++) {
            if (ui_wrap(all[i], 280, false, l, 4) > 3) bad++;
            if (text_missing(all[i])) bad++;
        }
        tkg.shells = saved;
        for (int i = 0; TKW_CREDITS[i]; i++) bad += text_width(TKW_CREDITS[i]) > SCREEN_W - 8 || text_missing(TKW_CREDITS[i]);
        for (int i = 0; i < IT_COUNT; i++) bad += tiny_missing(TKW_ITEM_NAME[i]) || text_missing(TKW_ITEM_NAME[i]);
        *out = bad;
        return 1;
    }
    /* the map */
    if (!strcmp(key, "map_w")) { *out = tkw_w.w; return 1; }
    if (!strcmp(key, "plats")) { *out = tkw_w.nplat; return 1; }
    if (!strcmp(key, "map_hash")) {
        uint32_t hsh = 2166136261u;
        for (int i = 0; i < tkw_w.nplat; i++) hsh = (hsh ^ (uint32_t)(tkw_w.plat[i].x * 7 + tkw_w.plat[i].y)) * 16777619u;
        *out = (int)(hsh & 0x7FFFFFFF);
        return 1;
    }
    if (!strcmp(key, "route")) { *out = tkw_w.nroute; return 1; }
    if (!strcmp(key, "route_bad")) { *out = route_ok(); return 1; }
    if (!strcmp(key, "goal_plat")) { *out = tkw_w.goal_plat; return 1; }
    if (!strcmp(key, "on_goal")) { *out = b->plat == tkw_w.goal_plat && !tkg.in_hall; return 1; }
    if (!strncmp(key, "count_", 6)) { *out = tkw_count_things(atoi(key + 6)); return 1; }
    if (!strcmp(key, "shell_pieces")) { *out = tkw_count_things(TH_COCKLE) + tkw_count_things(TH_WHELK); return 1; }
    if (!strcmp(key, "shell_value")) {
        *out = tkw_count_things(TH_COCKLE) + 4 * tkw_count_things(TH_WHELK);
        return 1;
    }
    if (!strcmp(key, "foams")) {
        int n = 0;
        for (int i = 0; i < tkw_w.nplat; i++) n += tkw_w.plat[i].kind == PK_FOAM;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "rocks")) {
        int n = 0;
        for (int i = 0; i < tkw_w.nplat; i++) n += tkw_w.plat[i].kind == PK_ROCK;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "mantas")) {
        int n = 0;
        for (int i = 0; i < tkw_w.nmanta; i++) n += tkw_w.manta[i].alive;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "plat_gone")) { *out = b->plat >= 0 ? tkw_w.plat[b->plat].gone : -1; return 1; }
    if (!strcmp(key, "plat_kind")) { *out = b->plat >= 0 ? tkw_w.plat[b->plat].kind : -1; return 1; }
    if (!strcmp(key, "takeoff_gone")) { *out = tkw_w.plat[h->takeoff].gone; return 1; }
    /* the first live thing of a kind: thing_state_K, thing_x_K */
    if (!strncmp(key, "thing_state_", 12) || !strncmp(key, "thing_x_", 8) || !strncmp(key, "thing_plat_", 11)) {
        const char *us = strrchr(key, '_');
        int i = first_thing(atoi(us + 1));
        *out = -1;
        if (i < 0) return 1;
        if (key[6] == 's') *out = tkw_w.th[i].state;
        else if (key[6] == 'x') *out = (int)lroundf(tkw_w.th[i].x);
        else *out = tkw_w.th[i].plat;
        return 1;
    }
    /* the brawl */
    if (!strcmp(key, "rounds")) { *out = tkg.rounds; return 1; }
    if (!strcmp(key, "round_no")) { *out = tkg.round_no; return 1; }
    if (!strcmp(key, "round_winner")) { *out = tkg.round_winner; return 1; }
    if (!strcmp(key, "wins0")) { *out = tkg.br[0].wins; return 1; }
    if (!strcmp(key, "wins1")) { *out = tkg.br[1].wins; return 1; }
    if (!strcmp(key, "out0")) { *out = tkg.br[0].out; return 1; }
    if (!strcmp(key, "out1")) { *out = tkg.br[1].out; return 1; }
    if (!strcmp(key, "bx0")) { *out = (int)lroundf(tkg.br[0].b.x); return 1; }
    if (!strcmp(key, "bx1")) { *out = (int)lroundf(tkg.br[1].b.x); return 1; }
    if (!strcmp(key, "by0")) { *out = (int)lroundf(tkg.br[0].b.y); return 1; }
    if (!strcmp(key, "bmode0")) { *out = tkg.br[0].b.mode; return 1; }
    if (!strcmp(key, "bmode1")) { *out = tkg.br[1].b.mode; return 1; }
    if (!strcmp(key, "bvx10_1")) { *out = (int)lroundf(tkg.br[1].b.vx * 10); return 1; }
    if (!strcmp(key, "solid_plats")) {
        int n = 0;
        for (int i = 0; i < tkw_w.nplat; i++) n += !tkw_w.plat[i].gone;
        *out = n;
        return 1;
    }
    /* the save */
    if (!strcmp(key, "save_signs")) { *out = count_signs(tks.signs); return 1; }
    if (!strcmp(key, "save_secret")) { *out = (tks.signs >> TKW_SIGNS) & 1; return 1; }
    if (!strcmp(key, "save_runs")) { *out = tks.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = tks.wins; return 1; }
    if (!strcmp(key, "save_cherries")) { *out = tks.cherries; return 1; }
    if (!strcmp(key, "save_terns")) { *out = tks.most_terns; return 1; }
    if (!strcmp(key, "save_chests")) { *out = tks.most_chests; return 1; }
    if (!strcmp(key, "save_pct")) { *out = tks.best_pct; return 1; }
    if (!strcmp(key, "save_shells")) { *out = tks.best_shells; return 1; }
    return 0;
}

/* the islet of the first live thing of a kind; Burl put beside it */
static bool place_near(int kind, float off) {
    int i = first_thing(kind);
    if (i < 0 || tkw_w.th[i].plat < 0) return false;
    tkw_hero_place(tkw_w.th[i].plat, tkw_w.th[i].x + off);
    tkg.h.ps = PS_PLAY;
    return true;
}

static int tkw_cheat(const char *cmd) {
    int a, b;
    float x, y;
    TkwHero *h = &tkg.h;
    if (sscanf(cmd, "seed %d", &a) == 1) { tkg.seed_next = (uint64_t)a; return 1; }
    if (!strcmp(cmd, "journey")) { start_journey(); return 1; }
    if (sscanf(cmd, "place %d %f", &a, &x) == 2) { tkw_hero_place(iclamp(a, 0, tkw_w.nplat - 1), x); h->ps = PS_PLAY; return 1; }
    if (sscanf(cmd, "land %d", &a) == 1) {
        a = iclamp(a, 0, tkw_w.nplat - 1);
        tkw_hero_place(a, tkw_w.plat[a].x + tkw_w.plat[a].w / 2.0f);
        h->ps = PS_PLAY;
        h->takeoff = a;
        h->takeoff_x = h->b.x;
        tkw_on_land(a);
        tkw_camera(true);
        return 1;
    }
    if (sscanf(cmd, "routepct %d", &a) == 1) {
        /* land on the route islet nearest that far along */
        int best = -1, bd = 1000;
        for (int i = 0; i < tkw_w.nplat; i++) {
            const TkwPlat *p = &tkw_w.plat[i];
            if (!p->route || p->kind == PK_FOAM) continue;
            int d = iabs(tkw_progress_at(p->x + p->w / 2.0f) - a);
            if (d < bd) { bd = d; best = i; }
        }
        char buf[32];
        snprintf(buf, sizeof buf, "land %d", best);
        return tkw_cheat(buf);
    }
    if (sscanf(cmd, "near %d %f", &a, &x) == 2) return place_near(a, x) ? 1 : 0;
    if (sscanf(cmd, "spawnpath %d %f", &a, &x) == 2) {
        /* a thing at that fraction of the way along the tern's ride back */
        if (h->ps != PS_RESCUE) return 0;
        const TkwPlat *p = &tkw_w.plat[h->takeoff];
        tkw_add_thing(&tkw_w, iclamp(a, 0, TH_KINDS - 1), h->rescue_x + (h->takeoff_x - h->rescue_x) * x,
                      h->rescue_y + (p->y - h->rescue_y) * x, -1);
        return 1;
    }
    if (sscanf(cmd, "pos %f %f", &x, &y) == 2) {
        h->b.x = x;
        h->b.y = y;
        h->b.mode = BM_AIR;
        h->b.plat = -1;
        h->b.ride = -1;
        h->b.vx = h->b.vy = 0;
        h->ps = PS_PLAY;
        h->flap_ok = false;
        return 1;
    }
    if (sscanf(cmd, "vel %f %f", &x, &y) == 2) { h->b.vx = x; h->b.vy = y; return 1; }
    if (sscanf(cmd, "shells %d", &a) == 1) { tkg.shells = a; return 1; }
    if (sscanf(cmd, "keys %d", &a) == 1) { tkg.keys = a; return 1; }
    if (sscanf(cmd, "terns %d", &a) == 1) { tkg.terns = a; return 1; }
    if (sscanf(cmd, "stamina %d", &a) == 1) { h->b.stamina = iclamp(a, 0, TKW_STAMINA); return 1; }
    if (sscanf(cmd, "item %d %d", &a, &b) == 2) { tkg.inv[iclamp(a, 0, IT_COUNT - 1)] = b; return 1; }
    if (sscanf(cmd, "wind %d", &a) == 1) { tkw_set_wind(isign(a), 0); return 1; }
    if (sscanf(cmd, "aim %f", &x) == 1) { h->aim = x; return 1; }
    if (sscanf(cmd, "face %d", &a) == 1) { h->face = a < 0 ? -1 : 1; return 1; }
    if (!strcmp(cmd, "god")) { tkg.god = !tkg.god; return 1; }
    if (!strcmp(cmd, "collect")) { tkg.bot_collect = !tkg.bot_collect; return 1; }
    if (!strcmp(cmd, "brawlbot")) { tkg.bot_brawl = !tkg.bot_brawl; return 1; }
    if (!strcmp(cmd, "hall")) { tkw_enter_hall(); return 1; }
    if (!strcmp(cmd, "goal")) {
        /* the last islet, at its west end */
        tkw_hero_place(tkw_w.goal_plat, tkw_w.plat[tkw_w.goal_plat].x + 6.0f);
        h->ps = PS_PLAY;
        tkw_camera(true);
        return 1;
    }
    if (sscanf(cmd, "ride %d", &a) == 1) {
        TkwManta *m = &tkw_w.manta[iclamp(a, 0, tkw_w.nmanta - 1)];
        h->b.mode = BM_RIDE;
        h->b.ride = iclamp(a, 0, tkw_w.nmanta - 1);
        h->b.plat = -1;
        h->b.x = m->x;
        h->b.y = m->y - TKW_MANTA_HH;
        h->ps = PS_PLAY;
        return 1;
    }
    if (sscanf(cmd, "manta %d %f %f", &a, &x, &y) == 3) {
        /* a manta parked here, still */
        a = iclamp(a, 0, TKW_MAX_MANTAS - 1);
        if (a >= tkw_w.nmanta) tkw_w.nmanta = a + 1;
        TkwManta *m = &tkw_w.manta[a];
        m->cx = x;
        m->y0 = y;
        m->span = 0;
        m->alive = 1;
        tkw_manta_pos(m, tkg.frame_t, &m->x, &m->y);
        return 1;
    }
    if (sscanf(cmd, "spawn %d %f %f", &a, &x, &y) == 3) {
        int t = tkw_add_thing(&tkw_w, iclamp(a, 0, TH_KINDS - 1), x, y, -1);
        /* stood on the islet under it, if any */
        for (int i = 0; t >= 0 && i < tkw_w.nplat; i++) {
            const TkwPlat *p = &tkw_w.plat[i];
            if (x >= p->x && x <= p->x + p->w && fabsf(y - p->y) < 1) tkw_w.th[t].plat = (int16_t)i;
        }
        return 1;
    }
    {
        /* plat X Y [W [KIND]] : a new islet */
        int wd = 40, kind = PK_LEDGE;
        if (sscanf(cmd, "plat %d %d %d %d", &a, &b, &wd, &kind) >= 2) {
            tkw_add_plat(&tkw_w, a, b, wd, iclamp(kind, 0, 2));
            return 1;
        }
    }
    if (!strcmp(cmd, "blank")) {
        /* an empty sky with one islet in it, for the rule tests */
        TkwWorld *w = &tkw_w;
        memset(w, 0, sizeof *w);
        w->w = TKW_MAP_W;
        w->h = TKW_MAP_H;
        w->sea_y = TKW_SEA_Y;
        w->start_plat = tkw_add_plat(w, 100, 200, 80, PK_LEDGE);
        w->goal_plat = -1;
        w->start_x = 140;
        w->goal_x = 140 + 4000;
        tkw_hero_place(w->start_plat, 140);
        h->takeoff = w->start_plat;
        h->takeoff_x = 140;
        h->ps = PS_PLAY;
        tkw_camera(true);
        return 1;
    }
    if (sscanf(cmd, "landkind %d", &a) == 1) {
        for (int i = 0; i < tkw_w.nplat; i++)
            if (tkw_w.plat[i].kind == a && !tkw_w.plat[i].gone) {
                char buf[32];
                snprintf(buf, sizeof buf, "land %d", i);
                return tkw_cheat(buf);
            }
        return 0;
    }
    if (!strcmp(cmd, "clear")) {
        /* nothing on the map but its islets (for the physics tests) */
        for (int i = 0; i < tkw_w.nth; i++) tkw_w.th[i].alive = 0;
        for (int k = 0; k < tkw_w.nmanta; k++) tkw_w.manta[k].alive = 0;
        return 1;
    }
    if (sscanf(cmd, "wares %d %d", &a, &b) == 2) {
        /* the first stall still standing sells these two */
        int i = first_thing(TH_SHOP);
        if (i < 0) return 0;
        tkw_w.th[i].arg = iclamp(a, 0, IT_COUNT - 1) | iclamp(b, 0, IT_COUNT - 1) << 4;
        return 1;
    }
    if (sscanf(cmd, "rounds %d", &a) == 1) { tkg.rounds = a; return 1; }
    if (sscanf(cmd, "brawlpos %d %f %f", &a, &x, &y) == 3) {
        TkwBrawler *br = &tkg.br[iclamp(a, 0, 1)];
        br->b.x = x;
        br->b.y = y;
        br->b.mode = BM_AIR;
        br->b.plat = -1;
        br->b.vx = br->b.vy = 0;
        return 1;
    }
    if (sscanf(cmd, "brawlvel %d %f %f", &a, &x, &y) == 3) {
        tkg.br[iclamp(a, 0, 1)].b.vx = x;
        tkg.br[iclamp(a, 0, 1)].b.vy = y;
        return 1;
    }
    if (sscanf(cmd, "crumble %d", &a) == 1) { tkg.crumble_t = a; return 1; }
    return 0;
}

const GameDef GAME_TUSKWIND = {
    "tuskwind",
    "TUSKWIND",
    "1986",
    "ARCADE PLATFORM",
    "A DREAMING WALRUS CROSSES A SKY OF ISLETS. AIM AND CHARGE EVERY LEAP.",
    {"GET HALFWAY THROUGH THE DREAM", "REACH THE OLD ONE AT THE DREAM'S END", "MEET THE OLD ONE CARRYING 50 SHELLS"},
    GLYPH_LEFT GLYPH_RIGHT "\tSCOOT ALONG THE ISLET\n"
    GLYPH_UP GLYPH_DOWN "\tSWING THE AIM\n"
    "HOLD " GLYPH_A "\tCHARGE; LET GO TO JUMP\n"
    "AIR: HOLD " GLYPH_A "\tFLY; " GLYPH_LEFT GLYPH_RIGHT " SPEED\n"
    "CHARGING: " GLYPH_B "\tCALL IT OFF\n"
    "HOLD " GLYPH_B "\tITEMS; LET GO TO USE\n"
    "START\tPAUSE\n"
    "\n"
    "AIM DEAD FLAT TO LUNGE.\n"
    "SPRATS REFILL THE FLAPPING BAR.\n"
    "WALK INTO THINGS TO USE THEM.",
    C_WINE, C_SKY,
    tkw_load, tkw_start, tkw_update, tkw_draw, tkw_quit, tkw_draw_label, tkw_query, tkw_cheat,
    "WALDORF'S JOURNEY", 21,
    NULL,
};
