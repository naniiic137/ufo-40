/* UFO 40 - bitmap fonts. All glyphs drawn for this project. */
#include "font.h"
#include "gfx.h"
#include <string.h>
#include <ctype.h>

/* 5x7 main font, rows separated by '|'. ASCII 32..126. */
static const char *GLYPHS[95] = {
    /* ' ' */ ".....|.....|.....|.....|.....|.....|.....",
    /* !   */ "#|#|#|#|#|.|#",
    /* "   */ "#.#|#.#|...|...|...|...|...",
    /* #   */ ".#.#.|.#.#.|#####|.#.#.|#####|.#.#.|.#.#.",
    /* $   */ "..#..|.####|#.#..|.###.|..#.#|####.|..#..",
    /* %   */ "##..#|##..#|...#.|..#..|.#...|#..##|#..##",
    /* &   */ ".##..|#..#.|#.#..|.#...|#.#.#|#..#.|.##.#",
    /* '   */ "#|#|.|.|.|.|.",
    /* (   */ ".#|#.|#.|#.|#.|#.|.#",
    /* )   */ "#.|.#|.#|.#|.#|.#|#.",
    /* *   */ ".....|#.#.#|.###.|#####|.###.|#.#.#|.....",
    /* +   */ ".....|..#..|..#..|#####|..#..|..#..|.....",
    /* ,   */ "..|..|..|..|..|.#|#.",
    /* -   */ "....|....|....|####|....|....|....",
    /* .   */ ".|.|.|.|.|.|#",
    /* /   */ "....#|....#|...#.|..#..|.#...|#....|#....",
    /* 0   */ ".###.|#...#|#..##|#.#.#|##..#|#...#|.###.",
    /* 1   */ "..#..|.##..|..#..|..#..|..#..|..#..|.###.",
    /* 2   */ ".###.|#...#|....#|..##.|.#...|#....|#####",
    /* 3   */ ".###.|#...#|....#|..##.|....#|#...#|.###.",
    /* 4   */ "...#.|..##.|.#.#.|#..#.|#####|...#.|...#.",
    /* 5   */ "#####|#....|####.|....#|....#|#...#|.###.",
    /* 6   */ "..##.|.#...|#....|####.|#...#|#...#|.###.",
    /* 7   */ "#####|....#|...#.|..#..|.#...|.#...|.#...",
    /* 8   */ ".###.|#...#|#...#|.###.|#...#|#...#|.###.",
    /* 9   */ ".###.|#...#|#...#|.####|....#|...#.|.##..",
    /* :   */ ".|.|#|.|.|#|.",
    /* ;   */ "..|..|.#|..|..|.#|#.",
    /* <   */ "...#|..#.|.#..|#...|.#..|..#.|...#",
    /* =   */ "....|....|####|....|####|....|....",
    /* >   */ "#...|.#..|..#.|...#|..#.|.#..|#...",
    /* ?   */ ".###.|#...#|....#|...#.|..#..|.....|..#..",
    /* @   */ ".###.|#...#|#.###|#.#.#|#.###|#....|.####",
    /* A   */ ".###.|#...#|#...#|#####|#...#|#...#|#...#",
    /* B   */ "####.|#...#|#...#|####.|#...#|#...#|####.",
    /* C   */ ".###.|#...#|#....|#....|#....|#...#|.###.",
    /* D   */ "###..|#..#.|#...#|#...#|#...#|#..#.|###..",
    /* E   */ "#####|#....|#....|####.|#....|#....|#####",
    /* F   */ "#####|#....|#....|####.|#....|#....|#....",
    /* G   */ ".###.|#...#|#....|#.###|#...#|#...#|.####",
    /* H   */ "#...#|#...#|#...#|#####|#...#|#...#|#...#",
    /* I   */ "###|.#.|.#.|.#.|.#.|.#.|###",
    /* J   */ "..###|...#.|...#.|...#.|...#.|#..#.|.##..",
    /* K   */ "#...#|#..#.|#.#..|##...|#.#..|#..#.|#...#",
    /* L   */ "#....|#....|#....|#....|#....|#....|#####",
    /* M   */ "#...#|##.##|#.#.#|#.#.#|#...#|#...#|#...#",
    /* N   */ "#...#|#...#|##..#|#.#.#|#..##|#...#|#...#",
    /* O   */ ".###.|#...#|#...#|#...#|#...#|#...#|.###.",
    /* P   */ "####.|#...#|#...#|####.|#....|#....|#....",
    /* Q   */ ".###.|#...#|#...#|#...#|#.#.#|#..#.|.##.#",
    /* R   */ "####.|#...#|#...#|####.|#.#..|#..#.|#...#",
    /* S   */ ".###.|#...#|#....|.###.|....#|#...#|.###.",
    /* T   */ "#####|..#..|..#..|..#..|..#..|..#..|..#..",
    /* U   */ "#...#|#...#|#...#|#...#|#...#|#...#|.###.",
    /* V   */ "#...#|#...#|#...#|#...#|#...#|.#.#.|..#..",
    /* W   */ "#...#|#...#|#...#|#.#.#|#.#.#|##.##|#...#",
    /* X   */ "#...#|#...#|.#.#.|..#..|.#.#.|#...#|#...#",
    /* Y   */ "#...#|#...#|.#.#.|..#..|..#..|..#..|..#..",
    /* Z   */ "#####|....#|...#.|..#..|.#...|#....|#####",
    /* [   */ "##|#.|#.|#.|#.|#.|##",
    /* \   */ "#....|#....|.#...|..#..|...#.|....#|....#",
    /* ]   */ "##|.#|.#|.#|.#|.#|##",
    /* ^   */ "..#..|.#.#.|#...#|.....|.....|.....|.....",
    /* _   */ ".....|.....|.....|.....|.....|.....|#####",
    /* `   */ "#.|.#|..|..|..|..|..",
    /* a-z map to upper case; placeholders never used */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* {   */ "..#|.#.|.#.|#..|.#.|.#.|..#",
    /* |   */ "#|#|#|#|#|#|#",
    /* }   */ "#..|.#.|.#.|..#|.#.|.#.|#..",
    /* ~   */ ".....|.....|.#...|#.#.#|...#.|.....|.....",
};

/* Icon glyphs 0x80.. */
static const char *ICONS[16] = {
    /* A btn */ ".#####.|###.###|##.#.##|##...##|##.#.##|##.#.##|.#####.",
    /* B btn */ ".#####.|##..###|##.#.##|##..###|##.#.##|##..###|.#####.",
    /* up    */ ".......|...#...|..###..|.#####.|#######|.......|.......",
    /* down  */ ".......|#######|.#####.|..###..|...#...|.......|.......",
    /* left  */ "...#|..##|.###|####|.###|..##|...#",
    /* right */ "#...|##..|###.|####|###.|##..|#...",
    /* heart */ ".##.##.|#######|#######|.#####.|..###..|...#...|.......",
    /* coin  */ ".###.|#...#|#.#.#|#.#.#|#.#.#|#...#|.###.",
    /* star  */ "...#...|...#...|#######|.#####.|..###..|.##.##.|.#...#.",
    /* dpad  */ "..###..|..#.#..|###.###|#.....#|###.###|..#.#..|..###..",
    /* dot   */ "...|...|###|###|###|...|...",
    /* lock  */ ".###.|#...#|#...#|#####|##.##|##.##|#####",
    /* skull */ ".###.|#####|#.#.#|#####|.###.|.#.#.|.....",
    /* note  */ "..###|..#.#|..#.#|..#..|###..|###..|.....",
    /* check */ "......#|.....##|#...##.|##.##..|.###...|..#....|.......",
    /* cross */ ".....|#...#|.#.#.|..#..|.#.#.|#...#|.....",
};

typedef struct {
    uint8_t w;
    uint8_t rows[7]; /* bit i = column i */
} Glyph;

static Glyph glyphs[128 + 16];
static int font_ready;

static void build(Glyph *g, const char *src, int trim) {
    char grid[7][8];
    memset(grid, '.', sizeof grid);
    int row = 0, col = 0, maxw = 0;
    for (const char *p = src; *p && row < 7; p++) {
        if (*p == '|') { row++; col = 0; continue; }
        if (col < 8) grid[row][col] = *p;
        col++;
        if (col > maxw) maxw = col;
    }
    int x0 = 0, x1 = maxw - 1;
    if (trim) {
        while (x0 <= x1) {
            int any = 0;
            for (int r = 0; r < 7; r++) any |= grid[r][x0] == '#';
            if (any) break;
            x0++;
        }
        while (x1 >= x0) {
            int any = 0;
            for (int r = 0; r < 7; r++) any |= grid[r][x1] == '#';
            if (any) break;
            x1--;
        }
    }
    if (x1 < x0) { g->w = 3; memset(g->rows, 0, 7); return; }
    g->w = (uint8_t)(x1 - x0 + 1);
    for (int r = 0; r < 7; r++) {
        g->rows[r] = 0;
        for (int c = x0; c <= x1; c++)
            if (grid[r][c] == '#') g->rows[r] |= (uint8_t)(1u << (c - x0));
    }
}

void font_init(void) {
    if (font_ready) return;
    memset(glyphs, 0, sizeof glyphs);
    for (int i = 0; i < 95; i++) {
        int ch = 32 + i;
        if (!GLYPHS[i]) continue;
        int trim = !(ch >= '0' && ch <= '9') && ch != ' ';
        build(&glyphs[ch], GLYPHS[i], trim);
    }
    glyphs[' '].w = 3;
    memset(glyphs[' '].rows, 0, 7);
    for (int i = 0; i < 16; i++) build(&glyphs[128 + i], ICONS[i], 1);
    font_ready = 1;
}

static const Glyph *glyph_for(unsigned char c) {
    if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 32);
    if (c >= 0x80 && c < 0x90) return &glyphs[128 + (c - 0x80)];
    if (c < 32 || c > 126) return &glyphs['?'];
    return &glyphs[c];
}

static int line_width(const char *s, int scale) {
    int w = 0;
    for (; *s && *s != '\n'; s++) {
        if (*s == '\t') { w = w < TAB_W * scale ? TAB_W * scale : w + 8 * scale; continue; }
        w += (glyph_for((unsigned char)*s)->w + 1) * scale;
    }
    return w > 0 ? w - scale : 0;
}

int text_width_scaled(const char *s, int scale) {
    int best = 0;
    while (*s) {
        int w = line_width(s, scale);
        if (w > best) best = w;
        while (*s && *s != '\n') s++;
        if (*s == '\n') s++;
    }
    return best;
}

int text_width(const char *s) { return text_width_scaled(s, 1); }

int text_draw_scaled(const char *s, int x, int y, int col, int scale) {
    int cx = x;
    for (; *s; s++) {
        if (*s == '\n') { cx = x; y += (LINE_H)*scale; continue; }
        if (*s == '\t') { /* tab stop: a fixed column for tables */
            cx = cx < x + TAB_W * scale ? x + TAB_W * scale : cx + 8 * scale;
            continue;
        }
        const Glyph *g = glyph_for((unsigned char)*s);
        for (int r = 0; r < 7; r++) {
            uint8_t bits = g->rows[r];
            if (!bits) continue;
            for (int c = 0; c < g->w; c++)
                if (bits & (1u << c)) {
                    if (scale == 1) gfx_pset(cx + c, y + r, col);
                    else gfx_rect(cx + c * scale, y + r * scale, scale, scale, col);
                }
        }
        cx += (g->w + 1) * scale;
    }
    return cx;
}

int text_draw(const char *s, int x, int y, int col) { return text_draw_scaled(s, x, y, col, 1); }

void text_render_mask(const char *s, int scale, uint8_t *buf, int bw, int bh, int ox, int oy) {
    int cx = ox, y = oy;
    for (; *s; s++) {
        if (*s == '\n') { cx = ox; y += LINE_H * scale; continue; }
        const Glyph *g = glyph_for((unsigned char)*s);
        for (int r = 0; r < 7; r++)
            for (int c = 0; c < g->w; c++) {
                if (!(g->rows[r] & (1u << c))) continue;
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++) {
                        int px = cx + c * scale + sx, py = y + r * scale + sy;
                        if (px >= 0 && py >= 0 && px < bw && py < bh) buf[py * bw + px] = 1;
                    }
            }
        cx += (g->w + 1) * scale;
    }
}

void text_shadow(const char *s, int x, int y, int col, int shadow) {
    text_draw(s, x + 1, y + 1, shadow);
    text_draw(s, x, y, col);
}

void text_outline(const char *s, int x, int y, int col, int outline) {
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (dx || dy) text_draw(s, x + dx, y + dy, outline);
    text_draw(s, x, y, col);
}

void text_center(const char *s, int cx, int y, int col) {
    /* centre each line separately */
    char buf[256];
    while (*s) {
        int n = 0;
        while (s[n] && s[n] != '\n' && n < 255) n++;
        memcpy(buf, s, (size_t)n);
        buf[n] = 0;
        text_draw(buf, cx - line_width(buf, 1) / 2, y, col);
        s += n;
        if (*s == '\n') s++;
        y += LINE_H;
    }
}

void text_center_shadow(const char *s, int cx, int y, int col, int shadow) {
    text_center(s, cx + 1, y + 1, shadow);
    text_center(s, cx, y, col);
}

int text_wrap(const char *s, int x, int y, int w, int col, int line_h) {
    int lines = 0;
    char line[256];
    int ln = 0;
    const char *p = s;
    while (*p) {
        /* next word */
        const char *wstart = p;
        while (*p && *p != ' ' && *p != '\n') p++;
        int wlen = (int)(p - wstart);
        char trial[256];
        int tl = 0;
        if (ln > 0) { memcpy(trial, line, (size_t)ln); tl = ln; trial[tl++] = ' '; }
        if (tl + wlen > 250) wlen = 250 - tl;
        memcpy(trial + tl, wstart, (size_t)wlen);
        tl += wlen;
        trial[tl] = 0;
        if (ln > 0 && line_width(trial, 1) > w) {
            line[ln] = 0;
            if (col >= 0) text_draw(line, x, y + lines * line_h, col);
            lines++;
            memcpy(line, wstart, (size_t)wlen);
            ln = wlen;
        } else {
            memcpy(line, trial, (size_t)tl);
            ln = tl;
        }
        if (*p == '\n') {
            line[ln] = 0;
            if (col >= 0) text_draw(line, x, y + lines * line_h, col);
            lines++;
            ln = 0;
            p++;
        } else if (*p == ' ') {
            p++;
        }
    }
    if (ln > 0) {
        line[ln] = 0;
        if (col >= 0) text_draw(line, x, y + lines * line_h, col);
        lines++;
    }
    return lines;
}

/* ---- tiny 3x5 font ----------------------------------------------------- */

static const char *TINY_DIGITS[10] = {
    "###|#.#|#.#|#.#|###", ".#.|##.|.#.|.#.|###", "##.|..#|.#.|#..|###",
    "##.|..#|.#.|..#|##.", "#.#|#.#|###|..#|..#", "###|#..|##.|..#|##.",
    ".##|#..|###|#.#|###", "###|..#|.#.|.#.|.#.", "###|#.#|###|#.#|###",
    "###|#.#|###|..#|##."};
static const char *TINY_ALPHA[26] = {
    ".#.|#.#|###|#.#|#.#", "##.|#.#|##.|#.#|##.", ".##|#..|#..|#..|.##",
    "##.|#.#|#.#|#.#|##.", "###|#..|##.|#..|###", "###|#..|##.|#..|#..",
    ".##|#..|#.#|#.#|.##", "#.#|#.#|###|#.#|#.#", "###|.#.|.#.|.#.|###",
    "..#|..#|..#|#.#|.#.", "#.#|#.#|##.|#.#|#.#", "#..|#..|#..|#..|###",
    "#.#|###|###|#.#|#.#", "##.|#.#|#.#|#.#|#.#", ".#.|#.#|#.#|#.#|.#.",
    "##.|#.#|##.|#..|#..", ".#.|#.#|#.#|##.|.##", "##.|#.#|##.|#.#|#.#",
    ".##|#..|.#.|..#|##.", "###|.#.|.#.|.#.|.#.", "#.#|#.#|#.#|#.#|###",
    "#.#|#.#|#.#|#.#|.#.", "#.#|#.#|###|###|#.#", "#.#|#.#|.#.|#.#|#.#",
    "#.#|#.#|.#.|.#.|.#.", "###|..#|.#.|#..|###"};
static const struct { char c; const char *g; } TINY_PUNCT[] = {
    {'.', "...|...|...|...|.#."}, {':', "...|.#.|...|.#.|..."}, {'-', "...|...|###|...|..."},
    {'/', "..#|..#|.#.|#..|#.."}, {'+', "...|.#.|###|.#.|..."}, {'*', "...|#.#|.#.|#.#|..."},
    {'!', ".#.|.#.|.#.|...|.#."}, {'?', "##.|..#|.#.|...|.#."}, {'%', "#.#|..#|.#.|#..|#.#"},
    {'\'', ".#.|.#.|...|...|..."}, {'(', ".#.|#..|#..|#..|.#."}, {')', ".#.|..#|..#|..#|.#."},
    {'>', "#..|.#.|..#|.#.|#.."}, {'<', "..#|.#.|#..|.#.|..#"}, {'=', "...|###|...|###|..."},
    {',', "...|...|...|.#.|#.."}, {'#', ".#.#|####|.#.#|####|.#.#"}, {'$', ".##|##.|.#.|.##|##."},
    {'\x8a', "...|...|.#.|...|..."}, /* GLYPH_DOT: a middle dot */
};

static const char *tiny_glyph(char c) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    if (c >= '0' && c <= '9') return TINY_DIGITS[c - '0'];
    if (c >= 'A' && c <= 'Z') return TINY_ALPHA[c - 'A'];
    for (size_t i = 0; i < sizeof TINY_PUNCT / sizeof TINY_PUNCT[0]; i++)
        if (TINY_PUNCT[i].c == c) return TINY_PUNCT[i].g;
    return NULL;
}

/* Glyphs are 3 pixels wide except a few (like '#') that need 4. */
static int tiny_adv(char c) {
    const char *g = tiny_glyph(c);
    int w = 0;
    if (g)
        while (g[w] && g[w] != '|') w++;
    return (w > 3 ? w : 3) + 1;
}

int tiny_width(const char *s) {
    int w = 0;
    for (; *s; s++) w += tiny_adv(*s);
    return w > 0 ? w - 1 : 0;
}

int tiny_draw(const char *s, int x, int y, int col) {
    for (; *s; x += tiny_adv(*s), s++) {
        const char *g = tiny_glyph(*s);
        if (!g) continue;
        int r = 0, c = 0;
        for (const char *p = g; *p; p++) {
            if (*p == '|') { r++; c = 0; continue; }
            if (*p == '#') gfx_pset(x + c, y + r, col);
            c++;
        }
    }
    return x;
}

void tiny_center(const char *s, int cx, int y, int col) { tiny_draw(s, cx - tiny_width(s) / 2, y, col); }
