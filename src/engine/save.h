/* UFO 40 - save data: a global progress file plus per-game blobs.
 * All files are wrapped with a magic, version, length and CRC32. */
#ifndef UFO_SAVE_H
#define UFO_SAVE_H

#include <stdint.h>
#include <stdbool.h>

/* Fifty slots, one for each UFO 50 number (the name stayed 40). Progress
 * files written when there were 40 slots still load: see progress_load. */
#define MAX_GAMES 50
#define LEGACY_GAMES 40

enum { GOAL_BEACON = 1, GOAL_SAUCER = 2, GOAL_ALIEN = 4 };

typedef struct Progress {
    uint8_t goals[MAX_GAMES];   /* GOAL_* bits */
    uint8_t played[MAX_GAMES];  /* times launched (saturating) */
    uint8_t music_vol;          /* 0..10 */
    uint8_t sfx_vol;            /* 0..10 */
    uint8_t scale;              /* PC window scale */
    uint8_t fullscreen;
    uint8_t last_game;
    uint8_t menu_pos;           /* main menu cursor (was reserved: old files read 0) */
    uint8_t reserved[14];
} Progress;

extern Progress g_progress;

uint32_t crc32_buf(const void *data, int len);

void progress_defaults(void);
bool progress_load(void);
/* Size in bytes of the progress record from the 40-slot days (goals[40],
 * played[40], then the same settings bytes). progress_load upgrades it. */
#define PROGRESS_LEGACY40_SIZE (2 * LEGACY_GAMES + 20)
bool progress_save(void);
/* Returns true if the goal was newly earned (queues a toast). */
bool progress_award(int game, int goal_bit);
int progress_goal_count(void);
/* Toast queue for newly earned goals (the shell displays them). */
bool progress_pop_toast(int *game, int *goal_bit);

/* Per-game save blobs ("gameNN.sav"). */
bool game_save_write(int game, const void *data, int len);
int game_save_read(int game, void *data, int len); /* bytes or -1 */
/* Bytes in the save file whether it reads back or not (0 = no file): a
 * file that is there but fails its checks is damaged. */
int game_save_raw_size(int game);
void game_save_erase(int game);

#endif
