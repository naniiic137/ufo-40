/* WITHERLEA - art checks. Every picture in this cartridge is drawn with
 * shapes in wth_draw.c (the rooms, the Keeper, the 54 things), so there is
 * nothing to build at load time; the test below makes sure every thing has
 * a picture of its own. */
#include "wth.h"

void wth_art_load(void) {}

/* every kind of thing draws some pixels, and no two draw the same picture */
int wth_art_bad(void) {
    static uint8_t pic[I_COUNT][12 * 12];
    int bad = 0;
    for (int it = 1; it < I_COUNT; it++) {
        gfx_rect(0, 0, 12, 12, C_NAVY);
        wth_draw_item(it, 0, 0);
        int n = 0;
        for (int y = 0; y < 12; y++)
            for (int x = 0; x < 12; x++) {
                int c = gfx_pget(x, y);
                pic[it][y * 12 + x] = (uint8_t)c;
                n += c != C_NAVY;
            }
        bad += n < 6;
        for (int j = 1; j < it; j++) bad += !memcmp(pic[it], pic[j], sizeof pic[it]);
    }
    return bad;
}
