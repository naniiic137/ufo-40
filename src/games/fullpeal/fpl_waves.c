/* FULL PEAL - the waves. Every one is fixed: the same formations in the
 * same order every time, built from a library of formations that come
 * back and get remixed as the run goes on (each named after a method
 * rung on church bells). All of them are ours.
 *
 * A spawn: {frame, kind, column, row, arg}. Columns 0-5 and rows 0-3 pick
 * a lane; column -1 or 6 brings the foe in from the left or right end of
 * the plane (a crosser, a lookout, a bourdon, a wisp). arg: a pendulum
 * starting upward (2); a bourdon's landing column; a dodger's first slip
 * (1 = left). */
#include "fpl.h"

#define CL EK_CLAPPER
#define TN EK_TENOR
#define TB EK_TREBLE
#define DG EK_DODGER
#define BD EK_BOURDON
#define CH EK_CROSSHEAD
#define FK EK_FORKER
#define PD EK_PENDULUM
#define SP EK_SPITE
#define SA EK_SALLY
#define QS EK_QUICKSALLY
#define LO EK_LOOKOUT
#define MO EK_MOTE
#define NB EK_NIBBLER
#define CT EK_CALTROP
#define BR EK_BROODER
#define L (-1)
#define R 6

/* ---- stage A's formations: clappers, crossheads, pendulums ---------- */
static const FplSpawn ROUNDS[] = {
    {0, CL, 0, 1, 0}, {18, CL, 1, 1, 0}, {36, CL, 2, 1, 0}, {54, CL, 3, 1, 0}, {72, CL, 4, 1, 0}, {90, CL, 5, 1, 0},
};
static const FplSpawn BACKROUNDS[] = {
    {0, CL, 5, 2, 0}, {18, CL, 4, 2, 0}, {36, CL, 3, 2, 0}, {54, CL, 2, 2, 0}, {72, CL, 1, 2, 0}, {90, CL, 0, 2, 0},
};
static const FplSpawn CALLCHANGE[] = {
    {0, CL, 2, 1, 0}, {0, CL, 3, 2, 0}, {30, CL, 3, 1, 0}, {30, CL, 2, 2, 0},
    {90, CL, 0, 0, 0}, {90, CL, 5, 3, 0}, {120, CL, 5, 0, 0}, {120, CL, 0, 3, 0},
};
static const FplSpawn PLAINHUNT[] = {
    {0, CL, 0, 0, 0}, {24, CL, 1, 1, 0}, {48, CL, 2, 2, 0}, {72, CL, 3, 3, 0},
    {110, CL, 5, 0, 0}, {134, CL, 4, 1, 0}, {158, CL, 3, 2, 0}, {182, CL, 2, 3, 0},
};
static const FplSpawn QUEENS[] = {
    {0, PD, 1, 0, 0}, {0, PD, 4, 3, 2}, {60, PD, 2, 3, 2}, {60, PD, 3, 0, 0},
};
static const FplSpawn TITTUMS[] = {
    {0, CH, 1, 1, 0}, {40, CH, 4, 2, 0}, {120, CL, 2, 0, 0}, {140, CL, 3, 3, 0}, {160, CL, 0, 2, 0},
};
static const FplSpawn WHITTINGTONS[] = {
    {0, CL, L, 0, 0}, {40, CL, R, 3, 0}, {80, CL, L, 1, 0}, {120, CL, R, 2, 0},
    {150, CL, 2, 0, 0}, {150, CL, 3, 3, 0},
};
static const FplSpawn KENT[] = {
    {0, PD, 0, 1, 0}, {30, PD, 2, 2, 2}, {60, PD, 4, 1, 0}, {80, CH, 5, 0, 0}, {80, CH, 0, 3, 0},
};
static const FplSpawn STEDMAN[] = {
    {0, CH, 0, 0, 0}, {0, CH, 5, 3, 0}, {50, CL, 1, 1, 0}, {50, CL, 4, 2, 0}, {90, CL, 2, 2, 0},
    {90, CL, 3, 1, 0}, {140, PD, 2, 0, 0}, {140, PD, 3, 3, 2},
};

/* ---- stage B: sallies, forkers, tenors, lookouts ----------------------- */
static const FplSpawn BOBMINOR[] = {
    {0, SA, 1, 1, 0}, {50, SA, 4, 2, 0}, {100, SA, 2, 3, 0}, {150, SA, 3, 0, 0},
};
static const FplSpawn GRANDSIRE[] = {
    {0, TN, 2, 1, 0}, {0, TN, 3, 2, 0}, {70, CL, 0, 0, 0}, {70, CL, 5, 3, 0}, {100, CL, 5, 0, 0}, {100, CL, 0, 3, 0},
};
static const FplSpawn DOUBLES[] = {
    {0, FK, 0, 1, 0}, {30, FK, 5, 2, 0}, {90, SA, 2, 1, 0}, {120, SA, 3, 2, 0},
};
static const FplSpawn LOOKOUTS[] = {
    {0, LO, L, 1, 0}, {60, LO, R, 2, 0}, {90, CL, 2, 0, 0}, {90, CL, 3, 3, 0}, {150, LO, L, 3, 0},
};
static const FplSpawn CAMBRIDGE[] = {
    {0, TN, 1, 1, 0}, {20, TN, 2, 1, 0}, {40, TN, 3, 1, 0}, {60, TN, 4, 1, 0},
};
static const FplSpawn SINGLES[] = {
    {0, SA, 0, 0, 0}, {40, SA, 5, 3, 0}, {80, SA, 2, 2, 0}, {120, SA, 3, 1, 0}, {160, SA, 1, 3, 0},
};
static const FplSpawn OXFORD[] = {
    {0, LO, L, 0, 0}, {0, LO, R, 3, 0}, {40, CH, 2, 1, 0}, {80, FK, 3, 2, 0}, {140, TN, 0, 2, 0}, {140, TN, 5, 1, 0},
};
static const FplSpawn LONDON[] = {
    {0, TN, L, 1, 0}, {50, TN, R, 2, 0}, {100, SA, 1, 0, 0}, {100, SA, 4, 3, 0}, {160, PD, 2, 0, 0}, {160, PD, 3, 3, 2},
};

/* ---- stage C: motes, bourdons, quick sallies, nibblers, spites --------- */
static const FplSpawn MURK[] = {
    {0, MO, 0, 0, 0}, {30, MO, 5, 3, 0}, {60, MO, 5, 0, 0}, {90, MO, 0, 3, 0},
};
static const FplSpawn GREATBELL[] = {
    {0, BD, L, 1, 1}, {40, BD, R, 2, 4}, {100, CL, 2, 0, 0}, {100, CL, 3, 3, 0}, {130, CL, 0, 0, 0}, {130, CL, 5, 3, 0},
};
static const FplSpawn SPITEFUL[] = {
    {0, SP, 1, 0, 0}, {40, SP, 4, 3, 2}, {80, SP, 2, 1, 0}, {80, SP, 3, 2, 2},
};
static const FplSpawn QUICKBOB[] = {
    {0, QS, 0, 1, 0}, {30, QS, 5, 2, 0}, {60, QS, 2, 0, 0}, {90, QS, 3, 3, 0},
};
static const FplSpawn SHOAL[] = {
    {0, NB, 0, 0, 0}, {40, NB, 5, 3, 0}, {100, MO, 2, 1, 0}, {100, MO, 3, 2, 0}, {150, NB, 5, 0, 0},
};
static const FplSpawn TOLL[] = {
    {0, BD, L, 0, 2}, {0, BD, R, 3, 3}, {60, PD, 0, 1, 0}, {60, PD, 5, 2, 2}, {120, SP, 1, 3, 2}, {120, SP, 4, 0, 0},
};
static const FplSpawn CORNERS[] = {
    {0, CH, 0, 0, 0}, {0, FK, 5, 0, 0}, {30, CH, 5, 3, 0}, {30, FK, 0, 3, 0}, {100, MO, 2, 2, 0}, {100, MO, 3, 1, 0},
};

/* ---- stage D: trebles, caltrops, brooders ------------------------------- */
static const FplSpawn TREBLES[] = {
    {0, TB, 0, 0, 0}, {14, TB, 1, 1, 0}, {28, TB, 2, 2, 0}, {42, TB, 3, 3, 0}, {56, TB, 4, 2, 0}, {70, TB, 5, 1, 0},
};
static const FplSpawn SPIKES[] = {
    {0, CT, 0, 0, 0}, {0, CT, 1, 0, 0}, {0, CT, 4, 3, 0}, {0, CT, 5, 3, 0},
    {60, CT, 2, 1, 0}, {60, CT, 3, 1, 0}, {60, CT, 2, 2, 0}, {60, CT, 3, 2, 0},
    {130, CT, 0, 3, 0}, {130, CT, 5, 0, 0}, {130, CT, 1, 2, 0}, {130, CT, 4, 1, 0},
};
static const FplSpawn BROOD[] = {
    {0, BR, 1, 1, 0}, {40, BR, 4, 2, 0}, {120, CL, 0, 0, 0}, {120, CL, 5, 3, 0},
};
static const FplSpawn ZIGZAG[] = {
    {0, TB, 0, 3, 0}, {12, TB, 1, 2, 0}, {24, TB, 2, 3, 0}, {36, TB, 3, 2, 0}, {48, TB, 4, 3, 0}, {60, TB, 5, 2, 0},
    {100, TB, 5, 0, 0}, {112, TB, 4, 1, 0}, {124, TB, 3, 0, 0}, {136, TB, 2, 1, 0}, {148, TB, 1, 0, 0}, {160, TB, 0, 1, 0},
};
static const FplSpawn GAUNTLET[] = {
    {0, CT, 0, 1, 0}, {0, CT, 5, 2, 0}, {40, TB, 2, 1, 0}, {40, TB, 3, 2, 0},
    {90, CT, 2, 0, 0}, {90, CT, 3, 3, 0}, {130, TB, 0, 0, 0}, {130, TB, 5, 3, 0}, {170, QS, 1, 2, 0}, {170, QS, 4, 1, 0},
};
static const FplSpawn NEST[] = {
    {0, BR, 0, 0, 0}, {0, BR, 5, 3, 0}, {60, NB, 2, 1, 0}, {120, TB, L, 2, 0}, {150, TB, R, 1, 0},
};
static const FplSpawn ANVIL[] = {
    {0, BD, L, 2, 0}, {0, BD, R, 1, 5}, {30, BD, L, 0, 2}, {30, BD, R, 3, 3},
    {120, TB, 1, 1, 0}, {120, TB, 4, 2, 0}, {150, TB, 2, 3, 0}, {150, TB, 3, 0, 0},
};

/* ---- stage E: dodgers, and everything before -------------------------- */
static const FplSpawn DODGES[] = {
    {0, DG, 1, 1, 0}, {30, DG, 4, 2, 1}, {60, DG, 2, 2, 1}, {90, DG, 3, 1, 0},
};
static const FplSpawn LONGLENGTH[] = {
    {0, DG, 0, 0, 0}, {0, DG, 5, 3, 1}, {50, CH, 2, 1, 0}, {50, FK, 3, 2, 0}, {120, TN, 1, 3, 0}, {120, TN, 4, 0, 0},
};
static const FplSpawn FULLCIRCLE[] = {
    {0, CL, L, 0, 0}, {0, CL, R, 3, 0}, {40, SA, 1, 1, 0}, {40, SA, 4, 2, 0}, {80, PD, 2, 0, 0}, {80, PD, 3, 3, 2},
    {120, MO, 0, 2, 0}, {120, MO, 5, 1, 0},
};
static const FplSpawn QUARTER[] = {
    {0, LO, L, 1, 0}, {0, LO, R, 2, 0}, {50, DG, 2, 0, 1}, {50, DG, 3, 3, 0}, {110, BR, 2, 2, 0}, {110, NB, 0, 0, 0},
};
static const FplSpawn SURPRISE[] = {
    {0, QS, 0, 0, 0}, {20, QS, 5, 0, 0}, {40, QS, 0, 3, 0}, {60, QS, 5, 3, 0}, {100, SP, 2, 1, 0}, {100, SP, 3, 2, 2},
};
static const FplSpawn DELIGHT[] = {
    {0, CT, 1, 1, 0}, {0, CT, 4, 2, 0}, {30, DG, 0, 2, 0}, {30, DG, 5, 1, 1}, {90, CT, 2, 2, 0}, {90, CT, 3, 1, 0},
    {130, TN, 2, 0, 0}, {130, TN, 3, 3, 0},
};

#define FORM(name, arr) {name, arr, ARRAY_LEN(arr)}
enum {
    F_ROUNDS, F_BACKROUNDS, F_CALLCHANGE, F_PLAINHUNT, F_QUEENS, F_TITTUMS, F_WHITTINGTONS, F_KENT, F_STEDMAN,
    F_BOBMINOR, F_GRANDSIRE, F_DOUBLES, F_LOOKOUTS, F_CAMBRIDGE, F_SINGLES, F_OXFORD, F_LONDON,
    F_MURK, F_GREATBELL, F_SPITEFUL, F_QUICKBOB, F_SHOAL, F_TOLL, F_CORNERS,
    F_TREBLES, F_SPIKES, F_BROOD, F_ZIGZAG, F_GAUNTLET, F_NEST, F_ANVIL,
    F_DODGES, F_LONGLENGTH, F_FULLCIRCLE, F_QUARTER, F_SURPRISE, F_DELIGHT,
    F_COUNT
};
const FplForm FPL_FORMS[] = {
    FORM("ROUNDS", ROUNDS), FORM("BACK ROUNDS", BACKROUNDS), FORM("CALL CHANGE", CALLCHANGE),
    FORM("PLAIN HUNT", PLAINHUNT), FORM("QUEENS", QUEENS), FORM("TITTUMS", TITTUMS),
    FORM("WHITTINGTONS", WHITTINGTONS), FORM("KENT", KENT), FORM("STEDMAN", STEDMAN),
    FORM("BOB MINOR", BOBMINOR), FORM("GRANDSIRE", GRANDSIRE), FORM("DOUBLES", DOUBLES),
    FORM("LOOKOUTS", LOOKOUTS), FORM("CAMBRIDGE", CAMBRIDGE), FORM("SINGLES", SINGLES),
    FORM("OXFORD", OXFORD), FORM("LONDON", LONDON),
    FORM("MURK", MURK), FORM("GREAT BELL", GREATBELL), FORM("SPITEFUL", SPITEFUL),
    FORM("QUICK BOB", QUICKBOB), FORM("SHOAL", SHOAL), FORM("TOLL", TOLL), FORM("CORNERS", CORNERS),
    FORM("TREBLES", TREBLES), FORM("SPIKES", SPIKES), FORM("BROOD", BROOD), FORM("ZIGZAG", ZIGZAG),
    FORM("GAUNTLET", GAUNTLET), FORM("NEST", NEST), FORM("ANVIL", ANVIL),
    FORM("DODGES", DODGES), FORM("LONG LENGTH", LONGLENGTH), FORM("FULL CIRCLE", FULLCIRCLE),
    FORM("QUARTER", QUARTER), FORM("SURPRISE", SURPRISE), FORM("DELIGHT", DELIGHT),
};
const int FPL_FORM_COUNT = F_COUNT;

/* ---- the twenty waves ------------------------------------------------------ */
static const uint8_t A1[] = {F_ROUNDS, F_BACKROUNDS, F_CALLCHANGE, F_PLAINHUNT};
static const uint8_t A2[] = {F_QUEENS, F_ROUNDS, F_WHITTINGTONS, F_TITTUMS, F_QUEENS};
static const uint8_t A3[] = {F_TITTUMS, F_PLAINHUNT, F_WHITTINGTONS, F_KENT, F_CALLCHANGE};
static const uint8_t A4[] = {F_KENT, F_STEDMAN, F_QUEENS, F_BACKROUNDS, F_TITTUMS, F_WHITTINGTONS};

static const uint8_t B1[] = {F_BOBMINOR, F_ROUNDS, F_GRANDSIRE, F_BOBMINOR, F_CALLCHANGE};
static const uint8_t B2[] = {F_DOUBLES, F_LOOKOUTS, F_QUEENS, F_CAMBRIDGE, F_SINGLES};
static const uint8_t B3[] = {F_GRANDSIRE, F_OXFORD, F_SINGLES, F_WHITTINGTONS, F_DOUBLES, F_KENT};
static const uint8_t B4[] = {F_LONDON, F_CAMBRIDGE, F_LOOKOUTS, F_BOBMINOR, F_OXFORD, F_STEDMAN, F_GRANDSIRE};

static const uint8_t C1[] = {F_MURK, F_QUEENS, F_GREATBELL, F_MURK, F_SPITEFUL};
static const uint8_t C2[] = {F_QUICKBOB, F_SHOAL, F_TOLL, F_LOOKOUTS, F_SPITEFUL, F_MURK};
static const uint8_t C3[] = {F_CORNERS, F_GREATBELL, F_QUICKBOB, F_SHOAL, F_DOUBLES, F_TOLL};
static const uint8_t C4[] = {F_SHOAL, F_SPITEFUL, F_CORNERS, F_TOLL, F_QUICKBOB, F_GREATBELL, F_MURK, F_OXFORD};

static const uint8_t D1[] = {F_TREBLES, F_BROOD, F_SPIKES, F_TREBLES, F_GRANDSIRE};
static const uint8_t D2[] = {F_ZIGZAG, F_SPIKES, F_NEST, F_QUICKBOB, F_GAUNTLET, F_BROOD};
static const uint8_t D3[] = {F_GAUNTLET, F_TREBLES, F_ANVIL, F_BROOD, F_SPIKES, F_SHOAL, F_ZIGZAG, F_TOLL, F_NEST,
                             F_SPITEFUL, F_GAUNTLET, F_TREBLES};
static const uint8_t D4[] = {F_NEST, F_ANVIL, F_ZIGZAG, F_CORNERS, F_GAUNTLET, F_BROOD, F_LONDON};

static const uint8_t E1[] = {F_DODGES, F_FULLCIRCLE, F_LONGLENGTH, F_DODGES, F_SURPRISE};
static const uint8_t E2[] = {F_QUARTER, F_DELIGHT, F_FULLCIRCLE, F_ANVIL, F_DODGES, F_OXFORD};
static const uint8_t E3[] = {F_SURPRISE, F_LONGLENGTH, F_GAUNTLET, F_QUARTER, F_TOLL, F_DELIGHT, F_NEST};
static const uint8_t E4[] = {F_FULLCIRCLE, F_DODGES, F_QUARTER, F_SPIKES, F_LONGLENGTH, F_SHOAL, F_SURPRISE,
                             F_ANVIL, F_DELIGHT, F_STEDMAN, F_ZIGZAG, F_FULLCIRCLE};

#define WAVE(a) {a, ARRAY_LEN(a)}
const FplStage FPL_STAGE[FPL_STAGES] = {
    {'A', "OUTER BELFRY", "CLARY HERE. EASY OUT THERE, BROTHER!",
     {WAVE(A1), WAVE(A2), WAVE(A3), WAVE(A4)}, BOSS_GLOAMEYE, C_NAVY, C_BLUE},
    {'B', "TIN NEBULA", "ODD SIGNALS COMING OFF KNELL. STAND BY!",
     {WAVE(B1), WAVE(B2), WAVE(B3), WAVE(B4)}, BOSS_KNUCKLEBELL, C_PURPLE, C_VIOLET},
    {'C', "THE MURK", "KNELL ISN'T KNELL ANY MORE. IT'S ALL CHANGED!",
     {WAVE(C1), WAVE(C2), WAVE(C3), WAVE(C4)}, BOSS_INKWELL, C_TEAL, C_FOREST},
    {'D', "TOLLGATE", "ANSEL, COME IN! TURN ROUND! TROUBLE AHEAD!",
     {WAVE(D1), WAVE(D2), WAVE(D3), WAVE(D4)}, BOSS_SHELLBACK, C_MAROON, C_WINE},
    {'E', "KNELL", "#A..N%S  !E?L..# ..A#NS?EL",
     {WAVE(E1), WAVE(E2), WAVE(E3), WAVE(E4)}, BOSS_SORDINA, C_NIGHT, C_DUSK},
};

int fpl_wave_total(int stage, int wave) {
    const FplWave *w = &FPL_STAGE[stage].wave[wave];
    int n = 0;
    for (int f = 0; f < w->n; f++) {
        const FplForm *fm = &FPL_FORMS[w->forms[f]];
        for (int k = 0; k < fm->n; k++) n += FPL_FOE[fm->s[k].kind].counted;
    }
    return n;
}

int fpl_wave_foes(int stage, int wave) {
    const FplWave *w = &FPL_STAGE[stage].wave[wave];
    int n = 0;
    for (int f = 0; f < w->n; f++) n += FPL_FORMS[w->forms[f]].n;
    return n;
}

int fpl_wave_forms(int stage, int wave) { return FPL_STAGE[stage].wave[wave].n; }
