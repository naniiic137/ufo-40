/* SHOWSTOPPER - the Hop: the bonus round the star passes pay for. Jelly, a
 * pink blob, hops from square to square on a board seen at an angle,
 * always onward, the way the pad last pointed (up: on; right and left:
 * across; down: back). Coins fill a meter, and each mark on it is a life.
 * Holes, spikes that come up and mouths that open take a try; each pass is
 * one try. The board is made as Jelly goes, harder the further on. */
#include "sst.h"

const int SST_HOP_MARK[HOP_MARKS] = {10, 25, 45, 70, 100, 135, 175, 220, 270};

#define H (sst_g.hop)
#define SPIKE_PERIOD 120
#define SPIKE_UP 40
#define MOUTH_PERIOD 150
#define MOUTH_OPEN 45

static void gen_row(void) {
    int g = H.gen_row++;
    int r = g % HOP_ROWS;
    int d = imin(10, g / 12);
    if (!H.practice && g >= HOP_LENGTH) {
        /* the far end: a row of plain squares, then nothing */
        for (int l = 0; l < HOP_LANES; l++) {
            H.tile[r][l] = (uint8_t)(g == HOP_LENGTH ? HT_TILE : HT_HOLE);
            H.coin[r][l] = 0;
            H.phase[r][l] = 0;
        }
        return;
    }
    /* one lane always safe, wandering a little, so there is a way on */
    static const int8_t wander[3] = {-1, 0, 1};
    int prev = g == 0 ? 3 : H.safe;
    int safe = iclamp(prev + wander[rng_range(&H.rng, 0, 2)], 1, HOP_LANES - 2);
    for (int l = 0; l < HOP_LANES; l++) {
        int t = HT_TILE;
        /* the safe lane, and where it came from in the last row, so the way
         * on never needs a diagonal hop */
        if (g >= 4 && l != safe && l != prev) {
            int roll = rng_range(&H.rng, 0, 99);
            if (roll < 8 + d * 3) t = HT_HOLE;
            else if (roll < 8 + d * 3 + 6 + d * 2) t = HT_SPIKE;
            else if (roll < 8 + d * 3 + 6 + d * 2 + 3 + d) t = HT_MOUTH;
        }
        H.tile[r][l] = (uint8_t)t;
        H.coin[r][l] = 0;
        if (t == HT_TILE && g >= 1) {
            int c = rng_range(&H.rng, 0, 99);
            H.coin[r][l] = c < 4 ? 5 : c < 34 ? 1 : 0;
        }
        H.phase[r][l] = (uint8_t)(rng_range(&H.rng, 0, 15) * 8);
    }
    H.safe = safe;
}

static int phase_of(int row, int lane) { return H.phase[row % HOP_ROWS][lane]; }

int sst_hop_tile(int row, int lane) {
    if (row < 0 || lane < 0 || lane >= HOP_LANES) return HT_HOLE;
    if (row >= H.gen_row || row < H.gen_row - HOP_ROWS) return HT_HOLE;
    return H.tile[row % HOP_ROWS][lane];
}

int sst_hop_coin(int row, int lane) {
    if (sst_hop_tile(row, lane) != HT_TILE) return 0;
    return H.coin[row % HOP_ROWS][lane];
}

/* is a spike up / a mouth open on that square now (or in a few frames) */
bool sst_hop_deadly(int row, int lane) {
    int t = sst_hop_tile(row, lane);
    if (t == HT_HOLE) return true;
    int f = sst_g.frame_t + phase_of(row, lane);
    if (t == HT_SPIKE) return f % SPIKE_PERIOD < SPIKE_UP;
    if (t == HT_MOUTH) return f % MOUTH_PERIOD < MOUTH_OPEN;
    return false;
}

static void fill_ahead(void) {
    while (H.gen_row < H.row + HOP_ROWS - 6) gen_row();
}

void sst_hop_start(int tries, bool practice) {
    memset(&H, 0, sizeof H);
    rng_seed(&H.rng, 4300u + (uint64_t)sst_g.stage * 77u + (uint64_t)sst_g.frame_t);
    H.tries = tries;
    H.practice = practice;
    H.lane = 3;
    H.row = 1;
    H.state = HB_READY;
    fill_ahead();
}

static void respawn(void) {
    /* back on a plain square of this row or the next */
    for (int dr = 0; dr < 4; dr++) {
        int r = imax(1, H.row + dr - 1);
        for (int k = 0; k < HOP_LANES; k++) {
            int l = 3 + ((k + 1) / 2) * (k % 2 ? 1 : -1);
            if (sst_hop_tile(r, l) == HT_TILE) {
                H.row = H.from_row = r;
                H.lane = H.from_lane = l;
                H.dir = H.next_dir = 0;
                H.state = HB_READY;
                H.st = 0;
                return;
            }
        }
    }
    H.lane = 3;
    H.state = HB_READY;
    H.st = 0;
}

static void lose_try(int how) {
    H.state = how;
    H.st = 0;
    sst_sfx(how == HB_FALL ? "sst_fall" : "sst_chomp");
}

void sst_hop_step(uint32_t held) {
    uint32_t press = held & ~sst_g.prev;
    sst_g.prev = held;
    sst_g.frame_t++;
    H.st++;
    if (press & BTN_UP) H.next_dir = 0;
    if (press & BTN_RIGHT) H.next_dir = 1;
    if (press & BTN_DOWN) H.next_dir = 2;
    if (press & BTN_LEFT) H.next_dir = 3;
    /* the view follows Jelly on */
    int want = imax(0, H.row - 3) * 16;
    if (H.cam < want) H.cam += imax(1, (want - H.cam) / 8);
    if (H.cam > want + 32) H.cam -= 2;
    switch (H.state) {
    case HB_READY:
        if (H.st >= 40) {
            H.state = HB_HOP;
            H.st = 0;
            H.hop_t = 0;
            H.dir = H.next_dir;
            H.from_lane = H.lane;
            H.from_row = H.row;
            H.lane += (H.dir == 1) - (H.dir == 3);
            H.row += (H.dir == 0) - (H.dir == 2);
        }
        break;
    case HB_HOP:
        if (++H.hop_t < HOP_HOP_T) break;
        /* landed */
        if (H.lane < 0 || H.lane >= HOP_LANES || sst_hop_tile(H.row, H.lane) == HT_HOLE) { lose_try(HB_FALL); break; }
        if (sst_hop_deadly(H.row, H.lane)) { lose_try(HB_BITTEN); break; }
        {
            int c = sst_hop_coin(H.row, H.lane);
            if (c) {
                H.coin[H.row % HOP_ROWS][H.lane] = 0;
                H.coins += c;
                sst_sfx(c > 1 ? "sst_bigcoin" : "sst_coin");
                while (H.next_mark < HOP_MARKS && H.coins >= SST_HOP_MARK[H.next_mark]) {
                    H.next_mark++;
                    if (!H.practice) {
                        H.lives_won++;
                        if (sst_g.lives < SST_MAX_LIVES) sst_g.lives++;
                        sst_sfx("sst_oneup");
                    }
                }
            }
        }
        fill_ahead();
        if (!H.practice && H.row >= HOP_LENGTH) {
            /* the far end: the round is over (any tries left are simply
             * over too: the passes were spent when it began) */
            H.made_it = 1;
            H.tries = 0;
            H.state = HB_DONE;
            H.st = 0;
            sst_sfx("sst_pass");
            break;
        }
        /* and on again */
        H.hop_t = 0;
        H.dir = H.next_dir;
        H.from_lane = H.lane;
        H.from_row = H.row;
        H.lane += (H.dir == 1) - (H.dir == 3);
        H.row = imax(0, H.row + (H.dir == 0) - (H.dir == 2));
        if (H.row == H.from_row && H.lane == H.from_lane) H.row++;
        break;
    case HB_FALL:
    case HB_BITTEN:
        if (H.st >= 50) {
            if (!H.practice) H.tries--; /* the free ride never runs out */
            if (H.tries <= 0) {
                H.state = HB_DONE;
                H.st = 0;
            } else {
                if (H.lane < 0 || H.lane >= HOP_LANES) H.lane = iclamp(H.lane, 0, HOP_LANES - 1);
                respawn();
            }
        }
        break;
    default: break;
    }
}
