/* UFO 40 - renders the PS Vita LiveArea artwork with the engine itself and
 * writes 8-bit indexed PNGs (the format the Vita requires):
 *   sce_sys/icon0.png                       128 x 128
 *   sce_sys/livearea/contents/bg.png        840 x 500
 *   sce_sys/livearea/contents/startup.png   280 x 158
 */
#include "../../shell/shell.h"
#include "imgwrite.h"

static void stars(int w, int h, int n, uint32_t seed) {
    for (int i = 0; i < n; i++) {
        uint32_t k = (uint32_t)(i + 1) * 2654435761u ^ seed;
        k ^= k >> 15;
        k *= 0x2c1b3c6dU;
        k ^= k >> 12;
        int x = (int)(k % (uint32_t)w), y = (int)((k >> 11) % (uint32_t)h);
        int c = (k >> 5) % 5 == 0 ? C_WHITE : (k >> 7) % 3 == 0 ? C_LIGHT : C_SLATE;
        gfx_pset(x, y, c);
        if (c == C_WHITE && (k >> 9) % 4 == 0) {
            gfx_pset(x - 1, y, C_DUSK); gfx_pset(x + 1, y, C_DUSK);
            gfx_pset(x, y - 1, C_DUSK); gfx_pset(x, y + 1, C_DUSK);
        }
    }
}

static void beam(int cx, int top, int bot, int spread) {
    for (int y = top; y < bot; y++) {
        int hw = 4 + (y - top) * spread / (bot - top);
        gfx_dither(cx - hw, y, hw * 2, 1, C_BLUE, 6);
        gfx_dither(cx - hw / 2, y, hw, 1, C_CYAN, 3);
    }
}

static void scale_write(const char *path, Surface *s, int scale, int out_w, int out_h) {
    /* nearest-neighbour upscale into an exact output size (pad with ink) */
    uint8_t *px = (uint8_t *)calloc((size_t)out_w * out_h, 1);
    for (int y = 0; y < out_h; y++)
        for (int x = 0; x < out_w; x++) {
            int sx = x / scale, sy = y / scale;
            px[y * out_w + x] = (sx < s->w && sy < s->h) ? s->px[sy * s->w + sx] : C_INK;
        }
    png_write_indexed(path, out_w, out_h, px, PALETTE_RGB, PAL_COUNT, 1);
    free(px);
    printf("  wrote %s (%dx%d)\n", path, out_w, out_h);
}

void vita_assets_render(const char *dir) {
    char path[600];
    /* ---- icon0: 128x128 native */
    {
        static uint8_t buf[128 * 128];
        Surface s = {128, 128, buf};
        gfx_set_target(&s);
        gfx_cls(C_NIGHT);
        for (int y = 0; y < 128; y++) gfx_dither(0, y, 128, 1, C_INK, 16 - y / 8);
        stars(128, 128, 40, 7);
        beam(64, 40, 118, 40);
        ui_saucer(22, 12, 0, 4);
        gfx_rect(0, 0, 0, 0, 0);
        ui_logo(64, 70, 2, 0);
        gfx_rectb(0, 0, 128, 128, C_DUSK);
        snprintf(path, sizeof path, "%s/icon0.png", dir);
        scale_write(path, &s, 1, 128, 128);
    }
    /* ---- bg: 280x167 scene, scaled 3x to 840x500 */
    {
        static uint8_t buf[280 * 167];
        Surface s = {280, 167, buf};
        gfx_set_target(&s);
        gfx_cls(C_INK);
        for (int y = 0; y < 167; y++) gfx_dither(0, y, 280, 1, C_NIGHT, y / 8);
        stars(280, 167, 90, 99);
        /* the saucer beaming down three cartridges */
        beam(140, 34, 150, 80);
        ui_saucer(119, 12, 3, 2);
        for (int i = 0; i < 3; i++) {
            const GameDef *g = GAMES[i];
            int x = 52 + i * 62, y = 96 + (i == 1 ? -8 : 0);
            ui_panel(x - 3, y - 3, 58, 42, C_INK, g->cart_accent);
            gfx_clip(x, y, 52, 36);
            g->draw_label(x, y, 52, 36, 20);
            gfx_noclip();
        }
        ui_logo(140, 58, 2, 0);
        tiny_center("FORTY GAMES FROM ANOTHER WORLD", 140, 150, C_SKY);
        tiny_center("3 OF 40 CARTRIDGES LOADED", 140, 158, C_SLATE);
        snprintf(path, sizeof path, "%s/livearea/contents/bg.png", dir);
        scale_write(path, &s, 3, 840, 500);
    }
    /* ---- startup: 140x79 art, scaled 2x to 280x158 */
    {
        static uint8_t buf[140 * 79];
        Surface s = {140, 79, buf};
        gfx_set_target(&s);
        gfx_cls(C_NIGHT);
        stars(140, 79, 25, 5);
        ui_panel(1, 1, 138, 77, C_NIGHT, C_YELLOW);
        ui_saucer(8, 8, 1, 1);
        ui_logo(70, 22, 2, 0);
        text_center("PRESS TO BEAM IN", 70, 58, C_YELLOW);
        snprintf(path, sizeof path, "%s/livearea/contents/startup.png", dir);
        scale_write(path, &s, 2, 280, 158);
    }
    gfx_set_target(NULL);
}
