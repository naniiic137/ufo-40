/* UFO 40 - SDL2 platform layer shared by Windows/Linux PC, PS Vita and the
 * web build (Emscripten -sUSE_SDL=2).
 *
 * The engine renders a 320x180 indexed framebuffer; we convert it through a
 * 32-entry palette LUT into a streaming texture and draw it integer-scaled. */
#include "../../shell/shell.h"

#if defined(__vita__) || defined(__EMSCRIPTEN__)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#ifdef __vita__
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#endif
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;
static SDL_AudioDeviceID adev;
static SDL_GameController *pads[2];
static SDL_Joystick *joy;
static uint32_t lut[PAL_COUNT];
static bool quit_flag;
static Uint64 perf_freq, last_counter;
static double accum;
static char save_dir[1024];
static int cur_scale = -1, cur_full = -1;
static volatile uint32_t touch_mask; /* web on-screen buttons */

/* ------------------------------------------------------------------ */
/* platform services                                                    */

int plat_kind(void) {
#if defined(__vita__)
    return PLAT_VITA;
#elif defined(__EMSCRIPTEN__)
    return PLAT_WEB;
#else
    return PLAT_PC;
#endif
}

const char *plat_name(void) {
    static const char *names[] = {"PC", "VITA", "WEB", "HEADLESS"};
    return names[plat_kind()];
}

void plat_request_quit(void) { quit_flag = true; }

const char *plat_save_where(void) {
#ifdef __EMSCRIPTEN__
    return "THIS BROWSER (LOCALSTORAGE)";
#else
    return save_dir[0] ? save_dir : "SAVES";
#endif
}

#ifdef __EMSCRIPTEN__
/* Web saves live in localStorage as hex strings (tiny files, synchronous). */
int plat_save_write(const char *name, const void *data, int len) {
    char *hex = (char *)malloc((size_t)len * 2 + 1);
    static const char *digits = "0123456789abcdef";
    const uint8_t *p = (const uint8_t *)data;
    for (int i = 0; i < len; i++) { hex[i * 2] = digits[p[i] >> 4]; hex[i * 2 + 1] = digits[p[i] & 15]; }
    hex[len * 2] = 0;
    int ok = EM_ASM_INT({
        try { localStorage.setItem("ufo40/" + UTF8ToString($0), UTF8ToString($1)); return 1; }
        catch (e) { return 0; }
    }, name, hex);
    free(hex);
    return ok ? 0 : -1;
}

int plat_save_read(const char *name, void *data, int maxlen) {
    int n = EM_ASM_INT({
        var v = null;
        try { v = localStorage.getItem("ufo40/" + UTF8ToString($0)); } catch (e) { v = null; }
        if (v === null) return -1;
        var bytes = v.length >> 1;
        if (bytes > $2) bytes = $2;
        for (var i = 0; i < bytes; i++) HEAPU8[$1 + i] = parseInt(v.substr(i * 2, 2), 16);
        return bytes;
    }, name, data, maxlen);
    return n;
}
#else
static void make_dir(const char *p) {
#if defined(__vita__)
    sceIoMkdir(p, 0777);
#elif defined(_WIN32)
    _mkdir(p);
#else
    mkdir(p, 0755);
#endif
}

static void init_save_dir(void) {
#if defined(__vita__)
    snprintf(save_dir, sizeof save_dir, "ux0:data/UFO40");
    make_dir("ux0:data");
    make_dir(save_dir);
#else
    /* portable mode: a file named "portable.txt" next to the exe keeps saves there */
    char *base = SDL_GetBasePath();
    char probe[1024];
    snprintf(probe, sizeof probe, "%sportable.txt", base ? base : "");
    FILE *f = fopen(probe, "rb");
    if (f) {
        fclose(f);
        snprintf(save_dir, sizeof save_dir, "%ssaves", base ? base : "");
        make_dir(save_dir);
    } else {
#ifdef _WIN32
        const char *appdata = getenv("APPDATA");
        if (appdata) {
            snprintf(save_dir, sizeof save_dir, "%s\\UFO40", appdata);
            make_dir(save_dir);
        } else {
            snprintf(save_dir, sizeof save_dir, "%ssaves", base ? base : "");
            make_dir(save_dir);
        }
#else
        char *pref = SDL_GetPrefPath("Beamdown", "UFO40");
        if (pref) {
            snprintf(save_dir, sizeof save_dir, "%s", pref);
            size_t n = strlen(save_dir);
            if (n && (save_dir[n - 1] == '/' || save_dir[n - 1] == '\\')) save_dir[n - 1] = 0;
            SDL_free(pref);
        } else {
            snprintf(save_dir, sizeof save_dir, "saves");
            make_dir(save_dir);
        }
#endif
    }
    if (base) SDL_free(base);
#endif
}

int plat_save_write(const char *name, const void *data, int len) {
    char path[1200], tmp[1210];
    snprintf(path, sizeof path, "%s/%s", save_dir, name);
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return -1;
    size_t w = len > 0 ? fwrite(data, 1, (size_t)len, f) : 0;
    fclose(f);
    if ((int)w != len) return -1;
    remove(path);
    if (rename(tmp, path) != 0) return -1;
    return 0;
}

int plat_save_read(const char *name, void *data, int maxlen) {
    char path[1200];
    snprintf(path, sizeof path, "%s/%s", save_dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    int n = (int)fread(data, 1, (size_t)maxlen, f);
    fclose(f);
    return n;
}
#endif

void plat_apply_video(int scale, int fullscreen) {
#if !defined(__vita__) && !defined(__EMSCRIPTEN__)
    if (!win) return;
    if (fullscreen != cur_full) {
        SDL_SetWindowFullscreen(win, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        cur_full = fullscreen;
        if (!fullscreen) cur_scale = -1; /* re-apply size */
    }
    if (!fullscreen && scale != cur_scale) {
        SDL_SetWindowSize(win, SCREEN_W * scale, SCREEN_H * scale);
        SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        cur_scale = scale;
    }
#else
    (void)scale;
    (void)fullscreen;
#endif
}

/* Web: on-screen touch buttons call this through Module._ufo_web_button */
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void ufo_web_button(int mask, int down) {
    if (down) touch_mask |= (uint32_t)mask;
    else touch_mask &= ~(uint32_t)mask;
}

/* ------------------------------------------------------------------ */
/* audio                                                                */

static void audio_cb(void *ud, Uint8 *stream, int len) {
    (void)ud;
    audio_render((int16_t *)stream, len / 4);
}
static void a_lock(void) { if (adev) SDL_LockAudioDevice(adev); }
static void a_unlock(void) { if (adev) SDL_UnlockAudioDevice(adev); }

static void open_audio(void) {
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = AUDIO_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
#ifdef __EMSCRIPTEN__
    want.samples = 2048;
#else
    want.samples = 1024;
#endif
    want.callback = audio_cb;
    adev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!adev) {
        SDL_Log("audio unavailable: %s", SDL_GetError());
        return;
    }
    if (have.freq != AUDIO_RATE || have.format != AUDIO_S16SYS || have.channels != 2) {
        /* SDL converts for us when allowed_changes = 0, so this should not happen */
        SDL_Log("audio format differs (%d Hz)", have.freq);
    }
    audio_set_lock(a_lock, a_unlock);
    SDL_PauseAudioDevice(adev, 0);
}

/* ------------------------------------------------------------------ */
/* input                                                                */

/* Up to two gamepads. Normally both drive the console; in versus mode the
 * second one is player 2. */
static void open_pads(void) {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(i);
            int slot = -1;
            for (int p = 1; p >= 0; p--) {
                if (pads[p] && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pads[p])) == id) { slot = -2; break; }
                if (!pads[p]) slot = p;
            }
            if (slot >= 0) pads[slot] = SDL_GameControllerOpen(i);
        } else if (!joy) {
            joy = SDL_JoystickOpen(i);
        }
    }
}

static void close_lost_pads(void) {
    for (int p = 0; p < 2; p++)
        if (pads[p] && !SDL_GameControllerGetAttached(pads[p])) {
            SDL_GameControllerClose(pads[p]);
            pads[p] = NULL;
        }
    if (!pads[0] && pads[1]) { pads[0] = pads[1]; pads[1] = NULL; }
}

static uint32_t pad_mask(SDL_GameController *pad) {
    uint32_t m = 0;
    if (!pad) return 0;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP)) m |= BTN_UP;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) m |= BTN_DOWN;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) m |= BTN_LEFT;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) m |= BTN_RIGHT;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A)) m |= BTN_A;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B)) m |= BTN_B;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START)) m |= BTN_START;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_BACK)) m |= BTN_SELECT;
    int ax = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
    int ay = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);
    if (ax < -16000) m |= BTN_LEFT;
    if (ax > 16000) m |= BTN_RIGHT;
    if (ay < -16000) m |= BTN_UP;
    if (ay > 16000) m |= BTN_DOWN;
    return m;
}

static uint32_t read_input(void) {
    uint32_t m = 0, m2 = 0;
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    SDL_Keymod mod = SDL_GetModState();
    bool vs = input_versus();
    if (((k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_KP_ENTER]) && !(mod & KMOD_ALT)) || k[SDL_SCANCODE_ESCAPE])
        m |= BTN_START;
    if (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT] || k[SDL_SCANCODE_BACKSPACE]) m |= BTN_SELECT;
    if (!vs) {
        if (k[SDL_SCANCODE_UP] || k[SDL_SCANCODE_W]) m |= BTN_UP;
        if (k[SDL_SCANCODE_DOWN] || k[SDL_SCANCODE_S]) m |= BTN_DOWN;
        if (k[SDL_SCANCODE_LEFT] || k[SDL_SCANCODE_A]) m |= BTN_LEFT;
        if (k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D]) m |= BTN_RIGHT;
        if (k[SDL_SCANCODE_Z] || k[SDL_SCANCODE_J] || k[SDL_SCANCODE_SPACE]) m |= BTN_A;
        if (k[SDL_SCANCODE_X] || k[SDL_SCANCODE_K]) m |= BTN_B;
    } else {
        /* split keyboard: player 1 on the left half, player 2 on the right */
        if (k[SDL_SCANCODE_W]) m |= BTN_UP;
        if (k[SDL_SCANCODE_S]) m |= BTN_DOWN;
        if (k[SDL_SCANCODE_A]) m |= BTN_LEFT;
        if (k[SDL_SCANCODE_D]) m |= BTN_RIGHT;
        if (k[SDL_SCANCODE_F] || k[SDL_SCANCODE_Z] || k[SDL_SCANCODE_SPACE]) m |= BTN_A;
        if (k[SDL_SCANCODE_G] || k[SDL_SCANCODE_X]) m |= BTN_B;
        if (k[SDL_SCANCODE_UP]) m2 |= BTN_UP;
        if (k[SDL_SCANCODE_DOWN]) m2 |= BTN_DOWN;
        if (k[SDL_SCANCODE_LEFT]) m2 |= BTN_LEFT;
        if (k[SDL_SCANCODE_RIGHT]) m2 |= BTN_RIGHT;
        if (k[SDL_SCANCODE_K] || k[SDL_SCANCODE_PERIOD]) m2 |= BTN_A;
        if (k[SDL_SCANCODE_L] || k[SDL_SCANCODE_SLASH]) m2 |= BTN_B;
    }
    if (pads[0] || pads[1]) {
        m |= pad_mask(pads[0]);
        uint32_t p2 = pad_mask(pads[1]);
        if (vs) {
            m2 |= p2 & ~(uint32_t)BTN_START;
            m |= p2 & BTN_START; /* either player can pause */
        } else {
            m |= p2;
        }
    } else if (joy) {
#ifdef __vita__
        /* SDL's Vita button order: triangle, circle, cross, square, L, R, down, left, up, right, select, start */
        static const int map[12] = {0, BTN_B, BTN_A, 0, 0, 0, BTN_DOWN, BTN_LEFT, BTN_UP, BTN_RIGHT, BTN_SELECT, BTN_START};
        for (int i = 0; i < 12 && i < SDL_JoystickNumButtons(joy); i++)
            if (SDL_JoystickGetButton(joy, i)) m |= (uint32_t)map[i];
#else
        if (SDL_JoystickNumButtons(joy) > 0 && SDL_JoystickGetButton(joy, 0)) m |= BTN_A;
        if (SDL_JoystickNumButtons(joy) > 1 && SDL_JoystickGetButton(joy, 1)) m |= BTN_B;
        if (SDL_JoystickNumHats(joy) > 0) {
            Uint8 h = SDL_JoystickGetHat(joy, 0);
            if (h & SDL_HAT_UP) m |= BTN_UP;
            if (h & SDL_HAT_DOWN) m |= BTN_DOWN;
            if (h & SDL_HAT_LEFT) m |= BTN_LEFT;
            if (h & SDL_HAT_RIGHT) m |= BTN_RIGHT;
        }
#endif
        if (SDL_JoystickNumAxes(joy) >= 2) {
            int ax = SDL_JoystickGetAxis(joy, 0), ay = SDL_JoystickGetAxis(joy, 1);
            if (ax < -16000) m |= BTN_LEFT;
            if (ax > 16000) m |= BTN_RIGHT;
            if (ay < -16000) m |= BTN_UP;
            if (ay > 16000) m |= BTN_DOWN;
        }
    }
    return m | touch_mask | (m2 << BTN_P2_SHIFT);
}

/* ------------------------------------------------------------------ */
/* frame                                                                */

static void toggle_fullscreen(void) {
    g_progress.fullscreen ^= 1;
    app_apply_settings();
    progress_save();
}

static void handle_events(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT: quit_flag = true; break;
        case SDL_KEYDOWN:
            if (!e.key.repeat) {
                if (e.key.keysym.scancode == SDL_SCANCODE_F11) toggle_fullscreen();
                if ((e.key.keysym.scancode == SDL_SCANCODE_RETURN) && (e.key.keysym.mod & KMOD_ALT)) toggle_fullscreen();
            }
            break;
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_JOYDEVICEADDED:
            open_pads();
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            close_lost_pads();
            open_pads();
            break;
        default: break;
        }
    }
}

static void present(void) {
    void *pixels;
    int pitch;
    gfx_build_lut(lut, 1);
    if (SDL_LockTexture(tex, NULL, &pixels, &pitch) == 0) {
        gfx_present((uint32_t *)pixels, pitch / 4, lut);
        SDL_UnlockTexture(tex);
    }
    int ow, oh;
    SDL_GetRendererOutputSize(ren, &ow, &oh);
    int s = ow / SCREEN_W < oh / SCREEN_H ? ow / SCREEN_W : oh / SCREEN_H;
    if (s < 1) s = 1;
    SDL_Rect dst = {(ow - SCREEN_W * s) / 2, (oh - SCREEN_H * s) / 2, SCREEN_W * s, SCREEN_H * s};
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_RenderPresent(ren);
}

static long smoke_frames = -1; /* --frames N: quit after N frames (CI smoke test) */

static void frame(void) {
    handle_events();
    if (smoke_frames >= 0 && --smoke_frames < 0) quit_flag = true;
    Uint64 now = SDL_GetPerformanceCounter();
    double dt = (double)(now - last_counter) / (double)perf_freq;
    last_counter = now;
    if (dt > 0.25) dt = 0.25;
    const double step = 1.0 / 60.0;
    /* Vsync snapping. On a ~60 Hz display (the Vita, most PCs, the browser)
     * every presented frame lasts one step plus timer noise. Feeding that noise
     * into the accumulator makes it straddle the step boundary, so some frames
     * run 0 updates and the next runs 2, and scrolling judders. Frames that
     * last a whole number of steps (within 1 ms) run exactly that many updates
     * and leave the accumulator alone, so there is no drift either. Other
     * refresh rates (120 Hz, 144 Hz, no vsync) use the accumulator. */
    int want = 0;
    for (int k = 1; k <= 3 && !want; k++)
        if (fabs(dt - k * step) < 0.001) want = k;
    if (!want) {
        accum += dt;
        while (accum >= step && want < 5) { accum -= step; want++; }
        if (want == 5) accum = 0;
    }
    int steps = 0;
    for (; steps < want; steps++) {
        input_set_raw(read_input());
        app_update();
    }
    if (steps > 0) {
        app_draw();
    }
    present();
#ifdef __EMSCRIPTEN__
    if (quit_flag) emscripten_cancel_main_loop();
#endif
}

int main(int argc, char **argv) {
    for (int i = 1; i + 1 < argc; i++)
        if (!strcmp(argv[i], "--frames")) smoke_frames = atol(argv[i + 1]);
#ifdef __vita__
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    /* Use SDL's native GXM renderer (precompiled shaders). The GLES2 one would
       need the user to have extracted libshacccg.suprx on the Vita. */
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "VITA gxm");
#endif
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
        /* try again without audio / pads */
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
            return 1;
        }
    }
#ifndef __EMSCRIPTEN__
    init_save_dir();
#endif
    rng_seed(&g_rng, (uint64_t)SDL_GetTicks() ^ 0x5f3759dfULL);
    app_init();

    int w = SCREEN_W * 3, h = SCREEN_H * 3;
    Uint32 flags = SDL_WINDOW_SHOWN;
#if defined(__vita__)
    w = 960;
    h = 544;
#elif defined(__EMSCRIPTEN__)
    w = SCREEN_W;
    h = SCREEN_H;
#else
    w = SCREEN_W * g_progress.scale;
    h = SCREEN_H * g_progress.scale;
    flags |= SDL_WINDOW_RESIZABLE;
#endif
    win = SDL_CreateWindow("UFO 40", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
    if (!win) {
        SDL_Log("window failed: %s", SDL_GetError());
        return 1;
    }
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(win, -1, 0);
    if (!ren) {
        SDL_Log("renderer failed: %s", SDL_GetError());
        return 1;
    }
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
    SDL_SetWindowMinimumSize(win, SCREEN_W, SCREEN_H);
    cur_scale = g_progress.scale;
    cur_full = 0;
    app_apply_settings();
    open_pads();
    open_audio();

    perf_freq = SDL_GetPerformanceFrequency();
    last_counter = SDL_GetPerformanceCounter();
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (!quit_flag) frame();
    progress_save();
    if (adev) SDL_CloseAudioDevice(adev);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
#endif
#ifdef __vita__
    sceKernelExitProcess(0);
#endif
    return 0;
}
