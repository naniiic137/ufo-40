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

/* The UFO 50 game behind every slot (index 0 = UFO 50 #1), in capitals. A
 * loaded cartridge credits its own GameDef.tribute; the grey placeholder in
 * an empty slot credits the game from this table. */
extern const char *const UFO50_TITLES[GAME_SLOTS];

/* NEW tags. After the progress file loads, every cartridge in GAMES[] that
 * the file didn't know yet is recorded as known; it is NEW until it is first
 * started, except on a brand-new save (nothing is NEW) and when a save from
 * before the tags is upgraded (only the latest batch is NEW). */
void shell_sync_cartridges(void);
bool shell_cart_is_new(int slot);
int shell_new_count(void);

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
enum { SAVE_NONE, SAVE_OK, SAVE_DAMAGED };
int shell_save_state(int game);
void shell_delete_save(int game);
void shell_reset_goals(int game); /* the goals and the save that would give them back */
void shell_delete_all(void);   /* every save and goal; settings stay */
/* The question and detail line for deleting cartridge g's save. */
void shell_delete_save_question(int g, char *q, int qn, char *d, int dn);

/* Button layouts, one per cartridge: the A, B and SELECT jobs can move
 * between the A, B and SELECT buttons (START always pauses). Saved in the
 * progress file; the runner applies the running cartridge's layout through
 * the input layer, except in the pause menu. */
enum { JOB_A, JOB_B, JOB_SELECT, JOB_COUNT };
int shell_remap_button(int slot, int job); /* the JOB_* button doing that job */
bool shell_remap_default(int slot);
void shell_remap_cycle(int slot, int job); /* the job moves to the next button */
void shell_remap_reset(int slot);
void shell_remap_apply(int slot);          /* -1: every button is itself */
/* The cartridge's controls list with its A, B and SELECT as now pressed. */
void shell_controls_for(int slot, char *out, int n);

/* The cartridge card: a panel over the library (SELECT on a cartridge). */
void cartinfo_open(int slot);
bool cartinfo_active(void);
void cartinfo_update(void);
void cartinfo_draw(void);

/* State for the headless tests ("menu_sel", "music_vol", ...). */
bool shell_query(const char *key, int *out);
/* Tests and screenshots: open the running game's pause menu at a page
 * (0 the menu, 1 controls, 2 restart?) even where the game doesn't allow
 * pausing yet, or show its goal toast for a goal bit. */
void app_test_pause(int page);
void app_test_toast(int goal_bit);

#endif
