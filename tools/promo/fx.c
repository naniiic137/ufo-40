/* UFO 40 - the promo's compositor. It plays the edit in tools/promo/cuts.txt
 * from the recorded frames (build/promo/clips/CLIP/000000.png ...) and draws
 * every effect at the output size, sampling the 320x180 game nearest-
 * neighbour only, so each game pixel stays a sharp block:
 *
 *   cuts on the beat with white flashes, pixel-mosaic and dither wipes and
 *   RGB-split / slice glitches; zoom punches and screen shakes on the big
 *   hits; captions that slide, drop or slam in while the TRIBUTE TO line
 *   types on; the glitchy hook, the 22-games grid that collapses into the
 *   saucer, and the end card with the logo beamed in and stars; a light CRT
 *   look (scanlines and a vignette) over the lot.
 *
 *   promo_fx [options] > frames.rgb     raw RGB24 frames, 60 a second
 *   promo_fx -wav OUT.wav [options]     the edit's sound effects instead
 *   promo_fx -info [options]            the edit's length and shots
 *
 *   -clips DIR       the recordings (build/promo/clips)
 *   -cuts FILE       the edit (tools/promo/cuts.txt)
 *   -scripts DIR     the clip scripts, for the captions (tools/promo/clips)
 *   -size W H S X Y  frame size, the game's scale and its top-left corner
 *   -crt 0|1         light scanlines and a vignette (default 1)
 *   -end link|nolink the end card with the repo link or without
 *   -range A B       only frames A..B-1 (up to 8, joined in order)
 *   -step N          every Nth frame (2 = 30 fps for the GIF)
 *   -cards TOP BOT   still cards drawn at scale S above and below the game
 *
 * Built from this file and the engine's gfx.c and font.c (the console's own
 * palette and fonts); see the Makefile's promo_fx target. */
#include "../../src/engine/font.h"
#include "../../src/engine/gfx.h"
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define SW SCREEN_W
#define SH SCREEN_H
#define BEAT 24 /* 150 bpm at 60 fps */
#define BAR 96

static void die(const char *msg, const char *arg) {
    fprintf(stderr, "promo_fx: %s %s\n", msg, arg ? arg : "");
    exit(1);
}

/* ---- inflate (after zlib's puff.c) --------------------------------------- */

typedef struct {
    const uint8_t *in;
    size_t inlen, inpos;
    uint32_t bitbuf;
    int bitcnt, err;
    uint8_t *out;
    size_t outlen, outpos;
} Inf;

static int getbits(Inf *s, int need) {
    uint32_t val = s->bitbuf;
    while (s->bitcnt < need) {
        if (s->inpos >= s->inlen) { s->err = 1; return 0; }
        val |= (uint32_t)s->in[s->inpos++] << s->bitcnt;
        s->bitcnt += 8;
    }
    s->bitbuf = val >> need;
    s->bitcnt -= need;
    return (int)(val & ((1u << need) - 1));
}

typedef struct { short count[16], symbol[320]; } Huff;

static int hdecode(Inf *s, const Huff *h) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len < 16; len++) {
        code |= getbits(s, 1);
        int count = h->count[len];
        if (code - count < first) return h->symbol[index + (code - first)];
        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }
    s->err = 2;
    return 0;
}

static void hbuild(Huff *h, const short *len, int n) {
    short offs[16];
    memset(h->count, 0, sizeof h->count);
    for (int i = 0; i < n; i++) h->count[len[i]]++;
    offs[1] = 0;
    for (int l = 1; l < 15; l++) offs[l + 1] = (short)(offs[l] + h->count[l]);
    for (int i = 0; i < n; i++)
        if (len[i]) h->symbol[offs[len[i]]++] = (short)i;
}

static const short LBASE[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27,
                                31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
static const short LEXT[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const short DBASE[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
                                193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
static const short DEXT[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

static void codes(Inf *s, const Huff *lc, const Huff *dc) {
    for (;;) {
        int sym = hdecode(s, lc);
        if (s->err) return;
        if (sym < 256) {
            if (s->outpos >= s->outlen) { s->err = 3; return; }
            s->out[s->outpos++] = (uint8_t)sym;
        } else if (sym == 256) {
            return;
        } else {
            sym -= 257;
            if (sym >= 29) { s->err = 4; return; }
            int len = LBASE[sym] + getbits(s, LEXT[sym]);
            int ds = hdecode(s, dc);
            if (s->err || ds >= 30) { s->err = 4; return; }
            size_t dist = (size_t)(DBASE[ds] + getbits(s, DEXT[ds]));
            if (dist > s->outpos || s->outpos + (size_t)len > s->outlen) { s->err = 5; return; }
            for (int i = 0; i < len; i++, s->outpos++) s->out[s->outpos] = s->out[s->outpos - dist];
        }
    }
}

static int inflate_buf(Inf *s) {
    static Huff FL, FD;
    static int fixed_ready;
    if (!fixed_ready) {
        short l[288];
        for (int i = 0; i < 288; i++) l[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
        hbuild(&FL, l, 288);
        for (int i = 0; i < 30; i++) l[i] = 5;
        hbuild(&FD, l, 30);
        fixed_ready = 1;
    }
    int last;
    do {
        last = getbits(s, 1);
        int type = getbits(s, 2);
        if (s->err) return s->err;
        if (type == 0) {
            s->bitbuf = 0;
            s->bitcnt = 0;
            if (s->inpos + 4 > s->inlen) return 6;
            size_t len = s->in[s->inpos] | (size_t)s->in[s->inpos + 1] << 8;
            s->inpos += 4;
            if (s->inpos + len > s->inlen || s->outpos + len > s->outlen) return 6;
            memcpy(s->out + s->outpos, s->in + s->inpos, len);
            s->inpos += len;
            s->outpos += len;
        } else if (type == 1) {
            codes(s, &FL, &FD);
        } else if (type == 2) {
            static const short ORD[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
            short lengths[320];
            Huff lc, dc;
            int nlen = getbits(s, 5) + 257, ndist = getbits(s, 5) + 1, ncode = getbits(s, 4) + 4;
            for (int i = 0; i < 19; i++) lengths[ORD[i]] = (short)(i < ncode ? getbits(s, 3) : 0);
            hbuild(&lc, lengths, 19);
            int idx = 0;
            while (idx < nlen + ndist && !s->err) {
                int sym = hdecode(s, &lc);
                if (sym < 16) { lengths[idx++] = (short)sym; continue; }
                int len = 0, rep;
                if (sym == 16) {
                    if (!idx) return 7;
                    len = lengths[idx - 1];
                    rep = 3 + getbits(s, 2);
                } else if (sym == 17) rep = 3 + getbits(s, 3);
                else rep = 11 + getbits(s, 7);
                if (idx + rep > nlen + ndist) return 7;
                while (rep--) lengths[idx++] = (short)len;
            }
            hbuild(&lc, lengths, nlen);
            hbuild(&dc, lengths + nlen, ndist);
            codes(s, &lc, &dc);
        } else return 8;
        if (s->err) return s->err;
    } while (!last);
    return 0;
}

/* ---- PNG (8-bit palette, RGB or RGBA; what the headless runner writes) --- */

typedef struct { int w, h; uint32_t *px; } Img; /* 0xRRGGBB */

static uint32_t be32(const uint8_t *p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }

static int paeth(int a, int b, int c) {
    int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
    return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}

static void png_load(const char *path, Img *img) {
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open", path);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc((size_t)n);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) die("cannot read", path);
    fclose(f);
    if (n < 8 || memcmp(buf, "\x89PNG", 4)) die("not a PNG:", path);
    uint8_t *idat = malloc((size_t)n);
    size_t idn = 0;
    uint8_t pal[256][3] = {{0}};
    int w = 0, h = 0, depth = 0, ctype = 0;
    for (long p = 8; p + 8 <= n;) {
        uint32_t len = be32(buf + p);
        const uint8_t *type = buf + p + 4, *d = buf + p + 8;
        if (p + 12 + (long)len > n) break;
        if (!memcmp(type, "IHDR", 4)) {
            w = (int)be32(d);
            h = (int)be32(d + 4);
            depth = d[8];
            ctype = d[9];
        } else if (!memcmp(type, "PLTE", 4)) {
            memcpy(pal, d, len > 768 ? 768 : len);
        } else if (!memcmp(type, "IDAT", 4)) {
            memcpy(idat + idn, d, len);
            idn += len;
        }
        p += 12 + (long)len;
    }
    int bpp = ctype == 3 ? 1 : ctype == 2 ? 3 : ctype == 6 ? 4 : 0;
    if (depth != 8 || !bpp || w <= 0 || h <= 0) die("unsupported PNG:", path);
    size_t stride = (size_t)w * bpp, rawn = (stride + 1) * h;
    uint8_t *raw = malloc(rawn);
    Inf s = {idat + 2, idn - 2, 0, 0, 0, 0, raw, rawn, 0};
    if (inflate_buf(&s) || s.outpos != rawn) die("bad PNG data:", path);
    for (int y = 0; y < h; y++) {
        uint8_t *row = raw + y * (stride + 1), ft = row[0], *r = row + 1;
        const uint8_t *up = y ? raw + (y - 1) * (stride + 1) + 1 : NULL;
        for (size_t x = 0; x < stride; x++) {
            int a = x >= (size_t)bpp ? r[x - bpp] : 0, b = up ? up[x] : 0, c = up && x >= (size_t)bpp ? up[x - bpp] : 0;
            int v = ft == 1 ? a : ft == 2 ? b : ft == 3 ? (a + b) / 2 : ft == 4 ? paeth(a, b, c) : 0;
            r[x] = (uint8_t)(r[x] + v);
        }
    }
    img->w = w;
    img->h = h;
    img->px = realloc(img->px, sizeof(uint32_t) * w * h);
    for (int y = 0; y < h; y++) {
        const uint8_t *r = raw + y * (stride + 1) + 1;
        for (int x = 0; x < w; x++) {
            const uint8_t *c = bpp == 1 ? pal[r[x]] : r + x * bpp;
            img->px[y * w + x] = (uint32_t)c[0] << 16 | (uint32_t)c[1] << 8 | c[2];
        }
    }
    free(raw);
    free(idat);
    free(buf);
}

/* ---- the recordings --------------------------------------------------------- */

static const char *clips_dir = "build/promo/clips";
static const char *cuts_path = "tools/promo/cuts.txt";
static const char *scripts_dir = "tools/promo/clips";

typedef struct {
    char name[32];
    int count;      /* frames recorded */
    int16_t *wav;   /* stereo, 800 sample frames per video frame */
    long wav_n;     /* sample frames */
} Clip;
static Clip clips[64];
static int nclips;

static int frame_exists(const char *name, int i) {
    char path[600];
    snprintf(path, sizeof path, "%s/%s/%06d.png", clips_dir, name, i);
    FILE *f = fopen(path, "rb");
    if (f) fclose(f);
    return f != NULL;
}

static int clip_id(const char *name) {
    for (int i = 0; i < nclips; i++)
        if (!strcmp(clips[i].name, name)) return i;
    if (nclips == 64) die("too many clips", NULL);
    Clip *c = &clips[nclips];
    snprintf(c->name, sizeof c->name, "%s", name);
    if (!frame_exists(name, 0)) die("no recording for", name);
    int lo = 1, hi = 1;
    while (frame_exists(name, hi)) { lo = hi + 1; hi *= 2; }
    while (lo < hi) { /* first missing frame */
        int mid = (lo + hi) / 2;
        if (frame_exists(name, mid)) lo = mid + 1;
        else hi = mid;
    }
    c->count = lo;
    return nclips++;
}

static void clip_wav(Clip *c) {
    if (c->wav || c->wav_n < 0) return;
    char path[600];
    snprintf(path, sizeof path, "%s/%s.wav", clips_dir, c->name);
    FILE *f = fopen(path, "rb");
    if (!f) { c->wav_n = -1; return; }
    fseek(f, 0, SEEK_END);
    long n = (ftell(f) - 44) / 4;
    fseek(f, 44, SEEK_SET);
    c->wav = malloc((size_t)(n > 0 ? n : 1) * 4);
    c->wav_n = (long)fread(c->wav, 4, (size_t)(n > 0 ? n : 0), f);
    fclose(f);
}

/* a small cache of decoded frames (at most a dozen are needed per frame) */
#define NSLOT 40
static struct { int clip, frame; unsigned used; Img img; } slot[NSLOT];
static unsigned use_clock;

static const Img *frame_get(int clip, int frame) {
    Clip *c = &clips[clip];
    if (frame < 0) frame = 0;
    if (frame >= c->count) frame = c->count - 1;
    int lru = 0;
    for (int i = 0; i < NSLOT; i++) {
        if (slot[i].used && slot[i].clip == clip && slot[i].frame == frame) {
            slot[i].used = ++use_clock;
            return &slot[i].img;
        }
        if (slot[i].used < slot[lru].used) lru = i;
    }
    char path[600];
    snprintf(path, sizeof path, "%s/%s/%06d.png", clips_dir, c->name, frame);
    png_load(path, &slot[lru].img);
    if (slot[lru].img.w != SW || slot[lru].img.h != SH) die("not a 320x180 frame:", path);
    slot[lru].clip = clip;
    slot[lru].frame = frame;
    slot[lru].used = ++use_clock;
    return &slot[lru].img;
}

/* ---- the edit ------------------------------------------------------------------ */

enum { K_CLIP, K_HOOK, K_GRID, K_END };
enum { T_CUT, T_FLASH, T_GLITCH, T_MOSAIC, T_DITHER, T_ZOOM };
enum { C_SLIDE, C_DROP, C_SLAM };

typedef struct {
    char align;
    int x, y, scale, col;
    char text[128];
} Text;

typedef struct {
    int kind, clip, first, frames, start, hold;
    int in, cap;
    int zoom[8], nzoom, shake[8], nshake;
    Text title, sub;
    int has_title, has_sub;
    char badge[16];
} Shot;

static Shot shots[64];
static int nshots, total;

typedef struct { int clip, first, num; } Game;
static Game games[64];
static int ngames;

static int parse_col(const char *s) {
    static const char LETTERS[] = "kndsglwmvroaycbtehqfjziNBuCIpVPK";
    if (isdigit((unsigned char)s[0])) return atoi(s) % PAL_COUNT;
    const char *p = s[0] ? strchr(LETTERS, s[0]) : NULL;
    return p ? (int)(p - LETTERS) : C_WHITE;
}

/* \xNN puts in a glyph (\x88 is a star); in cuts.txt '_' is a space */
static void unescape(char *dst, const char *src, size_t cap, int underscores) {
    size_t n = 0;
    for (const char *s = src; *s && n + 1 < cap; s++) {
        if (s[0] == '\\' && s[1] == 'x' && isxdigit((unsigned char)s[2]) && isxdigit((unsigned char)s[3])) {
            char hx[3] = {s[2], s[3], 0};
            dst[n++] = (char)strtol(hx, NULL, 16);
            s += 3;
        } else dst[n++] = underscores && *s == '_' ? ' ' : *s;
    }
    dst[n] = 0;
}

/* the captions of a clip: the overlay_text lines of tools/promo/clips/NAME_*.ufs */
static void script_captions(Shot *sh, const char *name) {
    DIR *d = opendir(scripts_dir);
    if (!d) return;
    struct dirent *e;
    size_t nl = strlen(name);
    char path[700] = {0};
    while ((e = readdir(d)))
        if (!strncmp(e->d_name, name, nl) && e->d_name[nl] == '_') {
            snprintf(path, sizeof path, "%s/%s", scripts_dir, e->d_name);
            break;
        }
    closedir(d);
    if (!path[0]) return;
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp(p, "overlay_text", 12)) continue;
        char *nlc = strpbrk(p, "\r\n");
        if (nlc) *nlc = 0;
        Text t = {0};
        char col[8] = {0};
        int used = 0;
        if (sscanf(p + 12, " %c %d %d %d %7s %n", &t.align, &t.x, &t.y, &t.scale, col, &used) < 5) continue;
        t.col = parse_col(col);
        unescape(t.text, p + 12 + used, sizeof t.text, 0);
        if (!sh->has_title) { sh->title = t; sh->has_title = 1; }
        else if (!sh->has_sub) { sh->sub = t; sh->has_sub = 1; }
    }
    fclose(f);
}

static int all_digits(const char *s) {
    if (!*s) return 0;
    for (; *s; s++)
        if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

static void add_game(int clip, int first) {
    for (int i = 0; i < ngames; i++)
        if (games[i].clip == clip) return;
    games[ngames].clip = clip;
    games[ngames].first = first;
    games[ngames].num = atoi(clips[clip].name);
    ngames++;
}

static int list(int *dst, const char *s) {
    int n = 0;
    while (*s && n < 8) {
        dst[n++] = atoi(s);
        while (*s && *s != ',') s++;
        if (*s == ',') s++;
    }
    return n;
}

static void load_cuts(void) {
    FILE *f = fopen(cuts_path, "r");
    if (!f) die("cannot open", cuts_path);
    char line[512];
    while (fgets(line, sizeof line, f)) {
        char *h = strchr(line, '#');
        if (h) *h = 0;
        char *tok[24];
        int nt = 0;
        for (char *t = strtok(line, " \t\r\n"); t && nt < 24; t = strtok(NULL, " \t\r\n")) tok[nt++] = t;
        if (!nt) continue;
        if (!strcmp(tok[0], "tile")) { /* a game shown only in the grid */
            if (nt < 3) die("bad tile line", tok[1]);
            add_game(clip_id(tok[1]), atoi(tok[2]));
            continue;
        }
        if (nt < 3 || nshots == 64) die("bad cut:", tok[0]);
        Shot *sh = &shots[nshots++];
        memset(sh, 0, sizeof *sh);
        sh->first = atoi(tok[1]);
        sh->frames = atoi(tok[2]);
        sh->kind = !strcmp(tok[0], "grid") ? K_GRID : K_CLIP;
        sh->clip = sh->kind == K_GRID ? -1 : clip_id(tok[0]);
        sh->hold = 1 << 30;
        for (int i = 3; i < nt; i++) {
            char *eq = strchr(tok[i], '=');
            if (!eq) die("bad effect:", tok[i]);
            *eq = 0;
            const char *k = tok[i], *v = eq + 1;
            if (!strcmp(k, "in")) {
                static const char *NAMES[] = {"cut", "flash", "glitch", "mosaic", "dither", "zoom"};
                sh->in = -1;
                for (int j = 0; j < 6; j++)
                    if (!strcmp(v, NAMES[j])) sh->in = j;
                if (sh->in < 0) die("unknown transition", v);
            } else if (!strcmp(k, "cap")) {
                sh->cap = !strcmp(v, "drop") ? C_DROP : !strcmp(v, "slam") ? C_SLAM : C_SLIDE;
            } else if (!strcmp(k, "zoom")) sh->nzoom = list(sh->zoom, v);
            else if (!strcmp(k, "shake")) sh->nshake = list(sh->shake, v);
            else if (!strcmp(k, "hold")) sh->hold = atoi(v);
            else if (!strcmp(k, "as")) sh->kind = !strcmp(v, "hook") ? K_HOOK : !strcmp(v, "end") ? K_END : K_CLIP;
            else if (!strcmp(k, "badge")) unescape(sh->badge, v, sizeof sh->badge, 1);
            else if (!strcmp(k, "title") || !strcmp(k, "sub")) {
                Text *t = k[0] == 't' ? &sh->title : &sh->sub;
                *t = (Text){'L', 3, k[0] == 't' ? 160 : 172, k[0] == 't' ? 1 : 0, k[0] == 't' ? C_YELLOW : C_LIGHT, ""};
                unescape(t->text, v, sizeof t->text, 1);
                if (k[0] == 't') sh->has_title = 1;
                else sh->has_sub = 1;
            } else die("unknown effect", k);
        }
        if (sh->kind == K_CLIP && !sh->has_title) script_captions(sh, tok[0]);
        if (sh->kind == K_CLIP && all_digits(tok[0])) add_game(sh->clip, sh->first);
    }
    fclose(f);
    /* the grid shows the games in slot order */
    for (int i = 1; i < ngames; i++)
        for (int j = i; j > 0 && games[j].num < games[j - 1].num; j--) {
            Game t = games[j];
            games[j] = games[j - 1];
            games[j - 1] = t;
        }
    total = 0;
    for (int i = 0; i < nshots; i++) {
        shots[i].start = total;
        total += shots[i].frames;
    }
}

static int shot_at(int f) {
    for (int i = nshots - 1; i >= 0; i--)
        if (f >= shots[i].start) return i;
    return 0;
}

/* ---- output ----------------------------------------------------------------------- */

static int W = 1920, H = 1080, S = 6, GX = 0, GY = 0, GW, GH;
static int crt = 1, end_link = 1;
static uint8_t *out;          /* W*H*3 */
static uint16_t *crt_map;     /* W*H, 256 = unchanged */
static uint8_t cap_px[SW * SH]; /* the caption layer, TRANSPARENT = none */
static Img card_top, card_bot;
static const char *card_paths[2];

static uint32_t hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}
static float rnd(uint32_t seed) { return (float)(hash(seed) & 0xffff) / 65535.0f; } /* 0..1 */

static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static float ease_out_back(float t) {
    const float c1 = 1.9f, c3 = c1 + 1;
    t -= 1;
    return 1 + c3 * t * t * t + c1 * t * t;
}

/* Everything a frame's game picture gets. */
typedef struct {
    float z, zx, zy;    /* zoom and its centre (source px) */
    int dx, dy;         /* shake, output px */
    int split;          /* RGB split, output px */
    int mosaic;         /* block size, source px */
    float flash;        /* white, 0..1 */
    const Img *other;   /* dither wipe: the other picture ... */
    float p;            /* ... how far the wipe is ... */
    int other_is_old;   /* ... and whether the other one is going away */
    int nslice, sy0[6], sy1[6], soff[6]; /* glitch slices (source rows, px) */
} FX;

static void fx_reset(FX *fx) {
    memset(fx, 0, sizeof *fx);
    fx->z = 1;
    fx->zx = SW / 2.0f;
    fx->zy = SH / 2.0f;
    fx->mosaic = 1;
}

static void glitch(FX *fx, uint32_t seed, float amount) {
    fx->split += (int)(S * (1 + 2 * amount) + 0.5f);
    fx->nslice = 2 + (int)(hash(seed) % 3);
    for (int i = 0; i < fx->nslice; i++) {
        uint32_t r = hash(seed * 7 + (uint32_t)i * 131);
        fx->sy0[i] = (int)(r % SH);
        fx->sy1[i] = fx->sy0[i] + 3 + (int)((r >> 8) % (int)(6 + 18 * amount));
        int off = 3 + (int)((r >> 16) % (int)(4 + 16 * amount));
        fx->soff[i] = (r >> 30) & 1 ? off : -off;
    }
}

static const uint8_t BAYER[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

static void draw_picture(const Img *a, const FX *fx) {
    static int *ucol[3];
    if (!ucol[0])
        for (int c = 0; c < 3; c++) ucol[c] = malloc(sizeof(int) * W);
    float sc = S * fx->z;
    float ox = GX + fx->zx * S + fx->dx, oy = GY + fx->zy * S + fx->dy;
    for (int c = 0; c < 3; c++) {
        int d = (c - 1) * -fx->split; /* red right, blue left */
        for (int X = GX; X < GX + GW; X++) ucol[c][X] = (int)floorf(fx->zx + (X + 0.5f - d - ox) / sc);
    }
    int b = fx->mosaic > 1 ? fx->mosaic : 1;
    float thresh = fx->p * 16;
    for (int Y = GY; Y < GY + GH; Y++) {
        int v = (int)floorf(fx->zy + (Y + 0.5f - oy) / sc), off = 0;
        for (int i = 0; i < fx->nslice; i++)
            if (v >= fx->sy0[i] && v < fx->sy1[i]) off += fx->soff[i];
        if (b > 1) v = v / b * b + b / 2;
        v = v < 0 ? 0 : v >= SH ? SH - 1 : v;
        uint8_t *o = out + ((size_t)Y * W + GX) * 3;
        const uint32_t *ra = a->px + v * SW, *rb = fx->other ? fx->other->px + v * SW : NULL;
        for (int X = GX; X < GX + GW; X++, o += 3) {
            for (int c = 0; c < 3; c++) {
                int u = ucol[c][X] + off;
                if (b > 1) u = u / b * b + b / 2;
                u = u < 0 ? 0 : u >= SW ? SW - 1 : u;
                const uint32_t *row = ra;
                if (rb) {
                    int in = BAYER[(v >> 1) & 3][(u >> 1) & 3] < thresh;
                    if (fx->other_is_old ? !in : in) row = rb;
                }
                uint32_t px = row[u];
                if (!fx->split && !rb) { /* one fetch does all three */
                    o[0] = (uint8_t)(px >> 16);
                    o[1] = (uint8_t)(px >> 8);
                    o[2] = (uint8_t)px;
                    break;
                }
                o[c] = (uint8_t)(px >> (16 - 8 * c));
            }
        }
    }
}

static void fill_rect(int x0, int y0, int w, int h, uint32_t rgb) {
    for (int y = y0 < 0 ? 0 : y0; y < y0 + h && y < H; y++)
        for (int x = x0 < 0 ? 0 : x0; x < x0 + w && x < W; x++) {
            uint8_t *o = out + ((size_t)y * W + x) * 3;
            o[0] = (uint8_t)(rgb >> 16);
            o[1] = (uint8_t)(rgb >> 8);
            o[2] = (uint8_t)rgb;
        }
}

static uint32_t pal_rgb(int c) { return (uint32_t)PALETTE_RGB[c][0] << 16 | (uint32_t)PALETTE_RGB[c][1] << 8 | PALETTE_RGB[c][2]; }

static void draw_caps(void) {
    for (int cy = 0; cy < SH; cy++)
        for (int cx = 0; cx < SW; cx++) {
            int c = cap_px[cy * SW + cx];
            if (c != TRANSPARENT) fill_rect(GX + cx * S, GY + cy * S, S, S, pal_rgb(c));
        }
}

static void flash_rect(int x0, int y0, int w, int h, float a) {
    if (a <= 0) return;
    int k = (int)(clampf(a, 0, 1) * 256);
    for (int y = y0; y < y0 + h; y++) {
        uint8_t *o = out + ((size_t)y * W + x0) * 3;
        for (int i = 0; i < w * 3; i++) o[i] = (uint8_t)(o[i] + ((255 - o[i]) * k >> 8));
    }
}

static void draw_card(const Img *c, int x0, int y0) {
    for (int y = 0; y < c->h * S; y++) {
        int yy = y0 + y;
        if (yy < 0 || yy >= H) continue;
        for (int x = 0; x < c->w * S; x++) {
            int xx = x0 + x;
            if (xx < 0 || xx >= W) continue;
            uint32_t px = c->px[(y / S) * c->w + x / S];
            uint8_t *o = out + ((size_t)yy * W + xx) * 3;
            o[0] = (uint8_t)(px >> 16);
            o[1] = (uint8_t)(px >> 8);
            o[2] = (uint8_t)px;
        }
    }
}

static void build_crt(void) {
    crt_map = malloc(sizeof(uint16_t) * W * H);
    float line = S >= 6 ? 0.84f : S >= 3 ? 0.9f : 1.0f;
    for (int Y = 0; Y < H; Y++)
        for (int X = 0; X < W; X++) {
            float nx = (X + 0.5f - W / 2.0f) / (W / 2.0f), ny = (Y + 0.5f - H / 2.0f) / (H / 2.0f);
            float r2 = nx * nx + ny * ny, v = 1 - 0.26f * clampf((r2 - 0.35f) / 1.65f, 0, 1);
            int in = X >= GX && X < GX + GW && Y >= GY && Y < GY + GH;
            if (in && (Y - GY) % S == S - 1) v *= line;
            crt_map[Y * W + X] = (uint16_t)(v * 256 + 0.5f);
        }
}

/* ---- captions and words (in the 320x180 caption layer) ------------------------- */

static void text_at(const char *s, int x, int y, int scale, int col, int plate) {
    int w = scale ? text_width_scaled(s, scale) : tiny_width(s), h = scale ? FONT_H * scale : 5;
    if (plate) gfx_rect(x - 3, y - 2, w + 6, h + 4, C_INK);
    for (int k = 0; k < 10; k++) {
        if (k == 4) continue;
        int dx = k < 9 ? k % 3 - 1 : 0, dy = k < 9 ? k / 3 - 1 : 0, c = k < 9 ? C_INK : col;
        if (scale) text_draw_scaled(s, x + dx, y + dy, c, scale);
        else tiny_draw(s, x + dx, y + dy, c);
    }
}

static int text_w(const char *s, int scale) { return scale ? text_width_scaled(s, scale) : tiny_width(s); }

/* the first n characters, and a cursor block while it types */
static void typed(char *dst, const char *s, int n, int cursor) {
    int len = (int)strlen(s);
    if (n < 0) n = 0;
    if (n > len) n = len;
    memcpy(dst, s, (size_t)n);
    dst[n] = 0;
    if (cursor && n < len) strcat(dst, "_");
}

/* a caption line that comes in by STYLE at frame l (0 = it starts) */
static void kinetic(const Text *t, int l, int style, int flash_col) {
    if (l < 0) return;
    int scale = t->scale, x = t->x, y = t->y, col = t->col;
    int w = text_w(t->text, scale), h = scale ? FONT_H * scale : 5;
    char a = (char)tolower((unsigned char)t->align);
    int plate = isupper((unsigned char)t->align);
    int top = y < SH / 2;
    if (style == C_SLIDE) {
        float k = l >= 10 ? 1 : ease_out_back(l / 10.0f);
        int from = a == 'r' ? SW + 8 : -w - 8;
        int tx = a == 'c' ? x - w / 2 : a == 'r' ? x - w : x;
        x = (int)lroundf(from + (tx - from) * k);
        a = 'l';
    } else if (style == C_DROP) {
        float k = l >= 10 ? 1 : ease_out_back(l / 10.0f);
        int from = top ? -h - 6 : SH + 6;
        y = (int)lroundf(from + (y - from) * k);
    } else if (style == C_SLAM && scale && l < 6) {
        int big = scale + (l < 2 ? 2 : l < 4 ? 1 : 0);
        while (big > scale && text_w(t->text, big) > SW - 8) big--; /* never wider than the screen */
        if (big != scale) {
            int bw = text_w(t->text, big);
            if (!top) y -= FONT_H * (big - scale); /* grows up from its baseline */
            if (a == 'c') x -= (bw - w) / 2;
            else if (a == 'r') x -= bw - w;
            scale = big;
            w = bw;
        } else if (l < 6) x += (l & 1) ? 1 : -1; /* a little rattle as it lands */
    }
    if (l < 14 && (l / 2) % 2 == 0) col = flash_col;
    int dx = a == 'c' ? x - w / 2 : a == 'r' ? x - w : x;
    text_at(t->text, dx, y, scale, col, plate);
}

static void typing(const Text *t, int l, int per) {
    if (l < 0) return;
    char buf[160];
    typed(buf, t->text, l * per / 2, (l / 4) % 2 == 0);
    char a = (char)tolower((unsigned char)t->align);
    int w = text_w(t->text, t->scale); /* the whole line's place, so it doesn't creep */
    int x = a == 'c' ? t->x - w / 2 : a == 'r' ? t->x - w : t->x;
    if (isupper((unsigned char)t->align) && buf[0]) { /* the plate grows with the text */
        int bw = text_w(buf, t->scale), h = t->scale ? FONT_H * t->scale : 5;
        gfx_rect(x - 3, t->y - 2, bw + 6, h + 4, C_INK);
    }
    text_at(buf, x, t->y, t->scale, t->col, 0);
}

/* a plus-shaped twinkle, arms r long */
static void sparkle(int x, int y, int r, int col) {
    if (r <= 0) return;
    gfx_pset(x, y, C_WHITE);
    for (int i = 1; i <= r; i++) {
        int c = i == r ? col : C_WHITE;
        gfx_pset(x - i, y, c);
        gfx_pset(x + i, y, c);
        gfx_pset(x, y - i, c);
        gfx_pset(x, y + i, c);
    }
}

/* stars that twinkle to the beat round the logo, from frame l0 */
static void twinkles(int l, int l0, int seed) {
    static const int P[][2] = {{22, 30}, {298, 26}, {40, 108}, {284, 112}, {72, 18}, {250, 40},
                               {12, 70}, {308, 80}, {128, 16}, {196, 22}, {30, 150}, {292, 154}};
    static const int COLS[] = {C_YELLOW, C_CYAN, C_WHITE, C_PINK};
    for (int i = 0; i < 12; i++) {
        int t = l - l0 - i * 6 - seed;
        if (t < 0) continue;
        int ph = t % 48; /* each lives half a bar, then waits half a bar */
        int r = ph < 3 ? ph : ph < 18 ? 3 - (ph - 3) / 5 : 0;
        sparkle(P[i][0], P[i][1], r, COLS[(i + t / 48) % 4]);
    }
}

/* a burst of stars from (cx, cy) at frame l0 */
static void star_burst(int l, int l0, int cx, int cy) {
    int t = l - l0;
    if (t < 0 || t >= 40) return;
    static const int COLS[] = {C_YELLOW, C_WHITE, C_CYAN, C_ORANGE, C_PINK};
    for (int i = 0; i < 32; i++) {
        float ang = i * 6.2831853f / 32 + rnd((uint32_t)i * 17) * 0.3f;
        float sp = 2.2f + 3.0f * rnd((uint32_t)i * 29 + 5);
        float d = sp * (t - t * t / 90.0f) * 1.2f; /* slows as it goes */
        int x = cx + (int)lroundf(cosf(ang) * d * 1.6f), y = cy + (int)lroundf(sinf(ang) * d);
        if (t > 30 && (t + i) % 2) continue; /* flickers out */
        int c = COLS[(i + t / 4) % 5];
        if (t < 12) sparkle(x, y, 1, c);
        else gfx_pset(x, y, c);
    }
}

/* ---- the shots ------------------------------------------------------------------------ */

/* zoom punches, shakes and the like at frame l of a shot */
static void punch(FX *fx, int l, int at, float amount, uint32_t seed) {
    int t = l - at;
    if (t < 0 || t >= 18) return;
    float e = t == 0 ? 0.6f : t == 1 ? 1 : powf(1 - (t - 1) / 17.0f, 2.2f);
    fx->z += amount * e;
    if (t < 3) fx->split += S * (3 - t) / 3;
    if (t < 2) fx->flash += t ? 0.12f : 0.3f;
    (void)seed;
}

static void shake(FX *fx, int l, int at, float amp, uint32_t seed) {
    int t = l - at;
    if (t < 0 || t >= 14) return;
    float a = amp * S * powf(1 - t / 14.0f, 1.5f);
    fx->dx += (int)lroundf(a * (rnd(seed + (uint32_t)t * 3) * 2 - 1));
    fx->dy += (int)lroundf(a * 0.7f * (rnd(seed + (uint32_t)t * 3 + 1) * 2 - 1));
}

/* the way into shot i (at k frames after its cut, or -k before it) */
static void transition(FX *fx, int i, int k, const Img **other, float *p, int *old) {
    const Shot *sh = &shots[i];
    uint32_t seed = (uint32_t)i * 977 + (uint32_t)(k + 50);
    switch (sh->in) {
    case T_FLASH:
        if (k >= 0 && k < 5) fx->flash += (float[]){1, 0.8f, 0.55f, 0.3f, 0.12f}[k];
        break;
    case T_GLITCH:
        if (k >= -3 && k < 4) {
            glitch(fx, seed, k < 0 ? (4 + k) / 3.0f : (4 - k) / 4.0f);
            if (k == 0) fx->flash += 0.25f;
            if (k == -1 || k == 1) fx->mosaic = 2;
        }
        break;
    case T_MOSAIC:
        if (k >= -4 && k < 0) fx->mosaic = (int[]){16, 8, 4, 2}[k + 4];
        if (k >= 0 && k < 5) fx->mosaic = (int[]){16, 16, 8, 4, 2}[k];
        break;
    case T_DITHER:
        if (k >= -6 && k < 6 && i > 0) {
            const Shot *a = &shots[i - 1];
            float pp = (k + 6 + 0.5f) / 12.0f;
            if (k < 0) {
                *other = frame_get(sh->clip >= 0 ? sh->clip : a->clip, sh->first);
                *p = pp;
                *old = 0;
            } else {
                *other = frame_get(a->clip >= 0 ? a->clip : sh->clip, a->first + a->frames - 1);
                *p = pp;
                *old = 1;
            }
        }
        break;
    case T_ZOOM:
        if (k >= 0 && k < 12) fx->z += 0.3f * powf(1 - k / 12.0f, 2);
        if (k == 0) fx->flash += 0.4f;
        break;
    }
}

/* the clean sky of the boot, for under the words (the boot's own lines go) */
static Img sky;

static const Img *boot_frame(int clip, int frame) {
    const Img *src = frame_get(clip, frame);
    static Img mix;
    if (!sky.px) {
        const Img *f0 = frame_get(clip, 0);
        sky.w = SW;
        sky.h = SH;
        sky.px = malloc(sizeof(uint32_t) * SW * SH);
        for (int y = 0; y < SH; y++) memcpy(sky.px + y * SW, f0->px + (y >= 164 ? y - 56 : y) * SW, SW * 4);
        mix.w = SW;
        mix.h = SH;
        mix.px = malloc(sizeof(uint32_t) * SW * SH);
    }
    memcpy(mix.px, src->px, 108 * SW * 4);
    memcpy(mix.px + 108 * SW, sky.px + 108 * SW, (SH - 108) * SW * 4);
    return &mix;
}

static char games_txt[8];

static void hook_words(int l) {
    /* on ink plates (the sky's own ink: they only hide its stars) */
    Text t1 = {'C', 160, 114, 2, C_YELLOW, "A UFO 50 TRIBUTE"};
    Text t2 = {'C', 160, 136, 1, C_WHITE, "FOR PS VITA & PC"};
    Text t3 = {'C', 160, 114, 2, C_YELLOW, "ONE PRETEND 1980S CONSOLE"};
    Text t4 = {'C', 160, 136, 1, C_CYAN, ""};
    snprintf(t4.text, sizeof t4.text, "50 SLOTS \x88 %s GAMES SO FAR", games_txt);
    if (l < 94) {
        kinetic(&t1, l - 10, C_SLAM, C_WHITE);
        typing(&t2, l - 22, 1);
    } else if (l >= 98) {
        kinetic(&t3, l - 98, C_SLAM, C_WHITE);
        typing(&t4, l - 108, 2);
    }
    star_burst(l, 8, 160, 78);
}

static void end_words(int l) {
    Text fan = {'C', 160, 170, 0, C_GREY, "FAN-MADE TRIBUTE \x8a NOT AFFILIATED WITH MOSSMOUTH"};
    Text count = {'C', 160, 150, 1, C_WHITE, ""};
    snprintf(count.text, sizeof count.text, "%s GAMES \x88 50 SLOTS", games_txt);
    if (end_link) {
        Text t1 = {'C', 160, 112, 2, C_YELLOW, "CHECK THE REPO"};
        Text t2 = {'C', 160, 134, 1, C_CYAN, "GITHUB.COM/NANIIIC137/UFO-40"};
        kinetic(&t1, l - 72, C_SLAM, C_WHITE);
        typing(&t2, l - 84, 2);
        kinetic(&count, l - 120, C_SLIDE, C_YELLOW);
    } else {
        count.y = 120;
        count.scale = 2;
        count.col = C_YELLOW;
        kinetic(&count, l - 72, C_SLAM, C_WHITE);
    }
    typing(&fan, l - 144, 4);
    twinkles(l, 50, 0);
    star_burst(l, 48, 160, 78);
}

static void clip_words(const Shot *sh, int l) {
    if (sh->has_title) kinetic(&sh->title, l - 1, sh->cap, C_WHITE);
    if (sh->has_sub) typing(&sh->sub, l - 9, 2);
    if (sh->badge[0]) { /* a sticker, bottom right, that bounces on the beat */
        int t = l - 2, beat = l % BEAT;
        if (t >= 0) {
            int scale = t < 2 ? 5 : t < 4 ? 4 : 3;
            static const int COLS[] = {C_YELLOW, C_ORANGE, C_MAGENTA, C_CYAN};
            int w = text_width_scaled(sh->badge, scale);
            int y = 150 - FONT_H * (scale - 3) - (beat < 3 ? 3 - beat : 0);
            text_at(sh->badge, 314 - w, y, scale, COLS[(l / 6) % 4], 1);
        }
    }
}

/* the grid: all the games at once, popping in on the sixteenths while a
 * counter runs up, then drawn up into the saucer */
#define POP 6
static void draw_grid(const Shot *sh, int l, FX *fx) {
    int n = S % 3 == 0 ? 3 : 2, T = n * n, ts = S / n, gw = SW * n, gh = SH * n;
    float e = clampf((l - (sh->frames - 26)) / 20.0f, 0, 1), e2 = e * e;
    float k = (1 - e2) * fx->z;
    float cx = GX + GW / 2.0f + fx->dx, cy = GY + GH / 2.0f + fx->dy;
    cx += (GX + 160 * S - cx) * e2; /* towards the saucer, top middle */
    cy += (GY + 22 * S - cy) * e2;
    fill_rect(GX, GY, GW, GH, pal_rgb(C_INK));
    if (k < 0.01f) return;
    const Img *img[9] = {0};
    float tf[9] = {0};
    for (int t = 0; t < T; t++) {
        int last = -1;
        for (int g = t; g < ngames; g += T)
            if (g * POP <= l) last = g;
        if (last < 0) continue;
        int since = l - last * POP;
        img[t] = frame_get(games[last].clip, games[last].first + since);
        if (since < 5) tf[t] = (float[]){1, 0.7f, 0.45f, 0.25f, 0.1f}[since];
    }
    float sc = ts * k;
    for (int Y = GY; Y < GY + GH; Y++) {
        int gy = (int)floorf((Y + 0.5f - cy) / sc + gh / 2.0f);
        uint8_t *o = out + ((size_t)Y * W + GX) * 3;
        if (gy < 0 || gy >= gh) continue;
        int tr = gy / SH, ty = gy % SH;
        for (int X = GX; X < GX + GW; X++, o += 3) {
            int gx = (int)floorf((X + 0.5f - cx) / sc + gw / 2.0f);
            if (gx < 0 || gx >= gw) continue;
            int tc = gx / SW, tx = gx % SW, t = tr * n + tc;
            if (tx < 2 || tx >= SW - 2 || ty < 2 || ty >= SH - 2 || !img[t]) {
                if (img[t] && e2 < 1) { o[0] = 0x0e; o[1] = 0x0b; o[2] = 0x16; }
                else { o[0] = 0x1f; o[1] = 0x1a; o[2] = 0x33; }
                continue;
            }
            uint32_t px = img[t]->px[ty * SW + tx];
            int r = (int)(px >> 16), g = (int)(px >> 8) & 255, b = (int)px & 255, a = (int)(tf[t] * 256);
            o[0] = (uint8_t)(r + ((255 - r) * a >> 8));
            o[1] = (uint8_t)(g + ((255 - g) * a >> 8));
            o[2] = (uint8_t)(b + ((255 - b) * a >> 8));
        }
    }
}

static void grid_words(const Shot *sh, int l) {
    int shown = l / POP + 1, pop = l % POP;
    if (shown > ngames) { shown = ngames; pop = 99; }
    if (l >= sh->frames - 8) return;
    char num[16];
    snprintf(num, sizeof num, "%02d", shown);
    int scale = pop == 0 ? 8 : pop == 1 ? 7 : 6;
    if (shown == ngames && pop == 99) scale = (l % BEAT) < 2 ? 7 : 6; /* pulses on the beat */
    int w = text_width_scaled(num, scale), h = FONT_H * scale;
    int y = 86 - h / 2 - 8;
    int col = shown == ngames ? ((l / 4) % 2 ? C_YELLOW : C_WHITE) : pop < 2 ? C_WHITE : C_YELLOW;
    gfx_rect(160 - w / 2 - 6, y - 5, w + 12, h + 10 + 22, C_INK);
    text_at(num, 160 - w / 2, y, scale, col, 0);
    text_at("GAMES", 160 - text_width_scaled("GAMES", 2) / 2, y + h + 6, 2, C_CYAN, 0);
}

/* ---- one frame ------------------------------------------------------------------------- */

static void render(int f) {
    int i = shot_at(f);
    const Shot *sh = &shots[i];
    int l = f - sh->start;
    FX fx;
    fx_reset(&fx);
    const Img *other = NULL;
    float p = 0;
    int old = 0;
    transition(&fx, i, l, &other, &p, &old);
    if (i + 1 < nshots) transition(&fx, i + 1, f - shots[i + 1].start, &other, &p, &old);
    fx.other = other;
    fx.p = p;
    fx.other_is_old = old;

    for (int z = 0; z < sh->nzoom; z++) punch(&fx, l, sh->zoom[z], 0.08f, (uint32_t)f);
    for (int s = 0; s < sh->nshake; s++) shake(&fx, l, sh->shake[s], 2.2f, (uint32_t)(sh->start + sh->shake[s]) * 31);

    gfx_set_target(&(Surface){SW, SH, cap_px});
    gfx_cls(TRANSPARENT);

    if (card_paths[0]) {
        fill_rect(0, 0, W, H, pal_rgb(C_INK));
        draw_card(&card_top, GX, GY - GH - 60 * S / 3);
        draw_card(&card_bot, GX, GY + GH + 60 * S / 3);
    }

    if (sh->kind == K_HOOK) {
        /* the glitchy reveal: the logo comes up out of a mosaic and an RGB
         * split, then locks in with a flash, a punch and a burst of stars;
         * the words swap on a glitch at the bar line */
        if (l < 8) {
            fx.mosaic = (int[]){4, 4, 3, 3, 2, 2, 1, 1}[l];
            glitch(&fx, (uint32_t)l * 13 + 7, (8 - l) / 8.0f);
        }
        if (l >= 8 && l < 13) fx.flash += (float[]){0.7f, 0.45f, 0.25f, 0.1f, 0.04f}[l - 8];
        punch(&fx, l, 8, 0.08f, 0);
        shake(&fx, l, 8, 2.0f, 991);
        if (l >= 44 && l < 47) glitch(&fx, (uint32_t)l * 23, 0.3f);
        if (l >= 93 && l < 99) glitch(&fx, (uint32_t)l * 29, 0.8f);
        for (int b = 48; b < sh->frames; b += BEAT)
            if (b != 96) punch(&fx, l, b, 0.025f, 0);
        draw_picture(boot_frame(sh->clip, sh->first + l < sh->hold ? sh->first + l : sh->hold), &fx);
        hook_words(l);
    } else if (sh->kind == K_END) {
        if (l >= 48 && l < 53) fx.flash += (float[]){0.9f, 0.6f, 0.35f, 0.15f, 0.05f}[l - 48];
        if (l >= 44 && l < 48) glitch(&fx, (uint32_t)l * 31, 0.3f);
        punch(&fx, l, 48, 0.1f, 0);
        shake(&fx, l, 48, 2.5f, 1234);
        for (int b = 96; b < sh->frames; b += BAR) punch(&fx, l, b, 0.03f, 0);
        draw_picture(boot_frame(sh->clip, sh->first + l < sh->hold ? sh->first + l : sh->hold), &fx);
        end_words(l);
    } else if (sh->kind == K_GRID) {
        for (int b = BEAT * 6; b < sh->frames - 26; b += BEAT) punch(&fx, l, b, 0.03f, 0);
        draw_grid(sh, l, &fx);
        grid_words(sh, l);
        if (l >= sh->frames - 4) fx.flash += (l - (sh->frames - 4) + 1) * 0.2f;
    } else {
        const Img *a = frame_get(sh->clip, sh->first + l < sh->hold ? sh->first + l : sh->hold);
        draw_picture(a, &fx);
        clip_words(sh, l);
    }
    gfx_set_target(NULL);
    draw_caps();
    flash_rect(GX, GY, GW, GH, fx.flash);
    if (crt)
        for (size_t k = 0, n = (size_t)W * H; k < n; k++) {
            uint16_t m = crt_map[k];
            if (m == 256) continue;
            uint8_t *o = out + k * 3;
            o[0] = (uint8_t)(o[0] * m >> 8);
            o[1] = (uint8_t)(o[1] * m >> 8);
            o[2] = (uint8_t)(o[2] * m >> 8);
        }
}

/* ---- the sound ------------------------------------------------------------------------ */

static void write_wav(const char *path) {
    long n = (long)total * 800;
    float *mix = calloc((size_t)n * 2, sizeof(float));
    for (int i = 0; i < nshots; i++) {
        const Shot *sh = &shots[i];
        long at = (long)sh->start * 800;
        if (sh->kind == K_GRID) {
            /* a rising blip for each game, and a swoop up into the saucer */
            for (int g = 0; g < ngames; g++) {
                float fq = 660 * powf(2, g / 12.0f);
                long s0 = at + (long)g * POP * 800;
                for (int k = 0; k < 2400 && s0 + k < n; k++) {
                    float env = expf(-k / 500.0f) * 0.1f;
                    float v = fmodf(k * fq / 48000.0f, 1) < 0.5f ? env : -env;
                    mix[(s0 + k) * 2] += v;
                    mix[(s0 + k) * 2 + 1] += v;
                }
            }
            long s0 = at + (long)(sh->frames - 26) * 800, len = 26 * 800;
            float ph = 0;
            for (long k = 0; k < len && s0 + k < n; k++) {
                float t = (float)k / len, fq = 180 + 1800 * t * t;
                ph += fq / 48000.0f;
                float env = 0.07f * (t < 0.1f ? t * 10 : 1) * (1 - t * 0.3f);
                float v = fmodf(ph, 1) < 0.5f ? env : -env;
                mix[(s0 + k) * 2] += v;
                mix[(s0 + k) * 2 + 1] += v;
            }
            continue;
        }
        Clip *c = &clips[sh->clip];
        clip_wav(c);
        if (c->wav_n <= 0) continue;
        for (long k = 0; k < (long)sh->frames * 800; k++) {
            long src = (long)sh->first * 800 + k;
            if (src >= c->wav_n) break;
            mix[(at + k) * 2] += c->wav[src * 2] / 32768.0f;
            mix[(at + k) * 2 + 1] += c->wav[src * 2 + 1] / 32768.0f;
        }
    }
    FILE *f = fopen(path, "wb");
    if (!f) die("cannot write", path);
    uint32_t bytes = (uint32_t)n * 4, riff = bytes + 36;
    uint8_t h[44] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0,
                     0x80, 0xBB, 0, 0, 0x00, 0xEE, 0x02, 0, 4, 0, 16, 0, 'd', 'a', 't', 'a', 0, 0, 0, 0};
    memcpy(h + 4, &riff, 4);
    memcpy(h + 40, &bytes, 4);
    fwrite(h, 1, 44, f);
    for (long k = 0; k < n * 2; k++) {
        float v = clampf(mix[k], -1, 1) * 32767;
        int16_t s = (int16_t)lroundf(v);
        fwrite(&s, 2, 1, f);
    }
    fclose(f);
    free(mix);
}

/* ---- main ------------------------------------------------------------------------------ */

int main(int argc, char **argv) {
    const char *wav = NULL;
    int info = 0, step = 1, nr = 0, ra[8], rb[8];
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-clips") && i + 1 < argc) clips_dir = argv[++i];
        else if (!strcmp(a, "-cuts") && i + 1 < argc) cuts_path = argv[++i];
        else if (!strcmp(a, "-scripts") && i + 1 < argc) scripts_dir = argv[++i];
        else if (!strcmp(a, "-size") && i + 5 < argc) {
            W = atoi(argv[++i]);
            H = atoi(argv[++i]);
            S = atoi(argv[++i]);
            GX = atoi(argv[++i]);
            GY = atoi(argv[++i]);
        } else if (!strcmp(a, "-crt") && i + 1 < argc) crt = atoi(argv[++i]);
        else if (!strcmp(a, "-end") && i + 1 < argc) end_link = strcmp(argv[++i], "nolink") != 0;
        else if (!strcmp(a, "-range") && i + 2 < argc && nr < 8) {
            ra[nr] = atoi(argv[++i]);
            rb[nr++] = atoi(argv[++i]);
        } else if (!strcmp(a, "-step") && i + 1 < argc) step = atoi(argv[++i]);
        else if (!strcmp(a, "-cards") && i + 2 < argc) {
            card_paths[0] = argv[++i];
            card_paths[1] = argv[++i];
        } else if (!strcmp(a, "-wav") && i + 1 < argc) wav = argv[++i];
        else if (!strcmp(a, "-info")) info = 1;
        else die("unknown option", a);
    }
    font_init();
    load_cuts();
    snprintf(games_txt, sizeof games_txt, "%d", ngames);
    if (info) {
        printf("%d shots, %d frames (%.2f s), %d games\n", nshots, total, total / 60.0, ngames);
        for (int i = 0; i < nshots; i++)
            printf("  %5d  %-8s %4d +%d\n", shots[i].start, shots[i].clip >= 0 ? clips[shots[i].clip].name : "grid",
                   shots[i].first, shots[i].frames);
        return 0;
    }
    if (wav) {
        write_wav(wav);
        return 0;
    }
    GW = SW * S;
    GH = SH * S;
    if (GX + GW > W || GY + GH > H || S < 1) die("the game doesn't fit the frame", NULL);
    out = calloc((size_t)W * H * 3, 1);
    if (crt) build_crt();
    if (card_paths[0]) {
        png_load(card_paths[0], &card_top);
        png_load(card_paths[1], &card_bot);
    }
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    static char obuf[1 << 20];
    setvbuf(stdout, obuf, _IOFBF, sizeof obuf);
    if (!nr) {
        ra[0] = 0;
        rb[nr++] = total;
    }
    for (int r = 0; r < nr; r++)
        for (int f = ra[r]; f < rb[r] && f < total; f += step) {
            render(f);
            if (fwrite(out, 3, (size_t)W * H, stdout) != (size_t)W * H) return 1;
        }
    fflush(stdout);
    return 0;
}
