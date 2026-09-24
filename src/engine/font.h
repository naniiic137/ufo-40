/* UFO 40 - bitmap fonts (original 5x7 proportional font + 3x5 tiny font). */
#ifndef UFO_FONT_H
#define UFO_FONT_H

/* Icon glyphs usable inside strings */
#define GLYPH_A      "\x80"
#define GLYPH_B      "\x81"
#define GLYPH_UP     "\x82"
#define GLYPH_DOWN   "\x83"
#define GLYPH_LEFT   "\x84"
#define GLYPH_RIGHT  "\x85"
#define GLYPH_HEART  "\x86"
#define GLYPH_COIN   "\x87"
#define GLYPH_STAR   "\x88"
#define GLYPH_DPAD   "\x89"
#define GLYPH_DOT    "\x8a"
#define GLYPH_LOCK   "\x8b"
#define GLYPH_SKULL  "\x8c"
#define GLYPH_NOTE   "\x8d"
#define GLYPH_CHECK  "\x8e"
#define GLYPH_CROSS  "\x8f"

#define FONT_H 7
#define LINE_H 9

void font_init(void);
int text_width(const char *s);             /* width of the widest line */
int text_draw(const char *s, int x, int y, int col);
void text_shadow(const char *s, int x, int y, int col, int shadow);
void text_outline(const char *s, int x, int y, int col, int outline);
void text_center(const char *s, int cx, int y, int col);
void text_center_shadow(const char *s, int cx, int y, int col, int shadow);
int text_draw_scaled(const char *s, int x, int y, int col, int scale);
int text_width_scaled(const char *s, int scale);
/* Word-wrap into a box; returns number of lines. Draws only if col >= 0. */
int text_wrap(const char *s, int x, int y, int w, int col, int line_h);

/* Rasterise text as 1s into a byte buffer (no clipping state involved). */
void text_render_mask(const char *s, int scale, unsigned char *buf, int bw, int bh, int ox, int oy);

int tiny_width(const char *s);
int tiny_draw(const char *s, int x, int y, int col);
void tiny_center(const char *s, int cx, int y, int col);

#endif
