/* SHOWSTOPPER - Clover's run against Morrow's show. Cartridge 43 of UFO 40,
 * a tribute to Elfazar's Hat (UFO 50 #43). See docs/games/43-showstopper.md.
 * This file: the title, the story, the map between stages, the stage-clear
 * and the offer of the Hop, the continue screen, the endings, the records,
 * the save and the test hooks. A stage is in sst_play.c, the foes in
 * sst_foes.c, the bosses in sst_boss.c, the Hop in sst_hop.c. */
#include "sst.h"

#define SAVE_MAGIC 0x53535431u

SstGame sst_g;
SstSave sst_sv;
bool sst_sim;

void sst_sfx(const char *name) {
    if (!sst_sim) sfx_play_name(name);
}

static void set_state(int s) {
    sst_g.state = s;
    sst_g.state_t = 0;
}

void sst_save_now(void) {
    if (!sst_sim) game_save_write(game_current_index(), &sst_sv, (int)sizeof sst_sv);
}

static void load_save(void) {
    SstSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { sst_sv = tmp; return; }
    memset(&sst_sv, 0, sizeof sst_sv);
    sst_sv.magic = SAVE_MAGIC;
}

/* the goals come back from the records (RESET GOALS clears both) */
static void goals_from_save(void) {
    if (sst_sv.lives_won > 0) game_award(GOAL_BEACON);
    if (sst_sv.wins > 0) game_award(GOAL_SAUCER);
    if (sst_sv.good_wins > 0) game_award(GOAL_ALIEN);
}

/* ---- the title's rows ------------------------------------------------------------ */

enum { MENU_1P, MENU_2P, MENU_FREE };
int sst_menu_rows(void) { return sst_sv.free_ride ? 3 : 2; }

static void to_title(void) {
    set_state(SS_TITLE);
    game_set_pausable(false);
    music_play(SST_MUS_TITLE);
    sst_g.practice = false;
}

static void record_run(void) {
    if (sst_g.passes_run > sst_sv.most_passes) sst_sv.most_passes = (uint16_t)sst_g.passes_run;
    if (sst_g.stage + 1 > sst_sv.best_stage) sst_sv.best_stage = (uint16_t)(sst_g.stage + 1);
    sst_save_now();
}

/* the music for where she is now */
static int play_music(void) {
    const SstBoss *b = &sst_g.boss;
    if (b->alive && !b->dying) {
        if (b->kind == BK_HEART) return SST_MUS_HEART;
        if (b->kind == BK_MAW) return SST_MUS_MAW;
        return SST_MUS_BOSS;
    }
    switch (sst_g.stage) {
    case 0: return SST_MUS_S1;
    case 1: return SST_MUS_S2;
    case 2: return sst_g.half ? SST_MUS_S3B : SST_MUS_S3A;
    case 3: return sst_g.half ? SST_MUS_S4B : SST_MUS_S4A;
    default: return SST_MUS_S5;
    }
}

static void begin_run(void) {
    sst_g.stage = 0;
    sst_g.lives = SST_START_LIVES;
    sst_g.continues = 0;
    sst_g.passes = 0;
    sst_g.found = 0;
    sst_g.passes_run = 0;
    sst_g.won = sst_g.good = false;
    memset(&sst_g.pl, 0, sizeof sst_g.pl);
    sst_g.pl.hp = SST_START_HP;
    sst_clear_powers();
    if (sst_sv.runs < 65535) sst_sv.runs++;
    sst_save_now();
}

static void to_map(void) {
    set_state(SS_MAP);
    game_set_pausable(true);
    music_play(SST_MUS_MAP);
}

static void to_play(void) {
    sst_start_stage(sst_g.stage);
    set_state(SS_PLAY);
    game_set_pausable(true);
    sst_g.prev = input_held() & 0xFF;
    music_play(play_music());
}

static void start_hop(int tries, bool practice) {
    sst_hop_start(tries, practice);
    sst_g.practice = practice;
    set_state(SS_HOP);
    game_set_pausable(true);
    sst_g.prev = input_held() & 0xFF;
    music_play(SST_MUS_HOP);
}

static void finish_run(bool good) {
    sst_g.won = true;
    sst_g.good = good;
    if (sst_sv.wins < 65535) sst_sv.wins++;
    if (good && sst_sv.good_wins < 65535) sst_sv.good_wins++;
    game_award(GOAL_SAUCER);
    if (good) game_award(GOAL_ALIEN);
    sst_g.stage = SST_STAGES - 1;
    record_run();
    set_state(SS_ENDING);
    game_set_pausable(false);
    music_play(good ? SST_MUS_GOOD : SST_MUS_BAD);
}

static bool fresh(int mask) { return btnp(mask); }

static void update_play(void) {
    uint32_t held = input_held() & 0xFF;
    int music_before = play_music();
    sst_play_step(held);
    int now = play_music();
    if (now != music_before) music_play(now);
    if (sst_g.pl.state == PS_GONE && sst_g.pl.st++ > 50) {
        set_state(SS_CONTINUE);
        sst_g.cont_sel = 0;
        sst_g.cont_t = 10 * 60;
        game_set_pausable(false);
        music_play(SST_MUS_OVER);
        return;
    }
    if (sst_g.stage_done) {
        sst_g.stage_done = false;
        if (sst_g.stage == SST_STAGES - 1) {
            finish_run(sst_g.boss.kind == BK_HEART);
            return;
        }
        set_state(SS_CLEAR);
        game_set_pausable(false);
        music_play(SST_MUS_CLEAR);
    }
}

static void update_hop(void) {
    uint32_t held = input_held() & 0xFF;
    int won_before = sst_g.hop.lives_won;
    sst_hop_step(held);
    if (sst_g.hop.lives_won > won_before && !sst_g.hop.practice) {
        if (sst_sv.lives_won < 65535) sst_sv.lives_won++;
        game_award(GOAL_BEACON);
        sst_save_now();
    }
    if (sst_g.hop.state == HB_DONE && sst_g.hop.st > 30) {
        if (sst_g.hop.coins > sst_sv.hop_best) sst_sv.hop_best = (uint16_t)imin(65535, sst_g.hop.coins);
        sst_save_now();
        set_state(SS_HOPEND);
        game_set_pausable(false);
        music_play(SST_MUS_CLEAR);
    }
}

static void sst_update(void) {
    sst_g.state_t++;
    switch (sst_g.state) {
    case SS_TITLE: {
        game_set_pausable(false);
        sst_g.frame_t++;
        int rows = sst_menu_rows();
        if (btn_repeat(BTN_UP)) { sst_g.menu = (sst_g.menu + rows - 1) % rows; sfx_play_name("ui_move"); }
        if (btn_repeat(BTN_DOWN)) { sst_g.menu = (sst_g.menu + 1) % rows; sfx_play_name("ui_move"); }
        if (fresh(BTN_B)) { game_exit_to_library(); break; }
        if ((fresh(BTN_A) || fresh(BTN_START)) && sst_g.state_t > 5) {
            if (sst_g.menu == MENU_2P) { sfx_play_name("ui_error"); break; } /* the next update */
            sfx_play_name("ui_ok");
            input_consume();
            if (sst_g.menu == MENU_FREE) { start_hop(3, true); break; }
            set_state(SS_STORY);
            music_play(SST_MUS_STORY);
        }
        break;
    }
    case SS_STORY:
        sst_g.frame_t++;
        if ((sst_g.state_t > 20 && (fresh(BTN_A) || fresh(BTN_START))) || sst_g.state_t > 1500) {
            input_consume();
            begin_run();
            to_map();
        }
        if (fresh(BTN_B)) to_title();
        break;
    case SS_MAP:
        sst_g.frame_t++;
        if ((sst_g.state_t > 40 && fresh(BTN_A)) || sst_g.state_t > 240) {
            input_consume();
            to_play();
        }
        break;
    case SS_PLAY:
        update_play();
        break;
    case SS_CLEAR:
        sst_g.frame_t++;
        if (sst_g.state_t > 150 || (sst_g.state_t > 60 && fresh(BTN_A))) {
            input_consume();
            record_run();
            sst_g.stage++;
            if (sst_g.passes > 0) {
                set_state(SS_OFFER);
                sst_g.offer_sel = 0;
                game_set_pausable(true);
                music_play(SST_MUS_MAP);
            } else {
                to_map();
            }
        }
        break;
    case SS_OFFER:
        sst_g.frame_t++;
        if (fresh(BTN_LEFT) || fresh(BTN_RIGHT) || fresh(BTN_UP) || fresh(BTN_DOWN)) { sst_g.offer_sel ^= 1; sfx_play_name("ui_move"); }
        if (fresh(BTN_A) && sst_g.state_t > 20) {
            input_consume();
            sfx_play_name("ui_ok");
            if (sst_g.offer_sel == 0) {
                int t = sst_g.passes;
                sst_g.passes = 0;
                start_hop(t, false);
            } else {
                to_map(); /* the passes are kept for a later break */
            }
        }
        break;
    case SS_HOP:
        update_hop();
        break;
    case SS_HOPEND:
        sst_g.frame_t++;
        if (sst_g.state_t > 60 && fresh(BTN_A)) {
            input_consume();
            if (sst_g.practice) to_title();
            else to_map();
        }
        break;
    case SS_CONTINUE:
        sst_g.frame_t++;
        if (sst_g.state_t < 30) break;
        if (fresh(BTN_LEFT) || fresh(BTN_RIGHT)) { sst_g.cont_sel ^= 1; sfx_play_name("ui_move"); }
        if (--sst_g.cont_t <= 0) sst_g.cont_sel = 1;
        if (fresh(BTN_A) || fresh(BTN_START) || sst_g.cont_t <= 0) {
            input_consume();
            if (sst_g.cont_sel == 0) {
                /* back to the start of this stage with a fresh credit; the
                 * true end is out of reach from now on */
                sst_g.continues++;
                sst_g.lives = SST_START_LIVES;
                sst_g.pl.hp = SST_START_HP;
                sst_clear_powers();
                to_play();
            } else {
                record_run();
                set_state(SS_OVER);
                music_play(SST_MUS_OVER);
            }
        }
        break;
    case SS_OVER:
        sst_g.frame_t++;
        if (sst_g.state_t > 60 && (fresh(BTN_A) || fresh(BTN_START))) {
            input_consume();
            to_title();
        }
        break;
    case SS_ENDING:
        sst_g.frame_t++;
        if (sst_g.state_t > 240 && (fresh(BTN_A) || fresh(BTN_START))) {
            input_consume();
            set_state(SS_CREDITS);
            music_play(SST_MUS_CREDITS);
        }
        break;
    case SS_CREDITS:
        sst_g.frame_t++;
        if (btn(BTN_A)) sst_g.state_t += 3;
        if (sst_g.state_t > 1500) {
            input_consume();
            to_title();
        }
        break;
    }
}

/* ------------------------------------------------------------------------------- */
/* cartridge interface                                                             */

static void sst_load(void) {
    sst_art_load();
    sst_audio_load();
}

static void sst_start(void) {
    load_save();
    memset(&sst_g, 0, sizeof sst_g);
    rng_seed(&sst_g.rng, (uint64_t)rng_next(&g_rng) + 43u);
    sst_g.cam_lock = -1;
    goals_from_save();
    to_title();
}

static void sst_quit(void) { sst_save_now(); }

static int count_foes(int kind) {
    int n = 0;
    for (int i = 0; i < SST_MAX_FOES; i++) n += sst_g.foe[i].alive && (kind < 0 || sst_g.foe[i].kind == kind);
    return n;
}

static int count_bullets(int kind) {
    int n = 0;
    for (int i = 0; i < SST_MAX_BULLETS; i++) n += sst_g.eb[i].alive && (kind < 0 || sst_g.eb[i].kind == kind);
    return n;
}

static int count_haz(int kind) {
    int n = 0;
    for (int i = 0; i < SST_MAX_HAZ; i++) n += sst_g.hz[i].alive && sst_g.hz[i].kind == kind;
    return n;
}

static int count_spawns(int stage, int kind) {
    const SstSpawn *list;
    int n, c = 0, keep = sst_g.half, keep_stage = sst_g.stage;
    sst_g.stage = stage;
    sst_g.half = 0;
    sst_stage_spawns(stage, &list, &n);
    for (int k = 0; k < n; k++) c += list[k].kind == kind;
    sst_g.half = keep;
    sst_g.stage = keep_stage;
    return c;
}

static int sst_query(const char *key, int *out) {
    const SstPlayer *p = &sst_g.pl;
    const SstBoss *b = &sst_g.boss;
    const SstHop *h = &sst_g.hop;
    if (!strcmp(key, "bot")) { *out = sst_bot_buttons(); return 1; }
    if (!strcmp(key, "bot_why")) { *out = sst_bot_why; return 1; }
    if (!strcmp(key, "state")) { *out = sst_g.state; return 1; }
    if (!strcmp(key, "menu")) { *out = sst_g.menu; return 1; }
    if (!strcmp(key, "menu_rows")) { *out = sst_menu_rows(); return 1; }
    if (!strcmp(key, "stage")) { *out = sst_g.stage + 1; return 1; }
    if (!strcmp(key, "half")) { *out = sst_g.half; return 1; }
    if (!strcmp(key, "lives")) { *out = sst_g.lives; return 1; }
    if (!strcmp(key, "continues")) { *out = sst_g.continues; return 1; }
    if (!strcmp(key, "passes")) { *out = sst_g.passes; return 1; }
    if (!strcmp(key, "passes_run")) { *out = sst_g.passes_run; return 1; }
    if (!strcmp(key, "found")) { *out = sst_g.found; return 1; }
    if (!strcmp(key, "won")) { *out = sst_g.won; return 1; }
    if (!strcmp(key, "good")) { *out = sst_g.good; return 1; }
    if (!strcmp(key, "hp")) { *out = p->hp; return 1; }
    if (!strcmp(key, "max_hp")) { *out = p->max_hp; return 1; }
    if (!strcmp(key, "px")) { *out = SST_PX(p->x); return 1; }
    if (!strcmp(key, "py")) { *out = SST_PX(p->y); return 1; }
    if (!strcmp(key, "pxf")) { *out = p->x; return 1; }
    if (!strcmp(key, "pyf")) { *out = p->y; return 1; }
    if (!strcmp(key, "pstate")) { *out = p->state; return 1; }
    if (!strcmp(key, "face")) { *out = p->face; return 1; }
    if (!strcmp(key, "firing")) { *out = p->firing; return 1; }
    if (!strcmp(key, "inv")) { *out = p->inv; return 1; }
    if (!strcmp(key, "bubble_t")) { *out = p->bub_t; return 1; }
    if (!strncmp(key, "pow", 3) && isdigit((unsigned char)key[3])) { *out = p->pow[iclamp(atoi(key + 3), 0, CC_COUNT - 1)]; return 1; }
    if (!strncmp(key, "shield", 6) && isdigit((unsigned char)key[6])) { *out = p->shield[iclamp(atoi(key + 6), 0, 4)]; return 1; }
    if (!strncmp(key, "hand", 4) && isdigit((unsigned char)key[4])) { *out = p->hand[iclamp(atoi(key + 4), 0, 2)]; return 1; }
    if (!strcmp(key, "nhand")) { *out = p->nhand; return 1; }
    if (!strcmp(key, "hand_res")) { *out = p->hand_res; return 1; }
    if (!strcmp(key, "hand_col")) { *out = p->hand_col; return 1; }
    if (!strcmp(key, "hands_a")) { *out = sst_g.hands_a; return 1; }
    if (!strcmp(key, "hands_b")) { *out = sst_g.hands_b; return 1; }
    if (!strcmp(key, "hands_none")) { *out = sst_g.hands_none; return 1; }
    if (!strcmp(key, "cards_dropped")) { *out = sst_g.cards_dropped; return 1; }
    if (!strcmp(key, "cards_got")) { *out = sst_g.cards_got; return 1; }
    if (!strcmp(key, "code_t")) { *out = sst_g.code_t; return 1; }
    if (!strcmp(key, "shots")) { *out = 0; for (int i = 0; i < SST_MAX_SHOTS; i++) *out += sst_g.ps[i].alive; return 1; }
    if (!strcmp(key, "optshots")) { *out = 0; for (int i = SST_MAX_SHOTS; i < SST_MAX_SHOTS + 8; i++) *out += sst_g.ps[i].alive; return 1; }
    if (!strcmp(key, "bigshots")) { *out = 0; for (int i = 0; i < SST_MAX_SHOTS; i++) *out += sst_g.ps[i].alive && sst_g.ps[i].big; return 1; }
    if (!strcmp(key, "shot0_vx")) { *out = 0; for (int i = 0; i < SST_MAX_SHOTS; i++) if (sst_g.ps[i].alive) { *out = sst_g.ps[i].vx; break; } return 1; }
    if (!strcmp(key, "shot0_vy")) { *out = 0; for (int i = 0; i < SST_MAX_SHOTS; i++) if (sst_g.ps[i].alive) { *out = sst_g.ps[i].vy; break; } return 1; }
    if (!strcmp(key, "shots_fired")) { *out = sst_g.shots_fired; return 1; }
    if (!strcmp(key, "dashes")) { *out = sst_g.dashes; return 1; }
    if (!strcmp(key, "stuns")) { *out = sst_g.stuns; return 1; }
    if (!strcmp(key, "kills")) { *out = sst_g.kills; return 1; }
    if (!strcmp(key, "hits")) { *out = sst_g.hits_taken; return 1; }
    if (!strcmp(key, "falls")) { *out = sst_g.falls; return 1; }
    if (!strcmp(key, "cam")) { *out = sst_g.cam_y; return 1; }
    if (!strcmp(key, "cam_lock")) { *out = sst_g.cam_lock; return 1; }
    if (!strcmp(key, "chasing")) { *out = sst_g.chasing; return 1; }
    if (!strcmp(key, "map_rows")) { *out = sst_map_rows(); return 1; }
    if (!strcmp(key, "foes")) { *out = sst_live_foes(); return 1; }
    if (!strncmp(key, "kind_", 5)) { *out = count_foes(atoi(key + 5)); return 1; }
    if (!strcmp(key, "bullets")) { *out = count_bullets(-1); return 1; }
    if (!strncmp(key, "eb_", 3)) { *out = count_bullets(atoi(key + 3)); return 1; }
    if (!strncmp(key, "hz_", 3)) { *out = count_haz(atoi(key + 3)); return 1; }
    if (!strcmp(key, "cards")) { *out = 0; for (int i = 0; i < SST_MAX_CARDS; i++) *out += sst_g.card[i].alive; return 1; }
    if (!strcmp(key, "pass_items")) { *out = 0; for (int i = 0; i < SST_MAX_PASSES; i++) *out += sst_g.pass[i].alive; return 1; }
    if (!strcmp(key, "bombs")) { *out = 0; for (int i = 0; i < SST_MAX_BOMBS; i++) *out += sst_g.bomb[i].alive; return 1; }
    if (!strcmp(key, "wave")) { *out = sst_g.wave; return 1; }
    if (!strcmp(key, "train_waves")) { *out = sst_train_wave_count(); return 1; }
    if (!strcmp(key, "lift_waves")) { *out = sst_lift_wave_count(); return 1; }
    if (!strcmp(key, "boss")) { *out = b->alive && !b->dying; return 1; }
    if (!strcmp(key, "boss_kind")) { *out = b->alive ? b->kind : 0; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = b->alive ? b->hp : 0; return 1; }
    if (!strcmp(key, "boss_maxhp")) { *out = b->alive ? b->maxhp : 0; return 1; }
    if (!strcmp(key, "boss_phase")) { *out = b->phase; return 1; }
    if (!strcmp(key, "boss_seen")) { *out = sst_g.boss_seen; return 1; }
    if (!strncmp(key, "part", 4) && isdigit((unsigned char)key[4])) {
        int i = atoi(key + 4);
        const char *u = strchr(key + 4, '_');
        if (i < 0 || i >= SST_MAX_PARTS || !u) return 0;
        const SstPart *pt = &b->part[i];
        if (!strcmp(u, "_alive")) { *out = pt->alive; return 1; }
        if (!strcmp(u, "_hp")) { *out = pt->hp; return 1; }
        if (!strcmp(u, "_shut")) { *out = pt->shut; return 1; }
        if (!strcmp(u, "_x")) { *out = SST_PX(pt->x); return 1; }
        if (!strcmp(u, "_y")) { *out = SST_PX(pt->y); return 1; }
        if (!strcmp(u, "_a")) { *out = pt->a; return 1; }
        if (!strcmp(u, "_kind")) { *out = pt->kind; return 1; }
        return 0;
    }
    if (!strncmp(key, "foe", 3) && isdigit((unsigned char)key[3])) {
        int i = atoi(key + 3);
        const char *u = strchr(key + 3, '_');
        if (i < 0 || i >= SST_MAX_FOES || !u) return 0;
        const SstFoe *f = &sst_g.foe[i];
        if (!strcmp(u, "_alive")) { *out = f->alive; return 1; }
        if (!strcmp(u, "_kind")) { *out = f->kind; return 1; }
        if (!strcmp(u, "_hp")) { *out = f->hp; return 1; }
        if (!strcmp(u, "_x")) { *out = SST_PX(f->x); return 1; }
        if (!strcmp(u, "_y")) { *out = SST_PX(f->y); return 1; }
        if (!strcmp(u, "_stun")) { *out = f->stun; return 1; }
        if (!strcmp(u, "_size")) { *out = f->size; return 1; }
        if (!strcmp(u, "_a")) { *out = f->a; return 1; }
        return 0;
    }
    /* the Hop */
    if (!strcmp(key, "hop_state")) { *out = h->state; return 1; }
    if (!strcmp(key, "hop_tries")) { *out = h->tries; return 1; }
    if (!strcmp(key, "hop_coins")) { *out = h->coins; return 1; }
    if (!strcmp(key, "hop_lives")) { *out = h->lives_won; return 1; }
    if (!strcmp(key, "hop_row")) { *out = h->row; return 1; }
    if (!strcmp(key, "hop_lane")) { *out = h->lane; return 1; }
    if (!strcmp(key, "hop_dir")) { *out = h->dir; return 1; }
    if (!strcmp(key, "hop_t")) { *out = h->hop_t; return 1; }
    if (!strcmp(key, "hop_cam")) { *out = h->cam; return 1; }
    if (!strcmp(key, "hop_gen")) { *out = h->gen_row; return 1; }
    if (!strcmp(key, "hop_mark")) { *out = h->next_mark; return 1; }
    if (!strcmp(key, "hop_practice")) { *out = h->practice; return 1; }
    if (!strcmp(key, "hop_tile")) { *out = sst_hop_tile(h->row, h->lane); return 1; }
    if (!strncmp(key, "hop_markat", 10)) { *out = SST_HOP_MARK[iclamp(atoi(key + 10), 0, HOP_MARKS - 1)]; return 1; }
    /* the structure, read from the stages */
    if (!strcmp(key, "n_stages")) { *out = SST_STAGES; return 1; }
    if (!strncmp(key, "rows_", 5)) { *out = SST_STAGE[iclamp(atoi(key + 5) - 1, 0, SST_STAGES - 1)].nrows; return 1; }
    if (!strncmp(key, "spots_", 6)) { *out = count_spawns(iclamp(atoi(key + 6) - 1, 0, SST_STAGES - 1), FK_SECRET); return 1; }
    if (!strncmp(key, "passdef_", 8)) { *out = SST_STAGE[iclamp(atoi(key + 8) - 1, 0, SST_STAGES - 1)].passes; return 1; }
    if (!strncmp(key, "spawns_", 7)) {
        /* spawns_S_K: how many of foe kind K stage S places */
        int s = atoi(key + 7);
        const char *u = strchr(key + 7, '_');
        if (!u) return 0;
        *out = count_spawns(iclamp(s - 1, 0, SST_STAGES - 1), atoi(u + 1));
        return 1;
    }
    if (!strcmp(key, "map_bad")) {
        *out = 0;
        for (int s = 0; s < SST_STAGES; s++)
            for (int r = 0; r < SST_STAGE[s].nrows; r++) *out += (int)strlen(SST_STAGE[s].rows[r]) != SST_COLS;
        return 1;
    }
    if (!strncmp(key, "guideat_", 8)) {
        int gx = atoi(key + 8);
        const char *u = strchr(key + 8, '_');
        *out = u ? sst_guide(gx, atoi(u + 1)) : -1;
        return 1;
    }
    if (!strcmp(key, "guide")) { *out = sst_guide(SST_PX(p->x) >> 4, SST_PX(p->y) >> 4); return 1; }
    if (!strcmp(key, "layout")) { *out = sst_layout_audit(); return 1; }
    if (!strcmp(key, "art_bad")) { *out = sst_art_check(); return 1; }
    if (!strcmp(key, "save_runs")) { *out = sst_sv.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = sst_sv.wins; return 1; }
    if (!strcmp(key, "save_good")) { *out = sst_sv.good_wins; return 1; }
    if (!strcmp(key, "save_stage")) { *out = sst_sv.best_stage; return 1; }
    if (!strcmp(key, "save_passes")) { *out = sst_sv.most_passes; return 1; }
    if (!strcmp(key, "save_hop")) { *out = sst_sv.hop_best; return 1; }
    if (!strcmp(key, "save_lives_won")) { *out = sst_sv.lives_won; return 1; }
    if (!strcmp(key, "free_ride")) { *out = sst_sv.free_ride; return 1; }
    return 0;
}

static int kind_by_name(const char *s) {
    for (int k = 1; k < FK_COUNT; k++) {
        char lower[24];
        snprintf(lower, sizeof lower, "%s", SST_FOE[k].name);
        for (char *c = lower; *c; c++) *c = *c == ' ' ? '_' : (char)tolower((unsigned char)*c);
        if (!strcmp(lower, s)) return k;
    }
    return isdigit((unsigned char)s[0]) ? atoi(s) : -1;
}

static void enter_stage(int s) {
    sst_g.stage = iclamp(s, 1, SST_STAGES) - 1;
    to_play();
}

static int sst_cheat(const char *cmd) {
    int a, b, c, d;
    char name[32];
    SstPlayer *p = &sst_g.pl;
    if (!strcmp(cmd, "run")) { begin_run(); enter_stage(1); return 1; }
    if (sscanf(cmd, "stage %d", &a) == 1) {
        if (sst_g.lives <= 0) begin_run();
        enter_stage(a);
        return 1;
    }
    if (sscanf(cmd, "tomap %d", &a) == 1) { sst_g.stage = iclamp(a, 1, SST_STAGES) - 1; to_map(); return 1; }
    if (!strcmp(cmd, "god")) { sst_g.god = true; return 1; }
    if (!strcmp(cmd, "mortal")) { sst_g.god = false; return 1; }
    if (!strcmp(cmd, "freeze")) { sst_g.frozen = true; return 1; }
    if (!strcmp(cmd, "thaw")) { sst_g.frozen = false; return 1; }
    if (!strcmp(cmd, "arena")) {
        /* nothing else on the screen, and nothing more comes */
        memset(sst_g.foe, 0, sizeof sst_g.foe);
        memset(sst_g.eb, 0, sizeof sst_g.eb);
        memset(sst_g.card, 0, sizeof sst_g.card);
        memset(sst_g.hz, 0, sizeof sst_g.hz);
        memset(&sst_g.boss, 0, sizeof sst_g.boss);
        const SstSpawn *list;
        int n;
        sst_stage_spawns(sst_g.stage, &list, &n);
        sst_g.next_spawn = n;
        sst_g.wave = 1000;
        sst_g.cam_lock = sst_g.cam_y;
        sst_g.chasing = false;
        return 1;
    }
    if (!strcmp(cmd, "clear")) {
        for (int i = 0; i < SST_MAX_FOES; i++) sst_g.foe[i].alive = 0;
        memset(sst_g.eb, 0, sizeof sst_g.eb);
        return 1;
    }
    if (!strcmp(cmd, "nobullets")) { memset(sst_g.eb, 0, sizeof sst_g.eb); return 1; }
    if (!strcmp(cmd, "nocards")) { memset(sst_g.card, 0, sizeof sst_g.card); return 1; }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) {
        p->x = a * SST_FX;
        p->y = b * SST_FX;
        p->safe_x = p->x;
        p->safe_y = p->y;
        return 1;
    }
    if (sscanf(cmd, "cam %d", &a) == 1) { sst_g.cam_y = a; return 1; }
    if (sscanf(cmd, "card %d", &a) == 1) { sst_give_card(iclamp(a, 1, 5)); return 1; }
    if (sscanf(cmd, "dropcard %d %d", &a, &b) == 2) { sst_drop_card(a * SST_FX, b * SST_FX); return 1; }
    if (sscanf(cmd, "pow %d %d", &a, &b) == 2) {
        a = iclamp(a, 1, 5);
        uint8_t h[3] = {(uint8_t)a, (uint8_t)a, (uint8_t)(b == 2 ? a : a % 5 + 1)};
        if (b > 0) sst_apply_hand(h);
        else p->pow[a] = 0;
        return 1;
    }
    if (sscanf(cmd, "lives %d", &a) == 1) { sst_g.lives = a; return 1; }
    if (sscanf(cmd, "hp %d", &a) == 1) { p->hp = a; return 1; }
    if (sscanf(cmd, "passes %d", &a) == 1) { sst_g.passes = a; return 1; }
    if (sscanf(cmd, "continues %d", &a) == 1) { sst_g.continues = a; return 1; }
    if (sscanf(cmd, "face %d", &a) == 1) { p->face = a & 7; return 1; }
    if (sscanf(cmd, "spawn %31s %d %d", name, &b, &c) == 3) {
        int k = kind_by_name(name);
        if (k <= 0 || k >= FK_COUNT) return 0;
        int j = sst_spawn_foe(k, b * SST_FX, (c + 0) * SST_FX);
        if (j >= 0 && k == FK_BAT) sst_g.foe[j].a = 1;
        return 1;
    }
    if (sscanf(cmd, "foestun %d %d", &a, &b) == 2) { if (a >= 0 && a < SST_MAX_FOES) sst_g.foe[a].stun = b; return 1; }
    if (sscanf(cmd, "foehp %d %d", &a, &b) == 2) { if (a >= 0 && a < SST_MAX_FOES) sst_g.foe[a].hp = b; return 1; }
    if (sscanf(cmd, "foet %d %d", &a, &b) == 2) { if (a >= 0 && a < SST_MAX_FOES) sst_g.foe[a].t = b; return 1; }
    if (sscanf(cmd, "boss %d %d %d", &a, &b, &c) == 3) { sst_boss_start(a, b * SST_FX, c * SST_FX); return 1; }
    if (sscanf(cmd, "bosshp %d", &a) == 1) {
        if (sst_g.boss.kind == BK_MAW) { sst_g.boss.part[0].hp = (a + 1) / 2; sst_g.boss.part[1].hp = a / 2; }
        else if (sst_g.boss.kind == BK_HEART) {
            for (int k = 0; k < 23; k++) if (sst_g.boss.part[k].alive) sst_g.boss.part[k].hp = imin(sst_g.boss.part[k].hp, 1);
        } else sst_g.boss.hp = a;
        return 1;
    }
    if (!strcmp(cmd, "breakgreens")) {
        /* the Heartstack's green blocks broken */
        for (int k = 0; k < SST_MAX_PARTS; k++)
            if (sst_g.boss.part[k].alive && sst_g.boss.part[k].kind == 9) sst_g.boss.part[k].alive = 0;
        return 1;
    }
    if (sscanf(cmd, "bosst %d", &a) == 1) { sst_g.boss.t = a; return 1; }
    if (sscanf(cmd, "bossphase %d %d", &a, &b) == 2) { sst_g.boss.phase = (uint8_t)a; sst_g.boss.pt = b; return 1; }
    if (!strcmp(cmd, "killboss")) {
        SstBoss *bs = &sst_g.boss;
        if (bs->alive && !bs->dying) {
            if (bs->kind == BK_MAW) bs->part[0].hp = bs->part[1].hp = 0;
            else if (bs->kind == BK_HEART) for (int k = 0; k < 23; k++) bs->part[k].alive = 0;
            bs->hp = 0;
        }
        return 1;
    }
    if (sscanf(cmd, "bullet %d %d %d %d %d", &a, &b, &c, &d, &(int){0}) >= 4) {
        int vy = 0;
        sscanf(cmd, "bullet %*d %*d %*d %*d %d", &vy);
        sst_add_bullet(a, b * SST_FX, c * SST_FX, d, vy);
        return 1;
    }
    if (!strcmp(cmd, "die")) { bool g = sst_g.god; sst_g.god = false; p->inv = 0; p->state = PS_WALK; sst_player_hurt(99); sst_g.god = g; return 1; }
    if (!strcmp(cmd, "done")) { sst_g.stage_done = true; return 1; }
    if (sscanf(cmd, "hop %d", &a) == 1) { start_hop(a, false); return 1; }
    if (sscanf(cmd, "hopcoins %d", &a) == 1) { sst_g.hop.coins = a; return 1; }
    if (sscanf(cmd, "hopat %d %d", &a, &b) == 2) { sst_g.hop.row = a; sst_g.hop.lane = b; return 1; }
    if (sscanf(cmd, "wave %d", &a) == 1) { sst_g.wave = a; return 1; }
    if (!strcmp(cmd, "freeride")) { sst_sv.free_ride = 1; return 1; }
    if (sscanf(cmd, "botdebug %d", &a) == 1) { extern int sst_bot_debug; sst_bot_debug = a; return 1; }
    if (sscanf(cmd, "menu %d", &a) == 1) { sst_g.menu = a; return 1; }
    return 0;
}

const GameDef GAME_SHOWSTOPPER = {
    "showstopper",
    "SHOWSTOPPER",
    "1988",
    "RUN AND GUN",
    "A SHOW RABBIT, A STOLEN WAND. HOLD TO AIM, DASH, PLAY YOUR CARDS.",
    {"WIN A LIFE IN THE HOP", "BRING DOWN MORROW'S TRUE FACE", "DO IT WITHOUT A CONTINUE"},
    "D-PAD\tWALK (8 WAYS)\n"
    GLYPH_A "\tSHOOT; HOLD: KEEP AIM, FIRE\n"
    "TAP " GLYPH_A "\tFASTER FIRE, NO AIM LOCK\n"
    GLYPH_B "\tDASH (SAFE, OVER PITS)\n"
    "CARDS\tPAIR: SMALL POWER\n"
    "  THREE OF A KIND: BIG POWER\n"
    "THE HOP\tD-PAD SETS THE NEXT HOP\n"
    "START\tPAUSE",
    C_WINE, C_YELLOW,
    sst_load, sst_start, sst_update, sst_draw, sst_quit, sst_draw_label, sst_query, sst_cheat,
    "ELFAZAR'S HAT", 43,
    NULL,
};
