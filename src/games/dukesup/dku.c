/* DUKES UP - Rook, Pip, Mack and Dolly punch their way up to the
 * penthouse. Cartridge 33 of UFO 40, a tribute to Fist Hell (UFO 50 #33).
 * See docs/games/33-dukes-up.md. This file: the title, the story, the
 * fighter select, the night cards, the corner shop, the continue screen,
 * the endings, the records, the save and the test hooks. A night itself is
 * in dku_world.c, the fighters in dku_play.c, the ghouls in dku_foes.c. */
#include "dku.h"

#define SAVE_MAGIC 0x44554B31u

DkuGame dk;
DkuSave dks;

static bool vita_single(void) { return plat_kind() == PLAT_VITA; }

void dku_sfx(const char *name) { sfx_play_name(name); }

static void set_state(int s) {
    dk.state = s;
    dk.state_t = 0;
}

void dku_save_now(void) { game_save_write(game_current_index(), &dks, (int)sizeof dks); }

static void load_save(void) {
    DkuSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { dks = tmp; return; }
    memset(&dks, 0, sizeof dks);
    dks.magic = SAVE_MAGIC;
    dks.fewest_continues = 0xFFFF;
}

/* the goals come back from the records (RESET GOALS clears both) */
static void goals_from_save(void) {
    if (dks.gym_best1 >= DKU_GYM_GIFT || dks.gym_best2 >= DKU_GYM_GIFT) game_award(GOAL_BEACON);
    if (dks.wins > 0) game_award(GOAL_SAUCER);
    if (dks.good_wins > 0) game_award(GOAL_ALIEN);
}

void dku_award_gym(void) { game_award(GOAL_BEACON); }

static int night_music(void) {
    static int *const M[DKU_NIGHTS] = {&DKU_MUS_N1, &DKU_MUS_N2, &DKU_MUS_N3, &DKU_MUS_N4, &DKU_MUS_N5};
    const DkuSection *s = &DKU_NIGHT[dk.night].sec[dk.sec];
    if (s->kind == SEC_LIFT) return DKU_MUS_LIFT;
    if (s->kind == SEC_BOSS) return dk.night == DKU_NIGHTS - 1 ? DKU_MUS_GRIST : DKU_MUS_BOSS;
    return *M[dk.night];
}

static void to_title(void) {
    set_state(DS_TITLE);
    game_set_pausable(false);
    input_set_versus(false);
    dk.gym = false;
    music_play(DKU_MUS_TITLE);
}

static void run_over(void) {
    if (dk.night + 1 > dks.best_night && !dk.gym) dks.best_night = (uint16_t)(dk.night + 1);
    dks.total_kos += (uint32_t)dk.kos;
    dku_save_now();
}

static void start_card(void) {
    set_state(DS_CARD);
    dk.dead_t = 0;
    game_set_pausable(false);
    music_stop();
}

static void begin_run(void) {
    dku_new_run(dk.menu == 1 ? 2 : 1);
    input_set_versus(dk.players == 2);
    if (dks.runs < 65535) dks.runs++;
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
    Actor *a = &dk.a[who];
    if (row == SHOP_HEAL) {
        if (a->hp >= DKU_MAX_HP) { dk.shop_msg = "ALREADY IN ONE PIECE."; dk.shop_msg_t = 90; return false; }
    } else if (dk.pr[who].stat[shop_stat(row)] >= 3) {
        dk.shop_msg = "THAT ONE'S MAXED OUT.";
        dk.shop_msg_t = 90;
        return false;
    }
    if (dk.cash < price) { dk.shop_msg = "NOT ENOUGH CASH."; dk.shop_msg_t = 90; return false; }
    dk.cash -= price;
    if (row == SHOP_HEAL) a->hp = DKU_MAX_HP;
    else dk.pr[who].stat[shop_stat(row)]++;
    dk.shop_msg = "THANKS, COME AGAIN!";
    dk.shop_msg_t = 60;
    return true;
}

static void open_shop(void) {
    set_state(DS_SHOP);
    dk.shop_sel = 0;
    dk.shop_who = 0;
    while (dk.shop_who < dk.players && (!dk.a[dk.shop_who].alive)) dk.shop_who++;
    dk.shop_msg = NULL;
    music_play(DKU_MUS_SHOP);
    game_set_pausable(true);
}

static void shop_update(void) {
    if (dk.shop_msg_t > 0) dk.shop_msg_t--;
    bool up = dk.shop_who == 0 ? btn_repeat(BTN_UP) : btn_repeat2(BTN_UP);
    bool down = dk.shop_who == 0 ? btn_repeat(BTN_DOWN) : btn_repeat2(BTN_DOWN);
    bool a = dk.shop_who == 0 ? btnp(BTN_A) : btnp2(BTN_A);
    if (up) { dk.shop_sel = (dk.shop_sel + SHOP_ROWS - 1) % SHOP_ROWS; sfx_play_name("ui_move"); }
    if (down) { dk.shop_sel = (dk.shop_sel + 1) % SHOP_ROWS; sfx_play_name("ui_move"); }
    if (!a) return;
    if (dk.shop_sel == SHOP_GO) {
        if (dk.players == 2 && dk.shop_who == 0) {
            /* player two's turn at the counter */
            dk.shop_who = 1;
            dk.shop_sel = 0;
            sfx_play_name("ui_ok");
            return;
        }
        sfx_play_name("ui_ok");
        input_consume();
        dku_start_night(dk.night + 1);
        start_card();
        return;
    }
    if (shop_buy(dk.shop_who, dk.shop_sel)) sfx_play_name("dku_buy");
    else sfx_play_name("ui_error");
}

/* ---- the continue screen --------------------------------------------------------- */


static void start_continue(void) {
    set_state(DS_CONTINUE);
    dk.cont_sel = 0;
    dk.cont_t = 10 * 60;
    dk.tip = (dk.continues + dk.night * 3 + dk.kos) % DKU_NTIPS;
    game_set_pausable(false);
    music_play(DKU_MUS_DOWN);
}

static void continue_update(void) {
    if (dk.state_t < 30) return;
    if (btnp(BTN_LEFT) || btnp(BTN_RIGHT)) { dk.cont_sel ^= 1; sfx_play_name("ui_move"); }
    if (--dk.cont_t <= 0) dk.cont_sel = 1;
    if (btnp(BTN_A) || btnp(BTN_START) || dk.cont_t <= 0) {
        input_consume();
        if (dk.cont_sel == 0) {
            /* back to the start of the night: cash and stats stay */
            dk.continues++;
            for (int p = 0; p < dk.players; p++) {
                dk.a[p].alive = 1;
                dk.a[p].kind = AK_FIGHTER;
                dk.a[p].hp = DKU_MAX_HP;
                dk.a[p].state = AS_FREE;
            }
            dku_start_night(dk.night);
            start_card();
        } else {
            set_state(DS_OVER);
            music_play(DKU_MUS_DOWN);
        }
    }
}

/* ---- the run ends ----------------------------------------------------------------- */

static void win_run(void) {
    dk.won = true;
    if (dks.wins < 65535) dks.wins++;
    if (dk.continues == 0 && dks.good_wins < 65535) dks.good_wins++;
    if ((uint32_t)dk.continues < dks.fewest_continues) dks.fewest_continues = (uint16_t)imin(65534, dk.continues);
    game_award(GOAL_SAUCER);
    if (dk.continues == 0) game_award(GOAL_ALIEN);
    dks.best_night = DKU_NIGHTS;
    run_over();
    set_state(DS_ENDING);
    game_set_pausable(false);
    music_play(dk.continues == 0 ? DKU_MUS_GOOD : DKU_MUS_BAD);
}

static void dku_update(void) {
    dk.state_t++;
    dk.frame_t++;
    switch (dk.state) {
    case DS_TITLE:
        game_set_pausable(false);
        if (btnp(BTN_UP) || btnp(BTN_DOWN)) { dk.menu ^= 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if ((btnp(BTN_A) || btnp(BTN_START)) && dk.state_t > 5) {
            if (dk.menu == 1 && vita_single()) { sfx_play_name("ui_error"); break; }
            sfx_play_name("ui_ok");
            set_state(DS_STORY);
        }
        break;
    case DS_STORY:
        if ((dk.state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) || dk.state_t > 1200) {
            input_consume();
            set_state(DS_SELECT);
            dk.picked[0] = dk.picked[1] = false;
            dk.players = dk.menu == 1 ? 2 : 1;
            input_set_versus(dk.players == 2);
            music_play(DKU_MUS_SELECT);
        }
        if (btnp(BTN_B)) to_title();
        break;
    case DS_SELECT: {
        for (int p = 0; p < dk.players; p++) {
            bool l = p == 0 ? btnp(BTN_LEFT) : btnp2(BTN_LEFT);
            bool r = p == 0 ? btnp(BTN_RIGHT) : btnp2(BTN_RIGHT);
            bool a = p == 0 ? btnp(BTN_A) : btnp2(BTN_A);
            bool b = p == 0 ? btnp(BTN_B) : btnp2(BTN_B);
            if (!dk.picked[p]) {
                if (l) { dk.sel[p] = (dk.sel[p] + DK_NFIGHTERS - 1) % DK_NFIGHTERS; sfx_play_name("ui_move"); }
                if (r) { dk.sel[p] = (dk.sel[p] + 1) % DK_NFIGHTERS; sfx_play_name("ui_move"); }
                if (a && dk.state_t > 10) { dk.picked[p] = true; dk.pr[p].pick = dk.sel[p]; sfx_play_name("ui_ok"); }
                if (b && p == 0) { to_title(); return; }
            } else if (b) {
                dk.picked[p] = false;
                sfx_play_name("ui_back");
            }
        }
        bool all = true;
        for (int p = 0; p < dk.players; p++) all &= dk.picked[p];
        if (all) {
            input_consume();
            begin_run();
        }
        break;
    }
    case DS_CARD:
        if (dk.state_t > 110 || (dk.state_t > 20 && btnp(BTN_A))) {
            input_consume();
            set_state(DS_PLAY);
            game_set_pausable(true);
            music_play(night_music());
        }
        break;
    case DS_PLAY: {
        game_set_pausable(true);
        int sec_before = dk.sec;
        bool gym_before = dk.gym;
        dku_world_update();
        if (dk.gym != gym_before || dk.sec != sec_before) {
            if (!dk.gym) music_play(night_music());
        }
        if (dku_fighters_alive() == 0) {
            if (++dk.dead_t > 70) {
                if (dk.gym) {
                    set_state(DS_GYMOVER);
                    music_play(DKU_MUS_DOWN);
                    dku_save_now();
                } else {
                    start_continue();
                }
            }
            break;
        }
        if (dk.night_done) {
            dk.night_done = false;
            dk.clear_cash = dk.cash;
            if (dk.night + 1 > dks.best_night) dks.best_night = (uint16_t)(dk.night + 1);
            dku_save_now();
            set_state(DS_CLEAR);
            music_play(DKU_MUS_CLEAR);
            game_set_pausable(false);
        }
        break;
    }
    case DS_CLEAR:
        if (dk.state_t > 60 && (btnp(BTN_A) || btnp(BTN_START) || (dk.players == 2 && btnp2(BTN_A)))) {
            input_consume();
            if (dk.night == DKU_NIGHTS - 1) win_run();
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
        if (dk.state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            run_over();
            to_title();
        }
        break;
    case DS_ENDING:
        if (dk.state_t > 150 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            set_state(DS_CREDITS);
        }
        break;
    case DS_CREDITS:
        if (btn(BTN_A)) dk.state_t += 3;
        if (dk.state_t > 1300) {
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
    memset(&dk, 0, sizeof dk);
    rng_seed(&dk.rng, (uint64_t)rng_next(&g_rng) + 33u);
    dk.boss = -1;
    dk.cam_lock = -1;
    goals_from_save();
    to_title();
}

static void dku_quit(void) {
    input_set_versus(false);
    dku_save_now();
}

static int count_kind(int k) {
    int n = 0;
    for (int i = 2; i < DKU_MAX_ACTORS; i++) n += dk.a[i].alive && dk.a[i].kind == k && dk.a[i].state != AS_DEAD;
    return n;
}

static int count_items(int k) {
    int n = 0;
    for (int i = 0; i < DKU_MAX_ITEMS; i++) n += dk.it[i].alive == 1 && (k < 0 || dk.it[i].kind == k);
    return n;
}

static int count_hz(int k) {
    int n = 0;
    for (int i = 0; i < DKU_MAX_HAZARDS; i++) n += dk.hz[i].alive && dk.hz[i].kind == k;
    return n;
}

static int dku_query(const char *key, int *out) {
    if (!strcmp(key, "bot")) { *out = dku_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = dk.state; return 1; }
    if (!strcmp(key, "frames")) { *out = dk.frame_t; return 1; }
    if (!strcmp(key, "bot_why")) { extern int dku_bot_why; *out = dku_bot_why; return 1; }
    if (!strcmp(key, "bot_target")) { extern int dku_bot_target; *out = dku_bot_target; return 1; }
    if (!strcmp(key, "menu")) { *out = dk.menu; return 1; }
    if (!strcmp(key, "players")) { *out = dk.players; return 1; }
    if (!strcmp(key, "night")) { *out = dk.night + 1; return 1; }
    if (!strcmp(key, "sec")) { *out = dk.sec; return 1; }
    if (!strcmp(key, "cam")) { *out = dk.cam; return 1; }
    if (!strcmp(key, "cam_lock")) { *out = dk.cam_lock; return 1; }
    if (!strcmp(key, "cash")) { *out = dk.cash; return 1; }
    if (!strcmp(key, "continues")) { *out = dk.continues; return 1; }
    if (!strcmp(key, "won")) { *out = dk.won; return 1; }
    if (!strcmp(key, "gym")) { *out = dk.gym; return 1; }
    if (!strcmp(key, "gym_wave")) { *out = dk.gym_wave; return 1; }
    if (!strcmp(key, "gym_beaten")) { *out = dk.gym ? dk.gym_wave - 1 : 0; return 1; }
    if (!strcmp(key, "kos")) { *out = dk.kos; return 1; }
    if (!strcmp(key, "kick_kills")) { *out = dk.kick_kills; return 1; }
    if (!strcmp(key, "enemies")) { *out = dku_enemies_alive(); return 1; }
    if (!strcmp(key, "awake")) { *out = dku_enemies_awake(); return 1; }
    if (!strcmp(key, "exit")) { *out = dk.exit_t; return 1; }
    if (!strcmp(key, "fade")) { *out = dk.fade; return 1; }
    if (!strcmp(key, "lift_wave")) { *out = dk.lift_wave; return 1; }
    if (!strcmp(key, "sel1")) { *out = dk.sel[0]; return 1; }
    if (!strcmp(key, "sel2")) { *out = dk.sel[1]; return 1; }
    if (!strcmp(key, "shop_sel")) { *out = dk.shop_sel; return 1; }
    if (!strcmp(key, "shop_who")) { *out = dk.shop_who; return 1; }
    if (!strcmp(key, "cont_sel")) { *out = dk.cont_sel; return 1; }
    if (!strcmp(key, "last_dmg")) { *out = dk.last_dmg; return 1; }
    if (!strncmp(key, "dmg_by", 6)) { int k = atoi(key + 6) + 10; *out = k >= 0 && k < 32 ? dk.dmg_by[k] : 0; return 1; }
    if (!strcmp(key, "boss_alive")) { *out = dk.boss >= 0 && dk.a[dk.boss].alive && dk.a[dk.boss].state != AS_DEAD; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = dk.boss >= 0 && dk.a[dk.boss].alive ? dk.a[dk.boss].hp : 0; return 1; }
    if (!strcmp(key, "boss_mode")) { *out = dk.boss >= 0 ? dk.a[dk.boss].mode : -1; return 1; }
    if (!strcmp(key, "boss_state")) { *out = dk.boss >= 0 ? dk.a[dk.boss].state : -1; return 1; }
    if (!strcmp(key, "boss_down")) { *out = dk.boss_down; return 1; }
    if (!strcmp(key, "dogs")) { *out = 0; for (int i = 2; i < DKU_MAX_ACTORS; i++) *out += dk.a[i].alive && dk.a[i].kind == AK_DOG && dk.a[i].team == 0; return 1; }
    if (!strcmp(key, "items")) { *out = count_items(-1); return 1; }
    if (!strncmp(key, "items_", 6)) { *out = count_items(atoi(key + 6)); return 1; }
    if (!strncmp(key, "kind_", 5)) { *out = count_kind(atoi(key + 5)); return 1; }
    if (!strncmp(key, "hz_", 3)) { *out = count_hz(atoi(key + 3)); return 1; }
    if (!strcmp(key, "props")) { *out = 0; for (int i = 0; i < DKU_MAX_PROPS; i++) *out += dk.pr_[i].alive; return 1; }
    if (!strcmp(key, "shots")) { *out = 0; for (int i = 0; i < DKU_MAX_SHOTS; i++) *out += dk.sh[i].alive; return 1; }
    if (!strcmp(key, "shot0_team")) { *out = -1; for (int i = 0; i < DKU_MAX_SHOTS; i++) if (dk.sh[i].alive) { *out = dk.sh[i].team; break; } return 1; }
    if (!strcmp(key, "shot0_vx")) { *out = 0; for (int i = 0; i < DKU_MAX_SHOTS; i++) if (dk.sh[i].alive) { *out = dk.sh[i].vx; break; } return 1; }
    if (!strcmp(key, "layout")) { *out = dku_layout_audit(); return 1; }
    if (!strcmp(key, "vita_single")) { *out = vita_single(); return 1; }
    if (!strncmp(key, "item", 4) && isdigit((unsigned char)key[4])) {
        int i = atoi(key + 4);
        const char *u = strchr(key + 4, '_');
        if (i < 0 || i >= DKU_MAX_ITEMS || !u) return 0;
        const Item *it = &dk.it[i];
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
        const Prop *p = &dk.pr_[i];
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
        const Actor *f = &dk.a[i];
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
        const Actor *p = &dk.a[pi];
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
        if (!strcmp(u, "carry")) { *out = p->carry >= 0 ? dk.it[p->carry].kind : 0; return 1; }
        if (!strcmp(u, "ammo")) { *out = p->carry >= 0 ? dk.it[p->carry].ammo : 0; return 1; }
        if (!strcmp(u, "partner")) { *out = p->partner; return 1; }
        if (!strcmp(u, "alive")) { *out = p->alive && p->state != AS_DEAD && p->state != AS_GONE && p->state != AS_FALL; return 1; }
        if (!strcmp(u, "pick")) { *out = dk.pr[pi].pick; return 1; }
        if (!strcmp(u, "power")) { *out = dk.pr[pi].stat[DK_POWER]; return 1; }
        if (!strcmp(u, "recov")) { *out = dk.pr[pi].stat[DK_RECOV]; return 1; }
        if (!strcmp(u, "tough")) { *out = dk.pr[pi].stat[DK_TOUGH]; return 1; }
        if (!strcmp(u, "throw")) { *out = dk.pr[pi].stat[DK_THROW]; return 1; }
        if (!strcmp(u, "air_jump")) { *out = p->air_jump; return 1; }
        if (!strcmp(u, "down_t")) { *out = p->down_t; return 1; }
        return 0;
    }
    if (!strcmp(key, "save_runs")) { *out = dks.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = dks.wins; return 1; }
    if (!strcmp(key, "save_good")) { *out = dks.good_wins; return 1; }
    if (!strcmp(key, "save_gym1")) { *out = dks.gym_best1; return 1; }
    if (!strcmp(key, "save_gym2")) { *out = dks.gym_best2; return 1; }
    if (!strcmp(key, "save_night")) { *out = dks.best_night; return 1; }
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
    if (sscanf(cmd, "pick %d %d", &a, &b) == 2) { dk.pr[a == 2 ? 1 : 0].pick = iclamp(b, 0, 3); dk.sel[a == 2 ? 1 : 0] = iclamp(b, 0, 3); return 1; }
    if (sscanf(cmd, "run %d", &a) == 1) {
        /* straight into night 1 with the picks as they are */
        dk.menu = a == 2 ? 1 : 0;
        begin_run();
        set_state(DS_PLAY);
        game_set_pausable(true);
        return 1;
    }
    if (sscanf(cmd, "night %d %d", &a, &b) == 2) {
        dk.night = iclamp(a, 1, DKU_NIGHTS) - 1;
        dku_start_night(dk.night);
        if (b > 0) {
            const DkuNight *n = &DKU_NIGHT[dk.night];
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
        for (int i = 2; i < DKU_MAX_ACTORS; i++) dk.a[i].alive = 0;
        memset(dk.it, 0, sizeof dk.it);
        memset(dk.pr_, 0, sizeof dk.pr_);
        memset(dk.sh, 0, sizeof dk.sh);
        memset(dk.hz, 0, sizeof dk.hz);
        memset(dk.ev_done, 1, sizeof dk.ev_done);
        dk.cam_lock = dk.cam;
        dk.stream_kind = -1;
        dk.boss = -1;
        for (int p = 0; p < 2; p++) {
            Actor *f = &dk.a[p];
            f->carry = f->partner = -1;
            if (f->alive && f->state != AS_DEAD) { dku_set_state(f, AS_FREE); f->z = f->vz = f->vx = f->vy = 0; }
        }
        return 1;
    }
    if (!strcmp(cmd, "god")) { dk.god = true; return 1; }
    if (!strcmp(cmd, "mortal")) { dk.god = false; return 1; }
    if (!strcmp(cmd, "freeze")) { dk.frozen = true; return 1; }
    if (!strcmp(cmd, "thaw")) { dk.frozen = false; return 1; }
    if (!strcmp(cmd, "clear")) { dku_clear_enemies(); return 1; }
    if (!strcmp(cmd, "noprops")) { memset(dk.pr_, 0, sizeof dk.pr_); return 1; }
    if (!strcmp(cmd, "nohazards")) { memset(dk.hz, 0, sizeof dk.hz); return 1; }
    if (!strcmp(cmd, "noitems")) { memset(dk.it, 0, sizeof dk.it); return 1; }
    if (sscanf(cmd, "cash %d", &a) == 1) { dk.cash = a; return 1; }
    if (sscanf(cmd, "hp %d %d", &a, &b) == 2) { dk.a[a == 2 ? 1 : 0].hp = b; return 1; }
    if (sscanf(cmd, "stat %d %d %d", &a, &b, &c) == 3) { dk.pr[a == 2 ? 1 : 0].stat[iclamp(b, 0, 3)] = (uint8_t)iclamp(c, 1, 3); return 1; }
    if (sscanf(cmd, "pos %d %d %d", &a, &b, &c) == 3) {
        Actor *p = &dk.a[a == 2 ? 1 : 0];
        p->x = dku_fx(b);
        p->y = dku_fx(c);
        return 1;
    }
    if (sscanf(cmd, "face %d %d", &a, &b) == 2) { dk.a[a == 2 ? 1 : 0].face = b < 0 ? -1 : 1; return 1; }
    if (sscanf(cmd, "cam %d", &a) == 1) { dk.cam = a; return 1; }
    if (sscanf(cmd, "spawn %31s %d %d", name, &b, &c) == 3) {
        int k = kind_by_name(name);
        if (k < 0) return 0;
        int i = dku_spawn(k, FROM_AT, b, c, 0);
        if (i >= 0) { dk.a[i].cool = 60; }
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
            dk.a[i].boss = 1;
            static const int H[AK_COUNT] = {[AK_RAMMER] = 90, [AK_TUSKER] = 120, [AK_VISITOR] = 40, [AK_UNDERTOW] = 60, [AK_GRIST] = 110};
            if (H[k]) dk.a[i].hp = dk.a[i].maxhp = H[k];
            dk.boss = i;
        }
        return 1;
    }
    if (sscanf(cmd, "foestrike %d", &a) == 1) {
        /* its ordinary blow, at once, the way it faces */
        if (a >= 2 && a < DKU_MAX_ACTORS && dk.a[a].alive) dku_start_attack(&dk.a[a], AT_E_HIT);
        return 1;
    }
    if (sscanf(cmd, "foeface %d %d", &a, &b) == 2) { if (a >= 2 && a < DKU_MAX_ACTORS) dk.a[a].face = b < 0 ? -1 : 1; return 1; }
    if (sscanf(cmd, "foehp %d %d", &a, &b) == 2) { if (a >= 2 && a < DKU_MAX_ACTORS) dk.a[a].hp = b; return 1; }
    if (sscanf(cmd, "foecool %d %d", &a, &b) == 2) { if (a >= 2 && a < DKU_MAX_ACTORS) dk.a[a].cool = b; return 1; }
    if (sscanf(cmd, "foepos %d %d %d", &a, &b, &c) == 3) { if (a >= 2 && a < DKU_MAX_ACTORS) { dk.a[a].x = dku_fx(b); dk.a[a].y = dku_fx(c); } return 1; }
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
        if (i >= 0) { dk.it[i].z = 0; dk.it[i].vz = 0; }
        return 1;
    }
    if (sscanf(cmd, "prop %d %d %d %d", &a, &b, &c, &(int){0}) >= 3) {
        int content = 0;
        sscanf(cmd, "prop %*d %*d %*d %d", &content);
        for (int i = 0; i < DKU_MAX_PROPS; i++) {
            Prop *p = &dk.pr_[i];
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
    if (!strcmp(cmd, "killboss")) {
        if (dk.boss >= 0 && dk.a[dk.boss].alive) { dk.a[dk.boss].hp = 1; dku_hit(dk.boss, 0, AT_NONE, 5, 1, 64 | 256); }
        return 1;
    }
    if (!strcmp(cmd, "nightdone")) { dk.night_done = true; return 1; }
    if (!strcmp(cmd, "die")) {
        dk.god = false;
        for (int p = 0; p < dk.players; p++) if (dk.a[p].alive) dku_hurt_fighter(p, 999, false, 1, true, -9);
        return 1;
    }
    if (sscanf(cmd, "botplan %d", &a) == 1) { dku_bot_plan = a; return 1; }
    if (sscanf(cmd, "menu %d", &a) == 1) { dk.menu = a ? 1 : 0; return 1; }
    if (sscanf(cmd, "continues %d", &a) == 1) { dk.continues = a; return 1; }
    if (sscanf(cmd, "gymwave %d", &a) == 1) { dk.gym_wave = a; return 1; }
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
