/* DUKES UP - the five nights, all drawn for UFO 40. Each section is a
 * script keyed to how far the view has scrolled: ghouls come on when the
 * view passes their mark, whoever is still standing. Floor depth runs from
 * DKU_FLOOR0 (108, the back) to DKU_FLOOR1 (172, the front). */
#include "dku.h"

#define S(at, k, from, mode, x, y) {at, EV_SPAWN, AK_##k, from, mode, x, y, 0, 0}
#define L(at, k, y) S(at, k, FROM_LEFT, 0, 0, y)
#define R(at, k, y) S(at, k, FROM_RIGHT, 0, 0, y)
#define AT(at, k, mode, x, y) S(at, k, FROM_AT, mode, x, y)
#define P(k, x, y, c1, c2) {0, EV_PROP, PR_##k, c1, c2, x, y, 0, 0}
#define ITEM(k, x, y) {0, EV_ITEM, IT_##k, 0, 0, x, y, 0, 0}
#define LOCK(at) {at, EV_LOCK, 0, 0, 0, 0, 0, 0, 0}
#define PIT(x, y, w, h) {0, EV_PIT, 0, 0, 0, x, y, w, h}
#define WATER(x, y, w, h) {0, EV_WATER, 0, 0, 0, x, y, w, h}
#define MINE(x, y) {0, EV_MINE, 0, 0, 0, x, y, 0, 0}
#define LAMP(x, y, drop) {0, EV_LAMP, 0, drop, 0, x, y, 0, 0}
#define FIREWALL(x, y, w, h) {0, EV_FIREWALL, 0, 0, 0, x, y, w, h}
#define CAR(at, y) {at, EV_CAR, 0, 0, 0, 0, y, 0, 0}
#define THRESHER(at, stop) {at, EV_THRESHER, 0, 0, 0, stop, 0, 0, 0}
#define SAUCER(at) {at, EV_SAUCER, 0, 0, 0, 0, 0, 0, 0}
#define DOG(x, y) {0, EV_DOG, 0, 0, 0, x, y, 0, 0}
#define PASSER(at, y, drop) {at, EV_PASSER, 0, drop, 0, 0, y, 0, 0}
#define BOSS(k, x, y) {0, EV_BOSS, AK_##k, 0, 0, x, y, 0, 0}
#define WAVE(n, k, c) {0, EV_WAVE, n, AK_##k, c, 0, 0, 0, 0}
#define BEAM(x) {0, EV_BEAM, 0, 0, 0, x, 0, 0, 0}
#define STREAM(k, every10, most) {0, EV_STREAM, k, every10, most, 0, 0, 0, 0}
#define DOOR(x) {0, EV_DOOR, 0, 0, 0, x, 0, 0, 0}
/* a way on that opens before the end of the street, once nobody is left */
#define LIFTDOOR(at, x) {at, EV_DOOR, 0, 1, 0, x, 0, 0, 0}
#define TUFT(x, y, c) P(TUFT, x, y, c, 0)

#define SEC(name, th, kind, len, ev) {name, th, kind, len, ev, ARRAY_LEN(ev)}

/* ===== NIGHT 1: Market Street, then the roller rink ===================== */
static const DkuEvt N1A[] = {
    P(BIN, 92, 120, IT_COIN, 0),
    R(40, SHAMBLER, 132),
    R(40, SHAMBLER, 158),
    AT(150, SHAMBLER, DM_FEED, 470, 122),
    DOG(560, 162),
    R(230, TORCH, 140),
    S(330, SHAMBLER, FROM_DOOR, 0, 700, 0),
    S(330, SHAMBLER, FROM_DOOR, 0, 716, 0),
    L(330, SHAMBLER, 150),
    LOCK(330),
    P(BIN, 760, 156, IT_APPLE, 0),
    SAUCER(440),
    R(520, TORCH, 120),
    R(540, SHAMBLER, 150),
    R(560, SHAMBLER, 164),
    CAR(600, 146),
    P(BIN, 980, 130, IT_NOTE, 0),
    ITEM(COIN, 1010, 160),
    R(720, CROW, 118),
    R(760, CROW, 160),
    R(840, TORCH, 140),
    L(900, SHAMBLER, 124),
    L(900, SHAMBLER, 160),
    L(900, CROW, 140),
    R(900, SHAMBLER, 140),
    LOCK(900),
    FIREWALL(1160, 108, 44, 22),
    P(BOX, 1240, 162, IT_SANDWICH, 0),
    ITEM(NOTE, 1300, 124),
    AT(1100, SHAMBLER, DM_FEED, 1380, 150),
    AT(1100, SHAMBLER, DM_FEED, 1420, 128),
    R(1180, TORCH, 132),
    DOOR(1540),
};
static const DkuEvt N1B[] = {
    BOSS(RAMMER, 250, 140),
    AT(0, SHAMBLER, DM_DANCE, 100, 118),
    AT(0, SHAMBLER, DM_DANCE, 150, 162),
    AT(0, SHAMBLER, DM_DANCE, 205, 116),
    AT(0, SHAMBLER, DM_DANCE, 70, 150),
    P(BOX, 30, 162, IT_SANDWICH, 0),
};
static const DkuSection N1[] = {
    SEC("MARKET STREET", TH_STREET, SEC_WALK, 1600, N1A),
    SEC("THE ROLLER RINK", TH_CLUB, SEC_BOSS, 320, N1B),
};

/* ===== NIGHT 2: Canal Row, the night ferry, the waxworks ===================== */
static const DkuEvt N2A[] = {
    P(BIN, 80, 124, IT_COIN, 0),
    AT(0, SHAMBLER, DM_FEED, 330, 150),
    R(60, SHAMBLER, 132),
    R(200, TORCH, 120),
    L(200, SHAMBLER, 140),
    L(200, SHAMBLER, 164),
    L(220, RAMMER, 150),
    LOCK(240),
    R(420, TORCH, 160),
    R(420, SHAMBLER, 120),
    R(440, SHAMBLER, 140),
    PASSER(560, 140, IT_SCATTER),
    L(600, CROW, 120),
    L(600, SHAMBLER, 160),
    L(620, SHAMBLER, 140),
    LOCK(620),
    P(BIN, 950, 160, IT_COIN, 0),
    P(SIGN, 1050, 118, IT_NOTE, 0),
    R(820, SHAMBLER, 130),
    R(840, CROW, 156),
    DOOR(1240),
};
static const DkuEvt N2B[] = {
    AT(0, CROW, DM_SLEEP, 300, 120),
    L(150, CROW, 150),
    L(150, SHAMBLER, 130),
    AT(0, SHAMBLER, DM_SIT, 440, 112),
    AT(0, SHAMBLER, DM_FEED, 560, 156),
    SAUCER(380),
    P(CRATE, 700, 120, IT_NOTE, 0),
    P(CRATE, 740, 162, IT_NOTE, 0),
    P(CRATE, 780, 140, IT_APPLE, 0),
    L(560, HOWLER, 120),
    L(570, HOWLER, 150),
    L(580, HOWLER, 165),
    LOCK(580),
    AT(0, SHAMBLER, DM_SIT, 960, 112),
    AT(0, SHAMBLER, DM_SIT, 1080, 112),
    AT(0, SHAMBLER, DM_SIT, 1160, 112),
    AT(0, CROW, DM_SLEEP, 1250, 150),
    R(1000, HOWLER, 140),
    P(CRATE, 1300, 120, IT_APPLE, 0),
    P(CRATE, 1330, 162, IT_NOTE, 0),
    DOOR(1340),
};
static const DkuEvt N2C[] = {
    BOSS(TUSKER, 250, 140),
    AT(0, SHAMBLER, DM_FEED, 110, 124),
    AT(0, SHAMBLER, DM_FEED, 150, 156),
    P(BIN, 40, 124, IT_SANDWICH, 0),
    STREAM(AK_HOWLER, 150, 1),
};
static const DkuSection N2[] = {
    SEC("CANAL ROW", TH_CANAL, SEC_WALK, 1300, N2A),
    SEC("THE NIGHT FERRY", TH_FERRY, SEC_WALK, 1400, N2B),
    SEC("THE WAXWORKS", TH_WAXWORKS, SEC_BOSS, 320, N2C),
};

/* ===== NIGHT 3: the old orchard, the graveyard, the visitors' ship ============ */
static const DkuEvt N3A[] = {
    MINE(200, 150),
    S(120, SLUDGER, FROM_ABOVE, 0, 330, 126),
    AT(0, SHAMBLER, DM_FEED, 430, 122),
    AT(0, SHAMBLER, DM_FEED, 460, 152),
    AT(0, SHAMBLER, DM_FEED, 500, 136),
    MINE(520, 164),
    PIT(610, 108, 80, 26),
    ITEM(PLANK, 586, 166),
    S(380, SLUDGER, FROM_ABOVE, 0, 760, 154),
    L(380, SHAMBLER, 140),
    R(470, CROW, 130),
    L(560, SHAMBLER, 120),
    L(560, SHAMBLER, 162),
    R(560, HOWLER, 130),
    R(560, HOWLER, 160),
    L(600, HOWLER, 140),
    R(600, RAMMER, 150),
    LOCK(600),
    PIT(900, 108, 60, 26),
    P(STUMP, 1000, 162, IT_NOTE, 0),
    P(STUMP, 1040, 126, IT_SANDWICH, 0),
    P(STUMP, 1080, 150, IT_BOTTLE, 0),
    THRESHER(820, 1660),
    R(860, SHAMBLER, 124),
    R(920, SHAMBLER, 156),
    R(980, SHAMBLER, 140),
    R(1040, SHAMBLER, 120),
    R(1100, SHAMBLER, 164),
    R(1160, SHAMBLER, 140),
    TUFT(1200, 168, IT_SAW),
    P(STUMP, 1300, 120, IT_NOTE, 0),
    P(STUMP, 1250, 160, IT_NOTE, 0),
    MINE(1400, 140),
    R(1350, SHAMBLER, 160),
    TUFT(1500, 168, IT_APPLE),
    TUFT(1560, 166, 0),
    TUFT(1700, 168, IT_SCATTER),
    DOOR(1840),
};
static const DkuEvt N3B[] = {
    DOG(150, 162),
    S(120, SHAMBLER, FROM_GROUND, 0, 330, 118),
    S(200, SHAMBLER, FROM_GROUND, 0, 420, 122),
    S(280, SHAMBLER, FROM_GROUND, 0, 520, 116),
    SAUCER(330),
    S(360, SHAMBLER, FROM_GROUND, 0, 600, 124),
    MINE(640, 160),
    L(500, CROW, 130),
    L(500, SHAMBLER, 150),
    R(500, SHAMBLER, 120),
    R(500, SHAMBLER, 162),
    L(520, RAMMER, 140),
    R(520, RAMMER, 140),
    LOCK(520),
    PIT(900, 150, 70, 22),
    S(760, SLUDGER, FROM_ABOVE, 0, 1000, 128),
    S(780, SLUDGER, FROM_ABOVE, 0, 1060, 132),
    R(800, SLUDGER, 140),
    R(800, SHAMBLER, 120),
    LOCK(820),
    S(1000, SHAMBLER, FROM_GROUND, 0, 1250, 120),
    S(1040, SHAMBLER, FROM_GROUND, 0, 1300, 150),
    BEAM(1420),
};
static const DkuEvt N3C[] = {
    BOSS(VISITOR, 220, 128),
};
static const DkuSection N3[] = {
    SEC("THE OLD ORCHARD", TH_ORCHARD, SEC_WALK, 1900, N3A),
    SEC("THE GRAVEYARD", TH_GRAVES, SEC_WALK, 1500, N3B),
    SEC("THE VISITORS' SHIP", TH_SHIP, SEC_BOSS, 320, N3C),
};

/* ===== NIGHT 4: the promenade, the fishing jetty ============================ */
static const DkuEvt N4A[] = {
    AT(0, SHAMBLER, DM_DANCE, 250, 130),
    AT(0, SHAMBLER, DM_DANCE, 280, 156),
    AT(0, BULWARK, DM_DANCE, 320, 140),
    S(120, SHAMBLER, FROM_DOOR, 0, 380, 0),
    S(120, SHAMBLER, FROM_DOOR, 0, 400, 0),
    L(260, SHAMBLER, 130),
    L(260, HOWLER, 160),
    LOCK(300),
    P(SIGN, 560, 118, IT_COIN, 0),
    R(500, BULWARK, 130),
    R(500, BULWARK, 160),
    L(500, SHAMBLER, 120),
    L(500, TORCH, 150),
    S(520, GIGGLER, FROM_DOOR, 0, 760, 0),
    S(520, GIGGLER, FROM_DOOR, 0, 780, 0),
    LOCK(540),
    P(BIN, 840, 150, IT_COIN, 0),
    P(JUNK, 940, 124, IT_APPLE, IT_COIN),
    P(JUNK, 980, 162, IT_BOTTLE, 0),
    DOG(1040, 150),
    S(800, TUSKER, FROM_DOOR, 0, 1010, 0),
    L(800, GIGGLER, 140),
    R(800, CROW, 130),
    LOCK(820),
    R(1000, CROW, 120),
    R(1000, CROW, 164),
    P(CRATE, 1300, 120, IT_ROAST, 0),
    P(CRATE, 1330, 145, IT_DRUMSTICK, 0),
    P(CRATE, 1360, 165, IT_NOTE, 0),
    P(CRATE, 1390, 120, IT_SANDWICH, 0),
    P(CRATE, 1420, 145, IT_NOTE, 0),
    P(CRATE, 1450, 165, IT_NOTE, 0),
    DOOR(1540),
};
static const DkuEvt N4B[] = {
    WATER(0, 100, 1500, 18),
    S(100, SKIPPER, FROM_WATER, 0, 300, 112),
    S(110, SKIPPER, FROM_WATER, 0, 340, 112),
    AT(0, SHAMBLER, DM_FEED, 420, 152),
    WATER(560, 152, 300, 30),
    R(420, SKIPPER, 136),
    R(430, SKIPPER, 128),
    R(440, SKIPPER, 144),
    L(430, HOWLER, 126),
    L(440, HOWLER, 140),
    L(450, HOWLER, 146),
    LOCK(460),
    AT(0, FEELER, 0, 940, 130),
    AT(0, FEELER, 0, 1010, 160),
    AT(0, FEELER, 0, 1080, 124),
    AT(0, FEELER, 0, 1150, 156),
    SAUCER(820),
    S(900, SKIPPER, FROM_WATER, 0, 1120, 112),
    S(980, SKIPPER, FROM_WATER, 0, 1200, 112),
    P(BIN, 1300, 160, IT_SANDWICH, 0),
    DOOR(1440),
};
static const DkuEvt N4C[] = {
    WATER(0, 100, 320, 18),
    WATER(268, 118, 60, 60),
    BOSS(UNDERTOW, 296, 128),
    STREAM(AK_SKIPPER, 18, 2),
};
static const DkuSection N4[] = {
    SEC("THE PROMENADE", TH_PROM, SEC_WALK, 1600, N4A),
    SEC("THE FISHING JETTY", TH_JETTY, SEC_WALK, 1500, N4B),
    SEC("THE END OF THE JETTY", TH_JETTY, SEC_BOSS, 320, N4C),
};

/* ===== NIGHT 5: the dunes, the Grand Hotel, the service lift, the penthouse ======= */
static const DkuEvt N5A[] = {
    WATER(0, 100, 1500, 14),
    AT(0, SHAMBLER, DM_FEED, 260, 140),
    AT(0, SHAMBLER, DM_FEED, 300, 160),
    AT(0, SHAMBLER, DM_FEED, 330, 126),
    S(120, SKIPPER, FROM_WATER, 0, 300, 110),
    S(120, SKIPPER, FROM_WATER, 0, 340, 110),
    S(120, SKIPPER, FROM_WATER, 0, 380, 110),
    L(140, HOWLER, 130),
    L(140, HOWLER, 160),
    L(150, CROW, 145),
    R(260, HOWLER, 140),
    LOCK(280),
    S(400, SHAMBLER, FROM_GROUND, 0, 640, 124),
    S(440, SHAMBLER, FROM_GROUND, 0, 690, 150),
    S(480, SHAMBLER, FROM_GROUND, 0, 740, 130),
    P(JUNK, 820, 162, IT_APPLE, 0),
    DOG(880, 150),
    AT(0, SLUDGER, DM_SIT, 1150, 160),
    AT(0, VISITOR, DM_SIT, 1180, 130),
    AT(0, VISITOR, DM_SIT, 1330, 120),
    AT(0, VISITOR, DM_SIT, 1370, 156),
    DOOR(1440),
};
static const DkuEvt N5B[] = {
    AT(0, SHAMBLER, DM_FEED, 260, 150),
    R(100, SHAMBLER, 130),
    R(200, SHAMBLER, 120),
    R(220, SHAMBLER, 162),
    LAMP(470, 132, 0),
    L(300, CROW, 130),
    L(300, CROW, 160),
    R(320, GIGGLER, 144),
    AT(0, SHAMBLER, DM_FEED, 640, 124),
    AT(0, SHAMBLER, DM_FEED, 680, 152),
    AT(0, SHAMBLER, DM_FEED, 720, 134),
    P(VASE, 800, 116, IT_SANDWICH, 0),
    R(560, SHAMBLER, 120),
    R(580, SHAMBLER, 162),
    LAMP(900, 150, 0),
    R(700, TORCH, 140),
    R(700, SHAMBLER, 120),
    R(700, SHAMBLER, 162),
    LOCK(720),
    L(880, SHAMBLER, 130),
    L(880, SHAMBLER, 160),
    R(980, TORCH, 125),
    R(1000, SHAMBLER, 150),
    R(1010, SHAMBLER, 164),
    P(VASE, 1150, 116, IT_APPLE, 0),
    P(VASE, 1190, 116, IT_DRUMSTICK, 0),
    LAMP(1260, 140, IT_SCATTER),
    R(1100, BULWARK, 140),
    R(1200, GIGGLER, 120),
    R(1220, GIGGLER, 160),
    L(1240, GIGGLER, 140),
    LOCK(1260),
    LIFTDOOR(1260, 1490),
    R(1380, RAMMER, 130),
    R(1380, TUSKER, 155),
    LOCK(1460),
};
static const DkuEvt N5C[] = {
    WAVE(1, SHAMBLER, 4),
    WAVE(2, BULWARK, 2),
    WAVE(3, SHAMBLER, 4),
    WAVE(4, GIGGLER, 2),
    WAVE(5, BULWARK, 2),
    WAVE(5, HOWLER, 2),
};
static const DkuEvt N5D[] = {
    BOSS(GRIST, 240, 140),
    P(BIN, 292, 162, IT_ROAST, 0),
    STREAM(0, 24, 4),
};
static const DkuSection N5[] = {
    SEC("THE DUNES", TH_DUNES, SEC_WALK, 1500, N5A),
    SEC("THE GRAND HOTEL", TH_HOTEL, SEC_WALK, 1800, N5B),
    SEC("THE SERVICE LIFT", TH_LIFT, SEC_LIFT, 320, N5C),
    SEC("THE PENTHOUSE", TH_PENTHOUSE, SEC_BOSS, 320, N5D),
};

const DkuNight DKU_NIGHT[DKU_NIGHTS] = {
    {"NIGHT 1", "MARKET STREET", N1, ARRAY_LEN(N1)},
    {"NIGHT 2", "CANAL ROW", N2, ARRAY_LEN(N2)},
    {"NIGHT 3", "THE OLD ORCHARD", N3, ARRAY_LEN(N3)},
    {"NIGHT 4", "THE PROMENADE", N4, ARRAY_LEN(N4)},
    {"NIGHT 5", "THE GRAND HOTEL", N5, ARRAY_LEN(N5)},
};

static const DkuEvt GYM_EV[] = {
    P(BIN, 30, 162, 0, 0),
};
const DkuSection DKU_GYM = {"THE HORNETS' GYM", TH_GYM, SEC_GYM, 320, GYM_EV, ARRAY_LEN(GYM_EV)};
