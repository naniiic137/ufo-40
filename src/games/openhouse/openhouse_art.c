/* OPEN HOUSE - guest portraits (16 x 16) and props. The people are painted
 * from a few parts (face, hair, hat, clothes, a prop) so every guest has a
 * look of their own; the creatures and stars are drawn by hand below. */
#include "openhouse.h"

Sprite ph_spr[PS_COUNT];

/* ------------------------------------------------------------------ */
/* the painter                                                          */

static char px[16][17];

static void clear(void) {
    for (int y = 0; y < 16; y++) { memset(px[y], '.', 16); px[y][16] = 0; }
}
static void put(int x, int y, char c) {
    if (x >= 0 && y >= 0 && x < 16 && y < 16) px[y][x] = c;
}
static void hline(int x0, int x1, int y, char c) { for (int x = x0; x <= x1; x++) put(x, y, c); }
static void box(int x0, int y0, int x1, int y1, char c) { for (int y = y0; y <= y1; y++) hline(x0, x1, y, c); }

/* a dark outline round everything painted */
static void outline(void) {
    char o[16][17];
    memcpy(o, px, sizeof o);
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            if (o[y][x] != '.') continue;
            static const int8_t d[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (int k = 0; k < 4; k++) {
                int nx = x + d[k][0], ny = y + d[k][1];
                if (nx >= 0 && ny >= 0 && nx < 16 && ny < 16 && o[ny][nx] != '.' && o[ny][nx] != 'k') { px[y][x] = 'k'; break; }
            }
        }
}

enum { H_BALD, H_SHORT, H_LONG, H_BUN, H_CURLY, H_SPIKY, H_SLICK, H_SCARF, H_WILD, H_TAIL };
enum { K_NONE, K_FEZ, K_CAP, K_CHEF, K_TURBAN, K_BERET, K_SUNHAT, K_TOPHAT, K_BAND, K_VISOR, K_FLOWER };
enum { F_NONE = 0, F_GLASSES = 1, F_SHADES = 2, F_MOUSTACHE = 4, F_BEARD = 8, F_EARRINGS = 16, F_PATCH = 32,
       F_STARS = 64, F_GRIN = 128, F_WHITEBEARD = 256, F_BLUSH = 512 };
enum { C_PLAIN, C_STRIPES, C_COLLAR, C_BOWTIE, C_TIE, C_APRON, C_SUIT, C_VEST, C_PEARLS, C_BELT, C_FLOWERS, C_SASH };
enum { P_NONE, P_CAMERA, P_BOOK, P_MIC, P_TAPE, P_WHISTLE, P_DRUM, P_SPARKS, P_BOX, P_CARDS, P_HEART, P_MOON, P_KEYS, P_BADGE, P_CUP };

typedef struct Person {
    char skin, shade, hair, hair_d;
    uint8_t hair_style, hat;
    char hat_col;
    uint16_t face;
    uint8_t cloth_style;
    char cloth, cloth2;
    uint8_t prop;
} Person;

static void paint_person(const Person *p) {
    clear();
    /* shoulders and chest */
    hline(4, 11, 12, p->cloth);
    hline(3, 12, 13, p->cloth);
    box(2, 14, 13, 15, p->cloth);
    switch (p->cloth_style) {
    case C_STRIPES: hline(3, 12, 13, p->cloth2); hline(2, 13, 15, p->cloth2); break;
    case C_COLLAR: put(6, 12, 'w'); put(9, 12, 'w'); put(7, 13, 'w'); put(8, 13, 'w'); break;
    case C_BOWTIE: put(6, 12, p->cloth2); put(7, 12, 'k'); put(8, 12, 'k'); put(9, 12, p->cloth2); break;
    case C_TIE: box(7, 12, 8, 12, 'w'); box(7, 13, 8, 15, p->cloth2); break;
    case C_APRON: box(5, 13, 10, 15, p->cloth2); put(5, 12, p->cloth2); put(10, 12, p->cloth2); break;
    case C_SUIT: box(6, 12, 9, 15, 'w'); box(7, 13, 8, 15, p->cloth2); put(7, 12, p->cloth2); put(8, 12, p->cloth2); break;
    case C_VEST: box(3, 13, 5, 15, p->cloth2); box(10, 13, 12, 15, p->cloth2); break;
    case C_PEARLS: hline(5, 10, 12, 'w'); put(4, 13, 'w'); put(11, 13, 'w'); break;
    case C_BELT: hline(2, 13, 15, 'y'); put(7, 15, 'a'); put(8, 15, 'a'); break;
    case C_FLOWERS: put(4, 14, p->cloth2); put(7, 13, p->cloth2); put(10, 15, p->cloth2); put(12, 14, p->cloth2); put(3, 15, 'y'); put(9, 14, 'y'); break;
    case C_SASH: for (int i = 0; i < 4; i++) put(4 + i * 2, 12 + i, p->cloth2), put(5 + i * 2, 12 + i, p->cloth2); break;
    default: break;
    }
    /* neck and head */
    box(7, 10, 8, 11, p->shade);
    hline(6, 9, 2, p->skin);
    hline(5, 10, 3, p->skin);
    box(4, 4, 11, 8, p->skin);
    hline(5, 10, 9, p->skin);
    hline(6, 9, 10, p->skin);
    put(4, 8, p->shade); put(11, 8, p->shade); hline(5, 10, 9, p->skin); put(5, 9, p->shade); put(10, 9, p->shade);
    /* ears */
    put(3, 6, p->skin); put(12, 6, p->skin);
    /* hair */
    char h = p->hair, hd = p->hair_d;
    switch (p->hair_style) {
    case H_SHORT: hline(6, 9, 1, h); hline(4, 11, 2, h); hline(4, 11, 3, h); put(4, 4, h); put(11, 4, h); put(5, 3, hd); break;
    case H_LONG: hline(6, 9, 1, h); hline(4, 11, 2, h); hline(3, 12, 3, h); for (int y = 4; y <= 10; y++) { put(3, y, h); put(12, y, h); } put(4, 4, h); put(11, 4, h); put(3, 10, hd); put(12, 10, hd); break;
    case H_BUN: box(6, 0, 9, 1, h); hline(5, 10, 2, h); hline(4, 11, 3, h); put(4, 4, h); put(11, 4, h); put(7, 0, hd); break;
    case H_CURLY: hline(5, 10, 0, h); hline(3, 12, 1, h); box(2, 2, 13, 3, h); for (int y = 4; y <= 7; y++) { put(3, y, h); put(12, y, h); put(2, y, hd); put(13, y, hd); } put(4, 1, hd); put(9, 2, hd); break;
    case H_SPIKY: put(5, 0, h); put(8, 0, h); put(11, 1, h); hline(4, 11, 1, h); hline(4, 11, 2, h); hline(4, 11, 3, h); put(4, 4, h); put(11, 4, h); put(7, 1, hd); break;
    case H_SLICK: hline(5, 10, 1, h); hline(4, 11, 2, h); hline(4, 11, 3, h); put(4, 4, h); put(11, 4, h); hline(5, 7, 2, 'w'); break;
    case H_SCARF: hline(5, 10, 1, h); box(3, 2, 12, 3, h); for (int y = 4; y <= 11; y++) { put(3, y, h); put(12, y, h); } put(4, 10, h); put(11, 10, h); hline(4, 11, 3, hd); break;
    case H_WILD: put(3, 0, h); put(7, 0, h); put(12, 0, h); hline(3, 12, 1, h); box(2, 2, 13, 3, h); for (int y = 4; y <= 11; y++) { put(2 + (y & 1), y, h); put(13 - (y & 1), y, h); } put(6, 1, hd); put(10, 2, hd); break;
    case H_TAIL: hline(6, 9, 1, h); hline(4, 11, 2, h); hline(4, 11, 3, h); put(4, 4, h); put(12, 3, h); put(13, 4, h); put(13, 5, h); put(13, 6, hd); break;
    default: put(6, 3, 'w'); break; /* a bald head's shine */
    }
    /* face */
    uint16_t f = p->face;
    put(6, 6, 'k'); put(9, 6, 'k');
    if (f & F_BLUSH) { put(5, 7, 'K'); put(10, 7, 'K'); }
    if (f & F_GRIN) { hline(6, 9, 8, 'k'); put(7, 8, 'w'); put(8, 8, 'w'); }
    else { put(7, 8, 'v'); put(8, 8, 'v'); }
    if (f & F_GLASSES) { hline(5, 7, 5, 'k'); hline(8, 10, 5, 'k'); put(5, 6, 'k'); put(7, 6, 'k'); put(8, 6, 'k'); put(10, 6, 'k'); put(6, 6, 'C'); put(9, 6, 'C'); }
    if (f & F_SHADES) { hline(5, 10, 5, 'k'); hline(5, 7, 6, 'k'); hline(8, 10, 6, 'k'); put(5, 5, 'n'); put(8, 5, 'n'); }
    if (f & F_STARS) { put(6, 5, 'y'); put(5, 6, 'y'); put(6, 6, 'P'); put(7, 6, 'y'); put(9, 5, 'y'); put(8, 6, 'y'); put(9, 6, 'P'); put(10, 6, 'y'); }
    if (f & F_PATCH) { put(9, 6, 'k'); put(10, 6, 'k'); put(9, 5, 'k'); hline(4, 11, 4, 'k'); }
    if (f & F_MOUSTACHE) { hline(6, 9, 7, p->hair_d == 'k' ? 'n' : p->hair_d); put(5, 8, p->hair_d); put(10, 8, p->hair_d); }
    if (f & F_BEARD) { hline(5, 10, 8, h); hline(5, 10, 9, h); hline(6, 9, 10, h); put(7, 8, 'v'); put(8, 8, 'v'); }
    if (f & F_WHITEBEARD) { hline(5, 10, 8, 'w'); hline(4, 11, 9, 'l'); hline(5, 10, 10, 'w'); hline(6, 9, 11, 'l'); put(7, 8, 'v'); put(8, 8, 'v'); }
    if (f & F_EARRINGS) { put(3, 7, 'y'); put(12, 7, 'y'); }
    /* hats */
    char hc = p->hat_col;
    switch (p->hat) {
    case K_FEZ: box(5, 0, 10, 2, hc); hline(5, 10, 3, hc); put(10, 0, 'k'); put(11, 1, 'k'); put(11, 2, 'k'); put(6, 0, 'o'); break;
    case K_CAP: box(4, 1, 11, 3, hc); hline(9, 13, 4, hc); put(5, 1, 'w'); put(13, 4, 'k'); break;
    case K_CHEF: box(5, 0, 10, 2, 'w'); put(4, 1, 'w'); put(11, 1, 'w'); hline(4, 11, 3, 'l'); put(6, 0, 'l'); put(9, 1, 'l'); break;
    case K_TURBAN: box(3, 1, 12, 3, hc); hline(4, 11, 0, hc); put(7, 1, 'P'); put(8, 1, 'P'); put(7, 2, 'y'); put(8, 2, 'y'); hline(3, 12, 3, 'y'); break;
    case K_BERET: hline(4, 11, 1, hc); hline(3, 12, 2, hc); put(11, 0, hc); break;
    case K_SUNHAT: hline(5, 10, 0, hc); box(5, 1, 10, 2, hc); hline(1, 14, 3, hc); hline(5, 10, 2, 'r'); break;
    case K_TOPHAT: box(5, 0, 10, 2, 'k'); hline(3, 12, 3, 'k'); hline(5, 10, 2, hc); break;
    case K_BAND: hline(4, 11, 3, hc); put(3, 3, hc); put(12, 3, hc); put(13, 4, hc); break;
    case K_VISOR: hline(3, 10, 4, hc); hline(2, 8, 5, hc); put(11, 4, 'k'); break;
    case K_FLOWER: put(10, 1, hc); put(11, 2, hc); put(9, 2, hc); put(10, 3, hc); put(10, 2, 'y'); break;
    default: break;
    }
    /* props */
    switch (p->prop) {
    case P_CAMERA: box(4, 12, 8, 14, 'k'); put(6, 13, 'C'); put(5, 12, 'g'); put(8, 12, 'w'); break;
    case P_BOOK: box(9, 12, 13, 15, 'r'); hline(9, 13, 13, 'c'); put(11, 14, 'c'); break;
    case P_MIC: put(12, 9, 'g'); put(12, 10, 'k'); put(12, 11, 'k'); put(11, 12, p->skin); put(12, 12, p->skin); break;
    case P_TAPE: put(3, 12, 'y'); put(4, 13, 'y'); put(5, 14, 'y'); put(6, 15, 'y'); put(12, 12, 'y'); put(11, 13, 'y'); break;
    case P_WHISTLE: put(8, 12, 'y'); put(8, 13, 'g'); put(9, 13, 'g'); break;
    case P_DRUM: box(9, 12, 14, 15, 'e'); hline(9, 14, 12, 'h'); put(10, 13, 'b'); put(13, 14, 'b'); break;
    case P_SPARKS: put(13, 9, 'y'); put(14, 8, 'a'); put(12, 7, 'y'); put(14, 10, 'o'); put(13, 11, 'k'); put(13, 12, 'k'); break;
    case P_BOX: box(9, 12, 13, 15, 't'); hline(9, 13, 13, 'b'); put(11, 12, 'b'); break;
    case P_CARDS: box(11, 11, 13, 13, 'w'); put(12, 12, 'r'); put(10, 12, 'w'); put(10, 13, 'k'); break;
    case P_HEART: put(8, 13, 'r'); put(10, 13, 'r'); hline(8, 10, 14, 'r'); put(9, 15, 'r'); break;
    case P_MOON: put(12, 8, 'y'); put(13, 9, 'y'); put(12, 10, 'y'); break;
    case P_KEYS: put(11, 13, 'y'); put(12, 13, 'y'); put(12, 14, 'y'); put(13, 14, 'g'); break;
    case P_BADGE: put(10, 13, 'y'); put(11, 13, 'y'); put(10, 14, 'a'); break;
    case P_CUP: box(10, 12, 12, 14, 'w'); put(13, 13, 'w'); put(11, 11, 'l'); break;
    default: break;
    }
    outline();
}

/* skin tones */
#define SK_A 'h', 'e'
#define SK_B 'e', 't'
#define SK_C 't', 'b'
#define SK_D 'c', 'h'

static const struct { uint8_t type; Person p; } PEOPLE[] = {
    {G_NEIGHBOUR,   {SK_A, 'l', 'g', H_SHORT, K_NONE, 0, F_GLASSES, C_VEST, 'e', 't', P_NONE}},
    {G_COUSIN,      {SK_B, 'k', 'n', H_SLICK, K_NONE, 0, F_GRIN, C_SUIT, 'N', 'y', P_KEYS}},
    {G_ROWDY,       {SK_D, 'o', 'r', H_SPIKY, K_NONE, 0, F_GRIN | F_BLUSH, C_STRIPES, 'o', 'y', P_NONE}},
    {G_CABBIE,      {SK_B, 'k', 'n', H_SHORT, K_CAP, 'y', F_MOUSTACHE, C_COLLAR, 'y', 'k', P_NONE}},
    {G_SLEUTH,      {SK_A, 'b', 'k', H_SHORT, K_CAP, 't', F_NONE, C_TIE, 't', 'b', P_NONE}},
    {G_SURFER,      {SK_D, 'y', 'a', H_LONG, K_NONE, 0, F_GRIN, C_FLOWERS, 'C', 'K', P_NONE}},
    {G_BOUNCER,     {SK_C, 'k', 'k', H_BALD, K_NONE, 0, F_SHADES, C_PLAIN, 'n', 'k', P_NONE}},
    {G_STRONGMAN,   {SK_A, 'k', 'n', H_SHORT, K_NONE, 0, F_MOUSTACHE | F_GRIN, C_STRIPES, 'r', 'w', P_NONE}},
    {G_DOORMAN,     {SK_B, 'k', 'n', H_SHORT, K_CAP, 'v', F_NONE, C_SUIT, 'v', 'y', P_NONE}},
    {G_FIREWORKER,  {SK_A, 'b', 'k', H_SPIKY, K_BAND, 'r', F_GRIN, C_PLAIN, 'o', 'a', P_SPARKS}},
    {G_GUIDE,       {SK_D, 'e', 't', H_TAIL, K_SUNHAT, 'y', F_NONE, C_COLLAR, 'j', 'f', P_WHISTLE}},
    {G_DRUMMER,     {SK_C, 'k', 'n', H_CURLY, K_NONE, 0, F_GRIN, C_SASH, 'V', 'y', P_DRUM}},
    {G_SOCIALITE,   {SK_D, 'P', 'v', H_CURLY, K_NONE, 0, F_EARRINGS | F_BLUSH, C_PEARLS, 'K', 'P', P_NONE}},
    {G_IDOL,        {SK_A, 'u', 'B', H_SPIKY, K_NONE, 0, F_STARS | F_GRIN, C_FLOWERS, 'P', 'y', P_NONE}},
    {G_COATCHECK,   {SK_B, 'k', 'n', H_SLICK, K_NONE, 0, F_NONE, C_BOWTIE, 'k', 'r', P_NONE}},
    {G_STORYTELLER, {SK_B, 'g', 's', H_BALD, K_FEZ, 'r', F_BEARD | F_GRIN, C_VEST, 'c', 'N', P_NONE}},
    {G_PAPARAZZO,   {SK_D, 'b', 'k', H_TAIL, K_NONE, 0, F_NONE, C_PLAIN, 's', 'g', P_CAMERA}},
    {G_PASTRY,      {SK_A, 'b', 'k', H_SHORT, K_CHEF, 0, F_MOUSTACHE | F_BLUSH, C_APRON, 'l', 'w', P_NONE}},
    {G_MERCHANT,    {SK_C, 'k', 'n', H_SHORT, K_TURBAN, 'c', F_BEARD, C_SASH, 'q', 'a', P_NONE}},
    {G_GRANNY,      {SK_D, 'l', 'g', H_BUN, K_NONE, 0, F_GLASSES | F_BLUSH, C_PEARLS, 'V', 'P', P_NONE}},
    {G_BOOKWORM,    {SK_A, 'b', 'k', H_SHORT, K_NONE, 0, F_GLASSES, C_COLLAR, 'f', 'j', P_BOOK}},
    {G_TAILOR,      {SK_B, 's', 'd', H_SHORT, K_NONE, 0, F_GLASSES | F_MOUSTACHE, C_VEST, 'w', 'b', P_TAPE}},
    {G_BARISTA,     {SK_D, 'r', 'm', H_TAIL, K_NONE, 0, F_GRIN, C_APRON, 't', 'b', P_CUP}},
    {G_POET,        {SK_A, 'k', 'n', H_LONG, K_BERET, 'k', F_NONE, C_SASH, 'd', 'r', P_NONE}},
    {G_UPSTART,     {SK_D, 'a', 'o', H_SLICK, K_NONE, 0, F_GRIN, C_TIE, 'u', 'r', P_BADGE}},
    {G_BANDLEADER,  {SK_B, 'k', 'n', H_SHORT, K_TOPHAT, 'r', F_GRIN, C_SASH, 'r', 'y', P_NONE}},
    {G_USHER,       {SK_C, 'k', 'n', H_SHORT, K_NONE, 0, F_GRIN, C_VEST, 'w', 'r', P_NONE}},
    {G_FORTUNE,     {SK_B, 'k', 'n', H_SCARF, K_NONE, 0, F_EARRINGS, C_PEARLS, 'p', 'V', P_NONE}},
    {G_MATCHMAKER,  {SK_D, 'r', 'v', H_CURLY, K_FLOWER, 'K', F_BLUSH | F_GRIN, C_PLAIN, 'K', 'r', P_HEART}},
    {G_SAGE,        {SK_B, 'l', 'g', H_BALD, K_FEZ, 'r', F_WHITEBEARD, C_PLAIN, 'c', 'h', P_NONE}},
    {G_MOONCHILD,   {SK_A, 'V', 'p', H_WILD, K_NONE, 0, F_GRIN, C_PLAIN, 'N', 'y', P_MOON}},
    {G_PUNK,         {SK_C, 'k', 'n', H_CURLY, K_NONE, 0, F_SHADES | F_GRIN, C_COLLAR, 'r', 'k', P_MIC}},
    {G_SMUGGLER,    {SK_B, 'k', 'n', H_SHORT, K_BAND, 'k', F_PATCH | F_MOUSTACHE, C_STRIPES, 'n', 'l', P_BOX}},
    {G_CARDSHARK,   {SK_A, 'k', 'n', H_SLICK, K_VISOR, 'j', F_MOUSTACHE, C_VEST, 'w', 'f', P_CARDS}},
    {G_TYCOON,      {SK_C, 'k', 'n', H_SHORT, K_TOPHAT, 'k', F_BEARD | F_GRIN, C_SASH, 'a', 'r', P_NONE}},
    {G_CHAMPION,    {SK_B, 'k', 'n', H_SHORT, K_BAND, 'r', F_GRIN, C_BELT, 'r', 'y', P_NONE}},
};

/* ------------------------------------------------------------------ */
/* the creatures and the stars, by hand                                 */

static const char GOAT[] =
    "................"
    ".kk..........kk."
    "kggk.kkkkkk.kggk"
    ".kggkwwwwwwkggk."
    "..kkwwwwwwwwkk.."
    "...kwwkwwkwwk..."
    "...kwwkwwkwwk..."
    "...kwwwwwwwwk..."
    "....kwwwwwwk...."
    "....kwKKKKwk...."
    ".....kKkkKk....."
    ".....kwwwwk....."
    "......kggk......"
    "......kggk......"
    ".......kk......."
    "................";
static const char KITTEN[] =
    "................"
    "..k..........k.."
    ".kok........kok."
    ".kook......kook."
    ".kooakkkkkkaook."
    ".kooooooooooook."
    "kooookooooooooko"
    "koookjkookjkoook"
    "kooooooooooooook"
    "kwwoooKkkKoooowk"
    "kwwwwooookooowwk"
    ".kwwwwwwwwwwwwk."
    "..kwwwwwwwwwwk.."
    "...kkoookooookk."
    ".....kk...kk...."
    "................";
static const char PARROT[] =
    "......kkkk......"
    ".....krrrrk....."
    "....krrrrrrk...."
    "...krwwkrrrrk..."
    "...krwkkrrrrk..."
    "..kyykwrrrrrk..."
    "..kyyykrrrrrk..."
    "...kyk.krrrrk..."
    "....k..kzzrrk..."
    ".......kzzzzk..."
    "......kzBzBzk..."
    "......kzBzBzk..."
    ".....kBBkBBk...."
    ".....kBk.kBk...."
    "....kk...kk....."
    "................";
static const char PILOT[] =
    ".....kkkkkk....."
    "....kCIIIICk...."
    "...kCIzzzzICk..."
    "...kIzkzzkzIk..."
    "...kIzzzzzzIk..."
    "...kCIzkkzICk..."
    "....kCIIIICk...."
    "..kkkkkkkkkkkk.."
    ".kllwwwwwwwwllk."
    "kggllllllllllggk"
    "kyysyysyysyysyyk"
    ".kggggggggggggk."
    "..kkkkkkkkkkkk.."
    "....ka.aa.ak...."
    ".....a.aa.a....."
    "................";
static const char WISHFISH[] =
    "................"
    "......kkk......."
    "....kkyyykk....k"
    "...kyyayyyakk.kk"
    "..kyykyyayyyakak"
    ".kyykkyayyyayaak"
    ".kyyyyyyayyyyaak"
    "kaayyyyyyayyyak."
    ".kaaayyyyyyayaak"
    "..kaaaayyyyayaak"
    "...kkaaaaaakk.kk"
    ".....kkkkkk....k"
    "......I...I....."
    "....I...I......."
    "................"
    "................";
static const char SERPENT[] =
    "...kkkk........."
    "..kjjjjk........"
    ".kjkjjjjk......."
    ".kjjjjjzjk......"
    ".kKjjjjjjk......"
    "..kkkjjjjk......"
    "....kjjjk..kkk.."
    "...kjjjk..kjjjk."
    "..kjjjk..kjjzjjk"
    "..kjjjk.kjjkkjjk"
    "..kjjjjkjjk.kjjk"
    "...kjjjjjk..kjk."
    ".BBBkkkkkBBBkkBB"
    "BBuBBBuBBBBuBBBB"
    "BBBBBBBBBuBBBBuB"
    "................";
static const char CYCLOPS[] =
    "....kkkkkkkk...."
    "...kvvvvvvvvk..."
    "..kvvbbbbbbvvk.."
    "..kjjjjjjjjjjk.."
    ".kjjkkkkkkkjjjk."
    ".kjkwwwwwwwkjjk."
    ".kjkwwkkkwwkjjk."
    ".kjkwwkrkwwkjjk."
    ".kjjkwwwwwkjjjk."
    ".kjjjkkkkkjjjjk."
    ".kjjjjjjjjjjjjk."
    ".kjjkwkwkwkjjjk."
    "..kjjkkkkkkjjk.."
    "..kttttttttttk.."
    ".kttbttttbtttk.."
    ".kkkkkkkkkkkkk..";
static const char PHOENIX[] =
    "......kk........"
    ".....kyak......."
    "....kyaaok......"
    "...kkaokok......"
    "..kyyk.kok.kk..."
    ".kyooork.kkyak.."
    "kyaoooorkkyaok.."
    "kaorrroooraook.."
    ".kaorrrrooaok..."
    "..kaorrrrrok...."
    "...kkorrrrk....."
    ".....kaorrk....."
    "....kaook.k....."
    "...kaok.kaok...."
    "..kyk....kyk...."
    "................";
static const char SHADOW[] =
    "................"
    ".....kkkkkk....."
    "....knnnnnnk...."
    "...knnnnnnnnk..."
    "...knIInnIInk..."
    "...knIInnIInk..."
    "...knnnnnnnnk..."
    "..knnnnkknnnnk.."
    "..knnnnnnnnnnk.."
    ".knnnnnnnnnnnnk."
    ".knnndnnnndnnnk."
    "knnnnnnnnnnnnnnk"
    "knndnnnndnnnndnk"
    "knnnknnnknnnknnk"
    ".kk.k.kk.k.kk.k."
    "................";
static const char SPHINX[] =
    "....kkkkkkkk...."
    "...kyNyNyNyNk..."
    "..kyNyNyNyNyNk.."
    "..kNkkkkkkkkyk.."
    "..kyktttttkNyk.."
    "..kNkkttkkkyNk.."
    "..kykttttttkNk.."
    "..kNkttkktkyyk.."
    "..kyyktttkyNyk.."
    "...kNykkkyNyk..."
    "....kkttttkk...."
    "..kkettttttekk.."
    ".kettteettttek.."
    ".ketttkkktttekk."
    ".kkkkk...kkkkkk."
    "................";

/* the door, the neighbour's lamp and the two sirens */
static const char DOOR[] =
    "....kkkkkkkk...."
    "..kkBBBBBBBBkk.."
    ".kBBuuuuuuuuBBk."
    ".kBuBBBBBBBBuBk."
    "kBuBkkkkkkkkBuBk"
    "kBuBkNNNNNNkBuBk"
    "kBuBkNBBBBNkBuBk"
    "kBuBkNBBBBNkBuBk"
    "kBuBkNNNNNNkBuBk"
    "kBuBkkkkkkkkBuBk"
    "kBuBBBBBBByyBuBk"
    "kBuBBBBBBByyBuBk"
    "kBuBkkkkkkkkBuBk"
    "kBuBkNNNNNNkBuBk"
    "kBuBkNBBBBNkBuBk"
    "kBBBkkkkkkkkBBBk";
static const char LAMP[] =
    "kkkkkkkkkk"
    "kyyyyyyyyk"
    "kyykkkkyyk"
    "kykhhhhkyk"
    "kykhkkhkyk"
    "kykhhhhkyk"
    "kyykhhkyyk"
    "kyyyhhyyyk"
    "kyyyyyyyyk"
    "kkkkkkkkkk";
static const char POLICE[] =
    "....kkkkkkkk...."
    "...kBBBrrrrrk..."
    "..kBIBBrrKrrrk.."
    "kkkkkkkkkkkkkkkk"
    "kwwwwwwwwwwwwwwk"
    "kwNNNwwwwwwNNNwk"
    "kwNNNwwwwwwNNNwk"
    "kwwwwwkkkkwwwwwk"
    "kkkkkkkkkkkkkkkk"
    ".kk.kk....kk.kk.";
static const char FIRE[] =
    "..........kkk..."
    ".........kyyyk.."
    "kkkkkkkkkkkkkkk."
    "krrrrrrrrrrrrrrk"
    "krwwrrrrrrrrwwrk"
    "krwwrryyyyrrwwrk"
    "krrrrrrrrrrrrrrk"
    "kyyyyyyyyyyyyyyk"
    "kkkkkkkkkkkkkkkk"
    ".kk.kk....kk.kk.";

void ph_art_load(void) {
    if (ph_spr[G_NEIGHBOUR].px) return;
    for (int i = 0; i < ARRAY_LEN(PEOPLE); i++) {
        paint_person(&PEOPLE[i].p);
        char buf[16 * 16 + 1];
        for (int y = 0; y < 16; y++) memcpy(buf + y * 16, px[y], 16);
        buf[256] = 0;
        spr_make(&ph_spr[PEOPLE[i].type], 16, 16, buf);
    }
    spr_make(&ph_spr[G_GOAT], 16, 16, GOAT);
    spr_make(&ph_spr[G_KITTEN], 16, 16, KITTEN);
    spr_make(&ph_spr[G_PARROT], 16, 16, PARROT);
    spr_make(&ph_spr[G_PILOT], 16, 16, PILOT);
    spr_make(&ph_spr[G_WISHFISH], 16, 16, WISHFISH);
    spr_make(&ph_spr[G_SERPENT], 16, 16, SERPENT);
    spr_make(&ph_spr[G_CYCLOPS], 16, 16, CYCLOPS);
    spr_make(&ph_spr[G_PHOENIX], 16, 16, PHOENIX);
    spr_make(&ph_spr[G_SHADOW], 16, 16, SHADOW);
    spr_make(&ph_spr[G_SPHINX], 16, 16, SPHINX);
    spr_make(&ph_spr[PS_DOOR], 16, 16, DOOR);
    spr_make(&ph_spr[PS_LAMP], 10, 10, LAMP);
    spr_make(&ph_spr[PS_POLICE], 16, 10, POLICE);
    spr_make(&ph_spr[PS_FIRE], 16, 10, FIRE);
}
