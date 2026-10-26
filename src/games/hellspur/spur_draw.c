/* HELLSPUR - drawing the title, the story pages, the field (the map, its
 * people, the wisps and the posse in a line), and the ending; the
 * cartridge label; and the layout checks the tests run. */
#include "spur.h"

static const uint8_t TITLE_GRAD[3] = {C_YELLOW, C_ORANGE, C_RED};

/* a red sky over the prairie, with a burning saloon on the skyline */
static void hell_sky(int t, bool fire) {
    for (int y = 0; y < SCREEN_H; y++) {
        int c = y < 40 ? C_INK : y < 70 ? C_MAROON : y < 100 ? C_WINE : y < 118 ? C_RED : C_INK;
        gfx_hline(0, SCREEN_W - 1, y, c);
    }
    gfx_dither(0, 36, SCREEN_W, 8, C_MAROON, 8);
    gfx_dither(0, 66, SCREEN_W, 8, C_WINE, 8);
    gfx_dither(0, 96, SCREEN_W, 8, C_RED, 8);
    gfx_circ(250, 92, 18, C_ORANGE);
    gfx_circ(250, 92, 14, C_YELLOW);
    /* the skyline */
    gfx_rect(0, 118, SCREEN_W, 62, C_INK);
    for (int x = 0; x < SCREEN_W; x += 4) gfx_rect(x, 112 + ((x * 7) % 9) / 3, 4, 8, C_INK);
    gfx_rect(40, 92, 44, 28, C_INK);   /* the saloon */
    gfx_rect(48, 84, 28, 10, C_INK);
    gfx_rect(100, 100, 24, 20, C_INK);
    gfx_rect(140, 104, 18, 16, C_INK);
    gfx_rect(196, 98, 6, 22, C_INK);    /* a windmill */
    for (int k = 0; k < 4; k++) {
        int a = t / 8 + k * 16;
        gfx_line(199, 98, 199 + (int)(10 * sinf(a * 0.098f)), 98 + (int)(10 * cosf(a * 0.098f)), C_INK);
    }
    if (fire)
        for (int k = 0; k < 12; k++) {
            int x = 42 + (k * 37) % 40, hh = 6 + ((t / 4 + k * 5) % 9);
            gfx_line(x, 92, x + ((t / 6 + k) % 3) - 1, 92 - hh, k % 3 ? C_ORANGE : C_YELLOW);
        }
}

static void draw_title(void) {
    int t = sg.state_t;
    hell_sky(t, true);
    ui_fancy_center("HELLSPUR", 160, 18, 3, TITLE_GRAD, 3, C_INK, C_MAROON);
    text_center_shadow("A TALE OF THE TERRITORY OF CALICO", 160, 46, C_CREAM, C_INK);
    int y = 132;
    if (spur_records.valid) {
        text_center("CONTINUE", 160, y, sg.title_sel == 0 ? C_YELLOW : C_GREY);
        y += 12;
    }
    text_center("NEW GAME", 160, y, sg.title_sel == 1 ? C_YELLOW : C_GREY);
    int sel_y = spur_records.valid ? 132 + sg.title_sel * 12 : 132;
    const char *lab = sg.title_sel == 0 && spur_records.valid ? "CONTINUE" : "NEW GAME";
    if ((t / 10) % 4 != 3) text_draw(GLYPH_RIGHT, 160 - text_width(lab) / 2 - 10, sel_y, C_YELLOW);
    char b[64];
    snprintf(b, sizeof b, "BATTLES %d   POSSE DEATHS %d", spur_records.battles, spur_records.deaths);
    tiny_center(b, 160, 162, C_GREY);
    tiny_draw(GLYPH_B " LIBRARY", 4, 172, C_DUSK);
    tiny_draw("BEAMDOWN SOFTWORKS 1988", SCREEN_W - 4 - tiny_width("BEAMDOWN SOFTWORKS 1988"), 172, C_DUSK);
}

static void draw_intro(void) {
    hell_sky(sg.state_t + sg.intro_page * 100, sg.intro_page >= 2);
    gfx_rect(16, 124, 288, 48, C_INK);
    gfx_rectb(16, 124, 288, 48, C_MAROON);
    const char *page = SPUR_INTRO[iclamp(sg.intro_page, 0, SPUR_INTRO_PAGES - 1)];
    char line[96];
    int y = 128, n = 0;
    for (const char *p = page;; p++) {
        if (*p == '\n' || !*p) {
            line[n] = 0;
            text_center(line, 160, y, C_CREAM);
            y += 10;
            n = 0;
            if (!*p) break;
        } else if (n < 94) line[n++] = *p;
    }
    if ((sg.state_t / 15) % 2) text_draw(GLYPH_A, 294, 160, C_YELLOW);
}

/* ---- the field ------------------------------------------------------------------------- */
static int lerp16(int a, int b, int num, int den) { return a * 16 + (b - a) * 16 * num / imax(1, den); }

static void draw_field(void) {
    int num = sg.moving ? sg.move_t : 1, den = sg.moving ? sg.step_frames : 1;
    int lx = lerp16(sg.prev[0].x, sg.pos[0].x, num, den), ly = lerp16(sg.prev[0].y, sg.pos[0].y, num, den);
    int mw = sg.map.w * TS, mh = sg.map.h * TS;
    int cx = lx + 8 - SCREEN_W / 2, cy = ly + 8 - SCREEN_H / 2;
    if (mw <= SCREEN_W) cx = (mw - SCREEN_W) / 2;
    else cx = iclamp(cx, 0, mw - SCREEN_W);
    if (mh <= SCREEN_H) cy = (mh - SCREEN_H) / 2;
    else cy = iclamp(cy, 0, mh - SCREEN_H);
    sg.cam_x = cx;
    sg.cam_y = cy;
    int edge = sg.map.kind == MK_WORLD ? (sg.map_id == M_HELL ? T_LAVA : T_SEA) : -1;
    gfx_cls(C_INK);
    int tx0 = (cx >= 0 ? cx / TS : -((-cx + TS - 1) / TS)), ty0 = (cy >= 0 ? cy / TS : -((-cy + TS - 1) / TS));
    for (int ty = ty0; ty <= ty0 + VIEW_H + 1; ty++)
        for (int tx = tx0; tx <= tx0 + VIEW_W + 1; tx++) {
            int t = spur_tile_at(&sg.map, tx, ty);
            if (t < 0) t = edge;
            if (t < 0) continue;
            spur_draw_tile(t, tx * TS - cx, ty * TS - cy, tx, ty);
        }
    /* people */
    for (int i = 0; i < sg.map.nnpc; i++) {
        if (!spur_npc_visible(i)) continue;
        const SpurNpc *n = &sg.map.npc[i];
        int fr = n->wander ? (sg.ani / 20 + i) % 2 : 0;
        spur_draw_person(n->look, n->pal, sg.npc_dir[i], fr, sg.npc_x[i] * TS - cx, sg.npc_y[i] * TS - cy);
    }
    /* wisps */
    for (int w = 0; w < SPUR_WISPS; w++) {
        const SpurWisp *q = &sg.wisp[w];
        if (!q->alive) continue;
        int f = imin(q->t, 8);
        int x = lerp16(q->ox, q->x, f, 8) - cx, y = lerp16(q->oy, q->y, f, 8) - cy;
        spur_draw_wisp(x, y - 2 + ((sg.ani / 8 + w) % 2), sg.ani + w * 5);
    }
    /* the posse, the last first */
    int n = spur_flag(F_POSSE) ? imax(1, sg.s.nposse) : 1;
    for (int i = n - 1; i >= 0; i--) {
        int x = lerp16(sg.prev[i].x, sg.pos[i].x, num, den) - cx, y = lerp16(sg.prev[i].y, sg.pos[i].y, num, den) - cy;
        int dir = i == 0 ? sg.face : sg.pos[i].dir;
        int fr = sg.moving ? (sg.move_t * 2 / imax(1, sg.step_frames)) % 2 : 0;
        if (!spur_flag(F_POSSE)) { spur_draw_person(LOOK_ANGEL, 0, dir, fr, x, y); continue; }
        int h = spur_hero_at(i);
        if (h < 0) continue;
        bool dead = !spur_alive(h);
        if (sg.mounted && !dead) {
            int lift = sg.mounted == 2 ? 6 + ((sg.ani / 10) % 2) : 0;
            if (sg.mounted == 2) gfx_dither(x + 2, y + 12, 12, 3, C_INK, 8);
            spur_draw_horse(x, y - lift, fr, sg.mounted == 2, dir);
            spur_draw_hero(h, dir, 0, x, y - 8 - lift, false);
        } else spur_draw_hero(h, dir, fr, x, y, dead);
    }
    if (sg.flash_poison) gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_LIME, 3);
    /* the place's name as you come in */
    if (sg.map_t < 100 && sg.map.kind != MK_WORLD) {
        const char *nm = spur_map_name(sg.map_id);
        int w = text_width(nm) + 16;
        gfx_rect(160 - w / 2, 6, w, 14, C_INK);
        gfx_rectb(160 - w / 2, 6, w, 14, C_BROWN);
        text_center(nm, 160, 10, C_CREAM);
    }
    if (sg.msg_t > 0) {
        int w = text_width(sg.msg) + 16;
        gfx_rect(160 - w / 2, 24, w, 14, C_INK);
        text_center(sg.msg, 160, 28, C_YELLOW);
    }
    if (sg.demo) {
        tiny_draw("DEMO", 4, 4, (sg.ani / 20) % 2 ? C_YELLOW : C_ORANGE);
    }
}

static void draw_ending(void) {
    gfx_cls(C_INK);
    int t = sg.end_t;
    /* the sun comes up yellow */
    int sun = iclamp(t / 4, 0, 60);
    for (int y = 0; y < 120; y++) gfx_hline(0, SCREEN_W - 1, y, y < 40 ? C_NAVY : y < 80 ? C_BLUE : C_SKY);
    gfx_circ(160, 140 - sun, 22, C_YELLOW);
    gfx_rect(0, 120, SCREEN_W, 60, C_HIDE);
    gfx_rect(110, 96, 100, 26, C_BROWN);    /* the new Garter, going up */
    gfx_rect(120, 86, 80, 12, C_BROWN);
    for (int x = 112; x < 210; x += 8) gfx_vline(x, 96, 121, C_EARTH);
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (h >= 0) spur_draw_hero(h, 0, (sg.ani / 30 + i) % 2, 120 + i * 22, 132, !spur_alive(h));
    }
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, 8);
    int base = 180 - t / 2 + 40;
    for (int k = 0;; k++) {
        const char *l = spur_ending_line(k);
        if (!l) break;
        int y = base + k * 12;
        if (y < -10 || y > SCREEN_H) continue;
        text_center_shadow(l, 160, y, !strcmp(l, "HELLSPUR") || !strcmp(l, "THE END") ? C_YELLOW : C_WHITE, C_INK);
    }
}

void spur_draw(void) {
    gfx_camera(0, 0);
    switch (sg.state) {
    case GS_TITLE: draw_title(); break;
    case GS_INTRO: draw_intro(); break;
    case GS_FIELD: draw_field(); spur_menu_draw(); break;
    case GS_BATTLE: spur_battle_draw(); break;
    case GS_ENDING: draw_ending(); break;
    default: gfx_cls(C_INK); break;
    }
}

void spur_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    for (int k = 0; k < h; k++) gfx_hline(x, x + w - 1, y + k, k < h / 3 ? C_MAROON : k < h * 2 / 3 ? C_RED : C_ORANGE);
    gfx_circ(x + w - 16, y + h - 14, 9, C_YELLOW);
    gfx_rect(x, y + h - 10, w, 10, C_INK);
    /* a spur on the skyline */
    int sx = x + 18, sy = y + h - 18;
    gfx_circb(sx, sy, 6, C_INK);
    gfx_circb(sx, sy, 5, C_INK);
    for (int k = 0; k < 8; k++) gfx_line(sx, sy, sx + (int)(8 * sinf(k * 0.785f + t * 0.02f)), sy + (int)(8 * cosf(k * 0.785f + t * 0.02f)), C_INK);
    gfx_rect(sx + 5, sy - 1, 14, 3, C_INK);
    ui_fancy_text("HELLSPUR", x + 4, y + 4, 1, TITLE_GRAD, 3, C_INK, C_MAROON);
    gfx_noclip();
}

/* tests: the words on the fixed screens fit and have glyphs */
int spur_layout_audit(void) {
    int bad = 0;
    ui_audit_begin("hellspur", true);
    ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
    ui_audit_text("subtitle", "A TALE OF THE TERRITORY OF CALICO", 160 - text_width("A TALE OF THE TERRITORY OF CALICO") / 2, 46);
    bad += ui_audit_end();
    for (int i = 0; i < SPUR_INTRO_PAGES; i++) {
        char line[96];
        int n = 0;
        for (const char *p = SPUR_INTRO[i];; p++) {
            if (*p == '\n' || !*p) {
                line[n] = 0;
                if (text_width(line) > 284 || text_missing(line)) { bad++; fprintf(stderr, "intro %d: \"%s\" too wide\n", i, line); }
                n = 0;
                if (!*p) break;
            } else if (n < 94) line[n++] = *p;
        }
    }
    for (int k = 0;; k++) {
        const char *l = spur_ending_line(k);
        if (!l) break;
        if (text_width(l) > SCREEN_W - 8 || text_missing(l)) { bad++; fprintf(stderr, "ending: \"%s\" too wide\n", l); }
    }
    for (int h = 0; h < H_COUNT; h++)
        for (int k = 0; k < 2; k++)
            if (text_width(SPUR_HERO[h].pick_line[k]) > 290 || text_missing(SPUR_HERO[h].pick_line[k])) { bad++; fprintf(stderr, "pick line %d\n", h); }
    for (int i = 1; i < IT_COUNT; i++)
        if (text_width(SPUR_ITEM[i].name) > 84 || text_missing(SPUR_ITEM[i].name)) { bad++; fprintf(stderr, "item \"%s\" too wide\n", SPUR_ITEM[i].name); }
    for (int s = 1; s < SK_COUNT; s++)
        if (text_width(SPUR_SKILL[s].name) > 74 || text_missing(SPUR_SKILL[s].name)) { bad++; fprintf(stderr, "skill \"%s\" too wide\n", SPUR_SKILL[s].name); }
    for (int e = 1; e < SPUR_ENEMIES; e++)
        if (text_width(SPUR_ENEMY[e].name) > 84 || text_missing(SPUR_ENEMY[e].name)) { bad++; fprintf(stderr, "foe \"%s\" too wide\n", SPUR_ENEMY[e].name); }
    for (int m = 0; m < M_COUNT; m++)
        if (text_width(spur_map_name(m)) > 300) bad++;
    return bad;
}
