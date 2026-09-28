/* LOST LINKS - reading the things out of the hand-drawn world: pins, irons,
 * scorecrows, strays, folk to talk to, birds, slicers, the four abilities,
 * the Star Pin's pieces, the altar and the sanctum. Each is a letter in the
 * map; the ground under it is whatever lies around it. */
#include "lostlinks.h"

LnkThing lnk_things[LNK_MAX_THINGS];
int lnk_nthings;

static int kind_of(char ch, int *id) {
    *id = -1;
    switch (ch) {
    case 'W': return EK_START;
    case 'F': return EK_PIN;
    case 'I': return EK_IRON;
    case 'C': return EK_CROW;
    case 'S': return EK_STRAY;
    case 'N': return EK_NPC;
    case 'L': return EK_LARK;
    case 'E': return EK_ALBA;
    case 'B': return EK_SLICER;
    case 'D': return EK_SIPPER;
    case 'K': return EK_KEEPER;
    case 'A': return EK_SIGN;
    case 'h': *id = AB_HAMMER; return EK_ABILITY;
    case 'j': *id = AB_BACKSPIN; return EK_ABILITY;
    case 'd': *id = AB_TREAD; return EK_ABILITY;
    case 'q': *id = AB_SKIPPER; return EK_ABILITY;
    case 'Q': return EK_PIECE;
    case 'Z': return EK_ALTAR;
    case 'P': return EK_PLATE;
    case 'M': return EK_TREE;
    case 'U': return EK_SAUCER;
    case 'G': return EK_DEN;
    default: return -1;
    }
}

bool lnk_is_marker(char ch) {
    int id;
    return kind_of(ch, &id) >= 0;
}

void lnk_world_scan(void) {
    if (lnk_nthings) return;
    int count[EK_KINDS] = {0};
    for (int l = 0; l < LNK_LAYERS; l++)
        for (int y = 0; y < LNK_MH; y++)
            for (int x = 0; x < LNK_MW; x++) {
                int id, k = kind_of(LNK_MAP[l][y][x], &id);
                if (k < 0 || lnk_nthings >= LNK_MAX_THINGS) continue;
                LnkThing *t = &lnk_things[lnk_nthings++];
                t->kind = (uint8_t)k;
                t->layer = (uint8_t)l;
                t->id = (uint8_t)(id >= 0 ? id : count[k]++);
                t->tx = (int16_t)x;
                t->ty = (int16_t)y;
            }
}

const LnkThing *lnk_find(int kind, int id) {
    lnk_world_scan();
    for (int i = 0; i < lnk_nthings; i++)
        if (lnk_things[i].kind == kind && lnk_things[i].id == id) return &lnk_things[i];
    return NULL;
}

int lnk_count(int kind) {
    lnk_world_scan();
    int n = 0;
    for (int i = 0; i < lnk_nthings; i++) n += lnk_things[i].kind == kind;
    return n;
}

/* the ground under a thing: the commonest ground next to it */
static char ground_under(int l, int x, int y) {
    static const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
    char best = '.';
    int best_n = 0;
    for (int i = 0; i < 4; i++) {
        int nx = x + DX[i], ny = y + DY[i];
        if (nx < 0 || ny < 0 || nx >= LNK_MW || ny >= LNK_MH) continue;
        char c = LNK_MAP[l][ny][nx];
        if (lnk_is_marker(c) || lnk_solid_char(c) || c == 'p' || c == 'y' || c == 'J' || c == '%' || c == 'o' || c == 'O' ||
            c == 'g' || c == 'k')
            continue;
        if (c >= '1' && c <= '9') continue;
        int n = 0;
        for (int j = 0; j < 4; j++) {
            int mx = x + DX[j], my = y + DY[j];
            if (mx >= 0 && my >= 0 && mx < LNK_MW && my < LNK_MH && LNK_MAP[l][my][mx] == c) n++;
        }
        if (n > best_n) { best_n = n; best = c; }
    }
    if (best == '~') best = ','; /* a thing never stands in the water */
    return best;
}

void lnk_tiles_reset(uint8_t (*t)[LNK_MH][LNK_MW]) {
    for (int l = 0; l < LNK_LAYERS; l++)
        for (int y = 0; y < LNK_MH; y++)
            for (int x = 0; x < LNK_MW; x++) {
                char c = LNK_MAP[l][y][x];
                t[l][y][x] = (uint8_t)(lnk_is_marker(c) ? ground_under(l, x, y) : c);
            }
}
