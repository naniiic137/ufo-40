/* FOSSIL FEUD - a match on screen: your turn (pray, take a blessing, then
 * the Ancestor's three actions top to bottom, each with the board cursor
 * and small menus), the Slate Flock's turn played out step by step, and
 * battles with their orders. */
#include "feud.h"

void feud_sfx(const char *name) {
    if (!feud_quiet) sfx_play_name(name);
}

void feud_say(const char *s) {
    snprintf(fg.note, sizeof fg.note, "%s", s);
    fg.msg_t = 150;
}

static int cur_act(void) { return fg.m.deck[fg.anc][fg.act_i]; }

/* ---- starting and finishing --------------------------------------------- */

void feud_match_begin(int scenario) {
    FeudSetup su;
    memset(&su, 0, sizeof su);
    fg.scenario = scenario;
    if (scenario < 0) {
        su.eyries = fsv.eyries;
        su.peaks = fsv.peaks;
        su.fog = fsv.fog;
        fg.ai_level = iclamp(fsv.opp, 0, FD_LEVELS - 1);
    } else {
        su.eyries = scenario == 7 ? 4 : 6; /* ordeal 5: two totems stand on open ground */
        su.peaks = 4;
        su.fog = 1;
        su.ordeal = (uint8_t)(scenario >= 3 ? scenario - 2 : 0);
        fg.ai_level = scenario < 3 ? scenario : 2;
    }
    su.seed = ((uint64_t)rng_next(&g_rng) << 32) | rng_next(&g_rng);
    feud_setup_match(&fg.m, &su);
    rng_seed(&fg.ai_rng, su.seed ^ 0x5eedull);
    fg.state = FS_MATCH;
    fg.state_t = 0;
    fg.mp = MP_BANNER;
    fg.mp_t = 0;
    fg.note[0] = 0;
    fg.msg_t = 0;
    for (int t = 0; t < FD_TILES; t++)
        if (fg.m.t[t].owner == 0 && fg.m.t[t].eyrie) fg.cur = t;
    fg.turns_played = 0;
    if (fsv.matches < 65535) fsv.matches++;
    feud_save_now();
    music_play(fg.m.ordeal ? FEUD_MUS_ORDEAL : FEUD_MUS_MAP);
    game_set_pausable(true);
}

void feud_finish_match(int winner) {
    fg.won = winner == 0;
    fg.state = FS_RESULT;
    fg.state_t = 0;
    fg.result_t = 0;
    if (fg.won) {
        if (fsv.wins < 65535) fsv.wins++;
        int turns = (fg.m.turn + 1) / 2;
        if (!fsv.best_turns || turns < fsv.best_turns) fsv.best_turns = (uint16_t)turns;
        if (fg.scenario >= 0) fsv.beaten |= (uint8_t)(1 << fg.scenario);
        if ((fsv.beaten & 7) == 7) game_award(GOAL_SAUCER);
        if ((fsv.beaten & 0xF8) == 0xF8) game_award(GOAL_ALIEN);
        music_play(FEUD_MUS_WIN);
    } else {
        if (fsv.losses < 65535) fsv.losses++;
        music_play(FEUD_MUS_LOSE);
    }
    feud_save_now();
    game_set_pausable(false);
}

static bool check_over(void) {
    if (fg.m.winner < 0) return false;
    feud_finish_match(fg.m.winner);
    return true;
}

/* ---- your turn ------------------------------------------------------------- */

static void first_valid(void) {
    uint32_t v = feud_valid_tiles();
    if (v & (1u << fg.cur)) return;
    for (int t = 0; t < FD_TILES; t++)
        if (v & (1u << t)) { fg.cur = t; return; }
}

static void next_action(void) {
    fg.act_i++;
    if (fg.act_i >= 3) {
        feud_end_turn(&fg.m);
        fg.turns_played++;
        fg.mp = MP_BANNER;
        fg.mp_t = 0;
        if (!check_over()) music_play(FEUD_MUS_RIVAL);
        return;
    }
    feud_start_action();
}

static void gift_check(void) {
    if (feud_gift_check(&fg.m, 0)) feud_award_gift();
}

void feud_start_action(void) {
    fg.act_id = cur_act();
    fg.msel = 0;
    fg.omen = -1;
    fg.tgt_stage = 0;
    switch (FEUD_ACT[fg.act_id].kind) {
    case AK_GATHER:
    case AK_CALLUP: fg.au = AU_CONFIRM; break;
    case AK_MARCH: feud_reset_moved(&fg.m); fg.au = AU_MAP; first_valid(); break;
    case AK_ENLIST:
    case AK_BUILD: fg.au = AU_MAP; first_valid(); break;
    default: fg.au = AU_MENU; break;
    }
    fg.mp = MP_ACT;
    fg.mp_t = 0;
}

static void begin_actions(void) {
    fg.act_i = 0;
    feud_start_action();
}

uint32_t feud_valid_tiles(void) {
    const FeudMatch *m = &fg.m;
    uint32_t v = 0;
    int kind = FEUD_ACT[fg.act_id].kind;
    if (fg.mp != MP_ACT) return 0;
    if (fg.au == AU_DEST) {
        if (fg.omen == OM_UPDRAFT) return feud_flight_dests(m, 0, fg.src);
        return feud_march_dests(m, 0, fg.src, feud_lv(m, 0, fg.act_id)->mflags);
    }
    if (fg.au == AU_TARGET) {
        if (fg.omen == OM_SHIFT && fg.tgt_stage == 1) return feud_shift_dests(m, fg.src);
        return feud_omen_targets(m, 0, fg.omen);
    }
    if (fg.au != AU_MAP) return 0;
    for (int t = 0; t < FD_TILES; t++) {
        bool ok = false;
        if (kind == AK_ENLIST) ok = feud_is_recruit_point(m, 0, t);
        else if (kind == AK_BUILD) ok = feud_can_build_on(m, 0, t);
        else if (kind == AK_MARCH) ok = feud_can_move_from(m, 0, t);
        if (ok) v |= 1u << t;
    }
    return v;
}

int feud_menu_count(void) {
    const FeudOffer *o;
    return feud_offers_of(&fg.m, 0, fg.act_id, &o);
}

static void move_cursor(void) {
    int c = fg.cur % FD_COLS, r = fg.cur / FD_COLS;
    if (btn_repeat(BTN_LEFT) && c > 0) { c--; feud_sfx("ui_move"); }
    if (btn_repeat(BTN_RIGHT) && c < FD_COLS - 1) { c++; feud_sfx("ui_move"); }
    if (btn_repeat(BTN_UP) && r > 0) { r--; feud_sfx("ui_move"); }
    if (btn_repeat(BTN_DOWN) && r < FD_ROWS - 1) { r++; feud_sfx("ui_move"); }
    fg.cur = r * FD_COLS + c;
}

static void start_split(int src) {
    fg.src = src;
    fg.split_sel = -1;
    for (int k = 0; k < FU_KINDS; k++) {
        fg.split[k] = feud_movable(&fg.m, src, k);
        if (fg.split[k] && fg.split_sel < 0) fg.split_sel = k;
    }
    fg.au = AU_SPLIT;
}

static void begin_battle(int atk, int src, int dst, const uint8_t counts[FU_KINDS], bool surprise, int ret) {
    feud_battle_begin(&fg.b, &fg.m, atk, src, dst, counts, surprise);
    fg.battle_return = ret;
    fg.state = FS_BATTLE;
    fg.state_t = 0;
    music_play(FEUD_MUS_BATTLE);
    feud_sfx("fd_clash");
}

static void say_claim(int t) {
    char b[96];
    int n = snprintf(b, sizeof b, "%s CLAIMED", FEUD_TERR_NAME[fg.m.t[t].terr]);
    for (int r = 0; r < FR_KINDS; r++)
        if (feud_last_got[r]) n += snprintf(b + n, sizeof b - (size_t)n, ": +%d %s", feud_last_got[r], FEUD_RES_NAME[r]);
    feud_say(b);
}

static void do_move(void) {
    uint8_t c[FU_KINDS];
    for (int k = 0; k < FU_KINDS; k++) c[k] = (uint8_t)fg.split[k];
    bool flight = fg.omen == OM_UPDRAFT;
    int dst = fg.cur;
    bool surprise = !flight && (feud_lv(&fg.m, 0, fg.act_id)->mflags & MF_SURPRISE);
    if (flight) {
        fg.m.s[0].res[FR_RELICS] -= fg.omen_cost;
        feud_sfx("fd_wind");
    }
    int owner_before = fg.m.t[dst].owner;
    int r = feud_move(&fg.m, 0, fg.src, dst, c);
    fg.au = flight ? AU_MENU : AU_MAP;
    if (r == 1) {
        begin_battle(0, fg.src, dst, c, surprise, 0);
        return;
    }
    feud_sfx("fd_step");
    if (owner_before < 0) say_claim(dst);
    else if (owner_before == 1) feud_say("THEIR EMPTY TILE IS OURS");
    if (!check_over() && !flight) first_valid();
}

static void apply_target(void) {
    FeudMatch *m = &fg.m;
    int t = fg.cur;
    char b[96];
    switch (fg.omen) {
    case OM_SHIFT:
        if (fg.tgt_stage == 0) { fg.src = t; fg.tgt_stage = 1; feud_sfx("ui_ok"); return; }
        m->s[0].res[FR_RELICS] -= fg.omen_cost;
        feud_shift(m, fg.src, t);
        feud_sfx("fd_quake");
        feud_say("THE PEAK MOVES; SCREE IS LEFT BEHIND");
        break;
    case OM_HAIL:
        m->s[0].res[FR_RELICS] -= fg.omen_cost;
        fg.last_frozen = feud_hail(m, t);
        snprintf(b, sizeof b, "HAILSTORM: %d OF THEIR BIRDS FROZEN", fg.last_frozen);
        feud_say(b);
        feud_sfx("fd_hail");
        break;
    case OM_BOLT:
        m->s[0].res[FR_RELICS] -= fg.omen_cost;
        fg.last_bolt = feud_bolt(m, t);
        snprintf(b, sizeof b, "THUNDERBOLT: %d OF THEIR BIRDS STRUCK DOWN", fg.last_bolt);
        feud_say(b);
        feud_sfx("fd_thunder");
        break;
    case OM_SIGHT:
        m->s[0].res[FR_RELICS] -= fg.omen_cost;
        m->t[t].seen = 1;
        feud_say("FAR SIGHT: THE TILE LIES OPEN TO YOU");
        feud_sfx("fd_sight");
        break;
    case OM_HATCH: {
        m->s[0].res[FR_RELICS] -= fg.omen_cost;
        int k = feud_hatch(m, 0, t);
        snprintf(b, sizeof b, "HATCHING: A %s CRACKS ITS SHELL", FEUD_UNIT[k].name);
        feud_say(b);
        feud_sfx("fd_hatch");
        break;
    }
    case OM_UPDRAFT:
        start_split(t);
        return;
    default: break;
    }
    fg.au = AU_MENU;
}

static void menu_pick(void) {
    FeudMatch *m = &fg.m;
    const FeudOffer *o;
    int n = feud_offers_of(m, 0, fg.act_id, &o);
    if (fg.msel < 0 || fg.msel >= n) return;
    const FeudOffer *x = &o[fg.msel];
    char b[96];
    switch (FEUD_ACT[fg.act_id].kind) {
    case AK_ENLIST:
        if (!feud_enlist(m, 0, fg.act_id, fg.cur, fg.msel)) { feud_sfx("ui_error"); return; }
        snprintf(b, sizeof b, "A %s JOINS THE FLOCK", FEUD_UNIT[x->what].name);
        feud_say(b);
        feud_sfx("fd_buy");
        break;
    case AK_BUILD:
        if (!feud_build(m, 0, fg.act_id, fg.cur, fg.msel)) { feud_sfx("ui_error"); return; }
        snprintf(b, sizeof b, "A %s GOES UP", FEUD_BLD_NAME[x->what]);
        feud_say(b);
        feud_sfx("fd_build");
        if (!feud_can_build_on(m, 0, fg.cur)) { fg.au = AU_MAP; first_valid(); }
        break;
    case AK_BARTER:
        if (!feud_trade(m, 0, fg.act_id, fg.msel)) { feud_sfx("ui_error"); return; }
        snprintf(b, sizeof b, "TRADED %d %s FOR %d %s", x->cost, FEUD_RES_NAME[x->what], x->gain, FEUD_RES_NAME[x->res]);
        feud_say(b);
        feud_sfx("fd_buy");
        break;
    case AK_OMEN:
        if (m->s[0].res[FR_RELICS] < x->cost) { feud_sfx("ui_error"); return; }
        fg.omen = x->what;
        fg.omen_cost = x->cost;
        fg.tgt_stage = 0;
        feud_sfx("ui_ok");
        if (fg.omen == OM_TITHE) { fg.au = AU_ANC; fg.anc_sel = 0; return; }
        if (!feud_omen_targets(m, 0, fg.omen)) { feud_sfx("ui_error"); fg.omen = -1; return; }
        fg.au = AU_TARGET;
        {
            uint32_t v = feud_omen_targets(m, 0, fg.omen);
            if (!(v >> fg.cur & 1))
                for (int t = 0; t < FD_TILES; t++)
                    if (v >> t & 1) { fg.cur = t; break; }
        }
        break;
    default: break;
    }
}

static void act_update(void) {
    FeudMatch *m = &fg.m;
    int kind = FEUD_ACT[fg.act_id].kind;
    switch (fg.au) {
    case AU_CONFIRM:
        if (btnp(BTN_A) && fg.mp_t > 6) {
            char b[96];
            if (kind == AK_GATHER) {
                int got[FR_KINDS];
                feud_gather(m, 0, fg.act_id, got);
                snprintf(b, sizeof b, "GATHERED +%d GRAIN +%d TOILERS +%d RELICS", got[0], got[1], got[2]);
            } else {
                int got[FU_KINDS];
                feud_callup(m, 0, fg.act_id, got);
                snprintf(b, sizeof b, "CALLED UP %d STOMPERS %d LANCERS %d MYSTICS %d SWIFTS", got[FU_STOMPER], got[FU_LANCER],
                         got[FU_MYSTIC], got[FU_SWIFT]);
            }
            feud_say(b);
            feud_sfx("fd_gather");
            next_action();
        }
        break;
    case AU_MAP:
        move_cursor();
        if (btnp(BTN_B)) { feud_sfx("ui_back"); next_action(); return; }
        if (btnp(BTN_A)) {
            if (!(feud_valid_tiles() >> fg.cur & 1)) { feud_sfx("ui_error"); return; }
            feud_sfx("ui_ok");
            if (kind == AK_MARCH) start_split(fg.cur);
            else { fg.au = AU_MENU; fg.msel = 0; }
        }
        break;
    case AU_MENU: {
        int n = feud_menu_count();
        if (btn_repeat(BTN_UP) && n) { fg.msel = (fg.msel + n - 1) % n; feud_sfx("ui_move"); }
        if (btn_repeat(BTN_DOWN) && n) { fg.msel = (fg.msel + 1) % n; feud_sfx("ui_move"); }
        if (btnp(BTN_B)) {
            feud_sfx("ui_back");
            if (kind == AK_ENLIST || kind == AK_BUILD) fg.au = AU_MAP;
            else next_action();
            return;
        }
        if (btnp(BTN_A)) menu_pick();
        break;
    }
    case AU_ANC:
        if (btn_repeat(BTN_LEFT)) { fg.anc_sel = (fg.anc_sel + FD_ANC - 1) % FD_ANC; feud_sfx("ui_move"); }
        if (btn_repeat(BTN_RIGHT)) { fg.anc_sel = (fg.anc_sel + 1) % FD_ANC; feud_sfx("ui_move"); }
        if (btnp(BTN_B)) { fg.au = AU_MENU; feud_sfx("ui_back"); return; }
        if (btnp(BTN_A)) {
            m->s[0].res[FR_RELICS] -= fg.omen_cost;
            feud_sfx("fd_pray");
            char b[96];
            snprintf(b, sizeof b, "TITHE: %s'S FAVOUR GROWS", FEUD_ANC[fg.anc_sel].name);
            feud_say(b);
            fg.au = AU_MENU;
            if (feud_add_favour(m, 0, fg.anc_sel)) {
                fg.bless_anc = fg.anc_sel;
                fg.bless_sel = 0;
                fg.tithe_bless = 1;
                fg.mp = MP_BLESS;
                fg.mp_t = 0;
            }
        }
        break;
    case AU_TARGET:
        move_cursor();
        if (btnp(BTN_B)) {
            feud_sfx("ui_back");
            if (fg.omen == OM_SHIFT && fg.tgt_stage == 1) fg.tgt_stage = 0;
            else fg.au = AU_MENU;
            return;
        }
        if (btnp(BTN_A)) {
            if (!(feud_valid_tiles() >> fg.cur & 1)) { feud_sfx("ui_error"); return; }
            apply_target();
        }
        break;
    case AU_SPLIT: {
        int rows[FU_KINDS], nr = 0, at = 0;
        for (int k = 0; k < FU_KINDS; k++)
            if (feud_movable(m, fg.src, k)) { if (k == fg.split_sel) at = nr; rows[nr++] = k; }
        if (!nr) { fg.au = AU_MAP; return; }
        if (btn_repeat(BTN_UP)) { at = (at + nr - 1) % nr; feud_sfx("ui_move"); }
        if (btn_repeat(BTN_DOWN)) { at = (at + 1) % nr; feud_sfx("ui_move"); }
        fg.split_sel = rows[at];
        int k = fg.split_sel, mx = feud_movable(m, fg.src, k);
        if (btn_repeat(BTN_LEFT) && fg.split[k] > 0) { fg.split[k]--; feud_sfx("ui_move"); }
        if (btn_repeat(BTN_RIGHT) && fg.split[k] < mx) { fg.split[k]++; feud_sfx("ui_move"); }
        if (btnp(BTN_B)) {
            feud_sfx("ui_back");
            fg.au = fg.omen == OM_UPDRAFT ? AU_TARGET : AU_MAP;
            return;
        }
        if (btnp(BTN_A)) {
            int tot = 0;
            for (int j = 0; j < FU_KINDS; j++) tot += fg.split[j];
            if (!tot) { feud_sfx("ui_error"); return; }
            feud_sfx("ui_ok");
            fg.au = AU_DEST;
        }
        break;
    }
    case AU_DEST:
        move_cursor();
        if (btnp(BTN_B)) { feud_sfx("ui_back"); fg.au = AU_SPLIT; fg.cur = fg.src; return; }
        if (btnp(BTN_A)) {
            if (!(feud_valid_tiles() >> fg.cur & 1)) { feud_sfx("ui_error"); return; }
            do_move();
        }
        break;
    }
}

/* the secret: LEFT RIGHT RIGHT LEFT LEFT LEFT while choosing whom to pray to */
static const int CODE[6] = {BTN_LEFT, BTN_RIGHT, BTN_RIGHT, BTN_LEFT, BTN_LEFT, BTN_LEFT};

static void pray_update(void) {
    FeudMatch *m = &fg.m;
    int n = m->noffer;
    static const int DIRS[4] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT};
    for (int d = 0; d < 4; d++) {
        if (!btnp(DIRS[d])) continue;
        if (DIRS[d] == CODE[fg.code_i]) fg.code_i++;
        else fg.code_i = DIRS[d] == CODE[0] ? 1 : 0;
        if (fg.code_i == 6) {
            fg.code_i = 0;
            fg.secret_t = 420;
            fsv.secret = 1;
            feud_save_now();
            feud_sfx("fd_secret");
        }
    }
    if (btn_repeat(BTN_LEFT)) { fg.pray_sel = (fg.pray_sel + n - 1) % n; feud_sfx("ui_move"); }
    if (btn_repeat(BTN_RIGHT)) { fg.pray_sel = (fg.pray_sel + 1) % n; feud_sfx("ui_move"); }
    if (btnp(BTN_A) && fg.mp_t > 10) {
        fg.anc = m->offer[fg.pray_sel];
        bool due = feud_pray(m, fg.anc);
        if (fsv.picks[fg.anc] < 65535) fsv.picks[fg.anc]++;
        feud_sfx("fd_pray");
        char b[96];
        snprintf(b, sizeof b, "YOU PRAY TO %s", FEUD_ANC[fg.anc].name);
        feud_say(b);
        if (due) {
            fg.bless_anc = fg.anc;
            fg.bless_sel = 0;
            fg.tithe_bless = 0;
            fg.mp = MP_BLESS;
            fg.mp_t = 0;
        } else {
            begin_actions();
        }
    }
}

static void bless_update(void) {
    FeudMatch *m = &fg.m;
    if (btn_repeat(BTN_UP)) { fg.bless_sel = (fg.bless_sel + 2) % 3; feud_sfx("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { fg.bless_sel = (fg.bless_sel + 1) % 3; feud_sfx("ui_move"); }
    if (btnp(BTN_A) && fg.mp_t > 10) {
        int act = m->deck[fg.bless_anc][fg.bless_sel];
        if (!feud_bless(m, 0, act)) { feud_sfx("ui_error"); return; }
        feud_sfx("fd_bless");
        char b[96];
        snprintf(b, sizeof b, "%s BLESSES %s", FEUD_ANC[fg.bless_anc].name, FEUD_ACT[act].name);
        feud_say(b);
        gift_check();
        if (fg.tithe_bless) {
            fg.tithe_bless = 0;
            fg.mp = MP_ACT;
            fg.au = AU_MENU;
            fg.act_id = cur_act();
        } else {
            begin_actions();
        }
    }
}

/* ---- the Slate Flock's turn ------------------------------------------------------ */

static int cpu_skill(void) { return fg.ai_level; }

static void cpu_note(const char *s) { feud_say(s); }

static void cpu_step(void) {
    FeudMatch *m = &fg.m;
    char b[96];
    if (fg.cpu_stage == 0) {
        fg.cpu_anc = feud_ai_pick_anc(m, 1, cpu_skill(), &fg.ai_rng);
        bool due = feud_pray(m, fg.cpu_anc);
        snprintf(b, sizeof b, "THE SLATE FLOCK PRAYS TO %s", FEUD_ANC[fg.cpu_anc].name);
        cpu_note(b);
        feud_sfx("fd_pray");
        if (due) {
            int act = feud_ai_pick_bless(m, 1, fg.cpu_anc);
            if (act >= 0) feud_bless(m, 1, act);
        }
        fg.cpu_stage = 1;
        fg.cpu_act = 0;
        fg.cpu_moves = 0;
        fg.cpu_wait = 70;
        return;
    }
    if (fg.cpu_act >= 3) {
        feud_end_turn(m);
        fg.mp = MP_BANNER;
        fg.mp_t = 0;
        if (!check_over()) music_play(m->ordeal ? FEUD_MUS_ORDEAL : FEUD_MUS_MAP);
        return;
    }
    int act = m->deck[fg.cpu_anc][fg.cpu_act];
    int kind = FEUD_ACT[act].kind;
    if (fg.cpu_moves == 0 && kind == AK_MARCH) feud_reset_moved(m);
    fg.hl_tile = -1;
    if (kind == AK_GATHER || kind == AK_CALLUP) {
        if (kind == AK_GATHER) feud_gather(m, 1, act, NULL);
        else feud_callup(m, 1, act, NULL);
        snprintf(b, sizeof b, "THE SLATE FLOCK: %s", FEUD_ACT[act].name);
        cpu_note(b);
        feud_sfx("fd_gather");
        fg.cpu_act++;
        fg.cpu_moves = 0;
        fg.cpu_wait = 40;
        return;
    }
    FeudStep st;
    if (fg.cpu_moves >= 30 || !feud_ai_next(m, 1, act, cpu_skill(), &st)) {
        fg.cpu_act++;
        fg.cpu_moves = 0;
        fg.cpu_wait = 10;
        return;
    }
    fg.cpu_moves++;
    fg.cpu_wait = 26;
    switch (st.type) {
    case ST_ENLIST: {
        const FeudOffer *o;
        feud_offers_of(m, 1, act, &o);
        feud_enlist(m, 1, act, st.tile, st.idx);
        fg.hl_tile = st.tile;
        if (m->fog) snprintf(b, sizeof b, "THE SLATE FLOCK ENLISTS BIRDS");
        else snprintf(b, sizeof b, "THE SLATE FLOCK ENLISTS A %s", FEUD_UNIT[o[st.idx].what].name);
        cpu_note(b);
        feud_sfx("fd_buy");
        break;
    }
    case ST_BUILD: {
        const FeudOffer *o;
        feud_offers_of(m, 1, act, &o);
        feud_build(m, 1, act, st.tile, st.idx);
        fg.hl_tile = st.tile;
        snprintf(b, sizeof b, "THE SLATE FLOCK BUILDS A %s", FEUD_BLD_NAME[o[st.idx].what]);
        cpu_note(b);
        feud_sfx("fd_build");
        break;
    }
    case ST_TRADE:
        feud_trade(m, 1, act, st.idx);
        cpu_note("THE SLATE FLOCK BARTERS");
        feud_sfx("fd_buy");
        break;
    case ST_MOVE: {
        int owner_before = m->t[st.tile2].owner;
        bool surprise = (feud_lv(m, 1, act)->mflags & MF_SURPRISE) != 0;
        fg.hl_tile = st.tile2;
        if (feud_move(m, 1, st.tile, st.tile2, st.counts) == 1) {
            cpu_note("THE SLATE FLOCK ATTACKS!");
            begin_battle(1, st.tile, st.tile2, st.counts, surprise, 1);
            return;
        }
        if (owner_before == 0) cpu_note("THE SLATE FLOCK TAKES AN EMPTY TILE OF OURS");
        else if (owner_before < 0) cpu_note("THE SLATE FLOCK CLAIMS NEW GROUND");
        else cpu_note("THE SLATE FLOCK MOVES BIRDS");
        feud_sfx("fd_step");
        check_over();
        break;
    }
    case ST_OMEN: {
        const FeudOffer *o;
        feud_offers_of(m, 1, act, &o);
        int om = o[st.idx].what;
        m->s[1].res[FR_RELICS] -= o[st.idx].cost;
        fg.hl_tile = st.tile;
        switch (om) {
        case OM_TITHE:
            if (feud_add_favour(m, 1, st.anc)) {
                int a2 = feud_ai_pick_bless(m, 1, st.anc);
                if (a2 >= 0) feud_bless(m, 1, a2);
            }
            snprintf(b, sizeof b, "THE SLATE FLOCK TITHES TO %s", FEUD_ANC[st.anc].name);
            fg.hl_tile = -1;
            feud_sfx("fd_pray");
            break;
        case OM_BOLT:
            snprintf(b, sizeof b, "THUNDERBOLT! %d OF OURS STRUCK DOWN", feud_bolt(m, st.tile));
            feud_sfx("fd_thunder");
            break;
        case OM_HAIL:
            snprintf(b, sizeof b, "HAILSTORM! %d OF OURS FROZEN", feud_hail(m, st.tile));
            feud_sfx("fd_hail");
            break;
        case OM_HATCH:
            feud_hatch(m, 1, st.tile);
            snprintf(b, sizeof b, "THE SLATE FLOCK HATCHES A BIRD");
            feud_sfx("fd_hatch");
            break;
        case OM_UPDRAFT: {
            fg.hl_tile = st.tile2;
            feud_sfx("fd_wind");
            if (feud_move(m, 1, st.tile, st.tile2, st.counts) == 1) {
                cpu_note("AN UPDRAFT CARRIES THE SLATE FLOCK INTO BATTLE!");
                begin_battle(1, st.tile, st.tile2, st.counts, false, 1);
                return;
            }
            snprintf(b, sizeof b, "AN UPDRAFT CARRIES THE SLATE FLOCK");
            check_over();
            break;
        }
        default: b[0] = 0; break;
        }
        cpu_note(b);
        break;
    }
    default: break;
    }
}

/* ---- the match, frame by frame -------------------------------------------------- */

void feud_match_update(void) {
    FeudMatch *m = &fg.m;
    fg.mp_t++;
    if (fg.msg_t > 0) fg.msg_t--;
    if (fg.secret_t > 0) fg.secret_t--;
    game_set_pausable(true);
    switch (fg.mp) {
    case MP_BANNER:
        if (check_over()) return;
        if (fg.mp_t > 70 || (fg.mp_t > 12 && btnp(BTN_A) && m->side == 0)) {
            if (m->side == 0) {
                fg.mp = MP_PRAY;
                fg.pray_sel = 0;
                fg.code_i = 0;
                if (music_playing() != FEUD_MUS_MAP && music_playing() != FEUD_MUS_ORDEAL)
                    music_play(m->ordeal ? FEUD_MUS_ORDEAL : FEUD_MUS_MAP);
            } else {
                fg.mp = MP_CPU;
                fg.cpu_stage = 0;
                fg.cpu_wait = 30;
                fg.hl_tile = -1;
            }
            fg.mp_t = 0;
            input_consume();
        }
        break;
    case MP_PRAY: pray_update(); break;
    case MP_BLESS: bless_update(); break;
    case MP_ACT: act_update(); break;
    case MP_CPU:
        if (fg.cpu_wait > 0) { fg.cpu_wait--; break; }
        cpu_step();
        break;
    default: break;
    }
}

void feud_battle_done(void) {
    FeudBattle *b = &fg.b;
    int dst = b->dst;
    feud_battle_apply(&fg.m, b, NULL);
    bool won = (b->result == BR_ATK_WIN || b->result == BR_DEF_FLED) == (b->atk == 0);
    char s[96];
    if (b->result == BR_ATK_WIN || b->result == BR_DEF_FLED)
        snprintf(s, sizeof s, "%s TAKES THE %s", FEUD_SIDE_NAME[b->atk], FEUD_TERR_NAME[fg.m.t[dst].terr]);
    else if (b->result == BR_ATK_BACK)
        snprintf(s, sizeof s, "THE %s PULLS BACK", FEUD_SIDE_NAME[b->atk]);
    else
        snprintf(s, sizeof s, "THE %s HOLDS", FEUD_SIDE_NAME[b->def]);
    feud_say(s);
    (void)won;
    fg.state = FS_MATCH;
    fg.state_t = 0;
    if (check_over()) return;
    if (fg.battle_return == 0) {
        music_play(fg.m.ordeal ? FEUD_MUS_ORDEAL : FEUD_MUS_MAP);
        if (fg.au == AU_MAP) first_valid();
    } else {
        music_play(FEUD_MUS_RIVAL);
        fg.cpu_wait = 30;
    }
}

void feud_battle_update(void) {
    FeudBattle *b = &fg.b;
    fg.state_t++;
    game_set_pausable(true);
    if (b->result == BR_NONE) {
        /* your orders, along the bottom; theirs from the planner */
        int me = 0;
        if (btn_repeat(BTN_LEFT) && b->order[me] > 0) { b->order[me]--; feud_sfx("ui_move"); }
        if (btn_repeat(BTN_RIGHT) && b->order[me] < FO_KINDS - 1) { b->order[me]++; feud_sfx("ui_move"); }
        if (b->t % 15 == 0) feud_battle_ai_order(b, 1);
    }
    feud_battle_step(b);
    if (b->result != BR_NONE && (b->end_t > 100 || (b->end_t > 20 && btnp(BTN_A)))) {
        input_consume();
        feud_battle_done();
    }
}

const char *feud_turn_hint(void) {
    if (fg.state == FS_BATTLE) return GLYPH_LEFT GLYPH_RIGHT " ORDERS";
    switch (fg.mp) {
    case MP_PRAY: return GLYPH_LEFT GLYPH_RIGHT " CHOOSE  " GLYPH_A " PRAY";
    case MP_BLESS: return GLYPH_UP GLYPH_DOWN " CHOOSE  " GLYPH_A " BLESS";
    case MP_ACT:
        switch (fg.au) {
        case AU_CONFIRM: return GLYPH_A " RECEIVE";
        case AU_MAP: return GLYPH_A " CHOOSE TILE  " GLYPH_B " DONE";
        case AU_MENU: return FEUD_ACT[fg.act_id].kind == AK_ENLIST || FEUD_ACT[fg.act_id].kind == AK_BUILD
                                 ? GLYPH_A " BUY  " GLYPH_B " BACK"
                                 : GLYPH_A " USE  " GLYPH_B " DONE";
        case AU_ANC: return GLYPH_LEFT GLYPH_RIGHT " ANCESTOR  " GLYPH_A " TITHE";
        case AU_TARGET: return GLYPH_A " TARGET  " GLYPH_B " BACK";
        case AU_SPLIT: return GLYPH_LEFT GLYPH_RIGHT " HOW MANY  " GLYPH_A " GO";
        case AU_DEST: return GLYPH_A " MOVE HERE  " GLYPH_B " BACK";
        }
        break;
    default: break;
    }
    return "";
}
