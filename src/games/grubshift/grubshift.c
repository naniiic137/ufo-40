/* GRUB SHIFT - turn-based grub control in a comet greenhouse.
 * Cartridge 02 of UFO 40. See docs/games/02-grub-shift.md.
 * Rules live in grubshift_logic.c; this file is presentation and flow. */
#include "grubshift.h"

#define TW 24
#define TH 20
#define BX 8
#define BY 28
#define RAISE_PX 5

enum { S_TITLE, S_HELP, S_BRIEF, S_PLAY, S_NIGHT, S_RESULT, S_CERT };
enum { F_HAND, F_SHOP, F_TARGET, F_BUYSLOT, F_REST };

typedef struct Save {
    uint32_t magic;
    uint8_t streak, best_streak, in_progress, certified;
    uint16_t contracts_done, total_kills;
    Board board;
} Save;
#define SAVE_MAGIC 0x47530002u

static Save sv;
static Board B, before_night;
static int state, state_t, focus, hand_sel, shop_sel, title_sel, help_page;
static int cur_x, cur_y, buy_offer, rest_armed;
static int shake, frame_t;
static float tilly_dx, tilly_dy; /* drawn position (tile units) */
static Events evq;
static int ev_base_t;
static bool sheet_mode;
static int result_kind;
static Rng seeds;

/* particles */
typedef struct { float x, y, vx, vy; int life, col, kind; } Part;
static Part parts[160];

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) {
            parts[i] = (Part){x, y, vx, vy, life, col, kind};
            return;
        }
}

static int tile_px(int x) { return BX + x * TW; }
static int tile_py(const Board *b, int x, int y) { return BY + y * TH - (b->elev[y][x] ? RAISE_PX : 0); }

/* ------------------------------------------------------------------ */
/* save                                                                 */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    sv.board = B;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else { memset(&sv, 0, sizeof sv); sv.magic = SAVE_MAGIC; }
}

/* ------------------------------------------------------------------ */
/* flow                                                                 */

static void begin_contract(void) {
    uint64_t seed = rng_next(&seeds) ^ ((uint64_t)rng_next(&seeds) << 32) ^ (uint64_t)(sv.contracts_done * 7919u);
    gs_new_contract(&B, sv.streak + 1, seed);
    sv.in_progress = 1;
    state = S_BRIEF;
    state_t = 0;
    tilly_dx = B.px;
    tilly_dy = B.py;
    save_now();
    music_play(GS_MUS_TITLE);
}

static void enter_play(void) {
    state = S_PLAY;
    state_t = 0;
    focus = F_HAND;
    hand_sel = 0;
    rest_armed = 0;
    tilly_dx = B.px;
    tilly_dy = B.py;
    game_set_pausable(true);
    music_play(GS_MUS_SHIFT);
}

static void finish_contract(void) {
    state = S_RESULT;
    state_t = 0;
    result_kind = B.status;
    sv.in_progress = 0;
    sv.total_kills = (uint16_t)imin(60000, sv.total_kills + B.kills);
    if (B.status == ST_WON) {
        sv.streak++;
        sv.contracts_done++;
        if (sv.streak > sv.best_streak) sv.best_streak = sv.streak;
        if (sv.streak >= 3) game_award(GOAL_SAUCER);
        if (sv.streak >= 5) game_award(GOAL_ALIEN);
        music_restart(GS_MUS_WIN);
    } else {
        sv.streak = 0;
        music_restart(GS_MUS_LOSE);
    }
    game_set_pausable(false);
    save_now();
}

/* Replay rule events as particles and sounds, staggered for chains. */
static void play_events(const Events *ev) {
    ev_base_t = frame_t;
    for (int i = 0; i < ev->n; i++) evq.ev[i] = ev->ev[i];
    evq.n = ev->n;
}

static void spawn_event_fx(const Event *e) {
    float cx = tile_px(e->x) + TW / 2.0f, cy = BY + e->y * TH + TH / 2.0f - 2;
    switch (e->type) {
    case EV_MOVE: sfx_play_name(e->a ? "gs_hop" : "gs_move"); break;
    case EV_BOOM:
        sfx_play_name("gs_boom");
        shake = imax(shake, 10);
        for (int i = 0; i < 18; i++) {
            float a = (float)i / 18.0f * 6.283f;
            part_add(cx, cy, cosf(a) * 1.8f, sinf(a) * 1.4f - 0.6f, 18 + i % 6, i % 3 ? C_YELLOW : C_LIME, 1);
        }
        part_add(cx, cy, 0, 0, 10, C_WHITE, 2);
        break;
    case EV_KILL: case EV_STOMP:
        sfx_play_name("gs_kill");
        for (int i = 0; i < 8; i++) part_add(cx, cy, (float)(i % 4 - 1.5f) * 0.7f, -1.2f - (i / 4), 16, C_LIGHT, 0);
        part_add(cx, cy - 6, 0, -0.5f, 30, C_YELLOW, 3); /* "+1" */
        if (e->type == EV_STOMP) shake = imax(shake, 5);
        break;
    case EV_HIT: sfx_play_name("gs_hit"); part_add(cx, cy, 0, -0.3f, 10, C_WHITE, 2); break;
    case EV_SPARK: {
        sfx_play_name("gs_spark");
        float tx = tile_px(e->x2) + TW / 2.0f, ty = BY + e->y2 * TH + TH / 2.0f;
        for (int i = 0; i < 5; i++) part_add(cx + (tx - cx) * i / 5, cy + (ty - cy) * i / 5, 0, 0, 12 + i * 2, C_YELLOW, 0);
        break;
    }
    case EV_HOLE: sfx_play_name("gs_hole"); for (int i = 0; i < 10; i++) part_add(cx, cy, (i - 5) * 0.4f, -1.5f, 20, C_TAN, 0); break;
    case EV_POD:
        if (e->a == 0) { sfx_play_name("gs_pod"); for (int i = 0; i < 6; i++) part_add(cx, cy, (i - 3) * 0.3f, -1.2f, 18, C_LIME, 0); }
        break;
    case EV_EGG:
        if (e->a == 255) sfx_play_name("gs_hatch");
        else { sfx_play_name("gs_egg"); for (int i = 0; i < 8; i++) part_add(cx, cy, (i - 4) * 0.5f, -1.0f, 16, C_CREAM, 0); }
        break;
    case EV_DIE: sfx_play_name("gs_die"); shake = 16; break;
    case EV_RAISE: sfx_play_name("gs_raise"); break;
    case EV_SHOT: sfx_play_name(e->a == 0 ? "gs_zap" : "gs_toss"); break;
    case EV_BUY: sfx_play_name("gs_buy"); break;
    case EV_PUSH: sfx_play_name("gs_move"); break;
    }
}

static void tick_events(void) {
    for (int i = 0; i < evq.n; i++) {
        if (frame_t - ev_base_t == i * 3) spawn_event_fx(&evq.ev[i]);
    }
    if (evq.n > 0 && frame_t - ev_base_t > evq.n * 3 + 2) evq.n = 0;
}

static void do_action(int slot, int x, int y) {
    Events ev;
    ev.n = 0;
    int combo0 = B.best_combo;
    if (!gs_apply(&B, slot, x, y, &ev)) { sfx_play_name("gs_nope"); return; }
    play_events(&ev);
    if (B.best_combo >= 4 && combo0 < 4) game_award(GOAL_BEACON);
    focus = F_HAND;
    rest_armed = 0;
    if (B.status != ST_PLAYING) state_t = -60; /* short pause, then result */
    save_now();
}

static void do_rest(void) {
    before_night = B;
    Events ev;
    ev.n = 0;
    gs_rest(&B, &ev);
    play_events(&ev);
    state = S_NIGHT;
    state_t = 0;
    focus = F_HAND;
    rest_armed = 0;
    if (B.status == ST_PLAYING) music_restart(GS_MUS_NIGHT);
    save_now();
}

/* ------------------------------------------------------------------ */
/* input                                                                */

static void first_target(int chip) {
    uint8_t v[GH][GW];
    gs_targets(&B, chip, v);
    int best = 999;
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (v[y][x]) {
                int d = iabs(x - B.px) + iabs(y - B.py);
                if (d < best) { best = d; cur_x = x; cur_y = y; }
            }
    if (best == 999) { cur_x = B.px; cur_y = B.py; }
}

static void play_input(void) {
    int dx = btn_repeat(BTN_RIGHT) - btn_repeat(BTN_LEFT);
    int dy = btn_repeat(BTN_DOWN) - btn_repeat(BTN_UP);
    switch (focus) {
    case F_HAND:
        if (dx) { hand_sel = (hand_sel + dx + 8) % 8; rest_armed = 0; sfx_play_name("ui_move"); }
        if (dy < 0) { focus = F_SHOP; shop_sel = hand_sel >= 4 ? 7 : 6; sfx_play_name("ui_move"); }
        if (btnp(BTN_A)) {
            if (hand_sel == 7) {
                bool unused = false;
                for (int i = 0; i < SLOTS; i++) unused |= !B.spent[i];
                if (unused && !rest_armed) { rest_armed = 1; sfx_play_name("ui_move"); }
                else do_rest();
            } else if (!B.spent[hand_sel]) {
                uint8_t v[GH][GW];
                gs_targets(&B, B.chips[hand_sel], v);
                bool any = false;
                for (int y = 0; y < GH; y++)
                    for (int x = 0; x < GW; x++) any |= v[y][x];
                if (!any) { sfx_play_name("gs_nope"); break; }
                focus = F_TARGET;
                first_target(B.chips[hand_sel]);
                sfx_play_name("ui_ok");
            } else {
                sfx_play_name("gs_nope");
            }
        }
        if (btnp(BTN_B)) rest_armed = 0;
        break;
    case F_SHOP:
        if (dx) { shop_sel ^= 1; sfx_play_name("ui_move"); }
        if (dy < 0 && shop_sel >= 2) { shop_sel -= 2; sfx_play_name("ui_move"); }
        else if (dy > 0) {
            if (shop_sel < 6) shop_sel += 2;
            else { focus = F_HAND; hand_sel = shop_sel == 7 ? 7 : 5; }
            sfx_play_name("ui_move");
        }
        if (btnp(BTN_B)) { focus = F_HAND; sfx_play_name("ui_back"); }
        if (btnp(BTN_A)) {
            int c = B.shop[shop_sel];
            if (c == TOOL_NONE || B.energy < CHIPS[c].cost) sfx_play_name("gs_nope");
            else { focus = F_BUYSLOT; buy_offer = shop_sel; hand_sel = 0; sfx_play_name("ui_ok"); }
        }
        break;
    case F_BUYSLOT:
        if (dx) { hand_sel = (hand_sel + dx + SLOTS) % SLOTS; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { focus = F_SHOP; sfx_play_name("ui_back"); }
        if (btnp(BTN_A)) {
            Events ev;
            ev.n = 0;
            if (gs_buy(&B, buy_offer, hand_sel, &ev)) { play_events(&ev); save_now(); }
            focus = F_HAND;
        }
        break;
    case F_TARGET:
        if (dx || dy) {
            cur_x = iclamp(cur_x + dx, 0, GW - 1);
            cur_y = iclamp(cur_y + dy, 0, GH - 1);
            sfx_play_name("ui_move");
        }
        if (btnp(BTN_B)) { focus = F_HAND; sfx_play_name("ui_back"); }
        if (btnp(BTN_A)) {
            uint8_t v[GH][GW];
            gs_targets(&B, B.chips[hand_sel], v);
            if (v[cur_y][cur_x]) do_action(hand_sel, cur_x, cur_y);
            else sfx_play_name("gs_nope");
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* update                                                               */

static void gs_update(void) {
    frame_t++;
    state_t++;
    if (shake > 0) shake--;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        if (p->kind == 0 || p->kind == 1) p->vy += 0.08f;
    }
    tick_events();
    tilly_dx += (B.px - tilly_dx) * 0.3f;
    tilly_dy += (B.py - tilly_dy) * 0.3f;
    switch (state) {
    case S_TITLE: {
        game_set_pausable(false);
        int n = sv.in_progress ? 3 : 2;
        if (btnp(BTN_UP)) { title_sel = (title_sel + n - 1) % n; sfx_play_name("ui_move"); }
        if (btnp(BTN_DOWN)) { title_sel = (title_sel + 1) % n; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) game_exit_to_library();
        if (btnp(BTN_A) || btnp(BTN_START)) {
            sfx_play_name("ui_ok");
            int opt = sv.in_progress ? title_sel : title_sel + 1;
            if (opt == 0) { B = sv.board; enter_play(); }
            else if (opt == 1) { sv.streak = 0; begin_contract(); }
            else { state = S_HELP; help_page = 0; state_t = 0; }
        }
        break;
    }
    case S_HELP:
        if (btnp(BTN_A) || btnp(BTN_RIGHT)) { help_page++; sfx_play_name("ui_move"); }
        if (btnp(BTN_LEFT) && help_page > 0) { help_page--; sfx_play_name("ui_move"); }
        if (btnp(BTN_B) || help_page > 3) { state = S_TITLE; state_t = 0; sfx_play_name("ui_back"); }
        break;
    case S_BRIEF:
        game_set_pausable(false);
        if (state_t > 30 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); input_consume(); enter_play(); }
        break;
    case S_PLAY:
        if (B.status != ST_PLAYING) {
            if (state_t > 0) finish_contract();
            break;
        }
        if (state_t < 0) break;
        play_input();
        break;
    case S_NIGHT:
        if (state_t == 1 && B.status != ST_PLAYING) state_t = 60;
        if (state_t > 100 || (state_t > 30 && btnp(BTN_A))) {
            if (B.status != ST_PLAYING) finish_contract();
            else { state = S_PLAY; state_t = 0; focus = F_HAND; hand_sel = 0; music_play(GS_MUS_SHIFT); }
        }
        break;
    case S_RESULT:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) {
            sfx_play_name("ui_ok");
            if (result_kind == ST_WON && sv.streak == 3 && !sv.certified) {
                sv.certified = 1;
                save_now();
                state = S_CERT;
                state_t = 0;
                music_restart(GS_MUS_WIN);
            } else if (result_kind == ST_WON) {
                begin_contract();
            } else {
                state = S_TITLE;
                state_t = 0;
                title_sel = 0;
                music_play(GS_MUS_TITLE);
            }
        }
        if (btnp(BTN_B) && state_t > 60) { state = S_TITLE; state_t = 0; title_sel = 0; music_play(GS_MUS_TITLE); }
        break;
    case S_CERT:
        if (state_t > 120 && (btnp(BTN_A) || btnp(BTN_START))) begin_contract();
        if (state_t > 120 && btnp(BTN_B)) { state = S_TITLE; state_t = 0; music_play(GS_MUS_TITLE); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */

static const uint8_t SPECIES_COL[SP_COUNT] = {C_GREY, C_YELLOW, C_SKY, C_EARTH, C_LEAF};
static const char *SPECIES_NAME[SP_COUNT] = {"", "SPARKMITE", "SHELLBUG", "BURROWER", "MOUNDMAKER"};

static void draw_backdrop(void) {
    gfx_cls(C_INK);
    /* the dome: glass ribs and stars */
    for (int i = 0; i < 40; i++) {
        uint32_t h = (uint32_t)(i * 2654435761u);
        int x = (int)(h % 320), y = (int)((h >> 12) % 180);
        gfx_pset(x, y, ((frame_t / 20 + i) % 7) ? C_DUSK : C_LIGHT);
    }
    for (int r = 0; r < 4; r++) gfx_circb(104, 220, 150 + r * 42, C_NIGHT);
    for (int k = -3; k <= 3; k++) gfx_line(104, 220, 104 + k * 70, 0, C_NIGHT);
}

static void draw_tile(const Board *b, int x, int y) {
    int px = tile_px(x), py = BY + y * TH;
    if (b->hole[y][x]) {
        gfx_rect(px, py, TW, TH, C_INK);
        gfx_rect(px + 2, py + 2, TW - 4, 3, C_NIGHT);
        gfx_hline(px, px + TW - 1, py, C_BROWN);
        gfx_dither(px + 3, py + 5, TW - 6, TH - 7, C_NIGHT, 4);
        return;
    }
    int checker = (x + y) & 1;
    if (b->elev[y][x]) {
        int top = py - RAISE_PX;
        /* planter box: soil top, wooden front */
        gfx_rect(px, top, TW, TH, checker ? C_FOREST : C_TEAL);
        gfx_dither(px, top, TW, TH, C_JADE, 3);
        gfx_hline(px, px + TW - 1, top, C_LEAF);
        gfx_rect(px, py + TH - RAISE_PX, TW, RAISE_PX, C_TAN);
        gfx_hline(px, px + TW - 1, py + TH - RAISE_PX, C_EARTH);
        gfx_hline(px, px + TW - 1, py + TH - 1, C_BROWN);
        for (int k = 4; k < TW; k += 8) gfx_vline(px + k, py + TH - RAISE_PX + 1, py + TH - 2, C_BROWN);
        gfx_vline(px, top, py + TH - 1, C_INK);
        /* sprout */
        gfx_pset(px + 5, top + 3, C_LIME); gfx_pset(px + 6, top + 2, C_LEAF);
    } else {
        gfx_rect(px, py, TW, TH, checker ? C_BROWN : C_MAROON);
        gfx_dither(px, py, TW, TH, C_TAN, 2);
        gfx_hline(px, px + TW - 1, py, checker ? C_TAN : C_BROWN);
        gfx_pset(px + 4 + (x * 7 + y * 3) % 14, py + 6 + (x + y * 5) % 9, C_EARTH);
    }
}

static int bug_sprite(int sp, int lv) {
    if (lv == LV_EGG) return GS_EGG;
    if (lv == LV_LARVA) return GS_LARVA_SPARK + (sp - 1);
    return GS_ADULT_SPARK + (sp - 1);
}

static void draw_bug(const Board *b, int x, int y, int sx, int sy) {
    int sp = b->bsp[y][x], lv = b->blv[y][x];
    int bob = ((frame_t / 12 + x * 3 + y) % 2);
    if (lv == LV_EGG) {
        int pulse = (frame_t / 8) % 2;
        gfx_dither_circle(sx + 12, sy + 11, 9 + pulse, C_WINE, 6);
        spr_draw(&gs_spr[(frame_t / 10) % 4 == 0 ? GS_EGG2 : GS_EGG], sx + 6, sy + 5 - pulse, 0);
        return;
    }
    Sprite *s = &gs_spr[bug_sprite(sp, lv)];
    int ox = sx + (TW - s->w) / 2, oy = sy + TH - s->h - 3 - bob;
    int fl = ((frame_t / 60 + x) % 2) ? SPR_FLIPX : 0;
    if (lv == LV_QUEEN) gfx_dither_circle(sx + 12, sy + 11, 11, SPECIES_COL[sp], 5);
    spr_draw(s, ox, oy, fl);
    if (lv == LV_QUEEN) spr_draw(&gs_spr[GS_CROWN], sx + 8, oy - 5, 0);
    if (b->bhp[y][x] > 1) { gfx_rect(sx + 17, sy + 2, 5, 5, C_INK); gfx_rect(sx + 18, sy + 3, 3, 3, C_SKY); }
}

static void draw_board(const Board *b, bool with_targets, int chip) {
    uint8_t v[GH][GW];
    if (with_targets) gs_targets(b, chip, v);
    else memset(v, 0, sizeof v);
    /* frame */
    ui_panel(BX - 3, BY - 8, GW * TW + 6, GH * TH + 11, C_NIGHT, C_DUSK);
    for (int y = 0; y < GH; y++) {
        for (int x = 0; x < GW; x++) draw_tile(b, x, y);
        for (int x = 0; x < GW; x++) {
            int sx = tile_px(x), sy = tile_py(b, x, y);
            if (v[y][x]) {
                gfx_dither(sx + 1, sy + 1, TW - 2, TH - 2, focus == F_TARGET ? C_YELLOW : C_AMBER, (frame_t / 8) % 2 ? 5 : 4);
                gfx_rectb(sx + 1, sy + 1, TW - 2, TH - 2, C_YELLOW);
            }
            /* pods */
            if (b->pods[y][x]) {
                int n = b->pods[y][x];
                int glow = (frame_t / 10 + x + y) % 2;
                for (int k = 0; k < n; k++) {
                    int ox = sx + (n == 1 ? 8 : 4 + k * 9), oy = sy + 6 - (k & 1);
                    spr_draw(&gs_spr[glow ? GS_POD2 : GS_POD1], ox, oy, 0);
                }
            }
            if (b->bsp[y][x]) draw_bug(b, x, y, sx, sy);
            /* Tilly (drawn at her animated position on her row) */
            if ((int)(tilly_dy + 0.5f) == y && x == GW - 1) {
                float fx = tilly_dx, fy = tilly_dy;
                int tx = (int)(fx + 0.5f), ty = (int)(fy + 0.5f);
                int tpx = BX + (int)(fx * TW), tpy = BY + (int)(fy * TH) - (b->elev[ty][tx] ? RAISE_PX : 0);
                bool moving = fabsf(fx - b->px) > 0.05f || fabsf(fy - b->py) > 0.05f;
                int spr = b->status == ST_DEAD ? GS_TILLY_DEAD : moving ? GS_TILLY_HOP : ((frame_t / 20) % 2 ? GS_TILLY1 : GS_TILLY2);
                spr_draw(&gs_spr[spr], tpx + 4, tpy + 1, 0);
            }
        }
    }
    /* attack preview: the tiles a line shot or pulse will hit */
    if (focus == F_TARGET && with_targets && v[cur_y][cur_x]) {
        int k = CHIPS[chip].kind;
        if (chip == CH_ZAP || chip == CH_ARC || chip == CH_BEAM) {
            int dx = isign(cur_x - b->px), dy = isign(cur_y - b->py);
            for (int x = b->px + dx, y = b->py + dy;; x += dx, y += dy) {
                gfx_rectb(tile_px(x) + 3, tile_py(b, x, y) + 3, TW - 6, TH - 6, C_RED);
                if (x == cur_x && y == cur_y) break;
            }
        } else if (chip == CH_PULSE) {
            for (int d = 0; d < 9; d++) {
                int x = b->px + d % 3 - 1, y = b->py + d / 3 - 1;
                if ((x != b->px || y != b->py) && x >= 0 && y >= 0 && x < GW && y < GH)
                    gfx_rectb(tile_px(x) + 3, tile_py(b, x, y) + 3, TW - 6, TH - 6, C_RED);
            }
        } else if (k == KIND_ATTACK) {
            gfx_rectb(tile_px(cur_x) + 3, tile_py(b, cur_x, cur_y) + 3, TW - 6, TH - 6, C_RED);
        }
    }
    /* cursor */
    if (focus == F_TARGET && with_targets) {
        int sx = tile_px(cur_x), sy = tile_py(b, cur_x, cur_y);
        int k = (frame_t / 6) % 2;
        int col = v[cur_y][cur_x] ? C_WHITE : C_RED;
        gfx_rectb(sx - k, sy - k, TW + k * 2, TH + k * 2, col);
        gfx_rectb(sx - 1 - k, sy - 1 - k, TW + 2 + k * 2, TH + 2 + k * 2, C_INK);
    }
}

static void draw_pattern(int chip, int x, int y, int col) {
    /* a tiny 5x5 diagram of the tool's reach (2x2 dots) */
    Board e;
    memset(&e, 0, sizeof e);
    e.px = 3;
    e.py = 2;
    uint8_t v[GH][GW];
    if (chip == CH_WARP || chip == CH_DETONATE) { e.pods[0][1] = 1; e.pods[4][5] = 1; e.pods[1][6] = 1; }
    if (chip == CH_DEVOLVE) { e.bsp[1][2] = 1; e.bsp[3][4] = 1; e.bsp[2][5] = 1; }
    gs_targets(&e, chip, v);
    if (chip == CH_PULSE) /* show the blast ring rather than the confirm tile */
        for (int yy = 1; yy <= 3; yy++)
            for (int xx = 2; xx <= 4; xx++) v[yy][xx] = !(xx == 3 && yy == 2);
    for (int dy = -2; dy <= 2; dy++)
        for (int dx = -2; dx <= 2; dx++) {
            int gx = 3 + dx, gy = 2 + dy;
            int c = C_NIGHT;
            if (dx == 0 && dy == 0) c = C_WHITE;
            else if (gx >= 0 && gy >= 0 && gx < GW && gy < GH && v[gy][gx]) c = col;
            gfx_rect(x + (dx + 2) * 3, y + (dy + 2) * 3, 2, 2, c);
        }
}

static int kind_col(int chip) {
    int k = CHIPS[chip].kind;
    return k == KIND_MOVE ? C_JADE : k == KIND_ATTACK ? C_RED : C_SKY;
}

static void draw_card(int x, int y, int w, int h, int chip, bool sel, bool spent, bool show_cost) {
    if (chip == TOOL_NONE) {
        ui_panel(x, y, w, h, C_INK, C_NIGHT);
        tiny_center("SOLD", x + w / 2, y + h / 2 - 2, C_DUSK);
        if (sel) gfx_rectb(x - 1, y - 1, w + 2, h + 2, C_YELLOW);
        return;
    }
    int col = kind_col(chip);
    ui_panel(x, y, w, h, C_NIGHT, sel ? C_WHITE : col);
    draw_pattern(chip, x + 2, y + 3, col);
    tiny_draw(CHIPS[chip].name, x + 18, y + 3, spent ? C_SLATE : C_WHITE);
    if (show_cost) {
        char buf[8];
        snprintf(buf, sizeof buf, "%d", CHIPS[chip].cost);
        for (int i = 0; i < CHIPS[chip].cost; i++) gfx_rect(x + 18 + i * 4, y + h - 7, 3, 3, B.energy >= CHIPS[chip].cost ? C_LIME : C_FOREST);
    } else {
        spr_draw(&gs_spr[CHIPS[chip].kind == KIND_MOVE ? GS_ICON_MOVE : CHIPS[chip].kind == KIND_ATTACK ? GS_ICON_ATTACK : GS_ICON_SPECIAL],
                 x + w - 10, y + h - 9, 0);
    }
    if (spent) {
        gfx_darken_rect(x + 1, y + 1, w - 2, h - 2, 2);
        text_draw(GLYPH_CHECK, x + w / 2 - 3, y + h / 2 - 3, C_GREY);
    }
    if (sel) {
        int k = (frame_t / 8) % 2;
        gfx_rectb(x - 1 - k, y - 1 - k, w + 2 + 2 * k, h + 2 + 2 * k, C_YELLOW);
    }
}

static void draw_hud_top(void) {
    gfx_rect(0, 0, 320, 17, C_NIGHT);
    gfx_hline(0, 319, 17, C_DUSK);
    char buf[64];
    snprintf(buf, sizeof buf, "CONTRACT %d", B.contract);
    text_draw(buf, 5, 5, C_LIGHT);
    snprintf(buf, sizeof buf, "SHIFT %d/%d", B.day, B.days);
    text_draw(buf, 84, 5, B.day == B.days ? C_ORANGE : C_LIGHT);
    spr_draw(&gs_spr[GS_LARVA_MOUND], 150, 5, 0);
    snprintf(buf, sizeof buf, "%d/%d", B.kills, B.quota);
    text_draw(buf, 164, 5, B.kills >= B.quota ? C_LIME : C_YELLOW);
    spr_draw(&gs_spr[GS_POD1], 210, 3, 0);
    snprintf(buf, sizeof buf, "%d", B.energy);
    text_draw(buf, 221, 5, C_LIME);
    snprintf(buf, sizeof buf, "STREAK %d", sv.streak);
    tiny_draw(buf, 270, 7, C_SLATE);
    /* quota bar */
    int w = 318 * imin(B.kills, B.quota) / imax(1, B.quota);
    gfx_hline(1, 1 + w, 16, C_LIME);
}

static void draw_play(void) {
    int sx = 0, sy = 0;
    if (shake > 0) { sx = (frame_t % 3) - 1; sy = ((frame_t / 2) % 3) - 1; }
    draw_backdrop();
    gfx_camera(sx, sy);
    int chip = (focus == F_TARGET || (focus == F_HAND && hand_sel < SLOTS && !B.spent[hand_sel])) ? B.chips[hand_sel] : -1;
    draw_board(&B, chip >= 0 && state == S_PLAY && B.status == ST_PLAYING, chip);
    /* particles */
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        if (p->kind == 2) gfx_circb((int)p->x, (int)p->y, 12 - p->life, p->col);
        else if (p->kind == 3) tiny_draw("+1", (int)p->x - 3, (int)p->y, p->col);
        else gfx_rect((int)p->x, (int)p->y, 2, 2, p->col);
    }
    gfx_camera(0, 0);
    draw_hud_top();
    /* shop */
    int shx = 206, shy = 22;
    text_draw("SUPPLY DRONE", shx, shy, C_SKY);
    for (int i = 0; i < OFFERS; i++) {
        int cx = shx + (i % 2) * 56, cy = shy + 11 + (i / 2) * 24;
        draw_card(cx, cy, 53, 22, B.shop[i], focus == F_SHOP && shop_sel == i, false, true);
        if (focus == F_BUYSLOT && buy_offer == i) gfx_rectb(cx - 2, cy - 2, 57, 26, C_LIME);
    }
    /* info box */
    int info_chip = -1;
    if (focus == F_SHOP || focus == F_BUYSLOT) info_chip = B.shop[focus == F_SHOP ? shop_sel : buy_offer];
    else if (hand_sel < SLOTS) info_chip = B.chips[hand_sel];
    ui_panel(204, 129, 114, 21, C_INK, C_DUSK);
    if (focus == F_BUYSLOT) tiny_draw("PICK A SLOT TO REPLACE", 207, 132, C_LIME), tiny_draw(GLYPH_A " OK   B BACK", 207, 140, C_GREY);
    else if (hand_sel == SLOTS && focus == F_HAND) {
        tiny_draw(rest_armed ? "TOOLS LEFT! A AGAIN TO REST" : "END THE SHIFT. GRUBS GROW,", 207, 132, rest_armed ? C_ORANGE : C_GREY);
        tiny_draw(rest_armed ? "" : "PODS FALL, TOOLS RECHARGE.", 207, 140, C_GREY);
    } else if (info_chip >= 0 && info_chip != TOOL_NONE) {
        char line[128];
        snprintf(line, sizeof line, "%s", CHIPS[info_chip].desc);
        /* tiny word wrap */
        int lx = 207, ly = 131, col = 0;
        char word[32];
        const char *p = line;
        while (*p) {
            int wl = 0;
            while (*p && *p != ' ' && wl < 30) word[wl++] = *p++;
            word[wl] = 0;
            if (col + wl > 27) { ly += 6; col = 0; }
            if (ly > 144) break;
            tiny_draw(word, lx + col * 4, ly, C_LIGHT);
            col += wl + 1;
            while (*p == ' ') p++;
        }
    }
    /* hand */
    gfx_rect(0, 151, 320, 29, C_INK);
    gfx_hline(0, 319, 151, C_DUSK);
    for (int i = 0; i < SLOTS; i++) {
        bool sel = (focus == F_HAND || focus == F_BUYSLOT || focus == F_TARGET) && hand_sel == i;
        draw_card(4 + i * 36, 154, 34, 24, B.chips[i], sel, B.spent[i] != 0, false);
    }
    bool rsel = focus == F_HAND && hand_sel == SLOTS;
    ui_panel(258, 154, 58, 24, rest_armed ? C_WINE : C_NIGHT, rsel ? C_WHITE : C_VIOLET);
    text_center("REST", 287, 158, rsel ? C_WHITE : C_LIGHT);
    tiny_center("END SHIFT", 287, 169, C_GREY);
    if (rsel) gfx_rectb(257 - (frame_t / 8) % 2, 153 - (frame_t / 8) % 2, 60 + 2 * ((frame_t / 8) % 2), 26 + 2 * ((frame_t / 8) % 2), C_YELLOW);
    if (focus == F_TARGET) {
        gfx_rect(BX - 2, 20, 196, 7, C_NIGHT);
        tiny_draw("CHOOSE A TARGET - A: CONFIRM  B: CANCEL", BX + 2, 21, C_YELLOW);
    }
}

static void draw_night(void) {
    draw_play();
    int t = state_t;
    int lvl = t < 20 ? t / 2 : t > 80 ? (100 - t) / 2 : 10;
    gfx_dither(0, 18, 320, 133, C_INK, iclamp(lvl, 0, 12));
    if (t > 10 && t < 95) {
        char buf[48];
        snprintf(buf, sizeof buf, "SHIFT %d", B.status == ST_PLAYING ? B.day : B.day);
        static const uint8_t grad[] = {C_ICE, C_CYAN, C_SKY};
        ui_fancy_center(B.status == ST_PLAYING ? buf : "LAST SHIFT OVER", 104, 64, 2, grad, 3, C_INK, C_NAVY);
        if (B.status == ST_PLAYING) text_center("THE GRUBS STIR IN THE DARK...", 104, 90, C_LIGHT);
    }
}

static void lettuce(int x, int y) {
    gfx_circ(x, y, 6, C_FOREST);
    gfx_circ(x - 2, y - 1, 4, C_JADE);
    gfx_circ(x + 2, y - 2, 3, C_LEAF);
    gfx_pset(x - 1, y - 4, C_LIME);
}

static void draw_title(void) {
    draw_backdrop();
    /* a comet streaks past the dome */
    int cx = 262 - (frame_t / 3) % 40, cy = 26;
    for (int k = 0; k < 40; k++) gfx_dither(cx + k, cy - k / 3, 2, 2, C_SKY, 12 - k / 4);
    gfx_circ(cx, cy, 3, C_ICE);
    gfx_pset(cx - 1, cy - 1, C_WHITE);
    /* soil and planter boxes */
    for (int i = 0; i < 14; i++) {
        int x = i * 24;
        gfx_rect(x, 140, 24, 40, (i & 1) ? C_BROWN : C_MAROON);
        gfx_dither(x, 140, 24, 40, C_TAN, 2);
        gfx_hline(x, x + 23, 140, C_TAN);
    }
    for (int i = 0; i < 2; i++) {
        int x = 18 + i * 244;
        gfx_rect(x, 128, 40, 12, C_TAN);
        gfx_hline(x, x + 39, 128, C_EARTH);
        for (int k = 6; k < 40; k += 10) gfx_vline(x + k, 129, 139, C_BROWN);
        lettuce(x + 10, 124);
        lettuce(x + 29, 123);
    }
    for (int i = 0; i < 4; i++) {
        int x = 72 + i * 56 + (int)(sinf(frame_t * 0.03f + i) * 6);
        spr_draw(&gs_spr[GS_LARVA_SPARK + i], x, 142 + ((frame_t / 10 + i) % 2), (frame_t / 40 + i) % 2 ? SPR_FLIPX : 0);
    }
    spr_draw_scaled(&gs_spr[(frame_t / 20) % 2 ? GS_TILLY1 : GS_TILLY2], 142, 108, 2, 0);
    spr_draw(&gs_spr[GS_POD2], 190, 129, 0);
    spr_draw(&gs_spr[GS_POD1], 200, 129, 0);
    static const uint8_t grad[] = {C_LIME, C_LEAF, C_JADE, C_FOREST};
    ui_fancy_center("GRUB SHIFT", 160, 18, 3, grad, 4, C_INK, C_TEAL);
    text_center("PEST CONTROL ON THE NIGHT SHIFT", 160, 46, C_LEAF);
    const char *items[3];
    int n = 0;
    if (sv.in_progress) items[n++] = "CONTINUE SHIFT";
    items[n++] = "NEW CONTRACT";
    items[n++] = "HOW TO PLAY";
    for (int i = 0; i < n; i++) {
        int y = 62 + i * 11;
        bool s = i == title_sel;
        text_center(items[i], 160, y, s ? C_WHITE : C_GREY);
        if (s) ui_cursor(160 - text_width(items[i]) / 2 - 10, y, frame_t);
    }
    char buf[64];
    snprintf(buf, sizeof buf, "BEST STREAK %d   CONTRACTS DONE %d", sv.best_streak, sv.contracts_done);
    gfx_rect(80, 166, 160, 11, C_INK);
    tiny_center(buf, 160, 169, C_GREY);
}

static const char *HELP_TITLE[4] = {"THE JOB", "FIZZ PODS", "GRUB LIFE", "THE PESTS"};
static const char *HELP_TEXT[4] = {
    "SQUASH ENOUGH GRUBS TO MEET THE QUOTA BEFORE THE LAST SHIFT ENDS. EACH TOOL WORKS ONCE PER SHIFT. "
    "WHEN YOU ARE DONE, PICK REST TO END THE SHIFT.",
    "WALK OVER FIZZ PODS TO COLLECT THEM, THEN SPEND THEM ON NEW TOOLS. A THIRD POD ON ONE TILE BLOWS IT OPEN. "
    "SHOOTING PODS SETS THEM OFF TOO - AND TILLY IS NOT BLAST-PROOF.",
    "EVERY NIGHT ONE KIND OF GRUB GROWS: LARVA, ADULT, THEN QUEEN. QUEENS LAY EGGS. "
    "AN EGG STILL THERE WHEN A SHIFT ENDS WILL HATCH - AND YOU ARE FIRED.",
    "SPARKMITES SPIT SPARKS WHEN THEY POP. SHELLBUGS TAKE TWO HITS, BUT A STOMP OR A PIT ENDS THEM. "
    "BURROWERS LEAVE HOLES. MOUNDMAKERS RAISE THE GROUND. PUSH GRUBS INTO PITS, OR ROLL DOWN ONTO THEM!",
};

static void draw_help(void) {
    draw_backdrop();
    int p = iclamp(help_page, 0, 3);
    ui_panel(24, 16, 272, 148, C_NIGHT, C_JADE);
    static const uint8_t grad[] = {C_LIME, C_LEAF, C_JADE};
    ui_fancy_center(HELP_TITLE[p], 160, 24, 2, grad, 3, C_INK, C_TEAL);
    text_wrap(HELP_TEXT[p], 36, 50, 248, C_LIGHT, 10);
    if (p == 3)
        for (int i = 1; i < SP_COUNT; i++) {
            spr_draw(&gs_spr[GS_ADULT_SPARK + i - 1], 44 + (i - 1) * 62, 128, 0);
            tiny_draw(SPECIES_NAME[i], 36 + (i - 1) * 62, 143, SPECIES_COL[i]);
        }
    if (p == 1) { spr_draw(&gs_spr[GS_POD1], 150, 128, 0); spr_draw(&gs_spr[GS_POD2], 160, 128, 0); }
    if (p == 2) {
        spr_draw(&gs_spr[GS_LARVA_SPARK], 90, 132, 0);
        spr_draw(&gs_spr[GS_ADULT_SPARK], 130, 128, 0);
        spr_draw(&gs_spr[GS_ADULT_SPARK], 180, 128, 0);
        spr_draw(&gs_spr[GS_CROWN], 184, 123, 0);
        spr_draw(&gs_spr[GS_EGG], 228, 130, 0);
    }
    char buf[32];
    snprintf(buf, sizeof buf, "%d/4   " GLYPH_A " NEXT   B BACK", p + 1);
    tiny_center(buf, 160, 156, C_GREY);
}

static void draw_brief(void) {
    draw_backdrop();
    ui_panel(40, 20, 240, 140, C_NIGHT, C_SKY);
    char buf[64];
    snprintf(buf, sizeof buf, "CONTRACT %d", B.contract);
    static const uint8_t grad[] = {C_ICE, C_CYAN, C_SKY};
    ui_fancy_center(buf, 160, 28, 2, grad, 3, C_INK, C_NAVY);
    snprintf(buf, sizeof buf, "QUOTA: %d GRUBS IN %d SHIFTS", B.quota, B.days);
    text_center(buf, 160, 52, C_YELLOW);
    text_center("PESTS SIGHTED IN THIS DOME:", 160, 68, C_LIGHT);
    for (int i = 0; i < 3; i++) {
        int sp = B.species[i];
        int x = 70 + i * 64;
        spr_draw(&gs_spr[GS_ADULT_SPARK + sp - 1], x, 82, 0);
        tiny_center(SPECIES_NAME[sp], x + 8, 98, SPECIES_COL[sp]);
        bool evo = sp == B.evolvers[0] || sp == B.evolvers[1];
        tiny_center(evo ? "GROWING" : "STUNTED", x + 8, 106, evo ? C_ORANGE : C_SLATE);
    }
    if (sv.streak > 0) {
        snprintf(buf, sizeof buf, "STREAK %d - KEEP IT GOING!", sv.streak);
        text_center(buf, 160, 122, C_LIME);
    }
    if (state_t > 30 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A " TO CLOCK IN", 160, 140, C_WHITE);
}

static void draw_result(void) {
    draw_play();
    gfx_dither(0, 18, 320, 162, C_INK, 10);
    ui_panel(50, 36, 220, 108, C_NIGHT, result_kind == ST_WON ? C_LIME : C_RED);
    static const uint8_t gw[] = {C_LIME, C_LEAF, C_JADE};
    static const uint8_t gl[] = {C_PINK, C_RED, C_WINE};
    const char *head = result_kind == ST_WON ? "CONTRACT DONE!" : "YOU'RE FIRED!";
    ui_fancy_center(head, 160, 44, 2, result_kind == ST_WON ? gw : gl, 3, C_INK, C_INK);
    const char *why = result_kind == ST_WON ? "THE GRUBS RETREAT FROM THE DOME."
                      : result_kind == ST_DEAD ? "TILLY GOT CAUGHT IN THE BLAST."
                      : result_kind == ST_HATCHED ? "AN EGG HATCHED OVERNIGHT."
                      : "THE QUOTA WAS NOT MET.";
    text_center(why, 160, 68, C_LIGHT);
    char buf[64];
    snprintf(buf, sizeof buf, "GRUBS %d/%d   BEST COMBO %d", B.kills, B.quota, B.best_combo);
    text_center(buf, 160, 84, C_GREY);
    snprintf(buf, sizeof buf, "STREAK %d   BEST %d", sv.streak, sv.best_streak);
    text_center(buf, 160, 96, C_YELLOW);
    if (state_t > 60 && (state_t / 20) % 2)
        text_center(result_kind == ST_WON ? GLYPH_A " NEXT CONTRACT   B TITLE" : GLYPH_A " TITLE", 160, 126, C_WHITE);
}

static void draw_cert(void) {
    gfx_cls(C_NIGHT);
    ui_panel(30, 14, 260, 152, C_CREAM, C_AMBER);
    gfx_rectb(34, 18, 252, 144, C_TAN);
    static const uint8_t grad[] = {C_AMBER, C_ORANGE, C_RED};
    ui_fancy_center("EMPLOYEE", 160, 26, 2, grad, 3, C_BROWN, -1);
    ui_fancy_center("OF THE MONTH", 160, 46, 2, grad, 3, C_BROWN, -1);
    spr_draw_scaled(&gs_spr[GS_TILLY1], 144, 70, 2, 0);
    text_center("TILLY", 160, 106, C_BROWN);
    text_center("THREE DOMES CLEARED IN A ROW.", 160, 118, C_TAN);
    text_center("ORCHARD STATION THANKS YOU!", 160, 128, C_TAN);
    for (int i = 0; i < 3; i++) ui_goal_icon(138 + i * 16, 138, 1 << i, (g_progress.goals[game_current_index()] >> i) & 1, frame_t);
    if (state_t > 120 && (state_t / 20) % 2) tiny_center("A: KEEP WORKING (OVERTIME)   B: TITLE", 160, 154, C_BROWN);
}

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    int x = 2, y = 2;
    for (int i = 0; i < GS_SPRITE_COUNT; i++) {
        Sprite *s = &gs_spr[i];
        if (!s->px) continue;
        if (x + s->w > 318) { x = 2; y += 20; }
        spr_draw_scaled(s, x, y, 1, 0);
        x += s->w + 3;
    }
    for (int c = 0; c < TOOL_COUNT; c++) draw_card(4 + (c % 5) * 60, 60 + (c / 5) * 28, 56, 24, c, false, c == 3, true);
}

static void gs_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_HELP: draw_help(); break;
    case S_BRIEF: draw_brief(); break;
    case S_PLAY: draw_play(); break;
    case S_NIGHT: draw_night(); break;
    case S_RESULT: draw_result(); break;
    case S_CERT: draw_cert(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void gs_load(void) {
    gs_art_load();
    gs_audio_load();
}

static void gs_start(void) {
    rng_seed(&seeds, g_rng.state ^ 0x6A0B5ull);
    load_save();
    state = S_TITLE;
    state_t = 0;
    title_sel = 0;
    sheet_mode = false;
    memset(parts, 0, sizeof parts);
    evq.n = 0;
    if (sv.in_progress) B = sv.board;
    music_play(GS_MUS_TITLE);
    game_set_pausable(false);
}

static void gs_quit(void) {
    if (state == S_PLAY || state == S_NIGHT || state == S_BRIEF) save_now();
}

static void gs_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_INK);
    for (int i = 0; i < 30; i++) gfx_pset(x + (i * 37) % w, y + (i * 13) % (h - 20), (t / 20 + i) % 5 ? C_DUSK : C_LIGHT);
    for (int r = 0; r < 3; r++) gfx_circb(x + w / 2, y + h + 40, 70 + r * 26, C_NIGHT);
    for (int i = 0; i < 7; i++) {
        int tx = x + i * 21;
        gfx_rect(tx, y + h - 16, 21, 16, (i & 1) ? C_BROWN : C_MAROON);
        gfx_hline(tx, tx + 20, y + h - 16, C_TAN);
    }
    gfx_rect(x + 84, y + h - 21, 21, 21, C_FOREST);
    gfx_hline(x + 84, x + 104, y + h - 21, C_LEAF);
    spr_draw(&gs_spr[(t / 20) % 2 ? GS_TILLY1 : GS_TILLY2], x + 20, y + h - 30, 0);
    spr_draw(&gs_spr[GS_ADULT_SHELL], x + 86, y + h - 34, 0);
    spr_draw(&gs_spr[GS_CROWN], x + 90, y + h - 39, 0);
    spr_draw(&gs_spr[GS_LARVA_SPARK], x + 50, y + h - 24, 0);
    spr_draw(&gs_spr[GS_EGG], x + 118, y + h - 27, 0);
    spr_draw(&gs_spr[(t / 10) % 2 ? GS_POD1 : GS_POD2], x + 64, y + h - 28, 0);
    /* a little zap */
    if ((t / 30) % 3 == 0) gfx_hline(x + 36, x + 46, y + h - 22, C_YELLOW);
}

static int gs_query(const char *key, int *out) {
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "status")) { *out = B.status; return 1; }
    if (!strcmp(key, "day")) { *out = B.day; return 1; }
    if (!strcmp(key, "kills")) { *out = B.kills; return 1; }
    if (!strcmp(key, "quota")) { *out = B.quota; return 1; }
    if (!strcmp(key, "energy")) { *out = B.energy; return 1; }
    if (!strcmp(key, "px")) { *out = B.px; return 1; }
    if (!strcmp(key, "py")) { *out = B.py; return 1; }
    if (!strcmp(key, "bugs")) { *out = gs_count_bugs(&B, -1); return 1; }
    if (!strcmp(key, "eggs")) { *out = gs_count_bugs(&B, LV_EGG); return 1; }
    if (!strcmp(key, "spent")) { int n = 0; for (int i = 0; i < SLOTS; i++) n += B.spent[i]; *out = n; return 1; }
    if (!strcmp(key, "streak")) { *out = sv.streak; return 1; }
    if (!strcmp(key, "contract")) { *out = B.contract; return 1; }
    if (!strcmp(key, "in_progress")) { *out = sv.in_progress; return 1; }
    if (!strcmp(key, "combo")) { *out = B.best_combo; return 1; }
    if (!strncmp(key, "chip", 4)) { *out = B.chips[atoi(key + 4) % SLOTS]; return 1; }
    if (!strncmp(key, "pods_at", 7)) { int x = key[7] - '0', y = key[8] - '0'; *out = B.pods[y][x]; return 1; }
    if (!strncmp(key, "hole_at", 7)) { int x = key[7] - '0', y = key[8] - '0'; *out = B.hole[y][x]; return 1; }
    if (!strncmp(key, "bug_at", 6)) { int x = key[6] - '0', y = key[7] - '0'; *out = B.bsp[y][x]; return 1; }
    if (!strncmp(key, "botwins", 7)) {
        /* balance probe: how many of N seeded contracts does the greedy bot win? */
        int n = atoi(key + 7), wins = 0, contract = 1;
        const char *cp = strchr(key + 7, 'c');
        if (cp) contract = atoi(cp + 1);
        if (n <= 0) n = 10;
        for (int i = 0; i < n; i++) {
            Board t;
            gs_new_contract(&t, contract, 1000 + (uint64_t)i * 77);
            gs_bot_contract(&t);
            wins += t.status == ST_WON;
        }
        *out = wins;
        return 1;
    }
    return 0;
}

static int gs_cheat(const char *cmd) {
    int a, b2, c, d;
    if (!strncmp(cmd, "contract", 8)) {
        /* contract N SEED : start a specific contract immediately */
        a = 1; c = 42;
        sscanf(cmd + 8, "%d %d", &a, &c);
        gs_new_contract(&B, a, (uint64_t)c);
        sv.in_progress = 1;
        sv.streak = a - 1;
        enter_play();
        save_now();
        return 1;
    }
    if (!strcmp(cmd, "clear")) {
        /* an empty flat field for rule tests */
        memset(B.elev, 0, sizeof B.elev);
        memset(B.hole, 0, sizeof B.hole);
        memset(B.pods, 0, sizeof B.pods);
        memset(B.bsp, 0, sizeof B.bsp);
        B.px = 3; B.py = 3;
        tilly_dx = 3; tilly_dy = 3;
        return 1;
    }
    if (sscanf(cmd, "bug %d %d %d %d", &a, &b2, &c, &d) == 4) {
        B.bsp[b2][a] = (uint8_t)c; B.blv[b2][a] = (uint8_t)d;
        B.bhp[b2][a] = (uint8_t)((c == SP_SHELL && d >= 1 && d <= 2) ? 2 : 1);
        return 1;
    }
    if (sscanf(cmd, "pods %d %d %d", &a, &b2, &c) == 3) { B.pods[b2][a] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "hole %d %d", &a, &b2) == 2) { B.hole[b2][a] = 1; return 1; }
    if (sscanf(cmd, "elev %d %d %d", &a, &b2, &c) == 3) { B.elev[b2][a] = (uint8_t)c; return 1; }
    if (sscanf(cmd, "tilly %d %d", &a, &b2) == 2) { B.px = (int8_t)a; B.py = (int8_t)b2; tilly_dx = a; tilly_dy = b2; return 1; }
    if (sscanf(cmd, "energy %d", &a) == 1) { B.energy = (uint16_t)a; return 1; }
    if (sscanf(cmd, "kills %d", &a) == 1) { B.kills = (uint16_t)a; return 1; }
    if (sscanf(cmd, "day %d", &a) == 1) { B.day = (uint8_t)a; return 1; }
    if (sscanf(cmd, "shop %d %d", &a, &b2) == 2) { B.shop[a] = (uint8_t)b2; return 1; }
    if (sscanf(cmd, "use %d %d %d", &a, &b2, &c) == 3) {
        /* use SLOT X Y directly (rule tests) */
        hand_sel = a;
        do_action(a, b2, c);
        return 1;
    }
    if (!strcmp(cmd, "rest")) { do_rest(); return 1; }
    if (sscanf(cmd, "buy %d %d", &a, &b2) == 2) {
        Events ev;
        ev.n = 0;
        if (gs_buy(&B, a, b2, &ev)) play_events(&ev);
        return 1;
    }
    if (!strcmp(cmd, "autoplay")) {
        /* the bot finishes the current contract */
        gs_bot_contract(&B);
        tilly_dx = B.px; tilly_dy = B.py;
        state = S_PLAY;
        state_t = 1;
        return 1;
    }
    if (!strcmp(cmd, "bot_turn")) { Events ev; ev.n = 0; if (gs_bot_turn(&B, &ev)) play_events(&ev); return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    return 0;
}

const GameDef GAME_GRUBSHIFT = {
    "grubshift",
    "GRUB SHIFT",
    "1983",
    "TACTICS",
    "TILLY THE ROBOT MUST CLEAR GRUBS FROM A COMET GREENHOUSE.",
    {"SQUASH 4 GRUBS IN ONE GO", "3 CONTRACTS IN A ROW", "5 CONTRACTS IN A ROW"},
    "D-PAD\tMOVE CURSOR\n"
    GLYPH_A "\tPICK TOOL / TARGET / BUY\n"
    GLYPH_B "\tCANCEL\n"
    "UP\tFROM TOOLS TO THE SHOP\n"
    "START\tPAUSE\n\n"
    "EACH TOOL WORKS ONCE PER SHIFT.\n"
    "REST ENDS THE SHIFT.",
    C_JADE, C_LIME,
    gs_load, gs_start, gs_update, gs_draw, gs_quit, gs_label, gs_query, gs_cheat,
    "BUG HUNTER", 2,
};
