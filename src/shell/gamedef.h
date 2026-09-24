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
} GameDef;

#define GAME_SLOTS 40

extern const GameDef *const GAMES[GAME_SLOTS];

/* Services the shell gives to running games. */
int game_current_index(void);
void game_award(int goal_bit);   /* GOAL_BEACON / GOAL_SAUCER / GOAL_ALIEN */
void game_exit_to_library(void); /* e.g. after the credits */
bool game_paused(void);
/* Games disable the START pause menu on their own title screens. */
void game_set_pausable(bool on);

#endif
