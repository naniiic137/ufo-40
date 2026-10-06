/* FULL PEAL - drawing: the view into the screen (stars streaming past,
 * the corridor's dotted frames, everything in depth from far to near,
 * then the plane), the cockpit strip along the top (stage, scanner,
 * Clary's monitor, ships, credits and grades), and the screens. */
#include "fpl.h"
#include "../chime/chime.h"

static const uint8_t GRAD_GOLD[4] = {C_CREAM, C_YELLOW, C_AMBER, C_ORANGE};
static const uint8_t GRAD_PINK[4] = {C_WHITE, C_PINK, C_MAGENTA, C_VIOLET};

/* ------------------------------------------------------------------ */
/* helpers                                                              */

void fpl_blit(const Sprite *s, float cx, float cy, float scale, int flags, const uint8_t *remap, int solid) {
    if (!s || !s->px || scale <= 0) return;
    int dw = (int)(s->w * scale + 0.5f), dh = (int)(s->h * scale + 0.5f);
    if (dw < 1) dw = 1;
    if (dh < 1) dh = 1;
    int x0 = (int)lroundf(cx) - dw / 2, y0 = (int)lroundf(cy) - dh / 2;
    for (int dy = 0; dy < dh; dy++) {
        int sy = dy * s->h / dh;
        if (flags & SPR_FLIPY) sy = s->h - 1 - sy;
        for (int dx = 0; dx < dw; dx++) {
            int sx = dx * s->w / dw;
            if (flags & SPR_FLIPX) sx = s->w - 1 - sx;
            uint8_t c = s->px[sy * s->w + sx];
            if (c == TRANSPARENT) continue;
            gfx_pset(x0 + dx, y0 + dy, solid >= 0 ? solid : remap ? remap[c] : c);
        }
    }
}

static int hash(int a) {
    unsigned h = (unsigned)a * 2654435761u;
    h ^= h >> 13;
    return (int)(h & 0x7FFFFFFF);
}

static void dotted_rect(int x0, int y0, int x1, int y1, int col, int step) {
    for (int x = x0; x <= x1; x += step) { gfx_pset(x, y0, col); gfx_pset(x, y1, col); }
    for (int y = y0; y <= y1; y += step) { gfx_pset(x0, y, col); gfx_pset(x1, y, col); }
}

/* ------------------------------------------------------------------ */
/* the backdrop                                                         */

static void draw_backdrop(int sky, int sky2, float scroll, int top) {
    gfx_rect(0, top, SCREEN_W, SCREEN_H - top, C_INK);
    /* a few far clouds of the stage's colours, drifting very slowly */
    for (int i = 0; i < 5; i++) {
        int cx = (hash(i * 11 + sky) % 360) - 20 + (int)(sinf(scroll * 0.3f + i) * 6);
        int cy = FPL_VY - 40 + hash(i * 13 + sky2) % 80;
        int r = 14 + hash(i * 17) % 18;
        if (cy - r < top) cy = top + r;
        gfx_dither_circle(cx, cy, r, i % 2 ? sky : sky2, 2);
        gfx_dither_circle(cx + 3, cy + 2, r * 2 / 3, i % 2 ? sky2 : sky, 3);
    }
    /* stars streaming out of the vanishing point */
    for (int i = 0; i < 70; i++) {
        float bx = (float)(hash(i * 3 + 1) % 2000 - 1000) / 125.0f;
        float by = (float)(hash(i * 3 + 2) % 2000 - 1000) / 180.0f;
        float ph = (float)(hash(i * 3 + 3) % 1000) / 1000.0f;
        float z = ph - scroll * 1.6f;
        z -= floorf(z);
        z = z * 1.2f - 0.05f;
        int x = (int)fpl_sx(bx, z), y = (int)fpl_sy(by, z);
        if (y < top) continue;
        int col = z > 0.7f ? C_DUSK : z > 0.35f ? C_SLATE : z > 0.1f ? C_GREY : C_WHITE;
        gfx_pset(x, y, col);
        if (z < 0.12f) gfx_pset(x + 1, y, col);
    }
}

static void draw_corridor(float scroll) {
    /* dotted frames of the plane's edge, coming at you */
    for (int i = 0; i < 6; i++) {
        float z = (float)i / 6.0f - scroll * 2.0f;
        z -= floorf(z);
        if (z < 0.02f) continue;
        int x0 = (int)fpl_sx(-FPL_PX, z), x1 = (int)fpl_sx(FPL_PX, z);
        int y0 = (int)fpl_sy(-FPL_PY, z), y1 = (int)fpl_sy(FPL_PY, z);
        int col = z > 0.6f ? C_NIGHT : z > 0.3f ? C_DUSK : C_SLATE;
        int len = (int)(10 * fpl_k(z)) + 2;
        /* just the corners */
        gfx_hline(x0, x0 + len, y0, col); gfx_vline(x0, y0, y0 + len, col);
        gfx_hline(x1 - len, x1, y0, col); gfx_vline(x1, y0, y0 + len, col);
        gfx_hline(x0, x0 + len, y1, col); gfx_vline(x0, y1 - len, y1, col);
        gfx_hline(x1 - len, x1, y1, col); gfx_vline(x1, y1 - len, y1, col);
    }
}

static void draw_edges(void) {
    /* the orange marks when the ship comes near an edge of its plane */
    if (!fpg.alive) return;
    int x0 = (int)fpl_sx(-FPL_PX, 0), x1 = (int)fpl_sx(FPL_PX, 0) - 1;
    int y0 = (int)fpl_sy(-FPL_PY, 0), y1 = (int)fpl_sy(FPL_PY, 0) - 1;
    int sx = (int)fpl_sx(fpg.x, 0), sy = (int)fpl_sy(fpg.y, 0);
    int c = (fpg.frame_t / 4) % 2 ? C_ORANGE : C_AMBER;
    if (fpg.x > FPL_SHIP_MX - FPL_EDGE_WARN) for (int y = sy - 14; y <= sy + 14; y += 3) gfx_rect(x1 - 1, y, 2, 2, c);
    if (fpg.x < -FPL_SHIP_MX + FPL_EDGE_WARN) for (int y = sy - 14; y <= sy + 14; y += 3) gfx_rect(x0, y, 2, 2, c);
    if (fpg.y > FPL_SHIP_MY - FPL_EDGE_WARN) for (int x = sx - 20; x <= sx + 20; x += 3) gfx_rect(x, y1 - 1, 2, 2, c);
    if (fpg.y < -FPL_SHIP_MY + FPL_EDGE_WARN) for (int x = sx - 20; x <= sx + 20; x += 3) gfx_rect(x, y0, 2, 2, c);
}

static void draw_planet_at(int cx, int cy, int r, int body) {
    /* lit from the upper left, in four bands */
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++) {
            float nx = (float)dx / r, ny = (float)dy / r, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float nz = sqrtf(1.0f - d), l = -0.5f * nx - 0.55f * ny + 0.67f * nz;
            int c = l > 0.75f ? PAL_LIGHTER[body] : l > 0.3f ? body : l > 0.0f ? PAL_DARKER[body] : PAL_DARKER[PAL_DARKER[body]];
            if (l > 0.0f && l <= 0.3f && ((dx + dy) & 1)) c = body;
            gfx_pset(cx + dx, cy + dy, c);
        }
}

static void draw_knell(int cx, int cy, int r);

static void draw_planet(int stage, float grow) {
    static const uint8_t BODY[FPL_STAGES] = {C_BLUE, C_VIOLET, C_JADE, C_WINE, C_MAGENTA};
    int r = (int)(4 + stage * 3 + grow);
    int cx = FPL_VX + 64, cy = FPL_VY - 36;
    if (stage == 4) { draw_knell(cx, cy, r); return; }
    draw_planet_at(cx, cy, r, BODY[stage]);
}

/* Knell: a ringed world, its ring a bell's rim */
static void draw_knell(int cx, int cy, int r) {
    {
        for (int a = 0; a < 64; a++) {
            float t = a * 0.0981748f;
            int x = cx + (int)(cosf(t) * (r + 6)), y = cy + (int)(sinf(t) * (r / 3 + 2));
            if (sinf(t) < 0) gfx_pset(x, y, C_AMBER);
        }
        draw_planet_at(cx, cy, r, C_MAGENTA);
        for (int a = 0; a < 64; a++) {
            float t = a * 0.0981748f;
            int x = cx + (int)(cosf(t) * (r + 6)), y = cy + (int)(sinf(t) * (r / 3 + 2));
            if (sinf(t) >= 0) gfx_pset(x, y, C_YELLOW);
        }
    }
}

/* ------------------------------------------------------------------ */
/* the things in depth                                                  */

typedef struct { float z; int16_t type, idx; } Item;
enum { IT_FOE, IT_SHELL, IT_FWD, IT_PART, IT_BOSS, IT_OPTION, IT_BALLOON };
static Item items[FPL_MAX_FOES + FPL_MAX_ESHOTS + FPL_MAX_PSHOTS + FPL_MAX_PARTS + FPL_BALLOONS + 8];

static int cmp_items(const void *a, const void *b) {
    float za = ((const Item *)a)->z, zb = ((const Item *)b)->z;
    return za < zb ? 1 : za > zb ? -1 : 0;
}

static int foe_sprite(int k) {
    switch (k) {
    case EK_CLAPPER: return SP_CLAPPER;
    case EK_TENOR: return SP_TENOR;
    case EK_TREBLE: return SP_TREBLE;
    case EK_DODGER: return SP_DODGER;
    case EK_BOURDON: return SP_BOURDON;
    case EK_CROSSHEAD: return SP_CROSSHEAD;
    case EK_FORKER: return SP_FORKER;
    case EK_PENDULUM: case EK_SPITE: return SP_PENDULUM;
    case EK_SALLY: case EK_QUICKSALLY: return SP_SALLY;
    case EK_LOOKOUT: return SP_LOOKOUT;
    case EK_MOTE: return SP_MOTE;
    case EK_NIBBLER: return SP_NIBBLER;
    case EK_CALTROP: return SP_CALTROP;
    case EK_BROODER: return SP_BROODER;
    case EK_WISP: return SP_WISP;
    case EK_FLARE: return SP_FLARE;
    default: return SP_CLAPPER;
    }
}

static void draw_foe(const FplFoe *f) {
    if (f->t < 0) {
        if (f->kind == EK_MOTE || f->kind == EK_NIBBLER) {
            /* shimmering into being */
            int x = (int)fpl_sx(f->x, 0), y = (int)fpl_sy(f->y, 0);
            for (int k = 0; k < 6; k++) {
                int a = hash(k + fpg.frame_t / 3) % 13 - 6, b = hash(k * 7 + fpg.frame_t / 3) % 11 - 5;
                gfx_pset(x + a, y + b, (k & 1) ? C_PINK : C_WHITE);
            }
        }
        return;
    }
    float z = f->z < 0 ? f->z : f->z;
    float k = fpl_k(z);
    float sx = fpl_sx(f->x, z), sy = fpl_sy(f->y, z);
    int spr = foe_sprite(f->kind);
    if (f->kind == EK_NIBBLER && (f->t / 8) % 2) spr = SP_NIBBLER2;
    uint8_t map[PAL_COUNT];
    const uint8_t *remap = NULL;
    if (f->kind == EK_SPITE) {
        pal_identity(map);
        map[C_YELLOW] = C_ORANGE;
        map[C_AMBER] = C_RED;
        remap = map;
    } else if (f->kind == EK_QUICKSALLY) {
        pal_identity(map);
        map[C_CYAN] = C_LEAF;
        map[C_SKY] = C_JADE;
        map[C_ICE] = C_LIME;
        remap = map;
    }
    if (f->kind == EK_SALLY || f->kind == EK_QUICKSALLY) {
        /* the rope trailing back into the distance */
        for (int s = 4; s >= 1; s--) {
            float zz = z + s * 0.045f;
            float xx = f->x - sinf((f->t - s * 6) * 0.2f) * 0.08f * s;
            fpl_blit(&fpl_spr[SP_SALLY_SEG], fpl_sx(xx, zz), fpl_sy(f->y, zz), fpl_k(zz), 0, remap, f->flash > 3 ? C_WHITE : -1);
        }
    }
    int flags = (f->kind == EK_LOOKOUT && f->dir < 0) ? SPR_FLIPX : 0;
    if (f->kind == EK_CALTROP) flags = (f->t / 4) % 2 ? SPR_FLIPY : 0;
    fpl_blit(&fpl_spr[spr], sx, sy, k, flags, remap, f->flash > 3 ? C_WHITE : -1);
}

static void draw_shell(const FplEShot *e) {
    float k = fpl_k(e->z);
    int x = (int)fpl_sx(e->x, e->z), y = (int)fpl_sy(e->y, e->z);
    int r = (int)(1 + 3 * k);
    gfx_circ(x, y, r, e->burst ? C_ORANGE : C_PINK);
    gfx_circ(x, y, r > 1 ? r - 1 : 0, (fpg.frame_t / 3) % 2 ? C_YELLOW : C_WHITE);
}

static void draw_option(int k) {
    float x, y;
    fpl_boss_option(k, &x, &y);
    float sc = 1.7f * fpl_k(FPL_BOSS_Z);
    uint8_t map[PAL_COUNT];
    pal_identity(map);
    if (fpg.boss.angry) { map[C_PINK] = C_YELLOW; map[C_MAGENTA] = C_AMBER; map[C_PURPLE] = C_ORANGE; }
    fpl_blit(&fpl_spr[SP_ORB], fpl_sx(x, FPL_BOSS_Z), fpl_sy(y, FPL_BOSS_Z), sc * 0.75f, 0, map, -1);
}

static void draw_boss(void) {
    const FplBoss *b = &fpg.boss;
    static const int SPR[BOSS_COUNT] = {SP_GLOAMEYE, SP_KNUCKLE, SP_INKWELL, SP_SHELLBACK, SP_SORDINA};
    float sc = 1.7f * fpl_k(FPL_BOSS_Z);
    float sx = fpl_sx(b->x, FPL_BOSS_Z), sy = fpl_sy(b->y, FPL_BOSS_Z);
    if (b->dead && b->dead_t > FPL_BOSS_FALL_T - 40) return;
    uint8_t map[PAL_COUNT];
    pal_identity(map);
    if (b->angry && b->kind == BOSS_GLOAMEYE) {
        /* gold at half health */
        map[C_SLATE] = C_AMBER; map[C_GREY] = C_YELLOW; map[C_DUSK] = C_ORANGE; map[C_NIGHT] = C_BROWN;
    } else if (b->angry) {
        map[C_GREY] = C_PINK; map[C_LIGHT] = C_WHITE; map[C_SKY] = C_VIOLET; map[C_LEAF] = C_LIME;
    }
    if (b->kind == BOSS_SORDINA && b->phase == 1) { map[C_MAGENTA] = C_PINK; map[C_VIOLET] = C_MAGENTA; }
    int solid = b->flash >= 5 ? C_WHITE : -1;
    if (b->dead) solid = (b->dead_t / 3) % 2 ? C_WHITE : -1;
    fpl_blit(&fpl_spr[SPR[b->kind]], sx, sy, sc, 0, map, solid);
    float wx, wy, wr;
    fpl_boss_weak(&wx, &wy, &wr);
    int mx = (int)fpl_sx(wx, FPL_BOSS_Z), my = (int)fpl_sy(wy, FPL_BOSS_Z);
    if (b->kind == BOSS_SORDINA && !b->dead) {
        if (b->mouth) {
            /* the muffle drawn back: an open mouth */
            gfx_rect(mx - 9, my - 3, 18, 7, C_MAROON);
            gfx_rect(mx - 7, my - 2, 14, 5, C_INK);
            for (int i = -6; i <= 6; i += 3) gfx_pset(mx + i, my - 2, C_WHITE);
        }
    }
    if (fpl_boss_weak_open() && !b->dead && (fpg.frame_t / 10) % 3 == 0 && !(b->kind == BOSS_SORDINA && b->phase == 2)) {
        /* a glint on the weak point */
        gfx_pset(mx - 1, my - 1, C_WHITE);
        gfx_pset(mx + 1, my + 1, C_WHITE);
    }
}

static void draw_fwd(const FplPShot *s) {
    int x = (int)fpl_sx(s->x, s->z), y = (int)fpl_sy(s->y, s->z);
    float k = fpl_k(s->z);
    int r = (int)(2.5f * k + 0.5f);
    gfx_circ(x, y, r, C_YELLOW);
    gfx_pset(x, y, C_WHITE);
}

static void draw_balloon(const FplBalloon *b) {
    uint8_t map[PAL_COUNT];
    pal_identity(map);
    if (b->orange) { map[C_RED] = C_ORANGE; map[C_WINE] = C_AMBER; }
    fpl_blit(&fpl_spr[SP_BALLOON], fpl_sx(b->x, b->z), fpl_sy(b->y, b->z), fpl_k(b->z), 0, map, -1);
}

static void draw_depth(void) {
    int n = 0;
    for (int i = 0; i < FPL_MAX_FOES; i++) {
        const FplFoe *f = &fpg.foe[i];
        if (f->alive && f->z > FPL_PLANE_Z) items[n++] = (Item){f->z, IT_FOE, (int16_t)i};
    }
    for (int i = 0; i < FPL_MAX_ESHOTS; i++)
        if (fpg.es[i].alive && fpg.es[i].kind == ES_SHELL) items[n++] = (Item){fpg.es[i].z, IT_SHELL, (int16_t)i};
    for (int i = 0; i < FPL_MAX_PSHOTS; i++)
        if (fpg.ps[i].alive && !fpg.ps[i].side) items[n++] = (Item){fpg.ps[i].z, IT_FWD, (int16_t)i};
    for (int i = 0; i < FPL_MAX_PARTS; i++)
        if (fpg.part[i].life > 0 && fpg.part[i].z > 0.01f) items[n++] = (Item){fpg.part[i].z, IT_PART, (int16_t)i};
    for (int i = 0; i < FPL_BALLOONS; i++)
        if (fpg.bal[i].alive && fpg.bal[i].z > FPL_PLANE_Z) items[n++] = (Item){fpg.bal[i].z, IT_BALLOON, (int16_t)i};
    if (fpg.boss.on) {
        items[n++] = (Item){FPL_BOSS_Z, IT_BOSS, 0};
        for (int k = 0; k < fpl_boss_noptions(); k++) {
            float ox, oy;
            fpl_boss_option(k, &ox, &oy);
            (void)ox;
            items[n++] = (Item){FPL_BOSS_Z - 0.002f + (oy > fpg.boss.y ? -0.001f : 0.001f), IT_OPTION, (int16_t)k};
        }
    }
    qsort(items, (size_t)n, sizeof items[0], cmp_items);
    for (int i = 0; i < n; i++) {
        const Item *it = &items[i];
        switch (it->type) {
        case IT_FOE: draw_foe(&fpg.foe[it->idx]); break;
        case IT_SHELL: draw_shell(&fpg.es[it->idx]); break;
        case IT_FWD: draw_fwd(&fpg.ps[it->idx]); break;
        case IT_BALLOON: draw_balloon(&fpg.bal[it->idx]); break;
        case IT_BOSS: draw_boss(); break;
        case IT_OPTION: draw_option(it->idx); break;
        case IT_PART: {
            const FplPart *p = &fpg.part[it->idx];
            gfx_pset((int)fpl_sx(p->x, p->z), (int)fpl_sy(p->y, p->z), p->col);
            break;
        }
        default: break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* the plane                                                            */

static void arrow(int x, int y, int dx, int dy, int col) {
    for (int i = 0; i < 4; i++) {
        if (dx) gfx_vline(x + dx * i, y - (3 - i), y + (3 - i), col);
        else gfx_hline(x - (3 - i), x + (3 - i), y + dy * i, col);
    }
}

static void draw_warnings(void) {
    int x0 = (int)fpl_sx(-FPL_PX, 0), x1 = (int)fpl_sx(FPL_PX, 0);
    int ytop = (int)fpl_sy(-FPL_PY, 0);
    bool blink = (fpg.frame_t / 5) % 2;
    for (int i = 0; i < FPL_MAX_FOES; i++) {
        const FplFoe *f = &fpg.foe[i];
        if (!f->alive) continue;
        if (f->kind == EK_BOURDON && f->state == FS_IN) {
            /* where it will land */
            int cx0 = (int)fpl_sx(f->tx - 0.5f, 0), cy0 = (int)fpl_sy(f->ty - 0.5f, 0);
            dotted_rect(cx0 + 1, cy0 + 1, cx0 + FPL_CELL_W - 2, cy0 + FPL_CELL_H - 2, blink ? C_GREY : C_SLATE, 2);
        }
        if (f->t >= 0 || !blink) continue;
        int y = (int)fpl_sy(f->kind == EK_BOURDON ? f->ty : f->y, 0);
        if (f->kind == EK_FLARE) arrow((int)fpl_sx(f->x, 0), ytop + 2, 0, 1, C_ORANGE);
        else if (f->x < 0) arrow(x0 + 2, y, 1, 0, C_ORANGE);
        else if (f->x > 0) arrow(x1 - 3, y, -1, 0, C_ORANGE);
    }
}

static void draw_ship_at(float x, float y, bool clary) {
    float sx = fpl_sx(x, 0), sy = fpl_sy(y, 0);
    uint8_t map[PAL_COUNT];
    pal_identity(map);
    if (clary) { map[C_YELLOW] = C_PINK; map[C_AMBER] = C_MAGENTA; map[C_ORANGE] = C_VIOLET; map[C_CREAM] = C_WHITE; }
    fpl_blit(&fpl_spr[SP_SHIP], sx, sy, 1.0f, 0, map, -1);
    /* the clapper's glow */
    int gy = (int)sy + 10;
    gfx_pset((int)sx - 1 + (fpg.frame_t / 2) % 2, gy, (fpg.frame_t / 3) % 2 ? C_CYAN : C_WHITE);
}

static void draw_plane(void) {
    draw_warnings();
    /* foes on the plane */
    for (int i = 0; i < FPL_MAX_FOES; i++) {
        const FplFoe *f = &fpg.foe[i];
        if (f->alive && f->z <= FPL_PLANE_Z) draw_foe(f);
    }
    for (int i = 0; i < FPL_BALLOONS; i++)
        if (fpg.bal[i].alive && fpg.bal[i].z <= FPL_PLANE_Z) draw_balloon(&fpg.bal[i]);
    /* Knucklebell's fists */
    const FplBoss *b = &fpg.boss;
    if (b->on && b->kind == BOSS_KNUCKLEBELL && !b->dead) {
        for (int k = 0; k < 2; k++)
            fpl_blit(&fpl_spr[SP_FIST], fpl_sx(b->fx[k], 0), fpl_sy(b->fy, 0), 1.0f, k ? SPR_FLIPX : 0, NULL, -1);
    }
    /* shots on the plane */
    for (int i = 0; i < FPL_MAX_ESHOTS; i++) {
        const FplEShot *e = &fpg.es[i];
        if (!e->alive || e->kind != ES_PLANE) continue;
        int x = (int)fpl_sx(e->x, 0), y = (int)fpl_sy(e->y, 0);
        gfx_circ(x, y, 2, C_RED);
        gfx_pset(x, y, (fpg.frame_t / 2) % 2 ? C_WHITE : C_PINK);
    }
    for (int i = 0; i < FPL_MAX_PSHOTS; i++) {
        const FplPShot *s = &fpg.ps[i];
        if (!s->alive || !s->side) continue;
        int x = (int)fpl_sx(s->x, 0), y = (int)fpl_sy(s->y, 0);
        int dx = s->vx > 0 ? 1 : s->vx < 0 ? -1 : 0, dy = s->vy > 0 ? 1 : s->vy < 0 ? -1 : 0;
        gfx_line(x - dx * 5, y - dy * 4, x, y, C_CYAN);
        gfx_pset(x, y, C_WHITE);
    }
    for (int i = 0; i < FPL_MAX_PARTS; i++) {
        const FplPart *p = &fpg.part[i];
        if (p->life > 0 && p->z <= 0.01f) gfx_pset((int)fpl_sx(p->x, 0), (int)fpl_sy(p->y, 0), p->col);
    }
    if (b->on && b->clary) draw_ship_at(b->clx, b->cly, true);
    if (fpg.alive && !(fpg.inv > 0 && (fpg.inv / 3) % 2)) draw_ship_at(fpg.x, fpg.y, false);
}

/* ------------------------------------------------------------------ */
/* the cockpit strip                                                    */

static void draw_radar(int x, int y) {
    /* the plane seen face on: a dot for each foe, brighter the nearer */
    gfx_rect(x, y, 38, 26, C_INK);
    gfx_rectb(x - 1, y - 1, 40, 28, C_DUSK);
    for (int c = 1; c < 6; c++) gfx_vline(x + 1 + c * 6, y + 1, y + 24, C_NIGHT);
    for (int r = 1; r < 4; r++) gfx_hline(x + 1, x + 36, y + 1 + r * 6, C_NIGHT);
    for (int i = 0; i < FPL_MAX_FOES; i++) {
        const FplFoe *f = &fpg.foe[i];
        if (!f->alive || f->t < 0) continue;
        int px = x + 1 + (int)((f->x + 3) * 6), py = y + 1 + (int)((f->y + 2) * 6);
        if (px < x || px > x + 37 || py < y || py > y + 25) continue;
        int col = f->z <= FPL_PLANE_Z ? C_RED : f->z < 0.35f ? C_ORANGE : f->z < 0.7f ? C_AMBER : C_GREY;
        gfx_pset(px, py, col);
    }
    if (fpg.boss.on && !fpg.boss.dead) gfx_rect(x + 1 + (int)((fpg.boss.x + 3) * 6) - 1, y + 1 + (int)((fpg.boss.y + 2) * 6) - 1, 3, 3, C_MAGENTA);
    if (fpg.alive) gfx_rect(x + (int)((fpg.x + 3) * 6), y + (int)((fpg.y + 2) * 6), 2, 2, C_YELLOW);
}

static void draw_monitor_static(int x, int y) {
    for (int r = 0; r < FPL_MON_H; r += 2)
        for (int c = 0; c < FPL_MON_W; c += 2) {
            int h = hash(r * 97 + c + fpg.frame_t * 31) % 9;
            if (h < 2) gfx_rect(x + c, y + r, 2, 2, h ? C_DUSK : C_SLATE);
        }
}

static void draw_monitor(int x, int y) {
    gfx_rect(x, y, FPL_MON_W, FPL_MON_H, C_INK);
    if (fpl_micro_showing()) {
        gfx_clip(x, y, FPL_MON_W, FPL_MON_H);
        fpl_micro_draw(x, y);
        gfx_noclip();
        return;
    }
    gfx_clip(x, y, FPL_MON_W, FPL_MON_H);
    if (fpg.radio) {
        /* Clary on the radio, her words typing out */
        chm_draw_face(1, x + 2, y + 2, 1);
        int len = (int)strlen(fpg.radio), shown = imin(len, fpg.radio_t / 2);
        char buf[96];
        snprintf(buf, sizeof buf, "%.*s", shown, fpg.radio);
        if (fpg.stage == 4 && fpg.state == PS_RADIO) {
            /* the last message comes through scrambled */
            for (char *p = buf; *p; p++)
                if (hash(fpg.frame_t / 4 + (int)(p - buf)) % 4 == 0 && *p != ' ') *p = "#%?!*"[hash((int)(p - buf)) % 5];
        }
        char lines[5][UI_WRAP_LEN];
        int nl = ui_wrap(buf, FPL_MON_W - 22, true, lines, 5);
        for (int i = 0; i < imin(nl, 5); i++) tiny_draw(lines[i], x + 20, y + 2 + i * 7, C_LIME);
        if (fpg.radio_t > len * 2 + 200 && fpg.state != PS_RADIO && fpg.state != PS_GRADE && fpg.state != PS_OVER &&
            fpg.state != PS_BONUS_END && fpg.state != PS_RESTART) fpg.radio = NULL;
    } else {
        draw_monitor_static(x, y);
        chm_draw_face(0, x + 2, y + 2, 1);
        char b[32];
        if (fpg.state == PS_BOSS) snprintf(b, sizeof b, "%s", FPL_BOSS_NAME[fpg.boss.kind]);
        else if (fpg.state == PS_BONUS) snprintf(b, sizeof b, "BALLOONS %d", fpg.bonus_pts);
        else snprintf(b, sizeof b, "WAVE %d  %d%%", fpg.wave + 1, fpl_wave_grade());
        tiny_draw(b, x + 20, y + 4, C_LIME);
        tiny_draw("ANSEL / TINKLER", x + 20, y + 13, C_JADE);
    }
    gfx_noclip();
    /* player 2 holding A: a bar fills */
    if (fpm.hold > 10 && fpm.state == MC_OFF) gfx_rect(x, y + FPL_MON_H - 2, fpm.hold * FPL_MON_W / FPL_HOLD_OPEN, 2, C_YELLOW);
}

static void draw_hud(void) {
    gfx_rect(0, 0, SCREEN_W, FPL_HUD_H, C_INK);
    gfx_hline(0, SCREEN_W - 1, FPL_HUD_H - 1, C_DUSK);
    const FplStage *st = &FPL_STAGE[fpg.stage];
    /* the stage */
    ui_panel(2, 2, 70, 41, C_NIGHT, C_DUSK);
    char b[48];
    snprintf(b, sizeof b, "STAGE %c", st->letter);
    text_draw(b, 6, 5, C_YELLOW);
    tiny_draw(st->name, 6, 15, C_LIGHT);
    const char *what = "";
    if (fpg.state == PS_WAVE || fpg.state == PS_GRADE) { snprintf(b, sizeof b, "WAVE %d OF 4", fpg.wave + 1); what = b; }
    else if (fpg.state == PS_BOSS || fpg.state == PS_BOSS_DOWN) what = "BOSS";
    else if (fpg.state >= PS_BONUS_IN && fpg.state <= PS_BONUS_END) what = "BALLOON ROUND";
    else if (fpg.state == PS_RADIO) what = "INCOMING";
    tiny_draw(what, 6, 23, C_GREY);
    /* how far into the wave (formations), or the boss's health */
    gfx_rect(6, 32, 62, 5, C_INK);
    if (fpg.boss.on && fpg.boss.maxhp > 0) {
        int w = 60 * imax(0, fpg.boss.hp) / fpg.boss.maxhp;
        gfx_rect(7, 33, w, 3, fpg.boss.phase == 1 ? C_PINK : C_RED);
    } else if (fpg.state == PS_WAVE) {
        int n = st->wave[fpg.wave].n;
        gfx_rect(7, 33, 60 * imin(fpg.form_i, n) / imax(1, n), 3, C_JADE);
    }
    /* the scanner */
    tiny_draw("SCAN", 85, 4, C_SLATE);
    draw_radar(76, 12);
    /* Clary's monitor */
    ui_panel(FPL_MON_X - 3, FPL_MON_Y - 3, FPL_MON_W + 6, FPL_MON_H + 6, C_DUSK, C_SLATE);
    draw_monitor(FPL_MON_X, FPL_MON_Y);
    /* ships, credits, grades */
    ui_panel(208, 2, 110, 41, C_NIGHT, C_DUSK);
    tiny_draw("SHIPS", 212, 5, C_GREY);
    for (int i = 0; i < imin(fpg.lives, 6); i++) {
        int x = 238 + i * 9;
        gfx_rect(x + 2, 4, 3, 2, C_YELLOW);
        gfx_rect(x + 1, 6, 5, 2, C_YELLOW);
        gfx_rect(x, 8, 7, 1, C_AMBER);
    }
    snprintf(b, sizeof b, "CREDITS %d", fpg.continues);
    tiny_draw(b, 212, 13, C_GREY);
    if (fpg.hugs) tiny_draw("HUGS", 290, 13, C_PINK);
    for (int w = 0; w < FPL_WAVES; w++) {
        int g = fpg.grade[fpg.stage][w];
        int x = 212 + w * 26, y = 22;
        gfx_rect(x, y, 24, 9, w == fpg.wave && (fpg.state == PS_WAVE) ? C_DUSK : C_INK);
        if (g < 0) snprintf(b, sizeof b, "--");
        else snprintf(b, sizeof b, "%d", g);
        tiny_center(b, x + 12, y + 2, g == 100 ? C_YELLOW : g == 0 ? C_PINK : g < 0 ? C_DUSK : C_WHITE);
    }
    int total = 0;
    for (int s = 0; s < FPL_STAGES; s++)
        for (int w = 0; w < FPL_WAVES; w++) total += imax(0, fpg.grade[s][w]);
    snprintf(b, sizeof b, "SCORE %d", total);
    tiny_draw(b, 212, 34, C_CREAM);
}

/* ------------------------------------------------------------------ */
/* the screens                                                          */

static void banner(const char *big, const char *small, int y, const uint8_t *grad) {
    ui_fancy_center(big, 160, y, 2, grad, 4, C_INK, C_NIGHT);
    if (small) text_center_shadow(small, 160, y + 20, C_WHITE, C_INK);
}

static void draw_records(int y) {
    char b[96];
    snprintf(b, sizeof b, "TOP  A %d  B %d  C %d  D %d  E %d", fpsv.top[0], fpsv.top[1], fpsv.top[2], fpsv.top[3], fpsv.top[4]);
    tiny_center(b, 160, y, C_LIGHT);
    snprintf(b, sizeof b, "BEST %d   WINS %d   FURTHEST %s   RUNS %d", fpsv.best, fpsv.wins,
             fpsv.furthest >= 6 ? "KNELL" : fpsv.furthest == 0 ? "-" : (const char[]){(char)('A' + fpsv.furthest - 1), 0},
             fpsv.runs);
    tiny_center(b, 160, y + 8, C_GREY);
}

static void draw_title(void) {
    draw_backdrop(C_NAVY, C_PURPLE, fpg.scroll, 0);
    draw_corridor(fpg.scroll);
    draw_knell(252, 96, 26);
    ui_fancy_center("FULL PEAL", 160, 14, 3, GRAD_GOLD, 4, C_INK, C_BROWN);
    tiny_center("ANSEL AND THE TINKLER, OUT TO KNELL", 160, 40, C_LIGHT);
    fpl_blit(&fpl_spr[SP_SHIP], 96 + sinf(fpg.frame_t * 0.03f) * 16, 98 + sinf(fpg.frame_t * 0.05f) * 5, 1.0f, 0, NULL, -1);
    static const char *const ITEMS[2] = {"START", "CODE"};
    for (int i = 0; i < 2; i++) {
        int y = 132 + i * 11;
        text_center(ITEMS[i], 160, y, fpg.sel == i ? C_YELLOW : C_GREY);
        if (fpg.sel == i) ui_cursor(140, y, fpg.frame_t);
    }
    if (fpg.hugs) tiny_center("HUGS-ONLY IS ON", 160, 124, C_PINK);
    draw_records(158);
}

static void draw_code(void) {
    draw_backdrop(C_NAVY, C_PURPLE, fpg.scroll, 0);
    ui_fancy_center("CODE", 160, 20, 2, GRAD_GOLD, 4, C_INK, C_BROWN);
    for (int i = 0; i < 8; i++) {
        int x = 160 - 8 * 11 / 2 - 5 + i * 11 + (i >= 4 ? 10 : 0);
        char ch[2] = {fpg.code[i], 0};
        bool sel = i == fpg.code_pos;
        ui_panel(x, 70, 10, 13, sel ? C_DUSK : C_NIGHT, sel ? C_YELLOW : C_SLATE);
        text_center(ch, x + 5, 73, sel ? C_YELLOW : C_WHITE);
    }
    text_center("-", 160, 73, C_GREY);
    tiny_center(GLYPH_UP GLYPH_DOWN " LETTER   " GLYPH_LEFT GLYPH_RIGHT " MOVE   " GLYPH_A " ENTER   " GLYPH_B " BACK", 160, 96, C_GREY);
    if (fpg.code_msg_t > 0 && fpg.code_msg) tiny_center(fpg.code_msg, 160, 112, C_LIME);
}

static void draw_owl(int x, int y) {
    ui_panel(x - 2, y - 2, 52, 48, C_NIGHT, C_YELLOW);
    fpl_blit(&fpl_spr[SP_OWL], x + 24, y + 20, 1.6f, 0, NULL, -1);
}

static void draw_overlay(void) {
    const FplStage *st = &FPL_STAGE[fpg.stage];
    char b[64];
    switch (fpg.state) {
    case PS_RADIO:
        if (fpg.state_t < 200) {
            snprintf(b, sizeof b, "STAGE %c", st->letter);
            banner(b, st->name, 70, GRAD_GOLD);
        }
        break;
    case PS_GRADE:
        snprintf(b, sizeof b, "%d%%", fpg.last_grade);
        banner(b, fpg.meta ? "SOMEDAY THE RINGING STOPS" : fpg.last_grade == 100 ? "EVERY ONE!" : "OF THE WAVE SHOT DOWN", 64,
               fpg.last_grade == 100 ? GRAD_GOLD : GRAD_PINK);
        if (fpg.owl) draw_owl(136, 104);
        break;
    case PS_BOSS:
        if (fpg.state_t < 120) banner(FPL_BOSS_NAME[fpg.boss.kind], NULL, 60, GRAD_PINK);
        break;
    case PS_BONUS_IN:
        banner("BALLOONS!", "RED 1  ORANGE 3  50 WINS A CONTINUE", 64, GRAD_GOLD);
        snprintf(b, sizeof b, "%d ORANGE THIS TIME (%d PERFECT WAVES)", imin(FPL_BALLOONS, FPL_BALLOON_ORANGE0 + FPL_BALLOON_ORANGE_PER * fpg.perfect_stage), fpg.perfect_stage);
        tiny_center(b, 160, 110, C_LIGHT);
        break;
    case PS_BONUS: {
        snprintf(b, sizeof b, "%d", fpg.bonus_pts);
        ui_fancy_center(b, 160, 50, 2, fpg.bonus_pts >= FPL_BONUS_NEED ? GRAD_GOLD : GRAD_PINK, 4, C_INK, C_NIGHT);
        int left = imax(0, FPL_BONUS_T - fpg.bonus_t) / 60;
        snprintf(b, sizeof b, "TIME %d", left);
        tiny_center(b, 160, 70, C_LIGHT);
        break;
    }
    case PS_BONUS_END:
        snprintf(b, sizeof b, "%d POINTS", fpg.bonus_pts);
        banner(b, fpg.bonus_pts >= FPL_BONUS_NEED ? "ONE MORE CONTINUE!" : "NO CONTINUE THIS TIME", 70,
               fpg.bonus_pts >= FPL_BONUS_NEED ? GRAD_GOLD : GRAD_PINK);
        break;
    case PS_RESTART:
        banner("CREDIT USED", NULL, 64, GRAD_PINK);
        snprintf(b, sizeof b, "STAGE %c AGAIN FROM WAVE 1   %d LEFT", st->letter, fpg.continues);
        tiny_center(b, 160, 92, C_LIGHT);
        break;
    case PS_OVER:
        banner("GAME OVER", NULL, 70, GRAD_PINK);
        if (fpg.state_t > 60) tiny_center(GLYPH_A " TITLE", 160, 100, C_GREY);
        break;
    default: break;
    }
}

static void draw_ending(void) {
    int t = fpg.state_t;
    draw_backdrop(C_NIGHT, C_PURPLE, fpg.scroll, 0);
    if (t < 420) {
        draw_planet(4, 30 + t * 0.15f);
        float s = 1.0f - t / 520.0f;
        fpl_blit(&fpl_spr[SP_SHIP], 150, 120 - t * 0.12f, s, 0, NULL, -1);
        uint8_t map[PAL_COUNT];
        pal_identity(map);
        map[C_YELLOW] = C_PINK; map[C_AMBER] = C_MAGENTA; map[C_ORANGE] = C_VIOLET;
        fpl_blit(&fpl_spr[SP_SHIP], 176, 128 - t * 0.12f, s, 0, map, -1);
        if (t > 60) text_center_shadow("THE BELLS OF KNELL RING OUT AGAIN.", 160, 20, C_WHITE, C_INK);
        if (t > 180) text_center_shadow("TIME TO GO DOWN AND HAVE A LOOK.", 160, 32, C_LIGHT, C_INK);
    } else if (t < 440) {
        gfx_cls(C_WHITE);
    } else if (t < 600) {
        gfx_cls(C_INK);
        for (int i = 0; i < 30; i++) {
            int x = 140 + hash(i) % 40, y = 120 - ((t - 440) + hash(i * 3) % 60) / 2 % 60;
            gfx_dither_circle(x, y, 3 + hash(i * 5) % 4, C_SLATE, 6);
        }
        gfx_rect(120, 128, 80, 6, C_DUSK);
    } else {
        gfx_cls(C_INK);
        tiny_center("THE END", 160, 86, C_GREY);
    }
}

static void draw_tally(void) {
    draw_backdrop(C_NIGHT, C_NAVY, fpg.scroll, 0);
    ui_fancy_center("THE FINAL COUNT", 160, 8, 2, GRAD_GOLD, 4, C_INK, C_BROWN);
    char b[64];
    int total = 0;
    for (int s = 0; s < FPL_STAGES; s++) {
        int y = 36 + s * 12, sum = 0;
        snprintf(b, sizeof b, "%c", FPL_STAGE[s].letter);
        text_draw(b, 70, y, C_YELLOW);
        for (int w = 0; w < FPL_WAVES; w++) {
            int g = imax(0, fpg.grade[s][w]);
            sum += g;
            snprintf(b, sizeof b, "%d", g);
            text_draw(b, 90 + w * 30, y, g == 100 ? C_YELLOW : C_WHITE);
        }
        snprintf(b, sizeof b, "%d", sum);
        text_draw(b, 220, y, C_CREAM);
        total += sum;
    }
    snprintf(b, sizeof b, "CONTINUES LEFT %d X 100 = %d", fpg.continues, fpg.continues * 100);
    text_center(b, 160, 102, C_LIGHT);
    snprintf(b, sizeof b, "%d", fpg.final_score);
    ui_fancy_center(b, 160, 116, 2, GRAD_GOLD, 4, C_INK, C_BROWN);
    (void)total;
    if (fpg.state_t > 90)
        text_center(fpg.final_score >= 1500 ? "1,500 OR MORE: THE THIRD GOAL!" : "1,500 OR MORE EARNS THE THIRD GOAL", 160, 144,
                    fpg.final_score >= 1500 ? C_LIME : C_GREY);
    if (fpg.state_t > 200) tiny_center(GLYPH_A " TITLE", 160, 166, C_GREY);
}

void fpl_draw(void) {
    switch (fpg.state) {
    case PS_TITLE: draw_title(); return;
    case PS_CODE: draw_code(); return;
    case PS_ENDING: draw_ending(); return;
    case PS_TALLY: draw_tally(); return;
    default: break;
    }
    const FplStage *st = &FPL_STAGE[fpg.stage];
    int shx = fpg.shake > 0 ? (fpg.shake % 2 ? 1 : -1) : 0;
    gfx_camera(shx, 0);
    gfx_clip(0, FPL_HUD_H, SCREEN_W, SCREEN_H - FPL_HUD_H);
    draw_backdrop(st->sky, st->sky2, fpg.scroll, FPL_HUD_H);
    draw_planet(fpg.stage, fpg.stage_t / 900.0f);
    draw_corridor(fpg.scroll);
    draw_depth();
    draw_plane();
    draw_edges();
    gfx_noclip();
    gfx_camera(0, 0);
    draw_overlay();
    draw_hud();
}

/* the cartridge label: the Tinkler flying down the corridor */
void fpl_draw_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_INK);
    gfx_clip(x, y, w, h);
    int cx = x + w / 2, cy = y + h / 2;
    for (int i = 0; i < 4; i++) {
        float z = (float)i / 4.0f - t * 0.01f;
        z -= floorf(z);
        int hw = (int)(w * 0.5f / (1 + 4 * z)), hh = (int)(h * 0.45f / (1 + 4 * z));
        dotted_rect(cx - hw, cy - hh, cx + hw, cy + hh, z > 0.5f ? C_DUSK : C_SLATE, 3);
    }
    for (int i = 0; i < 12; i++) gfx_pset(x + hash(i) % w, y + hash(i * 7) % h, C_GREY);
    gfx_circ(cx, cy - 2, 3, C_VIOLET);
    fpl_blit(&fpl_spr[SP_SHIP], cx + sinf(t * 0.05f) * 3, cy + h / 4, 0.75f, 0, NULL, -1);
    gfx_noclip();
}
