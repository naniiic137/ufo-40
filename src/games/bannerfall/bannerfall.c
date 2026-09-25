/* BANNERFALL - a real-time lane battle between two keeps.
 * Cartridge 09 of UFO 40, a tribute to Attactics (UFO 50 #9).
 * See docs/games/09-bannerfall.md. Rules live in bannerfall_logic.c;
 * this file is presentation, menus and flow. */
#include "bannerfall.h"

#define BX 48            /* board origin */
#define BY 26
#define TW 28            /* tile size */
#define TH 22
#define COUNT_FRAMES 30  /* one timer count = half a second */
#define SPAWN_FRAMES 14

enum { S_TITLE, S_CAMPAIGN, S_BRIEF, S_PLAY, S_RESULT, S_RANKED, S_SURVIVAL, S_VERSUS, S_ENDING };
enum { P_SPAWN, P_PLAN, P_RESOLVE, P_OVER };
enum { VS_BEGINNER, VS_MODERATE, VS_ADVANCED, VS_CUSTOM, VS_RANDOM, VS_PRESETS };

typedef struct Save {
    uint32_t magic;
    uint32_t beaten;        /* one bit per campaign battle */
    uint16_t losses;        /* campaign battles lost */
    uint16_t rank, best_rank;
    uint16_t pad;
    uint32_t best_survival;
    uint8_t level_cursor, seen_ending, pad2[2];
} Save;
#define SAVE_MAGIC 0x42460001u

typedef struct Hand {
    int cx, cy;
    bool grab;
    int pickup;
    int bump;
} Hand;

typedef struct Part {
    float x, y, vx, vy;
    int life, max, col, kind, val;
} Part;

static Save sv;
static Board B;
static Board snaps[PH_COUNT + 1];
static BEvents rev, sev;
static Hand hand[2];
static Part parts[180];
static Rng seeds;

static int state, state_t, frame_t;
static int sub, sub_t, plan_t;
static int res_phase, res_t, res_len[PH_COUNT];
static int title_sel, level_sel, cur_level;
static int shake, banner_t, banner_kind, tip_t;
static bool two_players, sheet_mode;
static int result_status, rank_delta;
static bool new_best;
static uint32_t result_score;
static uint16_t spawn_mask[2], spawn_total[2]; /* unit types spawned this battle (tests) */

/* versus setup */
/* versus setup: each army's pool is 8 places in 2 rows of 4 (player 2's is
 * the mirror image), plus its banners. cur: -1 the mode banner (player 1
 * only), 0..7 a place, 8 the banners row. */
#define VS_SLOTS 8
static int vs_preset, vs_cur[2];
static uint8_t vs_slot[2][VS_SLOTS], vs_rand[2][VS_SLOTS], vs_flags[2];

/* the end of a turn plays out in about a second: knives, attacks, moves, keeps, riders */
static const uint8_t PHASE_LEN[PH_COUNT] = {12, 14, 12, 14, 10};

/* one title per ten ranks; below 10 there is none */
static const char *RANK_TITLES[21] = {
    "", "PICKET", "TROOPER", "VETERAN", "SERGEANT", "BANNERMAN", "CAPTAIN",
    "KNIGHT", "MARSHAL", "COMMANDER", "WARLORD", "THANE", "MARGRAVE", "HIGH MARSHAL",
    "LORD OF LANES", "KEEPBREAKER", "FLAGTAKER", "IRON DUKE", "GRAND DUKE", "LEGEND", "MYTH",
};
static const char *rank_title(int r) { return RANK_TITLES[iclamp(r / 10, 0, 20)]; }

static bool vita_single(void) { return plat_kind() == PLAT_VITA; }

/* ------------------------------------------------------------------ */
/* particles                                                            */

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind, int val) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) {
            parts[i] = (Part){x, y, vx, vy, life, life, col, kind, val};
            return;
        }
}

static void dust(float x, float y, int n, int col) {
    for (int i = 0; i < n; i++) {
        float a = (float)i / (float)n * 6.283f;
        part_add(x, y, cosf(a) * 0.9f, sinf(a) * 0.5f - 0.6f, 14 + i % 5, col, 0, 0);
    }
}

/* ------------------------------------------------------------------ */
/* save & goals                                                         */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void load_save(void) {
    Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    else { memset(&sv, 0, sizeof sv); sv.magic = SAVE_MAGIC; }
    /* ranked starts at rank 10 (older saves that never won a ranked battle too) */
    if (sv.rank == 0 && sv.best_rank == 0) sv.rank = sv.best_rank = 10;
}

static int beaten_count(void) {
    int n = 0;
    for (int i = 0; i < BF_LEVELS; i++) n += (sv.beaten >> i) & 1;
    return n;
}
static bool level_open(int i) { return i == 0 || ((sv.beaten >> (i - 1)) & 1) || ((sv.beaten >> i) & 1); }
static bool campaign_done(void) { return (sv.beaten & 0xFFFFFFu) == 0xFFFFFFu; }

static void check_goals(void) {
    if ((sv.beaten & 0xFFFu) == 0xFFFu) game_award(GOAL_BEACON); /* the first 12 battles */
    if (campaign_done()) game_award(GOAL_SAUCER);                  /* all 24 */
    if (campaign_done() && sv.best_rank >= 100) game_award(GOAL_ALIEN);
}

/* ------------------------------------------------------------------ */
/* battles                                                              */

static uint64_t new_seed(void) { return rng_next(&seeds) ^ ((uint64_t)rng_next(&seeds) << 32); }

static void reset_hands(void) {
    hand[0] = (Hand){1, 2, false, 0, 0};
    hand[1] = (Hand){6, 3, false, 0, 0};
}

static void begin_turn(void) {
    sev.n = 0;
    bool hero = B.a[0].hero_due || B.a[1].hero_due; /* five promotions called a champion */
    bf_spawn(&B, &sev);
    for (int i = 0; i < sev.n; i++)
        if (sev.ev[i].type == BE_SPAWN) {
            spawn_mask[sev.ev[i].a & 1] |= (uint16_t)(1u << sev.ev[i].b);
            spawn_total[sev.ev[i].a & 1]++;
        }
    sub = P_SPAWN;
    sub_t = 0;
    if (B.turn == 40 || B.turn == 60) {
        banner_kind = B.turn == 40 ? 1 : 2;
        banner_t = 90;
        sfx_play_name("bf_double");
    }
    if (hero && banner_t <= 0) {
        banner_kind = 3;
        banner_t = 70;
    }
    sfx_play_name(hero ? "bf_hero" : "bf_spawn");
}

static void start_battle(void) {
    memset(parts, 0, sizeof parts);
    reset_hands();
    rev.n = 0;
    shake = 0;
    banner_t = 0;
    tip_t = B.mode == MODE_CAMPAIGN && BF_LEVELS_DEF[cur_level].tip ? 240 : 0;
    state = S_PLAY;
    state_t = 0;
    input_set_versus(two_players);
    game_set_pausable(true);
    music_play(BF_MUS_BATTLE);
    spawn_mask[0] = spawn_mask[1] = spawn_total[0] = spawn_total[1] = 0;
    begin_turn();
}

static void start_campaign_battle_seeded(int level, uint64_t seed) {
    cur_level = level;
    two_players = false;
    bf_setup_level(&B, level, seed);
    start_battle();
}

static void start_campaign_battle(int level) { start_campaign_battle_seeded(level, new_seed()); }

static void start_ranked(void) {
    two_players = false;
    bf_setup_ranked(&B, sv.rank, new_seed());
    start_battle();
}

static void start_survival(void) {
    two_players = false;
    bf_setup_survival(&B, new_seed());
    start_battle();
}

static const char *VS_POOLS[3] = {"FFBR", "FFBRPW", "FFBRPWSX"};

/* a place's mirror image across the field: same row, other end */
static int vs_mirror(int i) { return (i / 4) * 4 + 3 - i % 4; }

/* the presets: both armies field the same pool, player 2's mirrored */
static void vs_preset_slots(int p, uint8_t out[2][VS_SLOTS]) {
    memset(out, 0, 2 * VS_SLOTS);
    Army a;
    memset(&a, 0, sizeof a);
    bf_pool_from_letters(&a, VS_POOLS[iclamp(p, 0, 2)]);
    for (int i = 0; i < a.pool_n && i < VS_SLOTS; i++) {
        out[0][i] = a.pool[i];
        out[1][vs_mirror(i)] = a.pool[i];
    }
}

/* the random setting: a different pool for each side, never a champion */
static void vs_reroll(void) {
    memset(vs_rand, 0, sizeof vs_rand);
    for (int s2 = 0; s2 < 2; s2++) {
        int n = rng_range(&seeds, 4, VS_SLOTS);
        for (int i = 0; i < n; i++) vs_rand[s2][s2 ? vs_mirror(i) : i] = (uint8_t)rng_range(&seeds, U_FOOT, U_POWDER);
    }
}

static void army_from_slots(Army *a, const uint8_t *slot) {
    a->pool_n = 0;
    for (int i = 0; i < VS_SLOTS; i++)
        if (slot[i]) a->pool[a->pool_n++] = slot[i];
    if (a->pool_n == 0) a->pool[a->pool_n++] = U_FOOT;
}

static bool vs_mirrored(uint8_t slot[2][VS_SLOTS]) {
    for (int i = 0; i < VS_SLOTS; i++)
        if (slot[1][vs_mirror(i)] != slot[0][i]) return false;
    return true;
}

static void start_versus(void) {
    two_players = true;
    bf_setup(&B, MODE_VERSUS, new_seed());
    uint8_t slot[2][VS_SLOTS];
    if (vs_preset == VS_RANDOM) memcpy(slot, vs_rand, sizeof slot);
    else if (vs_preset == VS_CUSTOM) memcpy(slot, vs_slot, sizeof slot);
    else vs_preset_slots(vs_preset, slot);
    for (int s2 = 0; s2 < 2; s2++) {
        army_from_slots(&B.a[s2], slot[s2]);
        B.a[s2].flags = B.a[s2].start_flags = vs_preset == VS_CUSTOM ? vs_flags[s2] : 3;
        B.a[s2].heroes = 1; /* champions from promotions in every 2P battle */
    }
    /* matching spawns only when the pools mirror place for place; never in random */
    B.matched = vs_preset != VS_RANDOM && vs_mirrored(slot);
    start_battle();
}

static void finish_battle(void) {
    result_status = B.status;
    result_score = B.score;
    rank_delta = 0;
    new_best = false;
    input_set_versus(false);
    switch (B.mode) {
    case MODE_CAMPAIGN:
        if (B.status == BF_WIN_L) {
            sv.beaten |= 1u << cur_level;
            if (cur_level + 1 < BF_LEVELS) sv.level_cursor = (uint8_t)(cur_level + 1);
        } else if (B.status == BF_WIN_R) {
            if (sv.losses < 65535) sv.losses++;
        }
        break;
    case MODE_RANKED: {
        int fl = B.a[SIDE_L].flags, fr = B.a[SIDE_R].flags;
        rank_delta = bf_rank_change(fl, fr, B.status);
        int r = imax(0, (int)sv.rank + rank_delta);
        rank_delta = r - (int)sv.rank;
        sv.rank = (uint16_t)r;
        if (sv.rank > sv.best_rank) sv.best_rank = sv.rank;
        break;
    }
    case MODE_SURVIVAL:
        if (B.score > sv.best_survival) { sv.best_survival = B.score; new_best = true; }
        break;
    default: break;
    }
    save_now();
    check_goals();
    state = S_RESULT;
    state_t = 0;
    game_set_pausable(false);
    bool good = B.mode == MODE_VERSUS || B.mode == MODE_SURVIVAL ? true : B.status == BF_WIN_L;
    if (B.mode == MODE_SURVIVAL) good = new_best;
    music_restart(good ? BF_MUS_WIN : BF_MUS_LOSE);
}

/* ------------------------------------------------------------------ */
/* the resolution replay                                                */

static int tile_x(int x) { return BX + x * TW; }
static int tile_y(int y) { return BY + y * TH; }

static void begin_resolve(void) {
    for (int p = 0; p < 2; p++) hand[p].grab = false;
    rev.n = 0;
    bf_resolve(&B, &rev, snaps);
    for (int p = 0; p < PH_COUNT; p++) {
        res_len[p] = 0;
        for (int i = 0; i < rev.n; i++)
            if (rev.ev[i].phase == p) res_len[p] = PHASE_LEN[p];
    }
    sub = P_RESOLVE;
    res_phase = 0;
    res_t = 0;
    sfx_play_name("bf_go");
}

/* sounds and particles when a phase starts, hits and ends */
static void phase_fx(int p, int moment) {
    bool played_hit = false, played_die = false;
    for (int i = 0; i < rev.n; i++) {
        BEvent *e = &rev.ev[i];
        if (e->phase != p) continue;
        float cx = tile_x(e->x) + TW / 2.0f, cy = tile_y(e->y) + TH / 2.0f;
        if (moment == 0) {
            if (e->type == BE_ARROW) sfx_play_name("bf_arrow");
            if (e->type == BE_KNIFE) sfx_play_name("bf_knife");
            if (e->type == BE_MOVE && p != PH_KEEP) sfx_play_name("bf_march");
        } else if (moment == 1) {
            switch (e->type) {
            case BE_MELEE: case BE_REACH: case BE_CLASH: sfx_play_name("bf_clang"); break;
            case BE_DAMAGE:
                if (e->a > 0) {
                    part_add(cx, cy - 8, 0, -0.5f, 36, e->a >= 3 ? C_ORANGE : C_WHITE, 3, e->a);
                    for (int k = 0; k < 4; k++) part_add(cx, cy, (k - 1.5f) * 0.6f, -1.0f, 10, C_WHITE, 1, 0);
                    if (!played_hit) { sfx_play_name("bf_hit"); played_hit = true; }
                } else {
                    part_add(cx, cy - 8, 0, -0.4f, 30, C_GREY, 4, 0); /* blocked */
                }
                break;
            case BE_PROMOTE:
                sfx_play_name("bf_promote");
                for (int k = 0; k < 8; k++) {
                    float a = (float)k / 8.0f * 6.283f;
                    part_add(cx, cy - 4, cosf(a) * 1.2f, sinf(a) * 1.2f, 18, C_YELLOW, 1, 0);
                }
                break;
            case BE_KEEP: {
                int kx = e->a == SIDE_L ? 296 : 24;
                sfx_play_name("bf_keep");
                shake = imax(shake, 12);
                for (int k = 0; k < 14; k++) part_add((float)kx, cy, (k - 7) * 0.35f, -1.6f - (k % 3) * 0.4f, 26, k % 2 ? C_LIGHT : C_GREY, 0, 0);
                break;
            }
            default: break;
            }
        } else {
            switch (e->type) {
            case BE_DEATH:
                if (!played_die) { sfx_play_name("bf_die"); played_die = true; }
                dust(cx, cy + 4, 10, e->a == SIDE_L ? C_AMBER : C_VIOLET);
                dust(cx, cy + 4, 6, C_LIGHT);
                break;
            case BE_BLAST:
                sfx_play_name("bf_blast");
                shake = imax(shake, 14);
                part_add(cx + (e->x2 - e->x) * TW / 2.0f, cy, 0, 0, 16, C_YELLOW, 2, 0);
                for (int k = 0; k < 20; k++) {
                    float a = (float)k / 20.0f * 6.283f;
                    part_add(cx + (e->x2 - e->x) * TW / 2.0f, cy, cosf(a) * 2.2f, sinf(a) * 1.6f - 0.4f, 16 + k % 6, k % 3 ? C_ORANGE : C_YELLOW, 0, 0);
                }
                break;
            case BE_REAPPEAR:
                for (int k = 0; k < 8; k++) part_add(cx, cy, (k - 3.5f) * 0.3f, -1.2f, 16, C_CREAM, 1, 0);
                break;
            default: break;
            }
        }
    }
}

static void update_resolve(int speed) {
    for (int s = 0; s < speed; s++) {
        while (res_phase < PH_COUNT && res_len[res_phase] == 0) res_phase++;
        if (res_phase >= PH_COUNT) break;
        if (res_t == 0) phase_fx(res_phase, 0);
        if (res_t == res_len[res_phase] / 2) phase_fx(res_phase, 1);
        res_t++;
        if (res_t >= res_len[res_phase]) {
            phase_fx(res_phase, 2);
            res_phase++;
            res_t = 0;
        }
    }
    while (res_phase < PH_COUNT && res_len[res_phase] == 0) res_phase++;
    if (res_phase >= PH_COUNT) {
        if (B.status != BF_PLAYING) {
            sub = P_OVER;
            sub_t = 0;
        } else {
            begin_turn();
        }
    }
}

/* ------------------------------------------------------------------ */
/* the player's hands                                                   */

static void hand_update(int p) {
    Hand *h = &hand[p];
    bool (*held)(int) = p ? btn2 : btn;
    bool (*press)(int) = p ? btnp2 : btnp;
    bool (*rep)(int) = p ? btn_repeat2 : btn_repeat;
    int dx = rep(BTN_RIGHT) - rep(BTN_LEFT), dy = rep(BTN_DOWN) - rep(BTN_UP);
    if (h->bump > 0) h->bump--;
    if (h->grab) {
        if (!held(BTN_A) || sub != P_PLAN) {
            h->grab = false;
            sfx_play_name("bf_drop");
            return;
        }
        if (dx || dy) {
            int x = h->cx, y = h->cy;
            bool ok = dx ? bf_drag(&B, p, &x, &y, dx, 0, h->pickup) : bf_drag(&B, p, &x, &y, 0, dy, h->pickup);
            if (ok) { h->cx = x; h->cy = y; sfx_play_name("bf_drag"); }
            else { h->bump = 8; sfx_play_name("bf_bump"); }
        }
        return;
    }
    if (dx || dy) {
        /* the hand roams its own half of the field only */
        int lo = p ? BF_COLS - BF_ZONE : 0, hi = p ? BF_COLS - 1 : BF_ZONE - 1;
        int nx = iclamp(h->cx + dx, lo, hi), ny = iclamp(h->cy + dy, 0, BF_ROWS - 1);
        if (nx != h->cx || ny != h->cy) sfx_play_name("bf_move");
        h->cx = nx;
        h->cy = ny;
    }
    if (press(BTN_A) && sub == P_PLAN) {
        Unit *u = &B.g[h->cy][h->cx];
        if (u->type && u->side == p && bf_in_zone(p, h->cx)) {
            h->grab = true;
            h->pickup = h->cx;
            sfx_play_name("bf_grab");
        } else {
            h->bump = 8;
            sfx_play_name("bf_bump");
        }
    }
}

static void update_play(void) {
    bool ff = !two_players && btn(BTN_B);
    int speed = ff ? 3 : 1;
    if (banner_t > 0) banner_t--;
    if (tip_t > 0) tip_t--;
    hand_update(0);
    if (two_players) hand_update(1);
    switch (sub) {
    case P_SPAWN:
        sub_t += speed;
        if (sub_t >= SPAWN_FRAMES) { sub = P_PLAN; plan_t = 0; }
        break;
    case P_PLAN: {
        int total = bf_timer_counts(&B) * COUNT_FRAMES;
        int before = (total - plan_t + COUNT_FRAMES - 1) / COUNT_FRAMES;
        plan_t += ff ? 4 : 1;
        int after = (total - plan_t + COUNT_FRAMES - 1) / COUNT_FRAMES;
        if (after != before && after >= 1 && after <= 3) sfx_play_name("bf_tick");
        if (plan_t >= total) begin_resolve();
        break;
    }
    case P_RESOLVE: update_resolve(speed); break;
    case P_OVER:
        sub_t++;
        if (sub_t > 70) finish_battle();
        break;
    }
}

/* ------------------------------------------------------------------ */
/* menus                                                                */

static void go_title(void) {
    state = S_TITLE;
    state_t = 0;
    input_set_versus(false);
    game_set_pausable(false);
    music_play(BF_MUS_TITLE);
}

static void go_campaign(void) {
    state = S_CAMPAIGN;
    state_t = 0;
    level_sel = iclamp(sv.level_cursor, 0, BF_LEVELS - 1);
    while (level_sel > 0 && !level_open(level_sel)) level_sel--;
    input_set_versus(false);
    game_set_pausable(false);
    music_play(BF_MUS_CAMPAIGN);
}

static void go_versus_setup(void) {
    state = S_VERSUS;
    state_t = 0;
    vs_cur[0] = -1;
    vs_cur[1] = 0;
    input_set_versus(true);
    game_set_pausable(false);
    music_play(BF_MUS_CAMPAIGN);
}

static void update_title(void) {
    game_set_pausable(false);
    if (btn_repeat(BTN_UP)) { title_sel = (title_sel + 3) % 4; sfx_play_name("ui_move"); }
    if (btn_repeat(BTN_DOWN)) { title_sel = (title_sel + 1) % 4; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) {
        switch (title_sel) {
        case 0: sfx_play_name("ui_ok"); go_campaign(); break;
        case 1: sfx_play_name("ui_ok"); state = S_RANKED; state_t = 0; break;
        case 2: sfx_play_name("ui_ok"); state = S_SURVIVAL; state_t = 0; break;
        case 3:
            if (vita_single()) sfx_play_name("ui_error");
            else { sfx_play_name("ui_ok"); go_versus_setup(); }
            break;
        }
    }
}

static void update_campaign(void) {
    /* the road snakes across the map: odd rows run right to left */
    int r = level_sel / 6, c = r % 2 ? 5 - level_sel % 6 : level_sel % 6, old = level_sel;
    if (btn_repeat(BTN_LEFT)) c = (c + 5) % 6;
    if (btn_repeat(BTN_RIGHT)) c = (c + 1) % 6;
    if (btn_repeat(BTN_UP)) r = (r + 3) % 4;
    if (btn_repeat(BTN_DOWN)) r = (r + 1) % 4;
    level_sel = r * 6 + (r % 2 ? 5 - c : c);
    if (level_sel != old) sfx_play_name("ui_move");
    if (btnp(BTN_B)) { sfx_play_name("ui_back"); go_title(); return; }
    if (btnp(BTN_A) || btnp(BTN_START)) {
        if (!level_open(level_sel)) { sfx_play_name("ui_error"); return; }
        sfx_play_name("ui_ok");
        sv.level_cursor = (uint8_t)level_sel;
        cur_level = level_sel;
        state = S_BRIEF;
        state_t = 0;
    }
}

static void update_versus(void) {
    for (int p = 0; p < 2; p++) {
        bool (*rep)(int) = p ? btn_repeat2 : btn_repeat;
        bool (*press)(int) = p ? btnp2 : btnp;
        int *c = &vs_cur[p];
        if (*c == -1) {
            /* player 1 on the mode banner: left / right picks the mode */
            int d = rep(BTN_RIGHT) - rep(BTN_LEFT);
            if (d) {
                vs_preset = (vs_preset + d + VS_PRESETS) % VS_PRESETS;
                if (vs_preset == VS_RANDOM) vs_reroll();
                sfx_play_name("ui_move");
            }
            if (rep(BTN_DOWN)) {
                if (vs_preset == VS_CUSTOM) { *c = 0; sfx_play_name("ui_move"); }
                else if (vs_preset == VS_RANDOM) { vs_reroll(); sfx_play_name("bf_spawn"); } /* down deals new pools */
            }
            continue;
        }
        if (vs_preset != VS_CUSTOM) { *c = p ? 0 : -1; continue; }
        /* in the custom pools: the d-pad picks a place, A and B turn through the troops */
        int row = *c >= VS_SLOTS ? 2 : *c / 4, col = *c % 4;
        if (rep(BTN_UP)) {
            if (row == 0) { if (p == 0) *c = -1; }
            else *c = row == 2 ? 4 : *c - 4;
            sfx_play_name("ui_move");
        } else if (rep(BTN_DOWN) && row < 2) {
            *c = row == 0 ? *c + 4 : VS_SLOTS;
            sfx_play_name("ui_move");
        }
        if (*c < 0) continue;
        row = *c >= VS_SLOTS ? 2 : *c / 4;
        int d = rep(BTN_RIGHT) - rep(BTN_LEFT);
        if (row < 2) {
            if (d) {
                int nc = iclamp(col + d, 0, 3);
                if (nc != col) { *c = row * 4 + nc; sfx_play_name("ui_move"); }
            }
            int k = press(BTN_A) - press(BTN_B);
            if (k) {
                uint8_t *t = &vs_slot[p][*c];
                *t = (uint8_t)((*t + k + U_TYPES) % U_TYPES); /* empty, then the eight troops */
                sfx_play_name("bf_drag");
            }
        } else {
            int k = d + press(BTN_A) - press(BTN_B);
            if (k) { vs_flags[p] = (uint8_t)iclamp(vs_flags[p] + k, 1, 5); sfx_play_name("ui_move"); }
        }
    }
    if (vs_cur[0] == -1 && btnp(BTN_B)) { sfx_play_name("ui_back"); go_title(); return; }
    if (btnp(BTN_START) || (vs_cur[0] == -1 && btnp(BTN_A))) {
        sfx_play_name("ui_ok");
        start_versus();
    }
}

static void update_result(void) {
    if (state_t < 50) return;
    if (!(btnp(BTN_A) || btnp(BTN_START) || btnp(BTN_B))) return;
    sfx_play_name("ui_ok");
    switch (B.mode) {
    case MODE_CAMPAIGN:
        if (result_status == BF_WIN_L && campaign_done() && !sv.seen_ending) {
            sv.seen_ending = 1;
            save_now();
            state = S_ENDING;
            state_t = 0;
            music_play(BF_MUS_TITLE);
        } else {
            go_campaign();
        }
        break;
    case MODE_RANKED: state = S_RANKED; state_t = 0; music_play(BF_MUS_TITLE); break;
    case MODE_SURVIVAL: state = S_SURVIVAL; state_t = 0; music_play(BF_MUS_TITLE); break;
    default: go_versus_setup(); break;
    }
}

static void bf_update(void) {
    frame_t++;
    state_t++;
    if (shake > 0) shake--;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        if (p->kind == 0) p->vy += 0.1f;
        if (p->kind == 3 || p->kind == 4) p->vy *= 0.94f;
    }
    switch (state) {
    case S_TITLE: update_title(); break;
    case S_CAMPAIGN: update_campaign(); break;
    case S_BRIEF:
        if (btnp(BTN_B)) { sfx_play_name("ui_back"); go_campaign(); break; }
        if (state_t > 20 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); input_consume(); start_campaign_battle(cur_level); }
        break;
    case S_RANKED:
        if (btnp(BTN_B)) { sfx_play_name("ui_back"); go_title(); break; }
        if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); input_consume(); start_ranked(); }
        break;
    case S_SURVIVAL:
        if (btnp(BTN_B)) { sfx_play_name("ui_back"); go_title(); break; }
        if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); input_consume(); start_survival(); }
        break;
    case S_VERSUS: update_versus(); break;
    case S_PLAY: update_play(); break;
    case S_RESULT: update_result(); break;
    case S_ENDING:
        if (state_t > 120 && (btnp(BTN_A) || btnp(BTN_START))) { sfx_play_name("ui_ok"); go_title(); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing: the field                                                   */

/* grass colours per survival stage: dark, mid, light, flower */
static const uint8_t GRASS[5][4] = {
    {C_FOREST, C_JADE, C_LEAF, C_LIME},
    {C_EARTH, C_AMBER, C_YELLOW, C_CREAM},
    {C_WINE, C_MAGENTA, C_PINK, C_WHITE},
    {C_MAROON, C_WINE, C_RED, C_ORANGE},
    {C_INK, C_NIGHT, C_DUSK, C_SLATE},
};

static int field_stage(const Board *b) { return b->mode == MODE_SURVIVAL ? bf_survival_stage(b->turn) : 0; }

static void ants_rect(int x, int y, int w, int h, int col) {
    /* a dashed border */
    int n = 0, off = 0;
    for (int i = 0; i < w; i++, n++) {
        if ((n + off) % 6 < 3) { gfx_pset(x + i, y, col); gfx_pset(x + w - 1 - i, y + h - 1, col); }
    }
    for (int i = 0; i < h; i++, n++) {
        if ((n + off) % 6 < 3) { gfx_pset(x + w - 1, y + i, col); gfx_pset(x, y + h - 1 - i, col); }
    }
}

static void draw_field(const Board *b, bool planning) {
    const uint8_t *gc = GRASS[field_stage(b)];
    for (int y = 0; y < BF_ROWS; y++)
        for (int x = 0; x < BF_COLS; x++) {
            int px = tile_x(x), py = tile_y(y);
            gfx_rect(px, py, TW, TH, gc[1]);
            if (y & 1) gfx_dither(px, py, TW, TH, gc[0], 4);   /* mowed lanes */
            if ((x + y) & 1) gfx_dither(px, py, TW, TH, gc[2], 2);
            gfx_dither(px, py, TW, 1, gc[0], 12);
            uint32_t h = (uint32_t)(x * 7919 + y * 104729 + 17);
            h ^= h >> 7;
            h *= 2654435761u;
            h ^= h >> 13;
            for (int k = 0; k < 3; k++) {
                int bx = px + 2 + (int)((h >> (k * 5)) % (TW - 4)), by = py + 3 + (int)((h >> (k * 5 + 3)) % (TH - 6));
                gfx_pset(bx, by, gc[2]);
                gfx_pset(bx, by - 1, gc[3]);
            }
            if (h % 3 == 0 && field_stage(b) == 0) spr_draw(&bf_spr[BS_TUFT], px + 4 + (int)(h % 17), py + 6 + (int)((h >> 4) % 11), 0);
            if (h % 7 == 1 && field_stage(b) == 0) spr_draw(&bf_spr[BS_FLOWER], px + 3 + (int)((h >> 3) % 20), py + 4 + (int)((h >> 6) % 14), 0);
        }
    /* the midline */
    for (int yy = BY; yy < BY + BF_ROWS * TH; yy += 4) {
        gfx_vline(tile_x(BF_ZONE), yy, yy + 1, gc[3]);
        gfx_vline(tile_x(BF_ZONE) - 1, yy + 2, yy + 3, gc[0]);
    }
    /* the deployment zones while you plan */
    if (planning) {
        ants_rect(tile_x(0) + 1, BY + 1, BF_ZONE * TW - 2, BF_ROWS * TH - 2, C_YELLOW);
        if (two_players) ants_rect(tile_x(BF_ZONE) + 1, BY + 1, BF_ZONE * TW - 2, BF_ROWS * TH - 2, C_MAGENTA);
    }
}

static void draw_keep(const Board *b, int side) {
    int x0 = side == SIDE_L ? 0 : BX + BF_COLS * TW, w = BX;
    int stage = field_stage(b);
    int stone = stage == 4 ? C_DUSK : C_SLATE, light = stage == 4 ? C_SLATE : C_GREY, dark = stage == 4 ? C_NIGHT : C_DUSK;
    int top = BY - 8, bot = BY + BF_ROWS * TH;
    /* the wall */
    gfx_rect(x0, top, w, bot - top, stone);
    for (int yy = top + 5; yy < bot; yy += 6) {
        gfx_hline(x0, x0 + w - 1, yy, dark);
        for (int xx = x0 + ((yy / 6) % 2) * 5; xx < x0 + w; xx += 10) gfx_vline(xx, yy - 5, yy - 1, dark);
    }
    gfx_dither(x0, top, w, bot - top, light, 2);
    /* the sunlit rampart walk along the field side and its shadow on the grass */
    int edge = side == SIDE_L ? x0 + w - 4 : x0;
    gfx_rect(edge, top, 4, bot - top, light);
    gfx_vline(side == SIDE_L ? edge : edge + 3, top, bot - 1, C_LIGHT);
    gfx_dither(side == SIDE_L ? x0 + w : x0 - 3, BY, 3, bot - BY, C_INK, 6);
    for (int k = 0; k < 6; k++) gfx_rect(x0 + 1 + k * 8, top - 4, 5, 4, stone);
    gfx_hline(x0, x0 + w - 1, top, light);
    /* a gate facing every lane */
    int gx = side == SIDE_L ? x0 + w - 15 : x0 + 5;
    for (int y = 0; y < BF_ROWS; y++) {
        int gy = tile_y(y) + 4;
        gfx_rect(gx - 1, gy + 2, 12, 15, light);
        gfx_rect(gx, gy + 3, 10, 14, C_INK);
        gfx_rect(gx + 2, gy + 1, 6, 2, C_INK);
        gfx_rect(gx + 1, gy + 2, 8, 1, C_INK);
        for (int k = 1; k < 5; k++) gfx_vline(gx + k * 2, gy + 4, gy + 16, C_NIGHT); /* portcullis */
        gfx_hline(gx, gx + 9, gy + 9, C_NIGHT);
    }
    /* the promotion counter on the keep top: four stars, the fifth calls a champion */
    const Army *a = &b->a[side];
    if (a->heroes) {
        int px0 = x0 + 4;
        gfx_rect(px0 - 1, top + 1, 41, 8, C_INK);
        for (int i = 0; i < 4; i++) {
            int c = i < B.a[side].promos ? C_YELLOW : C_DUSK, sx = px0 + 4 + i * 10, sy = top + 4;
            gfx_pset(sx, sy - 2, c);
            gfx_hline(sx - 2, sx + 2, sy - 1, c);
            gfx_hline(sx - 1, sx + 1, sy, c);
            gfx_pset(sx - 2, sy + 1, c);
            gfx_pset(sx + 2, sy + 1, c);
        }
    }
    /* banners on their towers: standing, or fallen once taken */
    int tx = side == SIDE_L ? x0 + 3 : x0 + w - 19;
    if (a->flags == BF_NO_FLAGS) {
        char buf[16];
        gfx_rect(tx - 1, top + 20, 18, 22, C_INK);
        snprintf(buf, sizeof buf, "%d", b->keep_hits[SIDE_L]);
        tiny_center("HITS", tx + 8, top + 23, C_LIGHT);
        text_center(buf, tx + 8, top + 31, C_WHITE);
        return;
    }
    int n = imax(1, a->start_flags);
    int span = bot - top - 30;
    for (int i = 0; i < n; i++) {
        int ty = n == 1 ? top + span / 2 + 8 : top + 12 + i * span / (n - 1);
        /* the tower top */
        gfx_rect(tx, ty + 8, 16, 9, light);
        gfx_rect(tx, ty + 15, 16, 2, dark);
        for (int k = 0; k < 3; k++) gfx_rect(tx + k * 6, ty + 6, 4, 2, light);
        gfx_hline(tx, tx + 15, ty + 8, C_LIGHT);
        bool up = i < a->flags;
        int spr = up ? ((frame_t / 16 + i) % 2 ? BS_FLAG1 : BS_FLAG2) : BS_FLAG_DOWN;
        int fx = side == SIDE_L ? tx + 3 : tx + 3;
        spr_draw_ex(&bf_spr[spr], fx, ty - 3, side == SIDE_R ? SPR_FLIPX : 0, BF_TEAM_MAP[side], -1);
    }
}

static int unit_sprite(int type, int frame) {
    static const int base[U_TYPES] = {0, BS_FOOT1, BS_BOW1, BS_WARD1, BS_RIDER1, BS_PIKE1, BS_SHADE1, BS_POWDER1, BS_CHAMP1};
    return base[type] + (frame & 1);
}

/* the sprite a unit shows now: a warden at 3 HP or less has lost its shield */
static int unit_sprite_now(const Unit *u, int frame) {
    if (u->type == U_WARD && u->hp < 4) return BS_WARDX1 + (frame & 1);
    return unit_sprite(u->type, frame);
}

/* guard: a footman standing in a column of three, shrugging off blows */
static void draw_unit_at(const Unit *u, int px, int py, int frame, bool flash, bool lifted, bool guard) {
    const Sprite *s = &bf_spr[unit_sprite_now(u, frame)];
    int sx = px + (TW - s->w) / 2, sy = py + TH - s->h - 4 - (lifted ? 4 : 0);
    int fl = u->side == SIDE_R ? SPR_FLIPX : 0;
    /* shadow */
    gfx_dither(px + 6, py + TH - 6, TW - 12, 3, C_INK, lifted ? 6 : 9);
    if (guard && !flash) {
        int oc = u->side == SIDE_L ? C_CREAM : C_PINK;
        spr_draw_ex(s, sx - 1, sy, fl, NULL, oc);
        spr_draw_ex(s, sx + 1, sy, fl, NULL, oc);
        spr_draw_ex(s, sx, sy - 1, fl, NULL, oc);
    }
    if (flash) spr_draw_ex(s, sx, sy, fl, NULL, C_WHITE);
    else spr_draw_ex(s, sx, sy, fl, BF_TEAM_MAP[u->side], -1);
    /* hit points: bars, or stars once promoted (a promoted unit hits for two) */
    int hp = u->hp, maxhp = BF_UNITS[u->type].hp;
    int step = u->promo ? 4 : 3;
    int bw = maxhp * step;
    int bx = px + (TW - bw) / 2, by = py + TH - 3;
    int on = u->side == SIDE_L ? C_YELLOW : C_PINK;
    if (!u->promo) {
        gfx_rect(bx - 1, by - 1, bw + 1, 3, C_INK);
        for (int i = 0; i < maxhp; i++) gfx_rect(bx + i * 3, by, 2, 1, i < hp ? on : C_DUSK);
    } else {
        gfx_rect(bx - 1, by - 2, bw + 1, 5, C_INK);
        for (int i = 0; i < maxhp; i++) {
            int c = i < hp ? on : C_DUSK, x = bx + i * 4 + 1;
            gfx_pset(x, by - 1, c);
            gfx_hline(x - 1, x + 1, by, c);
            gfx_pset(x, by + 1, c);
        }
    }
}

static const BEvent *find_ev(int phase, int type, int id) {
    for (int i = 0; i < rev.n; i++)
        if (rev.ev[i].phase == phase && rev.ev[i].type == type && rev.ev[i].id == id) return &rev.ev[i];
    return NULL;
}

static bool damaged_now(int phase, int id, float f) {
    if (f < 0.5f || f > 0.8f) return false;
    for (int i = 0; i < rev.n; i++)
        if (rev.ev[i].phase == phase && rev.ev[i].type == BE_DAMAGE && rev.ev[i].id == id && rev.ev[i].a > 0) return true;
    return false;
}

static void draw_units(void) {
    const Board *b = &B;
    int phase = -1;
    float f = 0;
    if (state == S_PLAY && sub == P_RESOLVE && res_phase < PH_COUNT) {
        phase = res_phase;
        b = &snaps[phase];
        f = res_len[phase] ? (float)res_t / (float)res_len[phase] : 0;
    }
    for (int y = 0; y < BF_ROWS; y++)
        for (int x = 0; x < BF_COLS; x++) {
            const Unit *u = &b->g[y][x];
            if (!u->type) continue;
            float ox = 0, oy = 0;
            int frame = ((frame_t / 20) + x + y) & 1;
            bool lifted = false, flash = false;
            if (phase >= 0) {
                const BEvent *e;
                int dir = u->side == SIDE_L ? 1 : -1;
                if ((e = find_ev(phase, BE_MOVE, u->id)) != NULL) {
                    ox = (e->x2 - e->x) * TW * f;
                    frame = (int)(f * 4) & 1;
                    if (e->x2 < 0 || e->x2 >= BF_COLS) {
                        if (f > 0.6f) continue; /* into the keep */
                    }
                } else if ((e = find_ev(phase, BE_MELEE, u->id)) != NULL || (e = find_ev(phase, BE_REACH, u->id)) != NULL) {
                    int times = e->a ? 2 : 1; /* promoted: the attack plays twice */
                    float k = sinf(f * 3.14159f * times);
                    ox = dir * fabsf(k) * 5;
                    frame = 1;
                } else if ((e = find_ev(phase, BE_CLASH, u->id)) != NULL) {
                    ox = dir * sinf(f * 3.14159f) * 8;
                    frame = 1;
                } else {
                    /* the right-hand clasher: its event is keyed on the left unit */
                    for (int i = 0; i < rev.n; i++) {
                        const BEvent *c = &rev.ev[i];
                        if (c->phase == phase && c->type == BE_CLASH && c->x2 == x && c->y2 == y) {
                            ox = dir * sinf(f * 3.14159f) * 8;
                            frame = 1;
                        }
                    }
                }
                flash = damaged_now(phase, u->id, f) && (frame_t / 2) % 2;
            } else if (state == S_PLAY && sub == P_SPAWN) {
                for (int i = 0; i < sev.n; i++)
                    if (sev.ev[i].id == u->id) {
                        float t = (float)sub_t / SPAWN_FRAMES;
                        oy = -(1 - t) * (1 - t) * 26;
                        if (sub_t == SPAWN_FRAMES - 2) dust(tile_x(x) + TW / 2.0f, tile_y(y) + TH - 4, 6, C_TAN);
                    }
            }
            for (int p = 0; p < 2; p++)
                if (hand[p].grab && hand[p].cx == x && hand[p].cy == y && state == S_PLAY) lifted = true;
            bool guard = u->type == U_FOOT && bf_in_column(b, x, y);
            draw_unit_at(u, tile_x(x) + (int)ox, tile_y(y) + (int)oy, frame, flash, lifted, guard);
        }
    /* projectiles */
    if (phase < 0) return;
    for (int i = 0; i < rev.n; i++) {
        const BEvent *e = &rev.ev[i];
        if (e->phase != phase) continue;
        int shots = e->a ? 2 : 1;
        for (int k = 0; k < shots; k++) {
            float t = f * 1.3f - k * 0.3f;
            if (t < 0 || t > 1) continue;
            if (e->type == BE_ARROW) {
                float x = tile_x(e->x) + TW / 2 + (e->x2 - e->x) * TW * t;
                int y = tile_y(e->y) + 9 - (int)(sinf(t * 3.14159f) * 4);
                spr_draw(&bf_spr[BS_ARROW], (int)x - 3, y, e->b == SIDE_R ? SPR_FLIPX : 0);
            } else if (e->type == BE_KNIFE) {
                int x = tile_x(e->x) + TW / 2 - 1;
                float y = tile_y(e->y) + 8 + (e->y2 - e->y) * TH * t;
                spr_draw(&bf_spr[BS_KNIFE], x, (int)y, e->y2 > e->y ? SPR_FLIPY : 0);
            }
        }
    }
}

static void draw_hands(void) {
    if (state != S_PLAY) return;
    for (int p = 0; p < (two_players ? 2 : 1); p++) {
        Hand *h = &hand[p];
        int px = tile_x(h->cx), py = tile_y(h->cy);
        int col = p == 0 ? C_YELLOW : C_MAGENTA;
        if (h->bump > 0) px += (h->bump / 2) % 2 ? 1 : -1;
        /* reach while dragging: the zone up to where the unit was picked up */
        if (h->grab)
            for (int y = 0; y < BF_ROWS; y++)
                for (int x = 0; x < BF_COLS; x++) {
                    bool ok = bf_in_zone(p, x) && (p == 0 ? x <= h->pickup : x >= h->pickup);
                    if (ok && !(x == h->cx && y == h->cy)) gfx_dither(tile_x(x) + 2, tile_y(y) + 2, TW - 4, TH - 4, col, 3);
                }
        int k = (frame_t / 8) % 2;
        int x0 = px - k, y0 = py - k, x1 = px + TW - 1 + k, y1 = py + TH - 1 + k;
        gfx_hline(x0, x0 + 4, y0, col); gfx_vline(x0, y0, y0 + 4, col);
        gfx_hline(x1 - 4, x1, y0, col); gfx_vline(x1, y0, y0 + 4, col);
        gfx_hline(x0, x0 + 4, y1, col); gfx_vline(x0, y1 - 4, y1, col);
        gfx_hline(x1 - 4, x1, y1, col); gfx_vline(x1, y1 - 4, y1, col);
        spr_draw(&bf_spr[h->grab ? BS_HAND_GRAB : BS_HAND], px + TW - 9, py - 4 + (h->grab ? 0 : k), p ? SPR_FLIPX : 0);
        if (two_players) tiny_draw(p == 0 ? "1" : "2", px + 2, py + 1, col);
    }
}

static void draw_particles(void) {
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        switch (p->kind) {
        case 2: gfx_circb((int)p->x, (int)p->y, (p->max - p->life) * 2, p->col); break;
        case 3: {
            char buf[8];
            snprintf(buf, sizeof buf, "-%d", p->val);
            tiny_draw(buf, (int)p->x - 3, (int)p->y, p->life / 3 % 2 ? p->col : C_RED);
            break;
        }
        case 4: tiny_draw("0", (int)p->x - 1, (int)p->y, p->col); break;
        default: gfx_rect((int)p->x, (int)p->y, p->life > 6 ? 2 : 1, p->life > 6 ? 2 : 1, p->col); break;
        }
    }
}

/* the countdown: a number in the middle, bars on either side that drain
 * toward it; 9 counts, 6 in Double Time, 4 in Triple Time */
static void draw_timer(void) {
    int counts = bf_timer_counts(&B);
    int total = counts * COUNT_FRAMES;
    int left = sub == P_PLAN ? total - plan_t : sub == P_SPAWN ? total : 0;
    left = iclamp(left, 0, total); /* a frozen test board shows a full count */
    int shown = (left + COUNT_FRAMES - 1) / COUNT_FRAMES;
    const int BW = 54;
    int fill = total > 0 ? (BW * left + total - 1) / total : 0;
    bool hurry = shown <= 3;
    int bar = hurry ? C_ORANGE : C_CREAM;
    /* left bar drains toward the middle from its outer end, the right one mirrors it */
    gfx_rect(160 - 12 - BW - 1, 4, BW + 2, 8, C_INK);
    gfx_rect(160 + 12 - 1, 4, BW + 2, 8, C_INK);
    gfx_rect(160 - 12 - BW, 5, BW, 6, C_DUSK);
    gfx_rect(160 + 12, 5, BW, 6, C_DUSK);
    if (fill > 0) {
        gfx_rect(160 - 12 - fill, 5, fill, 6, bar);
        gfx_rect(160 + 12, 5, fill, 6, bar);
        gfx_hline(160 - 12 - fill, 160 - 13, 5, C_WHITE);
        gfx_hline(160 + 12, 160 + 11 + fill, 5, C_WHITE);
    }
    gfx_rect(160 - 10, 1, 20, 14, C_INK);
    if (sub == P_RESOLVE || sub == P_OVER) {
        gfx_rect(160 - 12 - BW - 1, 3, 2 * BW + 26, 10, C_NIGHT);
        text_center(sub == P_OVER ? "HALT!" : "MARCH!", 160, 4, (frame_t / 4) % 2 ? C_YELLOW : C_ORANGE);
    } else {
        char buf[12];
        snprintf(buf, sizeof buf, "%d", shown);
        text_draw_scaled(buf, 160 - text_width_scaled(buf, 2) / 2 + 1, 1, hurry && (frame_t / 4) % 2 ? C_RED : hurry ? C_ORANGE : C_WHITE, 2);
    }
    if (btn(BTN_B) && !two_players && sub != P_OVER) tiny_draw(GLYPH_RIGHT GLYPH_RIGHT, 160 + 12 + BW + 4, 6, C_LIME);
}

static void draw_hud(void) {
    gfx_rect(0, 0, SCREEN_W, 15, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 15, C_DUSK);
    char buf[64];
    switch (B.mode) {
    case MODE_CAMPAIGN: snprintf(buf, sizeof buf, "BATTLE %d", cur_level + 1); break;
    case MODE_RANKED: snprintf(buf, sizeof buf, "RANK %d", sv.rank); break;
    case MODE_SURVIVAL: snprintf(buf, sizeof buf, "SCORE %u", (unsigned)B.score); break;
    default: snprintf(buf, sizeof buf, "2P VERSUS"); break;
    }
    text_draw(buf, 4, 4, C_YELLOW);
    snprintf(buf, sizeof buf, "TURN %d", B.turn);
    text_draw(buf, SCREEN_W - 4 - text_width(buf), 4, C_LIGHT);
    draw_timer();
}

static void draw_info_side(int p, int x0, int w) {
    Hand *h = &hand[p];
    const Board *b = sub == P_RESOLVE && res_phase < PH_COUNT ? &snaps[res_phase] : &B;
    const Unit *u = &b->g[h->cy][h->cx];
    int y0 = 160;
    if (u->type) {
        const Sprite *s = &bf_spr[unit_sprite_now(u, 0)];
        spr_draw_ex(s, x0 + 2, y0 + 2, u->side == SIDE_R ? SPR_FLIPX : 0, BF_TEAM_MAP[u->side], -1);
        int tx = x0 + 4 + s->w;
        text_draw(BF_UNITS[u->type].name, tx, y0 + 1, u->side == SIDE_L ? C_YELLOW : C_MAGENTA);
        char buf[32];
        snprintf(buf, sizeof buf, "HP %d/%d%s", u->hp, BF_UNITS[u->type].hp, u->promo ? " " GLYPH_STAR : "");
        tiny_draw(buf, tx + text_width(BF_UNITS[u->type].name) + 4, y0 + 2, C_LIGHT);
        if (w > 200) {
            tiny_draw(BF_UNITS[u->type].line1, tx, y0 + 10, C_GREY);
            tiny_draw(BF_UNITS[u->type].line2, tx + tiny_width(BF_UNITS[u->type].line1) + 4, y0 + 10, C_GREY);
        } else {
            tiny_draw(BF_UNITS[u->type].line1, tx, y0 + 10, C_GREY);
        }
    } else {
        tiny_draw(two_players ? "HOLD A: GRAB  D-PAD: DRAG" : "HOLD A + D-PAD: DRAG A UNIT   HOLD B: FAST", x0 + 4, y0 + 7, C_SLATE);
    }
}

static void draw_bottom(void) {
    gfx_rect(0, 158, SCREEN_W, 22, C_NIGHT);
    gfx_hline(0, SCREEN_W - 1, 158, C_DUSK);
    if (two_players) {
        draw_info_side(0, 0, 160);
        gfx_vline(160, 160, 179, C_DUSK);
        draw_info_side(1, 161, 159);
    } else if (tip_t > 0 && B.mode == MODE_CAMPAIGN && BF_LEVELS_DEF[cur_level].tip) {
        /* the battle's lesson, for the first few seconds */
        tiny_center(BF_LEVELS_DEF[cur_level].tip, 160, 166, (tip_t / 8) % 4 ? C_YELLOW : C_CREAM);
    } else {
        draw_info_side(0, 0, 320);
        if (B.a[SIDE_R].handicap > 0) {
            char buf[24];
            snprintf(buf, sizeof buf, "FOE +%d%%", B.a[SIDE_R].handicap);
            tiny_draw(buf, SCREEN_W - 4 - tiny_width(buf), 161, C_PINK);
        }
    }
}

static void draw_play(void) {
    int sx = 0, sy = 0;
    if (shake > 0) { sx = (frame_t % 3) - 1; sy = ((frame_t / 2) % 3) - 1; }
    gfx_cls(C_NIGHT);
    gfx_camera(sx, sy);
    const Board *shown = sub == P_RESOLVE && res_phase < PH_COUNT ? &snaps[res_phase] : &B;
    draw_field(&B, sub == P_PLAN || sub == P_SPAWN);
    draw_keep(shown, SIDE_L);
    draw_keep(shown, SIDE_R);
    draw_units();
    draw_hands();
    draw_particles();
    gfx_camera(0, 0);
    draw_hud();
    draw_bottom();
    if (banner_t > 0) {
        static const uint8_t grad[] = {C_YELLOW, C_ORANGE, C_RED};
        int y = 70 + (banner_t > 80 ? (banner_t - 80) * 3 : 0);
        gfx_rect(0, y - 4, SCREEN_W, 26, C_INK);
        ui_fancy_center(banner_kind == 1 ? "QUICK MARCH!" : banner_kind == 2 ? "DOUBLE QUICK!" : "A CHAMPION ARRIVES!", 160, y, 2, grad, 3, C_INK, C_MAROON);
    }
    if (sub == P_OVER) {
        int lvl = imin(10, sub_t / 3);
        gfx_dither(0, 16, SCREEN_W, 142, C_INK, lvl);
        static const uint8_t gw[] = {C_CREAM, C_YELLOW, C_AMBER};
        static const uint8_t gl[] = {C_PINK, C_MAGENTA, C_VIOLET};
        static const uint8_t gd[] = {C_WHITE, C_LIGHT, C_GREY};
        const char *t = B.status == BF_WIN_L ? (two_players ? "GOLD WINS!" : "VICTORY!")
                        : B.status == BF_WIN_R ? (two_players ? "VIOLET WINS!" : B.mode == MODE_SURVIVAL ? "OVERRUN!" : "DEFEAT")
                        : "DRAW";
        const uint8_t *g = B.status == BF_WIN_L ? gw : B.status == BF_WIN_R ? gl : gd;
        if (sub_t > 10) ui_fancy_center(t, 160, 72, 3, g, 3, C_INK, C_NIGHT);
    }
}

/* ------------------------------------------------------------------ */
/* drawing: menus                                                       */

static void draw_sky(void) {
    for (int y = 0; y < 90; y++) gfx_hline(0, SCREEN_W - 1, y, y < 30 ? C_NAVY : y < 60 ? C_BLUE : C_SKY);
    gfx_dither(0, 28, SCREEN_W, 4, C_BLUE, 8);
    gfx_dither(0, 58, SCREEN_W, 4, C_SKY, 8);
    /* hills */
    for (int x = 0; x < SCREEN_W; x++) {
        int h1 = 96 + (int)(sinf(x * 0.021f) * 8 + sinf(x * 0.05f + 1) * 4);
        gfx_vline(x, h1, 110, C_FOREST);
        int h2 = 106 + (int)(sinf(x * 0.03f + 2) * 5);
        gfx_vline(x, h2, SCREEN_H - 1, C_JADE);
    }
    gfx_dither(0, 112, SCREEN_W, 68, C_LEAF, 3);
    /* clouds */
    for (int i = 0; i < 4; i++) {
        int cx = (i * 97 + frame_t / 6) % 360 - 20, cy = 8 + (i * 7) % 14;
        gfx_circ(cx, cy, 6, C_ICE);
        gfx_circ(cx + 8, cy - 2, 8, C_WHITE);
        gfx_circ(cx + 17, cy, 6, C_ICE);
        gfx_rect(cx, cy, 18, 6, C_WHITE);
    }
}

static void draw_mini_keep(int x, int y, int side, int flags) {
    gfx_rect(x, y, 34, 40, C_SLATE);
    gfx_dither(x, y, 34, 40, C_GREY, 3);
    for (int k = 0; k < 4; k++) gfx_rect(x + 1 + k * 9, y - 4, 6, 4, C_SLATE);
    gfx_rect(x + 12, y + 24, 10, 16, C_INK);
    gfx_rect(x + 14, y + 22, 6, 2, C_INK);
    for (int i = 0; i < flags; i++)
        spr_draw_ex(&bf_spr[(frame_t / 14 + i) % 2 ? BS_FLAG1 : BS_FLAG2], x + 2 + i * 11, y - 14, side ? SPR_FLIPX : 0, BF_TEAM_MAP[side], -1);
}

static void spr_team_scaled(const Sprite *s, int x, int y, int scale, int flip, int side) {
    for (int sy = 0; sy < s->h; sy++)
        for (int sx = 0; sx < s->w; sx++) {
            int srcx = flip ? s->w - 1 - sx : sx;
            uint8_t c = s->px[sy * s->w + srcx];
            if (c != TRANSPARENT) gfx_rect(x + sx * scale, y + sy * scale, scale, scale, BF_TEAM_MAP[side][c]);
        }
}

static void draw_title(void) {
    draw_sky();
    draw_mini_keep(10, 90, SIDE_L, 3);
    draw_mini_keep(276, 90, SIDE_R, 3);
    /* the two armies facing off across the meadow */
    static const int lineup[6] = {U_FOOT, U_BOW, U_PIKE, U_FOOT, U_SHADE, U_POWDER};
    for (int i = 0; i < 6; i++) {
        int t = lineup[i], bob = (frame_t / 10 + i) % 2;
        const Sprite *s = &bf_spr[unit_sprite(t, (frame_t / 10 + i) & 1)];
        int row = i % 2;
        spr_draw_ex(s, 52 + (i / 2) * 20 + row * 10, 118 + row * 12 - bob, 0, BF_TEAM_MAP[0], -1);
        spr_draw_ex(s, 252 - (i / 2) * 20 - row * 10, 118 + row * 12 - bob, SPR_FLIPX, BF_TEAM_MAP[1], -1);
    }
    /* the two captains up front */
    int bob = (frame_t / 12) % 2;
    spr_team_scaled(&bf_spr[BS_CHAMP1 + ((frame_t / 12) & 1)], 18, 138 - bob, 2, 0, SIDE_L);
    spr_team_scaled(&bf_spr[BS_WARD1 + ((frame_t / 12) & 1)], 270, 138 - bob, 2, 1, SIDE_R);
    static const uint8_t grad[] = {C_CREAM, C_YELLOW, C_AMBER, C_ORANGE};
    ui_fancy_center("BANNERFALL", 160, 12, 3, grad, 4, C_INK, C_MAROON);
    text_center("THE WAR FOR THE MARIGOLD MARCH", 160, 40, C_CREAM);
    static const char *items[4] = {"CAMPAIGN", "RANKED", "SURVIVAL", "2P VERSUS"};
    ui_panel(112, 52, 96, 58, C_NIGHT, C_AMBER);
    for (int i = 0; i < 4; i++) {
        int y = 57 + i * 12;
        bool s = i == title_sel;
        int col = s ? C_WHITE : C_GREY;
        if (i == 3 && vita_single()) col = s ? C_GREY : C_SLATE;
        text_center(items[i], 160, y, col);
        if (s) ui_cursor(160 - text_width(items[i]) / 2 - 10, y, frame_t);
    }
    char buf[64];
    if (title_sel == 3 && vita_single()) snprintf(buf, sizeof buf, "NEEDS TWO CONTROLLERS");
    else snprintf(buf, sizeof buf, "WON %d/24   RANK %d   LOSSES %d", beaten_count(), sv.rank, sv.losses);
    gfx_rect(80, 168, 160, 10, C_INK);
    tiny_center(buf, 160, 170, C_LIGHT);
}

static void draw_pool(const char *letters, int x, int y, int side, int maxw) {
    Army a;
    memset(&a, 0, sizeof a);
    bf_pool_from_letters(&a, letters);
    int step = a.pool_n > 1 ? imin(18, (maxw - 20) / (a.pool_n - 1)) : 18;
    for (int i = 0; i < a.pool_n; i++) {
        const Sprite *s = &bf_spr[unit_sprite(a.pool[i], 0)];
        spr_draw_ex(s, x + i * step, y, side ? SPR_FLIPX : 0, BF_TEAM_MAP[side], -1);
    }
}

/* the troops a battle brings in that no earlier battle had (a bit per type;
 * the champion called by promotions counts as the champion) */
static int new_troops(int lvl) {
    int seen = 0, here = 0;
    for (int i = 0; i <= lvl; i++) {
        const LevelDef *d = &BF_LEVELS_DEF[i];
        int m = 0;
        for (int side = 0; side < 2; side++) {
            Army a;
            memset(&a, 0, sizeof a);
            bf_pool_from_letters(&a, side ? d->pool_r : d->pool_l);
            for (int k = 0; k < a.pool_n; k++) m |= 1 << a.pool[k];
        }
        if (d->heroes) m |= 1 << U_CHAMP;
        if (i < lvl) seen |= m;
        else here = m;
    }
    return here & ~seen;
}

/* the next battle to win is shown, still locked: its name and new troops */
static bool level_named(int i) { return level_open(i) || (i > 0 && level_open(i - 1)); }

static void camp_pos(int i, int *x, int *y) {
    int r = i / 6, c = r % 2 ? 5 - i % 6 : i % 6;
    *x = 22 + c * 23;
    *y = 34 + r * 30;
}

static void tree(int x, int y) {
    gfx_rect(x, y + 3, 1, 3, C_BROWN);
    gfx_circ(x, y, 3, C_FOREST);
    gfx_circ(x - 1, y - 1, 2, C_JADE);
    gfx_pset(x - 1, y - 2, C_LEAF);
}

static void draw_campaign(void) {
    gfx_cls(C_NIGHT);
    char buf[64];
    static const uint8_t grad[] = {C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_text("THE CAMPAIGN", 6, 4, 1, grad, 3, C_INK, -1);
    snprintf(buf, sizeof buf, "WON %d/24  LOSSES %d", beaten_count(), sv.losses);
    tiny_draw(buf, 158 - tiny_width(buf), 7, C_GREY);
    /* the war-table map */
    ui_panel(4, 18, 152, 146, C_HIDE, C_BROWN);
    gfx_dither(5, 19, 150, 144, C_EARTH, 2);
    gfx_rectb(7, 21, 146, 140, C_TAN);
    /* a river and some woods */
    for (int yy = 22; yy < 160; yy++) {
        int rx = 84 + (int)(sinf(yy * 0.09f) * 5);
        gfx_hline(rx, rx + 3, yy, C_SKY);
        gfx_pset(rx + 1, yy, (yy + frame_t / 8) % 5 ? C_SKY : C_ICE);
    }
    static const int16_t TREES[][2] = {{12, 26}, {48, 50}, {130, 26}, {118, 80}, {30, 108}, {66, 138}, {140, 140}, {100, 110}, {16, 76}};
    for (int i = 0; i < ARRAY_LEN(TREES); i++) tree(TREES[i][0], TREES[i][1]);
    /* the road */
    for (int i = 0; i + 1 < BF_LEVELS; i++) {
        int x0, y0, x1, y1;
        camp_pos(i, &x0, &y0);
        camp_pos(i + 1, &x1, &y1);
        int col = (sv.beaten >> i) & 1 ? C_BROWN : C_TAN;
        if (y0 == y1) {
            for (int x = imin(x0, x1); x <= imax(x0, x1); x += 3) gfx_rect(x, y0, 2, 2, col);
        } else {
            for (int y = y0; y <= y1; y += 3) gfx_rect(x0, y, 2, 2, col);
        }
    }
    /* the battles */
    for (int i = 0; i < BF_LEVELS; i++) {
        int x, y;
        camp_pos(i, &x, &y);
        bool won = (sv.beaten >> i) & 1, open = level_open(i), sel = i == level_sel;
        int fill = won ? C_AMBER : open ? C_CREAM : C_TAN;
        gfx_circ(x, y + 1, 7, C_BROWN);
        gfx_circ(x, y, 7, C_INK);
        gfx_circ(x, y, 6, fill);
        if (won) {
            spr_draw_ex(&bf_spr[BS_FLAG1], x - 2, y - 7, 0, BF_TEAM_MAP[0], -1);
        } else if (open) {
            snprintf(buf, sizeof buf, "%d", i + 1);
            text_center(buf, x + 1, y - 3, C_BROWN);
            if ((frame_t / 20) % 2) gfx_circb(x, y, 9, C_ORANGE);
        } else {
            text_center(GLYPH_LOCK, x + 1, y - 3, C_EARTH);
        }
        if (sel) {
            int k = (frame_t / 8) % 2;
            gfx_circb(x, y, 10 + k, C_WHITE);
            gfx_circb(x, y, 11 + k, C_INK);
        }
    }
    /* details */
    const LevelDef *d = &BF_LEVELS_DEF[level_sel];
    ui_panel(160, 18, 156, 146, C_NIGHT, C_AMBER);
    snprintf(buf, sizeof buf, "BATTLE %d", level_sel + 1);
    tiny_draw(buf, 166, 23, C_GREY);
    text_draw(level_named(level_sel) ? d->name : "? ? ?", 166, 31, C_YELLOW);
    if (level_open(level_sel)) {
        snprintf(buf, sizeof buf, "YOUR BANNERS %d", d->flags_l);
        tiny_draw(buf, 166, 44, C_CREAM);
        draw_pool(d->pool_l, 166, 52, SIDE_L, 144);
        snprintf(buf, sizeof buf, "THEIR BANNERS %d", d->flags_r);
        tiny_draw(buf, 166, 74, C_PINK);
        draw_pool(d->pool_r, 166, 82, SIDE_R, 144);
        if (d->handicap) {
            snprintf(buf, sizeof buf, "THE HOST BRINGS %d%% MORE", d->handicap);
            tiny_draw(buf, 166, 104, C_PINK);
        } else tiny_draw("EVEN NUMBERS", 166, 104, C_GREY);
        if (d->heroes) tiny_draw("5 PROMOTIONS CALL A CHAMPION", 166, 112, C_YELLOW);
        if ((sv.beaten >> level_sel) & 1) text_draw(GLYPH_CHECK " WON", 166, 124, C_LIME);
    } else if (level_named(level_sel)) {
        snprintf(buf, sizeof buf, "WIN BATTLE %d TO OPEN IT", level_sel);
        tiny_draw(buf, 166, 44, C_SLATE);
        int nt = new_troops(level_sel), row = 0;
        if (nt) tiny_draw("NEW TROOPS", 166, 58, C_YELLOW);
        for (int t = U_FOOT; t < U_TYPES; t++) {
            if (!((nt >> t) & 1)) continue;
            int yy = 68 + row * 22;
            spr_draw_ex(&bf_spr[unit_sprite(t, 0)], 166, yy, 0, BF_TEAM_MAP[0], -1);
            tiny_draw(BF_UNITS[t].name, 186, yy + 1, C_CREAM);
            tiny_draw(BF_UNITS[t].line1, 186, yy + 9, C_GREY);
            row++;
        }
        if (!nt) tiny_draw("NO NEW TROOPS", 166, 58, C_GREY);
    } else {
        text_draw("WIN THE BATTLE\nBEFORE IT FIRST.", 166, 48, C_SLATE);
    }
    ui_hint(166, 150, GLYPH_A, "FIGHT", C_LIGHT);
    ui_hint(226, 150, GLYPH_B, "BACK", C_LIGHT);
}

static void draw_brief(void) {
    draw_sky();
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, 8);
    const LevelDef *d = &BF_LEVELS_DEF[cur_level];
    ui_panel(30, 20, 260, 140, C_NIGHT, C_AMBER);
    char buf[64];
    snprintf(buf, sizeof buf, "BATTLE %d", cur_level + 1);
    text_center(buf, 160, 27, C_GREY);
    static const uint8_t grad[] = {C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_center(d->name, 160, 38, 2, grad, 3, C_INK, C_MAROON);
    tiny_draw("MARIGOLD GUARD", 40, 62, C_YELLOW);
    snprintf(buf, sizeof buf, "%d BANNERS", d->flags_l);
    tiny_draw(buf, 240, 62, C_CREAM);
    draw_pool(d->pool_l, 40, 70, SIDE_L, 240);
    tiny_draw("THISTLE HOST", 40, 94, C_MAGENTA);
    snprintf(buf, sizeof buf, "%d BANNERS", d->flags_r);
    tiny_draw(buf, 240, 94, C_PINK);
    draw_pool(d->pool_r, 40, 102, SIDE_R, 240);
    if (d->handicap) snprintf(buf, sizeof buf, "THE HOST BRINGS %d%% MORE TROOPS.", d->handicap);
    else snprintf(buf, sizeof buf, "THE ARMIES ARE EVEN.");
    text_center(buf, 160, 122, C_PINK);
    if (d->tip) tiny_center(d->tip, 160, 134, C_YELLOW);
    if (state_t > 20 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A " TO MARCH", 160, 146, C_WHITE);
}

static void draw_ranked(void) {
    draw_sky();
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, 8);
    ui_panel(50, 24, 220, 132, C_NIGHT, C_AMBER);
    static const uint8_t grad[] = {C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_center("RANKED", 160, 32, 2, grad, 3, C_INK, C_MAROON);
    char buf[64];
    snprintf(buf, sizeof buf, "%d", sv.rank);
    static const uint8_t g2[] = {C_WHITE, C_CREAM, C_YELLOW};
    ui_fancy_center(buf, 160, 56, 3, g2, 3, C_INK, C_NIGHT);
    text_center(rank_title(sv.rank), 160, 84, C_YELLOW);
    snprintf(buf, sizeof buf, "BEST %d (%s)", sv.best_rank, rank_title(sv.best_rank));
    tiny_center(buf, 160, 96, C_GREY);
    snprintf(buf, sizeof buf, "3 BANNERS EACH. THE HOST BRINGS %d%% MORE.", sv.rank);
    tiny_center(buf, 160, 108, C_PINK);
    tiny_center("WIN BIG TO CLIMB: +10 +4 +2 / -1 -3 -5", 160, 118, C_GREY);
    if (state_t > 10 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A " TO MARCH", 160, 136, C_WHITE);
}

static void draw_survival_menu(void) {
    draw_sky();
    gfx_dither(0, 0, SCREEN_W, SCREEN_H, C_INK, 8);
    ui_panel(50, 24, 220, 132, C_NIGHT, C_RED);
    static const uint8_t grad[] = {C_PINK, C_RED, C_WINE};
    ui_fancy_center("SURVIVAL", 160, 32, 2, grad, 3, C_INK, C_MAROON);
    text_center("HOLD YOUR THREE BANNERS", 160, 62, C_LIGHT);
    text_center("AGAINST AN ENDLESS HOST.", 160, 72, C_LIGHT);
    tiny_center("EVERY TURN SCORES. EVERY HIT ON THEIR KEEP", 160, 88, C_GREY);
    tiny_center("MAKES EACH TURN WORTH ONE MORE.", 160, 96, C_GREY);
    char buf[48];
    snprintf(buf, sizeof buf, "BEST %u", (unsigned)sv.best_survival);
    text_center(buf, 160, 112, C_YELLOW);
    if (state_t > 10 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A " TO MARCH", 160, 136, C_WHITE);
}

/* one army's pool: 2 rows of 4 places */
static void draw_vs_pool(int p, const uint8_t *slot, int x0, int y0, bool cursor) {
    int col = p == 0 ? C_YELLOW : C_MAGENTA;
    for (int i = 0; i < VS_SLOTS; i++) {
        int x = x0 + (i % 4) * 26, y = y0 + (i / 4) * 24;
        bool sel = cursor && vs_cur[p] == i;
        gfx_rect(x, y, 22, 20, sel ? C_DUSK : C_NIGHT);
        gfx_rectb(x, y, 22, 20, sel && (frame_t / 8) % 2 ? col : C_DUSK);
        if (slot[i]) {
            const Sprite *sp = &bf_spr[unit_sprite(slot[i], sel ? (frame_t / 10) & 1 : 0)];
            spr_draw_ex(sp, x + (22 - sp->w) / 2, y + 2, p ? SPR_FLIPX : 0, BF_TEAM_MAP[p], -1);
        }
    }
}

static void draw_versus(void) {
    gfx_cls(C_NIGHT);
    static const uint8_t grad[] = {C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_center("2P VERSUS", 160, 4, 2, grad, 3, C_INK, C_MAROON);
    static const char *PRESETS[VS_PRESETS] = {"RECRUITS", "REGULARS", "FULL MUSTER", "CUSTOM", "RANDOM"};
    char buf[64];
    snprintf(buf, sizeof buf, GLYPH_LEFT " %s " GLYPH_RIGHT, PRESETS[vs_preset]);
    bool on_banner = vs_cur[0] == -1;
    text_center(buf, 160, 26, on_banner ? C_WHITE : C_GREY);
    if (on_banner) ui_cursor(160 - text_width(buf) / 2 - 10, 26, frame_t);
    uint8_t slot[2][VS_SLOTS];
    if (vs_preset == VS_RANDOM) memcpy(slot, vs_rand, sizeof slot);
    else if (vs_preset == VS_CUSTOM) memcpy(slot, vs_slot, sizeof slot);
    else vs_preset_slots(vs_preset, slot);
    bool custom = vs_preset == VS_CUSTOM;
    for (int p = 0; p < 2; p++) {
        int x0 = p == 0 ? 14 : 202, col = p == 0 ? C_YELLOW : C_MAGENTA;
        ui_panel(x0 - 6, 38, 116, 92, C_INK, col);
        tiny_draw(p == 0 ? "PLAYER 1 - GOLD" : "PLAYER 2 - VIOLET", x0, 42, col);
        draw_vs_pool(p, slot[p], x0, 52, custom);
        int fl = custom ? vs_flags[p] : 3;
        bool sel = custom && vs_cur[p] == VS_SLOTS;
        snprintf(buf, sizeof buf, "%sBANNERS %d%s", sel ? GLYPH_LEFT " " : "", fl, sel ? " " GLYPH_RIGHT : "");
        tiny_draw(buf, x0, 104, sel ? C_WHITE : C_GREY);
        for (int i = 0; i < fl; i++) spr_draw_ex(&bf_spr[BS_FLAG1], x0 + i * 12, 113, p ? SPR_FLIPX : 0, BF_TEAM_MAP[p], -1);
    }
    /* the middle column: what this mode does */
    const char *what = vs_preset == VS_RANDOM ? "EACH SIDE\nDRAWS FROM\nITS OWN\nPOOL.\n\n" GLYPH_DOWN " DEALS\nNEW POOLS"
                     : custom ? (vs_mirrored(slot) ? "MIRRORED\nPOOLS:\nMATCHING\nTROOPS." : "PICK EACH\nPLACE WITH\n" GLYPH_A " AND " GLYPH_B ".")
                     : "BOTH SIDES\nFIELD THE\nSAME TROOPS\nEACH TURN.";
    text_wrap(what, 134, 56, 56, C_LIGHT, 9);
    tiny_center("5 PROMOTIONS CALL A CHAMPION", 160, 138, C_YELLOW);
    text_center(on_banner ? GLYPH_A " FIGHT   " GLYPH_B " BACK" : "START: FIGHT", 160, 148, on_banner ? C_WHITE : C_GREY);
    tiny_center(plat_kind() == PLAT_PC || plat_kind() == PLAT_HEADLESS ? "KEYS: P1 WASD + F/G    P2 ARROWS + K/L    OR TWO PADS"
                                                                       : "TWO GAMEPADS, OR P1 WASD + F/G / P2 ARROWS + K/L",
                160, 168, C_SLATE);
}

static void draw_result(void) {
    draw_play();
    gfx_dither(0, 16, SCREEN_W, 164, C_INK, 10);
    bool win = result_status == BF_WIN_L;
    ui_panel(50, 36, 220, 108, C_NIGHT, win ? C_AMBER : result_status == BF_DRAW ? C_GREY : C_VIOLET);
    static const uint8_t gw[] = {C_CREAM, C_YELLOW, C_AMBER};
    static const uint8_t gl[] = {C_PINK, C_MAGENTA, C_VIOLET};
    static const uint8_t gd[] = {C_WHITE, C_LIGHT, C_GREY};
    char buf[64];
    const char *head;
    if (B.mode == MODE_VERSUS) head = win ? "GOLD WINS!" : result_status == BF_WIN_R ? "VIOLET WINS!" : "DRAW";
    else if (B.mode == MODE_SURVIVAL) head = "OVERRUN";
    else head = win ? "VICTORY!" : result_status == BF_WIN_R ? "DEFEAT" : "DRAW";
    ui_fancy_center(head, 160, 44, 2, win ? gw : result_status == BF_DRAW ? gd : gl, 3, C_INK, C_INK);
    switch (B.mode) {
    case MODE_CAMPAIGN:
        text_center(BF_LEVELS_DEF[cur_level].name, 160, 68, C_LIGHT);
        if (win) snprintf(buf, sizeof buf, "THE MARCH HOLDS. %d/24 WON.", beaten_count());
        else if (result_status == BF_DRAW) snprintf(buf, sizeof buf, "BOTH KEEPS FELL AT ONCE.");
        else snprintf(buf, sizeof buf, "THE HOST TOOK YOUR BANNERS.");
        text_center(buf, 160, 82, C_GREY);
        break;
    case MODE_RANKED:
        snprintf(buf, sizeof buf, "RANK %d  (%s%d)", sv.rank, rank_delta >= 0 ? "+" : "", rank_delta);
        text_center(buf, 160, 70, C_YELLOW);
        text_center(rank_title(sv.rank), 160, 82, C_LIGHT);
        break;
    case MODE_SURVIVAL:
        snprintf(buf, sizeof buf, "SCORE %u", (unsigned)result_score);
        text_center(buf, 160, 70, C_YELLOW);
        snprintf(buf, sizeof buf, new_best ? "A NEW BEST!" : "BEST %u", (unsigned)sv.best_survival);
        text_center(buf, 160, 82, new_best ? C_LIME : C_GREY);
        break;
    default:
        snprintf(buf, sizeof buf, "BANNERS LEFT  %d - %d", B.a[0].flags, B.a[1].flags);
        text_center(buf, 160, 74, C_LIGHT);
        break;
    }
    for (int i = 0; i < 3; i++) ui_goal_icon(138 + i * 16, 100, 1 << i, (g_progress.goals[game_current_index()] >> i) & 1, frame_t);
    if (state_t > 50 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 124, C_WHITE);
}

static void draw_ending(void) {
    draw_sky();
    draw_mini_keep(12, 94, SIDE_L, 3);
    for (int i = 0; i < 6; i++) {
        const Sprite *s = &bf_spr[unit_sprite(U_FOOT + i % 5, (frame_t / 10 + i) & 1)];
        spr_draw_ex(s, 70 + i * 34, 138 - ((frame_t / 8 + i) % 2) * 3, 0, BF_TEAM_MAP[0], -1);
    }
    ui_panel(40, 20, 240, 86, C_NIGHT, C_AMBER);
    static const uint8_t grad[] = {C_CREAM, C_YELLOW, C_AMBER};
    ui_fancy_center("THE MARCH IS FREE", 160, 28, 2, grad, 3, C_INK, C_MAROON);
    text_center("THE THISTLE DUKE LOWERS HIS LAST", 160, 54, C_LIGHT);
    text_center("BANNER AND RIDES HOME HUNGRY.", 160, 64, C_LIGHT);
    text_center("THE VALLEY KEEPS ITS HONEY.", 160, 78, C_YELLOW);
    if (state_t > 120 && (state_t / 20) % 2) tiny_center("PRESS A", 160, 94, C_WHITE);
}

static void draw_sheet(void) {
    gfx_cls(C_JADE);
    int x = 2, y = 2;
    for (int s = 0; s < 2; s++)
        for (int i = 0; i < BS_SPRITE_COUNT; i++) {
            const Sprite *sp = &bf_spr[i];
            if (x + sp->w > 318) { x = 2; y += 20; }
            spr_draw_ex(sp, x, y, s ? SPR_FLIPX : 0, BF_TEAM_MAP[s], -1);
            x += sp->w + 3;
        }
    for (int t = 1; t < U_TYPES; t++) {
        Unit u = {(uint8_t)t, (uint8_t)(t % 2), BF_UNITS[t].hp, (uint8_t)(t > 5), 0};
        draw_unit_at(&u, 2 + (t - 1) * 36, 120, 0, false, false, t == U_FOOT);
        spr_draw_scaled(&bf_spr[unit_sprite(t, 0)], 2 + (t - 1) * 36, 146, 2, 0);
    }
}

static void bf_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_CAMPAIGN: draw_campaign(); break;
    case S_BRIEF: draw_brief(); break;
    case S_PLAY: draw_play(); break;
    case S_RESULT: draw_result(); break;
    case S_RANKED: draw_ranked(); break;
    case S_SURVIVAL: draw_survival_menu(); break;
    case S_VERSUS: draw_versus(); break;
    case S_ENDING: draw_ending(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void bf_load(void) {
    bf_art_load();
    bf_audio_load();
}

static void bf_start(void) {
    rng_seed(&seeds, g_rng.state ^ 0xBA77E5ull);
    load_save();
    memset(parts, 0, sizeof parts);
    sheet_mode = false;
    title_sel = 0;
    two_players = false;
    vs_preset = VS_BEGINNER;
    vs_preset_slots(VS_ADVANCED, vs_slot); /* custom starts from the advanced pools */
    vs_flags[0] = vs_flags[1] = 3;
    vs_reroll();
    check_goals();
    go_title();
}

static void bf_quit(void) {
    input_set_versus(false);
    save_now();
}

static void bf_label(int x, int y, int w, int h, int t) {
    /* the label: two keeps across a meadow, banners flying */
    for (int yy = 0; yy < h; yy++) gfx_hline(x, x + w - 1, y + yy, yy < 18 ? C_BLUE : yy < 30 ? C_SKY : C_JADE);
    gfx_dither(x, y + 30, w, h - 30, C_LEAF, 3);
    for (int k = 0; k < 2; k++) {
        int kx = k ? x + w - 30 : x + 4;
        gfx_rect(kx, y + 18, 26, 26, C_SLATE);
        gfx_dither(kx, y + 18, 26, 26, C_GREY, 3);
        for (int c = 0; c < 3; c++) gfx_rect(kx + 1 + c * 9, y + 15, 6, 3, C_SLATE);
        gfx_rect(kx + 9, y + 32, 8, 12, C_INK);
        spr_draw_ex(&bf_spr[(t / 14 + k) % 2 ? BS_FLAG1 : BS_FLAG2], kx + 9, y + 4, k ? SPR_FLIPX : 0, BF_TEAM_MAP[k], -1);
    }
    int step = (t / 12) % 2;
    spr_draw_ex(&bf_spr[BS_PIKE1 + step], x + 36, y + 34, 0, BF_TEAM_MAP[0], -1);
    spr_draw_ex(&bf_spr[BS_RIDER1 + step], x + 52, y + 36, 0, BF_TEAM_MAP[0], -1);
    spr_draw_ex(&bf_spr[BS_WARD1 + step], x + w - 70, y + 34, SPR_FLIPX, BF_TEAM_MAP[1], -1);
    spr_draw_ex(&bf_spr[BS_FOOT1 + step], x + w - 54, y + 38, SPR_FLIPX, BF_TEAM_MAP[1], -1);
    /* an arrow in flight */
    int ax = x + 46 + (t % 60);
    if (ax < x + w - 70) spr_draw(&bf_spr[BS_ARROW], ax, y + 28, 0);
}

static int bf_query(const char *key, int *out) {
    if (!strcmp(key, "camp_sel")) { *out = level_sel; return 1; }
    if (!strcmp(key, "camp_named")) { *out = level_named(level_sel); return 1; }
    if (!strcmp(key, "camp_new")) { *out = new_troops(level_sel); return 1; }
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "sub")) { *out = sub; return 1; }
    if (!strcmp(key, "plan_t")) { *out = plan_t; return 1; }
    if (!strcmp(key, "status")) { *out = B.status; return 1; }
    if (!strcmp(key, "mode")) { *out = B.mode; return 1; }
    if (!strcmp(key, "turn")) { *out = B.turn; return 1; }
    if (!strcmp(key, "flags_l")) { *out = B.a[SIDE_L].flags; return 1; }
    if (!strcmp(key, "flags_r")) { *out = B.a[SIDE_R].flags; return 1; }
    if (!strcmp(key, "taken_l")) { *out = B.a[SIDE_L].taken; return 1; }
    if (!strcmp(key, "count_l")) { *out = bf_count(&B, SIDE_L, U_NONE); return 1; }
    if (!strcmp(key, "count_r")) { *out = bf_count(&B, SIDE_R, U_NONE); return 1; }
    if (!strcmp(key, "promos_l")) { *out = B.a[SIDE_L].promos; return 1; }
    if (!strcmp(key, "hero_due_l")) { *out = B.a[SIDE_L].hero_due; return 1; }
    if (!strcmp(key, "champs_l")) { *out = bf_count(&B, SIDE_L, U_CHAMP); return 1; }
    if (!strcmp(key, "matched")) { *out = B.matched; return 1; }
    if (!strcmp(key, "handicap")) { *out = B.a[SIDE_R].handicap; return 1; }
    if (!strcmp(key, "timer")) { *out = bf_timer_counts(&B); return 1; }
    if (!strcmp(key, "beaten")) { *out = (int)sv.beaten; return 1; }
    if (!strcmp(key, "won")) { *out = beaten_count(); return 1; }
    if (!strcmp(key, "losses")) { *out = sv.losses; return 1; }
    if (!strcmp(key, "rank")) { *out = sv.rank; return 1; }
    if (!strcmp(key, "best_rank")) { *out = sv.best_rank; return 1; }
    if (!strcmp(key, "score")) { *out = (int)B.score; return 1; }
    if (!strcmp(key, "best_survival")) { *out = (int)sv.best_survival; return 1; }
    if (!strcmp(key, "level")) { *out = cur_level + 1; return 1; }
    if (!strcmp(key, "keep_hits_l")) { *out = B.keep_hits[SIDE_L]; return 1; }
    if (!strcmp(key, "cx")) { *out = hand[0].cx; return 1; }
    if (!strcmp(key, "cy")) { *out = hand[0].cy; return 1; }
    if (!strcmp(key, "grab")) { *out = hand[0].grab; return 1; }
    if (!strcmp(key, "cx2")) { *out = hand[1].cx; return 1; }
    if (!strcmp(key, "cy2")) { *out = hand[1].cy; return 1; }
    if (!strcmp(key, "versus")) { *out = input_versus(); return 1; }
    if (!strcmp(key, "two_players")) { *out = two_players; return 1; }
    if (!strcmp(key, "banner")) { *out = banner_t > 0 ? banner_kind : 0; return 1; }
    if (!strcmp(key, "tip")) { *out = tip_t > 0; return 1; }
    if (!strcmp(key, "vs_preset")) { *out = vs_preset; return 1; }
    if (!strcmp(key, "vs_cur")) { *out = vs_cur[0]; return 1; }
    if (!strcmp(key, "vs_cur2")) { *out = vs_cur[1]; return 1; }
    if (!strcmp(key, "pool_n_l")) { *out = B.a[SIDE_L].pool_n; return 1; }
    if (!strcmp(key, "pool_n_r")) { *out = B.a[SIDE_R].pool_n; return 1; }
    if (!strcmp(key, "timer_left")) { *out = sub == P_PLAN ? (bf_timer_counts(&B) * COUNT_FRAMES - plan_t + COUNT_FRAMES - 1) / COUNT_FRAMES : -1; return 1; }
    if (!strcmp(key, "spawn_mask_l")) { *out = spawn_mask[0]; return 1; }
    if (!strcmp(key, "spawn_mask_r")) { *out = spawn_mask[1]; return 1; }
    if (!strcmp(key, "spawn_total_l")) { *out = spawn_total[0]; return 1; }
    if (!strcmp(key, "spawn_total_r")) { *out = spawn_total[1]; return 1; }
    if (!strncmp(key, "vs_slot", 7) && strlen(key) == 9) {
        /* vs_slotPI: player P's custom place I */
        int pp = key[7] - '0', i = key[8] - '0';
        if (pp < 0 || pp > 1 || i < 0 || i >= VS_SLOTS) return 0;
        *out = vs_slot[pp][i];
        return 1;
    }
    if (!strncmp(key, "vs_rand", 7) && strlen(key) == 9) {
        int pp = key[7] - '0', i = key[8] - '0';
        if (pp < 0 || pp > 1 || i < 0 || i >= VS_SLOTS) return 0;
        *out = vs_rand[pp][i];
        return 1;
    }
    if (!strcmp(key, "vs_flags")) { *out = vs_flags[0]; return 1; }
    if (!strcmp(key, "vs_flags2")) { *out = vs_flags[1]; return 1; }
    /* unit_at XY / hp_at XY / side_at XY / promo_at XY (X and Y single digits) */
    static const char *KEYS[4] = {"unit_at", "hp_at", "side_at", "promo_at"};
    for (int k = 0; k < 4; k++) {
        size_t n = strlen(KEYS[k]);
        if (!strncmp(key, KEYS[k], n) && strlen(key) == n + 2) {
            int x = key[n] - '0', y = key[n + 1] - '0';
            if (x < 0 || x >= BF_COLS || y < 0 || y >= BF_ROWS) return 0;
            const Unit *u = &B.g[y][x];
            *out = k == 0 ? u->type : !u->type ? -1 : k == 1 ? u->hp : k == 2 ? u->side : u->promo;
            return 1;
        }
    }
    return 0;
}

static int bf_cheat(const char *cmd) {
    int a, b2, c, d, e, f;
    if (sscanf(cmd, "battle %d", &a) == 1) {
        /* start campaign battle N straight away */
        cur_level = iclamp(a - 1, 0, BF_LEVELS - 1);
        start_campaign_battle(cur_level);
        return 1;
    }
    if (sscanf(cmd, "battle_seed %d %d", &a, &b2) == 2) {
        /* campaign battle N dealt from a fixed seed (the scripted battles) */
        start_campaign_battle_seeded(iclamp(a - 1, 0, BF_LEVELS - 1), (uint64_t)b2);
        return 1;
    }
    if (!strcmp(cmd, "ranked")) { start_ranked(); return 1; }
    if (!strcmp(cmd, "survival")) { start_survival(); return 1; }
    if (sscanf(cmd, "versus %d", &a) == 1) { vs_preset = iclamp(a, 0, VS_PRESETS - 1); start_versus(); return 1; }
    if (!strcmp(cmd, "clear")) {
        /* an empty field for rule tests; the turn timer stops */
        memset(B.g, 0, sizeof B.g);
        sub = P_PLAN;
        plan_t = -100000;
        return 1;
    }
    if (!strcmp(cmd, "freeze")) { plan_t = -100000; sub = P_PLAN; return 1; }
    int n = sscanf(cmd, "unit %d %d %d %d %d %d", &a, &b2, &c, &d, &e, &f);
    if (n >= 4) {
        if (a < 0 || a >= BF_COLS || b2 < 0 || b2 >= BF_ROWS || c < 0 || c >= U_TYPES) return 0;
        Unit *u = &B.g[b2][a];
        memset(u, 0, sizeof *u);
        if (c) {
            u->type = (uint8_t)c;
            u->side = (uint8_t)d;
            u->hp = (uint8_t)(n >= 5 ? e : BF_UNITS[c].hp);
            u->promo = (uint8_t)(n >= 6 ? f : c == U_CHAMP);
            u->id = B.next_id++;
        }
        return 1;
    }
    if (!strcmp(cmd, "resolve")) {
        /* one end of turn, instantly, with no spawn afterwards */
        rev.n = 0;
        bf_resolve(&B, &rev, NULL);
        return 1;
    }
    if (!strcmp(cmd, "endturn")) { if (sub == P_PLAN) plan_t = bf_timer_counts(&B) * COUNT_FRAMES; return 1; }
    if (!strcmp(cmd, "spawn")) { sev.n = 0; bf_spawn(&B, &sev); return 1; }
    if (sscanf(cmd, "flags %d %d", &a, &b2) == 2) { B.a[0].flags = (uint8_t)a; B.a[1].flags = (uint8_t)b2; return 1; }
    if (sscanf(cmd, "promos %d", &a) == 1) { B.a[0].promos = (uint8_t)a; return 1; }
    if (sscanf(cmd, "turn %d", &a) == 1) { B.turn = (uint16_t)a; return 1; }
    if (sscanf(cmd, "rank %d", &a) == 1) { sv.rank = (uint16_t)a; if (sv.rank > sv.best_rank) sv.best_rank = sv.rank; return 1; }
    if (sscanf(cmd, "beaten %d", &a) == 1) { sv.beaten = (uint32_t)a; save_now(); return 1; }
    if (sscanf(cmd, "seed %d", &a) == 1) { rng_seed(&B.rng, (uint64_t)a); return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    return 0;
}

const GameDef GAME_BANNERFALL = {
    "bannerfall",
    "BANNERFALL",
    "1984",
    "STRATEGY",
    "TWO KEEPS, SIX LANES. DRAG YOUR TROOPS INTO PLACE BEFORE THE DRUMS SOUND.",
    {"WIN THE FIRST 12 BATTLES", "WIN ALL 24 BATTLES", "WIN ALL 24 AND REACH RANK 100"},
    "D-PAD\tMOVE THE HAND (YOUR HALF)\n"
    "HOLD " GLYPH_A "\tGRAB A UNIT, D-PAD DRAGS IT:\n"
    "\tUP, DOWN, BACK; FORWARD ONLY\n"
    "\tTO WHERE YOU PICKED IT UP.\n"
    "\tONTO A FRIEND: THEY SWAP.\n"
    "HOLD " GLYPH_B "\tFAST-FORWARD (1 PLAYER)\n"
    "START\tPAUSE\n"
    "AT 0 ALL ATTACK AND MARCH: KNIVES,\n"
    "ATTACKS, MOVES, KEEPS, RIDERS AGAIN.\n"
    "2P SETUP: " GLYPH_A "/" GLYPH_B " CHANGE A PLACE\n"
    "2P KEYS: WASD+F/G, ARROWS+K/L",
    C_AMBER, C_VIOLET,
    bf_load, bf_start, bf_update, bf_draw, bf_quit, bf_label, bf_query, bf_cheat,
    "ATTACTICS", 9,
};
