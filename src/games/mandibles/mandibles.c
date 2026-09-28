/* MANDIBLES - a top-down ant war on foot. Cartridge 46 of UFO 40, a tribute
 * to Combatants (UFO 50 #46). See docs/games/46-mandibles.md.
 * The rules live in mandibles_logic.c, the fields in mandibles_maps.c and
 * the demo player in mandibles_bot.c; this file is menus, drawing and flow. */
#include "mandibles.h"

#define VIEW_Y 11
#define VIEW_H 160
#define FOOT_Y 172
#define FP MND_FP

enum { S_TITLE, S_MAP, S_BRIEF, S_PLAY, S_RESULT, S_VS_PICK, S_ENDING };

typedef struct Save {
    uint32_t magic;
    uint16_t won;          /* bit i: field i+1 taken (bit 12: the bonus) */
    uint8_t cursor;        /* the campaign map's cursor */
    uint8_t spiders;       /* longlegs slain (stops at 255) */
    uint16_t played, lost;
    uint8_t seen_ending, pad[3];
} Save;
#define SAVE_MAGIC 0x4D4E4401u
#define ALL_TWELVE 0x0FFFu

static Save sv;
static Rng seeds;
static int state, state_t, title_sel, map_sel, vs_sel, cur_map;
static bool versus, won_now, first_capital;
static int result_status, vs_wins[2];
static int cam_x[2], cam_y[2];
static int shake;
static int shout_t[2];
static int tags[8], n_tags; /* units the tests placed */
static int pscr_x[2], pscr_y[2]; /* each player's ant on the screen (the command cross) */
static int walk_from, walk_t;    /* the leader walking between fields on the map */


static bool vita_single(void) { return plat_kind() == PLAT_VITA; }

/* ------------------------------------------------------------------ */
/* save & goals                                                         */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else { memset(&sv, 0, sizeof sv); sv.magic = SAVE_MAGIC; }
}

static int won_count(void) {
    int n = 0;
    for (int i = 0; i < MND_MISSIONS; i++) n += (sv.won >> i) & 1;
    return n;
}

static void check_goals(void) {
    if (sv.spiders > 0) game_award(GOAL_BEACON);                       /* slay a longlegs */
    if (sv.won & (1u << (MND_MISSIONS - 1))) game_award(GOAL_SAUCER);   /* retake the capital */
    if ((sv.won & ALL_TWELVE) == ALL_TWELVE) game_award(GOAL_ALIEN);   /* every mission */
}

/* A field is open when it is the first, or a won field leads to it. */
static bool map_open(int i) {
    if (i == 0) return true;
    if (i == MND_BONUS) return (sv.won >> (MND_MISSIONS - 1)) & 1;
    for (int j = 0; j < MND_MISSIONS; j++) {
        if (!((sv.won >> j) & 1)) continue;
        for (int k = 0; k < 3; k++)
            if (MND_MAPS[j].next[k] == i + 1) return true;
    }
    return false;
}
static bool map_won(int i) { return (sv.won >> i) & 1; }

/* ------------------------------------------------------------------ */
/* flow                                                                 */

static uint64_t new_seed(void) { return rng_next(&seeds) ^ ((uint64_t)rng_next(&seeds) << 32); }

static void pause_pick(int i);
static const char *const PAUSE_CAMPAIGN[2] = {"RESTART MISSION", "RETREAT TO MAP"};
static const char *const PAUSE_VERSUS[2] = {"RESTART FIELD", "LEAVE FIELD"};

static void go(int s) {
    state = s;
    state_t = 0;
    if (s != S_PLAY) {
        game_pause_items(0, NULL, NULL);
        input_set_versus(false);
    }
    game_set_pausable(s != S_TITLE);
}

static void center_cams(void) {
    for (int s = 0; s < 2; s++) {
        int p = mnd_w.player[s];
        if (p < 0) p = mnd_queen_of(&mnd_w, s);
        if (p < 0) continue;
        cam_x[s] = mnd_w.u[p].x / FP;
        cam_y[s] = mnd_w.u[p].y / FP;
    }
}

static void start_field(int map, uint64_t seed, bool vs) {
    cur_map = map;
    versus = vs;
    mnd_load_map(&mnd_w, map, seed, vs);
    mnd_bot_reset();
    n_tags = 0;
    center_cams();
    shake = 0;
    shout_t[0] = shout_t[1] = 0;

    go(S_PLAY);
    input_set_versus(vs);
    game_pause_items(2, vs ? PAUSE_VERSUS : PAUSE_CAMPAIGN, pause_pick);
    if (vs) music_play(MND_MUS_BATTLE);
    else music_play(map >= MND_MISSIONS - 1 ? MND_MUS_CAPITAL : MND_MUS_FIELD);
    if (!vs) { if (sv.played < 65535) sv.played++; save_now(); }
}

static void go_title(void) {
    go(S_TITLE);
    music_play(MND_MUS_TITLE);
}

static void go_map(void) {
    go(S_MAP);
    if (!map_open(map_sel)) map_sel = 0;
    walk_from = map_sel;
    walk_t = 30;
    music_play(MND_MUS_MAP);
}

static void pause_pick(int i) {
    if (i == 0) start_field(cur_map, new_seed(), versus);
    else if (versus) { go(S_VS_PICK); music_play(MND_MUS_TITLE); }
    else go_map();
}

static void finish_field(void) {
    result_status = mnd_w.status;
    go(S_RESULT);
    if (versus) {
        vs_wins[result_status == MND_WON ? 0 : 1]++;
        music_play(MND_MUS_WIN);
        return;
    }
    won_now = result_status == MND_WON;
    first_capital = false;
    if (won_now) {
        if (cur_map == MND_MISSIONS - 1 && !map_won(cur_map)) first_capital = true;
        sv.won |= (uint16_t)(1u << cur_map);
        music_play(MND_MUS_WIN);
    } else {
        if (sv.lost < 65535) sv.lost++;
        music_play(MND_MUS_LOSE);
    }
    save_now();
    check_goals();
}

/* ------------------------------------------------------------------ */
/* input                                                                */

static MndPad read_pad(int p) {
    MndPad pd;
    memset(&pd, 0, sizeof pd);
    bool (*held)(int) = p ? btn2 : btn;
    bool (*pressed)(int) = p ? btnp2 : btnp;
    pd.up = held(BTN_UP);
    pd.down = held(BTN_DOWN);
    pd.left = held(BTN_LEFT);
    pd.right = held(BTN_RIGHT);
    pd.arm_pressed[ARM_UP] = pressed(BTN_UP);
    pd.arm_pressed[ARM_RIGHT] = pressed(BTN_RIGHT);
    pd.arm_pressed[ARM_DOWN] = pressed(BTN_DOWN);
    pd.arm_pressed[ARM_LEFT] = pressed(BTN_LEFT);
    pd.order_held = held(MND_BTN_ORDER);
    pd.order_pressed = pressed(MND_BTN_ORDER);
    pd.spit_held = held(MND_BTN_SPIT);
    return pd;
}

static void follow_cam(int s, int vw) {
    int p = mnd_w.player[s];
    if (p < 0) p = mnd_queen_of(&mnd_w, s);
    if (p < 0) return;
    int tx = mnd_w.u[p].x / FP, ty = mnd_w.u[p].y / FP;
    /* ease toward the ant */
    cam_x[s] += (tx - cam_x[s]) / 6;
    cam_y[s] += (ty - cam_y[s]) / 6;
    (void)vw;
}

static void play_sounds(void) {
    const MndEvents *e = &mnd_w.ev;
    if (e->spider_slain) sfx_play_name("mnd_spider_die");
    else if (e->deaths) sfx_play_name("mnd_die");
    else if (e->melee) sfx_play_name("mnd_crunch");
    else if (e->bites) sfx_play_name("mnd_bite");
    else if (e->hits) sfx_play_name("mnd_hit");
    else if (e->spits) sfx_play_name("mnd_spit");
    if (e->respawns) sfx_play_name("mnd_reborn");
    else if (e->deliveries) sfx_play_name("mnd_feed");
    else if (e->pickups) sfx_play_name("mnd_bead");
    else if (e->orders) sfx_play_name("mnd_shout");
    else if (e->menu_moves) sfx_play_name("mnd_menu");
    else if (e->spawns) sfx_play_name("mnd_hatch");
    if (e->melee || e->bites) shake = imax(shake, 4);
}

static void update_play(void) {
    MndPad pads[2];
    pads[0] = read_pad(0);
    pads[1] = versus ? read_pad(1) : (MndPad){0};
    bool was_open[2] = {mnd_w.menu[0].open, mnd_w.menu[1].open};
    int was_player = mnd_w.player[0];
    mnd_step(&mnd_w, pads);
    for (int s = 0; s < 2; s++) {
        if (!was_open[s] && mnd_w.menu[s].open) sfx_play_name("mnd_open");
        if (mnd_w.ev.orders && was_open[s] && !mnd_w.menu[s].open) shout_t[s] = 14;
        if (shout_t[s] > 0) shout_t[s]--;
    }
    if (was_player >= 0 && mnd_w.player[0] < 0 && !versus) sfx_play_name("mnd_ouch");
    play_sounds();
    if (mnd_w.ev.blue_spider_kill && !versus) {
        if (sv.spiders < 255) sv.spiders++;
        save_now();
        check_goals();
    }
    if (shake > 0) shake--;
    follow_cam(0, versus ? 159 : SCREEN_W);
    if (versus) follow_cam(1, 159);
    if (mnd_w.status != MND_PLAYING && mnd_w.surrendered && !versus) {
        /* withdrawing: straight back to the map, counted as a loss */
        if (sv.lost < 65535) sv.lost++;
        save_now();
        sfx_play_name("ui_back");
        go_map();
        return;
    }
    if (mnd_w.status != MND_PLAYING) finish_field();
}

/* campaign map: move the cursor to the nearest open field that way */
static void map_move(int dx, int dy) {
    const MndMap *c = &MND_MAPS[map_sel];
    int best = -1, bestd = 1 << 30;
    for (int i = 0; i <= MND_BONUS; i++) {
        if (i == map_sel || !map_open(i)) continue;
        int ex = MND_MAPS[i].map_x - c->map_x, ey = MND_MAPS[i].map_y - c->map_y;
        int along = ex * dx + ey * dy, perp = iabs(ex * dy - ey * dx);
        if (along <= 0 || perp > along * 2) continue;
        int d = along + perp * 2;
        if (d < bestd) { bestd = d; best = i; }
    }
    if (best >= 0) {
        walk_from = map_sel;
        walk_t = 0;
        map_sel = best;
        sv.cursor = (uint8_t)best;
        sfx_play_name("ui_move");
    }
}

static void mnd_update(void) {
    state_t++;
    switch (state) {
    case S_TITLE:
        if (btnp(BTN_UP) || btnp(BTN_DOWN)) { title_sel ^= 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_A) || btnp(BTN_START)) {
            if (title_sel == 0) { sfx_play_name("ui_ok"); go_map(); }
            else if (vita_single()) sfx_play_name("ui_error");
            else { sfx_play_name("ui_ok"); go(S_VS_PICK); }
        }
        if (btnp(BTN_B)) game_exit_to_library();
        break;
    case S_MAP:
        walk_t++;
        if (btnp(BTN_LEFT)) map_move(-1, 0);
        if (btnp(BTN_RIGHT)) map_move(1, 0);
        if (btnp(BTN_UP)) map_move(0, -1);
        if (btnp(BTN_DOWN)) map_move(0, 1);
        if (btnp(BTN_A)) { sfx_play_name("ui_ok"); go(S_BRIEF); }
        else if (btnp(BTN_B)) { sfx_play_name("ui_back"); save_now(); go_title(); }
        break;
    case S_BRIEF:
        if (state_t > 10 && btnp(BTN_A)) start_field(map_sel, new_seed(), false);
        else if (btnp(BTN_B)) { sfx_play_name("ui_back"); go(S_MAP); }
        break;
    case S_PLAY: update_play(); break;
    case S_RESULT:
        if (state_t > 40 && (btnp(BTN_A) || btnp(BTN_START))) {
            if (versus) { go(S_VS_PICK); music_play(MND_MUS_TITLE); }
            else if (won_now && first_capital) { sv.seen_ending = 1; save_now(); go(S_ENDING); music_play(MND_MUS_TITLE); }
            else if (won_now) go_map();
            else start_field(cur_map, new_seed(), false);
        } else if (state_t > 40 && btnp(BTN_B)) {
            if (versus) { go(S_VS_PICK); music_play(MND_MUS_TITLE); }
            else go_map();
        }
        break;
    case S_VS_PICK:
        if (btnp(BTN_LEFT)) { vs_sel = (vs_sel + MND_VS_MAPS - 1) % MND_VS_MAPS; sfx_play_name("ui_move"); }
        if (btnp(BTN_RIGHT)) { vs_sel = (vs_sel + 1) % MND_VS_MAPS; sfx_play_name("ui_move"); }
        if (btnp(BTN_A) || btnp(BTN_START)) start_field(MND_MISSIONS + 1 + vs_sel, new_seed(), true);
        else if (btnp(BTN_B)) { sfx_play_name("ui_back"); go_title(); }
        break;
    case S_ENDING:
        if (state_t > 120 && (btnp(BTN_A) || btnp(BTN_START))) go_map();
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing the field                                                    */

static uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static bool is_tile(int tx, int ty, int t) {
    if (tx < 0 || ty < 0 || tx >= mnd_w.w || ty >= mnd_w.h) return t == TL_ROCK;
    return mnd_w.tile[ty][tx] == t;
}

static void draw_tiles(int x0, int y0, int x1, int y1) {
    int t = state_t;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++) {
            int px = tx * MND_TILE, py = ty * MND_TILE;
            if (tx < 0 || ty < 0 || tx >= mnd_w.w || ty >= mnd_w.h) {
                gfx_rect(px, py, 8, 8, C_BROWN);
                continue;
            }
            uint32_t h = hash2(tx, ty);
            int tile = mnd_w.tile[ty][tx];
            switch (tile) {
            case TL_ROCK:
                gfx_rect(px, py, 8, 8, C_EARTH);
                if (is_tile(tx - 1, ty, TL_ROCK) || is_tile(tx + 1, ty, TL_ROCK) || is_tile(tx, ty - 1, TL_ROCK) || is_tile(tx, ty + 1, TL_ROCK))
                    spr_draw(&mnd_spr[MS_ROCK2], px, py, h & 1 ? SPR_FLIPX : 0);
                else
                    spr_draw(&mnd_spr[MS_ROCK1], px, py, h & 1 ? SPR_FLIPX : 0);
                if (!is_tile(tx, ty + 1, TL_ROCK)) gfx_hline(px, px + 7, py + 7, C_DUSK);
                break;
            case TL_ROOT:
                spr_draw(&mnd_spr[MS_ROOT], px, py, h & 1 ? SPR_FLIPX : SPR_FLIPY);
                break;
            case TL_WATER: {
                gfx_rect(px, py, 8, 8, C_BLUE);
                int wv = (int)((h >> 3) & 7);
                int yy = (wv + t / 20) % 8;
                gfx_hline(px + (wv & 3), px + (wv & 3) + 2, py + yy, C_SKY);
                if (!is_tile(tx, ty - 1, TL_WATER)) gfx_hline(px, px + 7, py, C_CYAN);
                break;
            }
            case TL_GRASS:
                gfx_rect(px, py, 8, 8, C_EARTH);
                if (h & 2) spr_draw(&mnd_spr[MS_TUFT], px, py, h & 4 ? SPR_FLIPX : 0);
                else gfx_pset(px + (int)(h >> 5 & 7), py + (int)(h >> 9 & 7), C_LEAF);
                break;
            case TL_PEBBLE:
                gfx_rect(px, py, 8, 8, C_EARTH);
                spr_draw(&mnd_spr[MS_PEBBLE], px, py, h & 1 ? SPR_FLIPX : 0);
                break;
            default:
                gfx_rect(px, py, 8, 8, C_EARTH);
                if ((h & 7) == 0) gfx_pset(px + (int)(h >> 5 & 7), py + (int)(h >> 9 & 7), C_TAN);
                if ((h & 31) == 3) gfx_pset(px + (int)(h >> 12 & 7), py + (int)(h >> 16 & 7), C_HIDE);
                break;
            }
        }
}

static int ant_sprite(const MndUnit *u, int *flags) {
    int base = u->kind == MK_WORKER ? MS_WRK_E1 : MS_SOL_E1;
    int frame = (u->anim / 6) & 1;
    static const int VIEW[8] = {0, 2, 4, 2, 0, 2, 4, 2};
    static const int FLAGS[8] = {0, 0, 0, SPR_FLIPX, SPR_FLIPX, SPR_FLIPX | SPR_FLIPY, SPR_FLIPY, SPR_FLIPY};
    int f = u->face & 7;
    *flags = FLAGS[f];
    return base + VIEW[f] + frame;
}

static void draw_unit(const MndUnit *u, int self_side) {
    int x = u->x / FP, y = u->y / FP;
    bool flash = u->hurt > 0 && (u->hurt / 2) % 2;
    const uint8_t *team = MND_TEAM[u->side == MND_RED ? 1 : 0];
    switch (u->kind) {
    case MK_QUEEN: {
        const Sprite *s = &mnd_spr[(u->anim / 20) & 1 ? MS_QUEEN2 : MS_QUEEN1];
        gfx_dither(x - 7, y + 5, 14, 4, C_TAN, 8);
        spr_draw_ex(s, x - 8, y - 8, 0, team, flash ? C_WHITE : -1);
        /* a thin health bar and the beads in store */
        int w = 14 * u->hp / imax(1, u->maxhp);
        gfx_rect(x - 7, y - 11, 14, 2, C_INK);
        gfx_rect(x - 7, y - 11, w, 2, u->side == MND_RED ? C_ORANGE : C_CYAN);
        if (u->gest > 0) gfx_rect(x - 7, y - 13, 14 - 14 * u->gest / 90, 1, C_LIME);
        if (u->ack > 0 && u->side == self_side) {
            gfx_rect(x, y - 19, 1, 3, C_WHITE);
            gfx_pset(x, y - 15, C_WHITE);
        }
        break;
    }
    case MK_SPIDER: {
        const Sprite *s = &mnd_spr[(u->anim / 10) & 1 ? MS_SPIDER2 : MS_SPIDER1];
        gfx_dither(x - 7, y + 4, 14, 5, C_BROWN, 6);
        spr_draw_ex(s, x - 8, y - 8, (u->face >= 3 && u->face <= 5) ? SPR_FLIPX : 0, NULL, flash ? C_WHITE : -1);
        break;
    }
    default: {
        int flags, id = ant_sprite(u, &flags);
        if (u->kind == MK_PLAYER) team = MND_TEAM[u->side == MND_RED ? 1 : 2];
        spr_draw_ex(&mnd_spr[id], x - 4, y - 4, flags, team, flash ? C_WHITE : -1);
        if (u->carry) spr_draw(&mnd_spr[MS_BEAD], x - 2, y - 7, 0);
        if (u->kind == MK_PLAYER) {
            int bob = (state_t / 12) & 1;
            spr_draw_ex(&mnd_spr[MS_CROWN], x - 2, y - 9 - bob - (u->carry ? 3 : 0), 0,
                        u->side == MND_RED && versus ? MND_TEAM[1] : NULL, -1);
        } else if (u->ack > 0 && u->side == self_side) {
            gfx_rect(x, y - 11, 1, 3, C_WHITE);
            gfx_pset(x, y - 7, C_WHITE);
        } else if (u->kind == MK_SOLDIER && u->lock >= 0 && !versus) {
            gfx_pset(x, y - 6, (state_t / 4) & 1 ? C_YELLOW : C_ORANGE);
        }
        break;
    }
    }
}

static void draw_world(int s, int vx, int vw) {
    int cx = iclamp(cam_x[s] - vw / 2, 0, imax(0, mnd_w.w * MND_TILE - vw));
    int cy = iclamp(cam_y[s] - VIEW_H / 2, 0, imax(0, mnd_w.h * MND_TILE - VIEW_H));
    int ox = 0, oy = 0;
    if (mnd_w.w * MND_TILE < vw) ox = (vw - mnd_w.w * MND_TILE) / 2;
    if (mnd_w.h * MND_TILE < VIEW_H) oy = (VIEW_H - mnd_w.h * MND_TILE) / 2;
    int sx = shake > 0 && s == 0 ? ((shake & 1) ? 1 : -1) : 0;
    gfx_clip(vx, VIEW_Y, vw, VIEW_H);
    gfx_rect(vx, VIEW_Y, vw, VIEW_H, C_BROWN);
    gfx_camera(cx - vx - ox + sx, cy - VIEW_Y - oy);
    draw_tiles(cx / MND_TILE - 1, cy / MND_TILE - 1, (cx + vw) / MND_TILE + 1, (cy + VIEW_H) / MND_TILE + 1);
    for (int b = 0; b < mnd_w.n_beads; b++)
        if (mnd_w.bead[b].on) spr_draw(&mnd_spr[MS_BEAD], mnd_w.bead[b].x - 2, mnd_w.bead[b].y - 2, 0);
    /* queens first, then ants, then longlegs on top */
    static const int ORDER[3][2] = {{MK_QUEEN, MK_QUEEN}, {MK_PLAYER, MK_SOLDIER}, {MK_SPIDER, MK_SPIDER}};
    for (int pass = 0; pass < 3; pass++)
        for (int i = 0; i < mnd_w.n_units; i++) {
            const MndUnit *u = &mnd_w.u[i];
            if (u->kind < ORDER[pass][0] || u->kind > ORDER[pass][1]) continue;
            draw_unit(u, s);
        }
    for (int i = 0; i < MND_MAX_SHOTS; i++) {
        const MndShot *sh = &mnd_w.shot[i];
        if (!sh->on) continue;
        int col = sh->side == MND_RED ? C_ORANGE : C_CYAN;
        spr_draw_ex(&mnd_spr[MS_SPIT], sh->x / FP - 1, sh->y / FP - 1, 0, NULL, col);
    }
    /* the shout radius while the menu is open, and a ring as an order goes out */
    int p = mnd_w.player[s];
    if (p >= 0) {
        int px = mnd_w.u[p].x / FP, py = mnd_w.u[p].y / FP;
        if (mnd_w.menu[s].open) {
            for (int a = 0; a < 48; a++) {
                if (((a + state_t / 3) & 3) == 0) continue;
                float ang = (float)a * 6.2832f / 48.0f;
                gfx_pset(px + (int)(cosf(ang) * MND_SHOUT_R), py + (int)(sinf(ang) * MND_SHOUT_R), C_WHITE);
            }
        }
        if (shout_t[s] > 0) gfx_circb(px, py, MND_SHOUT_R - shout_t[s] * 2, C_YELLOW);
        pscr_x[s] = px - gfx_cam_x();
        pscr_y[s] = py - gfx_cam_y();
    }
    /* brawls: a cloud of dust with stars flying out of it */
    for (int i = 0; i < mnd_w.n_units; i++) {
        const MndUnit *a = &mnd_w.u[i];
        if (a->brawl_t <= 0 || a->brawl_with < i) continue;
        const MndUnit *b = &mnd_w.u[a->brawl_with];
        int cx = (a->x + b->x) / 2 / FP, cy = (a->y + b->y) / 2 / FP;
        int t = state_t + i * 7;
        for (int k = 0; k < 6; k++) {
            int ox = (int)(hash2(k, t / 4) % 11) - 5, oy = (int)(hash2(k + 9, t / 4) % 9) - 4;
            gfx_circ(cx + ox, cy + oy, 3 + (k & 1), k & 2 ? C_HIDE : C_TAN);
        }
        for (int k = 0; k < 3; k++) {
            int ph = (t + k * 5) % 16;
            int sx = cx + MND_DX[(k * 3 + t / 16) & 7] * (4 + ph / 2), sy = cy + MND_DY[(k * 3 + t / 16) & 7] * (4 + ph / 2) - 2;
            gfx_pset(sx, sy, C_YELLOW);
            gfx_pset(sx - 1, sy, C_WHITE);
            gfx_pset(sx + 1, sy, C_WHITE);
            gfx_pset(sx, sy - 1, C_WHITE);
            gfx_pset(sx, sy + 1, C_WHITE);
        }
    }
    gfx_camera(0, 0);
    gfx_noclip();
}

/* the command cross around your ant: up, right, down, left */
static void draw_menu(int s, int vx, int vw) {
    const MndMenu *mn = &mnd_w.menu[s];
    if (!mn->open) return;
    int cx = pscr_x[s], cy = pscr_y[s];
    static const int8_t AX[ARM_COUNT] = {0, 1, 0, -1}, AY[ARM_COUNT] = {-1, 0, 1, 0};
    for (int a = 0; a < ARM_COUNT; a++) {
        bool sel = mn->arm == a;
        int slot = sel ? mn->slot : 0;
        int cmd = MND_ARMS[a][slot];
        bool ok = mnd_cmd_ok(&mnd_w, s, cmd);
        const char *label = MND_CMD_NAMES[cmd];
        int w = tiny_width(label) + 6, h = 9;
        int x = cx + AX[a] * 14 - (AX[a] < 0 ? w : AX[a] > 0 ? 0 : w / 2);
        int y = cy + AY[a] * 14 - (AY[a] < 0 ? h : AY[a] > 0 ? 0 : h / 2);
        x = iclamp(x, vx + 1, vx + vw - w - 1);
        y = iclamp(y, VIEW_Y + 1, VIEW_Y + VIEW_H - h - 9);
        ui_panel(x, y, w, h, sel ? C_NAVY : C_NIGHT, sel ? C_YELLOW : C_DUSK);
        tiny_draw(label, x + 3, y + 2, !ok ? C_SLATE : sel ? C_WHITE : C_LIGHT);
        /* a pip per command on this arm, the highlighted one lit */
        int n = mnd_arm_slots(a);
        for (int k = 0; k < n; k++) gfx_rect(x + 3 + k * 3, y + h, 2, 2, sel && k == slot ? C_YELLOW : C_SLATE);
    }
}

static void draw_hud(int s, int x0, int w) {
    char buf[64];
    int p = mnd_w.player[s];
    int q = mnd_queen_of(&mnd_w, s);
    int store = 0;
    for (int k = 0; k < mnd_w.n_units; k++)
        if (mnd_w.u[k].kind == MK_QUEEN && mnd_w.u[k].side == s) store += mnd_w.u[k].store;
    int x = x0 + 3;
    /* your ant's health */
    if (p >= 0) {
        for (int i = 0; i < mnd_w.u[p].maxhp; i++)
            gfx_rect(x + i * 5, 3, 4, 4, i < mnd_w.u[p].hp ? (s ? C_ORANGE : C_CYAN) : C_DUSK);
        x += mnd_w.u[p].maxhp * 5 + 3;
    } else {
        if (mnd_can_respawn(&mnd_w, s)) snprintf(buf, sizeof buf, "HATCHING");
        else snprintf(buf, sizeof buf, mnd_queen_of(&mnd_w, s) < 0 ? "NO QUEEN" : "NO ANTS LEFT");
        tiny_draw(buf, x, 3, (state_t / 10) & 1 ? C_YELLOW : C_AMBER);
        x += tiny_width(buf) + 4;
    }
    spr_draw(&mnd_spr[MS_BEAD], x, 2, 0);
    snprintf(buf, sizeof buf, "%d", store);
    tiny_draw(buf, x + 6, 3, C_LIME);
    x += 6 + tiny_width(buf) + 4;
    if (q >= 0) {
        tiny_draw(mnd_w.u[q].prod == PROD_SOLDIER ? "SOLDIERS" : "WORKERS", x, 3, C_LIGHT);
    }
    /* how many are left on each side */
    int mine = mnd_count(&mnd_w, s, MK_NONE), theirs = mnd_count(&mnd_w, s ^ 1, MK_NONE);
    snprintf(buf, sizeof buf, "%d VS %d", mine, theirs);
    tiny_draw(buf, x0 + w - 3 - tiny_width(buf), 3, C_WHITE);
    (void)x;
}

static void draw_foot(void) {
    gfx_rect(0, FOOT_Y, SCREEN_W, SCREEN_H - FOOT_Y, C_INK);
    char buf[80];
    /* the button hint, the way the original shows its own at the bottom */
    for (int s = 0; s < (versus ? 2 : 1); s++) {
        int fx = s ? 164 : 3;
        fx = tiny_draw("HOLD", fx, FOOT_Y + 2, C_GREY) + 3;
        fx = text_draw(MND_GLYPH_ORDER, fx, FOOT_Y, C_WHITE) + 2;
        fx = tiny_draw("ORDERS", fx, FOOT_Y + 2, C_GREY) + 6;
        fx = text_draw(MND_GLYPH_SPIT, fx, FOOT_Y, C_WHITE) + 2;
        tiny_draw("SPIT", fx, FOOT_Y + 2, C_GREY);
    }
    if (versus) return;
    const MndMenu *mn = &mnd_w.menu[0];
    if (mn->flash_t > 0) {
        snprintf(buf, sizeof buf, "\"%s!\"", MND_CMD_NAMES[iclamp(mn->flash, 0, CMD_COUNT - 1)]);
        tiny_center(buf, 170, FOOT_Y + 2, C_YELLOW);
    }
    tiny_draw(MND_MAPS[cur_map].name, SCREEN_W - 3 - tiny_width(MND_MAPS[cur_map].name), FOOT_Y + 2, C_SLATE);
}

static void draw_play(void) {
    gfx_cls(C_INK);
    if (versus) {
        for (int s = 0; s < 2; s++) {
            int vx = s ? 161 : 0;
            draw_world(s, vx, 159);
            draw_menu(s, vx, 159);
            gfx_rect(vx, 0, 159, VIEW_Y, C_NIGHT);
            draw_hud(s, vx, 159);
        }
        gfx_rect(159, 0, 2, FOOT_Y, C_INK);
    } else {
        draw_world(0, 0, SCREEN_W);
        draw_menu(0, 0, SCREEN_W);
        gfx_rect(0, 0, SCREEN_W, VIEW_Y, C_NIGHT);
        draw_hud(0, 0, SCREEN_W);
    }
    draw_foot();
}

/* ------------------------------------------------------------------ */
/* menus and screens                                                    */

static void meadow(int t) {
    for (int y = 0; y < SCREEN_H; y += 8)
        for (int x = 0; x < SCREEN_W; x += 8) {
            uint32_t h = hash2(x / 8, y / 8);
            gfx_rect(x, y, 8, 8, C_EARTH);
            if ((h & 3) == 0) spr_draw(&mnd_spr[MS_TUFT], x, y, h & 4 ? SPR_FLIPX : 0);
            else if ((h & 15) == 5) gfx_pset(x + 3, y + 4, C_TAN);
        }
    (void)t;
}

static void draw_ant_at(int kind, int side, int x, int y, int face, int anim, int scale) {
    MndUnit u;
    memset(&u, 0, sizeof u);
    u.kind = (uint8_t)kind;
    u.side = (uint8_t)side;
    u.face = (int8_t)face;
    u.anim = (int16_t)anim;
    int flags, id = ant_sprite(&u, &flags);
    if (scale <= 1) spr_draw_ex(&mnd_spr[id], x, y, flags, MND_TEAM[side], -1);
    else {
        /* scaled with the team colours */
        const Sprite *s = &mnd_spr[id];
        for (int yy = 0; yy < s->h; yy++)
            for (int xx = 0; xx < s->w; xx++) {
                int sx = flags & SPR_FLIPX ? s->w - 1 - xx : xx, sy = flags & SPR_FLIPY ? s->h - 1 - yy : yy;
                uint8_t c = s->px[sy * s->w + sx];
                if (c == TRANSPARENT) continue;
                gfx_rect(x + xx * scale, y + yy * scale, scale, scale, MND_TEAM[side][c]);
            }
    }
}

static void draw_sprite_scaled_team(const Sprite *s, int x, int y, int scale, const uint8_t *team, int flags) {
    for (int yy = 0; yy < s->h; yy++)
        for (int xx = 0; xx < s->w; xx++) {
            int sx = flags & SPR_FLIPX ? s->w - 1 - xx : xx;
            uint8_t c = s->px[yy * s->w + sx];
            if (c == TRANSPARENT) continue;
            gfx_rect(x + xx * scale, y + yy * scale, scale, scale, team ? team[c] : c);
        }
}

static void draw_title(void) {
    meadow(state_t);
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_BROWN, 6);
    /* a column of blue ants marching past, a red one the other way */
    for (int i = 0; i < 7; i++) {
        int x = ((state_t / 2 + i * 50) % 380) - 30;
        draw_ant_at(i == 3 ? MK_WORKER : MK_SOLDIER, 0, x, 150, 0, state_t + i * 3, 2);
    }
    int rx = SCREEN_W - ((state_t / 3) % 400);
    draw_ant_at(MK_SOLDIER, 1, rx, 128, 4, state_t, 2);
    static const uint8_t grad[] = {C_CYAN, C_SKY, C_BLUE};
    ui_fancy_center("MANDIBLES", SCREEN_W / 2, 24, 3, grad, 3, C_INK, C_NAVY);
    text_center_shadow("THE BLUEBELL COLONY STRIKES BACK", SCREEN_W / 2, 52, C_CREAM, C_INK);
    static const char *const ITEMS[2] = {"CAMPAIGN", "2P VERSUS"};
    for (int i = 0; i < 2; i++) {
        int y = 76 + i * 14;
        bool sel = title_sel == i;
        bool locked = i == 1 && vita_single();
        ui_panel(110, y - 3, 100, 13, sel ? C_NAVY : C_NIGHT, sel ? C_CYAN : C_DUSK);
        char buf[32];
        snprintf(buf, sizeof buf, "%s%s", ITEMS[i], locked ? " " GLYPH_LOCK : "");
        text_center(buf, SCREEN_W / 2, y, locked ? C_SLATE : sel ? C_WHITE : C_GREY);
    }
    char buf[64];
    ui_panel(70, 104, 180, 21, C_NIGHT, C_DUSK);
    snprintf(buf, sizeof buf, "FIELDS TAKEN %d/12   LONGLEGS SLAIN %d", won_count(), sv.spiders);
    tiny_center(buf, SCREEN_W / 2, 108, C_CREAM);
    tiny_center("HOLD " "A" ": ORDERS    " "B" ": SPIT    START: PAUSE", SCREEN_W / 2, 117, C_LIGHT);
    gfx_rect(110, 168, 100, 9, C_NIGHT);
    tiny_center("BEAMDOWN SOFTWORKS 1989", SCREEN_W / 2, 170, C_HIDE);
}

static void draw_map(void) {
    meadow(0);
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_TAN, 5);
    /* the brook and the old oak, for looks */
    for (int y = 0; y < SCREEN_H; y++) {
        int x = 190 + (int)(sinf((float)y * 0.05f) * 10.0f);
        gfx_rect(x, y, 4, 1, C_BLUE);
        gfx_pset(x + 1 + ((y + state_t / 8) % 3), y, C_SKY);
    }
    gfx_rect(0, 0, SCREEN_W, 14, C_NIGHT);
    static const uint8_t grad[] = {C_CYAN, C_SKY};
    ui_fancy_text("THE ROAD TO THE CAPITAL", 6, 3, 1, grad, 2, C_INK, -1);
    /* roads */
    for (int i = 0; i < MND_MISSIONS; i++) {
        const MndMap *m = &MND_MAPS[i];
        for (int k = 0; k < 3; k++) {
            int n = m->next[k] - 1;
            if (n < 0 || (n == MND_BONUS && !map_open(MND_BONUS))) continue;
            const MndMap *o = &MND_MAPS[n];
            bool lit = map_won(i);
            int steps = imax(iabs(o->map_x - m->map_x), iabs(o->map_y - m->map_y)) / 3;
            for (int sidx = 0; sidx <= steps; sidx++) {
                int x = m->map_x + (o->map_x - m->map_x) * sidx / imax(1, steps);
                int y = m->map_y + (o->map_y - m->map_y) * sidx / imax(1, steps);
                gfx_rect(x, y, 2, 2, lit ? C_CREAM : C_BROWN);
            }
        }
    }
    for (int i = 0; i <= MND_BONUS; i++) {
        if (i == MND_BONUS && !map_open(i)) continue;
        const MndMap *m = &MND_MAPS[i];
        bool open = map_open(i), won = map_won(i);
        int col = won ? C_BLUE : open ? C_RED : C_SLATE;
        gfx_circ(m->map_x, m->map_y, 7, C_INK);
        gfx_circ(m->map_x, m->map_y, 6, col);
        if (i == MND_MISSIONS - 1) spr_draw(&mnd_spr[MS_CROWN], m->map_x - 2, m->map_y - 12, 0);
        char num[8];
        if (i == MND_BONUS) snprintf(num, sizeof num, "?");
        else snprintf(num, sizeof num, "%d", i + 1);
        tiny_center(num, m->map_x, m->map_y - 2, open || won ? C_WHITE : C_DUSK);
    }
    /* your leader walks the road from field to field */
    {
        const MndMap *a = &MND_MAPS[walk_from], *b = &MND_MAPS[map_sel];
        int t = imin(walk_t, 30);
        int lx = a->map_x + (b->map_x - a->map_x) * t / 30, ly = a->map_y + (b->map_y - a->map_y) * t / 30;
        int face = t < 30 ? mnd_octant(b->map_x - a->map_x, b->map_y - a->map_y) : 2;
        if (face < 0) face = 2;
        gfx_circb(b->map_x, b->map_y, 9 + ((state_t / 10) & 1), C_YELLOW);
        MndUnit lead;
        memset(&lead, 0, sizeof lead);
        lead.kind = MK_PLAYER;
        lead.face = (int8_t)face;
        lead.anim = (int16_t)(t < 30 ? state_t : 0);
        int flags, id = ant_sprite(&lead, &flags);
        spr_draw_ex(&mnd_spr[id], lx - 4, ly - 12, flags, MND_TEAM[2], -1);
        spr_draw(&mnd_spr[MS_CROWN], lx - 2, ly - 16, 0);
    }
    const MndMap *m = &MND_MAPS[map_sel];
    gfx_rect(0, 158, SCREEN_W, 22, C_NIGHT);
    char buf[80];
    snprintf(buf, sizeof buf, "%s%s", m->name, map_won(map_sel) ? "  " GLYPH_CHECK " TAKEN" : "");
    text_draw(buf, 6, 160, C_WHITE);
    int fx = ui_hint(6, 170, GLYPH_A, "BRIEFING", C_LIGHT);
    ui_hint(fx, 170, GLYPH_B, "TITLE", C_LIGHT);
    snprintf(buf, sizeof buf, "%d/12 TAKEN", won_count());
    text_draw(buf, SCREEN_W - 6 - text_width(buf), 170, C_GREY);
}

static int count_in_map(int map, char c) {
    int n = 0;
    for (const char *const *r = MND_MAPS[map].rows; *r; r++)
        for (const char *p = *r; *p; p++) n += *p == c;
    return n;
}

static void draw_brief(void) {
    gfx_cls(C_NIGHT);
    ui_panel(8, 8, 304, 164, C_INK, C_DUSK);
    draw_sprite_scaled_team(&mnd_spr[MS_GENERAL], 18, 18, 3, NULL, 0);
    tiny_draw("GENERAL STAG", 18, 68, C_YELLOW);
    const MndMap *m = &MND_MAPS[map_sel];
    char buf[80];
    if (map_sel == MND_BONUS) snprintf(buf, sizeof buf, "BONUS MISSION");
    else snprintf(buf, sizeof buf, "MISSION %d", map_sel + 1);
    tiny_draw(buf, 74, 18, C_SKY);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_text(m->name, 74, 26, 1, grad, 3, C_INK, -1);
    /* the letter types itself out */
    int shown = imin((int)strlen(m->brief), state_t);
    char text[400];
    snprintf(text, sizeof text, "%.*s", shown, m->brief);
    text_wrap(text, 74, 42, 228, C_LIGHT, 10);
    /* the field's size and what each side starts with */
    int fw = 0, fh = 0;
    while (m->rows[fh]) { fw = imax(fw, (int)strlen(m->rows[fh])); fh++; }
    snprintf(buf, sizeof buf, "FIELD %dx%d", fw, fh);
    tiny_draw(buf, 18, 118, C_LIGHT);
    snprintf(buf, sizeof buf, "BLUE: YOU, QUEEN, %d WORKER%s, %d SOLDIER%s", count_in_map(map_sel, 'W'),
             count_in_map(map_sel, 'W') == 1 ? "" : "S", count_in_map(map_sel, 'S'), count_in_map(map_sel, 'S') == 1 ? "" : "S");
    tiny_draw(buf, 18, 127, C_CYAN);
    snprintf(buf, sizeof buf, "RED: %d QUEEN%s, %d WORKER%s, %d SOLDIER%s", count_in_map(map_sel, 'q'),
             count_in_map(map_sel, 'q') == 1 ? "" : "S", count_in_map(map_sel, 'w'), count_in_map(map_sel, 'w') == 1 ? "" : "S",
             count_in_map(map_sel, 's'), count_in_map(map_sel, 's') == 1 ? "" : "S");
    tiny_draw(buf, 18, 136, C_ORANGE);
    if (count_in_map(map_sel, 'X')) {
        snprintf(buf, sizeof buf, "LONGLEGS: %d", count_in_map(map_sel, 'X'));
        tiny_draw(buf, 18, 145, C_GREY);
    }
    int fx = ui_hint(18, 156, GLYPH_A, "MARCH", C_LIGHT);
    ui_hint(fx, 156, GLYPH_B, "MAP", C_LIGHT);
}

static void draw_result(void) {
    draw_play();
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(60, 40, 200, 96, C_INK, C_GREY);
    char buf[80];
    const char *head;
    if (versus) head = result_status == MND_WON ? "BLUE TAKES THE FIELD" : "RED TAKES THE FIELD";
    else head = result_status == MND_WON ? "FIELD TAKEN!" : "THE COLONY FALLS BACK";
    static const uint8_t gw[] = {C_WHITE, C_CYAN}, gl[] = {C_WHITE, C_ORANGE};
    ui_fancy_center(head, SCREEN_W / 2, 48, 1, result_status == MND_WON ? gw : gl, 2, C_INK, -1);
    int y = 66;
    snprintf(buf, sizeof buf, "RED ANTS DOWN     %d", mnd_w.losses[MND_RED]);
    tiny_draw(buf, 80, y, C_LIGHT); y += 8;
    snprintf(buf, sizeof buf, "BLUE ANTS LOST    %d", mnd_w.losses[MND_BLUE]);
    tiny_draw(buf, 80, y, C_LIGHT); y += 8;
    snprintf(buf, sizeof buf, "SAP CARRIED HOME  %d", mnd_w.delivered[MND_BLUE]);
    tiny_draw(buf, 80, y, C_LIGHT); y += 8;
    snprintf(buf, sizeof buf, "TIME              %d:%02d", mnd_w.frame / 3600, mnd_w.frame / 60 % 60);
    tiny_draw(buf, 80, y, C_LIGHT); y += 8;
    if (versus) {
        snprintf(buf, sizeof buf, "WINS  BLUE %d  RED %d", vs_wins[0], vs_wins[1]);
        tiny_draw(buf, 80, y, C_YELLOW);
    } else if (mnd_w.spiders_slain) {
        snprintf(buf, sizeof buf, "LONGLEGS SLAIN    %d", mnd_w.spiders_slain);
        tiny_draw(buf, 80, y, C_YELLOW);
    }
    if (state_t > 40) {
        if (versus) ui_hint(80, 122, GLYPH_A, "AGAIN", C_WHITE);
        else if (result_status == MND_WON) ui_hint(80, 122, GLYPH_A, "ONWARD", C_WHITE);
        else { int fx = ui_hint(80, 122, GLYPH_A, "TRY AGAIN", C_WHITE); ui_hint(fx, 122, GLYPH_B, "MAP", C_WHITE); }
    }
}

static void draw_vs_pick(void) {
    meadow(0);
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_BROWN, 8);
    static const uint8_t grad[] = {C_YELLOW, C_AMBER};
    ui_fancy_center("2P VERSUS", SCREEN_W / 2, 10, 2, grad, 2, C_INK, C_WINE);
    tiny_center("BLUE AGAINST RED, THE SAME ANTS ON BOTH SIDES", SCREEN_W / 2, 32, C_CREAM);
    const MndMap *m = &MND_MAPS[MND_MISSIONS + 1 + vs_sel];
    /* a tiny picture of the field */
    int h = 0, w = 0;
    while (m->rows[h]) { w = imax(w, (int)strlen(m->rows[h])); h++; }
    int ox = SCREEN_W / 2 - w, oy = 48;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < (int)strlen(m->rows[y]); x++) {
            char c = m->rows[y][x];
            int col = c == '#' ? C_SLATE : c == '~' ? C_BLUE : c == 'o' ? C_LIME : c == 'Q' || c == 'P' || c == 'W' ? C_CYAN
                    : c == 'q' || c == 'p' || c == 'w' ? C_ORANGE : c == 'X' ? C_INK : C_EARTH;
            gfx_rect(ox + x * 2, oy + y * 2, 2, 2, col);
        }
    char buf[64];
    snprintf(buf, sizeof buf, GLYPH_LEFT " %s " GLYPH_RIGHT, m->name);
    text_center(buf, SCREEN_W / 2, oy + h * 2 + 8, C_WHITE);
    tiny_center("PLAYER 1: WASD + F/G    PLAYER 2: ARROWS + K/L", SCREEN_W / 2, 150, C_LIGHT);
    int fx = ui_hint(90, 166, GLYPH_A, "FIGHT", C_WHITE);
    ui_hint(fx, 166, GLYPH_B, "BACK", C_WHITE);
}

static void draw_ending(void) {
    meadow(state_t);
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_NIGHT, 10);
    static const uint8_t grad[] = {C_CYAN, C_SKY, C_BLUE};
    ui_fancy_center("THE CAPITAL IS OURS", SCREEN_W / 2, 20, 2, grad, 3, C_INK, C_NAVY);
    text_wrap("THE OLD COLONY'S HALLS ARE BLUE AGAIN. THE QUEEN MOVES BACK IN, THE SAP RUNS, AND GENERAL STAG "
              "WRITES A VERY LONG REPORT ABOUT HIS OWN BRILLIANCE. NOBODY READS IT.",
              40, 50, 240, C_CREAM, 10);
    for (int i = 0; i < 9; i++) draw_ant_at(i % 3 ? MK_SOLDIER : MK_WORKER, 0, 40 + i * 28, 118 + ((state_t / 10 + i) & 1), 6, state_t + i, 2);
    tiny_center("MANDIBLES - BEAMDOWN SOFTWORKS 1989", SCREEN_W / 2, 150, C_LIGHT);
    if (state_t > 120) tiny_center("PRESS A", SCREEN_W / 2, 164, (state_t / 20) & 1 ? C_WHITE : C_GREY);
}

static void mnd_draw(void) {
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_MAP: draw_map(); break;
    case S_BRIEF: draw_brief(); break;
    case S_PLAY: draw_play(); break;
    case S_RESULT: draw_result(); break;
    case S_VS_PICK: draw_vs_pick(); break;
    case S_ENDING: draw_ending(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void mnd_load(void) {
    mnd_art_load();
    mnd_audio_load();
}

static void mnd_start(void) {
    rng_seed(&seeds, g_rng.state ^ 0xA7711Dull);
    load_save();
    title_sel = 0;
    map_sel = sv.cursor <= MND_BONUS ? sv.cursor : 0;
    vs_sel = 0;
    vs_wins[0] = vs_wins[1] = 0;
    versus = false;
    check_goals();
    go_title();
}

static void mnd_quit(void) {
    input_set_versus(false);
    save_now();
}

static void mnd_label(int x, int y, int w, int h, int t) {
    for (int yy = 0; yy < h; yy += 8)
        for (int xx = 0; xx < w; xx += 8) {
            uint32_t hh = hash2(xx / 8, yy / 8);
            gfx_rect(x + xx, y + yy, 8, 8, C_EARTH);
            if ((hh & 3) == 0) spr_draw(&mnd_spr[MS_TUFT], x + xx, y + yy, hh & 4 ? SPR_FLIPX : 0);
        }
    /* a longlegs looms over a blue squad; a red ant spits back */
    draw_sprite_scaled_team(&mnd_spr[(t / 12) & 1 ? MS_SPIDER2 : MS_SPIDER1], x + w - 52, y + 2, 3, NULL, 0);
    for (int i = 0; i < 3; i++) draw_ant_at(MK_SOLDIER, 0, x + 6 + i * 18, y + 18 + (i & 1) * 14, 0, t + i * 4, 2);
    draw_ant_at(MK_SOLDIER, 1, x + 70, y + 26, 4, t, 2);
    int sx = x + 24 + (t % 40);
    if (sx < x + 66) gfx_rect(sx, y + 33, 3, 3, C_CYAN);
    for (int i = 0; i < 4; i++) spr_draw(&mnd_spr[MS_BEAD], x + 10 + i * 9, y + 50, 0);
}

static int unit_field(const char *f, const MndUnit *u, int *out) {
    if (!strcmp(f, "kind")) *out = u->kind;
    else if (!strcmp(f, "side")) *out = u->side;
    else if (!strcmp(f, "hp")) *out = u->hp;
    else if (!strcmp(f, "x")) *out = u->x / FP;
    else if (!strcmp(f, "y")) *out = u->y / FP;
    else if (!strcmp(f, "order")) *out = u->order;
    else if (!strcmp(f, "carry")) *out = u->carry;
    else if (!strcmp(f, "lock")) *out = u->lock;
    else if (!strcmp(f, "face")) *out = u->face;
    else if (!strcmp(f, "store")) *out = u->store;
    else if (!strcmp(f, "prod")) *out = u->prod;
    else if (!strcmp(f, "ack")) *out = u->ack > 0;
    else if (!strcmp(f, "brawl")) *out = u->brawl_t > 0;
    else if (!strcmp(f, "sticky")) *out = u->sticky >= 0;
    else return 0;
    return 1;
}

static int mnd_query(const char *key, int *out) {
    const MndWorld *w = &mnd_w;
    int p = w->player[0];
    if (!strcmp(key, "bot")) {
        if (state == S_PLAY) *out = mnd_bot_buttons(w, 0);
        else if (state == S_RESULT) *out = state_t > 45 && (state_t / 6) % 2 ? BTN_A : 0;
        else if (state == S_BRIEF || state == S_ENDING) *out = state_t > 125 && (state_t / 6) % 2 ? BTN_A : 0;
        else *out = 0;
        return 1;
    }
    if (!strcmp(key, "bot_target")) { *out = mnd_bot_debug(0); return 1; }
    if (!strcmp(key, "bot_rule")) { *out = mnd_bot_debug(1); return 1; }
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "status")) { *out = w->status; return 1; }
    if (!strcmp(key, "result")) { *out = state == S_RESULT ? result_status : 0; return 1; }
    if (!strcmp(key, "map")) { *out = cur_map + 1; return 1; }
    if (!strcmp(key, "map_sel")) { *out = map_sel + 1; return 1; }
    if (!strcmp(key, "map_open")) { *out = 0; for (int i = 0; i <= MND_BONUS; i++) *out |= map_open(i) << i; return 1; }
    if (!strcmp(key, "won")) { *out = sv.won; return 1; }
    if (!strcmp(key, "won_count")) { *out = won_count(); return 1; }
    if (!strcmp(key, "spiders_saved")) { *out = sv.spiders; return 1; }
    if (!strcmp(key, "title_sel")) { *out = title_sel; return 1; }
    if (!strcmp(key, "vs_sel")) { *out = vs_sel; return 1; }
    if (!strcmp(key, "versus")) { *out = versus; return 1; }
    if (!strcmp(key, "input_versus")) { *out = input_versus(); return 1; }
    if (!strcmp(key, "field_t")) { *out = w->frame; return 1; } /* "frame" is the console's */
    if (!strcmp(key, "alive")) { *out = p >= 0; return 1; }
    if (!strcmp(key, "hp")) { *out = p >= 0 ? w->u[p].hp : 0; return 1; }
    if (!strcmp(key, "px")) { *out = p >= 0 ? w->u[p].x / FP : -1; return 1; }
    if (!strcmp(key, "py")) { *out = p >= 0 ? w->u[p].y / FP : -1; return 1; }
    if (!strcmp(key, "face")) { *out = p >= 0 ? w->u[p].face : -1; return 1; }
    if (!strcmp(key, "carry")) { *out = p >= 0 ? w->u[p].carry : 0; return 1; }
    if (!strcmp(key, "player")) { *out = p; return 1; }
    if (!strcmp(key, "p2")) { *out = w->player[1]; return 1; }
    if (!strcmp(key, "p2x")) { *out = w->player[1] >= 0 ? w->u[w->player[1]].x / FP : -1; return 1; }
    if (!strcmp(key, "p2hp")) { *out = w->player[1] >= 0 ? w->u[w->player[1]].hp : 0; return 1; }
    if (!strcmp(key, "red_queen_hp")) {
        *out = 0;
        for (int i = 0; i < w->n_units; i++)
            if (w->u[i].kind == MK_QUEEN && w->u[i].side == MND_RED) { *out = w->u[i].hp; break; }
        return 1;
    }
    if (!strcmp(key, "red_worker_hp")) {
        *out = 0;
        for (int i = 0; i < w->n_units; i++)
            if (w->u[i].kind == MK_WORKER && w->u[i].side == MND_RED) { *out = w->u[i].hp; break; }
        return 1;
    }
    if (!strcmp(key, "dead_t")) { *out = w->dead_t[0]; return 1; }
    if (!strcmp(key, "queen")) { *out = mnd_queen_of(w, MND_BLUE); return 1; }
    if (!strcmp(key, "queen_hp")) { int q = mnd_queen_of(w, MND_BLUE); *out = q >= 0 ? w->u[q].hp : 0; return 1; }
    if (!strcmp(key, "store")) { int q = mnd_queen_of(w, MND_BLUE); *out = q >= 0 ? w->u[q].store : -1; return 1; }
    if (!strcmp(key, "prod")) { int q = mnd_queen_of(w, MND_BLUE); *out = q >= 0 ? w->u[q].prod : -1; return 1; }
    /* the command menu ("menu_sel" belongs to the console's main menu) */
    if (!strcmp(key, "cmd_open")) { *out = w->menu[0].open; return 1; }
    if (!strcmp(key, "cmd")) { *out = mnd_menu_cmd(&w->menu[0]); return 1; }
    if (!strcmp(key, "cmd_arm")) { *out = w->menu[0].arm; return 1; }
    if (!strcmp(key, "cmd_slot")) { *out = w->menu[0].slot; return 1; }
    if (!strcmp(key, "cmd_last")) { *out = w->menu[0].last; return 1; }
    if (!strcmp(key, "cmd2_open")) { *out = w->menu[1].open; return 1; }
    if (!strcmp(key, "cmd2")) { *out = mnd_menu_cmd(&w->menu[1]); return 1; }
    if (!strcmp(key, "blue")) { *out = mnd_count(w, MND_BLUE, MK_NONE); return 1; }
    if (!strcmp(key, "red")) { *out = mnd_count(w, MND_RED, MK_NONE); return 1; }
    if (!strcmp(key, "red_queens")) { *out = mnd_count(w, MND_RED, MK_QUEEN); return 1; }
    if (!strcmp(key, "red_workers")) { *out = mnd_count(w, MND_RED, MK_WORKER); return 1; }
    if (!strcmp(key, "red_soldiers")) { *out = mnd_count(w, MND_RED, MK_SOLDIER); return 1; }
    if (!strcmp(key, "red_scouts") || !strcmp(key, "red_sticky")) {
        int n = 0;
        for (int i = 0; i < w->n_units; i++)
            if (w->u[i].kind && w->u[i].side == MND_RED) n += key[4] == 's' && key[5] == 'c' ? w->u[i].scout : w->u[i].sticky >= 0;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "blue_workers")) { *out = mnd_count(w, MND_BLUE, MK_WORKER); return 1; }
    if (!strcmp(key, "blue_soldiers")) { *out = mnd_count(w, MND_BLUE, MK_SOLDIER); return 1; }
    if (!strcmp(key, "spiders")) { *out = mnd_count(w, MND_WILD, MK_SPIDER); return 1; }
    if (!strcmp(key, "spiders_slain")) { *out = w->spiders_slain; return 1; }
    if (!strcmp(key, "beads")) { *out = mnd_beads_left(w); return 1; }
    if (!strcmp(key, "delivered")) { *out = w->delivered[0]; return 1; }
    if (!strcmp(key, "respawns")) { *out = w->respawns[0]; return 1; }
    if (!strcmp(key, "kills")) { *out = w->kills[0]; return 1; }
    if (!strcmp(key, "losses")) { *out = w->losses[0]; return 1; }
    if (!strcmp(key, "units")) { *out = w->n_units; return 1; }
    if (!strcmp(key, "tags_alive") || !strcmp(key, "tags_hp")) {
        /* how many of the units a test placed are left, and their health together */
        int n = 0, hp = 0;
        for (int k = 0; k < n_tags; k++)
            if (w->u[tags[k]].kind) { n++; hp += w->u[tags[k]].hp; }
        *out = key[5] == 'a' ? n : hp;
        return 1;
    }
    if (!strcmp(key, "shots")) { int n = 0; for (int i = 0; i < MND_MAX_SHOTS; i++) n += w->shot[i].on; *out = n; return 1; }
    if (!strcmp(key, "follow")) {
        int n = 0;
        for (int i = 0; i < w->n_units; i++) n += (w->u[i].kind == MK_WORKER || w->u[i].kind == MK_SOLDIER) && w->u[i].side == 0 && w->u[i].order == ORD_FOLLOW;
        *out = n;
        return 1;
    }
    if (!strcmp(key, "vs_wins_blue")) { *out = vs_wins[0]; return 1; }
    if (!strcmp(key, "vs_wins_red")) { *out = vs_wins[1]; return 1; }
    /* tK_FIELD: the K-th unit a test added (tK_alive: still there) */
    if (key[0] == 't' && key[1] >= '0' && key[1] <= '7' && key[2] == '_') {
        int k = key[1] - '0';
        if (k >= n_tags) return 0;
        const MndUnit *u = &w->u[tags[k]];
        if (!strcmp(key + 3, "alive")) { *out = u->kind != MK_NONE; return 1; }
        return unit_field(key + 3, u, out);
    }
    /* u_FIELD_I: unit I's field */
    if (!strncmp(key, "u_", 2)) {
        const char *us = strrchr(key, '_');
        if (!us || us == key + 1) return 0;
        int i = atoi(us + 1);
        if (i < 0 || i >= MND_MAX_UNITS) return 0;
        char f[16];
        int n = (int)(us - key - 2);
        if (n <= 0 || n >= (int)sizeof f) return 0;
        memcpy(f, key + 2, (size_t)n);
        f[n] = 0;
        return unit_field(f, &w->u[i], out);
    }
    return 0;
}

static int mnd_cheat(const char *cmd) {
    int a, b, c, d, e;
    if (sscanf(cmd, "mission %d %d", &a, &b) == 2) { map_sel = iclamp(a - 1, 0, MND_BONUS); start_field(map_sel, (uint64_t)b, false); return 1; }
    if (sscanf(cmd, "mission %d", &a) == 1) { map_sel = iclamp(a - 1, 0, MND_BONUS); start_field(map_sel, new_seed(), false); return 1; }
    if (sscanf(cmd, "versus %d %d", &a, &b) == 2) { vs_sel = iclamp(a - 1, 0, MND_VS_MAPS - 1); start_field(MND_MISSIONS + 1 + vs_sel, (uint64_t)b, true); return 1; }
    if (sscanf(cmd, "won %d", &a) == 1) { sv.won = (uint16_t)a; save_now(); check_goals(); return 1; }
    if (!strcmp(cmd, "clear")) {
        /* an empty field for rule tests: only the two players' ants and the queens stay */
        n_tags = 0;
        for (int i = 0; i < mnd_w.n_units; i++) {
            MndUnit *u = &mnd_w.u[i];
            if (u->kind && u->kind != MK_PLAYER && u->kind != MK_QUEEN) u->kind = MK_NONE;
        }
        for (int i = 0; i < mnd_w.n_beads; i++) mnd_w.bead[i].on = 0;
        for (int i = 0; i < MND_MAX_SHOTS; i++) mnd_w.shot[i].on = 0;
        return 1;
    }
    if (!strcmp(cmd, "clear_queens")) {
        for (int i = 0; i < mnd_w.n_units; i++)
            if (mnd_w.u[i].kind == MK_QUEEN && mnd_w.u[i].side == MND_RED) mnd_w.u[i].kind = MK_NONE;
        return 1;
    }
    int n = sscanf(cmd, "unit %d %d %d %d %d", &a, &b, &c, &d, &e);
    if (n >= 4) {
        /* unit KIND SIDE X Y [HP]: the tests call them t0, t1 ... in order */
        int i = mnd_add_unit(&mnd_w, a, b, c, d);
        if (i >= 0 && n == 5) mnd_w.u[i].hp = (int16_t)e;
        if (i >= 0 && n_tags < ARRAY_LEN(tags)) tags[n_tags++] = i;
        return i >= 0;
    }
    if (sscanf(cmd, "tag_order %d %d", &a, &b) == 2 && a >= 0 && a < n_tags) { mnd_w.u[tags[a]].order = (uint8_t)b; return 1; }
    if (sscanf(cmd, "tag_hp %d %d", &a, &b) == 2 && a >= 0 && a < n_tags) { mnd_w.u[tags[a]].hp = (int16_t)b; return 1; }
    if (sscanf(cmd, "tag_carry %d", &a) == 1 && a >= 0 && a < n_tags) { mnd_w.u[tags[a]].carry = 1; return 1; }
    if (sscanf(cmd, "tag_remove %d", &a) == 1 && a >= 0 && a < n_tags) { mnd_w.u[tags[a]].kind = MK_NONE; return 1; }
    if (sscanf(cmd, "tag_at %d %d %d", &a, &b, &c) == 3 && a >= 0 && a < n_tags) {
        mnd_w.u[tags[a]].x = b * FP;
        mnd_w.u[tags[a]].y = c * FP;
        mnd_w.u[tags[a]].step = 0;
        return 1;
    }
    if (sscanf(cmd, "bead %d %d", &a, &b) == 2) {
        for (int i = 0; i < MND_MAX_BEADS; i++)
            if (!mnd_w.bead[i].on) {
                mnd_w.bead[i] = (MndBead){1, (int16_t)a, (int16_t)b};
                if (i >= mnd_w.n_beads) mnd_w.n_beads = i + 1;
                return 1;
            }
        return 0;
    }
    if (sscanf(cmd, "at %d %d", &a, &b) == 2) {
        int p = mnd_w.player[0];
        if (p < 0) return 0;
        mnd_w.u[p].x = a * FP;
        mnd_w.u[p].y = b * FP;
        return 1;
    }
    if (sscanf(cmd, "set %d hp %d", &a, &b) == 2 && a >= 0 && a < MND_MAX_UNITS) { mnd_w.u[a].hp = (int16_t)b; return 1; }
    if (sscanf(cmd, "set %d order %d", &a, &b) == 2 && a >= 0 && a < MND_MAX_UNITS) { mnd_w.u[a].order = (uint8_t)b; return 1; }
    /* a queen's store (her egg in progress is dropped) */
    if (sscanf(cmd, "store %d", &a) == 1) {
        int q = mnd_queen_of(&mnd_w, 0);
        if (q < 0) return 0;
        mnd_w.u[q].store = (int16_t)a;
        mnd_w.u[q].gest = 0;
        return 1;
    }
    if (sscanf(cmd, "red_store %d", &a) == 1) {
        for (int i = 0; i < mnd_w.n_units; i++)
            if (mnd_w.u[i].kind == MK_QUEEN && mnd_w.u[i].side == MND_RED) { mnd_w.u[i].store = (int16_t)a; mnd_w.u[i].gest = 0; }
        return 1;
    }
    if (sscanf(cmd, "mistakes %d", &a) == 1) { mnd_w.red_mistake = (uint8_t)a; return 1; }
    if (sscanf(cmd, "queen_hp %d %d", &a, &b) == 2) {
        /* queen_hp SIDE HP: every queen of that side */
        for (int i = 0; i < mnd_w.n_units; i++)
            if (mnd_w.u[i].kind == MK_QUEEN && mnd_w.u[i].side == a) mnd_w.u[i].hp = (int16_t)b;
        return 1;
    }
    if (sscanf(cmd, "seed %d", &a) == 1) { rng_seed(&mnd_w.rng, (uint64_t)a); return 1; }
    return 0;
}

const GameDef GAME_MANDIBLES = {
    "mandibles",
    "MANDIBLES",
    "1989",
    "STRATEGY",
    "ONE SLOW BLUE ANT AGAINST A RED HORDE. SHOUT ORDERS, RECLAIM THE OLD COLONY.",
    {"BRING DOWN A LONGLEGS", "RECLAIM THE OLD COLONY", "TAKE EVERY FIELD"},
    "D-PAD\tWALK\n"
    MND_GLYPH_SPIT "\tSPIT (FOLLOWERS SPIT TOO)\n"
    "HOLD " MND_GLYPH_ORDER "\tCOMMANDS, LET GO TO SHOUT:\n"
    GLYPH_UP "\tLAY WORKERS/SOLDIERS, WITHDRAW\n"
    GLYPH_RIGHT "\tFALL IN / SQUAD FALL IN\n"
    GLYPH_DOWN "\tFREE WILL / SQUAD FREE WILL\n"
    GLYPH_LEFT "\tHALT / SQUAD HALT\n"
    "\tPRESS AGAIN FOR THE NEXT ONE\n"
    "TAP " MND_GLYPH_ORDER "\tTHE LAST COMMAND AGAIN\n"
    "WALK OVER SAP, TOUCH YOUR QUEEN.\n"
    "START\tPAUSE",
    C_BLUE, C_RED,
    mnd_load, mnd_start, mnd_update, mnd_draw, mnd_quit, mnd_label, mnd_query, mnd_cheat,
    "COMBATANTS", 46,
};
