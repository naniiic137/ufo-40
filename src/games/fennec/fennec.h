/* FENNEC FOUNTAIN - shared declarations. Cartridge 15 of UFO 40.
 * A tribute to Block Koala (UFO 50 #15); see docs/games/15-fennec-fountain.md. */
#ifndef FENNEC_H
#define FENNEC_H

#ifndef FN_NO_SHELL
#include "../../shell/gamedef.h"
#include "../../shell/ui.h"
#else
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#endif

#define FN_W 16
#define FN_H 9
#define FN_MAX_BLOCKS 40
#define FN_MAX_GECKOS 4
#define FN_ROOMS 50
#define FN_CUSTOM 10
#define FN_NONE 255

/* block kinds */
enum { BK_NONE = 0, BK_SAND, BK_LAPIS, BK_BASALT, BK_STONE, BK_MARBLE };
enum { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT };

typedef struct FnBlock {
    uint8_t kind, n, x, y; /* a basalt block covers n x n tiles from (x, y) */
} FnBlock;

/* the fixed part of a room */
typedef struct FnRoom {
    uint8_t w, h;
    uint8_t wall[FN_H][FN_W];
    uint8_t goal_x, goal_y;
    uint8_t door_x, door_y;  /* the way out (FN_NONE when there is none) */
} FnRoom;

/* everything that moves */
typedef struct FnState {
    uint8_t px, py;
    uint8_t ng;
    uint8_t gx[FN_MAX_GECKOS], gy[FN_MAX_GECKOS];
    uint8_t nb;
    FnBlock b[FN_MAX_BLOCKS];
    int8_t push_blk;         /* the basalt block the fennec is pushing, or -1 */
    int8_t push_dir;
    uint8_t won, exited;
} FnState;

/* what happened in a step, for the animation */
enum { FE_WALK, FE_BUMP, FE_PUSH, FE_MERGE, FE_SHRINK, FE_CRUMBLE, FE_GECKO, FE_MARBLE, FE_WIN, FE_EXIT };
typedef struct FnEvent {
    uint8_t type, a;         /* a: block index (after the step) or gecko index */
    int8_t x, y, x2, y2;
} FnEvent;
typedef struct FnEvents {
    FnEvent ev[64];
    int n;
} FnEvents;

/* rules (fennec_logic.c) */
int fn_parse(const char *const *rows, FnRoom *room, FnState *st); /* returns 0 on success */
bool fn_step(const FnRoom *r, FnState *s, int dir, FnEvents *ev);
int fn_weight(const FnBlock *b);
int fn_block_at(const FnState *s, int x, int y); /* index or -1 */
bool fn_solution_check(const FnRoom *r, const FnState *start, const char *moves);

#ifndef FN_NO_SHELL
/* the rooms (fennec_rooms.c) */
typedef struct RoomDef {
    const char *name;
    const char *rows[FN_H];
    const char *solution; /* U R D L, checked by the tests */
} RoomDef;
extern const RoomDef FN_ROOMS_DEF[FN_ROOMS];

/* art & audio */
enum {
    FS_FEN_D, FS_FEN_D2, FS_FEN_U, FS_FEN_U2, FS_FEN_S, FS_FEN_S2, FS_FEN_PUSH,
    FS_ZIZI, FS_HUMPH, FS_GECKO1, FS_GECKO2,
    FS_STONE, FS_SPRING_DRY, FS_SPRING_WET, FS_DROP, FS_PALM, FS_DOOR, FS_LOCK, FS_CURSOR,
    FS_SPRITE_COUNT
};
extern Sprite fn_spr[FS_SPRITE_COUNT];
void fn_art_load(void);
void fn_audio_load(void);
extern int FN_MUS_HUB, FN_MUS_ROOM, FN_MUS_PALACE, FN_MUS_WIN, FN_MUS_EDIT, FN_MUS_END;
#endif

#endif
