/* UFO 40 - the game library: 40 cartridge slots. */
#include "shell.h"

#define GRID_X 8
#define GRID_Y 24
#define CELL_W 20
#define CELL_H 27
#define COLS 8
#define ROWS 5
#define PANEL_X 172
#define PANEL_W 142

static int t, launch_t, shake_t;
static bool launching;

static void lib_enter(void) {
    t = 0;
    launching = false;
    launch_t = 0;
    shake_t = 0;
    game_set_pausable(true);
    shell_menu_music();
}

static int available_count(void) {
    int n = 0;
    for (int i = 0; i < GAME_SLOTS; i++) n += GAMES[i] != NULL;
    return n;
}

static void lib_update(void) {
    t++;
    if (shake_t > 0) shake_t--;
    if (launching) {
        launch_t++;
        if (launch_t == 26) gfx_set_flash(3);
        if (launch_t == 34) app_launch_game(g_library_cursor, true);
        return;
    }
    int c = g_library_cursor % COLS, r = g_library_cursor / COLS;
    int oc = c, or_ = r;
    if (btn_repeat(BTN_LEFT)) c = (c + COLS - 1) % COLS;
    if (btn_repeat(BTN_RIGHT)) c = (c + 1) % COLS;
    if (btn_repeat(BTN_UP)) r = (r + ROWS - 1) % ROWS;
    if (btn_repeat(BTN_DOWN)) r = (r + 1) % ROWS;
    if (c != oc || r != or_) {
        g_library_cursor = r * COLS + c;
        sfx_play_name("ui_move");
    }
    if (btnp(BTN_A) || btnp(BTN_START)) {
        if (GAMES[g_library_cursor]) {
            launching = true;
            launch_t = 0;
            sfx_play_name("cart_insert");
            music_fade(30);
        } else {
            shake_t = 12;
            sfx_play_name("ui_error");
        }
    }
    if (btnp(BTN_SELECT)) {
        sfx_play_name("ui_ok");
        shell_open_options(&SCENE_LIBRARY);
    } else if (btnp(BTN_B)) {
        sfx_play_name("ui_back");
        scene_goto(&SCENE_MENU);
    }
}

static void draw_cart(int x, int y, int idx, bool sel) {
    const GameDef *g = GAMES[idx];
    int body = g ? C_LIGHT : C_DUSK, edge = g ? C_GREY : C_NIGHT, dark = g ? C_SLATE : C_INK;
    gfx_rect(x + 2, y, 14, 2, body);
    gfx_rect(x, y + 2, 18, 20, body);
    gfx_vline(x + 17, y + 2, y + 21, edge);
    gfx_hline(x, x + 17, y + 21, edge);
    gfx_vline(x, y + 2, y + 21, g ? C_WHITE : C_SLATE);
    /* grip ridges */
    for (int i = 0; i < 3; i++) gfx_hline(x + 5, x + 12, y + 1 + i * 1 + 1, i % 2 ? edge : body);
    char num[12];
    snprintf(num, sizeof num, "%02d", idx + 1);
    if (g) {
        gfx_rect(x + 2, y + 5, 14, 12, g->cart_main);
        gfx_rect(x + 2, y + 13, 14, 4, g->cart_accent);
        gfx_hline(x + 2, x + 15, y + 12, C_INK);
        tiny_draw(num, x + 4, y + 6, C_WHITE);
        /* goal pips */
        for (int b = 0; b < 3; b++) {
            bool on = (g_progress.goals[idx] >> b) & 1;
            gfx_rect(x + 4 + b * 4, y + 14, 2, 2, on ? C_YELLOW : C_INK);
        }
    } else {
        gfx_rect(x + 2, y + 5, 14, 12, C_NIGHT);
        text_draw("?", x + 7, y + 7, sel ? C_GREY : C_SLATE);
    }
    /* contacts */
    gfx_rect(x + 3, y + 18, 12, 3, dark);
    for (int i = 0; i < 6; i++) gfx_pset(x + 4 + i * 2, y + 19, g ? C_AMBER : C_DUSK);
}

static void draw_panel(void) {
    int idx = g_library_cursor;
    const GameDef *g = GAMES[idx];
    int x = PANEL_X, y = 22;
    /* label window */
    gfx_rect(x - 1, y - 1, PANEL_W + 2, 66, C_INK);
    ui_panel(x - 2, y - 2, PANEL_W + 4, 68, C_INK, g ? C_GREY : C_DUSK);
    if (g && g->draw_label) {
        gfx_clip(x, y, PANEL_W, 64);
        g->draw_label(x, y, PANEL_W, 64, t);
        if (g->tribute) {
            /* a small sticker along the bottom of the label art */
            char tb[64];
            snprintf(tb, sizeof tb, "TRIBUTE TO %s " GLYPH_DOT " UFO 50 #%d", g->tribute, g->tribute_no);
            if (tiny_width(tb) > PANEL_W - 2) /* long names: a shorter sticker */
                snprintf(tb, sizeof tb, "TRIBUTE: %s " GLYPH_DOT " UFO 50 #%d", g->tribute, g->tribute_no);
            gfx_rect(x, y + 57, PANEL_W, 7, C_INK);
            gfx_hline(x, x + PANEL_W - 1, y + 56, C_NIGHT);
            tiny_center(tb, x + PANEL_W / 2, y + 58, C_GREY);
        }
        gfx_noclip();
    } else {
        /* no-signal static */
        for (int yy = 0; yy < 64; yy++)
            for (int xx = 0; xx < PANEL_W; xx += 2) {
                uint32_t h = (uint32_t)(xx * 73856093u) ^ (uint32_t)((yy + t * 3) * 19349663u);
                h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
                int c = (h & 7) == 0 ? C_SLATE : (h & 7) == 1 ? C_DUSK : C_NIGHT;
                gfx_rect(x + xx, y + yy, 2, 1, c);
            }
        ui_panel(x + 26, y + 22, 90, 20, C_INK, C_DUSK);
        text_center("NO SIGNAL", x + PANEL_W / 2, y + 28, (t / 30) % 2 ? C_GREY : C_SLATE);
    }
    int ty = y + 70;
    if (g) {
        static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
        int scale = ui_fancy_width(g->title, 2) <= PANEL_W ? 2 : 1;
        ui_fancy_text(g->title, x, ty, scale, grad, 4, C_INK, C_WINE);
        ty += scale == 2 ? 18 : 11;
        char meta[64];
        snprintf(meta, sizeof meta, "%s - %s", g->year, g->genre);
        tiny_draw(meta, x, ty, C_SKY);
        text_wrap(g->blurb, x, ty + 8, PANEL_W, C_LIGHT, 9);
        int gy = 150;
        for (int b = 0; b < 3; b++) {
            int bit = 1 << b;
            ui_goal_icon(x + b * 13, gy, bit, (g_progress.goals[idx] & bit) != 0, t);
        }
        const char *names[3] = {"BEACON", "SAUCER", "ALIEN"};
        int sel_goal = (t / 150) % 3;
        char line[80];
        bool got = (g_progress.goals[idx] >> sel_goal) & 1;
        snprintf(line, sizeof line, "%s: %s", names[sel_goal], g->goal_desc[sel_goal]);
        (void)got;
        gfx_rect(x + sel_goal * 13, gy + 10, 9, 1, C_YELLOW);
        /* the goal's wording, wrapped onto a second line if it's long */
        const char *desc = g->goal_desc[sel_goal];
        int room = PANEL_W - 42;
        if (tiny_width(desc) <= room) {
            tiny_draw(names[sel_goal], x + 42, gy, C_YELLOW);
            tiny_draw(desc, x + 42, gy + 6, C_GREY);
        } else {
            char first[64];
            int cut = 0;
            for (int i = 0; desc[i] && i < (int)sizeof first - 1; i++) {
                if (desc[i] != ' ') continue;
                memcpy(first, desc, (size_t)i);
                first[i] = 0;
                if (tiny_width(first) > room) break;
                cut = i;
            }
            if (cut == 0) cut = (int)strlen(desc);
            memcpy(first, desc, (size_t)imin(cut, (int)sizeof first - 1));
            first[imin(cut, (int)sizeof first - 1)] = 0;
            tiny_draw(names[sel_goal], x + 42, gy - 3, C_YELLOW);
            tiny_draw(first, x + 42, gy + 3, C_GREY);
            if (desc[cut]) tiny_draw(desc + cut + 1, x + 42, gy + 9, C_GREY);
        }
    } else {
        static const uint8_t grad[] = {C_GREY, C_SLATE};
        ui_fancy_text("COMING SOON", x, ty, 1, grad, 2, C_INK, -1);
        char buf[64];
        snprintf(buf, sizeof buf, "SLOT %02d IS STILL IN THE\nSAUCER'S CARGO HOLD.", idx + 1);
        text_draw(buf, x, ty + 14, C_SLATE);
        tiny_draw("CHECK BACK AFTER THE NEXT LANDING", x, ty + 38, C_DUSK);
    }
}

static void lib_draw(void) {
    ui_starfield(t, C_INK);
    /* header */
    gfx_rect(0, 0, SCREEN_W, 17, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 17, C_DUSK);
    static const uint8_t g_ufo[] = {C_YELLOW, C_AMBER, C_ORANGE};
    static const uint8_t g_40[] = {C_CYAN, C_SKY, C_BLUE};
    ui_fancy_text("UFO", 7, 5, 1, g_ufo, 3, C_INK, -1);
    ui_fancy_text("40", 27, 5, 1, g_40, 3, C_INK, -1);
    text_draw("GAME LIBRARY", 46, 5, C_LIGHT);
    char buf[48];
    snprintf(buf, sizeof buf, "%d/40 LOADED", available_count());
    tiny_draw(buf, 148, 7, C_SLATE);
    snprintf(buf, sizeof buf, "%d/%d", progress_goal_count(), available_count() * 3);
    ui_goal_icon(252, 4, GOAL_SAUCER, true, t);
    text_draw(buf, 264, 5, C_YELLOW);

    /* grid */
    int shake = shake_t > 0 ? ((shake_t / 2) % 2 ? 2 : -2) : 0;
    for (int i = 0; i < GAME_SLOTS; i++) {
        int c = i % COLS, r = i / COLS;
        int x = GRID_X + c * CELL_W, y = GRID_Y + r * CELL_H;
        bool sel = i == g_library_cursor;
        int lift = 0;
        if (sel) {
            lift = 2 + ((t / 10) % 2);
            if (launching) lift = 2 + launch_t / 2;
            x += shake;
        }
        if (sel && launching && launch_t > 20 && (launch_t / 2) % 2) continue;
        if (sel) gfx_rect(x + 1, y + 21, 17, 3, C_NIGHT); /* shadow */
        draw_cart(x, y - lift, i, sel);
        if (sel && !launching) {
            int bl = (t / 16) % 2;
            int col = GAMES[i] ? C_YELLOW : C_GREY;
            int x0 = x - 2 - bl, y0 = y - lift - 2 - bl, x1 = x + 19 + bl, y1 = y - lift + 23 + bl;
            gfx_hline(x0, x0 + 3, y0, col); gfx_vline(x0, y0, y0 + 3, col);
            gfx_hline(x1 - 3, x1, y0, col); gfx_vline(x1, y0, y0 + 3, col);
            gfx_hline(x0, x0 + 3, y1, col); gfx_vline(x0, y1 - 3, y1, col);
            gfx_hline(x1 - 3, x1, y1, col); gfx_vline(x1, y1 - 3, y1, col);
        }
    }
    draw_panel();

    /* footer */
    gfx_rect(0, 167, SCREEN_W, 13, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 166, C_DUSK);
    int fx = ui_hint(6, 170, GLYPH_A, "PLAY", C_LIGHT);
    fx = ui_hint(fx, 170, GLYPH_B, "MENU", C_LIGHT);
    fx = text_draw("SELECT", fx, 170, C_WHITE);
    text_draw("OPTIONS", fx + 4, 170, C_LIGHT);
    snprintf(buf, sizeof buf, "SLOT %02d/40", g_library_cursor + 1);
    text_draw(buf, SCREEN_W - 6 - text_width(buf), 170, C_GREY);
}

const Scene SCENE_LIBRARY = {"library", lib_enter, lib_update, lib_draw, NULL};
