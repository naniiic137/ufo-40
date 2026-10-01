/* BUZZBOLT - the five waves: every group, where it comes from and when.
 * Our own layouts; only each wave's make-up follows the original (which
 * kinds of foe turn up in which wave, and what each wave is for). */
#include "buzzbolt.h"

/* a group: when, what, how it flies, where, how many, which way, how many
 * times each fires, a formation's extra; F adds the shots in each volley
 * (a small aimed fan; S is one shot) */
#define S(t, k, f, x, y, n, d, sh, a) {t, EK_##k, FM_##f, x, y, n, d, sh, a, 1}
#define F(t, k, f, x, y, n, d, sh, a, fan) {t, EK_##k, FM_##f, x, y, n, d, sh, a, fan}

/* Wave 1, OUTER MEADOW: gnats only. The first squads fire once and break
 * away. The reinforcements fire again and again, in small aimed fans,
 * from both sides at once so their volleys meet; the low side runs cross
 * where the letters fall. Then the Ironback pair. */
static const Spawn W1[] = {
    S(40, GNAT, DIVE, 96, 64, 4, 1, 1, 0),
    S(130, GNAT, DIVE, 224, 64, 4, -1, 1, 0),
    S(250, GNAT, DIVE, 160, 76, 4, 1, 1, 0),
    S(340, GNAT, SIDE, 0, 36, 4, 1, 1, 0),
    S(400, GNAT, SIDE, 0, 54, 4, -1, 1, 0),
    S(470, GNAT, DIVE, 56, 56, 3, 1, 1, 0),
    S(470, GNAT, DIVE, 264, 56, 3, -1, 1, 0),
    S(600, GNAT, COLUMN, 120, 0, 4, 0, 1, 0),
    S(640, GNAT, COLUMN, 200, 0, 4, 0, 1, 0),
    S(800, GNAT, SIDE, 0, 40, 4, 1, 1, 0),
    S(820, GNAT, SIDE, 0, 64, 4, -1, 1, 0),
    F(1000, GNAT, ARC, 160, 42, 5, 0, 2, 0, 2),
    F(1180, GNAT, SIDE, 0, 96, 4, 1, 2, 0, 2),
    F(1180, GNAT, SIDE, 0, 112, 4, -1, 2, 0, 2),
    F(1300, GNAT, DIVE, 80, 72, 3, 1, 2, 0, 2),
    F(1300, GNAT, DIVE, 240, 72, 3, -1, 2, 0, 2),
    F(1470, GNAT, COLUMN, 44, 0, 3, 0, 3, 0, 2),
    F(1470, GNAT, COLUMN, 276, 0, 3, 0, 3, 0, 2),
    F(1640, GNAT, SIDE, 0, 34, 4, 1, 2, 0, 3),
    F(1660, GNAT, SIDE, 0, 56, 4, -1, 2, 0, 3),
    F(1880, GNAT, ARC, 160, 48, 6, 0, 3, 0, 2),
    F(2100, GNAT, DIVE, 40, 60, 3, 1, 3, 0, 2),
    F(2100, GNAT, DIVE, 280, 60, 3, -1, 3, 0, 2),
    F(2160, GNAT, SIDE, 0, 100, 4, 1, 2, 0, 2),
    F(2300, GNAT, COLUMN, 100, 0, 3, 0, 3, 0, 3),
    F(2300, GNAT, COLUMN, 220, 0, 3, 0, 3, 0, 3),
    F(2480, GNAT, SIDE, 0, 44, 4, 1, 3, 0, 2),
    F(2480, GNAT, SIDE, 0, 70, 4, -1, 3, 0, 2),
    F(2540, GNAT, DIVE, 160, 90, 3, -1, 2, 0, 3),
    F(2740, GNAT, ARC, 120, 40, 3, 0, 3, 0, 3),
    F(2740, GNAT, ARC, 200, 58, 3, 0, 3, 0, 3),
    F(2900, GNAT, SIDE, 0, 104, 4, -1, 2, 0, 2),
    F(3020, GNAT, DIVE, 128, 84, 3, -1, 3, 0, 3),
    F(3020, GNAT, DIVE, 192, 84, 3, 1, 3, 0, 3),
    F(3200, GNAT, COLUMN, 60, 0, 3, 0, 3, 0, 2),
    F(3200, GNAT, COLUMN, 160, 0, 3, 0, 3, 0, 2),
    F(3200, GNAT, COLUMN, 260, 0, 3, 0, 3, 0, 2),
    F(3420, GNAT, SIDE, 0, 50, 4, 1, 3, 0, 3),
    F(3440, GNAT, SIDE, 0, 80, 4, -1, 3, 0, 3),
};

/* Wave 2, THE THICKET: popcorn midges, whirlers, crickets with the first
 * homing shots, mixed so their volleys come from many sides at once (no
 * gnats until the boss). Then the Bloatfly, who calls gnats in. */
static const Spawn W2[] = {
    S(40, MIDGE, ZIG, 80, 0, 6, 0, 1, 0),
    S(80, MIDGE, ZIG, 240, 0, 6, 0, 1, 0),
    S(260, WHIRLER, HOVER, 100, 52, 1, -1, 4, 260),
    S(280, WHIRLER, HOVER, 220, 52, 1, 1, 4, 260),
    F(420, MIDGE, SIDE, 0, 48, 6, 1, 2, 0, 2),
    F(440, MIDGE, SIDE, 0, 72, 6, -1, 2, 0, 2),
    F(560, MIDGE, ZIG, 160, 0, 6, 0, 2, 0, 2),
    S(700, CRICKET, HOVER, 160, 40, 1, 1, 4, 420),
    F(760, MIDGE, ZIG, 44, 0, 6, 0, 2, 0, 2),
    F(800, MIDGE, ZIG, 276, 0, 6, 0, 2, 0, 2),
    S(1000, WHIRLER, SIDE, 0, 40, 5, 1, 3, 0),
    S(1040, WHIRLER, SIDE, 0, 64, 5, -1, 3, 0),
    S(1240, WHIRLER, ARC, 160, 40, 5, 0, 3, 0),
    F(1400, MIDGE, COLUMN, 100, 0, 6, 0, 2, 0, 2),
    F(1440, MIDGE, COLUMN, 220, 0, 6, 0, 2, 0, 2),
    S(1600, CRICKET, HOVER, 80, 50, 1, -1, 4, 380),
    S(1640, CRICKET, HOVER, 240, 50, 1, 1, 4, 380),
    F(1800, MIDGE, COLUMN, 160, 0, 8, 0, 2, 0, 2),
    F(2000, MIDGE, SIDE, 0, 100, 6, 1, 2, 0, 3),
    F(2040, MIDGE, SIDE, 0, 112, 6, -1, 2, 0, 3),
    S(2220, WHIRLER, SIDE, 0, 36, 5, 1, 3, 0),
    S(2240, WHIRLER, SIDE, 0, 58, 5, -1, 3, 0),
    F(2400, MIDGE, ZIG, 100, 0, 7, 0, 2, 0, 2),
    F(2400, MIDGE, ZIG, 220, 0, 7, 0, 2, 0, 2),
    S(2620, CRICKET, HOVER, 160, 36, 1, 1, 5, 420),
    F(2700, MIDGE, COLUMN, 52, 0, 5, 0, 3, 0, 2),
    F(2700, MIDGE, COLUMN, 268, 0, 5, 0, 3, 0, 2),
    S(2960, WHIRLER, HOVER, 70, 60, 1, -1, 5, 240),
    S(2960, WHIRLER, HOVER, 250, 60, 1, 1, 5, 240),
    F(3100, MIDGE, ZIG, 160, 0, 8, 0, 2, 0, 2),
    S(3300, WHIRLER, ARC, 160, 44, 7, 0, 3, 0),
    S(3520, CRICKET, HOVER, 110, 44, 1, -1, 4, 400),
    S(3520, CRICKET, HOVER, 210, 44, 1, 1, 4, 400),
    S(3700, WHIRLER, SIDE, 0, 60, 5, -1, 3, 0),
    S(3720, WHIRLER, SIDE, 0, 84, 5, 1, 3, 0),
    F(3860, MIDGE, ZIG, 60, 0, 6, 0, 2, 0, 2),
    F(3860, MIDGE, ZIG, 260, 0, 6, 0, 2, 0, 2),
    F(4060, MIDGE, SIDE, 0, 40, 6, 1, 3, 0, 2),
    F(4100, MIDGE, SIDE, 0, 66, 6, -1, 3, 0, 2),
};

/* Wave 3, THE ROT: the hard one. Blisters' dense patterns, crickets'
 * homing shots, popcorn and spore-rocks all at once, two Ironbacks as
 * midbosses. Then the Queen Tick. */
static const Spawn W3[] = {
    S(40, BIGPUFF, DRIFT, 50, 0, 3, 1, 0, 110),
    F(140, MIDGE, ZIG, 160, 0, 8, 0, 2, 0, 2),
    S(320, BLISTER, HOVER, 90, 46, 1, -1, 5, 300),
    S(350, BLISTER, HOVER, 230, 46, 1, 1, 5, 300),
    S(520, PUFF, DRIFT, 30, 0, 6, 1, 0, 52),
    S(620, CRICKET, HOVER, 160, 40, 1, 1, 4, 360),
    F(700, MIDGE, ZIG, 60, 0, 7, 0, 2, 0, 2),
    F(730, MIDGE, ZIG, 260, 0, 7, 0, 2, 0, 2),
    S(920, BLISTER, ARC, 160, 44, 5, 0, 4, 0),
    S(1100, BIGPUFF, DRIFT, 90, 0, 2, -1, 0, 140),
    F(1180, MIDGE, COLUMN, 160, 0, 8, 0, 2, 0, 3),
    S(1240, BLISTER, SIDE, 0, 40, 4, -1, 4, 0),
    S(1300, CRICKET, HOVER, 260, 44, 1, 1, 4, 360),
    S(1360, IRONBACK, MIDBOSS, 160, 44, 1, 1, 0, 780),
    F(1480, MIDGE, COLUMN, 40, 0, 6, 0, 2, 0, 2),
    F(1480, MIDGE, COLUMN, 280, 0, 6, 0, 2, 0, 2),
    S(1760, PUFF, DRIFT, 20, 0, 7, -1, 0, 46),
    S(1900, CRICKET, HOVER, 90, 50, 1, -1, 3, 300),
    S(1960, BLISTER, HOVER, 60, 56, 1, -1, 5, 280),
    S(1960, BLISTER, HOVER, 260, 56, 1, 1, 5, 280),
    S(2020, BLISTER, HOVER, 160, 34, 1, 1, 5, 280),
    S(2100, BLISTER, SIDE, 0, 64, 4, 1, 4, 0),
    S(2260, CRICKET, HOVER, 100, 48, 1, -1, 4, 360),
    S(2300, CRICKET, HOVER, 220, 48, 1, 1, 4, 360),
    F(2420, MIDGE, ZIG, 160, 0, 10, 0, 2, 0, 2),
    S(2600, BIGPUFF, DRIFT, 40, 0, 3, 1, 0, 120),
    S(2760, IRONBACK, MIDBOSS, 100, 40, 1, -1, 0, 780),
    S(2820, BLISTER, SIDE, 0, 72, 4, 1, 4, 0),
    F(3100, MIDGE, ZIG, 80, 0, 8, 0, 2, 0, 3),
    F(3100, MIDGE, ZIG, 240, 0, 8, 0, 2, 0, 3),
    S(3300, BLISTER, ARC, 160, 40, 7, 0, 4, 0),
    S(3420, BLISTER, SIDE, 0, 50, 4, -1, 4, 0),
    S(3500, PUFF, DRIFT, 10, 0, 8, 1, 0, 40),
    S(3600, CRICKET, HOVER, 160, 50, 1, 1, 5, 420),
    S(3800, BLISTER, HOVER, 80, 50, 1, -1, 5, 260),
    S(3800, BLISTER, HOVER, 240, 50, 1, 1, 5, 260),
    F(3960, MIDGE, ZIG, 120, 0, 7, 0, 2, 0, 2),
    F(3960, MIDGE, ZIG, 200, 0, 7, 0, 2, 0, 2),
    S(4160, BIGPUFF, DRIFT, 70, 0, 2, 1, 0, 180),
    S(4260, BLISTER, SIDE, 0, 60, 5, -1, 4, 0),
};

/* Wave 4, THE WALLS: rows of rot wall, some of them gates that switch on
 * and off (safe letters, if you keep your head), then the golden swarm:
 * from the left first, then from the right. Then Scythewing and Dustwing. */
static const Spawn W4[] = {
    S(40, GNAT, DIVE, 160, 60, 5, 1, 2, 0),
    S(260, ROTWALL, WALL, 0, 0, 0, 0, 0, 0x3FFF),  /* cols 14-15 open */
    S(430, ROTWALL, WALL, 0, 0, 0, 0, 0, 0xFFCF),  /* 4-5 */
    S(600, ROTWALL, WALL, 0, 0, 0, 5, 6, 0xFFFF),  /* a gate row */
    S(770, ROTWALL, WALL, 0, 0, 0, 0, 0, 0xFE7F),  /* 7-8 */
    S(940, ROTWALL, WALL, 0, 0, 0, 3, 5, 0xFFFF),  /* gates */
    S(1110, ROTWALL, WALL, 0, 0, 0, 0, 0, 0x3FFC), /* both edges */
    S(1280, ROTWALL, WALL, 0, 0, 0, 6, 4, 0xFFFF), /* gates */
    S(1450, ROTWALL, WALL, 0, 0, 0, 0, 0, 0xFC3F), /* the middle */
    S(1680, GNAT, SIDE, 0, 40, 5, 1, 3, 0),
    S(1740, GNAT, SIDE, 0, 60, 5, -1, 3, 0),
    S(1960, GOLDBUG, STREAM, 0, 42, 12, 1, 0, 0),
    S(2100, GOLDBUG, STREAM, 0, 66, 12, 1, 0, 0),
    S(2380, GNAT, ARC, 160, 40, 6, 0, 3, 0),
    S(2440, GNAT, DIVE, 60, 70, 4, 1, 3, 0),
    S(2760, GOLDBUG, STREAM, 0, 46, 12, -1, 1, 0),
    S(2900, GOLDBUG, STREAM, 0, 72, 12, -1, 1, 0),
    S(3000, GNAT, DIVE, 260, 70, 4, -1, 3, 0),
    S(3240, GNAT, DIVE, 90, 70, 4, 1, 3, 0),
    S(3280, GNAT, DIVE, 230, 70, 4, -1, 3, 0),
    S(3520, GNAT, COLUMN, 60, 0, 4, 0, 3, 0),
    S(3520, GNAT, COLUMN, 260, 0, 4, 0, 3, 0),
    S(3700, GNAT, SIDE, 0, 50, 6, 1, 3, 0),
};

/* Wave 5, THE HEART: the last fight, and a gnat that will not leave you be. */
static const Spawn W5[] = {
    S(0, GNAT, DIVE, 160, 60, 0, 1, 0, 0),
};

const WaveDef BZZ_WAVE[BZZ_WAVES] = {
    {"OUTER MEADOW", W1, ARRAY_LEN(W1), 3780, 100},
    {"THE THICKET", W2, ARRAY_LEN(W2), 4420, 120},
    {"THE ROT", W3, ARRAY_LEN(W3), 4600, 130},
    {"THE WALLS", W4, ARRAY_LEN(W4), 4080, 120},
    {"THE HEART", W5, ARRAY_LEN(W5), 90, 80},
};
