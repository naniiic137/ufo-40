/* TILTSHOT - a way round every hole, found by the route finder in
 * tiltshot_bot.c and written here by tools/tiltshot/mkroutes.sh. The tests
 * replay them with real presses: every hole can be finished. Each shot is
 * {aim, frames of A, frames into the flight to slam (-1: no slam)}. */
#include "tiltshot.h"

const TshShot TSH_ROUTE[TSH_HOLES][TSH_ROUTE_MAX] = {
    /* 01 OPENING TEE, par 3: 1 strokes */ {{6, 56, 120}, {-1, 0, 0}},
    /* 02 STONE SKIP, par 3: 2 strokes */ {{11, 58, 66}, {1, 28, -1}, {-1, 0, 0}},
    /* 03 BOUNCE HOUSE, par 3: 1 strokes */ {{8, 48, 51}, {-1, 0, 0}},
    /* 04 SKYLARK, par 4: 2 strokes */ {{10, 56, 82}, {7, 52, -1}, {-1, 0, 0}},
    /* 05 THE CHUTE, par 4: 2 strokes */ {{10, 60, 120}, {3, 48, -1}, {-1, 0, 0}},
    /* 06 THREE STOREYS, par 4: 2 strokes */ {{9, 50, 30}, {8, 44, -1}, {-1, 0, 0}},
    /* 07 DUNE STEPS, par 3: 2 strokes */ {{5, 57, 73}, {9, 52, -1}, {-1, 0, 0}},
    /* 08 FROST TUNNEL, par 4: 2 strokes */ {{4, 59, 9}, {5, 36, -1}, {-1, 0, 0}},
    /* 09 TOSS-UP, par 1: 1 strokes */ {{10, 60, -1}, {-1, 0, 0}},
    /* 10 DOUBLE SPRING, par 3: 1 strokes */ {{2, 56, 104}, {-1, 0, 0}},
    /* 11 PEBBLE, par 2: 2 strokes */ {{3, 59, -1}, {0, 2, -1}, {-1, 0, 0}},
    /* 12 ATOLL, par 4: 2 strokes */ {{6, 60, -1}, {13, 52, -1}, {-1, 0, 0}},
    /* 13 BOULDER, par 3: 2 strokes */ {{11, 60, 46}, {0, 30, -1}, {-1, 0, 0}},
    /* 14 BUMPER ALLEY, par 4: 2 strokes */ {{5, 39, 23}, {0, 10, -1}, {-1, 0, 0}},
    /* 15 THE HIGH SHELF, par 4: 2 strokes */ {{2, 58, 15}, {12, 60, 80}, {-1, 0, 0}},
    /* 16 NERVE, par 3: 2 strokes */ {{10, 60, -1}, {9, 48, -1}, {-1, 0, 0}},
    /* 17 THE PAGODA, par 3: 2 strokes */ {{3, 60, 26}, {0, 6, -1}, {-1, 0, 0}},
    /* 18 LAST ORBIT, par 6: 4 strokes */ {{2, 56, 14}, {14, 56, 72}, {4, 60, 126}, {10, 34, -1}, {-1, 0, 0}},
};
