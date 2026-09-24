#include "scene.h"
#include "gfx.h"
#include "input.h"
#include <stddef.h>

static const Scene *current, *pending;
static int phase;  /* 0 idle, 1 fading out, 2 fading in */
static int step_timer, step_len = 2, fade;

void scene_set(const Scene *s) {
    if (current && current->leave) current->leave();
    current = s;
    phase = 0;
    gfx_set_fade(0);
    input_consume();
    if (current && current->enter) current->enter();
}

void scene_goto_speed(const Scene *s, int frames_per_step) {
    pending = s;
    phase = 1;
    fade = gfx_get_fade();
    step_len = frames_per_step < 1 ? 1 : frames_per_step;
    step_timer = step_len;
}

void scene_goto(const Scene *s) { scene_goto_speed(s, 2); }

const Scene *scene_current(void) { return current; }
bool scene_transitioning(void) { return phase != 0; }

void scene_update(void) {
    if (phase == 1) {
        if (--step_timer <= 0) {
            step_timer = step_len;
            fade++;
            gfx_set_fade(fade);
            if (fade >= 7) {
                if (current && current->leave) current->leave();
                current = pending;
                pending = NULL;
                input_consume();
                if (current && current->enter) current->enter();
                phase = 2;
            }
        }
        return; /* the outgoing scene is frozen while fading */
    }
    if (phase == 2) {
        if (--step_timer <= 0) {
            step_timer = step_len;
            fade--;
            gfx_set_fade(fade);
            if (fade <= 0) { phase = 0; gfx_set_fade(0); }
        }
    }
    if (current && current->update) current->update();
}

void scene_draw(void) {
    if (current && current->draw) current->draw();
}
