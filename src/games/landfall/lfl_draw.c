/* LANDFALL - drawing: the planet's rooms (with a tile of each neighbour
 * showing at the sides and top), creatures, Wick, the HUD (energy, two
 * hands, four resources, three chart slots, the clock), talk boxes in plain
 * words or glyphs, the title, landing, lift-off and the ending. */
#include "lfl.h"

static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};

static uint32_t hash(uint32_t a, uint32_t b) {
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    return h;
}

/* ------------------------------------------------------------------------ */
/* the glyph tongue: every letter and digit upside down, with a tick over it */

void lfl_glyph_text(const char *s, int x, int y, int col) {
    unsigned char m[8 * 10];
    for (; *s; s++) {
        char ch = *s;
        if (ch == ' ') { x += 5; continue; }
        if (!isalnum((unsigned char)ch)) {
            char one[2] = {ch, 0};
            text_draw(one, x, y, col);
            x += text_width(one) + 1;
            continue;
        }
        char one[2] = {ch, 0};
        memset(m, 0, sizeof m);
        text_render_mask(one, 1, m, 8, 10, 0, 1);
        int w = text_width(one);
        for (int yy = 0; yy < 10; yy++)
            for (int xx = 0; xx < 8; xx++)
                if (m[yy * 8 + xx]) gfx_pset(x + w - 1 - xx, y + 8 - yy, col);
        gfx_pset(x + w / 2, y - 1, col);
        x += w + 2;
    }
}

/* ------------------------------------------------------------------------ */
/* tiles                                                                    */

static void draw_tile(int room, int tx, int ty, int x, int y) {
    const LflRoom *r = &lp.room[room];
    int t = r->exists ? r->t[ty][tx] : T_STONE;
    bool dun = r->area == AREA_DUN;
    int g = dun ? C_NIGHT : lp.ground_col, gd = dun ? C_INK : lp.ground_dark;
    int wall = dun ? C_DUSK : lp.wall_col;
    uint32_t h = hash((uint32_t)(room * 300 + ty * 20 + tx), lp.seed);
    int f = engine_frame();
    /* ground under everything that isn't a wall */
    gfx_rect(x, y, 16, 16, g);
    if ((h & 7) == 0) gfx_pset(x + (int)(h >> 4) % 14 + 1, y + (int)(h >> 8) % 14 + 1, gd);
    if ((h & 15) == 3) gfx_hline(x + (int)(h >> 5) % 10 + 2, x + (int)(h >> 5) % 10 + 4, y + (int)(h >> 9) % 12 + 2, gd);
    switch (t) {
    case T_FLOOR: case T_SHIP: break;
    case T_WOOD:
        gfx_rect(x, y + 1, 16, 14, C_BROWN);
        for (int k = 0; k < 16; k += 4) gfx_vline(x + k, y + 1, y + 14, C_EARTH);
        gfx_hline(x, x + 15, y + 1, C_TAN);
        break;
    case T_STONE:
        gfx_rect(x, y, 16, 16, wall);
        gfx_hline(x, x + 15, y + 7, C_INK);
        gfx_vline(x + ((ty & 1) ? 4 : 11), y, y + 6, C_INK);
        gfx_vline(x + ((ty & 1) ? 11 : 4), y + 8, y + 15, C_INK);
        gfx_hline(x, x + 15, y, PAL_LIGHTER[wall]);
        break;
    case T_ROCK:
    case T_SECRET:
        gfx_circ(x + 8, y + 9, 7, C_INK);
        gfx_circ(x + 8, y + 8, 7, wall);
        gfx_circ(x + 6, y + 6, 3, PAL_LIGHTER[wall]);
        gfx_pset(x + 10, y + 11, C_INK);
        break;
    case T_PLANT:
        gfx_rect(x + 7, y + 9, 3, 7, C_BROWN);
        gfx_circ(x + 8, y + 7, 7, C_INK);
        gfx_circ(x + 8, y + 6, 6, dun ? C_SLATE : lp.plant_col);
        gfx_circ(x + 6, y + 4, 2, PAL_LIGHTER[dun ? C_SLATE : lp.plant_col]);
        break;
    case T_HUT:
        gfx_rect(x, y + 4, 16, 12, C_TAN);
        gfx_rect(x + 6, y + 9, 4, 7, C_INK);
        gfx_rect(x, y + 2, 16, 4, C_HIDE);
        gfx_hline(x, x + 15, y + 2, C_ORANGE);
        break;
    case T_FOLIAGE:
        gfx_rect(x + 1, y + 1, 14, 14, C_FOREST);
        for (int k = 0; k < 6; k++) gfx_circ(x + 3 + (k % 3) * 5, y + 4 + (k / 3) * 7, 3, (k & 1) ? C_LEAF : C_JADE);
        gfx_rectb(x + 1, y + 1, 14, 14, C_INK);
        break;
    case T_CRYSTAL:
        for (int k = 0; k < 3; k++) {
            int cx = x + 4 + k * 4, top = y + 2 + (k == 1 ? 0 : 4);
            for (int yy = top; yy < y + 15; yy++) {
                int wdt = imin(3, (yy - top) / 2 + 1);
                gfx_hline(cx - wdt, cx + wdt, yy, k == 1 ? C_ICE : C_CYAN);
            }
            gfx_vline(cx, top + 1, y + 14, C_WHITE);
        }
        gfx_hline(x + 1, x + 14, y + 15, C_NAVY);
        break;
    case T_FLESH:
        gfx_circ(x + 8, y + 8, 7, C_MAGENTA);
        gfx_circ(x + 5, y + 6, 3, C_PINK);
        gfx_circ(x + 11, y + 10, 2, C_WINE);
        break;
    case T_TECH:
        gfx_rect(x + 1, y, 14, 16, C_BLUE);
        gfx_rectb(x + 1, y, 14, 16, C_NAVY);
        gfx_rect(x + 4, y + 3, 8, 3, C_SKY);
        gfx_rect(x + 4, y + 9, 8, 3, (f / 20) & 1 ? C_CYAN : C_SKY);
        break;
    case T_TECHDOWN:
        gfx_rect(x + 1, y + 1, 14, 14, C_NAVY);
        gfx_rectb(x + 1, y + 1, 14, 14, C_BLUE);
        break;
    case T_SWAMP:
    case T_WARP: {
        int c1 = t == T_WARP ? C_VIOLET : C_PURPLE, c2 = t == T_WARP ? C_WHITE : C_MAGENTA;
        gfx_rect(x, y, 16, 16, c1);
        int b = (f / 12 + (int)h) & 15;
        gfx_circ(x + 4 + (int)(h % 7), y + 4 + (b & 7), 1 + (b >> 3), c2);
        gfx_pset(x + 11 - (int)(h % 5), y + 11, c2);
        if (t == T_WARP) gfx_circb(x + 8, y + 8, 2 + (f / 6) % 5, C_WHITE);
        /* under dry ground its top is a muddy bank she can tread (lfl_play.c) */
        int up = ty > 0 ? r->t[ty - 1][tx] : T_STONE;
        if (up != T_SWAMP && up != T_WARP) {
            gfx_rect(x, y, 16, 5, gd);
            gfx_hline(x, x + 15, y + 5, c2);
        }
        break;
    }
    case T_PIT:
        gfx_rect(x, y, 16, 16, C_INK);
        gfx_hline(x, x + 15, y, gd);
        gfx_hline(x, x + 15, y + 1, C_NIGHT);
        break;
    case T_HOLE:
        gfx_rect(x, y, 16, 16, wall);
        gfx_rect(x + 5, y + 5, 6, 6, C_INK);
        gfx_hline(x, x + 15, y, PAL_LIGHTER[wall]);
        break;
    case T_LOG:
        gfx_rect(x, y + 3, 16, 10, C_BROWN);
        gfx_hline(x, x + 15, y + 3, C_TAN);
        gfx_hline(x, x + 15, y + 12, C_EARTH);
        gfx_circ(x + 8, y + 8, 3, C_INK);
        break;
    case T_STAIRS_DN:
    case T_STAIRS_UP:
        gfx_rect(x + 1, y + 1, 14, 14, C_INK);
        for (int k = 0; k < 4; k++) gfx_hline(x + 2 + k, x + 13 - k, y + 2 + k * 3, t == T_STAIRS_UP ? C_LIGHT : C_GREY);
        break;
    case T_VOID:
        gfx_rect(x, y, 16, 16, C_INK);
        break;
    default: break;
    }
}

static void draw_room_tiles(int room, int sx, int sy) {
    for (int ty = 0; ty < LFL_RH; ty++)
        for (int tx = 0; tx < LFL_RW; tx++) {
            int x = sx + tx * LFL_TILE, y = sy + ty * LFL_TILE;
            if (x > SCREEN_W || y > LFL_PLAY_H || x + 16 < 0 || y + 16 < 0) continue;
            draw_tile(room, tx, ty, x, y);
        }
}

/* the ship, 32 x 32, standing (or rising) with its top-left at (x, y) */
static void draw_ship(int x, int y, bool flame, int t) {
    if (flame) {
        int fl = 6 + (t / 2) % 4;
        gfx_rect(x + 12, y + 30, 8, fl, C_ORANGE);
        gfx_rect(x + 14, y + 30, 4, fl + 3, C_YELLOW);
    }
    gfx_line(x + 4, y + 31, x + 9, y + 22, C_GREY);
    gfx_line(x + 27, y + 31, x + 22, y + 22, C_GREY);
    gfx_circ(x + 16, y + 18, 12, C_INK);
    gfx_circ(x + 16, y + 17, 11, C_LIGHT);
    gfx_rect(x + 5, y + 18, 23, 4, C_AMBER);
    gfx_circ(x + 16, y + 10, 6, C_INK);
    gfx_circ(x + 16, y + 10, 5, C_SKY);
    gfx_circ(x + 14, y + 8, 2, C_WHITE);
    gfx_pset(x + 9, y + 15, C_WHITE);
    gfx_pset(x + 23, y + 15, C_GREY);
}

/* ------------------------------------------------------------------------ */
/* creatures                                                                */

static void draw_snail(const LflMob *m, int x, int y) {
    bool shell = (m->flags & MF_SHELL) != 0;
    int face = m->face;
    int bx = x + 14 - DX[face] * 4, by = y + 16 - DY[face] * 2;
    if (!shell) {
        /* the body sticks out the way it faces */
        int hx = x + 14 + DX[face] * 12, hy = y + 16 + DY[face] * 10;
        gfx_circ(hx, hy, 6, C_INK);
        gfx_circ(hx, hy, 5, C_TAN);
        gfx_pset(hx - 2, hy - 2, C_INK);
        gfx_pset(hx + 2, hy - 2, C_INK);
        gfx_line(hx - 2, hy - 4, hx - 4, hy - 9, C_TAN);
        gfx_line(hx + 2, hy - 4, hx + 4, hy - 9, C_TAN);
    }
    gfx_circ(bx, by, 13, C_INK);
    gfx_circ(bx, by, 12, shell ? C_GREY : C_ORANGE);
    gfx_circb(bx, by, 8, shell ? C_SLATE : C_BROWN);
    gfx_circb(bx + 1, by + 1, 4, shell ? C_SLATE : C_BROWN);
    gfx_circ(bx - 5, by - 6, 2, shell ? C_LIGHT : C_AMBER);
    if (shell) {
        /* its open end */
        gfx_circ(x + 14 + DX[face] * 9, y + 16 + DY[face] * 9, 4, C_INK);
    }
}

static void draw_mob(const LflMob *m, int ox, int oy) {
    int x = ox + LFL_PX(m->x), y = oy + LFL_PX(m->y);
    int f = engine_frame();
    if (m->sp == SP_SNAIL) { draw_snail(m, x, y); return; }
    if (m->flags & MF_MORPHED) {
        spr_draw(&lfl_spr[(m->t / 10) & 1 ? SPR_CHICK1 : SPR_CHICK0], x, y, m->face == DIR_LEFT ? SPR_FLIPX : 0);
        return;
    }
    if (m->sp == SP_GNAT) {
        spr_draw(&lfl_spr[(f / 3) & 1 ? SPR_GNAT1 : SPR_GNAT0], x, y, 0);
        return;
    }
    const LflSpecies *s = &lp.sp[m->sp];
    const Sprite *sp = &lfl_spr[SPR_LOOK0 + s->look * 2 + ((m->t / 16) & 1)];
    int flags = m->face == DIR_LEFT ? SPR_FLIPX : 0;
    int bob = (m->flags & MF_FRIENDLY) && !(m->flags & MF_ANGRY) && m->talk_t > 0 ? ((m->talk_t / 8) & 1) : 0;
    if (m->hurt_t > 0 && (m->hurt_t & 2)) spr_draw_ex(sp, x - 2, y - 2 - bob, flags, NULL, C_WHITE);
    else spr_draw(sp, x - 2, y - 2 - bob, flags);
    if (m->flags & MF_ANGRY) gfx_pset(x + 6, y - 4, C_RED);
    if ((m->flags & MF_FRIENDLY) && !(m->flags & MF_ANGRY) && m->talk_t > 0) spr_draw(&lfl_spr[SPR_TALK], x + 10, y - 10, 0);
}

static void draw_wick(int x, int y) {
    const LflPlayer *p = &lflg.pl;
    if (p->inv > 0 && (p->inv & 4) && lflg.state == LS_PLAY) return;
    if (p->shrink_t > 0) {
        spr_draw(&lfl_spr[SPR_WICK_SMALL], x - 1, y - 2, p->face == DIR_LEFT ? SPR_FLIPX : 0);
        return;
    }
    int step = (p->walk_t / 8) & 1;
    int id = p->face == DIR_UP ? SPR_WICK_U0 : p->face == DIR_DOWN ? SPR_WICK_D0 : SPR_WICK_R0;
    int flags = p->face == DIR_LEFT ? SPR_FLIPX : 0;
    const Sprite *s = &lfl_spr[id + step];
    if (p->hurt_flash > 0) spr_draw_ex(s, x - 1, y - 4, flags, NULL, C_RED);
    else spr_draw(s, x - 1, y - 4, flags);
}

static void draw_things(int ox, int oy) {
    for (int i = 0; i < lp.nthing; i++) {
        const LflThing *t = &lp.thing[i];
        if (!t->alive || t->room != lflg.cur) continue;
        int x = ox + t->x, y = oy + t->y;
        switch (t->kind) {
        case TH_TOOL: spr_draw(&lfl_spr[SPR_TOOL0 + t->sub], x, y, 0); break;
        case TH_CUBE: spr_draw(&lfl_spr[SPR_CUBE], x, y - ((engine_frame() / 15) & 1), 0); break;
        case TH_MAP: spr_draw(&lfl_spr[SPR_MAP], x, y, 0); break;
        case TH_BUNDLE:
            spr_draw(&lfl_spr[SPR_BUNDLE], x, y, 0);
            spr_draw(&lfl_spr[SPR_RES0 + t->sub], x + 6, y + 5, 0);
            break;
        default: break;
        }
    }
}

static void draw_room_contents(int ox, int oy) {
    int f = engine_frame();
    for (int i = 0; i < LFL_MAX_DROPS; i++) {
        const LflDrop *d = &lflg.drop[i];
        if (!d->alive) continue;
        int x = ox + LFL_PX(d->x), y = oy + LFL_PX(d->y);
        if (d->kind == DR_RES) spr_draw(&lfl_spr[SPR_RES0 + d->sub], x, y, 0);
        else spr_draw(&lfl_spr[d->cooked ? SPR_CHOP_COOKED : SPR_CHOP], x, y, 0);
        if (d->kind == DR_CHOP && d->cooked && (f / 10) & 1) gfx_pset(x + 3, y - 2, C_LIGHT);
    }
    draw_things(ox, oy);
    for (int i = 0; i < LFL_MAX_BOMBS; i++) {
        const LflBomb *b = &lflg.bomb[i];
        if (!b->alive) continue;
        int x = ox + LFL_PX(b->x), y = oy + LFL_PX(b->y);
        gfx_circ(x, y + 1, 5, C_INK);
        gfx_circ(x, y, 4, C_NIGHT);
        gfx_pset(x - 1, y - 2, C_LIGHT);
        gfx_pset(x + 2, y - 5, (b->t / (b->t < 30 ? 2 : 6)) & 1 ? C_RED : C_YELLOW);
    }
    /* the big ones first, so the small ones show on top */
    for (int i = 0; i < LFL_MAX_MOBS; i++)
        if (lflg.mob[i].alive && lflg.mob[i].sp == SP_SNAIL) draw_mob(&lflg.mob[i], ox, oy);
    for (int i = 0; i < LFL_MAX_MOBS; i++)
        if (lflg.mob[i].alive && lflg.mob[i].sp != SP_SNAIL) draw_mob(&lflg.mob[i], ox, oy);
    if (lflg.state != LS_DYING || (lflg.state_t < 40 && (lflg.state_t & 2))) draw_wick(ox + LFL_PX(lflg.pl.x), oy + LFL_PX(lflg.pl.y));
    for (int i = 0; i < LFL_MAX_SHOTS; i++) {
        const LflShot *s = &lflg.shot[i];
        if (!s->alive) continue;
        int x = ox + LFL_PX(s->x), y = oy + LFL_PX(s->y);
        switch (s->kind) {
        case SH_RIVET: gfx_rect(x, y, 4, 4, C_LIGHT); gfx_pset(x + 1, y + 1, C_WHITE); break;
        case SH_DISC:
            gfx_circ(x + 4, y + 4, 4, C_INK);
            gfx_circ(x + 4, y + 4, 3, (f / 3) & 1 ? C_ORANGE : C_AMBER);
            gfx_pset(x + 4, y + 4, C_INK);
            break;
        case SH_HATCH: gfx_circ(x + 3, y + 3, 3, C_PINK); gfx_pset(x + 2, y + 2, C_WHITE); break;
        case SH_FOE: gfx_circ(x + 2, y + 2, 3, C_INK); gfx_circ(x + 2, y + 2, 2, (f / 4) & 1 ? C_RED : C_ORANGE); break;
        case SH_SHARD: gfx_rect(x, y, 3, 3, C_ICE); break;
        default: break;
        }
    }
    for (int i = 0; i < LFL_MAX_BLASTS; i++) {
        const LflBlast *b = &lflg.blast[i];
        if (!b->alive || b->kind != 0) continue;
        int r = b->r * (20 - b->t) / 20 + 4;
        gfx_circ(ox + b->x, oy + b->y, r, (b->t / 3) & 1 ? C_YELLOW : C_ORANGE);
        gfx_circ(ox + b->x, oy + b->y, r / 2, C_WHITE);
    }
}

/* the view: the room at (16, 16), its neighbours at the sides and top */
static void room_origin(int room, int *x, int *y) {
    *x = lp.room[room].gx * LFL_ROOM_PW;
    *y = lp.room[room].gy * LFL_ROOM_PH;
}

static void draw_world(void) {
    int cx, cy;
    room_origin(lflg.cur, &cx, &cy);
    int camx = cx - LFL_VIEW_X, camy = cy - LFL_VIEW_Y;
    if (lflg.state == LS_SCROLL) {
        int tx, ty;
        room_origin(lflg.scroll_to, &tx, &ty);
        int t = lflg.scroll_t;
        camx += (tx - cx) * t / 40;
        camy += (ty - cy) * t / 40;
    }
    gfx_clip(0, 0, SCREEN_W, LFL_PLAY_H);
    int area = lp.room[lflg.cur].area;
    int gx0 = camx / LFL_ROOM_PW - 1, gy0 = camy / LFL_ROOM_PH - 1;
    for (int gy = gy0; gy <= gy0 + 3; gy++)
        for (int gx = gx0; gx <= gx0 + 3; gx++) {
            int sx = gx * LFL_ROOM_PW - camx, sy = gy * LFL_ROOM_PH - camy;
            if (sx >= SCREEN_W || sy >= LFL_PLAY_H || sx + LFL_ROOM_PW <= 0 || sy + LFL_ROOM_PH <= 0) continue;
            int room = lfl_room_index(area, gx, gy);
            if (room >= 0 && lp.room[room].exists) draw_room_tiles(room, sx, sy);
            else gfx_rect(sx, sy, LFL_ROOM_PW, LFL_ROOM_PH, area == AREA_DUN ? C_INK : lp.wall_col);
        }
    int ox = cx - camx, oy = cy - camy;
    if (lflg.cur == lp.ship_room) {
        int lift = 0;
        bool flame = false;
        if (lflg.state == LS_LANDING) { lift = imax(0, (130 - lflg.state_t) * 3 / 2); flame = lift > 0; }
        if (lflg.state == LS_LIFTOFF) { lift = imax(0, lflg.state_t - 40) * imax(1, lflg.state_t / 40); flame = true; }
        draw_ship(ox + lp.ship_tx * LFL_TILE, oy + lp.ship_ty * LFL_TILE - lift, flame, lflg.frame_t);
    }
    if (lflg.state == LS_SCROLL) {
        /* only Wick is carried along */
        draw_wick(ox + LFL_PX(lflg.pl.x), oy + LFL_PX(lflg.pl.y));
    } else if (lflg.state == LS_LANDING) {
        if (lflg.state_t > 135) draw_wick(ox + LFL_PX(lflg.pl.x), oy + LFL_PX(lflg.pl.y));
        draw_things(ox, oy);
    } else if (lflg.state != LS_LIFTOFF) {
        draw_room_contents(ox, oy);
    }
    if (lflg.pulse_flash > 30) gfx_dither(0, 0, SCREEN_W, LFL_PLAY_H, C_WHITE, (lflg.pulse_flash - 30) * 2);
    gfx_noclip();
}

/* ------------------------------------------------------------------------ */
/* the HUD                                                                  */

static void draw_hud(void) {
    const LflPlayer *p = &lflg.pl;
    int f = engine_frame();
    gfx_rect(0, LFL_PLAY_H, SCREEN_W, SCREEN_H - LFL_PLAY_H, C_INK);
    gfx_hline(0, SCREEN_W - 1, LFL_PLAY_H, C_SLATE);
    /* energy: one block a point, up to seven */
    for (int i = 0; i < p->hpmax; i++) {
        int x = 4 + i * 9, y = 166;
        if (i < p->hp) { gfx_rect(x, y, 7, 7, C_RED); gfx_pset(x + 1, y + 1, C_PINK); }
        else gfx_rectb(x, y, 7, 7, C_MAROON);
    }
    /* the two hands, the active one framed */
    for (int k = 0; k < 2; k++) {
        int x = 70 + k * 20, y = 162;
        bool active = k == p->act;
        gfx_rect(x, y, 16, 16, C_NIGHT);
        gfx_rectb(x - 1, y - 1, 18, 18, active ? C_WHITE : C_SLATE);
        if (p->hold[k] >= 0) {
            const Sprite *s = &lfl_spr[SPR_TOOL0 + p->hold[k]];
            bool flying = false;
            for (int i = 0; i < LFL_MAX_SHOTS; i++) flying |= lflg.shot[i].alive && lflg.shot[i].kind == SH_DISC;
            if (p->hold[k] == TL_DISC && flying) spr_draw_ex(s, x + 2, y + 2, 0, NULL, C_DUSK);
            else spr_draw(s, x + 2, y + 2, 0);
        }
    }
    /* the resources */
    for (int r = 0; r < RS_COUNT; r++) {
        int x = 116 + r * 28, y = 166;
        bool flash = lflg.res_flash[r] > 0 && (f / 4) & 1;
        if (flash) gfx_rect(x - 2, y - 2, 26, 12, C_RED);
        spr_draw(&lfl_spr[SPR_RES0 + r], x, y, 0);
        char buf[8];
        snprintf(buf, sizeof buf, "%02d", lflg.res[r]);
        text_draw(buf, x + 10, y, flash ? C_WHITE : C_LIGHT);
    }
    /* the chart */
    for (int k = 0; k < 3; k++) {
        int x = 230 + k * 14, y = 164;
        if (k < lflg.maps) spr_draw(&lfl_spr[SPR_MAP], x, y, 0);
        else gfx_rectb(x + 1, y + 1, 10, 10, C_SLATE);
    }
    /* the clock (it stops while talking or paused) */
    char tb[16];
    uint32_t s = lflg.run_t / 60;
    if (s > 99 * 60 + 59) s = 99 * 60 + 59;
    snprintf(tb, sizeof tb, "%02u:%02u", (unsigned)(s / 60), (unsigned)(s % 60));
    text_draw(tb, SCREEN_W - 4 - text_width(tb), 168, C_CREAM);
}

static void draw_talk(void) {
    int x = 16, y = 100, w = 288, h = 56;
    ui_panel(x, y, w, h, C_INK, C_LIGHT);
    for (int i = 0; i < lflg.talk_lines; i++) {
        const char *s = lflg.talk[i];
        bool glyph = lflg.talk_glyph && !(i == 2 && lflg.talk_trade >= 0);
        if (glyph) lfl_glyph_text(s, x + 10, y + 8 + i * 15, C_YELLOW);
        else text_draw(s, x + 10, y + 8 + i * 15, C_WHITE);
    }
}

/* ------------------------------------------------------------------------ */
/* screens                                                                  */

static void stars(int t) {
    gfx_cls(C_INK);
    for (int i = 0; i < 70; i++) {
        uint32_t h = hash((uint32_t)i, 8);
        int x = (int)(h % SCREEN_W), y = (int)((h >> 9) % SCREEN_H);
        int c = (i + t / 20) % 9 == 0 ? C_WHITE : (h >> 20) & 1 ? C_SLATE : C_GREY;
        gfx_pset(x, y, c);
    }
}

static const char *const OVER_T[] = {"WICK'S TRIP ENDS HERE.", "THIS PLANET KEEPS ITS SECRETS.", "THE NEXT ONE WILL BE NEW."};
static const char *const END_T[] = {"THE THREE PIECES FIT.", "THE STAR CHART IS WHOLE,", "AND WICK HEADS FOR HOME."};

static void draw_title(void) {
    int t = lflg.state_t;
    stars(t);
    gfx_circ(160, 330, 210, C_TEAL);
    gfx_dither(0, 120, SCREEN_W, 60, C_JADE, 4);
    gfx_circb(160, 330, 210, C_CYAN);
    int sx = 40 + (t / 2) % 300, sy = 30 + ((t / 9) & 1);
    draw_ship(sx - 16, sy, true, t);
    static const uint8_t grad[3] = {C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center("LANDFALL", 160, 62, 3, grad, 3, C_INK, C_MAROON);
    if ((t / 30) & 1 || t < 30) text_center(GLYPH_A " LAND ON A NEW PLANET", 160, 104, C_WHITE);
    char rec[64];
    snprintf(rec, sizeof rec, "MOST TOOLS FOUND %d    MOST ENERGY FOUND %d", lsv.most_items, lsv.most_energy);
    tiny_center(rec, 160, 150, C_LIGHT);
    tiny_center("1984 BEAMDOWN SOFTWORKS", 160, 170, C_SLATE);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    for (int i = 0; i < 3; i++) text_center(OVER_T[i], 160, 60 + i * 14, i == 0 ? C_RED : C_LIGHT);
    if (lflg.state_t > 60) tiny_center("PRESS " GLYPH_A, 160, 140, C_SLATE);
}

static void draw_ending(void) {
    int t = lflg.state_t;
    stars(t);
    draw_ship(SCREEN_W - (t * 2) % (SCREEN_W + 80), 40 + ((t / 10) & 1), true, t);
    for (int i = 0; i < 3; i++) text_center(END_T[i], 160, 90 + i * 12, C_CREAM);
    char tb[40];
    uint32_t s = lflg.run_t / 60;
    snprintf(tb, sizeof tb, "TIME ON YOMBRA %02u:%02u", (unsigned)(s / 60), (unsigned)(s % 60));
    text_center(tb, 160, 136, lflg.run_t < LFL_CHERRY_FRAMES ? C_YELLOW : C_LIGHT);
    if (t > 120) tiny_center("THE END", 160, 160, C_SLATE);
}

void lfl_draw(void) {
    switch (lflg.state) {
    case LS_TITLE: draw_title(); return;
    case LS_OVER: draw_over(); return;
    case LS_ENDING: draw_ending(); return;
    default: break;
    }
    gfx_cls(C_INK);
    draw_world();
    draw_hud();
    if (lflg.state == LS_TALK) draw_talk();
    if (lflg.msg_t > 0) {
        int w = text_width(lflg.msg);
        gfx_rect(160 - w / 2 - 4, 2, w + 8, 11, C_INK);
        text_draw(lflg.msg, 160 - w / 2, 4, C_YELLOW);
    }
    if (lflg.state == LS_LIFTOFF && lflg.state_t > 120) gfx_set_fade(0);
}

void lfl_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_INK);
    for (int i = 0; i < 18; i++) {
        uint32_t hh = hash((uint32_t)i, 88);
        gfx_pset(x + (int)(hh % (uint32_t)w), y + (int)((hh >> 8) % (uint32_t)h), (i + t / 15) % 5 ? C_GREY : C_WHITE);
    }
    gfx_circ(x + w / 2, y + h + 70, 90, C_TEAL);
    gfx_circb(x + w / 2, y + h + 70, 90, C_CYAN);
    draw_ship(x + w / 2 - 16, y + h - 44 + ((t / 20) & 1), (t / 40) & 1, t);
    spr_draw(&lfl_spr[SPR_WICK_R0 + ((t / 8) & 1)], x + 12 + (t / 2) % 20, y + h - 18, 0);
    spr_draw(&lfl_spr[SPR_MAP], x + w - 20, y + h - 18, 0);
    static const uint8_t grad[2] = {C_YELLOW, C_AMBER};
    ui_fancy_text("LANDFALL", x + 6, y + 4, 1, grad, 2, C_INK, C_NAVY);
    gfx_noclip();
}

/* tests: the screens' words stay inside the screen and clear of each other,
 * and every talk line fits its box */
int lfl_layout_audit(void) {
    int bad = 0;
    ui_audit_begin("landfall", true);
    ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
    ui_audit_text("land", GLYPH_A " LAND ON A NEW PLANET", 160 - text_width(GLYPH_A " LAND ON A NEW PLANET") / 2, 104);
    bad += ui_audit_end();
    for (int i = 0; i < 3; i++) {
        bad += text_width(OVER_T[i]) > SCREEN_W - 8;
        bad += text_width(END_T[i]) > SCREEN_W - 8;
        bad += text_missing(OVER_T[i]) + text_missing(END_T[i]);
    }
    /* every hint and offer that can be said */
    for (int k = 0; k < 3; k++) {
        const char *s = lfl_hint_line(k == 2 ? 1 : 0);
        bad += text_width(s) > 268 || text_missing(s);
    }
    for (int i = 0; i < lflg.talk_lines; i++) bad += text_width(lflg.talk[i]) > 268 || text_missing(lflg.talk[i]);
    char rec[64];
    snprintf(rec, sizeof rec, "MOST TOOLS FOUND %d    MOST ENERGY FOUND %d", 8, 5);
    bad += tiny_width(rec) > SCREEN_W - 8;
    return bad;
}
