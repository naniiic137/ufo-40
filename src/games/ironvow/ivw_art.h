/* IRONVOW - the pictures shared by the art, the view and the drawing. */
#ifndef IVW_ART_H
#define IVW_ART_H

#include "ivw.h"

enum { TEX_GAOL, TEX_WARREN, TEX_STAR, TEX_DOOR, TEX_LOCKED, TEX_GATE, TEX_EXIT, TEX_COUNT };
enum {
    DEC_TORCH_LIT0, DEC_TORCH_LIT1, DEC_TORCH_OUT, DEC_FUNGUS_LIT, DEC_FUNGUS_OUT, DEC_LANTERN_ON, DEC_LANTERN_OFF,
    DEC_LEVER_UP, DEC_LEVER_DOWN, DEC_SWITCH, DEC_GEMS, DEC_GEMS_DONE, DEC_HIDDEN, DEC_COUNT
};
extern Sprite ivw_tex[TEX_COUNT];
extern Sprite ivw_decal[DEC_COUNT];
extern Sprite ivw_glyph_decal[SP_COUNT];
extern Sprite ivw_foe_spr[FK_COUNT][2];

void ivw_glyph_bounds(const char *g, int *minx, int *miny, int *maxx, int *maxy);

#endif
