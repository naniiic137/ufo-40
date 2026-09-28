/* UFO 40 - the contract between the console shell and each game cartridge. */
#ifndef UFO_GAMEDEF_H
#define UFO_GAMEDEF_H

#include "../engine/engine.h"

typedef struct GameDef {
    const char *id;          /* short id for scripts: "underdelve" */
    const char *title;       /* shown on the cartridge */
    const char *year;        /* fictional release year */
    const char *genre;
    const char *blurb;       /* word-wrapped in the library panel */
    const char *goal_desc[3];/* beacon, saucer, alien */
    const char *controls;    /* lines separated by \n, shown in pause menu */
    uint8_t cart_main, cart_accent; /* cartridge colours */

    void (*load)(void);      /* one-time asset setup (idempotent) */
    void (*start)(void);     /* enter the game (its own title screen) */
    void (*update)(void);
    void (*draw)(void);
    void (*quit)(void);      /* leaving to the library: persist state */
    /* Cartridge label art inside (x,y,w,h). t = animation frame. */
    void (*draw_label)(int x, int y, int w, int h, int t);

    /* Test hooks for the headless runner. */
    int (*query)(const char *key, int *out); /* returns 1 if key known */
    int (*cheat)(const char *cmd);           /* returns 1 if handled */

    /* The UFO 50 game this cartridge pays tribute to, and its number there.
     * A cartridge lives in the library slot with that same number. */
    const char *tribute;
    int tribute_no;

    /* Optional: a cartridge whose world keeps going in real time while the
     * console is switched on (the base in slot 44 keeps producing during
     * other games and in the menus, as its original does while the whole
     * collection is open). The shell calls it once per update, in every
     * scene and whichever cartridge is running, with the real milliseconds
     * since the last call, measured on the platform's wall clock: a stalled
     * frame loop hands over all the time it missed at once. Time while the
     * console is off never counts (the first call after start-up gets 0).
     * The cartridge keeps and saves its own accounting. NULL for most. */
    void (*realtime)(uint32_t ms);
} GameDef;

/* One slot per UFO 50 number: 01-50. */
#define GAME_SLOTS MAX_GAMES

extern const GameDef *const GAMES[GAME_SLOTS];

/* Services the shell gives to running games. */
int game_current_index(void);
void game_award(int goal_bit);   /* GOAL_BEACON / GOAL_SAUCER / GOAL_ALIEN */
void game_exit_to_library(void); /* e.g. after the credits */
bool game_paused(void);
/* Games disable the START pause menu on their own title screens. */
void game_set_pausable(bool on);
/* A game can add up to two items of its own to the START pause menu, under
 * RESUME (e.g. "RESTART LEVEL"); picking one unpauses and calls pick(i).
 * n = 0 removes them. They are cleared whenever a game starts. */
void game_pause_items(int n, const char *const *items, void (*pick)(int i));

#endif
