/* placeholder - replaced by the real game */
#include "../../shell/gamedef.h"
static void load(void) {}
static void start(void) {}
static void update(void) { if (btnp(BTN_B)) game_exit_to_library(); }
static void draw(void) { gfx_cls(C_NIGHT); text_center("ROOFCAT", 160, 80, C_WHITE); }
static void label(int x, int y, int w, int h, int t) { (void)t; gfx_rect(x, y, w, h, C_DUSK); text_center("ROOFCAT", x + w / 2, y + h / 2 - 3, C_YELLOW); }
const GameDef GAME_ROOFCAT = {"roofcat", "ROOFCAT", "1983", "GENRE", "A PLACEHOLDER BLURB FOR THIS CARTRIDGE.", {"GOAL ONE", "GOAL TWO", "GOAL THREE"}, "D-PAD  MOVE", C_BLUE, C_RED, load, start, update, draw, NULL, label, NULL, NULL};
