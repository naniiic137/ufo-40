/* TILTSHOT - the Comet Classic's field and scorecard: eight golfers (one or
 * two players, the rest CPU rivals), stroke play over the eighteen holes,
 * lowest total against par wins. The CPU rivals don't swing: like the
 * original's, they are a scoreboard, weak and random, and the best of them
 * finishes somewhere between 4 and 13 over par. */
#include "tiltshot.h"

const char *const TSH_GOLFER_NAME[TSH_GOLFERS] = {"NOVA", "DIGBY", "PEACHES", "TUCK", "MOSS", "WICK", "KIP"};

const char *const TSH_RIVAL_NAME[TSH_RIVALS] = {
    "ORBO", "GLIMMA", "GEARBOX", "MAVIS", "BIG NED", "ZIBBO",
    "LADY FEN", "SPROUT", "DR QUILL", "BLIX", "HONK", "MARGO",
};

void tsh_cpu_round(Rng *r, uint8_t out[][TSH_HOLES], int ncpu) {
    int target[TSH_FIELD];
    /* the leader's total, then each rival a little further back */
    target[0] = rng_range(r, 4, 13);
    for (int i = 1; i < ncpu; i++) target[i] = target[i - 1] + rng_range(r, 0, 4);
    for (int i = ncpu - 1; i > 0; i--) {
        int j = rng_range(r, 0, i), t = target[i];
        target[i] = target[j];
        target[j] = t;
    }
    int parsum = 0;
    for (int h = 0; h < TSH_HOLES; h++) parsum += TSH_HOLE[h].par;
    for (int i = 0; i < ncpu; i++) {
        int delta[TSH_HOLES] = {0};
        int need = target[i];
        /* a birdie or three along the way, paid back elsewhere */
        int birdies = rng_range(r, 0, 3);
        for (int b = 0; b < birdies; b++)
            for (int tries = 0; tries < 20; tries++) {
                int h = rng_range(r, 0, TSH_HOLES - 1);
                if (TSH_HOLE[h].par < 2 || delta[h]) continue;
                delta[h] = -1;
                need++;
                break;
            }
        while (need > 0) {
            /* longer holes go wrong more often */
            int pick = rng_range(r, 0, parsum - 1), h = 0;
            while (pick >= TSH_HOLE[h].par) pick -= TSH_HOLE[h++].par;
            if (delta[h] < 0 || delta[h] >= 4) continue;
            delta[h]++;
            need--;
        }
        for (int h = 0; h < TSH_HOLES; h++) out[i][h] = (uint8_t)(TSH_HOLE[h].par + delta[h]);
    }
}

void tsh_tour_new(TshTour *t, int humans, const int *golfer, const int *coat, uint64_t seed) {
    memset(t, 0, sizeof *t);
    rng_seed(&t->rng, seed);
    t->humans = iclamp(humans, 1, 2);
    for (int i = 0; i < t->humans; i++) {
        t->e[i].human = (int8_t)(i + 1);
        t->e[i].golfer = (uint8_t)golfer[i];
        t->e[i].coat = (uint8_t)coat[i];
    }
    /* rivals drawn from the tour's regulars */
    int pool[TSH_RIVALS];
    for (int i = 0; i < TSH_RIVALS; i++) pool[i] = i;
    for (int i = TSH_RIVALS - 1; i > 0; i--) {
        int j = rng_range(&t->rng, 0, i), x = pool[i];
        pool[i] = pool[j];
        pool[j] = x;
    }
    int ncpu = TSH_FIELD - t->humans;
    uint8_t rounds[TSH_FIELD][TSH_HOLES];
    tsh_cpu_round(&t->rng, rounds, ncpu);
    for (int i = 0; i < ncpu; i++) {
        TshEntrant *e = &t->e[t->humans + i];
        e->human = 0;
        e->golfer = (uint8_t)pool[i];
        memcpy(e->score, rounds[i], TSH_HOLES);
    }
}

int tsh_tour_strokes(const TshTour *t, int i, int holes) {
    int s = 0;
    for (int h = 0; h < holes && h < TSH_HOLES; h++) s += t->e[i].score[h];
    return s;
}

int tsh_tour_total(const TshTour *t, int i, int holes) {
    int s = 0;
    for (int h = 0; h < holes && h < TSH_HOLES; h++)
        if (t->e[i].score[h]) s += t->e[i].score[h] - TSH_HOLE[h].par;
    return s;
}

void tsh_tour_rank(const TshTour *t, int holes, int *order, int *place) {
    int tot[TSH_FIELD];
    for (int i = 0; i < TSH_FIELD; i++) {
        order[i] = i;
        tot[i] = tsh_tour_total(t, i, holes);
    }
    /* stable: on a tie the players are listed first */
    for (int i = 1; i < TSH_FIELD; i++)
        for (int j = i; j > 0 && tot[order[j]] < tot[order[j - 1]]; j--) {
            int x = order[j];
            order[j] = order[j - 1];
            order[j - 1] = x;
        }
    for (int k = 0; k < TSH_FIELD; k++) {
        int p = 1;
        for (int m = 0; m < TSH_FIELD; m++)
            if (tot[m] < tot[order[k]]) p++;
        place[k] = p;
    }
}

int tsh_tour_place(const TshTour *t, int i, int holes) {
    int mine = tsh_tour_total(t, i, holes), p = 1;
    for (int m = 0; m < TSH_FIELD; m++)
        if (tsh_tour_total(t, m, holes) < mine) p++;
    return p;
}

const char *tsh_score_word(int strokes, int par) {
    if (strokes == 1) return "HOLE IN ONE!";
    switch (strokes - par) {
    case -4: return "CONDOR!";
    case -3: return "ALBATROSS!";
    case -2: return "EAGLE!";
    case -1: return "BIRDIE!";
    case 0: return "PAR";
    case 1: return "BOGEY";
    case 2: return "DOUBLE BOGEY";
    case 3: return "TRIPLE BOGEY";
    default: return NULL;
    }
}
