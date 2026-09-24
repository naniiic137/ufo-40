/* UFO 40 - tiny image writers (indexed PNG and animated GIF), no dependencies. */
#ifndef UFO_IMGWRITE_H
#define UFO_IMGWRITE_H

#include <stdint.h>
#include <stdio.h>

/* Write an 8-bit palette PNG. pixels are palette indices (w*h). */
int png_write_indexed(const char *path, int w, int h, const uint8_t *pixels, const uint8_t (*pal)[3],
                      int ncolors, int scale);

typedef struct GifWriter {
    FILE *f;
    int w, h, scale;
    uint8_t *buf;
} GifWriter;

int gif_begin(GifWriter *g, const char *path, int w, int h, int scale, const uint8_t (*pal)[3]);
int gif_frame(GifWriter *g, const uint8_t *pixels, int delay_cs);
void gif_end(GifWriter *g);

#endif
