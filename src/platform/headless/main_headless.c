/* UFO 40 - headless platform: runs the console with scripted input, dumps
 * PNG screenshots / GIF clips and checks expectations. Used for all tests.
 *
 *   ufo40_headless --script tests/foo.ufs [--out DIR] [--save-dir DIR]
 *   ufo40_headless --frames 600 --shot-every 60 --out DIR
 *   ufo40_headless --vita-assets DIR
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

/* ---- platform services ------------------------------------------------ */

int plat_kind(void) { return PLAT_HEADLESS; }
const char *plat_name(void) { return "HEADLESS"; }
void plat_apply_video(int scale, int fullscreen) { (void)scale; (void)fullscreen; }
void plat_request_quit(void) {}

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

static void step(int n) {
    for (int i = 0; i < n; i++) {
        input_set_raw(held);
        app_update();
        if (audio_on) audio_render(audio_buf, 800);
        if (gif_left > 0) {
            if ((gif_count++ % gif_every) == 0) {
                app_draw();
                int d = gif_every * 100 / 60;
                if (d < 2) d = 2;
                gif_frame(&gif, g_screen.px, d);
            }
            if (--gif_left == 0) gif_end(&gif);
        }
    }
}

static void save_shot(const char *name) {
    char path[640];
    mkdirs(out_dir);
    snprintf(path, sizeof path, "%s/%s.png", out_dir, name);
    app_draw();
    png_write_indexed(path, SCREEN_W, SCREEN_H, g_screen.px, PALETTE_RGB, PAL_COUNT, shot_scale);
    printf("  shot %s\n", path);
}

static uint32_t parse_buttons(char *s) {
    uint32_t m = 0;
    for (char *tok = strtok(s, " +\t\r\n"); tok; tok = strtok(NULL, " +\t\r\n")) {
        if (isdigit((unsigned char)tok[0])) break;
        if (!strcmp(tok, "UP")) m |= BTN_UP;
        else if (!strcmp(tok, "DOWN")) m |= BTN_DOWN;
        else if (!strcmp(tok, "LEFT")) m |= BTN_LEFT;
        else if (!strcmp(tok, "RIGHT")) m |= BTN_RIGHT;
        else if (!strcmp(tok, "A")) m |= BTN_A;
        else if (!strcmp(tok, "B")) m |= BTN_B;
        else if (!strcmp(tok, "START")) m |= BTN_START;
        else if (!strcmp(tok, "SELECT")) m |= BTN_SELECT;
        else if (!strcmp(tok, "ALL")) m |= BTN_ANY;
    }
    return m;
}

/* last integer in a string, or def */
static int last_int(const char *s, int def) {
    const char *p = s + strlen(s);
    while (p > s && !isdigit((unsigned char)p[-1])) p--;
    if (p == s) return def;
    const char *e = p;
    while (p > s && isdigit((unsigned char)p[-1])) p--;
    if (p > s && p[-1] == '-') p--;
    (void)e;
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
        } else if (!strcmp(cmd, "reload")) {
            /* simulate quitting the program and starting it again */
            int gi = game_current_index();
            if (gi >= 0 && scene_current() == &SCENE_RUNNER && GAMES[gi]->quit) GAMES[gi]->quit();
            app_init();
            step(1);
        } else if (!strcmp(cmd, "wipe_saves")) {
            const char *names[] = {"progress.dat", "game01.sav", "game02.sav", "game03.sav"};
            for (int i = 0; i < 4; i++) { char pth[700]; snprintf(pth, sizeof pth, "%s/%s", save_dir, names[i]); remove(pth); }
            progress_defaults();
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
                uint32_t data_bytes = (uint32_t)total * 4;
                uint8_t h[44] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0,
                                 0x80, 0xBB, 0, 0, 0x00, 0xEE, 0x02, 0, 4, 0, 16, 0, 'd', 'a', 't', 'a', 0, 0, 0, 0};
                uint32_t riff = data_bytes + 36;
                memcpy(h + 4, &riff, 4);
                memcpy(h + 40, &data_bytes, 4);
                fwrite(h, 1, 44, wf);
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
        } else if (!strcmp(cmd, "log")) {
            printf("  %s\n", arg);
        } else {
            fail("unknown command '%s'%ld", cmd, 0);
        }
    }
    fclose(f);
    if (gif_left > 0) gif_end(&gif);
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
