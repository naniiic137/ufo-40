/* FORLORN HOPE - drawing the map, the troop's door, what the volunteers
 * left behind, the foes and the shots. The HUD and the screens are in
 * forlorn.c. */
#include "forlorn.h"

static unsigned hash(int x, int y) {
    unsigned h = (unsigned)(x * 374761393 + y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

enum { BD_SKY, BD_CAVE, BD_CASTLE };
static int backdrop(int tx, int ty) {
    if (ty < 2) return BD_SKY;
    if (tx >= 74 && ty < 60) return BD_CASTLE;
    if (ty < 20 && tx < 74) return BD_SKY;
    return BD_CAVE;
}

static const uint8_t PLATE_COL[FRL_PLATES] = {C_TEAL, C_AMBER, C_VIOLET, C_ORANGE};

static bool open_at(const FrlWorld *w, int tx, int ty) {
    if (tx < 0 || tx >= FRL_MW || ty < 0 || ty >= FRL_MH) return false;
    return !frl_solid(w, tx, ty);
}

static void draw_back(int tx, int ty, int px, int py, int t) {
    switch (backdrop(tx, ty)) {
    case BD_SKY: {
        /* dusk over Holloway: night above, a low red glow on the horizon */
        static const uint8_t BAND[6] = {C_INK, C_NIGHT, C_NAVY, C_DUSK, C_PURPLE, C_WINE};
        int yy = ty * FRL_T;
        int b = iclamp(yy / 36, 0, 5);
        gfx_rect(px, py, FRL_T, FRL_T, BAND[b]);
        int edge = (b + 1) * 36 - yy; /* dither into the next band */
        if (b < 5 && edge < 10) gfx_dither(px, py + edge, FRL_T, 10 - edge, BAND[b + 1], 8);
        unsigned h = hash(tx, ty);
        if (ty < 12 && (h & 15) == 0) {
            int sx = px + (int)(h >> 8) % 10, sy = py + (int)(h >> 12) % 10;
            gfx_pset(sx, sy, ((h >> 16) & 3) == 0 && (t / 20 + (int)h) % 7 == 0 ? C_WHITE : C_GREY);
        }
        break;
    }
    case BD_CAVE: {
        gfx_rect(px, py, FRL_T, FRL_T, C_INK);
        unsigned h = hash(tx, ty);
        if ((h & 7) == 0) gfx_pset(px + (int)(h >> 8) % 10, py + (int)(h >> 12) % 10, C_NIGHT);
        if ((h & 31) == 1) gfx_rect(px + (int)(h >> 8) % 7, py + (int)(h >> 12) % 6, 3, 4, C_NIGHT);
        break;
    }
    default: {
        gfx_rect(px, py, FRL_T, FRL_T, C_NIGHT);
        int off = (ty & 1) ? 5 : 0;
        gfx_hline(px, px + 9, py + 9, C_INK);
        gfx_vline(px + off, py, py + 8, C_INK);
        unsigned h = hash(tx, ty);
        if ((h & 15) == 0) gfx_pset(px + 3, py + 3, C_DUSK);
        /* the heart chamber's walls are overgrown with thorns */
        if (tx >= 124 && ty >= 10 && (h & 3) == 0) {
            gfx_line(px, py + (int)(h >> 4) % 10, px + 9, py + (int)(h >> 8) % 10, C_MAROON);
            gfx_pset(px + (int)(h >> 12) % 10, py + (int)(h >> 16) % 10, C_WINE);
        }
        break;
    }
    }
}

/* far off over the meadow: a moon, low hills, and Thornkeep's towers */
static void draw_far(int cx, int t) {
    int par = cx / 3; /* the far layer drifts slower than the map */
    gfx_clip(0, FRL_OY, imax(0, 740 - cx), FRL_VH);
    int mx = cx + 230 - par / 2, my = 36;
    gfx_circ(mx, my, 9, C_CREAM);
    gfx_circ(mx + 4, my - 2, 8, C_PURPLE);
    for (int x = cx - 10; x < cx + SCREEN_W + 10; x += 2) {
        int wx = x - cx + par;
        int hh = 150 + (int)(10.0f * sinf((float)wx * 0.021f) + 6.0f * sinf((float)wx * 0.057f + 1.3f));
        gfx_rect(x, hh, 2, 200 - hh, C_NIGHT);
    }
    int kx = cx + 520 - par;
    gfx_rect(kx, 112, 34, 40, C_INK);
    gfx_rect(kx - 8, 100, 10, 52, C_INK);
    gfx_rect(kx + 30, 96, 10, 56, C_INK);
    gfx_rect(kx + 12, 86, 10, 66, C_INK);
    gfx_pset(kx + 16, 96, (t / 6) % 8 < 2 ? C_RED : C_WINE);
    gfx_pset(kx - 4, 108, C_AMBER);
    gfx_noclip();
    gfx_clip(0, FRL_OY, SCREEN_W, FRL_VH);
}

static void draw_rock(const FrlWorld *w, int tx, int ty, int px, int py, int base, int dark, int light) {
    gfx_rect(px, py, FRL_T, FRL_T, base);
    unsigned h = hash(tx, ty);
    for (int k = 0; k < 3; k++) {
        unsigned g = h >> (k * 8);
        gfx_pset(px + (int)(g % 9), py + (int)((g >> 4) % 9), k == 2 ? light : dark);
    }
    if (open_at(w, tx, ty - 1)) {
        if (backdrop(tx, ty - 1) == BD_SKY && frl_tile(w, tx, ty - 1) != FT_SPIKES) {
            gfx_rect(px, py, FRL_T, 3, C_FOREST);
            gfx_hline(px, px + 9, py, C_LEAF);
            if (h & 1) gfx_pset(px + (int)(h >> 3) % 9, py - 1, C_LEAF);
        } else {
            gfx_hline(px, px + 9, py, light);
        }
    }
    if (open_at(w, tx, ty + 1)) {
        gfx_hline(px, px + 9, py + 9, C_INK);
        if (backdrop(tx, ty + 1) == BD_CAVE && frl_tile(w, tx, ty + 1) == FT_AIR && (h & 3) == 1) {
            int sx = px + 2 + (int)(h >> 5) % 6;
            gfx_vline(sx, py + 10, py + 11 + (int)(h >> 9) % 3, base);
            gfx_pset(sx, py + 13 + (int)(h >> 9) % 3, dark);
        }
    }
    if (open_at(w, tx, ty - 1) && backdrop(tx, ty - 1) == BD_CAVE && frl_tile(w, tx, ty - 1) == FT_AIR && (h & 15) == 2) {
        int sx = px + 2 + (int)(h >> 6) % 6;
        gfx_vline(sx, py - 2, py - 1, C_TAN);
        gfx_hline(sx - 1, sx + 1, py - 3, C_TEAL);
        gfx_pset(sx, py - 4, C_CYAN);
    }
    if (open_at(w, tx - 1, ty)) gfx_vline(px, py, py + 9, dark);
    if (open_at(w, tx + 1, ty)) gfx_vline(px + 9, py, py + 9, C_INK);
}

static void draw_brick(const FrlWorld *w, int tx, int ty, int px, int py, bool seal) {
    gfx_rect(px, py, FRL_T, FRL_T, C_SLATE);
    gfx_hline(px, px + 9, py + 4, C_DUSK);
    gfx_hline(px, px + 9, py + 9, C_DUSK);
    int off = (ty & 1) ? 5 : 0;
    gfx_vline(px + off, py, py + 3, C_DUSK);
    gfx_vline(px + (off + 5) % 10, py + 5, py + 8, C_DUSK);
    if (open_at(w, tx, ty - 1)) gfx_hline(px, px + 9, py, C_GREY);
    if (seal) gfx_pset(px + 7, py + 2, C_GREY); /* a hairline crack, if you look */
}

static void draw_tile(const FrlWorld *w, int tx, int ty, int px, int py, int t) {
    int k = w->tile[ty][tx];
    switch (k) {
    case FT_ROCK: draw_rock(w, tx, ty, px, py, C_DUSK, C_NIGHT, C_SLATE); break;
    case FT_BRICK: draw_brick(w, tx, ty, px, py, false); break;
    case FT_SEAL: draw_brick(w, tx, ty, px, py, true); break;
    case FT_LOOSE:
        draw_rock(w, tx, ty, px, py, C_EARTH, C_BROWN, C_TAN);
        gfx_line(px + 2, py + 1, px + 5, py + 5, C_INK);
        gfx_line(px + 5, py + 5, px + 3, py + 8, C_INK);
        gfx_line(px + 5, py + 5, px + 8, py + 6, C_INK);
        break;
    case FT_WOOD:
        gfx_rect(px, py + 1, FRL_T, 7, C_BROWN);
        gfx_hline(px, px + 9, py + 1, C_TAN);
        gfx_hline(px, px + 9, py + 7, C_EARTH);
        if (hash(tx, ty) & 1) gfx_hline(px + 2, px + 5, py + 4, C_EARTH);
        else gfx_pset(px + 6, py + 3, C_EARTH);
        if (!frl_solid(w, tx - 1, ty)) gfx_vline(px, py + 2, py + 6, C_EARTH);
        if (!frl_solid(w, tx + 1, ty)) gfx_vline(px + 9, py + 2, py + 6, C_EARTH);
        break;
    case FT_LEAVES: {
        unsigned h = hash(tx, ty);
        gfx_dither(px, py, FRL_T, FRL_T, C_FOREST, 8 + (int)(h & 3));
        gfx_pset(px + (int)(h >> 4) % 9, py + (int)(h >> 8) % 9, C_JADE);
        gfx_pset(px + (int)(h >> 12) % 9, py + (int)(h >> 16) % 9, C_LEAF);
        break;
    }
    case FT_TRUNK:
        gfx_rect(px, py, FRL_T, FRL_T, C_EARTH);
        gfx_vline(px + ((tx & 1) ? 2 : 6), py, py + 9, C_BROWN);
        if ((hash(tx, ty) & 3) == 0) gfx_pset(px + 4, py + 5, C_INK);
        break;
    case FT_SPIKES:
        for (int i = 0; i < 3; i++) {
            int sx = px + 1 + i * 3;
            gfx_vline(sx + 1, py + 4, py + 9, C_LIGHT);
            gfx_vline(sx, py + 7, py + 9, C_GREY);
            gfx_vline(sx + 2, py + 7, py + 9, C_GREY);
        }
        gfx_hline(px, px + 9, py + 9, C_SLATE);
        break;
    case FT_STONE:
        /* a mason's stone: a little statue's face is cut in it */
        gfx_rect(px, py, FRL_T, FRL_T, C_GREY);
        gfx_hline(px, px + 9, py, C_LIGHT);
        gfx_vline(px, py, py + 9, C_LIGHT);
        gfx_hline(px, px + 9, py + 9, C_SLATE);
        gfx_vline(px + 9, py, py + 9, C_SLATE);
        gfx_pset(px + 3, py + 4, C_SLATE);
        gfx_pset(px + 6, py + 4, C_SLATE);
        gfx_hline(px + 4, px + 5, py + 7, C_SLATE);
        break;
    case FT_DOOR: {
        bool top = frl_tile(w, tx, ty - 1) != FT_DOOR;
        gfx_rect(px, py, FRL_T, FRL_T, C_BROWN);
        gfx_vline(px + 3, py, py + 9, C_EARTH);
        gfx_vline(px + 6, py, py + 9, C_EARTH);
        gfx_hline(px, px + 9, py + (top ? 2 : 6), C_GREY);
        if (top) gfx_hline(px, px + 9, py, C_INK);
        else { gfx_rect(px + 4, py + 2, 2, 3, C_INK); gfx_pset(px + 4, py + 1, C_YELLOW); }
        gfx_vline(px, py, py + 9, C_INK);
        gfx_vline(px + 9, py, py + 9, C_INK);
        break;
    }
    case FT_DOOR_OPEN:
        gfx_vline(px, py, py + 9, C_EARTH);
        gfx_vline(px + 9, py, py + 9, C_EARTH);
        if (frl_tile(w, tx, ty - 1) != FT_DOOR_OPEN) gfx_hline(px, px + 9, py, C_EARTH);
        break;
    case FT_BLOCK1: case FT_BLOCK2: case FT_BLOCK3: case FT_BLOCK4: {
        int pi = k - FT_BLOCK1, c = PLATE_COL[pi];
        if (w->plate[pi].down) {
            gfx_rect(px, py, FRL_T, FRL_T, c);
            gfx_rectb(px, py, FRL_T, FRL_T, C_INK);
            gfx_hline(px + 1, px + 8, py + 1, C_WHITE);
            gfx_rect(px + 3, py + 3, 4, 4, PAL_DARKER[c]);
        } else {
            /* where it will be: a faint dotted outline */
            for (int i = 0; i < 10; i += 3) {
                gfx_pset(px + i, py, c);
                gfx_pset(px + i, py + 9, c);
                gfx_pset(px, py + i, c);
                gfx_pset(px + 9, py + i, c);
            }
        }
        break;
    }
    case FT_COMB:
        gfx_rect(px, py, FRL_T, FRL_T, C_NAVY);
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) gfx_rect(px + 1 + i * 3 + (j & 1), py + 1 + j * 3, 2, 2, (t / 20 + i + j) % 4 ? C_BLUE : C_SKY);
        break;
    case FT_GULP:
        gfx_rect(px, py, FRL_T, FRL_T, C_MAROON);
        gfx_hline(px, px + 9, py + ((ty + t / 30) & 1 ? 2 : 7), C_RED);
        gfx_vline((tx & 1) ? px + 9 : px, py, py + 9, C_INK);
        break;
    default: break;
    }
}

static void draw_base(const FrlWorld *w, int t) {
    int x = w->base_x, y = w->base_y;
    /* the troop's hut: timber walls, a roof, the door */
    gfx_rect(x - 22, y - 30, 44, 30, C_EARTH);
    for (int k = 0; k < 5; k++) gfx_hline(x - 22, x + 21, y - 28 + k * 6, C_BROWN);
    for (int k = 0; k < 22; k++) gfx_hline(x - 26 + k, x + 25 - k, y - 31 - k / 2, k & 2 ? C_MAROON : C_WINE);
    gfx_rect(x - 6, y - 16, 12, 16, C_INK);
    gfx_rectb(x - 7, y - 17, 14, 17, C_BROWN);
    if (w->phase == FWP_SELECT && (t / 20) & 1) gfx_rect(x - 5, y - 15, 10, 15, C_NIGHT);
    /* the counter over the door: volunteers left */
    gfx_rect(x - 10, y - 28, 20, 9, C_INK);
    gfx_rectb(x - 11, y - 29, 22, 11, C_TAN);
    char b[8];
    snprintf(b, sizeof b, "%02d", w->lives);
    text_center(b, x, y - 27, w->lives <= 10 ? C_RED : C_CREAM);
    /* a lantern */
    gfx_pset(x + 14, y - 12, (t / 12) & 1 ? C_YELLOW : C_AMBER);
    gfx_pset(x + 14, y - 11, C_ORANGE);
    /* the waystone pad: lit once there is a waystone, red if a foe waits by it */
    int px = w->pad_x, py = w->pad_y;
    int c = !w->way_on ? C_SLATE : frl_way_red(w) ? C_RED : ((t / 8) & 1 ? C_CYAN : C_SKY);
    gfx_hline(px - 5, px + 5, py - 1, c);
    gfx_hline(px - 3, px + 3, py - 2, c);
    gfx_rect(px - 1, py - 12, 3, 9, C_GREY);
    gfx_pset(px, py - 9, c);
    gfx_pset(px, py - 6, c);
}

static void draw_waystone(const FrlWorld *w, int t) {
    if (!w->way_on) return;
    int x = w->way_x, y = w->way_y;
    bool red = frl_way_red(w);
    int c = red ? C_RED : C_CYAN, c2 = red ? C_ORANGE : C_ICE;
    int r = 5 + ((t / 6) & 1);
    gfx_circb(x, y, r, c);
    gfx_circb(x, y, r - 2, (t / 3) & 1 ? c2 : c);
    gfx_pset(x, y, C_WHITE);
    gfx_pset(x + (int)(4.0f * cosf((float)t * 0.2f)), y + (int)(4.0f * sinf((float)t * 0.2f)), c2);
}

void frl_draw_world(const FrlWorld *w, int t) {
    int sx = 0, sy = 0;
    if (w->shake) { sx = ((t * 7) % 5) - 2; sy = ((t * 3) % 3) - 1; }
    int cx = w->cam_x + sx, cy = w->cam_y + sy;
    gfx_clip(0, FRL_OY, SCREEN_W, FRL_VH);
    gfx_rect(0, FRL_OY, SCREEN_W, FRL_VH, C_INK);
    gfx_camera(cx, cy - FRL_OY);
    int tx0 = imax(0, cx / FRL_T - 1), tx1 = imin(FRL_MW - 1, (cx + SCREEN_W) / FRL_T + 1);
    int ty0 = imax(0, cy / FRL_T - 1), ty1 = imin(FRL_MH - 1, (cy + FRL_VH) / FRL_T + 1);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++)
            if (!frl_solid(w, tx, ty) || w->tile[ty][tx] == FT_COMB || w->tile[ty][tx] == FT_WOOD) draw_back(tx, ty, tx * FRL_T, ty * FRL_T, t);
    if (cy < 200 && cx < 740) draw_far(cx, t);
    /* torches and banners on the castle's walls */
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            if (backdrop(tx, ty) != BD_CASTLE || frl_tile(w, tx, ty) != FT_AIR || tx >= 124) continue;
            unsigned h = hash(tx, ty);
            int px = tx * FRL_T, py = ty * FRL_T;
            if ((h & 63) == 5 && frl_solid(w, tx, ty + 2)) {
                gfx_vline(px + 5, py + 4, py + 8, C_BROWN);
                gfx_pset(px + 5, py + 3, ((t / 4) + (int)h) & 1 ? C_YELLOW : C_ORANGE);
                gfx_pset(px + 5 + (((t / 6) + (int)h) & 1), py + 2, C_ORANGE);
            } else if ((h & 127) == 9 && frl_tile(w, tx, ty + 1) == FT_AIR && frl_tile(w, tx, ty - 1) == FT_AIR) {
                gfx_rect(px + 2, py, 6, 9, C_MAROON);
                gfx_hline(px + 2, px + 7, py, C_AMBER);
                gfx_pset(px + 4, py + 4, C_FOREST);
                gfx_pset(px + 5, py + 5, C_FOREST);
            }
        }
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) draw_tile(w, tx, ty, tx * FRL_T, ty * FRL_T, t);
    draw_base(w, t);
    /* plates */
    for (int i = 0; i < FRL_PLATES; i++) {
        const FrlPlate *p = &w->plate[i];
        if (!p->tx && !p->ty) continue;
        if (w->tile[p->ty][p->tx] == FT_STONE) continue;
        int px = p->tx * FRL_T, py = p->ty * FRL_T;
        int hgt = p->down ? 1 : 3;
        gfx_rect(px + 1, py + 10 - hgt, 8, hgt, PLATE_COL[i]);
        gfx_hline(px + 1, px + 8, py + 10 - hgt, C_WHITE);
        gfx_hline(px, px + 9, py + 9, C_INK);
    }
    /* drains */
    for (int i = 0; i < w->ndrain; i++) {
        const FrlDrain *d = &w->drain[i];
        if (w->tile[d->ty][d->tx] == FT_STONE) continue;
        int px = d->tx * FRL_T, py = d->ty * FRL_T + (d->ceiling ? 0 : 7);
        gfx_rect(px + 1, py, 8, 3, C_INK);
        for (int k = 0; k < 4; k++) gfx_vline(px + 2 + k * 2, py, py + 2, C_SLATE);
        if ((t / 10 + i) % 6 == 0) gfx_pset(px + 4, d->ceiling ? py + 3 : py - 1, C_JADE);
    }
    /* chutes */
    for (int i = 0; i < w->npipe; i++) {
        const FrlPipe *p = &w->pipe[i];
        int px = p->tx * FRL_T, py = p->ty * FRL_T;
        int ph = p->len * FRL_T;
        gfx_rect(px + 1, py, 8, ph, C_GREY);
        gfx_vline(px + 1, py, py + ph - 1, C_LIGHT);
        gfx_vline(px + 8, py, py + ph - 1, C_SLATE);
        gfx_rect(px + 3, py + 2, 4, ph - 4, C_DUSK);
        gfx_rect(px, py, FRL_T, 3, C_LIGHT);
        gfx_hline(px, px + 9, py + 2, C_SLATE);
        for (int k = 1; k < p->len; k++) gfx_hline(px + 1, px + 8, py + k * FRL_T, C_SLATE);
        gfx_rect(px, py + ph - 2, FRL_T, 2, C_GREY);
    }
    /* keys */
    for (int i = 0; i < w->nkey; i++) {
        const FrlKey *k = &w->key[i];
        if (k->taken) continue;
        int px = k->tx * FRL_T + 2, py = k->ty * FRL_T + 2 + (((t / 12) + i) & 1);
        gfx_circb(px + 2, py + 2, 2, C_YELLOW);
        gfx_vline(px + 2, py + 4, py + 8, C_YELLOW);
        gfx_hline(px + 3, px + 4, py + 7, C_YELLOW);
        gfx_hline(px + 3, px + 4, py + 5, C_AMBER);
        if ((t / 30 + i) % 5 == 0) gfx_pset(px + 1, py + 1, C_WHITE);
    }
    /* pouches */
    for (int i = 0; i < w->npouch; i++) {
        int px = w->pouch[i].x, py = w->pouch[i].y;
        gfx_rect(px - 3, py - 5, 7, 5, C_BROWN);
        gfx_rectb(px - 3, py - 5, 7, 5, C_INK);
        gfx_hline(px - 1, px + 1, py - 6, C_YELLOW);
        gfx_pset(px, py - 3, C_TAN);
    }
    draw_waystone(w, t);
    /* foes */
    for (int i = 0; i < w->nfoe; i++) {
        const FrlFoe *f = &w->foe[i];
        if (!f->on) continue;
        frl_draw_foe(f, f->x >> 8, f->y >> 8, t + i * 7);
    }
    /* the volunteer */
    if (w->phase == FWP_PLAY || (w->phase == FWP_WON && w->phase_t < 4)) {
        const FrlUnit *u = &w->u;
        int ux = (u->x >> 8) - 1, uy = (u->y >> 8) - 1;
        if (u->mode == FUM_WARP) {
            int r = u->mode_t < FRL_WARP_T / 2 ? u->mode_t : FRL_WARP_T - u->mode_t;
            gfx_circb(ux + 4, uy + 5, 1 + r / 2, C_CYAN);
        } else if (u->mode == FUM_PIPE) {
            gfx_rect(ux + 2, uy + 2, 4, 6, C_INK);
        } else {
            int frame = !u->ground ? 3 : (u->vx != 0 ? 1 + (int)(u->life_t / 6) % 2 : 0);
            bool flash = u->charge >= FRL_CHARGE ? (t / 3) & 1 : (u->charge >= FRL_ARMED && (t / 8) % 4 == 0);
            frl_draw_unit_full(u->cls, ux, uy, u->face, frame, flash, w->players == 2 ? u->player : 0, u->atk_t > 4 && u->cls == FRC_MASON);
        }
    }
    /* shots */
    for (int i = 0; i < FRL_SHOTS; i++) {
        const FrlShot *s = &w->shot[i];
        if (!s->on) continue;
        int x = s->x >> 8, y = s->y >> 8;
        switch (s->kind) {
        case FS_PELLET: gfx_rect(x - 1, y - 1, 3, 3, (t / 2) & 1 ? C_PINK : C_MAGENTA); gfx_pset(x, y, C_WHITE); break;
        case FS_AXE: {
            int a = (s->t / 3) & 3;
            gfx_pset(x, y, C_BROWN);
            gfx_rect(x + (a == 0 ? 1 : a == 2 ? -2 : -1), y + (a == 1 ? 1 : a == 3 ? -2 : -1), 2, 2, C_LIGHT);
            break;
        }
        case FS_BUBBLE: gfx_circb(x, y, 3, C_CYAN); gfx_pset(x - 1, y - 1, C_WHITE); break;
        case FS_CRESCENT:
            gfx_line(x - 2, y - 3, x + 1, y, C_RED);
            gfx_line(x + 1, y, x - 2, y + 3, C_RED);
            gfx_pset(x - 1, y, C_ORANGE);
            break;
        case FS_BULLET: gfx_hline(x - 2, x + 1, y, C_YELLOW); gfx_pset(x + (s->vx > 0 ? 1 : -2), y, C_WHITE); break;
        case FS_STAR:
            gfx_pset(x, y, C_WHITE);
            if ((s->t / 2) & 1) { gfx_pset(x - 1, y, C_LIGHT); gfx_pset(x + 1, y, C_LIGHT); gfx_pset(x, y - 1, C_GREY); gfx_pset(x, y + 1, C_GREY); }
            else { gfx_pset(x - 1, y - 1, C_LIGHT); gfx_pset(x + 1, y + 1, C_LIGHT); gfx_pset(x + 1, y - 1, C_GREY); gfx_pset(x - 1, y + 1, C_GREY); }
            break;
        case FS_WRENCH: {
            int a = (s->t / 3) & 1;
            gfx_line(x - 2, y - 2 + a * 4, x + 2, y + 2 - a * 4, C_LIGHT);
            gfx_pset(x - 2, y - 2 + a * 4, C_GREY);
            break;
        }
        default: break;
        }
    }
    gfx_camera(0, 0);
    gfx_noclip();
}
