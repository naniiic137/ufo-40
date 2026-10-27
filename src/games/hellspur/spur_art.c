/* HELLSPUR - all the pictures, drawn for UFO 40. The ground, walls and
 * buildings are painted by code into 16 x 16 tiles when the cartridge
 * loads. The people are small templates (a body, a hat, colours per person).
 * The monsters are painted by code too, one routine per kind; their colour
 * sets swap the browns for reds, purples, blacks or blues, the way an old
 * cartridge stretched one drawing across a whole bestiary. */
#include "spur.h"

static Sprite tiles[T_COUNT][2];
static Sprite chest_open;
static Sprite foes[A_ZENITH + 1];
static Sprite people[LOOK_COUNT + H_COUNT][3][2]; /* look, view (front, back, side), frame */
static Sprite animals[SPUR_ANIMALS];
static Sprite horse[2], nightmare[2], coffin, wisp[3];
static uint8_t remaps[5][PAL_COUNT];

/* ---- painting into a sprite ----------------------------------------------------------- */
static Surface canvas;
static uint8_t canvas_px[64 * 64];

static void begin(int w, int h) {
    canvas.w = w;
    canvas.h = h;
    canvas.px = canvas_px;
    gfx_set_target(&canvas);
    gfx_cls(TRANSPARENT);
}

static void end(Sprite *s) {
    s->w = (int16_t)canvas.w;
    s->h = (int16_t)canvas.h;
    s->px = malloc((size_t)canvas.w * canvas.h);
    if (s->px) memcpy(s->px, canvas.px, (size_t)canvas.w * canvas.h);
    gfx_set_target(NULL);
}

static uint32_t h3(int a, int b, int c) {
    uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u + (uint32_t)c * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static void speckle(int c, int n, int seed) {
    for (int i = 0; i < n; i++) {
        uint32_t r = h3(i, seed, 7);
        gfx_pset((int)(r % 16), (int)((r >> 8) % 16), c);
    }
}

/* ---- tiles --------------------------------------------------------------------------- */
static void grass(int v) { gfx_cls(C_LEAF); speckle(C_FOREST, 10, 1 + v); speckle(C_LIME, 4, 3 + v); }
static void sand(int v) { gfx_cls(C_TAN); speckle(C_HIDE, 9, 11 + v); speckle(C_CREAM, 5, 13 + v); }
static void dirt(int v) { gfx_cls(C_HIDE); speckle(C_EARTH, 10, 21 + v); speckle(C_TAN, 4, 23 + v); }
static void ash(int v) { gfx_cls(C_SLATE); speckle(C_DUSK, 10, 31 + v); speckle(C_GREY, 4, 33 + v); }
static void stone_floor(int base, int line) {
    gfx_cls(base);
    gfx_hline(0, 15, 7, line);
    gfx_hline(0, 15, 15, line);
    gfx_vline(5, 0, 7, line);
    gfx_vline(12, 8, 15, line);
}
static void bricks(int base, int mortar) {
    gfx_cls(base);
    for (int y = 3; y < 16; y += 4) gfx_hline(0, 15, y, mortar);
    for (int r = 0; r < 4; r++) for (int x = (r % 2) * 4; x < 16; x += 8) gfx_vline(x, r * 4, r * 4 + 2, mortar);
}
static void rockwall(int base, int hi, int lo, int v) {
    gfx_cls(base);
    for (int i = 0; i < 5; i++) {
        uint32_t r = h3(i, v, 41);
        int x = (int)(r % 14), y = (int)((r >> 6) % 14);
        gfx_circ(x + 1, y + 1, 2, hi);
        gfx_pset(x + 2, y + 2, lo);
    }
    gfx_hline(0, 15, 15, lo);
}
static void tree(int x, int y, int crown, int dark, int trunk) {
    gfx_rect(x + 7, y + 10, 2, 5, trunk);
    gfx_circ(x + 8, y + 7, 6, crown);
    gfx_circ(x + 9, y + 9, 4, dark);
    gfx_circ(x + 7, y + 5, 3, PAL_LIGHTER[crown]);
}
static void cactus(int x, int y) {
    gfx_rect(x + 7, y + 3, 3, 12, C_JADE);
    gfx_rect(x + 3, y + 7, 2, 4, C_JADE);
    gfx_rect(x + 3, y + 10, 4, 2, C_JADE);
    gfx_rect(x + 12, y + 5, 2, 4, C_JADE);
    gfx_rect(x + 10, y + 8, 3, 2, C_JADE);
    gfx_vline(x + 8, y + 4, y + 14, C_LEAF);
}
static void flames(int v, int base) {
    for (int i = 0; i < 6; i++) {
        uint32_t r = h3(i, v, 51);
        int x = (int)(r % 14) + 1, hh = 5 + (int)((r >> 5) % 8);
        gfx_line(x, 15, x + (v ? 1 : -1), 15 - hh, base);
        gfx_line(x + 1, 15, x + 1, 16 - hh / 2, C_YELLOW);
    }
}
static void building(int wall, int roof) {
    gfx_rect(2, 6, 12, 9, wall);
    gfx_line(1, 7, 8, 1, roof);
    gfx_line(8, 1, 15, 7, roof);
    for (int y = 2; y < 7; y++) gfx_hline(9 - y, 7 + y, y + 1, roof);
    gfx_rect(7, 10, 3, 5, C_INK);
}

static void paint_tile(int t, int v) {
    switch (t) {
    case T_SEA:
        gfx_cls(C_NAVY);
        for (int i = 0; i < 3; i++) gfx_hline((i * 5 + v * 3) % 12, (i * 5 + v * 3) % 12 + 3, 3 + i * 5, C_BLUE);
        break;
    case T_RIVER: case T_WATER:
        gfx_cls(C_BLUE);
        for (int i = 0; i < 3; i++) gfx_hline((i * 6 + v * 4) % 12, (i * 6 + v * 4) % 12 + 2, 2 + i * 5, C_SKY);
        break;
    case T_GRASS: grass(v); break;
    case T_PRAIRIE:
        grass(v);
        for (int i = 0; i < 6; i++) { uint32_t r = h3(i, v, 61); int x = (int)(r % 15), y = 4 + (int)((r >> 5) % 11); gfx_vline(x, y - 3, y, C_LIME); }
        break;
    case T_SAND: sand(v); break;
    case T_DIRT: dirt(v); break;
    case T_SCRUB: sand(v); gfx_circ(5, 9, 3, C_FOREST); gfx_circ(11, 5, 2, C_JADE); gfx_pset(5, 8, C_LEAF); break;
    case T_FOREST: grass(v); tree(0, 0, C_JADE, C_FOREST, C_BROWN); break;
    case T_PINE:
        grass(v);
        gfx_rect(7, 12, 2, 3, C_BROWN);
        for (int y = 1; y < 12; y++) gfx_hline(8 - y / 2, 8 + y / 2, y, y % 3 ? C_FOREST : C_JADE);
        break;
    case T_CACTUS: sand(v); cactus(0, 0); break;
    case T_HILL: grass(v); gfx_circ(8, 12, 7, C_FOREST); gfx_circ(7, 11, 5, C_JADE); gfx_rect(0, 13, 16, 3, C_LEAF); break;
    case T_MOUNT:
        gfx_cls(C_HIDE);
        for (int y = 1; y < 16; y++) gfx_hline(8 - y / 2, 8 + y / 2, y, C_EARTH);
        for (int y = 1; y < 16; y++) gfx_hline(8 - y / 2, 8, y, C_BROWN);
        for (int y = 1; y < 5; y++) gfx_hline(8 - y / 2, 8 + y / 2, y, C_CREAM);
        if (v) { gfx_rect(0, 12, 4, 4, C_BROWN); gfx_rect(12, 11, 4, 5, C_EARTH); }
        break;
    case T_PEAK: gfx_cls(C_DUSK); for (int y = 0; y < 16; y++) gfx_hline(8 - y / 2, 8 + y / 2, y, C_SLATE); break;
    case T_SWAMP:
        gfx_cls(C_FOREST);
        speckle(C_EARTH, 8, 71 + v);
        gfx_rect(3 + v * 5, 6, 6, 3, C_TEAL);
        gfx_vline(12 - v * 8, 2, 7, C_LIME);
        break;
    case T_ROAD: gfx_cls(C_TAN); speckle(C_HIDE, 8, 81 + v); gfx_hline(0, 15, 5, C_HIDE); gfx_hline(0, 15, 11, C_HIDE); break;
    case T_BRIDGE:
        gfx_cls(C_BLUE);
        gfx_rect(2, 0, 12, 16, C_BROWN);
        for (int y = 1; y < 16; y += 3) gfx_hline(2, 13, y, C_EARTH);
        gfx_vline(2, 0, 15, C_EARTH);
        gfx_vline(13, 0, 15, C_EARTH);
        break;
    case T_FERRY:
        gfx_cls(C_BLUE);
        gfx_rect(1, 4, 14, 8, C_BROWN);
        for (int x = 2; x < 15; x += 3) gfx_vline(x, 4, 11, C_EARTH);
        gfx_hline(0, 15, 2, C_GREY);
        break;
    case T_LAVA:
        gfx_cls(C_RED);
        for (int i = 0; i < 4; i++) { uint32_t r = h3(i, v, 91); gfx_circ((int)(r % 16), (int)((r >> 5) % 16), 2, C_ORANGE); }
        speckle(C_YELLOW, 4, 93 + v);
        break;
    case T_ASH: ash(v); break;
    case T_BONES: ash(v); gfx_line(3, 10, 8, 8, C_CREAM); gfx_pset(3, 9, C_WHITE); gfx_pset(8, 7, C_WHITE); gfx_circ(12, 4, 2, C_CREAM); gfx_pset(11, 4, C_INK); break;
    case T_CLOUD: gfx_cls(C_SKY); gfx_circ(5, 9, 5, C_WHITE); gfx_circ(11, 8, 5, C_WHITE); gfx_circ(8, 5, 4, C_ICE); break;
    case T_MESA:
        gfx_cls(C_ORANGE);
        for (int y = 0; y < 16; y += 4) gfx_hline(0, 15, y, C_RED);
        gfx_rect(0, 13, 16, 3, C_MAROON);
        speckle(C_AMBER, 6, 101 + v);
        break;
    case T_SALT: gfx_cls(C_LIGHT); speckle(C_WHITE, 12, 111 + v); break;
    case T_DEADTREE:
        dirt(v);
        gfx_line(8, 15, 8, 4, C_INK);
        gfx_line(8, 8, 3, 4, C_INK);
        gfx_line(8, 6, 13, 2, C_INK);
        gfx_line(5, 6, 5, 3, C_INK);
        break;
    case T_FLOWERS: grass(v); gfx_pset(3, 4, C_YELLOW); gfx_pset(10, 7, C_PINK); gfx_pset(6, 12, C_WHITE); gfx_pset(13, 12, C_YELLOW); break;
    case T_TOWN:
        grass(v);
        building(C_BROWN, C_WINE);
        gfx_rect(11, 9, 4, 6, C_TAN);
        gfx_rect(12, 11, 2, 4, C_INK);
        break;
    case T_FORT:
        dirt(v);
        for (int x = 1; x < 16; x += 3) { gfx_rect(x, 4, 2, 11, C_BROWN); gfx_pset(x, 3, C_EARTH); }
        gfx_rect(6, 9, 4, 6, C_INK);
        break;
    case T_MISSION:
        grass(v);
        gfx_rect(2, 7, 12, 8, C_CREAM);
        gfx_rect(6, 1, 4, 7, C_CREAM);
        gfx_pset(7, 0, C_BROWN); gfx_pset(8, 0, C_BROWN);
        gfx_rect(7, 3, 2, 2, C_INK);
        gfx_rect(7, 10, 2, 5, C_BROWN);
        break;
    case T_HOUSE: grass(v); building(C_HIDE, C_EARTH); break;
    case T_CAVE:
        gfx_cls(C_EARTH);
        gfx_circ(8, 10, 6, C_BROWN);
        gfx_circ(8, 11, 4, C_INK);
        gfx_rect(4, 11, 9, 5, C_INK);
        break;
    case T_MINE:
        gfx_cls(C_HIDE);
        gfx_rect(3, 5, 10, 11, C_INK);
        gfx_rect(2, 4, 12, 2, C_BROWN);
        gfx_rect(2, 4, 2, 12, C_BROWN);
        gfx_rect(12, 4, 2, 12, C_BROWN);
        gfx_hline(4, 11, 14, C_GREY);
        break;
    case T_TOWER:
        ash(v);
        gfx_rect(4, 1, 8, 15, C_LIGHT);
        for (int y = 2; y < 16; y += 3) gfx_hline(4, 11, y, C_GREY);
        gfx_rect(6, 10, 4, 6, C_INK);
        gfx_rect(3, 0, 10, 2, C_CREAM);
        break;
    case T_GATE:
        ash(v);
        gfx_rect(3, 0, 10, 16, C_INK);
        gfx_rect(5, 2, 6, 14, spur_flag(F_GATE_OPEN) ? C_RED : C_NIGHT);
        gfx_pset(4, 1, C_PURPLE);
        break;
    case T_SPIKE:
        gfx_cls(TRANSPARENT);
        gfx_rect(6, 2, 4, 14, C_GREY);
        gfx_rect(3, 0, 10, 3, C_LIGHT);
        gfx_vline(7, 3, 15, C_LIGHT);
        gfx_line(6, 15, 9, 15, C_SLATE);
        break;
    case T_SPIKE_GONE: sand(v); gfx_circ(8, 8, 3, C_INK); gfx_circ(8, 8, 1, C_EARTH); break;
    case T_CITADEL:
        gfx_cls(C_SLATE);
        gfx_rect(2, 4, 12, 12, C_INK);
        gfx_rect(6, 0, 4, 6, C_INK);
        gfx_rect(1, 2, 3, 4, C_INK);
        gfx_rect(12, 2, 3, 4, C_INK);
        gfx_rect(7, 10, 2, 6, C_RED);
        gfx_pset(7, 2, C_RED);
        break;
    case T_SHRINE: sand(v); gfx_rect(3, 7, 10, 8, C_HIDE); gfx_line(2, 7, 8, 2, C_EARTH); gfx_line(8, 2, 14, 7, C_EARTH); gfx_rect(7, 11, 2, 4, C_INK); gfx_line(13, 15, 15, 9, C_JADE); break;
    case T_SALOONSKY: gfx_cls(C_SKY); gfx_circ(8, 13, 7, C_WHITE); building(C_BROWN, C_RED); break;
    case T_SMITHY: ash(v); building(C_SLATE, C_INK); gfx_rect(11, 3, 2, 4, C_GREY); gfx_pset(11, 2, C_ORANGE); break;
    case T_RUIN: dirt(v); gfx_rect(2, 6, 3, 9, C_GREY); gfx_rect(10, 9, 4, 6, C_GREY); break;
    case T_ANGELS:
        grass(v);
        for (int k = 0; k < 2; k++) {
            int x = 2 + k * 7;
            gfx_rect(x + 1, 5, 3, 8, C_WHITE);
            gfx_circ(x + 2, 3, 2, C_CREAM);
            gfx_line(x - 1, 6, x + 1, 4, C_ICE);
            gfx_line(x + 5, 6, x + 3, 4, C_ICE);
            gfx_hline(x, x + 4, 0, C_YELLOW);
        }
        break;
    case T_RUBBLE: dirt(v); gfx_circ(5, 10, 4, C_GREY); gfx_circ(11, 9, 4, C_SLATE); gfx_circ(8, 5, 3, C_GREY); gfx_line(1, 15, 6, 12, C_RED); break;
    case T_FIREWALL: gfx_cls(C_MAROON); flames(v, C_ORANGE); break;
    case T_BARRIER: gfx_cls(C_INK); gfx_dither(0, 0, 16, 16, C_PURPLE, 4 + v * 2); break;
    case T_LOCKED: gfx_cls(C_BROWN); gfx_rect(2, 0, 12, 16, C_EARTH); gfx_rect(7, 7, 3, 4, C_YELLOW); gfx_pset(8, 9, C_INK); break;
    case T_FLOOR:
        gfx_cls(C_BROWN);
        for (int y = 3; y < 16; y += 4) gfx_hline(0, 15, y, C_EARTH);
        gfx_vline(v ? 5 : 10, 0, 3, C_EARTH);
        gfx_vline(v ? 12 : 3, 8, 11, C_EARTH);
        break;
    case T_STONE: stone_floor(C_LIGHT, C_GREY); break;
    case T_WALL: bricks(C_GREY, C_SLATE); break;
    case T_ROCK: rockwall(C_EARTH, C_BROWN, C_INK, v); break;
    case T_ROOF:
        gfx_cls(C_WINE);
        for (int y = 3; y < 16; y += 4) gfx_hline(0, 15, y, C_MAROON);
        for (int r = 0; r < 4; r++) for (int x = (r % 2) * 3; x < 16; x += 6) gfx_vline(x, r * 4, r * 4 + 2, C_MAROON);
        break;
    case T_BWALL:
        gfx_cls(C_HIDE);
        for (int x = 0; x < 16; x += 4) gfx_vline(x, 0, 15, C_BROWN);
        gfx_hline(0, 15, 15, C_EARTH);
        break;
    case T_DOOR:
        gfx_cls(C_HIDE);
        gfx_rect(3, 2, 10, 14, C_EARTH);
        gfx_rect(4, 3, 8, 13, C_INK);
        gfx_rect(2, 0, 12, 2, C_BROWN);
        break;
    case T_WINDOW:
        paint_tile(T_BWALL, v);
        gfx_set_target(&canvas);
        gfx_rect(4, 4, 8, 7, C_BROWN);
        gfx_rect(5, 5, 6, 5, C_AMBER);
        gfx_vline(8, 5, 9, C_BROWN);
        break;
    case T_COUNTER: gfx_cls(C_BROWN); gfx_rect(0, 0, 16, 5, C_TAN); gfx_hline(0, 15, 5, C_EARTH); break;
    case T_FENCE: grass(v); gfx_rect(0, 6, 16, 2, C_BROWN); gfx_rect(0, 11, 16, 2, C_BROWN); gfx_rect(2, 4, 2, 11, C_EARTH); gfx_rect(12, 4, 2, 11, C_EARTH); break;
    case T_WELL: grass(v); gfx_circ(8, 9, 6, C_GREY); gfx_circ(8, 9, 4, C_INK); gfx_rect(2, 1, 12, 2, C_BROWN); gfx_vline(3, 1, 8, C_BROWN); gfx_vline(12, 1, 8, C_BROWN); break;
    case T_BARREL: dirt(v); gfx_rect(4, 2, 8, 13, C_BROWN); gfx_hline(4, 11, 5, C_EARTH); gfx_hline(4, 11, 11, C_EARTH); gfx_rect(5, 2, 6, 1, C_TAN); break;
    case T_CRATE: dirt(v); gfx_rect(2, 3, 12, 12, C_TAN); gfx_rectb(2, 3, 12, 12, C_BROWN); gfx_line(2, 3, 13, 14, C_BROWN); break;
    case T_UP: stone_floor(C_GREY, C_SLATE); for (int y = 2; y < 16; y += 3) gfx_hline(2, 13, y, C_LIGHT); gfx_hline(2, 13, 1, C_WHITE); break;
    case T_DOWN: gfx_cls(C_INK); for (int y = 2; y < 16; y += 3) gfx_hline(2 + y / 3, 13 - y / 3, y, C_SLATE); break;
    case T_EXIT: gfx_cls(C_INK); gfx_rect(3, 0, 10, 16, C_NIGHT); gfx_dither(3, 0, 10, 16, C_DUSK, 6); break;
    case T_CHEST: dirt(v); gfx_rect(2, 5, 12, 9, C_BROWN); gfx_rect(2, 5, 12, 3, C_EARTH); gfx_rect(7, 7, 2, 3, C_YELLOW); gfx_rectb(2, 5, 12, 9, C_INK); break;
    case T_BED: gfx_cls(C_BROWN); gfx_rect(1, 1, 14, 14, C_EARTH); gfx_rect(2, 2, 12, 4, C_WHITE); gfx_rect(2, 6, 12, 8, C_RED); break;
    case T_TABLE: paint_tile(T_FLOOR, v); gfx_set_target(&canvas); gfx_circ(8, 8, 6, C_TAN); gfx_circ(8, 8, 4, C_HIDE); gfx_pset(7, 6, C_WHITE); break;
    case T_RAIL: dirt(v); gfx_vline(4, 0, 15, C_GREY); gfx_vline(11, 0, 15, C_GREY); for (int y = 1; y < 16; y += 4) gfx_hline(2, 13, y, C_BROWN); break;
    case T_ORE: rockwall(C_EARTH, C_BROWN, C_INK, v); gfx_pset(6, 6, C_WHITE); gfx_pset(9, 9, v ? C_YELLOW : C_CYAN); gfx_pset(4, 11, C_YELLOW); gfx_pset(10, 4, C_WHITE); break;
    case T_HERB: grass(v); gfx_circ(5, 9, 3, C_JADE); gfx_circ(11, 10, 3, C_FOREST); gfx_pset(5, 7, C_WHITE); gfx_pset(11, 8, v ? C_YELLOW : C_WHITE); break;
    case T_BARS: stone_floor(C_LIGHT, C_GREY); gfx_rect(0, 0, 16, 2, C_SLATE); for (int x = 1; x < 16; x += 3) gfx_vline(x, 0, 15, C_SLATE); break;
    case T_ALTAR: stone_floor(C_LIGHT, C_GREY); gfx_rect(1, 4, 14, 10, C_WHITE); gfx_rect(1, 4, 14, 2, C_YELLOW); gfx_vline(8, 0, 3, C_YELLOW); break;
    case T_TORCH: bricks(C_GREY, C_SLATE); gfx_rect(7, 7, 2, 6, C_BROWN); gfx_circ(8, 5, 2, v ? C_YELLOW : C_ORANGE); break;
    case T_GRAVE: grass(v); gfx_rect(5, 3, 6, 11, C_GREY); gfx_circ(8, 4, 3, C_GREY); gfx_hline(6, 9, 6, C_SLATE); gfx_vline(8, 4, 9, C_SLATE); break;
    case T_BURNING: gfx_cls(C_BROWN); flames(v, C_RED); break;
    case T_PEW: gfx_cls(C_LIGHT); gfx_rect(0, 4, 16, 6, C_BROWN); gfx_rect(0, 3, 16, 2, C_EARTH); break;
    case T_RUG: gfx_cls(C_RED); gfx_rectb(1, 0, 14, 16, C_AMBER); gfx_pset(8, 8, C_YELLOW); break;
    case T_BONEWALL: gfx_cls(C_CREAM); for (int i = 0; i < 5; i++) { uint32_t r = h3(i, v, 121); gfx_circ((int)(r % 14) + 1, (int)((r >> 4) % 14) + 1, 2, C_LIGHT); gfx_pset((int)(r % 14) + 1, (int)((r >> 4) % 14) + 1, C_INK); } gfx_hline(0, 15, 15, C_TAN); break;
    case T_DREAMFLOOR: gfx_cls(C_PURPLE); speckle(C_VIOLET, 8, 131 + v); speckle(C_PINK, 2, 133 + v); break;
    case T_HELLFLOOR: stone_floor(C_MAROON, C_INK); speckle(C_WINE, 5, 141 + v); break;
    case T_HELLWALL: gfx_cls(C_INK); gfx_line(2, 15, 6, 2, C_NIGHT); gfx_line(9, 15, 13, 3, C_NIGHT); gfx_pset(6, 2, C_RED); break;
    case T_GLASS: gfx_cls(C_VIOLET); gfx_line(0, 15, 15, 0, C_PINK); gfx_line(4, 15, 15, 4, C_MAGENTA); gfx_rect(2, 2, 3, 3, C_WHITE); break;
    case T_SIGN: grass(v); gfx_rect(7, 7, 2, 8, C_BROWN); gfx_rect(2, 2, 12, 6, C_TAN); gfx_hline(4, 11, 4, C_EARTH); break;
    case T_BOOKS: gfx_cls(C_BROWN); for (int y = 1; y < 16; y += 5) for (int x = 1; x < 15; x += 2) gfx_rect(x, y, 1, 4, (x * 3 + y) % 4 == 0 ? C_RED : (x + y) % 3 ? C_NAVY : C_FOREST); break;
    case T_PIANO: gfx_cls(C_INK); gfx_rect(0, 8, 16, 4, C_WHITE); for (int x = 1; x < 16; x += 2) gfx_vline(x, 8, 10, C_INK); gfx_rect(0, 0, 16, 7, C_EARTH); break;
    case T_TRACKS: paint_tile(T_RAIL, v); break;
    case T_STOOL: paint_tile(T_FLOOR, v); gfx_set_target(&canvas); gfx_circ(8, 8, 3, C_TAN); break;
    case T_TREE: grass(v); tree(0, 0, C_JADE, C_FOREST, C_BROWN); break;
    case T_HIVE: grass(v); tree(0, 0, C_JADE, C_FOREST, C_BROWN); gfx_circ(11, 10, 3, C_AMBER); gfx_hline(9, 13, 10, C_BROWN); break;
    case T_WINCH: stone_floor(C_MAROON, C_INK); gfx_circ(8, 8, 6, C_GREY); gfx_circ(8, 8, 3, C_SLATE); gfx_line(8, 8, 14, 2, C_LIGHT); break;
    case T_LIFT: gfx_cls(C_INK); gfx_rect(1, 1, 14, 14, C_GREY); gfx_rectb(1, 1, 14, 14, C_YELLOW); gfx_vline(8, 3, 12, C_YELLOW); break;
    case T_PORTAL: gfx_cls(C_NIGHT); gfx_circ(8, 8, 7, C_PURPLE); gfx_circ(8, 8, 5, v ? C_MAGENTA : C_VIOLET); gfx_circ(8, 8, 2, C_WHITE); break;
    case T_CAULDRON: stone_floor(C_GREY, C_SLATE); gfx_rect(2, 4, 12, 10, C_INK); gfx_rect(4, 6, 8, 5, v ? C_ORANGE : C_RED); break;
    case T_SECRET: paint_tile(T_COUNTER, v); break;
    default: gfx_cls(C_MAGENTA); break;
    }
}

/* ---- people ------------------------------------------------------------------------- */
/* body templates: H hat, R hair, K skin, S shirt, P trousers or skirt, B boots, A accent */
static const char *const BODY_FRONT[2][16] = {
    {"................", "......RRRR......", ".....RRRRRR.....", ".....RKKKKR.....", ".....KkKKkK.....", ".....KKKKKK.....",
     "......KKKK......", "....SSSSSSSS....", "...SSSSAASSSS...", "...KSSSAASSSK...", "...KSSSSSSSSK...", "....PPPPPPPP....",
     "....PPP..PPP....", "....PPP..PPP....", "....BBB..BBB....", "...BBBB..BBBB..."},
    {"................", "......RRRR......", ".....RRRRRR.....", ".....RKKKKR.....", ".....KkKKkK.....", ".....KKKKKK.....",
     "......KKKK......", "....SSSSSSSS....", "...SSSSAASSSS...", "...KSSSAASSSK...", "...KSSSSSSSSK...", "....PPPPPPPP....",
     "....PPP..PPP....", "...PPP....PPP...", "...BBB....BBB...", "..BBB......BBB.."},
};
static const char *const BODY_BACK[2][16] = {
    {"................", "......RRRR......", ".....RRRRRR.....", ".....RRRRRR.....", ".....RRRRRR.....", ".....RRRRRR.....",
     "......KKKK......", "....SSSSSSSS....", "...SSSSSSSSSS...", "...KSSSSSSSSK...", "...KSSSSSSSSK...", "....PPPPPPPP....",
     "....PPP..PPP....", "....PPP..PPP....", "....BBB..BBB....", "...BBBB..BBBB..."},
    {"................", "......RRRR......", ".....RRRRRR.....", ".....RRRRRR.....", ".....RRRRRR.....", ".....RRRRRR.....",
     "......KKKK......", "....SSSSSSSS....", "...SSSSSSSSSS...", "...KSSSSSSSSK...", "...KSSSSSSSSK...", "....PPPPPPPP....",
     "....PPP..PPP....", "...PPP....PPP...", "...BBB....BBB...", "..BBB......BBB.."},
};
static const char *const BODY_SIDE[2][16] = {
    {"................", ".....RRRR.......", "....RRRRRR......", "....RRKKKK......", "....RKKKkK......", "....RKKKKKK.....",
     ".....KKKK.......", ".....SSSSA......", "....SSSSSA......", "....SSKSSS......", "....SSKSSS......", ".....PPPP.......",
     ".....PP.PP......", "....PP...PP.....", "....BB...BB.....", "...BBB...BBB...."},
    {"................", ".....RRRR.......", "....RRRRRR......", "....RRKKKK......", "....RKKKkK......", "....RKKKKKK.....",
     ".....KKKK.......", ".....SSSSA......", "....SSSSSA......", "....SSSKSS......", "....SSSKSS......", ".....PPPP.......",
     ".....PPPP.......", ".....PPPP.......", ".....BBB........", ".....BBBB......."},
};
/* a skirt over the legs */
static const char *const SKIRT_FRONT[5] = {"...PPPPPPPPPP...", "...PPPPPPPPPP...", "..PPPPPPPPPPPP..", "..PPPPPPPPPPPP..", "....BB....BB...."};
static const char *const SKIRT_SIDE[5] = {"....PPPPPP......", "...PPPPPPPP.....", "...PPPPPPPP.....", "...PPPPPPPP.....", "....BB..BB......"};

enum { HAT_NONE, HAT_COWBOY, HAT_TOP, HAT_BOWLER, HAT_BONNET, HAT_VEIL, HAT_HORNS, HAT_HALO, HAT_FEATHER, HAT_KEPI,
       HAT_MINER, HAT_WIDE, HAT_HOOD, HAT_PLUME };
/* rows 0..2 for front/back, and for the side view */
static const char *const HAT_ROWS[][2][3] = {
    [HAT_NONE] = {{NULL, NULL, NULL}, {NULL, NULL, NULL}},
    [HAT_COWBOY] = {{".....HHHHHH.....", "..HHHHHHHHHHHH..", ".....RKKKKR....."}, {"....HHHHHH......", "..HHHHHHHHHHH...", "....RRKKKK......"}},
    [HAT_TOP] = {{"......HHHH......", "......HHHH......", "....HHHHHHHH...."}, {".....HHHH.......", ".....HHHH.......", "...HHHHHHHH....."}},
    [HAT_BOWLER] = {{"................", "......HHHH......", "....HHHHHHHH...."}, {"................", ".....HHHH.......", "...HHHHHHHH....."}},
    [HAT_BONNET] = {{".....HHHHHH.....", "....HHHHHHHH....", "....HRKKKKRH...."}, {"....HHHHHH......", "...HHHHHHHH.....", "...HHRKKKK......"}},
    [HAT_VEIL] = {{".....HHHHHH.....", "....HHHHHHHH....", "....HHKKKKHH...."}, {"....HHHHHH......", "...HHHHHHHH.....", "...HHHKKKK......"}},
    [HAT_HORNS] = {{"..H..........H..", "..HH.RRRR..HH...", "....HRRRRRRH...."}, {"..H.............", "..HH.RRRR.......", "....RRRRRR......"}},
    [HAT_HALO] = {{".....HHHHHH.....", "......RRRR......", ".....RRRRRR....."}, {"....HHHHHH......", ".....RRRR.......", "....RRRRRR......"}},
    [HAT_FEATHER] = {{"..........HA....", "......RRRRH.....", ".....RRRRRR....."}, {".........HA.....", ".....RRRRH......", "....RRRRRR......"}},
    [HAT_KEPI] = {{"......HHHH......", ".....HHHHHH.....", "....HHHHHHHH...."}, {".....HHHH.......", "....HHHHHH......", "....HHHHHHHH...."}},
    [HAT_MINER] = {{"......HHHH......", ".....HHAAHH.....", "....HHHHHHHH...."}, {".....HHHH.......", "....HHHHHHA.....", "...HHHHHHHH....."}},
    [HAT_WIDE] = {{"....HHHHHHHH....", ".HHHHHHHHHHHHHH.", ".....RKKKKR....."}, {"...HHHHHHHH.....", ".HHHHHHHHHHHH...", "....RRKKKK......"}},
    [HAT_HOOD] = {{".....HHHHHH.....", "....HHHHHHHH....", "....HHKKKKHH...."}, {"....HHHHHH......", "...HHHHHHHH.....", "...HHHKKKKK....."}},
    [HAT_PLUME] = {{"...........A....", "......RRRRAA....", ".....RRRRRR....."}, {"..........A.....", ".....RRRRAA.....", "....RRRRRR......"}},
};

typedef struct { uint8_t hat, skirt; uint8_t H, R, K, S, P, B, A; } Outfit;

static Outfit outfit_for(int look, int pal) {
    static const uint8_t SHIRTS[] = {C_RED, C_BLUE, C_CREAM, C_JADE, C_AMBER, C_SKY, C_WINE, C_TAN, C_LIGHT, C_PURPLE, C_TEAL, C_ORANGE, C_FOREST, C_PINK, C_GREY};
    static const uint8_t PANTS[] = {C_NAVY, C_BROWN, C_SLATE, C_EARTH, C_INK, C_DUSK, C_HIDE};
    static const uint8_t HATS[] = {C_BROWN, C_INK, C_TAN, C_EARTH, C_GREY, C_CREAM, C_SLATE};
    static const uint8_t HAIRS[] = {C_BROWN, C_INK, C_AMBER, C_GREY, C_EARTH, C_ORANGE, C_LIGHT};
    static const uint8_t SKINS[] = {C_CREAM, C_TAN, C_HIDE, C_CREAM};
    Outfit o = {HAT_NONE, 0, HATS[pal % 7], HAIRS[pal % 7], SKINS[pal % 4], SHIRTS[pal % 15], PANTS[pal % 7], C_EARTH, SHIRTS[(pal + 5) % 15]};
    switch (look) {
    case LOOK_MAN: o.hat = pal % 3 ? HAT_COWBOY : HAT_BOWLER; break;
    case LOOK_WOMAN: o.skirt = 1; o.hat = pal % 2 ? HAT_BONNET : HAT_NONE; o.P = SHIRTS[(pal + 3) % 15]; break;
    case LOOK_KID: o.hat = HAT_NONE; break;
    case LOOK_OLD: o.hat = HAT_WIDE; o.R = C_WHITE; o.S = C_GREY; break;
    case LOOK_SHERIFF: o.hat = HAT_COWBOY; o.H = C_INK; o.S = C_TAN; o.A = C_YELLOW; break;
    case LOOK_PRIEST: o.hat = HAT_NONE; o.S = C_INK; o.P = C_INK; o.A = C_WHITE; o.skirt = 1; break;
    case LOOK_KEEPER: o.hat = HAT_BOWLER; o.S = C_WHITE; o.A = C_RED; break;
    case LOOK_DEMON: o.hat = HAT_HORNS; o.H = C_CREAM; o.K = C_RED; o.R = C_INK; o.S = SHIRTS[(pal * 2) % 15]; break;
    case LOOK_ANGEL: o.hat = HAT_HALO; o.H = C_YELLOW; o.S = C_WHITE; o.P = C_WHITE; o.skirt = 1; o.R = pal % 2 ? C_AMBER : C_WHITE; break;
    case LOOK_SOLDIER: o.hat = HAT_KEPI; o.H = C_NAVY; o.S = C_NAVY; o.P = C_SKY; o.A = C_YELLOW; break;
    case LOOK_OUTLAW: o.hat = HAT_WIDE; o.H = C_INK; o.S = SHIRTS[(pal + 6) % 15]; o.A = C_RED; break;
    case LOOK_NUN: o.hat = HAT_VEIL; o.H = C_INK; o.S = C_INK; o.P = C_INK; o.skirt = 1; o.A = C_WHITE; break;
    case LOOK_MINER: o.hat = HAT_MINER; o.H = C_AMBER; o.A = C_YELLOW; o.S = C_BROWN; break;
    case LOOK_BARKEEP: o.hat = HAT_NONE; o.S = C_WHITE; o.A = C_INK; o.R = C_INK; break;
    case LOOK_GHOST: o.hat = HAT_HOOD; o.H = C_ICE; o.S = C_ICE; o.P = C_ICE; o.K = C_ICE; o.skirt = 1; break;
    default: break;
    }
    return o;
}

static Outfit hero_outfit(int h) {
    Outfit o = {HAT_NONE, 0, C_BROWN, C_BROWN, C_CREAM, C_RED, C_NAVY, C_EARTH, C_YELLOW};
    switch (h) {
    case H_TUCKER: o = (Outfit){HAT_COWBOY, 0, C_TAN, C_BROWN, C_HIDE, C_RED, C_NAVY, C_EARTH, C_CREAM}; break;
    case H_SAGE: o = (Outfit){HAT_FEATHER, 1, C_WHITE, C_INK, C_HIDE, C_TEAL, C_HIDE, C_BROWN, C_RED}; break;
    case H_HOLLIS: o = (Outfit){HAT_TOP, 0, C_INK, C_GREY, C_CREAM, C_INK, C_INK, C_INK, C_WHITE}; break;
    case H_QUILL: o = (Outfit){HAT_BOWLER, 0, C_EARTH, C_AMBER, C_CREAM, C_CREAM, C_BROWN, C_EARTH, C_FOREST}; break;
    case H_VERA: o = (Outfit){HAT_WIDE, 0, C_INK, C_INK, C_TAN, C_WINE, C_INK, C_INK, C_RED}; break;
    case H_HATTIE: o = (Outfit){HAT_BONNET, 1, C_PURPLE, C_ORANGE, C_CREAM, C_VIOLET, C_PURPLE, C_INK, C_AMBER}; break;
    case H_CORA: o = (Outfit){HAT_PLUME, 1, C_RED, C_AMBER, C_CREAM, C_RED, C_RED, C_INK, C_PINK}; break;
    default: break;
    }
    return o;
}

static int outfit_col(const Outfit *o, char c) {
    switch (c) {
    case 'H': return o->H;
    case 'R': return o->R;
    case 'K': return o->K;
    case 'S': return o->S;
    case 'P': return o->P;
    case 'B': return o->B;
    case 'A': return o->A;
    case 'k': return C_INK;
    default: return TRANSPARENT;
    }
}

static void person_sprite(Sprite *s, const Outfit *o, int view, int frame, int hero) {
    begin(16, 16);
    const char *const *rows = view == 0 ? BODY_FRONT[frame] : view == 1 ? BODY_BACK[frame] : BODY_SIDE[frame];
    for (int y = 0; y < 16; y++) {
        const char *r = rows[y];
        if (o->skirt && y >= 11) r = view == 2 ? SKIRT_SIDE[y - 11] : SKIRT_FRONT[y - 11];
        if (y <= 2 && o->hat != HAT_NONE) {
            const char *hr = HAT_ROWS[o->hat][view == 2 ? 1 : 0][y];
            if (hr) r = hr;
            if (view == 1 && hr && y == 2) {
                /* from behind: no face under the hat */
                static char tmp[17];
                memcpy(tmp, hr, 17);
                for (int i = 0; i < 16; i++) if (tmp[i] == 'K') tmp[i] = 'R';
                r = tmp;
            }
        }
        for (int x = 0; x < 16; x++) {
            int c = outfit_col(o, r[x]);
            if (c != TRANSPARENT) gfx_pset(x, y, c);
        }
    }
    /* each stranger's own touch */
    if (hero == H_HOLLIS) {
        if (view == 0) { gfx_pset(12, 9, o->S); gfx_pset(12, 10, o->S); }
        if (view == 2) { gfx_pset(6, 9, o->S); gfx_pset(6, 10, o->S); gfx_pset(7, 9, o->S); gfx_pset(7, 10, o->S); }
    }
    if (hero == H_HATTIE && view != 1) { gfx_rect(view == 2 ? 9 : 12, 10, 3, 3, C_AMBER); gfx_pset(view == 2 ? 10 : 13, 9, C_INK); }
    if (hero == H_QUILL && view == 0) { gfx_pset(6, 4, C_WHITE); gfx_pset(9, 4, C_WHITE); }
    if (hero == H_VERA && view == 0) gfx_hline(6, 9, 5, C_RED);
    if (hero == H_SAGE && view != 1) { gfx_vline(view == 2 ? 4 : 5, 3, 8, C_INK); if (view == 0) gfx_vline(10, 3, 8, C_INK); }
    if (hero == H_TUCKER && view == 0) gfx_hline(6, 9, 7, C_CREAM);
    end(s);
}

static void dog_sprite(Sprite *s, int view, int frame, int body, int patch, bool oneeye) {
    begin(16, 16);
    if (view == 2) {
        gfx_rect(3, 7, 9, 5, body);
        gfx_rect(10, 4, 5, 5, body);
        gfx_rect(14, 6, 2, 2, body);
        gfx_pset(15, 6, C_INK);
        gfx_rect(10, 3, 2, 2, patch);
        gfx_pset(12, 5, oneeye ? patch : C_INK);
        gfx_line(3, 8, 0, 5, body);
        int a = frame ? 1 : 0;
        gfx_rect(4 - a, 12, 2, 3, body);
        gfx_rect(9 + a, 12, 2, 3, body);
        gfx_rect(5, 8, 4, 3, patch);
    } else {
        gfx_rect(4, 7, 8, 7, body);
        gfx_rect(5, 2, 6, 6, body);
        gfx_rect(4, 1, 2, 3, patch);
        gfx_rect(10, 1, 2, 3, patch);
        if (view == 0) {
            gfx_pset(6, 4, oneeye ? patch : C_INK);
            gfx_pset(9, 4, C_INK);
            gfx_rect(7, 5, 2, 2, C_INK);
            if (oneeye) gfx_rect(5, 3, 3, 3, C_INK);
        }
        gfx_rect(4, 13 + frame, 2, 2, body);
        gfx_rect(10, 14 - frame, 2, 2, body);
    }
    end(s);
}

static void horse_sprite(Sprite *s, int frame, int body, int mane, bool wings) {
    begin(24, 18);
    gfx_rect(4, 7, 13, 6, body);
    gfx_rect(16, 2, 4, 7, body);
    gfx_rect(18, 1, 5, 4, body);
    gfx_pset(21, 2, C_INK);
    gfx_rect(15, 1, 2, 6, mane);
    gfx_line(4, 8, 1, 13, mane);
    int a = frame ? 2 : 0;
    gfx_rect(5 + a, 13, 2, 5, body);
    gfx_rect(14 - a, 13, 2, 5, body);
    gfx_rect(8 - a, 13, 2, 4, PAL_DARKER[body]);
    if (wings) {
        gfx_line(9, 7, 3, frame ? 0 : 3, C_INK);
        gfx_line(11, 7, 6, frame ? 0 : 2, C_INK);
        gfx_line(3, frame ? 0 : 3, 9, 4, C_DUSK);
        gfx_pset(20, 2, C_RED);
    }
    end(s);
}

/* ---- the animals Sage talks to ------------------------------------------------------------- */
static void animal_sprite(Sprite *s, int a) {
    begin(16, 16);
    switch (a) {
    case 0: /* jackrabbit */
        gfx_circ(8, 11, 4, C_TAN); gfx_circ(10, 7, 3, C_TAN); gfx_rect(9, 0, 1, 5, C_TAN); gfx_rect(11, 0, 1, 5, C_TAN);
        gfx_pset(11, 6, C_INK); gfx_pset(4, 11, C_WHITE); break;
    case 1: /* owl */
        gfx_circ(8, 9, 6, C_BROWN); gfx_circ(6, 7, 2, C_CREAM); gfx_circ(10, 7, 2, C_CREAM); gfx_pset(6, 7, C_INK); gfx_pset(10, 7, C_INK);
        gfx_pset(8, 9, C_AMBER); gfx_pset(3, 3, C_BROWN); gfx_pset(13, 3, C_BROWN); break;
    case 2: /* coyote */
        gfx_rect(3, 8, 9, 4, C_TAN); gfx_rect(10, 5, 4, 4, C_TAN); gfx_rect(10, 3, 1, 2, C_TAN); gfx_rect(12, 3, 1, 2, C_TAN);
        gfx_pset(12, 6, C_INK); gfx_rect(4, 12, 1, 3, C_TAN); gfx_rect(10, 12, 1, 3, C_TAN); gfx_line(3, 9, 0, 12, C_HIDE); break;
    case 3: /* roadrunner */
        gfx_circ(7, 9, 3, C_EARTH); gfx_circ(10, 5, 2, C_EARTH); gfx_line(11, 5, 15, 5, C_INK); gfx_line(4, 9, 0, 6, C_EARTH);
        gfx_vline(6, 12, 15, C_AMBER); gfx_vline(8, 12, 15, C_AMBER); gfx_pset(10, 4, C_RED); gfx_line(9, 2, 8, 0, C_NAVY); break;
    case 4: /* armadillo */
        gfx_circ(7, 10, 5, C_GREY); for (int x = 3; x < 12; x += 2) gfx_vline(x, 6, 13, C_SLATE); gfx_circ(13, 11, 2, C_LIGHT);
        gfx_pset(14, 10, C_INK); break;
    case 5: /* burro */
        gfx_rect(3, 7, 9, 5, C_GREY); gfx_rect(11, 4, 4, 5, C_GREY); gfx_rect(11, 1, 1, 3, C_GREY); gfx_rect(13, 1, 1, 3, C_GREY);
        gfx_pset(13, 5, C_INK); gfx_rect(4, 12, 1, 4, C_GREY); gfx_rect(10, 12, 1, 4, C_GREY); gfx_rect(14, 7, 2, 2, C_LIGHT); break;
    case 6: /* horned toad */
        gfx_circ(8, 10, 5, C_HIDE); for (int x = 4; x < 13; x += 3) gfx_pset(x, 6, C_CREAM); gfx_pset(6, 9, C_INK); gfx_pset(10, 9, C_INK);
        gfx_line(2, 13, 0, 15, C_HIDE); break;
    case 7: /* scorpion */
        gfx_circ(8, 11, 3, C_AMBER); gfx_line(8, 8, 8, 3, C_AMBER); gfx_line(8, 3, 11, 2, C_AMBER); gfx_pset(12, 3, C_RED);
        gfx_line(5, 10, 2, 7, C_AMBER); gfx_line(11, 10, 14, 7, C_AMBER); gfx_hline(4, 12, 14, C_EARTH); break;
    case 8: /* cougar */
        gfx_rect(2, 7, 10, 5, C_AMBER); gfx_rect(10, 4, 5, 5, C_AMBER); gfx_pset(11, 3, C_AMBER); gfx_pset(14, 3, C_AMBER);
        gfx_pset(12, 5, C_INK); gfx_rect(3, 12, 2, 3, C_AMBER); gfx_rect(9, 12, 2, 3, C_AMBER); gfx_line(2, 8, 0, 3, C_AMBER); break;
    case 9: /* condor */
        gfx_line(0, 6, 7, 9, C_INK); gfx_line(15, 6, 8, 9, C_INK); gfx_line(0, 7, 7, 10, C_DUSK); gfx_line(15, 7, 8, 10, C_DUSK);
        gfx_circ(8, 9, 3, C_INK); gfx_circ(8, 5, 2, C_PINK); gfx_hline(6, 10, 7, C_WHITE); break;
    }
    end(s);
}

/* ---- the monsters ---------------------------------------------------------------------- */
/* the five colour sets: as drawn, crimson, violet, black, ghost-blue */
static void make_remaps(void) {
    for (int k = 0; k < 5; k++) pal_identity(remaps[k]);
    static const uint8_t FROM[] = {C_BROWN, C_TAN, C_HIDE, C_EARTH, C_FOREST, C_JADE, C_LEAF, C_GREY, C_LIGHT};
    static const uint8_t TO[4][9] = {
        {C_WINE, C_RED, C_ORANGE, C_MAROON, C_MAROON, C_WINE, C_RED, C_PINK, C_PINK},
        {C_PURPLE, C_VIOLET, C_MAGENTA, C_NIGHT, C_NIGHT, C_PURPLE, C_VIOLET, C_VIOLET, C_PINK},
        {C_SLATE, C_GREY, C_LIGHT, C_INK, C_INK, C_SLATE, C_GREY, C_DUSK, C_GREY},
        {C_NAVY, C_SKY, C_ICE, C_NAVY, C_TEAL, C_BLUE, C_SKY, C_ICE, C_WHITE},
    };
    for (int k = 0; k < 4; k++)
        for (int i = 0; i < 9; i++) remaps[k + 1][FROM[i]] = TO[k][i];
}

static void eye(int x, int y, int c) { gfx_rect(x, y, 2, 2, c); gfx_pset(x, y, C_WHITE); }
static void eyes(int cx, int y, int gap, int c) { eye(cx - gap - 1, y, c); eye(cx + gap - 1, y, c); }
static void shade(int x, int y, int w, int h) { gfx_dither(x, y, w, h, C_INK, 5); }

static void paint_foe(int art) {
    int W = canvas.w, H = canvas.h, cx = W / 2;
    switch (art) {
    case A_RAT:
        gfx_circ(cx, 15, 7, C_BROWN); gfx_circ(cx, 9, 5, C_BROWN);
        gfx_circ(cx - 5, 4, 3, C_BROWN); gfx_circ(cx + 5, 4, 3, C_BROWN); gfx_circ(cx - 5, 4, 1, C_PINK); gfx_circ(cx + 5, 4, 1, C_PINK);
        eyes(cx, 8, 3, C_RED); gfx_rect(cx - 1, 12, 2, 2, C_PINK);
        gfx_line(cx - 2, 12, cx - 8, 11, C_LIGHT); gfx_line(cx + 2, 12, cx + 8, 11, C_LIGHT);
        gfx_line(cx + 6, 19, cx + 11, 23, C_PINK); gfx_rect(cx - 6, 21, 3, 2, C_PINK); gfx_rect(cx + 3, 21, 3, 2, C_PINK);
        shade(cx - 7, 17, 14, 5);
        break;
    case A_WEED:
        for (int i = 0; i < 14; i++) {
            int a = i * 23;
            gfx_line(cx, 12, cx + (int)(10 * sinf((float)a)), 12 + (int)(10 * cosf((float)a)), i % 2 ? C_TAN : C_BROWN);
        }
        gfx_circb(cx, 12, 9, C_TAN); gfx_circb(cx, 12, 6, C_BROWN);
        eyes(cx, 10, 3, C_YELLOW);
        break;
    case A_BIRD:
        gfx_circ(cx, 14, 6, C_BROWN); gfx_circ(cx, 7, 4, C_HIDE);
        gfx_line(cx - 6, 12, cx - 11, 6, C_BROWN); gfx_line(cx - 6, 13, cx - 11, 8, C_EARTH); gfx_line(cx - 11, 6, cx - 11, 12, C_BROWN);
        gfx_line(cx + 6, 12, cx + 11, 6, C_BROWN); gfx_line(cx + 6, 13, cx + 11, 8, C_EARTH); gfx_line(cx + 11, 6, cx + 11, 12, C_BROWN);
        gfx_rect(cx - 1, 9, 2, 3, C_AMBER); eyes(cx, 6, 2, C_INK);
        gfx_vline(cx - 2, 20, 23, C_AMBER); gfx_vline(cx + 1, 20, 23, C_AMBER);
        break;
    case A_BAT:
        gfx_circ(cx, 11, 4, C_EARTH);
        for (int s = -1; s <= 1; s += 2) {
            gfx_line(cx + s * 4, 9, cx + s * 11, 5, C_BROWN);
            gfx_line(cx + s * 11, 5, cx + s * 9, 14, C_BROWN);
            gfx_line(cx + s * 9, 14, cx + s * 4, 12, C_BROWN);
            for (int k = 5; k < 11; k++) gfx_line(cx + s * 4, 10, cx + s * k, 6 + (k - 5), C_BROWN);
        }
        gfx_pset(cx - 3, 6, C_EARTH); gfx_pset(cx + 2, 6, C_EARTH);
        eyes(cx, 10, 2, C_RED); gfx_pset(cx - 1, 13, C_WHITE); gfx_pset(cx, 13, C_WHITE);
        break;
    case A_BANDIT:
        gfx_rect(cx - 6, 10, 12, 9, C_BROWN); gfx_circ(cx, 6, 4, C_CREAM);
        gfx_rect(cx - 8, 2, 16, 2, C_INK); gfx_rect(cx - 4, -1, 8, 4, C_INK);
        gfx_rect(cx - 4, 7, 8, 3, C_RED); eyes(cx, 5, 2, C_INK);
        gfx_rect(cx - 5, 19, 4, 5, C_EARTH); gfx_rect(cx + 1, 19, 4, 5, C_EARTH);
        gfx_rect(cx + 6, 12, 5, 2, C_GREY); gfx_rect(cx + 6, 12, 2, 4, C_EARTH);
        gfx_rect(cx - 9, 11, 3, 6, C_BROWN);
        shade(cx - 6, 15, 12, 4);
        break;
    case A_PUCK:
        gfx_circ(cx, 15, 6, C_FOREST); gfx_circ(cx, 8, 6, C_JADE);
        gfx_line(cx - 5, 6, cx - 11, 2, C_JADE); gfx_line(cx + 5, 6, cx + 11, 2, C_JADE);
        eyes(cx, 7, 3, C_YELLOW); gfx_hline(cx - 3, cx + 3, 11, C_INK); gfx_pset(cx - 2, 12, C_WHITE); gfx_pset(cx + 2, 12, C_WHITE);
        gfx_rect(cx - 4, 21, 3, 3, C_FOREST); gfx_rect(cx + 1, 21, 3, 3, C_FOREST);
        gfx_line(cx - 6, 14, cx - 10, 18, C_JADE); gfx_line(cx + 6, 14, cx + 10, 18, C_JADE);
        break;
    case A_DIGGER:
        gfx_circ(cx, 15, 7, C_BROWN); gfx_circ(cx, 8, 5, C_TAN);
        gfx_rect(cx - 6, 1, 12, 4, C_GREY); gfx_rect(cx - 1, 2, 2, 2, C_YELLOW);
        eyes(cx, 8, 2, C_INK); gfx_rect(cx - 4, 11, 8, 3, C_LIGHT);
        gfx_line(cx + 7, 18, cx + 11, 8, C_EARTH); gfx_line(cx + 8, 8, cx + 12, 6, C_GREY); gfx_line(cx + 10, 6, cx + 12, 10, C_GREY);
        gfx_rect(cx - 5, 21, 4, 3, C_EARTH); gfx_rect(cx + 1, 21, 4, 3, C_EARTH);
        break;
    case A_MOLD:
        gfx_circ(cx, 16, 9, C_FOREST); gfx_circ(cx - 4, 13, 5, C_JADE); gfx_circ(cx + 5, 12, 4, C_LEAF);
        gfx_rect(cx - 9, 18, 18, 6, C_FOREST);
        eyes(cx, 14, 3, C_WHITE); gfx_pset(cx - 3, 15, C_INK); gfx_pset(cx + 3, 15, C_INK);
        gfx_pset(cx - 6, 9, C_LIME); gfx_pset(cx + 2, 8, C_LIME);
        break;
    case A_DOG:
        gfx_rect(cx - 7, 11, 14, 8, C_TAN); gfx_circ(cx, 8, 5, C_TAN);
        gfx_line(cx - 4, 4, cx - 7, -1, C_TAN); gfx_line(cx - 3, 4, cx - 6, 0, C_TAN); gfx_line(cx + 4, 4, cx + 7, -1, C_TAN); gfx_line(cx + 3, 4, cx + 6, 0, C_TAN);
        gfx_rect(cx - 3, 10, 6, 4, C_HIDE); gfx_rect(cx - 1, 10, 2, 2, C_INK);
        eyes(cx, 6, 3, C_YELLOW); gfx_pset(cx - 2, 14, C_WHITE); gfx_pset(cx + 1, 14, C_WHITE);
        gfx_rect(cx - 6, 19, 3, 5, C_TAN); gfx_rect(cx + 3, 19, 3, 5, C_TAN);
        shade(cx - 7, 16, 14, 3);
        break;
    case A_SPIDER:
        gfx_circ(cx, 14, 7, C_EARTH); gfx_circ(cx, 7, 4, C_BROWN);
        for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 4; k++) {
            int y = 9 + k * 3;
            gfx_line(cx + s * 5, y, cx + s * 10, y - 3 + k, C_BROWN);
            gfx_line(cx + s * 10, y - 3 + k, cx + s * 11, y + 5, C_BROWN);
        }
        eyes(cx, 6, 2, C_RED); gfx_pset(cx - 1, 4, C_RED); gfx_pset(cx, 4, C_RED);
        gfx_rect(cx - 2, 13, 4, 4, C_HIDE);
        break;
    case A_SNAKE:
        gfx_circ(cx, 19, 7, C_JADE); gfx_circ(cx, 19, 4, C_FOREST); gfx_rect(cx - 2, 8, 4, 10, C_JADE);
        gfx_circ(cx, 6, 5, C_JADE); eyes(cx, 4, 2, C_YELLOW);
        gfx_line(cx, 10, cx - 1, 14, C_RED); gfx_line(cx, 10, cx + 1, 14, C_RED);
        for (int y = 9; y < 18; y += 3) gfx_hline(cx - 2, cx + 1, y, C_LEAF);
        gfx_rect(cx + 6, 18, 2, 4, C_TAN);
        break;
    case A_GHOUL:
        gfx_rect(cx - 6, 9, 12, 11, C_GREY); gfx_circ(cx, 6, 5, C_LIGHT);
        gfx_rect(cx - 3, 4, 2, 3, C_INK); gfx_rect(cx + 1, 4, 2, 3, C_INK); gfx_hline(cx - 2, cx + 2, 9, C_INK);
        gfx_line(cx - 6, 10, cx - 11, 17, C_LIGHT); gfx_line(cx + 6, 10, cx + 11, 17, C_LIGHT);
        gfx_rect(cx - 5, 20, 3, 4, C_GREY); gfx_rect(cx + 2, 20, 3, 4, C_GREY);
        gfx_hline(cx - 6, cx + 5, 14, C_BROWN);
        shade(cx - 6, 16, 12, 4);
        break;
    case A_GOLEM:
        gfx_rect(cx - 10, 10, 20, 14, C_GREY); gfx_rect(cx - 7, 2, 14, 10, C_LIGHT);
        gfx_rect(cx - 15, 10, 6, 14, C_GREY); gfx_rect(cx + 9, 10, 6, 14, C_GREY);
        gfx_rect(cx - 16, 20, 8, 8, C_LIGHT); gfx_rect(cx + 8, 20, 8, 8, C_LIGHT);
        eyes(cx, 6, 3, C_ORANGE); gfx_hline(cx - 3, cx + 3, 9, C_SLATE);
        gfx_rect(cx - 8, 24, 6, 8, C_GREY); gfx_rect(cx + 2, 24, 6, 8, C_GREY);
        gfx_line(cx - 6, 13, cx - 2, 18, C_SLATE); gfx_line(cx + 4, 12, cx + 6, 17, C_SLATE);
        shade(cx - 10, 18, 20, 6);
        break;
    case A_HARE:
        gfx_circ(cx, 16, 7, C_TAN); gfx_circ(cx, 9, 5, C_TAN);
        gfx_rect(cx - 4, 0, 2, 7, C_TAN); gfx_rect(cx + 2, 0, 2, 7, C_TAN);
        gfx_line(cx - 3, 2, cx - 8, -2, C_CREAM); gfx_line(cx + 3, 2, cx + 8, -2, C_CREAM); /* antlers */
        gfx_line(cx - 6, 0, cx - 7, 3, C_CREAM); gfx_line(cx + 6, 0, cx + 7, 3, C_CREAM);
        eyes(cx, 8, 2, C_RED); gfx_pset(cx, 11, C_PINK);
        gfx_rect(cx - 6, 21, 4, 3, C_TAN); gfx_rect(cx + 2, 21, 4, 3, C_TAN); gfx_circ(cx, 18, 2, C_WHITE);
        break;
    case A_GOAT:
        gfx_rect(cx - 7, 10, 14, 9, C_GREY); gfx_circ(cx, 7, 5, C_LIGHT);
        gfx_line(cx - 3, 3, cx - 9, 0, C_SLATE); gfx_line(cx + 3, 3, cx + 9, 0, C_SLATE);
        eyes(cx, 6, 2, C_RED);
        for (int x = cx - 6; x <= cx + 6; x += 3) gfx_line(x, 10, x, 7, C_SLATE); /* the spines */
        gfx_rect(cx - 6, 19, 2, 5, C_GREY); gfx_rect(cx + 4, 19, 2, 5, C_GREY);
        gfx_rect(cx - 1, 11, 2, 3, C_WHITE);
        break;
    case A_APE:
        gfx_rect(cx - 10, 10, 20, 16, C_BROWN); gfx_circ(cx, 8, 7, C_BROWN);
        gfx_rect(cx - 4, 6, 8, 6, C_TAN); eyes(cx, 7, 2, C_INK); gfx_hline(cx - 2, cx + 1, 11, C_INK);
        gfx_rect(cx - 15, 11, 5, 15, C_BROWN); gfx_rect(cx + 10, 11, 5, 15, C_BROWN);
        gfx_rect(cx - 9, 26, 7, 6, C_EARTH); gfx_rect(cx + 2, 26, 7, 6, C_EARTH);
        gfx_rect(cx - 5, 14, 10, 8, C_HIDE);
        shade(cx - 10, 22, 20, 4);
        break;
    case A_RIDER:
        gfx_rect(cx - 12, 16, 24, 9, C_INK); gfx_rect(cx - 12, 25, 4, 7, C_INK); gfx_rect(cx + 8, 25, 4, 7, C_INK);
        gfx_rect(cx - 16, 12, 6, 8, C_INK); gfx_pset(cx - 14, 14, C_RED);
        gfx_rect(cx - 5, 3, 10, 13, C_BROWN); gfx_circ(cx, 3, 4, C_GREY);
        gfx_rect(cx - 7, 0, 14, 2, C_INK); gfx_rect(cx - 3, -3, 6, 4, C_INK);
        eyes(cx, 3, 2, C_RED);
        gfx_line(cx + 5, 6, cx + 14, 0, C_LIGHT);
        shade(cx - 12, 22, 24, 3);
        break;
    case A_TROOPER:
        gfx_rect(cx - 8, 10, 16, 12, C_BROWN); gfx_circ(cx, 6, 5, C_RED);
        gfx_line(cx - 4, 2, cx - 8, -3, C_CREAM); gfx_line(cx + 4, 2, cx + 8, -3, C_CREAM);
        eyes(cx, 5, 2, C_YELLOW); gfx_hline(cx - 2, cx + 2, 9, C_INK);
        gfx_rect(cx - 12, 11, 4, 10, C_BROWN); gfx_rect(cx + 8, 11, 4, 10, C_BROWN);
        gfx_line(cx + 11, 4, cx + 11, 28, C_GREY); gfx_line(cx + 9, 6, cx + 13, 4, C_LIGHT);
        gfx_rect(cx - 7, 22, 5, 10, C_EARTH); gfx_rect(cx + 2, 22, 5, 10, C_EARTH);
        gfx_hline(cx - 8, cx + 7, 16, C_AMBER);
        shade(cx - 8, 18, 16, 4);
        break;
    case A_BISON:
        gfx_rect(cx - 13, 12, 26, 14, C_BROWN); gfx_circ(cx, 10, 9, C_EARTH);
        gfx_line(cx - 8, 6, cx - 15, 1, C_CREAM); gfx_line(cx + 8, 6, cx + 15, 1, C_CREAM);
        gfx_line(cx - 9, 7, cx - 15, 3, C_CREAM); gfx_line(cx + 9, 7, cx + 15, 3, C_CREAM);
        eyes(cx, 9, 4, C_RED); gfx_rect(cx - 3, 13, 6, 4, C_HIDE);
        gfx_rect(cx - 12, 26, 5, 6, C_EARTH); gfx_rect(cx + 7, 26, 5, 6, C_EARTH);
        gfx_rect(cx - 13, 12, 26, 3, C_HIDE);
        shade(cx - 13, 22, 26, 4);
        break;
    case A_MOTH:
        for (int s = -1; s <= 1; s += 2) {
            gfx_circ(cx + s * 8, 11, 7, C_TAN); gfx_circ(cx + s * 7, 21, 5, C_HIDE);
            gfx_circ(cx + s * 9, 10, 3, C_BROWN); gfx_circ(cx + s * 9, 10, 1, C_YELLOW);
        }
        gfx_rect(cx - 2, 6, 4, 20, C_EARTH); eyes(cx, 6, 1, C_RED);
        gfx_line(cx - 1, 5, cx - 5, 0, C_EARTH); gfx_line(cx, 5, cx + 4, 0, C_EARTH);
        break;
    case A_CENTI:
        for (int k = 0; k < 6; k++) {
            int y = 4 + k * 5;
            int x = cx + (k % 2 ? 3 : -3);
            gfx_circ(x, y, 4, k == 0 ? C_RED : C_BROWN);
            gfx_line(x - 4, y, x - 10, y + 3, C_EARTH);
            gfx_line(x + 4, y, x + 10, y + 3, C_EARTH);
        }
        eyes(cx - 3, 3, 2, C_YELLOW);
        gfx_line(cx - 5, 6, cx - 8, 10, C_CREAM); gfx_line(cx - 1, 6, cx + 2, 10, C_CREAM);
        break;
    case A_GATOR:
        gfx_rect(cx - 14, 16, 28, 10, C_FOREST); gfx_rect(cx - 8, 4, 16, 14, C_JADE);
        gfx_rect(cx - 6, 14, 12, 6, C_LEAF);
        for (int x = cx - 6; x < cx + 6; x += 2) { gfx_pset(x, 14, C_WHITE); gfx_pset(x + 1, 19, C_WHITE); }
        eyes(cx, 6, 4, C_YELLOW);
        gfx_rect(cx - 14, 24, 5, 6, C_FOREST); gfx_rect(cx + 9, 24, 5, 6, C_FOREST);
        for (int x = cx - 12; x < cx + 12; x += 4) gfx_pset(x, 16, C_LEAF);
        break;
    case A_HAG:
        gfx_rect(cx - 9, 12, 18, 18, C_FOREST); gfx_circ(cx, 8, 6, C_JADE);
        gfx_rect(cx - 8, 2, 16, 4, C_INK); gfx_rect(cx - 9, 6, 3, 18, C_INK); gfx_rect(cx + 6, 6, 3, 18, C_INK);
        eyes(cx, 7, 2, C_YELLOW); gfx_rect(cx - 3, 11, 6, 2, C_LIME);
        for (int x = cx - 2; x <= cx + 1; x++) gfx_pset(x, 11, C_WHITE);
        gfx_line(cx - 9, 14, cx - 15, 22, C_JADE); gfx_line(cx + 9, 14, cx + 15, 22, C_JADE);
        gfx_line(cx - 15, 22, cx - 16, 25, C_INK); gfx_line(cx + 15, 22, cx + 16, 25, C_INK);
        break;
    case A_EYE:
        gfx_circ(cx, 11, 9, C_LIGHT); gfx_circ(cx, 11, 5, C_RED); gfx_circ(cx, 11, 2, C_INK); gfx_pset(cx - 2, 8, C_WHITE);
        for (int k = 0; k < 5; k++) gfx_line(cx - 6 + k * 3, 19, cx - 8 + k * 4, 23, C_GREY);
        gfx_circb(cx, 11, 9, C_GREY);
        break;
    case A_OWL:
        gfx_circ(cx, 13, 9, C_BROWN); gfx_circ(cx - 4, 10, 4, C_CREAM); gfx_circ(cx + 4, 10, 4, C_CREAM);
        gfx_circ(cx - 4, 10, 2, C_YELLOW); gfx_circ(cx + 4, 10, 2, C_YELLOW); gfx_pset(cx - 4, 10, C_INK); gfx_pset(cx + 4, 10, C_INK);
        gfx_rect(cx - 1, 13, 2, 3, C_AMBER);
        gfx_line(cx - 6, 3, cx - 8, 0, C_BROWN); gfx_line(cx + 6, 3, cx + 8, 0, C_BROWN);
        gfx_circ(cx, 15, 2, C_WHITE); /* a woman's face in the breast feathers */
        gfx_rect(cx - 4, 21, 2, 3, C_AMBER); gfx_rect(cx + 2, 21, 2, 3, C_AMBER);
        break;
    case A_CANNON:
        gfx_rect(cx - 12, 14, 24, 12, C_GREY); gfx_rect(cx - 3, 4, 14, 8, C_SLATE); gfx_circ(cx + 11, 8, 4, C_INK);
        gfx_circ(cx - 8, 26, 5, C_BROWN); gfx_circ(cx + 8, 26, 5, C_BROWN); gfx_circ(cx - 8, 26, 2, C_EARTH); gfx_circ(cx + 8, 26, 2, C_EARTH);
        gfx_circ(cx - 6, 9, 4, C_RED); eyes(cx - 6, 8, 1, C_YELLOW);
        gfx_line(cx - 9, 4, cx - 11, 0, C_CREAM); gfx_line(cx - 3, 4, cx - 1, 0, C_CREAM);
        break;
    case A_GHOST:
        gfx_circ(cx, 9, 8, C_LIGHT); gfx_rect(cx - 8, 9, 16, 11, C_LIGHT);
        for (int x = cx - 8; x < cx + 8; x += 4) gfx_circ(x + 2, 20, 2, C_LIGHT);
        gfx_rect(cx - 5, 7, 3, 4, C_INK); gfx_rect(cx + 2, 7, 3, 4, C_INK); gfx_circ(cx, 14, 2, C_INK);
        gfx_dither(cx - 8, 14, 16, 8, C_GREY, 6);
        break;
    case A_BEAN:
        gfx_circ(cx, 14, 7, C_HIDE); gfx_circ(cx, 12, 5, C_TAN);
        eyes(cx, 11, 2, C_INK); gfx_line(cx - 2, 15, cx + 2, 15, C_INK);
        gfx_line(cx - 8, 10, cx - 11, 7, C_YELLOW); gfx_line(cx + 8, 10, cx + 11, 7, C_YELLOW); /* jitter lines */
        gfx_line(cx - 9, 16, cx - 11, 18, C_YELLOW); gfx_line(cx + 9, 16, cx + 11, 18, C_YELLOW);
        break;
    case A_MONK:
        gfx_rect(cx - 7, 9, 14, 15, C_EARTH); gfx_circ(cx, 7, 6, C_EARTH);
        gfx_circ(cx, 8, 4, C_INK); eyes(cx, 7, 2, C_RED);
        gfx_line(cx - 7, 11, cx - 11, 20, C_EARTH); gfx_line(cx + 7, 11, cx + 11, 20, C_EARTH);
        gfx_hline(cx - 6, cx + 6, 15, C_RED); gfx_rect(cx - 1, 15, 2, 6, C_RED);
        shade(cx - 7, 19, 14, 5);
        break;
    /* ---- bosses: 48 x 48 ---- */
    case A_PUCKKING:
        gfx_circ(cx, 30, 14, C_FOREST); gfx_circ(cx, 18, 11, C_JADE);
        gfx_line(cx - 9, 14, cx - 22, 6, C_JADE); gfx_line(cx + 9, 14, cx + 22, 6, C_JADE);
        eyes(cx, 16, 5, C_YELLOW); gfx_hline(cx - 6, cx + 6, 24, C_INK);
        for (int x = cx - 5; x <= cx + 5; x += 2) gfx_pset(x, 25, C_WHITE);
        for (int k = 0; k < 5; k++) { gfx_rect(cx - 10 + k * 4, 4 + (k % 2) * 2, 3, 4, C_YELLOW); gfx_pset(cx - 9 + k * 4, 5, C_RED); }
        gfx_rect(cx - 10, 42, 6, 6, C_FOREST); gfx_rect(cx + 4, 42, 6, 6, C_FOREST);
        break;
    case A_SCOURGE:
        gfx_rect(cx - 10, 14, 20, 24, C_EARTH); gfx_circ(cx, 10, 8, C_GREY);
        gfx_rect(cx - 6, 7, 12, 3, C_INK); eyes(cx, 8, 3, C_RED);
        for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 3; k++) {
            int y = 16 + k * 7;
            gfx_line(cx + s * 10, y, cx + s * 18, y - 2, C_GREY);
            gfx_line(cx + s * 18, y - 2, cx + s * 23, y + 8, C_BROWN);
        }
        gfx_rect(cx - 8, 38, 6, 10, C_INK); gfx_rect(cx + 2, 38, 6, 10, C_INK);
        gfx_hline(cx - 10, cx + 9, 26, C_YELLOW);
        shade(cx - 10, 32, 20, 6);
        break;
    case A_BRUTUS:
        gfx_rect(cx - 16, 14, 32, 26, C_RED); gfx_circ(cx, 10, 9, C_RED);
        gfx_line(cx - 6, 3, cx - 12, -4, C_CREAM); gfx_line(cx + 6, 3, cx + 12, -4, C_CREAM);
        eyes(cx, 9, 4, C_YELLOW); gfx_rect(cx - 5, 14, 10, 3, C_INK);
        gfx_rect(cx - 22, 15, 7, 20, C_RED); gfx_rect(cx + 15, 15, 7, 20, C_RED);
        gfx_rect(cx - 16, 26, 32, 4, C_INK); gfx_rect(cx - 3, 26, 6, 4, C_YELLOW);
        gfx_rect(cx - 14, 40, 10, 8, C_MAROON); gfx_rect(cx + 4, 40, 10, 8, C_MAROON);
        shade(cx - 16, 34, 32, 6);
        break;
    case A_TALLYMAN:
        gfx_rect(cx - 9, 14, 18, 24, C_INK); gfx_circ(cx, 9, 7, C_LIGHT);
        gfx_rect(cx - 10, 0, 20, 3, C_INK); gfx_rect(cx - 6, -6, 12, 7, C_INK);
        gfx_circ(cx - 3, 9, 2, C_WHITE); gfx_circ(cx + 3, 9, 2, C_WHITE); gfx_hline(cx - 1, cx + 1, 9, C_YELLOW);
        gfx_rect(cx - 18, 22, 12, 14, C_CREAM); gfx_rect(cx + 6, 22, 12, 14, C_CREAM);
        for (int y = 24; y < 35; y += 3) { gfx_hline(cx - 16, cx - 8, y, C_SLATE); gfx_hline(cx + 8, cx + 16, y, C_SLATE); }
        gfx_line(cx - 2, 16, cx - 2, 38, C_RED);
        gfx_rect(cx - 7, 38, 5, 10, C_INK); gfx_rect(cx + 2, 38, 5, 10, C_INK);
        break;
    case A_JACK:
        gfx_rect(cx - 10, 16, 20, 20, C_BROWN); gfx_circ(cx, 12, 8, C_TAN);
        gfx_line(cx - 5, 6, cx - 9, -2, C_TAN); gfx_line(cx + 5, 6, cx + 9, -2, C_TAN);
        gfx_rect(cx - 4, 15, 8, 6, C_HIDE); gfx_rect(cx - 2, 15, 4, 2, C_INK);
        eyes(cx, 10, 4, C_YELLOW);
        for (int x = cx - 3; x <= cx + 3; x += 2) gfx_pset(x, 20, C_WHITE);
        gfx_rect(cx - 16, 18, 6, 14, C_BROWN); gfx_rect(cx + 10, 18, 6, 14, C_BROWN);
        gfx_rect(cx - 20, 24, 6, 3, C_GREY); gfx_rect(cx + 14, 24, 6, 3, C_GREY);
        gfx_rect(cx - 8, 36, 6, 12, C_EARTH); gfx_rect(cx + 2, 36, 6, 12, C_EARTH);
        gfx_hline(cx - 10, cx + 9, 26, C_INK);
        break;
    case A_MAGPIE:
        gfx_rect(cx - 9, 16, 18, 22, C_INK); gfx_rect(cx - 4, 16, 8, 14, C_WHITE); gfx_circ(cx, 10, 7, C_CREAM);
        gfx_rect(cx - 10, 2, 20, 3, C_INK); gfx_rect(cx - 5, -4, 10, 7, C_INK);
        eyes(cx, 9, 3, C_INK); gfx_hline(cx - 3, cx + 3, 14, C_BROWN);
        gfx_rect(cx - 1, 18, 2, 4, C_RED);
        gfx_rect(cx - 20, 16, 8, 10, C_WHITE); gfx_rectb(cx - 20, 16, 8, 10, C_RED); gfx_pset(cx - 16, 21, C_RED);
        gfx_rect(cx + 12, 16, 8, 10, C_WHITE); gfx_rectb(cx + 12, 16, 8, 10, C_INK); gfx_pset(cx + 16, 21, C_INK);
        gfx_rect(cx - 7, 38, 5, 10, C_INK); gfx_rect(cx + 2, 38, 5, 10, C_INK);
        break;
    case A_MIMIC:
        gfx_rect(cx - 10, 14, 20, 30, C_INK); gfx_circ(cx, 10, 9, C_CREAM);
        gfx_circ(cx - 4, 8, 3, C_INK); gfx_circ(cx + 4, 12, 2, C_INK); gfx_pset(cx - 4, 8, C_RED); gfx_pset(cx + 4, 12, C_RED);
        gfx_line(cx - 6, 15, cx + 7, 13, C_RED);
        for (int k = 0; k < 6; k++) gfx_line(cx - 10 + k * 4, 44, cx - 12 + k * 5, 48, C_PINK);
        gfx_rect(cx - 2, 18, 4, 12, C_WHITE); gfx_rect(cx - 5, 21, 10, 3, C_WHITE);
        gfx_line(cx - 10, 18, cx - 20, 30, C_PINK); gfx_line(cx + 10, 18, cx + 20, 30, C_PINK);
        break;
    case A_MOLLY:
        gfx_rect(cx - 10, 16, 20, 24, C_WINE); gfx_circ(cx, 11, 7, C_TAN);
        gfx_rect(cx - 14, 3, 28, 3, C_INK); gfx_rect(cx - 6, -3, 12, 7, C_INK);
        gfx_rect(cx - 7, 4, 3, 14, C_INK); gfx_rect(cx + 4, 4, 3, 14, C_INK);
        eyes(cx, 10, 3, C_INK); gfx_hline(cx - 2, cx + 2, 15, C_RED);
        gfx_rect(cx - 18, 20, 8, 3, C_GREY); gfx_rect(cx + 10, 20, 8, 3, C_GREY);
        gfx_rect(cx - 14, 40, 28, 8, C_WINE);
        gfx_hline(cx - 10, cx + 9, 28, C_INK);
        break;
    case A_CAPTAIN:
        gfx_rect(cx - 12, 14, 24, 22, C_BROWN); gfx_circ(cx, 9, 7, C_RED);
        gfx_line(cx - 5, 3, cx - 12, -4, C_CREAM); gfx_line(cx + 5, 3, cx + 12, -4, C_CREAM);
        eyes(cx, 8, 3, C_YELLOW);
        gfx_rect(cx - 12, 14, 24, 4, C_AMBER);
        gfx_line(cx + 16, 0, cx + 16, 44, C_GREY); gfx_rect(cx + 13, 2, 7, 6, C_RED);
        gfx_rect(cx - 9, 36, 7, 12, C_EARTH); gfx_rect(cx + 2, 36, 7, 12, C_EARTH);
        break;
    case A_STORMCROW:
        gfx_circ(cx, 20, 10, C_NAVY); gfx_circ(cx, 9, 7, C_NAVY);
        for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 6; k++) gfx_line(cx + s * 8, 18 + k, cx + s * 24, 6 + k * 4, k % 2 ? C_BLUE : C_NAVY);
        gfx_rect(cx - 2, 11, 4, 6, C_YELLOW); eyes(cx, 7, 3, C_CYAN);
        gfx_line(cx - 18, 30, cx - 22, 40, C_YELLOW); gfx_line(cx - 22, 40, cx - 18, 42, C_YELLOW);
        gfx_line(cx + 18, 30, cx + 22, 40, C_YELLOW); gfx_line(cx + 22, 40, cx + 18, 42, C_YELLOW);
        gfx_vline(cx - 3, 30, 40, C_AMBER); gfx_vline(cx + 2, 30, 40, C_AMBER);
        break;
    case A_WORM:
        for (int k = 0; k < 7; k++) gfx_circ(cx + (k % 2 ? 3 : -3), 44 - k * 6, 9 - k / 2, k % 2 ? C_RED : C_MAROON);
        gfx_circ(cx, 8, 9, C_ORANGE);
        gfx_circ(cx, 10, 5, C_INK);
        for (int k = 0; k < 8; k++) gfx_pset(cx - 4 + k, 6 + (k % 2) * 7, C_YELLOW);
        eyes(cx, 3, 6, C_YELLOW);
        for (int k = 0; k < 6; k++) gfx_pset(cx - 10 + k * 4, 30 + (k % 2) * 6, C_YELLOW);
        break;
    case A_MOTHER:
        gfx_rect(cx - 9, 4, 18, 44, C_JADE); gfx_circ(cx, 4, 9, C_JADE);
        gfx_rect(cx - 22, 18, 8, 4, C_JADE); gfx_rect(cx - 22, 6, 4, 14, C_JADE);
        gfx_rect(cx + 14, 22, 8, 4, C_JADE); gfx_rect(cx + 18, 10, 4, 14, C_JADE);
        for (int y = 2; y < 46; y += 3) { gfx_pset(cx - 5, y, C_LEAF); gfx_pset(cx + 5, y + 1, C_LEAF); }
        eyes(cx, 12, 4, C_PINK); gfx_hline(cx - 4, cx + 4, 19, C_FOREST);
        gfx_circ(cx, -1, 3, C_PINK); gfx_circ(cx - 5, 1, 2, C_YELLOW); gfx_circ(cx + 5, 1, 2, C_YELLOW);
        break;
    case A_ARM:
        gfx_rect(cx - 4, 4, 9, 28, C_JADE); gfx_circ(cx, 4, 6, C_JADE);
        for (int y = 4; y < 30; y += 4) { gfx_pset(cx - 5, y, C_CREAM); gfx_pset(cx + 5, y + 2, C_CREAM); }
        eyes(cx, 6, 2, C_PINK);
        break;
    case A_BLIGHT:
        gfx_rect(cx - 20, 24, 40, 14, C_AMBER); gfx_rect(cx - 20, 38, 6, 10, C_AMBER); gfx_rect(cx + 14, 38, 6, 10, C_AMBER);
        gfx_rect(cx - 24, 18, 8, 10, C_AMBER);
        gfx_rect(cx - 7, 4, 14, 22, C_JADE); gfx_circ(cx, 4, 6, C_LIME);
        eyes(cx, 3, 2, C_INK);
        for (int k = 0; k < 20; k++) { uint32_t r = h3(k, 3, 9); gfx_pset((int)(r % 48), (int)((r >> 7) % 30), C_INK); }
        break;
    case A_CARNAGE:
        gfx_rect(cx - 20, 24, 40, 14, C_RED); gfx_rect(cx - 20, 38, 6, 10, C_RED); gfx_rect(cx + 14, 38, 6, 10, C_RED);
        gfx_rect(cx - 24, 18, 8, 10, C_RED); gfx_pset(cx - 21, 20, C_YELLOW);
        gfx_rect(cx - 8, 4, 16, 22, C_MAROON); gfx_circ(cx, 4, 6, C_GREY); gfx_rect(cx - 4, 3, 8, 2, C_INK);
        gfx_line(cx + 8, 10, cx + 22, -2, C_LIGHT); gfx_line(cx + 9, 10, cx + 23, -2, C_WHITE);
        gfx_rect(cx - 8, 12, 16, 2, C_AMBER);
        break;
    case A_HUNGER:
        gfx_rect(cx - 20, 24, 40, 12, C_INK); gfx_rect(cx - 20, 36, 4, 12, C_INK); gfx_rect(cx + 16, 36, 4, 12, C_INK);
        for (int x = cx - 18; x < cx + 18; x += 3) gfx_vline(x, 26, 34, C_GREY); /* ribs */
        gfx_rect(cx - 24, 18, 8, 8, C_INK);
        gfx_rect(cx - 4, 2, 8, 24, C_LIGHT); gfx_circ(cx, 2, 5, C_WHITE); gfx_rect(cx - 2, 1, 2, 2, C_INK); gfx_rect(cx + 1, 1, 2, 2, C_INK);
        gfx_line(cx - 4, 12, cx - 16, 20, C_LIGHT); gfx_line(cx + 4, 12, cx + 16, 6, C_LIGHT);
        gfx_line(cx + 16, 6, cx + 18, 2, C_AMBER); /* the scales */
        gfx_hline(cx + 14, cx + 22, 2, C_AMBER);
        break;
    case A_REAPER:
        gfx_rect(cx - 20, 24, 40, 14, C_LIGHT); gfx_rect(cx - 20, 38, 6, 10, C_LIGHT); gfx_rect(cx + 14, 38, 6, 10, C_LIGHT);
        gfx_rect(cx - 24, 18, 8, 10, C_LIGHT); gfx_pset(cx - 21, 20, C_INK);
        gfx_rect(cx - 7, 4, 14, 22, C_INK); gfx_circ(cx, 4, 5, C_WHITE);
        gfx_rect(cx - 12, -2, 24, 3, C_LIGHT); gfx_rect(cx - 5, -6, 10, 5, C_LIGHT);
        gfx_rect(cx - 2, 3, 2, 2, C_INK); gfx_rect(cx + 1, 3, 2, 2, C_INK);
        gfx_rect(cx - 4, 12, 3, 3, C_YELLOW); /* an old tin star */
        gfx_line(cx + 10, 0, cx + 10, 30, C_BROWN); gfx_line(cx + 10, 0, cx + 22, 6, C_LIGHT);
        break;
    case A_JUGGERNAUT:
        gfx_rect(cx - 22, 10, 44, 34, C_INK); gfx_rect(cx - 18, 2, 10, 10, C_SLATE); gfx_rect(cx + 8, 4, 8, 8, C_SLATE);
        gfx_circ(cx, 24, 10, C_GREY); gfx_circ(cx, 24, 7, C_RED); gfx_circ(cx, 24, 3, C_YELLOW);
        gfx_rect(cx - 22, 40, 44, 4, C_GREY);
        for (int x = cx - 20; x < cx + 20; x += 8) gfx_circ(x + 4, 44, 3, C_SLATE);
        for (int k = 0; k < 4; k++) gfx_line(cx - 22 + k * 3, 44, cx - 24 + k * 6, 48, C_LIGHT); /* the cow-catcher */
        gfx_circ(cx - 13, 0, 4, C_GREY);
        eyes(cx, 16, 12, C_RED);
        break;
    case A_GOVERNOR:
        gfx_rect(cx - 14, 16, 28, 26, C_NAVY); gfx_rect(cx - 4, 16, 8, 16, C_WHITE); gfx_rect(cx - 1, 18, 2, 12, C_RED);
        gfx_circ(cx, 10, 9, C_CREAM);
        gfx_rect(cx - 10, -2, 20, 5, C_INK); gfx_rect(cx - 13, 3, 26, 2, C_INK);
        eyes(cx, 9, 3, C_INK); gfx_hline(cx - 7, cx + 7, 14, C_RED); gfx_pset(cx - 7, 13, C_RED); gfx_pset(cx + 7, 13, C_RED);
        gfx_rect(cx - 22, 30, 12, 16, C_CREAM); gfx_rect(cx + 10, 30, 12, 16, C_CREAM);
        gfx_rect(cx - 10, 42, 8, 6, C_INK); gfx_rect(cx + 2, 42, 8, 6, C_INK);
        break;
    case A_NADIR:
        gfx_rect(cx - 14, 16, 28, 28, C_MAROON); gfx_circ(cx, 12, 10, C_RED);
        gfx_line(cx - 8, 4, cx - 20, -6, C_INK); gfx_line(cx + 8, 4, cx + 20, -6, C_INK);
        gfx_line(cx - 7, 5, cx - 18, -4, C_INK); gfx_line(cx + 7, 5, cx + 18, -4, C_INK);
        eyes(cx, 10, 4, C_YELLOW); gfx_hline(cx - 6, cx + 6, 17, C_INK);
        for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 5; k++) gfx_line(cx + s * 14, 20 + k * 2, cx + s * 24, 10 + k * 5, C_INK);
        gfx_rect(cx - 6, 26, 12, 8, C_WHITE); gfx_rectb(cx - 6, 26, 12, 8, C_INK); gfx_pset(cx, 30, C_RED); /* a card */
        gfx_rect(cx - 12, 44, 8, 4, C_INK); gfx_rect(cx + 4, 44, 8, 4, C_INK);
        break;
    case A_ZENITH:
        gfx_rect(cx - 12, 16, 24, 30, C_WHITE); gfx_circ(cx, 11, 9, C_CREAM);
        gfx_circb(cx, 0, 11, C_YELLOW); gfx_circb(cx, 0, 10, C_AMBER);
        for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 6; k++) gfx_line(cx + s * 12, 18 + k * 2, cx + s * 24, 4 + k * 6, k % 2 ? C_ICE : C_WHITE);
        gfx_circ(cx - 4, 10, 2, C_WHITE); gfx_circ(cx + 4, 10, 2, C_WHITE); gfx_pset(cx - 4, 10, C_SKY); gfx_pset(cx + 4, 10, C_SKY);
        gfx_hline(cx - 4, cx + 4, 16, C_AMBER);
        gfx_rect(cx - 6, 26, 12, 8, C_WHITE); gfx_rectb(cx - 6, 26, 12, 8, C_YELLOW); gfx_pset(cx, 30, C_INK); /* his card */
        gfx_rect(cx - 12, 40, 24, 6, C_YELLOW);
        break;
    default: gfx_rect(0, 0, W, H, C_MAGENTA); break;
    }
}

static int foe_size(int art) {
    if (art >= A_PUCKKING) return art == A_ARM ? 32 : 48;
    switch (art) {
    case A_GOLEM: case A_APE: case A_RIDER: case A_TROOPER: case A_BISON: case A_MOTH: case A_CENTI: case A_GATOR:
    case A_HAG: case A_CANNON: return 32;
    default: return 24;
    }
}

int spur_enemy_w(int e) { return foe_size(SPUR_ENEMY[e].art); }
int spur_enemy_h(int e) { return foe_size(SPUR_ENEMY[e].art); }

void spur_draw_enemy(int e, int x, int y, int flash) {
    const SpurEnemyDef *d = &SPUR_ENEMY[e];
    const Sprite *s = &foes[d->art];
    if (flash && (flash / 2) % 2) { spr_draw_ex(s, x, y, 0, NULL, C_WHITE); return; }
    spr_draw_outline(s, x, y, 0, C_INK);
    spr_draw_ex(s, x, y, 0, remaps[d->pal % 5], -1);
}

/* a boss on the map: its battle picture at half size */
static void draw_boss_small(int art, int x, int y) {
    const Sprite *s = &foes[art];
    for (int sy = 0; sy < s->h; sy += 2)
        for (int sx = 0; sx < s->w; sx += 2) {
            uint8_t c = s->px[sy * s->w + sx];
            if (c != TRANSPARENT) gfx_pset(x + sx / 2, y + sy / 2, c);
        }
}

/* ---- drawing people ----------------------------------------------------------------------- */
static int view_of(int dir) { return dir == 0 ? 0 : dir == 1 ? 1 : 2; }

void spur_draw_person(int look, int pal, int dir, int frame, int x, int y) {
    if (look == LOOK_BOSS) { draw_boss_small(pal, x - 4, y - 8); return; }
    if (look == LOOK_ANIMAL) { spr_draw_outline(&animals[pal % SPUR_ANIMALS], x, y, 0, C_INK); return; }
    if (look == LOOK_COFFIN) { spr_draw_outline(&coffin, x, y + 6, 0, C_INK); return; }
    if (look == LOOK_HORSE) { spr_draw_outline(&horse[0], x - 4, y - 2, dir == 2 ? SPR_FLIPX : 0, C_INK); return; }
    if (look >= LOOK_COUNT) { spur_draw_hero(look - LOOK_COUNT, dir, frame, x, y, false); return; }
    int v = view_of(dir);
    int idx = look;
    const Sprite *s;
    static Sprite cache[LOOK_COUNT][16][3][2];
    static bool made[LOOK_COUNT][16];
    if (look == LOOK_DOG) s = &people[look][v][frame & 1];
    else {
        int p = pal % 16;
        if (!made[idx][p]) {
            Outfit o = outfit_for(look, pal);
            for (int vv = 0; vv < 3; vv++)
                for (int f = 0; f < 2; f++) person_sprite(&cache[idx][p][vv][f], &o, vv, f, -1);
            made[idx][p] = true;
        }
        s = &cache[idx][p][v][frame & 1];
    }
    spr_draw_outline(s, x, y, dir == 2 ? SPR_FLIPX : 0, C_INK);
}

void spur_draw_hero(int h, int dir, int frame, int x, int y, bool dead) {
    if (dead) { spr_draw_outline(&coffin, x, y + 6, 0, C_INK); return; }
    const Sprite *s = &people[LOOK_COUNT + h][view_of(dir)][frame & 1];
    spr_draw_outline(s, x, y, dir == 2 ? SPR_FLIPX : 0, C_INK);
}

void spur_draw_hero_big(int h, int pose, int x, int y, bool dead) {
    if (dead) {
        spr_draw_scaled(&coffin, x, y + 16, 2, 0);
        return;
    }
    const Sprite *s = &people[LOOK_COUNT + h][2][pose == 1 ? 1 : 0];
    /* an ink edge, then the hero twice the size */
    for (int k = 0; k < 4; k++) {
        int dx = (k == 0) - (k == 1), dy = (k == 2) - (k == 3);
        for (int sy = 0; sy < s->h; sy++)
            for (int sx = 0; sx < s->w; sx++)
                if (s->px[sy * s->w + sx] != TRANSPARENT) gfx_rect(x + sx * 2 + dx, y + sy * 2 + dy, 2, 2, C_INK);
    }
    spr_draw_scaled(s, x, y, 2, 0);
    if (pose == 2) gfx_rect(x + 26, y + 16, 6, 2, C_YELLOW); /* the shot */
}

void spur_draw_horse(int x, int y, int frame, bool flying, int dir) {
    const Sprite *s = flying ? &nightmare[frame & 1] : &horse[frame & 1];
    spr_draw_outline(s, x - 4, y, dir == 2 ? SPR_FLIPX : 0, C_INK);
}

void spur_draw_wisp(int x, int y, int t) {
    spr_draw_ex(&wisp[(t / 6) % 3], x, y, 0, NULL, -1);
}

void spur_draw_tile(int t, int x, int y, int mx, int my) {
    if (t < 0 || t >= T_COUNT) return;
    int v;
    switch (t) {
    case T_SEA: case T_RIVER: case T_WATER: case T_LAVA: case T_FIREWALL: case T_BURNING: case T_BARRIER: case T_PORTAL:
    case T_ORE: case T_HERB: case T_TORCH: case T_CAULDRON:
        v = (sg.ani / 24 + mx + my) & 1;
        break;
    default: v = (int)(h3(mx, my, 5) & 1); break;
    }
    if (t == T_SPIKE) spr_draw(&tiles[T_SAND][v], x, y, 0);
    if (t == T_CHEST) {
        for (int i = 0; i < sg.map.nchest; i++)
            if (sg.map.chest[i].x == mx && sg.map.chest[i].y == my && spur_chest_open(sg.map.chest[i].id)) {
                spr_draw(&chest_open, x, y, 0);
                return;
            }
    }
    /* trees in the desert towns are cacti */
    if (t == T_TREE && (sg.map_id == M_DOLORES || sg.map_id == M_SHRINE)) { spr_draw(&tiles[T_CACTUS][v], x, y, 0); return; }
    spr_draw(&tiles[t][v], x, y, 0);
}

/* ---- loading ------------------------------------------------------------------------------- */
void spur_art_load(void) {
    if (tiles[T_GRASS][0].px) return;
    make_remaps();
    for (int t = 0; t < T_COUNT; t++)
        for (int v = 0; v < 2; v++) {
            begin(16, 16);
            paint_tile(t, v);
            gfx_set_target(&canvas);
            end(&tiles[t][v]);
        }
    begin(16, 16);
    dirt(0);
    gfx_rect(2, 5, 12, 9, C_BROWN);
    gfx_rect(3, 6, 10, 4, C_INK);
    gfx_rect(2, 2, 12, 3, C_EARTH);
    gfx_rectb(2, 5, 12, 9, C_INK);
    end(&chest_open);
    for (int a = 0; a <= A_ZENITH; a++) {
        int sz = foe_size(a);
        begin(sz, sz);
        paint_foe(a);
        gfx_set_target(&canvas);
        end(&foes[a]);
    }
    for (int h = 0; h < H_COUNT; h++)
        for (int v = 0; v < 3; v++)
            for (int f = 0; f < 2; f++) {
                if (h == H_BISCUIT) dog_sprite(&people[LOOK_COUNT + h][v][f], v, f, C_TAN, C_WHITE, true);
                else {
                    Outfit o = hero_outfit(h);
                    person_sprite(&people[LOOK_COUNT + h][v][f], &o, v, f, h);
                }
            }
    for (int v = 0; v < 3; v++)
        for (int f = 0; f < 2; f++) dog_sprite(&people[LOOK_DOG][v][f], v, f, C_BROWN, C_TAN, false);
    for (int a = 0; a < SPUR_ANIMALS; a++) animal_sprite(&animals[a], a);
    for (int f = 0; f < 2; f++) {
        horse_sprite(&horse[f], f, C_BROWN, C_INK, false);
        horse_sprite(&nightmare[f], f, C_INK, C_PURPLE, true);
    }
    begin(16, 9);
    gfx_rect(1, 1, 14, 7, C_BROWN);
    gfx_rectb(1, 1, 14, 7, C_EARTH);
    gfx_vline(5, 2, 6, C_EARTH);
    gfx_hline(3, 7, 4, C_TAN);
    end(&coffin);
    for (int f = 0; f < 3; f++) {
        begin(16, 16);
        gfx_circ(8, 11, 5, C_NAVY);
        gfx_circ(8, 10, 4, C_BLUE);
        gfx_line(8, 2 + f, 5 + f, 8, C_SKY);
        gfx_line(8, 1 + (2 - f), 11 - f, 8, C_SKY);
        gfx_circ(8, 10, 2, C_ICE);
        gfx_pset(8, 9, C_WHITE);
        gfx_pset(4 + f * 3, 4 + f, C_CYAN);
        end(&wisp[f]);
    }
}

/* tests: every picture made, none blank */
int spur_art_check(void) {
    int bad = 0;
    for (int t = 0; t < T_COUNT; t++) if (!tiles[t][0].px) bad++;
    for (int a = 0; a <= A_ZENITH; a++) {
        if (!foes[a].px) { bad++; continue; }
        int n = 0;
        for (int i = 0; i < foes[a].w * foes[a].h; i++) n += foes[a].px[i] != TRANSPARENT;
        if (n < 40) { bad++; fprintf(stderr, "foe art %d nearly blank\n", a); }
    }
    for (int e = 1; e < SPUR_ENEMIES; e++) if (SPUR_ENEMY[e].art > A_ZENITH) bad++;
    return bad;
}
