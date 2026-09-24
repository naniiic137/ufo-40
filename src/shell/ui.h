/* UFO 40 - shared UI helpers used by the shell and by games. */
#ifndef UFO_UI_H
#define UFO_UI_H

#include "../engine/engine.h"

/* Gradient-filled, outlined, shadowed text (logos and titles).
 * grad: array of n colours from top to bottom of the glyphs. */
void ui_fancy_text(const char *s, int x, int y, int scale, const uint8_t *grad, int n,
                   int outline, int shadow);
int ui_fancy_width(const char *s, int scale);
void ui_fancy_center(const char *s, int cx, int y, int scale, const uint8_t *grad, int n,
                     int outline, int shadow);

/* The UFO 40 logo. */
void ui_logo(int cx, int y, int scale, int t);

/* Rounded panel with border. */
void ui_panel(int x, int y, int w, int h, int fill, int border);
/* Starfield backdrop (animated). */
void ui_starfield(int t, int bg);
/* Goal icons: 9x9. earned=false draws the dim silhouette. */
void ui_goal_icon(int x, int y, int goal_bit, bool earned, int t);
/* A small flying saucer sprite (the console mascot). */
void ui_saucer(int x, int y, int t, int scale);
/* Menu cursor arrow blinking. */
void ui_cursor(int x, int y, int t);
/* Draws "A" / "B" button glyph + label, returns x after. */
int ui_hint(int x, int y, const char *glyph, const char *label, int col);

void ui_init(void);

#endif
