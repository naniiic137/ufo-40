/* DUNE EXPRESS - desert outlaws rob the Governor's trains.
 * Cartridge 28 of UFO 40, a tribute to Rail Heist (UFO 50 #28).
 * See docs/games/28-dune-express.md. The rules live in dune_logic.c and the
 * twenty trains in dune_levels.c; this file is the menus, the drawing, saving
 * and the flow. */
#include "dune.h"

#define OY 20
enum { S_TITLE, S_MENU, S_BRIEF, S_PLAY, S_RESULT, S_ENDING, S_VS_INTRO, S_VS_TURN };

typedef struct Save {
    uint32_t magic;
    uint32_t beaten;
    uint8_t stars[DX_MISSIONS];
    uint8_t cursor, seen_intro;
    uint16_t pad;
} Save;
#define SAVE_MAGIC 0x44580001u

static Save sv;
static DxWorld W;
static int state, state_t, frame_t, cur, menu_sel;
static float cam_x;
static int cam_lock = -1; /* the headless runner can hold the camera still */
static int result_stars, result_new, alert_t[DX_MAX_ACTORS];
static int last_ev[9], last_phase;
static bool versus;
static uint32_t vs_seed;
/* the opening look along the train: 1 panning, 2 waiting for a button */
static int intro;

typedef struct Part {
    float x, y, vx, vy;
    int life, col, kind;
} Part;
static Part parts[200];

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) { parts[i] = (Part){x, y, vx, vy, life, col, kind}; return; }
}

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
    if (sv.cursor >= DX_MISSIONS) sv.cursor = 0;
}

static bool beaten(int m) { return (sv.beaten >> m) & 1; }
static int total_stars(void) {
    int n = 0;
    for (int i = 0; i < DX_MISSIONS; i++) n += (sv.stars[i] & 1) + ((sv.stars[i] >> 1) & 1) + ((sv.stars[i] >> 2) & 1);
    return n;
}
static int unlocked_upto(void) {
    int n = 0;
    while (n < DX_MISSIONS - 1 && beaten(n)) n++;
    return n;
}

static void check_goals(void) {
    if (beaten(9)) game_award(GOAL_BEACON);
    if ((sv.beaten & 0xFFFFF) == 0xFFFFF) {
        game_award(GOAL_SAUCER);
        if (total_stars() >= 40) game_award(GOAL_ALIEN);
    }
}

/* ------------------------------------------------------------------ */
/* flow                                                                 */

static void start_mission(int m) {
    versus = false;
    cur = m;
    if (dx_load(&W, &DX_MISSIONS_DEF[m], m) != 0) return;
    state = S_PLAY;
    state_t = 0;
    /* the camera looks the train over from the far end back to the outlaw;
     * a button then starts the heist and the clock */
    intro = 1;
    cam_x = (float)(W.w * DX_T - SCREEN_W);
    memset(parts, 0, sizeof parts);
    memset(last_ev, 0, sizeof last_ev);
    memset(alert_t, 0, sizeof alert_t);
    last_phase = PH_FREE;
    game_set_pausable(true);
    music_play(DX_MUS_QUIET);
}

static void start_versus(void) {
    versus = true;
    intro = 0;
    vs_seed = rng_next(&g_rng);
    dx_versus_train(&W, vs_seed);
    state = S_VS_INTRO;
    state_t = 0;
    cam_x = W.a[W.ctrl].x - 100;
    memset(parts, 0, sizeof parts);
    memset(last_ev, 0, sizeof last_ev);
    last_phase = W.phase;
    music_play(DX_MUS_TENSE);
}

static void go_menu(void) {
    state = S_MENU;
    state_t = 0;
    input_set_versus(false);
    game_set_pausable(true);
    music_play(DX_MUS_MAP);
}

static void finish(void) {
    state = S_RESULT;
    state_t = 0;
    input_set_versus(false);
    if (versus) {
        music_restart(W.result == DX_WON ? DX_MUS_WIN : DX_MUS_LOSE);
        return;
    }
    if (W.result == DX_WON) {
        result_stars = dx_stars(&W);
        result_new = result_stars & ~sv.stars[cur];
        sv.stars[cur] |= (uint8_t)result_stars;
        sv.beaten |= 1u << cur;
        if (cur + 1 < DX_MISSIONS) sv.cursor = (uint8_t)(cur + 1);
        check_goals();
        save_now();
        music_restart(DX_MUS_WIN);
    } else {
        music_restart(DX_MUS_LOSE);
    }
}

/* ------------------------------------------------------------------ */
/* playing                                                              */

static DxInput read_input(bool p2) {
    DxInput in;
    memset(&in, 0, sizeof in);
    if (p2) {
        in.left = btn2(BTN_LEFT); in.right = btn2(BTN_RIGHT); in.up = btn2(BTN_UP); in.down = btn2(BTN_DOWN);
        in.a = btn2(BTN_A); in.b = btn2(BTN_B); in.a_pressed = btnp2(BTN_A); in.b_pressed = btnp2(BTN_B);
        in.select_pressed = btnp2(BTN_SELECT);
        in.down_pressed = btnp2(BTN_DOWN) || btnp(BTN_DOWN);
        /* hot seat on one pad works too */
        in.left |= btn(BTN_LEFT); in.right |= btn(BTN_RIGHT); in.up |= btn(BTN_UP); in.down |= btn(BTN_DOWN);
        in.a |= btn(BTN_A); in.b |= btn(BTN_B); in.a_pressed |= btnp(BTN_A); in.b_pressed |= btnp(BTN_B);
        return in;
    }
    in.left = btn(BTN_LEFT); in.right = btn(BTN_RIGHT); in.up = btn(BTN_UP); in.down = btn(BTN_DOWN);
    in.a = btn(BTN_A); in.b = btn(BTN_B); in.a_pressed = btnp(BTN_A); in.b_pressed = btnp(BTN_B);
    in.select_pressed = btnp(BTN_SELECT);
    in.down_pressed = btnp(BTN_DOWN);
    return in;
}

static void sounds_and_effects(void) {
    int16_t ev[9] = {W.ev_shot, W.ev_boom, W.ev_punch, W.ev_coin, W.ev_stun, W.ev_break, W.ev_gate, W.ev_alert, W.ev_turn};
    static const char *const SND[9] = {"dx_shot", "dx_boom", "dx_punch", "dx_coin", "dx_stun", "dx_break", "dx_gate", "dx_alert", "dx_turn"};
    for (int i = 0; i < 9; i++)
        if (ev[i] != last_ev[i]) { sfx_play_name(SND[i]); last_ev[i] = ev[i]; }
    if (W.phase != last_phase) {
        if (W.phase != PH_FREE && last_phase == PH_FREE) music_play(DX_MUS_TENSE);
        if (W.phase == PH_FREE && !versus) music_play(DX_MUS_QUIET);
        last_phase = W.phase;
    }
    /* guards who just noticed something get an exclamation mark */
    for (int i = 0; i < W.na; i++) {
        static uint8_t was[DX_MAX_ACTORS];
        if (W.a[i].kind != AK_OUTLAW && W.a[i].state == LS_ALERT && was[i] != LS_ALERT) alert_t[i] = 50;
        was[i] = W.a[i].state;
        if (alert_t[i] > 0) alert_t[i]--;
    }
}

/* the camera leads the outlaw a little in the way he faces */
static void follow_camera(void) {
    const DxActor *me = &W.a[W.ctrl];
    float want = me->x + 6 - SCREEN_W / 2 + (me->face ? 30 : -30);
    cam_x += (want - cam_x) * 0.12f;
    if (cam_lock >= 0) cam_x = (float)cam_lock;
    cam_x = fclamp(cam_x, 0, (float)(W.w * DX_T - SCREEN_W));
}

static void update_play(void) {
    if (intro && !versus) {
        const DxActor *me = &W.a[W.ctrl];
        float want = fclamp(me->x + 6 - SCREEN_W / 2 + 30, 0, (float)(W.w * DX_T - SCREEN_W));
        bool any = btnp(BTN_A) || btnp(BTN_B) || btnp(BTN_LEFT) || btnp(BTN_RIGHT) || btnp(BTN_UP) || btnp(BTN_DOWN);
        if (intro == 1) {
            cam_x -= 5;
            if (cam_x <= want) { cam_x = want; intro = 2; }
            if (any && state_t > 10) { cam_x = want; intro = 0; input_consume(); }
        } else if (any) {
            intro = 0;
            input_consume();
        }
        return;
    }
    DxInput in = read_input(false), in2 = read_input(true);
    if (versus) input_set_versus(true);
    dx_step(&W, &in, versus ? &in2 : NULL);
    follow_camera();
    sounds_and_effects();
    /* dust behind the train's wheels, smoke from a blast */
    if (frame_t % 6 == 0) part_add(cam_x + 330, OY + 9 * DX_T - 2, -3.0f, -0.2f, 90, C_TAN, 1);
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        if (p->kind == 0) p->vy += 0.12f;
    }
    if (W.result && W.end_t <= 0) finish(); /* a failed heist: the fall plays out first */
}

/* ------------------------------------------------------------------ */
/* drawing the world                                                    */

static const uint8_t SKY[3][4] = {
    {C_SKY, C_CYAN, C_ICE, C_HIDE},       /* afternoon */
    {C_VIOLET, C_MAGENTA, C_ORANGE, C_AMBER}, /* sunset */
    {C_NIGHT, C_DUSK, C_PURPLE, C_NAVY},  /* night */
};

static int sky_of(int m) { return versus ? 1 : m < 7 ? 0 : m < 14 ? 1 : 2; }

static void draw_sky(void) {
    int s = sky_of(cur);
    for (int y = 0; y < SCREEN_H; y++) {
        int band = y < 50 ? 0 : y < 80 ? 1 : y < 100 ? 2 : 3;
        gfx_hline(0, SCREEN_W - 1, y, SKY[s][band]);
    }
    for (int k = 0; k < 3; k++) gfx_dither(0, 46 + k * 30, SCREEN_W, 6, SKY[s][k + 1], 8);
    if (s == 2) {
        for (int i = 0; i < 50; i++) gfx_pset((i * 67 + 13) % 320, (i * 29) % 90, (frame_t / 20 + i) % 7 ? C_GREY : C_WHITE);
        gfx_circ(250, 36, 10, C_CREAM);
    } else {
        gfx_circ(s ? 240 : 260, s ? 92 : 34, s ? 18 : 12, s ? C_YELLOW : C_WHITE);
    }
    /* dunes, far and near, slide by as the train runs */
    int far = (int)(cam_x * 0.1f + frame_t * 0.2f), near = (int)(cam_x * 0.3f + frame_t * 0.8f);
    for (int x = 0; x < SCREEN_W; x++) {
        int h1 = 100 + (int)(8 * sinf((x + far) * 0.02f) + 5 * sinf((x + far) * 0.051f));
        gfx_vline(x, h1, 140, s == 2 ? C_DUSK : s == 1 ? C_WINE : C_EARTH);
        int h2 = 122 + (int)(6 * sinf((x + near) * 0.03f) + 4 * sinf((x + near) * 0.07f));
        gfx_vline(x, h2, 180, s == 2 ? C_NIGHT : s == 1 ? C_TAN : C_HIDE);
    }
    /* telegraph poles whip past */
    int pole = (int)(cam_x * 1.2f + frame_t * 3.0f) % 160;
    for (int x = -pole; x < SCREEN_W; x += 160) {
        gfx_rect(x, 60, 3, 90, s == 2 ? C_INK : C_BROWN);
        gfx_rect(x - 6, 64, 15, 2, s == 2 ? C_INK : C_BROWN);
        gfx_line(x + 2, 66, x + 162, 70, s == 2 ? C_DUSK : C_BROWN);
    }
}

static void draw_rails(void) {
    int y = OY + 9 * DX_T;
    gfx_rect(0, y, SCREEN_W, SCREEN_H - y, sky_of(cur) == 2 ? C_INK : C_BROWN);
    int off = (int)(cam_x + frame_t * 4) % 12;
    for (int x = -off; x < SCREEN_W; x += 12) gfx_rect(x, y + 4, 6, 3, sky_of(cur) == 2 ? C_DUSK : C_TAN);
    gfx_hline(0, SCREEN_W - 1, y + 2, C_GREY);
    gfx_hline(0, SCREEN_W - 1, y + 3, C_SLATE);
}

static int car_colour(int car) {
    static const uint8_t C[6] = {C_WINE, C_FOREST, C_NAVY, C_TAN, C_TEAL, C_MAROON};
    return C[car % 6];
}

/* a wheel: rim, hub and a crank pin turning with the train */
static void draw_wheel(int x, int y) {
    float ang = frame_t * 0.35f;
    gfx_circ(x, y, 7, C_INK);
    gfx_circb(x, y, 6, C_SLATE);
    gfx_line(x, y, x + (int)lroundf(cosf(ang) * 4), y + (int)lroundf(sinf(ang) * 4), C_SLATE);
    gfx_rect(x - 1, y - 1, 3, 3, C_GREY);
    gfx_rect(x + (int)lroundf(cosf(ang) * 4) - 1, y + (int)lroundf(sinf(ang) * 4) - 1, 2, 2, C_GREY);
}

static void draw_tile(int tx, int ty, int px, int py) {
    int t = W.tile[ty][tx];
    switch (t) {
    case TL_ROOF:
        gfx_rect(px, py, DX_T, DX_T, C_MAROON);
        gfx_rect(px, py, DX_T, 4, C_WINE);
        gfx_hline(px, px + DX_T - 1, py, C_RED);
        gfx_pset(px + 3, py + 8, C_WINE);
        gfx_pset(px + 11, py + 8, C_WINE);
        break;
    case TL_WALL:
        gfx_rect(px, py, DX_T, DX_T, C_TAN);
        gfx_vline(px + 5, py, py + DX_T - 1, C_BROWN);
        gfx_vline(px + 11, py, py + DX_T - 1, C_BROWN);
        gfx_pset(px + 2, py + 3, C_EARTH);
        gfx_pset(px + 8, py + 11, C_EARTH);
        break;
    case TL_FLOOR:
        gfx_rect(px, py, DX_T, DX_T, C_BROWN);
        gfx_rect(px, py, DX_T, 3, C_TAN);
        gfx_hline(px, px + DX_T - 1, py, C_EARTH);
        gfx_vline(px + (tx % 2 ? 4 : 12), py + 3, py + DX_T - 1, C_MAROON);
        break;
    case TL_ARMOR:
        gfx_rect(px, py, DX_T, DX_T, C_SLATE);
        gfx_rectb(px, py, DX_T, DX_T, C_DUSK);
        gfx_hline(px + 1, px + DX_T - 2, py + 1, C_GREY);
        for (int k = 0; k < 4; k++) gfx_pset(px + 3 + (k % 2) * 9, py + 3 + (k / 2) * 9, C_LIGHT);
        break;
    case TL_GATE:
        for (int k = 0; k < 4; k++) gfx_rect(px + 1 + k * 4, py, 2, DX_T, C_GREY);
        gfx_hline(px, px + DX_T - 1, py + 7, C_SLATE);
        gfx_pset(px + 7, py + 7, C_YELLOW);
        break;
    case TL_GATE_OPEN:
        gfx_rect(px, py, DX_T, 2, C_SLATE);
        for (int k = 0; k < 4; k++) gfx_rect(px + 1 + k * 4, py, 2, 2, C_GREY);
        break;
    case TL_CELL:
        for (int k = 0; k < 4; k++) gfx_rect(px + 1 + k * 4, py, 2, DX_T, C_SLATE);
        break;
    case TL_LADDER:
        gfx_vline(px + 3, py, py + DX_T - 1, C_TAN);
        gfx_vline(px + 12, py, py + DX_T - 1, C_TAN);
        for (int k = 2; k < DX_T; k += 5) gfx_hline(px + 3, px + 12, py + k, C_EARTH);
        break;
    case TL_SHELF:
        gfx_rect(px, py, DX_T, 3, C_TAN);
        gfx_hline(px, px + DX_T - 1, py + 3, C_BROWN);
        break;
    case TL_WHEELS:
        gfx_rect(px, py, DX_T, 4, C_INK);
        draw_wheel(px + 8, py + 8);
        break;
    case TL_COUPLING:
        gfx_rect(px, py + 4, DX_T, 4, C_INK);
        gfx_rect(px + 4, py + 2, 8, 8, C_SLATE);
        gfx_rect(px + 6, py + 4, 4, 4, C_INK);
        break;
    default: break;
    }
    /* a plank or plate that has taken punches cracks, and flashes as it's struck */
    int d = t == TL_ARMOR ? W.dmg[ty][tx] * 3 / DX_ARMOR_HP : W.dmg[ty][tx];
    if (d > 0) {
        gfx_line(px + 3, py + 2, px + 8, py + 8, C_INK);
        gfx_line(px + 8, py + 8, px + 6, py + 13, C_INK);
        if (d > 1) { gfx_line(px + 13, py + 3, px + 9, py + 9, C_INK); gfx_line(px + 9, py + 9, px + 12, py + 14, C_INK); }
    }
    if (W.tflash[ty][tx] > 4) gfx_rect(px, py, DX_T, DX_T, C_WHITE);
    else if (W.tflash[ty][tx] > 0) gfx_dither(px, py, DX_T, DX_T, C_WHITE, 8);
}

/* the inside of a car: its back wall, with the desert passing in the windows */
static bool car_has_roof(int car) {
    for (int x = W.car_x0[car]; x <= W.car_x1[car]; x++)
        if (W.tile[3][x] == TL_ROOF || W.tile[3][x] == TL_ARMOR) return true;
    return false;
}

static void draw_car_back(int car) {
    if (!car_has_roof(car)) return;
    int x0 = W.car_x0[car] * DX_T - (int)cam_x, x1 = (W.car_x1[car] + 1) * DX_T - (int)cam_x;
    if (x1 < 0 || x0 > SCREEN_W) return;
    int y0 = OY + 3 * DX_T, y1 = OY + 8 * DX_T;
    gfx_rect(x0, y0, x1 - x0, y1 - y0, W.seen[car] ? PAL_DARKER[car_colour(car)] : car_colour(car));
    if (!W.seen[car]) {
        /* the open doorways at the ends of a car nobody has entered yet: dark */
        for (int e = 0; e < 2; e++) {
            int tx = e ? W.car_x1[car] : W.car_x0[car];
            for (int ty = 4; ty <= 6; ty++)
                if (W.tile[ty][tx] == TL_AIR) gfx_rect(tx * DX_T - (int)cam_x, OY + ty * DX_T, DX_T, DX_T, C_NIGHT);
        }
    }
    if (W.seen[car]) {
        for (int x = x0 + 12; x + 18 < x1; x += 28) {
            gfx_rect(x, OY + 4 * DX_T + 4, 18, 12, SKY[sky_of(cur)][1]);
            int d = (int)(frame_t * 2 + x) % 18;
            gfx_rect(x + (18 - d) % 18, OY + 4 * DX_T + 12, 5, 4, SKY[sky_of(cur)][3]);
            gfx_rectb(x - 1, OY + 4 * DX_T + 3, 20, 14, C_INK);
        }
    }
}

/* the fog: a car nobody has entered shows its outside */
static void draw_car_front(int car) {
    if (W.seen[car] || !car_has_roof(car)) return;
    /* the end walls and their doorways stay as they are; the fog is the inside */
    int x0 = (W.car_x0[car] + 1) * DX_T - (int)cam_x, x1 = W.car_x1[car] * DX_T - (int)cam_x;
    if (x1 < 0 || x0 > SCREEN_W) return;
    int y0 = OY + 4 * DX_T, y1 = OY + 7 * DX_T;
    int c = car_colour(car);
    gfx_rect(x0, y0, x1 - x0, y1 - y0, c);
    gfx_hline(x0, x1 - 1, y0, PAL_LIGHTER[c]);
    gfx_hline(x0, x1 - 1, y1 - 3, PAL_DARKER[c]);
    for (int x = x0 + 10; x + 16 < x1; x += 26) {
        gfx_rect(x, y0 + 8, 14, 12, C_INK);
        gfx_rect(x + 1, y0 + 9, 12, 5, C_NIGHT);
    }
    char b[8];
    snprintf(b, sizeof b, "%d", car + 1);
    tiny_draw(b, x0 + 4, y1 - 10, PAL_LIGHTER[c]);
}

static void person_colours(uint8_t *map, const DxActor *a) {
    pal_identity(map);
    int skin = C_HIDE, shirt = C_RED, sash = C_YELLOW, pants = C_BROWN, boots = C_TAN;
    if (a->kind == AK_OUTLAW) {
        if (a->who == OUT_WADE) { skin = C_EARTH; shirt = C_CREAM; sash = C_RED; pants = C_TAN; boots = C_BROWN; }
        else if (a->who == OUT_HUSH) { skin = C_TAN; shirt = C_NAVY; sash = C_BLUE; pants = C_NIGHT; boots = C_DUSK; }
        else { skin = C_HIDE; shirt = C_PURPLE; sash = C_MAGENTA; pants = C_WINE; boots = C_BROWN; }
    } else if (a->kind == AK_GUARD) {
        skin = C_HIDE; shirt = C_BLUE; sash = C_WHITE; pants = C_RED; boots = C_INK;
    } else {
        skin = C_HIDE; shirt = C_NAVY; sash = C_YELLOW; pants = C_WHITE; boots = C_INK;
    }
    pal_swap(map, C_HIDE, skin);
    pal_swap(map, C_RED, shirt);
    pal_swap(map, C_YELLOW, sash);
    pal_swap(map, C_BROWN, pants);
    pal_swap(map, C_EARTH, boots);
}

static void draw_actor(int i) {
    DxActor *a = &W.a[i];
    if (a->escaped == 1 || a->hidden) return;
    int car = dx_car_at(&W, a->x + 6);
    bool inside = a->y + 30 > 4 * DX_T && a->y < 7 * DX_T;
    if (car >= 0 && inside && !W.seen[car] && a->kind != AK_OUTLAW) return; /* hidden by the fog */
    int px = (int)a->x - 2 - (int)cam_x, py = (int)a->y - 1 + OY;
    int frame = DS_STAND;
    if (!a->alive) frame = DS_DOWN;
    else if (a->stun) frame = a->ground ? DS_DUCK : DS_JUMP;
    else if (a->climb) frame = DS_CLIMB;
    else if (a->punch_t > 6) frame = DS_PUNCH;
    else if (a->gun == -1) frame = DS_GUN;
    else if (a->duck) frame = fabsf(a->vx) > 0.1f || (a->anim / 6) % 2 ? DS_ROLL : DS_DUCK;
    else if (!a->ground) frame = DS_JUMP;
    else if (a->anim % 24 >= 12) frame = (a->anim / 12) % 4 < 2 ? DS_WALK1 : DS_WALK2;
    if (a->duck && a->anim && frame != DS_ROLL && frame != DS_DUCK) frame = DS_DUCK;
    uint8_t map[PAL_COUNT];
    person_colours(map, a);
    int flip = a->face ? 0 : SPR_FLIPX;
    if (a->escaped == 2) { /* locked in the cell */
        frame = DS_DUCK;
    }
    spr_draw_ex(&dx_body[frame], px, py, flip, map, -1);
    /* the hat */
    int hat = a->kind == AK_GUARD ? DO_HAT_G : a->kind == AK_GOVERNOR ? DO_HAT_GOV
            : a->who == OUT_WADE ? DO_HAT_W : a->who == OUT_HUSH ? DO_HAT_H : DO_HAT_P;
    int hy = py;
    if (frame == DS_DUCK || frame == DS_ROLL) hy += 14;
    if (frame != DS_ROLL && frame != DS_DOWN && frame != DS_CLIMB) spr_draw(&dx_obj[hat], px, hy, flip);
    if (frame == DS_CLIMB && a->kind == AK_OUTLAW) spr_draw(&dx_obj[hat], px, hy + 1, 0);
    /* the gun loading: a small bar over the head */
    if (a->gun > 0) {
        int need = a->quick ? DX_QUICKDRAW : DX_DRAW;
        gfx_rect(px + 1, py - 5, 14, 3, C_INK);
        gfx_rect(px + 2, py - 4, 12 * a->gun / need, 1, C_YELLOW);
    }
    if (a->stun && a->alive) {
        for (int k = 0; k < 3; k++) {
            float ang = frame_t * 0.15f + k * 2.1f;
            gfx_pset(px + 8 + (int)(cosf(ang) * 6), (frame == DS_DUCK ? py + 14 : py) + 1 + (int)(sinf(ang) * 2), C_YELLOW);
        }
    }
    if (a->kind != AK_OUTLAW && a->alive) {
        if (alert_t[i] > 0 && (alert_t[i] / 4) % 2) { gfx_rect(px + 5, py - 12, 5, 10, C_INK); text_draw("!", px + 6, py - 11, C_YELLOW); }
        if (!a->armed && a->carry < 0 && a->state == LS_ALERT) gfx_pset(px + 8, py - 2, C_ORANGE);
    }
    if (i == W.ctrl && a->alive && state == S_PLAY && (frame_t / 16) % 2) {
        gfx_line(px + 6, py - 6, px + 8, py - 4, C_WHITE);
        gfx_line(px + 10, py - 6, px + 8, py - 4, C_WHITE);
    }
}

static void draw_obj(int i) {
    DxObj *o = &W.o[i];
    if (!o->alive) return;
    int car = dx_car_at(&W, o->x + o->w / 2.0f);
    bool inside = o->y + o->h > 4 * DX_T && o->y < 7 * DX_T;
    if (car >= 0 && inside && !W.seen[car] && o->held < 0 && o->type != OB_CAMEL) return;
    int px = (int)o->x - 1 - (int)cam_x, py = (int)o->y - 2 + OY;
    int flip = o->face ? 0 : SPR_FLIPX;
    switch (o->type) {
    case OB_BARREL: spr_draw(&dx_obj[DO_BARREL], px, py, 0); break;
    case OB_CRATE: spr_draw(&dx_obj[DO_CRATE], px, py, 0); break;
    case OB_ANVIL: spr_draw(&dx_obj[DO_ANVIL], px, py, 0); break;
    case OB_DYNAMITE:
        spr_draw(&dx_obj[DO_DYNAMITE], px, py, 0);
        if (o->timer > 0 && (frame_t / 3) % 2) gfx_rect(px + 10, py, 3, 3, C_WHITE);
        break;
    case OB_GOOSE: spr_draw(&dx_obj[(o->held >= 0 || o->thrower) && (frame_t / 5) % 2 ? DO_GOOSE2 : DO_GOOSE], px, py, flip ^ SPR_FLIPX); break;
    case OB_RAM: spr_draw(&dx_obj[DO_RAM], px - 1, py, flip ^ SPR_FLIPX); break;
    case OB_GUN: {
        /* green while it guards the train, red once it's turned on the
         * lawmen, dark when its six rounds are spent */
        uint8_t map[PAL_COUNT];
        pal_identity(map);
        pal_swap(map, C_LIGHT, o->friendly ? C_RED : C_LIME);
        if (o->ammo <= 0) { pal_swap(map, C_LIGHT, C_DUSK); pal_swap(map, C_GREY, C_DUSK); }
        spr_draw_ex(&dx_obj[DO_GUN], px, py, flip, map, -1);
        if (o->burst == 0 && o->timer > 0 && o->ammo > 0 && (frame_t / 3) % 2) gfx_circ(px + (o->face ? 15 : 0), py + 4, 3, o->friendly ? C_RED : C_LIME);
        break;
    }
    case OB_LEVER: spr_draw(&dx_obj[DO_LEVER], px, py, o->face ? SPR_FLIPX : 0); break;
    case OB_LOOT: spr_draw(&dx_obj[o->marked ? DO_LOOT : DO_CRATE], px, py, 0); if (o->marked && (frame_t / 20) % 3 == 0) gfx_pset(px + 11, py + 5, C_WHITE); break;
    case OB_COIN: spr_draw(&dx_obj[DO_COIN], px, py + ((frame_t / 8 + i) % 2), 0); break;
    case OB_AMMO: spr_draw(&dx_obj[DO_AMMO], px, py, 0); break;
    case OB_POWER: spr_draw(&dx_icon[o->content ? (o->content - 1) % 8 : 0], px, py - ((frame_t / 10) % 2), 0); break;
    case OB_CAMEL: {
        /* whoever made it out sits up on Biscuit */
        int k = 0;
        for (int j = 0; j < W.na; j++) {
            const DxActor *r = &W.a[j];
            if (r->kind != AK_OUTLAW || r->escaped != 1) continue;
            uint8_t map[PAL_COUNT];
            person_colours(map, r);
            int rx = px + 4 + k * 8, ry = py - 13;
            gfx_clip(rx - 2, 0, 20, py + 9);
            spr_draw_ex(&dx_body[DS_STAND], rx, ry, 0, map, -1);
            spr_draw(&dx_obj[r->who == OUT_WADE ? DO_HAT_W : r->who == OUT_HUSH ? DO_HAT_H : DO_HAT_P], rx, ry, 0);
            gfx_noclip();
            k++;
        }
        spr_draw(&dx_obj[(frame_t / 10) % 2 ? DO_CAMEL : DO_CAMEL2], px - 2, py, 0);
        break;
    }
    default: break;
    }
}

static void draw_world(void) {
    draw_sky();
    int cx = (int)cam_x;
    int tx0 = imax(0, cx / DX_T), tx1 = imin(W.w - 1, (cx + SCREEN_W) / DX_T);
    for (int c = 0; c < W.ncars; c++) draw_car_back(c);
    draw_rails();
    for (int ty = 0; ty < DX_H; ty++)
        for (int tx = tx0; tx <= tx1; tx++) draw_tile(tx, ty, tx * DX_T - cx, OY + ty * DX_T);
    for (int i = 0; i < W.no; i++)
        if (W.o[i].held < 0) draw_obj(i);
    for (int i = 0; i < W.na; i++)
        if (W.a[i].kind != AK_OUTLAW) draw_actor(i);
    for (int i = 0; i < W.na; i++)
        if (W.a[i].kind == AK_OUTLAW) draw_actor(i);
    for (int i = 0; i < W.no; i++)
        if (W.o[i].held >= 0) draw_obj(i);
    for (int c = 0; c < W.ncars; c++) draw_car_front(c);
    for (int i = 0; i < DX_MAX_SHOTS; i++) {
        DxShot *s = &W.s[i];
        if (!s->alive) continue;
        int sx = (int)s->x - cx, sy = (int)s->y + OY;
        gfx_hline(sx - (s->vx > 0 ? 6 : 0), sx + (s->vx > 0 ? 0 : 6), sy, C_YELLOW);
        gfx_pset(sx, sy, C_WHITE);
    }
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life > 0) gfx_rect((int)p->x - cx, (int)p->y, 2, 2, p->col);
    }
}

static void draw_hud(void) {
    gfx_rect(0, 0, SCREEN_W, OY, C_INK);
    gfx_hline(0, SCREEN_W - 1, OY - 1, C_DUSK);
    char b[48];
    if (versus) {
        snprintf(b, sizeof b, "COINS %d/10", W.coins);
        text_draw(b, 4, 6, C_YELLOW);
        snprintf(b, sizeof b, "TURN %d/10", imin(10, W.turns + 1));
        text_draw(b, 86, 6, C_CREAM);
    } else {
        snprintf(b, sizeof b, "%d", cur + 1);
        gfx_rect(2, 3, 16, 13, C_WINE);
        text_center(b, 10, 6, C_WHITE);
        int secs = (W.master + 59) / 60;
        snprintf(b, sizeof b, "%d:%02d", secs / 60, secs % 60);
        text_draw(b, 22, 6, secs < 30 && (frame_t / 15) % 2 ? C_RED : C_CREAM);
        /* the objective */
        if (W.objective == OBJ_LOOT) { snprintf(b, sizeof b, "%d/%d", W.loot, W.need); spr_draw(&dx_obj[DO_COIN], 58, 7, 0); text_draw(b, 68, 6, C_YELLOW); }
        else if (W.objective == OBJ_RESCUE) text_draw(W.rescue_open ? "FREED!" : "FREE HIM", 58, 6, C_YELLOW);
        else if (W.objective == OBJ_AMMO) { snprintf(b, sizeof b, "BELT %d/6", W.a[W.ctrl].ammo); text_draw(b, 58, 6, C_YELLOW); }
        else text_draw(W.governor_dead ? "DONE" : "GOVERNOR", 58, 6, C_YELLOW);
    }
    /* whose turn */
    int x = 120;
    if (W.phase == PH_FREE) {
        text_draw("QUIET", x, 6, C_LIME);
    } else {
        bool mine = W.phase == PH_PLAYER;
        int total = mine || versus ? DX_TURN_FRAMES : DX_LAW_FRAMES;
        gfx_rect(x, 3, 64, 13, mine ? C_FOREST : C_WINE);
        gfx_rect(x + 1, 13, 62 * W.turn_t / total, 2, mine ? C_LIME : C_ORANGE);
        text_draw(mine ? (versus ? "BANDITS" : "YOUR TURN") : (versus ? "GUARDS" : "GUARDS!"), x + 3, 4, C_WHITE);
        if (mine && !versus) { snprintf(b, sizeof b, "%d", (W.turn_t + 59) / 60); text_draw(b, x + 66, 6, C_LIME); }
    }
    /* the outlaw at the controls */
    DxActor *me = &W.a[W.ctrl];
    uint8_t map[PAL_COUNT];
    person_colours(map, me);
    gfx_clip(200, 1, 16, 17);
    spr_draw_ex(&dx_body[DS_STAND], 200, 2, 0, map, -1);
    spr_draw(&dx_obj[me->who == OUT_WADE ? DO_HAT_W : me->who == OUT_HUSH ? DO_HAT_H : DO_HAT_P], 200, 2, 0);
    gfx_noclip();
    for (int k = 0; k < DX_MAX_AMMO; k++) gfx_rect(220 + k * 5, 6, 3, 7, k < me->ammo ? C_YELLOW : C_DUSK);
    int ix = 254;
    if (me->blast) { spr_draw(&dx_icon[1], ix, 6, 0); ix += 10; }
    if (me->boots) { spr_draw(&dx_icon[2], ix, 6, 0); ix += 10; }
    if (me->charm) { spr_draw(&dx_icon[3], ix, 6, 0); ix += 10; }
    if (me->fist) { spr_draw(&dx_icon[4], ix, 6, 0); ix += 10; }
    if (me->quick) { spr_draw(&dx_icon[5], ix, 6, 0); ix += 10; }
}

static void draw_play(void) {
    draw_world();
    draw_hud();
    if (W.phase == PH_LAW && (frame_t / 20) % 2 == 0) {
        gfx_rect(0, OY, SCREEN_W, 2, C_RED);
        gfx_rect(0, SCREEN_H - 2, SCREEN_W, 2, C_RED);
    }
    if (state == S_PLAY && intro && !versus) {
        const DxMission *m = &DX_MISSIONS_DEF[cur];
        int y = 150;
        gfx_rect(0, y, SCREEN_W, 14, C_INK);
        tiny_center(m->brief, 160, y + 4, C_CREAM);
        if (intro == 2 && (frame_t / 16) % 2) text_center("PRESS A BUTTON TO START", 160, 134, C_WHITE);
    }
}

/* ------------------------------------------------------------------ */
/* menus                                                                */

static const char *OUTLAW_NAME[3] = {"WADE", "HUSH", "PEARL"};

static void draw_title(void) {
    cur = 14;
    cam_x = frame_t * 0.5f;
    draw_sky();
    draw_rails();
    /* the night train: three cars and the engine, the windows lit */
    int y0 = OY + 72, yb = OY + 124, wy = OY + 136;
    for (int c = 0; c < 3; c++) {
        int x = -30 + c * 104, w = 98, col = car_colour(c + 2);
        gfx_rect(x + 2, y0 + 6, w - 4, yb - y0 - 6, col);
        gfx_rect(x, y0, w, 7, C_MAROON);
        gfx_hline(x + 1, x + w - 2, y0, PAL_LIGHTER[C_MAROON]);
        gfx_hline(x + 2, x + w - 3, yb - 16, PAL_LIGHTER[col]);
        gfx_hline(x + 2, x + w - 3, yb - 3, PAL_DARKER[col]);
        for (int k = 0; k < 4; k++) {
            int wx = x + 10 + k * 21;
            gfx_rect(wx, y0 + 12, 14, 14, C_INK);
            gfx_rect(wx + 1, y0 + 13, 12, 12, C_AMBER);
            gfx_rect(wx + 1, y0 + 13, 12, 5, C_YELLOW);
            if ((c + k) % 3 == 1) { /* a guard in the window, hat and shoulders */
                gfx_rect(wx + 5, y0 + 15, 4, 2, C_BROWN);
                gfx_rect(wx + 3, y0 + 17, 8, 1, C_BROWN);
                gfx_rect(wx + 5, y0 + 18, 4, 3, C_BROWN);
                gfx_rect(wx + 2, y0 + 21, 10, 4, C_BROWN);
            }
        }
        gfx_rect(x, yb, w, 5, C_INK);
        gfx_rect(x + w, yb + 1, 6, 3, C_INK);
        draw_wheel(x + 14, wy); draw_wheel(x + 30, wy);
        draw_wheel(x + w - 30, wy); draw_wheel(x + w - 14, wy);
    }
    /* the engine */
    int ex = 282;
    gfx_rect(ex, y0 - 4, 36, yb - y0 + 4, C_MAROON);           /* the cab */
    gfx_rect(ex - 2, y0 - 8, 40, 6, C_INK);
    gfx_rect(ex + 6, y0 + 8, 20, 16, C_INK);
    gfx_rect(ex + 7, y0 + 9, 18, 14, C_AMBER);
    gfx_rect(ex + 7, y0 + 9, 18, 5, C_YELLOW);
    gfx_rect(ex + 36, y0 + 20, 60, yb - y0 - 20, C_SLATE);   /* the boiler, off the right edge */
    for (int k = 0; k < 3; k++) gfx_vline(ex + 44 + k * 14, y0 + 20, yb - 1, C_GREY);
    gfx_rect(ex, yb, 40, 5, C_INK);
    draw_wheel(ex + 16, wy); draw_wheel(ex + 32, wy);
    /* the engine's smoke streams back and thins out */
    for (int k = 0; k < 8; k++) {
        int t = (frame_t + k * 10) % 80;
        int sx = 322 - t * 2, sy = y0 - 10 - t / 5 - (int)(2 * sinf((t + k * 9) * 0.15f));
        int r = 2 + t / 20;
        if (t < 40) gfx_circ(sx, sy, r, t < 16 ? C_GREY : C_SLATE);
        else gfx_dither_circle(sx, sy, r, C_SLATE, 8);
    }
    /* Wade on the roof */
    {
        DxActor a;
        memset(&a, 0, sizeof a);
        a.kind = AK_OUTLAW; a.who = OUT_WADE; a.face = 1;
        uint8_t map[PAL_COUNT];
        person_colours(map, &a);
        int kx = 20 + (frame_t / 2) % 220, fr = (frame_t / 8) % 2 ? DS_WALK1 : DS_WALK2;
        if ((frame_t / 2) % 220 > 200) fr = DS_STAND;
        spr_draw_ex(&dx_body[fr], kx, y0 - 31, 0, map, -1);
        spr_draw(&dx_obj[DO_HAT_W], kx, y0 - 31, 0);
    }
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_AMBER, C_ORANGE};
    ui_fancy_center("DUNE", 160, 6, 3, grad, 4, C_INK, C_MAROON);
    ui_fancy_center("EXPRESS", 160, 30, 3, grad, 4, C_INK, C_MAROON);
    tiny_center("THE GOVERNOR'S TRAINS. THE VILLAGES' MONEY.", 160, 55, C_CREAM);
    if (state_t > 20 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 166, C_WHITE);
}

/* a five-pointed star, 5 x 5 */
static void mini_star(int x, int y, int c) {
    static const char *S[5] = {"..#..", "#####", ".###.", ".#.#.", "#...#"};
    for (int yy = 0; yy < 5; yy++)
        for (int xx = 0; xx < 5; xx++)
            if (S[yy][xx] == '#') gfx_pset(x + xx, y + yy, c);
}

static void draw_menu(void) {
    cur = sv.cursor;
    cam_x = frame_t * 0.3f;
    draw_sky();
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(6, 6, 308, 168, C_NIGHT, C_AMBER);
    text_draw("THE SALT LINE", 14, 12, C_CREAM);
    char b[64];
    snprintf(b, sizeof b, GLYPH_STAR " %d/60", total_stars());
    text_draw(b, 250, 12, C_YELLOW);
    int top = unlocked_upto();
    for (int i = 0; i < DX_MISSIONS; i++) {
        int col = i / 10, row = i % 10;
        int x = 14 + col * 150, y = 28 + row * 12;
        bool open = i <= top, sel = menu_sel == i;
        if (sel) gfx_rect(x - 4, y - 2, 146, 12, C_DUSK);
        snprintf(b, sizeof b, "%2d %s", i + 1, open ? DX_MISSIONS_DEF[i].name : "...");
        text_draw(b, x, y, !open ? C_SLATE : sel ? C_WHITE : C_GREY);
        for (int s = 0; s < 3; s++) {
            bool got = (sv.stars[i] >> s) & 1;
            int c = !got ? (sel ? C_SLATE : C_DUSK) : s == 0 ? C_ICE : s == 1 ? C_RED : C_YELLOW;
            mini_star(x + 116 + s * 8, y, c);
        }
    }
    bool vs_sel = menu_sel == DX_MISSIONS;
    if (vs_sel) gfx_rect(10, 150, 146, 12, C_DUSK);
    text_draw("2P VERSUS", 14, 152, vs_sel ? C_WHITE : C_GREY);
    tiny_draw("STARS: " "ANGEL (NO KILLS)", 164, 150, C_ICE);
    tiny_draw("DEVIL (ALL GUARDS) " GLYPH_DOT " TIME", 164, 158, C_ORANGE);
}

static void update_menu(void) {
    int top = unlocked_upto();
    int n = menu_sel;
    if (btn_repeat(BTN_DOWN)) n = n == DX_MISSIONS ? 0 : n + 1;
    if (btn_repeat(BTN_UP)) n = n == 0 ? DX_MISSIONS : n - 1;
    if (btn_repeat(BTN_RIGHT) && n < 10) n += 10;
    if (btn_repeat(BTN_LEFT) && n >= 10 && n < DX_MISSIONS) n -= 10;
    if (n != DX_MISSIONS && n > top) n = n > menu_sel ? DX_MISSIONS : top;
    if (n != menu_sel) { menu_sel = n; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { state = S_TITLE; state_t = 0; sfx_play_name("ui_back"); return; }
    if (btnp(BTN_A)) {
        sfx_play_name("ui_ok");
        if (menu_sel == DX_MISSIONS) { start_versus(); return; }
        cur = menu_sel;
        sv.cursor = (uint8_t)cur;
        state = S_BRIEF;
        state_t = 0;
    }
}

/* an outlaw drawn large, from the hat down to the given row */
static void draw_outlaw_big(int who, int x, int y, int scale, int rows) {
    DxActor fake;
    memset(&fake, 0, sizeof fake);
    fake.who = (uint8_t)who;
    uint8_t map[PAL_COUNT];
    person_colours(map, &fake);
    for (int sy = 0; sy < rows && sy < 32; sy++)
        for (int sx = 0; sx < 16; sx++) {
            uint8_t p = dx_body[DS_STAND].px[sy * 16 + sx];
            if (p != TRANSPARENT) gfx_rect(x + sx * scale, y + sy * scale, scale, scale, map[p]);
        }
    const Sprite *hat = &dx_obj[who == OUT_WADE ? DO_HAT_W : who == OUT_HUSH ? DO_HAT_H : DO_HAT_P];
    for (int sy = 0; sy < hat->h; sy++)
        for (int sx = 0; sx < 16; sx++) {
            uint8_t p = hat->px[sy * 16 + sx];
            if (p != TRANSPARENT) gfx_rect(x + sx * scale, y + sy * scale, scale, scale, p);
        }
}

static void draw_brief(void) {
    const DxMission *m = &DX_MISSIONS_DEF[cur];
    cam_x = frame_t * 0.3f;
    draw_sky();
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(20, 16, 280, 148, C_NIGHT, C_AMBER);
    char b[64];
    snprintf(b, sizeof b, "MISSION %d", cur + 1);
    tiny_draw(b, 30, 24, C_AMBER);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_AMBER};
    ui_fancy_text(m->name, 28, 32, 1, grad, 3, C_INK, -1);
    for (int k = 0; k < m->outlaws; k++) {
        int x = 36 + k * 60;
        draw_outlaw_big(m->who[k], x, 54, 2, 32);
        tiny_center(OUTLAW_NAME[m->who[k]], x + 16, 122, C_CREAM);
    }
    int tx = 150;
    text_wrap(m->brief, tx, 60, 140, C_LIGHT, 9);
    snprintf(b, sizeof b, "TIME %d:%02d  " GLYPH_STAR " UNDER %d:%02d", m->seconds / 60, m->seconds % 60, m->star_seconds / 60, m->star_seconds % 60);
    tiny_draw(b, tx, 118, C_YELLOW);
    for (int s = 0; s < 3; s++) {
        bool got = (sv.stars[cur] >> s) & 1;
        static const char *const N[3] = {"ANGEL", "DEVIL", "TIME"};
        mini_star(tx + s * 46, 130, got ? (s == 0 ? C_ICE : s == 1 ? C_RED : C_YELLOW) : C_DUSK);
        tiny_draw(N[s], tx + 8 + s * 46, 130, got ? C_WHITE : C_SLATE);
    }
    if (state_t > 20 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 146, C_WHITE);
}

static const char *WHY_TEXT[] = {"", "SHOT!", "OFF THE TRAIN!", "FLATTENED!", "BLOWN SKY HIGH!", "TRAMPLED!", "THE TRAIN REACHED COPPER BEND.", "CAUGHT BY THE CRANK GUN!"};

static void draw_result(void) {
    draw_play();
    bool won = W.result == DX_WON;
    ui_panel(50, 50, 220, 84, C_NIGHT, won ? C_YELLOW : C_RED);
    static const uint8_t g1[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER}, g2[] = {C_WHITE, C_ORANGE, C_RED};
    if (versus) {
        ui_fancy_center(won ? "BANDITS WIN!" : "GUARDS WIN!", 160, 58, 2, won ? g1 : g2, won ? 4 : 3, C_INK, C_NIGHT);
        tiny_center(won ? "TEN COINS OFF THE TRAIN." : "THEY HELD OUT FOR TEN TURNS.", 160, 84, C_CREAM);
    } else if (won) {
        ui_fancy_center("CLEAN GETAWAY!", 160, 58, 2, g1, 4, C_INK, C_NIGHT);
        static const char *const N[3] = {"ANGEL", "DEVIL", "TIME"};
        for (int s = 0; s < 3; s++) {
            bool got = (result_stars >> s) & 1;
            int x = 86 + s * 52;
            text_draw(GLYPH_STAR, x, 84, got ? (s == 0 ? C_ICE : s == 1 ? C_RED : C_YELLOW) : C_DUSK);
            tiny_draw(N[s], x + 10, 86, got ? C_WHITE : C_SLATE);
            if ((result_new >> s) & 1 && (frame_t / 8) % 2) tiny_draw("NEW", x + 10, 94, C_LIME);
        }
        int secs = W.elapsed / 60;
        char b[40];
        snprintf(b, sizeof b, "%d:%02d  " GLYPH_DOT "  %d GUARDS DOWN", secs / 60, secs % 60, W.kills);
        tiny_center(b, 160, 106, C_GREY);
    } else {
        ui_fancy_center("THE HEIST FAILS", 160, 58, 2, g2, 3, C_INK, C_NIGHT);
        tiny_center(WHY_TEXT[W.why % ARRAY_LEN(WHY_TEXT)], 160, 84, C_CREAM);
        tiny_center("A: TRY AGAIN   B: THE LINE", 160, 106, C_GREY);
    }
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 118, C_WHITE);
}

static void update_result(void) {
    if (state_t < 40) return;
    if (versus) {
        if (btnp(BTN_A) || btnp(BTN_B)) go_menu();
        return;
    }
    if (W.result == DX_WON && btnp(BTN_A)) {
        if (cur == DX_MISSIONS - 1) { state = S_ENDING; state_t = 0; music_play(DX_MUS_END); }
        else { menu_sel = sv.cursor; go_menu(); }
    } else if (W.result != DX_WON) {
        if (btnp(BTN_A)) start_mission(cur);
        else if (btnp(BTN_B)) go_menu();
    }
}

static void draw_ending(void) {
    cur = 19;
    cam_x = frame_t * 0.4f;
    draw_sky();
    draw_rails();
    /* Pearl up on Biscuit, heading for the coast */
    int bob = (frame_t / 10) % 2;
    draw_outlaw_big(OUT_PEARL, 134, 84 + bob, 2, 17);
    spr_draw_scaled(&dx_obj[bob ? DO_CAMEL : DO_CAMEL2], 128, 100, 2, 0);
    ui_panel(20, 10, 280, 56, C_NIGHT, C_AMBER);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_AMBER};
    ui_fancy_center("ONE LAST RIDE", 160, 16, 2, grad, 3, C_INK, C_MAROON);
    text_center("THE BAND SPLITS THE LAST OF THE GOLD.\nPEARL AND BISCUIT RIDE FOR THE COAST.", 160, 38, C_LIGHT);
    char b[48];
    snprintf(b, sizeof b, GLYPH_STAR " %d OF 60 STARS", total_stars());
    text_center(b, 160, 168, C_YELLOW);
}

/* ------------------------------------------------------------------ */
/* update and draw                                                      */

static void de_update(void) {
    frame_t++;
    state_t++;
    switch (state) {
    case S_TITLE:
        game_set_pausable(false);
        if (btnp(BTN_B) && state_t > 10) { game_exit_to_library(); break; }
        if (state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); input_consume(); menu_sel = sv.cursor; go_menu(); }
        break;
    case S_MENU: update_menu(); break;
    case S_BRIEF:
        if (state_t > 20 && btnp(BTN_A)) { sfx_play_name("ui_ok"); start_mission(cur); }
        if (btnp(BTN_B)) go_menu();
        break;
    case S_PLAY: update_play(); break;
    case S_RESULT: update_result(); break;
    case S_ENDING:
        if (state_t > 180 && btnp(BTN_A)) { menu_sel = DX_MISSIONS - 1; go_menu(); }
        break;
    case S_VS_INTRO:
        if (state_t > 30 && btnp(BTN_A)) { state = S_PLAY; state_t = 0; game_set_pausable(true); }
        break;
    default: break;
    }
}

static void draw_vs_intro(void) {
    cam_x = W.a[W.ctrl].x - 100;
    draw_world();
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(30, 30, 260, 110, C_NIGHT, C_AMBER);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_AMBER};
    ui_fancy_center("2P VERSUS", 160, 36, 2, grad, 3, C_INK, C_MAROON);
    text_center("PLAYER 1: THE BANDITS\nSTEAL 10 COINS.\n\nPLAYER 2: THE GUARDS\nHOLD OUT FOR 10 TURNS.", 160, 60, C_LIGHT);
    tiny_center("EACH SIDE MOVES ONE OF ITS OWN A TURN.", 160, 116, C_GREY);
    if (state_t > 30 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 128, C_WHITE);
}

static bool sheet_mode;

static void draw_sheet(void) {
    gfx_cls(C_HIDE);
    for (int i = 0; i < DS_FRAMES; i++) {
        DxActor fake;
        memset(&fake, 0, sizeof fake);
        fake.kind = (uint8_t)(i % 3 == 0 ? AK_OUTLAW : i % 3 == 1 ? AK_GUARD : AK_GOVERNOR);
        fake.who = (uint8_t)(i % 3);
        uint8_t map[PAL_COUNT];
        person_colours(map, &fake);
        spr_draw_ex(&dx_body[i], 4 + i * 20, 4, 0, map, -1);
    }
    for (int i = 0; i < DO_COUNT; i++) spr_draw(&dx_obj[i], 4 + (i % 10) * 32, 50 + (i / 10) * 36, 0);
    for (int i = 0; i < 6; i++) spr_draw(&dx_icon[i], 4 + i * 12, 160, 0);
}

static void de_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_MENU: draw_menu(); break;
    case S_BRIEF: draw_brief(); break;
    case S_PLAY: draw_play(); break;
    case S_RESULT: draw_result(); break;
    case S_ENDING: draw_ending(); break;
    case S_VS_INTRO: draw_vs_intro(); break;
    default: draw_play(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void de_load(void) {
    dx_art_load();
    dx_audio_load();
}

static void de_start(void) {
    load_save();
    state = S_TITLE;
    state_t = 0;
    sheet_mode = false;
    versus = false;
    game_set_pausable(false);
    music_play(DX_MUS_TITLE);
}

static void de_quit(void) {
    input_set_versus(false);
    save_now();
}

static void de_label(int x, int y, int w, int h, int t) {
    for (int yy = 0; yy < h; yy++) gfx_hline(x, x + w - 1, y + yy, yy < 20 ? C_VIOLET : yy < 34 ? C_ORANGE : C_AMBER);
    gfx_circ(x + 110, y + 34, 12, C_YELLOW);
    for (int xx = 0; xx < w; xx++) gfx_vline(x + xx, y + 44 + (int)(3 * sinf((xx + t) * 0.05f)), y + h - 1, C_TAN);
    /* a car, an outlaw crouched on the roof, the camel keeping pace */
    gfx_rect(x + 10, y + 34, 70, 21, C_WINE);
    gfx_rect(x + 10, y + 32, 70, 3, C_MAROON);
    gfx_hline(x + 10, x + 79, y + 32, PAL_LIGHTER[C_MAROON]);
    for (int k = 0; k < 3; k++) {
        gfx_rect(x + 16 + k * 22, y + 38, 12, 9, C_INK);
        gfx_rect(x + 17 + k * 22, y + 39, 10, 4, C_AMBER);
    }
    gfx_circ(x + 22, y + 56, 4, C_INK);
    gfx_circ(x + 68, y + 56, 4, C_INK);
    DxActor fake;
    memset(&fake, 0, sizeof fake);
    fake.who = OUT_PEARL;
    uint8_t map[PAL_COUNT];
    person_colours(map, &fake);
    spr_draw_ex(&dx_body[DS_DUCK], x + 46, y + 1, 0, map, -1);
    spr_draw(&dx_obj[DO_HAT_P], x + 46, y + 15, 0);
    spr_draw(&dx_obj[(t / 10) % 2 ? DO_CAMEL : DO_CAMEL2], x + 96, y + 28, 0);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_AMBER};
    ui_fancy_text("DUNE EXPRESS", x + 4, y + 3, 1, grad, 3, C_INK, -1);
}

static int de_query(const char *key, int *out) {
    DxActor *me = &W.a[W.ctrl];
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "intro")) { *out = intro; return 1; }
    if (!strcmp(key, "cam")) { *out = (int)cam_x; return 1; }
    if (!strcmp(key, "rolling")) { *out = me->rolling; return 1; }
    if (!strcmp(key, "end_t")) { *out = W.end_t; return 1; }
    if (!strcmp(key, "gun_ammo") || !strcmp(key, "gun_friendly")) {
        for (int i = 0; i < W.no; i++)
            if (W.o[i].alive && W.o[i].type == OB_GUN) { *out = key[4] == 'a' ? W.o[i].ammo : W.o[i].friendly; return 1; }
        *out = -1;
        return 1;
    }
    if (!strcmp(key, "mission")) { *out = cur + 1; return 1; }
    if (!strcmp(key, "x")) { *out = (int)me->x; return 1; }
    if (!strcmp(key, "y")) { *out = (int)me->y; return 1; }
    if (!strcmp(key, "tx")) { *out = (int)((me->x + 6) / DX_T); return 1; }
    if (!strcmp(key, "ty")) { *out = (int)((me->y + 29) / DX_T); return 1; }
    if (!strcmp(key, "ground")) { *out = me->ground; return 1; }
    if (!strcmp(key, "duck")) { *out = me->duck; return 1; }
    if (!strcmp(key, "climb")) { *out = me->climb; return 1; }
    if (!strcmp(key, "hidden")) { *out = me->hidden; return 1; }
    if (!strcmp(key, "gun")) { *out = me->gun; return 1; }
    if (!strcmp(key, "ammo")) { *out = me->ammo; return 1; }
    if (!strcmp(key, "carry")) { *out = me->carry >= 0 ? W.o[me->carry].type : 0; return 1; }
    if (!strcmp(key, "stun")) { *out = me->stun; return 1; }
    if (!strcmp(key, "alive")) { *out = me->alive; return 1; }
    if (!strcmp(key, "ctrl")) { *out = W.ctrl; return 1; }
    if (!strcmp(key, "who")) { *out = me->who; return 1; }
    if (!strcmp(key, "phase")) { *out = W.phase; return 1; }
    if (!strcmp(key, "turn_t")) { *out = W.turn_t; return 1; }
    if (!strcmp(key, "master")) { *out = W.master; return 1; }
    if (!strcmp(key, "elapsed")) { *out = W.elapsed; return 1; }
    if (!strcmp(key, "result")) { *out = W.result; return 1; }
    if (!strcmp(key, "why")) { *out = W.why; return 1; }
    if (!strcmp(key, "loot")) { *out = W.loot; return 1; }
    if (!strcmp(key, "kills")) { *out = W.kills; return 1; }
    if (!strcmp(key, "active")) { *out = dx_active_lawmen(&W); return 1; }
    if (!strcmp(key, "coins")) { *out = W.coins; return 1; }
    if (!strcmp(key, "turns")) { *out = W.turns; return 1; }
    if (!strcmp(key, "stars")) { *out = dx_stars(&W); return 1; }
    if (!strcmp(key, "total_stars")) { *out = total_stars(); return 1; }
    if (!strcmp(key, "beaten_mask")) { *out = (int)sv.beaten; return 1; }
    if (!strcmp(key, "rescued")) { *out = W.rescue_open; return 1; }
    if (!strcmp(key, "gov")) { *out = W.governor_dead; return 1; }
    if (!strcmp(key, "boots")) { *out = me->boots; return 1; }
    if (!strcmp(key, "charm")) { *out = me->charm; return 1; }
    if (!strcmp(key, "blast")) { *out = me->blast; return 1; }
    if (!strcmp(key, "fist")) { *out = me->fist; return 1; }
    if (!strcmp(key, "quick")) { *out = me->quick; return 1; }
    if (!strcmp(key, "seen_cars")) { int n = 0; for (int c = 0; c < W.ncars; c++) n += W.seen[c]; *out = n; return 1; }
    if (!strncmp(key, "tile", 4)) {
        /* tileXXXY: the tile at column XXX, row Y */
        int v = atoi(key + 4);
        *out = dx_tile(&W, v / 10, v % 10);
        return 1;
    }
    if (!strncmp(key, "law", 3)) {
        /* lawN_state / _alive / _armed / _stun / _x / _y / _face / _sees */
        int n = atoi(key + 3), k = 0;
        for (int i = 0; i < W.na; i++) {
            if (W.a[i].kind == AK_OUTLAW) continue;
            if (k++ != n) continue;
            const char *f = strchr(key, '_');
            if (!f) return 0;
            if (!strcmp(f, "_state")) *out = W.a[i].state;
            else if (!strcmp(f, "_alive")) *out = W.a[i].alive;
            else if (!strcmp(f, "_armed")) *out = W.a[i].armed;
            else if (!strcmp(f, "_stun")) *out = W.a[i].stun;
            else if (!strcmp(f, "_x")) *out = (int)W.a[i].x;
            else if (!strcmp(f, "_y")) *out = (int)W.a[i].y;
            else if (!strcmp(f, "_face")) *out = W.a[i].face;
            else if (!strcmp(f, "_sees")) *out = dx_sees(&W, i, W.ctrl);
            else return 0;
            return 1;
        }
        *out = -1;
        return 1;
    }
    if (!strncmp(key, "otype", 5)) {
        /* otypeT_F: field F (x, y, vx, thrower, held) of the first thing of type T */
        int t = atoi(key + 5);
        const char *f = strchr(key, '_');
        for (int i = 0; i < W.no; i++)
            if (W.o[i].alive && W.o[i].type == t && f) {
                DxObj *o = &W.o[i];
                *out = !strcmp(f, "_x") ? (int)o->x : !strcmp(f, "_y") ? (int)o->y : !strcmp(f, "_vx") ? (int)(o->vx * 100)
                     : !strcmp(f, "_thrower") ? o->thrower : o->held;
                return 1;
            }
        *out = -999;
        return 1;
    }
    if (!strcmp(key, "coinx") || !strcmp(key, "coiny")) {
        for (int i = 0; i < W.no; i++)
            if (W.o[i].alive && W.o[i].type == OB_COIN) { *out = (int)(key[4] == 'x' ? W.o[i].x : W.o[i].y); return 1; }
        *out = -1;
        return 1;
    }
    if (!strncmp(key, "objs", 4)) {
        /* how many things of a type are still around */
        int t = atoi(key + 4), n = 0;
        for (int i = 0; i < W.no; i++) n += W.o[i].alive && W.o[i].type == t;
        *out = n;
        return 1;
    }
    return 0;
}

static int de_cheat(const char *cmd) {
    int a, b;
    DxActor *me = &W.a[W.ctrl];
    if (sscanf(cmd, "mission %d", &a) == 1) {
        /* straight into mission N, past its opening look along the train */
        start_mission(iclamp(a, 1, DX_MISSIONS) - 1);
        intro = 0;
        cam_x = W.a[W.ctrl].x - 100;
        return 1;
    }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) { me->x = (float)(a * DX_T + 2); me->y = (float)((b - 1) * DX_T + 2); me->vx = me->vy = 0; me->climb = 0; me->ground = 1; return 1; }
    if (sscanf(cmd, "face %d", &a) == 1) { me->face = (uint8_t)a; return 1; }
    if (sscanf(cmd, "ammo %d", &a) == 1) { me->ammo = (uint8_t)a; return 1; }
    if (sscanf(cmd, "master %d", &a) == 1) { W.master = a * 60; return 1; }
    if (sscanf(cmd, "power %d", &a) == 1) {
        if (a == PW_BLAST) me->blast = 1; else if (a == PW_BOOTS) me->boots = 1; else if (a == PW_CHARM) me->charm = 1;
        else if (a == PW_FIST) me->fist = 1; else if (a == PW_QUICK) me->quick = 1;
        return 1;
    }
    if (!strncmp(cmd, "law ", 4)) {
        /* law N X Y FACE STATE: move lawman N */
        int n, x, y, f, s;
        if (sscanf(cmd + 4, "%d %d %d %d %d", &n, &x, &y, &f, &s) == 5) {
            int k = 0;
            for (int i = 0; i < W.na; i++) {
                if (W.a[i].kind == AK_OUTLAW) continue;
                if (k++ != n) continue;
                W.a[i].x = (float)(x * DX_T + 2);
                W.a[i].y = (float)((y - 1) * DX_T + 2);
                W.a[i].face = (uint8_t)f;
                W.a[i].state = (uint8_t)s;
                W.a[i].vx = W.a[i].vy = 0;
            }
        }
        return 1;
    }
    if (!strcmp(cmd, "clear_law")) { for (int i = 0; i < W.na; i++) if (W.a[i].kind != AK_OUTLAW) W.a[i].alive = 0, W.a[i].x = -500; return 1; }
    if (!strcmp(cmd, "loot_all")) { W.loot = W.need; return 1; }
    if (!strcmp(cmd, "swap")) { dx_swap_outlaw(&W); return 1; }
    if (sscanf(cmd, "beaten %d", &a) == 1) { sv.beaten = a >= 32 ? 0xFFFFFFFFu : (1u << a) - 1; save_now(); return 1; }
    if (sscanf(cmd, "stars %d %d", &a, &b) == 2) { sv.stars[(a - 1) % DX_MISSIONS] = (uint8_t)b; save_now(); return 1; }
    if (!strcmp(cmd, "win")) { W.result = DX_WON; finish(); return 1; }
    if (!strcmp(cmd, "versus")) { start_versus(); state = S_PLAY; return 1; }
    if (sscanf(cmd, "camx %d", &a) == 1) { cam_lock = a; return 1; }
    if (!strcmp(cmd, "see_all")) { for (int c = 0; c < W.ncars; c++) W.seen[c] = 1; return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    if (!strcmp(cmd, "menu")) { go_menu(); return 1; }
    return 0;
}

const GameDef GAME_DUNE = {
    "dune",
    "DUNE EXPRESS",
    "1987",
    "STEALTH",
    "DESERT OUTLAWS ROB THE GOVERNOR'S TRAINS. MOVE WHILE THE GUARDS WAIT.",
    {"BEAT MISSION 10", "BEAT ALL 20 MISSIONS", "BEAT THE GAME WITH 40 STARS"},
    "D-PAD\tWALK, CLIMB, " GLYPH_DOWN " DUCKS\n"
    GLYPH_A "\tJUMP (HOLD: HIGHER)\n"
    GLYPH_DOWN "+" GLYPH_A "\tROLL (HOLD " GLYPH_DOWN ")\n"
    GLYPH_B "\tPUNCH / THROW\n"
    GLYPH_DOWN "+" GLYPH_B "\tPICK UP / PUT DOWN\n"
    "WALK ON\tPUSH A BOX ALONG\n"
    "BARREL\t" GLYPH_DOWN " HIDE, " GLYPH_LEFT GLYPH_RIGHT " ROLL, " GLYPH_UP " OUT\n"
    "HOLD " GLYPH_UP "\tLOAD + DRAW, " GLYPH_B " FIRES\n"
    "SELECT\tSWAP OUTLAWS (QUIET)\n"
    "START\tPAUSE",
    C_ORANGE, C_WINE,
    de_load, de_start, de_update, de_draw, de_quit, de_label, de_query, de_cheat,
    "RAIL HEIST", 28,
};
