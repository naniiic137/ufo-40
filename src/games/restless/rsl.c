/* RESTLESS - Old Gaunt will not stay dead.
 * Cartridge 38 of UFO 40, a tribute to Rakshasa (UFO 50 #38).
 * See docs/games/38-restless.md. This file: the title, the story, the run
 * from half to half, dying and coming back, the tally, the ending, the
 * high scores, the save and the test hooks. */
#include "rsl.h"

RslGame rg;
RslSave rgs;

#define SAVE_MAGIC 0x52534C01u

int rsl_main_h(void);

static const int *half_music(void) {
    static int m[RSL_HALVES];
    m[0] = RSL_MUS_S1A; m[1] = RSL_MUS_S1B; m[2] = RSL_MUS_S2A;
    m[3] = RSL_MUS_S2B; m[4] = RSL_MUS_S3A; m[5] = RSL_MUS_S3B;
    return m;
}

static void set_state(int s) {
    rg.state = s;
    rg.state_t = 0;
}

bool rsl_simulating;
uint32_t rsl_sim_cur, rsl_sim_prev;

void rsl_sfx(const char *name) {
    if (!rsl_simulating) sfx_play_name(name);
}
void rsl_music(int id) {
    if (!rsl_simulating) music_play(id);
}
void rsl_music_stop(void) {
    if (!rsl_simulating) music_stop();
}

void rsl_say(const char *m) {
    snprintf(rg.msg, sizeof rg.msg, "%s", m);
    rg.msg_t = 120;
}

void rsl_save_now(void) {
    rgs.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &rgs, (int)sizeof rgs);
}

static void load_save(void) {
    RslSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { rgs = tmp; return; }
    memset(&rgs, 0, sizeof rgs);
    rgs.magic = SAVE_MAGIC;
    memcpy(rgs.name, "OLD", 4);
}

/* the goals come back from the records (RESET GOALS clears both) */
static void goals_from_save(void) {
    if (rgs.most_revivals >= RSL_GIFT_REVIVALS) game_award(GOAL_BEACON);
    if (rgs.wins > 0) game_award(GOAL_SAUCER);
    if (rgs.best_won_score >= RSL_CHERRY_SCORE) game_award(GOAL_ALIEN);
}

void rsl_award_check(void) {
    if (rg.revivals >= RSL_GIFT_REVIVALS) game_award(GOAL_BEACON);
}

void rsl_add_score(int pts) {
    if (pts <= 0) return;
    rg.score += (uint32_t)pts;
    if (rg.score > 9999990u) rg.score = 9999990u;
    while (rg.score >= (uint32_t)rg.bonus_next) {
        rg.bonus_next += RSL_BONUS_EVERY;
        if (rg.bonus_pending < 3) rg.bonus_pending++;
    }
}

/* ------------------------------------------------------------------------ */

static void to_title(void) {
    set_state(RS_TITLE);
    game_set_pausable(false);
    music_play(RSL_MUS_TITLE);
}

static void start_half(int h) {
    rsl_load_half(h);
    rg.time = RSL_HALF_TIME;
    rg.msg_t = 0;
    music_play(half_music()[h]);
}

static void new_run(void) {
    memset(&rg.pl, 0, sizeof rg.pl);
    memset(&rg.owl, 0, sizeof rg.owl);
    rng_seed(&rg.rng, ((uint64_t)rng_next(&g_rng) << 32) | rng_next(&g_rng));
    rg.score = 0;
    rg.deaths = 1; /* he begins dead */
    rg.revivals = 0;
    rg.kills = 0;
    rg.won = false;
    rg.bonus_next = RSL_BONUS_EVERY;
    rg.bonus_pending = 0;
    rg.pl.weapon = WP_STAFF;
    start_half(0);
    if (rgs.runs < 65535) rgs.runs++;
    rsl_save_now();
}

static void begin_spirit(void) {
    rsl_spirit_start();
    set_state(RS_SPIRIT);
    game_set_pausable(true);
}

static void revive(void) {
    RslPlayer *p = &rg.pl;
    if (rg.pit >= 0) rsl_leave_pit();
    else {
        p->x = p->safe_x;
        p->y = p->safe_y;
    }
    p->vx = p->vy = 0;
    p->ground = true;
    p->lift = -1;
    p->lag = 0;
    p->duck = false;
    p->dropping = false;
    p->charging = false;
    p->charge = 0;
    p->inv = RSL_REVIVE_INV;
    p->weapon = WP_STAFF;
    rsl_camera_update(true);
    rsl_clear_screen(false);
    /* what was lying about goes too: a bell, a wheel */
    for (int i = 0; i < RSL_MAX_ITEMS; i++)
        if (rg.item[i].alive && rsl_in_view(RSL_PX(rg.item[i].x), RSL_PX(rg.item[i].y), 16)) rg.item[i].alive = 0;
    if (rg.time <= RSL_LATE_TIME) rg.time = RSL_LATE_TIME;
    rg.revivals++;
    if (rg.revivals > rgs.most_revivals) rgs.most_revivals = (uint16_t)imin(65535, rg.revivals);
    rsl_award_check();
    rg.quiet_t = 120;
    rg.lackey_t = imax(rg.lackey_t, 160);
    set_state(RS_PLAY);
    game_set_pausable(true);
    rsl_sfx("rsl_revive");
    if (rg.boss_on && !rg.boss_dead)
        music_play(rg.boss_kind == BOSS_KEEPER || rg.boss_kind == BOSS_TONGUE || rg.boss_kind == BOSS_RAMMER ? RSL_MUS_MIDBOSS : RSL_MUS_BOSS);
    else if (rg.pit >= 0 && RSL_HALF[rg.half].treasure_pit == rg.pit) music_play(RSL_MUS_SPECIAL);
    else music_play(half_music()[rg.half]);
}

static bool qualifies(void) {
    return rg.score > 0 && rg.score > rgs.top[RSL_HIGH_SCORES - 1].score;
}

static void run_over(void) {
    if (rg.deaths > rgs.most_deaths) rgs.most_deaths = (uint16_t)imin(65535, rg.deaths);
    rsl_save_now();
    if (qualifies()) {
        set_state(RS_NAME);
        memcpy(rg.name, rgs.name, 4);
        if (!isupper((unsigned char)rg.name[0])) memcpy(rg.name, "OLD", 4);
        rg.name_pos = 0;
        music_play(RSL_MUS_SCORES);
    } else {
        set_state(RS_SCORES);
        music_play(RSL_MUS_SCORES);
    }
}

static void enter_score(void) {
    int at = RSL_HIGH_SCORES - 1;
    while (at > 0 && rg.score > rgs.top[at - 1].score) at--;
    for (int i = RSL_HIGH_SCORES - 1; i > at; i--) rgs.top[i] = rgs.top[i - 1];
    RslScoreRow *r = &rgs.top[at];
    memset(r, 0, sizeof *r);
    r->score = rg.score;
    memcpy(r->name, rg.name, 3);
    r->name[3] = 0;
    r->stage = (uint8_t)(rg.half / 2 + 1);
    r->won = rg.won;
    memcpy(rgs.name, rg.name, 4);
    rg.name_slot = at;
    rsl_save_now();
}

static void win_run(void) {
    rg.won = true;
    if (rgs.wins < 65535) rgs.wins++;
    if (rg.score > rgs.best_won_score) rgs.best_won_score = rg.score;
    game_award(GOAL_SAUCER);
    if (rg.score >= RSL_CHERRY_SCORE) game_award(GOAL_ALIEN);
    rsl_save_now();
}

/* ------------------------------------------------------------------------ */

static void boss_trigger(void) {
    if (rg.boss_on || rg.boss_dead || rg.boss_spawn < 0 || rg.pit >= 0) return;
    const RslSpawn *s = &rg.spawn[rg.boss_spawn];
    int scx = s->tx / RSL_SW, scy = s->ty / RSL_SH;
    int px = RSL_PX(rg.pl.x), py = RSL_PX(rg.pl.y) - 1;
    /* the pair waits until he is in the middle, between them */
    int door = rg.boss_kind == BOSS_PAIR ? 150 : 36;
    if (px < scx * RSL_SW * RSL_TILE + door || px >= (scx + 1) * RSL_SW * RSL_TILE) return;
    if (py / (RSL_SH * RSL_TILE) != scy) return;
    rg.lock_x = scx * RSL_SW * RSL_TILE;
    rg.lock_y = scy * RSL_SH * RSL_TILE + RSL_SH * RSL_TILE - SCREEN_H;
    rsl_camera_update(true);
    for (int i = 0; i < RSL_MAX_FOES; i++)
        if (rg.foe[i].alive) {
            if (rg.foe[i].spawn >= 0) rg.spawn[rg.foe[i].spawn].child = -1;
            rg.foe[i].alive = 0;
        }
    rsl_boss_start();
}

void rsl_play_step(void) {
    if (!rsl_simulating) game_set_pausable(true);
    if (rg.msg_t > 0) rg.msg_t--;
    if (!rg.boss_dead && !rg.no_timer) {
        int step = rg.deaths >= 6 ? 2 : 1; /* six deaths: the clock runs twice as fast */
        if (rg.time > 0) {
            rg.time = imax(0, rg.time - step);
            if (rg.time == 0) { rsl_say("TIME IS UP: THE CLOCKFIRES COME"); rsl_sfx("rsl_timeup"); }
            else if (rg.time <= 10 * 60 && rg.time % 60 < step) rsl_sfx("rsl_tick");
        }
    }
    rsl_world_update();
    if (rg.state != RS_PLAY) return;
    boss_trigger();
    if (rg.boss_dead) {
        rg.boss_done_t++;
        if (rg.boss_done_t == 60) rsl_music(RSL_MUS_CLEAR);
        if (rg.boss_done_t > 240) {
            /* the time bonus: seconds left, rounded up to a multiple of five, times 50 */
            int secs = (rg.time + 59) / 60;
            rg.tally_secs = (secs + 4) / 5 * 5; /* rounded up to the next 5 */
            rg.tally_bonus = rg.tally_secs * 50;
            rsl_add_score(rg.tally_bonus);
            set_state(RS_TALLY);
        }
    }
}

static void name_update(void) {
    static const char SET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-";
    int n = (int)sizeof SET - 1;
    char *c = &rg.name[rg.name_pos];
    const char *p = strchr(SET, *c);
    int idx = p && *c ? (int)(p - SET) : 0;
    if (btn_repeat(BTN_UP)) { idx = (idx + 1) % n; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { idx = (idx + n - 1) % n; sfx_play_name("ui_move"); }
    *c = SET[idx];
    if (btnp(BTN_RIGHT) && rg.name_pos < 2) { rg.name_pos++; sfx_play_name("ui_move"); }
    if (btnp(BTN_LEFT) && rg.name_pos > 0) { rg.name_pos--; sfx_play_name("ui_move"); }
    if (btnp(BTN_B) && rg.name_pos > 0) { rg.name_pos--; sfx_play_name("ui_back"); }
    if (btnp(BTN_A) || btnp(BTN_START)) {
        sfx_play_name("ui_ok");
        if (rg.name_pos < 2 && !btnp(BTN_START)) rg.name_pos++;
        else {
            input_consume();
            enter_score();
            set_state(RS_SCORES);
        }
    }
}

static void rsl_update(void) {
    rg.state_t++;
    rg.frame_t++;
    switch (rg.state) {
    case RS_TITLE:
        game_set_pausable(false);
        if (btnp(BTN_UP) || btnp(BTN_DOWN)) { rg.menu ^= 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if ((btnp(BTN_A) || btnp(BTN_START)) && rg.state_t > 5) {
            sfx_play_name("ui_ok");
            input_consume();
            if (rg.menu == 1) { set_state(RS_SCORES); break; }
            set_state(RS_STORY);
            music_play(RSL_MUS_STORY);
        }
        break;
    case RS_STORY:
        game_set_pausable(false);
        if ((rg.state_t > 20 && (btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_START))) || rg.state_t > 1100) {
            input_consume();
            new_run();
            /* the run opens with Gaunt already dead */
            begin_spirit();
        }
        break;
    case RS_PLAY:
        rsl_play_step();
        break;
    case RS_DYING:
        game_set_pausable(true);
        for (int i = 0; i < RSL_MAX_PARTS; i++)
            if (rg.part[i].life > 0) { rg.part[i].life--; rg.part[i].x += rg.part[i].vx; rg.part[i].y += rg.part[i].vy; }
        if (rg.state_t > 60) begin_spirit();
        break;
    case RS_SPIRIT:
        game_set_pausable(true);
        rsl_spirit_update();
        if (rg.sp.failed && rg.sp.done_t > 80) {
            set_state(RS_OVER);
            game_set_pausable(false);
            music_play(RSL_MUS_OVER);
        } else if (!rg.sp.failed && rg.sp.got >= rg.sp.n && rg.sp.done_t > 50) {
            set_state(RS_REVIVE);
        }
        break;
    case RS_REVIVE:
        game_set_pausable(true);
        if (rg.state_t > 30) revive();
        break;
    case RS_TALLY:
        game_set_pausable(false);
        if (rg.state_t > 200 || (rg.state_t > 60 && btnp(BTN_A))) {
            input_consume();
            if (rg.half >= RSL_HALVES - 1) {
                win_run();
                set_state(RS_ENDING);
                music_play(RSL_MUS_ENDING);
            } else {
                start_half(rg.half + 1);
                set_state(RS_PLAY);
            }
        }
        break;
    case RS_OVER:
        game_set_pausable(false);
        if (rg.state_t > 100 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            run_over();
        }
        break;
    case RS_NAME:
        game_set_pausable(false);
        name_update();
        break;
    case RS_ENDING:
        game_set_pausable(false);
        if (btn(BTN_A)) rg.state_t += 3;
        if (rg.state_t > 1500) {
            input_consume();
            run_over();
        }
        break;
    case RS_SCORES:
        game_set_pausable(false);
        if (rg.state_t > 20 && (btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_START))) {
            input_consume();
            to_title();
        }
        break;
    default: break;
    }
}

/* ------------------------------------------------------------------------ */
/* cartridge interface                                                      */

static void rsl_load(void) {
    rsl_art_load();
    rsl_audio_load();
}

static void rsl_start(void) {
    load_save();
    memset(&rg, 0, sizeof rg);
    rng_seed(&rg.rng, 38);
    goals_from_save();
    to_title();
}

static void rsl_quit(void) { rsl_save_now(); }

static int count_items(int kind) {
    int n = 0;
    for (int i = 0; i < RSL_MAX_ITEMS; i++) n += rg.item[i].alive && (kind < 0 || rg.item[i].kind == kind);
    return n;
}

static int first_foe(int kind) {
    for (int i = 0; i < RSL_MAX_FOES; i++)
        if (rg.foe[i].alive && rg.foe[i].kind == kind) return i;
    return -1;
}

static int count_spawns(int type, int a, bool used_only) {
    int n = 0;
    for (int i = 0; i < rg.nspawn; i++)
        if (rg.spawn[i].type == type && (a < 0 || rg.spawn[i].a == a) && (!used_only || rg.spawn[i].used)) n++;
    return n;
}

static int rsl_query(const char *key, int *out) {
    const RslPlayer *p = &rg.pl;
    if (!strcmp(key, "bot")) { *out = rsl_bot_buttons(); return 1; }
    if (!strcmp(key, "bot_last")) { extern int rsl_bot_last; *out = rsl_bot_last; return 1; }
    if (!strcmp(key, "state")) { *out = rg.state; return 1; }
    if (!strcmp(key, "menu")) { *out = rg.menu; return 1; }
    if (!strcmp(key, "half")) { *out = rg.half; return 1; }
    if (!strcmp(key, "stage")) { *out = rg.half / 2 + 1; return 1; }
    if (!strcmp(key, "score")) { *out = (int)rg.score; return 1; }
    if (!strcmp(key, "deaths")) { *out = rg.deaths; return 1; }
    if (!strcmp(key, "revivals")) { *out = rg.revivals; return 1; }
    if (!strcmp(key, "hits")) { *out = rg.hits; return 1; }
    if (!strcmp(key, "kills")) { *out = rg.kills; return 1; }
    if (!strcmp(key, "time")) { *out = (rg.time + 59) / 60; return 1; }
    if (!strcmp(key, "time_frames")) { *out = rg.time; return 1; }
    if (!strcmp(key, "bonus_pending")) { *out = rg.bonus_pending; return 1; }
    if (!strcmp(key, "bonus_next")) { *out = rg.bonus_next; return 1; }
    if (!strcmp(key, "tally_bonus")) { *out = rg.tally_bonus; return 1; }
    if (!strcmp(key, "won")) { *out = rg.won; return 1; }
    if (!strcmp(key, "weapon")) { *out = p->weapon; return 1; }
    if (!strcmp(key, "px")) { *out = RSL_PX(p->x); return 1; }
    if (!strcmp(key, "py")) { *out = RSL_PX(p->y); return 1; }
    if (!strcmp(key, "px256")) { *out = p->x; return 1; }
    if (!strcmp(key, "py256")) { *out = p->y; return 1; }
    if (!strcmp(key, "ground")) { *out = p->ground; return 1; }
    if (!strcmp(key, "duck")) { *out = p->duck; return 1; }
    if (!strcmp(key, "lag")) { *out = p->lag; return 1; }
    if (!strcmp(key, "face")) { *out = p->face; return 1; }
    if (!strcmp(key, "inv")) { *out = p->inv; return 1; }
    if (!strcmp(key, "charge")) { *out = p->charge; return 1; }
    if (!strcmp(key, "lift")) { *out = p->lift; return 1; }
    if (!strcmp(key, "fired")) { *out = p->shots_fired; return 1; }
    if (!strcmp(key, "owl")) { *out = rg.owl.on; return 1; }
    if (!strcmp(key, "owl_target")) { *out = rg.owl.target; return 1; }
    if (!strcmp(key, "owl_flag")) { *out = rg.owl.flag_t; return 1; }
    if (!strcmp(key, "cam_x")) { *out = rg.cam_x; return 1; }
    if (!strcmp(key, "cam_y")) { *out = rg.cam_y; return 1; }
    if (!strcmp(key, "lock")) { *out = rg.lock_x; return 1; }
    if (!strcmp(key, "pit")) { *out = rg.pit; return 1; }
    if (!strcmp(key, "pit_left")) {
        /* the pit room's foe spots not yet done with */
        int n = 0;
        for (int i = 0; i < rg.nspawn; i++) {
            const RslSpawn *s = &rg.spawn[i];
            if (s->type == SP_FOE && s->ty >= rsl_main_h() && s->tx / RSL_SW == rg.pit && !s->used && s->foe != FO_CRAB) n++;
        }
        *out = n;
        return 1;
    }
    if (!strcmp(key, "pit_done_t")) { *out = rg.pit_done_t; return 1; }
    if (!strcmp(key, "writing")) {
        /* the words on the first pit's wall of the falls show from 5 deaths */
        extern char rsl_hint(int tx, int ty);
        int n = 0;
        if (rg.pit >= 0 && rg.deaths >= 5)
            for (int ty = rsl_main_h(); ty < rg.mh; ty++)
                for (int tx = rg.pit * RSL_SW; tx < rg.pit * RSL_SW + RSL_SW; tx++) n += rsl_hint(tx, ty) == 'Y';
        *out = n;
        return 1;
    }
    if (!strcmp(key, "section_x")) { *out = RSL_PX(rg.section_x); return 1; }
    if (!strcmp(key, "map_w")) { *out = rg.mw * RSL_TILE; return 1; }
    if (!strcmp(key, "main_h")) { *out = rsl_main_h() * RSL_TILE; return 1; }
    if (!strcmp(key, "boss_on")) { *out = rg.boss_on; return 1; }
    if (!strcmp(key, "boss_kind")) { *out = rg.boss_kind; return 1; }
    if (!strcmp(key, "lackeys_made")) { *out = rg.lackeys_made; return 1; }
    if (!strcmp(key, "greens_made")) { *out = rg.greens_made; return 1; }
    if (!strcmp(key, "art_bad")) { extern int rsl_art_check(void); *out = rsl_art_check(); return 1; }
    if (!strncmp(key, "hp_of_", 6)) { *out = RSL_FOE_HP[iclamp(atoi(key + 6), 0, FO_KINDS - 1)]; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = rg.boss_dead; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = rsl_boss_hp_total(); return 1; }
    if (!strcmp(key, "boss_blue")) { int b = first_foe(FO_KEEPER); *out = b >= 0 ? (rg.foe[b].flags & FF_BLUE) != 0 : -1; return 1; }
    if (!strcmp(key, "boss_state")) {
        *out = -1;
        for (int i = 0; i < RSL_MAX_FOES; i++)
            if (rg.foe[i].alive && RSL_IS_BOSS(rg.foe[i].kind)) { *out = rg.foe[i].state; break; }
        return 1;
    }
    if (!strcmp(key, "foes")) { *out = rsl_foes_alive(false); return 1; }
    if (!strncmp(key, "foes_", 5)) { *out = rsl_count_foes(atoi(key + 5)); return 1; }
    if (!strncmp(key, "foe", 3) && isdigit((unsigned char)key[3])) {
        int i = atoi(key + 3);
        const char *u = strchr(key + 3, '_');
        if (i < 0 || i >= RSL_MAX_FOES || !u) return 0;
        const RslFoe *f = &rg.foe[i];
        if (!strcmp(u, "_alive")) { *out = f->alive; return 1; }
        if (!strcmp(u, "_kind")) { *out = f->kind; return 1; }
        if (!strcmp(u, "_hp")) { *out = f->alive ? f->hp : 0; return 1; }
        if (!strcmp(u, "_x")) { *out = RSL_PX(f->x); return 1; }
        if (!strcmp(u, "_y")) { *out = RSL_PX(f->y); return 1; }
        if (!strcmp(u, "_state")) { *out = f->state; return 1; }
        if (!strcmp(u, "_green")) { *out = (f->flags & FF_GREEN) != 0; return 1; }
        if (!strcmp(u, "_harmless")) { *out = (f->flags & FF_HARMLESS) != 0; return 1; }
        if (!strcmp(u, "_var")) { *out = f->var; return 1; }
        if (!strcmp(u, "_blue")) { *out = (f->flags & FF_BLUE) != 0; return 1; }
        return 0;
    }
    if (!strcmp(key, "eshots")) { int n = 0; for (int i = 0; i < RSL_MAX_ESHOTS; i++) n += rg.es[i].alive; *out = n; return 1; }
    if (!strncmp(key, "eshots_", 7)) {
        int k = atoi(key + 7), n = 0;
        for (int i = 0; i < RSL_MAX_ESHOTS; i++) n += rg.es[i].alive && rg.es[i].kind == k;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "shots")) { *out = rsl_shots_alive(-1); return 1; }
    if (!strcmp(key, "shot0_dmg")) { *out = -1; for (int i = 0; i < RSL_MAX_SHOTS; i++) if (rg.shot[i].alive) { *out = rg.shot[i].dmg; break; } return 1; }
    if (!strcmp(key, "shot0_vx")) { *out = -9999; for (int i = 0; i < RSL_MAX_SHOTS; i++) if (rg.shot[i].alive) { *out = rg.shot[i].vx; break; } return 1; }
    if (!strcmp(key, "shot0_vy")) { *out = -9999; for (int i = 0; i < RSL_MAX_SHOTS; i++) if (rg.shot[i].alive) { *out = rg.shot[i].vy; break; } return 1; }
    if (!strcmp(key, "shot0_charged")) { *out = -1; for (int i = 0; i < RSL_MAX_SHOTS; i++) if (rg.shot[i].alive) { *out = rg.shot[i].charged; break; } return 1; }
    if (!strcmp(key, "shot0_life")) { *out = -1; for (int i = 0; i < RSL_MAX_SHOTS; i++) if (rg.shot[i].alive) { *out = rg.shot[i].life; break; } return 1; }
    if (!strcmp(key, "hovering")) { int n = 0; for (int i = 0; i < RSL_MAX_SHOTS; i++) n += rg.shot[i].alive && rg.shot[i].hover > 0; *out = n; return 1; }
    if (!strcmp(key, "items")) { *out = count_items(-1); return 1; }
    if (!strncmp(key, "item_y_", 7)) {
        int k = atoi(key + 7);
        *out = -999;
        for (int i = 0; i < RSL_MAX_ITEMS; i++) if (rg.item[i].alive && rg.item[i].kind == k) { *out = RSL_PX(rg.item[i].y); break; }
        return 1;
    }
    if (!strncmp(key, "items_", 6)) { *out = count_items(atoi(key + 6)); return 1; }
    if (!strcmp(key, "torches")) { *out = count_spawns(SP_TORCH, -1, false); return 1; }
    if (!strcmp(key, "torches_lit")) { *out = count_spawns(SP_TORCH, -1, true); return 1; }
    if (!strcmp(key, "secrets")) { *out = rg.half_secrets; return 1; }
    if (!strcmp(key, "secrets_found")) { *out = rg.secrets_found; return 1; }
    if (!strcmp(key, "lifts")) { *out = rg.nlift; return 1; }
    if (!strcmp(key, "launchers")) { *out = count_spawns(SP_LAUNCHER, -1, false); return 1; }
    if (!strcmp(key, "firewalls")) { *out = count_spawns(SP_FIREWALL, -1, false); return 1; }
    if (!strcmp(key, "rockfalls")) { *out = count_spawns(SP_ROCKFALL, -1, false); return 1; }
    if (!strcmp(key, "pieces")) { *out = rg.sp.n; return 1; }
    if (!strcmp(key, "pieces_got")) { *out = rg.sp.got; return 1; }
    if (!strcmp(key, "guards")) { *out = rg.sp.nguards; return 1; }
    if (!strcmp(key, "flames")) { int n = 0; for (int i = 0; i < rg.sp.nguards; i++) n += rg.sp.guard[i].piece < 0; *out = n; return 1; }
    if (!strcmp(key, "spirit_failed")) { *out = rg.sp.failed; return 1; }
    if (!strcmp(key, "glow_torches")) { *out = rg.sp.ntorch; return 1; }
    if (!strcmp(key, "spirit_inv")) { *out = rg.sp.inv; return 1; }
    if (!strcmp(key, "wisp_x")) { *out = RSL_PX(rg.sp.x); return 1; }
    if (!strcmp(key, "wisp_y")) { *out = RSL_PX(rg.sp.y); return 1; }
    if (!strcmp(key, "name_pos")) { *out = rg.name_pos; return 1; }
    if (!strcmp(key, "name_slot")) { *out = rg.name_slot; return 1; }
    if (!strcmp(key, "last_hurt")) { *out = rg.last_hurt; return 1; }
    if (!strcmp(key, "layout")) { *out = rsl_layout_audit(); return 1; }
    if (!strcmp(key, "save_runs")) { *out = rgs.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = rgs.wins; return 1; }
    if (!strcmp(key, "save_most_deaths")) { *out = rgs.most_deaths; return 1; }
    if (!strcmp(key, "save_most_revivals")) { *out = rgs.most_revivals; return 1; }
    if (!strcmp(key, "save_best_won")) { *out = (int)rgs.best_won_score; return 1; }
    if (!strncmp(key, "save_top", 8) && isdigit((unsigned char)key[8])) { *out = (int)rgs.top[iclamp(atoi(key + 8), 0, 4)].score; return 1; }
    if (!strncmp(key, "save_name", 9) && isdigit((unsigned char)key[9])) { *out = rgs.top[iclamp(atoi(key + 9), 0, 4)].name[0]; return 1; }
    return 0;
}

static int rsl_cheat(const char *cmd) {
    int a, b, c;
    if (sscanf(cmd, "half %d", &a) == 1) {
        /* straight into half a (0-5), alive, no story */
        if (rg.state == RS_TITLE || rg.state == RS_STORY) new_run();
        rg.deaths = 1;
        start_half(iclamp(a, 0, RSL_HALVES - 1));
        set_state(RS_PLAY);
        game_set_pausable(true);
        return 1;
    }
    if (!strcmp(cmd, "run")) { new_run(); set_state(RS_PLAY); game_set_pausable(true); return 1; }
    if (sscanf(cmd, "deaths %d", &a) == 1) { rg.deaths = a; return 1; }
    if (sscanf(cmd, "score %d", &a) == 1) {
        rg.score = (uint32_t)a;
        rg.bonus_next = (a / RSL_BONUS_EVERY + 1) * RSL_BONUS_EVERY;
        rg.bonus_pending = 0;
        return 1;
    }
    if (sscanf(cmd, "addscore %d", &a) == 1) { rsl_add_score(a); return 1; }
    if (sscanf(cmd, "time %d", &a) == 1) { rg.time = a * 60; return 1; }
    if (sscanf(cmd, "timeframes %d", &a) == 1) { rg.time = a; return 1; }
    if (sscanf(cmd, "weapon %d", &a) == 1) { rg.pl.weapon = iclamp(a, 0, WP_COUNT - 1); return 1; }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) {
        rg.pl.x = a * RSL_FX;
        rg.pl.y = b * RSL_FX;
        rg.pl.vy = 0;
        rg.pl.lift = -1;
        rg.pl.ground = rsl_floor_px(a, b);
        rg.pl.safe_x = rg.pl.x;
        rg.pl.safe_y = rg.pl.y;
        rsl_camera_update(true);
        return 1;
    }
    if (sscanf(cmd, "foe %d %d %d", &a, &b, &c) == 3) {
        int f = rsl_spawn_foe(iclamp(a, 1, FO_KINDS - 1), b * RSL_FX, c * RSL_FX, 0);
        if (f >= 0 && a == FO_LACKEY) rg.foe[f].state = 1;
        if (f >= 0 && a == FO_CRAB) rg.foe[f].state = 1;
        if (f >= 0 && a == FO_TOAD) { rg.foe[f].state = 1; rg.foe[f].t = 24; rg.foe[f].c = rg.foe[f].y; }
        if (f >= 0 && a == FO_WISP) { rg.foe[f].b = c * RSL_FX; rg.foe[f].a = 15; rg.foe[f].dir = -1; }
        return 1;
    }
    {
        int k, x, y, vx, vy;
        if (sscanf(cmd, "eshot %d %d %d %d %d", &k, &x, &y, &vx, &vy) == 5) {
            /* a foe shot at (x, y) pixels moving (vx, vy) in 1/256 px a frame */
            rsl_add_eshot(k, x * RSL_FX, y * RSL_FX, vx, vy);
            return 1;
        }
    }
    if (sscanf(cmd, "foestate %d %d", &a, &b) == 2) { if (a >= 0 && a < RSL_MAX_FOES) rg.foe[a].state = b; return 1; }
    if (sscanf(cmd, "foet %d %d", &a, &b) == 2) { if (a >= 0 && a < RSL_MAX_FOES) rg.foe[a].t = b; return 1; }
    if (sscanf(cmd, "green %d", &a) == 1) { if (a >= 0 && a < RSL_MAX_FOES) rg.foe[a].flags |= FF_GREEN; return 1; }
    if (sscanf(cmd, "hurtfoe %d %d", &a, &b) == 2) { if (a >= 0 && a < RSL_MAX_FOES) rsl_hurt_foe(a, b, 0); return 1; }
    if (sscanf(cmd, "killfoe %d", &a) == 1) { if (a >= 0 && a < RSL_MAX_FOES) rsl_kill_foe(a, true); return 1; }
    if (sscanf(cmd, "foehp %d %d", &a, &b) == 2) { if (a >= 0 && a < RSL_MAX_FOES) rg.foe[a].hp = (int16_t)b; return 1; }
    if (sscanf(cmd, "item %d %d %d", &a, &b, &c) == 3) {
        int i = rsl_drop_item(iclamp(a, 1, IT_KINDS - 1), b * RSL_FX, c * RSL_FX, 0);
        if (i >= 0) { rg.item[i].vx = 0; rg.item[i].vy = 0; rg.item[i].t = 9; }
        return 1;
    }
    if (sscanf(cmd, "wheel %d %d", &a, &b) == 2) {
        /* a weapon torch at (a, b) pixels, lit */
        if (rg.nspawn < RSL_MAX_SPAWNS) {
            RslSpawn *s = &rg.spawn[rg.nspawn];
            memset(s, 0, sizeof *s);
            s->type = SP_TORCH;
            s->a = TORCH_WHEEL;
            s->tx = a / RSL_TILE;
            s->ty = b / RSL_TILE;
            s->child = -1;
            rsl_light_torch(rg.nspawn++);
        }
        return 1;
    }
    if (sscanf(cmd, "torch %d %d %d", &a, &b, &c) == 3) {
        /* a torch at tile (b, c): a = 0 random, 1 wheel, 2 egg */
        if (rg.nspawn < RSL_MAX_SPAWNS) {
            RslSpawn *s = &rg.spawn[rg.nspawn++];
            memset(s, 0, sizeof *s);
            s->type = SP_TORCH;
            s->a = a;
            s->tx = b;
            s->ty = c;
            s->child = -1;
        }
        return 1;
    }
    if (!strcmp(cmd, "god")) { rg.god = true; return 1; }
    if (!strcmp(cmd, "tallyhits")) { rg.tally_hits = true; rg.hits = 0; return 1; }
    if (!strcmp(cmd, "mortal")) { rg.god = false; return 1; }
    if (!strcmp(cmd, "nospawn")) { rg.no_spawn = true; return 1; }
    if (!strcmp(cmd, "spawn")) { rg.no_spawn = false; return 1; }
    if (!strcmp(cmd, "notimer")) { rg.no_timer = true; return 1; }
    if (!strcmp(cmd, "timer")) { rg.no_timer = false; return 1; }
    if (!strcmp(cmd, "clear")) {
        for (int i = 0; i < RSL_MAX_FOES; i++)
            if (rg.foe[i].alive && !RSL_IS_BOSS(rg.foe[i].kind)) {
                if (rg.foe[i].spawn >= 0) rg.spawn[rg.foe[i].spawn].child = -1;
                rg.foe[i].alive = 0;
            }
        memset(rg.es, 0, sizeof rg.es);
        return 1;
    }
    if (!strcmp(cmd, "lightall")) {
        /* every torch in view, lit */
        for (int i = 0; i < rg.nspawn; i++)
            if (rg.spawn[i].type == SP_TORCH && !rg.spawn[i].used &&
                rsl_in_view(rg.spawn[i].tx * RSL_TILE + 8, rg.spawn[i].ty * RSL_TILE + 8, 0))
                rsl_light_torch(i);
        return 1;
    }
    if (!strcmp(cmd, "clearitems")) { memset(rg.item, 0, sizeof rg.item); return 1; }
    if (!strcmp(cmd, "die")) { rg.pl.inv = 0; rg.god = false; rg.owl.on = false; rsl_kill_player(0); return 1; }
    if (!strcmp(cmd, "hurt")) { rg.pl.inv = 0; rsl_player_hurt(0); return 1; }
    if (!strcmp(cmd, "owl")) { rg.owl.on = true; rg.owl.x = rg.pl.x; rg.owl.y = rg.pl.y - 30 * RSL_FX; return 1; }
    if (!strcmp(cmd, "boss")) {
        /* to the boss screen's door */
        if (rg.boss_spawn >= 0) {
            const RslSpawn *s = &rg.spawn[rg.boss_spawn];
            int scx = s->tx / RSL_SW;
            int x = scx * RSL_SW * RSL_TILE + (rg.boss_kind == BOSS_PAIR ? 160 : 44);
            int g = rsl_ground_below(x, s->ty * RSL_TILE);
            rg.pl.x = x * RSL_FX;
            rg.pl.y = g * RSL_FX;
            rg.pl.ground = true;
            rg.pl.lift = -1;
            rsl_camera_update(true);
            boss_trigger();
        }
        return 1;
    }
    if (!strcmp(cmd, "killboss")) {
        for (int i = 0; i < RSL_MAX_FOES; i++)
            if (rg.foe[i].alive && RSL_IS_BOSS(rg.foe[i].kind) && rg.foe[i].kind != FO_SKULL) rsl_kill_foe(i, true);
        return 1;
    }
    if (sscanf(cmd, "bosshp %d", &a) == 1) {
        for (int i = 0; i < RSL_MAX_FOES; i++)
            if (rg.foe[i].alive && RSL_IS_BOSS(rg.foe[i].kind) && rg.foe[i].kind != FO_SKULL) rg.foe[i].hp = (int16_t)a;
        return 1;
    }
    if (!strcmp(cmd, "spirit")) { begin_spirit(); return 1; }
    if (!strcmp(cmd, "winspirit")) {
        for (int i = 0; i < rg.sp.n; i++) rg.sp.piece[i].taken = true;
        rg.sp.got = rg.sp.n;
        return 1;
    }
    if (sscanf(cmd, "wisp %d %d", &a, &b) == 2) { rg.sp.x = a * RSL_FX; rg.sp.y = b * RSL_FX; return 1; }
    if (sscanf(cmd, "piece %d %d %d", &a, &b, &c) == 3) {
        if (a >= 0 && a < rg.sp.n) { rg.sp.piece[a].x = b * RSL_FX; rg.sp.piece[a].y = c * RSL_FX; }
        return 1;
    }
    if (sscanf(cmd, "guard %d %d %d", &a, &b, &c) == 3) {
        if (a >= 0 && a < rg.sp.nguards) {
            RslGuard *g = &rg.sp.guard[a];
            g->x = b * RSL_FX;
            g->y = c * RSL_FX;
            g->lunge = -1000; /* parked */
            g->speed = 0;
            g->vx = g->vy = 0;
            if (g->piece >= 0) { rg.sp.piece[g->piece].x = g->x - 20 * RSL_FX; rg.sp.piece[g->piece].y = g->y; g->ang = 0; g->r = 20; }
        }
        return 1;
    }
    if (sscanf(cmd, "botplan %d", &a) == 1) { rsl_bot_plan = a; return 1; }
    if (sscanf(cmd, "menu %d", &a) == 1) { rg.menu = a ? 1 : 0; return 1; }
    if (!strcmp(cmd, "tally")) {
        if (rg.boss_spawn >= 0) {
            rg.boss_on = true;
            rg.boss_dead = true;
            rg.boss_done_t = 240;
        }
        return 1;
    }
    if (!strcmp(cmd, "secret")) {
        /* the nearest hidden spot, found */
        int best = -1, bd = 1 << 30;
        for (int i = 0; i < rg.nspawn; i++)
            if (rg.spawn[i].type == SP_SECRET && !rg.spawn[i].used && !rg.spawn[i].a) {
                int d = iabs(rg.spawn[i].tx * RSL_TILE - RSL_PX(rg.pl.x));
                if (d < bd) { bd = d; best = i; }
            }
        if (best >= 0) rsl_find_secret(best);
        return 1;
    }
    if (sscanf(cmd, "goto_secret %d", &a) == 1) {
        int n = 0;
        for (int i = 0; i < rg.nspawn; i++)
            if (rg.spawn[i].type == SP_SECRET && !rg.spawn[i].a) {
                if (n++ == a) {
                    rg.pl.x = (rg.spawn[i].tx * RSL_TILE + 8) * RSL_FX;
                    rg.pl.y = (rg.spawn[i].ty * RSL_TILE + 16) * RSL_FX;
                    rsl_camera_update(true);
                }
            }
        return 1;
    }
    if (sscanf(cmd, "pit %d", &a) == 1) { rsl_enter_pit(a); return 1; }
    if (!strcmp(cmd, "leavepit")) { rsl_leave_pit(); return 1; }
    if (!strcmp(cmd, "win")) {
        rg.half = RSL_HALVES - 1;
        rg.boss_on = rg.boss_dead = true;
        rg.boss_done_t = 240;
        set_state(RS_PLAY);
        return 1;
    }
    if (!strcmp(cmd, "gameover")) {
        set_state(RS_OVER);
        music_play(RSL_MUS_OVER);
        return 1;
    }
    return 0;
}

const GameDef GAME_RESTLESS = {
    "restless",
    "RESTLESS",
    "1988",
    "ACTION PLATFORMER",
    "OLD GAUNT WON'T STAY DEAD. NO LIVES: EVERY DEATH, WIN BACK YOUR SOUL.",
    {"COME BACK FROM THE DEAD THREE TIMES IN ONE RUN", "LAY THE HOLLOW KING TO REST",
     "LAY HIM TO REST WITH 50,000 POINTS OR MORE"},
    GLYPH_LEFT GLYPH_RIGHT "\tWALK (ONE PACE)\n"
    GLYPH_DOWN "\tCROUCH\n"
    "TAP " GLYPH_A "\tSHOOT THE STAFF\n"
    "HOLD " GLYPH_A "\tCHARGE, LET GO TO LOOSE\n"
    GLYPH_UP "+" GLYPH_A "\tSHOOT UP AT A SLANT\n"
    GLYPH_DOWN "+" GLYPH_A "\tDOWN AT A SLANT (IN THE AIR)\n"
    GLYPH_B "\tJUMP (NO STEERING)\n"
    GLYPH_DOWN "+" GLYPH_B "\tDROP THROUGH A LEDGE\n"
    "LOW GLOW\t" GLYPH_DPAD " FLY THE WISP\n"
    "START\tPAUSE",
    C_MAROON, C_ORANGE,
    rsl_load, rsl_start, rsl_update, rsl_draw, rsl_quit, rsl_draw_label, rsl_query, rsl_cheat,
    "RAKSHASA", 38,
    NULL,
};
