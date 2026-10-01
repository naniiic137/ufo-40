/* UFO 40 - the cartridge card: SELECT on a cartridge in the library opens
 * a small panel over the library (still there behind it, dimmed) with how
 * long and how often the cartridge has been played, its goals, DELETE SAVE
 * and its CONTROLS, where the A, B and SELECT jobs can move between those
 * three buttons for that cartridge only. SELECT or B closes it.
 *
 * Also here: the per-cartridge button layouts themselves (shell_remap_*),
 * which the runner hands to the input layer while the cartridge plays. */
#include "shell.h"
#include <ctype.h>

/* ---- button layouts ------------------------------------------------------ */

/* The physical button (JOB_*) that does each job: three 2-bit offsets in the
 * saved byte, so 0 is the layout the cartridge was made with. Anything that
 * isn't one button per job reads as the default. */
static void remap_decode(uint8_t code, int phys[JOB_COUNT]) {
    bool seen[JOB_COUNT] = {false};
    for (int j = 0; j < JOB_COUNT; j++) {
        phys[j] = (j + ((code >> (2 * j)) & 3)) % JOB_COUNT;
        if (seen[phys[j]]) {
            for (int k = 0; k < JOB_COUNT; k++) phys[k] = k;
            return;
        }
        seen[phys[j]] = true;
    }
}

static uint8_t remap_encode(const int phys[JOB_COUNT]) {
    int code = 0;
    for (int j = 0; j < JOB_COUNT; j++) code |= ((phys[j] - j + JOB_COUNT) % JOB_COUNT) << (2 * j);
    return (uint8_t)code;
}

int shell_remap_button(int slot, int job) {
    if (slot < 0 || slot >= GAME_SLOTS || job < 0 || job >= JOB_COUNT) return job;
    int phys[JOB_COUNT];
    remap_decode(g_progress.remap[slot], phys);
    return phys[job];
}

bool shell_remap_default(int slot) {
    for (int j = 0; j < JOB_COUNT; j++)
        if (shell_remap_button(slot, j) != j) return false;
    return true;
}

void shell_remap_cycle(int slot, int job) {
    if (slot < 0 || slot >= GAME_SLOTS || job < 0 || job >= JOB_COUNT) return;
    int phys[JOB_COUNT];
    remap_decode(g_progress.remap[slot], phys);
    int next = (phys[job] + 1) % JOB_COUNT;
    for (int k = 0; k < JOB_COUNT; k++)
        if (phys[k] == next) phys[k] = phys[job]; /* the job that had it takes this one's */
    phys[job] = next;
    g_progress.remap[slot] = remap_encode(phys);
    progress_save();
}

void shell_remap_reset(int slot) {
    if (slot < 0 || slot >= GAME_SLOTS) return;
    g_progress.remap[slot] = 0;
    progress_save();
}

static const int JOB_BIT[JOB_COUNT] = {4, 5, 7}; /* BTN_A, BTN_B, BTN_SELECT */
static const char *const JOB_NAME[JOB_COUNT] = {GLYPH_A, GLYPH_B, "SELECT"};

void shell_remap_apply(int slot) {
    if (slot < 0 || slot >= GAME_SLOTS || shell_remap_default(slot)) {
        input_set_remap(NULL);
        return;
    }
    uint8_t src[8];
    for (int i = 0; i < 8; i++) src[i] = (uint8_t)i;
    for (int j = 0; j < JOB_COUNT; j++) src[JOB_BIT[j]] = (uint8_t)JOB_BIT[shell_remap_button(slot, j)];
    input_set_remap(src);
}

/* The cartridge's controls list as the player now presses it: each A and B
 * glyph, and each word SELECT, becomes the button that does that job. */
void shell_controls_for(int slot, char *out, int n) {
    const char *s = GAMES[slot] ? GAMES[slot]->controls : "";
    int o = 0;
    for (const char *p = s; *p && o < n - 1;) {
        const char *put = NULL;
        int skip = 1;
        if (*p == GLYPH_A[0]) put = JOB_NAME[shell_remap_button(slot, JOB_A)];
        else if (*p == GLYPH_B[0]) put = JOB_NAME[shell_remap_button(slot, JOB_B)];
        else if (!strncmp(p, "SELECT", 6) && (p == s || !isalpha((unsigned char)p[-1])) && !isalpha((unsigned char)p[6])) {
            put = JOB_NAME[shell_remap_button(slot, JOB_SELECT)];
            skip = 6;
        }
        if (put) {
            for (const char *q = put; *q && o < n - 1; q++) out[o++] = *q;
            p += skip;
        } else {
            out[o++] = *p++;
        }
    }
    out[o] = 0;
}

/* ---- the card ------------------------------------------------------------ */

enum { PAGE_MAIN, PAGE_CONTROLS, PAGE_CONFIRM };
enum { ITEM_DELETE, ITEM_CONTROLS, ITEM_CLOSE, ITEM_COUNT };
/* the controls page: the three jobs in a row, then RESET and BACK */
enum { CI_JOB_A, CI_JOB_B, CI_JOB_SELECT, CI_RESET, CI_BACK, CI_COUNT };

#define PX 6
#define PY 6
#define PW 308
#define PH 156

static bool open_;
static int slot, page, sel, csel, yes, t, msg_t;
static const char *msg;

void cartinfo_open(int s) {
    open_ = s >= 0 && s < GAME_SLOTS && GAMES[s];
    slot = s;
    page = PAGE_MAIN;
    sel = ITEM_CONTROLS;
    csel = CI_JOB_A;
    yes = 0;
    msg_t = 0;
    t = 0;
}

bool cartinfo_active(void) { return open_; }

static bool can_delete(void) { return shell_save_state(slot) != SAVE_NONE; }

static void close_card(void) {
    open_ = false;
    sfx_play_name("ui_back");
}

void cartinfo_update(void) {
    if (!open_) return;
    t++;
    if (msg_t > 0) msg_t--;
    switch (page) {
    case PAGE_MAIN:
        if (btnp(BTN_SELECT) || btnp(BTN_B)) { close_card(); return; }
        if (btn_repeat(BTN_UP)) { sel = (sel + ITEM_COUNT - 1) % ITEM_COUNT; sfx_play_name("ui_move"); }
        if (btn_repeat(BTN_DOWN)) { sel = (sel + 1) % ITEM_COUNT; sfx_play_name("ui_move"); }
        if (btnp(BTN_A) || btnp(BTN_START)) {
            if (sel == ITEM_CLOSE) { close_card(); return; }
            if (sel == ITEM_CONTROLS) { page = PAGE_CONTROLS; csel = CI_JOB_A; sfx_play_name("ui_ok"); }
            else if (!can_delete()) sfx_play_name("ui_error");
            else { page = PAGE_CONFIRM; yes = 0; sfx_play_name("ui_ok"); }
        }
        break;
    case PAGE_CONTROLS: {
        if (btnp(BTN_SELECT)) { close_card(); return; }
        if (btnp(BTN_B)) { page = PAGE_MAIN; sfx_play_name("ui_back"); break; }
        int old = csel;
        bool top = csel <= CI_JOB_SELECT;
        if (btn_repeat(BTN_LEFT)) csel = top ? (csel + 2) % 3 : csel == CI_RESET ? CI_BACK : CI_RESET;
        if (btn_repeat(BTN_RIGHT)) csel = top ? (csel + 1) % 3 : csel == CI_RESET ? CI_BACK : CI_RESET;
        if (btn_repeat(BTN_UP) || btn_repeat(BTN_DOWN)) csel = top ? (csel == CI_JOB_SELECT ? CI_BACK : CI_RESET) : (csel == CI_BACK ? CI_JOB_SELECT : CI_JOB_A);
        if (csel != old) sfx_play_name("ui_move");
        if (btnp(BTN_A) || btnp(BTN_START)) {
            if (csel == CI_BACK) { page = PAGE_MAIN; sfx_play_name("ui_back"); }
            else if (csel == CI_RESET) {
                shell_remap_reset(slot);
                msg = "BUTTONS AS MADE";
                msg_t = 90;
                sfx_play_name("ui_ok");
            } else {
                shell_remap_cycle(slot, csel - CI_JOB_A);
                sfx_play_name("ui_ok");
            }
        }
        break;
    }
    case PAGE_CONFIRM:
        if (btnp(BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN)) { yes ^= 1; sfx_play_name("ui_move"); }
        if (btnp(BTN_B) || btnp(BTN_SELECT) || (btnp(BTN_A) && !yes)) { page = PAGE_MAIN; sfx_play_name("ui_back"); break; }
        if (btnp(BTN_A) && yes) {
            shell_delete_save(slot); /* the save data screen's own delete */
            page = PAGE_MAIN;
            msg = "SAVE DELETED";
            msg_t = 120;
            sfx_play_name("ui_back");
        }
        break;
    }
}

/* ---- drawing ------------------------------------------------------------- */

/* "1:05": hours and minutes. */
static void fmt_time(uint32_t secs, char *out, int n) {
    snprintf(out, (size_t)n, "%lu:%02lu", (unsigned long)(secs / 3600u), (unsigned long)(secs / 60u % 60u));
}

/* "2026-10-01" from Unix seconds (UTC), or "NEVER". */
static void fmt_date(uint32_t unix_s, char *out, int n) {
    if (!unix_s) { snprintf(out, (size_t)n, "NEVER"); return; }
    /* days to a civil date (H. Hinnant's algorithm) */
    long z = (long)(unix_s / 86400u) + 719468;
    long era = z / 146097, doe = z - era * 146097;
    long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long y = yoe + era * 400, doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    long mp = (5 * doy + 2) / 153, d = doy - (153 * mp + 2) / 5 + 1, m = mp < 10 ? mp + 3 : mp - 9;
    snprintf(out, (size_t)n, "%04ld-%02ld-%02ld", y + (m <= 2), m, d);
}

/* One entry of the goals list; the trophies to come can fill the same list. */
typedef struct {
    int icon;         /* GOAL_* bit for the icon */
    const char *name;
    const char *desc;
    bool earned;
} CardGoal;

static int card_goals(int s, CardGoal out[], int max) {
    static const char *const names[3] = {"BEACON", "SAUCER", "ALIEN"};
    int n = 0;
    for (int b = 0; b < 3 && n < max; b++, n++) {
        out[n].icon = 1 << b;
        out[n].name = names[b];
        out[n].desc = GAMES[s]->goal_desc[b];
        out[n].earned = (g_progress.goals[s] >> b) & 1;
    }
    return n;
}

static void draw_frame(void) {
    ui_panel(PX, PY, PW, PH, C_NIGHT, C_GREY);
    ui_audit_area("card", PX + 2, PY + 2, PW - 4, PH - 4);
    const GameDef *g = GAMES[slot];
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_YELLOW, C_AMBER};
    const char *head = page == PAGE_CONTROLS ? "CONTROLS" : g->title;
    ui_fancy_text(head, PX + 8, PY + 6, 1, grad, 4, C_INK, -1);
    ui_audit_fancy("title", head, PX + 8, PY + 6, 1, false);
    char buf[48];
    if (page == PAGE_CONTROLS) snprintf(buf, sizeof buf, "%02d %s", slot + 1, g->title);
    else snprintf(buf, sizeof buf, "SLOT %02d", slot + 1);
    int bx = PX + PW - 8 - tiny_width(buf);
    tiny_draw(buf, bx, PY + 8, C_SLATE);
    ui_audit_tiny("slot", buf, bx, PY + 8);
}

static void draw_main(void) {
    char buf[64];
    /* stats: three columns */
    static const char *const labels[3] = {"TIME PLAYED", "OPENED", "LAST PLAYED"};
    char val[3][24];
    fmt_time(g_progress.play_secs[slot], val[0], sizeof val[0]);
    int opened = g_progress.played[slot];
    snprintf(val[1], sizeof val[1], "%d%s TIME%s", opened, opened >= 255 ? "+" : "", opened == 1 ? "" : "S");
    fmt_date(g_progress.last_played[slot], val[2], sizeof val[2]);
    static const char *const lwhat[3] = {"time label", "opened label", "last label"};
    static const char *const vwhat[3] = {"time", "opened", "last"};
    for (int i = 0; i < 3; i++) {
        int x = PX + 8 + i * 100;
        tiny_draw(labels[i], x, PY + 21, C_SLATE);
        ui_audit_tiny(lwhat[i], labels[i], x, PY + 21);
        text_draw(val[i], x, PY + 28, C_LIGHT);
        ui_audit_text(vwhat[i], val[i], x, PY + 28);
    }
    gfx_hline(PX + 6, PX + PW - 7, PY + 40, C_DUSK);
    /* goals */
    CardGoal goals[8];
    int n = card_goals(slot, goals, 8);
    tiny_draw("GOALS", PX + 8, PY + 44, C_YELLOW);
    ui_audit_tiny("goals heading", "GOALS", PX + 8, PY + 44);
    static const char *const gwhat[3][3] = {{"goal 1 name", "goal 1 line 1", "goal 1 line 2"},
                                            {"goal 2 name", "goal 2 line 1", "goal 2 line 2"},
                                            {"goal 3 name", "goal 3 line 1", "goal 3 line 2"}};
    for (int i = 0; i < imin(n, 3); i++) {
        int y = PY + 52 + i * 15;
        ui_goal_icon(PX + 8, y, goals[i].icon, goals[i].earned, t);
        ui_audit_box("goal icon", PX + 8, y, 9, 9);
        text_draw(goals[i].name, PX + 21, y + 1, goals[i].earned ? C_YELLOW : C_GREY);
        ui_audit_text(gwhat[i][0], goals[i].name, PX + 21, y + 1);
        char l[2][UI_WRAP_LEN];
        int dx = PX + 70, dw = PW - 70 - 24;
        int k = ui_wrap(goals[i].desc, dw, true, l, 2);
        for (int j = 0; j < imin(k, 2); j++) {
            tiny_draw(l[j], dx, y + j * 7, goals[i].earned ? C_LIGHT : C_SLATE);
            ui_audit_tiny(gwhat[i][1 + j], l[j], dx, y + j * 7);
        }
        if (k > 2) ui_audit_fail(gwhat[i][0], "its words need more than two lines");
        const char *mark = goals[i].earned ? GLYPH_CHECK : "-";
        text_draw(mark, PX + PW - 16, y + 1, goals[i].earned ? C_LIME : C_DUSK);
        ui_audit_text("goal mark", GLYPH_CHECK, PX + PW - 16, y + 1);
    }
    gfx_hline(PX + 6, PX + PW - 7, PY + 98, C_DUSK);
    /* actions, with a word on the one under the cursor */
    static const char *const items[ITEM_COUNT] = {"DELETE SAVE", "CONTROLS", "CLOSE"};
    for (int i = 0; i < ITEM_COUNT; i++) {
        int y = PY + 104 + i * 13;
        bool s = i == sel, en = i != ITEM_DELETE || can_delete();
        if (s) gfx_rect(PX + 6, y - 3, 96, 13, C_DUSK);
        if (s) ui_cursor(PX + 9, y, t);
        text_draw(items[i], PX + 20, y, !en ? C_SLATE : s ? C_WHITE : C_GREY);
        ui_audit_text("item", items[i], PX + 20, y);
    }
    ui_audit_box("cursor", PX + 9, PY + 104, 5, 33);
    static const char *const st[3] = {"NO SAVE FILE YET.", "A SAVE FILE. ITS GOALS STAY IF IT GOES.",
                                      "A DAMAGED SAVE FILE. IT CAN GO."};
    const char *about = sel == ITEM_DELETE ? st[shell_save_state(slot)]
                        : sel == ITEM_CONTROLS
                            ? (shell_remap_default(slot) ? "WHICH BUTTON DOES WHAT, FOR THIS CARTRIDGE ONLY. AS MADE."
                                                         : "WHICH BUTTON DOES WHAT, FOR THIS CARTRIDGE ONLY. CHANGED.")
                            : "BACK TO THE LIBRARY.";
    char l[3][UI_WRAP_LEN];
    int k = ui_wrap(about, PW - 120, true, l, 3);
    static const char *const awhat[3] = {"about line 1", "about line 2", "about line 3"};
    for (int j = 0; j < imin(k, 3); j++) {
        tiny_draw(l[j], PX + 112, PY + 105 + j * 7, C_GREY);
        ui_audit_tiny(awhat[j], l[j], PX + 112, PY + 105 + j * 7);
    }
    if (k > 3) ui_audit_fail("about", "needs more than three lines");
    snprintf(buf, sizeof buf, "SELECT OR " GLYPH_B ": CLOSE");
    int hx = PX + PW / 2 - text_width(buf) / 2;
    text_draw(buf, hx, PY + PH - 13, C_SLATE);
    ui_audit_text("hint", buf, hx, PY + PH - 13);
}

static void draw_controls(void) {
    /* the three jobs: the game's button, and the one you press for it */
    static const int chip_x[JOB_COUNT] = {6, 92, 178}, chip_w[JOB_COUNT] = {84, 84, 124};
    for (int j = 0; j < JOB_COUNT; j++) {
        int x = PX + chip_x[j], y = PY + 20;
        bool s = csel == CI_JOB_A + j;
        int phys = shell_remap_button(slot, j);
        if (s) gfx_rect(x, y - 3, chip_w[j], 13, C_DUSK);
        if (s) ui_cursor(x + 3, y, t);
        char buf[32];
        snprintf(buf, sizeof buf, "%s ON %s", JOB_NAME[j], JOB_NAME[phys]);
        text_draw(buf, x + 14, y, phys != j ? C_YELLOW : s ? C_WHITE : C_GREY);
        ui_audit_text("job", buf, x + 14, y);
    }
    static const char *const below[2] = {"RESET TO DEFAULT", "BACK"};
    static const int below_x[2] = {6, 122}, below_w[2] = {110, 56};
    for (int i = 0; i < 2; i++) {
        int x = PX + below_x[i], y = PY + 33;
        bool s = csel == CI_RESET + i;
        if (s) gfx_rect(x, y - 3, below_w[i], 13, C_DUSK);
        if (s) ui_cursor(x + 3, y, t);
        text_draw(below[i], x + 14, y, s ? C_WHITE : C_GREY);
        ui_audit_text("item", below[i], x + 14, y);
    }
    char l[2][UI_WRAP_LEN];
    int k = ui_wrap("A MOVES A JOB TO THE NEXT BUTTON. THIS CARTRIDGE ONLY.", PW - 4 - 184, true, l, 2);
    static const char *const cwhat[2] = {"caption line 1", "caption line 2"};
    for (int j = 0; j < imin(k, 2); j++) {
        tiny_draw(l[j], PX + 186, PY + 31 + j * 6, C_SLATE);
        ui_audit_tiny(cwhat[j], l[j], PX + 186, PY + 31 + j * 6);
    }
    if (k > 2) ui_audit_fail("caption", "needs more than two lines");
    gfx_hline(PX + 6, PX + PW - 7, PY + 45, C_DUSK);
    char list[600];
    shell_controls_for(slot, list, sizeof list);
    text_draw(list, PX + 10, PY + 48, C_LIGHT);
    ui_audit_text("controls", list, PX + 10, PY + 48); /* every line inside the card */
}

static void draw_confirm(void) {
    char q[64], d[64];
    shell_delete_save_question(slot, q, sizeof q, d, sizeof d);
    ui_confirm_box(q, d, "YES", yes, C_YELLOW, NULL, t);
}

void cartinfo_draw(void) {
    if (!open_) return;
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    draw_frame();
    if (page == PAGE_CONTROLS) draw_controls();
    else draw_main();
    if (page == PAGE_CONFIRM) {
        gfx_darken_rect(PX, PY, PW, PH, 1);
        draw_confirm();
    }
    if (msg_t > 0) {
        int w = text_width(msg) + 20;
        ui_panel(SCREEN_W / 2 - w / 2, 80, w, 17, C_INK, C_YELLOW);
        text_center(msg, SCREEN_W / 2, 85, C_YELLOW);
    }
}

/* Draws the card's pages for every cartridge with the layout audit on, each
 * with the cursor on every item (the words beside the items change with
 * it), and its delete question; returns the problems found. */
static int audit_cards(void) {
    bool o = open_;
    int s0 = slot, p0 = page, sel0 = sel, c0 = csel, y0 = yes, m0 = msg_t, bad = 0;
    msg_t = 0;
    char subject[80];
    for (int s = 0; s < GAME_SLOTS; s++) {
        if (!GAMES[s]) continue;
        slot = s;
        open_ = true;
        for (sel = 0; sel < ITEM_COUNT; sel++) {
            page = PAGE_MAIN;
            snprintf(subject, sizeof subject, "card %s, item %d", GAMES[s]->title, sel);
            ui_audit_begin(subject, true);
            draw_frame();
            draw_main();
            bad += ui_audit_end();
        }
        for (csel = 0; csel < CI_COUNT; csel++) {
            page = PAGE_CONTROLS;
            snprintf(subject, sizeof subject, "card %s, controls %d", GAMES[s]->title, csel);
            ui_audit_begin(subject, true);
            draw_frame();
            draw_controls();
            bad += ui_audit_end();
        }
        for (yes = 0; yes <= 1; yes++) {
            snprintf(subject, sizeof subject, "card %s, delete question", GAMES[s]->title);
            ui_audit_begin(subject, true);
            draw_confirm();
            bad += ui_audit_end();
        }
    }
    open_ = o;
    slot = s0;
    page = p0;
    sel = sel0;
    csel = c0;
    yes = y0;
    msg_t = m0;
    return bad;
}

bool cartinfo_query(const char *key, int *out) {
    if (!strcmp(key, "card_open")) { *out = open_; return true; }
    if (!strcmp(key, "card_page")) { *out = open_ ? page : -1; return true; }
    if (!strcmp(key, "card_item")) { *out = page == PAGE_CONTROLS ? csel : sel; return true; }
    if (!strcmp(key, "card_layout")) { *out = audit_cards(); return true; }
    return false;
}
