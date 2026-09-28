/* HOMESPUN - drawing: the tiles of each part of Oddmoor, the rooms and
 * what stands in them, camp and its buildings as they grow, the HUD, the
 * menus and the cartridge label. The sprites are in homespun_sprites.c. */
#include "homespun.h"

Sprite hs_spr[HS_SPR_COUNT];
int hs_art_bad;

typedef struct HsSprDef { int id, w, h; const char *px; } HsSprDef;
const void *hs_sprite_table(int *n);

void hs_art_load(void) {
    if (hs_spr[HS_WICK_D0].px) return;
    int n = 0;
    const HsSprDef *d = (const HsSprDef *)hs_sprite_table(&n);
    hs_art_bad = 0;
    for (int i = 0; i < n; i++) {
        if ((int)strlen(d[i].px) != d[i].w * d[i].h) { hs_art_bad++; continue; }
        spr_make(&hs_spr[d[i].id], d[i].w, d[i].h, d[i].px);
    }
    /* every foe has its sprite */
    for (int k = 0; k < E_KINDS; k++)
        if (!hs_spr[HS_FOE0 + k].px) hs_art_bad++;
}

/* ---- tiles ------------------------------------------------------------------------ */

typedef struct Look { uint8_t f1, f2, w1, w2, w3, q1, q2; } Look;
static const Look LOOK[B_COUNT] = {
    [B_MEADOW] = {C_LEAF, C_LIME, C_FOREST, C_LEAF, C_EARTH, C_BLUE, C_SKY},
    [B_MARSH] = {C_JADE, C_TEAL, C_FOREST, C_JADE, C_NIGHT, C_NAVY, C_BLUE},
    [B_WOODS] = {C_FOREST, C_JADE, C_NIGHT, C_FOREST, C_BROWN, C_NAVY, C_BLUE},
    [B_DUNES] = {C_TAN, C_AMBER, C_EARTH, C_BROWN, C_MAROON, C_BLUE, C_SKY},
    [B_CRAGS] = {C_GREY, C_SLATE, C_DUSK, C_SLATE, C_NIGHT, C_NAVY, C_BLUE},
    [B_ASH] = {C_DUSK, C_MAROON, C_NIGHT, C_DUSK, C_INK, C_RED, C_ORANGE},
    [B_CAVE] = {C_EARTH, C_BROWN, C_NIGHT, C_DUSK, C_INK, C_NAVY, C_BLUE},
    [B_DUN] = {C_SLATE, C_DUSK, C_NIGHT, C_PURPLE, C_INK, C_NAVY, C_BLUE},
    [B_CAMP] = {C_TAN, C_HIDE, C_BROWN, C_EARTH, C_INK, C_BLUE, C_SKY},
};

static uint32_t th(int a, int b) {
    uint32_t h = (uint32_t)a * 73856093u ^ (uint32_t)b * 19349663u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}

static void floor_tile(const Look *L, int x, int y, int tx, int ty, int room) {
    gfx_rect(x, y, HS_T, HS_T, L->f1);
    uint32_t h = th(tx + room * 31, ty);
    for (int k = 0; k < 3; k++) {
        int px = (int)((h >> (k * 8)) & 15), py = (int)((h >> (k * 8 + 4)) & 15);
        gfx_pset(x + px, y + py, L->f2);
        if (k == 0) gfx_pset(x + px + 1, y + py, L->f2);
    }
}

void hs_draw_tile(int t, int biome, int x, int y, int tx, int ty, int room) {
    const Look *L = &LOOK[biome < B_COUNT ? biome : 0];
    int f = engine_frame();
    switch (t) {
    case T_WALL:
        if (biome == B_MEADOW || biome == B_WOODS || biome == B_MARSH) {
            gfx_rect(x, y, HS_T, HS_T, L->f1);
            gfx_circ(x + 8, y + 7, 7, L->w1);
            gfx_circ(x + 6, y + 5, 3, L->w2);
            gfx_rect(x + 7, y + 12, 2, 4, L->w3);
        } else if (biome == B_DUN) {
            gfx_rect(x, y, HS_T, HS_T, L->w1);
            int o = (ty % 2) * 4;
            for (int k = 0; k < 2; k++) gfx_hline(x, x + 15, y + k * 8 + 7, L->w3);
            gfx_vline(x + o + 3, y, y + 7, L->w3);
            gfx_vline(x + (o + 11) % 16, y + 8, y + 15, L->w3);
            gfx_pset(x + 2, y + 2, L->w2);
        } else {
            gfx_rect(x, y, HS_T, HS_T, L->w1);
            gfx_rect(x + 1, y + 1, 14, 9, L->w2);
            gfx_hline(x + 2, x + 12, y + 2, L->f2);
            gfx_rect(x + 3, y + 11, 10, 4, L->w3);
        }
        break;
    case T_WATER:
        gfx_rect(x, y, HS_T, HS_T, L->q1);
        for (int k = 0; k < 2; k++) {
            int wx = (int)((th(tx, ty + k) + (uint32_t)f / 20) % 12);
            gfx_hline(x + wx, x + wx + 3, y + 4 + k * 7, L->q2);
        }
        break;
    case T_LAVA:
        gfx_rect(x, y, HS_T, HS_T, (f / 16 + tx) % 2 ? C_RED : C_ORANGE);
        gfx_rect(x + 3 + (f / 8 + ty) % 8, y + 5, 3, 2, C_YELLOW);
        break;
    case T_BLOCK:
        floor_tile(L, x, y, tx, ty, room);
        gfx_line(x + 1, y + 2, x + 14, y + 13, C_BROWN);
        gfx_line(x + 1, y + 13, x + 14, y + 2, C_BROWN);
        gfx_line(x + 2, y + 2, x + 15, y + 13, C_EARTH);
        gfx_line(x + 2, y + 13, x + 15, y + 2, C_EARTH);
        gfx_pset(x + 5, y + 5, C_WHITE);
        gfx_pset(x + 10, y + 10, C_WHITE);
        break;
    case T_CAVE:
        gfx_rect(x, y, HS_T, HS_T, L->w1);
        gfx_circ(x + 8, y + 12, 7, C_INK);
        gfx_rect(x + 1, y + 12, 14, 4, C_INK);
        break;
    case T_CRACK:
        hs_draw_tile(T_WALL, biome, x, y, tx, ty, room);
        gfx_line(x + 3, y + 1, x + 8, y + 8, C_INK);
        gfx_line(x + 8, y + 8, x + 6, y + 14, C_INK);
        gfx_line(x + 8, y + 8, x + 13, y + 11, C_INK);
        break;
    case T_SHUT:
        floor_tile(L, x, y, tx, ty, room);
        for (int k = 0; k < 4; k++) gfx_rect(x + 1 + k * 4, y, 2, HS_T, C_GREY);
        gfx_hline(x, x + 15, y + 3, C_SLATE);
        gfx_hline(x, x + 15, y + 12, C_SLATE);
        break;
    case T_BRIDGE:
        gfx_rect(x, y, HS_T, HS_T, L->q1);
        for (int k = 0; k < 4; k++) gfx_rect(x, y + k * 4, HS_T, 3, C_TAN);
        gfx_hline(x, x + 15, y, C_BROWN);
        break;
    case T_PATH: case T_DOOR:
        floor_tile(L, x, y, tx, ty, room);
        if (biome != B_DUN && biome != B_CAVE) gfx_dither(x, y, HS_T, HS_T, L->f2, 6);
        break;
    default:
        floor_tile(L, x, y, tx, ty, room);
        break;
    }
}

/* ---- things, foes, Wick ------------------------------------------------------------------ */

void hs_draw_icon(int kind, int x, int y) {
    int id = kind < IT_KINDS ? HS_ICON0 + kind : kind;
    spr_draw(&hs_spr[id], x, y, 0);
}

static void draw_thing(const HsThing *t) {
    int x = t->tx * HS_T, y = HS_OY + t->ty * HS_T;
    int f = engine_frame();
    switch (t->kind) {
    case TH_POT:
        gfx_circ(x + 8, y + 10, 5, C_BROWN);
        gfx_rect(x + 5, y + 3, 6, 3, C_EARTH);
        gfx_pset(x + 6, y + 8, C_TAN);
        break;
    case TH_CHEST: case TH_CACHE:
        gfx_rect(x + 2, y + 5, 12, 9, t->kind == TH_CACHE ? C_EARTH : C_BROWN);
        gfx_rect(x + 2, y + 5, 12, 3, t->open ? C_INK : C_TAN);
        gfx_rectb(x + 2, y + 5, 12, 9, C_INK);
        if (!t->open) gfx_rect(x + 7, y + 8, 2, 2, C_YELLOW);
        break;
    case TH_PAD: {
        bool awake = (sv.pads >> t->arg & 1) != 0;
        gfx_circ(x + 8, y + 9, 7, C_SLATE);
        gfx_circ(x + 8, y + 9, 5, awake ? ((f / 10) % 2 ? C_CYAN : C_SKY) : C_DUSK);
        char b[4];
        snprintf(b, sizeof b, "%d", t->arg);
        tiny_center(b, x + 8, y + 7, C_WHITE);
        break;
    }
    case TH_NPC:
        spr_draw(&hs_spr[HS_NPC0 + t->arg], x, y - ((f / 30) % 2), 0);
        break;
    case TH_VENDOR:
        spr_draw(&hs_spr[HS_NOODLER], x, y - ((f / 12) % 2), (f / 40) % 2 ? SPR_FLIPX : 0);
        break;
    case TH_TABLE:
        gfx_rect(x - 8, y + 4, 32, 8, C_MAROON);
        gfx_hline(x - 8, x + 23, y + 4, C_PINK);
        break;
    case TH_GEAR:
        gfx_circ(x + 8, y + 12, 6, C_SLATE);
        spr_draw(&hs_spr[HS_ICON0 + IT_GEAR], x + 4, y + 2 - (f / 20) % 2, 0);
        break;
    case TH_PART:
        if (hs.boss >= 0) break;
        gfx_rect(x + 2, y + 11, 12, 4, C_SLATE);
        spr_draw(&hs_spr[HS_ICON0 + IT_PART], x + 4, y + 2 - (f / 20) % 2, 0);
        break;
    default: break;
    }
}

static void draw_mob(const HsMob *m) {
    const HsFoeDef *d = &HS_FOE[m->kind];
    int big = d->tier == 3 ? 24 : 16;
    int x = m->x / HS_U - big / 2, y = HS_OY + m->y / HS_U - big / 2;
    int f = engine_frame();
    if ((m->kind == E_GRUBLET || m->kind == E_SPLOSH) && !m->up) {
        /* only a ripple or a mound shows */
        int c = m->kind == E_SPLOSH ? C_SKY : C_EARTH;
        gfx_hline(x + 4, x + 11, y + 13, c);
        gfx_hline(x + 6, x + 9, y + 12, c);
        return;
    }
    if (m->kind == E_MAWBO && m->st == 9) return;
    int bob = (f / 10 + m->spawn) % 2;
    int flags = (hs.px < m->x) ? SPR_FLIPX : 0;
    if (m->kind == E_SPITBLOOM || m->kind == E_PUFFCAP || m->kind == E_THORNMOTHER) flags = 0, bob = 0;
    bool tell = hs_mob_tell(m);
    if (tell) x += (f / 2) % 2 ? 1 : -1; /* it shakes before a leap or a dive */
    if ((m->flash > 0 && (m->flash / 2) % 2) || (tell && (f / 4) % 2)) {
        spr_draw_ex(&hs_spr[HS_FOE0 + m->kind], x, y - bob, flags, NULL, tell ? C_YELLOW : C_WHITE);
    } else {
        spr_draw(&hs_spr[HS_FOE0 + m->kind], x, y - bob, flags);
    }
    if (d->tier == 3 && m->kind == E_MAWBO) {
        /* six notches for six fights */
        for (int k = 0; k < 6; k++) gfx_rect(x + 1 + k * 4, y - 5, 3, 2, k < sv.mawbo_wins ? C_YELLOW : C_DUSK);
    }
}

static void draw_wick(void) {
    int x = hs.px / HS_U - 8, y = HS_OY + hs.py / HS_U - 11;
    if (hs.inv > 0 && (hs.inv / 3) % 2 && sv.on_trip) return;
    bool walking = hs.walk_t > 0;
    int fr = walking ? (hs.walk_t / 8) % 2 : 0;
    int id, flags = 0;
    if (hs.yo_t > 0 || hs.muzzle_t > 0) {
        if (hs.dirx != 0) { id = HS_WICK_THROW; flags = hs.dirx < 0 ? SPR_FLIPX : 0; }
        else id = hs.diry < 0 ? HS_WICK_U0 : HS_WICK_D0;
    } else if (hs.dirx != 0) {
        id = fr ? HS_WICK_S1 : HS_WICK_S0;
        flags = hs.dirx < 0 ? SPR_FLIPX : 0;
    } else if (hs.diry < 0) {
        id = fr ? HS_WICK_U1 : HS_WICK_U0;
    } else {
        id = fr ? HS_WICK_D1 : HS_WICK_D0;
    }
    gfx_rect(x + 4, y + 15, 8, 2, C_INK); /* shadow */
    spr_draw(&hs_spr[id], x, y, flags);
    if (sv.pipe && sv.weapon == 1) {
        int gx = x + 8 + hs.dirx * 7, gy = y + 10 + hs.diry * 5;
        gfx_rect(gx - 2, gy - 1, 5, 3, C_SLATE);
        if (hs.muzzle_t > 3) gfx_circ(gx + hs.dirx * 5, gy + hs.diry * 5, 3, C_YELLOW);
    }
    if (hs.yo_t > 0) {
        int hx = x + 8 + hs.dirx * 3, hy = y + 9;
        int yx = hs.yo_x / HS_U, yy = HS_OY + hs.yo_y / HS_U;
        gfx_line(hx, hy, yx, yy, C_WHITE);
        int rc = hs_yoyo_dmg() == 3 ? C_VIOLET : hs_yoyo_dmg() == 2 ? C_GREY : C_RED;
        gfx_circ(yx, yy, 3, rc);
        gfx_pset(yx - 1, yy - 1, C_WHITE);
    }
}

static void draw_shots(void) {
    int f = engine_frame();
    for (int i = 0; i < HS_MAX_SHOTS; i++) {
        const HsShot *s = &hs.shot[i];
        if (!s->on) continue;
        int x = s->x / HS_U, y = HS_OY + s->y / HS_U;
        switch (s->kind) {
        case SH_SLUG: gfx_circ(x, y, 2, C_YELLOW); gfx_pset(x, y, C_WHITE); break;
        case SH_ROCK: gfx_circ(x, y, 3, C_EARTH); gfx_pset(x - 1, y - 1, C_TAN); break;
        case SH_SEED: gfx_circ(x, y, 2, C_LIME); gfx_pset(x, y, C_FOREST); break;
        case SH_SPORE: gfx_circ(x, y, 2 + (f / 6) % 2, C_PINK); break;
        case SH_WEB: gfx_circb(x, y, 3, C_WHITE); gfx_pset(x, y, C_LIGHT); break;
        case SH_BOLT: gfx_rect(x - 2, y - 2, 5, 5, (f / 2) % 2 ? C_CYAN : C_WHITE); break;
        case SH_BOMB: gfx_circ(x, y - (s->life > 0 ? (s->life * (30 - s->life)) / 20 : 0), 4, C_INK);
            gfx_pset(x + 2, y - 4, (f / 4) % 2 ? C_YELLOW : C_RED); break;
        case SH_BLAST: gfx_circ(x, y, 14 - s->life / 2, (f / 2) % 2 ? C_ORANGE : C_YELLOW); break;
        case SH_SHELL: gfx_rect(x - 2, y - 2, 5, 5, C_GREY); gfx_rectb(x - 2, y - 2, 5, 5, C_INK); break;
        default: gfx_circ(x, y, 2, C_ORANGE); gfx_pset(x, y, C_YELLOW); break;
        }
    }
}

static void draw_fx(void) {
    for (int i = 0; i < ARRAY_LEN(hs.fxs); i++) {
        const HsFx *e = &hs.fxs[i];
        if (!e->on) continue;
        int x = e->x, y = HS_OY + e->y, t = e->t;
        switch (e->kind) {
        case FX_PUFF: gfx_circb(x, y, 2 + t / 3, t < 10 ? C_WHITE : C_GREY); break;
        case FX_SPARK: for (int k = 0; k < 4; k++) gfx_pset(x + HS_DX[k] * t / 2, y + HS_DY[k] * t / 2, C_YELLOW); break;
        case FX_BOOM: gfx_circ(x, y, imax(1, 10 - t / 2), t % 4 < 2 ? C_ORANGE : C_YELLOW); break;
        case FX_HOP: gfx_hline(x - 4, x + 4, y + 7, C_LIGHT); break;
        default: break;
        }
    }
}

static void draw_items(void) {
    int f = engine_frame();
    for (int i = 0; i < HS_MAX_ITEMS; i++) {
        const HsItem *it = &hs.item[i];
        if (!it->on) continue;
        int x = it->x / HS_U - 4, y = HS_OY + it->y / HS_U - 4 - (it->t < 8 ? (8 - it->t) : (f / 12 + i) % 2);
        hs_draw_icon(it->kind, x, y);
        if (it->kind == IT_GLINT && it->amount > 1) {
            char b[8];
            snprintf(b, sizeof b, "%d", it->amount);
            tiny_draw(b, x + 8, y + 3, C_CREAM);
        }
    }
}

void hs_draw_room(void) {
    gfx_camera(0, 0);
    for (int ty = 0; ty < HS_TH; ty++)
        for (int tx = 0; tx < HS_TW; tx++)
            hs_draw_tile(hs.tile[ty][tx], hs.biome, tx * HS_T, HS_OY + ty * HS_T, tx, ty, hs.room);
    for (int i = 0; i < HS_MAX_THINGS; i++)
        if (hs.thing[i].on) draw_thing(&hs.thing[i]);
    draw_items();
    for (int i = 0; i < HS_MAX_MOBS; i++)
        if (hs.mob[i].on && HS_FOE[hs.mob[i].kind].tier < 3) draw_mob(&hs.mob[i]);
    draw_wick();
    for (int i = 0; i < HS_MAX_MOBS; i++)
        if (hs.mob[i].on && HS_FOE[hs.mob[i].kind].tier == 3) draw_mob(&hs.mob[i]);
    draw_shots();
    draw_fx();
    if (hs.boss >= 0) {
        const HsMob *b = &hs.mob[hs.boss];
        int w = 120, hp = b->hp * w / HS_FOE[b->kind].hp;
        gfx_rect(100, HS_OY + 4, w + 2, 6, C_INK);
        gfx_rect(101, HS_OY + 5, imax(0, hp), 4, C_RED);
        tiny_center(HS_FOE[b->kind].name, 160, HS_OY + 11, C_WHITE);
    }
}

/* ---- camp ----------------------------------------------------------------------------------- */

static void spot_xy(int p, int *x, int *y) { *x = HS_SPOT[p].x; *y = HS_OY + HS_SPOT[p].y; }

static void plot(int x, int y, int w, int h) {
    gfx_rectb(x, y, w, h, C_EARTH);
    for (int k = 2; k < w - 2; k += 4) gfx_pset(x + k, y + h / 2, C_EARTH);
}

void hs_draw_base(int t) {
    gfx_camera(0, 0);
    /* heather and a palisade */
    for (int ty = 0; ty < HS_TH; ty++)
        for (int tx = 0; tx < HS_TW; tx++) {
            int x = tx * HS_T, y = HS_OY + ty * HS_T;
            if (hs.tile[ty][tx] == T_WALL) {
                gfx_rect(x, y, HS_T, HS_T, C_EARTH);
                for (int k = 0; k < 4; k++) {
                    gfx_rect(x + k * 4, y + 1, 3, 14, C_BROWN);
                    gfx_pset(x + k * 4 + 1, y + 1, C_TAN);
                }
            } else {
                hs_draw_tile(hs.tile[ty][tx], B_CAMP, x, y, tx, ty, HS_R_BASE);
            }
        }
    int x, y;
    /* Old Burl and the glintbuds */
    spot_xy(P_BURL, &x, &y);
    spr_draw(&hs_spr[HS_BURL], x, y - 2 + ((t / 40) % 2), 0);
    for (int i = 0; i < HS_PLANTS; i++) {
        int px = 58 + (i % 6) * 14, py = HS_OY + 18 + (i / 6) * 14;
        gfx_rect(px, py + 8, 10, 3, C_EARTH);
        if (i < sv.plants) {
            gfx_vline(px + 5, py + 3, py + 8, C_LEAF);
            gfx_pset(px + 4, py + 5, C_LIME);
            gfx_circ(px + 5, py + 2, 2, (t / 20 + i) % 6 == 0 ? C_WHITE : C_CYAN);
        }
    }
    /* Tolly at the gate */
    spot_xy(P_TOLLY, &x, &y);
    spr_draw(&hs_spr[HS_TOLLY], x, y, 0);
    /* the Glowstone */
    spot_xy(P_STONE, &x, &y);
    {
        int cx = x + 16, cy = y + 16;
        for (int k = 0; k < 16; k++) {
            gfx_hline(cx - k, cx + k, cy - 15 + k, k % 3 ? C_CYAN : C_SKY);
            gfx_hline(cx - k, cx + k, cy + 15 - k, k % 3 ? C_BLUE : C_SKY);
        }
        gfx_line(cx - 4, cy - 7, cx, cy - 11, C_WHITE);
        if ((t / 30) % 3 == 0) gfx_pset(cx + 3, cy - 3, C_WHITE);
    }
    /* the Tumbleweed */
    spot_xy(P_SHIP, &x, &y);
    gfx_rect(x + 6, y + 8, 66, 16, C_LIGHT);
    gfx_rect(x, y + 18, 80, 8, C_GREY);
    gfx_rect(x + 28, y + 1, 22, 9, C_SKY);
    gfx_rectb(x + 28, y + 1, 22, 9, C_INK);
    gfx_rectb(x + 6, y + 8, 66, 16, C_INK);
    gfx_line(x + 60, y + 10, x + 70, y + 24, C_INK); /* a crack in the hull */
    gfx_rect(x + 8, y + 26, 6, 6, C_SLATE);
    gfx_rect(x + 66, y + 26, 6, 6, C_SLATE);
    for (int p = 0; p < 3; p++) gfx_rect(x + 12 + p * 14, y + 19, 10, 5, (sv.parts >> p & 1) ? C_ORANGE : C_DUSK);
    if (sv.research >> RS_FUEL & 1) gfx_rect(x + 60, y + 19, 10, 5, C_LIME);
    if (sv.idol) spr_draw(&hs_spr[HS_ICON0 + IT_IDOL], x + 35, y - 7, 0);
    /* the workbench */
    spot_xy(P_BENCH, &x, &y);
    gfx_rect(x, y + 3, 32, 6, C_BROWN);
    gfx_hline(x, x + 31, y + 3, C_TAN);
    gfx_rect(x + 2, y + 9, 3, 7, C_EARTH);
    gfx_rect(x + 27, y + 9, 3, 7, C_EARTH);
    gfx_rect(x + 11, y, 10, 3, C_AMBER);
    /* huts */
    for (int h = 0; h < 2; h++) {
        spot_xy(P_HUT1 + h, &x, &y);
        int lv = sv.huts[h];
        if (!lv) { plot(x, y, 24, 24); continue; }
        gfx_rect(x + 1, y + 10, 22, 14, C_TAN);
        for (int k = 0; k < 10; k++) gfx_hline(x + 11 - k - 1, x + 12 + k + 1, y + k, C_WINE);
        gfx_rect(x + 9, y + 16, 6, 8, C_EARTH);
        for (int k = 0; k < lv; k++) gfx_rect(x + 3 + k * 7, y + 12, 3, 3, C_YELLOW); /* a light per room */
    }
    /* the smokehouse and Gristle */
    spot_xy(P_SMOKE, &x, &y);
    if (!sv.smoke_built) plot(x, y, 32, 24);
    else {
        gfx_rect(x + 1, y + 6, 30, 18, C_BROWN);
        gfx_rect(x, y + 2, 32, 6, C_MAROON);
        gfx_rect(x + 24, y - 6, 5, 8, C_SLATE);
        if ((t / 10) % 3 != 0) gfx_circ(x + 27 + (t / 10) % 3, y - 9 - (t / 7) % 5, 2, C_GREY);
        for (int k = 0; k < sv.smoke_stock; k++) gfx_rect(x + 3 + k * 4, y + 10, 2, 6, C_MAROON);
        spr_draw(&hs_spr[HS_GRISTLE], x + 12, y + 8, 0);
    }
    /* bins */
    for (int b = 0; b < 2; b++) {
        spot_xy(P_BIN1 + b, &x, &y);
        if (!sv.bins[b]) { plot(x, y, 16, 24); continue; }
        int hgt = sv.bins[b] == 2 ? 24 : 16;
        gfx_rect(x + 1, y + 24 - hgt, 14, hgt, C_GREY);
        gfx_rect(x + 1, y + 24 - hgt, 14, 3, C_LIGHT);
        gfx_rectb(x + 1, y + 24 - hgt, 14, hgt, C_INK);
    }
    /* anvils and their hands */
    for (int a = 0; a < HS_ANVILS; a++) {
        spot_xy(P_ANVIL1 + a, &x, &y);
        if (!(sv.anvils >> a & 1)) { plot(x, y, 16, 16); continue; }
        gfx_rect(x + 1, y + 5, 14, 4, C_SLATE);
        gfx_hline(x + 1, x + 14, y + 5, C_GREY);
        gfx_rect(x + 5, y + 9, 6, 7, C_DUSK);
        if (sv.anvil_hand >> a & 1) {
            spr_draw(&hs_spr[HS_HAND], x, y - 10 + ((t / 8 + a) % 2), 0);
            if ((t / 8 + a) % 4 == 0) gfx_pset(x + 8, y + 4, C_YELLOW);
        }
    }
    /* the Thinker and Dr. Orrery */
    spot_xy(P_LAB, &x, &y);
    gfx_rect(x, y, 16, 32, sv.lab_fixed ? C_BLUE : C_DUSK);
    gfx_rectb(x, y, 16, 32, C_INK);
    gfx_circ(x + 8, y + 9, 5, sv.lab_fixed ? ((t / 15) % 2 ? C_CYAN : C_SKY) : C_SLATE);
    if (!sv.lab_fixed) gfx_line(x + 3, y + 4, x + 13, y + 14, C_INK);
    for (int k = 0; k < sv.lab_hands; k++) gfx_rect(x + 2 + (k % 3) * 4, y + 20 + (k / 3) * 4, 3, 3, C_LIME);
    spot_xy(P_ORRERY, &x, &y);
    spr_draw(&hs_spr[HS_ORRERY], x, y, 0);
    /* Mother Loom's nest */
    if (hs_spot_shown(P_NEST)) {
        spot_xy(P_NEST, &x, &y);
        for (int k = 0; k < 5; k++) gfx_line(x + 12, y + 12, x + k * 6, y + (k % 2) * 23, C_WHITE);
        gfx_circ(x + 12, y + 12, 5, C_VIOLET);
        gfx_pset(x + 11, y + 11, C_WHITE);
    }
    /* standing stones */
    for (int s = 0; s < 3; s++) {
        if (!hs_spot_shown(P_STONE_HASTE + s)) break;
        spot_xy(P_STONE_HASTE + s, &x, &y);
        gfx_rect(x, y, 8, 16, C_SLATE);
        gfx_rect(x + 1, y, 6, 2, C_GREY);
        for (int k = 0; k < sv.stone[s]; k++) gfx_hline(x + 2, x + 5, y + 3 + k * 2, C_YELLOW);
    }
    /* camp's hopstone */
    spot_xy(P_PAD, &x, &y);
    gfx_circ(x + 8, y + 8, 7, C_SLATE);
    gfx_circ(x + 8, y + 8, 5, (t / 10) % 2 ? C_CYAN : C_SKY);
    /* hands with nothing to do stroll about */
    for (int k = 0; k < hs_free_hands() && k < 4; k++) {
        int wxp = 70 + k * 18 + (int)(sinf((float)(t + k * 60) * 0.02f) * 12.0f);
        spr_draw(&hs_spr[HS_HAND], wxp, HS_OY + 44, (t / 60 + k) % 2 ? SPR_FLIPX : 0);
    }
    draw_items();
    draw_wick();
    draw_fx();
}

/* ---- HUD --------------------------------------------------------------------------------------- */

static int res_col(int r, int v) { return v >= hs_cap(r) ? C_RED : C_WHITE; }

static void count(int icon, int v, int x, int y, int col) {
    char b[12];
    snprintf(b, sizeof b, "%d", v);
    hs_draw_icon(icon, x, y);
    tiny_draw(b, x + 9, y + 2, col);
}

void hs_draw_hud(void) {
    gfx_camera(0, 0);
    gfx_rect(0, 0, SCREEN_W, HS_OY, C_INK);
    gfx_hline(0, SCREEN_W - 1, HS_OY - 1, C_DUSK);
    static const int ICON[RES_COUNT] = {IT_GLINT, IT_BAR, IT_JERKY, IT_DATA, IT_THREAD};
    if (!sv.on_trip) {
        /* camp: the stores, red when full */
        int xs[RES_COUNT] = {2, 44, 78, 106, 140};
        for (int r = 0; r < RES_COUNT; r++) count(ICON[r], sv.res[r], xs[r], 2, res_col(r, sv.res[r]));
        char b[32];
        snprintf(b, sizeof b, "HANDS %d/%d", hs_free_hands(), hs_hands());
        tiny_draw(b, 2, 12, C_GREY);
        snprintf(b, sizeof b, "BUDS %d", sv.plants);
        tiny_draw(b, 48, 12, C_GREY);
        if (sv.smoke_built) { snprintf(b, sizeof b, "RACKS %d", sv.smoke_stock); tiny_draw(b, 82, 12, C_GREY); }
        if (sv.odds) count(IT_ODD, sv.odds, 176, 2, C_YELLOW);
        spr_draw(&hs_spr[sv.pipe && sv.weapon ? HS_ICON_PIPE : HS_ICON_YOYO], 306, 2, 0);
        tiny_draw("CAMP", 210, 4, C_AMBER);
        return;
    }
    /* the Wilds: what this trip found (red when the bag is full) */
    int xs[RES_COUNT] = {2, 36, 62, 84, 108};
    for (int r = 0; r < RES_COUNT; r++) {
        int col = sv.res[r] + sv.gain[r] >= hs_cap(r) ? C_RED : C_WHITE;
        count(ICON[r], sv.gain[r], xs[r] + (r > 2 ? -8 : 0), r > 2 ? 11 : 2, col);
    }
    count(IT_ODD, sv.odds, 2, 11, C_YELLOW);
    /* the clock, top centre */
    int s = sv.time_f / 60;
    char b[16];
    snprintf(b, sizeof b, "%d:%02d", s / 60, s % 60);
    int col = s < 30 ? ((engine_frame() / 8) % 2 ? C_RED : C_ORANGE) : C_CREAM;
    int w = text_width_scaled(b, 2);
    text_draw_scaled(b, 160 - w / 2, 2, col, 2);
    /* what she carries */
    int ix = 196;
    for (int p = 0; p < 3; p++)
        if (sv.carry >> p & 1) { hs_draw_icon(IT_PART, ix, 2); ix += 9; }
    if (sv.carry & 8) { hs_draw_icon(IT_GEAR, ix, 2); ix += 9; }
    if (sv.carry & 16) { hs_draw_icon(IT_IDOL, ix, 2); ix += 9; }
    if (sv.letter) { spr_draw(&hs_spr[HS_ICON_LETTER], ix, 2, 0); ix += 9; }
    spr_draw(&hs_spr[sv.pipe && sv.weapon ? HS_ICON_PIPE : HS_ICON_YOYO], 196, 11, 0);
    if (sv.parts == 7 && !sv.idol) {
        snprintf(b, sizeof b, "MAWBO %d/6", sv.mawbo_wins);
        if (hs.area == AR_OVER) tiny_draw(b, 208, 12, C_PINK);
    }
    /* where she is: a dot on the Wilds (the name of a cave or dungeon instead) */
    int mx = 290, my = 3;
    gfx_rect(mx - 1, my - 1, HS_OW_W * 3 + 2, HS_OW_H * 2 + 2, C_DUSK);
    int here = hs.room;
    if (hs.area == AR_CAVE) here = hw.cave_screen[hs.room - HS_R_CAVE0];
    if (hs.area == AR_DUN) here = hw.cave_screen[hs_dun_of(hs.room)];
    if (here >= 0 && here < HS_SCREENS && (engine_frame() / 15) % 2)
        gfx_rect(mx + (here % HS_OW_W) * 3, my + (here / HS_OW_W) * 2, 3, 2, C_YELLOW);
    if (hs.area != AR_OVER) tiny_draw(hs_room_name(hs.room), 208, 12, C_GREY);
}

/* ---- menus -------------------------------------------------------------------------------------- */

void hs_draw_menu(void) {
    int w = 220, h = 24 + hm.n * 11;
    int x = 160 - w / 2, y = 96 - h / 2;
    ui_panel(x, y, w, h, C_NIGHT, C_AMBER);
    text_center(hm.title, 160, y + 5, C_YELLOW);
    for (int i = 0; i < hm.n; i++) {
        const HsMenuLine *l = &hm.line[i];
        int ly = y + 18 + i * 11;
        bool sel = i == hm.sel, info = l->act == M_NOTHING;
        if (sel && !info) gfx_rect(x + 4, ly - 2, w - 8, 11, C_DUSK);
        int col = info ? C_GREY : l->ok ? (sel ? C_WHITE : C_LIGHT) : C_SLATE;
        text_draw(l->label, x + 16, ly, col);
        if (l->cost[0]) tiny_draw(l->cost, x + w - 8 - tiny_width(l->cost), ly + 1, l->ok ? C_CREAM : C_SLATE);
        if (sel && !info) ui_cursor(x + 6, ly, engine_frame());
    }
}

/* ---- the cartridge label ------------------------------------------------------------------------ */

void hs_draw_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_NIGHT);
    for (int i = 0; i < 24; i++) gfx_pset(x + (i * 37) % w, y + (i * 13) % (h / 2), i % 3 ? C_SLATE : C_WHITE);
    gfx_circ(x + w - 22, y + 12, 8, C_CREAM);
    gfx_circ(x + w - 19, y + 10, 7, C_NIGHT);
    for (int xx = 0; xx < w; xx++) gfx_vline(x + xx, y + 38 + (int)(sinf((float)xx * 0.08f) * 3.0f), y + h, C_WINE);
    /* the Tumbleweed in the heather, smoking */
    gfx_rect(x + 8, y + 32, 40, 10, C_LIGHT);
    gfx_rect(x + 4, y + 38, 48, 5, C_GREY);
    gfx_rect(x + 22, y + 27, 12, 6, C_SKY);
    gfx_circ(x + 44 + (t / 12) % 3, y + 26 - (t / 8) % 8, 2, C_GREY);
    /* the Glowstone */
    int cx = x + 70, cy = y + 36;
    for (int k = 0; k < 8; k++) {
        gfx_hline(cx - k, cx + k, cy - 8 + k, C_CYAN);
        gfx_hline(cx - k, cx + k, cy + 8 - k, C_BLUE);
    }
    /* Wick, throwing her yo-yo at it */
    spr_draw(&hs_spr[HS_WICK_THROW], x + 86, y + 34, SPR_FLIPX);
    int reach = (t / 2) % 20;
    reach = reach < 10 ? reach : 20 - reach;
    gfx_line(x + 88, y + 43, x + 86 - reach, y + 42, C_WHITE);
    gfx_circ(x + 86 - reach, y + 42, 2, C_RED);
    if (reach > 8) gfx_pset(cx + 4, cy - 2, C_WHITE);
    spr_draw(&hs_spr[HS_FOE0 + E_SKITTER], x + w - 28 + (t / 20) % 4, y + 36, 0);
    (void)h;
}
