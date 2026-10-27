/* HELLSPUR - the world: the Territory of Calico (128 x 96 tiles), the
 * Scorched Reach in Hell, eleven towns and the small places, all laid out
 * for UFO 40, and twenty dungeon floors built by a seeded generator from a
 * table of what each one holds. Nothing here comes from the original's maps:
 * only the structure does (how many towns and dungeons, which way they gate
 * one another, roughly where they sit). */
#include "spur.h"

/* ---- the maps ------------------------------------------------------------- */
/*  name, kind, parent, door on the parent (x, y), train stop */
const SpurMapInfo SPUR_MAPINFO[M_COUNT] = {
    [M_WORLD] = {"THE TERRITORY", MK_WORLD, M_WORLD, 0, 0, 0},
    [M_HELL] = {"THE SCORCHED REACH", MK_WORLD, M_WORLD, 24, 27, 0},
    [M_SALOON] = {"THE GOLDEN GARTER", MK_PLACE, M_WORLD, 16, 74, 0},
    [M_GLADWELL] = {"GLADWELL", MK_TOWN, M_WORLD, 16, 74, 1},
    [M_MERIDIAN] = {"MERIDIAN", MK_TOWN, M_WORLD, 40, 80, 2},
    [M_STCLARE] = {"ST. CLARE MISSION", MK_TOWN, M_WORLD, 24, 64, 0},
    [M_HOPE] = {"HOPE SPRINGS", MK_TOWN, M_WORLD, 80, 72, 3},
    [M_STEEPLE] = {"STEEPLETON", MK_TOWN, M_WORLD, 84, 40, 4},
    [M_FORT] = {"FORT PATIENCE", MK_TOWN, M_WORLD, 68, 30, 0},
    [M_DOLORES] = {"DOLORES", MK_TOWN, M_WORLD, 100, 32, 0},
    [M_HOLLOW] = {"HOLLOW GAP", MK_TOWN, M_WORLD, 44, 12, 0},
    [M_LASTCHANCE] = {"LAST CHANCE", MK_TOWN, M_WORLD, 14, 30, 5},
    [M_ASHMARKET] = {"ASHMARKET", MK_TOWN, M_WORLD, 120, 44, 0},
    [M_HERMITAGE] = {"THE HERMITAGE", MK_TOWN, M_WORLD, 100, 85, 0},
    [M_PIKESHACK] = {"PIKE'S SHACK", MK_PLACE, M_WORLD, 8, 78, 0},
    [M_ROOST] = {"THE OUTLAW ROOST", MK_PLACE, M_WORLD, 106, 14, 0},
    [M_THORNS] = {"OUR LADY OF THORNS", MK_PLACE, M_WORLD, 76, 16, 0},
    [M_WIDOW] = {"THE WIDOW'S HOUSE", MK_PLACE, M_WORLD, 20, 46, 0},
    [M_SHRINE] = {"THE AGAVE SHRINE", MK_PLACE, M_WORLD, 34, 35, 0},
    [M_SKYBAR] = {"THE SKY BAR", MK_PLACE, M_WORLD, 32, 53, 0},
    [M_SMITH] = {"THE DEMON SMITHY", MK_PLACE, M_WORLD, 7, 65, 0},
    [M_DRYWASH] = {"DRY WASH CAVE", MK_DUNGEON, M_WORLD, 54, 88, 0},
    [M_SIXPENNY] = {"SIXPENNY MINE", MK_DUNGEON, M_WORLD, 34, 70, 0},
    [M_PEN1] = {"THE DEEP PEN", MK_DUNGEON, M_WORLD, 104, 60, 0},
    [M_PEN2] = {"THE DEEP PEN, LOWER", MK_DUNGEON, M_WORLD, 104, 60, 0},
    [M_NOTCH] = {"WIDOW'S NOTCH", MK_DUNGEON, M_WORLD, 84, 56, 0},
    [M_CRYPT] = {"THE CRYPT", MK_DUNGEON, M_WORLD, 76, 16, 0},
    [M_ROOSTMINE] = {"THE ROOST MINE", MK_DUNGEON, M_WORLD, 106, 14, 0},
    [M_HOLLOWMINE] = {"HOLLOW GAP MINE", MK_DUNGEON, M_WORLD, 38, 6, 0},
    [M_CUT] = {"SMUGGLERS' CUT", MK_DUNGEON, M_WORLD, 62, 44, 0},
    [M_SNAKE1] = {"THE SNAKE TUNNELS", MK_DUNGEON, M_WORLD, 46, 48, 0},
    [M_SNAKE2] = {"THE DEEP TUNNELS", MK_DUNGEON, M_WORLD, 6, 42, 0},
    [M_DREAM1] = {"THE DREAMING", MK_DUNGEON, M_WORLD, 34, 35, 0},
    [M_DREAM2] = {"THE DREAMING, DEEP", MK_DUNGEON, M_WORLD, 34, 35, 0},
    [M_TOWER1] = {"THE BONE TOWER", MK_DUNGEON, M_HELL, 32, 6, 0},
    [M_TOWER2] = {"THE BONE TOWER, 2", MK_DUNGEON, M_HELL, 32, 6, 0},
    [M_TOWER3] = {"THE BONE TOWER, 3", MK_DUNGEON, M_HELL, 32, 6, 0},
    [M_TOWER4] = {"THE BONE TOWER, TOP", MK_DUNGEON, M_HELL, 32, 6, 0},
    [M_SPUR1] = {"HELLSPUR", MK_DUNGEON, M_WORLD, 16, 12, 0},
    [M_SPUR2] = {"HELLSPUR, UPPER", MK_DUNGEON, M_WORLD, 16, 12, 0},
    [M_SPUR3] = {"HELLSPUR, SUMMIT", MK_PLACE, M_WORLD, 16, 12, 0},
    [M_INSIDE] = {"INDOORS", MK_PLACE, M_WORLD, 0, 0, 0},
};

const char *spur_inside_name(void);
const char *spur_map_name(int id) {
    if (id == M_INSIDE && sg.map_id == M_INSIDE) return spur_inside_name();
    return id >= 0 && id < M_COUNT ? SPUR_MAPINFO[id].name : "";
}

/* ---- a seeded hash for the painters (never the game's RNG) ------------------ */
static uint32_t hsh(uint32_t a, uint32_t b, uint32_t c) {
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u ^ (c + 0x165667B1u) * 0xC2B2AE3Du;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    h *= 0x297A2D39u;
    h ^= h >> 15;
    return h;
}
static int noise(int x, int y, int seed) { return (int)(hsh((uint32_t)x, (uint32_t)y, (uint32_t)seed) & 255); }

typedef struct { uint32_t s; } DRng;
static int drand(DRng *r, int lo, int hi) {
    r->s = r->s * 1664525u + 1013904223u;
    uint32_t v = hsh(r->s, 77, 3);
    return hi <= lo ? lo : lo + (int)(v % (uint32_t)(hi - lo + 1));
}

/* ---- painting -------------------------------------------------------------------- */
static void put(SpurMap *m, int x, int y, int t) {
    if (x >= 0 && y >= 0 && x < m->w && y < m->h) m->tile[y][x] = (uint8_t)t;
}
static int get(const SpurMap *m, int x, int y) {
    if (x < 0 || y < 0 || x >= m->w || y >= m->h) return -1;
    return m->tile[y][x];
}
static void rectf(SpurMap *m, int x0, int y0, int x1, int y1, int t) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) put(m, x, y, t);
}
/* an ellipse; with rough > 0 its edge frays by the noise */
static void ellf(SpurMap *m, int cx, int cy, int rx, int ry, int t, int rough, int seed) {
    for (int y = cy - ry - 2; y <= cy + ry + 2; y++)
        for (int x = cx - rx - 2; x <= cx + rx + 2; x++) {
            int dx = (x - cx) * 100 / imax(1, rx), dy = (y - cy) * 100 / imax(1, ry);
            int d = dx * dx + dy * dy;
            int lim = 10000 + (rough ? (noise(x, y, seed) - 128) * rough * 30 : 0);
            if (d <= lim) put(m, x, y, t);
        }
}
/* replace one tile with another inside a box, where the noise says */
static void sprinkle(SpurMap *m, int x0, int y0, int x1, int y1, int from, int to, int chance, int seed) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (get(m, x, y) == from && noise(x, y, seed) < chance) put(m, x, y, to);
}
/* clumps: a smoothed noise field */
static int field(int x, int y, int seed) {
    int s = 0;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) s += noise((x + dx) / 2, (y + dy) / 2, seed);
    return s / 9;
}
static void clumps(SpurMap *m, int x0, int y0, int x1, int y1, int from, int to, int above, int seed) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (get(m, x, y) == from && field(x, y, seed) > above) put(m, x, y, to);
}
/* a road: an L between two points, laid over walkable ground only */
static void road(SpurMap *m, int x0, int y0, int x1, int y1) {
    int x = x0, y = y0;
    while (x != x1) {
        if (spur_tile_walk(get(m, x, y)) && get(m, x, y) < T_TOWN) put(m, x, y, T_ROAD);
        x += isign(x1 - x);
    }
    while (y != y1) {
        if (spur_tile_walk(get(m, x, y)) && get(m, x, y) < T_TOWN) put(m, x, y, T_ROAD);
        y += isign(y1 - y);
    }
}
static void ring(SpurMap *m, int x0, int y0, int x1, int y1, int t) {
    for (int x = x0; x <= x1; x++) { put(m, x, y0, t); put(m, x, y1, t); }
    for (int y = y0; y <= y1; y++) { put(m, x0, y, t); put(m, x1, y, t); }
}

/* ---- the Territory's places -------------------------------------------------------- */
typedef struct { int8_t x, y; uint8_t map; int8_t end; } Poi; /* end: 0 start, 1 back */
static const Poi WORLD_POI[] = {
    {16, 74, M_GLADWELL, 0}, {40, 80, M_MERIDIAN, 0}, {24, 64, M_STCLARE, 0}, {80, 72, M_HOPE, 0},
    {84, 40, M_STEEPLE, 0}, {68, 30, M_FORT, 0}, {100, 32, M_DOLORES, 0}, {44, 12, M_HOLLOW, 0},
    {14, 30, M_LASTCHANCE, 0}, {120, 44, M_ASHMARKET, 0}, {100, 85, M_HERMITAGE, 0},
    {8, 78, M_PIKESHACK, 0}, {106, 14, M_ROOST, 0}, {76, 16, M_THORNS, 0}, {20, 46, M_WIDOW, 0},
    {34, 35, M_SHRINE, 0}, {32, 53, M_SKYBAR, 0}, {7, 65, M_SMITH, 0},
    {54, 88, M_DRYWASH, 0}, {34, 70, M_SIXPENNY, 0}, {104, 60, M_PEN1, 0},
    {84, 56, M_NOTCH, 0}, {84, 51, M_NOTCH, 1}, {59, 12, M_ROOSTMINE, 1}, {38, 6, M_HOLLOWMINE, 0},
    {62, 44, M_CUT, 0}, {59, 38, M_CUT, 1}, {46, 48, M_SNAKE1, 0}, {38, 44, M_SNAKE1, 1},
    {6, 42, M_SNAKE2, 0}, {6, 34, M_SNAKE2, 1}, {24, 27, M_HELL, 0}, {16, 12, M_SPUR1, 0},
};
#define WORLD_POIS ARRAY_LEN(WORLD_POI)

static int poi_tile(int map) {
    switch (SPUR_MAPINFO[map].kind) {
    case MK_TOWN: return map == M_FORT ? T_FORT : map == M_STCLARE || map == M_HERMITAGE ? T_MISSION : T_TOWN;
    case MK_PLACE:
        if (map == M_THORNS) return T_MISSION;
        if (map == M_SHRINE) return T_SHRINE;
        if (map == M_SKYBAR) return T_SALOONSKY;
        if (map == M_SMITH) return T_SMITHY;
        return T_HOUSE;
    default:
        if (map == M_HELL) return T_GATE;
        if (map == M_SPUR1) return T_CITADEL;
        if (map == M_SIXPENNY || map == M_HOLLOWMINE) return T_MINE;
        if (map == M_PEN1) return T_TOWER;
        return T_CAVE;
    }
}

int spur_poi_at(int map, int x, int y) {
    if (map == M_WORLD) {
        for (int i = 0; i < (int)WORLD_POIS; i++)
            if (WORLD_POI[i].x == x && WORLD_POI[i].y == y) return WORLD_POI[i].map;
    }
    return -1;
}

/* the iron spikes and the citadel's veil */
static const int8_t SPIKE_AT[3][2] = {{104, 7}, {33, 30}, {118, 70}};

static void build_world(SpurMap *m) {
    m->w = 128;
    m->h = 96;
    rectf(m, 0, 0, 127, 95, T_SEA);
    /* the land, with a frayed coast */
    for (int y = 2; y <= 91; y++)
        for (int x = 2; x <= 113; x++) {
            int edge = imin(imin(x - 2, 113 - x), imin(y - 2, 91 - y));
            if (edge >= 2 || noise(x, y, 11) < 150 + edge * 50) put(m, x, y, T_GRASS);
        }
    /* bays */
    ellf(m, 112, 80, 5, 6, T_SEA, 2, 12);
    ellf(m, 2, 50, 4, 5, T_SEA, 2, 13);
    ellf(m, 86, 2, 8, 3, T_SEA, 2, 14);
    /* the Sweetgrass (south-west): prairie, scrub and groves */
    clumps(m, 3, 57, 58, 91, T_GRASS, T_PRAIRIE, 130, 21);
    clumps(m, 3, 57, 58, 91, T_GRASS, T_SCRUB, 140, 22);
    ellf(m, 10, 86, 5, 3, T_FOREST, 2, 23);
    ellf(m, 47, 64, 6, 3, T_FOREST, 2, 24);
    ellf(m, 28, 76, 3, 2, T_FOREST, 2, 25);
    sprinkle(m, 3, 57, 58, 91, T_GRASS, T_FLOWERS, 12, 26);
    /* the Bottoms (south-east): grass to the west, swamp to the east */
    clumps(m, 62, 57, 113, 91, T_GRASS, T_PRAIRIE, 135, 31);
    clumps(m, 88, 57, 113, 91, T_GRASS, T_SWAMP, 120, 32);
    clumps(m, 88, 57, 113, 91, T_PRAIRIE, T_SWAMP, 120, 32);
    ellf(m, 70, 86, 5, 3, T_FOREST, 2, 33);
    ellf(m, 92, 64, 3, 2, T_FOREST, 2, 34);
    sprinkle(m, 62, 57, 113, 91, T_SWAMP, T_DEADTREE, 14, 35);
    /* the High Mesa (north-east): sand, scrub, cactus and mesas */
    for (int y = 2; y <= 50; y++)
        for (int x = 63; x <= 113; x++)
            if (get(m, x, y) == T_GRASS) put(m, x, y, T_SAND);
    clumps(m, 63, 2, 113, 50, T_SAND, T_SCRUB, 140, 41);
    sprinkle(m, 63, 2, 113, 50, T_SAND, T_CACTUS, 16, 42);
    ellf(m, 92, 44, 3, 2, T_MESA, 1, 43);
    ellf(m, 74, 24, 2, 3, T_MESA, 1, 44);
    ellf(m, 96, 22, 3, 2, T_MESA, 1, 45);
    ellf(m, 110, 28, 2, 4, T_MESA, 1, 46);
    rectf(m, 70, 44, 80, 50, T_FOREST); /* the woods, where the big ones roam */
    sprinkle(m, 70, 44, 80, 50, T_FOREST, T_PINE, 90, 47);
    /* Hollow Gap's valley (north, middle) */
    for (int y = 2; y <= 22; y++)
        for (int x = 36; x <= 58; x++)
            if (get(m, x, y) == T_GRASS) put(m, x, y, T_DIRT);
    clumps(m, 36, 2, 58, 22, T_DIRT, T_SCRUB, 135, 51);
    sprinkle(m, 36, 2, 58, 22, T_DIRT, T_DEADTREE, 18, 52);
    /* the crater (north-west): bones and ash */
    for (int y = 2; y <= 22; y++)
        for (int x = 2; x <= 32; x++)
            if (get(m, x, y) == T_GRASS) put(m, x, y, (noise(x, y, 61) < 90) ? T_BONES : T_ASH);
    /* the Burnt Hills (west, middle) */
    for (int y = 26; y <= 50; y++)
        for (int x = 2; x <= 58; x++)
            if (get(m, x, y) == T_GRASS) put(m, x, y, noise(x / 3, y / 3, 71) < 110 ? T_ASH : T_DIRT);
    clumps(m, 2, 26, 58, 50, T_DIRT, T_HILL, 145, 72);
    sprinkle(m, 2, 26, 58, 50, T_ASH, T_DEADTREE, 14, 73);
    ellf(m, 50, 42, 2, 1, T_LAVA, 1, 74);
    ellf(m, 10, 47, 2, 1, T_LAVA, 1, 75);
    ellf(m, 28, 34, 1, 1, T_LAVA, 0, 76);

    /* the ranges between the regions (mountains: on foot, never; on wings, yes) */
    rectf(m, 2, 51, 113, 56, T_MOUNT);   /* the southern range */
    rectf(m, 59, 2, 62, 56, T_MOUNT);    /* the spine */
    rectf(m, 2, 23, 58, 25, T_MOUNT);    /* north of the Burnt Hills */
    rectf(m, 33, 2, 35, 22, T_MOUNT);    /* crater from valley */
    rectf(m, 39, 26, 41, 50, T_MOUNT);   /* the Burnt Hills: east from west */
    rectf(m, 2, 38, 38, 40, T_MOUNT);    /* ... and north from south */
    /* foothills soften the ranges' faces */
    for (int y = 1; y < 95; y++)
        for (int x = 1; x < 127; x++) {
            if (get(m, x, y) != T_MOUNT) continue;
            for (int k = 0; k < 4; k++) {
                int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
                int t = get(m, nx, ny);
                if ((t == T_GRASS || t == T_PRAIRIE || t == T_SAND || t == T_DIRT || t == T_ASH) && noise(nx, ny, 81) < 70)
                    put(m, nx, ny, T_HILL);
            }
        }
    /* the Wide River, Sweetgrass from the Bottoms, with the Mule Crossing ferry */
    for (int y = 57; y <= 93; y++) {
        int w = noise(0, y, 91) < 60 ? 1 : 0;
        rectf(m, 59 - w, y, 61, y, T_RIVER);
    }
    rectf(m, 58, 76, 62, 76, T_FERRY);
    put(m, 57, 76, T_ROAD); put(m, 63, 76, T_ROAD);
    /* the Burnt Hills' lake, with the widow's house on its island */
    ellf(m, 20, 46, 6, 3, T_RIVER, 1, 101);
    ellf(m, 20, 46, 2, 1, T_GRASS, 0, 0);
    rectf(m, 20, 47, 20, 49, T_BRIDGE);
    put(m, 20, 50, T_DIRT);
    /* the flight-only places */
    ellf(m, 120, 44, 6, 5, T_ASH, 2, 111);              /* Ashmarket's island */
    ellf(m, 118, 70, 2, 2, T_SAND, 0, 0);                /* a spike's islet */
    ellf(m, 40, 93, 7, 2, T_SCRUB, 1, 113);              /* the bluff */
    rectf(m, 95, 80, 105, 90, T_MOUNT);                  /* the hermitage's hidden valley */
    rectf(m, 97, 82, 103, 88, T_GRASS);
    sprinkle(m, 97, 82, 103, 88, T_GRASS, T_FLOWERS, 40, 114);
    rectf(m, 3, 60, 11, 70, T_MOUNT);                    /* the smithy's pit */
    rectf(m, 5, 63, 9, 67, T_ASH);
    rectf(m, 99, 3, 109, 11, T_MESA);                    /* a spike on a mesa top */
    rectf(m, 101, 5, 107, 9, T_SAND);
    rectf(m, 29, 27, 38, 34, T_RIVER);                   /* a spike in a lake */
    rectf(m, 31, 29, 35, 31, T_SAND);
    rectf(m, 27, 52, 37, 54, T_CLOUD);                   /* a cloud caught on the range */
    for (int i = 0; i < 3; i++) put(m, SPIKE_AT[i][0], SPIKE_AT[i][1], spur_flag(F_SPIKE1 + i) ? T_SPIKE_GONE : T_SPIKE);
    /* the citadel and its veil */
    rectf(m, 12, 8, 20, 16, T_BONES);
    put(m, 16, 12, T_CITADEL);
    bool veil = !(spur_flag(F_SPIKE1) && spur_flag(F_SPIKE2) && spur_flag(F_SPIKE3));
    if (veil) { ring(m, 13, 9, 19, 15, T_BARRIER); ring(m, 14, 10, 18, 14, T_BARRIER); }
    /* the gates the story opens */
    put(m, 84, 57, spur_flag(F_AMBROSE) ? T_DIRT : T_ANGELS);
    put(m, 63, 44, spur_flag(F_CUT_OPEN) ? T_DIRT : T_RUBBLE);
    put(m, 64, 44, T_DIRT);
    /* the places */
    for (int i = 0; i < (int)WORLD_POIS; i++) put(m, WORLD_POI[i].x, WORLD_POI[i].y, poi_tile(WORLD_POI[i].map));
    /* clear ground round every door that stands in the open */
    for (int i = 0; i < (int)WORLD_POIS; i++) {
        int x = WORLD_POI[i].x, y = WORLD_POI[i].y;
        for (int k = 0; k < 4; k++) {
            int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
            int t = get(m, nx, ny);
            if (t == T_FOREST || t == T_PINE || t == T_CACTUS || t == T_DEADTREE || t == T_LAVA || t == T_MESA) put(m, nx, ny, T_DIRT);
        }
    }
    /* roads */
    road(m, 16, 74, 24, 65);
    road(m, 16, 75, 40, 79);
    road(m, 41, 80, 57, 76);
    road(m, 63, 76, 80, 73);
    road(m, 80, 71, 84, 58);
    road(m, 84, 50, 84, 41);
    road(m, 83, 40, 69, 30);
    road(m, 85, 40, 100, 33);
    road(m, 14, 31, 23, 28);
    road(m, 15, 30, 33, 35);
    road(m, 104, 61, 81, 72);
    /* the hive in the Bottoms, the lightning tree on the Mesa */
    put(m, 86, 84, T_HIVE);
    put(m, 78, 46, T_DEADTREE);
    for (int k = 0; k < 4; k++) put(m, 86 + (k == 0) - (k == 1), 84 + (k == 2) - (k == 3), T_GRASS);

    /* links: every door opens its map */
    for (int i = 0; i < (int)WORLD_POIS && m->nlink < SPUR_MAX_LINK; i++)
        m->link[m->nlink++] = (SpurLink){WORLD_POI[i].x, WORLD_POI[i].y, WORLD_POI[i].map, (int8_t)(WORLD_POI[i].end ? -2 : -1), 0};
    m->zone = -1;
    /* Sage's animals out in the open */
    static const struct { int8_t x, y, a; } AN[] = {{26, 82, 0}, {96, 70, 2}, {92, 26, 4}, {50, 30, 8}, {50, 6, 9}};
    for (int i = 0; i < ARRAY_LEN(AN); i++) {
        put(m, AN[i].x, AN[i].y, T_DIRT);
        m->npc[m->nnpc++] = (SpurNpc){AN[i].x, AN[i].y, LOOK_ANIMAL, (uint8_t)AN[i].a, 0, 0, 0, -1, -1};
    }
    /* patches to rummage in */
    static const int8_t FORAGE[][2] = {{12, 68}, {46, 72}, {90, 80}, {96, 46}, {72, 20}, {56, 46}, {26, 43}};
    for (int i = 0; i < ARRAY_LEN(FORAGE) && m->nspot < SPUR_MAX_SPOT; i++) {
        put(m, FORAGE[i][0], FORAGE[i][1], T_HERB);
        m->spot[m->nspot++] = (SpurSpot){FORAGE[i][0], FORAGE[i][1], SPOT_FORAGE, (uint8_t)(i < 2 ? 0 : i < 5 ? 1 : 2)};
    }
    /* things hidden out in the open */
    m->chest[m->nchest++] = (SpurChest){78, 46, 0, IT_THUNDERBOW, 0, 1};  /* in the lightning tree */
    m->chest[m->nchest++] = (SpurChest){86, 84, 1, -1, 0, 1};            /* the hive (honey: a story find) */
    m->chest[m->nchest++] = (SpurChest){52, 6, 2, IT_WISHBONE, 0, 1};    /* by the condor's nest */
    m->chest[m->nchest++] = (SpurChest){70, 82, 3, IT_SOUPBONE, 0, 1};   /* a cow skull by the river */
    put(m, 52, 6, T_BONES);
    put(m, 70, 82, T_BONES);
}

/* the Scorched Reach: the walk from the Black Gate to the Bone Tower */
static void build_hell(SpurMap *m) {
    m->w = 64;
    m->h = 48;
    rectf(m, 0, 0, 63, 47, T_LAVA);
    static const int8_t WAY[][2] = {{32, 44}, {14, 38}, {12, 26}, {24, 20}, {46, 24}, {54, 14}, {40, 8}, {32, 7}};
    for (int i = 0; i + 1 < ARRAY_LEN(WAY); i++) {
        int x0 = WAY[i][0], y0 = WAY[i][1], x1 = WAY[i + 1][0], y1 = WAY[i + 1][1];
        int n = imax(iabs(x1 - x0), iabs(y1 - y0));
        for (int k = 0; k <= n; k++) {
            int x = x0 + (x1 - x0) * k / imax(1, n), y = y0 + (y1 - y0) * k / imax(1, n);
            ellf(m, x, y, 3, 2, T_ASH, 1, 200 + i);
        }
    }
    ellf(m, 32, 8, 5, 3, T_BONES, 1, 210);
    for (int k = 0; k <= 10; k++) ellf(m, 32 + k * 2, 44 - k * 4 / 5, 2, 2, T_ASH, 0, 0); /* the side way to the pocket */
    ellf(m, 50, 36, 4, 3, T_ASH, 1, 211);        /* a side pocket with a find */
    ellf(m, 44, 38, 4, 2, T_ASH, 1, 212);
    ellf(m, 38, 41, 3, 2, T_ASH, 1, 213);
    sprinkle(m, 0, 0, 63, 47, T_ASH, T_BONES, 50, 214);
    rectf(m, 30, 44, 34, 46, T_ASH);
    put(m, 32, 46, T_GATE);
    put(m, 32, 6, T_TOWER);
    put(m, 32, 7, T_BONES);
    m->link[m->nlink++] = (SpurLink){32, 46, M_WORLD, 24, 27};
    m->link[m->nlink++] = (SpurLink){32, 6, M_TOWER1, -1, 0};
    m->chest[m->nchest++] = (SpurChest){52, 36, 0, IT_HOLYOIL, 0, 0};
    m->chest[m->nchest++] = (SpurChest){10, 26, 1, 0, 2500, 0};
    put(m, 52, 36, T_CHEST);
    put(m, 10, 26, T_CHEST);
    m->zone = 16;
    m->start_x = 32;
    m->start_y = 45;
}

/* ---- the hand-made maps -------------------------------------------------------------- */
static const char *const MAP_GLADWELL[] = {
    "TTTTTTTTTTTTT++TTTTTTTTTTTTT",
    "T,,,,,,,,,,,,++,,,,,,,,,,,,T",
    "T,RRRRRR,,,,,++,,,RRRRRRR,,T",
    "T,RRRRRR,,,,,++,,,RRRRRRR,,T",
    "T,WwWCWW,,6,,++,,,WwWHWwW,,T",
    "T,,,,+,,,,,,,++,,,,,,+,,,,,T",
    "T,,,,+,,,,,,,++,,,,,,+,,,,,T",
    "T,,,++++++++++++++++++++,,,T",
    "T,,,,,,,,,0,,++,,,,,,,,,,,,T",
    "T,#..#..#,,,,++,,,RRRRRR,,,T",
    "T,......#,,,,++,,1RRRRRR,,,T",
    "T,#..#...,o,,++,,,WwWGWW,,,T",
    "+++++++++++++++,,,,,,+,,2,,T",
    "T,,,,,,,,,,,,+++++++++,,,,,T",
    "T,,3,,,,,,,,,++,,,,,,,,,,,,T",
    "T,ff,,,b,,4,,++,,5,,,,,,b,,T",
    "T,,,,,,,,,,,,@+,,,,,ff,,,,,T",
    "TTTTTTTTTTTTT++TTTTTTTTTTTTT",
    NULL,
};
static const char *const MAP_MERIDIAN[] = {
    "TTTTTTTTTTTTTTTT++TTTTTTTTTTTTTTTT",
    "T,,,,,,,,,,,,,,,++,,,,,,,,,,,,,,,T",
    "T,RRRRRRR,RRRRR,++,RRRRRRR,RRRRR,T",
    "T,RRRRRRR,RRRRR,++,RRRRRRR,RRRRR,T",
    "T,WwWHWwW,WwCwW,++,WwWNWwW,WwSwW,T",
    "T,,,,+,,,,,,+,,,++,,,,+,,,,,,+,,,T",
    "T,,,,+,,,,,,+,,,++,,,,+,,,,,,+,,,T",
    "T,0++++++++++++++++++++++++++++,,T",
    "T,,,,,,,,,,,,,,,++,,,,,,,8,,,,,1,T",
    "T,RRRRR,,RRRRR,,++,RRRRR,,RRRRRR,T",
    "T,RRRRR,,RRRRR,,++,RRRRR,,RRRRRR,T",
    "T,WwGwW,,WwUwW,,++,WwXwW,,WwWLWW,T",
    "T,,,+,,,7,,+,,,,++,,,+,,,,,,,+,,,T",
    "++++++++++++++++++++++++++++++++++",
    "T,,2,,+,,,,,,,,,++,,,,,,,,,,,,,,,T",
    "T,RRRRRRRR,,o,,,++,,,3,,,,,,,,,,,T",
    "T,RRRRRRRR,,,,,,++,,,,==,===,ff,,T",
    "T,WwWwYwWW,,4,,,++,,,,=,,,,=,,,,,T",
    "T,yyyyyyyy,,,,,,++,,,,=,5,,=,6,,,T",
    "T,,,,,,,,,,,,b,,++,,,,======,,,x,T",
    "T,,,,,,,,,,,,,,,@+,,,,,,,,,,,,,,,T",
    "TTTTTTTTTTTTTTTT++TTTTTTTTTTTTTTTT",
    NULL,
};
static const char *const MAP_STCLARE[] = {
    "########################",
    "#::::::RRRRRRRRRR::::::#",
    "#:o::::RRRRRRRRRR::::::#",
    "#::::::RRRRRRRRRR::::::#",
    "#::::::WwWwWCWwWW:::4::#",
    "#:::3::::::::::::::::::#",
    "#::::::::::::::::::::::#",
    "#:::::::::::::1::::::::#",
    "#:fffff::::::::::ggggg:#",
    "#:f,,,f::::::::::::::::#",
    "#:f,0,f::::::::::ggggg:#",
    "#:f,,,f::::::::::::::::#",
    "#:fffff::::::::::ggggg:#",
    "#::::::::::::::::::2:::#",
    "#::::::::::@:::::::::::#",
    "###########::###########",
    NULL,
};
static const char *const MAP_HOPE[] = {
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
    "T,RRRRRRR,RRRRR,,RRRRRR,RRRRRT",
    "T,RRRRRRR,RRRRR,,RRRRRR,RRRRRT",
    "T,RRRRRRR,RRRRR,,RRRRRR,RRRRRT",
    "T,WwWHWwW,WwCwW,,WwWQWW,WwSwWT",
    "T,,,,+,,,,,,+,,,,,,,0,,,,,+,,T",
    "T,,++++++++++++++++++++++++,,T",
    "T,,,,,,,,,,,,,,,,,,,,,,,,,,,,T",
    "T,,,1,,,o,,,,,,,,,,,,,,,2,,b,T",
    "T,,,,,,,,,,3,,,,,,4,,,,,,,,,,T",
    "++++++++++++++++++++++++++++++",
    "T,,,,,,,,,,,,,++,,,,,,,,,,,,,T",
    "T,RRRRR,RRRRR,++,,RRRRRRRRR7,T",
    "T,RRRRR,RRRRR,++,,RRRRRRRRR,,T",
    "T,WwGwW,WwUwW,++,,WwWwYwWwW,,T",
    "T,,,++++++++++++,,yyyy+yyyy,,T",
    "T,,,,,5,,,,,,,+++++++++,,,,,,T",
    "T,x,ffffff,,8,++,,,,,,,,,,b,,T",
    "T,,,,,,,,,,,,,@+,,,,6,,,,,,,,T",
    "TTTTTTTTTTTTTT++TTTTTTTTTTTTTT",
    NULL,
};
static const char *const MAP_STEEPLE[] = {
    "TTTTTTTTTTTTTTT++TTTTTTTTTTTTTTT",
    "T,RRRRRR,RRRRR,++,RRRRR,RRRRRR,T",
    "T,RRRRRR,RRRRR,++,RRRRR,RRRRRR,T",
    "T,RRRRRR,RRRRR,++,RRRRR,RRRRRR,T",
    "T,WwHwWW,WwCwW,++,WwGwW,WwWXWW,T",
    "T,,,+,,,,,,+,,,++,,,+,,,,,,+,,,T",
    "T,,++++++++++++++++++++++++++,,T",
    "T,,,,,,,,,,,,,,++,,,,,,,,,,,,,,T",
    "T,,,,,0,,,,,,,,++,,7,,,,,1,,,,,T",
    "T,,,,,,,,o,,,,,+++++++++++++++++",
    "T,RRRRRR,,,,,,,++,,,,,3,,,,,,,,T",
    "T,RRRRRR,,,,2,,++,,,,,,,,,,,,,,T",
    "T,WwWLWW,,,,,,,++,,RRRRRRRRRRR,T",
    "T,,,,++++++++++++,,RRRRRRRRRRR,T",
    "T,,,,,,,,,,,,,,++,,WwWwWYWwWwW,T",
    "T,,,,,,,==,===,++,,yyyyy+yyyyy,T",
    "T,bb,,,,=,4,,=,++++++++++,,,,,,T",
    "T,,,,,,,======,++,,,,,,,,,,6,,,T",
    "T,,,5,,,,,,,,,,@+,,,,,,,,,,,,x,T",
    "TTTTTTTTTTTTTTT++TTTTTTTTTTTTTTT",
    NULL,
};
static const char *const MAP_FORT[] = {
    "############################",
    "############################",
    "#:RRRRRRRR:RRRRRR::RRRRRRR:#",
    "#:RRRRRRRR:RRRRRR::RRRRRRR:#",
    "#:WwWwHwWW:WwWCWW::WwWGWwW:#",
    "#::::::::::::::::::::::::::#",
    "#:::::::::::::0:::::::::6::#",
    "#::7:::::::::::::::::::::::#",
    "#::::::::1:::::::::::::::::#",
    "#:::::::::.......::xxx:xxx:#",
    "#:gggggg::.......::::::::::#",
    "#:::::::::.......5:::::::::#",
    "#:gggggg::...o...:::::2::::#",
    "#:::::::::.......::::::::::#",
    "#:gggggg::.......::::::::::#",
    "#:::::::::.......::::::::::#",
    "#::::3:::::::::::::::4:::::#",
    "#::::::::::::::::::::::::::#",
    "#::::::::::::@:::::::::::::#",
    "#############::#############",
    NULL,
};
static const char *const MAP_DOLORES[] = {
    "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv",
    "v.RRRRRRR.RRRRRRRRR.RRRRR.RRRRRv",
    "v.RRRRRRR.RRRRRRRRR.RRRRR.RRRRRv",
    "v.RRRRRRR.RRRRRRRRR.RRRRR.RRRRRv",
    "v.WwWHWwW.WwWwSwWwW.WwCwW.WwDwWv",
    "v....+........+.......+........v",
    "v....+........+9......+........v",
    "v....+....0...+....3..+......6.v",
    "v....+........+.......+........v",
    "++++++++++++++++++++++++++++++++",
    "v..............++..............v",
    "v..............++..............v",
    "v..RRRRRR..o...++...RRRRR=====.v",
    "v..RRRRRR..1...++...RRRRR=...=.v",
    "v..WwWGWW......++...WwDwW=.2.=.v",
    "v.....+++++++++++........=...=.v",
    "v...........7..++........==.==.v",
    "v.x..4...v.....++.......5......v",
    "v..............@+...v........b.v",
    "vvvvvvvvvvvvvvv++vvvvvvvvvvvvvvv",
    NULL,
};
static const char *const MAP_HOLLOW[] = {
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTT",
    "T..........................T",
    "T.RRRRRRR.RRRRRR..RRRRRR...T",
    "T.RRRRRRR.RRRRRR..RRRRRR...T",
    "T.WwWHWwW.WwWGWW..WwWUWW...T",
    "T.......................x..T",
    "T...........6........3.....T",
    "T.....0....................T",
    "T...........o....1.........T",
    "T..........................T",
    "T..RRRRR...........RRRRRR..T",
    "T..RRRRR...........RRRRRR..T",
    "T..WwDwW...........WwWDWW..T",
    "T.........2..............5.T",
    "T.......b..................T",
    "T.x.4.................b....T",
    "T............@.............T",
    "TTTTTTTTTTTTT..TTTTTTTTTTTTT",
    NULL,
};
static const char *const MAP_LASTCHANCE[] = {
    "TTTTTTTTTTTTTTTT++TTTTTTTTTTTTTTTT",
    "T,RRRRRRR,RRRRR,++,RRRRRRR,RRRRR,T",
    "T,RRRRRRR,RRRRR,++,RRRRRRR,RRRRR,T",
    "T,RRRRRRR,RRRRR,++,RRRRRRR,RRRRR,T",
    "T,WwWHWwW,WwCwW,++,WwWNWwW,WwSwW,T",
    "T,,,,+,,,,,,+,,,++,,,,+,,,,,,+,,,T",
    "T,0++++++++++++++++++++++++++++,,T",
    "T,,,,,,8,,,,,,,,++,,,,,,,,,,,,,1,T",
    "T,RRRRR,RRRRRR,,++,RRRRR,,RRRRRR,T",
    "T,RRRRR,RRRRRR,,++,RRRRR,,RRRRRR,T",
    "T,WwGwW,WwWUWW,,++,WwXwW7,WwWLWW,T",
    "T,,,+,,,,,,+,,,,++,,,+,,,,,,,+,,,T",
    "++++++++++++++++++++++++++++++++++",
    "T,,,,,,,,2,,,,,,++,,,,,,,,,,,,,,,T",
    "T,,,,,,+,,,,,,,,++,,,3,,,,,,,,,,,T",
    "T,RRRRRRRRRR,o,,++,,,,===,====ff,T",
    "T,RRRRRRRRRR,,,,++,,,,=,,,,,,=,,,T",
    "T,WwWwWYWwWW4,,,++,,,,=,,5,,,=6,,T",
    "T,yyyyyyyyyy,,,,++,,,,=,,,,,,=,b,T",
    "T,,,,,,,,,,,,x,,++,,,,========,,,T",
    "T,,,,,,,,,,,,,,,@+,,,,,,,,,,,,,,,T",
    "TTTTTTTTTTTTTTTT++TTTTTTTTTTTTTTTT",
    NULL,
};
static const char *const MAP_ASHMARKET[] = {
    "##############################",
    "#.RRRRRRR.RRRRRRRRR.RRRRRRRR.#",
    "#.RRRRRRR.RRRRRRRRR.RRRRRRRR.#",
    "#.RRRRRRR.RRRRRRRRR.RRRRRRRR.#",
    "#.WwWHWwW.WwWwSwWwW.WwWwGwWW.#",
    "#............................#",
    "#............................#",
    "#.....0...........6....1.....#",
    "#............................#",
    "#..........~~~..~~~..........#",
    "#............................#",
    "#..RRRRRRR..........RRRRRRR..#",
    "#..RRRRRRR..........RRRRRRR..#",
    "#..WwWUWwW..2.......WwWDWwW..#",
    "#............................#",
    "#.................3..........#",
    "#.b...4..x..............5..b.#",
    "#............................#",
    "#.............@..............#",
    "##############..##############",
    NULL,
};
static const char *const MAP_HERMITAGE[] = {
    "########################",
    "#::RRRRRRRR::RRRRRRRR::#",
    "#::RRRRRRRR::RRRRRRRR::#",
    "#::RRRRRRRR::RRRRRRRR::#",
    "#::WwWwCwWW::WwWwGwWW::#",
    "#::::::::::::::::::::::#",
    "#::::::::::1:::::::::::#",
    "#::::::::::::::::::::::#",
    "#:fffffff::::::~~~~~~~:#",
    "#:f,,,,,f:::o::~~~~~~~:#",
    "#:f,,0,,f::::::~~~~~~~:#",
    "#:f,,,,,f:::::3~~~~~~~:#",
    "#:fffffff::::::~~~~~~~:#",
    "#:::4:::::::::::::2::::#",
    "#::::::::::@:::::::::::#",
    "###########::###########",
    NULL,
};
static const char *const MAP_SALOON[] = {
    "######################",
    "######################",
    "#_kkkkkkkk__z_PP_____#",
    "#___________________z#",
    "#z_0__________2______#",
    "#_________1__________#",
    "#___t____t_____t_____#",
    "#________________4___#",
    "#____3_____________z_#",
    "#________5___________#",
    "#___t___________t____#",
    "#z_7______@____6_____#",
    "#______z_____z______z#",
    "##########ee##########",
    NULL,
};
static const char *const MAP_PIKESHACK[] = {
    "##############",
    "#____________#",
    "#_BB______t__#",
    "#____0_______#",
    "#____________#",
    "#________1___#",
    "#_K________b_#",
    "#____________#",
    "#_____@______#",
    "######e#######",
    NULL,
};
static const char *const MAP_ROOST[] = {
    "%%%%%%%%%%%%%%%%%%%%%%",
    "%%%%%%%%%%%%%%%%%%%%%%",
    "%.............kkkkkkk%",
    "%..t............0....%",
    "%............3.......%",
    "%....1.........%%%%%%%",
    "%..............l...M%%",
    "%..............%%%%%%%",
    "%......t.............%",
    "%...b......2.........%",
    "%....................%",
    "%.x.................b%",
    "%.........@..........%",
    "%%%%%%%%%%e%%%%%%%%%%%",
    NULL,
};
static const char *const MAP_THORNS[] = {
    "####################",
    "####################",
    "#::::::::aa::::::::#",
    "#::::::::rr:::4::J:#",
    "#::::::::0r::::::::#",
    "#::ppppp:rr:ppppp::#",
    "#::::::::rr::::::::#",
    "#::ppppp:rr:ppppp::#",
    "#::::::::rr::::::::#",
    "#:1ppppp:rr:ppppp::#",
    "#::::::::rr::::::::#",
    "#::ppppp:rr:ppppp::#",
    "#::::::::rr::::::2:#",
    "#::::3:::rr::::::::#",
    "#::::::::@r::::::::#",
    "#########e##########",
    NULL,
};
static const char *const MAP_WIDOW[] = {
    "################",
    "#______________#",
    "#_BB________KK_#",
    "#______t_______#",
    "#______________#",
    "#____0_______2_#",
    "#_________1____#",
    "#______________#",
    "#_m_________c__#",
    "#______________#",
    "#______@_______#",
    "#######e########",
    NULL,
};
static const char *const MAP_SHRINE[] = {
    "vvvvvvvvvvvvvvvvvvvv",
    "vvvvvvvvvvvvvvvvvvvv",
    "v::::::::::::::::::v",
    "v::::::::aa::::::::v",
    "v:::v::::::::::v:::v",
    "v::::::::0:::::::::v",
    "v:::::::::J::::::::v",
    "v::::1:::::::::::::v",
    "v::::::::::::::::::v",
    "v:::v::::::::::v:::v",
    "v::::::::::::::::::v",
    "v:::::::::::::2::::v",
    "v::::::::@:::::::::v",
    "vvvvvvvvvevvvvvvvvvv",
    NULL,
};
static const char *const MAP_SKYBAR[] = {
    "##################",
    "#________________#",
    "#___kkkkkkkkkk___#",
    "#_P_____0________#",
    "#________________#",
    "#________________#",
    "#_t____________t_#",
    "#___1________2___#",
    "#________________#",
    "#_______@________#",
    "########e#########",
    NULL,
};
static const char *const MAP_SMITH[] = {
    "%%%%%%%%%%%%%%%%",
    "%::::::::::::::%",
    "%::VV:::::::kk:%",
    "%::::::::::::::%",
    "%:::::::0::::::%",
    "%::::::::::::::%",
    "%::::::::::::::%",
    "%:x::::::::::x:%",
    "%::::::::::::::%",
    "%::::::@:::::::%",
    "%%%%%%%e%%%%%%%%",
    NULL,
};
static const char *const MAP_SPUR3[] = {
    "########################",
    "########################",
    "########################",
    "#___V______________V___#",
    "#___V______aa______V___#",
    "#___V______________V___#",
    "#___V______01______V___#",
    "#___V______________V___#",
    "#___V______________V___#",
    "#___V__2________3__V___#",
    "#___V______________V___#",
    "#___V______________V___#",
    "#___V______@_______V___#",
    "#___V______e_______V___#",
    "#___V______________V___#",
    "########################",
    NULL,
};

typedef struct { uint8_t look, pal, dir, wander; int16_t show_if, hide_if; char in; } NpcDef; /* in: the door they live behind (h: upstairs) */
typedef struct { int8_t x, y; int16_t item; int32_t teeth; } FindDef; /* hidden: search the tile */
typedef struct {
    int map;
    const char *const *rows;
    char under;
    NpcDef npc[10];
    int16_t chest_item[4]; int32_t chest_teeth[4]; /* the 'c' marks in order */
    FindDef find[4];
    int8_t shop_story;      /* 'Q' door's story number */
} HandMap;

#define NO -1
#define N(look, pal, dir, wander) {look, pal, dir, wander, NO, NO}
#define NS(look, pal, dir, wander, s, h) {look, pal, dir, wander, s, h, 0}
#define NI(look, pal, dir, wander, s, h, in) {look, pal, dir, wander, s, h, in}
static const HandMap HAND[] = {
    {M_GLADWELL, MAP_GLADWELL, ',',
     {N(LOOK_MAN, 0, 0, 1), N(LOOK_WOMAN, 0, 0, 0), N(LOOK_KID, 0, 0, 1), NI(LOOK_OLD, 0, 0, 0, NO, NO, 'h'), N(LOOK_DOG, 0, 0, 1),
      N(LOOK_MAN, 1, 0, 0), N(LOOK_SHERIFF, 0, 0, 0)},
     {0}, {0}, {{7, 15, IT_BANDAGE, 0}, {24, 15, 0, 25}}, 0},
    {M_MERIDIAN, MAP_MERIDIAN, ',',
     {N(LOOK_MAN, 2, 0, 1), N(LOOK_WOMAN, 1, 0, 1), N(LOOK_KID, 1, 0, 1), N(LOOK_SHERIFF, 1, 0, 0), N(LOOK_OLD, 1, 0, 0),
      N(LOOK_HORSE, 0, 2, 0), NI(LOOK_WOMAN, 2, 0, 0, NO, NO, 'N'), N(LOOK_MINER, 0, 0, 0), N(LOOK_BARKEEP, 0, 0, 0)},
     {0}, {0}, {{13, 19, IT_HERB, 0}, {31, 19, 0, 40}}, 0},
    {M_STCLARE, MAP_STCLARE, ':',
     {N(LOOK_NUN, 0, 0, 1), N(LOOK_ANGEL, 0, 0, 0), N(LOOK_PRIEST, 0, 0, 0), N(LOOK_NUN, 1, 0, 0), N(LOOK_KID, 2, 0, 1)},
     {0}, {0}, {{2, 2, IT_HOLYOIL, 0}, {19, 10, IT_SNAKEROOT, 0}, {19, 8, IT_BEEFBONE, 0}}, 0},
    {M_HOPE, MAP_HOPE, ',',
     {NS(LOOK_BOSS, A_BRUTUS, 0, 0, NO, F_BRUTUS_GONE), NS(LOOK_DEMON, 0, 0, 1, NO, F_HOPE_FREE),
      NS(LOOK_DEMON, 1, 0, 1, NO, F_HOPE_FREE), N(LOOK_WOMAN, 3, 0, 0), N(LOOK_MAN, 3, 0, 0), N(LOOK_KID, 3, 0, 1),
      N(LOOK_OLD, 2, 0, 0), NS(LOOK_DEMON, 2, 0, 0, NO, F_HOPE_FREE), N(LOOK_MAN, 4, 0, 1)},
     {0}, {0}, {{27, 8, IT_SALVE, 0}, {2, 17, 0, 60}}, 0},
    {M_STEEPLE, MAP_STEEPLE, ',',
     {N(LOOK_MAN, 5, 0, 1), NI(LOOK_WOMAN, 4, 0, 0, NO, NO, 'h'), N(LOOK_KID, 4, 0, 1), N(LOOK_SHERIFF, 2, 0, 0), N(LOOK_HORSE, 1, 3, 0),
      N(LOOK_OLD, 3, 0, 0), N(LOOK_SOLDIER, 0, 0, 0), NS(LOOK_MAN, 6, 0, 0, NO, NO)},
     {0}, {0}, {{2, 16, IT_GINGERPOP, 0}, {29, 18, 0, 120}}, 1},
    {M_FORT, MAP_FORT, ':',
     {NS(LOOK_ANGEL, 5, 0, 0, F_FORT, F_CUT_OPEN),
      N(LOOK_SOLDIER, 1, 0, 1), N(LOOK_SOLDIER, 2, 0, 0), N(LOOK_PRIEST, 1, 0, 0), N(LOOK_SOLDIER, 3, 0, 1),
      NI(LOOK_SOLDIER, 4, 0, 0, NO, F_SOLDIER_DEAD, 'h'), N(LOOK_SOLDIER, 5, 0, 0), N(LOOK_SOLDIER, 6, 0, 0)},
     {0}, {0}, {{4, 12, IT_HOLYOIL, 0}, {19, 9, 0, 300}}, 0},
    {M_DOLORES, MAP_DOLORES, '.',
     {N(LOOK_OUTLAW, 0, 0, 1), N(LOOK_OLD, 4, 0, 0), N(LOOK_ANIMAL, 5, 0, 0), N(LOOK_MAN, 7, 0, 0), N(LOOK_KID, 5, 0, 1),
      N(LOOK_DOG, 1, 0, 1), N(LOOK_OUTLAW, 1, 0, 0), NI(LOOK_BOSS, A_MAGPIE, 0, 0, NO, F_MAGPIE, 'S'),
      NI(LOOK_OUTLAW, 3, 0, 0, NO, NO, 'S'), N(LOOK_MAN, 11, 0, 0)},
     {0}, {0}, {{2, 17, 0, 1000}, {29, 18, IT_SNAKEROOT, 0}}, 0},
    {M_HOLLOW, MAP_HOLLOW, '.',
     {NS(LOOK_MAN, 8, 0, 1, F_MISC0, NO), NS(LOOK_WOMAN, 5, 0, 1, F_MISC0, NO), NS(LOOK_KID, 6, 0, 0, F_MISC0, NO),
      NS(LOOK_MINER, 1, 0, 0, F_MISC0, NO), NS(LOOK_OLD, 5, 0, 0, F_MISC0, NO), NS(LOOK_KEEPER, 0, 0, 0, F_MISC0, NO),
      NS(LOOK_DOG, 2, 0, 0, NO, F_MISC0)},
     {0}, {0}, {{12, 8, IT_BEEFBONE, 0}, {24, 5, 0, 777}}, 0},
    {M_LASTCHANCE, MAP_LASTCHANCE, ',',
     {N(LOOK_MAN, 9, 0, 1), N(LOOK_WOMAN, 6, 0, 1), N(LOOK_ANGEL, 2, 0, 0), N(LOOK_SHERIFF, 3, 0, 0), N(LOOK_KID, 7, 0, 1),
      N(LOOK_HORSE, 2, 2, 0), N(LOOK_OLD, 6, 0, 0), N(LOOK_OUTLAW, 2, 0, 0), N(LOOK_MAN, 10, 0, 0)},
     {0}, {0}, {{31, 18, IT_MEDCHEST, 0}, {13, 19, 0, 900}}, 0},
    {M_ASHMARKET, MAP_ASHMARKET, '.',
     {N(LOOK_DEMON, 3, 0, 1), N(LOOK_DEMON, 4, 0, 0), N(LOOK_DEMON, 5, 0, 1), N(LOOK_DEMON, 6, 0, 0), N(LOOK_DEMON, 7, 0, 1),
      N(LOOK_DEMON, 8, 0, 0), N(LOOK_DEMON, 0, 0, 0)},
     {0}, {0}, {{2, 16, IT_HALOPIN, 0}, {27, 16, 0, 6666}}, 0},
    {M_HERMITAGE, MAP_HERMITAGE, ':',
     {N(LOOK_NUN, 2, 0, 1), N(LOOK_PRIEST, 2, 0, 0), N(LOOK_DOG, 3, 0, 1), N(LOOK_PRIEST, 3, 0, 0), N(LOOK_NUN, 3, 0, 0)},
     {0}, {0}, {{0, 0, 0, 0}}, 0},
    {M_SALOON, MAP_SALOON, '_',
     {N(LOOK_COUNT + H_TUCKER, 0, 0, 0), N(LOOK_COUNT + H_SAGE, 0, 0, 0), N(LOOK_COUNT + H_HOLLIS, 0, 0, 0),
      N(LOOK_COUNT + H_QUILL, 0, 0, 0), N(LOOK_COUNT + H_VERA, 0, 0, 0), N(LOOK_COUNT + H_HATTIE, 0, 0, 0),
      N(LOOK_COUNT + H_CORA, 0, 0, 0), N(LOOK_COUNT + H_BISCUIT, 0, 0, 0)},
     {0}, {0}, {{0, 0, 0, 0}}, 0},
    {M_PIKESHACK, MAP_PIKESHACK, '_',
     {NS(LOOK_OUTLAW, 3, 0, 0, F_PIKE_FOUND, NO), N(LOOK_MAN, 11, 0, 0)},
     {0}, {0}, {{11, 6, IT_LARIAT, 0}}, 0},
    {M_ROOST, MAP_ROOST, '.',
     {NS(LOOK_BOSS, A_MOLLY, 0, 0, NO, NO), N(LOOK_OUTLAW, 4, 0, 0), N(LOOK_OUTLAW, 5, 0, 1), N(LOOK_OUTLAW, 6, 0, 0)},
     {0}, {0}, {{4, 9, IT_BLACKBREW, 0}, {2, 11, 0, 250}}, 0},
    {M_THORNS, MAP_THORNS, ':',
     {NS(LOOK_PRIEST, 4, 0, 0, NO, F_MIMIC), N(LOOK_NUN, 4, 0, 0), N(LOOK_MAN, 12, 0, 0), N(LOOK_WOMAN, 7, 0, 0),
      N(LOOK_KID, 8, 0, 0)},
     {0}, {0}, {{0, 0, 0, 0}}, 0},
    {M_WIDOW, MAP_WIDOW, '_',
     {N(LOOK_OLD, 7, 0, 0), N(LOOK_WOMAN, 8, 0, 0), N(LOOK_KID, 9, 0, 1)},
     {IT_BUCKSKIN}, {0}, {{2, 8, IT_SOUPBONE, 0}}, 0},
    {M_SHRINE, MAP_SHRINE, ':',
     {N(LOOK_PRIEST, 5, 0, 0), N(LOOK_WOMAN, 9, 0, 0), N(LOOK_MAN, 13, 0, 0)},
     {0}, {0}, {{0, 0, 0, 0}}, 0},
    {M_SKYBAR, MAP_SKYBAR, '_',
     {N(LOOK_BARKEEP, 1, 0, 0), N(LOOK_ANGEL, 3, 0, 0), N(LOOK_MAN, 14, 0, 0)},
     {0}, {0}, {{0, 0, 0, 0}}, 0},
    {M_SMITH, MAP_SMITH, ':',
     {N(LOOK_DEMON, 9, 0, 0)},
     {0}, {0}, {{0, 0, 0, 0}}, 0},
    {M_SPUR3, MAP_SPUR3, '_',
     {NS(LOOK_BOSS, A_ZENITH, 0, 0, NO, F_WON), NS(LOOK_BOSS, A_NADIR, 0, 0, NO, F_WON), N(LOOK_ANGEL, 4, 0, 0), N(LOOK_DEMON, 10, 0, 0)},
     {0}, {0}, {{0, 0, 0, 0}}, 0},
};
#undef N
#undef NS

static int char_tile(char c, char under) {
    switch (c) {
    case ',': return T_GRASS;
    case '.': return T_DIRT;
    case '+': return T_ROAD;
    case ':': return T_STONE;
    case '_': return T_FLOOR;
    case 'R': return T_ROOF;
    case 'W': return T_BWALL;
    case 'w': return T_WINDOW;
    case 'T': return T_TREE;
    case 'v': return T_TREE;
    case '=': return T_FENCE;
    case 'o': return T_WELL;
    case 'b': return T_BARREL;
    case 'x': return T_CRATE;
    case '~': return T_WATER;
    case 'g': return T_GRAVE;
    case 'c': return T_CHEST;
    case 'k': return T_COUNTER;
    case 'p': return T_PEW;
    case 'a': return T_ALTAR;
    case 'r': return T_RUG;
    case 'B': return T_BED;
    case 't': return T_TABLE;
    case '#': return T_WALL;
    case '%': return T_ROCK;
    case 'f': return T_FLOWERS;
    case 'y': return T_TRACKS;
    case 's': return T_STOOL;
    case 'P': return T_PIANO;
    case 'K': return T_BOOKS;
    case 'z': return T_BURNING;
    case 'm': return T_TABLE;
    case 'V': return T_CAULDRON;
    case 'h': return T_HIVE;
    case 'l': return T_LOCKED;
    case 'M': return T_DOWN;
    case 'J': return under == ':' ? T_STONE : T_FLOOR;
    case 'e': return T_EXIT;
    default:
        if (c >= '0' && c <= '9') return char_tile(under, under);
        if (c == '@') return char_tile(under, under);
        if (strchr("HCGUNSYLXDQ", c)) return T_DOOR;
        return T_GRASS;
    }
}

static int door_service(char c) {
    switch (c) {
    case 'H': return SV_HOTEL;
    case 'C': return SV_CHAPEL;
    case 'G': return SV_SHOP_A;
    case 'U': return SV_SHOP_B;
    case 'N': return SV_BANK;
    case 'S': return SV_SALOON;
    case 'Y': return SV_TRAIN;
    case 'L': return SV_STABLES;
    case 'X': return SV_SHERIFF;
    case 'Q': return SV_STORY;
    default: return SV_LOCKED;
    }
}

static void build_hand(SpurMap *m, const HandMap *h) {
    int y = 0, chests = 0;
    while (h->rows[m->h]) m->h++;
    for (; h->rows[y]; y++) {
        const char *r = h->rows[y];
        int w = (int)strlen(r);
        m->w = imax(m->w, w);
        for (int x = 0; x < w; x++) {
            char c = r[x];
            put(m, x, y, char_tile(c, h->under));
            if (c == '@') { m->start_x = x; m->start_y = y; }
            if (c >= '0' && c <= '9') {
                int i = c - '0';
                const NpcDef *d = &h->npc[i];
                if (m->nnpc <= i) m->nnpc = i + 1;
                if (d->in && spur_has_inside(h->map, SV_HOTEL)) continue; /* they are indoors */
                m->npc[i] = (SpurNpc){(int8_t)x, (int8_t)y, d->look, d->pal, d->dir, d->wander, (uint16_t)(h->map * 32 + i),
                                      d->show_if, d->hide_if};
            }
            if (strchr("HCGUNSYLXDQ", c) && m->ndoor < SPUR_MAX_DOOR)
                m->door[m->ndoor++] = (SpurDoor){(int8_t)x, (int8_t)y, (uint8_t)door_service(c), (uint8_t)h->shop_story};
            if (c == 'c' && m->nchest < SPUR_MAX_CHEST) {
                m->chest[m->nchest] = (SpurChest){(int8_t)x, (int8_t)y, (uint16_t)(h->map * 16 + m->nchest),
                                                  h->chest_item[chests], h->chest_teeth[chests], 0};
                m->nchest++;
                chests++;
            }
            if (c == 'J') {
                /* a way down the story opens: under the mission, into the dream */
                if (h->map == M_THORNS && spur_flag(F_MIMIC)) put(m, x, y, T_DOWN);
                if (h->map == M_SHRINE && spur_flag(F_MISC0 + 1)) put(m, x, y, T_PORTAL);
                if (h->map == M_THORNS) m->link[m->nlink++] = (SpurLink){(int8_t)x, (int8_t)y, M_CRYPT, -1, 0};
                if (h->map == M_SHRINE) m->link[m->nlink++] = (SpurLink){(int8_t)x, (int8_t)y, M_DREAM1, -1, 0};
            }
            if (c == 'M') m->link[m->nlink++] = (SpurLink){(int8_t)x, (int8_t)y, M_ROOSTMINE, -1, 0};
            if (c == 'l' && spur_flag(F_ROOST_DOOR)) put(m, x, y, T_DIRT);
            if (c == 'e') m->link[m->nlink++] = (SpurLink){(int8_t)x, (int8_t)y, M_WORLD, -3, 0};
        }
    }
    m->h = y;
    for (int i = 0; i < 4; i++)
        if (h->find[i].item || h->find[i].teeth)
            m->chest[m->nchest++] = (SpurChest){h->find[i].x, h->find[i].y, (uint16_t)(h->map * 16 + 8 + i),
                                                h->find[i].item, h->find[i].teeth, 1};
    m->zone = -1;
    /* the summit's way down leads back into the citadel */
    if (h->map == M_SPUR3)
        for (int i = 0; i < m->nlink; i++) { m->link[i].map = M_SPUR2; m->link[i].tx = -2; }
}

/* ---- inside the town buildings ------------------------------------------------------- */
/* One room per door (two floors for a hotel). '0' is whoever works the counter, '1' and '2'
 * are folk passing the time, '5' to '7' are where the town's own people stand when the
 * table above puts them indoors. 'u' and 'd' are the hotel stairs, 'e' the way out. */
static const char *const IN_HOTEL[] = {
    "##################",
    "#Kb.#...0..#....u#",
    "#...#kkkkkk#.....#",
    "#.t1.............#",
    "#.s......rrr..t2.#",
    "#........rrr..s..#",
    "#.t.s............#",
    "#...........K..b.#",
    "#................#",
    "########e#########",
    NULL,
};
static const char *const IN_UPSTAIRS[] = {
    "##################",
    "#B..#B..#B..#B..B#",
    "#5..#..6#...#7...#",
    "#t..#t..#t..#t..t#",
    "##.###.###.###.###",
    "#..............d.#",
    "#................#",
    "#.K...b.....b..K.#",
    "#................#",
    "##################",
    NULL,
};
static const char *const IN_SALOON[] = {
    "##################",
    "#rr5#...0.x.#...6#",
    "#rrr#kkkkkkk#....#",
    "#........P.......#",
    "#.s.t.s.......s.t#",
    "#.....1.......s..#",
    "#.s.t.s.....2....#",
    "#............t.s.#",
    "#b..............b#",
    "########e#########",
    NULL,
};
static const char *const IN_SHOP[] = {
    "##################",
    "#KKKbx..0..xbKKKK#",
    "#kkkkkkkkkkkkkkkk#",
    "#................#",
    "#.x...t......x.1.#",
    "#................#",
    "#.b..........b...#",
    "#....x.....2.....#",
    "#................#",
    "########e#########",
    NULL,
};
static const char *const IN_BANK[] = {
    "##################",
    "#5..#iii0iiiii#xx#",
    "#...#kkkkkkkkk#bb#",
    "#................#",
    "#..rrrrrrrrrrrr..#",
    "#..r..........r..#",
    "#.t.s......s.t.1.#",
    "#.........2......#",
    "#K..............K#",
    "########e#########",
    NULL,
};
static const char *const IN_SHERIFF[] = {
    "##################",
    "#..#..#.....K..bK#",
    "#..#..#..........#",
    "#ii#ii#506.......#",
    "#......kkkkk######",
    "#................#",
    "#.t.s.......b....#",
    "#.....1..........#",
    "#K........2.....b#",
    "########e#########",
    NULL,
};
static const char *const IN_CHAPEL[] = {
    "##################",
    "#K......0......K.#",
    "#......aaa.......#",
    "#................#",
    "#.pppppp..pppppp1#",
    "#................#",
    "#.pppppp..pppppp.#",
    "#................#",
    "#.pppppp..pppppp2#",
    "########e#########",
    NULL,
};

/* hidden finds indoors: search the tile (chest ids 12..15 of the town's sixteen) */
typedef struct { uint8_t town, service, floor; int8_t x, y; int16_t item; int32_t teeth; } InsideFind;
static const InsideFind IN_FINDS[] = {
    {M_MERIDIAN, SV_SALOON, 0, 10, 1, IT_QUAD, 0},      /* behind the bar, past the loose counter flap */
    {M_MERIDIAN, SV_HOTEL, 1, 16, 1, 0, 300},          /* under the end bed */
    {M_STEEPLE, SV_HOTEL, 1, 1, 1, IT_SALVE, 0},
    {M_DOLORES, SV_SALOON, 0, 16, 8, 0, 450},          /* in the corner barrel */
    {M_LASTCHANCE, SV_SHERIFF, 0, 16, 8, IT_HALOPIN, 0}, /* in the evidence barrel */
    {M_HOPE, SV_HOTEL, 1, 9, 1, IT_HOLYOIL, 0},
};
/* the one saloon with a way behind its bar */
#define SECRET_BAR_TOWN M_MERIDIAN

static const char *const *inside_rows(int service, int floor) {
    switch (service) {
    case SV_HOTEL: return floor ? IN_UPSTAIRS : IN_HOTEL;
    case SV_SALOON: return IN_SALOON;
    case SV_SHOP_A: case SV_SHOP_B: return IN_SHOP;
    case SV_BANK: return IN_BANK;
    case SV_SHERIFF: case SV_STORY: return IN_SHERIFF;
    case SV_CHAPEL: return IN_CHAPEL;
    default: return NULL;
    }
}

static const HandMap *hand_of(int map) {
    for (int i = 0; i < ARRAY_LEN(HAND); i++)
        if (HAND[i].map == map) return &HAND[i];
    return NULL;
}

static char service_char(int service) {
    switch (service) {
    case SV_HOTEL: return 'H';
    case SV_CHAPEL: return 'C';
    case SV_SHOP_A: return 'G';
    case SV_SHOP_B: return 'U';
    case SV_BANK: return 'N';
    case SV_SALOON: return 'S';
    case SV_TRAIN: return 'Y';
    case SV_STABLES: return 'L';
    case SV_SHERIFF: return 'X';
    case SV_STORY: return 'Q';
    default: return 'D';
    }
}

bool spur_town_door(int town, int service, int *x, int *y) {
    const HandMap *h = hand_of(town);
    if (!h) return false;
    char c = service_char(service);
    for (int yy = 0; h->rows[yy]; yy++)
        for (int xx = 0; h->rows[yy][xx]; xx++)
            if (h->rows[yy][xx] == c) { *x = xx; *y = yy; return true; }
    return false;
}

bool spur_has_inside(int town, int service) {
    if (town < SPUR_FIRST_TOWN || town >= SPUR_FIRST_TOWN + SPUR_TOWNS) return false;
    if (town == M_HOLLOW) return false; /* the ghost town keeps its doors: its past is a dream */
    return inside_rows(service, 0) != NULL;
}

int spur_town_here(void) { return sg.map_id == M_INSIDE ? sg.inside.town : sg.map_id; }

/* who works the counter */
static void counter_look(int town, int service, uint8_t *look, uint8_t *pal) {
    int ti = town - SPUR_FIRST_TOWN;
    *pal = (uint8_t)(ti * 3 + service);
    switch (service) {
    case SV_HOTEL: case SV_SHOP_A: case SV_SHOP_B: *look = LOOK_KEEPER; break;
    case SV_BANK: *look = LOOK_MAN; break;
    case SV_SALOON: *look = LOOK_BARKEEP; break;
    case SV_SHERIFF: *look = LOOK_SHERIFF; break;
    case SV_CHAPEL: *look = (town == M_STCLARE || town == M_HERMITAGE) ? LOOK_NUN : LOOK_PRIEST; break;
    case SV_STORY: *look = LOOK_BOSS; *pal = A_TALLYMAN; break;
    default: *look = LOOK_MAN; break;
    }
    if (town == M_ASHMARKET) *look = LOOK_DEMON;
    if (town == M_FORT && service != SV_CHAPEL) *look = LOOK_SOLDIER;
}

static const char *const IN_WORD[] = {
    [SV_HOTEL] = "HOTEL", [SV_CHAPEL] = "CHAPEL", [SV_SHOP_A] = "STORE", [SV_SHOP_B] = "STORE", [SV_BANK] = "BANK",
    [SV_SALOON] = "SALOON", [SV_SHERIFF] = "SHERIFF'S OFFICE", [SV_STORY] = "JAILHOUSE",
};
const char *spur_inside_name(void) {
    static char nm[48];
    int s = sg.inside.service;
    const char *w = s < (int)ARRAY_LEN(IN_WORD) && IN_WORD[s] ? IN_WORD[s] : "HOUSE";
    if (sg.inside.town == M_FORT && s == SV_HOTEL) w = "BARRACKS";
    snprintf(nm, sizeof nm, "%s %s%s", SPUR_MAPINFO[sg.inside.town].name, w, sg.inside.floor ? ", UPSTAIRS" : "");
    return nm;
}

static void build_inside(SpurMap *m) {
    int town = sg.inside.town, service = sg.inside.service, floor = sg.inside.floor;
    const HandMap *h = hand_of(town);
    const char *const *rows = inside_rows(service, floor);
    if (!h || !rows) { town = M_GLADWELL; service = SV_HOTEL; floor = 0; h = hand_of(town); rows = IN_HOTEL; }
    int anchor_x[10], anchor_y[10];
    for (int i = 0; i < 10; i++) anchor_x[i] = -1, anchor_y[i] = -1;
    for (int y = 0; rows[y]; y++) {
        int w = (int)strlen(rows[y]);
        m->w = imax(m->w, w);
        m->h = y + 1;
        for (int x = 0; x < w; x++) {
            char c = rows[y][x];
            int t = T_FLOOR;
            switch (c) {
            case '#': t = T_WALL; break;
            case 'k': t = T_COUNTER; break;
            case 'r': t = T_RUG; break;
            case 't': t = T_TABLE; break;
            case 's': t = T_STOOL; break;
            case 'B': t = T_BED; break;
            case 'P': t = T_PIANO; break;
            case 'K': t = T_BOOKS; break;
            case 'b': t = T_BARREL; break;
            case 'x': t = T_CRATE; break;
            case 'p': t = T_PEW; break;
            case 'a': t = T_ALTAR; break;
            case 'i': t = T_BARS; break;
            case 'u': t = T_UP; break;
            case 'd': t = T_DOWN; break;
            case 'e': t = T_EXIT; break;
            default: break;
            }
            if (service == SV_CHAPEL && t == T_FLOOR) t = T_STONE;
            if (town == M_ASHMARKET && t == T_FLOOR) t = T_HELLFLOOR;
            put(m, x, y, t);
            if (c >= '0' && c <= '9') { anchor_x[c - '0'] = x; anchor_y[c - '0'] = y; }
            if (c == 'e') {
                m->link[m->nlink++] = (SpurLink){(int8_t)x, (int8_t)y, (uint8_t)town, sg.inside.dx, (int8_t)(sg.inside.dy + 1)};
                m->start_x = x;
                m->start_y = y - 1;
            }
            if (c == 'u') {
                m->link[m->nlink++] = (SpurLink){(int8_t)x, (int8_t)y, M_INSIDE, -2, 1};
                m->back_x = x;
                m->back_y = y + 1;
            }
            if (c == 'd') {
                m->link[m->nlink++] = (SpurLink){(int8_t)x, (int8_t)y, M_INSIDE, -2, 0};
                m->back_x = x - 1;
                m->back_y = y;
                m->start_x = x - 1;
                m->start_y = y;
            }
        }
    }
    /* the loose flap in one bar */
    if (service == SV_SALOON && town == SECRET_BAR_TOWN) put(m, 11, 2, T_SECRET);
    /* the counter */
    if (anchor_x[0] >= 0) {
        uint8_t look, pal;
        counter_look(town, service, &look, &pal);
        SpurNpc *n = &m->npc[0];
        *n = (SpurNpc){(int8_t)anchor_x[0], (int8_t)anchor_y[0], look, pal, 0, 0, (uint16_t)(0xF000 | service), NO, NO};
        if (service == SV_STORY) n->hide_if = F_HOPE_FREE; /* the Tallyman's desk */
        m->nnpc = 1;
    }
    /* the town's own people who live indoors */
    char want = floor ? 'h' : service_char(service);
    int slot = 5;
    for (int i = 0; i < 10 && h; i++) {
        const NpcDef *d = &h->npc[i];
        if (d->in != want || slot > 7 || anchor_x[slot] < 0) continue;
        m->npc[m->nnpc++] = (SpurNpc){(int8_t)anchor_x[slot], (int8_t)anchor_y[slot], d->look, d->pal, d->dir, 0,
                                      (uint16_t)(town * 32 + i), d->show_if, d->hide_if};
        slot++;
    }
    /* the Tallyman's imps, at his elbows */
    if (service == SV_STORY)
        for (int k = 0; k < 2 && slot <= 7; k++, slot++)
            if (anchor_x[slot] >= 0)
                m->npc[m->nnpc++] = (SpurNpc){(int8_t)anchor_x[slot], (int8_t)anchor_y[slot], LOOK_DEMON, (uint8_t)(k + 2), 0, 0,
                                              (uint16_t)(M_HOPE * 32 + 1 + k), NO, F_HOPE_FREE};
    /* folk passing the time */
    for (int k = 1; k <= 2; k++) {
        if (anchor_x[k] < 0) continue;
        static const uint8_t LOOKS[] = {LOOK_MAN, LOOK_WOMAN, LOOK_OLD, LOOK_OUTLAW, LOOK_MINER, LOOK_KID};
        int ti = town - SPUR_FIRST_TOWN;
        uint8_t look = LOOKS[(ti * 2 + k + service) % ARRAY_LEN(LOOKS)];
        if (town == M_ASHMARKET) look = LOOK_DEMON;
        if (town == M_FORT) look = LOOK_SOLDIER;
        if (town == M_STCLARE || town == M_HERMITAGE) look = k == 1 ? LOOK_NUN : LOOK_OLD;
        if (service == SV_SALOON && town == M_HOPE) continue; /* the devils drink alone */
        m->npc[m->nnpc++] = (SpurNpc){(int8_t)anchor_x[k], (int8_t)anchor_y[k], look, (uint8_t)(ti + k * 5), 0, 0,
                                      (uint16_t)(0xE000 | (service << 4) | (k - 1)), NO, NO};
    }
    for (int i = 0; i < ARRAY_LEN(IN_FINDS); i++) {
        const InsideFind *f = &IN_FINDS[i];
        if (f->town != town || f->service != service || f->floor != floor || m->nchest >= SPUR_MAX_CHEST) continue;
        m->chest[m->nchest++] = (SpurChest){f->x, f->y, (uint16_t)(town * 16 + 12 + i % 4), f->item, f->teeth, 1};
    }
    m->zone = -1;
}

int spur_inside_finds(void) { return ARRAY_LEN(IN_FINDS); }

/* ---- dungeons -------------------------------------------------------------------------- */
enum { DS_CAVE, DS_MINE, DS_PRISON, DS_PASS, DS_CRYPT, DS_SNAKE, DS_DREAM, DS_TOWER, DS_CITADEL };
enum { DX_NONE, DX_NEXT, DX_OUT };
typedef struct {
    int map;
    uint8_t style, w, h;
    uint32_t seed;
    uint8_t exitkind;       /* what the far end holds */
    uint8_t next;           /* DX_NEXT: the floor beyond; DX_OUT: the world's door there (index into WORLD_POI) */
    int8_t prev;            /* the floor before (-1: the entrance is a way out) */
    uint8_t zone;
    uint8_t boss_troop, boss_art; int16_t boss_flag; uint8_t block; /* a boss in the last corridor */
    int16_t key;            /* the prize in the last room: an item, or 1000 + key item */
    int16_t chest[10];      /* items, or -teeth */
    uint8_t ambush_troop[2];
    uint8_t ores, ore_table;
    int8_t animal;
    int8_t story_npc;       /* a story person in the last room (LOOK_*), or -1 */
    uint8_t story_pal;
    int8_t ambush_far;      /* the prize is guarded by an ambush */
    int8_t loop_ambush;     /* an ambush that comes back every time (troop) */
} DunSpec;

static int poi_index(int map, int end) {
    for (int i = 0; i < (int)WORLD_POIS; i++)
        if (WORLD_POI[i].map == map && WORLD_POI[i].end == end) return i;
    return 0;
}

#define T(n) (-(n))
static const DunSpec DUN[] = {
    {M_DRYWASH, DS_CAVE, 40, 29, 101, DX_NONE, 0, -1, 1, 0, 0, NO, 0, 0,
     {IT_CAMPKIT, IT_SAPLING, T(50), IT_HERB, IT_BANDAGE}, {0}, 0, 0, 1, LOOK_OUTLAW, 3, 0, 0},
    {M_SIXPENNY, DS_MINE, 48, 36, 202, DX_NONE, 0, -1, 2, 82, A_PUCKKING, F_PUCKKING, 0, 1000 + KI_CRANK,
     {IT_WORNBOOTS, T(100), IT_BANDAGE, IT_SNAKEROOT, IT_IRONORE, IT_DUCKFOOT}, {101, 102}, 1, 0, -1, -1, 0, 0, 0},
    {M_PEN1, DS_PRISON, 48, 36, 303, DX_NEXT, M_PEN2, -1, 4, 0, 0, NO, 0, 0,
     {T(50), IT_SILVER, T(100), IT_LONGTOM, IT_GINGERPOP, IT_BOLOTIE}, {103, 0}, 0, 0, -1, LOOK_COFFIN, 0, 0, 0},
    {M_PEN2, DS_PRISON, 48, 36, 304, DX_NONE, 0, M_PEN1, 4, 83, A_SCOURGE, F_SCOURGE, 0, 0,
     {IT_WOOL, IT_SALVE, T(1500), IT_EMERALD, IT_LARIAT, IT_BOWIE, IT_BANDAGE, T(139), IT_SNAKESKIN}, {104, 0}, 0, 0, -1,
     LOOK_ANGEL, 5, 0, 0},
    {M_NOTCH, DS_PASS, 56, 29, 405, DX_OUT, 1, -1, 5, 0, 0, NO, 0, 0,
     {IT_SALVE, T(890), IT_CAMPKIT}, {0}, 0, 0, 3, -1, 0, 0, 0},
    {M_CRYPT, DS_CRYPT, 40, 29, 506, DX_NONE, 0, -1, 8, 0, 0, NO, 0, 1000 + KI_CELLARKEY,
     {IT_HOLYOIL, IT_BUCKSKIN, IT_RUBY}, {116, 0}, 0, 0, -1, -1, 0, 1, 0},
    {M_ROOSTMINE, DS_MINE, 56, 36, 607, DX_OUT, 2, -1, 9, 0, 0, NO, 0, 0,
     {IT_LARIAT, IT_SMUDGE, T(640), IT_SALVE, IT_CRANKGUN, T(909), IT_MEDCHEST}, {105, 0}, 1, 1, 6, -1, 0, 0, 0},
    {M_HOLLOWMINE, DS_MINE, 48, 36, 708, DX_NONE, 0, -1, 11, 0, 0, NO, 0, 1000 + KI_PLUNGER,
     {T(1400), IT_TOOTHNECK, IT_SALVE, IT_GINGERPOP}, {106, 0}, 1, 2, -1, -1, 0, 1, 0},
    {M_CUT, DS_PASS, 56, 29, 809, DX_OUT, 3, -1, 12, 91, A_STORMCROW, F_STORMCROW, 0, 0,
     {IT_SMUDGE, T(830), IT_SNAKEROOT, IT_CAMPKIT, T(1800), IT_SILVERBELLE, IT_BLACKBREW}, {107, 0}, 0, 0, -1, -1, 0, 0, 0},
    {M_SNAKE1, DS_SNAKE, 56, 36, 910, DX_OUT, 4, -1, 14, 0, 0, NO, 0, 0,
     {IT_BLACKBREW, T(4280), IT_SERAPE, IT_STEADYEYE, T(390), T(3600)}, {115, 0}, 1, 2, 7, -1, 0, 0, 58},
    {M_SNAKE2, DS_SNAKE, 48, 36, 1011, DX_OUT, 5, -1, 14, 92, A_WORM, F_WORM, T_FIREWALL, 0,
     {IT_SALVE, IT_HOLYOIL, T(2000)}, {0}, 0, 0, -1, -1, 0, 0, 0},
    {M_DREAM1, DS_DREAM, 48, 36, 1112, DX_NEXT, M_DREAM2, -1, 15, 0, 0, NO, 0, 0,
     {IT_CACTUSFIG, IT_HOLYOIL, IT_MOCCASIN}, {0}, 0, 0, -1, LOOK_OLD, 10, 0, 0},
    {M_DREAM2, DS_DREAM, 40, 29, 1213, DX_NONE, 0, M_DREAM1, 26, 93, A_MOTHER, F_MOTHER, 0, 0,
     {IT_TOPAZ, IT_CACTUSFIG}, {0}, 0, 0, -1, -1, 0, 0, 0},
    {M_TOWER1, DS_TOWER, 40, 29, 1314, DX_NEXT, M_TOWER2, -1, 17, 94, A_BLIGHT, F_BLIGHT, 0, 0,
     {IT_SMUDGE, T(201), IT_HORNBOW, T(1890), IT_HALOPIN, IT_SNAKEROOT}, {0}, 0, 0, -1, -1, 0, 0, 0},
    {M_TOWER2, DS_TOWER, 40, 29, 1415, DX_NEXT, M_TOWER3, M_TOWER1, 18, 95, A_CARNAGE, F_CARNAGE, 0, 0,
     {IT_LAWMAN, T(1190), IT_THRESHER, IT_NEEDLEGUN, IT_BLACKGLASS}, {110, 0}, 0, 0, -1, -1, 0, 0, 0},
    {M_TOWER3, DS_TOWER, 40, 29, 1516, DX_NEXT, M_TOWER4, M_TOWER2, 19, 96, A_HUNGER, F_HUNGER, 0, 0,
     /* a floor of empty chests, one of them a joke, and the wishbone */
     {T(SPUR_CHEST_EMPTY), T(SPUR_CHEST_EMPTY), T(SPUR_CHEST_EMPTY), T(SPUR_CHEST_EMPTY), T(SPUR_CHEST_EMPTY),
      T(SPUR_CHEST_JOKE), IT_WISHBONE}, {0}, 0, 0, -1, -1, 0, 0, 0},
    {M_TOWER4, DS_TOWER, 40, 29, 1617, DX_NONE, 0, M_TOWER3, 20, 97, A_REAPER, F_REAPER, 0, 0,
     {IT_MEDCHEST, IT_HALOPIN, IT_DEVILSOWN, IT_LARIAT, T(2220), IT_HOLYOIL}, {109, 111}, 0, 0, -1, LOOK_ANGEL, 6, 0, 0},
    {M_SPUR1, DS_CITADEL, 56, 36, 1718, DX_NEXT, M_SPUR2, -1, 21, 0, 0, NO, 0, 0,
     {IT_MEDCHEST, T(3000), IT_SMUDGE, IT_BLACKBREW}, {112, 0}, 0, 0, -1, -1, 0, 0, 0},
    {M_SPUR2, DS_CITADEL, 56, 36, 1819, DX_NEXT, M_SPUR3, M_SPUR1, 21, 99, A_GOVERNOR, F_GOVERNOR, 0, 0,
     {IT_HOLYOIL, IT_ELEPHANT, IT_STARSAPPHIRE, T(9999), IT_BLACKBREW, IT_RUBY, T(666), IT_LARIAT, IT_SATIN}, {113, 0}, 0, 0, -1, -1,
     0, 0, 0},
};
#undef T
#define DUNS ARRAY_LEN(DUN)

/* the art for a boss standing on the map: its battle art number */
int spur_dun_index(int map) {
    for (int i = 0; i < (int)DUNS; i++)
        if (DUN[i].map == map) return i;
    return -1;
}

typedef struct { int rx, ry, rw, rh; int parent, deg, dist, leaf; int corr[64][2]; int ncorr; } Cell;

static void carve(SpurMap *m, int x, int y, int floor) {
    if (x > 0 && y > 0 && x < m->w - 1 && y < m->h - 1) m->tile[y][x] = (uint8_t)floor;
}

/* a corridor between two room centres: an L, recorded so a boss can stand in it */
static void corridor(SpurMap *m, Cell *to, int x0, int y0, int x1, int y1, int floor, bool hfirst) {
    int x = x0, y = y0;
    to->ncorr = 0;
    while (x != x1 || y != y1) {
        if (hfirst ? x != x1 : y == y1) x += isign(x1 - x);
        else y += isign(y1 - y);
        carve(m, x, y, floor);
        if (to->ncorr < 64) { to->corr[to->ncorr][0] = x; to->corr[to->ncorr][1] = y; to->ncorr++; }
    }
}

/* the mines' last stretch: two steps across, two down, over and over */
static void corridor_wind(SpurMap *m, Cell *to, int x0, int y0, int x1, int y1, int floor) {
    int x = x0, y = y0, step = 0;
    to->ncorr = 0;
    while (x != x1 || y != y1) {
        bool across = x != x1 && (y == y1 || (step / 2) % 2 == 0);
        if (across) x += isign(x1 - x);
        else y += isign(y1 - y);
        step++;
        carve(m, x, y, floor);
        if (to->ncorr < 64) { to->corr[to->ncorr][0] = x; to->corr[to->ncorr][1] = y; to->ncorr++; }
    }
}

static bool in_room(const Cell *c, int x, int y) { return x >= c->rx && y >= c->ry && x < c->rx + c->rw && y < c->ry + c->rh; }

static void style_tiles(int style, int *wall, int *floor, int *path) {
    switch (style) {
    case DS_MINE: *wall = T_ROCK; *floor = T_DIRT; *path = T_RAIL; break;
    case DS_PRISON: *wall = T_WALL; *floor = T_STONE; *path = T_STONE; break;
    case DS_PASS: *wall = T_MOUNT; *floor = T_DIRT; *path = T_SCRUB; break;
    case DS_CRYPT: *wall = T_WALL; *floor = T_STONE; *path = T_RUG; break;
    case DS_SNAKE: *wall = T_ROCK; *floor = T_DIRT; *path = T_DIRT; break;
    case DS_DREAM: *wall = T_GLASS; *floor = T_DREAMFLOOR; *path = T_DREAMFLOOR; break;
    case DS_TOWER: *wall = T_BONEWALL; *floor = T_HELLFLOOR; *path = T_HELLFLOOR; break;
    case DS_CITADEL: *wall = T_HELLWALL; *floor = T_HELLFLOOR; *path = T_RUG; break;
    default: *wall = T_ROCK; *floor = T_DIRT; *path = T_DIRT; break;
    }
}

/* how many open tiles can be walked to from a place (fire and bosses count as open) */
static uint8_t vis[SPUR_MAX_H][SPUR_MAX_W]; /* what the last reach_count could walk to */
static int reach_count(const SpurMap *m, int sx, int sy) {
    static int qx[SPUR_MAX_W * SPUR_MAX_H], qy[SPUR_MAX_W * SPUR_MAX_H];
    memset(vis, 0, sizeof vis);
    int qh = 0, qt = 0, n = 0;
    qx[qt] = sx; qy[qt] = sy; qt++;
    vis[sy][sx] = 1;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh];
        qh++;
        n++;
        for (int k = 0; k < 4; k++) {
            int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
            if (nx < 0 || ny < 0 || nx >= m->w || ny >= m->h || vis[ny][nx]) continue;
            int t = m->tile[ny][nx];
            if (!spur_tile_walk(t) && t != T_FIREWALL) continue;
            vis[ny][nx] = 1;
            qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    return n;
}

/* put something that blocks the way down only where it cuts nothing off */
static bool safe_put(SpurMap *m, int x, int y, int t, int sx, int sy) {
    int old = get(m, x, y);
    if (old < 0 || !spur_tile_walk(old) || (x == sx && y == sy)) return false;
    for (int i = 0; i < m->nlink; i++) if (m->link[i].x == x && m->link[i].y == y) return false;
    int before = reach_count(m, sx, sy);
    put(m, x, y, t);
    if (reach_count(m, sx, sy) >= before - 1) return true;
    put(m, x, y, old);
    return false;
}

/* a free corner of a room for something that blocks */
static bool room_spot(SpurMap *m, const Cell *cc, int t, int sx, int sy, int *ox, int *oy) {
    const int CX[4] = {cc->rx + cc->rw - 1, cc->rx, cc->rx + cc->rw - 1, cc->rx};
    const int CY[4] = {cc->ry, cc->ry, cc->ry + cc->rh - 1, cc->ry + cc->rh - 1};
    for (int k = 0; k < 4; k++)
        if (safe_put(m, CX[k], CY[k], t, sx, sy)) { *ox = CX[k]; *oy = CY[k]; return true; }
    return false;
}

/* ---- the set pieces each kind of dungeon is known for ------------------------------------ */
static bool tile_free(const SpurMap *m, int x, int y, int floor) {
    if (get(m, x, y) != floor) return false;
    for (int i = 0; i < m->nnpc; i++) if (m->npc[i].x == x && m->npc[i].y == y) return false;
    for (int i = 0; i < m->nchest; i++) if (m->chest[i].x == x && m->chest[i].y == y) return false;
    for (int i = 0; i < m->nambush; i++) if (m->ambush[i].x == x && m->ambush[i].y == y) return false;
    for (int i = 0; i < m->nlink; i++) if (m->link[i].x == x && m->link[i].y == y) return false;
    return true;
}
/* away from the doorways: no way in or out of the room touches it */
static bool inner_tile(const SpurMap *m, const Cell *cc, int x, int y) {
    for (int k = 0; k < 4; k++) {
        int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
        if (!in_room(cc, nx, ny) && spur_tile_walk(get(m, nx, ny))) return false;
    }
    return true;
}
/* a free floor tile in a room with a free floor tile beside it (to land on) */
static bool spot_with_landing(const SpurMap *m, const Cell *cc, int floor, int *x, int *y, int *lx, int *ly) {
    for (int yy = cc->ry; yy < cc->ry + cc->rh; yy++)
        for (int xx = cc->rx; xx < cc->rx + cc->rw; xx++) {
            if (!tile_free(m, xx, yy, floor) || !inner_tile(m, cc, xx, yy)) continue;
            for (int k = 0; k < 4; k++) {
                int nx = xx + (k == 0) - (k == 1), ny = yy + (k == 2) - (k == 3);
                if (in_room(cc, nx, ny) && tile_free(m, nx, ny, floor)) { *x = xx; *y = yy; *lx = nx; *ly = ny; return true; }
            }
        }
    return false;
}

static void set_pieces(SpurMap *m, const DunSpec *d, int di, Cell *c, int n, int entry, int far, int ex, int ey, int floor, int wall) {
    Cell *f = &c[far];
    int fx = f->rx + f->rw / 2, fy = f->ry + f->rh / 2;
    (void)di;
    /* the Bone Tower: past each rider, an exit portal straight back out to the Reach */
    if (d->style == DS_TOWER && d->exitkind == DX_NEXT)
        for (int k = 0; k < 4; k++) {
            int px = fx + (k == 0) - (k == 1), py = fy + (k == 2) - (k == 3);
            if (in_room(f, px, py) && tile_free(m, px, py, floor) && inner_tile(m, f, px, py) && m->nlink < SPUR_MAX_LINK) {
                put(m, px, py, T_PORTAL);
                m->link[m->nlink++] = (SpurLink){(int8_t)px, (int8_t)py, M_HELL, 32, 6};
                break;
            }
        }
    /* the mines: the winding last stretch has a gang waiting in it that you can't go round */
    if (d->style == DS_MINE && f->ncorr > 4 && m->nambush < SPUR_MAX_AMBUSH) {
        int k = f->ncorr / 2;
        while (k > 0) {
            int x = f->corr[k][0], y = f->corr[k][1];
            bool room = false;
            for (int i = 0; i < n; i++) room |= in_room(&c[i], x, y);
            if (!room && tile_free(m, x, y, get(m, x, y)) && spur_tile_walk(get(m, x, y))) break;
            k--;
        }
        if (k > 0) {
            int troop = d->ambush_troop[1] ? d->ambush_troop[1] : d->ambush_troop[0] ? d->ambush_troop[0] : SPUR_ZONE[d->zone].troop[0];
            int which = d->map == M_SIXPENNY ? 0 : d->map == M_ROOSTMINE ? 1 : 2;
            m->ambush[m->nambush++] = (SpurAmbush){(int8_t)f->corr[k][0], (int8_t)f->corr[k][1], (uint8_t)troop, (int16_t)(F_AMBUSH0 + 40 + which)};
        }
    }
    /* the snake tunnels: sinkholes that drop into sealed pockets (a rope climbs back) */
    if (d->style == DS_SNAKE) {
        int made = 0;
        for (int i = 0; i < n && made < 2; i++) {
            Cell *L = &c[i];
            if (i == far || i == entry || L->deg != 1 || L->parent < 0 || L->rw < 3 || L->rh < 3) continue;
            Cell *P = &c[L->parent];
            if (L->parent == entry || P->rw < 3 || P->rh < 3) continue;
            bool treasure = false;
            for (int k = 0; k < m->nchest; k++) treasure |= in_room(L, m->chest[k].x, m->chest[k].y);
            if (!treasure) continue;
            /* wall the room off, all round */
            static int8_t sx_[64], sy_[64];
            static uint8_t st_[64];
            int ns = 0;
            for (int y = L->ry - 1; y <= L->ry + L->rh && ns < 64; y++)
                for (int x = L->rx - 1; x <= L->rx + L->rw && ns < 64; x++) {
                    if (in_room(L, x, y) || get(m, x, y) < 0 || get(m, x, y) == wall) continue;
                    sx_[ns] = (int8_t)x; sy_[ns] = (int8_t)y; st_[ns] = (uint8_t)get(m, x, y); ns++;
                    put(m, x, y, wall);
                }
            /* the rest of the floor must still join up */
            reach_count(m, ex, ey);
            bool ok = true;
            for (int k = 0; k < n && ok; k++)
                if (k != i && c[k].rw >= 1) ok = vis[c[k].ry + c[k].rh / 2][c[k].rx + c[k].rw / 2];
            for (int k = 0; k < m->nlink && ok; k++) ok = vis[m->link[k].y][m->link[k].x];
            for (int k = 0; k < m->nnpc && ok; k++) ok = in_room(L, m->npc[k].x, m->npc[k].y) || vis[m->npc[k].y][m->npc[k].x];
            int hx, hy, hlx, hly, rx, ry, rlx, rly;
            ok = ok && spot_with_landing(m, P, floor, &hx, &hy, &hlx, &hly) && spot_with_landing(m, L, floor, &rx, &ry, &rlx, &rly) &&
                 m->nlink + 2 <= SPUR_MAX_LINK;
            if (!ok) {
                for (int k = 0; k < ns; k++) put(m, sx_[k], sy_[k], st_[k]);
                continue;
            }
            put(m, hx, hy, T_DOWN);
            put(m, rx, ry, T_UP);
            m->link[m->nlink++] = (SpurLink){(int8_t)hx, (int8_t)hy, (uint8_t)d->map, (int8_t)rlx, (int8_t)rly};
            m->link[m->nlink++] = (SpurLink){(int8_t)rx, (int8_t)ry, (uint8_t)d->map, (int8_t)hlx, (int8_t)hly};
            made++;
        }
    }
}

static void build_dungeon(SpurMap *m, const DunSpec *d, int di) {
    DRng r = {d->seed * 2654435761u + 17};
    m->w = d->w;
    m->h = d->h;
    int wall, floor, path;
    style_tiles(d->style, &wall, &floor, &path);
    rectf(m, 0, 0, m->w - 1, m->h - 1, wall);
    /* the citadel is a maze: smaller rooms, more of them, hardly any loops */
    const int CW = d->style == DS_CITADEL ? 6 : 8, CH = d->style == DS_CITADEL ? 5 : 7;
    int gw = (m->w - 1) / CW, gh = (m->h - 1) / CH;
    static Cell c[64];
    int n = gw * gh;
    memset(c, 0, sizeof c);
    for (int i = 0; i < n; i++) {
        int cx = (i % gw) * CW, cy = (i / gw) * CH;
        bool tiny = drand(&r, 0, 99) < 18;
        c[i].rw = tiny ? 1 : drand(&r, 3, CW - 2);
        c[i].rh = tiny ? 1 : drand(&r, 3, CH - 2);
        c[i].rx = cx + 1 + drand(&r, 0, CW - 1 - c[i].rw - 1);
        c[i].ry = cy + 1 + drand(&r, 0, CH - 1 - c[i].rh - 1);
        c[i].parent = -2;
    }
    /* the entrance: bottom row, middle */
    int entry = (gh - 1) * gw + gw / 2;
    if (c[entry].rw < 3) { c[entry].rw = 3; c[entry].rh = 3; c[entry].rx = (entry % gw) * CW + 2; c[entry].ry = (entry / gw) * CH + 2; }
    /* a spanning tree by a random walk with backtracking */
    int stack[64], sp = 0;
    stack[sp++] = entry;
    c[entry].parent = -1;
    c[entry].dist = 0;
    while (sp) {
        int cur = stack[sp - 1];
        int opts[4], no = 0;
        int cx = cur % gw, cy = cur / gw;
        if (cx > 0 && c[cur - 1].parent == -2) opts[no++] = cur - 1;
        if (cx < gw - 1 && c[cur + 1].parent == -2) opts[no++] = cur + 1;
        if (cy > 0 && c[cur - gw].parent == -2) opts[no++] = cur - gw;
        if (cy < gh - 1 && c[cur + gw].parent == -2) opts[no++] = cur + gw;
        if (!no) { sp--; continue; }
        int nx = opts[drand(&r, 0, no - 1)];
        c[nx].parent = cur;
        c[nx].dist = c[cur].dist + 1;
        c[cur].deg++;
        c[nx].deg++;
        stack[sp++] = nx;
    }
    /* the far end: the deepest leaf */
    int far = entry;
    for (int i = 0; i < n; i++)
        if (i != entry && c[i].deg == 1 && c[i].dist > c[far].dist) far = i;
    if (c[far].rw < 3) { c[far].rw = 3; c[far].rh = 3; c[far].rx = (far % gw) * CW + 2; c[far].ry = (far / gw) * CH + 2; }
    /* carve the rooms and the tree's corridors */
    for (int i = 0; i < n; i++)
        for (int y = c[i].ry; y < c[i].ry + c[i].rh; y++)
            for (int x = c[i].rx; x < c[i].rx + c[i].rw; x++) carve(m, x, y, floor);
    for (int i = 0; i < n; i++) {
        if (c[i].parent < 0) continue;
        int p = c[i].parent;
        bool hfirst = drand(&r, 0, 1) == 0;
        if (d->style == DS_MINE && i == far)
            corridor_wind(m, &c[i], c[p].rx + c[p].rw / 2, c[p].ry + c[p].rh / 2, c[i].rx + c[i].rw / 2, c[i].ry + c[i].rh / 2, path);
        else
            corridor(m, &c[i], c[p].rx + c[p].rw / 2, c[p].ry + c[p].rh / 2, c[i].rx + c[i].rw / 2, c[i].ry + c[i].rh / 2,
                     path, hfirst);
    }
    /* a few loops, never into the far room */
    int loops = d->style == DS_CITADEL ? n / 16 : n / 6;
    for (int k = 0; k < loops * 4 && loops > 0; k++) {
        int a = drand(&r, 0, n - 1);
        int b = drand(&r, 0, 1) ? a + 1 : a + gw;
        if (b >= n || (b == a + 1 && b % gw == 0)) continue;
        if (a == far || b == far || c[a].parent == b || c[b].parent == a) continue;
        Cell tmp;
        corridor(m, &tmp, c[a].rx + c[a].rw / 2, c[a].ry + c[a].rh / 2, c[b].rx + c[b].rw / 2, c[b].ry + c[b].rh / 2, path,
                 drand(&r, 0, 1) == 0);
        loops--;
    }
    /* the far room must be reached by its own corridor only: wall off anything else that touched it */
    {
        Cell *f = &c[far];
        for (int y = f->ry - 1; y <= f->ry + f->rh; y++)
            for (int x = f->rx - 1; x <= f->rx + f->rw; x++) {
                if (in_room(f, x, y)) continue;
                bool own = false;
                for (int k = 0; k < f->ncorr; k++) own |= f->corr[k][0] == x && f->corr[k][1] == y;
                if (!own && get(m, x, y) != wall) put(m, x, y, wall);
            }
    }
    /* the entrance */
    int ex = c[entry].rx + c[entry].rw / 2, ey = c[entry].ry + c[entry].rh - 1;
    if (d->prev < 0) {
        put(m, ex, ey, T_EXIT);
        if (d->map == M_CRYPT) m->link[m->nlink++] = (SpurLink){(int8_t)ex, (int8_t)ey, M_THORNS, -3, 0};
        else if (d->map == M_ROOSTMINE) m->link[m->nlink++] = (SpurLink){(int8_t)ex, (int8_t)ey, M_ROOST, -3, 0};
        else if (d->map == M_DREAM1) m->link[m->nlink++] = (SpurLink){(int8_t)ex, (int8_t)ey, M_SHRINE, -3, 0};
        else if (d->map == M_TOWER1) m->link[m->nlink++] = (SpurLink){(int8_t)ex, (int8_t)ey, M_HELL, 32, 6};
        else {
            const Poi *pp = &WORLD_POI[poi_index(d->map, 0)];
            m->link[m->nlink++] = (SpurLink){(int8_t)ex, (int8_t)ey, M_WORLD, pp->x, pp->y};
        }
    } else {
        bool up = d->style == DS_TOWER || d->style == DS_CITADEL;
        put(m, ex, ey, up ? T_DOWN : T_UP);
        m->link[m->nlink++] = (SpurLink){(int8_t)ex, (int8_t)ey, (uint8_t)d->prev, -2, 0};
    }
    m->start_x = ex;
    m->start_y = ey;
    /* the far end */
    Cell *f = &c[far];
    int fx = f->rx + f->rw / 2, fy = f->ry + f->rh / 2;
    m->back_x = fx;
    m->back_y = fy;
    if (d->exitkind == DX_NEXT) {
        bool up = d->style == DS_TOWER || d->style == DS_CITADEL;
        put(m, fx, fy, up ? T_UP : T_DOWN);
        m->link[m->nlink++] = (SpurLink){(int8_t)fx, (int8_t)fy, d->next, -1, 0};
    } else if (d->exitkind == DX_OUT) {
        put(m, fx, fy, T_EXIT);
        const Poi *pp = &WORLD_POI[poi_index(d->map, 1)];
        m->link[m->nlink++] = (SpurLink){(int8_t)fx, (int8_t)fy, M_WORLD, pp->x, pp->y};
    }
    if (d->map == M_TOWER4) {
        /* past the Reaper: the way home, once he is beaten */
        put(m, fx, fy, spur_flag(F_REAPER) ? T_PORTAL : floor);
        m->link[m->nlink++] = (SpurLink){(int8_t)fx, (int8_t)fy, M_WORLD, 24, 27};
    }
    if (d->map == M_DREAM2) {
        put(m, fx, fy, spur_flag(F_MOTHER) ? T_PORTAL : floor);
        m->link[m->nlink++] = (SpurLink){(int8_t)fx, (int8_t)fy, M_SHRINE, -3, 0};
    }
    /* the prize in the last room */
    int kx = f->rx, ky = f->ry;
    if (d->key && room_spot(m, f, T_CHEST, ex, ey, &kx, &ky)) {
        m->chest[m->nchest] = (SpurChest){(int8_t)kx, (int8_t)ky, (uint16_t)(d->map * 16 + m->nchest),
                                          (int16_t)(d->key >= 1000 ? 1000 + (d->key - 1000) : d->key), 0, 0};
        m->nchest++;
        if (d->ambush_far && m->nambush < SPUR_MAX_AMBUSH) {
            int axx = kx, ayy = ky + 1;
            for (int q = 0; q < 4; q++) {
                int nx = kx + (q == 0) - (q == 1), ny = ky + (q == 2) - (q == 3);
                if (spur_tile_walk(get(m, nx, ny))) { axx = nx; ayy = ny; break; }
            }
            m->ambush[m->nambush++] = (SpurAmbush){(int8_t)axx, (int8_t)ayy, d->ambush_troop[0], (int16_t)(F_AMBUSH0 + di * 2)};
        }
    }
    int sx = 0, sy = 0;
    if (d->story_npc >= 0 && m->nnpc < SPUR_MAX_NPC && room_spot(m, f, T_WALL, ex, ey, &sx, &sy)) {
        put(m, sx, sy, floor);
        m->npc[m->nnpc] = (SpurNpc){(int8_t)sx, (int8_t)sy, (uint8_t)d->story_npc, d->story_pal, 0, 0,
                                    (uint16_t)(d->map * 32 + m->nnpc), -1, -1};
        if (d->map == M_PEN2) { m->npc[m->nnpc].show_if = F_SCOURGE; m->npc[m->nnpc].hide_if = F_AMBROSE; }
        if (d->map == M_DRYWASH) m->npc[m->nnpc].hide_if = F_PIKE_FOUND;
        m->nnpc++;
    }
    /* the boss in the last corridor (and the fire before it) */
    if (d->boss_troop && f->ncorr > 0) {
        int k = f->ncorr - 1;
        while (k > 0 && (in_room(f, f->corr[k][0], f->corr[k][1]))) k--;
        int bx = f->corr[k][0], by = f->corr[k][1];
        m->npc[m->nnpc] = (SpurNpc){(int8_t)bx, (int8_t)by, LOOK_BOSS, d->boss_art, 0, 0, (uint16_t)(d->map * 32 + m->nnpc), -1,
                                    d->boss_flag};
        m->nnpc++;
        if (d->block && k > 0) put(m, f->corr[k - 1][0], f->corr[k - 1][1], d->block);
    }
    /* chests in the dead ends, deepest first */
    int order[64], no = 0;
    for (int i = 0; i < n; i++)
        if (i != far && i != entry && c[i].rw >= 3) order[no++] = i;
    for (int a = 0; a < no; a++)
        for (int b = a + 1; b < no; b++) {
            int ka = c[order[a]].dist * 10 + (c[order[a]].deg == 1) * 100, kb = c[order[b]].dist * 10 + (c[order[b]].deg == 1) * 100;
            if (kb > ka) { int t = order[a]; order[a] = order[b]; order[b] = t; }
        }
    int ci = 0, amb = 0;
    for (int k = 0; k < 10 && d->chest[k] && ci < no; k++, ci++) {
        Cell *cc = &c[order[ci]];
        int x, y;
        if (!room_spot(m, cc, T_CHEST, ex, ey, &x, &y)) { k--; continue; }
        int item = d->chest[k] > 0 ? d->chest[k] : 0, teeth = d->chest[k] < 0 ? -d->chest[k] : 0;
        m->chest[m->nchest] = (SpurChest){(int8_t)x, (int8_t)y, (uint16_t)(d->map * 16 + m->nchest), (int16_t)item, teeth, 0};
        m->nchest++;
        /* the two best finds are guarded */
        if (amb < 2 && d->ambush_troop[amb] && !(amb == 0 && d->ambush_far) && m->nambush < SPUR_MAX_AMBUSH &&
            (k == 5 || k == 2 || k == 0)) {
            /* the guard stands on the open tile beside the chest */
            int axx = x - 1, ayy = y;
            for (int q = 0; q < 4; q++) {
                int nx = x + (q == 0) - (q == 1), ny = y + (q == 2) - (q == 3);
                if (spur_tile_walk(get(m, nx, ny))) { axx = nx; ayy = ny; break; }
            }
            m->ambush[m->nambush++] = (SpurAmbush){(int8_t)axx, (int8_t)ayy, d->ambush_troop[amb], (int16_t)(F_AMBUSH0 + di * 2 + amb)};
            amb++;
        }
    }
    /* ore seams in the rooms' top walls */
    for (int k = 0; k < d->ores && ci < no; k++, ci++) {
        Cell *cc = &c[order[ci]];
        int x = cc->rx + cc->rw / 2 - 1, y = cc->ry - 1;
        if (x == cc->rx + cc->rw / 2) x--;
        if (get(m, x, y) == wall && m->nspot < SPUR_MAX_SPOT) {
            put(m, x, y, T_ORE);
            m->spot[m->nspot++] = (SpurSpot){(int8_t)x, (int8_t)y, SPOT_ORE, d->ore_table};
        }
    }
    /* an animal for Sage in a room half way in */
    if (d->animal >= 0) {
        int best = -1;
        for (int i = 0; i < n; i++)
            if (i != far && i != entry && c[i].rw >= 3 && (best < 0 || iabs(c[i].dist - c[far].dist / 2) < iabs(c[best].dist - c[far].dist / 2)))
                best = i;
        int ax, ay;
        if (best >= 0 && room_spot(m, &c[best], T_WALL, ex, ey, &ax, &ay)) {
            put(m, ax, ay, floor);
            m->npc[m->nnpc] = (SpurNpc){(int8_t)ax, (int8_t)ay, LOOK_ANIMAL, (uint8_t)d->animal, 0, 0, (uint16_t)(d->map * 32 + m->nnpc), -1, -1};
            m->nnpc++;
        }
    }
    /* the alligator run: ambushes that never stop coming */
    if (d->loop_ambush) {
        int best = -1;
        for (int i = 0; i < n; i++)
            if (i != far && i != entry && c[i].rw >= 4 && c[i].deg >= 2 && (best < 0 || c[i].dist > c[best].dist)) best = i;
        if (best >= 0)
            for (int k = 0; k < 2 && m->nambush < SPUR_MAX_AMBUSH; k++)
                m->ambush[m->nambush++] = (SpurAmbush){(int8_t)(c[best].rx + 1 + k), (int8_t)(c[best].ry + c[best].rh / 2), (uint8_t)d->loop_ambush, -1};
    }
    /* the citadel's winches, three a floor: wind all six and the lift at the gate rises */
    if (d->map == M_SPUR1 || d->map == M_SPUR2) {
        int placed = 0;
        for (int i = n - 1; i >= 0 && placed < 3; i--) {
            if (i == far || i == entry || c[i].rw < 4) continue;
            int x = c[i].rx, y = c[i].ry + c[i].rh - 1;
            put(m, x, y, T_WINCH);
            placed++;
        }
        if (d->map == M_SPUR1) {
            int lx = c[entry].rx + c[entry].rw - 1, ly = c[entry].ry;
            bool up = true;
            for (int k = 4; k < 10; k++) up &= spur_flag(F_MISC0 + k);
            put(m, lx, ly, up ? T_LIFT : T_WALL);
            m->link[m->nlink++] = (SpurLink){(int8_t)lx, (int8_t)ly, M_SPUR3, -1, 0};
        }
    }
    /* decoration that never blocks a way: pools, rails, bones */
    for (int i = 0; i < n; i++) {
        Cell *cc = &c[i];
        if (cc->rw < 5 || cc->rh < 4 || i == far || i == entry) continue;
        int deco = d->style == DS_SNAKE ? T_WATER : d->style == DS_TOWER || d->style == DS_CITADEL ? T_LAVA
                 : d->style == DS_CRYPT ? T_GRAVE : d->style == DS_DREAM ? T_TREE : d->style == DS_PASS ? T_TREE
                 : d->style == DS_PRISON ? T_BARS : T_CRATE;
        int x = cc->rx, y = cc->ry + cc->rh - 1;
        if (get(m, x, y) == floor) safe_put(m, x, y, deco, ex, ey);
    }
    set_pieces(m, d, di, c, n, entry, far, ex, ey, floor, wall);
    m->zone = d->zone;
}

/* ---- the builder --------------------------------------------------------------------- */
void spur_world_build(SpurMap *m, int id) {
    memset(m, 0, sizeof *m);
    m->id = id;
    m->kind = SPUR_MAPINFO[id].kind;
    m->zone = -1;
    if (id == M_WORLD) build_world(m);
    else if (id == M_HELL) build_hell(m);
    else if (id == M_INSIDE) build_inside(m);
    else {
        bool done = false;
        for (int i = 0; i < ARRAY_LEN(HAND) && !done; i++)
            if (HAND[i].map == id) { build_hand(m, &HAND[i]); done = true; }
        for (int i = 0; i < (int)DUNS && !done; i++)
            if (DUN[i].map == id) { build_dungeon(m, &DUN[i], i); done = true; }
    }
    /* the place's doors out: every walkable edge tile of a town */
    if (m->kind == MK_TOWN) {
        for (int y = 0; y < m->h; y++)
            for (int x = 0; x < m->w; x++)
                if ((x == 0 || y == 0 || x == m->w - 1 || y == m->h - 1) && spur_tile_walk(m->tile[y][x]))
                    m->tile[y][x] = m->tile[y][x] == T_ROAD ? T_ROAD : m->tile[y][x];
    }
}

int spur_tile_at(const SpurMap *m, int x, int y) {
    if (x < 0 || y < 0 || x >= m->w || y >= m->h) return -1;
    return m->tile[y][x];
}

/* ---- encounter zones on the big maps --------------------------------------------------- */
int spur_world_zone(int map, int x, int y) {
    if (map == M_HELL) return 16;
    if (map != M_WORLD) return -1;
    static const struct { int8_t x0, y0, x1, y1, z; } Z[] = {
        {0, 0, 34, 22, 22}, {36, 0, 58, 22, 10}, {0, 26, 58, 50, 13}, {70, 44, 80, 50, 7}, {63, 0, 113, 50, 6},
        {98, 57, 113, 66, 25}, {62, 57, 113, 92, 3}, {30, 89, 50, 95, 23}, {0, 57, 58, 92, 0},
    };
    for (int i = 0; i < ARRAY_LEN(Z); i++)
        if (x >= Z[i].x0 && x <= Z[i].x1 && y >= Z[i].y0 && y <= Z[i].y1) return Z[i].z;
    return 22;
}

/* ---- checks for the tests -------------------------------------------------------------- */
static uint8_t seen[SPUR_MAX_H][SPUR_MAX_W];
static int flood_run(const SpurMap *m, int sx, int sy, bool open_all);
static int flood(const SpurMap *m, int sx, int sy, bool open_all) {
    memset(seen, 0, sizeof seen);
    int cnt = flood_run(m, sx, sy, open_all);
    /* a floor's own sinkholes lead on to more of it */
    for (bool more = true; more;) {
        more = false;
        for (int i = 0; i < m->nlink; i++) {
            const SpurLink *l = &m->link[i];
            if (l->map != m->id || l->tx < 0 || !seen[l->y][l->x] || seen[l->ty][l->tx]) continue;
            cnt += flood_run(m, l->tx, l->ty, open_all);
            more = true;
        }
    }
    return cnt;
}
static int flood_run(const SpurMap *m, int sx, int sy, bool open_all) {
    int qh = 0, qt = 0, cnt = 0;
    static int bigx[SPUR_MAX_W * SPUR_MAX_H], bigy[SPUR_MAX_W * SPUR_MAX_H];
    bigx[qt] = sx; bigy[qt] = sy; qt++;
    seen[sy][sx] = 1;
    while (qh < qt) {
        int x = bigx[qh], y = bigy[qh];
        qh++;
        cnt++;
        for (int k = 0; k < 4; k++) {
            int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
            int t = spur_tile_at(m, nx, ny);
            if (t < 0 || seen[ny][nx]) continue;
            bool w = spur_tile_walk(t) || (open_all && (t == T_FIREWALL || t == T_LOCKED || t == T_FERRY || t == T_ANGELS || t == T_RUBBLE));
            bool npc = false;
            for (int i = 0; i < m->nnpc; i++) npc |= m->npc[i].x == nx && m->npc[i].y == ny && m->npc[i].look == LOOK_BOSS;
            if (!w && !npc) continue;
            seen[ny][nx] = 1;
            bigx[qt] = nx; bigy[qt] = ny; qt++;
        }
    }
    return cnt;
}

/* every link, chest and person on a map can be reached from where you come in */
static int check_one(int id) {
    static SpurMap m;
    spur_world_build(&m, id);
    int bad = 0;
    if (id == M_WORLD) return 0;
    flood(&m, m.start_x, m.start_y, true);
    for (int i = 0; i < m.nlink; i++) {
        int x = m.link[i].x, y = m.link[i].y;
        bool ok = seen[y][x];
        for (int k = 0; k < 4 && !ok; k++) {
            int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
            if (nx >= 0 && ny >= 0 && nx < m.w && ny < m.h) ok |= seen[ny][nx];
        }
        /* the citadel's lift and the summit link wait for the story */
        if (!ok && !(id == M_SPUR1 && m.link[i].map == M_SPUR3) && !(id == M_THORNS) && !(id == M_SHRINE) && !(id == M_DREAM2) && !(id == M_TOWER4) && !(id == M_ROOST)) {
            bad++;
            fprintf(stderr, "map %s: link %d at %d,%d cut off\n", spur_map_name(id), i, x, y);
        }
    }
    for (int i = 0; i < m.nchest; i++) {
        int x = m.chest[i].x, y = m.chest[i].y;
        bool ok = false;
        for (int k = 0; k < 4 && !ok; k++) {
            int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
            if (nx >= 0 && ny >= 0 && nx < m.w && ny < m.h) ok |= seen[ny][nx];
        }
        if (!ok) { bad++; fprintf(stderr, "map %s: chest %d at %d,%d cut off\n", spur_map_name(id), i, x, y); }
    }
    for (int i = 0; i < m.nnpc; i++) {
        int x = m.npc[i].x, y = m.npc[i].y;
        if (x == 0 && y == 0 && m.npc[i].talk == 0) continue; /* lives indoors */
        bool ok = seen[y][x];
        for (int k = 0; k < 4 && !ok; k++) {
            int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
            if (nx >= 0 && ny >= 0 && nx < m.w && ny < m.h) ok |= seen[ny][nx];
            /* or across a counter, an altar or a grille */
            int t = spur_tile_at(&m, nx, ny), fx = nx + (k == 0) - (k == 1), fy = ny + (k == 2) - (k == 3);
            if ((t == T_COUNTER || t == T_ALTAR || t == T_BARS) && fx >= 0 && fy >= 0 && fx < m.w && fy < m.h) ok |= seen[fy][fx];
        }
        if (!ok) { bad++; fprintf(stderr, "map %s: person %d at %d,%d cut off\n", spur_map_name(id), i, x, y); }
    }
    if (bad && getenv("SPUR_MAPDEBUG"))
        for (int y = 0; y < m.h; y++) {
            for (int x = 0; x < m.w; x++) {
                int t = m.tile[y][x];
                char c = seen[y][x] ? '.' : spur_tile_walk(t) ? 'o' : '#';
                for (int i = 0; i < m.nnpc; i++) if (m.npc[i].x == x && m.npc[i].y == y) c = 'N';
                for (int i = 0; i < m.nchest; i++) if (m.chest[i].x == x && m.chest[i].y == y) c = 'c';
                for (int i = 0; i < m.nlink; i++) if (m.link[i].x == x && m.link[i].y == y) c = 'L';
                if (t == T_LAVA) c = '~';
                fputc(c, stderr);
            }
            fputc('\n', stderr);
        }
    return bad;
}

int spur_map_check(int id) {
    if (id != M_INSIDE) return check_one(id);
    /* every room behind every town door */
    static const int SERVICES[] = {SV_HOTEL, SV_CHAPEL, SV_SHOP_A, SV_SHOP_B, SV_BANK, SV_SALOON, SV_SHERIFF, SV_STORY};
    unsigned char keep[sizeof sg.inside];
    memcpy(keep, &sg.inside, sizeof keep);
    int bad = 0;
    for (int town = SPUR_FIRST_TOWN; town < SPUR_FIRST_TOWN + SPUR_TOWNS; town++)
        for (int k = 0; k < ARRAY_LEN(SERVICES); k++) {
            int x, y;
            if (!spur_has_inside(town, SERVICES[k]) || !spur_town_door(town, SERVICES[k], &x, &y)) continue;
            for (int f = 0; f <= (SERVICES[k] == SV_HOTEL); f++) {
                sg.inside.town = (uint8_t)town;
                sg.inside.service = (uint8_t)SERVICES[k];
                sg.inside.floor = (uint8_t)f;
                sg.inside.dx = (int8_t)x;
                sg.inside.dy = (int8_t)y;
                int b = check_one(M_INSIDE);
                if (b) fprintf(stderr, "  (town %d, door %d, floor %d)\n", town, SERVICES[k], f);
                bad += b;
            }
        }
    memcpy(&sg.inside, keep, sizeof keep);
    return bad;
}

/* the Territory on foot: which places can be walked to from Gladwell with the
 * story's gates as the flags say (bit per map in the result) */
uint64_t spur_world_reach(int sx, int sy) {
    static SpurMap m;
    spur_world_build(&m, M_WORLD);
    memset(seen, 0, sizeof seen);
    static int qx[SPUR_MAX_W * SPUR_MAX_H], qy[SPUR_MAX_W * SPUR_MAX_H];
    int qh = 0, qt = 0;
    qx[qt] = sx; qy[qt] = sy; qt++;
    seen[sy][sx] = 1;
    uint64_t got = 0;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh];
        qh++;
        int p = spur_poi_at(M_WORLD, x, y);
        if (p >= 0) got |= (uint64_t)1 << p;
        if (p >= 0 && !(x == sx && y == sy)) continue; /* a door is a stop, not a way through */
        for (int k = 0; k < 4; k++) {
            int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
            int t = spur_tile_at(&m, nx, ny);
            if (t < 0 || seen[ny][nx]) continue;
            bool w = spur_tile_walk(t) || (t == T_FERRY && spur_flag(F_FERRY));
            if (!w) continue;
            seen[ny][nx] = 1;
            qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    return got;
}

int spur_chest_total(void) {
    static SpurMap m;
    int n = 0;
    for (int id = 0; id < M_COUNT; id++) {
        if (id == M_INSIDE) continue;
        spur_world_build(&m, id);
        n += m.nchest;
    }
    return n + spur_inside_finds();
}
