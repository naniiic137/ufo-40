/* LANDFALL - Wick lands on Yombra to find the three pieces of a star chart.
 * Cartridge 08 of UFO 40, a tribute to Planet Zoldath (UFO 50 #08).
 * See docs/games/08-landfall.md. This file: the title, landing, talking and
 * trading, death (a new planet next time), lift-off and the ending, the
 * records and goals, and the test hooks. */
#include "lfl.h"

LflGame lflg;
LflSave lsv;

#define SAVE_MAGIC 0x4C464C01u

static void set_state(int s) {
    lflg.state = s;
    lflg.state_t = 0;
}

static void save_now(void) {
    lsv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &lsv, (int)sizeof lsv);
}

static void load_save(void) {
    LflSave tmp;
    memset(&tmp, 0, sizeof tmp);
    int n = game_save_read(game_current_index(), &tmp, (int)sizeof tmp);
    if (n == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) { lsv = tmp; return; }
    memset(&lsv, 0, sizeof lsv);
    lsv.magic = SAVE_MAGIC;
}

static void goals_from_save(void) {
    if (lsv.found_map) game_award(GOAL_BEACON);
    if (lsv.wins) game_award(GOAL_SAUCER);
    if (lsv.best_time && lsv.best_time < LFL_CHERRY_FRAMES) game_award(GOAL_ALIEN);
}

static bool holding(int tool) { return lflg.pl.hold[0] == tool || lflg.pl.hold[1] == tool; }

static int area_music(void) { return lp.room[lflg.cur].area == AREA_DUN ? LFL_MUS_DUNGEON : LFL_MUS_PLANET; }

/* ------------------------------------------------------------------------ */

static void to_title(void) {
    set_state(LS_TITLE);
    game_set_pausable(false);
    music_play(LFL_MUS_TITLE);
}

static void new_run(uint32_t seed) {
    lfl_generate(seed);
    int keep_menu = lflg.menu;
    bool god = lflg.god, nomobs = lflg.no_mobs, notimer = lflg.no_timer;
    memset(&lflg, 0, sizeof lflg);
    lflg.menu = keep_menu;
    lflg.god = god;
    lflg.no_mobs = nomobs;
    lflg.no_timer = notimer;
    lflg.cur = -1;
    LflPlayer *p = &lflg.pl;
    p->hp = p->hpmax = LFL_HP_START;
    p->hold[0] = p->hold[1] = TL_NONE;
    p->face = DIR_DOWN;
    /* Wick steps out just below the ship */
    p->x = (lp.ship_tx * LFL_TILE + 16 - 5) * LFL_FX;
    p->y = ((lp.ship_ty + 2) * LFL_TILE + 3) * LFL_FX;
    lfl_enter_room(lp.ship_room, false);
    lfl_bot_reset();
    if (lsv.runs < 65535) lsv.runs++;
    save_now();
}

static void start_landing(void) {
    new_run(rng_next(&g_rng));
    set_state(LS_LANDING);
    game_set_pausable(true);
    music_play(LFL_MUS_LAND);
}

void lfl_win(void) {
    lflg.won = true;
    if (lsv.wins < 65535) lsv.wins++;
    if (!lsv.best_time || lflg.run_t < lsv.best_time) lsv.best_time = lflg.run_t;
    game_award(GOAL_SAUCER);
    if (lflg.run_t < LFL_CHERRY_FRAMES) game_award(GOAL_ALIEN);
    save_now();
    set_state(LS_LIFTOFF);
    music_play(LFL_MUS_LIFTOFF);
    lfl_sfx("lfl_liftoff");
}

/* ------------------------------------------------------------------------ */
/* talking and trading                                                      */

static const char *const CHATTER_A[] = {
    "SKY FOLK! YOU FELL FROM THE STARS?",
    "THE BIG SHELLS WILL GO ANYWHERE FOR A CHOP.",
    "BOG WATER BITES. MIND YOUR FEET.",
    "THE BLUE STONES SLEEP UNTIL THEY HEAR A THUMP.",
    "WE COUNT IN BEADS AND BURRS AND FERNS.",
    "THE CRYSTALS BITE BACK WHEN THEY BURST.",
    "SOMETHING HUMS UNDER THE GROUND.",
    "THE WILD ONES CHANGE WITH EVERY MOON.",
    "A LITTLE ONE MAY SLIP WHERE A BIG ONE CANNOT.",
    "WE NEVER GO NEAR THE STAIRS.",
};
static const char *const CHATTER_B[] = {
    "THE STARS ARE LOUD TONIGHT.",
    "WE TRADE FAIR. ALWAYS FAIR.",
    "OUR WORDS ARE OLD WORDS.",
    "WHAT FALLS FROM THE SKY BELONGS TO THE SKY.",
    "THE GNATS LOVE A COOKED CHOP.",
    "A BLAST IN THE BOG OPENS A DOOR.",
    "THE GROUND REMEMBERS EVERY STEP.",
    "WELCOME, WALKER.",
};

static void talk_set(int line, const char *s) {
    snprintf(lflg.talk[line], sizeof lflg.talk[line], "%s", s);
    if (line + 1 > lflg.talk_lines) lflg.talk_lines = line + 1;
}

/* who = a creature, or -1 for the boots' and talk box's own words */
void lfl_start_talk(int who) {
    memset(lflg.talk, 0, sizeof lflg.talk);
    lflg.talk_lines = 0;
    lflg.talk_trade = -1;
    lflg.talk_glyph = 0;
    lflg.talk_t = 0;
    if (who < 0) {
        if (lflg.talk_kind == 2) {
            talk_set(0, "BOG BOOTS: THE BOG CANNOT HURT YOU");
            talk_set(1, "WHILE YOU CARRY THEM.");
        } else {
            talk_set(0, "TALK BOX: WHILE YOU CARRY IT, THE");
            talk_set(1, "GLYPH TONGUE READS PLAINLY.");
        }
        set_state(LS_TALK);
        return;
    }
    LflMob *m = &lflg.mob[who];
    const LflSpecies *s = &lp.sp[m->sp];
    lflg.talk_glyph = s->glyph && !holding(TL_TALKBOX);
    if (m->flags & MF_MUTE) {
        lflg.talk_glyph = 2; /* no box can read this one */
        talk_set(0, "ONE DAY I WILL FLY TOO");
    } else if (m->flags & MF_GIVER) {
        const LflRoom *r = &lp.room[lflg.cur];
        const LflVillage *v = &lp.village[r->village - 1];
        if (v->trade < 0) {
            talk_set(0, "A SECRET, WALKER:");
            talk_set(1, lfl_hint_line(0));
            talk_set(2, lfl_hint_line(1));
        } else {
            const LflTrade *t = &lp.trade[v->trade];
            char buf[48];
            lflg.talk_trade = v->trade;
            if (t->kind == TR_RES) {
                snprintf(buf, sizeof buf, "%d %s FOR %d %s.", t->get_n, lfl_res_name(t->get_res), t->pay_n, lfl_res_name(t->pay_res));
                talk_set(0, "I GIVE");
                talk_set(1, buf);
            } else if (t->done) {
                lflg.talk_trade = -1;
                talk_set(0, "THAT WAS MY ONLY ONE.");
            } else if (t->kind == TR_ITEM) {
                snprintf(buf, sizeof buf, "MY %s FOR %d %s.", lfl_tool_names()[t->item], t->pay_n, lfl_res_name(t->pay_res));
                talk_set(0, "I GIVE");
                talk_set(1, buf);
            } else {
                snprintf(buf, sizeof buf, "A STAR CHART PIECE FOR %d %s.", t->pay_n, lfl_res_name(t->pay_res));
                talk_set(0, "I GIVE");
                talk_set(1, buf);
            }
            if (lflg.talk_trade >= 0 && !lflg.talk_glyph) talk_set(2, GLYPH_B " TRADE   " GLYPH_A " NO THANKS");
        }
    } else {
        int line = iabs(m->line) + (int)(lp.seed % 7);
        if (s->glyph) talk_set(0, CHATTER_B[line % ARRAY_LEN(CHATTER_B)]);
        else talk_set(0, CHATTER_A[line % ARRAY_LEN(CHATTER_A)]);
    }
    m->talk_t = 1;
    set_state(LS_TALK);
    lfl_sfx("lfl_talk");
}

/* tests: every line a local can say fits the talk box (the longest
 * names and numbers in each offer) */
static int talk_audit(void) {
    int bad = 0;
    char buf[64];
    for (size_t i = 0; i < ARRAY_LEN(CHATTER_A); i++) bad += text_width(CHATTER_A[i]) > 268 || text_missing(CHATTER_A[i]);
    for (size_t i = 0; i < ARRAY_LEN(CHATTER_B); i++) bad += text_width(CHATTER_B[i]) > 268 || text_missing(CHATTER_B[i]);
    for (int r = 0; r < RS_COUNT; r++) {
        snprintf(buf, sizeof buf, "%d %s FOR %d %s.", 32, lfl_res_name(r), 16, lfl_res_name(r));
        bad += text_width(buf) > 268 || text_missing(buf);
        snprintf(buf, sizeof buf, "A STAR CHART PIECE FOR %d %s.", 32, lfl_res_name(r));
        bad += text_width(buf) > 268 || text_missing(buf);
        for (int t = 0; t < TL_COUNT; t++) {
            snprintf(buf, sizeof buf, "MY %s FOR %d %s.", lfl_tool_names()[t], 32, lfl_res_name(r));
            bad += text_width(buf) > 268 || text_missing(buf);
        }
    }
    const char *fixed[] = {"BOG BOOTS: THE BOG CANNOT HURT YOU", "WHILE YOU CARRY THEM.", "TALK BOX: WHILE YOU CARRY IT, THE",
                           "GLYPH TONGUE READS PLAINLY.", "ONE DAY I WILL FLY TOO", "A SECRET, WALKER:", "THAT WAS MY ONLY ONE.",
                           GLYPH_B " TRADE   " GLYPH_A " NO THANKS", "THEY SAY SKYWELL IS NOT HALF THIS GOOD"};
    for (size_t i = 0; i < ARRAY_LEN(fixed); i++) bad += text_width(fixed[i]) > 268 || text_missing(fixed[i]);
    const char *h;
    for (int k = 0; (h = lfl_hint_any(k)) != NULL; k++) bad += text_width(h) > 268 || text_missing(h);
    return bad;
}

static void do_trade(void) {
    LflTrade *t = &lp.trade[lflg.talk_trade];
    if (lflg.res[t->pay_res] < t->pay_n) {
        lflg.res_flash[t->pay_res] = 60;
        lfl_sfx("lfl_nope");
        return;
    }
    if (t->kind != TR_RES && t->done) return;
    lflg.res[t->pay_res] -= t->pay_n;
    if (t->kind == TR_RES) {
        lfl_add_res(t->get_res, t->get_n);
        lfl_sfx("lfl_trade");
    } else if (t->kind == TR_ITEM) {
        t->done = 1;
        /* the tool is set down in front of the giver */
        const LflVillage *v = &lp.village[lp.room[lflg.cur].village - 1];
        lfl_give_tool_drop(t->item, v->gx * LFL_TILE + 2, (v->gy + 1) * LFL_TILE + 2);
        for (int i = 0; i < lp.nthing; i++)
            if (lp.thing[i].alive && lp.thing[i].kind == TH_TOOL && lp.thing[i].sub == t->item) lp.thing[i].cd = 0;
        lfl_sfx("lfl_trade");
    } else {
        t->done = 1;
        lfl_take_map(2);
    }
    set_state(LS_PLAY);
}

static void talk_update(void) {
    lflg.talk_t++;
    if (lflg.talk_t < 8) return;
    if (btnp(BTN_B)) {
        input_consume();
        if (lflg.talk_trade >= 0 && !lflg.talk_glyph) do_trade();
        else set_state(LS_PLAY);
        return;
    }
    if (btnp(BTN_A) || btnp(BTN_START)) {
        input_consume();
        set_state(LS_PLAY);
    }
}

/* ------------------------------------------------------------------------ */

static void lfl_update(void) {
    lflg.state_t++;
    lflg.frame_t++;
    switch (lflg.state) {
    case LS_TITLE:
        game_set_pausable(false);
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if ((btnp(BTN_A) || btnp(BTN_START)) && lflg.state_t > 5) {
            input_consume();
            sfx_play_name("ui_ok");
            start_landing();
        }
        break;
    case LS_LANDING:
        game_set_pausable(true);
        if (lflg.state_t > 150) {
            set_state(LS_PLAY);
            music_play(LFL_MUS_PLANET);
        }
        break;
    case LS_PLAY:
        game_set_pausable(true);
        lfl_play_update();
        if (lflg.state == LS_PLAY && music_finished()) music_play(area_music());
        break;
    case LS_SCROLL:
        lfl_scroll_update();
        break;
    case LS_TALK:
        talk_update();
        break;
    case LS_DYING:
        if (lflg.state_t > 90) {
            if (lsv.deaths < 65535) lsv.deaths++;
            lflg.deaths++;
            save_now();
            set_state(LS_OVER);
            game_set_pausable(false);
            music_play(LFL_MUS_OVER);
        }
        break;
    case LS_OVER:
        game_set_pausable(false);
        if (lflg.state_t > 60 && (btnp(BTN_A) || btnp(BTN_START) || btnp(BTN_B))) {
            input_consume();
            to_title();
        }
        break;
    case LS_LIFTOFF:
        game_set_pausable(false);
        if (lflg.state_t > 200) {
            set_state(LS_ENDING);
            music_play(LFL_MUS_ENDING);
        }
        break;
    case LS_ENDING:
        game_set_pausable(false);
        if (lflg.state_t > 120 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            to_title();
        }
        break;
    default: break;
    }
}

/* ------------------------------------------------------------------------ */

static void lfl_load(void) {
    lfl_art_load();
    lfl_audio_load();
}

static void lfl_start(void) {
    load_save();
    memset(&lflg, 0, sizeof lflg);
    lflg.cur = -1;
    goals_from_save();
    to_title();
}

static void lfl_quit(void) { save_now(); }

static int count_things(int kind, int sub, bool alive_only) {
    int n = 0;
    for (int i = 0; i < lp.nthing; i++)
        if (lp.thing[i].kind == kind && (sub < 0 || lp.thing[i].sub == sub) && (!alive_only || lp.thing[i].alive)) n++;
    return n;
}

static int count_tiles(int tile, int room) {
    int n = 0;
    for (int r = 0; r < LFL_MAX_ROOMS; r++) {
        if (!lp.room[r].exists || (room >= 0 && r != room)) continue;
        for (int y = 0; y < LFL_RH; y++)
            for (int x = 0; x < LFL_RW; x++) n += lp.room[r].t[y][x] == tile;
    }
    return n;
}

static int find_gate_role(int role) {
    for (int g = 0; g < lp.ngate; g++)
        if (lp.gate[g].role == role) return g;
    return -1;
}

static int gate_room_neighbor(int g) {
    const LflGate *gt = &lp.gate[g];
    if (gt->nook) return -1;
    return lfl_neighbor(gt->room, gt->side);
}

bool lfl_gate_open(int g) {
    const LflGate *gt = &lp.gate[g];
    if (gt->nook) {
        /* a nook is open once one of the tiles of its wall beside the nook
         * is gone (a corner gone opens nothing) */
        for (int y = gt->ty; y < gt->ty + gt->h; y++)
            for (int x = gt->tx; x < gt->tx + gt->w; x++) {
                int t = lp.room[gt->room].t[y][x];
                if (lfl_nook_door(g, x, y) && (t == T_FLOOR || t == T_WOOD || t == T_TECHDOWN)) return true;
            }
        return false;
    }
    switch (gt->kind) {
    case GK_SNAIL: return gt->snail == 1;
    case GK_PIT: return gt->used;
    case GK_SWAMP: case GK_HOLE: return false;
    default:
        for (int y = gt->ty; y < gt->ty + gt->h; y++)
            for (int x = gt->tx; x < gt->tx + gt->w; x++) {
                int t = lp.room[gt->room].t[y][x];
                if (t == T_CRYSTAL || t == T_FOLIAGE || t == T_TECH || t == T_FLESH) return false;
            }
        return true;
    }
}

/* a whole sweep of the generator: n planets from seed s, every rule held */
static int audit_many(int from, int n, int what) {
    int bad = 0;
    int counts = 0;
    for (int k = 0; k < n; k++) {
        lfl_generate((uint32_t)(from + k));
        int a = lfl_gen_audit();
        if (what == 0 && a) { bad++; printf("  planet %d breaks rule %d\n", from + k, a); }
        if (what == 1) counts += lp.gw == 7; /* how many 7 x 4 */
        if (what == 2) counts += lp.nvillage == 7;
        if (what == 3) counts += lp.ncubes == 5;
        if (what == 4) {
            int dg = 0;
            for (int g = 0; g < lp.ngate; g++) dg += lp.gate[g].role == GR_DUN;
            counts += dg;
        }
        if (what == 5) counts = imax(counts, lp.gen_tries);
        if (what == 6) counts += lp.trapped_room != 255; /* a trapped giver */
        if (what == 7)
            for (int g = 0; g < lp.ngate; g++) counts += lp.gate[g].nook != 0; /* a gated nook in the grid */
        if (what == 8) {
            /* two real gates of the same kind */
            bool same = false;
            for (int g = 0; g < lp.ngate; g++)
                for (int h = 0; h < g; h++)
                    same |= lp.gate[g].role != GR_DUN && lp.gate[h].role != GR_DUN && lp.gate[g].kind == lp.gate[h].kind;
            counts += same;
        }
    }
    return what == 0 ? bad : counts;
}

static int lfl_query(const char *key, int *out) {
    const LflPlayer *p = &lflg.pl;
    if (!strcmp(key, "bot")) { *out = lfl_bot_buttons(); return 1; }
    if (!strcmp(key, "bot_last")) { *out = lfl_bot_last; return 1; }
    if (!strcmp(key, "bot_task")) { *out = lfl_bot_task; return 1; }
    if (!strcmp(key, "bot_need")) { *out = lfl_bot_need; return 1; }
    if (!strncmp(key, "here", 4)) {
        /* the k-th live thing in this room: kind*1000 + x/16*10 + y/16 */
        int k = atoi(key + 4), n = 0;
        *out = -1;
        for (int i = 0; i < lp.nthing; i++)
            if (lp.thing[i].alive && lp.thing[i].room == lflg.cur && n++ == k) *out = lp.thing[i].kind * 1000 + lp.thing[i].x / 16 * 10 + lp.thing[i].y / 16 + lp.thing[i].cd * 100000;
        return 1;
    }
    if (!strncmp(key, "death_by", 8)) { *out = lfl_death_cause[iclamp(atoi(key + 8), 0, 7)]; return 1; }
    if (!strcmp(key, "bot_tx")) { *out = lfl_bot_tx; return 1; }
    if (!strcmp(key, "bot_ty")) { *out = lfl_bot_ty; return 1; }
    if (!strcmp(key, "wx")) { *out = lp.room[lflg.cur].gx * LFL_RW + (LFL_PX(lflg.pl.x) + 5) / LFL_TILE; return 1; }
    if (!strcmp(key, "wy")) { *out = lp.room[lflg.cur].gy * LFL_RH + (LFL_PX(lflg.pl.y) + 5) / LFL_TILE; return 1; }
    if (!strcmp(key, "state")) { *out = lflg.state; return 1; }
    if (!strcmp(key, "room")) { *out = lflg.cur; return 1; }
    if (!strcmp(key, "room_kind")) { *out = lflg.cur >= 0 ? lp.room[lflg.cur].kind : -1; return 1; }
    if (!strcmp(key, "area")) { *out = lflg.cur >= 0 ? lp.room[lflg.cur].area : -1; return 1; }
    if (!strcmp(key, "gx")) { *out = lp.room[lflg.cur].gx; return 1; }
    if (!strcmp(key, "gy")) { *out = lp.room[lflg.cur].gy; return 1; }
    if (!strcmp(key, "px")) { *out = LFL_PX(p->x); return 1; }
    if (!strcmp(key, "py")) { *out = LFL_PX(p->y); return 1; }
    if (!strcmp(key, "px256")) { *out = p->x; return 1; }
    if (!strcmp(key, "face")) { *out = p->face; return 1; }
    if (!strcmp(key, "hp")) { *out = p->hp; return 1; }
    if (!strcmp(key, "hpmax")) { *out = p->hpmax; return 1; }
    if (!strcmp(key, "inv")) { *out = p->inv; return 1; }
    if (!strcmp(key, "small")) { *out = p->shrink_t > 0; return 1; }
    if (!strcmp(key, "hold0")) { *out = p->hold[0]; return 1; }
    if (!strcmp(key, "hold1")) { *out = p->hold[1]; return 1; }
    if (!strcmp(key, "active")) { *out = p->hold[p->act]; return 1; }
    if (!strcmp(key, "act")) { *out = p->act; return 1; }
    if (!strncmp(key, "res", 3) && isdigit((unsigned char)key[3])) { *out = lflg.res[iclamp(atoi(key + 3), 0, 3)]; return 1; }
    if (!strncmp(key, "flash", 5) && isdigit((unsigned char)key[5])) { *out = lflg.res_flash[iclamp(atoi(key + 5), 0, 3)]; return 1; }
    if (!strncmp(key, "tier_of", 7)) { *out = lp.tier_of[iclamp(atoi(key + 7), 0, 3)]; return 1; }
    if (!strncmp(key, "res_of_tier", 11)) { *out = lp.res_of_tier[iclamp(atoi(key + 11), 0, 3)]; return 1; }
    if (!strncmp(key, "cost", 4) && isdigit((unsigned char)key[4])) { *out = lp.cost_res[iclamp(atoi(key + 4), 0, TL_COUNT - 1)]; return 1; }
    if (!strcmp(key, "maps")) { *out = lflg.maps; return 1; }
    if (!strcmp(key, "map_from")) { *out = lflg.map_from; return 1; }
    if (!strcmp(key, "time")) { *out = (int)lflg.run_t; return 1; }
    if (!strcmp(key, "time_s")) { *out = (int)(lflg.run_t / 60); return 1; }
    if (!strcmp(key, "won")) { *out = lflg.won; return 1; }
    if (!strcmp(key, "cubes_found")) { *out = lflg.cubes_found; return 1; }
    if (!strcmp(key, "items_found")) { *out = lflg.items_found; return 1; }
    if (!strcmp(key, "kills")) { *out = lflg.kills; return 1; }
    if (!strcmp(key, "hits")) { *out = lflg.hits_taken; return 1; }
    if (!strcmp(key, "last_hurt")) { *out = lflg.last_hurt; return 1; }
    if (!strcmp(key, "warps")) { *out = lflg.warps_taken; return 1; }
    if (!strcmp(key, "gnats_called")) { *out = lflg.mosquitos; return 1; }
    if (!strcmp(key, "talk_lines")) { *out = lflg.talk_lines; return 1; }
    if (!strcmp(key, "talk_glyph")) { *out = lflg.talk_glyph; return 1; }
    if (!strcmp(key, "talk_trade")) { *out = lflg.talk_trade; return 1; }
    if (!strcmp(key, "msg")) { *out = lflg.msg_t > 0; return 1; }
    if (!strcmp(key, "seed")) { *out = (int)lp.seed; return 1; }
    if (!strcmp(key, "gw")) { *out = lp.gw; return 1; }
    if (!strcmp(key, "gh")) { *out = lp.gh; return 1; }
    if (!strcmp(key, "villages")) { *out = lp.nvillage; return 1; }
    if (!strcmp(key, "ncubes")) { *out = lp.ncubes; return 1; }
    if (!strcmp(key, "ship_room")) { *out = lp.ship_room; return 1; }
    if (!strcmp(key, "audit")) { *out = lfl_gen_audit(); return 1; }
    if (!strcmp(key, "winnable")) { *out = lfl_winnable(); return 1; }
    if (!strcmp(key, "all_tools")) { *out = lfl_all_tools(); return 1; }
    if (!strcmp(key, "gates")) { *out = lp.ngate; return 1; }
    if (!strcmp(key, "secret_key")) { *out = lp.secret_key; return 1; }
    if (!strcmp(key, "trapped_room")) { *out = lp.trapped_room; return 1; }
    if (!strcmp(key, "secret_x")) { *out = lp.room[lp.secret_room].secret_x; return 1; }
    if (!strcmp(key, "secret_y")) { *out = lp.room[lp.secret_room].secret_y; return 1; }
    if (!strcmp(key, "secret_look")) { *out = lp.room[lp.secret_room].tmpl; return 1; }
    if (!strcmp(key, "feet_bog")) { *out = lfl_feet_in_bog(); return 1; }
    if (!strcmp(key, "secret_room")) { *out = lp.secret_room; return 1; }
    if (!strcmp(key, "secret_open")) {
        const LflRoom *r = &lp.room[lp.secret_room];
        *out = r->t[(int)r->secret_y][(int)r->secret_x] != T_SECRET;
        return 1;
    }
    if (!strncmp(key, "gate_role", 9)) { *out = find_gate_role(atoi(key + 9)); return 1; }
    if (!strncmp(key, "gate", 4) && isdigit((unsigned char)key[4])) {
        int g = atoi(key + 4);
        const char *u = strchr(key + 4, '_');
        if (g < 0 || g >= lp.ngate || !u) return 0;
        const LflGate *gt = &lp.gate[g];
        if (!strcmp(u, "_kind")) { *out = gt->kind; return 1; }
        if (!strcmp(u, "_role")) { *out = gt->role; return 1; }
        if (!strcmp(u, "_room")) { *out = gt->room; return 1; }
        if (!strcmp(u, "_side")) { *out = gt->side; return 1; }
        if (!strcmp(u, "_open")) { *out = lfl_gate_open(g); return 1; }
        if (!strcmp(u, "_snail")) { *out = gt->snail; return 1; }
        if (!strcmp(u, "_outer")) { *out = gate_room_neighbor(g); return 1; }
        if (!strcmp(u, "_tx")) { *out = gt->tx; return 1; }
        if (!strcmp(u, "_nook")) { *out = gt->nook; return 1; }
        if (!strcmp(u, "_ty")) { *out = gt->ty; return 1; }
        return 0;
    }
    if (!strncmp(key, "trade", 5) && isdigit((unsigned char)key[5])) {
        int t = iclamp(atoi(key + 5), 0, 5);
        const char *u = strchr(key + 5, '_');
        if (!u) return 0;
        const LflTrade *tr = &lp.trade[t];
        if (!strcmp(u, "_kind")) { *out = tr->kind; return 1; }
        if (!strcmp(u, "_pay")) { *out = tr->pay_res; return 1; }
        if (!strcmp(u, "_payn")) { *out = tr->pay_n; return 1; }
        if (!strcmp(u, "_get")) { *out = tr->get_res; return 1; }
        if (!strcmp(u, "_getn")) { *out = tr->get_n; return 1; }
        if (!strcmp(u, "_item")) { *out = tr->item; return 1; }
        if (!strcmp(u, "_done")) { *out = tr->done; return 1; }
        if (!strcmp(u, "_glyph")) { *out = tr->glyph; return 1; }
        if (!strcmp(u, "_room")) { *out = tr->room; return 1; }
        return 0;
    }
    if (!strncmp(key, "vil", 3) && isdigit((unsigned char)key[3])) {
        int v = iclamp(atoi(key + 3), 0, 6);
        const char *u = strchr(key + 3, '_');
        if (!u) return 0;
        const LflVillage *vl = &lp.village[v];
        if (!strcmp(u, "_room")) { *out = vl->room; return 1; }
        if (!strcmp(u, "_gx")) { *out = vl->gx; return 1; }
        if (!strcmp(u, "_gy")) { *out = vl->gy; return 1; }
        if (!strcmp(u, "_trade")) { *out = vl->trade; return 1; }
        return 0;
    }
    if (!strcmp(key, "mute_room")) { *out = lp.mute_room; return 1; }
    if (!strncmp(key, "where", 5) && isdigit((unsigned char)key[5])) { *out = lp.item_where[iclamp(atoi(key + 5), 0, 7)]; return 1; }
    if (!strncmp(key, "sp", 2) && isdigit((unsigned char)key[2])) {
        int s = iclamp(atoi(key + 2), 0, SP_COUNT - 1);
        const char *u = strchr(key + 2, '_');
        if (!u) return 0;
        const LflSpecies *sp = &lp.sp[s];
        if (!strcmp(u, "_friendly")) { *out = sp->friendly; return 1; }
        if (!strcmp(u, "_glyph")) { *out = sp->glyph; return 1; }
        if (!strcmp(u, "_move")) { *out = sp->move; return 1; }
        if (!strcmp(u, "_attack")) { *out = sp->attack; return 1; }
        if (!strcmp(u, "_hp")) { *out = sp->hp; return 1; }
        if (!strcmp(u, "_dmg")) { *out = sp->dmg; return 1; }
        if (!strcmp(u, "_death")) { *out = sp->death; return 1; }
        if (!strcmp(u, "_drop")) { *out = sp->drop; return 1; }
        if (!strcmp(u, "_look")) { *out = sp->look; return 1; }
        return 0;
    }
    if (!strcmp(key, "mobs")) { *out = lfl_count_mobs(0, 0); return 1; }
    if (!strcmp(key, "hostiles")) { *out = lfl_count_mobs(0, MF_FRIENDLY | MF_SHELL); return 1; }
    if (!strcmp(key, "friendlies")) { *out = lfl_count_mobs(MF_FRIENDLY, 0); return 1; }
    if (!strcmp(key, "morphed")) { *out = lfl_count_mobs(MF_MORPHED, 0); return 1; }
    if (!strcmp(key, "shells")) { *out = lfl_count_mobs(MF_SHELL, 0); return 1; }
    if (!strcmp(key, "angry")) { *out = lfl_count_mobs(MF_ANGRY, 0); return 1; }
    if (!strcmp(key, "gnats")) {
        int n = 0;
        for (int i = 0; i < LFL_MAX_MOBS; i++) n += lflg.mob[i].alive && lflg.mob[i].sp == SP_GNAT;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "mob", 3) && isdigit((unsigned char)key[3])) {
        int i = atoi(key + 3);
        const char *u = strchr(key + 3, '_');
        if (i < 0 || i >= LFL_MAX_MOBS || !u) return 0;
        const LflMob *m = &lflg.mob[i];
        if (!strcmp(u, "_alive")) { *out = m->alive; return 1; }
        if (!strcmp(u, "_sp")) { *out = m->sp; return 1; }
        if (!strcmp(u, "_hp")) { *out = m->hp; return 1; }
        if (!strcmp(u, "_x")) { *out = LFL_PX(m->x); return 1; }
        if (!strcmp(u, "_y")) { *out = LFL_PX(m->y); return 1; }
        if (!strcmp(u, "_face")) { *out = m->face; return 1; }
        if (!strcmp(u, "_flags")) { *out = m->flags; return 1; }
        return 0;
    }
    if (!strncmp(key, "drop", 4) && isdigit((unsigned char)key[4])) {
        /* the i-th live drop: kind*10000 + sub*1000 + x/16*10 + y/16 (tests) */
        int k = atoi(key + 4), n = 0;
        *out = -1;
        for (int i = 0; i < LFL_MAX_DROPS; i++)
            if (lflg.drop[i].alive && n++ == k)
                *out = lflg.drop[i].kind * 10000 + lflg.drop[i].sub * 1000 + LFL_PX(lflg.drop[i].x) / 16 * 10 + LFL_PX(lflg.drop[i].y) / 16;
        return 1;
    }
    if (!strcmp(key, "drops")) { int n = 0; for (int i = 0; i < LFL_MAX_DROPS; i++) n += lflg.drop[i].alive; *out = n; return 1; }
    if (!strcmp(key, "chops")) { int n = 0; for (int i = 0; i < LFL_MAX_DROPS; i++) n += lflg.drop[i].alive && lflg.drop[i].kind == DR_CHOP; *out = n; return 1; }
    if (!strcmp(key, "cooked")) { int n = 0; for (int i = 0; i < LFL_MAX_DROPS; i++) n += lflg.drop[i].alive && lflg.drop[i].kind == DR_CHOP && lflg.drop[i].cooked; *out = n; return 1; }
    if (!strncmp(key, "drops_res", 9)) {
        int r = atoi(key + 9), n = 0;
        for (int i = 0; i < LFL_MAX_DROPS; i++) n += lflg.drop[i].alive && lflg.drop[i].kind == DR_RES && lflg.drop[i].sub == r;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "shots")) { int n = 0; for (int i = 0; i < LFL_MAX_SHOTS; i++) n += lflg.shot[i].alive && lflg.shot[i].kind != SH_FOE && lflg.shot[i].kind != SH_SHARD; *out = n; return 1; }
    if (!strcmp(key, "foe_shots")) { int n = 0; for (int i = 0; i < LFL_MAX_SHOTS; i++) n += lflg.shot[i].alive && lflg.shot[i].kind == SH_FOE; *out = n; return 1; }
    if (!strcmp(key, "shards")) { int n = 0; for (int i = 0; i < LFL_MAX_SHOTS; i++) n += lflg.shot[i].alive && lflg.shot[i].kind == SH_SHARD; *out = n; return 1; }
    if (!strcmp(key, "bombs")) { int n = 0; for (int i = 0; i < LFL_MAX_BOMBS; i++) n += lflg.bomb[i].alive; *out = n; return 1; }
    if (!strncmp(key, "tiles", 5) && isdigit((unsigned char)key[5])) { *out = count_tiles(atoi(key + 5), lflg.cur); return 1; }
    if (!strncmp(key, "alltiles", 8)) { *out = count_tiles(atoi(key + 8), -1); return 1; }
    if (!strncmp(key, "tile_", 5)) {
        int x = 0, y = 0;
        if (sscanf(key + 5, "%d_%d", &x, &y) == 2) { *out = lfl_tile(lflg.cur, x, y); return 1; }
        return 0;
    }
    if (!strncmp(key, "things", 6) && isdigit((unsigned char)key[6])) { *out = count_things(atoi(key + 6), -1, true); return 1; }
    if (!strncmp(key, "tool_at_", 8)) {
        /* where tool n lies: its room, or -1 if held / gone */
        int t = atoi(key + 8);
        *out = -1;
        for (int i = 0; i < lp.nthing; i++)
            if (lp.thing[i].alive && lp.thing[i].kind == TH_TOOL && lp.thing[i].sub == t) *out = lp.thing[i].room;
        return 1;
    }
    if (!strcmp(key, "neighbors")) {
        int n = 0;
        for (int d = 0; d < 4; d++) n += lfl_neighbor(lflg.cur, d) >= 0;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "nb", 2) && isdigit((unsigned char)key[2])) { *out = lfl_neighbor(lflg.cur, atoi(key + 2) & 3); return 1; }
    if (!strncmp(key, "dun", 3) && isdigit((unsigned char)key[3])) { *out = lp.dun_room[iclamp(atoi(key + 3), 0, LFL_DUN_ROOMS - 1)]; return 1; }
    if (!strcmp(key, "dun_step")) { *out = lp.room[lflg.cur].dun_step; return 1; }
    if (!strcmp(key, "dun_entry")) { *out = lp.dun_entry_room; return 1; }
    if (!strcmp(key, "spawns_taken")) {
        int n = 0;
        for (int i = 0; i < lp.room[lflg.cur].nspawn; i++) n += lp.room[lflg.cur].spawn[i].taken;
        *out = n;
        return 1;
    }
    if (!strncmp(key, "room_dirty", 10)) { *out = lp.room[iclamp(atoi(key + 10), 0, LFL_MAX_ROOMS - 1)].dirty; return 1; }
    if (!strcmp(key, "warp_rooms")) { *out = (lp.warp_room[0] >= 0) + (lp.warp_room[1] >= 0); return 1; }
    if (!strcmp(key, "art_bad")) { *out = lfl_art_check(); return 1; }
    if (!strcmp(key, "layout")) { *out = lfl_layout_audit() + talk_audit(); return 1; }
    if (!strncmp(key, "audit_many_", 11)) {
        int from = 0, n = 0, what = 0;
        if (sscanf(key + 11, "%d_%d_%d", &from, &n, &what) < 2) return 0;
        uint32_t keep = lp.seed;
        *out = audit_many(from, n, what);
        lfl_generate(keep);
        return 1;
    }
    if (!strcmp(key, "save_runs")) { *out = lsv.runs; return 1; }
    if (!strcmp(key, "save_wins")) { *out = lsv.wins; return 1; }
    if (!strcmp(key, "save_deaths")) { *out = lsv.deaths; return 1; }
    if (!strcmp(key, "save_items")) { *out = lsv.most_items; return 1; }
    if (!strcmp(key, "save_energy")) { *out = lsv.most_energy; return 1; }
    if (!strcmp(key, "save_best")) { *out = (int)lsv.best_time; return 1; }
    if (!strcmp(key, "save_map")) { *out = lsv.found_map; return 1; }
    return 0;
}

static int lfl_cheat(const char *cmd) {
    int a, b, c;
    if (sscanf(cmd, "planet %d", &a) == 1) {
        /* straight onto planet a, past the landing */
        new_run((uint32_t)a);
        set_state(LS_PLAY);
        game_set_pausable(true);
        music_play(LFL_MUS_PLANET);
        return 1;
    }
    if (sscanf(cmd, "gen %d", &a) == 1) { lfl_generate((uint32_t)a); return 1; }
    if (sscanf(cmd, "room %d %d %d", &a, &b, &c) == 3) {
        /* room a, Wick at pixel (b, c) */
        lflg.pl.x = b * LFL_FX;
        lflg.pl.y = c * LFL_FX;
        lfl_enter_room(iclamp(a, 0, LFL_MAX_ROOMS - 1), true);
        set_state(LS_PLAY);
        return 1;
    }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) { lflg.pl.x = a * LFL_FX; lflg.pl.y = b * LFL_FX; return 1; }
    if (sscanf(cmd, "hold %d %d", &a, &b) == 2) { lflg.pl.hold[0] = (int8_t)a; lflg.pl.hold[1] = (int8_t)b; lflg.pl.act = 0; return 1; }
    if (sscanf(cmd, "res %d %d", &a, &b) == 2) { lflg.res[iclamp(a, 0, 3)] = iclamp(b, 0, LFL_RES_CAP); return 1; }
    if (sscanf(cmd, "hp %d %d", &a, &b) == 2) { lflg.pl.hp = a; lflg.pl.hpmax = b; return 1; }
    if (sscanf(cmd, "maps %d", &a) == 1) { lflg.maps = a; return 1; }
    if (sscanf(cmd, "face %d", &a) == 1) { lflg.pl.face = a & 3; return 1; }
    if (sscanf(cmd, "time %d", &a) == 1) { lflg.run_t = (uint32_t)a; return 1; }
    if (sscanf(cmd, "tile %d %d %d", &a, &b, &c) == 3) { lp.room[lflg.cur].t[iclamp(b, 0, LFL_RH - 1)][iclamp(a, 0, LFL_RW - 1)] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "mob %d %d %d", &a, &b, &c) == 3) { lfl_spawn_mob(iclamp(a, 0, SP_COUNT - 1), b, c, -1); return 1; }
    if (sscanf(cmd, "species %d %d %d", &a, &b, &c) == 3) {
        /* species a: movement b, attack c */
        lp.sp[iclamp(a, 0, SP_COUNT - 1)].move = (uint8_t)b;
        lp.sp[iclamp(a, 0, SP_COUNT - 1)].attack = (uint8_t)c;
        return 1;
    }
    {
        int s, hp, dmg, death;
        if (sscanf(cmd, "spset %d %d %d %d", &s, &hp, &dmg, &death) == 4) {
            LflSpecies *sp = &lp.sp[iclamp(s, 0, SP_COUNT - 1)];
            sp->hp = (uint8_t)hp;
            sp->dmg = (uint8_t)dmg;
            sp->death = (uint8_t)death;
            return 1;
        }
    }
    if (sscanf(cmd, "drop %d %d %d", &a, &b, &c) == 3) {
        /* a resource (a >= 0) or a chop (a = -1) at (b, c) */
        for (int i = 0; i < LFL_MAX_DROPS; i++)
            if (!lflg.drop[i].alive) {
                LflDrop *d = &lflg.drop[i];
                memset(d, 0, sizeof *d);
                d->alive = 1;
                d->kind = a < 0 ? DR_CHOP : DR_RES;
                d->sub = (uint8_t)imax(0, a);
                d->x = b * LFL_FX;
                d->y = c * LFL_FX;
                d->spawn = -1;
                int t = lfl_tile(lflg.cur, (b + 4) / LFL_TILE, (c + 4) / LFL_TILE);
                d->cooked = a < 0 && (t == T_SWAMP || t == T_WARP);
                break;
            }
        return 1;
    }
    {
        int kind, sub, x, y;
        if (sscanf(cmd, "thing %d %d %d %d", &kind, &sub, &x, &y) == 4) {
            for (int i = 0; i <= lp.nthing && i < LFL_MAX_THINGS; i++)
                if (i == lp.nthing || (!lp.thing[i].alive && lp.thing[i].kind == TH_TOOL)) {
                    LflThing *t = &lp.thing[i];
                    memset(t, 0, sizeof *t);
                    t->alive = 1;
                    t->kind = (uint8_t)kind;
                    t->sub = (uint8_t)sub;
                    t->n = (uint8_t)(kind == TH_BUNDLE ? 2 : 1);
                    t->room = (uint8_t)lflg.cur;
                    t->x = (int16_t)x;
                    t->y = (int16_t)y;
                    if (i == lp.nthing) lp.nthing++;
                    break;
                }
            return 1;
        }
    }
    if (sscanf(cmd, "explode %d %d", &a, &b) == 2) { lfl_explode(a, b, 22, 2, 4, false); return 1; }
    if (!strcmp(cmd, "god")) { lflg.god = true; return 1; }
    if (!strcmp(cmd, "mortal")) { lflg.god = false; return 1; }
    if (!strcmp(cmd, "nomobs")) { lflg.no_mobs = true; return 1; }
    if (!strcmp(cmd, "mobs")) { lflg.no_mobs = false; return 1; }
    if (!strcmp(cmd, "notimer")) { lflg.no_timer = true; return 1; }
    if (!strcmp(cmd, "clear")) { memset(lflg.mob, 0, sizeof lflg.mob); memset(lflg.shot, 0, sizeof lflg.shot); return 1; }
    if (!strcmp(cmd, "clearitems")) { memset(lflg.drop, 0, sizeof lflg.drop); return 1; }
    if (!strcmp(cmd, "die")) { lflg.god = false; lflg.pl.inv = 0; lfl_hurt_player(99, 0); return 1; }
    if (!strcmp(cmd, "win")) { lfl_win(); return 1; }
    if (!strcmp(cmd, "secret")) {
        lflg.pl.x = 0;
        lfl_enter_room(lp.secret_room, true);
        return 1;
    }
    return 0;
}

const GameDef GAME_LANDFALL = {
    "landfall",
    "LANDFALL",
    "1984",
    "ADVENTURE",
    "A NEW PLANET EVERY TIME. TWO HANDS, EIGHT TOOLS, NO SECOND CHANCES.",
    {"FIND A PIECE OF THE STAR CHART", "LEAVE YOMBRA WITH THE WHOLE CHART",
     "LEAVE IN UNDER 20 MINUTES ON THE CLOCK"},
    GLYPH_DPAD "\tWALK\n"
    GLYPH_A "\tUSE THE TOOL IN HAND\n"
    "HOLD " GLYPH_A "\tKEEP FACING (STRAFE)\n"
    GLYPH_B "\tTALK OR TRADE, ELSE SWAP HANDS\n"
    "HOLD " GLYPH_B "\tSET THE TOOL DOWN\n"
    "START\tPAUSE (THE CLOCK STOPS)",
    C_TEAL, C_AMBER,
    lfl_load, lfl_start, lfl_update, lfl_draw, lfl_quit, lfl_draw_label, lfl_query, lfl_cheat,
    "PLANET ZOLDATH", 8,
    NULL,
};
