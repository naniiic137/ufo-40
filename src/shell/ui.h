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
/* Two choices side by side (NO / YES), centred as a pair on cx: each is a
 * cursor's room and its label, and the cursor sits at the chosen one (sel 0
 * or 1). The chosen label is drawn in on, the other in off. Reports the
 * labels, the cursor and the pair's centring to the layout audit. */
void ui_choices(int cx, int y, const char *a, const char *b, int sel, int t, int on, int off);

void ui_init(void);

/* Word-wrap s to lines no wider than w, in the main font (tiny = false) or
 * the tiny one. Fills up to max lines (each at most UI_WRAP_LEN - 1 chars)
 * and returns how many lines the whole text needs, which can be more than
 * max. If it is, the last line kept ends in "..." within w. A single word
 * wider than w still gets a line of its own (the layout audit flags it). */
#define UI_WRAP_LEN 96
int ui_wrap(const char *s, int w, bool tiny, char lines[][UI_WRAP_LEN], int max);

/* Layout audit, for the headless tests. A screen's draw code reports each
 * text element's box as it draws it; while an audit is open, every box must
 * lie inside the current area and must not overlap another box of that
 * area, and every character must have a glyph in its font. Outside an audit
 * these calls do nothing. log = print each problem to stderr. */
void ui_audit_begin(const char *subject, bool log);
bool ui_audit_on(void);
void ui_audit_area(const char *name, int x, int y, int w, int h);
void ui_audit_box(const char *what, int x, int y, int w, int h);
/* Text drawn with text_draw (or wrapped lines of it) / tiny_draw at x,y. */
void ui_audit_text(const char *what, const char *s, int x, int y);
void ui_audit_tiny(const char *what, const char *s, int x, int y);
/* ui_fancy_text(s, x, y, scale, ...) with or without a shadow. */
void ui_audit_fancy(const char *what, const char *s, int x, int y, int scale, bool shadow);
/* A row of width w starting at x must sit in the middle of the current area:
 * its left and right margins equal, give or take a pixel. */
void ui_audit_centred(const char *what, int x, int w);
/* A problem found by the screen itself (say, a blurb with too many lines). */
void ui_audit_fail(const char *what, const char *why);
int ui_audit_end(void); /* closes the audit, returns the problems found */

#endif
