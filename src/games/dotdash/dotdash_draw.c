/* DOT & DASH - drawing: the room at two zooms, the micro and deep worlds,
 * everyone in them, and the screens around the game. */
#include "dotdash.h"

extern uint8_t dd_tilepx[T_COUNT][4][64];
void dd_draw_npc(int sub, int x, int y, int flip, int t);
void dd_draw_obj(int sub, int x, int y, int param);
void dd_draw_boss(const Ent *e, int x, int y, int t);

static int T; /* animation clock */

/* ------------------------------------------------------------------ */
/* backgrounds                                                           */

static const uint8_t BIOME_BG[M_COUNT][3] = {
    [M_NONE] = {C_INK, C_NIGHT, C_DUSK},
    [M_WOOD] = {C_NIGHT, C_EARTH, C_BROWN},
    [M_WALL] = {C_DUSK, C_SLATE, C_GREY},
    [M_CERAMIC] = {C_MAROON, C_WINE, C_ORANGE},
    [M_SOIL] = {C_INK, C_NIGHT, C_EARTH},
    [M_FABRIC] = {C_NIGHT, C_MAROON, C_WINE},
    [M_PAPER] = {C_DUSK, C_TAN, C_CREAM},
    [M_METAL] = {C_INK, C_NIGHT, C_SLATE},
    [M_GLASS] = {C_NAVY, C_BLUE, C_ICE},
    [M_DUST] = {C_NIGHT, C_DUSK, C_SLATE},
    [M_CARD] = {C_WINE, C_RED, C_AMBER},
    [M_FUR] = {C_EARTH, C_BROWN, C_TAN},
    [M_CELL] = {C_INK, C_NIGHT, C_PURPLE},
    [M_STONE] = {C_INK, C_DUSK, C_SLATE},
    [M_PLANT] = {C_INK, C_TEAL, C_FOREST},
    [M_WAX] = {C_NIGHT, C_BROWN, C_AMBER},
};

static void biome_back(int mat, int cx, int cy, int sx0, int sx1) {
    const uint8_t *c = BIOME_BG[iclamp(mat, 0, M_COUNT - 1)];
    gfx_rect(sx0, 0, sx1 - sx0, SCREEN_H, c[0]);
    int px = cx / 2, py = cy / 2;
    for (int y = -(py % 16); y < SCREEN_H; y += 16)
        for (int x = sx0 - ((px + sx0) % 16); x < sx1; x += 16) {
            uint32_t h = dd_hash((uint32_t)((x + px) / 16), (uint32_t)((y + py) / 16 + mat * 977));
            int k = (int)(h % 7);
            switch (mat) {
            case M_WOOD: case M_CARD: gfx_hline(imax(x, sx0), imin(x + 15, sx1 - 1), y + (int)(h % 16), c[1]); break;
            case M_FABRIC: gfx_line(x, y, x + 15, y + 15, c[1]); if (k < 3) gfx_line(x + 15, y, x, y + 15, c[1]); break;
            case M_PAPER: gfx_hline(imax(x, sx0), imin(x + 15, sx1 - 1), y + 4, c[1]); gfx_hline(imax(x, sx0), imin(x + 15, sx1 - 1), y + 11, c[1]); break;
            case M_METAL: gfx_rectb(x, y, 16, 16, c[1]); break;
            case M_CELL: if (k < 3) gfx_circb(x + 8, y + 8, 3 + k * 2, k ? c[1] : c[2]); break;
            case M_DUST: if (k < 4) gfx_dither_circle(x + 8, y + 8, 4 + k, c[1], 8); break;
            case M_GLASS: gfx_dither(x, y, 16, 16, c[1], 3 + k); break;
            case M_PLANT: if (k < 3) gfx_vline(x + 8, y, y + 15, c[1]); break;
            default: if (k < 2) gfx_pset(x + 8, y + 8, c[1]); if (k == 3) gfx_pset(x + 3, y + 12, c[2]); break;
            }
        }
}

static void room_back_tile(int b, int tx, int ty, int sx, int sy, int ts) {
    int c = C_SLATE;
    switch (b) {
    case BG_PAPER: c = ((tx / 3) % 2) ? C_TEAL : C_FOREST; if (ts == DD_TS && (tx + ty) % 7 == 0) c = C_JADE; break;
    case BG_DOOR: c = (tx % 9 == 0 || ty % 16 == 0) ? C_EARTH : C_BROWN; break;
    case BG_OUTLET: c = (tx % 2 == 1 && ty % 2 == 1) ? C_INK : C_CREAM; break;
    case BG_FRIED: c = (tx + ty) % 2 ? C_INK : C_NIGHT; break;
    case BG_CORD: c = C_INK; break;
    case BG_STALK: c = C_FOREST; break;
    case BG_CLOCK: c = C_CREAM; break;
    case BG_FRAME: c = (ty % 4 < 2) ? C_SKY : C_BLUE; if (ty >= 25) c = C_LEAF; break;
    case BG_BRACKET: c = C_SLATE; break;
    case BG_CAVITY: c = (tx % 12 == 0) ? C_DUSK : C_NIGHT; break;
    case BG_BOX: c = ((tx / 4 + ty / 4) % 2) ? C_CREAM : C_TAN; break;
    case BG_DARK: c = C_INK; break;
    case BG_BULB: c = dd_lamp_dark ? C_GREY : C_YELLOW; break;
    case BG_HOLLOW: c = C_NIGHT; break;
    default: c = C_INK; break;
    }
    gfx_rect(sx, sy, ts, ts, c);
}

/* ------------------------------------------------------------------ */
/* tiles                                                                  */

static bool solid_t(int t) { return DD_TILE[t].flags & (TF_SOLID); }

static void draw_tile8(int t, int tx, int ty, int sx, int sy) {
    const TileInfo *I = &DD_TILE[t];
    int v = (int)(dd_hash((uint32_t)tx, (uint32_t)ty) & 3);
    const uint8_t *px = dd_tilepx[t][v];
    bool top = !solid_t(lv_tile(&dd_lv, tx, ty - 1)) && !(I->flags & (TF_ONEWAY | TF_GOO | TF_HURT));
    for (int y = 0; y < 8; y++) {
        int yy = sy + y;
        if (yy < 0 || yy >= SCREEN_H) continue;
        for (int x = 0; x < 8; x++) {
            int xx = sx + x;
            if (xx < 0 || xx >= SCREEN_W) continue;
            uint8_t c = px[y * 8 + x];
            if (c == TRANSPARENT) continue;
            if (top && y == 0) c = I->hi;
            g_screen.px[yy * SCREEN_W + xx] = c;
        }
    }
    if (t == T_GOO || t == T_SLIME) {
        int w = (T / 8 + tx) % 4;
        gfx_hline(sx + w, sx + w + 2, sy + 1, I->hi);
    }
}

static void draw_level_small(int cx, int cy) {
    int ts = DD_TS;
    int x0 = cx / ts, y0 = cy / ts;
    bool strip = dd_lv.d.kind == LV_STRIP;
    /* backdrop */
    if (strip) {
        int c0 = iclamp(x0 / CHUNK_W, 0, dd_lv.d.n - 1), c1 = iclamp((x0 + SCREEN_W / ts + 1) / CHUNK_W, 0, dd_lv.d.n - 1);
        for (int c = c0; c <= c1; c++) {
            int sx0 = imax(0, c * CHUNK_W * ts - cx), sx1 = imin(SCREEN_W, (c + 1) * CHUNK_W * ts - cx);
            int mat = dd_lv.biome[c];
            if (dd_lv.town[c] >= 0) mat = mat == M_CELL ? M_CELL : mat;
            biome_back(mat, cx, cy, sx0, sx1);
        }
    } else if (dd_lv.d.kind == LV_SPECIAL) {
        biome_back(dd_lv.d.id == SP_DASHFUR ? M_FUR : dd_lv.d.id == SP_PUFFFUR ? M_DUST : dd_lv.d.id == SP_SPROCKET ? M_METAL : M_CELL, cx, cy, 0, SCREEN_W);
    } else gfx_cls(C_INK);
    for (int ty = y0; ty <= y0 + SCREEN_H / ts + 1; ty++)
        for (int tx = x0; tx <= x0 + SCREEN_W / ts + 1; tx++) {
            if (tx < 0 || ty < 0 || tx >= dd_lv.w || ty >= dd_lv.h) continue;
            int sx = tx * ts - cx, sy = ty * ts - cy;
            int b = dd_lv.bg[ty * dd_lv.w + tx];
            if (b && !strip) room_back_tile(b, tx, ty, sx, sy, ts);
            else if (b == BG_HOLLOW) {
                /* town houses: a lit window now and then */
                gfx_rect(sx, sy, ts, ts, C_NIGHT);
                if ((tx * 3 + ty) % 5 == 0) gfx_rect(sx + 2, sy + 2, 4, 4, dd_lamp_dark ? C_DUSK : C_AMBER);
            } else if (b == BG_DARK) gfx_rect(sx, sy, ts, ts, C_INK);
            int t = dd_lv.t[ty * dd_lv.w + tx];
            if (t) draw_tile8(t, tx, ty, sx, sy);
        }
    /* the room's big props, drawn over their tiles */
    if (dd_lv.d.kind == LV_AREA && dd_lv.d.id == AR_ROOM) {
        /* clock face */
        int ccx = 66 * ts - cx, ccy = 22 * ts - cy;
        gfx_circ(ccx, ccy, 28, C_CREAM);
        gfx_circb(ccx, ccy, 28, C_BROWN);
        for (int k = 0; k < 12; k++) gfx_pset(ccx + (int)(cosf((float)k * 0.5236f) * 24), ccy + (int)(sinf((float)k * 0.5236f) * 24), C_INK);
        float mm = (float)(dd_sv.clock_min % 60) / 60.0f * 6.283f - 1.5708f, hh = (float)(dd_sv.clock_min % 720) / 720.0f * 6.283f - 1.5708f;
        gfx_line(ccx, ccy, ccx + (int)(cosf(mm) * 22), ccy + (int)(sinf(mm) * 22), C_INK);
        gfx_line(ccx, ccy, ccx + (int)(cosf(hh) * 14), ccy + (int)(sinf(hh) * 14), C_INK);
        /* the lamp's light */
        if (!dd_lamp_dark) {
            int lx = 116 * ts + 4 - cx, ly = 24 * ts - cy;
            gfx_dither_circle(lx, ly, 18, C_YELLOW, 4);
        }
    }
}

/* the room at full size: every tile a 2-px cell */
static void draw_room_full(void) {
    for (int ty = 0; ty < ROOM_H; ty++)
        for (int tx = 0; tx < ROOM_W; tx++) {
            int t = dd_lv.t[ty * ROOM_W + tx], b = dd_lv.bg[ty * ROOM_W + tx];
            int c;
            if (t) {
                const TileInfo *I = &DD_TILE[t];
                c = I->base;
                if (!solid_t(lv_tile(&dd_lv, tx, ty - 1)) && (I->flags & TF_SOLID)) c = I->hi;
                else if ((tx * 7 + ty * 3) % 11 == 0) c = I->lo;
            } else {
                switch (b) {
                case BG_PAPER: c = ((tx / 3) % 2) ? C_TEAL : C_FOREST; break;
                case BG_DOOR: c = (tx % 9 == 7 || ty % 16 == 4) ? C_EARTH : C_BROWN; break;
                case BG_OUTLET: c = ((tx + ty) % 2) ? C_CREAM : C_LIGHT; break;
                case BG_FRIED: c = C_INK; break;
                case BG_CORD: c = C_INK; break;
                case BG_STALK: c = C_FOREST; break;
                case BG_CLOCK: c = C_CREAM; break;
                case BG_FRAME: c = ty < 25 ? C_SKY : C_LEAF; break;
                case BG_BRACKET: c = C_SLATE; break;
                case BG_BULB: c = dd_lamp_dark ? C_GREY : C_YELLOW; break;
                default: c = C_FOREST; break;
                }
            }
            gfx_rect(tx * 2, ty * 2, 2, 2, c);
        }
    /* a strip of light under the door, from the party */
    gfx_hline(14, 70, 179, (T / 20) % 2 ? C_YELLOW : C_AMBER);
    int ccx = 66 * 2, ccy = 22 * 2;
    float mm = (float)(dd_sv.clock_min % 60) / 60.0f * 6.283f - 1.5708f, hh = (float)(dd_sv.clock_min % 720) / 720.0f * 6.283f - 1.5708f;
    gfx_line(ccx, ccy, ccx + (int)(cosf(mm) * 6), ccy + (int)(sinf(mm) * 6), C_INK);
    gfx_line(ccx, ccy, ccx + (int)(cosf(hh) * 4), ccy + (int)(sinf(hh) * 4), C_INK);
    if (!dd_lamp_dark) gfx_dither_circle(116 * 2 + 1, 26 * 2, 14, C_YELLOW, 3);
    else gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, 4);
}

/* ------------------------------------------------------------------ */
/* entities                                                               */

static void draw_foe(const Ent *e, int sx, int sy) {
    if (e->sub >= F_FIRST_BOSS) { dd_draw_boss(e, sx, sy, T); return; }
    int fr = (T / 10 + (int)(e->x)) % 2;
    if (e->stun && !e->held) fr = 0;
    const Sprite *s = &dd_spr[S_FOE0 + e->sub * 2 + fr];
    int flags = (e->dir ? 0 : SPR_FLIPX) | (e->flip && e->stun ? SPR_FLIPY : 0);
    int ox = (e->w - s->w) / 2, oy = e->h - s->h;
    if (e->hurt_t > 0 && (T % 4) < 2) {
        spr_draw_ex(s, sx + ox, sy + oy, flags, NULL, C_WHITE);
        return;
    }
    if (e->poison && T % 8 < 4) {
        spr_draw_ex(s, sx + ox, sy + oy, flags, NULL, C_LIME);
        return;
    }
    if (e->frozen || dd_freeze_t() > 0) {
        spr_draw_ex(s, sx + ox, sy + oy, flags, NULL, C_ICE);
        return;
    }
    spr_draw(s, sx + ox, sy + oy, flags);
    if (e->cmd) gfx_pset(sx + e->w / 2, sy - 3, (T / 4) % 2 ? C_YELLOW : C_WHITE);
}

static void draw_ent(const Ent *e, int cx, int cy) {
    int sx = (int)e->x - cx, sy = (int)e->y - cy;
    if (sx < -60 || sx > SCREEN_W + 60 || sy < -60 || sy > SCREEN_H + 60) return;
    switch (e->kind) {
    case EK_DASH: {
        if (dd_scale == SC_FULL) { spr_draw(&dd_spr[S_DASHBIG], sx, sy - 1, e->dir ? 0 : SPR_FLIPX); break; }
        int sp = e->state == 5 || (e->state == 4) ? S_DASH_WINGS : (fabsf(e->vx) > 0.1f ? (T / 8) % 2 ? S_DASH1 : S_DASH2 : S_DASH_SIT);
        if (e->state == 9) sp = S_DASH1;
        const Sprite *s = &dd_spr[sp];
        spr_draw(s, sx + (e->w - s->w) / 2, sy + e->h - s->h, e->dir ? 0 : SPR_FLIPX);
        if (dd_has(U_PLATE)) gfx_hline(sx + 3, sx + 8, sy + e->h - 5, C_LIGHT);
        if (dd_sniff_x >= 0 && e->state == 0 && (T / 20) % 2) {
            int ax = dd_sniff_x < e->x ? -1 : 1;
            text_draw(ax < 0 ? GLYPH_LEFT : GLYPH_RIGHT, sx + 2, sy - 10, C_YELLOW);
        }
        break;
    }
    case EK_NPC:
        if (e->sub == N_BUBBLE) {
            dd_draw_npc(N_FERN, sx, sy + (int)(sinf((float)T * 0.05f) * 2), 0, T);
            gfx_circb(sx + 4, sy + 7 + (int)(sinf((float)T * 0.05f) * 2), 9, C_ICE);
            gfx_pset(sx + 1, sy + 1, C_WHITE);
            break;
        }
        dd_draw_npc(e->sub, sx, sy, e->dir, T);
        break;
    case EK_FOE: draw_foe(e, sx, sy); break;
    case EK_OBJ: dd_draw_obj(e->sub, sx + (e->w - 8) / 2, sy + e->h - 8, e->param); break;
    case EK_PICK: {
        int bob = (int)(sinf((float)(T + (int)e->x) * 0.1f) * 1.5f);
        int sp = e->sub == P_GLINT1 ? S_GLINT1 : e->sub == P_GLINT5 ? S_GLINT5 : e->sub == P_GLINT50 ? S_GLINT50 : e->sub == P_HEART ? S_HEART : S_GIFT;
        if (e->sub == P_UPGRADE && e->param >= U_EGG0) sp = S_EGG;
        else if (e->sub == P_UPGRADE && e->param >= U_HEART0) sp = e->param - U_HEART0 < 6 ? S_HEART : S_HALF;
        const Sprite *s = &dd_spr[sp];
        spr_draw(s, sx + (e->w - s->w) / 2, sy + e->h - s->h + bob, 0);
        if ((e->sub == P_GLINT50 || e->sub == P_UPGRADE) && (T / 6) % 4 == 0) gfx_pset(sx + 1, sy - 1 + bob, C_WHITE);
        break;
    }
    case EK_SHOT: {
        if (e->sub >= 5) { spr_draw(&dd_spr[S_DROP], sx, sy, 0); if (e->sub == 6) gfx_rect(sx, sy + 1, 3, 3, C_AMBER); break; }
        int sp = e->sub == 2 ? S_SPELL : e->sub == 3 ? S_BOLT : S_PEA;
        if (e->sub == 3) gfx_circ(sx + 3, sy + 3, 3, C_SLATE), gfx_pset(sx + 2, sy + 2, C_GREY);
        else if (e->sub == 4) gfx_rect(sx, sy + 2, 6, 3, (T / 3) % 2 ? C_ORANGE : C_YELLOW);
        else spr_draw(&dd_spr[sp], sx, sy, 0);
        break;
    }
    case EK_DOOR: {
        /* doorways drawn by what they are */
        if (dd_door_is(e->param, "seam")) { if ((T / 10) % 3 == 0) gfx_pset(sx + 4, sy + 12, C_VIOLET); break; }
        if (dd_door_is(e->param, "owl")) break;
        if (dd_door_is(e->param, "shrine")) { spr_draw(&dd_spr[S_DOOR], sx, sy, 0); gfx_rect(sx + 2, sy + 2, 4, 3, C_YELLOW); break; }
        if (dd_door_is(e->param, "grate")) {
            if (!dd_flag(FL_GRATE_OPEN)) for (int k = 0; k < 4; k++) gfx_vline(sx + 1 + k * 2, sy, sy + 15, C_BROWN);
            break;
        }
        if (dd_door_is(e->param, "lair") && !dd_flag(FL_WORM_FRIENDS)) { gfx_circ(sx + 4, sy + 9, 5, C_EARTH); gfx_circb(sx + 4, sy + 9, 5, C_NIGHT); break; }
        spr_draw(&dd_spr[S_DOOR], sx, sy, 0);
        break;
    }
    case EK_STAND: {
        int it[3], pr[3];
        dd_shop_items(e->param, it, pr);
        int k = iclamp(e->sub, 0, 2);
        spr_draw(&dd_spr[S_STAND], sx - 1, sy + 4, 0);
        if (it[k] >= 100) dd_draw_obj(it[k] - 100, sx, sy - 5, 0);
        else if (it[k] >= 0) {
            if (dd_has(it[k])) tiny_draw("SOLD", sx - 3, sy, C_GREY);
            else spr_draw(&dd_spr[S_GIFT], sx, sy - 5, 0);
        }
        char b[8];
        snprintf(b, sizeof b, "%d", pr[k]);
        if (it[k] >= 0 && !(it[k] < 100 && dd_has(it[k]))) tiny_center(b, sx + 4, sy - 12, C_YELLOW);
        break;
    }
    case EK_DRIP: break;
    }
}

static void draw_dot(int cx, int cy) {
    int sx = (int)dd_p.x - cx, sy = (int)dd_p.y - cy;
    if (dd_hurt_t > 0 && (dd_hurt_t / 3) % 2 && dd_dead_t == 0) return;
    int flags = dd_p.facing ? 0 : SPR_FLIPX;
    if (dd_scale == SC_FULL) {
        int sp = !dd_p.ground ? S_DOTBIG_JUMP : fabsf(dd_p.vx) > 0.1f ? ((T / 7) % 2 ? S_DOTBIG_WALK1 : S_DOTBIG_WALK2) : S_DOTBIG_STAND;
        const Sprite *s = &dd_spr[sp];
        spr_draw(s, sx + (dd_p.w - s->w) / 2, sy + dd_p.h - s->h, flags);
        return;
    }
    int sp = S_DOT_STAND;
    if (dd_dead_t > 0) sp = S_DOT_HURT;
    else if (dd_kick_t() > 0) sp = S_DOT_KICK;
    else if (dd_lift_t() > 0 || dd_p.crouch) sp = S_DOT_CROUCH;
    else if (dd_carry >= 0) sp = !dd_p.ground ? S_DOT_LIFT : S_DOT_LIFT;
    else if (!dd_p.ground) sp = S_DOT_JUMP;
    else if (fabsf(dd_p.vx) > 0.1f) sp = (T / (dd_p.sprint ? 4 : 7)) % 2 ? S_DOT_WALK1 : S_DOT_WALK2;
    const Sprite *s = &dd_spr[sp];
    spr_draw(s, sx + (dd_p.w - s->w) / 2, sy + dd_p.h - s->h, flags);
    if (dd_p.sprint && dd_p.ground && T % 3 == 0) dd_part(dd_p.x + (dd_p.facing ? 0 : dd_p.w), dd_p.y + dd_p.h - 1, 0, -0.2f, 8, C_LIGHT);
}

/* charge rings for shrinking and growing */
static void draw_charge(int cx, int cy) {
    int t = dd_shrink_t ? dd_shrink_t : dd_grow_t;
    if (t < 8) return;
    int x = (int)(dd_p.x + dd_p.w / 2) - cx, y = (int)(dd_p.y + dd_p.h / 2) - cy;
    int r = dd_shrink_t ? 22 - t * 18 / 90 : 4 + t * 18 / 90;
    gfx_circb(x, y, r, (T / 3) % 2 ? C_YELLOW : C_WHITE);
    gfx_circb(x, y, r + (dd_shrink_t ? 3 : -3), C_AMBER);
}

static void iris(void) {
    if (!dd_trans) return;
    int half = 18, t = dd_trans > half ? 36 - dd_trans : dd_trans; /* 0..18..0 */
    int r = (int)((1.0f - (float)t / (float)half) * 220.0f);
    int cx = (int)(dd_p.x + dd_p.w / 2 - (dd_scale == SC_FULL ? 0 : dd_cam_x));
    int cy = (int)(dd_p.y + dd_p.h / 2 - (dd_scale == SC_FULL ? 0 : dd_cam_y));
    if (dd_trans_kind == 1) { cx = SCREEN_W / 2; cy = SCREEN_H / 2; }
    for (int y = 0; y < SCREEN_H; y++) {
        int dy = y - cy;
        if (r <= 0 || iabs(dy) >= r) { gfx_hline(0, SCREEN_W - 1, y, C_INK); continue; }
        int w = (int)sqrtf((float)(r * r - dy * dy));
        if (cx - w > 0) gfx_hline(0, cx - w - 1, y, C_INK);
        if (cx + w < SCREEN_W - 1) gfx_hline(cx + w + 1, SCREEN_W - 1, y, C_INK);
    }
}

void dd_draw_world(void) {
    T++;
    int sh = dd_shake();
    if (dd_scale == SC_FULL) {
        draw_room_full();
        for (int i = 0; i < DD_MAX_ENTS; i++)
            if (dd_ent[i].alive && dd_ent[i].kind == EK_DASH) draw_ent(&dd_ent[i], 0, 0);
        draw_dot(0, 0);
        draw_charge(0, 0);
        dd_parts_draw(0, 0);
        iris();
        return;
    }
    int cx = (int)dd_cam_x + sh, cy = (int)dd_cam_y;
    draw_level_small(cx, cy);
    /* draw order: doors, stands, npcs, picks, objects, foes, Dash, Dot, shots */
    static const int ORDER[] = {EK_DOOR, EK_STAND, EK_NPC, EK_PICK, EK_OBJ, EK_FOE, EK_DASH};
    for (int o = 0; o < ARRAY_LEN(ORDER); o++)
        for (int i = 0; i < DD_MAX_ENTS; i++)
            if (dd_ent[i].alive && dd_ent[i].kind == ORDER[o] && !dd_ent[i].held) draw_ent(&dd_ent[i], cx, cy);
    draw_dot(cx, cy);
    if (dd_carry >= 0 && dd_ent[dd_carry].alive) draw_ent(&dd_ent[dd_carry], cx, cy);
    for (int i = 0; i < DD_MAX_ENTS; i++)
        if (dd_ent[i].alive && dd_ent[i].kind == EK_SHOT) draw_ent(&dd_ent[i], cx, cy);
    draw_charge(cx, cy);
    dd_parts_draw(cx, cy);
    /* the lamp off: the whole room goes dim */
    if (dd_lamp_dark && dd_lv.d.kind == LV_AREA && dd_lv.d.id == AR_ROOM) gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, 5);
    iris();
}

/* ------------------------------------------------------------------ */
/* the HUD                                                                */

static int place_t;
static const char *last_place;

void dd_draw_hud(void) {
    /* top right: hearts, energy, glints */
    int max = dd_hp_max, hp = dd_hp, nh = (max + 1) / 2;
    char b[48];
    int x0 = SCREEN_W - 4 - nh * 8;
    for (int k = 0; k < nh; k++) {
        int x = x0 + k * 8, y = 3;
        int full = hp - k * 2;
        spr_draw(&dd_spr[S_HEART], x, y, 0);
        if (full <= 0) spr_draw_ex(&dd_spr[S_HEART], x, y, 0, NULL, C_DUSK);
        else if (full == 1) { spr_draw_ex(&dd_spr[S_HEART], x, y, 0, NULL, C_DUSK); spr_draw(&dd_spr[S_HALF], x, y, 0); }
        if (max - k * 2 == 1) gfx_rect(x + 4, y, 4, 7, C_INK);
    }
    /* energy: a little bolt and the level */
    int ex = SCREEN_W - 58, ey = 12;
    gfx_line(ex + 3, ey, ex, ey + 3, C_YELLOW);
    gfx_hline(ex, ex + 3, ey + 3, C_YELLOW);
    gfx_line(ex + 3, ey + 3, ex, ey + 6, C_YELLOW);
    snprintf(b, sizeof b, "%d", dd_pep());
    tiny_draw(b, ex + 6, ey + 1, C_LIME);
    snprintf(b, sizeof b, "%d", dd_sv.glints);
    spr_draw(&dd_spr[S_GLINT5], SCREEN_W - 40, 11, 0);
    text_shadow(b, SCREEN_W - 31, 11, C_YELLOW, C_INK);
    /* top left: the size, the clock */
    static const char *SZ[4] = {"FULL", "SMALL", "MICRO", "DEEP"};
    for (int k = 0; k < 4; k++) {
        int x = 4 + k * 6;
        if (k == dd_scale) gfx_rect(x, 4, 4, 4, C_WHITE);
        else gfx_rectb(x, 4, 4, 4, k <= (dd_has(U_TONIC2) ? 3 : dd_has(U_TONIC1) ? 2 : 1) ? C_GREY : C_DUSK);
    }
    tiny_draw(SZ[dd_scale], 30, 4, C_GREY);
    /* carried and stored */
    int ix = 4;
    if (dd_scale == SC_FULL && dd_sv.carry) { gfx_rectb(ix - 1, 12, 10, 10, C_SLATE); dd_draw_obj(dd_sv.carry - 1, ix, 13, dd_sv.carry_param); }
    if (dd_has(U_SATCHEL1))
        for (int k = 0; k < (dd_has(U_SATCHEL2) ? 2 : 1); k++) {
            int x = ix + 14 + k * 11;
            gfx_rectb(x - 1, 12, 10, 10, C_BROWN);
            if (dd_sv.satchel[k]) dd_draw_obj(dd_sv.satchel[k] - 1, x, 13, dd_sv.satchel_param[k]);
        }
    /* the clock */
    int m = dd_sv.clock_min % 720;
    snprintf(b, sizeof b, "%d:%02d", m / 60 == 0 ? 12 : m / 60, m % 60);
    tiny_draw(b, 58, 4, C_LIGHT);
    /* where we are */
    const char *pl = dd_place_name();
    if (pl != last_place && (last_place == NULL || strcmp(pl, last_place))) { place_t = 150; last_place = pl; }
    static char keep[48];
    if (place_t > 0) {
        place_t--;
        snprintf(keep, sizeof keep, "%s", pl);
        last_place = keep;
        if (place_t > 20 || (place_t / 3) % 2) {
            int w = text_width(keep);
            gfx_rect(SCREEN_W / 2 - w / 2 - 4, 20, w + 8, 11, C_INK);
            text_center(keep, SCREEN_W / 2, 22, C_CREAM);
        }
    }
    const char *msg = dd_message();
    if (msg && !dd_dialog_active()) {
        int w = imin(text_width(msg), 300);
        gfx_rect(SCREEN_W / 2 - w / 2 - 4, 162, w + 8, 12, C_INK);
        text_center(msg, SCREEN_W / 2, 164, C_WHITE);
    }
}

/* ------------------------------------------------------------------ */
/* the bag                                                                */

/* the things Dot owns, in the order they're shown */
static int bag_list(int *out) {
    int n = 0;
    for (int u = 0; u < U_ABILITIES; u++) if (dd_has(u)) out[n++] = u;
    for (int k = 0; k < 8; k++) if ((dd_sv.hearts_got >> k) & 1) out[n++] = U_HEART0 + k;
    for (int k = 0; k < 8; k++) if ((dd_sv.eggs_got >> k) & 1) out[n++] = U_EGG0 + k;
    return n;
}
int dd_bag_count(void) { int l[U_TOTAL]; return bag_list(l); }

void dd_draw_bag(int page) {
    int list[U_TOTAL], n = bag_list(list);
    gfx_rect(0, 0, SCREEN_W, 58, C_INK);
    gfx_hline(0, SCREEN_W - 1, 58, C_AMBER);
    char b[64];
    snprintf(b, sizeof b, "UPGRADES %d/%d", dd_upgrade_count(), U_TOTAL);
    tiny_draw(b, 4, 3, C_CREAM);
    snprintf(b, sizeof b, "BIG GLINTS %d/24", dd_sv.bigs_found);
    tiny_draw(b, 100, 3, C_YELLOW);
    for (int i = 0; i < n; i++) {
        int x = 4 + (i % 26) * 12, y = 12 + (i / 26) * 12;
        int u = list[i];
        if (u >= U_EGG0) spr_draw(&dd_spr[S_EGG], x + 1, y, 0);
        else if (u >= U_HEART0) spr_draw(&dd_spr[u - U_HEART0 < 6 ? S_HEART : S_HALF], x, y + 1, 0);
        else spr_draw(&dd_spr[S_GIFT], x, y, 0);
        if (i == page) gfx_rectb(x - 2, y - 2, 12, 12, (engine_frame() / 8) % 2 ? C_WHITE : C_YELLOW);
    }
    if (n > 0) {
        int u = list[iclamp(page, 0, n - 1)];
        const char *nm = u < U_ABILITIES ? DD_UPNAME[u] : u < U_EGG0 ? (u - U_HEART0 < 6 ? "HEART BUTTON" : "HALF BUTTON") : "PEP EGG";
        const char *ds = u < U_ABILITIES ? DD_UPDESC[u] : u < U_EGG0 ? "MORE HEARTS" : "THROWS, KICKS AND FLIPS HIT HARDER";
        text_draw(nm, 4, 38, C_YELLOW);
        text_draw(ds, 4 + text_width(nm) + 8, 38, C_WHITE);
    }
    tiny_draw(GLYPH_LEFT GLYPH_RIGHT " LOOK   " GLYPH_DOWN " BACK", 4, 50, C_GREY);
}

/* ------------------------------------------------------------------ */
/* title, ending, label                                                    */

void dd_draw_title(int sel, bool can_continue, int t) {
    T = t;
    gfx_cls(C_FOREST);
    for (int x = 0; x < SCREEN_W; x += 6) gfx_rect(x, 0, 3, 140, C_TEAL);
    gfx_rect(0, 140, SCREEN_W, 40, C_BROWN);
    for (int x = 0; x < SCREEN_W; x += 18) gfx_vline(x, 140, 179, C_EARTH);
    gfx_rect(20, 30, 70, 110, C_EARTH);
    gfx_rectb(20, 30, 70, 110, C_NIGHT);
    gfx_circ(80, 88, 3, C_YELLOW);
    gfx_hline(20, 90, 139, (t / 20) % 2 ? C_YELLOW : C_AMBER);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_center("DOT & DASH", 196, 22, 3, grad, 4, C_INK, C_NIGHT);
    tiny_center("BEAMDOWN SOFTWORKS 1989", 196, 52, C_CREAM);
    /* a magnifying glass over the floor, with Dot and Dash inside */
    int mx = 196, my = 104;
    gfx_circ(mx, my, 26, C_TEAL);
    gfx_circb(mx, my, 26, C_LIGHT);
    gfx_circb(mx, my, 27, C_SLATE);
    gfx_line(mx + 18, my + 18, mx + 34, my + 34, C_EARTH);
    gfx_line(mx + 19, my + 18, mx + 35, my + 34, C_EARTH);
    gfx_rect(mx - 20, my + 10, 40, 6, C_WINE);
    spr_draw(&dd_spr[(t / 12) % 2 ? S_DOT_WALK1 : S_DOT_STAND], mx - 8, my - 4, 0);
    spr_draw(&dd_spr[(t / 12) % 2 ? S_DASH1 : S_DASH2], mx + 2, my + 3, 0);
    int y = 144;
    if (can_continue) {
        text_center(sel == 0 ? GLYPH_RIGHT " CONTINUE" : "CONTINUE", 196, y, sel == 0 ? C_YELLOW : C_CREAM);
        text_center(sel == 1 ? GLYPH_RIGHT " NEW GAME" : "NEW GAME", 196, y + 11, sel == 1 ? C_YELLOW : C_CREAM);
    } else if ((t / 25) % 2) text_center("PRESS " GLYPH_A, 196, y + 5, C_YELLOW);
    tiny_draw(GLYPH_B " LIBRARY", 4, 172, C_TAN);
}

static const char *const END_GOLD[] = {
    "THE GREAT DOOR SWINGS OPEN.",
    "DOT GROWS BACK TO HER OWN SIZE ON THE LANDING, WITH DASH AT HER HEELS.",
    "DOWNSTAIRS THE PARTY STOPS DEAD. A LITTLE GIRL AND A DOG HAVE JUST WALKED OUT OF THE LUMBER ROOM.",
    "THEY MARCH STRAIGHT TO THE CAKE.",
    "OTTO IS GROUNDED UNTIL CHRISTMAS.",
};
static const char *const END_TRUE[] = {
    "TOCK'S CLOCK CHIMES, AND THE WHOLE ROOM SEEMS TO BREATHE OUT.",
    "THE GREAT DOOR OPENS BY ITSELF, AND STAYS OPEN.",
    "BEHIND DOT, THE TOWNS IN THE RUG AND THE POTS AND THE WALLS LIGHT THEIR LAMPS AND WAVE.",
    "DOT AND DASH GO DOWN TO THE PARTY. EVERY SPECK OF THE HOUSE IS A LITTLE BRIGHTER.",
    "AND OTTO? OTTO IS STILL GROUNDED.",
};
static const char *const CREDITS[] = {
    "DOT & DASH", "", "A BEAMDOWN SOFTWORKS GAME", "1989", "", "", "DESIGN", "THE BEAMDOWN TEAM", "",
    "WORLD AND TOWNS", "THE BEAMDOWN TEAM", "", "PIXELS", "THE BEAMDOWN TEAM", "", "MUSIC", "THE BEAMDOWN TEAM", "",
    "THE PEOPLE OF THE ROOM", "GRANNY THIMBLE", "PROFESSOR CRUMB", "MOTHER FLUFF", "ELDER TALLOW", "MAYOR FERN",
    "CAPTAIN PEAT", "QUEEN WRIGGLA", "OLD CAP", "THE HIGH LUMEN", "TOCK", "NIB", "AND QUEEN TABITHA", "", "",
    "A TRIBUTE TO MINI & MAX", "UFO 50 #45", "", "THANK YOU FOR PLAYING",
};

void dd_draw_ending(int kind, int t, int cy) {
    T = t;
    gfx_cls(kind ? C_NAVY : C_INK);
    if (cy < 0) {
        static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
        ui_fancy_center(kind ? "THE ROOM IS IN BALANCE" : "ESCAPED!", SCREEN_W / 2, 50, 2, grad, 4, C_INK, C_NIGHT);
        spr_draw_scaled(&dd_spr[S_DOT_STAND], 138, 90, 3, 0);
        spr_draw_scaled(&dd_spr[S_DASH_SIT], 166, 106, 3, 0);
        tiny_center("PRESS " GLYPH_A, SCREEN_W / 2, 160, (t / 20) % 2 ? C_YELLOW : C_AMBER);
        return;
    }
    const char *const *L = kind ? END_TRUE : END_GOLD;
    int shown = imin(5, t / 110 + 1);
    for (int k = 0; k < shown; k++) text_wrap(L[k], 20, 14 + k * 24, 280, k == shown - 1 ? C_WHITE : C_GREY, 9);
    if (t > 600) {
        int y0 = 180 - (cy - 540) / 2;
        for (int k = 0; k < ARRAY_LEN(CREDITS); k++) {
            int y = y0 + k * 12;
            if (y > 130 && y < 176) text_center(CREDITS[k], SCREEN_W / 2, y, k == 0 ? C_YELLOW : C_CREAM);
        }
    }
    spr_draw(&dd_spr[(t / 10) % 2 ? S_DOT_WALK1 : S_DOT_WALK2], (t / 2) % 360 - 20, 150, 0);
    spr_draw(&dd_spr[(t / 8) % 2 ? S_DASH1 : S_DASH2], (t / 2) % 360 - 34, 157, 0);
}

void dd_draw_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_FOREST);
    for (int xx = 0; xx < w; xx += 6) gfx_rect(x + xx, y, 3, h, C_TEAL);
    gfx_rect(x, y + 46, w, h - 46, C_BROWN);
    gfx_rect(x, y + 44, w, 3, C_WINE);
    /* the pot, the bookshelf and the door */
    gfx_rect(x + 8, y + 6, 24, 38, C_EARTH);
    gfx_circ(x + 28, y + 26, 2, C_YELLOW);
    gfx_rect(x + w - 30, y + 4, 26, 40, C_BROWN);
    for (int k = 0; k < 4; k++) gfx_rect(x + w - 28, y + 8 + k * 9, 22, 1, C_TAN);
    gfx_rect(x + 50, y + 30, 16, 14, C_ORANGE);
    gfx_rect(x + 57, y + 14, 2, 16, C_FOREST);
    gfx_rect(x + 52, y + 16, 5, 2, C_LEAF);
    gfx_rect(x + 59, y + 22, 5, 2, C_LEAF);
    /* a lens over the rug */
    int mx = x + 94, my = y + 30;
    gfx_circ(mx, my, 16, C_WINE);
    gfx_circb(mx, my, 16, C_LIGHT);
    gfx_line(mx + 11, my + 11, mx + 20, my + 20, C_EARTH);
    spr_draw(&dd_spr[(t / 12) % 2 ? S_DOT_WALK1 : S_DOT_STAND], mx - 8, my - 8, 0);
    spr_draw(&dd_spr[(t / 12) % 2 ? S_DASH1 : S_DASH2], mx, my - 1, 0);
    if ((t / 6) % 5 == 0) gfx_pset(mx - 10, my - 10, C_WHITE);
}
