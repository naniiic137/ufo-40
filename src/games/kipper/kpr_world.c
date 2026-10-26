/* KIPPER - the sea's tiles: what is solid, the breakables (blue blocks,
 * cracked and sealed walls), dirt, the switch gates and blasts. */
#include "kpr.h"

static int16_t brk_of[KPR_H][KPR_W];
static int16_t brk_x[KPR_MAX_BRK], brk_y[KPR_MAX_BRK];
static int nbrk;
static int8_t item_of_tile_ready;
static int16_t grp_of[KPR_MAX_BRK];      /* a big blue block (or a sealed wall) breaks as one */
static int16_t item_tile[KPR_H][KPR_W];

void kpr_world_init(void) {
    if (nbrk) return;
    for (int y = 0; y < KPR_H; y++)
        for (int x = 0; x < KPR_W; x++) {
            char c = KPR_ROWS[y][x];
            brk_of[y][x] = -1;
            if ((c == 'B' || c == '?' || c == 'X') && nbrk < KPR_MAX_BRK) {
                brk_of[y][x] = (int16_t)nbrk;
                brk_x[nbrk] = (int16_t)x;
                brk_y[nbrk] = (int16_t)y;
                nbrk++;
            }
        }
    /* blue blocks and sealed walls touching each other make one piece */
    for (int i = 0; i < nbrk; i++) grp_of[i] = (int16_t)i;
    for (int pass = 0; pass < 8; pass++)
        for (int i = 0; i < nbrk; i++) {
            char c = KPR_ROWS[brk_y[i]][brk_x[i]];
            if (c == '?') continue;
            static const int D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (int k = 0; k < 4; k++) {
                int j = kpr_brk_index(brk_x[i] + D[k][0], brk_y[i] + D[k][1]);
                if (j >= 0 && KPR_ROWS[brk_y[j]][brk_x[j]] == c && grp_of[j] < grp_of[i]) grp_of[i] = grp_of[j];
            }
        }
    for (int y = 0; y < KPR_H; y++)
        for (int x = 0; x < KPR_W; x++) item_tile[y][x] = -1;
    for (int i = 0; i < KPR_NITEMS; i++) item_tile[KPR_ITEMS[i].y][KPR_ITEMS[i].x] = (int16_t)i;
    item_of_tile_ready = 1;
}

int kpr_nbrk(void) { return nbrk; }
int kpr_brk_index(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= KPR_W || ty >= KPR_H) return -1;
    return brk_of[ty][tx];
}
int kpr_item_at_tile(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= KPR_W || ty >= KPR_H || !item_of_tile_ready) return -1;
    return item_tile[ty][tx];
}

bool kpr_brk_banked(int i) { return (kg.cur.broken[i >> 3] >> (i & 7)) & 1; }

/* the sea as the dive starts: the drawn rows, minus what blasts have opened
 * for good; dirt is back and the gates are shut */
void kpr_build_tiles(void) {
    for (int y = 0; y < KPR_H; y++)
        for (int x = 0; x < KPR_W; x++) {
            uint8_t c = (uint8_t)KPR_ROWS[y][x];
            int b = brk_of[y][x];
            if (b >= 0 && kpr_brk_banked(b)) c = '.';
            kg.tile[y][x] = c;
        }
    kg.gates = 0;
    memset(kg.crack, 0, sizeof kg.crack);
}

int kpr_tile(int tx, int ty) {
    if (tx < 0 || tx >= KPR_W || ty >= KPR_H) return '#';
    if (ty < 0) return '^';
    return kg.tile[ty][tx];
}

bool kpr_solid_tile(int tx, int ty) {
    int c = kpr_tile(tx, ty);
    switch (c) {
    case '.': case 'k': case 'c': return false;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
        return !((kg.gates >> (c - '0')) & 1);
    default: return true;
    }
}

bool kpr_box_solid(float cx, float cy, int w, int h) {
    int x0 = (int)floorf((cx - w / 2.0f) / KPR_TILE), x1 = (int)floorf((cx + w / 2.0f - 0.01f) / KPR_TILE);
    int y0 = (int)floorf((cy - h / 2.0f) / KPR_TILE), y1 = (int)floorf((cy + h / 2.0f - 0.01f) / KPR_TILE);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (kpr_solid_tile(x, y)) return true;
    return false;
}

bool kpr_shot_solid(float x, float y) {
    return kpr_solid_tile((int)floorf(x / KPR_TILE), (int)floorf(y / KPR_TILE));
}

int kpr_area_of(float y) {
    int ty = (int)(y / KPR_TILE);
    return ty < KPR_AREA2 ? 0 : ty < KPR_AREA3 ? 1 : 2;
}

/* a breakable or a dirt tile opens (for this dive; a dock banks blasts) */
void kpr_break_tile(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= KPR_W || ty >= KPR_H) return;
    int c = kg.tile[ty][tx];
    if (c != 'B' && c != '?' && c != 'X' && c != 'd') return;
    kg.tile[ty][tx] = '.';
    int b = brk_of[ty][tx];
    if (b >= 0) kg.cur.broken[b >> 3] |= (uint8_t)(1 << (b & 7));
    float px = tx * KPR_TILE + 8.0f, py = ty * KPR_TILE + 8.0f;
    int col = c == 'B' ? C_SKY : c == 'd' ? C_BROWN : C_GREY;
    kpr_burst(px, py, col, 6, 1.2f);
}

/* a missile's or a depth charge's blast: blue blocks, cracked and sealed
 * walls within reach break */
void kpr_blast(float x, float y, int radius) {
    int r = radius + 8;
    int x0 = (int)((x - r) / KPR_TILE), x1 = (int)((x + r) / KPR_TILE);
    int y0 = (int)((y - r) / KPR_TILE), y1 = (int)((y + r) / KPR_TILE);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++) {
            int c = kpr_tile(tx, ty);
            if (c != 'B' && c != '?' && c != 'X') continue;
            float dx = tx * KPR_TILE + 8.0f - x, dy = ty * KPR_TILE + 8.0f - y;
            if (dx * dx + dy * dy > (float)(r * r)) continue;
            int b = kpr_brk_index(tx, ty);
            if (c == '?' || b < 0) { kpr_break_tile(tx, ty); continue; }
            /* the whole block goes */
            for (int j = 0; j < nbrk; j++)
                if (grp_of[j] == grp_of[b]) kpr_break_tile(brk_x[j], brk_y[j]);
        }
}
