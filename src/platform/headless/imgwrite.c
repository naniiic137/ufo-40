/* UFO 40 - minimal PNG (fixed-Huffman deflate) and GIF (LZW) writers. */
#include "imgwrite.h"
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------- deflate */

typedef struct {
    uint8_t *out;
    size_t len, cap;
    uint32_t bitbuf;
    int bitcnt;
} BitW;

static void bw_byte(BitW *b, uint8_t v) {
    if (b->len == b->cap) {
        b->cap = b->cap ? b->cap * 2 : 65536;
        b->out = (uint8_t *)realloc(b->out, b->cap);
    }
    b->out[b->len++] = v;
}
static void bw_put(BitW *b, uint32_t bits, int n) {
    b->bitbuf |= bits << b->bitcnt;
    b->bitcnt += n;
    while (b->bitcnt >= 8) {
        bw_byte(b, (uint8_t)(b->bitbuf & 0xFF));
        b->bitbuf >>= 8;
        b->bitcnt -= 8;
    }
}
static void bw_huff(BitW *b, uint32_t code, int n) {
    uint32_t rev = 0;
    for (int i = 0; i < n; i++) rev |= ((code >> i) & 1u) << (n - 1 - i);
    bw_put(b, rev, n);
}
static void put_lit(BitW *b, int lit) {
    if (lit < 144) bw_huff(b, 0x30u + (uint32_t)lit, 8);
    else if (lit < 256) bw_huff(b, 0x190u + (uint32_t)(lit - 144), 9);
    else if (lit < 280) bw_huff(b, (uint32_t)(lit - 256), 7);
    else bw_huff(b, 0xC0u + (uint32_t)(lit - 280), 8);
}

static const uint16_t LBASE[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                                   35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
static const uint8_t LEXT[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const uint16_t DBASE[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
                                   193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
static const uint8_t DEXT[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
                                 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

static void put_match(BitW *b, int len, int dist) {
    int i = 28;
    while (LBASE[i] > len) i--;
    put_lit(b, 257 + i);
    if (LEXT[i]) bw_put(b, (uint32_t)(len - LBASE[i]), LEXT[i]);
    int j = 29;
    while (DBASE[j] > dist) j--;
    bw_huff(b, (uint32_t)j, 5);
    if (DEXT[j]) bw_put(b, (uint32_t)(dist - DBASE[j]), DEXT[j]);
}

#define WSIZE 32768
#define HBITS 15
static uint8_t *zlib_compress(const uint8_t *data, size_t n, size_t *out_len) {
    BitW b = {0};
    bw_byte(&b, 0x78);
    bw_byte(&b, 0x01);
    bw_put(&b, 1, 1); /* final block */
    bw_put(&b, 1, 2); /* fixed huffman */
    int32_t *head = (int32_t *)malloc(sizeof(int32_t) << HBITS);
    int32_t *prev = (int32_t *)malloc(sizeof(int32_t) * WSIZE);
    for (int i = 0; i < (1 << HBITS); i++) head[i] = -1;
    size_t i = 0;
    while (i < n) {
        int best_len = 0, best_dist = 0;
        if (i + 3 <= n) {
            uint32_t h = ((uint32_t)data[i] << 10 ^ (uint32_t)data[i + 1] << 5 ^ data[i + 2]) & ((1u << HBITS) - 1);
            int32_t cand = head[h];
            int chain = 48;
            while (cand >= 0 && chain-- > 0 && (int64_t)i - cand <= WSIZE - 1) {
                size_t maxl = n - i < 258 ? n - i : 258;
                size_t l = 0;
                while (l < maxl && data[cand + l] == data[i + l]) l++;
                if ((int)l > best_len) {
                    best_len = (int)l;
                    best_dist = (int)(i - (size_t)cand);
                    if (l == maxl) break;
                }
                cand = prev[cand & (WSIZE - 1)];
            }
            prev[i & (WSIZE - 1)] = head[h];
            head[h] = (int32_t)i;
        }
        if (best_len >= 3) {
            put_match(&b, best_len, best_dist);
            /* insert skipped positions into the hash */
            for (size_t k = i + 1; k < i + (size_t)best_len && k + 3 <= n; k++) {
                uint32_t h = ((uint32_t)data[k] << 10 ^ (uint32_t)data[k + 1] << 5 ^ data[k + 2]) & ((1u << HBITS) - 1);
                prev[k & (WSIZE - 1)] = head[h];
                head[h] = (int32_t)k;
            }
            i += (size_t)best_len;
        } else {
            put_lit(&b, data[i]);
            i++;
        }
    }
    put_lit(&b, 256);
    if (b.bitcnt > 0) bw_put(&b, 0, 8 - b.bitcnt);
    uint32_t a = 1, s2 = 0;
    for (size_t k = 0; k < n; k++) {
        a = (a + data[k]) % 65521u;
        s2 = (s2 + a) % 65521u;
    }
    uint32_t adler = (s2 << 16) | a;
    bw_byte(&b, (uint8_t)(adler >> 24));
    bw_byte(&b, (uint8_t)(adler >> 16));
    bw_byte(&b, (uint8_t)(adler >> 8));
    bw_byte(&b, (uint8_t)adler);
    free(head);
    free(prev);
    *out_len = b.len;
    return b.out;
}

/* ---------------------------------------------------------------- PNG */

static uint32_t crc_table[256];
static void crc_init(void) {
    static int done;
    if (done) return;
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_table[n] = c;
    }
    done = 1;
}
static uint32_t crc_update(uint32_t c, const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
    return c;
}
static void be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}
static void write_chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    uint8_t hdr[8];
    be32(hdr, len);
    memcpy(hdr + 4, type, 4);
    fwrite(hdr, 1, 8, f);
    if (len) fwrite(data, 1, len, f);
    uint32_t c = crc_update(0xFFFFFFFFu, (const uint8_t *)type, 4);
    if (len) c = crc_update(c, data, len);
    uint8_t cb[4];
    be32(cb, c ^ 0xFFFFFFFFu);
    fwrite(cb, 1, 4, f);
}

int png_write_indexed(const char *path, int w, int h, const uint8_t *px, const uint8_t (*pal)[3], int ncolors,
                      int scale) {
    crc_init();
    if (scale < 1) scale = 1;
    int W = w * scale, H = h * scale;
    size_t raw_len = (size_t)(W + 1) * H;
    uint8_t *raw = (uint8_t *)malloc(raw_len);
    for (int y = 0; y < H; y++) {
        uint8_t *row = raw + (size_t)y * (W + 1);
        row[0] = 0;
        const uint8_t *src = px + (y / scale) * w;
        for (int x = 0; x < W; x++) row[1 + x] = src[x / scale];
    }
    size_t zlen;
    uint8_t *z = zlib_compress(raw, raw_len, &zlen);
    free(raw);
    FILE *f = fopen(path, "wb");
    if (!f) { free(z); return -1; }
    static const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13];
    be32(ihdr, (uint32_t)W);
    be32(ihdr + 4, (uint32_t)H);
    ihdr[8] = 8; ihdr[9] = 3; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    write_chunk(f, "IHDR", ihdr, 13);
    uint8_t plte[256 * 3];
    for (int i = 0; i < ncolors; i++) { plte[i * 3] = pal[i][0]; plte[i * 3 + 1] = pal[i][1]; plte[i * 3 + 2] = pal[i][2]; }
    write_chunk(f, "PLTE", plte, (uint32_t)(ncolors * 3));
    write_chunk(f, "IDAT", z, (uint32_t)zlen);
    write_chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(z);
    return 0;
}

/* ---------------------------------------------------------------- GIF */

typedef struct {
    FILE *f;
    uint8_t block[256];
    int blen;
    uint32_t bits;
    int nbits;
} GifOut;

static void go_byte(GifOut *o, uint8_t v) {
    o->block[o->blen++] = v;
    if (o->blen == 255) {
        fputc(255, o->f);
        fwrite(o->block, 1, 255, o->f);
        o->blen = 0;
    }
}
static void go_code(GifOut *o, int code, int size) {
    o->bits |= (uint32_t)code << o->nbits;
    o->nbits += size;
    while (o->nbits >= 8) {
        go_byte(o, (uint8_t)(o->bits & 0xFF));
        o->bits >>= 8;
        o->nbits -= 8;
    }
}
static void go_flush(GifOut *o) {
    if (o->nbits > 0) go_byte(o, (uint8_t)(o->bits & 0xFF));
    o->bits = 0;
    o->nbits = 0;
    if (o->blen > 0) {
        fputc(o->blen, o->f);
        fwrite(o->block, 1, (size_t)o->blen, o->f);
        o->blen = 0;
    }
    fputc(0, o->f);
}

static int16_t (*gif_child)[32];

static void lzw_encode(FILE *f, const uint8_t *px, int n) {
    const int min = 5, clear = 32, eoi = 33, first = 34;
    if (!gif_child) gif_child = (int16_t(*)[32])malloc(sizeof(int16_t) * 32 * 4096);
    GifOut o = {f, {0}, 0, 0, 0};
    fputc(min, f);
    int size = min + 1, next = first;
    memset(gif_child, 0xFF, sizeof(int16_t) * 32 * 4096);
    go_code(&o, clear, size);
    int prefix = px[0] & 31;
    for (int i = 1; i < n; i++) {
        int k = px[i] & 31;
        int c = gif_child[prefix][k];
        if (c >= 0) { prefix = c; continue; }
        go_code(&o, prefix, size);
        if (next < 4096) {
            gif_child[prefix][k] = (int16_t)next;
            next++;
            if (next > (1 << size) && size < 12) size++;
        } else {
            go_code(&o, clear, size);
            memset(gif_child, 0xFF, sizeof(int16_t) * 32 * 4096);
            size = min + 1;
            next = first;
        }
        prefix = k;
    }
    go_code(&o, prefix, size);
    go_code(&o, eoi, size);
    go_flush(&o);
}

static void le16(FILE *f, int v) { fputc(v & 0xFF, f); fputc((v >> 8) & 0xFF, f); }

int gif_begin(GifWriter *g, const char *path, int w, int h, int scale, const uint8_t (*pal)[3]) {
    memset(g, 0, sizeof *g);
    g->f = fopen(path, "wb");
    if (!g->f) return -1;
    g->w = w; g->h = h; g->scale = scale < 1 ? 1 : scale;
    g->buf = (uint8_t *)malloc((size_t)w * h * g->scale * g->scale);
    fwrite("GIF89a", 1, 6, g->f);
    le16(g->f, w * g->scale);
    le16(g->f, h * g->scale);
    fputc(0xF4, g->f); /* global colour table, 32 entries */
    fputc(0, g->f);
    fputc(0, g->f);
    for (int i = 0; i < 32; i++) fwrite(pal[i], 1, 3, g->f);
    static const uint8_t loop[] = {0x21, 0xFF, 0x0B, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0', 0x03, 0x01, 0x00, 0x00, 0x00};
    fwrite(loop, 1, sizeof loop, g->f);
    return 0;
}

int gif_frame(GifWriter *g, const uint8_t *px, int delay_cs) {
    int W = g->w * g->scale, H = g->h * g->scale;
    /* only encode the rectangle that changed since the previous frame */
    int x0 = 0, y0 = 0, x1 = g->w - 1, y1 = g->h - 1;
    if (g->prev) {
        x0 = g->w; y0 = g->h; x1 = -1; y1 = -1;
        for (int y = 0; y < g->h; y++)
            for (int x = 0; x < g->w; x++)
                if (px[y * g->w + x] != g->prev[y * g->w + x]) {
                    if (x < x0) x0 = x;
                    if (x > x1) x1 = x;
                    if (y < y0) y0 = y;
                    if (y > y1) y1 = y;
                }
        if (x1 < 0) { x0 = y0 = x1 = y1 = 0; }
    } else {
        g->prev = (uint8_t *)malloc((size_t)g->w * g->h);
    }
    memcpy(g->prev, px, (size_t)g->w * g->h);
    int rw = (x1 - x0 + 1) * g->scale, rh = (y1 - y0 + 1) * g->scale;
    int n = 0;
    for (int y = 0; y < rh; y++)
        for (int x = 0; x < rw; x++)
            g->buf[n++] = px[(y0 + y / g->scale) * g->w + x0 + x / g->scale];
    uint8_t gce[] = {0x21, 0xF9, 0x04, 0x04, (uint8_t)(delay_cs & 0xFF), (uint8_t)(delay_cs >> 8), 0, 0};
    fwrite(gce, 1, sizeof gce, g->f);
    fputc(0x2C, g->f);
    le16(g->f, x0 * g->scale); le16(g->f, y0 * g->scale); le16(g->f, rw); le16(g->f, rh);
    fputc(0, g->f);
    lzw_encode(g->f, g->buf, rw * rh);
    (void)W;
    (void)H;
    return 0;
}

void gif_end(GifWriter *g) {
    if (!g->f) return;
    fputc(0x3B, g->f);
    fclose(g->f);
    free(g->buf);
    free(g->prev);
    g->prev = NULL;
    g->f = NULL;
}
