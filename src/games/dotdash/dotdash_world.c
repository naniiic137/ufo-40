/* DOT & DASH - the world: the lumber room, the places inside things, and the
 * micro and deep strips that every tile holds. All layouts are our own. */
#include "dotdash.h"

/* ------------------------------------------------------------------ */
/* tiles                                                               */

#define S TF_SOLID
#define O TF_ONEWAY
const TileInfo DD_TILE[T_COUNT] = {
    [T_AIR] = {0, M_NONE, C_INK, C_INK, C_INK},
    [T_WOOD] = {S, M_WOOD, C_BROWN, C_TAN, C_EARTH},
    [T_WOODDK] = {S, M_WOOD, C_EARTH, C_BROWN, C_NIGHT},
    [T_PLANK] = {S, M_WOOD, C_TAN, C_CREAM, C_BROWN},
    [T_WALL] = {S, M_WALL, C_GREY, C_LIGHT, C_SLATE},
    [T_CERAMIC] = {S, M_CERAMIC, C_ORANGE, C_AMBER, C_MAROON},
    [T_SOIL] = {S, M_SOIL, C_EARTH, C_BROWN, C_NIGHT},
    [T_FABRIC] = {S, M_FABRIC, C_WINE, C_RED, C_MAROON},
    [T_FABRIC2] = {S, M_FABRIC, C_RED, C_ORANGE, C_WINE},
    [T_BOOKR] = {S, M_PAPER, C_RED, C_ORANGE, C_MAROON},
    [T_BOOKB] = {S, M_PAPER, C_BLUE, C_SKY, C_NAVY},
    [T_BOOKG] = {S, M_PAPER, C_FOREST, C_LEAF, C_TEAL},
    [T_BOOKY] = {S, M_PAPER, C_AMBER, C_YELLOW, C_ORANGE},
    [T_BOOKW] = {S, M_PAPER, C_CREAM, C_WHITE, C_TAN},
    [T_METAL] = {S, M_METAL, C_SLATE, C_GREY, C_NIGHT},
    [T_BRASS] = {S, M_METAL, C_AMBER, C_YELLOW, C_BROWN},
    [T_GLASS] = {S, M_GLASS, C_ICE, C_WHITE, C_CYAN},
    [T_DUST] = {S, M_DUST, C_GREY, C_LIGHT, C_SLATE},
    [T_CARD] = {S, M_CARD, C_YELLOW, C_CREAM, C_AMBER},
    [T_CARD2] = {S, M_CARD, C_RED, C_ORANGE, C_WINE},
    [T_FUR] = {S, M_FUR, C_TAN, C_CREAM, C_BROWN},
    [T_CELL] = {S, M_CELL, C_PURPLE, C_VIOLET, C_NIGHT},
    [T_CELL2] = {S, M_CELL, C_TEAL, C_JADE, C_NIGHT},
    [T_STONE] = {S, M_STONE, C_SLATE, C_LIGHT, C_DUSK},
    [T_PENCIL] = {S, M_WOOD, C_YELLOW, C_CREAM, C_ORANGE},
    [T_ROOT] = {S, M_SOIL, C_HIDE, C_TAN, C_EARTH},
    [T_SHADE] = {S, M_GLASS, C_CREAM, C_WHITE, C_TAN},
    [T_WAX] = {S, M_WAX, C_CREAM, C_WHITE, C_AMBER},
    [T_MOSS] = {S, M_PLANT, C_FOREST, C_LEAF, C_TEAL},
    [T_CRACK] = {S | TF_BOMB, M_WOOD, C_BROWN, C_TAN, C_INK},
    [T_GLUE] = {S, M_PLANT, C_LIME, C_WHITE, C_LEAF},
    [T_GEAR] = {S, M_METAL, C_GREY, C_LIGHT, C_SLATE},
    [T_BONE] = {S, M_STONE, C_WHITE, C_WHITE, C_LIGHT},
    [T_LEAF] = {O, M_PLANT, C_LEAF, C_LIME, C_FOREST},
    [T_THREAD] = {O, M_FABRIC, C_LIGHT, C_WHITE, C_GREY},
    [T_LEDGE] = {O, M_WOOD, C_TAN, C_CREAM, C_BROWN},
    [T_SHROOM] = {O | TF_BOUNCE, M_PLANT, C_PINK, C_WHITE, C_MAGENTA},
    [T_STRAND] = {O, M_FUR, C_CREAM, C_WHITE, C_TAN},
    [T_THORN] = {TF_HURT, M_NONE, C_LIGHT, C_WHITE, C_SLATE},
    [T_GOO] = {TF_GOO, M_NONE, C_LIME, C_WHITE, C_LEAF},
    [T_SLIME] = {TF_GOO, M_NONE, C_PINK, C_WHITE, C_MAGENTA},
};
#undef S
#undef O

const char *dd_biome_name(int mat) {
    static const char *N[M_COUNT] = {"", "WOODGRAIN", "PLASTER", "GLAZE", "LOAM", "WEAVE", "PULP", "LATTICE",
                                     "GLOW", "FLUFF", "PASTEBOARD", "FUR", "THE MOTES", "GRIT", "GREENERY", "WAX"};
    return mat >= 0 && mat < M_COUNT ? N[mat] : "";
}

uint32_t dd_hash(uint32_t a, uint32_t b) {
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u + (a << 6) + (a >> 2));
    h ^= h >> 16; h *= 0x85EBCA6Bu; h ^= h >> 13; h *= 0xC2B2AE35u; h ^= h >> 16;
    return h;
}

int dd_level_material(const Level *L, int x, int y) { return DD_TILE[lv_tile(L, x, y)].mat; }

/* ------------------------------------------------------------------ */
/* the level currently being built                                     */

Level dd_lv;
int dd_scale;
LevelDesc dd_stack[DEPTH_MAX];
int dd_depth;

static Level *B;
static bool spawning;
static int SPX, SPY; /* arrival pixel, to keep creatures off it */

static void fill(int x0, int y0, int x1, int y1, int t) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) lv_set(B, x, y, t);
}
static void bgf(int x0, int y0, int x1, int y1, int b) {
    for (int y = imax(y0, 0); y <= y1 && y < B->h; y++)
        for (int x = imax(x0, 0); x <= x1 && x < B->w; x++) B->bg[y * B->w + x] = (uint8_t)b;
}
static int tile_at(int x, int y) { return lv_tile(B, x, y); }

/* ---- entity placement (only while spawning) ---- */
static uint32_t place_key; /* the level or chunk being populated */
static int place_n;

static uint32_t pid_next(void) { return dd_hash(place_key, (uint32_t)(++place_n + 7777)) | 1u; }

/* foot tile tx,ty: the thing stands in tile row ty (on row ty+1) */
static int put(int kind, int sub, int tx, int ty) {
    if (!spawning) return -1;
    int w = 8, h = 8;
    if (kind == EK_FOE) dd_foe_size(sub, &w, &h);
    else if (kind == EK_NPC) { w = 8; h = 14; }
    else if (kind == EK_DOOR) { w = 8; h = 16; }
    else if (kind == EK_PICK) { w = 7; h = 7; }
    int i = dd_add_ent(kind, sub, (float)(tx * DD_TS + (DD_TS - w) / 2), (float)((ty + 1) * DD_TS - h));
    if (i >= 0) { dd_ent[i].w = (int16_t)w; dd_ent[i].h = (int16_t)h; dd_ent[i].home_x = (int16_t)dd_ent[i].x; dd_ent[i].home_y = (int16_t)dd_ent[i].y; }
    return i;
}
static void npc(int sub, int tx, int ty) {
    if (!spawning || !dd_npc_visible(sub)) return;
    int i = put(EK_NPC, sub, tx, ty);
    if (i >= 0) dd_ent[i].dir = (uint8_t)(tx * 7 % 2);
}
static void foe(int sub, int tx, int ty) {
    if (!spawning || (dd_no_foes && sub < F_FIRST_BOSS)) return;
    if (iabs(tx * DD_TS - SPX) < 6 * DD_TS && iabs(ty * DD_TS - SPY) < 6 * DD_TS && sub < F_FIRST_BOSS) return;
    put(EK_FOE, sub, tx, ty);
}
static void pick(int sub, int tx, int ty, int param) {
    if (!spawning) return;
    uint32_t pid = pid_next();
    if (dd_collected(pid)) return;
    int i = put(EK_PICK, sub, tx, ty);
    if (i >= 0) { dd_ent[i].pid = pid; dd_ent[i].param = param; }
}
/* a spot holding an upgrade of some kind: it gives the next level of that
 * line (or the next heart button or pep egg); once the kind is full, the
 * spot holds glints instead */
static void upgrade_copy(int u, int tx, int ty, bool boxed) {
    if (!spawning) return;
    uint32_t pid = pid_next();
    if (dd_collected(pid)) return;
    int r = dd_resolve_upgrade(u);
    int i;
    if (r < 0) i = put(EK_PICK, P_GLINT5, tx, ty);
    else if (boxed) i = put(EK_OBJ, O_GIFTBOX, tx, ty);
    else i = put(EK_PICK, P_UPGRADE, tx, ty);
    if (i >= 0) { dd_ent[i].pid = pid; dd_ent[i].param = boxed ? u : r; }
}
static void obj(int sub, int tx, int ty) {
    if (!spawning) return;
    put(EK_OBJ, sub, tx, ty);
}
static void door(int id, int tx, int ty) {
    if (!spawning) return;
    int i = put(EK_DOOR, 0, tx, ty);
    if (i >= 0) dd_ent[i].param = id;
}
static void glints_row(int tx, int ty, int n) {
    for (int k = 0; k < n; k++) pick(P_GLINT1, tx + k, ty, 0);
}

/* ------------------------------------------------------------------ */
/* doors between the room and the places inside things                  */

/* door ids: where each one leads */
enum {
    DR_THIMBLE_IN, DR_THIMBLE_OUT, DR_OUTLETW_IN, DR_OUTLETW_OUT, DR_OUTLETE_IN, DR_OUTLETE_OUT,
    DR_HOLLOW_IN, DR_HOLLOW_OUT, DR_SIEGE_IN, DR_SIEGE_OUT, DR_CLOCK_IN, DR_CLOCK_OUT, DR_LAIR_IN,
    DR_LAIR_OUT, DR_THRONE_IN, DR_THRONE_OUT, DR_SHAFT_IN, DR_SHAFT_TOP, DR_SHAFT_OUT, DR_TRAIN_IN, DR_TRAIN_OUT,
    /* not doors but spots: seams to shrink into, and things to press UP at */
    DR_SEAM_KEYHOLE, DR_SEAM_SUMMIT, DR_OWL, DR_CONSOLE, DR_COUNT
};
typedef struct DoorDef { int area, tx, ty; } DoorDef;
/* where you arrive through each door */
static const DoorDef DOOR_TO[DR_COUNT] = {
    [DR_THIMBLE_IN] = {AR_THIMBLE, 4, 16}, [DR_THIMBLE_OUT] = {AR_ROOM, 18, 82},
    [DR_OUTLETW_IN] = {AR_CAVITY, 5, 30}, [DR_OUTLETW_OUT] = {AR_ROOM, 80, 83},
    [DR_OUTLETE_IN] = {AR_CAVITY, 114, 30}, [DR_OUTLETE_OUT] = {AR_ROOM, 153, 83},
    [DR_HOLLOW_IN] = {AR_HOLLOW, 4, 16}, [DR_HOLLOW_OUT] = {AR_ROOM, 138, 79},
    [DR_SIEGE_IN] = {AR_SIEGE, 4, 19}, [DR_SIEGE_OUT] = {AR_ROOM, 132, 41},
    [DR_CLOCK_IN] = {AR_CLOCKWORKS, 4, 23}, [DR_CLOCK_OUT] = {AR_ROOM, 73, 26},
    [DR_LAIR_IN] = {AR_LAIR, 4, 18}, [DR_LAIR_OUT] = {AR_CAVITY, 101, 30},
    [DR_THRONE_IN] = {AR_THRONE, 4, 19}, [DR_THRONE_OUT] = {AR_ROOM, 28, 68},
    [DR_SHAFT_IN] = {AR_SHAFT, 33, 9}, [DR_SHAFT_TOP] = {AR_ROOM, 57, 29}, [DR_SHAFT_OUT] = {AR_ROOM, 57, 82},
    [DR_TRAIN_IN] = {AR_TRAIN, 34, 16}, [DR_TRAIN_OUT] = {AR_ROOM, 139, 70},
};
int dd_door_target(int id, int *area, int *tx, int *ty) {
    if (id < 0 || id >= DR_SEAM_KEYHOLE) return 0;
    *area = DOOR_TO[id].area; *tx = DOOR_TO[id].tx; *ty = DOOR_TO[id].ty;
    return 1;
}
bool dd_door_is(int id, const char *what) {
    if (!strcmp(what, "seam")) return id == DR_SEAM_KEYHOLE || id == DR_SEAM_SUMMIT;
    if (!strcmp(what, "keyhole")) return id == DR_SEAM_KEYHOLE;
    if (!strcmp(what, "summit")) return id == DR_SEAM_SUMMIT;
    if (!strcmp(what, "owl")) return id == DR_OWL;
    if (!strcmp(what, "console")) return id == DR_CONSOLE;
    if (!strcmp(what, "fried")) return id == DR_OUTLETE_IN;
    if (!strcmp(what, "lair")) return id == DR_LAIR_IN;
    if (!strcmp(what, "throne")) return id == DR_THRONE_IN;
    if (!strcmp(what, "train")) return id == DR_TRAIN_IN;
    return false;
}

/* ------------------------------------------------------------------ */
/* the lumber room (area 0): 160 x 90 tiles, the room at full size is the
 * same map drawn at 2 px a tile                                           */

static void pencil_ramp(void) {
    for (int i = 0; i <= 20; i++) {
        fill(84 + i, 83 - i, 84 + i, 84 - i, T_PENCIL);
    }
}

static void room_tiles(void) {
    bgf(0, 0, ROOM_W - 1, ROOM_H - 1, BG_PAPER);
    fill(0, 0, ROOM_W - 1, 2, T_WALL);
    fill(0, 0, 4, ROOM_H - 1, T_WALL);
    fill(155, 0, 159, ROOM_H - 1, T_WALL);
    for (int x = 5; x <= 154; x++) fill(x, 84, x, 89, (x / 9) % 2 ? T_WOOD : T_WOODDK);
    /* the door, its lintel (the door top) and latch */
    bgf(7, 20, 35, 83, BG_DOOR);
    fill(5, 18, 37, 19, T_WOODDK);
    /* the door knob, just higher than Dot's head at full size: Latchtown sits on it */
    fill(27, 69, 29, 70, T_BRASS);
    /* the spider's web in the top corner over the door */
    fill(5, 8, 10, 8, T_THREAD);
    /* the rug */
    for (int x = 8; x <= 64; x++) lv_set(B, x, 83, (x / 4) % 2 ? T_FABRIC : T_FABRIC2);
    /* Granny Thimble's thimble */
    fill(11, 78, 16, 82, T_BRASS);
    /* the west pot and its plant */
    fill(36, 82, 54, 82, T_CERAMIC);
    fill(37, 62, 39, 63, T_CERAMIC);
    fill(51, 62, 53, 63, T_CERAMIC);
    fill(38, 64, 39, 81, T_CERAMIC);
    fill(51, 64, 52, 81, T_CERAMIC);
    fill(38, 80, 52, 81, T_CERAMIC);
    fill(40, 66, 50, 79, T_SOIL);
    static const int IVY[] = {80, 77, 74, 71, 68, 65};
    for (int k = 0; k < 6; k++) {
        /* ivy up the pots' sides, its leaves staggered so it can be climbed
         * down as well as up */
        int a = (k % 2) * 2;
        fill(53 + a, IVY[k], 54 + a, IVY[k], T_LEAF);
        fill(34 + a, IVY[k], 35 + a, IVY[k], T_LEAF);
        fill(122 + a, IVY[k] + 1, 123 + a, IVY[k] + 1, T_LEAF); /* the east pot's ivy */
    }
    bgf(45, 30, 45, 65, BG_STALK);
    for (int k = 0, y = 63; y >= 33; k++, y -= 3) {
        if (k % 2 == 0) fill(41, y, 44, y, T_LEAF);
        else fill(46, y, 49, y, T_LEAF);
    }
    fill(45, 30, 53, 30, T_LEAF);
    /* the plant's topmost shoots reach over towards the door's lintel */
    fill(39, 27, 41, 27, T_LEAF);
    fill(38, 24, 40, 24, T_LEAF);
    fill(38, 21, 40, 21, T_LEAF);
    /* the hanging shelf, the clock, a matchbox and the portrait */
    fill(56, 30, 93, 30, T_PLANK);
    bgf(58, 31, 58, 34, BG_BRACKET);
    bgf(91, 31, 91, 34, BG_BRACKET);
    fill(60, 17, 71, 29, T_WOODDK);
    /* pendulum weights hanging down the left side of the clock */
    fill(58, 27, 59, 27, T_LEDGE);
    fill(58, 24, 59, 24, T_LEDGE);
    fill(58, 21, 59, 21, T_LEDGE);
    bgf(62, 19, 69, 26, BG_CLOCK);
    fill(73, 27, 78, 29, T_CARD2);
    fill(76, 24, 79, 26, T_BOOKG);
    fill(72, 24, 74, 24, T_LEDGE);
    fill(72, 21, 74, 21, T_LEDGE);
    fill(80, 20, 88, 29, T_WOOD);
    bgf(81, 21, 87, 28, BG_FRAME);
    /* the reading lamp at the end of the shelf: Glimmer lives on its shade */
    fill(90, 29, 92, 29, T_BRASS);
    fill(91, 25, 91, 28, T_BRASS);
    fill(89, 22, 94, 24, T_SHADE);
    bgf(92, 25, 93, 26, BG_BULB);
    /* the money box (a tin robot bank), its crank and the coin pile */
    fill(60, 73, 70, 83, T_METAL);
    fill(58, 76, 59, 77, T_METAL);
    fill(71, 77, 71, 77, T_LEDGE);
    fill(72, 83, 77, 83, T_BRASS);
    fill(72, 82, 75, 82, T_BRASS);
    fill(72, 81, 73, 81, T_BRASS);
    /* the west outlet (on the wall above the floor) */
    bgf(78, 79, 81, 83, BG_OUTLET);
    /* the leaning pencil up to the east pot */
    pencil_ramp();
    /* the east pot and its plant */
    fill(104, 83, 122, 83, T_CERAMIC);
    fill(105, 63, 107, 64, T_CERAMIC);
    fill(119, 63, 121, 64, T_CERAMIC);
    fill(106, 65, 107, 81, T_CERAMIC);
    fill(119, 65, 120, 81, T_CERAMIC);
    fill(106, 80, 120, 82, T_CERAMIC);
    fill(108, 67, 118, 79, T_SOIL);
    bgf(113, 36, 113, 66, BG_STALK);
    for (int k = 0, y = 64; y >= 37; k++, y -= 3) {
        if (k % 2 == 0) fill(109, y, 112, y, T_LEAF);
        else fill(114, y, 117, y, T_LEAF);
    }
    /* the lamp hanging from the ceiling */
    bgf(116, 3, 116, 17, BG_CORD);
    fill(114, 18, 118, 18, T_SHADE);
    fill(113, 19, 119, 19, T_SHADE);
    fill(112, 20, 120, 21, T_SHADE);
    bgf(115, 22, 117, 24, BG_BULB);
    /* the bookshelf: four compartments, each board with a gap closed by a
     * one-way ledge you can jump up through (and drop down with a bangle) */
    fill(126, 10, 127, 73, T_WOOD);
    /* screws in the bookshelf's side, a staggered ladder down its outside */
    for (int k = 0, y = 14; y <= 62; k++, y += 4) fill(k % 2 ? 124 : 122, y, k % 2 ? 125 : 123, y, T_LEDGE);
    fill(151, 10, 152, 75, T_WOOD);
    fill(124, 10, 154, 11, T_PLANK);
    fill(128, 10, 129, 10, T_LEDGE);
    fill(128, 11, 129, 11, T_AIR);
    fill(128, 26, 147, 26, T_PLANK);
    fill(148, 26, 150, 26, T_LEDGE);
    fill(131, 42, 150, 42, T_PLANK);
    fill(128, 42, 130, 42, T_LEDGE);
    fill(128, 58, 147, 58, T_PLANK);
    fill(148, 58, 150, 58, T_LEDGE);
    fill(126, 74, 127, 75, T_PLANK);
    fill(131, 74, 152, 75, T_PLANK);
    fill(128, 75, 130, 75, T_LEDGE);
    fill(126, 82, 127, 83, T_WOOD);
    fill(151, 82, 152, 83, T_WOOD);
    /* compartment A (top): books climbing left to the gap in the top board */
    fill(144, 23, 147, 25, T_BOOKG);
    fill(140, 20, 143, 25, T_BOOKY);
    fill(136, 17, 139, 25, T_BOOKR);
    fill(128, 14, 135, 25, T_BOOKB);
    /* compartment B: the SIEGE board game box and two books */
    fill(134, 36, 145, 41, T_CARD);
    fill(131, 39, 132, 41, T_BOOKY);
    fill(146, 33, 147, 41, T_BOOKR);
    fill(148, 29, 150, 41, T_BOOKG);
    /* compartment C: a staircase of books to the left-hand gap */
    fill(144, 55, 146, 57, T_BOOKY);
    fill(141, 52, 143, 57, T_BOOKB);
    fill(138, 49, 140, 57, T_BOOKR);
    fill(135, 46, 137, 57, T_BOOKG);
    fill(128, 45, 134, 57, T_BOOKW);
    /* compartment D (bottom): the toy train and the stone owl bookend */
    fill(131, 71, 146, 73, T_METAL);
    fill(142, 68, 145, 70, T_METAL);
    fill(134, 67, 135, 70, T_METAL);
    fill(147, 64, 150, 73, T_STONE);
    fill(149, 61, 150, 61, T_LEDGE);
    /* on top of the bookshelf: a pencil and a spool of blue thread */
    fill(131, 8, 144, 9, T_PENCIL);
    fill(146, 6, 149, 9, T_FABRIC2);
    /* the dust pile under the bookshelf, with a hump under the board's gap */
    fill(130, 80, 146, 83, T_DUST);
    fill(128, 79, 130, 79, T_DUST);
    /* the fried outlet behind the bookshelf */
    bgf(153, 79, 154, 83, BG_FRIED);
}

static int put_ferry(int ax, int ay, int bx, int by);

static void room_ents(void) {
    door(DR_THIMBLE_IN, 17, 82);
    door(DR_OUTLETW_IN, 79, 83);
    door(DR_OUTLETE_IN, 153, 83);
    door(DR_HOLLOW_IN, 138, 79);
    door(DR_SIEGE_IN, 133, 41);
    door(DR_CLOCK_IN, 72, 29);
    door(DR_OWL, 148, 63);
    door(DR_THRONE_IN, 29, 68);
    door(DR_SHAFT_IN, 57, 29);
    door(DR_TRAIN_IN, 139, 70);
    door(DR_SEAM_SUMMIT, 118, 17);
    npc(N_CRUMB, 47, 65);
    npc(N_VOLT, 83, 83);
    npc(N_INKY, 137, 16);
    npc(N_WEEPY, 119, 62);
    npc(N_SILK, 7, 7);
    npc(N_FILAMENT, 93, 21);
    upgrade_copy(U_MUSK, 84, 19, true);
    upgrade_copy(U_TOP1, 134, 66, true);
    if (spawning) {
        /* things that come back while their quests are open */
        if (!dd_flag(FL_WEEPY_DONE) && !dd_obj_exists(O_THREAD, -1)) obj(O_THREAD, 147, 5);
        static const int TW[3][2] = {{110, 51}, {115, 42}, {110, 39}};
        for (int k = 0; k < 3; k++)
            if (!dd_flag(FL_TWIG_DONE) && !((dd_sv.counts[QC_TWIGS] >> k) & 1) && !dd_obj_exists(O_TWIG, k)) {
                int i = put(EK_OBJ, O_TWIG, TW[k][0], TW[k][1]);
                if (i >= 0) dd_ent[i].param = k;
            }
        int d = dd_add_ent(EK_DRIP, 0, 108 * DD_TS + 3, 3 * DD_TS);
        (void)d;
        /* the flyports: a fly shuttling between the two pots, and one
         * between the reading lamp and the great lamp */
        put_ferry(54, 29, 107, 39);
        put_ferry(95, 21, 116, 17);
        /* a wall-climbing bug on the lintel, the way up to the web */
        int cl = put(EK_FOE, F_CLIMBER, 22, 17);
        if (cl >= 0) dd_ent[cl].dir = 0;
    }
    foe(F_BEETLE, 116, 66);
    foe(F_MOTH, 39, 57);
    foe(F_ANT, 96, 83);
    foe(F_ANT, 30, 82);
    foe(F_SPRING, 140, 57);
    foe(F_SPRING, 138, 25);
    foe(F_BUMBLE, 50, 45);
    foe(F_SPRING, 70, 29);
    foe(F_ANT, 64, 72);
    glints_row(20, 82, 3);
    glints_row(66, 72, 3);
    glints_row(46, 29, 4);
    glints_row(132, 7, 3);
    glints_row(88, 29, 3);
    glints_row(132, 73, 3);
    glints_row(108, 62, 4);
    glints_row(20, 17, 4);
    pick(P_GLINT5, 131, 44, 0);
    pick(P_GLINT5, 62, 16, 0);
    pick(P_GLINT5, 153, 9, 0);
    pick(P_GLINT5, 35, 17, 0);
    if (spawning && (dd_sv.glints_total >= 333 || dd_flag(FL_MET_QUEEN))) {
        foe(F_LANCER, 100, 83);
        foe(F_LANCER, 25, 82);
        foe(F_LANCER, 130, 41);
    }
    if (spawning && dd_sv.glints_total >= 667) {
        foe(F_TINMOUSE, 90, 83);
        foe(F_TINMOUSE, 140, 73);
    }
}

/* a ferry fly: waits at A, climbs to the cruising height, flies over to B,
 * comes down, waits, and back (foot tiles like put) */
static int put_ferry(int ax, int ay, int bx, int by) {
    int i = put(EK_FOE, F_FERRY, ax, ay);
    if (i < 0) return -1;
    Ent *e = &dd_ent[i];
    e->param = bx * DD_TS + (DD_TS - e->w) / 2;
    e->param2 = (by + 1) * DD_TS - e->h;
    e->st = 0;
    e->state = 0;
    return i;
}

/* ------------------------------------------------------------------ */
/* the places inside things (S1)                                        */

static void frame_box(int w, int h, int t, int floor_row) {
    fill(0, 0, w - 1, 1, t);
    fill(0, 0, 1, h - 1, t);
    fill(w - 2, 0, w - 1, h - 1, t);
    fill(0, floor_row, w - 1, h - 1, t);
}

static void area_thimble(void) {
    if (!spawning) {
        bgf(0, 0, 39, 19, BG_HOLLOW);
        frame_box(40, 20, T_BRASS, 17);
        fill(27, 13, 31, 13, T_LEDGE);
        fill(8, 14, 11, 14, T_LEDGE);
        fill(33, 15, 37, 16, T_WOOD);
        return;
    }
    door(DR_THIMBLE_OUT, 3, 16);
    npc(N_GRANNY, 21, 16);
    if (!dd_flag(FL_SPECS_GIVEN) && !dd_obj_exists(O_SPECS, -1)) obj(O_SPECS, 29, 12);
    if (!dd_flag(FL_CATFOOD_DONE) && !dd_obj_exists(O_CATFOOD, -1)) obj(O_CATFOOD, 35, 14);
    glints_row(8, 13, 3);
    pick(P_GLINT5, 30, 12, 0);
}

static void area_hollow(void) {
    if (!spawning) {
        bgf(0, 0, 49, 19, BG_HOLLOW);
        frame_box(50, 20, T_DUST, 17);
        fill(28, 13, 33, 13, T_THREAD);
        fill(8, 13, 12, 13, T_THREAD);
        return;
    }
    door(DR_HOLLOW_OUT, 3, 16);
    npc(N_MOTHERFLUFF, 12, 16);
    for (int k = 0; k < 3; k++) {
        int i = put(EK_STAND, k, 16 + k * 3, 16);
        if (i >= 0) dd_ent[i].param = N_MOTHERFLUFF;
    }
    npc(N_TUFTY, 30, 16);
    npc(N_PUFFIN, 40, 16);
    glints_row(29, 12, 4);
    glints_row(8, 12, 3);
}

static void area_clockworks(void) {
    if (!spawning) {
        bgf(0, 0, 39, 25, BG_DARK);
        frame_box(40, 26, T_WOODDK, 24);
        fill(8, 20, 13, 20, T_GEAR);
        fill(16, 16, 21, 16, T_GEAR);
        fill(24, 12, 29, 12, T_GEAR);
        fill(14, 8, 26, 8, T_GEAR);
        fill(31, 17, 35, 17, T_GEAR);
        bgf(20, 9, 20, 23, BG_CORD);
        return;
    }
    door(DR_CLOCK_OUT, 3, 23);
    npc(N_TOCK, 20, 7);
    npc(N_OLDCAP, 33, 16);
    glints_row(9, 19, 3);
    glints_row(25, 11, 3);
    pick(P_GLINT5, 16, 7, 0);
}

static void area_lair(void) {
    if (!spawning) {
        bgf(0, 0, 39, 21, BG_DARK);
        frame_box(40, 22, T_WOOD, 19);
        fill(6, 15, 10, 15, T_LEDGE);
        fill(29, 15, 33, 15, T_LEDGE);
        return;
    }
    door(DR_LAIR_OUT, 3, 18);
    if (!dd_flag(FL_EARWIG_DEAD)) foe(F_EARWIG, 26, 18);
    else pick(P_GLINT50, 20, 18, 0);
    {
        /* a pod in the far wall shoots low along the lair: it hits the
         * earwig as well as Dot, who can watch from a ledge */
        int i = put(EK_FOE, F_POD, 37, 17);
        if (i >= 0) dd_ent[i].dir = 0;
    }
}

/* the SIEGE board game: a battlefield printed on a board, dice, walls and
 * towers climbing in steps, tin tanks and paper planes all the way, and the
 * Big Bang waiting at the far end */
static void area_siege(void) {
    if (!spawning) {
        bgf(0, 0, 159, 23, BG_BOX);
        fill(0, 0, 159, 1, T_CARD2);
        fill(0, 0, 1, 23, T_CARD2);
        fill(158, 0, 159, 23, T_CARD2);
        fill(0, 20, 159, 23, T_CARD);
        for (int x = 2; x < 158; x += 8) fill(x, 20, x + 3, 20, T_CARD2);
        fill(18, 16, 22, 19, T_BONE);            /* a die */
        fill(34, 16, 36, 19, T_CARD2);           /* a castle wall */
        fill(40, 12, 42, 19, T_CARD2);           /* a tower */
        fill(46, 15, 51, 15, T_LEDGE);
        fill(56, 16, 60, 19, T_BONE);            /* a die */
        fill(62, 12, 67, 12, T_LEDGE);
        fill(74, 16, 76, 19, T_CARD2);
        fill(78, 12, 80, 19, T_CARD2);           /* the tall tower */
        fill(81, 16, 82, 16, T_LEDGE);           /* a step down its far side */
        fill(82, 8, 86, 8, T_LEDGE);
        fill(90, 16, 94, 19, T_BONE);
        fill(98, 12, 103, 12, T_LEDGE);
        fill(108, 16, 110, 19, T_CARD2);
        fill(116, 16, 119, 19, T_BONE);
        fill(122, 12, 127, 12, T_LEDGE);
        fill(133, 16, 136, 16, T_LEDGE);
        fill(140, 16, 142, 19, T_CARD2);         /* the last wall */
        fill(149, 16, 152, 16, T_LEDGE);
        fill(155, 16, 157, 19, T_CARD2);
        return;
    }
    door(DR_SIEGE_OUT, 3, 19);
    upgrade_copy(U_FIZZ, 20, 15, true);
    upgrade_copy(U_HEART0 + 6, 84, 7, true);
    pick(P_GLINT50, 41, 11, 0);
    pick(P_GLINT50, 100, 11, 0);
    glints_row(46, 14, 5);
    glints_row(62, 11, 5);
    glints_row(122, 11, 5);
    foe(F_ANT, 30, 19);
    foe(F_TANK, 50, 19);
    foe(F_AXEANT, 70, 19);
    foe(F_POD, 79, 11);
    foe(F_TANK, 100, 19);
    foe(F_AXEANT, 114, 19);
    foe(F_SPRING, 120, 15);
    foe(F_TANK, 130, 19);
    foe(F_TANK, 146, 19);
    foe(F_PLANE, 60, 4);
    foe(F_PLANE, 120, 5);
    if (!dd_flag(FL_BIGBANG_DONE) && !dd_obj_exists(O_BIGBANG, -1)) obj(O_BIGBANG, 151, 15);
}

/* between the walls: studs with drilled holes, nogging boards, wires */
static void area_cavity(void) {
    if (!spawning) {
        bgf(0, 0, 119, 33, BG_CAVITY);
        fill(0, 0, 119, 1, T_WALL);
        fill(0, 0, 1, 33, T_WALL);
        fill(118, 0, 119, 33, T_WALL);
        fill(0, 31, 119, 33, T_WOOD);
        static const int STUD[4] = {20, 50, 80, 110};
        for (int k = 0; k < 4; k++) {
            fill(STUD[k], 2, STUD[k] + 3, 26, T_WOOD);
            fill(STUD[k], 27, STUD[k] + 3, 30, T_AIR);
        }
        /* board A between the first two studs, board B (Wormwood) and board C;
         * their ends are one-way where the climbing ledges come up under them */
        fill(24, 24, 46, 24, T_PLANK);
        fill(24, 24, 26, 24, T_LEDGE);
        fill(24, 28, 26, 28, T_LEDGE);
        fill(54, 16, 76, 16, T_PLANK);
        fill(77, 24, 79, 24, T_LEDGE);
        fill(54, 16, 56, 16, T_LEDGE);
        fill(54, 28, 56, 28, T_LEDGE);
        fill(57, 24, 59, 24, T_LEDGE);
        fill(54, 20, 56, 20, T_LEDGE);
        fill(87, 20, 106, 20, T_PLANK);
        fill(87, 20, 89, 20, T_LEDGE);
        fill(84, 28, 86, 28, T_LEDGE);
        fill(87, 24, 89, 24, T_LEDGE);
        /* wires strung between studs */
        fill(4, 22, 19, 22, T_THREAD);
        fill(8, 28, 10, 28, T_LEDGE);
        fill(4, 25, 6, 25, T_LEDGE);
        fill(26, 12, 47, 12, T_THREAD);
        fill(44, 16, 46, 16, T_LEDGE);
        fill(40, 20, 42, 20, T_LEDGE);
        /* the pink slime seep and a cubby walled in with weak, cracked wood,
         * with a crate each side */
        fill(90, 31, 93, 31, T_SLIME);
        fill(98, 26, 107, 26, T_WOOD);
        fill(98, 27, 98, 30, T_CRACK);
        fill(107, 27, 107, 30, T_WOOD);
        fill(95, 30, 96, 30, T_WOOD);
        fill(108, 30, 109, 30, T_WOOD);
        /* a nook in the first stud behind weak, cracked wood */
        fill(21, 18, 22, 21, T_AIR);
        fill(20, 18, 20, 21, T_CRACK);
        return;
    }
    door(DR_OUTLETW_OUT, 3, 30);
    door(DR_OUTLETE_OUT, 116, 30);
    door(DR_LAIR_IN, 112, 30);
    pick(P_GLINT5, 21, 21, 0);
    pick(P_GLINT5, 22, 21, 0);
    for (int k = 0; k < 3; k++) pick(P_GLINT5, 100 + k * 2, 30, 0);
    if (spawning) { int d = dd_add_ent(EK_DRIP, 2, 88 * DD_TS + 2, 2 * DD_TS); (void)d; } /* a pink stalactite over the seep */
    if (!dd_flag(FL_GEAR_DONE) && !dd_obj_exists(O_GEAR, -1) && dd_flag(FL_POWER_OFF)) obj(O_GEAR, 114, 30);
    foe(F_ANT, 40, 30);
    foe(F_SPRING, 70, 30);
    foe(F_AXEANT, 65, 15);
    foe(F_BUMBLE, 30, 8);
    glints_row(8, 21, 4);
    glints_row(30, 23, 4);
    glints_row(60, 15, 4);
    pick(P_GLINT50, 44, 11, 0);
    if (dd_sv.glints_total >= 333 || dd_flag(FL_MET_QUEEN)) { foe(F_LANCER, 75, 30); foe(F_LANCER, 95, 30); }
    if (dd_sv.glints_total >= 667) foe(F_TINMOUSE, 60, 30);
}

/* ------------------------------------------------------------------ */
/* the hand-made micro and deep places                                   */

static void sp_dashfur(void) {
    if (!spawning) {
        fill(0, 20, 59, 23, T_FUR);
        fill(0, 0, 1, 23, T_FUR);
        fill(58, 0, 59, 23, T_FUR);
        for (int x = 4; x < 56; x += 6) {
            int y = 16 - (x / 6) % 3 * 3;
            fill(x, y, x + 3, y, T_STRAND);
        }
        fill(20, 17, 24, 19, T_FUR);
        fill(38, 16, 42, 19, T_FUR);
        return;
    }
    obj(O_POPGUN, 6, 19);
    static const int NX[8] = {9, 14, 18, 27, 32, 36, 46, 52};
    for (int k = 0; k < 8; k++)
        if (!((dd_sv.counts[QC_NIPS] >> k) & 1)) {
            int i = put(EK_FOE, F_NIP, NX[k], 19);
            if (i >= 0) dd_ent[i].param = k;
        }
    glints_row(20, 16, 5);
}

static void sp_pufffur(void) {
    if (!spawning) {
        fill(0, 20, 49, 23, T_DUST);
        fill(0, 0, 1, 23, T_DUST);
        fill(48, 0, 49, 23, T_DUST);
        for (int x = 5; x < 46; x += 7) fill(x, 15 - (x % 3), x + 3, 15 - (x % 3), T_THREAD);
        fill(24, 17, 27, 19, T_DUST);
        return;
    }
    obj(O_POPGUN, 5, 19);
    static const int MX[6] = {9, 16, 23, 30, 37, 43};
    for (int k = 0; k < 6; k++)
        if (!((dd_sv.counts[QC_MITES] >> k) & 1)) {
            int i = put(EK_FOE, F_MITE, MX[k], 19);
            if (i >= 0) dd_ent[i].param = k;
        }
}

/* inside the clockwork knight: a gauntlet to his control console. Its
 * bulkheads leave one-tile gaps only the tiny Dot fits through; a long drop
 * down his chest; a high step to the console room */
static void sp_sprocket(void) {
    if (!spawning) {
        bgf(0, 0, 99, 39, BG_DARK);
        fill(0, 0, 99, 1, T_METAL);
        fill(0, 0, 1, 39, T_METAL);
        fill(98, 0, 99, 39, T_METAL);
        fill(0, 36, 99, 39, T_METAL);
        /* the upper deck */
        fill(2, 12, 45, 13, T_METAL);
        fill(12, 12, 15, 12, T_THORN);
        fill(24, 2, 26, 10, T_METAL);            /* bulkhead: a gap at the floor */
        fill(34, 9, 36, 9, T_GEAR);
        /* down the chest: a drop of eighteen tiles beside his breastplate */
        fill(50, 2, 97, 13, T_METAL);
        /* the lower deck */
        fill(2, 30, 97, 35, T_METAL);
        fill(60, 14, 62, 28, T_METAL);           /* bulkhead: a gap at the floor */
        fill(66, 30, 70, 30, T_THORN);
        fill(67, 27, 69, 27, T_GEAR);
        /* the high step to the console room */
        fill(80, 23, 97, 29, T_METAL);
        fill(74, 26, 76, 26, T_LEDGE);
        return;
    }
    foe(F_SPARK, 18, 8);
    foe(F_SPARK, 40, 6);
    foe(F_POD, 58, 29);
    foe(F_SPARK, 72, 20);
    /* a moth to glide down on, if the feather charm is missing */
    put(EK_FOE, F_MOTH, 40, 9);
    glints_row(30, 11, 4);
    glints_row(84, 22, 5);
    obj(O_PEBBLE, 8, 11);
    obj(O_AXE, 54, 29);
    door(DR_CONSOLE, 93, 22);
}

/* inside the keyhole, behind the throne: Nib, who turns the lock */
static void sp_keyhole(void) {
    if (!spawning) {
        frame_box(30, 18, T_BRASS, 15);
        fill(8, 11, 12, 11, T_LEDGE);
        return;
    }
    npc(N_NIB, 18, 14);
    glints_row(8, 10, 5);
}

/* the climb up inside the great lamp's rim to the glow at the top */
static void sp_summit(void) {
    if (!spawning) {
        bgf(0, 0, 39, 47, BG_DARK);
        frame_box(40, 48, T_SHADE, 45);
        static const int L[][3] = {
            {3, 41, 10}, {9, 37, 16}, {15, 33, 22}, {21, 29, 28}, {27, 25, 34}, {21, 21, 28},
            {15, 17, 22}, {9, 13, 16}, {15, 9, 22}, {21, 5, 36},
        };
        for (int k = 0; k < ARRAY_LEN(L); k++) fill(L[k][0], L[k][1], L[k][2], L[k][1], T_LEDGE);
        return;
    }
    npc(N_GLOW, 33, 4);
    glints_row(10, 36, 3);
    glints_row(16, 16, 3);
    foe(F_SPARK, 30, 18);
}

/* inside the mouse's head: the tin mage pulling the strings */
static void sp_shrewhead(void) {
    if (!spawning) {
        bgf(0, 0, 39, 19, BG_DARK);
        frame_box(40, 20, T_FUR, 17);
        fill(10, 13, 14, 13, T_STRAND);
        fill(25, 13, 29, 13, T_STRAND);
        return;
    }
    if (!dd_flag(FL_MAGE_BEATEN)) put(EK_FOE, F_TINMAGE, 28, 16); /* quest creatures always come */
    glints_row(10, 12, 3);
}

/* the ferry fly's cockpit */
static void sp_cockpit(void) {
    if (!spawning) {
        bgf(0, 0, 29, 15, BG_HOLLOW);
        frame_box(30, 16, T_FUR, 13);
        fill(20, 9, 26, 9, T_LEDGE);
        return;
    }
    npc(N_PILOT, 14, 12);
    glints_row(21, 8, 3);
}

/* inside the door knob: the queen's throne room, at small size */
static void area_throne(void) {
    if (!spawning) {
        bgf(0, 0, 59, 23, BG_HOLLOW);
        frame_box(60, 24, T_BRASS, 20);
        fill(26, 17, 33, 19, T_FABRIC);           /* the throne */
        fill(28, 13, 31, 16, T_FABRIC2);
        fill(8, 16, 12, 16, T_LEDGE);
        fill(15, 13, 19, 13, T_LEDGE);
        /* the far corner, walled off until the knight is gone */
        if (!dd_flag(FL_SPROCKET_DEAD)) fill(50, 2, 51, 19, T_METAL);
        fill(52, 19, 57, 19, T_BRASS);
        return;
    }
    door(DR_THRONE_OUT, 3, 19);
    if (!dd_flag(FL_PAID_QUEEN) || dd_flag(FL_SPROCKET_DEAD)) npc(N_TABITHA, 30, 12);
    if (dd_flag(FL_PAID_QUEEN) && !dd_flag(FL_SPROCKET_DEAD)) put(EK_FOE, F_SPROCKET, 40, 19);
    door(DR_SEAM_KEYHOLE, 55, 18);
    glints_row(8, 15, 4);
    glints_row(15, 12, 4);
}

/* a shaft between the walls, from the hanging shelf down to the floor: a
 * long, long drop, and a mouse at the bottom who isn't himself */
static void area_shaft(void) {
    if (!spawning) {
        bgf(0, 0, 39, 63, BG_CAVITY);
        frame_box(40, 64, T_WOOD, 61);
        fill(6, 10, 37, 11, T_PLANK);             /* the top landing */
        fill(6, 30, 11, 30, T_LEDGE);             /* nails sticking out on the way down */
        fill(6, 45, 11, 45, T_LEDGE);
        return;
    }
    door(DR_SHAFT_TOP, 34, 9);
    door(DR_SHAFT_OUT, 36, 60);
    /* moths drift over the landing: hold one and the fall is a glide */
    put(EK_FOE, F_MOTH, 12, 7);
    put(EK_FOE, F_MOTH, 26, 6);
    npc(N_SHREW, 18, 60);
    glints_row(8, 9, 4);
    glints_row(7, 29, 4);
    pick(P_GLINT5, 9, 44, 0);
    foe(F_ANT, 26, 60);
}

/* inside the toy train's caboose: the cowpoke's hideout */
static void area_train(void) {
    if (!spawning) {
        bgf(0, 0, 39, 19, BG_HOLLOW);
        frame_box(40, 20, T_METAL, 17);
        fill(12, 11, 26, 11, T_LEDGE);            /* a luggage rack */
        fill(16, 15, 17, 16, T_METAL);            /* a trunk */
        return;
    }
    door(DR_TRAIN_OUT, 34, 16);
    npc(N_COWPOKE, 4, 16);
    {
        /* the cowpoke's guard gun: its peas break a carried drink */
        int i = put(EK_FOE, F_POD, 9, 16);
        if (i >= 0) dd_ent[i].dir = 1;
    }
    obj(O_CRACKER, 20, 10);
    obj(O_CINDER, 24, 16);
    obj(O_CINDER, 24, 15);
    glints_row(14, 10, 4);
    glints_row(28, 16, 3);
}

const AreaMap DD_AREA[AR_COUNT] = {
    [AR_ROOM] = {"THE LUMBER ROOM", ROOM_W, ROOM_H, NULL, 0, 0, MU_SMALL},
    [AR_THIMBLE] = {"GRANNY THIMBLE'S", 40, 20, NULL, 13, 77, MU_TOWN},
    [AR_CAVITY] = {"BETWEEN THE WALLS", 120, 34, NULL, 80, 83, MU_WALLS},
    [AR_HOLLOW] = {"FLUFF HOLLOW", 50, 20, NULL, 138, 79, MU_TOWN},
    [AR_SIEGE] = {"SIEGE!", 160, 24, NULL, 133, 41, MU_SIEGE},
    [AR_LAIR] = {"THE EARWIG'S LAIR", 40, 22, NULL, 153, 83, MU_BOSS},
    [AR_CLOCKWORKS] = {"THE CLOCKWORKS", 40, 26, NULL, 66, 16, MU_TOWN},
    [AR_THRONE] = {"THE THRONE ROOM", 60, 24, NULL, 28, 68, MU_LATCH},
    [AR_SHAFT] = {"THE LONG DROP", 40, 64, NULL, 57, 82, MU_WALLS},
    [AR_TRAIN] = {"THE CABOOSE", 40, 20, NULL, 139, 70, MU_TOWN},
};
const AreaMap DD_SPECIAL[SP_COUNT] = {
    [SP_DASHFUR] = {"IN DASH'S FUR", 60, 24, NULL, 0, 0, MU_MICRO},
    [SP_PUFFFUR] = {"IN PUFFIN'S FLUFF", 50, 24, NULL, 0, 0, MU_MICRO},
    [SP_SPROCKET] = {"INSIDE SIR SPROCKET", 100, 40, NULL, 0, 0, MU_BOSS},
    [SP_KEYHOLE] = {"THE KEYHOLE", 30, 18, NULL, 0, 0, MU_DEEP},
    [SP_SUMMIT] = {"THE GREAT LAMP'S RIM", 40, 48, NULL, 0, 0, MU_DEEP},
    [SP_SHREWHEAD] = {"INSIDE BARLEY'S HEAD", 40, 20, NULL, 0, 0, MU_BOSS},
    [SP_COCKPIT] = {"THE FERRY'S COCKPIT", 30, 16, NULL, 0, 0, MU_MICRO},
};
/* where Dot arrives in each special (the row she stands in) */
const int DD_SPECIAL_FLOOR[SP_COUNT] = {19, 19, 11, 14, 44, 16, 12};

static void build_area(int id) {
    switch (id) {
    case AR_ROOM: if (!spawning) room_tiles(); else room_ents(); break;
    case AR_THIMBLE: area_thimble(); break;
    case AR_CAVITY: area_cavity(); break;
    case AR_HOLLOW: area_hollow(); break;
    case AR_SIEGE: area_siege(); break;
    case AR_LAIR: area_lair(); break;
    case AR_CLOCKWORKS: area_clockworks(); break;
    case AR_THRONE: area_throne(); break;
    case AR_SHAFT: area_shaft(); break;
    case AR_TRAIN: area_train(); break;
    }
}
static void build_special(int id) {
    switch (id) {
    case SP_DASHFUR: sp_dashfur(); break;
    case SP_PUFFFUR: sp_pufffur(); break;
    case SP_SPROCKET: sp_sprocket(); break;
    case SP_KEYHOLE: sp_keyhole(); break;
    case SP_SUMMIT: sp_summit(); break;
    case SP_SHREWHEAD: sp_shrewhead(); break;
    case SP_COCKPIT: sp_cockpit(); break;
    }
}

/* ------------------------------------------------------------------ */
/* towns and landmarks at micro size: hand-made chunks in the strips     */

enum {
    TW_TUFTVILLE, TW_SNARL, TW_ROTIFER, TW_FERNBY, TW_HOPPER, TW_DEN, TW_OUTPOST, TW_HIVE,
    TW_LOAMTON, TW_SOLDIER, TW_MOLD, TW_REDCAVE, TW_WORKSHOP, TW_SNOUT, TW_CRASH, TW_KITCHEN,
    TW_TICKBURG, TW_GLIMMER, TW_LATCH, TW_WORMWOOD, TW_BLUEDUST, TW_COUNT
};
typedef struct TownDef { const char *name; int area, row, x, n, music; } TownDef;
static const TownDef TOWN[TW_COUNT] = {
    [TW_TUFTVILLE] = {"TUFTVILLE", AR_ROOM, 83, 24, 2, MU_TOWN},
    [TW_SNARL] = {"SNARL", AR_ROOM, 83, 57, 1, MU_MICRO},
    [TW_ROTIFER] = {"THE WHIRLPOOL", AR_ROOM, 66, 41, 1, MU_BOSS},
    [TW_FERNBY] = {"FERNBY", AR_ROOM, 66, 43, 2, MU_TOWN},
    [TW_HOPPER] = {"THE QUIET HOLLOW", AR_ROOM, 66, 46, 1, MU_MICRO},
    [TW_DEN] = {"THE LANCERS' DEN", AR_ROOM, 66, 49, 1, MU_MICRO},
    [TW_OUTPOST] = {"FERNBY OUTPOST", AR_ROOM, 30, 46, 1, MU_MICRO},
    [TW_HIVE] = {"THE HIVE", AR_ROOM, 62, 39, 1, MU_MICRO},
    [TW_LOAMTON] = {"LOAMTON", AR_ROOM, 67, 111, 2, MU_TOWN},
    [TW_SOLDIER] = {"THE DRY PATROL", AR_ROOM, 67, 109, 1, MU_MICRO},
    [TW_MOLD] = {"THE MOLD PATCH", AR_ROOM, 67, 115, 1, MU_MICRO},
    [TW_REDCAVE] = {"THE RED NEST", AR_ROOM, 67, 117, 1, MU_MICRO},
    [TW_WORKSHOP] = {"SPINNER'S WORKSHOP", AR_ROOM, 73, 64, 1, MU_TOWN},
    [TW_SNOUT] = {"THE SNOUT", AR_ROOM, 76, 58, 1, MU_MICRO},
    [TW_CRASH] = {"THE CRASH SITE", AR_ROOM, 81, 72, 1, MU_MICRO},
    [TW_KITCHEN] = {"CHEF MOREL'S KITCHEN", AR_ROOM, 83, 76, 1, MU_TOWN},
    [TW_TICKBURG] = {"TICKBURG", AR_ROOM, 17, 64, 2, MU_TOWN},
    [TW_GLIMMER] = {"GLIMMER", AR_ROOM, 22, 90, 2, MU_TOWN},
    [TW_LATCH] = {"LATCHTOWN", AR_ROOM, 69, 27, 3, MU_LATCH},
    [TW_WORMWOOD] = {"WORMWOOD", AR_CAVITY, 16, 64, 2, MU_TOWN},
    [TW_BLUEDUST] = {"THE BLUE DRIFT", AR_ROOM, 14, 130, 1, MU_MICRO},
};
#define TOWN_H 12   /* town ground row: neighbours meet it here */

/* fixed things in generated micro chunks: upgrade copies, lost waxlings,
 * paperfish, glue blocks, bubbles and the dangerous caves */
enum { X_UP, X_BABY, X_PFISH, X_GLUE, X_BUBBLE, X_DANGER, X_OBJ, X_GERM, X_UPGAP, X_NPC, X_DUMP, X_SCAMPER, X_FACE };
typedef struct Extra { int area, row, x, type, param; } Extra;
static const Extra EXTRA[] = {
    /* the ten dangerous caves, two big glints each */
    {AR_ROOM, 83, 30, X_DANGER, 0}, {AR_ROOM, 66, 47, X_DANGER, 1}, {AR_ROOM, 67, 114, X_DANGER, 2},
    {AR_ROOM, 8, 132, X_DANGER, 3}, {AR_CAVITY, 24, 30, X_DANGER, 4}, {AR_ROOM, 18, 30, X_DANGER, 5},
    {AR_ROOM, 73, 68, X_DANGER, 6}, {AR_ROOM, 80, 134, X_DANGER, 7}, {AR_ROOM, 17, 68, X_DANGER, 8},
    {AR_ROOM, 18, 118, X_DANGER, 9},
    /* whole heart buttons sit in the dangerous caves; half buttons and pep
     * eggs are everywhere. Every spot gives the next one Dot lacks */
    {AR_ROOM, 83, 30, X_UP, U_HEART0}, {AR_ROOM, 67, 114, X_UP, U_HEART0}, {AR_ROOM, 8, 132, X_UP, U_HEART0},
    {AR_CAVITY, 24, 30, X_UP, U_HEART0}, {AR_ROOM, 18, 30, X_UP, U_HEART0}, {AR_ROOM, 73, 68, X_UP, U_HEART0},
    {AR_ROOM, 80, 134, X_UP, U_HEART0}, {AR_ROOM, 17, 68, X_UP, U_HEART0},
    {AR_ROOM, 83, 22, X_UP, U_HEART0 + 6}, {AR_ROOM, 66, 44, X_UP, U_HEART0 + 6}, {AR_ROOM, 62, 40, X_UP, U_HEART0 + 6},
    {AR_ROOM, 71, 140, X_UP, U_HEART0 + 6}, {AR_ROOM, 17, 63, X_UP, U_HEART0 + 6}, {AR_ROOM, 80, 145, X_UP, U_HEART0 + 6},
    {AR_ROOM, 18, 118, X_UP, U_EGG0}, {AR_ROOM, 67, 114, X_UP, U_EGG0}, {AR_ROOM, 66, 47, X_UP, U_EGG0},
    {AR_ROOM, 73, 68, X_UP, U_EGG0}, {AR_ROOM, 83, 20, X_UP, U_EGG0}, {AR_ROOM, 62, 52, X_UP, U_EGG0},
    {AR_ROOM, 49, 139, X_UP, U_EGG0}, {AR_ROOM, 71, 137, X_UP, U_EGG0}, {AR_ROOM, 30, 50, X_UP, U_EGG0},
    {AR_ROOM, 18, 26, X_UP, U_EGG0}, {AR_ROOM, 46, 137, X_UP, U_EGG0}, {AR_ROOM, 14, 134, X_UP, U_EGG0},
    /* the common upgrades, many spots each; a spot gives level one, or
     * level two once Dot has the first */
    {AR_ROOM, 83, 33, X_UP, U_MITT1}, {AR_ROOM, 52, 142, X_UP, U_MITT1}, {AR_CAVITY, 16, 58, X_UP, U_MITT1},
    {AR_ROOM, 17, 69, X_UP, U_MITT1}, {AR_ROOM, 80, 131, X_UPGAP, U_MITT1},
    {AR_ROOM, 66, 48, X_UP, U_SATCHEL1}, {AR_ROOM, 18, 10, X_UP, U_SATCHEL1}, {AR_ROOM, 46, 136, X_UP, U_SATCHEL1},
    {AR_ROOM, 63, 106, X_UP, U_SATCHEL1}, {AR_ROOM, 83, 57, X_UP, U_SATCHEL1},
    {AR_ROOM, 18, 117, X_UP, U_BEAN1}, {AR_ROOM, 17, 61, X_UP, U_BEAN1}, {AR_ROOM, 8, 142, X_UP, U_BEAN1},
    {AR_ROOM, 30, 46, X_UP, U_BEAN1}, {AR_ROOM, 63, 120, X_UP, U_BEAN1},
    {AR_ROOM, 8, 135, X_UP, U_CLOGS1}, {AR_ROOM, 67, 116, X_UP, U_CLOGS1}, {AR_CAVITY, 20, 90, X_UP, U_CLOGS1},
    {AR_ROOM, 80, 140, X_UP, U_CLOGS1}, {AR_ROOM, 14, 133, X_UP, U_CLOGS1},
    {AR_ROOM, 55, 145, X_UPGAP, U_BUZZ1}, {AR_CAVITY, 24, 40, X_UP, U_BUZZ1}, {AR_ROOM, 20, 142, X_UPGAP, U_BUZZ1},
    {AR_ROOM, 83, 58, X_UP, U_BUZZ1},
    {AR_ROOM, 71, 138, X_UP, U_TOP1}, {AR_ROOM, 83, 19, X_UP, U_TOP1}, {AR_ROOM, 8, 138, X_UP, U_TOP1},
    {AR_ROOM, 46, 135, X_UPGAP, U_TOP1},
    {AR_ROOM, 20, 84, X_UP, U_TONIC2}, {AR_ROOM, 20, 87, X_UP, U_TONIC2},
    /* the ones with no second level */
    {AR_ROOM, 8, 140, X_UP, U_FEATHER}, {AR_ROOM, 80, 136, X_UP, U_BANGLE},
    {AR_ROOM, 18, 116, X_UP, U_FIZZ}, {AR_ROOM, 83, 17, X_UP, U_WHISTLE},
    /* eight lost waxlings */
    {AR_ROOM, 83, 21, X_BABY, 0}, {AR_ROOM, 83, 34, X_BABY, 1}, {AR_ROOM, 18, 12, X_BABY, 2},
    {AR_ROOM, 62, 53, X_BABY, 3}, {AR_ROOM, 63, 106, X_BABY, 4}, {AR_ROOM, 73, 61, X_BABY, 5},
    {AR_ROOM, 80, 144, X_BABY, 6}, {AR_ROOM, 17, 61, X_BABY, 7},
    /* six paperfish in the books */
    {AR_ROOM, 49, 139, X_PFISH, 0}, {AR_ROOM, 46, 136, X_PFISH, 1}, {AR_ROOM, 52, 142, X_PFISH, 2},
    {AR_ROOM, 55, 145, X_PFISH, 3}, {AR_ROOM, 45, 131, X_PFISH, 4}, {AR_ROOM, 20, 141, X_PFISH, 5},
    /* three glue blocks for Fernby's builders */
    {AR_ROOM, 62, 51, X_GLUE, 0}, {AR_ROOM, 36, 47, X_GLUE, 1}, {AR_ROOM, 63, 120, X_GLUE, 2},
    /* four mossfolk caught in bubbles */
    {AR_ROOM, 66, 40, X_BUBBLE, 0}, {AR_ROOM, 66, 45, X_BUBBLE, 1}, {AR_ROOM, 66, 50, X_BUBBLE, 2},
    {AR_ROOM, 62, 38, X_BUBBLE, 3},
    /* one-off things */
    {AR_ROOM, 71, 136, X_OBJ, O_BLUEEYE},
    {AR_ROOM, 8, 6, X_GERM, F_GERM}, {AR_ROOM, 80, 142, X_GERM, F_GERM2},
    /* folk with a favour or a word, and the odd things they talk about */
    {AR_ROOM, 20, 83, X_NPC, N_EXILE}, {AR_ROOM, 30, 56, X_NPC, N_DUMPER}, {AR_ROOM, 36, 140, X_NPC, N_EXILE2},
    {AR_ROOM, 17, 137, X_NPC, N_FACE}, {AR_ROOM, 14, 131, X_FACE, 0},
    {AR_ROOM, 30, 52, X_DUMP, 0}, {AR_ROOM, 30, 53, X_DUMP, 1}, {AR_ROOM, 84, 6, X_SCAMPER, 0},
    {AR_ROOM, 83, 26, X_OBJ, O_SPORE}, {AR_ROOM, 66, 42, X_OBJ, O_SPORE},
    {AR_ROOM, 62, 37, X_OBJ, O_PUPA}, {AR_ROOM, 83, 31, X_OBJ, O_HARD},
};

static int town_lookup(int area, int row, int x, int *part) {
    for (int k = 0; k < TW_COUNT; k++)
        if (TOWN[k].area == area && TOWN[k].row == row && x >= TOWN[k].x && x < TOWN[k].x + TOWN[k].n) {
            *part = x - TOWN[k].x;
            return k;
        }
    return -1;
}

int dd_town_at(const Level *L, int tx) {
    int c = tx / CHUNK_W;
    if (L->d.kind != LV_STRIP || c < 0 || c >= L->d.n) return -1;
    return L->town[c];
}

/* the parent area of a strip whose parent is an area (-1 otherwise) */
static int strip_area(const LevelDesc *d) {
    for (int a = 0; a < AR_COUNT; a++)
        if (d->key == dd_hash(0xA0u, (uint32_t)a)) return a;
    return -1;
}

/* ---- town chunk builders -------------------------------------------- */

static int OX; /* chunk origin in the level */

static void flat_ground(int mat_tile, int under) {
    fill(OX, TOWN_H, OX + CHUNK_W - 1, CHUNK_H - 1, under);
    fill(OX, TOWN_H, OX + CHUNK_W - 1, TOWN_H, mat_tile);
    for (int x = OX; x < OX + CHUNK_W; x++) B->surf[x] = TOWN_H;
}
/* a little house: back wall as decoration, a roof you can stand on */
static void house(int x, int w, int h, int roof) {
    bgf(OX + x, TOWN_H - h, OX + x + w - 1, TOWN_H - 1, BG_HOLLOW);
    fill(OX + x - 1, TOWN_H - h - 1, OX + x + w, TOWN_H - h - 1, roof);
    /* a ladder of little ledges up the left wall to the roof */
    for (int y = TOWN_H - 3; y > TOWN_H - h - 1 + 1; y -= 3)
        if (x - 3 >= 0) fill(OX + imax(0, x - 3), y, OX + x - 2, y, T_LEDGE);
}
static void tnpc(int sub, int x) { npc(sub, OX + x, TOWN_H - 1); }
static void tfoe(int sub, int x) {
    /* a town's creatures are part of its design: they come even beside the way in */
    if (!spawning || (dd_no_foes && sub < F_FIRST_BOSS)) return;
    put(EK_FOE, sub, OX + x, TOWN_H - 1);
}
static void tstands(int owner, int x) {
    if (!spawning) return;
    for (int k = 0; k < 3; k++) {
        int i = put(EK_STAND, k, OX + x + k * 3, TOWN_H - 1);
        if (i >= 0) dd_ent[i].param = owner;
    }
}

static void town_chunk(int tw, int part) {
    int base = T_SOIL, top = T_MOSS;
    switch (tw) {
    case TW_TUFTVILLE: case TW_SNARL: case TW_KITCHEN: base = T_FABRIC; top = T_WAX; break;
    case TW_LOAMTON: case TW_SOLDIER: case TW_MOLD: case TW_REDCAVE: case TW_FERNBY: case TW_ROTIFER: case TW_HOPPER: case TW_DEN:
        base = T_SOIL; top = T_MOSS; break;
    case TW_OUTPOST: case TW_HIVE: base = T_MOSS; top = T_LEAF; break;
    case TW_WORKSHOP: case TW_SNOUT: case TW_CRASH: base = T_METAL; top = T_GEAR; break;
    case TW_TICKBURG: case TW_WORMWOOD: case TW_LATCH: base = T_WOOD; top = T_PLANK; break;
    case TW_GLIMMER: base = T_SHADE; top = T_GLASS; break;
    case TW_BLUEDUST: base = T_BOOKB; top = T_BOOKW; break;
    }
    if (tw == TW_KITCHEN) { base = T_BRASS; top = T_BRASS; }
    if (!spawning) flat_ground(top, base);
    switch (tw) {
    case TW_TUFTVILLE:
        if (part == 0) {
            if (!spawning) { house(4, 7, 5, T_WAX); house(16, 9, 6, T_WAX); house(30, 6, 4, T_WAX); }
            tnpc(N_TAPER, 12);
            tstands(N_TAPER, 18);
            tnpc(N_SNUFF, 34);
            glints_row(OX + 5, TOWN_H - 7, 4);
        } else {
            if (!spawning) { house(3, 10, 7, T_WAX); house(20, 8, 5, T_WAX); fill(OX + 30, TOWN_H - 3, OX + 34, TOWN_H - 3, T_THREAD); }
            tnpc(N_TALLOW, 8);
            glints_row(OX + 30, TOWN_H - 4, 5);
        }
        break;
    case TW_SNARL:
        if (!spawning) { house(6, 8, 6, T_WAX); house(24, 10, 6, T_WAX); fill(OX + 18, TOWN_H - 4, OX + 21, TOWN_H - 4, T_LEDGE); }
        tnpc(N_SMUDGE, 28);
        tfoe(F_WAXGUARD, 10);
        tfoe(F_WAXGUARD, 20);
        tfoe(F_WAXGUARD, 33);
        if (!dd_flag(FL_SNARL_DONE) && spawning && !dd_obj_exists(O_ARTIFACT, -1)) obj(O_ARTIFACT, OX + 30, TOWN_H - 8);
        break;
    case TW_ROTIFER:
        if (!spawning) {
            fill(OX + 2, TOWN_H, OX + 37, TOWN_H + 1, T_AIR);
            fill(OX + 2, TOWN_H + 2, OX + 37, TOWN_H + 2, T_MOSS);
            for (int x = OX; x < OX + CHUNK_W; x++) B->surf[x] = (uint8_t)(x < OX + 2 || x > OX + 37 ? TOWN_H : TOWN_H + 2);
            fill(OX + 6, TOWN_H - 3, OX + 9, TOWN_H - 3, T_LEAF);
            fill(OX + 30, TOWN_H - 3, OX + 33, TOWN_H - 3, T_LEAF);
        }
        if (dd_flag(FL_OUTPOST) && !dd_flag(FL_ROTIFER_DEAD)) foe(F_ROTIFER, OX + 20, TOWN_H + 1);
        break;
    case TW_FERNBY:
        if (part == 0) {
            if (!spawning) { house(3, 8, 6, T_LEAF); house(15, 6, 5, T_LEAF); house(27, 9, 7, T_LEAF); }
            tnpc(N_FERN, 7);
            tnpc(N_LICHEN, 18);
            tnpc(N_MASON, 30);
            glints_row(OX + 27, TOWN_H - 9, 5);
        } else {
            if (!spawning) { house(12, 10, 6, T_LEAF); }
            tnpc(N_SPRIG, 6);
            tstands(N_SPRIG, 10);
            glints_row(OX + 24, TOWN_H - 1, 3);
        }
        break;
    case TW_HOPPER:
        if (!spawning) { house(26, 6, 5, T_MOSS); }
        tfoe(F_GULP, 12);
        tfoe(F_ANT, 16);
        tfoe(F_ANT, 18);
        tnpc(N_HOPPERSAGE, 28);
        break;
    case TW_DEN:
        if (!spawning) {
            fill(OX + 8, TOWN_H + 1, OX + 32, TOWN_H + 8, T_AIR);
            fill(OX + 8, TOWN_H + 9, OX + 32, TOWN_H + 9, T_ROOT);
            fill(OX + 8, TOWN_H + 6, OX + 9, TOWN_H + 6, T_LEDGE);    /* steps up under the hole */
            fill(OX + 10, TOWN_H + 3, OX + 11, TOWN_H + 3, T_LEDGE);
            fill(OX + 8, TOWN_H, OX + 11, TOWN_H, T_AIR);
            for (int x = OX + 8; x <= OX + 11; x++) B->surf[x] = TOWN_H + 9;
        }
        foe(F_LANCER, OX + 16, TOWN_H + 8);
        foe(F_LANCER, OX + 24, TOWN_H + 8);
        foe(F_ANT, OX + 20, TOWN_H + 8);
        if (!dd_flag(FL_EGG_DONE) && spawning && !dd_obj_exists(O_EGG, -1)) obj(O_EGG, OX + 29, TOWN_H + 8);
        break;
    case TW_OUTPOST:
        if (!spawning) { house(14, 8, 6, T_LEAF); }
        tnpc(N_BRACKEN, 18);
        break;
    case TW_HIVE:
        if (!spawning) { house(10, 14, 8, T_WAX); fill(OX + 12, TOWN_H - 1, OX + 21, TOWN_H - 1, T_AIR); fill(OX + 30, TOWN_H, OX + 33, TOWN_H, T_WAX); }
        tfoe(F_BUMBLE, 8);
        tfoe(F_BUMBLE, 26);
        tnpc(N_BEE, 30);
        if (spawning) { int i = dd_add_ent(EK_DRIP, 1, (float)((OX + 17) * DD_TS + 3), (float)((TOWN_H - 8) * DD_TS)); (void)i; } /* honey from the comb */
        break;
    case TW_LOAMTON:
        if (part == 0) {
            if (!spawning) { house(3, 8, 6, T_MOSS); house(16, 7, 5, T_MOSS); house(28, 9, 7, T_MOSS); }
            tnpc(N_CLOD, 6);
            tstands(N_CLOD, 10);
            tnpc(N_SORREL, 20);
            tnpc(N_PEAT, 32);
        } else {
            if (!spawning) { house(6, 12, 8, T_STONE); fill(OX + 24, TOWN_H - 2, OX + 26, TOWN_H - 1, T_STONE); }
            tnpc(N_SMITH, 12);
            tnpc(N_HISTORIAN, 22);
            if (dd_flag(FL_WATER_GIVEN)) tnpc(N_SOLDIER, 30);
        }
        break;
    case TW_SOLDIER:
        if (!dd_flag(FL_WATER_GIVEN)) tnpc(N_SOLDIER, 20);
        if (!spawning) house(24, 6, 4, T_MOSS);
        break;
    case TW_MOLD:
        if (!spawning) { fill(OX + 18, TOWN_H - 3, OX + 21, TOWN_H - 1, T_MOSS); fill(OX + 16, TOWN_H - 4, OX + 23, TOWN_H - 4, T_LEAF); }
        /* a mold plant bearing three fruits: a thrown fruit splits into a seed */
        obj(O_MOLDFRUIT, OX + 16, TOWN_H - 5);
        obj(O_MOLDFRUIT, OX + 23, TOWN_H - 5);
        obj(O_MOLDFRUIT, OX + 28, TOWN_H - 1);
        break;
    case TW_REDCAVE:
        if (!spawning) {
            fill(OX + 6, TOWN_H + 1, OX + 34, TOWN_H + 7, T_AIR);
            fill(OX + 6, TOWN_H, OX + 9, TOWN_H, T_AIR);
            for (int x = OX + 6; x <= OX + 9; x++) B->surf[x] = TOWN_H + 8;
            fill(OX + 6, TOWN_H + 8, OX + 34, TOWN_H + 8, T_SOIL);
            fill(OX + 6, TOWN_H + 5, OX + 7, TOWN_H + 5, T_LEDGE);     /* steps up under the hole */
            fill(OX + 8, TOWN_H + 2, OX + 9, TOWN_H + 2, T_LEDGE);
        }
        /* the lancers' nest, kept by cave octopods: one drops a red egg */
        foe(F_LANCER, OX + 20, TOWN_H + 7);
        tfoe(F_OCTO, 26);
        if (spawning) {
            int i = put(EK_FOE, F_OCTO, OX + 31, TOWN_H + 7);
            if (i >= 0) dd_ent[i].param = 1; /* carries the egg */
        }
        break;
    case TW_WORKSHOP:
        if (!spawning) { house(8, 16, 8, T_GEAR); fill(OX + 28, TOWN_H - 3, OX + 31, TOWN_H - 3, T_LEDGE); }
        tnpc(N_SPINNER, 16);
        glints_row(OX + 28, TOWN_H - 4, 4);
        break;
    case TW_SNOUT:
        if (!spawning) { fill(OX + 16, TOWN_H - 3, OX + 22, TOWN_H - 1, T_METAL); }
        upgrade_copy(U_WHISTLE, OX + 19, TOWN_H - 4, true);
        tfoe(F_SPARK, 30);
        break;
    case TW_CRASH:
        if (!spawning) { fill(OX + 14, TOWN_H - 2, OX + 26, TOWN_H - 1, T_BONE); }
        if (!dd_flag(FL_CRATE_DONE) && spawning && !dd_obj_exists(O_CRATE, -1)) obj(O_CRATE, OX + 20, TOWN_H - 3);
        tfoe(F_BUZZER, 8);
        break;
    case TW_KITCHEN:
        if (!spawning) { house(10, 14, 7, T_BRASS); }
        tnpc(N_MOREL, 16);
        break;
    case TW_TICKBURG:
        if (part == 0) {
            if (!spawning) { house(4, 8, 7, T_PLANK); house(18, 6, 5, T_PLANK); house(30, 7, 6, T_PLANK); }
            tnpc(N_ONEEYE, 8);
            tnpc(N_GILL, 21);
            tnpc(N_SEEDKEEPER, 33);
        } else {
            if (!spawning) { house(10, 12, 8, T_PLANK); }
            if (!dd_flag(FL_TABLET_GIVEN)) tnpc(N_OLDCAP, 16);
            glints_row(OX + 26, TOWN_H - 1, 5);
        }
        break;
    case TW_GLIMMER:
        if (part == 0) {
            if (!spawning) { house(6, 10, 7, T_GLASS); house(24, 8, 5, T_GLASS); }
            tnpc(N_HIGHLUMEN, 11);
            glints_row(OX + 24, TOWN_H - 7, 4);
        } else {
            if (!spawning) { house(10, 8, 6, T_GLASS); fill(OX + 33, TOWN_H - 1, OX + 33, TOWN_H - 1, T_GLASS); }
            tnpc(N_WARDEN, 14);
            tnpc(N_PILGRIM, 30); /* by the flashing beacon at the town's end */
        }
        break;
    case TW_LATCH:
        if (part == 0) {
            /* the far end: the mage's house, with the prison key on a hook */
            if (!spawning) { house(8, 16, 7, T_METAL); fill(OX + 30, TOWN_H - 3, OX + 33, TOWN_H - 3, T_LEDGE); }
            tnpc(N_MAGE, 12);
            if (spawning && !dd_flag(FL_KEY_TAKEN) && !dd_flag(FL_PELL_FREED) && !dd_flag(FL_PELL_GUARDED) && !dd_obj_exists(O_KEY, -1))
                obj(O_KEY, OX + 20, TOWN_H - 1);
            glints_row(OX + 30, TOWN_H - 4, 4);
        } else if (part == 1) {
            /* the main hall, and the cells under it through a trapdoor */
            if (!spawning) {
                house(4, 14, 8, T_METAL);
                fill(OX + 8, TOWN_H + 1, OX + 32, TOWN_H + 10, T_AIR);
                fill(OX + 8, TOWN_H + 11, OX + 32, TOWN_H + 11, T_METAL);
                fill(OX + 20, TOWN_H, OX + 22, TOWN_H, T_LEDGE);     /* the trapdoor */
                fill(OX + 12, TOWN_H + 7, OX + 15, TOWN_H + 7, T_LEDGE); /* steps back up */
                fill(OX + 16, TOWN_H + 4, OX + 23, TOWN_H + 4, T_LEDGE);
                bgf(OX + 25, TOWN_H + 1, OX + 31, TOWN_H + 10, BG_BOX);
            }
            tnpc(N_RATCHET, 8);
            npc(N_KNIGHT, OX + 11, TOWN_H + 10);
            if (!dd_flag(FL_PELL_FREED) && !dd_flag(FL_PELL_GUARDED)) npc(N_PELL, OX + 28, TOWN_H + 10);
        } else {
            if (!spawning) { house(6, 8, 6, T_METAL); house(22, 12, 7, T_METAL); }
            if (dd_sv.glints_total >= 667) tfoe(F_TINMOUSE, 18);
            glints_row(OX + 4, TOWN_H - 1, 3);
        }
        break;
    case TW_WORMWOOD:
        if (part == 0) {
            if (!spawning) { house(4, 9, 6, T_WOOD); house(18, 12, 8, T_WOOD); }
            tnpc(N_WRIGGLA, 24);
            tnpc(N_BORER, 8);
        } else {
            if (!spawning) { house(8, 12, 6, T_WOOD); if (!dd_flag(FL_WORM_FRIENDS)) fill(OX + 1, TOWN_H - 5, OX + 2, TOWN_H - 1, T_WOODDK); }
            tnpc(N_KNOT, 10);
            if (dd_flag(FL_WORM_FRIENDS)) tstands(N_KNOT, 14);
        }
        break;
    case TW_BLUEDUST:
        if (!spawning) { fill(OX + 14, TOWN_H - 2, OX + 24, TOWN_H - 1, T_DUST); }
        if (!dd_flag(FL_BLUEDUST_DONE) && spawning && !dd_obj_exists(O_BLUEDUST, -1)) obj(O_BLUEDUST, OX + 19, TOWN_H - 3);
        tfoe(F_MITE, 8);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* the generator: every exposed tile holds a chunk of the next size down */

static uint32_t row_key; /* hash of the parent key and row */

static int boundary_h(const LevelDesc *d, int i) {
    /* the surface height where chunk i-1 meets chunk i; towns sit at TOWN_H */
    int x = d->x0 + i, part, ax = d->pabs + d->x0 + i;
    int area = strip_area(d);
    if (area >= 0 && (town_lookup(area, d->row, x, &part) >= 0 || town_lookup(area, d->row, x - 1, &part) >= 0)) return TOWN_H;
    return 8 + (int)(dd_hash(row_key, (uint32_t)(ax + 1000)) % 4);
}

static int biome_tile(int mat, int variant) {
    switch (mat) {
    case M_WOOD: return variant ? T_WOODDK : T_WOOD;
    case M_WALL: return T_WALL;
    case M_CERAMIC: return T_CERAMIC;
    case M_SOIL: return variant ? T_ROOT : T_SOIL;
    case M_FABRIC: return variant ? T_FABRIC2 : T_FABRIC;
    case M_PAPER: return variant ? T_BOOKW : T_BOOKY;
    case M_METAL: return variant ? T_GEAR : T_METAL;
    case M_GLASS: return variant ? T_SHADE : T_GLASS;
    case M_DUST: return T_DUST;
    case M_CARD: return variant ? T_CARD2 : T_CARD;
    case M_FUR: return T_FUR;
    case M_CELL: return variant ? T_CELL2 : T_CELL;
    case M_STONE: return T_STONE;
    case M_PLANT: return variant ? T_LEAF : T_MOSS;
    case M_WAX: return T_WAX;
    }
    return T_STONE;
}
static int biome_ledge(int mat) {
    switch (mat) {
    case M_FABRIC: case M_DUST: return T_THREAD;
    case M_PLANT: case M_SOIL: return T_LEAF;
    case M_FUR: return T_STRAND;
    default: return T_LEDGE;
    }
}

static const Extra *extra_for(const LevelDesc *d, int x, int type, int k) {
    int area = strip_area(d);
    if (area < 0 || d->scale != SC_MICRO) return NULL;
    int n = 0;
    for (int i = 0; i < ARRAY_LEN(EXTRA); i++)
        if (EXTRA[i].area == area && EXTRA[i].row == d->row && EXTRA[i].x == x && EXTRA[i].type == type && n++ == k) return &EXTRA[i];
    return NULL;
}

/* the key of the micro chunk over a tile of an area, as the strip builder
 * makes it, so that fixed things there have ids known from outside */
static uint32_t chunk_key(int area, int row, int x) {
    return dd_hash(dd_hash(dd_hash(0xA0u, (uint32_t)area), (uint32_t)row), (uint32_t)x);
}
static uint32_t extra_pid(const Extra *e) {
    return dd_hash(chunk_key(e->area, e->row, e->x), 0xE000u + (uint32_t)(e - EXTRA)) | 1u;
}
static uint32_t big_pid(int area, int row, int x, int j) {
    return dd_hash(chunk_key(area, row, x), 0xB000u + (uint32_t)j) | 1u;
}

/* Dash's nose: a room tile whose micro chunk holds an upgrade still to be
 * had, a dangerous cave's big glints, or a town Dot hasn't found */
bool dd_sniff_spot(int area, int row, int x) {
    for (int i = 0; i < ARRAY_LEN(EXTRA); i++) {
        const Extra *e = &EXTRA[i];
        if (e->area != area || e->row != row || e->x != x) continue;
        if ((e->type == X_UP || e->type == X_UPGAP) && !dd_collected(extra_pid(e)) && dd_resolve_upgrade(e->param) >= 0) return true;
        if (e->type == X_DANGER && ((dd_sv.caves_done >> (e->param * 2)) & 3) != 3) return true;
    }
    int part;
    int k = town_lookup(area, row, x, &part);
    if (k >= 0 && part == 0 && !((dd_sv.towns_seen >> k) & 1)) return true;
    return false;
}

int dd_town_mark(int k, int *area, int *row, int *x) {
    if (k < 0 || k >= TW_COUNT || !((dd_sv.towns_seen >> k) & 1)) return 0;
    *area = TOWN[k].area; *row = TOWN[k].row; *x = TOWN[k].x;
    return 1;
}

void dd_note_town(void) {
    if (dd_lv.d.kind != LV_STRIP) return;
    int c = dd_chunk_of((int)(dd_p.x + dd_p.w / 2));
    if (dd_lv.town[c] >= 0 && !((dd_sv.towns_seen >> dd_lv.town[c]) & 1)) {
        dd_sv.towns_seen |= 1u << dd_lv.town[c];
        dd_autosave();
    }
}

bool dd_in_danger_cave(void) {
    if (dd_lv.d.kind != LV_STRIP) return false;
    return dd_lv.special[dd_chunk_of((int)(dd_p.x + dd_p.w / 2))] != 0;
}

static void gen_chunk(const LevelDesc *d, int i) {
    int ox = i * CHUNK_W, mat = B->biome[i];
    Rng r;
    rng_seed(&r, B->ckey[i]);
    int hL = boundary_h(d, i), hR = boundary_h(d, i + 1);
    int solid_t = biome_tile(mat, 0), alt_t = biome_tile(mat, 1);
    /* surface profile */
    int h = hL, x = 0;
    while (x < 30) {
        int len = rng_range(&r, 4, 8);
        for (int k = 0; k < len && x < 30; k++, x++) B->surf[ox + x] = (uint8_t)h;
        h = iclamp(h + rng_range(&r, -2, 2), 7, 13);
    }
    while (x < CHUNK_W) {
        if (x >= 32 && h != hR) h += iclamp(hR - h, -2, 2);
        B->surf[ox + x] = (uint8_t)h;
        x++;
        if (x == CHUNK_W - 1) h = hR;
    }
    B->surf[ox + CHUNK_W - 1] = (uint8_t)hR;
    for (x = 0; x < CHUNK_W; x++) {
        int s = B->surf[ox + x];
        for (int y = s; y < CHUNK_H; y++) {
            int t = solid_t;
            if (mat == M_WOOD && (y / 3) % 2) t = alt_t;
            else if (mat == M_FABRIC && ((x + y) / 3) % 2) t = alt_t;
            else if (mat == M_PAPER && y % 4 == 0) t = alt_t;
            else if (mat == M_METAL && (x % 6 == 0)) t = alt_t;
            else if (mat == M_CELL && dd_hash(B->ckey[i], (uint32_t)(x * 64 + y)) % 5 == 0) t = alt_t;
            else if (mat == M_SOIL && dd_hash(B->ckey[i], (uint32_t)(x * 64 + y)) % 13 == 0) t = T_STONE;
            else if (mat == M_CARD && y % 5 == 0) t = alt_t;
            lv_set(B, ox + x, y, t);
        }
    }
    /* the cave: a staircase down to a chamber */
    B->cave_x0[i] = B->cave_x1[i] = B->cave_floor[i] = -1;
    B->gap_x[i] = B->gap_y[i] = -1;
    bool danger = extra_for(d, d->x0 + i, X_DANGER, 0) != NULL;
    bool need = danger || extra_for(d, d->x0 + i, X_UP, 0) || extra_for(d, d->x0 + i, X_OBJ, 0);
    B->special[i] = danger;
    int cave_e = 99, cave_end = -1;
    if (need || rng_chance(&r, 55)) {
        /* the chamber stays roofed: its floor goes deep enough to keep two
         * tiles of ground over it wherever the surface lies in this chunk */
        int ch = danger ? 7 : 5, maxs = 0;
        for (int xx = 0; xx < CHUNK_W; xx++) maxs = imax(maxs, B->surf[ox + xx]);
        int e = danger ? rng_range(&r, 3, 5) : rng_range(&r, 3, 10);
        int s0 = B->surf[ox + e];
        int steps = imax(2, (maxs + ch + 2 - s0 + 1) / 2);
        while (s0 + 2 * steps > CHUNK_H - 3) steps--;
        int F = s0 + 2 * steps;
        for (int k = 0; k < steps; k++) {
            int f = s0 + 2 * (k + 1);
            fill(ox + e + 2 * k, f - 4, ox + e + 2 * k + 2, f - 1, T_AIR);
        }
        int cx0 = e + 2 * steps;
        int W = imin(danger ? 14 : rng_range(&r, 8, 12), CHUNK_W - 3 - cx0);
        int cx1 = cx0 + W - 1;
        fill(ox + cx0 - 1, F - ch, ox + cx1, F - 1, T_AIR);
        fill(ox + cx0 - 1, F, ox + cx1, F, solid_t);
        for (int xx = e; xx < e + 3; xx++) B->surf[ox + xx] = (uint8_t)imin(B->surf[ox + xx], s0);
        B->cave_x0[i] = (int8_t)cx0;
        B->cave_x1[i] = (int8_t)cx1;
        B->cave_floor[i] = (int8_t)F;
        cave_e = e;
        cave_end = e + 2 * steps + 2;
        if (danger) {
            /* thorns across the chamber floor with safe stones between */
            for (int xx = cx0 + 2; xx <= cx1 - 2; xx++)
                if ((xx - cx0) % 4 == 3) lv_set(B, ox + xx, F - 1, mat == M_SOIL ? T_GOO : T_THORN); /* a bad tile in every four */
            fill(ox + cx0 + 3, F - 4, ox + cx0 + 6, F - 4, biome_ledge(mat));
            fill(ox + cx1 - 6, F - 4, ox + cx1 - 3, F - 4, biome_ledge(mat));
        } else if (rng_chance(&r, 35)) {
            fill(ox + cx0 + 2, F - 3, ox + cx0 + 5, F - 3, biome_ledge(mat));
        }
    }
    /* the surface as it is now (a stair may have opened the ground) */
    for (x = 1; x < CHUNK_W - 1; x++) {
        int y = 1;
        while (y < CHUNK_H - 1 && !(DD_TILE[tile_at(ox + x, y)].flags & TF_SOLID)) y++;
        B->surf[ox + x] = (uint8_t)y;
    }
    /* a nook under a slab, reached through a gap one tile high: only Dot at
     * her tiniest fits. Some hold an upgrade, the rest a handful of glints */
    bool gap_need = extra_for(d, d->x0 + i, X_UPGAP, 0) != NULL;
    if (gap_need || rng_chance(&r, 22)) {
        for (int tries = 0; tries < 14 && B->gap_x[i] < 0; tries++) {
            int gx = 12 + (int)(dd_hash(B->ckey[i], (uint32_t)(tries + 91)) % 16u);
            if (gx + 8 >= CHUNK_W - 1) continue;
            if (gx <= cave_end && gx + 8 >= cave_e) continue;   /* clear of the cave's stair */
            int s0 = B->surf[ox + gx];
            bool ok = s0 >= 7;
            for (int k = 0; k <= 8 && ok; k++) ok = B->surf[ox + gx + k] == s0 && tile_at(ox + gx + k, s0 - 5) == T_AIR;
            if (!ok) continue;
            fill(ox + gx + 1, s0 - 4, ox + gx + 7, s0 - 2, solid_t);
            fill(ox + gx + 7, s0 - 1, ox + gx + 7, s0 - 1, solid_t);
            B->gap_x[i] = (int8_t)(gx + 5);
            B->gap_y[i] = (int8_t)(s0 - 1);
            for (int k = 1; k <= 7; k++) B->surf[ox + gx + k] = (uint8_t)(s0 - 4);
        }
        if (gap_need && B->gap_x[i] < 0) {
            /* an upgrade must have its nook: level a patch of ground for it,
             * right of the cave's stair */
            int gx = imax(cave_end + 2, 12);
            if (gx + 9 >= CHUNK_W - 1) gx = CHUNK_W - 11;
            int s0 = iclamp(B->surf[ox + gx], 8, 13);
            for (int k = 0; k <= 8; k++) {
                fill(ox + gx + k, 1, ox + gx + k, s0 - 1, T_AIR);
                for (int y = s0; y < s0 + 3; y++)
                    if (tile_at(ox + gx + k, y) == T_AIR) lv_set(B, ox + gx + k, y, solid_t);
                B->surf[ox + gx + k] = (uint8_t)s0;
            }
            fill(ox + gx + 1, s0 - 4, ox + gx + 7, s0 - 2, solid_t);
            fill(ox + gx + 7, s0 - 1, ox + gx + 7, s0 - 1, solid_t);
            B->gap_x[i] = (int8_t)(gx + 5);
            B->gap_y[i] = (int8_t)(s0 - 1);
            for (int k = 1; k <= 7; k++) B->surf[ox + gx + k] = (uint8_t)(s0 - 4);
        }
    }
    /* ledges above the ground */
    if (rng_chance(&r, 55)) {
        int lx = rng_range(&r, 2, CHUNK_W - 10), lw = rng_range(&r, 4, 7);
        int top = 99;
        for (int k = 0; k < lw; k++) top = imin(top, B->surf[ox + lx + k]);
        int ly = top - 4;
        if (ly > 2) fill(ox + lx, ly, ox + lx + lw - 1, ly, biome_ledge(mat));
    }
    /* biome touches */
    if (mat == M_FABRIC || mat == M_FUR) {
        for (int k = 0; k < 2; k++) {
            int lx = rng_range(&r, 2, CHUNK_W - 6);
            int ly = B->surf[ox + lx] - rng_range(&r, 3, 4);
            if (ly > 2 && tile_at(ox + lx, ly) == T_AIR) fill(ox + lx, ly, ox + lx + 3, ly, mat == M_FUR ? T_STRAND : T_THREAD);
        }
    }
    if (mat == M_PLANT && rng_chance(&r, 30)) {
        int lx = rng_range(&r, 4, CHUNK_W - 6);
        if (tile_at(ox + lx, B->surf[ox + lx] - 1) == T_AIR) lv_set(B, ox + lx, B->surf[ox + lx] - 1, T_SHROOM);
    }
    if (mat == M_SOIL && rng_chance(&r, 25)) {
        int lx = rng_range(&r, 4, CHUNK_W - 8);
        bool flat = true;
        for (int k = 0; k < 4; k++) flat &= B->surf[ox + lx + k] == B->surf[ox + lx];
        if (flat) fill(ox + lx, B->surf[ox + lx], ox + lx + 3, B->surf[ox + lx], T_GOO), (void)0;
    }
}

/* which creatures live where */
static int biome_foe(int mat, Rng *r) {
    switch (mat) {
    case M_WOOD: { static const int L[] = {F_SPRING, F_AXEANT, F_ANT, F_CLIMBER}; return L[rng_range(r, 0, 3)]; }
    case M_WALL: { static const int L[] = {F_ANT, F_SPRING, F_CLIMBER}; return L[rng_range(r, 0, 2)]; }
    case M_CERAMIC: { static const int L[] = {F_SPRING, F_BUMBLE, F_CLIMBER}; return L[rng_range(r, 0, 2)]; }
    case M_SOIL: { static const int L[] = {F_ANT, F_WIGGLER, F_SPRING, F_GERM2}; return L[rng_range(r, 0, 3)]; }
    case M_FABRIC: { static const int L[] = {F_MITE, F_MOTH, F_SPRING}; return L[rng_range(r, 0, 2)]; }
    case M_PAPER: { static const int L[] = {F_SPRING, F_MITE, F_SPITTER}; return L[rng_range(r, 0, 2)]; }
    case M_METAL: { static const int L[] = {F_SPARK, F_POD, F_ANT, F_SPITTER}; return L[rng_range(r, 0, 3)]; }
    case M_GLASS: { static const int L[] = {F_MOTH, F_SPARK}; return L[rng_range(r, 0, 1)]; }
    case M_DUST: { static const int L[] = {F_MITE, F_GERM, F_GERM2}; return L[rng_range(r, 0, 2)]; }
    case M_CARD: { static const int L[] = {F_ANT, F_SPITTER}; return L[rng_range(r, 0, 1)]; }
    case M_FUR: return F_NIP;
    case M_CELL: { static const int L[] = {F_GERM, F_GERM2, F_WIGGLER}; return L[rng_range(r, 0, 2)]; }
    case M_PLANT: { static const int L[] = {F_BUMBLE, F_SPRING, F_MOTH, F_BUZZER}; return L[rng_range(r, 0, 3)]; }
    default: return F_SPRING;
    }
}
static int biome_block(int mat, Rng *r) {
    static const int ANY[] = {O_CRACKER, O_DART, O_BOOMER, O_ROLLER, O_HOURGLASS, O_QUAKE, O_VENOM, O_PUPA, O_SPORE, O_MOLDFRUIT, O_AXE, O_POPGUN, O_GLUE};
    if (mat == M_SOIL && rng_chance(r, 40)) return O_MOLDFRUIT;
    if (mat == M_PLANT && rng_chance(r, 40)) return O_SPORE;
    if (mat == M_FABRIC && rng_chance(r, 30)) return O_PUPA;
    if (mat == M_WOOD && rng_chance(r, 30)) return O_AXE;
    int k = rng_range(r, 0, ARRAY_LEN(ANY) - 2); /* glue blocks only where the builders need them */
    return ANY[k];
}

/* an upgrade spot with an id of its own (fixed things in micro chunks) */
static void upgrade_spot(const Extra *e, int tx, int ty, bool boxed) {
    if (!spawning) return;
    uint32_t pid = extra_pid(e);
    if (dd_collected(pid)) return;
    int r = dd_resolve_upgrade(e->param);
    int i = r < 0 ? put(EK_PICK, P_GLINT5, tx, ty) : boxed ? put(EK_OBJ, O_GIFTBOX, tx, ty) : put(EK_PICK, P_UPGRADE, tx, ty);
    if (i >= 0) { dd_ent[i].pid = pid; dd_ent[i].param = boxed || r < 0 ? e->param : r; }
}

static void spawn_chunk(const LevelDesc *d, int i) {
    int ox = i * CHUNK_W, mat = B->biome[i];
    Rng r;
    rng_seed(&r, B->ckey[i] ^ 0x5151u);
    place_key = B->ckey[i];
    place_n = 0;
    int px = d->x0 + i;
    bool danger = B->special[i];
    int cx0 = B->cave_x0[i], cx1 = B->cave_x1[i], F = B->cave_floor[i];
    int area = strip_area(d);
    /* surface glints */
    if (rng_chance(&r, 45)) {
        int gx = rng_range(&r, 3, CHUNK_W - 6);
        for (int k = 0; k < 3; k++) {
            int s = B->surf[ox + gx + k];
            if (tile_at(ox + gx + k, s - 1) == T_AIR) pick(P_GLINT1, ox + gx + k, s - 1, 0);
        }
    }
    /* up on a ledge */
    for (int x = 1; x < CHUNK_W - 1; x++)
        for (int y = 2; y < B->surf[ox + x] - 1; y++)
            if (DD_TILE[tile_at(ox + x, y)].flags & TF_ONEWAY && tile_at(ox + x, y - 1) == T_AIR && (x + y) % 3 == 0) pick(P_GLINT1, ox + x, y - 1, 0);
    /* the cave's treasure */
    if (F > 0) {
        if (danger) {
            const Extra *de = extra_for(d, px, X_DANGER, 0);
            for (int j = 0; j < 2; j++) {
                uint32_t pid = big_pid(area, d->row, px, j);
                if (dd_collected(pid)) continue;
                int k = put(EK_PICK, P_GLINT50, ox + (j ? cx1 - 4 : cx0 + 4), F - 5);
                if (k >= 0) { dd_ent[k].pid = pid; dd_ent[k].param = 1 + (de ? de->param : 0) * 2 + j; }
            }
        } else {
            if (rng_chance(&r, 50)) pick(P_GLINT5, ox + (cx0 + cx1) / 2, F - 1, 0);
            if (spawning && rng_chance(&r, 25)) {
                /* a pink stalactite drips slime from the roof */
                int dx = ox + cx0 + 2, dy = F - 1;
                while (dy > 1 && tile_at(dx, dy - 1) == T_AIR) dy--;
                int j = dd_add_ent(EK_DRIP, 2, (float)(dx * DD_TS + 3), (float)(dy * DD_TS));
                (void)j;
            }
        }
    }
    /* fixed things */
    for (int k = 0; k < 4; k++) {
        const Extra *e = extra_for(d, px, X_UP, k);
        if (!e) break;
        static const int SAFE[4] = {1, 5, 9, 13};
        int tx = F > 0 ? ox + cx0 + SAFE[k] : ox + 20 + k * 3;
        int ty = F > 0 ? F - 1 : B->surf[tx] - 1;
        upgrade_spot(e, tx, ty, true);
    }
    const Extra *e;
    if (B->gap_x[i] >= 0) {
        /* the nook behind the gap */
        if ((e = extra_for(d, px, X_UPGAP, 0))) upgrade_spot(e, ox + B->gap_x[i], B->gap_y[i], false); /* loose: no room to lift a box */
        else { pick(P_GLINT5, ox + B->gap_x[i], B->gap_y[i], 0); pick(P_GLINT5, ox + B->gap_x[i] - 2, B->gap_y[i], 0); }
    }
    if ((e = extra_for(d, px, X_BABY, 0)) && !((dd_sv.counts[QC_BABIES] >> e->param) & 1) && !dd_obj_exists(O_BABY, e->param)) {
        {
            int tx = ox + 30, ty = B->surf[tx] - 1;
            int j = put(EK_OBJ, O_BABY, tx, ty);
            if (j >= 0) dd_ent[j].param = e->param;
        }
    }
    if ((e = extra_for(d, px, X_PFISH, 0)) && !((dd_sv.counts[QC_PAPERFISH] >> e->param) & 1)) {
        int tx = ox + 24, ty = B->surf[tx] - 1;
        int j = spawning ? put(EK_FOE, F_PAPERFISH, tx, ty) : -1; /* quest creatures always come */
        if (j >= 0) dd_ent[j].param = e->param + 1;
    }
    if ((e = extra_for(d, px, X_GLUE, 0)) && !((dd_sv.counts[QC_GLUE] >> e->param) & 1) && !dd_obj_exists(O_GLUE, e->param)) {
        int tx = ox + 14, ty = B->surf[tx] - 1;
        int j = put(EK_OBJ, O_GLUE, tx, ty);
        if (j >= 0) dd_ent[j].param = e->param;
    }
    if ((e = extra_for(d, px, X_BUBBLE, 0)) && !((dd_sv.counts[QC_BUBBLES] >> e->param) & 1)) {
        int tx = ox + 26, ty = B->surf[tx] - 2; /* a bubble drifting at head height */
        int j = spawning ? put(EK_NPC, N_BUBBLE, tx, ty) : -1;
        if (j >= 0) dd_ent[j].param = e->param;
    }
    if ((e = extra_for(d, px, X_OBJ, 0))) {
        bool done = (e->param == O_BLUEEYE && dd_flag(FL_BLUEEYE_DONE)) || (e->param == O_HARD && dd_flag(FL_SMITH_DONE));
        if (!done && !dd_obj_exists(e->param, -1)) {
            int tx = F > 0 ? ox + cx1 - 1 : ox + 10;
            int ty = F > 0 ? F - 1 : B->surf[tx] - 1;
            obj(e->param, tx, ty);
        }
    }
    if ((e = extra_for(d, px, X_GERM, 0))) {
        int j = spawning ? put(EK_FOE, e->param, ox + 18, B->surf[ox + 18] - 4) : -1; /* a quest creature */
        (void)j;
    }
    if ((e = extra_for(d, px, X_NPC, 0)) && spawning) {
        int tx = ox + 16;
        npc(e->param, tx, B->surf[tx] - 1);
    }
    for (int k = 0; k < 2; k++)
        if ((e = extra_for(d, px, X_DUMP, k)))
            for (int g = 0; g < 7; g++) pick(P_GLINT5, ox + 8 + g * 3, B->surf[ox + 8 + g * 3] - 1, 0);
    if ((e = extra_for(d, px, X_SCAMPER, 0)) && spawning && !dd_flag(FL_SCAMPER)) put(EK_FOE, F_SCAMPER, ox + 20, B->surf[ox + 20] - 1);
    if ((e = extra_for(d, px, X_FACE, 0)) && spawning && !dd_flag(FL_FACE_DONE) && !(dd_carry >= 0 && dd_ent[dd_carry].kind == EK_FOE && dd_ent[dd_carry].sub == F_FACE))
        put(EK_FOE, F_FACE, ox + 22, B->surf[ox + 22] - 1);
    /* creatures */
    int nf = danger ? 3 : rng_range(&r, 0, 2);
    for (int k = 0; k < nf; k++) {
        int fx = rng_range(&r, 3, CHUNK_W - 4);
        int sub = biome_foe(mat, &r);
        int fy = B->surf[ox + fx] - 1;
        if (danger && F > 0 && k < 2) { fx = cx0 + 3 + k * 5; fy = F - 2; }
        if (sub == F_MOTH || sub == F_GERM || sub == F_GERM2 || sub == F_SPARK || sub == F_BUMBLE || sub == F_BUZZER) fy -= 4;
        foe(sub, ox + fx, fy);
    }
    if ((dd_sv.glints_total >= 333 || dd_flag(FL_MET_QUEEN)) && rng_chance(&r, 25)) {
        int fx = rng_range(&r, 4, CHUNK_W - 4);
        foe(F_LANCER, ox + fx, B->surf[ox + fx] - 1);
    }
    if (dd_sv.glints_total >= 667 && rng_chance(&r, 18)) {
        int fx = rng_range(&r, 4, CHUNK_W - 4);
        foe(F_TINMOUSE, ox + fx, B->surf[ox + fx] - 1);
    }
    /* things to carry */
    if (rng_chance(&r, 50)) {
        int bx = rng_range(&r, 3, CHUNK_W - 4);
        obj(O_PEBBLE, ox + bx, B->surf[ox + bx] - 1);
    }
    if (rng_chance(&r, 14)) {
        int bx = rng_range(&r, 3, CHUNK_W - 4);
        obj(biome_block(mat, &r), ox + bx, B->surf[ox + bx] - 1);
    }
    if (rng_chance(&r, 9)) {
        int bx = rng_range(&r, 3, CHUNK_W - 4);
        obj(O_HEARTBOX, ox + bx, B->surf[ox + bx] - 1);
    }
    /* someone who knows the way */
    if (!danger && rng_chance(&r, 12)) {
        int nx = rng_range(&r, 4, CHUNK_W - 5);
        int j = spawning ? put(EK_NPC, N_NATIVE, ox + nx, B->surf[ox + nx] - 1) : -1;
        if (j >= 0) { dd_ent[j].param = (int32_t)(B->ckey[i] & 0x7FFFFFFF); dd_ent[j].param2 = mat; }
    }
}

/* ------------------------------------------------------------------ */
/* building a level                                                      */

static uint8_t tbuf[DEPTH_MAX + 1][LV_MAXW * LV_MAXH];
static uint8_t bbuf[DEPTH_MAX + 1][LV_MAXW * LV_MAXH];
static uint8_t sbuf[DEPTH_MAX + 1][LV_MAXW];

bool dd_strip_run(const Level *L, int row, int x, int *x0, int *n) {
    int t = lv_tile(L, x, row);
    if (!(DD_TILE[t].flags & (TF_SOLID | TF_ONEWAY)) || !DD_TILE[t].mat) return false;
    if (DD_TILE[lv_tile(L, x, row - 1)].flags & TF_SOLID) return false;
    int a = x, b = x;
    while (a - 1 >= 0 && a > x - STRIP_MAX / 2) {
        int u = lv_tile(L, a - 1, row);
        if (!(DD_TILE[u].flags & (TF_SOLID | TF_ONEWAY)) || !DD_TILE[u].mat || (DD_TILE[lv_tile(L, a - 1, row - 1)].flags & TF_SOLID)) break;
        a--;
    }
    while (b + 1 < L->w && b - a + 1 < STRIP_MAX) {
        int u = lv_tile(L, b + 1, row);
        if (!(DD_TILE[u].flags & (TF_SOLID | TF_ONEWAY)) || !DD_TILE[u].mat || (DD_TILE[lv_tile(L, b + 1, row - 1)].flags & TF_SOLID)) break;
        b++;
    }
    *x0 = a;
    *n = b - a + 1;
    return true;
}

static uint32_t level_key(const LevelDesc *d) {
    if (d->kind == LV_AREA) return dd_hash(0xA0u, d->id);
    if (d->kind == LV_SPECIAL) return dd_hash(0x5Eu, d->id);
    return dd_hash(dd_hash(d->key, (uint32_t)d->row), 0x51u);
}

static void level_alloc(Level *L, int slot, int w, int h) {
    L->w = w;
    L->h = h;
    L->t = tbuf[slot];
    L->bg = bbuf[slot];
    L->surf = sbuf[slot];
    memset(L->t, 0, (size_t)(w * h));
    memset(L->bg, 0, (size_t)(w * h));
    memset(L->surf, 0, (size_t)w);
}

static int build_slot;

void dd_build(Level *out, const LevelDesc *d, const Level *parent) {
    Level *saveB = B;
    B = out;
    out->d = *d;
    out->key = level_key(d);
    spawning = false;
    if (d->kind == LV_AREA) {
        const AreaMap *A = &DD_AREA[d->id];
        level_alloc(out, build_slot, A->w, A->h);
        build_area(d->id);
    } else if (d->kind == LV_SPECIAL) {
        const AreaMap *A = &DD_SPECIAL[d->id];
        level_alloc(out, build_slot, A->w, A->h);
        build_special(d->id);
    } else {
        level_alloc(out, build_slot, d->n * CHUNK_W, CHUNK_H);
        row_key = dd_hash(d->key, (uint32_t)d->row);
        int area = strip_area(d);
        for (int i = 0; i < d->n; i++) {
            int px = d->x0 + i;
            int mat = parent ? dd_level_material(parent, px, d->row) : M_WOOD;
            out->biome[i] = (uint8_t)mat;
            out->ckey[i] = dd_hash(row_key, (uint32_t)(d->pabs + px));
            int part = 0;
            out->town[i] = (int8_t)(area >= 0 && d->scale == SC_MICRO ? town_lookup(area, d->row, px, &part) : -1);
            out->town_part[i] = (int8_t)part;
        }
        for (int i = 0; i < d->n; i++) {
            OX = i * CHUNK_W;
            if (out->town[i] >= 0) {
                out->cave_x0[i] = out->cave_x1[i] = out->cave_floor[i] = -1;
                out->gap_x[i] = out->gap_y[i] = -1;
                out->special[i] = 0;
                town_chunk(out->town[i], out->town_part[i]);
            } else gen_chunk(d, i);
        }
        /* the ends of the strip: the edge of the material */
        fill(0, 0, 0, CHUNK_H - 1, biome_tile(out->biome[0], 0));
        fill(out->w - 1, 0, out->w - 1, CHUNK_H - 1, biome_tile(out->biome[d->n - 1], 0));
    }
    B = saveB;
}

void dd_spawn_level(void) {
    if (dd_scale == SC_FULL) return; /* at full size only Dot and Dash move */
    B = &dd_lv;
    spawning = true;
    SPX = (int)dd_p.x;
    SPY = (int)dd_p.y;
    const LevelDesc *d = &dd_lv.d;
    if (d->kind == LV_AREA || d->kind == LV_SPECIAL) {
        place_key = dd_lv.key;
        place_n = 0;
        if (d->kind == LV_AREA) build_area(d->id);
        else build_special(d->id);
    } else {
        for (int i = 0; i < d->n; i++) {
            OX = i * CHUNK_W;
            if (dd_lv.town[i] >= 0) {
                place_key = dd_lv.ckey[i];
                place_n = 0;
                town_chunk(dd_lv.town[i], dd_lv.town_part[i]);
            } else spawn_chunk(d, i);
        }
    }
    spawning = false;
}

/* The chain of sizes down to the level Dot is in: chain[k] lives in buffer
 * slot k, and dd_lv is a copy of the deepest one (sharing its tiles). */
static Level chain[DEPTH_MAX];

static void build_depth(int k) {
    build_slot = k;
    dd_build(&chain[k], &dd_stack[k], k > 0 ? &chain[k - 1] : NULL);
}

void dd_world_init(void) {
    for (int k = 0; k <= dd_depth; k++) build_depth(k);
    dd_lv = chain[dd_depth];
    dd_scale = dd_stack[dd_depth].scale;
}

/* go one size down into a level described by d (its parent is dd_lv) */
void dd_push_level(const LevelDesc *d) {
    chain[dd_depth] = dd_lv;
    dd_depth++;
    dd_stack[dd_depth] = *d;
    build_depth(dd_depth);
    dd_lv = chain[dd_depth];
    dd_scale = d->scale;
}

/* back up one size: the parent is still built */
void dd_pop_level(void) {
    if (dd_depth == 0) return;
    dd_depth--;
    dd_lv = chain[dd_depth];
    dd_scale = dd_stack[dd_depth].scale;
}

/* replace the level at this depth (doors between places of one size) */
void dd_replace_level(const LevelDesc *d) {
    dd_stack[dd_depth] = *d;
    build_depth(dd_depth);
    dd_lv = chain[dd_depth];
    dd_scale = d->scale;
}

const Level *dd_parent_level(void) { return dd_depth > 0 ? &chain[dd_depth - 1] : NULL; }


/* ------------------------------------------------------------------ */
/* names and places                                                     */

const char *dd_place_name(void) {
    static char buf[48];
    const LevelDesc *d = &dd_lv.d;
    if (d->kind == LV_AREA) return d->id == AR_ROOM && dd_scale == SC_FULL ? "THE LUMBER ROOM" : DD_AREA[d->id].name;
    if (d->kind == LV_SPECIAL) return DD_SPECIAL[d->id].name;
    int c = dd_chunk_of((int)dd_p.x);
    if (dd_lv.town[c] >= 0) return TOWN[(int)dd_lv.town[c]].name;
    snprintf(buf, sizeof buf, "MICRO %s", dd_biome_name(dd_lv.biome[c]));
    return buf;
}

int dd_chunk_of(int px) {
    if (dd_lv.d.kind != LV_STRIP) return 0;
    return iclamp(px / (CHUNK_W * DD_TS), 0, dd_lv.d.n - 1);
}

int dd_music_for_level(void) {
    const LevelDesc *d = &dd_lv.d;
    if (dd_scale == SC_FULL) return MU_ROOM;
    if (d->kind == LV_AREA) return DD_AREA[d->id].music;
    if (d->kind == LV_SPECIAL) return DD_SPECIAL[d->id].music;
    int c = dd_chunk_of((int)dd_p.x);
    if (dd_lv.town[c] >= 0) return TOWN[(int)dd_lv.town[c]].music;
    if (dd_lv.special[c]) return MU_CAVE;
    switch (dd_lv.biome[c]) {
    case M_SOIL: case M_PLANT: case M_CERAMIC: case M_FUR: return MU_MICRO2;
    case M_METAL: case M_GLASS: case M_DUST: case M_FABRIC: return MU_MICRO3;
    default: return MU_MICRO;
    }
}

int dd_town_id(const char *name) {
    for (int k = 0; k < TW_COUNT; k++) {
        char low[32];
        int n = 0;
        for (const char *p = TOWN[k].name; *p && n < 31; p++)
            if (*p != ' ' && *p != '\'') low[n++] = (char)(*p >= 'A' && *p <= 'Z' ? *p + 32 : *p);
        low[n] = 0;
        if (!strcmp(low, name)) return k;
    }
    return -1;
}
void dd_town_where(int k, int *area, int *row, int *x) {
    *area = TOWN[k].area;
    *row = TOWN[k].row;
    *x = TOWN[k].x;
}
int dd_town_count(void) { return TW_COUNT; }
const char *dd_town_name(int k) { return k >= 0 && k < TW_COUNT ? TOWN[k].name : ""; }
int dd_extra_count(void) { return ARRAY_LEN(EXTRA); }
void dd_extra(int k, int *area, int *row, int *x, int *type, int *param) {
    *area = EXTRA[k].area; *row = EXTRA[k].row; *x = EXTRA[k].x; *type = EXTRA[k].type; *param = EXTRA[k].param;
}

/* Named spots for tests and the demo player, as a tile Dot stands in. */
bool dd_find_place(const char *name, int *tx, int *ty) {
    /* entities by kind name: "npc:N", "door:D", "obj:O", "foe:F", "pick:P" */
    int want_kind = -1, want_sub = -1;
    if (!strncmp(name, "npc:", 4)) { want_kind = EK_NPC; want_sub = atoi(name + 4); }
    else if (!strncmp(name, "door:", 5)) { want_kind = EK_DOOR; want_sub = atoi(name + 5); }
    else if (!strncmp(name, "obj:", 4)) { want_kind = EK_OBJ; want_sub = atoi(name + 4); }
    else if (!strncmp(name, "foe:", 4)) { want_kind = EK_FOE; want_sub = atoi(name + 4); }
    else if (!strncmp(name, "pick:", 5)) { want_kind = EK_PICK; want_sub = atoi(name + 5); }
    else if (!strncmp(name, "stand:", 6)) { want_kind = EK_STAND; want_sub = atoi(name + 6); }
    else if (!strncmp(name, "drip:", 5)) { want_kind = EK_DRIP; want_sub = atoi(name + 5); }
    if (want_kind >= 0) {
        for (int i = 0; i < DD_MAX_ENTS; i++) {
            const Ent *e = &dd_ent[i];
            if (!e->alive || e->kind != want_kind) continue;
            if (want_kind == EK_DOOR ? e->param != want_sub : e->sub != want_sub) continue;
            *tx = (int)((e->x + e->w / 2) / DD_TS);
            *ty = (int)((e->y + e->h - 1) / DD_TS);
            return true;
        }
        return false;
    }
    if (!strncmp(name, "town:", 5)) {
        int k = dd_town_id(name + 5);
        if (k < 0 || dd_lv.d.kind != LV_STRIP) return false;
        for (int c = 0; c < dd_lv.d.n; c++)
            if (dd_lv.town[c] == k && dd_lv.town_part[c] == 0) {
                *tx = c * CHUNK_W + 20;
                *ty = TOWN_H - 1;
                return true;
            }
        return false;
    }
    if (!strncmp(name, "chunk:", 6)) {
        /* chunk:PARENTX : the middle of the chunk above that parent tile */
        int px = atoi(name + 6), c = px - dd_lv.d.x0;
        if (dd_lv.d.kind != LV_STRIP || c < 0 || c >= dd_lv.d.n) return false;
        *tx = c * CHUNK_W + 20;
        *ty = dd_lv.surf[*tx] - 1;
        return true;
    }
    if (!strncmp(name, "objat:", 6)) {
        /* objat:SUB:PARENTX : a thing of that kind in the chunk over that parent tile */
        int sub = atoi(name + 6);
        const char *c2 = strchr(name + 6, ':');
        if (!c2 || dd_lv.d.kind != LV_STRIP) return false;
        int c = atoi(c2 + 1) - dd_lv.d.x0;
        for (int i = 0; i < DD_MAX_ENTS; i++) {
            const Ent *e = &dd_ent[i];
            if (!e->alive || e->kind != EK_OBJ || e->sub != sub || (int)(e->x / (CHUNK_W * DD_TS)) != c) continue;
            *tx = (int)((e->x + e->w / 2) / DD_TS);
            *ty = (int)((e->y + e->h - 1) / DD_TS);
            return true;
        }
        return false;
    }
    if (!strncmp(name, "gap:", 4)) {
        /* gap:PARENTX : the tile inside the nook behind a one-tile gap */
        int px = atoi(name + 4), c = px - dd_lv.d.x0;
        if (dd_lv.d.kind != LV_STRIP || c < 0 || c >= dd_lv.d.n || dd_lv.gap_x[c] < 0) return false;
        *tx = c * CHUNK_W + dd_lv.gap_x[c];
        *ty = dd_lv.gap_y[c];
        return true;
    }
    if (!strncmp(name, "cave:", 5)) {
        int px = atoi(name + 5), c = px - dd_lv.d.x0;
        if (dd_lv.d.kind != LV_STRIP || c < 0 || c >= dd_lv.d.n || dd_lv.cave_floor[c] < 0) return false;
        *tx = c * CHUNK_W + dd_lv.cave_x0[c] + 1;
        *ty = dd_lv.cave_floor[c] - 1;
        return true;
    }
    return false;
}
