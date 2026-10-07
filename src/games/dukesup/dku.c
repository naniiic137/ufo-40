/* DUKES UP - Rook, Pip, Mack and Dolly punch their way up to the
 * penthouse. Cartridge 33 of UFO 40, a tribute to Fist Hell (UFO 50 #33).
 * See docs/games/33-dukes-up.md. This file: the title, the story, the
 * fighter select, the night cards, the corner shop, the continue screen,
 * the endings, the records, the save and the test hooks. A night itself is
 * in dku_world.c, the fighters in dku_play.c, the ghouls in dku_foes.c. */
#include "dku.h"

#define SAVE_MAGIC 0x44554B31u

DkuGame dku_g;
DkuSave dku_sv;

static bool vita_single(void) { return plat_kind() == PLAT_VITA; }

void dku_sfx(const char *name) { sfx_play_name(name); }

static void set_state(int s) {
    dku_g.state = s;
    dku_g.state_t = 0;
}

void dku_save_now(void) { game_save_write(game_current_index(), &dku_sv, (int)sizeof dku_sv); }

static void load_save(void) {
    DkuSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { dku_sv = tmp; return; }
    memset(&dku_sv, 0, sizeof dku_sv);
    dku_sv.magic = SAVE_MAGIC;
    dku_sv.fewest_continues = 0xFFFF;
}

/* the goals come back from the records (RESET GOALS clears both) */
static void goals_from_save(void) {
    if (dku_sv.gym_best1 >= DKU_GYM_GIFT || dku_sv.gym_best2 >= DKU_GYM_GIFT) game_award(GOAL_BEACON);
    if (dku_sv.wins > 0) game_award(GOAL_SAUCER);
    if (dku_sv.good_wins > 0) game_award(GOAL_ALIEN);
}

void dku_award_gym(void) { game_award(GOAL_BEACON); }

static int night_music(void) {
    static int *const M[DKU_NIGHTS] = {&DKU_MUS_N1, &DKU_MUS_N2, &DKU_MUS_N3, &DKU_MUS_N4, &DKU_MUS_N5};
    const DkuSection *s = &DKU_NIGHT[dku_g.night].sec[dku_g.sec];
    if (s->kind == SEC_LIFT) return DKU_MUS_LIFT;
    if (s->kind == SEC_BOSS) return dku_g.night == DKU_NIGHTS - 1 ? DKU_MUS_GRIST : DKU_MUS_BOSS;
    return *M[dku_g.night];
}

static void to_title(void) {
    set_state(DS_TITLE);
    game_set_pausable(false);
    input_set_versus(false);
    dku_g.gym = false;
    music_play(DKU_MUS_TITLE);
}

static void run_over(void) {
    if (dku_g.night + 1 > dku_sv.best_night && !dku_g.gym) dku_sv.best_night = (uint16_t)(dku_g.night + 1);
    dku_sv.total_kos += (uint32_t)dku_g.kos;
    dku_save_now();
}

static void start_card(void) {
    set_state(DS_CARD);
    dku_g.dead_t = 0;
    game_set_pausable(false);
    music_stop();
}

static void begin_run(void) {
    dku_new_run(dku_g.menu == 1 ? 2 : 1);
    input_set_versus(dku_g.players == 2);
    if (dku_sv.runs < 65535) dku_sv.runs++;
    dku_save_now();
    dku_start_night(0);
    start_card();
}

/* ---- the shop ------------------------------------------------------------------ */

enum { SHOP_HEAL, SHOP_POWER, SHOP_RECOV, SHOP_TOUGH, SHOP_GO, SHOP_ROWS };
static int shop_price(int row) { return row == SHOP_HEAL ? DKU_PRICE_HEAL : row == SHOP_GO ? 0 : DKU_PRICE_STAT; }
static int shop_stat(int row) { return row == SHOP_POWER ? DK_POWER : row == SHOP_RECOV ? DK_RECOV : DK_TOUGH; }

static bool shop_buy(int who, int row) {
    if (row == SHOP_GO) return false;
    int price = shop_price(row);
    Actor *a = &dku_g.a[who];
    if (row == SHOP_HEAL) {
        if (a->hp >= DKU_MAX_HP) { dku_g.shop_msg = "ALREADY IN ONE PIECE."; dku_g.shop_msg_t = 90; return false; }
    } else if (dku_g.pr[who].stat[shop_stat(row)] >= 3) {
        dku_g.shop_msg = "THAT ONE'S MAXED OUT.";
        dku_g.shop_msg_t = 90;
        return false;
    }
    if (dku_g.cash < price) { dku_g.shop_msg = "NOT ENOUGH CASH."; dku_g.shop_msg_t = 90; return false; }
    dku_g.cash -= price;
    if (row == SHOP_HEAL) a->hp = DKU_MAX_HP;
    else dku_g.pr[who].stat[shop_stat(row)]++;
    dku_g.shop_msg = "THANKS, COME AGAIN!";
    dku_g.shop_msg_t = 60;
    return true;
}

static void open_shop(void) {
    set_state(DS_SHOP);
    dku_g.shop_sel = 0;
    dku_g.shop_who = 0;
    while (dku_g.shop_who < dku_g.players && (!dku_g.a[dku_g.shop_who].alive)) dku_g.shop_who++;
    dku_g.shop_msg = NULL;
    music_play(DKU_MUS_SHOP);
    game_set_pausable(true);
}

static void shop_update(void) {
    if (dku_g.shop_msg_t > 0) dku_g.shop_msg_t--;
    bool up = dku_g.shop_who == 0 ? btn_repeat(BTN_UP) : btn_repeat2(BTN_UP);
    bool down = dku_g.shop_who == 0 ? btn_repeat(BTN_DOWN) : btn_repeat2(BTN_DOWN);
    bool a = dku_g.shop_who == 0 ? btnp(BTN_A) : btnp2(BTN_A);
    if (up) { dku_g.shop_sel = (dku_g.shop_sel + SHOP_ROWS - 1) % SHOP_ROWS; sfx_play_name("ui_move"); }
    if (down) { dku_g.shop_sel = (dku_g.shop_sel + 1) % SHOP_ROWS; sfx_play_name("ui_move"); }
    if (!a) return;
    if (dku_g.shop_sel == SHOP_GO) {
        if (dku_g.players == 2 && dku_g.shop_who == 0) {
            /* player two's turn at the counter */
            dku_g.shop_who = 1;
            dku_g.shop_sel = 0;
            sfx_play_name("ui_ok");
            return;
        }
        sfx_play_name("ui_ok");
        input_consume();
        dku_start_night(dku_g.night + 1);
        start_card();
        return;
    }
    if (shop_buy(dku_g.shop_who, dku_g.shop_sel)) sfx_play_name("dku_buy");
    else sfx_play_name("ui_error");
}

/* ---- the continue screen --------------------------------------------------------- */


static void start_continue(void) {
    set_state(DS_CONTINUE);
    dku_g.cont_sel = 0;
    dku_g.cont_t = 10 * 60;
    dku_g.tip = (dku_g.continues + dku_g.night * 3 + dku_g.kos) % DKU_NTIPS;
    game_set_pausable(false);
    music_play(DKU_MUS_DOWN);
}

static void continue_update(void) {
    if (dku_g.state_t < 30) return;
    if (btnp(BTN_LEFT) || btnp(BTN_RIGHT)) { dku_g.cont_sel ^= 1; sfx_play_name("ui_move"); }
    if (--dku_g.cont_t <= 0) dku_g.cont_sel = 1;
    if (btnp(BTN_A) || btnp(BTN_START) || dku_g.cont_t <= 0) {
        input_consume();
        if (dku_g.cont_sel == 0) {
            /* back to the start of the night: cash and stats stay */
            dku_g.continues++;
            for (int p = 0; p < dku_g.players; p++) {
                dku_g.a[p].alive = 1;
                dku_g.a[p].kind = AK_FIGHTER;
                dku_g.a[p].hp = DKU_MAX_HP;
                dku_g.a[p].state = AS_FREE;
            }
            dku_start_night(dku_g.night);
            start_card();
        } else {
            set_state(DS_OVER);
            music_play(DKU_MUS_DOWN);
        }
    }
}

/* ---- the run ends ----------------------------------------------------------------- */

static void win_run(void) {
    dku_g.won = true;
    if (dku_sv.wins < 65535) dku_sv.wins++;
    if (dku_g.continues == 0 && dku_sv.good_wins < 65535) dku_sv.good_wins++;
    if ((uint32_t)dku_g.continues < dku_sv.fewest_continues) dku_sv.fewest_continues = (uint16_t)imin(65534, dku_g.continues);
    game_award(GOAL_SAUCER);
    if (dku_g.continues == 0) game_award(GOAL_ALIEN);
    dku_sv.best_night = DKU_NIGHTS;
    run_over();
    set_state(DS_ENDING);
    game_set_pausable(false);
    music_play(dku_g.continues == 0 ? DKU_MUS_GOOD : DKU_MUS_BAD);
}

static void dku_update(void) {
    dku_g.state_t++;
    dku_g.frame_t++;
    switch (dku_g.state) {
    case DS_TITLE:
        game_set_pausable(false);
        if (btnp(BTN_UP) || btnp(BTN_DOWN)) { dku_g.menu ^= 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if ((btnp(BTN_A) || btnp(BTN_START)) && dku_g.state_t > 5) {
            if (dku_g.menu == 1 && vita_single()) { sfx_play_name("ui_error"); break; }
            sfx_play_name("ui_ok");
            set_state(DS_STORY);
        }
        break;
    case DS_STORY:
        if ((dku_g.state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) || dku_g.state_t > 1200) {
            input_consume();
            set_state(DS_SELECT);
            dku_g.picked[0] = dku_g.picked[1] = false;
            dku_g.players = dku_g.menu == 1 ? 2 : 1;
            input_set_versus(dku_g.players == 2);
            music_play(DKU_MUS_SELECT);
        }
        if (btnp(BTN_B)) to_title();
        break;
    case DS_SELECT: {
        for (int p = 0; p < dku_g.players; p++) {
            bool l = p == 0 ? btnp(BTN_LEFT) : btnp2(BTN_LEFT);
            bool r = p == 0 ? btnp(BTN_RIGHT) : btnp2(BTN_RIGHT);
            bool a = p == 0 ? btnp(BTN_A) : btnp2(BTN_A);
            bool b = p == 0 ? btnp(BTN_B) : btnp2(BTN_B);
            if (!dku_g.picked[p]) {
                if (l) { dku_g.sel[p] = (dku_g.sel[p] + DK_NFIGHTERS - 1) % DK_NFIGHTERS; sfx_play_name("ui_move"); }
                if (r) { dku_g.sel[p] = (dku_g.sel[p] + 1) % DK_NFIGHTERS; sfx_play_name("ui_move"); }
                if (a && dku_g.state_t > 10) { dku_g.picked[p] = true; dku_g.pr[p].pick = dku_g.sel[p]; sfx_play_name("ui_ok"); }
                if (b && p == 0) { to_title(); return; }
            } else if (b) {
                dku_g.picked[p] = false;
                sfx_play_name("ui_back");
            }
        }
        bool all = true;
        for (int p = 0; p < dku_g.players; p++) all &= dku_g.picked[p];
        if (all) {
            input_consume();
            begin_run();
        }
        break;
    }
    case DS_CARD:
        if (dku_g.state_t > 110 || (dku_g.state_t > 20 && btnp(BTN_A))) {
            input_consume();
            set_state(DS_PLAY);
            game_set_pausable(true);
            music_play(night_music());
        }
        break;
    case DS_PLAY: {
        game_set_pausable(true);
        int sec_before = dku_g.sec;
        bool gym_before = dku_g.gym;
        dku_world_update();
        if (dku_g.gym != gym_before || dku_g.sec != sec_before) {
            if (!dku_g.gym) music_play(night_music());
        }
        if (dku_fighters_alive() == 0) {
            if (++dku_g.dead_t > 70) {
                if (dku_g.gym) {
                    set_state(DS_GYMOVER);
                    music_play(DKU_MUS_DOWN);
                    dku_save_now();
                } else {
                    start_continue();
                }
            }
            break;
        }
        if (dku_g.night_done) {
            dku_g.night_done = false;
            dku_g.clear_cash = dku_g.cash;
            if (dku_g.night + 1 > dku_sv.best_night) dku_sv.best_night = (uint16_t)(dku_g.night + 1);
            dku_save_now();
            set_state(DS_CLEAR);
            music_play(DKU_MUS_CLEAR);
            game_set_pausable(false);
        }
        break;
    }
    case DS_CLEAR:
        if (dku_g.state_t > 60 && (btnp(BTN_A) || btnp(BTN_START) || (dku_g.players == 2 && btnp2(BTN_A)))) {
            input_consume();
            if (dku_g.night == DKU_NIGHTS - 1) win_run();
            else open_shop();
        }
        break;
    case DS_SHOP:
        game_set_pausable(true);
        shop_update();
        break;
    case DS_CONTINUE:
        continue_update();
        break;
    case DS_OVER:
    case DS_GYMOVER:
        if (dku_g.state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            run_over();
            to_title();
        }
        break;
    case DS_ENDING:
        if (dku_g.state_t > 150 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            set_state(DS_CREDITS);
        }
        break;
    case DS_CREDITS:
        if (btn(BTN_A)) dku_g.state_t += 3;
        if (dku_g.state_t > 1300) {
            input_consume();
            to_title();
        }
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* cartridge interface                                                      */

static void dku_load(void) {
    dku_art_load();
    dku_audio_load();
}

static void dku_start(void) {
    load_save();
    memset(&dku_g, 0, sizeof dku_g);
    rng_seed(&dku_g.rng, (uint64_t)rng_next(&g_rng) + 33u);
    dku_g.boss = -1;
    dku_g.cam_lock = -1;
    goals_from_save();
    to_title();
}

static void dku_quit(void) {
    input_set_versus(false);
    dku_save_now();
}

static int count_kind(int k) {
    int n = 0;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) n += dku_g.a[i].alive && dku_g.a[i].kind == k && dku_g.a[i].state != AS_DEAD;
    return n;
}

static int count_items(int k) {
    int n = 0;
    for (int i = 0; i < DKU_MAX_ITEMS; i++) n += dku_g.it[i].alive == 1 && (k < 0 || dku_g.it[i].kind == k);
    return n;
}

static int count_hz(int k) {
    int n = 0;
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) n += dku_g.hz[i].alive && dku_g.hz[i].kind == k;
    return n;
}

static int dku_query(const char *key, int *out) {
    if (!strcmp(key, "bot")) { *out = dku_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = dku_g.state; return 1; }
    if (!strcmp(key, "frames")) { *out = dku_g.frame_t; return 1; }
    if (!strcmp(key, "bot_why")) { extern int dku_bot_why; *out = dku_bot_why; return 1; }
    if (!strcmp(key, "bot_target")) { extern int dku_bot_target; *out = dku_bot_target; return 1; }
    if (!strcmp(key, "menu")) { *out = dku_g.menu; return 1; }
    if (!strcmp(key, "players")) { *out = dku_g.players; return 1; }
    if (!strcmp(key, "night")) { *out = dku_g.night + 1; return 1; }
    if (!strcmp(key, "sec")) { *out = dku_g.sec; return 1; }
    if (!strcmp(key, "cam")) { *out = dku_g.cam; return 1; }
    if (!strcmp(key, "cam_lock")) { *out = dku_g.cam_lock; return 1; }
    if (!strcmp(key, "cash")) { *out = dku_g.cash; return 1; }
    if (!strcmp(key, "continues")) { *out = dku_g.continues; return 1; }
    if (!strcmp(key, "won")) { *out = dku_g.won; return 1; }
    if (!strcmp(key, "gym")) { *out = dku_g.gym; return 1; }
    if (!strcmp(key, "gym_wave")) { *out = dku_g.gym_wave; return 1; }
    if (!strcmp(key, "gym_beaten")) { *out = dku_g.gym ? dku_g.gym_wave - 1 : 0; return 1; }
    if (!strcmp(key, "kos")) { *out = dku_g.kos; return 1; }
    if (!strcmp(key, "kick_kills")) { *out = dku_g.kick_kills; return 1; }
    if (!strcmp(key, "enemies")) { *out = dku_enemies_alive(); return 1; }
    if (!strcmp(key, "awake")) { *out = dku_enemies_awake(); return 1; }
    if (!strcmp(key, "exit")) { *out = dku_g.exit_t; return 1; }
    if (!strcmp(key, "fade")) { *out = dku_g.fade; return 1; }
    if (!strcmp(key, "lift_wave")) { *out = dku_g.lift_wave; return 1; }
    if (!strcmp(key, "sel1")) { *out = dku_g.sel[0]; return 1; }
    if (!strcmp(key, "sel2")) { *out = dku_g.sel[1]; return 1; }
    if (!strcmp(key, "shop_sel")) { *out = dku_g.shop_sel; return 1; }
    if (!strcmp(key, "shop_who")) { *out = dku_g.shop_who; return 1; }
    if (!strcmp(key, "cont_sel")) { *out = dku_g.cont_sel; return 1; }
    if (!strcmp(key, "last_dmg")) { *out = dku_g.last_dmg; return 1; }
    if (!strncmp(key, "dmg_by", 6)) { int k = atoi(key + 6) + 10; *out = k >= 0 && k < 32 ? dku_g.dmg_by[k] : 0; return 1; }
    if (!strcmp(key, "boss_alive")) { *out = dku_g.boss >= 0 && dku_g.a[dku_g.boss].alive && dku_g.a[dku_g.boss].state != AS_DEAD; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = dku_g.boss >= 0 && dku_g.a[dku_g.boss].alive ? dku_g.a[dku_g.boss].hp : 0; return 1; }
    if (!strcmp(key, "boss_mode")) { *out = dku_g.boss >= 0 ? dku_g.a[dku_g.boss].mode : -1; return 1; }
    if (!strcmp(key, "boss_state")) { *out = dku_g.boss >= 0 ? dku_g.a[dku_g.boss].state : -1; return 1; }
    if (!strcmp(key, "boss_down")) { *out = dku_g.boss_down; return 1; }
    if (!strcmp(key, "dogs")) { *out = 0; for (int i = 2; i < DKU_MAX_ACTORS; i++) *out += dku_g.a[i].alive && dku_g.a[i].kind == AK_DOG && dku_g.a[i].team == 0; return 1; }
    if (!strcmp(key, "items")) { *out = count_items(-1); return 1; }
    if (!strncmp(key, "items_", 6)) { *out = count_items(atoi(key + 6)); return 1; }
    if (!strncmp(key, "kind_", 5)) { *out = count_kind(atoi(key + 5)); return 1; }
    if (!strncmp(key, "hz_", 3)) { *out = count_hz(atoi(key + 3)); return 1; }
    if (!strncmp(key, "lamp_", 5)) {
        for (int i = 0; i < DKU_MAX_HAZARDS; i++)
            if (dku_g.hz[i].alive && dku_g.hz[i].kind == HZ_LAMP) {
                *out = key[5] == 'x' ? dku_g.hz[i].x : key[5] == 'y' ? dku_g.hz[i].y : dku_g.hz[i].t;
                return 1;
            }
        *out = -1;
        return 1;
    }
    if (!strcmp(key, "props")) { *out = 0; for (int i = 0; i < DKU_MAX_PROPS; i++) *out += dku_g.pr_[i].alive; return 1; }
    if (!strcmp(key, "shots")) { *out = 0; for (int i = 0; i < DKU_MAX_SHOTS; i++) *out += dku_g.sh[i].alive; return 1; }
    if (!strncmp(key, "shotkind_", 9)) { int k = atoi(key + 9); *out = 0; for (int i = 0; i < DKU_MAX_SHOTS; i++) *out += dku_g.sh[i].alive && dku_g.sh[i].kind == k; return 1; }
    if (!strcmp(key, "shot0_team")) { *out = -1; for (int i = 0; i < DKU_MAX_SHOTS; i++) if (dku_g.sh[i].alive) { *out = dku_g.sh[i].team; break; } return 1; }
    if (!strcmp(key, "shot0_vx")) { *out = 0; for (int i = 0; i < DKU_MAX_SHOTS; i++) if (dku_g.sh[i].alive) { *out = dku_g.sh[i].vx; break; } return 1; }
    if (!strcmp(key, "layout")) { *out = dku_layout_audit(); return 1; }
    if (!strcmp(key, "vita_single")) { *out = vita_single(); return 1; }
    if (!strncmp(key, "item", 4) && isdigit((unsigned char)key[4])) {
        int i = atoi(key + 4);
        const char *u = strchr(key + 4, '_');
        if (i < 0 || i >= DKU_MAX_ITEMS || !u) return 0;
        const Item *it = &dku_g.it[i];
        if (!strcmp(u, "_kind")) { *out = it->alive == 1 ? it->kind : 0; return 1; }
        if (!strcmp(u, "_x")) { *out = dku_px(it->x); return 1; }
        if (!strcmp(u, "_y")) { *out = dku_px(it->y); return 1; }
        if (!strcmp(u, "_z")) { *out = dku_px(it->z); return 1; }
        return 0;
    }
    if (!strncmp(key, "prop", 4) && isdigit((unsigned char)key[4])) {
        int i = atoi(key + 4);
        const char *u = strchr(key + 4, '_');
        if (i < 0 || i >= DKU_MAX_PROPS || !u) return 0;
        const Prop *p = &dku_g.pr_[i];
        if (!strcmp(u, "_alive")) { *out = p->alive; return 1; }
        if (!strcmp(u, "_kind")) { *out = p->kind; return 1; }
        if (!strcmp(u, "_x")) { *out = dku_px(p->x); return 1; }
        if (!strcmp(u, "_y")) { *out = dku_px(p->y); return 1; }
        if (!strcmp(u, "_hp")) { *out = p->hp; return 1; }
        return 0;
    }
    if (!strncmp(key, "foe", 3) && isdigit((unsigned char)key[3])) {
        int i = atoi(key + 3);
        const char *u = strchr(key + 3, '_');
        if (i < 0 || i >= DKU_MAX_ACTORS || !u) return 0;
        const Actor *f = &dku_g.a[i];
        if (!strcmp(u, "_alive")) { *out = f->alive && f->state != AS_DEAD && f->state != AS_FALL; return 1; }
        if (!strcmp(u, "_hp")) { *out = f->alive ? f->hp : 0; return 1; }
        if (!strcmp(u, "_x")) { *out = dku_px(f->x); return 1; }
        if (!strcmp(u, "_y")) { *out = dku_px(f->y); return 1; }
        if (!strcmp(u, "_z")) { *out = dku_px(f->z); return 1; }
        if (!strcmp(u, "_state")) { *out = f->state; return 1; }
        if (!strcmp(u, "_kind")) { *out = f->kind; return 1; }
        if (!strcmp(u, "_stun")) { *out = f->stun; return 1; }
        if (!strcmp(u, "_team")) { *out = f->team; return 1; }
        if (!strcmp(u, "_mode")) { *out = f->mode; return 1; }
        if (!strcmp(u, "_face")) { *out = f->face; return 1; }
        return 0;
    }
    if (key[0] == 'p' && (key[1] == '1' || key[1] == '2') && key[2] == '_') {
        int pi = key[1] - '1';
        const Actor *p = &dku_g.a[pi];
        const char *u = key + 3;
        if (!strcmp(u, "x")) { *out = dku_px(p->x); return 1; }
        if (!strcmp(u, "y")) { *out = dku_px(p->y); return 1; }
        if (!strcmp(u, "z")) { *out = dku_px(p->z); return 1; }
        if (!strcmp(u, "hp")) { *out = p->alive ? p->hp : 0; return 1; }
        if (!strcmp(u, "state")) { *out = p->state; return 1; }
        if (!strcmp(u, "atk")) { *out = p->state == AS_ATTACK || p->state == AS_AIR || p->state == AS_SPIN ? p->atk : 0; return 1; }
        if (!strcmp(u, "combo")) { *out = p->combo; return 1; }
        if (!strcmp(u, "face")) { *out = p->face; return 1; }
        if (!strcmp(u, "running")) { *out = p->running; return 1; }
        if (!strcmp(u, "charged")) { *out = p->charged; return 1; }
        if (!strcmp(u, "carry")) { *out = p->carry >= 0 ? dku_g.it[p->carry].kind : 0; return 1; }
        if (!strcmp(u, "ammo")) { *out = p->carry >= 0 ? dku_g.it[p->carry].ammo : 0; return 1; }
        if (!strcmp(u, "partner")) { *out = p->partner; return 1; }
        if (!strcmp(u, "alive")) { *out = p->alive && p->state != AS_DEAD && p->state != AS_GONE && p->state != AS_FALL; return 1; }
        if (!strcmp(u, "pick")) { *out = dku_g.pr[pi].pick; return 1; }
        if (!strcmp(u, "power")) { *out = dku_g.pr[pi].stat[DK_POWER]; return 1; }
        if (!strcmp(u, "recov")) { *out = dku_g.pr[pi].stat[DK_RECOV]; return 1; }
        if (!strcmp(u, "tough")) { *out = dku_g.pr[pi].stat[DK_TOUGH]; return 1; }
        if (!strcmp(u, "throw")) { *out = dku_g.pr[pi].stat[DK_THROW]; return 1; }
        if (!strcmp(u, "air_jump")) { *out = p->air_jump; return 1; }
        if (!strcmp(u, "down_t")) { *out = p->down_t; return 1; }
        return 0;
    }
    /* the structure, read from the nights' scripts */
    if (!strcmp(key, "n_nights")) { *out = DKU_NIGHTS; return 1; }
    if (!strncmp(key, "secs_", 5)) { int n = atoi(key + 5) - 1; *out = n >= 0 && n < DKU_NIGHTS ? DKU_NIGHT[n].nsec : 0; return 1; }
    if (!strncmp(key, "boss_of_", 8)) {
        int n = atoi(key + 8) - 1;
        *out = -1;
        if (n < 0 || n >= DKU_NIGHTS) return 1;
        for (int si = 0; si < DKU_NIGHT[n].nsec; si++)
            for (int k = 0; k < DKU_NIGHT[n].sec[si].nev; k++)
                if (DKU_NIGHT[n].sec[si].ev[k].op == EV_BOSS) *out = DKU_NIGHT[n].sec[si].ev[k].a;
        return 1;
    }
    if (!strcmp(key, "lift_waves")) {
        *out = 0;
        for (int n = 0; n < DKU_NIGHTS; n++)
            for (int si = 0; si < DKU_NIGHT[n].nsec; si++)
                for (int k = 0; k < DKU_NIGHT[n].sec[si].nev; k++)
                    if (DKU_NIGHT[n].sec[si].ev[k].op == EV_WAVE) *out = imax(*out, DKU_NIGHT[n].sec[si].ev[k].a);
        return 1;
    }
    if (!strcmp(key, "enemy_kinds")) {
        /* the kinds of ghoul in the nights (not the bosses' own kinds, not dogs or passers-by) */
        bool seen[AK_COUNT] = {0};
        for (int n = 0; n < DKU_NIGHTS; n++)
            for (int si = 0; si < DKU_NIGHT[n].nsec; si++)
                for (int k = 0; k < DKU_NIGHT[n].sec[si].nev; k++) {
                    const DkuEvt *e = &DKU_NIGHT[n].sec[si].ev[k];
                    if (e->op == EV_SPAWN || e->op == EV_STREAM) seen[e->a] = true;
                    if (e->op == EV_WAVE) seen[e->b] = true;
                }
        *out = 0;
        for (int k = AK_SHAMBLER; k <= AK_FEELER; k++) *out += seen[k];
        return 1;
    }
    if (!strcmp(key, "price_heal")) { *out = DKU_PRICE_HEAL; return 1; }
    if (!strcmp(key, "price_stat")) { *out = DKU_PRICE_STAT; return 1; }
    if (!strncmp(key, "fighter_", 8)) {
        /* fighter_F_S: fighter F's stat S as it starts */
        int f = atoi(key + 8);
        const char *u = strchr(key + 8, '_');
        if (f < 0 || f >= DK_NFIGHTERS || !u) return 0;
        *out = DKU_FIGHTERS[f].stat[iclamp(atoi(u + 1), 0, 3)];
        return 1;
    }
    if (!strcmp(key, "save_runs")) { *out = dku_sv.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = dku_sv.wins; return 1; }
    if (!strcmp(key, "save_good")) { *out = dku_sv.good_wins; return 1; }
    if (!strcmp(key, "save_gym1")) { *out = dku_sv.gym_best1; return 1; }
    if (!strcmp(key, "save_gym2")) { *out = dku_sv.gym_best2; return 1; }
    if (!strcmp(key, "save_night")) { *out = dku_sv.best_night; return 1; }
    return 0;
}

static int kind_by_name(const char *s) {
    for (int k = 0; k < AK_COUNT; k++) {
        char lower[24];
        snprintf(lower, sizeof lower, "%s", DKU_KINDS[k].name);
        for (char *c = lower; *c; c++) *c = (char)tolower((unsigned char)*c);
        if (!strcmp(lower, s)) return k;
    }
    return isdigit((unsigned char)s[0]) ? atoi(s) : -1;
}

static int dku_cheat(const char *cmd) {
    int a, b, c;
    char name[32];
    if (sscanf(cmd, "pick %d %d", &a, &b) == 2) { dku_g.pr[a == 2 ? 1 : 0].pick = iclamp(b, 0, 3); dku_g.sel[a == 2 ? 1 : 0] = iclamp(b, 0, 3); return 1; }
    if (sscanf(cmd, "run %d", &a) == 1) {
        /* straight into night 1 with the picks as they are */
        dku_g.menu = a == 2 ? 1 : 0;
        begin_run();
        set_state(DS_PLAY);
        game_set_pausable(true);
        return 1;
    }
    if (sscanf(cmd, "night %d %d", &a, &b) == 2) {
        dku_g.night = iclamp(a, 1, DKU_NIGHTS) - 1;
        dku_start_night(dku_g.night);
        if (b > 0) {
            const DkuNight *n = &DKU_NIGHT[dku_g.night];
            int s = iclamp(b, 0, n->nsec - 1);
            for (int k = 0; k < s; k++) dku_start_section(k + 1);
        }
        set_state(DS_PLAY);
        game_set_pausable(true);
        music_play(night_music());
        return 1;
    }
    if (!strcmp(cmd, "gym")) { dku_start_gym(); set_state(DS_PLAY); return 1; }
    if (!strcmp(cmd, "arena")) {
        /* an empty floor for the tests: nobody else, nothing scripted, the view held */
        for (int i = 2; i < DKU_MAX_ACTORS; i++) dku_g.a[i].alive = 0;
        memset(dku_g.it, 0, sizeof dku_g.it);
        memset(dku_g.pr_, 0, sizeof dku_g.pr_);
        memset(dku_g.sh, 0, sizeof dku_g.sh);
        memset(dku_g.hz, 0, sizeof dku_g.hz);
        memset(dku_g.ev_done, 1, sizeof dku_g.ev_done);
        dku_g.cam_lock = dku_g.cam;
        dku_g.stream_kind = -1;
        dku_g.boss = -1;
        for (int p = 0; p < 2; p++) {
            Actor *f = &dku_g.a[p];
            f->carry = f->partner = -1;
            if (f->alive && f->state != AS_DEAD) { dku_set_state(f, AS_FREE); f->z = f->vz = f->vx = f->vy = 0; }
        }
        return 1;
    }
    if (!strcmp(cmd, "god")) { dku_g.god = true; return 1; }
    if (!strcmp(cmd, "mortal")) { dku_g.god = false; return 1; }
    if (!strcmp(cmd, "freeze")) { dku_g.frozen = true; return 1; }
    if (!strcmp(cmd, "thaw")) { dku_g.frozen = false; return 1; }
    if (!strcmp(cmd, "clear")) { dku_clear_enemies(); return 1; }
    if (!strcmp(cmd, "noprops")) { memset(dku_g.pr_, 0, sizeof dku_g.pr_); return 1; }
    if (!strcmp(cmd, "nohazards")) { memset(dku_g.hz, 0, sizeof dku_g.hz); return 1; }
    if (!strcmp(cmd, "noitems")) { memset(dku_g.it, 0, sizeof dku_g.it); return 1; }
    if (sscanf(cmd, "cash %d", &a) == 1) { dku_g.cash = a; return 1; }
    if (sscanf(cmd, "hp %d %d", &a, &b) == 2) { dku_g.a[a == 2 ? 1 : 0].hp = b; return 1; }
    if (sscanf(cmd, "stat %d %d %d", &a, &b, &c) == 3) { dku_g.pr[a == 2 ? 1 : 0].stat[iclamp(b, 0, 3)] = (uint8_t)iclamp(c, 1, 3); return 1; }
    if (sscanf(cmd, "pos %d %d %d", &a, &b, &c) == 3) {
        Actor *p = &dku_g.a[a == 2 ? 1 : 0];
        p->x = dku_fx(b);
        p->y = dku_fx(c);
        return 1;
    }
    if (sscanf(cmd, "face %d %d", &a, &b) == 2) { dku_g.a[a == 2 ? 1 : 0].face = b < 0 ? -1 : 1; return 1; }
    if (sscanf(cmd, "cam %d", &a) == 1) { dku_g.cam = a; return 1; }
    if (sscanf(cmd, "spawn %31s %d %d", name, &b, &c) == 3) {
        int k = kind_by_name(name);
        if (k < 0) return 0;
        int i = dku_spawn(k, FROM_AT, b, c, 0);
        if (i >= 0) { dku_g.a[i].cool = 60; }
        return 1;
    }
    if (sscanf(cmd, "dormant %31s %d %d %d", name, &a, &b, &c) == 4) {
        int k = kind_by_name(name);
        if (k < 0) return 0;
        dku_spawn(k, FROM_AT, b, c, a);
        return 1;
    }
    if (sscanf(cmd, "boss %31s %d %d", name, &b, &c) == 3) {
        int k = kind_by_name(name);
        if (k < 0) return 0;
        int i = dku_spawn(k, FROM_AT, b, c, 0);
        if (i >= 0) {
            dku_g.a[i].boss = 1;
            static const int H[AK_COUNT] = {[AK_RAMMER] = 90, [AK_TUSKER] = 120, [AK_VISITOR] = 40, [AK_UNDERTOW] = 48, [AK_GRIST] = 110};
            if (H[k]) dku_g.a[i].hp = dku_g.a[i].maxhp = H[k];
            dku_g.boss = i;
        }
        return 1;
    }
    if (sscanf(cmd, "spawnfrom %31s %d %d %d", name, &a, &b, &c) == 4) {
        int k = kind_by_name(name);
        if (k < 0) return 0;
        dku_spawn(k, a, b, c, 0);
        return 1;
    }
    if (sscanf(cmd, "foeslam %d", &a) == 1) {
        /* the body slam at the first fighter, now */
        if (a >= 2 && a < DKU_MAX_ACTORS && dku_g.a[a].alive) {
            Actor *f = &dku_g.a[a];
            dku_set_state(f, AS_SLAM);
            f->tx = dku_g.a[0].x;
            f->ty = dku_g.a[0].y;
            f->vz = 56;
            f->z = 1;
            f->vx = (dku_g.a[0].x - f->x) / 28;
            f->vy = (dku_g.a[0].y - f->y) / 28;
            f->timer2 = -1;
        }
        return 1;
    }
    if (sscanf(cmd, "foestrike %d", &a) == 1) {
        /* its ordinary blow, at once, the way it faces */
        if (a >= 2 && a < DKU_MAX_ACTORS && dku_g.a[a].alive) dku_start_attack(&dku_g.a[a], AT_E_HIT);
        return 1;
    }
    if (sscanf(cmd, "foeface %d %d", &a, &b) == 2) { if (a >= 2 && a < DKU_MAX_ACTORS) dku_g.a[a].face = b < 0 ? -1 : 1; return 1; }
    if (sscanf(cmd, "foehp %d %d", &a, &b) == 2) { if (a >= 2 && a < DKU_MAX_ACTORS) dku_g.a[a].hp = b; return 1; }
    if (sscanf(cmd, "foecool %d %d", &a, &b) == 2) { if (a >= 2 && a < DKU_MAX_ACTORS) dku_g.a[a].cool = b; return 1; }
    if (sscanf(cmd, "foepos %d %d %d", &a, &b, &c) == 3) { if (a >= 2 && a < DKU_MAX_ACTORS) { dku_g.a[a].x = dku_fx(b); dku_g.a[a].y = dku_fx(c); } return 1; }
    if (sscanf(cmd, "item %31s %d %d", name, &b, &c) == 3) {
        int k = -1;
        for (int i = 1; i < IT_COUNT; i++) {
            char lower[24];
            snprintf(lower, sizeof lower, "%s", DKU_ITEMS[i].name);
            for (char *ch = lower; *ch; ch++) *ch = *ch == ' ' ? '_' : (char)tolower((unsigned char)*ch);
            if (!strcmp(lower, name)) k = i;
        }
        if (k < 0 && isdigit((unsigned char)name[0])) k = atoi(name);
        if (k <= 0 || k >= IT_COUNT) return 0;
        int i = dku_drop_item(k, b, c);
        if (i >= 0) { dku_g.it[i].z = 0; dku_g.it[i].vz = 0; }
        return 1;
    }
    if (sscanf(cmd, "prop %d %d %d %d", &a, &b, &c, &(int){0}) >= 3) {
        int content = 0;
        sscanf(cmd, "prop %*d %*d %*d %d", &content);
        for (int i = 0; i < DKU_MAX_PROPS; i++) {
            Prop *p = &dku_g.pr_[i];
            if (p->alive) continue;
            memset(p, 0, sizeof *p);
            p->alive = 1;
            p->kind = (uint8_t)iclamp(a, 0, PR_COUNT - 1);
            p->x = dku_fx(b);
            p->y = dku_fx(c);
            p->content = (uint8_t)content;
            p->hp = (p->kind == PR_CRATE || p->kind == PR_STUMP || p->kind == PR_JUNK) ? 2 : 1;
            break;
        }
        return 1;
    }
    if (sscanf(cmd, "hazard %d %d %d %d %d", &a, &b, &c, &(int){0}, &(int){0}) >= 3) {
        int w = 20, h = 10;
        sscanf(cmd, "hazard %*d %*d %*d %d %d", &w, &h);
        if (a == HZ_LAMP) { dku_add_hazard(HZ_LAMP, b, c, 36, 12, 0, 1); return 1; }
        if (a == HZ_MINE) { dku_add_hazard(HZ_MINE, b - 5, c - 3, 10, 6, 0, 0); return 1; }
        if (a == HZ_CAR) { dku_add_hazard(HZ_CAR, dku_view_left() - 70, c, 56, 14, 0, 0); return 1; }
        if (a == HZ_THRESHER) { dku_add_hazard(HZ_THRESHER, b, DKU_FLOOR0, 64, DKU_FLOOR1 - DKU_FLOOR0, 0, b + 2000); return 1; }
        dku_add_hazard(a, b, c, w, h, a == HZ_FIRE || a == HZ_CLOUD ? 600 : 0, a == HZ_CLOUD ? 2 : 0);
        return 1;
    }
    if (sscanf(cmd, "shot %d %d %d %d", &a, &b, &c, &(int){0}) >= 3) {
        int vx = -40;
        sscanf(cmd, "shot %*d %*d %*d %d", &vx);
        int kz = a == SH_BOTTLE || a == SH_BOMB ? 30 : 16;
        dku_add_shot(a, 1, dku_fx(b), dku_fx(c), dku_fx(kz), vx, 0, a == SH_BOTTLE || a == SH_BOMB ? 0 : 0, 10, -1);
        return 1;
    }
    if (sscanf(cmd, "killfoe %d", &a) == 1) {
        if (a >= 2 && a < DKU_MAX_ACTORS && dku_g.a[a].alive) { dku_g.a[a].hp = 1; dku_hit(a, 0, AT_NONE, 5, 1, 64 | 256); }
        return 1;
    }
    if (!strcmp(cmd, "killboss")) {
        if (dku_g.boss >= 0 && dku_g.a[dku_g.boss].alive) { dku_g.a[dku_g.boss].hp = 1; dku_hit(dku_g.boss, 0, AT_NONE, 5, 1, 64 | 256); }
        return 1;
    }
    if (!strcmp(cmd, "nightdone")) { dku_g.night_done = true; return 1; }
    if (!strcmp(cmd, "die")) {
        dku_g.god = false;
        for (int p = 0; p < dku_g.players; p++) if (dku_g.a[p].alive) dku_hurt_fighter(p, 999, false, 1, true, -9);
        return 1;
    }
    if (sscanf(cmd, "botplan %d", &a) == 1) { dku_bot_plan = a; return 1; }
    if (sscanf(cmd, "menu %d", &a) == 1) { dku_g.menu = a ? 1 : 0; return 1; }
    if (sscanf(cmd, "continues %d", &a) == 1) { dku_g.continues = a; return 1; }
    if (sscanf(cmd, "gymwave %d", &a) == 1) { dku_g.gym_wave = a; return 1; }
    return 0;
}

const GameDef GAME_DUKESUP = {
    "dukesup",
    "DUKES UP",
    "1987",
    "BRAWLER",
    "FIVE NIGHTS OF GHOULS, ONE HEALTH BAR. FOUR PUNCHES, A BREATH, FOUR MORE.",
    {"HOLD OUT FOR NINE WAVES IN THE GYM", "PUNCH YOUR WAY UP TO THE PENTHOUSE", "MAKE IT TO THE TOP WITHOUT A CONTINUE"},
    "D-PAD\tWALK; TAP TWICE: RUN / DODGE\n"
    GLYPH_A "\tPUNCH (5TH: KICK); PICK UP; USE\n"
    "HOLD " GLYPH_A "\tCHARGED PUNCH ON RELEASE\n"
    GLYPH_B "\tJUMP; " GLYPH_A " IN THE AIR: KICK\n"
    GLYPH_A "+" GLYPH_B "\tSPIN (COSTS HEALTH IF IT HITS)\n"
    "GRAB\tWALK INTO A STUNNED GHOUL\n"
    "  " GLYPH_A " HIT, DIR+" GLYPH_A " THROW, " GLYPH_B " SLAM\n"
    "START\tPAUSE",
    C_MAROON, C_YELLOW,
    dku_load, dku_start, dku_update, dku_draw, dku_quit, dku_draw_label, dku_query, dku_cheat,
    "FIST HELL", 33,
    NULL,
};
