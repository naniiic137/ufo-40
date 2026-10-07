/* RESTLESS - drawing: the three stages' ground and skies, everything that
 * moves, the HUD, the Low Glow and the screens around a run. */
#include "rsl.h"

int rsl_main_h(void);
char rsl_hint(int tx, int ty);

static const uint8_t TITLE_GRAD[3] = {C_YELLOW, C_ORANGE, C_RED};

static int stage_of(void) { return rg.half / 2; } /* 0 1 2 */

static uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

/* ------------------------------------------------------------------------ */
/* skies                                                                    */

static void draw_sky(void) {
    int st = stage_of();
    int t = rg.frame_t;
    if (rg.pit >= 0) {
        gfx_cls(C_INK);
        for (int y = 0; y < SCREEN_H; y += 6) gfx_dither(0, y, SCREEN_W, 3, C_NIGHT, 6);
        return;
    }
    if (st == 0) {
        /* the greenwood at dusk */
        gfx_cls(C_TEAL);
        gfx_dither(0, 0, SCREEN_W, 60, C_JADE, 6);
        gfx_dither(0, 100, SCREEN_W, 80, C_FOREST, 10);
        int off = (rg.cam_x / 4) % 64;
        for (int x = -64; x < SCREEN_W + 64; x += 64) {
            int bx = x - off;
            gfx_circ(bx + 20, 92, 26, C_FOREST);
            gfx_circ(bx + 50, 104, 22, C_FOREST);
            gfx_rect(bx + 18, 100, 6, 80, C_EARTH);
        }
        int off2 = (rg.cam_x / 2) % 96;
        for (int x = -96; x < SCREEN_W + 96; x += 96) {
            int bx = x - off2;
            gfx_rect(bx + 30, 60, 5, 120, C_NIGHT);
            gfx_circ(bx + 32, 58, 18, C_NIGHT);
            gfx_circ(bx + 70, 120, 14, C_NIGHT);
        }
    } else if (st == 1) {
        /* the warrens */
        gfx_cls(C_NIGHT);
        int off = (rg.cam_x / 3) % 48;
        for (int x = -48; x < SCREEN_W + 48; x += 48) {
            int bx = x - off;
            for (int k = 0; k < 6; k++) gfx_line(bx + 10 + k, 0, bx + 20, 30 + (k * 7) % 20, C_DUSK);
            gfx_rect(bx + 30, 120, 10, 60, C_DUSK);
            gfx_circ(bx + 35, 120, 6, C_DUSK);
        }
        gfx_dither(0, 150, SCREEN_W, 30, C_MAROON, 3);
    } else {
        /* the high falls under the moon */
        gfx_cls(C_NAVY);
        gfx_dither(0, 0, SCREEN_W, 70, C_NIGHT, 8);
        for (int i = 0; i < 30; i++) {
            int sx = (int)(hash2(i, 7) % SCREEN_W), sy = (int)(hash2(i, 9) % 80);
            if ((t / 20 + i) % 7) gfx_pset(sx, sy, C_LIGHT);
        }
        gfx_circ(250 - (rg.cam_x / 16) % 40, 30, 12, C_CREAM);
        gfx_circ(254 - (rg.cam_x / 16) % 40, 27, 11, C_NAVY);
        int off = (rg.cam_x / 4) % 160;
        for (int x = -160; x < SCREEN_W + 160; x += 160) {
            int bx = x - off;
            gfx_rect(bx + 40, 50, 60, 130, C_SLATE);
            for (int k = 0; k < 4; k++) {
                int fx = bx + 55 + k * 8;
                int ph = (t * 2 + k * 13) % 40;
                gfx_vline(fx, 50 + ph, 180, C_SKY);
            }
        }
    }
}

/* ------------------------------------------------------------------------ */
/* tiles                                                                    */

static void rock_tile(int x, int y, int tx, int ty) {
    int st = stage_of();
    uint8_t base = st == 0 ? C_EARTH : st == 1 ? C_BROWN : C_SLATE;
    uint8_t dark = st == 0 ? C_BROWN : st == 1 ? C_MAROON : C_DUSK;
    uint8_t top = st == 0 ? C_LEAF : st == 1 ? C_TAN : C_TEAL;
    if (rg.pit >= 0 && ty >= rsl_main_h()) { base = C_DUSK; dark = C_NIGHT; top = C_SLATE; }
    gfx_rect(x, y, 16, 16, base);
    uint32_t h = hash2(tx, ty);
    for (int k = 0; k < 3; k++) gfx_pset(x + (int)((h >> (k * 5)) % 15), y + (int)((h >> (k * 5 + 3)) % 15), dark);
    gfx_rect(x + (int)(h % 9), y + 9 + (int)((h >> 8) % 4), 4, 2, dark);
    char up = rsl_tile(tx, ty - 1);
    if (up == '.' || up == '%' || up == 'I' || up == 'w' || up == '~') {
        gfx_rect(x, y, 16, 3, top);
        gfx_pset(x + (int)(h % 16), y + 3, top);
        gfx_pset(x + (int)((h >> 4) % 16), y + 3, top);
    }
}

static void brick_tile(int x, int y, int tx, int ty) {
    int st = stage_of();
    uint8_t base = st == 0 ? C_TAN : st == 1 ? C_HIDE : C_GREY;
    uint8_t line = st == 0 ? C_EARTH : st == 1 ? C_BROWN : C_SLATE;
    gfx_rect(x, y, 16, 16, base);
    gfx_hline(x, x + 15, y + 7, line);
    gfx_hline(x, x + 15, y + 15, line);
    int o = (ty & 1) ? 8 : 0;
    gfx_vline(x + o, y, y + 7, line);
    gfx_vline(x + ((o + 8) & 15), y + 8, y + 15, line);
    if (hash2(tx, ty) % 5 == 0) gfx_pset(x + 4, y + 3, C_LIGHT);
}

static void draw_tiles(void) {
    int x0 = rg.cam_x / RSL_TILE, y0 = rg.cam_y / RSL_TILE;
    int t = rg.frame_t;
    for (int ty = y0; ty <= y0 + SCREEN_H / RSL_TILE + 1; ty++)
        for (int tx = x0; tx <= x0 + SCREEN_W / RSL_TILE + 1; tx++) {
            if (tx < 0 || tx >= rg.mw || ty < 0 || ty >= rg.mh) continue;
            char c = rg.map[ty][tx];
            int x = tx * RSL_TILE, y = ty * RSL_TILE;
            switch (c) {
            case '#': rock_tile(x, y, tx, ty); break;
            case 'B': brick_tile(x, y, tx, ty); break;
            case 'D':
                gfx_rect(x, y, 16, 6, C_BROWN);
                gfx_hline(x, x + 15, y, C_TAN);
                gfx_vline(x + 5, y, y + 5, C_EARTH);
                gfx_vline(x + 11, y, y + 5, C_EARTH);
                gfx_rect(x + 7, y + 6, 2, 10, C_EARTH);
                break;
            case '=':
                gfx_rect(x, y, 16, 4, C_BROWN);
                gfx_hline(x, x + 15, y, C_TAN);
                gfx_vline(x + 15, y, y + 3, C_EARTH);
                break;
            case '-':
                gfx_rect(x, y + 1, 16, 5, C_EARTH);
                gfx_hline(x, x + 15, y + 1, C_TAN);
                gfx_circb(x + 2, y + 3, 2, C_BROWN);
                break;
            case 'F': {
                brick_tile(x, y, tx, ty);
                gfx_rect(x + 3, y + 3, 3, 3, C_INK);
                gfx_rect(x + 10, y + 3, 3, 3, C_INK);
                gfx_rect(x + 4, y + 9, 8, 4, C_INK);
                gfx_rect(x + 5, y + 10, 6, 2, C_RED);
                break;
            }
            case 'S':
                brick_tile(x, y, tx, ty);
                gfx_rect(x + 2, y + 6, 12, 4, C_INK);
                gfx_hline(x + 3, x + 12, y + 7, C_GREY);
                break;
            case 'w': {
                gfx_rect(x, y, 16, 16, C_BLUE);
                for (int k = 0; k < 4; k++) {
                    int ph = (t * 3 + k * 9 + tx * 5) % 16;
                    gfx_vline(x + 2 + k * 4, y + ph, y + imin(15, ph + 6), C_SKY);
                }
                break;
            }
            case '~':
                gfx_rect(x, y + 4, 16, 12, C_BLUE);
                for (int k = 0; k < 16; k++) gfx_pset(x + k, y + 3 + ((k + t / 6) % 4 == 0), C_ICE);
                break;
            case '%':
                gfx_dither(x, y, 16, 16, stage_of() == 0 ? C_FOREST : C_DUSK, 9);
                break;
            case 'I':
                gfx_rect(x + 5, y, 6, 16, stage_of() == 0 ? C_EARTH : C_SLATE);
                gfx_vline(x + 6, y, y + 15, C_BROWN);
                break;
            default: break;
            }
        }
}

/* things fixed in the map: torches, traps, holes, the pit's writing */
static void draw_spawns(void) {
    int t = rg.frame_t;
    int mh = rsl_main_h();
    for (int i = 0; i < rg.nspawn; i++) {
        const RslSpawn *s = &rg.spawn[i];
        int x = s->tx * RSL_TILE, y = s->ty * RSL_TILE;
        if (!rsl_in_view(x + 8, y + 8, 32)) continue;
        switch (s->type) {
        case SP_TORCH:
            spr_draw(&rsl_spr[s->used ? SPR_TORCH_OUT : (t / 6 + i) % 2 ? SPR_TORCH0 : SPR_TORCH1], x + 3, y, 0);
            if (!s->used && rg.deaths >= 4 && s->b > 120) gfx_circb(x + 8, y + 2, 5, C_RED);
            break;
        case SP_BUTTON:
            gfx_rect(x + 3, y + 13, 10, 3, s->t > 0 ? C_RED : C_AMBER);
            gfx_hline(x + 3, x + 12, y + 13, C_YELLOW);
            break;
        case SP_ROCKFALL:
            gfx_rect(x + 3 + (s->b > 0 ? (t % 2) : 0), y, 10, 7, C_BROWN);
            gfx_hline(x + 3, x + 12, y + 6, C_EARTH);
            break;
        case SP_FIREWALL:
            if (s->b >= 40 && s->b < 70 && t % 4 < 2) gfx_rect(x + (s->a > 0 ? 12 : 0), y + 9, 4, 4, C_ORANGE);
            break;
        case SP_FOE:
            if (s->foe == FO_TOAD) {
                gfx_rect(x + 2, y + 13, 12, 3, C_INK);
                gfx_hline(x + 1, x + 14, y + 12, C_EARTH);
            }
            break;
        case SP_SECRET:
            if (s->a == 'Z') {
                gfx_rect(x, y - 16, 16, 32, C_NIGHT);
                gfx_rectb(x, y - 16, 16, 32, (t / 8) % 2 ? C_YELLOW : C_AMBER);
                tiny_draw("OUT", x + 1, y - 26, C_YELLOW);
            }
            break;
        default: break;
        }
    }
    /* the writing on the first pit's wall of the falls, for the many-times dead */
    if (rg.pit >= 0 && rg.deaths >= 5)
        for (int ty = mh; ty < rg.mh; ty++)
            for (int tx = rg.pit * RSL_SW; tx < rg.pit * RSL_SW + RSL_SW; tx++)
                if (rsl_hint(tx, ty) == 'Y') {
                    text_draw("NOTHING STAYS BURIED", tx * RSL_TILE, ty * RSL_TILE, C_VIOLET);
                }
}

static void draw_lifts(void) {
    for (int i = 0; i < rg.nlift; i++) {
        const RslLift *l = &rg.lift[i];
        int x = RSL_PX(l->x), y = RSL_PX(l->y);
        gfx_rect(x, y, l->w, 6, C_BROWN);
        gfx_hline(x, x + l->w - 1, y, C_TAN);
        gfx_rect(x + 2, y + 6, l->w - 4, 2, C_EARTH);
        if (l->vertical) {
            gfx_vline(x + 4, y - 200, y, C_GREY);
            gfx_vline(x + l->w - 5, y - 200, y, C_GREY);
        }
    }
}

/* ------------------------------------------------------------------------ */

static void draw_foe(const RslFoe *f) {
    int x = RSL_PX(f->x), y = RSL_PX(f->y);
    int fl = f->dir < 0 ? SPR_FLIPX : 0;
    uint8_t map[PAL_COUNT];
    pal_identity(map);
    bool remap = false;
    if (f->flash > 0 && (f->flash & 2)) {
        for (int c = 0; c < PAL_COUNT; c++) if (c != C_INK) map[c] = C_WHITE;
        remap = true;
    }
    int spr = -1, w = f->w, h = f->h;
    switch (f->kind) {
    case FO_LACKEY:
        spr = (f->t / 10) % 2 ? SPR_LACKEY0 : SPR_LACKEY1;
        if ((f->flags & FF_GREEN) && !remap) { pal_swap(map, C_RED, C_JADE); pal_swap(map, C_MAROON, C_FOREST); remap = true; }
        if (f->state == 0) {
            /* still climbing out of the ground: only the top shows */
            int show = imin(20, f->t);
            gfx_clip(x - 6 - gfx_cam_x(), y + 10 - show - gfx_cam_y(), 14, show);
            spr_draw_ex(&rsl_spr[spr], x - 5, y - 10, fl, remap ? map : NULL, -1);
            gfx_noclip();
            return;
        }
        break;
    case FO_BLOOM: spr = SPR_BLOOM; break;
    case FO_SPITTER: spr = SPR_SPITTER; break;
    case FO_HIVE: spr = SPR_HIVE; break;
    case FO_GNAT:
        spr = SPR_GNAT;
        if (f->var && !remap) { pal_swap(map, C_GREY, C_RED); remap = true; }
        break;
    case FO_WISP: spr = SPR_WISPFOE; break;
    case FO_TUMBLER: spr = SPR_TUMBLER; break;
    case FO_TOAD: spr = SPR_TOAD; break;
    case FO_BOULDER: spr = SPR_BOULDER; break;
    case FO_CRAB: spr = f->state == 2 ? SPR_CRAB_FLIP : SPR_CRAB; break;
    case FO_LEAPER: spr = SPR_LEAPER; fl = f->vx < 0 ? SPR_FLIPX : 0; break;
    case FO_WEEPER: spr = f->state ? SPR_WEEPER_OPEN : SPR_WEEPER_SHUT; break;
    case FO_SHADE: spr = SPR_SHADE; break;
    case FO_CROW: spr = (f->t / 8) % 2 ? SPR_CROW0 : SPR_CROW1; break;
    case FO_FIREMAW: spr = SPR_FIREMAW; break;
    case FO_SHIELD:
        spr = SPR_SHIELDLING;
        break;
    case FO_HULK: spr = (f->t / 16) % 2 ? SPR_HULK0 : SPR_HULK1; break;
    case FO_CLOCKFIRE: spr = SPR_CLOCKFIRE; break;
    case FO_CLOUD: spr = SPR_CLOUD; break;
    case FO_KEEPER:
        spr = SPR_KEEPER;
        if ((f->flags & FF_BLUE) && !remap) { pal_swap(map, C_RED, C_BLUE); remap = true; }
        if (f->state == 1) y += imin(6, f->st / 4); /* the crouch */
        break;
    case FO_RATTLE: spr = SPR_RATTLE; break;
    case FO_HEAD: {
        /* the neck up into the rock */
        for (int k = 1; k < 6; k++) gfx_circ(x - f->dir * k, y - k * 8, 6, C_JADE);
        spr = SPR_HEAD;
        break;
    }
    case FO_GUST: spr = SPR_GUST; break;
    case FO_GASH: spr = SPR_GASH; break;
    case FO_RAMMER: spr = SPR_RAMMER; break;
    case FO_SKULL: spr = SPR_SKULL; break;
    case FO_KING: {
        /* the Hollow King: a great face in the wall */
        int t = rg.frame_t;
        uint8_t skin = f->flash > 0 && (f->flash & 2) ? C_WHITE : C_PURPLE;
        gfx_circ(x, y, 46, C_INK);
        gfx_circ(x, y, 44, skin);
        gfx_circ(x, y + 8, 38, C_VIOLET);
        for (int k = 0; k < 5; k++) gfx_line(x - 40 + k * 20, y - 40, x - 34 + k * 20, y - 58 - (k % 2) * 8, C_YELLOW);
        for (int e = 0; e < 2; e++) {
            int ex = x + (e ? 22 : -22), ey = y - 10;
            gfx_circ(ex, ey, 8, C_INK);
            gfx_circ(ex, ey, 6, C_YELLOW);
            gfx_circ(ex + (rg.pl.x < f->x ? -2 : 2), ey, 3, C_RED);
        }
        if (f->a) {
            gfx_circ(x, y - 30, 7, C_INK);
            gfx_circ(x, y - 30, 5, (t / 4) % 2 ? C_WHITE : C_PINK);
            gfx_circ(x, y - 30, 2, C_RED);
        } else {
            gfx_hline(x - 6, x + 6, y - 30, C_INK);
        }
        gfx_rect(x - 24, y + 18, 48, 14, C_INK);
        for (int k = 0; k < 6; k++) gfx_rect(x - 22 + k * 8, y + 18, 5, 5, C_WHITE);
        if (t % 170 > 40 && t % 170 < 70) gfx_rect(x - 20, y + 24, 40, 8, C_MAROON);
        return;
    }
    default: break;
    }
    if (spr < 0) return;
    const Sprite *sp = &rsl_spr[spr];
    (void)w;
    (void)h;
    int dx = x - sp->w / 2, dy = y - sp->h / 2;
    if (f->kind == FO_LACKEY || f->kind == FO_SHADE || f->kind == FO_HULK || f->kind == FO_GASH || f->kind == FO_KEEPER ||
        f->kind == FO_RAMMER || f->kind == FO_RATTLE || f->kind == FO_SHIELD || f->kind == FO_SPITTER || f->kind == FO_BLOOM ||
        f->kind == FO_TOAD || f->kind == FO_FIREMAW || f->kind == FO_CRAB || f->kind == FO_TUMBLER)
        dy = y + f->h / 2 - sp->h; /* feet on the floor */
    spr_draw_ex(sp, dx, dy, fl, remap ? map : NULL, -1);
    if (f->kind == FO_SHIELD && f->state == 0) spr_draw(&rsl_spr[SPR_SHIELD], x + f->dir * 8 - 3, y - 9, 0);
    if (f->kind == FO_GNAT && (f->t / 2) % 2) gfx_hline(x - 4, x + 3, y - 3, C_LIGHT);
}

static void draw_eshot(const RslEShot *e) {
    int x = RSL_PX(e->x), y = RSL_PX(e->y);
    int t = rg.frame_t;
    switch (e->kind) {
    case ES_BALL: gfx_circ(x, y, 3, C_INK); gfx_circ(x, y, 2, (t / 3) % 2 ? C_PINK : C_MAGENTA); break;
    case ES_SPEAR:
        gfx_hline(x - 7, x + 6, y, C_TAN);
        gfx_rect(e->vx > 0 ? x + 5 : x - 8, y - 1, 3, 3, C_GREY);
        break;
    case ES_STAR:
        gfx_line(x - 3, y - 3, x + 3, y + 3, C_LIGHT);
        gfx_line(x - 3, y + 3, x + 3, y - 3, C_LIGHT);
        gfx_pset(x, y, C_WHITE);
        break;
    case ES_FIRE: gfx_circ(x, y, 4, C_RED); gfx_circ(x, y, 2, C_YELLOW); break;
    case ES_TEAR: gfx_rect(x - 1, y - 3, 3, 6, C_CYAN); gfx_pset(x, y - 3, C_WHITE); break;
    case ES_ROCK: gfx_circ(x, y, 6, C_INK); gfx_circ(x, y, 5, C_BROWN); gfx_pset(x - 2, y - 2, C_TAN); break;
    case ES_BOMB:
        gfx_circ(x, y, 4, C_INK);
        gfx_circ(x - 1, y - 1, 1, C_GREY);
        if ((t / 3) % 2) gfx_pset(x + 2, y - 5, C_YELLOW);
        break;
    case ES_FLAME:
    case ES_BLAZE:
        gfx_circ(x, y, e->kind == ES_BLAZE ? 7 : 5, C_RED);
        gfx_circ(x + ((t / 2) % 3) - 1, y - 1, e->kind == ES_BLAZE ? 4 : 3, C_ORANGE);
        gfx_pset(x, y - 2, C_YELLOW);
        break;
    case ES_BOLT:
        for (int k = 0; k < 4; k++) gfx_line(x + (k % 2 ? 2 : -2), y - 8 + k * 4, x + (k % 2 ? -2 : 2), y - 4 + k * 4, C_YELLOW);
        break;
    case ES_TONGUE: {
        const RslFoe *f = e->owner >= 0 ? &rg.foe[e->owner] : NULL;
        if (f) {
            int fx = RSL_PX(f->x), fy = RSL_PX(f->y) - 2;
            int ex = x + e->vx * e->w / 2;
            gfx_line(fx, fy, ex, fy, C_PINK);
            gfx_circ(ex, fy, 2, C_RED);
        }
        break;
    }
    case ES_TRAP:
        gfx_hline(x - 9, x + 8, y, C_LIGHT);
        gfx_hline(x - 9, x + 8, y + 1, C_GREY);
        gfx_rect(e->vx > 0 ? x + 7 : x - 10, y - 1, 3, 4, C_WHITE);
        break;
    case ES_BLADE:
        gfx_line(x - 6, y - 4, x + 6, y + 4, C_WHITE);
        gfx_line(x - 6, y - 3, x + 6, y + 5, C_LIGHT);
        break;
    case ES_STONE: gfx_circ(x, y, 3, C_GREY); break;
    default: gfx_circ(x, y, 3, C_RED); break;
    }
}

static void draw_items(void) {
    int t = rg.frame_t;
    for (int i = 0; i < RSL_MAX_ITEMS; i++) {
        const RslItem *it = &rg.item[i];
        if (!it->alive) continue;
        if (it->t > 780 && (t / 3) % 2) continue;
        int x = RSL_PX(it->x), y = RSL_PX(it->y);
        int spr = -1;
        switch (it->kind) {
        case IT_POT: spr = SPR_POT; break;
        case IT_URN: spr = SPR_URN; break;
        case IT_JAR: spr = SPR_JAR; break;
        case IT_COIN: spr = SPR_COIN; break;
        case IT_IDOL: spr = SPR_IDOL; break;
        case IT_BEETLE: spr = SPR_BEETLE; break;
        case IT_CLOCK: spr = SPR_CLOCK; break;
        case IT_LILY: spr = SPR_LILY; break;
        case IT_CHARM: spr = SPR_CHARM; break;
        case IT_EGG: spr = SPR_EGG; break;
        case IT_BELL: spr = SPR_BELL; break;
        case IT_ASH: spr = SPR_ASH; break;
        case IT_WEAPON: spr = it->var == WP_SEEKER ? SPR_WP_SEEKER : it->var == WP_EMBER ? SPR_WP_EMBER : SPR_WP_SCATTER; break;
        default: break;
        }
        if (spr < 0) continue;
        const Sprite *s = &rsl_spr[spr];
        if (it->kind == IT_ASH) gfx_dither_circle(x, y, 14, C_ICE, 4);
        if (it->wheel) gfx_circb(x, y, 7, (t / 4) % 2 ? C_YELLOW : C_WHITE);
        spr_draw(s, x - s->w / 2, y - s->h / 2, 0);
    }
}

static void draw_player(void) {
    const RslPlayer *p = &rg.pl;
    if (rg.state == RS_SPIRIT) return;
    int x = RSL_PX(p->x), y = RSL_PX(p->y);
    int fl = p->face < 0 ? SPR_FLIPX : 0;
    if (rg.state == RS_DYING || rg.state == RS_OVER) {
        spr_draw(&rsl_spr[SPR_GAUNT_DEAD], x - 11, y - 8, fl);
        return;
    }
    if (p->inv > 0 && (rg.frame_t / 3) % 2) return;
    int spr = SPR_GAUNT0;
    if (p->duck) spr = SPR_GAUNT_DUCK;
    else if (!p->ground) spr = SPR_GAUNT_JUMP;
    else if (btn(BTN_LEFT) || btn(BTN_RIGHT)) spr = (p->anim / 8) % 2 ? SPR_GAUNT1 : SPR_GAUNT2;
    const Sprite *s = &rsl_spr[spr];
    uint8_t map[PAL_COUNT];
    const uint8_t *m = NULL;
    if (p->charge >= RSL_CHARGE_T && (rg.frame_t / 3) % 2) {
        pal_identity(map);
        pal_swap(map, C_WINE, C_MAGENTA);
        pal_swap(map, C_MAROON, C_PINK);
        m = map;
    }
    int dx = fl ? x - s->w + 5 : x - 5;
    spr_draw_ex(s, dx, y - s->h, fl, m, -1);
    if (p->fire_t > 6) {
        int mx = x + p->face * 10, my = y - (p->duck ? 8 : 17);
        gfx_circ(mx, my, 2, C_WHITE);
    }
}

static void draw_shots(void) {
    int t = rg.frame_t;
    for (int i = 0; i < RSL_MAX_SHOTS; i++) {
        const RslShot *s = &rg.shot[i];
        if (!s->alive) continue;
        int dx = 0, dy = 0;
        if (s->kind == WP_EMBER && s->charged) {
            int o = rsl_sin(s->t * 3 + s->phase) * 7;
            dx = -rsl_sin(s->ang) * o / 256;
            dy = rsl_cos(s->ang) * o / 256;
        }
        int x = RSL_PX(s->x + dx), y = RSL_PX(s->y + dy);
        switch (s->kind) {
        case WP_STAFF:
            gfx_circ(x, y, s->charged ? 4 : 3, C_INK);
            gfx_circ(x, y, s->charged ? 3 : 2, C_ICE);
            gfx_pset(x, y, C_WHITE);
            break;
        case WP_EMBER:
            gfx_circ(x - (s->vx > 0 ? 3 : -3), y, 2, C_RED);
            gfx_circ(x, y, s->charged ? 4 : 3, C_ORANGE);
            gfx_circ(x, y, 1, C_YELLOW);
            break;
        case WP_SEEKER:
            gfx_circb(x, y, s->charged ? 5 : 3, (t / 2) % 2 ? C_VIOLET : C_PURPLE);
            gfx_pset(x, y, C_PINK);
            break;
        case WP_SCATTER:
            gfx_circ(x, y, s->charged ? 3 : 2, s->hover ? ((t / 3) % 2 ? C_WHITE : C_CYAN) : C_CYAN);
            break;
        }
    }
}

static void draw_owlet(void) {
    const RslOwlet *o = &rg.owl;
    int t = rg.frame_t;
    if (o->flag_t > 0) {
        int x = RSL_PX(o->x), y = RSL_PX(o->y);
        spr_draw(&rsl_spr[SPR_OWLET0], x - 5, y - 5, 0);
        spr_draw(&rsl_spr[SPR_FLAG], x + 4, y - 12, 0);
    }
    if (!o->on) return;
    int x = RSL_PX(o->x), y = RSL_PX(o->y);
    spr_draw(&rsl_spr[(t / 6) % 2 ? SPR_OWLET0 : SPR_OWLET1], x - 5, y - 5, 0);
    if (o->target >= 0 && (t / 6) % 2) gfx_circb(x, y, 8, C_YELLOW);
}

static void draw_parts(void) {
    for (int i = 0; i < RSL_MAX_PARTS; i++) {
        const RslPart *q = &rg.part[i];
        if (q->life <= 0) continue;
        gfx_pset(RSL_PX(q->x), RSL_PX(q->y), q->col);
    }
}

/* ------------------------------------------------------------------------ */
/* the HUD                                                                  */

static void draw_hud(void) {
    char buf[48];
    gfx_camera(0, 0);
    snprintf(buf, sizeof buf, "%07u", (unsigned)rg.score);
    text_outline(buf, 6, 4, C_WHITE, C_INK);
    /* a skull for every death */
    int n = imin(rg.deaths, 12);
    for (int i = 0; i < n; i++) spr_draw(&rsl_spr[SPR_SKULL_ICON], 6 + i * 8, 14, 0);
    if (rg.deaths > 12) { snprintf(buf, sizeof buf, "+%d", rg.deaths - 12); tiny_draw(buf, 6 + 12 * 8, 15, C_WHITE); }
    /* the clock */
    int secs = (rg.time + 59) / 60;
    snprintf(buf, sizeof buf, "%d", secs);
    int col = rg.time <= 0 ? ((rg.frame_t / 8) % 2 ? C_RED : C_ORANGE) : secs <= 20 ? C_ORANGE : C_WHITE;
    int w = text_width_scaled(buf, 2);
    gfx_rect(SCREEN_W - w - 10, 3, w + 6, 17, C_INK);
    text_draw_scaled(buf, SCREEN_W - w - 7, 5, col, 2);
    if (rg.deaths >= 6) tiny_draw("x2", SCREEN_W - w - 22, 8, C_RED);
    /* the weapon */
    static const int ICON[WP_COUNT] = {SPR_WP_STAFF, SPR_WP_SEEKER, SPR_WP_EMBER, SPR_WP_SCATTER};
    gfx_rect(SCREEN_W - w - 40, 3, 14, 14, C_INK);
    spr_draw(&rsl_spr[ICON[rg.pl.weapon]], SCREEN_W - w - 38, 5, 0);
    if (rg.owl.on) spr_draw(&rsl_spr[SPR_OWLET0], SCREEN_W - w - 54, 5, 0);
    if (rg.msg_t > 0) {
        int mw = text_width(rg.msg);
        gfx_rect(160 - mw / 2 - 4, 160, mw + 8, 12, C_INK);
        text_draw(rg.msg, 160 - mw / 2, 162, C_YELLOW);
    }
    /* a boss's strength */
    if (rg.boss_on && !rg.boss_dead) {
        int hp = rsl_boss_hp_total(), mx = 0;
        for (int i = 0; i < RSL_MAX_FOES; i++)
            if (rg.foe[i].alive && RSL_IS_BOSS(rg.foe[i].kind) && rg.foe[i].kind != FO_SKULL) mx += rg.foe[i].maxhp;
        static const int HEADS = 3 * 60;
        if (rg.boss_kind == BOSS_TONGUE) mx = HEADS;
        if (rg.boss_kind == BOSS_PAIR) mx = 400;
        if (mx > 0) {
            gfx_rect(80, 172, 160, 5, C_INK);
            gfx_rect(81, 173, 158 * hp / mx, 3, C_RED);
        }
    }
    if (rg.pit >= 0) tiny_center(RSL_HALF[rg.half].treasure_pit == rg.pit ? "A HIDDEN HOARD" : "DOWN IN THE PIT: CLEAR IT", 160, 22, C_LIGHT);
}

/* ------------------------------------------------------------------------ */

static void draw_world(void) {
    int sx = rg.shake > 0 ? (rg.frame_t % 2 ? 2 : -2) : 0;
    gfx_camera(0, 0);
    draw_sky();
    gfx_camera(rg.cam_x + sx, rg.cam_y);
    draw_tiles();
    draw_spawns();
    draw_lifts();
    draw_items();
    for (int i = 0; i < RSL_MAX_FOES; i++)
        if (rg.foe[i].alive) draw_foe(&rg.foe[i]);
    draw_player();
    draw_owlet();
    draw_shots();
    for (int i = 0; i < RSL_MAX_ESHOTS; i++)
        if (rg.es[i].alive) draw_eshot(&rg.es[i]);
    draw_parts();
    gfx_camera(0, 0);
    draw_hud();
}

static void draw_spirit(void) {
    const RslSpirit *s = &rg.sp;
    int t = rg.frame_t;
    gfx_cls(C_INK);
    for (int i = 0; i < 60; i++) {
        int x = (int)(hash2(i, 3) % SCREEN_W), y = (int)((hash2(i, 5) % SCREEN_H + (unsigned)(t / (2 + i % 3))) % SCREEN_H);
        gfx_pset(x, y, i % 3 ? C_DUSK : C_VIOLET);
    }
    gfx_rectb(12, 22, 296, 150, C_PURPLE);
    for (int i = 0; i < s->n; i++) {
        const RslPiece *p = &s->piece[i];
        if (p->taken) continue;
        int x = RSL_PX(p->x), y = RSL_PX(p->y);
        gfx_dither_circle(x, y, 6, C_YELLOW, 3 + (t / 6) % 3);
        spr_draw(&rsl_spr[SPR_PIECE], x - 3, y - 3, 0);
    }
    for (int i = 0; i < s->nguards; i++) {
        const RslGuard *g = &s->guard[i];
        int x = RSL_PX(g->x), y = RSL_PX(g->y);
        if (g->piece < 0) spr_draw(&rsl_spr[SPR_FLAME], x - 4, y - 5, 0);
        else spr_draw(&rsl_spr[SPR_GUARD], x - 5, y - 5, g->lunge > 0 ? 0 : (t / 10) % 2 ? SPR_FLIPX : 0);
    }
    if (!s->failed && (s->inv == 0 || (t / 2) % 2)) {
        int x = RSL_PX(s->x), y = RSL_PX(s->y);
        gfx_dither_circle(x, y, 7, C_ICE, 4);
        spr_draw(&rsl_spr[SPR_WISP], x - 4, y - 4, 0);
    }
    char buf[40];
    snprintf(buf, sizeof buf, "THE LOW GLOW  SOUL %d/%d", s->got, s->n);
    text_outline(buf, 14, 8, C_ICE, C_INK);
    snprintf(buf, sizeof buf, "DEATHS %d", rg.deaths);
    text_outline(buf, SCREEN_W - text_width(buf) - 14, 8, C_LIGHT, C_INK);
    if (s->t < 60) text_center_shadow("GATHER YOUR SOUL", 160, 80, C_WHITE, C_PURPLE);
    if (s->got >= s->n && !s->failed) text_center_shadow("GAUNT RISES AGAIN", 160, 80, C_YELLOW, C_MAROON);
    if (s->failed) text_center_shadow("THE SOUL SCATTERS", 160, 80, C_RED, C_INK);
}

/* ------------------------------------------------------------------------ */
/* screens around a run                                                     */

static void draw_title(void) {
    int t = rg.frame_t;
    gfx_cls(C_INK);
    for (int y = 0; y < 90; y += 2) gfx_hline(0, SCREEN_W - 1, y + 90, y % 4 ? C_MAROON : C_NIGHT);
    /* the burning village */
    for (int k = 0; k < 8; k++) {
        int x = 20 + k * 40, h = 20 + (int)(hash2(k, 1) % 20);
        gfx_rect(x, 150 - h, 24, h, C_NIGHT);
        gfx_line(x - 4, 150 - h, x + 12, 136 - h, C_NIGHT);
        gfx_line(x + 12, 136 - h, x + 28, 150 - h, C_NIGHT);
        int fl = (t / 4 + k) % 3;
        gfx_circ(x + 12, 140 - h - fl, 4 + fl, C_RED);
        gfx_circ(x + 12, 142 - h - fl, 2 + fl / 2, C_YELLOW);
    }
    gfx_rect(0, 150, SCREEN_W, 30, C_INK);
    ui_fancy_center("RESTLESS", 160, 26, 3, TITLE_GRAD, 3, C_INK, C_MAROON);
    tiny_center("OLD GAUNT WILL NOT STAY DEAD", 160, 56, C_LIGHT);
    spr_draw_scaled(&rsl_spr[SPR_GAUNT0], 30, 100, 2, 0);
    spr_draw(&rsl_spr[SPR_WISP], 64 + (int)(rsl_sin(t) * 6 / 256), 96 + (int)(rsl_cos(t) * 4 / 256), 0);
    const char *items[2] = {"START", "HIGH SCORES"};
    for (int i = 0; i < 2; i++) {
        int y = 82 + i * 14;
        text_center(items[i], 160, y, rg.menu == i ? C_YELLOW : C_GREY);
        if (rg.menu == i) ui_cursor(160 - text_width(items[i]) / 2 - 12, y, t);
    }
    char buf[40];
    snprintf(buf, sizeof buf, "BEST %07u", (unsigned)rgs.top[0].score);
    tiny_center(buf, 160, 116, C_AMBER);
    tiny_center(GLYPH_A " START   " GLYPH_B " BACK TO THE LIBRARY", 160, 168, C_GREY);
    tiny_draw("1988 BEAMDOWN", 4, 4, C_DUSK);
}

static const char *const STORY[] = {
    "THE HOLLOW HOST CAME BY NIGHT.",
    "MOSSFOLD BURNED, AND OLD GAUNT,",
    "ITS CHIEF, FELL WITH HIS CROOK",
    "STILL IN HIS HAND.",
    "",
    "BUT GAUNT HAS NEVER ONCE DONE",
    "WHAT HE WAS TOLD.",
    "",
    "GATHER HIS SOUL. RAISE HIM UP.",
    "LAY THE HOLLOW KING TO REST.",
};

static void draw_story(void) {
    gfx_cls(C_INK);
    int n = imin(ARRAY_LEN(STORY), rg.state_t / 30 + 1);
    for (int i = 0; i < n; i++) text_center(STORY[i], 160, 30 + i * 11, i < 4 ? C_LIGHT : C_CREAM);
    if (rg.state_t > 60) tiny_center(GLYPH_A " GO", 160, 168, C_GREY);
}

static void draw_tally(void) {
    draw_world();
    gfx_camera(0, 0);
    ui_panel(60, 40, 200, 96, C_INK, C_AMBER);
    const RslHalf *H = &RSL_HALF[rg.half];
    char buf[48];
    snprintf(buf, sizeof buf, "STAGE %d%c CLEAR", H->stage, H->half ? 'B' : 'A');
    text_center(buf, 160, 50, C_YELLOW);
    text_center(H->name, 160, 64, C_LIGHT);
    snprintf(buf, sizeof buf, "TIME LEFT  %d", rg.tally_secs);
    text_center(buf, 160, 84, C_WHITE);
    snprintf(buf, sizeof buf, "BONUS  %d", rg.tally_bonus);
    text_center(buf, 160, 98, C_AMBER);
    snprintf(buf, sizeof buf, "SECRETS %d/%d", rg.secrets_found, rg.half_secrets);
    text_center(buf, 160, 114, C_GREY);
}

static void draw_over(void) {
    gfx_cls(C_INK);
    text_center_shadow("GAME OVER", 160, 60, C_RED, C_MAROON);
    text_center("THE SOUL SCATTERED IN THE LOW GLOW", 160, 82, C_LIGHT);
    char buf[48];
    snprintf(buf, sizeof buf, "SCORE %u   DEATHS %d", (unsigned)rg.score, rg.deaths);
    text_center(buf, 160, 100, C_WHITE);
    if (rg.state_t > 100) tiny_center(GLYPH_A " ON", 160, 168, C_GREY);
}

static void draw_name(void) {
    gfx_cls(C_INK);
    text_center_shadow("A NAME FOR THE ASHES", 160, 40, C_YELLOW, C_MAROON);
    char buf[48];
    snprintf(buf, sizeof buf, "%07u", (unsigned)rg.score);
    text_center(buf, 160, 60, C_WHITE);
    for (int i = 0; i < 3; i++) {
        char c[2] = {rg.name[i], 0};
        int x = 136 + i * 20;
        text_draw_scaled(c, x, 84, i == rg.name_pos ? C_YELLOW : C_LIGHT, 2);
        if (i == rg.name_pos && (rg.frame_t / 8) % 2) gfx_hline(x - 1, x + 10, 100, C_YELLOW);
    }
    tiny_center(GLYPH_UP GLYPH_DOWN " LETTER   " GLYPH_A " NEXT   " GLYPH_B " BACK", 160, 130, C_GREY);
}

static void draw_scores(void) {
    gfx_cls(C_INK);
    text_center_shadow("THE BRAVEST OF THE DEAD", 160, 20, C_YELLOW, C_MAROON);
    char buf[48];
    for (int i = 0; i < RSL_HIGH_SCORES; i++) {
        const RslScoreRow *r = &rgs.top[i];
        int y = 46 + i * 16;
        if (r->score == 0) snprintf(buf, sizeof buf, "%d.  ---   -------", i + 1);
        else snprintf(buf, sizeof buf, "%d.  %-3s  %07u  %s", i + 1, r->name, (unsigned)r->score, r->won ? "LAID TO REST" : "");
        if (r->score && !r->won) {
            char st[16];
            snprintf(st, sizeof st, "STAGE %d", r->stage);
            snprintf(buf, sizeof buf, "%d.  %-3s  %07u  %s", i + 1, r->name, (unsigned)r->score, st);
        }
        text_draw(buf, 60, y, rg.state == RS_SCORES && rg.name_slot == i && rg.frame_t % 30 < 15 && rg.score == r->score ? C_YELLOW : C_LIGHT);
    }
    snprintf(buf, sizeof buf, "MOST DEATHS %d", rgs.most_deaths);
    tiny_center(buf, 160, 136, C_GREY);
    tiny_center(GLYPH_A " BACK", 160, 168, C_GREY);
}

static void draw_ending(void) {
    int t = rg.state_t;
    gfx_cls(C_NAVY);
    for (int y = 120; y < SCREEN_H; y += 2) gfx_hline(0, SCREEN_W - 1, y, C_FOREST);
    gfx_circ(250, 40, 16, C_CREAM);
    spr_draw_scaled(&rsl_spr[SPR_GAUNT0], 140, 74, 2, 0);
    static const char *const LINES[] = {
        "THE HOLLOW KING IS LAID TO REST.",
        "THE HOST SCATTERS LIKE ASH.",
        "",
        "OLD GAUNT WALKS HOME TO MOSSFOLD",
        "AND STARTS TO BUILD IT AGAIN.",
        "",
        "HE SAYS HE WILL REST WHEN IT'S DONE.",
        "NOBODY BELIEVES HIM.",
        "",
        "RESTLESS",
        "A UFO 40 CARTRIDGE",
        "THANK YOU FOR PLAYING",
    };
    int y0 = SCREEN_H - t / 3;
    for (int i = 0; i < ARRAY_LEN(LINES); i++) {
        int y = y0 + i * 12;
        if (y < -10 || y > SCREEN_H) continue;
        text_center_shadow(LINES[i], 160, y, C_WHITE, C_INK);
    }
}

void rsl_draw(void) {
    gfx_camera(0, 0);
    switch (rg.state) {
    case RS_TITLE: draw_title(); break;
    case RS_STORY: draw_story(); break;
    case RS_PLAY: case RS_DYING: case RS_REVIVE: draw_world(); break;
    case RS_SPIRIT: draw_spirit(); break;
    case RS_TALLY: draw_tally(); break;
    case RS_OVER: draw_over(); break;
    case RS_NAME: draw_name(); break;
    case RS_SCORES: draw_scores(); break;
    case RS_ENDING: draw_ending(); break;
    default: gfx_cls(C_INK); break;
    }
    if (rg.state == RS_REVIVE && (rg.state_t / 3) % 2) gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_WHITE, 6);
    gfx_camera(0, 0);
}

/* the cartridge label */
void rsl_draw_label(int x, int y, int w, int h, int t) {
    gfx_clip(x, y, w, h);
    gfx_rect(x, y, w, h, C_INK);
    for (int k = 0; k < h; k += 2) gfx_hline(x, x + w - 1, y + k, k > h / 2 ? C_MAROON : C_NIGHT);
    for (int k = 0; k < 4; k++) {
        int fx = x + 8 + k * 24;
        int f = (t / 5 + k) % 3;
        gfx_circ(fx, y + h - 8 - f, 3 + f, C_RED);
        gfx_circ(fx, y + h - 7 - f, 1 + f / 2, C_YELLOW);
    }
    spr_draw(&rsl_spr[SPR_GAUNT0], x + 8, y + h - 30, 0);
    spr_draw(&rsl_spr[SPR_WISP], x + 26 + (rsl_sin(t * 2) * 4 / 256), y + h - 38 + (rsl_cos(t * 2) * 3 / 256), 0);
    spr_draw(&rsl_spr[(t / 10) % 2 ? SPR_LACKEY0 : SPR_LACKEY1], x + w - 22, y + h - 28, SPR_FLIPX);
    for (int k = 0; k < 3; k++) spr_draw(&rsl_spr[SPR_SKULL_ICON], x + 40 + k * 9, y + h - 12, 0);
    static const uint8_t grad[2] = {C_YELLOW, C_ORANGE};
    ui_fancy_text("RESTLESS", x + 6, y + 4, 1, grad, 2, C_INK, C_MAROON);
    gfx_noclip();
}

/* tests: the screens' words stay inside the screen and clear of each other */
int rsl_layout_audit(void) {
    int bad = 0;
    int keep = rg.state;
    static const int SCREENS[] = {RS_TITLE, RS_STORY, RS_OVER, RS_NAME, RS_SCORES};
    for (int i = 0; i < ARRAY_LEN(SCREENS); i++) {
        rg.state = SCREENS[i];
        rg.state_t = 2000;
        ui_audit_begin("restless", true);
        ui_audit_area("screen", 0, 0, SCREEN_W, SCREEN_H);
        rsl_draw();
        /* every line of words in these screens */
        if (SCREENS[i] == RS_STORY)
            for (int k = 0; k < ARRAY_LEN(STORY); k++)
                if (STORY[k][0]) ui_audit_text("story", STORY[k], 160 - text_width(STORY[k]) / 2, 30 + k * 11);
        if (SCREENS[i] == RS_TITLE) {
            ui_audit_text("start", "START", 160 - text_width("START") / 2, 82);
            ui_audit_text("scores", "HIGH SCORES", 160 - text_width("HIGH SCORES") / 2, 96);
        }
        bad += ui_audit_end();
        for (int k = 0; k < ARRAY_LEN(STORY); k++) bad += text_missing(STORY[k]) > 0;
    }
    rg.state = keep;
    return bad;
}
