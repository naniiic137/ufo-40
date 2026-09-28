/* DOT & DASH - locked in the lumber room with her dog, Dot shrinks into the
 * things around her to earn her way out.
 * Cartridge 45 of UFO 40, a tribute to Mini & Max (UFO 50 #45).
 * See docs/games/45-dot-and-dash.md. */
#include "dotdash.h"

DDSave dd_sv;
int dd_state, dd_state_t;
uint32_t dd_in, dd_in_prev;
bool dd_quiet;

#define DD_MAGIC 0x44443001u
#define START_CLOCK (20 * 60 + 5) /* 8:05 in the evening */

static int title_sel, bag_page, credits_y, ending_kind, ending_pending = -1, intro_page;
static bool has_save;

void dd_dialog_reset(void);
void dd_draw_world(void);
void dd_draw_hud(void);
void dd_draw_label(int x, int y, int w, int h, int t);
void dd_draw_bag(int page);
int dd_bag_count(void);
void dd_draw_title(int sel, bool can_continue, int t);
void dd_draw_ending(int kind, int t, int cy);

/* ------------------------------------------------------------------ */
/* saving                                                                */

static void fresh_game(void) {
    memset(&dd_sv, 0, sizeof dd_sv);
    dd_sv.magic = DD_MAGIC;
    dd_sv.clock_min = START_CLOCK;
    dd_sv.started = 1;
    memset(dd_stack, 0, sizeof dd_stack);
    dd_stack[0] = (LevelDesc){SC_SMALL, LV_AREA, AR_ROOM, 0, 0, 0, 0, 0};
    dd_depth = 0;
    dd_scale = SC_FULL;
    dd_p = (Body){0};
    dd_p.w = 8;
    dd_p.h = 24;
    dd_p.x = 26 * DD_TS0;
    dd_p.y = 83 * DD_TS0 - 24;
    dd_p.facing = 1;
    dd_sv.full = 1; /* start at full size */
}

static bool load_game(void) {
    DDSave tmp;
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n != (int)sizeof tmp || tmp.magic != DD_MAGIC) return false;
    dd_sv = tmp;
    return true;
}

bool dd_collected(uint32_t pid) {
    for (int i = 0; i < dd_sv.n_collect; i++)
        if (dd_sv.collect[i] == pid) return true;
    return false;
}
void dd_collect(uint32_t pid) {
    if (!pid || dd_collected(pid)) return;
    if (dd_sv.n_collect < DD_COLLECT_MAX) dd_sv.collect[dd_sv.n_collect++] = pid;
}

/* ------------------------------------------------------------------ */
/* goals: 100 glints, the escape, balance restored                       */

void dd_check_goals(void) {
    if (dd_sv.glints_total >= 100) game_award(GOAL_BEACON);
    if (dd_flag(FL_ESCAPED)) game_award(GOAL_SAUCER);
    if (dd_flag(FL_TRUE_END)) game_award(GOAL_ALIEN);
}

/* ------------------------------------------------------------------ */
/* starting and resuming                                                  */

static void enter_play(void) {
    dd_reset_play_state();
    dd_dialog_reset();
    dd_depth = dd_sv.depth;
    memcpy(dd_stack, dd_sv.stack, sizeof dd_stack);
    dd_world_init();
    dd_scale = dd_sv.full && dd_depth == 0 ? SC_FULL : dd_stack[dd_depth].scale;
    dd_hp_max = dd_hp_max_now();
    dd_hp = dd_hp_max;
    dd_lamp_dark = dd_flag(FL_LAMP_OFF);
    dd_play_enter_level(0);
    dd_restore_carry_after_load();
    dd_spawn_dash_now();
    dd_state = ST_PLAY;
    dd_state_t = 0;
    game_set_pausable(true);
    dd_check_goals();
}

static void new_game(void) {
    fresh_game();
    dd_sv.px = (int16_t)dd_p.x;
    dd_sv.py = (int16_t)dd_p.y;
    memcpy(dd_sv.stack, dd_stack, sizeof dd_stack);
    dd_sv.depth = 0;
    dd_state = ST_INTRO;
    dd_state_t = 0;
    intro_page = 0;
    music_play(DD_MUS[MU_TITLE]);
}

/* Every session begins in the room at full size: the game keeps what Dot
 * has found, never where she was (or what was in her hands). */
static void resume(void) {
    memset(dd_sv.stack, 0, sizeof dd_sv.stack);
    dd_sv.stack[0] = (LevelDesc){SC_SMALL, LV_AREA, AR_ROOM, 0, 0, 0, 0, 0};
    dd_sv.depth = 0;
    dd_sv.full = 1;
    dd_sv.carry = 0;
    dd_sv.dash_away = 0;
    dd_p = (Body){0};
    dd_p.x = 26 * DD_TS0;
    dd_p.y = 83 * DD_TS0 - 24;
    dd_p.facing = 1;
    enter_play();
}

void dd_start_ending(bool true_end) { ending_pending = true_end ? 1 : 0; }

static void begin_ending(int kind) {
    ending_kind = kind;
    dd_set(FL_ESCAPED);
    if (kind) dd_set(FL_TRUE_END);
    dd_set(FL_SEEN_END);
    dd_check_goals();
    dd_save_now();
    dd_state = ST_ENDING;
    dd_state_t = 0;
    credits_y = 0;
    game_set_pausable(false);
    music_play(DD_MUS[kind ? MU_TRUE : MU_ENDING]);
}

static void after_credits(void) {
    /* another party, and Dot is locked in again: the room is still there */
    while (dd_depth > 0) dd_pop_level();
    LevelDesc room = {SC_SMALL, LV_AREA, AR_ROOM, 0, 0, 0, 0, 0};
    dd_stack[0] = room;
    dd_depth = 0;
    dd_sv.depth = 0;
    memcpy(dd_sv.stack, dd_stack, sizeof dd_stack);
    dd_sv.full = 1;
    dd_sv.px = 26 * DD_TS0;
    dd_sv.py = 83 * DD_TS0 - 24;
    dd_sv.carry = 0;
    dd_save_now();
    resume();
    dd_say("DASH", ending_kind ? "THAT WAS THE BEST PARTY EVER, DOT. ...WAIT, WHY ARE WE BACK IN HERE?" :
                                 "ANOTHER PARTY, AND OTTO'S LOCKED US IN AGAIN. WELL. WE KNOW THE WAY NOW, DON'T WE?");
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                    */

static void dd_load(void) {
    dd_art_load();
    dd_audio_load();
}

static void dd_start(void) {
    has_save = load_game();
    if (!has_save) fresh_game();
    dd_state = ST_TITLE;
    dd_state_t = 0;
    title_sel = 0;
    ending_pending = -1;
    dd_dialog_reset();
    game_set_pausable(false);
    music_play(DD_MUS[MU_TITLE]);
    if (has_save) dd_check_goals();
}

static void dd_quit(void) {
    if (dd_state == ST_PLAY || dd_state == ST_BAG) dd_save_now();
}

static const char *const INTRO[] = {
    "EIGHT O'CLOCK. MUM AND DAD HAVE GONE OUT TO THE THEATRE, AND BIG BROTHER OTTO IS THROWING A PARTY.",
    "HE DOESN'T WANT HIS LITTLE SISTER DOT, OR HER DOG DASH, UNDER EVERYONE'S FEET. SO HE LOCKS THEM IN THE LUMBER ROOM.",
    "\"DON'T CRY, DOT,\" SAYS DASH, WHO HAS ALWAYS BEEN ABLE TO TALK WHEN NOBODY ELSE IS LISTENING. \"LOOK AT THE ROOM FROM DOWN LOW. HOLD DOWN AND GET SMALL. I'LL COME TOO.\"",
};

static void dd_update(void) {
    dd_in_prev = dd_in;
    dd_in = input_held();
    dd_state_t++;
    switch (dd_state) {
    case ST_TITLE:
        if (btnp(BTN_B) && dd_state_t > 10) { game_exit_to_library(); return; }
        if (has_save && (btnp(BTN_UP) || btnp(BTN_DOWN))) { title_sel ^= 1; sfx_play_name("ui_move"); }
        if ((btnp(BTN_A) || btnp(BTN_START)) && dd_state_t > 20) {
            sfx_play_name("ui_ok");
            if (has_save && title_sel == 0) { music_stop(); resume(); }
            else new_game();
        }
        break;
    case ST_INTRO:
        if ((btnp(BTN_A) || btnp(BTN_START)) && dd_state_t > 15) {
            intro_page++;
            dd_state_t = 0;
            if (intro_page >= ARRAY_LEN(INTRO)) {
                dd_set(FL_INTRO);
                enter_play();
                dd_save_now();
            }
        }
        break;
    case ST_PLAY:
        /* at full size, UP goes up to the item strip at the top of the screen */
        if (dd_scale == SC_FULL && btnp(BTN_UP) && !dd_dialog_active() && !dd_trans && dd_upgrade_count() > 0) {
            dd_state = ST_BAG;
            bag_page = 0;
            sfx_play_name("ui_pause");
            break;
        }
        dd_play_update();
        if (ending_pending >= 0 && !dd_dialog_active() && !dd_trans) {
            int k = ending_pending;
            ending_pending = -1;
            begin_ending(k);
        }
        break;
    case ST_BAG: {
        /* bag_page is the cursor over the owned things */
        int n = dd_bag_count();
        if (btnp(BTN_B) || btnp(BTN_A) || btnp(BTN_DOWN)) { dd_state = ST_PLAY; sfx_play_name("ui_back"); }
        if (btn_repeat(BTN_LEFT) && n) { bag_page = (bag_page + n - 1) % n; sfx_play_name("ui_move"); }
        if (btn_repeat(BTN_RIGHT) && n) { bag_page = (bag_page + 1) % n; sfx_play_name("ui_move"); }
        break;
    }
    case ST_ENDING:
        if (dd_state_t > 60) credits_y++;
        if ((btnp(BTN_A) || btnp(BTN_START)) && dd_state_t > 240) { dd_state = ST_CREDITS; dd_state_t = 0; }
        if (credits_y > 1400) { dd_state = ST_CREDITS; dd_state_t = 0; }
        break;
    case ST_CREDITS:
        if ((btnp(BTN_A) || btnp(BTN_START)) && dd_state_t > 30) after_credits();
        break;
    }
}

static void dd_draw(void) {
    switch (dd_state) {
    case ST_TITLE: dd_draw_title(title_sel, has_save, dd_state_t); break;
    case ST_INTRO: {
        gfx_cls(C_INK);
        int t = dd_state_t;
        for (int k = 0; k < 40; k++) gfx_pset((k * 83 + t / 3) % SCREEN_W, (k * 47) % 110 + 10, k % 3 ? C_DUSK : C_SLATE);
        spr_draw_scaled(&dd_spr[S_DOT_STAND], 120, 120, 3, 0);
        spr_draw_scaled(&dd_spr[S_DASH_SIT], 160, 135, 3, 0);
        ui_panel(16, 18, 288, 74, C_NIGHT, C_CREAM);
        const char *s = INTRO[imin(intro_page, ARRAY_LEN(INTRO) - 1)];
        char buf[256];
        int n = imin((int)strlen(s), t * 2);
        memcpy(buf, s, (size_t)n);
        buf[n] = 0;
        text_wrap(buf, 26, 28, 268, C_WHITE, 10);
        if (t > 30 && (t / 20) % 2) text_draw(GLYPH_A, 290, 80, C_CREAM);
        break;
    }
    case ST_PLAY:
    case ST_BAG:
        dd_draw_world();
        dd_draw_hud();
        dd_dialog_draw();
        if (dd_state == ST_BAG) dd_draw_bag(bag_page);
        break;
    case ST_ENDING:
    case ST_CREDITS:
        dd_draw_ending(ending_kind, dd_state == ST_CREDITS ? 100000 : dd_state_t, dd_state == ST_CREDITS ? -1 : credits_y);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* test hooks                                                              */

static int count_ents(int kind, int sub) {
    int n = 0;
    for (int i = 0; i < DD_MAX_ENTS; i++)
        if (dd_ent[i].alive && dd_ent[i].kind == kind && (sub < 0 || dd_ent[i].sub == sub)) n++;
    return n;
}

static const char *const FLAG_NAME[FL_COUNT] = {
    "intro", "met_granny", "specs_given", "tonic_given", "mitt_sold", "dash_clean", "puff_clean", "puff_paid",
    "bluedust_done", "inky_done", "snarl_done", "scroll_done", "germ2_done", "pell_freed", "pell_guarded",
    "silk_done", "egg_done", "twig_done", "crate_done", "letter1", "letter2", "letter_done", "smith_done",
    "gear_done", "outpost", "rotifer_asked", "rotifer_dead", "rotifer_paid", "water_given", "water_paid",
    "tablet_given", "oldcap_done", "blueeye_done", "morel_done", "seed_done", "redegg_done", "cowpoke_done",
    "weepy_done", "shrew_free", "shrew_paid", "mage_beaten", "power_off", "earwig_dead", "wings_done",
    "lumen", "lamp_off", "filament_asked", "bigbang_done", "throne_open", "catfood_done", "met_queen",
    "paid_queen", "sprocket_dead", "escaped", "met_nib", "balance", "true_end", "siege_dead", "shrine",
    "worm_friends", "puff_met", "tufty_asked", "bubbles_asked", "sticky_asked", "glue_done", "hopper_seen",
    "volt_met", "knight_met", "filament_paid", "tock_met", "grate_open", "grass_seen", "seen_end",
};
static const char *const UP_ID[U_ABILITIES] = {
    "mitt1", "mitt2", "tonic1", "tonic2", "bangle", "satchel1", "satchel2", "bean1", "bean2", "feather", "clogs1",
    "clogs2", "musk", "buzz1", "buzz2", "fizz", "top1", "top2", "whistle", "stew", "shell", "plate", "wings",
};
static int flag_id(const char *n) {
    for (int i = 0; i < FL_COUNT; i++) if (!strcmp(FLAG_NAME[i], n)) return i;
    return -1;
}
static int up_id(const char *n) {
    for (int i = 0; i < U_ABILITIES; i++) if (!strcmp(UP_ID[i], n)) return i;
    return -1;
}

static int mark_hash = -1, mark_ents = -1;
static int level_hash_now(void) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < dd_lv.w * dd_lv.h; i++) h = (h ^ dd_lv.t[i]) * 16777619u;
    return (int)(h & 0x7FFFFFFF);
}
static int ents_now(void) { int n = 0; for (int i = 0; i < DD_MAX_ENTS; i++) n += dd_ent[i].alive; return n; }

static int dd_query(const char *key, int *out) {
    int a;
    if (!strcmp(key, "bot")) { *out = (int)dd_bot_buttons(); return 1; }
    if (!strcmp(key, "state")) { *out = dd_state; return 1; }
    if (!strcmp(key, "scale")) { *out = dd_scale; return 1; }
    if (!strcmp(key, "depth")) { *out = dd_depth; return 1; }
    if (!strcmp(key, "px")) { *out = (int)dd_p.x; return 1; }
    if (!strcmp(key, "py")) { *out = (int)dd_p.y; return 1; }
    if (!strcmp(key, "tx")) { *out = (int)((dd_p.x + dd_p.w / 2) / (dd_scale == SC_FULL ? DD_TS0 : DD_TS)); return 1; }
    if (!strcmp(key, "ty")) { *out = (int)((dd_p.y + dd_p.h - 1) / (dd_scale == SC_FULL ? DD_TS0 : DD_TS)); return 1; }
    if (!strcmp(key, "vx10")) { *out = (int)(dd_p.vx * 10); return 1; }
    if (!strcmp(key, "vy10")) { *out = (int)(dd_p.vy * 10); return 1; }
    if (!strcmp(key, "ground")) { *out = dd_p.ground; return 1; }
    if (!strcmp(key, "sprint")) { *out = dd_p.sprint; return 1; }
    if (!strcmp(key, "peak")) { *out = dd_p.peak_y; return 1; }
    if (!strcmp(key, "landed")) { *out = dd_p.landed_fall; return 1; }
    if (!strcmp(key, "hp")) { *out = dd_hp; return 1; }
    if (!strcmp(key, "hpmax")) { *out = dd_hp_max; return 1; }
    if (!strcmp(key, "pep")) { *out = dd_pep(); return 1; }
    if (!strcmp(key, "glints")) { *out = dd_sv.glints; return 1; }
    if (!strcmp(key, "glints_total")) { *out = dd_sv.glints_total; return 1; }
    if (!strcmp(key, "bigs")) { *out = dd_sv.bigs_found; return 1; }
    if (!strcmp(key, "clock")) { *out = dd_sv.clock_min; return 1; }
    if (!strcmp(key, "upgrades")) { *out = dd_upgrade_count(); return 1; }
    if (!strcmp(key, "ups")) { *out = (int)dd_sv.ups; return 1; }
    if (!strcmp(key, "hearts_got")) { *out = dd_sv.hearts_got; return 1; }
    if (!strcmp(key, "eggs_got")) { *out = dd_sv.eggs_got; return 1; }
    if (!strcmp(key, "carry")) { *out = dd_carry >= 0 ? dd_ent[dd_carry].sub : -1; return 1; }
    if (!strcmp(key, "carry_kind")) { *out = dd_carry >= 0 ? dd_ent[dd_carry].kind : -1; return 1; }
    if (!strcmp(key, "carry_param")) { *out = dd_carry >= 0 ? dd_ent[dd_carry].param : -1; return 1; }
    if (!strcmp(key, "pocket")) { *out = dd_sv.carry ? dd_sv.carry - 1 : -1; return 1; }
    if (!strcmp(key, "satchel0")) { *out = dd_sv.satchel[0] ? dd_sv.satchel[0] - 1 : -1; return 1; }
    if (!strcmp(key, "satchel1")) { *out = dd_sv.satchel[1] ? dd_sv.satchel[1] - 1 : -1; return 1; }
    if (!strcmp(key, "dash")) { *out = dd_dash >= 0; return 1; }
    if (!strcmp(key, "dash_x")) { *out = dd_dash >= 0 ? (int)dd_ent[dd_dash].x : -1; return 1; }
    if (!strcmp(key, "dash_y")) { *out = dd_dash >= 0 ? (int)dd_ent[dd_dash].y : -1; return 1; }
    if (!strcmp(key, "dash_dist")) { *out = dd_dash >= 0 ? (int)fabsf(dd_ent[dd_dash].x - dd_p.x) : -1; return 1; }
    if (!strcmp(key, "dash_state")) { *out = dd_dash >= 0 ? dd_ent[dd_dash].state : -1; return 1; }
    if (!strcmp(key, "dash_away")) { *out = dd_sv.dash_away; return 1; }
    if (!strcmp(key, "sniff")) { *out = dd_sniff_x >= 0; return 1; }
    if (!strcmp(key, "riding")) { *out = dd_riding() >= 0 ? dd_ent[dd_riding()].kind * 100 + dd_ent[dd_riding()].sub : -1; return 1; }
    if (!strcmp(key, "level_kind")) { *out = dd_lv.d.kind; return 1; }
    if (!strcmp(key, "level_id")) { *out = dd_lv.d.id; return 1; }
    if (!strcmp(key, "level_row")) { *out = dd_lv.d.row; return 1; }
    if (!strcmp(key, "level_x0")) { *out = dd_lv.d.x0; return 1; }
    if (!strcmp(key, "level_n")) { *out = dd_lv.d.n; return 1; }
    if (!strcmp(key, "level_w")) { *out = dd_lv.w; return 1; }
    if (!strcmp(key, "chunk")) { *out = dd_lv.d.x0 + dd_chunk_of((int)dd_p.x); return 1; }
    if (!strcmp(key, "biome")) { *out = dd_lv.d.kind == LV_STRIP ? dd_lv.biome[dd_chunk_of((int)dd_p.x)] : -1; return 1; }
    if (!strcmp(key, "town")) { *out = dd_town_at(&dd_lv, (int)(dd_p.x / DD_TS)); return 1; }
    if (!strcmp(key, "dialog")) { *out = dd_dialog_active(); return 1; }
    if (!strcmp(key, "bag")) { *out = dd_state == ST_BAG ? bag_page : -1; return 1; }
    if (!strcmp(key, "asking")) { extern bool dd_dialog_asking(void); *out = dd_dialog_asking(); return 1; }
    if (!strcmp(key, "trans")) { *out = dd_trans; return 1; }
    if (!strcmp(key, "dead")) { *out = dd_dead_t; return 1; }
    if (!strcmp(key, "hurt")) { *out = dd_hurt_t; return 1; }
    if (!strcmp(key, "shrink_t")) { *out = dd_shrink_t; return 1; }
    if (!strcmp(key, "grow_t")) { *out = dd_grow_t; return 1; }
    if (!strcmp(key, "freeze")) { *out = dd_freeze_t(); return 1; }
    if (!strcmp(key, "lamp_dark")) { *out = dd_lamp_dark; return 1; }
    if (!strcmp(key, "has_save")) { *out = game_save_raw_size(game_current_index()) > 0; return 1; }
    if (!strcmp(key, "bot_state")) { *out = dd_bot_state; return 1; }
    if (!strcmp(key, "bot_plan")) { *out = dd_bot_plan_len(); return 1; }
    if (!strcmp(key, "moves_here")) { extern int dd_bot_moves_here(void); *out = dd_bot_moves_here(); return 1; }
    if (!strcmp(key, "towns")) { *out = dd_town_count(); return 1; }
    if (!strcmp(key, "collected")) { *out = dd_sv.n_collect; return 1; }
    if (!strncmp(key, "flag.", 5)) { int f = flag_id(key + 5); if (f < 0) return 0; *out = dd_flag(f); return 1; }
    if (!strncmp(key, "up.", 3)) { int u = up_id(key + 3); if (u < 0) return 0; *out = dd_has(u); return 1; }
    if (sscanf(key, "flag_%d", &a) == 1) { *out = dd_flag(a); return 1; }
    if (sscanf(key, "up_%d", &a) == 1) { *out = a < U_ABILITIES ? dd_has(a) : a < U_EGG0 ? (dd_sv.hearts_got >> (a - U_HEART0)) & 1 : (dd_sv.eggs_got >> (a - U_EGG0)) & 1; return 1; }
    if (sscanf(key, "qc_%d", &a) == 1) { *out = dd_sv.counts[iclamp(a, 0, 15)]; return 1; }
    if (sscanf(key, "foes_%d", &a) == 1) { *out = count_ents(EK_FOE, a); return 1; }
    if (!strcmp(key, "foes")) { *out = count_ents(EK_FOE, -1); return 1; }
    if (sscanf(key, "objs_%d", &a) == 1) { *out = count_ents(EK_OBJ, a); return 1; }
    if (sscanf(key, "picks_%d", &a) == 1) { *out = count_ents(EK_PICK, a); return 1; }
    if (sscanf(key, "npcs_%d", &a) == 1) { *out = count_ents(EK_NPC, a); return 1; }
    if (sscanf(key, "foehp_%d", &a) == 1) {
        *out = -1;
        for (int i = 0; i < DD_MAX_ENTS; i++) if (dd_ent[i].alive && dd_ent[i].kind == EK_FOE && dd_ent[i].sub == a) { *out = dd_ent[i].hp; break; }
        return 1;
    }
    if (sscanf(key, "foey_%d", &a) == 1) {
        *out = -1;
        for (int i = 0; i < DD_MAX_ENTS; i++) if (dd_ent[i].alive && dd_ent[i].kind == EK_FOE && dd_ent[i].sub == a) { *out = (int)dd_ent[i].y; break; }
        return 1;
    }
    if (sscanf(key, "foex_%d", &a) == 1) {
        *out = -1;
        for (int i = 0; i < DD_MAX_ENTS; i++) if (dd_ent[i].alive && dd_ent[i].kind == EK_FOE && dd_ent[i].sub == a) { *out = (int)dd_ent[i].x; break; }
        return 1;
    }
    if (sscanf(key, "foestun_%d", &a) == 1) {
        *out = -1;
        for (int i = 0; i < DD_MAX_ENTS; i++) if (dd_ent[i].alive && dd_ent[i].kind == EK_FOE && dd_ent[i].sub == a) { *out = dd_ent[i].stun; break; }
        return 1;
    }
    if (sscanf(key, "foepoison_%d", &a) == 1) {
        *out = 0;
        for (int i = 0; i < DD_MAX_ENTS; i++) if (dd_ent[i].alive && dd_ent[i].kind == EK_FOE && dd_ent[i].sub == a && dd_ent[i].poison) (*out)++;
        return 1;
    }
    if (sscanf(key, "foestate_%d", &a) == 1) {
        *out = -1;
        for (int i = 0; i < DD_MAX_ENTS; i++) if (dd_ent[i].alive && dd_ent[i].kind == EK_FOE && dd_ent[i].sub == a) { *out = dd_ent[i].state; break; }
        return 1;
    }
    if (!strncmp(key, "tile_", 5)) {
        int x, y;
        if (sscanf(key + 5, "%d_%d", &x, &y) == 2) { *out = lv_tile(&dd_lv, x, y); return 1; }
    }
    if (!strncmp(key, "place_x.", 8) || !strncmp(key, "place_y.", 8)) {
        int tx, ty;
        *out = dd_find_place(key + 8, &tx, &ty) ? (key[6] == 'x' ? tx : ty) : -1;
        return 1;
    }
    if (!strncmp(key, "reach.", 6)) {
        /* reach.X_Y : can the demo player walk from here to that tile */
        int x, y;
        if (sscanf(key + 6, "%d_%d", &x, &y) == 2) {
            int fx = (int)((dd_p.x + dd_p.w / 2) / DD_TS), fy = (int)((dd_p.y + dd_p.h - 1) / DD_TS);
            *out = dd_bot_can_reach(fx, fy, x, y);
            return 1;
        }
    }
    if (!strcmp(key, "level_hash")) { *out = level_hash_now(); return 1; }
    if (!strcmp(key, "ents")) { *out = ents_now(); return 1; }
    if (!strcmp(key, "same_as_mark")) { *out = level_hash_now() == mark_hash; return 1; }
    if (!strcmp(key, "same_ents_as_mark")) { *out = ents_now() == mark_ents; return 1; }
    if (!strcmp(key, "strip_errors")) {
        /* every chunk of this strip: the ground is continuous and no cave traps you */
        int bad = 0;
        if (dd_lv.d.kind == LV_STRIP)
            for (int x = 1; x < dd_lv.w - 2; x++) {
                int a2 = dd_lv.surf[x], b2 = dd_lv.surf[x + 1];
                if (b2 < a2 - 3) bad++;
            }
        *out = bad;
        return 1;
    }
    return 0;
}

void dd_bot_goto_place(const char *name);

static int dd_cheat(const char *cmd) {
    int a, b, c, d;
    char s[64];
    if (sscanf(cmd, "glints %d", &a) == 1) { dd_sv.glints = a; if (dd_sv.glints_total < a) dd_sv.glints_total = a; return 1; }
    if (sscanf(cmd, "give %d", &a) == 1) { dd_give_upgrade(a); if (dd_dialog_active()) dd_dialog_reset(); return 1; }
    if (sscanf(cmd, "flag %d", &a) == 1) { dd_set(a); return 1; }
    if (sscanf(cmd, "set %63s", s) == 1 && flag_id(s) >= 0) { dd_set(flag_id(s)); return 1; }
    if (sscanf(cmd, "up %63s", s) == 1 && up_id(s) >= 0) { dd_give_upgrade(up_id(s)); dd_dialog_reset(); return 1; }
    if (sscanf(cmd, "unflag %d", &a) == 1) { dd_sv.flags[a >> 3] &= (uint8_t)~(1u << (a & 7)); return 1; }
    if (sscanf(cmd, "hp %d", &a) == 1) { dd_hp = a; return 1; }
    if (sscanf(cmd, "hearts %d", &a) == 1) { dd_sv.hearts_got = (uint8_t)a; dd_hp_max = dd_hp_max_now(); dd_hp = dd_hp_max; return 1; }
    if (sscanf(cmd, "qc %d %d", &a, &b) == 2) { dd_sv.counts[a] = (uint8_t)b; return 1; }
    if (sscanf(cmd, "clock %d", &a) == 1) { dd_sv.clock_min = a; return 1; }
    if (!strcmp(cmd, "no_foes")) { dd_no_foes = true; for (int i = 0; i < DD_MAX_ENTS; i++) if (dd_ent[i].kind == EK_FOE && dd_ent[i].sub < F_FIRST_BOSS) dd_ent[i].alive = 0; return 1; }
    if (!strcmp(cmd, "foes_on")) { dd_no_foes = false; return 1; }
    if (!strcmp(cmd, "god")) { dd_god = !dd_god; return 1; }
    if (!strcmp(cmd, "quiet")) { dd_quiet = true; return 1; }
    if (!strcmp(cmd, "save")) { dd_save_now(); return 1; }
    if (!strcmp(cmd, "skip_dialog")) { dd_dialog_reset(); return 1; }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) { dd_p.x = (float)a; dd_p.y = (float)b; dd_p.vx = dd_p.vy = 0; return 1; }
    if (sscanf(cmd, "tile %d %d", &a, &b) == 2) {
        /* stand Dot in tile a,b (on the ground below it) */
        int ts = dd_scale == SC_FULL ? DD_TS0 : DD_TS;
        dd_p.x = (float)(a * ts + ts / 2) - dd_p.w / 2;
        dd_p.y = (float)((b + 1) * ts) - dd_p.h;
        dd_p.vx = dd_p.vy = 0;
        return 1;
    }
    if (sscanf(cmd, "area %d %d %d", &a, &b, &c) == 3) {
        if (dd_scale == SC_FULL) dd_scale = SC_SMALL;
        while (dd_depth > 0) dd_pop_level();
        dd_goto_area(a, b, c);
        return 1;
    }
    if (sscanf(cmd, "micro %d %d %d %d", &a, &b, &c, &d) == 4) {
        /* micro AREA ROW X SUBX : go straight into the micro strip over that tile */
        if (dd_scale == SC_FULL) dd_scale = SC_SMALL;
        while (dd_depth > 0) dd_pop_level();
        if (dd_lv.d.id != a) dd_goto_area(a, c, b - 1);
        dd_p.x = (float)(c * DD_TS + d) - dd_p.w / 2;
        dd_p.y = (float)(b * DD_TS) - dd_p.h;
        dd_p.ground = 1;
        if (!dd_has(U_TONIC1)) dd_sv.ups |= 1u << U_TONIC1;
        dd_change_scale(1);
        while (dd_trans > 0) dd_play_update();
        return 1;
    }
    if (!strcmp(cmd, "shrink")) { dd_p.ground = 1; dd_change_scale(1); while (dd_trans > 0) dd_play_update(); return 1; }
    if (!strcmp(cmd, "grow")) { dd_change_scale(-1); while (dd_trans > 0) dd_play_update(); return 1; }
    if (!strcmp(cmd, "full")) {
        while (dd_scale != SC_FULL) { dd_change_scale(-1); while (dd_trans > 0) dd_play_update(); }
        return 1;
    }
    if (sscanf(cmd, "obj %d", &a) == 1 && !strchr(cmd + 4, ' ')) {
        /* put a thing in Dot's hands */
        int j = dd_add_obj(a, dd_p.x, dd_p.y - 8);
        if (j >= 0) {
            if (dd_carry >= 0) dd_ent[dd_carry].alive = 0;
            dd_ent[j].held = 1;
            dd_ent[j].state = 1;
            dd_carry = j;
        }
        return 1;
    }
    if (sscanf(cmd, "objparam %d", &a) == 1) { if (dd_carry >= 0) dd_ent[dd_carry].param = a; return 1; }
    if (sscanf(cmd, "drop_obj %d %d %d", &a, &b, &c) == 3) { dd_add_obj(a, (float)b, (float)c); return 1; }
    if (sscanf(cmd, "foe %d %d %d", &a, &b, &c) == 3) {
        int j = dd_add_ent(EK_FOE, a, (float)b, (float)c);
        if (j >= 0) { dd_ent[j].home_x = (int16_t)b; dd_ent[j].home_y = (int16_t)c; }
        return 1;
    }
    if (sscanf(cmd, "foe_under %d", &a) == 1) {
        /* a creature in the air with Dot standing on it, 24 px up */
        int w, h;
        dd_foe_size(a, &w, &h);
        float fx = dd_p.x + dd_p.w / 2 - (float)w / 2, fy = dd_p.y + dd_p.h - 24;
        int j = dd_add_ent(EK_FOE, a, fx, fy);
        if (j >= 0) { dd_ent[j].home_x = (int16_t)fx; dd_ent[j].home_y = (int16_t)fy; dd_ent[j].pad = 1; }
        dd_p.y = fy - dd_p.h;
        dd_p.vy = 0;
        return 1;
    }
    if (sscanf(cmd, "hold_foe %d", &a) == 1) {
        int j = dd_add_ent(EK_FOE, a, dd_p.x, dd_p.y - 8);
        if (j >= 0) { if (dd_carry >= 0) dd_ent[dd_carry].alive = 0; dd_ent[j].held = 1; dd_carry = j; }
        return 1;
    }
    if (!strcmp(cmd, "mark")) { mark_hash = level_hash_now(); mark_ents = ents_now(); return 1; }
    if (!strcmp(cmd, "nodash")) { if (dd_dash >= 0) dd_ent[dd_dash].alive = 0; dd_dash = -1; dd_sv.dash_away = 1; return 1; }
    if (!strcmp(cmd, "dash")) { if (dd_dash >= 0) dd_ent[dd_dash].alive = 0; dd_dash = -1; dd_spawn_dash_now(); return 1; }
    if (!strcmp(cmd, "kill_boss")) {
        for (int i = 0; i < DD_MAX_ENTS; i++)
            if (dd_ent[i].alive && dd_ent[i].kind == EK_FOE && dd_ent[i].sub >= F_FIRST_BOSS && dd_ent[i].sub != F_SPROCKET) { dd_kill_foe(i); break; }
        return 1;
    }
    if (sscanf(cmd, "bot_tile %d %d", &a, &b) == 2) { dd_bot_goto(a, b); return 1; }
    if (sscanf(cmd, "bot_to %63s", s) == 1) { dd_bot_goto_place(s); return 1; }
    if (!strcmp(cmd, "bot_clear")) { dd_bot_clear(); return 1; }
    if (!strcmp(cmd, "bot_talk")) { extern void dd_bot_talk(void); dd_bot_talk(); return 1; }
    if (sscanf(cmd, "bot_hunt %d", &a) == 1) { extern void dd_bot_hunt(int sub); dd_bot_hunt(a); return 1; }
    if (sscanf(cmd, "dash_at %d %d", &a, &b) == 2) { if (dd_dash < 0) dd_spawn_dash_now(); if (dd_dash >= 0) { dd_ent[dd_dash].x = (float)a; dd_ent[dd_dash].y = (float)b; dd_ent[dd_dash].state = 0; } return 1; }
    if (!strcmp(cmd, "title")) { dd_start(); return 1; }
    if (!strcmp(cmd, "newgame")) { fresh_game(); dd_sv.px = (int16_t)dd_p.x; dd_sv.py = (int16_t)dd_p.y; memcpy(dd_sv.stack, dd_stack, sizeof dd_stack); dd_set(FL_INTRO); enter_play(); return 1; }
    if (!strcmp(cmd, "ending")) { begin_ending(0); return 1; }
    if (!strcmp(cmd, "ending_true")) { begin_ending(1); return 1; }
    return 0;
}

const GameDef GAME_DOTDASH = {
    "dotdash",
    "DOT & DASH",
    "1989",
    "ADVENTURE",
    "LOCKED IN THE LUMBER ROOM WITH HER DOG, DOT SHRINKS INTO THE THINGS AROUND HER TO BUY HER WAY OUT.",
    {"COLLECT 100 GLINTS", "ESCAPE THE LUMBER ROOM", "RESTORE THE ROOM'S BALANCE"},
    GLYPH_LEFT GLYPH_RIGHT "\tWALK\n"
    GLYPH_A "\tJUMP\n"
    "HOLD " GLYPH_DOWN "\tSHRINK INTO WHAT YOU STAND ON\n"
    "HOLD " GLYPH_UP "\tGROW BACK A SIZE\n"
    GLYPH_B "\tPICK UP WHAT'S UNDERFOOT / THROW\n"
    GLYPH_UP "+" GLYPH_B " / " GLYPH_DOWN "+" GLYPH_B "\tTHROW UP / SET DOWN\n"
    GLYPH_UP "\tTALK, DOORS, SHOPS\n"
    "SELECT\tTHE BAG: UPGRADES AND GLINTS\n"
    "START\tPAUSE",
    C_TEAL, C_AMBER,
    dd_load, dd_start, dd_update, dd_draw, dd_quit, dd_draw_label, dd_query, dd_cheat,
    "MINI & MAX", 45,
};
