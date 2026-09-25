/* UFO 40 - boot animation: the saucer beams the logo down. */
#include "shell.h"

static int t;

static void boot_enter(void) {
    t = 0;
    music_stop();
}

static void boot_update(void) {
    t++;
    if (t == 16) music_restart(MUS_BOOT);
    if (t > 8 && btnp(BTN_ANY)) {
        sfx_play_name("ui_ok");
        music_fade(10);
        scene_goto(&SCENE_MENU);
        return;
    }
    if (t > 330) scene_goto_speed(&SCENE_MENU, 3);
}

static int saucer_x(void) {
    if (t < 70) {
        float k = (float)t / 70.0f;
        float e = 1.0f - (1.0f - k) * (1.0f - k) * (1.0f - k);
        return (int)(-50 + (139 + 50) * e);
    }
    if (t < 170) return 139;
    float k = (float)(t - 170) / 40.0f;
    return 139 + (int)(k * k * 260);
}

static int saucer_y(void) {
    if (t < 70) return 18 + (int)(sinf((float)t * 0.12f) * 10.0f * (1.0f - (float)t / 70.0f));
    if (t < 170) return 18 + (int)(sinf((float)t * 0.08f) * 2.0f);
    float k = (float)(t - 170) / 40.0f;
    return 18 - (int)(k * k * 70);
}

static void boot_draw(void) {
    ui_starfield(t, C_INK);
    int sx = saucer_x(), sy = saucer_y();
    int beam_on = t >= 72 && t < 168;
    int reveal = 0;
    if (t >= 84) reveal = (t - 84) * 2;
    if (reveal > 60) reveal = 60;

    /* tractor beam */
    if (beam_on) {
        int top = sy + 20, bot = 108;
        int flick = (t / 2) % 3;
        for (int y = top; y < bot; y++) {
            float k = (float)(y - top) / (float)(bot - top);
            int hw = 8 + (int)(k * 86);
            int lvl = 5 + flick + (int)(k * 3);
            if (t < 80) lvl = (t - 72);
            if (t > 160) lvl = (168 - t);
            gfx_dither(160 - hw, y, hw * 2, 1, C_BLUE, lvl);
            gfx_dither(160 - hw / 2, y, hw, 1, C_CYAN, lvl - 3);
        }
        /* sparkles rising in the beam */
        for (int i = 0; i < 14; i++) {
            int px = 160 + (int)(sinf((float)(i * 37 + t) * 0.07f) * (10 + i * 5));
            int py = 108 - ((t * 2 + i * 23) % 70);
            if (py > top) gfx_pset(px, py, (i + t / 3) % 3 ? C_ICE : C_WHITE);
        }
    }

    /* logo, revealed top-down while the beam is on */
    if (reveal > 0) {
        gfx_clip(0, 0, SCREEN_W, 50 + reveal);
        ui_logo(160, 54, 4, t);
        gfx_noclip();
        if (reveal < 60) gfx_hline(40, 280, 50 + reveal, C_ICE);
    }

    ui_saucer(sx, sy, t, 2);

    if (t > 176) {
        int a = t - 176;
        int col = a < 4 ? C_DUSK : a < 8 ? C_SLATE : a < 12 ? C_GREY : C_LIGHT;
        text_center("BEAMDOWN SOFTWORKS", 160, 118, col);
        tiny_center("FORTY GAMES FROM ANOTHER WORLD", 160, 130, a < 12 ? C_DUSK : C_SKY);
    }
    if (t > 210 && (t / 20) % 2 == 0) text_center("PRESS ANY BUTTON", 160, 146, C_YELLOW);
    tiny_center("A FAN-MADE PARODY TRIBUTE", 160, 170, C_DUSK);
}

const Scene SCENE_BOOT = {"boot", boot_enter, boot_update, boot_draw, NULL};
