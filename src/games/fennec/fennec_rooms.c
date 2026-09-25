/* FENNEC FOUNTAIN - the fifty rooms. Every wall and block placement here is
 * an original design for UFO 40, laid out on our own wall plans and checked
 * with a breadth-first solver on the game's own rule code. Legend: # wall,
 * . floor, K the fennec, D the way out, G the dry spring, S the water stone,
 * 1-4 sandstone, 5 marble, a-d lapis 1-4, w-z basalt 1-4 (n x n), g gecko.
 * Each solution is the shortest one; tests replay them all. */
#include "fennec.h"

const RoomDef FN_ROOMS_DEF[FN_ROOMS] = {
    /*  1: 6 moves */
    {"FIRST SPRING",
     {"##########", "#........#", "DK...S..G#", "#........#", "##########"},
     "RRRRRR"},
    /*  2: 8 moves */
    {"AROUND THE PALM",
     {"#########", "D.......#", "#K..#...#", "#...#.S.#", "#.....G.#", "#########"},
     "URRRRRDD"},
    /*  3: 11 moves */
    {"THE LOW WALL",
     {"##########", "D...#....#", "#K..#..G.#", "#...#.S..#", "#........#", "##########"},
     "RRDDRRURDRU"},
    /*  4: 15 moves */
    {"SAND STEPS",
     {"###########", "#....#....#", "D.K..#.S..#", "#....#1G..#", "##.###....#", "#.........#", "###########"},
     "DDDRRRRUURRUULD"},
    /*  5: 17 moves */
    {"ONE AND ONE",
     {"############", "#....#1....#", "D.K..#..#S.#", "#....#..#G.#", "#.#.##..####", "#..1..1....#", "############"},
     "DLDDRRRRRUUURURRD"},
    /*  6: 17 moves */
    {"GARDEN PATH",
     {"###########", "#...#.....#", "D.K1#.###1#", "#.....#G..#", "###.#.#..3#", "#.....#.S.#", "#...#.....#", "###########"},
     "DRRRDDDRRRUUDRUUL"},
    /*  7: 25 moves */
    {"THE DRY CANAL",
     {"###########", "#.....#.1.#", "D.K...#.G.#", "#.#.###S..#", "#......1..#", "#...#...3.#", "###########"},
     "RDDRRRRRUULDRDLDLLURRDRUU"},
    /*  8: 33 moves */
    {"SHADY CORNER",
     {"##########", "#...#....#", "D.K.#.1..#", "#1....1#.#", "#.S.#..#G#", "#...######", "##########"},
     "RDRRDRUUDLLLDDLULURRRRDRULURRURDD"},
    /*  9: 14 moves */
    {"DATE GROVE",
     {"##########", "#....#...#", "D.K.1#.#.#", "#...1..#.#", "#.##.#.#S#", "#....#..G#", "##########"},
     "URRDDRRUURRDDD"},
    /* 10: 16 moves */
    {"FIRST MARBLE",
     {"############", "#.....#....#", "D.K...#....#", "#.....#.GS.#", "#...###..4.#", "#......41..#", "############"},
     "RDDDRRRRUUURRRDL"},
    /* 11: 16 moves */
    {"NARROW GATE",
     {"############", "#...1#.....#", "D.K1.#.###.#", "#....#.#GS.#", "#......#...#", "#..#...#...#", "#..#.......#", "############"},
     "RDRDRRUUURRRRDDL"},
    /* 12: 25 moves */
    {"THE LONG ROW",
     {"###########", "#....#....#", "D.K..#....#", "#.........#", "#1S#####..#", "#.......1.#", "#....#.4G.#", "###########"},
     "DDULDDRRRRRRLDRRUURUULDDD"},
    /* 13: 36 moves */
    {"SUNDIAL",
     {"###########", "#....1....#", "D.K.#.#.#.#", "#.........#", "#.#S#.#.#.#", "#2......G.#", "###########"},
     "RDRRDDLLUULUURDLDRRLUURRDDDULLDDRRRR"},
    /* 14: 28 moves */
    {"CISTERN",
     {"##############", "#.....#......#", "D.K...#......#", "#.....#..##..#", "#.S...4....G.#", "#.....#.1##4.#", "#.....#.....1#", "##############"},
     "RRRDDRRURDLLLULLLLDRRRRRRRRR"},
    /* 15: 33 moves */
    {"THE CROOKED WELL",
     {"###########", "#....#....#", "D.K..#....#", "#...S2....#", "###.###.###", "#.....11..#", "#...G.....#", "###########"},
     "RDDDRRRRUULLRRDDLLLLUURUULDDDDLDR"},
    /* 16: 39 moves */
    {"PALM HALL",
     {"############", "D1K.#......#", "#...#.####.#", "#..3......G#", "#.1S#.####.#", "#...#......#", "############"},
     "DDRRRUURRRRRDDLLLLLLLULDLDDRRULURRRRRRR"},
    /* 17: 18 moves */
    {"BLUE TILE",
     {"##########", "#........#", "D.K.a....#", "#.S.##...#", "#...#G...#", "##########"},
     "RDDLULURRRRURDDRDL"},
    /* 18: 23 moves */
    {"THE TILED PORCH",
     {"############", "#...#...1..#", "D.K.#.S....#", "#...b......#", "#.3.#.###..#", "#...#...#G.#", "############"},
     "RDDDLULURRRRURRRLURRDDD"},
    /* 19: 24 moves */
    {"LAPIS LANE",
     {"###########", "#...#.....#", "D.K.#.S.3.#", "#.....##..#", "#...#.#Ga.#", "#...#.....#", "###########"},
     "RDRRUURRDRULLLDRRURDDRDL"},
    /* 20: 27 moves */
    {"MOSAIC",
     {"#############", "#...#.......#", "D.K.#.#####.#", "#...a.#...#.#", "#23.#.#.G.S.#", "#...b.......#", "#############"},
     "DLDRURDLDRRRUUUURRRRRRDDDLL"},
    /* 21: 25 moves */
    {"THE FOUNTAIN STEPS",
     {"###########", "#....#....#", "D.KS.#....#", "#....3b...#", "#....#..G.#", "###########"},
     "URDRDRRRLLLULLDRRRRURDLDR"},
    /* 22: 30 moves */
    {"COURTYARD",
     {"############", "#....#.....#", "D.K..#..2..#", "#....a...#G#", "#.S..#...#.#", "#....#.....#", "############"},
     "RDDDLULURRRRRUURRDDDLULURRRURD"},
    /* 23: 36 moves */
    {"ARCHES",
     {"#############", "#.....#.....#", "D.K...#S.a..#", "#...........#", "#.....#.cG..#", "#.....#.....#", "#############"},
     "RRRDRRRUULDRDLLLULLDRRRRRRDRURULULDD"},
    /* 24: 38 moves */
    {"BLUE DOOR",
     {"###########", "#.........#", "D.K.#S#...#", "#a..#.#.3.#", "#....b....#", "###.#.#.G.#", "#.........#", "###########"},
     "URRRRRRDDRDLLLLLLUUURRDDUULLDDDRRRRURD"},
    /* 25: 35 moves */
    {"THE WATER STAIR",
     {"##########", "#........#", "D.K.#a##.#", "#...#..#.#", "#.b23.1.G#", "#...#S####", "#........#", "##########"},
     "RDDUUURRRRRDDDLLLLLDLDRRRUDLLUURRRR"},
    /* 26: 46 moves */
    {"TILE MAZE",
     {"############", "#....#.....#", "D.K..b.....#", "#.S..#.##..#", "#.3a.#..#G.#", "#....#.....#", "############"},
     "LDDRUDRDRUULURRRRRURDDDUULLLLLULLLDRRRRRRRURDD"},
    /* 27: 50 moves */
    {"THE LONG HALL",
     {"##############", "#......#.....#", "D.K.2..#.....#", "#......cS..4.#", "#......#.###.#", "#......b...#G#", "#......#.....#", "##############"},
     "URRDDDLDRRRRRUURURRRDLLLLLRRUULDRDLDDLLUURRRRRURDD"},
    /* 28: 47 moves */
    {"THE BLUE PALACE",
     {"############", "#....#.....#", "D.KS.#.....#", "#...3c...1.#", "#....#.##..#", "#....#...G.#", "############"},
     "RDRRRURRRDULLLDRRURDDRDLUULLLLLLUURDLDRRRRRURDD"},
    /* 29: 22 moves */
    {"OLD BASALT",
     {"############", "#..xx#.1...#", "D.Kxx#.S.xx#", "#......##xx#", "#....#..#G.#", "#....#.....#", "############"},
     "DRRRRUURRRDULLLDRRURDD"},
    /* 30: 22 moves */
    {"CRUMBLING WALL",
     {"###########", "#..xx.....#", "D.Kxx.....#", "#...#S....#", "#...#...G.#", "###########"},
     "RRDRRDDLURUULLDRRRURDD"},
    /* 31: 27 moves */
    {"THE QUARRY",
     {"#############", "#.....#.....#", "D.K...#yyy..#", "#..S...yyy..#", "#.....#yyy..#", "#.....#..G..#", "#############"},
     "RRRDRRURDLLLULLLDRRRRRRURDD"},
    /* 32: 28 moves */
    {"BIG STONE",
     {"############", "#......xx..#", "D.K....xx..#", "#...####...#", "#.S.#..#.G.#", "#........w.#", "############"},
     "RDDDLUULURRRRURRRLLLDRRRURDD"},
    /* 33: 28 moves */
    {"THE BOULDER",
     {"############", "#....#.....#", "D.K..#.xx..#", "#......xx..#", "#2S..#...G.#", "#....#.....#", "############"},
     "RRDRRRLLLDDLLUDLUURRRRRRRURD"},
    /* 34: 35 moves */
    {"THE TERRACE",
     {"##############", "#......#...xx#", "D.Kyyy.#S..xx#", "#..yyy.......#", "#..yyy.#.....#", "#......#...G.#", "#......#.....#", "##############"},
     "URRRDRDRRRUULDRDLLLULLDRRRRURDDLDRR"},
    /* 35: 42 moves */
    {"ROCKFALL",
     {"#############", "#.....#.....#", "D.K...#S....#", "#w....#..#xx#", "#.........xx#", "#.....#..#G.#", "#############"},
     "RRRDDRRURURRDUULLLDDRDLLLULLDRRRRRRLUURRDD"},
    /* 36: 34 moves */
    {"THE OLD DAM",
     {"#############", "#...........#", "D.Kxx#S#....#", "#..xx#.#....#", "#........2..#", "#....#.#..G.#", "#############"},
     "URRRRDDUURRRDDDLLLLLULLDRRRRRRRURD"},
    /* 37: 37 moves */
    {"BASALT AND BLUE",
     {"############", "#..a.......#", "D.K.#S.#...#", "#.xx#..#...#", "#.xx....2..#", "#...#..#.G.#", "############"},
     "DRDRRURUULDDRDRRRLLLLLLULLDRRRRRRRURD"},
    /* 38: 40 moves */
    {"MASON'S YARD",
     {"###############", "#.......#.....#", "D.Kyyy..#w....#", "#..yyy..#S.#..#", "#..yyyxx......#", "#.....xx#..#G.#", "#.......#.....#", "###############"},
     "URRRRRDDDRRRUUULDRDDLLLULLDRRRRRRLUURRDD"},
    /* 39: 14 moves */
    {"THE COPYCAT",
     {"##########", "#........#", "D.K......#", "#.1......#", "#.Sg...G.#", "##########"},
     "RDUULLDDDRRRRR"},
    /* 40: 22 moves */
    {"GECKO GARDEN",
     {"############", "#....#.....#", "D.K..#.....#", "#..........#", "#.g..#S..G.#", "#....#.....#", "############"},
     "RRDRRUURRRDDLULULDRRRD"},
    /* 41: 24 moves */
    {"SIDE BY SIDE",
     {"#############", "#...........#", "D.K.#.#.#.#.#", "#...#.S2..#.#", "#.g.#.#G#.#.#", "#...........#", "#############"},
     "URRRRRDDDUUULLDDRLUURRDD"},
    /* 42: 22 moves */
    {"MIRROR COURT",
     {"###########", "#.........#", "D.K...#...#", "#.S...#...#", "#.g..2..G.#", "###########"},
     "RURRRRDULLLDDLDRRRRRUR"},
    /* 43: 24 moves */
    {"GECKO PAIR",
     {"############", "#.....#...2#", "D.K...#S...#", "#..........#", "#.g...#1.G.#", "#.....#.g..#", "############"},
     "RRRDRRRUULDURRDRLLLRDLDR"},
    /* 44: 26 moves */
    {"TWO STEPS",
     {"############", "#..........#", "D.KS##.##..#", "#........3.#", "#.g.##.##G.#", "#..........#", "############"},
     "URRRRRRRRLLLLLLLDURRRRRRDD"},
    /* 45: 21 moves */
    {"THE LONG SHADOW",
     {"##############", "#......#.....#", "D.K....#.....#", "#.....2......#", "#.g..S.#..G..#", "#.....1#..g..#", "##############"},
     "RRDRURULDLLDRRRRRRURD"},
    /* 46: 35 moves */
    {"SUN AND SHADE",
     {"############", "#....#.3.b.#", "D.K..#..S..#", "#....#.##..#", "#.g......G.#", "#....#.....#", "############"},
     "RRDDRRUUURDLDDRDURRRUUUDLLLULDDURRR"},
    /* 47: 30 moves */
    {"FOLLOW THE LEADER",
     {"#############", "#...#.......#", "D.K.#..2....#", "#......a##..#", "#.g.#..S#G..#", "#...#.......#", "#############"},
     "RDRRUURRRDLLDLLRRUUDRRRURDDRDL"},
    /* 48: 31 moves */
    {"THE MAZE OF COPIES",
     {"#############", "#...........#", "D.KS#####xx.#", "#...1....xx.#", "#.g.#####.G.#", "#...........#", "#############"},
     "URRRRRRRDULLLLLLDURRRRRRDDURRLD"},
    /* 49: 59 moves */
    {"THE LAST GARDEN",
     {"############", "#.....#....#", "D.K...#...a#", "#.....#S##.#", "#.g.3......#", "#.....#..#G#", "############"},
     "RDRRDRRDURRRUDLLLLLDLLLURRRUUDDRRRRLLLLUUULDDLLDURRRDRRRRLD"},
    /* 50: 28 moves */
    {"THE VIZIER'S BATH",
     {"################", "#...yyy.#......#", "D.K.yyy.#......#", "#...yyy.#..##xx#", "#............xx#", "#.g..S..#..#G.a#", "#.......#12....#", "################"},
     "RRDRDULDRRRRRUURRRRDRLDURDDL"},
};
