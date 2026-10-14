/* LIMPET - the last crew tank of Spindle Station.
 * Cartridge 20 of UFO 40, a tribute to Warptank (UFO 50 #20).
 * See docs/games/20-limpet.md. This file: the title, the story, the hub
 * (pods, gates, people, paint bays), entering and clearing decks, dying and
 * the choice after it, the endings, the save, the goals and the test hooks. */
#include "lmp.h"

LmpGame lg;
LmpSave lgs;

#define SAVE_MAGIC 0x4C4D5001u

bool lmp_simulating;
uint32_t lmp_sim_cur, lmp_sim_prev;

void lmp_sfx(const char *name) {
    if (!lmp_simulating) sfx_play_name(name);
}
void lmp_music(int id) {
    if (!lmp_simulating) music_play(id);
}
static void award(int bit) {
    if (!lmp_simulating) game_award(bit);
}

void lmp_say(const char *m) {
    snprintf(lg.msg, sizeof lg.msg, "%s", m);
    lg.msg_t = 150;
}

static void set_state(int s) {
    lg.state = s;
    lg.state_t = 0;
}

int lmp_cleared_count(void) {
    int n = 0;
    for (int i = 0; i < LMP_NORMAL_DECKS; i++) n += (lgs.cleared >> i) & 1;
    return n;
}

int lmp_honey_count(void) {
    int n = 0;
    for (int i = 0; i < 27; i++) n += (lgs.honey >> i) & 1;
    return n;
}

void lmp_save_now(void) {
    if (lmp_simulating) return;
    lgs.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &lgs, (int)sizeof lgs);
}

static void load_save(void) {
    LmpSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { lgs = tmp; return; }
    memset(&lgs, 0, sizeof lgs);
    lgs.magic = SAVE_MAGIC;
    lgs.last_pod = 255;
}

static bool cherry_done(void) { return lgs.secret && lmp_honey_count() >= LMP_HONEY_TOTAL; }

/* the goals come back from the save (RESET GOALS clears both) */
static void goals_from_save(void) {
    if (lgs.turncoat) award(GOAL_BEACON);
    if (lgs.dread) award(GOAL_SAUCER);
    if (cherry_done()) award(GOAL_ALIEN);
}

/* ---- the hub ----------------------------------------------------------------- */

static int hub_zone_at(int x, int y) {
    for (int i = 0; i < LMP_HUB_NZONES; i++) {
        const LmpZone *z = &LMP_HUB_ZONE[i];
        if (x >= z->x0 && x <= z->x1 && y >= z->y0 && y <= z->y1) return z->zone;
    }
    return ZN_CONCOURSE;
}

static int zone_music(int z) {
    switch (z) {
    case ZN_CANTEEN: return LMP_MUS_CANTEEN;
    case ZN_GRISTLE: return LMP_MUS_GRISTLE;
    case ZN_UPPER: return LMP_MUS_UPPER;
    default: return LMP_MUS_CONCOURSE;
    }
}

static int nth_cell(char c, int idx, int *ox, int *oy) {
    int k = 0;
    for (int y = 0; y < lg.mh; y++)
        for (int x = 0; x < lg.mw; x++)
            if (lg.map[y][x] == c) {
                if (k == idx) { *ox = x; *oy = y; return 1; }
                k++;
            }
    return 0;
}

static int index_of_cell(char c, int cx, int cy) {
    int k = 0;
    for (int y = 0; y < lg.mh; y++)
        for (int x = 0; x < lg.mw; x++)
            if (lg.map[y][x] == c) {
                if (x == cx && y == cy) return k;
                k++;
            }
    return -1;
}

static void place_on_floor(int x, int y) {
    if (lmp_state_ok(x, y, DR_U)) { lmp_tank_place(x, y, DR_U); return; }
    for (int n = 0; n < 8; n += 2)
        if (lmp_state_ok(x, y, n)) { lmp_tank_place(x, y, n); return; }
    lmp_tank_place(x, y, DR_U);
}

static void enter_hub(bool at_start) {
    int col = lgs.colour;
    lmp_load_level(LV_HUB);
    lg.tk.colour = col;
    int px, py;
    if (!at_start && lgs.last_pod < LMP_HUB_NPODS && nth_cell('$', lgs.last_pod, &px, &py)) place_on_floor(px, py);
    lg.tk.colour = col;
    lg.check.set = false;
    lmp_take_checkpoint();
    lg.pod_here = -1;
    lg.talk_npc = -1;
    game_pause_items(0, NULL, NULL);
    lmp_music(zone_music(hub_zone_at(lg.tk.x, lg.tk.y)));
    set_state(GS_PLAY);
}

static void leave_pick(int i) {
    (void)i;
    if (lg.level != LV_HUB) enter_hub(false);
}

static int deck_music(int level) {
    const LmpLevel *L = LMP_LEVEL[level];
    switch (L->kind) {
    case LK_SURVIVAL: return LMP_MUS_SURVIVAL;
    case LK_SECRET: return LMP_MUS_COMB;
    case LK_TURNCOAT: return LMP_MUS_TURNCOAT;
    case LK_DREAD: return LMP_MUS_DREAD;
    default: return LMP_MUS_DECK[L->music & 3];
    }
}

static void enter_level(int level) {
    int col = lgs.colour;
    lmp_load_level(level);
    lg.tk.colour = col;
    lg.deaths_here = 0;
    static const char *const ITEMS[1] = {"LEAVE DECK"};
    if (!lmp_simulating) game_pause_items(1, ITEMS, leave_pick);
    lmp_music(deck_music(level));
    set_state(GS_PLAY);
}

static void hub_update(void) {
    LmpTank *t = &lg.tk;
    lg.pod_here = -1;
    if (t->pn == 0) {
        if (lmp_tile(t->x, t->y) == '$') lg.pod_here = index_of_cell('$', t->x, t->y);
        if (lmp_axis(t->n)) {
            int sx = t->x - LMP_DX[t->n], sy = t->y - LMP_DY[t->n];
            if (lmp_tile(sx, sy) == 'V' && !lmp_simulating) {
                int bay = index_of_cell('V', sx, sy);
                if (bay >= 0 && lgs.colour != bay + 1) {
                    lgs.colour = (uint8_t)(bay + 1);
                    lgs.colours |= (uint8_t)(1 << bay);
                    t->colour = lgs.colour;
                    lmp_sfx("lmp_paint");
                    lmp_save_now();
                }
            }
        }
    }
    /* people talk while the tank is next to them */
    lg.talk_npc = -1;
    for (int dy = -1; dy <= 1 && lg.talk_npc < 0; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (lmp_tile(t->x + dx, t->y + dy) == 'N') { lg.talk_npc = index_of_cell('N', t->x + dx, t->y + dy); break; }
    lmp_music(zone_music(hub_zone_at(t->x, t->y)));
    if (lg.pod_here >= 0 && lg.pod_here < LMP_HUB_NPODS && lmp_btnp(BTN_B) && t->pn == 0 && !lmp_simulating) {
        lgs.last_pod = (uint8_t)lg.pod_here;
        lgs.started = 1;
        lmp_save_now();
        lmp_sfx("lmp_enter");
        input_consume();
        enter_level(LMP_HUB_POD[lg.pod_here].level);
    }
}

/* ---- decks ------------------------------------------------------------------ */

void lmp_level_cleared(void) {
    if (lg.state != GS_PLAY) return;
    if (lmp_simulating) { set_state(GS_CLEAR); return; }
    int lv = lg.level;
    lg.clear_level = lv;
    lg.clear_time = lg.timer;
    lg.clear_honey = 0;
    lg.clear_new = 0;
    if (lv < LMP_SECTORS) {
        if (!((lgs.cleared >> lv) & 1)) lg.clear_new = 1;
        lgs.cleared |= 1u << lv;
        if (lgs.best[lv] == 0 || (uint32_t)lg.timer < lgs.best[lv]) lgs.best[lv] = (uint32_t)lg.timer;
        lg.clear_best = (int)lgs.best[lv];
        if (lg.honey_held && lv < LMP_NORMAL_DECKS) {
            if (!((lgs.honey >> lv) & 1)) lg.clear_honey = 1;
            lgs.honey |= 1u << lv;
        }
        if (lv == LMP_SECTORS - 1) lgs.secret = 1;
    } else if (lv == LV_TURNCOAT) {
        lgs.turncoat = 1;
    } else if (lv == LV_DREAD) {
        if (lg.honey_held) {
            if (!((lgs.honey >> 26) & 1)) lg.clear_honey = 1;
            lgs.honey |= 1u << 26;
        }
    }
    lgs.started = 1;
    goals_from_save();
    lmp_save_now();
    lmp_music(LMP_MUS_CLEAR);
    lmp_sfx("lmp_clear");
    game_pause_items(0, NULL, NULL);
    set_state(GS_CLEAR);
}

void lmp_tank_died(int cause) {
    (void)cause;
    int x, y;
    lmp_tank_centre(&x, &y);
    lmp_burst(x, y, C_ORANGE, 18, 3);
    lmp_burst(x, y, C_YELLOW, 10, 2);
    lg.shake = 8;
    lg.deaths_here++;
    lg.sim_deaths++;
    if (!lmp_simulating && lgs.deaths < 65535) lgs.deaths++;
    lmp_sfx("lmp_die");
    lg.honey_held = false;
    lg.tk.pn = 0;
    set_state(GS_DEAD);
}

void lmp_play_step(void) {
    if (lg.msg_t > 0) lg.msg_t--;
    if (lg.level != LV_HUB) lg.timer++;
    lmp_world_update();
    if (lg.state != GS_PLAY) return;
    lmp_tank_update();
    if (lg.state != GS_PLAY) return;
    lmp_shots_update();
    lmp_foes_update();
    if (lg.state != GS_PLAY) return;
    lmp_eshots_update();
    lmp_survival_update();
    lmp_parts_update();
    if (lg.shake > 0) lg.shake--;
    if (lg.level == LV_HUB) hub_update();
    if (lg.boss_dead && lg.level == LV_TURNCOAT) award(GOAL_BEACON);
    if (lg.boss_dead && lg.level == LV_DREAD && !lgs.dread && !lmp_simulating) {
        lgs.dread = 1;
        award(GOAL_SAUCER);
        lmp_save_now();
    }
    if (lg.boss_dead && lg.level == LV_TURNCOAT && !lgs.turncoat && !lmp_simulating) {
        lgs.turncoat = 1;
        lmp_save_now();
    }
}

/* ---- the story and the endings ------------------------------------------------- */

const char *const LMP_STORY[] = {
    "SPINDLE STATION, HIGH ORBIT, 1985.",
    "",
    "THE GRISTLE CAME UP THROUGH THE CARGO PODS",
    "FROM THE UNDERNEATH, AND THE CREW FLED.",
    "",
    "ONE HULL TANK STAYED BEHIND: LIMPET.",
    "IT CAN'T JUMP. IT CAN'T FALL. IT STICKS.",
    "",
    "CLEAR EVERY DECK. SHUT EVERY POD.",
};
const int LMP_STORY_N = (int)(sizeof LMP_STORY / sizeof LMP_STORY[0]);

const char *const LMP_ENDING[] = {
    "THE DREADNOUGHT COMES APART BOLT BY BOLT.",
    "",
    "WITHOUT IT THE GRISTLE DRIES TO A CRUST",
    "AND FLAKES AWAY INTO THE DARK.",
    "",
    "SPINDLE STATION TURNS SLOWLY, QUIET AGAIN.",
    "LIMPET CLINGS TO THE HULL AND WATCHES",
    "THE SUN COME UP OVER THE EARTH.",
};
const int LMP_ENDING_N = (int)(sizeof LMP_ENDING / sizeof LMP_ENDING[0]);

const char *const LMP_CHERRY_END[] = {
    "DEEP IN THE COMB, THE LAST POD SHUTS.",
    "",
    "THE BEES OF THE HYDRO DECK COME HOME,",
    "EVERY JAR OF HONEY BACK ON ITS SHELF.",
    "",
    "THE CREW WILL RETURN TO A STATION",
    "THAT SMELLS OF HONEY AND HOT METAL.",
    "",
    "THE LINK TO THE UNDERNEATH IS GONE.",
};
const int LMP_CHERRY_END_N = (int)(sizeof LMP_CHERRY_END / sizeof LMP_CHERRY_END[0]);

/* ---- the frame --------------------------------------------------------------- */

static void new_game(void) {
    uint8_t keep_col = 0;
    (void)keep_col;
    uint16_t deaths = lgs.deaths, wins = lgs.wins;
    memset(&lgs, 0, sizeof lgs);
    lgs.magic = SAVE_MAGIC;
    lgs.last_pod = 255;
    lgs.deaths = deaths;
    lgs.wins = wins;
    lgs.started = 1;
    lmp_save_now();
}

static void to_title(void) {
    set_state(GS_TITLE);
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    lg.menu = lgs.started ? 0 : 1;
    music_play(LMP_MUS_TITLE);
}

static void lmp_update(void) {
    lg.state_t++;
    lg.frame_t++;
    switch (lg.state) {
    case GS_TITLE: {
        game_set_pausable(false);
        int items = lgs.started ? 2 : 1;
        if (lg.talk_t > 0) {
            /* the question before a new game wipes the log */
            if (btnp(BTN_LEFT) || btnp(BTN_RIGHT)) { lg.talk_npc ^= 1; sfx_play_name("ui_move"); }
            if (btnp(BTN_B)) { lg.talk_t = 0; sfx_play_name("ui_back"); }
            else if (btnp(BTN_A)) {
                input_consume();
                lg.talk_t = 0;
                if (lg.talk_npc == 1) { new_game(); set_state(GS_STORY); music_play(LMP_MUS_STORY); }
            }
            break;
        }
        if (items == 2 && (btnp(BTN_UP) || btnp(BTN_DOWN))) { lg.menu ^= 1; sfx_play_name("ui_move"); }
        if (items == 1) lg.menu = 1;
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if ((btnp(BTN_A) || btnp(BTN_START)) && lg.state_t > 5) {
            sfx_play_name("ui_ok");
            input_consume();
            if (lg.menu == 0) { enter_hub(false); game_set_pausable(true); }
            else if (lgs.started) { lg.talk_t = 1; lg.talk_npc = 0; }
            else { new_game(); set_state(GS_STORY); music_play(LMP_MUS_STORY); }
        }
        break;
    }
    case GS_STORY:
        game_set_pausable(false);
        if ((lg.state_t > 20 && (btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_START))) || lg.state_t > 1200) {
            input_consume();
            lgs.last_pod = 255;
            enter_hub(true);
        }
        break;
    case GS_PLAY:
        game_set_pausable(true);
        lmp_play_step();
        break;
    case GS_DEAD:
        game_set_pausable(true);
        lmp_parts_update();
        if (lg.shake > 0) lg.shake--;
        if (lg.state_t > LMP_DEAD_T) {
            if (lg.level == LV_HUB) { lmp_restore_checkpoint(); set_state(GS_PLAY); }
            else { set_state(GS_PROMPT); lg.menu = 0; lmp_music(LMP_MUS_DEATH); }
        }
        break;
    case GS_PROMPT:
        game_set_pausable(false);
        if (lmp_btnp(BTN_UP) || lmp_btnp(BTN_DOWN)) { lg.menu ^= 1; lmp_sfx("ui_move"); }
        if (lmp_btnp(BTN_A) || lmp_btnp(BTN_START)) {
            if (!lmp_simulating) input_consume();
            if (lg.menu == 0) {
                lmp_restore_checkpoint();
                lmp_music(deck_music(lg.level));
                set_state(GS_PLAY);
            } else {
                enter_hub(false);
            }
        }
        break;
    case GS_CLEAR:
        game_set_pausable(false);
        lmp_parts_update();
        if (lg.state_t > 200 || (lg.state_t > 40 && btnp(BTN_A))) {
            input_consume();
            if (lg.clear_level == LV_DREAD) {
                if (lgs.wins < 65535) lgs.wins++;
                lmp_save_now();
                set_state(GS_ENDING);
                music_play(LMP_MUS_ENDING);
            } else if (lg.clear_level == LMP_SECTORS - 1) {
                set_state(GS_CHERRY);
                music_play(LMP_MUS_ENDING);
            } else {
                enter_hub(false);
            }
        }
        break;
    case GS_ENDING:
    case GS_CHERRY:
        game_set_pausable(false);
        if (btn(BTN_A)) lg.state_t += 3;
        if (lg.state_t > 1500) {
            input_consume();
            lgs.ends_seen |= lg.state == GS_ENDING ? 1 : 2;
            lgs.last_pod = 255;
            lmp_save_now();
            /* back at the start of the station, the bees out to show the honey */
            enter_hub(true);
        }
        break;
    default: break;
    }
}

/* ---- cartridge interface ---------------------------------------------------- */

static void lmp_load(void) {
    lmp_art_load();
    lmp_audio_load();
}

static void lmp_start(void) {
    load_save();
    memset(&lg, 0, sizeof lg);
    rng_seed(&lg.rng, 20);
    lg.talk_npc = -1;
    goals_from_save();
    to_title();
}

static void lmp_quit(void) { lmp_save_now(); }

static int count_es(int kind) {
    int n = 0;
    for (int i = 0; i < LMP_MAX_ESHOTS; i++) n += lg.es[i].alive && (kind < 0 || lg.es[i].kind == kind);
    return n;
}

static int boss_index(void) {
    for (int i = 0; i < LMP_MAX_FOES; i++)
        if (lg.foe[i].alive && (lg.foe[i].kind == FO_TURNCOAT || lg.foe[i].kind == FO_DREAD)) return i;
    return -1;
}

static int lmp_query(const char *key, int *out) {
    const LmpTank *t = &lg.tk;
    if (!strcmp(key, "bot")) { *out = lmp_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = lg.state; return 1; }
    if (!strcmp(key, "menu")) { *out = lg.menu; return 1; }
    if (!strcmp(key, "level")) { *out = lg.level; return 1; }
    if (!strcmp(key, "kind")) { *out = lg.kind; return 1; }
    if (!strcmp(key, "tx")) { *out = t->x; return 1; }
    if (!strcmp(key, "ty")) { *out = t->y; return 1; }
    if (!strcmp(key, "tn")) { *out = t->n; return 1; }
    if (!strcmp(key, "moving")) { *out = t->pn > 0; return 1; }
    if (!strcmp(key, "lag")) { *out = t->lag; return 1; }
    if (!strcmp(key, "fizz")) { *out = t->fizz_t; return 1; }
    if (!strcmp(key, "blast")) { *out = t->blast_t; return 1; }
    if (!strcmp(key, "colour")) { *out = t->colour; return 1; }
    if (!strcmp(key, "shots")) { int n = 0; for (int i = 0; i < LMP_MAX_SHOTS; i++) n += lg.shot[i].alive; *out = n; return 1; }
    if (!strcmp(key, "shot_vx")) { *out = 0; for (int i = 0; i < LMP_MAX_SHOTS; i++) if (lg.shot[i].alive) { *out = lg.shot[i].vx; break; } return 1; }
    if (!strcmp(key, "shot_vy")) { *out = 0; for (int i = 0; i < LMP_MAX_SHOTS; i++) if (lg.shot[i].alive) { *out = lg.shot[i].vy; break; } return 1; }
    if (!strcmp(key, "eshots")) { *out = count_es(-1); return 1; }
    if (!strncmp(key, "eshots_", 7)) { *out = count_es(atoi(key + 7)); return 1; }
    if (!strcmp(key, "foes")) { *out = lmp_foes_alive(); return 1; }
    if (!strncmp(key, "foes_", 5)) { *out = lmp_count_foes(atoi(key + 5)); return 1; }
    if (!strncmp(key, "foe", 3) && isdigit((unsigned char)key[3])) {
        int i = atoi(key + 3);
        const char *u = strchr(key + 3, '_');
        if (i < 0 || i >= LMP_MAX_FOES || !u) return 0;
        const LmpFoe *f = &lg.foe[i];
        if (!strcmp(u, "_alive")) { *out = f->alive; return 1; }
        if (!strcmp(u, "_kind")) { *out = f->kind; return 1; }
        if (!strcmp(u, "_hp")) { *out = f->alive ? f->hp : 0; return 1; }
        if (!strcmp(u, "_cx")) { *out = f->cx; return 1; }
        if (!strcmp(u, "_cy")) { *out = f->cy; return 1; }
        if (!strcmp(u, "_n")) { *out = f->n; return 1; }
        if (!strcmp(u, "_x")) { *out = f->x / LMP_FX; return 1; }
        if (!strcmp(u, "_y")) { *out = f->y / LMP_FX; return 1; }
        if (!strcmp(u, "_state")) { *out = f->state; return 1; }
        return 0;
    }
    if (!strcmp(key, "boss_hp")) { int b = boss_index(); *out = b >= 0 ? lg.foe[b].hp : 0; return 1; }
    if (!strcmp(key, "boss_state")) { int b = boss_index(); *out = b >= 0 ? lg.foe[b].state : -1; return 1; }
    if (!strcmp(key, "boss_dead")) { *out = lg.boss_dead; return 1; }
    if (!strcmp(key, "parity")) { *out = lg.parity; return 1; }
    if (!strcmp(key, "bulb")) { *out = lg.bulb_out; return 1; }
    if (!strcmp(key, "timed")) { *out = lg.timed_t; return 1; }
    if (!strcmp(key, "timer")) { *out = lg.timer; return 1; }
    if (!strcmp(key, "wave")) { *out = lg.wave; return 1; }
    if (!strcmp(key, "honey_shown")) { *out = lg.honey_shown; return 1; }
    if (!strcmp(key, "honey_held")) { *out = lg.honey_held; return 1; }
    if (!strcmp(key, "honey_x")) { *out = lg.honey_x; return 1; }
    if (!strcmp(key, "honey_y")) { *out = lg.honey_y; return 1; }
    if (!strcmp(key, "check_x")) { *out = lg.check.x; return 1; }
    if (!strcmp(key, "check_y")) { *out = lg.check.y; return 1; }
    if (!strcmp(key, "exit_x")) { *out = lg.exit_x; return 1; }
    if (!strcmp(key, "exit_y")) { *out = lg.exit_y; return 1; }
    if (!strcmp(key, "pod_here")) { *out = lg.pod_here; return 1; }
    if (!strcmp(key, "pod_level")) { *out = lg.pod_here >= 0 ? LMP_HUB_POD[lg.pod_here].level : -1; return 1; }
    if (!strcmp(key, "talk")) { *out = lg.talk_npc; return 1; }
    if (!strcmp(key, "zone")) { *out = hub_zone_at(t->x, t->y); return 1; }
    if (!strcmp(key, "cleared")) { *out = lmp_cleared_count(); return 1; }
    if (!strcmp(key, "cleared_bits")) { *out = (int)lgs.cleared; return 1; }
    if (!strcmp(key, "honey")) { *out = lmp_honey_count(); return 1; }
    if (!strcmp(key, "honey_bits")) { *out = (int)lgs.honey; return 1; }
    if (!strcmp(key, "turncoat")) { *out = lgs.turncoat; return 1; }
    if (!strcmp(key, "dread")) { *out = lgs.dread; return 1; }
    if (!strcmp(key, "secret")) { *out = lgs.secret; return 1; }
    if (!strcmp(key, "started")) { *out = lgs.started; return 1; }
    if (!strcmp(key, "colours")) { *out = lgs.colours; return 1; }
    if (!strcmp(key, "save_colour")) { *out = lgs.colour; return 1; }
    if (!strcmp(key, "save_deaths")) { *out = lgs.deaths; return 1; }
    if (!strcmp(key, "save_wins")) { *out = lgs.wins; return 1; }
    if (!strcmp(key, "ends")) { *out = lgs.ends_seen; return 1; }
    if (!strncmp(key, "best", 4) && isdigit((unsigned char)key[4])) { *out = (int)lgs.best[iclamp(atoi(key + 4), 0, LMP_SECTORS - 1)]; return 1; }
    if (!strcmp(key, "total_best")) { uint32_t s = 0; for (int i = 0; i < LMP_SECTORS; i++) s += lgs.best[i]; *out = (int)s; return 1; }
    if (!strcmp(key, "deaths_here")) { *out = lg.deaths_here; return 1; }
    if (!strcmp(key, "hits")) { *out = lg.hits; return 1; }
    if (!strcmp(key, "last_hurt")) { *out = lg.last_hurt; return 1; }
    if (!strcmp(key, "cam_x")) { *out = lg.cam_x; return 1; }
    if (!strcmp(key, "cam_y")) { *out = lg.cam_y; return 1; }
    if (!strcmp(key, "mw")) { *out = lg.mw; return 1; }
    if (!strcmp(key, "mh")) { *out = lg.mh; return 1; }
    if (!strcmp(key, "movers")) { *out = lg.nmover; return 1; }
    if (!strcmp(key, "mover0_x")) { *out = lg.nmover ? lg.mover[0].x : -1; return 1; }
    if (!strcmp(key, "mover0_y")) { *out = lg.nmover ? lg.mover[0].y : -1; return 1; }
    if (!strncmp(key, "tile_", 5)) {
        int x = 0, y = 0;
        if (sscanf(key + 5, "%d_%d", &x, &y) != 2) return 0;
        *out = lmp_tile(x, y);
        return 1;
    }
    if (!strncmp(key, "count_", 6)) { *out = lmp_count_tiles(key[6]); return 1; }
    if (!strcmp(key, "warp_ok")) { int a, b, c; *out = lmp_warp_target(t->x, t->y, t->n, &a, &b, &c); return 1; }
    if (!strcmp(key, "art_bad")) { *out = lmp_art_check(); return 1; }
    if (!strcmp(key, "layout")) { *out = lmp_layout_audit(); return 1; }
    if (!strncmp(key, "reach_", 6)) {
        /* reach_X_Y_N: the demo player's map search, steps or -1 */
        int x = 0, y = 0, n = 0;
        if (sscanf(key + 6, "%d_%d_%d", &x, &y, &n) != 3) return 0;
        *out = lmp_bfs_reach(x, y, n);
        return 1;
    }
    if (!strcmp(key, "levels_ok")) {
        /* every deck: one start, one exit, a bulb (normal decks), a honey and its ! (honey decks) */
        int bad = 0;
        LmpGame keep = lg;
        for (int lv = 0; lv < LMP_LEVELS; lv++) {
            lmp_load_level(lv);
            const LmpLevel *L = LMP_LEVEL[lv];
            if (lv != LV_HUB && lmp_count_tiles('E') != 1) { bad++; fprintf(stderr, "level %d: exits %d\n", lv, lmp_count_tiles('E')); }
            if ((L->kind == LK_NORMAL || L->kind == LK_SECRET) && lmp_count_tiles('R') < 1) { bad++; fprintf(stderr, "level %d: no bulb\n", lv); }
            bool honey_deck = L->kind == LK_NORMAL;
            if (honey_deck != (lg.honey_x >= 0) || honey_deck != (lmp_count_tiles('!') >= 1)) { bad++; fprintf(stderr, "level %d: honey\n", lv); }
            if (lmp_count_tiles('@') != 0) bad++;
        }
        lg = keep;
        *out = bad;
        return 1;
    }
    return 0;
}

static int lmp_cheat(const char *cmd) {
    int a, b, c;
    if (sscanf(cmd, "level %d", &a) == 1) {
        lgs.started = 1;
        if (a == LV_HUB) enter_hub(true);
        else enter_level(iclamp(a, 0, LMP_LEVELS - 1));
        game_set_pausable(true);
        return 1;
    }
    if (!strcmp(cmd, "hub")) { lgs.started = 1; enter_hub(false); game_set_pausable(true); return 1; }
    if (sscanf(cmd, "pos %d %d %d", &a, &b, &c) == 3) { lmp_tank_place(a, b, c); lg.tk.colour = lgs.colour; return 1; }
    if (!strcmp(cmd, "god")) { lg.god = true; return 1; }
    if (!strcmp(cmd, "mortal")) { lg.god = false; return 1; }
    if (!strcmp(cmd, "tallyhits")) { lg.tally_hits = true; lg.hits = 0; return 1; }
    if (!strcmp(cmd, "nofoes")) {
        memset(lg.foe, 0, sizeof lg.foe);
        memset(lg.es, 0, sizeof lg.es);
        lg.nspawn = 0;
        return 1;
    }
    {
        int k = DR_U;
        if (sscanf(cmd, "foe %d %d %d %d", &a, &b, &c, &k) >= 3) {
            lmp_spawn_foe(iclamp(a, 1, FO_KINDS - 1), b, c, k);
            return 1;
        }
    }
    if (sscanf(cmd, "setcleared %d", &a) == 1) {
        lgs.cleared = 0;
        for (int i = 0; i < a && i < LMP_NORMAL_DECKS; i++) lgs.cleared |= 1u << i;
        lgs.started = 1;
        lmp_save_now();
        return 1;
    }
    if (sscanf(cmd, "clearbits %d", &a) == 1) { lgs.cleared = (uint32_t)a; lmp_save_now(); return 1; }
    if (sscanf(cmd, "sethoney %d", &a) == 1) {
        lgs.honey = 0;
        for (int i = 0, k = 0; i < 27 && k < a; i++) {
            if (i < LMP_NORMAL_DECKS && LMP_LEVEL[i]->kind != LK_NORMAL) continue;
            lgs.honey |= 1u << i;
            k++;
        }
        lmp_save_now();
        return 1;
    }
    if (sscanf(cmd, "turncoat %d", &a) == 1) { lgs.turncoat = (uint8_t)a; lmp_save_now(); return 1; }
    if (sscanf(cmd, "dread %d", &a) == 1) { lgs.dread = (uint8_t)a; lmp_save_now(); return 1; }
    if (!strcmp(cmd, "killall")) {
        for (int i = 0; i < LMP_MAX_FOES; i++)
            if (lg.foe[i].alive && lg.foe[i].kind != FO_TURNCOAT && lg.foe[i].kind != FO_DREAD) lg.foe[i].alive = 0;
        return 1;
    }
    if (sscanf(cmd, "bosshp %d", &a) == 1) { int i = boss_index(); if (i >= 0) lg.foe[i].hp = (int16_t)a; return 1; }
    if (sscanf(cmd, "bossstate %d %d", &a, &b) == 2) { int i = boss_index(); if (i >= 0) { lg.foe[i].state = a; lg.foe[i].t = b; } return 1; }
    if (sscanf(cmd, "bosspos %d", &a) == 1) { int i = boss_index(); if (i >= 0) lg.foe[i].cx = a; return 1; }
    if (!strcmp(cmd, "die")) { lg.god = false; lg.tally_hits = false; lmp_kill_tank(0); return 1; }
    if (!strcmp(cmd, "reveal")) { lg.honey_shown = true; return 1; }
    if (!strcmp(cmd, "holdhoney")) { if (lg.honey_x >= 0) { lg.honey_shown = lg.honey_held = true; } return 1; }
    if (!strcmp(cmd, "bulb")) { lmp_open_exit(); return 1; }
    if (!strcmp(cmd, "finish")) { lmp_level_cleared(); return 1; }
    if (sscanf(cmd, "parity %d", &a) == 1) { lg.parity = a & 1; return 1; }
    if (sscanf(cmd, "timer %d", &a) == 1) { lg.timer = a; return 1; }
    if (sscanf(cmd, "botplan %d", &a) == 1) { lmp_bot_honey = a; return 1; }
    if (sscanf(cmd, "wave %d", &a) == 1) { lg.wave = a; lg.wave_t = 1; return 1; }
    if (sscanf(cmd, "colour %d", &a) == 1) { lgs.colour = (uint8_t)a; lg.tk.colour = a; return 1; }
    return 0;
}

const GameDef GAME_LIMPET = {
    "limpet",
    "LIMPET",
    "1985",
    "ADVENTURE PUZZLE",
    "A TANK THAT STICKS TO ANY HULL. ROLL ON WALLS, WARP ACROSS, CLEAR THE STATION.",
    {"SCRAP THE TURNCOAT", "SCRAP THE DREADNOUGHT", "BANK ALL 24 HONEY AND CLEAR THE COMB"},
    GLYPH_DPAD "\tROLL ALONG THE SURFACE\n"
    "TAP " GLYPH_A "\tFIRE (STRAIGHT OUT)\n"
    "HOLD " GLYPH_A "\tA BURST OF THREE\n"
    GLYPH_B "\tWARP TO THE FAR SURFACE\n"
    GLYPH_B " AT A POD\tGO IN\n"
    "START\tPAUSE",
    C_TEAL, C_AMBER,
    lmp_load, lmp_start, lmp_update, lmp_draw, lmp_quit, lmp_draw_label, lmp_query, lmp_cheat,
    "WARPTANK", 20,
    NULL,
};

int lmp_hub_zone(void) { return hub_zone_at(lg.tk.x, lg.tk.y); }
