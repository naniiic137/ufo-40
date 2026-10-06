/* DRIFTLINE - drive the coast road from morning to daybreak with a gun in
 * the back seat. Cartridge 48 of UFO 40, a tribute to Seaside Drive (UFO 50
 * #48). See docs/games/48-driftline.md. This file: the title, the screens
 * between stages, the ending, the records, and the test hooks. The drive
 * itself is in driftline_play.c, the foes in driftline_foes.c. */
#include "driftline.h"

#define SAVE_MAGIC 0x444C0001u
#define CHERRY_SCORE 300000u

static void set_state(int s) {
    dfg.state = s;
    dfg.state_t = 0;
}

static bool vita_single(void) { return plat_kind() == PLAT_VITA; }

static void load_save(void) {
    DflSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { dfs = tmp; return; }
    memset(&dfs, 0, sizeof dfs);
    dfs.magic = SAVE_MAGIC;
}

static void to_title(void) {
    set_state(DS_TITLE);
    game_set_pausable(false);
    input_set_versus(false);
    music_play(DFL_MUS_TITLE);
}

/* the run is over, won or lost: keep its records */
static void record_run(void) {
    if (dfg.score > dfs.best) dfs.best = dfg.score;
    if (dfg.most_cars > dfs.most_cars) dfs.most_cars = (uint8_t)imin(dfg.most_cars, 255);
    if (dfg.won && dfs.wins < 65535) dfs.wins++;
    if (dfg.won && dfg.score >= CHERRY_SCORE && dfs.cherries < 65535) dfs.cherries++;
    dfl_save_now();
}

static void start_run(int players) {
    input_consume();
    game_set_pausable(true);
    input_set_versus(players == 2);
    dfl_new_run(players);
    set_state(DS_PLAY);
    music_play(DFL_MUS_STAGE[0]);
}

static void next_stage(void) {
    if (dfg.stage + 1 >= DFL_STAGES) {
        /* the end of the road */
        dfg.won = true;
        game_award(GOAL_SAUCER);
        if (dfg.score >= CHERRY_SCORE) game_award(GOAL_ALIEN);
        record_run();
        set_state(DS_ENDING);
        game_set_pausable(false);
        music_play(DFL_MUS_ENDING);
        return;
    }
    dfl_start_stage(dfg.stage + 1);
    set_state(DS_PLAY);
    game_set_pausable(true);
    music_play(DFL_MUS_STAGE[dfg.stage]);
}

/* every car is gone and none is coming back */
static bool run_lost(void) {
    for (int p = 0; p < DFL_CARS; p++) {
        const DflCar *c = &dfg.car[p];
        if (c->on && (c->alive || c->dead_t > 0)) return false;
    }
    return true;
}

static void inputs(uint16_t *in0, uint16_t *in1) {
    uint32_t held = input_held();
    *in0 = (uint16_t)(held & 0xFF);
    *in1 = dfg.players == 2 ? (uint16_t)((held >> BTN_P2_SHIFT) & 0xFF) : 0;
}

static void title_update(void) {
    game_set_pausable(false);
    dfg.frame_t++;
    dfg.scroll += 1.0f;
    if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { dfg.sel ^= 1; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if (dfg.state_t < 10 || !(btnp(BTN_A) || btnp(BTN_START))) return;
    if (dfg.sel == 1 && vita_single()) { sfx_play_name("ui_error"); return; }
    sfx_play_name("dfl_start");
    start_run(dfg.sel + 1);
}

static void dfl_update(void) {
    dfg.state_t++;
    uint16_t in0, in1;
    switch (dfg.state) {
    case DS_TITLE: title_update(); break;
    case DS_PLAY:
        game_set_pausable(true);
        inputs(&in0, &in1);
        dfl_play_update(in0, in1);
        if (run_lost()) {
            record_run();
            set_state(DS_OVER);
            game_set_pausable(false);
            music_play(DFL_MUS_OVER);
            break;
        }
        if (dfg.boss.on && dfg.boss.dead && dfg.boss.dead_t >= DFL_BOSS_FALL_T) {
            set_state(DS_CLEAR);
            music_play(DFL_MUS_CLEAR);
        }
        break;
    case DS_CLEAR:
        dfg.frame_t++;
        dfg.scroll += DFL_SCROLL;
        if (dfg.state_t >= DFL_CLEAR_T || (dfg.state_t > 90 && btnp(BTN_A))) {
            input_consume();
            /* a stage with no car lost leads to a bonus stage (not after the last) */
            if (dfg.lost_stage == 0 && dfg.stage < DFL_STAGES - 1) {
                dfl_bonus_start();
                set_state(DS_BONUS);
                dfg.stage_t = 0;
                music_play(DFL_MUS_BONUS);
            } else {
                next_stage();
            }
        }
        break;
    case DS_BONUS:
        game_set_pausable(true);
        inputs(&in0, &in1);
        dfl_bonus_update(in0, in1);
        if (!dfg.ball_live && !dfg.coin_out) {
            dfg.clear_t++;
            if (dfg.clear_t > 100) {
                dfg.clear_t = 0;
                set_state(DS_BONUS_END);
                if (!dfg.bonus_won) music_play(DFL_MUS_CLEAR);
            }
        }
        break;
    case DS_BONUS_END:
        dfg.frame_t++;
        if (dfg.state_t >= 200 || (dfg.state_t > 60 && btnp(BTN_A))) { input_consume(); next_stage(); }
        break;
    case DS_OVER:
        dfg.frame_t++;
        game_set_pausable(false);
        if (dfg.state_t >= 300 || (dfg.state_t > 60 && btnp(BTN_A))) { input_consume(); to_title(); }
        break;
    case DS_ENDING:
        dfg.frame_t++;
        dfg.scroll += 0.6f;
        if (dfg.state_t >= 900 || (dfg.state_t > 240 && btnp(BTN_A))) {
            input_consume();
            set_state(DS_CREDITS);
            music_play(DFL_MUS_CREDITS);
        }
        break;
    case DS_CREDITS:
        dfg.frame_t++;
        dfg.scroll += 1.0f;
        if (btn(BTN_A)) dfg.state_t += 3; /* hold A to hurry them along */
        if (dfg.state_t >= 1500) { input_consume(); to_title(); }
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* cartridge interface                                                      */

static void dfl_load(void) {
    dfl_art_load();
    dfl_audio_load();
}

static void dfl_start(void) {
    load_save();
    memset(&dfg, 0, sizeof dfg);
    to_title();
}

static void dfl_quit(void) {
    input_set_versus(false);
    if (dfg.state != DS_TITLE && dfg.state != DS_CREDITS && dfg.state != DS_ENDING && dfg.state != DS_OVER) record_run();
    else dfl_save_now();
}

static int count_foes(int kind) {
    int n = 0;
    for (int i = 0; i < DFL_MAX_FOES; i++) n += dfg.foe[i].alive && (kind < 0 || dfg.foe[i].kind == kind);
    return n;
}

static int first_foe(int kind) {
    for (int i = 0; i < DFL_MAX_FOES; i++)
        if (dfg.foe[i].alive && dfg.foe[i].kind == kind) return i;
    return -1;
}

static int dfl_query(const char *key, int *out) {
    const DflBoss *b = &dfg.boss;
    if (!strcmp(key, "bot")) {
        int m = dfl_bot_buttons(0);
        if (dfg.players == 2 && dfg.bot2) m |= dfl_bot_buttons(1) << BTN_P2_SHIFT;
        *out = m;
        return 1;
    }
    if (!strcmp(key, "state")) { *out = dfg.state; return 1; }
    if (!strcmp(key, "sel")) { *out = dfg.sel; return 1; }
    if (!strcmp(key, "players")) { *out = dfg.players; return 1; }
    if (!strcmp(key, "stage")) { *out = dfg.stage + 1; return 1; }
    if (!strcmp(key, "stage_t")) { *out = dfg.stage_t; return 1; }
    if (!strcmp(key, "score")) { *out = (int)dfg.score; return 1; }
    if (!strcmp(key, "score_k")) { *out = (int)(dfg.score / 1000); return 1; }
    if (!strcmp(key, "spare")) { *out = dfg.spare; return 1; }
    if (!strcmp(key, "most_cars")) { *out = dfg.most_cars; return 1; }
    if (!strcmp(key, "deaths")) { *out = dfg.deaths; return 1; }
    if (!strcmp(key, "lost_stage")) { *out = dfg.lost_stage; return 1; }
    if (!strcmp(key, "kills")) { *out = dfg.kills; return 1; }
    if (!strcmp(key, "kills_stage")) { *out = dfg.kills_stage; return 1; }
    if (!strcmp(key, "foes_stage")) { *out = dfg.foes_stage; return 1; }
    if (!strcmp(key, "escaped")) { *out = dfg.escaped; return 1; }
    if (!strcmp(key, "stage_points")) { *out = dfg.stage_points; return 1; }
    if (!strcmp(key, "won")) { *out = dfg.won; return 1; }
    if (!strcmp(key, "coins")) { *out = dfg.coins; return 1; }
    if (!strcmp(key, "bonus_played")) { *out = dfg.bonus_played; return 1; }
    if (!strncmp(key, "bot_dbg", 7) && key[7] >= '0' && key[7] <= '3') {
        extern int dfl_bot_dbg[4];
        *out = dfl_bot_dbg[key[7] - '0'];
        return 1;
    }
    if (!strcmp(key, "cause")) { *out = dfg.cause; return 1; }
    if (!strcmp(key, "last_hit_dmg")) { *out = dfg.last_hit_dmg; return 1; }
    /* the car: alive0, x0, vx10_0, aim0, meter0, tier0, face0, inv0, drifting0, on0, dead_t0, fired0, pairs0 */
    {
        static const char *const KEYS[] = {"alive", "x", "vx10_", "aim", "meter", "tier", "face", "inv", "drifting", "on", "dead_t", "fired", "pairs"};
        for (int k = 0; k < ARRAY_LEN(KEYS); k++) {
            size_t n = strlen(KEYS[k]);
            if (!strncmp(key, KEYS[k], n) && key[n] == '0' && !key[n + 1]) {
                const DflCar *c = &dfg.car[key[n] - '0'];
                switch (k) {
                case 0: *out = c->alive; break;
                case 1: *out = (int)lroundf(c->x); break;
                case 2: *out = (int)lroundf(c->vx * 10); break;
                case 3: *out = (int)lroundf(c->aim); break;
                case 4: *out = c->meter; break;
                case 5: *out = dfl_tier(c->meter); break;
                case 6: *out = c->face; break;
                case 7: *out = c->inv; break;
                case 8: *out = c->drifting; break;
                case 9: *out = c->on; break;
                case 10: *out = c->dead_t; break;
                case 11: *out = c->fired; break;
                default: *out = c->pairs; break;
                }
                return 1;
            }
        }
    }
    if (!strcmp(key, "foes")) { *out = count_foes(-1); return 1; }
    if (!strncmp(key, "foes_", 5)) { *out = count_foes(atoi(key + 5)); return 1; }
    if (!strcmp(key, "eshots")) {
        int n = 0;
        for (int i = 0; i < DFL_MAX_ESHOTS; i++) n += dfg.es[i].alive;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "eshots_", 7)) {
        int k = atoi(key + 7), n = 0;
        for (int i = 0; i < DFL_MAX_ESHOTS; i++) n += dfg.es[i].alive && dfg.es[i].kind == k;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "pshots") || !strcmp(key, "pshots_side") || !strcmp(key, "pshots_main")) {
        int n = 0, want = key[7] == 's' ? 1 : key[7] == 'm' ? 0 : -1;
        for (int i = 0; i < DFL_MAX_PSHOTS; i++) n += dfg.ps[i].alive && (want < 0 || dfg.ps[i].side == want);
        *out = n;
        return 1;
    }
    /* the newest player shot's heading: its vx and vy times ten */
    if (!strcmp(key, "pshot_vx10") || !strcmp(key, "pshot_vy10") || !strcmp(key, "pshot_dmg")) {
        *out = -999;
        for (int i = DFL_MAX_PSHOTS - 1; i >= 0; i--)
            if (dfg.ps[i].alive) {
                *out = key[6] == 'd' ? dfg.ps[i].dmg : (int)lroundf((key[7] == 'x' ? dfg.ps[i].vx : dfg.ps[i].vy) * 10);
                break;
            }
        return 1;
    }
    /* the side shots flying left and right */
    if (!strcmp(key, "side_left") || !strcmp(key, "side_right")) {
        int n = 0, want = key[5] == 'l' ? -1 : 1;
        for (int i = 0; i < DFL_MAX_PSHOTS; i++) n += dfg.ps[i].alive && dfg.ps[i].side && (dfg.ps[i].vx > 0 ? 1 : -1) == want;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "side_up")) {
        int n = 0;
        for (int i = 0; i < DFL_MAX_PSHOTS; i++) n += dfg.ps[i].alive && dfg.ps[i].side && dfg.ps[i].vy < 0;
        *out = n;
        return 1;
    }
    /* the first live foe of a kind: kind_hp_K, kind_x_K, kind_y_K, kind_state_K, kind_ground_K, kind_flash_K */
    if (!strncmp(key, "kind_", 5)) {
        const char *f = key + 5, *us = strrchr(key, '_');
        if (!us) return 0;
        int i = first_foe(atoi(us + 1));
        *out = -1;
        if (i < 0) return 1;
        const DflFoe *e = &dfg.foe[i];
        if (!strncmp(f, "hp_", 3)) *out = e->hp;
        else if (!strncmp(f, "x_", 2)) *out = (int)lroundf(e->x);
        else if (!strncmp(f, "y_", 2)) *out = (int)lroundf(e->y);
        else if (!strncmp(f, "state_", 6)) *out = e->state;
        else if (!strncmp(f, "ground_", 7)) *out = e->ground;
        else if (!strncmp(f, "vy10_", 5)) *out = (int)lroundf(e->vy * 10);
        else return 0;
        return 1;
    }
    if (!strcmp(key, "boss_on")) { *out = b->on; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = b->dead; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = b->on ? b->hp : 0; return 1; }
    if (!strcmp(key, "boss_kind")) { *out = b->on ? b->kind : -1; return 1; }
    if (!strcmp(key, "boss_y")) { *out = (int)lroundf(b->y); return 1; }
    if (!strcmp(key, "boss_x")) { *out = (int)lroundf(b->x); return 1; }
    if (!strncmp(key, "part_hp", 7) && key[7] >= '0' && key[7] <= '5') { *out = b->part_hp[key[7] - '0']; return 1; }
    if (!strncmp(key, "part_t", 6) && key[6] >= '0' && key[6] <= '5') { *out = b->part_t[key[6] - '0']; return 1; }
    if (!strcmp(key, "orbiters")) {
        int n = 0;
        for (int k = 0; k < 4; k++) n += b->part_hp[k] > 0;
        *out = b->on && b->kind == BOSS_ORRERY ? n : 0;
        return 1;
    }
    if (!strcmp(key, "core_col")) {
        /* the Orrery's core: 0 white, 1 yellow, 2 red */
        int third = b->maxhp ? b->hp * 3 / b->maxhp : 3;
        *out = third >= 2 ? 0 : third >= 1 ? 1 : 2;
        return 1;
    }
    if (!strcmp(key, "hand_state")) { *out = b->hand_state; return 1; }
    if (!strcmp(key, "hand_x")) { *out = (int)lroundf(b->hx); return 1; }
    if (!strcmp(key, "revealed")) { *out = b->revealed; return 1; }
    if (!strcmp(key, "swells")) {
        int n = 0;
        for (int k = 0; k < DFL_MAX_SWELLS; k++) n += dfg.swell[k].alive;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "swell_breaking")) {
        int n = 0;
        for (int k = 0; k < DFL_MAX_SWELLS; k++) n += dfl_swell_breaking(k);
        *out = n;
        return 1;
    }
    if (!strcmp(key, "blocks_left")) { *out = dfg.blocks_left; return 1; }
    if (!strcmp(key, "blocks_total")) { *out = dfg.blocks_total; return 1; }
    if (!strcmp(key, "blocks_broken")) { *out = dfg.blocks_broken; return 1; }
    if (!strcmp(key, "blocks_shot")) { *out = dfg.blocks_shot; return 1; }
    if (!strcmp(key, "bonus_points")) { *out = (int)dfg.bonus_points; return 1; }
    if (!strcmp(key, "ball_live")) { *out = dfg.ball_live; return 1; }
    if (!strcmp(key, "ball_y")) { *out = (int)lroundf(dfg.by); return 1; }
    if (!strcmp(key, "ball_x")) { *out = (int)lroundf(dfg.bx); return 1; }
    if (!strcmp(key, "ball_vy10")) { *out = (int)lroundf(dfg.bvy * 10); return 1; }
    if (!strcmp(key, "next_block_value")) { *out = 1000 + 200 * dfg.blocks_broken; return 1; }
    if (!strcmp(key, "es0_vx10") || !strcmp(key, "es0_vy10")) {
        *out = -999;
        for (int i = 0; i < DFL_MAX_ESHOTS; i++)
            if (dfg.es[i].alive) { *out = (int)lroundf((key[5] == 'x' ? dfg.es[i].vx : dfg.es[i].vy) * 10); break; }
        return 1;
    }
    /* the cherry's arithmetic: every foe of every stage and all four bosses,
     * plus the best one bonus stage could give (every block shot), or the
     * two smallest bonus stages with no block shot */
    if (!strcmp(key, "cherry_one_bonus_max") || !strcmp(key, "cherry_two_bonus_min")) {
        int all = 0, bonus[3];
        for (int s = 0; s < DFL_STAGES; s++) all += (int)dfl_stage_points(s) + DFL_BOSS_POINTS[s];
        for (int m = 0; m < 3; m++) {
            int n = 0;
            for (int r = 0; r < DFL_BROWS; r++)
                for (int c = 0; c < DFL_BCOLS; c++) n += DFL_BONUS_MAP[m][r][c] == '#';
            bonus[m] = n * 1000 + 200 * n * (n - 1) / 2;
            if (key[7] == 'o') bonus[m] += 800 * n;
        }
        if (key[7] == 'o') *out = all + imax(bonus[0], imax(bonus[1], bonus[2]));
        else {
            int hi = imax(bonus[0], imax(bonus[1], bonus[2]));
            *out = all + bonus[0] + bonus[1] + bonus[2] - hi;
        }
        return 1;
    }
    if (!strcmp(key, "bonus_won")) { *out = dfg.bonus_won; return 1; }
    if (!strncmp(key, "block_hp_", 9)) {
        int r = key[9] - '0', c = atoi(key + 11);
        if (r < 0 || r >= DFL_BROWS || c < 0 || c >= DFL_BCOLS) return 0;
        *out = dfg.block[r][c];
        return 1;
    }
    /* the stages as written */
    if (!strncmp(key, "plan_points", 11) && key[11] >= '1' && key[11] <= '4') { *out = (int)dfl_stage_points(key[11] - '1'); return 1; }
    if (!strncmp(key, "plan_foes", 9) && key[9] >= '1' && key[9] <= '4') { *out = dfl_stage_foes(key[9] - '1'); return 1; }
    if (!strncmp(key, "plan_blocks", 11) && key[11] >= '1' && key[11] <= '3') {
        int n = 0;
        for (int r = 0; r < DFL_BROWS; r++)
            for (int c = 0; c < DFL_BCOLS; c++) n += DFL_BONUS_MAP[key[11] - '1'][r][c] == '#';
        *out = n;
        return 1;
    }
    if (!strncmp(key, "plan_boss_t", 11) && key[11] >= '1' && key[11] <= '4') { *out = DFL_STAGE[key[11] - '1'].boss_t; return 1; }
    /* the save */
    if (!strcmp(key, "save_best")) { *out = (int)dfs.best; return 1; }
    if (!strcmp(key, "save_runs")) { *out = dfs.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = dfs.wins; return 1; }
    if (!strcmp(key, "save_coins")) { *out = dfs.coins; return 1; }
    if (!strcmp(key, "save_cherries")) { *out = dfs.cherries; return 1; }
    if (!strcmp(key, "save_most")) { *out = dfs.most_cars; return 1; }
    if (!strcmp(key, "save_stage")) { *out = dfs.best_stage; return 1; }
    return 0;
}

static int dfl_cheat(const char *cmd) {
    int a, b;
    float x, y, vx, vy;
    if (sscanf(cmd, "run %d", &a) == 1) { start_run(iclamp(a, 1, 2)); return 1; }
    if (sscanf(cmd, "stage %d", &a) == 1) {
        dfl_start_stage(iclamp(a, 1, DFL_STAGES) - 1);
        set_state(DS_PLAY);
        music_play(DFL_MUS_STAGE[dfg.stage]);
        return 1;
    }
    if (!strcmp(cmd, "boss")) {
        dfg.spawn_i = DFL_STAGE[dfg.stage].count;
        dfg.stage_t = DFL_STAGE[dfg.stage].boss_t - 1;
        /* the swells that would have come and gone by now */
        dfg.swell_i = 0;
        while (dfg.swell_i < DFL_SWELL_COUNT && DFL_SWELLS[dfg.swell_i].t <= dfg.stage_t) dfg.swell_i++;
        return 1;
    }
    if (!strcmp(cmd, "nospawn")) { dfg.spawn_i = DFL_STAGE[dfg.stage].count; return 1; }
    if (!strcmp(cmd, "bossstill")) { dfg.boss_still = !dfg.boss_still; return 1; }
    if (sscanf(cmd, "parthp %d %d", &a, &b) == 2) {
        DflBoss *bs = &dfg.boss;
        bs->part_hp[iclamp(a, 0, 5)] = b;
        if (bs->kind == BOSS_ZEPHYR) bs->hp = imax(0, bs->part_hp[0]) + imax(0, bs->part_hp[1]) + imax(0, bs->part_hp[2]);
        return 1;
    }
    if (!strcmp(cmd, "quiet")) { dfg.spawn_i = DFL_STAGE[dfg.stage].count; dfg.stage_t = -1000000; return 1; }
    if (!strcmp(cmd, "god")) { dfg.god = !dfg.god; return 1; }
    if (!strcmp(cmd, "clear")) { dfl_clear_world(true); return 1; }
    if (!strcmp(cmd, "bot2")) { dfg.bot2 = !dfg.bot2; return 1; }
    if (sscanf(cmd, "score %d", &a) == 1) { dfg.score = (uint32_t)a; return 1; }
    if (sscanf(cmd, "spare %d", &a) == 1) { dfg.spare = a; dfl_note_cars(); return 1; }
    if (sscanf(cmd, "meter %d", &a) == 1) { dfg.car[0].meter = iclamp(a, 0, DFL_METER_MAX); return 1; }
    if (sscanf(cmd, "posboss %f", &x) == 1) { dfg.car[0].x = dfg.boss.x + x; dfg.car[0].vx = 0; return 1; }
    if (sscanf(cmd, "pos %f", &x) == 1) { dfg.car[0].x = x; dfg.car[0].vx = 0; return 1; }
    if (sscanf(cmd, "aim %f", &x) == 1) { dfg.car[0].aim = x; return 1; }
    if (sscanf(cmd, "foe %d %f %f %d", &a, &x, &y, &b) == 4) {
        /* a foe of a kind, here (and settled here), doing what it does (dir b) */
        int i = dfl_spawn_foe(iclamp(a, 0, FK_COUNT - 1), x, y, b, 0);
        if (i >= 0 && dfg.foe[i].kind == FK_PRISM) dfg.foe[i].arg = 3;
        if (i >= 0 && dfg.foe[i].kind == FK_HOG) dfg.foe[i].vx = b * 1.2f;
        if (i >= 0 && dfg.foe[i].kind == FK_TUMBLER) {
            dfg.foe[i].vx = b * 1.1f;
            dfg.foe[i].ground = y >= DFL_ROAD_Y - 8; /* up in the air if put there */
        }
        if (i >= 0 && dfg.foe[i].kind == FK_WRECK) dfg.foe[i].vx = -DFL_SCROLL * 0.7f;
        return 1;
    }
    if (sscanf(cmd, "dummy %d %f %f", &a, &x, &y) == 3) {
        /* a foe that sits still and does nothing */
        int i = dfl_spawn_foe(iclamp(a, 0, FK_COUNT - 1), x, y, 1, 0);
        if (i >= 0) { dfg.foe[i].still = 1; dfg.foe[i].hp = 1000; }
        return 1;
    }
    if (sscanf(cmd, "hp %d", &a) == 1) {
        for (int i = 0; i < DFL_MAX_FOES; i++) if (dfg.foe[i].alive) { dfg.foe[i].hp = a; break; }
        return 1;
    }
    if (sscanf(cmd, "eshot %f %f %f %f", &x, &y, &vx, &vy) == 4) { dfl_add_eshot(ES_PELLET, x, y, vx, vy); return 1; }
    if (sscanf(cmd, "bosshp %d", &a) == 1) {
        DflBoss *bs = &dfg.boss;
        bs->hp = a;
        if (bs->kind == BOSS_ZEPHYR) for (int k = 0; k < 3; k++) bs->part_hp[k] = k == 1 ? a : 0;
        return 1;
    }
    if (sscanf(cmd, "orbiters %d", &a) == 1) { for (int k = 0; k < 4; k++) dfg.boss.part_hp[k] = a; return 1; }
    if (sscanf(cmd, "attack %d", &a) == 1) { dfg.boss.hand_state = iclamp(a, 0, 3); dfg.boss.hand_t = 0; return 1; }
    if (!strncmp(cmd, "kill", 4)) {
        int p = 0;
        bool g = dfg.god;
        dfg.god = false;
        dfg.car[p].inv = 0;
        dfl_kill_car(p, CAUSE_TEST);
        dfg.god = g;
        return 1;
    }
    if (!strcmp(cmd, "bonus")) {
        dfl_bonus_start();
        set_state(DS_BONUS);
        dfg.stage_t = 0;
        music_play(DFL_MUS_BONUS);
        return 1;
    }
    if (sscanf(cmd, "blocks %d", &a) == 1) {
        /* keep only the first a blocks */
        int n = 0;
        for (int r = 0; r < DFL_BROWS; r++)
            for (int c = 0; c < DFL_BCOLS; c++)
                if (dfg.block[r][c]) { if (n >= a) dfg.block[r][c] = 0; else n++; }
        dfg.blocks_left = n;
        return 1;
    }
    if (sscanf(cmd, "ball %f %f %f %f", &x, &y, &vx, &vy) == 4) { dfg.bx = x; dfg.by = y; dfg.bvx = vx; dfg.bvy = vy; dfg.ball_live = true; return 1; }
    if (!strcmp(cmd, "win")) { dfg.stage = DFL_STAGES - 1; next_stage(); return 1; }
    if (!strcmp(cmd, "over")) {
        dfg.spare = 0;
        dfg.god = false;
        for (int p = 0; p < DFL_CARS; p++) { dfg.car[p].inv = 0; dfl_kill_car(p, CAUSE_TEST); }
        return 1;
    }
    return 0;
}

const GameDef GAME_DRIFTLINE = {
    "driftline",
    "DRIFTLINE",
    "1989",
    "ARCADE SHOOTER",
    "THE COAST ROAD, MORNING TO DAYBREAK. STEER TO AIM, DRIFT LEFT TO CHARGE.",
    {"WIN A BONUS COIN", "DRIVE THE WHOLE COAST ROAD", "FINISH THE DRIVE WITH 300,000 POINTS"},
    GLYPH_LEFT GLYPH_RIGHT "\tDRIVE; THE GUN SWINGS WITH YOU\n"
    GLYPH_LEFT "\tDRIFT: FILLS THE METER\n"
    GLYPH_UP "\tGUN BACK TO STRAIGHT UP\n"
    "HOLD " GLYPH_B "\tMAIN GUN, UP\n"
    "HOLD " GLYPH_A "\tSIDE GUNS, ALONG THE ROAD\n"
    "START\tPAUSE\n"
    "2 PLAYERS\tP1 DRIVES, P2 AIMS + FIRES\n"
    "\n"
    "GREY, GREEN, RED: THE REDDER\n"
    "THE METER, THE HARDER YOU HIT.",
    C_RED, C_CYAN,
    dfl_load, dfl_start, dfl_update, dfl_draw, dfl_quit, dfl_draw_label, dfl_query, dfl_cheat,
    "SEASIDE DRIVE", 48,
    NULL,
};
