/* LIMPET - drawing: the station's hull and every tile, the pods, the
 * people, foes and shots, the tank, the status strip, the camera that
 * snaps along in steps, and the title, story, death, clear and ending
 * screens. */
#include "lmp.h"

extern const char *const LMP_STORY[];
extern const int LMP_STORY_N;
extern const char *const LMP_ENDING[];
extern const int LMP_ENDING_N;
extern const char *const LMP_CHERRY_END[];
extern const int LMP_CHERRY_END_N;

#define SNAP 40 /* the camera moves in steps of this many pixels */

static int cam_x, cam_y;

static void update_camera(void) {
    int tx, ty;
    lmp_tank_centre(&tx, &ty);
    int mwp = lg.mw * LMP_T, mhp = lg.mh * LMP_T;
    int x, y;
    if (mwp <= SCREEN_W) x = (mwp - SCREEN_W) / 2;
    else {
        x = tx - SCREEN_W / 2;
        x = ((x + SNAP / 2) / SNAP) * SNAP;
        if (tx - SCREEN_W / 2 + SNAP / 2 < 0) x = 0;
        x = iclamp(x, 0, mwp - SCREEN_W);
    }
    if (mhp <= LMP_VIEW_H) y = (mhp - LMP_VIEW_H) / 2;
    else {
        y = ty - LMP_VIEW_H / 2;
        y = ((y + SNAP / 2) / SNAP) * SNAP;
        if (ty - LMP_VIEW_H / 2 + SNAP / 2 < 0) y = 0;
        y = iclamp(y, 0, mhp - LMP_VIEW_H);
    }
    cam_x = x;
    cam_y = y - LMP_HUD;
    lg.cam_x = x;
    lg.cam_y = y;
}

/* ---- tiles ------------------------------------------------------------------ */

static int hull_col(void) {
    if (lg.level == LV_HUB) return C_SLATE;
    switch (lg.kind) {
    case LK_SURVIVAL: return C_WINE;
    case LK_SECRET: return C_BROWN;
    case LK_TURNCOAT: return C_DUSK;
    case LK_DREAD: return C_MAROON;
    default: break;
    }
    static const int C[4] = {C_SLATE, C_DUSK, C_NAVY, C_FOREST};
    return C[LMP_LEVEL[lg.level]->music & 3];
}

static void tri_fill(int x, int y, char c, int col, int edge) {
    for (int r = 0; r < LMP_T; r++) {
        int a, b;
        switch (c) {
        case 'J': a = LMP_T - 1 - r; b = LMP_T - 1; break;   /* lower right */
        case 'L': a = 0; b = r; break;                       /* lower left */
        case '7': a = r; b = LMP_T - 1; break;               /* upper right */
        default: a = 0; b = LMP_T - 1 - r; break;            /* F: upper left */
        }
        gfx_hline(x + a, x + b, y + r, col);
        gfx_pset(x + (c == 'J' || c == '7' ? a : b), y + r, edge);
    }
}

static void plate(int x, int y, int col, int hi) {
    gfx_rect(x, y, LMP_T, LMP_T, col);
    gfx_hline(x, x + LMP_T - 1, y, hi);
    gfx_vline(x, y, y + LMP_T - 1, hi);
    gfx_hline(x, x + LMP_T - 1, y + LMP_T - 1, PAL_DARKER[col]);
    gfx_vline(x + LMP_T - 1, y, y + LMP_T - 1, PAL_DARKER[col]);
}

static void arrow(int cx, int cy, int dir, int col) {
    int dx = LMP_DX[dir], dy = LMP_DY[dir];
    for (int k = 0; k < 3; k++) {
        int px = cx + dx * (2 - k), py = cy + dy * (2 - k);
        if (dx) gfx_vline(px, py - k, py + k, col);
        else gfx_hline(px - k, px + k, py, col);
    }
}

static void draw_tile(int tx, int ty, char c, int t) {
    int x = tx * LMP_T, y = ty * LMP_T;
    int hc = hull_col();
    switch (c) {
    case '.':
        gfx_rect(x, y, LMP_T, LMP_T, C_NIGHT);
        if ((tx + ty * 3) % 7 == 0) gfx_pset(x + 2, y + 2, C_DUSK);
        if (ty % 4 == 0) gfx_hline(x, x + LMP_T - 1, y, C_INK);
        break;
    case '#': case 'G':
        plate(x, y, hc, PAL_LIGHTER[hc]);
        if ((tx + ty) % 3 == 0) gfx_pset(x + 2, y + 2, PAL_LIGHTER[PAL_LIGHTER[hc]]);
        if (c == 'G')
            for (int k = 0; k < LMP_T; k += 4) gfx_line(x + k, y + LMP_T - 1, x + k + 3, y, C_AMBER);
        break;
    case 'X':
        plate(x, y, C_TAN, C_CREAM);
        if ((tx * 5 + ty) % 4 == 0) gfx_hline(x + 2, x + 7, y + 5, C_HIDE);
        break;
    case 'Z':
        gfx_rect(x, y, LMP_T, LMP_T, C_WINE);
        gfx_circ(x + 3 + (tx % 3), y + 4, 3, C_MAGENTA);
        gfx_circ(x + 7, y + 7 - (ty % 2), 2, C_PINK);
        break;
    case 'J': case 'L': case '7': case 'F':
        tri_fill(x, y, c, hc, PAL_LIGHTER[hc]);
        break;
    case 'M':
        plate(x, y, C_NAVY, C_BLUE);
        gfx_rect(x + 2, y + 3, 6, 4, C_RED);
        gfx_rect(x + 4, y + 3, 2, 2, C_NAVY);
        gfx_hline(x + 2, x + 3, y + 6, C_WHITE);
        gfx_hline(x + 6, x + 7, y + 6, C_WHITE);
        break;
    case '=': case '-': case '[': case ']': {
        int dir = c == '=' ? DR_U : c == '-' ? DR_D : c == '[' ? DR_R : DR_L;
        if (c == '=' || c == '-') { gfx_rect(x, y + 3, LMP_T, 4, C_GREY); gfx_hline(x, x + 9, y + 3, C_LIGHT); }
        else { gfx_rect(x + 3, y, 4, LMP_T, C_GREY); gfx_vline(x + 3, y, y + 9, C_LIGHT); }
        arrow(x + 5, y + 5, dir, (t / 15) % 2 ? C_CYAN : C_SKY);
        break;
    }
    case '^': {
        gfx_rect(x, y, LMP_T, LMP_T, C_INK);
        for (int d = 0; d < 8; d += 2) {
            if (!lmp_open(tx + LMP_DX[d], ty + LMP_DY[d])) continue;
            for (int k = 0; k < 2; k++) {
                int ox = d == DR_U || d == DR_D ? k * 5 : 0, oy = d == DR_L || d == DR_R ? k * 5 : 0;
                for (int r = 0; r < 4; r++) {
                    if (d == DR_U) gfx_hline(x + ox + 2 - r / 2, x + ox + 2 + r / 2, y + r, C_LIGHT);
                    if (d == DR_D) gfx_hline(x + ox + 2 - r / 2, x + ox + 2 + r / 2, y + 9 - r, C_LIGHT);
                    if (d == DR_L) gfx_vline(x + r, y + oy + 2 - r / 2, y + oy + 2 + r / 2, C_LIGHT);
                    if (d == DR_R) gfx_vline(x + 9 - r, y + oy + 2 - r / 2, y + oy + 2 + r / 2, C_LIGHT);
                }
            }
        }
        gfx_rect(x + 2, y + 2, 6, 6, C_GREY);
        break;
    }
    case 'B':
        plate(x, y, C_BROWN, C_TAN);
        gfx_line(x + 2, y + 2, x + 5, y + 6, C_EARTH);
        gfx_line(x + 5, y + 6, x + 8, y + 4, C_EARTH);
        break;
    case 'O':
        for (int k = 0; k < LMP_T; k += 2) {
            gfx_pset(x + k, y, C_CYAN); gfx_pset(x + k, y + 9, C_CYAN);
            gfx_pset(x, y + k, C_CYAN); gfx_pset(x + 9, y + k, C_CYAN);
        }
        break;
    case 'o':
        plate(x, y, C_TEAL, C_CYAN);
        break;
    case 'T': case 'K': {
        int on = (c == 'T') == (lg.parity == 0);
        int col = c == 'T' ? C_JADE : C_PINK;
        if (on) { plate(x, y, col, PAL_LIGHTER[col]); gfx_rect(x + 3, y + 3, 4, 4, PAL_DARKER[col]); }
        else for (int k = 1; k < LMP_T; k += 3) { gfx_pset(x + k, y + 1, col); gfx_pset(x + k, y + 8, col); }
        break;
    }
    case 'S':
        plate(x, y, C_GREY, C_LIGHT);
        gfx_rect(x + 2, y + 2, 3, 6, lg.parity == 0 ? C_JADE : C_DUSK);
        gfx_rect(x + 5, y + 2, 3, 6, lg.parity == 1 ? C_PINK : C_DUSK);
        break;
    case 'P':
        plate(x, y, C_EARTH, C_TAN);
        gfx_line(x + 1, y + 1, x + 8, y + 8, C_BROWN);
        gfx_line(x + 8, y + 1, x + 1, y + 8, C_BROWN);
        break;
    case 'D':
        gfx_rect(x, y, LMP_T, LMP_T, C_DUSK);
        for (int k = 1; k < LMP_T; k += 3) gfx_vline(x + k, y, y + 9, C_LIGHT);
        break;
    case 'd': case 'y': case 'u':
        plate(x, y, C_SLATE, C_GREY);
        gfx_circ(x + 5, y + 5, 3, c == 'd' ? C_LEAF : c == 'y' ? C_AMBER : C_DUSK);
        gfx_pset(x + 4, y + 4, C_WHITE);
        break;
    case 'Y':
        if (lg.timed_t > 0) { for (int k = 1; k < LMP_T; k += 3) gfx_pset(x + k, y + 5, C_AMBER); }
        else { gfx_rect(x, y, LMP_T, LMP_T, C_BROWN); for (int k = 1; k < LMP_T; k += 3) gfx_vline(x + k, y, y + 9, C_AMBER); }
        break;
    case 'C': {
        bool cur = lg.check.set && lg.check.x - LMP_DX[lg.check.n] == tx && lg.check.y - LMP_DY[lg.check.n] == ty;
        plate(x, y, cur ? C_LEAF : C_FOREST, C_LIME);
        tiny_draw("C", x + 4, y + 3, cur ? C_WHITE : C_LIME);
        break;
    }
    case '!':
        plate(x, y, C_AMBER, C_YELLOW);
        gfx_vline(x + 5, y + 2, y + 5, C_INK);
        gfx_pset(x + 5, y + 7, C_INK);
        break;
    case 'R':
        plate(x, y, C_SLATE, C_GREY);
        gfx_circ(x + 5, y + 5, 3, (t / 8) % 2 ? C_RED : C_ORANGE);
        gfx_pset(x + 4, y + 4, C_WHITE);
        break;
    case 'r':
        plate(x, y, C_SLATE, C_GREY);
        gfx_circ(x + 5, y + 5, 3, C_MAROON);
        break;
    case 'W':
        plate(x, y, C_ORANGE, C_AMBER);
        for (int k = 2; k < 8; k += 2) gfx_hline(x + 2, x + 7, y + k, C_BROWN);
        break;
    case '{': case '}': case '<': case '>': {
        int col = c == '{' || c == '}' ? C_ORANGE : C_SKY;
        int r = 3 + (t / 6 + tx) % 2;
        gfx_circb(x + 5, y + 5, r, col);
        gfx_circb(x + 5, y + 5, r - 2, PAL_LIGHTER[col]);
        break;
    }
    case 'm': {
        int mi = lmp_mover_at(tx, ty);
        bool lethal = mi >= 0 && lg.mover[mi].lethal;
        plate(x, y, lethal ? C_GREY : C_TAN, lethal ? C_LIGHT : C_CREAM);
        if (lethal) gfx_rect(x + 2, y + 2, 6, 6, C_SLATE);
        else gfx_hline(x + 1, x + 8, y + 5, C_BROWN);
        break;
    }
    case 'V': {
        static const int PC[3] = {C_PINK, C_LEAF, C_RED};
        int k = 0;
        for (int yy = 0; yy < lg.mh; yy++)
            for (int xx = 0; xx < lg.mw; xx++)
                if (lg.map[yy][xx] == 'V') { if (xx == tx && yy == ty) goto found; k++; }
    found:
        plate(x, y, C_GREY, C_LIGHT);
        gfx_rect(x + 2, y + 2, 6, 6, PC[k % 3]);
        tiny_draw("T", x + 4, y + 3, C_WHITE);
        break;
    }
    case '&':
        if (lmp_honey_count() >= LMP_HONEY_TOTAL) {
            gfx_circ(x + 5, y + 5, 4, C_AMBER);
            gfx_circb(x + 5, y + 5, 4, C_YELLOW);
            gfx_pset(x + 3, y + 3, C_WHITE);
        }
        break;
    case ' ':
        break;
    default:
        /* marks, spawn points and the like: back wall */
        gfx_rect(x, y, LMP_T, LMP_T, C_NIGHT);
        break;
    }
}

static void draw_pod(int cx, int cy, int state, int t, const char *label, bool open) {
    /* a capsule standing in the cell above its floor: state 0 grey, 1 cleared, 2 gold, 3 boss, 4 secret */
    int x = cx * LMP_T, y = cy * LMP_T;
    static const int BODY[5] = {C_GREY, C_LEAF, C_YELLOW, C_VIOLET, C_AMBER};
    int col = BODY[iclamp(state, 0, 4)];
    gfx_rect(x - 3, y - 8, 16, 18, C_INK);
    gfx_rect(x - 2, y - 7, 14, 17, PAL_DARKER[col]);
    gfx_rect(x - 1, y - 6, 12, 15, col);
    gfx_rect(x + 1, y - 4, 8, 11, open ? C_NIGHT : C_PINK);
    if (!open) { gfx_circ(x + 3, y, 2, C_MAGENTA); gfx_circ(x + 7, y + 4, 2, C_WINE); }
    int blink = (t / 10) % 2;
    gfx_pset(x - 1, y - 8, blink ? C_RED : C_YELLOW);
    gfx_pset(x + 10, y - 8, blink ? C_YELLOW : C_RED);
    if (label && label[0]) tiny_center(label, x + 5, y - 15, C_WHITE);
}

static void draw_world(void) {
    int t = lg.frame_t;
    /* the sky behind the station: stars that never move */
    gfx_rect(0, LMP_HUD, SCREEN_W, LMP_VIEW_H, C_INK);
    for (int k = 0; k < 70; k++) {
        int sx = (k * 97 + 13) % SCREEN_W, sy = LMP_HUD + (k * 61 + 7) % LMP_VIEW_H;
        gfx_pset(sx, sy, k % 9 == 0 ? C_LIGHT : C_DUSK);
    }
    gfx_clip(0, LMP_HUD, SCREEN_W, LMP_VIEW_H);
    int sx = lg.shake > 0 ? ((t & 2) ? 1 : -1) : 0;
    gfx_camera(cam_x + sx, cam_y);
    int x0 = imax(0, (cam_x) / LMP_T), x1 = imin(lg.mw - 1, (cam_x + SCREEN_W) / LMP_T + 1);
    int y0 = imax(0, (cam_y + LMP_HUD) / LMP_T), y1 = imin(lg.mh - 1, (cam_y + SCREEN_H) / LMP_T + 1);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++) draw_tile(tx, ty, lg.map[ty][tx], t);
    /* the exit pod */
    if (lg.exit_x >= 0 && lg.level != LV_HUB)
        draw_pod(lg.exit_x, lg.exit_y, lg.kind == LK_SECRET ? 4 : 1, t, "", lg.bulb_out != 0);
    /* the hub's pods and people */
    if (lg.level == LV_HUB) {
        int k = 0, np = 0;
        for (int ty = 0; ty < lg.mh; ty++)
            for (int tx = 0; tx < lg.mw; tx++) {
                char c = lg.map[ty][tx];
                if (c == '$' && k < LMP_HUB_NPODS) {
                    int lv = LMP_HUB_POD[k].level;
                    char lab[16];
                    int st = 0;
                    bool open = true;
                    if (lv < LMP_SECTORS) {
                        snprintf(lab, sizeof lab, lv == LMP_SECTORS - 1 ? "?" : "%d", lv + 1);
                        if ((lgs.cleared >> lv) & 1) st = 1;
                        if (lv < LMP_NORMAL_DECKS && ((lgs.honey >> lv) & 1)) st = 2;
                        if (lv == LMP_SECTORS - 1) st = 4;
                    } else {
                        snprintf(lab, sizeof lab, "%s", GLYPH_SKULL);
                        st = 3;
                        open = !(lv == LV_TURNCOAT ? lgs.turncoat : lgs.dread);
                    }
                    draw_pod(tx, ty, st, t, lab, open);
                    k++;
                } else if (c == 'N') {
                    int sp = SPR_NPC0 + (np % 5);
                    spr_draw(&lmp_spr[sp], tx * LMP_T + 1, ty * LMP_T - 2 + ((t / 30 + np) % 2), 0);
                    np++;
                }
            }
    }
    /* the hidden honey, and the bee that shows where it is once the Dreadnought is gone */
    if (lg.honey_x >= 0 && lg.honey_shown && !lg.honey_held)
        spr_draw(&lmp_spr[SPR_HONEY], lg.honey_x * LMP_T + 1, lg.honey_y * LMP_T + ((t / 12) % 2), 0);
    if (lgs.dread && lg.level < LMP_NORMAL_DECKS && lg.kind == LK_NORMAL) {
        for (int ty = 0; ty < lg.mh; ty++)
            for (int tx = 0; tx < lg.mw; tx++)
                if (lg.map[ty][tx] == '!') {
                    int ph = (t / 3) % 24;
                    int bx = tx * LMP_T + 2 + (ph < 12 ? ph : 24 - ph) - 6;
                    int by = ty * LMP_T - 6 + ((t / 5) % 3);
                    spr_draw(&lmp_spr[SPR_BEE], bx, by, ph < 12 ? 0 : SPR_FLIPX);
                }
    }
    /* foes */
    for (int i = 0; i < LMP_MAX_FOES; i++) {
        const LmpFoe *f = &lg.foe[i];
        if (!f->alive) continue;
        int fx = f->x / LMP_FX, fy = f->y / LMP_FX;
        int r = lmp_axis(f->n) ? f->n / 2 : 0;
        bool fl = f->flash > 0 && (t & 2);
        uint8_t white[PAL_COUNT];
        if (fl) for (int c = 0; c < PAL_COUNT; c++) white[c] = C_WHITE;
        const uint8_t *map = fl ? white : NULL;
        switch (f->kind) {
        case FO_TURRET: {
            const Sprite *s = &lmp_rot[SPR_TURRET][r];
            spr_draw_ex(s, fx - 5, fy - 5, 0, map, -1);
            if (f->state == 1 && (t & 2)) gfx_circ(fx + LMP_DX[f->n] * 4, fy + LMP_DY[f->n] * 4, 2, C_RED);
            break;
        }
        case FO_LASER: {
            spr_draw_ex(&lmp_rot[SPR_TURRET][r], fx - 5, fy - 5, 0, map, -1);
            gfx_circ(fx + LMP_DX[f->n] * 3, fy + LMP_DY[f->n] * 3, 2, f->state ? ((t & 2) ? C_WHITE : C_CYAN) : C_BLUE);
            break;
        }
        case FO_FLAME: {
            spr_draw(&lmp_rot[SPR_NOZZLE][r], fx - 5, fy - 5, 0);
            int reach = lmp_flame_reach(f);
            if (f->state == 1 && (t & 4)) gfx_circ(fx + LMP_DX[f->n] * 6, fy + LMP_DY[f->n] * 6, 2, C_ORANGE);
            for (int k = f->state == 2 ? 0 : 1; k <= reach; k++) {
                int cx = fx + LMP_DX[f->n] * k * LMP_T, cy = fy + LMP_DY[f->n] * k * LMP_T;
                int rr = 3 + ((t / 3 + k) % 2);
                gfx_circ(cx, cy, rr + 1, C_RED);
                gfx_circ(cx, cy, rr, C_ORANGE);
                gfx_circ(cx, cy, rr - 2, C_YELLOW);
            }
            break;
        }
        case FO_DRONE:
            spr_draw_ex(&lmp_spr[SPR_DRONE0], fx - 5, fy - 4, 0, map, -1);
            break;
        case FO_EYE:
            spr_draw_ex(&lmp_spr[(t / 40) % 6 == 0 ? SPR_EYE1 : SPR_EYE0], fx - 4, fy - 4, f->dir < 0 ? SPR_FLIPX : 0, map, -1);
            break;
        case FO_CRAWLER:
            spr_draw_ex(&lmp_rot[(f->a / 6) % 2 ? SPR_CRAWLER1 : SPR_CRAWLER0][r], fx - 5, fy - 5, 0, map, -1);
            break;
        case FO_SAC:
            spr_draw_ex(&lmp_rot[(f->a / 20) % 2 ? SPR_SAC1 : SPR_SAC0][r], fx - 5, fy - 5, 0, map, -1);
            break;
        case FO_SPECK:
            spr_draw_ex(&lmp_spr[(f->a / 4) % 2 ? SPR_SPECK1 : SPR_SPECK0], fx - 3, fy - 3, 0, map, -1);
            break;
        case FO_WORM: case FO_SWITCHWORM:
            for (int k = 23; k >= 3; k -= 4) spr_draw(&lmp_spr[SPR_WORM_BODY], f->trail[k][0] - 4, f->trail[k][1] - 4, 0);
            spr_draw_ex(&lmp_spr[SPR_WORM_HEAD], fx - 4, fy - 4, f->vx < 0 ? SPR_FLIPX : 0, map, -1);
            if (f->kind == FO_SWITCHWORM) {
                gfx_rect(fx - 2, fy - 7, 2, 2, lg.parity == 0 ? C_JADE : C_DUSK);
                gfx_rect(fx + 1, fy - 7, 2, 2, lg.parity == 1 ? C_PINK : C_DUSK);
            }
            break;
        case FO_BALL:
            spr_draw(&lmp_spr[SPR_BALL], fx - 4, fy - 4, (f->a / 6) % 2 ? SPR_FLIPX : 0);
            break;
        case FO_TURNCOAT: {
            const Sprite *s = &lmp_tank_spr[4][f->n & 7];
            spr_draw_ex(s, fx - 5, fy - 5, 0, map, -1);
            if (f->state == 1 && (t & 4)) gfx_circb(fx, fy, 7, C_RED);
            break;
        }
        case FO_DREAD: {
            int bx0 = (f->cx - 6) * LMP_T, by0 = f->cy * LMP_T, w = 13 * LMP_T, h = 5 * LMP_T;
            int body = f->flash > 0 && (t & 2) ? C_WHITE : C_WINE;
            gfx_rect(bx0, by0, w, h, C_INK);
            gfx_rect(bx0 + 2, by0, w - 4, h - 2, body);
            gfx_rect(bx0 + 6, by0 + 4, w - 12, h - 12, PAL_DARKER[body]);
            for (int k = 0; k < 13; k++) gfx_rect(bx0 + k * LMP_T + 2, by0 + h - 6, 6, 4, C_GREY);
            int mx = f->cx * LMP_T;
            gfx_rect(mx - 2, by0 + h, 14, 3 * LMP_T, C_INK);
            gfx_rect(mx, by0 + h, 10, 3 * LMP_T - 2, C_GREY);
            gfx_rect(mx + 2, by0 + h, 6, 3 * LMP_T - 2, C_NIGHT);
            int core = f->state == 2 ? ((t & 4) ? C_YELLOW : C_ORANGE) : f->state == 1 ? C_RED : C_MAROON;
            gfx_circ(mx + 5, by0 + h - 4, 5, core);
            gfx_rect(bx0 + 20, by0 + 10, 8, 6, C_RED);
            gfx_rect(bx0 + w - 28, by0 + 10, 8, 6, C_RED);
            if (f->state == 1 && (t & 2)) gfx_circ(mx + 5, by0 + h + 3 * LMP_T, 3, C_ORANGE);
            break;
        }
        default: break;
        }
    }
    /* shots */
    for (int i = 0; i < LMP_MAX_SHOTS; i++) {
        const LmpShot *s = &lg.shot[i];
        if (!s->alive) continue;
        int px = s->x / LMP_FX, py = s->y / LMP_FX;
        gfx_rect(px - 1, py - 1, 3, 3, C_YELLOW);
        gfx_pset(px, py, C_WHITE);
    }
    for (int i = 0; i < LMP_MAX_ESHOTS; i++) {
        const LmpEShot *e = &lg.es[i];
        if (!e->alive) continue;
        int px = e->x / LMP_FX, py = e->y / LMP_FX;
        switch (e->kind) {
        case ES_BULLET: gfx_circ(px, py, 2, C_RED); gfx_pset(px, py, C_YELLOW); break;
        case ES_LASER:
            if (e->vx) gfx_hline(px - 5, px + 5, py, (t & 2) ? C_CYAN : C_WHITE);
            else gfx_vline(px, py - 5, py + 5, (t & 2) ? C_CYAN : C_WHITE);
            break;
        case ES_BEAM: gfx_rect(px - 2, py - 2, 4, 4, C_VIOLET); gfx_pset(px, py, C_WHITE); break;
        case ES_RING: gfx_circb(px, py, 3 + (e->t / 6) % 2, C_SKY); break;
        case ES_SHELL: gfx_circ(px, py, 4, C_ORANGE); gfx_circ(px, py, 2, C_YELLOW); break;
        case ES_BOLT: gfx_circ(px, py, 2, (t & 2) ? C_YELLOW : C_ORANGE); break;
        default: break;
        }
    }
    /* the tank */
    if (lg.state != GS_DEAD && lg.state != GS_PROMPT) {
        int tx, ty;
        lmp_tank_centre(&tx, &ty);
        const LmpTank *tk = &lg.tk;
        int n = tk->n;
        if (tk->pn > 0) {
            /* the slope's picture halfway through a ramp */
            int at = tk->pt, tot = 0;
            for (int i = 0; i < tk->pn; i++) {
                int d = (i == 0 ? !lmp_axis(tk->sn) : !lmp_axis(tk->pnn[i - 1])) || !lmp_axis(tk->pnn[i]) ? LMP_RAMP_T : LMP_STEP_T;
                if (at < tot + d) { n = at < tot + d / 2 ? (i == 0 ? tk->sn : tk->pnn[i - 1]) : tk->pnn[i]; break; }
                tot += d;
            }
        }
        if (tk->warp_fx_t > 0) {
            int k = tk->warp_fx_t;
            gfx_line(tk->warp_from_x, tk->warp_from_y, tx, ty, k > 6 ? C_WHITE : C_CYAN);
            gfx_circb(tk->warp_from_x, tk->warp_from_y, 14 - k, C_CYAN);
        }
        spr_draw(&lmp_tank_spr[iclamp(tk->colour, 0, 3)][n & 7], tx - 5, ty - 5, 0);
        if (tk->fizz_t > 0 && (t & 2)) gfx_circb(tx + LMP_DX[n] * 6, ty + LMP_DY[n] * 6, 3, C_GREY);
        if (tk->blast_t > 0) {
            int td = (tk->blast_n + 2) & 7;
            int bx = tk->blast_x * LMP_T + 5, by = tk->blast_y * LMP_T + 5;
            for (int k = -2; k <= 2; k++) {
                if (!k) continue;
                int r = 2 + (tk->blast_t / 3) % 3;
                gfx_circ(bx + k * LMP_DX[td] * LMP_T, by + k * LMP_DY[td] * LMP_T, r, iabs(k) == 1 ? C_YELLOW : C_ORANGE);
            }
        }
        if (lg.honey_held) spr_draw(&lmp_spr[SPR_BEE], tx - 2 + ((t / 4) % 3) - 1, ty - 12, 0);
    }
    /* sparks */
    for (int i = 0; i < LMP_MAX_PARTS; i++) {
        const LmpPart *p = &lg.part[i];
        if (p->life > 0) gfx_pset(p->x / LMP_FX, p->y / LMP_FX, p->col);
    }
    gfx_camera(0, 0);
    gfx_noclip();
}

static void time_str(char *buf, int n, int frames) {
    int cs = frames * 100 / 60;
    snprintf(buf, (size_t)n, "%d:%02d.%02d", cs / 6000, (cs / 100) % 60, cs % 100);
}

static void draw_hud(void) {
    gfx_rect(0, 0, SCREEN_W, LMP_HUD, C_INK);
    gfx_hline(0, SCREEN_W - 1, LMP_HUD - 1, C_DUSK);
    char buf[64];
    if (lg.level == LV_HUB) {
        extern int lmp_hub_zone(void);
        tiny_draw(LMP_ZONE_NAME[lmp_hub_zone()], 4, 3, C_CYAN);
        snprintf(buf, sizeof buf, "DECKS %d/%d   HONEY %d/%d", lmp_cleared_count(), LMP_NORMAL_DECKS, lmp_honey_count(),
                 LMP_HONEY_TOTAL);
        tiny_draw(buf, SCREEN_W - 4 - tiny_width(buf), 3, C_LIGHT);
        return;
    }
    int x = 4;
    if (lg.honey_held) { spr_draw(&lmp_spr[SPR_HONEY], 2, 0, 0); x = 13; }
    const LmpLevel *L = LMP_LEVEL[lg.level];
    if (lg.level < LMP_SECTORS) snprintf(buf, sizeof buf, "DECK %d  %s", lg.level + 1, L->name);
    else snprintf(buf, sizeof buf, "%s", L->name);
    tiny_draw(buf, x, 3, C_CYAN);
    if (lg.kind == LK_SURVIVAL && !lg.bulb_out) {
        snprintf(buf, sizeof buf, "WAVE %d/%d", imax(1, lg.wave), L->nwaves);
        tiny_draw(buf, 190, 3, C_PINK);
    }
    if (lg.kind == LK_TURNCOAT || lg.kind == LK_DREAD) {
        for (int i = 0; i < LMP_MAX_FOES; i++)
            if (lg.foe[i].alive && (lg.foe[i].kind == FO_TURNCOAT || lg.foe[i].kind == FO_DREAD)) {
                int mx = LMP_FOE_HP[lg.foe[i].kind];
                gfx_rect(170, 3, 72, 4, C_MAROON);
                gfx_rect(170, 3, 72 * lg.foe[i].hp / mx, 4, C_RED);
            }
    }
    time_str(buf, sizeof buf, lg.timer);
    tiny_draw(buf, SCREEN_W - 4 - tiny_width(buf), 3, C_WHITE);
}

static void draw_talk(void) {
    if (lg.level != LV_HUB || lg.talk_npc < 0 || lg.talk_npc >= LMP_HUB_NNPCS) return;
    const LmpNpc *n = &LMP_HUB_NPC[lg.talk_npc];
    int h = 14;
    for (int i = 0; i < 4 && n->lines[i]; i++) h += 8;
    int y = SCREEN_H - h - 4;
    ui_panel(8, y, SCREEN_W - 16, h, C_INK, C_CYAN);
    tiny_draw(n->name, 14, y + 4, C_YELLOW);
    for (int i = 0; i < 4 && n->lines[i]; i++) tiny_draw(n->lines[i], 14, y + 12 + i * 8, C_WHITE);
}

static void draw_pod_hint(void) {
    if (lg.level != LV_HUB || lg.pod_here < 0) return;
    int lv = LMP_HUB_POD[lg.pod_here].level;
    char buf[64];
    if (lv < LMP_SECTORS) {
        char tb[16] = "";
        if (lgs.best[lv]) time_str(tb, sizeof tb, (int)lgs.best[lv]);
        snprintf(buf, sizeof buf, "DECK %d %s %s", lv + 1, LMP_LEVEL[lv]->name, tb);
    } else {
        snprintf(buf, sizeof buf, "%s", LMP_LEVEL[lv]->name);
    }
    int w = tiny_width(buf) + 26;
    ui_panel(SCREEN_W / 2 - w / 2, LMP_HUD + 4, w, 12, C_INK, C_AMBER);
    tiny_draw(GLYPH_B, SCREEN_W / 2 - w / 2 + 5, LMP_HUD + 7, C_YELLOW);
    tiny_draw(buf, SCREEN_W / 2 - w / 2 + 16, LMP_HUD + 7, C_WHITE);
}

/* ---- screens ------------------------------------------------------------------ */

static void draw_title(void) {
    int t = lg.frame_t;
    ui_starfield(t, C_INK);
    /* a strip of hull with Limpet rolling along it */
    gfx_rect(0, 128, SCREEN_W, 12, C_SLATE);
    gfx_hline(0, SCREEN_W - 1, 128, C_GREY);
    int x = (t / 2) % (SCREEN_W + 40) - 20;
    spr_draw(&lmp_tank_spr[0][DR_U], x, 118, 0);
    gfx_rect(0, 40, SCREEN_W, 10, C_SLATE);
    gfx_hline(0, SCREEN_W - 1, 49, C_GREY);
    spr_draw(&lmp_tank_spr[0][DR_D], SCREEN_W - x, 50, 0);
    static const uint8_t grad[3] = {C_CYAN, C_TEAL, C_FOREST};
    ui_fancy_center("LIMPET", SCREEN_W / 2, 12, 3, grad, 3, C_INK, C_NAVY);
    int y = 70;
    if (lgs.started) {
        text_center("CONTINUE", SCREEN_W / 2, y, lg.menu == 0 ? C_YELLOW : C_GREY);
        if (lg.menu == 0) ui_cursor(SCREEN_W / 2 - text_width("CONTINUE") / 2 - 10, y, t);
        y += 12;
    }
    text_center("NEW GAME", SCREEN_W / 2, y, lg.menu == 1 ? C_YELLOW : C_GREY);
    if (lg.menu == 1) ui_cursor(SCREEN_W / 2 - text_width("NEW GAME") / 2 - 10, y, t);
    char buf[64];
    if (lgs.started) {
        uint32_t s = 0;
        for (int i = 0; i < LMP_SECTORS; i++) s += lgs.best[i];
        char tb[16];
        time_str(tb, sizeof tb, (int)s);
        snprintf(buf, sizeof buf, "DECKS %d/%d  HONEY %d/%d  BEST TIMES %s", lmp_cleared_count(), LMP_NORMAL_DECKS,
                 lmp_honey_count(), LMP_HONEY_TOTAL, tb);
        tiny_center(buf, SCREEN_W / 2, 150, C_LIGHT);
    }
    tiny_center("BEAMDOWN SOFTWORKS 1985", SCREEN_W / 2, 166, C_DUSK);
    if (lg.talk_t > 0) ui_confirm_box("START A NEW GAME?", "THE STATION LOG WILL BE WIPED", "YES", lg.talk_npc, C_RED, NULL, t);
}

static void draw_lines(const char *const *lines, int n, int y0) {
    for (int i = 0; i < n; i++)
        if (lines[i][0]) text_center(lines[i], SCREEN_W / 2, y0 + i * 11, C_WHITE);
}

static void draw_story(void) {
    gfx_cls(C_INK);
    ui_starfield(lg.frame_t, C_INK);
    int shown = imin(LMP_STORY_N, lg.state_t / 40 + 1);
    draw_lines(LMP_STORY, shown, 30);
    spr_draw_scaled(&lmp_tank_spr[0][DR_U], SCREEN_W / 2 - 10, 140, 2, 0);
}

static void draw_prompt(void) {
    gfx_darken_rect(0, LMP_HUD, SCREEN_W, LMP_VIEW_H, 2);
    ui_panel(80, 60, 160, 54, C_INK, C_RED);
    text_center("SCRAPPED", SCREEN_W / 2, 66, C_RED);
    text_center("TRY AGAIN", SCREEN_W / 2, 82, lg.menu == 0 ? C_YELLOW : C_GREY);
    text_center("BACK TO THE STATION", SCREEN_W / 2, 96, lg.menu == 1 ? C_YELLOW : C_GREY);
    ui_cursor(SCREEN_W / 2 - text_width(lg.menu == 0 ? "TRY AGAIN" : "BACK TO THE STATION") / 2 - 10, lg.menu == 0 ? 82 : 96,
              lg.frame_t);
}

static void draw_clear(void) {
    gfx_darken_rect(0, LMP_HUD, SCREEN_W, LMP_VIEW_H, 2);
    ui_panel(60, 48, 200, 84, C_INK, C_LEAF);
    const LmpLevel *L = LMP_LEVEL[lg.clear_level];
    char buf[64], tb[16];
    if (lg.clear_level < LMP_SECTORS) snprintf(buf, sizeof buf, "DECK %d  %s", lg.clear_level + 1, L->name);
    else snprintf(buf, sizeof buf, "%s", L->name);
    text_center(buf, SCREEN_W / 2, 54, C_CYAN);
    text_center(lg.clear_level >= LMP_SECTORS ? "SCRAPPED!" : "POD SHUT", SCREEN_W / 2, 68, C_YELLOW);
    time_str(tb, sizeof tb, lg.clear_time);
    snprintf(buf, sizeof buf, "TIME %s", tb);
    text_center(buf, SCREEN_W / 2, 84, C_WHITE);
    if (lg.clear_level < LMP_SECTORS) {
        time_str(tb, sizeof tb, lg.clear_best);
        snprintf(buf, sizeof buf, "BEST %s", tb);
        text_center(buf, SCREEN_W / 2, 96, C_LIGHT);
    }
    if (lg.clear_honey) {
        spr_draw(&lmp_spr[SPR_HONEY], SCREEN_W / 2 - 50, 109, 0);
        text_center("HONEY BANKED", SCREEN_W / 2 + 6, 110, C_AMBER);
    }
}

static void draw_ending(const char *const *lines, int n) {
    int t = lg.frame_t;
    gfx_cls(C_INK);
    ui_starfield(t, C_INK);
    /* the Earth coming up under the station */
    gfx_circ(SCREEN_W / 2, SCREEN_H + 120, 170, C_NAVY);
    gfx_circ(SCREEN_W / 2, SCREEN_H + 120, 166, C_BLUE);
    gfx_circ(SCREEN_W / 2 - 60, SCREEN_H + 30, 30, C_FOREST);
    gfx_rect(SCREEN_W / 2 - 60, 130, 120, 8, C_SLATE);
    gfx_hline(SCREEN_W / 2 - 60, SCREEN_W / 2 + 59, 130, C_GREY);
    spr_draw(&lmp_tank_spr[iclamp(lgs.colour, 0, 3)][DR_U], SCREEN_W / 2 - 5, 120, 0);
    if (lg.state == GS_CHERRY)
        for (int k = 0; k < 6; k++)
            spr_draw(&lmp_spr[SPR_BEE], SCREEN_W / 2 - 40 + k * 15 + (t / 5 + k) % 3, 104 + ((t / 7 + k * 3) % 5), 0);
    int shown = imin(n, lg.state_t / 60 + 1);
    draw_lines(lines, shown, 14);
}

int lmp_hub_zone(void);

void lmp_draw(void) {
    switch (lg.state) {
    case GS_TITLE: draw_title(); return;
    case GS_STORY: draw_story(); return;
    case GS_ENDING: draw_ending(LMP_ENDING, LMP_ENDING_N); return;
    case GS_CHERRY: draw_ending(LMP_CHERRY_END, LMP_CHERRY_END_N); return;
    default: break;
    }
    gfx_cls(C_INK);
    update_camera();
    draw_world();
    draw_hud();
    if (lg.state == GS_PLAY) { draw_talk(); draw_pod_hint(); }
    if (lg.msg_t > 0) text_center(lg.msg, SCREEN_W / 2, SCREEN_H - 14, C_YELLOW);
    if (lg.state == GS_PROMPT) draw_prompt();
    if (lg.state == GS_CLEAR) draw_clear();
}

void lmp_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_INK);
    for (int k = 0; k < 24; k++) gfx_pset(x + (k * 37) % w, y + (k * 23) % h, C_DUSK);
    /* a hull room: floor, wall and ceiling, Limpet climbing round */
    gfx_rect(x, y + h - 8, w, 8, C_SLATE);
    gfx_hline(x, x + w - 1, y + h - 8, C_GREY);
    gfx_rect(x + w - 10, y + 16, 10, h - 24, C_SLATE);
    gfx_vline(x + w - 10, y + 16, y + h - 9, C_GREY);
    int k = (t / 4) % 60;
    if (k < 30) spr_draw(&lmp_tank_spr[0][DR_U], x + 10 + k * 2, y + h - 18, 0);
    else spr_draw(&lmp_tank_spr[0][DR_L], x + w - 20, y + h - 18 - (k - 30) * 1, 0);
    spr_draw(&lmp_spr[SPR_HONEY], x + 8, y + h - 34, 0);
    static const uint8_t grad[2] = {C_CYAN, C_TEAL};
    ui_fancy_text("LIMPET", x + 6, y + 4, 1, grad, 2, C_INK, C_NAVY);
    gfx_noclip();
}

/* tests: the screens' words stay inside the screen and clear of each other */
int lmp_layout_audit(void) {
    int bad = 0;
    LmpGame keep = lg;
    static const int SCREENS[] = {GS_TITLE, GS_STORY, GS_ENDING, GS_CHERRY};
    for (int i = 0; i < ARRAY_LEN(SCREENS); i++) {
        lg.state = SCREENS[i];
        lg.state_t = 3000;
        ui_audit_begin("limpet", true);
        ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
        lmp_draw();
        const char *const *lines = SCREENS[i] == GS_STORY ? LMP_STORY : SCREENS[i] == GS_ENDING ? LMP_ENDING : LMP_CHERRY_END;
        int n = SCREENS[i] == GS_STORY ? LMP_STORY_N : SCREENS[i] == GS_ENDING ? LMP_ENDING_N : LMP_CHERRY_END_N;
        int y0 = SCREENS[i] == GS_STORY ? 30 : 14;
        if (SCREENS[i] != GS_TITLE)
            for (int k = 0; k < n; k++)
                if (lines[k][0]) ui_audit_text("line", lines[k], SCREEN_W / 2 - text_width(lines[k]) / 2, y0 + k * 11);
        for (int k = 0; k < n; k++) bad += text_missing(lines[k]) > 0;
        bad += ui_audit_end();
    }
    /* the people's words fit their box */
    for (int i = 0; i < LMP_HUB_NNPCS; i++)
        for (int k = 0; k < 4 && LMP_HUB_NPC[i].lines[k]; k++) {
            if (tiny_width(LMP_HUB_NPC[i].lines[k]) > SCREEN_W - 28) { bad++; fprintf(stderr, "npc %d line %d too wide\n", i, k); }
            bad += tiny_missing(LMP_HUB_NPC[i].lines[k]) > 0;
        }
    for (int lv = 0; lv < LMP_LEVELS; lv++) bad += tiny_missing(LMP_LEVEL[lv]->name) > 0;
    lg = keep;
    return bad;
}
