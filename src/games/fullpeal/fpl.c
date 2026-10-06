/* FULL PEAL - into the screen, out to Knell. Cartridge 49 of UFO 40, a
 * tribute to Campanella 3 (UFO 50 #49). See docs/games/49-full-peal.md.
 * This file: the title and the code screen, the run from stage to stage
 * (radio, waves, grades, boss, balloon round), credits lost and the game
 * over, the ending and the final count, the records, and the test hooks.
 * The rules of flight are in fpl_play.c; Clary's console in fpl_micro.c. */
#include "fpl.h"

#define SAVE_MAGIC 0x46504C01u
#define CHERRY_SCORE 1500
#define CODE_HUGS "HUGSONLY"
#define RESTART_T 150
#define BONUS_IN_T 120
#define BONUS_END_T 200
#define BOSS_IN_T 90

FplSave fpsv;


static uint8_t prev_p2;

static void set_state(int s) {
    fpg.state = s;
    fpg.state_t = 0;
}

static void load_save(void) {
    FplSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { fpsv = tmp; return; }
    memset(&fpsv, 0, sizeof fpsv);
    fpsv.magic = SAVE_MAGIC;
}

static void save_now(void) {
    if (fpg.hugs) return; /* the code keeps the records out of it */
    game_save_write(game_current_index(), &fpsv, (int)sizeof fpsv);
}

void fpl_save_micro(int game, uint32_t score); /* called by the console */
void fpl_save_micro(int game, uint32_t score) {
    if (game < 0 || game >= FPL_MICROS || fpg.hugs) return;
    if (score > fpsv.micro[game]) {
        fpsv.micro[game] = score;
        game_save_write(game_current_index(), &fpsv, (int)sizeof fpsv);
    }
}

static void to_title(void) {
    set_state(PS_TITLE);
    game_set_pausable(false);
    fpl_micro_reset();
    fpg.radio = NULL;
    music_play(FPL_MUS_TITLE);
}

static int grades_total(void) {
    int n = 0;
    for (int s = 0; s < FPL_STAGES; s++)
        for (int w = 0; w < FPL_WAVES; w++) n += imax(0, fpg.grade[s][w]);
    return n;
}

static int stage_total(int s) {
    int n = 0;
    for (int w = 0; w < FPL_WAVES; w++) n += imax(0, fpg.grade[s][w]);
    return n;
}

static void enter_stage(int s) {
    fpl_start_stage(s);
    set_state(PS_RADIO);
    game_set_pausable(true);
    fpl_radio(FPL_STAGE[fpg.stage].radio);
    if (!fpg.hugs && fpsv.furthest < fpg.stage + 1) { fpsv.furthest = (uint8_t)(fpg.stage + 1); save_now(); }
    music_play(FPL_MUS_RADIO);
}

static void start_run(bool hugs) {
    input_consume();
    fpl_new_run(hugs);
    if (!hugs && fpsv.runs < 65535) { fpsv.runs++; save_now(); }
    enter_stage(0);
}

static void ending(void) {
    fpg.won = true;
    fpg.final_score = grades_total() + 100 * fpg.continues;
    if (!fpg.hugs) {
        game_award(GOAL_SAUCER);
        if (fpsv.furthest < 6) fpsv.furthest = 6;
        if (fpsv.wins < 65535) fpsv.wins++;
        save_now();
    }
    set_state(PS_ENDING);
    game_set_pausable(false);
    music_play(FPL_MUS_ENDING);
}

static void next_stage(void) {
    if (fpg.stage + 1 >= FPL_STAGES) { ending(); return; }
    enter_stage(fpg.stage + 1);
}

static void grade_wave(void) {
    int g = fpl_wave_grade();
    fpg.last_grade = g;
    fpg.grade[fpg.stage][fpg.wave] = g;
    fpg.owl = g == 100;
    fpg.meta = fpg.stage == 0 && fpg.wave == 0 && g == 50;
    if (g == 100) { fpg.perfect_stage++; fpg.perfects++; }
    if (g == 100 || g == 0) fpg.bonus_due = true;
    static char line[48];
    if (fpg.meta) snprintf(line, sizeof line, "WAVE 1: 50%%. HALF A PEAL IS STILL A PEAL.");
    else if (g == 100) snprintf(line, sizeof line, "WAVE %d: 100%%! THE OWL APPROVES.", fpg.wave + 1);
    else snprintf(line, sizeof line, "WAVE %d: %d%%", fpg.wave + 1, g);
    fpl_radio(line);
    set_state(PS_GRADE);
    music_play(g == 100 ? FPL_MUS_PERFECT : FPL_MUS_GRADE);
}

static void after_grade(void) {
    if (fpg.wave + 1 < FPL_WAVES) {
        fpl_start_wave(fpg.wave + 1);
        set_state(PS_WAVE);
        music_play(FPL_MUS_STAGE[fpg.stage]);
        return;
    }
    /* the stage's four waves are in: its best total is a record */
    if (!fpg.hugs && stage_total(fpg.stage) > fpsv.top[fpg.stage]) { fpsv.top[fpg.stage] = (uint16_t)stage_total(fpg.stage); save_now(); }
    fpl_boss_start(FPL_STAGE[fpg.stage].boss);
    fpl_radio(FPL_BOSS_NAME[fpg.boss.kind]);
    set_state(PS_BOSS);
    music_play(fpg.boss.kind == BOSS_SORDINA ? FPL_MUS_QUEEN : FPL_MUS_BOSS);
}

static void finish_bonus(void) {
    set_state(PS_BONUS_END);
    if (fpg.bonus_pts >= FPL_BONUS_NEED) {
        fpg.continues = imin(FPL_MAX_CONTINUES, fpg.continues + 1);
        fpg.bonuses_won++;
        fpl_radio("50 POINTS! ONE MORE CONTINUE.");
        if (!fpg.hugs) {
            game_award(GOAL_BEACON);
            if (fpsv.bonuses < 65535) fpsv.bonuses++;
            save_now();
        }
        music_play(FPL_MUS_PERFECT);
    } else {
        fpl_radio("NOT QUITE. ON WE GO.");
        music_play(FPL_MUS_GRADE);
    }
}

/* every ship of this credit is gone: a continue (the stage again from its
 * first wave) or the end */
static bool check_credit(void) {
    if (fpg.alive || fpg.lives > 0 || fpg.dead_t > 0) return false;
    if (fpg.continues > 0) {
        fpg.continues--;
        fpg.credits_lost++;
        set_state(PS_RESTART);
        music_play(FPL_MUS_OVER);
        fpl_radio("CREDIT USED. BACK TO THE FIRST WAVE.");
    } else {
        set_state(PS_OVER);
        game_set_pausable(false);
        fpl_radio("TINKLER DOWN. NO CREDITS LEFT.");
        music_play(FPL_MUS_OVER);
    }
    return true;
}

static void record_tally(void) {
    if (fpg.hugs) return;
    if (fpg.final_score > fpsv.best) fpsv.best = (uint16_t)fpg.final_score;
    if (fpg.perfects > fpsv.perfects) fpsv.perfects = (uint8_t)fpg.perfects;
    if (fpg.final_score >= CHERRY_SCORE) {
        game_award(GOAL_ALIEN);
        if (fpsv.cherries < 65535) fpsv.cherries++;
    }
    save_now();
}

/* ------------------------------------------------------------------ */
/* title and code                                                       */

static void check_code(void) {
    if (!strcmp(fpg.code, CODE_HUGS)) {
        fpg.hugs = !fpg.hugs;
        fpg.code_msg = fpg.hugs ? "HUGS-ONLY: NO GUNS. RECORDS AND GOALS ARE OFF" : "HUGS-ONLY IS OFF";
        sfx_play_name("fpl_extra");
    } else {
        fpg.code_msg = "NO SUCH CODE";
        sfx_play_name("ui_error");
    }
    fpg.code_msg_t = 150;
}

static void code_update(void) {
    if (fpg.code_msg_t > 0) fpg.code_msg_t--;
    char *c = &fpg.code[fpg.code_pos];
    if (btn_repeat(BTN_LEFT)) { fpg.code_pos = (fpg.code_pos + 7) % 8; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_RIGHT)) { fpg.code_pos = (fpg.code_pos + 1) % 8; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_UP)) { *c = (char)(*c == 'Z' ? 'A' : *c + 1); sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { *c = (char)(*c == 'A' ? 'Z' : *c - 1); sfx_play_name("ui_move"); }
    if (btnp(BTN_A) && fpg.state_t > 5) check_code();
    if (btnp(BTN_B)) { set_state(PS_TITLE); fpg.sel = 1; }
}

static void title_update(void) {
    game_set_pausable(false);
    fpg.scroll += 0.01f;
    if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { fpg.sel ^= 1; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if (fpg.state_t < 10 || !(btnp(BTN_A) || btnp(BTN_START))) return;
    if (fpg.sel == 1) {
        fpg.code_pos = 0;
        fpg.code_msg = NULL;
        set_state(PS_CODE);
        sfx_play_name("ui_ok");
        return;
    }
    sfx_play_name("fpl_start");
    start_run(fpg.hugs);
}

/* ------------------------------------------------------------------ */

static void fpl_update(void) {
    fpg.state_t++;
    fpg.frame_t++;
    if (fpg.radio) fpg.radio_t++;
    uint32_t held = input_held();
    uint8_t in = (uint8_t)(held & 0xFF);
    uint8_t p2 = (uint8_t)((held >> BTN_P2_SHIFT) & 0xFF);
    uint8_t p2p = p2 & ~prev_p2;
    prev_p2 = p2;
    bool console = fpg.state != PS_TITLE && fpg.state != PS_CODE;
    fpl_micro_update(p2, p2p, console);
    if (fpg.state != PS_TITLE && fpg.state != PS_CODE && fpg.state != PS_OVER) fpg.scroll += 0.012f;
    switch (fpg.state) {
    case PS_TITLE: title_update(); break;
    case PS_CODE: code_update(); break;
    case PS_RADIO:
        fpl_play_update(in);
        if (check_credit()) break;
        if (fpg.state_t >= FPL_RADIO_T && !fpg.sandbox) {
            fpl_start_wave(0);
            set_state(PS_WAVE);
            music_play(FPL_MUS_STAGE[fpg.stage]);
        }
        break;
    case PS_WAVE:
        fpl_play_update(in);
        if (check_credit()) break;
        if (!fpg.sandbox && fpl_wave_done()) grade_wave();
        break;
    case PS_GRADE:
        fpl_play_update(in);
        if (check_credit()) break;
        if (fpg.state_t >= FPL_GRADE_T) after_grade();
        break;
    case PS_BOSS:
        fpl_play_update(in);
        if (check_credit()) break;
        if (fpg.boss.dead) set_state(PS_BOSS_DOWN);
        break;
    case PS_BOSS_DOWN:
        fpl_play_update(in);
        if (check_credit()) break;
        if (fpg.state_t >= FPL_BOSS_FALL_T) {
            fpg.boss.on = false;
            if (fpg.bonus_due) {
                set_state(PS_BONUS_IN);
                fpl_radio("A BALLOON ROUND! 50 POINTS FOR A CONTINUE.");
                music_play(FPL_MUS_BONUS);
            } else {
                next_stage();
            }
        }
        break;
    case PS_BONUS_IN:
        fpl_ship_update(in);
        fpl_shots_update();
        if (fpg.state_t >= BONUS_IN_T) {
            fpl_bonus_start();
            set_state(PS_BONUS);
        }
        break;
    case PS_BONUS:
        fpl_bonus_update(in);
        if (fpl_bonus_over()) finish_bonus();
        break;
    case PS_BONUS_END:
        fpl_ship_update(in);
        fpl_shots_update();
        fpl_parts_update();
        if (fpg.state_t >= BONUS_END_T) next_stage();
        break;
    case PS_RESTART:
        fpl_parts_update();
        if (fpg.state_t >= RESTART_T) {
            fpg.lives = FPL_LIVES;
            fpg.alive = false;
            enter_stage(fpg.stage);
        }
        break;
    case PS_ENDING:
        fpl_parts_update();
        if (fpg.state_t >= 900 || (fpg.state_t > 300 && btnp(BTN_A))) {
            input_consume();
            set_state(PS_TALLY);
            record_tally();
            music_play(FPL_MUS_TALLY);
        }
        break;
    case PS_TALLY:
        if (fpg.state_t > 200 && (btnp(BTN_A) || btnp(BTN_START))) { input_consume(); to_title(); }
        break;
    case PS_OVER:
        game_set_pausable(false);
        /* the second player can keep playing the console here: only player
         * 1's buttons leave */
        if (fpg.state_t > 60 && (btnp(BTN_A) || btnp(BTN_START) || btnp(BTN_B))) { input_consume(); to_title(); }
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* cartridge interface                                                      */

static void fpl_load(void) {
    fpl_art_load();
    fpl_audio_load();
}

static void fpl_start(void) {
    load_save();
    memset(&fpg, 0, sizeof fpg);
    memcpy(fpg.code, "AAAAAAAA", 9);
    prev_p2 = 0;
    input_set_spare_p2(true);
    to_title();
}

static void fpl_quit(void) {
    input_set_spare_p2(false);
    save_now();
}

static int fpl_query(const char *key, int *out) {
    const FplBoss *b = &fpg.boss;
    if (!strcmp(key, "bot")) { *out = fpl_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = fpg.state; return 1; }
    if (!strcmp(key, "sel")) { *out = fpg.sel; return 1; }
    if (!strcmp(key, "hugs")) { *out = fpg.hugs; return 1; }
    if (!strcmp(key, "code_pos")) { *out = fpg.code_pos; return 1; }
    if (!strcmp(key, "stage")) { *out = fpg.stage + 1; return 1; }
    if (!strcmp(key, "wave")) { *out = fpg.wave + 1; return 1; }
    if (!strcmp(key, "lives")) { *out = fpg.lives; return 1; }
    if (!strcmp(key, "continues")) { *out = fpg.continues; return 1; }
    if (!strcmp(key, "deaths")) { *out = fpg.deaths; return 1; }
    if (!strcmp(key, "credits_lost")) { *out = fpg.credits_lost; return 1; }
    if (!strcmp(key, "alive")) { *out = fpg.alive; return 1; }
    if (!strcmp(key, "inv")) { *out = fpg.inv; return 1; }
    if (!strcmp(key, "x100")) { *out = (int)lroundf(fpg.x * 100); return 1; }
    if (!strcmp(key, "y100")) { *out = (int)lroundf(fpg.y * 100); return 1; }
    if (!strcmp(key, "col")) { *out = fpl_col(fpg.x); return 1; }
    if (!strcmp(key, "row")) { *out = fpl_row(fpg.y); return 1; }
    if (!strcmp(key, "side_dx")) { *out = fpg.side_dx; return 1; }
    if (!strcmp(key, "side_dy")) { *out = fpg.side_dy; return 1; }
    if (!strcmp(key, "fired_fwd")) { *out = fpg.fired_fwd; return 1; }
    if (!strcmp(key, "fired_side")) { *out = fpg.fired_side; return 1; }
    if (!strcmp(key, "cause")) { *out = fpg.cause; return 1; }
    if (!strcmp(key, "wave_total")) { *out = fpg.wave_total; return 1; }
    if (!strcmp(key, "wave_kills")) { *out = fpg.wave_kills; return 1; }
    if (!strcmp(key, "wave_escaped")) { *out = fpg.wave_escaped; return 1; }
    if (!strcmp(key, "wave_spawned")) { *out = fpg.wave_spawned; return 1; }
    if (!strcmp(key, "form_i")) { *out = fpg.form_i; return 1; }
    if (!strcmp(key, "last_grade")) { *out = fpg.last_grade; return 1; }
    if (!strcmp(key, "owl")) { *out = fpg.owl; return 1; }
    if (!strcmp(key, "meta")) { *out = fpg.meta; return 1; }
    if (!strcmp(key, "perfect_stage")) { *out = fpg.perfect_stage; return 1; }
    if (!strcmp(key, "perfects")) { *out = fpg.perfects; return 1; }
    if (!strcmp(key, "bonus_due")) { *out = fpg.bonus_due; return 1; }
    if (!strcmp(key, "grades")) { *out = grades_total(); return 1; }
    if (!strcmp(key, "final")) { *out = fpg.final_score; return 1; }
    if (!strcmp(key, "won")) { *out = fpg.won; return 1; }
    if (!strncmp(key, "grade_", 6) && key[6] >= 'a' && key[6] <= 'e' && key[7] >= '1' && key[7] <= '4') {
        *out = fpg.grade[key[6] - 'a'][key[7] - '1'];
        return 1;
    }
    if (!strncmp(key, "plan_total_", 11) && key[11] >= 'a' && key[11] <= 'e' && key[12] >= '1' && key[12] <= '4') {
        *out = fpl_wave_total(key[11] - 'a', key[12] - '1');
        return 1;
    }
    if (!strncmp(key, "plan_forms_", 11) && key[11] >= 'a' && key[11] <= 'e' && key[12] >= '1' && key[12] <= '4') {
        *out = fpl_wave_forms(key[11] - 'a', key[12] - '1');
        return 1;
    }
    if (!strcmp(key, "plan_waves")) {
        int n = 0;
        for (int s = 0; s < FPL_STAGES; s++) n += FPL_WAVES;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "plan_max_forms")) {
        int m = 0;
        for (int s = 0; s < FPL_STAGES; s++)
            for (int w = 0; w < FPL_WAVES; w++) m = imax(m, fpl_wave_forms(s, w));
        *out = m;
        return 1;
    }
    if (!strcmp(key, "plan_kinds")) {
        /* how many foe kinds the waves use */
        bool used[EK_COUNT] = {0};
        for (int s = 0; s < FPL_STAGES; s++)
            for (int w = 0; w < FPL_WAVES; w++) {
                const FplWave *wv = &FPL_STAGE[s].wave[w];
                for (int f = 0; f < wv->n; f++)
                    for (int k = 0; k < FPL_FORMS[wv->forms[f]].n; k++) used[FPL_FORMS[wv->forms[f]].s[k].kind] = true;
            }
        int n = 0;
        for (int k = 0; k < EK_COUNT; k++) n += used[k];
        *out = n;
        return 1;
    }
    if (!strcmp(key, "foes")) { *out = fpl_count_foes(-1); return 1; }
    if (!strncmp(key, "foes_", 5)) { *out = fpl_count_foes(atoi(key + 5)); return 1; }
    if (!strncmp(key, "kind_", 5)) {
        /* the first live foe of a kind: kind_hp_K kind_z100_K kind_x100_K kind_y100_K kind_state_K */
        const char *f = key + 5, *us = strrchr(key, '_');
        if (!us) return 0;
        int k = atoi(us + 1), i = -1;
        for (int j = 0; j < FPL_MAX_FOES; j++)
            if (fpg.foe[j].alive && fpg.foe[j].kind == k) { i = j; break; }
        *out = -999;
        if (i < 0) return 1;
        const FplFoe *e = &fpg.foe[i];
        if (!strncmp(f, "hp_", 3)) *out = e->hp;
        else if (!strncmp(f, "z100_", 5)) *out = (int)lroundf(e->z * 100);
        else if (!strncmp(f, "x100_", 5)) *out = (int)lroundf(e->x * 100);
        else if (!strncmp(f, "y100_", 5)) *out = (int)lroundf(e->y * 100);
        else if (!strncmp(f, "state_", 6)) *out = e->state;
        else if (!strncmp(f, "t_", 2)) *out = e->t;
        else return 0;
        return 1;
    }
    if (!strcmp(key, "eshots") || !strcmp(key, "eshots_plane") || !strcmp(key, "eshots_shell")) {
        int n = 0, want = !key[6] ? -1 : key[7] == 'p' ? ES_PLANE : ES_SHELL;
        for (int i = 0; i < FPL_MAX_ESHOTS; i++) n += fpg.es[i].alive && (want < 0 || fpg.es[i].kind == want);
        *out = n;
        return 1;
    }
    if (!strcmp(key, "eshots_ymin100") || !strcmp(key, "eshots_xspread100")) {
        /* the shots on the plane: the highest one, and how wide they are spread */
        float ymin = 9, xmin = 9, xmax = -9;
        for (int i = 0; i < FPL_MAX_ESHOTS; i++) {
            const FplEShot *e = &fpg.es[i];
            if (!e->alive || e->kind != ES_PLANE) continue;
            ymin = fminf(ymin, e->y);
            xmin = fminf(xmin, e->x);
            xmax = fmaxf(xmax, e->x);
        }
        *out = key[7] == 'y' ? (int)lroundf(ymin * 100) : xmax < xmin ? 0 : (int)lroundf((xmax - xmin) * 100);
        return 1;
    }
    if (!strcmp(key, "pshots") || !strcmp(key, "pshots_side") || !strcmp(key, "pshots_fwd")) {
        int n = 0, want = !key[6] ? -1 : key[7] == 's' ? 1 : 0;
        for (int i = 0; i < FPL_MAX_PSHOTS; i++) n += fpg.ps[i].alive && (want < 0 || fpg.ps[i].side == want);
        *out = n;
        return 1;
    }
    if (!strcmp(key, "side_vx100") || !strcmp(key, "side_vy100")) {
        /* the side shot fired last */
        const FplPShot *s = &fpg.ps[iclamp(fpg.last_side, 0, FPL_MAX_PSHOTS - 1)];
        *out = s->alive && s->side ? (int)lroundf((key[6] == 'x' ? s->vx : s->vy) * 100) : -999;
        return 1;
    }
    if (!strcmp(key, "boss_on")) { *out = b->on; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = b->dead; return 1; }
    if (!strcmp(key, "boss_kind")) { *out = b->on ? b->kind : -1; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = b->hp; return 1; }
    if (!strcmp(key, "boss_maxhp")) { *out = b->maxhp; return 1; }
    if (!strcmp(key, "boss_angry")) { *out = b->angry; return 1; }
    if (!strcmp(key, "boss_phase")) { *out = b->phase; return 1; }
    if (!strcmp(key, "boss_hits")) { *out = b->hits; return 1; }
    if (!strcmp(key, "boss_blocked")) { *out = b->blocked; return 1; }
    if (!strcmp(key, "boss_mouth")) { *out = b->mouth; return 1; }
    if (!strcmp(key, "boss_fired")) { *out = b->fired; return 1; }
    if (!strcmp(key, "boss_period")) { *out = b->period; return 1; }
    if (!strcmp(key, "boss_last_fan")) { *out = b->last_fan; return 1; }
    if (!strcmp(key, "boss_fy100")) { *out = (int)lroundf(b->fy * 100); return 1; }
    if (!strcmp(key, "boss_clary")) { *out = b->clary; return 1; }
    if (!strcmp(key, "boss_clary_fire")) { *out = b->clary_fire; return 1; }
    if (!strcmp(key, "boss_x100")) { *out = (int)lroundf(b->x * 100); return 1; }
    if (!strcmp(key, "boss_y100")) { *out = (int)lroundf(b->y * 100); return 1; }
    if (!strcmp(key, "fist0_x100")) { *out = (int)lroundf(b->fx[0] * 100); return 1; }
    if (!strcmp(key, "fist1_x100")) { *out = (int)lroundf(b->fx[1] * 100); return 1; }
    if (!strcmp(key, "weak_open")) { *out = fpl_boss_weak_open(); return 1; }
    if (!strcmp(key, "bonus_pts")) { *out = fpg.bonus_pts; return 1; }
    if (!strcmp(key, "bonus_orange")) { *out = fpg.bonus_orange; return 1; }
    if (!strcmp(key, "bonus_spawned")) { *out = fpg.bonus_spawned; return 1; }
    if (!strcmp(key, "bonus_won")) { *out = fpg.bonus_won; return 1; }
    if (!strcmp(key, "bonuses_won")) { *out = fpg.bonuses_won; return 1; }
    if (!strcmp(key, "balloons")) {
        int n = 0;
        for (int i = 0; i < FPL_BALLOONS; i++) n += fpg.bal[i].alive;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "radio_len")) { *out = fpg.radio ? (int)strlen(fpg.radio) : 0; return 1; }
    if (!strcmp(key, "text_missing")) {
        /* characters the fonts have no glyph for, in everything the cartridge writes */
        int n = tiny_missing("#%?!*-.,'/:");
        for (int s = 0; s < FPL_STAGES; s++) n += tiny_missing(FPL_STAGE[s].name) + tiny_missing(FPL_STAGE[s].radio);
        for (int k = 0; k < BOSS_COUNT; k++) n += text_missing(FPL_BOSS_NAME[k]) + tiny_missing(FPL_BOSS_NAME[k]);
        for (int g = 0; g < FPL_MICROS; g++) n += tiny_missing(fpl_micro_def(g)->name) + text_missing(fpl_micro_def(g)->name);
        static const char *const TINY[] = {"HUGS-ONLY: NO GUNS. RECORDS AND GOALS ARE OFF", "WAVE 1: 50%. HALF A PEAL IS STILL A PEAL.",
                                           "50 POINTS! ONE MORE CONTINUE.", "CREDIT USED. BACK TO THE FIRST WAVE.",
                                           "CLARY: HOLD ON, ANSEL! I'M COMING IN!", "A BALLOON ROUND! 50 POINTS FOR A CONTINUE.",
                                           "ANSEL AND THE TINKLER, OUT TO KNELL", "TOTAL 000000", "A PLAY   B OFF", "GAME OVER",
                                           "BEST!", "SCAN", "ANSEL / TINKLER", "CREDITS 2", "SHIPS", "HUGS"};
        for (int i = 0; i < ARRAY_LEN(TINY); i++) n += tiny_missing(TINY[i]);
        static const char *const MAIN[] = {"THE BELLS OF KNELL RING OUT AGAIN.", "TIME TO GO DOWN AND HAVE A LOOK.",
                                           "1,500 OR MORE: THE THIRD GOAL!", "CONTINUES LEFT 3 X 100 = 300",
                                           "HALF A PEAL IS STILL A PEAL", "RED 1  ORANGE 3  50 WINS A CONTINUE", "BALLOONS!",
                                           "THE FINAL COUNT", "FULL PEAL", "GAME OVER", "CREDIT USED", "100%"};
        for (int i = 0; i < ARRAY_LEN(MAIN); i++) n += text_missing(MAIN[i]);
        *out = n;
        return 1;
    }
    /* the console */
    if (!strcmp(key, "mc_state")) { *out = fpm.state; return 1; }
    if (!strcmp(key, "mc_sel")) { *out = fpm.sel + 1; return 1; }
    if (!strcmp(key, "mc_game")) { *out = fpm.game + 1; return 1; }
    if (!strcmp(key, "mc_score")) { *out = (int)fpm.score; return 1; }
    if (!strcmp(key, "mc_hold")) { *out = fpm.hold; return 1; }
    if (!strcmp(key, "mc_total")) { *out = (int)fpl_micro_total(); return 1; }
    if (!strcmp(key, "mc_gt")) { *out = fpm.gt; return 1; }
    if (!strcmp(key, "mc_n")) { *out = fpm.n; return 1; }
    if (!strncmp(key, "mc_i", 4) && key[4] >= '0' && key[4] <= '9') { *out = fpm.ivar[iclamp(atoi(key + 4), 0, 63)]; return 1; }
    if (!strncmp(key, "mc_f100_", 8)) { *out = (int)lroundf(fpm.fvar[iclamp(atoi(key + 8), 0, 63)] * 100); return 1; }
    if (!strncmp(key, "mc_hi", 5) && key[5] >= '0' && key[5] <= '9') { *out = (int)fpsv.micro[iclamp(atoi(key + 5) - 1, 0, 49)]; return 1; }
    /* the save */
    if (!strcmp(key, "save_best")) { *out = fpsv.best; return 1; }
    if (!strcmp(key, "save_runs")) { *out = fpsv.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = fpsv.wins; return 1; }
    if (!strcmp(key, "save_cherries")) { *out = fpsv.cherries; return 1; }
    if (!strcmp(key, "save_bonuses")) { *out = fpsv.bonuses; return 1; }
    if (!strcmp(key, "save_furthest")) { *out = fpsv.furthest; return 1; }
    if (!strcmp(key, "save_perfects")) { *out = fpsv.perfects; return 1; }
    if (!strncmp(key, "save_top_", 9) && key[9] >= 'a' && key[9] <= 'e') { *out = fpsv.top[key[9] - 'a']; return 1; }
    if (!strncmp(key, "bot_dbg", 7) && key[7] >= '0' && key[7] <= '3') { *out = fpl_bot_dbg[key[7] - '0']; return 1; }
    return 0;
}

static int fpl_cheat(const char *cmd) {
    int a, b, c, d;
    float x, y, vx, vy;
    if (!strcmp(cmd, "run")) { start_run(fpg.hugs); return 1; }
    if (sscanf(cmd, "stage %d", &a) == 1) {
        if (fpg.state == PS_TITLE) fpl_new_run(fpg.hugs);
        enter_stage(iclamp(a, 1, FPL_STAGES) - 1);
        return 1;
    }
    if (sscanf(cmd, "wave %d", &a) == 1) {
        fpl_start_wave(iclamp(a, 1, FPL_WAVES) - 1);
        set_state(PS_WAVE);
        music_play(FPL_MUS_STAGE[fpg.stage]);
        return 1;
    }
    if (!strcmp(cmd, "boss")) {
        fpl_clear_world();
        fpg.wave = FPL_WAVES - 1;
        fpl_boss_start(FPL_STAGE[fpg.stage].boss);
        set_state(PS_BOSS);
        music_play(fpg.boss.kind == BOSS_SORDINA ? FPL_MUS_QUEEN : FPL_MUS_BOSS);
        return 1;
    }
    if (!strcmp(cmd, "quiet")) {
        /* the wave releases nothing more */
        fpg.form_i = 999;
        return 1;
    }
    if (!strcmp(cmd, "endwave")) {
        /* the wave's formations are all done and every foe flies off */
        fpg.form_i = 999;
        for (int i = 0; i < FPL_MAX_FOES; i++) fpg.foe[i].alive = 0;
        return 1;
    }
    if (sscanf(cmd, "kills %d", &a) == 1) { fpg.wave_kills = a; return 1; }
    if (!strcmp(cmd, "bonus")) {
        fpg.boss.on = false;
        fpl_bonus_start();
        set_state(PS_BONUS);
        music_play(FPL_MUS_BONUS);
        return 1;
    }
    if (sscanf(cmd, "perfect %d", &a) == 1) { fpg.perfect_stage = a; return 1; }
    if (sscanf(cmd, "bonuspts %d", &a) == 1) { fpg.bonus_pts = a; return 1; }
    if (!strcmp(cmd, "god")) { fpg.god = !fpg.god; return 1; }
    if (!strcmp(cmd, "sandbox")) {
        /* the plane to ourselves: nothing comes, nothing ends (state WAVE) */
        fpg.sandbox = !fpg.sandbox;
        if (fpg.sandbox) { fpl_clear_world(); set_state(PS_WAVE); }
        return 1;
    }
    if (!strcmp(cmd, "still")) { fpg.still = !fpg.still; return 1; }
    if (sscanf(cmd, "lives %d", &a) == 1) { fpg.lives = a; return 1; }
    if (sscanf(cmd, "continues %d", &a) == 1) { fpg.continues = a; return 1; }
    if (sscanf(cmd, "pos %f %f", &x, &y) == 2) { fpg.x = x; fpg.y = y; return 1; }
    if (sscanf(cmd, "foe %d %d %d %d", &a, &b, &c, &d) == 4) { fpl_spawn_foe(iclamp(a, 0, EK_COUNT - 1), b, c, d); return 1; }
    if (sscanf(cmd, "foez %d %f %f %f", &a, &x, &y, &vx) == 4) {
        /* a foe of a kind at a spot and depth */
        int i = fpl_spawn_foe(iclamp(a, 0, EK_COUNT - 1), 0, 0, 0);
        if (i >= 0) { fpg.foe[i].x = x; fpg.foe[i].y = y; fpg.foe[i].z = vx; fpg.foe[i].t = 0; if (vx <= 0) { fpg.foe[i].state = FS_HOLD; fpg.foe[i].ty = y; } }
        return 1;
    }
    if (sscanf(cmd, "eshot %f %f %f %f", &x, &y, &vx, &vy) == 4) { fpl_add_eshot(ES_PLANE, x, y, 0, vx, vy, 0, BURST_NONE); return 1; }
    if (sscanf(cmd, "shell %f %f %d", &x, &y, &a) == 3) { fpl_shell_at(x, y, 0.5f, x, y, 30, a); return 1; }
    if (sscanf(cmd, "bosshp %d", &a) == 1) { fpg.boss.hp = a; return 1; }
    if (sscanf(cmd, "bosspos %f %f", &x, &y) == 2) { fpg.boss.x = x; fpg.boss.y = y; return 1; }
    if (!strcmp(cmd, "bossopen")) { fpg.boss.mouth = true; fpg.boss.shut_by_hit = false; fpg.boss.mouth_t = 150; return 1; }
    if (!strcmp(cmd, "kill")) {
        bool g = fpg.god;
        fpg.god = false;
        fpg.inv = 0;
        fpl_lose_ship(CAUSE_TEST);
        fpg.god = g;
        return 1;
    }
    if (!strcmp(cmd, "win")) { ending(); return 1; }
    if (sscanf(cmd, "grades %d", &a) == 1) {
        /* every wave flown so far given this grade (tests of the count) */
        for (int s = 0; s < FPL_STAGES; s++)
            for (int w = 0; w < FPL_WAVES; w++) fpg.grade[s][w] = a;
        return 1;
    }
    if (sscanf(cmd, "micro %d", &a) == 1) { fpl_micro_start(iclamp(a, 1, FPL_MICROS) - 1); return 1; }
    if (sscanf(cmd, "mcvar %d %d", &a, &b) == 2) { fpm.ivar[iclamp(a, 0, 63)] = b; return 1; }
    if (sscanf(cmd, "mcscore %d", &a) == 1) { fpm.score = (uint32_t)a; return 1; }
    return 0;
}

const GameDef GAME_FULLPEAL = {
    "fullpeal",
    "FULL PEAL",
    "1989",
    "ARCADE SHOOTER",
    "INTO THE SCREEN, OUT TO KNELL. TWO GUNS AND A GRADE FOR EVERY WAVE.",
    {"WIN A CONTINUE IN A BALLOON ROUND", "SILENCE QUEEN SORDINA", "FINISH THE RUN WITH 1,500 POINTS"},
    GLYPH_DPAD "\tFLY ANYWHERE ON THE PLANE\n"
    "HOLD " GLYPH_A "\tFORWARD GUN: INTO THE DISTANCE\n"
    "HOLD " GLYPH_B "\tSIDE BLASTER, AGAINST YOUR\n"
    "\tMOVE; HOLD IT TO KEEP AIM\n"
    "START\tPAUSE\n"
    "\n"
    "FAR OFF: FORWARD GUN. HERE ON\n"
    "THE PLANE: SIDE BLASTER.",
    C_NAVY, C_AMBER,
    fpl_load, fpl_start, fpl_update, fpl_draw, fpl_quit, fpl_draw_label, fpl_query, fpl_cheat,
    "CAMPANELLA 3", 49,
    NULL,
};
