/* UFO 40 - indexed-colour software renderer.
 * Everything draws into 8-bit palette-indexed surfaces. The platform layer
 * converts the screen surface to RGB once per presented frame. */
#ifndef UFO_GFX_H
#define UFO_GFX_H

#include <stdint.h>
#include <stdbool.h>

#define SCREEN_W 320
#define SCREEN_H 180
#define PAL_COUNT 32
#define TRANSPARENT 0xFF

/* Palette: 32 original colours, grouped in ramps.
 * The single-letter names are the characters used in sprite strings. */
enum {
    C_INK = 0,    /* k */
    C_NIGHT,      /* n */
    C_DUSK,       /* d */
    C_SLATE,      /* s */
    C_GREY,       /* g */
    C_LIGHT,      /* l */
    C_WHITE,      /* w */
    C_MAROON,     /* m */
    C_WINE,       /* v */
    C_RED,        /* r */
    C_ORANGE,     /* o */
    C_AMBER,      /* a */
    C_YELLOW,     /* y */
    C_CREAM,      /* c */
    C_BROWN,      /* b */
    C_TAN,        /* t */
    C_EARTH,      /* e */
    C_HIDE,       /* h */
    C_TEAL,       /* q */
    C_FOREST,     /* f */
    C_JADE,       /* j */
    C_LEAF,       /* z */
    C_LIME,       /* i */
    C_NAVY,       /* N */
    C_BLUE,       /* B */
    C_SKY,        /* u */
    C_CYAN,       /* C */
    C_ICE,        /* I */
    C_PURPLE,     /* p */
    C_VIOLET,     /* V */
    C_MAGENTA,    /* P */
    C_PINK        /* K */
};

extern const uint8_t PALETTE_RGB[PAL_COUNT][3];
extern const uint8_t PAL_DARKER[PAL_COUNT];
extern const uint8_t PAL_LIGHTER[PAL_COUNT];

typedef struct Surface {
    int w, h;
    uint8_t *px;
} Surface;

typedef struct Sprite {
    int16_t w, h;
    uint8_t *px; /* w*h indices, TRANSPARENT = skip */
} Sprite;

enum { SPR_FLIPX = 1, SPR_FLIPY = 2 };

extern Surface g_screen;

void gfx_init(void);
void gfx_set_target(Surface *s);     /* NULL = screen */
Surface *gfx_get_target(void);
void gfx_clip(int x, int y, int w, int h);
void gfx_noclip(void);
void gfx_camera(int x, int y);
int gfx_cam_x(void);
int gfx_cam_y(void);

void gfx_cls(int c);
void gfx_pset(int x, int y, int c);
int gfx_pget(int x, int y);
void gfx_hline(int x0, int x1, int y, int c);
void gfx_vline(int x, int y0, int y1, int c);
void gfx_rect(int x, int y, int w, int h, int c);      /* filled */
void gfx_rectb(int x, int y, int w, int h, int c);     /* outline */
void gfx_line(int x0, int y0, int x1, int y1, int c);
void gfx_circ(int cx, int cy, int r, int c);           /* filled */
void gfx_circb(int cx, int cy, int r, int c);          /* outline */
/* Ordered-dither fill: level 0..16 (0 = nothing, 16 = solid). */
void gfx_dither(int x, int y, int w, int h, int c, int level);
void gfx_dither_circle(int cx, int cy, int r, int c, int level);
/* Remap every pixel in a rect through a table (for shadows / tints). */
void gfx_remap_rect(int x, int y, int w, int h, const uint8_t *map);
void gfx_darken_rect(int x, int y, int w, int h, int steps);

/* Sprites */
void spr_make(Sprite *s, int w, int h, const char *data);
void spr_make_sub(Sprite *s, int w, int h, const char *data, int stride, int sx, int sy);
void spr_free(Sprite *s);
void spr_draw(const Sprite *s, int x, int y, int flags);
void spr_draw_ex(const Sprite *s, int x, int y, int flags, const uint8_t *remap, int solid);
void spr_draw_outline(const Sprite *s, int x, int y, int flags, int outline_col);
void spr_draw_scaled(const Sprite *s, int x, int y, int scale, int flags);

/* Palette remap helpers */
void pal_identity(uint8_t *map);
void pal_swap(uint8_t *map, int from, int to);

/* Presentation: fades and flashes are applied at conversion time. */
void gfx_set_fade(int level);     /* 0 = normal, 7 = black */
int gfx_get_fade(void);
void gfx_set_flash(int frames);   /* white flash for n frames */
void gfx_tick_effects(void);
void gfx_build_lut(uint32_t lut[PAL_COUNT], int argb); /* argb=1 ARGB8888 else ABGR */
void gfx_present(uint32_t *dst, int pitch_pixels, const uint32_t lut[PAL_COUNT]);

int pal_char_index(char ch); /* sprite-string character -> palette index */

#endif
