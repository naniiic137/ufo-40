/* CHIME CIRCUIT - six chime ships race eight laps round each of eight
 * one-screen tracks, for points, in a tournament for the Chime Cup.
 * Cartridge 19 of UFO 40, a tribute to The Big Bell Race (UFO 50 #19).
 * See docs/games/19-chime-circuit.md. The rules live in chime_race.c;
 * this file is the screens, the tournament, saving, the code screen, the
 * demo pilot's menu presses and the test hooks. */
#include "chime.h"

enum { S_TITLE, S_CODE, S_PAGE, S_PILOTS, S_CARD, S_RACE, S_RESULT, S_STANDINGS, S_FINAL, S_ENDING, S_CREDITS };

/* Stats and records. A tournament in progress is not kept: like the
 * original, it is played in one sitting (the pause menu is there). */
typedef struct Save {
    uint32_t magic;
    uint32_t lap_sum[2], lap_n[2];   /* players one and two: laps, in frames */
    uint32_t race_sum[2], race_n[2]; /* races finished, in frames */
    uint16_t cups, cup_wins, race_wins;
    uint8_t pilot[2];                /* the last picks */
    uint8_t best;                    /* the best tournament score */
    uint8_t pad[3];
} Save;
#define SAVE_MAGIC 0x43484D01u

static Save sv;
static ChmCup cup;
static ChmRace race;
static uint8_t track_px[SCREEN_W * SCREEN_H];
static int state, state_t, frame_t;
static int title_sel;
static int pick[2];
static bool locked[2];
static bool fast_loot;        /* the LOOT-GALE code: stations restock four times as fast */
static bool cup_no_goals;     /* a tournament begun with the code on earns no goals or stats */
static char code[9] = "AAAAAAAA";
static int code_pos;
static const char *code_msg;
static int code_msg_t;
static int prev_laps[CHM_SHIPS];
static bool race_counted;
static int over_t;
static const char *banner;
static int banner_t, banner_col;
static int gained[CHM_SHIPS];
static int champion = -1;
static int credits_t;
static int bot_players = 1;   /* the demo pilot's pick on the title (tests) */

#define CODE_LOOT "LOOTGALE"
#define CODE_PAGE "LASTLAMP"

/* ------------------------------------------------------------------ */
/* saving                                                               */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    static Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else {
        memset(&sv, 0, sizeof sv);
        sv.magic = SAVE_MAGIC;
        sv.pilot[1] = 1;
    }
}

/* ------------------------------------------------------------------ */
/* helpers                                                              */

static void goto_state(int s) {
    state = s;
    state_t = 0;
    input_consume();
}

static const char *secs(uint32_t frames) {
    static char bufs[4][16];
    static int k;
    char *b = bufs[k++ & 3];
    uint32_t cs = frames * 100u / 60u;
    snprintf(b, 16, "%lu.%02lu", (unsigned long)(cs / 100u), (unsigned long)(cs % 100u));
    return b;
}

static const char *ordinal(int n) {
    static const char *const O[] = {"--", "1ST", "2ND", "3RD", "4TH", "5TH", "6TH"};
    return O[iclamp(n, 0, 6)];
}

static int race_song(int track) {
    static const int *const S[CHM_TRACKS] = {&CHM_MUS_RACE1, &CHM_MUS_RACE2, &CHM_MUS_RACE3, &CHM_MUS_RACE1,
                                             &CHM_MUS_RACE2, &CHM_MUS_RACE3, &CHM_MUS_RACE1, &CHM_MUS_FINALE};
    return *S[track % CHM_TRACKS];
}

static const char *pilot_name(int ship) { return CHM_PILOT[cup.pilot[ship] % CHM_PILOTS].name; }

static void say_banner(const char *s, int col) {
    banner = s;
    banner_t = 90;
    banner_col = col;
}

/* ------------------------------------------------------------------ */
/* the flow                                                             */

static void start_show(void) {
    /* the title screen's race: six CPU pilots round the first track */
    ChmCup c;
    uint8_t none[2] = {0, 1};
    chm_cup_new(&c, 0, none, &g_rng);
    chm_race_begin(&race, &c, 0, (uint64_t)rng_next(&g_rng) << 16 | 19);
    race.phase = RP_RUN;
    chm_render_track(&chm_map, track_px);
}

static void title_enter(void) {
    goto_state(S_TITLE);
    game_set_pausable(false);
    input_set_versus(false);
    music_play(CHM_MUS_TITLE);
    start_show();
}

static void pilots_enter(int humans) {
    cup.humans = (uint8_t)humans;
    pick[0] = sv.pilot[0] % CHM_PILOTS;
    pick[1] = sv.pilot[1] % CHM_PILOTS;
    if (pick[1] == pick[0]) pick[1] = (pick[0] + 1) % CHM_PILOTS;
    locked[0] = locked[1] = false;
    input_set_versus(humans == 2);
    goto_state(S_PILOTS);
    music_play(CHM_MUS_PIT);
}

static void card_enter(void) {
    chm_map_build(&chm_map, cup.race % CHM_TRACKS);
    goto_state(S_CARD);
    game_set_pausable(true);
    music_play(CHM_MUS_PIT);
}

static void start_cup(void) {
    uint8_t p[2] = {(uint8_t)pick[0], (uint8_t)pick[1]};
    chm_cup_new(&cup, cup.humans, p, &g_rng);
    cup.fast_loot = fast_loot;
    cup_no_goals = fast_loot;
    sv.pilot[0] = (uint8_t)pick[0];
    if (cup.humans > 1) sv.pilot[1] = (uint8_t)pick[1];
    save_now();
    card_enter();
}

static void race_enter(void) {
    uint64_t seed = (uint64_t)rng_next(&g_rng) << 32 | rng_next(&g_rng);
    chm_race_begin(&race, &cup, cup.race % CHM_TRACKS, seed);
    chm_render_track(&chm_map, track_px);
    for (int i = 0; i < CHM_SHIPS; i++) prev_laps[i] = 0;
    race_counted = false;
    over_t = 0;
    banner = NULL;
    goto_state(S_RACE);
    game_set_pausable(true);
    music_play(race_song(race.track));
}

/* the race is over: points, the next grid, stats and goals */
static void count_race(void) {
    race_counted = true;
    chm_cup_score(&cup, &race);
    for (int i = 0; i < CHM_SHIPS; i++) gained[i] = CHM_POINTS[iclamp(race.s[i].place, 1, CHM_SHIPS) - 1];
    bool won = false;
    for (int i = 0; i < CHM_SHIPS; i++) {
        const ChmShip *s = &race.s[i];
        if (!s->human) continue;
        if (!cup_no_goals && s->finished && s->finish_t) {
            sv.race_sum[s->human - 1] += s->finish_t;
            sv.race_n[s->human - 1]++;
        }
        if (s->place == 1) {
            won = true;
            if (!cup_no_goals) {
                game_award(GOAL_BEACON);
                if (sv.race_wins < 65535) sv.race_wins++;
            }
        }
    }
    if (cup.race >= CHM_RACES) {
        champion = chm_cup_leader(&cup);
        if (!cup_no_goals) {
            if (sv.cups < 65535) sv.cups++;
            for (int i = 0; i < cup.humans; i++)
                if (cup.points[i] > sv.best) sv.best = cup.points[i];
            if (champion < cup.humans) {
                if (sv.cup_wins < 65535) sv.cup_wins++;
                game_award(GOAL_SAUCER);
                if (chm_cup_wins(&cup, champion) == CHM_RACES) game_award(GOAL_ALIEN);
            }
        }
    }
    save_now();
    music_play(won ? CHM_MUS_FLAG : CHM_MUS_ALSO);
}

static void final_enter(void) {
    goto_state(S_FINAL);
    champion = chm_cup_leader(&cup);
    music_play(champion < cup.humans ? CHM_MUS_CUP : CHM_MUS_ALSO);
}

/* ------------------------------------------------------------------ */
/* updates                                                              */

static void update_title(void) {
    race.fast_loot = 0;
    unsigned none[2] = {0, 0};
    chm_race_step(&race, none);
    if (race.phase == RP_OVER || race.t > 60 * 90) start_show();
    if (btn_repeat(BTN_UP)) { title_sel = (title_sel + 2) % 3; sfx_play_name("chm_move"); }
    if (btn_repeat(BTN_DOWN)) { title_sel = (title_sel + 1) % 3; sfx_play_name("chm_move"); }
    if (btnp(BTN_B) && state_t > 10) { game_exit_to_library(); return; }
    if (state_t > 15 && btnp(BTN_A)) {
        sfx_play_name("ui_ok");
        if (title_sel < 2) pilots_enter(title_sel + 1);
        else {
            code_pos = 0;
            code_msg = NULL;
            goto_state(S_CODE);
        }
    }
}

static void check_code(void) {
    if (!strcmp(code, CODE_LOOT)) {
        fast_loot = !fast_loot;
        code_msg = fast_loot ? "LOOT-GALE: STATIONS RESTOCK FAST" : "LOOT-GALE IS OFF";
        sfx_play_name("chm_pick");
    } else if (!strcmp(code, CODE_PAGE)) {
        sfx_play_name("ui_ok");
        goto_state(S_PAGE);
        return;
    } else {
        code_msg = "NO SUCH CODE";
        sfx_play_name("ui_error");
    }
    code_msg_t = 150;
}

static void update_code(void) {
    if (code_msg_t > 0) code_msg_t--;
    if (btn_repeat(BTN_LEFT)) { code_pos = (code_pos + 7) % 8; sfx_play_name("chm_move"); }
    if (btn_repeat(BTN_RIGHT)) { code_pos = (code_pos + 1) % 8; sfx_play_name("chm_move"); }
    if (btn_repeat(BTN_UP)) { code[code_pos] = (char)(code[code_pos] == 'Z' ? 'A' : code[code_pos] + 1); sfx_play_name("chm_move"); }
    if (btn_repeat(BTN_DOWN)) { code[code_pos] = (char)(code[code_pos] == 'A' ? 'Z' : code[code_pos] - 1); sfx_play_name("chm_move"); }
    if (btnp(BTN_B)) { sfx_play_name("ui_back"); goto_state(S_TITLE); return; }
    if (btnp(BTN_A) && state_t > 5) check_code();
}

static bool taken(int p) { return cup.humans == 2 && locked[1 - p] && pick[1 - p] == pick[p]; }

static void update_pilots(void) {
    for (int p = 0; p < cup.humans; p++) {
        bool (*rep)(int) = p ? btn_repeat2 : btn_repeat;
        bool (*pr)(int) = p ? btnp2 : btnp;
        if (!locked[p]) {
            int dx = rep(BTN_RIGHT) ? 1 : rep(BTN_LEFT) ? -1 : 0;
            if (dx) {
                do pick[p] = (pick[p] + dx + CHM_PILOTS) % CHM_PILOTS;
                while (cup.humans == 2 && locked[1 - p] && pick[p] == pick[1 - p]);
                sfx_play_name("chm_move");
            }
            if (pr(BTN_A) && state_t > 8 && !taken(p)) {
                locked[p] = true;
                sfx_play_name("ui_ok");
                /* the other player can't sit on the same pilot */
                if (cup.humans == 2 && !locked[1 - p] && pick[1 - p] == pick[p]) pick[1 - p] = (pick[p] + 1) % CHM_PILOTS;
            }
            if (pr(BTN_B) && p == 0) { sfx_play_name("ui_back"); title_enter(); return; }
        } else if (pr(BTN_B)) {
            locked[p] = false;
            sfx_play_name("ui_back");
        }
    }
    bool all = true;
    for (int p = 0; p < cup.humans; p++) all &= locked[p];
    if (all) start_cup();
}

static unsigned read_ctl(int p) {
    bool (*h)(int) = p ? btn2 : btn;
    bool (*pr)(int) = p ? btnp2 : btnp;
    unsigned c = 0;
    if (h(BTN_LEFT)) c |= CHF_LEFT;
    if (h(BTN_RIGHT)) c |= CHF_RIGHT;
    if (h(BTN_A)) c |= CHF_THRUST;
    if (pr(BTN_B)) c |= CHM_CTL_SLASH;
    return c;
}

static void race_sounds(uint32_t ev, uint32_t eh) {
    if (ev & EV_BEEP) sfx_play_name("chm_beep");
    if (ev & EV_GO) sfx_play_name("chm_go");
    if (ev & EV_WRECK) sfx_play_name("chm_wreck");
    else if (ev & EV_BLAST) sfx_play_name("chm_blast");
    else if (eh & EV_WALL) sfx_play_name("chm_wall");
    else if (ev & EV_FIRE) sfx_play_name("chm_fire");
    else if (eh & EV_SLASH) sfx_play_name("chm_slash");
    else if (ev & EV_SHOT && (frame_t & 3) == 0) sfx_play_name("chm_shot");
    if (eh & EV_LASTLAP) sfx_play_name("chm_last");
    else if (eh & EV_FINISH) sfx_play_name("chm_last");
    else if (eh & EV_LAP) sfx_play_name("chm_lap");
    else if (eh & EV_PICK) sfx_play_name("chm_pick");
    else if (eh & EV_KNOCK) sfx_play_name("chm_knock");
    else if (eh & EV_BOOST) sfx_play_name("chm_boost");
    else if (ev & EV_WARN) sfx_play_name("chm_warn");
    else if (ev & EV_LAUNCH) sfx_play_name("chm_launch");
}

static void update_race(void) {
    unsigned ctl[2] = {read_ctl(0), cup.humans > 1 ? read_ctl(1) : 0};
    chm_race_step(&race, ctl);
    race_sounds(race.ev, race.ev_human);
    for (int i = 0; i < CHM_SHIPS; i++) {
        const ChmShip *s = &race.s[i];
        if (s->laps > prev_laps[i] && s->human) {
            if (!cup_no_goals) {
                sv.lap_sum[s->human - 1] += s->last_lap;
                sv.lap_n[s->human - 1]++;
            }
            if (s->finished) say_banner(s->place == 1 ? "FINISH! 1ST!" : "FINISH!", C_YELLOW);
            else if (s->laps == CHM_LAPS - 1) say_banner("FINAL LAP", C_ORANGE);
        }
        prev_laps[i] = s->laps;
    }
    if (banner_t > 0) banner_t--;
    if (race.phase == RP_OVER) {
        if (!race_counted) count_race();
        over_t++;
        if (over_t > 240 || (over_t > 50 && btnp(BTN_A))) {
            sfx_play_name("ui_ok");
            goto_state(S_RESULT);
            music_play(CHM_MUS_PIT);
        }
    }
}

static void update(void) {
    frame_t++;
    state_t++;
    switch (state) {
    case S_TITLE: update_title(); break;
    case S_CODE: update_code(); break;
    case S_PAGE:
        if (state_t > 20 && btnp(BTN_A | BTN_B)) { sfx_play_name("ui_back"); goto_state(S_CODE); }
        break;
    case S_PILOTS: update_pilots(); break;
    case S_CARD:
        if (state_t > 20 && btnp(BTN_A)) { sfx_play_name("ui_ok"); race_enter(); }
        break;
    case S_RACE: update_race(); break;
    case S_RESULT:
        if (state_t > 20 && btnp(BTN_A)) { sfx_play_name("ui_ok"); goto_state(S_STANDINGS); }
        break;
    case S_STANDINGS:
        if (state_t > 20 && btnp(BTN_A)) {
            sfx_play_name("ui_ok");
            if (cup.race < CHM_RACES) card_enter();
            else final_enter();
        }
        break;
    case S_FINAL:
        if (state_t > 60 && btnp(BTN_A)) {
            sfx_play_name("ui_ok");
            if (champion >= 0 && champion < cup.humans) {
                goto_state(S_ENDING);
                music_play(CHM_MUS_HOME);
            } else title_enter();
        }
        break;
    case S_ENDING:
        if (state_t > 60 && btnp(BTN_A)) { sfx_play_name("ui_ok"); goto_state(S_CREDITS); credits_t = 0; }
        break;
    case S_CREDITS:
        credits_t += btn(BTN_A) ? 4 : 1;
        if (credits_t > 60 * 20 && btnp(BTN_A | BTN_B)) title_enter();
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */

static const uint8_t GRAD[] = {C_WHITE, C_ICE, C_CYAN, C_SKY};
static const uint8_t GOLD[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};

static void backdrop(void) {
    gfx_cls(C_NAVY);
    for (int i = 0; i < 50; i++) gfx_pset((i * 97 + 13) % 320, (i * 53 + 7) % 180, (frame_t / 20 + i) % 7 ? C_BLUE : C_ICE);
    gfx_dither(0, 120, SCREEN_W, 60, C_NIGHT, 6);
}

static void draw_power_bits(const ChmRace *r, const ChmShip *s, int x, int y) {
    switch (s->power) {
    case PW_FIREBALLS:
        for (int b = 0; b < 2; b++) {
            int a = s->orbit + b * 128;
            int fx = x + chm_cos(a) * 14 / 127, fy = y + chm_sin(a) * 14 / 127;
            spr_draw(&chm_spr[CA_FIREBALL], fx - 3, fy - 3, (frame_t >> 2) & 1 ? SPR_FLIPX : 0);
        }
        break;
    case PW_PAYLOAD: {
        int bx = (int)(s->ball_x >> 8), by = (int)(s->ball_y >> 8);
        for (int k = 1; k < 4; k++) gfx_pset(x + (bx - x) * k / 4, y + 4 + (by - y - 4) * k / 4, C_GREY);
        spr_draw(&chm_spr[CA_BALL], bx - 3, by - 3, 0);
        if (s->power_t < 60 && (frame_t >> 2) & 1) gfx_circb(bx, by, 5, C_RED);
        break;
    }
    case PW_SUPER:
        if ((frame_t >> 1) & 1) gfx_circb(x, y, 9, C_CYAN);
        break;
    case PW_BULLETS:
        gfx_pset(x, y - 8, C_YELLOW);
        break;
    default: break;
    }
    (void)r;
}

static void draw_slash(const ChmShip *s, int x, int y) {
    if (!s->slash_t) return;
    bool super = s->power == PW_SUPER;
    int f = s->f.face >= 0 ? 1 : -1, age = CHM_SLASH_T - s->slash_t;
    int rad = super ? 16 : 12;
    static const int8_t CS[9][2] = {{0, -10}, {4, -9}, {7, -7}, {9, -4}, {10, 0}, {9, 4}, {7, 7}, {4, 9}, {0, 10}};
    int shown = imin(9, 3 + age * 2);
    for (int k = 0; k < shown; k++) {
        int px = x + f * (CS[k][0] * rad / 10 + 3), py = y + CS[k][1] * rad / 10;
        int col = age < 6 ? C_WHITE : C_CYAN;
        gfx_rect(px - 1, py - 1, 2, 2, col);
        if (super) gfx_pset(px + f * 2, py, C_ICE);
    }
}

static void draw_ship_full(const ChmRace *r, int i) {
    const ChmShip *s = &r->s[i];
    if (!s->alive || s->finished || s->parked) return;
    int x = (int)(s->f.x >> 8), y = (int)(s->f.y >> 8);
    draw_power_bits(r, s, x, y);
    if (!(s->mercy && (frame_t >> 1) % 3 == 0))
        chm_draw_ship(s->pilot, x, y, s->f.face, (s->ctl & CHF_THRUST) && !s->stun, 0, s->hp, frame_t + i * 5);
    draw_slash(s, x, y);
    if (s->human && (r->humans > 1 || r->t < 240 || r->phase == RP_COUNTDOWN)) {
        const char *tag = s->human == 1 ? "1P" : "2P";
        int tw = tiny_width(tag);
        gfx_rect(x - tw / 2 - 1, y - 14, tw + 2, 7, C_INK);
        tiny_draw(tag, x - tw / 2, y - 13, s->human == 1 ? C_YELLOW : C_CYAN);
    }
}

static void draw_hud(const ChmRace *r) {
    gfx_rect(0, 0, 30, 42, C_INK);
    gfx_rectb(0, 0, 30, 42, C_DUSK);
    for (int k = 0; k < CHM_SHIPS; k++) {
        const ChmShip *s = &r->s[r->order[k]];
        int y = 2 + k * 7;
        char b[8];
        snprintf(b, sizeof b, "%d", k + 1);
        tiny_draw(b, 2, y, C_GREY);
        const ChmPilot *p = &CHM_PILOT[s->pilot % CHM_PILOTS];
        gfx_rect(7, y, 8, 5, p->body);
        gfx_rect(7, y + 4, 8, 1, p->trim);
        if (s->human) gfx_rectb(6, y - 1, 10, 7, s->human == 1 ? C_YELLOW : C_CYAN);
        if (!s->alive && !s->finished) gfx_line(7, y, 14, y + 4, C_INK);
        if (s->finished) spr_draw(&chm_spr[CA_FLAG], 19, y - 2, 0);
        else {
            snprintf(b, sizeof b, "%d", CHM_LAPS - s->laps);
            tiny_draw(b, 20, y, s->laps == CHM_LAPS - 1 ? C_ORANGE : C_WHITE);
        }
    }
}

static void draw_race_scene(const ChmRace *r, bool hud) {
    memcpy(g_screen.px, track_px, sizeof track_px);
    const ChmMap *m = &chm_map;
    /* boost arrows shimmer */
    for (int row = 0; row < CHM_TH; row++)
        for (int col = 0; col < CHM_TW; col++)
            if (m->boost[row][col] && ((frame_t / 6 + col + row) & 3) == 0)
                gfx_rect(col * CHM_TILE + 3, CHM_OY + row * CHM_TILE + 3, 2, 2, C_WHITE);
    /* the launcher glows before a relaunch */
    for (int i = 0; i < CHM_SHIPS; i++)
        if (!r->s[i].alive && !r->s[i].finished && r->s[i].wreck_t < 30 && (frame_t & 2))
            gfx_rect(m->launch_c * CHM_TILE + 1, CHM_OY + m->launch_r * CHM_TILE, 6, 2, C_YELLOW);
    for (int k = 0; k < r->nst; k++) {
        const ChmStation *st = &r->st[k];
        gfx_circb(st->x, st->y + 6, 5, C_SLATE);
        gfx_hline(st->x - 4, st->x + 4, st->y + 8, C_GREY);
        if (st->state == 1 && (frame_t >> 2) & 1) {
            gfx_rect(st->x - 5, st->y - 5, 11, 9, C_INK);
            text_draw("!!", st->x - 4, st->y - 4, C_YELLOW);
        } else if (st->state == 2) {
            int bob = ((frame_t >> 3) & 1);
            gfx_circ(st->x, st->y - bob, 7, C_INK);
            gfx_circb(st->x, st->y - bob, 7, (frame_t >> 2) & 1 ? C_WHITE : C_CYAN);
            spr_draw(&chm_spr[CA_ICON_BULLETS + st->kind - 1], st->x - 5, st->y - 5 - bob, 0);
        }
    }
    for (int k = 0; k < CHM_FIRES; k++)
        if (r->fire[k].on) spr_draw(&chm_spr[(frame_t >> 2) & 1 ? CA_FIRE1 : CA_FIRE2], r->fire[k].x - 4, r->fire[k].y - 6, 0);
    for (int k = 0; k < CHM_MINES; k++) {
        const ChmMine *mn = &r->mine[k];
        if (!mn->on) continue;
        spr_draw(&chm_spr[CA_MINE], mn->x - 3, mn->y - 3, 0);
        if (mn->t >= 30 && (frame_t >> 3) & 1) gfx_pset(mn->x, mn->y, C_RED);
    }
    for (int i = 0; i < CHM_SHIPS; i++) draw_ship_full(r, i);
    for (int k = 0; k < CHM_BULLETS; k++)
        if (r->shot[k].on) gfx_rect((int)(r->shot[k].x >> 8) - 1, (int)(r->shot[k].y >> 8) - 1, 2, 2, C_YELLOW);
    for (int k = 0; k < CHM_PARTS; k++) {
        const ChmPart *p = &r->part[k];
        if (!p->on) continue;
        int x = p->x / 16, y = p->y / 16, c = p->col;
        if (p->kind == 1) c = p->life > 16 ? C_GREY : C_SLATE;
        if (p->kind == 2) c = p->life > 20 ? C_YELLOW : p->life > 10 ? C_ORANGE : C_RED;
        if (p->kind == 2 && p->life > 14) gfx_rect(x - 1, y - 1, 3, 3, c);
        else gfx_pset(x, y, c);
    }
    if (hud) draw_hud(r);
}

static void draw_countdown(const ChmRace *r) {
    if (r->phase == RP_COUNTDOWN) {
        int n = 3 - r->count_t / 40;
        char b[8];
        snprintf(b, sizeof b, "%d", n);
        ui_fancy_center(b, 160, 70, 4, GOLD, 4, C_INK, C_WINE);
    } else if (r->t < 45 && r->phase == RP_RUN && state == S_RACE) {
        ui_fancy_center("GO!", 160, 70, 4, GRAD, 4, C_INK, C_NAVY);
    }
}

static void draw_race(void) {
    draw_race_scene(&race, true);
    draw_countdown(&race);
    if (banner && banner_t > 0) {
        int w = text_width(banner);
        gfx_rect(160 - w / 2 - 6, 60, w + 12, 13, C_INK);
        text_center(banner, 160, 63, banner_col);
    }
    if (race.phase == RP_OVER && over_t > 20) {
        gfx_rect(96, 80, 128, 22, C_INK);
        gfx_rectb(96, 80, 128, 22, C_YELLOW);
        text_center("RACE OVER", 160, 83, C_YELLOW);
        text_center(GLYPH_A " RESULTS", 160, 92, C_GREY);
    }
}

static void draw_title(void) {
    draw_race_scene(&race, false);
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_fancy_center("CHIME", 160, 10, 3, GRAD, 4, C_INK, C_NAVY);
    ui_fancy_center("CIRCUIT", 160, 38, 3, GOLD, 4, C_INK, C_WINE);
    tiny_center("THE CHIME CUP " GLYPH_DOT " SIX SHIPS " GLYPH_DOT " EIGHT TRACKS " GLYPH_DOT " EIGHT LAPS", 160, 64, C_CREAM);
    static const char *const ITEMS[3] = {"1 PLAYER", "2 PLAYERS", "CODE"};
    ui_panel(112, 78, 96, 44, C_INK, C_SKY);
    for (int k = 0; k < 3; k++) {
        int y = 84 + k * 12;
        text_center(ITEMS[k], 160, y, title_sel == k ? C_YELLOW : C_LIGHT);
        if (title_sel == k) ui_cursor(118, y, frame_t);
    }
    if (fast_loot) {
        gfx_rect(116, 126, 88, 9, C_INK);
        tiny_center("LOOT-GALE IS ON", 160, 128, C_LIME);
    }
    char b[96];
    gfx_rect(0, 150, SCREEN_W, 30, C_INK);
    for (int p = 0; p < 2; p++) {
        char lap[16], rc[16];
        snprintf(lap, sizeof lap, "%s", sv.lap_n[p] ? secs(sv.lap_sum[p] / sv.lap_n[p]) : "--");
        snprintf(rc, sizeof rc, "%s", sv.race_n[p] ? secs(sv.race_sum[p] / sv.race_n[p]) : "--");
        snprintf(b, sizeof b, "P%d AVG LAP %s S " GLYPH_DOT " AVG RACE %s S", p + 1, lap, rc);
        tiny_center(b, 160, 153 + p * 8, p ? C_CYAN : C_YELLOW);
    }
    snprintf(b, sizeof b, "CUPS WON %d  " GLYPH_DOT "  RACES WON %d", sv.cup_wins, sv.race_wins);
    tiny_center(b, 160, 170, C_GREY);
}

static void draw_code(void) {
    backdrop();
    ui_fancy_center("CODE", 160, 14, 2, GRAD, 4, C_INK, C_NAVY);
    tiny_center("A CODE FROM THE PITS CHANGES THE RACES.", 160, 40, C_LIGHT);
    tiny_center("A TOURNAMENT BEGUN WITH ONE ON EARNS NO GOALS.", 160, 48, C_GREY);
    int x0 = 160 - (9 * 16) / 2;
    for (int k = 0; k < 9; k++) {
        int x = x0 + k * 16;
        if (k == 4) { text_draw_scaled("-", x + 3, 76, C_GREY, 2); continue; }
        int ci = k < 4 ? k : k - 1;
        char ch[2] = {code[ci], 0};
        bool sel = ci == code_pos;
        gfx_rect(x, 70, 13, 20, sel ? C_DUSK : C_NIGHT);
        text_draw_scaled(ch, x + 2, 74, sel ? C_YELLOW : C_WHITE, 2);
        if (sel) {
            text_draw(GLYPH_UP, x + 3, 61, C_GREY);
            text_draw(GLYPH_DOWN, x + 3, 92, C_GREY);
        }
    }
    if (code_msg && code_msg_t > 0) text_center(code_msg, 160, 112, C_LIME);
    text_center(GLYPH_DPAD " LETTERS   " GLYPH_A " ENTER   " GLYPH_B " BACK", 160, 162, C_GREY);
}

static void draw_page(void) {
    gfx_cls(C_CREAM);
    gfx_rect(30, 16, 260, 148, C_WHITE);
    gfx_rectb(30, 16, 260, 148, C_TAN);
    for (int y = 34; y < 160; y += 10) gfx_hline(36, 284, y + 8, C_ICE);
    text_draw("LAST-LAMP", 40, 22, C_RED);
    static const char *const L[] = {
        "A NOTE TAPED INSIDE THE CARTRIDGE:",
        "",
        "TWO WEEKS. EIGHT TRACKS. SIX SHIPS.",
        "I HAVE FLOWN EVERY LAP OF EVERY",
        "ONE OF THEM A HUNDRED TIMES.",
        "I CAN HEAR CHIMES WITH MY EYES SHUT.",
        "",
        "WAKE ME AT THE FINISH LINE.",
    };
    for (int k = 0; k < ARRAY_LEN(L); k++) text_draw(L[k], 40, 36 + k * 10, C_NAVY);
    text_draw("- T.", 240, 150, C_NAVY);
}

static void draw_pilots(void) {
    backdrop();
    ui_fancy_center("PICK A PILOT", 160, 6, 2, GRAD, 4, C_INK, C_NAVY);
    for (int k = 0; k < CHM_PILOTS; k++) {
        int x = 8 + k * 52, y = 34;
        const ChmPilot *p = &CHM_PILOT[k];
        bool s1 = pick[0] == k, s2 = cup.humans > 1 && pick[1] == k;
        gfx_rect(x, y, 48, 70, C_NIGHT);
        gfx_rectb(x, y, 48, 70, s1 ? (locked[0] ? C_YELLOW : C_AMBER) : s2 ? (locked[1] ? C_CYAN : C_SKY) : C_DUSK);
        if (s1 && s2) gfx_rectb(x + 1, y + 1, 46, 68, locked[1] ? C_CYAN : C_SKY);
        chm_draw_face(k, x + 8, y + 4, 2);
        tiny_center(p->name, x + 24, y + 40, C_WHITE);
        chm_draw_ship(k, x + 24, y + 56, 1, (frame_t / 8 + k) % 3 == 0, 0, 3, frame_t);
        if (s1) tiny_center(locked[0] ? "1P OK" : "1P", x + 24, y - 7, C_YELLOW);
        if (s2) tiny_center(locked[1] ? "2P OK" : "2P", x + 24, y + 72, C_CYAN);
    }
    const ChmPilot *p = &CHM_PILOT[pick[0] % CHM_PILOTS];
    gfx_rect(0, 118, SCREEN_W, 36, C_INK);
    char b[64];
    snprintf(b, sizeof b, "%s " GLYPH_DOT " %s", p->name, p->ship);
    text_center(b, 160, 122, C_YELLOW);
    tiny_center(p->line, 160, 134, C_LIGHT);
    if (cup.humans > 1) {
        const ChmPilot *q = &CHM_PILOT[pick[1] % CHM_PILOTS];
        snprintf(b, sizeof b, "2P: %s " GLYPH_DOT " %s", q->name, q->ship);
        tiny_center(b, 160, 144, C_CYAN);
    }
    text_center("ALL SHIPS FLY THE SAME   " GLYPH_A " PICK   " GLYPH_B " BACK", 160, 164, C_GREY);
}

static void draw_standings_list(int x, int y, bool with_gain) {
    int order[CHM_SHIPS];
    for (int i = 0; i < CHM_SHIPS; i++) order[chm_cup_rank(&cup, i) - 1] = i;
    for (int k = 0; k < CHM_SHIPS; k++) {
        int i = order[k];
        int yy = y + k * 13;
        const ChmPilot *p = &CHM_PILOT[cup.pilot[i] % CHM_PILOTS];
        bool human = i < cup.humans;
        gfx_rect(x, yy, 132, 12, human ? C_DUSK : C_NIGHT);
        char b[32];
        snprintf(b, sizeof b, "%d", k + 1);
        text_draw(b, x + 3, yy + 3, C_GREY);
        gfx_rect(x + 12, yy + 3, 8, 6, p->body);
        text_draw(p->name, x + 24, yy + 3, human ? (i == 0 ? C_YELLOW : C_CYAN) : C_WHITE);
        snprintf(b, sizeof b, "%d", cup.points[i]);
        text_draw(b, x + 100 - text_width(b), yy + 3, C_WHITE);
        if (with_gain) {
            snprintf(b, sizeof b, "+%d", gained[i]);
            tiny_draw(b, x + 106, yy + 4, C_LIME);
        }
    }
}

static void draw_card(void) {
    backdrop();
    const ChmTrackDef *d = &CHM_TRACK[cup.race % CHM_TRACKS];
    char b[48];
    snprintf(b, sizeof b, "RACE %d OF %d", cup.race + 1, CHM_RACES);
    text_draw(b, 8, 6, C_GREY);
    ui_fancy_text(d->name, 8, 16, 2, GOLD, 4, C_INK, C_WINE);
    tiny_draw(d->where, 8, 38, C_LIGHT);
    chm_draw_mini_track(&chm_map, 8, 50);
    tiny_draw("8 LAPS " GLYPH_DOT " 9 7 5 3 2 1 POINTS", 8, 144, C_GREY);
    text_draw("STANDINGS", 180, 30, C_CYAN);
    draw_standings_list(180, 42, false);
    if (cup.fast_loot) tiny_draw("LOOT-GALE IS ON", 180, 124, C_LIME);
    text_center(GLYPH_A " RACE!", 160, 164, (frame_t >> 4) & 1 ? C_YELLOW : C_WHITE);
}

static void draw_result(void) {
    backdrop();
    char b[64];
    snprintf(b, sizeof b, "RACE %d: %s", cup.race, CHM_TRACK[(cup.race + CHM_TRACKS - 1) % CHM_TRACKS].name);
    text_center(b, 160, 8, C_GREY);
    ui_fancy_center("RESULTS", 160, 20, 2, GOLD, 4, C_INK, C_WINE);
    for (int pl = 1; pl <= CHM_SHIPS; pl++) {
        int i = 0;
        for (int k = 0; k < CHM_SHIPS; k++)
            if (race.s[k].place == pl) i = k;
        const ChmShip *s = &race.s[i];
        int y = 46 + (pl - 1) * 15;
        bool human = s->human != 0;
        gfx_rect(40, y, 240, 13, human ? C_DUSK : C_NIGHT);
        text_draw(ordinal(pl), 46, y + 3, pl == 1 ? C_YELLOW : C_LIGHT);
        chm_draw_ship(s->pilot, 86, y + 6, 1, false, 0, 3, 0);
        text_draw(CHM_PILOT[s->pilot].name, 100, y + 3, human ? (s->human == 1 ? C_YELLOW : C_CYAN) : C_WHITE);
        if (s->finished) snprintf(b, sizeof b, "%s S", secs(s->finish_t));
        else snprintf(b, sizeof b, "--");
        text_draw(b, 170, y + 3, C_LIGHT);
        snprintf(b, sizeof b, "+%d", CHM_POINTS[pl - 1]);
        text_draw(b, 270 - text_width(b), y + 3, C_LIME);
    }
    text_center(GLYPH_A " STANDINGS", 160, 168, C_GREY);
}

static void draw_standings(void) {
    backdrop();
    char b[48];
    snprintf(b, sizeof b, "AFTER %d OF %d RACES", cup.race, CHM_RACES);
    text_center(b, 160, 8, C_GREY);
    ui_fancy_center("STANDINGS", 160, 20, 2, GRAD, 4, C_INK, C_NAVY);
    draw_standings_list(94, 50, true);
    text_center(cup.race < CHM_RACES ? GLYPH_A " NEXT RACE" : GLYPH_A " THE CUP", 160, 168, C_GREY);
}

static void draw_final(void) {
    backdrop();
    ui_fancy_center("THE CHIME CUP", 160, 6, 2, GOLD, 4, C_INK, C_WINE);
    int order[CHM_SHIPS];
    for (int i = 0; i < CHM_SHIPS; i++) order[chm_cup_rank(&cup, i) - 1] = i;
    static const int PX_[3] = {160, 100, 220}, PH[3] = {40, 26, 16};
    for (int k = 0; k < 3; k++) {
        int i = order[k], x = PX_[k], base = 124;
        gfx_rect(x - 26, base - PH[k], 52, PH[k], k == 0 ? C_AMBER : k == 1 ? C_LIGHT : C_TAN);
        char b[8];
        snprintf(b, sizeof b, "%d", k + 1);
        text_center(b, x, base - PH[k] + 4, C_INK);
        chm_draw_face(cup.pilot[i], x - 16, base - PH[k] - 34, 2);
        text_center(pilot_name(i), x, base + 4, i < cup.humans ? C_YELLOW : C_WHITE);
        snprintf(b, sizeof b, "%d PTS", cup.points[i]);
        tiny_center(b, x, base + 14, C_LIGHT);
    }
    if (champion >= 0) spr_draw(&chm_spr[CA_CUP], 152, 30, 0);
    char b[64];
    if (champion >= 0 && champion < cup.humans) snprintf(b, sizeof b, "%s WINS THE CUP!", pilot_name(champion));
    else snprintf(b, sizeof b, "THE CUP GOES TO %s. NEXT SEASON!", champion >= 0 ? pilot_name(champion) : "?");
    text_center(b, 160, 150, C_YELLOW);
    if (cup_no_goals) tiny_center("(LOOT-GALE WAS ON: NO GOALS)", 160, 161, C_GREY);
    text_center(GLYPH_A " ON", 160, 169, C_GREY);
}

static const char *const ENDING[CHM_PILOTS][3] = {
    {"ANSEL HANGS THE CHIME CUP FROM THE", "TINKLER'S NOSE. IT RINGS ALL THE WAY", "HOME, AND HE LETS IT."},
    {"CLARY WINS BY A NOSE. BY SUPPER IT'S", "A LAP, AND BY MORNING IT'S A MILE IN", "EVERY STORY SHE TELLS ANSEL."},
    {"WICK DELIVERS THE CUP TO HERSELF.", "SIGNED FOR, ON TIME, AND NOT A", "SCRATCH ON THE TUMBLEWEED."},
    {"TANGER TAKES THE CUP HOME TO THE", "CAVES AND BANGS IT LIKE A GONG.", "EVERYONE COMES OUT TO SEE."},
    {"KIP BOLTS THE CUP ON AS A NEW NOSE", "CONE. IT IS THE SHINIEST THING", "SHE HAS EVER FOUND."},
    {"ZORP PUTS THE CUP IN THE THIRD", "WALLET. IT DOES NOT FIT. ZORP", "BUYS A FOURTH WALLET."},
};

static void draw_ending(void) {
    int p = champion >= 0 ? cup.pilot[champion] % CHM_PILOTS : 0;
    gfx_cls(C_NAVY);
    for (int i = 0; i < 60; i++) gfx_pset((i * 89 + 7) % 320, (i * 61 + 3) % 100, i % 4 ? C_BLUE : C_WHITE);
    gfx_rect(0, 100, SCREEN_W, 80, C_FOREST);
    gfx_dither(0, 100, SCREEN_W, 8, C_JADE, 8);
    int sx = 60 + (state_t * 2) % 240, sy = 70 + (state_t / 6) % 8;
    chm_draw_ship(p, sx, sy, 1, true, 0, 3, frame_t);
    spr_draw(&chm_spr[CA_CUP], sx - 8, sy - 24, 0);
    chm_draw_face(p, 16, 110, 3);
    for (int k = 0; k < 3; k++) text_draw(ENDING[p][k], 72, 116 + k * 12, C_WHITE);
    if (state_t > 60) text_center(GLYPH_A " ON", 160, 168, C_LIME);
}

static const char *const CREDITS[] = {
    "CHIME CIRCUIT", "", "PRESENTED BY", "BEAMDOWN SOFTWORKS", "1985", "",
    "THE PILOTS", "ANSEL AND CLARY", "WICK AND TANGER, VISITING FROM HOMESPUN", "KIP, VISITING FROM SKYWELL",
    "ZORP, VISITING FROM WOBBLE DERBY", "",
    "THE TRACKS", "PRELUDE RING", "THE DIPPER", "NEEDLE'S EYE", "HOURGLASS", "FORKED REED", "TWIN FLUE",
    "CROOKED MILE", "GRAND OCTAVE", "",
    "OVERHEARD IN THE PITS", "LOOT-GALE", "", "SCRAWLED ON A LOCKER", "LAST-LAMP", "",
    "THANKS FOR RACING", "",
};

static void draw_credits(void) {
    gfx_cls(C_INK);
    int y0 = 190 - credits_t / 2;
    int n = ARRAY_LEN(CREDITS);
    for (int k = 0; k < n; k++) {
        int y = y0 + k * 12;
        if (y < -10 || y > 190) continue;
        bool head = k == 0 || (k > 0 && !CREDITS[k - 1][0] && CREDITS[k][0]);
        text_center(CREDITS[k], 160, y, head ? C_YELLOW : C_LIGHT);
    }
    if (y0 + n * 12 < 60) {
        ui_fancy_center("THE END", 160, 72, 3, GOLD, 4, C_INK, C_WINE);
        if (credits_t > 60 * 20) text_center(GLYPH_A " TITLE", 160, 168, C_GREY);
    }
}

static void draw(void) {
    gfx_camera(0, 0);
    gfx_noclip();
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_CODE: draw_code(); break;
    case S_PAGE: draw_page(); break;
    case S_PILOTS: draw_pilots(); break;
    case S_CARD: draw_card(); break;
    case S_RACE: draw_race(); break;
    case S_RESULT: draw_result(); break;
    case S_STANDINGS: draw_standings(); break;
    case S_FINAL: draw_final(); break;
    case S_ENDING: draw_ending(); break;
    case S_CREDITS: draw_credits(); break;
    }
}

/* ------------------------------------------------------------------ */
/* the demo pilot's buttons (tests and screenshots)                     */

static int press(int b) { return (frame_t & 1) ? b : 0; }

static int chm_bot(void) {
    switch (state) {
    case S_TITLE:
        if (state_t < 20) return 0;
        if (title_sel != bot_players - 1) return press(BTN_DOWN);
        return press(BTN_A);
    case S_PILOTS: return state_t > 12 ? press(BTN_A) : 0;
    case S_CARD: return state_t > 25 ? press(BTN_A) : 0;
    case S_RACE: {
        if (race.phase == RP_OVER) return over_t > 60 ? press(BTN_A) : 0;
        if (race.phase != RP_RUN) return 0;
        int i = chm_ship_by_human(&race, 1);
        if (i < 0) return 0;
        unsigned c = chm_ai(&race, i, true);
        int m = 0;
        if (c & CHF_LEFT) m |= BTN_LEFT;
        if (c & CHF_RIGHT) m |= BTN_RIGHT;
        if (c & CHF_THRUST) m |= BTN_A;
        if (c & CHM_CTL_SLASH) m |= BTN_B;
        return m;
    }
    case S_FINAL:
    case S_ENDING: return state_t > 70 ? press(BTN_A) : 0;
    case S_CREDITS: return credits_t > 60 * 20 + 10 ? press(BTN_A) : BTN_A;
    default: return state_t > 25 ? press(BTN_A) : 0;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void chm_load(void) {
    chm_art_load();
    chm_audio_load();
}

static void chm_start(void) {
    load_save();
    memset(&cup, 0, sizeof cup);
    fast_loot = false;
    cup_no_goals = false;
    title_sel = 0;
    champion = -1;
    title_enter();
}

static void chm_quit(void) {
    input_set_versus(false);
    save_now();
}

static void chm_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_SKY);
    gfx_rect(x, y + h / 2, w, h / 2, C_CYAN);
    gfx_dither(x, y + h / 2 - 6, w, 6, C_CYAN, 8);
    /* a loop of track */
    gfx_rect(x, y + h - 14, w, 14, C_TAN);
    gfx_hline(x, x + w - 1, y + h - 14, C_LEAF);
    gfx_rect(x + 30, y + 30, w - 60, 10, C_TAN);
    gfx_hline(x + 30, x + w - 31, y + 30, C_LEAF);
    for (int k = 0; k < 3; k++) {
        int sx = x + ((t * (3 + k)) / 3 + k * 47) % (w + 24) - 12;
        int sy = y + h - 24 + ((t / 7 + k * 3) % 5) - (k == 1 ? 26 : 0);
        chm_draw_ship(k == 1 ? 1 : k == 2 ? 4 : 0, sx, sy, 1, (t / 4 + k) % 3 != 0, 0, 3, t);
    }
    ui_fancy_text("CHIME CIRCUIT", x + 4, y + 3, 1, GOLD, 4, C_INK, -1);
}

static int ship_key(const char *key, const char *name, int *ship) {
    size_t n = strlen(name);
    if (strncmp(key, name, n) || key[n] < '0' || key[n] > '9' || key[n + 1]) return 0;
    *ship = (key[n] - '0') % CHM_SHIPS;
    return 1;
}

static int chm_query(const char *key, int *out) {
    int i;
    if (!strcmp(key, "bot")) { *out = chm_bot(); return 1; }
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "race")) { *out = cup.race; return 1; }
    if (!strcmp(key, "track")) { *out = race.track; return 1; }
    if (!strcmp(key, "phase")) { *out = race.phase; return 1; }
    if (!strcmp(key, "t")) { *out = (int)race.t; return 1; }
    if (!strcmp(key, "humans")) { *out = cup.humans; return 1; }
    if (!strcmp(key, "map_ok")) { *out = chm_map.ok; return 1; }
    if (!strcmp(key, "art_ok")) { *out = chm_art_ok(); return 1; }
    if (!strcmp(key, "nst")) { *out = race.nst; return 1; }
    if (!strcmp(key, "fast_loot")) { *out = fast_loot; return 1; }
    if (!strcmp(key, "code_pos")) { *out = code_pos; return 1; }
    if (!strcmp(key, "title_sel")) { *out = title_sel; return 1; }
    if (!strcmp(key, "p1pick")) { *out = pick[0]; return 1; }
    if (!strcmp(key, "p2pick")) { *out = pick[1]; return 1; }
    if (!strcmp(key, "p1locked")) { *out = locked[0]; return 1; }
    if (!strcmp(key, "p2locked")) { *out = locked[1]; return 1; }
    if (!strcmp(key, "champion")) { *out = champion; return 1; }
    if (!strcmp(key, "leader")) { *out = chm_cup_leader(&cup); return 1; }
    if (!strcmp(key, "nfinished")) { *out = race.nfinished; return 1; }
    if (!strcmp(key, "cups")) { *out = sv.cups; return 1; }
    if (!strcmp(key, "cup_wins")) { *out = sv.cup_wins; return 1; }
    if (!strcmp(key, "race_wins")) { *out = sv.race_wins; return 1; }
    if (!strcmp(key, "best")) { *out = sv.best; return 1; }
    if (!strcmp(key, "credits_t")) { *out = credits_t; return 1; }
    if (!strcmp(key, "ncp")) { *out = chm_map.ncp; return 1; }
    if (!strcmp(key, "spawn_x")) { *out = (int)(chm_map.spawn_x >> 8); return 1; }
    if (!strcmp(key, "spawn_y")) { *out = (int)(chm_map.spawn_y >> 8); return 1; }
    if (!strcmp(key, "shots")) { int n = 0; for (int k = 0; k < CHM_BULLETS; k++) n += race.shot[k].on; *out = n; return 1; }
    if (!strcmp(key, "mines")) { int n = 0; for (int k = 0; k < CHM_MINES; k++) n += race.mine[k].on; *out = n; return 1; }
    if (!strcmp(key, "fires")) { int n = 0; for (int k = 0; k < CHM_FIRES; k++) n += race.fire[k].on; *out = n; return 1; }
    if (!strcmp(key, "pickups_seen")) { int n = 0; for (int k = 0; k < race.nst; k++) n += race.st[k].state == 2; *out = n; return 1; }
    if (!strcmp(key, "pts_total")) { int n = 0; for (int k = 0; k < CHM_SHIPS; k++) n += cup.points[k]; *out = n; return 1; }
    if (!strcmp(key, "route_len0")) { *out = chm_route_length(&chm_map, 0); return 1; }
    if (!strcmp(key, "cpu_knocks")) { int n = 0; for (int k = 0; k < CHM_SHIPS; k++) if (!race.s[k].human) n += race.s[k].knocks; *out = n; return 1; }
    if (!strcmp(key, "hunters")) { int n = 0; for (int k = 0; k < CHM_SHIPS; k++) n += !race.s[k].human && race.s[k].aggro == AG_HUNT; *out = n; return 1; }
    if (!strcmp(key, "cpu_laps")) { int n = 0; for (int k = 0; k < CHM_SHIPS; k++) if (!race.s[k].human) n += race.s[k].laps; *out = n; return 1; }
    /* per player: stats */
    for (int p = 0; p < 2; p++) {
        char k1[16], k2[16], k3[16], k4[16];
        snprintf(k1, sizeof k1, "lap_n%d", p + 1);
        snprintf(k2, sizeof k2, "lap_avg%d", p + 1);
        snprintf(k3, sizeof k3, "race_n%d", p + 1);
        snprintf(k4, sizeof k4, "race_avg%d", p + 1);
        if (!strcmp(key, k1)) { *out = (int)sv.lap_n[p]; return 1; }
        if (!strcmp(key, k2)) { *out = sv.lap_n[p] ? (int)(sv.lap_sum[p] / sv.lap_n[p]) : 0; return 1; }
        if (!strcmp(key, k3)) { *out = (int)sv.race_n[p]; return 1; }
        if (!strcmp(key, k4)) { *out = sv.race_n[p] ? (int)(sv.race_sum[p] / sv.race_n[p]) : 0; return 1; }
    }
    if (ship_key(key, "order", &i)) { *out = race.order[i]; return 1; }
    if (ship_key(key, "st_state", &i)) { *out = i < race.nst ? race.st[i].state : -1; return 1; }
    if (ship_key(key, "st_kind", &i)) { *out = i < race.nst ? race.st[i].kind : -1; return 1; }
    if (ship_key(key, "st_t", &i)) { *out = i < race.nst ? race.st[i].t : -1; return 1; }
    if (ship_key(key, "points", &i)) { *out = cup.points[i]; return 1; }
    if (ship_key(key, "rank", &i)) { *out = chm_cup_rank(&cup, i); return 1; }
    if (ship_key(key, "wins", &i)) { *out = chm_cup_wins(&cup, i); return 1; }
    if (ship_key(key, "grid", &i)) { *out = cup.grid[i]; return 1; }
    if (ship_key(key, "cpilot", &i)) { *out = cup.pilot[i]; return 1; }
    static const char *const SK[] = {"laps", "hp", "alive", "place", "cp", "x", "y", "vx", "vy", "stun", "mercy",
                                     "power", "face", "walls", "wrecks", "lost", "human", "pilot", "fin", "slash",
                                     "knocks", "lastlap", "fin_t", "prog", "rz", "route", "skill", "slot"};
    for (int k = 0; k < ARRAY_LEN(SK); k++)
        if (ship_key(key, SK[k], &i)) {
            const ChmShip *s = &race.s[i];
            switch (k) {
            case 0: *out = s->laps; break;
            case 1: *out = s->hp; break;
            case 2: *out = s->alive; break;
            case 3: *out = s->place; break;
            case 4: *out = s->cp; break;
            case 5: *out = (int)(s->f.x >> 8); break;
            case 6: *out = (int)(s->f.y >> 8); break;
            case 7: *out = s->f.vx; break;
            case 8: *out = s->f.vy; break;
            case 9: *out = s->stun; break;
            case 10: *out = s->mercy; break;
            case 11: *out = s->power; break;
            case 12: *out = s->f.face; break;
            case 13: *out = s->wall_hits; break;
            case 14: *out = s->wrecks; break;
            case 15: *out = s->lost; break;
            case 16: *out = s->human; break;
            case 17: *out = s->pilot; break;
            case 18: *out = s->finished; break;
            case 19: *out = s->slash_t; break;
            case 20: *out = s->knocks; break;
            case 21: *out = s->last_lap; break;
            case 22: *out = (int)s->finish_t; break;
            case 23: *out = (int)(chm_progress(&race, i) >> 4); break;
            case 24: *out = s->rz; break;
            case 25: *out = s->route; break;
            case 26: *out = s->skill; break;
            case 27: *out = s->grid; break;
            }
            return 1;
        }
    return 0;
}

static int chm_cheat(const char *cmd) {
    int a, b, c;
    if (sscanf(cmd, "cup %d", &a) == 1) {
        /* a fresh tournament with a players (pilots 0 and 1), at its first card */
        pick[0] = 0;
        pick[1] = 1;
        cup.humans = (uint8_t)iclamp(a, 1, 2);
        input_set_versus(cup.humans == 2);
        start_cup();
        return 1;
    }
    if (sscanf(cmd, "race %d", &a) == 1) {
        /* straight into race a (1-8) of a fresh one-player tournament */
        pick[0] = 0;
        pick[1] = 1;
        cup.humans = 1;
        start_cup();
        cup.race = (uint8_t)((a - 1) % CHM_TRACKS);
        race_enter();
        return 1;
    }
    if (!strcmp(cmd, "go")) {
        /* skip the countdown */
        race.phase = RP_RUN;
        race.count_t = CHM_COUNTDOWN;
        return 1;
    }
    if (sscanf(cmd, "park %d", &a) == 1) { race.s[a % CHM_SHIPS].parked = 1; return 1; }
    if (sscanf(cmd, "idle %d", &a) == 1) { race.s[a % CHM_SHIPS].idle = 1; return 1; }
    if (!strcmp(cmd, "clear")) {
        /* no shots, mines or fires on the course */
        memset(race.shot, 0, sizeof race.shot);
        memset(race.mine, 0, sizeof race.mine);
        memset(race.fire, 0, sizeof race.fire);
        return 1;
    }
    if (sscanf(cmd, "revive %d", &a) == 1) {
        ChmShip *s = &race.s[a % CHM_SHIPS];
        s->alive = 1;
        s->hp = CHM_HP;
        s->wreck_t = 0;
        s->mercy = 0;
        return 1;
    }
    if (sscanf(cmd, "fire %d %d", &a, &b) == 2) { chm_race_fire(&race, a, b); return 1; }
    if (!strcmp(cmd, "park_cpus")) {
        for (int i = 0; i < CHM_SHIPS; i++) if (!race.s[i].human) race.s[i].parked = 1;
        return 1;
    }
    if (sscanf(cmd, "place %d %d %d", &a, &b, &c) == 3) {
        ChmShip *s = &race.s[a % CHM_SHIPS];
        s->f.x = b * CHF_ONE;
        s->f.y = c * CHF_ONE;
        s->f.vx = s->f.vy = 0;
        s->parked = 0;
        return 1;
    }
    if (sscanf(cmd, "vel %d %d %d", &a, &b, &c) == 3) { race.s[a % CHM_SHIPS].f.vx = b; race.s[a % CHM_SHIPS].f.vy = c; return 1; }
    if (sscanf(cmd, "face %d %d", &a, &b) == 2) { race.s[a % CHM_SHIPS].f.face = (int8_t)(b < 0 ? -1 : 1); return 1; }
    if (sscanf(cmd, "hp %d %d", &a, &b) == 2) { race.s[a % CHM_SHIPS].hp = (int8_t)b; return 1; }
    if (sscanf(cmd, "laps %d %d", &a, &b) == 2) { race.s[a % CHM_SHIPS].laps = (uint8_t)b; prev_laps[a % CHM_SHIPS] = b; return 1; }
    if (sscanf(cmd, "cp %d %d", &a, &b) == 2) { race.s[a % CHM_SHIPS].cp = (uint8_t)b; return 1; }
    if (sscanf(cmd, "rz %d %d", &a, &b) == 2) { race.s[a % CHM_SHIPS].rz = (uint8_t)b; return 1; }
    if (sscanf(cmd, "power %d %d", &a, &b) == 2) { chm_race_power(&race, a % CHM_SHIPS, b); return 1; }
    if (sscanf(cmd, "station %d %d", &a, &b) == 2) { chm_station_fill(&race, a, b); return 1; }
    if (sscanf(cmd, "mercy %d %d", &a, &b) == 2) { race.s[a % CHM_SHIPS].mercy = (uint16_t)b; return 1; }
    if (sscanf(cmd, "wreck %d", &a) == 1) { chm_race_wreck(&race, a % CHM_SHIPS); return 1; }
    if (!strcmp(cmd, "plan")) {
        extern int chm_ai_debug;
        chm_ai_debug = 1;
        return 1;
    }
    if (sscanf(cmd, "dump %d", &a) == 1) {
        /* debugging: the distance field to zone a over the ship-fits grid */
        for (int j = 0; j < CHM_NH; j++) {
            char line[CHM_NW + 1];
            for (int k = 0; k < CHM_NW; k++) {
                uint16_t v = chm_map.field[a % CHM_ZONES][j * CHM_NW + k];
                line[k] = !chm_map.nav[j][k] ? '#' : v == CHM_FAR ? '?' : (char)('0' + (v / 100) % 10);
            }
            line[CHM_NW] = 0;
            printf("%s\n", line);
        }
        return 1;
    }
    if (sscanf(cmd, "bot_players %d", &a) == 1) { bot_players = iclamp(a, 1, 2); return 1; }
    if (sscanf(cmd, "points %d %d", &a, &b) == 2) { cup.points[a % CHM_SHIPS] = (uint8_t)b; return 1; }
    if (!strncmp(cmd, "finish ", 7)) {
        /* end the race now with this finishing order (ship numbers, first to last) */
        int v[CHM_SHIPS];
        if (sscanf(cmd, "finish %d %d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) return 0;
        for (int k = 0; k < CHM_SHIPS; k++) {
            ChmShip *s = &race.s[v[k] % CHM_SHIPS];
            s->place = (uint8_t)(k + 1);
            s->finished = 1;
            s->laps = CHM_LAPS;
            s->finish_t = race.t ? race.t : 1;
        }
        race.nfinished = CHM_SHIPS;
        race.phase = RP_OVER;
        return 1;
    }
    return 0;
}

const GameDef GAME_CHIME = {
    "chime",
    "CHIME CIRCUIT",
    "1985",
    "RACING",
    "EIGHT TRACKS, EIGHT LAPS. THRUST, STEER AND SLASH RIVALS INTO THE WALLS.",
    {"TAKE 1ST PLACE IN A RACE", "WIN THE CHIME CUP", "TAKE 1ST PLACE IN ALL 8 RACES"},
    GLYPH_LEFT GLYPH_RIGHT "\tSTEER\n"
    GLYPH_A " (HOLD)\tTHRUST UP\n"
    GLYPH_B "\tSLASH (KNOCKS, NO HARM)\n"
    "START\tPAUSE\n"
    "2P\tTHE SECOND PAD",
    C_BLUE, C_AMBER,
    chm_load, chm_start, update, draw, chm_quit, chm_label, chm_query, chm_cheat,
    "THE BIG BELL RACE", 19,
    NULL,
};
