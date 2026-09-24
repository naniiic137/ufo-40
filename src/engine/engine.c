#include "engine.h"

static uint32_t frame_counter;

void engine_init(void) {
    gfx_init();
    font_init();
    audio_init();
    frame_counter = 0;
}

void engine_update(void) {
    input_update();
    scene_update();
    gfx_tick_effects();
    frame_counter++;
}

void engine_draw(void) {
    gfx_set_target(NULL);
    scene_draw();
}

uint32_t engine_frame(void) { return frame_counter; }
