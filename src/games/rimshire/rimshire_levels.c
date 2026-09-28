/* RIMSHIRE - the data: the sixteen disks, the eight skills, the ten
 * campaign wars (every board our own, drawn for UFO 40) and the maker of
 * the random boards for the streak. Numbers and where they come from are in
 * docs/games/41-rimshire.md. */
#include "rimshire.h"

/*  name        letter size      hp cost melee moves ranged rkind  charge pcharge fx      aqua anch drain triple */
const RshKind RSH_KIND[K_COUNT] = {
    {"SQUIRE",  'S', SZ_MID,    5, 3, 1, 2, 0, RG_NONE,   6, 0, FX_NONE,   0, 0, 0, 0, "QUICK AND STURDY, BUT HITS SOFTLY."},
    {"WARDEN",  'W', SZ_MID,    8, 6, 2, 2, 0, RG_NONE,   5, 0, FX_NONE,   0, 0, 0, 0, "A SQUIRE GROWN UP: HARD AND HARD-HITTING."},
    {"FERRET",  'F', SZ_SMALL,  3, 4, 1, 3, 0, RG_NONE,   6, 0, FX_NONE,   0, 0, 0, 0, "THREE DASHES A TURN. SHOVES BIG ONES ABOUT."},
    {"BRUTE",   'B', SZ_LARGE,  8, 5, 3, 1, 0, RG_NONE,   4, 0, FX_STUN,   0, 0, 0, 0, "ONE SLOW, HEAVY BLOW THAT STUNS."},
    {"SLINGER", 'L', SZ_MID,    4, 4, 1, 1, 3, RG_HIT,    4, 7, FX_NONE,   0, 0, 0, 0, "A STONE FOR A SHARD: 3 DAMAGE."},
    {"TOADKIN", 'T', SZ_MID,    5, 5, 2, 1, 2, RG_STUN,   6, 5, FX_NONE,   1, 0, 0, 0, "SWIMS. ITS SPIT DOES 2 AND STUNS."},
    {"OOZE",    'O', SZ_SMALL,  3, 3, 1, 2, 0, RG_NONE,   5, 0, FX_NONE,   1, 0, 0, 0, "A CHEAP LITTLE SWIMMER."},
    {"HEXER",   'H', SZ_MID,    5, 5, 1, 2, 1, RG_EMBERS, 6, 7, FX_NONE,   0, 0, 0, 0, "ITS SPELL LEAVES A RING OF SIX EMBERS."},
    {"MENHIR",  'M', SZ_MID,    8, 6, 2, 2, 0, RG_NONE,   4, 0, FX_NONE,   0, 1, 0, 0, "NOTHING KNOCKS IT ABOUT."},
    {"ADDER",   'A', SZ_SMALL,  3, 4, 0, 2, 0, RG_NONE,   6, 0, FX_POISON, 0, 0, 0, 0, "NO BITE, BUT ITS POISON KILLS ON THE NEXT HIT."},
    {"FRIAR",   'R', SZ_MID,    4, 5, 1, 2, 2, RG_HEAL,   3, 7, FX_NONE,   0, 0, 0, 0, "ITS BALM HEALS 2. LEECHES IT BURNS."},
    {"PIPER",   'P', SZ_MID,    6, 4, 1, 1, 1, RG_STAR,   6, 7, FX_NONE,   0, 0, 0, 0, "ITS TUNE: +1 ATTACK FOR THE BATTLE."},
    {"LEECH",   'E', SZ_MID,    6, 7, 2, 2, 0, RG_NONE,   5, 0, FX_NONE,   0, 0, 1, 0, "DRINKS 1 FROM EVERY FOE IT TOUCHES. HEALING HURTS."},
    {"DELVER",  'D', SZ_MID,    4, 3, 1, 2, 0, RG_NONE,   5, 0, FX_NONE,   0, 0, 0, 1, "PICKS UP THREE TIMES AS MUCH."},
    {"WYRM",    'Y', SZ_LARGE,  8, 8, 2, 2, 3, RG_HIT,    3, 7, FX_NONE,   0, 0, 0, 0, "BIG, SLOW AND FIERY. THE DEAREST OF ALL."},
    {"EMPRESS", 'Q', SZ_MID,   12, 0, 2, 2, 0, RG_NONE,   6, 0, FX_NONE,   0, 0, 0, 0, "THE PLUM EMPRESS HERSELF."},
};

const char *const RSH_SKILL_NAME[RSH_SKILLS] = {
    "COMMAND", "STOCKPILE", "SCOUTING", "HAGGLING", "PROSPECTING", "SIGHTLINE", "HOBNAILS", "REMEDY",
};
const char *const RSH_SKILL_TEXT[RSH_SKILLS] = {
    "CHOOSE FROM TWO MORE DISKS IN BATTLE.",
    "START EVERY BATTLE WITH 3 MORE SHARDS.",
    "ONE MORE SPACE ON THE BOARD EVERY TURN.",
    "4 COINS EVERY TIME YOU ARRIVE AT AN INN.",
    "YOUR SEAMS PAY TWICE AS MUCH.",
    "AN AIM LINE THAT SHOWS WHERE A HIT SENDS THINGS.",
    "SAND NO LONGER SLOWS YOUR DISKS.",
    "EVERY DISK STARTS A BATTLE WITH 1 MORE HP.",
};

int rsh_kind_of_letter(char c) {
    for (int k = 0; k < K_COUNT; k++)
        if (RSH_KIND[k].letter == c) return k;
    return -1;
}

int rsh_kind_value(int k) {
    if (k == K_EMPRESS) return 34;
    return RSH_KIND[k].cost * 2 + RSH_KIND[k].hp;
}

#define KB(k) (1u << (k))
#define POOL1 (KB(K_SQUIRE) | KB(K_WARDEN) | KB(K_FERRET) | KB(K_BRUTE))
#define POOL2 (POOL1 | KB(K_SLINGER))
#define POOL3 (POOL2 | KB(K_TOADKIN))
#define POOL4 (POOL3 | KB(K_OOZE) | KB(K_HEXER) | KB(K_MENHIR))
#define POOL5 (POOL4 | KB(K_ADDER))
#define POOL6 (POOL5 | KB(K_FRIAR))
#define POOL7 (POOL6 | KB(K_PIPER) | KB(K_LEECH))
#define POOL8 (POOL7 | KB(K_DELVER) | KB(K_WYRM))

/* Each war brings one new idea, as the original's ten do; the boards and
 * everything on them are ours. */
const RshScenario RSH_SCEN[RSH_SCENARIOS] = {
    {"THE LONG LANE", "ONE ROAD, TWO BANNERS. MEET IN THE MIDDLE.", 13, 3, '.',
     {"                         ",
      " . . : : . . s s . . : : ",
      "1-+-I-+-+-C-+-C-+-+-I-+-2",
      " . . s . : : . . s . . . ",
      "                         "},
     {4, 4}, {"SS", "SS"}, {"SSW", "SSW"}, POOL1, POOL1, 0, 1, 0, 0, 0x4101u},
    {"TWO ROADS", "OLD TOMES LIE ON THE OUTER ROADS.", 13, 5, '.',
     {"+-+-+-+-T-+-+-+-+-+-+-+-+",
      "|. . : : . .|. . s s . .|",
      "+           C           +",
      "|. . : . . .|. . . s . .|",
      "1-+-I-+-+   +   +-+-I-+-2",
      "|. . . s . .|. . : . . .|",
      "+           C           +",
      "|. . s s . .|. . : : . .|",
      "+-+-+-+-+-+-+-+-T-+-+-+-+"},
     {5, 5}, {"SS", "SS"}, {"SSW", "SSW"}, POOL2, KB(K_SLINGER), 0, 2, 0, 0, 0x4102u},
    {"MARSHMOOT", "A ROAD ROUND A MERE. MIND THE WATER.", 14, 6, '~',
     {"    +-+-+-+-T-+-+-+-+-+    ",
      " . .|~ ~ ~ ~ ~ ~ ~ ~ ~|. . ",
      "    +                 +    ",
      " . .|~ ~ ~ ~ ~ ~ ~ ~ ~|. . ",
      "1-+-I                 I-+-2",
      " . .|~ ~ ~ ~ ~ ~ ~ ~ ~|. . ",
      "    +-+-+-+-+-C-+-+-+-+    ",
      " . .|~ ~ ~ ~ ~ ~ ~ ~ ~|. . ",
      "    +                 +    ",
      " . .|s ~ ~ ~ ~ ~ ~ ~ s|. . ",
      "    +-+-+-+-+-+-T-+-+-+    "},
     {5, 5}, {"SS", "SS"}, {"SSW", "SSW"}, POOL3, KB(K_TOADKIN), 0, 3, 0, 0, 0x4103u},
    {"COIN FEVER", "SIX SEAMS. WHOEVER HOLDS THEM, PAYS.", 14, 5, '.',
     {"  +-+-$-+-+-+-+-+-+-$-+-+  ",
      " .|: : s .|. : :|. . s :|: ",
      "  +       +     +       +  ",
      " .|. : s .|: ~ ~|: . s :|. ",
      "1-+-I-+-+-$-+-+-$-+-+-I-+-2",
      " .|: : s .|: ~ ~|: . s :|: ",
      "  +       +     +       +  ",
      " .|. : : s|s . .|s s : :|. ",
      "  +-+-$-+-+-+-+-+-+-$-+-+  "},
     {4, 4}, {"SS", "SS"}, {"SSW", "SSW"}, POOL4, KB(K_OOZE) | KB(K_HEXER) | KB(K_MENHIR), 0, 4, 0, 0, 0x4104u},
    {"ADDER NEST", "YOU MARCH WITH A WARDEN AND TWO ADDERS.", 14, 6, '.',
     {"    +-+-+-C-+-+-+-+-T-+    ",
      " . .|s . . . .|. : : .|. . ",
      "    +         +       +    ",
      " . ~|~ . . s .|. : : .|. . ",
      "1-+-I-+-+     +     +-+-+-+",
      " . .|s . . ~ ~|. . s|. . .|",
      "+-+-+-+       +     +-I-+-2",
      " . .|. : : . .|~ ~ . .|s . ",
      "    +         +       +    ",
      " . .|s . . . .|: : . .|. . ",
      "    +-T-+-+-+-+-C-+-+-+    "},
     {5, 5}, {"WAA", "SS"}, {"SSW", "SSW"}, POOL5, KB(K_ADDER), 0, 5, 0, 0, 0x4105u},
    {"THE KEEP", "BOTH HOMES ARE CASTLES NOW: LOSE THERE, LOSE ALL.", 14, 6, ':',
     {"+-+-+-T-+-+-+-+-+-+-C-+-+-+",
      "|. . : :|. . . . :|: . . .|",
      "+       +         +       +",
      "|. : . .|. s s . .|. : . .|",
      "+   +-+-+-$-+-+-$-+-+-+   +",
      "|. .|: .|~ ~ ~ ~ .|: .|. .|",
      "1-+-I   +         +   I-+-2",
      "|. .|: .|~ ~ ~ ~ .|: .|. .|",
      "+   +-+-+-C-+-+-C-+-+-+   +",
      "|. : . .|. s s . .|. : : .|",
      "+-+-+-C-+-+-+-+-+-+-T-+-+-+"},
     {5, 5}, {"SS", "SS"}, {"SSW", "SSW"}, POOL6, KB(K_FRIAR), 3, 6, 0, 0, 0x4106u},
    {"PIPERS' MARCH", "YOU MARCH WITH A WARDEN AND TWO PIPERS.", 15, 6, '.',
     {"+-+-+-+-T-+-+-+-+-+-C-+-+-+-+",
      "|. . . s s .|. . . :|: . . .|",
      "+           +       +       +",
      "|. . s . . .|. . . ~|~ . . .|",
      "1-+-I-+-$-+-+       +-+-$-+-I",
      "|. . . :|: .|~ ~ . .|s . . .|",
      "+       +-+-+-+-C-+-+-+     +",
      "|. . ~ ~ . .|: : . .|. . s .|",
      "+           +       +       2",
      "|. . . : : .|. . . s|s . . .|",
      "+-C-+-+-+-+-+-+-T-+-+-+-+-+-+"},
     {5, 5}, {"WPP", "SS"}, {"SSW", "SSW"}, POOL7, KB(K_PIPER) | KB(K_LEECH), 3, 7, 0, 0, 0x4107u},
    {"DEEP DIGGINGS", "THREE DELVERS EACH, NO RESERVES, NO COIN ON THE BOARD.", 15, 6, ':',
     {"  +-+-+-+-+-I-+-+-+-+-+-+-+  ",
      " :|: s s : :|. . : : s s :|: ",
      "  +         +             +  ",
      " :|s : : . .|: : . . : : s|: ",
      "1-+-+-I-+-+-+     +-+-+-I-+-2",
      " :|: . . s s : : s|s . . :|: ",
      "  +           +-+-+       +  ",
      " :|s : : . . :|: . . : : s|: ",
      "  +           +           +  ",
      " :|: s s : : .|. : : s s :|: ",
      "  +-+-+-+-+-+-+-I-+-+-+-+-+  "},
     {0, 0}, {"DDD", "DDD"}, {"", ""}, POOL8, KB(K_DELVER) | KB(K_WYRM), 3, 8, 0, 4, 0x4108u},
    {"THREE BRIDGES", "TWO SHORES AND THREE WAYS ACROSS.", 15, 7, '~',
     {"+-+-+-+-T-+-+   +-+-C-+-+-+-+",
      "|. . : : . .|~ ~|. . : : . .|",
      "+           +-+-+           +",
      "|. . s . . .|~ ~ . . . s . .|",
      "+   I-+-+   +       +-$-+   +",
      "|. .|. .|. .|~ ~ . .|. .|. .|",
      "1-+-+-+-$-+-+-+-+-+-+-+-+-I-2",
      "|. :|. .|. .|~ ~ . .|. .|: .|",
      "+   +-C-+   +       +-+-+   +",
      "|. . s . . .|~ ~ . . . s . .|",
      "+           +-+-+           +",
      "|. . : : . .|~ ~|. . : : . .|",
      "+-+-+-+-+-+-T   +-+-+-+-T-+-+"},
     {5, 5}, {"SS", "SS"}, {"SSW", "SSW"}, POOL8, 0, 3, 9, 0, 0, 0x4109u},
    {"THE EMPRESS", "THE PLUM EMPRESS WAITS IN HER RESERVES.", 16, 7, '.',
     {"+-+-+-+-T-+-+-+-+-+-+-C-+-+-+-+",
      "|. . : :|. . .|. s s|. . .|. :|",
      "+       +     +     +     +   +",
      "|. . s .|. ~ ~|. . .|. : :|. .|",
      "+-$-+-+-+-+   +     +   +-+-$-+",
      "|. :|: . .|~ ~|. . s|s . .|~|.|",
      "1-+-I     +-+-C-C-+-+     +-I-2",
      "|. .|s s .|. :|: . .|~ ~ .|.|s|",
      "+-$-+-+-+-+   +     +   +-+-$-+",
      "|. . : :|. . s|s . .|~ ~ .|. :|",
      "+       +     +     +     +   +",
      "|. . s .|. . .|: : .|. . .|~ .|",
      "+-+-+-+-C-+-+-+-+-+-+-T-+-+-+-+"},
     {6, 6}, {"SS", "SS"}, {"SSW", "SSWQ"}, POOL8, KB(K_EMPRESS), 3, 9, 0, 0, 0x410Au},
};

/* ------------------------------------------------------------------ */
/* the streak's boards: a random maze of roads with loops, homes at the
 * two ends, and the same kinds of places as the campaign's */

static bool st_node(const RshWar *w, int x, int y) { return x >= 0 && y >= 0 && x < w->w && y < w->h && w->node[y][x] != N_NONE; }

static void carve(RshWar *w, int x0, int y0, int x1, int y1) {
    if (y0 == y1) w->er[y0][imin(x0, x1)] = 1;
    else w->ed[imin(y0, y1)][x0] = 1;
}

static int free_node(RshWar *w, Rng *r, int x0, int x1) {
    for (int tries = 0; tries < 200; tries++) {
        int x = rng_range(r, x0, x1), y = rng_range(r, 0, w->h - 1);
        if (w->node[y][x] == N_PLAIN) return y * RSH_MW + x;
    }
    return -1;
}

static void put(RshWar *w, Rng *r, int kind, int n, int x0, int x1) {
    for (int i = 0; i < n; i++) {
        int c = free_node(w, r, iclamp(x0, 0, w->w - 1), iclamp(x1, 0, w->w - 1));
        if (c >= 0) w->node[c / RSH_MW][c % RSH_MW] = (uint8_t)kind;
    }
}

void rsh_make_streak_map(RshWar *w, Rng *r) {
    static uint8_t seen[RSH_MH][RSH_MW];
    for (int attempt = 0; attempt < 40; attempt++) {
        memset(w->node, 0, sizeof w->node);
        memset(w->er, 0, sizeof w->er);
        memset(w->ed, 0, sizeof w->ed);
        w->w = rng_range(r, 12, 15);
        w->h = rng_range(r, 5, 7);
        w->border = '.';
        int keep = attempt < 30 ? 82 : 100;
        for (int y = 0; y < w->h; y++)
            for (int x = 0; x < w->w; x++) w->node[y][x] = rng_chance(r, keep) ? N_PLAIN : N_NONE;
        int b0y = w->h / 2 + rng_range(r, -1, 1), b1y = w->h / 2 + rng_range(r, -1, 1);
        w->node[b0y][0] = N_PLAIN;
        w->node[b1y][w->w - 1] = N_PLAIN;
        /* a random maze from the Brass home */
        memset(seen, 0, sizeof seen);
        int sx[RSH_MW * RSH_MH], sy[RSH_MW * RSH_MH], sp = 0;
        sx[sp] = 0;
        sy[sp++] = b0y;
        seen[b0y][0] = 1;
        while (sp > 0) {
            int x = sx[sp - 1], y = sy[sp - 1], opts[4], no = 0;
            for (int d = 0; d < 4; d++) {
                int nx = x + RSH_DX[d], ny = y + RSH_DY[d];
                if (st_node(w, nx, ny) && !seen[ny][nx]) opts[no++] = d;
            }
            if (!no) { sp--; continue; }
            int d = opts[rng_range(r, 0, no - 1)], nx = x + RSH_DX[d], ny = y + RSH_DY[d];
            carve(w, x, y, nx, ny);
            seen[ny][nx] = 1;
            sx[sp] = nx;
            sy[sp++] = ny;
        }
        if (!seen[b1y][w->w - 1]) continue;
        for (int y = 0; y < w->h; y++)
            for (int x = 0; x < w->w; x++)
                if (!seen[y][x]) w->node[y][x] = N_NONE;
        /* loops, so routes can cut each other off */
        for (int y = 0; y < w->h; y++)
            for (int x = 0; x < w->w; x++) {
                if (!st_node(w, x, y)) continue;
                if (st_node(w, x + 1, y) && !w->er[y][x] && rng_chance(r, 24)) w->er[y][x] = 1;
                if (st_node(w, x, y + 1) && !w->ed[y][x] && rng_chance(r, 24)) w->ed[y][x] = 1;
            }
        w->node[b0y][0] = N_BASE0;
        w->node[b1y][w->w - 1] = N_BASE1;
        int third = w->w / 3;
        put(w, r, N_INN, 1, 1, third);
        put(w, r, N_INN, 1, w->w - 1 - third, w->w - 2);
        put(w, r, N_INN, 1, third + 1, w->w - 2 - third);
        put(w, r, N_TOME, 2, third, w->w - 1 - third);
        put(w, r, N_SEAM, 3, 1, w->w - 2);
        put(w, r, N_CHEST, 3, 1, w->w - 2);
        break;
    }
    /* the land: mostly grass, with patches of stone, sand and water */
    for (int y = 0; y <= RSH_MH; y++)
        for (int x = 0; x <= RSH_MW; x++) w->tile[y][x] = T_GRASS;
    int blobs = rng_range(r, 5, 8);
    for (int b = 0; b < blobs; b++) {
        int t = rng_range(r, 1, 3), cx = rng_range(r, 0, w->w), cy = rng_range(r, 0, w->h), rad = rng_range(r, 1, 2);
        for (int y = cy - rad; y <= cy + rad; y++)
            for (int x = cx - rad; x <= cx + rad; x++)
                if (x >= 0 && y >= 0 && x <= w->w && y <= w->h && iabs(x - cx) + iabs(y - cy) <= rad) w->tile[y][x] = (uint8_t)t;
    }
}
