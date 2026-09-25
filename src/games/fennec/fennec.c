/* FENNEC FOUNTAIN - block-pushing rooms around a dried-up oasis.
 * Cartridge 15 of UFO 40, a tribute to Block Koala (UFO 50 #15).
 * See docs/games/15-fennec-fountain.md. Rules live in fennec_logic.c,
 * the fifty rooms in fennec_rooms.c; this file is the hub, the rooms'
 * presentation, the editor and the flow. */
#include "fennec.h"

#define TS 16
#define HUB_W 40
#define HUB_H 26
#define UNDO_MAX 600

enum { S_TITLE, S_HUB, S_ROOM, S_CLEAR, S_TALK, S_SLOTS, S_EDIT, S_EDITMENU, S_ENDING };
enum { AREA_1, AREA_2, AREA_3, AREA_4, AREA_5, AREA_P };

typedef struct Save {
    uint32_t magic;
    uint32_t done_lo, done_hi;               /* a bit per room */
    uint8_t hub_x, hub_y, seen_intro, alien;
    char custom[FN_CUSTOM][FN_H][FN_W];      /* the ten custom rooms */
    uint8_t custom_used[FN_CUSTOM], custom_done[FN_CUSTOM];
} Save;
#define SAVE_MAGIC 0x464E0001u

static Save sv;
static int state, state_t, frame_t;

/* ------------------------------------------------------------------ */
/* the hub garden                                                       */

static const char *const HUB[HUB_H] = {
    "########################################",
    "#b..T..b..T.b#c..c..c..c..#.....f......#",
    "#............#............#.P........P.#",
    "#.TT......TT.#.T..~~~~..T.#............#",
    "#b..........b#c...~~~~...c#.P.~~~~~~.P.#",
    "#....~~~~....#.....N......#...~~~~~~...#",
    "#....~~~~....2...........c#.P.~~~~~~.P.#",
    "#b....N.....b#c...........#............#",
    "#.TT......TT.#..T......T..#.P........P.#",
    "#............#c..c....c..c#.....N......#",
    "#..b......T..#............#............#",
    "#............#............#............#",
    "######1############3############5#######",
    "#............#............#............#",
    "#a..T....T..a#d..d....d..d#e..e....e..e#",
    "#............#............#............#",
    "#..a..~~~..a.#.T..~~~~..T.#.T..~~~~..T.#",
    "#.....~~~....#d...~~~~...d#e...~~~~...e#",
    "#a..........a#......N.....4............#",
    "#..T..N...T..#d..........d#e....N.....e#",
    "#.a........a.#..T......T..#..T......T..#",
    "#............#.d........d.#.e..e..e....#",
    "#..W.........#............#............#",
    "#K...........#............#............#",
    "#............#............#............#",
    "########################################",
};

static const int GATE_NEED[5] = {5, 10, 20, 30, 40};
static const char AREA_DOOR[6] = {'a', 'b', 'c', 'd', 'e', 'f'};
static const int AREA_FIRST[6] = {1, 9, 17, 29, 39, 50};

static int8_t door_x[FN_ROOMS], door_y[FN_ROOMS];
static int8_t room_at[HUB_H][HUB_W]; /* room number - 1 at door tiles, else -1 */

static int area_of(int x, int y) {
    if (y < 12) return x < 13 ? AREA_2 : x < 26 ? AREA_3 : AREA_P;
    return x < 13 ? AREA_1 : x < 26 ? AREA_4 : AREA_5;
}

static void hub_parse(void) {
    memset(room_at, -1, sizeof room_at);
    for (int a = 0; a < 6; a++) {
        int n = AREA_FIRST[a] - 1;
        for (int y = 0; y < HUB_H; y++)
            for (int x = 0; x < HUB_W; x++)
                if (HUB[y][x] == AREA_DOOR[a] && n < FN_ROOMS) {
                    door_x[n] = (int8_t)x;
                    door_y[n] = (int8_t)y;
                    room_at[y][x] = (int8_t)n;
                    n++;
                }
    }
}

static bool done(int room) { return room < 32 ? (sv.done_lo >> room) & 1 : (sv.done_hi >> (room - 32)) & 1; }
static void set_done(int room) {
    if (room < 32) sv.done_lo |= 1u << room;
    else sv.done_hi |= 1u << (room - 32);
}
static int drops(void) {
    int n = 0;
    for (int i = 0; i < FN_ROOMS; i++) n += done(i);
    return n;
}
static bool area_done(int a) {
    int first = AREA_FIRST[a] - 1, last = a == AREA_P ? 49 : AREA_FIRST[a + 1] - 2;
    for (int i = first; i <= last; i++)
        if (!done(i)) return false;
    return true;
}

static bool hub_solid(int x, int y) {
    if (x < 0 || y < 0 || x >= HUB_W || y >= HUB_H) return true;
    char c = HUB[y][x];
    if (c >= '1' && c <= '5') return drops() < GATE_NEED[c - '1'];
    return c == '#' || c == 'T' || c == '~' || c == 'P' || c == 'N';
}

static const char *npc_text(int area) {
    switch (area) {
    case AREA_1: return "ZIZI: THE SPRINGS ARE PLUGGED! PUSH EACH\nWATER STONE ONTO ITS DRY SPRING.\nMADE A MESS? " GLYPH_B " TAKES BACK A STEP.\nTHE WORKSHOP LETS YOU BUILD ROOMS.";
    case AREA_2: return "OLD SHEIKHA: A BLOCK PUSHES ANOTHER ONLY\nIF ITS NUMBER IS AS BIG OR BIGGER.\nPUSH ONE ONTO A 1 AND THEY ADD UP.\nAT FIVE THEY TURN TO MARBLE.";
    case AREA_3: return "THE TILE-SETTER: LAPIS WON'T MOVE FOR\nPAWS. PUSH IT WITH ANOTHER BLOCK AS\nHEAVY AS IT, OR HEAVIER.";
    case AREA_4: return "THE MASON: BASALT IS OLD AND TIRED.\nEACH TIME YOU STOP PUSHING IT, IT\nCRUMBLES A SIZE SMALLER.";
    case AREA_5: return "COUSIN FARIS: THE GECKOS COPY EVERY\nSTEP YOU TAKE. EVERY SINGLE ONE.";
    default: return "GRAND VIZIER HUMPH: MY BATH! MY LOVELY\nBATH! FINE. ONE LAST SPRING. IF YOU\nCAN MOVE IT, LITTLE FOX.";
    }
}

/* ------------------------------------------------------------------ */
/* save & goals                                                         */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) {
        sv = tmp;
    } else {
        memset(&sv, 0, sizeof sv);
        sv.magic = SAVE_MAGIC;
    }
    for (int y = 0; y < HUB_H; y++)
        for (int x = 0; x < HUB_W; x++)
            if (HUB[y][x] == 'K' && !sv.hub_x) { sv.hub_x = (uint8_t)x; sv.hub_y = (uint8_t)y; }
}

/* ------------------------------------------------------------------ */
/* playing a room                                                       */

static FnRoom R;
static FnState S, prev_S;
static FnState undo_stack[UNDO_MAX];
static int undo_n;
static FnEvents last_ev;
static int cur_room;          /* 0..49, or 100 + custom slot */
static bool testing;          /* editor test play */
static int anim_t, face = 2, bump_t, queued = -1;
static int moves_made;
static const char *autoplay; /* the headless runner can play a solution back */

typedef struct Part {
    float x, y, vx, vy;
    int life, col, kind;
} Part;
static Part parts[120];

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) {
            parts[i] = (Part){x, y, vx, vy, life, col, kind};
            return;
        }
}

static int room_ox(void) { return (SCREEN_W - R.w * TS) / 2; }
static int room_oy(void) { return 18 + (162 - R.h * TS) / 2; }

static char adhoc[FN_H][FN_W + 1];

static const char *const *room_rows(int room) {
    static const char *rows[FN_H];
    if (room == 99) {
        for (int y = 0; y < FN_H; y++) rows[y] = adhoc[y][0] ? adhoc[y] : NULL;
        return rows;
    }
    if (room >= 100) {
        for (int y = 0; y < FN_H; y++) rows[y] = sv.custom[room - 100][y];
        return rows;
    }
    return FN_ROOMS_DEF[room].rows;
}

static bool load_room(int room) {
    autoplay = NULL;
    static char buf[FN_H][FN_W + 1];
    const char *rows[FN_H];
    const char *const *src = room_rows(room);
    for (int y = 0; y < FN_H; y++) {
        if (!src[y]) { rows[y] = NULL; continue; }
        snprintf(buf[y], sizeof buf[y], "%.*s", FN_W, src[y]);
        rows[y] = buf[y];
    }
    if (fn_parse(rows, &R, &S) != 0) return false;
    prev_S = S;
    undo_n = 0;
    anim_t = 99;
    last_ev.n = 0;
    face = 2;
    queued = -1;
    moves_made = 0;
    memset(parts, 0, sizeof parts);
    return true;
}

static void enter_room(int room) {
    if (!load_room(room)) return;
    cur_room = room;
    state = S_ROOM;
    state_t = 0;
    game_set_pausable(true);
    music_play(room == 49 ? FN_MUS_PALACE : FN_MUS_ROOM);
    sfx_play_name("fn_door");
}

static void go_hub(void) {
    state = S_HUB;
    state_t = 0;
    testing = false;
    game_set_pausable(true);
    music_play(FN_MUS_HUB);
}

static void check_goals_on_clear(int room) {
    if (room >= 100) {
        game_award(GOAL_BEACON); /* a custom room completed */
        return;
    }
    if (room == 49) {
        game_award(GOAL_SAUCER);
        /* like the original, the last room checks for every drop */
        if (drops() >= FN_ROOMS) { sv.alien = 1; game_award(GOAL_ALIEN); }
    }
}

static void room_won(void) {
    state = S_CLEAR;
    state_t = 0;
    music_restart(FN_MUS_WIN);
    if (cur_room == 99) return; /* a test room from the headless runner */
    if (cur_room < 100) {
        set_done(cur_room);
        sv.hub_x = (uint8_t)door_x[cur_room];
        sv.hub_y = (uint8_t)door_y[cur_room];
    } else {
        sv.custom_done[cur_room - 100] = 1;
    }
    check_goals_on_clear(cur_room);
    save_now();
    float gx = room_ox() + R.goal_x * TS + 8, gy = room_oy() + R.goal_y * TS + 8;
    for (int i = 0; i < 30; i++) {
        float a = (float)i / 30 * 6.283f;
        part_add(gx, gy, cosf(a) * 1.6f, sinf(a) * 1.2f - 1.4f, 30 + i % 10, i % 3 ? C_CYAN : C_ICE, 0);
    }
}

static void do_step(int dir) {
    if (undo_n < UNDO_MAX) undo_stack[undo_n++] = S;
    else { memmove(undo_stack, undo_stack + 1, sizeof undo_stack[0] * (UNDO_MAX - 1)); undo_stack[UNDO_MAX - 1] = S; }
    prev_S = S;
    last_ev.n = 0;
    bool moved = fn_step(&R, &S, dir, &last_ev);
    face = dir;
    anim_t = 0;
    moves_made++;
    if (!moved) {
        bump_t = 8;
        sfx_play_name("fn_bump");
        /* a bump that changed nothing isn't worth an undo step */
        if (!memcmp(&prev_S, &S, sizeof S)) undo_n--;
    } else {
        sfx_play_name("fn_step");
    }
    bool pushed = false, merged = false, marble = false, shrank = false, crumbled = false, gecko = false;
    for (int i = 0; i < last_ev.n; i++) {
        FnEvent *e = &last_ev.ev[i];
        float x = room_ox() + e->x * TS + 8, y = room_oy() + e->y * TS + 8;
        switch (e->type) {
        case FE_PUSH: pushed = true; break;
        case FE_MERGE:
            merged = true;
            for (int k = 0; k < 8; k++) part_add(room_ox() + e->x2 * TS + 8, room_oy() + e->y2 * TS + 8, (k - 3.5f) * 0.4f, -1.0f - (k % 3) * 0.3f, 16, C_YELLOW, 1);
            break;
        case FE_MARBLE: marble = true; break;
        case FE_SHRINK: shrank = true; for (int k = 0; k < 10; k++) part_add(x + 8, y + 8, (k - 5) * 0.4f, -0.8f, 20, C_SLATE, 0); break;
        case FE_CRUMBLE: crumbled = true; for (int k = 0; k < 14; k++) part_add(x, y, (k - 7) * 0.4f, -1.2f, 22, k % 2 ? C_DUSK : C_SLATE, 0); break;
        case FE_GECKO: gecko = true; break;
        default: break;
        }
    }
    if (marble) sfx_play_name("fn_marble");
    else if (merged) sfx_play_name("fn_merge");
    else if (crumbled) sfx_play_name("fn_crumble");
    else if (shrank) sfx_play_name("fn_shrink");
    else if (pushed) sfx_play_name("fn_push");
    else if (gecko) sfx_play_name("fn_gecko");
}

static void undo(void) {
    if (undo_n <= 0) { sfx_play_name("fn_bump"); return; }
    S = undo_stack[--undo_n];
    prev_S = S;
    last_ev.n = 0;
    anim_t = 99;
    sfx_play_name("fn_undo");
}

static void leave_room(void) {
    if (cur_room == 99) { go_hub(); return; }
    if (testing) { state = S_EDIT; state_t = 0; music_play(FN_MUS_EDIT); return; }
    if (cur_room >= 100) { state = S_SLOTS; state_t = 0; music_play(FN_MUS_EDIT); return; }
    sv.hub_x = (uint8_t)door_x[cur_room];
    sv.hub_y = (uint8_t)door_y[cur_room];
    save_now();
    go_hub();
}

static void update_room(void) {
    if (bump_t > 0) bump_t--;
    if (anim_t < 99) anim_t++;
    int dir = -1;
    if (btn_repeat(BTN_UP)) dir = DIR_UP;
    else if (btn_repeat(BTN_DOWN)) dir = DIR_DOWN;
    else if (btn_repeat(BTN_LEFT)) dir = DIR_LEFT;
    else if (btn_repeat(BTN_RIGHT)) dir = DIR_RIGHT;
    if (dir >= 0) queued = dir;
    if (btn_repeat(BTN_B)) { undo(); queued = -1; }
    /* custom rooms may have no door: SELECT walks out of them */
    if (cur_room >= 100 && btnp(BTN_SELECT)) { leave_room(); return; }
    if (autoplay && *autoplay && anim_t >= 8 && queued < 0) {
        char c = *autoplay++;
        queued = c == 'U' ? DIR_UP : c == 'R' ? DIR_RIGHT : c == 'D' ? DIR_DOWN : DIR_LEFT;
    }
    if (queued >= 0 && anim_t >= 5) {
        int d = queued;
        queued = -1;
        do_step(d);
        if (S.won) { room_won(); return; }
        if (S.exited) { sfx_play_name("fn_door"); leave_room(); return; }
    }
}

/* ------------------------------------------------------------------ */
/* the hub                                                              */

static int hub_px, hub_py, hub_face = 2, hub_move_t, hub_from_x, hub_from_y, hub_bump;
static int talk_area, gate_msg_t, gate_msg_need;

static void hub_enter(void) {
    hub_px = sv.hub_x;
    hub_py = sv.hub_y;
    hub_from_x = hub_px;
    hub_from_y = hub_py;
    hub_move_t = 99;
}

static void update_hub(void) {
    if (hub_move_t < 99) hub_move_t++;
    if (gate_msg_t > 0) gate_msg_t--;
    if (hub_bump > 0) hub_bump--;
    if (hub_move_t >= 6) {
        int dx = 0, dy = 0;
        if (btn(BTN_UP)) dy = -1;
        else if (btn(BTN_DOWN)) dy = 1;
        else if (btn(BTN_LEFT)) dx = -1;
        else if (btn(BTN_RIGHT)) dx = 1;
        if (dx || dy) {
            hub_face = dy < 0 ? DIR_UP : dy > 0 ? DIR_DOWN : dx < 0 ? DIR_LEFT : DIR_RIGHT;
            int nx = hub_px + dx, ny = hub_py + dy;
            if (!hub_solid(nx, ny)) {
                hub_from_x = hub_px;
                hub_from_y = hub_py;
                hub_px = nx;
                hub_py = ny;
                hub_move_t = 0;
                if (frame_t % 2 == 0) sfx_play_name("fn_step");
            } else {
                char c = HUB[iclamp(ny, 0, HUB_H - 1)][iclamp(nx, 0, HUB_W - 1)];
                if (c >= '1' && c <= '5' && hub_bump == 0) {
                    gate_msg_need = GATE_NEED[c - '1'];
                    gate_msg_t = 90;
                    sfx_play_name("fn_bump");
                    hub_bump = 20;
                }
            }
        }
    }
    if (btnp(BTN_A)) {
        int r = room_at[hub_py][hub_px];
        if (r >= 0) {
            sv.hub_x = (uint8_t)hub_px;
            sv.hub_y = (uint8_t)hub_py;
            save_now();
            enter_room(r);
            return;
        }
        if (HUB[hub_py][hub_px] == 'W') {
            sfx_play_name("fn_door");
            state = S_SLOTS;
            state_t = 0;
            music_play(FN_MUS_EDIT);
            return;
        }
        static const int8_t FX[4] = {0, 1, 0, -1}, FY[4] = {-1, 0, 1, 0};
        int tx = hub_px + FX[hub_face], ty = hub_py + FY[hub_face];
        if (tx >= 0 && ty >= 0 && tx < HUB_W && ty < HUB_H && HUB[ty][tx] == 'N') {
            talk_area = area_of(tx, ty);
            state = S_TALK;
            state_t = 0;
            sfx_play_name("fn_talk");
        }
    }
}

/* ------------------------------------------------------------------ */
/* the workshop: ten custom rooms                                       */

enum {
    P_WALL, P_FLOOR, P_FENNEC, P_SPRING, P_STONE, P_DOOR, P_GECKO, P_S1, P_S2, P_S3, P_S4, P_MARBLE,
    P_L1, P_L2, P_L3, P_L4, P_B1, P_B2, P_B3, P_B4, P_COUNT
};
static const char PIECE_CH[P_COUNT] = {'#', '.', 'K', 'G', 'S', 'D', 'g', '1', '2', '3', '4', '5',
                                       'a', 'b', 'c', 'd', 'w', 'x', 'y', 'z'};
static const char *PIECE_NAME[P_COUNT] = {"WALL", "FLOOR", "FENNEC", "SPRING", "WATER STONE", "DOOR", "GECKO",
                                          "SANDSTONE 1", "SANDSTONE 2", "SANDSTONE 3", "SANDSTONE 4", "MARBLE",
                                          "LAPIS 1", "LAPIS 2", "LAPIS 3", "LAPIS 4",
                                          "BASALT 1", "BASALT 2", "BASALT 3", "BASALT 4"};
static int slot_sel, slot_menu, ed_x = 1, ed_y = 1, ed_piece = P_WALL, menu_sel, ed_slot, ed_msg_t;
static const char *ed_msg;

static void custom_blank(int slot) {
    for (int y = 0; y < FN_H; y++)
        for (int x = 0; x < FN_W; x++)
            sv.custom[slot][y][x] = (y == 0 || x == 0 || y == FN_H - 1 || x == FN_W - 1) ? '#' : '.';
    sv.custom_used[slot] = 1;
    sv.custom_done[slot] = 0;
}

/* each custom room is stored as a full 16 x 9 grid of room letters */
static char ed_get(int x, int y) { return sv.custom[ed_slot][y][x]; }

static void ed_clear_piece_at(int x, int y) {
    char c = ed_get(x, y);
    if (c >= 'w' && c <= 'z') {
        /* remove the whole basalt square this tile belongs to */
        for (int yy = 0; yy < FN_H; yy++)
            for (int xx = 0; xx < FN_W; xx++)
                if (sv.custom[ed_slot][yy][xx] == c) {
                    int n = c - 'w' + 1;
                    if (x >= xx && x < xx + n && y >= yy && y < yy + n) {
                        for (int a = yy; a < yy + n && a < FN_H; a++)
                            for (int b = xx; b < xx + n && b < FN_W; b++)
                                if (sv.custom[ed_slot][a][b] == c) sv.custom[ed_slot][a][b] = '.';
                        return;
                    }
                }
    }
    sv.custom[ed_slot][y][x] = '.';
}

static void ed_place(int piece) {
    char ch = PIECE_CH[piece];
    sv.custom_done[ed_slot] = 0;
    if (ch == 'K' || ch == 'G' || ch == 'S' || ch == 'D') {
        /* only one of these per room */
        for (int y = 0; y < FN_H; y++)
            for (int x = 0; x < FN_W; x++)
                if (sv.custom[ed_slot][y][x] == ch) sv.custom[ed_slot][y][x] = '.';
    }
    if (ch >= 'w' && ch <= 'z') {
        int n = ch - 'w' + 1;
        if (ed_x + n > FN_W || ed_y + n > FN_H) { ed_msg = "NO ROOM FOR THAT BASALT"; ed_msg_t = 60; sfx_play_name("fn_bump"); return; }
        for (int y = ed_y; y < ed_y + n; y++)
            for (int x = ed_x; x < ed_x + n; x++) ed_clear_piece_at(x, y);
        for (int y = ed_y; y < ed_y + n; y++)
            for (int x = ed_x; x < ed_x + n; x++) sv.custom[ed_slot][y][x] = ch;
    } else {
        ed_clear_piece_at(ed_x, ed_y);
        sv.custom[ed_slot][ed_y][ed_x] = ch;
    }
    sfx_play_name("fn_place");
}

static int count_ch(int slot, char ch) {
    int n = 0;
    for (int y = 0; y < FN_H; y++)
        for (int x = 0; x < FN_W; x++) n += sv.custom[slot][y][x] == ch;
    return n;
}

static void ed_test(void) {
    if (count_ch(ed_slot, 'K') != 1 || count_ch(ed_slot, 'G') != 1 || count_ch(ed_slot, 'S') != 1) {
        ed_msg = "NEEDS A FENNEC, A SPRING AND A STONE";
        ed_msg_t = 90;
        sfx_play_name("fn_bump");
        return;
    }
    save_now();
    testing = true;
    enter_room(100 + ed_slot);
}

static void update_slots(void) {
    if (slot_menu) {
        if (btn_repeat(BTN_UP)) { slot_menu = slot_menu == 1 ? 3 : slot_menu - 1; sfx_play_name("ui_move"); }
        if (btn_repeat(BTN_DOWN)) { slot_menu = slot_menu == 3 ? 1 : slot_menu + 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { slot_menu = 0; sfx_play_name("ui_back"); }
        if (btnp(BTN_A)) {
            int m = slot_menu;
            slot_menu = 0;
            ed_slot = slot_sel;
            if (!sv.custom_used[ed_slot]) custom_blank(ed_slot);
            if (m == 1) { state = S_EDIT; state_t = 0; sfx_play_name("ui_ok"); }
            else if (m == 2) {
                if (count_ch(ed_slot, 'K') == 1 && count_ch(ed_slot, 'G') == 1 && count_ch(ed_slot, 'S') == 1) {
                    testing = false;
                    enter_room(100 + ed_slot);
                } else sfx_play_name("ui_error");
            } else sfx_play_name("ui_back");
        }
        return;
    }
    if (btn_repeat(BTN_UP)) { slot_sel = (slot_sel + FN_CUSTOM - 1) % FN_CUSTOM; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { slot_sel = (slot_sel + 1) % FN_CUSTOM; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { save_now(); sfx_play_name("ui_back"); go_hub(); return; }
    if (btnp(BTN_A) && state_t > 5) { slot_menu = 1; sfx_play_name("ui_ok"); }
}

static void update_edit(void) {
    if (ed_msg_t > 0) ed_msg_t--;
    int dx = btn_repeat(BTN_RIGHT) - btn_repeat(BTN_LEFT), dy = btn_repeat(BTN_DOWN) - btn_repeat(BTN_UP);
    if (dx || dy) {
        ed_x = iclamp(ed_x + dx, 0, FN_W - 1);
        ed_y = iclamp(ed_y + dy, 0, FN_H - 1);
        sfx_play_name("ui_move");
    }
    if (btn(BTN_A) && (btnp(BTN_A) || dx || dy)) ed_place(ed_piece);
    if (btn(BTN_B) && (btnp(BTN_B) || dx || dy)) {
        ed_clear_piece_at(ed_x, ed_y);
        sv.custom_done[ed_slot] = 0;
        sfx_play_name("fn_erase");
    }
    if (btnp(BTN_SELECT)) { state = S_EDITMENU; state_t = 0; menu_sel = ed_piece; sfx_play_name("ui_ok"); }
}

#define MENU_ITEMS (P_COUNT + 3) /* pieces, then TEST, SAVE AND EXIT, CLEAR */

static void update_editmenu(void) {
    int cols = 5;
    int dx = btn_repeat(BTN_RIGHT) - btn_repeat(BTN_LEFT), dy = btn_repeat(BTN_DOWN) - btn_repeat(BTN_UP);
    if (dx || dy) {
        if (menu_sel < P_COUNT) {
            int c = menu_sel % cols, r = menu_sel / cols;
            c = iclamp(c + dx, 0, cols - 1);
            r += dy;
            if (r < 0) r = 0;
            int n = r * cols + c;
            if (n >= P_COUNT) n = P_COUNT;
            menu_sel = n;
        } else {
            if (dy < 0) menu_sel = menu_sel == P_COUNT ? P_COUNT - 1 : menu_sel - 1;
            else if (dy > 0) menu_sel = imin(menu_sel + 1, MENU_ITEMS - 1);
        }
        sfx_play_name("ui_move");
    }
    if (btnp(BTN_B) || btnp(BTN_SELECT)) { state = S_EDIT; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A)) {
        if (menu_sel < P_COUNT) { ed_piece = menu_sel; state = S_EDIT; sfx_play_name("ui_ok"); }
        else if (menu_sel == P_COUNT) { state = S_EDIT; ed_test(); }
        else if (menu_sel == P_COUNT + 1) { save_now(); sfx_play_name("ui_ok"); state = S_SLOTS; state_t = 0; }
        else { custom_blank(ed_slot); sfx_play_name("fn_erase"); state = S_EDIT; }
    }
}

/* ------------------------------------------------------------------ */
/* update                                                               */

static void fn_update(void) {
    frame_t++;
    state_t++;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        if (p->kind == 0) p->vy += 0.08f;
    }
    switch (state) {
    case S_TITLE:
        game_set_pausable(false);
        if (btnp(BTN_B) && state_t > 10) { game_exit_to_library(); break; }
        if (state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) {
            sfx_play_name("ui_ok");
            input_consume();
            if (!sv.seen_intro) { sv.seen_intro = 1; save_now(); state = S_TALK; talk_area = -1; state_t = 0; hub_enter(); music_play(FN_MUS_HUB); }
            else { hub_enter(); go_hub(); }
        }
        break;
    case S_HUB: update_hub(); break;
    case S_TALK:
        if (state_t > 15 && (btnp(BTN_A) || btnp(BTN_B))) { sfx_play_name("fn_talk"); state = S_HUB; state_t = 0; game_set_pausable(true); }
        break;
    case S_ROOM: update_room(); break;
    case S_CLEAR:
        if (anim_t < 99) anim_t++; /* finish the last slide */
        if (state_t > 100 || (state_t > 40 && btnp(BTN_A))) {
            if (testing) { testing = false; state = S_EDIT; state_t = 0; ed_msg = "COMPLETE! IT CAN BE SOLVED."; ed_msg_t = 120; music_play(FN_MUS_EDIT); }
            else if (cur_room >= 100) { state = S_SLOTS; state_t = 0; music_play(FN_MUS_EDIT); }
            else if (cur_room == 99) { hub_enter(); go_hub(); }
            else if (cur_room == 49) { state = S_ENDING; state_t = 0; music_play(FN_MUS_END); }
            else { hub_enter(); go_hub(); }
        }
        break;
    case S_SLOTS: update_slots(); break;
    case S_EDIT: update_edit(); break;
    case S_EDITMENU: update_editmenu(); break;
    case S_ENDING:
        if (state_t > 180 && (btnp(BTN_A) || btnp(BTN_START))) { hub_enter(); go_hub(); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing: blocks and tiles                                            */

static void draw_number(int n, int cx, int cy, int col, int shadow) {
    char buf[4];
    snprintf(buf, sizeof buf, "%d", n);
    text_draw(buf, cx - 2 + 1, cy - 3 + 1, shadow);
    text_draw(buf, cx - 2, cy - 3, col);
}

static void draw_block(const FnBlock *b, int px, int py) {
    int sz = b->kind == BK_BASALT ? b->n : 1;
    int w = sz * TS, h = sz * TS;
    switch (b->kind) {
    case BK_SAND:
        gfx_rect(px + 1, py + 2, w - 2, h - 2, C_BROWN);
        gfx_rect(px + 1, py + 1, w - 2, h - 4, C_ORANGE);
        gfx_rect(px + 2, py + 1, w - 4, 2, C_AMBER);
        gfx_dither(px + 2, py + 4, w - 4, h - 8, C_RED, 2);
        gfx_rectb(px, py, w, h - 1, C_INK);
        draw_number(b->n, px + w / 2, py + h / 2 - 1, C_CREAM, C_MAROON);
        break;
    case BK_LAPIS:
        gfx_rect(px + 1, py + 2, w - 2, h - 2, C_NAVY);
        gfx_rect(px + 1, py + 1, w - 2, h - 4, C_BLUE);
        gfx_rect(px + 2, py + 1, w - 4, 2, C_SKY);
        gfx_pset(px + w - 3, py + 3, C_YELLOW);
        gfx_pset(px + 2, py + h - 5, C_YELLOW);
        gfx_rectb(px, py, w, h - 1, C_INK);
        draw_number(b->n, px + w / 2, py + h / 2 - 1, C_ICE, C_NAVY);
        break;
    case BK_BASALT:
        gfx_rect(px + 1, py + 2, w - 2, h - 2, C_NIGHT);
        gfx_rect(px + 1, py + 1, w - 2, h - 4, C_DUSK);
        gfx_rect(px + 2, py + 1, w - 4, 2, C_SLATE);
        gfx_dither(px + 2, py + 4, w - 4, h - 8, C_NIGHT, 3);
        for (int k = 0; k < sz; k++) gfx_line(px + 4 + k * 13, py + 5, px + 7 + k * 13, py + h - 7, C_NIGHT);
        gfx_rectb(px, py, w, h - 1, C_INK);
        draw_number(b->n, px + w / 2, py + h / 2 - 1, C_LIGHT, C_INK);
        break;
    case BK_MARBLE:
        gfx_rect(px + 1, py + 2, w - 2, h - 2, C_GREY);
        gfx_rect(px + 1, py + 1, w - 2, h - 4, C_LIGHT);
        gfx_rect(px + 2, py + 1, w - 4, 2, C_WHITE);
        gfx_line(px + 3, py + 4, px + 8, py + 10, C_GREY);
        gfx_line(px + 8, py + 10, px + 12, py + 6, C_GREY);
        gfx_rectb(px, py, w, h - 1, C_INK);
        draw_number(5, px + w / 2, py + h / 2 - 1, C_SLATE, C_WHITE);
        break;
    case BK_STONE:
        spr_draw(&fn_spr[FS_STONE], px, py - ((frame_t / 20) % 2), 0);
        break;
    default: break;
    }
}

static void draw_floor_tile(int px, int py, int x, int y) {
    gfx_rect(px, py, TS, TS, C_HIDE);
    gfx_dither(px, py, TS, TS, C_CREAM, 2);
    uint32_t h = (uint32_t)(x * 73856093u ^ y * 19349663u);
    gfx_pset(px + (int)(h % 14) + 1, py + (int)((h >> 5) % 14) + 1, C_EARTH);
    gfx_pset(px + (int)((h >> 9) % 14) + 1, py + (int)((h >> 13) % 14) + 1, C_EARTH);
}

static void draw_wall_tile(int px, int py, int x, int y, bool below_open) {
    gfx_rect(px, py, TS, TS, C_TAN);
    gfx_dither(px, py, TS, TS, C_EARTH, 3);
    gfx_hline(px, px + TS - 1, py, C_EARTH);
    int k = (x + y) % 2 ? 8 : 0;
    gfx_hline(px, px + TS - 1, py + 7, C_BROWN);
    gfx_vline(px + k, py, py + 6, C_BROWN);
    gfx_vline(px + ((k + 8) % 16), py + 8, py + 15, C_BROWN);
    if (below_open) gfx_rect(px, py + TS - 3, TS, 3, C_BROWN);
}

static void draw_room(void) {
    int ox = room_ox(), oy = room_oy();
    float t = anim_t >= 99 ? 1.0f : fminf(1.0f, (anim_t + 1) / 5.0f);
    /* tiles */
    for (int y = 0; y < R.h; y++)
        for (int x = 0; x < R.w; x++) {
            int px = ox + x * TS, py = oy + y * TS;
            if (R.wall[y][x]) {
                bool below = y + 1 < R.h && !R.wall[y + 1][x];
                /* only draw walls that touch the room, leave the outside dark */
                bool near = false;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && ny >= 0 && nx < R.w && ny < R.h && !R.wall[ny][nx]) near = true;
                    }
                if (near) draw_wall_tile(px, py, x, y, below);
            } else {
                draw_floor_tile(px, py, x, y);
            }
        }
    if (R.door_x != FN_NONE) {
        int px = ox + R.door_x * TS, py = oy + R.door_y * TS;
        draw_floor_tile(px, py, R.door_x, R.door_y);
        gfx_rect(px + 2, py + 2, 12, 12, C_BROWN);
        gfx_rect(px + 4, py + 4, 8, 10, C_INK);
    }
    if (R.goal_x != FN_NONE) spr_draw(&fn_spr[S.won ? FS_SPRING_WET : FS_SPRING_DRY], ox + R.goal_x * TS, oy + R.goal_y * TS, 0);
    /* blocks, back to front, sliding in from where they were */
    for (int row = 0; row < R.h; row++)
        for (int i = 0; i < S.nb; i++) {
            const FnBlock *b = &S.b[i];
            if (b->y != row) continue;
            float bx = b->x, by = b->y;
            for (int k = 0; k < last_ev.n && t < 1; k++) {
                const FnEvent *e = &last_ev.ev[k];
                if (e->type == FE_PUSH && e->a == i) { bx = e->x + (e->x2 - e->x) * t; by = e->y + (e->y2 - e->y) * t; }
            }
            draw_block(b, ox + (int)(bx * TS), oy + (int)(by * TS));
        }
    /* merged blocks slide into their new partner, then vanish */
    for (int k = 0; k < last_ev.n && t < 1; k++) {
        const FnEvent *e = &last_ev.ev[k];
        if (e->type != FE_MERGE) continue;
        int px = ox + (int)((e->x + (e->x2 - e->x) * t) * TS), py = oy + (int)((e->y + (e->y2 - e->y) * t) * TS);
        gfx_dither(px + 2, py + 2, TS - 4, TS - 4, C_YELLOW, 8);
    }
    /* geckos */
    for (int g = 0; g < S.ng; g++) {
        float gx = S.gx[g], gy = S.gy[g];
        for (int k = 0; k < last_ev.n && t < 1; k++) {
            const FnEvent *e = &last_ev.ev[k];
            if (e->type == FE_GECKO && e->a == g) { gx = e->x + (e->x2 - e->x) * t; gy = e->y + (e->y2 - e->y) * t; }
        }
        spr_draw(&fn_spr[(frame_t / 12 + g) % 2 ? FS_GECKO1 : FS_GECKO2], ox + (int)(gx * TS), oy + (int)(gy * TS), 0);
    }
    /* Fen */
    float fx = S.px, fy = S.py;
    bool pushing = false;
    for (int k = 0; k < last_ev.n && t < 1; k++) {
        const FnEvent *e = &last_ev.ev[k];
        if (e->type == FE_WALK || e->type == FE_EXIT) { fx = e->x + (e->x2 - e->x) * t; fy = e->y + (e->y2 - e->y) * t; }
        if (e->type == FE_PUSH) pushing = true;
    }
    int spr, flip = 0;
    bool step = (int)(t * 4) % 2 && t < 1;
    if (face == DIR_UP) spr = step ? FS_FEN_U2 : FS_FEN_U;
    else if (face == DIR_DOWN) spr = step ? FS_FEN_D2 : FS_FEN_D;
    else { spr = pushing && t < 1 ? FS_FEN_PUSH : step ? FS_FEN_S2 : FS_FEN_S; flip = face == DIR_LEFT ? SPR_FLIPX : 0; }
    int bx = bump_t > 0 ? ((bump_t / 2) % 2 ? 1 : -1) : 0;
    spr_draw(&fn_spr[spr], ox + (int)(fx * TS) + bx, oy + (int)(fy * TS) - 2, flip);
    /* particles */
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life > 0) gfx_rect((int)p->x, (int)p->y, 2, 2, p->col);
    }
}

static void draw_room_screen(void) {
    gfx_cls(C_NIGHT);
    /* a sandy dusk outside the room */
    for (int y = 16; y < SCREEN_H; y += 4) gfx_dither(0, y, SCREEN_W, 2, C_DUSK, 3);
    draw_room();
    gfx_rect(0, 0, SCREEN_W, 16, C_INK);
    gfx_hline(0, SCREEN_W - 1, 15, C_DUSK);
    char buf[64];
    if (cur_room == 99) snprintf(buf, sizeof buf, "TEST ROOM");
    else if (cur_room >= 100) snprintf(buf, sizeof buf, "ROOM %d  %s", 51 + cur_room - 100, testing ? "TEST" : "CUSTOM");
    else snprintf(buf, sizeof buf, "ROOM %d  %s", cur_room + 1, FN_ROOMS_DEF[cur_room].name);
    text_draw(buf, 4, 4, C_CREAM);
    spr_draw(&fn_spr[FS_DROP], SCREEN_W - 44, 5, 0);
    snprintf(buf, sizeof buf, "%d/50", drops());
    text_draw(buf, SCREEN_W - 36, 4, C_CYAN);
    tiny_draw(testing ? "B UNDO  SELECT EDIT" : cur_room >= 100 ? "B UNDO  SELECT LEAVE" : "B UNDO  DOOR: LEAVE",
              SCREEN_W / 2 - 34, 172, C_SLATE);
}

/* ------------------------------------------------------------------ */
/* drawing: the hub                                                     */

static int cam_x, cam_y;

static void draw_hub_tile(int x, int y, int px, int py) {
    char c = HUB[y][x];
    int area = area_of(x, y);
    switch (c) {
    case '#': {
        bool edge = y + 1 < HUB_H && HUB[y + 1][x] != '#';
        draw_wall_tile(px, py, x, y, edge);
        return;
    }
    case '~': {
        bool wet = area == AREA_P ? !done(49) : area_done(area);
        gfx_rect(px, py, TS, TS, wet ? C_BLUE : C_EARTH);
        if (wet) {
            gfx_dither(px, py, TS, TS, C_SKY, 3);
            if ((x + y + frame_t / 16) % 5 == 0) gfx_hline(px + 3, px + 8, py + 6, C_ICE);
        } else {
            gfx_line(px + 2, py + 3, px + 8, py + 7, C_TAN);
            gfx_line(px + 8, py + 7, px + 13, py + 5, C_TAN);
            gfx_line(px + 6, py + 7, px + 5, py + 13, C_TAN);
        }
        return;
    }
    default: break;
    }
    draw_floor_tile(px, py, x, y);
    if (area == AREA_P) {
        /* palace tiles */
        gfx_dither(px, py, TS, TS, C_LIGHT, 3);
        if ((x + y) % 2) gfx_rect(px + 6, py + 6, 4, 4, C_SKY);
    } else if (c == '.') {
        uint32_t h = (uint32_t)(x * 2654435761u) ^ (uint32_t)(y * 40503u);
        h ^= h >> 15;
        if (h % 7 == 0) { gfx_line(px + 4, py + 12, px + 3, py + 8, C_TAN); gfx_line(px + 6, py + 12, px + 7, py + 7, C_EARTH); gfx_line(px + 8, py + 12, px + 10, py + 9, C_TAN); }
        else if (h % 11 == 1) { gfx_rect(px + 9, py + 10, 3, 2, C_GREY); gfx_pset(px + 9, py + 10, C_LIGHT); }
    }
    switch (c) {
    case 'T': spr_draw(&fn_spr[FS_PALM], px, py - 2, 0); break;
    case 'P':
        gfx_rect(px + 3, py - 8, 10, 22, C_LIGHT);
        gfx_rect(px + 3, py - 8, 3, 22, C_WHITE);
        gfx_rect(px + 1, py - 10, 14, 3, C_WHITE);
        gfx_rect(px + 1, py + 12, 14, 3, C_GREY);
        break;
    case 'W':
        gfx_rect(px, py - 6, TS, 22, C_BROWN);
        gfx_rect(px + 1, py - 5, TS - 2, 4, C_ORANGE);
        gfx_rect(px + 4, py + 4, 8, 12, C_INK);
        gfx_pset(px + 7, py - 3, C_YELLOW);
        break;
    default:
        if (c >= '1' && c <= '5') {
            bool open = drops() >= GATE_NEED[c - '1'];
            if (!open) {
                gfx_rect(px, py + 2, TS, 12, C_BROWN);
                for (int k = 0; k < 4; k++) gfx_vline(px + 2 + k * 4, py, py + 15, C_TAN);
                gfx_hline(px, px + 15, py + 5, C_EARTH);
                gfx_hline(px, px + 15, py + 11, C_EARTH);
                char buf[8];
                snprintf(buf, sizeof buf, "%d", GATE_NEED[c - '1']);
                gfx_rect(px + 2, py - 7, 12, 8, C_CREAM);
                tiny_center(buf, px + 8, py - 6, C_BROWN);
            } else {
                gfx_rect(px, py, 2, TS, C_TAN);
                gfx_rect(px + 14, py, 2, TS, C_TAN);
            }
        }
        break;
    }
}

/* the oasis folk: Fen's family and friends in their own colours */
static const uint8_t *npc_map(int area) {
    static uint8_t maps[6][PAL_COUNT];
    static bool made;
    if (!made) {
        for (int i = 0; i < 6; i++) pal_identity(maps[i]);
        pal_swap(maps[AREA_2], C_EARTH, C_GREY); pal_swap(maps[AREA_2], C_HIDE, C_LIGHT); pal_swap(maps[AREA_2], C_CREAM, C_WHITE);
        pal_swap(maps[AREA_2], C_PINK, C_VIOLET); pal_swap(maps[AREA_2], C_MAGENTA, C_PURPLE);
        pal_swap(maps[AREA_3], C_PINK, C_SKY); pal_swap(maps[AREA_3], C_MAGENTA, C_BLUE);
        pal_swap(maps[AREA_4], C_EARTH, C_TAN); pal_swap(maps[AREA_4], C_HIDE, C_EARTH);
        pal_swap(maps[AREA_4], C_PINK, C_YELLOW); pal_swap(maps[AREA_4], C_MAGENTA, C_AMBER);
        pal_swap(maps[AREA_5], C_PINK, C_LEAF); pal_swap(maps[AREA_5], C_MAGENTA, C_JADE);
        made = true;
    }
    return maps[area];
}

static void draw_hub(void) {
    int fx = hub_px * TS, fy = hub_py * TS;
    if (hub_move_t < 6) {
        float t = hub_move_t / 6.0f;
        fx = (int)((hub_from_x + (hub_px - hub_from_x) * t) * TS);
        fy = (int)((hub_from_y + (hub_py - hub_from_y) * t) * TS);
    }
    cam_x = iclamp(fx + 8 - SCREEN_W / 2, 0, HUB_W * TS - SCREEN_W);
    cam_y = iclamp(fy + 8 - 16 - (SCREEN_H - 16) / 2, -16, HUB_H * TS - SCREEN_H);
    gfx_cls(C_NIGHT);
    gfx_camera(cam_x, cam_y);
    int x0 = imax(0, cam_x / TS), y0 = imax(0, cam_y / TS - 1);
    int x1 = imin(HUB_W - 1, (cam_x + SCREEN_W) / TS), y1 = imin(HUB_H - 1, (cam_y + SCREEN_H) / TS + 1);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) draw_hub_tile(x, y, x * TS, y * TS);
    /* doors, people and Fen, back to front */
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            int r = room_at[y][x];
            int px = x * TS, py = y * TS;
            if (r >= 0) {
                /* a little mud-brick hut with the room's number over the door */
                int wall = r == 49 ? C_LIGHT : C_EARTH;
                gfx_rect(px + 1, py - 5, 14, 19, wall);
                gfx_dither(px + 1, py - 2, 14, 16, r == 49 ? C_WHITE : C_TAN, 2);
                gfx_rect(px, py - 8, 16, 4, r == 49 ? C_AMBER : C_TAN);
                gfx_hline(px, px + 15, py - 8, r == 49 ? C_YELLOW : C_HIDE);
                gfx_vline(px, py - 8, py + 13, C_BROWN);
                gfx_vline(px + 15, py - 8, py + 13, C_BROWN);
                gfx_rect(px + 5, py + 5, 6, 9, C_INK);
                gfx_rect(px + 6, py + 4, 4, 1, C_INK);
                gfx_rect(px + 3, py - 3, 10, 7, done(r) ? C_CYAN : C_CREAM);
                gfx_rectb(px + 3, py - 3, 10, 7, C_BROWN);
                char buf[4];
                snprintf(buf, sizeof buf, "%d", r + 1);
                tiny_center(buf, px + 8, py - 2, done(r) ? C_NAVY : C_BROWN);
                if (done(r)) spr_draw(&fn_spr[FS_DROP], px + 5, py - 16 + ((frame_t / 16 + r) % 2), 0);
            }
            if (HUB[y][x] == 'N') {
                int a = area_of(x, y);
                if (a == AREA_P) spr_draw(&fn_spr[FS_HUMPH], px, py - 2, 0);
                else spr_draw_ex(&fn_spr[FS_ZIZI], px, py - 2, (frame_t / 90 + x) % 2 ? SPR_FLIPX : 0, npc_map(a), -1);
                if ((frame_t / 30) % 2) text_draw("!", px + 12, py - 12, C_YELLOW);
            }
        }
        if (y == hub_py || (hub_move_t < 6 && y == hub_from_y && hub_py < hub_from_y)) {
            if (y == (hub_move_t < 6 ? imax(hub_py, hub_from_y) : hub_py)) {
                int spr, flip = 0;
                bool step = hub_move_t < 6 && (hub_move_t / 3) % 2;
                if (hub_face == DIR_UP) spr = step ? FS_FEN_U2 : FS_FEN_U;
                else if (hub_face == DIR_DOWN) spr = step ? FS_FEN_D2 : FS_FEN_D;
                else { spr = step ? FS_FEN_S2 : FS_FEN_S; flip = hub_face == DIR_LEFT ? SPR_FLIPX : 0; }
                spr_draw(&fn_spr[spr], fx, fy - 3, flip);
            }
        }
    }
    gfx_camera(0, 0);
    /* HUD */
    gfx_rect(0, 0, SCREEN_W, 16, C_INK);
    gfx_hline(0, SCREEN_W - 1, 15, C_DUSK);
    text_draw("THE DRY OASIS", 4, 4, C_CREAM);
    spr_draw(&fn_spr[FS_DROP], SCREEN_W - 44, 5, 0);
    char buf[64];
    snprintf(buf, sizeof buf, "%d/50", drops());
    text_draw(buf, SCREEN_W - 36, 4, C_CYAN);
    int r = room_at[hub_py][hub_px];
    if (r >= 0 && hub_move_t >= 6) {
        ui_panel(60, 150, 200, 26, C_NIGHT, done(r) ? C_CYAN : C_AMBER);
        snprintf(buf, sizeof buf, "ROOM %d", r + 1);
        tiny_draw(buf, 68, 154, C_GREY);
        text_draw(FN_ROOMS_DEF[r].name, 68, 162, C_CREAM);
        if (done(r)) spr_draw(&fn_spr[FS_DROP], 236, 160, 0);
        text_draw(GLYPH_A, 244, 158, C_WHITE);
    } else if (HUB[hub_py][hub_px] == 'W' && hub_move_t >= 6) {
        ui_panel(60, 150, 200, 26, C_NIGHT, C_ORANGE);
        text_draw("THE WORKSHOP", 68, 154, C_CREAM);
        tiny_draw("BUILD AND SHARE YOUR OWN ROOMS", 68, 166, C_GREY);
        text_draw(GLYPH_A, 244, 158, C_WHITE);
    }
    if (gate_msg_t > 0) {
        ui_panel(70, 60, 180, 30, C_NIGHT, C_AMBER);
        snprintf(buf, sizeof buf, "THIS GATE OPENS AT %d DROPS", gate_msg_need);
        text_center(buf, 160, 66, C_CREAM);
        snprintf(buf, sizeof buf, "YOU HAVE %d", drops());
        tiny_center(buf, 160, 78, C_CYAN);
    }
}

static void draw_talk(void) {
    draw_hub();
    ui_panel(10, 104, 300, 70, C_NIGHT, C_AMBER);
    const char *txt = talk_area < 0
        ? "THE OASIS HAS RUN DRY! GRAND VIZIER HUMPH\nHAS PLUGGED EVERY SPRING AND PIPED THE\nWATER INTO HIS PALACE BATH.\nFEN AND ZIZI SET OUT TO UNPLUG THEM."
        : npc_text(talk_area);
    text_draw(txt, 18, 112, C_LIGHT);
    if (state_t > 15 && (state_t / 20) % 2) text_draw(GLYPH_A, 298, 162, C_WHITE);
}

/* ------------------------------------------------------------------ */
/* drawing: the workshop                                                */

static void draw_piece_icon(int piece, int px, int py) {
    char ch = PIECE_CH[piece];
    FnBlock b = {0, 0, 0, 0};
    switch (ch) {
    case '#': draw_wall_tile(px, py, 0, 0, false); return;
    case '.': draw_floor_tile(px, py, 0, 0); return;
    case 'K': draw_floor_tile(px, py, 0, 0); spr_draw(&fn_spr[FS_FEN_D], px, py, 0); return;
    case 'G': draw_floor_tile(px, py, 0, 0); spr_draw(&fn_spr[FS_SPRING_DRY], px, py, 0); return;
    case 'D': draw_floor_tile(px, py, 0, 0); gfx_rect(px + 2, py + 2, 12, 12, C_BROWN); gfx_rect(px + 4, py + 4, 8, 10, C_INK); return;
    case 'g': draw_floor_tile(px, py, 0, 0); spr_draw(&fn_spr[FS_GECKO1], px, py, 0); return;
    case 'S': b.kind = BK_STONE; break;
    case '5': b.kind = BK_MARBLE; b.n = 5; break;
    default:
        if (ch >= '1' && ch <= '4') { b.kind = BK_SAND; b.n = (uint8_t)(ch - '0'); }
        else if (ch >= 'a' && ch <= 'd') { b.kind = BK_LAPIS; b.n = (uint8_t)(ch - 'a' + 1); }
        else { b.kind = BK_BASALT; b.n = (uint8_t)(ch - 'w' + 1); }
        break;
    }
    draw_floor_tile(px, py, 0, 0);
    if (b.kind == BK_BASALT && b.n > 1) {
        /* a small preview of big basalt */
        FnBlock one = b;
        one.n = 1;
        draw_block(&one, px, py);
        char buf[4];
        snprintf(buf, sizeof buf, "%d", b.n);
        gfx_rect(px + 4, py + 4, 8, 8, C_DUSK);
        text_draw(buf, px + 6, py + 5, C_LIGHT);
    } else {
        draw_block(&b, px, py);
    }
}

static void draw_custom_grid(int slot, int ox, int oy) {
    FnRoom r;
    FnState st;
    const char *rows[FN_H];
    static char buf[FN_H][FN_W + 1];
    for (int y = 0; y < FN_H; y++) { memcpy(buf[y], sv.custom[slot][y], FN_W); buf[y][FN_W] = 0; rows[y] = buf[y]; }
    for (int y = 0; y < FN_H; y++)
        for (int x = 0; x < FN_W; x++) {
            char c = sv.custom[slot][y][x];
            if (c == '#') draw_wall_tile(ox + x * TS, oy + y * TS, x, y, y + 1 < FN_H && sv.custom[slot][y + 1][x] != '#');
            else draw_floor_tile(ox + x * TS, oy + y * TS, x, y);
        }
    /* a room that can't parse (a half-built basalt) still shows its pieces */
    if (fn_parse(rows, &r, &st) != 0 && fn_parse(rows, &r, &st) != 4) return;
    if (r.goal_x != FN_NONE) spr_draw(&fn_spr[FS_SPRING_DRY], ox + r.goal_x * TS, oy + r.goal_y * TS, 0);
    if (r.door_x != FN_NONE) { gfx_rect(ox + r.door_x * TS + 2, oy + r.door_y * TS + 2, 12, 12, C_BROWN); gfx_rect(ox + r.door_x * TS + 4, oy + r.door_y * TS + 4, 8, 10, C_INK); }
    for (int i = 0; i < st.nb; i++) draw_block(&st.b[i], ox + st.b[i].x * TS, oy + st.b[i].y * TS);
    for (int g = 0; g < st.ng; g++) spr_draw(&fn_spr[FS_GECKO1], ox + st.gx[g] * TS, oy + st.gy[g] * TS, 0);
    if (st.px != FN_NONE) spr_draw(&fn_spr[FS_FEN_D], ox + st.px * TS, oy + st.py * TS - 2, 0);
}

static void draw_slots(void) {
    gfx_cls(C_NIGHT);
    static const uint8_t grad[] = {C_CREAM, C_AMBER, C_ORANGE};
    ui_fancy_center("THE WORKSHOP", 160, 4, 2, grad, 3, C_INK, C_BROWN);
    for (int i = 0; i < FN_CUSTOM; i++) {
        int y = 28 + i * 13;
        bool s = i == slot_sel;
        if (s) gfx_rect(40, y - 2, 240, 12, C_DUSK);
        char buf[48];
        snprintf(buf, sizeof buf, "ROOM %d", 51 + i);
        text_draw(buf, 56, y, s ? C_WHITE : C_GREY);
        const char *st = !sv.custom_used[i] ? "EMPTY" : sv.custom_done[i] ? "COMPLETE" : "DRAFT";
        text_draw(st, 150, y, sv.custom_done[i] ? C_CYAN : C_SLATE);
        if (sv.custom_done[i]) spr_draw(&fn_spr[FS_DROP], 214, y + 1, 0);
        if (s) ui_cursor(44, y, frame_t);
    }
    if (slot_menu) {
        ui_panel(200, 60, 90, 50, C_NIGHT, C_AMBER);
        static const char *M[3] = {"EDIT", "PLAY", "BACK"};
        for (int i = 0; i < 3; i++) {
            text_draw(M[i], 222, 68 + i * 12, slot_menu == i + 1 ? C_WHITE : C_GREY);
            if (slot_menu == i + 1) ui_cursor(210, 68 + i * 12, frame_t);
        }
    }
    tiny_center("A: CHOOSE   B: BACK TO THE OASIS", 160, 170, C_SLATE);
}

static void draw_edit(void) {
    gfx_cls(C_NIGHT);
    int ox = (SCREEN_W - FN_W * TS) / 2, oy = 20;
    draw_custom_grid(ed_slot, ox, oy);
    int k = (frame_t / 8) % 2;
    spr_draw(&fn_spr[FS_CURSOR], ox + ed_x * TS - k, oy + ed_y * TS - k, 0);
    gfx_rectb(ox + ed_x * TS - 1 - k, oy + ed_y * TS - 1 - k, TS + 2 + 2 * k, TS + 2 + 2 * k, C_WHITE);
    gfx_rect(0, 0, SCREEN_W, 18, C_INK);
    char buf[48];
    snprintf(buf, sizeof buf, "ROOM %d", 51 + ed_slot);
    text_draw(buf, 4, 5, C_CREAM);
    draw_piece_icon(ed_piece, 120, 1);
    text_draw(PIECE_NAME[ed_piece], 140, 5, C_YELLOW);
    tiny_draw("A PLACE  B ERASE  SELECT MENU", 4, 172, C_SLATE);
    if (ed_msg_t > 0 && ed_msg) {
        ui_panel(40, 70, 240, 22, C_NIGHT, C_AMBER);
        text_center(ed_msg, 160, 77, C_CREAM);
    }
}

static void draw_editmenu(void) {
    draw_edit();
    gfx_dither(0, 18, SCREEN_W, 162, C_INK, 10);
    ui_panel(20, 22, 280, 150, C_NIGHT, C_AMBER);
    for (int i = 0; i < P_COUNT; i++) {
        int c = i % 5, r = i / 5;
        int px = 34 + c * 52, py = 30 + r * 24;
        draw_piece_icon(i, px, py);
        if (i == menu_sel) { gfx_rectb(px - 2, py - 2, 20, 20, C_WHITE); }
    }
    static const char *EXTRA[3] = {"PLAY TEST", "SAVE AND EXIT", "CLEAR ROOM"};
    for (int i = 0; i < 3; i++) {
        bool s = menu_sel == P_COUNT + i;
        text_draw(EXTRA[i], 200, 132 + i * 11, s ? C_WHITE : C_GREY);
        if (s) ui_cursor(190, 132 + i * 11, frame_t);
    }
    if (menu_sel < P_COUNT) text_draw(PIECE_NAME[menu_sel], 34, 132, C_YELLOW);
}

/* ------------------------------------------------------------------ */
/* drawing: title, clear and ending                                     */

static void ellipse(int cx, int cy, int rx, int ry, int col) {
    for (int y = -ry; y <= ry; y++) {
        float f = 1.0f - (float)(y * y) / (float)(ry * ry);
        int w = (int)(rx * sqrtf(f > 0 ? f : 0));
        gfx_hline(cx - w, cx + w, cy + y, col);
    }
}

static void draw_fountain(int cx, int cy, bool wet) {
    /* the basin */
    ellipse(cx, cy + 8, 40, 11, C_BROWN);
    ellipse(cx, cy + 6, 40, 11, C_TAN);
    ellipse(cx, cy + 6, 36, 8, C_HIDE);
    ellipse(cx, cy + 7, 32, 6, wet ? C_BLUE : C_EARTH);
    if (wet) {
        ellipse(cx, cy + 6, 26, 4, C_SKY);
        for (int k = 0; k < 4; k++) gfx_hline(cx - 20 + k * 11 + (frame_t / 8) % 4, cx - 16 + k * 11 + (frame_t / 8) % 4, cy + 7, C_ICE);
    } else {
        gfx_line(cx - 20, cy + 6, cx - 8, cy + 9, C_TAN);
        gfx_line(cx - 8, cy + 9, cx + 4, cy + 5, C_TAN);
        gfx_line(cx + 4, cy + 5, cx + 18, cy + 8, C_TAN);
    }
    /* the column and its bowl */
    gfx_rect(cx - 4, cy - 22, 8, 28, C_TAN);
    gfx_rect(cx - 4, cy - 22, 3, 28, C_HIDE);
    ellipse(cx, cy - 22, 13, 4, C_TAN);
    ellipse(cx, cy - 23, 12, 3, C_HIDE);
    ellipse(cx, cy - 23, 9, 2, wet ? C_SKY : C_EARTH);
    if (wet)
        for (int i = 0; i < 12; i++) {
            float t = ((frame_t * 2 + i * 11) % 40) / 40.0f;
            int dx = (i % 2 ? 1 : -1) * (int)(6 + t * 22);
            int dy = (int)(-26 - 12 * sinf(t * 3.14159f) + t * 34);
            gfx_rect(cx + dx, cy + dy, 2, 2, i % 3 ? C_CYAN : C_ICE);
        }
}

static void draw_title(void) {
    for (int y = 0; y < SCREEN_H; y++) gfx_hline(0, SCREEN_W - 1, y, y < 60 ? C_ORANGE : y < 100 ? C_AMBER : C_HIDE);
    gfx_dither(0, 56, SCREEN_W, 8, C_AMBER, 8);
    gfx_dither(0, 96, SCREEN_W, 8, C_HIDE, 8);
    gfx_circ(250, 40, 16, C_YELLOW);
    gfx_circ(250, 40, 12, C_CREAM);
    for (int i = 0; i < 6; i++) spr_draw(&fn_spr[FS_PALM], 10 + i * 58 - (i % 2) * 10, 100 + (i % 2) * 6, i % 2 ? SPR_FLIPX : 0);
    draw_fountain(160, 140, false);
    spr_draw_scaled(&fn_spr[(frame_t / 20) % 2 ? FS_FEN_D : FS_FEN_D2], 92, 118, 2, 0);
    spr_draw_scaled(&fn_spr[FS_ZIZI], 196, 118, 2, SPR_FLIPX);
    FnBlock b = {BK_SAND, 3, 0, 0};
    draw_block(&b, 60, 146);
    FnBlock l = {BK_LAPIS, 2, 0, 0};
    draw_block(&l, 244, 146);
    static const uint8_t grad[] = {C_WHITE, C_ICE, C_CYAN, C_SKY};
    ui_fancy_center("FENNEC", 160, 10, 3, grad, 4, C_INK, C_NAVY);
    ui_fancy_center("FOUNTAIN", 160, 36, 3, grad, 4, C_INK, C_NAVY);
    char buf[48];
    snprintf(buf, sizeof buf, "DROPS %d/50", drops());
    gfx_rect(110, 166, 100, 11, C_INK);
    tiny_center(buf, 160, 169, C_CYAN);
    if (state_t > 20 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 70, C_BROWN);
}

static void draw_clear(void) {
    if (cur_room >= 100 && testing) draw_room_screen();
    else draw_room_screen();
    int t = state_t;
    static const uint8_t grad[] = {C_WHITE, C_ICE, C_CYAN};
    if (t > 10) {
        gfx_rect(0, 64, SCREEN_W, 34, C_INK);
        ui_fancy_center(cur_room == 49 ? "THE LAST SPRING!" : "SPRING RESTORED!", 160, 70, 2, grad, 3, C_INK, C_NAVY);
    }
}

static void draw_ending(void) {
    for (int y = 0; y < SCREEN_H; y++) gfx_hline(0, SCREEN_W - 1, y, y < 80 ? C_SKY : C_HIDE);
    for (int i = 0; i < 6; i++) spr_draw(&fn_spr[FS_PALM], 10 + i * 58 - (i % 2) * 10, 70 + (i % 2) * 6, i % 2 ? SPR_FLIPX : 0);
    draw_fountain(160, 110, true);
    spr_draw_scaled(&fn_spr[FS_FEN_D], 80, 100, 2, 0);
    spr_draw_scaled(&fn_spr[FS_ZIZI], 212, 100, 2, SPR_FLIPX);
    spr_draw(&fn_spr[FS_HUMPH], 150, 130, 0);
    ui_panel(30, 10, 260, 50, C_NIGHT, C_CYAN);
    static const uint8_t grad[] = {C_WHITE, C_ICE, C_CYAN};
    ui_fancy_center("THE OASIS FLOWS!", 160, 16, 2, grad, 3, C_INK, C_NAVY);
    const char *line = drops() >= FN_ROOMS ? "EVERY SPRING RUNS. EVEN HUMPH TAKES A DIP." : "HUMPH SULKS IN A PUDDLE. SOME SPRINGS WAIT.";
    tiny_center(line, 160, 38, C_LIGHT);
    char buf[32];
    snprintf(buf, sizeof buf, "DROPS %d/50", drops());
    tiny_center(buf, 160, 48, C_CYAN);
    if (state_t > 180 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 166, C_BROWN);
}

static bool sheet_mode;

static void draw_sheet(void) {
    gfx_cls(C_HIDE);
    int x = 2;
    for (int i = 0; i < FS_SPRITE_COUNT; i++) {
        spr_draw(&fn_spr[i], x, 2, 0);
        x += fn_spr[i].w + 2;
    }
    FnBlock bs[] = {{BK_SAND, 1, 0, 0}, {BK_SAND, 4, 0, 0}, {BK_LAPIS, 2, 0, 0}, {BK_MARBLE, 5, 0, 0},
                    {BK_BASALT, 1, 0, 0}, {BK_BASALT, 2, 0, 0}, {BK_BASALT, 3, 0, 0}, {BK_STONE, 1, 0, 0}};
    x = 4;
    for (int i = 0; i < 8; i++) {
        draw_block(&bs[i], x, 30);
        x += (bs[i].kind == BK_BASALT ? bs[i].n : 1) * TS + 6;
    }
}

static void fn_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_HUB: draw_hub(); break;
    case S_TALK: draw_talk(); break;
    case S_ROOM: draw_room_screen(); break;
    case S_CLEAR: draw_clear(); break;
    case S_SLOTS: draw_slots(); break;
    case S_EDIT: draw_edit(); break;
    case S_EDITMENU: draw_editmenu(); break;
    case S_ENDING: draw_ending(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void fn_load(void) {
    fn_art_load();
    fn_audio_load();
    hub_parse();
}

static void fn_start(void) {
    load_save();
    memset(parts, 0, sizeof parts);
    state = S_TITLE;
    state_t = 0;
    testing = false;
    sheet_mode = false;
    hub_enter();
    game_set_pausable(false);
    music_play(FN_MUS_HUB);
}

static void fn_quit(void) {
    if (state == S_HUB || state == S_ROOM) {
        sv.hub_x = (uint8_t)hub_px;
        sv.hub_y = (uint8_t)hub_py;
        if (state == S_ROOM && cur_room < 100) { sv.hub_x = (uint8_t)door_x[cur_room]; sv.hub_y = (uint8_t)door_y[cur_room]; }
    }
    save_now();
}

static void fn_label(int x, int y, int w, int h, int t) {
    for (int yy = 0; yy < h; yy++) gfx_hline(x, x + w - 1, y + yy, yy < 22 ? C_ORANGE : yy < 34 ? C_AMBER : C_HIDE);
    gfx_circ(x + w - 24, y + 14, 8, C_YELLOW);
    spr_draw(&fn_spr[FS_PALM], x + 4, y + 22, 0);
    spr_draw(&fn_spr[FS_PALM], x + w - 20, y + 24, SPR_FLIPX);
    FnBlock b3 = {BK_SAND, 3, 0, 0}, b1 = {BK_SAND, 1, 0, 0}, l2 = {BK_LAPIS, 2, 0, 0};
    int push = (t / 30) % 2;
    draw_block(&b3, x + 44 + push * 2, y + 38);
    draw_block(&b1, x + 62 + push * 2, y + 38);
    draw_block(&l2, x + 96, y + 38);
    spr_draw(&fn_spr[push ? FS_FEN_PUSH : FS_FEN_S], x + 26 + push * 2, y + 36, 0);
    spr_draw(&fn_spr[FS_STONE], x + 114, y + 36, 0);
}

static int fn_query(const char *key, int *out) {
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "room")) { *out = cur_room >= 100 ? 51 + cur_room - 100 : cur_room + 1; return 1; }
    if (!strcmp(key, "drops")) { *out = drops(); return 1; }
    if (!strcmp(key, "won")) { *out = S.won; return 1; }
    if (!strcmp(key, "px")) { *out = S.px; return 1; }
    if (!strcmp(key, "py")) { *out = S.py; return 1; }
    if (!strcmp(key, "nb")) { *out = S.nb; return 1; }
    if (!strcmp(key, "undo")) { *out = undo_n; return 1; }
    if (!strcmp(key, "hub_x")) { *out = hub_px; return 1; }
    if (!strcmp(key, "hub_y")) { *out = hub_py; return 1; }
    if (!strcmp(key, "testing")) { *out = testing; return 1; }
    if (!strcmp(key, "ed_x")) { *out = ed_x; return 1; }
    if (!strcmp(key, "ed_piece")) { *out = ed_piece; return 1; }
    if (!strcmp(key, "ed_char")) { *out = sv.custom[ed_slot][ed_y][ed_x]; return 1; }
    if (!strcmp(key, "gx0")) { *out = S.ng ? S.gx[0] : -1; return 1; }
    if (!strcmp(key, "gy0")) { *out = S.ng ? S.gy[0] : -1; return 1; }
    if (!strncmp(key, "done", 4) && key[4]) { *out = done(atoi(key + 4) - 1); return 1; }
    if (!strncmp(key, "custom_done", 11) && key[11]) { *out = sv.custom_done[atoi(key + 11) % FN_CUSTOM]; return 1; }
    if (!strncmp(key, "tile", 4)) {
        /* tileXY: the block number at a tile (kind * 10 + n), 0 if none */
        int x = key[4] - '0', y = key[5] - '0';
        if (key[4] && key[5] && key[6]) { x = (key[4] - '0') * 10 + (key[5] - '0'); y = key[6] - '0'; }
        int i = fn_block_at(&S, x, y);
        *out = i < 0 ? 0 : S.b[i].kind * 10 + S.b[i].n;
        return 1;
    }
    if (!strcmp(key, "check_all")) {
        /* replay every room's stored solution */
        int ok = 0;
        for (int r = 0; r < FN_ROOMS; r++) {
            FnRoom room;
            FnState st;
            if (fn_parse(FN_ROOMS_DEF[r].rows, &room, &st) == 0 && fn_solution_check(&room, &st, FN_ROOMS_DEF[r].solution)) ok++;
            else fprintf(stderr, "room %d does not solve\n", r + 1);
        }
        *out = ok;
        return 1;
    }
    if (!strcmp(key, "gates_open")) {
        int n = 0;
        for (int g = 0; g < 5; g++) n += drops() >= GATE_NEED[g];
        *out = n;
        return 1;
    }
    return 0;
}

static int fn_cheat(const char *cmd) {
    int a, b;
    if (sscanf(cmd, "room %d", &a) == 1) { enter_room(iclamp(a, 1, FN_ROOMS) - 1); return 1; }
    if (!strcmp(cmd, "autosolve")) { autoplay = cur_room < 100 ? FN_ROOMS_DEF[cur_room].solution : NULL; return 1; }
    if (!strcmp(cmd, "solve")) {
        const char *sol = cur_room < 100 ? FN_ROOMS_DEF[cur_room].solution : "";
        for (const char *p = sol; *p; p++) {
            int d = *p == 'U' ? DIR_UP : *p == 'R' ? DIR_RIGHT : *p == 'D' ? DIR_DOWN : DIR_LEFT;
            do_step(d);
            if (S.won) { room_won(); break; }
        }
        return 1;
    }
    if (!strncmp(cmd, "moves ", 6)) {
        for (const char *p = cmd + 6; *p; p++) {
            int d = *p == 'U' ? DIR_UP : *p == 'R' ? DIR_RIGHT : *p == 'D' ? DIR_DOWN : *p == 'L' ? DIR_LEFT : -1;
            if (d < 0) continue;
            do_step(d);
            if (S.won) { room_won(); break; }
            if (S.exited) { leave_room(); break; }
        }
        return 1;
    }
    if (sscanf(cmd, "drops %d", &a) == 1) {
        sv.done_lo = sv.done_hi = 0;
        for (int i = 0; i < a && i < FN_ROOMS; i++) set_done(i);
        save_now();
        return 1;
    }
    if (sscanf(cmd, "undone %d", &a) == 1) {
        a--;
        if (a < 32) sv.done_lo &= ~(1u << a);
        else sv.done_hi &= ~(1u << (a - 32));
        return 1;
    }
    if (sscanf(cmd, "hub %d %d", &a, &b) == 2) {
        hub_px = a; hub_py = b; hub_from_x = a; hub_from_y = b; hub_move_t = 99;
        state = S_HUB;
        return 1;
    }
    if (!strncmp(cmd, "setroom ", 8) || !strncmp(cmd, "custom ", 7)) {
        /* rows separated by '/': an ad-hoc room to play, or a custom slot */
        bool custom = cmd[0] == 'c';
        int slot = 0;
        const char *p = cmd + (custom ? 7 : 8);
        if (custom) { slot = atoi(p) % FN_CUSTOM; while (*p && *p != ' ') p++; while (*p == ' ') p++; }
        char rows[FN_H][FN_W + 1];
        memset(rows, 0, sizeof rows);
        int y = 0, x = 0;
        for (; *p && y < FN_H; p++) {
            if (*p == '/') { y++; x = 0; continue; }
            if (x < FN_W) rows[y][x++] = *p;
        }
        if (custom) {
            for (int yy = 0; yy < FN_H; yy++)
                for (int xx = 0; xx < FN_W; xx++) sv.custom[slot][yy][xx] = rows[yy][xx] ? rows[yy][xx] : '#';
            sv.custom_used[slot] = 1;
            sv.custom_done[slot] = 0;
            ed_slot = slot;
            save_now();
        } else {
            memcpy(adhoc, rows, sizeof adhoc);
            testing = false;
            enter_room(99);
        }
        return 1;
    }
    if (!strcmp(cmd, "workshop")) { state = S_SLOTS; state_t = 10; slot_menu = 0; return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    return 0;
}

const GameDef GAME_FENNEC = {
    "fennec",
    "FENNEC FOUNTAIN",
    "1985",
    "PUZZLE",
    "PUSH THE WATER STONES BACK ONTO THE DRY SPRINGS. BIG NUMBERS PUSH SMALL ONES.",
    {"COMPLETE A CUSTOM ROOM", "BEAT ROOM 50", "BEAT ROOM 50 WITH ALL 50 DROPS"},
    "D-PAD\tWALK AND PUSH\n"
    GLYPH_B "\tUNDO A STEP\n"
    GLYPH_A "\tENTER A DOOR / TALK\n"
    "START\tPAUSE\n\n"
    "A BLOCK PUSHES ONLY BLOCKS WITH\n"
    "THE SAME NUMBER OR SMALLER.\n"
    "EDITOR: SELECT OPENS THE MENU.",
    C_AMBER, C_CYAN,
    fn_load, fn_start, fn_update, fn_draw, fn_quit, fn_label, fn_query, fn_cheat,
    "BLOCK KOALA", 15,
};
