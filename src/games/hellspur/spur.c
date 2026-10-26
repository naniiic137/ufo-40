/* HELLSPUR - cartridge 40 of UFO 40, a tribute to Grimstone (UFO 50 #40).
 * See docs/games/40-hellspur.md. This file: the title (CONTINUE / NEW GAME
 * and the two records), the story pages and the burning saloon, the run of
 * states (field, battle, ending), saving, the goals and the test hooks. */
#include "spur.h"
#include <ctype.h>

SpurGame sg;
SpurSave spur_records; /* what the save file holds right now */
bool spur_sim;

/* ---- small services -------------------------------------------------------------- */
void spur_sfx(const char *name) {
    if (!spur_sim) sfx_play_name(name);
}

void spur_toast(const char *m) {
    snprintf(sg.msg, sizeof sg.msg, "%s", m);
    sg.msg_t = 90;
}

int spur_rand(int lo, int hi) { return rng_range(&sg.rng, lo, hi); }

bool spur_flag(int f) { return f >= 0 && f < F_COUNT && (sg.s.flags[f / 8] >> (f % 8)) & 1; }
void spur_set_flag(int f) { if (f >= 0 && f < F_COUNT) sg.s.flags[f / 8] |= (uint8_t)(1u << (f % 8)); }
void spur_clear_flag(int f) { if (f >= 0 && f < F_COUNT) sg.s.flags[f / 8] &= (uint8_t)~(1u << (f % 8)); }
bool spur_chest_open(int id) { return id >= 0 && id < SPUR_CHESTS && (sg.s.chests[id / 8] >> (id % 8)) & 1; }
void spur_set_chest(int id) { if (id >= 0 && id < SPUR_CHESTS) sg.s.chests[id / 8] |= (uint8_t)(1u << (id % 8)); }
bool spur_key(int k) { return (sg.s.keyitems >> k) & 1; }
void spur_give_key(int k) { sg.s.keyitems |= 1u << k; }
void spur_take_key(int k) { sg.s.keyitems &= ~(1u << k); }

void spur_set_state(int s) {
    sg.state = s;
    sg.state_t = 0;
}

void spur_award_level_goal(void) {
    if (sg.demo || sg.god) return;
    game_award(GOAL_BEACON);
}

/* the file holds the game in progress and the records */
void spur_write_file(void) {
    if (sg.demo) return;
    spur_records.magic = SPUR_SAVE_MAGIC;
    game_save_write(game_current_index(), &spur_records, (int)sizeof spur_records);
}

/* saving happens in towns: a night at a hotel, leaving town, waking in a chapel */
void spur_save_game(void) {
    if (sg.demo || !spur_flag(F_POSSE)) return;
    int wins = spur_records.wins, cher = spur_records.cherries, best = spur_records.best_battles;
    int maxl = imax(spur_records.max_level, sg.s.max_level);
    spur_records = sg.s;
    spur_records.valid = 1;
    spur_records.map = (uint8_t)sg.map_id;
    spur_records.x = (int8_t)sg.pos[0].x;
    spur_records.y = (int8_t)sg.pos[0].y;
    spur_records.wins = wins;
    spur_records.cherries = cher;
    spur_records.best_battles = best;
    spur_records.max_level = maxl;
    spur_write_file();
}

static void load_file(void) {
    SpurSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SPUR_SAVE_MAGIC) spur_records = tmp;
    else {
        memset(&spur_records, 0, sizeof spur_records);
        spur_records.magic = SPUR_SAVE_MAGIC;
    }
}

static void goals_from_file(void) {
    if (spur_records.max_level >= 10) game_award(GOAL_BEACON);
    if (spur_records.wins > 0) game_award(GOAL_SAUCER);
    if (spur_records.cherries > 0) game_award(GOAL_ALIEN);
}

/* ---- the run --------------------------------------------------------------------------- */
static void to_title(void) {
    spur_set_state(GS_TITLE);
    sg.title_sel = spur_records.valid ? 0 : 1;
    sg.demo = 0;
    sg.ui.kind = UI_NONE;
    game_set_pausable(false);
    music_play(SPUR_MUS_TITLE);
}

static void reset_game(void) {
    int keep_god = sg.god;
    memset(&sg.s, 0, sizeof sg.s);
    sg.s.magic = SPUR_SAVE_MAGIC;
    memset(sg.s.order, 0xFF, sizeof sg.s.order);
    memset(sg.saved, 0, sizeof sg.saved);
    sg.nsaved = 0;
    sg.mounted = 0;
    sg.cherry_run = false;
    sg.god = keep_god;
    spur_clear_wisps();
    sg.ui.kind = UI_NONE;
    sg.pending_battle = sg.pending_event = 0;
    for (int h = 0; h < H_COUNT; h++) spur_hero_init(h);
}

static void new_game(void) {
    reset_game();
    spur_set_state(GS_INTRO);
    sg.intro_page = 0;
    music_play(SPUR_MUS_SALOON);
}

static void start_saloon(void) {
    spur_set_state(GS_FIELD);
    spur_enter_map(M_SALOON, -1, 0, 1);
    game_set_pausable(true);
}

static void continue_game(void) {
    reset_game();
    sg.s = spur_records;
    spur_set_state(GS_FIELD);
    int map = sg.s.map >= SPUR_FIRST_TOWN && sg.s.map < M_COUNT ? sg.s.map : M_GLADWELL;
    spur_enter_map(map, sg.s.x > 0 ? sg.s.x : -1, sg.s.y, 0);
    game_set_pausable(true);
}

/* the demo on the title: the posse the old attract screens showed, playing
 * itself for a minute */
static void start_demo(void) {
    reset_game();
    sg.demo = 1;
    sg.demo_t = 0;
    static const int DEMO[4] = {H_TUCKER, H_HOLLIS, H_CORA, H_BISCUIT};
    for (int i = 0; i < 4; i++) sg.saved[DEMO[i]] = 1;
    sg.nsaved = 4;
    sg.s.nposse = 0;
    for (int h = 0; h < H_COUNT; h++)
        if (sg.saved[h]) { sg.s.hero[h].in = 1; sg.s.order[sg.s.nposse++] = (uint8_t)h; }
    sg.s.teeth = 200;
    sg.s.chapel = M_GLADWELL;
    spur_set_flag(F_POSSE);
    spur_set_state(GS_FIELD);
    spur_enter_map(M_GLADWELL, -1, 0, 1);
}

static void title_update(void) {
    game_set_pausable(false);
    if (sg.state_t > 20 * 60 && !(input_held() & 0xFF)) { start_demo(); return; }
    int n = spur_records.valid ? 2 : 1;
    if (n == 2 && (btnp(BTN_UP) || btnp(BTN_DOWN))) { sg.title_sel ^= 1; sfx_play_name("ui_move"); sg.state_t = 30; }
    if (n == 1) sg.title_sel = 1;
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if ((btnp(BTN_A) || btnp(BTN_START)) && sg.state_t > 10) {
        input_consume();
        sfx_play_name("ui_ok");
        if (sg.title_sel == 0) continue_game();
        else new_game();
    }
}

static void ending_update(void) {
    sg.end_t++;
    if (btn(BTN_A)) sg.end_t += 2;
    int n = 0;
    while (spur_ending_line(n)) n++;
    if (sg.end_t > 160 + n * 70 + 300 && btnp(BTN_A)) { input_consume(); to_title(); }
}

/* ---- the cartridge interface -------------------------------------------------------------- */
static void spur_load(void) {
    spur_art_load();
    spur_audio_load();
}

static void spur_start(void) {
    memset(&sg, 0, sizeof sg);
    load_file();
    rng_seed(&sg.rng, ((uint64_t)rng_next(&g_rng) << 32) | rng_next(&g_rng));
    goals_from_file();
    to_title();
}

static void spur_quit(void) {
    /* nothing is saved on the way out: only hotels, chapels and leaving town save */
}

static void spur_update(void) {
    sg.state_t++;
    sg.ani++;
    if (sg.demo) {
        if (++sg.demo_t > 60 * 60 || (input_held() & 0xFF)) { input_consume(); to_title(); return; }
    }
    switch (sg.state) {
    case GS_TITLE: title_update(); break;
    case GS_INTRO:
        if ((btnp(BTN_A) || btnp(BTN_START)) && sg.state_t > 20) {
            input_consume();
            sg.state_t = 0;
            if (++sg.intro_page >= SPUR_INTRO_PAGES) start_saloon();
        }
        break;
    case GS_FIELD: spur_field_update(); break;
    case GS_BATTLE: spur_battle_update(); break;
    case GS_ENDING: ending_update(); break;
    default: break;
    }
}

/* ---- tests ------------------------------------------------------------------------------- */
static int count_wisps(void) {
    int n = 0;
    for (int i = 0; i < SPUR_WISPS; i++) n += sg.wisp[i].alive;
    return n;
}

static int spur_query(const char *key, int *out) {
    const SpurBattle *b = &sg.b;
    if (!strcmp(key, "bot")) { *out = spur_bot_buttons(); return 1; }
    if (!strcmp(key, "bot_step")) { *out = spur_bot_step(); return 1; }
    if (!strcmp(key, "state")) { *out = sg.state; return 1; }
    if (!strcmp(key, "map")) { *out = sg.map_id; return 1; }
    if (!strcmp(key, "x")) { *out = sg.pos[0].x; return 1; }
    if (!strcmp(key, "y")) { *out = sg.pos[0].y; return 1; }
    if (!strncmp(key, "tx", 2) && key[2] >= '0' && key[2] <= '3') { *out = sg.pos[key[2] - '0'].x; return 1; }
    if (!strncmp(key, "ty", 2) && key[2] >= '0' && key[2] <= '3') { *out = sg.pos[key[2] - '0'].y; return 1; }
    if (!strcmp(key, "face")) { *out = sg.face; return 1; }
    if (!strcmp(key, "moving")) { *out = sg.moving; return 1; }
    if (!strcmp(key, "ui")) { *out = sg.ui.kind; return 1; }
    if (!strcmp(key, "ui_page")) { *out = sg.ui.page; return 1; }
    if (!strcmp(key, "menu_page")) { *out = sg.menu_page; return 1; }
    if (!strcmp(key, "mounted")) { *out = sg.mounted; return 1; }
    if (!strcmp(key, "horses")) { *out = sg.s.horses; return 1; }
    if (!strcmp(key, "posse")) { *out = sg.s.nposse; return 1; }
    if (!strcmp(key, "nsaved")) { *out = sg.nsaved; return 1; }
    if (!strncmp(key, "saved", 5)) { *out = sg.saved[iclamp(atoi(key + 5), 0, H_COUNT - 1)]; return 1; }
    if (!strncmp(key, "order", 5)) { *out = spur_hero_at(iclamp(atoi(key + 5), 0, 3)); return 1; }
    if (!strncmp(key, "lv", 2) && isdigit((unsigned char)key[2])) { *out = sg.s.hero[iclamp(atoi(key + 2), 0, H_COUNT - 1)].level; return 1; }
    if (!strncmp(key, "hp", 2) && isdigit((unsigned char)key[2])) { *out = sg.s.hero[iclamp(atoi(key + 2), 0, H_COUNT - 1)].hp; return 1; }
    if (!strncmp(key, "maxhp", 5)) { *out = spur_maxhp(iclamp(atoi(key + 5), 0, H_COUNT - 1)); return 1; }
    if (!strncmp(key, "sp", 2) && isdigit((unsigned char)key[2])) { *out = sg.s.hero[iclamp(atoi(key + 2), 0, H_COUNT - 1)].sp; return 1; }
    if (!strncmp(key, "xp", 2) && isdigit((unsigned char)key[2])) { *out = sg.s.hero[iclamp(atoi(key + 2), 0, H_COUNT - 1)].xp; return 1; }
    if (!strncmp(key, "status", 6)) { *out = sg.s.hero[iclamp(atoi(key + 6), 0, H_COUNT - 1)].status; return 1; }
    if (!strncmp(key, "atk", 3)) { *out = spur_attack_power(iclamp(atoi(key + 3), 0, H_COUNT - 1)); return 1; }
    if (!strncmp(key, "stat", 4)) {
        int h = 0, s = 0;
        if (sscanf(key + 4, "%d_%d", &h, &s) == 2) { *out = spur_stat(iclamp(h, 0, H_COUNT - 1), iclamp(s, 0, ST_COUNT - 1)); return 1; }
        return 0;
    }
    if (!strncmp(key, "passive", 7)) {
        int h = 0, p = 0;
        if (sscanf(key + 7, "%d_%d", &h, &p) == 2) { *out = spur_passive(iclamp(h, 0, H_COUNT - 1), iclamp(p, 0, PS_COUNT - 1)); return 1; }
        return 0;
    }
    if (!strncmp(key, "skill", 5)) {
        int h = 0, s = 0;
        if (sscanf(key + 5, "%d_%d", &h, &s) == 2) { *out = spur_has_skill(iclamp(h, 0, H_COUNT - 1), s); return 1; }
        return 0;
    }
    if (!strncmp(key, "equip", 5)) {
        int h = 0, s = 0;
        if (sscanf(key + 5, "%d_%d", &h, &s) == 2) { *out = sg.s.hero[iclamp(h, 0, H_COUNT - 1)].equip[iclamp(s, 0, 4)]; return 1; }
        return 0;
    }
    if (!strcmp(key, "teeth")) { *out = sg.s.teeth; return 1; }
    if (!strcmp(key, "bank")) { *out = sg.s.bank; return 1; }
    if (!strcmp(key, "bag")) { *out = spur_bag_count(); return 1; }
    if (!strncmp(key, "bagslot", 7)) { *out = sg.s.bag[iclamp(atoi(key + 7), 0, SPUR_BAG - 1)]; return 1; }
    if (!strncmp(key, "has", 3)) { *out = spur_bag_find(atoi(key + 3)) >= 0; return 1; }
    if (!strncmp(key, "count", 5)) { int it = atoi(key + 5), n = 0; for (int i = 0; i < SPUR_BAG; i++) n += sg.s.bag[i] == it; *out = n; return 1; }
    if (!strncmp(key, "key", 3) && isdigit((unsigned char)key[3])) { *out = spur_key(atoi(key + 3)); return 1; }
    if (!strncmp(key, "flag", 4)) { *out = spur_flag(atoi(key + 4)); return 1; }
    if (!strcmp(key, "chapel")) { *out = sg.s.chapel; return 1; }
    if (!strcmp(key, "stations")) { *out = sg.s.stations; return 1; }
    if (!strcmp(key, "battles")) { *out = sg.s.battles; return 1; }
    if (!strcmp(key, "deaths")) { *out = sg.s.deaths; return 1; }
    if (!strcmp(key, "steps")) { *out = sg.s.steps; return 1; }
    if (!strcmp(key, "alive")) { *out = spur_posse_alive(); return 1; }
    if (!strcmp(key, "wisps")) { *out = count_wisps(); return 1; }
    if (!strncmp(key, "wisp", 4) && isdigit((unsigned char)key[4])) {
        int i = iclamp(key[4] - '0', 0, SPUR_WISPS - 1);
        if (key[5] == 'x') *out = sg.wisp[i].alive ? sg.wisp[i].x : -1;
        else *out = sg.wisp[i].alive ? sg.wisp[i].y : -1;
        return 1;
    }
    if (!strcmp(key, "ward")) { *out = sg.ward_steps; return 1; }
    if (!strcmp(key, "save_valid")) { *out = spur_records.valid; return 1; }
    if (!strcmp(key, "save_map")) { *out = spur_records.map; return 1; }
    if (!strcmp(key, "save_wins")) { *out = spur_records.wins; return 1; }
    if (!strcmp(key, "save_cherries")) { *out = spur_records.cherries; return 1; }
    if (!strcmp(key, "save_teeth")) { *out = spur_records.teeth; return 1; }
    if (!strcmp(key, "save_level")) { *out = spur_records.max_level; return 1; }
    if (!strcmp(key, "cherry")) { *out = sg.cherry_run; return 1; }
    if (!strcmp(key, "demo")) { *out = sg.demo; return 1; }
    if (!strcmp(key, "tile")) { *out = spur_tile_at(&sg.map, spur_facing_x(), spur_facing_y()); return 1; }
    if (!strcmp(key, "tile_here")) { *out = spur_tile_at(&sg.map, sg.pos[0].x, sg.pos[0].y); return 1; }
    if (!strcmp(key, "npc_ahead")) { *out = spur_npc_at(spur_facing_x(), spur_facing_y()); return 1; }
    if (!strcmp(key, "art_bad")) { *out = spur_art_check(); return 1; }
    if (!strcmp(key, "layout")) { *out = spur_layout_audit(); return 1; }
    if (!strcmp(key, "maps_bad")) { int bad = 0; for (int i = 1; i < M_COUNT; i++) bad += spur_map_check(i); *out = bad; return 1; }
    if (!strncmp(key, "map_bad", 7)) { *out = spur_map_check(atoi(key + 7)); return 1; }
    if (!strncmp(key, "reach_", 6)) {
        /* reach_X_Y_M: can the map M's door be walked to from (X, Y) on the Territory */
        int x = 0, y = 0, m = 0;
        if (sscanf(key + 6, "%d_%d_%d", &x, &y, &m) != 3) return 0;
        *out = (int)((spur_world_reach(x, y) >> m) & 1);
        return 1;
    }
    if (!strcmp(key, "chests")) { *out = spur_chest_total(); return 1; }
    if (!strcmp(key, "zone")) { *out = sg.map.kind == MK_WORLD ? spur_world_zone(sg.map_id, sg.pos[0].x, sg.pos[0].y) : sg.map.zone; return 1; }
    /* battle */
    if (!strcmp(key, "b_phase")) { *out = b->phase; return 1; }
    if (!strcmp(key, "b_troop")) { *out = b->troop; return 1; }
    if (!strcmp(key, "b_foes")) { *out = spur_foes_left(); return 1; }
    if (!strcmp(key, "b_nfoe")) { *out = b->nfoe; return 1; }
    if (!strncmp(key, "b_foehp", 7)) { *out = b->foe[iclamp(atoi(key + 7), 0, SPUR_FOES - 1)].hp; return 1; }
    if (!strncmp(key, "b_foe", 5) && isdigit((unsigned char)key[5])) { *out = b->foe[iclamp(atoi(key + 5), 0, SPUR_FOES - 1)].e; return 1; }
    if (!strcmp(key, "b_cur")) { *out = b->cur; return 1; }
    if (!strcmp(key, "b_actor")) { *out = b->actor; return 1; }
    if (!strcmp(key, "b_nz")) { *out = b->nz; return 1; }
    if (!strncmp(key, "b_zres", 6)) { *out = b->z[iclamp(atoi(key + 6), 0, BAR_MAXZ - 1)].result; return 1; }
    if (!strcmp(key, "b_bar")) { *out = b->bar_x; return 1; }
    if (!strncmp(key, "b_za", 4)) { *out = b->z[iclamp(atoi(key + 4), 0, BAR_MAXZ - 1)].a; return 1; }
    if (!strncmp(key, "b_zr", 4)) { *out = b->z[iclamp(atoi(key + 4), 0, BAR_MAXZ - 1)].red; return 1; }
    if (!strncmp(key, "b_zb", 4)) { *out = b->z[iclamp(atoi(key + 4), 0, BAR_MAXZ - 1)].b; return 1; }
    if (!strcmp(key, "b_shots")) { *out = b->bar_shots; return 1; }
    if (!strcmp(key, "b_kind")) { *out = b->bar_kind; return 1; }
    if (!strcmp(key, "b_auto")) { *out = b->bar_auto; return 1; }
    if (!strcmp(key, "b_won")) { *out = b->won; return 1; }
    if (!strcmp(key, "b_wiped")) { *out = b->wiped; return 1; }
    if (!strcmp(key, "b_fled")) { *out = b->fled; return 1; }
    if (!strcmp(key, "b_xp")) { *out = b->xp_gain; return 1; }
    if (!strcmp(key, "b_teeth")) { *out = b->teeth_gain; return 1; }
    if (!strcmp(key, "b_queue0")) { *out = b->queue[0]; return 1; }
    if (!strncmp(key, "b_queue", 7)) { *out = b->queue[iclamp(atoi(key + 7), 0, 11)]; return 1; }
    if (!strcmp(key, "b_nqueue")) { *out = b->nqueue; return 1; }
    if (!strncmp(key, "b_stun", 6)) { *out = b->foe[iclamp(atoi(key + 6), 0, SPUR_FOES - 1)].stun; return 1; }
    if (!strncmp(key, "b_reload", 8)) { *out = sg.s.hero[iclamp(atoi(key + 8), 0, H_COUNT - 1)].reload; return 1; }
    if (!strcmp(key, "b_final")) { *out = b->final_kill; return 1; }
    if (!strncmp(key, "b_buffatk", 9)) { *out = b->buff[iclamp(atoi(key + 9), 0, 3)].atk; return 1; }
    if (!strncmp(key, "b_buffevd", 9)) { *out = b->buff[iclamp(atoi(key + 9), 0, 3)].evd; return 1; }
    if (!strncmp(key, "b_steady", 8)) { *out = b->buff[iclamp(atoi(key + 8), 0, 3)].steady; return 1; }
    if (!strcmp(key, "dump")) {
        /* the map on screen as letters, for looking at in a terminal */
        static const char CH[T_COUNT + 1] = "~=,;:.*TPc^MASrbFlab_Cmsdf"
                                            "HHHHCCCGXxCSSSR"
                                            "AXFVL"
                                            "_:#%RWDwk=obx<>E$Btr%oo|nA/zp_K::#|bP=_TH&LOU";
        for (int y = 0; y < sg.map.h; y++) {
            for (int x = 0; x < sg.map.w; x++) {
                int t = sg.map.tile[y][x];
                char c = t < (int)sizeof CH - 1 ? CH[t] : '?';
                if (x == sg.pos[0].x && y == sg.pos[0].y) c = '@';
                putchar(c);
            }
            putchar('\n');
        }
        *out = 0;
        return 1;
    }
    if (!strcmp(key, "xp_for10")) { *out = spur_xp_for(10); return 1; }
    if (!strcmp(key, "xp_for50")) { *out = spur_xp_for(50); return 1; }
    if (!strcmp(key, "ending")) { int n = 0; while (spur_ending_line(n)) n++; *out = n; return 1; }
    return 0;
}

static void give_posse(const char *s) {
    /* "posse 0 2 6 7": these strangers, in this order */
    reset_game();
    int n = 0, h;
    const char *p = s;
    while (n < 4 && sscanf(p, "%d", &h) == 1) {
        if (h >= 0 && h < H_COUNT) sg.saved[h] = 1, sg.s.order[n++] = (uint8_t)h;
        while (*p == ' ') p++;
        while (*p && *p != ' ') p++;
    }
    sg.nsaved = n;
    sg.s.nposse = (uint8_t)n;
    for (int i = 0; i < n; i++) sg.s.hero[sg.s.order[i]].in = 1;
    sg.s.teeth = 200;
    sg.s.chapel = M_GLADWELL;
    spur_set_flag(F_POSSE);
    sg.s.valid = 1;
    spur_set_state(GS_FIELD);
    game_set_pausable(true);
}

static int spur_cheat(const char *cmd) {
    int a, b, c;
    if (!strncmp(cmd, "posse", 5)) { give_posse(cmd + 5); spur_enter_map(M_GLADWELL, -1, 0, 1); return 1; }
    if (sscanf(cmd, "warp %d %d %d", &a, &b, &c) == 3) { sg.ui.kind = UI_NONE; spur_set_state(GS_FIELD); spur_enter_map(iclamp(a, 0, M_COUNT - 1), b, c, 0); return 1; }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) { spur_place_party(a, b, sg.face); return 1; }
    if (sscanf(cmd, "face %d", &a) == 1) { sg.face = a & 3; return 1; }
    if (sscanf(cmd, "level %d %d", &a, &b) == 2) {
        int h = iclamp(a, 0, H_COUNT - 1);
        sg.s.hero[h].level = 1;
        sg.s.hero[h].xp = spur_xp_for(iclamp(b, 1, 50));
        spur_level_up_check(h, NULL, 0);
        sg.s.hero[h].hp = (int16_t)spur_maxhp(h);
        sg.s.hero[h].sp = (int16_t)spur_maxsp(h);
        return 1;
    }
    if (sscanf(cmd, "xp %d %d", &a, &b) == 2) { sg.s.hero[iclamp(a, 0, H_COUNT - 1)].xp = b; return 1; }
    if (sscanf(cmd, "hp %d %d", &a, &b) == 2) { sg.s.hero[iclamp(a, 0, H_COUNT - 1)].hp = (int16_t)b; return 1; }
    if (sscanf(cmd, "sp %d %d", &a, &b) == 2) { sg.s.hero[iclamp(a, 0, H_COUNT - 1)].sp = (int16_t)b; return 1; }
    if (sscanf(cmd, "status %d %d", &a, &b) == 2) { sg.s.hero[iclamp(a, 0, H_COUNT - 1)].status = (uint8_t)b; return 1; }
    if (sscanf(cmd, "equip %d %d %d", &a, &b, &c) == 3) { sg.s.hero[iclamp(a, 0, H_COUNT - 1)].equip[iclamp(b, 0, 4)] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "learn %d %d", &a, &b) == 2) { spur_learn_skill(iclamp(a, 0, H_COUNT - 1), b); return 1; }
    if (sscanf(cmd, "give %d", &a) == 1) { spur_bag_add(a); return 1; }
    if (!strcmp(cmd, "emptybag")) { memset(sg.s.bag, 0, sizeof sg.s.bag); return 1; }
    if (sscanf(cmd, "key %d", &a) == 1) { spur_give_key(a); return 1; }
    if (sscanf(cmd, "flag %d", &a) == 1) { spur_set_flag(a); return 1; }
    if (sscanf(cmd, "unflag %d", &a) == 1) { spur_clear_flag(a); return 1; }
    if (sscanf(cmd, "teeth %d", &a) == 1) { sg.s.teeth = a; return 1; }
    if (sscanf(cmd, "bank %d", &a) == 1) { sg.s.bank = a; return 1; }
    if (sscanf(cmd, "wisp %d %d", &a, &b) == 2) { spur_spawn_wisp_at(a, b); return 1; }
    if (!strcmp(cmd, "nowisps")) { spur_clear_wisps(); return 1; }
    if (sscanf(cmd, "ward %d", &a) == 1) { sg.ward_steps = a; return 1; }
    if (sscanf(cmd, "battle %d", &a) == 1) { sg.ui.kind = UI_NONE; spur_battle_begin(iclamp(a, 1, SPUR_TROOPS - 1), -1); return 1; }
    if (sscanf(cmd, "foehp %d %d", &a, &b) == 2) { if (a >= 0 && a < SPUR_FOES) sg.b.foe[a].hp = b; return 1; }
    if (sscanf(cmd, "mount %d", &a) == 1) { spur_set_mount(a); return 1; }
    if (!strcmp(cmd, "god")) { sg.god = 1; return 1; }
    if (!strcmp(cmd, "mortal")) { sg.god = 0; return 1; }
    if (!strcmp(cmd, "save")) { spur_save_game(); return 1; }
    if (!strcmp(cmd, "refresh")) { spur_refresh_map(); return 1; }
    if (!strcmp(cmd, "title")) { to_title(); return 1; }
    if (!strcmp(cmd, "heal")) {
        for (int h = 0; h < H_COUNT; h++) { sg.s.hero[h].hp = (int16_t)spur_maxhp(h); sg.s.hero[h].sp = (int16_t)spur_maxsp(h); sg.s.hero[h].status = 0; }
        return 1;
    }
    if (sscanf(cmd, "botgoal %d", &a) == 1) { spur_bot_goal = a; return 1; }
    {
        int p[4], n = sscanf(cmd, "botpick %d %d %d %d", &p[0], &p[1], &p[2], &p[3]);
        if (n >= 1 && !strncmp(cmd, "botpick", 7)) { spur_bot_pick(p, n); return 1; }
    }
    if (sscanf(cmd, "botdebug %d", &a) == 1) { spur_bot_debug = a; return 1; }
    if (sscanf(cmd, "stations %d", &a) == 1) { sg.s.stations = (uint8_t)a; return 1; }
    if (sscanf(cmd, "chapel %d", &a) == 1) { sg.s.chapel = (uint8_t)a; return 1; }
    if (sscanf(cmd, "horses %d", &a) == 1) { sg.s.horses = (uint8_t)a; return 1; }
    return 0;
}

const GameDef GAME_HELLSPUR = {
    "hellspur",
    "HELLSPUR",
    "1988",
    "RPG",
    "AN ANGEL SAVED FOUR OF THE EIGHT. RIDE THROUGH HELL AND MAKE IT COUNT.",
    {"REACH LEVEL 10", "END THE GAME IN HELLSPUR", "WIN WITH ALL THE POSSE ALIVE"},
    GLYPH_DPAD "\tWALK\n"
    GLYPH_A "\tTALK, SEARCH, CHOOSE\n"
    GLYPH_B "\tTHE MENU, BACK\n"
    "BATTLE\t" GLYPH_A " ON YELLOW: HIT\n"
    "\t" GLYPH_A " ON RED: CRITICAL\n"
    "\tHOLD " GLYPH_RIGHT ": THE BAR RUNS FASTER\n"
    "\tHEAVY GUNS: HOLD " GLYPH_A "\n"
    "START\tPAUSE",
    C_MAROON, C_AMBER,
    spur_load, spur_start, spur_update, spur_draw, spur_quit, spur_draw_label, spur_query, spur_cheat,
    "GRIMSTONE", 40,
    NULL,
};
