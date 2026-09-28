/* BOOMTOWN - set up fireworks round Mossbury's market square and light one
 * fuse a night to blow the bogles back into the marsh.
 * Cartridge 10 of UFO 40, a tribute to Devilition (UFO 50 #10).
 * Every rule and where it comes from is in docs/games/10-boomtown.md. */
#include "boomtown.h"

#define FX 80              /* the square on screen */
#define FY 18
#define TILE 16
#define ROW_X 104          /* the three fireworks on offer */
#define ROW_Y 150
#define SLOT_W 24
#define LIGHT_X 186        /* the LIGHT button */
#define LIGHT_W 52
#define HOLD_CANCEL 24     /* frames B is held to put a firework back */
#define SETTLE 40          /* frames the smoke clears after a chain */
#define SECRET_SECS 355    /* 5:55 on the clock */

enum { S_TITLE, S_STORY, S_NIGHT, S_PICK, S_PLACE, S_AIM, S_CHAIN, S_RESULT, S_OVER, S_WIN };

typedef struct Save {
    uint32_t magic;
    uint32_t best_score;
    uint8_t most_folk, most_left, best_nights, wins;
} Save;
#define SAVE_MAGIC 0x424D0001u

static Save sv;
static BmGame G;
static BmChain CH;
static int state, state_t, frame_t, shake;
static uint32_t run_frames;        /* the clock: the whole run, pauses excluded */
static int sel;                    /* 0..2 a firework on offer, 3 the LIGHT button */
static int cx, cy, pdir, place_slot, bhold;
static bool forced;                /* nothing left to place: a fuse must be lit */
static int result, settle_t, secret_t, final_score, final_bonus;
static bool sheet_mode;
static Rng seeds;

/* effects */
typedef struct { float x, y, vx, vy; int life, col, kind; } Part;
static Part parts[220];
static uint8_t flash[BM_H][BM_W], flash_col[BM_H][BM_W];
static uint8_t born[BM_H][BM_W];   /* frames since a bogle, hole or neighbour appeared */
typedef struct { int active, col, x0, y0, x1, y1, t, perch; } Flight;
static Flight flights[2];
static int hazel_cheer, king_flash;

/* ------------------------------------------------------------------ */
/* save                                                                 */

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
}

static void award_from_save(void) {
    if (sv.best_nights >= 5) game_award(GOAL_BEACON);
    if (sv.wins > 0) game_award(GOAL_SAUCER);
    if (sv.wins > 0 && sv.best_score >= 30000) game_award(GOAL_ALIEN);
}

static void load_save(void) {
    Save tmp;
    memset(&sv, 0, sizeof sv);
    sv.magic = SAVE_MAGIC;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp && tmp.magic == SAVE_MAGIC) sv = tmp;
    award_from_save();
}

/* the run's records (most folk is kept as it happens) */
static void note_records(void) {
    int folk = bm_count(&G, BO_FOLK);
    if (folk > G.folk_most) G.folk_most = (uint8_t)folk;
    if (G.folk_most > sv.most_folk) sv.most_folk = G.folk_most;
}

/* ------------------------------------------------------------------ */
/* effects                                                              */

static void part_add(float x, float y, float vx, float vy, int life, int col, int kind) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) { parts[i] = (Part){x, y, vx, vy, life, col, kind}; return; }
}

static float tile_cx(int x) { return (float)(FX + x * TILE + TILE / 2); }
static float tile_cy(int y) { return (float)(FY + y * TILE + TILE / 2); }

static int piece_colour(int kind, int col) {
    switch (kind) {
    case BM_STARBURST: return C_ORANGE;
    case BM_CANDLE: return C_SKY;
    case BM_ROCKET: case BM_PERCH: return col == 2 ? C_LIME : C_RED;
    case BM_BANGER: return C_YELLOW;
    case BM_PINWHEEL: return C_MAGENTA;
    case BM_JACK: return C_LIME;
    case BM_TWIN: return C_JADE;
    case BM_FOUNTAIN: return C_PINK;
    default: return C_WHITE;
    }
}

static void burst(float x, float y, int col, int n, float speed) {
    for (int i = 0; i < n; i++) {
        float a = (float)i / (float)n * 6.2832f + (float)(frame_t % 7) * 0.1f;
        part_add(x, y, cosf(a) * speed, sinf(a) * speed - 0.3f, 18 + i % 7, i % 3 ? col : C_WHITE, 0);
    }
}

static void fx_fire(int x, int y, int kind, int dir, int col) {
    (void)dir;
    burst(tile_cx(x), tile_cy(y), piece_colour(kind, col), kind == BM_ROCKET ? 6 : 14, 1.6f);
    flash[y][x] = 10;
    flash_col[y][x] = (uint8_t)C_WHITE;
    shake = imax(shake, 3);
    sfx_play_name(kind == BM_ROCKET ? "bm_whoosh" : "bm_bang");
}

static void fx_hit(int x, int y, int what, int dead) {
    flash[y][x] = 12;
    flash_col[y][x] = (uint8_t)C_YELLOW;
    part_add(tile_cx(x) + (float)((frame_t * 7 + x * 3) % 7 - 3), tile_cy(y), 0, -0.4f, 14, C_AMBER, 0);
    if (what == BO_BOGLE) {
        if (dead) { burst(tile_cx(x), tile_cy(y), C_JADE, 10, 1.1f); sfx_play_name("bm_pop"); }
        else sfx_play_name("bm_hurt");
    } else if (what == BO_FOLK) {
        /* a neighbour blown clean out of town (the hat goes last) */
        part_add(tile_cx(x), tile_cy(y), 0.5f, -2.2f, 70, C_BROWN, 2);
        sfx_play_name("bm_folk");
    } else if (what == BO_KING) {
        king_flash = 8;
        shake = imax(shake, 5);
        sfx_play_name("bm_king_hit");
    }
}

static void fx_launch(int x, int y, int col, int to_x, int to_y, int has_perch) {
    Flight *f = &flights[col == 2 ? 1 : 0];
    *f = (Flight){1, col, x, y, to_x, to_y, 0, has_perch};
}

static const BmFx FX_CALLS = {fx_fire, fx_hit, fx_launch};

/* ------------------------------------------------------------------ */
/* flow                                                                 */

static void enter(int s) {
    state = s;
    state_t = 0;
    bhold = 0;
}

static void begin_night(void) {
    /* remember what was there so the new arrivals can climb out */
    BmCell before[BM_H][BM_W];
    memcpy(before, G.c, sizeof before);
    bm_round_begin(&G);
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++)
            born[y][x] = G.c[y][x].occ != before[y][x].occ ? 0 : 255;
    enter(S_NIGHT);
    music_play(G.round >= BM_ROUNDS ? BM_MUS_KING : BM_MUS_NIGHT);
    sfx_play_name(G.round >= BM_ROUNDS ? "bm_king_hit" : "bm_spawn");
}

static void new_game(void) {
    uint64_t seed = rng_next(&seeds) ^ ((uint64_t)rng_next(&seeds) << 32);
    bm_new_game(&G, seed);
    memset(born, 0, sizeof born);
    memset(parts, 0, sizeof parts);
    memset(flash, 0, sizeof flash);
    memset(flights, 0, sizeof flights);
    run_frames = 0;
    secret_t = 0;
    final_score = 0;
    final_bonus = 0;
    sel = 0;
    cx = BM_W / 2;
    cy = BM_H / 2;
    pdir = BD_UP;
    game_set_pausable(true);
    enter(S_NIGHT);
    music_play(BM_MUS_NIGHT);
}

/* the next thing to do once a firework is down: with nothing left to place
 * and fireworks on the square, straight to choosing the fuse */
static void to_pick(void) {
    forced = G.hand <= 0 && bm_row_count(&G) == 0 && bm_pieces_on_board(&G) > 0;
    if (forced) { sel = BM_ROW; enter(S_AIM); return; }
    enter(S_PICK);
    if (sel < BM_ROW && !G.row[sel]) {
        int s = -1;
        for (int i = 0; i < BM_ROW && s < 0; i++)
            if (G.row[i]) s = i;
        sel = s < 0 ? BM_ROW : s;
    }
}

static void end_night(void) {
    BmCell before[BM_H][BM_W];
    memcpy(before, G.c, sizeof before);
    result = bm_round_end(&G);
    /* a wounded old bogle heals; a new neighbour climbs out of their door */
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++) {
            if (G.c[y][x].occ == BO_BOGLE && G.c[y][x].hp > before[y][x].hp) {
                flash[y][x] = 12;
                flash_col[y][x] = (uint8_t)C_LIME;
                burst(tile_cx(x), tile_cy(y), C_LIME, 6, 0.7f);
                sfx_play_name("bm_heal");
            }
            if (G.c[y][x].occ == BO_FOLK && before[y][x].occ != BO_FOLK) born[y][x] = 0;
        }
    note_records();
    enter(S_RESULT);
    switch (result) {
    case BR_HOLDS: case BR_CLEARED:
        if (G.round > sv.best_nights) sv.best_nights = G.round;
        if (G.round >= 5) game_award(GOAL_BEACON); /* five nights held */
        if (result == BR_CLEARED) sfx_play_name("bm_newfolk");
        hazel_cheer = 90;
        music_restart(BM_MUS_HOLD);
        break;
    case BR_WON: {
        sv.best_nights = BM_ROUNDS;
        final_score = bm_score(&G, run_frames, &final_bonus);
        sv.wins = (uint8_t)imin(255, sv.wins + 1);
        if ((uint32_t)final_score > sv.best_score) sv.best_score = (uint32_t)final_score;
        int left = bm_pieces_on_board(&G) + G.hand;
        if (left > sv.most_left) sv.most_left = (uint8_t)imin(255, left);
        game_award(GOAL_SAUCER);
        if (final_score >= 30000) game_award(GOAL_ALIEN);
        music_restart(BM_MUS_HOLD);
        hazel_cheer = 200;
        break;
    }
    default:
        music_restart(BM_MUS_OVER);
        shake = 16;
        break;
    }
    save_now();
}

static void light(int x, int y) {
    bm_chain_start(&G, &CH, x, y);
    enter(S_CHAIN);
    settle_t = 0;
    sfx_play_name("bm_fuse");
    /* a secret: light the fuse when the clock reads 5:55 */
    if (run_frames / 60u == SECRET_SECS) { secret_t = 300; sfx_play_name("bm_secret"); }
}

/* ------------------------------------------------------------------ */
/* input                                                                */

static void move_cursor(void) {
    int ox = cx, oy = cy;
    if (btn_repeat(BTN_LEFT)) cx--;
    if (btn_repeat(BTN_RIGHT)) cx++;
    if (btn_repeat(BTN_UP)) cy--;
    if (btn_repeat(BTN_DOWN)) cy++;
    cx = iclamp(cx, 0, BM_W - 1);
    cy = iclamp(cy, 0, BM_H - 1);
    if (cx != ox || cy != oy) sfx_play_name("bm_move");
}

static bool nothing_left(void) {
    return G.hand <= 0 && bm_row_count(&G) == 0 && bm_pieces_on_board(&G) == 0;
}

static void pick_update(void) {
    /* no fireworks left at all: the night ends by itself */
    if (nothing_left()) {
        if (state_t > 20) end_night();
        return;
    }
    int old = sel;
    if (btn_repeat(BTN_LEFT)) {
        for (int s = sel - 1; s >= 0; s--)
            if (G.row[s]) { sel = s; break; }
    }
    if (btn_repeat(BTN_RIGHT)) {
        int s = sel + 1;
        while (s < BM_ROW && !G.row[s]) s++;
        sel = imin(s, BM_ROW);
    }
    if (sel != old) sfx_play_name("bm_move");
    if (btnp(BTN_A)) {
        if (sel < BM_ROW && G.row[sel]) {
            place_slot = sel;
            pdir = BD_UP;
            sfx_play_name("bm_pick");
            enter(S_PLACE);
        } else if (sel == BM_ROW) {
            if (bm_pieces_on_board(&G) > 0) {
                sfx_play_name("bm_pick");
                forced = false;
                enter(S_AIM);
            } else {
                sfx_play_name("bm_nope");
            }
        }
    }
}

static void place_update(void) {
    move_cursor();
    int kind = G.row[place_slot];
    if (btnp(BTN_B) && bm_turns(kind)) {
        pdir = (pdir + 1) & 3;
        sfx_play_name("bm_turn");
    }
    if (btn(BTN_B)) {
        if (++bhold >= HOLD_CANCEL) {
            /* held: the firework goes back on offer */
            sfx_play_name("bm_cancel");
            enter(S_PICK);
            return;
        }
    } else {
        bhold = 0;
    }
    if (btnp(BTN_A)) {
        if (!bm_free(&G, cx, cy)) { sfx_play_name("bm_nope"); return; }
        bm_place(&G, place_slot, cx, cy, pdir);
        sfx_play_name("bm_place");
        part_add(tile_cx(cx), tile_cy(cy) + 5, -0.4f, -0.2f, 10, C_GREY, 0);
        part_add(tile_cx(cx), tile_cy(cy) + 5, 0.4f, -0.2f, 10, C_GREY, 0);
        to_pick();
    }
}

static void aim_update(void) {
    move_cursor();
    if (btnp(BTN_B) && !forced) { sfx_play_name("bm_cancel"); enter(S_PICK); return; }
    if (btnp(BTN_A)) {
        if (G.c[cy][cx].occ == BO_PIECE) light(cx, cy);
        else sfx_play_name("bm_nope");
    }
}

static void chain_update(void) {
    if (CH.n > 0 || CH.now == 0) {
        bm_chain_tick(&G, &CH, &FX_CALLS);
        return;
    }
    if (++settle_t >= SETTLE) end_night();
}

static void result_update(void) {
    int wait = result == BR_WON ? 150 : 110;
    if (state_t < wait && !(state_t > 30 && btnp(BTN_A))) return;
    switch (result) {
    case BR_HOLDS: case BR_CLEARED:
        begin_night();
        break;
    case BR_WON:
        enter(S_WIN);
        music_restart(BM_MUS_END);
        game_set_pausable(false);
        break;
    default:
        enter(S_OVER);
        game_set_pausable(false);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* the demo player: it chooses real buttons each frame for the runner's */
/* "bot" query (botplay / botuntil in the tests)                        */

enum { PLAN_NONE, PLAN_PLACE, PLAN_LIGHT };
static struct { int kind, slot, x, y, dir, valid, tick; } plan;
static int bot_reserve = 4; /* fireworks it keeps back for the last night */

typedef struct { int value, x, y, ok, bogles, folk, reach; } Outcome;

/* How good is a night that ends like this? */
static int judge(const BmGame *g, const BmChain *ch, int *ok) {
    if (g->round >= BM_ROUNDS) {
        *ok = g->king_hp == 0;
        return (BM_BOSS_HP - g->king_hp) * 100 - ch->folk_lost * 5 + (*ok ? 5000 : 0);
    }
    int bogles = 0, hp = 0, folk = bm_count(g, BO_FOLK);
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++)
            if (g->c[y][x].occ == BO_BOGLE) { bogles++; hp += g->c[y][x].kind == BG_OLD ? 2 : g->c[y][x].hp; }
    *ok = bogles <= folk;
    int v = -hp * 30 + folk * 80;
    if (*ok) v += 1000;
    if (bogles == 0) v += 300;
    return v;
}

/* How close does a chain come to what is still standing? Free tiles its
 * blasts reach, next to a bogle (or the Bog King), are where the next
 * firework can take it further. */
static int reach(const BmGame *after, const BmChain *ch) {
    int v = 0;
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++) {
            if (!ch->hitmap[y][x] || after->c[y][x].occ != BO_EMPTY) continue;
            int near = 0;
            for (int oy = -2; oy <= 2; oy++)
                for (int ox = -2; ox <= 2; ox++) {
                    int ax = x + ox, ay = y + oy;
                    if (ax < 0 || ay < 0 || ax >= BM_W || ay >= BM_H) continue;
                    int o = after->c[ay][ax].occ;
                    if (o == BO_BOGLE || o == BO_KING) near += (iabs(ox) <= 1 && iabs(oy) <= 1) ? 3 : 1;
                }
            v += imin(near, 6);
        }
    return v;
}

/* the best fuse to light on this square, and how the night would end */
static Outcome best_light(const BmGame *g) {
    Outcome best = {-1000000, -1, -1, 0, 0, 0, 0};
    int best_keep = 0;
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++) {
            if (g->c[y][x].occ != BO_PIECE) continue;
            static BmGame t;
            static BmChain ch;
            t = *g;
            bm_chain_start(&t, &ch, x, y);
            bm_chain_run(&t, &ch);
            int ok;
            int v = judge(&t, &ch, &ok);
            int r = reach(&t, &ch), keep = bm_pieces_on_board(&t); /* fireworks kept for later break ties */
            if (v * 64 + r * 4 + keep > best.value * 64 + best.reach * 4 + best_keep) {
                best_keep = keep;
                best = (Outcome){v, x, y, ok, bm_count(&t, BO_BOGLE), bm_count(&t, BO_FOLK), r};
            }
        }
    return best;
}

/* a firework on its own, before anything joins it up */
static int promise(const BmGame *g, int x, int y, int kind, int dir) {
    int8_t t[BM_W + BM_H + 8][2];
    int n = bm_attack(g, x, y, kind, dir, t), v = 0;
    for (int i = 0; i < n; i++) {
        const BmCell *c = &g->c[t[i][1]][t[i][0]];
        if (c->occ == BO_BOGLE) v += 10 * c->hp;
        else if (c->occ == BO_FOLK) v -= 40;
        else if (c->occ == BO_KING) v += 30;
    }
    return v;
}

static int place_value(const BmGame *after, int had_pieces, const BmGame *before, int x, int y, int kind, int dir) {
    Outcome o = best_light(after);
    int v = o.value * 16 + o.reach * 6;
    if (!had_pieces) v += promise(before, x, y, kind, dir) * 4;
    return v;
}

/* the best place for the firework in `slot` of square g */
static int best_spot(const BmGame *g, int slot, int *bx, int *by, int *bd) {
    int kind = g->row[slot], best = -100000000;
    int had = bm_pieces_on_board(g) > 0;
    *bx = -1;
    *bd = 0;
    if (kind == BM_ROCKET) {
        /* the rocket goes where the chain will light it; its perch is what counts */
        Outcome now = best_light(g);
        static BmChain ch;
        static BmGame r;
        r = *g;
        memset(&ch, 0, sizeof ch);
        if (now.x >= 0) { bm_chain_start(&r, &ch, now.x, now.y); bm_chain_run(&r, &ch); }
        for (int y = 0; y < BM_H; y++)
            for (int x = 0; x < BM_W; x++)
                if (bm_free(g, x, y)) {
                    int v = (ch.hitmap[y][x] ? 100 : 0) - iabs(x * 2 - BM_W) - iabs(y * 2 - BM_H);
                    if (v > best) { best = v; *bx = x; *by = y; }
                }
        if (*bx < 0) return best;
        r = *g;
        bm_place(&r, slot, *bx, *by, 0);
        for (int s = 0; s < BM_ROW; s++)
            if (r.row[s] == BM_PERCH) {
                int px, py, pd;
                return best_spot(&r, s, &px, &py, &pd) - 40;
            }
        return -100000; /* nothing left to pay for the perch tonight */
    }
    int dirs = kind == BM_TWIN ? 2 : bm_turns(kind) ? 4 : 1;
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++) {
            if (!bm_free(g, x, y)) continue;
            for (int d = 0; d < dirs; d++) {
                static BmGame t;
                t = *g;
                bm_place(&t, slot, x, y, d);
                int v = place_value(&t, had, g, x, y, kind, d);
                if (v > best) { best = v; *bx = x; *by = y; *bd = d; }
            }
        }
    return best;
}

static void bot_think(void) {
    memset(&plan, 0, sizeof plan);
    plan.valid = 1;
    Outcome now = best_light(&G);
    bool last = G.round >= BM_ROUNDS;
    bool have_row = bm_row_count(&G) > 0;
    bool spare = G.hand > bot_reserve || last;
    if (now.x >= 0 && (!have_row || (last ? now.ok : now.ok && (now.bogles == 0 || !spare)))) {
        plan.kind = PLAN_LIGHT;
        plan.x = now.x;
        plan.y = now.y;
        return;
    }
    int best = -1000000;
    for (int s = 0; s < BM_ROW; s++) {
        if (!G.row[s]) continue;
        int x, y, d, v = best_spot(&G, s, &x, &y, &d);
        if (x >= 0 && v > best) { best = v; plan.kind = PLAN_PLACE; plan.slot = s; plan.x = x; plan.y = y; plan.dir = d; }
    }
    /* nothing helps any more: light up what there is */
    if (now.x >= 0 && now.ok && best <= now.value * 16 + now.reach * 6 && !last) plan.kind = PLAN_NONE;
    if (plan.kind == PLAN_NONE && now.x >= 0) { plan.kind = PLAN_LIGHT; plan.x = now.x; plan.y = now.y; }
}

static int bot_steer(int tx, int ty) {
    if (cx < tx) return BTN_RIGHT;
    if (cx > tx) return BTN_LEFT;
    if (cy < ty) return BTN_DOWN;
    if (cy > ty) return BTN_UP;
    return BTN_A;
}

static int bot_buttons(void) {
    if (++plan.tick & 1) return 0; /* let go between presses */
    switch (state) {
    case S_TITLE: case S_STORY: case S_OVER:
        return state_t > 20 ? BTN_A : 0;
    case S_WIN:
        return state_t > 320 ? BTN_A : 0;
    case S_NIGHT: case S_RESULT: case S_CHAIN:
        plan.valid = 0;
        return state != S_CHAIN && state_t > 30 ? BTN_A : 0;
    case S_PICK:
        if (!plan.valid) bot_think();
        if (plan.kind == PLAN_LIGHT) return sel < BM_ROW ? BTN_RIGHT : BTN_A;
        if (plan.kind == PLAN_PLACE) return sel < plan.slot ? BTN_RIGHT : sel > plan.slot ? BTN_LEFT : BTN_A;
        return 0;
    case S_PLACE: {
        if (!plan.valid || plan.kind != PLAN_PLACE || plan.slot != place_slot) {
            memset(&plan, 0, sizeof plan);
            plan.valid = 1;
            plan.kind = PLAN_PLACE;
            plan.slot = place_slot;
            best_spot(&G, place_slot, &plan.x, &plan.y, &plan.dir);
        }
        if (pdir != plan.dir && bm_turns(G.row[place_slot])) return BTN_B;
        int b = bot_steer(plan.x, plan.y);
        if (b == BTN_A) plan.valid = 0; /* think again after this one */
        return b;
    }
    case S_AIM:
        if (!plan.valid || plan.kind != PLAN_LIGHT) {
            Outcome o = best_light(&G);
            plan.kind = PLAN_LIGHT;
            plan.x = o.x;
            plan.y = o.y;
            plan.valid = 1;
        }
        return bot_steer(plan.x, plan.y);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* update                                                               */

static void tick_effects(void) {
    if (shake > 0) shake--;
    if (hazel_cheer > 0) hazel_cheer--;
    if (king_flash > 0) king_flash--;
    if (secret_t > 0) secret_t--;
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vy += p->kind == 2 ? 0.03f : 0.05f;
    }
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++) {
            if (flash[y][x]) flash[y][x]--;
            if (born[y][x] < 255) born[y][x]++;
        }
    for (int i = 0; i < 2; i++)
        if (flights[i].active && ++flights[i].t > BM_ROCKET_FLIGHT + 4) flights[i].active = 0;
}

static void bm_update(void) {
    state_t++;
    frame_t++;
    tick_effects();
    bool clock = state >= S_NIGHT && state <= S_RESULT;
    if (clock) run_frames++;
    switch (state) {
    case S_TITLE:
        game_set_pausable(false);
        if (btnp(BTN_B)) { game_exit_to_library(); break; }
        if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) {
            sfx_play_name("ui_ok");
            input_consume();
            enter(S_STORY);
        }
        break;
    case S_STORY:
        if (state_t > 10 && (btnp(BTN_A) || btnp(BTN_START))) {
            input_consume();
            new_game();
        }
        break;
    case S_NIGHT:
        if (state_t >= 90 || (state_t > 30 && btnp(BTN_A))) {
            input_consume();
            to_pick();
        }
        break;
    case S_PICK: pick_update(); break;
    case S_PLACE: place_update(); break;
    case S_AIM: aim_update(); break;
    case S_CHAIN: chain_update(); break;
    case S_RESULT: result_update(); break;
    case S_OVER:
        if (state_t > 60 && (btnp(BTN_A) || btnp(BTN_START))) { enter(S_TITLE); music_play(BM_MUS_TITLE); }
        break;
    case S_WIN:
        if (state_t % 23 == 0) {
            int c[5] = {C_RED, C_YELLOW, C_SKY, C_LIME, C_PINK};
            burst((float)(40 + (state_t * 37) % 240), (float)(20 + (state_t * 13) % 60), c[(state_t / 23) % 5], 18, 1.4f);
        }
        if (state_t > 320 && (btnp(BTN_A) || btnp(BTN_START))) { enter(S_TITLE); music_play(BM_MUS_TITLE); }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */

static void draw_cobbles(void) {
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++) {
            int px = FX + x * TILE, py = FY + y * TILE;
            int base = (x + y) & 1 ? C_SLATE : C_DUSK;
            gfx_rect(px, py, TILE, TILE, base);
            /* two rows of stones per tile */
            int off = (y & 1) * 4;
            gfx_hline(px, px + TILE - 1, py + 7, C_NIGHT);
            gfx_hline(px, px + TILE - 1, py + 15, C_NIGHT);
            gfx_vline(px + ((off + 5) & 15), py, py + 6, C_NIGHT);
            gfx_vline(px + ((off + 13) & 15), py + 8, py + 14, C_NIGHT);
            gfx_pset(px + 2 + off, py + 2, base == C_SLATE ? C_GREY : C_SLATE);
            gfx_pset(px + 9, py + 10, base == C_SLATE ? C_GREY : C_SLATE);
        }
}

static void draw_hole(int px, int py, int look, int age) {
    int r = age < 12 ? age / 2 : 6;
    gfx_circ(px + 8, py + 8, r + 1, C_EARTH);
    gfx_circ(px + 8, py + 9, r, C_INK);
    gfx_dither_circle(px + 8, py + 10, r - 1, C_TEAL, 5);
    if (r >= 6) {
        /* reeds and a slow bubble */
        gfx_vline(px + 2 + look, py + 3, py + 7, C_FOREST);
        gfx_pset(px + 2 + look, py + 2, C_BROWN);
        gfx_vline(px + 13 - look, py + 5, py + 9, C_FOREST);
        if ((frame_t / 20 + look) % 5 == 0) gfx_circb(px + 7 + look % 3, py + 9, 1, C_JADE);
    }
}

static void draw_king(int bx, int by) {
    int px = FX + bx * TILE, py = FY + by * TILE;
    int bob = (frame_t / 16) % 2;
    int body = king_flash && (king_flash / 2) % 2 ? C_WHITE : C_FOREST;
    gfx_circ(px + 16, py + 22, 15, C_INK);
    gfx_circ(px + 16, py + 22 - bob, 14, body);
    gfx_dither_circle(px + 16, py + 24 - bob, 12, C_TEAL, 6);
    gfx_rect(px + 2, py + 24, 28, 8, C_INK);
    gfx_rect(px + 3, py + 24, 26, 7, body);
    /* the reed crown */
    for (int i = 0; i < 5; i++) {
        int rx = px + 7 + i * 4, top = py + 2 + (i % 2) * 2 - bob;
        gfx_vline(rx, top, py + 10 - bob, C_AMBER);
        gfx_rect(rx - 1, top, 3, 3, C_BROWN);
    }
    /* eyes and mouth */
    int glow = (frame_t / 10) % 2 ? C_YELLOW : C_AMBER;
    gfx_circ(px + 10, py + 15 - bob, 3, glow);
    gfx_circ(px + 22, py + 15 - bob, 3, glow);
    gfx_rect(px + 10, py + 14 - bob, 2, 3, C_RED);
    gfx_rect(px + 21, py + 14 - bob, 2, 3, C_RED);
    gfx_rect(px + 8, py + 22 - bob, 16, 4, C_INK);
    for (int i = 0; i < 4; i++) gfx_rect(px + 9 + i * 4, py + 22 - bob, 2, 2, C_CREAM);
    /* mud dripping */
    for (int i = 0; i < 3; i++) {
        int d = (frame_t / 6 + i * 5) % 12;
        gfx_pset(px + 6 + i * 10, py + 26 + d / 3, C_EARTH);
    }
    /* the health bar */
    gfx_rect(px, py - 5, 32, 4, C_INK);
    for (int i = 0; i < BM_BOSS_HP; i++) gfx_rect(px + 1 + i * 3, py - 4, 2, 2, i < G.king_hp ? C_RED : C_MAROON);
}

static void draw_things(void) {
    uint8_t m[PAL_COUNT];
    bool king_drawn = false;
    for (int y = 0; y < BM_H; y++)
        for (int x = 0; x < BM_W; x++) {
            const BmCell *c = &G.c[y][x];
            int px = FX + x * TILE, py = FY + y * TILE;
            int age = born[y][x];
            switch (c->occ) {
            case BO_HOLE: draw_hole(px, py, c->look, age); break;
            case BO_FOLK: {
                int bob = ((frame_t / 20) + x + y) % 2;
                gfx_dither(px + 3, py + 13, 10, 2, C_INK, 8);
                /* a lantern glow */
                gfx_dither_circle(px + 8, py + 8, 7, C_AMBER, 2);
                int climb = age < 16 ? 16 - age : 0;
                gfx_clip(px, py, TILE, TILE);
                spr_draw(&bm_spr[BS_FOLK1 + (c->look & 3)], px + 2, py + 1 - bob / 2 + climb, (x + frame_t / 90) % 2 ? SPR_FLIPX : 0);
                gfx_noclip();
                break;
            }
            case BO_BOGLE: {
                int f = (frame_t / 12 + x * 3 + y) % 2;
                int id = c->kind == BG_SMALL ? BS_SMALL1 : c->kind == BG_BIG ? BS_BIG1 : BS_OLD1;
                int w = c->kind == BG_SMALL ? 12 : 14;
                pal_identity(m);
                if (c->kind == BG_BIG && c->hp < 2) { /* hurt: it goes red */
                    m[C_VIOLET] = C_RED; m[C_PURPLE] = C_WINE; m[C_CREAM] = C_ORANGE;
                }
                if (c->kind == BG_OLD && c->hp < 2) { m[C_TEAL] = C_EARTH; m[C_LIGHT] = C_GREY; }
                int climb = age < 16 ? 16 - age : 0;
                gfx_clip(px, py, TILE, TILE);
                spr_draw_ex(&bm_spr[id + f], px + (TILE - w) / 2, py + TILE - (c->kind == BG_SMALL ? 13 : 15) + climb, 0, m, -1);
                gfx_noclip();
                break;
            }
            case BO_PIECE: bm_draw_piece(px, py, c->kind, c->dir, c->col, frame_t + x * 5 + y * 3); break;
            case BO_KING:
                if (!king_drawn) { king_drawn = true; draw_king(x, y); }
                break;
            }
            if (flash[y][x]) {
                gfx_dither(px, py, TILE, TILE, flash_col[y][x], flash[y][x]);
                if (flash[y][x] > 6) gfx_rectb(px + 1, py + 1, TILE - 2, TILE - 2, C_WHITE);
            }
        }
}

/* the tiles the firework being placed will hit */
static void draw_preview(int kind, int dir) {
    int8_t t[BM_W + BM_H + 8][2];
    int n = bm_attack(&G, cx, cy, kind, dir, t);
    for (int i = 0; i < n; i++) {
        int px = FX + t[i][0] * TILE, py = FY + t[i][1] * TILE;
        gfx_dither(px + 1, py + 1, TILE - 2, TILE - 2, C_YELLOW, 5 + (frame_t / 8) % 2 * 2);
        gfx_rectb(px + 1, py + 1, TILE - 2, TILE - 2, C_AMBER);
    }
}

static void draw_cursor(int col) {
    int px = FX + cx * TILE, py = FY + cy * TILE;
    int o = (frame_t / 12) % 2;
    int x0 = px - 1 - o, y0 = py - 1 - o, x1 = px + TILE + o, y1 = py + TILE + o;
    gfx_hline(x0, x0 + 4, y0, col); gfx_vline(x0, y0, y0 + 4, col);
    gfx_hline(x1 - 4, x1, y0, col); gfx_vline(x1, y0, y0 + 4, col);
    gfx_hline(x0, x0 + 4, y1, col); gfx_vline(x0, y1 - 4, y1, col);
    gfx_hline(x1 - 4, x1, y1, col); gfx_vline(x1, y1 - 4, y1, col);
}

static void draw_flights(void) {
    for (int i = 0; i < 2; i++) {
        Flight *f = &flights[i];
        if (!f->active) continue;
        int half = BM_ROCKET_FLIGHT / 2;
        float x, y;
        if (f->t < half) { /* up and away */
            float k = (float)f->t / (float)half;
            x = tile_cx(f->x0);
            y = tile_cy(f->y0) - k * (tile_cy(f->y0) + 20.0f);
        } else if (f->perch) { /* and down on the perch */
            float k = (float)(f->t - half) / (float)half;
            x = tile_cx(f->x1);
            y = -20.0f + k * (tile_cy(f->y1) + 20.0f);
        } else {
            continue;
        }
        int col = f->col == 2 ? C_LIME : C_RED;
        gfx_rect((int)x - 1, (int)y - 4, 3, 7, col);
        gfx_pset((int)x, (int)y - 5, C_WHITE);
        bool up = f->t < half;
        for (int k = 1; k < 5; k++) gfx_pset((int)x + (k % 2), (int)y + (up ? 3 + k * 2 : -5 - k * 2), k < 3 ? C_YELLOW : C_ORANGE);
    }
}

static void draw_parts(void) {
    for (int i = 0; i < ARRAY_LEN(parts); i++) {
        Part *p = &parts[i];
        if (p->life <= 0) continue;
        if (p->kind == 2) {
            /* a flying hat */
            gfx_rect((int)p->x - 3, (int)p->y, 7, 2, C_BROWN);
            gfx_rect((int)p->x - 2, (int)p->y - 3, 5, 3, C_BROWN);
            gfx_hline((int)p->x - 2, (int)p->x + 2, (int)p->y - 1, C_INK);
        } else {
            gfx_rect((int)p->x, (int)p->y, p->life > 8 ? 2 : 1, p->life > 8 ? 2 : 1, p->col);
        }
    }
}

static const char *piece_name(int kind) {
    switch (kind) {
    case BM_STARBURST: return "STARBURST";
    case BM_CANDLE: return "ROMAN CANDLE";
    case BM_ROCKET: return "SKYROCKET";
    case BM_PERCH: return "ROCKET PERCH";
    case BM_BANGER: return "BANGER";
    case BM_PINWHEEL: return "PINWHEEL";
    case BM_JACK: return "JUMPING JACK";
    case BM_TWIN: return "TWIN TUBE";
    case BM_FOUNTAIN: return "FOUNTAIN";
    default: return "";
    }
}

static void draw_row(void) {
    char buf[16];
    /* how many fireworks are left in the cart */
    snprintf(buf, sizeof buf, "%d", G.hand);
    gfx_rect(FX, ROW_Y + 2, 20, 20, C_NIGHT);
    gfx_rectb(FX, ROW_Y + 2, 20, 20, C_DUSK);
    tiny_center("LEFT", FX + 10, ROW_Y + 4, C_SLATE);
    text_center(buf, FX + 10, ROW_Y + 11, G.hand > 0 ? C_WHITE : C_RED);
    for (int i = 0; i < BM_ROW; i++) {
        int x = ROW_X + i * SLOT_W, y = ROW_Y;
        bool on = (state == S_PICK && sel == i) || ((state == S_PLACE) && place_slot == i);
        ui_panel(x, y, 22, 24, C_NIGHT, on ? C_YELLOW : C_DUSK);
        if (G.row[i] && !(state == S_PLACE && place_slot == i && (frame_t / 6) % 2))
            bm_draw_piece(x + 3, y + 4, G.row[i], BD_UP, G.row_col[i], frame_t + i * 7);
    }
    if (G.perch_owed) {
        int x = ROW_X + BM_ROW * SLOT_W - 8;
        tiny_draw("+", x + 1, ROW_Y + 1, C_RED);
    }
    bool on = state == S_PICK && sel == BM_ROW;
    bool can = bm_pieces_on_board(&G) > 0;
    ui_panel(LIGHT_X, ROW_Y, LIGHT_W, 24, on ? C_WINE : C_NIGHT, on ? C_YELLOW : C_DUSK);
    /* a match */
    gfx_rect(LIGHT_X + 6, ROW_Y + 12, 8, 2, C_TAN);
    gfx_rect(LIGHT_X + 13, ROW_Y + 11, 3, 4, C_RED);
    if (on && (frame_t / 4) % 2) gfx_pset(LIGHT_X + 16, ROW_Y + 10, C_YELLOW);
    text_draw("LIGHT", LIGHT_X + 19, ROW_Y + 9, can ? (on ? C_WHITE : C_LIGHT) : C_SLATE);
}

/* A small 5 x 5 diagram of what a firework hits, the piece in the middle,
 * for the one highlighted in the row (or held). */
static void draw_diagram(int x0, int y0, int kind, int dir, int col) {
    const int C = 5;
    int pc = piece_colour(kind, col);
    gfx_rect(x0 - 1, y0 - 1, C * 5 + 2, C * 5 + 2, C_DUSK);
    for (int gy = 0; gy < 5; gy++)
        for (int gx = 0; gx < 5; gx++) gfx_rect(x0 + gx * C, y0 + gy * C, C - 1, C - 1, C_INK);
    /* the pattern from the middle of the real square, cut to 2 tiles round */
    int8_t t[BM_W + BM_H + 8][2];
    int n = bm_attack(&G, 5, 4, kind, dir, t);
    for (int i = 0; i < n; i++) {
        int ox = t[i][0] - 5, oy = t[i][1] - 4;
        if (ox < -2 || ox > 2 || oy < -2 || oy > 2) continue;
        gfx_rect(x0 + (ox + 2) * C, y0 + (oy + 2) * C, C - 1, C - 1, pc);
    }
    int mx = x0 + 2 * C, my = y0 + 2 * C;
    gfx_rect(mx, my, C - 1, C - 1, C_WHITE);
    if (kind == BM_CANDLE) {
        /* the line runs on to the edge: an arrow past the grid */
        static const int DXS[4] = {0, 1, 0, -1}, DYS[4] = {-1, 0, 1, 0};
        int ax = mx + 2 + DXS[dir & 3] * 15, ay = my + 2 + DYS[dir & 3] * 15;
        for (int k = 0; k < 3; k++) {
            int px = ax + DXS[dir & 3] * k, py = ay + DYS[dir & 3] * k;
            int w = 2 - k;
            if (DXS[dir & 3]) gfx_vline(px, py - w, py + w, pc);
            else gfx_hline(px - w, px + w, py, pc);
        }
    } else if (kind == BM_ROCKET) {
        /* up it goes, and down on its perch */
        gfx_vline(mx + 2, y0 - 1, my - 1, pc);
        gfx_hline(mx + 1, mx + 3, y0, pc);
        gfx_pset(mx + 2, y0 - 2, pc);
        tiny_draw("PERCH", x0 + C * 5 + 4, y0 + 2, C_SLATE);
        gfx_circb(x0 + C * 5 + 14, y0 + 16, 4, pc);
        gfx_pset(x0 + C * 5 + 14, y0 + 16, pc);
    }
}

static void draw_left_panel(void) {
    char buf[24];
    ui_panel(2, 12, 74, 164, C_NIGHT, C_DUSK);
    tiny_draw("NIGHT", 8, 18, C_SLATE);
    snprintf(buf, sizeof buf, "%d/%d", G.round, BM_ROUNDS);
    text_draw(buf, 8, 26, G.round >= BM_ROUNDS ? C_RED : C_WHITE);
    tiny_draw("CLOCK", 8, 42, C_SLATE);
    uint32_t s = run_frames / 60u;
    snprintf(buf, sizeof buf, "%u:%02u", (unsigned)imin(99, (int)(s / 60u)), (unsigned)(s % 60u));
    text_draw(buf, 8, 50, C_LIGHT);
    /* Hazel at her cart */
    int spr = hazel_cheer > 0 && (hazel_cheer / 10) % 2 ? BS_HAZEL_CHEER : (frame_t / 90) % 12 == 0 ? BS_HAZEL2 : BS_HAZEL1;
    gfx_rect(8, 150, 62, 20, C_BROWN);
    gfx_rect(8, 150, 62, 2, C_TAN);
    gfx_circ(18, 170, 5, C_INK);
    gfx_circ(58, 170, 5, C_INK);
    gfx_circ(18, 170, 3, C_EARTH);
    gfx_circ(58, 170, 3, C_EARTH);
    for (int i = 0; i < 4; i++) gfx_rect(12 + i * 8, 144 - (i % 2) * 3, 4, 8 + (i % 2) * 3, i % 3 == 0 ? C_RED : i % 3 == 1 ? C_BLUE : C_JADE);
    spr_draw(&bm_spr[spr], 50, 126, 0);
    int kind = 0, dir = 0, col = 0;
    if (state == S_PLACE) { kind = G.row[place_slot]; dir = pdir; col = G.row_col[place_slot]; }
    else if (state == S_PICK && sel < BM_ROW) { kind = G.row[sel]; col = G.row_col[sel]; }
    if (kind) {
        text_wrap(piece_name(kind), 8, 64, 66, state == S_PLACE ? C_YELLOW : C_CREAM, 9);
        draw_diagram(12, 88, kind, dir, col);
    }
}

static void draw_right_panel(void) {
    ui_panel(244, 12, 74, 164, C_NIGHT, C_DUSK);
    /* the marsh at night, with the Bog King's eyes a little nearer every night */
    gfx_circ(296, 32, 8, C_CREAM);
    gfx_circ(299, 30, 7, C_NIGHT);
    for (int i = 0; i < 18; i++) gfx_pset(250 + (i * 37) % 62, 20 + (i * 23) % 60, (i + frame_t / 30) % 5 ? C_DUSK : C_LIGHT);
    gfx_rect(248, 118, 66, 54, C_TEAL);
    gfx_dither(248, 118, 66, 54, C_FOREST, 8);
    for (int i = 0; i < 14; i++) {
        int rx = 250 + i * 5, top = 104 + (i * 7) % 12;
        gfx_vline(rx, top, 124, C_FOREST);
        gfx_rect(rx - 1, top, 2, 4, C_BROWN);
    }
    int near = iclamp(G.round, 1, BM_ROUNDS);
    int ey = 112 + near * 3, gap = 4 + near;
    if (G.round < BM_ROUNDS && (frame_t / 50) % 7) {
        gfx_rect(281 - gap, ey, 2 + near / 4, 2, C_YELLOW);
        gfx_rect(283 + gap - near / 4, ey, 2 + near / 4, 2, C_YELLOW);
    }
}

static void draw_board(void) {
    int sx = 0, sy = 0;
    if (shake > 0) { sx = (frame_t % 3) - 1; sy = ((frame_t / 2) % 3) - 1; }
    gfx_camera(sx, sy);
    gfx_rect(FX - 3, FY - 3, BM_W * TILE + 6, BM_H * TILE + 6, C_INK);
    gfx_rectb(FX - 2, FY - 2, BM_W * TILE + 4, BM_H * TILE + 4, C_BROWN);
    draw_cobbles();
    draw_things();
    if (state == S_PLACE) {
        int kind = G.row[place_slot];
        draw_preview(kind, pdir);
        if ((frame_t / 6) % 3) bm_draw_piece(FX + cx * TILE, FY + cy * TILE, kind, pdir, G.row_col[place_slot], frame_t);
        draw_cursor(bm_free(&G, cx, cy) ? C_YELLOW : C_RED);
        if (bhold > 4) gfx_rect(FX + cx * TILE, FY + cy * TILE + TILE + 1, TILE * bhold / HOLD_CANCEL, 2, C_RED);
    } else if (state == S_AIM) {
        draw_cursor(G.c[cy][cx].occ == BO_PIECE ? C_ORANGE : C_GREY);
    }
    draw_flights();
    draw_parts();
    gfx_camera(0, 0);
}

static void draw_banner(const char *a, const char *b, int col) {
    int w = imax(text_width(a), text_width(b)) + 24;
    ui_panel(160 - w / 2, 64, w, b[0] ? 32 : 20, C_INK, col);
    text_center(a, 160, 70, C_WHITE);
    if (b[0]) text_center(b, 160, 82, col);
}

static void draw_play(void) {
    gfx_cls(C_INK);
    gfx_rect(0, 0, SCREEN_W, 11, C_NIGHT);
    static const uint8_t grad[] = {C_YELLOW, C_ORANGE, C_RED};
    ui_fancy_center("BOOMTOWN", 160, 2, 1, grad, 3, C_INK, -1);
    draw_left_panel();
    draw_right_panel();
    draw_board();
    draw_row();
    char a[48], b[64];
    if (state == S_NIGHT && state_t < 80) {
        if (G.round >= BM_ROUNDS) draw_banner("THE LAST NIGHT", "THE BOG KING RISES!", C_RED);
        else {
            snprintf(a, sizeof a, "NIGHT %d", G.round);
            if (G.round > 1) snprintf(b, sizeof b, "+%d FIREWORKS", bm_pieces_added(G.round));
            else snprintf(b, sizeof b, "%d FIREWORKS IN THE CART", G.hand);
            draw_banner(a, b, C_AMBER);
        }
    } else if (state == S_RESULT) {
        switch (result) {
        case BR_HOLDS: draw_banner("MOSSBURY HOLDS", "SEE YOU TOMORROW NIGHT", C_LIME); break;
        case BR_CLEARED: draw_banner("NOT A BOGLE LEFT!", "A NEW NEIGHBOUR MOVES IN", C_YELLOW); break;
        case BR_WON: draw_banner("THE BOG KING SINKS!", "MOSSBURY IS SAVED", C_YELLOW); break;
        case BR_LOST: draw_banner("THE BOG KING STANDS", "THE TOWN IS LOST", C_RED); break;
        default: draw_banner("OVERRUN!", "MORE BOGLES THAN FOLK", C_RED); break;
        }
    } else if (state == S_AIM) {
        tiny_center("WHICH FUSE?", 160, ROW_Y - 3, C_ORANGE);
    }
    if (secret_t > 0) {
        gfx_rect(40, 150, 240, 12, C_INK);
        text_center("THE TEA COSY SOCIETY MEETS AT MIDNIGHT", 160, 152, (secret_t / 4) % 2 ? C_PINK : C_WHITE);
    }
}

static void draw_sky_town(int t) {
    for (int y = 0; y < 180; y++) gfx_hline(0, 319, y, y < 60 ? C_INK : y < 110 ? C_NIGHT : C_NAVY);
    gfx_dither(0, 56, 320, 8, C_NIGHT, 8);
    gfx_dither(0, 106, 320, 8, C_NAVY, 8);
    for (int i = 0; i < 40; i++) gfx_pset((i * 83) % 320, (i * 29) % 100, (i + t / 20) % 6 ? C_DUSK : C_LIGHT);
    /* rooftops of Mossbury */
    static const int16_t ROOF[][3] = {{0, 128, 40}, {36, 118, 44}, {76, 132, 34}, {106, 112, 30}, {132, 124, 50},
                                      {178, 116, 36}, {210, 128, 44}, {250, 110, 34}, {280, 122, 40}};
    for (int i = 0; i < ARRAY_LEN(ROOF); i++) {
        int x = ROOF[i][0], y = ROOF[i][1], w = ROOF[i][2];
        gfx_rect(x, y, w, 180 - y, C_INK);
        for (int k = 0; k < w / 2; k++) gfx_hline(x + k, x + w - 1 - k, y - k / 2 - 1, C_INK);
        if ((i + t / 60) % 3) gfx_rect(x + w / 2 - 2, y + 10, 4, 5, C_AMBER);
    }
    gfx_rect(146, 86, 8, 40, C_INK); /* the church spire */
    for (int k = 0; k < 6; k++) gfx_hline(146 + k / 2, 153 - k / 2, 80 + k, C_INK);
}

static void draw_title(void) {
    draw_sky_town(frame_t);
    draw_parts();
    static const uint8_t grad[] = {C_YELLOW, C_AMBER, C_ORANGE, C_RED};
    ui_fancy_center("BOOMTOWN", 160, 14, 3, grad, 4, C_INK, C_WINE);
    spr_draw_scaled(&bm_spr[(frame_t / 40) % 4 == 0 ? BS_HAZEL_CHEER : BS_HAZEL1], 22, 92, 3, 0);
    char buf[48];
    ui_panel(186, 136, 124, 38, C_INK, C_DUSK);
    snprintf(buf, sizeof buf, "BEST SCORE  %u", (unsigned)sv.best_score);
    tiny_draw(buf, 192, 142, C_YELLOW);
    snprintf(buf, sizeof buf, "MOST FOLK   %d", sv.most_folk);
    tiny_draw(buf, 192, 150, C_LIGHT);
    snprintf(buf, sizeof buf, "MOST LEFT   %d", sv.most_left);
    tiny_draw(buf, 192, 158, C_LIGHT);
    if ((frame_t / 24) % 2) text_center("PRESS " GLYPH_A, 160, 60, C_WHITE);
}

static void draw_story(void) {
    draw_sky_town(frame_t);
    ui_panel(24, 20, 272, 110, C_INK, C_AMBER);
    int y = 30;
    y += 9 * text_wrap("EVERY NIGHT THE BOGLES CRAWL OUT OF THE MARSH AND INTO MOSSBURY.", 36, y, 248, C_LIGHT, 9) + 5;
    y += 9 * text_wrap("HAZEL THE FIREWORKS MAKER HAS A CART FULL OF FIREWORKS, AND ONE MATCH A NIGHT.", 36, y, 248, C_LIGHT, 9) + 5;
    y += 9 * text_wrap("SET THEM ROUND THE SQUARE, LIGHT ONE FUSE, AND KEEP AS MANY FOLK AS THERE ARE BOGLES. TEN NIGHTS TO GO.", 36, y, 248, C_CREAM, 9);
    if ((frame_t / 24) % 2) text_center("PRESS " GLYPH_A, 160, 138, C_WHITE);
    spr_draw_scaled(&bm_spr[BS_HAZEL1], 140, 140, 1, 0);
}

static void draw_over(void) {
    draw_play();
    gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
    ui_panel(70, 40, 180, 92, C_INK, C_RED);
    static const uint8_t grad[] = {C_RED, C_WINE, C_MAROON};
    bool king = result == BR_LOST;
    ui_fancy_center(king ? "SWAMPED" : "OVERRUN", 160, 48, 2, grad, 3, C_INK, -1);
    char buf[48];
    snprintf(buf, sizeof buf, "YOU HELD OUT %d NIGHT%s", G.round - 1, G.round - 1 == 1 ? "" : "S");
    text_center(buf, 160, 74, C_LIGHT);
    tiny_center(king ? "THE BOG KING PULLS MOSSBURY INTO THE MARSH" : "THE BOGLES DANCE IN THE SQUARE TILL DAWN", 160, 92, C_GREY);
    if (state_t > 60 && (state_t / 20) % 2) text_center("PRESS " GLYPH_A, 160, 112, C_WHITE);
}

static void draw_win(void) {
    int t = state_t;
    draw_sky_town(t);
    draw_parts();
    spr_draw_scaled(&bm_spr[(t / 12) % 2 ? BS_HAZEL_CHEER : BS_HAZEL1], 22, 110, 2, 0);
    for (int i = 0; i < 4; i++) spr_draw(&bm_spr[BS_FOLK1 + i], 70 + i * 16, 150 - ((t / 8 + i) % 2), i % 2 ? SPR_FLIPX : 0);
    ui_panel(150, 22, 160, 98, C_INK, C_YELLOW);
    text_center("MOSSBURY IS SAVED", 230, 28, C_YELLOW);
    char buf[48];
    int folk = bm_count(&G, BO_FOLK), board = bm_pieces_on_board(&G);
    struct { const char *label; int value; } lines[] = {
        {"VICTORY", 10000},
        {"FOLK", folk * 1000},
        {"ON THE SQUARE", board * 1000},
        {"IN THE CART", G.hand * 1000},
        {"CLOCK", final_bonus},
    };
    for (int i = 0; i < 5; i++) {
        if (t < 30 + i * 25) break;
        tiny_draw(lines[i].label, 158, 42 + i * 9, C_LIGHT);
        snprintf(buf, sizeof buf, "%d", lines[i].value);
        tiny_draw(buf, 300 - tiny_width(buf), 42 + i * 9, C_WHITE);
    }
    if (t > 170) {
        gfx_hline(158, 300, 88, C_DUSK);
        snprintf(buf, sizeof buf, "%d", final_score);
        text_draw("SCORE", 158, 92, C_YELLOW);
        text_draw(buf, 300 - text_width(buf), 92, final_score >= 30000 ? C_YELLOW : C_WHITE);
    }
    if (t > 230) tiny_center("THE END", 230, 108, C_GREY);
    if (t > 320 && (t / 20) % 2) text_center("PRESS " GLYPH_A, 230, 126, C_WHITE);
}

static void draw_sheet(void) {
    gfx_cls(C_DUSK);
    int x = 2, y = 2;
    for (int i = 0; i < BS_SPRITE_COUNT; i++) {
        spr_draw_scaled(&bm_spr[i], x, y, 2, 0);
        x += bm_spr[i].w * 2 + 3;
        if (x > 280) { x = 2; y += 50; }
    }
    for (int k = BM_STARBURST; k < BM_KINDS; k++)
        for (int d = 0; d < 4; d++) bm_draw_piece(4 + (k - 1) * 30, 110 + d * 17, k, d, k == BM_ROCKET || k == BM_PERCH ? 1 + d % 2 : 0, frame_t);
}

static void bm_draw(void) {
    if (sheet_mode) { draw_sheet(); return; }
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_STORY: draw_story(); break;
    case S_OVER: draw_over(); break;
    case S_WIN: draw_win(); break;
    default: draw_play(); break;
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void bm_load(void) {
    bm_art_load();
    bm_audio_load();
}

static void bm_start(void) {
    rng_seed(&seeds, g_rng.state ^ 0xB0057ull);
    load_save();
    memset(&G, 0, sizeof G);
    memset(parts, 0, sizeof parts);
    memset(flights, 0, sizeof flights);
    sheet_mode = false;
    enter(S_TITLE);
    game_set_pausable(false);
    music_play(BM_MUS_TITLE);
}

static void bm_quit(void) {
    /* a run is never saved half-way; its records are */
    if (state >= S_NIGHT && state <= S_RESULT) note_records();
    save_now();
}

static void bm_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_INK);
    for (int i = 0; i < 14; i++) gfx_pset(x + (i * 37) % w, y + (i * 17) % 30, C_DUSK);
    for (int i = 0; i < 4; i++) {
        int bx = x + 18 + i * 34, by = y + 14 + (i % 2) * 8, k = (t / 3 + i * 9) % 40;
        int col[4] = {C_RED, C_YELLOW, C_SKY, C_LIME};
        for (int a = 0; a < 12 && k < 26; a++) {
            float an = (float)a * 0.5236f, r = (float)k * 0.55f;
            gfx_rect(bx + (int)(cosf(an) * r), by + (int)(sinf(an) * r) + k / 8, k < 18 ? 2 : 1, k < 18 ? 2 : 1, a % 3 ? col[i] : C_WHITE);
        }
        if (k >= 26) gfx_vline(bx, by + 30 - (k - 26), by + 32 - (k - 26), C_AMBER);
    }
    for (int i = 0; i < 9; i++) gfx_rect(x + i * 16, y + 40, 16, 24, (i % 2) ? C_SLATE : C_DUSK);
    bm_draw_piece(x + 20, y + 38, BM_STARBURST, 0, 0, t);
    bm_draw_piece(x + 44, y + 38, BM_CANDLE, 1, 0, t);
    spr_draw(&bm_spr[BS_SMALL1 + (t / 12) % 2], x + 84, y + 41, 0);
    spr_draw(&bm_spr[BS_BIG1 + (t / 14) % 2], x + 104, y + 40, 0);
    spr_draw(&bm_spr[BS_FOLK2], x + 124, y + 40, 0);
}

static int cell_arg(const char *key, int skip, int *x, int *y) {
    if (sscanf(key + skip, "%d_%d", x, y) != 2 || *x < 0 || *y < 0 || *x >= BM_W || *y >= BM_H) return 0;
    return 1;
}

static int bm_query(const char *key, int *out) {
    int x, y;
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "round")) { *out = G.round; return 1; }
    if (!strcmp(key, "hand")) { *out = G.hand; return 1; }
    if (!strcmp(key, "sel")) { *out = sel; return 1; }
    if (!strcmp(key, "cx")) { *out = cx; return 1; }
    if (!strcmp(key, "cy")) { *out = cy; return 1; }
    if (!strcmp(key, "pdir")) { *out = pdir; return 1; }
    if (!strcmp(key, "folk")) { *out = bm_count(&G, BO_FOLK); return 1; }
    if (!strcmp(key, "bogles")) { *out = bm_count(&G, BO_BOGLE); return 1; }
    if (!strcmp(key, "holes")) { *out = bm_count(&G, BO_HOLE); return 1; }
    if (!strcmp(key, "pieces")) { *out = bm_pieces_on_board(&G); return 1; }
    if (!strcmp(key, "king")) { *out = bm_count(&G, BO_KING); return 1; }
    if (!strcmp(key, "king_hp")) { *out = G.king_hp; return 1; }
    if (!strcmp(key, "perch_owed")) { *out = G.perch_owed; return 1; }
    if (!strcmp(key, "result")) { *out = result; return 1; }
    if (!strcmp(key, "score")) { *out = final_score; return 1; }
    if (!strcmp(key, "time_bonus")) { *out = final_bonus; return 1; }
    if (!strcmp(key, "secs")) { *out = (int)(run_frames / 60u); return 1; }
    if (!strcmp(key, "secret")) { *out = secret_t; return 1; }
    if (!strcmp(key, "folk_most")) { *out = G.folk_most; return 1; }
    if (!strcmp(key, "fired")) { *out = CH.fired; return 1; }
    if (!strcmp(key, "killed")) { *out = CH.killed; return 1; }
    if (!strcmp(key, "folk_lost")) { *out = CH.folk_lost; return 1; }
    if (!strcmp(key, "king_hits")) { *out = CH.king_hits; return 1; }
    if (!strcmp(key, "chain_live")) { *out = CH.n; return 1; }
    if (!strcmp(key, "bag_i")) { *out = G.bag_i; return 1; }
    if (!strcmp(key, "best_score")) { *out = (int)sv.best_score; return 1; }
    if (!strcmp(key, "most_folk")) { *out = sv.most_folk; return 1; }
    if (!strcmp(key, "most_left")) { *out = sv.most_left; return 1; }
    if (!strcmp(key, "best_nights")) { *out = sv.best_nights; return 1; }
    if (!strcmp(key, "wins")) { *out = sv.wins; return 1; }
    if (!strcmp(key, "art_ok")) { *out = bm_art_ok(); return 1; }
    if (!strncmp(key, "tb_", 3)) { *out = bm_time_bonus((uint32_t)atoi(key + 3) * 60u); return 1; }
    if (!strcmp(key, "small") || !strcmp(key, "big") || !strcmp(key, "old")) {
        int k = key[0] == 's' ? BG_SMALL : key[0] == 'b' ? BG_BIG : BG_OLD, n = 0;
        for (int yy = 0; yy < BM_H; yy++)
            for (int xx = 0; xx < BM_W; xx++) n += G.c[yy][xx].occ == BO_BOGLE && G.c[yy][xx].kind == k;
        *out = n;
        return 1;
    }
    /* deal 100 fresh bags: how many break the one / three / two rule? */
    if (!strcmp(key, "bag_bad")) {
        static BmGame t;
        t = G;
        memset(t.c, 0, sizeof t.c);
        t.perch_owed = 0;
        t.bag_i = 6;
        int bad = 0;
        for (int b = 0; b < 100; b++) {
            int tiers[4] = {0, 0, 0, 0};
            for (int i = 0; i < 6; i++) {
                memset(t.row, 0, sizeof t.row);
                t.hand = 1;
                bm_fill_row(&t);
                tiers[bm_tier(t.row[0])]++;
            }
            bad += !(tiers[1] == 1 && tiers[2] == 3 && tiers[3] == 2);
        }
        *out = bad;
        return 1;
    }
    if (!strcmp(key, "bot")) { *out = bot_buttons(); return 1; }
    if (!strncmp(key, "row", 3) && key[3] >= '0' && key[3] < '0' + BM_ROW && !key[4]) { *out = G.row[key[3] - '0']; return 1; }
    if (!strncmp(key, "rowcol", 6) && key[6] >= '0' && key[6] < '0' + BM_ROW && !key[7]) { *out = G.row_col[key[6] - '0']; return 1; }
    if (!strncmp(key, "occ_", 4) && cell_arg(key, 4, &x, &y)) { *out = G.c[y][x].occ; return 1; }
    if (!strncmp(key, "kind_", 5) && cell_arg(key, 5, &x, &y)) { *out = G.c[y][x].kind; return 1; }
    if (!strncmp(key, "hp_", 3) && cell_arg(key, 3, &x, &y)) { *out = G.c[y][x].hp; return 1; }
    if (!strncmp(key, "dir_", 4) && cell_arg(key, 4, &x, &y)) { *out = G.c[y][x].dir; return 1; }
    if (!strncmp(key, "col_", 4) && cell_arg(key, 4, &x, &y)) { *out = G.c[y][x].col; return 1; }
    /* the tiles a firework of a kind would hit from (x,y) facing d: "hits_K_D_X_Y" */
    if (!strncmp(key, "hits_", 5)) {
        int k, d;
        if (sscanf(key + 5, "%d_%d_%d_%d", &k, &d, &x, &y) != 4) return 0;
        int8_t t[BM_W + BM_H + 8][2];
        *out = bm_attack(&G, x, y, k, d, t);
        return 1;
    }
    /* bag statistics: deal N bags and count each kind (tests of the bag) */
    if (!strncmp(key, "dealt_", 6)) {
        int k = atoi(key + 6);
        static BmGame t;
        t = G;
        memset(t.c, 0, sizeof t.c);
        t.perch_owed = 0;
        t.bag_i = 6;
        int n = 0;
        for (int i = 0; i < 600; i++) {
            t.hand = 3;
            memset(t.row, 0, sizeof t.row);
            memset(t.row_col, 0, sizeof t.row_col);
            bm_fill_row(&t);
            n += t.row[0] == k;
            t.row[1] = t.row[2] = 0;
        }
        *out = n;
        return 1;
    }
    return 0;
}

static int bm_cheat(const char *cmd) {
    int a, b, c, d, e;
    char name[16];
    if (!strcmp(cmd, "new")) { new_game(); return 1; }
    if (!strcmp(cmd, "sheet")) { sheet_mode = !sheet_mode; return 1; }
    if (!strcmp(cmd, "clear")) {
        /* an empty square (nothing on it) */
        memset(G.c, 0, sizeof G.c);
        G.perch_owed = 0;
        G.king_hp = 0;
        memset(born, 255, sizeof born);
        return 1;
    }
    if (sscanf(cmd, "put %15s %d %d %d %d %d", name, &a, &b, &c, &d, &e) >= 3) {
        if (a < 0 || b < 0 || a >= BM_W || b >= BM_H) return 0;
        BmCell *cl = &G.c[b][a];
        int n = sscanf(cmd, "put %15s %d %d %d %d %d", name, &a, &b, &c, &d, &e);
        if (!strcmp(name, "folk")) *cl = (BmCell){BO_FOLK, 0, 0, 1, 0, (uint8_t)(a & 3)};
        else if (!strcmp(name, "hole")) *cl = (BmCell){BO_HOLE, 0, 0, 0, 0, 0};
        else if (!strcmp(name, "empty")) *cl = (BmCell){0};
        else if (!strcmp(name, "bogle")) { /* put bogle X Y KIND [HP] */
            int k = n >= 4 ? iclamp(c, 0, 2) : 0;
            *cl = (BmCell){BO_BOGLE, (uint8_t)k, 0, (uint8_t)(n >= 5 ? d : k == 0 ? 1 : 2), 0, 0};
        } else if (!strcmp(name, "piece")) { /* put piece X Y KIND [DIR] [COL] */
            if (n < 4) return 0;
            *cl = (BmCell){BO_PIECE, (uint8_t)c, (uint8_t)(n >= 5 ? d & 3 : 0), 0, (uint8_t)(n >= 6 ? e : 0), 0};
        } else return 0;
        born[b][a] = 255;
        return 1;
    }
    if (sscanf(cmd, "row %d %d %d", &a, &b, &c) == 3) {
        G.row[0] = (uint8_t)a; G.row[1] = (uint8_t)b; G.row[2] = (uint8_t)c;
        for (int i = 0; i < BM_ROW; i++) G.row_col[i] = G.row[i] == BM_ROCKET ? (uint8_t)(i == 2 ? 2 : 1) : 0;
        return 1;
    }
    if (sscanf(cmd, "hand %d", &a) == 1) { G.hand = (int16_t)a; return 1; }
    if (sscanf(cmd, "clock %d", &a) == 1) { run_frames = (uint32_t)a * 60u; return 1; }
    if (sscanf(cmd, "night %d", &a) == 1) {
        /* jump to the start of night A (the square as it is) */
        G.round = (uint8_t)(a - 1);
        begin_night();
        return 1;
    }
    if (sscanf(cmd, "cursor %d %d", &a, &b) == 2) { cx = iclamp(a, 0, BM_W - 1); cy = iclamp(b, 0, BM_H - 1); return 1; }
    if (sscanf(cmd, "sel %d", &a) == 1) { sel = iclamp(a, 0, BM_ROW); return 1; }
    if (!strcmp(cmd, "pick")) { to_pick(); return 1; }
    return 0;
}

const GameDef GAME_BOOMTOWN = {
    "boomtown",
    "BOOMTOWN",
    "1984",
    "PUZZLE",
    "LIGHT ONE FUSE A NIGHT AND BLOW THE BOGLES BACK INTO THE MARSH.",
    {"HOLD OUT FOR 5 NIGHTS", "BEAT THE BOG KING", "WIN WITH 30,000 POINTS"},
    GLYPH_LEFT GLYPH_RIGHT "\tCHOOSE A FIREWORK\n"
    GLYPH_A "\tTAKE IT / SET IT DOWN\n"
    GLYPH_B "\tTURN IT\n"
    "HOLD " GLYPH_B "\tPUT IT BACK\n"
    "LIGHT\tPICK ONE FUSE, " GLYPH_A "\n"
    "START\tPAUSE",
    C_WINE, C_AMBER,
    bm_load, bm_start, bm_update, bm_draw, bm_quit, bm_label, bm_query, bm_cheat,
    "DEVILITION", 10,
};
