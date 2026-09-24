/* UFO 40 - console shell (boot, library, settings, game runner). */
#ifndef UFO_SHELL_H
#define UFO_SHELL_H

#include "../engine/engine.h"
#include "gamedef.h"
#include "ui.h"

extern int MUS_BOOT, MUS_LIBRARY;

extern const Scene SCENE_BOOT, SCENE_LIBRARY, SCENE_SETTINGS, SCENE_RUNNER;

void shell_audio_init(void);

/* App entry points used by every platform. */
void app_init(void);
void app_update(void);
void app_draw(void);
void app_launch_game(int index, bool with_transition);
void app_apply_settings(void);
int app_find_game(const char *id_or_number);
const char *app_scene_name(void);

/* Library selection persists across scenes. */
extern int g_library_cursor;

/* Controls reference text for the current platform. */
const char *shell_controls_text(void);

#endif
