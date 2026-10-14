/* ANTIPODE - a gravity-flipping suit on a station turned against it.
 * Cartridge 29 of UFO 40, a tribute to Vainger (UFO 50 #29).
 * See docs/games/29-antipode.md. This file: the title, the code, the
 * story, the run (cradles, saving, losing a body, game over), the cradle's
 * MODS and MAP pages, terminals, teleporters, the ending and the credits,
 * the records, and the test hooks. */
#include "apd.h"

ApdGame ag;
ApdSave ags;

#define SAVE_MAGIC 0x41504401u

const char *const APD_MOD_NAME[APD_MODS] = {"EMBER", "TWIN", "VOLT", "MASS"};
const char *const APD_SYS_NAME[APD_SYS] = {"OFF", "FLIP", "FRAME", "GUN"};
const char *const APD_EFFECT_NAME[APD_MODS][APD_SYS] = {
    {"", "FLARE", "KILNSKIN", "SCORCH"},
    {"", "AFTERIMAGE", "SATELLITE", "FANSHOT"},
    {"", "STORMBALL", "SIPHON", "PHASER"},
    {"", "ANVIL", "IRONSOLE", "REACH"},
};

bool apd_simulating;
uint32_t apd_sim_cur, apd_sim_prev;

int apd_max_armor(void);
int apd_tele_dest(int link, int from_room);
void apd_boss_tick(void);

void apd_sfx(const char *name) {
    if (!apd_simulating) sfx_play_name(name);
}
void apd_music(int id) {
    if (!apd_simulating) music_play(id);
}

void apd_say(const char *m) {
    snprintf(ag.msg, sizeof ag.msg, "%s", m);
    ag.msg_t = 120;
}

static void set_state(int s) {
    ag.state = s;
    ag.state_t = 0;
}

/* ---- mods -------------------------------------------------------------------- */

int apd_safe_slots(void) { return 1 + ag.run.dampers; }
int apd_sys_count(int sys) {
    int n = 0;
    for (int m = 0; m < APD_MODS; m++) n += ag.run.mod_sys[m] == sys;
    return n;
}
bool apd_has(int mod, int sys) { return ag.run.mod_sys[mod] == sys; }
bool apd_overload(int sys) { return sys != SYS_OFF && apd_sys_count(sys) > apd_safe_slots(); }

/* ---- completion ---------------------------------------------------------------- */

static int crown_arena(void) {
    static int r = -2;
    if (r == -2) r = apd_room_find(SEC_CROWN, 'F');
    return r;
}

/* rooms seen, as a whole per cent; the Crown's own hall counts once Halloran
 * is beaten (so a run with everything stands at 98 % before the last fight) */
int apd_explore_pct(void) {
    int n = 0;
    for (int i = 0; i < apd_nrooms; i++)
        if (ag.run.visited[i] && (i != crown_arena() || ag.run.boss_dead[BS_HALLORAN])) n++;
    return n * 100 / imax(1, apd_nrooms);
}
int apd_items_got(void) {
    int n = 0;
    for (int i = 0; i < apd_nitems; i++) n += ag.run.item_got[i];
    return n;
}
int apd_items_pct(void) { return apd_items_got() * 100 / imax(1, apd_nitems); }

/* ---- saving -------------------------------------------------------------------- */

void apd_save_now(void) {
    if (apd_simulating) return;
    ags.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &ags, (int)sizeof ags);
}

static void load_save(void) {
    ApdSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { ags = tmp; return; }
    memset(&ags, 0, sizeof ags);
    ags.magic = SAVE_MAGIC;
}

static void records(void) {
    if (ag.run.code_on || apd_simulating) return;
    int up = apd_items_got(), ex = apd_explore_pct();
    if (up > ags.best_upgrades) ags.best_upgrades = (uint16_t)up;
    if (ex > ags.best_explore) ags.best_explore = (uint16_t)ex;
}

/* walking up to a cradle: saved, armor and spare bodies full */
void apd_pod_save(int pod) {
    ag.pl.armor = apd_max_armor();
    ag.spares = ag.run.spares_max;
    if (apd_simulating) return;
    ag.run.pod = (int16_t)pod;
    ag.run.room = (int16_t)ag.room;
    records();
    if (ag.run.code_on) { apd_say("CRADLE: NO SAVING WITH THE CODE"); return; }
    ags.has_run = 1;
    ags.run = ag.run;
    apd_save_now();
    apd_say("CRADLE: SAVED");
    apd_sfx("apd_save");
}

/* ---- music ------------------------------------------------------------------------- */

void apd_room_music(void);
void apd_room_music(void) {
    if (ag.boss_on) { apd_music(ag.boss == BS_HALLORAN ? APD_MUS_FINAL : APD_MUS_BOSS); return; }
    static const int *const M[APD_SECTORS] = {&APD_MUS_GANTRY, &APD_MUS_BRINE, &APD_MUS_COIL, &APD_MUS_CROWN};
    apd_music(*M[apd_rooms[ag.room].sector]);
}

/* ---- the run --------------------------------------------------------------------- */

static void place_at_pod(int pod) {
    for (int i = 0; i < ag.nth; i++)
        if (ag.th[i].kind == TH_POD && ag.th[i].id == pod) {
            ag.pl.x = (ag.th[i].tx * APD_TILE + 8 + 22) * APD_FX;
            ag.pl.y = ((ag.th[i].ty + 1) * APD_TILE - APD_PH / 2) * APD_FX;
            return;
        }
}

static void begin_room(int room, int pod) {
    ApdPlayer *p = &ag.pl;
    int keep_entry_x = p->entry_x, keep_entry_y = p->entry_y;
    apd_build_room(room);
    if (pod >= 0) place_at_pod(pod);
    else if (apd_rooms[room].spec >= 0 && (APD_SPECS[apd_rooms[room].spec].flags & RF_START)) {
        p->x = p->entry_x;
        p->y = p->entry_y;
    } else {
        p->x = keep_entry_x;
        p->y = keep_entry_y;
    }
    p->grav = 1;
    p->face = 1;
    p->vx = p->vy = 0;
    p->ground = true;
    p->entry_x = p->x;
    p->entry_y = p->y;
    p->entry_grav = 1;
    p->entry_face = 1;
    p->armor = apd_max_armor();
    p->inv = 0;
    ag.spares = ag.run.spares_max;
    ag.run.visited[room] = 1;
    ag.pod_zone = pod;
    apd_camera(true);
    apd_room_music();
}

static void new_run(bool code) {
    memset(&ag.run, 0, sizeof ag.run);
    memset(&ag.pl, 0, sizeof ag.pl);
    for (int m = 0; m < APD_MODS; m++) ag.run.mod_sys[m] = MOD_NONE;
    ag.run.spares_max = APD_SPARES0;
    ag.run.pod = -1;
    ag.run.room = (int16_t)apd_room_find(SEC_GANTRY, 'S');
    ag.run.code_on = code;
    if (code) {
        /* the code: every upgrade from the start, and no saving */
        for (int i = 0; i < apd_nitems; i++) {
            const ApdItemDef *it = &apd_items[i];
            ag.run.item_got[i] = 1;
            if (it->kind == IT_ARMOR) ag.run.cells++;
            else if (it->kind == IT_SPARE) ag.run.spares_max++;
            else if (it->kind == IT_DAMPER) ag.run.dampers++;
        }
        ag.run.mod_sys[MOD_EMBER] = SYS_GUN;
        ag.run.mod_sys[MOD_MASS] = SYS_FLIP;
        ag.run.mod_sys[MOD_VOLT] = SYS_FRAME;
        ag.run.mod_sys[MOD_TWIN] = SYS_OFF;
    }
    ag.boss_dead_t = 0;
    begin_room(ag.run.room, -1);
    if (!code && !apd_simulating) {
        /* the start counts as the first save */
        ags.has_run = 1;
        ags.run = ag.run;
        apd_save_now();
    }
    set_state(AS_PLAY);
}

static void continue_run(void) {
    ag.run = ags.run;
    ag.boss_dead_t = 0;
    memset(&ag.pl, 0, sizeof ag.pl);
    int room = ag.run.room >= 0 ? ag.run.room : apd_room_find(SEC_GANTRY, 'S');
    begin_room(room, ag.run.pod);
    set_state(AS_PLAY);
}

void apd_lose_body(void) {
    if (ag.state != AS_PLAY) return;
    ag.run.deaths++;
    if (!apd_simulating && !ag.run.code_on) ags.times_died++;
    apd_burst(APD_PX(ag.pl.x), APD_PX(ag.pl.y), C_WHITE, 16, 3);
    apd_burst(APD_PX(ag.pl.x), APD_PX(ag.pl.y), C_SKY, 12, 2);
    apd_sfx("apd_die");
    set_state(AS_DYING);
}

static void dying_update(void) {
    for (int i = 0; i < APD_MAX_PARTS; i++)
        if (ag.part[i].life > 0) { ag.part[i].life--; ag.part[i].x += ag.part[i].vx; ag.part[i].y += ag.part[i].vy; }
    if (ag.state_t < 60) return;
    if (ag.spares > 0) {
        ag.spares--;
        apd_respawn_here();
        apd_boss_regain();
        set_state(AS_PLAY);
        apd_sfx("apd_respawn");
        apd_say("A SPARE BODY TAKES OVER");
    } else {
        set_state(AS_OVER);
        if (!apd_simulating) { music_play(APD_MUS_OVER); apd_save_now(); }
    }
}

void apd_final_defeated(void);
void apd_final_defeated(void) {
    ag.run.won = 1;
    ag.run.boss_dead[BS_HALLORAN] = 1;
    ag.run.visited[crown_arena()] = 1;
    ag.cherry_run = apd_explore_pct() >= 100 && apd_items_got() >= apd_nitems;
    if (!ag.run.code_on && !apd_simulating) {
        game_award(GOAL_SAUCER);
        if (ags.wins < 65535) ags.wins++;
        records();
        apd_save_now();
    }
    set_state(AS_ENDING);
    if (!apd_simulating) music_play(APD_MUS_ENDING);
}

/* ---- the cradle ------------------------------------------------------------------ */

void apd_enter_pod(void) {
    set_state(AS_POD);
    ag.pod_page = 0;
    ag.pod_mod = 0;
    ag.pod_sector = apd_rooms[ag.room].sector;
    apd_sfx("apd_pod");
    apd_music(APD_MUS_POD);
}

static void pod_update(void) {
    if (ag.state_t < 4) return;
    if (btnp(BTN_B)) {
        input_consume();
        set_state(AS_PLAY);
        apd_room_music();
        apd_sfx("ui_back");
        return;
    }
    if (ag.pod_page == 0) {
        if (btnp(BTN_LEFT) && ag.pod_mod > 0) { ag.pod_mod--; sfx_play_name("ui_move"); }
        else if (btnp(BTN_RIGHT)) {
            if (ag.pod_mod < APD_MODS - 1) ag.pod_mod++;
            else { ag.pod_page = 1; ag.pod_sector = apd_rooms[ag.room].sector; }
            sfx_play_name("ui_move");
        }
        int m = ag.pod_mod;
        if (ag.run.mod_sys[m] != MOD_NONE) {
            /* rows from the top: FLIP, FRAME, GUN, OFF */
            static const int ROW_OF[APD_SYS] = {3, 0, 1, 2};
            static const int SYS_AT[4] = {SYS_FLIP, SYS_FRAME, SYS_GUN, SYS_OFF};
            int row = ROW_OF[ag.run.mod_sys[m]];
            if (btnp(BTN_UP) && row > 0) { ag.run.mod_sys[m] = (uint8_t)SYS_AT[row - 1]; sfx_play_name("apd_slot"); }
            else if (btnp(BTN_DOWN) && row < 3) { ag.run.mod_sys[m] = (uint8_t)SYS_AT[row + 1]; sfx_play_name("apd_slot"); }
        }
    } else {
        if (btnp(BTN_LEFT)) { ag.pod_page = 0; ag.pod_mod = APD_MODS - 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_UP)) { ag.pod_sector = (ag.pod_sector + APD_SECTORS - 1) % APD_SECTORS; sfx_play_name("ui_move"); }
        if (btnp(BTN_DOWN)) { ag.pod_sector = (ag.pod_sector + 1) % APD_SECTORS; sfx_play_name("ui_move"); }
    }
}

/* ---- terminals and teleporters ------------------------------------------------- */

void apd_read_terminal(int id) {
    ag.text_id = id;
    ag.text_page = 0;
    set_state(AS_TEXT);
    apd_sfx("apd_term");
}

static void text_update(void) {
    if (ag.state_t > 10 && (btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_UP))) {
        input_consume();
        set_state(AS_PLAY);
    }
}

void apd_start_teleport(int link) {
    ag.tele_to = link;
    set_state(AS_TELE);
    apd_sfx("apd_tele");
}

static void tele_update(void) {
    if (ag.state_t == 30) {
        int dest = apd_tele_dest(ag.tele_to, ag.room);
        if (dest >= 0) {
            apd_build_room(dest);
            for (int i = 0; i < ag.nth; i++)
                if (ag.th[i].kind == TH_TELE && ag.th[i].id == ag.tele_to) {
                    ag.pl.x = (ag.th[i].tx * APD_TILE + 8) * APD_FX;
                    ag.pl.y = ((ag.th[i].ty + 1) * APD_TILE - APD_PH / 2) * APD_FX;
                }
            ag.pl.grav = 1;
            ag.pl.vx = ag.pl.vy = 0;
            ag.pl.ground = true;
            ag.pl.entry_x = ag.pl.x;
            ag.pl.entry_y = ag.pl.y;
            ag.pl.entry_grav = 1;
            ag.pl.entry_face = ag.pl.face;
            ag.run.visited[dest] = 1;
            ag.pod_zone = -1;
            apd_camera(true);
            apd_room_music();
        }
    }
    if (ag.state_t >= 60) set_state(AS_PLAY);
}

/* ---- one frame of play ---------------------------------------------------------- */

void apd_play_step(void) {
    if (!apd_simulating) game_set_pausable(true);
    ag.run.frames++;
    if (ag.msg_t > 0) ag.msg_t--;
    apd_player_update();
    if (ag.state != AS_PLAY) return;
    apd_shots_update();
    apd_foes_update();
    if (ag.state != AS_PLAY) return;
    apd_eshots_update();
    if (ag.state != AS_PLAY) return;
    apd_hazards_update();
    if (ag.state != AS_PLAY) return;
    apd_things_update();
    if (ag.state != AS_PLAY) return;
    apd_boss_tick();
    if (ag.state != AS_PLAY) return;
    apd_room_change_check();
    for (int i = 0; i < APD_MAX_PARTS; i++)
        if (ag.part[i].life > 0) { ag.part[i].life--; ag.part[i].x += ag.part[i].vx; ag.part[i].y += ag.part[i].vy; }
    if (ag.shake > 0) ag.shake--;
    apd_camera(false);
    if (!apd_simulating && music_playing() == APD_MUS_ITEM && music_finished()) apd_room_music();
}

/* ---- the title, the code, the story --------------------------------------------- */

static void to_title(void) {
    set_state(AS_TITLE);
    game_set_pausable(false);
    music_play(APD_MUS_TITLE);
    ag.menu = ags.has_run ? 1 : 0;
}

static void title_update(void) {
    game_set_pausable(false);
    if (btnp(BTN_UP)) { ag.menu = (ag.menu + 2) % 3; sfx_play_name("ui_move"); }
    if (btnp(BTN_DOWN)) { ag.menu = (ag.menu + 1) % 3; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if ((btnp(BTN_A) || btnp(BTN_START)) && ag.state_t > 5) {
        input_consume();
        if (ag.menu == 1 && !ags.has_run) { sfx_play_name("ui_error"); return; }
        sfx_play_name("ui_ok");
        if (ag.menu == 0) {
            set_state(AS_INTRO);
            music_play(APD_MUS_INTRO);
        } else if (ag.menu == 1) {
            continue_run();
        } else {
            set_state(AS_CODE);
            memcpy(ag.code, "AAAA-AAAA", 9);
            ag.code_pos = 0;
        }
    }
}

static void code_update(void) {
    game_set_pausable(false);
    char *c = &ag.code[ag.code_pos];
    if (btn_repeat(BTN_UP)) { *c = *c >= 'Z' ? 'A' : (char)(*c + 1); sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { *c = *c <= 'A' ? 'Z' : (char)(*c - 1); sfx_play_name("ui_move"); }
    if (btnp(BTN_RIGHT) && ag.code_pos < 8) { ag.code_pos += ag.code_pos == 3 ? 2 : 1; sfx_play_name("ui_move"); }
    if (btnp(BTN_LEFT) && ag.code_pos > 0) { ag.code_pos -= ag.code_pos == 5 ? 2 : 1; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { input_consume(); to_title(); ag.menu = 2; return; }
    if (btnp(BTN_A)) {
        input_consume();
        char want[10];
        snprintf(want, sizeof want, "%s", APD_CODE);
        ag.code_armed = !strncmp(ag.code, want, 9);
        sfx_play_name(ag.code_armed ? "apd_unlock" : "ui_error");
        if (ag.code_armed) {
            set_state(AS_INTRO);
            music_play(APD_MUS_INTRO);
        }
    }
}

static void intro_update(void) {
    game_set_pausable(false);
    if ((ag.state_t > 20 && (btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_START))) || ag.state_t > 1500) {
        input_consume();
        new_run(ag.code_armed);
        ag.code_armed = false;
    }
}

static void ending_update(void) {
    game_set_pausable(false);
    if (btn(BTN_A)) ag.state_t += 2;
    if (ag.state_t > 1300) {
        set_state(AS_CREDITS);
        if (!apd_simulating) music_play(APD_MUS_CREDITS);
    }
}

#define CREDITS_LEN 2100
static void credits_update(void) {
    game_set_pausable(false);
    if (btn(BTN_A)) ag.state_t += 2;
    if (ag.state_t > CREDITS_LEN) {
        /* the cherry only once the credits have run to their end */
        if (ag.cherry_run && !ag.run.code_on && !apd_simulating) {
            game_award(GOAL_ALIEN);
            if (ags.cherries < 65535) ags.cherries++;
        }
        apd_save_now();
        input_consume();
        to_title();
    }
}

static void over_update(void) {
    game_set_pausable(false);
    if (ag.state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) {
        input_consume();
        if (ag.run.code_on || !ags.has_run) {
            ag.code_armed = ag.run.code_on;
            new_run(ag.code_armed);
        } else continue_run();
    } else if (ag.state_t > 60 && btnp(BTN_B)) {
        input_consume();
        to_title();
    }
}

static void apd_update(void) {
    ag.state_t++;
    ag.frame_t++;
    switch (ag.state) {
    case AS_TITLE: title_update(); break;
    case AS_CODE: code_update(); break;
    case AS_INTRO: intro_update(); break;
    case AS_PLAY: apd_play_step(); break;
    case AS_POD: game_set_pausable(true); pod_update(); break;
    case AS_TEXT: game_set_pausable(true); text_update(); break;
    case AS_TELE: game_set_pausable(true); tele_update(); break;
    case AS_DYING: game_set_pausable(true); dying_update(); break;
    case AS_OVER: over_update(); break;
    case AS_ENDING: ending_update(); break;
    case AS_CREDITS: credits_update(); break;
    default: break;
    }
}

/* ---- the cartridge ---------------------------------------------------------------- */

static void apd_load(void) {
    apd_world_init();
    apd_art_load();
    apd_audio_load();
}

static void apd_start(void) {
    load_save();
    memset(&ag, 0, sizeof ag);
    ag.boss_foe = -1;
    ag.boss = -1;
    if (ags.gift) game_award(GOAL_BEACON);
    if (ags.wins > 0) game_award(GOAL_SAUCER);
    if (ags.cherries > 0) game_award(GOAL_ALIEN);
    to_title();
}

static void apd_quit(void) { apd_save_now(); }

/* ---- test hooks ------------------------------------------------------------------- */

int apd_reach_check(void);
int apd_bot_idle(void);

static int count_things(int kind) {
    int n = 0;
    for (int i = 0; i < ag.nth; i++) n += ag.th[i].alive && ag.th[i].kind == kind;
    return n;
}

static int apd_query(const char *key, int *out) {
    const ApdPlayer *p = &ag.pl;
    if (!strcmp(key, "bot")) { *out = apd_bot_buttons(); return 1; }
    if (!strcmp(key, "bot_idle")) { *out = apd_bot_idle(); return 1; }
    if (!strcmp(key, "bot_goal")) { *out = apd_bot_goal; return 1; }
    if (!strcmp(key, "state")) { *out = ag.state; return 1; }
    if (!strcmp(key, "menu")) { *out = ag.menu; return 1; }
    if (!strcmp(key, "room")) { *out = ag.room; return 1; }
    if (!strcmp(key, "room_id")) { *out = apd_rooms[ag.room].id; return 1; }
    if (!strcmp(key, "sector")) { *out = apd_rooms[ag.room].sector; return 1; }
    if (!strcmp(key, "px")) { *out = APD_PX(p->x); return 1; }
    if (!strcmp(key, "py")) { *out = APD_PX(p->y); return 1; }
    if (!strcmp(key, "vy")) { *out = p->vy; return 1; }
    if (!strcmp(key, "grav")) { *out = p->grav; return 1; }
    if (!strcmp(key, "ground")) { *out = p->ground; return 1; }
    if (!strcmp(key, "crouch")) { *out = p->crouch; return 1; }
    if (!strcmp(key, "face")) { *out = p->face; return 1; }
    if (!strcmp(key, "aim")) { *out = p->aim; return 1; }
    if (!strcmp(key, "inv")) { *out = p->inv; return 1; }
    if (!strcmp(key, "stomp")) { *out = p->stomp; return 1; }
    if (!strcmp(key, "thunder")) { *out = p->thunder_t; return 1; }
    if (!strcmp(key, "flips")) { *out = p->flips; return 1; }
    if (!strcmp(key, "fired")) { *out = p->shots_fired; return 1; }
    if (!strcmp(key, "armor")) { *out = p->armor; return 1; }
    if (!strcmp(key, "armor_max")) { *out = apd_max_armor(); return 1; }
    if (!strcmp(key, "spares")) { *out = ag.spares; return 1; }
    if (!strcmp(key, "spares_max")) { *out = ag.run.spares_max; return 1; }
    if (!strcmp(key, "cells")) { *out = ag.run.cells; return 1; }
    if (!strcmp(key, "dampers")) { *out = ag.run.dampers; return 1; }
    if (!strcmp(key, "safe")) { *out = apd_safe_slots(); return 1; }
    if (!strcmp(key, "pass")) { *out = ag.run.pass; return 1; }
    if (!strcmp(key, "ciphers")) { *out = ag.run.ciphers; return 1; }
    if (!strncmp(key, "mod", 3) && isdigit((unsigned char)key[3])) { *out = ag.run.mod_sys[(key[3] - '0') & 3]; return 1; }
    if (!strncmp(key, "overload", 8)) { *out = apd_overload(atoi(key + 8)); return 1; }
    if (!strcmp(key, "explore")) { *out = apd_explore_pct(); return 1; }
    if (!strcmp(key, "items")) { *out = apd_items_pct(); return 1; }
    if (!strcmp(key, "items_got")) { *out = apd_items_got(); return 1; }
    if (!strcmp(key, "deaths")) { *out = (int)ag.run.deaths; return 1; }
    if (!strcmp(key, "won")) { *out = ag.run.won; return 1; }
    if (!strcmp(key, "code_on")) { *out = ag.run.code_on; return 1; }
    if (!strcmp(key, "secret")) { *out = ag.run.secret_open; return 1; }
    if (!strcmp(key, "boss")) { *out = ag.boss; return 1; }
    if (!strcmp(key, "boss_on")) { *out = ag.boss_on; return 1; }
    if (!strcmp(key, "boss_lock")) { *out = ag.boss_lock; return 1; }
    if (!strcmp(key, "boss_hp")) { *out = apd_boss_hp(); return 1; }
    if (!strcmp(key, "boss_max")) { *out = apd_boss_maxhp(); return 1; }
    if (!strcmp(key, "boss_phase")) { *out = ag.boss_phase; return 1; }
    if (!strcmp(key, "boss_flags")) { *out = ag.boss_foe >= 0 ? ag.foe[ag.boss_foe].flags : -1; return 1; }
    if (!strcmp(key, "boss_up")) { *out = ag.boss_foe >= 0 ? ag.foe[ag.boss_foe].up : -1; return 1; }
    if (!strcmp(key, "boss_state")) { *out = ag.boss_foe >= 0 ? ag.foe[ag.boss_foe].state : -1; return 1; }
    if (!strncmp(key, "boss_dead", 9)) { *out = ag.run.boss_dead[iclamp(atoi(key + 9), 0, APD_BOSSES - 1)]; return 1; }
    if (!strcmp(key, "lightning")) { *out = ag.lightning_t; return 1; }
    if (!strcmp(key, "foes")) { *out = apd_foes_alive(); return 1; }
    if (!strncmp(key, "foes_", 5)) { *out = apd_count_foes(atoi(key + 5)); return 1; }
    if (!strncmp(key, "foe", 3) && isdigit((unsigned char)key[3])) {
        int i = atoi(key + 3);
        const char *u = strchr(key + 3, '_');
        if (i < 0 || i >= APD_MAX_FOES || !u) return 0;
        const ApdFoe *f = &ag.foe[i];
        if (!strcmp(u, "_alive")) { *out = f->alive; return 1; }
        if (!strcmp(u, "_kind")) { *out = f->kind; return 1; }
        if (!strcmp(u, "_hp")) { *out = f->alive ? f->hp : 0; return 1; }
        if (!strcmp(u, "_x")) { *out = APD_PX(f->x); return 1; }
        if (!strcmp(u, "_y")) { *out = APD_PX(f->y); return 1; }
        if (!strcmp(u, "_up")) { *out = f->up; return 1; }
        return 0;
    }
    if (!strcmp(key, "shots")) { int n = 0; for (int i = 0; i < APD_MAX_SHOTS; i++) n += ag.shot[i].alive; *out = n; return 1; }
    if (!strncmp(key, "shots_", 6)) { int k = atoi(key + 6), n = 0; for (int i = 0; i < APD_MAX_SHOTS; i++) n += ag.shot[i].alive && ag.shot[i].kind == k; *out = n; return 1; }
    if (!strcmp(key, "shot0_dmg")) { *out = -1; for (int i = 0; i < APD_MAX_SHOTS; i++) if (ag.shot[i].alive) { *out = ag.shot[i].dmg; break; } return 1; }
    if (!strcmp(key, "shot0_life")) { *out = -1; for (int i = 0; i < APD_MAX_SHOTS; i++) if (ag.shot[i].alive) { *out = ag.shot[i].life; break; } return 1; }
    if (!strcmp(key, "shot0_vx")) { *out = -9999; for (int i = 0; i < APD_MAX_SHOTS; i++) if (ag.shot[i].alive) { *out = ag.shot[i].vx; break; } return 1; }
    if (!strcmp(key, "shot0_vy")) { *out = -9999; for (int i = 0; i < APD_MAX_SHOTS; i++) if (ag.shot[i].alive) { *out = ag.shot[i].vy; break; } return 1; }
    if (!strncmp(key, "eshots_", 7)) { int k = atoi(key + 7), n = 0; for (int i = 0; i < APD_MAX_ESHOTS; i++) n += ag.es[i].alive && ag.es[i].kind == k; *out = n; return 1; }
    if (!strncmp(key, "tileat_", 7)) {
        int x = atoi(key + 7);
        const char *u = strchr(key + 7, '_');
        *out = u ? apd_tile_at(x, atoi(u + 1)) : -1;
        return 1;
    }
    if (!strncmp(key, "gate_", 5)) { int d = atoi(key + 5); *out = d >= 0 && d < apd_ndoors ? ag.run.gate_open[d] : -1; return 1; }
    if (!strncmp(key, "door_", 5) && strlen(key) == 8) {
        /* door_SAB: the door between rooms A and B of sector S */
        int s = key[5] - '0', a = apd_room_find(s, key[6]), b = apd_room_find(s, key[7]);
        *out = -1;
        for (int d = 0; d < apd_ndoors; d++)
            if ((apd_doors[d].ra == a && apd_doors[d].rb == b) || (apd_doors[d].ra == b && apd_doors[d].rb == a)) *out = d;
        return 1;
    }
    if (!strcmp(key, "air_flips")) { *out = p->air_flips; return 1; }
    if (!strcmp(key, "eshots")) { int n = 0; for (int i = 0; i < APD_MAX_ESHOTS; i++) n += ag.es[i].alive; *out = n; return 1; }
    if (!strcmp(key, "shadow")) { *out = ag.shadow.on; return 1; }
    if (!strcmp(key, "shadow_x")) { *out = APD_PX(ag.shadow.x); return 1; }
    if (!strcmp(key, "shadow_y")) { *out = APD_PX(ag.shadow.y); return 1; }
    if (!strcmp(key, "shadow_aim")) { *out = ag.shadow.aim; return 1; }
    if (!strcmp(key, "button0")) { *out = ag.button_t[0]; return 1; }
    if (!strcmp(key, "button1")) { *out = ag.button_t[1]; return 1; }
    if (!strcmp(key, "drone")) { *out = ag.drone.on; return 1; }
    if (!strcmp(key, "drone_latch")) { *out = ag.drone.latch; return 1; }
    if (!strcmp(key, "pod_zone")) { *out = ag.pod_zone; return 1; }
    if (!strcmp(key, "pod_page")) { *out = ag.pod_page; return 1; }
    if (!strcmp(key, "pod_mod")) { *out = ag.pod_mod; return 1; }
    if (!strcmp(key, "pod_sector")) { *out = ag.pod_sector; return 1; }
    if (!strcmp(key, "text_id")) { *out = ag.text_id; return 1; }
    if (!strcmp(key, "pods_here")) { *out = count_things(TH_POD); return 1; }
    if (!strcmp(key, "items_here")) { *out = count_things(TH_ITEM); return 1; }
    if (!strcmp(key, "teles_here")) { *out = count_things(TH_TELE); return 1; }
    if (!strcmp(key, "terms_here")) { *out = count_things(TH_TERM); return 1; }
    if (!strcmp(key, "balls_here")) { *out = count_things(TH_BALL); return 1; }
    if (!strcmp(key, "save_has")) { *out = ags.has_run; return 1; }
    if (!strcmp(key, "save_room")) { *out = ags.run.room; return 1; }
    if (!strcmp(key, "save_pod")) { *out = ags.run.pod; return 1; }
    if (!strcmp(key, "save_items")) { int n = 0; for (int i = 0; i < apd_nitems; i++) n += ags.run.item_got[i]; *out = n; return 1; }
    if (!strcmp(key, "rec_upgrades")) { *out = ags.best_upgrades; return 1; }
    if (!strcmp(key, "rec_explore")) { *out = ags.best_explore; return 1; }
    if (!strcmp(key, "rec_died")) { *out = (int)ags.times_died; return 1; }
    if (!strcmp(key, "rec_wins")) { *out = ags.wins; return 1; }
    if (!strcmp(key, "rec_cherries")) { *out = ags.cherries; return 1; }
    if (!strcmp(key, "cherry_run")) { *out = ag.cherry_run; return 1; }
    if (!strcmp(key, "nrooms")) { *out = apd_nrooms; return 1; }
    if (!strcmp(key, "ndoors")) { *out = apd_ndoors; return 1; }
    if (!strcmp(key, "nitems")) { *out = apd_nitems; return 1; }
    if (!strcmp(key, "npods")) { *out = apd_npods; return 1; }
    if (!strncmp(key, "nitems_", 7)) { int k = atoi(key + 7), n = 0; for (int i = 0; i < apd_nitems; i++) n += apd_items[i].kind == k; *out = n; return 1; }
    if (!strncmp(key, "rooms_", 6)) { int s = atoi(key + 6), n = 0; for (int i = 0; i < apd_nrooms; i++) n += apd_rooms[i].sector == s; *out = n; return 1; }
    if (!strcmp(key, "reach")) { *out = apd_reach_check(); return 1; }
    if (!strcmp(key, "place")) { extern int apd_place_check(void); *out = apd_place_check(); return 1; }
    if (!strcmp(key, "tile")) { *out = apd_tile_at(APD_PX(p->x), APD_PX(p->y)); return 1; }
    if (!strcmp(key, "msg_t")) { *out = ag.msg_t; return 1; }
    if (!strcmp(key, "hits")) { *out = ag.hits_taken; return 1; }
    if (!strcmp(key, "last_hurt")) { *out = ag.last_hurt; return 1; }
    if (!strcmp(key, "frames")) { *out = (int)ag.run.frames; return 1; }
    if (!strcmp(key, "layout")) { *out = apd_layout_audit(); return 1; }
    if (!strcmp(key, "art_bad")) { *out = apd_art_check(); return 1; }
    return 0;
}

static int apd_cheat(const char *cmd) {
    int a, b, c;
    char ch;
    if (!strcmp(cmd, "newgame")) { new_run(false); return 1; }
    if (!strcmp(cmd, "codegame")) { new_run(true); return 1; }
    if (sscanf(cmd, "room %d %c", &a, &ch) == 2) {
        int r = apd_room_find(a, ch);
        if (r < 0) return 0;
        if (ag.state == AS_TITLE || ag.state == AS_INTRO) new_run(false);
        apd_build_room(r);
        ag.run.visited[r] = 1;
        /* stand at the room's first cradle, teleporter, or the middle of its floor */
        int x = 40, y = 0;
        bool found = false;
        for (int i = 0; i < ag.nth && !found; i++)
            if (ag.th[i].kind == TH_POD || ag.th[i].kind == TH_TELE) {
                x = ag.th[i].tx * APD_TILE + 8 + (ag.th[i].kind == TH_POD ? 22 : 0);
                y = (ag.th[i].ty + 1) * APD_TILE - APD_PH / 2;
                found = true;
            }
        if (!found) {
            x = 40;
            y = ag.rh * APD_TILE - APD_TILE - APD_PH / 2;
            for (int yy = y; yy > 0 && apd_box_solid(x, yy, APD_PW, APD_PH, 1, 0); yy -= 4) y = yy - 4;
        }
        ag.pl.x = x * APD_FX;
        ag.pl.y = y * APD_FX;
        ag.pl.grav = 1;
        ag.pl.vy = 0;
        ag.pl.ground = false;
        ag.pl.entry_x = ag.pl.x;
        ag.pl.entry_y = ag.pl.y;
        ag.pl.entry_grav = 1;
        ag.pl.entry_face = 1;
        ag.pl.armor = apd_max_armor();
        set_state(AS_PLAY);
        apd_camera(true);
        apd_room_music();
        return 1;
    }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) {
        ag.pl.x = a * APD_FX;
        ag.pl.y = b * APD_FX;
        ag.pl.vy = 0;
        ag.pl.entry_x = ag.pl.x;
        ag.pl.entry_y = ag.pl.y;
        apd_camera(true);
        return 1;
    }
    if (sscanf(cmd, "grav %d", &a) == 1) { ag.pl.grav = a < 0 ? -1 : 1; ag.pl.entry_grav = ag.pl.grav; return 1; }
    if (sscanf(cmd, "mod %d %d", &a, &b) == 2) { ag.run.mod_sys[a & 3] = (uint8_t)(b < 0 ? MOD_NONE : b); return 1; }
    if (sscanf(cmd, "dampers %d", &a) == 1) { ag.run.dampers = (uint8_t)iclamp(a, 0, 2); return 1; }
    if (sscanf(cmd, "pass %d", &a) == 1) { ag.run.pass = (uint8_t)iclamp(a, 0, 3); return 1; }
    if (sscanf(cmd, "ciphers %d", &a) == 1) { ag.run.ciphers = (uint8_t)(a & 15); return 1; }
    if (sscanf(cmd, "armor %d", &a) == 1) { ag.pl.armor = a; return 1; }
    if (sscanf(cmd, "cells %d", &a) == 1) { ag.run.cells = (uint8_t)a; ag.pl.armor = apd_max_armor(); return 1; }
    if (sscanf(cmd, "spares %d", &a) == 1) { ag.spares = a; return 1; }
    if (sscanf(cmd, "sparesmax %d", &a) == 1) { ag.run.spares_max = (uint8_t)a; return 1; }
    if (sscanf(cmd, "pickup %d", &a) == 1) { apd_pickup(a); return 1; }
    if (sscanf(cmd, "gate %d", &a) == 1) { apd_open_gate(a); return 1; }
    if (sscanf(cmd, "foe %d %d %d %d", &a, &b, &c, &(int){0}) >= 3) {
        int up = 0;
        sscanf(cmd, "foe %d %d %d %d", &a, &b, &c, &up);
        apd_spawn_foe(iclamp(a, 1, FO_KINDS - 1), b * APD_FX, c * APD_FX, up != 0);
        return 1;
    }
    if (sscanf(cmd, "killkind %d", &a) == 1) {
        extern void apd_kill_foe(int i);
        for (int i = 0; i < APD_MAX_FOES; i++)
            if (ag.foe[i].alive && ag.foe[i].kind == a) apd_kill_foe(i);
        return 1;
    }
    if (sscanf(cmd, "killfoe %d", &a) == 1) { extern void apd_kill_foe(int i); apd_kill_foe(a); return 1; }
    if (sscanf(cmd, "hurtfoe %d %d", &a, &b) == 2) { apd_hurt_foe(a, b, true); return 1; }
    if (sscanf(cmd, "foehp %d %d", &a, &b) == 2) { if (a >= 0 && a < APD_MAX_FOES) ag.foe[a].hp = (int16_t)b; return 1; }
    if (sscanf(cmd, "eshot %d %d %d %d %d", &a, &b, &c, &(int){0}, &(int){0}) >= 3) {
        int k, x, y, vx = 0, vy = 0;
        sscanf(cmd, "eshot %d %d %d %d %d", &k, &x, &y, &vx, &vy);
        apd_add_eshot(k, x * APD_FX, y * APD_FX, vx, vy, 10);
        return 1;
    }
    if (!strcmp(cmd, "god")) { ag.god = true; return 1; }
    if (!strcmp(cmd, "mortal")) { ag.god = false; return 1; }
    if (!strcmp(cmd, "nofoes")) { ag.no_foes = true; return 1; }
    if (!strcmp(cmd, "foes")) { ag.no_foes = false; return 1; }
    if (!strcmp(cmd, "clear")) { memset(ag.foe, 0, sizeof ag.foe); memset(ag.es, 0, sizeof ag.es); return 1; }
    if (!strcmp(cmd, "boss")) { if (ag.boss >= 0 && !ag.boss_on) apd_boss_spawn(ag.boss); return 1; }
    if (sscanf(cmd, "bosshp %d", &a) == 1) { if (ag.boss_foe >= 0) ag.foe[ag.boss_foe].hp = (int16_t)a; return 1; }
    if (sscanf(cmd, "bosst %d", &a) == 1) { if (ag.boss_foe >= 0) { ag.foe[ag.boss_foe].t = a; ag.boss_t = a; } return 1; }
    if (sscanf(cmd, "bossstate %d", &a) == 1) { if (ag.boss_foe >= 0) { ag.foe[ag.boss_foe].state = a; ag.foe[ag.boss_foe].t = 0; } return 1; }
    if (!strcmp(cmd, "killboss")) { if (ag.boss_foe >= 0) apd_boss_hurt(ag.boss_foe, 30000); return 1; }
    if (!strcmp(cmd, "die")) { ag.god = false; ag.pl.inv = 0; ag.pl.armor = 1; apd_player_hurt(50, 0, false); return 1; }
    if (sscanf(cmd, "hurt %d", &a) == 1) { ag.pl.inv = 0; apd_player_hurt(a, 0, false); return 1; }
    if (!strcmp(cmd, "visitall")) { for (int i = 0; i < apd_nrooms; i++) ag.run.visited[i] = 1; return 1; }
    if (!strcmp(cmd, "allitems")) { for (int i = 0; i < apd_nitems; i++) if (!ag.run.item_got[i]) apd_pickup(i); ag.msg_t = 0; return 1; }
    if (sscanf(cmd, "dump %d %c", &a, &ch) == 2) {
        int r = apd_room_find(a, ch);
        if (r < 0) return 0;
        ApdGame keep = ag;
        apd_build_room(r);
        printf("room %d %c (%d x %d)\n", a, ch, apd_rooms[r].w, apd_rooms[r].h);
        apd_room_dump(r);
        ag = keep;
        return 1;
    }
    if (!strcmp(cmd, "dumpall")) {
        ApdGame keep = ag;
        for (int r = 0; r < apd_nrooms; r++) {
            apd_build_room(r);
            printf("room %d %c (%d x %d) %s\n", apd_rooms[r].sector, apd_rooms[r].id, apd_rooms[r].w, apd_rooms[r].h, apd_room_name(r));
            apd_room_dump(r);
        }
        ag = keep;
        return 1;
    }
    if (sscanf(cmd, "botplan %d", &a) == 1) { apd_bot_plan = a; return 1; }
    if (sscanf(cmd, "botlog %d", &a) == 1) { extern int apd_bot_log; apd_bot_log = a; return 1; }
    if (sscanf(cmd, "menu %d", &a) == 1) { ag.menu = a; return 1; }
    if (!strcmp(cmd, "win")) { apd_final_defeated(); return 1; }
    if (!strcmp(cmd, "cherrywin")) {
        for (int i = 0; i < apd_nrooms; i++) ag.run.visited[i] = 1;
        for (int i = 0; i < apd_nitems; i++) ag.run.item_got[i] = 1;
        ag.run.boss_dead[BS_HALLORAN] = 1;
        apd_final_defeated();
        return 1;
    }
    if (sscanf(cmd, "credits %d", &a) == 1) { ag.state_t = a; return 1; }
    if (!strcmp(cmd, "save")) { ags.has_run = 1; ags.run = ag.run; apd_save_now(); return 1; }
    return 0;
}

const GameDef GAME_ANTIPODE = {
    "antipode",
    "ANTIPODE",
    "1987",
    "ACTION ADVENTURE",
    "A SUIT ON A STATION GONE WRONG. JUMP AGAIN IN THE AIR TO FLIP GRAVITY.",
    {"GET A MOD", "BEAT HALLORAN IN THE CROWN", "BEAT HIM WITH EVERY ROOM AND UPGRADE"},
    GLYPH_LEFT GLYPH_RIGHT "\tWALK\n"
    GLYPH_DOWN "\tCROUCH\n"
    GLYPH_A " (HOLD)\tFIRE (FACING LOCKS)\n"
    GLYPH_UP "+" GLYPH_A "\tFIRE OVER YOUR HEAD\n"
    GLYPH_DOWN "+" GLYPH_A "\tFIRE UNDER YOU (IN THE AIR)\n"
    GLYPH_B "\tJUMP (HOLD: HIGHER)\n"
    GLYPH_B " IN THE AIR\tFLIP GRAVITY\n"
    GLYPH_UP "\tCRADLE, TERMINAL, TELEPORTER\n"
    "START\tPAUSE",
    C_NAVY, C_PINK,
    apd_load, apd_start, apd_update, apd_draw, apd_quit, apd_draw_label, apd_query, apd_cheat,
    "VAINGER", 29,
    NULL,
};
