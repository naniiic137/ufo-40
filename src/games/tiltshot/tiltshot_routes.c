/* TILTSHOT - a way round every hole, found by the route finder in
 * tiltshot_bot.c and written here by tools/tiltshot/mkroutes.sh. The tests
 * replay them with real presses: every hole can be finished. Each shot is
 * {aim, frames of A, frames into the flight to slam (-1: no slam)}. */
#include "tiltshot.h"

const TshShot TSH_ROUTE[TSH_HOLES][TSH_ROUTE_MAX] = {
    /* 01 OPENING TEE, par 3: 1 strokes */ {{5, 58, -1}, {-1, 0, 0}},
    /* 02 STONE SKIP, par 3: 2 strokes */ {{3, 60, -1}, {0, 0, -1}, {-1, 0, 0}},
    /* 03 BOUNCE HOUSE, par 3: 1 strokes */ {{10, 26, -1}, {-1, 0, 0}},
    /* 04 SKYLARK, par 4: 1 strokes */ {{8, 56, 88}, {-1, 0, 0}},
    /* 05 THE CHUTE, par 4: 1 strokes */ {{0, 32, 40}, {-1, 0, 0}},
    /* 06 THREE STOREYS, par 4: 2 strokes */ {{9, 60, 116}, {3, 56, -1}, {-1, 0, 0}},
    /* 07 DUNE STEPS, par 3: 2 strokes */ {{5, 57, 73}, {8, 44, -1}, {-1, 0, 0}},
    /* 08 FROST TUNNEL, par 4: 2 strokes */ {{4, 59, 9}, {5, 36, -1}, {-1, 0, 0}},
    /* 09 TOSS-UP, par 1: 1 strokes */ {{5, 60, -1}, {-1, 0, 0}},
    /* 10 DOUBLE SPRING, par 3: 1 strokes */ {{1, 56, -1}, {-1, 0, 0}},
    /* 11 PEBBLE, par 2: 1 strokes */ {{0, 40, -1}, {-1, 0, 0}},
    /* 12 ATOLL, par 4: 2 strokes */ {{6, 60, -1}, {4, 44, -1}, {-1, 0, 0}},
    /* 13 BOULDER, par 3: 2 strokes */ {{11, 60, 115}, {2, 60, -1}, {-1, 0, 0}},
    /* 14 BUMPER ALLEY, par 4: 1 strokes */ {{3, 60, -1}, {-1, 0, 0}},
    /* 15 THE HIGH SHELF, par 4: 2 strokes */ {{10, 60, 104}, {6, 58, -1}, {-1, 0, 0}},
    /* 16 NERVE, par 3: 1 strokes */ {{10, 58, 101}, {-1, 0, 0}},
    /* 17 THE PAGODA, par 3: 2 strokes */ {{5, 50, 23}, {2, 56, -1}, {-1, 0, 0}},
    /* 18 LAST ORBIT, par 6: 2 strokes */ {{4, 58, 94}, {10, 60, -1}, {-1, 0, 0}},
};
