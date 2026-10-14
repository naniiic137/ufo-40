/* FOSSIL FEUD - birds at war over the totem fields.
 * Cartridge 12 of UFO 40, a tribute to Avianos (UFO 50 #12).
 * See docs/games/12-fossil-feud.md. This file: the title, the campaign
 * (three moults and five ordeals), the custom match, the story, the
 * results and the ending, the records, the save and the test hooks. A
 * match itself is in feud_turn.c, its rules in feud_rules.c. */
#include "feud.h"

FeudGame fg;
FeudSave fsv;

#define SAVE_MAGIC 0x46455501u

const char *const FEUD_SCEN_NAME[FD_SCEN] = {
    "FIRST MOULT", "SECOND MOULT", "FULL PLUME", "ORDEAL I: THE FAT GRANARY", "ORDEAL II: THE SECOND EYRIE",
    "ORDEAL III: THE CHOSEN FLOCK", "ORDEAL IV: THE TANGLED RITES", "ORDEAL V: ALL AT ONCE",
};
const char *const FEUD_SCEN_SAY[FD_SCEN] = {
    "THE SLATE FLOCK IS YOUNG AND RASH.",
    "THE SLATE FLOCK HAS LEARNED A THING OR TWO.",
    "THEIR ELDERS LEAD NOW. BEAT THEM AND THE WAR IS OVER.",
    "THEY START WITH A GRANARY'S WORTH OF EXTRA GRAIN.",
    "THEY START WITH A SECOND EYRIE ALREADY THEIRS.",
    "THEY START WITH AN ANCESTOR'S BLESSING.",
    "THE ANCESTORS' ACTIONS ARE DEALT OUT AFRESH.",
    "ALL OF THE ABOVE, AND TWO TOTEMS STAND ON OPEN GROUND.",
};

const char *const FEUD_STORY =
    "LONG AGO THE FURRED ONES BURNED THEIR WORLD AND WENT QUIET. THE BIRDS CAME DOWN FROM THE TREES AND TOOK WHAT WAS "
    "LEFT.\n\nNOW TWO NATIONS OF BIRDS, THE RUST ROOST AND THE SLATE FLOCK, FIGHT OVER THE TOTEM FIELDS, AND BOTH PRAY TO "
    "THE GIANTS WHOSE BONES LIE UNDER THE GROUND.";
const char *const FEUD_ENDING =
    "THE LAST TOTEM BOWS TO THE RUST ROOST, AND THE SLATE FLOCK SCATTERS TO THE CLIFFS.\n\nTHAT NIGHT THE ELDERS DIG A NEW "
    "SHRINE. THEIR SPADES STRIKE SOMETHING HARD: A WALL OF GLASS AND STEEL, LEFT BY THE FURRED ONES, WHO ALSO WON EVERY "
    "WAR THEY EVER FOUGHT.\n\nTHE ELDERS FILL THE HOLE IN AGAIN, AND SAY NOTHING.";
const char *const FEUD_SECRET = "THE BONES DO NOT ANSWER. THEY NEVER DID. WE ONLY EVER HEAR OURSELVES.";

static void set_state(int s) {
    fg.state = s;
    fg.state_t = 0;
}

/* ---- the save ------------------------------------------------------------------ */

static void save_defaults(void) {
    memset(&fsv, 0, sizeof fsv);
    fsv.magic = SAVE_MAGIC;
    fsv.eyries = 6;
    fsv.peaks = 4;
    fsv.fog = 1;
    fsv.opp = 1;
}

static void load_save(void) {
    FeudSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) {
        fsv = tmp;
        fsv.eyries = (uint8_t)iclamp(fsv.eyries, 2, 6);
        fsv.peaks = (uint8_t)iclamp(fsv.peaks, 0, 8);
        fsv.fog = fsv.fog ? 1 : 0;
        fsv.opp = (uint8_t)iclamp(fsv.opp, 0, FD_LEVELS - 1);
        return;
    }
    save_defaults();
}

void feud_save_now(void) {
    fsv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &fsv, (int)sizeof fsv);
}

static void goals_from_save(void) {
    if (fsv.gift) game_award(GOAL_BEACON);
    if ((fsv.beaten & 7) == 7) game_award(GOAL_SAUCER);
    if ((fsv.beaten & 0xF8) == 0xF8) game_award(GOAL_ALIEN);
}

void feud_award_gift(void) {
    if (!fsv.gift) {
        fsv.gift = 1;
        feud_save_now();
    }
    game_award(GOAL_BEACON);
}

static bool unlocked(int s) { return s == 0 || (fsv.beaten >> (s - 1) & 1); }

/* ---- the screens ------------------------------------------------------------------ */

static void pause_pick(int i);
static const char *const PAUSE_ITEMS[] = {"LEAVE THE MATCH"};

static void to_title(void) {
    set_state(FS_TITLE);
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    music_play(FEUD_MUS_TITLE);
}

static void to_campaign(void) {
    set_state(FS_CAMPAIGN);
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    music_play(FEUD_MUS_TITLE);
}

static void pause_pick(int i) {
    (void)i;
    if (fg.scenario < 0) { set_state(FS_CUSTOM); game_set_pausable(false); game_pause_items(0, NULL, NULL); music_play(FEUD_MUS_TITLE); }
    else to_campaign();
}

static void start_scenario(int s) {
    game_pause_items(1, PAUSE_ITEMS, pause_pick);
    feud_match_begin(s);
}

static void title_update(void) {
    game_set_pausable(false);
    if (btn_repeat(BTN_UP)) { fg.title_sel = (fg.title_sel + 3) % 4; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { fg.title_sel = (fg.title_sel + 1) % 4; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if ((btnp(BTN_A) || btnp(BTN_START)) && fg.state_t > 8) {
        if (fg.title_sel == 2) { sfx_play_name("ui_error"); return; } /* 2 PLAYERS: the next update */
        sfx_play_name("ui_ok");
        input_consume();
        if (fg.title_sel == 0) { set_state(FS_CAMPAIGN); fg.camp_sel = 0; for (int s = 0; s < FD_SCEN; s++) if (unlocked(s) && !(fsv.beaten >> s & 1)) { fg.camp_sel = s; break; } }
        else if (fg.title_sel == 1) { set_state(FS_CUSTOM); fg.cust_sel = 0; }
        else { set_state(FS_RECORDS); }
    }
}

static void campaign_update(void) {
    game_set_pausable(false);
    if (btn_repeat(BTN_UP)) { fg.camp_sel = (fg.camp_sel + FD_SCEN - 1) % FD_SCEN; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { fg.camp_sel = (fg.camp_sel + 1) % FD_SCEN; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { sfx_play_name("ui_back"); to_title(); return; }
    if ((btnp(BTN_A) || btnp(BTN_START)) && fg.state_t > 8) {
        if (!unlocked(fg.camp_sel)) { sfx_play_name("ui_error"); return; }
        sfx_play_name("ui_ok");
        input_consume();
        if (fg.camp_sel == 0) { set_state(FS_STORY); music_play(FEUD_MUS_STORY); }
        else start_scenario(fg.camp_sel);
    }
}

#define CUST_ROWS 5
static void custom_update(void) {
    game_set_pausable(false);
    if (btn_repeat(BTN_UP)) { fg.cust_sel = (fg.cust_sel + CUST_ROWS - 1) % CUST_ROWS; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { fg.cust_sel = (fg.cust_sel + 1) % CUST_ROWS; sfx_play_name("ui_move"); }
    int d = btn_repeat(BTN_RIGHT) ? 1 : btn_repeat(BTN_LEFT) ? -1 : 0;
    if (d) {
        switch (fg.cust_sel) {
        case 0: fsv.opp = (uint8_t)((fsv.opp + FD_LEVELS + d) % FD_LEVELS); break;
        case 1: fsv.eyries = (uint8_t)iclamp(fsv.eyries + d, 2, 6); break;
        case 2: fsv.peaks = (uint8_t)iclamp(fsv.peaks + d, 0, 8); break;
        case 3: fsv.fog ^= 1; break;
        default: break;
        }
        sfx_play_name("ui_move");
    }
    if (btnp(BTN_B)) { sfx_play_name("ui_back"); feud_save_now(); to_title(); return; }
    if ((btnp(BTN_A) || btnp(BTN_START)) && fg.cust_sel == CUST_ROWS - 1 && fg.state_t > 8) {
        sfx_play_name("ui_ok");
        input_consume();
        feud_save_now();
        start_scenario(-1);
    }
}

static void feud_update(void) {
    fg.state_t++;
    fg.frame_t++;
    switch (fg.state) {
    case FS_TITLE: title_update(); break;
    case FS_CAMPAIGN: campaign_update(); break;
    case FS_CUSTOM: custom_update(); break;
    case FS_RECORDS:
        game_set_pausable(false);
        if (fg.state_t > 10 && (btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_START))) { input_consume(); to_title(); }
        break;
    case FS_STORY:
        game_set_pausable(false);
        if ((fg.state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) || fg.state_t > 1500) {
            input_consume();
            start_scenario(0);
        }
        break;
    case FS_MATCH: feud_match_update(); break;
    case FS_BATTLE: feud_battle_update(); break;
    case FS_RESULT:
        game_set_pausable(false);
        if (fg.state_t > 90 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            game_pause_items(0, NULL, NULL);
            if (fg.won && fg.scenario == 2) { set_state(FS_ENDING); music_play(FEUD_MUS_ENDING); }
            else if (fg.scenario < 0) { set_state(FS_CUSTOM); music_play(FEUD_MUS_TITLE); }
            else {
                to_campaign();
                if (fg.won) for (int s = 0; s < FD_SCEN; s++) if (unlocked(s) && !(fsv.beaten >> s & 1)) { fg.camp_sel = s; break; }
            }
        }
        break;
    case FS_ENDING:
        game_set_pausable(false);
        if (btn(BTN_A)) fg.state_t += 2;
        if (fg.state_t > 1400) { input_consume(); to_campaign(); fg.camp_sel = 3; }
        break;
    default: break;
    }
}

/* ---- cartridge interface ----------------------------------------------------------- */

static void feud_load(void) {
    feud_art_load();
    feud_audio_load();
}

static void feud_start(void) {
    load_save();
    memset(&fg, 0, sizeof fg);
    fg.hl_tile = -1;
    goals_from_save();
    to_title();
}

static void feud_quit(void) { feud_save_now(); }

static int tile_query(const char *key, int *out) {
    /* t<i>_owner, _terr, _flag, _eyrie, _frozen, _seen, _ndrop, _u<k>, _b<k>, _moved<k>, _units */
    int i = atoi(key + 1);
    const char *u = strchr(key, '_');
    if (i < 0 || i >= FD_TILES || !u) return 0;
    const FeudTile *t = &fg.m.t[i];
    if (!strcmp(u, "_owner")) { *out = t->owner; return 1; }
    if (!strcmp(u, "_terr")) { *out = t->terr; return 1; }
    if (!strcmp(u, "_flag")) { *out = t->flag; return 1; }
    if (!strcmp(u, "_eyrie")) { *out = t->eyrie; return 1; }
    if (!strcmp(u, "_frozen")) { *out = t->frozen; return 1; }
    if (!strcmp(u, "_seen")) { *out = t->seen; return 1; }
    if (!strcmp(u, "_ndrop")) { *out = t->ndrop; return 1; }
    if (!strcmp(u, "_units")) { *out = feud_units_on(&fg.m, i); return 1; }
    if (!strcmp(u, "_blds")) { *out = feud_tile_blds(t); return 1; }
    if (!strncmp(u, "_moved", 6)) { *out = t->moved[iclamp(atoi(u + 6), 0, FU_KINDS - 1)]; return 1; }
    if (!strncmp(u, "_u", 2)) { *out = t->u[iclamp(atoi(u + 2), 0, FU_KINDS - 1)]; return 1; }
    if (!strncmp(u, "_b", 2)) { *out = t->b[iclamp(atoi(u + 2), 0, FB_KINDS - 1)]; return 1; }
    return 0;
}

static int feud_query(const char *key, int *out) {
    FeudMatch *m = &fg.m;
    FeudBattle *b = &fg.b;
    if (!strcmp(key, "bot")) { *out = feud_bot_buttons(); return 1; }
    if (!strcmp(key, "bot_goal")) { *out = feud_bot_goal_type; return 1; }
    if (!strcmp(key, "state")) { *out = fg.state; return 1; }
    if (!strcmp(key, "title_sel")) { *out = fg.title_sel; return 1; }
    if (!strcmp(key, "camp_sel")) { *out = fg.camp_sel; return 1; }
    if (!strcmp(key, "cust_sel")) { *out = fg.cust_sel; return 1; }
    if (!strcmp(key, "scenario")) { *out = fg.scenario; return 1; }
    if (!strcmp(key, "ai_level")) { *out = fg.ai_level; return 1; }
    if (!strcmp(key, "mp")) { *out = fg.mp; return 1; }
    if (!strcmp(key, "au")) { *out = fg.au; return 1; }
    if (!strcmp(key, "cur")) { *out = fg.cur; return 1; }
    if (!strcmp(key, "msel")) { *out = fg.msel; return 1; }
    if (!strcmp(key, "act_i")) { *out = fg.act_i; return 1; }
    if (!strcmp(key, "act_id")) { *out = fg.act_id; return 1; }
    if (!strcmp(key, "anc")) { *out = fg.anc; return 1; }
    if (!strcmp(key, "cpu_anc")) { *out = fg.cpu_anc; return 1; }
    if (!strcmp(key, "pray_sel")) { *out = fg.pray_sel; return 1; }
    if (!strcmp(key, "bless_sel")) { *out = fg.bless_sel; return 1; }
    if (!strcmp(key, "anc_sel")) { *out = fg.anc_sel; return 1; }
    if (!strcmp(key, "split_sel")) { *out = fg.split_sel; return 1; }
    if (!strncmp(key, "split", 5) && isdigit((unsigned char)key[5])) { *out = fg.split[iclamp(atoi(key + 5), 0, FU_KINDS - 1)]; return 1; }
    if (!strcmp(key, "src")) { *out = fg.src; return 1; }
    if (!strcmp(key, "omen")) { *out = fg.omen; return 1; }
    if (!strcmp(key, "tgt_stage")) { *out = fg.tgt_stage; return 1; }
    if (!strcmp(key, "secret_t")) { *out = fg.secret_t; return 1; }
    if (!strcmp(key, "last_frozen")) { *out = fg.last_frozen; return 1; }
    if (!strcmp(key, "last_bolt")) { *out = fg.last_bolt; return 1; }
    if (!strcmp(key, "won")) { *out = fg.won; return 1; }
    if (!strcmp(key, "turn")) { *out = m->turn; return 1; }
    if (!strcmp(key, "side")) { *out = m->side; return 1; }
    if (!strcmp(key, "winner")) { *out = m->winner; return 1; }
    if (!strcmp(key, "fog")) { *out = m->fog; return 1; }
    if (!strcmp(key, "ordeal")) { *out = m->ordeal; return 1; }
    if (!strcmp(key, "shuffled")) { *out = m->shuffled; return 1; }
    if (!strcmp(key, "noffer")) { *out = m->noffer; return 1; }
    if (!strncmp(key, "offer", 5) && isdigit((unsigned char)key[5])) { *out = m->offer[iclamp(atoi(key + 5), 0, 2)]; return 1; }
    if (!strncmp(key, "recent", 6) && isdigit((unsigned char)key[6])) { *out = m->recent[iclamp(atoi(key + 6), 0, 1)]; return 1; }
    if (!strncmp(key, "flags", 5) && isdigit((unsigned char)key[5])) { *out = feud_flags_held(m, key[5] - '0'); return 1; }
    if (!strncmp(key, "tiles", 5) && isdigit((unsigned char)key[5])) { *out = feud_tiles_held(m, key[5] - '0'); return 1; }
    if (!strncmp(key, "army", 4) && isdigit((unsigned char)key[4])) { *out = feud_army(m, key[4] - '0'); return 1; }
    if (!strncmp(key, "hold", 4) && isdigit((unsigned char)key[4])) { *out = m->s[key[4] - '0'].hold; return 1; }
    if (!strncmp(key, "res", 3) && isdigit((unsigned char)key[3]) && key[4] == '_') {
        *out = m->s[iclamp(key[3] - '0', 0, 1)].res[iclamp(atoi(key + 5), 0, FR_KINDS - 1)];
        return 1;
    }
    if (!strncmp(key, "favour", 6) && isdigit((unsigned char)key[6]) && key[7] == '_') {
        *out = m->s[iclamp(key[6] - '0', 0, 1)].favour[iclamp(atoi(key + 8), 0, FD_ANC - 1)];
        return 1;
    }
    if (!strncmp(key, "star", 4) && isdigit((unsigned char)key[4]) && key[5] == '_') {
        *out = m->star[iclamp(key[4] - '0', 0, 1)][iclamp(atoi(key + 6), 0, FD_ACTS - 1)];
        return 1;
    }
    if (!strncmp(key, "deck", 4) && isdigit((unsigned char)key[4]) && key[5] == '_') {
        *out = m->deck[iclamp(key[4] - '0', 0, FD_ANC - 1)][iclamp(atoi(key + 6), 0, 2)];
        return 1;
    }
    if (key[0] == 't' && isdigit((unsigned char)key[1])) return tile_query(key, out);
    if (!strcmp(key, "blds_total0")) { int n = 0; for (int k = 0; k < FB_KINDS; k++) n += feud_bld_count(m, 0, k); *out = n; return 1; }
    if (!strncmp(key, "bldc", 4) && isdigit((unsigned char)key[4]) && key[5] == '_') {
        *out = feud_bld_count(m, key[4] - '0', iclamp(atoi(key + 6), 0, FB_KINDS - 1));
        return 1;
    }
    if (!strcmp(key, "valid")) { *out = (int)feud_valid_tiles(); return 1; }
    if (!strcmp(key, "march_mask")) { *out = (int)feud_march_dests(m, 0, fg.src, feud_lv(m, 0, fg.act_id)->mflags); return 1; }
    if (!strcmp(key, "peaks")) { int n = 0; for (int t = 0; t < FD_TILES; t++) n += m->t[t].terr == FT_PEAK; *out = n; return 1; }
    if (!strcmp(key, "eyries")) { int n = 0; for (int t = 0; t < FD_TILES; t++) n += m->t[t].eyrie; *out = n; return 1; }
    if (!strcmp(key, "totems")) { int n = 0; for (int t = 0; t < FD_TILES; t++) n += m->t[t].flag; *out = n; return 1; }
    if (!strcmp(key, "bare_totems")) { int n = 0; for (int t = 0; t < FD_TILES; t++) n += m->t[t].flag && !m->t[t].eyrie; *out = n; return 1; }
    /* the battle */
    if (!strcmp(key, "b_result")) { *out = b->result; return 1; }
    if (!strcmp(key, "b_t")) { *out = b->t; return 1; }
    if (!strcmp(key, "b_atk")) { *out = b->atk; return 1; }
    if (!strcmp(key, "b_terr")) { *out = b->terr; return 1; }
    if (!strcmp(key, "b_hits")) { *out = b->hits; return 1; }
    if (!strcmp(key, "b_still")) { *out = b->still_t; return 1; }
    if (!strcmp(key, "b_woke")) { *out = b->woke; return 1; }
    if (!strcmp(key, "b_surprise")) { *out = b->surprise; return 1; }
    if (!strcmp(key, "b_shield_saves")) { *out = b->shield_saves; return 1; }
    if (!strcmp(key, "b_blocked")) { *out = b->blocked_shots; return 1; }
    if (!strcmp(key, "b_nbld")) { int n = 0; for (int k = 0; k < b->nbld; k++) n += b->bld[k].hp > 0; *out = n; return 1; }
    if (!strncmp(key, "b_order", 7)) { *out = b->order[iclamp(atoi(key + 7), 0, 1)]; return 1; }
    if (!strncmp(key, "b_count", 7)) { *out = feud_battle_count(b, iclamp(atoi(key + 7), 0, 1)); return 1; }
    if (!strncmp(key, "b_field", 7)) {
        int s = iclamp(atoi(key + 7), 0, 1), n = 0;
        for (int i = 0; i < FB_MAX; i++) n += b->s[i].alive && b->s[i].side == s;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "b_reserve", 9)) { int s = iclamp(atoi(key + 9), 0, 1), n = 0; for (int k = 0; k < FU_KINDS; k++) n += b->reserve[s][k]; *out = n; return 1; }
    if (!strncmp(key, "b_lost", 6)) { int s = iclamp(atoi(key + 6), 0, 1), n = 0; for (int k = 0; k < FU_KINDS; k++) n += b->lost[s][k]; *out = n; return 1; }
    if (!strncmp(key, "b_escaped", 9)) { int s = iclamp(atoi(key + 9), 0, 1), n = 0; for (int k = 0; k < FU_KINDS; k++) n += b->escaped[s][k]; *out = n; return 1; }
    if (!strncmp(key, "b_cells", 7)) {
        int c = atoi(key + 7), n = 0;
        for (int y = 0; y < FB_GH; y++)
            for (int x = 0; x < FB_GW; x++) n += b->grid[y][x] == c;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "b_frozen")) { int n = 0; for (int i = 0; i < FB_MAX; i++) n += b->s[i].alive && b->s[i].frozen; *out = n; return 1; }
    if (!strcmp(key, "b_maxx_def")) {
        int mx = 0;
        for (int i = 0; i < FB_MAX; i++) if (b->s[i].alive && b->s[i].side == b->def) mx = imax(mx, b->s[i].x >> 8);
        *out = mx;
        return 1;
    }
    if (!strcmp(key, "b_shield0")) {
        *out = -1;
        for (int i = 0; i < FB_MAX; i++) if (b->s[i].alive && b->s[i].kind == FU_LANCER) { *out = b->s[i].shield; break; }
        return 1;
    }
    /* the save */
    if (!strcmp(key, "save_beaten")) { *out = fsv.beaten; return 1; }
    if (!strcmp(key, "save_gift")) { *out = fsv.gift; return 1; }
    if (!strcmp(key, "save_wins")) { *out = fsv.wins; return 1; }
    if (!strcmp(key, "save_losses")) { *out = fsv.losses; return 1; }
    if (!strcmp(key, "save_matches")) { *out = fsv.matches; return 1; }
    if (!strcmp(key, "save_secret")) { *out = fsv.secret; return 1; }
    if (!strcmp(key, "save_eyries")) { *out = fsv.eyries; return 1; }
    if (!strcmp(key, "save_peaks")) { *out = fsv.peaks; return 1; }
    if (!strcmp(key, "save_fog")) { *out = fsv.fog; return 1; }
    if (!strcmp(key, "save_opp")) { *out = fsv.opp; return 1; }
    if (!strncmp(key, "save_picks", 10)) { *out = fsv.picks[iclamp(atoi(key + 10), 0, FD_ANC - 1)]; return 1; }
    if (!strcmp(key, "layout")) { *out = feud_layout_audit(); return 1; }
    if (!strcmp(key, "art_bad")) { *out = feud_art_check(); return 1; }
    if (!strcmp(key, "eval0")) { *out = feud_eval(m, 0); return 1; }
    if (!strncmp(key, "stars", 5) && isdigit((unsigned char)key[5])) {
        int n = 0;
        for (int a = 0; a < FD_ACTS; a++) n += m->star[iclamp(key[5] - '0', 0, 1)][a];
        *out = n;
        return 1;
    }
    if (!strcmp(key, "deck_ok")) {
        /* every one of the fifteen actions dealt exactly once */
        int seen = 0;
        for (int a = 0; a < FD_ANC; a++)
            for (int i = 0; i < 3; i++) seen |= 1 << m->deck[a][i];
        *out = seen == (1 << FD_ACTS) - 1;
        return 1;
    }
    if (!strcmp(key, "deck_moved")) {
        int n = 0;
        for (int a = 0; a < FD_ANC; a++)
            for (int i = 0; i < 3; i++) n += m->deck[a][i] != a * 3 + i;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "playing_", 8)) {
        /* playing_title, _map, _rival, _battle, _ordeal, _ending, _story */
        static const char *const N[] = {"title", "map", "rival", "battle", "ordeal", "ending", "story"};
        const int ids[] = {FEUD_MUS_TITLE, FEUD_MUS_MAP, FEUD_MUS_RIVAL, FEUD_MUS_BATTLE, FEUD_MUS_ORDEAL, FEUD_MUS_ENDING, FEUD_MUS_STORY};
        for (int i = 0; i < 7; i++)
            if (!strcmp(key + 8, N[i])) { *out = music_playing() == ids[i]; return 1; }
        return 0;
    }
    return 0;
}

static int feud_cheat(const char *cmd) {
    FeudMatch *m = &fg.m;
    int a, b, c, d, e, f;
    if (sscanf(cmd, "match %d", &a) == 1) {
        start_scenario(iclamp(a, -1, FD_SCEN - 1));
        return 1;
    }
    if (sscanf(cmd, "beaten %d", &a) == 1) { fsv.beaten = (uint8_t)a; feud_save_now(); return 1; }
    if (sscanf(cmd, "res %d %d %d %d", &a, &b, &c, &d) == 4) {
        a = iclamp(a, 0, 1);
        m->s[a].res[0] = b;
        m->s[a].res[1] = c;
        m->s[a].res[2] = d;
        return 1;
    }
    if (sscanf(cmd, "units %d %d %d %d %d %d", &a, &b, &c, &d, &e, &f) == 6) {
        FeudTile *t = &m->t[iclamp(a, 0, FD_TILES - 1)];
        t->u[0] = (uint8_t)b; t->u[1] = (uint8_t)c; t->u[2] = (uint8_t)d; t->u[3] = (uint8_t)e; t->u[4] = (uint8_t)f;
        memset(t->moved, 0, sizeof t->moved);
        return 1;
    }
    if (sscanf(cmd, "blds %d %d %d %d %d", &a, &b, &c, &d, &e) == 5) {
        FeudTile *t = &m->t[iclamp(a, 0, FD_TILES - 1)];
        t->b[0] = (uint8_t)b; t->b[1] = (uint8_t)c; t->b[2] = (uint8_t)d; t->b[3] = (uint8_t)e;
        return 1;
    }
    if (sscanf(cmd, "own %d %d", &a, &b) == 2) { m->t[iclamp(a, 0, FD_TILES - 1)].owner = (int8_t)iclamp(b, -1, 1); return 1; }
    if (sscanf(cmd, "terr %d %d", &a, &b) == 2) { m->t[iclamp(a, 0, FD_TILES - 1)].terr = (uint8_t)iclamp(b, 0, FT_KINDS - 1); return 1; }
    if (sscanf(cmd, "flag %d %d", &a, &b) == 2) { m->t[iclamp(a, 0, FD_TILES - 1)].flag = (uint8_t)(b != 0); return 1; }
    if (sscanf(cmd, "eyrie %d %d", &a, &b) == 2) { m->t[iclamp(a, 0, FD_TILES - 1)].eyrie = (uint8_t)(b != 0); return 1; }
    if (sscanf(cmd, "frozen %d %d", &a, &b) == 2) { m->t[iclamp(a, 0, FD_TILES - 1)].frozen = (uint8_t)b; return 1; }
    if (sscanf(cmd, "drop %d %d %d", &a, &b, &c) == 3) {
        FeudTile *t = &m->t[iclamp(a, 0, FD_TILES - 1)];
        t->ndrop = (uint8_t)iclamp(b, 0, 2);
        t->drop[0] = t->drop[1] = (uint8_t)iclamp(c, 0, FR_KINDS - 1);
        return 1;
    }
    if (!strcmp(cmd, "clearboard")) {
        /* an empty board but for the two homes, which keep only their totems */
        for (int t = 0; t < FD_TILES; t++) {
            FeudTile *x = &m->t[t];
            bool home = x->owner >= 0 && x->eyrie && x->b[FB_ROOST];
            int8_t own = x->owner;
            memset(x, 0, sizeof *x);
            x->owner = home ? own : -1;
            x->terr = home ? FT_EYRIE : FT_MEADOW;
            x->eyrie = home;
            x->flag = home;
        }
        return 1;
    }
    if (sscanf(cmd, "offer %d %d %d", &a, &b, &c) == 3) {
        m->offer[0] = (uint8_t)a; m->offer[1] = (uint8_t)b; m->offer[2] = (uint8_t)c; m->noffer = 3;
        return 1;
    }
    if (sscanf(cmd, "star %d %d %d", &a, &b, &c) == 3) { m->star[iclamp(a, 0, 1)][iclamp(b, 0, FD_ACTS - 1)] = (uint8_t)iclamp(c, 0, 2); return 1; }
    if (sscanf(cmd, "favour %d %d %d", &a, &b, &c) == 3) { m->s[iclamp(a, 0, 1)].favour[iclamp(b, 0, FD_ANC - 1)] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "hold %d %d", &a, &b) == 2) { m->s[iclamp(a, 0, 1)].hold = (uint8_t)b; return 1; }
    if (sscanf(cmd, "fog %d", &a) == 1) { m->fog = (uint8_t)(a != 0); return 1; }
    if (sscanf(cmd, "deck %d %d %d", &a, &b, &c) == 3) { m->deck[iclamp(a, 0, FD_ANC - 1)][iclamp(b, 0, 2)] = (uint8_t)iclamp(c, 0, FD_ACTS - 1); return 1; }
    if (sscanf(cmd, "ai %d", &a) == 1) { fg.ai_level = iclamp(a, 0, FD_LEVELS - 1); return 1; }
    if (sscanf(cmd, "cursor %d", &a) == 1) { fg.cur = iclamp(a, 0, FD_TILES - 1); return 1; }
    if (sscanf(cmd, "pray %d", &a) == 1) {
        /* straight to praying to Ancestor a on your turn (as if it were on offer) */
        m->offer[0] = (uint8_t)a; m->noffer = 1 + (m->noffer > 1 ? m->noffer - 1 : 0);
        fg.mp = MP_PRAY; fg.pray_sel = 0; fg.mp_t = 20;
        return 1;
    }
    if (sscanf(cmd, "wipe %d", &a) == 1) {
        /* every tile of a side, gone */
        for (int t = 0; t < FD_TILES; t++)
            if (m->t[t].owner == a) { m->t[t].owner = -1; memset(m->t[t].u, 0, sizeof m->t[t].u); }
        return 1;
    }
    if (!strcmp(cmd, "skipbanner")) { fg.mp_t = 999; return 1; }
    if (!strcmp(cmd, "endturn")) { feud_end_turn(m); fg.mp = MP_BANNER; fg.mp_t = 0; return 1; }
    if (sscanf(cmd, "winner %d", &a) == 1) { m->winner = (int8_t)a; return 1; }
    if (!strncmp(cmd, "battle ", 7)) {
        /* battle ATK SRC DST n0 n1 [n2 n3 n4 surprise]: a fight set up by hand */
        int v[10] = {0};
        int n = sscanf(cmd, "battle %d %d %d %d %d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8]);
        uint8_t cnt[FU_KINDS] = {0};
        for (int k = 0; k < FU_KINDS && 3 + k < n; k++) cnt[k] = (uint8_t)v[3 + k];
        bool surprise = n >= 9 && v[8];
        feud_battle_begin(&fg.b, m, iclamp(v[0], 0, 1), v[1], iclamp(v[2], 0, FD_TILES - 1), cnt, surprise);
        fg.lock_orders = 0;
        feud_battle_ai_order(&fg.b, 1);
        fg.battle_return = 0;
        fg.state = FS_BATTLE;
        fg.state_t = 0;
        music_play(FEUD_MUS_BATTLE);
        return 1;
    }
    if (sscanf(cmd, "order %d %d", &a, &b) == 2) { fg.b.order[iclamp(a, 0, 1)] = iclamp(b, 0, FO_KINDS - 1); fg.lock_orders = 1; return 1; }
    if (!strcmp(cmd, "battlestep")) { feud_battle_step(&fg.b); return 1; }
    return 0;
}

const GameDef GAME_FOSSILFEUD = {
    "fossilfeud",
    "FOSSIL FEUD",
    "1985",
    "STRATEGY",
    "TWO BIRD NATIONS, FIVE OLD GODS OF BONE. PRAY, MARCH, HOLD FOUR TOTEMS.",
    {"MAX ALL THREE ACTIONS OF ONE ANCESTOR IN A MATCH", "WIN ALL THREE MOULTS",
     "PASS ALL FIVE ORDEALS"},
    GLYPH_DPAD "\tMOVE THE CURSOR, CHOOSE\n"
    GLYPH_A "\tPRAY, PICK, CONFIRM\n"
    GLYPH_B "\tBACK, OR FINISH AN ACTION\n"
    GLYPH_LEFT GLYPH_RIGHT "\tHOW MANY BIRDS GO\n"
    "BATTLE\t" GLYPH_LEFT GLYPH_RIGHT " CHANGE THE ORDERS\n"
    "START\tPAUSE",
    C_EARTH, C_AMBER,
    feud_load, feud_start, feud_update, feud_draw, feud_quit, feud_draw_label, feud_query, feud_cheat,
    "AVIANOS", 12,
    NULL,
};
