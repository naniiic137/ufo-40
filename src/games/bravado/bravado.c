/* BRAVADO - Dice bets her way back to a ship in the Glass Pit.
 * Cartridge 34 of UFO 40, a tribute to Overbold (UFO 50 #34).
 * See docs/games/34-bravado.md. This file: the title, the story, the
 * screens between fights, the shop's buttons, the ending, the records,
 * the save and the test hooks. A fight itself is in bravado_play.c. */
#include "bravado.h"

#define SAVE_MAGIC 0x42525601u
#define BEACON_PRIZE 500
#define CHERRY_CASH 4500

static bool vita_single(void) { return plat_kind() == PLAT_VITA; }

static void set_state(int s) {
    bv.state = s;
    bv.state_t = 0;
}

static void load_save(void) {
    BrvSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { bvs = tmp; return; }
    memset(&bvs, 0, sizeof bvs);
    bvs.magic = SAVE_MAGIC;
}

/* the goals come back from the records (RESET GOALS clears both) */
static void goals_from_save(void) {
    if (bvs.best_fight >= BEACON_PRIZE) game_award(GOAL_BEACON);
    if (bvs.wins > 0) game_award(GOAL_SAUCER);
    if (bvs.best_cash >= CHERRY_CASH) game_award(GOAL_ALIEN);
}

/* the run is over, won or lost: keep its records */
static void run_over(void) {
    if (bv.upgrades > bvs.most_upgrades) bvs.most_upgrades = (uint16_t)imin(65535, bv.upgrades);
    if (bv.kills > bvs.most_kills) bvs.most_kills = (uint16_t)imin(65535, bv.kills);
    bvs.total_kills += (uint32_t)bv.kills;
    if (bv.round + 1 > bvs.best_round) bvs.best_round = (uint16_t)(bv.round + 1);
    brv_save_now();
}

static void to_title(void) {
    set_state(BS_TITLE);
    game_set_pausable(false);
    input_set_versus(false);
    music_play(BRV_MUS_TITLE);
}

static void start_banner(void) {
    brv_start_fight();
    set_state(BS_BANNER);
    game_set_pausable(true);
    music_play(bv.round == BRV_ROUNDS - 1 ? BRV_MUS_LAST : bv.round >= 3 ? BRV_MUS_FIGHT2 : BRV_MUS_FIGHT1);
}

static void open_shop(void) {
    bv.round++;
    brv_new_lineup();
    brv_shop_changes();
    bv.shop_row = 0;
    bv.shop_btn = bv.round == BRV_ROUNDS - 1 ? 2 : 1;
    bv.shop_msg = NULL;
    bv.shop_msg_t = 0;
    set_state(BS_SHOP);
    music_play(BRV_MUS_SHOP);
}

static void win_fight(void) {
    bv.cash += bv.prize;
    if (bv.prize > bvs.best_fight) bvs.best_fight = (uint16_t)imin(65535, bv.prize);
    if (bv.round + 1 > bvs.best_round) bvs.best_round = (uint16_t)(bv.round + 1);
    if (bv.prize >= BEACON_PRIZE) game_award(GOAL_BEACON);
    if (bv.round == BRV_ROUNDS - 1) {
        bv.won = true;
        if (bvs.wins < 65535) bvs.wins++;
        if ((uint32_t)bv.cash > bvs.best_cash) bvs.best_cash = (uint32_t)bv.cash;
        game_award(GOAL_SAUCER);
        if (bv.cash >= CHERRY_CASH) game_award(GOAL_ALIEN);
    }
    brv_save_now();
    /* what's left on the floor goes quiet */
    for (int i = 0; i < BRV_MAX_BOMBS; i++) bv.bomb[i].alive = 0;
    for (int i = 0; i < BRV_MAX_ESHOTS; i++) bv.es[i].alive = 0;
    set_state(BS_WON);
    music_play(BRV_MUS_WIN);
}

static int shop_input(int b) {
    /* either player can work the shop */
    return btnp(b) || (bv.players == 2 && btnp2(b));
}
static int shop_repeat(int b) { return btn_repeat(b) || (bv.players == 2 && btn_repeat2(b)); }

static void shop_say(const char *m) {
    bv.shop_msg = m;
    bv.shop_msg_t = 120;
}

static void shop_update(void) {
    if (bv.shop_msg_t > 0) bv.shop_msg_t--;
    if (bv.raise_flash > 0) bv.raise_flash--;
    bool last = bv.round == BRV_ROUNDS - 1;
    if (bv.shop_row == 0) {
        if (shop_repeat(BTN_LEFT)) { bv.shop_btn = (bv.shop_btn + 2) % 3; sfx_play_name("ui_move"); }
        if (shop_repeat(BTN_RIGHT)) { bv.shop_btn = (bv.shop_btn + 1) % 3; sfx_play_name("ui_move"); }
        if (shop_input(BTN_UP) && bv.shop_btn == 0) { bv.shop_row = 1; sfx_play_name("ui_move"); }
        if (shop_input(BTN_A)) {
            if (bv.shop_btn == 0) {
                bv.shop_row = 1;
                sfx_play_name("ui_ok");
            } else if (bv.shop_btn == 1) {
                if (!last && brv_raise()) {
                    bv.raise_flash = 20;
                    sfx_play_name("brv_raise");
                } else {
                    sfx_play_name("ui_error");
                    shop_say(last ? "THE LAST FIGHT IS ALREADY FULL." : "THE BOOK IS FULL: 16 PACKS IS THE LIMIT.");
                }
            } else {
                sfx_play_name("brv_bell");
                input_consume();
                start_banner();
            }
        }
    } else {
        int s = bv.shop_sel;
        if (shop_repeat(BTN_LEFT)) { s = (s & ~3) | ((s + 3) & 3); sfx_play_name("ui_move"); }
        if (shop_repeat(BTN_RIGHT)) { s = (s & ~3) | ((s + 1) & 3); sfx_play_name("ui_move"); }
        if (shop_repeat(BTN_UP)) { s = (s + 12) & 15; sfx_play_name("ui_move"); }
        if (shop_repeat(BTN_DOWN)) {
            if (s >= 12) { bv.shop_row = 0; bv.shop_btn = 0; }
            else s += 4;
            sfx_play_name("ui_move");
        }
        bv.shop_sel = s;
        if (shop_input(BTN_B)) { bv.shop_row = 0; bv.shop_btn = 0; sfx_play_name("ui_back"); }
        else if (shop_input(BTN_A) && bv.shop_row == 1) {
            if (brv_maxed(s)) { sfx_play_name("ui_error"); shop_say("SOLD OUT."); }
            else if (brv_price(s) > bv.cash) { sfx_play_name("ui_error"); shop_say("NOT ENOUGH CASH."); }
            else {
                brv_buy(s);
                sfx_play_name("brv_buy");
                if (bv.hiked_bought >= 3 && !bv.hike_note) {
                    bv.hike_note = true;
                    shop_say("THE HOUSE THANKS YOU FOR YOUR GENEROSITY!");
                }
            }
        }
    }
}

static void brv_update(void) {
    bv.state_t++;
    bv.frame_t++;
    switch (bv.state) {
    case BS_TITLE:
        game_set_pausable(false);
        if (btnp(BTN_UP) || btnp(BTN_DOWN)) { bv.menu ^= 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if ((btnp(BTN_A) || btnp(BTN_START)) && bv.state_t > 5) {
            if (bv.menu == 1 && vita_single()) { sfx_play_name("ui_error"); break; }
            sfx_play_name("ui_ok");
            set_state(BS_STORY);
            music_play(BRV_MUS_STORY);
        }
        break;
    case BS_STORY:
        game_set_pausable(false);
        if ((bv.state_t > 20 && (btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_START))) || bv.state_t > 900) {
            input_consume();
            brv_new_run(bv.menu == 1 ? 2 : 1);
            start_banner();
        }
        break;
    case BS_BANNER:
        game_set_pausable(true);
        if (bv.state_t > 100 || (bv.state_t > 30 && btnp(BTN_A))) {
            set_state(BS_FIGHT);
        }
        break;
    case BS_FIGHT: {
        game_set_pausable(true);
        brv_fight_update();
        bool any = false;
        for (int w = 0; w < 2; w++) any |= bv.p[w].on && !bv.p[w].down;
        if (!any) {
            set_state(BS_OVER);
            music_play(BRV_MUS_OVER);
            game_set_pausable(false);
        } else if (bv.qi >= bv.nqueue && brv_alive_foes(true) == 0) {
            win_fight();
        }
        break;
    }
    case BS_WON:
        game_set_pausable(true);
        for (int i = 0; i < BRV_MAX_PARTS; i++)
            if (bv.part[i].life > 0) { bv.part[i].life--; bv.part[i].x += bv.part[i].vx; bv.part[i].y += bv.part[i].vy; }
        if (bv.state_t > 40 && (btnp(BTN_A) || (bv.players == 2 && btnp2(BTN_A)))) {
            input_consume();
            if (bv.round == BRV_ROUNDS - 1) {
                set_state(BS_ENDING);
                game_set_pausable(false);
                music_play(BRV_MUS_ENDING);
            } else {
                open_shop();
            }
        }
        break;
    case BS_SHOP:
        game_set_pausable(true);
        shop_update();
        break;
    case BS_OVER:
        game_set_pausable(false);
        if (bv.state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            run_over();
            to_title();
        }
        break;
    case BS_ENDING:
        game_set_pausable(false);
        if (bv.state_t > 120 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            set_state(BS_CREDITS);
        }
        break;
    case BS_CREDITS:
        game_set_pausable(false);
        if (btn(BTN_A)) bv.state_t += 3;
        if (bv.state_t > 1200) {
            input_consume();
            run_over();
            to_title();
        }
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* cartridge interface                                                      */

static void brv_load(void) {
    brv_art_load();
    brv_audio_load();
}

static void brv_start(void) {
    load_save();
    memset(&bv, 0, sizeof bv);
    bv.sale = bv.hike = -1;
    goals_from_save();
    to_title();
}

static void brv_quit(void) {
    input_set_versus(false);
    brv_save_now();
}

static int count_kind(int k) {
    int n = 0;
    for (int i = 0; i < BRV_MAX_FOES; i++) n += bv.foe[i].alive && bv.foe[i].kind == k;
    return n;
}

static int first_of(int k) {
    for (int i = 0; i < BRV_MAX_FOES; i++)
        if (bv.foe[i].alive && bv.foe[i].kind == k) return i;
    return -1;
}

static int brv_query(const char *key, int *out) {
    const Player *p0 = &bv.p[0], *p1 = &bv.p[1];
    if (!strcmp(key, "bot")) { *out = brv_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = bv.state; return 1; }
    if (!strcmp(key, "menu")) { *out = bv.menu; return 1; }
    if (!strcmp(key, "players")) { *out = bv.players; return 1; }
    if (!strcmp(key, "round")) { *out = bv.round + 1; return 1; }
    if (!strcmp(key, "cash")) { *out = bv.cash; return 1; }
    if (!strcmp(key, "prize")) { *out = bv.prize; return 1; }
    if (!strcmp(key, "groups")) { *out = bv.ngroups; return 1; }
    if (!strcmp(key, "lineup_monsters")) { *out = brv_lineup_monsters(); return 1; }
    if (!strncmp(key, "group", 5) && isdigit((unsigned char)key[5])) {
        int g = atoi(key + 5);
        const char *u = strchr(key + 5, '_');
        if (g < 0 || g >= BRV_MAX_GROUPS || !u) return 0;
        if (!strcmp(u, "_kind")) { *out = g < bv.ngroups ? bv.lineup[g].kind : -1; return 1; }
        if (!strcmp(u, "_count")) { *out = g < bv.ngroups ? bv.lineup[g].count : -1; return 1; }
        return 0;
    }
    if (!strcmp(key, "shop_row")) { *out = bv.shop_row; return 1; }
    if (!strcmp(key, "shop_btn")) { *out = bv.shop_btn; return 1; }
    if (!strcmp(key, "shop_sel")) { *out = bv.shop_sel; return 1; }
    if (!strcmp(key, "sale")) { *out = bv.sale; return 1; }
    if (!strcmp(key, "hike")) { *out = bv.hike; return 1; }
    if (!strcmp(key, "sale_price")) { *out = bv.sale >= 0 ? brv_price(bv.sale) : -1; return 1; }
    if (!strcmp(key, "hike_price")) { *out = bv.hike >= 0 ? brv_price(bv.hike) : -1; return 1; }
    if (!strcmp(key, "hike_base")) { *out = bv.hike >= 0 ? brv_base_price(bv.hike, bv.gear[bv.hike]) : -1; return 1; }
    if (!strcmp(key, "sale_base")) { *out = bv.sale >= 0 ? brv_base_price(bv.sale, bv.gear[bv.sale]) : -1; return 1; }
    if (!strcmp(key, "bad_groups")) {
        /* packs whose size isn't what their kind comes in */
        static const int LO[MK_KINDS] = {9, 9, 3, 3, 2, 2, 3}, HI[MK_KINDS] = {9, 9, 6, 6, 5, 4, 3};
        int n = 0;
        for (int g = 0; g < bv.ngroups; g++) {
            int k = bv.lineup[g].kind, c = bv.lineup[g].count;
            if (k >= MK_KINDS || c < LO[k] || c > HI[k]) n++;
        }
        *out = n;
        return 1;
    }
    if (!strcmp(key, "hike_note")) { *out = bv.hike_note; return 1; }
    if (!strcmp(key, "upgrades")) { *out = bv.upgrades; return 1; }
    if (!strcmp(key, "kills")) { *out = bv.kills; return 1; }
    if (!strcmp(key, "total_gear_cost")) { *out = brv_total_gear_cost(); return 1; }
    if (!strncmp(key, "price", 5) && isdigit((unsigned char)key[5])) { *out = brv_price(atoi(key + 5)); return 1; }
    if (!strncmp(key, "base", 4) && isdigit((unsigned char)key[4])) {
        /* baseI_T: tier T of item I as listed */
        int i = atoi(key + 4);
        const char *u = strchr(key + 4, '_');
        *out = u ? brv_base_price(i, atoi(u + 1)) : 0;
        return 1;
    }
    if (!strncmp(key, "gear", 4) && isdigit((unsigned char)key[4])) { int i = atoi(key + 4); *out = i < GR_COUNT ? bv.gear[i] : -1; return 1; }
    if (!strncmp(key, "spawn_interval", 14)) { *out = brv_spawn_interval(atoi(key + 14)); return 1; }
    if (!strcmp(key, "queue_left")) { *out = bv.nqueue - bv.qi; return 1; }
    if (!strcmp(key, "queue")) { *out = bv.nqueue; return 1; }
    if (!strcmp(key, "enemies_left")) { *out = brv_enemies_left(); return 1; }
    if (!strcmp(key, "foes")) { *out = brv_alive_foes(false); return 1; }
    if (!strncmp(key, "foes_", 5)) { *out = count_kind(atoi(key + 5)); return 1; }
    if (!strncmp(key, "queued_", 7)) {
        int k = atoi(key + 7), n = 0;
        for (int i = 0; i < bv.nqueue; i++) n += bv.queue[i] == k;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "pools")) {
        int n = 0;
        for (int i = 0; i < BRV_MAX_POOLS; i++) n += bv.pool[i].alive && !bv.pool[i].cooled;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "boss_out")) { *out = bv.boss_out; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = bv.boss_dead; return 1; }
    if (!strcmp(key, "boss_hp")) { int b = first_of(MK_BOSS); *out = b >= 0 ? bv.foe[b].hp : 0; return 1; }
    if (!strcmp(key, "boss_state")) { int b = first_of(MK_BOSS); *out = b >= 0 ? bv.foe[b].state : -1; return 1; }
    if (!strcmp(key, "tone")) { *out = bv.peeper_tone_t; return 1; }
    if (!strcmp(key, "medkits")) { int n = 0; for (int i = 0; i < BRV_MAX_MEDS; i++) n += bv.med[i].alive; *out = n; return 1; }
    if (!strcmp(key, "bombs_out")) { int n = 0; for (int i = 0; i < BRV_MAX_BOMBS; i++) n += bv.bomb[i].alive; *out = n; return 1; }
    if (!strcmp(key, "drone_bombs")) { int n = 0; for (int i = 0; i < BRV_MAX_BOMBS; i++) n += bv.bomb[i].alive && bv.bomb[i].drone; *out = n; return 1; }
    if (!strcmp(key, "blasts")) { int n = 0; for (int i = 0; i < BRV_MAX_BLASTS; i++) n += bv.blast[i].alive; *out = n; return 1; }
    if (!strcmp(key, "pshots")) { int n = 0; for (int i = 0; i < BRV_MAX_PSHOTS; i++) n += bv.ps[i].alive && bv.ps[i].kind == PS_GUN; *out = n; return 1; }
    if (!strcmp(key, "nails")) { int n = 0; for (int i = 0; i < BRV_MAX_PSHOTS; i++) n += bv.ps[i].alive && bv.ps[i].kind == PS_NAIL; *out = n; return 1; }
    if (!strcmp(key, "drone_shots")) { int n = 0; for (int i = 0; i < BRV_MAX_PSHOTS; i++) n += bv.ps[i].alive && bv.ps[i].kind == PS_DRONE; *out = n; return 1; }
    if (!strcmp(key, "eshots")) { int n = 0; for (int i = 0; i < BRV_MAX_ESHOTS; i++) n += bv.es[i].alive; *out = n; return 1; }
    if (!strcmp(key, "shot0_vx100")) { *out = -9999; for (int i = 0; i < BRV_MAX_PSHOTS; i++) if (bv.ps[i].alive) { *out = (int)lroundf(bv.ps[i].vx * 100); break; } return 1; }
    if (!strcmp(key, "shot0_vy100")) { *out = -9999; for (int i = 0; i < BRV_MAX_PSHOTS; i++) if (bv.ps[i].alive) { *out = (int)lroundf(bv.ps[i].vy * 100); break; } return 1; }
    if (!strcmp(key, "shot0_bounces")) { *out = -1; for (int i = 0; i < BRV_MAX_PSHOTS; i++) if (bv.ps[i].alive) { *out = bv.ps[i].bounces; break; } return 1; }
    if (!strcmp(key, "shot0_dmg")) { *out = -1; for (int i = 0; i < BRV_MAX_PSHOTS; i++) if (bv.ps[i].alive) { *out = bv.ps[i].dmg; break; } return 1; }
    if (!strncmp(key, "foe", 3) && isdigit((unsigned char)key[3])) {
        int i = atoi(key + 3);
        const char *u = strchr(key + 3, '_');
        if (i < 0 || i >= BRV_MAX_FOES || !u) return 0;
        const Foe *f = &bv.foe[i];
        if (!strcmp(u, "_alive")) { *out = f->alive; return 1; }
        if (!strcmp(u, "_hp")) { *out = f->alive ? f->hp : 0; return 1; }
        if (!strcmp(u, "_x")) { *out = (int)lroundf(f->x); return 1; }
        if (!strcmp(u, "_y")) { *out = (int)lroundf(f->y); return 1; }
        if (!strcmp(u, "_state")) { *out = f->state; return 1; }
        if (!strcmp(u, "_kind")) { *out = f->kind; return 1; }
        if (!strcmp(u, "_size")) { *out = f->size; return 1; }
        if (!strcmp(u, "_dir")) { *out = f->dir; return 1; }
        return 0;
    }
    /* players: p1_x, p2_hp, ... */
    if (key[0] == 'p' && (key[1] == '1' || key[1] == '2') && key[2] == '_') {
        const Player *p = key[1] == '1' ? p0 : p1;
        const char *u = key + 3;
        if (!strcmp(u, "x")) { *out = (int)lroundf(p->x); return 1; }
        if (!strcmp(u, "y")) { *out = (int)lroundf(p->y); return 1; }
        if (!strcmp(u, "x10")) { *out = (int)lroundf(p->x * 10); return 1; }
        if (!strcmp(u, "y10")) { *out = (int)lroundf(p->y * 10); return 1; }
        if (!strcmp(u, "face")) { *out = p->face; return 1; }
        if (!strcmp(u, "locked")) { *out = p->locked; return 1; }
        if (!strcmp(u, "hp")) { *out = p->hp; return 1; }
        if (!strcmp(u, "maxhp")) { *out = p->maxhp; return 1; }
        if (!strcmp(u, "down")) { *out = p->down; return 1; }
        if (!strcmp(u, "on")) { *out = p->on; return 1; }
        if (!strcmp(u, "bombs")) { *out = p->bombs; return 1; }
        if (!strcmp(u, "blast_sh")) { *out = p->blast_sh; return 1; }
        if (!strcmp(u, "shot_sh")) { *out = p->shot_sh; return 1; }
        if (!strcmp(u, "walks")) { *out = p->walks; return 1; }
        if (!strcmp(u, "dash")) { *out = p->dash_t; return 1; }
        if (!strcmp(u, "dashes")) { *out = p->dashes; return 1; }
        if (!strcmp(u, "fired")) { *out = p->fired; return 1; }
        if (!strcmp(u, "inv")) { *out = p->inv; return 1; }
        if (!strcmp(u, "drone")) { *out = p->drone_on; return 1; }
        if (!strcmp(u, "drone_hp")) { *out = p->drone_hp; return 1; }
        if (!strcmp(u, "drone_x")) { *out = (int)lroundf(p->dx); return 1; }
        if (!strcmp(u, "drone_y")) { *out = (int)lroundf(p->dy); return 1; }
        if (!strcmp(u, "lava")) { *out = brv_in_lava(p->x, p->y + 4); return 1; }
        return 0;
    }
    if (!strcmp(key, "hurt_src")) { *out = bv.hurt_src; return 1; }
    if (!strcmp(key, "dash_reading")) { *out = bv.dash_reading; return 1; }
    if (!strcmp(key, "won")) { *out = bv.won; return 1; }
    if (!strcmp(key, "fight_kills")) { *out = bv.fight_kills; return 1; }
    if (!strcmp(key, "fight_t")) { *out = bv.fight_t; return 1; }
    if (!strcmp(key, "vita_single")) { *out = vita_single(); return 1; }
    if (!strcmp(key, "shop_layout")) { *out = brv_shop_audit(); return 1; }
    /* the save */
    if (!strcmp(key, "save_runs")) { *out = bvs.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = bvs.wins; return 1; }
    if (!strcmp(key, "save_most_upgrades")) { *out = bvs.most_upgrades; return 1; }
    if (!strcmp(key, "save_most_kills")) { *out = bvs.most_kills; return 1; }
    if (!strcmp(key, "save_best_fight")) { *out = bvs.best_fight; return 1; }
    if (!strcmp(key, "save_best_round")) { *out = bvs.best_round; return 1; }
    if (!strcmp(key, "save_best_cash")) { *out = (int)bvs.best_cash; return 1; }
    return 0;
}

static int brv_cheat(const char *cmd) {
    int a, b, c;
    float x, y, vx, vy;
    if (sscanf(cmd, "run %d", &a) == 1) {
        brv_new_run(a == 2 ? 2 : 1);
        start_banner();
        set_state(BS_FIGHT);
        return 1;
    }
    if (sscanf(cmd, "round %d", &a) == 1) {
        /* the shop before fight a (2-8) */
        bv.round = iclamp(a, 2, BRV_ROUNDS) - 2;
        open_shop();
        return 1;
    }
    if (!strcmp(cmd, "fight")) { start_banner(); set_state(BS_FIGHT); return 1; }
    if (sscanf(cmd, "cash %d", &a) == 1) { bv.cash = a; return 1; }
    if (sscanf(cmd, "gear %d %d", &a, &b) == 2) {
        if (a >= 0 && a < GR_COUNT) bv.gear[a] = (uint8_t)iclamp(b, 0, BRV_GEAR[a].tiers);
        return 1;
    }
    if (sscanf(cmd, "sale %d", &a) == 1) { bv.sale = a; return 1; }
    if (sscanf(cmd, "hike %d", &a) == 1) { bv.hike = a; return 1; }
    if (!strcmp(cmd, "nolineup")) { bv.ngroups = 0; return 1; }
    if (sscanf(cmd, "group %d %d", &a, &b) == 2) {
        if (bv.ngroups < BRV_MAX_GROUPS) bv.lineup[bv.ngroups++] = (Group){(uint8_t)a, (uint8_t)b};
        return 1;
    }
    if (sscanf(cmd, "prize %d", &a) == 1) { bv.prize = a; return 1; }
    if (!strcmp(cmd, "god")) { bv.god = true; return 1; }
    if (!strcmp(cmd, "mortal")) { bv.god = false; return 1; }
    if (!strcmp(cmd, "nospawn")) { bv.no_spawn = !bv.no_spawn; return 1; }
    if (!strcmp(cmd, "empty")) {
        /* an empty floor that stays open: nothing queued, nothing about */
        bv.qi = bv.nqueue = 0;
        brv_clear_fight();
        bv.queue[0] = MK_MITE;
        bv.nqueue = 1;
        bv.no_spawn = true;
        return 1;
    }
    if (!strcmp(cmd, "clear")) {
        for (int i = 0; i < BRV_MAX_FOES; i++) if (bv.foe[i].alive) brv_kill_foe(i, KILL_QUIET);
        bv.qi = bv.nqueue;
        bv.no_spawn = false;
        return 1;
    }
    if (!strcmp(cmd, "skipq")) {
        /* straight on to the last thing queued, the floor emptied */
        for (int i = 0; i < BRV_MAX_FOES; i++) if (bv.foe[i].alive) brv_kill_foe(i, KILL_QUIET);
        if (bv.nqueue > 0) bv.qi = bv.nqueue - 1;
        bv.spawn_t = 1;
        return 1;
    }
    if (!strcmp(cmd, "nopools")) { memset(bv.pool, 0, sizeof bv.pool); return 1; }
    if (sscanf(cmd, "pool %f %f", &x, &y) == 2) {
        for (int i = 0; i < BRV_MAX_POOLS; i++)
            if (!bv.pool[i].alive) { bv.pool[i] = (Pool){x, y, 16, 9, 1, 0}; break; }
        return 1;
    }
    if (sscanf(cmd, "foe %d %f %f", &a, &x, &y) == 3) {
        int i = brv_spawn_foe(iclamp(a, 0, MK_ALL - 1), x, y);
        if (i >= 0 && a == MK_PEEPER) { bv.foe[i].state = 2; bv.foe[i].st = 1 << 20; bv.foe[i].fire_t = 1 << 20; }
        return 1;
    }
    if (sscanf(cmd, "still %d %f %f", &a, &x, &y) == 3) {
        /* a monster that doesn't move or shoot (it still splits and blows up) */
        int i = brv_spawn_foe(iclamp(a, 0, MK_ALL - 1), x, y);
        if (i >= 0) {
            bv.foe[i].still = 1;
            if (a == MK_PEEPER) bv.foe[i].state = 2;
        }
        return 1;
    }
    if (!strcmp(cmd, "killboss")) { int i = first_of(MK_BOSS); if (i >= 0) brv_kill_foe(i, KILL_SHOT); return 1; }
    if (sscanf(cmd, "foehp %d %d", &a, &b) == 2) { if (a >= 0 && a < BRV_MAX_FOES) bv.foe[a].hp = b; return 1; }
    if (sscanf(cmd, "bosshp %d", &a) == 1) { int i = first_of(MK_BOSS); if (i >= 0) bv.foe[i].hp = a; return 1; }
    if (sscanf(cmd, "pos %d %f %f", &a, &x, &y) == 3) { Player *p = &bv.p[a == 2 ? 1 : 0]; p->x = x; p->y = y; return 1; }
    if (sscanf(cmd, "hp %d %d", &a, &b) == 2) { bv.p[a == 2 ? 1 : 0].hp = b; return 1; }
    if (sscanf(cmd, "bombs %d %d", &a, &b) == 2) { bv.p[a == 2 ? 1 : 0].bombs = b; return 1; }
    if (sscanf(cmd, "face %d %d", &a, &b) == 2) { bv.p[a == 2 ? 1 : 0].face = b & 7; return 1; }
    if (sscanf(cmd, "inv %d %d", &a, &b) == 2) { bv.p[a == 2 ? 1 : 0].inv = b; return 1; }
    if (sscanf(cmd, "eshot %d %f %f %f %f", &a, &x, &y, &vx, &vy) == 5) { brv_add_eshot(a, x, y, vx, vy); return 1; }
    if (sscanf(cmd, "bomb %f %f %d", &x, &y, &c) == 3) { int i = brv_drop_bomb(0, x, y - 3, false); if (i >= 0) bv.bomb[i].fuse = c; return 1; }
    if (sscanf(cmd, "medkit %f %f", &x, &y) == 2) {
        for (int i = 0; i < BRV_MAX_MEDS; i++) if (!bv.med[i].alive) { bv.med[i] = (Medkit){x, y, 0, 1}; break; }
        return 1;
    }
    if (sscanf(cmd, "dashreading %d", &a) == 1) { bv.dash_reading = a ? 1 : 0; return 1; }
    if (sscanf(cmd, "botplan %d", &a) == 1) { brv_bot_plan = a; return 1; }
    if (sscanf(cmd, "menu %d", &a) == 1) { bv.menu = a ? 1 : 0; return 1; }
    if (!strcmp(cmd, "win")) {
        /* the last fight, won */
        bv.round = BRV_ROUNDS - 1;
        bv.prize = BRV_LAST_PRIZE;
        win_fight();
        return 1;
    }
    if (!strcmp(cmd, "die")) { bv.god = false; bv.p[0].inv = 0; brv_hurt_player(0, 999, HURT_HIT); if (bv.p[1].on) { bv.p[1].inv = 0; brv_hurt_player(1, 999, HURT_HIT); } return 1; }
    return 0;
}

const GameDef GAME_BRAVADO = {
    "bravado",
    "BRAVADO",
    "1987",
    "ARENA SHOOTER",
    "EIGHT FIGHTS, ONE PIT. EACH RAISE ADDS 100 AND A PACK OF MONSTERS.",
    {"WIN A FIGHT WORTH 500 OR MORE", "COME OUT OF ALL EIGHT FIGHTS STANDING", "WALK OUT WITH 4,500 OR MORE IN CASH"},
    "D-PAD\tMOVE, EIGHT WAYS\n"
    "TAP " GLYPH_A "\tSHOOT THE WAY YOU FACE\n"
    "HOLD " GLYPH_A "\tKEEP SHOOTING, AIM HELD\n"
    GLYPH_B "\tDROP A BOMB (IT HURTS YOU TOO)\n"
    GLYPH_B " " GLYPH_B "\tDASH, ONCE YOU OWN IT\n"
    "START\tPAUSE\n"
    "\n"
    "SHOP: " GLYPH_A " BUY, RAISE, FIGHT; " GLYPH_B " BACK",
    C_RED, C_AMBER,
    brv_load, brv_start, brv_update, brv_draw, brv_quit, brv_draw_label, brv_query, brv_cheat,
    "OVERBOLD", 34,
    NULL,
};
