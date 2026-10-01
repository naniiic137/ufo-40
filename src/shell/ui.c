#include "ui.h"

static uint8_t mask_px[320 * 72];
static Surface mask_surf = {320, 72, mask_px};
static Sprite spr_saucer, spr_goal[3];
static int ui_ready;

static const char SAUCER[] =
    "........CCCCC........"
    "......CIIICCCCC......"
    ".....CIICCCCCCCu....."
    ".....CICCCCCCCuu....."
    "..ssglllllllllllgss.."
    ".sgllwwwwwwwwwwwllgs."
    "sglwwwwwwwwwwwwwwwlgs"
    "sgaysgaysgaysgaysgays"
    ".ssgggggggggggggggss."
    "....sssssssssssss....";

static const char ICON_BEACON[] =
    "y...y...y"
    ".y..c..y."
    "...ccc..."
    "..cyyyc.."
    "..cyaac.."
    "...ccc..."
    "...sgs..."
    "..sgggs.."
    ".sgggggs.";
static const char ICON_SAUCER[] =
    "........."
    "...CCC..."
    "..CICCC.."
    ".aayyyaa."
    "ayyyyyyya"
    "aoycyoyca"
    ".aaaaaaa."
    "...o.o..."
    "..o...o..";
static const char ICON_ALIEN[] =
    "..z...z.."
    "...z.z..."
    "..zzzzz.."
    ".zzzzzzz."
    "zkkzzzkkz"
    "zkkizzkkz"
    ".zzzzzzz."
    "..zzkzz.."
    "...zzz...";

void ui_init(void) {
    if (ui_ready) return;
    spr_make(&spr_saucer, 21, 10, SAUCER);
    spr_make(&spr_goal[0], 9, 9, ICON_BEACON);
    spr_make(&spr_goal[1], 9, 9, ICON_SAUCER);
    spr_make(&spr_goal[2], 9, 9, ICON_ALIEN);
    ui_ready = 1;
}

int ui_fancy_width(const char *s, int scale) { return text_width_scaled(s, scale) + 3; }

static void fancy_from_mask(int x, int y, int w, int h, int gh, const uint8_t *grad, int n, int outline,
                            int shadow, int scale);

void ui_fancy_text(const char *s, int x, int y, int scale, const uint8_t *grad, int n, int outline,
                   int shadow) {
    int w = text_width_scaled(s, scale) + 4;
    int h = 7 * scale + 4;
    if (w > mask_surf.w) w = mask_surf.w;
    if (h > mask_surf.h) h = mask_surf.h;
    memset(mask_px, 0, sizeof mask_px);
    text_render_mask(s, scale, mask_px, mask_surf.w, mask_surf.h, 2, 2);
    fancy_from_mask(x, y, w, h, 7 * scale, grad, n, outline, shadow, scale);
}

/* Big chunky logo letters, 12 rows tall, drawn for UFO 40. */
static const char *LOGO_GLYPHS[5][12] = {
    {"##......##", "##......##", "##......##", "##......##", "##......##", "##......##",
     "##......##", "##......##", "##......##", "###....###", ".########.", "..######.."},
    {"#########", "#########", "##.......", "##.......", "##.......", "#######..",
     "#######..", "##.......", "##.......", "##.......", "##.......", "##......."},
    {"..######..", ".########.", "###....###", "##......##", "##......##", "##......##",
     "##......##", "##......##", "##......##", "###....###", ".########.", "..######.."},
    {".....###..", "....####..", "...##.##..", "..##..##..", ".##...##..", "##....##..",
     "##########", "##########", "......##..", "......##..", "......##..", "......##.."},
    {"..#####..", ".#######.", "###...###", "##.....##", "##.....##", "##.....##",
     "##.....##", "##.....##", "##.....##", "###...###", ".#######.", "..#####.."},
};

static int logo_mask(int scale, int which) {
    /* which: 0 = "UFO", 1 = "40". Returns width in pixels (unscaled*scale). */
    int first = which == 0 ? 0 : 3, last = which == 0 ? 2 : 4;
    int x = 2;
    for (int g = first; g <= last; g++) {
        int gw = (int)strlen(LOGO_GLYPHS[g][0]);
        for (int r = 0; r < 12; r++)
            for (int c = 0; c < gw; c++)
                if (LOGO_GLYPHS[g][r][c] == '#')
                    for (int sy = 0; sy < scale; sy++)
                        for (int sx = 0; sx < scale; sx++) {
                            int px = x + c * scale + sx, py = 2 + r * scale + sy;
                            if (px < mask_surf.w && py < mask_surf.h) mask_px[py * mask_surf.w + px] = 1;
                        }
        x += (gw + 2) * scale;
    }
    return x - 2 - 2 * scale;
}

static void fancy_from_mask(int x, int y, int w, int h, int gh, const uint8_t *grad, int n, int outline,
                            int shadow, int scale) {
#define M(xx, yy) ((xx) >= 0 && (yy) >= 0 && (xx) < mask_surf.w && (yy) < mask_surf.h && mask_px[(yy)*mask_surf.w + (xx)])
    /* shadow = outlined shape offset by (1,1) */
    if (shadow >= 0) {
        for (int yy = 0; yy < h; yy++)
            for (int xx = 0; xx < w; xx++) {
                bool on = M(xx, yy);
                if (!on && outline >= 0)
                    on = M(xx - 1, yy) || M(xx + 1, yy) || M(xx, yy - 1) || M(xx, yy + 1) ||
                         M(xx - 1, yy - 1) || M(xx + 1, yy + 1) || M(xx + 1, yy - 1) || M(xx - 1, yy + 1);
                if (on) {
                    gfx_pset(x + xx - 2 + 1, y + yy - 2 + 1, shadow);
                    if (scale >= 2) gfx_pset(x + xx - 2 + 1, y + yy - 2 + 2, shadow);
                }
            }
    }
    for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++) {
            if (M(xx, yy)) {
                int gi = (yy - 2) * n / (gh > 0 ? gh : 1);
                gi = gi < 0 ? 0 : gi >= n ? n - 1 : gi;
                gfx_pset(x + xx - 2, y + yy - 2, grad[gi]);
            } else if (outline >= 0 &&
                       (M(xx - 1, yy) || M(xx + 1, yy) || M(xx, yy - 1) || M(xx, yy + 1) ||
                        M(xx - 1, yy - 1) || M(xx + 1, yy + 1) || M(xx + 1, yy - 1) || M(xx - 1, yy + 1))) {
                gfx_pset(x + xx - 2, y + yy - 2, outline);
            }
        }
#undef M
}

void ui_fancy_center(const char *s, int cx, int y, int scale, const uint8_t *grad, int n, int outline,
                     int shadow) {
    ui_fancy_text(s, cx - text_width_scaled(s, scale) / 2, y, scale, grad, n, outline, shadow);
}

void ui_logo(int cx, int y, int scale, int t) {
    static const uint8_t g_ufo[] = {C_CREAM, C_YELLOW, C_YELLOW, C_AMBER, C_AMBER, C_ORANGE, C_ORANGE, C_RED};
    static const uint8_t g_40[] = {C_ICE, C_CYAN, C_CYAN, C_SKY, C_SKY, C_BLUE, C_BLUE, C_VIOLET};
    /* measure */
    memset(mask_px, 0, sizeof mask_px);
    int w1 = logo_mask(scale, 0);
    memset(mask_px, 0, sizeof mask_px);
    int w2 = logo_mask(scale, 1);
    int gap = scale * 6;
    int x = cx - (w1 + gap + w2) / 2;
    int h = 12 * scale + 4;
    memset(mask_px, 0, sizeof mask_px);
    logo_mask(scale, 0);
    fancy_from_mask(x, y, w1 + 4, h, 12 * scale, g_ufo, 8, C_INK, C_PURPLE, scale);
    memset(mask_px, 0, sizeof mask_px);
    logo_mask(scale, 1);
    fancy_from_mask(x + w1 + gap, y, w2 + 4, h, 12 * scale, g_40, 8, C_INK, C_NAVY, scale);
    /* a glint sweeping across the letters */
    int gx = (t * 3) % 400 - 40;
    for (int i = 0; i < 12 * scale; i++) {
        int px = x + gx + i / 2, py = y + i;
        if (gfx_pget(px, py) != C_INK && gfx_pget(px, py) != C_PURPLE && gfx_pget(px, py) != C_NAVY &&
            px >= x && px < x + w1 + gap + w2)
            if (gfx_pget(px, py) != 0) gfx_pset(px, py, C_WHITE);
    }
}

void ui_panel(int x, int y, int w, int h, int fill, int border) {
    gfx_rect(x + 1, y, w - 2, h, fill);
    gfx_rect(x, y + 1, w, h - 2, fill);
    gfx_hline(x + 1, x + w - 2, y, border);
    gfx_hline(x + 1, x + w - 2, y + h - 1, border);
    gfx_vline(x, y + 1, y + h - 2, border);
    gfx_vline(x + w - 1, y + 1, y + h - 2, border);
}

static uint32_t hash32(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
    return x;
}

void ui_starfield(int t, int bg) {
    gfx_cls(bg);
    for (int layer = 0; layer < 3; layer++) {
        int count = 26 + layer * 10;
        int speed = 3 - layer; /* far layers slower */
        for (int i = 0; i < count; i++) {
            uint32_t h = hash32((uint32_t)(i * 7919 + layer * 104729));
            int x = (int)(h % 640);
            int y = (int)((h >> 10) % SCREEN_H);
            x = ((x - t * (4 - speed) / 8) % 640 + 640) % 640 - 160;
            if (x < 0 || x >= SCREEN_W) continue;
            int tw = (int)((h >> 20) & 63);
            int phase = (t / 4 + tw) & 63;
            int col = layer == 0 ? C_DUSK : layer == 1 ? C_SLATE : C_LIGHT;
            if (layer == 2 && phase < 6) col = C_WHITE;
            if (layer == 2 && phase > 58) col = C_GREY;
            gfx_pset(x, y, col);
            if (layer == 2 && phase < 3) {
                gfx_pset(x - 1, y, C_SLATE); gfx_pset(x + 1, y, C_SLATE);
                gfx_pset(x, y - 1, C_SLATE); gfx_pset(x, y + 1, C_SLATE);
            }
        }
    }
}

void ui_goal_icon(int x, int y, int bit, bool earned, int t) {
    int idx = bit == GOAL_BEACON ? 0 : bit == GOAL_SAUCER ? 1 : 2;
    if (earned) {
        spr_draw_outline(&spr_goal[idx], x, y, 0, C_INK);
        if (((t / 6) + idx * 5) % 40 == 0) gfx_pset(x + 2, y + 2, C_WHITE);
    } else {
        spr_draw_ex(&spr_goal[idx], x, y, 0, NULL, C_DUSK);
    }
}

void ui_saucer(int x, int y, int t, int scale) {
    if (scale <= 1) {
        spr_draw(&spr_saucer, x, y, 0);
        /* chase lights */
        int k = (t / 6) % 4;
        for (int i = 0; i < 5; i++) gfx_pset(x + 2 + i * 4 + k % 2, y + 7, (i + k) % 2 ? C_YELLOW : C_RED);
    } else {
        spr_draw_scaled(&spr_saucer, x, y, scale, 0);
        int k = (t / 6) % 4;
        for (int i = 0; i < 5; i++)
            gfx_rect(x + (2 + i * 4 + k % 2) * scale, y + 7 * scale, scale, scale, (i + k) % 2 ? C_YELLOW : C_RED);
    }
}

void ui_cursor(int x, int y, int t) {
    int dx = (t / 8) % 2;
    text_draw(GLYPH_RIGHT, x + dx, y, C_YELLOW);
}

int ui_hint(int x, int y, const char *glyph, const char *label, int col) {
    int nx = text_draw(glyph, x, y, C_WHITE);
    return text_draw(label, nx + 2, y, col) + 6;
}

#define CHOICE_CURSOR 8 /* the cursor's room before a label */
#define CHOICE_GAP 18   /* between the first label and the second cursor */

void ui_choices(int cx, int y, const char *a, const char *b, int sel, int t, int on, int off) {
    int wa = CHOICE_CURSOR + text_width(a), wb = CHOICE_CURSOR + text_width(b);
    int total = wa + CHOICE_GAP + wb;
    int xa = cx - total / 2, xb = xa + wa + CHOICE_GAP;
    text_draw(a, xa + CHOICE_CURSOR, y, sel == 0 ? on : off);
    text_draw(b, xb + CHOICE_CURSOR, y, sel == 1 ? on : off);
    ui_cursor(sel == 0 ? xa : xb, y, t);
    ui_audit_text("first choice", a, xa + CHOICE_CURSOR, y);
    ui_audit_text("second choice", b, xb + CHOICE_CURSOR, y);
    ui_audit_box("cursor", sel == 0 ? xa : xb, y, 5, 7); /* it bobs a pixel to the right */
    ui_audit_centred("choices", xa, total);
}

void ui_confirm_box(const char *q, const char *d, const char *yes_label, int yes, int border, const char *step,
                    int t) {
    /* Every line, and the NO / YES pair as a whole, sits in the middle of
     * the box (the pair used to start at a fixed spot, so a long YES label
     * such as DELETE ALL ran off to the right); the audit measures each
     * one's margins. */
    int cx = SCREEN_W / 2;
    int w = imax(text_width(q), tiny_width(d)) + 24;
    w = imax(w, 150);
    int bx = cx - w / 2;
    ui_audit_area("screen", 4, 0, SCREEN_W - 8, 166);
    ui_audit_box("question box", bx, 58, w, 62);
    ui_panel(bx, 58, w, 62, C_INK, border);
    ui_audit_area("question box", bx + 2, 60, w - 4, 58);
    int qx = cx - text_width(q) / 2, dx = cx - tiny_width(d) / 2;
    text_draw(q, qx, 66, border == C_RED ? C_RED : C_YELLOW);
    ui_audit_text("question", q, qx, 66);
    ui_audit_centred("question", qx, text_width(q));
    tiny_draw(d, dx, 80, C_LIGHT);
    ui_audit_tiny("detail", d, dx, 80);
    ui_audit_centred("detail", dx, tiny_width(d));
    ui_choices(cx, 100, "NO", yes_label, yes, t, C_WHITE, C_SLATE);
    if (step) {
        int sx = cx - tiny_width(step) / 2;
        tiny_draw(step, sx, 112, C_GREY);
        ui_audit_tiny("step", step, sx, 112);
        ui_audit_centred("step", sx, tiny_width(step));
    }
}

/* ---- word wrap ------------------------------------------------------------ */

static int wrap_width(const char *s, bool tiny) { return tiny ? tiny_width(s) : text_width(s); }

int ui_wrap(const char *s, int w, bool tiny, char lines[][UI_WRAP_LEN], int max) {
    int n = 0;
    char cur[UI_WRAP_LEN] = "";
    const char *p = s;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        const char *ws = p;
        while (*p && *p != ' ' && *p != '\n') p++;
        char trial[UI_WRAP_LEN];
        int len = (int)(p - ws);
        if (cur[0]) snprintf(trial, sizeof trial, "%s %.*s", cur, len, ws);
        else snprintf(trial, sizeof trial, "%.*s", len, ws);
        if (cur[0] && wrap_width(trial, tiny) > w) {
            if (n < max) snprintf(lines[n], UI_WRAP_LEN, "%s", cur);
            n++;
            snprintf(cur, sizeof cur, "%.*s", len, ws);
        } else {
            snprintf(cur, sizeof cur, "%s", trial);
        }
        if (*p == '\n') {
            if (n < max) snprintf(lines[n], UI_WRAP_LEN, "%s", cur);
            n++;
            cur[0] = 0;
            p++;
        }
    }
    if (cur[0]) {
        if (n < max) snprintf(lines[n], UI_WRAP_LEN, "%s", cur);
        n++;
    }
    if (n > max && max > 0) {
        /* too long: the last line shown ends in "...", dropping words to fit */
        char *last = lines[max - 1];
        char trial[UI_WRAP_LEN + 4];
        for (;;) {
            snprintf(trial, sizeof trial, "%s...", last);
            if (wrap_width(trial, tiny) <= w) break;
            char *sp = strrchr(last, ' ');
            if (!sp) { /* one word: cut letters */
                int l = (int)strlen(last);
                if (l <= 1) break;
                last[l - 1] = 0;
            } else {
                *sp = 0;
            }
        }
        trial[UI_WRAP_LEN - 1] = 0; /* it fits in w, so it fits here */
        memcpy(last, trial, UI_WRAP_LEN);
    }
    return n;
}

/* ---- layout audit --------------------------------------------------------- */

#define AUDIT_BOXES 64
static struct {
    bool on, log;
    char subject[96];
    const char *area;
    int ax, ay, aw, ah;
    struct { const char *what; int x, y, w, h; } box[AUDIT_BOXES];
    int n, problems;
} au;

void ui_audit_begin(const char *subject, bool log) {
    memset(&au, 0, sizeof au);
    au.on = true;
    au.log = log;
    snprintf(au.subject, sizeof au.subject, "%s", subject);
    au.area = "screen";
    au.aw = SCREEN_W;
    au.ah = SCREEN_H;
}

bool ui_audit_on(void) { return au.on; }

void ui_audit_area(const char *name, int x, int y, int w, int h) {
    if (!au.on) return;
    au.area = name;
    au.ax = x; au.ay = y; au.aw = w; au.ah = h;
    au.n = 0;
}

void ui_audit_fail(const char *what, const char *why) {
    if (!au.on) return;
    au.problems++;
    if (au.log) fprintf(stderr, "  layout: %s: %s %s\n", au.subject, what, why);
}

void ui_audit_box(const char *what, int x, int y, int w, int h) {
    if (!au.on) return;
    char why[160];
    if (x < au.ax || y < au.ay || x + w > au.ax + au.aw || y + h > au.ay + au.ah) {
        snprintf(why, sizeof why, "(%d,%d %dx%d) sticks out of the %s (%d,%d %dx%d)", x, y, w, h, au.area, au.ax,
                 au.ay, au.aw, au.ah);
        ui_audit_fail(what, why);
    }
    for (int i = 0; i < au.n; i++) {
        int bx = au.box[i].x, by = au.box[i].y, bw = au.box[i].w, bh = au.box[i].h;
        if (x < bx + bw && bx < x + w && y < by + bh && by < y + h) {
            snprintf(why, sizeof why, "(%d,%d %dx%d) overlaps %s (%d,%d %dx%d)", x, y, w, h, au.box[i].what, bx, by,
                     bw, bh);
            ui_audit_fail(what, why);
        }
    }
    if (au.n < AUDIT_BOXES) {
        au.box[au.n].what = what;
        au.box[au.n].x = x; au.box[au.n].y = y; au.box[au.n].w = w; au.box[au.n].h = h;
        au.n++;
    }
}

static void audit_glyphs(const char *what, const char *s, int missing) {
    if (missing <= 0) return;
    char why[160];
    snprintf(why, sizeof why, "\"%.100s\" has %d character%s the font can't draw", s, missing, missing == 1 ? "" : "s");
    ui_audit_fail(what, why);
}

void ui_audit_centred(const char *what, int x, int w) {
    if (!au.on) return;
    int left = x - au.ax, right = au.ax + au.aw - (x + w);
    if (left - right > 1 || right - left > 1) {
        char why[160];
        snprintf(why, sizeof why, "is off centre in the %s: %d px to its left, %d to its right", au.area, left, right);
        ui_audit_fail(what, why);
    }
}

void ui_audit_text(const char *what, const char *s, int x, int y) {
    if (!au.on || !*s) return;
    int lines = 1;
    for (const char *p = s; *p; p++) lines += *p == '\n';
    audit_glyphs(what, s, text_missing(s));
    /* a table row: what comes before the tab must end short of the tab stop,
     * or the second column slips out of line */
    for (const char *p = s; *p;) {
        const char *end = p, *tab = NULL;
        while (*end && *end != '\n') {
            if (*end == '\t' && !tab) tab = end;
            end++;
        }
        if (tab) {
            char key[UI_WRAP_LEN];
            snprintf(key, sizeof key, "%.*s", (int)(tab - p), p);
            if (text_width(key) > TAB_W - 3) {
                char why[160];
                snprintf(why, sizeof why, "\"%s\" runs past the tab stop (%d > %d)", key, text_width(key), TAB_W - 3);
                ui_audit_fail(what, why);
            }
        }
        p = *end ? end + 1 : end;
    }
    ui_audit_box(what, x, y, text_width(s), (lines - 1) * LINE_H + FONT_H);
}

void ui_audit_tiny(const char *what, const char *s, int x, int y) {
    if (!au.on || !*s) return;
    audit_glyphs(what, s, tiny_missing(s));
    ui_audit_box(what, x, y, tiny_width(s), 5);
}

void ui_audit_fancy(const char *what, const char *s, int x, int y, int scale, bool shadow) {
    if (!au.on || !*s) return;
    audit_glyphs(what, s, text_missing(s));
    /* the outline adds a pixel all round, the shadow one more (two rows at scale 2) */
    int w = text_width_scaled(s, scale) + (shadow ? 3 : 2);
    int h = 7 * scale + 2 + (shadow ? (scale >= 2 ? 2 : 1) : 0);
    ui_audit_box(what, x - 1, y - 1, w, h);
}

int ui_audit_end(void) {
    int n = au.problems;
    au.on = false;
    return n;
}
