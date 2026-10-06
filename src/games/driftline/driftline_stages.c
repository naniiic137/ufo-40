/* DRIFTLINE - the four stages' waves, stage 4's swells and the three bonus
 * stages' blocks. All hand-placed for UFO 40; the same every time.
 *
 * A spawn: {t, kind, x, y, n, dir, gap, arg}
 *   kites:     x the first one's spot, y its height, arg the spacing
 *   rotors:    x the first one's spot, y its height, arg the spacing
 *   road hogs: dir the way they drive, arg their speed in tenths
 *   squirts, saucers: y the height, dir the way across
 *   sharks, tumblers: dir the way across
 *   the rest come down from the top at x (each one arg further on) to y */
#include "driftline.h"

#define S(t, k, x, y, n, d, g, a) {t, k, x, y, n, d, g, a}

static const DflSpawn HARBOUR[] = {
    S(120, FK_KITE, 200, 40, 4, -1, 16, 22),
    S(300, FK_BUZZER, 60, 70, 3, 1, 20, 40),
    S(520, FK_HOG, 0, 0, 1, -1, 0, 12),
    S(700, FK_KITE, 110, 34, 4, 1, 16, 22),
    S(900, FK_BUZZER, 180, 60, 4, 1, 16, 30),
    S(1100, FK_HOG, 0, 0, 1, 1, 0, 18),
    S(1250, FK_ROTOR, 240, 50, 1, -1, 0, 0),
    S(1500, FK_KITE, 180, 30, 5, -1, 14, 20),
    S(1700, FK_HOG, 0, 0, 2, -1, 70, 10),
    S(1900, FK_BUZZER, 40, 80, 5, 1, 14, 50),
    S(2150, FK_ROTOR, 80, 44, 1, 1, 0, 0),
    S(2300, FK_KITE, 140, 46, 4, 1, 16, 24),
    S(2500, FK_HOG, 0, 0, 2, 1, 60, 16),
    S(2700, FK_BUZZER, 120, 64, 3, 1, 12, 40),
    S(2900, FK_ROTOR, 220, 40, 2, -1, 60, 40),
    S(3200, FK_KITE, 200, 36, 6, -1, 12, 18),
    S(3400, FK_HOG, 0, 0, 2, -1, 50, 14),
    S(3600, FK_BUZZER, 50, 56, 6, 1, 12, 44),
    S(3900, FK_KITE, 100, 30, 4, 1, 14, 20),
    S(4000, FK_HOG, 0, 0, 1, 1, 0, 20),
    S(4200, FK_ROTOR, 100, 46, 2, 1, 60, 60),
    S(4500, FK_BUZZER, 30, 74, 4, 1, 10, 70),
    S(4700, FK_HOG, 0, 0, 3, -1, 60, 12),
    S(4900, FK_KITE, 160, 40, 6, -1, 12, 18),
    S(5200, FK_ROTOR, 250, 36, 1, -1, 0, 0),
    S(5300, FK_HOG, 0, 0, 2, 1, 50, 18),
    S(5500, FK_BUZZER, 80, 60, 5, 1, 12, 40),
    S(5750, FK_KITE, 180, 30, 5, 1, 14, 22),
    S(6000, FK_HOG, 0, 0, 2, -1, 40, 14),
    S(6200, FK_ROTOR, 60, 40, 2, 1, 50, 50),
    S(6450, FK_BUZZER, 100, 70, 6, 1, 10, 24),
    S(6700, FK_KITE, 120, 34, 6, -1, 12, 20),
    S(6900, FK_HOG, 0, 0, 3, 1, 50, 16),
    S(7200, FK_ROTOR, 200, 44, 2, -1, 40, 50),
    S(7400, FK_KITE, 150, 40, 4, 1, 14, 20),
    S(7600, FK_BUZZER, 40, 66, 6, 1, 12, 48),
    S(7850, FK_KITE, 200, 34, 5, -1, 14, 20),
    S(8100, FK_HOG, 0, 0, 2, -1, 60, 12),
    S(8300, FK_ROTOR, 160, 44, 2, 1, 50, 60),
};

static const DflSpawn SUNDOWN[] = {
    S(120, FK_SHARD, 80, 0, 4, 1, 30, 50),
    S(400, FK_SHARD, 240, 0, 4, 1, 30, -50),
    S(700, FK_HOOP, 60, 0, 1, 1, 0, 0),
    S(1000, FK_PRISM, 40, 40, 1, 1, 0, 2),
    S(1100, FK_SHARD, 200, 0, 3, 1, 24, 30),
    S(1400, FK_TUMBLER, 0, 0, 1, -1, 0, 0),
    S(1700, FK_SHARD, 60, 0, 5, 1, 20, 45),
    S(1900, FK_HOOP, 260, 0, 1, -1, 0, 0),
    S(2200, FK_PRISM, 280, 36, 1, 1, 0, 2),
    S(2300, FK_SHARD, 120, 0, 3, 1, 24, 40),
    S(2600, FK_TUMBLER, 0, 0, 1, 1, 0, 0),
    S(2800, FK_SHARD, 40, 0, 6, 1, 18, 46),
    S(3100, FK_HOOP, 100, 0, 1, 1, 0, 0),
    S(3160, FK_HOOP, 220, 0, 1, -1, 0, 0),
    S(3500, FK_PRISM, 40, 44, 1, 1, 0, 2),
    S(3500, FK_PRISM, 280, 44, 1, 1, 0, 2),
    S(3700, FK_SHARD, 160, 0, 4, 1, 30, 20),
    S(4000, FK_TUMBLER, 0, 0, 1, -1, 0, 0),
    S(4150, FK_TUMBLER, 0, 0, 1, 1, 0, 0),
    S(4400, FK_SHARD, 280, 0, 6, 1, 18, -44),
    S(4700, FK_HOOP, 160, 0, 1, 1, 0, 0),
    S(4900, FK_SHARD, 40, 0, 4, 1, 20, 70),
    S(5200, FK_PRISM, 60, 40, 1, 1, 0, 3),
    S(5300, FK_TUMBLER, 0, 0, 1, -1, 0, 0),
    S(5600, FK_SHARD, 200, 0, 5, 1, 20, -30),
    S(5900, FK_HOOP, 40, 0, 1, 1, 0, 0),
    S(5960, FK_HOOP, 280, 0, 1, -1, 0, 0),
    S(6300, FK_PRISM, 270, 34, 1, 1, 0, 2),
    S(6400, FK_SHARD, 60, 0, 6, 1, 16, 40),
    S(6700, FK_TUMBLER, 0, 0, 1, 1, 0, 0),
    S(6800, FK_TUMBLER, 0, 0, 1, -1, 0, 0),
    S(7000, FK_SHARD, 280, 0, 6, 1, 16, -40),
    S(7300, FK_HOOP, 120, 0, 1, -1, 0, 0),
    S(7500, FK_SHARD, 100, 0, 5, 1, 20, 30),
    S(7800, FK_TUMBLER, 0, 0, 1, -1, 0, 0),
    S(8000, FK_SHARD, 60, 0, 6, 1, 16, 40),
    S(8250, FK_HOOP, 200, 0, 1, -1, 0, 0),
    S(8300, FK_PRISM, 40, 40, 1, 1, 0, 2),
};

static const DflSpawn MOONLIT[] = {
    S(120, FK_SKULL, 60, 0, 3, 1, 40, 60),
    S(500, FK_SKULL, 260, 0, 3, 1, 40, -60),
    S(800, FK_SHEET, 100, 40, 1, 1, 0, 0),
    S(1100, FK_SKULL, 160, 0, 4, 1, 30, 30),
    S(1400, FK_SNAPPER, 240, 90, 1, 1, 0, 0),
    S(1700, FK_SKULL, 40, 0, 4, 1, 30, 50),
    S(1900, FK_SHEET, 220, 36, 1, 1, 0, 0),
    S(2200, FK_SLAB, 160, 0, 1, 1, 0, 0),
    S(2500, FK_SKULL, 280, 0, 4, 1, 30, -50),
    S(2800, FK_SNAPPER, 80, 80, 1, 1, 0, 0),
    S(3000, FK_SHEET, 60, 44, 1, 1, 0, 0),
    S(3010, FK_SHEET, 260, 44, 1, 1, 0, 0),
    S(3400, FK_SKULL, 100, 0, 5, 1, 24, 30),
    S(3700, FK_SAUCER, 0, 30, 1, 1, 0, 0),
    S(3800, FK_SKULL, 200, 0, 3, 1, 40, 40),
    S(4300, FK_SNAPPER, 160, 80, 2, 1, 60, 60),
    S(4600, FK_SLAB, 80, 0, 1, 1, 0, 0),
    S(4800, FK_SKULL, 240, 0, 4, 1, 30, -40),
    S(5100, FK_SHEET, 160, 36, 1, 1, 0, 0),
    S(5400, FK_SKULL, 40, 0, 5, 1, 24, 50),
    S(5700, FK_SNAPPER, 200, 90, 1, 1, 0, 0),
    S(5900, FK_SHEET, 80, 40, 1, 1, 0, 0),
    S(5910, FK_SHEET, 240, 40, 1, 1, 0, 0),
    S(6300, FK_SLAB, 240, 0, 1, 1, 0, 0),
    S(6500, FK_SKULL, 160, 0, 6, 1, 20, 20),
    S(6900, FK_SNAPPER, 100, 84, 2, 1, 40, 120),
    S(7200, FK_SHEET, 200, 40, 1, 1, 0, 0),
    S(7400, FK_SKULL, 60, 0, 5, 1, 24, 40),
    S(7750, FK_SNAPPER, 160, 84, 1, 1, 0, 0),
    S(8000, FK_SKULL, 240, 0, 5, 1, 24, -40),
    S(8300, FK_SHEET, 120, 40, 1, 1, 0, 0),
};

static const DflSpawn OPENWATER[] = {
    S(120, FK_JELLY, 60, 100, 4, 1, 20, 50),
    S(500, FK_DARTFISH, 240, 0, 3, 1, 30, -30),
    S(800, FK_JELLY, 200, 110, 4, 1, 20, 30),
    S(1100, FK_SQUIRT, 0, 50, 1, -1, 0, 0),
    S(1400, FK_DARTFISH, 80, 0, 4, -1, 30, 40),
    S(1700, FK_JELLY, 40, 96, 6, 1, 16, 46),
    S(2000, FK_SHARK, 0, 0, 1, 1, 0, 0),
    S(2300, FK_DARTFISH, 200, 0, 4, 1, 24, -40),
    S(2600, FK_SQUIRT, 0, 40, 1, 1, 0, 0),
    S(2700, FK_SQUIRT, 0, 64, 1, -1, 0, 0),
    S(3000, FK_JELLY, 100, 104, 5, 1, 20, 30),
    S(3300, FK_DARTFISH, 60, 0, 5, 1, 24, 50),
    S(3700, FK_JELLY, 280, 100, 5, 1, 20, -40),
    S(4000, FK_SQUIRT, 0, 46, 1, 1, 0, 0),
    S(4300, FK_DARTFISH, 160, 0, 4, -1, 20, 30),
    S(4600, FK_SHARK, 0, 0, 1, -1, 0, 0),
    S(4900, FK_JELLY, 60, 110, 6, 1, 14, 40),
    S(5200, FK_DARTFISH, 260, 0, 5, 1, 20, -40),
    S(5500, FK_SQUIRT, 0, 56, 1, -1, 0, 0),
    S(5600, FK_SQUIRT, 0, 36, 1, 1, 0, 0),
    S(5900, FK_JELLY, 120, 100, 5, 1, 16, 30),
    S(6200, FK_DARTFISH, 100, 0, 5, -1, 20, 40),
    S(6500, FK_SQUIRT, 0, 44, 1, 1, 0, 0),
    S(6600, FK_SQUIRT, 0, 60, 1, -1, 0, 0),
    S(6900, FK_JELLY, 40, 96, 6, 1, 14, 50),
    S(7200, FK_DARTFISH, 220, 0, 4, 1, 24, -40),
    S(7500, FK_JELLY, 180, 106, 4, 1, 18, 30),
    S(7800, FK_SQUIRT, 0, 50, 1, 1, 0, 0),
    S(8050, FK_DARTFISH, 60, 0, 4, 1, 24, 50),
    S(8300, FK_JELLY, 120, 104, 5, 1, 16, 30),
};

const DflStage DFL_STAGE[DFL_STAGES] = {
    {"HARBOUR ROAD", HARBOUR, ARRAY_LEN(HARBOUR), 8700},
    {"SUNDOWN STRIP", SUNDOWN, ARRAY_LEN(SUNDOWN), 8700},
    {"MOONLIT MILE", MOONLIT, ARRAY_LEN(MOONLIT), 8700},
    {"OPEN WATER", OPENWATER, ARRAY_LEN(OPENWATER), 8700},
};

const char *const DFL_STAGE_TIME[DFL_STAGES] = {"MORNING", "SUNSET", "MIDNIGHT", "DAYBREAK"};

/* stage 4: where each swell breaks, all the way through (the boss too) */
const DflSwellCue DFL_SWELLS[] = {
    {300, 80}, {555, 220}, {810, 150}, {1065, 40}, {1320, 270}, {1575, 120}, {1830, 200},
    {2085, 60}, {2340, 240}, {2595, 160}, {2850, 100}, {3105, 280}, {3360, 30}, {3615, 190},
    {3870, 130}, {4125, 250}, {4380, 70}, {4635, 210}, {4890, 140}, {5145, 50}, {5400, 230},
    {5655, 110}, {5910, 260}, {6165, 90}, {6420, 180}, {6675, 36}, {6930, 284}, {7185, 150},
    {7440, 76}, {7695, 226}, {7950, 120}, {8205, 196}, {8460, 46}, {8715, 266},
};
const int DFL_SWELL_COUNT = ARRAY_LEN(DFL_SWELLS);

/* the bonus stages: a sun, a sunset and a full moon in blocks */
const char *const DFL_BONUS_MAP[3][DFL_BROWS] = {
    {
        "....####....",
        "..##....##..",
        ".#........#.",
        ".#........#.",
        "..##....##..",
        ".....##.....",
        "............",
    },
    {
        "...######...",
        "............",
        "..########..",
        "............",
        "##...##...##",
        "............",
        "............",
    },
    {
        ".....##.....",
        "....####....",
        "...##..##...",
        "...#....#...",
        "...##..##...",
        "....####....",
        ".....##.....",
    },
};
