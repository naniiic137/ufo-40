/* ANTIPODE - Tallow Station: the four sector maps, the room table, the cell
 * templates and building a room's tiles. All our own map: only the
 * structure follows the original (three sectors of rooms joined by doors,
 * teleporters between them, a fourth sector opened by four ciphers). */
#include "apd.h"

const char *const APD_SECTOR_NAME[APD_SECTORS] = {"GANTRY", "BRINE", "COIL", "THE CROWN"};

/* ---- the sector maps ------------------------------------------------------
 * Even rows are cells: a letter is a room (the same letter side by side is
 * one room), '.' no cell. Between two cells of different rooms sits an edge:
 *   ' ' wall      'o' a door (or an open shaft between floors)
 *   '1' '2' '3'   a door that needs that pass level
 *   'b' a boss door (it shuts behind you)   'n' a nano-wall (sentry orbs)
 *   'h' orange barrels (SCORCH burns them)  'H' a hidden wall that burns
 *   'm' cracked concrete in a shaft (ANVIL) 'e' an electric field (STORMBALL)
 *   'z' a shutter opened by a target inside the wall (PHASER)
 * Odd rows hold the edges between a cell and the one below it. Between two
 * cells of one room the edge character is only there to keep the columns. */
static const char *const GANTRY_MAP[] = {
    ". . . . F . . . .",
    "        o        ",
    ". a-aoC-C-C2doe-e",
    "  o       o e   o",
    "NzfhgbW-W hoi . j",
    "  o       o o   |",
    "lomoPoS-Son Hhooj",
    "| o   o m h   | m",
    "l rosot u vowoo y",
    "| 1   o       o |",
    "lhA-AocoD . E-E y",
    "h   o   2     o  ",
    "LHXHM . K . . R .",
    NULL,
};
static const char *const BRINE_MAP[] = {
    "aob-boced-deeeZ",
    "|   m 1   o |  ",
    "a f g-goh2i ebY",
    "o | o   o | e |",
    "j1f kol-l iom Y",
    "o   |     o |  ",
    "n1o kbB-B p moq",
    "o | m | | o   |",
    "r oos B-B tou q",
    "o 1       o o o",
    "Tov-vow1x-xoyoU",
    NULL,
};
static const char *const COIL_MAP[] = {
    "a-aob2c-c3doezZ",
    "o   |   o | |  ",
    "f g boh i d e3P",
    "o |     b o    ",
    "Togoj k-kblom-m",
    "  o | | |   o e",
    "noo j k-k poq r",
    "| m o     | o |",
    "n sot2u vnp w r",
    "o   |     o |  ",
    "x-x tby-ybzow .",
    NULL,
};
static const char *const CROWN_MAP[] = {
    "Toa F-F",
    "  o | |",
    ". bbF-F",
    NULL,
};
static const char *const *const SECTOR_MAPS[APD_SECTORS] = {GANTRY_MAP, BRINE_MAP, COIL_MAP, CROWN_MAP};

/* ---- the cell templates ----------------------------------------------------
 * The 18 x 8 inside of a cell (its border is wall, cut open where doors and
 * joins are). '#' wall, '^' spikes on the floor, 'v' spikes on the ceiling,
 * '=' a one-way plate (solid from the side your feet are on), 'a' acid,
 * 'r' a red spike ball (live while gravity points down), 'p' a purple one
 * (live while it points up), 'G' a grey wall, '%' a cover, '*' an item's
 * place, 'b' a button. The middle two columns of the top and bottom rows
 * and the middle rows at the sides are kept for shafts and doors. */
typedef struct { char id; const char *row[8]; } Tpl;
static const Tpl TPL[] = {
    {'A', {"..................", "..................", "..................", "..................",
           "..................", "..................", "..................", ".................."}},
    {'B', {"..................", "..................", "..................", "..................",
           ".....##.....##....", ".....##.....##....", ".....##.....##....", ".....##.....##...."}},
    {'C', {".....##.....##....", ".....##.....##....", ".....##.....##....", ".....##.....##....",
           "..................", "..................", "..................", ".................."}},
    {'D', {"..................", "..................", "..................", "..................",
           "..................", "..................", "..................", "....^^^^^^^^^^...."}},
    {'E', {"....vvvvvvvvvv....", "..................", "..................", "..................",
           "..................", "..................", "..................", ".................."}},
    {'F', {".........vvvvvvv..", "..................", "..................", "..................",
           "..................", "..................", "..................", "..^^^^^^^........."}},
    {'G', {"..vvvvvvv.........", "..................", "..................", "..................",
           "..................", "..................", "..................", ".........^^^^^^^.."}},
    {'H', {"...####.....####..", "...####.....####..", "..................", "..................",
           "..................", "..................", "......####........", "......####........"}},
    {'I', {"..................", "..................", "..................", ".....========.....",
           "..................", "..................", "..................", ".................."}},
    {'J', {"..................", "..................", "..................", "..................",
           "..................", "............###...", ".........######...", ".....##########..."}},
    {'K', {".....##########...", ".........######...", "............###...", "..................",
           "..................", "..................", "..................", ".................."}},
    {'L', {"...........##.....", "...........##.....", "...........##.....", "..................",
           "..................", "...........##.....", "...........##.....", "...........##....."}},
    {'M', {"..................", "..................", "..................", "..................",
           "..................", "...##.......##....", "...##.......##....", "^^^##^^^^^^^##^^^^"}},
    {'N', {"vvv##vvvvvvv##vvvv", "...##.......##....", "...##.......##....", "..................",
           "..................", "..................", "..................", ".................."}},
    {'O', {"..................", "..................", "....p.......p.....", "..................",
           "..................", ".......r......r...", "..................", ".................."}},
    {'P', {"..................", "..==....==....==..", "..................", ".....==....==.....",
           "..................", "..==....==....==..", "..................", ".................."}},
    {'Q', {"..................", "..................", "......######......", "......######......",
           "......######......", "......######......", "..................", ".................."}},
    {'R', {".....vv......vv...", "..................", "..................", "..................",
           "..................", "..................", "..................", "..^^......^^......"}},
    {'S', {"..................", "..................", "..................", "..................",
           "..................", "..................", "..aaaaaaaaaaaaaa..", "..aaaaaaaaaaaaaa.."}},
    {'T', {"..................", "..................", "..................", "..................",
           "..................", "....#....#....#...", "....#....#....#...", "aaaa#aaaa#aaaa#aaa"}},
    {'V', {"..................", ".###..........###.", "..................", "..................",
           "..................", "..................", ".###..........###.", ".................."}},
    {'W', {"..##....##....##..", "..##....##....##..", "..................", "..................",
           "..................", "..................", "......##....##....", "......##....##...."}},
    /* the hidden room by the west teleporter: a sealed closet up top, two
     * buttons sunk in the floor, far apart */
    {'x', {"......######......", "......#.*..#......", "......#GGGG#......", "..................",
           "..................", "..................", "..................", ".#b#..........#b#."}},
    /* the start: a desk with a cover over a hidden cell */
    {'s', {"..................", "..................", "..................", "..................",
           "..................", "..................", "..................", "..............*%.."}},
};

/* ---- the rooms ------------------------------------------------------------
 * things: space-separated, each with an optional @ and a cell (0 = the first
 * cell, row by row) and an anchor: f on a floor, c under a ceiling, m in the
 * air, h under a cover, a in the acid, e behind the last foe placed, t at
 * the template's '*'.
 *   pod  logN  passN  teleN  start  buttons  bossN
 *   armor spare damper modN (the four mods: 0 EMBER, 1 TWIN, 2 VOLT, 3 MASS)
 *   tread treadr treadg spitter eye hive skip skimmer sentry (*N for several) */
const ApdSpec APD_SPECS[] = {
    /* ---- the Gantry ---- */
    {SEC_GANTRY, 'F', "A", 0, "tele9", "THE UPLINK"},
    {SEC_GANTRY, 'a', "IC", 0, "tread*2", NULL},
    {SEC_GANTRY, 'C', "AQA", 0, "pod@1 log3@2 spitter@0 tread@2", "THE CONCOURSE"},
    {SEC_GANTRY, 'd', "D", 0, "treadr skimmer", NULL},
    {SEC_GANTRY, 'e', "MN", 0, "eye@0 armor@1m", NULL},
    {SEC_GANTRY, 'N', "A", 0, "armor@0f skimmer", NULL},
    {SEC_GANTRY, 'f', "P", 0, "tread skimmer", NULL},
    {SEC_GANTRY, 'g', "H", 0, "treadr*2", NULL},
    {SEC_GANTRY, 'W', "AA", RF_ARENA, "boss1", "THE WARDEN'S HALL"},
    {SEC_GANTRY, 'h', "B", 0, "spitter tread", NULL},
    {SEC_GANTRY, 'i', "I", 0, "armor@0c tread", NULL},
    {SEC_GANTRY, 'j', "VV", RF_HEAT, "skimmer*2", "THE FLUE"},
    {SEC_GANTRY, 'l', "VPV", 0, "eye@1 armor@1e skimmer@2", NULL},
    {SEC_GANTRY, 'm', "A", 0, "log1 tread", NULL},
    {SEC_GANTRY, 'P', "A", 0, "pod", NULL},
    {SEC_GANTRY, 'S', "sA", RF_START, "start@0 log0@0 armor@0h", "THE CRADLE BAY"},
    {SEC_GANTRY, 'n', "C", 0, "tread", NULL},
    {SEC_GANTRY, 'H', "J", 0, "mod0@0f spitter@0", NULL},
    {SEC_GANTRY, 'o', "VW", 0, "tread*2 skip@1", NULL},
    {SEC_GANTRY, 'r', "E", 0, "tread skip", NULL},
    {SEC_GANTRY, 's', "R", 0, "spitter*2", NULL},
    {SEC_GANTRY, 't', "F", 0, "treadr", NULL},
    {SEC_GANTRY, 'u', "A", 0, "spare@0f", NULL},
    {SEC_GANTRY, 'v', "Q", 0, "armor@0f tread*2", NULL},
    {SEC_GANTRY, 'w', "G", 0, "skip*2", NULL},
    {SEC_GANTRY, 'y', "VM", 0, "hive@0 armor@1m", NULL},
    {SEC_GANTRY, 'A', "KA", 0, "armor@1f log2@1 treadr*2", "THE ARMORY"},
    {SEC_GANTRY, 'c', "O", 0, "tread", NULL},
    {SEC_GANTRY, 'D', "B", 0, "tread spitter log5", NULL},
    {SEC_GANTRY, 'E', "AI", 0, "pod@0 skimmer@1", NULL},
    {SEC_GANTRY, 'L', "A", 0, "tele1", NULL},
    {SEC_GANTRY, 'X', "x", 0, "spare@0t buttons log4@0", NULL},
    {SEC_GANTRY, 'M', "A", RF_ARENA, "boss0", NULL},
    {SEC_GANTRY, 'K', "A", 0, "tele3", NULL},
    {SEC_GANTRY, 'R', "A", 0, "tele2", NULL},
    /* ---- the Brine ---- */
    {SEC_BRINE, 'a', "AS", RF_ACID, "pod@0 armor@1a", NULL},
    {SEC_BRINE, 'b', "TS", RF_ACID, "armor@0m tread@1 skimmer", NULL},
    {SEC_BRINE, 'c', "E", 0, "tread spitter", NULL},
    {SEC_BRINE, 'd', "LO", 0, "skimmer*2 eye@1", NULL},
    {SEC_BRINE, 'e', "AV", 0, "pod@0 skip@1", NULL},
    {SEC_BRINE, 'Z', "A", 0, "damper@0f log9@0", NULL},
    {SEC_BRINE, 'f', "VJ", 0, "mod3@1f treadr@1", NULL},
    {SEC_BRINE, 'g', "BR", 0, "treadr spitter", NULL},
    {SEC_BRINE, 'h', "A", 0, "armor@0c tread", NULL},
    {SEC_BRINE, 'i', "VV", 0, "log8@1 skimmer skip@1", NULL},
    {SEC_BRINE, 'Y', "AA", RF_ARENA, "boss5", "THE MIRROR ROOM"},
    {SEC_BRINE, 'j', "E", 0, "tread", NULL},
    {SEC_BRINE, 'k', "VV", 0, "skip*2", NULL},
    {SEC_BRINE, 'l', "AI", 0, "pass2@0 log7@1 tread*2", NULL},
    {SEC_BRINE, 'm', "VO", 0, "skimmer*2", NULL},
    {SEC_BRINE, 'n', "A", 0, "log6 tread", NULL},
    {SEC_BRINE, 'o', "VV", 0, "armor@1f tread skip", NULL},
    {SEC_BRINE, 'B', "AASS", RF_ARENA | RF_ACID, "boss2", "THE SETTLING TANK"},
    {SEC_BRINE, 'p', "A", 0, "pod tread", NULL},
    {SEC_BRINE, 'q', "NV", RF_SPIKES, "armor@0m skimmer@1", NULL},
    {SEC_BRINE, 'r', "D", 0, "treadr", NULL},
    {SEC_BRINE, 's', "Q", 0, "armor@0f spitter", NULL},
    {SEC_BRINE, 't', "H", 0, "treadr skip", NULL},
    {SEC_BRINE, 'u', "P", 0, "skimmer tread", NULL},
    {SEC_BRINE, 'T', "A", 0, "tele1", NULL},
    {SEC_BRINE, 'v', "AA", 0, "pass1@1 tread@0", NULL},
    {SEC_BRINE, 'w', "S", RF_ACID, "skimmer", NULL},
    {SEC_BRINE, 'x', "TS", RF_ACID, "armor@1a tread@0", NULL},
    {SEC_BRINE, 'y', "F", 0, "armor@0f treadr", NULL},
    {SEC_BRINE, 'U', "A", 0, "tele2", NULL},
    /* ---- the Coil ---- */
    {SEC_COIL, 'a', "WR", 0, "eye@0 armor@1f", NULL},
    {SEC_COIL, 'b', "VV", 0, "armor@0m treadg@1 skimmer", NULL},
    {SEC_COIL, 'c', "PL", 0, "treadg spitter", NULL},
    {SEC_COIL, 'd', "AV", 0, "pod@0 skimmer@1", NULL},
    {SEC_COIL, 'e', "MN", RF_SPIKES, "skip@0", NULL},
    {SEC_COIL, 'Z', "A", 0, "damper@0f log13@0", NULL},
    {SEC_COIL, 'f', "A", 0, "tread", NULL},
    {SEC_COIL, 'g', "AV", 0, "pod@0 skimmer@1", NULL},
    {SEC_COIL, 'h', "B", 0, "log12 spitter", NULL},
    {SEC_COIL, 'i', "O", 0, "treadr", NULL},
    {SEC_COIL, 'P', "J", 0, "mod2@0f log11@0", NULL},
    {SEC_COIL, 'T', "A", 0, "tele3", NULL},
    {SEC_COIL, 'j', "VV", 0, "hive@1 armor@1e skimmer@0", NULL},
    {SEC_COIL, 'k', "AAAA", RF_ARENA, "boss4", "THE DYNAMO"},
    {SEC_COIL, 'l', "I", 0, "treadg", NULL},
    {SEC_COIL, 'm', "FG", 0, "treadr skip", NULL},
    {SEC_COIL, 'n', "VV", 0, "spitter skimmer", NULL},
    {SEC_COIL, 'o', "A", 0, "log10 tread", NULL},
    {SEC_COIL, 'p', "AA", 0, "pod@0 sentry*3@1", NULL},
    {SEC_COIL, 'q', "E", 0, "armor@0c treadg", NULL},
    {SEC_COIL, 'r', "MV", 0, "armor@1f skip@1", NULL},
    {SEC_COIL, 's', "Q", 0, "armor@0f tread", NULL},
    {SEC_COIL, 't', "VV", 0, "treadr skimmer", NULL},
    {SEC_COIL, 'u', "A", 0, "pass3 tread", NULL},
    {SEC_COIL, 'v', "A", 0, "spare@0f", NULL},
    {SEC_COIL, 'w', "VB", RF_HEAT, "armor@1f skip@0", "THE KILN"},
    {SEC_COIL, 'x', "MM", RF_SPIKES, "armor@1f", NULL},
    {SEC_COIL, 'y', "AA", RF_ARENA, "boss3", NULL},
    {SEC_COIL, 'z', "J", 0, "mod1@0f treadg", NULL},
    /* ---- the Crown ---- */
    {SEC_CROWN, 'T', "A", 0, "tele9", NULL},
    {SEC_CROWN, 'a', "A", 0, "pod log14", NULL},
    {SEC_CROWN, 'b', "T", RF_ACID, "armor@0a skimmer", NULL},
    {SEC_CROWN, 'F', "AAAA", RF_ARENA, "boss6", "THE CROWN"},
};
const int APD_NSPECS = (int)(sizeof APD_SPECS / sizeof APD_SPECS[0]);

/* ---- the world, worked out once ------------------------------------------- */
ApdRoom apd_rooms[APD_MAX_ROOMS];
int apd_nrooms;
ApdDoor apd_doors[APD_MAX_DOORS];
int apd_ndoors;
ApdItemDef apd_items[APD_MAX_ITEMS];
int apd_nitems;
int16_t apd_cell[APD_SECTORS][APD_GH][APD_GW];
int apd_npods, apd_nterms;
static int16_t room_first_pod[APD_MAX_ROOMS];
static int16_t tele_room[10][2];

static int edge_type(char c) {
    switch (c) {
    case 'o': return E_DOOR;
    case '1': return E_PASS1;
    case '2': return E_PASS2;
    case '3': return E_PASS3;
    case 'b': return E_BOSS;
    case 'n': return E_NANO;
    case 'h': return E_BARREL;
    case 'H': return E_HIDDEN;
    case 'm': return E_CRACK;
    case 'e': return E_ZAP;
    case 'z': return E_TARGET;
    default: return E_WALL;
    }
}

static char map_at(const char *const *m, int row, int col) {
    for (int r = 0; r <= row; r++)
        if (!m[r]) return ' ';
    const char *s = m[row];
    return col < (int)strlen(s) ? s[col] : ' ';
}

int apd_room_find(int sector, char id) {
    for (int i = 0; i < apd_nrooms; i++)
        if (apd_rooms[i].sector == sector && apd_rooms[i].id == id) return i;
    return -1;
}

int apd_door_at(int sector, int ax, int ay, int vertical) {
    for (int i = 0; i < apd_ndoors; i++) {
        const ApdDoor *d = &apd_doors[i];
        if (d->sector == sector && d->ax == ax && d->ay == ay && d->vertical == vertical) return i;
    }
    return -1;
}

static int spec_of(int sector, char id) {
    for (int i = 0; i < APD_NSPECS; i++)
        if (APD_SPECS[i].sector == sector && APD_SPECS[i].id == id) return i;
    return -1;
}

/* next token of a things string: returns false at the end */
static bool next_token(const char **p, char *tok, int n) {
    const char *s = *p;
    while (*s == ' ') s++;
    if (!*s) return false;
    int k = 0;
    while (*s && *s != ' ') {
        if (k < n - 1) tok[k++] = *s;
        s++;
    }
    tok[k] = 0;
    *p = s;
    return true;
}

static int item_kind_of(const char *tok, int *var) {
    *var = 0;
    if (!strncmp(tok, "armor", 5)) return IT_ARMOR;
    if (!strncmp(tok, "spare", 5)) return IT_SPARE;
    if (!strncmp(tok, "damper", 6)) return IT_DAMPER;
    if (!strncmp(tok, "mod", 3) && isdigit((unsigned char)tok[3])) { *var = tok[3] - '0'; return IT_MOD; }
    return -1;
}

void apd_world_init(void) {
    static bool done;
    if (done) return;
    done = true;
    memset(apd_cell, 0xff, sizeof apd_cell);
    apd_nrooms = apd_ndoors = apd_nitems = apd_npods = apd_nterms = 0;
    memset(tele_room, 0xff, sizeof tele_room);
    for (int s = 0; s < APD_SECTORS; s++) {
        const char *const *m = SECTOR_MAPS[s];
        /* rooms */
        for (int gy = 0; gy < APD_GH; gy++)
            for (int gx = 0; gx < APD_GW; gx++) {
                char c = map_at(m, gy * 2, gx * 2);
                if (c == '.' || c == ' ' || apd_cell[s][gy][gx] >= 0) continue;
                int w = 1, h = 1;
                while (gx + w < APD_GW && map_at(m, gy * 2, (gx + w) * 2) == c) w++;
                while (gy + h < APD_GH && map_at(m, (gy + h) * 2, gx * 2) == c) h++;
                ApdRoom *r = &apd_rooms[apd_nrooms];
                r->id = c;
                r->sector = (uint8_t)s;
                r->gx = (uint8_t)gx;
                r->gy = (uint8_t)gy;
                r->w = (uint8_t)w;
                r->h = (uint8_t)h;
                r->spec = (int16_t)spec_of(s, c);
                for (int y = 0; y < h; y++)
                    for (int x = 0; x < w; x++) apd_cell[s][gy + y][gx + x] = (int16_t)apd_nrooms;
                apd_nrooms++;
            }
        /* edges */
        for (int gy = 0; gy < APD_GH; gy++)
            for (int gx = 0; gx < APD_GW; gx++) {
                int a = apd_cell[s][gy][gx];
                if (a < 0) continue;
                if (gx + 1 < APD_GW) {
                    int b = apd_cell[s][gy][gx + 1];
                    int t = edge_type(map_at(m, gy * 2, gx * 2 + 1));
                    if (b >= 0 && b != a && t != E_WALL) {
                        ApdDoor *d = &apd_doors[apd_ndoors++];
                        d->sector = (uint8_t)s; d->type = (uint8_t)t; d->vertical = 0;
                        d->ax = (uint8_t)gx; d->ay = (uint8_t)gy; d->ra = (int16_t)a; d->rb = (int16_t)b;
                    }
                }
                if (gy + 1 < APD_GH) {
                    int b = apd_cell[s][gy + 1][gx];
                    int t = edge_type(map_at(m, gy * 2 + 1, gx * 2));
                    if (b >= 0 && b != a && t != E_WALL) {
                        ApdDoor *d = &apd_doors[apd_ndoors++];
                        d->sector = (uint8_t)s; d->type = (uint8_t)t; d->vertical = 1;
                        d->ax = (uint8_t)gx; d->ay = (uint8_t)gy; d->ra = (int16_t)a; d->rb = (int16_t)b;
                    }
                }
            }
    }
    /* items, cradles, terminals and teleporters, in the room table's order */
    for (int i = 0; i < apd_nrooms; i++) {
        ApdRoom *r = &apd_rooms[i];
        r->first_item = (int16_t)apd_nitems;
        r->nitems = 0;
        room_first_pod[i] = -1;
        if (r->spec < 0) continue;
        const char *p = APD_SPECS[r->spec].things;
        char tok[32];
        while (next_token(&p, tok, sizeof tok)) {
            int var, k = item_kind_of(tok, &var);
            if (k >= 0 && apd_nitems < APD_MAX_ITEMS) {
                apd_items[apd_nitems].kind = (uint8_t)k;
                apd_items[apd_nitems].var = (uint8_t)var;
                apd_items[apd_nitems].room = (int16_t)i;
                apd_nitems++;
                r->nitems++;
            } else if (!strncmp(tok, "pod", 3)) {
                if (room_first_pod[i] < 0) room_first_pod[i] = (int16_t)apd_npods;
                apd_npods++;
            } else if (!strncmp(tok, "log", 3) || !strncmp(tok, "pass", 4)) {
                apd_nterms++;
            } else if (!strncmp(tok, "tele", 4)) {
                int l = atoi(tok + 4);
                if (l >= 0 && l < 10) tele_room[l][tele_room[l][0] < 0 ? 0 : 1] = (int16_t)i;
            }
        }
    }
}

int apd_room_pod(int room) { return room >= 0 && room < apd_nrooms ? room_first_pod[room] : -1; }

/* the room at the other end of a teleporter link */
int apd_tele_dest(int link, int from_room);
int apd_tele_dest(int link, int from_room) {
    if (link < 0 || link >= 10) return -1;
    return tele_room[link][0] == from_room ? tele_room[link][1] : tele_room[link][0];
}

const char *apd_room_name(int room) {
    if (room < 0 || room >= apd_nrooms || apd_rooms[room].spec < 0) return "";
    const char *n = APD_SPECS[apd_rooms[room].spec].name;
    return n ? n : APD_SECTOR_NAME[apd_rooms[room].sector];
}

/* ---- building a room ------------------------------------------------------ */

static const Tpl *tpl_of(char c) {
    for (int i = 0; i < ARRAY_LEN(TPL); i++)
        if (TPL[i].id == c) return &TPL[i];
    return &TPL[0];
}

static uint8_t occ[APD_RTH][APD_RTW]; /* placing things: taken spots */
static int last_foe_tx, last_foe_ty;

static bool in_room(int tx, int ty) { return tx >= 0 && ty >= 0 && tx < ag.rw && ty < ag.rh; }
static int tile(int tx, int ty) { return in_room(tx, ty) ? ag.tile[ty][tx] : T_WALL; }
static bool open_t(int tx, int ty) {
    int t = tile(tx, ty);
    return (t == T_AIR || t == T_PROP) && !occ[ty][tx];
}

static void set_t(int tx, int ty, int t) {
    if (in_room(tx, ty)) ag.tile[ty][tx] = (uint8_t)t;
}

/* a spot in cell (cx, cy) of the given kind, as near column pref as can be */
static bool find_spot(int cx, int cy, char anchor, int pref, int *otx, int *oty) {
    int x0 = cx * APD_CW, y0 = cy * APD_CH;
    for (int k = 0; k < 32; k++) {
        int dc = (k + 1) / 2 * ((k & 1) ? 1 : -1);
        int col = pref + dc;
        if (col < 3 || col > 16) continue;
        int tx = x0 + col;
        if (anchor == 'f') {
            for (int row = 8; row >= 1; row--) {
                int ty = y0 + row;
                if (open_t(tx, ty) && tile(tx, ty + 1) == T_WALL && open_t(tx, ty - 1)) { *otx = tx; *oty = ty; return true; }
            }
        } else if (anchor == 'c') {
            for (int row = 1; row <= 8; row++) {
                int ty = y0 + row;
                if (open_t(tx, ty) && tile(tx, ty - 1) == T_WALL && open_t(tx, ty + 1)) { *otx = tx; *oty = ty; return true; }
            }
        } else if (anchor == 'a') {
            for (int row = 8; row >= 1; row--) {
                int ty = y0 + row;
                if (tile(tx, ty) == T_ACID && !occ[ty][tx]) { *otx = tx; *oty = ty; return true; }
            }
        } else { /* 'm' the air */
            for (int d = 0; d < 4; d++) {
                int row = 4 + ((d + 1) / 2) * ((d & 1) ? 1 : -1);
                int ty = y0 + row;
                if (open_t(tx, ty) && open_t(tx, ty - 1) && open_t(tx, ty + 1)) { *otx = tx; *oty = ty; return true; }
            }
        }
    }
    return false;
}

static void take(int tx, int ty) {
    for (int y = ty - 1; y <= ty + 1; y++)
        for (int x = tx - 1; x <= tx + 1; x++)
            if (in_room(x, y)) occ[y][x] = 1;
}

static int add_thing(int kind, int var, int tx, int ty, int id) {
    if (ag.nth >= APD_MAX_THINGS) return -1;
    ApdThing *t = &ag.th[ag.nth];
    memset(t, 0, sizeof *t);
    t->kind = (uint8_t)kind;
    t->var = (uint8_t)var;
    t->alive = 1;
    t->tx = (int16_t)tx;
    t->ty = (int16_t)ty;
    t->id = (int16_t)id;
    return ag.nth++;
}

static int foe_kind_of(const char *tok) {
    static const struct { const char *n; int k; } F[] = {
        {"treadr", FO_TREAD_R}, {"treadg", FO_TREAD_G}, {"tread", FO_TREAD}, {"spitter", FO_SPITTER},
        {"eye", FO_STAREYE}, {"hive", FO_HIVE}, {"skip", FO_SKIP}, {"skimmer", FO_SKIMMER},
        {"sentry", FO_SENTRY},
    };
    for (int i = 0; i < ARRAY_LEN(F); i++) {
        size_t n = strlen(F[i].n);
        if (!strncmp(tok, F[i].n, n) && (tok[n] == 0 || tok[n] == '*' || tok[n] == '@')) return F[i].k;
    }
    return 0;
}

void apd_foe_size(int kind, int *w, int *h);

static void place_foe(int kind, int cx, int cy, int idx) {
    static const int PREF[] = {12, 6, 15, 4, 9, 13};
    int pref = PREF[idx % ARRAY_LEN(PREF)];
    int w, h, tx, ty;
    apd_foe_size(kind, &w, &h);
    bool walker = kind == FO_TREAD || kind == FO_TREAD_R || kind == FO_TREAD_G || kind == FO_SKIP;
    bool mount = kind == FO_SPITTER || kind == FO_HIVE;
    char anchor = 'm';
    if (walker || mount) anchor = (idx & 1) && kind != FO_SKIP ? 'c' : 'f';
    if (!find_spot(cx, cy, anchor, pref, &tx, &ty) && !find_spot(cx, cy, anchor == 'c' ? 'f' : 'm', pref, &tx, &ty))
        return;
    take(tx, ty);
    int x = tx * APD_TILE + 8, y;
    bool up = anchor == 'c';
    if (anchor == 'f') y = (ty + 1) * APD_TILE - h / 2;
    else if (anchor == 'c') y = ty * APD_TILE + h / 2;
    else y = ty * APD_TILE + 8;
    apd_spawn_foe(kind, x * APD_FX, y * APD_FX, up);
    last_foe_tx = tx;
    last_foe_ty = ty;
}

static void carve_air(int x0, int y0, int x1, int y1) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (in_room(x, y)) ag.tile[y][x] = T_AIR;
}

static int door_tile(const ApdDoor *d, int di, bool vertical) {
    bool open = ag.run.gate_open[di] != 0;
    switch (d->type) {
    case E_PASS1: case E_PASS2: case E_PASS3: return T_LOCK;
    case E_NANO: case E_TARGET: return open ? (vertical ? T_AIR : T_DOOR) : T_SHUT;
    case E_BARREL: return open ? T_AIR : T_BARREL;
    case E_HIDDEN: return open ? T_AIR : T_FAKE;
    case E_CRACK: return open ? T_AIR : T_CRACK;
    case E_ZAP: return T_ZAP;
    default: return vertical ? T_AIR : T_DOOR;
    }
}

static void put_doors(int room) {
    const ApdRoom *r = &apd_rooms[room];
    for (int di = 0; di < apd_ndoors; di++) {
        const ApdDoor *d = &apd_doors[di];
        if (d->ra != room && d->rb != room) continue;
        bool is_a = d->ra == room;
        int gx = d->ax + (!is_a && !d->vertical ? 1 : 0);
        int gy = d->ay + (!is_a && d->vertical ? 1 : 0);
        int lx = (gx - r->gx) * APD_CW, ly = (gy - r->gy) * APD_CH;
        int t = door_tile(d, di, d->vertical);
        if (!d->vertical) {
            int bx = is_a ? lx + APD_CW - 1 : lx;      /* the border column */
            int sx = is_a ? bx - 1 : bx + 1;           /* the stub beside it */
            int ix = is_a ? bx - 2 : bx + 2;
            for (int row = 1; row <= 8; row++) set_t(sx, ly + row, (row >= 4 && row <= 5) ? T_AIR : T_WALL);
            set_t(ix, ly + 4, T_AIR);
            set_t(ix, ly + 5, T_AIR);
            for (int row = 4; row <= 5; row++) {
                set_t(bx, ly + row, t);
                ag.tdoor[ly + row][bx] = (uint8_t)(di + 1);
            }
            if (d->type == E_TARGET && !ag.run.gate_open[di]) add_thing(TH_TARGET, 0, sx, ly + 2, di);
            if (d->type == E_ZAP) {
                add_thing(TH_EMITTER, 0, bx, ly + 3, di);
                add_thing(TH_EMITTER, 1, bx, ly + 6, di);
            }
        } else {
            int sy = is_a ? ly + APD_CH - 1 : ly;      /* the slab row */
            if (is_a) carve_air(lx + 8, ly + 7, lx + 11, ly + 8);
            else carve_air(lx + 8, ly + 1, lx + 11, ly + 2);
            for (int col = 9; col <= 10; col++) {
                set_t(lx + col, sy, t);
                ag.tdoor[sy][lx + col] = (uint8_t)(di + 1);
            }
            if (d->type == E_ZAP) {
                add_thing(TH_EMITTER, 2, lx + 8, sy, di);
                add_thing(TH_EMITTER, 3, lx + 11, sy, di);
            }
        }
    }
}

int apd_room_cell_x(int px) { return iclamp(px / APD_CPW, 0, apd_rooms[ag.room].w - 1); }
int apd_room_cell_y(int py) { return iclamp(py / APD_CPH, 0, apd_rooms[ag.room].h - 1); }

void apd_build_room(int room) {
    const ApdRoom *r = &apd_rooms[room];
    const ApdSpec *sp = r->spec >= 0 ? &APD_SPECS[r->spec] : NULL;
    ag.room = room;
    ag.rw = r->w * APD_CW;
    ag.rh = r->h * APD_CH;
    memset(ag.tile, T_WALL, sizeof ag.tile);
    memset(ag.tdoor, 0, sizeof ag.tdoor);
    memset(occ, 0, sizeof occ);
    memset(ag.foe, 0, sizeof ag.foe);
    memset(ag.es, 0, sizeof ag.es);
    memset(ag.shot, 0, sizeof ag.shot);
    memset(ag.th, 0, sizeof ag.th);
    memset(ag.part, 0, sizeof ag.part);
    ag.nth = 0;
    ag.boss_on = false;
    ag.boss_lock = false;
    ag.boss_foe = -1;
    ag.boss = -1;
    ag.lightning_t = 0;
    ag.button_t[0] = ag.button_t[1] = 0;
    ag.shadow.on = false;
    ag.drone.latch = -1;
    /* the cells */
    for (int cy = 0; cy < r->h; cy++)
        for (int cx = 0; cx < r->w; cx++) {
            char tc = sp && sp->tpl && (int)strlen(sp->tpl) > cy * r->w + cx ? sp->tpl[cy * r->w + cx] : 'A';
            const Tpl *tp = tpl_of(tc);
            int x0 = cx * APD_CW, y0 = cy * APD_CH;
            for (int row = 0; row < 8; row++)
                for (int col = 0; col < 18; col++) {
                    char c = tp->row[row][col];
                    int tx = x0 + 1 + col, ty = y0 + 1 + row, t = T_AIR;
                    switch (c) {
                    case '#': t = T_WALL; break;
                    case '^': t = T_SPUP; break;
                    case 'v': t = T_SPDN; break;
                    case '=': t = T_PLAT; break;
                    case 'a': t = T_ACID; break;
                    case 'G': t = T_GREY; break;
                    case '%': t = T_PROP; break;
                    case 'r': add_thing(TH_BALL, 0, tx, ty, -1); break;
                    case 'p': add_thing(TH_BALL, 1, tx, ty, -1); break;
                    case 'b': add_thing(TH_BUTTON, (uint8_t)(ag.nth > 0 && ag.th[ag.nth - 1].kind == TH_BUTTON), tx, ty, -1); break;
                    default: break;
                    }
                    ag.tile[ty][tx] = (uint8_t)t;
                }
        }
    /* joins to the same room (after every cell is drawn) */
    for (int cy = 0; cy < r->h; cy++)
        for (int cx = 0; cx < r->w; cx++) {
            int x0 = cx * APD_CW, y0 = cy * APD_CH;
            if (cx + 1 < r->w) {
                for (int row = 1; row <= 8; row++) { set_t(x0 + 19, y0 + row, T_AIR); set_t(x0 + 20, y0 + row, T_AIR); }
            }
            if (cy + 1 < r->h) {
                /* a floor between two cells of one room: a wide hole, ledges each side
                 * (an arena is left wide open) */
                bool arena = sp && (sp->flags & RF_ARENA);
                for (int col = arena ? 1 : 6; col <= (arena ? 18 : 13); col++) {
                    set_t(x0 + col, y0 + 9, T_AIR);
                    set_t(x0 + col, y0 + 10, T_AIR);
                    /* nothing seals the hole from either side */
                    if (!arena) { set_t(x0 + col, y0 + 8, T_AIR); set_t(x0 + col, y0 + 11, T_AIR); }
                }
                if (cx + 1 < r->w && arena) {
                    set_t(x0 + 19, y0 + 9, T_AIR); set_t(x0 + 20, y0 + 9, T_AIR);
                    set_t(x0 + 19, y0 + 10, T_AIR); set_t(x0 + 20, y0 + 10, T_AIR);
                }
            }
        }
    /* the grey wall of the hidden room stays open once its buttons have been hit */
    if (ag.run.secret_open)
        for (int y = 0; y < ag.rh; y++)
            for (int x = 0; x < ag.rw; x++)
                if (ag.tile[y][x] == T_GREY) ag.tile[y][x] = T_AIR;
    put_doors(room);
    /* the doors' corridors and the shafts are no place for things */
    for (int y = 0; y < ag.rh; y++)
        for (int x = 0; x < ag.rw; x++)
            if (ag.tdoor[y][x]) take(x, y);
    if (!sp) return;
    /* things */
    const char *p = sp->things;
    char tok[32];
    int item_k = 0, pod_k = 0, foe_idx = 0;
    while (next_token(&p, tok, sizeof tok)) {
        int cell = 0;
        char anchor = 0;
        const char *at = strchr(tok, '@');
        if (at) {
            cell = at[1] - '0';
            anchor = at[2];
        }
        int cx = cell % r->w, cy = cell / r->w;
        if (cy >= r->h) cy = r->h - 1;
        int count = 1;
        const char *star = strchr(tok, '*');
        if (star) count = atoi(star + 1);
        int var, ik = item_kind_of(tok, &var);
        int tx = 0, ty = 0;
        if (ik >= 0) {
            int item = r->first_item + item_k++;
            if (!anchor) anchor = 'f';
            bool ok;
            if (anchor == 'e') { tx = last_foe_tx; ty = last_foe_ty; ok = true; }
            else if (anchor == 't' || anchor == 'h') {
                ok = false;
                const Tpl *tp = tpl_of(sp->tpl[cell]);
                for (int row = 0; row < 8 && !ok; row++)
                    for (int col = 0; col < 18 && !ok; col++)
                        if (tp->row[row][col] == '*') { tx = cx * APD_CW + 1 + col; ty = cy * APD_CH + 1 + row; ok = true; }
            } else ok = find_spot(cx, cy, anchor, 10, &tx, &ty);
            if (ok && !ag.run.item_got[item]) {
                take(tx, ty);
                int th = add_thing(TH_ITEM, (uint8_t)(anchor == 'h' || anchor == 'e' || anchor == 'a'), tx, ty, item);
                if (anchor == 'h') set_t(tx + 1, ty, T_PROP);
                (void)th;
            }
            continue;
        }
        if (!strncmp(tok, "pod", 3)) {
            if (find_spot(cx, cy, 'f', 10, &tx, &ty)) {
                take(tx, ty);
                take(tx + 1, ty);
                add_thing(TH_POD, 0, tx, ty, room_first_pod[room] + pod_k);
            }
            pod_k++;
        } else if (!strncmp(tok, "log", 3) || !strncmp(tok, "pass", 4)) {
            bool pass = tok[0] == 'p';
            int n = atoi(tok + (pass ? 4 : 3));
            if (find_spot(cx, cy, 'f', pass ? 9 : 7, &tx, &ty)) {
                take(tx, ty);
                add_thing(TH_TERM, (uint8_t)(pass ? n : 0), tx, ty, pass ? -1 : n);
            }
        } else if (!strncmp(tok, "tele", 4)) {
            if (find_spot(cx, cy, 'f', 10, &tx, &ty)) {
                take(tx, ty);
                take(tx + 1, ty);
                add_thing(TH_TELE, 0, tx, ty, atoi(tok + 4));
            }
        } else if (!strncmp(tok, "start", 5)) {
            /* to the right of the first terminal: its scratched code shows only
             * to someone who comes at it from the left */
            if (find_spot(cx, cy, 'f', 13, &tx, &ty)) {
                ag.pl.entry_x = (tx * APD_TILE + 8) * APD_FX;
                ag.pl.entry_y = ((ty + 1) * APD_TILE - APD_PH / 2) * APD_FX;
                ag.pl.entry_grav = 1;
                ag.pl.entry_face = 1;
                take(tx, ty);
            }
        } else if (!strncmp(tok, "boss", 4)) {
            ag.boss = atoi(tok + 4);
        } else if (!strcmp(tok, "buttons")) {
            /* placed by the template */
        } else {
            int k = foe_kind_of(tok);
            if (k && !ag.no_foes)
                for (int i = 0; i < count; i++) {
                    int c = at ? cell : i % (r->w * r->h);
                    place_foe(k, c % r->w, c / r->w, foe_idx++);
                }
        }
    }
}

/* ---- tiles and collision ------------------------------------------------- */

int apd_tile_at(int px, int py) {
    if (px < 0 || py < 0) return T_WALL;
    int tx = px / APD_TILE, ty = py / APD_TILE;
    return tile(tx, ty);
}

static bool tile_solid(int t) {
    return t == T_WALL || t == T_SPUP || t == T_SPDN || t == T_CRACK || t == T_BARREL || t == T_DOOR ||
           t == T_LOCK || t == T_SHUT || t == T_FAKE || t == T_GREY;
}

bool apd_solid_tile(int tx, int ty, int grav, int dir_y) {
    (void)grav;
    (void)dir_y;
    if (tx < 0 || tx >= ag.rw) {
        /* off the side of the room: only a door's opening leads on */
        return !(ty >= 0 && ty < ag.rh);
    }
    if (ty < 0 || ty >= ag.rh) return !(tx >= 0 && tx < ag.rw);
    return tile_solid(ag.tile[ty][tx]);
}

/* Is the box (centre cx, cy, size w x h, in pixels) in something solid?
 * One-way plates count only when the box comes onto them along gravity:
 * vy is the movement's direction (0: plates don't count). Outside the room
 * only the door openings are free. */
bool apd_box_solid(int cx, int cy, int w, int h, int grav, int vy) {
    int x0 = cx - w / 2, x1 = cx - w / 2 + w - 1;
    int y0 = cy - h / 2, y1 = cy - h / 2 + h - 1;
    int tx0 = (x0 >= 0 ? x0 : x0 - 15) / APD_TILE, tx1 = (x1 >= 0 ? x1 : x1 - 15) / APD_TILE;
    int ty0 = (y0 >= 0 ? y0 : y0 - 15) / APD_TILE, ty1 = (y1 >= 0 ? y1 : y1 - 15) / APD_TILE;
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            int t;
            if (tx < 0 || tx >= ag.rw || ty < 0 || ty >= ag.rh) {
                /* beyond the border: free only straight out of an open doorway */
                int bx = iclamp(tx, 0, ag.rw - 1), by = iclamp(ty, 0, ag.rh - 1);
                t = ag.tile[by][bx];
                if (tile_solid(t) || !ag.tdoor[by][bx]) return true;
                continue;
            }
            t = ag.tile[ty][tx];
            if (tile_solid(t)) return true;
            if (t == T_PLAT && vy != 0 && isign(vy) == grav) {
                /* a plate: solid only at its face the feet come down onto */
                int edge = grav > 0 ? ty * APD_TILE : ty * APD_TILE + APD_TILE - 1;
                int feet = grav > 0 ? y1 : y0;
                if (grav > 0 ? (feet - vy <= edge && feet >= edge) : (feet - vy >= edge && feet <= edge)) return true;
            }
        }
    return false;
}

/* ---- gates ------------------------------------------------------------------ */

void apd_open_gate(int door) {
    if (door < 0 || door >= apd_ndoors) return;
    ag.run.gate_open[door] = 1;
    for (int y = 0; y < ag.rh; y++)
        for (int x = 0; x < ag.rw; x++)
            if (ag.tdoor[y][x] == door + 1) {
                int t = ag.tile[y][x];
                if (t != T_ZAP) ag.tile[y][x] = T_AIR;
            }
    for (int i = 0; i < ag.nth; i++)
        if (ag.th[i].kind == TH_TARGET && ag.th[i].id == door) ag.th[i].alive = 0;
}

/* a fight shuts every way out of the arena (and opens them again after) */
void apd_lock_boss_doors(bool lock) {
    ag.boss_lock = lock;
    for (int y = 0; y < ag.rh; y++)
        for (int x = 0; x < ag.rw; x++) {
            int d = ag.tdoor[y][x];
            if (!d) continue;
            const ApdDoor *dd = &apd_doors[d - 1];
            if (lock) ag.tile[y][x] = T_SHUT;
            else ag.tile[y][x] = (uint8_t)door_tile(dd, d - 1, dd->vertical);
        }
}

/* every sentry orb gone: the room's nano-walls open for good */
void apd_open_nano(void) {
    for (int di = 0; di < apd_ndoors; di++) {
        const ApdDoor *d = &apd_doors[di];
        if (d->type == E_NANO && (d->ra == ag.room || d->rb == ag.room) && !ag.run.gate_open[di]) {
            apd_open_gate(di);
            for (int y = 0; y < ag.rh; y++)
                for (int x = 0; x < ag.rw; x++)
                    if (ag.tdoor[y][x] == di + 1 && !d->vertical) ag.tile[y][x] = T_DOOR;
        }
    }
}

/* tests: the room as text */
void apd_room_dump(int room) {
    static const char CH[] = ".#^v=a%BDLSpFzG";
    for (int y = 0; y < ag.rh; y++) {
        char line[APD_RTW + 1];
        for (int x = 0; x < ag.rw; x++) {
            int t = ag.tile[y][x];
            char c = t == T_CRACK ? 'X' : t == T_PROP ? '%' : CH[t < (int)sizeof CH - 1 ? t : 0];
            for (int i = 0; i < ag.nth; i++)
                if (ag.th[i].alive && ag.th[i].tx == x && ag.th[i].ty == y) c = "PTt*obB!"[ag.th[i].kind];
            for (int i = 0; i < APD_MAX_FOES; i++)
                if (ag.foe[i].alive && APD_PX(ag.foe[i].x) / APD_TILE == x && APD_PX(ag.foe[i].y) / APD_TILE == y) c = 'e';
            line[x] = c;
        }
        line[ag.rw] = 0;
        printf("%s\n", line);
    }
    (void)room;
}

/* tests: every room's things found a place (items, cradles, terminals,
 * teleporters, foes), with nothing collected */
int apd_place_check(void);
int apd_place_check(void) {
    ApdGame keep = ag;
    int bad = 0;
    memset(&ag.run, 0, sizeof ag.run);
    for (int r = 0; r < apd_nrooms; r++) {
        apd_build_room(r);
        const ApdSpec *sp = &APD_SPECS[apd_rooms[r].spec];
        int want[4] = {0, 0, 0, 0}, got[4] = {0, 0, 0, 0}, foes = 0, nfoes = 0;
        const char *p = sp->things;
        char tok[32];
        while (next_token(&p, tok, sizeof tok)) {
            int var;
            if (item_kind_of(tok, &var) >= 0) want[0]++;
            else if (!strncmp(tok, "pod", 3)) want[1]++;
            else if (!strncmp(tok, "log", 3) || !strncmp(tok, "pass", 4)) want[2]++;
            else if (!strncmp(tok, "tele", 4)) want[3]++;
            else if (foe_kind_of(tok)) {
                const char *st = strchr(tok, '*');
                foes += st ? atoi(st + 1) : 1;
            }
        }
        for (int i = 0; i < ag.nth; i++) {
            int k = ag.th[i].kind;
            if (k == TH_ITEM) got[0]++;
            if (k == TH_POD) got[1]++;
            if (k == TH_TERM) got[2]++;
            if (k == TH_TELE) got[3]++;
        }
        nfoes = apd_foes_alive();
        if (memcmp(want, got, sizeof want) || foes != nfoes) {
            bad++;
            fprintf(stderr, "room %d %c: items %d/%d pods %d/%d terms %d/%d teles %d/%d foes %d/%d\n", apd_rooms[r].sector,
                    apd_rooms[r].id, got[0], want[0], got[1], want[1], got[2], want[2], got[3], want[3], nfoes, foes);
        }
    }
    ag = keep;
    return bad;
}
