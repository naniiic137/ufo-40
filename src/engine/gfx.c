/* UFO 40 - indexed-colour software renderer. */
#include "gfx.h"
#include <stdlib.h>
#include <string.h>

/* The UFO 40 house palette: 32 colours in seven ramps. Original. */
const uint8_t PALETTE_RGB[PAL_COUNT][3] = {
    {0x0e, 0x0b, 0x16}, /* k ink     */
    {0x1f, 0x1a, 0x33}, /* n night   */
    {0x3a, 0x33, 0x56}, /* d dusk    */
    {0x5e, 0x58, 0x7c}, /* s slate   */
    {0x91, 0x8b, 0xab}, /* g grey    */
    {0xc9, 0xc5, 0xde}, /* l light   */
    {0xf7, 0xf4, 0xff}, /* w white   */
    {0x41, 0x19, 0x2b}, /* m maroon  */
    {0x82, 0x2b, 0x43}, /* v wine    */
    {0xcf, 0x41, 0x40}, /* r red     */
    {0xf0, 0x77, 0x3f}, /* o orange  */
    {0xff, 0xae, 0x4a}, /* a amber   */
    {0xff, 0xe0, 0x6b}, /* y yellow  */
    {0xff, 0xf6, 0xc7}, /* c cream   */
    {0x4f, 0x2d, 0x23}, /* b brown   */
    {0x8b, 0x55, 0x33}, /* t tan     */
    {0xc9, 0x8f, 0x58}, /* e earth   */
    {0xef, 0xcb, 0x9e}, /* h hide    */
    {0x11, 0x30, 0x3a}, /* q teal    */
    {0x1b, 0x5c, 0x4a}, /* f forest  */
    {0x2d, 0x94, 0x54}, /* j jade    */
    {0x6c, 0xcb, 0x55}, /* z leaf    */
    {0xc6, 0xf2, 0x7e}, /* i lime    */
    {0x18, 0x2b, 0x63}, /* N navy    */
    {0x27, 0x59, 0xb3}, /* B blue    */
    {0x3d, 0x96, 0xe6}, /* u sky     */
    {0x79, 0xd6, 0xf4}, /* C cyan    */
    {0xd0, 0xf7, 0xff}, /* I ice     */
    {0x4d, 0x2a, 0x7a}, /* p purple  */
    {0x8c, 0x54, 0xcc}, /* V violet  */
    {0xd9, 0x7f, 0xe2}, /* P magenta */
    {0xff, 0xa6, 0xc6}, /* K pink    */
};

const uint8_t PAL_DARKER[PAL_COUNT] = {
    0, 0, 1, 2, 3, 4, 5,          /* greys  */
    0, 7, 8, 9, 10, 11, 12,       /* warm   */
    7, 14, 15, 16,                /* browns */
    0, 18, 19, 20, 21,            /* greens */
    1, 23, 24, 25, 26,            /* blues  */
    1, 28, 29, 30                 /* purple */
};

const uint8_t PAL_LIGHTER[PAL_COUNT] = {
    1, 2, 3, 4, 5, 6, 6,
    8, 9, 10, 11, 12, 13, 6,
    15, 16, 17, 13,
    19, 20, 21, 22, 13,
    24, 25, 26, 27, 6,
    29, 30, 31, 6
};

static const char PAL_CHARS[PAL_COUNT + 1] = "kndsglwmvroaycbtehqfjziNBuCIpVPK";
static int8_t char_to_index[256];

static uint8_t screen_px[SCREEN_W * SCREEN_H];
Surface g_screen = {SCREEN_W, SCREEN_H, screen_px};

static Surface *target = &g_screen;
static int clip_x0, clip_y0, clip_x1, clip_y1; /* inclusive-exclusive */
static int cam_x, cam_y;
static int fade_level;
static int flash_frames;

static const uint8_t BAYER4[4][4] = {
    {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

void gfx_init(void) {
    memset(char_to_index, -1, sizeof char_to_index);
    for (int i = 0; i < PAL_COUNT; i++) char_to_index[(uint8_t)PAL_CHARS[i]] = (int8_t)i;
    gfx_set_target(NULL);
    fade_level = 0;
    flash_frames = 0;
}

int pal_char_index(char ch) { return char_to_index[(uint8_t)ch]; }

void gfx_set_target(Surface *s) {
    target = s ? s : &g_screen;
    gfx_noclip();
    cam_x = cam_y = 0;
}
Surface *gfx_get_target(void) { return target; }

void gfx_clip(int x, int y, int w, int h) {
    clip_x0 = x < 0 ? 0 : x;
    clip_y0 = y < 0 ? 0 : y;
    clip_x1 = x + w > target->w ? target->w : x + w;
    clip_y1 = y + h > target->h ? target->h : y + h;
}
void gfx_noclip(void) {
    clip_x0 = clip_y0 = 0;
    clip_x1 = target->w;
    clip_y1 = target->h;
}
void gfx_camera(int x, int y) { cam_x = x; cam_y = y; }
int gfx_cam_x(void) { return cam_x; }
int gfx_cam_y(void) { return cam_y; }

void gfx_cls(int c) { memset(target->px, c, (size_t)target->w * target->h); }

void gfx_pset(int x, int y, int c) {
    x -= cam_x; y -= cam_y;
    if (x < clip_x0 || y < clip_y0 || x >= clip_x1 || y >= clip_y1) return;
    target->px[y * target->w + x] = (uint8_t)c;
}

int gfx_pget(int x, int y) {
    x -= cam_x; y -= cam_y;
    if (x < 0 || y < 0 || x >= target->w || y >= target->h) return 0;
    return target->px[y * target->w + x];
}

void gfx_hline(int x0, int x1, int y, int c) {
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    gfx_rect(x0, y, x1 - x0 + 1, 1, c);
}
void gfx_vline(int x, int y0, int y1, int c) {
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    gfx_rect(x, y0, 1, y1 - y0 + 1, c);
}

void gfx_rect(int x, int y, int w, int h, int c) {
    x -= cam_x; y -= cam_y;
    int x0 = x < clip_x0 ? clip_x0 : x, y0 = y < clip_y0 ? clip_y0 : y;
    int x1 = x + w > clip_x1 ? clip_x1 : x + w, y1 = y + h > clip_y1 ? clip_y1 : y + h;
    if (x0 >= x1 || y0 >= y1) return;
    for (int yy = y0; yy < y1; yy++) memset(target->px + yy * target->w + x0, c, (size_t)(x1 - x0));
}

void gfx_rectb(int x, int y, int w, int h, int c) {
    if (w <= 0 || h <= 0) return;
    gfx_rect(x, y, w, 1, c);
    gfx_rect(x, y + h - 1, w, 1, c);
    gfx_rect(x, y + 1, 1, h - 2, c);
    gfx_rect(x + w - 1, y + 1, 1, h - 2, c);
}

void gfx_line(int x0, int y0, int x1, int y1, int c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        gfx_pset(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_circ(int cx, int cy, int r, int c) {
    if (r < 0) return;
    for (int y = -r; y <= r; y++) {
        int w = 0;
        while ((w + 1) * (w + 1) + y * y <= r * r + r) w++;
        gfx_rect(cx - w, cy + y, 2 * w + 1, 1, c);
    }
}

void gfx_circb(int cx, int cy, int r, int c) {
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        gfx_pset(cx + x, cy + y, c); gfx_pset(cx - x, cy + y, c);
        gfx_pset(cx + x, cy - y, c); gfx_pset(cx - x, cy - y, c);
        gfx_pset(cx + y, cy + x, c); gfx_pset(cx - y, cy + x, c);
        gfx_pset(cx + y, cy - x, c); gfx_pset(cx - y, cy - x, c);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void gfx_dither(int x, int y, int w, int h, int c, int level) {
    if (level <= 0) return;
    if (level >= 16) { gfx_rect(x, y, w, h, c); return; }
    x -= cam_x; y -= cam_y;
    int x0 = x < clip_x0 ? clip_x0 : x, y0 = y < clip_y0 ? clip_y0 : y;
    int x1 = x + w > clip_x1 ? clip_x1 : x + w, y1 = y + h > clip_y1 ? clip_y1 : y + h;
    for (int yy = y0; yy < y1; yy++) {
        uint8_t *row = target->px + yy * target->w;
        for (int xx = x0; xx < x1; xx++)
            if (BAYER4[yy & 3][xx & 3] < level) row[xx] = (uint8_t)c;
    }
}

void gfx_dither_circle(int cx, int cy, int r, int c, int level) {
    if (level <= 0 || r <= 0) return;
    for (int yy = -r; yy <= r; yy++) {
        int w = 0;
        while ((w + 1) * (w + 1) + yy * yy <= r * r) w++;
        int y = cy + yy - cam_y;
        if (y < clip_y0 || y >= clip_y1) continue;
        uint8_t *row = target->px + y * target->w;
        for (int xx = cx - w; xx <= cx + w; xx++) {
            int x = xx - cam_x;
            if (x < clip_x0 || x >= clip_x1) continue;
            if (level >= 16 || BAYER4[y & 3][x & 3] < level) row[x] = (uint8_t)c;
        }
    }
}

void gfx_remap_rect(int x, int y, int w, int h, const uint8_t *map) {
    x -= cam_x; y -= cam_y;
    int x0 = x < clip_x0 ? clip_x0 : x, y0 = y < clip_y0 ? clip_y0 : y;
    int x1 = x + w > clip_x1 ? clip_x1 : x + w, y1 = y + h > clip_y1 ? clip_y1 : y + h;
    for (int yy = y0; yy < y1; yy++) {
        uint8_t *row = target->px + yy * target->w;
        for (int xx = x0; xx < x1; xx++) row[xx] = map[row[xx] & 31];
    }
}

void gfx_darken_rect(int x, int y, int w, int h, int steps) {
    uint8_t map[PAL_COUNT];
    for (int i = 0; i < PAL_COUNT; i++) {
        int c = i;
        for (int s = 0; s < steps; s++) c = PAL_DARKER[c];
        map[i] = (uint8_t)c;
    }
    gfx_remap_rect(x, y, w, h, map);
}

/* ---- sprites ---------------------------------------------------------- */

void spr_make_sub(Sprite *s, int w, int h, const char *data, int stride, int sx, int sy) {
    s->w = (int16_t)w;
    s->h = (int16_t)h;
    s->px = (uint8_t *)malloc((size_t)w * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            char ch = data[(sy + y) * stride + sx + x];
            int idx = pal_char_index(ch);
            s->px[y * w + x] = idx < 0 ? TRANSPARENT : (uint8_t)idx;
        }
}

void spr_make(Sprite *s, int w, int h, const char *data) { spr_make_sub(s, w, h, data, w, 0, 0); }

void spr_free(Sprite *s) {
    free(s->px);
    s->px = NULL;
}

void spr_draw_ex(const Sprite *s, int x, int y, int flags, const uint8_t *remap, int solid) {
    if (!s || !s->px) return;
    x -= cam_x; y -= cam_y;
    if (x >= clip_x1 || y >= clip_y1 || x + s->w <= clip_x0 || y + s->h <= clip_y0) return;
    int sy0 = y < clip_y0 ? clip_y0 - y : 0, sy1 = y + s->h > clip_y1 ? clip_y1 - y : s->h;
    int sx0 = x < clip_x0 ? clip_x0 - x : 0, sx1 = x + s->w > clip_x1 ? clip_x1 - x : s->w;
    for (int sy = sy0; sy < sy1; sy++) {
        int srcy = (flags & SPR_FLIPY) ? s->h - 1 - sy : sy;
        const uint8_t *src = s->px + srcy * s->w;
        uint8_t *dst = target->px + (y + sy) * target->w + x;
        for (int sx = sx0; sx < sx1; sx++) {
            int srcx = (flags & SPR_FLIPX) ? s->w - 1 - sx : sx;
            uint8_t c = src[srcx];
            if (c == TRANSPARENT) continue;
            if (solid >= 0) c = (uint8_t)solid;
            else if (remap) c = remap[c];
            dst[sx] = c;
        }
    }
}

void spr_draw(const Sprite *s, int x, int y, int flags) { spr_draw_ex(s, x, y, flags, NULL, -1); }

void spr_draw_outline(const Sprite *s, int x, int y, int flags, int oc) {
    spr_draw_ex(s, x - 1, y, flags, NULL, oc);
    spr_draw_ex(s, x + 1, y, flags, NULL, oc);
    spr_draw_ex(s, x, y - 1, flags, NULL, oc);
    spr_draw_ex(s, x, y + 1, flags, NULL, oc);
    spr_draw_ex(s, x, y, flags, NULL, -1);
}

void spr_draw_scaled(const Sprite *s, int x, int y, int scale, int flags) {
    if (!s || !s->px) return;
    for (int sy = 0; sy < s->h; sy++)
        for (int sx = 0; sx < s->w; sx++) {
            int srcx = (flags & SPR_FLIPX) ? s->w - 1 - sx : sx;
            int srcy = (flags & SPR_FLIPY) ? s->h - 1 - sy : sy;
            uint8_t c = s->px[srcy * s->w + srcx];
            if (c != TRANSPARENT) gfx_rect(x + sx * scale, y + sy * scale, scale, scale, c);
        }
}

void pal_identity(uint8_t *map) {
    for (int i = 0; i < PAL_COUNT; i++) map[i] = (uint8_t)i;
}
void pal_swap(uint8_t *map, int from, int to) { map[from] = (uint8_t)to; }

/* ---- presentation ------------------------------------------------------ */

void gfx_set_fade(int level) { fade_level = level < 0 ? 0 : level > 7 ? 7 : level; }
int gfx_get_fade(void) { return fade_level; }
void gfx_set_flash(int frames) { flash_frames = frames; }
void gfx_tick_effects(void) {
    if (flash_frames > 0) flash_frames--;
}

void gfx_build_lut(uint32_t lut[PAL_COUNT], int argb) {
    for (int i = 0; i < PAL_COUNT; i++) {
        int c = i;
        for (int f = 0; f < fade_level; f++) c = PAL_DARKER[c];
        if (flash_frames > 0) c = PAL_LIGHTER[PAL_LIGHTER[c]];
        const uint8_t *rgb = PALETTE_RGB[c];
        if (argb)
            lut[i] = 0xFF000000u | ((uint32_t)rgb[0] << 16) | ((uint32_t)rgb[1] << 8) | rgb[2];
        else
            lut[i] = 0xFF000000u | ((uint32_t)rgb[2] << 16) | ((uint32_t)rgb[1] << 8) | rgb[0];
    }
}

void gfx_present(uint32_t *dst, int pitch_pixels, const uint32_t lut[PAL_COUNT]) {
    const uint8_t *src = g_screen.px;
    for (int y = 0; y < SCREEN_H; y++) {
        uint32_t *row = dst + y * pitch_pixels;
        for (int x = 0; x < SCREEN_W; x++) row[x] = lut[src[x] & 31];
        src += SCREEN_W;
    }
}
