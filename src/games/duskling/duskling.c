/* DUSKLING - a small pink creature looks for eggs, steered by two sides of
 * the pad: every way on the D-pad is left, both buttons are right.
 * Cartridge 13 of UFO 40, a tribute to Mooncat (UFO 50 #13).
 * See docs/games/13-duskling.md. */
#include "duskling.h"

#define TS DK_TS

enum { ST_TITLE, ST_HELP, ST_PLAY, ST_REBIRTH, ST_ENDING };
#define HELP_PAGES 3

typedef struct {
    uint32_t magic;
    uint8_t eggs;          /* EGG_* bits */
    uint8_t runs;
    uint8_t warps[16];     /* warps taken: room * 3 + n */
    uint8_t seen[8];       /* rooms seen */
    uint32_t best[3];      /* fastest run to each egg, in frames */
} Save;
#define SAVE_MAGIC 0x444B0001u
static Save sv;

static int state, state_t, frame_t, title_sel, help_page;
static int nplayers = 1;
static float cam_x, cam_y;
static int fade_t;               /* a short dark moment between rooms */
static int pending_room = -1, pending_arrive, pending_ev;
static float carry_vx;
static int32_t run_t;
static int deaths, run_warps;
static int egg_got, ending_page;
static bool egg_new;
static int away_t[2];
static int last_alive[2];
/* the last flight, for the tests: how high, how far, how long */
static float air_x0, air_y0, air_top;
static int air_t, last_rise, last_dx, last_air;
static bool was_air;

/* ------------------------------------------------------------------ */
/* save                                                                 */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else { memset(&sv, 0, sizeof sv); sv.magic = SAVE_MAGIC; }
}

static int bitcount(const uint8_t *b, int n) {
    int c = 0;
    for (int i = 0; i < n; i++)
        for (int k = 0; k < 8; k++) c += (b[i] >> k) & 1;
    return c;
}
static int eggs_found(void) { return (sv.eggs & 1) + ((sv.eggs >> 1) & 1) + ((sv.eggs >> 2) & 1); }

static void give_goals(void) {
    if (bitcount(sv.warps, 16) > 0) game_award(GOAL_BEACON);
    if (sv.eggs) game_award(GOAL_SAUCER);
    if (sv.eggs == 7) game_award(GOAL_ALIEN);
}

static void mark_seen(int room) {
    if (room < 0 || room >= DK_ROOMS) return;
    if (!(sv.seen[room / 8] & (1 << (room % 8)))) {
        sv.seen[room / 8] |= (uint8_t)(1 << (room % 8));
        save_now();
    }
}

/* ------------------------------------------------------------------ */
/* dust and sounds the world asks for                                   */

typedef struct { float x, y, vx, vy; int life, col; } Part;
static Part parts[160];

void dk_fx(const char *sfx, float x, float y, int col, int n) {
    if (dk_sim_quiet) return;
    if (sfx && *sfx) sfx_play_name(sfx);
    for (int k = 0; k < n; k++)
        for (int i = 0; i < ARRAY_LEN(parts); i++)
            if (parts[i].life <= 0) {
                float a = (float)(rng_range(&g_rng, 0, 628)) / 100.0f, sp = 0.4f + rng_range(&g_rng, 0, 100) / 70.0f;
                parts[i] = (Part){x, y, cosf(a) * sp, sinf(a) * sp - 0.6f, rng_range(&g_rng, 14, 30), col};
                break;
            }
}

static void parts_update(void) {
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vy += 0.06f;
    }
}

/* ------------------------------------------------------------------ */
/* rooms                                                                */

static int room_song(int room) {
    const DKRoom *R = &DK_ROOM[room];
    for (int y = 0; R->rows[y]; y++)
        if (strpbrk(R->rows[y], "WHG")) return DK_MUS_BOSS;
    switch (R->biome) {
    case BI_DUSK: return DK_MUS_DUSK;
    case BI_WOOD: return DK_MUS_WOOD;
    case BI_MERE: return DK_MUS_MERE;
    case BI_STEPS: return DK_MUS_STEPS;
    case BI_WORKS: return DK_MUS_WORKS;
    case BI_CAVES: return DK_MUS_CAVES;
    case BI_HEIGHTS: return DK_MUS_HEIGHTS;
    default: return DK_MUS_POCKET;
    }
}

static void camera_snap(void);

static void enter_room(int room, int arrive) {
    dk_room_load(room, arrive, nplayers);
    mark_seen(room);
    music_play(room_song(room));
    camera_snap();
    away_t[0] = away_t[1] = 0;
}

static void retry_room(void) {
    if (state != ST_PLAY) return;
    deaths++;
    dk_room_reset();
    camera_snap();
}

static const char *const PAUSE_ITEMS[1] = {"RETRY ROOM"};
static void pause_pick(int i) { if (i == 0) retry_room(); }

static void start_run(int players) {
    nplayers = players;
    run_t = 0;
    deaths = 0;
    run_warps = 0;
    dk_w.head_bounces = 0;
    state = ST_PLAY;
    state_t = 0;
    fade_t = 0;
    pending_room = -1;
    input_set_versus(players == 2);
    game_set_pausable(true);
    game_pause_items(1, PAUSE_ITEMS, pause_pick);
    enter_room(RM_M0, 0);
    dk_w.head_bounces = 0;
}

static void to_title(void) {
    state = ST_TITLE;
    state_t = 0;
    input_set_versus(false);
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    music_play(DK_MUS_TITLE);
}

/* ------------------------------------------------------------------ */
/* camera                                                               */

static void cam_focus(float *fx, float *fy) {
    float sx = 0, sy = 0;
    int n = 0;
    for (int i = 0; i < dk_w.nplayers; i++)
        if (dk_w.P[i].alive) { sx += dk_w.P[i].x; sy += dk_w.P[i].y; n++; }
    if (!n) { *fx = cam_x + 160; *fy = cam_y + 90; return; }
    *fx = sx / n + 3;
    *fy = sy / n + 4;
}

static void cam_clamp(float *x, float *y) {
    *x = fclamp(*x, 0, (float)imax(0, dk_room_w() * TS - SCREEN_W));
    *y = fclamp(*y, 0, (float)imax(0, dk_room_h() * TS - SCREEN_H));
}

static void camera_snap(void) {
    float fx, fy;
    cam_focus(&fx, &fy);
    cam_x = fx - 150;
    cam_y = fy - 96;
    cam_clamp(&cam_x, &cam_y);
}

static void camera_update(void) {
    float fx, fy;
    cam_focus(&fx, &fy);
    float wx = fx - 150, wy = fy - 96;
    cam_clamp(&wx, &wy);
    cam_x += (wx - cam_x) * 0.18f;
    cam_y += (wy - cam_y) * 0.12f;
    if (fabsf(wx - cam_x) < 0.3f) cam_x = wx;
    if (fabsf(wy - cam_y) < 0.3f) cam_y = wy;
}

/* two players: one left far behind is brought to the other */
static void keep_together(void) {
    if (dk_w.nplayers < 2) return;
    for (int i = 0; i < 2; i++) {
        DKPlayer *p = &dk_w.P[i], *q = &dk_w.P[1 - i];
        if (!p->alive || !q->alive) { away_t[i] = 0; continue; }
        bool off = p->x < cam_x - 12 || p->x > cam_x + SCREEN_W + 4 || p->y > cam_y + SCREEN_H + 10 || p->y < cam_y - 40;
        away_t[i] = off ? away_t[i] + 1 : 0;
        if (away_t[i] > 40 && q->ground) {
            p->x = q->x;
            p->y = q->y - 12;
            p->vx = p->vy = 0;
            away_t[i] = 0;
        }
    }
}

/* ------------------------------------------------------------------ */
/* play                                                                 */

static int egg_of_room(int room) { return room == RM_A7 ? EGG_AMBER : room == RM_R8 ? EGG_ROSE : EGG_WHITE; }

static void reach_egg(void) {
    egg_got = egg_of_room(dk_w.room);
    egg_new = !(sv.eggs & (1 << egg_got));
    sv.eggs |= (uint8_t)(1 << egg_got);
    if (sv.runs < 255) sv.runs++;
    if (!sv.best[egg_got] || (uint32_t)run_t < sv.best[egg_got]) sv.best[egg_got] = (uint32_t)run_t;
    save_now();
    give_goals();
    state = ST_ENDING;
    state_t = 0;
    ending_page = 0;
    game_set_pausable(false);
    game_pause_items(0, NULL, NULL);
    input_set_versus(false);
    music_play(DK_MUS_EGG);
}

static void on_event(int ev) {
    const DKRoom *R = &DK_ROOM[dk_w.room];
    if (ev == EV_EXIT && R->next >= 0) {
        pending_room = R->next;
        pending_arrive = 0;
        carry_vx = dk_w.P[0].vx;
        fade_t = 10;
        pending_ev = ev;
        sfx_play_name("dk_door");
    } else if (ev >= EV_WARP1 && ev <= EV_WARP3) {
        int k = ev - EV_WARP1;
        if (R->warp_room[k] < 0) return;
        int id = dk_w.room * 3 + k;
        bool first = bitcount(sv.warps, 16) == 0;
        if (!(sv.warps[id / 8] & (1 << (id % 8)))) { sv.warps[id / 8] |= (uint8_t)(1 << (id % 8)); save_now(); }
        run_warps++;
        if (first) give_goals();
        pending_room = R->warp_room[k];
        pending_arrive = R->warp_at[k] == '@' ? 1 : R->warp_at[k] == '&' ? 2 : 0;
        carry_vx = 0;
        fade_t = 40;
        pending_ev = ev;
        sfx_play_name("dk_warp");
    } else if (ev == EV_EGG) {
        reach_egg();
    } else if (ev == EV_REBIRTH) {
        state = ST_REBIRTH;
        state_t = 0;
        music_play(DK_MUS_WAKE);
    }
}

static void play_update(void) {
    frame_t++;
    if (fade_t > 0) {
        if (--fade_t == 0 && pending_room >= 0) {
            enter_room(pending_room, pending_arrive);
            /* walking through keeps its speed */
            if (pending_ev == EV_EXIT)
                for (int i = 0; i < dk_w.nplayers; i++) dk_w.P[i].vx = carry_vx;
            pending_room = -1;
        }
        return;
    }
    run_t++;
    uint32_t p1 = input_held();
    uint32_t pad1 = p1 & 0xFF, pad2 = (p1 >> BTN_P2_SHIFT) & 0xFF;
    for (int i = 0; i < 2; i++) last_alive[i] = dk_w.P[i].alive;
    int room = dk_w.room;
    int ev = dk_world_step(pad1, pad2);
    for (int i = 0; i < dk_w.nplayers; i++)
        if (last_alive[i] && !dk_w.P[i].alive) deaths++;
    {
        const DKPlayer *p = &dk_w.P[0];
        if (!p->ground && !was_air) { air_x0 = p->x; air_y0 = p->y; air_top = p->y; air_t = 0; }
        if (!p->ground) { air_top = fminf(air_top, p->y); air_t++; }
        if (p->ground && was_air) { last_rise = (int)lroundf(air_y0 - air_top); last_dx = (int)lroundf(p->x - air_x0); last_air = air_t; }
        was_air = !p->ground;
    }
    if (dk_w.room != room || dk_w.t == 0) camera_snap(); /* a restart */
    if (ev != EV_NONE) on_event(ev);
    if (state != ST_PLAY) return;
    keep_together();
    camera_update();
}

/* ------------------------------------------------------------------ */
/* update                                                               */

static void dk_update(void) {
    state_t++;
    parts_update();
    switch (state) {
    case ST_TITLE: {
        frame_t++;
        game_set_pausable(false);
        bool two_ok = plat_kind() != PLAT_VITA;
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if (btnp(BTN_SELECT)) { sfx_play_name("ui_ok"); state = ST_HELP; state_t = 0; help_page = 0; break; }
        if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) { title_sel ^= 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_A) || btnp(BTN_START)) {
            if (title_sel == 1 && !two_ok) { sfx_play_name("ui_error"); break; }
            sfx_play_name("ui_ok");
            input_consume();
            start_run(title_sel == 1 ? 2 : 1);
        }
        break;
    }
    case ST_HELP:
        frame_t++;
        if (btnp(BTN_A) || btnp(BTN_RIGHT)) {
            sfx_play_name("ui_move");
            if (++help_page >= HELP_PAGES) { state = ST_TITLE; state_t = 0; }
        }
        if (btnp(BTN_LEFT) && help_page > 0) { help_page--; sfx_play_name("ui_move"); }
        if (btnp(BTN_B) || btnp(BTN_SELECT) || btnp(BTN_START)) { sfx_play_name("ui_back"); state = ST_TITLE; state_t = 0; }
        break;
    case ST_PLAY: play_update(); break;
    case ST_REBIRTH:
        frame_t++;
        if (state_t >= 200) {
            state = ST_PLAY;
            state_t = 0;
            enter_room(RM_M1, 0);
        }
        break;
    case ST_ENDING:
        frame_t++;
        if (state_t > 200 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            if (++ending_page >= 2) to_title();
            else state_t = 120;
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */

typedef struct {
    uint8_t sky0, sky1, far, ground, ground_dk, top, top_dk, ledge, ledge_hi, thorn, water, water_hi;
} Theme;

static const Theme THEMES[BI_COUNT] = {
    /* the dusk wood: an orange evening */
    {C_ORANGE, C_WINE, C_MAROON, C_BROWN, C_EARTH, C_LEAF, C_FOREST, C_TAN, C_CREAM, C_EARTH, C_BLUE, C_SKY},
    /* the hush wood: blue night, pink mushroom caps */
    {C_NIGHT, C_DUSK, C_TEAL, C_EARTH, C_BROWN, C_JADE, C_FOREST, C_MAGENTA, C_PINK, C_LIGHT, C_BLUE, C_SKY},
    /* the sunken mere */
    {C_NAVY, C_NIGHT, C_TEAL, C_SLATE, C_NIGHT, C_LEAF, C_JADE, C_PINK, C_WHITE, C_LIGHT, C_BLUE, C_CYAN},
    /* the old steps */
    {C_PURPLE, C_DUSK, C_VIOLET, C_TAN, C_HIDE, C_CREAM, C_TAN, C_PINK, C_WHITE, C_LIGHT, C_BLUE, C_SKY},
    /* the humming works */
    {C_INK, C_NIGHT, C_SLATE, C_GREY, C_SLATE, C_LIGHT, C_GREY, C_TEAL, C_CYAN, C_YELLOW, C_NAVY, C_BLUE},
    /* the ember caves */
    {C_INK, C_MAROON, C_WINE, C_WINE, C_MAROON, C_ORANGE, C_RED, C_AMBER, C_YELLOW, C_YELLOW, C_RED, C_ORANGE},
    /* the windy heights */
    {C_BLUE, C_SKY, C_ICE, C_LIGHT, C_GREY, C_WHITE, C_ICE, C_WHITE, C_ICE, C_SLATE, C_BLUE, C_CYAN},
    /* the warp pockets */
    {C_PURPLE, C_VIOLET, C_MAGENTA, C_VIOLET, C_PURPLE, C_PINK, C_MAGENTA, C_CYAN, C_ICE, C_WHITE, C_NAVY, C_BLUE},
};

static const Theme *theme(void) { return &THEMES[DK_ROOM[dk_w.room].biome]; }

static void draw_background(int cx, int cy) {
    const Theme *T = theme();
    int bi = DK_ROOM[dk_w.room].biome;
    for (int y = 0; y < SCREEN_H; y += 6) {
        int lv = y * 16 / SCREEN_H;
        gfx_rect(0, y, SCREEN_W, 6, T->sky0);
        gfx_dither(0, y, SCREEN_W, 6, T->sky1, lv);
    }
    /* stars or motes */
    for (int i = 0; i < 40; i++) {
        int sx = (i * 97 + 13) % 640 - (cx / 6) % 640, sy = (i * 53 + 7) % 110;
        if (sx < 0) sx += 640;
        if (sx >= SCREEN_W) continue;
        int col = bi == BI_CAVES ? ((i + frame_t / 20) % 3 ? C_WINE : C_ORANGE) : bi == BI_HEIGHTS ? C_WHITE : ((i + frame_t / 30) % 5 ? C_DUSK : C_LIGHT);
        if (bi == BI_WORKS) col = (i + frame_t / 8) % 7 ? C_NIGHT : C_TEAL;
        gfx_pset(sx, sy, col);
    }
    /* far silhouettes: trees, weed, pillars, pipes, stalactites, peaks */
    int off = cx / 3;
    for (int k = -1; k < 12; k++) {
        int x = k * 36 - off % 36, seed = (k + off / 36) * 7919;
        int h = 30 + (seed >> 3) % 40;
        int base = SCREEN_H - (cy / 4) % 20;
        switch (bi) {
        case BI_DUSK: case BI_WOOD:
            gfx_rect(x + 14, base - h - 10, 4, h + 10, T->far);
            gfx_circ(x + 16, base - h - 12, 12 + seed % 5, T->far);
            break;
        case BI_MERE:
            for (int j = 0; j < h; j += 3) gfx_pset(x + 10 + (int)(sinf((j + frame_t) * 0.1f) * 2), base - j, T->far);
            gfx_dither(x, base - h / 2, 30, h / 2, T->far, 3);
            break;
        case BI_STEPS:
            gfx_rect(x + 8, base - h - 20, 12, h + 20, T->far);
            gfx_rect(x + 5, base - h - 24, 18, 4, T->far);
            break;
        case BI_WORKS:
            gfx_rect(x, base - h, 36, 3, T->far);
            gfx_rect(x + 16, base - h, 3, h, T->far);
            if ((frame_t / 10 + k) % 4 == 0) gfx_pset(x + 17, base - h - 2, C_CYAN);
            break;
        case BI_CAVES:
            for (int j = 0; j < 8; j++) gfx_hline(x + 12 + j / 2, x + 24 - j / 2, j + (seed % 12), T->far);
            break;
        case BI_HEIGHTS:
            for (int j = 0; j < h; j++) gfx_hline(x + 18 - j / 2, x + 18 + j / 2, base - h + j, T->far);
            break;
        default:
            gfx_circb(x + 18, 60 + (seed % 60), 8 + (int)(sinf((frame_t + seed) * 0.03f) * 3), T->far);
            break;
        }
    }
}

static bool solidish(int tx, int ty) {
    char c = dk_tile_raw(tx, ty);
    if (tx < 0 || tx >= dk_room_w() || ty < 0) return true;
    return c == '#' || c == 'X' || (c == '?' && dk_hidden_shown(tx, ty));
}

static void draw_tile(int tx, int ty, int sx, int sy) {
    const Theme *T = theme();
    char c = dk_tile_raw(tx, ty);
    switch (c) {
    case '#': case 'X': {
        gfx_rect(sx, sy, TS, TS, T->ground);
        int h = (tx * 31 + ty * 17) & 7;
        gfx_pset(sx + 2 + h % 5, sy + 3 + h / 2, T->ground_dk);
        gfx_pset(sx + 6, sy + 7 - h / 3, T->ground_dk);
        if (!solidish(tx, ty - 1)) {
            gfx_rect(sx, sy, TS, 3, T->top);
            gfx_hline(sx, sx + TS - 1, sy + 3, T->top_dk);
            if (h & 1) gfx_pset(sx + h, sy - 1, T->top);
        }
        if (!solidish(tx - 1, ty)) gfx_vline(sx, sy, sy + TS - 1, T->ground_dk);
        if (!solidish(tx + 1, ty)) gfx_vline(sx + TS - 1, sy, sy + TS - 1, T->ground_dk);
        break;
    }
    case '=':
        gfx_rect(sx, sy, TS, 4, T->ledge);
        gfx_hline(sx, sx + TS - 1, sy, T->ledge_hi);
        if ((tx & 1) == 0) gfx_pset(sx + 3, sy + 2, T->ledge_hi);
        gfx_dither(sx, sy + 4, TS, 2, T->ledge, 6);
        break;
    case '^':
        for (int k = 0; k < 3; k++) {
            int bx = sx + 1 + k * 3;
            gfx_line(bx, sy + TS - 1, bx + 1, sy + 4, T->thorn);
            gfx_line(bx + 2, sy + TS - 1, bx + 1, sy + 4, T->thorn);
        }
        break;
    case '~': {
        bool top = dk_tile_raw(tx, ty - 1) != '~';
        gfx_dither(sx, sy, TS, TS, T->water, 9);
        if (top) {
            int w = (int)(sinf((frame_t + tx * 6) * 0.08f) * 1.5f);
            gfx_hline(sx, sx + TS - 1, sy + 1 + w, T->water_hi);
        }
        break;
    }
    case '?':
        if (dk_hidden_shown(tx, ty)) {
            gfx_dither(sx, sy, TS, TS, T->ground, 12);
            gfx_rectb(sx, sy, TS, TS, T->top);
        }
        break;
    case '%':
        if (dk_hidden_shown(tx, ty)) {
            gfx_rect(sx + 1, sy + 1, TS - 2, TS - 2, C_MAGENTA);
            gfx_rectb(sx, sy, TS, TS, C_PINK);
            gfx_hline(sx + 2, sx + 7, sy + 3, C_PINK);
            gfx_hline(sx + 2, sx + 7, sy + 6, C_PINK);
        }
        break;
    case 'I': spr_draw(&dk_spr[dk_face_awake(tx, ty) ? S_FACE_AWAKE : S_FACE], sx, sy, 0); break;
    case 'b': spr_draw(&dk_spr[S_BELL], sx + 1, sy + 1 + ((frame_t / 40 + tx) & 1), 0); break;
    case 'o': spr_draw(&dk_spr[S_POPPY], sx + 1, sy + 1 + ((frame_t / 40 + tx) & 1), 0); break;
    case 't': spr_draw(&dk_spr[S_TENT], sx - 5, sy - 6, 0); break;
    case 'D':
        if (!dk_w.boss_down) {
            gfx_rect(sx, sy, TS, TS, C_SLATE);
            gfx_vline(sx + 2, sy, sy + TS - 1, C_GREY);
            gfx_vline(sx + 7, sy, sy + TS - 1, C_GREY);
        }
        break;
    case 'Q': {
        static const uint8_t EGGC[3][2] = {{C_WHITE, C_LIGHT}, {C_AMBER, C_ORANGE}, {C_PINK, C_MAGENTA}};
        int e = egg_of_room(dk_w.room);
        uint8_t map[PAL_COUNT];
        pal_identity(map);
        pal_swap(map, C_WHITE, EGGC[e][0]);
        pal_swap(map, C_LIGHT, EGGC[e][1]);
        gfx_rect(sx - 4, sy + 7, 18, 3, C_BROWN);
        gfx_hline(sx - 3, sx + 12, sy + 7, C_TAN);
        spr_draw_ex(&dk_spr[S_EGG], sx - 1, sy - 4 + (int)(sinf(frame_t * 0.05f) * 1.2f), 0, map, -1);
        break;
    }
    default: break;
    }
}

static void draw_level(int cx, int cy) {
    int x0 = cx / TS, y0 = cy / TS;
    /* the background layer first (flowers, faces), then ground */
    for (int ty = y0 - 1; ty <= y0 + SCREEN_H / TS + 1; ty++)
        for (int tx = x0 - 1; tx <= x0 + SCREEN_W / TS + 1; tx++) {
            char c = dk_tile_raw(tx, ty);
            if (c != '~' && dk_wet(tx, ty)) gfx_dither(tx * TS - cx, ty * TS - cy, TS, TS, theme()->water, 9);
            if (c == ' ' || c == '>' || c == 'S' || c == '@' || c == '&' || (c >= '1' && c <= '3')) continue;
            draw_tile(tx, ty, tx * TS - cx, ty * TS - cy);
        }
}

static const uint8_t *player_remap(int i) {
    static uint8_t dusk[PAL_COUNT], p2[PAL_COUNT];
    static bool ready;
    if (!ready) {
        pal_identity(dusk);
        pal_swap(dusk, C_PINK, C_AMBER);
        pal_swap(dusk, C_MAGENTA, C_ORANGE);
        pal_swap(dusk, C_WHITE, C_CREAM);
        pal_identity(p2);
        pal_swap(p2, C_PINK, C_ICE);
        pal_swap(p2, C_MAGENTA, C_SKY);
        ready = true;
    }
    if (dk_w.room == RM_M0) return dusk;
    return i == 1 ? p2 : NULL;
}

static void draw_player(int i, int cx, int cy) {
    const DKPlayer *p = &dk_w.P[i];
    if (!p->alive) return;
    if (p->stun > 0 && (frame_t / 3) % 2) return;
    int sp;
    if (p->act == ACT_POUND) sp = S_PIM_POUND;
    else if (p->act == ACT_SLIDE) sp = (p->anim / 3) % 2 ? S_PIM_ROLL1 : S_PIM_ROLL2;
    else if (p->in_water) sp = S_PIM_SWIM;
    else if (p->spun && !p->ground) sp = (p->anim / 3) % 2 ? S_PIM_ROLL1 : S_PIM_ROLL2;
    else if (!p->ground) sp = p->vy < 0 ? S_PIM_JUMP : S_PIM_FALL;
    else if (fabsf(p->vx) > 0.2f) sp = (p->anim / (p->act == ACT_SPRINT ? 3 : 6)) % 2 ? S_PIM2 : S_PIM3;
    else sp = S_PIM1;
    int x = (int)lroundf(p->x) - 1 - cx, y = (int)lroundf(p->y) - 1 - cy;
    spr_draw_ex(&dk_spr[sp], x, y, p->face < 0 ? SPR_FLIPX : 0, player_remap(i), -1);
    if (p->act == ACT_SPRINT && (frame_t & 2)) gfx_pset(x - p->face * 3 + 4, y + 8, C_LIGHT);
}

static void draw_foes(int cx, int cy) {
    for (int i = 0; i < DK_FOES; i++) {
        const DKFoe *f = &dk_w.foe[i];
        if (!f->alive) continue;
        int x = (int)lroundf(f->x) - cx, y = (int)lroundf(f->y) - cy;
        int fl = f->dir > 0 ? SPR_FLIPX : 0;
        int an = (f->t / 8) % 2;
        switch (f->kind) {
        case F_PRICKLE: spr_draw(&dk_spr[an ? S_PRICKLE1 : S_PRICKLE2], x, y, fl); break;
        case F_WASP: case F_WASPV: spr_draw(&dk_spr[(f->t / 3) % 2 ? S_WASP1 : S_WASP2], x, y, fl); break;
        case F_PUFF: spr_draw(&dk_spr[S_PUFF], x, y, 0); break;
        case F_FROG: spr_draw(&dk_spr[f->state ? S_FROG2 : S_FROG1], x, y, 0); break;
        case F_GULPER: {
            const Theme *T = theme();
            int open = f->state == 1;
            gfx_rect(x, y + (open ? 3 : 0), 20, open ? 7 : 10, C_WINE);
            gfx_rectb(x, y + (open ? 3 : 0), 20, open ? 7 : 10, C_MAROON);
            if (open) {
                gfx_rect(x + 2, y + 3, 16, 3, C_INK);
                for (int k = 0; k < 5; k++) gfx_pset(x + 3 + k * 3, y + 2, C_WHITE);
                for (int k = 0; k < 5; k++) gfx_pset(x + 3 + k * 3, y + 1, C_WHITE);
            } else {
                gfx_hline(x + 2, x + 17, y + 5, C_MAROON);
                gfx_pset(x + 4, y + 2, C_RED);
                gfx_pset(x + 15, y + 2, C_RED);
                int k = (f->t + f->phase) % 150;
                if (k > 80) gfx_hline(x + 2, x + 17, y, T->top); /* about to open */
            }
            break;
        }
        case F_EYE: {
            if (f->state == 0) {
                int ty = (int)f->y0 - cy;
                gfx_vline(x + 5, ty - 4, y, C_GREY);
            }
            spr_draw(&dk_spr[f->state == 0 ? S_EYE : S_EYE_OPEN], x, y, 0);
            break;
        }
        case F_BEETLE: spr_draw(&dk_spr[f->state ? S_BEETLE_BALL : an ? S_BEETLE1 : S_BEETLE2], x, y, fl); break;
        case F_NEWT: spr_draw(&dk_spr[f->state ? S_NEWT2 : S_NEWT1], x, y, fl); break;
        case F_CROW: spr_draw(&dk_spr[(f->t / 50) % 4 == 0 ? S_CROW2 : S_CROW1], x, y, 0); break;
        case F_FISH: spr_draw(&dk_spr[an ? S_FISH1 : S_FISH2], x, y, fl); break;
        default: break;
        }
    }
    for (int i = 0; i < DK_SHOTS; i++) {
        const DKShot *s = &dk_w.shot[i];
        if (!s->alive) continue;
        int x = (int)lroundf(s->x) - cx, y = (int)lroundf(s->y) - cy;
        switch (s->kind) {
        case SH_SPEAR: spr_draw(&dk_spr[S_SPEAR], x - 5, y - 2, s->vx > 0 ? SPR_FLIPX : 0); break;
        case SH_SPARK: spr_draw(&dk_spr[S_SPARK], x - 3, y - 3, (s->t / 3) & 1 ? SPR_FLIPX : 0); break;
        case SH_BLAST: gfx_circ(x, y, 5, C_ORANGE); gfx_circ(x, y, 3, C_YELLOW); break;
        default: spr_draw(&dk_spr[(s->t / 4) % 2 ? S_FIRE1 : S_FIRE2], x - 3, y - 3, 0); break;
        }
    }
}

static void draw_boss(int cx, int cy) {
    const DKBoss *b = &dk_w.boss;
    if (!b->alive) return;
    int x = (int)lroundf(b->x) - cx, y = (int)lroundf(b->y) - cy;
    int fl = b->dir > 0 ? SPR_FLIPX : 0;
    if (b->flash > 0 && (frame_t / 2) % 2) return;
    switch (b->kind) {
    case BOSS_WARDEN: {
        const Sprite *s = &dk_spr[(b->t / 10) % 2 ? S_WARDEN1 : S_WARDEN2];
        if (!b->solid && b->flash == 0) {
            /* fading through: every other line */
            for (int yy = 0; yy < s->h; yy += 2)
                for (int xx = 0; xx < s->w; xx++) {
                    uint8_t c = s->px[yy * s->w + (fl ? s->w - 1 - xx : xx)];
                    if (c != TRANSPARENT) gfx_pset(x + xx, y + yy, C_SLATE);
                }
        } else {
            spr_draw(s, x, y, fl);
        }
        break;
    }
    case BOSS_HERMIT:
        if (b->state == 1 || (b->state == 0 && (frame_t / 2) % 2) || (b->state == 2 && b->t < 15 && (frame_t / 2) % 2))
            spr_draw(&dk_spr[b->state == 1 && b->t > 30 && b->t < 50 ? S_HERMIT2 : S_HERMIT1], x, y, fl);
        else if (b->state == 0) gfx_circb(x + 8, y + 8, 8 - b->t / 4, C_ORANGE);
        break;
    default:
        spr_draw(&dk_spr[!b->ground ? S_BADGER_JUMP : (b->t / 12) % 2 ? S_BADGER1 : S_BADGER2], x, y, fl);
        break;
    }
    /* hit points: small pips over the arena */
    for (int k = 0; k < b->hp; k++) gfx_rect(SCREEN_W / 2 - b->hp * 4 + k * 8, 4, 6, 3, C_RED);
}

static void draw_parts(int cx, int cy) {
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        const Part *p = &parts[i];
        if (p->life <= 0 || !p->col) continue;
        gfx_pset((int)p->x - cx, (int)p->y - cy, p->col);
    }
}

static void draw_orb(int cx, int cy) {
    /* in the dusk wood a little light keeps close to the dayling */
    if (dk_w.room != RM_M0 || !dk_w.P[0].alive) return;
    int x = (int)dk_w.P[0].x - cx - 10 + (int)(sinf(frame_t * 0.05f) * 4);
    int y = (int)dk_w.P[0].y - cy - 14 + (int)(sinf(frame_t * 0.07f) * 3);
    spr_draw(&dk_spr[S_ORB], x, y, 0);
}

static void draw_play(void) {
    int cx = (int)lroundf(cam_x), cy = (int)lroundf(cam_y);
    draw_background(cx, cy);
    draw_level(cx, cy);
    draw_boss(cx, cy);
    draw_foes(cx, cy);
    draw_orb(cx, cy);
    for (int i = dk_w.nplayers - 1; i >= 0; i--) draw_player(i, cx, cy);
    draw_parts(cx, cy);
    if (fade_t > 0) {
        int lv = pending_ev == EV_EXIT ? 16 - fade_t : 16;
        if (pending_ev != EV_EXIT) {
            /* a warp: the screen swirls to violet */
            gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_VIOLET, imin(16, (40 - fade_t) / 2));
            for (int k = 0; k < 12; k++) {
                float a = k * 0.52f + fade_t * 0.2f, r = (40 - fade_t) * 3.0f;
                gfx_circ(160 + (int)(cosf(a) * r), 90 + (int)(sinf(a) * r * 0.6f), 3, C_PINK);
            }
        } else {
            gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, imin(16, lv + 6));
        }
    }
}

static void draw_egg_icons(int cx, int y) {
    static const uint8_t EGGC[3][2] = {{C_WHITE, C_LIGHT}, {C_AMBER, C_ORANGE}, {C_PINK, C_MAGENTA}};
    for (int e = 0; e < 3; e++) {
        uint8_t map[PAL_COUNT];
        pal_identity(map);
        bool got = sv.eggs & (1 << e);
        pal_swap(map, C_WHITE, got ? EGGC[e][0] : C_DUSK);
        pal_swap(map, C_LIGHT, got ? EGGC[e][1] : C_NIGHT);
        pal_swap(map, C_GREY, got ? C_GREY : C_NIGHT);
        spr_draw_ex(&dk_spr[S_EGG], cx - 24 + e * 16, y, 0, map, -1);
    }
}

static void draw_title(void) {
    gfx_cls(C_NIGHT);
    for (int y = 0; y < 180; y += 6) gfx_dither(0, y, 320, 6, C_PURPLE, y * 16 / 180);
    for (int i = 0; i < 50; i++) gfx_pset((i * 83 + 11) % 320, (i * 47) % 110, (i + frame_t / 25) % 6 ? C_DUSK : C_WHITE);
    /* a dusk wood across the bottom */
    for (int k = 0; k < 12; k++) {
        int x = k * 30 - 6, h = 26 + (k * 37) % 30;
        gfx_rect(x + 12, 150 - h, 4, h, C_TEAL);
        gfx_circ(x + 14, 150 - h, 10 + k % 3, C_TEAL);
    }
    gfx_rect(0, 150, 320, 30, C_EARTH);
    gfx_rect(0, 150, 320, 3, C_JADE);
    /* the duskling hops along; the little light drifts over it */
    int t = frame_t % 320;
    int hx = t - 10, hop = (t / 20) % 3 == 0 ? (int)(sinf((t % 20) * 0.157f) * 10) : 0;
    spr_draw(&dk_spr[hop ? S_PIM_JUMP : (t / 6) % 2 ? S_PIM2 : S_PIM3], hx, 140 - hop, 0);
    spr_draw(&dk_spr[S_ORB], 250 + (int)(sinf(frame_t * 0.03f) * 10), 70 + (int)(sinf(frame_t * 0.05f) * 6), 0);
    static const uint8_t grad[] = {C_WHITE, C_PINK, C_MAGENTA, C_VIOLET};
    ui_fancy_center("DUSKLING", 160, 18, 3, grad, 4, C_PURPLE, C_INK);
    text_center("FIND THE EGGS", 160, 48, C_PINK);
    bool two_ok = plat_kind() != PLAT_VITA;
    const char *opt[2] = {"1 PLAYER", two_ok ? "2 PLAYERS" : "2 PLAYERS " GLYPH_LOCK};
    for (int i = 0; i < 2; i++) {
        int y = 78 + i * 12;
        int col = i == title_sel ? C_WHITE : (i == 1 && !two_ok) ? C_DUSK : C_LIGHT;
        text_center(opt[i], 160, y, col);
        if (i == title_sel) ui_cursor(160 - text_width(opt[i]) / 2 - 12, y, frame_t);
    }
    draw_egg_icons(160, 104);
    char buf[48];
    int seen = bitcount(sv.seen, 6);
    snprintf(buf, sizeof buf, "ROOMS SEEN %d/%d", seen, DK_ROOMS);
    tiny_center(buf, 160, 120, C_LIGHT);
    text_center(GLYPH_A " START  " GLYPH_B " LIBRARY  SELECT HOW TO PLAY", 160, 168, C_LIGHT);
}

static void draw_help(void) {
    gfx_cls(C_NIGHT);
    for (int y = 0; y < 180; y += 6) gfx_dither(0, y, 320, 6, C_PURPLE, y * 16 / 180);
    static const uint8_t grad[] = {C_WHITE, C_PINK, C_MAGENTA, C_VIOLET};
    ui_fancy_center("HOW TO PLAY", 160, 6, 2, grad, 4, C_PURPLE, C_INK);
    static const char *const PAGE[HELP_PAGES] = {
        "THE PAD HAS TWO SIDES.\n"
        "EVERY WAY ON THE " GLYPH_DPAD " IS LEFT.\n"
        "BOTH " GLYPH_A " AND " GLYPH_B " ARE RIGHT.\n\n"
        "HOLD ONE SIDE AND TAP THE OTHER TO JUMP\n"
        "TOWARD THE SIDE YOU HELD. KEEP THE TAP\n"
        "DOWN LONGER TO JUMP HIGHER.\n"
        "TAP A SIDE, THEN PRESS BOTH AT ONCE: A LOW HOP.",
        "TAP A SIDE TWICE TO ROLL. KEEP THE SECOND\n"
        "TAP HELD AND YOU SPRINT.\n\n"
        "IN THE AIR, TAP A SIDE TWICE TO SOMERSAULT:\n"
        "IT ADDS SPEED THAT WAY, OR TAKES IT AWAY.\n\n"
        "IN THE AIR, HOLD ONE SIDE AND PRESS THE OTHER\n"
        "AGAIN TO SLAM DOWN. A SLAM BEATS FOES, BOUNCES\n"
        "YOU UP, AND DROPS THROUGH PINK LEDGES.",
        "ANY TOUCH IS THE END OF YOU, BUT EVERY ROOM\n"
        "IS A FRESH START: YOU COME BACK WHERE YOU\n"
        "CAME IN, AS OFTEN AS IT TAKES.\n\n"
        "SOME CREATURES ARE HARMLESS. SOME ARE LEDGES.\n"
        "NOT EVERYTHING IS WHERE IT SEEMS.\n\n"
        "NOTHING IS SAVED ON THE WAY. ONLY THE EGGS\n"
        "YOU FIND ARE REMEMBERED.",
    };
    ui_panel(8, 30, 304, 128, C_INK, C_PURPLE);
    text_draw(PAGE[help_page], 14, 36, C_LIGHT);
    char buf[16];
    snprintf(buf, sizeof buf, "%d/%d", help_page + 1, HELP_PAGES);
    tiny_center(buf, 160, 162, C_GREY);
    text_center(GLYPH_A " NEXT   " GLYPH_B " BACK", 160, 170, C_LIGHT);
}

static void draw_rebirth(void) {
    gfx_cls(C_INK);
    int t = state_t;
    /* the light sinks into the dark and a small pink thing opens its eyes */
    int oy = imin(90, 20 + t / 2);
    if (t < 170) spr_draw(&dk_spr[S_ORB], 156, oy - 6, 0);
    if (t > 60) gfx_dither_circle(160, 100, imin(40, (t - 60) / 2), C_PURPLE, imin(8, (t - 60) / 10));
    if (t > 110) {
        const Sprite *s = &dk_spr[t > 150 ? S_PIM1 : S_PIM_FALL];
        spr_draw(s, 156, 104, 0);
        gfx_hline(140, 180, 114, C_DUSK);
    }
    if (t > 165 && t < 180) gfx_circb(160, 100, (t - 165) * 3, C_PINK);
}

static void draw_ending(void) {
    static const char *const WHAT[3] = {
        "INSIDE THE WHITE EGG:\nA LITTLE COLD FOG AND SOME GREY MUD.\nNOTHING ELSE AT ALL.",
        "INSIDE THE AMBER EGG:\nAN OLD MOTH SHAKES OUT DUSTY WINGS.\nLONG AGO THEY CALLED IT TALLOW,\nKEEPER OF THE LAMPS.",
        "INSIDE THE ROSE EGG:\nA DENTED HELMET AND A SMALL BENT SWORD,\nTHE KIND A BRAVE WANDERER ONCE CARRIED.",
    };
    static const uint8_t EGGC[3][2] = {{C_WHITE, C_LIGHT}, {C_AMBER, C_ORANGE}, {C_PINK, C_MAGENTA}};
    gfx_cls(C_INK);
    for (int y = 0; y < 180; y += 6) gfx_dither(0, y, 320, 6, C_NIGHT, y * 12 / 180);
    int t = state_t;
    if (ending_page == 0) {
        uint8_t map[PAL_COUNT];
        pal_identity(map);
        pal_swap(map, C_WHITE, EGGC[egg_got][0]);
        pal_swap(map, C_LIGHT, EGGC[egg_got][1]);
        int shake = t > 40 && t < 100 ? ((t / 2) % 3) - 1 : 0;
        spr_draw_scaled(&dk_spr[t < 100 ? S_EGG : S_EGG_CRACK], 144 + shake, 40, 3, 0);
        if (t >= 100) {
            for (int i = 0; i < 3; i++) gfx_pset(160 + (i - 1) * 12, 40 - (t - 100) % 30, EGGC[egg_got][0]);
            if (egg_got == EGG_AMBER) spr_draw_scaled(&dk_spr[S_MOTH], 148, 20 - imin(10, (t - 100) / 6), 2, 0);
            if (egg_got == EGG_ROSE) spr_draw_scaled(&dk_spr[S_HELM], 150, 26, 2, 0);
            if (egg_got == EGG_WHITE) gfx_dither_circle(160, 40, imin(30, (t - 100) / 2), C_GREY, 4);
        }
        spr_draw(&dk_spr[S_PIM1], 120, 78, 0);
        if (nplayers == 2) spr_draw_ex(&dk_spr[S_PIM1], 196, 78, SPR_FLIPX, player_remap(1), -1);
        gfx_hline(100, 220, 88, C_DUSK);
        if (t > 120) text_wrap(WHAT[egg_got], 30, 100, 260, C_LIGHT, 10);
        if (t > 200) text_center(GLYPH_A, 160, 168, (t / 20) % 2 ? C_WHITE : C_GREY);
        /* the two of you bounced on each other's heads three times */
        if (nplayers == 2 && dk_w.head_bounces >= 3 && t > 160) tiny_center("COMFY SOCKS, ALWAYS", 160, 150, C_DUSK);
    } else {
        static const uint8_t grad[] = {C_WHITE, C_PINK, C_MAGENTA, C_VIOLET};
        ui_fancy_center("THE END", 160, 22, 2, grad, 4, C_PURPLE, C_INK);
        char buf[64];
        snprintf(buf, sizeof buf, "EGGS FOUND %d OF 3", eggs_found());
        text_center(buf, 160, 60, egg_new ? C_YELLOW : C_LIGHT);
        draw_egg_icons(160, 74);
        int s = run_t / 60;
        snprintf(buf, sizeof buf, "TIME %d:%02d   FALLS %d   WARPS %d", s / 60, s % 60, deaths, run_warps);
        text_center(buf, 160, 100, C_LIGHT);
        if (eggs_found() < 3) text_center("OTHER EGGS ARE STILL OUT THERE.", 160, 122, C_DUSK);
        else text_center("EVERY EGG IS FOUND.", 160, 122, C_PINK);
        tiny_center("DUSKLING  " GLYPH_DOT "  BEAMDOWN SOFTWORKS 1985", 160, 150, C_GREY);
        if (t > 200) text_center(GLYPH_A, 160, 168, (t / 20) % 2 ? C_WHITE : C_GREY);
    }
}

static void dk_draw(void) {
    gfx_noclip();
    gfx_camera(0, 0);
    switch (state) {
    case ST_TITLE: draw_title(); break;
    case ST_HELP: draw_help(); break;
    case ST_PLAY: draw_play(); break;
    case ST_REBIRTH: draw_rebirth(); break;
    case ST_ENDING: draw_ending(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void dk_load(void) {
    dk_art_load();
    dk_audio_load();
}

static void dk_start(void) {
    dk_nerf = 0;
    load_save();
    give_goals();
    title_sel = 0;
    memset(parts, 0, sizeof parts);
    dk_room_load(RM_M1, 0, 1);
    to_title();
}

static void dk_quit(void) {
    input_set_versus(false);
    save_now();
}

static void dk_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_NIGHT);
    for (int yy = 0; yy < h; yy += 4) gfx_dither(x, y + yy, w, 4, C_PURPLE, yy * 16 / h);
    for (int k = 0; k < 5; k++) {
        int tx = x + 8 + k * 28;
        gfx_rect(tx + 8, y + h - 26, 3, 16, C_TEAL);
        gfx_circ(tx + 9, y + h - 26, 7, C_TEAL);
    }
    gfx_rect(x, y + h - 10, w, 10, C_EARTH);
    gfx_hline(x, x + w - 1, y + h - 10, C_JADE);
    gfx_rect(x + 60, y + h - 26, 20, 4, C_MAGENTA);
    gfx_hline(x + 60, x + 79, y + h - 26, C_PINK);
    int k = t % 80;
    int hop = k < 20 ? (int)(sinf(k * 0.157f) * 12) : 0;
    spr_draw(&dk_spr[hop ? S_PIM_JUMP : S_PIM1], x + 40 + k / 4, y + h - 20 - hop, 0);
    spr_draw(&dk_spr[S_ORB], x + w - 30, y + 8 + (int)(sinf(t * 0.06f) * 3), 0);
    spr_draw(&dk_spr[S_BELL], x + 100, y + h - 19, 0);
    spr_draw(&dk_spr[S_EGG], x + 118, y + h - 22, 0);
}

static int count_mut(void) {
    int n = 0;
    for (int i = 0; i < DK_MUT; i++) n += dk_w.mut[i];
    return n;
}

static int dk_query(const char *key, int *out) {
    const DKPlayer *p = &dk_w.P[0];
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "room")) { *out = state == ST_PLAY && fade_t > 0 && pending_room >= 0 ? pending_room : dk_w.room; return 1; }
    if (!strcmp(key, "room_now")) { *out = dk_w.room; return 1; }
    if (!strcmp(key, "fading")) { *out = fade_t; return 1; }
    if (!strcmp(key, "px")) { *out = (int)lroundf(p->x); return 1; }
    if (!strcmp(key, "py")) { *out = (int)lroundf(p->y); return 1; }
    if (!strcmp(key, "vx10")) { *out = (int)lroundf(p->vx * 10); return 1; }
    if (!strcmp(key, "vy10")) { *out = (int)lroundf(p->vy * 10); return 1; }
    if (!strcmp(key, "ground")) { *out = p->ground; return 1; }
    if (!strcmp(key, "act")) { *out = p->act; return 1; }
    if (!strcmp(key, "face")) { *out = p->face; return 1; }
    if (!strcmp(key, "alive")) { *out = p->alive; return 1; }
    if (!strcmp(key, "spun")) { *out = p->spun; return 1; }
    if (!strcmp(key, "water")) { *out = p->in_water; return 1; }
    if (!strcmp(key, "p2_alive")) { *out = dk_w.P[1].alive; return 1; }
    if (!strcmp(key, "p2_stun")) { *out = dk_w.P[1].stun; return 1; }
    if (!strcmp(key, "p2x")) { *out = (int)lroundf(dk_w.P[1].x); return 1; }
    if (!strcmp(key, "players")) { *out = dk_w.nplayers; return 1; }
    if (!strcmp(key, "head_bounces")) { *out = dk_w.head_bounces; return 1; }
    if (!strcmp(key, "deaths")) { *out = deaths; return 1; }
    if (!strcmp(key, "t")) { *out = dk_w.t; return 1; }
    if (!strcmp(key, "eggs")) { *out = sv.eggs; return 1; }
    if (!strcmp(key, "egg_got")) { *out = egg_got; return 1; }
    if (!strcmp(key, "warps")) { *out = bitcount(sv.warps, 16); return 1; }
    if (!strcmp(key, "seen")) { *out = bitcount(sv.seen, 6); return 1; }
    if (!strcmp(key, "runs")) { *out = sv.runs; return 1; }
    if (!strcmp(key, "shown")) { *out = count_mut(); return 1; }
    if (!strcmp(key, "foes")) { *out = dk_foe_count(-1); return 1; }
    if (!strcmp(key, "boss_hp")) { *out = dk_w.boss.alive ? dk_w.boss.hp : 0; return 1; }
    if (!strcmp(key, "boss_down")) { *out = dk_w.boss_down; return 1; }
    if (!strcmp(key, "boss_solid")) { *out = dk_w.boss.solid; return 1; }
    if (!strcmp(key, "boss_x")) { *out = (int)lroundf(dk_w.boss.x); return 1; }
    if (!strcmp(key, "boss_y")) { *out = (int)lroundf(dk_w.boss.y); return 1; }
    if (!strcmp(key, "shots")) { int n = 0; for (int i = 0; i < DK_SHOTS; i++) n += dk_w.shot[i].alive; *out = n; return 1; }
    if (!strcmp(key, "socks")) { *out = state == ST_ENDING && nplayers == 2 && dk_w.head_bounces >= 3; return 1; }
    if (!strcmp(key, "p2y")) { *out = (int)lroundf(dk_w.P[1].y); return 1; }
    if (!strcmp(key, "run_t")) { *out = run_t; return 1; }
    if (!strcmp(key, "solve_ok")) { *out = dk_solve_ok; return 1; }
    if (!strcmp(key, "last_rise")) { *out = last_rise; return 1; }
    if (!strcmp(key, "last_dx")) { *out = last_dx; return 1; }
    if (!strcmp(key, "last_air")) { *out = last_air; return 1; }
    if (!strcmp(key, "cam_x")) { *out = (int)cam_x; return 1; }
    if (!strcmp(key, "cam_y")) { *out = (int)cam_y; return 1; }
    if (!strncmp(key, "count_", 6)) {
        static const char *N[F_KINDS] = {"prickle", "wasp", "waspv", "puff", "frog", "gulper", "eye", "beetle", "newt", "crow", "fish"};
        for (int k = 0; k < F_KINDS; k++)
            if (!strcmp(key + 6, N[k])) { *out = dk_foe_count(k); return 1; }
        return 0;
    }
    if (!strncmp(key, "foe_", 4)) {
        /* foe_KIND_x / _y / _state: the first live foe of that kind */
        static const char *N[F_KINDS] = {"prickle", "wasp", "waspv", "puff", "frog", "gulper", "eye", "beetle", "newt", "crow", "fish"};
        const char *u = strrchr(key, '_');
        if (!u || u == key + 3) return 0;
        char kn[16];
        snprintf(kn, sizeof kn, "%.*s", (int)(u - key - 4), key + 4);
        for (int k = 0; k < F_KINDS; k++)
            if (!strcmp(kn, N[k])) {
                *out = -999;
                for (int i = 0; i < DK_FOES; i++) {
                    const DKFoe *f = &dk_w.foe[i];
                    if (!f->alive || f->kind != k) continue;
                    *out = u[1] == 'x' ? (int)lroundf(f->x) : u[1] == 'y' ? (int)lroundf(f->y) : f->state;
                    break;
                }
                return 1;
            }
        return 0;
    }
    if (!strncmp(key, "tile_", 5)) {
        int x = atoi(key + 5);
        const char *u = strchr(key + 5, '_');
        if (!u) return 0;
        int y = atoi(u + 1);
        *out = dk_solid_at(x, y) ? 1 : 0;
        return 1;
    }
    if (!strcmp(key, "room_errors")) {
        /* every room: rows the same width, one way in, a way on unless it is
         * an egg room, warps and flowers together, exits that lead somewhere */
        int bad = 0;
        for (int r = 0; r < DK_ROOMS; r++) {
            const DKRoom *R = &DK_ROOM[r];
            int w = (int)strlen(R->rows[0]), starts = 0, exits = 0, eggs = 0, flowers = 0, h = 0;
            int warps[3] = {0, 0, 0};
            for (int y = 0; R->rows[y]; y++, h++) {
                if ((int)strlen(R->rows[y]) != w) { bad++; fprintf(stderr, "room %d (%s) row %d width %d != %d\n", r, R->name, y, (int)strlen(R->rows[y]), w); }
                for (const char *c = R->rows[y]; *c; c++) {
                    starts += *c == 'S';
                    exits += *c == '>';
                    eggs += *c == 'Q';
                    flowers += *c == 'b' || *c == 'o';
                    if (*c >= '1' && *c <= '3') warps[*c - '1']++;
                }
            }
            if (w > DK_MAXW || h > DK_MAXH || h < 18 || w < 32) { bad++; fprintf(stderr, "room %d (%s): %dx%d\n", r, R->name, w, h); }
            if (starts != 1) { bad++; fprintf(stderr, "room %d (%s): %d starts\n", r, R->name, starts); }
            if (r == RM_M0) continue;
            if (eggs == 0 && (exits == 0 || R->next < 0)) { bad++; fprintf(stderr, "room %d (%s): no way on\n", r, R->name); }
            if (exits > 0 && R->next < 0) { bad++; fprintf(stderr, "room %d (%s): an exit that goes nowhere\n", r, R->name); }
            bool any_warp = false;
            for (int k = 0; k < 3; k++) {
                if (warps[k]) any_warp = true;
                if ((warps[k] > 0) != (R->warp_room[k] >= 0)) { bad++; fprintf(stderr, "room %d (%s): warp %d\n", r, R->name, k + 1); }
            }
            if (any_warp != (flowers > 0)) { bad++; fprintf(stderr, "room %d (%s): warps without flowers or flowers without warps\n", r, R->name); }
        }
        *out = bad;
        return 1;
    }
    return 0;
}

static int dk_cheat(const char *cmd) {
    int a, b, c;
    if (sscanf(cmd, "room2 %d", &a) == 1) {
        start_run(2);
        enter_room(iclamp(a, 0, DK_ROOMS - 1), 0);
        return 1;
    }
    if (sscanf(cmd, "room %d %d", &a, &b) == 2 || (b = 0, sscanf(cmd, "room %d", &a) == 1)) {
        if (state != ST_PLAY) start_run(1);
        fade_t = 0;
        pending_room = -1;
        enter_room(iclamp(a, 0, DK_ROOMS - 1), b);
        return 1;
    }
    if (sscanf(cmd, "pos2 %d %d", &a, &b) == 2) {
        DKPlayer *p = &dk_w.P[1];
        p->x = (float)a; p->y = (float)b; p->vx = p->vy = 0; p->ground = 0; p->act = ACT_NONE; p->coyote = 0; p->on_foe = -1;
        return 1;
    }
    if (sscanf(cmd, "pos %d %d", &a, &b) == 2) {
        DKPlayer *p = &dk_w.P[0];
        p->x = (float)a; p->y = (float)b; p->vx = p->vy = 0; p->ground = 0; p->act = ACT_NONE; p->coyote = 0; p->on_foe = -1;
        camera_snap();
        return 1;
    }
    {
        /* above_foe KIND DY: put the duskling DY pixels over the first such foe */
        char kn[16];
        int nth = 0;
        if (sscanf(cmd, "above_foe %15s %d %d", kn, &a, &nth) >= 2) {
            static const char *N[F_KINDS] = {"prickle", "wasp", "waspv", "puff", "frog", "gulper", "eye", "beetle", "newt", "crow", "fish"};
            for (int k = 0; k < F_KINDS; k++) {
                if (strcmp(kn, N[k])) continue;
                for (int i = 0; i < DK_FOES; i++) {
                    const DKFoe *f = &dk_w.foe[i];
                    if (!f->alive || f->kind != k) continue;
                    if (nth-- > 0) continue;
                    DKPlayer *p = &dk_w.P[0];
                    float top = f->y + (k == F_FROG ? 3 : k == F_BEETLE ? 2 : 0);
                    p->x = f->x + (k == F_GULPER ? 6 : 1.5f);
                    p->y = top - 9 - (float)a;
                    p->vx = p->vy = 0; p->ground = 0; p->act = ACT_NONE; p->coyote = 0; p->on_foe = -1;
                    camera_snap();
                    return 1;
                }
            }
            return 0;
        }
    }
    if (sscanf(cmd, "above_boss %d", &a) == 1) {
        DKPlayer *p = &dk_w.P[0];
        p->x = dk_w.boss.x + 6;
        p->y = dk_w.boss.y - 9 - (float)a;
        p->vx = p->vy = 0; p->ground = 0; p->act = ACT_NONE; p->coyote = 0; p->on_foe = -1;
        return 1;
    }
    if (sscanf(cmd, "eggs %d", &a) == 1) { sv.eggs = (uint8_t)(a & 7); save_now(); return 1; }
    if (!strcmp(cmd, "no_foes")) { memset(dk_w.foe, 0, sizeof dk_w.foe); return 1; }
    if (sscanf(cmd, "boss_hp %d", &a) == 1) { dk_w.boss.hp = (int16_t)a; return 1; }
    if (sscanf(cmd, "solve %d %d %d", &a, &b, &c) == 3 || (c = 12000, sscanf(cmd, "solve %d %d", &a, &b) == 2)) {
        /* print a route through room a to target b (0 way on, 1-3 warps, 4 egg) */
        int ok = dk_solve(a, 0, b, c, stdout);
        fflush(stdout);
        if (state != ST_PLAY) start_run(1);
        enter_room(a, 0);
        return ok ? 1 : 1;
    }
    if (sscanf(cmd, "solve_tile %d %d", &a, &b) == 2) { dk_solve_tile(a, b); return 1; }
    if (sscanf(cmd, "nerf %d", &a) == 1) { dk_nerf = a; return 1; }
    if (sscanf(cmd, "solve_at %d %d %d", &a, &b, &c) == 3) {
        /* the same from a warp's landing place ('@' = 1, '&' = 2) */
        int ok = dk_solve(a, b, c, 12000, stdout);
        fflush(stdout);
        return ok ? 1 : 1;
    }
    return 0;
}

const GameDef GAME_DUSKLING = {
    "duskling",
    "DUSKLING",
    "1985",
    "PLATFORMER",
    "EVERY WAY ON THE PAD IS LEFT, EVERY BUTTON IS RIGHT. FIND THE EGGS.",
    {"FIND A WARP", "FIND AN EGG", "FIND ALL THREE EGGS"},
    "D-PAD\tWALK LEFT (ANY WAY)\n"
    GLYPH_A " / " GLYPH_B "\tWALK RIGHT\n"
    "HOLD + TAP\tJUMP TO THE HELD SIDE\n"
    "HOLD LONGER\tJUMP HIGHER\n"
    "TAP, BOTH\tA LOW HOP\n"
    "TAP TAP\tROLL (HOLD: SPRINT)\n"
    "IN THE AIR\tTAP TAP: SOMERSAULT\n"
    "IN THE AIR\tHOLD + TAP: SLAM\n"
    "START\tPAUSE",
    C_PINK, C_PURPLE,
    dk_load, dk_start, dk_update, dk_draw, dk_quit, dk_label, dk_query, dk_cheat,
    "MOONCAT", 13,
};
