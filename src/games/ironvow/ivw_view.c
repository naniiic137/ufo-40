/* IRONVOW - the first-person view: walls cast column by column through the
 * grid, the floor and ceiling cast row by row, then everything standing in
 * the halls drawn as flat cut-outs behind the walls' depth. Steps and turns
 * glide over a few frames. This is the only part of the game that uses
 * floats; nothing here changes the game. */
#include "ivw_art.h"

#define VIEW_MAX_W 256
#define STEP_FRAMES 8
#define TURN_FRAMES 6
#define PI_F 3.14159265f

static float zbuf[VIEW_MAX_W];
static int vx0, vy0, VW, VH;
static float cam_x, cam_y, dir_x, dir_y, plane_x, plane_y;

/* ---- what is solid to the eye ---------------------------------------------------------------------- */
static bool solid_view(int x, int y) {
    int t = ivw_tile(x, y);
    switch (t) {
    case T_WALL: case T_DOOR: case T_LOCKED: case T_HIDDEN: return true;
    case T_SHIFT_A: return ig.w.shift == 0;
    case T_SHIFT_B: return ig.w.shift == 1;
    case T_EXIT: return !ig.w.escape;
    default: return IVW_IS_DECOR(t);
    }
}

static int shade(int c, int steps) {
    if (steps >= 6) return C_INK;
    for (int i = 0; i < steps; i++) c = PAL_DARKER[c];
    return c;
}

static int dist_steps(float d) {
    int s = (int)(d * 1.15f) - 1;
    return s < 0 ? 0 : s;
}

static const Sprite *decal_for(int x, int y) {
    int t = ivw_tile(x, y);
    int st = ig.w.st[y][x];
    switch (t) {
    case T_TORCH: return st ? &ivw_decal[(ig.frame_t / 8) & 1 ? DEC_TORCH_LIT1 : DEC_TORCH_LIT0] : &ivw_decal[DEC_TORCH_OUT];
    case T_FUNGUS: return &ivw_decal[st ? DEC_FUNGUS_LIT : DEC_FUNGUS_OUT];
    case T_LANTERN: return &ivw_decal[st ? DEC_LANTERN_ON : DEC_LANTERN_OFF];
    case T_LEVER: return &ivw_decal[st ? DEC_LEVER_DOWN : DEC_LEVER_UP];
    case T_SWITCH: return &ivw_decal[DEC_SWITCH];
    case T_GEMS: return &ivw_decal[st ? DEC_GEMS_DONE : DEC_GEMS];
    case T_MURAL: return &ivw_glyph_decal[ig.w.id[y][x] % SP_COUNT];
    case T_HIDDEN:
        /* the Warren's outlines are much harder to make out */
        return &ivw_decal[IVW_FLOOR[ig.hero.floor].biome == BIO_WARREN ? DEC_HIDDEN_FAINT : DEC_HIDDEN];
    default: return NULL;
    }
}

static int wall_texel(int x, int y, int u, int v) {
    int t = ivw_tile(x, y);
    const Sprite *tex;
    if (t == T_DOOR) tex = &ivw_tex[TEX_DOOR];
    else if (t == T_LOCKED) tex = &ivw_tex[TEX_LOCKED];
    else tex = &ivw_tex[IVW_FLOOR[ig.hero.floor].biome];
    const Sprite *d = decal_for(x, y);
    if (d) {
        uint8_t c = d->px[v * d->w + u];
        if (c != TRANSPARENT) return c;
    }
    return tex->px[v * tex->w + u];
}

/* ---- floor and ceiling ------------------------------------------------------------------------------- */
static int floor_texel(int tx, int ty, float fu, float fv) {
    int t = ivw_tile(tx, ty);
    int biome = IVW_FLOOR[ig.hero.floor].biome;
    int u = (int)(fu * 8), v = (int)(fv * 8);
    bool edge = u == 0 || v == 0;
    switch (t) {
    case T_PIT: return C_INK;
    case T_LAVA: return ((u + v + ig.frame_t / 10) % 4) ? C_ORANGE : C_RED;
    case T_SLIME_A: case T_SLIME_B: {
        bool wet = (t == T_SLIME_A) == !ig.w.slime_b;
        if (wet) return ((u * 3 + v + ig.frame_t / 16) % 5) ? C_LEAF : C_LIME;
        break;
    }
    case T_SPIKES:
        if ((u & 1) && (v & 1)) return ivw_spikes_up() ? C_WHITE : C_SLATE;
        break;
    case T_STAIRS: return v > 4 || u < 2 || u > 5 ? C_INK : C_DUSK;
    default: break;
    }
    bool search = tx >= 0 && ty >= 0 && tx < IVW_W && ty < IVW_H && (ig.w.st[ty][tx] & 0x80);
    if (search && u >= 3 && u <= 4 && v >= 3 && v <= 4) return biome == BIO_GAOL ? C_SLATE : C_TAN;
    bool speck = ((u * 3 + v * 5 + tx * 7 + ty * 3) % 11) == 0;
    if (biome == BIO_GAOL) return edge ? C_NIGHT : speck ? C_SLATE : C_DUSK;
    if (biome == BIO_WARREN) return edge ? C_MAROON : speck ? C_TAN : C_BROWN;
    return edge ? C_PURPLE : speck ? C_VIOLET : C_NIGHT;
}

static int ceil_texel(float fu, float fv) {
    int biome = IVW_FLOOR[ig.hero.floor].biome;
    int u = (int)(fu * 8), v = (int)(fv * 8);
    bool edge = u == 0 || v == 0;
    if (biome == BIO_GAOL) return edge ? C_INK : C_NIGHT;
    if (biome == BIO_WARREN) return edge ? C_INK : C_MAROON;
    return ((u * 5 + v * 3) % 23 == 0) ? C_DUSK : C_INK;
}

static void put(int x, int y, int c) { g_screen.px[(vy0 + y) * SCREEN_W + vx0 + x] = (uint8_t)c; }

static void draw_floor(void) {
    int half = VH / 2;
    float focal = VW * 0.25f;
    for (int y = 0; y < VH; y++) {
        int p = y - half;
        if (p == 0) { for (int x = 0; x < VW; x++) put(x, y, C_INK); continue; }
        float dist = focal / (fabsf((float)p) - 0.5f + 1.0f);
        float lx = cam_x + dist * (dir_x - plane_x), ly = cam_y + dist * (dir_y - plane_y);
        float sx = dist * 2 * plane_x / VW, sy = dist * 2 * plane_y / VW;
        int steps = dist_steps(dist);
        for (int x = 0; x < VW; x++) {
            int tx = (int)floorf(lx), ty = (int)floorf(ly);
            float fu = lx - tx, fv = ly - ty;
            int c = p > 0 ? floor_texel(tx, ty, fu, fv) : ceil_texel(fu, fv);
            put(x, y, shade(c, steps));
            lx += sx; ly += sy;
        }
    }
}

/* ---- walls ---------------------------------------------------------------------------------------------- */
typedef struct { float dist; int mx, my, side; float wx; } Layer;

static void draw_walls(void) {
    int half = VH / 2;
    for (int x = 0; x < VW; x++) {
        float camx = 2.0f * x / VW - 1.0f;
        float rx = dir_x + plane_x * camx, ry = dir_y + plane_y * camx;
        int mx = (int)floorf(cam_x), my = (int)floorf(cam_y);
        float ddx = rx == 0 ? 1e30f : fabsf(1.0f / rx), ddy = ry == 0 ? 1e30f : fabsf(1.0f / ry);
        int stx, sty;
        float sdx, sdy;
        if (rx < 0) { stx = -1; sdx = (cam_x - mx) * ddx; } else { stx = 1; sdx = (mx + 1.0f - cam_x) * ddx; }
        if (ry < 0) { sty = -1; sdy = (cam_y - my) * ddy; } else { sty = 1; sdy = (my + 1.0f - cam_y) * ddy; }
        int side = 0;
        bool hit = false;
        Layer gates[4];
        int ng = 0;
        for (int k = 0; k < 24 && !hit; k++) {
            if (sdx < sdy) { sdx += ddx; mx += stx; side = 0; }
            else { sdy += ddy; my += sty; side = 1; }
            if (mx < 0 || my < 0 || mx >= IVW_W || my >= IVW_H) { hit = true; break; }
            int t = ivw_tile(mx, my);
            if (t == T_GATE && ng < 4) {
                float d = side == 0 ? sdx - ddx : sdy - ddy;
                float wx = side == 0 ? cam_y + d * ry : cam_x + d * rx;
                gates[ng++] = (Layer){d, mx, my, side, wx - floorf(wx)};
            }
            if (solid_view(mx, my)) hit = true;
        }
        float perp = side == 0 ? sdx - ddx : sdy - ddy;
        if (perp < 0.05f) perp = 0.05f;
        zbuf[x] = perp;
        if (hit) {
            float wx = side == 0 ? cam_y + perp * ry : cam_x + perp * rx;
            wx -= floorf(wx);
            int u = (int)(wx * 32);
            if ((side == 0 && rx > 0) || (side == 1 && ry < 0)) u = 31 - u;
            u = iclamp(u, 0, 31);
            float lh = VW * 0.5f / perp;
            int top = (int)(half - lh / 2), bot = (int)(half + lh / 2);
            int steps = dist_steps(perp) + side;
            for (int y = imax(0, top); y < imin(VH, bot); y++) {
                int v = iclamp((int)((y - top) * 32 / lh), 0, 31);
                int c = (mx >= 0 && my >= 0 && mx < IVW_W && my < IVW_H) ? wall_texel(mx, my, u, v) : C_INK;
                put(x, y, shade(c, steps));
            }
        }
        /* the bars of gates in front of the wall, far ones first */
        for (int g = ng - 1; g >= 0; g--) {
            if (gates[g].dist >= perp) continue;
            const Sprite *tex = &ivw_tex[TEX_GATE];
            int u = iclamp((int)(gates[g].wx * 32), 0, 31);
            float lh = VW * 0.5f / gates[g].dist;
            int top = (int)(half - lh / 2), bot = (int)(half + lh / 2);
            int steps = dist_steps(gates[g].dist) + gates[g].side;
            for (int y = imax(0, top); y < imin(VH, bot); y++) {
                int v = iclamp((int)((y - top) * 32 / lh), 0, 31);
                uint8_t c = tex->px[v * 32 + u];
                if (c != TRANSPARENT) put(x, y, shade(c, steps));
            }
            zbuf[x] = fminf(zbuf[x], gates[g].dist + 0.01f);
        }
    }
}

/* ---- cut-outs ---------------------------------------------------------------------------------------------- */
typedef struct {
    const Sprite *s;
    float x, y, d, height;
    const uint8_t *remap;
} Cutout;

static Cutout cut[96];
static int ncut;

static void add_cut(const Sprite *s, float x, float y, float height, const uint8_t *remap) {
    if (ncut >= ARRAY_LEN(cut) || !s || !s->px) return;
    float dx = x - cam_x, dy = y - cam_y;
    float d = dx * dx + dy * dy;
    if (d > 64.0f) return;
    cut[ncut++] = (Cutout){s, x, y, d, height, remap};
}

static void draw_cut(const Cutout *c) {
    float inv = 1.0f / (plane_x * dir_y - dir_x * plane_y);
    float rx = c->x - cam_x, ry = c->y - cam_y;
    float tx = inv * (dir_y * rx - dir_x * ry);
    float ty = inv * (-plane_y * rx + plane_x * ry);
    if (ty <= 0.1f) return;
    int half = VH / 2;
    float hpx = VW * 0.5f / ty * c->height;
    float wpx = hpx * c->s->w / c->s->h;
    float sx = VW * 0.5f * (1 + tx / ty);
    float floor_y = half + VW * 0.25f / ty;
    int x0 = (int)(sx - wpx / 2), x1 = (int)(sx + wpx / 2);
    int y0 = (int)(floor_y - hpx), y1 = (int)floor_y;
    int steps = dist_steps(ty);
    for (int x = imax(0, x0); x < imin(VW, x1); x++) {
        if (ty >= zbuf[x]) continue;
        int u = iclamp((int)((x - x0) * c->s->w / wpx), 0, c->s->w - 1);
        for (int y = imax(0, y0); y < imin(VH, y1); y++) {
            int v = iclamp((int)((y - y0) * c->s->h / hpx), 0, c->s->h - 1);
            uint8_t col = c->s->px[v * c->s->w + u];
            if (col == TRANSPARENT) continue;
            if (c->remap) col = c->remap[col];
            put(x, y, shade(col, steps));
        }
    }
}

static int cut_cmp(const void *a, const void *b) {
    float da = ((const Cutout *)a)->d, db = ((const Cutout *)b)->d;
    return da < db ? 1 : da > db ? -1 : 0;
}

static uint8_t map_sweet[PAL_COUNT], map_foul[PAL_COUNT], map_free[PAL_COUNT];

static void gather_cuts(void) {
    IvwWorld *w = &ig.w;
    ncut = 0;
    static bool maps_ready;
    if (!maps_ready) {
        pal_identity(map_sweet); pal_identity(map_foul);
        pal_swap(map_sweet, C_BLUE, C_SKY);
        pal_swap(map_foul, C_BLUE, C_LEAF);
        /* the last throne, the free one, is another colour */
        pal_identity(map_free);
        pal_swap(map_free, C_WINE, C_NAVY);
        pal_swap(map_free, C_RED, C_BLUE);
        pal_swap(map_free, C_AMBER, C_LIGHT);
        pal_swap(map_free, C_YELLOW, C_WHITE);
        maps_ready = true;
    }
    for (int i = 0; i < w->nobj; i++) {
        const IvwObj *o = &w->obj[i];
        float x = o->x + 0.5f, y = o->y + 0.5f;
        switch (o->kind) {
        case OB_CHEST: add_cut(&ivw_spr[o->state ? SPR_CHEST_OPEN : SPR_CHEST], x, y, 0.55f, NULL); break;
        case OB_MIMIC: add_cut(&ivw_spr[SPR_CHEST], x, y, 0.55f, NULL); break;
        case OB_SWEET_WELL: add_cut(&ivw_spr[SPR_WELL], x, y, 0.7f, map_sweet); break;
        case OB_FOUL_WELL: add_cut(&ivw_spr[SPR_WELL], x, y, 0.7f, map_foul); break;
        case OB_THRONE: add_cut(&ivw_spr[SPR_THRONE], x, y, 0.85f, ig.hero.floor == 6 ? map_free : NULL); break;
        case OB_PEDLAR: add_cut(&ivw_spr[SPR_PEDLAR], x, y, 0.75f, NULL); break;
        case OB_KNIGHT: add_cut(&ivw_spr[SPR_KNIGHT_NPC], x, y, 0.85f, NULL); break;
        case OB_PILLAR: add_cut(&ivw_spr[o->param ? SPR_RACK : SPR_PILLAR], x, y, o->param ? 0.75f : 1.0f, NULL); break;
        case OB_SOCKET: add_cut(&ivw_spr[SPR_SOCKET], x, y, 1.0f, NULL); break;
        case OB_BOSS: {
            int k = o->param == BOSS_WARDEN ? FK_WARDEN : FK_MATRON;
            add_cut(&ivw_foe_spr[k][0], x, y, 0.95f, NULL);
            break;
        }
        default: break;
        }
    }
    for (int i = 0; i < w->ndrop; i++) add_cut(&ivw_spr[SPR_ITEM_BAG], w->drop[i].x + 0.5f, w->drop[i].y + 0.5f, 0.5f, NULL);
    for (int i = 0; i < IVW_MAX_FLAME; i++)
        if (w->flame[i].alive) {
            float bob = 0.04f * sinf((ig.frame_t + i * 20) * 0.15f);
            add_cut(&ivw_spr[SPR_FLAME], w->flame[i].x + 0.5f, w->flame[i].y + 0.5f, 0.6f + bob, NULL);
        }
    for (int y = 0; y < IVW_H; y++)
        for (int x = 0; x < IVW_W; x++) {
            if (w->tile[y][x] == T_STAIRS) add_cut(&ivw_spr[SPR_STAIRS], x + 0.5f, y + 0.5f, 0.6f, NULL);
            if (w->tile[y][x] == T_EXIT && w->escape) add_cut(&ivw_tex[TEX_EXIT], x + 0.5f, y + 0.5f, 1.0f, NULL);
        }
    qsort(cut, (size_t)ncut, sizeof cut[0], cut_cmp);
}

/* ---- the camera, gliding through steps and turns ------------------------------------------------------------------ */
static float dir_angle(int d) { return (d - 1) * PI_F / 2; }

static void set_camera(void) {
    IvwHero *h = &ig.hero;
    float x = h->x + 0.5f, y = h->y + 0.5f, a = dir_angle(h->dir);
    if (ig.anim_kind == 1 && ig.anim_t > 0) {
        float t = 1.0f - (float)ig.anim_t / STEP_FRAMES;
        x = ig.from_x + 0.5f + (x - ig.from_x - 0.5f) * t;
        y = ig.from_y + 0.5f + (y - ig.from_y - 0.5f) * t;
    } else if (ig.anim_kind == 2 && ig.anim_t > 0) {
        float t = 1.0f - (float)ig.anim_t / TURN_FRAMES;
        int diff = (h->dir - ig.from_dir + 4) & 3;
        float turn = diff == 3 ? -1.0f : diff == 1 ? 1.0f : 2.0f;
        a = dir_angle(ig.from_dir) + turn * PI_F / 2 * t;
    }
    cam_x = x; cam_y = y;
    dir_x = cosf(a); dir_y = sinf(a);
    plane_x = -dir_y; plane_y = dir_x;
}

void ivw_view_draw(int vx, int vy, int vw, int vh) {
    vx0 = vx; vy0 = vy;
    VW = imin(vw, VIEW_MAX_W);
    VH = vh;
    set_camera();
    draw_floor();
    draw_walls();
    gather_cuts();
    for (int i = 0; i < ncut; i++) draw_cut(&cut[i]);
}
