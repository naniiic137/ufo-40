/* UFO 40 - console shell (boot, main menu, library, options, jukebox, save
 * data, game runner). */
#ifndef UFO_SHELL_H
#define UFO_SHELL_H

#include "../engine/engine.h"
#include "gamedef.h"
#include "ui.h"

extern int MUS_BOOT, MUS_LIBRARY;

extern const Scene SCENE_BOOT, SCENE_MENU, SCENE_LIBRARY, SCENE_SETTINGS, SCENE_JUKEBOX, SCENE_SAVEDATA,
    SCENE_CONTROLS, SCENE_CREDITS, SCENE_RUNNER;

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

/* Controls reference text for the current platform (the pause menu's
 * CONTROLS page lists each game's own moves instead). */
const char *shell_controls_text(void);

/* Options: remembers which screen opened it (the main menu or the library). */
void shell_open_options(const Scene *back);

/* Menu music: the console theme, unless the jukebox is playing a pick. */
void shell_menu_music(void);
/* The song the jukebox started (-1 = none). */
extern int g_jukebox_song;
/* Which cartridge slot defined a song (-1 = the console itself). */
int shell_song_owner(int song);
/* A friendly title for a song ("ROOFTOP RUN"). */
const char *shell_song_title(int song);

/* Save data helpers. */
int shell_save_size(int game); /* bytes in the cartridge's save, 0 if none */
void shell_delete_save(int game);
void shell_reset_goals(int game);
void shell_delete_all(void);   /* every save and goal; settings stay */

/* State for the headless tests ("menu_sel", "music_vol", ...). */
bool shell_query(const char *key, int *out);

#endif
