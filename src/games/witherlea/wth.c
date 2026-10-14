/* WITHERLEA - you wake in the cold store of a house in the fens, and the
 * Keeper is somewhere in it. Cartridge 42 of UFO 40, a tribute to Night
 * Manor (UFO 50 #42). See docs/games/42-witherlea.md.
 *
 * This file is the screens (the warning, the title, the opening, the house,
 * the bag, the diary, waking, the endings, the credits and the mirror),
 * the cursor, saving (every room and every change, so CONTINUE puts you
 * back where you were), the goals and the test hooks. */
#include "wth.h"

WthState wth;

#define TRANS_LEN 24
#define TRANS_HALF 12
#define CUR_CALM 320   /* 1.25 px a frame */
#define CUR_SCARED 176 /* 0.69 px a frame, and it shakes */

/* ------------------------------------------------------------------ */
/* saving                                                               */

static bool cheat_used; /* tests only: nothing */

void wth_save_now(void) {
    if (!wth.s.started) return;
    wth.s.magic = WTH_MAGIC;
    wth.s.version = WTH_VERSION;
    wth.s.cx = (int16_t)(wth.cx >> 8);
    wth.s.cy = (int16_t)(wth.cy >> 8);
    game_save_write(game_current_index(), &wth.s, (int)sizeof wth.s);
}

static bool load_save(WthSave *out) {
    static WthSave tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == WTH_MAGIC &&
        tmp.version == WTH_VERSION && tmp.started) {
        *out = tmp;
        return true;
    }
    return false;
}

static bool save_exists(void) {
    WthSave t;
    return load_save(&t);
}

void wth_award(int bit) { game_award(bit); }

/* goals come back from the save (RESET GOALS clears the save with them) */
static void rebuild_goals(const WthSave *s) {
    WthState keep = wth;
    wth.s = *s;
    if (wth_flag(F_GIFT)) game_award(GOAL_BEACON);
    if (wth_flag(F_GOLD_DONE)) game_award(GOAL_SAUCER);
    if (wth_flag(F_CHERRY_DONE)) game_award(GOAL_ALIEN);
    wth = keep;
}

static void new_game(void) {
    WthSave old;
    bool had = load_save(&old);
    memset(&wth.s, 0, sizeof wth.s);
    wth.s.started = 1;
    wth.s.room = R_CELL;
    wth.s.interval = WTH_FIRST_WAIT;
    wth.s.spawn_left = WTH_FIRST_WAIT;
    wth.s.body_room = 0xFF;
    /* the goals already earned stay earned (the save keeps the record) */
    if (had) {
        if ((old.flag[F_GIFT >> 3] >> (F_GIFT & 7)) & 1) wth_set(F_GIFT);
        if ((old.flag[F_GOLD_DONE >> 3] >> (F_GOLD_DONE & 7)) & 1) wth_set(F_GOLD_DONE);
        if ((old.flag[F_CHERRY_DONE >> 3] >> (F_CHERRY_DONE & 7)) & 1) wth_set(F_CHERRY_DONE);
        if ((old.flag[F_BEST_DONE >> 3] >> (F_BEST_DONE & 7)) & 1) wth_set(F_BEST_DONE);
        wth.s.golds = old.golds;
        wth.s.cherries = old.cherries;
        wth.s.bests = old.bests;
    }
    wth.cx = 160 << 8;
    wth.cy = 80 << 8;
    wth.held = 0;
    wth_keeper_reset();
    wth.msg_n = wth.msg_page = 0;
    wth_save_now();
}

static void continue_game(void) {
    load_save(&wth.s);
    wth.cx = iclamp(wth.s.cx, WTH_PX, WTH_PX + WTH_PW - 1) << 8;
    wth.cy = iclamp(wth.s.cy, WTH_PY, WTH_PY + WTH_PH - 1) << 8;
    wth.held = 0;
    wth_keeper_reset();
    wth.msg_n = wth.msg_page = 0;
    wth.trans_t = 0;
}

/* ------------------------------------------------------------------ */
/* scenes and music                                                     */

void wth_room_music(void) {
    if (wth.scene != SC_PLAY && wth.scene != SC_BAG && wth.scene != SC_DIARY) return;
    int k = wth.k.state;
    if (k == E_HERE || k == E_FOLLOW || k == E_STUN) { music_play(WTH_MUS_CHASE); return; }
    if (k == E_HIDING) { music_play(WTH_MUS_HIDE); return; }
    int r = wth.s.room;
    int m;
    if (r == R_CELLAR || r == R_SPORE) m = WTH_MUS_CELLAR;
    else if (r == R_MAZE || r == R_HEART) m = WTH_MUS_MAZE;
    else if (wth_room_outside(r)) m = WTH_MUS_GARDEN;
    else if (wth_room_dark(r)) m = WTH_MUS_UPSTAIRS;
    else m = WTH_MUS_HOUSE;
    music_play(m);
}

void wth_goto(int scene) {
    int was = wth.scene;
    wth.scene = scene;
    wth.scene_t = 0;
    input_consume();
    game_set_pausable(scene != SC_WARNING && scene != SC_TITLE);
    switch (scene) {
    case SC_WARNING: music_stop(); break;
    case SC_TITLE: music_play(WTH_MUS_TITLE); break;
    case SC_INTRO: music_play(WTH_MUS_TITLE); break;
    case SC_PLAY:
        if (was != SC_BAG && was != SC_DIARY) wth_room_music();
        break;
    case SC_WAKE: music_stop(); break;
    case SC_ENDING: music_play(WTH_MUS_ENDING); break;
    case SC_CREDITS: music_play(wth.ending_best ? WTH_MUS_DAWN : WTH_MUS_ENDING); break;
    case SC_STINGER: music_play(WTH_MUS_STING); break;
    default: break;
    }
}

void wth_end_game(int ending) {
    wth.ending = ending;
    wth.ending_caught = wth_flag(F_EVER_CAUGHT);
    wth.ending_best = ending == END_CELLAR && !wth_flag(F_EVER_CAUGHT);
    if (ending == END_CAR) {
        wth_set(F_GOLD_DONE);
        if (wth.s.golds < 65535) wth.s.golds++;
        wth_award(GOAL_SAUCER);
    } else {
        wth_set(F_CHERRY_DONE);
        if (wth.s.cherries < 65535) wth.s.cherries++;
        wth_award(GOAL_ALIEN);
        if (wth.ending_best) {
            wth_set(F_BEST_DONE);
            if (wth.s.bests < 65535) wth.s.bests++;
        }
    }
    wth.held = 0;
    wth.msg_n = wth.msg_page = 0;
    wth_keeper_reset();
    wth_save_now();
    wth_goto(SC_ENDING);
}

/* ------------------------------------------------------------------ */
/* the house                                                            */

static int point_x(void) { return (int)(wth.cx >> 8) + wth.shake_x; }
static int point_y(void) { return (int)(wth.cy >> 8) + wth.shake_y; }

static int floor_at(int x, int y) {
    for (int i = 0; i < WTH_FLOOR; i++) {
        if (!wth.s.floor[wth.s.room][i]) continue;
        int fx = 70 + i * 60, fy = 110;
        if (x >= fx - 2 && x < fx + 16 && y >= fy - 2 && y < fy + 16) return i;
    }
    return -1;
}

static void move_cursor(void) {
    int dx = (btn(BTN_RIGHT) ? 1 : 0) - (btn(BTN_LEFT) ? 1 : 0);
    int dy = (btn(BTN_DOWN) ? 1 : 0) - (btn(BTN_UP) ? 1 : 0);
    bool scared = wth_scared();
    int sp = scared ? CUR_SCARED : CUR_CALM;
    wth.cx += dx * sp;
    wth.cy += dy * sp;
    if (scared) {
        /* trembling: the hand jerks a pixel now and then, and the arrow shakes */
        if ((wth.frame % 8) == 0) {
            wth.cx += wth_rand(-1, 1) * 256;
            wth.cy += wth_rand(-1, 1) * 256;
        }
        if ((wth.frame % 3) == 0) {
            wth.shake_x = wth_rand(-1, 1);
            wth.shake_y = wth_rand(-1, 1);
        }
    } else wth.shake_x = wth.shake_y = 0;
    wth.cx = iclamp((int)wth.cx, WTH_PX << 8, (WTH_PX + WTH_PW - 1) << 8);
    wth.cy = iclamp((int)wth.cy, WTH_PY << 8, (WTH_PY + WTH_PH - 1) << 8);
}

static void click_exit(int s) {
    const WthSpot *sp = &WTH_SPOT[s];
    if (wth.s.room == R_MAZE && sp->dir != DIR_DOWN) {
        if (sp->dir == WTH_MAZE_WAY[wth.s.maze]) {
            wth.s.maze++;
            if (wth.s.maze >= WTH_MAZE_STEPS) {
                wth.s.maze = 0;
                wth_start_exit(R_HEART);
            } else wth_start_exit(R_MAZE);
        } else {
            wth.s.maze = 0;
            wth_start_exit(R_MAZE);
            wth_noise(true);
            sfx_play_name("wth_bell");
            wth_say("A LITTLE BELL TINKLES ON A WIRE STRUNG THROUGH THE HEDGE. THE PATHS ALL LOOK THE SAME, AND YOU "
                    "COME OUT WHERE YOU STARTED.");
        }
        return;
    }
    int d = wth_exit_dest(s);
    if (d < 0) {
        if (s == S_DRV_CELLAR) wth_say("STEPS GO DOWN INTO PITCH DARK. NOT WITHOUT A LIGHT.");
        return;
    }
    if (d == R_MAZE || d == R_COURTYARD) wth.s.maze = 0;
    wth_start_exit(d);
}

static void press_a(void) {
    int x = point_x(), y = point_y();
    if (wth_keeper_visible() && wth.k.state != E_CAUGHT) {
        int kx, ky, kw, kh;
        wth_keeper_rect(&kx, &ky, &kw, &kh);
        if (x >= kx && y >= ky && x < kx + kw && y < ky + kh) {
            wth_keeper_click(wth.held);
            return;
        }
    }
    int f = floor_at(x, y);
    if (f >= 0) {
        int it = wth.s.floor[wth.s.room][f];
        wth.s.floor[wth.s.room][f] = 0;
        wth_give(it);
        wth_sayf("YOU PICK UP THE %s.", WTH_ITEM[it].name);
        wth_save_now();
        return;
    }
    int s = wth_spot_at(x, y);
    if (s < 0) return;
    if (WTH_SPOT[s].kind == HS_EXIT) click_exit(s);
    else wth_click(s, wth.held);
    if (wth.scene == SC_PLAY) wth_save_now();
}

/* the Keeper keeps coming whatever screen you have open */
static void house_clock(void) {
    bool trans = wth.trans_t > 0;
    if (wth.trans_t > 0) {
        wth.trans_t++;
        if (wth.trans_t == TRANS_HALF) wth_goto_room(wth.trans_to);
        if (wth.trans_t >= TRANS_LEN) wth.trans_t = 0;
    }
    wth_keeper_step(trans);
    if (wth.s.frames < 0xFFFFFFF0u) wth.s.frames++;
    int k = wth.k.state;
    if ((k == E_CAUGHT || k == E_HIDING) && (wth.scene == SC_BAG || wth.scene == SC_DIARY)) {
        wth.scene = SC_PLAY;
        wth.scene_t = 0;
    }
}

static void update_play(void) {
    house_clock();
    if (wth.scene != SC_PLAY) return;
    if (wth.k.state == E_HIDING) {
        wth_mini_step(btnp(BTN_A));
        return;
    }
    if (wth.k.state == E_CAUGHT || wth.trans_t > 0) return;
    if (wth_talking()) {
        if (btnp(BTN_A)) {
            wth.msg_page++;
            if (wth.msg_page >= wth.msg_n) wth.msg_n = wth.msg_page = 0;
        }
        return;
    }
    move_cursor();
    if (btnp(BTN_B)) {
        wth.bag_sel = wth.held ? imax(0, wth_slot_of(wth.held)) : 0;
        sfx_play_name("wth_bag");
        wth.scene = SC_BAG;
        wth.scene_t = 0;
        return;
    }
    if (btnp(BTN_A)) press_a();
}

static void update_bag(void) {
    house_clock();
    if (wth.scene != SC_BAG) return;
    if (wth_talking()) {
        if (btnp(BTN_A)) {
            wth.msg_page++;
            if (wth.msg_page >= wth.msg_n) wth.msg_n = wth.msg_page = 0;
        }
        return;
    }
    int c = wth.bag_sel % 10, r = wth.bag_sel / 10;
    if (btn_repeat(BTN_LEFT)) { c = (c + 9) % 10; sfx_play_name("wth_tick"); }
    if (btn_repeat(BTN_RIGHT)) { c = (c + 1) % 10; sfx_play_name("wth_tick"); }
    if (btn_repeat(BTN_UP)) { r = (r + 2) % 3; sfx_play_name("wth_tick"); }
    if (btn_repeat(BTN_DOWN)) { r = (r + 1) % 3; sfx_play_name("wth_tick"); }
    wth.bag_sel = r * 10 + c;
    if (btnp(BTN_B)) {
        sfx_play_name("wth_bag");
        wth.scene = SC_PLAY;
        wth.scene_t = 0;
        return;
    }
    if (btnp(BTN_A)) {
        int it = wth.s.bag[wth.bag_sel];
        if (!wth.held) {
            if (it == I_DIARY) {
                wth.diary_sel = 0;
                while (wth.diary_sel < WTH_PAGES && !((wth.s.pages >> wth.diary_sel) & 1)) wth.diary_sel++;
                wth.scene = SC_DIARY;
                wth.scene_t = 0;
                sfx_play_name("wth_page");
            } else if (it) {
                wth.held = it;
                wth.held_slot = wth.bag_sel;
                sfx_play_name("wth_tick");
                wth.scene = SC_PLAY;
                wth.scene_t = 0;
            }
        } else if (it == wth.held) {
            wth.held = 0; /* back in the bag */
            sfx_play_name("wth_tick");
        } else if (it) {
            int a = wth.held;
            wth.held = 0;
            wth_combine(a, it);
            wth_save_now();
        }
    }
}

static void update_diary(void) {
    house_clock();
    if (wth.scene != SC_DIARY) return;
    if (btn_repeat(BTN_LEFT)) {
        for (int i = wth.diary_sel - 1; i >= 0; i--)
            if ((wth.s.pages >> i) & 1) { wth.diary_sel = i; sfx_play_name("wth_page"); break; }
    }
    if (btn_repeat(BTN_RIGHT)) {
        for (int i = wth.diary_sel + 1; i < WTH_PAGES; i++)
            if ((wth.s.pages >> i) & 1) { wth.diary_sel = i; sfx_play_name("wth_page"); break; }
    }
    if (btnp(BTN_A) || btnp(BTN_B)) {
        wth.scene = SC_BAG;
        wth.scene_t = 0;
    }
}

/* ------------------------------------------------------------------ */
/* the other screens                                                    */

#define INTRO_PAGES 3
#define END_PAGES 3

static void update_title(void) {
    bool has = save_exists();
    int n = has ? 2 : 1;
    if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) {
        wth.title_sel = (wth.title_sel + 1) % n;
        wth.title_erase = 0;
        sfx_play_name("wth_tick");
    }
    if (wth.title_sel >= n) wth.title_sel = 0;
    if (btnp(BTN_B) && wth.scene_t > 10) { game_exit_to_library(); return; }
    if (btnp(BTN_A) && wth.scene_t > 15) {
        bool cont = has && wth.title_sel == 0;
        if (cont) {
            continue_game();
            sfx_play_name("ui_ok");
            wth_goto(SC_PLAY);
        } else if (has && !wth.title_erase) {
            wth.title_erase = 1;
            sfx_play_name("wth_tick");
        } else {
            new_game();
            sfx_play_name("ui_ok");
            wth.diary_sel = 0; /* the opening's page */
            wth_goto(SC_INTRO);
        }
    }
}

static void update_screens(void) {
    switch (wth.scene) {
    case SC_WARNING:
        if (btnp(BTN_A) && wth.scene_t > 30) wth_goto(SC_TITLE);
        break;
    case SC_TITLE: update_title(); break;
    case SC_INTRO:
        if (btnp(BTN_A) && wth.scene_t > 20) {
            wth.diary_sel++;
            wth.scene_t = 0;
            if (wth.diary_sel >= INTRO_PAGES) {
                wth.diary_sel = 0;
                wth_goto(SC_PLAY);
                wth_say("YOU'RE ON A HEAP OF STRAW IN A STONE ROOM. YOUR HEAD THROBS. THE DOOR IS SHUT, AND THERE'S NO "
                        "HANDLE ON THIS SIDE.");
            }
        }
        break;
    case SC_WAKE:
        if (btnp(BTN_A) && wth.scene_t > 60) {
            wth.s.room = R_CELL;
            wth_keeper_reset();
            wth.s.spawn_left = wth.s.interval;
            wth.held = 0;
            wth.trans_t = 0;
            wth.cx = 160 << 8;
            wth.cy = 80 << 8;
            wth_save_now();
            wth_goto(SC_PLAY);
        }
        break;
    case SC_ENDING:
        if (btnp(BTN_A) && wth.scene_t > 30) {
            wth.diary_sel++;
            wth.scene_t = 0;
            int pages = END_PAGES + (wth.ending_best ? 1 : 0);
            if (wth.diary_sel >= pages) {
                wth.diary_sel = 0;
                wth_goto(SC_CREDITS);
            }
        }
        break;
    case SC_CREDITS:
        if ((btnp(BTN_A) && wth.scene_t > 90) || wth.scene_t > 1500) {
            if (wth.ending_caught) wth_goto(SC_STINGER);
            else wth_goto(SC_TITLE);
        }
        break;
    case SC_STINGER:
        if (wth.scene_t == 90) {
            sfx_play_name("wth_sting");
            gfx_set_flash(3);
        }
        if (btnp(BTN_A) && wth.scene_t > 200) wth_goto(SC_TITLE);
        break;
    default: break;
    }
}

/* ------------------------------------------------------------------ */
/* the cartridge                                                        */

static void wth_load(void) {
    wth_art_load();
    wth_audio_load();
}

static void wth_start(void) {
    memset(&wth, 0, sizeof wth);
    rng_seed(&wth.rng, rng_next(&g_rng));
    WthSave s;
    if (load_save(&s)) rebuild_goals(&s);
    wth_keeper_reset();
    cheat_used = false;
    wth_goto(SC_WARNING);
}

static void wth_quit(void) {
    if (wth.scene == SC_PLAY || wth.scene == SC_BAG || wth.scene == SC_DIARY) wth_save_now();
}

static void update(void) {
    wth.frame++;
    wth.scene_t++;
    switch (wth.scene) {
    case SC_PLAY: update_play(); break;
    case SC_BAG: update_bag(); break;
    case SC_DIARY: update_diary(); break;
    default: update_screens(); break;
    }
}

static void draw(void) {
    switch (wth.scene) {
    case SC_PLAY: wth_draw_play(); break;
    case SC_BAG: wth_draw_play(); wth_draw_bag(); break;
    case SC_DIARY: wth_draw_diary(); break;
    default: wth_draw_screen(); break;
    }
}

/* ------------------------------------------------------------------ */
/* test hooks                                                           */

static const char *const ITEM_ID[I_COUNT] = {
    "none", "shoehorn", "mug", "wire", "newspaper", "diary", "pennies", "matches", "knife", "soda", "tongs",
    "tinkey", "cuphook", "copperkey", "gear", "twine", "oil", "torch", "paraffin", "bar", "batteries", "cane",
    "longhook", "hymn", "shedkey", "spanner", "screwdriver", "shears", "spade", "pewterkey", "motor", "carkeys",
    "pig", "brasskey", "saw", "garnet", "ring", "jade", "snuff", "clove", "borax", "lens", "combo", "opal", "doll",
    "silverkey", "password", "glasses", "directions", "crossbow", "steelwire", "amber", "bolt", "keeperkey", "brew",
};
static const char *const ROOM_ID[R_COUNT] = {
    "cell", "passage", "sickroom", "hall", "parlour", "dining", "kitchen", "scullery", "smoking", "washroom",
    "guest", "glasshouse", "garage", "pond", "shed", "garden", "drive", "courtyard", "maze", "heart", "landing",
    "master", "balcony", "study", "bath", "nursery", "children", "attic", "cellar", "spore",
};
#define X(id, ...) #id,
static const char *const SPOT_ID[S_COUNT] = {WTH_SPOT_LIST(X)};
#undef X
#define X(f) #f,
static const char *const FLAG_ID[F_COUNT] = {WTH_FLAG_LIST(X)};
#undef X

static int find_name(const char *name, const char *const *list, int n) {
    for (int i = 0; i < n; i++)
        if (!strcmp(name, list[i])) return i;
    return -1;
}
int wth_find_item(const char *n) { return find_name(n, ITEM_ID, I_COUNT); }
int wth_find_spot(const char *n) { return find_name(n, SPOT_ID, S_COUNT); }
const char *wth_spot_name(int s) { return s >= 0 && s < S_COUNT ? SPOT_ID[s] : "-"; }
const char *wth_room_id(int r) { return r >= 0 && r < R_COUNT ? ROOM_ID[r] : "-"; }

static int prefixed(const char *key, const char *pre, const char *const *list, int n) {
    size_t l = strlen(pre);
    if (strncmp(key, pre, l)) return -2;
    return find_name(key + l, list, n);
}

static int wth_query(const char *key, int *out) {
    int i;
    if (!strcmp(key, "bot")) { *out = wth.bot_on ? (int)wth_bot() : 0; return 1; }
    if (!strcmp(key, "scene")) { *out = wth.scene; return 1; }
    if (!strcmp(key, "room")) { *out = wth.s.room; return 1; }
    if (!strcmp(key, "cx")) { *out = (int)(wth.cx >> 8); return 1; }
    if (!strcmp(key, "cy")) { *out = (int)(wth.cy >> 8); return 1; }
    if (!strcmp(key, "cx256")) { *out = (int)wth.cx; return 1; }
    if (!strcmp(key, "cy256")) { *out = (int)wth.cy; return 1; }
    if (!strcmp(key, "held")) { *out = wth.held; return 1; }
    if (!strcmp(key, "bag")) { *out = wth_bag_count(); return 1; }
    if (!strcmp(key, "bag_sel")) { *out = wth.bag_sel; return 1; }
    if (!strcmp(key, "pages")) { *out = wth_page_count(); return 1; }
    if (!strcmp(key, "diary_sel")) { *out = wth.diary_sel; return 1; }
    if (!strcmp(key, "enc")) { *out = wth.k.state; return 1; }
    if (!strcmp(key, "enc_t")) { *out = (int)wth.k.t; return 1; }
    if (!strcmp(key, "chain")) { *out = wth.k.chain; return 1; }
    if (!strcmp(key, "kroom")) { *out = wth.k.room; return 1; }
    if (!strcmp(key, "scared")) { *out = wth_scared(); return 1; }
    if (!strcmp(key, "visible")) { *out = wth_keeper_visible(); return 1; }
    if (!strcmp(key, "mini_n")) { *out = wth.k.mini.n; return 1; }
    if (!strcmp(key, "mini_miss")) { *out = wth.k.mini.miss; return 1; }
    if (!strcmp(key, "mini_hit")) { *out = wth.k.mini.hit; return 1; }
    if (!strcmp(key, "mini_zw")) { *out = wth.k.mini.zw; return 1; }
    if (!strcmp(key, "mini_vel")) { *out = iabs((int)wth.k.mini.vel); return 1; }
    if (!strcmp(key, "mini_in")) {
        int x = (int)(wth.k.mini.pos >> 8);
        *out = wth.k.mini.pause == 0 && x >= wth.k.mini.zone && x < wth.k.mini.zone + wth.k.mini.zw;
        return 1;
    }
    if (!strcmp(key, "mini_pause")) { *out = wth.k.mini.pause; return 1; }
    if (!strcmp(key, "spawn_left")) { *out = (int)wth.s.spawn_left; return 1; }
    if (!strcmp(key, "interval")) { *out = (int)wth.s.interval; return 1; }
    if (!strcmp(key, "caught")) { *out = wth.s.caught; return 1; }
    if (!strcmp(key, "escapes")) { *out = wth.s.escapes; return 1; }
    if (!strcmp(key, "stuns")) { *out = wth.s.stuns; return 1; }
    if (!strcmp(key, "ending")) { *out = wth.ending; return 1; }
    if (!strcmp(key, "ending_best")) { *out = wth.ending_best; return 1; }
    if (!strcmp(key, "ending_caught")) { *out = wth.ending_caught; return 1; }
    if (!strcmp(key, "golds")) { *out = wth.s.golds; return 1; }
    if (!strcmp(key, "cherries")) { *out = wth.s.cherries; return 1; }
    if (!strcmp(key, "bests")) { *out = wth.s.bests; return 1; }
    if (!strcmp(key, "talking")) { *out = wth_talking(); return 1; }
    if (!strcmp(key, "trans")) { *out = wth.trans_t; return 1; }
    if (!strcmp(key, "maze")) { *out = wth.s.maze; return 1; }
    if (!strcmp(key, "mug")) { *out = wth.s.mug; return 1; }
    if (!strcmp(key, "noises")) { *out = wth.noise_count; return 1; }
    if (!strcmp(key, "title_sel")) { *out = wth.title_sel; return 1; }
    if (!strcmp(key, "spot_under")) { *out = wth_spot_at((int)(wth.cx >> 8) + wth.shake_x, (int)(wth.cy >> 8) + wth.shake_y); return 1; }
    if (!strcmp(key, "save_bytes")) { *out = (int)sizeof(WthSave); return 1; }
    if (!strcmp(key, "save_exists")) { *out = save_exists(); return 1; }
    if (!strcmp(key, "bot_step")) { *out = wth_bot_step; return 1; }
    if (!strcmp(key, "bot_op")) { *out = wth_bot_cur(0); return 1; }
    if (!strcmp(key, "bot_a")) { *out = wth_bot_cur(1); return 1; }
    if (!strcmp(key, "bot_b")) { *out = wth_bot_cur(2); return 1; }
    if (!strcmp(key, "art_bad")) { *out = wth_art_bad(); return 1; }
    if (!strcmp(key, "layout_bad")) { *out = wth_layout_audit(); return 1; }
    if (!strcmp(key, "dark")) { *out = wth_room_dark(wth.s.room); return 1; }
    if (!strcmp(key, "music")) { *out = music_playing(); return 1; }
    if (!strcmp(key, "mus_chase")) { *out = WTH_MUS_CHASE; return 1; }
    if (!strcmp(key, "mus_hide")) { *out = WTH_MUS_HIDE; return 1; }
    if (!strcmp(key, "mus_house")) { *out = WTH_MUS_HOUSE; return 1; }
    if (!strcmp(key, "mus_dawn")) { *out = WTH_MUS_DAWN; return 1; }
    if (!strcmp(key, "item_kinds")) { *out = WTH_ITEM_KINDS; return 1; }
    if (!strcmp(key, "room_count")) { *out = R_COUNT; return 1; }
    if (!strcmp(key, "page_count")) { *out = WTH_PAGES; return 1; }
    if (!strcmp(key, "spot_count")) { *out = S_COUNT; return 1; }
    if (!strcmp(key, "rooms_bad")) {
        /* every room has a way out, every spot lies inside the picture, and each page lies at a real spot */
        int bad = 0;
        for (int r = 0; r < R_COUNT; r++) {
            int exits = 0;
            for (int s = 0; s < S_COUNT; s++) exits += WTH_SPOT[s].room == r && WTH_SPOT[s].kind == HS_EXIT;
            bad += exits == 0;
        }
        for (int s = 0; s < S_COUNT; s++) {
            const WthSpot *p = &WTH_SPOT[s];
            bad += p->x < WTH_PX || p->y < WTH_PY || p->x + p->w > WTH_PX + WTH_PW || p->y + p->h > WTH_PY + WTH_PH;
        }
        for (int i = 0; i < WTH_PAGES; i++)
            for (int j = i + 1; j < WTH_PAGES; j++) bad += WTH_PAGE_SPOT[i] == WTH_PAGE_SPOT[j];
        *out = bad;
        return 1;
    }
    if ((i = prefixed(key, "has_", ITEM_ID, I_COUNT)) != -2) { *out = i >= 0 && wth_has(i); return 1; }
    if ((i = prefixed(key, "slotof_", ITEM_ID, I_COUNT)) != -2) { *out = i >= 0 ? wth_slot_of(i) : -9; return 1; }
    if ((i = prefixed(key, "item_", ITEM_ID, I_COUNT)) != -2) { *out = i; return 1; }
    if ((i = prefixed(key, "room_", ROOM_ID, R_COUNT)) != -2) { *out = i; return 1; }
    if ((i = prefixed(key, "flag_", FLAG_ID, F_COUNT)) != -2) { *out = i >= 0 && wth_flag(i); return 1; }
    if ((i = prefixed(key, "live_", SPOT_ID, S_COUNT)) != -2) { *out = i >= 0 && wth_spot_live(i); return 1; }
    if ((i = prefixed(key, "spot_", SPOT_ID, S_COUNT)) != -2) { *out = i; return 1; }
    if (!strncmp(key, "slot", 4) && key[4] >= '0' && key[4] <= '9') {
        int n = atoi(key + 4);
        *out = n < WTH_BAG ? wth.s.bag[n] : -1;
        return 1;
    }
    if (!strncmp(key, "hare", 4) && key[4] >= '0' && key[4] <= '3' && !key[5]) { *out = wth.s.hare[key[4] - '0']; return 1; }
    if (!strncmp(key, "floor", 5) && key[5] >= '0' && key[5] <= '2' && !key[6]) {
        *out = wth.s.floor[wth.s.room][key[5] - '0'];
        return 1;
    }
    return 0;
}

static void aim_at(int s) {
    const WthSpot *p = &WTH_SPOT[s];
    wth.cx = (p->x + p->w / 2) << 8;
    wth.cy = (p->y + p->h / 2) << 8;
}

static int wth_cheat(const char *cmd) {
    char name[64];
    int a, b;
    cheat_used = true;
    if (sscanf(cmd, "room %63s", name) == 1) {
        int r = find_name(name, ROOM_ID, R_COUNT);
        if (r < 0) return 0;
        wth.s.room = (uint8_t)r;
        wth.trans_t = 0;
        wth_set(F_LEFT_CELL);
        if (wth.scene != SC_PLAY) wth_goto(SC_PLAY);
        wth_room_music();
        return 1;
    }
    if (sscanf(cmd, "give %63s", name) == 1) {
        int it = find_name(name, ITEM_ID, I_COUNT);
        if (it <= 0) return 0;
        wth_give(it);
        return 1;
    }
    if (sscanf(cmd, "drop %63s", name) == 1) {
        int it = find_name(name, ITEM_ID, I_COUNT);
        if (it <= 0) return 0;
        wth_take_away(it);
        return 1;
    }
    if (sscanf(cmd, "hold %63s", name) == 1) {
        int it = find_name(name, ITEM_ID, I_COUNT);
        if (it < 0) return 0;
        if (it && !wth_has(it)) wth_give(it);
        wth.held = it;
        return 1;
    }
    if (sscanf(cmd, "set %63s", name) == 1) {
        int f = find_name(name, FLAG_ID, F_COUNT);
        if (f < 0) return 0;
        wth_set(f);
        return 1;
    }
    if (sscanf(cmd, "unset %63s", name) == 1) {
        int f = find_name(name, FLAG_ID, F_COUNT);
        if (f < 0) return 0;
        wth_clear(f);
        return 1;
    }
    if (sscanf(cmd, "aim %63s", name) == 1) {
        int s = find_name(name, SPOT_ID, S_COUNT);
        if (s < 0) return 0;
        aim_at(s);
        return 1;
    }
    if (sscanf(cmd, "cursor %d %d", &a, &b) == 2) { wth.cx = a << 8; wth.cy = b << 8; return 1; }
    if (sscanf(cmd, "spawn %d", &a) == 1) { wth.s.spawn_left = a; return 1; }
    if (sscanf(cmd, "interval %d", &a) == 1) { wth.s.interval = a; return 1; }
    if (!strcmp(cmd, "noise")) { wth_noise(false); return 1; }
    if (!strcmp(cmd, "keeper")) {
        wth_noise(false);
        wth.k.t = 1;
        return 1;
    }
    if (!strcmp(cmd, "hush")) { wth.msg_n = wth.msg_page = 0; return 1; }
    if (!strcmp(cmd, "new")) { new_game(); wth_set(F_LEFT_CELL); wth_goto(SC_PLAY); return 1; }
    if (!strcmp(cmd, "save")) { wth_save_now(); return 1; }
    if (sscanf(cmd, "scene %d", &a) == 1) { wth_goto(a); return 1; }
    if (sscanf(cmd, "maze %d", &a) == 1) { wth.s.maze = (uint8_t)a; return 1; }
    if (sscanf(cmd, "pages %d", &a) == 1) { wth.s.pages = (uint32_t)a; if (a) wth_give(I_DIARY); return 1; }
    if (sscanf(cmd, "mug %d", &a) == 1) { wth.s.mug = (uint8_t)a; return 1; }
    if (sscanf(cmd, "minizone %d %d", &a, &b) == 2) { wth.k.mini.zone = (int16_t)a; wth.k.mini.zw = (int16_t)b; return 1; }
    if (sscanf(cmd, "ending %d", &a) == 1) { wth_end_game(a); return 1; }
    if (sscanf(cmd, "seed %d", &a) == 1) { rng_seed(&wth.rng, (uint64_t)a); return 1; }
    if (sscanf(cmd, "bot %d", &a) == 1) { wth.bot_on = 1; wth.bot_plan = (uint8_t)a; wth_bot_reset(a); return 1; }
    if (sscanf(cmd, "bot_debug %d", &a) == 1) { wth_bot_debug = a; return 1; }
    if (!strcmp(cmd, "bot_off")) { wth.bot_on = 0; return 1; }
    return 0;
}

const GameDef GAME_WITHERLEA = {
    "witherlea",
    "WITHERLEA",
    "1988",
    "ADVENTURE",
    "LOCKED IN A HOUSE ON THE FENS WITH THE KEEPER. LOOK, TAKE, HIDE, ESCAPE.",
    {"ESCAPE THE KEEPER", "DRIVE AWAY FROM WITHERLEA", "KILL WHAT GROWS UNDER THE HOUSE"},
    GLYPH_DPAD "\tMOVE THE HAND (8 WAYS)\n"
    GLYPH_A "\tLOOK, TAKE, USE, GO / NEXT LINE\n"
    GLYPH_B "\tTHE BAG: HOLD A THING, OR\n"
    "\tPUT TWO THINGS TOGETHER\n"
    "HIDING\t" GLYPH_A " WHEN THE MARK IS IN THE BOX\n"
    "START\tPAUSE",
    C_NIGHT, C_ICE,
    wth_load, wth_start, update, draw, wth_quit, wth_draw_label, wth_query, wth_cheat,
    "NIGHT MANOR", 42,
    NULL,
};
