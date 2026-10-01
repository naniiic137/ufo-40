/* UFO 40 - headless platform: runs the console with scripted input, dumps
 * PNG screenshots / GIF clips and checks expectations. Used for all tests.
 *
 *   ufo40_headless --script tests/foo.ufs [--out DIR] [--save-dir DIR]
 *   ufo40_headless --frames 600 --shot-every 60 --out DIR
 *   ufo40_headless --vita-assets DIR
 *
 * For videos: record_start / record_stop write every frame as a PNG,
 * wav_start / wav_stop the same frames' audio, overlay_* add captions.
 */
#include "../../shell/shell.h"
#include "imgwrite.h"
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#define MKDIR(p) mkdir(p, 0755)
#endif

static char save_dir[512] = "build/headless_save";
static char out_dir[512] = "build/shots";
static uint32_t held;
static int failures, checks;
static const char *script_name = "";
static int line_no;
static bool audio_on;
static int16_t audio_buf[800 * 2];
static GifWriter gif;
static int gif_every, gif_left, gif_count;
static int shot_scale = 1;

/* record_start / wav_start: every frame as a PNG, every frame's audio to a WAV */
static char rec_dir[640];
static bool rec_on;
static int rec_frames;
static FILE *wav_f;
static uint32_t wav_samples;
static float wav_peak;

/* overlay_*: text and plates drawn over every saved frame (not the game's own
 * screen), in the console's font, for captions in videos */
#define OVL_MAX 16
typedef struct { bool box; char align; int x, y, w, h, scale, col; char text[128]; } Overlay;
static Overlay ovl[OVL_MAX];
static int ovl_n;
static uint8_t ovl_px[SCREEN_W * SCREEN_H];

/* ---- platform services ------------------------------------------------ */

int plat_kind(void) { return PLAT_HEADLESS; }
const char *plat_name(void) { return "HEADLESS"; }
void plat_apply_video(int scale, int fullscreen) { (void)scale; (void)fullscreen; }
static bool quit_requested;
void plat_request_quit(void) { quit_requested = true; }

/* A pretend wall clock: each frame moves it on 1/60 s, and the script's
 * clock_skip moves it on without running any frames (a stalled loop). */
static uint64_t clock_sixtieths; /* in 1/60 ms */
uint32_t plat_clock_ms(void) { return (uint32_t)(clock_sixtieths / 60u); }
/* a fixed morning (2026-10-01 09:00 UTC) plus the run's own clock, so the
 * tests and screenshots always see the same dates */
uint32_t plat_unix_time(void) { return 1790845200u + (uint32_t)(clock_sixtieths / 60000u); }
const char *plat_save_where(void) { return save_dir; }

static void mkdirs(const char *path) {
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char *p = tmp + 1; *p; p++)
        if (*p == '/' || *p == '\\') {
            char c = *p;
            *p = 0;
            MKDIR(tmp);
            *p = c;
        }
    MKDIR(tmp);
}

int plat_save_write(const char *name, const void *data, int len) {
    char path[640];
    mkdirs(save_dir);
    snprintf(path, sizeof path, "%s/%s", save_dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    if (len > 0) fwrite(data, 1, (size_t)len, f);
    fclose(f);
    return 0;
}

int plat_save_read(const char *name, void *data, int maxlen) {
    char path[640];
    snprintf(path, sizeof path, "%s/%s", save_dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    int n = (int)fread(data, 1, (size_t)maxlen, f);
    fclose(f);
    return n;
}

/* ---- helpers ------------------------------------------------------------ */

static void fail(const char *fmt, const char *a, long got) {
    failures++;
    fprintf(stderr, "FAIL %s:%d: ", script_name, line_no);
    fprintf(stderr, fmt, a, got);
    fprintf(stderr, "\n");
}

/* Palette colour by number (0-31) or by its sprite letter (y = yellow...). */
static int parse_col(const char *s) {
    static const char LETTERS[] = "kndsglwmvroaycbtehqfjziNBuCIpVPK";
    if (isdigit((unsigned char)s[0])) return atoi(s) % PAL_COUNT;
    const char *p = strchr(LETTERS, s[0]);
    return p && s[0] ? (int)(p - LETTERS) : C_WHITE;
}

/* Text with a one-pixel ink outline; scale 0 is the tiny 3x5 font. An
 * upper-case alignment (L C R) puts it on an ink plate as well. */
static void ovl_draw_text(const Overlay *o) {
    int w = o->scale ? text_width_scaled(o->text, o->scale) : tiny_width(o->text);
    char a = (char)tolower((unsigned char)o->align);
    int x = a == 'c' ? o->x - w / 2 : a == 'r' ? o->x - w : o->x;
    if (isupper((unsigned char)o->align)) gfx_rect(x - 3, o->y - 2, w + 6, (o->scale ? 7 * o->scale : 5) + 4, C_INK);
    /* the eight outline offsets, then the text itself (k = 4 is the centre) */
    for (int k = 0; k < 10; k++) {
        if (k == 4) continue;
        int dx = k < 9 ? k % 3 - 1 : 0, dy = k < 9 ? k / 3 - 1 : 0, col = k < 9 ? C_INK : o->col;
        if (o->scale) text_draw_scaled(o->text, x + dx, o->y + dy, col, o->scale);
        else tiny_draw(o->text, x + dx, o->y + dy, col);
    }
}

/* The frame as saved: the game's screen with the overlays on a copy of it. */
static const uint8_t *frame_pixels(void) {
    if (!ovl_n) return g_screen.px;
    memcpy(ovl_px, g_screen.px, sizeof ovl_px);
    Surface s = {SCREEN_W, SCREEN_H, ovl_px};
    int cx = gfx_cam_x(), cy = gfx_cam_y();
    gfx_set_target(&s);
    for (int i = 0; i < ovl_n; i++) {
        const Overlay *o = &ovl[i];
        if (!o->box) ovl_draw_text(o);
        else if (o->col < 0) gfx_darken_rect(o->x, o->y, o->w, o->h, -o->col);
        else gfx_rect(o->x, o->y, o->w, o->h, o->col);
    }
    gfx_set_target(NULL);
    gfx_camera(cx, cy);
    return ovl_px;
}

static void step(int n) {
    for (int i = 0; i < n; i++) {
        clock_sixtieths += 1000;
        input_set_raw(held);
        app_update();
        if (audio_on || wav_f) audio_render(audio_buf, 800);
        if (wav_f) {
            fwrite(audio_buf, 4, 800, wav_f);
            wav_samples += 800;
            float pk;
            audio_stats(&pk, NULL);
            if (pk > wav_peak) wav_peak = pk;
        }
        bool gif_now = gif_left > 0 && (gif_count++ % gif_every) == 0;
        if (gif_now || rec_on) {
            app_draw();
            const uint8_t *px = frame_pixels();
            if (gif_now) {
                int d = gif_every * 100 / 60;
                if (d < 2) d = 2;
                gif_frame(&gif, px, d);
            }
            if (rec_on) {
                /* the colours as presented: screen fades and flashes included */
                uint32_t lut[PAL_COUNT];
                uint8_t pal[PAL_COUNT][3];
                gfx_build_lut(lut, 1);
                for (int c = 0; c < PAL_COUNT; c++)
                    for (int k = 0; k < 3; k++) pal[c][k] = (uint8_t)(lut[c] >> (16 - 8 * k));
                char path[700];
                snprintf(path, sizeof path, "%s/%06d.png", rec_dir, rec_frames++);
                png_write_indexed(path, SCREEN_W, SCREEN_H, px, (const uint8_t(*)[3])pal, PAL_COUNT, shot_scale);
            }
        }
        if (gif_left > 0 && --gif_left == 0) gif_end(&gif);
    }
}

static void save_shot(const char *name) {
    char path[640];
    mkdirs(out_dir);
    snprintf(path, sizeof path, "%s/%s.png", out_dir, name);
    app_draw();
    png_write_indexed(path, SCREEN_W, SCREEN_H, frame_pixels(), PALETTE_RGB, PAL_COUNT, shot_scale);
    printf("  shot %s\n", path);
}

/* A 16-bit stereo 48 kHz WAV header for n sample frames. */
static void wav_header(FILE *f, uint32_t n) {
    uint32_t data_bytes = n * 4, riff = data_bytes + 36;
    uint8_t h[44] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0,
                     0x80, 0xBB, 0, 0, 0x00, 0xEE, 0x02, 0, 4, 0, 16, 0, 'd', 'a', 't', 'a', 0, 0, 0, 0};
    memcpy(h + 4, &riff, 4);
    memcpy(h + 40, &data_bytes, 4);
    fwrite(h, 1, 44, f);
}

static void wav_stop(void) {
    if (!wav_f) return;
    fseek(wav_f, 0, SEEK_SET);
    wav_header(wav_f, wav_samples);
    fclose(wav_f);
    wav_f = NULL;
    printf("  wav %u samples (peak %.2f)\n", (unsigned)wav_samples, wav_peak);
}

static uint32_t parse_buttons(char *s) {
    uint32_t m = 0;
    for (char *tok = strtok(s, " +\t\r\n"); tok; tok = strtok(NULL, " +\t\r\n")) {
        if (isdigit((unsigned char)tok[0])) break;
        /* P2UP, P2A, ...: the same buttons for player 2 */
        int sh = 0;
        if (!strncmp(tok, "P2", 2) && tok[2]) { tok += 2; sh = BTN_P2_SHIFT; }
        uint32_t b = 0;
        if (!strcmp(tok, "UP")) b = BTN_UP;
        else if (!strcmp(tok, "DOWN")) b = BTN_DOWN;
        else if (!strcmp(tok, "LEFT")) b = BTN_LEFT;
        else if (!strcmp(tok, "RIGHT")) b = BTN_RIGHT;
        else if (!strcmp(tok, "A")) b = BTN_A;
        else if (!strcmp(tok, "B")) b = BTN_B;
        else if (!strcmp(tok, "START")) b = BTN_START;
        else if (!strcmp(tok, "SELECT")) b = BTN_SELECT;
        else if (!strcmp(tok, "ALL")) b = BTN_ANY;
        m |= b << sh;
    }
    return m;
}

/* last integer in a string, or def */
static int last_int(const char *s, int def) {
    const char *p = s + strlen(s);
    while (p > s && !isdigit((unsigned char)p[-1])) p--;
    if (p == s) return def;
    while (p > s && isdigit((unsigned char)p[-1])) p--;
    if (p > s && p[-1] == '-') p--;
    /* only a number of its own counts ("P2A" is a button, not a 2) */
    if (p > s && !isspace((unsigned char)p[-1]) && p[-1] != '+') return def;
    return atoi(p);
}

static bool query(const char *key, int *out) {
    int gi = game_current_index();
    if (!strcmp(key, "goals")) { *out = gi >= 0 ? g_progress.goals[gi] : 0; return true; }
    if (!strcmp(key, "frame")) { *out = (int)engine_frame(); return true; }
    if (!strcmp(key, "audio.peak")) { float p; audio_stats(&p, NULL); *out = (int)(p * 1000); return true; }
    if (!strcmp(key, "audio.rms")) { float r; audio_stats(NULL, &r); *out = (int)(r * 1000); return true; }
    if (!strcmp(key, "paused")) { *out = game_paused(); return true; }
    if (!strcmp(key, "total_goals")) { *out = progress_goal_count(); return true; }
    if (!strncmp(key, "goals.", 6)) {
        int g = app_find_game(key + 6);
        *out = g >= 0 ? g_progress.goals[g] : -1;
        return true;
    }
    if (!strcmp(key, "quit_requested")) { *out = quit_requested; return true; }
    if (!strcmp(key, "record.frames")) { *out = rec_frames; return true; }
    if (!strcmp(key, "wav.samples")) { *out = (int)wav_samples; return true; }
    if (!strcmp(key, "wav.peak")) { *out = (int)(wav_peak * 1000); return true; }
    if (shell_query(key, out)) return true;
    if (gi >= 0 && GAMES[gi] && GAMES[gi]->query && GAMES[gi]->query(key, out)) return true;
    return false;
}

static bool compare(int a, const char *op, int b) {
    if (!strcmp(op, "==")) return a == b;
    if (!strcmp(op, "!=")) return a != b;
    if (!strcmp(op, "<")) return a < b;
    if (!strcmp(op, "<=")) return a <= b;
    if (!strcmp(op, ">")) return a > b;
    if (!strcmp(op, ">=")) return a >= b;
    if (!strcmp(op, "&")) return (a & b) != 0;
    if (!strcmp(op, "!&")) return (a & b) == 0;
    return false;
}

static void check_songs(void) {
    for (int s = 0; s < song_count(); s++) {
        if (!song_loops(s)) continue;
        int base = -1;
        for (int c = 0; c < CH_COUNT; c++) {
            int t = song_channel_loop_ticks(s, c);
            if (t == 0) continue;
            if (base < 0) base = t;
            checks++;
            if (t != base) {
                failures++;
                fprintf(stderr, "FAIL %s:%d: song '%s' channel %d loops every %d ticks, channel 0 every %d\n",
                        script_name, line_no, song_name(s), c, t, base);
            }
        }
    }
}

static int run_script(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return 1; }
    script_name = path;
    char line[512];
    line_no = 0;
    while (fgets(line, sizeof line, f)) {
        line_no++;
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        char *nl = strpbrk(p, "\r\n");
        if (nl) *nl = 0;
        if (!*p || *p == '#') continue;
        char cmd[32] = {0};
        sscanf(p, "%31s", cmd);
        char *arg = p + strlen(cmd);
        while (*arg == ' ') arg++;
        if (!strcmp(cmd, "seed")) {
            rng_seed(&g_rng, (uint64_t)atoll(arg));
        } else if (!strcmp(cmd, "game")) {
            char name[64] = {0};
            sscanf(arg, "%63s", name);
            int gi = app_find_game(name);
            if (gi < 0) { fail("unknown game '%s'%ld", name, 0); continue; }
            app_launch_game(gi, false);
            step(1);
        } else if (!strcmp(cmd, "wait")) {
            step(atoi(arg));
        } else if (!strcmp(cmd, "clock_skip")) {
            /* clock_skip MS : real time passes with no frames run (a hidden
             * browser tab, a stall); the next frame sees all of it at once */
            clock_sixtieths += (uint64_t)atoll(arg) * 60u;
        } else if (!strcmp(cmd, "idle")) {
            /* idle MINUTES : the collection sits open for that long, one
             * frame a second (quick to run, same real time as frames) */
            long frames = atol(arg) * 60;
            for (long k = 0; k < frames; k++) {
                clock_sixtieths += 59000u; /* 59/60 of a second with no frame ... */
                step(1);                   /* ... then one frame */
            }
        } else if (!strcmp(cmd, "hold")) {
            char tmp[256];
            snprintf(tmp, sizeof tmp, "%s", arg);
            held |= parse_buttons(tmp);
            int n = last_int(arg, 0);
            if (n > 0) { step(n); held = 0; }
        } else if (!strcmp(cmd, "release")) {
            char tmp[256];
            snprintf(tmp, sizeof tmp, "%s", arg);
            uint32_t m = parse_buttons(tmp);
            held = m ? held & ~m : 0;
        } else if (!strcmp(cmd, "press")) {
            char tmp[256];
            snprintf(tmp, sizeof tmp, "%s", arg);
            uint32_t m = parse_buttons(tmp);
            int n = last_int(arg, 2);
            held |= m;
            step(n);
            held &= ~m;
            step(1);
        } else if (!strcmp(cmd, "shot")) {
            save_shot(arg);
        } else if (!strcmp(cmd, "shotscale")) {
            shot_scale = atoi(arg);
        } else if (!strcmp(cmd, "gif")) {
            char name[128];
            int every = 2, frames = 180, scale = 1;
            sscanf(arg, "%127s %d %d %d", name, &every, &frames, &scale);
            char gpath[700];
            mkdirs(out_dir);
            snprintf(gpath, sizeof gpath, "%s/%s.gif", out_dir, name);
            gif_begin(&gif, gpath, SCREEN_W, SCREEN_H, scale, PALETTE_RGB);
            gif_every = every < 1 ? 1 : every;
            gif_left = frames;
            gif_count = 0;
            step(frames);
            printf("  gif %s\n", gpath);
        } else if (!strcmp(cmd, "gifstart")) {
            /* gifstart NAME EVERY SCALE : record until gifstop, inputs keep working */
            char name[128];
            int every = 2, scale = 1;
            sscanf(arg, "%127s %d %d", name, &every, &scale);
            char gpath[700];
            mkdirs(out_dir);
            snprintf(gpath, sizeof gpath, "%s/%s.gif", out_dir, name);
            gif_begin(&gif, gpath, SCREEN_W, SCREEN_H, scale, PALETTE_RGB);
            gif_every = every < 1 ? 1 : every;
            gif_left = 1 << 30;
            gif_count = 0;
            printf("  gif %s\n", gpath);
        } else if (!strcmp(cmd, "gifstop")) {
            if (gif_left > 0) gif_end(&gif);
            gif_left = 0;
        } else if (!strcmp(cmd, "cheat")) {
            int gi = game_current_index();
            if (gi < 0 || !GAMES[gi]->cheat || !GAMES[gi]->cheat(arg)) fail("cheat not handled: %s%ld", arg, 0);
        } else if (!strcmp(cmd, "expect")) {
            char key[96], op[8];
            int val;
            checks++;
            if (sscanf(arg, "%95s %7s %d", key, op, &val) != 3) { fail("bad expect: %s%ld", arg, 0); continue; }
            int got;
            if (!query(key, &got)) { fail("unknown key '%s'%ld", key, 0); continue; }
            if (!compare(got, op, val)) {
                failures++;
                fprintf(stderr, "FAIL %s:%d: expected %s %s %d, got %d\n", script_name, line_no, key, op, val, got);
            }
        } else if (!strcmp(cmd, "expect_scene")) {
            checks++;
            if (strcmp(app_scene_name(), arg) != 0) {
                failures++;
                fprintf(stderr, "FAIL %s:%d: expected scene %s, got %s\n", script_name, line_no, arg, app_scene_name());
            }
        } else if (!strcmp(cmd, "test_pause")) {
            /* test_pause PAGE : the pause menu (0), its controls page (1) or
             * the restart question (2), whether or not the game allows it now */
            app_test_pause(atoi(arg));
        } else if (!strcmp(cmd, "test_toast")) {
            /* test_toast BIT : the running game's goal toast (1, 2 or 4) */
            app_test_toast(atoi(arg));
        } else if (!strcmp(cmd, "reload")) {
            /* simulate quitting the program and starting it again */
            int gi = game_current_index();
            if (gi >= 0 && scene_current() == &SCENE_RUNNER && GAMES[gi]->quit) GAMES[gi]->quit();
            app_init();
            step(1);
        } else if (!strcmp(cmd, "wipe_saves")) {
            char pth[700];
            snprintf(pth, sizeof pth, "%s/progress.dat", save_dir);
            remove(pth);
            for (int i = 0; i < GAME_SLOTS; i++) { snprintf(pth, sizeof pth, "%s/game%02d.sav", save_dir, i + 1); remove(pth); }
            progress_defaults();
        } else if (!strcmp(cmd, "botplay")) {
            /* botplay FRAMES : the running cartridge's demo player chooses the
             * buttons each frame (its "bot" query), and they are pressed for real */
            int n = atoi(arg), gi = game_current_index();
            for (int i = 0; i < n; i++) {
                int mask = 0;
                if (gi >= 0 && GAMES[gi]->query && GAMES[gi]->query("bot", &mask)) held = (uint32_t)mask;
                step(1);
            }
            held = 0;
        } else if (!strcmp(cmd, "waituntil")) {
            /* waituntil KEY OP VALUE MAXFRAMES : step (buttons as they are) until the condition holds */
            char key[96], op[8];
            int val = 0, maxf = 0, got = 0, i = 0;
            checks++;
            if (sscanf(arg, "%95s %7s %d %d", key, op, &val, &maxf) != 4) { fail("bad waituntil: %s%ld", arg, 0); continue; }
            for (; i < maxf; i++) {
                if (query(key, &got) && compare(got, op, val)) break;
                step(1);
            }
            if (i >= maxf) {
                failures++;
                fprintf(stderr, "FAIL %s:%d: %s %s %d not reached in %d frames (got %d)\n", script_name, line_no, key, op, val, maxf, got);
            }
        } else if (!strcmp(cmd, "botuntil")) {
            /* botuntil KEY OP VALUE MAXFRAMES : the demo player plays until the
             * condition holds (or the frames run out, which fails) */
            char key[96], op[8];
            int val = 0, maxf = 0, gi = game_current_index();
            checks++;
            if (sscanf(arg, "%95s %7s %d %d", key, op, &val, &maxf) != 4) { fail("bad botuntil: %s%ld", arg, 0); continue; }
            int got = 0, i = 0;
            for (; i < maxf; i++) {
                if (query(key, &got) && compare(got, op, val)) break;
                int mask = 0;
                if (gi >= 0 && GAMES[gi]->query && GAMES[gi]->query("bot", &mask)) held = (uint32_t)mask;
                step(1);
            }
            held = 0;
            if (i >= maxf) {
                failures++;
                fprintf(stderr, "FAIL %s:%d: %s %s %d not reached in %d frames (got %d)\n", script_name, line_no, key, op, val, maxf, got);
            }
        } else if (!strcmp(cmd, "botloop")) {
            /* botloop KEY OP VALUE ROUNDS FRAMES SKIP_MS : the demo player plays
             * FRAMES, then SKIP_MS of real time pass with no frames (the console
             * left on while nobody plays), round after round until the condition
             * holds; it fails if ROUNDS run out first */
            char key[96], op[8];
            int val = 0, rounds = 0, frames = 0, gi = game_current_index();
            long skip = 0;
            checks++;
            if (sscanf(arg, "%95s %7s %d %d %d %ld", key, op, &val, &rounds, &frames, &skip) != 6) { fail("bad botloop: %s%ld", arg, 0); continue; }
            int got = 0, r = 0;
            bool done = false;
            for (; r < rounds && !done; r++) {
                int idle = 0, v = 0;
                for (int i = 0; i < frames; i++) {
                    if (query(key, &got) && compare(got, op, val)) { done = true; break; }
                    int mask = 0;
                    if (gi >= 0 && GAMES[gi]->query && GAMES[gi]->query("bot", &mask)) held = (uint32_t)mask;
                    step(1);
                    /* a demo player with nothing to do but wait ends the round early */
                    idle = gi >= 0 && GAMES[gi]->query && GAMES[gi]->query("bot_idle", &v) && v ? idle + 1 : 0;
                    if (idle > 60) break;
                }
                held = 0;
                if (!done) clock_sixtieths += (uint64_t)skip * 60u;
            }
            if (!done && query(key, &got) && compare(got, op, val)) done = true;
            printf("  botloop %s: %d rounds\n", key, r);
            if (!done) {
                failures++;
                fprintf(stderr, "FAIL %s:%d: %s %s %d not reached in %d rounds (got %d)\n", script_name, line_no, key, op, val, rounds, got);
            }
        } else if (!strcmp(cmd, "fake_save")) {
            /* fake_save GAME BYTES : give a cartridge a valid save file of that size */
            char name[64] = {0};
            int bytes = 0;
            sscanf(arg, "%63s %d", name, &bytes);
            int gi = app_find_game(name);
            if (gi < 0 || bytes <= 0 || bytes > 4096) { fail("bad fake_save '%s'%ld", arg, 0); continue; }
            uint8_t blob[4096];
            for (int i = 0; i < bytes; i++) blob[i] = (uint8_t)(i * 7 + gi);
            game_save_write(gi, blob, bytes);
        } else if (!strcmp(cmd, "junk_save")) {
            /* junk_save GAME BYTES : a save file that fails its checks (damaged) */
            char name[64] = {0};
            int bytes = 0;
            sscanf(arg, "%63s %d", name, &bytes);
            int gi = app_find_game(name);
            if (gi < 0 || bytes <= 0 || bytes > 4096) { fail("bad junk_save '%s'%ld", arg, 0); continue; }
            uint8_t blob[4096];
            for (int i = 0; i < bytes; i++) blob[i] = (uint8_t)(0xA5 ^ i);
            char fname[32];
            snprintf(fname, sizeof fname, "game%02d.sav", gi + 1);
            plat_save_write(fname, blob, bytes);
        } else if (!strcmp(cmd, "legacy_progress") || !strcmp(cmd, "untagged_progress")) {
            /* legacy_progress MUSIC SFX LAST SLOT BITS [SLOT BITS ...] : write a
             * progress file in the old 40-slot layout (goals[40], played[40],
             * settings), as a UFO 40 from before the library grew would have
             * left it. SLOT is 1-based; each listed slot counts as played once.
             * untagged_progress takes the same and writes the 50-slot layout
             * from before the NEW tags (v0.7.0 and older). */
            bool tagless = !strcmp(cmd, "untagged_progress");
            int slots = tagless ? MAX_GAMES : LEGACY_GAMES;
            int size = tagless ? PROGRESS_UNTAGGED_SIZE : PROGRESS_LEGACY40_SIZE;
            uint8_t old[PROGRESS_UNTAGGED_SIZE + 16];
            uint8_t *b = old + 16;
            memset(old, 0, sizeof old);
            char tmp[256];
            snprintf(tmp, sizeof tmp, "%s", arg);
            int vals[64], nv = 0;
            for (char *tok = strtok(tmp, " \t"); tok && nv < 64; tok = strtok(NULL, " \t")) vals[nv++] = atoi(tok);
            if (nv < 3 || (nv - 3) % 2) { fail("bad legacy_progress: %s%ld", arg, 0); continue; }
            for (int i = 3; i + 1 < nv; i += 2) {
                int slot = vals[i] - 1;
                if (slot < 0 || slot >= slots) { fail("bad progress slot: %s%ld", arg, vals[i]); continue; }
                b[slot] = (uint8_t)(vals[i + 1] & 7);
                b[slots + slot] = 1;
            }
            uint8_t *st = b + 2 * slots;
            st[0] = (uint8_t)vals[0]; st[1] = (uint8_t)vals[1]; st[2] = 3; st[3] = 0;
            st[4] = (uint8_t)vals[2]; st[5] = 0;
            uint32_t hdr[4] = {0x30344655u, 1u, (uint32_t)size, crc32_buf(b, size)};
            for (int k = 0; k < 4; k++)
                for (int j = 0; j < 4; j++) old[k * 4 + j] = (uint8_t)(hdr[k] >> (8 * j));
            plat_save_write("progress.dat", old, size + 16);
        } else if (!strcmp(cmd, "progress_bytes")) {
            /* progress_bytes N : the progress file on disk holds N bytes of data */
            checks++;
            uint8_t buf[1024];
            int n = plat_save_read("progress.dat", buf, (int)sizeof buf);
            int want = atoi(arg);
            if (n - 16 != want) {
                failures++;
                fprintf(stderr, "FAIL %s:%d: progress file holds %d bytes, expected %d\n", script_name, line_no, n - 16, want);
            }
        } else if (!strcmp(cmd, "forget_cart")) {
            /* forget_cart GAME : the progress file forgets a cartridge, as if
             * it was written by a UFO 40 that didn't have that cartridge yet */
            char name[64] = {0};
            sscanf(arg, "%63s", name);
            int gi = app_find_game(name);
            if (gi < 0) { fail("bad forget_cart '%s'%ld", arg, 0); continue; }
            progress_set_bit(g_progress.known, gi, false);
            progress_set_bit(g_progress.opened, gi, false);
            g_progress.played[gi] = 0;
            progress_save();
        } else if (!strcmp(cmd, "set_goals")) {
            /* set_goals GAME BITS : set a cartridge's goals (1 beacon, 2 saucer, 4 alien) */
            char name[64] = {0};
            int bits = 0;
            sscanf(arg, "%63s %d", name, &bits);
            int gi = app_find_game(name);
            if (gi < 0) { fail("bad set_goals '%s'%ld", arg, 0); continue; }
            g_progress.goals[gi] = (uint8_t)(bits & 7);
            progress_save();
        } else if (!strcmp(cmd, "audio")) {
            audio_on = strncmp(arg, "on", 2) == 0;
        } else if (!strcmp(cmd, "check_songs")) {
            check_songs();
        } else if (!strcmp(cmd, "music")) {
            /* music NAME : start a song by name */
            int id = song_find(arg);
            if (id < 0) fail("unknown song '%s'%ld", arg, 0);
            else music_restart(id);
        } else if (!strcmp(cmd, "wav")) {
            /* wav NAME SECONDS : render the audio output to a 16-bit stereo WAV */
            char name[128];
            float secs = 5;
            sscanf(arg, "%127s %f", name, &secs);
            char wpath[700];
            mkdirs(out_dir);
            snprintf(wpath, sizeof wpath, "%s/%s.wav", out_dir, name);
            FILE *wf = fopen(wpath, "wb");
            if (wf) {
                int total = (int)(secs * AUDIO_RATE);
                wav_header(wf, (uint32_t)total);
                float peak_all = 0;
                for (int done = 0; done < total; done += 800) {
                    int n = total - done < 800 ? total - done : 800;
                    audio_render(audio_buf, n);
                    fwrite(audio_buf, 4, (size_t)n, wf);
                    float pk;
                    audio_stats(&pk, NULL);
                    if (pk > peak_all) peak_all = pk;
                }
                fclose(wf);
                printf("  wav %s (peak %.2f)\n", wpath, peak_all);
            }
        } else if (!strcmp(cmd, "record_start")) {
            /* record_start NAME : every frame from here on as OUT/NAME/000000.png,
             * 000001.png ... (at shotscale) until record_stop */
            char name[128] = {0};
            sscanf(arg, "%127s", name);
            snprintf(rec_dir, sizeof rec_dir, "%s/%s", out_dir, name);
            mkdirs(rec_dir);
            rec_on = true;
            rec_frames = 0;
        } else if (!strcmp(cmd, "record_stop")) {
            if (rec_on) printf("  recorded %d frames in %s\n", rec_frames, rec_dir);
            rec_on = false;
        } else if (!strcmp(cmd, "wav_start")) {
            /* wav_start NAME : the audio of every frame from here on (800
             * samples each, 48 kHz stereo) into OUT/NAME.wav until wav_stop */
            char name[128] = {0}, wpath[700];
            sscanf(arg, "%127s", name);
            wav_stop();
            mkdirs(out_dir);
            snprintf(wpath, sizeof wpath, "%s/%s.wav", out_dir, name);
            wav_f = fopen(wpath, "wb");
            wav_samples = 0;
            wav_peak = 0;
            if (!wav_f) { fail("cannot write %s%ld", wpath, 0); continue; }
            wav_header(wav_f, 0);
        } else if (!strcmp(cmd, "wav_stop")) {
            wav_stop();
        } else if (!strcmp(cmd, "volume")) {
            /* volume MUSIC SFX : 0-10 each, for this run only (not saved) */
            int m = 10, s = 10;
            sscanf(arg, "%d %d", &m, &s);
            audio_set_volume(m, s);
        } else if (!strcmp(cmd, "overlay_text") || !strcmp(cmd, "overlay_box")) {
            /* overlay_text ALIGN(l/c/r, L/C/R on a plate) X Y SCALE(0 = tiny) COLOUR TEXT...
             * overlay_box X Y W H COLOUR (a negative colour darkens N steps)
             * Colours: 0-31 or the sprite letter (w white, y yellow, k ink...). */
            if (ovl_n >= OVL_MAX) { fail("too many overlays%s%ld", "", 0); continue; }
            Overlay *o = &ovl[ovl_n];
            memset(o, 0, sizeof *o);
            char col[8] = {0};
            int used = 0;
            if (cmd[8] == 't') {
                o->align = 'l';
                if (sscanf(arg, " %c %d %d %d %7s %n", &o->align, &o->x, &o->y, &o->scale, col, &used) < 5) {
                    fail("bad overlay_text: %s%ld", arg, 0);
                    continue;
                }
                /* \xNN puts in a glyph (\x88 is a star) */
                char *t = o->text;
                for (const char *s = arg + used; *s && t < o->text + sizeof o->text - 1; s++) {
                    if (s[0] == '\\' && s[1] == 'x' && isxdigit((unsigned char)s[2]) && isxdigit((unsigned char)s[3])) {
                        char hx[3] = {s[2], s[3], 0};
                        *t++ = (char)strtol(hx, NULL, 16);
                        s += 3;
                    } else *t++ = *s;
                }
            } else {
                o->box = true;
                if (sscanf(arg, "%d %d %d %d %7s", &o->x, &o->y, &o->w, &o->h, col) < 5) {
                    fail("bad overlay_box: %s%ld", arg, 0);
                    continue;
                }
            }
            o->col = col[0] == '-' ? -atoi(col + 1) : parse_col(col);
            ovl_n++;
        } else if (!strcmp(cmd, "overlay_clear")) {
            ovl_n = 0;
        } else if (!strcmp(cmd, "log")) {
            printf("  %s\n", arg);
        } else if (!strcmp(cmd, "print")) {
            int v;
            if (query(arg, &v)) printf("  %s = %d\n", arg, v);
            else fail("unknown key '%s'%ld", arg, 0);
        } else {
            fail("unknown command '%s'%ld", cmd, 0);
        }
    }
    fclose(f);
    if (gif_left > 0) gif_end(&gif);
    wav_stop();
    return failures;
}

void vita_assets_render(const char *dir); /* shell/vita_assets.c */

int main(int argc, char **argv) {
    const char *script = NULL, *vita_dir = NULL;
    int frames = 0, shot_every = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--script") && i + 1 < argc) script = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) snprintf(out_dir, sizeof out_dir, "%s", argv[++i]);
        else if (!strcmp(argv[i], "--save-dir") && i + 1 < argc) snprintf(save_dir, sizeof save_dir, "%s", argv[++i]);
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--shot-every") && i + 1 < argc) shot_every = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--vita-assets") && i + 1 < argc) vita_dir = argv[++i];
    }
    rng_seed(&g_rng, 1983);
    app_init();
    if (vita_dir) {
        vita_assets_render(vita_dir);
        return 0;
    }
    if (script) {
        int r = run_script(script);
        printf("%s %s (%d checks)\n", r ? "FAIL" : "PASS", script, checks);
        return r ? 1 : 0;
    }
    for (int f = 1; f <= frames; f++) {
        step(1);
        if (shot_every > 0 && f % shot_every == 0) {
            char name[64];
            snprintf(name, sizeof name, "frame_%05d", f);
            save_shot(name);
        }
    }
    return 0;
}
