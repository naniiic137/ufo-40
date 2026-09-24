/* UFO 40 - minimal scene/state system with palette fades between scenes. */
#ifndef UFO_SCENE_H
#define UFO_SCENE_H

#include <stdbool.h>

typedef struct Scene {
    const char *name;
    void (*enter)(void);
    void (*update)(void);
    void (*draw)(void);
    void (*leave)(void);
} Scene;

void scene_set(const Scene *s);           /* switch immediately */
void scene_goto(const Scene *s);          /* fade out, switch, fade in */
void scene_goto_speed(const Scene *s, int frames_per_step);
const Scene *scene_current(void);
bool scene_transitioning(void);
void scene_update(void);
void scene_draw(void);

#endif
