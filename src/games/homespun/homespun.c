/* HOMESPUN - fix the Tumbleweed and fly home from Oddmoor.
 * Cartridge 44 of UFO 40, a tribute to Pilot Quest (UFO 50 #44).
 * Every rule and where it comes from is in docs/games/44-homespun.md.
 * This file is the cartridge: the title, the story, camp and the Wilds on
 * screen, talking, the noodlers, Shuffle's chests, the hopstones, the
 * endings, the save, the real-time hook and the test hooks. The camp's
 * economy is in homespun_base.c, the action in homespun_wild.c. */
#include "homespun.h"

#define HS_MAGIC 0x48530004u
#define SAVE_EVERY_MS 30000u   /* while the console is on, the camp is saved this often */

static int state, state_t, frame_t, title_sel, confirm_new, intro_page;
static bool rt_loaded, had_file, god;
static uint32_t rt_since_save, rt_ms_frac;
static int home_gain[RES_COUNT], home_carry, home_odds;
static int launch_idol;

/* talk: up to four pages; the Wilds wait while it is open */
static char talk_page[4][96];
static int talk_n, talk_i, talk_t;
/* Shuffle's chests */
static int gam_state, gam_t, gam_bet, gam_sel, gam_prize[6], gam_pos[6];

int hs_game_state(void) { return state; }
bool hs_talk_open(void) { return talk_n > 0; }
int hs_gamble_state(void) { return gam_state; }

/* ------------------------------------------------------------------ */
/* save                                                                 */

static void sync_trip(void) {
    if (!sv.on_trip || state != S_PLAY || hs.area == AR_BASE) return;
    sv.room = (uint8_t)hs.room;
    sv.px = (int16_t)(hs.px / HS_U);
    sv.py = (int16_t)(hs.py / HS_U);
}

static void fresh_save(uint32_t seed) {
    memset(&sv, 0, sizeof sv);
    sv.magic = HS_MAGIC;
    sv.seed = seed;
    hs_world_make(seed);
}

static void read_save(void) {
    HsSave tmp;
    int n = game_save_read(HS_SLOT, &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == HS_MAGIC) {
        sv = tmp;
        had_file = true;
    } else {
        fresh_save(0);
        had_file = false;
    }
    hs_world_make(sv.seed);
    rt_since_save = 0;
    rt_loaded = true;
}

static void save_now(void) {
    if (!sv.started) return;
    /* the SAVE DATA screen may have deleted the file while another cartridge ran:
     * then this camp is gone, and it must not come back */
    if (had_file && game_save_raw_size(HS_SLOT) == 0) {
        fresh_save(0);
        had_file = false;
        return;
    }
    sync_trip();
    sv.magic = HS_MAGIC;
    game_save_write(HS_SLOT, &sv, (int)sizeof sv);
    had_file = true;
    rt_since_save = 0;
}

static void check_goals(void) {
    if (sv.loom_home) game_award(GOAL_BEACON);
    if (sv.escapes) game_award(GOAL_SAUCER);
    if (sv.idol_escapes) game_award(GOAL_ALIEN);
}

/* ------------------------------------------------------------------ */
/* the real-time hook: the camp works while the console is on           */

static void hs_realtime(uint32_t ms) {
    if (!rt_loaded) read_save();
    if (!sv.started) return;
    hs_produce(ms);
    rt_ms_frac += ms;
    sv.alive_s += rt_ms_frac / 1000u;
    rt_ms_frac %= 1000u;
    rt_since_save += ms;
    if (rt_since_save >= SAVE_EVERY_MS) save_now();
}

/* ------------------------------------------------------------------ */
/* flow                                                                  */

static void pause_pick(int i);
static const char *const PAUSE_PIPE[1] = {"USE THE THUNDERPIPE"};
static const char *const PAUSE_YOYO[1] = {"USE THE YO-YO"};

static void set_pause_items(void) {
    if (state == S_PLAY && sv.pipe) game_pause_items(1, sv.weapon ? PAUSE_YOYO : PAUSE_PIPE, pause_pick);
    else game_pause_items(0, NULL, NULL);
}

static void pause_pick(int i) {
    (void)i;
    if (!sv.pipe) return;
    sv.weapon ^= 1;
    set_pause_items();
    save_now();
}

static void area_music(void) {
    if (state != S_PLAY) return;
    int song = HS_MUS_CAMP;
    if (hs.area == AR_OVER) song = hs.biome >= B_CRAGS ? HS_MUS_DEEP : HS_MUS_WILDS;
    else if (hs.area == AR_CAVE) song = HS_MUS_CAVE;
    else if (hs.area == AR_DUN) song = HS_MUS_DEEP;
    if (hs.boss >= 0) song = hs.mob[hs.boss].kind == E_MAWBO ? HS_MUS_MAWBO : HS_MUS_BOSS;
    music_play(song);
}

static void to_title(void) {
    state = S_TITLE;
    state_t = 0;
    title_sel = 0;
    confirm_new = 0;
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    music_play(HS_MUS_TITLE);
}

static void enter_camp(int x, int y) {
    hs_sim_enter(HS_R_BASE, x * HS_U, y * HS_U);
}

static void to_play(void) {
    state = S_PLAY;
    state_t = 0;
    talk_n = 0;
    gam_state = 0;
    hm.open = 0;
    game_set_pausable(true);
    if (sv.on_trip) {
        hs_sim_enter(sv.room < HS_ROOMS - 1 ? sv.room : HS_START_SCREEN, sv.px * HS_U, sv.py * HS_U);
        hs.dirx = 0;
        hs.diry = 1;
    } else {
        enter_camp(160, 104);
        hs.dirx = 0;
        hs.diry = -1;
    }
    set_pause_items();
    area_music();
}

static void talk(int n, const char *const *pages) {
    talk_n = imin(n, 4);
    talk_i = 0;
    talk_t = 0;
    for (int i = 0; i < talk_n; i++) snprintf(talk_page[i], sizeof talk_page[i], "%s", pages[i]);
    sfx_play_name("hs_talk");
}

static void talk1(const char *s) { talk(1, &s); }

static const char *const INTRO[3] = {
    "WICK FLIES THE PARCEL ROUTE PAST THE MOOR-MOONS. ONE NIGHT A SPARK IN THE ENGINE, A SPIN, A THUMP...",
    "THE TUMBLEWEED LIES IN THE HEATHER OF ODDMOOR. THREE PARTS ARE GONE, CARRIED OFF INTO THE WILDS.",
    "OLD BURL THE TREE WAVES A BRANCH. \"WELCOME, PILOT! HIT THE GLOWSTONE. WE'LL GROW THE REST.\"",
};

static void new_game(void) {
    uint32_t seed = rng_next(&g_rng) ^ (uint32_t)engine_frame() * 2654435761u;
    if (seed == 0) seed = 1;
    fresh_save(seed);
    sv.started = 1;
    sv.room = HS_R_BASE;
    save_now();
    state = S_INTRO;
    state_t = 0;
    intro_page = 0;
    game_set_pausable(false);
}

/* after the ending: a new camp on the same moon, the stones remembered */
static void new_loop(void) {
    HsSave keep = sv;
    fresh_save(keep.seed);
    sv.started = 1;
    sv.loop = (uint8_t)imin(255, keep.loop + 1);
    sv.marks = keep.marks;
    sv.spent_marks = keep.spent_marks;
    memcpy(sv.stone, keep.stone, sizeof sv.stone);
    sv.escapes = keep.escapes;
    sv.idol_escapes = keep.idol_escapes;
    sv.intro_seen = 1;
    sv.trips = keep.trips;
    sv.fadeouts = keep.fadeouts;
    sv.hits = keep.hits;
    sv.alive_s = keep.alive_s;
    sv.excursion = keep.excursion;
    sv.room = HS_R_BASE;
    save_now();
}

/* ------------------------------------------------------------------ */
/* trips                                                                 */

static void begin_trip_at(int room, int x, int y) {
    hs_start_trip(room, x * HS_U, y * HS_U);
    sfx_play_name("hs_gate");
    save_now();
    area_music();
}

static void come_home(void) {
    for (int r = 0; r < RES_COUNT; r++) home_gain[r] = sv.gain[r];
    home_carry = sv.carry;
    home_odds = sv.odds;
    hs_bank_trip();
    state = S_HOME;
    state_t = 0;
    enter_camp(160, 26);
    hs.dirx = 0;
    hs.diry = 1;
    save_now();
    check_goals();
    music_play(HS_MUS_HOME);
}

static void faded(void) {
    hs_lose_trip();
    state = S_FADED;
    state_t = 0;
    enter_camp(160, 26);
    save_now();
    music_play(HS_MUS_FADE);
}

/* ------------------------------------------------------------------ */
/* talking to folk and things in the Wilds                               */

static void npc_talk(int who) {
    switch (who) {
    case N_KIT: {
        static const char *const P[3] = {
            "KIT: WICK?! IT'S ME, YOUR SISTER! MY SKIFF CAME DOWN HERE YEARS AGO.",
            "KIT: I LIKE IT HERE. DON'T TELL MUM. THOSE TWO CHESTS FILL UP AGAIN EVERY TIME YOU COME.",
            "KIT: WHEN YOU FLY HOME, WAVE AT MY CAVE.",
        };
        talk(3, P);
        break;
    }
    case N_HUSH:
        if (sv.pipe) talk1("HUSH: TANGER SAYS THANK YOU. I MEAN... I SAY THANK YOU. BYE.");
        else if (sv.letter) talk1("HUSH: DID... DID TANGER GET MY LETTER YET?");
        else {
            static const char *const P[2] = {
                "HUSH: EEP! OH. YOU'RE NOT A GRUBLET. SORRY. I DON'T GO OUT MUCH.",
                "HUSH: COULD YOU TAKE THIS LETTER TO TANGER? SHE LIVES IN ANOTHER CAVE. THE ORANGE ONE.",
            };
            talk(2, P);
            sv.letter = 1;
            save_now();
        }
        break;
    case N_TANGER:
        if (sv.letter) {
            static const char *const P[2] = {
                "TANGER: A LETTER FROM HUSH? AT LAST! OH, THAT SHY LITTLE BLUE THING.",
                "TANGER: TAKE MY GRANDMA'S THUNDERPIPE. EACH SHOT COSTS A GLINT. PICK IT FROM THE START MENU.",
            };
            talk(2, P);
            sv.letter = 0;
            sv.pipe = 1;
            set_pause_items();
            sfx_play_name("hs_part");
            save_now();
        } else if (sv.pipe) {
            talk1("TANGER: BOOM! HA HA. GIVE MY LOVE TO HUSH.");
        } else {
            talk1("TANGER: HUSH NEVER WRITES. HUSH NEVER COMES OUT OF THAT CAVE.");
        }
        break;
    case N_SHUFFLE:
        hs_menu_clear("MADAME SHUFFLE");
        for (int b = 0; b < 3; b++) {
            static const int BET[3] = {1, 5, 10};
            char l[28], c[20];
            snprintf(l, sizeof l, "BET %d ODD%s", BET[b], BET[b] > 1 ? "S" : "");
            snprintf(c, sizeof c, "%d/%d", sv.odds, BET[b]);
            hs_menu_add(l, c, M_BET, BET[b], sv.odds >= BET[b]);
        }
        hm.open = 1;
        break;
    default: break;
    }
}

static const char *const VENDOR_NAME[V_KINDS] = {"10 DATA", "15 THREAD", "A TONIC", "A TONIC"};
static const int VENDOR_PRICE[V_KINDS] = {3, 5, 3, 2};

static void thing_act(int i) {
    HsThing *t = &hs.thing[i];
    char l[28], c[20];
    switch (t->kind) {
    case TH_CHEST: case TH_CACHE:
        if (!t->open) hs_open_chest(i);
        break;
    case TH_NPC:
        npc_talk(t->arg);
        break;
    case TH_TABLE:
        npc_talk(N_SHUFFLE);
        break;
    case TH_VENDOR:
        hs_menu_clear("A NOODLER");
        snprintf(l, sizeof l, "BUY %s", VENDOR_NAME[t->arg % V_KINDS]);
        snprintf(c, sizeof c, "%d ODDS", VENDOR_PRICE[t->arg % V_KINDS]);
        hs_menu_add(l, c, M_BUY, t->arg, sv.odds >= VENDOR_PRICE[t->arg % V_KINDS]);
        hm.open = 1;
        break;
    case TH_PAD: {
        int p = t->arg;
        if (!(sv.pads >> p & 1)) {
            static const int PRICE[4] = {1, 5, 10, 15};
            int price = PRICE[imin(sv.pads_woken, 3)];
            hs_menu_clear("A SLEEPING HOPSTONE");
            snprintf(c, sizeof c, "%d ODDS", price);
            hs_menu_add("WAKE IT", c, M_WAKE, p, sv.odds >= price);
        } else {
            snprintf(l, sizeof l, "HOPSTONE %d", p);
            hs_menu_clear(l);
            hs_menu_add("HOP HOME TO CAMP", "", M_HOP, 0, true);
            for (int k = 1; k < HS_PADS; k++)
                if (k != p && (sv.pads >> k & 1)) {
                    snprintf(l, sizeof l, "HOP TO STONE %d", k);
                    hs_menu_add(l, "", M_HOP, k, true);
                }
        }
        hm.open = 1;
        break;
    }
    default: break;
    }
}

static void pad_arrive(int p) {
    int s = hw.pad_screen[p];
    hs_sim_enter(s, (4 * HS_T + 8) * HS_U, (6 * HS_T + 8 + 14) * HS_U);
    hs.dirx = 0;
    hs.diry = 1;
    sfx_play_name("hs_hop");
    area_music();
}

static void gamble_start(int bet) {
    gam_bet = bet;
    sv.odds = (uint16_t)(sv.odds - bet);
    int prize[6] = {0, bet, 2 * bet, 3 * bet, 6 * bet, -bet}; /* bars; the last: jerky and odds */
    for (int i = 0; i < 6; i++) gam_prize[i] = prize[i];
    for (int i = 5; i > 0; i--) {
        int j = rng_range(&g_rng, 0, i), tt = gam_prize[i];
        gam_prize[i] = gam_prize[j];
        gam_prize[j] = tt;
    }
    for (int i = 0; i < 6; i++) gam_pos[i] = i;
    gam_state = 1;
    gam_t = 0;
    gam_sel = 0;
    sfx_play_name("hs_shuffle");
}

static void gamble_pay(void) {
    int p = gam_prize[gam_sel];
    char b[64];
    if (p > 0) {
        hs_drop(IT_BAR, p, hs.px, hs.py + 12 * HS_U);
        snprintf(b, sizeof b, "SHUFFLE: %d BAR%s! THE MOON LIKES YOU.", p, p > 1 ? "S" : "");
    } else if (p < 0) {
        hs_drop(IT_JERKY, -p, hs.px, hs.py + 12 * HS_U);
        hs_drop(IT_ODD, -2 * p, hs.px, hs.py + 12 * HS_U);
        snprintf(b, sizeof b, "SHUFFLE: JERKY AND ODDS! LUNCH IS ON ME.");
    } else {
        snprintf(b, sizeof b, "SHUFFLE: EMPTY! THE CHESTS GIVETH AND THE CHESTS TAKETH.");
    }
    hs_say(b);
}

static void menu_do(int i) {
    HsMenuLine *l = &hm.line[i];
    if (!l->ok) { sfx_play_name("hs_nope"); return; }
    int act = l->act, arg = l->arg;
    if (act == M_LAUNCH) {
        if (arg == 0) {
            hs_menu_clear("LAUNCH FOR HOME?");
            hs_menu_add("LAUNCH NOW", "", M_LAUNCH, 1, true);
            hs_menu_add("NOT YET", "", M_CLOSE, 0, true);
            hm.open = 1;
            hm.sel = 1;
            return;
        }
        hm.open = 0;
        state = S_LAUNCH;
        state_t = 0;
        launch_idol = sv.idol;
        sv.escapes = (uint8_t)imin(255, sv.escapes + 1);
        sv.marks = (uint8_t)imin(255, sv.marks + 1);
        if (launch_idol) sv.idol_escapes = (uint8_t)imin(255, sv.idol_escapes + 1);
        game_set_pausable(false);
        game_pause_items(0, NULL, NULL);
        save_now();
        check_goals();
        music_play(HS_MUS_END);
        return;
    }
    if (act == M_CLOSE) { hm.open = 0; return; }
    if (act == M_WAKE) {
        static const int PRICE[4] = {1, 5, 10, 15};
        int price = PRICE[imin(sv.pads_woken, 3)];
        if (sv.odds < price) { sfx_play_name("hs_nope"); return; }
        sv.odds = (uint16_t)(sv.odds - price);
        sv.pads |= (uint8_t)(1 << arg);
        sv.pads_woken++;
        hm.open = 0;
        hs_say("THE HOPSTONE HUMS AWAKE.");
        sfx_play_name("hs_hop");
        save_now();
        return;
    }
    if (act == M_HOP) {
        hm.open = 0;
        if (hs.area == AR_BASE) {
            if (sv.res[RES_JERKY] <= 0) { sfx_play_name("hs_nope"); return; }
            hs_start_trip(hw.pad_screen[arg], (4 * HS_T + 8) * HS_U, (6 * HS_T + 8 + 14) * HS_U);
            save_now();
            sfx_play_name("hs_hop");
            area_music();
        } else if (arg == 0) {
            come_home();
        } else {
            pad_arrive(arg);
        }
        return;
    }
    if (act == M_BUY) {
        int price = VENDOR_PRICE[arg % V_KINDS];
        if (sv.odds < price) { sfx_play_name("hs_nope"); return; }
        sv.odds = (uint16_t)(sv.odds - price);
        int32_t x = hs.px, y = hs.py + 10 * HS_U;
        if (arg == V_DATA) hs_drop(IT_DATA, 10, x, y);
        else if (arg == V_THREAD) hs_drop(IT_THREAD, 15, x, y);
        else sv.time_f += TONIC_S * 60;
        sfx_play_name("hs_buy");
        thing_act(hs_front_thing() >= 0 ? hs_front_thing() : 0);
        return;
    }
    if (act == M_BET) {
        hm.open = 0;
        gamble_start(arg);
        return;
    }
    if (hs_base_act(act, arg)) {
        sfx_play_name("hs_buy");
        if (act == M_RESEARCH) sfx_play_name("hs_part");
        save_now();
        int keep = hm.sel;
        int p = hs_front_spot();
        if (p >= 0) hs_base_menu(p);
        hm.sel = imin(keep, hm.n - 1);
    } else {
        sfx_play_name("hs_nope");
    }
}

static void menu_update(void) {
    if (hm.n == 0) { hm.open = 0; return; }
    if (btn_repeat(BTN_UP)) { hm.sel = (hm.sel + hm.n - 1) % hm.n; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { hm.sel = (hm.sel + 1) % hm.n; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { hm.open = 0; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A)) {
        if (hm.line[hm.sel].act == M_NOTHING) { hm.open = 0; return; }
        menu_do(hm.sel);
    }
}

static void gamble_update(void) {
    gam_t++;
    if (gam_state == 1) {
        /* the chests change places for a while */
        if (gam_t % 8 == 0) {
            int a = rng_range(&g_rng, 0, 5), b = rng_range(&g_rng, 0, 5), tt = gam_pos[a];
            gam_pos[a] = gam_pos[b];
            gam_pos[b] = tt;
            sfx_play_name("hs_step");
        }
        if (gam_t >= 90) { gam_state = 2; gam_t = 0; }
    } else if (gam_state == 2) {
        if (btnp(BTN_LEFT)) { gam_sel = (gam_sel + 5) % 6; sfx_play_name("ui_move"); }
        if (btnp(BTN_RIGHT)) { gam_sel = (gam_sel + 1) % 6; sfx_play_name("ui_move"); }
        if (btnp(BTN_A)) { gam_state = 3; gam_t = 0; sfx_play_name("hs_chest"); }
    } else if (gam_state == 3 && gam_t >= 50) {
        gamble_pay();
        gam_state = 0;
    }
}

/* ------------------------------------------------------------------ */
/* play                                                                  */

static void camp_a(void) {
    int p = hs_front_spot();
    if (p < 0) return;
    switch (p) {
    case P_STONE: talk1("THE GLOWSTONE HUMS. HIT IT WITH THE YO-YO: GLINTS FALL OUT."); break;
    case P_TOLLY:
        if (sv.res[RES_JERKY] > 0) {
            char b[96];
            snprintf(b, sizeof b, "TOLLY: %d JERKY IS %d SECONDS OUT THERE. THE WILDS ARE THROUGH THE GATE.",
                     sv.res[RES_JERKY], sv.res[RES_JERKY] * hs_jerky_seconds());
            talk1(b);
        } else {
            talk1("TOLLY: NO JERKY, NO WILDS. GRISTLE SMOKES IT, IF YOU BUILD HER A SMOKEHOUSE.");
        }
        break;
    case P_NEST: talk1("MOTHER LOOM SPINS HER THREAD, ONE SPOOL EVERY HALF MINUTE."); break;
    default:
        hs_base_menu(p);
        if (hm.open) sfx_play_name("hs_talk");
        break;
    }
}

static void play_update(void) {
    if (talk_n > 0) {
        talk_t++;
        if (talk_t > 8 && btnp(BTN_A | BTN_B)) {
            talk_t = 0;
            if (++talk_i >= talk_n) talk_n = 0;
            sfx_play_name("hs_talk");
        }
        return;
    }
    if (hm.open) { menu_update(); return; }
    if (gam_state) { gamble_update(); return; }
    uint32_t held = input_held();
    uint32_t press = 0;
    for (int b = 0; b < 8; b++)
        if (btnp(1 << b)) press |= 1u << b;
    if (press & BTN_A) {
        if (hs.area == AR_BASE) camp_a();
        else {
            int t = hs_front_thing();
            if (t >= 0) thing_act(t);
        }
        if (talk_n || hm.open || gam_state) return;
    }
    int room = hs.room, boss = hs.boss;
    if (god && sv.on_trip) sv.time_f = imax(sv.time_f, 600);
    hs.events = 0;
    hs_sim_update(held, press);
    int ev = hs.events;
    if (ev & EV_LOOM) { game_award(GOAL_BEACON); save_now(); }
    if (ev & EV_FADED) { faded(); return; }
    if (ev & EV_HOME) { come_home(); return; }
    if (ev & EV_GATE) {
        if (sv.res[RES_JERKY] > 0) begin_trip_at(HS_START_SCREEN, 160, HS_FH - 10);
        else {
            hs.py = 26 * HS_U;
            talk1("TOLLY: WHOA THERE. NO JERKY, NO WILDS.");
        }
        return;
    }
    if (hs.room != room || (hs.boss >= 0) != (boss >= 0)) area_music();
    if (ev & (EV_BOSSDOWN | EV_IDOL)) save_now();
}

static void hs_update(void) {
    frame_t++;
    state_t++;
    switch (state) {
    case S_TITLE: {
        bool cont = sv.started != 0;
        int rows = cont ? 2 : 1;
        if (confirm_new) {
            if (btnp(BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN)) { confirm_new = 3 - confirm_new; sfx_play_name("ui_move"); }
            if (btnp(BTN_B)) { confirm_new = 0; sfx_play_name("ui_back"); }
            if (btnp(BTN_A)) {
                if (confirm_new == 1) new_game();
                confirm_new = 0;
                sfx_play_name("ui_ok");
            }
            break;
        }
        if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { title_sel = (title_sel + 1) % rows; sfx_play_name("ui_move"); }
        if (state_t > 20 && btnp(BTN_A | BTN_START)) {
            sfx_play_name("ui_ok");
            if (cont && title_sel == 0) {
                if (!sv.intro_seen) { state = S_INTRO; state_t = 0; intro_page = 0; }
                else to_play();
            } else if (cont) {
                confirm_new = 2; /* NO is the default */
            } else {
                new_game();
            }
        }
        break;
    }
    case S_INTRO:
        if (state_t > 20 && btnp(BTN_A | BTN_START)) {
            state_t = 0;
            if (++intro_page >= 3) { sv.intro_seen = 1; save_now(); to_play(); }
            sfx_play_name("hs_talk");
        }
        if (state_t == 1 && intro_page == 0) music_play(HS_MUS_TITLE);
        break;
    case S_PLAY:
        play_update();
        break;
    case S_FADED:
    case S_HOME:
        if (state_t > 40 && btnp(BTN_A | BTN_B | BTN_START)) {
            state = S_PLAY;
            state_t = 0;
            game_set_pausable(true);
            set_pause_items();
            area_music();
        }
        break;
    case S_LAUNCH:
        if (state_t > 420 && btnp(BTN_A | BTN_START)) { state = S_CREDITS; state_t = 0; }
        break;
    case S_CREDITS:
        if (state_t > 600 || (state_t > 60 && btnp(BTN_A | BTN_START))) {
            new_loop();
            to_title();
        }
        break;
    default: break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                               */

static void draw_box_text(const char *s, int y) {
    ui_panel(16, y, 288, 42, C_NIGHT, C_CREAM);
    text_wrap(s, 24, y + 7, 272, C_WHITE, 10);
}

static void draw_gamble(void) {
    ui_panel(40, 40, 240, 84, C_NIGHT, C_MAGENTA);
    text_center("MADAME SHUFFLE'S CHESTS", 160, 46, C_PINK);
    for (int i = 0; i < 6; i++) {
        int slot = gam_pos[i];
        int x = 58 + slot * 36, y = 70;
        if (gam_state == 1) y += (int)(sinf((float)(gam_t + i * 7) * 0.5f) * 4.0f);
        bool sel = gam_state >= 2 && gam_sel == slot;
        gfx_rect(x, y, 24, 18, C_BROWN);
        gfx_rect(x, y, 24, 6, C_EARTH);
        gfx_rectb(x, y, 24, 18, sel ? C_YELLOW : C_INK);
        gfx_rect(x + 10, y + 5, 4, 4, C_AMBER);
        if (gam_state == 3 && sel) {
            int p = gam_prize[i];
            char b[16];
            if (p > 0) snprintf(b, sizeof b, "%dB", p);
            else if (p < 0) snprintf(b, sizeof b, "MEAT");
            else snprintf(b, sizeof b, "--");
            tiny_center(b, x + 12, y + 22, C_YELLOW);
        }
    }
    if (gam_state == 2) tiny_center(GLYPH_LEFT GLYPH_RIGHT " PICK A CHEST   " GLYPH_A " OPEN", 160, 110, C_LIGHT);
    else if (gam_state == 1) tiny_center("WATCH THEM GO...", 160, 110, C_LIGHT);
}

static void draw_title(void) {
    gfx_cls(C_NIGHT);
    for (int i = 0; i < 60; i++) {
        int x = (i * 73 + 11) % SCREEN_W, y = (i * 41 + 7) % 110;
        gfx_pset(x, y, (i + frame_t / 20) % 5 ? C_SLATE : C_LIGHT);
    }
    /* two small moons, the heather, the crash */
    gfx_circ(292, 18, 9, C_CREAM);
    gfx_circ(296, 15, 8, C_NIGHT);
    gfx_circ(20, 26, 4, C_LIGHT);
    for (int x = 0; x < SCREEN_W; x++) gfx_vline(x, 128 + (int)(sinf((float)x * 0.05f) * 4.0f), SCREEN_H, C_WINE);
    for (int x = 0; x < SCREEN_W; x += 3) gfx_pset(x, 136 + (x * 7) % 9, C_PURPLE);
    gfx_rect(26, 118, 58, 14, C_LIGHT);
    gfx_rect(20, 126, 70, 7, C_GREY);
    gfx_rect(46, 111, 18, 8, C_SKY);
    gfx_rectb(26, 118, 58, 14, C_INK);
    gfx_line(72, 120, 80, 131, C_INK);
    gfx_circ(78 + (frame_t / 12) % 3, 108 - (frame_t / 8) % 10, 2, C_GREY);
    int gx = 270, gy = 118;
    for (int k = 0; k < 11; k++) {
        gfx_hline(gx - k, gx + k, gy - 10 + k, k % 3 ? C_CYAN : C_SKY);
        gfx_hline(gx - k, gx + k, gy + 10 - k, k % 3 ? C_BLUE : C_SKY);
    }
    spr_draw(&hs_spr[HS_WICK_THROW], 224, 116, 0);
    int reach = (frame_t / 2) % 20;
    reach = reach < 10 ? reach : 20 - reach;
    gfx_line(236, 125, 240 + reach * 2, 124, C_WHITE);
    gfx_circ(240 + reach * 2, 124, 2, C_RED);
    static const uint8_t grad[] = {C_CREAM, C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center("HOMESPUN", 160, 18, 3, grad, 4, C_INK, C_WINE);
    text_center("A LONG WAY FROM HOME, ON A STRING", 160, 48, C_LIGHT);
    int y = 76;
    if (confirm_new) {
        text_center("START OVER? THIS CAMP IS LOST.", 160, y, C_YELLOW);
        text_draw("YES", 126, y + 14, confirm_new == 1 ? C_WHITE : C_SLATE);
        text_draw("NO", 182, y + 14, confirm_new == 2 ? C_WHITE : C_SLATE);
        ui_cursor(confirm_new == 1 ? 118 : 174, y + 14, frame_t);
        return;
    }
    const char *rows[2];
    int n = 0;
    if (sv.started) rows[n++] = "CONTINUE";
    rows[n++] = "NEW GAME";
    for (int i = 0; i < n; i++) {
        text_center(rows[i], 160, y + i * 12, i == title_sel ? C_WHITE : C_GREY);
        if (i == title_sel) ui_cursor(160 - text_width(rows[i]) / 2 - 10, y + i * 12, frame_t);
    }
    if (sv.started) {
        char b[80];
        snprintf(b, sizeof b, "TRIPS %u   ESCAPES %d   LOOP %d", (unsigned)sv.trips, sv.escapes, sv.loop + 1);
        tiny_center(b, 160, 168, C_GREY);
    } else {
        tiny_center("1988 BEAMDOWN SOFTWORKS", 160, 168, C_GREY);
    }
}

static void draw_launch(void) {
    int t = state_t;
    gfx_cls(C_NIGHT);
    for (int i = 0; i < 80; i++) {
        int x = (i * 97 + 13) % SCREEN_W, y = ((i * 53 + 5) + t / (1 + i % 3)) % SCREEN_H;
        gfx_pset(x, y, i % 4 ? C_SLATE : C_WHITE);
    }
    int ground = 150 + imin(t, 200) / 2;
    gfx_rect(0, ground, SCREEN_W, SCREEN_H, C_WINE);
    int sy = 120 - (t > 60 ? (t - 60) * (t - 60) / 60 : 0);
    int sx = 150;
    if (sy > -40) {
        gfx_rect(sx - 20, sy - 8, 40, 16, C_LIGHT);
        gfx_rect(sx - 26, sy, 52, 6, C_GREY);
        gfx_rect(sx - 6, sy - 14, 12, 8, C_SKY);
        if (t > 50) {
            int fl = 6 + (t / 3) % 5;
            gfx_rect(sx - 4, sy + 8, 8, fl, (t / 2) % 2 ? C_YELLOW : C_ORANGE);
        }
        if (launch_idol) gfx_rect(sx + 14, sy - 12, 5, 5, C_YELLOW);
    }
    if (t > 240) {
        const char *s = launch_idol
                            ? "THE TUMBLEWEED CLIMBS PAST THE MOONS. ON THE DASH, THE GRINNING IDOL GRINS WIDER. "
                              "SOMEWHERE BELOW, SIX TIMES BEATEN, MAWBO SULKS. WICK IS GOING HOME, AND SHE IS BRINGING A STORY."
                            : "THE TUMBLEWEED CLIMBS PAST THE MOONS, PATCHED AND PROUD. WICK WAVES AT A CAVE SHE WILL NEVER FORGET. "
                              "THE PARCELS WILL BE LATE. THEY WILL BE DELIVERED.";
        ui_panel(16, 18, 288, 62, C_NIGHT, C_CREAM);
        text_wrap(s, 24, 25, 272, C_WHITE, 10);
        if (t > 420) tiny_center(GLYPH_A " ON", 160, 170, C_GREY);
    }
}

static void draw_credits(void) {
    gfx_cls(C_INK);
    static const char *const L[] = {
        "HOMESPUN", "", "WICK, COURIER PILOT", "OLD BURL, A JOLLY TREE", "GRISTLE THE SMOKER", "DR. ORRERY",
        "TOLLY AT THE GATE", "KIT, HUSH, TANGER", "MADAME SHUFFLE", "THE NOODLERS", "MOTHER LOOM", "", "AND MAWBO",
        "", "1988 BEAMDOWN SOFTWORKS", "", "THE STANDING STONES REMEMBER.", "A NEW CAMP WAITS ON THE SAME MOON.",
    };
    int y0 = 180 - state_t / 2;
    for (int i = 0; i < ARRAY_LEN(L); i++) {
        int y = y0 + i * 12;
        if (y < -10 || y > 180) continue;
        text_center(L[i], 160, y, i == 0 ? C_YELLOW : C_LIGHT);
    }
}

static void draw_summary(void) {
    if (state == S_FADED) {
        ui_panel(40, 50, 240, 70, C_NIGHT, C_RED);
        text_center("TIME RAN OUT", 160, 58, C_RED);
        text_center("TOLLY DRAGS WICK HOME BY THE BOOTS.", 160, 76, C_WHITE);
        text_center("EVERYTHING FROM THE TRIP IS GONE.", 160, 88, C_LIGHT);
        if (state_t > 40) tiny_center(GLYPH_A " ON", 160, 108, C_GREY);
        return;
    }
    ui_panel(40, 40, 240, 92, C_NIGHT, C_LIME);
    text_center("HOME SAFE", 160, 48, C_LIME);
    char b[80];
    snprintf(b, sizeof b, "+%d GLINTS  +%d BARS  +%d JERKY", home_gain[RES_GLINT], home_gain[RES_BAR], home_gain[RES_JERKY]);
    text_center(b, 160, 64, C_WHITE);
    snprintf(b, sizeof b, "+%d DATA  +%d THREAD  %d ODDS KEPT", home_gain[RES_DATA], home_gain[RES_THREAD], home_odds);
    text_center(b, 160, 76, C_WHITE);
    int y = 90;
    if (home_carry & 7) { text_center("A SHIP PART FOR THE TUMBLEWEED!", 160, y, C_YELLOW); y += 11; }
    if (home_carry & 8) { text_center("THE THINKER HAS ITS GEAR!", 160, y, C_YELLOW); y += 11; }
    if (home_carry & 16) { text_center("THE GRINNING IDOL COMES ABOARD!", 160, y, C_YELLOW); y += 11; }
    if (state_t > 40) tiny_center(GLYPH_A " ON", 160, 124, C_GREY);
}

static void hs_draw(void) {
    switch (state) {
    case S_TITLE: draw_title(); return;
    case S_INTRO:
        gfx_cls(C_NIGHT);
        draw_box_text(INTRO[iclamp(intro_page, 0, 2)], 60);
        if (state_t > 20) tiny_center(GLYPH_A " ON", 160, 110, C_GREY);
        return;
    case S_LAUNCH: draw_launch(); return;
    case S_CREDITS: draw_credits(); return;
    default: break;
    }
    if (hs.area == AR_BASE) hs_draw_base(frame_t);
    else hs_draw_room();
    hs_draw_hud();
    if (hm.open) hs_draw_menu();
    if (gam_state) draw_gamble();
    if (talk_n > 0) {
        draw_box_text(talk_page[talk_i], 132);
        if ((frame_t / 20) % 2) text_draw(GLYPH_A, 294, 164, C_YELLOW);
    } else if (hs.msg_t > 0 && !hm.open) {
        int w = text_width(hs.msg) + 12;
        if (w > 300) w = 300;
        ui_panel(160 - w / 2, 150, w, 14, C_INK, C_AMBER);
        text_center(hs.msg, 160, 153, C_CREAM);
    }
    if (state == S_FADED || state == S_HOME) draw_summary();
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                   */

static void hs_load(void) {
    hs_art_load();
    hs_audio_load();
    rt_loaded = false; /* the save is read on the first real-time tick */
}

static void hs_start(void) {
    if (!rt_loaded) read_save();
    else if (had_file && game_save_raw_size(HS_SLOT) == 0) read_save(); /* deleted meanwhile */
    memset(&hs, 0, sizeof hs);
    hs.boss = -1;
    hs_sim_seed(rng_next(&g_rng));
    god = false;
    talk_n = 0;
    gam_state = 0;
    hm.open = 0;
    hs_bot_reset();
    check_goals();
    to_title();
}

static void hs_quit(void) {
    if (sv.started) save_now();
    game_pause_items(0, NULL, NULL);
}


static int hs_query(const char *key, int *out) {
    static const char *const RES[RES_COUNT] = {"glints", "bars", "jerky", "data", "thread"};
    for (int r = 0; r < RES_COUNT; r++) {
        if (!strcmp(key, RES[r])) { *out = sv.res[r]; return 1; }
        char k[32];
        snprintf(k, sizeof k, "gain.%s", RES[r]);
        if (!strcmp(key, k)) { *out = sv.gain[r]; return 1; }
        snprintf(k, sizeof k, "cap.%s", RES[r]);
        if (!strcmp(key, k)) { *out = hs_cap(r); return 1; }
        snprintf(k, sizeof k, "total.%s", RES[r]);
        if (!strcmp(key, k)) { *out = hs_total(r); return 1; }
    }
    if (!strcmp(key, "bot")) { *out = hs_bot_buttons(); return 1; }
    if (!strcmp(key, "bot_idle")) { *out = hs_bot_idle; return 1; }
    { int k; if (sscanf(key, "hurtby%d", &k) == 1) { *out = hs_hurt_by[k & 15]; return 1; } }
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "area")) { *out = hs.area; return 1; }
    if (!strcmp(key, "room")) { *out = hs.room; return 1; }
    if (!strcmp(key, "time")) { *out = sv.time_f / 60; return 1; }
    if (!strcmp(key, "time_f")) { *out = sv.time_f; return 1; }
    if (!strcmp(key, "on_trip")) { *out = sv.on_trip; return 1; }
    if (!strcmp(key, "odds")) { *out = sv.odds; return 1; }
    if (!strcmp(key, "plants")) { *out = sv.plants; return 1; }
    if (!strcmp(key, "smoke")) { *out = sv.smoke_built; return 1; }
    if (!strcmp(key, "smoke_stock")) { *out = sv.smoke_stock; return 1; }
    if (!strcmp(key, "hands")) { *out = hs_hands(); return 1; }
    if (!strcmp(key, "free_hands")) { *out = hs_free_hands(); return 1; }
    if (!strcmp(key, "anvils")) { *out = sv.anvils; return 1; }
    if (!strcmp(key, "anvil_hands")) { *out = hs_anvil_hands(); return 1; }
    if (!strcmp(key, "lab")) { *out = sv.lab_fixed; return 1; }
    if (!strcmp(key, "lab_hands")) { *out = sv.lab_hands; return 1; }
    if (!strcmp(key, "bins")) { *out = hs_bin_level(); return 1; }
    if (!strcmp(key, "research")) { *out = sv.research; return 1; }
    if (!strcmp(key, "parts")) { *out = sv.parts; return 1; }
    if (!strcmp(key, "carry")) { *out = sv.carry; return 1; }
    if (!strcmp(key, "loom")) { *out = sv.loom_home; return 1; }
    if (!strcmp(key, "pipe")) { *out = sv.pipe; return 1; }
    if (!strcmp(key, "letter")) { *out = sv.letter; return 1; }
    if (!strcmp(key, "weapon")) { *out = sv.weapon; return 1; }
    if (!strcmp(key, "idol")) { *out = sv.idol; return 1; }
    if (!strcmp(key, "escapes")) { *out = sv.escapes; return 1; }
    if (!strcmp(key, "idol_escapes")) { *out = sv.idol_escapes; return 1; }
    if (!strcmp(key, "loop")) { *out = sv.loop; return 1; }
    if (!strcmp(key, "marks")) { *out = sv.marks - sv.spent_marks; return 1; }
    if (!strcmp(key, "stone0")) { *out = sv.stone[0]; return 1; }
    if (!strcmp(key, "stone1")) { *out = sv.stone[1]; return 1; }
    if (!strcmp(key, "stone2")) { *out = sv.stone[2]; return 1; }
    if (!strcmp(key, "mawbo_wins")) { *out = sv.mawbo_wins; return 1; }
    if (!strcmp(key, "mawbo_room")) { *out = sv.mawbo_room; return 1; }
    if (!strcmp(key, "mawbo_here")) { *out = hs_mawbo_here(); return 1; }
    if (!strcmp(key, "mawbo_hp")) { *out = sv.mawbo_hp; return 1; }
    if (!strcmp(key, "volt_down")) { *out = sv.volt_down; return 1; }
    if (!strcmp(key, "trips")) { *out = (int)sv.trips; return 1; }
    if (!strcmp(key, "fadeouts")) { *out = (int)sv.fadeouts; return 1; }
    if (!strcmp(key, "hits")) { *out = (int)sv.hits; return 1; }
    if (!strcmp(key, "last_hit")) { *out = hs.last_hit_s; return 1; }
    if (!strcmp(key, "pads")) { *out = sv.pads; return 1; }
    if (!strcmp(key, "alive_s")) { *out = (int)sv.alive_s; return 1; }
    if (!strcmp(key, "excursion")) { *out = (int)sv.excursion; return 1; }
    if (!strcmp(key, "started")) { *out = sv.started; return 1; }
    if (!strcmp(key, "seed")) { *out = (int)(sv.seed & 0x7FFFFFFF); return 1; }
    if (!strcmp(key, "menu")) { *out = hm.open; return 1; }
    if (!strcmp(key, "menu_n")) { *out = hm.n; return 1; }
    if (!strcmp(key, "menu_sel")) { *out = hm.sel; return 1; }
    if (!strcmp(key, "talk")) { *out = talk_n > 0; return 1; }
    if (!strcmp(key, "gamble")) { *out = gam_state; return 1; }
    if (!strcmp(key, "gam_bars")) { int s = 0; for (int i = 0; i < 6; i++) s += gam_prize[i] > 0 ? gam_prize[i] : 0; *out = s; return 1; }
    if (!strcmp(key, "gam_meat")) { int s = 0; for (int i = 0; i < 6; i++) s += gam_prize[i] < 0; *out = s; return 1; }
    if (!strcmp(key, "px")) { *out = hs.px / HS_U; return 1; }
    if (!strcmp(key, "py")) { *out = hs.py / HS_U; return 1; }
    if (!strcmp(key, "yoyo")) { *out = hs.yo_t; return 1; }
    if (!strcmp(key, "boss")) { *out = hs.boss >= 0 ? hs.mob[hs.boss].kind : -1; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = hs.boss >= 0 ? hs.mob[hs.boss].hp : 0; return 1; }
    if (!strcmp(key, "boss_x")) { *out = hs.boss >= 0 ? hs.mob[hs.boss].x / HS_U : -1; return 1; }
    if (!strcmp(key, "boss_y")) { *out = hs.boss >= 0 ? hs.mob[hs.boss].y / HS_U : -1; return 1; }
    if (!strcmp(key, "boss_st")) { *out = hs.boss >= 0 ? hs.mob[hs.boss].st * 1000 + hs.mob[hs.boss].t : -1; return 1; }
    if (!strcmp(key, "shut")) { *out = hs.shut; return 1; }
    if (!strcmp(key, "shot_seen")) { *out = (int)hs.shot_seen; return 1; }
    if (!strcmp(key, "goal")) { *out = hs_bot_goal; return 1; }
    if (!strcmp(key, "art_bad")) { *out = hs_art_bad; return 1; }
    if (!strcmp(key, "dun_check")) { *out = hs_dun_check(); return 1; }
    if (!strcmp(key, "items")) {
        int n = 0;
        for (int i = 0; i < HS_MAX_ITEMS; i++) n += hs.item[i].on;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "mobs")) {
        int n = 0;
        for (int i = 0; i < HS_MAX_MOBS; i++) n += hs.mob[i].on;
        *out = n;
        return 1;
    }
    int a;
    if (sscanf(key, "mob%d", &a) == 1) {
        *out = -1;
        for (int i = 0; i < HS_MAX_MOBS; i++)
            if (hs.mob[i].on && a-- == 0) { *out = hs.mob[i].kind; break; }
        return 1;
    }
    if (sscanf(key, "mobhp%d", &a) == 1) { *out = a >= 0 && a < HS_MAX_MOBS ? hs.mob[a].hp : 0; return 1; }
    if (sscanf(key, "world_check%d", &a) == 1) {
        /* the first N seeds from 1 */
        int bad = 0;
        for (int s = 1; s <= a && !bad; s++) {
            int r = hs_world_check((uint32_t)s * 2654435761u);
            if (r) bad = s * 1000 + r;
        }
        hs_world_make(sv.seed);
        *out = bad;
        return 1;
    }
    if (sscanf(key, "variety%d", &a) == 1) { *out = hs_world_variety(a); return 1; }
    if (sscanf(key, "cave_screen%d", &a) == 1) { *out = hw.cave_screen[iclamp(a, 0, HS_CAVES - 1)]; return 1; }
    if (sscanf(key, "pad_screen%d", &a) == 1) { *out = hw.pad_screen[iclamp(a, 0, HS_PADS - 1)]; return 1; }
    if (!strcmp(key, "pad_here")) { *out = hs.room >= 0 && hs.room < HS_SCREENS ? hw.pad_of[hs.room] : -1; return 1; }
    if (!strcmp(key, "cave_here")) { *out = hs.area == AR_CAVE ? hw.cave_kind[hs.room - HS_R_CAVE0] : hs.area == AR_DUN ? hs_dun_of(hs.room) : -1; return 1; }
    if (!strcmp(key, "ruins_here")) { *out = hs.room == hw.ruins ? 1 : hs.room == hw.lair ? 2 : 0; return 1; }
    if (!strcmp(key, "gate_shut")) {
        /* in the ruins: is the way to the lair barred */
        int d = hw.lair_dir, x = d == D_LEFT ? 0 : d == D_RIGHT ? HS_TW - 1 : 9, y = d == D_UP ? 0 : 4;
        *out = hs.room == hw.ruins && hs.tile[y][x] == T_SHUT;
        return 1;
    }
    if (!strcmp(key, "ruins")) { *out = hw.ruins; return 1; }
    if (!strcmp(key, "lair")) { *out = hw.lair; return 1; }
    if (!strcmp(key, "exits")) { *out = hs.room < HS_SCREENS ? hs_exits(hs.room) : 0; return 1; }
    return 0;
}

static int hs_cheat(const char *cmd) {
    int a, b, c;
    char name[24];
    if (!strcmp(cmd, "title")) { to_title(); return 1; }
    if (!strcmp(cmd, "play")) { to_play(); return 1; }
    if (sscanf(cmd, "newgame %d", &a) == 1) {
        fresh_save((uint32_t)a);
        sv.started = 1;
        sv.intro_seen = 1;
        save_now();
        to_play();
        return 1;
    }
    if (!strcmp(cmd, "god")) { god = !god; return 1; }
    if (sscanf(cmd, "research %d", &a) == 1) { sv.research = (uint8_t)a; return 1; }
    if (sscanf(cmd, "res %23s %d", name, &a) == 2) {
        static const char *const RES[RES_COUNT] = {"glints", "bars", "jerky", "data", "thread"};
        for (int r = 0; r < RES_COUNT; r++)
            if (!strcmp(name, RES[r])) { sv.res[r] = a; return 1; }
        return 0;
    }
    if (sscanf(cmd, "odds %d", &a) == 1) { sv.odds = (uint16_t)a; return 1; }
    if (sscanf(cmd, "plants %d", &a) == 1) { sv.plants = (uint8_t)iclamp(a, 0, HS_PLANTS); return 1; }
    if (sscanf(cmd, "research %d", &a) == 1) { sv.research = (uint8_t)a; return 1; }
    if (sscanf(cmd, "huts %d %d", &a, &b) == 2) { sv.huts[0] = (uint8_t)a; sv.huts[1] = (uint8_t)b; return 1; }
    if (sscanf(cmd, "bins %d %d", &a, &b) == 2) { sv.bins[0] = (uint8_t)a; sv.bins[1] = (uint8_t)b; return 1; }
    if (sscanf(cmd, "anvils %d %d", &a, &b) == 2) { sv.anvils = (uint8_t)a; sv.anvil_hand = (uint8_t)b; return 1; }
    if (sscanf(cmd, "lab %d %d", &a, &b) == 2) { sv.lab_fixed = (uint8_t)a; sv.lab_hands = (uint8_t)b; return 1; }
    if (sscanf(cmd, "smoke %d %d", &a, &b) == 2) { sv.smoke_built = (uint8_t)a; sv.smoke_stock = (uint8_t)b; return 1; }
    if (sscanf(cmd, "parts %d", &a) == 1) { sv.parts = (uint8_t)a; return 1; }
    if (sscanf(cmd, "carry %d", &a) == 1) { sv.carry = (uint8_t)a; return 1; }
    if (sscanf(cmd, "loom %d", &a) == 1) { sv.loom_home = (uint8_t)a; return 1; }
    if (sscanf(cmd, "pipe %d", &a) == 1) { sv.pipe = (uint8_t)a; set_pause_items(); return 1; }
    if (sscanf(cmd, "letter %d", &a) == 1) { sv.letter = (uint8_t)a; return 1; }
    if (sscanf(cmd, "idol %d", &a) == 1) { sv.idol = (uint8_t)a; return 1; }
    if (sscanf(cmd, "weapon %d", &a) == 1) { sv.weapon = (uint8_t)a; set_pause_items(); return 1; }
    if (sscanf(cmd, "marks %d", &a) == 1) { sv.marks = (uint8_t)a; sv.spent_marks = 0; return 1; }
    if (sscanf(cmd, "stones %d %d %d", &a, &b, &c) == 3) { sv.stone[0] = (uint8_t)a; sv.stone[1] = (uint8_t)b; sv.stone[2] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "pads %d", &a) == 1) { sv.pads = (uint8_t)a; return 1; }
    if (sscanf(cmd, "time %d", &a) == 1) { sv.time_f = a * 60; return 1; }
    if (sscanf(cmd, "mawbo %d %d", &a, &b) == 2) { sv.mawbo_room = (uint8_t)a; sv.mawbo_wins = (uint8_t)b; return 1; }
    if (sscanf(cmd, "trip %d", &a) == 1) {
        /* a trip with this much jerky, from the screen above camp */
        sv.res[RES_JERKY] = a;
        state = S_PLAY;
        game_set_pausable(true);
        begin_trip_at(HS_START_SCREEN, 160, HS_FH - 10);
        return 1;
    }
    if (sscanf(cmd, "room %d %d %d", &a, &b, &c) == 3) { hs_sim_enter(a, b * HS_U, c * HS_U); area_music(); return 1; }
    if (sscanf(cmd, "gopad %d", &a) == 1 && a >= 1 && a < HS_PADS) { pad_arrive(a); return 1; }
    if (!strcmp(cmd, "gomawbo")) {
        hs_sim_enter(sv.mawbo_room, 60 * HS_U, 88 * HS_U);
        /* somewhere Wick can stand, left of the middle */
        for (int x = 40; x < 150; x += 4)
            if (hs_box_free(x, 88, 5)) { hs.px = x * HS_U; break; }
        area_music();
        return 1;
    }
    if (!strcmp(cmd, "goruins")) { hs_sim_enter(hw.ruins, 160 * HS_U, 110 * HS_U); area_music(); return 1; }
    if (!strcmp(cmd, "golair")) { hs_sim_enter(hw.lair, 160 * HS_U, 110 * HS_U); area_music(); return 1; }
    if (sscanf(cmd, "goscreen %d", &a) == 1 && a >= 0 && a < HS_SCREENS) { hs_sim_enter(a, 160 * HS_U, 88 * HS_U); area_music(); return 1; }
    if (sscanf(cmd, "govendor %d", &a) == 1) {
        /* in front of the noodler selling ware a (below it) */
        for (int r = HS_R_DUN0; r < HS_R_BASE; r++) {
            HsThingSpawn ts[HS_MAX_THINGS];
            int n = hs_room_things(r, ts, HS_MAX_THINGS);
            for (int i = 0; i < n; i++)
                if (ts[i].kind == TH_VENDOR && ts[i].arg == a) {
                    hs_sim_enter(r, (ts[i].tx * HS_T + 8) * HS_U, (ts[i].ty * HS_T + 8 + 16) * HS_U);
                    hs.dirx = 0;
                    hs.diry = -1;
                    return 1;
                }
        }
        return 0;
    }
    if (sscanf(cmd, "gocave %d", &a) == 1 && a >= 0 && a < HS_CAVES) {
        /* just below a cave's mouth, on its screen */
        int s = hw.cave_screen[a];
        hs_sim_enter(s, (hw.cave_x[s] * HS_T + 8) * HS_U, (HS_T + 14) * HS_U);
        return 1;
    }
    if (sscanf(cmd, "at %d %d", &a, &b) == 2) { hs.px = a * HS_U; hs.py = b * HS_U; return 1; }
    if (sscanf(cmd, "face %d %d", &a, &b) == 2) { hs.dirx = a; hs.diry = b; return 1; }
    if (sscanf(cmd, "spawn %d %d %d", &a, &b, &c) == 3) { hs_add_mob(a, b, c, 0xFF); return 1; }
    if (!strcmp(cmd, "killall")) { hs_kill_all(); return 1; }
    if (sscanf(cmd, "botcherry %d", &a) == 1) { hs_bot_cherry = a != 0; return 1; }
    if (sscanf(cmd, "botarena %d", &a) == 1) { hs_bot_arena = a != 0; return 1; }
    if (sscanf(cmd, "acc %d", &a) == 1) { sv.acc_plant = sv.acc_smoke = sv.acc_anvil = sv.acc_lab = sv.acc_loom = (uint32_t)a; return 1; }
    if (!strcmp(cmd, "clearmobs")) { for (int i = 0; i < HS_MAX_MOBS; i++) hs.mob[i].on = 0; hs.boss = -1; return 1; }
    if (!strcmp(cmd, "holdmobs")) { for (int i = 0; i < HS_MAX_MOBS; i++) hs.mob[i].stun = 30000; return 1; }
    if (sscanf(cmd, "bosshp %d", &a) == 1) { if (hs.boss >= 0) hs.mob[hs.boss].hp = (int16_t)a; return 1; }
    if (sscanf(cmd, "drop %d %d", &a, &b) == 2) { hs_drop(a, b, hs.px + 16 * HS_U * hs.dirx, hs.py + 16 * HS_U * hs.diry); return 1; }
    if (!strcmp(cmd, "save")) { save_now(); return 1; }
    if (sscanf(cmd, "produce %d", &a) == 1) { hs_produce((uint32_t)a); return 1; }
    if (!strcmp(cmd, "launch")) {
        hm.open = 0;
        hs_menu_clear("X");
        hs_menu_add("GO", "", M_LAUNCH, 1, true);
        menu_do(0);
        return 1;
    }
    return 0;
}

const GameDef GAME_HOMESPUN = {
    "homespun",
    "HOMESPUN",
    "1988",
    "ADVENTURE",
    "A COURIER PILOT CRASHES ON A MOOR-MOON. BUILD A CAMP THAT WORKS WHILE YOU'RE AWAY, THEN BRAVE THE WILDS ON TWO MINUTES OF JERKY A STRIP.",
    {"TAME MOTHER LOOM", "FLY THE TUMBLEWEED HOME", "BEAT MAWBO SIX TIMES IN ONE TRIP, THEN FLY HOME WITH ITS IDOL"},
    GLYPH_DPAD "\tWALK (EIGHT WAYS)\n"
    GLYPH_B "\tTHROW THE YO-YO\n"
    GLYPH_A "\tTALK, BUILD, BUY, OPEN\n"
    "START\tPAUSE (SWAP TO THE\n"
    "\tTHUNDERPIPE ONCE YOU OWN IT)\n"
    "\tTHE CAMP WORKS WHILE THE\n"
    "\tCONSOLE IS ON, IN ANY GAME",
    C_WINE, C_AMBER,
    hs_load, hs_start, hs_update, hs_draw, hs_quit, hs_draw_label, hs_query, hs_cheat,
    "PILOT QUEST", 44,
    hs_realtime,
};
