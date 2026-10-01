/* BUZZBOLT - fly out with the Hive Wing and burn back the Blight.
 * Cartridge 39 of UFO 40, a tribute to Star Waspir (UFO 50 #39).
 * See docs/games/39-buzzbolt.md. This file: the title, the ship select,
 * the screens between waves, the ending, the high scores, the save, and
 * the test hooks. The play itself is in buzzbolt_play.c. */
#include "buzzbolt.h"

#define SAVE_MAGIC 0x425A0001u
#define CHERRY_SCORE 300000u

/* the table as it comes: its initials spell a message */
static const HiScore DEFAULT_HS[BZZ_HISCORES] = {
    {"NIX", 50000, BZ_FIREFLY, 5, 1, 0}, {"ORB", 40000, BZ_LACEWING, 5, 1, 0},
    {"TAD", 30000, BZ_SHIELDBUG, 4, 0, 0}, {"ASH", 25000, BZ_FIREFLY, 4, 0, 0},
    {"LUX", 20000, BZ_LACEWING, 3, 0, 0}, {"OWL", 15000, BZ_SHIELDBUG, 3, 0, 0},
    {"NUB", 10000, BZ_FIREFLY, 2, 0, 0}, {"EEL", 5000, BZ_LACEWING, 1, 0, 0},
};

static void set_state(int s) {
    bz.state = s;
    bz.state_t = 0;
}

static void load_save(void) {
    BzzSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { bzs = tmp; return; }
    memset(&bzs, 0, sizeof bzs);
    bzs.magic = SAVE_MAGIC;
    memcpy(bzs.hs, DEFAULT_HS, sizeof bzs.hs);
}

/* where a score lands in the table (BZZ_HISCORES = not in it) */
static int rank_of(uint32_t score) {
    for (int i = 0; i < BZZ_HISCORES; i++)
        if (score > bzs.hs[i].score) return i;
    return BZZ_HISCORES;
}

/* the run is over, won or lost: keep its records, then the table */
static void run_over(void) {
    if (bz.score > bzs.best[bz.ship]) bzs.best[bz.ship] = bz.score;
    if (bz.best_mult > bzs.best_mult) bzs.best_mult = (uint16_t)bz.best_mult;
    if (bz.won && bzs.wins < 65535) bzs.wins++;
    if (bz.won && bz.score >= CHERRY_SCORE && bzs.cherries < 65535) bzs.cherries++;
    bzz_save_now();
    bz.name_rank = rank_of(bz.score);
    if (bz.name_rank < BZZ_HISCORES) {
        memcpy(bz.name, "AAA", 4);
        bz.name_pos = 0;
        set_state(BS_NAME);
        music_play(BZZ_MUS_NAME);
    } else {
        bz.scores_page = 1;
        set_state(BS_SCORES);
        music_play(BZZ_MUS_TITLE);
    }
}

static void enter_name(void) {
    int r = bz.name_rank;
    for (int i = BZZ_HISCORES - 1; i > r; i--) bzs.hs[i] = bzs.hs[i - 1];
    HiScore *h = &bzs.hs[r];
    memset(h, 0, sizeof *h);
    memcpy(h->name, bz.name, 3);
    h->score = bz.score;
    h->ship = (uint8_t)bz.ship;
    h->wave = (uint8_t)(bz.won ? BZZ_WAVES : bz.wave + 1);
    h->won = bz.won;
    bzz_save_now();
    bz.scores_page = 1;
    set_state(BS_SCORES);
    music_play(BZZ_MUS_TITLE);
}

static void to_title(void) {
    set_state(BS_TITLE);
    game_set_pausable(false);
    music_play(BZZ_MUS_TITLE);
}

static void start_ending(void) {
    bz.won = true;
    game_award(GOAL_SAUCER);
    if (bz.score >= CHERRY_SCORE) game_award(GOAL_ALIEN);
    set_state(BS_ENDING);
    game_set_pausable(false);
    music_play(BZZ_MUS_ENDING);
}

static void name_update(void) {
    char *c = &bz.name[bz.name_pos];
    static const char SET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .!";
    int n = (int)strlen(SET), at = 0;
    for (int i = 0; i < n; i++) if (SET[i] == *c) at = i;
    if (btn_repeat(BTN_UP)) { *c = SET[(at + 1) % n]; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { *c = SET[(at + n - 1) % n]; sfx_play_name("ui_move"); }
    if (btnp(BTN_RIGHT) && bz.name_pos < 2) { bz.name_pos++; sfx_play_name("ui_move"); }
    if (btnp(BTN_LEFT) && bz.name_pos > 0) { bz.name_pos--; sfx_play_name("ui_move"); }
    if (btnp(BTN_B) && bz.name_pos > 0) { bz.name_pos--; sfx_play_name("ui_back"); }
    if (btnp(BTN_A)) {
        sfx_play_name("ui_ok");
        if (bz.name_pos < 2) bz.name_pos++;
        else enter_name();
    } else if (btnp(BTN_START)) {
        sfx_play_name("ui_ok");
        enter_name();
    }
}

static void bzz_update(void) {
    bz.state_t++;
    switch (bz.state) {
    case BS_TITLE:
        bz.frame_t++;
        bz.scroll += 1.0f;
        game_set_pausable(false);
        if (bz.state_t > 600) { bz.scores_page = 0; set_state(BS_SCORES); break; }
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if (btnp(BTN_A) || btnp(BTN_START)) {
            sfx_play_name("ui_ok");
            set_state(BS_SELECT);
            music_play(BZZ_MUS_SELECT);
        }
        break;
    case BS_SCORES:
        bz.frame_t++;
        bz.scroll += 1.0f;
        game_set_pausable(false);
        if ((btnp(BTN_A) || btnp(BTN_START) || btnp(BTN_B)) && bz.state_t > 10) { sfx_play_name("ui_ok"); to_title(); }
        else if (bz.state_t > 420) to_title();
        break;
    case BS_SELECT:
        bz.frame_t++;
        bz.scroll += 2.0f;
        game_set_pausable(false);
        if (btnp(BTN_LEFT)) { bz.sel = (bz.sel + BZ_SHIPS - 1) % BZ_SHIPS; sfx_play_name("ui_move"); }
        if (btnp(BTN_RIGHT)) { bz.sel = (bz.sel + 1) % BZ_SHIPS; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { sfx_play_name("ui_back"); to_title(); break; }
        if ((btnp(BTN_A) || btnp(BTN_START)) && bz.state_t > 6) {
            sfx_play_name("bzz_launch");
            input_consume();
            game_set_pausable(true);
            bzz_new_run(bz.sel);
        }
        break;
    case BS_PLAY:
        game_set_pausable(true);
        bzz_play_update();
        if (bz.state == BS_OVER) game_set_pausable(false);
        break;
    case BS_CLEAR:
        bz.frame_t++;
        bz.scroll += 3.0f;
        if (bz.bonus_shown < bz.time_bonus) bz.bonus_shown = imin(bz.time_bonus, bz.bonus_shown + 50);
        if (bz.state_t > 200 || (bz.state_t > 60 && btnp(BTN_A))) {
            input_consume();
            if (bz.wave + 1 >= BZZ_WAVES) start_ending();
            else bzz_start_wave(bz.wave + 1);
        }
        break;
    case BS_OVER:
        bz.frame_t++;
        game_set_pausable(false);
        if (bz.state_t > 180 || (bz.state_t > 60 && btnp(BTN_A))) { input_consume(); run_over(); }
        break;
    case BS_ENDING:
        bz.frame_t++;
        if (bz.state_t > 720 || (bz.state_t > 240 && btnp(BTN_A))) {
            input_consume();
            set_state(BS_CREDITS);
            music_play(BZZ_MUS_CREDITS);
        }
        break;
    case BS_CREDITS:
        bz.frame_t++;
        bz.scroll += 1.0f;
        if (btn(BTN_A)) bz.state_t += 3; /* hold A to hurry them along */
        if (bz.state_t > 1500) { input_consume(); run_over(); }
        break;
    case BS_NAME:
        bz.frame_t++;
        game_set_pausable(false);
        if (bz.state_t > 10) name_update();
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* cartridge interface                                                      */

static void bzz_load(void) {
    bzz_art_load();
    bzz_audio_load();
}

static void bzz_start(void) {
    load_save();
    memset(&bz, 0, sizeof bz);
    bz.sel = 0;
    bz.mult = 1;
    to_title();
}

static void bzz_quit(void) { bzz_save_now(); }

static int count_alive(int what) {
    int n = 0;
    switch (what) {
    case 0: for (int i = 0; i < BZZ_MAX_FOES; i++) n += bz.foe[i].alive; break;
    case 1: for (int i = 0; i < BZZ_MAX_ESHOTS; i++) n += bz.es[i].alive; break;
    case 2: for (int i = 0; i < BZZ_MAX_PSHOTS; i++) n += bz.ps[i].alive; break;
    default: for (int i = 0; i < BZZ_MAX_LETTERS; i++) n += bz.lt[i].alive; break;
    }
    return n;
}

static int first_boss(void) {
    for (int i = 0; i < BZZ_MAX_FOES; i++)
        if (bz.foe[i].alive && bz.foe[i].role == ROLE_BOSS) return i;
    return -1;
}

static int bzz_query(const char *key, int *out) {
    if (!strcmp(key, "bot")) { *out = bzz_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = bz.state; return 1; }
    if (!strcmp(key, "wave")) { *out = bz.wave + 1; return 1; }
    if (!strcmp(key, "wave_t")) { *out = bz.wave_t; return 1; }
    if (!strcmp(key, "ship")) { *out = bz.ship; return 1; }
    if (!strcmp(key, "sel")) { *out = bz.sel; return 1; }
    if (!strcmp(key, "score")) { *out = (int)bz.score; return 1; }
    if (!strcmp(key, "score_k")) { *out = (int)(bz.score / 1000); return 1; }
    if (!strcmp(key, "mult")) { *out = bz.mult; return 1; }
    if (!strcmp(key, "best_mult_run")) { *out = bz.best_mult; return 1; }
    if (!strcmp(key, "lives")) { *out = bz.lives; return 1; }
    if (!strcmp(key, "extends")) { *out = bz.extends; return 1; }
    if (!strcmp(key, "deaths")) { *out = bz.deaths; return 1; }
    if (!strcmp(key, "alive")) { *out = bz.alive; return 1; }
    if (!strcmp(key, "inv")) { *out = bz.inv; return 1; }
    if (!strcmp(key, "nword")) { *out = bz.nword; return 1; }
    if (!strncmp(key, "word", 4) && key[4] >= '0' && key[4] <= '2') { *out = bz.word[key[4] - '0']; return 1; }
    if (!strcmp(key, "last_word")) { *out = bz.last_word; return 1; }
    if (!strcmp(key, "drops")) { *out = bz.drop_idx; return 1; }
    if (!strcmp(key, "next_letter")) { *out = bz.drop_idx % 3 == 0 ? LT_B : LT_Z; return 1; }
    if (!strcmp(key, "caught")) { *out = bz.letters_caught; return 1; }
    if (!strcmp(key, "px")) { *out = (int)lroundf(bz.px); return 1; }
    if (!strcmp(key, "py")) { *out = (int)lroundf(bz.py); return 1; }
    if (!strcmp(key, "px10")) { *out = (int)lroundf(bz.px * 10); return 1; }
    if (!strcmp(key, "focused")) { *out = bz.focused; return 1; }
    if (!strcmp(key, "fire_hold")) { *out = bz.fire_hold; return 1; }
    if (!strcmp(key, "spread_win")) { *out = bz.spread_win; return 1; }
    if (!strcmp(key, "charge")) { *out = bz.charge; return 1; }
    if (!strcmp(key, "volleys")) { *out = bz.volley; return 1; }
    if (!strcmp(key, "nopt")) { *out = bz.nopt; return 1; }
    if (!strcmp(key, "opt0_hp")) { *out = bz.nopt > 0 ? bz.opt[0].hp : -1; return 1; }
    if (!strcmp(key, "opt0_dy")) { *out = bz.nopt > 0 ? (int)lroundf(bz.opt[0].y - bz.py) : 999; return 1; }
    if (!strcmp(key, "opt0_dx")) { *out = bz.nopt > 0 ? (int)lroundf(bz.opt[0].x - bz.px) : 999; return 1; }
    if (!strcmp(key, "orbs")) { *out = bz.orb[0] + bz.orb[1]; return 1; }
    if (!strcmp(key, "power_t")) { *out = bz.power_t; return 1; }
    if (!strcmp(key, "bombs")) { *out = bz.bombs; return 1; }
    if (!strcmp(key, "blasts")) { int n = 0; for (int i = 0; i < BZZ_MAX_BLASTS; i++) n += bz.blast[i].alive; *out = n; return 1; }
    if (!strcmp(key, "bombs_flying")) { int n = 0; for (int i = 0; i < BZZ_MAX_BOMBS; i++) n += bz.bomb[i].alive; *out = n; return 1; }
    if (!strcmp(key, "hive_on")) { *out = bz.hive.on; return 1; }
    if (!strcmp(key, "fly_on")) { *out = bz.fly.on; return 1; }
    if (!strcmp(key, "fly_fired")) { *out = bz.fly.fired; return 1; }
    if (!strcmp(key, "laser_len")) { *out = bz.laser_t > 0 ? bz.laser_len : 0; return 1; }
    if (!strcmp(key, "foes")) { *out = count_alive(0); return 1; }
    if (!strcmp(key, "eshots")) { *out = count_alive(1); return 1; }
    if (!strcmp(key, "pshots")) { *out = count_alive(2); return 1; }
    if (!strcmp(key, "letters")) { *out = count_alive(3); return 1; }
    if (!strncmp(key, "pshots_", 7)) {
        int k = atoi(key + 7), n = 0;
        for (int i = 0; i < BZZ_MAX_PSHOTS; i++) n += bz.ps[i].alive && bz.ps[i].kind == k;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "eshots_", 7)) {
        int k = atoi(key + 7), n = 0;
        for (int i = 0; i < BZZ_MAX_ESHOTS; i++) n += bz.es[i].alive && bz.es[i].kind == k;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "foes_", 5)) {
        int k = atoi(key + 5), n = 0;
        for (int i = 0; i < BZZ_MAX_FOES; i++) n += bz.foe[i].alive && bz.foe[i].kind == k;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "lance_w")) {
        *out = 0;
        for (int i = 0; i < BZZ_MAX_PSHOTS; i++) if (bz.ps[i].alive && bz.ps[i].kind == PS_LANCE) *out = (int)(bz.ps[i].w * 2);
        return 1;
    }
    if (!strcmp(key, "lance_dmg")) {
        *out = 0;
        for (int i = 0; i < BZZ_MAX_PSHOTS; i++) if (bz.ps[i].alive && bz.ps[i].kind == PS_LANCE) *out = bz.ps[i].dmg;
        return 1;
    }
    if (!strcmp(key, "foe0_hp")) { *out = bz.foe[0].alive ? bz.foe[0].hp : 0; return 1; }
    if (!strcmp(key, "foe1_hp")) { *out = bz.foe[1].alive ? bz.foe[1].hp : 0; return 1; }
    if (!strcmp(key, "foe0_alive")) { *out = bz.foe[0].alive; return 1; }
    if (!strcmp(key, "foe0_state")) { *out = bz.foe[0].state; return 1; }
    if (!strcmp(key, "foe0_y")) { *out = (int)lroundf(bz.foe[0].y); return 1; }
    if (!strcmp(key, "boss_out")) { *out = bz.boss_out; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = bz.boss_dead; return 1; }
    if (!strcmp(key, "boss_count")) { *out = bz.boss_count; return 1; }
    if (!strcmp(key, "boss_hp")) { int b = first_boss(); *out = b >= 0 ? bz.foe[b].hp : 0; return 1; }
    if (!strcmp(key, "boss_kind")) { int b = first_boss(); *out = b >= 0 ? bz.foe[b].kind : -1; return 1; }
    if (!strcmp(key, "midbosses")) { int n = 0; for (int i = 0; i < BZZ_MAX_FOES; i++) n += bz.foe[i].alive && bz.foe[i].role == ROLE_MIDBOSS; *out = n; return 1; }
    if (!strcmp(key, "pests")) { int n = 0; for (int i = 0; i < BZZ_MAX_FOES; i++) n += bz.foe[i].alive && bz.foe[i].form == FM_PEST; *out = n; return 1; }
    if (!strcmp(key, "gnat_value")) { *out = 0; for (int i = 0; i < BZZ_MAX_FOES; i++) if (bz.foe[i].alive && bz.foe[i].kind == EK_GNAT) { *out = bz.foe[i].value; break; } return 1; }
    if (!strcmp(key, "time_bonus")) { *out = bz.time_bonus; return 1; }
    if (!strcmp(key, "gates_on") || !strcmp(key, "gates_off")) {
        int n = 0, want = key[7] == 'n';
        for (int i = 0; i < BZZ_MAX_FOES; i++)
            n += bz.foe[i].alive && bz.foe[i].kind == EK_ROTWALL && bz.foe[i].sub > 0 && bz.foe[i].state == want;
        *out = n;
        return 1;
    }
    /* wave 3 has more enemy shots on screen, on average, than waves 1, 2 and 4 */
    if (!strcmp(key, "wave3_spike")) {
        int a3 = bz.es_frames[2] ? bz.es_sum[2] * 10 / bz.es_frames[2] : 0;
        *out = 1;
        for (int w = 0; w < 4; w++)
            if (w != 2 && bz.es_frames[w] && bz.es_sum[w] * 10 / bz.es_frames[w] >= a3) *out = 0;
        return 1;
    }
    if (!strcmp(key, "gold_dir")) {
        *out = 0;
        for (int i = 0; i < BZZ_MAX_FOES; i++)
            if (bz.foe[i].alive && bz.foe[i].kind == EK_GOLDBUG && bz.foe[i].t >= 0) { *out = bz.foe[i].dir; break; }
        return 1;
    }
    if (!strcmp(key, "bonus_given")) { *out = bz.bonus_given; return 1; }
    if (!strcmp(key, "bonus_diff")) { *out = bz.bonus_given - bz.time_bonus; return 1; }
    if (!strcmp(key, "es0_x") || !strcmp(key, "es0_y")) {
        *out = -999;
        for (int i = 0; i < BZZ_MAX_ESHOTS; i++)
            if (bz.es[i].alive) { *out = (int)lroundf(key[4] == 'x' ? bz.es[i].x : bz.es[i].y); break; }
        return 1;
    }
    if (!strcmp(key, "kills")) { *out = bz.kills; return 1; }
    if (!strcmp(key, "kills_wave")) { *out = bz.kills_wave; return 1; }
    if (!strcmp(key, "escaped")) { *out = bz.escaped; return 1; }
    if (!strcmp(key, "won")) { *out = bz.won; return 1; }
    if (!strcmp(key, "name_rank")) { *out = bz.name_rank; return 1; }
    if (!strcmp(key, "rings_bounced")) { int n = 0; for (int i = 0; i < BZZ_MAX_ESHOTS; i++) n += bz.es[i].alive && bz.es[i].kind == ES_RING && bz.es[i].bounced; *out = n; return 1; }
    /* the save */
    if (!strcmp(key, "save_best_mult")) { *out = bzs.best_mult; return 1; }
    if (!strncmp(key, "save_best", 9) && key[9] >= '0' && key[9] <= '2') { *out = (int)bzs.best[key[9] - '0']; return 1; }
    if (!strcmp(key, "save_runs")) { *out = bzs.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = bzs.wins; return 1; }
    if (!strcmp(key, "save_cherries")) { *out = bzs.cherries; return 1; }
    if (!strcmp(key, "save_best_wave")) { *out = bzs.best_wave; return 1; }
    if (!strncmp(key, "hs", 2) && key[2] >= '0' && key[2] <= '7') {
        HiScore *h = &bzs.hs[key[2] - '0'];
        if (!strcmp(key + 3, "_score")) { *out = (int)h->score; return 1; }
        if (!strcmp(key + 3, "_ship")) { *out = h->ship; return 1; }
        if (!strcmp(key + 3, "_c0")) { *out = h->name[0]; return 1; }
        if (!strcmp(key + 3, "_c1")) { *out = h->name[1]; return 1; }
        if (!strcmp(key + 3, "_c2")) { *out = h->name[2]; return 1; }
    }
    /* the waves as written: how many of each foe, and the bosses */
    if (!strncmp(key, "wave_count_", 11)) {
        int w = key[11] - '1', k = atoi(key + 13), n = 0;
        if (w < 0 || w >= BZZ_WAVES || key[12] != '_') return 0;
        const WaveDef *wd = &BZZ_WAVE[w];
        for (int i = 0; i < wd->count; i++) {
            const Spawn *s = &wd->spawns[i];
            if (s->kind != k) continue;
            if (s->form == FM_WALL) { for (int c = 0; c < 16; c++) n += (s->arg >> c) & 1; }
            else n += s->form == FM_MIDBOSS ? 1 : s->n;
        }
        *out = n;
        return 1;
    }
    if (!strcmp(key, "sheet")) { *out = bz.sheet; return 1; }
    /* enemy shots on screen, on average over a wave (times ten) */
    if (!strncmp(key, "es_avg", 6) && key[6] >= '1' && key[6] <= '5') {
        int w = key[6] - '1';
        *out = bz.es_frames[w] ? bz.es_sum[w] * 10 / bz.es_frames[w] : 0;
        return 1;
    }
    return 0;
}

static int bzz_cheat(const char *cmd) {
    int a, b, c;
    float x, y, vx, vy;
    if (sscanf(cmd, "run %d", &a) == 1) { bzz_new_run(iclamp(a, 0, BZ_SHIPS - 1)); game_set_pausable(true); return 1; }
    if (sscanf(cmd, "wave %d", &a) == 1) { bzz_start_wave(iclamp(a, 1, BZZ_WAVES) - 1); bz.wave_t = 0; return 1; }
    if (!strcmp(cmd, "boss")) { bz.wave_t = BZZ_WAVE[bz.wave].boss_t; bz.spawn_i = BZZ_WAVE[bz.wave].count; return 1; }
    if (!strcmp(cmd, "god")) { bz.god = !bz.god; return 1; }
    if (!strcmp(cmd, "clear")) { bzz_clear_world(); bz.spawn_i = BZZ_WAVE[bz.wave].count; bz.boss_out = true; bz.boss_count = 0; return 1; }
    if (!strcmp(cmd, "quiet")) { bz.spawn_i = BZZ_WAVE[bz.wave].count; bz.boss_out = true; return 1; }
    if (sscanf(cmd, "score %d", &a) == 1) { bz.score = (uint32_t)a; return 1; }
    if (sscanf(cmd, "addscore %d", &a) == 1) { bzz_add_score((uint32_t)a); return 1; }
    if (sscanf(cmd, "mult %d", &a) == 1) { bz.mult = a; if (a > bz.best_mult) bz.best_mult = a; return 1; }
    if (sscanf(cmd, "lives %d", &a) == 1) { bz.lives = a; return 1; }
    if (sscanf(cmd, "pos %f %f", &x, &y) == 2) { bz.px = x; bz.py = y; return 1; }
    if (sscanf(cmd, "drops %d", &a) == 1) { bz.drop_idx = a; return 1; }
    if (sscanf(cmd, "letter %f %f", &x, &y) == 2) {
        /* the next letter in the cycle, dropped here */
        for (int i = 0; i < BZZ_MAX_LETTERS; i++)
            if (!bz.lt[i].alive) { bz.lt[i] = (Letter){x, y, 0, 0, 1, (uint8_t)bzz_next_drop(), 0}; break; }
        return 1;
    }
    if (sscanf(cmd, "dummy %d %f %f", &a, &x, &y) == 3) {
        /* a foe that sits still and never fires */
        int i = bzz_spawn_foe(iclamp(a, 0, EK_COUNT - 1), FM_HOVER, x, y, 0, 0, 0, 1 << 30);
        if (i >= 0) { bz.foe[i].y = y; bz.foe[i].t = 0; }
        return 1;
    }
    if (sscanf(cmd, "gate %f %f %d", &x, &y, &a) == 3) {
        /* a still gate block that switches every a frames */
        int i = bzz_spawn_foe(EK_ROTWALL, FM_HOVER, x, y, 0, 0, 0, 1 << 30);
        if (i >= 0) { bz.foe[i].y = y; bz.foe[i].t = 0; bz.foe[i].sub = a; }
        return 1;
    }
    if (sscanf(cmd, "bigdummy %d %f %f", &a, &x, &y) == 3) {
        /* a still midboss: it drops three letters */
        int i = bzz_spawn_foe(iclamp(a, 0, EK_COUNT - 1), FM_HOVER, x, y, 0, 0, 0, 1 << 30);
        if (i >= 0) { bz.foe[i].y = y; bz.foe[i].t = 0; bz.foe[i].role = ROLE_MIDBOSS; }
        return 1;
    }
    if (sscanf(cmd, "foe %d %d %f %f %d", &a, &b, &x, &y, &c) == 5) { bzz_spawn_foe(a, b, x, y, 0, 1, c, 0); return 1; }
    if (sscanf(cmd, "hp %d %d", &a, &b) == 2) { if (a >= 0 && a < BZZ_MAX_FOES) bz.foe[a].hp = b; return 1; }
    if (sscanf(cmd, "bosshp %d", &a) == 1) { for (int i = 0; i < BZZ_MAX_FOES; i++) if (bz.foe[i].alive && bz.foe[i].role == ROLE_BOSS) bz.foe[i].hp = a; return 1; }
    if (sscanf(cmd, "eshot %d %f %f %f %f", &a, &x, &y, &vx, &vy) == 5) { bzz_add_eshot(a, x, y, vx, vy); return 1; }
    if (sscanf(cmd, "word %d", &a) == 1) { bzz_word_effect(a); return 1; }
    if (sscanf(cmd, "botship %d", &a) == 1) { bzz_bot_ship = iclamp(a, 0, BZ_SHIPS - 1); return 1; }
    if (!strcmp(cmd, "sheet")) { bz.sheet = !bz.sheet; return 1; }
    if (!strcmp(cmd, "win")) { bz.wave = BZZ_WAVES - 1; start_ending(); return 1; }
    if (!strcmp(cmd, "over")) { bz.lives = 0; bz.god = false; bz.inv = 0; bzz_kill_ship(); return 1; }
    return 0;
}

const GameDef GAME_BUZZBOLT = {
    "buzzbolt",
    "BUZZBOLT",
    "1988",
    "ARCADE SHOOTER",
    "FAST, WIDE, ONE HIT. EVERY KILL DROPS A LETTER: SPELL BZZ TO MULTIPLY.",
    {"REACH A 10X MULTIPLIER", "BEAT ALL FIVE WAVES", "WIN WITH 300,000 POINTS"},
    "D-PAD\tFLY\n"
    "TAP " GLYPH_A "\tSPREAD SHOT, FULL SPEED\n"
    "HOLD " GLYPH_A "\tFOCUSED SHOT, SLOWER\n"
    GLYPH_B "\tFIREFLY: LAUNCH A BOMB\n"
    "START\tPAUSE\n"
    "\n"
    "FLY INTO THE LETTERS THAT FALL.\n"
    "EVERY THREE MAKE A WORD.",
    C_AMBER, C_NAVY,
    bzz_load, bzz_start, bzz_update, bzz_draw, bzz_quit, bzz_draw_label, bzz_query, bzz_cheat,
    "STAR WASPIR", 39,
    NULL,
};
