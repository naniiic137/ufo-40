/* FENNEC FOUNTAIN - the fifty rooms. Every wall and block placement here is
 * an original design for UFO 40, laid out on our own wall plans and checked
 * with a breadth-first solver on the game's own rule code. Legend: # wall,
 * . floor, K the fennec, G the dry spring, S the water stone, 1-4 sandstone,
 * 5 marble, a-d lapis 1-4, w-z basalt 1-4 (n x n), g gecko, ^ > v < arrows,
 * : a stone patch, o a plate, | a door. Each solution is the shortest one;
 * the tests replay them all. */
#include "fennec.h"

const RoomDef FN_ROOMS_DEF[FN_ROOMS] = {
    /*  1: 6 moves */
    {"FIRST SPRING",
     {"##########", "#........#", "#K...S..G#", "#........#", "##########"},
     "RRRRRR"},
    /*  2: 8 moves */
    {"AROUND THE PALM",
     {"#########", "#.......#", "#K..#...#", "#...#.S.#", "#.....G.#", "#########"},
     "URRRRRDD"},
    /*  3: 11 moves */
    {"THE LOW WALL",
     {"##########", "#...#....#", "#K..#..G.#", "#...#.S..#", "#........#", "##########"},
     "RRDDRRURDRU"},
    /*  4: 15 moves */
    {"SAND STEPS",
     {"###########", "#....#....#", "#.K..#.S..#", "#....#1G..#", "##.###....#", "#.........#", "###########"},
     "DDDRRRRUURRUULD"},
    /*  5: 17 moves */
    {"ONE AND ONE",
     {"############", "#....#1....#", "#.K..#..#S.#", "#....#..#G.#", "#.#.##..####", "#..1..1....#", "############"},
     "DLDDRRRRRUUURURRD"},
    /*  6: 26 moves */
    {"OLD BASALT",
     {"############", "#..xx#.1...#", "#.Kxx#.S.xx#", "#......##xx#", "#....#..#G.#", "#....#.....#", "############"},
     "DRRRRUURRRDUDDUULLLDRRURDD"},
    /*  7: 17 moves */
    {"GARDEN PATH",
     {"###########", "#...#.....#", "#.K1#.###1#", "#.....#G..#", "###.#.#..3#", "#.....#.S.#", "#...#.....#", "###########"},
     "DRRRDDDRRRUUDRUUL"},
    /*  8: 22 moves */
    {"CRUMBLING WALL",
     {"###########", "#..xx.....#", "#.Kxx.....#", "#...#S....#", "#...#...G.#", "###########"},
     "RRDRRDDLURUULLDRRRURDD"},
    /*  9: 14 moves */
    {"DATE GROVE",
     {"##########", "#....#...#", "#.K.1#.#.#", "#...1..#.#", "#.##.#.#S#", "#....#..G#", "##########"},
     "URRDDRRUURRDDD"},
    /* 10: 16 moves */
    {"FIRST MARBLE",
     {"############", "#.....#....#", "#.K...#....#", "#.....#.GS.#", "#...###..4.#", "#......41..#", "############"},
     "RDDDRRRRUUURRRDL"},
    /* 11: 25 moves */
    {"THE DRY CANAL",
     {"###########", "#.....#.1.#", "#.K...#.G.#", "#.#.###S..#", "#......1..#", "#...#...3.#", "###########"},
     "RDDRRRRRUULDRDLDLLURRDRUU"},
    /* 12: 16 moves */
    {"NARROW GATE",
     {"############", "#...1#.....#", "#.K1.#.###.#", "#....#.#GS.#", "#......#...#", "#..#...#...#", "#..#.......#", "############"},
     "RDRDRRUUURRRRDDL"},
    /* 13: 28 moves */
    {"BIG STONE",
     {"############", "#......xx..#", "#.K....xx..#", "#...####...#", "#.S.#..#.G.#", "#........w.#", "############"},
     "RDDDLUULURRRRURRRLLLDRRRURDD"},
    /* 14: 33 moves */
    {"SHADY CORNER",
     {"##########", "#...#....#", "#.K.#.1..#", "#1....1#.#", "#.S.#..#G#", "#...######", "##########"},
     "RDRRDRUUDLLLDDLULURRRRDRULURRURDD"},
    /* 15: 27 moves */
    {"THE LONG ROW",
     {"###########", "#....#....#", "#.K..#....#", "#.........#", "#1S#####..#", "#.......1.#", "#....#.4G.#", "###########"},
     "DLDDRULURRRRRRURDRDDLRUULDD"},
    /* 16: 27 moves */
    {"THE QUARRY",
     {"#############", "#.....#.....#", "#.K...#yyy..#", "#..S...yyy..#", "#.....#yyy..#", "#.....#..G..#", "#############"},
     "RRRDRRURDLLLULLLDRRRRRRURDD"},
    /* 17: 27 moves */
    {"BLUE TILE",
     {"##########", "#.K......#", "#....2..a#", "#.S.##..G#", "#...#....#", "##########"},
     "RRRRDLLLLRDDLULURRRRRURDLDR"},
    /* 18: 23 moves */
    {"THE TILED PORCH",
     {"############", "#...#...1..#", "#.K.#.S....#", "#...b......#", "#.3.#.###..#", "#...#...#G.#", "############"},
     "RDDDLULURRRRURRRLURRDDD"},
    /* 19: 26 moves */
    {"LAPIS LANE",
     {"###########", "#...#.....#", "#.K.#.S.3.#", "#.....##..#", "#...#.#Ga.#", "#...#.....#", "###########"},
     "RDRRURURRDDDUUULLDRURDDRDL"},
    /* 20: 36 moves */
    {"SUNDIAL",
     {"###########", "#....1....#", "#.K.#.#.#.#", "#.........#", "#.#S#.#.#.#", "#2......G.#", "###########"},
     "RDRRDDLLUULUURDLDRRLUURRDDDULLDDRRRR"},
    /* 21: 27 moves */
    {"MOSAIC",
     {"#############", "#...#.......#", "#.K.#.#####.#", "#...a.#...#.#", "#23.#.#.G.S.#", "#...b.......#", "#############"},
     "DLDRURDLDRRRUUUURRRRRRDDDLL"},
    /* 22: 25 moves */
    {"THE FOUNTAIN STEPS",
     {"###########", "#....#....#", "#.KS.#....#", "#....3b...#", "#....#..G.#", "###########"},
     "URDRDRRRLLLULLDRRRRURDLDR"},
    /* 23: 28 moves */
    {"THE BOULDER",
     {"############", "#....#.....#", "#.K..#.xx..#", "#......xx..#", "#2S..#...G.#", "#....#.....#", "############"},
     "RRDRRRLLLDDLLUDLUURRRRRRRURD"},
    /* 24: 44 moves */
    {"COURTYARD",
     {"############", "#....#...1G#", "#....#.....#", "#..2.a...#.#", "#....#S..#.#", "#..K.#.....#", "############"},
     "ULURRRRRDDLURULLLULLDRRRRDRULURRRLDLDDRRRUUU"},
    /* 25: 36 moves */
    {"ARCHES",
     {"#############", "#.....#.....#", "#.K...#S.a..#", "#...........#", "#.....#.cG..#", "#.....#.....#", "#############"},
     "RRRDRRRUULDRDLLLULLDRRRRRRDRRUULDRDL"},
    /* 26: 28 moves */
    {"CISTERN",
     {"##############", "#.....#......#", "#.K...#......#", "#.....#..##..#", "#.S...4....G.#", "#.....#.1##4.#", "#.....#.....1#", "##############"},
     "RRRDDRRURDLLLULLLLDRRRRRRRRR"},
    /* 27: 38 moves */
    {"BLUE DOOR",
     {"###########", "#.........#", "#.K.#S#...#", "#a..#.#.3.#", "#....b....#", "###.#.#.G.#", "#.........#", "###########"},
     "URRRRRRDDRDLLLLLLUUURRDDUULLDDDRRRRURD"},
    /* 28: 33 moves */
    {"THE CROOKED WELL",
     {"###########", "#....#....#", "#.K..#....#", "#...S2....#", "###.###.###", "#.....11..#", "#...G.....#", "###########"},
     "RDDDRRRRUULLRRDDLLLLUURUULDDDDLDR"},
    /* 29: 14 moves */
    {"THE COPYCAT",
     {"##########", "#........#", "#.K......#", "#.1......#", "#.Sg...G.#", "##########"},
     "RDUULLDDDRRRRR"},
    /* 30: 22 moves */
    {"GECKO GARDEN",
     {"############", "#....#.....#", "#.K..#.....#", "#..........#", "#.g..#S..G.#", "#....#.....#", "############"},
     "RRDRRUURRRDDLULULDRRRD"},
    /* 31: 35 moves */
    {"THE TERRACE",
     {"##############", "#......#...xx#", "#.Kyyy.#S..xx#", "#..yyy.......#", "#..yyy.#.....#", "#......#...G.#", "#......#.....#", "##############"},
     "URRRDRDRRRUULDRDLLLULLDRRRRURDDLDRR"},
    /* 32: 24 moves */
    {"SIDE BY SIDE",
     {"#############", "#...........#", "#.K.#.#.#.#.#", "#...#.S2..#.#", "#.g.#.#G#.#.#", "#...........#", "#############"},
     "URRRRRDDDUUULLDDRLUURRDD"},
    /* 33: 39 moves */
    {"PALM HALL",
     {"############", "#1K.#......#", "#...#.####.#", "#..3......G#", "#.1S#.####.#", "#...#......#", "############"},
     "DDRRRUURRRRRDDLLLLLLLULLDDDRRULURRRRRRR"},
    /* 34: 22 moves */
    {"MIRROR COURT",
     {"###########", "#.........#", "#.K...#...#", "#.S...#...#", "#.g..2..G.#", "###########"},
     "RURRRRDULLLDDLDRRRRRUR"},
    /* 35: 43 moves */
    {"ROCKFALL",
     {"#############", "#.....#.....#", "#.K...#S....#", "#w....#..#xx#", "#.........xx#", "#.....#..#G.#", "#############"},
     "RRRDDRRURUULDDURRRDLDLLLLLULLDRRRRRRLUURRDD"},
    /* 36: 23 moves */
    {"GECKO PAIR",
     {"############", "#.....#...2#", "#.K...#S...#", "#..........#", "#.g...#1.G.#", "#.....#.g..#", "############"},
     "RRRDRRLLDDUUUULDDRRRRLD"},
    /* 37: 46 moves */
    {"TILE MAZE",
     {"############", "#....#.....#", "#.K..b.....#", "#.S..#.##..#", "#.3a.#..#G.#", "#....#.....#", "############"},
     "LDDRUDRDRUULURRRRRURDDDUULLLLLULLLDRRRRRRRURDD"},
    /* 38: 34 moves */
    {"THE OLD DAM",
     {"#############", "#...........#", "#.Kxx#S#....#", "#..xx#.#....#", "#........2..#", "#....#.#..G.#", "#############"},
     "URRRRDDUURRRDDDLLLLLULLDRRRRRRRURD"},
    /* 39: 31 moves */
    {"ONE WAY",
     {"###########", "#....#....#", "#....>....#", "#..2.#....#", "#...S<....#", "#K..1#...G#", "###########"},
     "UUURRRRRDDLLLDLUULURRRRRRRURDDD"},
    /* 40: 50 moves */
    {"THE LONG HALL",
     {"##############", "#......#.....#", "#.K.2..#.....#", "#......cS..4.#", "#......#.###.#", "#......b...#G#", "#......#.....#", "##############"},
     "URRDDDLDRRRRRUURURRRDLLLLLRRUULDRDLDDLLUURRRRRURDD"},
    /* 41: 32 moves */
    {"STEPPING STONES",
     {"###########", "#........1#", "#.S:::....#", "#..:::..K.#", "#..2....G.#", "###########"},
     "LLLLLLDRRRRRRUUULLLLLLDDLDRRRRRR"},
    /* 42: 26 moves */
    {"TWO STEPS",
     {"############", "#..........#", "#.KS##.##..#", "#........3.#", "#.g.##.##G.#", "#..........#", "############"},
     "URRRRRRRRLLLLLLLDURRRRRRDD"},
    /* 43: 35 moves */
    {"THE HEAVY DOOR",
     {"###########", "#....#.1..#", "#.2..|....#", "#....#S..G#", "#.o.K#....#", "###########"},
     "UUULLDDURRRRRDDLURULLLULLDRRRRRRURD"},
    /* 44: 37 moves */
    {"BASALT AND BLUE",
     {"############", "#..a.......#", "#.K.#S.#...#", "#.xx#..#...#", "#.xx....2..#", "#...#..#.G.#", "############"},
     "DRDRRRRRDRUUURULLLLLDDUULLDDDRRRRRURD"},
    /* 45: 25 moves */
    {"CROSSWINDS",
     {"############", "#..K.#.....#", "#.>>>#.2...#", "#....#....1#", "#...GvS....#", "#....v.3<<.#", "#....#.....#", "############"},
     "RDDDRRUURURDDRDLLRDDLURUL"},
    /* 46: 37 moves */
    {"THE BLUE PALACE",
     {"############", "#....#.....#", "#.KS.#.....#", "#...3c...1.#", "#....#.##..#", "#....#...G.#", "############"},
     "RDRRRRRURDDRDLUULLLLLLUURDLDRRRRRURDD"},
    /* 47: 33 moves */
    {"TWO LOCKS",
     {"############", "#...K#.....#", "#.oG.|.....#", "#.S..#.....#", "##.#####.###", "#.3..#.....#", "#..1.|..o..#", "#....#..2..#", "############"},
     "DLLLDRDDDUUUURRDLULDDDLDDRUUUULUR"},
    /* 48: 59 moves */
    {"THE LAST GARDEN",
     {"############", "#.....#....#", "#.K...#...a#", "#.....#S##.#", "#.g.3......#", "#.....#..#G#", "############"},
     "RDRRDRRDURRRUDLLLLLDLLLURRRUUDDRRRRLLLLUUULDDLLDURRRDRRRRLD"},
    /* 49: 40 moves */
    {"ALL THE WAYS",
     {"#############", "#.....#....G#", "#.:::1|.>>..#", "#...KS#....2#", "#.o.3.#..^..#", "#.....#.....#", "#############"},
     "LDDRRULLUURRRRRRRLLLLLLDDRULURRRRRDRULUR"},
    /* 50: 31 moves */
    {"HUMPH'S BATH",
     {"################", "#...yyy.#......#", "#.K.yyy.#......#", "#...yyy.#..##xx#", "#............xx#", "#.g..S..#..#G.a#", "#.......#12....#", "################"},
     "RRDRDULDRRRRRUURRRRDLDUURDLDRDL"},
};
