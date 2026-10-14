/* LIMPET - every deck (temporary test deck while the engine is checked). */
#include "lmp.h"

static const LmpLevel TEST = {
    "TEST", LK_NORMAL,
    {
        "##############################",
        "#............................#",
        "#............................#",
        "#.....................R......#",
        "#............##########......#",
        "#............##########......#",
        "#.........................E..#",
        "#.......................######",
        "#............................#",
        "#..H.........................#",
        "###!##.......................#",
        "#............................#",
        "#.........###................#",
        "#J.......F#####7.............#",
        "#@........#####..............#",
        "##############################",
    },
    "( ! H ) B E", NULL, 0, NULL, 0, 0,
};

static const LmpLevel HUB = {
    "SPINDLE STATION", LK_HUB,
    {
        "##############################",
        "#............................#",
        "#............................#",
        "#............................#",
        "#............................#",
        "#............................#",
        "#............................#",
        "#............................#",
        "#............................#",
        "#............................#",
        "#............................#",
        "#............................#",
        "#............................#",
        "#..@....$...N....$...........#",
        "##############################",
    },
    NULL, NULL, 0, NULL, 0, 0,
};

const LmpLevel *const LMP_LEVEL[LMP_LEVELS] = {
    &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &TEST,
    &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &TEST,
    &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &TEST, &HUB,
};

const LmpPod LMP_HUB_POD[] = {{0}, {1}};
const int LMP_HUB_NPODS = 2;
const LmpGate LMP_HUB_GATE[] = {{0, 0, 0, 0, GATE_CLEARED, 99, ""}};
const int LMP_HUB_NGATES = 1;
const LmpNpc LMP_HUB_NPC[] = {{"GUS", {"HELLO", NULL, NULL, NULL}}};
const int LMP_HUB_NNPCS = 1;
const LmpZone LMP_HUB_ZONE[] = {{0, 200, 0, 200, ZN_CONCOURSE}};
const int LMP_HUB_NZONES = 1;
const char *const LMP_ZONE_NAME[ZN_COUNT] = {"CONCOURSE", "CANTEEN", "GRISTLE WING", "UPPER RING"};
const char *const LMP_SPEAK_NAME = "";
